/* etclib.c - ETC1-Bloecke entpacken (Stufe 3 Punkt 1, Ersatz fuer libs/etclib.c).
 *
 * Nach der Spezifikation Khronos OES_compressed_ETC1_RGB8_texture (siehe
 * etclib.h, warum nicht NRCs Fassung): ein Block sind 64 Bit, das hoehere Wort
 * zuerst (big endian).
 *   - Bit 33 "diff", Bit 32 "flip" (Bits 1 und 0 des hoeheren Wortes).
 *   - diff = 0 (einzeln): je Farbe zwei 4-Bit-Grundwerte (R1 63..60, R2 59..56,
 *     G1 55..52, G2 51..48, B1 47..44, B2 43..40), auf 8 Bit durch
 *     Wiederholen (x * 17).
 *   - diff = 1 (differenziell): je Farbe ein 5-Bit-Grundwert (R1' 63..59,
 *     G1' 55..51, B1' 47..43) und ein 3-Bit-Unterschied im Zweierkomplement
 *     (58..56, 50..48, 42..40); der zweite Wert ist die 5-Bit-Summe; auf 8 Bit
 *     durch (x << 3) | (x >> 2).
 *   - Tabellennummer des ersten Teilblocks Bits 39..37, des zweiten 36..34.
 *   - Teilbloecke: ohne flip die Spalten 0-1 und 2-3, mit flip die Zeilen 0-1
 *     und 2-3.
 *   - Bildpunkt (x, y): Bit a = x * 4 + y des niedrigeren Wortes ist das
 *     niedrige, Bit 16 + a das hohe Bit der Nummer; 0 = +klein, 1 = +gross,
 *     2 = -klein, 3 = -gross aus der Tabelle der Spezifikation; jede Farbe
 *     Grundwert + Wert, auf 0..255 begrenzt.
 *
 *   Copyright (C) 2026 BehavEd-Remake contributors.
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version. This program is distributed in the hope that it will be
 *   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 *   Public License for more details (LICENSE).
 */
#include "etclib.h"

/* Tabelle 3.17.2 der Spezifikation: der kleine und der grosse Wert je Nummer */
static const int etc1_werte[8][2] = {
	{ 2, 8 }, { 5, 17 }, { 9, 29 }, { 13, 42 }, { 18, 60 }, { 24, 80 }, { 33, 106 }, { 47, 183 }
};

static int etc1_begrenzt( int v ){
	return v < 0 ? 0 : ( v > 255 ? 255 : v );
}

/* 3-Bit-Zweierkomplement -4..3 */
static int etc1_unterschied( unsigned int bits3 ){
	return ( bits3 & 4u ) != 0 ? (int)bits3 - 8 : (int)bits3;
}

void ETC_DecodeETC1Block( const byte* in, byte* out, bool outRGBA ){
	const unsigned int hoch = ( (unsigned int)in[0] << 24 ) | ( (unsigned int)in[1] << 16 ) | ( (unsigned int)in[2] << 8 ) | (unsigned int)in[3];
	const unsigned int tief = ( (unsigned int)in[4] << 24 ) | ( (unsigned int)in[5] << 16 ) | ( (unsigned int)in[6] << 8 ) | (unsigned int)in[7];
	const int diff = ( hoch >> 1 ) & 1u;
	const int flip = hoch & 1u;
	int grund[2][3];
	int teil, kanal, x, y;
	const unsigned int tabelle[2] = { ( hoch >> 5 ) & 7u, ( hoch >> 2 ) & 7u };

	for ( kanal = 0; kanal < 3; ++kanal ) {
		/* die Grundwerte stehen in den Bytes 0, 1, 2 des hoeheren Wortes (R, G, B) */
		const unsigned int b = ( hoch >> ( 24 - 8 * kanal ) ) & 0xffu;
		if ( diff ) {
			const unsigned int a5 = b >> 3;
			const unsigned int b5 = ( a5 + (unsigned int)etc1_unterschied( b & 7u ) ) & 31u;
			grund[0][kanal] = (int)( ( a5 << 3 ) | ( a5 >> 2 ) );
			grund[1][kanal] = (int)( ( b5 << 3 ) | ( b5 >> 2 ) );
		}
		else {
			grund[0][kanal] = (int)( ( b >> 4 ) * 17u );
			grund[1][kanal] = (int)( ( b & 15u ) * 17u );
		}
	}

	for ( y = 0; y < 4; ++y ) {
		for ( x = 0; x < 4; ++x ) {
			const int a = x * 4 + y;
			const unsigned int nummer = ( ( tief >> a ) & 1u ) | ( ( ( tief >> ( 16 + a ) ) & 1u ) << 1 );
			int wert;
			byte* q = out + ( outRGBA ? 4 : 3 ) * ( x + 4 * y );
			teil = flip ? ( y >= 2 ) : ( x >= 2 );
			wert = etc1_werte[tabelle[teil]][nummer & 1u];
			if ( nummer & 2u ) {
				wert = -wert;
			}
			for ( kanal = 0; kanal < 3; ++kanal ) {
				q[kanal] = (byte)etc1_begrenzt( grund[teil][kanal] + wert );
			}
			if ( outRGBA ) {
				q[3] = 255;
			}
		}
	}
}
