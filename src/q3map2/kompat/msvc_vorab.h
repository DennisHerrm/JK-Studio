// msvc_vorab.h - vor jeder Quelle von q3map2 eingebunden (/FI), Stufe 3 Punkt 1.
//
// Was MinGW von sich aus mitbringt und MSVC nicht, ohne die Quellen von NRC zu
// aendern (Anpassungen an MSVC, Liste in NOTIZEN_karte3.md):
//
//   Nr. 2  R_OK / access(): tools/quake3/common/cmdlib.cpp:172 prueft mit
//          access( path, R_OK ), ob eine Datei lesbar ist. MinGW liefert beides
//          ueber <io.h>, MSVC kennt access() (veraltet als POSIX-Name, siehe
//          _CRT_NONSTDC_NO_DEPRECATE) aber nicht R_OK.
//   Nr. 4  ASSIMP_INCLUDE(x): model.cpp bindet assimp ueber dieses Makro ein
//          (Makefile: -DASSIMP_INCLUDE(x)=<x>). Ein Makro mit Klammern und
//          spitzen Klammern ueberlebt den Weg ueber MSBuild auf die
//          Befehlszeile nicht - darum hier.
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#pragma once

#include <io.h>

#ifndef F_OK
#define F_OK 0
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef R_OK
#define R_OK 4
#endif

#define ASSIMP_INCLUDE(x) <x>
