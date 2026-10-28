// wiederherstellen.cpp - Karten-Editor, Stufe 9 Punkt 12 (siehe wiederherstellen.h)
//
// Portiert aus NetRadiant-Custom 20260114 (der Name der Autosave-Datei, autosave.cpp); die
// Sitzungsdatei ist neu.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#include "bhed/karte/wiederherstellen.h"

#include "bhed/karte/kartendatei.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace bhed::karte {

namespace {
std::filesystem::path pfadVon(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }
}  // namespace

Wiederherstellung wiederherstellungPruefen(const std::string& sitzungsPfad, const std::string& unbenanntAutosave, unsigned long prozess) {
    Wiederherstellung w;
    std::ifstream f(pfadVon(sitzungsPfad), std::ios::binary);
    if (!f) { return w; }
    std::string karte;
    std::string nummer;
    std::getline(f, karte);
    std::getline(f, nummer);
    while (!karte.empty() && (karte.back() == '\r' || karte.back() == '\n')) { karte.pop_back(); }
    if (karte.empty() || nummer == std::to_string(prozess)) { return w; }   // dieser Prozess: kein Absturz
    w.karte = karte;
    w.autosave = karte == "unnamed.map" ? unbenanntAutosave : autosavePfad(karte);
    std::error_code e1, e2;
    if (w.autosave.empty() || !std::filesystem::is_regular_file(pfadVon(w.autosave), e1)) { return w; }
    const auto ta = std::filesystem::last_write_time(pfadVon(w.autosave), e1);
    const auto tk = std::filesystem::last_write_time(pfadVon(karte), e2);
    w.anbieten = !e1 && (e2 || karte == "unnamed.map" || ta > tk);
    return w;
}

void sitzungSchreiben(const std::string& sitzungsPfad, const std::string& karte, unsigned long prozess) {
    std::ofstream f(pfadVon(sitzungsPfad), std::ios::binary | std::ios::trunc);
    f << karte << "\n" << prozess << "\n";
}

void sitzungLoeschen(const std::string& sitzungsPfad) {
    std::error_code ec;
    std::filesystem::remove(pfadVon(sitzungsPfad), ec);
}

}  // namespace bhed::karte
