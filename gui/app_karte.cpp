// app_karte.cpp - der Arbeitsbereich "Karten-Editor" (Stufe 1b): Karte neu,
// oeffnen, speichern, zuletzt geoeffnet, das Spiel des Editors und der Rahmen.
//
// Portiert aus NetRadiant-Custom (Stand master 2026): radiant/map.cpp
// (NewMap, OpenMap, SaveMap, SaveMapAs, Map_LoadFile, Map_New, Map_Free,
// Map_Unnamed), radiant/qe3.cpp (ConfirmModified, Sys_SetTitle),
// radiant/mru.cpp (ueber bhed/karte/zuletzt.h), libs/maplib.h
// (UndoFileChangeTracker, ueber Rueckgaengig::gespeichert).
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
#include "karteneditor.h"

#include "app_internal.h"
#include "bhed/karte/texdef.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <utility>

namespace bhed::gui::karteneditor {

namespace {

namespace fs = std::filesystem;

Editor& g() { return editor(); }

// UTF-8 -> Pfad ohne das in C++20 veraltete u8path (wie src/karte).
fs::path utf8Pfad(const std::string& p) {
    return fs::path(std::u8string(reinterpret_cast<const char8_t*>(p.data()), p.size()));
}

// "Map (*.map)" wie NRCs MapFormat-Dateitypen fuer Quake 3 (mapq3:
// "quake3 maps" *.map, "quake3 region" *.reg).
const char* const kKartenFilter = "Map (*.map)|*.map|Region (*.reg)|*.reg|*.*|*.*";

std::string spielName(int spiel) {
    return spiel == kRegelwerkMovieDuels    ? "Movie Duels (ja + MD)"
           : spiel == kRegelwerkJediOutcast ? "Jedi Outcast (jk2)"
                                            : "OpenJK (ja)";
}

// Die Suchpfade des Spiels, Mod zuerst wie QE_InitVFS (qe3.cpp): fs_game vor
// basegame. Movie Duels: die Movie-Duels-Ordner vor den uebrigen; OpenJK:
// ohne sie; Jedi Outcast: die eigenen JK2-Ordner.
std::vector<std::string> suchpfade(int spiel) {
    std::vector<std::string> mod;
    std::vector<std::string> basis;
    for (const std::string& o : eingerichteteSpielordner()) {
        if (movieDuelsOrdner(o)) {
            if (spiel == kRegelwerkMovieDuels) { mod.push_back(o); }
        } else {
            basis.push_back(o);
        }
    }
    mod.insert(mod.end(), basis.begin(), basis.end());
    return mod;
}

// data/karte/<spiel> neben data/base (wie data/jk2, eingebaut.cpp packt die
// eingebauten .def dorthin aus).
std::string defOrdner(int spiel) {
    return directoryOf(g_app->dataDir) + (spiel == kRegelwerkJediOutcast ? "/karte/jk2" : "/karte/ja");
}

std::shared_ptr<Spielkontext> spielLadenJetzt(int spiel, std::vector<std::string> ordner, std::string defs) {
    const auto t0 = std::chrono::steady_clock::now();
    auto k = std::make_shared<Spielkontext>();
    k->spiel = spiel;
    k->name = spielName(spiel);
    k->ordner = std::move(ordner);
    for (const std::string& o : k->ordner) { k->fs.ordnerHinzufuegen(o, "sp"); }
    // Shaders_Load: shaderlist.txt aus den Archiven, sonst alle Shader
    // ("No shaderlist.txt found: loading all shaders"). NRCs
    // default_shaderlist.txt stammt aus dem Gamepack und kommt nicht mit.
    k->inventar.laden(k->fs);
    const std::size_t fehlerDef = k->klassen.ordnerLaden(defs, "sp");
    if (spiel == kRegelwerkMovieDuels) { (void)k->klassen.entdefsErgaenzen(); }
    k->meldungen = k->inventar.meldungen();
    if (fehlerDef > 0) { k->meldungen.push_back(defs + ": " + std::to_string(fehlerDef) + " Fehler in den .def"); }
    if (k->klassen.anzahl() == 0) { k->meldungen.push_back("keine Entitaetsklassen in " + defs); }
    k->sekunden = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    return k;
}

void spielUebernehmen(std::shared_ptr<Spielkontext> k) {
    g().spiel = std::move(k);
    const Spielkontext& s = *g().spiel;
    for (const std::string& m : s.meldungen) { warnung(m); }
    char z[300];
    std::snprintf(z, sizeof(z), tr(Str::KeGameLoaded), s.name.c_str(), static_cast<int>(s.inventar.shader().size()),
                  static_cast<int>(s.klassen.anzahl()), s.sekunden);
    ausgabe(z);
    if (s.ordner.empty()) {
        std::snprintf(z, sizeof(z), tr(Str::KeGameNoFolders), s.name.c_str());
        warnung(z);
    }
    // Die leere Karte des Starts bekommt das Spiel, sobald es da ist.
    if (!g().karteSpiel && !geaendert()) {
        g().karteSpiel = g().spiel;
        g().karte.klassen = &g().karteSpiel->klassen;
        g().karte.kontext.texturen = &g().karteSpiel->inventar;
    }
}

void zuletztLaden() {
    if (g().zuletztGeladen) { return; }
    g().zuletztGeladen = true;
    g().zuletzt.setzen(g_app->settings.karteZuletzt);
}

void zuletztMerken() { g_app->settings.karteZuletzt = g().zuletzt.pfade(); }

// MRU_AddFile
void zuletztHinzu(const std::string& pfad) {
    zuletztLaden();
    g().zuletzt.hinzufuegen(pfad);
    zuletztMerken();
}

// Map_Free + die Karte leeren; das Spiel der neuen Karte ist das aktuelle.
void karteFreigeben() {
    Editor& e = g();
    e.karte.auswahl.leeren();
    e.karte.leeren();
    e.karte.undo.clear();
    e.karteSpiel = spielWarten();
    e.karte.klassen = e.karteSpiel ? &e.karteSpiel->klassen : nullptr;
    e.karte.kontext.texturen = e.karteSpiel ? &e.karteSpiel->inventar : nullptr;
    e.karte.kontext.format = karte::PinselFormat::Quake3;
}

// ConfirmModified (qe3.cpp:128-150) als Klappfenster: Save / Discard / Cancel.
void mitRueckfrage(const char* titel, std::function<void()> danach) {
    if (!geaendert()) {
        danach();
        return;
    }
    g().frageOffen = true;
    g().frageTitel = titel;
    g().nachFrage = std::move(danach);
}

void zeichneRueckfrage() {
    Editor& e = g();
    if (!e.frageOffen) { return; }
    const std::string id = e.frageTitel + "###kartefrage";
    if (!ImGui::IsPopupOpen(id.c_str())) { ImGui::OpenPopup(id.c_str()); }
    if (ImGui::BeginPopupModal(id.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextUnformatted(tr(Str::KeConfirmModified));
        ImGui::Spacing();
        int wahl = -1;
        if (ImGui::Button(tr(Str::KeSave))) { wahl = 0; }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::KeDiscard))) { wahl = 1; }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::KeCancel)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { wahl = 2; }
        if (wahl >= 0) {
            e.frageOffen = false;
            ImGui::CloseCurrentPopup();
            std::function<void()> danach = std::move(e.nachFrage);
            e.nachFrage = nullptr;
            bool weiter = wahl == 1;
            if (wahl == 0) {
                // Map_SaveAs bei einer unbenannten Karte, sonst Map_Save.
                weiter = e.name == "unnamed.map" ? karteSpeichernUnterDialog() : karteSpeichern();
            }
            if (weiter && danach) { danach(); }
        }
        ImGui::EndPopup();
    }
}

std::string startOrdner() {
    // map_open: bei einer unbenannten Karte der maps-Ordner, sonst die Karte.
    if (g().name != "unnamed.map") { return directoryOf(g().name); }
    return g_app->settings.karte.count("mapsPath") != 0U ? g_app->settings.karte.at("mapsPath") : std::string();
}

}  // namespace

Editor& editor() {
    static Editor e;
    static bool angelegt = false;
    if (!angelegt) {
        angelegt = true;
        // Die Konsole bekommt die Undo-Meldungen (undo.cpp:292-333).
        e.karte.undo.ausgabe = [](const std::string& s) { ausgabe(s); };
        e.karte.undo.alsGespeichertMarkieren();
    }
    return e;
}

bool aktiv() { return g_app != nullptr && g_app->leftMode == 3; }

bool nrcBegriffe() {
    const auto it = g_app->settings.karte.find("nrcBegriffe");
    return it != g_app->settings.karte.end() && it->second == "1";
}

void setzeNrcBegriffe(bool an) { g_app->settings.karte["nrcBegriffe"] = an ? "1" : "0"; }

const char* trK(i18n::Str id) { return nrcBegriffe() ? i18n::trIn(i18n::Language::English, id) : tr(id); }

bool geaendert() { return !g().karte.undo.gespeichert(); }

void ausgabe(const std::string& text) {
    g().konsole.push_back({KonsolenZeile::Art::Ausgabe, text});
    diag::detail("Karten-Editor: " + text);
}
void warnung(const std::string& text) {
    g().konsole.push_back({KonsolenZeile::Art::Warnung, text});
    diag::detail("Karten-Editor Warnung: " + text);
}
void fehler(const std::string& text) {
    g().konsole.push_back({KonsolenZeile::Art::Fehler, text});
    diag::detail("Karten-Editor Fehler: " + text);
}

void spielPruefen() {
    Editor& e = g();
    if (e.spielLaden.valid() &&
        e.spielLaden.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        spielUebernehmen(e.spielLaden.get());
    }
    const int spiel = regelwerkSpielErmitteln();
    if (spiel == e.spielStand) { return; }
    if (e.spielLaden.valid()) { return; }   // erst das laufende fertig laden
    e.spielStand = spiel;
    // NRC default_scale (ja.game, jk2.game): 0.25 statt des Grundwerts 0.5
    // (Texdef_getDefaultTextureScale, brush_primit.cpp).
    karte::setzeStandardTexturSkalierung(0.25F);
    char z[200];
    std::snprintf(z, sizeof(z), tr(Str::KeGameLoading), spielName(spiel).c_str());
    ausgabe(z);
    e.spielLaden = std::async(std::launch::async, spielLadenJetzt, spiel, suchpfade(spiel), defOrdner(spiel));
}

bool spielBereit() { return g().spiel != nullptr && !g().spielLaden.valid(); }

const std::shared_ptr<Spielkontext>& spielWarten() {
    Editor& e = g();
    if (e.spielStand < 0) { spielPruefen(); }
    if (e.spielLaden.valid()) { spielUebernehmen(e.spielLaden.get()); }
    return e.spiel;
}

// --- Datei -------------------------------------------------------------------

void neueKarte() {
    mitRueckfrage("New Map", [] {
        // Map_Free + Map_New (map.cpp:1264)
        karteFreigeben();
        g().name = "unnamed.map";
        g().karte.undo.alsGespeichertMarkieren();
    });
}

bool karteOeffnen(const std::string& pfad) {
    Editor& e = g();
    karteFreigeben();
    e.name = pfad;
    // Map_LoadFile (map.cpp:1001-1026)
    ausgabe("Loading map from " + pfad);
    const auto t0 = std::chrono::steady_clock::now();
    karte::LeseErgebnis ergebnis;
    const bool ok = karte::karteLaden(pfad, e.karte, ergebnis);
    for (const std::string& m : ergebnis.meldungen) {
        if (m.find("rror") != std::string::npos) { fehler(m); } else { warnung(m); }
    }
    e.karte.anmelden();
    e.karte.undo.clear();
    e.karte.undo.alsGespeichertMarkieren();
    std::size_t primitive = 0;
    for (const auto& ent : e.karte.entitaeten()) { primitive += ent->kinder().size(); }
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    char z[160];
    std::snprintf(z, sizeof(z), "map load: %.0f ms", ms);   // ScopeTimer timer( "map load" )
    ausgabe(z);
    ausgabe("--- LoadMapFile ---");
    ausgabe(e.name);
    ausgabe(std::to_string(primitive) + " primitives");
    ausgabe(std::to_string(e.karte.entitaeten().size()) + " entities");
    if (!ok) {
        std::snprintf(z, sizeof(z), tr(Str::KeMapLoadFailed), pfad.c_str());
        fehler(z);
    }
    return ok;
}

void karteOeffnenDialog() {
    mitRueckfrage("Open Map", [] {
        const std::string pfad = platform::openFileDialog("Open Map", kKartenFilter, startOrdner());
        if (pfad.empty()) { return; }
        zuletztHinzu(pfad);   // MRU_AddFile vor dem Laden (map.cpp:1848)
        (void)karteOeffnen(pfad);
    });
}

bool karteSpeichernUnter(const std::string& pfad) {
    Editor& e = g();
    zuletztHinzu(pfad);   // Map_SaveAs: MRU_AddFile, Map_Rename, Map_Save
    e.name = pfad;
    std::string grund;
    // MapResource_save: mit Sicherung der alten Datei als .bak.
    if (!karte::karteSpeichern(pfad, e.karte, true, &grund)) {
        char z[400];
        std::snprintf(z, sizeof(z), tr(Str::KeMapSaveFailed), pfad.c_str(), grund.c_str());
        fehler(z);
        return false;
    }
    e.karte.undo.alsGespeichertMarkieren();
    ausgabe("Saved " + pfad);
    return true;
}

bool karteSpeichernUnterDialog() {
    const std::string pfad = platform::saveFileDialog("Save Map", kKartenFilter, startOrdner(), fileName(g().name));
    if (pfad.empty()) { return false; }
    return karteSpeichernUnter(pfad);
}

bool karteSpeichern() {
    Editor& e = g();
    // SaveMap (map.cpp:1879-1887): unbenannt -> Speichern unter; sonst nur,
    // wenn geaendert, und erst dann in die zuletzt-Liste.
    if (e.name == "unnamed.map") { return karteSpeichernUnterDialog(); }
    if (!geaendert()) { return true; }
    std::string grund;
    if (!karte::karteSpeichern(e.name, e.karte, true, &grund)) {
        char z[400];
        std::snprintf(z, sizeof(z), tr(Str::KeMapSaveFailed), e.name.c_str(), grund.c_str());
        fehler(z);
        return false;
    }
    e.karte.undo.alsGespeichertMarkieren();
    zuletztHinzu(e.name);
    return true;
}

void zuletztOeffnen(std::size_t index) {
    mitRueckfrage("Open Map", [index] {
        zuletztLaden();
        const std::string pfad = index < g().zuletzt.anzahl() ? g().zuletzt.pfade()[index] : std::string();
        std::error_code ec;
        const bool lesbar = !pfad.empty() && fs::is_regular_file(utf8Pfad(pfad), ec);
        const std::string laden = g().zuletzt.aktivieren(index, lesbar);
        zuletztMerken();
        if (!laden.empty()) { (void)karteOeffnen(laden); }
    });
}

// --- Rahmen ------------------------------------------------------------------

namespace {

// Das File-Menue (create_file_menu, mainframe.cpp:866-890) - bis die
// Befehlstabelle steht (Punkt 4) direkt verdrahtet.
void zeichneDateiMenue() {
    if (!ImGui::BeginMenu(trK(Str::KeMenuFile))) { return; }
    if (ImGui::MenuItem(trK(Str::KeCmdNewMap))) { neueKarte(); }
    ImGui::Separator();
    if (ImGui::MenuItem(trK(Str::KeCmdOpenMap), "Ctrl+O")) { karteOeffnenDialog(); }
    ImGui::Separator();
    if (ImGui::MenuItem(trK(Str::KeCmdSaveMap), "Ctrl+S")) { (void)karteSpeichern(); }
    if (ImGui::MenuItem(trK(Str::KeCmdSaveMapAs))) { (void)karteSpeichernUnterDialog(); }
    ImGui::Separator();
    // MRU_constructMenu (mru.cpp:186-236)
    zuletztLaden();
    const karte::ZuletztListe& z = g().zuletzt;
    for (std::size_t i = 0; i < karte::ZuletztListe::kMax; ++i) {
        if (i > 0 && i >= z.anzahl()) { break; }
        const std::string roh = karte::ohneMnemonik(z.beschriftung(i));
        const std::string text = (roh == "Recent Files" ? std::string(trK(Str::KeRecentFiles)) : roh) + "###karteMru" +
                                 std::to_string(i);
        if (ImGui::MenuItem(text.c_str(), nullptr, false, i < z.anzahl())) { zuletztOeffnen(i); }
    }
    ImGui::EndMenu();
}

}  // namespace

void zeichne(float hoehe) {
    Editor& e = g();
    e.gezeichnet = true;
    spielPruefen();
    ImGui::BeginChild("##karteneditor", ImVec2{-FLT_MIN, hoehe}, ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (ImGui::BeginMenuBar()) {
        zeichneDateiMenue();
        ImGui::EndMenuBar();
    }
    // Sys_SetTitle: "<name> *" - der Titel des Arbeitsbereichs.
    ImGui::TextUnformatted((e.name + (geaendert() ? " *" : "")).c_str());
    if (!spielBereit()) {
        char z[200];
        std::snprintf(z, sizeof(z), tr(Str::KeGameLoading), spielName(regelwerkSpielErmitteln()).c_str());
        ImGui::TextDisabled("%s", z);
    }
    for (std::size_t i = e.konsole.size() > 20 ? e.konsole.size() - 20 : 0; i < e.konsole.size(); ++i) {
        ImGui::TextUnformatted(e.konsole[i].text.c_str());
    }
    ImGui::EndChild();
    zeichneRueckfrage();
}

}  // namespace bhed::gui::karteneditor
