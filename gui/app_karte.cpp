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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <functional>
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

// --- Rahmen (MainFrame::Create, mainframe.cpp:1667-1883) ------------------------
//
// Vier Fensteranordnungen wie NRC (g_Layout_viewStyle):
//   Regular       links XY ueber der Konsole, rechts die Kamera ueber dem
//                 Texturbrowser (QSplitter m_hSplit, m_vSplit, m_vSplit2)
//   Regular Left  dasselbe mit getauschten Spalten
//   Split         vier Felder: links Kamera ueber YZ, rechts XY ueber XZ;
//                 Konsole und Texturen im Gruppendialog
//   Floating      Camera, XY, XZ, YZ und der Gruppendialog als eigene Fenster
// Floating: ImGui dieser Fassung (1.93 WIP, ohne Docking/Viewports) kann keine
// eigenen Desktop-Fenster oeffnen - die Fenster schweben im Programmfenster.

namespace {

enum class Feld { XY, XZ, YZ, Kamera, Konsole, Texturen, Gruppe };

// QSplitter-Griff (PM_SplitterWidth im Fusion-Stil).
constexpr float kGriff = 5.0F;

ImVec4 farbe(const Farben::F3& c) { return ImVec4{c[0], c[1], c[2], 1.0F}; }

const char* feldName(Feld f) {
    switch (f) {
    case Feld::XY: return "##feldXY";
    case Feld::XZ: return "##feldXZ";
    case Feld::YZ: return "##feldYZ";
    case Feld::Kamera: return "##feldKamera";
    case Feld::Konsole: return "##feldKonsole";
    case Feld::Texturen: return "##feldTexturen";
    case Feld::Gruppe: return "##feldGruppe";
    }
    return "##feld";
}

// radiant/console.cpp: Ausgabe in der Textfarbe, Warnungen (255,127,0),
// Fehler (255,0,0) - in einem schreibgeschuetzten Textfeld.
void konsoleZeichnen() {
    Editor& ed = editor();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::BeginChild("##konsoleText", ImVec2{-FLT_MIN, -FLT_MIN}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar);
    static std::size_t gesehen = 0;
    ImGuiListClipper clip;
    clip.Begin(static_cast<int>(ed.konsole.size()));
    while (clip.Step()) {
        for (int i = clip.DisplayStart; i < clip.DisplayEnd; ++i) {
            const KonsolenZeile& z = ed.konsole[static_cast<std::size_t>(i)];
            if (z.art == KonsolenZeile::Art::Ausgabe) {
                ImGui::TextUnformatted(z.text.c_str());
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, z.art == KonsolenZeile::Art::Warnung ? ImVec4{1.0F, 0.498F, 0.0F, 1.0F}
                                                                                         : ImVec4{1.0F, 0.0F, 0.0F, 1.0F});
                ImGui::TextUnformatted(z.text.c_str());
                ImGui::PopStyleColor();
            }
        }
    }
    // Neue Zeilen: ans Ende rollen (Sys_Print: moveCursor End).
    if (gesehen != ed.konsole.size()) {
        gesehen = ed.konsole.size();
        ImGui::SetScrollHereY(1.0F);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// Der Inhalt eines Feldes. Die Ansichten selbst kommen mit den Punkten 5
// (XYWnd), 6 (CamWnd) und 8 (Texturbrowser); bis dahin der Hintergrund in der
// NRC-Farbe und der Name.
void feldInhalt(Feld f) {
    Editor& ed = editor();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = ImGui::GetWindowPos();
    const ImVec2 b{a.x + ImGui::GetWindowWidth(), a.y + ImGui::GetWindowHeight()};
    switch (f) {
    case Feld::XY:
    case Feld::XZ:
    case Feld::YZ: {
        dl->AddRectFilled(a, b, ImGui::GetColorU32(farbe(ed.farben.rasterHinten)));
        const char* titel = f == Feld::XY ? trK(Str::KeViewXY) : f == Feld::XZ ? trK(Str::KeViewXZ) : trK(Str::KeViewYZ);
        dl->AddText(ImVec2{a.x + 4.0F, a.y + 4.0F}, ImGui::GetColorU32(farbe(ed.farben.ansichtName)), titel);
        break;
    }
    case Feld::Kamera:
        dl->AddRectFilled(a, b, ImGui::GetColorU32(farbe(ed.farben.kameraHinten)));
        break;
    case Feld::Konsole:
        konsoleZeichnen();
        break;
    case Feld::Texturen:
        dl->AddRectFilled(a, b, ImGui::GetColorU32(farbe(ed.farben.texBro)));
        break;
    case Feld::Gruppe:
        break;
    }
}

void feld(Feld f, ImVec2 groesse) {
    Editor& ed = editor();
    ImGui::BeginChild(feldName(f), groesse, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    // MaximizeView merkt sich das Feld unter der Maus.
    if (!ed.maximiert && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
        ed.maximiertFeld = static_cast<int>(f);
    }
    feldInhalt(f);
    ImGui::EndChild();
}

// Ein QSplitter mit zwei Feldern; anteil ist die Groesse des ersten.
void teiler(const char* id, ImVec2 groesse, bool senkrecht, float& anteil, const std::function<void(ImVec2)>& erstes,
            const std::function<void(ImVec2)>& zweites) {
    const float gesamt = senkrecht ? groesse.y : groesse.x;
    const float eins = std::floor(std::clamp(anteil, 0.0F, 1.0F) * (gesamt - kGriff));
    const ImVec2 start = ImGui::GetCursorScreenPos();
    erstes(senkrecht ? ImVec2{groesse.x, eins} : ImVec2{eins, groesse.y});
    const ImVec2 griffA = senkrecht ? ImVec2{start.x, start.y + eins} : ImVec2{start.x + eins, start.y};
    ImGui::SetCursorScreenPos(griffA);
    ImGui::InvisibleButton(id, senkrecht ? ImVec2{groesse.x, kGriff} : ImVec2{kGriff, groesse.y});
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
        ImGui::SetMouseCursor(senkrecht ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
    }
    if (ImGui::IsItemActive() && gesamt > kGriff) {
        const float d = senkrecht ? ImGui::GetIO().MouseDelta.y : ImGui::GetIO().MouseDelta.x;
        anteil = std::clamp((eins + d) / (gesamt - kGriff), 0.0F, 1.0F);
    }
    if (ImGui::IsItemDeactivated()) { einstellungenSpeichern(); }
    ImGui::SetCursorScreenPos(senkrecht ? ImVec2{start.x, start.y + eins + kGriff} : ImVec2{start.x + eins + kGriff, start.y});
    zweites(senkrecht ? ImVec2{groesse.x, gesamt - eins - kGriff} : ImVec2{gesamt - eins - kGriff, groesse.y});
}

void anordnungZeichnen(ImVec2 groesse) {
    Editor& ed = editor();
    using befehle::Anordnung;
    if (ed.maximiert) {
        feld(static_cast<Feld>(ed.maximiertFeld), groesse);
        return;
    }
    if (ed.anordnung == Anordnung::Regular || ed.anordnung == Anordnung::RegularLeft) {
        const auto links = [&](ImVec2 g) {   // m_vSplit: XY, Konsole
            ImGui::BeginChild("##vSplit", g, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
            teiler("##vgriff", g, true, ed.vSplit, [](ImVec2 g1) { feld(Feld::XY, g1); },
                   [](ImVec2 g2) { feld(Feld::Konsole, g2); });
            ImGui::EndChild();
        };
        const auto rechts = [&](ImVec2 g) {   // m_vSplit2: Kamera, Texturen
            ImGui::BeginChild("##vSplit2", g, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
            if (ed.gruppenDialogEingebaut) {
                feld(Feld::Kamera, g);
            } else {
                teiler("##v2griff", g, true, ed.vSplit2, [](ImVec2 g1) { feld(Feld::Kamera, g1); },
                       [](ImVec2 g2) { feld(Feld::Texturen, g2); });
            }
            ImGui::EndChild();
        };
        if (ed.anordnung == Anordnung::Regular) {
            teiler("##hgriff", groesse, false, ed.hSplit, links, rechts);
        } else {
            teiler("##hgriff", groesse, false, ed.hSplit, rechts, links);
        }
    } else if (ed.anordnung == Anordnung::Split) {
        // 4 way (mainframe.cpp:1838-1863): links Kamera ueber YZ, rechts XY ueber XZ
        teiler("##hgriff", groesse, false, ed.hSplit,
               [&](ImVec2 g) {
                   ImGui::BeginChild("##vSplit", g, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
                   teiler("##vgriff", g, true, ed.vSplit2, [](ImVec2 g1) { feld(Feld::Kamera, g1); },
                          [](ImVec2 g2) { feld(Feld::YZ, g2); });
                   ImGui::EndChild();
               },
               [&](ImVec2 g) {
                   ImGui::BeginChild("##vSplit2", g, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
                   teiler("##v2griff", g, true, ed.vSplit2, [](ImVec2 g1) { feld(Feld::XY, g1); },
                          [](ImVec2 g2) { feld(Feld::XZ, g2); });
                   ImGui::EndChild();
               });
    } else {
        // Floating: der Hauptbereich bleibt leer, die Ansichten schweben.
        ImGui::Dummy(groesse);
    }
}

// Ein schwebendes Fenster (Floating: g_guiSettings.addWindow( ..., w, h, x, y )).
void schwebend(Feld f, const char* titel, bool& sichtbar, ImVec2 groesse, ImVec2 lage, ImVec2 ursprung) {
    if (!sichtbar) { return; }
    ImGui::SetNextWindowPos(ImVec2{ursprung.x + lage.x, ursprung.y + lage.y}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(groesse, ImGuiCond_FirstUseEver);
    const std::string id = std::string(titel) + "###schwebend" + feldName(f);
    if (ImGui::Begin(id.c_str(), &sichtbar, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse)) {
        feldInhalt(f);
    }
    ImGui::End();
}

// Der Gruppendialog (groupdialog.cpp, Fenstertitel "Entities"): Reiter
// Entities, (Console, Textures bei Floating/Split), Models, Layers.
void gruppenDialog(ImVec2 ursprung, ImVec2 bereich) {
    Editor& ed = editor();
    using befehle::Anordnung;
    const bool schwebendeSeiten = ed.anordnung == Anordnung::Floating || ed.anordnung == Anordnung::Split;
    if (!ed.gruppenDialogOffen) { return; }
    ImGui::SetNextWindowPos(ImVec2{ursprung.x + bereich.x * 0.55F, ursprung.y + bereich.y * 0.1F}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2{bereich.x * 0.35F, bereich.y * 0.75F}, ImGuiCond_FirstUseEver);
    const char* seitenTitel[5] = {trK(Str::KePageEntities), trK(Str::KePageConsole), trK(Str::KePageTextures),
                                  trK(Str::KePageModels), trK(Str::KePageLayers)};
    const std::string titel = std::string(seitenTitel[ed.gruppenSeite]) + "###kartegruppe";
    if (ImGui::Begin(titel.c_str(), &ed.gruppenDialogOffen, ImGuiWindowFlags_NoCollapse)) {
        if (ImGui::BeginTabBar("##gruppenreiter")) {
            for (int s = 0; s < 5; ++s) {
                if ((s == 1 || s == 2) && !schwebendeSeiten && !(s == 2 && ed.gruppenDialogEingebaut)) { continue; }
                ImGuiTabItemFlags fl = ImGuiTabItemFlags_None;
                if (ed.gruppenSeiteWechseln && ed.gruppenSeite == s) { fl |= ImGuiTabItemFlags_SetSelected; }
                if (ImGui::BeginTabItem(seitenTitel[s], nullptr, fl)) {
                    if (!ed.gruppenSeiteWechseln) { ed.gruppenSeite = s; }
                    if (s == 1) {
                        konsoleZeichnen();
                    } else if (s == 2) {
                        ImGui::BeginChild("##gruppeTexturen", ImVec2{-FLT_MIN, -FLT_MIN});
                        feldInhalt(Feld::Texturen);
                        ImGui::EndChild();
                    }
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
        ed.gruppenSeiteWechseln = false;
    }
    ImGui::End();
}

}  // namespace

void zeichne(float hoehe) {
    Editor& e = g();
    e.gezeichnet = true;
    e.knoepfeImBild.clear();
    spielPruefen();
    if (!e.belegungGeladen) { einstellungenLaden(); }
    farbenAusThema();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    ImGui::BeginChild("##karteneditor", ImVec2{-FLT_MIN, hoehe}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(2);
    zeichneMenueleiste();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    zeichneHauptleiste();
    const ImVec2 rest = ImGui::GetContentRegionAvail();
    const float statusH = statuszeilenHoehe();
    const float flaecheH = std::max(10.0F, rest.y - statusH);
    const ImVec2 start = ImGui::GetCursorScreenPos();
    // Filterleiste rechts angedockt (RightToolBarArea), daneben die Felder.
    const float filterB = e.filterleiste ? std::round(ImGui::GetFontSize() * 2.0F) + 10.0F : 0.0F;
    anordnungZeichnen(ImVec2{std::max(10.0F, rest.x - filterB), flaecheH});
    ImGui::SetCursorScreenPos(ImVec2{start.x + rest.x - filterB, start.y});
    (void)zeichneFilterleiste(flaecheH);
    ImGui::SetCursorScreenPos(ImVec2{start.x, start.y + flaecheH});
    zeichneStatuszeile();
    ImGui::PopStyleVar();
    ImGui::EndChild();
    // Schwebende Fenster (Floating) und der Gruppendialog.
    if (e.anordnung == befehle::Anordnung::Floating) {
        schwebend(Feld::Kamera, trK(Str::KeViewCamera), e.schwebendSichtbar[3], ImVec2{400, 300}, ImVec2{50, 100}, start);
        schwebend(Feld::XY, trK(Str::KeViewXY), e.schwebendSichtbar[2], ImVec2{400, 300}, ImVec2{500, 100}, start);
        schwebend(Feld::XZ, trK(Str::KeViewXZ), e.schwebendSichtbar[1], ImVec2{400, 300}, ImVec2{500, 450}, start);
        schwebend(Feld::YZ, trK(Str::KeViewYZ), e.schwebendSichtbar[0], ImVec2{400, 300}, ImVec2{50, 450}, start);
    }
    gruppenDialog(start, rest);
    zeichneEinstellungen();
    zeichneTastenDialog();
    zeichneRueckfrage();
    tastenVerarbeiten();
}
}  // namespace bhed::gui::karteneditor
