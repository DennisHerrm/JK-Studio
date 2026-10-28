// bhed_dxr.cpp - BehavEd Stufe 8g Punkt 3: DXR als Vortest (siehe bhed_dxr.h).
//
//   Copyright (C) 2026 BehavEd-Remake contributors.
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
#include "q3map2.h"
#include "bhed_dxr.h"
#include "bhed_gpuschatten.h"

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <mutex>

namespace
{

#include "g_dxr.h"   // DXIL (cs_6_5) aus bhed_dxr.hlsl, im Bau mit dxc uebersetzt

template<typename T>
void frei( T *&p ){
	if ( p != nullptr ) {
		p->Release();
		p = nullptr;
	}
}

struct Dxr
{
	HMODULE d3d12 = nullptr, dxgi = nullptr;
	ID3D12Device5 *geraet = nullptr;
	ID3D12CommandQueue *schlange = nullptr;
	ID3D12CommandAllocator *speicher = nullptr;
	ID3D12GraphicsCommandList4 *liste = nullptr;
	ID3D12Fence *zaun = nullptr;
	UINT64 zaunWert = 0;
	HANDLE ereignis = nullptr;
	ID3D12Resource *ecken = nullptr, *blas = nullptr, *tlas = nullptr, *kratz = nullptr, *instanz = nullptr;
	ID3D12RootSignature *wurzel = nullptr;
	ID3D12PipelineState *pso = nullptr;
	ID3D12Resource *strahlen = nullptr, *treffer = nullptr, *lesen = nullptr;
	std::size_t kapazitaet = 0;
	bool bereit = false;
	long long mikros = 0;
	std::string name;
	std::mutex sperre;
};
Dxr d;

ID3D12Resource *puffer( D3D12_HEAP_TYPE art, UINT64 groesse, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES zustand ){
	D3D12_HEAP_PROPERTIES hp{};
	hp.Type = art;
	D3D12_RESOURCE_DESC rd{};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = std::max<UINT64>( groesse, 256 );
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	rd.Flags = flags;
	ID3D12Resource *r = nullptr;
	if ( FAILED( d.geraet->CreateCommittedResource( &hp, D3D12_HEAP_FLAG_NONE, &rd, zustand, nullptr, __uuidof( ID3D12Resource ), reinterpret_cast<void **>( &r ) ) ) ) {
		return nullptr;
	}
	return r;
}

bool ausfuehrenUndWarten(){
	if ( FAILED( d.liste->Close() ) ) {
		return false;
	}
	ID3D12CommandList *l[] = { d.liste };
	d.schlange->ExecuteCommandLists( 1, l );
	++d.zaunWert;
	if ( FAILED( d.schlange->Signal( d.zaun, d.zaunWert ) ) ) {
		return false;
	}
	if ( d.zaun->GetCompletedValue() < d.zaunWert ) {
		d.zaun->SetEventOnCompletion( d.zaunWert, d.ereignis );
		WaitForSingleObject( d.ereignis, INFINITE );
	}
	return SUCCEEDED( d.speicher->Reset() ) && SUCCEEDED( d.liste->Reset( d.speicher, nullptr ) ) && d.geraet->GetDeviceRemovedReason() == S_OK;
}

void uavSperre( ID3D12Resource *r ){
	D3D12_RESOURCE_BARRIER b{};
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	b.UAV.pResource = r;
	d.liste->ResourceBarrier( 1, &b );
}

void wechsel( ID3D12Resource *r, D3D12_RESOURCE_STATES von, D3D12_RESOURCE_STATES nach ){
	D3D12_RESOURCE_BARRIER b{};
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource = r;
	b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	b.Transition.StateBefore = von;
	b.Transition.StateAfter = nach;
	d.liste->ResourceBarrier( 1, &b );
}

// der Adapter wie bei -bhedgpu: der mit dem meisten eigenen Grafikspeicher (BHED_GPUDXR_ADAPTER=<Teil des
// Namens>: ein bestimmter, etwa die Intel-GPU ohne DXR fuer den Test des Rueckfalls)
IDXGIAdapter1 *adapter(){
	d.dxgi = LoadLibraryW( L"dxgi.dll" );
	if ( d.dxgi == nullptr ) {
		return nullptr;
	}
	using Fabrik = HRESULT( WINAPI * )( REFIID, void ** );
	const auto fabrik = reinterpret_cast<Fabrik>( reinterpret_cast<void *>( GetProcAddress( d.dxgi, "CreateDXGIFactory1" ) ) );
	IDXGIFactory1 *f = nullptr;
	if ( fabrik == nullptr || FAILED( fabrik( __uuidof( IDXGIFactory1 ), reinterpret_cast<void **>( &f ) ) ) ) {
		return nullptr;
	}
	IDXGIAdapter1 *bester = nullptr;
	SIZE_T speicher = 0;
	IDXGIAdapter1 *a = nullptr;
	for ( UINT i = 0; f->EnumAdapters1( i, &a ) != DXGI_ERROR_NOT_FOUND; ++i ) {
		DXGI_ADAPTER_DESC1 desc{};
		const char *wahl = std::getenv( "BHED_GPUDXR_ADAPTER" );
		if ( wahl != nullptr && SUCCEEDED( a->GetDesc1( &desc ) ) ) {
			char z[ 256 ] = {};
			WideCharToMultiByte( CP_UTF8, 0, desc.Description, -1, z, sizeof( z ) - 1, nullptr, nullptr );
			if ( std::strstr( z, wahl ) != nullptr ) {
				frei( bester );
				bester = a;
				break;
			}
			a->Release();
			continue;
		}
		if ( SUCCEEDED( a->GetDesc1( &desc ) ) && !( desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ) && ( bester == nullptr || desc.DedicatedVideoMemory > speicher ) ) {
			frei( bester );
			bester = a;
			speicher = desc.DedicatedVideoMemory;
			continue;
		}
		a->Release();
	}
	f->Release();
	return bester;
}

void allesFrei(){
	frei( d.strahlen );
	frei( d.treffer );
	frei( d.lesen );
	frei( d.pso );
	frei( d.wurzel );
	frei( d.ecken );
	frei( d.blas );
	frei( d.tlas );
	frei( d.kratz );
	frei( d.instanz );
	frei( d.liste );
	frei( d.speicher );
	frei( d.schlange );
	frei( d.zaun );
	frei( d.geraet );
	if ( d.ereignis != nullptr ) {
		CloseHandle( d.ereignis );
		d.ereignis = nullptr;
	}
	d.kapazitaet = 0;
	d.bereit = false;
}

bool kein( const char *grund ){
	Sys_Printf( "BehavEd DXR pre-test: %s - not used\n", grund );
	allesFrei();
	return false;
}

} // namespace

bool BhedDxrStart( const std::vector<float>& ecken ){
	d.d3d12 = LoadLibraryW( L"d3d12.dll" );
	if ( d.d3d12 == nullptr ) {
		return kein( "d3d12.dll missing" );
	}
	const auto erzeugen = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>( reinterpret_cast<void *>( GetProcAddress( d.d3d12, "D3D12CreateDevice" ) ) );
	if ( erzeugen == nullptr ) {
		return kein( "no D3D12CreateDevice" );
	}
	IDXGIAdapter1 *a = adapter();
	if ( a != nullptr ) {
		DXGI_ADAPTER_DESC1 desc{};
		a->GetDesc1( &desc );
		char z[ 256 ] = {};
		WideCharToMultiByte( CP_UTF8, 0, desc.Description, -1, z, sizeof( z ) - 1, nullptr, nullptr );
		d.name = z;
	}
	const HRESULT hr = erzeugen( a, D3D_FEATURE_LEVEL_12_0, __uuidof( ID3D12Device5 ), reinterpret_cast<void **>( &d.geraet ) );
	frei( a );
	if ( FAILED( hr ) ) {
		return kein( "no Direct3D 12 device with ID3D12Device5" );
	}
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 o5{};
	if ( FAILED( d.geraet->CheckFeatureSupport( D3D12_FEATURE_D3D12_OPTIONS5, &o5, sizeof( o5 ) ) ) || o5.RaytracingTier < D3D12_RAYTRACING_TIER_1_1 ) {
		return kein( "the GPU has no DXR tier 1.1 (inline raytracing)" );
	}
	D3D12_COMMAND_QUEUE_DESC qd{};
	qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	if ( FAILED( d.geraet->CreateCommandQueue( &qd, __uuidof( ID3D12CommandQueue ), reinterpret_cast<void **>( &d.schlange ) ) ) ||
	     FAILED( d.geraet->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof( ID3D12CommandAllocator ), reinterpret_cast<void **>( &d.speicher ) ) ) ||
	     FAILED( d.geraet->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, d.speicher, nullptr, __uuidof( ID3D12GraphicsCommandList4 ), reinterpret_cast<void **>( &d.liste ) ) ) ||
	     FAILED( d.geraet->CreateFence( 0, D3D12_FENCE_FLAG_NONE, __uuidof( ID3D12Fence ), reinterpret_cast<void **>( &d.zaun ) ) ) ) {
		return kein( "no command queue" );
	}
	d.ereignis = CreateEventW( nullptr, FALSE, FALSE, nullptr );
	// Wurzelsignatur steht im Shader ([RootSignature]), die Pipeline aus dem vorab uebersetzten DXIL
	if ( FAILED( d.geraet->CreateRootSignature( 0, g_dxr, sizeof( g_dxr ), __uuidof( ID3D12RootSignature ), reinterpret_cast<void **>( &d.wurzel ) ) ) ) {
		return kein( "the device does not take the root signature" );
	}
	D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};
	pd.pRootSignature = d.wurzel;
	pd.CS = { g_dxr, sizeof( g_dxr ) };
	if ( FAILED( d.geraet->CreateComputePipelineState( &pd, __uuidof( ID3D12PipelineState ), reinterpret_cast<void **>( &d.pso ) ) ) ) {
		return kein( "the device does not take the DXIL shader" );
	}
	const auto t0 = std::chrono::steady_clock::now();
	// Ecken (Upload-Heap reicht als Eingabe des Aufbaus)
	const UINT64 bytes = ecken.size() * sizeof( float );
	d.ecken = puffer( D3D12_HEAP_TYPE_UPLOAD, bytes, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ );
	if ( d.ecken == nullptr ) {
		return kein( "not enough memory for the triangles" );
	}
	void *m = nullptr;
	d.ecken->Map( 0, nullptr, &m );
	std::memcpy( m, ecken.data(), bytes );
	d.ecken->Unmap( 0, nullptr );
	D3D12_RAYTRACING_GEOMETRY_DESC geo{};
	geo.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geo.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	geo.Triangles.VertexBuffer.StartAddress = d.ecken->GetGPUVirtualAddress();
	geo.Triangles.VertexBuffer.StrideInBytes = 12;
	geo.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geo.Triangles.VertexCount = UINT( ecken.size() / 3 );
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS unten{};
	unten.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	unten.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	unten.NumDescs = 1;
	unten.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	unten.pGeometryDescs = &geo;
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO iu{};
	d.geraet->GetRaytracingAccelerationStructurePrebuildInfo( &unten, &iu );
	D3D12_RAYTRACING_INSTANCE_DESC inst{};
	inst.Transform[ 0 ][ 0 ] = inst.Transform[ 1 ][ 1 ] = inst.Transform[ 2 ][ 2 ] = 1.0f;
	inst.InstanceMask = 0xff;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS oben{};
	oben.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	oben.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	oben.NumDescs = 1;
	oben.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO io{};
	d.geraet->GetRaytracingAccelerationStructurePrebuildInfo( &oben, &io );
	d.blas = puffer( D3D12_HEAP_TYPE_DEFAULT, iu.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE );
	d.tlas = puffer( D3D12_HEAP_TYPE_DEFAULT, io.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE );
	d.kratz = puffer( D3D12_HEAP_TYPE_DEFAULT, std::max( iu.ScratchDataSizeInBytes, io.ScratchDataSizeInBytes ), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
	                  D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
	d.instanz = puffer( D3D12_HEAP_TYPE_UPLOAD, sizeof( inst ), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ );
	if ( d.blas == nullptr || d.tlas == nullptr || d.kratz == nullptr || d.instanz == nullptr ) {
		return kein( "not enough memory for the acceleration structure" );
	}
	inst.AccelerationStructure = d.blas->GetGPUVirtualAddress();
	d.instanz->Map( 0, nullptr, &m );
	std::memcpy( m, &inst, sizeof( inst ) );
	d.instanz->Unmap( 0, nullptr );
	oben.InstanceDescs = d.instanz->GetGPUVirtualAddress();
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bu{};
	bu.Inputs = unten;
	bu.DestAccelerationStructureData = d.blas->GetGPUVirtualAddress();
	bu.ScratchAccelerationStructureData = d.kratz->GetGPUVirtualAddress();
	d.liste->BuildRaytracingAccelerationStructure( &bu, 0, nullptr );
	uavSperre( d.blas );
	uavSperre( d.kratz );
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bo{};
	bo.Inputs = oben;
	bo.DestAccelerationStructureData = d.tlas->GetGPUVirtualAddress();
	bo.ScratchAccelerationStructureData = d.kratz->GetGPUVirtualAddress();
	d.liste->BuildRaytracingAccelerationStructure( &bo, 0, nullptr );
	uavSperre( d.tlas );
	if ( !ausfuehrenUndWarten() ) {
		return kein( "building the acceleration structure failed" );
	}
	d.bereit = true;
	Sys_Printf( "BehavEd DXR pre-test: %s, %zu triangles, acceleration structure %llu KB in %.0f ms\n", d.name.c_str(), ecken.size() / 9,
	            static_cast<unsigned long long>( ( iu.ResultDataMaxSizeInBytes + io.ResultDataMaxSizeInBytes ) >> 10 ),
	            std::chrono::duration<double, std::milli>( std::chrono::steady_clock::now() - t0 ).count() );
	return true;
}

bool BhedDxrStrahlen( const BhedGpuStrahl *strahlen, std::size_t n, std::vector<std::uint32_t>& treffer ){
	treffer.assign( n, 0 );
	if ( !d.bereit || n == 0 ) {
		return d.bereit;
	}
	std::lock_guard<std::mutex> l( d.sperre );
	const auto t0 = std::chrono::steady_clock::now();
	if ( n > d.kapazitaet ) {
		frei( d.strahlen );
		frei( d.treffer );
		frei( d.lesen );
		std::size_t k = 65536;
		while ( k < n ) {
			k *= 2;
		}
		d.strahlen = puffer( D3D12_HEAP_TYPE_UPLOAD, k * sizeof( BhedGpuStrahl ), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ );
		d.treffer = puffer( D3D12_HEAP_TYPE_DEFAULT, k * 4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
		d.lesen = puffer( D3D12_HEAP_TYPE_READBACK, k * 4, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST );
		if ( d.strahlen == nullptr || d.treffer == nullptr || d.lesen == nullptr ) {
			allesFrei();
			return false;
		}
		d.kapazitaet = k;
	}
	void *m = nullptr;
	d.strahlen->Map( 0, nullptr, &m );
	std::memcpy( m, strahlen, n * sizeof( BhedGpuStrahl ) );
	d.strahlen->Unmap( 0, nullptr );
	d.liste->SetComputeRootSignature( d.wurzel );
	d.liste->SetPipelineState( d.pso );
	d.liste->SetComputeRootShaderResourceView( 0, d.tlas->GetGPUVirtualAddress() );
	d.liste->SetComputeRootShaderResourceView( 1, d.strahlen->GetGPUVirtualAddress() );
	d.liste->SetComputeRootUnorderedAccessView( 2, d.treffer->GetGPUVirtualAddress() );
	d.liste->SetComputeRoot32BitConstant( 3, UINT( n ), 0 );
	d.liste->Dispatch( UINT( ( n + 63 ) / 64 ), 1, 1 );
	wechsel( d.treffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE );
	d.liste->CopyBufferRegion( d.lesen, 0, d.treffer, 0, n * 4 );
	wechsel( d.treffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
	if ( !ausfuehrenUndWarten() ) {
		Sys_Printf( "BehavEd DXR pre-test: the device was lost - stopped\n" );
		allesFrei();
		return false;
	}
	D3D12_RANGE r{ 0, n * 4 };
	d.lesen->Map( 0, &r, &m );
	std::memcpy( treffer.data(), m, n * 4 );
	D3D12_RANGE leer{ 0, 0 };
	d.lesen->Unmap( 0, &leer );
	d.mikros += std::chrono::duration_cast<std::chrono::microseconds>( std::chrono::steady_clock::now() - t0 ).count();
	return true;
}

bool BhedDxrBereit(){
	return d.bereit;
}

long long BhedDxrMikros(){
	return d.mikros;
}

std::string BhedDxrAdapter(){
	return d.name;
}

void BhedDxrEnde(){
	allesFrei();
}
