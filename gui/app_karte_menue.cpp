// app_karte_menue.cpp - Karten-Editor: Thema, Symbole, Menueleiste,
// Werkzeugleisten und Statuszeile wie NetRadiant-Custom.
//
// Portiert aus NetRadiant-Custom (Stand master 2026): radiant/mainframe.cpp
// (create_main_menu, create_main_toolbar, create_main_statusbar,
// MainFrame::SetGridStatus), radiant/filterbar.cpp (create_filter_toolbar,
// FilterToolbarHandler: Rechtsklick), radiant/gtkmisc.cpp (ToolbarItems:
// versteckte Knoepfe, Kontextmenue der Leisten, toolbar_action_set_tooltip),
// radiant/colors.cpp (g_ColoursMenu, Viewports-Themen), radiant/theme.cpp
// (Thema Dark mit Fusion, dark.qss), radiant/qe3.cpp (QE_brushCountChange),
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

ImU32 rgb(int r, int g, int b, int a = 255) { return IM_COL32(r, g, b, a); }
ImVec4 rgbV(int r, int g, int b, float a = 1.0F) {
    return ImVec4{static_cast<float>(r) / 255.0F, static_cast<float>(g) / 255.0F, static_cast<float>(b) / 255.0F, a};
}

// --- Thema (theme.cpp:37-156, dark.qss / darker.qss) -----------------------
struct Thema {
    ImVec4 window, base, altBase, button, text, disabled, highlight, highlightText;
    ImU32 leiste, leisteRand, knopf, knopfHover, knopfHoverRand, knopfAn, knopfAnRand, knopfAnHover, knopfAnHoverRand;
    ImU32 trennerMenue, rollHinten, rollGriff, rollGriffHover;
};

Thema themaFuer(int nr) {
    Thema t{};
    if (nr == 3) {   // Darker (theme.cpp:125-156, darker.qss)
        t.window = rgbV(45, 45, 45);
        t.base = rgbV(18, 18, 18);
        t.altBase = rgbV(14, 14, 14);
        t.button = rgbV(45, 45, 45);
        t.highlight = rgbV(42, 130, 218);
        t.highlightText = rgbV(0, 0, 0);
        t.leiste = rgb(32, 32, 32);
        t.leisteRand = rgb(120, 120, 120);
        t.knopf = rgb(64, 64, 64);
        t.knopfHover = rgb(94, 94, 94);
        t.knopfHoverRand = rgb(154, 154, 154);
        t.knopfAn = IM_COL32(42, 130, 218, 153);
        t.knopfAnRand = IM_COL32(89, 172, 255, 204);
        t.knopfAnHover = IM_COL32(42, 130, 218, 204);
        t.knopfAnHoverRand = IM_COL32(89, 172, 255, 255);
        t.trennerMenue = rgb(93, 94, 91);
        t.rollHinten = rgb(73, 74, 71);
        t.rollGriff = rgb(111, 105, 100);
        t.rollGriffHover = rgb(42, 130, 218);
    } else if (nr == 1) {   // Fusion: Qts Standardpalette des Fusion-Stils
        t.window = rgbV(239, 239, 239);
        t.base = rgbV(255, 255, 255);
        t.altBase = rgbV(247, 247, 247);
        t.button = rgbV(239, 239, 239);
        t.highlight = rgbV(48, 140, 198);
        t.highlightText = rgbV(255, 255, 255);
        t.leiste = rgb(239, 239, 239);
        t.leisteRand = rgb(200, 200, 200);
        t.knopf = rgb(239, 239, 239);
        t.knopfHover = rgb(220, 220, 220);
        t.knopfHoverRand = rgb(160, 160, 160);
        t.knopfAn = rgb(200, 200, 200);
        t.knopfAnRand = rgb(48, 140, 198);
        t.knopfAnHover = rgb(190, 190, 190);
        t.knopfAnHoverRand = rgb(48, 140, 198);
        t.trennerMenue = rgb(200, 200, 200);
        t.rollHinten = rgb(230, 230, 230);
        t.rollGriff = rgb(190, 190, 190);
        t.rollGriffHover = rgb(48, 140, 198);
    } else {   // Dark (theme.cpp:93-124, dark.qss) - NRCs Vorgabe
        t.window = rgbV(83, 84, 81);
        t.base = rgbV(46, 52, 54);
        t.altBase = rgbV(35, 40, 42);
        t.button = rgbV(108, 109, 105);
        t.highlight = rgbV(250, 203, 129);
        t.highlightText = rgbV(0, 0, 0);
        t.leiste = rgb(70, 70, 70);
        t.leisteRand = rgb(120, 120, 120);
        t.knopf = rgb(100, 100, 100);
        t.knopfHover = rgb(120, 120, 120);
        t.knopfHoverRand = rgb(180, 180, 180);
        t.knopfAn = rgb(46, 52, 54);
        t.knopfAnRand = IM_COL32(250, 203, 129, 191);
        t.knopfAnHover = rgb(70, 79, 82);
        t.knopfAnHoverRand = IM_COL32(250, 203, 129, 242);
        t.trennerMenue = rgb(93, 94, 91);
        t.rollHinten = rgb(73, 74, 71);
        t.rollGriff = rgb(111, 105, 100);
        t.rollGriffHover = rgb(250, 203, 129);
    }
    const bool hell = nr == 1;
    t.text = hell ? rgbV(0, 0, 0) : rgbV(255, 255, 255);
    t.disabled = rgbV(127, 127, 127);
    return t;
}

const Thema& thema() {
    static Thema t;
    static int fuer = -1;
    if (fuer != e().guiThema) {
        fuer = e().guiThema;
        t = themaFuer(fuer);
    }
    return t;
}

// --- Symbole ----------------------------------------------------------------
struct Symbole {
    void* textur = nullptr;
    unsigned stand = ~0U;
    int breite = 0;
};
Symbole& symbole() {
    static Symbole s;
    if (s.textur == nullptr || s.stand != render::texturStand()) {
        const image::Image bild = image::decodePng(kKarteIconPng, kKarteIconPngGroesse);
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

// Ein Menuepunkt wie QMenu im Fusion-Stil: unter der Maus mit Highlight-
// Hintergrund und HighlightedText (schwarz im Thema Dark). ImGui kennt keine
// eigene Textfarbe fuer markierte Punkte - deshalb wird die Lage vorher
// bestimmt und die Textfarbe davor gesetzt.
bool menuePunkt(const char* text, const char* taste, bool haken, bool aktiv, const std::string& tooltip) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float h = ImGui::GetTextLineHeightWithSpacing();
    const ImVec2 m = ImGui::GetIO().MousePos;
    const ImGuiWindow* w = ImGui::GetCurrentWindow();
    const bool ueber = aktiv && ImGui::IsWindowHovered() && m.y >= p.y - ImGui::GetStyle().ItemSpacing.y * 0.5F &&
                       m.y < p.y + h - ImGui::GetStyle().ItemSpacing.y * 0.5F && m.x >= w->Pos.x && m.x < w->Pos.x + w->Size.x;
    if (ueber) { ImGui::PushStyleColor(ImGuiCol_Text, thema().highlightText); }
    const bool gewaehlt = ImGui::MenuItem(text, taste != nullptr && taste[0] != 0 ? taste : nullptr, haken, aktiv);
    if (ueber) { ImGui::PopStyleColor(); }
    if (!tooltip.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", tooltip.c_str());
    }
    return gewaehlt;
}

bool menueAnfang(const char* text, bool aktiv = true) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 groesse = ImGui::CalcTextSize(text, nullptr, true);
    const ImGuiStyle& st = ImGui::GetStyle();
    const ImVec2 m = ImGui::GetIO().MousePos;
    const bool ueber = aktiv && ImGui::IsWindowHovered() && m.x >= p.x && m.x < p.x + groesse.x + st.ItemSpacing.x * 2.0F &&
                       m.y >= p.y - st.FramePadding.y && m.y < p.y + groesse.y + st.FramePadding.y * 2.0F;
    if (ueber) { ImGui::PushStyleColor(ImGuiCol_Text, thema().highlightText); }
    const bool offen = ImGui::BeginMenu(text, aktiv);
    if (ueber) { ImGui::PopStyleColor(); }
    return offen;
}

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
                ImGui::PushStyleColor(ImGuiCol_Separator, thema().trennerMenue);
                ImGui::Separator();
                ImGui::PopStyleColor();
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

void farbeWaehlen(const FarbEintrag& f) {
    // ChooseColour::operator() (colors.cpp:73-84): QColorDialog, danach
    // wird gesetzt. Hier ein Farbwaehler im Menue selbst.
    float* wert = (e().farben.*(f.feld)).data();
    const std::string id = std::string("##farbe") + f.name;
    ImGui::ColorPicker3(id.c_str(), wert, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_DisplayRGB);
}

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
        // themes/_colors/*.json (colors.cpp:218-226), load_colors_theme
        for (const FarbThema& t : farbThemen()) {
            if (menuePunkt(t.name, nullptr, false, true, std::string())) {
                const auto& liste = farbEintraege();
                for (std::size_t k = 0; k < liste.size() && k < t.farben.size(); ++k) {
                    ed.farben.*(liste[k].feld) = t.farben[k];
                }
                einstellungenSpeichern();
            }
        }
    } else if (name == "GuiThemen") {
        // theme_construct_menu (theme.cpp:182-200): Radio, Index = ETheme
        static const char* const kNamen[] = {"Default", "Fusion", "Dark", "Darker", "GreyBlue2", "BlenderDark", "BlenderLight"};
        for (int k = 0; k < 7; ++k) {
            const bool da = k == 1 || k == 2 || k == 3;
            if (menuePunkt(kNamen[k], nullptr, ed.guiThema == k, da, da ? std::string() : stufeText("OpenGLFont"))) {
                ed.guiThema = k;
                einstellungenSpeichern();
            }
        }
    } else if (name == "Farben") {
        for (const FarbEintrag& f : farbEintraege()) {
            const Farben::F3& c = ed.farben.*(f.feld);
            ImGui::ColorButton((std::string("##fk") + f.name).c_str(), ImVec4{c[0], c[1], c[2], 1.0F},
                               ImGuiColorEditFlags_NoTooltip, ImVec2{ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()});
            ImGui::SameLine();
            if (ImGui::BeginMenu(f.text)) {
                farbeWaehlen(f);
                ImGui::EndMenu();
            }
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

const std::vector<FarbThema>& farbThemen() {
    using F = Farben::F3;
    const F g25{.25F, .25F, .25F};
    const F g251{.250980F, .250980F, .250980F};
    const F x{1, 0, 0};
    const F y{0, 1, 0};
    const F z{0, 0, 1};
    const F x2{1, .2F, .321568F};
    const F y2{.545098F, .862745F, 0};
    const F z2{.156862F, .564705F, 1};
    // setup/data/tools/themes/_colors/*.json, alphabetisch (Directory_forEach)
    static const std::vector<FarbThema> v = {
        {"Adwaita Dark", {g25, g25, g25, F{.25F, .25F, .25F}, F{.14F, .15F, .15F}, F{.21F, .23F, .23F}, F{0, 0, 0},
                          F{1, 1, 1}, F{.73F, .73F, .73F}, x, x, z, F{.5F, 0, .75F}, z, x, y, z}},
        {"Black and Green", {g25, g25, g25, F{0, 0, 0}, F{.3F, .5F, .5F}, F{.2F, .2F, .2F}, F{1, 1, 1}, z, F{1, 1, 1}, x,
                             x, z, F{.7F, .7F, 0}, z, x, y, z}},
        {"Blender Dark", {g25, g25, g25, F{.225803F, .225803F, .225803F}, F{.301960F, .301960F, .301960F},
                          F{.254902F, .254902F, .254902F}, F{.972549F, .972549F, .972549F}, F{1, 1, 1}, F{0, 0, 0},
                          F{1, .627451F, 0}, F{1, .627451F, 0}, z, F{.516136F, .516136F, .516136F}, z, x, y, z}},
        {"Blender Lighter", {g251, g251, g251, F{.290196F, .290196F, .290196F}, F{.156862F, .156862F, .156862F},
                             F{.239215F, .239215F, .239215F}, F{.721568F, .721568F, .721568F}, F{.250980F, .501960F, 1},
                             F{.843137F, .843137F, .843137F}, F{1, .627450F, .156862F}, F{1, .627450F, .156862F},
                             F{.501960F, 1, 1}, F{.721568F, .721568F, .721568F}, F{.960784F, .945098F, .301960F}, x2, y2, z2}},
        {"Green", {g251, g251, g251, F{.094117F, .317647F, .274509F}, F{.223529F, .450980F, .407843F},
                   F{.145098F, .360784F, .317647F}, F{.223529F, .749019F, .647058F}, F{.250980F, .564705F, 1},
                   F{.901960F, .901960F, .901960F}, F{.298039F, 1, .862745F}, F{.501960F, 1, .901960F}, F{.501960F, 1, 1},
                   F{.486274F, .749019F, .698039F}, F{.960784F, .945098F, .301960F}, x2, y2, z2}},
        {"Maya.Max.Lightwave Emulation", {g25, g25, g25, F{.77F, .77F, .77F}, F{.89F, .89F, .89F}, F{.83F, .83F, .83F},
                                          F{0, 0, 0}, F{1, 1, 1}, F{0, 0, 0}, x, x, z, F{.5F, 0, .75F}, z, x, y, z}},
        {"Q3Radiant Original", {g25, g25, g25, F{.96F, .96F, .96F}, F{.627F, .627F, .627F}, F{.96F, .96F, .96F},
                                F{0, 0, 0}, z, F{0, 0, 0}, x, x, z, F{.5F, 0, .75F}, z, x, y, z}},
        {"QE4 Original", {g25, g25, g25, F{.96F, .96F, .96F}, F{.627F, .627F, .627F}, F{.882F, .882F, .882F}, F{0, 0, 0},
                          z, F{0, 0, 0}, x, x, z, F{.5F, 0, .75F}, z, x, y, z}},
    };
    return v;
}

// --- Thema -------------------------------------------------------------------

namespace {
int g_themaFarben = 0;   // so viele PushStyleColor hat themaAnfang gemacht
}  // namespace

void themaAnfang() {
    const Thema& t = thema();
    g_themaFarben = 0;
    const auto c = [](ImGuiCol k, const ImVec4& v) {
        ImGui::PushStyleColor(k, v);
        ++g_themaFarben;
    };
    const auto u = [](ImGuiCol k, ImU32 v) {
        ImGui::PushStyleColor(k, v);
        ++g_themaFarben;
    };
    c(ImGuiCol_WindowBg, t.window);
    c(ImGuiCol_ChildBg, t.window);
    c(ImGuiCol_PopupBg, t.window);
    c(ImGuiCol_MenuBarBg, t.window);
    c(ImGuiCol_Text, t.text);
    c(ImGuiCol_TextDisabled, t.disabled);
    c(ImGuiCol_FrameBg, t.base);
    c(ImGuiCol_FrameBgHovered, t.altBase);
    c(ImGuiCol_FrameBgActive, t.altBase);
    c(ImGuiCol_Button, t.button);
    u(ImGuiCol_ButtonHovered, rgb(128, 128, 128));   // QPushButton:hover
    u(ImGuiCol_ButtonActive, rgb(120, 120, 120));
    c(ImGuiCol_Header, t.highlight);
    c(ImGuiCol_HeaderHovered, t.highlight);
    c(ImGuiCol_HeaderActive, t.highlight);
    u(ImGuiCol_Separator, rgb(53, 54, 52));
    u(ImGuiCol_ScrollbarBg, t.rollHinten);
    u(ImGuiCol_ScrollbarGrab, t.rollGriff);
    u(ImGuiCol_ScrollbarGrabHovered, t.rollGriffHover);
    u(ImGuiCol_ScrollbarGrabActive, t.rollGriffHover);
    u(ImGuiCol_Border, rgb(53, 54, 52));
    c(ImGuiCol_CheckMark, t.text);
    c(ImGuiCol_Tab, t.button);
    c(ImGuiCol_TabHovered, t.window);
    c(ImGuiCol_TabSelected, t.window);
    c(ImGuiCol_TitleBg, t.base);
    c(ImGuiCol_TitleBgActive, t.base);
    c(ImGuiCol_TableHeaderBg, t.button);
    c(ImGuiCol_TableRowBgAlt, t.altBase);
    c(ImGuiCol_TextSelectedBg, t.highlight);
}

void themaEnde() { ImGui::PopStyleColor(g_themaFarben); }

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
    const Thema& t = thema();
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
    ImU32 hinten = t.knopf;
    ImU32 kante = 0;
    if (an) {
        hinten = hover && da ? t.knopfAnHover : t.knopfAn;
        kante = hover && da ? t.knopfAnHoverRand : t.knopfAnRand;
    } else if (hover && da) {
        hinten = ImGui::IsItemActive() ? rgb(120, 120, 120) : t.knopfHover;
        kante = t.knopfHoverRand;
    }
    dl->AddRectFilled(a, b, hinten);
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
    const ImU32 c = rgb(140, 140, 140);
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
    const Thema& t = thema();
    const float s = symbolGroesse();
    const float pad = 3.0F;   // QToolBar padding 3px
    const float hoehe = s + 4.0F + pad * 2.0F;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const float breite = ImGui::GetContentRegionAvail().x;
    dl->AddRectFilled(a, ImVec2{a.x + breite, a.y + hoehe}, t.leiste);
    dl->AddLine(a, ImVec2{a.x + breite, a.y}, t.leisteRand);
    dl->AddLine(ImVec2{a.x, a.y + hoehe - 1.0F}, ImVec2{a.x + breite, a.y + hoehe - 1.0F}, t.leisteRand);
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
    const Thema& t = thema();
    const float s = symbolGroesse();
    const float pad = 3.0F;
    const float breite = s + 4.0F + pad * 2.0F;
    ImGui::BeginChild("##filterleiste", ImVec2{breite, hoehe}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = ImGui::GetCursorScreenPos();
    dl->AddRectFilled(a, ImVec2{a.x + breite, a.y + hoehe}, t.leiste);
    dl->AddLine(a, ImVec2{a.x, a.y + hoehe}, t.leisteRand);   // QToolBar:right - Rand links
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
    const ImU32 linie = rgb(53, 54, 52);
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
    ed.guiThema = std::clamp(static_cast<int>(zahl("GUITheme", 2)), 0, 6);
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
    for (const FarbEintrag& f : farbEintraege()) {
        const auto fi = k.find(f.name);
        if (fi == k.end()) { continue; }
        std::istringstream in(fi->second);
        Farben::F3 c{};
        if (in >> c[0] >> c[1] >> c[2]) { ed.farben.*(f.feld) = c; }
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
    k["GUITheme"] = std::to_string(ed.guiThema);
    k["CubicScale"] = std::to_string(ed.kubikSkala);
    std::string versteckt;
    for (const std::string& n : ed.versteckteKnoepfe) { versteckt += "%" + n + "%"; }
    k["ToolbarHiddenButtons"] = versteckt;
    for (const FarbEintrag& f : farbEintraege()) {
        const Farben::F3& c = ed.farben.*(f.feld);
        k[f.name] = zahl(c[0]) + " " + zahl(c[1]) + " " + zahl(c[2]);
    }
    g_app->settings.karteTasten.clear();
    for (const auto& [name, taste] : ed.belegung.abweichende()) { g_app->settings.karteTasten[name] = taste; }
}

}  // namespace bhed::gui::karteneditor
