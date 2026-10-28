// bhed_dxr_aus.cpp - BehavEd Stufe 8g Punkt 3: der DXR-Vortest, wenn im Bau kein dxc da war
// (siehe bhed_dxr.h): er meldet das und tut nichts.
//
//   Copyright (C) 2026 BehavEd-Remake contributors.
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
#include "q3map2.h"
#include "bhed_dxr.h"

bool BhedDxrStart( const std::vector<float>& ){
	Sys_Printf( "BehavEd DXR pre-test: built without dxc - not used\n" );
	return false;
}
bool BhedDxrStrahlen( const BhedGpuStrahl *, std::size_t, std::vector<std::uint32_t>& treffer ){
	treffer.clear();
	return false;
}
bool BhedDxrBereit(){
	return false;
}
long long BhedDxrMikros(){
	return 0;
}
std::string BhedDxrAdapter(){
	return std::string();
}
void BhedDxrEnde(){
}
