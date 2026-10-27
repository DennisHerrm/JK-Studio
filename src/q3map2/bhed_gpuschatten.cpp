// bhed_gpuschatten.cpp - BehavEd Stufe 8b Punkt 9: Schattentests auf der GPU
// (siehe bhed_gpuschatten.h). Der Compute-Shader ist TraceLine_r und
// TraceTriangle aus q3map2/light_trace.cpp, Zeile fuer Zeile:
//
//   - jede Rechnung "precise" (keine Zusammenfassung zu mad, keine Umordnung),
//     dot und cross wie libs/math/vector.h von links nach rechts;
//   - die Divisionen (frac = front / (front - back), invDet = 1 / det) genau
//     gerundet mit Ganzzahlen - die Division von Direct3D 11 darf 2,5 ULP
//     abweichen;
//   - Himmel-Dreiecke aendern die Entscheidung nicht (TraceTriangle gibt dort
//     immer false zurueck) und werden uebersprungen; ein Alpha-/Filter-Dreieck,
//     das die Geometrie-Tests besteht, gibt Status 3: die CPU rechnet den Strahl.
//
//   Copyright (C) 1999-2007 id Software, Inc. and contributors.
//   Copyright (C) 2026 BehavEd-Remake contributors.
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
#include "q3map2.h"
#include "bhed_gpuschatten.h"

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>

namespace
{

const char *const g_shader = R"HLSL(
struct Knoten { int typ; float nx; float ny; float nz; float dist; int k0; int k1; int anzahl; int erstes; };
struct Dreieck { float v0x; float v0y; float v0z; float e1x; float e1y; float e1z; float e2x; float e2y; float e2z; int info; };
struct Info { int cast; int skipGrid; int flaeche; int flags; };
struct Strahl { float ox; float oy; float oz; float ex; float ey; float ez; float dx; float dy; float dz; float dist; float inhibit;
                int recv; int sErste; int sAnzahl; int flags; int rest; };
struct Ergebnis { int status; float tiefe; float hx; float hy; float hz; int p0; int p1; int p2; };
cbuffer Kopf : register(b0) { uint anzahl; int kopfKnoten; float baryEps; float baryEins; float coplanar; float selbst; float traceOn; float aslfFrei; };
StructuredBuffer<Knoten> knoten : register(t0);
StructuredBuffer<int> items : register(t1);
StructuredBuffer<Dreieck> dreiecke : register(t2);
StructuredBuffer<Info> infos : register(t3);
StructuredBuffer<int> flaechen : register(t4);
StructuredBuffer<Strahl> strahlen : register(t5);
RWStructuredBuffer<Ergebnis> aus : register(u0);

static bool g_unsicher;

// IEEE-754 float-Division, genau gerundet (round to nearest even) - nur normale Ergebnisse
float teilen(float a, float b)
{
    uint ua = asuint(a);
    uint ub = asuint(b);
    uint s = (ua ^ ub) & 0x80000000u;
    int ea = int((ua >> 23) & 0xffu);
    int eb = int((ub >> 23) & 0xffu);
    uint ma = ua & 0x7fffffu;
    uint mb = ub & 0x7fffffu;
    if (ea == 255 || eb == 255 || (eb == 0 && mb == 0)) { g_unsicher = true; return 0.0f; }
    if (ea == 0 && ma == 0) { return asfloat(s); }
    if (ea == 0) { ea = 1; [loop] while ((ma & 0x800000u) == 0u) { ma <<= 1; ea -= 1; } } else { ma |= 0x800000u; }
    if (eb == 0) { eb = 1; [loop] while ((mb & 0x800000u) == 0u) { mb <<= 1; eb -= 1; } } else { mb |= 0x800000u; }
    int e = ea - eb + 127;
    if (ma < mb) { ma <<= 1; e -= 1; }
    uint q = 0u;
    uint r = ma;
    [loop] for (int k = 0; k < 25; ++k) { q <<= 1; if (r >= mb) { r -= mb; q |= 1u; } r <<= 1; }
    uint guard = q & 1u;
    q >>= 1;
    if (guard != 0u && (r != 0u || (q & 1u) != 0u)) { q += 1u; if (q == 0x1000000u) { q >>= 1; e += 1; } }
    if (e <= 0 || e >= 255) { g_unsicher = true; return 0.0f; }
    return asfloat(s | (uint(e) << 23) | (q & 0x7fffffu));
}

float punkt3(float3 a, float3 b) { precise float d = a.x * b.x + a.y * b.y + a.z * b.z; return d; }
float3 kreuz(float3 a, float3 b)
{
    precise float x = a.y * b.z - a.z * b.y;
    precise float y = a.z * b.x - a.x * b.z;
    precise float z = a.x * b.y - a.y * b.x;
    return float3(x, y, z);
}

// TraceTriangle fuer alle Dreiecke eines Blatts: 0 nichts, 1 undurchsichtig (tiefe), 3 die CPU
int blattTesten(int knr, Strahl r, float3 start, float3 richtung, inout float tiefeAus)
{
    const Knoten n = knoten[knr];
    [loop] for (int j = 0; j < n.anzahl; ++j)
    {
        const Dreieck d = dreiecke[items[n.erstes + j]];
        const Info f = infos[d.info];
        if ((f.flags & 1) != 0) { continue; }   // Himmel: TraceTriangle gibt immer false
        if (r.recv == 1) { if (f.cast <= 0) { continue; } }
        else if (r.recv > 1) { if (f.cast != 1 && abs(f.cast) != abs(r.recv)) { continue; } }
        else { if (abs(f.cast) != abs(r.recv)) { continue; } }
        if ((r.flags & 1) != 0 && f.skipGrid != 0) { continue; }
        const float3 v0 = float3(d.v0x, d.v0y, d.v0z);
        const float3 e1 = float3(d.e1x, d.e1y, d.e1z);
        const float3 e2 = float3(d.e2x, d.e2y, d.e2z);
        precise float3 pvec = kreuz(richtung, e2);
        precise float det = punkt3(e1, pvec);
        if (abs(det) < coplanar) { continue; }
        precise float3 tvec = start - v0;
        precise float uu = punkt3(tvec, pvec);
        // Vorpruefung mit der Hardware-Division: nur verwerfen, was auch genau gerechnet
        // sicher verworfen wird (Abstand zur Grenze 1e-3 relativ, die Division weicht
        // hoechstens wenige ULP ab); alles andere rechnet unten genau.
        {
            const float invA = 1.0f / det;
            const float uA = uu * invA;
            const float tolU = 1e-3f * max(1.0f, abs(uA));
            if (uA < -baryEps - tolU || uA > baryEins + tolU) { continue; }
        }
        precise float invDet = teilen(1.0f, det);
        precise float u = uu * invDet;
        if (u < -baryEps || u > baryEins) { continue; }
        precise float3 qvec = kreuz(tvec, e1);
        precise float vv = punkt3(richtung, qvec);
        precise float v = vv * invDet;
        precise float uv = u + v;
        if (v < -baryEps || uv > baryEins) { continue; }
        precise float tt = punkt3(e2, qvec);
        precise float tiefe = tt * invDet;
        if (tiefe <= r.inhibit || tiefe >= r.dist) { continue; }
        if (tiefe <= selbst)
        {
            bool eigen = false;
            [loop] for (int s = 0; s < r.sAnzahl; ++s) { if (flaechen[r.sErste + s] == f.flaeche) { eigen = true; break; } }
            if (eigen) { continue; }
        }
        if (g_unsicher) { return 3; }
        if ((f.flags & 2) != 0) { return 3; }   // Alpha-Schatten / Lichtfilter: die CPU
        tiefeAus = tiefe;
        return 1;
    }
    return g_unsicher ? 3 : 0;
}

// TraceLine: die Blaetter werden gleich beim Erreichen getestet (die ersten 256 mit
// Dreiecken, MAX_TRACE_TEST_NODES). Fuer die Entscheidung ist das gleichwertig:
// undurchsichtig, wenn ein festes Blatt erreicht wird ODER ein undurchsichtiges Dreieck
// eines der ersten 256 Blaetter die Tests besteht; ein Alpha-Dreieck gibt Status 3.
[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    const uint i = id.x;
    if (i >= anzahl) { return; }
    g_unsicher = false;
    const Strahl r = strahlen[i];
    if (r.rest != 0) { Ergebnis x; x.status = 3; x.tiefe = 0.0f; x.hx = 0.0f; x.hy = 0.0f; x.hz = 0.0f; x.p0 = 0; x.p1 = 0; x.p2 = 0; aus[i] = x; return; }
    precise float3 start = float3(r.ox, r.oy, r.oz);
    precise float3 ende = float3(r.ex, r.ey, r.ez);
    precise float3 richtung = float3(r.dx, r.dy, r.dz);
    Ergebnis e;
    e.status = 0; e.tiefe = 0.0f; e.hx = 0.0f; e.hy = 0.0f; e.hz = 0.0f; e.p0 = 0; e.p1 = 0; e.p2 = 0;
    const bool flaechenTesten = (r.flags & 2) == 0;   // noSurfaces

    // TraceLine_r (halb iterativ; die zweite Seite als Fortsetzung auf dem Stapel)
    int nTest = 0;
    int stK[96];
    float3 stO[96];
    float3 stE[96];
    int sp = 0;
    int k = kopfKnoten;
    precise float3 o = start;
    precise float3 en = ende;
    [loop] while (true)
    {
        if (k < 0) { e.status = 2; e.hx = o.x; e.hy = o.y; e.hz = o.z; break; }
        const Knoten n = knoten[k];
        if (n.typ == -2) { e.status = 2; e.hx = o.x; e.hy = o.y; e.hz = o.z; break; }
        if (n.typ < 0)
        {
            if (n.anzahl > 0 && nTest < 256)
            {
                nTest += 1;
                if (flaechenTesten)
                {
                    float tiefe = 0.0f;
                    const int b = blattTesten(k, r, start, richtung, tiefe);
                    if (b == 1) { e.status = 1; e.tiefe = tiefe; break; }
                    if (b == 3) { e.status = 3; break; }
                }
            }
            if (sp == 0) { break; }
            sp -= 1;
            k = stK[sp]; o = stO[sp]; en = stE[sp];
            continue;
        }
        precise float vorn;
        precise float hinten;
        if (n.typ == 0) { vorn = o.x - n.dist; hinten = en.x - n.dist; }
        else if (n.typ == 1) { vorn = o.y - n.dist; hinten = en.y - n.dist; }
        else if (n.typ == 2) { vorn = o.z - n.dist; hinten = en.z - n.dist; }
        else
        {
            const float3 nn = float3(n.nx, n.ny, n.nz);
            precise float a = punkt3(o, nn);
            precise float b = punkt3(en, nn);
            vorn = a - n.dist;
            hinten = b - n.dist;
        }
        if (vorn >= -traceOn && hinten >= -traceOn) { k = n.k0; continue; }
        if (vorn < traceOn && hinten < traceOn) { k = n.k1; continue; }
        const int seite = vorn < 0.0f ? 1 : 0;
        precise float nenner = vorn - hinten;
        precise float frac = teilen(vorn, nenner);
        precise float3 mitte = o + (en - o) * frac;
        if (sp >= 96) { g_unsicher = true; break; }
        stK[sp] = seite == 1 ? n.k0 : n.k1; stO[sp] = mitte; stE[sp] = en; sp += 1;
        k = seite == 1 ? n.k1 : n.k0;
        en = mitte;
    }
    if (g_unsicher) { e.status = 3; }
    aus[i] = e;
}
)HLSL";

struct Gpu
{
	HMODULE d3d = nullptr, compiler = nullptr;
	ID3D11Device *geraet = nullptr;
	ID3D11DeviceContext *ktx = nullptr;
	ID3D11ComputeShader *cs = nullptr;
	ID3D11Buffer *puffer[ 5 ] = {};
	ID3D11ShaderResourceView *srv[ 5 ] = {};
	ID3D11Buffer *kopf = nullptr;
	ID3D11Buffer *strahlen = nullptr, *ergebnis = nullptr, *lesen = nullptr;
	ID3D11ShaderResourceView *strahlenSrv = nullptr;
	ID3D11UnorderedAccessView *ergebnisUav = nullptr;
	UINT kapazitaet = 0;
	int kopfKnoten = 0;
	float konst[ 5 ] = {};
	std::mutex sperre;
	std::string name;
};
Gpu g_gpu;
std::atomic<int> g_nutzer{ 0 };   // Faeden, die gerade die GPU nutzen oder darauf warten
std::atomic<long long> g_zBelegt{ 0 };
std::atomic<long long> g_zStrahlen{ 0 }, g_zFrei{ 0 }, g_zVerdeckt{ 0 }, g_zFest{ 0 }, g_zCpu{ 0 }, g_zAbweichung{ 0 }, g_zBloecke{ 0 }, g_zGpuMikros{ 0 };

template<typename T>
void frei( T *&p ){
	if ( p != nullptr ) {
		p->Release();
		p = nullptr;
	}
}

bool strukturPuffer( const void *daten, UINT stride, UINT anzahl, ID3D11Buffer *&buf, ID3D11ShaderResourceView *&srv ){
	if ( anzahl == 0 ) {
		anzahl = 1;   // leere Liste: ein Platzhalter
	}
	std::vector<unsigned char> leer;
	if ( daten == nullptr ) {
		leer.assign( size_t( stride ) * anzahl, 0 );
		daten = leer.data();
	}
	D3D11_BUFFER_DESC bd{};
	bd.ByteWidth = stride * anzahl;
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = stride;
	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = daten;
	if ( FAILED( g_gpu.geraet->CreateBuffer( &bd, &sd, &buf ) ) ) {
		return false;
	}
	D3D11_SHADER_RESOURCE_VIEW_DESC vd{};
	vd.Format = DXGI_FORMAT_UNKNOWN;
	vd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	vd.Buffer.NumElements = anzahl;
	return SUCCEEDED( g_gpu.geraet->CreateShaderResourceView( buf, &vd, &srv ) );
}

bool kapazitaet( UINT n ){
	if ( n <= g_gpu.kapazitaet ) {
		return true;
	}
	frei( g_gpu.strahlenSrv );
	frei( g_gpu.ergebnisUav );
	frei( g_gpu.strahlen );
	frei( g_gpu.ergebnis );
	frei( g_gpu.lesen );
	UINT k = 65536;
	while ( k < n ) {
		k *= 2;
	}
	D3D11_BUFFER_DESC bd{};
	bd.ByteWidth = UINT( sizeof( BhedGpuStrahl ) ) * k;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof( BhedGpuStrahl );
	if ( FAILED( g_gpu.geraet->CreateBuffer( &bd, nullptr, &g_gpu.strahlen ) ) ) {
		return false;
	}
	D3D11_SHADER_RESOURCE_VIEW_DESC vd{};
	vd.Format = DXGI_FORMAT_UNKNOWN;
	vd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	vd.Buffer.NumElements = k;
	if ( FAILED( g_gpu.geraet->CreateShaderResourceView( g_gpu.strahlen, &vd, &g_gpu.strahlenSrv ) ) ) {
		return false;
	}
	bd.ByteWidth = UINT( sizeof( BhedGpuErgebnis ) ) * k;
	bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	bd.StructureByteStride = sizeof( BhedGpuErgebnis );
	if ( FAILED( g_gpu.geraet->CreateBuffer( &bd, nullptr, &g_gpu.ergebnis ) ) ) {
		return false;
	}
	D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};
	ud.Format = DXGI_FORMAT_UNKNOWN;
	ud.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	ud.Buffer.NumElements = k;
	if ( FAILED( g_gpu.geraet->CreateUnorderedAccessView( g_gpu.ergebnis, &ud, &g_gpu.ergebnisUav ) ) ) {
		return false;
	}
	bd.Usage = D3D11_USAGE_STAGING;
	bd.BindFlags = 0;
	bd.MiscFlags = 0;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	if ( FAILED( g_gpu.geraet->CreateBuffer( &bd, nullptr, &g_gpu.lesen ) ) ) {
		return false;
	}
	g_gpu.kapazitaet = k;
	return true;
}

void allesFrei(){
	for ( int i = 0; i < 5; ++i ) {
		frei( g_gpu.srv[ i ] );
		frei( g_gpu.puffer[ i ] );
	}
	frei( g_gpu.strahlenSrv );
	frei( g_gpu.ergebnisUav );
	frei( g_gpu.strahlen );
	frei( g_gpu.ergebnis );
	frei( g_gpu.lesen );
	frei( g_gpu.kopf );
	frei( g_gpu.cs );
	frei( g_gpu.ktx );
	frei( g_gpu.geraet );
	g_gpu.kapazitaet = 0;
}

} // namespace

bool BhedGpuStart( const std::vector<BhedGpuKnoten>& knoten, const std::vector<int>& items, const std::vector<BhedGpuDreieck>& dreiecke,
                   const std::vector<BhedGpuInfo>& infos, const int *flaechen, int anzahlFlaechen, int kopfKnoten, const float konstanten[ 5 ] ){
	g_gpu.d3d = LoadLibraryW( L"d3d11.dll" );
	g_gpu.compiler = LoadLibraryW( L"d3dcompiler_47.dll" );
	if ( g_gpu.d3d == nullptr || g_gpu.compiler == nullptr ) {
		Sys_Printf( "BehavEd GPU: d3d11.dll or d3dcompiler_47.dll missing - CPU\n" );
		return false;
	}
	const auto erzeugen = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>( reinterpret_cast<void *>( GetProcAddress( g_gpu.d3d, "D3D11CreateDevice" ) ) );
	const auto kompilieren = reinterpret_cast<pD3DCompile>( reinterpret_cast<void *>( GetProcAddress( g_gpu.compiler, "D3DCompile" ) ) );
	if ( erzeugen == nullptr || kompilieren == nullptr ) {
		return false;
	}
	// Der Adapter mit dem meisten eigenen Grafikspeicher (Notebook: die schnelle GPU statt
	// der eingebauten, die Windows als erste nennt); dxgi.dll ebenfalls zur Laufzeit
	IDXGIAdapter1 *adapter = nullptr;
	if ( HMODULE dxgi = LoadLibraryW( L"dxgi.dll" ); dxgi != nullptr ) {
		using Fabrik = HRESULT( WINAPI * )( REFIID, void ** );
		const auto fabrik = reinterpret_cast<Fabrik>( reinterpret_cast<void *>( GetProcAddress( dxgi, "CreateDXGIFactory1" ) ) );
		IDXGIFactory1 *f = nullptr;
		if ( fabrik != nullptr && SUCCEEDED( fabrik( __uuidof( IDXGIFactory1 ), reinterpret_cast<void **>( &f ) ) ) ) {
			SIZE_T bester = 0;
			IDXGIAdapter1 *a = nullptr;
			for ( UINT i = 0; f->EnumAdapters1( i, &a ) != DXGI_ERROR_NOT_FOUND; ++i ) {
				DXGI_ADAPTER_DESC1 d{};
				if ( SUCCEEDED( a->GetDesc1( &d ) ) && !( d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ) && ( adapter == nullptr || d.DedicatedVideoMemory > bester ) ) {
					frei( adapter );
					adapter = a;
					bester = d.DedicatedVideoMemory;
					continue;
				}
				a->Release();
			}
			f->Release();
		}
	}
	const D3D_FEATURE_LEVEL stufen[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL stufe{};
	const HRESULT erzeugt = erzeugen( adapter, adapter != nullptr ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, stufen, 2, D3D11_SDK_VERSION,
	                                  &g_gpu.geraet, &stufe, &g_gpu.ktx );
	frei( adapter );
	if ( FAILED( erzeugt ) ) {
		Sys_Printf( "BehavEd GPU: no Direct3D 11 hardware device - CPU\n" );
		return false;
	}
	{
		IDXGIDevice *dx = nullptr;
		if ( SUCCEEDED( g_gpu.geraet->QueryInterface( __uuidof( IDXGIDevice ), reinterpret_cast<void **>( &dx ) ) ) ) {
			IDXGIAdapter *ad = nullptr;
			if ( SUCCEEDED( dx->GetAdapter( &ad ) ) ) {
				DXGI_ADAPTER_DESC d{};
				if ( SUCCEEDED( ad->GetDesc( &d ) ) ) {
					char z[ 256 ] = {};
					WideCharToMultiByte( CP_UTF8, 0, d.Description, -1, z, sizeof( z ) - 1, nullptr, nullptr );
					g_gpu.name = z;
				}
				ad->Release();
			}
			dx->Release();
		}
	}
	ID3DBlob *code = nullptr;
	ID3DBlob *fehler = nullptr;
	const UINT flags = D3DCOMPILE_IEEE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
	if ( FAILED( kompilieren( g_shader, strlen( g_shader ), "bhed_gpuschatten", nullptr, nullptr, "main", "cs_5_0", flags, 0, &code, &fehler ) ) ) {
		Sys_Printf( "BehavEd GPU: shader error: %s\n", fehler != nullptr ? static_cast<const char *>( fehler->GetBufferPointer() ) : "?" );
		frei( fehler );
		allesFrei();
		return false;
	}
	frei( fehler );
	const HRESULT hr = g_gpu.geraet->CreateComputeShader( code->GetBufferPointer(), code->GetBufferSize(), nullptr, &g_gpu.cs );
	code->Release();
	if ( FAILED( hr ) ) {
		allesFrei();
		return false;
	}
	bool ok = strukturPuffer( knoten.data(), sizeof( BhedGpuKnoten ), UINT( knoten.size() ), g_gpu.puffer[ 0 ], g_gpu.srv[ 0 ] ) &&
	          strukturPuffer( items.data(), sizeof( int ), UINT( items.size() ), g_gpu.puffer[ 1 ], g_gpu.srv[ 1 ] ) &&
	          strukturPuffer( dreiecke.data(), sizeof( BhedGpuDreieck ), UINT( dreiecke.size() ), g_gpu.puffer[ 2 ], g_gpu.srv[ 2 ] ) &&
	          strukturPuffer( infos.data(), sizeof( BhedGpuInfo ), UINT( infos.size() ), g_gpu.puffer[ 3 ], g_gpu.srv[ 3 ] ) &&
	          strukturPuffer( anzahlFlaechen > 0 ? flaechen : nullptr, sizeof( int ), UINT( anzahlFlaechen ), g_gpu.puffer[ 4 ], g_gpu.srv[ 4 ] );
	if ( ok ) {
		D3D11_BUFFER_DESC bd{};
		bd.ByteWidth = 32;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		ok = SUCCEEDED( g_gpu.geraet->CreateBuffer( &bd, nullptr, &g_gpu.kopf ) ) && kapazitaet( 65536 );
	}
	if ( !ok ) {
		Sys_Printf( "BehavEd GPU: buffers failed - CPU\n" );
		allesFrei();
		return false;
	}
	g_gpu.kopfKnoten = kopfKnoten;
	memcpy( g_gpu.konst, konstanten, sizeof( g_gpu.konst ) );
	Sys_Printf( "BehavEd GPU shadow tests: %s (feature level %x), %zu nodes, %zu triangles\n", g_gpu.name.c_str(), unsigned( stufe ), knoten.size(), dreiecke.size() );
	return true;
}

void BhedGpuRechnen( BhedGpuSitzung& s ){
	const size_t n = s.strahlen.size();
	s.erg.resize( n );
	bool ok = false;
	if ( n > 0 ) {
		struct Nutzer {
			Nutzer(){ ++g_nutzer; }
			~Nutzer(){ --g_nutzer; }
		} nutzer;
		std::lock_guard<std::mutex> l( g_gpu.sperre );
		const auto t0 = std::chrono::steady_clock::now();
		if ( kapazitaet( UINT( n ) ) ) {
			D3D11_BOX box{ 0, 0, 0, UINT( n * sizeof( BhedGpuStrahl ) ), 1, 1 };
			g_gpu.ktx->UpdateSubresource( g_gpu.strahlen, 0, &box, s.strahlen.data(), 0, 0 );
			struct { UINT anzahl; int kopf; float k[ 6 ]; } kopf{ UINT( n ), g_gpu.kopfKnoten, { g_gpu.konst[ 0 ], g_gpu.konst[ 1 ], g_gpu.konst[ 2 ], g_gpu.konst[ 3 ], g_gpu.konst[ 4 ], 0 } };
			g_gpu.ktx->UpdateSubresource( g_gpu.kopf, 0, nullptr, &kopf, 0, 0 );
			ID3D11ShaderResourceView *srv[ 6 ] = { g_gpu.srv[ 0 ], g_gpu.srv[ 1 ], g_gpu.srv[ 2 ], g_gpu.srv[ 3 ], g_gpu.srv[ 4 ], g_gpu.strahlenSrv };
			g_gpu.ktx->CSSetShader( g_gpu.cs, nullptr, 0 );
			g_gpu.ktx->CSSetShaderResources( 0, 6, srv );
			g_gpu.ktx->CSSetUnorderedAccessViews( 0, 1, &g_gpu.ergebnisUav, nullptr );
			g_gpu.ktx->CSSetConstantBuffers( 0, 1, &g_gpu.kopf );
			g_gpu.ktx->Dispatch( UINT( ( n + 63 ) / 64 ), 1, 1 );
			ID3D11UnorderedAccessView *keine = nullptr;
			g_gpu.ktx->CSSetUnorderedAccessViews( 0, 1, &keine, nullptr );
			D3D11_BOX box2{ 0, 0, 0, UINT( n * sizeof( BhedGpuErgebnis ) ), 1, 1 };
			g_gpu.ktx->CopySubresourceRegion( g_gpu.lesen, 0, 0, 0, 0, g_gpu.ergebnis, 0, &box2 );
			D3D11_MAPPED_SUBRESOURCE m{};
			if ( SUCCEEDED( g_gpu.ktx->Map( g_gpu.lesen, 0, D3D11_MAP_READ, 0, &m ) ) ) {
				memcpy( s.erg.data(), m.pData, n * sizeof( BhedGpuErgebnis ) );
				g_gpu.ktx->Unmap( g_gpu.lesen, 0 );
				ok = true;
			}
		}
		g_zGpuMikros += std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::steady_clock::now() - t0 ).count();
		++g_zBloecke;
	}
	if ( !ok ) {
		for ( BhedGpuErgebnis& e : s.erg ) {
			e = BhedGpuErgebnis{ 3, 0, { 0, 0, 0 }, { 0, 0, 0 } };
		}
	}
}

void BhedGpuBelegt(){
	++g_zBelegt;
}

bool BhedGpuFrei(){
	return g_nutzer.load() < 2;
}

void BhedGpuZaehlen( int status, bool abweichung ){
	if ( abweichung ) {
		++g_zAbweichung;
		return;
	}
	++g_zStrahlen;
	switch ( status ) {
	case 0: ++g_zFrei; break;
	case 1: ++g_zVerdeckt; break;
	case 2: ++g_zFest; break;
	default: ++g_zCpu; break;
	}
}

void BhedGpuEnde(){
	if ( g_gpu.geraet != nullptr ) {
		Sys_Printf( "BehavEd GPU: %lld rays (free %lld, blocked %lld, solid %lld, on the CPU %lld), %lld input mismatches, %lld blocks (%lld left to the CPU: GPU busy), GPU %.2f s\n",
		            g_zStrahlen.load(), g_zFrei.load(), g_zVerdeckt.load(), g_zFest.load(), g_zCpu.load(), g_zAbweichung.load(), g_zBloecke.load(),
		            g_zBelegt.load(), double( g_zGpuMikros.load() ) / 1e6 );
	}
	allesFrei();
	g_bhedGpu = false;
}
