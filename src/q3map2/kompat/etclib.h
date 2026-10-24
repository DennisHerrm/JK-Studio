// etclib.h - ETC1-Bloecke entpacken (Stufe 3 Punkt 1, Ersatz fuer libs/etclib.h).
//
// q3map2 (NRC 20260114, tools/quake3/common/qimagelib.cpp LoadKTXBufferFirstImage)
// ruft ETC_DecodeETC1Block fuer KTX-Bilder mit ETC1. NRCs libs/etclib.c stammt aus
// Android (Copyright 2009 Google Inc.) und steht unter der Apache-Lizenz 2.0 - die
// ist mit der GPL v2 dieses Programms nicht vertraeglich. Darum eine eigene
// Fassung mit derselben Schnittstelle, nach der Spezifikation geschrieben
// (Khronos OES_compressed_ETC1_RGB8_texture, Abschnitt "ETC1 Compressed Texture
// Image Formats"); das Ergebnis ist durch die Spezifikation festgelegt.
//
// Ausgabe: 4 x 4 Bildpunkte zeilenweise (x + 4 * y), RGB oder RGBA (A = 255).
//
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#pragma once

#include "bytebool.h"

#ifdef __cplusplus
extern "C"
{
#endif

void ETC_DecodeETC1Block( const byte* in, byte* out, bool outRGBA );

#ifdef __cplusplus
}
#endif
