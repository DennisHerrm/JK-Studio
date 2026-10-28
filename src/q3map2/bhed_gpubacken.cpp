// bhed_gpubacken.cpp - BehavEd Stufe 8c/8g: GPU-Backen fuer q3map2 -light
// (siehe bhed_gpubacken.h). Die Compute-Shader (bhed_gpubacken.hlsl) sind
// LightContributionToSample, SetupTrace, TraceLine/TraceLine_r/TraceTriangle,
// PointToPolygonFormFactor, SubsampleRawLuxel_r, SubmapRawLuxel, ClusterForPointExt und
// die Lichtschleife von IlluminateRawLightmap aus q3map2 (light.cpp, light_trace.cpp,
// light_ydnar.cpp), Zeile fuer Zeile:
//
//   - float-Rechnungen "precise" in der Reihenfolge von MSVC /fp:precise (dot und
//     cross wie libs/math/vector.h von links nach rechts, keine mad-Zusammenfassung);
//   - float-Division genau gerundet mit Ganzzahlen (teilen, wie Stufe 8b);
//   - VectorNormalize von q3map2 rechnet in double: hier ebenso, die Wurzel genau
//     gerundet (Newton, dann der Test nach Tuckerman mit fma); float-Wurzeln als
//     genau gerundete double-Wurzel, auf float gerundet (53 >= 2*24+2: kein
//     schaedliches doppeltes Runden); acos in double nach fdlibm;
//   - GPUs ohne double (ExtendedDoublesShaderInstructions): float-Naeherungen,
//     der Bericht nennt es.
//
//   Copyright (C) 1999-2007 id Software, Inc. and contributors.
//   fdlibm e_acos.c: Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
//   Developed at SunSoft, a Sun Microsystems, Inc. business. Permission to use, copy,
//   modify, and distribute this software is freely granted, provided that this notice
//   is preserved.
//   Copyright (C) 2026 BehavEd-Remake contributors.
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
#include "q3map2.h"
#include "bhed_gpubacken.h"

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_6.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <limits>
#include <string>
#include <thread>

// Hybrid-Notebooks: die schnelle GPU fuer diese exe (wie jkstudio.exe, gui/main_win32.cpp).
// Die Exporte muessen in der exe stehen, nicht in einer DLL.
extern "C" {
__declspec( dllexport ) DWORD NvOptimusEnablement = 1;
__declspec( dllexport ) int AmdPowerXpressRequestHighPerformance = 1;
}

static_assert( sizeof( BhedBakeLuxel ) == 40, "Luxel im Shader" );
static_assert( sizeof( BhedBakeLm ) == 112, "Lm im Shader" );
static_assert( sizeof( BhedBakeAktiv ) == 32, "Aktiv im Shader" );
static_assert( sizeof( BhedBakeLicht ) == 128, "Licht im Shader" );
static_assert( sizeof( BhedBakeGitterPunkt ) == 144, "GitterPunkt im Shader" );
static_assert( sizeof( BhedBakeBsp::Knoten ) == 16 && sizeof( BhedBakeBsp::Ebene ) == 16 && sizeof( BhedBakeBsp::Blatt ) == 16 &&
               sizeof( BhedBakeBsp::Brush ) == 16, "BSP im Shader" );

namespace
{

// Stufe 8g Punkt 1: die Shader (bhed_gpubacken.hlsl) sind im Bau mit fxc vorab uebersetzt
// (src/q3map2/CMakeLists.txt) - DXBC als Header, zur Laufzeit wird nichts uebersetzt.
#include "g_bake_erst_d0_m0.h"
#include "g_bake_erst_d1_m0.h"
#include "g_bake_erst_d1_m1.h"
#include "g_bake_erst_d1_m2.h"
#include "g_bake_kandidaten_d0_m0.h"
#include "g_bake_kandidaten_d1_m0.h"
#include "g_bake_unter_d0_m0.h"
#include "g_bake_unter_d1_m0.h"
#include "g_bake_unter_d1_m1.h"
#include "g_bake_unter_d1_m2.h"
#include "g_bake_stempellauf_d0_m0.h"
#include "g_bake_stempellauf_d1_m0.h"
#include "g_bake_stempellauf_d1_m1.h"
#include "g_bake_stempellauf_d1_m2.h"
#include "g_bake_filtern_d0_m0.h"
#include "g_bake_filtern_d1_m0.h"
#include "g_bake_gitterKern_d0_m0.h"
#include "g_bake_gitterKern_d1_m0.h"
#include "g_bake_gitterKern_d1_m1.h"
#include "g_bake_gitterKern_d1_m2.h"

struct Bytecode
{
	const void *daten;
	SIZE_T groesse;
};
template<std::size_t N>
constexpr Bytecode bc( const BYTE ( &b )[ N ] ){
	return Bytecode{ b, N };
}

enum Kern { kErst, kKandidaten, kUnter, kStempellauf, kFiltern, kGitter, kKerne };

// die Variante zu double/Messschalter (die Messschalter nur mit double: sie messen die NVIDIA-GPU)
Bytecode shaderWahl( Kern k, bool doppelt, int mess ){
	const int m = doppelt ? std::clamp( mess, 0, 2 ) : 0;
	switch ( k ) {
	case kErst: {
		static const Bytecode t[ 4 ] = { bc( g_bake_erst_d0_m0 ), bc( g_bake_erst_d1_m0 ), bc( g_bake_erst_d1_m1 ), bc( g_bake_erst_d1_m2 ) };
		return t[ doppelt ? 1 + m : 0 ];
	}
	case kKandidaten:
		return doppelt ? bc( g_bake_kandidaten_d1_m0 ) : bc( g_bake_kandidaten_d0_m0 );
	case kUnter: {
		static const Bytecode t[ 4 ] = { bc( g_bake_unter_d0_m0 ), bc( g_bake_unter_d1_m0 ), bc( g_bake_unter_d1_m1 ), bc( g_bake_unter_d1_m2 ) };
		return t[ doppelt ? 1 + m : 0 ];
	}
	case kStempellauf: {
		static const Bytecode t[ 4 ] = { bc( g_bake_stempellauf_d0_m0 ), bc( g_bake_stempellauf_d1_m0 ), bc( g_bake_stempellauf_d1_m1 ),
		                                 bc( g_bake_stempellauf_d1_m2 ) };
		return t[ doppelt ? 1 + m : 0 ];
	}
	case kGitter: {
		static const Bytecode t[ 4 ] = { bc( g_bake_gitterKern_d0_m0 ), bc( g_bake_gitterKern_d1_m0 ), bc( g_bake_gitterKern_d1_m1 ),
		                                 bc( g_bake_gitterKern_d1_m2 ) };
		return t[ doppelt ? 1 + m : 0 ];
	}
	default:
		return doppelt ? bc( g_bake_filtern_d1_m0 ) : bc( g_bake_filtern_d0_m0 );
	}
}

// ein Puffer mit UAV, der mitwaechst (und eine Kopie zum Lesen auf der CPU)
struct RwPuffer
{
	ID3D11Buffer *buf = nullptr;
	ID3D11UnorderedAccessView *uav = nullptr;
	ID3D11Buffer *lesen = nullptr;
	UINT kapazitaet = 0;
};

// Register t0..t21 (siehe bhed_gpubacken.hlsl)
enum Srv { sKnoten, sItems, sDreiecke, sInfos, sFlaechen, sVis, sLichter, sWindungen, sLuxel, sLms, sIdx, sLichtListe, sLmCluster, sAktiv, sAktivNr,
           sBspKnoten, sEbenen, sBlaetter, sBlattBrushes, sBrushes, sSeiten, sStartWert, sAnzahl };
// Register u0..u7
enum Uav { uP, uF, uS, uZaehler, uLmFlag, uR, uGitter, uZustand, uKand, uHilfe, uAnzahl };
constexpr int kUavSlots = 8;   // u6/u7: Gitter/Zustand im Gitterkern, Kandidaten/Hilfe in den Lightmap-Kernen

struct Gpu
{
	HMODULE d3d = nullptr, dxgi = nullptr;
	ID3D11Device *geraet = nullptr;
	ID3D11DeviceContext *ktx = nullptr;
	ID3D11ComputeShader *cs[ kKerne ] = {};
	ID3D11Buffer *buf[ sAnzahl ] = {};
	ID3D11ShaderResourceView *srv[ sAnzahl ] = {};
	RwPuffer rw[ uAnzahl ];
	ID3D11Buffer *kopf = nullptr;
	int kopfKnoten = 0;
	UINT visBytes = 0;
	int leafBytes = 0;
	int maxOpaqueBrush = -1;
	BhedBakeSchalter schalter{};
	bool doppelt = false;
	bool bereit = false;
	std::string name;
	unsigned long long eigenerSpeicher = 0, geteilterSpeicher = 0;
	D3D_FEATURE_LEVEL stufe{};
	std::size_t budget = 0;
	BhedBakeZahlen zahlen;
	long long verlustNach = -1;   // Test: nach so vielen Dispatches einen Geraeteverlust vortaeuschen
	bool zeiten = false;          // BHED_GPUBAKE_ZEITEN: jeden Kern bis zum Ende messen
	size_t lichterAnzahl = 0;
	int gitterTeil = 64;          // Lichter je Gitter-Dispatch (passt sich an)
	bool nanTest = false, nanGitterTest = false;   // BHED_GPUBAKE_NAN: nicht endliche Werte vortaeuschen
	UINT stueck[ kKerne ] = { 1u << 18, 1u << 18, 1u << 18, 4096, 1u << 18, 4096 };   // Faeden je Dispatch (passt sich an)
};
Gpu g;

template<typename T>
void frei( T *&p ){
	if ( p != nullptr ) {
		p->Release();
		p = nullptr;
	}
}

// unveraenderlicher Puffer mit SRV (strukturiert oder roh)
bool puffer( const void *daten, UINT stride, UINT anzahl, Srv nr, bool roh = false ){
	frei( g.srv[ nr ] );
	frei( g.buf[ nr ] );
	if ( anzahl == 0 ) {
		anzahl = 1;
		daten = nullptr;
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
	bd.MiscFlags = roh ? D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS : D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = roh ? 0 : stride;
	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = daten;
	if ( FAILED( g.geraet->CreateBuffer( &bd, &sd, &g.buf[ nr ] ) ) ) {
		return false;
	}
	D3D11_SHADER_RESOURCE_VIEW_DESC vd{};
	if ( roh ) {
		vd.Format = DXGI_FORMAT_R32_TYPELESS;
		vd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		vd.BufferEx.NumElements = bd.ByteWidth / 4;
		vd.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
	}
	else {
		vd.Format = DXGI_FORMAT_UNKNOWN;
		vd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		vd.Buffer.NumElements = anzahl;
	}
	return SUCCEEDED( g.geraet->CreateShaderResourceView( g.buf[ nr ], &vd, &g.srv[ nr ] ) );
}
template<typename T>
bool puffer( const std::vector<T>& v, Srv nr ){
	return puffer( v.empty() ? nullptr : v.data(), UINT( sizeof( T ) ), UINT( v.size() ), nr );
}

// UAV-Puffer mit mindestens n Elementen zu stride Byte; lesbar: dazu ein Staging-Puffer
bool platz( Uav nr, UINT n, UINT stride, bool lesbar ){
	RwPuffer& p = g.rw[ nr ];
	if ( n <= p.kapazitaet ) {
		return true;
	}
	frei( p.uav );
	frei( p.buf );
	frei( p.lesen );
	p.kapazitaet = 0;
	UINT k = 1024;
	while ( k < n ) {
		k = k > 0x40000000u ? n : k * 2;
	}
	D3D11_BUFFER_DESC bd{};
	bd.ByteWidth = stride * k;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = stride;
	if ( FAILED( g.geraet->CreateBuffer( &bd, nullptr, &p.buf ) ) ) {
		return false;
	}
	D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};
	ud.Format = DXGI_FORMAT_UNKNOWN;
	ud.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	ud.Buffer.NumElements = k;
	if ( FAILED( g.geraet->CreateUnorderedAccessView( p.buf, &ud, &p.uav ) ) ) {
		return false;
	}
	if ( lesbar ) {
		bd.Usage = D3D11_USAGE_STAGING;
		bd.BindFlags = 0;
		bd.MiscFlags = 0;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		if ( FAILED( g.geraet->CreateBuffer( &bd, nullptr, &p.lesen ) ) ) {
			return false;
		}
	}
	p.kapazitaet = k;
	return true;
}

// die ersten bytes eines UAV-Puffers auf die CPU
bool lesen( Uav nr, void *ziel, std::size_t bytes ){
	if ( bytes == 0 ) {
		return true;
	}
	RwPuffer& p = g.rw[ nr ];
	D3D11_BOX box{ 0, 0, 0, UINT( bytes ), 1, 1 };
	g.ktx->CopySubresourceRegion( p.lesen, 0, 0, 0, 0, p.buf, 0, &box );
	D3D11_MAPPED_SUBRESOURCE m{};
	if ( FAILED( g.ktx->Map( p.lesen, 0, D3D11_MAP_READ, 0, &m ) ) ) {
		return false;
	}
	std::memcpy( ziel, m.pData, bytes );
	g.ktx->Unmap( p.lesen, 0 );
	return g.geraet->GetDeviceRemovedReason() == S_OK;
}

void allesFrei(){
	for ( int i = 0; i < sAnzahl; ++i ) {
		frei( g.srv[ i ] );
		frei( g.buf[ i ] );
	}
	for ( RwPuffer& p : g.rw ) {
		frei( p.uav );
		frei( p.buf );
		frei( p.lesen );
		p.kapazitaet = 0;
	}
	frei( g.kopf );
	for ( ID3D11ComputeShader *&c : g.cs ) {
		frei( c );
	}
	frei( g.ktx );
	frei( g.geraet );
	g.bereit = false;
}

void ausfall( const char *grund ){
	if ( !g.zahlen.ausgefallen ) {
		g.zahlen.ausgefallen = true;
		g.zahlen.grund = grund;
		Sys_Printf( "BehavEd GPU bake: %s - the rest is computed on the CPU\n", grund );
	}
	allesFrei();
}

// Meldungen des Geraetestarts: er laeuft in einem eigenen Faden neben dem Laden der Karte
// (BhedBakeGeraetVorab), die Ausgabe folgt danach in BhedBakeStart
std::vector<std::string> g_meldungen;
void melden( const char *format, ... ){
	char z[ 1024 ];
	va_list args;
	va_start( args, format );
	std::vsnprintf( z, sizeof( z ), format, args );
	va_end( args );
	g_meldungen.emplace_back( z );
}

std::string utf8( const wchar_t *w ){
	char z[ 512 ] = {};
	WideCharToMultiByte( CP_UTF8, 0, w, -1, z, sizeof( z ) - 1, nullptr, nullptr );
	return z;
}

// Adapter: -bhedgpuadapter <n|Teil des Namens>, sonst die schnellste GPU
// (EnumAdapterByGpuPreference HIGH_PERFORMANCE), ohne IDXGIFactory6 der Adapter
// mit dem meisten eigenen Grafikspeicher. Software-Adapter nie.
IDXGIAdapter1 *adapterWaehlen( const std::string& wahl ){
	g.dxgi = LoadLibraryW( L"dxgi.dll" );
	if ( g.dxgi == nullptr ) {
		return nullptr;
	}
	using Fabrik = HRESULT( WINAPI * )( REFIID, void ** );
	const auto fabrik = reinterpret_cast<Fabrik>( reinterpret_cast<void *>( GetProcAddress( g.dxgi, "CreateDXGIFactory1" ) ) );
	IDXGIFactory1 *f = nullptr;
	if ( fabrik == nullptr || FAILED( fabrik( __uuidof( IDXGIFactory1 ), reinterpret_cast<void **>( &f ) ) ) ) {
		return nullptr;
	}
	std::vector<IDXGIAdapter1 *> liste;
	IDXGIFactory6 *f6 = nullptr;
	if ( SUCCEEDED( f->QueryInterface( __uuidof( IDXGIFactory6 ), reinterpret_cast<void **>( &f6 ) ) ) ) {
		IDXGIAdapter1 *a = nullptr;
		for ( UINT i = 0; SUCCEEDED( f6->EnumAdapterByGpuPreference( i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof( IDXGIAdapter1 ), reinterpret_cast<void **>( &a ) ) ); ++i ) {
			liste.push_back( a );
		}
		f6->Release();
	}
	else {
		IDXGIAdapter1 *a = nullptr;
		for ( UINT i = 0; f->EnumAdapters1( i, &a ) != DXGI_ERROR_NOT_FOUND; ++i ) {
			liste.push_back( a );
		}
		std::stable_sort( liste.begin(), liste.end(), []( IDXGIAdapter1 *x, IDXGIAdapter1 *y ){
			DXGI_ADAPTER_DESC1 a{}, b{};
			x->GetDesc1( &a );
			y->GetDesc1( &b );
			return a.DedicatedVideoMemory > b.DedicatedVideoMemory;
		} );
	}
	f->Release();
	std::vector<IDXGIAdapter1 *> echt;
	for ( IDXGIAdapter1 *a : liste ) {
		DXGI_ADAPTER_DESC1 d{};
		if ( SUCCEEDED( a->GetDesc1( &d ) ) && !( d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ) ) {
			echt.push_back( a );
		}
		else {
			a->Release();
		}
	}
	IDXGIAdapter1 *gewaehlt = nullptr;
	if ( !wahl.empty() ) {
		char *rest = nullptr;
		const long n = std::strtol( wahl.c_str(), &rest, 10 );
		if ( rest != nullptr && *rest == 0 && n >= 0 && size_t( n ) < echt.size() ) {
			gewaehlt = echt[ size_t( n ) ];
		}
		else {
			std::string w = wahl;
			std::transform( w.begin(), w.end(), w.begin(), []( unsigned char c ){ return char( std::tolower( c ) ); } );
			for ( IDXGIAdapter1 *a : echt ) {
				DXGI_ADAPTER_DESC1 d{};
				a->GetDesc1( &d );
				std::string name = utf8( d.Description );
				std::transform( name.begin(), name.end(), name.begin(), []( unsigned char c ){ return char( std::tolower( c ) ); } );
				if ( name.find( w ) != std::string::npos ) {
					gewaehlt = a;
					break;
				}
			}
		}
		if ( gewaehlt == nullptr ) {
			melden( "BehavEd GPU bake: no adapter \"%s\" - the fastest one\n", wahl.c_str() );
		}
	}
	if ( gewaehlt == nullptr && !echt.empty() ) {
		gewaehlt = echt.front();
	}
	for ( size_t i = 0; i < echt.size(); ++i ) {
		DXGI_ADAPTER_DESC1 d{};
		echt[ i ]->GetDesc1( &d );
		melden( "BehavEd GPU bake: adapter %zu: %s (%llu MB)%s\n", i, utf8( d.Description ).c_str(),
		             static_cast<unsigned long long>( d.DedicatedVideoMemory >> 20 ), echt[ i ] == gewaehlt ? " - chosen" : "" );
		if ( echt[ i ] != gewaehlt ) {
			echt[ i ]->Release();
		}
	}
	return gewaehlt;
}

long long mikros( std::chrono::steady_clock::time_point t0 ){
	return std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::steady_clock::now() - t0 ).count();
}

// auf die GPU warten; false: Geraeteverlust
bool warten(){
	ID3D11Query *ende = nullptr;
	D3D11_QUERY_DESC qd{ D3D11_QUERY_EVENT, 0 };
	if ( FAILED( g.geraet->CreateQuery( &qd, &ende ) ) ) {
		return false;
	}
	g.ktx->End( ende );
	BOOL fertig = FALSE;
	bool ok = true;
	while ( g.ktx->GetData( ende, &fertig, sizeof( fertig ), 0 ) == S_FALSE ) {
		if ( g.geraet->GetDeviceRemovedReason() != S_OK ) {
			ok = false;
			break;
		}
		SwitchToThread();
	}
	frei( ende );
	return ok && g.geraet->GetDeviceRemovedReason() == S_OK;
}

// der Konstantenpuffer (cbuffer Kopf im Shader)
struct Kopf
{
	UINT anzahl, basis;
	int kopfKnoten, faster, lightAngleHL, bouncing, testOcclusion, oneSky, noSurfaces;
	float linearScale;
	UINT visBytes;
	int leafBytes;
	int nLms, lightSamples;
	float suchBox;
	int maxOpaqueBrush;
	int nAktivGesamt, nLuxel, nZeilen;
	float inhibitGitter, gridAmbientDirectionality, gridDirectionality;
	int cheapgrid, nGitterLichter;
	int gitterPhase, lichtVon, lichtBis;
	UINT kandVon;
};
static_assert( sizeof( Kopf ) == 112, "cbuffer Kopf" );

Kopf kopfFuer( const BhedBakeBlock& b ){
	const BhedBakeSchalter& s = g.schalter;
	return Kopf{ 0, 0, g.kopfKnoten, s.faster, s.lightAngleHL, s.bouncing, s.testOcclusion, s.oneSky, s.noSurfaces, s.linearScale, g.visBytes, g.leafBytes,
	             int( b.lms.size() ), s.lightSamples, s.suchBox, g.maxOpaqueBrush, int( b.aktiv.size() ), int( b.luxel.size() ), int( b.zeilen ), 0.0f, 0.0f,
	             0.0f, 0, 0, 0, 0, 0, 0 };
}

// n Faeden des Kerns k in kurzen Dispatches (Windows-Zeitlimit TDR 2 s, die Oberflaeche nutzt
// dieselbe GPU): je Dispatch g.stueck[k] Faeden, nach je warten Dispatches ein Flush mit Warten auf
// die GPU. Die Dispatch-Groesse passt sich an: dauert eine Wartegruppe ueber 0,5 s, halbiert sie
// sich, unter 0,03 s verdoppelt sie sich (4096 bis 4 M Faeden - nie kleiner: bei teuren Faeden,
// etwa -samples 4, liefe sonst kaum noch etwas gleichzeitig). false: Geraeteverlust (echt oder
// im Test vorgetaeuscht).
bool lauf( Kern k, UINT n, int warten, Kopf kopf ){
	if ( n == 0 ) {
		return true;
	}
	const auto t0 = std::chrono::steady_clock::now();
	g.ktx->CSSetShader( g.cs[ k ], nullptr, 0 );
	ID3D11Query *ende = nullptr;
	D3D11_QUERY_DESC qd{ D3D11_QUERY_EVENT, 0 };
	(void)g.geraet->CreateQuery( &qd, &ende );
	bool ok = true;
	int seitWarten = 0;
	auto tGruppe = std::chrono::steady_clock::now();
	const auto warte = [&](){
		g.ktx->End( ende );
		BOOL fertig = FALSE;
		while ( g.ktx->GetData( ende, &fertig, sizeof( fertig ), 0 ) == S_FALSE ) {
			if ( g.geraet->GetDeviceRemovedReason() != S_OK ) {
				return false;
			}
			SwitchToThread();
		}
		return true;
	};
	for ( UINT basis = 0; basis < n && ok; ) {
		const UINT stueck = g.stueck[ k ];
		kopf.anzahl = std::min( stueck, n - basis );
		kopf.basis = basis;
		basis += kopf.anzahl;
		g.ktx->UpdateSubresource( g.kopf, 0, nullptr, &kopf, 0, 0 );
		g.ktx->Dispatch( ( kopf.anzahl + 63 ) / 64, 1, 1 );
		++g.zahlen.dispatches;
		if ( g.verlustNach >= 0 && g.zahlen.dispatches > g.verlustNach ) {
			ok = false;   // Test: Geraeteverlust vorgetaeuscht (BHED_GPUBAKE_VERLUST)
			break;
		}
		if ( ++seitWarten >= warten && ende != nullptr ) {
			seitWarten = 0;
			ok = warte();
			const double dauer = std::chrono::duration<double>( std::chrono::steady_clock::now() - tGruppe ).count();
			tGruppe = std::chrono::steady_clock::now();
			if ( dauer > 0.5 && g.stueck[ k ] > 4096 ) {
				g.stueck[ k ] /= 2;
			}
			else if ( dauer < 0.03 && kopf.anzahl == stueck && g.stueck[ k ] < ( 1u << 22 ) ) {
				g.stueck[ k ] *= 2;
			}
		}
	}
	if ( ok && g.zeiten && ende != nullptr ) {   // Messung: auf das Ende warten
		ok = warte();
		g.zahlen.kernMikros[ k ] += mikros( t0 );
	}
	frei( ende );
	return ok && g.geraet->GetDeviceRemovedReason() == S_OK;
}

void binden( bool gitter = false ){
	g.ktx->CSSetShaderResources( 0, sAnzahl, g.srv );
	ID3D11UnorderedAccessView *u[ kUavSlots ];
	for ( int i = 0; i < 6; ++i ) {
		u[ i ] = g.rw[ i ].uav;
	}
	u[ 6 ] = g.rw[ gitter ? uGitter : uKand ].uav;
	u[ 7 ] = g.rw[ gitter ? uZustand : uHilfe ].uav;
	g.ktx->CSSetUnorderedAccessViews( 0, kUavSlots, u, nullptr );
	g.ktx->CSSetConstantBuffers( 0, 1, &g.kopf );
}

void loesen(){
	ID3D11UnorderedAccessView *u[ kUavSlots ] = {};
	g.ktx->CSSetUnorderedAccessViews( 0, kUavSlots, u, nullptr );
}

void verloren(){
	char z[ 160 ];
	std::snprintf( z, sizeof( z ), "the graphics device was lost (%s)", g.verlustNach >= 0 ? "simulated for the test" : "TDR or driver reset" );
	ausfall( z );
}

} // namespace

namespace
{

// Geraet, Faehigkeiten und Shader (ohne die Szene); false: keine GPU (die Meldung steht in g_meldungen)
bool geraetStarten( const std::string& adapterWahl ){
	g.d3d = LoadLibraryW( L"d3d11.dll" );
	if ( g.d3d == nullptr ) {
		melden( "BehavEd GPU bake: d3d11.dll missing - CPU\n" );
		return false;
	}
	const auto erzeugen = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>( reinterpret_cast<void *>( GetProcAddress( g.d3d, "D3D11CreateDevice" ) ) );
	if ( erzeugen == nullptr ) {
		return false;
	}
	IDXGIAdapter1 *adapter = adapterWaehlen( adapterWahl );
	const D3D_FEATURE_LEVEL stufen[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
	const HRESULT erzeugt = erzeugen( adapter, adapter != nullptr ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, stufen, 2, D3D11_SDK_VERSION,
	                                  &g.geraet, &g.stufe, &g.ktx );
	if ( adapter != nullptr ) {
		DXGI_ADAPTER_DESC1 d{};
		adapter->GetDesc1( &d );
		g.name = utf8( d.Description );
		g.eigenerSpeicher = d.DedicatedVideoMemory;
		g.geteilterSpeicher = d.SharedSystemMemory;
		adapter->Release();
	}
	if ( FAILED( erzeugt ) ) {
		melden( "BehavEd GPU bake: no Direct3D 11 hardware device - CPU\n" );
		return false;
	}
	D3D11_FEATURE_DATA_DOUBLES dd{};
	D3D11_FEATURE_DATA_D3D11_OPTIONS op{};
	const bool d1 = SUCCEEDED( g.geraet->CheckFeatureSupport( D3D11_FEATURE_DOUBLES, &dd, sizeof( dd ) ) ) && dd.DoublePrecisionFloatShaderOps;
	const bool d2 = SUCCEEDED( g.geraet->CheckFeatureSupport( D3D11_FEATURE_D3D11_OPTIONS, &op, sizeof( op ) ) ) && op.ExtendedDoublesShaderInstructions;
	g.doppelt = d1 && d2 && std::getenv( "BHED_GPUBAKE_OHNE_DOUBLE" ) == nullptr;
	const char *mess = std::getenv( "BHED_GPUBAKE_MESS" );
	for ( int k = 0; k < kKerne; ++k ) {
		const Bytecode code = shaderWahl( Kern( k ), g.doppelt, mess != nullptr ? std::atoi( mess ) : 0 );
		if ( FAILED( g.geraet->CreateComputeShader( code.daten, code.groesse, nullptr, &g.cs[ k ] ) ) ) {
			melden( "BehavEd GPU bake: the device does not take the shader - CPU\n" );
			allesFrei();
			return false;
		}
	}
	return true;
}

std::thread g_startFaden;
bool g_startOk = false;
std::string g_startWahl;

} // namespace

void BhedBakeGeraetVorab( const std::string& adapterWahl ){
	if ( g_startFaden.joinable() ) {
		return;
	}
	g_startWahl = adapterWahl;
	g_startFaden = std::thread( [](){
		g_startOk = geraetStarten( g_startWahl );
	} );
}

bool BhedBakeStart( const std::vector<BhedBakeKnoten>& knoten, const std::vector<int>& items, const std::vector<BhedBakeDreieck>& dreiecke,
                    const std::vector<BhedBakeInfo>& infos, const int *flaechen, int anzahlFlaechen, int kopfKnoten,
                    const unsigned char *vis, std::size_t visBytes, const BhedBakeBsp& bsp, const std::string& adapterWahl ){
	if ( const char *v = std::getenv( "BHED_GPUBAKE_VERLUST" ) ) {
		g.verlustNach = std::atoll( v );
	}
	g.zeiten = std::getenv( "BHED_GPUBAKE_ZEITEN" ) != nullptr;
	g.nanTest = g.nanGitterTest = std::getenv( "BHED_GPUBAKE_NAN" ) != nullptr;
	if ( !g_startFaden.joinable() ) {
		BhedBakeGeraetVorab( adapterWahl );
	}
	g_startFaden.join();
	for ( const std::string& z : g_meldungen ) {
		Sys_Printf( "%s", z.c_str() );
	}
	g_meldungen.clear();
	if ( !g_startOk ) {
		allesFrei();
		return false;
	}
	const D3D_FEATURE_LEVEL stufe = g.stufe;
	// Speicherbedarf der Szene (BVH-Ersatz: Knoten, Dreiecke; BSP) gegen den Grafikspeicher
	const size_t bedarf = knoten.size() * sizeof( BhedBakeKnoten ) + items.size() * 4 + dreiecke.size() * sizeof( BhedBakeDreieck ) +
	                      infos.size() * sizeof( BhedBakeInfo ) + visBytes + bsp.knoten.size() * 16 + bsp.ebenen.size() * 16 + bsp.blaetter.size() * 16 +
	                      bsp.blattBrushes.size() * 4 + bsp.brushes.size() * 16 + bsp.seiten.size() * 4;
	std::vector<unsigned char> visKopie( vis != nullptr ? vis : reinterpret_cast<const unsigned char *>( "" ), vis != nullptr ? vis + visBytes : reinterpret_cast<const unsigned char *>( "" ) );
	visKopie.resize( ( visKopie.size() + 3 ) & ~size_t( 3 ), 0 );
	if ( visKopie.empty() ) {
		visKopie.assign( 4, 0 );
	}
	bool ok = puffer( knoten, sKnoten ) && puffer( items, sItems ) && puffer( dreiecke, sDreiecke ) && puffer( infos, sInfos ) &&
	          puffer( anzahlFlaechen > 0 ? flaechen : nullptr, UINT( sizeof( int ) ), UINT( anzahlFlaechen ), sFlaechen ) &&
	          puffer( visKopie.data(), 4, UINT( visKopie.size() / 4 ), sVis, true ) &&
	          puffer( bsp.knoten, sBspKnoten ) && puffer( bsp.ebenen, sEbenen ) && puffer( bsp.blaetter, sBlaetter ) &&
	          puffer( bsp.blattBrushes, sBlattBrushes ) && puffer( bsp.brushes, sBrushes ) && puffer( bsp.seiten, sSeiten );
	if ( ok ) {
		D3D11_BUFFER_DESC bd{};
		bd.ByteWidth = sizeof( Kopf );
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		ok = SUCCEEDED( g.geraet->CreateBuffer( &bd, nullptr, &g.kopf ) );
	}
	if ( !ok ) {
		Sys_Printf( "BehavEd GPU bake: not enough graphics memory for the scene (%zu MB) - CPU\n", bedarf >> 20 );
		allesFrei();
		return false;
	}
	g.kopfKnoten = kopfKnoten;
	g.visBytes = UINT( visBytes );
	g.leafBytes = visBytes > 8 ? reinterpret_cast<const int *>( vis )[ 1 ] : 0;
	g.maxOpaqueBrush = bsp.maxOpaqueBrush;
	// Grafikspeicher vorab schaetzen: ein Block darf ein Viertel des eigenen Grafikspeichers
	// belegen (64 MB bis 1,5 GB; GPUs mit wenig eigenem Speicher: die Haelfte des geteilten),
	// je Paar 40 Byte (Farbe, Merker, Unterabtastung, Kandidatenliste)
	unsigned long long vram = g.eigenerSpeicher;
	if ( vram < ( 512ull << 20 ) ) {
		vram = std::max( vram, g.geteilterSpeicher / 2 );
	}
	unsigned long long bytes = std::clamp<unsigned long long>( vram / 4, 64ull << 20, 1536ull << 20 );
	if ( const char *b = std::getenv( "BHED_GPUBAKE_BUDGET_MB" ) ) {   // Test: kleine Bloecke erzwingen
		bytes = std::max<unsigned long long>( 1, std::strtoull( b, nullptr, 10 ) ) << 20;
	}
	g.budget = std::size_t( bytes / 40 );
	g.zahlen.budget = static_cast<long long>( g.budget );
	g.bereit = true;
	Sys_Printf( "BehavEd GPU bake: %s (feature level %x, %s), %zu nodes, %zu triangles, scene %zu KB, blocks up to %zu pairs (%llu MB)\n", g.name.c_str(),
	             unsigned( stufe ), g.doppelt ? "double precision" : "no double precision - float approximations", knoten.size(), dreiecke.size(), bedarf >> 10,
	             g.budget, bytes >> 20 );
	return true;
}

bool BhedBakeLichter( const std::vector<BhedBakeLicht>& lichter, const std::vector<float>& windungen, const BhedBakeSchalter& s,
                      const std::vector<BhedBakeInfo>& infos ){
	if ( !g.bereit ) {
		return false;
	}
	g.schalter = s;
	g.lichterAnzahl = lichter.size();
	if ( !puffer( infos, sInfos ) || !puffer( lichter, sLichter ) || !puffer( windungen, sWindungen ) ) {
		ausfall( "not enough graphics memory for the lights" );
		return false;
	}
	return true;
}

std::size_t BhedBakeBudgetPaare(){
	return g.budget;
}

bool BhedBakeErst( const BhedBakeBlock& b, std::vector<std::uint32_t>& zaehler, std::vector<std::uint32_t>& flags ){
	zaehler.assign( b.lichtListe.size(), 0 );
	flags.assign( b.lms.size(), 1 );
	if ( !g.bereit ) {
		return false;
	}
	const auto t0 = std::chrono::steady_clock::now();
	const std::vector<BhedBakeAktiv> keinAktiv;
	const std::vector<int> keineNr;
	const UINT n = std::max<UINT>( b.paare, 1 );
	if ( !puffer( b.luxel, sLuxel ) || !puffer( b.lms, sLms ) || !puffer( b.idx, sIdx ) || !puffer( b.lichtListe, sLichtListe ) ||
	     !puffer( b.lmCluster, sLmCluster ) || !puffer( keinAktiv, sAktiv ) || !puffer( keineNr, sAktivNr ) || !puffer( b.startWert.empty() ? nullptr : b.startWert.data(), 16, UINT( b.startWert.size() / 4 ), sStartWert ) ||
	     !platz( uP, n, 16, false ) || !platz( uF, n, 4, false ) || !platz( uS, g.schalter.lightSamples > 1 ? n : 1, 16, false ) ||
	     !platz( uZaehler, UINT( std::max<size_t>( b.lichtListe.size(), 1 ) ), 4, true ) || !platz( uLmFlag, UINT( std::max<size_t>( b.lms.size(), 1 ) ), 4, true ) ||
	     !platz( uR, 1, 16, true ) || !platz( uKand, g.schalter.lightSamples > 1 ? n : 1, 4, false ) || !platz( uHilfe, 1, 4, true ) ) {
		ausfall( "not enough graphics memory for a block" );
		return false;
	}
	const UINT null[ 4 ] = {};
	g.ktx->ClearUnorderedAccessViewUint( g.rw[ uZaehler ].uav, null );
	g.ktx->ClearUnorderedAccessViewUint( g.rw[ uLmFlag ].uav, null );
	binden();
	bool ok = lauf( kErst, b.paare, 4, kopfFuer( b ) );
	loesen();
	ok = ok && lesen( uZaehler, zaehler.data(), zaehler.size() * 4 ) && lesen( uLmFlag, flags.data(), flags.size() * 4 );
	g.zahlen.gpuMikros += mikros( t0 );
	if ( !ok ) {
		verloren();
		flags.assign( b.lms.size(), 1 );
		return false;
	}
	g.zahlen.paare += b.paare;
	++g.zahlen.bloecke;
	return true;
}

bool BhedBakeRest( const BhedBakeBlock& b, std::vector<float>& luxelAus, std::vector<std::uint32_t>& flags ){
	luxelAus.assign( size_t( b.aus ) * 4, 0.0f );
	flags.assign( b.lms.size(), 1 );
	if ( !g.bereit ) {
		return false;
	}
	const auto t0 = std::chrono::steady_clock::now();
	if ( !puffer( b.lms, sLms ) || !puffer( b.aktiv, sAktiv ) || !puffer( b.aktivNr, sAktivNr ) ||
	     !puffer( b.startWert.empty() ? nullptr : b.startWert.data(), 16, UINT( b.startWert.size() / 4 ), sStartWert ) || !platz( uR, std::max<UINT>( b.aus, 1 ), 16, true ) ||
	     !platz( uHilfe, UINT( b.aktiv.size() + 3 + b.maskeBits / 32 ), 4, true ) ) {
		ausfall( "not enough graphics memory for a block" );
		return false;
	}
	binden();
	const Kopf kopf = kopfFuer( b );
	bool ok = true;
	if ( g.schalter.lightSamples > 1 ) {
		// Vorhersage: die Kandidaten kommen in eine Liste (hilfe[0] zaehlt ab 0xffffffff), dazu je aktivem Licht
		// der erste vorhergesagte Stempel; dann nur die Kandidaten unterabtasten
		const UINT voll[ 4 ] = { 0xffffffffu, 0xffffffffu, 0xffffffffu, 0xffffffffu };
		g.ktx->ClearUnorderedAccessViewUint( g.rw[ uHilfe ].uav, voll );
		// Runde 0 mit den Werten der Erstabtastung, dann bis zu drei weitere Runden mit den Ergebnissen der
		// vorigen (ein nachabgetasteter Luxel aendert die Stempel um ihn) - so muss der serielle Lauf kaum
		// noch selbst nachabtasten. Jedes vorab berechnete Ergebnis ist gueltig (es haengt nur vom Wert der
		// Erstabtastung ab); nicht gebrauchte bleiben ungenutzt.
		Kopf k = kopf;
		UINT bisher = 0;
		static const int runden = std::getenv( "BHED_GPUBAKE_RUNDEN" ) != nullptr ? std::max( 1, std::atoi( std::getenv( "BHED_GPUBAKE_RUNDEN" ) ) ) : 4;   // Messung
		for ( int runde = 0; runde < runden && ok; ++runde ) {
			k.gitterPhase = runde;
			k.kandVon = 0;
			ok = lauf( kKandidaten, b.zeilen, 4, k );
			std::uint32_t zahl = 0;
			loesen();
			ok = ok && lesen( uHilfe, &zahl, 4 );
			binden();
			const UINT neu = zahl + 1u;
			if ( g.zeiten ) {
				g.zahlen.kandidaten[ std::min( runde, 3 ) ] += neu - bisher;
			}
			if ( !ok || neu == bisher ) {
				break;
			}
			k.kandVon = bisher;
			ok = lauf( kUnter, neu - bisher, 1, k );
			bisher = neu;
		}
		ok = ok && lauf( kStempellauf, UINT( b.aktiv.size() ), 1, kopf );
	}
	ok = ok && lauf( kFiltern, UINT( b.luxel.size() ), 4, kopf );
	loesen();
	ok = ok && lesen( uR, luxelAus.data(), luxelAus.size() * 4 ) && lesen( uLmFlag, flags.data(), flags.size() * 4 );
	if ( g.nanTest && !luxelAus.empty() ) {   // Test: ein Treiberfehler (nicht endlicher Wert) im ersten Block
		g.nanTest = false;
		luxelAus[ 0 ] = std::numeric_limits<float>::quiet_NaN();
	}
	g.zahlen.gpuMikros += mikros( t0 );
	if ( !ok ) {
		verloren();
		flags.assign( b.lms.size(), 1 );
		return false;
	}
	return true;
}

bool BhedBakeGitter( std::vector<BhedBakeGitterPunkt>& punkte, const BhedBakeGitterSchalter& s ){
	if ( !g.bereit ) {
		return false;
	}
	const auto t0 = std::chrono::steady_clock::now();
	// Bloecke von Punkten im Speicherbudget (144 + 48 Byte je Punkt), hoechstens 4 M Faeden je Dispatch
	const size_t jeBlock = std::clamp<size_t>( g.budget * 40 / ( sizeof( BhedBakeGitterPunkt ) + 48 ), 4096, size_t( 65535 ) * 64 );
	const BhedBakeBlock leer;
	Kopf kopf = kopfFuer( leer );
	kopf.inhibitGitter = s.inhibit;
	kopf.gridAmbientDirectionality = s.ambientDirectionality;
	kopf.gridDirectionality = s.directionality;
	kopf.cheapgrid = s.cheapgrid;
	kopf.nGitterLichter = int( g.lichterAnzahl );
	const int n = int( g.lichterAnzahl );
	bool ok = true;
	std::vector<int> stand;
	for ( size_t a = 0; a < punkte.size() && ok; a += jeBlock ) {
		const size_t m = std::min( jeBlock, punkte.size() - a );
		if ( !platz( uGitter, UINT( m ), sizeof( BhedBakeGitterPunkt ), true ) || !platz( uZustand, UINT( m ), 48, true ) ) {
			ausfall( "not enough graphics memory for the light grid" );
			return false;
		}
		D3D11_BOX box{ 0, 0, 0, UINT( m * sizeof( BhedBakeGitterPunkt ) ), 1, 1 };
		g.ktx->UpdateSubresource( g.rw[ uGitter ].buf, 0, &box, punkte.data() + a, 0, 0 );
		binden( true );
		g.ktx->CSSetShader( g.cs[ kGitter ], nullptr, 0 );
		kopf.anzahl = UINT( m );
		kopf.basis = 0;
		// alle Punkte je Dispatch, die Lichter in Teilen; die Teilgroesse passt sich an (0,02 bis 0,15 s je Dispatch)
		for ( int phase = 0; phase < 2 && ok; ++phase ) {
			kopf.gitterPhase = phase;
			for ( int von = 0; von < n && ok; ) {
				const int bis = std::min( n, von + g.gitterTeil );
				kopf.lichtVon = von;
				kopf.lichtBis = bis;
				const auto tz = std::chrono::steady_clock::now();
				g.ktx->UpdateSubresource( g.kopf, 0, nullptr, &kopf, 0, 0 );
				g.ktx->Dispatch( UINT( ( m + 63 ) / 64 ), 1, 1 );
				++g.zahlen.dispatches;
				if ( g.verlustNach >= 0 && g.zahlen.dispatches > g.verlustNach ) {
					ok = false;
					break;
				}
				ok = warten();
				const double dauer = std::chrono::duration<double>( std::chrono::steady_clock::now() - tz ).count();
				if ( dauer > 0.15 && g.gitterTeil > 1 ) {
					g.gitterTeil /= 2;
				}
				else if ( dauer < 0.02 && bis - von == g.gitterTeil && g.gitterTeil < ( 1 << 20 ) ) {
					g.gitterTeil *= 2;
				}
				von = bis;
			}
		}
		g.zahlen.kernMikros[ kGitter ] += mikros( t0 );
		loesen();
		stand.resize( m * 12 );
		ok = ok && lesen( uGitter, punkte.data() + a, m * sizeof( BhedBakeGitterPunkt ) ) && lesen( uZustand, stand.data(), m * 48 );
		if ( g.nanGitterTest && m > 0 ) {   // Test: ein nicht endlicher Wert im Gitter
			g.nanGitterTest = false;
			punkte[ a ].ambient[ 0 ] = std::numeric_limits<float>::infinity();
		}
		// alle Beitraege der ersten Phase auch in der zweiten gefunden (sonst die CPU)
		for ( size_t k = 0; ok && k < m; ++k ) {
			if ( stand[ k * 12 + 2 ] != stand[ k * 12 + 0 ] ) {
				punkte[ a + k ].flag = 1;
			}
		}
	}
	g.zahlen.gpuMikros += mikros( t0 );
	if ( !ok ) {
		verloren();
		return false;
	}
	return true;
}

bool BhedBakeBereit(){
	return g.bereit;
}

void BhedBakeEnde(){
	allesFrei();
}

std::string BhedBakeAdapter(){
	return g.name;
}

bool BhedBakeDoppelt(){
	return g.doppelt;
}

BhedBakeZahlen BhedBakeStatistik(){
	return g.zahlen;
}
