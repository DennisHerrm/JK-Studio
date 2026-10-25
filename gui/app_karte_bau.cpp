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

#include "app_internal.h"
#include "platform.h"

#include "bhed/karte/bau.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/zuletzt.h"
#include "bhed/regelwerk.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
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
    return mitSchraegstrich(ed.bauKarte.empty() ? ed.name : ed.bauKarte);
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
    const ImGuiIO& io = ImGui::GetIO();
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

// --- RunBSP (qe3.cpp:235-262) bis zu den Befehlen ----------------------------------

void bauStarten(std::size_t index) {
    Editor& ed = e();
    bauMenueSicherstellen();
    // if( !g_region_active ) SaveMap();
    if (!ed.region.aktiv) { (void)karteSpeichern(); }
    if (ed.name == "unnamed.map") {   // Map_Unnamed
        fehler("build cancelled: the map is unnamed");
        return;
    }
    if (ed.region.aktiv) {   // Map_SaveRegion( <map>.reg )
        const std::string karte = ed.name.substr(0, ed.name.rfind('.') == std::string::npos ? ed.name.size() : ed.name.rfind('.'));
        (void)regionSpeichernUnter(karte + ".reg");
    }
    if (index < ed.bauProjekt.eintraege.size()) { ed.bauZuletzt = static_cast<long long>(index); }   // g_lastExecutedBuild
    std::vector<std::string> meldungen;
    const std::vector<std::string> befehle = karte::bauBefehle(ed.bauProjekt, index, bauVariablen(), &meldungen);
    for (const std::string& m : meldungen) { fehler(m); }
    bauAusfuehren(befehle);
}

// BuildMonitor_Run: Punkt 3 baut die Befehle; das Ausfuehren mit Ueberwachung
// kommt in Punkt 4 (bis dahin nur die Befehle in der Konsole).
void bauAusfuehren(const std::vector<std::string>& befehle) {
    for (const std::string& b : befehle) {
        ausgabe("=== running build command ===");
        ausgabe(b);
    }
}

}  // namespace bhed::gui::karteneditor
