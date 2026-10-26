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
#include "karte_echtlicht.h"

#include "app_internal.h"
#include "bhed/theme.h"
#include "karte_gpu.h"
#include "bhed/karte/texdef.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <iterator>
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

std::shared_ptr<Spielkontext> spielLadenJetzt(int spiel, std::vector<std::string> ordner, std::string defs, std::string modus) {
    const auto t0 = std::chrono::steady_clock::now();
    auto k = std::make_shared<Spielkontext>();
    k->spiel = spiel;
    k->modus = modus;
    k->name = spielName(spiel);
    k->ordner = std::move(ordner);
    for (const std::string& o : k->ordner) { k->fs.ordnerHinzufuegen(o, modus); }
    // Shaders_Load: shaderlist.txt aus den Archiven, sonst alle Shader
    // ("No shaderlist.txt found: loading all shaders"). NRCs
    // default_shaderlist.txt stammt aus dem Gamepack und kommt nicht mit.
    k->inventar.laden(k->fs);
    (void)k->klassen.ordnerLaden(defs, modus);
    if (spiel == kRegelwerkMovieDuels) { (void)k->klassen.entdefsErgaenzen(); }
    k->meldungen = k->inventar.meldungen();
    // Die Konsole wie NRC beim Start: QE_InitVFS (vfs directory, pak file),
    // "filesystem initialised", dann die Beobachter: Shaders_Load und
    // EntityClassQuake3_Construct.
    k->protokoll = k->fs.protokoll;
    karte::ausgabe(k->protokoll, "filesystem initialised\n");
    k->protokoll.insert(k->protokoll.end(), k->inventar.protokoll.begin(), k->inventar.protokoll.end());
    k->protokoll.insert(k->protokoll.end(), k->klassen.protokoll.begin(), k->klassen.protokoll.end());
    k->sekunden = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    return k;
}

void spielUebernehmen(std::shared_ptr<Spielkontext> k) {
    g().spiel = std::move(k);
    const Spielkontext& s = *g().spiel;
    konsoleProtokoll(s.protokoll);
    char z[300];
    std::snprintf(z, sizeof(z), "%s: %d shaders, %d entity classes (%.1f s)", s.name.c_str(), static_cast<int>(s.inventar.shader().size()),
                  static_cast<int>(s.klassen.anzahl()), s.sekunden);
    diag::detail(std::string("Karten-Editor: ") + z);
    if (s.ordner.empty()) {
        std::snprintf(z, sizeof(z), tr(Str::KeGameNoFolders), s.name.c_str());
        warnung(z);
    }
    // Die leere Karte des Starts bekommt das Spiel, sobald es da ist - und
    // jede offene Karte das neu geladene (Stufe 7 Punkt 6f, Tester: nach dem
    // Start "SHADER NOT FOUND" auf allen Flaechen, auch auf neuem caulk, bis
    // eine Karte von Hand geoeffnet wurde). NRC: EnginePath_Realise /
    // gamemode_set bauen VFS und Shader neu, und die Flaechen der offenen
    // Karte holen ihre Shader beim Realise neu (Shader_Realise ueber die
    // Namen). Hier zeigen Karte und Flaechen danach auf das neue Spiel; die
    // Klassen der vorhandenen Entities bleiben gueltig (das alte Spiel lebt in
    // karteSpielAlt weiter, bis die Karte freigegeben wird).
    Editor& e = g();
    if (e.karteSpiel != e.spiel) {
        if (e.karteSpiel) { e.karteSpielAlt.push_back(e.karteSpiel); }
        e.karteSpiel = e.spiel;
        e.karte.klassen = &e.karteSpiel->klassen;
        e.karte.kontext.texturen = &e.karteSpiel->inventar;
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
    karte::regionAus(e.karte, e.region, e.raster.maxKoord());   // Map_Free: Map_RegionOff
    e.karte.auswahl.leeren();
    e.bspHerkunft = BspHerkunft{};   // Stufe 4: die fertige Karte ist nicht mehr offen
    pluginsFreigeben();              // Stufe 4 Punkt 3: BobToolz_destroy
    e.bearbeitung.neueKarte();
    for (auto& b : e.beobachter) { b = FensterBeobachter{b.breite, b.hoehe}; }
    e.karte.leeren();
    e.karte.undo.clear();
    e.karteSpiel = spielWarten();
    e.karteSpielAlt.clear();   // keine Entity zeigt mehr auf ein frueheres Spiel
    e.karte.klassen = e.karteSpiel ? &e.karteSpiel->klassen : nullptr;
    e.karte.kontext.texturen = e.karteSpiel ? &e.karteSpiel->inventar : nullptr;
    // Map_Free: Brush_unlatchPreferences -> Brush_toggleFormat( g_brushTypes[g_brushType] )
    // ("New map Brush Type"; beim Lesen erkennt Map_Read das Format der Datei)
    e.karte.kontext.format = static_cast<karte::PinselFormat>(std::clamp(e.pinselTyp, 0, 2));
    // Map_Free: FlushReferences (ModelCache_clear); die Modelle kommen aus dem Spiel
    modelleLeeren();
    e.karte.kontext.modelle = &kartenModelle();
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

// Ein Pfad, wie NRC ihn nennt (Qts Dateiwahl liefert '/').
std::string nrcPfad(std::string p) {
    std::replace(p.begin(), p.end(), '\\', '/');
    return p;
}

// ScopeTimer (map.cpp:982-993): "<text> timer: %5.2f second(s) elapsed"
std::string zeitText(const char* was, std::chrono::steady_clock::time_point t0) {
    const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    char z[160];
    std::snprintf(z, sizeof(z), "%s timer: %5.2f second(s) elapsed", was, s);   // FloatFormat( sec, 5, 2 )
    return z;
}

// Map_Save (map.cpp:1249-1255): ScopeTimer "map save" um SaveReferences ->
// MapResource_save (referencecache.cpp:96-140): Sicherung .bak, dann
// "Open file X for write..." + success / failure.
bool karteSchreibenMitMeldungen(const std::string& pfad) {
    punktdateiLeeren();   // Map_Save: Pointfile_Clear
    const auto t0 = std::chrono::steady_clock::now();
    const std::string anzeige = nrcPfad(pfad);
    std::string grund;
    const bool ok = karte::karteSpeichern(pfad, g().karte, true, &grund);
    if (!ok && grund.rfind("map path", 0) == 0) {   // file_saveBackup schlug fehl
        fehler(grund);
        fehler("failed to save map file: \"" + anzeige + "\"");
    } else {
        konsoleSchreiben(KonsolenZeile::Art::Ausgabe, "Open file " + anzeige + " for write...");
        konsoleSchreiben(ok ? KonsolenZeile::Art::Ausgabe : KonsolenZeile::Art::Fehler, ok ? "success\n" : "failure\n");
    }
    ausgabe(zeitText("map save", t0));
    return ok;
}

// MapResource_saveFile (referencecache.cpp:98-111)
bool dateiSchreibenMitMeldung(const std::string& pfad, const std::string& text) {
    konsoleSchreiben(KonsolenZeile::Art::Ausgabe, "Open file " + nrcPfad(pfad) + " for write...");
    const std::string roh = karte::textmodusSchreiben(text);
    std::ofstream f(utf8Pfad(pfad), std::ios::binary | std::ios::trunc);
    bool ok = static_cast<bool>(f);
    if (ok) {
        f.write(roh.data(), static_cast<std::streamsize>(roh.size()));
        ok = static_cast<bool>(f);
    }
    konsoleSchreiben(ok ? KonsolenZeile::Art::Ausgabe : KonsolenZeile::Art::Fehler, ok ? "success\n" : "failure\n");
    return ok;
}

std::string startOrdner() {
    // map_open: bei einer unbenannten Karte der maps-Ordner, sonst die Karte.
    if (g().name != "unnamed.map") { return directoryOf(g().name); }
    return g_app->settings.karte.count("mapsPath") != 0U ? g_app->settings.karte.at("mapsPath") : std::string();
}

}  // namespace

void rueckfrageDann(const char* titel, std::function<void()> danach) { mitRueckfrage(titel, std::move(danach)); }

bool karteDateiSchreiben(const std::string& pfad, const std::string& text) { return dateiSchreibenMitMeldung(pfad, text); }

bool karteDateiLesen(const std::string& pfad, std::string& text) {   // MapResource_loadFile
    konsoleSchreiben(KonsolenZeile::Art::Ausgabe, "Open file " + nrcPfad(pfad) + " for read...");
    std::ifstream f(utf8Pfad(pfad), std::ios::binary);
    if (!f) {
        konsoleSchreiben(KonsolenZeile::Art::Fehler, "failure\n");
        return false;
    }
    konsoleSchreiben(KonsolenZeile::Art::Ausgabe, "success\n");
    const std::string roh((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    text = karte::textmodusLesen(roh);
    return true;
}

std::string kartenStartOrdner() { return startOrdner(); }
const char* kartenDateiFilter() { return kKartenFilter; }

std::string kartenBenutzerOrdner() {
    // m_userGamePath: der Spielordner (fs_game vor basegame) + "maps/";
    // im Selbsttest der Laufordner (nie in den Spielordner schreiben)
    if (std::getenv("BHED_EDITORTEST") != nullptr) { return nrcPfad(fs::current_path().string()) + "/maps/"; }
    const Editor& e = g();
    if (e.karteSpiel && !e.karteSpiel->ordner.empty()) { return nrcPfad(e.karteSpiel->ordner.front()) + "/maps/"; }
    return nrcPfad(platform::settingsDirectory()) + "/maps/";
}

// Entity_reloadDefinitions (entity.cpp:607-612): GlobalEntityClassManager()
// unrealise / realise liest die Klassen neu; MapEntityClasses (map.cpp:
// 2374-2396) haengt daran und laedt die Karte neu (flush + realise der
// MapResource: aus der Datei, eine unbenannte ist danach leer).
void entitaetenKlassenNeuLaden() {
    Editor& e = g();
    ++e.neuGeladen;
    const std::string name = e.name;
    karteFreigeben();   // die Entitaeten zeigen auf die alten Klassen
    if (e.karteSpiel) {
        Spielkontext& s = *e.karteSpiel;
        s.klassen.leeren();
        s.klassen.protokoll.clear();
        (void)s.klassen.ordnerLaden(defOrdner(s.spiel), s.modus);
        if (s.spiel == kRegelwerkMovieDuels) { (void)s.klassen.entdefsErgaenzen(); }
        konsoleProtokoll(s.klassen.protokoll);   // EntityClassQuake3_Construct (realise)
    }
    ++e.ent.klassenStand;
    e.ent.geladen = false;
    if (name != "unnamed.map") {
        (void)karteOeffnen(name);
    } else {
        e.name = "unnamed.map";
        e.karte.undo.alsGespeichertMarkieren();
        ++e.ladeZaehler;
    }
}

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

std::string fensterTitel() { return g().name + (geaendert() ? " *" : ""); }   // Map_UpdateTitle -> Sys_SetTitle


void spielPruefen() {
    Editor& e = g();
    if (e.spielLaden.valid() &&
        e.spielLaden.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        spielUebernehmen(e.spielLaden.get());
    }
    const int spiel = kartenSpielNummer();
    // EnginePath_Realise / gamemode_set: neu laden, wenn sich Spiel, Suchordner oder Modus aendern
    const std::vector<std::string> ordner = kartenVfsOrdner();
    std::string kennung = std::to_string(spiel) + "|" + e.spielModus;
    for (const std::string& o : ordner) { kennung += "|" + o; }
    if (kennung == e.spielKennung) { return; }
    if (e.spielLaden.valid()) { return; }   // erst das laufende fertig laden
    e.spielKennung = kennung;
    e.spielStand = spiel;
    // Brush_Construct (brushmodule.cpp:166-182): default_scale aus ja.game/jk2.game
    // (0.25 statt des Grundwerts 0.5), dann die gemerkte "TexdefDefaultScale".
    float skala = karte::spielBeschreibung(spiel == kRegelwerkJediOutcast ? "jk2.game" : "ja.game").standardSkala;
    if (const auto it = g_app->settings.karte.find("TexdefDefaultScale"); it != g_app->settings.karte.end()) {
        try {
            skala = std::stof(it->second);
        } catch (...) {
        }
    }
    karte::setzeStandardTexturSkalierung(skala);
    diag::detail("Karten-Editor: lade " + spielName(spiel));
    e.spielLaden = std::async(std::launch::async, spielLadenJetzt, spiel, ordner, defOrdner(spiel), e.spielModus);
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
        punktdateiLeeren();   // Map_Free: Pointfile_Clear
        karteFreigeben();
        g().name = "unnamed.map";
        g().karte.undo.alsGespeichertMarkieren();
        g().autosave.leeren();   // Map::realise: AutoSave_clear
        ++g().ladeZaehler;
        startPosition();   // Map_New: FocusViews( g_vector3_identity, 0 ) ueber Map_StartPosition
    });
}

bool karteOeffnen(const std::string& pfad) {
    Editor& e = g();
    punktdateiLeeren();   // Map_LoadFile -> Map_Free: Pointfile_Clear
    karteFreigeben();
    e.name = pfad;
    // Map_LoadFile (map.cpp:1001-1026): Pfade mit '/' wie aus Qts Dateiwahl
    const std::string anzeige = nrcPfad(pfad);
    ausgabe("Loading map from " + anzeige);
    const auto t0 = std::chrono::steady_clock::now();
    karte::LeseErgebnis ergebnis;
    // MapResource_loadFile (referencecache.cpp:64-79): "Open file X for read..." + success / failure
    konsoleSchreiben(KonsolenZeile::Art::Ausgabe, "Open file " + anzeige + " for read...");
    const bool ok = karte::karteLaden(pfad, e.karte, ergebnis);
    if (ok) {
        konsoleSchreiben(KonsolenZeile::Art::Ausgabe, "success\n");
        for (const std::string& m : ergebnis.meldungen) {
            if (m.find("rror") != std::string::npos) { fehler(m); } else { warnung(m); }
        }
    } else {
        konsoleSchreiben(KonsolenZeile::Art::Fehler, "failure\n");
    }
    e.karte.anmelden();
    e.karte.undo.clear();
    e.karte.undo.alsGespeichertMarkieren();
    e.autosave.leeren();   // Map::realise: AutoSave_clear
    std::size_t primitive = 0;
    for (const auto& ent : e.karte.entitaeten()) { primitive += ent->kinder().size(); }
    ausgabe(zeitText("map load", t0));   // ScopeTimer timer( "map load" )
    ausgabe("--- LoadMapFile ---");
    ausgabe(nrcPfad(e.name));
    ausgabe(std::to_string(primitive) + " primitives");
    ausgabe(std::to_string(e.karte.entitaeten().size()) + " entities");
    ++e.ladeZaehler;
    startPosition();   // Map_LoadFile: Map_StartPosition
    return ok;
}

// Das Spiel des Editors: "gamefile" aus Global > Game, sonst nach dem Regelwerk
// (CGameDialog::Init: m_sGameFile; Movie Duels ist Jedi Academy mit eigenen Ordnern).
int kartenSpielNummer() {
    const int regel = regelwerkSpielErmitteln();
    const std::string& d = g().spielDatei;
    if (d == "jk2.game") { return kRegelwerkJediOutcast; }
    if (d == "ja.game") { return regel == kRegelwerkJediOutcast ? kRegelwerkOpenJK : regel; }
    return regel;
}

// g_pGameDescription: ja.game bzw. jk2.game zum Spiel des Editors
const karte::SpielBeschreibung& kartenSpiel() {
    return karte::spielBeschreibung(kartenSpielNummer() == kRegelwerkJediOutcast ? "jk2.game" : "ja.game");
}

// gamename_get: der Ordner des Spiels (basegame, solange keine Mod gewaehlt ist)
std::string kartenFsGame() { return g().spielName.empty() ? std::string(kartenSpiel().basisSpiel) : g().spielName; }

// EnginePath_get: der gesetzte Pfad, sonst der Ordner ueber dem ersten
// Spielordner des Programms (der Engine-Pfad, unter dem basegame liegt).
// NRC#164 (Stufe 6): ein relativer Pfad (../../Academy/) loeste in NRC beim
// Speichern eine Zusicherung aus und gilt fuer die Werkzeuge relativ zu deren
// Arbeitsordner. Beim Setzen wird er absolut - zum Programmordner, wie NRC
// ihn mit dem Programmordner als Arbeitsordner meinte.
std::string ordnerAbsolut(const std::string& pfad) {
    if (pfad.empty()) { return pfad; }
    fs::path p = utf8Pfad(pfad);
    if (p.is_relative()) { p = utf8Pfad(platform::executableDirectory()) / p; }
    const std::u8string u = p.lexically_normal().u8string();
    return karte::ordnerBereinigt(std::string(u.begin(), u.end()));
}

std::string enginePfadAnzeige() {
    if (!g().enginePfad.empty()) { return g().enginePfad; }
    const std::vector<std::string> prog = suchpfade(kartenSpielNummer());
    if (prog.empty()) { return {}; }
    const fs::path p = utf8Pfad(karte::ordnerBereinigt(prog.front()));
    const std::u8string u = p.parent_path().parent_path().u8string();
    return karte::ordnerBereinigt(std::string(u.begin(), u.end()));
}

// QE_InitVFS (qe3.cpp:68-102): die zusaetzlichen Pfade, <Engine><fs_game>/,
// <Engine><basegame>/. Ohne gesetzten Engine-Pfad stehen statt
// <Engine><basegame>/ die Spielordner des Programms (Movie Duels vor base).
std::vector<std::string> kartenVfsOrdner() {
    const Editor& e = g();
    const karte::SpielBeschreibung& sb = kartenSpiel();
    std::vector<std::string> zusatz;
    for (const std::string& p : e.zusatzPfade) {
        if (!p.empty()) { zusatz.push_back(p); }
    }
    if (!e.enginePfad.empty()) { return karte::vfsOrdner(e.enginePfad, kartenFsGame(), sb.basisSpiel, zusatz); }
    std::vector<std::string> pfade = karte::vfsOrdner(enginePfadAnzeige(), kartenFsGame(), sb.basisSpiel, zusatz);
    if (!pfade.empty()) { pfade.pop_back(); }   // <Engine><basegame>/ - dafuer die Spielordner
    for (const std::string& o : suchpfade(kartenSpielNummer())) {
        const std::string b = karte::ordnerBereinigt(o);
        if (std::none_of(pfade.begin(), pfade.end(), [&](const std::string& q) { return karte::ordnerBereinigt(q) == b; })) { pfade.push_back(o); }
    }
    return pfade;
}

// setEnginePath / GameImport / gamemode_set: Unrealise, aendern, Realise. Die
// Entity-Klassen haengen daran, und mit ihnen die Karte (MapEntityClasses: aus
// der Datei neu, eine unbenannte ist danach leer).
void spielNeuRealisieren(const char* titel, std::function<void()> aendern) {
    mitRueckfrage(titel, [aendern] {
        Editor& e = g();
        const std::string name = e.name;
        aendern();
        spielPruefen();   // EnginePath_Realise: neu laden
        if (name != "unnamed.map") {
            (void)karteOeffnen(name);   // karteFreigeben -> spielWarten: mit dem neuen Spiel
        } else {
            punktdateiLeeren();   // Map_Free
            karteFreigeben();
            e.name = "unnamed.map";
            e.karte.undo.alsGespeichertMarkieren();
            ++e.ladeZaehler;
        }
    });
}

// Radiant_Restart (mainframe.cpp:638-656): ConfirmModified, dann mit der Karte
// neu starten - hier der Editor mit dem gewaehlten Spiel (m_sGameFile).
void radiantNeustart() {
    mitRueckfrage("Restart Radiant", [] {
        Editor& e = g();
        const std::string name = e.name;
        e.spielKennung.clear();
        spielPruefen();
        if (name != "unnamed.map") {
            (void)karteOeffnen(name);
        } else {
            punktdateiLeeren();   // Map_Free
            karteFreigeben();
            e.name = "unnamed.map";
            e.karte.undo.alsGespeichertMarkieren();
            ++e.ladeZaehler;
        }
    });
}

// MRU_AddFile (fuer den Selbsttest)
void zuletztEintragen(const std::string& pfad) { zuletztHinzu(pfad); }

// OpenBugReportURL (mainframe.cpp:697-700)
std::string bugReportUrl() { return "https://github.com/Garux/netradiant-custom/issues"; }

// OpenURL / Q_Exec: im Selbsttest ersetzbar (sonst Browser bzw. Programm)
// Im Selbsttest (BHED_EDITORTEST) startet der Editor nie ein anderes Programm
// und oeffnet keine Datei mit ihrer Anwendung - ohne Ersatz nur gemerkt
// (Stufe 3 Punkt 0c: Strg+Umschalt+M1 im Texturbrowser oeffnete eine lose
// .shader-Datei sonst mit der Anwendung des Anwenders).
void kartenUrlOeffnen(const std::string& url) {
    if (g().urlOeffnen) {
        g().urlOeffnen(url);
    } else if (std::getenv("BHED_EDITORTEST") != nullptr) {
        g().letzteUrl = url;
        ausgabe("(Selbsttest) nicht geoeffnet: " + url);
    } else {
        platform::openWithDefaultApp(url);
    }
}

std::string kartenDateiDialog(bool oeffnen, const char* titel, const char* filter, const std::string& start, const std::string& name) {
    if (g().dateiDialog) { return g().dateiDialog(titel); }
    return oeffnen ? platform::openFileDialog(titel, filter, start) : platform::saveFileDialog(titel, filter, start, name);
}

std::string kartenOrdnerDialog(const char* titel, const std::string& start) {
    if (g().dateiDialog) { return g().dateiDialog(titel); }
    return platform::pickFolder(titel, start);
}

bool kartenBefehlStarten(const std::string& zeile) {
    if (g().befehlStarten) { return g().befehlStarten(zeile); }
    if (std::getenv("BHED_EDITORTEST") != nullptr) {   // siehe kartenUrlOeffnen
        g().letzteUrl = zeile;
        ausgabe("(Selbsttest) nicht gestartet: " + zeile);
        return true;
    }
    return platform::befehlStarten(zeile);
}

// EnginePath_verify (mainframe.cpp:467-475): fehlt der Engine-Pfad, beim Start
// "Engine Path Configuration" (hier: ohne Spielordner im Programm).
void enginePfadPruefen() {
    Editor& e = g();
    if (e.enginePfadGeprueft) { return; }
    e.enginePfadGeprueft = true;
    const std::string pfad = enginePfadAnzeige();
    std::error_code ec;
    if (pfad.empty() || !fs::exists(utf8Pfad(pfad), ec)) { e.enginePfadDialogOffen = true; }   // !file_exists( g_strEnginePath )
}

// main.cpp:462-464: beim Start (hier: beim ersten Bild des Editors) die letzte
// Karte, wenn "Load last map at startup" an ist (Map_LoadFile, ohne MRU).
void letzteKarteBeimStart() {
    Editor& e = g();
    if (e.letzteKarteGeprueft) { return; }
    e.letzteKarteGeprueft = true;
    if (e.letzteKarteLaden && !e.letzteKarte.empty()) { (void)karteOeffnen(e.letzteKarte); }
}

// main.cpp:476-478: beim Beenden der Name der Karte, wenn sie einen hat.
void beimBeenden() {
    Editor& e = g();
    if (!e.belegungGeladen) { return; }   // der Editor lief nicht: "LastMap" bleibt
    if (e.name != "unnamed.map") { e.letzteKarte = e.name; }   // !Map_Unnamed( g_map )
    g_app->settings.karte["LastMap"] = e.letzteKarte;
    bauMenueSpeichern();   // BuildMenu_Destroy: SaveBuildMenu
    bauBeimBeenden();      // BuildMonitor_Destroy
    portalEinstellungenSchreiben();   // PrtView_destroy: SaveConfig
}

void karteOeffnenDialog() {
    mitRueckfrage("Open Map", [] {
        const std::string pfad = kartenDateiDialog(true, "Open Map", kKartenFilter, startOrdner());
        if (pfad.empty()) { return; }
        zuletztHinzu(pfad);   // MRU_AddFile vor dem Laden (map.cpp:1848)
        (void)karteOeffnen(pfad);
    });
}

bool karteSpeichernUnter(const std::string& pfad) {
    Editor& e = g();
    zuletztHinzu(pfad);   // Map_SaveAs: MRU_AddFile, Map_Rename, Map_Save
    e.name = pfad;
    if (!karteSchreibenMitMeldungen(pfad)) { return false; }
    e.karte.undo.alsGespeichertMarkieren();
    return true;
}

bool karteSpeichernUnterDialog() {
    const std::string pfad = kartenDateiDialog(false, "Save Map", kKartenFilter, startOrdner(), fileName(g().name));
    if (pfad.empty()) { return false; }
    return karteSpeichernUnter(pfad);
}

bool karteSpeichern() {
    Editor& e = g();
    // SaveMap (map.cpp:1879-1887): unbenannt -> Speichern unter; sonst nur,
    // wenn geaendert, und erst dann in die zuletzt-Liste.
    if (e.name == "unnamed.map") { return karteSpeichernUnterDialog(); }
    if (!geaendert()) { return true; }
    if (!karteSchreibenMitMeldungen(e.name)) { return false; }
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

namespace {
ImFont* g_glSchrift = nullptr;

// (hhea ascender - descender) / head unitsPerEm: stb_truetype rastert die
// ImGui-Groesse auf Ober- plus Unterlaenge (ScaleForPixelHeight).
float schriftVerhaeltnis(const std::string& datei) {
    std::FILE* f = std::fopen(datei.c_str(), "rb");
    if (f == nullptr) { return 1.33F; }
    std::vector<unsigned char> d;
    unsigned char puffer[65536];
    std::size_t n = 0;
    while ((n = std::fread(puffer, 1, sizeof(puffer), f)) > 0 && d.size() < (1U << 24)) { d.insert(d.end(), puffer, puffer + n); }
    std::fclose(f);
    const auto u16 = [&](std::size_t o) { return o + 1 < d.size() ? static_cast<unsigned>((d[o] << 8U) | d[o + 1]) : 0U; };
    const auto u32 = [&](std::size_t o) { return (static_cast<std::size_t>(u16(o)) << 16U) | u16(o + 2); };
    std::size_t basis = 0;
    if (d.size() > 16 && std::memcmp(d.data(), "ttcf", 4) == 0) { basis = u32(12); }   // die erste Schrift der Sammlung
    const unsigned tabellen = u16(basis + 4);
    int einheiten = 0;
    int ober = 0;
    int unter = 0;
    for (unsigned i = 0; i < tabellen; ++i) {
        const std::size_t e = basis + 12 + 16 * static_cast<std::size_t>(i);
        if (e + 16 > d.size()) { break; }
        const std::size_t o = u32(e + 8);
        if (std::memcmp(&d[e], "head", 4) == 0) { einheiten = static_cast<int>(u16(o + 18)); }
        if (std::memcmp(&d[e], "hhea", 4) == 0) {
            ober = static_cast<std::int16_t>(u16(o + 4));
            unter = static_cast<std::int16_t>(u16(o + 6));
        }
    }
    return einheiten > 0 && ober - unter > 0 ? static_cast<float>(ober - unter) / static_cast<float>(einheiten) : 1.33F;
}
}  // namespace

// GlobalOpenGL().m_font: die OpenGL-Schrift, wenn ihre Datei geladen ist; sonst
// die Kartenschrift (Myriad Pro gehoert nicht zu Windows - Segoe UI wie bisher).
ImFont* glSchrift() { return g_glSchrift != nullptr ? g_glSchrift : g_schrift; }

// glfont_create( font, size ): Punkte bei 96 dpi (Qt rechnet size * 96 / 72),
// als ImGui-Groesse mit dem Verhaeltnis der Schrift.
float glSchriftPx() {
    const Editor& e = g();
    const float verhaeltnis = g_glSchrift != nullptr ? e.glSchriftVerhaeltnis : 1.33F;
    return static_cast<float>(e.glSchriftGroesse) * 96.0F / 72.0F * verhaeltnis;
}

// OpenGLFont_select (mainframe.cpp:1987-2000): g_OpenGLFont / g_OpenGLFontSize
// und die Schrift neu (glfont_create) - hier: die Schriften neu bauen.
void glSchriftSetzen(const std::string& familie, int punkte) {
    Editor& e = g();
    e.glSchriftName = familie;
    e.glSchriftGroesse = punkte;
    einstellungenSpeichern();
    schriftenNeuBauen();
}

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

void feldInhalt(Feld f);
void gruppenInhalt(int breite, int hoehe);

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
    case Feld::Gruppe:   // der eingebaute Gruppendialog (m_hSplit->addWidget( GroupDialog_getWindow() ))
        gruppenInhalt(static_cast<int>(std::lround(ImGui::GetWindowWidth() / g_s)),
                      static_cast<int>(std::lround(ImGui::GetWindowHeight() / g_s)));
        break;
    }
}

bool istXyFeld(Feld f) { return f == Feld::XY || f == Feld::XZ || f == Feld::YZ; }

// --- "Rahmen + Abstand" (Stufe 3 Punkt 0b, Entscheidung shank) ---------------
//
// "Es sollte dem Original treu sein, nur halt bisschen besser erkennbar und
// sauberer": jedes Feld bekommt einen Rahmen von einem Bildpunkt in einem Ton
// zwischen Hintergrund und Schrift des Farbschemas (dunkel: etwas heller,
// hell: etwas dunkler), ohne Rundung, ohne Schatten und ohne Polster - er
// liegt auf der Innenkante des Feldes, Lagen und Groessen bleiben die von NRC.
// Der Abstand dazwischen ist NRCs Griff (PM_SplitterWidth 4) mit seinen
// Punkten. Das aktive Feld (zuletzt angeklickt) hat den Rahmen in der
// Akzentfarbe des Farbschemas - so wie NRC die aktive 2D-Ansicht mit einem
// farbigen Umriss von einem Bildpunkt zeigt (XYWnd overlayDraw,
// "ShowWindowOutline"; bei den 2D-Ansichten gilt dieser Schalter weiter).
ImU32 rahmenFarbe(bool aktiv) {
    const theme::Theme* th = g_app->activeTheme;
    if (th == nullptr) { return ImGui::GetColorU32(aktiv ? ImGuiCol_CheckMark : ImGuiCol_Border); }
    if (aktiv) { return ImGui::ColorConvertFloat4ToU32(ImVec4{th->accent.r, th->accent.g, th->accent.b, 1.0F}); }
    const float k = 0.18F;
    return ImGui::ColorConvertFloat4ToU32(ImVec4{th->window.r + (th->text.r - th->window.r) * k, th->window.g + (th->text.g - th->window.g) * k,
                                                 th->window.b + (th->text.b - th->window.b) * k, 1.0F});
}

// Am Ende des Fensters eines Feldes: aktiv werden (Klick mit einer Maustaste)
// und den Rahmen ueber den Inhalt legen.
void feldRahmen(Feld f) {
    Editor& ed = editor();
    const ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
        (io.MouseClicked[0] || io.MouseClicked[1] || io.MouseClicked[2])) {
        ed.aktivesFeld = static_cast<int>(f);
    }
    const bool aktiv = ed.aktivesFeld == static_cast<int>(f) && (!istXyFeld(f) || ed.anzeige.umriss);
    // InnerRect: bei den schwebenden Fenstern (Floating) ohne die Titelleiste
    const ImRect innen = ImGui::GetCurrentWindow()->InnerRect;
    const ImVec2 a{std::round(innen.Min.x), std::round(innen.Min.y)};
    const ImVec2 b{std::round(innen.Max.x), std::round(innen.Max.y)};
    const ImU32 farbe = rahmenFarbe(aktiv);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(a, b, false);
    dl->AddRect(ImVec2{a.x + 0.5F, a.y + 0.5F}, ImVec2{b.x - 0.5F, b.y - 0.5F}, farbe, 0.0F, 0, 1.0F);
    dl->PopClipRect();
    ed.rahmenImBild.push_back({feldKurz(f), a.x, a.y, b.x, b.y, farbe, aktiv});
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
    feldRahmen(f);
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
    // MainFrame::Create: Floating zeigt den Gruppendialog (GroupDialog_show),
    // eingebaut steht er als Kind von m_hSplit da (QSplitter::addWidget
    // zeigt ihn); sonst bleibt er versteckt, bis N ihn holt.
    ed.gruppenDialogOffen = ed.anordnung == Anordnung::Floating || ed.gruppenDialogEingebaut;
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
    // Wechselt die aktive 2D-Ansicht (Klick, NextView, Anordnung), wird sie das
    // aktive Feld.
    if (ed.aktivXy != ed.aktivXyVorher) {
        ed.aktivXyVorher = ed.aktivXy;
        static const Feld kXy[3] = {Feld::XY, Feld::XZ, Feld::YZ};
        ed.aktivesFeld = static_cast<int>(kXy[std::clamp(ed.aktivXy, 0, 2)]);
    }
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
        if (ed.gruppenDialogEingebaut) {
            kinder.push_back(gruppe);
            ed.hSplit.kinder.back().versteckt = !ed.gruppenDialogOffen;   // GroupDialog_setShown
        }
        teilerZeichnen("h", ed.hSplit, false, x, y, b, h, kinder);
    } else if (ed.anordnung == Anordnung::Split) {
        // 4 way (mainframe.cpp:1829-1855): links Kamera ueber YZ, rechts XY ueber XZ
        std::vector<Inhalt> kinder{spalte(ed.vSplit, "v", {Feld::Kamera, Feld::YZ}),
                                   spalte(ed.vSplit2, "v2", {Feld::XY, Feld::XZ})};
        if (ed.gruppenDialogEingebaut) {
            kinder.push_back(gruppe);
            ed.hSplit.kinder.back().versteckt = !ed.gruppenDialogOffen;
        }
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
        feldRahmen(f);
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
    if (ed.gruppenDialogEingebaut && ed.anordnung != Anordnung::Floating) { return; }   // steht in m_hSplit
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
    (void)schwebendeSeiten;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool sichtbar = ImGui::Begin(titel.c_str(), &ed.gruppenDialogOffen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
                                                                                   ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    if (sichtbar) {
        const ImVec2 gr = ImGui::GetContentRegionAvail();
        ImGui::BeginChild("##gruppeinnen", gr, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        gruppenInhalt(static_cast<int>(std::lround(gr.x / g_s)), static_cast<int>(std::lround(gr.y / g_s)));
        ImGui::EndChild();
    }
    ImGui::End();
}

// Der Inhalt des Gruppendialogs (GroupDlg::Create): QTabWidget mit den
// Reitern unten (TabPosition::South), Seiten Entities, (Console, Textures),
// Models, Layers. Masse wie Qt 5.15 (Fusion): Reiter fm.height() 13 + 12 =
// 25 hoch und Text + 24 breit (QTabBar::tabSizeHint), linksbuendig; die
// Seite (SE_TabWidgetTabPane) reicht 2 in die Reiter hinein, ihr Inhalt
// (SE_TabWidgetTabContents) liegt 2 innen. Foto dlg_View_Entity_Inspector_0.png.
void gruppenInhalt(int breite, int hoehe) {
    Editor& ed = editor();
    using befehle::Anordnung;
    const bool schwebendeSeiten = ed.anordnung == Anordnung::Floating || ed.anordnung == Anordnung::Split;
    const ImVec2 alt = g_ursprung;
    g_ursprung = ImGui::GetWindowPos();
    const char* seitenTitel[5] = {trK(Str::KePageEntities), trK(Str::KePageConsole), trK(Str::KePageTextures),
                                  trK(Str::KePageModels), trK(Str::KePageLayers)};
    std::vector<int> seiten;
    for (int s = 0; s < 5; ++s) {
        if ((s == 1 || s == 2) && !schwebendeSeiten && !(s == 2 && ed.gruppenDialogEingebaut)) { continue; }
        seiten.push_back(s);
    }
    if (std::find(seiten.begin(), seiten.end(), ed.gruppenSeite) == seiten.end()) { ed.gruppenSeite = 0; }
    ed.gruppenSeiteWechseln = false;
    constexpr int kReiterH = 25;
    const int seiteH = std::max(0, hoehe - kReiterH + 2);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // Reiter
    dialogSchriftAn();
    int x = 0;
    for (const int s : seiten) {
        const int w = static_cast<int>(std::ceil(ImGui::CalcTextSize(seitenTitel[s]).x / g_s)) + 24;
        const bool aktiv = s == ed.gruppenSeite;
        merkeFeld(std::string("Gruppe:Reiter:") + std::to_string(s), x, hoehe - kReiterH, w, kReiterH);
        // CE_TabBarTabShape (Fusion, gedreht fuer South): der aktive 2 unter
        // der Oberkante, die uebrigen 3 darunter und 2 kuerzer
        const ImVec2 a = bildpunkt(x, hoehe - kReiterH + (aktiv ? 0 : 1));
        const ImVec2 c = bildpunkt(x + w, hoehe - (aktiv ? 0 : 2));
        ImGui::SetCursorScreenPos(bildpunkt(x, hoehe - kReiterH));
        if (ImGui::InvisibleButton((std::string("##gruppenreiter") + std::to_string(s)).c_str(),
                                   ImVec2{std::round(static_cast<float>(w) * g_s), std::round(static_cast<float>(kReiterH) * g_s)})) {
            ed.gruppenSeite = s;   // QTabWidget::currentChanged -> GroupDialog_updatePageTitle
        }
        const ImU32 farbe = aktiv ? ImGui::GetColorU32(ImGuiCol_WindowBg)
                                  : ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_TabHovered : ImGuiCol_Tab);
        dl->AddRectFilled(a, c, farbe, 2.0F * g_s, ImDrawFlags_RoundCornersBottom);
        // ImGui 1.92.8: AddRect( ..., rounding, thickness, flags ) - mit nur
        // fuenf Werten kamen die Flags als Dicke 192 an.
        dl->AddRect(a, c, ImGui::GetColorU32(ImGuiCol_Border), 2.0F * g_s, 1.0F, ImDrawFlags_RoundCornersBottom);
        const ImVec2 t = ImGui::CalcTextSize(seitenTitel[s]);
        dl->AddText(ImVec2{std::floor((a.x + c.x - t.x) * 0.5F), std::floor((a.y + c.y - t.y) * 0.5F)}, ImGui::GetColorU32(ImGuiCol_Text),
                    seitenTitel[s]);
        x += w;
    }
    dialogSchriftAus();
    // Seite (PE_FrameTabWidget) und Inhalt
    merkeFeld("Gruppe:Seite", 0, 0, breite, seiteH);
    dl->AddRect(bildpunkt(0, 0), bildpunkt(breite, seiteH), ImGui::GetColorU32(ImGuiCol_Border));
    const int ib = std::max(0, breite - 4);
    const int ih = std::max(0, seiteH - 4);
    merkeFeld("Gruppe:Inhalt", 2, 2, ib, ih);
    if (ib > 0 && ih > 0) {
        ImGui::SetCursorScreenPos(bildpunkt(2, 2));
        const ImVec2 p0 = bildpunkt(2, 2);
        const ImVec2 p1 = bildpunkt(2 + ib, 2 + ih);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
        ImGui::BeginChild((std::string("##gruppenseite") + std::to_string(ed.gruppenSeite)).c_str(), ImVec2{p1.x - p0.x, p1.y - p0.y},
                          ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar();
        if (ed.gruppenSeite == 0) {
            entitaetenSeiteZeichnen(ib, ih);
        } else if (ed.gruppenSeite == 1) {
            konsoleZeichnen();
        } else if (ed.gruppenSeite == 2) {
            feldInhalt(Feld::Texturen);
        } else if (ed.gruppenSeite == 3) {
            modellBrowserZeichnen(ib, ih);   // ModelBrowser_constructWindow
        } else if (ed.gruppenSeite == 4) {
            ebenenSeiteZeichnen(ib, ih);   // LayersBrowser_constructWindow
        } else {
            char z[120];
            std::snprintf(z, sizeof(z), tr(Str::KeStufe), 2);
            ImGui::TextDisabled("%s", z);
        }
        ImGui::EndChild();
    }
    g_ursprung = alt;
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

// Die Listen eines Bildes (Knoepfe, Felder, Rahmen) einmal je Bild leeren -
// die Menueleiste steht seit Stufe 3 Punkt 0b im Hauptfenster und wird vor
// dem Editor gezeichnet.
void bildBeginnen() {
    Editor& e = g();
    if (e.listenBild == ImGui::GetFrameCount()) { return; }
    e.listenBild = ImGui::GetFrameCount();
    e.knoepfeImBild.clear();
    e.knoepfeVerdeckt.clear();
    e.felderImBild.clear();
    e.klappOrte.clear();
    e.rahmenImBild.clear();
}

namespace {
// NRCs Schrift: Segoe UI 9 pt (Menues und Statuszeile von Qt unter Windows) =
// 12 Punkte Geviert, mal Massstab. ImGui multipliziert die Grundgroesse noch
// mit FontScaleDpi und FontScaleMain.
float schriftGroesse() { return static_cast<float>(mass::kSchrift) * g_s / std::max(0.01F, g_app->dpiScale * g_app->uiScale); }
}  // namespace

// --- Eine Leiste, die umschaltet (Stufe 3 Punkt 0b, Entscheidung shank) -----
//
// Im Modus "Karten-Editor" ist die Menueleiste des Programmfensters selbst die
// von NRC (create_main_menu: File Edit View Modify Build Grid Misc Entity
// Brush Curve Plugins Help); eine zweite Leiste im Editor gibt es nicht mehr.
// Damit sie so hoch ist wie NRCs QMenuBar (21 Punkte), stehen Schrift und
// FramePadding schon beim Begin des Hauptfensters (ImGui rechnet dort
// MenuBarHeight = Schrift + 2 x FramePadding.y).
//
// Stufe 7 Punkt 6b: im Stil des Programms (Editor::menueNrcStil aus, Vorgabe)
// bleibt die Leiste so, wie das Programm sie zeichnet - Schrift und Hoehe des
// Programms; der Editor beginnt trotzdem direkt darunter (Editor::leisteUnten).
namespace {
bool g_hauptleisteNrc = false;   // was hauptleisteStilAn geschoben hat
}
void hauptleisteStilAn() {
    g_s = massstab();
    g_hauptleisteNrc = g().menueNrcStil;
    if (!g_hauptleisteNrc) { return; }
    ImGui::PushFont(g_schrift, schriftGroesse());
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, static_cast<float>(mass::kMenueH - mass::kSchrift) * 0.5F * g_s});
}
void hauptleisteStilAus() {
    if (!g_hauptleisteNrc) { return; }
    ImGui::PopStyleVar();
    ImGui::PopFont();
}

// Die Menueleiste von NRC in der Leiste des Programmfensters (zwischen dessen
// Begin und dem Editor). Lagen in logischen Punkten ab der linken oberen Ecke
// der Leiste.
void zeichneHauptmenue() {
    bildBeginnen();
    g_s = massstab();
    const bool nrc = g().menueNrcStil;   // Stufe 7 Punkt 6b: sonst Schrift und Abstaende des Programms
    if (nrc) {
        ImGui::PushFont(g_schrift, schriftGroesse());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, static_cast<float>(mass::kMenueH - mass::kSchrift) * 0.5F * g_s});
    }
    const ImVec2 alt = g_ursprung;
    g_ursprung = ImGui::GetWindowPos();
    zeichneMenueleiste();
    const ImGuiWindow* w = ImGui::GetCurrentWindow();
    g().leisteUnten = w->Pos.y + w->TitleBarHeight + w->MenuBarHeight;
    g_ursprung = alt;
    if (nrc) {
        ImGui::PopStyleVar(2);
        ImGui::PopFont();
    }
}

void zeichne(float hoehe) {
    Editor& e = g();
    const auto zeichenStart = std::chrono::steady_clock::now();
    kartegpu::aufraeumen();   // Bilder, die ein frueheres Bild freigab
    e.gezeichnet = true;
    bildBeginnen();
    // Stufe 7 Punkt 6f: erst die Einstellungen (gamefile, GameMode, EnginePath,
    // ExtraResourcePath), dann das Spiel - wie NRC (Preferences vor
    // Radiant_Initialise und dem VFS, main.cpp). Vorher lud das erste Bild das
    // Spiel mit den Vorgaben, die letzte bzw. leere Karte bekam dieses, und das
    // naechste Bild lud es mit den gespeicherten Ordnern noch einmal.
    if (!e.belegungGeladen) { einstellungenLaden(); }
    spielPruefen();
    npcModelleVorab();   // Stufe 7: die NPC-Dateien des Optimierers im Faden, nicht beim ersten Oeffnen
    echtLichtSchritt(geometrieStand());   // Stufe 7 Punkt 5b: Schnell-Backen nach einer Pause ohne Aenderung
    letzteKarteBeimStart();
    enginePfadPruefen();   // EnginePath_verify (MainFrame_Construct)
    if (!e.spielFrageGezeigt) {   // CGameDialog::Init: mit m_bGamePrompt die "Global Preferences" beim Start
        e.spielFrageGezeigt = true;
        if (e.spielFrage) { e.globaleEinstellungenOffen = true; }
    }
    zusatztastenPruefen();   // g_modifiers (onModifierDown / onModifierUp)
    texturenAktualisieren();   // g_ActiveShaders: Shader der Karte, in Gebrauch
    filterPruefen();           // Filterable::updateFiltered (Node::eFiltered, Face::m_filtered)
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
    ImGui::PushFont(g_schrift, schriftGroesse());
    // Direkt unter der Menueleiste des Programmfensters (ohne den Zellrand der
    // Tabelle dazwischen).
    if (e.leisteUnten > 0.0F) { ImGui::SetCursorScreenPos(ImVec2{ImGui::GetCursorScreenPos().x, e.leisteUnten}); }
    // hoehe <= 0 wie bei BeginChild: der restliche Platz minus |hoehe|
    // (app.cpp gibt frameH = -fussH).
    const ImVec2 rest = ImGui::GetContentRegionAvail();
    ImVec2 platz{rest.x, hoehe > 0.0F ? hoehe : std::max(1.0F, rest.y + hoehe)};
    // Der Bereich wie NRCs Client: die Menueleiste (21) steht darueber im
    // Programmfenster, der Editor beginnt mit der Werkzeugleiste.
    if (bereichErsatzB > 0 && bereichErsatzH > 0) {
        platz = ImVec2{static_cast<float>(bereichErsatzB) * g_s, static_cast<float>(bereichErsatzH - mass::kMenueH) * g_s};
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    ImGui::BeginChild("##karteneditor", platz, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(2);
    // Lagen wie bisher ab der Oberkante der Menueleiste: der Editor beginnt
    // bei y = 21 (mass::kMenueH), direkt unter der Leiste.
    g_ursprung = ImVec2{ImGui::GetWindowPos().x, ImGui::GetWindowPos().y - std::round(static_cast<float>(mass::kMenueH) * g_s)};
    e.editorOben = ImGui::GetWindowPos().y;
    const int breite = static_cast<int>(std::floor(platz.x / g_s + 0.001F));
    const int hoeheL = static_cast<int>(std::floor(platz.y / g_s + 0.001F)) + mass::kMenueH;
    merkeFeld("Bereich", 0, 0, breite, hoeheL);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    int y = mass::kMenueH;
    if (e.hauptleiste) {
        zeichneHauptleiste(y, breite);
        y += mass::leisteH();
    }
    const int statusY = hoeheL - mass::kStatusH;
    const int mitteH = std::max(0, statusY - y);
    // Filterleiste rechts angedockt (Qt::RightToolBarArea), daneben die Mitte.
    const int filterB = (e.filterleiste || e.pluginleiste) ? mass::leisteH() : 0;
    const int mitteB = std::max(0, breite - filterB);
    // Lage der Maus ueber m_hSplit (fuer MaximizeView).
    const ImVec2 maus = ImGui::GetIO().MousePos;
    e.mausX = static_cast<int>(std::floor((maus.x - bildpunkt(0, y).x) / g_s));
    e.mausY = static_cast<int>(std::floor((maus.y - bildpunkt(0, y).y) / g_s));
    const auto ansichtenStart = std::chrono::steady_clock::now();
    anordnungZeichnen(0, y, mitteB, mitteH);
    e.ansichtenMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - ansichtenStart).count();
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
    entitaetenDialogeZeichnen();
    zeichneEinstellungen();
    zeichneTastenDialog();
    zeichneCsgWerkzeug();
    zeichneSeitenDialog();
    zeichnePinselSuche();
    zeichneKartenInfo();
    zeichneEbenenMenue();
    zeichneQtEingabe();
    zeichneArbitraerDialoge();
    zeichnePatchDialog();
    zeichneSurfaceInspector();
    zeichneBspDialog();   // Stufe 4: File > Open .bsp...
    zeichneOptimierer();  // Stufe 5: Build > Optimizer...
    pluginDialogeZeichnen();   // Stufe 4 Punkt 3: die Plugins
    zeichneGlobaleEinstellungen();
    zeichneAbrisse();
    zeichneUeber();
    zeichneEnginePfadDialog();
    zeichneSchriftDialog();
    bauDialogZeichnen();   // Stufe 3: "Build Menu" (DoBuildMenu)
    bauRueckmeldungenZeichnen();   // Stufe 3: "Q3Map debug window"
    bauFortschrittZeichnen();      // Stufe 8a: Baufortschritt mit Restzeit
    bspRueckSchritt();             // Stufe 8a: misc_model aus der .bsp zurueckgewinnen
    zusammenDialogZeichnen();      // Stufe 8a: Zur fertigen Karte hinzufuegen
    portalDialogeZeichnen();       // Stufe 3 Punkt 5: prtview ("Load .prt", "Portal Viewer Configuration")
    // PreferencesDialog_showDialog (preferences.cpp:783-797): nach OK die Liste der
    // Einstellungen, die einen Neustart brauchen
    if (!e.neustartFragen.empty() && !e.einstellungenOffen && !e.frageOffen && !e.meldung.offen) {
        std::string text = "Preference changes require a restart:\n\n";
        for (const std::string& n : e.neustartFragen) { text += n + "\n"; }
        e.neustartFragen.clear();
        text += "\nRestart now?";
        qtMeldung("Restart is required", text, MeldungArt::Frage, true, [](bool ja) {
            if (!ja) { return; }
            Editor& ed = g();
            if (!ed.spielDateiNeu.empty()) {   // m_sGameFile: der neue Wert gilt nach dem Start
                ed.spielDatei = ed.spielDateiNeu;
                ed.spielDateiNeu.clear();
            }
            radiantNeustart();
        });
    }
    zeichneQtMeldung();
    zeichneRueckfrage();
    ImGui::PopFont();
    tastenVerarbeiten();
    e.zeichenMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - zeichenStart).count();
}
}  // namespace bhed::gui::karteneditor

namespace bhed::gui {
// Aus buildFonts (main_win32.cpp): NRCs Schrift fuer den Karten-Editor.
void setKartenSchrift(ImFont* schrift) { karteneditor::setzeSchrift(schrift); }

// buildFonts: die Datei der OpenGL-Schrift ("" = keine eigene) und die geladene Schrift
std::string kartenGlSchriftDatei() {
    if (g_app == nullptr) { return {}; }
    const auto it = g_app->settings.karte.find("OpenGLFont");
    const std::string name = it != g_app->settings.karte.end() ? it->second : std::string("Myriad Pro");
    return platform::schriftDatei(name, false, false);
}

void setKartenGlSchrift(ImFont* schrift, const std::string& datei) {
    karteneditor::g_glSchrift = schrift;
    karteneditor::editor().glSchriftGeladen = schrift != nullptr ? datei : std::string();
    karteneditor::editor().glSchriftVerhaeltnis = schrift != nullptr ? karteneditor::schriftVerhaeltnis(datei) : 1.33F;
}
}  // namespace bhed::gui
