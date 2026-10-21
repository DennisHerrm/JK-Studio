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
    e.bearbeitung.neueKarte();
    for (auto& b : e.beobachter) { b = FensterBeobachter{b.breite, b.hoehe}; }
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
        // MainFrame::Create: m_pXYWnd XY, m_pXZWnd XZ, m_pYZWnd YZ
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[1].ansicht.typ = karte::Sicht::XZ;
        e.xy[2].ansicht.typ = karte::Sicht::YZ;
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
        ++g().ladeZaehler;
        startPosition();   // Map_New: FocusViews( g_vector3_identity, 0 ) ueber Map_StartPosition
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
    ++e.ladeZaehler;
    startPosition();   // Map_LoadFile: Map_StartPosition
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
// Floating: ImGui dieser Fassung (1.92, ohne Docking/Viewports) kann keine
// eigenen Desktop-Fenster oeffnen - die Fenster schweben im Programmfenster.
//
// Groessen: alles in logischen Qt-Bildpunkten (mass::), verteilt mit Qts
// eigener Rechnung (bhed/karte/teiler.h), mal massstab(). Am Foto von NRC
// 20260114 (maximiert, 2560 x 1600 bei 150 % = Qt-Massstab 2) gemessen:
// Menue 21, Werkzeugleiste 39, Filterleiste 39 breit, Statuszeile 21,
// Griffe 4; Regular XY 495 x 629, Konsole 33, Kamera 742 x 414,
// Texturbrowser 248.

float massstabErsatz = 0.0F;
int bereichErsatzB = 0;
int bereichErsatzH = 0;

namespace {

ImFont* g_schrift = nullptr;
ImVec2 g_ursprung{0.0F, 0.0F};   // linke obere Ecke des Arbeitsbereichs (Bildpunkte)
float g_s = 1.0F;                // massstab() dieses Bildes

}  // namespace

void setzeSchrift(ImFont* schrift) { g_schrift = schrift; }
ImFont* schrift() { return g_schrift; }

float massstab() {
    if (massstabErsatz > 0.0F) { return massstabErsatz; }
    // QHighDpiScaling, Rundung Round (Qt-5.15-Vorgabe; NRC main.cpp:410-411
    // laesst PassThrough auskommentiert): qRound(1,5) = 2.
    const float qt = std::max(1.0F, std::round(g_app->dpiScale));
    return qt * g_app->uiScale;
}

ImVec2 bildpunkt(int x, int y) {
    return ImVec2{g_ursprung.x + std::round(static_cast<float>(x) * g_s),
                  g_ursprung.y + std::round(static_cast<float>(y) * g_s)};
}

void merkeFeld(const std::string& name, int x, int y, int b, int h) { g().felderImBild.push_back({name, x, y, b, h}); }

namespace {

enum class Feld { XY, XZ, YZ, Kamera, Konsole, Texturen, Gruppe };

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

const char* feldKurz(Feld f) {
    switch (f) {
    case Feld::XY: return "XY";
    case Feld::XZ: return "XZ";
    case Feld::YZ: return "YZ";
    case Feld::Kamera: return "Kamera";
    case Feld::Konsole: return "Konsole";
    case Feld::Texturen: return "Texturen";
    case Feld::Gruppe: return "Gruppe";
    }
    return "?";
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

// Der Inhalt eines Feldes: die Ansichten (XYWnd, CamWnd), die Konsole und
// der Texturbrowser.
void feldInhalt(Feld f) {
    switch (f) {
    case Feld::XY:
        xyZeichnen(0);
        break;
    case Feld::XZ:
        xyZeichnen(1);
        break;
    case Feld::YZ:
        xyZeichnen(2);
        break;
    case Feld::Kamera:
        kameraZeichnen();
        break;
    case Feld::Konsole:
        konsoleZeichnen();
        break;
    case Feld::Texturen:   // TextureBrowser_constructWindow
        texturBrowserZeichnen(static_cast<int>(std::lround(ImGui::GetWindowWidth() / g_s)),
                              static_cast<int>(std::lround(ImGui::GetWindowHeight() / g_s)));
        break;
    case Feld::Gruppe:
        break;
    }
}

// Ein Feld an seiner Stelle (logische Punkte).
void feld(Feld f, int x, int y, int b, int h) {
    merkeFeld(feldKurz(f), x, y, b, h);
    if (b <= 0 || h <= 0) { return; }   // eingeklappt oder 0 breit (setGeo)
    const ImVec2 p0 = bildpunkt(x, y);
    const ImVec2 p1 = bildpunkt(x + b, y + h);
    ImGui::SetCursorScreenPos(p0);
    ImGui::BeginChild(feldName(f), ImVec2{p1.x - p0.x, p1.y - p0.y}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    feldInhalt(f);
    ImGui::EndChild();
}

// Was ein Kind eines Teilers zeichnet: ein Feld oder ein innerer Teiler.
using Inhalt = std::function<void(int x, int y, int b, int h)>;

// Ein QSplitter: ordnet seine Kinder auf das Rechteck an (doResize), zeichnet
// sie und die Griffe; Ziehen wie QSplitterHandle::mouseMoveEvent (Lage der
// Maus minus der Stelle, an der der Griff gepackt wurde -> moveSplitter).
void teilerZeichnen(const char* id, karte::Teiler& t, bool senkrecht, int x, int y, int b, int h,
                    const std::vector<Inhalt>& kinder) {
    t.griff = mass::kTeilerGriff;
    t.anordnen(0, senkrecht ? h : b);
    static int packStelle = 0;   // d->mouseOffset
    for (std::size_t i = 0; i < t.kinder.size() && i < kinder.size(); ++i) {
        const karte::Teiler::Kind& k = t.kinder[i];
        if (k.versteckt) { continue; }
        if (!t.griffVersteckt(static_cast<int>(i))) {
            const int gpos = t.griffPos(static_cast<int>(i));
            const int gx = senkrecht ? x : x + gpos;
            const int gy = senkrecht ? y + gpos : y;
            const int gb = senkrecht ? b : t.griff;
            const int gh = senkrecht ? t.griff : h;
            merkeFeld(std::string("Griff:") + id + ":" + std::to_string(i), gx, gy, gb, gh);
            const ImVec2 p0 = bildpunkt(gx, gy);
            const ImVec2 p1 = bildpunkt(gx + gb, gy + gh);
            ImGui::SetCursorScreenPos(p0);
            const std::string kennung = std::string("##griff") + id + std::to_string(i);
            ImGui::InvisibleButton(kennung.c_str(), ImVec2{std::max(1.0F, p1.x - p0.x), std::max(1.0F, p1.y - p0.y)});
            const bool aktivGriff = ImGui::IsItemActive();
            if (ImGui::IsItemHovered() || aktivGriff) {
                ImGui::SetMouseCursor(senkrecht ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
            }
            // Fusion zeichnet den Griff in der Fensterfarbe mit Punkten in der
            // Mitte (QFusionStyle CE_Splitter); hier in den Farben des Programms.
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 m{std::floor((p0.x + p1.x) * 0.5F), std::floor((p0.y + p1.y) * 0.5F)};
            const ImU32 punkt = ImGui::GetColorU32(aktivGriff ? ImGuiCol_SeparatorActive : ImGuiCol_Separator);
            const float d = std::max(1.0F, std::round(0.5F * g_s));
            for (int k2 = -3; k2 <= 3; ++k2) {
                const float o = static_cast<float>(k2) * 3.0F * d;
                const ImVec2 q = senkrecht ? ImVec2{m.x + o, m.y} : ImVec2{m.x, m.y + o};
                dl->AddRectFilled(ImVec2{q.x - d, q.y - d}, ImVec2{q.x + d, q.y + d}, punkt);
            }
            const float maus = senkrecht ? ImGui::GetIO().MousePos.y : ImGui::GetIO().MousePos.x;
            const float anfang = senkrecht ? bildpunkt(0, y).y : bildpunkt(x, 0).x;
            const int mausLog = static_cast<int>(std::floor((maus - anfang) / g_s));
            if (ImGui::IsItemActivated()) { packStelle = mausLog - gpos; }
            if (aktivGriff && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0F)) {
                t.ziehen(mausLog - packStelle, static_cast<int>(i));
            }
            if (ImGui::IsItemDeactivated()) {
                Editor& ed = editor();
                if (ed.anordnung == befehle::Anordnung::Regular || ed.anordnung == befehle::Anordnung::RegularLeft) {
                    ed.hGroessen = ed.hSplit.groessen();
                    ed.vGroessen = ed.vSplit.groessen();
                    ed.v2Groessen = ed.vSplit2.groessen();
                    einstellungenSpeichern();
                }
            }
        }
        const karte::Teiler::Kind& kk = t.kinder[i];
        if (kk.eingeklappt) {
            kinder[i](senkrecht ? x : x + kk.pos, senkrecht ? y + kk.pos : y, 0, 0);
            continue;
        }
        if (senkrecht) {
            kinder[i](x, y + kk.pos, b, kk.groesse);
        } else {
            kinder[i](x + kk.pos, y, kk.groesse, h);
        }
    }
}

}  // namespace

void teilerImFenster(const char* id, karte::Teiler& t, bool senkrecht, int b, int h, const std::vector<TeilerInhalt>& kinder) {
    const ImVec2 alt = g_ursprung;
    g_ursprung = ImGui::GetWindowPos();
    std::vector<Inhalt> k(kinder.begin(), kinder.end());
    teilerZeichnen(id, t, senkrecht, 0, 0, b, h, k);
    g_ursprung = alt;
}

namespace {

// Ein Kind eines QSplitter: GL-Fenster (QOpenGLWidget ohne gueltigen
// sizeHint, ohne Eltern erzeugt = 640 x 480) oder ein innerer QSplitter
// (sizeHint aus Kindern ohne gueltigen Hinweis = 0 x 0).
karte::Teiler::Kind glKind(bool senkrecht) {
    karte::Teiler::Kind k;
    k.hinweis = -1;
    k.startGroesse = senkrecht ? 480 : 640;
    return k;
}
karte::Teiler::Kind teilerKind() {
    karte::Teiler::Kind k;
    k.hinweis = 0;
    return k;
}

}  // namespace

// Die Teiler fuer die Anordnung aufbauen (MainFrame::Create) und die
// Groessen setzen (RestoreGuiState, mainframe.cpp:1891-1900).
void teilerAufbauen() {
    Editor& ed = editor();
    using befehle::Anordnung;
    const int fuer = static_cast<int>(ed.anordnung) * 2 + (ed.gruppenDialogEingebaut ? 1 : 0);
    if (ed.teilerFuer == fuer) { return; }
    ed.teilerFuer = fuer;
    ed.maximiert = false;
    ed.hSplit = karte::Teiler{};
    ed.vSplit = karte::Teiler{};
    ed.vSplit2 = karte::Teiler{};
    const bool regular = ed.anordnung == Anordnung::Regular || ed.anordnung == Anordnung::RegularLeft;
    if (regular) {
        ed.hSplit.kinder = {teilerKind(), teilerKind()};
        ed.vSplit.kinder = {glKind(true), glKind(true)};   // XY, Konsole
        ed.vSplit2.kinder = {glKind(true)};                 // Kamera
        if (!ed.gruppenDialogEingebaut) { ed.vSplit2.kinder.push_back(glKind(true)); }   // Texturbrowser
    } else if (ed.anordnung == Anordnung::Split) {
        ed.hSplit.kinder = {teilerKind(), teilerKind()};
        ed.vSplit.kinder = {glKind(true), glKind(true)};    // Kamera, YZ
        ed.vSplit2.kinder = {glKind(true), glKind(true)};   // XY, XZ
    } else {
        return;   // Floating: keine Teiler
    }
    if (ed.gruppenDialogEingebaut) {
        // m_hSplit->addWidget( GroupDialog_getWindow() ) mit den Dehnfaktoren
        // 2222, 2222, 0 (mainframe.cpp:1857-1862). Der Hinweis des
        // Gruppendialogs ist nicht gemessen: seine Fensterbreite 444
        // (groupdialog.cpp:85).
        karte::Teiler::Kind gruppe;
        gruppe.hinweis = 444;
        ed.hSplit.kinder.push_back(gruppe);
        ed.hSplit.kinder[0].dehnfaktor = 2222;
        ed.hSplit.kinder[1].dehnfaktor = 2222;
    }
    if (regular) {
        // g_guiSettings.addSplitter( ..., { 384, 576 } ) usw. - oder die
        // gemerkten Groessen (restoreState).
        ed.hSplit.setzeGroessen(ed.hGroessen.empty() ? std::vector<int>{384, 576} : ed.hGroessen);
        ed.vSplit.setzeGroessen(ed.vGroessen.empty() ? std::vector<int>{377, 20} : ed.vGroessen);
        ed.vSplit2.setzeGroessen(ed.v2Groessen.empty() ? std::vector<int>{250, 150} : ed.v2Groessen);
    }
}

namespace {

void anordnungZeichnen(int x, int y, int b, int h) {
    Editor& ed = editor();
    using befehle::Anordnung;
    teilerAufbauen();
    const auto spalte = [](karte::Teiler& t, const char* id, std::vector<Feld> felder) -> Inhalt {
        return [&t, id, felder](int x2, int y2, int b2, int h2) {
            std::vector<Inhalt> kinder;
            for (const Feld f : felder) {
                kinder.emplace_back([f](int x3, int y3, int b3, int h3) { feld(f, x3, y3, b3, h3); });
            }
            teilerZeichnen(id, t, true, x2, y2, b2, h2, kinder);
        };
    };
    const Inhalt gruppe = [](int x2, int y2, int b2, int h2) { feld(Feld::Gruppe, x2, y2, b2, h2); };
    if (ed.anordnung == Anordnung::Regular || ed.anordnung == Anordnung::RegularLeft) {
        std::vector<Feld> rechts{Feld::Kamera};
        if (!ed.gruppenDialogEingebaut) { rechts.push_back(Feld::Texturen); }
        const Inhalt xySpalte = spalte(ed.vSplit, "v", {Feld::XY, Feld::Konsole});
        const Inhalt kamSpalte = spalte(ed.vSplit2, "v2", rechts);
        std::vector<Inhalt> kinder = ed.anordnung == Anordnung::Regular ? std::vector<Inhalt>{xySpalte, kamSpalte}
                                                                       : std::vector<Inhalt>{kamSpalte, xySpalte};
        if (ed.gruppenDialogEingebaut) { kinder.push_back(gruppe); }
        teilerZeichnen("h", ed.hSplit, false, x, y, b, h, kinder);
    } else if (ed.anordnung == Anordnung::Split) {
        // 4 way (mainframe.cpp:1829-1855): links Kamera ueber YZ, rechts XY ueber XZ
        std::vector<Inhalt> kinder{spalte(ed.vSplit, "v", {Feld::Kamera, Feld::YZ}),
                                   spalte(ed.vSplit2, "v2", {Feld::XY, Feld::XZ})};
        if (ed.gruppenDialogEingebaut) { kinder.push_back(gruppe); }
        teilerZeichnen("h", ed.hSplit, false, x, y, b, h, kinder);
    } else {
        // Floating: der Hauptbereich bleibt leer, die Ansichten schweben.
        merkeFeld("Mitte", x, y, b, h);
    }
}

// Ein schwebendes Fenster (Floating): g_guiSettings.addWindow( window, ...,
// 400, 300, x, y ) - Innenmass 400 x 300 und die Lage des Rahmens in
// Bildschirmpunkten (mainframe.cpp:1756, 1775, 1793, 1811); hier auf die
// Ecke des Programmfensters bezogen.
void schwebend(Feld f, const char* titel, bool& sichtbar, int lx, int ly) {
    if (!sichtbar) { return; }
    const ImGuiStyle& st = ImGui::GetStyle();
    const float titelH = ImGui::GetFontSize() + st.FramePadding.y * 2.0F;
    const ImVec2 vp = ImGui::GetMainViewport()->Pos;
    const ImGuiCond wann = editor().schwebendZuruecksetzen ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
    ImGui::SetNextWindowPos(ImVec2{vp.x + std::round(static_cast<float>(lx) * g_s), vp.y + std::round(static_cast<float>(ly) * g_s)},
                            wann);
    // Innenmass 400 x 300: ImGui-Fenster ohne Polster, Inhalt = Groesse minus Titelleiste.
    ImGui::SetNextWindowSize(ImVec2{std::round(400.0F * g_s), std::round(300.0F * g_s) + titelH}, wann);
    const std::string id = std::string(titel) + "###schwebend" + feldName(f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    if (ImGui::Begin(id.c_str(), &sichtbar, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse)) {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const ImVec2 gr = ImGui::GetContentRegionAvail();
        merkeFeld(std::string("Fenster:") + feldKurz(f), static_cast<int>(std::lround((p.x - vp.x) / g_s)),
                  static_cast<int>(std::lround((p.y - vp.y) / g_s)), static_cast<int>(std::lround(gr.x / g_s)),
                  static_cast<int>(std::lround(gr.y / g_s)));
        feldInhalt(f);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// Der Gruppendialog (groupdialog.cpp, Fenstertitel "Entities"): Reiter
// Entities, (Console, Textures bei Floating/Split), Models, Layers.
// g_guiSettings.addWindow( m_window, "GroupDlg/geometry", 444, 777 ).
void gruppenDialog() {
    Editor& ed = editor();
    using befehle::Anordnung;
    const bool schwebendeSeiten = ed.anordnung == Anordnung::Floating || ed.anordnung == Anordnung::Split;
    if (!ed.gruppenDialogOffen) { return; }
    const ImGuiStyle& st = ImGui::GetStyle();
    const float titelH = ImGui::GetFontSize() + st.FramePadding.y * 2.0F;
    const ImVec2 bereich = ImGui::GetMainViewport()->Size;
    const ImVec2 vp = ImGui::GetMainViewport()->Pos;
    const ImVec2 groesse{std::round(444.0F * g_s), std::round(777.0F * g_s) + titelH};
    ImGui::SetNextWindowPos(ImVec2{vp.x + std::max(0.0F, (bereich.x - groesse.x) * 0.5F),
                                   vp.y + std::max(0.0F, (bereich.y - groesse.y) * 0.5F)},
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(groesse, ImGuiCond_FirstUseEver);
    const char* seitenTitel[5] = {trK(Str::KePageEntities), trK(Str::KePageConsole), trK(Str::KePageTextures),
                                  trK(Str::KePageModels), trK(Str::KePageLayers)};
    // GroupDialog_updatePageTitle: die Textur-Seite meldet ihren Titel
    // (TextureBrowser_exportTitle, "Textures: <dir>").
    const std::string titel = (ed.gruppenSeite == 2 ? texturTitel() : std::string(seitenTitel[ed.gruppenSeite])) + "###kartegruppe";
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
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
                        ImGui::BeginChild("##gruppeTexturen", ImVec2{-FLT_MIN, -FLT_MIN}, ImGuiChildFlags_None,
                                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                        ImGui::PopStyleVar();
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

// MaximizeView (mainframe.cpp:1550-1595): je Teiler das Feld unter der Maus
// auf 9999, das andere auf 0; beim Zuruecknehmen die alten Groessen.
void maximierenUmschalten() {
    Editor& ed = editor();
    if (ed.anordnung == befehle::Anordnung::Floating) { return; }   // m_hSplit == 0
    teilerAufbauen();
    if (ed.maximiert) {
        ed.maximiert = false;
        ed.vSplit.setzeGroessen(ed.maxV);
        ed.vSplit2.setzeGroessen(ed.maxV2);
        ed.hSplit.setzeGroessen(ed.maxH);
        return;
    }
    ed.maximiert = true;
    ed.maxV = ed.vSplit.groessen();
    ed.maxV2 = ed.vSplit2.groessen();
    ed.maxH = ed.hSplit.groessen();
    const auto erstes = [](const std::vector<int>& v) { return v.empty() ? 0 : v[0]; };
    ed.vSplit.setzeGroessen(ed.mausY < erstes(ed.maxV) ? std::vector<int>{9999, 0} : std::vector<int>{0, 9999});
    ed.vSplit2.setzeGroessen(ed.mausY < erstes(ed.maxV2) ? std::vector<int>{9999, 0} : std::vector<int>{0, 9999});
    ed.hSplit.setzeGroessen(ed.mausX < erstes(ed.maxH) ? std::vector<int>{9999, 0} : std::vector<int>{0, 9999});
}

void zeichne(float hoehe) {
    Editor& e = g();
    e.gezeichnet = true;
    e.knoepfeImBild.clear();
    e.knoepfeVerdeckt.clear();
    e.felderImBild.clear();
    spielPruefen();
    if (!e.belegungGeladen) { einstellungenLaden(); }
    zusatztastenPruefen();   // g_modifiers (onModifierDown / onModifierUp)
    texturenAktualisieren();   // g_ActiveShaders: Shader der Karte, in Gebrauch
    // GetGridSize / GetSnapGridSize und Select_getWorkZone fuer das Auswahlsystem
    if (e.karte.auswahl.anzahl() != 0) {   // Selection_UpdateWorkzone: getBoundsSelected
        const karte::Kasten b = karte::auswahlGrenzenPivot(e.karte);
        if (karte::kastenGueltig(b)) { e.workzone = b; }
    }
    e.bearbeitung.raster = e.raster.groesse();
    e.bearbeitung.fang = e.raster.einrastGroesse();
    e.bearbeitung.arbeitsbereich = e.workzone;
    farbenAusThema();
    g_s = massstab();
    // NRCs Schrift: Segoe UI 9 pt (Menues und Statuszeile von Qt unter
    // Windows) = 12 Punkte Geviert, mal Massstab. ImGui multipliziert die
    // Grundgroesse noch mit FontScaleDpi und FontScaleMain.
    const float schrift = static_cast<float>(mass::kSchrift) * g_s / std::max(0.01F, g_app->dpiScale * g_app->uiScale);
    ImGui::PushFont(g_schrift, schrift);
    // hoehe <= 0 wie bei BeginChild: der restliche Platz minus |hoehe|
    // (app.cpp gibt frameH = -fussH).
    const ImVec2 rest = ImGui::GetContentRegionAvail();
    ImVec2 platz{rest.x, hoehe > 0.0F ? hoehe : std::max(1.0F, rest.y + hoehe)};
    if (bereichErsatzB > 0 && bereichErsatzH > 0) {
        platz = ImVec2{static_cast<float>(bereichErsatzB) * g_s, static_cast<float>(bereichErsatzH) * g_s};
    }
    // QMenuBar 21 hoch: Schrift 16 + 2 x 2,5 (ImGui: FontSize + FramePadding.y * 2).
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2{0.0F, static_cast<float>(mass::kMenueH - mass::kSchrift) * 0.5F * g_s});
    ImGui::BeginChild("##karteneditor", platz, ImGuiChildFlags_None,
                      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(3);
    g_ursprung = ImGui::GetWindowPos();
    const int breite = static_cast<int>(std::floor(platz.x / g_s + 0.001F));
    const int hoeheL = static_cast<int>(std::floor(platz.y / g_s + 0.001F));
    merkeFeld("Bereich", 0, 0, breite, hoeheL);
    zeichneMenueleiste();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    int y = mass::kMenueH;
    if (e.hauptleiste) {
        zeichneHauptleiste(y, breite);
        y += mass::kLeisteH;
    }
    const int statusY = hoeheL - mass::kStatusH;
    const int mitteH = std::max(0, statusY - y);
    // Filterleiste rechts angedockt (Qt::RightToolBarArea), daneben die Mitte.
    const int filterB = (e.filterleiste || e.pluginleiste) ? mass::kLeisteH : 0;
    const int mitteB = std::max(0, breite - filterB);
    // Lage der Maus ueber m_hSplit (fuer MaximizeView).
    const ImVec2 maus = ImGui::GetIO().MousePos;
    e.mausX = static_cast<int>(std::floor((maus.x - bildpunkt(0, y).x) / g_s));
    e.mausY = static_cast<int>(std::floor((maus.y - bildpunkt(0, y).y) / g_s));
    anordnungZeichnen(0, y, mitteB, mitteH);
    zeichneRechteLeisten(mitteB, y, mitteH);
    zeichneStatuszeile(statusY, breite);
    ImGui::PopStyleVar();
    ImGui::EndChild();
    // Schwebende Fenster (Floating) und der Gruppendialog.
    if (e.anordnung == befehle::Anordnung::Floating) {
        schwebend(Feld::Kamera, trK(Str::KeViewCamera), e.schwebendSichtbar[3], 50, 100);
        schwebend(Feld::XY, trK(Str::KeViewXY), e.schwebendSichtbar[2], 500, 100);
        schwebend(Feld::XZ, trK(Str::KeViewXZ), e.schwebendSichtbar[1], 500, 450);
        schwebend(Feld::YZ, trK(Str::KeViewYZ), e.schwebendSichtbar[0], 50, 450);
        e.schwebendZuruecksetzen = false;
    }
    gruppenDialog();
    texturDialogeZeichnen();
    zeichneEinstellungen();
    zeichneTastenDialog();
    zeichneRueckfrage();
    ImGui::PopFont();
    tastenVerarbeiten();
}
}  // namespace bhed::gui::karteneditor

namespace bhed::gui {
// Aus buildFonts (main_win32.cpp): NRCs Schrift fuer den Karten-Editor.
void setKartenSchrift(ImFont* schrift) { karteneditor::setzeSchrift(schrift); }
}  // namespace bhed::gui
