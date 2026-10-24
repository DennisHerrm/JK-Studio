// app_karte_menue.cpp - Karten-Editor: Farben, Symbole, Menueleiste,
// Werkzeugleisten und Statuszeile wie NetRadiant-Custom.
//
// Portiert aus NetRadiant-Custom (Stand master 2026): radiant/mainframe.cpp
// (create_main_menu, create_main_toolbar, create_main_statusbar,
// MainFrame::SetGridStatus), radiant/filterbar.cpp (create_filter_toolbar,
// FilterToolbarHandler: Rechtsklick), radiant/gtkmisc.cpp (ToolbarItems:
// versteckte Knoepfe, Kontextmenue der Leisten, toolbar_action_set_tooltip),
// radiant/colors.cpp (g_ColoursMenu, Viewports-Themen als Menueeintraege),
// radiant/qe3.cpp (QE_brushCountChange),
// radiant/mru.cpp (Menueeintraege).
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

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

namespace bhed::gui::karteneditor {

using befehle::Knopf;
using befehle::MenueArt;
using befehle::MenueEintrag;

namespace {

Editor& e() { return editor(); }

// Die Farben kommen aus dem Farbschema des Programms (bhed/theme.h, Ansicht >
// Farbgebung) - Entscheidung shank: die Farben muessen nicht wie NRC sein,
// Anordnung, Menues, Knoepfe und Bedienung schon.
ImU32 stil(ImGuiCol c, float a = 1.0F) { return ImGui::GetColorU32(c, a); }

bool hellesThema() {
    const ImVec4 w = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    return 0.2126F * w.x + 0.7152F * w.y + 0.0722F * w.z > 0.5F;
}

// --- Symbole ----------------------------------------------------------------
// Eigene Vektorsymbole im Stil des Programms (karte_vektoricons.cpp) statt
// NRCs Bitmaps - shank: "moderner ... aber dieselbe Positionen". Gesperrt
// (QIcon::Disabled) in der Farbe fuer gesperrten Text.
void symbolZeichnen(ImDrawList* dl, const char* name, ImVec2 a, float g, bool aktiv) {
    const ImVec4 f = aktiv ? karteIconFarbe(name) : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
    (void)karteIcon(dl, name, ImVec2{std::floor(a.x), std::floor(a.y)}, g, ImGui::GetColorU32(f));
}

std::string stufeText(const std::string& befehl) {
    const befehle::Befehl* b = befehle::finden(befehl);
    char z[160];
    // Plugin-Befehle ("bobToolz::...") stehen nicht in der Tabelle: Stufe 3.
    const int stufe = b != nullptr ? b->stufe : (befehl.find("::") != std::string::npos ? 3 : 2);
    std::snprintf(z, sizeof(z), tr(Str::KeStufe), stufe);
    return z;
}

// --- Menues -------------------------------------------------------------------
//
// Gezeichnet wie QMenu (app_karte_qtmenue.cpp): Zeilen, Trenner, Breite,
// Symbol, Haken und Mnemonik nach Qt 5.15 / Fusion / dark.qss.

// Das Symbol eines Befehls, wenn er einen Werkzeugknopf hat (QMenu zeigt das
// Symbol der Aktion, dasselbe QAction wie der Werkzeugknopf, z. B. Modify >
// Components).
const char* knopfBild(const std::string& befehl) {
    static std::map<std::string, const char*> bilder;
    if (bilder.empty()) {
        for (const auto* l : {&befehle::werkzeugleiste(), &befehle::filterleiste(), &befehle::pluginleiste()}) {
            for (const Knopf& k : *l) {
                if (!k.trenner && k.befehl[0] != 0 && k.bild[0] != 0) { bilder.emplace(k.befehl, k.bild); }
            }
        }
    }
    const auto it = bilder.find(befehl);
    return it != bilder.end() ? it->second : nullptr;
}

// Exklusive Aktionen (QActionGroup) zeigen einen Punkt statt des Kaestchens:
// das Raster (grid.cpp:224-227) und das GUI-Thema (theme.cpp:160).
bool exklusiv(const std::string& befehl) { return befehl.rfind("SetGrid", 0) == 0; }

void listeZeichnen(const std::string& name);

// Die Eintraege ab i bis zum passenden Ende (create_*_menu).
void eintraegeZeichnen(const std::vector<MenueEintrag>& liste, std::size_t& i) {
    Editor& ed = e();
    bool letzterTrenner = true;   // separatorsCollapsible: keine Trenner am Anfang/doppelt
    for (; i < liste.size(); ++i) {
        const MenueEintrag& m = liste[i];
        if (m.art == MenueArt::Ende) { return; }
        if (!befehle::bedingungGilt(m.bedingung, ed.anordnung, ed.gruppenDialogEingebaut)) {
            if (m.art == MenueArt::Unter) {   // ganzen Zweig ueberspringen
                int tiefe = 1;
                while (tiefe > 0 && ++i < liste.size()) {
                    if (liste[i].art == MenueArt::Unter) { ++tiefe; }
                    if (liste[i].art == MenueArt::Ende) { --tiefe; }
                }
            }
            continue;
        }
        switch (m.art) {
        case MenueArt::Trenner: {
            // Nur zeichnen, wenn danach noch etwas Sichtbares kommt.
            bool folgt = false;
            for (std::size_t k = i + 1; k < liste.size() && liste[k].art != MenueArt::Ende; ++k) {
                if (liste[k].art != MenueArt::Trenner &&
                    befehle::bedingungGilt(liste[k].bedingung, ed.anordnung, ed.gruppenDialogEingebaut)) {
                    folgt = true;
                    break;
                }
            }
            if (!letzterTrenner && folgt) {
                qtTrenner();
                letzterTrenner = true;
            }
            break;
        }
        case MenueArt::Punkt:
        case MenueArt::Haken: {
            const std::string befehl = m.befehl;
            const bool da = nachgebaut(befehl);
            const std::string taste = befehle::tasteAlsText(ed.belegung.taste(befehl));
            const std::string id = std::string(trK(m.str)) + "###" + befehl + std::to_string(i);
            QtPunkt p;
            p.id = id.c_str();
            p.nrc = m.text;
            p.taste = taste.c_str();
            p.haken = m.art == MenueArt::Haken && angehakt(befehl);
            p.aktiv = da;
            p.art = m.art == MenueArt::Haken ? (exklusiv(befehl) ? QtArt::Exklusiv : QtArt::Pruefbar) : QtArt::Normal;
            p.bild = knopfBild(befehl);
            if (!da) { p.tooltip = stufeText(befehl); }
            if (qtPunkt(p)) { (void)ausfuehren(befehl); }
            letzterTrenner = false;
            break;
        }
        case MenueArt::Ruf: {
            // Eigene Rueckrufe (Set 2D Background image, Bug report, About).
            const std::string id = std::string(trK(m.str)) + "###ruf" + std::to_string(i);
            const bool da = nachgebaut(m.befehl);
            QtPunkt p;
            p.id = id.c_str();
            p.nrc = m.text;
            p.aktiv = da;
            if (!da) { p.tooltip = stufeText(m.befehl); }
            if (qtPunkt(p)) { (void)ausfuehren(m.befehl); }
            letzterTrenner = false;
            break;
        }
        case MenueArt::Unter: {
            const std::string id = std::string(trK(m.str)) + "###unter" + std::to_string(i);
            if (qtMenueAnfang(id.c_str(), true, m.text)) {
                ++i;
                eintraegeZeichnen(liste, i);
                qtMenueEnde();
            } else {
                int tiefe = 1;
                while (tiefe > 0 && ++i < liste.size()) {
                    if (liste[i].art == MenueArt::Unter) { ++tiefe; }
                    if (liste[i].art == MenueArt::Ende) { --tiefe; }
                }
            }
            letzterTrenner = false;
            break;
        }
        case MenueArt::Liste:
            listeZeichnen(m.befehl);
            letzterTrenner = false;
            break;
        case MenueArt::Menue:
        case MenueArt::Ende:
            break;
        }
    }
}

// Ein ausgegrauter Eintrag einer Liste.
void grauerPunkt(const char* text, const std::string& tooltip, QtArt art = QtArt::Normal, const float* farbe = nullptr) {
    QtPunkt p;
    p.id = text;
    p.aktiv = false;
    p.art = art;
    p.farbe = farbe;
    p.tooltip = tooltip;
    (void)qtPunkt(p);
}

// --- Build-Menue (build.cpp:1224 Build_constructMenu) ---------------------------
//
// LoadBuildMenu (build.cpp:1276-1293): ohne eigene build_menu.xml die
// default_build_menu.xml des Spiels (NRC gamepacks/ja.game bzw. jk2.game,
// hier data/karte/ja und data/karte/jk2). <build name> wird ein Eintrag mit
// den Befehlen als Tooltip, <separator/> ein Trenner. Ausfuehren kommt mit
// dem Kompilieren (Stufe 3).
struct BuildEintrag {
    std::string name;   // leer = Trenner
    std::string befehle;
};

std::string xmlText(const std::string& s) {
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '&') {
            out.push_back(s[i]);
            continue;
        }
        static const char* const kEnt[][2] = {{"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&apos;", "'"}};
        bool ersetzt = false;
        for (const auto& en : kEnt) {
            if (s.compare(i, std::strlen(en[0]), en[0]) == 0) {
                out += en[1];
                i += std::strlen(en[0]) - 1;
                ersetzt = true;
                break;
            }
        }
        if (!ersetzt) { out.push_back('&'); }
    }
    return out;
}

const std::vector<BuildEintrag>& buildEintraege() {
    static std::vector<BuildEintrag> liste;
    static int fuer = -2;
    if (fuer == e().spielStand) { return liste; }
    fuer = e().spielStand;
    liste.clear();
    const std::string datei = directoryOf(g_app->dataDir) +
                              (e().spielStand == kRegelwerkJediOutcast ? "/karte/jk2" : "/karte/ja") + "/default_build_menu.xml";
    std::ifstream f(datei, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    const std::string x = ss.str();
    std::size_t pos = 0;
    while ((pos = x.find('<', pos)) != std::string::npos) {
        if (x.compare(pos, 4, "<!--") == 0) {
            const std::size_t ende = x.find("-->", pos);
            if (ende == std::string::npos) { break; }
            pos = ende + 3;
            continue;
        }
        if (x.compare(pos, 10, "<separator") == 0) {
            liste.push_back({});
        } else if (x.compare(pos, 7, "<build ") == 0) {
            const std::size_t n = x.find("name=\"", pos);
            const std::size_t ende = x.find("</build>", pos);
            if (n == std::string::npos || ende == std::string::npos) { break; }
            BuildEintrag b;
            b.name = xmlText(x.substr(n + 6, x.find('"', n + 6) - (n + 6)));
            // Tooltip: die Befehle, je eine Zeile (Build_constructMenu)
            std::size_t c = pos;
            while ((c = x.find("<command>", c)) != std::string::npos && c < ende) {
                const std::size_t ce = x.find("</command>", c);
                if (!b.befehle.empty()) { b.befehle += '\n'; }
                b.befehle += xmlText(x.substr(c + 9, ce - (c + 9)));
                c = ce;
            }
            liste.push_back(b);
            pos = ende;
        }
        ++pos;
    }
    return liste;
}

// --- dynamische Listen ---------------------------------------------------------

// themes/_colors/*.json (Directory_forEach, alphabetisch)
const char* const kViewportThemen[] = {"Adwaita Dark", "Black and Green", "Blender Dark", "Blender Lighter", "Green",
                                       "Maya.Max.Lightwave Emulation", "Q3Radiant Original", "QE4 Original"};

void listeZeichnen(const std::string& name) {
    Editor& ed = e();
    if (name == "MRU") {
        // MRU_constructMenu (mru.cpp:186-236): "&1" .. "&9"
        if (!ed.zuletztGeladen) {
            ed.zuletztGeladen = true;
            ed.zuletzt.setzen(g_app->settings.karteZuletzt);
        }
        const karte::ZuletztListe& z = ed.zuletzt;
        for (std::size_t i = 0; i < karte::ZuletztListe::kMax; ++i) {
            if (i > 0 && i >= z.anzahl()) { break; }
            const std::string mitUnd = z.beschriftung(i);
            const std::string roh = karte::ohneMnemonik(mitUnd);
            const std::string text = (roh == "Recent Files" ? std::string(trK(Str::KeRecentFiles)) : roh) + "###karteMru" +
                                     std::to_string(i);
            QtPunkt p;
            p.id = text.c_str();
            p.nrc = mitUnd.c_str();
            p.aktiv = i < z.anzahl();
            if (qtPunkt(p)) { zuletztOeffnen(i); }
        }
    } else if (name == "Viewportthemen") {
        // themes/_colors/*.json (colors.cpp:218-226) - die Farben folgen hier
        // dem Farbschema des Programms (Entscheidung shank), die Eintraege
        // stehen da, ausgegraut.
        for (const char* t : kViewportThemen) { grauerPunkt(t, tr(Str::KeFarbenProgramm)); }
    } else if (name == "GuiThemen") {
        // theme_construct_menu (theme.cpp, NRC 20260114: vier Themen in einer
        // QActionGroup; die drei weiteren gibt es erst in master)
        for (const char* t : {"Default", "Fusion", "Dark", "Darker"}) { grauerPunkt(t, tr(Str::KeFarbenProgramm), QtArt::Exklusiv); }
    } else if (name == "Farben") {
        // g_ColoursMenu (colors.cpp:153-171): je Farbe ein Eintrag, das
        // Farbfeld als Symbol der Aktion.
        for (const FarbEintrag& f : farbEintraege()) {
            const Farben::F3& c = ed.farben.*(f.feld);
            grauerPunkt(f.text, tr(Str::KeFarbenProgramm), QtArt::Normal, c.data());
        }
    } else if (name == "Transforms") {
        // Modify > Repeat (mainframe.cpp:1116-1131): vier Eintraege, deren Text
        // TransformsObserved (selection.cpp:918-940) setzt - ' ' bzw. 'x' (nicht
        // die Identitaet), dann die wiederholbare Verschiebung, Drehung,
        // Skalierung, Scherung; resetTransforms() setzt sie gleich zu Beginn.
        // Ein Klick setzt nur diesen Teil zurueck (resetTransforms( mode )).
        for (int i = 0; i < 4; ++i) {
            const std::string t = ed.bearbeitung.transformText(i);
            const std::string id = t + "###karteTransform" + std::to_string(i);
            QtPunkt p;
            p.id = id.c_str();
            p.nrc = t.c_str();
            if (qtPunkt(p)) { ed.bearbeitung.transformZuruecksetzen(i); }
        }
    } else if (name == "Build") {
        bool letzterTrenner = true;   // der Trenner davor steht in der Tabelle
        const auto& liste = buildEintraege();
        for (std::size_t i = 0; i < liste.size(); ++i) {
            const BuildEintrag& b = liste[i];
            if (b.name.empty()) {
                if (!letzterTrenner && i + 1 < liste.size()) { qtTrenner(); }
                letzterTrenner = true;
                continue;
            }
            const std::string id = b.name + "##build" + std::to_string(i);
            QtPunkt p;
            p.id = id.c_str();
            p.nrc = b.name.c_str();
            p.aktiv = false;
            p.tooltip = b.befehle + "\n" + stufeText("BuildMenuCustomize");
            (void)qtPunkt(p);
            letzterTrenner = false;
        }
    } else if (name == "Plugins") {
        // create_plugins_menu (pluginmenu.cpp:131) mit PluginsMenu_populate:
        // je Plugin ein Untermenue (Modulname), Eintraege von hinten nach
        // vorn (PlugInMenu_Add); die Plugins kommen in Stufe 3.
        const auto& liste = befehle::pluginEintraege();
        for (std::size_t i = 0; i < liste.size();) {
            const std::string menue = liste[i].menue;
            std::size_t j = i;
            while (j < liste.size() && menue == liste[j].menue) { ++j; }
            if (qtMenueAnfang((menue + "##plugin").c_str())) {
                bool letzterTrenner = true;
                for (std::size_t k = i; k < j; ++k) {
                    if (liste[k].titel[0] == 0) {
                        if (!letzterTrenner) { qtTrenner(); }
                        letzterTrenner = true;
                        continue;
                    }
                    const std::string text = karte::ohneMnemonik(liste[k].titel) + "##pl" + liste[k].befehl;
                    const std::string befehl = liste[k].befehl;
                    const bool da = nachgebaut(befehl);
                    const std::string taste = befehle::tasteAlsText(e().belegung.taste(befehl));
                    QtPunkt p;
                    p.id = text.c_str();
                    p.nrc = liste[k].titel;
                    p.taste = taste.c_str();
                    p.aktiv = da;
                    p.bild = knopfBild(befehl);
                    if (!da) { p.tooltip = stufeText(befehl); }
                    if (qtPunkt(p)) { (void)ausfuehren(befehl); }
                    letzterTrenner = false;
                }
                qtMenueEnde();
            }
            i = j;
        }
    } else if (name == "Hilfe") {
        // create_game_help_menu (help.cpp:112): global.xlink "General" und
        // game.xlink des Spiels - Verweise auf NRCs docs/ und das JA-SDK.
        static const char* const kGeneral[] = {
            "Mouse Shortcuts", "Complete list of Q3map2 command line parameters", "Complete list of entity keys",
            "Shader Manual", "q3map2 handbook", "Additional map compiler features", "Additional map editor features",
            "Editor features presentation", "Q3Map2 - FS20 - R5 readme", "Blendmodes cheatsheet", "MBSPC documentation",
            "iD Tech 3 mapping trello"};
        if (qtMenueAnfang("General")) {
            for (const char* t : kGeneral) { grauerPunkt(t, tr(Str::KeNichtDabei)); }
            qtMenueEnde();
        }
        static const char* const kJa[] = {"Tutorial: Mapping in NRC for JKA/MBII", "About Jedi Academy editing package",
                                          "Jedi Academy Tools disclaimer", "Jedi Academy SDK readme",
                                          "Bot waypoint tutorial", "ICARUS Manual", "Using EffectsEd", "How to play Siege",
                                          "Jedi Academy forum @ Raven Software",
                                          "Jedi Academy Editing forum @ MapCenter"};
        // game.xlink des Spiels unter dessen Namen (ja.game: name="Jedi
        // Knight Jedi Academy"); jk2.game hat keine game.xlink - kein Untermenue.
        if (e().spielStand != kRegelwerkJediOutcast && qtMenueAnfang("Jedi Knight Jedi Academy")) {
            for (const char* t : kJa) { grauerPunkt(t, tr(Str::KeNichtDabei)); }
            qtMenueEnde();
        }
    }
}

}  // namespace

// --- Farben (colors.cpp:153-171) ----------------------------------------------

const std::vector<FarbEintrag>& farbEintraege() {
    static const std::vector<FarbEintrag> v = {
        {"Texture Browser Background...", "ColorTexBroBackground", &Farben::texBro},
        {"Model Browser Background...", "ColorModBroBackground", &Farben::modBro},
        {"Camera Background...", "ColorCameraBackground", &Farben::kameraHinten},
        {"Grid Background...", "ColorGridBackground", &Farben::rasterHinten},
        {"Grid Major...", "ColorGridMajor", &Farben::rasterGross},
        {"Grid Minor...", "ColorGridMinor", &Farben::rasterKlein},
        {"Grid Text...", "ColorGridText", &Farben::rasterText},
        {"Grid _blocksize Blocks...", "ColorGridBlocks", &Farben::rasterBlock},
        {"Grid Worldspawn...", "ColorGridWorldspawn", &Farben::pinsel},
        {"Grid Selection and Sizing...", "ColorGridSelection", &Farben::auswahl},
        {"Camera Selection...", "ColorCameraSelection", &Farben::auswahl3d},
        {"Clipper Split...", "ColorClipperSplit", &Farben::clipper},
        {"Active Grid Name and Outline...", "ColorGridActive", &Farben::ansichtName},
        {"Camera Icon...", "ColorCameraIcon", &Farben::kamera},
        {"Axis X...", "ColorAxisX", &Farben::achseX},
        {"Axis Y...", "ColorAxisY", &Farben::achseY},
        {"Axis Z...", "ColorAxisZ", &Farben::achseZ},
    };
    return v;
}

// --- Farben der Ansichten aus dem Farbschema des Programms ------------------
//
// NRCs Rollen (g_xywindow_globals, g_camwindow_globals, colors.cpp) mit
// Werten aus dem Farbschema: Hintergrund der 2D-Ansicht = Listenflaeche,
// Rasterlinien = Text, schwach bzw. staerker eingemischt, Auswahl und aktive
// Ansicht = Hervorhebung des Schemas. Die Achsen bleiben rot, gruen, blau
// (render.h:291), die Entities behalten die Farben ihrer Klassen.
void farbenAusThema() {
    Editor& ed = e();
    const auto f3 = [](const ImVec4& v) { return Farben::F3{v.x, v.y, v.z}; };
    const auto mische = [](const ImVec4& a, const ImVec4& b, float t) {
        return Farben::F3{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
    };
    const ImVec4 hinten = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg).w > 0.5F ? ImGui::GetStyleColorVec4(ImGuiCol_ChildBg)
                                                                             : ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    const ImVec4 text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    ImVec4 akzent = ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
    if (const theme::Theme* t = g_app->activeTheme) { akzent = ImVec4{t->accent.r, t->accent.g, t->accent.b, 1.0F}; }
    Farben& f = ed.farben;
    f.rasterHinten = f3(hinten);
    f.rasterKlein = mische(hinten, text, 0.07F);
    f.rasterGross = mische(hinten, text, 0.16F);
    f.rasterText = f3(text);
    f.rasterBlock = f3(akzent);
    f.pinsel = mische(hinten, text, 0.8F);
    f.auswahl = f3(akzent);
    f.auswahl3d = f3(akzent);
    f.clipper = mische(akzent, text, 0.35F);
    f.ansichtName = f3(akzent);
    f.kamera = f3(akzent);
    f.kameraHinten = f3(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
    f.texBro = f3(hinten);
    f.modBro = f3(hinten);
}

bool symbol(const char* name, float groesse, bool hell) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec4 f = hell ? karteIconFarbe(name) : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
    const bool da = karteIcon(ImGui::GetWindowDrawList(), name, ImVec2{std::floor(p.x), std::floor(p.y)}, groesse,
                              ImGui::GetColorU32(f));
    ImGui::Dummy(ImVec2{groesse, groesse});
    return da;
}

// --- Menueleiste (create_main_menu, mainframe.cpp:1225) ---------------------
//
// Lage der Eintraege wie QMenuBarPrivate::calcActionRects (qmenubar.cpp:
// 409-480): Feld = Textbreite + 8 (CT_MenuBarItem, Fusion), Abstand 6
// (PM_MenuBarItemSpacing), der erste bei 6; der Text mittig, also 4 nach dem
// Feldanfang. ImGui verlaengert die Hervorhebung eines Menuetitels um
// ItemSpacing.x nach beiden Seiten (BeginMenu verdoppelt es fuer Selectable),
// darum ItemSpacing.x = 4: Hervorhebung = Text + 8 = Qts Feld.

void zeichneMenueleiste() {
    if (!ImGui::BeginMenuBar()) { return; }
    const float s = massstab();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2{static_cast<float>(mass::kMenuePolster) * 0.5F * s, ImGui::GetStyle().ItemSpacing.y});
    const float halb = std::trunc(static_cast<float>(mass::kMenuePolster) * 0.5F * s * 0.5F);   // BeginMenu: += IM_TRUNC(ItemSpacing.x * 0.5)
    const float zeile = ImGui::GetCursorScreenPos().y;
    int x = mass::kMenueAbstand;
    const auto& liste = befehle::menues();
    for (std::size_t i = 0; i < liste.size(); ++i) {
        if (liste[i].art != MenueArt::Menue) { continue; }
        const char* text = trK(liste[i].str);
        // fm.size(): boundingRect(...).size() - das Rechteck auf ganze
        // logische Punkte aufgerundet (QRectF::toAlignedRect). Am Foto
        // nachgemessen: aufgerundet stehen alle zwoelf Titel auf dem Punkt.
        const int w = static_cast<int>(std::ceil(ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.0F, text).x / s - 0.001F));
        merkeFeld(std::string("Menue:") + text, x, 0, w + mass::kMenuePolster, mass::kMenueH);
        ImGui::SetCursorScreenPos(ImVec2{bildpunkt(x, 0).x + static_cast<float>(mass::kMenuePolster) * 0.5F * s - halb, zeile});
        const std::string id = std::string(text) + "###menue" + std::to_string(i);
        if (e().menueOeffnen == static_cast<int>(i)) {   // Alt + Mnemonik (QMenuBar)
            ImGui::OpenPopup(id.c_str());
            e().menueOeffnen = -1;
        }
        // CE_MenuBarItem (Fusion): Text mit Qt::TextShowMnemonic - unterstrichen
        const ImVec2 textPos{bildpunkt(x, 0).x + static_cast<float>(mass::kMenuePolster) * 0.5F * s,
                             zeile + ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset};
        qtUnterstreichen(ImGui::GetWindowDrawList(), textPos, text, qtMnemonik(liste[i].text, text), stil(ImGuiCol_Text));
        if (qtMenueAnfang(id.c_str(), true, liste[i].text)) {
            ++i;
            eintraegeZeichnen(liste, i);
            qtMenueEnde();
        }
        x += w + mass::kMenuePolster + mass::kMenueAbstand;
    }
    ImGui::PopStyleVar();
    ImGui::EndMenuBar();
}

// --- Werkzeugleisten -------------------------------------------------------------
//
// dark.qss "QToolBar": Rand 1, padding 3, spacing 0; Griff 9
// (PM_ToolBarHandleExtent); Knoepfe 31 x 31 mit Symbol 24; Trenner 8.
// Erster Knopf also 13 vom Rand, 4 von oben (gemessen: 13 und 4).

namespace {

bool g_rechtsklickBehandelt = false;

// Ein Knopf einer QToolBar an der Stelle (x, y) in logischen Punkten.
void knopf(const Knopf& k, int x, int y) {
    Editor& ed = e();
    const float s = massstab();
    const std::string befehl = k.befehl;
    const bool da = befehl.empty() ? false : nachgebaut(befehl);
    ed.knoepfeImBild.push_back(befehl);
    merkeFeld("Knopf:" + (befehl.empty() ? std::string(k.bild) : befehl), x, y, mass::kKnopf, mass::kKnopf);
    const std::string id = std::string("##knopf") + (befehl.empty() ? k.bild : befehl);
    const ImVec2 a = bildpunkt(x, y);
    const ImVec2 b = bildpunkt(x + mass::kKnopf, y + mass::kKnopf);
    ImGui::SetCursorScreenPos(a);
    ImGui::InvisibleButton(id.c_str(), ImVec2{b.x - a.x, b.y - a.y}, ImGuiButtonFlags_MouseButtonLeft);
    const bool hover = ImGui::IsItemHovered();
    const bool an = k.schalter && angehakt(befehl);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // Knopfstil des Programms (iconKnopf, uiicons.cpp): ruhend ohne Flaeche,
    // eingeschaltet (QToolButton checked) in der Farbe gedrueckter Knoepfe,
    // unter der Maus hervorgehoben, abgerundet. Platz und Groesse wie NRC.
    const ImGuiStyle& st = ImGui::GetStyle();
    ImVec4 grund{0.0F, 0.0F, 0.0F, 0.0F};
    if (an) {
        grund = st.Colors[ImGuiCol_ButtonActive];
        grund.w = std::max(grund.w, 0.55F);
    }
    if (hover && da) { grund = st.Colors[ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered]; }
    if (grund.w > 0.0F) { dl->AddRectFilled(a, b, ImGui::GetColorU32(grund), st.FrameRounding + 2.0F); }
    // Das Symbol 24 x 24 mittig (QToolButton: Rand 1 + padding 1 + 1,5).
    const float g = std::round(static_cast<float>(mass::kSymbol) * s);
    symbolZeichnen(dl, k.bild, ImVec2{std::floor(a.x + (b.x - a.x - g) * 0.5F), std::floor(a.y + (b.y - a.y - g) * 0.5F)}, g, da);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && da) { (void)ausfuehren(befehl); }
    // FilterToolbarHandler (filterbar.cpp:104-118): Rechtsklick auf einen
    // Filterknopf mit eigener Handlung fuehrt sie beim Druecken (auch beim
    // Doppelklick) aus; das Kontextmenue (unter Windows beim Loslassen,
    // WM_CONTEXTMENU) wird dort geschluckt.
    if (hover && k.rechtsklick[0] != 0) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) { filterleisteRechtsklick(k); }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { g_rechtsklickBehandelt = true; }   // eventFilter: return true
    }
    if (hover) {
        std::vector<std::string> zeilen;
        std::stringstream ss(k.beschreibung);
        std::string zeile;
        std::size_t z = 0;
        while (std::getline(ss, zeile)) {
            const bool tab = !zeile.empty() && zeile[0] == '\t';
            std::string text = tab ? zeile.substr(1) : zeile;
            if (z < k.zeilen.size() && k.zeilen[z] != Str::Count) { text = tr(k.zeilen[z]); }
            zeilen.push_back((tab ? "    " : "") + text);
            ++z;
        }
        std::string tip = befehle::knopfTooltip(zeilen, befehle::tasteAlsText(ed.belegung.taste(befehl)));
        if (!da && !befehl.empty()) { tip += "\n" + stufeText(befehl); }
        ImGui::SetTooltip("%s", tip.c_str());
    }
}

// toolbar_construct_control_menu (gtkmisc.cpp:230-258) mit den Leisten davor
// (QMainWindow::createPopupMenu).
void leistenMenue() {
    Editor& ed = e();
    if (!qtPopupAnfang("##leistenmenue")) { return; }
    // toggleViewAction der Leisten (pruefbar)
    for (auto [text, an] : {std::pair<const char*, bool*>{tr(Str::KeMainToolbar), &ed.hauptleiste},
                            {tr(Str::KeFilterToolbar), &ed.filterleiste},
                            {tr(Str::KePluginToolbar), &ed.pluginleiste}}) {
        QtPunkt p;
        p.id = text;
        p.art = QtArt::Pruefbar;
        p.haken = *an;
        if (qtPunkt(p)) { *an = !*an; }
    }
    qtTrenner();
    // je Knopf eine pruefbare Aktion mit seinem Symbol, Trenner zwischen den
    // Leisten; "prevent closing the menu on clicks": das Menue bleibt offen.
    const std::vector<Knopf>* const listen[] = {&befehle::werkzeugleiste(), &befehle::filterleiste(), &befehle::pluginleiste()};
    bool davor = false;
    for (const auto* liste : listen) {
        bool erster = true;
        for (const Knopf& k : *liste) {
            if (k.trenner || k.befehl[0] == 0) { continue; }
            if (erster && davor) { qtTrenner(); }
            erster = false;
            davor = true;
            const bool sichtbar = ed.versteckteKnoepfe.count(k.befehl) == 0U;
            const std::string id = std::string(k.befehl) + "##lm";
            QtPunkt p;
            p.id = id.c_str();
            p.nrc = k.befehl;
            p.art = QtArt::Pruefbar;
            p.haken = sichtbar;
            p.bild = k.bild;
            ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
            const bool geklickt = qtPunkt(p);
            ImGui::PopItemFlag();
            if (geklickt) {
                if (sichtbar) { ed.versteckteKnoepfe.insert(k.befehl); } else { ed.versteckteKnoepfe.erase(k.befehl); }
                einstellungenSpeichern();
            }
        }
    }
    qtPopupEnde();
}

// Der Griff einer beweglichen QToolBar (PM_ToolBarHandleExtent 9): Punkte
// in zwei Reihen, in der Farbe fuer gesperrten Text.
void griff(ImDrawList* dl, int x, int y, int b, int h, bool waagerecht) {
    const float s = massstab();
    const ImU32 c = stil(ImGuiCol_TextDisabled);
    const float d = std::max(1.0F, std::round(s));
    const ImVec2 a = bildpunkt(x, y);
    const ImVec2 e2 = bildpunkt(x + b, y + h);
    const float mitte = waagerecht ? std::floor((a.x + e2.x) * 0.5F) : std::floor((a.y + e2.y) * 0.5F);
    const float lang0 = waagerecht ? a.y + 2.0F * d : a.x + 2.0F * d;
    const float lang1 = waagerecht ? e2.y - 2.0F * d : e2.x - 2.0F * d;
    for (float k = lang0; k + d <= lang1; k += 3.0F * d) {
        for (const float q : {mitte - 2.0F * d, mitte + d}) {
            if (waagerecht) {
                dl->AddRectFilled(ImVec2{q, k}, ImVec2{q + d, k + d}, c);
            } else {
                dl->AddRectFilled(ImVec2{k, q}, ImVec2{k + d, q + d}, c);
            }
        }
    }
}

// Die Knoepfe einer Leiste, die in dieser Anordnung erzeugt werden
// (create_main_toolbar fragt die Anordnung ab).
std::vector<const Knopf*> erzeugteKnoepfe(const std::vector<Knopf>& liste) {
    Editor& ed = e();
    std::vector<const Knopf*> v;
    for (const Knopf& k : liste) {
        if (befehle::bedingungGilt(k.bedingung, ed.anordnung, ed.gruppenDialogEingebaut)) { v.push_back(&k); }
    }
    return v;
}

// Was QToolBarLayout sieht: Knoepfe 31, Trenner 8; versteckt (leer) sind
// ausgeblendete Knoepfe und Trenner nach ToolbarItems::
// update_separators_visibility (gtkmisc.cpp:189-211): ein Trenner nur mit
// einem sichtbaren Knopf davor und einem bis zum naechsten Trenner danach.
std::vector<karte::LeistenEintrag> leistenEintraege(const std::vector<const Knopf*>& k) {
    Editor& ed = e();
    const std::size_t n = k.size();
    std::vector<bool> an(n, false);
    for (std::size_t i = 0; i < n; ++i) { an[i] = !k[i]->trenner && ed.versteckteKnoepfe.count(k[i]->befehl) == 0U; }
    for (std::size_t i = 0; i < n; ++i) {
        if (!k[i]->trenner) { continue; }
        bool danach = false;
        for (std::size_t j = i + 1; j < n && !k[j]->trenner; ++j) { danach = danach || an[j]; }
        bool davor = false;
        for (std::size_t j = 0; j < i; ++j) { davor = davor || an[j]; }
        an[i] = danach && davor;
    }
    std::vector<karte::LeistenEintrag> v;
    for (std::size_t i = 0; i < n; ++i) { v.push_back({k[i]->trenner ? mass::kTrenner : mass::kKnopf, !an[i]}); }
    return v;
}

// Der Erweiterungsknopf (QToolBarExtension, PM_ToolBarExtensionExtent 12):
// zwei Winkel in Richtung der verdeckten Knoepfe; Klick zeigt sie in einem
// Menue (QToolBarLayout mit popupMenu: Symbol und Text je Aktion).
void erweiterung(const char* id, const std::vector<const Knopf*>& verdeckt, int x, int y, int b, int h, bool waagerecht) {
    Editor& ed = e();
    const float s = massstab();
    merkeFeld(std::string("Erweiterung:") + id, x, y, b, h);
    const ImVec2 a = bildpunkt(x, y);
    const ImVec2 c = bildpunkt(x + b, y + h);
    ImGui::SetCursorScreenPos(a);
    const std::string kennung = std::string("##erweiterung") + id;
    const bool geklickt = ImGui::InvisibleButton(kennung.c_str(), ImVec2{c.x - a.x, c.y - a.y});
    const bool hover = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (hover) {
        dl->AddRectFilled(a, c, ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered),
                          ImGui::GetStyle().FrameRounding);
        ImGui::SetTooltip("%s", tr(Str::KeLeisteMehr));
    }
    const ImU32 farbe = ImGui::GetColorU32(ImGuiCol_Text);
    const float m = std::max(1.0F, std::round(s));
    const ImVec2 mitte{std::floor((a.x + c.x) * 0.5F), std::floor((a.y + c.y) * 0.5F)};
    for (int k = 0; k < 2; ++k) {
        const float o = (static_cast<float>(k) * 3.0F - 1.5F) * m;
        if (waagerecht) {   // ">>"
            dl->AddLine(ImVec2{mitte.x + o - 1.5F * m, mitte.y - 2.5F * m}, ImVec2{mitte.x + o + 1.0F * m, mitte.y}, farbe, m);
            dl->AddLine(ImVec2{mitte.x + o + 1.0F * m, mitte.y}, ImVec2{mitte.x + o - 1.5F * m, mitte.y + 2.5F * m}, farbe, m);
        } else {   // nach unten
            dl->AddLine(ImVec2{mitte.x - 2.5F * m, mitte.y + o - 1.5F * m}, ImVec2{mitte.x, mitte.y + o + 1.0F * m}, farbe, m);
            dl->AddLine(ImVec2{mitte.x, mitte.y + o + 1.0F * m}, ImVec2{mitte.x + 2.5F * m, mitte.y + o - 1.5F * m}, farbe, m);
        }
    }
    const std::string popup = std::string("##erweiterungMenue") + id;
    if (geklickt) { ImGui::OpenPopup(popup.c_str()); }
    if (ImGui::BeginPopup(popup.c_str())) {
        for (const Knopf* k : verdeckt) {
            if (k->trenner) {
                ImGui::Separator();
                continue;
            }
            ed.knoepfeVerdeckt.push_back(k->befehl);
            const bool da = nachgebaut(k->befehl);
            const float g = ImGui::GetTextLineHeight();
            symbolZeichnen(ImGui::GetWindowDrawList(), k->bild, ImGui::GetCursorScreenPos(), g, da);
            ImGui::Indent(g * 1.3F);
            std::string text = k->beschreibung;
            text = text.substr(0, text.find('\n'));
            if (k->zeilen[0] != Str::Count) { text = tr(k->zeilen[0]); }
            const bool haken = k->schalter && angehakt(k->befehl);
            if (ImGui::MenuItem((text + "##em" + k->befehl).c_str(), nullptr, haken, da)) { (void)ausfuehren(k->befehl); }
            if (!da && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) { ImGui::SetTooltip("%s", stufeText(k->befehl).c_str()); }
            ImGui::Unindent(g * 1.3F);
        }
        ImGui::EndPopup();
    }
}

// Eine QToolBar an (x, y) mit der Laenge laenge entlang ihrer Richtung.
void leisteZeichnen(const char* id, const std::vector<Knopf>& liste, bool waagerecht, int x, int y, int laenge) {
    const std::vector<const Knopf*> k = erzeugteKnoepfe(liste);
    const std::vector<karte::LeistenEintrag> ein = leistenEintraege(k);
    const karte::LeistenErgebnis r = karte::leisteAnordnen(ein, laenge);
    const int b = waagerecht ? laenge : mass::kLeisteH;
    const int h = waagerecht ? mass::kLeisteH : laenge;
    merkeFeld(std::string("Leiste:") + id, x, y, b, h);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = bildpunkt(x, y);
    const ImVec2 c = bildpunkt(x + b, y + h);
    dl->AddRectFilled(a, c, stil(ImGuiCol_MenuBarBg));
    const ImU32 linie = stil(ImGuiCol_Border);
    if (waagerecht) {   // QToolBar:horizontal: border-top/bottom-color
        dl->AddRectFilled(a, ImVec2{c.x, bildpunkt(0, y + mass::kLeisteRand).y}, linie);
        dl->AddRectFilled(ImVec2{a.x, bildpunkt(0, y + h - mass::kLeisteRand).y}, c, linie);
    } else {   // QToolBar:right: border-left-color
        dl->AddRectFilled(a, ImVec2{bildpunkt(x + mass::kLeisteRand, 0).x, c.y}, linie);
    }
    const int innen = mass::kLeisteRand + mass::kLeistePolster;
    if (waagerecht) {
        griff(dl, x + innen, y + innen, mass::kLeisteGriff, mass::kKnopf, true);
    } else {
        griff(dl, x + innen, y + innen, mass::kKnopf, mass::kLeisteGriff, false);
    }
    for (std::size_t i = 0; i < k.size(); ++i) {
        if (r.lage[i] < 0 || k[i]->trenner) { continue; }
        knopf(*k[i], waagerecht ? x + r.lage[i] : x + innen, waagerecht ? y + innen : y + r.lage[i]);
    }
    if (r.erweiterung) {
        std::vector<const Knopf*> verdeckt;
        for (std::size_t i = static_cast<std::size_t>(r.ersterVersteckter); i < k.size(); ++i) {
            if (!ein[i].leer) { verdeckt.push_back(k[i]); }
        }
        if (waagerecht) {
            erweiterung(id, verdeckt, x + r.erweiterungLage, y + innen, 12, mass::kKnopf, true);
        } else {
            erweiterung(id, verdeckt, x + innen, y + r.erweiterungLage, mass::kKnopf, 12, false);
        }
    }
    // Rechtsklick auf die Leiste oder einen ihrer Knoepfe ohne eigene
    // Handlung: das Kontextmenue des Hauptfensters (createPopupMenu).
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseHoveringRect(a, c) &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !g_rechtsklickBehandelt) {
        ImGui::OpenPopup("##leistenmenue");
    }
}

// {minimumSize, sizeHint} einer Leiste entlang ihrer Richtung.
std::pair<int, int> leistenMass(const std::vector<Knopf>& liste) {
    const karte::LeistenErgebnis r = karte::leisteAnordnen(leistenEintraege(erzeugteKnoepfe(liste)), karte::kLayoutMax);
    return {r.minimum, r.hinweis};
}

}  // namespace

// Obere Zeile (Qt::TopToolBarArea): nur die Hauptleiste, sie reicht bis zum
// Ende (fitLayout: die letzte Leiste einer Zeile).
void zeichneHauptleiste(int y, int breite) {
    g_rechtsklickBehandelt = false;
    const std::vector<karte::Spanne> zeile = karte::leistenZeile({leistenMass(befehle::werkzeugleiste())}, breite);
    merkeFeld("Werkzeugleiste", zeile[0].pos, y, zeile[0].groesse, mass::kLeisteH);
    leisteZeichnen("haupt", befehle::werkzeugleiste(), true, zeile[0].pos, y, zeile[0].groesse);
    leistenMenue();
}

// Rechte Zeile (Qt::RightToolBarArea): Filterleiste und darunter die
// Plugin-Leiste (mainframe.cpp:1686-1697), verteilt mit fitLayout.
void zeichneRechteLeisten(int x, int y, int hoehe) {
    Editor& ed = e();
    g_rechtsklickBehandelt = false;
    std::vector<const std::vector<Knopf>*> leisten;
    std::vector<const char*> namen;
    if (ed.filterleiste) {
        leisten.push_back(&befehle::filterleiste());
        namen.push_back("filter");
    }
    if (ed.pluginleiste) {
        leisten.push_back(&befehle::pluginleiste());
        namen.push_back("plugin");
    }
    if (leisten.empty()) { return; }
    std::vector<std::pair<int, int>> masse;
    for (const auto* l : leisten) { masse.push_back(leistenMass(*l)); }
    const std::vector<karte::Spanne> zeile = karte::leistenZeile(masse, hoehe);
    for (std::size_t i = 0; i < leisten.size(); ++i) {
        if (i == 0 && ed.filterleiste) {
            merkeFeld("Filterleiste", x, y + zeile[i].pos, mass::kLeisteH, zeile[i].groesse);
            static bool drin = false;   // QEvent::Enter der Filterleiste: m_recentFunc = nullptr
            const bool jetzt = ImGui::IsMouseHoveringRect(bildpunkt(x, y + zeile[i].pos), bildpunkt(x + mass::kLeisteH, y + zeile[i].pos + zeile[i].groesse));
            if (jetzt && !drin) { filterleisteBetreten(); }
            drin = jetzt;
        }
        leisteZeichnen(namen[i], *leisten[i], false, x, y + zeile[i].pos, zeile[i].groesse);
    }
    leistenMenue();
}

// --- Statuszeile (create_main_statusbar, mainframe.cpp:1445-1490) -----------------
//
// QStatusBar::reformat: 3 oben, Schrifthoehe 16, 2 unten; Eintraege mit
// addPermanentWidget: Befehl (Dehnung 1), Position (1), die Zaehler (0),
// Textur (1), Raster (0) - verteilt wie QBoxLayout (statuszeileVerteilen).

void zeichneStatuszeile(int y, int breite) {
    Editor& ed = e();
    const float s = massstab();
    merkeFeld("Statuszeile", 0, y, breite, mass::kStatusH);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // QE_brushCountChange (qe3.cpp:107-119): Gesamtzahlen, bei einer Auswahl
    // die ausgewaehlten; 0 steht leer.
    std::size_t zahl[3] = {0, 0, 0};
    std::size_t gewaehlt[3] = {0, 0, 0};
    for (const auto& ent : ed.karte.entitaeten()) {
        if (ent->klassenname() != "worldspawn") { ++zahl[2]; }
        for (const auto& p : ent->kinder()) { ++zahl[p->art() == karte::Primitiv::Art::Pinsel ? 0 : 1]; }
    }
    for (const auto& w : ed.karte.auswahl.elemente()) {
        const auto p = w.primitiv.lock();
        if (p) {
            ++gewaehlt[p->art() == karte::Primitiv::Art::Pinsel ? 0 : 1];
        } else if (w.entitaet.lock()) {
            ++gewaehlt[2];
        }
    }
    const bool auswahl = ed.karte.auswahl.anzahl() != 0;
    // MainFrame::SetGridStatus (mainframe.cpp:1968-1976)
    char raster[96];
    const char* format = ed.karte.kontext.format == karte::PinselFormat::Quake3BP     ? "BP"
                         : ed.karte.kontext.format == karte::PinselFormat::Quake3Valve220 ? "220"
                                                                                          : "AP";
    std::snprintf(raster, sizeof(raster), "%s%g  F:%s  C:%d  L:%s", ed.raster.einrastGroesse() > 0 ? "G:" : "g:",
                  static_cast<double>(ed.raster.groesse()), format, ed.kameraE.kubikSkala,
                  ed.karte.kontext.texturLock ? "ON   " : "OFF  ");
    // QLabel::sizeHint: fm.boundingRect, auf ganze Punkte aufgerundet.
    const auto textB = [s](const std::string& t) {
        return static_cast<int>(std::ceil(ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.0F, t.c_str()).x / s - 0.001F));
    };
    // Zahlenfelder: setMinimumWidth( fontMetrics().horizontalAdvance( "99999" ) )
    // - gemessen, solange das QLabel noch die Grundschrift von Qt unter
    // Windows hat (MS Shell Dlg 2 = Tahoma 8 pt, 10,67 Punkte; Ziffer 1118/2048
    // Geviert, auf ganze Geraetepunkte gerundet): 5 x 6 = 30 bei Massstab 2.
    const float qt = std::max(1.0F, std::round(s));
    const int zahlB = static_cast<int>(5.0F * std::round(1118.0F / 2048.0F * (8.0F * 96.0F / 72.0F) * qt) / qt);
    const int zaehlerB = 3 * mass::kStatusBild + 3 * zahlB + 5 * mass::kStatusAbstand;
    const int befehlB = textB(ed.statusBefehl);
    const int positionB = textB(ed.statusPosition);
    const int texturB = textB(ed.statusTextur);
    const int rasterB = textB(raster);
    const std::vector<karte::Spanne> sp = karte::statuszeileVerteilen(
        {{befehlB, befehlB, 1}, {positionB, positionB, 1}, {zaehlerB, zaehlerB, 0}, {texturB, texturB, 1}, {rasterB, rasterB, 0}},
        breite);
    const int ty = y + 3;   // vbox->addSpacing(3)
    const ImU32 text = ImGui::GetColorU32(ImGuiCol_Text);
    const auto feld = [&](const char* name, const std::string& t, const karte::Spanne& f) {
        merkeFeld(std::string("Status:") + name, f.pos, ty, f.groesse, mass::kSchrift);
        const ImVec2 p0 = bildpunkt(f.pos, ty);
        const ImVec2 p1 = bildpunkt(f.pos + f.groesse, ty + mass::kSchrift);
        dl->PushClipRect(p0, p1, true);
        dl->AddText(p0, text, t.c_str());
        dl->PopClipRect();
    };
    feld("Befehl", ed.statusBefehl, sp[0]);
    feld("Position", ed.statusPosition, sp[1]);
    merkeFeld("Status:Zaehler", sp[2].pos, ty, sp[2].groesse, mass::kSchrift);
    static const char* const kBilder[3] = {"status_brush", "status_patch", "status_entity"};
    int x = sp[2].pos;
    for (int k = 0; k < 3; ++k) {
        const ImVec2 p = bildpunkt(x, ty);
        symbolZeichnen(dl, kBilder[k], p, std::round(static_cast<float>(mass::kStatusBild) * s), true);
        x += mass::kStatusBild + mass::kStatusAbstand;
        const std::size_t n = auswahl ? gewaehlt[k] : zahl[k];
        const std::string t = n != 0 ? std::to_string(n) : std::string();
        dl->AddText(bildpunkt(x, ty), text, t.c_str());
        x += zahlB + mass::kStatusAbstand;
    }
    feld("Textur", ed.statusTextur, sp[3]);
    feld("Raster", raster, sp[4]);
    const ImVec2 r0 = bildpunkt(sp[4].pos, y);
    const ImVec2 r1 = bildpunkt(sp[4].pos + sp[4].groesse, y + mass::kStatusH);
    if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(r0, r1)) { ImGui::SetTooltip("%s", tr(Str::KeGridStatusTip)); }
}

// --- Einstellungen ------------------------------------------------------------------

void einstellungenLaden() {
    Editor& ed = e();
    const auto& k = g_app->settings.karte;
    const auto zahl = [&](const char* n, float vorgabe) {
        const auto it = k.find(n);
        if (it == k.end()) { return vorgabe; }
        try {
            return std::stof(it->second);
        } catch (...) {
            return vorgabe;
        }
    };
    ed.anordnung = static_cast<befehle::Anordnung>(std::clamp(static_cast<int>(zahl("QE4StyleWindows", 0)), 0, 3));
    ed.gruppenDialogEingebaut = zahl("BuiltInGroupDialog", 0) != 0.0F;
    ed.bearbeitung.waehlEpsilon = std::clamp(zahl("SELECT_EPSILON", 12), 2.0F, 64.0F);
    ed.bearbeitung.einstellung.punktEntitaetenIn2d = zahl("PreferPointEntsIn2D", 1) != 0.0F;
    ed.bearbeitung.pinselIn3d = zahl("3DCreateBrushes", 1) != 0.0F;
    ed.bearbeitung.bewegStil3d = std::clamp(static_cast<int>(zahl("3DMoveStyle", 0)), 0, 1);
    // Clipper_Construct (clippertool.cpp:202-206)
    ed.bearbeitung.clipperCaulk = zahl("ClipperCaulk", 1) != 0.0F;
    ed.bearbeitung.clipperUmdrehenZuruecksetzen = zahl("ClipperResetFlip", 1) != 0.0F;
    ed.bearbeitung.clipperPunkteZuruecksetzen = zahl("ClipperResetPoints", 1) != 0.0F;
    ed.bearbeitung.clipper2Punkte2d = zahl("Clipper2PointsIn2D", 1) != 0.0F;
    ed.bearbeitung.clipperDoppelklickTeilen = std::clamp(static_cast<int>(zahl("ClipperDoubleclickedSplit", 1)), 0, 1);
    // PatchPreferences_construct (patchmanip.cpp:684-686): "Subdivisions"
    karte::setzePatchUnterteilungSchwelle(static_cast<int>(zahl("Subdivisions", karte::kPatchUnterteilungVorgabe)));
    // SurfaceInspector_Construct (surfacedialog.cpp:1765-1770)
    ed.siWerte.skalierung[0] = zahl("SI_SurfaceTexdef_Scale1", 0.5F);
    ed.siWerte.skalierung[1] = zahl("SI_SurfaceTexdef_Scale2", 0.5F);
    ed.siWerte.verschiebung[0] = zahl("SI_SurfaceTexdef_Shift1", 8.0F);
    ed.siWerte.verschiebung[1] = zahl("SI_SurfaceTexdef_Shift2", 8.0F);
    ed.siWerte.drehung = zahl("SI_SurfaceTexdef_Rotate", 45.0F);
    ed.siWerte.rasterAnpassen = zahl("SnapTToGrid", 0) != 0.0F;
    ed.tags.gui = zahl("TagsShowGui", 0) != 0.0F;   // TextureBrowser_Construct (texwindow.cpp:2133)
    // Textures_Construct (textures.cpp:848-852)
    ed.texQual.mipStufe = std::clamp(static_cast<int>(zahl("TextureMipLevel", 0)), 0, 3);
    ed.texQual.gamma = zahl("SI_Gamma", 1.0F);
    ed.texQual.modus = std::clamp(static_cast<int>(zahl("TextureFiltering", 5)), 0, 5);
    ed.texQual.anisotropie = zahl("TextureAnisotropy", 1) != 0.0F;
    ed.texQual.kompression = 0;
    // ModelBrowser_Construct (modelwindow.cpp:1378-1380)
    ed.modBroE.zelle = static_cast<int>(zahl("ModelBrowserCellSize", 80));
    {
        const auto it = k.find("ModelBrowserFolders");
        if (it != k.end()) { ed.modBroE.ordner = it->second; }
        const auto ft = k.find("ColorModBroBackground");
        float r = 0;
        float g = 0;
        float b = 0;
        if (ft != k.end() && std::sscanf(ft->second.c_str(), "%f %f %f", &r, &g, &b) == 3) { ed.modBroE.hintergrund = {r, g, b}; }
    }
    // ConstructFilters: "SI_Exclude" (filters.cpp:206)
    ed.filterMaske = static_cast<unsigned int>(zahl("SI_Exclude", 0));
    // Autosave_Construct (autosave.cpp:184-190), RegionBoxShader (watchbsp.cpp:266)
    ed.autosave.an = zahl("Autosave", 1) != 0.0F;
    ed.autosave.minuten = std::clamp(static_cast<int>(zahl("AutosaveMinutes", 15)), 1, 60);
    ed.autosave.schnappschuesse = zahl("Snapshots", 0) != 0.0F;
    {
        const auto it = k.find("RegionBoxShader");
        ed.regionKastenShader = it != k.end() ? it->second : std::string();
    }
    ed.abtasterGesetzt = false;
    // MainFrame/m_hSplit usw. (SplitterSetting::save: die Groessen), als
    // "a,b" in logischen Punkten; anderes (alte Anteile) gilt als nicht gemerkt.
    const auto groessen = [&](const char* n) {
        std::vector<int> v;
        const auto it = k.find(n);
        if (it == k.end() || it->second.find('.') != std::string::npos) { return v; }
        std::stringstream ss(it->second);
        std::string teil;
        while (std::getline(ss, teil, ',')) {
            try {
                v.push_back(std::stoi(teil));
            } catch (...) {
                return std::vector<int>{};
            }
        }
        return v;
    };
    ed.hGroessen = groessen("hSplit");
    ed.vGroessen = groessen("vSplit");
    ed.v2Groessen = groessen("vSplit2");
    ed.texGroessen = groessen("TextureBrowserSplitter");   // "TextureBrowser/splitter"
    // Entitaeten (Entity_Construct, EntityList_Construct, entity plugin: registerPreference)
    {
        EntitaetenOberflaeche& u = ed.ent;
        u.groessen = groessen("EntityInspectorSplitter");   // "EntityInspector/splitter"
        u.teiler = karte::Teiler{};
        u.listeVonAnfang = zahl("EntListSearchFromStart", 0) != 0.0F;
        u.lichtLetzte = static_cast<int>(zahl("LastLightIntensity", 300));
        u.namenAbstand = std::clamp(static_cast<int>(zahl("SI_ShowNamesDist", 512)), 0, 200500);
        ed.anzeige.namenVerhaeltnis = std::clamp(static_cast<int>(zahl("SI_ShowNamesRatio", 64)), 0, 100500);
        u.zielnamen = zahl("SI_ShowTargetNames", 0) != 0.0F;
        // EntitySelectedColor: Vector3 "r g b"
        if (const auto f = k.find("EntitySelectedColor"); f != k.end()) {
            float c[3] = {1, 1, 1};
            if (std::sscanf(f->second.c_str(), "%f %f %f", &c[0], &c[1], &c[2]) == 3) { u.farbe = karte::Vektor3f(c[0], c[1], c[2]); }
        }
        // QColorDialogStaticData::readSettings: QtProject "Qt/customColors/i"
        u.eigeneFarben.fill(0xFFFFFFU);
        for (std::size_t i = 0; i < u.eigeneFarben.size(); ++i) {
            const auto f = k.find("QtCustomColor" + std::to_string(i));
            if (f != k.end()) {
                try {
                    u.eigeneFarben[i] = static_cast<unsigned>(std::stoul(f->second)) & 0xFFFFFFU;
                } catch (...) {
                }
            }
        }
        u.eigeneFarbenGeladen = true;
    }
    // Texturbrowser (TextureBrowser_Construct: registerPreference, texwindow.cpp:2113-2138)
    karte::TexturBrowserEinstellungen& t = ed.texBro.e;
    const int skala = static_cast<int>(zahl("TextureScale", 50));
    t.skala = karte::texturSkalaIndex(skala) >= 0 ? skala : 50;
    if (const int g = static_cast<int>(zahl("UniformTextureSize", 160)); g >= 16) { t.einheitsGroesse = g; }   // UniformTextureSizeImport
    if (const int g = static_cast<int>(zahl("UniformTextureMinSize", 48)); g >= 16) { t.einheitsMin = g; }
    t.rollbalken = zahl("TextureScrollbar", 1) != 0.0F;
    t.shaderZeigen = zahl("ShowShaders", 1) != 0.0F;
    t.texturenZeigen = zahl("ShowTextures", 1) != 0.0F;
    t.nurShaderliste = zahl("ShowShaderlistOnly", 0) != 0.0F;
    t.festeGroesse = zahl("FixedSize", 1) != 0.0F;
    t.ohneNotex = zahl("FilterNotex", 0) != 0.0F;
    t.transparenz = zahl("EnableAlpha", 0) != 0.0F;
    t.tags = zahl("TagsShowGui", 0) != 0.0F;
    t.vonAnfang = zahl("SearchFromStart", 0) != 0.0F;
    t.startShader = std::clamp(static_cast<int>(zahl("LoadShaders", 0)), 0, 1);
    t.radSchritt = std::clamp(static_cast<int>(zahl("WheelMouseInc", 64)), 0, 8192);
    t.commonOhneTexturen = zahl("HideNonShadersInCommon", 1) != 0.0F;
    ed.texBro.hoeheGeaendert();
    ed.teilerFuer = -1;
    // Grid_construct (grid.cpp:264-275): GridDefault -> g_grid_power; GridMaxCoordPower ->
    // maxGridCoordPowerImport (Region_defaultMinMax)
    ed.rasterStandard = std::clamp(static_cast<int>(zahl("GridDefault", karte::Raster::kStandard)), 0, 13);
    ed.raster.standardSetzen(ed.rasterStandard);
    ed.rasterMaxPotenz = static_cast<int>(zahl("GridMaxCoordPower", 4));
    ed.raster.setzeMaxKoordPotenz(ed.rasterMaxPotenz);
    karte::regionStandard(ed.region, ed.raster.maxKoord());
    // Selection_construct "NudgeAfterClone" (select.cpp:1925); Map_Construct
    // "LastMap", "LoadLastMap" (map.cpp:2447-2448)
    ed.nachKlonenSchubsen = zahl("NudgeAfterClone", 0) != 0.0F;
    ed.letzteKarteLaden = zahl("LoadLastMap", 1) != 0.0F;
    {
        const auto it = k.find("LastMap");
        ed.letzteKarte = it != k.end() ? it->second : std::string();
    }
    // UndoSystemAPI: "UndoLevels" (undo.cpp:422) -> setLevels
    ed.karte.undo.setzeStufen(static_cast<std::size_t>(std::max(0, static_cast<int>(zahl("UndoLevels", 512)))));
    // Brush_Construct (brushmodule.cpp:132-182): BrushType, BrushAlwaysCaulk, BrushSnapPlanes
    // (TexdefDefaultScale liest spielPruefen: nach default_scale aus der .game-Datei)
    ed.pinselTyp = std::clamp(static_cast<int>(zahl("BrushType", 1)), 0, 2);
    // die leere Karte des Starts (Map_New nach Brush_Construct) im Format der Einstellung
    if (ed.karte.entitaeten().empty()) { ed.karte.kontext.format = static_cast<karte::PinselFormat>(ed.pinselTyp); }
    ed.bearbeitung.immerCaulk = zahl("BrushAlwaysCaulk", 0) != 0.0F;
    karte::setzeEbenenPunkteGanzzahlig(zahl("BrushSnapPlanes", 0) != 0.0F);
    // XYWindow_Construct (xywindow.cpp:2280-2285): XYMSAA (Abtastungen), 2DZoomInToPointer,
    // ChaseMouse, NoStipple
    {
        const int xyMsaa = static_cast<int>(zahl("XYMSAA", 8));
        ed.anzeige.msaa = (xyMsaa == 0 || xyMsaa == 2 || xyMsaa == 4 || xyMsaa == 8 || xyMsaa == 16 || xyMsaa == 32) ? xyMsaa : 8;
    }
    ed.anzeige.zumZeiger = zahl("2DZoomInToPointer", 1) != 0.0F;
    ed.anzeige.jagen = zahl("ChaseMouse", 1) != 0.0F;
    ed.anzeige.keineStrichelung = zahl("NoStipple", 1) != 0.0F;
    // BuildMonitor_Construct (watchbsp.cpp:257-268)
    {
        Editor::BauEinstellungen& b = ed.bau;
        b.ueberwachen = zahl("BuildMonitor", 1) != 0.0F;   // ja.game: kein no_bsp_monitor
        b.spielStarten = zahl("BuildRunGame", 0) != 0.0F;
        b.leckStopp = zahl("BuildLeakStop", 1) != 0.0F;
        b.logAusgeben = zahl("BuildDumpLog", 0) != 0.0F;
        const auto text = [&](const char* n, karte::VorgabeText& t) {
            const auto it = k.find(n);
            t.importieren(it != k.end() ? it->second : std::string());
        };
        text("BuildEngineExecutable", b.engine);
        text("BuildEngineExecutableMP", b.engineMp);
        text("BuildEngineArgs", b.argumente);
        text("BuildEngineArgsMP", b.argumenteMp);
    }
    // Kamera (Camera_registerPreferencesPage, CamWnd_Construct: camwindow.cpp:2543-2563)
    karte::KameraEinstellungen& c = ed.kameraE;
    c.kubikSkala = std::clamp(static_cast<int>(zahl("CubicScale", 14)), 1, 23);
    c.tempo = std::clamp(static_cast<int>(zahl("CamMoveSpeed", 500)), 1, karte::kTempoMax);
    c.zeitBisMax = std::clamp(static_cast<int>(zahl("CamMoveTimeToMaxSpeed", 200)), 0, 5000);
    c.radTempo = std::clamp(static_cast<int>(zahl("ScrollMoveSpeed", 100)), 0, 999);
    c.strafeTempo = std::clamp(zahl("CamStrafeSpeed", 1.0F), 0.1F, 10.0F);
    c.winkelTempo = std::clamp(zahl("Sensitivity", 3.0F), 0.1F, 30.0F);
    c.mausUmkehren = zahl("CamInverseMouse", 0) != 0.0F;
    c.zumZeiger = zahl("3DZoomInToPointer", 1) != 0.0F;
    c.diskret = zahl("CamDiscrete", 1) != 0.0F;
    c.kubisch = zahl("CubicClipping", 0) != 0.0F;
    c.faerben = zahl("CameraFaceFill", 1) != 0.0F;
    c.flaechenDraht = zahl("CameraFaceWire", 1) != 0.0F;
    c.modus = karte::modusAusZahl(static_cast<int>(zahl("CameraRenderMode", 2)));
    if (c.modus == karte::KameraModus::Licht) { c.modus = karte::KameraModus::Textur; }   // nur doom3
    // CameraMSAA: die Zahl der Abtastungen (CamMSAAImport: 1 << Index)
    const int msaa = static_cast<int>(zahl("CameraMSAA", 8));
    c.msaa = (msaa == 0 || msaa == 2 || msaa == 4 || msaa == 8 || msaa == 16 || msaa == 32) ? msaa : 8;
    c.strafeModus = std::clamp(static_cast<int>(zahl("StrafeMode", 3)), 0, 4);
    c.sichtfeld = std::clamp(zahl("fieldOfView", 100.0F), 1.0F, 175.0F);
    c.zeigeStatistik = zahl("ShowStats", 0) != 0.0F;
    c.zeigeWorkzone = zahl("ShowWorkzone3d", 1) != 0.0F;
    c.zeigeGroesse = zahl("ShowSize3d", 1) != 0.0F;
    // ToolbarHiddenButtons: "%Name%%Name%..." (gtkmisc.cpp:147)
    ed.versteckteKnoepfe.clear();
    const auto it = k.find("ToolbarHiddenButtons");
    if (it == k.end()) {
        for (const std::string& n : befehle::standardVersteckt()) { ed.versteckteKnoepfe.insert(n); }
    } else {
        std::size_t p = 0;
        while ((p = it->second.find('%', p)) != std::string::npos) {
            const std::size_t q = it->second.find('%', p + 1);
            if (q == std::string::npos) { break; }
            if (q > p + 1) { ed.versteckteKnoepfe.insert(it->second.substr(p + 1, q - p - 1)); }
            p = q + 1;
        }
    }
    // Tastenbelegung (LoadCommandMap)
    std::vector<std::pair<std::string, std::string>> eigene(g_app->settings.karteTasten.begin(),
                                                             g_app->settings.karteTasten.end());
    for (const std::string& m : ed.belegung.laden(eigene)) { warnung(m); }
    ed.belegungGeladen = true;
}

void einstellungenSpeichern() {
    Editor& ed = e();
    auto& k = g_app->settings.karte;
    k["QE4StyleWindows"] = std::to_string(static_cast<int>(ed.anordnung));
    k["BuiltInGroupDialog"] = ed.gruppenDialogEingebaut ? "1" : "0";
    k["ClipperCaulk"] = ed.bearbeitung.clipperCaulk ? "1" : "0";
    k["ClipperResetFlip"] = ed.bearbeitung.clipperUmdrehenZuruecksetzen ? "1" : "0";
    k["ClipperResetPoints"] = ed.bearbeitung.clipperPunkteZuruecksetzen ? "1" : "0";
    k["Clipper2PointsIn2D"] = ed.bearbeitung.clipper2Punkte2d ? "1" : "0";
    k["ClipperDoubleclickedSplit"] = std::to_string(ed.bearbeitung.clipperDoppelklickTeilen);
    k["Subdivisions"] = std::to_string(karte::patchUnterteilungSchwelle());
    k["SI_SurfaceTexdef_Scale1"] = karte::zahlText(ed.siWerte.skalierung[0]);
    k["SI_SurfaceTexdef_Scale2"] = karte::zahlText(ed.siWerte.skalierung[1]);
    k["SI_SurfaceTexdef_Shift1"] = karte::zahlText(ed.siWerte.verschiebung[0]);
    k["SI_SurfaceTexdef_Shift2"] = karte::zahlText(ed.siWerte.verschiebung[1]);
    k["SI_SurfaceTexdef_Rotate"] = karte::zahlText(ed.siWerte.drehung);
    k["SnapTToGrid"] = ed.siWerte.rasterAnpassen ? "1" : "0";
    k["TagsShowGui"] = ed.tags.gui ? "1" : "0";
    k["SI_Exclude"] = std::to_string(ed.filterMaske);
    k["ModelBrowserCellSize"] = std::to_string(ed.modBroE.zelle);
    k["ModelBrowserFolders"] = ed.modBroE.ordner;
    {
        char z[96];
        std::snprintf(z, sizeof(z), "%g %g %g", static_cast<double>(ed.modBroE.hintergrund[0]), static_cast<double>(ed.modBroE.hintergrund[1]),
                      static_cast<double>(ed.modBroE.hintergrund[2]));
        k["ColorModBroBackground"] = z;
    }
    k["Autosave"] = ed.autosave.an ? "1" : "0";
    k["AutosaveMinutes"] = std::to_string(ed.autosave.minuten);
    k["Snapshots"] = ed.autosave.schnappschuesse ? "1" : "0";
    k["RegionBoxShader"] = ed.regionKastenShader;
    k["GridDefault"] = std::to_string(ed.rasterStandard);
    k["GridMaxCoordPower"] = std::to_string(ed.rasterMaxPotenz);
    k["NudgeAfterClone"] = ed.nachKlonenSchubsen ? "1" : "0";
    k["LoadLastMap"] = ed.letzteKarteLaden ? "1" : "0";
    k["LastMap"] = ed.letzteKarte;
    k["UndoLevels"] = std::to_string(ed.karte.undo.stufen());
    k["BrushType"] = std::to_string(ed.pinselTyp);
    k["BrushAlwaysCaulk"] = ed.bearbeitung.immerCaulk ? "1" : "0";
    k["BrushSnapPlanes"] = karte::ebenenPunkteGanzzahlig() ? "1" : "0";
    k["TexdefDefaultScale"] = karte::zahlText(karte::standardTexturSkalierung());
    k["XYMSAA"] = std::to_string(ed.anzeige.msaa);
    k["2DZoomInToPointer"] = ed.anzeige.zumZeiger ? "1" : "0";
    k["ChaseMouse"] = ed.anzeige.jagen ? "1" : "0";
    k["NoStipple"] = ed.anzeige.keineStrichelung ? "1" : "0";
    k["BuildMonitor"] = ed.bau.ueberwachen ? "1" : "0";
    k["BuildRunGame"] = ed.bau.spielStarten ? "1" : "0";
    k["BuildLeakStop"] = ed.bau.leckStopp ? "1" : "0";
    k["BuildDumpLog"] = ed.bau.logAusgeben ? "1" : "0";
    k["BuildEngineExecutable"] = ed.bau.engine.text;   // DefaultableString::Export: ohne Vorgabe
    k["BuildEngineExecutableMP"] = ed.bau.engineMp.text;
    k["BuildEngineArgs"] = ed.bau.argumente.text;
    k["BuildEngineArgsMP"] = ed.bau.argumenteMp.text;
    k["TextureMipLevel"] = std::to_string(ed.texQual.mipStufe);
    k["SI_Gamma"] = karte::zahlText(ed.texQual.gamma);
    k["TextureFiltering"] = std::to_string(ed.texQual.modus);
    k["TextureAnisotropy"] = ed.texQual.anisotropie ? "1" : "0";
    k["TextureCompressionFormat"] = std::to_string(ed.texQual.kompression);
    const auto liste = [](const std::vector<int>& v) {
        std::string s;
        for (const int i : v) { s += (s.empty() ? "" : ",") + std::to_string(i); }
        return s;
    };
    if (!ed.hGroessen.empty()) { k["hSplit"] = liste(ed.hGroessen); }
    if (!ed.vGroessen.empty()) { k["vSplit"] = liste(ed.vGroessen); }
    if (!ed.v2Groessen.empty()) { k["vSplit2"] = liste(ed.v2Groessen); }
    if (!ed.texGroessen.empty()) { k["TextureBrowserSplitter"] = liste(ed.texGroessen); }
    {
        const EntitaetenOberflaeche& u = ed.ent;
        if (!u.groessen.empty()) { k["EntityInspectorSplitter"] = liste(u.groessen); }
        k["EntListSearchFromStart"] = u.listeVonAnfang ? "1" : "0";
        k["LastLightIntensity"] = std::to_string(u.lichtLetzte);
        k["SI_ShowNamesDist"] = std::to_string(u.namenAbstand);
        k["SI_ShowNamesRatio"] = std::to_string(ed.anzeige.namenVerhaeltnis);
        k["SI_ShowTargetNames"] = u.zielnamen ? "1" : "0";
        char farbe[96];
        std::snprintf(farbe, sizeof(farbe), "%g %g %g", static_cast<double>(u.farbe[0]), static_cast<double>(u.farbe[1]),
                      static_cast<double>(u.farbe[2]));
        k["EntitySelectedColor"] = farbe;
        for (std::size_t i = 0; i < u.eigeneFarben.size(); ++i) {
            if (u.eigeneFarben[i] != 0xFFFFFFU) {
                k["QtCustomColor" + std::to_string(i)] = std::to_string(u.eigeneFarben[i]);
            } else {
                k.erase("QtCustomColor" + std::to_string(i));
            }
        }
    }
    const karte::TexturBrowserEinstellungen& t = ed.texBro.e;
    k["TextureScale"] = std::to_string(t.skala);
    k["UniformTextureSize"] = std::to_string(t.einheitsGroesse);
    k["UniformTextureMinSize"] = std::to_string(t.einheitsMin);
    k["TextureScrollbar"] = t.rollbalken ? "1" : "0";
    k["ShowShaders"] = t.shaderZeigen ? "1" : "0";
    k["ShowTextures"] = t.texturenZeigen ? "1" : "0";
    k["ShowShaderlistOnly"] = t.nurShaderliste ? "1" : "0";
    k["FixedSize"] = t.festeGroesse ? "1" : "0";
    k["FilterNotex"] = t.ohneNotex ? "1" : "0";
    k["EnableAlpha"] = t.transparenz ? "1" : "0";
    k["TagsShowGui"] = t.tags ? "1" : "0";
    k["SearchFromStart"] = t.vonAnfang ? "1" : "0";
    k["LoadShaders"] = std::to_string(t.startShader);
    k["WheelMouseInc"] = std::to_string(t.radSchritt);
    k["HideNonShadersInCommon"] = t.commonOhneTexturen ? "1" : "0";
    const karte::KameraEinstellungen& c = ed.kameraE;
    const auto zahlText = [](float f) {
        char z[32];
        std::snprintf(z, sizeof(z), "%g", static_cast<double>(f));
        return std::string(z);
    };
    // SelectionSystem_Construct: registerPreference
    k["SELECT_EPSILON"] = std::to_string(static_cast<int>(ed.bearbeitung.waehlEpsilon));
    k["PreferPointEntsIn2D"] = ed.bearbeitung.einstellung.punktEntitaetenIn2d ? "1" : "0";
    k["3DCreateBrushes"] = ed.bearbeitung.pinselIn3d ? "1" : "0";
    k["3DMoveStyle"] = std::to_string(ed.bearbeitung.bewegStil3d);
    k["CubicScale"] = std::to_string(c.kubikSkala);
    k["CamMoveSpeed"] = std::to_string(c.tempo);
    k["CamMoveTimeToMaxSpeed"] = std::to_string(c.zeitBisMax);
    k["ScrollMoveSpeed"] = std::to_string(c.radTempo);
    k["CamStrafeSpeed"] = zahlText(c.strafeTempo);
    k["Sensitivity"] = zahlText(c.winkelTempo);
    k["CamInverseMouse"] = c.mausUmkehren ? "1" : "0";
    k["3DZoomInToPointer"] = c.zumZeiger ? "1" : "0";
    k["CamDiscrete"] = c.diskret ? "1" : "0";
    k["CubicClipping"] = c.kubisch ? "1" : "0";
    k["CameraFaceFill"] = c.faerben ? "1" : "0";
    k["CameraFaceWire"] = c.flaechenDraht ? "1" : "0";
    k["CameraRenderMode"] = std::to_string(static_cast<int>(c.modus));
    k["CameraMSAA"] = std::to_string(c.msaa);
    k["StrafeMode"] = std::to_string(c.strafeModus);
    k["fieldOfView"] = zahlText(c.sichtfeld);
    k["ShowStats"] = c.zeigeStatistik ? "1" : "0";
    k["ShowWorkzone3d"] = c.zeigeWorkzone ? "1" : "0";
    k["ShowSize3d"] = c.zeigeGroesse ? "1" : "0";
    std::string versteckt;
    for (const std::string& n : ed.versteckteKnoepfe) { versteckt += "%" + n + "%"; }
    k["ToolbarHiddenButtons"] = versteckt;
    g_app->settings.karteTasten.clear();
    for (const auto& [name, taste] : ed.belegung.abweichende()) { g_app->settings.karteTasten[name] = taste; }
}

void menueEintraege(const std::vector<MenueEintrag>& liste, std::size_t erster) {
    std::size_t i = erster;
    eintraegeZeichnen(liste, i);
}

// QMenuBar: "&File" usw. sind Alt-Kuerzel (QKeySequence::mnemonic) - nur mit
// NRCs Wortlaut (uebersetzte Titel haben keine).
bool leistenMnemonik(unsigned zeichen) {
    const auto& liste = befehle::menues();
    for (std::size_t i = 0; i < liste.size(); ++i) {
        if (liste[i].art != MenueArt::Menue) { continue; }
        const std::string text = trK(liste[i].str);
        const int l = qtMnemonik(liste[i].text, text);
        if (l >= 0 && static_cast<unsigned>(std::toupper(static_cast<unsigned char>(text[static_cast<std::size_t>(l)]))) == zeichen) {
            e().menueOeffnen = static_cast<int>(i);
            return true;
        }
    }
    return false;
}

void leistenSymbol(ImDrawList* dl, const char* name, ImVec2 a, float g, bool aktiv) { symbolZeichnen(dl, name, a, g, aktiv); }

}  // namespace bhed::gui::karteneditor
