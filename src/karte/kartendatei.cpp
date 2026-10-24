// kartendatei.cpp - Karten-Editor: Map Info, Find Brush, Import und Autosave.
//
// Portiert aus NetRadiant-Custom 20260114 (Quellen in kartendatei.h).
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
#include "bhed/karte/kartendatei.h"

#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/region.h"

#include <cctype>
#include <filesystem>
#include <memory>
#include <system_error>

namespace bhed::karte {

namespace {

// string_compare_nocase_n (strnicmp): hoechstens n Zeichen, endet am Nullzeichen
bool gleichOhneFallN(const char* a, const char* b, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        const int ca = std::tolower(static_cast<unsigned char>(a[i]));
        const int cb = std::tolower(static_cast<unsigned char>(b[i]));
        if (ca != cb) { return false; }
        if (ca == 0) { return true; }
    }
    return true;
}

bool gleichOhneFall(const std::string& a, const char* b) { return gleichOhneFallN(a.c_str(), b, a.size() + 1); }

bool pfadTrenner(char c) { return c == '/' || c == '\\'; }   // path_separator

std::size_t dateinameAnfang(const std::string& p) {   // path_get_filename_start
    std::size_t i = p.size();
    while (i != 0 && !pfadTrenner(p[i - 1])) { --i; }
    return i;
}

std::size_t dateinameBasisEnde(const std::string& p) {   // path_get_filename_base_end
    std::size_t i = p.size();
    while (i != 0) {
        --i;
        if (pfadTrenner(p[i])) { break; }
        if (p[i] == '.') { return i; }
    }
    return p.size();
}

std::filesystem::path utf8(const std::string& p) {
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(p.data()), p.size()));
}

bool istLeer(const std::weak_ptr<Primitiv>& w) {
    return !w.owner_before(std::weak_ptr<Primitiv>{}) && !std::weak_ptr<Primitiv>{}.owner_before(w);
}

}  // namespace

// --- DoMapInfo ---------------------------------------------------------------------------

KartenInfo karteInfo(const Karte& k) {
    KartenInfo i;
    for (const auto& ent : k.entitaeten()) {
        ++i.entitaeten;
        const std::string klasse = ent->klassenname();
        ++i.aufschluesselung[klasse];   // EntityBreakdownWalker
        for (const auto& p : ent->kinder()) {
            if (p->art() == Primitiv::Art::Pinsel) {
                ++i.pinsel;
            } else {
                ++i.patches;
            }
        }
        // CountStuffWalker
        const char* c = klasse.c_str();
        if (ent->istBehaelter()) {
            ++i.gruppen;
            if (!gleichOhneFall(klasse, "func_group") && !gleichOhneFall(klasse, "_decal") && !gleichOhneFallN(c, "func_detail", 11)) {
                ++i.gruppenImSpiel;
                ++i.imSpiel;
            }
        } else if (!gleichOhneFallN(c, "light", 5) && !gleichOhneFall(klasse, "misc_model") && !gleichOhneFall(klasse, "info_null")) {
            ++i.imSpiel;
        }
    }
    return i;
}

// --- Find Brush --------------------------------------------------------------------------

void auswahlNummern(const Karte& k, int& entitaet, int& pinsel) {   // GetSelectionIndex
    std::size_t zaehlerPinsel = 0;
    std::size_t zaehlerEntitaet = 0;
    const auto& el = k.auswahl.elemente();
    if (!el.empty()) {
        const auto ent = el.back().entitaet.lock();   // ultimateSelected
        const auto p = el.back().primitiv.lock();
        if (ent && p) {   // path.size() == 3: BrushFindIndexWalker
            for (const auto& kind : ent->kinder()) {
                if (kind == p) { break; }
                ++zaehlerPinsel;
            }
        }
        if (ent && (p || istLeer(el.back().primitiv))) {   // EntityFindIndexWalker
            for (const auto& e : k.entitaeten()) {
                if (e == ent) { break; }
                ++zaehlerEntitaet;
            }
        }
    }
    pinsel = static_cast<int>(zaehlerPinsel);
    entitaet = static_cast<int>(zaehlerEntitaet);
}

bool pinselFinden(Karte& k, int entitaet, int pinsel, Vektor3f* mitte) {   // SelectBrush
    if (entitaet < 0 || static_cast<std::size_t>(entitaet) >= k.entitaeten().size()) { return false; }
    const auto ent = k.entitaeten()[static_cast<std::size_t>(entitaet)];   // EntityFindByIndexWalker
    if (pinsel >= 0 && static_cast<std::size_t>(pinsel) < ent->kinder().size()) {   // BrushFindByIndexWalker
        const auto p = ent->kinder()[static_cast<std::size_t>(pinsel)];
        k.auswahl.waehlen(ent, p);
        if (mitte != nullptr) { *mitte = instanzGrenzen(*p, k.kontext).origin; }
        return true;
    }
    if (!ent->kinder().empty()) { return false; }   // path.size() == 2 && !Node_hasChildren
    k.auswahl.waehlen(ent);
    if (mitte != nullptr) { *mitte = instanzGrenzen(*ent, k.kontext).origin; }
    return true;
}

// --- Import ------------------------------------------------------------------------------

void karteImportieren(Karte& k, const std::string& text) {   // ImportMap / Map_ImportFile
    k.undo.start();
    einfuegen(k, text, false);   // MergeMap: die vorige Auswahl bleibt
    k.undo.finish("mapImport");
}

// --- Autosave ----------------------------------------------------------------------------

AutoSpeichern::Aktion AutoSpeichern::pruefen(std::time_t jetzt, std::size_t aenderungen, bool unbenannt) {   // QE_CheckAutoSave
    if (m_start == 0 || m_aenderungen == aenderungen) { m_start = jetzt; }
    if ((jetzt - m_start) > (60 * minuten)) {
        m_start = jetzt;
        m_aenderungen = aenderungen;
        if (!an) { return Aktion::Uebersprungen; }   // "Autosave skipped..."
        // only snapshot if not working on a default map
        return schnappschuesse && !unbenannt ? Aktion::Schnappschuss : Aktion::Speichern;
    }
    return Aktion::Nichts;
}

std::string autosavePfad(const std::string& karte) {   // StringRange( name, extension ), ".autosave", extension
    const std::size_t ende = dateinameBasisEnde(karte);
    return karte.substr(0, ende) + ".autosave" + karte.substr(ende);
}

std::string schnappschussOrdner(const std::string& karte) {   // PathFilenameless( mapname ), "snapshots"
    return karte.substr(0, dateinameAnfang(karte)) + "snapshots";
}

std::string schnappschussPfad(const std::string& karte, std::uintmax_t* summe) {   // Map_Snapshot
    const std::string neu = schnappschussOrdner(karte) + '/' + karte.substr(dateinameAnfang(karte));
    const std::size_t ext = dateinameBasisEnde(neu);
    std::uintmax_t groesse = 0;
    std::string pfad;
    for (int n = 0;; ++n) {
        // "<path>/snapshots/<name>.<count>.<ext>"
        pfad = neu.substr(0, ext) + '.' + std::to_string(n) + neu.substr(ext);
        std::error_code ec;
        if (!std::filesystem::exists(utf8(pfad), ec)) { break; }   // DoesFileExist
        const auto g = std::filesystem::file_size(utf8(pfad), ec);
        if (!ec) { groesse += g; }
    }
    if (summe != nullptr) { *summe = groesse; }
    return pfad;
}

}  // namespace bhed::karte
