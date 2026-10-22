// protokoll.h - Karten-Editor: Meldungen fuer die Konsole, so wie NRC sie
// schreibt (globalOutputStream / globalWarningStream / globalErrorStream).
// Ein Teil ist ein Stueck Text mit seiner Art; Zeilen enden erst am '\n',
// darum stehen z. B. "Open file X for read..." (Ausgabe) und "failure\n"
// (Fehler) auf einer Zeile wie in NRCs Konsole (Sys_Print, console.cpp).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/console.cpp (Sys_Print,
// SYS_STD / SYS_WRN / SYS_ERR) und include/iscriplib.h, libs/stream.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_KARTE_PROTOKOLL_H
#define BHED_KARTE_PROTOKOLL_H

#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

struct ProtokollTeil {
    enum class Art { Ausgabe, Warnung, Fehler };   // SYS_STD, SYS_WRN, SYS_ERR
    Art art = Art::Ausgabe;
    std::string text;                              // wie geschrieben, mit '\n'
};
using Protokoll = std::vector<ProtokollTeil>;

inline void schreiben(Protokoll& p, ProtokollTeil::Art art, std::string text) { p.push_back({art, std::move(text)}); }
inline void ausgabe(Protokoll& p, std::string text) { schreiben(p, ProtokollTeil::Art::Ausgabe, std::move(text)); }
inline void warnung(Protokoll& p, std::string text) { schreiben(p, ProtokollTeil::Art::Warnung, std::move(text)); }
inline void fehler(Protokoll& p, std::string text) { schreiben(p, ProtokollTeil::Art::Fehler, std::move(text)); }

}  // namespace bhed::karte

#endif
