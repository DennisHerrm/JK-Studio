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
#include "karte_icons.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
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
struct Symbole {
    void* textur = nullptr;
    unsigned stand = ~0U;
    int breite = 0;
};
// Wie NRC (set_icon_theme): die hellen Bitmaps zu einer hellen Farbgebung.
Symbole& symbole() {
    static Symbole dunkel;
    static Symbole hell;
    const bool h = hellesThema();
    Symbole& s = h ? hell : dunkel;
    if (s.textur == nullptr || s.stand != render::texturStand()) {
        const image::Image bild = h ? image::decodePng(kKarteIconPngHell, kKarteIconPngHellGroesse)
                                    : image::decodePng(kKarteIconPng, kKarteIconPngGroesse);
        if (bild.ok) {
            s.textur = render::createTexture(bild.rgba.data(), bild.width, bild.height);
            s.breite = bild.width;
            s.stand = render::texturStand();
        }
    }
    return s;
}

int symbolSpalte(const char* name) {
    for (std::size_t i = 0; i < kKarteIconAnzahl; ++i) {
        if (std::strcmp(kKarteIcons[i].name, name) == 0) { return kKarteIcons[i].spalte; }
    }
    return -1;
}

// Das Symbol an eine Stelle zeichnen (fuer Knoepfe).
void symbolZeichnen(ImDrawList* dl, const char* name, ImVec2 a, float g, bool aktiv) {
    Symbole& s = symbole();
    const int spalte = symbolSpalte(name);
    if (s.textur == nullptr || spalte < 0 || s.breite <= 0) { return; }
    const float u0 = static_cast<float>(spalte * kKarteIconGroesse) / static_cast<float>(s.breite);
    const float u1 = static_cast<float>((spalte + 1) * kKarteIconGroesse) / static_cast<float>(s.breite);
    // QIcon::Disabled: Qt zeichnet das Symbol blass.
    dl->AddImage(reinterpret_cast<ImTextureID>(s.textur), a, ImVec2{a.x + g, a.y + g}, ImVec2{u0, 0.0F},
                 ImVec2{u1, 1.0F}, aktiv ? IM_COL32_WHITE : IM_COL32(255, 255, 255, 90));
}

float symbolGroesse() {
    // PM_ToolBarIconSize des Fusion-Stils: 24 Punkte bei 96 dpi, zur
    // Standardschrift von 9 pt (12 Punkte) - also doppelt so gross wie die
    // Schrift, wie auf dem Foto von NRC.
    return std::round(ImGui::GetFontSize() * 2.0F);
}

std::string stufeText(const std::string& befehl) {
    const befehle::Befehl* b = befehle::finden(befehl);
    char z[160];
    std::snprintf(z, sizeof(z), tr(Str::KeStufe), b != nullptr ? b->stufe : 2);
    return z;
}

// --- Menues -------------------------------------------------------------------

// Ein Menuepunkt (create_menu_item_with_mnemonic): Beschriftung, Taste,
// Haken, ausgegraut mit Tooltip, wenn er erst in einer spaeteren Stufe kommt.
bool menuePunkt(const char* text, const char* taste, bool haken, bool aktiv, const std::string& tooltip) {
    const bool gewaehlt = ImGui::MenuItem(text, taste != nullptr && taste[0] != 0 ? taste : nullptr, haken, aktiv);
    if (!tooltip.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", tooltip.c_str());
    }
    return gewaehlt;
}

bool menueAnfang(const char* text, bool aktiv = true) { return ImGui::BeginMenu(text, aktiv); }

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
                ImGui::Separator();
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
            const bool haken = m.art == MenueArt::Haken && angehakt(befehl);
            if (menuePunkt(id.c_str(), taste.c_str(), haken, da, da ? std::string() : stufeText(befehl))) {
                (void)ausfuehren(befehl);
            }
            letzterTrenner = false;
            break;
        }
        case MenueArt::Ruf: {
            // Eigene Rueckrufe (Set 2D Background image, Bug report, About).
            const std::string id = std::string(trK(m.str)) + "###ruf" + std::to_string(i);
            const bool da = nachgebaut(m.befehl);
            if (menuePunkt(id.c_str(), nullptr, false, da, da ? std::string() : stufeText(m.befehl))) {
                (void)ausfuehren(m.befehl);
            }
            letzterTrenner = false;
            break;
        }
        case MenueArt::Unter: {
            const std::string id = std::string(trK(m.str)) + "###unter" + std::to_string(i);
            const std::size_t start = i;
            if (menueAnfang(id.c_str())) {
                ++i;
                eintraegeZeichnen(liste, i);
                ImGui::EndMenu();
            } else {
                int tiefe = 1;
                while (tiefe > 0 && ++i < liste.size()) {
                    if (liste[i].art == MenueArt::Unter) { ++tiefe; }
                    if (liste[i].art == MenueArt::Ende) { --tiefe; }
                }
            }
            (void)start;
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
            const std::string roh = karte::ohneMnemonik(z.beschriftung(i));
            const std::string text = (roh == "Recent Files" ? std::string(trK(Str::KeRecentFiles)) : roh) + "###karteMru" +
                                     std::to_string(i);
            if (menuePunkt(text.c_str(), nullptr, false, i < z.anzahl(), std::string())) { zuletztOeffnen(i); }
        }
    } else if (name == "Viewportthemen") {
        // themes/_colors/*.json (colors.cpp:218-226) - die Farben folgen hier
        // dem Farbschema des Programms (Entscheidung shank), die Eintraege
        // stehen da, ausgegraut.
        for (const char* t : kViewportThemen) { (void)menuePunkt(t, nullptr, false, false, tr(Str::KeFarbenProgramm)); }
    } else if (name == "GuiThemen") {
        // theme_construct_menu (theme.cpp:182-200)
        for (const char* t : {"Default", "Fusion", "Dark", "Darker", "GreyBlue2", "BlenderDark", "BlenderLight"}) {
            (void)menuePunkt(t, nullptr, false, false, tr(Str::KeFarbenProgramm));
        }
    } else if (name == "Farben") {
        // g_ColoursMenu (colors.cpp:153-171): je Farbe ein Eintrag mit Farbfeld.
        for (const FarbEintrag& f : farbEintraege()) {
            const Farben::F3& c = ed.farben.*(f.feld);
            ImGui::ColorButton((std::string("##fk") + f.name).c_str(), ImVec4{c[0], c[1], c[2], 1.0F},
                               ImGuiColorEditFlags_NoTooltip, ImVec2{ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()});
            ImGui::SameLine();
            (void)menuePunkt(f.text, nullptr, false, false, tr(Str::KeFarbenProgramm));
        }
    } else if (name == "Transforms") {
        // SelectionSystem_connectTransformsCallbacks: "Reset Translation" usw.
        // erst mit den Manipulatoren (Stufe 1b, Punkt 7).
        for (const char* t : {"Reset Translation", "Reset Rotation", "Reset Scale", "Reset Skew"}) {
            (void)menuePunkt(t, nullptr, false, false, stufeText("ResetTransforms"));
        }
    } else if (name == "Build") {
        // Build_constructMenu: die Eintraege aus der Build-Konfiguration -
        // mit dem Kompilieren (Stufe 3).
        (void)menuePunkt("(build menu)", nullptr, false, false, stufeText("BuildMenuCustomize"));
    } else if (name == "Plugins") {
        // create_plugins_menu (pluginmenu.cpp:131): je Plugin ein Untermenue.
        for (const char* p : {"bobToolz", "brushexport", "meshtex", "Portal Viewer", "shaderplug", "sunplug"}) {
            if (ImGui::BeginMenu(p, false)) { ImGui::EndMenu(); }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", stufeText("bobToolz::About").c_str());
            }
        }
    } else if (name == "Hilfe") {
        // create_game_help_menu (help.cpp:112): global.xlink "General" und
        // game.xlink des Spiels - Verweise auf NRCs docs/ und das JA-SDK.
        static const char* const kGeneral[] = {
            "Mouse Shortcuts", "Complete list of Q3map2 command line parameters", "Complete list of entity keys",
            "Shader Manual", "q3map2 handbook", "Additional map compiler features", "Additional map editor features",
            "Editor features presentation", "Q3Map2 - FS20 - R5 readme", "Blendmodes cheatsheet", "MBSPC documentation",
            "iD Tech 3 mapping trello"};
        if (ImGui::BeginMenu("General")) {
            for (const char* t : kGeneral) { (void)menuePunkt(t, nullptr, false, false, tr(Str::KeNichtDabei)); }
            ImGui::EndMenu();
        }
        static const char* const kJa[] = {"Tutorial: Mapping in NRC for JKA/MBII", "About Jedi Academy editing package",
                                          "Jedi Academy Tools disclaimer", "Jedi Academy SDK readme",
                                          "Bot waypoint tutorial", "ICARUS Manual", "Using EffectsEd", "How to play Siege",
                                          "Jedi Academy forum @ Raven Software",
                                          "Jedi Academy Editing forum @ MapCenter"};
        if (ImGui::BeginMenu("Jedi Academy")) {
            for (const char* t : kJa) { (void)menuePunkt(t, nullptr, false, false, tr(Str::KeNichtDabei)); }
            ImGui::EndMenu();
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
    if (symbolSpalte(name) < 0) { return false; }
    const ImVec2 p = ImGui::GetCursorScreenPos();
    symbolZeichnen(ImGui::GetWindowDrawList(), name, p, groesse, hell);
    ImGui::Dummy(ImVec2{groesse, groesse});
    return true;
}

// --- Menueleiste (create_main_menu, mainframe.cpp:1225) ---------------------

void zeichneMenueleiste() {
    if (!ImGui::BeginMenuBar()) { return; }
    const auto& liste = befehle::menues();
    for (std::size_t i = 0; i < liste.size(); ++i) {
        if (liste[i].art != MenueArt::Menue) { continue; }
        const std::string id = std::string(trK(liste[i].str)) + "###menue" + std::to_string(i);
        if (menueAnfang(id.c_str())) {
            ++i;
            eintraegeZeichnen(liste, i);
            ImGui::EndMenu();
        }
    }
    ImGui::EndMenuBar();
}

// --- Werkzeugleisten -------------------------------------------------------------

namespace {

bool g_rechtsklickBehandelt = false;

// Ein Knopf einer QToolBar (dark.qss "QToolBar QToolButton").
void knopf(const Knopf& k, float s, bool waagerecht, std::size_t nr) {
    Editor& ed = e();
    const float rand = 2.0F;   // padding 1px + border 1px
    const ImVec2 groesse{s + rand * 2.0F, s + rand * 2.0F};
    const std::string befehl = k.befehl;
    const bool da = befehl.empty() ? false : nachgebaut(befehl);
    ed.knoepfeImBild.push_back(befehl);
    const std::string id = std::string("##knopf") + (befehl.empty() ? k.bild : befehl);
    (void)nr;
    ImGui::InvisibleButton(id.c_str(), groesse, ImGuiButtonFlags_MouseButtonLeft);
    const bool hover = ImGui::IsItemHovered();
    const bool an = k.schalter && angehakt(befehl);
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // QToolButton: gedrueckt (an) mit Rahmen in der Hervorhebung, unter der
    // Maus hervorgehoben - in den Farben des Programms.
    ImU32 hinten = 0;
    ImU32 kante = 0;
    if (an) {
        hinten = stil(hover && da ? ImGuiCol_ButtonHovered : ImGuiCol_FrameBg);
        kante = stil(ImGuiCol_CheckMark, hover && da ? 0.95F : 0.75F);
    } else if (hover && da) {
        hinten = stil(ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
        kante = stil(ImGuiCol_Border);
    }
    if (hinten != 0U) { dl->AddRectFilled(a, b, hinten); }
    if (kante != 0U) { dl->AddRect(a, b, kante); }
    symbolZeichnen(dl, k.bild, ImVec2{a.x + rand, a.y + rand}, s, da);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && da) { (void)ausfuehren(befehl); }
    // FilterToolbarHandler (filterbar.cpp:104-118): Rechtsklick auf einen
    // Filterknopf mit eigener Handlung fuehrt sie aus.
    // Kontextmenues kommen unter Windows beim Loslassen (WM_CONTEXTMENU).
    if (hover && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && k.rechtsklick[0] != 0) {
        const std::string r = k.rechtsklick;
        g_rechtsklickBehandelt = true;   // eventFilter: event->accept(); return true
        if (r.rfind("tex:", 0) != 0 && r != "func_group") { (void)ausfuehren(r); }
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
    if (waagerecht) { ImGui::SameLine(0.0F, 0.0F); }
}

// toolbar_construct_control_menu (gtkmisc.cpp:230-258) mit den Leisten davor
// (QMainWindow::createPopupMenu).
void leistenMenue() {
    Editor& ed = e();
    if (!ImGui::BeginPopup("##leistenmenue")) { return; }
    ImGui::MenuItem(tr(Str::KeMainToolbar), nullptr, &ed.hauptleiste);
    ImGui::MenuItem(tr(Str::KeFilterToolbar), nullptr, &ed.filterleiste);
    ImGui::MenuItem(tr(Str::KePluginToolbar), nullptr, &ed.pluginleiste);
    ImGui::Separator();
    for (const auto* liste : {&befehle::werkzeugleiste(), &befehle::filterleiste()}) {
        for (const Knopf& k : *liste) {
            if (k.trenner || k.befehl[0] == 0) { continue; }
            const bool sichtbar = ed.versteckteKnoepfe.count(k.befehl) == 0U;
            symbolZeichnen(ImGui::GetWindowDrawList(), k.bild, ImGui::GetCursorScreenPos(), ImGui::GetTextLineHeight(), true);
            ImGui::Indent(ImGui::GetTextLineHeight() * 1.3F);
            // "prevent closing the menu on clicks"
            if (ImGui::MenuItem((std::string(k.befehl) + "##lm").c_str(), nullptr, sichtbar, true)) {
                if (sichtbar) { ed.versteckteKnoepfe.insert(k.befehl); } else { ed.versteckteKnoepfe.erase(k.befehl); }
                einstellungenSpeichern();
            }
            ImGui::Unindent(ImGui::GetTextLineHeight() * 1.3F);
        }
        ImGui::Separator();
    }
    ImGui::EndPopup();
}

// Die Knoepfe einer Leiste, mit ToolbarItems::update_separators_visibility:
// ein Trenner nur zwischen sichtbaren Knoepfen.
std::vector<const Knopf*> sichtbareKnoepfe(const std::vector<Knopf>& liste) {
    Editor& ed = e();
    std::vector<const Knopf*> v;
    for (const Knopf& k : liste) {
        if (!befehle::bedingungGilt(k.bedingung, ed.anordnung, ed.gruppenDialogEingebaut)) { continue; }
        if (!k.trenner && ed.versteckteKnoepfe.count(k.befehl) != 0U) { continue; }
        if (k.trenner && (v.empty() || v.back()->trenner)) { continue; }
        v.push_back(&k);
    }
    while (!v.empty() && v.back()->trenner) { v.pop_back(); }
    return v;
}

void griff(ImDrawList* dl, ImVec2 a, float h, bool waagerecht) {
    // Der Griff einer beweglichen QToolBar: zwei Reihen Punkte.
    const ImU32 c = stil(ImGuiCol_TextDisabled);
    for (float k = 4.0F; k < h - 3.0F; k += 3.0F) {
        if (waagerecht) {
            dl->AddRectFilled(ImVec2{a.x + 3.0F, a.y + k}, ImVec2{a.x + 4.0F, a.y + k + 1.0F}, c);
            dl->AddRectFilled(ImVec2{a.x + 6.0F, a.y + k}, ImVec2{a.x + 7.0F, a.y + k + 1.0F}, c);
        } else {
            dl->AddRectFilled(ImVec2{a.x + k, a.y + 3.0F}, ImVec2{a.x + k + 1.0F, a.y + 4.0F}, c);
            dl->AddRectFilled(ImVec2{a.x + k, a.y + 6.0F}, ImVec2{a.x + k + 1.0F, a.y + 7.0F}, c);
        }
    }
}

}  // namespace

void zeichneHauptleiste() {
    Editor& ed = e();
    if (!ed.hauptleiste) { return; }
    const float s = symbolGroesse();
    const float pad = 3.0F;   // QToolBar padding 3px
    const float hoehe = s + 4.0F + pad * 2.0F;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const float breite = ImGui::GetContentRegionAvail().x;
    dl->AddRectFilled(a, ImVec2{a.x + breite, a.y + hoehe}, stil(ImGuiCol_MenuBarBg));
    dl->AddLine(a, ImVec2{a.x + breite, a.y}, stil(ImGuiCol_Border));
    dl->AddLine(ImVec2{a.x, a.y + hoehe - 1.0F}, ImVec2{a.x + breite, a.y + hoehe - 1.0F}, stil(ImGuiCol_Border));
    griff(dl, a, hoehe, true);
    ImGui::SetCursorScreenPos(ImVec2{a.x + 10.0F, a.y + pad});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    std::size_t nr = 0;
    for (const Knopf* k : sichtbareKnoepfe(befehle::werkzeugleiste())) {
        if (k->trenner) {
            ImGui::Dummy(ImVec2{8.0F, s});   // QToolBar::separator 8px
            ImGui::SameLine(0.0F, 0.0F);
            continue;
        }
        knopf(*k, s, true, nr++);
    }
    ImGui::PopStyleVar();
    // Rechtsklick auf die Leiste oder einen ihrer Knoepfe: das Kontextmenue
    // des Hauptfensters (QMainWindow::createPopupMenu).
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
        ImGui::IsMouseHoveringRect(a, ImVec2{a.x + breite, a.y + hoehe}) && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("##leistenmenue");
    }
    leistenMenue();
    ImGui::SetCursorScreenPos(ImVec2{a.x, a.y + hoehe});
    ImGui::Dummy(ImVec2{0.0F, 0.0F});
}

float zeichneFilterleiste(float hoehe) {
    Editor& ed = e();
    if (!ed.filterleiste) { return 0.0F; }
    const float s = symbolGroesse();
    const float pad = 3.0F;
    const float breite = s + 4.0F + pad * 2.0F;
    ImGui::BeginChild("##filterleiste", ImVec2{breite, hoehe}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    dl->AddRectFilled(a, ImVec2{a.x + breite, a.y + hoehe}, stil(ImGuiCol_MenuBarBg));
    dl->AddLine(a, ImVec2{a.x, a.y + hoehe}, stil(ImGuiCol_Border));   // QToolBar:right - Rand links
    griff(dl, a, breite, false);
    ImGui::SetCursorScreenPos(ImVec2{a.x + pad, a.y + 10.0F});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    std::size_t nr = 100;
    for (const Knopf* k : sichtbareKnoepfe(befehle::filterleiste())) {
        ImGui::SetCursorScreenPos(ImVec2{a.x + pad, ImGui::GetCursorScreenPos().y});
        if (k->trenner) {
            ImGui::Dummy(ImVec2{s, 8.0F});
            continue;
        }
        knopf(*k, s, false, nr++);
    }
    ImGui::PopStyleVar();
    if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !g_rechtsklickBehandelt) {
        // Rechtsklick ohne eigene Handlung des Knopfs: das Leistenmenue.
        ImGui::OpenPopup("##leistenmenue");
    }
    g_rechtsklickBehandelt = false;
    leistenMenue();
    ImGui::EndChild();
    return breite;
}

// --- Statuszeile (create_main_statusbar, mainframe.cpp:1447-1490) -----------------

float statuszeilenHoehe() { return ImGui::GetTextLineHeight() + 8.0F; }

void zeichneStatuszeile() {
    Editor& ed = e();
    const float h = statuszeilenHoehe();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const float breite = ImGui::GetContentRegionAvail().x;
    const ImU32 linie = stil(ImGuiCol_Border);
    dl->AddLine(a, ImVec2{a.x + breite, a.y}, linie);
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
                  static_cast<double>(ed.raster.groesse()), format, ed.kubikSkala,
                  ed.karte.kontext.texturLock ? "ON   " : "OFF  ");
    const float s = std::round(16.0F * g_app->dpiScale * g_app->uiScale);
    const float zahlB = ImGui::CalcTextSize("99999").x;
    const float rasterB = ImGui::CalcTextSize(raster).x + 12.0F;
    const float festB = (s + zahlB + 8.0F) * 3.0F + rasterB;
    const float frei = std::max(0.0F, breite - festB);
    float x = a.x + 4.0F;
    const float ty = a.y + (h - ImGui::GetTextLineHeight()) * 0.5F;
    const ImU32 text = ImGui::GetColorU32(ImGuiCol_Text);
    const auto feld = [&](const std::string& t, float w) {
        dl->PushClipRect(ImVec2{x, a.y}, ImVec2{x + w - 6.0F, a.y + h}, true);
        dl->AddText(ImVec2{x, ty}, text, t.c_str());
        dl->PopClipRect();
        x += w;
        dl->AddLine(ImVec2{x - 3.0F, a.y + 2.0F}, ImVec2{x - 3.0F, a.y + h - 2.0F}, linie);
    };
    feld(ed.statusBefehl, frei / 3.0F);
    feld(ed.statusPosition, frei / 3.0F);
    static const char* const kBilder[3] = {"status_brush", "status_patch", "status_entity"};
    for (int k = 0; k < 3; ++k) {
        symbolZeichnen(dl, kBilder[k], ImVec2{x, a.y + (h - s) * 0.5F}, s, true);
        x += s + 4.0F;
        const std::size_t n = auswahl ? gewaehlt[k] : zahl[k];
        const std::string t = n != 0 ? std::to_string(n) : std::string();
        dl->AddText(ImVec2{x, ty}, text, t.c_str());
        x += zahlB + 4.0F;
    }
    dl->AddLine(ImVec2{x - 2.0F, a.y + 2.0F}, ImVec2{x - 2.0F, a.y + h - 2.0F}, linie);
    feld(ed.statusTextur, frei / 3.0F);
    const ImVec2 rasterA{x, a.y};
    dl->AddText(ImVec2{x + 4.0F, ty}, text, raster);
    ImGui::Dummy(ImVec2{breite, h});
    if (ImGui::IsMouseHoveringRect(rasterA, ImVec2{rasterA.x + rasterB, a.y + h})) {
        ImGui::SetTooltip("%s", tr(Str::KeGridStatusTip));
    }
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
    ed.hSplit = std::clamp(zahl("hSplit", ed.hSplit), 0.05F, 0.95F);
    ed.vSplit = std::clamp(zahl("vSplit", ed.vSplit), 0.05F, 0.98F);
    ed.vSplit2 = std::clamp(zahl("vSplit2", ed.vSplit2), 0.05F, 0.95F);
    ed.raster.standardSetzen(std::clamp(static_cast<int>(zahl("GridDefault", karte::Raster::kStandard)), 0, 13));
    ed.raster.setzeMaxKoordPotenz(static_cast<int>(zahl("GridMaxCoordPower", 4)));
    ed.kubikSkala = std::clamp(static_cast<int>(zahl("CubicScale", 14)), 1, 23);
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
    const auto zahl = [](float v) {
        char z[32];
        std::snprintf(z, sizeof(z), "%g", static_cast<double>(v));
        return std::string(z);
    };
    k["QE4StyleWindows"] = std::to_string(static_cast<int>(ed.anordnung));
    k["BuiltInGroupDialog"] = ed.gruppenDialogEingebaut ? "1" : "0";
    k["hSplit"] = zahl(ed.hSplit);
    k["vSplit"] = zahl(ed.vSplit);
    k["vSplit2"] = zahl(ed.vSplit2);
    k["CubicScale"] = std::to_string(ed.kubikSkala);
    std::string versteckt;
    for (const std::string& n : ed.versteckteKnoepfe) { versteckt += "%" + n + "%"; }
    k["ToolbarHiddenButtons"] = versteckt;
    g_app->settings.karteTasten.clear();
    for (const auto& [name, taste] : ed.belegung.abweichende()) { g_app->settings.karteTasten[name] = taste; }
}

}  // namespace bhed::gui::karteneditor
