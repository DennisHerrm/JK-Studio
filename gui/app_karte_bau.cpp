// app_karte_bau.cpp - Karten-Editor, Stufe 3: Karten bauen.
//
// Das Build-Menue (Eintraege aus der build_menu.xml des Spiels, Tooltip mit
// den Befehlen), "Run recent build" (F5), der Dialog "Build Menu" (Customize):
// Liste der Eintraege und der Befehle mit Bearbeiten an Ort und Stelle,
// Entfernen, Kopieren/Einfuegen, Ziehen (mit Strg kopieren), die Variablen
// (Konstanten zum Kopieren, Werkzeuge zum Bearbeiten), OK / Cancel / Reset.
// Die Variablen wie build_init_variables, der Ablauf vor dem Bauen wie RunBSP.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/build.cpp
// (Build_constructMenu, Build_refreshMenu, BSPCommandList_Construct,
// project_cell_edited, project_selection_changed, commands_cell_edited,
// QTreeWidget_drag, QTreeWidget_project, QTreeWidget_commands,
// CustomDelegate0/1, BuildMenuDialog_construct, DoBuildMenu, LoadBuildMenu,
// SaveBuildMenu, BuildMenu_Construct/Destroy, Build_runRecentExecutedBuild),
// radiant/qe3.cpp (build_init_variables, RunBSP), radiant/mainframe.cpp
// (create_bsp_menu: setToolTipsVisible).
//
//   Copyright (C) 2001-2006, William Joseph.
//   All Rights Reserved.
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
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
#include "karte_bau_plattform.h"
#include "platform.h"

#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/bau.h"
#include "bhed/karte/baufortschritt.h"
#include "bhed/karte/baumonitor.h"
#include "bhed/karte/bspoeffnen.h"
#include "bhed/karte/editordaten.h"
#include "bhed/karte/kartendatei.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/optimierer.h"
#include "bhed/karte/region.h"
#include "bhed/karte/zuletzt.h"
#include "bhed/pk3.h"
#include "bhed/regelwerk.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace bhed::gui::karteneditor {

namespace {

namespace fs = std::filesystem;

Editor& e() { return editor(); }

fs::path pfadAus(const std::string& p) { return fs::path(std::u8string(reinterpret_cast<const char8_t*>(p.data()), p.size())); }

std::string mitSchraegstrich(std::string p) {   // Qts Dateiwahl / NRC: '/'
    std::replace(p.begin(), p.end(), '\\', '/');
    return p;
}

bool dateiLesen(const std::string& pfad, std::string& text) {
    std::ifstream f(pfadAus(pfad), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    text = s.str();
    return true;
}

// ja.game oder jk2.game (g_pGameDescription->mGameFile), fuer den Dateinamen
std::string bauSpielKurz() { return kartenSpielNummer() == kRegelwerkJediOutcast ? "jk2" : "ja"; }

// g_buildMenuFullPah: absolut oder im Einstellungsordner des Spiels (bei NRC
// SettingsPath/<game>/; hier karte_<spiel>_<name> wie die anderen Dateien des
// Editors im Einstellungsordner des Programms).
std::string menuePfad(const std::string& name) {
    if (pfadAus(name).is_absolute()) { return name; }
    return platform::settingsDirectory() + "\\karte_" + bauSpielKurz() + "_" + name;
}

// GameToolsPath_get() + "default_build_menu.xml"
std::string standardMenuePfad() {
    return directoryOf(g_app->dataDir) + (kartenSpielNummer() == kRegelwerkJediOutcast ? "/karte/jk2" : "/karte/ja") +
           "/default_build_menu.xml";
}

bool gleichOhneFall(const std::string& a, const std::string& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
           });
}

// build_commands_parse
bool menueParsen(const std::string& pfad) {
    std::string text;
    if (!dateiLesen(pfad, text)) { return false; }
    std::vector<std::string> meldungen;
    const bool ok = karte::bauMenueLesen(text, e().bauProjekt, &meldungen);
    for (const std::string& m : meldungen) {
        if (m == "failed to parse build menu") {
            fehler("failed to parse build menu: \"" + mitSchraegstrich(pfad) + "\"");
        } else {
            fehler(m);
        }
    }
    return ok;
}

// LoadBuildMenu (build.cpp:1276-1293)
void menueLaden() {
    Editor& ed = e();
    if (ed.bauMenueName.empty() || !menueParsen(menuePfad(ed.bauMenueName))) {
        if (!gleichOhneFall(ed.bauMenueName, "build_menu.xml")) {
            ed.bauMenueName = "build_menu.xml";
            if (menueParsen(menuePfad(ed.bauMenueName))) { return; }
        }
        const std::string standard = standardMenuePfad();
        if (!menueParsen(standard)) { fehler("failed to parse default build commands: " + mitSchraegstrich(standard)); }
    }
}

}  // namespace

// --- Build-Menue ------------------------------------------------------------------

void bauMenueSicherstellen() {
    Editor& ed = e();
    const int spiel = kartenSpielNummer() == kRegelwerkJediOutcast ? kRegelwerkJediOutcast : kRegelwerkOpenJK;
    if (ed.bauProjektFuer == spiel) { return; }
    if (ed.bauProjektFuer != -2) { bauMenueSpeichern(); }   // anderes Spiel: das alte sichern
    ed.bauProjektFuer = spiel;
    ed.bauProjekt = karte::BauProjekt{};
    ed.bauZuletzt = -1;
    // BuildMenu_Construct: registerPreference( "BuildMenu" ), LoadBuildMenu
    const auto it = g_app->settings.karte.find("BuildMenu");
    ed.bauMenueName = it != g_app->settings.karte.end() ? it->second : std::string();
    menueLaden();
    g_app->settings.karte["BuildMenu"] = ed.bauMenueName;
}

void bauMenueSpeichern() {   // SaveBuildMenu
    Editor& ed = e();
    if (!ed.bauGeaendert || ed.bauProjektFuer == -2) { return; }
    ed.bauGeaendert = false;
    const std::string pfad = menuePfad(ed.bauMenueName);
    std::error_code ec;
    fs::create_directories(pfadAus(pfad).parent_path(), ec);
    // TextFileOutputStream: Textmodus (CR LF unter Windows)
    const std::string roh = karte::textmodusSchreiben(karte::bauMenueSchreiben(ed.bauProjekt));
    std::ofstream f(pfadAus(pfad), std::ios::binary | std::ios::trunc);
    if (f) { f.write(roh.data(), static_cast<std::streamsize>(roh.size())); }
    g_app->settings.karte["BuildMenu"] = ed.bauMenueName;
}

void bauMenueNeuLaden() {   // Reset: build_commands_clear(); LoadBuildMenu()
    Editor& ed = e();
    ed.bauProjekt = karte::BauProjekt{};
    menueLaden();
    if (ed.bauZuletzt >= static_cast<long long>(ed.bauProjekt.eintraege.size())) { ed.bauZuletzt = -1; }
}

std::string bauMenueDateiPfad() { return menuePfad(e().bauMenueName.empty() ? std::string("build_menu.xml") : e().bauMenueName); }

// build_init_variables (qe3.cpp:152-178)
karte::BauVariablen bauVariablen() {
    Editor& ed = e();
    karte::BauVariablen v;
    std::string programm = mitSchraegstrich(platform::executableDirectory());
    if (programm.empty() || programm.back() != '/') { programm += '/'; }
    v["RadiantPath"] = programm;   // AppPath_get
    v["ExecutableType"] = "exe";   // RADIANT_EXECUTABLE
    v["EnginePath"] = enginePfadAnzeige();
    v["UserEnginePath"] = "";      // g_qeglobals.m_userEnginePath: unter Windows nur, wenn "My Games/<prefix>" besteht - die .game haben kein prefix
    std::string zusatz;
    for (const std::string& p : ed.zusatzPfade) {
        if (!p.empty()) { zusatz += " -fs_pakpath \"" + p + "\""; }   // << " -fs_pakpath " << Quoted( path )
    }
    v["ExtraResourcePaths"] = zusatz;
    v["MonitorAddress"] = ed.bau.ueberwachen ? kBauMonitorAdresse : "";
    v["GameName"] = bauSpielName();
    const std::string karte = bauKartenPfad();
    const std::string ohneEndung = karte.substr(0, karte.rfind('.') == std::string::npos ? karte.size() : karte.rfind('.'));
    v["BspFile"] = ohneEndung + ".bsp";
    v["MapFile"] = ed.region.aktiv ? ohneEndung + ".reg" : karte;
    v["MapName"] = karte.substr(karte.find_last_of('/') == std::string::npos ? 0 : karte.find_last_of('/') + 1);
    return v;
}

// gamename_get - beim Regelwerk Movie Duels ohne gesetztes fs_game der Ordner der Mod
// (Abweichung: NRC kennt nur "fs_game"; hier sind die Spielordner des Programms
// die Vorgabe, und Movie Duels braucht -fs_game MD).
std::string bauSpielName() {
    Editor& ed = e();
    if (!ed.spielName.empty()) { return ed.spielName; }
    if (kartenSpielNummer() == kRegelwerkMovieDuels) {
        const std::string basis = kartenSpiel().basisSpiel;
        for (const std::string& o : kartenVfsOrdner()) {
            std::string n = mitSchraegstrich(o);
            while (!n.empty() && n.back() == '/') { n.pop_back(); }
            const std::string blatt = n.substr(n.find_last_of('/') == std::string::npos ? 0 : n.find_last_of('/') + 1);
            if (movieDuelsOrdner(o) && !gleichOhneFall(blatt, basis)) { return blatt; }   // ...\MD
        }
    }
    return kartenFsGame();
}

// Map_Name - die Karte, die gebaut wird (Abweichung "Ziel-Ordner": eine Kopie
// im Arbeitsordner, siehe bauKartenPfad in Punkt 4)
std::string bauKartenPfad() {
    Editor& ed = e();
    return mitSchraegstrich(ed.bauKarte.empty() || ed.bauKarteFuer != ed.name ? ed.name : ed.bauKarte);
}

// Build_constructMenu (build.cpp:1224-1248): je Eintrag ein Menuepunkt mit den
// Befehlen als Tooltip (setToolTipsVisible), je Trenner ein Trenner.
void bauMenueEintraegeZeichnen() {
    bauMenueSicherstellen();
    Editor& ed = e();
    bool letzterTrenner = true;   // der Trenner davor steht in der Tabelle
    const std::vector<karte::BauEintrag>& liste = ed.bauProjekt.eintraege;
    for (std::size_t i = 0; i < liste.size(); ++i) {
        const karte::BauEintrag& b = liste[i];
        if (karte::bauIstTrenner(b)) {
            // QMenu::separatorsCollapsible: keine zwei Trenner hintereinander, keiner am Ende
            if (!letzterTrenner && i + 1 < liste.size()) { qtTrenner(); }
            letzterTrenner = true;
            continue;
        }
        const std::string text = karte::ohneMnemonik(b.name) + "##build" + std::to_string(i);
        QtPunkt p;
        p.id = text.c_str();
        p.nrc = b.name.c_str();
        p.tooltip = karte::bauTooltip(b);
        if (qtPunkt(p)) { bauStarten(i); }   // BuildMenuItem::run -> RunBSP( m_buildIdx )
        letzterTrenner = false;
    }
    // Abweichung (Auftrag): ein laufendes Bauen abbrechen
    qtTrenner();
    const std::string abbrechen = std::string(trK(Str::KeBmAbbrechen)) + "##bauabbrechen";
    QtPunkt p;
    p.id = abbrechen.c_str();
    p.aktiv = bauLaeuft();
    if (qtPunkt(p)) { bauAbbrechen(); }
    // Stufe 4 Punkt 2 (Abweichung, eigener Eintrag): nur die Entities einer
    // geoeffneten .bsp aktualisieren - Geometrie und Licht bleiben
    const std::string nurEnt = std::string(trK(Str::KeBmNurEntities)) + "##baunurentities";
    QtPunkt n;
    n.id = nurEnt.c_str();
    n.aktiv = bspNurEntitaetenMoeglich() && !bauLaeuft();
    n.tooltip = bspNurEntitaetenMoeglich() ? trK(Str::KeBmNurEntitiesTip) : trK(Str::KeBmNurEntitiesAus);
    if (qtPunkt(n)) { bspNurEntitaeten(); }
    // Stufe 5 (Abweichung, eigene Funktion - NRC hat keinen Optimierer): die
    // Grenzen von JKA / JK2 / Movie Duels zaehlen, Bauprobleme finden und beheben
    const std::string opt = std::string(trK(Str::KeOptMenue)) + "##bauoptimierer";
    QtPunkt o;
    o.id = opt.c_str();
    o.tooltip = trK(Str::KeOptMenueTip);
    if (qtPunkt(o)) { optimiererOeffnen(); }
    // Stufe 7 (Abweichung, eigene Funktionen - nicht in NRC): Echtzeit-Licht in der
    // Kamera (Vorschau) und Schnell-Backen im Hintergrund
    const std::string licht = std::string(trK(Str::KeLichtMenue)) + "##bauechtlicht";
    QtPunkt l;
    l.id = licht.c_str();
    l.art = QtArt::Pruefbar;
    l.haken = ed.echtLicht.an;
    l.tooltip = trK(Str::KeLichtMenueTip);
    if (qtPunkt(l)) { ed.echtLicht.an = !ed.echtLicht.an; }
    const std::string backen = std::string(trK(Str::KeBackenMenue)) + "##bauschnellbacken";
    QtPunkt b;
    b.id = backen.c_str();
    b.art = QtArt::Pruefbar;
    b.haken = ed.echtLicht.backen;
    b.tooltip = trK(Str::KeBackenMenueTip);
    if (qtPunkt(b)) { ed.echtLicht.backen = !ed.echtLicht.backen; }
    // Stufe 8a Punkt 2 (Abweichung, eigene Funktion): Editor-Daten behalten
    const std::string edDaten = std::string(trK(Str::KeEdMenue)) + "##baueditordaten";
    QtPunkt ed2;
    ed2.id = edDaten.c_str();
    ed2.art = QtArt::Pruefbar;
    ed2.haken = ed.bau.editorDaten;
    ed2.tooltip = trK(Str::KeEdMenueTip);
    if (qtPunkt(ed2)) {
        ed.bau.editorDaten = !ed.bau.editorDaten;
        einstellungenSpeichern();
    }
    // Stufe 8a Punkt 5a (Abweichung, eigene Funktion): Teil-Kompilierung - VIS wiederverwenden
    const std::string visW = std::string(trK(Str::KeTkVis)) + "##bauviswieder";
    QtPunkt vw;
    vw.id = visW.c_str();
    vw.art = QtArt::Pruefbar;
    vw.haken = ed.bau.visWieder;
    vw.tooltip = trK(Str::KeTkVisTip);
    if (qtPunkt(vw)) {
        ed.bau.visWieder = !ed.bau.visWieder;
        einstellungenSpeichern();
    }
    const std::string lichtW = std::string(trK(Str::KeTkLicht)) + "##baulichtwieder";
    QtPunkt lw;
    lw.id = lichtW.c_str();
    lw.art = QtArt::Pruefbar;
    lw.haken = ed.bau.lichtWieder;
    lw.tooltip = trK(Str::KeTkLichtTip);
    if (qtPunkt(lw)) {
        ed.bau.lichtWieder = !ed.bau.lichtWieder;
        einstellungenSpeichern();
    }
    // Stufe 8g (Abweichung, eigene Funktion): die ganze Lichtrechnung auf der GPU (-bhedgpubake)
    const std::string gpu = std::string(trK(Str::KeGpuMenue)) + "##baugpubacken";
    std::string gpuTip = trK(Str::KeGpuMenueTip);
    if (!ed.bau.gpuBericht.empty()) {
        char z[1024];
        std::snprintf(z, sizeof(z), trK(Str::KeGpuMenueTipBericht), ed.bau.gpuBericht.c_str());
        gpuTip += std::string("\n\n") + z;
    }
    // Stufe 8d Punkt 1b: ein Untermenue Auto / Ein / Aus (Vorgabe Auto, kGpuBackenVorgabe)
    if (!ed.bau.gpuWahlText.empty()) { gpuTip += "\n\n" + ed.bau.gpuWahlText; }
    if (qtMenueAnfang(gpu.c_str())) {
        const struct {
            karte::GpuBacken wert;
            Str text;
            const char* id;
        } werte[] = {{karte::GpuBacken::Auto, Str::KeGpuAuto, "##baugpuauto"},
                     {karte::GpuBacken::Ein, Str::KeGpuEin, "##baugpuein"},
                     {karte::GpuBacken::Aus, Str::KeGpuAus, "##baugpuaus"}};
        for (const auto& w : werte) {
            const std::string id = std::string(trK(w.text)) + w.id;
            QtPunkt gp;
            gp.id = id.c_str();
            gp.art = QtArt::Exklusiv;
            gp.haken = ed.bau.gpuBacken == w.wert;
            gp.tooltip = w.wert == karte::GpuBacken::Auto ? std::string(trK(Str::KeGpuAutoTip)) : gpuTip;
            if (qtPunkt(gp)) {
                ed.bau.gpuBacken = w.wert;
                einstellungenSpeichern();
            }
        }
        qtMenueEnde();
    }
    // Stufe 8a Punkt 5c (Abweichung, eigene Funktion): Zur fertigen Karte hinzufuegen (-mergebsp)
    const std::string zf = std::string(trK(Str::KeZfMenue)) + "##bauzusammen";
    QtPunkt zp;
    zp.id = zf.c_str();
    zp.aktiv = !bauLaeuft();
    zp.tooltip = trK(Str::KeZfMenueTip);
    if (qtPunkt(zp)) { zusammenDialogOeffnen(); }
    // Stufe 8c (Abweichung, eigene Funktion): die gebaute Karte als .pk3 neben die .bsp
    const std::string pk = std::string(trK(Str::KePkMenue)) + "##baupk3";
    QtPunkt pp;
    pp.id = pk.c_str();
    pp.aktiv = !bauLaeuft();
    pp.tooltip = trK(Str::KePkMenueTip);
    if (qtPunkt(pp)) { (void)karteAlsPk3(); }
    // Stufe 8a Punkt 1 (Abweichung, eigene Funktion): melden, wenn ein Bau fertig ist
    const std::string melden = std::string(trK(Str::KeFsMelden)) + "##baumelden";
    QtPunkt m;
    m.id = melden.c_str();
    m.art = QtArt::Pruefbar;
    m.haken = ed.bau.melden;
    m.tooltip = trK(Str::KeFsMeldenTip);
    if (qtPunkt(m)) {
        ed.bau.melden = !ed.bau.melden;
        einstellungenSpeichern();
    }
}

// Build_runRecentExecutedBuild (build.cpp:1311-1314): der zuletzt gebaute
// Eintrag, sonst der erste (Project_find faellt auf 0 zurueck).
void bauZuletztStarten() {
    bauMenueSicherstellen();
    Editor& ed = e();
    if (ed.bauProjekt.eintraege.empty()) { return; }
    const long long i = ed.bauZuletzt;
    bauStarten(i >= 0 && i < static_cast<long long>(ed.bauProjekt.eintraege.size()) ? static_cast<std::size_t>(i) : 0U);
}

// --- Dialog "Build Menu" (BuildMenuDialog_construct, DoBuildMenu) ---------------

namespace {

constexpr int kRand = 9;          // QGridLayout eines Kind-Widgets: PM_DefaultChildMargin
constexpr int kAbstand = 6;       // PM_DefaultLayoutSpacing
constexpr int kKnopfB = 80;
constexpr int kKnopfH = 22;
constexpr int kRahmenOben = 17;   // QGroupBox: Titel (fm 13) + 3 + 1
constexpr int kInhaltOben = 28;
constexpr int kInhaltRand = 12;
constexpr int kZeile = 16;        // Segoe UI 9 pt: sizeHintForRow
constexpr int kRollbalken = 7;    // dark.qss
constexpr int kHaken = 14;
constexpr int kKopfH = 41;        // flache, ankreuzbare QGroupBox ohne Inhalt (Foto: 450..491)
constexpr const char* kLetzte = "+";   // LAST_ITER_STRING

enum Liste { kKeine = 0, kEintraege = 1, kBefehle = 2, kKonstanten = 3, kWerkzeuge = 4 };

struct BauDialog {
    bool offen = false;
    karte::BauProjekt sicherung;   // bakProj, bakTools
    long long sicherungZuletzt = -1;
    bool eintraegeGeaendert = false;   // projectList.m_changed
    bool werkzeugeGeaendert = false;   // g_tools_changed
    bool befehleGeaendert = false;     // g_build_changed (commands_cell_edited)
    int aktuell = -1;          // currentItem der Eintragsliste (Index; == Anzahl: "+")
    int befehlAktuell = -1;
    int werkzeugAktuell = -1;
    int fokus = kKeine;
    // Bearbeiten an Ort und Stelle (QLineEdit des Delegaten)
    int bearbeitet = kKeine;
    int bearbeitetZeile = -1;
    int bearbeitetSpalte = 0;
    std::string puffer;
    bool bearbeitenFokus = false;
    int bearbeitenBild = -1;       // das Bild, in dem das Bearbeiten begann (der Doppelklick)
    // Ctrl+C / Ctrl+V (m_buildpair_copied, m_buildcommand_copied)
    karte::BauEintrag kopiert;
    std::string kopierterBefehl;
    // Ziehen (QAbstractItemView::InternalMove, mit Strg DragDrop = kopieren)
    int ziehListe = kKeine;
    int ziehVon = -1;
    bool zieht = false;
    ImVec2 ziehStart{0.0F, 0.0F};
    bool variablenOffen = false;   // die ankreuzbare Gruppe "Build Variables"
    float rollen[2] = {0.0F, 0.0F};
    double kopiertBis = 0.0;       // QToolTip "Copied to clipboard." (666 ms)
    ImVec2 kopiertAn{0.0F, 0.0F};
    int ergebnis = 0;              // 1 OK, 2 Cancel, 3 Reset (eIDOK, eIDCANCEL, eIDNO)
    // fuer den Selbsttest: Lage der Listen und Tabellen im letzten Bild (Bildpunkte)
    ImVec2 listeO[4] = {};
    float listeB[4] = {};
    float spalte0[4] = {};
    float zeileH[4] = {};
};
BauDialog g_bd;

float g_sk = 1.0F;
ImVec2 g_o{0.0F, 0.0F};
ImVec2 bp(float x, float y) { return ImVec2{std::round(g_o.x + x * g_sk), std::round(g_o.y + y * g_sk)}; }
float px(float l) { return std::round(l * g_sk); }
ImU32 farbe(ImGuiCol c) { return ImGui::GetColorU32(c); }

std::vector<karte::BauEintrag>& eintraege() { return e().bauProjekt.eintraege; }

bool strg() { return ImGui::GetIO().KeyCtrl || ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl); }

// Gruppe wie QGroupBox: Titel (Tahoma 8 pt) und Rahmen ab kRahmenOben
void gruppe(const char* titel, int x, int y, int b, int h) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dialogSchriftAn();
    dl->AddText(bp(static_cast<float>(x + 2), static_cast<float>(y)), farbe(ImGuiCol_Text), titel);
    dialogSchriftAus();
    dl->AddRect(bp(static_cast<float>(x), static_cast<float>(y + kRahmenOben)), bp(static_cast<float>(x + b), static_cast<float>(y + h)),
                farbe(ImGuiCol_Border), ImGui::GetStyle().FrameRounding);
}

// project_cell_edited (build.cpp:678-707)
void eintragBearbeitet(int zeile, const std::string& text) {
    BauDialog& d = g_bd;
    auto& liste = eintraege();
    if (zeile >= 0 && zeile < static_cast<int>(liste.size())) {   // edit
        d.eintraegeGeaendert = true;
        if (text.empty()) {   // empty = delete
            liste.erase(liste.begin() + zeile);
            d.aktuell = std::min(zeile, static_cast<int>(liste.size()));
        } else {
            liste[static_cast<std::size_t>(zeile)].name = text;
        }
    } else if (!text.empty() && text != kLetzte) {   // add new
        d.eintraegeGeaendert = true;
        liste.push_back(karte::BauEintrag{text, {}});
        d.aktuell = static_cast<int>(liste.size()) - 1;   // currentItemChanged( item ): Befehle neu
        d.befehlAktuell = -1;
    }
}

// commands_cell_edited (build.cpp:738-764)
void befehlBearbeitet(int zeile, const std::string& text) {
    BauDialog& d = g_bd;
    auto& liste = eintraege();
    if (d.aktuell < 0 || d.aktuell >= static_cast<int>(liste.size())) { return; }   // g_current_build == 0
    std::vector<std::string>& b = liste[static_cast<std::size_t>(d.aktuell)].befehle;
    if (zeile >= 0 && zeile < static_cast<int>(b.size())) {   // edit
        d.befehleGeaendert = true;
        e().bauGeaendert = true;
        if (text.empty()) {
            b.erase(b.begin() + zeile);
        } else {
            b[static_cast<std::size_t>(zeile)] = text;
        }
    } else if (!text.empty() && text != kLetzte) {   // add new
        d.befehleGeaendert = true;
        e().bauGeaendert = true;
        b.push_back(text);
    }
}

// CustomDelegate0::setModelData: ein neues Werkzeug ("+") oder ein neuer Name
void werkzeugNameBearbeitet(const std::string& alt, const std::string& neu) {
    auto& w = e().bauProjekt.werkzeuge;
    if (neu == kLetzte || neu.empty() || w.count(neu) != 0U) { return; }   // nur etwas Neues
    if (alt == kLetzte) {
        w[neu];   // insert
    } else {
        auto knoten = w.extract(alt);
        if (knoten.empty()) { return; }
        knoten.key() = neu;
        w.insert(std::move(knoten));
    }
    g_bd.werkzeugeGeaendert = true;
}

// CustomDelegate1::setModelData: der Text als <var>-Inhalt gelesen
void werkzeugWertBearbeitet(const std::string& name, const std::string& xml) {
    karte::BauWerkzeug neu;
    if (!karte::bauWerkzeugAusXml(xml, neu)) { return; }
    e().bauProjekt.werkzeuge[name] = std::move(neu);
    g_bd.werkzeugeGeaendert = true;
}

void bearbeitenBeginnen(int liste, int zeile, int spalte, const std::string& text) {
    BauDialog& d = g_bd;
    d.bearbeitet = liste;
    d.bearbeitetZeile = zeile;
    d.bearbeitetSpalte = spalte;
    d.puffer = text;
    d.bearbeitenFokus = true;
    d.bearbeitenBild = ImGui::GetFrameCount();
}

void bearbeitenAbschliessen(bool uebernehmen) {
    BauDialog& d = g_bd;
    const int liste = d.bearbeitet;
    const int zeile = d.bearbeitetZeile;
    d.bearbeitet = kKeine;
    if (!uebernehmen) { return; }
    if (liste == kEintraege) { eintragBearbeitet(zeile, d.puffer); }
    if (liste == kBefehle) { befehlBearbeitet(zeile, d.puffer); }
    if (liste == kWerkzeuge) {
        std::vector<std::string> namen;
        for (const auto& [n, w] : e().bauProjekt.werkzeuge) { namen.push_back(n); }
        const std::string alt = zeile < static_cast<int>(namen.size()) ? namen[static_cast<std::size_t>(zeile)] : std::string(kLetzte);
        if (d.bearbeitetSpalte == 0) {
            werkzeugNameBearbeitet(alt, d.puffer);
        } else if (alt != kLetzte) {
            werkzeugWertBearbeitet(alt, d.puffer);
        }
    }
}

// Eine Baumliste wie QTreeWidget_drag: Zeilen 16, Text 3 nach dem Rand, die
// letzte Zeile "+" zum Anhaengen; Doppelklick oder F2 bearbeitet.
// Gibt die Hoehe des Inhalts zurueck.
void baumListe(int art, const std::vector<std::string>& zeilen, int x, int y, int b, int h, int& aktuell) {
    BauDialog& d = g_bd;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = bp(static_cast<float>(x), static_cast<float>(y));
    const ImVec2 c = bp(static_cast<float>(x + b), static_cast<float>(y + h));
    merkeFeld(art == kEintraege ? "Bau:Eintraege" : "Bau:Befehle", x, y, b, h);
    dl->AddRectFilled(a, c, farbe(ImGuiCol_FrameBg));
    dl->AddRect(a, c, farbe(ImGuiCol_Border));
    const int anzahl = static_cast<int>(zeilen.size()) + 1;   // + "+"
    const float zeileH = px(static_cast<float>(kZeile));
    const float innenH = (c.y - a.y) - px(2.0F) - (art == kBefehle ? px(static_cast<float>(kRollbalken)) : 0.0F);
    const float inhaltH = zeileH * static_cast<float>(anzahl);
    const bool rollbar = art == kEintraege && inhaltH > innenH;   // ScrollBarAsNeeded (senkrecht nur bei der Eintragsliste)
    float& rollen = d.rollen[art == kEintraege ? 0 : 1];
    rollen = std::clamp(rollen, 0.0F, std::max(0.0F, inhaltH - innenH));
    const float textB = (c.x - a.x) - px(2.0F) - (rollbar ? px(static_cast<float>(kRollbalken)) : 0.0F);
    ImGui::SetCursorScreenPos(ImVec2{a.x + px(1.0F), a.y + px(1.0F)});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    ImGui::BeginChild(art == kEintraege ? "##bauliste" : "##baubefehle", ImVec2{textB, innenH}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* zl = ImGui::GetWindowDrawList();
    const ImVec2 o = ImGui::GetCursorScreenPos();
    const ImGuiIO& io = ImGui::GetIO();
    // Die Flaeche als unsichtbarer Knopf: ein Klick ist ein Klick in die Liste,
    // kein Ziehen des Fensters (ImGui verschiebt sonst den Dialog).
    ImGui::InvisibleButton("##flaeche", ImVec2{textB, innenH});
    ImGui::SetCursorScreenPos(o);
    const bool ueber = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    d.listeO[art - 1] = ImVec2{o.x, o.y - rollen};
    d.listeB[art - 1] = textB;
    d.zeileH[art - 1] = zeileH;
    if (ueber && io.MouseWheel != 0.0F) { rollen -= io.MouseWheel * zeileH * 3.0F; }   // drei Zeilen je Raste
    rollen = std::clamp(rollen, 0.0F, std::max(0.0F, inhaltH - innenH));
    for (int i = 0; i < anzahl; ++i) {
        const float zy = o.y + zeileH * static_cast<float>(i) - rollen;
        if (zy + zeileH < o.y || zy > o.y + innenH) { continue; }
        const std::string text = i < static_cast<int>(zeilen.size()) ? zeilen[static_cast<std::size_t>(i)] : std::string(kLetzte);
        const bool gewaehlt = aktuell == i;
        if (gewaehlt) {
            zl->AddRectFilled(ImVec2{o.x, zy}, ImVec2{o.x + textB, zy + zeileH},
                              farbe(d.fokus == art ? ImGuiCol_HeaderActive : ImGuiCol_Header));
        }
        if (d.bearbeitet == art && d.bearbeitetZeile == i) {
            ImGui::SetCursorScreenPos(ImVec2{o.x, zy});
            ImGui::SetNextItemWidth(textB);
            if (d.bearbeitenFokus) {
                ImGui::SetKeyboardFocusHere();
                d.bearbeitenFokus = false;
            }
            char puffer[2048];
            std::snprintf(puffer, sizeof(puffer), "%s", d.puffer.c_str());
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(2.0F), std::max(0.0F, (zeileH - ImGui::GetTextLineHeight()) * 0.5F)});
            if (ImGui::InputText("##baufeld", puffer, sizeof(puffer))) { d.puffer = puffer; }
            ImGui::PopStyleVar();
            if (ImGui::IsItemDeactivated()) { bearbeitenAbschliessen(!ImGui::IsKeyPressed(ImGuiKey_Escape)); }
            continue;
        }
        zl->AddText(ImVec2{o.x + px(3.0F), std::floor(zy + (zeileH - ImGui::GetTextLineHeight()) * 0.5F)}, farbe(ImGuiCol_Text), text.c_str());
        // Maus: Klick waehlt, Doppelklick bearbeitet, Ziehen verschiebt
        const bool zeileUeber = ueber && io.MousePos.y >= zy && io.MousePos.y < zy + zeileH && io.MousePos.x >= o.x &&
                                io.MousePos.x < o.x + textB;
        if (zeileUeber && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (d.bearbeitet != kKeine) { bearbeitenAbschliessen(true); }
            aktuell = i;
            d.fokus = art;
            if (art == kEintraege) {
                d.befehlAktuell = -1;   // project_selection_changed: Befehle neu
                d.rollen[1] = 0.0F;
            }
            if (i < static_cast<int>(zeilen.size())) {   // c_itemFlagsDraggable (nicht "+")
                d.ziehListe = art;
                d.ziehVon = i;
                d.ziehStart = io.MousePos;
                d.zieht = false;
            }
        }
        if (zeileUeber && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {   // EditTrigger::DoubleClicked
            d.ziehListe = kKeine;
            bearbeitenBeginnen(art, i, 0, text);
        }
    }
    // Ziehen: ab der Schwelle der Maus (QApplication::startDragDistance 10)
    if (d.ziehListe == art && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const float dx = io.MousePos.x - d.ziehStart.x;
        const float dy = io.MousePos.y - d.ziehStart.y;
        if (!d.zieht && dx * dx + dy * dy >= px(10.0F) * px(10.0F)) { d.zieht = true; }
    }
    int ziel = -1;   // Einfuegestelle (vor Zeile ziel)
    if (d.ziehListe == art && d.zieht) {
        const float rel = io.MousePos.y - o.y + rollen;
        const int zeile = static_cast<int>(std::floor(rel / zeileH));
        // dragMoveEvent/positionBelowLast: nicht unter die letzte Zeile ("+") und nicht ausserhalb
        if (ueber && zeile >= 0 && zeile < anzahl) {
            const bool untereHaelfte = rel - static_cast<float>(zeile) * zeileH > zeileH * 0.5F;
            if (!(zeile == anzahl - 1 && untereHaelfte)) { ziel = zeile + (untereHaelfte ? 1 : 0); }
        }
        if (ziel >= 0) {   // QAbstractItemView: Einfuegelinie
            const float ly = o.y + zeileH * static_cast<float>(ziel) - rollen;
            zl->AddLine(ImVec2{o.x, ly}, ImVec2{o.x + textB, ly}, farbe(ImGuiCol_Text));
        }
    }
    if (d.ziehListe == art && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (d.zieht && ziel >= 0) {
            const bool kopieren = strg();   // mousePressEvent: ControlModifier -> DragDrop
            const int von = d.ziehVon;
            if (art == kEintraege) {
                auto& liste = eintraege();
                karte::BauEintrag eintrag = liste[static_cast<std::size_t>(von)];
                if (kopieren) {
                    liste.insert(liste.begin() + ziel, eintrag);
                    aktuell = ziel;
                } else {
                    // splice: erst ans Ende, dann an die Stelle start (rowsInserted)
                    const int start = ziel > von ? ziel - 1 : ziel;
                    liste.erase(liste.begin() + von);
                    liste.insert(liste.begin() + start, eintrag);
                    aktuell = start;
                }
                d.eintraegeGeaendert = true;
            } else if (d.aktuell >= 0 && d.aktuell < static_cast<int>(eintraege().size())) {
                std::vector<std::string>& bl = eintraege()[static_cast<std::size_t>(d.aktuell)].befehle;
                const std::string befehl = bl[static_cast<std::size_t>(von)];
                if (kopieren) {
                    bl.insert(bl.begin() + ziel, befehl);
                    aktuell = ziel;
                } else {
                    const int start = ziel > von ? ziel - 1 : ziel;
                    bl.erase(bl.begin() + von);
                    bl.insert(bl.begin() + start, befehl);
                    aktuell = start;
                }
                d.befehleGeaendert = true;
                e().bauGeaendert = true;
            }
        }
        d.ziehListe = kKeine;
        d.zieht = false;
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    if (rollbar) {   // Rollbalken 7 (dark.qss), Griff im Verhaeltnis
        const float rx = c.x - px(1.0F) - px(static_cast<float>(kRollbalken));
        const float ry0 = a.y + px(1.0F);
        const float griffH = std::max(px(10.0F), innenH * innenH / inhaltH);
        const float gy = ry0 + (innenH - griffH) * (rollen / std::max(1.0F, inhaltH - innenH));
        dl->AddRectFilled(ImVec2{rx, ry0}, ImVec2{rx + px(static_cast<float>(kRollbalken)), ry0 + innenH}, farbe(ImGuiCol_ScrollbarBg));
        dl->AddRectFilled(ImVec2{rx, gy}, ImVec2{rx + px(static_cast<float>(kRollbalken)), gy + griffH}, farbe(ImGuiCol_ScrollbarGrab));
    }
}

// keyPressEvent von QTreeWidget_project / QTreeWidget_commands und die
// Tasten des Baums (Pfeile, F2)
void listenTasten() {
    BauDialog& d = g_bd;
    if (d.bearbeitet != kKeine || (d.fokus != kEintraege && d.fokus != kBefehle)) { return; }
    auto& liste = eintraege();
    const bool eintragDa = d.aktuell >= 0 && d.aktuell < static_cast<int>(liste.size());
    if (d.fokus == kEintraege) {
        const int anzahl = static_cast<int>(liste.size()) + 1;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true) && d.aktuell + 1 < anzahl) {
            ++d.aktuell;
            d.befehlAktuell = -1;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) && d.aktuell > 0) {
            --d.aktuell;
            d.befehlAktuell = -1;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F2) && d.aktuell >= 0 && d.aktuell < anzahl) {   // EditKeyPressed
            bearbeitenBeginnen(kEintraege, d.aktuell, 0, eintragDa ? liste[static_cast<std::size_t>(d.aktuell)].name : std::string(kLetzte));
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && eintragDa) {   // QKeySequence::Delete
            liste.erase(liste.begin() + d.aktuell);
            d.eintraegeGeaendert = true;
            d.befehlAktuell = -1;   // currentItemChanged( topLevelItem( id ) )
        }
        if (strg() && ImGui::IsKeyPressed(ImGuiKey_C) && eintragDa) { d.kopiert = liste[static_cast<std::size_t>(d.aktuell)]; }
        if (strg() && ImGui::IsKeyPressed(ImGuiKey_V) && !d.kopiert.name.empty() && d.aktuell >= 0 &&
            d.aktuell < anzahl) {   // project.insert( x, copied ): vor dem aktuellen
            liste.insert(liste.begin() + d.aktuell, d.kopiert);
            d.eintraegeGeaendert = true;
        }
    } else if (eintragDa) {
        std::vector<std::string>& b = liste[static_cast<std::size_t>(d.aktuell)].befehle;
        const int anzahl = static_cast<int>(b.size()) + 1;
        const bool befehlDa = d.befehlAktuell >= 0 && d.befehlAktuell < static_cast<int>(b.size());
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true) && d.befehlAktuell + 1 < anzahl) { ++d.befehlAktuell; }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) && d.befehlAktuell > 0) { --d.befehlAktuell; }
        if (ImGui::IsKeyPressed(ImGuiKey_F2) && d.befehlAktuell >= 0 && d.befehlAktuell < anzahl) {
            bearbeitenBeginnen(kBefehle, d.befehlAktuell, 0, befehlDa ? b[static_cast<std::size_t>(d.befehlAktuell)] : std::string(kLetzte));
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && befehlDa) {
            b.erase(b.begin() + d.befehlAktuell);
            d.befehleGeaendert = true;
            e().bauGeaendert = true;
        }
        if (strg() && ImGui::IsKeyPressed(ImGuiKey_C) && befehlDa) { d.kopierterBefehl = b[static_cast<std::size_t>(d.befehlAktuell)]; }
        if (strg() && ImGui::IsKeyPressed(ImGuiKey_V) && !d.kopierterBefehl.empty() && d.befehlAktuell >= 0 &&
            d.befehlAktuell < anzahl) {
            b.insert(b.begin() + d.befehlAktuell, d.kopierterBefehl);
            d.befehleGeaendert = true;
            e().bauGeaendert = true;
        }
    }
}

// Eine Tabelle wie QTableWidget ohne Koepfe, Spalte 0 nach Inhalt, Spalte 1
// gedehnt, Zeilen nach Inhalt; Gitterlinien. Gibt die Hoehe zurueck.
int tabelle(int art, const std::vector<std::pair<std::string, std::string>>& zeilen, int x, int y, int b, bool symbol) {
    BauDialog& d = g_bd;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float zeileH = px(static_cast<float>(kZeile + 4));   // ResizeToContents: Text + Rand
    int spalte0 = 0;
    for (const auto& [k, w] : zeilen) {
        (void)w;
        const float t = ImGui::CalcTextSize(k.c_str()).x / g_sk;
        spalte0 = std::max(spalte0, static_cast<int>(std::ceil(t)) + 6 + (symbol ? 18 : 0));
    }
    const int h = static_cast<int>(zeilen.size()) * (kZeile + 4) + 2;
    const ImVec2 a = bp(static_cast<float>(x), static_cast<float>(y));
    const ImVec2 c = bp(static_cast<float>(x + b), static_cast<float>(y + h));
    merkeFeld(art == kKonstanten ? "Bau:Konstanten" : "Bau:Werkzeuge", x, y, b, h);
    dl->AddRectFilled(a, c, farbe(ImGuiCol_FrameBg));
    dl->AddRect(a, c, farbe(ImGuiCol_Border));
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetCursorScreenPos(a);   // unsichtbarer Knopf ueber der Tabelle (kein Ziehen des Fensters)
    ImGui::InvisibleButton(art == kKonstanten ? "##konstflaeche" : "##wzflaeche", ImVec2{c.x - a.x, c.y - a.y});
    const bool tabUeber = ImGui::IsItemHovered();
    const float x1 = a.x + px(1.0F) + px(static_cast<float>(spalte0));
    d.listeO[art - 1] = ImVec2{a.x + px(1.0F), a.y + px(1.0F)};
    d.listeB[art - 1] = c.x - a.x - px(2.0F);
    d.spalte0[art - 1] = x1 - a.x - px(1.0F);
    d.zeileH[art - 1] = zeileH;
    dl->AddLine(ImVec2{x1, a.y}, ImVec2{x1, c.y}, farbe(ImGuiCol_TableBorderLight));
    for (std::size_t i = 0; i < zeilen.size(); ++i) {
        const float zy = a.y + px(1.0F) + zeileH * static_cast<float>(i);
        if (i + 1 < zeilen.size()) { dl->AddLine(ImVec2{a.x, zy + zeileH}, ImVec2{c.x, zy + zeileH}, farbe(ImGuiCol_TableBorderLight)); }
        const float ty = std::floor(zy + (zeileH - ImGui::GetTextLineHeight()) * 0.5F);
        float tx = a.x + px(4.0F);
        if (symbol) {
            karteIcon(dl, "copy", ImVec2{tx, std::floor(zy + (zeileH - px(16.0F)) * 0.5F)}, px(16.0F), farbe(ImGuiCol_Text));
            tx += px(18.0F);
        }
        const int zeile = static_cast<int>(i);
        const bool feld0 = d.bearbeitet == art && d.bearbeitetZeile == zeile && d.bearbeitetSpalte == 0;
        const bool feld1 = d.bearbeitet == art && d.bearbeitetZeile == zeile && d.bearbeitetSpalte == 1;
        const auto feld = [&](float fx, float fb) {
            ImGui::SetCursorScreenPos(ImVec2{fx, zy});
            ImGui::SetNextItemWidth(fb);
            if (d.bearbeitenFokus) {
                ImGui::SetKeyboardFocusHere();
                d.bearbeitenFokus = false;
            }
            char puffer[4096];
            std::snprintf(puffer, sizeof(puffer), "%s", d.puffer.c_str());
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(2.0F), std::max(0.0F, (zeileH - ImGui::GetTextLineHeight()) * 0.5F)});
            if (ImGui::InputText("##baufeldtab", puffer, sizeof(puffer))) { d.puffer = puffer; }
            ImGui::PopStyleVar();
            if (ImGui::IsItemDeactivated()) { bearbeitenAbschliessen(!ImGui::IsKeyPressed(ImGuiKey_Escape)); }
        };
        if (feld0) {
            feld(a.x + px(1.0F), x1 - a.x - px(1.0F));
        } else {
            dl->AddText(ImVec2{tx, ty}, farbe(ImGuiCol_Text), zeilen[i].first.c_str());
        }
        if (feld1) {
            feld(x1 + px(1.0F), c.x - x1 - px(2.0F));
        } else {
            dl->PushClipRect(ImVec2{x1 + px(1.0F), zy}, ImVec2{c.x - px(1.0F), zy + zeileH}, true);
            dl->AddText(ImVec2{x1 + px(4.0F), ty}, farbe(ImGuiCol_Text), zeilen[i].second.c_str());
            dl->PopClipRect();
        }
        const bool ueber = tabUeber && io.MousePos.y >= zy && io.MousePos.y < zy + zeileH && io.MousePos.x >= a.x &&
                           io.MousePos.x < c.x;
        if (!ueber) { continue; }
        const int spalte = io.MousePos.x < x1 ? 0 : 1;
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (d.bearbeitet != kKeine && !(feld0 || feld1)) { bearbeitenAbschliessen(true); }
            d.fokus = art;
            if (art == kWerkzeuge) { d.werkzeugAktuell = zeile; }
            if (art == kKonstanten && spalte == 0) {   // itemClicked: in die Zwischenablage, QToolTip 666 ms
                ImGui::SetClipboardText(zeilen[i].first.c_str());
                d.kopiertBis = ImGui::GetTime() + 0.666;
                d.kopiertAn = ImVec2{a.x, zy + zeileH};
            }
        }
        if (art == kWerkzeuge && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            const bool letzte = zeilen[i].first == kLetzte;
            if (spalte == 0) {
                bearbeitenBeginnen(kWerkzeuge, zeile, 0, zeilen[i].first);
            } else if (!letzte) {   // CustomDelegate1::createEditor: nicht in der "+"-Zeile
                std::vector<std::string> namen;
                for (const auto& [n, w] : e().bauProjekt.werkzeuge) { namen.push_back(n); }
                bearbeitenBeginnen(kWerkzeuge, zeile, 1, karte::bauWerkzeugXml(e().bauProjekt.werkzeuge[zeilen[i].first]));
            }
        }
    }
    return h;
}

void dialogErgebnis() {   // DoBuildMenu nach exec()
    BauDialog& d = g_bd;
    Editor& ed = e();
    if (d.ergebnis == 2) {   // eIDCANCEL (auch Schliessen)
        if (d.eintraegeGeaendert || d.befehleGeaendert || d.werkzeugeGeaendert) {
            ed.bauProjekt = d.sicherung;
            ed.bauZuletzt = d.sicherungZuletzt;
        }
    } else if (d.ergebnis == 3) {   // eIDNO: RESET
        bauMenueNeuLaden();
    } else if (d.eintraegeGeaendert || d.werkzeugeGeaendert) {
        ed.bauGeaendert = true;
    }
    if (ed.bauZuletzt >= static_cast<long long>(ed.bauProjekt.eintraege.size())) { ed.bauZuletzt = -1; }
}

}  // namespace

void bauDialogZeichnen() {
    Editor& ed = e();
    BauDialog& d = g_bd;
    const char* kKennung = "###kartebaumenue";
    if (!ed.bauDialogOffen) {
        d.offen = false;
        return;
    }
    bauMenueSicherstellen();
    const std::string titel = std::string(trK(Str::KeBmTitel)) + kKennung;
    if (!d.offen) {   // DoBuildMenu: Sicherung; BSPCommandList_Construct ohne Wahl
        d = BauDialog{};
        d.offen = true;
        d.sicherung = ed.bauProjekt;
        d.sicherungZuletzt = ed.bauZuletzt;
        ImGui::OpenPopup(titel.c_str());
    }
    g_sk = massstab();
    // g_guiSettings.addWindow( dialog, "BuildMenu/geometry", 700, 500 )
    int gb = 700;
    int gh = 500;
    {
        const auto it = g_app->settings.karte.find("BuildMenu/geometry");
        if (it != g_app->settings.karte.end()) { (void)std::sscanf(it->second.c_str(), "%d %d", &gb, &gh); }
        gb = std::max(gb, 300);
        gh = std::max(gh, 250);
    }
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowSize(ImVec2{px(static_cast<float>(gb)), px(static_cast<float>(gh)) + titelH}, ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_Appearing,
                            ImVec2{0.5F, 0.5F});
    ImGui::SetNextWindowSizeConstraints(ImVec2{px(300.0F), px(250.0F) + titelH}, ImVec2{FLT_MAX, FLT_MAX});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});   // QDialog::exec dunkelt nicht ab
    // QDialog ohne Groessengriff (setSizeGripEnabled false): die Ecke bleibt unsichtbar, die Raender ziehen
    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});
    bool offen = true;
    const bool sichtbar = ImGui::BeginPopupModal(titel.c_str(), &offen,
                                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
                                                     ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
    if (!sichtbar) {
        if (d.offen) {   // von aussen geschlossen
            d.ergebnis = 2;
            dialogErgebnis();
            ed.bauDialogOffen = false;
        }
        return;
    }
    g_o = ImGui::GetCursorScreenPos();
    const ImVec2 groesse = ImGui::GetContentRegionAvail();
    const int b = static_cast<int>(std::floor(groesse.x / g_sk + 0.001F));
    const int h = static_cast<int>(std::floor(groesse.y / g_sk + 0.001F));
    g_app->settings.karte["BuildMenu/geometry"] = std::to_string(b) + " " + std::to_string(h);
    merkeFeld("Bau:Dialog", 0, 0, b, h);
    auto& liste = eintraege();
    if (d.aktuell > static_cast<int>(liste.size())) { d.aktuell = static_cast<int>(liste.size()); }
    // --- Spalte 1: QDialogButtonBox senkrecht (OK, Cancel, Reset)
    const int knopfX = b - kRand - kKnopfB;
    const int gruppeB = knopfX - kAbstand - kRand;
    dialogSchriftAn();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (px(static_cast<float>(kKnopfH)) - ImGui::GetTextLineHeight()) * 0.5F)});
    const ImVec2 kg{px(static_cast<float>(kKnopfB)), px(static_cast<float>(kKnopfH))};
    const auto knopf = [&](const char* text, const char* id, int y, const char* tipp) {
        merkeFeld(std::string("Bau:") + id, knopfX, y, kKnopfB, kKnopfH);
        ImGui::SetCursorScreenPos(bp(static_cast<float>(knopfX), static_cast<float>(y)));
        const bool k = ImGui::Button((std::string(text) + "##bau" + id).c_str(), kg);
        if (tipp != nullptr && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) { ImGui::SetTooltip("%s", tipp); }
        return k;
    };
    if (knopf(trK(Str::KePrefOk), "OK", kRand, nullptr)) { d.ergebnis = 1; }
    if (knopf(trK(Str::KeCancel), "Cancel", kRand + kKnopfH + kAbstand, nullptr)) { d.ergebnis = 2; }
    if (knopf(trK(Str::KeReset), "Reset", kRand + 2 * (kKnopfH + kAbstand), trK(Str::KeBmResetTipp))) { d.ergebnis = 3; }
    ImGui::PopStyleVar();
    dialogSchriftAus();
    // --- Spalte 0: Build menu (dehnt sich), Commandline, Build Variables
    std::vector<std::string> befehle;
    const bool eintragDa = d.aktuell >= 0 && d.aktuell < static_cast<int>(liste.size());
    if (eintragDa) { befehle = liste[static_cast<std::size_t>(d.aktuell)].befehle; }
    // QTreeWidget_commands::rowsInserted: sizeHintForRow(0) * max(Zeilen, 4) + Rollbalken + 2
    const int befehlsListeH = kZeile * std::max(eintragDa ? static_cast<int>(befehle.size()) + 1 : 0, 4) + kRollbalken + 2;
    const int befehlsGruppeH = kInhaltOben + befehlsListeH + kInhaltRand;
    // Build Variables: die Tabellen, wenn angekreuzt
    std::vector<std::pair<std::string, std::string>> konstanten;
    std::vector<std::pair<std::string, std::string>> werkzeuge;
    int variablenH = kKopfH;
    karte::BauVariablen v = bauVariablen();
    if (d.variablenOffen) {
        for (const auto& [n, w] : v) { konstanten.emplace_back("[" + n + "]", w); }
        karte::BauVariablen mitWerkzeugen = v;
        karte::bauWerkzeugeAuswerten(ed.bauProjekt, mitWerkzeugen);
        for (const auto& [n, w] : ed.bauProjekt.werkzeuge) {
            (void)w;
            werkzeuge.emplace_back(n, mitWerkzeugen[n]);
        }
        werkzeuge.emplace_back(kLetzte, "");
        variablenH = kHaken + 7 + (kZeile + 4) + static_cast<int>(konstanten.size()) * (kZeile + 4) + 2 + kAbstand + (kZeile + 4) +
                     static_cast<int>(werkzeuge.size()) * (kZeile + 4) + 2 + kRand;
    }
    // QLayout (SetDefaultConstraint): passt der Inhalt mit der Eintragsliste auf
    // ihrer Mindesthoehe nicht, waechst der Dialog (Build Variables aufgeklappt).
    const int mindestListe = 2 * kZeile + 2;
    const int noetig = 2 * kRand + kInhaltOben + mindestListe + kInhaltRand + kAbstand + befehlsGruppeH + kAbstand + variablenH;
    if (h < noetig) {
        ImGui::SetWindowSize(ImVec2{ImGui::GetWindowSize().x, ImGui::GetWindowSize().y + px(static_cast<float>(noetig - h))});
    }
    const int eintragsGruppeH = std::max(kInhaltOben + mindestListe + kInhaltRand,
                                         h - 2 * kRand - befehlsGruppeH - variablenH - 2 * kAbstand);
    int y = kRand;
    gruppe(trK(Str::KeBmGruppe), kRand, y, gruppeB, eintragsGruppeH);
    std::vector<std::string> namen;
    for (const karte::BauEintrag& en : liste) { namen.push_back(en.name); }
    baumListe(kEintraege, namen, kRand + kInhaltRand, y + kInhaltOben, gruppeB - 2 * kInhaltRand,
              eintragsGruppeH - kInhaltOben - kInhaltRand, d.aktuell);
    y += eintragsGruppeH + kAbstand;
    gruppe(trK(Str::KeBmBefehle), kRand, y, gruppeB, befehlsGruppeH);
    if (eintragDa) {
        baumListe(kBefehle, befehle, kRand + kInhaltRand, y + kInhaltOben, gruppeB - 2 * kInhaltRand, befehlsListeH, d.befehlAktuell);
    } else {
        // kein Eintrag gewaehlt (g_current_build == 0): leere Liste
        std::vector<std::string> leer;
        int keiner = -1;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 a = bp(static_cast<float>(kRand + kInhaltRand), static_cast<float>(y + kInhaltOben));
        const ImVec2 c = bp(static_cast<float>(kRand + gruppeB - kInhaltRand), static_cast<float>(y + kInhaltOben + befehlsListeH));
        merkeFeld("Bau:Befehle", kRand + kInhaltRand, y + kInhaltOben, gruppeB - 2 * kInhaltRand, befehlsListeH);
        dl->AddRectFilled(a, c, farbe(ImGuiCol_FrameBg));
        dl->AddRect(a, c, farbe(ImGuiCol_Border));
        (void)leer;
        (void)keiner;
    }
    y += befehlsGruppeH + kAbstand;
    // Build Variables: flache, ankreuzbare QGroupBox (zu Beginn aus)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        merkeFeld("Bau:Variablen", kRand, y, gruppeB, variablenH);
        ImGui::SetCursorScreenPos(bp(static_cast<float>(kRand + 1), static_cast<float>(y)));
        dialogSchriftAn();
        bool an = d.variablenOffen;
        if (qtHaken("##bauvariablen", &an, static_cast<float>(kHaken))) { d.variablenOffen = an; }
        dl->AddText(bp(static_cast<float>(kRand + 1 + kHaken + 5), static_cast<float>(y) + (kHaken - 13) * 0.5F), farbe(ImGuiCol_Text),
                    trK(Str::KeBmVariablen));
        // der flache Rahmen: eine Linie unter dem Titel, eine am Ende
        dl->AddLine(bp(static_cast<float>(kRand), static_cast<float>(y + kHaken + 4)),
                    bp(static_cast<float>(kRand + gruppeB), static_cast<float>(y + kHaken + 4)), farbe(ImGuiCol_Border));
        dl->AddLine(bp(static_cast<float>(kRand), static_cast<float>(y + variablenH - 2)),
                    bp(static_cast<float>(kRand + gruppeB), static_cast<float>(y + variablenH - 2)), farbe(ImGuiCol_Border));
        if (d.variablenOffen) {
            int vy = y + kHaken + 7;
            dl->AddText(bp(static_cast<float>(kRand + kInhaltRand), static_cast<float>(vy + 3)), farbe(ImGuiCol_Text), trK(Str::KeBmKonstanten));
            vy += kZeile + 4;
            dialogSchriftAus();
            vy += tabelle(kKonstanten, konstanten, kRand + kInhaltRand, vy, gruppeB - 2 * kInhaltRand, true) + kAbstand;
            dialogSchriftAn();
            dl->AddText(bp(static_cast<float>(kRand + kInhaltRand), static_cast<float>(vy + 3)), farbe(ImGuiCol_Text), trK(Str::KeBmWerkzeuge));
            vy += kZeile + 4;
            dialogSchriftAus();
            (void)tabelle(kWerkzeuge, werkzeuge, kRand + kInhaltRand, vy, gruppeB - 2 * kInhaltRand, false);
        } else {
            dialogSchriftAus();
        }
    }
    if (d.kopiertBis > ImGui::GetTime()) {   // QToolTip::showText( ..., "Copied to clipboard.", 666 )
        ImGui::SetNextWindowPos(d.kopiertAn);
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(trK(Str::KeBmKopiert));
        ImGui::EndTooltip();
    }
    listenTasten();
    // Ein Klick ausserhalb eines Feldes beendet das Bearbeiten (Fokus weg)
    if (d.bearbeitet != kKeine && d.bearbeitenBild != ImGui::GetFrameCount() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::IsAnyItemHovered()) {
        bearbeitenAbschliessen(true);
    }
    if (!offen && d.ergebnis == 0) { d.ergebnis = 2; }   // Schliessen = reject
    if (ImGui::IsKeyPressed(ImGuiKey_Escape) && d.bearbeitet == kKeine && d.ergebnis == 0) { d.ergebnis = 2; }   // QDialog: Esc = reject
    if (d.ergebnis != 0) {
        if (d.bearbeitet != kKeine) { bearbeitenAbschliessen(d.ergebnis == 1); }
        dialogErgebnis();
        ed.bauDialogOffen = false;
        d.offen = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// Fuer den Selbsttest: Zustand und Eingriffe in den Dialog (ohne Maus)
BauDialogStand bauDialogStand() {
    BauDialogStand s;
    s.offen = e().bauDialogOffen;
    s.aktuell = g_bd.aktuell;
    s.befehlAktuell = g_bd.befehlAktuell;
    s.variablenOffen = g_bd.variablenOffen;
    s.bearbeitet = g_bd.bearbeitet;
    s.kopiertSichtbar = g_bd.kopiertBis > ImGui::GetTime();
    return s;
}

ImVec2 bauDialogPunkt(int x, int y) { return bp(static_cast<float>(x), static_cast<float>(y)); }

ImVec2 bauDialogZeile(int liste, int zeile, int spalte) {
    if (liste < 1 || liste > 4) { return ImVec2{0.0F, 0.0F}; }
    const BauDialog& d = g_bd;
    const int i = liste - 1;
    float x = d.listeO[i].x + d.listeB[i] * 0.5F;
    if (liste >= 3) {   // Tabellen: Mitte der Spalte
        x = spalte == 0 ? d.listeO[i].x + d.spalte0[i] * 0.5F : d.listeO[i].x + d.spalte0[i] + (d.listeB[i] - d.spalte0[i]) * 0.5F;
    }
    return ImVec2{x, d.listeO[i].y + d.zeileH[i] * (static_cast<float>(zeile) + 0.5F)};
}

// --- Bauen mit Ueberwachung (watchbsp.cpp: CWatchBSP) -------------------------------
//
// Ein Schritt nach dem anderen: lauschen (127.0.0.1:39000), q3map2 starten
// (ohne Fenster), die Verbindung annehmen (5 s Zeit), den XML-Strom lesen
// (BauMonitor), bei "Connection closed." den naechsten Schritt. Ohne
// Ueberwachung oder bei einem Befehl ohne Rueckkanal (RunBatch): die
// restlichen Befehle nacheinander, ihre Ausgabe ueber eine Pipe in die Konsole.
// Dazu (Abweichung, Auftrag): Fortschritt in der Statuszeile, Abbrechen, am
// Ende Zeit und Ergebnis; q3map2 endet mit dem Editor (Job).

namespace {

enum class BauZustand { Leerlauf, Schrittbeginn, Beobachten, Stapel };

struct BauLauf {
    BauZustand zustand = BauZustand::Leerlauf;   // m_eState (Stapel: RunBatch)
    std::vector<std::string> befehle;              // m_commands
    std::size_t schritt = 0;                       // m_iCurrentStep
    std::string eintrag;                           // der Name des Eintrags (fuer die Anzeige)
    std::string bspName;                           // m_sBSPName (PathFilename der Karte)
    bauplattform::Prozess prozess;
    bauplattform::Horcher horcher;
    karte::BauNetzRahmen rahmen;
    karte::BauMonitor monitor;
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point schrittStart;   // m_timeout_timer
    bool aktiv = false;                            // eine Folge laeuft (fuer die Zusammenfassung)
    bool leckGemeldet = false;
    std::string zeilenRest;                        // Stapel: angefangene Zeile
    std::string logPfad;                           // Stapel mit "Dump non Monitored Builds Log": junk.txt
    std::vector<std::pair<std::size_t, int>> rueckgaben;   // Schritt und Rueckgabe von q3map2 (Abweichung: in der Zeile am Ende)
    std::function<void(int)> danach;               // Stufe 4: bauEigeneBefehle
    // Stufe 8a Punkt 1 (Abweichung, eigene Funktion): Fortschritt und Restzeit
    std::string kartenPfad;                        // Schluessel der Bauzeiten (die Karte des Editors)
    karte::BauSchrittBeobachter beob;              // Phase und Pacifier des laufenden Schritts
    karte::BauLaufZeiten jetzt;                    // Zeiten dieses Laufs
    std::vector<double> fertig;                    // Dauern der fertigen Schritte
    bool hatVerlauf = false;
    karte::GpuBackBericht gpu;                     // Stufe 8g: was bhed_q3map2 -bhedgpubake ausgibt
    karte::GpuBacken gpuModus = karte::GpuBacken::Aus;   // Stufe 8d: Aus/Ein/Auto dieses Baus
    long long autoCpuMs = 0, autoGpuMs = 0;        // Stufe 8d: die an -bhedgpubakezeiten gegebenen Zeiten
    bool autoGemeldet = false;                     // Stufe 8d: die Wahl steht schon in der Konsole
    std::string gpuRest;                           // angefangene Zeile dazu
    karte::BauLaufZeiten verlauf;                  // der letzte Bau dieser Karte mit diesem Eintrag
    // Stufe 8a Punkt 2: nach dem Bau <karte>.bhed schreiben (Quelle, .bsp)
    std::string editorDatenQuelle;
    std::string editorDatenBsp;
};
BauLauf g_lauf;

double sekundenSeit(std::chrono::steady_clock::time_point t) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t).count();
}

// Die Bauzeiten aller Karten (karte_bauzeiten.json im Einstellungsordner)
std::string bauZeitenPfad() { return platform::settingsDirectory() + "\\karte_bauzeiten.json"; }
// Stufe 8d Punkt 1b: die gemerkten -light-Zeiten je Karte, Schaltern, CPU und GPU
std::string gpuZeitenPfad() { return platform::settingsDirectory() + "\\karte_gpuzeiten.txt"; }

karte::BauZeitenSpeicher bauZeitenLesen() {
    karte::BauZeitenSpeicher sp;
    std::string text;
    if (dateiLesen(bauZeitenPfad(), text)) { (void)sp.lesen(text); }
    return sp;
}

}  // namespace

karte::GpuZeiten gpuZeitenHolen(const std::string& karte, const std::string& schalter) {
    karte::GpuZeitenSpeicher sp;
    std::string text;
    if (dateiLesen(gpuZeitenPfad(), text)) { (void)sp.lesen(text); }
    return sp.holen(karte, schalter);
}

void gpuZeitMerken(const std::string& karte, const std::string& schalter, bool gpu, double sekunden) {
    karte::GpuZeitenSpeicher sp;
    std::string text;
    if (dateiLesen(gpuZeitenPfad(), text)) { (void)sp.lesen(text); }
    sp.merken(karte, schalter, gpu, static_cast<long long>(sekunden * 1000.0 + 0.5));
    std::ofstream f(pfadAus(gpuZeitenPfad()), std::ios::binary | std::ios::trunc);
    const std::string t = sp.schreiben();
    if (f) { f.write(t.data(), static_cast<std::streamsize>(t.size())); }
    diag::detail("GPU bake Auto: " + karte + " [" + schalter + "] " + (gpu ? "GPU " : "CPU ") + std::to_string(sekunden) + " s gemerkt");
}

std::string gpuWahlText(const karte::GpuBackBericht& b, long long cpuMs, long long gpuMs) {
    if (!b.autoGesehen) { return {}; }
    const auto sek = [](long long ms) {
        char z[32];
        std::snprintf(z, sizeof(z), "%.1f", static_cast<double>(ms) / 1000.0);
        return std::string(z);
    };
    const std::string luxel = std::to_string(b.autoLuxel);
    char grund[512];
    const std::string& g = b.autoGrund;
    if (g == "nogpu") {
        std::snprintf(grund, sizeof(grund), "%s", trK(Str::KeGpuGrundNogpu));
    } else if (g == "nodouble") {
        std::snprintf(grund, sizeof(grund), "%s", trK(Str::KeGpuGrundNodouble));
    } else if (g == "learnedgpu" || g == "learnedcpu") {
        std::snprintf(grund, sizeof(grund), trK(Str::KeGpuGrundLearned), sek(cpuMs).c_str(), sek(gpuMs).c_str());
    } else if (g == "small") {
        std::snprintf(grund, sizeof(grund), trK(Str::KeGpuGrundSmall), luxel.c_str());
    } else if (g == "samples") {
        std::snprintf(grund, sizeof(grund), "%s", trK(Str::KeGpuGrundSamples));
    } else if (g == "measuregpu") {
        std::snprintf(grund, sizeof(grund), trK(Str::KeGpuGrundMeasure), sek(cpuMs).c_str());
    } else {
        std::snprintf(grund, sizeof(grund), trK(Str::KeGpuGrundLarge), luxel.c_str());
    }
    char z[768];
    std::snprintf(z, sizeof(z), trK(Str::KeGpuWahl), trK(b.autoGpu ? Str::KeGpuWahlGpu : Str::KeGpuWahlCpu), grund);
    return z;
}

namespace {

// Ein Schritt ist zu Ende: seine Dauer und Phasen merken
void schrittZeitMerken() {
    BauLauf& l = g_lauf;
    if (l.schritt >= l.befehle.size() || l.jetzt.schritte.size() != l.schritt) { return; }
    const double d = sekundenSeit(l.schrittStart);
    l.jetzt.schritte.push_back({l.befehle[l.schritt], d, l.beob.marken()});
    l.fertig.push_back(d);
}

// Stufe 8g: die Zeilen des GPU-Backens (GPU, Abweichungsbericht) aus der Ausgabe
void gpuBeobachten(const std::string& text) {
    BauLauf& l = g_lauf;
    l.gpuRest += text;
    std::size_t p = 0;
    for (std::size_t q; (q = l.gpuRest.find('\n', p)) != std::string::npos; p = q + 1) {
        std::string z = l.gpuRest.substr(p, q - p);
        if (!z.empty() && z.back() == '\r') { z.pop_back(); }
        if (karte::gpuBackZeile(z, l.gpu) && !l.gpu.adapter.empty()) { e().bau.gpuAdapter = l.gpu.adapter; }
        if (l.gpu.autoGesehen && !l.autoGemeldet) {   // Stufe 8d: was Auto gewaehlt hat und warum
            l.autoGemeldet = true;
            e().bau.gpuWahlText = gpuWahlText(l.gpu, l.autoCpuMs, l.autoGpuMs);
            konsoleSchreiben(KonsolenZeile::Art::Ausgabe, e().bau.gpuWahlText + "\n");
        }
    }
    l.gpuRest.erase(0, p);
    if (l.gpuRest.size() > 4096) { l.gpuRest.clear(); }
}

void schrittBeobachten(const std::string& text) {
    g_lauf.beob.text(text, sekundenSeit(g_lauf.schrittStart));
    gpuBeobachten(text);
}

// Stufe 8g: nach dem Bau der Abweichungsbericht des GPU-Backens in der Konsole (und im Tooltip von
// Build > GPU bake)
void gpuBerichtAusgeben() {
    BauLauf& l = g_lauf;
    gpuBeobachten("\n");
    if (l.gpu.keineGpu) {
        konsoleSchreiben(KonsolenZeile::Art::Warnung, std::string(trK(Str::KeGpuKeineGpu)) + "\n");
        e().bau.gpuBericht = trK(Str::KeGpuKeineGpu);
        return;
    }
    if (!l.gpu.bericht) { return; }
    char z[1024];
    std::snprintf(z, sizeof(z), trK(Str::KeGpuBericht), l.gpu.adapter.c_str(), l.gpu.gleich.c_str(), std::to_string(l.gpu.luxel).c_str(),
                  std::to_string(l.gpu.lms).c_str(), std::to_string(l.gpu.lmsAnders).c_str(), std::to_string(l.gpu.gpuLm).c_str(),
                  std::to_string(l.gpu.cpuLm).c_str(), std::to_string(l.gpu.gitterGpu).c_str());
    konsoleSchreiben(l.gpu.lmsAnders == 0 ? KonsolenZeile::Art::Ausgabe : KonsolenZeile::Art::Warnung, std::string(z) + "\n");
    e().bau.gpuBericht = z;
}

// Stufe 8a Punkt 2: nach einem Bau mit BSP-Stufe die Quelle als <karte>.bhed neben die .bsp
void editorDatenSchreiben() {
    BauLauf& l = g_lauf;
    if (l.editorDatenQuelle.empty()) { return; }
    std::string mapText;
    std::string bsp;
    if (dateiLesen(l.editorDatenQuelle, mapText) && dateiLesen(l.editorDatenBsp, bsp)) {
        const std::string pfad = karte::editorDatenPfad(l.editorDatenBsp);
        const std::string inhalt = karte::editorDatenSchreiben(mapText, bsp);
        std::ofstream f(pfadAus(pfad), std::ios::binary | std::ios::trunc);
        if (f && f.write(inhalt.data(), static_cast<std::streamsize>(inhalt.size()))) {
            char z[1024];
            std::snprintf(z, sizeof(z), trK(Str::KeEdGeschrieben), mitSchraegstrich(pfad).c_str());
            ausgabe(z);
        } else {
            fehler("writing " + mitSchraegstrich(pfad) + " failure");
        }
    }
    l.editorDatenQuelle.clear();
}

// Bau fertig: ist das Fenster nicht vorne, blinkt der Knopf in der Taskleiste (ohne
// Ton). Im Selbsttest nie - dort zaehlt nur, ob gemeldet wuerde.
void bauFertigMelden() {
    Editor& ed = e();
    if (!ed.bau.melden) { return; }
    if (std::getenv("BHED_EDITORTEST") != nullptr) {
        ++ed.bauMeldungenTest;
        return;
    }
    if (!platform::hauptfensterVorne()) { platform::taskleisteBlinken(); }
}

std::string zeitText(double s) {
    char z[32];
    std::snprintf(z, sizeof(z), "%.1f", s);
    return z;
}

void meldungsKasten(const std::string& text) {   // qt_MessageBox( ..., "Build monitoring", EMessageBoxType::Error )
    qtMeldung("Build monitoring", text, MeldungArt::Fehler, false, nullptr);
}

void zusammenfassung(int art, const std::string& grund) {   // 0 fertig, 1 gestoppt, 2 abgebrochen
    BauLauf& l = g_lauf;
    if (!l.aktiv) { return; }
    l.aktiv = false;
    const std::string zeit = zeitText(sekundenSeit(l.start));
    char z[512];
    // Stufe 4: der Rueckruf eines eigenen Auftrags (bauEigeneBefehle) nach der Schlusszeile
    struct Danach {
        std::function<void(int)> f;
        int art;
        ~Danach() {
            if (f) { f(art); }
        }
    } danach{std::move(l.danach), art};
    l.danach = nullptr;
    if (art == 0) {
        std::snprintf(z, sizeof(z), trK(Str::KeBmFertig), l.eintrag.c_str(), static_cast<int>(l.befehle.size()), zeit.c_str());
        std::string text = z;
        for (const auto& [schritt, code] : l.rueckgaben) {
            if (code == 0) { continue; }
            char c[32];
            const unsigned u = static_cast<unsigned>(code);
            std::snprintf(c, sizeof(c), u > 0xFFFFU ? "0x%08X" : "%u", u);   // Absturz: 0xC0000005
            text += " (" + std::to_string(schritt + 1) + ": exit " + c + ")";
        }
        konsoleSchreiben(KonsolenZeile::Art::Ausgabe, text + "\n");
        // Stufe 8a Punkt 1: die Zeit je Schritt gegen den letzten Bau, dann merken
        if (l.jetzt.schritte.size() == l.befehle.size() && !l.befehle.empty()) {
            std::string zeiten = std::string(trK(Str::KeFsZeitenKopf)) + "\n";
            for (const std::string& z2 : karte::bauZeitenVergleich(l.jetzt, l.hatVerlauf ? &l.verlauf : nullptr, trK(Str::KeFsZeitJetzt),
                                                                    trK(Str::KeFsZeitVergleich))) {
                zeiten += z2 + "\n";
            }
            konsoleSchreiben(KonsolenZeile::Art::Ausgabe, zeiten);
            if (!l.kartenPfad.empty()) {
                karte::BauZeitenSpeicher sp = bauZeitenLesen();
                sp.setzen(l.kartenPfad, l.eintrag, l.jetzt);
                std::ofstream f(pfadAus(bauZeitenPfad()), std::ios::binary | std::ios::trunc);
                const std::string j = sp.schreiben();
                if (f) { f.write(j.data(), static_cast<std::streamsize>(j.size())); }
            }
        }
        editorDatenSchreiben();
        gpuBerichtAusgeben();   // Stufe 8g
        // Stufe 8d Punkt 1b: die Dauer jedes -light-Schritts als CPU- bzw. GPU-Zeit dieser Karte merken
        for (const auto& st : l.jetzt.schritte) {
            const std::string sch = karte::lichtSchalter(st.befehl);
            if (sch.empty() || l.kartenPfad.empty()) { continue; }
            const bool gpuLief = !l.gpu.keineGpu && (l.gpuModus == karte::GpuBacken::Ein ||
                                                     (l.gpuModus == karte::GpuBacken::Auto && l.gpu.autoGpu));
            gpuZeitMerken(l.kartenPfad, sch, gpuLief, st.dauer);
        }
        bauFertigMelden();
        // Stufe 5: nach einem Bau des Build-Menues (nicht nach eigenen
        // Auftraegen wie AutoCaulk) auf Wunsch der Optimierer im Hintergrund
        if (!danach.f && e().optimierer.nachBau) { optimiererStarten(true); }
    } else if (art == 1) {
        std::snprintf(z, sizeof(z), trK(Str::KeBmGestoppt), l.eintrag.c_str(), static_cast<int>(l.schritt + 1),
                      static_cast<int>(l.befehle.size()), grund.c_str(), zeit.c_str());
        konsoleSchreiben(KonsolenZeile::Art::Fehler, std::string(z) + "\n");
        // ein Leck haelt den Bau an - gerade dann zeigt der Optimierer die
        // .lin und Detail-Pinsel an der Leck-Linie
        if (!danach.f && grund == "leak" && e().optimierer.nachBau) { optimiererStarten(true); }
    } else {
        std::snprintf(z, sizeof(z), trK(Str::KeBmAbgebrochen), l.eintrag.c_str(), static_cast<int>(l.schritt + 1),
                      static_cast<int>(l.befehle.size()), zeit.c_str());
        konsoleSchreiben(KonsolenZeile::Art::Warnung, std::string(z) + "\n");
    }
    e().statusBefehl.clear();
}

void leckHinweis() {   // Abweichung (Auftrag): ein Hinweis in der Konsole, wie man das Leck findet
    if (g_lauf.leckGemeldet) { return; }
    g_lauf.leckGemeldet = true;
    konsoleSchreiben(KonsolenZeile::Art::Warnung, std::string(trK(Str::KeBmLeckHinweis)) + "\n");
}

// CWatchBSP::Reset: Sockets zu, Leerlauf (der Prozess laeuft weiter wie bei NRC)
void ueberwachungZuruecksetzen() {
    BauLauf& l = g_lauf;
    l.horcher.schliessen();
    l.rahmen = karte::BauNetzRahmen{};
    l.zustand = BauZustand::Leerlauf;
}

// runEngineCmd / runEngine (watchbsp.cpp:273-307); constructEngineArgs nimmt
// gamename_get - hier bauSpielName (Movie Duels: +set fs_game MD)
void spielStarten() {
    Editor& ed = e();
    const karte::SpielBeschreibung& sb = kartenSpiel();
    std::string exe = ed.bau.engine.wert(sb.engine);
    std::string args = ed.bau.argumente.wert(karte::engineArgumente(bauSpielName(), sb.basisSpiel));
    if (ed.spielModus == "mp") {
        const std::string mp = ed.bau.engineMp.wert(sb.mpEngine);
        if (!mp.empty()) {
            exe = mp;
            args = ed.bau.argumenteMp.wert(karte::engineArgumente(bauSpielName(), sb.basisSpiel));
        }
    }
    const std::string engine = enginePfadAnzeige();
    std::string cmd = "\"" + engine + exe + "\" ";
    for (std::size_t p = 0;;) {   // substitute "%mapname%"
        const std::size_t q = args.find("%mapname%", p);
        if (q == std::string::npos) {
            cmd += args.substr(p);
            break;
        }
        cmd += args.substr(p, q - p) + g_lauf.bspName;
        p = q + 9;
    }
    ausgabe("Running engine...");
    ausgabe(cmd);
    // Im Selbsttest (BHED_EDITORTEST) nur gemerkt: nie ein Spiel starten
    const bool ok = (e().befehlStarten || std::getenv("BHED_EDITORTEST") != nullptr) ? kartenBefehlStarten(cmd)
                                                                                     : bauplattform::losgeloestStarten(cmd, engine);
    if (!ok) {
        const std::string msg = "Failed to execute the following command: " + cmd + "\n";
        konsoleSchreiben(KonsolenZeile::Art::Ausgabe, msg);
        meldungsKasten(msg);
    }
}

// Stapel (RunBatch): eine Zeile der Ausgabe mit der Farbe, die q3map2 ihr in
// seiner Konsole gaebe (set_console_colour_for_flag: WARNING gelb, ERROR rot).
void stapelZeile(const std::string& zeile) {
    KonsolenZeile::Art art = KonsolenZeile::Art::Ausgabe;
    if (zeile.find("WARNING") != std::string::npos) { art = KonsolenZeile::Art::Warnung; }
    if (zeile.find("ERROR") != std::string::npos || zeile.find("leaked") != std::string::npos ||
        zeile.find("LEAKED") != std::string::npos) {
        art = KonsolenZeile::Art::Fehler;
    }
    konsoleSchreiben(art, zeile + "\n");
}

void stapelAusgabe(const std::string& stueck) {
    BauLauf& l = g_lauf;
    if (stueck.empty()) { return; }
    schrittBeobachten(stueck);   // Stufe 8a Punkt 1
    if (!l.logPfad.empty()) {
        std::ofstream f(pfadAus(l.logPfad), std::ios::binary | std::ios::app);
        f.write(stueck.data(), static_cast<std::streamsize>(stueck.size()));
    }
    l.zeilenRest += stueck;
    std::size_t p = 0;
    for (std::size_t q; (q = l.zeilenRest.find('\n', p)) != std::string::npos; p = q + 1) {
        std::string z = l.zeilenRest.substr(p, q - p);
        if (!z.empty() && z.back() == '\r') { z.pop_back(); }
        stapelZeile(z);
    }
    l.zeilenRest.erase(0, p);
}

bool stapelBefehlStarten() {
    BauLauf& l = g_lauf;
    ausgabe("=== running build command ===");
    ausgabe(l.befehle[l.schritt]);
    std::string grund;
    if (!l.prozess.starten(l.befehle[l.schritt], std::string(), true, grund)) {
        const std::string msg = "Failed to execute the following command: " + l.befehle[l.schritt] +
                                "\nCheck that the file exists and that you don't run out of system resources.\n";
        konsoleSchreiben(KonsolenZeile::Art::Ausgabe, msg);
        meldungsKasten(msg);
        return false;
    }
    l.schrittStart = std::chrono::steady_clock::now();
    l.beob = karte::BauSchrittBeobachter{};
    return true;
}

void schrittBeginnen();

// EndMonitoringLoop mit dem Grund (Abweichung: die Zusammenfassung)
void ueberwachungBeenden(const std::string& grund) {
    ueberwachungZuruecksetzen();
    zusammenfassung(1, grund);
}

// CWatchBSP::DoEBeginStep
void schrittBeginnen() {
    BauLauf& l = g_lauf;
    Editor& ed = e();
    ueberwachungZuruecksetzen();
    if (l.schritt == l.befehle.size()) {   // finita
        zusammenfassung(0, std::string());
        if (ed.bau.spielStarten) { spielStarten(); }
        return;
    }
    // monitoring off or unmonitored command
    if (!ed.bau.ueberwachen || l.befehle[l.schritt].find(kBauMonitorAdresse) == std::string::npos) {
        // RunBatch: die restlichen Befehle - hier nacheinander im Hintergrund statt
        // einer qe3bsp.bat in einem Konsolenfenster (Abweichung)
        if (ed.bau.logAusgeben) {   // "> junk.txt" beim ersten, ">>" danach
            l.logPfad = platform::settingsDirectory() + "\\junk.txt";
            std::ofstream f(pfadAus(l.logPfad), std::ios::binary | std::ios::trunc);
            ausgabe("The build output will be saved in '" + mitSchraegstrich(l.logPfad) + "'");
        } else {
            l.logPfad.clear();
        }
        l.zustand = BauZustand::Stapel;
        if (!stapelBefehlStarten()) {
            l.zustand = BauZustand::Leerlauf;
            zusammenfassung(1, "exec");
        }
        return;
    }
    // SetupListening
    ausgabe("Setting up");
    std::string grund;
    if (!l.horcher.lauschen(39000, grund)) {
        const std::string msg = "Failed to get a listening socket on port 39000.\nTry running with Build monitoring disabled if you can't fix this.\n";
        konsoleSchreiben(KonsolenZeile::Art::Ausgabe, msg);
        meldungsKasten(msg);
        zusammenfassung(1, "port 39000");
        return;
    }
    ausgabe("Listening...");
    l.schrittStart = std::chrono::steady_clock::now();   // m_timeout_timer.start()
    l.beob = karte::BauSchrittBeobachter{};
    ausgabe("=== running build command ===");
    ausgabe(l.befehle[l.schritt]);
    if (!l.prozess.starten(l.befehle[l.schritt], std::string(), false, grund)) {
        const std::string msg = "Failed to execute the following command: " + l.befehle[l.schritt] +
                                "\nCheck that the file exists and that you don't run out of system resources.\n";
        konsoleSchreiben(KonsolenZeile::Art::Ausgabe, msg);
        meldungsKasten(msg);
        ueberwachungZuruecksetzen();
        zusammenfassung(1, "exec");
        return;
    }
    if (l.schritt == 0) {   // re-initialise the debug window (g_DbgDlg.Init)
        ed.bauRueckmeldungen.clear();
        ed.bauRueckmeldungWahl = -1;
    }
    l.zustand = BauZustand::Schrittbeginn;
}

}  // namespace

// BuildMonitor_Run -> CWatchBSP::DoMonitoringLoop
void bauAusfuehren(const std::vector<std::string>& befehle) {
    BauLauf& l = g_lauf;
    Editor& ed = e();
    ++ed.bauGestartet;
    if (l.zustand != BauZustand::Leerlauf || l.aktiv) {
        warnung("WatchBSP got a monitoring request while not idling...");
        // Abweichung: der alte Prozess endet (nicht zwei q3map2 an einer Karte)
        l.prozess.beenden();
        ueberwachungZuruecksetzen();
        zusammenfassung(2, std::string());
    }
    if (befehle.empty()) { return; }
    l.befehle = befehle;
    l.schritt = 0;
    l.start = std::chrono::steady_clock::now();
    l.aktiv = !befehle.empty();
    l.leckGemeldet = false;
    l.zeilenRest.clear();
    l.rueckgaben.clear();
    l.jetzt = karte::BauLaufZeiten{};
    l.fertig.clear();
    l.beob = karte::BauSchrittBeobachter{};
    l.hatVerlauf = false;
    l.gpu = karte::GpuBackBericht{};
    l.gpuRest.clear();
    l.autoGemeldet = false;   // Stufe 8d
    l.autoCpuMs = l.autoGpuMs = 0;
    ed.bau.gpuAdapter.clear();   // Stufe 8g: die GPU dieses Baus (aus seiner Ausgabe)
    if (!l.kartenPfad.empty()) {
        const karte::BauZeitenSpeicher sp = bauZeitenLesen();
        if (const karte::BauLaufZeiten* v = sp.finden(l.kartenPfad, l.eintrag)) {
            l.verlauf = *v;
            l.hatVerlauf = true;
        }
    }
    l.monitor = karte::BauMonitor{};
    l.monitor.leckStopp = ed.bau.leckStopp;
    l.monitor.drucken = [](int stufe, const std::string& text) {   // Sys_Print( msg_level, ... )
        schrittBeobachten(text);   // Stufe 8a Punkt 1
        konsoleSchreiben(stufe == karte::kBauStufeErr   ? KonsolenZeile::Art::Fehler
                         : stufe == karte::kBauStufeWrn ? KonsolenZeile::Art::Warnung
                                                        : KonsolenZeile::Art::Ausgabe,
                         text);
    };
    l.monitor.warnung = [](const std::string& z) { warnung(z); };
    l.monitor.fehler = [](const std::string& z) { fehler(z); };
    l.monitor.rueckmeldung = [](const karte::BauRueckmeldung& r) {   // g_DbgDlg.Push: das Fenster zeigt sich
        e().bauRueckmeldungen.push_back(r);
        e().bauRueckmeldungOffen = true;
    };
    l.monitor.leckLinie = [](const std::vector<karte::Vektor3f>& punkte) {   // CPointfile: GenerateDisplayList
        Editor& ed2 = e();
        ed2.punktdatei = punkte;
        ed2.punktdateiSichtbar = !punkte.empty();
        ed2.punktdateiStelle = 0;   // s_check_point = begin()
        ++ed2.sichtStand;
        leckHinweis();
    };
    if (!l.aktiv) { return; }
    schrittBeginnen();
}

// CWatchBSP::RoutineProcessing (jedes Bild) und der Stapel
void bauSchritt() {
    BauLauf& l = g_lauf;
    Editor& ed = e();
    if (l.aktiv) {   // Fortschritt in der Statuszeile (Abweichung)
        char z[256];
        const int s = static_cast<int>(sekundenSeit(l.start));
        char zeit[16];
        std::snprintf(zeit, sizeof(zeit), "%d:%02d", s / 60, s % 60);
        std::snprintf(z, sizeof(z), trK(Str::KeBmStatus), l.eintrag.c_str(), static_cast<int>(std::min(l.schritt + 1, l.befehle.size())),
                      static_cast<int>(l.befehle.size()), zeit);
        ed.statusBefehl = z;
        if (!l.gpu.adapter.empty()) {   // Stufe 8g: die GPU des GPU-Backens
            char g[256];
            std::snprintf(g, sizeof(g), trK(Str::KeGpuStatus), l.gpu.adapter.c_str());
            ed.statusBefehl += std::string(" - ") + g;
        }
        if (l.gpu.autoGesehen && !ed.bau.gpuWahlText.empty()) { ed.statusBefehl += " - " + ed.bau.gpuWahlText; }   // Stufe 8d
    }
    switch (l.zustand) {
    case BauZustand::Schrittbeginn:
        // timeout: if we don't get an incoming connection fast enough, go back to idle
        if (sekundenSeit(l.schrittStart) > 5.0) {
            meldungsKasten("The connection timed out, assuming the build process failed\nMake sure you are using a networked version of Q3Map?\n"
                           "Otherwise you need to disable BSP Monitoring in prefs.");
            ueberwachungBeenden("timeout");
            return;
        }
        if (l.horcher.annehmen()) {
            ausgabe("Connected.");
            l.monitor.verbunden();   // memset( &m_message_info, 0 ), m_bNeedCtxtInit
            l.rahmen = karte::BauNetzRahmen{};
            l.zustand = BauZustand::Beobachten;
        }
        break;
    case BauZustand::Beobachten: {
        std::string daten;
        const int r = l.horcher.empfangen(daten);
        if (r == -1) {
            fehler("SOCKET_ERROR in CWatchBSP::RoutineProcessing");
            fehler("Terminating the connection.");
            ueberwachungBeenden("socket");
            return;
        }
        if (!daten.empty()) {
            l.rahmen.einspeisen(daten.data(), daten.size());
            std::string n;
            bool kaputt = false;
            while (l.rahmen.naechste(n, &kaputt)) {
                l.monitor.einspeisen(n);
                if (l.monitor.beenden()) {   // EndMonitoringLoop aus dem SAX-Rueckruf
                    ueberwachungBeenden(l.monitor.leck() ? "leak" : "error");
                    return;
                }
            }
            if (kaputt) {
                fehler("SOCKET_ERROR in CWatchBSP::RoutineProcessing");
                ueberwachungBeenden("socket");
                return;
            }
        }
        if (r == 2) {   // error or connection closed/reset
            l.horcher.schliessen();
            ausgabe("Connection closed.");
            // q3map2 schliesst den Socket beim Beenden - einen Augenblick warten, bis es
            // ganz zu Ende ist, fuer die Rueckgabe (ein Absturz zeigt sich sonst nicht)
            if (l.prozess.warten(1000)) { l.rueckgaben.emplace_back(l.schritt, l.prozess.rueckgabe()); }
            schrittZeitMerken();   // Stufe 8a Punkt 1
            ++l.schritt;   // move to next step or finish
            schrittBeginnen();
        }
        break;
    }
    case BauZustand::Stapel: {
        stapelAusgabe(l.prozess.lesen());
        if (l.prozess.laeuft()) { break; }
        stapelAusgabe(l.prozess.lesen());
        if (!l.zeilenRest.empty()) {
            stapelZeile(l.zeilenRest);
            l.zeilenRest.clear();
        }
        l.rueckgaben.emplace_back(l.schritt, l.prozess.rueckgabe());
        // das Leck nach einem Schritt: die .lin liegt neben der Karte
        std::error_code ec;
        const std::string karte = bauKartenPfad();
        if (fs::exists(pfadAus(karte.substr(0, karte.rfind('.')) + ".lin"), ec)) { leckHinweis(); }
        schrittZeitMerken();   // Stufe 8a Punkt 1
        ++l.schritt;
        if (l.schritt < l.befehle.size()) {
            if (!stapelBefehlStarten()) {
                l.zustand = BauZustand::Leerlauf;
                zusammenfassung(1, "exec");
            }
        } else {
            l.zustand = BauZustand::Leerlauf;
            zusammenfassung(0, std::string());
            if (ed.bau.spielStarten) { spielStarten(); }   // RunBatch: "cd /D <engine>" + runEngineCmd
        }
        break;
    }
    case BauZustand::Leerlauf:
        break;
    }
}

bool bauLaeuft() { return g_lauf.aktiv || g_lauf.zustand != BauZustand::Leerlauf; }

// CDbgDlg::feedback_selection_changed / SetHighlight / Highlight
void bauRueckmeldungWaehlen(int zeile) {
    Editor& ed = e();
    if (zeile < 0 || zeile >= static_cast<int>(ed.bauRueckmeldungen.size())) {
        ed.bauRueckmeldungWahl = -1;
        return;
    }
    ed.bauRueckmeldungWahl = zeile;
    const karte::BauRueckmeldung& r = ed.bauRueckmeldungen[static_cast<std::size_t>(zeile)];
    if (r.art == karte::BauRueckmeldung::Art::Auswahl) {   // CSelectMsg::Highlight
        ed.karte.auswahl.leeren();   // GlobalSelectionSystem().setSelectedAll( false )
        karte::Vektor3f mitte;
        if (r.entitaet >= 0 && r.pinsel >= 0 && karte::pinselFinden(ed.karte, r.entitaet, r.pinsel, &mitte)) {   // SelectBrush
            for (auto& x : ed.xy) { x.ansicht.setzeUrsprung(mitte); }
        }
        auswahlGeaendert();
    }
    ++ed.sichtStand;   // CPointMsg/CWindingMsg: m_pDraw2D (in den 2D-Ansichten)
}

// CDbgDlg::BuildDialog: "Q3Map debug window", ein QTreeWidget ohne Kopf
void bauRueckmeldungenZeichnen() {
    Editor& ed = e();
    if (!ed.bauRueckmeldungOffen) { return; }
    const float s = massstab();
    ImGui::SetNextWindowSize(ImVec2{std::round(320.0F * s), std::round(200.0F * s)}, ImGuiCond_FirstUseEver);
    const std::string titel = std::string(trK(Str::KeDbgTitel)) + "###kartedbgdlg";
    bool offen = true;
    if (ImGui::Begin(titel.c_str(), &offen, ImGuiWindowFlags_NoCollapse)) {
        for (std::size_t i = 0; i < ed.bauRueckmeldungen.size(); ++i) {
            const std::string text = ed.bauRueckmeldungen[i].name + "##dbg" + std::to_string(i);
            if (ImGui::Selectable(text.c_str(), ed.bauRueckmeldungWahl == static_cast<int>(i))) { bauRueckmeldungWaehlen(static_cast<int>(i)); }
        }
    }
    ImGui::End();
    if (!offen) { ed.bauRueckmeldungOffen = false; }
}

// Stufe 8a Punkt 1 (Abweichung, eigene Funktion - nicht in NRC): der Fortschritt
// des laufenden Baus in einem kleinen Fenster - Eintrag, Schritt und Stufe, die
// Phase von q3map2 mit ihrem Pacifier (bzw. "Bounce x von y"), ein Balken ueber
// alle Schritte mit der Restzeit, Abbrechen.
BauFortschrittStand bauFortschrittStand() {
    const BauLauf& l = g_lauf;
    BauFortschrittStand st;
    st.laeuft = l.aktiv;
    if (!l.aktiv) { return st; }
    st.schritt = std::min(l.schritt, l.befehle.empty() ? 0U : l.befehle.size() - 1);
    st.schritte = l.befehle.size();
    st.stufe = st.schritt < l.befehle.size() ? karte::bauStufeName(l.befehle[st.schritt]) : std::string();
    st.phase = l.beob.phase();
    st.pacifier = l.beob.pacifier();
    st.bounce = l.beob.bounce();
    st.bounceVon = l.beob.bounceVon();
    const double t = sekundenSeit(l.schrittStart);
    st.schaetzung = karte::bauSchaetzen(l.hatVerlauf ? &l.verlauf : nullptr, l.befehle, l.schritt, l.beob, t, l.fertig);
    st.vergangen = sekundenSeit(l.start);
    return st;
}

void bauFortschrittZeichnen() {
    Editor& ed = e();
    const BauFortschrittStand st = bauFortschrittStand();
    if (!st.laeuft) {
        ed.bauFortschrittZu = false;   // beim naechsten Bau wieder zeigen
        return;
    }
    if (ed.bauFortschrittZu) { return; }
    const float s = massstab();
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x - 16.0F * s, vp->WorkPos.y + vp->WorkSize.y - 48.0F * s}, ImGuiCond_FirstUseEver,
                            ImVec2{1.0F, 1.0F});
    ImGui::SetNextWindowSize(ImVec2{std::round(420.0F * s), 0.0F}, ImGuiCond_FirstUseEver);
    const std::string titel = std::string(trK(Str::KeFsTitel)) + "###kartebaufortschritt";
    bool offen = true;
    if (ImGui::Begin(titel.c_str(), &offen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        char z[512];
        std::snprintf(z, sizeof(z), trK(Str::KeFsSchritt), g_lauf.eintrag.c_str(), static_cast<int>(st.schritt + 1), static_cast<int>(st.schritte),
                      st.stufe.c_str());
        ImGui::TextUnformatted(z);
        if (!g_lauf.gpu.adapter.empty()) {   // Stufe 8g: die GPU des GPU-Backens
            std::snprintf(z, sizeof(z), trK(Str::KeGpuFsAdapter), g_lauf.gpu.adapter.c_str());
            ImGui::TextUnformatted(z);
        }
        // Balken 1: die Phase von q3map2 (Pacifier 0...9); ohne Pacifier wandernd
        std::string phase = st.phase.empty() ? std::string(trK(Str::KeFsWarten)) : st.phase;
        if (st.bounceVon > 0 && st.bounce > 0) {
            std::snprintf(z, sizeof(z), trK(Str::KeFsBounce), st.bounce, st.bounceVon);
            phase += " - " + std::string(z);
        }
        const float breite = ImGui::GetContentRegionAvail().x;
        if (st.pacifier >= 0.0) {
            std::snprintf(z, sizeof(z), "%s  %d %%", phase.c_str(), static_cast<int>(std::lround(st.pacifier * 100.0)));
            ImGui::ProgressBar(static_cast<float>(st.pacifier), ImVec2{breite, 0.0F}, z);
        } else {
            ImGui::ProgressBar(-1.0F * static_cast<float>(ImGui::GetTime()), ImVec2{breite, 0.0F}, phase.c_str());
        }
        // Balken 2: alle Schritte des Eintrags, mit Restzeit
        ImGui::TextUnformatted(trK(Str::KeFsGesamt));
        std::snprintf(z, sizeof(z), "%d %%", static_cast<int>(std::lround(st.schaetzung.gesamtAnteil * 100.0)));
        ImGui::ProgressBar(static_cast<float>(st.schaetzung.gesamtAnteil), ImVec2{breite, 0.0F}, z);
        std::string rest;
        if (st.schaetzung.rest >= 0.0) {
            std::snprintf(z, sizeof(z), trK(st.schaetzung.ausVerlauf ? Str::KeFsRestVerlauf : Str::KeFsRestGeschaetzt),
                          karte::bauZeitText(st.schaetzung.rest).c_str());
            rest = z;
        } else {
            rest = trK(Str::KeFsRestUnbekannt);
        }
        std::snprintf(z, sizeof(z), trK(Str::KeFsVergangen), karte::bauZeitText(st.vergangen).c_str());
        ImGui::TextUnformatted((std::string(z) + " - " + rest).c_str());
        ed.bauFortschrittText = rest;
        if (ImGui::Button((std::string(trK(Str::KeBmAbbrechen)) + "##fortschrittabbrechen").c_str())) { bauAbbrechen(); }
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s", trK(Str::KeFsHinweis));
        ImGui::PopStyleColor();
    }
    ImGui::End();
    if (!offen) { ed.bauFortschrittZu = true; }
}

// Abweichung (Auftrag): Bauen abbrechen - q3map2 beenden, Ueberwachung zu
void bauAbbrechen() {
    BauLauf& l = g_lauf;
    if (!bauLaeuft()) { return; }
    l.prozess.beenden();
    ueberwachungZuruecksetzen();
    zusammenfassung(2, std::string());
}

void bauBeimBeenden() {   // ~CWatchBSP; dazu: kein q3map2 ohne Editor
    optimiererBeenden();   // Stufe 5: den Faden des Optimierers abwarten
    echtLichtBeenden();    // Stufe 7: einen laufenden Schnell-Bau abbrechen
    bspRueckBeenden();     // Stufe 8a: die Rueckgewinnung nach Open .bsp
    g_lauf.prozess.beenden();
    g_lauf.prozess.loslassen();
    ueberwachungZuruecksetzen();
}


// --- Wo gebaut wird (Abweichung: Ziel-Ordner, nie im Spielordner) -----------------

namespace {

std::string kleinPfad(std::string p) {
    p = mitSchraegstrich(p);
    std::transform(p.begin(), p.end(), p.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (!p.empty() && p.back() != '/') { p += '/'; }
    return p;
}

// Liegt der Pfad im Spielordner (Engine-Pfad oder ein Suchordner des Spiels)?
bool imSpielordner(const std::string& pfad) {
    const std::string p = kleinPfad(pfad);
    std::vector<std::string> spiel = kartenVfsOrdner();
    spiel.push_back(enginePfadAnzeige());
    for (const std::string& o : spiel) {
        const std::string k = kleinPfad(o);
        if (k.size() > 1 && p.rfind(k, 0) == 0) { return true; }
    }
    return false;
}

}  // namespace

// Der Ordner, in dem gebaut wird ("" = neben der Karte), und warum nicht dort.
std::string bauOrdnerVorschau(std::string* hinweis) {
    Editor& ed = e();
    const std::string arbeit = platform::settingsDirectory() + "\\karte_bau";
    char z[1024];
    if (!ed.bau.zielOrdner.empty()) {
        if (imSpielordner(ed.bau.zielOrdner)) {
            std::snprintf(z, sizeof(z), trK(Str::KeBmZielImSpiel), mitSchraegstrich(ed.bau.zielOrdner).c_str(), mitSchraegstrich(arbeit).c_str());
            if (hinweis != nullptr) { *hinweis = z; }
            return arbeit;
        }
        return ed.bau.zielOrdner;
    }
    if (imSpielordner(ed.name)) {
        std::snprintf(z, sizeof(z), trK(Str::KeBmArbeitsordner), mitSchraegstrich(arbeit).c_str());
        if (hinweis != nullptr) { *hinweis = z; }
        return arbeit;
    }
    return std::string();
}

// Stufe 8c (Abweichung, eigene Funktion - nicht in NRC): die gebaute Karte als
// .pk3 neben die .bsp packen, mit maps/<Karte>.bsp und, falls da, maps/<Karte>.bhed
// (Editor-Daten der Stufe 8a). Die .bsp liegt nach bauOrdnerVorschau nie im
// Spielordner; die Pruefung unten ist nur die zweite Sicherung. Ins Spiel
// kopiert der Anwender das Archiv selbst.
std::string karteAlsPk3() {
    Editor& ed = e();
    if (ed.name.empty() || ed.name == "unnamed.map") {
        char z[1024];
        std::snprintf(z, sizeof(z), trK(Str::KePkKeineBsp), "unnamed.bsp");
        fehler(z);
        return std::string();
    }
    const std::string ordner = bauOrdnerVorschau();
    const fs::path karte = pfadAus(ed.name);
    const fs::path basis = ordner.empty() ? karte.parent_path() : pfadAus(ordner);
    const std::u8string stammU = karte.stem().u8string();
    const std::string stamm(stammU.begin(), stammU.end());
    const fs::path bsp = basis / (karte.stem().u8string() + u8".bsp");
    const fs::path ziel = basis / (karte.stem().u8string() + u8".pk3");
    const auto text = [](const fs::path& p) {
        const std::u8string u = p.u8string();
        return mitSchraegstrich(std::string(u.begin(), u.end()));
    };
    std::error_code ec;
    if (!fs::is_regular_file(bsp, ec)) {
        char z[1024];
        std::snprintf(z, sizeof(z), trK(Str::KePkKeineBsp), text(bsp).c_str());
        fehler(z);
        return std::string();
    }
    if (imSpielordner(text(ziel))) {
        char z[1024];
        std::snprintf(z, sizeof(z), trK(Str::KePkSpielordner), text(ziel).c_str());
        fehler(z);
        return std::string();
    }
    const auto lesen = [](const fs::path& p, std::string& aus) {
        std::ifstream f(p, std::ios::binary);
        if (!f) { return false; }
        std::ostringstream o;
        o << f.rdbuf();
        aus = o.str();
        return true;
    };
    std::vector<Pk3Datei> dateien(1);
    dateien[0].name = "maps/" + stamm + ".bsp";
    if (!lesen(bsp, dateien[0].inhalt)) {
        char z[1024];
        std::snprintf(z, sizeof(z), trK(Str::KePkKeineBsp), text(bsp).c_str());
        fehler(z);
        return std::string();
    }
    const fs::path bhed = basis / (karte.stem().u8string() + u8".bhed");
    if (fs::is_regular_file(bhed, ec)) {
        Pk3Datei d;
        d.name = "maps/" + stamm + ".bhed";
        if (lesen(bhed, d.inhalt)) { dateien.push_back(std::move(d)); }
    }
    std::string err;
    if (!writePk3(text(ziel), dateien, &err)) {
        char z[1400];
        std::snprintf(z, sizeof(z), trK(Str::KePkFehler), text(ziel).c_str(), err.c_str());
        fehler(z);
        return std::string();
    }
    std::size_t groesse = 0;
    for (const Pk3Datei& d : dateien) { groesse += d.inhalt.size(); }
    char g[32];
    std::snprintf(g, sizeof(g), "%.0f KB", static_cast<double>(groesse) / 1024.0);
    char z[1400];
    std::snprintf(z, sizeof(z), trK(Str::KePkFertig), text(ziel).c_str(), g, stamm.c_str());
    ausgabe(z);
    return text(ziel);
}

namespace {

// Die Datei, die q3map2 baut: die Karte selbst, oder (Ziel-Ordner, Karte im
// Spielordner) eine Kopie in einem Arbeitsordner - .bsp, .prt, .srf und .lin
// entstehen dann dort. false, wenn die Kopie scheitert.
bool bauOrtBestimmen() {
    Editor& ed = e();
    ed.bauKarte.clear();
    const std::string stamm = ed.name.substr(0, ed.name.rfind('.') == std::string::npos ? ed.name.size() : ed.name.rfind('.'));
    const std::string quelle = ed.region.aktiv ? stamm + ".reg" : ed.name;
    std::string hinweis;
    const std::string ordner = bauOrdnerVorschau(&hinweis);
    if (!hinweis.empty()) { warnung(hinweis); }
    if (ordner.empty()) { return true; }
    std::error_code ec;
    fs::create_directories(pfadAus(ordner), ec);
    const fs::path q = pfadAus(quelle);
    const fs::path ziel = pfadAus(ordner) / q.filename();
    fs::copy_file(q, ziel, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        fehler("build cancelled: cannot copy " + mitSchraegstrich(quelle) + " to " + mitSchraegstrich(ordner) + " (" + ec.message() + ")");
        return false;
    }
    const fs::path karte = pfadAus(ordner) / (pfadAus(ed.name).stem().u8string() + u8".map");
    const std::u8string u = karte.u8string();
    ed.bauKarte = std::string(u.begin(), u.end());
    ed.bauKarteFuer = ed.name;
    return true;
}

}  // namespace

// --- RunBSP (qe3.cpp:235-262) bis zu den Befehlen ----------------------------------

void bauStarten(std::size_t index) {
    Editor& ed = e();
    bauMenueSicherstellen();
    // Abweichung (Stufe 5): externe Lightmaps, die das Spiel nicht laden kann
    // (Jedi Outcast, NRC#98) oder die mit Lichtstilen kaputt sind (RBSP, NRC#48),
    // gar nicht erst bauen - dieselbe Regel wie die Pruefung "baubefehl".
    if (index < ed.bauProjekt.eintraege.size()) {
        std::vector<std::pair<std::string, std::string>> zeilen;
        for (const std::string& b : ed.bauProjekt.eintraege[index].befehle) { zeilen.emplace_back(ed.bauProjekt.eintraege[index].name, b); }
        const auto probleme = karte::bauBefehlProbleme(karte::grenzSpielFuerRegelwerk(kartenSpielNummer()), karte::lichtstileInKarte(ed.karte), zeilen);
        if (!probleme.empty()) {
            char z[1024];
            std::snprintf(z, sizeof(z), trK(Str::KeBmGesperrt), karte::ohneMnemonik(probleme.front().eintrag).c_str(), probleme.front().schalter.c_str(),
                          probleme.front().grund.c_str());
            fehler(z);
            return;
        }
    }
    // if( !g_region_active ) SaveMap();
    if (!ed.region.aktiv) { (void)karteSpeichern(); }
    if (ed.name == "unnamed.map") {   // Map_Unnamed (SaveMap: Speichern unter abgebrochen)
        ausgabe("build cancelled");
        return;
    }
    if (ed.region.aktiv) {   // Map_SaveRegion( <map>.reg )
        const std::string karte = ed.name.substr(0, ed.name.rfind('.') == std::string::npos ? ed.name.size() : ed.name.rfind('.'));
        (void)regionSpeichernUnter(karte + ".reg");
        // Stufe 6 (NRC#132): NRC baut trotzdem und meldet das Leck erst danach (ABSICHT) -
        // hier vorher ein Hinweis mit Grund und Ausweg; gebaut wird wie in NRC.
        if (karte::regionStartImFesten(ed.karte, ed.region, ed.kamera.ursprung)) { warnung(trK(Str::KeRegionLeckHinweis)); }
    }
    // if( !g_region_active && g_SnapShots_Enabled && Map_Modified( g_map ) ) Map_Snapshot(); -
    // nach SaveMap ist die Karte nicht mehr geaendert: der Zweig wirkt nur, wenn
    // das Speichern scheiterte - dann baut auch q3map2 nicht die Aenderung.
    if (!bauOrtBestimmen()) { return; }
    // Pointfile_Delete: die alte Leck-Linie (.lin) der gebauten Datei weg
    {
        const std::string karte = bauKartenPfad();
        std::error_code ec;
        fs::remove(pfadAus(karte.substr(0, karte.rfind('.')) + ".lin"), ec);
    }
    if (index < ed.bauProjekt.eintraege.size()) { ed.bauZuletzt = static_cast<long long>(index); }   // g_lastExecutedBuild
    std::vector<std::string> meldungen;
    std::vector<std::string> befehle = karte::bauBefehle(ed.bauProjekt, index, bauVariablen(), &meldungen);
    for (const std::string& m : meldungen) { fehler(m); }
    // Stufe 8a Punkt 2 (Abweichung, eigene Bauoption): Editor-Daten behalten - jede
    // BSP-Stufe mit -keeplights, nach dem Bau die Quelle als <karte>.bhed neben die .bsp
    std::string edQuelle;
    std::string edBsp;
    if (ed.bau.editorDaten) {
        int n = 0;
        for (std::string& b : befehle) {
            const std::string neu = karte::befehlMitLichtern(b);
            if (neu != b) { ++n; }
            if (karte::bauStufeAusBefehl(b) == karte::BauStufe::Bsp) {
                const karte::BauVariablen v = bauVariablen();
                edQuelle = v.at("MapFile");
                edBsp = v.at("BspFile");
            }
            b = neu;
        }
        if (n > 0) {
            char z[256];
            std::snprintf(z, sizeof(z), trK(Str::KeEdLichter), n);
            ausgabe(z);
        }
    }
    // Stufe 8a Punkt 5a (Abweichung): VIS wiederverwenden, wenn sich die Portale nicht aendern
    if (ed.bau.visWieder) {
        for (std::string& b : befehle) { b = karte::befehlMitVisWiederverwendung(b); }
    }
    // Stufe 8a Punkt 5b/5d (Abweichung): Licht nur fuer betroffene Lightmaps, Nachbearbeitung ohne neu zu tracen
    if (ed.bau.lichtWieder) {
        for (std::string& b : befehle) { b = karte::befehlMitLichtWiederverwendung(b); }
    }
    // Stufe 8g (Abweichung): die Lichtrechnung auf der GPU, Byte-gleich mit der CPU. Stufe 8d: Auto -
    // bhed_q3map2 waehlt mit den gemerkten Zeiten dieser Karte die schnellere Variante
    long long autoCpu = 0;
    long long autoGpu = 0;
    if (ed.bau.gpuBacken == karte::GpuBacken::Ein) {
        for (std::string& b : befehle) { b = karte::befehlMitGpuBacken(b); }
    } else if (ed.bau.gpuBacken == karte::GpuBacken::Auto) {
        for (std::string& b : befehle) {
            const std::string sch = karte::lichtSchalter(b);
            if (sch.empty()) { continue; }
            const karte::GpuZeiten z = gpuZeitenHolen(ed.name, sch);
            autoCpu = z.cpuMs;
            autoGpu = z.gpuMs;
            b = karte::befehlMitGpuBackenAuto(b, z.cpuMs, z.gpuMs);
        }
    }
    // bspname: PathFilename( Map_Name( g_map ) ) fuer die Engine
    const std::u8string stamm = pfadAus(ed.name).stem().u8string();
    const std::string name = index < ed.bauProjekt.eintraege.size() ? karte::ohneMnemonik(ed.bauProjekt.eintraege[index].name) : std::string();
    if (bauLaeuft()) {   // DoMonitoringLoop: nicht im Leerlauf
        bauAusfuehren(std::vector<std::string>{});
    }
    g_lauf.eintrag = name;
    g_lauf.bspName = std::string(stamm.begin(), stamm.end());
    g_lauf.kartenPfad = ed.name;
    bauAusfuehren(befehle);
    g_lauf.gpuModus = ed.bau.gpuBacken;   // Stufe 8d
    g_lauf.autoCpuMs = autoCpu;
    g_lauf.autoGpuMs = autoGpu;
    g_lauf.editorDatenQuelle = edQuelle;
    g_lauf.editorDatenBsp = edBsp;
}

void bauEigeneBefehle(const std::string& eintrag, const std::vector<std::string>& befehle, std::function<void(int)> danach) {
    if (bauLaeuft()) { bauAusfuehren(std::vector<std::string>{}); }
    g_lauf.eintrag = eintrag;
    g_lauf.bspName.clear();
    g_lauf.kartenPfad = e().name;
    bauAusfuehren(befehle);
    g_lauf.gpuModus = karte::GpuBacken::Aus;   // Stufe 8d: eigene Auftraege ohne GPU-Wahl
    g_lauf.editorDatenQuelle.clear();
    g_lauf.danach = std::move(danach);   // nach bauAusfuehren: ein abgebrochener alter Auftrag ruft ihn nicht
}

// --- AutoCaulk Selected (map.cpp:2121-2368 map_autocaulk_selected, F4) -------------
//
// Die Auswahl (ohne trigger_*) als eigene Karte <Karte>_ac.map mit einem Kasten
// und einem Startpunkt am Auge, "q3map2 -game quake3 ... -autocaulk -fulldetail"
// darauf (wartet wie Q_Exec( ..., waitfor = true )), dann <Karte>_ac.caulk: je
// Flaeche, die von innen nicht zu sehen ist, caulk (bzw. watercaulk, nodraw ...).

void autoCaulkAuswahl() {
    Editor& ed = e();
    if (ed.name == "unnamed.map") {   // Map_Unnamed: Map_SaveAs
        if (!karteSpeichernUnterDialog()) { return; }
    }
    karte::triggerAbwaehlen(ed.karte);   // DeselectTriggers
    auswahlGeaendert();
    if (ed.karte.auswahl.anzahl() == 0) {
        fehler("map_autocaulk_selected(): nothing is selected");
        return;
    }
    // PathExtensionless( g_map.m_name ) + "_ac.map" - Abweichung: wie beim Bauen
    // nie im Spielordner (dann im Arbeitsordner), mit Ziel-Ordner dort
    std::string hinweis;
    const std::string ordner = bauOrdnerVorschau(&hinweis);
    if (!hinweis.empty()) { warnung(hinweis); }
    const std::string name = mitSchraegstrich(ed.name);
    std::string stamm = name.substr(0, name.rfind('.') == std::string::npos ? name.size() : name.rfind('.'));
    if (!ordner.empty()) {
        std::error_code ec;
        fs::create_directories(pfadAus(ordner), ec);
        const std::u8string s = pfadAus(ed.name).stem().u8string();
        stamm = mitSchraegstrich(ordner);
        if (!stamm.empty() && stamm.back() != '/') { stamm += '/'; }
        stamm += std::string(s.begin(), s.end());
    }
    const std::string datei = stamm + "_ac.map";
    {   // write .map
        const karte::Vektor3f spawn = ed.kamera.ursprung;   // Camera_getOrigin
        const karte::Kasten b = karte::auswahlGrenzenPivot(ed.karte);   // Select_GetBounds
        const karte::Vektor3f mins = b.origin - b.extents - karte::Vektor3f(1024, 1024, 1024);
        const karte::Vektor3f maxs = b.origin + b.extents + karte::Vektor3f(1024, 1024, 1024);
        const karte::Kasten k = karte::kastenAusMinMax(mins, maxs);
        bool innen = true;
        for (std::size_t i = 0; i < 3; ++i) { innen = innen && std::fabs(spawn[i] - k.origin[i]) < k.extents[i]; }   // aabb_intersects_point
        if (!innen) {
            fehler("map_autocaulk_selected(): camera must be near selection!");
            return;
        }
        const std::vector<std::shared_ptr<karte::Pinsel>> pinsel = karte::autocaulkPinsel(ed.karte);
        std::vector<const karte::Pinsel*> liste;
        for (const auto& p : pinsel) { liste.push_back(p.get()); }
        std::array<std::shared_ptr<karte::Pinsel>, 6> kasten;
        std::array<karte::Pinsel*, 6> roh{};
        std::array<const karte::Pinsel*, 6> konst{};
        for (std::size_t i = 0; i < 6; ++i) {
            kasten[i] = std::make_shared<karte::Pinsel>();
            roh[i] = kasten[i].get();
            konst[i] = kasten[i].get();
        }
        std::string w;
        karte::regionPinselBauen(roh, mins, maxs, karte::regionKastenShader(ed.regionKastenShader, &w), ed.karte.kontext);   // ConstructRegionBrushes
        if (!w.empty()) { warnung(w); }
        const std::string text = karte::autocaulkKarteSchreiben(ed.karte, liste, konst, spawn,
                                                                [&](const karte::Entitaet& en) { return karte::autocaulkPunktEntitaet(ed.karte, en); });
        std::ofstream f(pfadAus(datei), std::ios::binary | std::ios::trunc);   // TextFileOutputStream ("wt")
        const std::string roh2 = karte::textmodusSchreiben(text);
        if (!f || !f.write(roh2.data(), static_cast<std::streamsize>(roh2.size()))) {
            fehler("writing " + datei + " failure");
            return;
        }
    }
    std::string ausgabeQ3;
    std::string grund;
    bool gestartet = false;
    int rueckgabe = 0;
    {   // Stufe 6 (eigener Fund, auch in NRC): die .caulk eines frueheren Laufs weg - schreibt
        // q3map2 keine neue (Leck, q3map2 fehlt), las "load" sonst die alte und caulkte damit
        // stillschweigend Flaechen der jetzigen Auswahl
        std::error_code ec;
        fs::remove(pfadAus(stamm + "_ac.caulk"), ec);
    }
    {   // compile: Q_Exec( nullptr, cmd, nullptr, false, true ) - ohne Fenster, warten
        std::string programm = mitSchraegstrich(platform::executableDirectory());
        if (!programm.empty() && programm.back() != '/') { programm += '/'; }
        // Stufe 6 (Fund): NRC ruft "-game quake3" fuer jedes Spiel. Bei Jedi Academy/Jedi
        // Outcast liegen die Shader in shaders/, q3map2 -game quake3 liest scripts/ - es
        // kannte dann kein Wasser, keine Lava, nichts Durchsichtiges und schrieb caulk
        // (fest) auf verdeckte Flaechen von Wasserpinseln statt watercaulk. Jetzt der
        // q3map2-Typ des Spiels wie beim Bauen und Oeffnen einer .bsp.
        const char* spiel = kartenSpielNummer() == kRegelwerkJediOutcast ? "jk2" : "ja";
        const std::string cmd = "\"" + programm + "bhed_q3map2.exe\" -game " + spiel + " -fs_basepath \"" + enginePfadAnzeige() +
                                "\" -fs_homepath \"\" -fs_game " + bauSpielName() + " -autocaulk -fulldetail \"" + datei + "\"";
        bauplattform::Prozess p;
        // NRC#213 (Stufe 6): die Ausgabe von q3map2 mitlesen - scheitert es, nennt die
        // Konsole den Grund statt nur "reading ..._ac.caulk failure".
        if (p.starten(cmd, std::string(), true, grund)) {
            // Abweichung: hoechstens 10 Minuten (NRC wartet ohne Grenze)
            const auto start = std::chrono::steady_clock::now();
            while (!p.warten(50)) {
                ausgabeQ3 += p.lesen();
                if (std::chrono::steady_clock::now() - start > std::chrono::minutes(10)) {
                    p.beenden();
                    grund = "timeout after 10 minutes";
                    break;
                }
            }
            ausgabeQ3 += p.lesen();
            gestartet = true;
            rueckgabe = p.rueckgabe();
        }
    }
    std::map<std::size_t, std::string> caulk;
    {   // load
        const std::string datei2 = stamm + "_ac.caulk";
        std::string text;
        if (!dateiLesen(datei2, text)) {
            fehler("reading " + datei2 + " failure");
            if (!gestartet) {
                fehler("map_autocaulk_selected(): bhed_q3map2 could not be started" + (grund.empty() ? std::string() : ": " + grund));
            } else {
                fehler("map_autocaulk_selected(): bhed_q3map2 -autocaulk ended with code " + std::to_string(rueckgabe) +
                       (grund.empty() ? std::string() : " (" + grund + ")") + ", its last lines:");
                std::vector<std::string> zeilen;
                std::size_t a = 0;
                while (a < ausgabeQ3.size()) {
                    std::size_t e2 = ausgabeQ3.find('\n', a);
                    if (e2 == std::string::npos) { e2 = ausgabeQ3.size(); }
                    std::string z = ausgabeQ3.substr(a, e2 - a);
                    while (!z.empty() && (z.back() == '\r' || z.back() == ' ')) { z.pop_back(); }
                    if (!z.empty()) { zeilen.push_back(z); }
                    a = e2 + 1;
                }
                for (std::size_t i = zeilen.size() > 8 ? zeilen.size() - 8 : 0; i < zeilen.size(); ++i) { fehler("  " + zeilen[i]); }
            }
            return;
        }
        caulk = karte::caulkLesen(karte::textmodusLesen(text));
    }
    (void)karte::caulkAnwenden(ed.karte, caulk, [&ed](const std::string& shader) {   // apply: "AutoCaulk N faces"
        return !ed.karteSpiel || ed.karteSpiel->inventar.shaderFinden(shader) != nullptr;
    });
    ++ed.sichtStand;
}

}  // namespace bhed::gui::karteneditor
