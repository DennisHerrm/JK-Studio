// bhed_gpubacken.cpp - BehavEd Stufe 8c Punkt 2: GPU-Backen fuer q3map2 -light
// (siehe bhed_gpubacken.h). Der Compute-Shader ist LightContributionToSample,
// SetupTrace, TraceLine/TraceLine_r/TraceTriangle und PointToPolygonFormFactor
// aus q3map2 (light.cpp, light_trace.cpp), Zeile fuer Zeile:
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
#include <cstring>
#include <string>

// Hybrid-Notebooks: die schnelle GPU fuer diese exe (wie jkstudio.exe, gui/main_win32.cpp).
// Die Exporte muessen in der exe stehen, nicht in einer DLL.
extern "C" {
__declspec( dllexport ) DWORD NvOptimusEnablement = 1;
__declspec( dllexport ) int AmdPowerXpressRequestHighPerformance = 1;
}

namespace
{

// Stufe 8g Punkt 1: die Shader (bhed_gpubacken.hlsl) sind im Bau mit fxc vorab uebersetzt
// (src/q3map2/CMakeLists.txt) - DXBC als Header, zur Laufzeit wird nichts uebersetzt.
#include "g_bake_d0_m0.h"
#include "g_bake_d0_m1.h"
#include "g_bake_d0_m2.h"
#include "g_bake_d1_m0.h"
#include "g_bake_d1_m1.h"
#include "g_bake_d1_m2.h"
#include "g_bake_d1_diag.h"

struct Bytecode
{
	const void *daten;
	SIZE_T groesse;
};
template<std::size_t N>
constexpr Bytecode bc( const BYTE ( &b )[ N ] ){
	return Bytecode{ b, N };
}

// die Variante zu double/Messschalter/Diagnose
Bytecode shaderWahl( bool doppelt, int mess, bool diag ){
	if ( doppelt && diag && mess == 0 ) {
		return bc( g_bake_d1_diag );
	}
	static const Bytecode tafel[ 2 ][ 3 ] = { { bc( g_bake_d0_m0 ), bc( g_bake_d0_m1 ), bc( g_bake_d0_m2 ) },
	                                         { bc( g_bake_d1_m0 ), bc( g_bake_d1_m1 ), bc( g_bake_d1_m2 ) } };
	return tafel[ doppelt ? 1 : 0 ][ std::clamp( mess, 0, 2 ) ];
}

struct Gpu
{
	HMODULE d3d = nullptr, dxgi = nullptr;
	ID3D11Device *geraet = nullptr;
	ID3D11DeviceContext *ktx = nullptr;
	ID3D11ComputeShader *cs = nullptr;
	ID3D11Buffer *szene[ 6 ] = {};
	ID3D11ShaderResourceView *szeneSrv[ 6 ] = {};
	ID3D11Buffer *licht[ 2 ] = {};
	ID3D11ShaderResourceView *lichtSrv[ 2 ] = {};
	ID3D11Buffer *block[ 3 ] = {};
	ID3D11ShaderResourceView *blockSrv[ 3 ] = {};
	ID3D11Buffer *erg = nullptr, *lesen = nullptr, *kopf = nullptr;
	ID3D11UnorderedAccessView *ergUav = nullptr;
	UINT ergKapazitaet = 0;
	int kopfKnoten = 0;
	UINT visBytes = 0;
	int leafBytes = 0;
	BhedBakeSchalter schalter{};
	bool doppelt = false;
	bool bereit = false;
	std::string name;
	BhedBakeZahlen zahlen;
	long long verlustNach = -1;   // Test: nach so vielen Dispatches einen Geraeteverlust vortaeuschen
};
Gpu g;

template<typename T>
void frei( T *&p ){
	if ( p != nullptr ) {
		p->Release();
		p = nullptr;
	}
}

bool puffer( const void *daten, UINT stride, UINT anzahl, ID3D11Buffer *&buf, ID3D11ShaderResourceView *&srv, bool roh = false ){
	frei( srv );
	frei( buf );
	if ( anzahl == 0 ) {
		anzahl = 1;
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
	if ( FAILED( g.geraet->CreateBuffer( &bd, &sd, &buf ) ) ) {
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
	return SUCCEEDED( g.geraet->CreateShaderResourceView( buf, &vd, &srv ) );
}

bool ergebnisPlatz( UINT n ){
	if ( n <= g.ergKapazitaet ) {
		return true;
	}
	frei( g.ergUav );
	frei( g.erg );
	frei( g.lesen );
	g.ergKapazitaet = 0;
	UINT k = 65536;
	while ( k < n ) {
		k *= 2;
	}
	D3D11_BUFFER_DESC bd{};
	bd.ByteWidth = UINT( sizeof( BhedBakeErg ) ) * k;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof( BhedBakeErg );
	if ( FAILED( g.geraet->CreateBuffer( &bd, nullptr, &g.erg ) ) ) {
		return false;
	}
	D3D11_UNORDERED_ACCESS_VIEW_DESC ud{};
	ud.Format = DXGI_FORMAT_UNKNOWN;
	ud.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	ud.Buffer.NumElements = k;
	if ( FAILED( g.geraet->CreateUnorderedAccessView( g.erg, &ud, &g.ergUav ) ) ) {
		return false;
	}
	bd.Usage = D3D11_USAGE_STAGING;
	bd.BindFlags = 0;
	bd.MiscFlags = 0;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	if ( FAILED( g.geraet->CreateBuffer( &bd, nullptr, &g.lesen ) ) ) {
		return false;
	}
	g.ergKapazitaet = k;
	return true;
}

void allesFrei(){
	for ( int i = 0; i < 6; ++i ) {
		frei( g.szeneSrv[ i ] );
		frei( g.szene[ i ] );
	}
	for ( int i = 0; i < 2; ++i ) {
		frei( g.lichtSrv[ i ] );
		frei( g.licht[ i ] );
	}
	for ( int i = 0; i < 3; ++i ) {
		frei( g.blockSrv[ i ] );
		frei( g.block[ i ] );
	}
	frei( g.ergUav );
	frei( g.erg );
	frei( g.lesen );
	frei( g.kopf );
	frei( g.cs );
	frei( g.ktx );
	frei( g.geraet );
	g.ergKapazitaet = 0;
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
			Sys_Printf( "BehavEd GPU bake: no adapter \"%s\" - the fastest one\n", wahl.c_str() );
		}
	}
	if ( gewaehlt == nullptr && !echt.empty() ) {
		gewaehlt = echt.front();
	}
	for ( size_t i = 0; i < echt.size(); ++i ) {
		DXGI_ADAPTER_DESC1 d{};
		echt[ i ]->GetDesc1( &d );
		Sys_Printf( "BehavEd GPU bake: adapter %zu: %s (%llu MB)%s\n", i, utf8( d.Description ).c_str(),
		             static_cast<unsigned long long>( d.DedicatedVideoMemory >> 20 ), echt[ i ] == gewaehlt ? " - chosen" : "" );
		if ( echt[ i ] != gewaehlt ) {
			echt[ i ]->Release();
		}
	}
	return gewaehlt;
}

bool dispatchen( UINT basis, UINT anzahl ){
	struct Kopf
	{
		UINT anzahl, basis;
		int kopfKnoten, faster, lightAngleHL, bouncing, testOcclusion, oneSky, noSurfaces;
		float linearScale;
		UINT visBytes;
		int leafBytes;
	} kopf{ anzahl, basis, g.kopfKnoten, g.schalter.faster, g.schalter.lightAngleHL, g.schalter.bouncing, g.schalter.testOcclusion, g.schalter.oneSky,
	        g.schalter.noSurfaces, g.schalter.linearScale, g.visBytes, g.leafBytes };
	g.ktx->UpdateSubresource( g.kopf, 0, nullptr, &kopf, 0, 0 );
	g.ktx->Dispatch( ( anzahl + 63 ) / 64, 1, 1 );
	++g.zahlen.dispatches;
	if ( g.verlustNach >= 0 && g.zahlen.dispatches > g.verlustNach ) {
		return false;   // Test: Geraeteverlust vorgetaeuscht (BHED_GPUBAKE_VERLUST)
	}
	return g.geraet->GetDeviceRemovedReason() == S_OK;
}

} // namespace

bool BhedBakeStart( const std::vector<BhedBakeKnoten>& knoten, const std::vector<int>& items, const std::vector<BhedBakeDreieck>& dreiecke,
                    const std::vector<BhedBakeInfo>& infos, const int *flaechen, int anzahlFlaechen, int kopfKnoten,
                    const unsigned char *vis, std::size_t visBytes, const std::string& adapterWahl ){
	if ( const char *v = std::getenv( "BHED_GPUBAKE_VERLUST" ) ) {
		g.verlustNach = std::atoll( v );
	}
	g.d3d = LoadLibraryW( L"d3d11.dll" );
	if ( g.d3d == nullptr ) {
		Sys_Printf( "BehavEd GPU bake: d3d11.dll missing - CPU\n" );
		return false;
	}
	const auto erzeugen = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>( reinterpret_cast<void *>( GetProcAddress( g.d3d, "D3D11CreateDevice" ) ) );
	if ( erzeugen == nullptr ) {
		return false;
	}
	IDXGIAdapter1 *adapter = adapterWaehlen( adapterWahl );
	const D3D_FEATURE_LEVEL stufen[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL stufe{};
	const HRESULT erzeugt = erzeugen( adapter, adapter != nullptr ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, stufen, 2, D3D11_SDK_VERSION,
	                                  &g.geraet, &stufe, &g.ktx );
	if ( adapter != nullptr ) {
		DXGI_ADAPTER_DESC1 d{};
		adapter->GetDesc1( &d );
		g.name = utf8( d.Description );
		adapter->Release();
	}
	if ( FAILED( erzeugt ) ) {
		Sys_Printf( "BehavEd GPU bake: no Direct3D 11 hardware device - CPU\n" );
		return false;
	}
	D3D11_FEATURE_DATA_DOUBLES dd{};
	D3D11_FEATURE_DATA_D3D11_OPTIONS op{};
	const bool d1 = SUCCEEDED( g.geraet->CheckFeatureSupport( D3D11_FEATURE_DOUBLES, &dd, sizeof( dd ) ) ) && dd.DoublePrecisionFloatShaderOps;
	const bool d2 = SUCCEEDED( g.geraet->CheckFeatureSupport( D3D11_FEATURE_D3D11_OPTIONS, &op, sizeof( op ) ) ) && op.ExtendedDoublesShaderInstructions;
	g.doppelt = d1 && d2 && std::getenv( "BHED_GPUBAKE_OHNE_DOUBLE" ) == nullptr;
	const char *mess = std::getenv( "BHED_GPUBAKE_MESS" );
	const Bytecode code = shaderWahl( g.doppelt, mess != nullptr ? std::atoi( mess ) : 0, std::getenv( "BHED_GPUBAKE_DIAG" ) != nullptr );
	if ( FAILED( g.geraet->CreateComputeShader( code.daten, code.groesse, nullptr, &g.cs ) ) ) {
		Sys_Printf( "BehavEd GPU bake: the device does not take the shader - CPU\n" );
		allesFrei();
		return false;
	}
	// Speicherbedarf der Szene (BVH-Ersatz: Knoten, Dreiecke) gegen den Grafikspeicher
	const size_t bedarf = knoten.size() * sizeof( BhedBakeKnoten ) + items.size() * 4 + dreiecke.size() * sizeof( BhedBakeDreieck ) +
	                      infos.size() * sizeof( BhedBakeInfo ) + visBytes;
	std::vector<unsigned char> visKopie( vis != nullptr ? vis : reinterpret_cast<const unsigned char *>( "" ), vis != nullptr ? vis + visBytes : reinterpret_cast<const unsigned char *>( "" ) );
	visKopie.resize( ( visKopie.size() + 3 ) & ~size_t( 3 ), 0 );
	if ( visKopie.empty() ) {
		visKopie.assign( 4, 0 );
	}
	bool ok = puffer( knoten.data(), sizeof( BhedBakeKnoten ), UINT( knoten.size() ), g.szene[ 0 ], g.szeneSrv[ 0 ] ) &&
	          puffer( items.data(), sizeof( int ), UINT( items.size() ), g.szene[ 1 ], g.szeneSrv[ 1 ] ) &&
	          puffer( dreiecke.data(), sizeof( BhedBakeDreieck ), UINT( dreiecke.size() ), g.szene[ 2 ], g.szeneSrv[ 2 ] ) &&
	          puffer( infos.data(), sizeof( BhedBakeInfo ), UINT( infos.size() ), g.szene[ 3 ], g.szeneSrv[ 3 ] ) &&
	          puffer( anzahlFlaechen > 0 ? flaechen : nullptr, sizeof( int ), UINT( anzahlFlaechen ), g.szene[ 4 ], g.szeneSrv[ 4 ] ) &&
	          puffer( visKopie.data(), 4, UINT( visKopie.size() / 4 ), g.szene[ 5 ], g.szeneSrv[ 5 ], true );
	if ( ok ) {
		D3D11_BUFFER_DESC bd{};
		bd.ByteWidth = 48;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		ok = SUCCEEDED( g.geraet->CreateBuffer( &bd, nullptr, &g.kopf ) ) && ergebnisPlatz( 1u << 20 );
	}
	if ( !ok ) {
		Sys_Printf( "BehavEd GPU bake: not enough graphics memory for the scene (%zu MB) - CPU\n", bedarf >> 20 );
		allesFrei();
		return false;
	}
	g.kopfKnoten = kopfKnoten;
	g.visBytes = UINT( visBytes );
	g.leafBytes = visBytes > 8 ? reinterpret_cast<const int *>( vis )[ 1 ] : 0;
	g.bereit = true;
	Sys_Printf( "BehavEd GPU bake: %s (feature level %x, %s), %zu nodes, %zu triangles, scene %zu KB\n", g.name.c_str(), unsigned( stufe ),
	             g.doppelt ? "double precision" : "no double precision - float approximations", knoten.size(), dreiecke.size(), bedarf >> 10 );
	return true;
}

bool BhedBakeLichter( const std::vector<BhedBakeLicht>& lichter, const std::vector<float>& windungen, const BhedBakeSchalter& s,
                      const std::vector<BhedBakeInfo>& infos ){
	if ( !g.bereit ) {
		return false;
	}
	g.schalter = s;
	if ( !puffer( infos.data(), sizeof( BhedBakeInfo ), UINT( infos.size() ), g.szene[ 3 ], g.szeneSrv[ 3 ] ) ||
	     !puffer( lichter.data(), sizeof( BhedBakeLicht ), UINT( lichter.size() ), g.licht[ 0 ], g.lichtSrv[ 0 ] ) ||
	     !puffer( windungen.data(), sizeof( float ), UINT( windungen.size() ), g.licht[ 1 ], g.lichtSrv[ 1 ] ) ) {
		ausfall( "not enough graphics memory for the lights" );
		return false;
	}
	return true;
}

bool BhedBakeRechnen( const std::vector<BhedBakeLuxel>& luxel, const std::vector<BhedBakeLm>& lms, const std::vector<BhedBakePaar>& paare,
                      std::vector<BhedBakeErg>& erg ){
	const size_t n = paare.size();
	erg.resize( n );
	const auto alleCpu = [&](){
		for ( BhedBakeErg& e : erg ) {
			e = BhedBakeErg{ { 0, 0, 0 }, 9 };
		}
		g.zahlen.cpuPaare += static_cast<long long>( n );
	};
	if ( !g.bereit ) {
		alleCpu();
		return false;
	}
	if ( n == 0 ) {
		return true;
	}
	const auto t0 = std::chrono::steady_clock::now();
	if ( !puffer( luxel.data(), sizeof( BhedBakeLuxel ), UINT( luxel.size() ), g.block[ 0 ], g.blockSrv[ 0 ] ) ||
	     !puffer( lms.data(), sizeof( BhedBakeLm ), UINT( lms.size() ), g.block[ 1 ], g.blockSrv[ 1 ] ) ||
	     !puffer( paare.data(), sizeof( BhedBakePaar ), UINT( n ), g.block[ 2 ], g.blockSrv[ 2 ] ) || !ergebnisPlatz( UINT( n ) ) ) {
		ausfall( "not enough graphics memory for a block" );
		alleCpu();
		return false;
	}
	ID3D11ShaderResourceView *srv[ 11 ] = { g.szeneSrv[ 0 ], g.szeneSrv[ 1 ], g.szeneSrv[ 2 ], g.szeneSrv[ 3 ], g.szeneSrv[ 4 ], g.szeneSrv[ 5 ],
	                                        g.lichtSrv[ 0 ], g.lichtSrv[ 1 ], g.blockSrv[ 0 ], g.blockSrv[ 1 ], g.blockSrv[ 2 ] };
	g.ktx->CSSetShader( g.cs, nullptr, 0 );
	g.ktx->CSSetShaderResources( 0, 11, srv );
	g.ktx->CSSetUnorderedAccessViews( 0, 1, &g.ergUav, nullptr );
	g.ktx->CSSetConstantBuffers( 0, 1, &g.kopf );
	// Kurze Dispatches (Windows-Zeitlimit TDR 2 s, die Oberflaeche nutzt dieselbe GPU):
	// je 32768 Paare, nach je 16 ein Flush mit Warten auf die GPU.
	constexpr UINT kStueck = 32768;
	bool ok = true;
	ID3D11Query *ende = nullptr;
	D3D11_QUERY_DESC qd{ D3D11_QUERY_EVENT, 0 };
	(void)g.geraet->CreateQuery( &qd, &ende );
	int seitWarten = 0;
	for ( UINT basis = 0; basis < UINT( n ) && ok; basis += kStueck ) {
		ok = dispatchen( basis, std::min<UINT>( kStueck, UINT( n ) - basis ) );
		if ( ok && ++seitWarten >= 16 && ende != nullptr ) {
			seitWarten = 0;
			g.ktx->End( ende );
			BOOL fertig = FALSE;
			while ( g.ktx->GetData( ende, &fertig, sizeof( fertig ), 0 ) == S_FALSE ) {
				if ( g.geraet->GetDeviceRemovedReason() != S_OK ) {
					ok = false;
					break;
				}
				SwitchToThread();
			}
		}
	}
	frei( ende );
	if ( ok ) {
		ID3D11UnorderedAccessView *keine = nullptr;
		g.ktx->CSSetUnorderedAccessViews( 0, 1, &keine, nullptr );
		D3D11_BOX box{ 0, 0, 0, UINT( n * sizeof( BhedBakeErg ) ), 1, 1 };
		g.ktx->CopySubresourceRegion( g.lesen, 0, 0, 0, 0, g.erg, 0, &box );
		D3D11_MAPPED_SUBRESOURCE m{};
		if ( SUCCEEDED( g.ktx->Map( g.lesen, 0, D3D11_MAP_READ, 0, &m ) ) ) {
			std::memcpy( erg.data(), m.pData, n * sizeof( BhedBakeErg ) );
			g.ktx->Unmap( g.lesen, 0 );
		}
		else {
			ok = false;
		}
	}
	g.zahlen.gpuMikros += std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::steady_clock::now() - t0 ).count();
	++g.zahlen.bloecke;
	if ( !ok ) {
		char z[ 160 ];
		std::snprintf( z, sizeof( z ), "the graphics device was lost (%s)", g.verlustNach >= 0 ? "simulated for the test" : "TDR or driver reset" );
		ausfall( z );
		alleCpu();
		return false;
	}
	// Treiberfehler: nicht endliche Werte rechnet die CPU nach
	for ( BhedBakeErg& e : erg ) {
		if ( e.code != 9 && !( std::isfinite( e.c[ 0 ] ) && std::isfinite( e.c[ 1 ] ) && std::isfinite( e.c[ 2 ] ) ) ) {
			e.code = 9;
			++g.zahlen.nichtEndlich;
		}
		if ( e.code == 9 ) {
			++g.zahlen.cpuPaare;
		}
	}
	g.zahlen.paare += static_cast<long long>( n );
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
