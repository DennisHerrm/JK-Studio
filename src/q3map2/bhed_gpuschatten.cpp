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

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>

namespace
{

// Stufe 8g Punkt 1: der Shader (bhed_gpuschatten.hlsl) ist im Bau mit fxc vorab uebersetzt.
#include "g_schatten.h"

struct Gpu
{
	HMODULE d3d = nullptr;
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
	if ( g_gpu.d3d == nullptr ) {
		Sys_Printf( "BehavEd GPU: d3d11.dll missing - CPU\n" );
		return false;
	}
	const auto erzeugen = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>( reinterpret_cast<void *>( GetProcAddress( g_gpu.d3d, "D3D11CreateDevice" ) ) );
	if ( erzeugen == nullptr ) {
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
	const HRESULT hr = g_gpu.geraet->CreateComputeShader( g_schatten, sizeof( g_schatten ), nullptr, &g_gpu.cs );
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
