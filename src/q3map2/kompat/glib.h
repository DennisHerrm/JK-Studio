// glib.h - das Wenige aus GLib, das q3map2 braucht (Stufe 3 Punkt 1).
//
// tools/quake3/common/inout.cpp (NRC 20260114) wandelt jede Meldung fuer den
// XML-Strom zum Editor mit g_locale_to_utf8 von der Zeichensatz-Einstellung des
// Systems nach UTF-8 und gibt sie mit g_free frei. Fuer diese zwei Aufrufe die
// ganze GLib (LGPL, mit meson, gnulib und pcre2) unter MSVC mitzubauen, waere
// ein grosser Brocken; unter Windows nimmt GLib dafuer den ANSI-Zeichensatz des
// Systems (g_get_charset -> GetACP), hier dasselbe mit MultiByteToWideChar
// (CP_ACP) und WideCharToMultiByte (CP_UTF8). Anpassung an MSVC Nummer 1
// (NOTIZEN_karte3.md).
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

#include <cstdlib>
#include <cstring>
#include <cwchar>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

typedef char gchar;
typedef long gssize;
typedef unsigned long gsize;
struct GError;

// g_locale_to_utf8( text, -1, nullptr, nullptr, nullptr ): neu angelegt (g_free),
// nullptr, wenn sich der Text nicht wandeln laesst.
inline gchar* g_locale_to_utf8( const gchar* text, gssize laenge, gsize* gelesen, gsize* geschrieben, GError** fehler ){
	(void)fehler;
	const int n = laenge < 0 ? static_cast<int>( std::strlen( text ) ) : static_cast<int>( laenge );
	if ( gelesen != nullptr ) {
		*gelesen = static_cast<gsize>( n );
	}
	const int w = n > 0 ? MultiByteToWideChar( CP_ACP, 0, text, n, nullptr, 0 ) : 0;
	if ( n > 0 && w <= 0 ) {
		return nullptr;
	}
	wchar_t* breit = static_cast<wchar_t*>( std::malloc( sizeof( wchar_t ) * static_cast<size_t>( w + 1 ) ) );
	if ( breit == nullptr ) {
		return nullptr;
	}
	if ( w > 0 ) {
		MultiByteToWideChar( CP_ACP, 0, text, n, breit, w );
	}
	const int u = w > 0 ? WideCharToMultiByte( CP_UTF8, 0, breit, w, nullptr, 0, nullptr, nullptr ) : 0;
	gchar* utf8 = static_cast<gchar*>( std::malloc( static_cast<size_t>( u + 1 ) ) );
	if ( utf8 != nullptr ) {
		if ( u > 0 ) {
			WideCharToMultiByte( CP_UTF8, 0, breit, w, utf8, u, nullptr, nullptr );
		}
		utf8[u] = '\0';
		if ( geschrieben != nullptr ) {
			*geschrieben = static_cast<gsize>( u );
		}
	}
	std::free( breit );
	return utf8;
}

inline void g_free( void* p ){
	std::free( p );
}

// GDir (tools/quake3/common/vfs.cpp: Pakete und .shader-Dateien eines Ordners).
// GLib oeffnet unter Windows den Ordner mit _wopendir (UTF-8 -> UTF-16), liest
// mit _wreaddir in der Reihenfolge des Dateisystems (FindFirstFileW/
// FindNextFileW), laesst "." und ".." aus und gibt die Namen in UTF-8 zurueck -
// hier dasselbe direkt mit FindFirstFileW. Anpassung an MSVC Nummer 1 (wie oben).
struct GDir {
	HANDLE suche = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATAW daten{};
	bool erster = true;
	char name[4 * MAX_PATH] = {};
};

inline GDir* g_dir_open( const gchar* pfad, unsigned int flags, GError** fehler ){
	(void)flags;
	(void)fehler;
	const int n = MultiByteToWideChar( CP_UTF8, 0, pfad, -1, nullptr, 0 );
	if ( n <= 0 ) {
		return nullptr;
	}
	wchar_t* breit = static_cast<wchar_t*>( std::malloc( sizeof( wchar_t ) * static_cast<size_t>( n + 3 ) ) );
	if ( breit == nullptr ) {
		return nullptr;
	}
	MultiByteToWideChar( CP_UTF8, 0, pfad, -1, breit, n );
	size_t l = std::wcslen( breit );
	if ( l > 0 && breit[l - 1] != L'/' && breit[l - 1] != L'\\' ) {
		breit[l++] = L'\\';
	}
	breit[l++] = L'*';
	breit[l] = L'\0';
	GDir* d = new GDir;
	d->suche = FindFirstFileW( breit, &d->daten );
	std::free( breit );
	if ( d->suche == INVALID_HANDLE_VALUE ) {
		delete d;
		return nullptr;
	}
	return d;
}

inline const gchar* g_dir_read_name( GDir* d ){
	for ( ;; ) {
		if ( !d->erster && FindNextFileW( d->suche, &d->daten ) == 0 ) {
			return nullptr;
		}
		d->erster = false;
		const wchar_t* w = d->daten.cFileName;
		if ( ( w[0] == L'.' && w[1] == L'\0' ) || ( w[0] == L'.' && w[1] == L'.' && w[2] == L'\0' ) ) {
			continue;
		}
		if ( WideCharToMultiByte( CP_UTF8, 0, w, -1, d->name, static_cast<int>( sizeof( d->name ) ), nullptr, nullptr ) <= 0 ) {
			continue;
		}
		return d->name;
	}
}

inline void g_dir_close( GDir* d ){
	if ( d != nullptr ) {
		FindClose( d->suche );
		delete d;
	}
}
