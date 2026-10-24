// app_karte_texturen.cpp - Karten-Editor: der Texturbrowser (Leiste, Filter,
// Verzeichnisbaum, Vorschaubilder, Rollbalken, Maus, Kontextmenue), die
// Befehle des Texturbrowsers, Find / Replace, Shader Info und der Shader-Text.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/texwindow.cpp
// (TextureBrowser_constructWindow, TexWndGLWidget: mousePressEvent,
// mouseDoubleClick, mouseReleaseEvent, wheelEvent, resizeGL, initializeGL;
// TextureBrowser::draw, SelectTexture, TextureBrowser_Tracking_MouseDown/Up,
// TextureBrowser_ViewShader, TextureBrowser_ContextMenu, TreeView_onRowActivated,
// TextureBrowser_SetSelectedShader, TextureBrowser_SetStatus, Filter_QLineEdit,
// TextureBrowser_filterSetModeIcon, die Befehle aus TextureBrowser_Construct,
// RefreshShaders, TextureBrowser_constructPreferences), libs/gtkutil/
// mousepresses.h (MousePresses, DoubleClickTimer 250 ms), radiant/
// findtexturedialog.cpp (FindTextureDialog, FindReplaceTextures),
// radiant/gtkdlgs.cpp (DoShaderInfoDlg, DoShaderView, TextEditor,
// QLineEdit_search) und radiant/renderstate.cpp (FaceShader-Gebrauch).
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
//
// Lagen: die Teile des Browsers in logischen Qt-Punkten wie NRC (Leiste 25,
// Knoepfe 21 mit Abstand 1, Filterfeld 22, Rollbalken 7 - dark.qss), die
// Vorschaubilder in Geraetepunkten (m_width = Breite * devicePixelRatio).
#include "karteneditor.h"

#include "app_internal.h"
#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "karte_gpu.h"
#include "platform.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace bhed::gui::karteneditor {

namespace {

Editor& e() { return editor(); }

std::filesystem::path pfadAus(const std::string& p) {
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(p.data()), p.size()));
}

ImU32 stil(ImGuiCol c, float a = 1.0F) { return ImGui::GetColorU32(c, a); }

constexpr int kLeisteH = 25;        // QSplitter QToolBar: Knopf 21 + 2 x (Rand 1 + Polster 1)
constexpr int kKnopf = 21;          // PM_SmallIconSize 16 + 3 + Rand 2 x 1 (Polster 0)
constexpr int kKnopfAbstand = 1;    // QSplitter QToolBar: spacing 1px
constexpr int kKnopfSymbol = 16;    // PM_SmallIconSize
constexpr int kFilterH = 22;        // QLineEdit::sizeHint
constexpr int kRollbalken = 7;      // dark.qss QScrollBar: width 7px
constexpr int kZeile = 16;          // QTreeView: Zeilenhoehe (Schrift 16)
constexpr int kEinzug = 20;         // QTreeView::indentation
constexpr int kTextEinzug = 3;      // Abstand der Beschriftung im Eintrag
constexpr int kReiterH = 22;        // QTabBar (documentMode)

struct Zustand {
    std::shared_ptr<Spielkontext> spiel;           // fuer dieses Spiel gelten die aktiven Shader
    std::size_t gebrauchStand = static_cast<std::size_t>(-1);
    std::size_t gebrauchLaden = static_cast<std::size_t>(-1);
    bool glInit = false;                           // TexWndGLWidget::initializeGL
    // Verzeichnisbaum (QTreeView mit QStandardItemModel)
    std::vector<karte::TexturGruppe> baum;
    bool baumAlt = true;
    std::size_t baumAnzahl = 0;                    // aktive Shader beim letzten Bau
    std::string baumAktuell;                       // currentIndex: Pfad, Gruppen mit "#"
    std::set<std::string> baumOffen;               // aufgeklappte Gruppen
    bool baumFokus = false;                        // Qt::ClickFocus
    float baumRollen = -1.0F;                      // scrollTo nach Tasten
    // MousePresses
    int knopf = -1;                                // m_button
    int knopfVorher = -1;                          // m_buttonPrev
    double letzterDruck = -1.0e9;                  // DoubleClickTimer
    bool doppelt = false;                          // m_doubleClick.fired()
    // TextureBrowser_Tracking (FreezePointer)
    bool verschieben = false;                      // m_move_started
    int verschiebeMenge = 0;                       // m_move_amount
    ImVec2 anker{0, 0};
    float letzteY = 0;
    ImVec2 druck{0, 0};                            // Fensterpunkt des Drucks (Geraetepunkte)
    // Kontextmenue
    bool menueOeffnen = false;
    std::string menueShader;
    // Rollbalken
    bool rollZiehen = false;
    float rollPack = 0;
    bool teilerGezogen = false;
    TexturBrowserLage lage;
};
Zustand& z() {
    static Zustand s;
    return s;
}

const karte::Texturinventar* inventar() {
    const Editor& ed = e();
    return ed.karteSpiel ? &ed.karteSpiel->inventar : nullptr;
}

// TextureBrowser_getCommonShadersDir / ..._getCommonShadersName (ja.game und
// jk2.game: common_shaders_dir "system/", common_shaders_name "System").
const char* commonName() { return "System"; }

// --- MousePresses (libs/gtkutil/mousepresses.h) ---------------------------------------
enum class Druck { Keiner, Links, Rechts, Mitte, Links2, Rechts2, Mitte2 };
Druck zweifach(int b) { return b == 0 ? Druck::Links2 : b == 1 ? Druck::Rechts2 : Druck::Mitte2; }
Druck einfach(int b) { return b == 0 ? Druck::Links : b == 1 ? Druck::Rechts : Druck::Mitte; }

bool andereTasteGedrueckt(int b) {
    for (int i = 0; i < 3; ++i) {
        if (i != b && ImGui::IsMouseDown(i)) { return true; }
    }
    return false;
}

Druck druecken(int b) {   // MousePresses::press
    Zustand& s = z();
    if (andereTasteGedrueckt(b)) { return Druck::Keiner; }   // event->button() == event->buttons()
    s.knopf = b;
    const double jetzt = ImGui::GetTime();
    s.doppelt = (jetzt - s.letzterDruck) * 1000.0 < 250.0;   // DoubleClickTimer::click
    s.letzterDruck = jetzt;
    return s.doppelt && s.knopf == s.knopfVorher ? zweifach(b) : einfach(b);
}

Druck loslassen() {   // MousePresses::release: wenn die letzte Taste losgelassen ist
    Zustand& s = z();
    if (s.knopf < 0) { return Druck::Keiner; }
    for (int i = 0; i < 3; ++i) {
        if (ImGui::IsMouseDown(i)) { return Druck::Keiner; }
    }
    const Druck r = s.doppelt && s.knopf == s.knopfVorher ? zweifach(s.knopf) : einfach(s.knopf);
    s.knopfVorher = s.knopf;
    s.knopf = -1;
    return r;
}

// --- Shader-Text (gtkdlgs.cpp: DoShaderView, TextEditor) -------------------------------

// DoGtkTextEditor: "scroll to shader" - die Zeile "<name>" oder
// "<name>:q3map", danach irgendwo eine Zeile mit "{".
int shaderZeile(const std::string& text, const std::string& name) {
    std::vector<std::string> zeilen;
    std::size_t p = 0;
    while (p <= text.size()) {
        const std::size_t q = text.find('\n', p);
        zeilen.push_back(text.substr(p, q == std::string::npos ? std::string::npos : q - p));
        if (q == std::string::npos) { break; }
        p = q + 1;
    }
    const auto klein = [](std::string s) {
        for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return s;
    };
    const std::string n = klein(name);
    for (std::size_t i = 0; i < zeilen.size(); ++i) {
        std::string z2 = zeilen[i];
        if (!z2.empty() && z2.back() == '\r') { z2.pop_back(); }
        std::size_t a = 0;
        while (a < z2.size() && std::isspace(static_cast<unsigned char>(z2[a]))) { ++a; }
        const std::string rest = klein(z2.substr(a));
        if (rest != n && rest != n + ":q3map") { continue; }
        for (std::size_t k = i + 1; k < zeilen.size(); ++k) {   // doc->find( "^\s*\{" ) danach
            std::size_t b = 0;
            while (b < zeilen[k].size() && std::isspace(static_cast<unsigned char>(zeilen[k][b]))) { ++b; }
            if (b < zeilen[k].size() && zeilen[k][b] == '{') { return static_cast<int>(i); }
        }
    }
    return -1;
}

void shaderAnsehen(const std::string& datei, const std::string& name, bool extern_) {   // DoShaderView
    Editor& ed = e();
    if (!ed.karteSpiel) { return; }
    bool ordner = false;
    const std::string wurzel = ed.karteSpiel->fs.finden(datei, &ordner);
    const std::string voll = wurzel + (ordner ? "" : "::") + datei;
    if (wurzel.empty()) {
        fehler("Failed to load shader file " + datei);
        return;
    }
    if (extern_ && ordner) {
        // g_TextEditor_editorCommand leer: ShellExecute( pathFull )
        platform::openWithDefaultApp(voll);
        return;
    }
    std::string text;
    if (!ed.karteSpiel->fs.dateiLesen(datei, text)) { return; }
    Editor::ShaderText& t = ed.shaderText;
    if (t.offen && t.editierbar && t.geaendert) {   // ensure_saved
        t.frageSchliessen = true;
        return;
    }
    t.offen = true;
    t.datei = voll;
    t.pfad = voll;
    t.text = text;
    t.editierbar = ordner;
    t.geaendert = false;
    t.cursor = 0;
    t.auswahlEnde = 0;
    t.springen = shaderZeile(text, name);
    t.springenBilder = 3;
    if (t.springen >= 0) {   // setTextCursor auf die Zeile
        std::size_t p = 0;
        for (int i = 0; i < t.springen && p != std::string::npos; ++i) {
            p = text.find('\n', p);
            if (p != std::string::npos) { ++p; }
        }
        t.cursor = p == std::string::npos ? 0 : p;
        t.auswahlEnde = t.cursor;
    }
    ImGui::SetWindowFocus("###shadertext");
}

void shaderInfo(const std::string& name, const std::string& datei) {   // DoShaderInfoDlg
    Editor& ed = e();
    ed.shaderInfoOffen = true;
    ed.shaderInfoName = name;
    ed.shaderInfoDatei = datei;
}

// TextureBrowser_ViewShader
void shaderBei(int mx, int my, bool strg) {
    Editor& ed = e();
    const karte::AktiverShader* s = ed.texBro.texturBei(mx, my, ed.aktiveShader);
    if (s == nullptr) { return; }
    if (s->standard) {
        warnung(s->name + " is not a shader, it's a texture.");
    } else {
        shaderAnsehen(s->datei, s->name, strg);
    }
}

// --- Knoepfe der Leiste ---------------------------------------------------------------

bool leistenKnopf(const char* befehl, const char* bild, Str tip, int lx, int ly, float sc, ImVec2 o) {
    const ImVec2 a{o.x + std::round(static_cast<float>(lx) * sc), o.y + std::round(static_cast<float>(ly) * sc)};
    const ImVec2 b{o.x + std::round(static_cast<float>(lx + kKnopf) * sc), o.y + std::round(static_cast<float>(ly + kKnopf) * sc)};
    const bool da = befehl[0] == 0 || nachgebaut(befehl);
    ImGui::SetCursorScreenPos(a);
    const std::string id = std::string("##texknopf") + (befehl[0] == 0 ? bild : befehl);
    const bool geklickt = ImGui::InvisibleButton(id.c_str(), ImVec2{b.x - a.x, b.y - a.y});
    const bool hover = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (hover && da) {
        dl->AddRectFilled(a, b, stil(ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered),
                          ImGui::GetStyle().FrameRounding + 2.0F);
    }
    const float g = std::round(static_cast<float>(kKnopfSymbol) * sc);
    leistenSymbol(dl, bild, ImVec2{std::floor(a.x + (b.x - a.x - g) * 0.5F), std::floor(a.y + (b.y - a.y - g) * 0.5F)}, g, da);
    if (hover) {
        std::string t = tr(tip);
        const std::string taste = befehl[0] == 0 ? std::string() : befehle::tasteAlsText(e().belegung.taste(befehl));
        if (!taste.empty()) { t += " (" + taste + ")"; }
        ImGui::SetTooltip("%s", t.c_str());
    }
    return geklickt && da;
}

// --- Verzeichnisbaum (TextureBrowser_createTreeViewTree) ----------------------------

struct BaumZeile {
    const karte::TexturGruppe* g;
    int tiefe;
    std::string schluessel;   // Pfad, Gruppen "#name"
};

std::vector<BaumZeile> baumZeilen() {
    Zustand& s = z();
    std::vector<BaumZeile> v;
    for (const auto& g : s.baum) {
        const std::string k = g.pfad.empty() ? "#" + g.name : g.pfad;
        v.push_back({&g, 0, k});
        if (!g.kinder.empty() && s.baumOffen.count(k) != 0U) {
            for (const auto& c : g.kinder) { v.push_back({&c, 1, c.pfad}); }
        }
    }
    return v;
}

// TreeView_onRowActivated
void baumAktivieren(const karte::TexturGruppe& g) {
    if (g.pfad.empty()) { return; }   // empty = directory group root
    texturVerzeichnisZeigen(g.pfad + "/");
    z().baumFokus = false;   // deactivate, so SPACE and RETURN wont be broken for 2d
}

void baumZeichnen(ImVec2 a, ImVec2 c, float sc) {
    Editor& ed = e();
    Zustand& s = z();
    if (s.baumAlt && ed.karteSpiel) {   // TextureBrowser_constructTreeStore
        s.baum = karte::texturGruppenBaum(karte::texturGruppen(ed.aktiveShader, ed.karteSpiel->inventar, ed.texBro.e.nurShaderliste));
        s.baumAlt = false;
    }
    ImGui::SetCursorScreenPos(a);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, std::round(static_cast<float>(kRollbalken) * sc));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{std::round(sc), std::round(sc)});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::BeginChild("##texbaum", ImVec2{c.x - a.x, c.y - a.y}, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
    const float zeile = std::round(static_cast<float>(kZeile) * sc);
    const std::vector<BaumZeile> zeilen = baumZeilen();
    int aktuell = -1;
    for (std::size_t i = 0; i < zeilen.size(); ++i) {
        if (zeilen[i].schluessel == s.baumAktuell) { aktuell = static_cast<int>(i); }
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (std::size_t i = 0; i < zeilen.size(); ++i) {
        const BaumZeile& r = zeilen[i];
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float einzug = static_cast<float>(kEinzug * r.tiefe) * sc;
        const float textX = einzug + static_cast<float>(kEinzug + kTextEinzug) * sc;
        const float textB = ImGui::CalcTextSize(r.g->name.c_str()).x;
        const bool gewaehlt = r.schluessel == s.baumAktuell;
        const std::string id = "##tb" + r.schluessel;
        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
        const bool klick = ImGui::Selectable(id.c_str(), gewaehlt, ImGuiSelectableFlags_AllowDoubleClick,
                                             ImVec2{std::max(ImGui::GetContentRegionAvail().x, textX + textB + 4.0F * sc), zeile});
        ImGui::PopStyleColor();
        if (klick) {
            s.baumAktuell = r.schluessel;
            s.baumFokus = true;
            const float mx = ImGui::GetIO().MousePos.x - p.x;
            const bool pfeil = !r.g->kinder.empty() && mx >= einzug && mx < einzug + static_cast<float>(kEinzug) * sc;
            if (pfeil) {   // Klick auf das Dreieck klappt auf/zu
                if (s.baumOffen.count(r.schluessel) != 0U) {
                    s.baumOffen.erase(r.schluessel);
                } else {
                    s.baumOffen.insert(r.schluessel);
                }
            } else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {   // activated (Fusion: Doppelklick)
                baumAktivieren(*r.g);
            }
        }
        if (!r.g->pfad.empty() && ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", r.g->pfad.c_str()); }   // ToolTipRole
        if (!r.g->kinder.empty()) {   // QStyle::PE_IndicatorBranch
            const bool offen = s.baumOffen.count(r.schluessel) != 0U;
            ImGui::RenderArrow(dl, ImVec2{p.x + einzug + 5.0F * sc, p.y + (zeile - ImGui::GetFontSize()) * 0.5F}, stil(ImGuiCol_Text),
                               offen ? ImGuiDir_Down : ImGuiDir_Right, 0.7F);
        }
        dl->AddText(ImVec2{p.x + textX, p.y + (zeile - ImGui::GetFontSize()) * 0.5F}, stil(ImGuiCol_Text), r.g->name.c_str());
        if (s.baumRollen >= 0.0F && static_cast<int>(i) == aktuell) {
            ImGui::SetScrollHereY(0.5F);
            s.baumRollen = -1.0F;
        }
    }
    // Tasten (QTreeView): hoch/runter, links/rechts zu/auf, Return/Enter aktiviert
    if (s.baumFokus && !zeilen.empty()) {
        e().tasteVerbrauchtBild = ImGui::GetFrameCount();   // ShortcutOverride: accept
        int neu = aktuell;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { neu = std::min(static_cast<int>(zeilen.size()) - 1, aktuell + 1); }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { neu = std::max(0, aktuell - 1); }
        if (neu != aktuell && neu >= 0) {
            s.baumAktuell = zeilen[static_cast<std::size_t>(neu)].schluessel;
            s.baumRollen = 0.0F;
        }
        if (aktuell >= 0) {
            const BaumZeile& r = zeilen[static_cast<std::size_t>(aktuell)];
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && !r.g->kinder.empty()) { s.baumOffen.insert(r.schluessel); }
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) { s.baumOffen.erase(r.schluessel); }
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) { baumAktivieren(*r.g); }
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
    // Klick ausserhalb nimmt den Fokus (Qt::ClickFocus)
    if (s.baumFokus && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1) || ImGui::IsMouseClicked(2)) &&
        !ImGui::IsMouseHoveringRect(a, c)) {
        s.baumFokus = false;
    }
}

// --- Filterfeld (Filter_QLineEdit) -----------------------------------------------------

void filterZeichnen(ImVec2 a, ImVec2 c, float sc) {
    Editor& ed = e();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(a, c, stil(ImGuiCol_FrameBg), ImGui::GetStyle().FrameRounding);
    dl->AddRect(a, c, stil(ImGuiCol_Border), ImGui::GetStyle().FrameRounding);
    const float h = c.y - a.y;
    const float g = std::round(16.0F * sc);
    // Die Aktion vorn (QLineEdit::LeadingPosition): Suchart umschalten
    const ImVec2 ia{a.x + std::round(4.0F * sc), std::floor(a.y + (h - g) * 0.5F)};
    ImGui::SetCursorScreenPos(ia);
    if (ImGui::InvisibleButton("##texfiltermodus", ImVec2{g, g})) { (void)ausfuehren("SearchFromStart"); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::KeTexFilterTip)); }
    leistenSymbol(dl, ed.texBro.e.vonAnfang ? "search_from_start" : "search", ia, g, true);
    // Loeschknopf hinten (setClearButtonEnabled)
    const float loeschB = ed.texFilterText.empty() ? 0.0F : g + std::round(4.0F * sc);
    if (!ed.texFilterText.empty()) {
        const ImVec2 la{c.x - loeschB, std::floor(a.y + (h - g) * 0.5F)};
        ImGui::SetCursorScreenPos(la);
        if (ImGui::InvisibleButton("##texfilterloeschen", ImVec2{g, g})) {
            ed.texFilterText.clear();
            ed.texBro.filter.clear();
            ed.texBro.hoeheGeaendert();
        }
        uiIcon(dl, UiIcon::Schliessen, la, g, stil(ImGui::IsItemHovered() ? ImGuiCol_Text : ImGuiCol_TextDisabled));
    }
    const float x0 = ia.x + g + std::round(3.0F * sc);
    ImGui::SetCursorScreenPos(ImVec2{x0, a.y});
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{0, 0, 0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (h - ImGui::GetFontSize()) * 0.5F)});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0F);
    ImGui::SetNextItemWidth(std::max(1.0F, c.x - loeschB - x0 - std::round(2.0F * sc)));
    const ImGuiID id = ImGui::GetID("##texfilter");
    // enterEvent: setFocus, leaveEvent: clearFocus
    const bool ueber = ImGui::IsMouseHoveringRect(a, c) && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    if (ueber && ImGui::GetActiveID() != id && !ImGui::IsAnyMouseDown()) { ImGui::SetKeyboardFocusHere(); }
    char puffer[256];
    std::snprintf(puffer, sizeof(puffer), "%s", ed.texFilterText.c_str());
    if (ImGui::InputText("##texfilter", puffer, sizeof(puffer), ImGuiInputTextFlags_EscapeClearsAll)) {   // textChanged
        ed.texFilterText = puffer;
        ed.texBro.filter = ed.texFilterText;   // toLatin1
        ed.texBro.hoeheGeaendert();
    }
    if (!ueber && ImGui::GetActiveID() == id) { ImGui::ClearActiveID(); }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

// --- Kontextmenue (TextureBrowser_ContextMenu) -----------------------------------------

void kontextMenue() {
    Editor& ed = e();
    Zustand& s = z();
    if (s.menueOeffnen) {
        ImGui::OpenPopup("##texmenue");
        s.menueOeffnen = false;
    }
    if (!qtPopupAnfang("##texmenue")) { return; }
    const karte::AktiverShader* sh = ed.aktiveShader.finden(s.menueShader);
    const std::string name = sh != nullptr ? sh->name : s.menueShader;
    const bool standard = sh == nullptr || sh->standard;
    const std::string datei = sh != nullptr ? sh->datei : std::string();
    const auto punkt = [](const std::string& id, bool aktiv = true, const char* bild = nullptr) {
        QtPunkt p;
        p.id = id.c_str();
        p.nrc = id.c_str();   // QAction-Text: '&' waere Mnemonik
        p.aktiv = aktiv;
        p.bild = bild;
        return qtPunkt(p);
    };
    // copyable shader name (copy.png als Symbol der Aktion)
    if (punkt(name + "##texkopieren", true, "copy")) { ImGui::SetClipboardText(name.c_str()); }
    qtTrenner();
    if (punkt(trK(Str::KeTexOpenShader), !standard)) { shaderAnsehen(datei, name, false); }
    if (punkt(trK(Str::KeTexOpenShaderExt), !standard)) { shaderAnsehen(datei, name, true); }
    if (punkt(trK(Str::KeTexShaderInfo), !standard)) { shaderInfo(name, datei); }
    qtTrenner();
    if (punkt(trK(Str::KeTexSelObjects))) {   // Scene_BrushPatchSelectByShader
        karte::waehleNachShader(ed.karte, name);
        auswahlGeaendert();
    }
    if (punkt(trK(Str::KeTexSelSurfaces))) {   // Select_FacesAndPatchesByShader
        karte::waehleFlaechenNachShader(ed.karte, name);
        auswahlGeaendert();
    }
    qtTrenner();
    if (qtMenueAnfang(trK(Str::KeTexRecent), !ed.texBro.zuletzt.empty(), trK(Str::KeTexRecent), false)) {
        const std::vector<std::string> ordner = ed.texBro.zuletzt;
        for (const std::string& f : ordner) {
            if (punkt(f + "##texzuletzt")) { texturVerzeichnisZeigen(f); }
        }
        qtMenueEnde();
    }
    if (punkt(trK(Str::KeTexShaderFolder))) {   // TextureBrowser_ShowDirectoryOfShader
        if (ed.karteSpiel) {
            std::vector<std::string> w;
            std::size_t n = 0;
            if (ed.texBro.verzeichnisDesShaders(name, ed.aktiveShader, ed.karteSpiel->inventar, ed.karteSpiel->fs, &w, &n)) {
                for (const std::string& m : w) { warnung(m); }   // waehrend des Aufbaus (texwindow.cpp:679)
                ausgabe("Showing " + std::to_string(n) + " shaders.");
            }
        }
    }
    if (punkt("-> " + ed.texBro.commonVerzeichnis + "##texcommon")) { texturVerzeichnisZeigen(ed.texBro.commonVerzeichnis); }
    qtPopupEnde();
}

// --- Die GL-Flaeche (TexWndGLWidget) -------------------------------------------------

// Ein Rechteck aus Bildpunkten (Spalten x0..x1-1, Zeilen y0..y1-1).
void punkte(ImDrawList* dl, ImVec2 o, int x0, int y0, int x1, int y1, ImU32 farbe) {
    if (x1 <= x0 || y1 <= y0) { return; }
    dl->AddRectFilled(ImVec2{o.x + static_cast<float>(x0), o.y + static_cast<float>(y0)},
                      ImVec2{o.x + static_cast<float>(x1), o.y + static_cast<float>(y1)}, farbe);
}

// TEXBRO_RENDER_BORDER (GL_LINE_LOOP) in Bildpunkten des Fensters: x/y die
// Bildecke oben links (Spalte, Zeile), b/h die Bildgroesse. breit = 2: die
// um 0,5 erweiterten Ecken mit glLineWidth( 2 ). strichel: GL_LINE_STIPPLE
// 1, 0xF000 - der Zaehler laeuft ueber die ganze Schleife.
void rahmen(ImDrawList* dl, ImVec2 o, int x, int y, int b, int h, bool breit, bool strichel, ImU32 farbe) {
    // Die vier Strecken der Schleife mit ihren Bildpunkten in Zeichenfolge:
    // links nach unten, unten nach rechts, rechts nach oben, oben nach links.
    struct Strecke {
        bool senkrecht;
        int fest;        // Spalte (senkrecht) bzw. Zeile
        int von;         // erste Lage entlang der Strecke
        int bis;         // letzte Lage (eingeschlossen)
        int schritt;     // +1 / -1
    };
    std::vector<Strecke> s;
    int dicke = 1;
    if (!breit) {
        // xfMin = x - 1,5 -> Spalte x - 2; xfMax = x + b + 1,5 -> Spalte x + b + 1;
        // yfMax/yfMin -> Zeilen y - 2 und y + h + 1. Diamant-Regel: Anfang mit, Ende ohne.
        const int l = x - 2;
        const int r = x + b + 1;
        const int t = y - 2;
        const int u = y + h + 1;
        s = {{true, l, t, u - 1, 1}, {false, u, l, r - 1, 1}, {true, r, u, t + 1, -1}, {false, t, r, l + 1, -1}};
    } else {
        // 2 breit an den ganzzahligen Ecken x - 2, x + b + 2, y - 2, y + h + 2:
        // je Strecke [min, max) entlang, quer zwei Punkte (gemessen am Foto).
        dicke = 2;
        const int l = x - 2;
        const int r = x + b + 2;
        const int t = y - 2;
        const int u = y + h + 2;
        s = {{true, l, t, u - 1, 1}, {false, u, l, r - 1, 1}, {true, r, u - 1, t, -1}, {false, t, r - 1, l, -1}};
    }
    int zaehler = 0;
    for (const Strecke& st : s) {
        for (int p = st.von;; p += st.schritt) {
            const bool an = !strichel || (zaehler % 16) >= 12;   // 0xF000: Bits 12..15
            ++zaehler;
            if (an) {
                if (dicke == 1) {
                    if (st.senkrecht) {
                        punkte(dl, o, st.fest, p, st.fest + 1, p + 1, farbe);
                    } else {
                        punkte(dl, o, p, st.fest, p + 1, st.fest + 1, farbe);
                    }
                } else if (st.senkrecht) {   // quer: Spalten fest - 1 und fest
                    punkte(dl, o, st.fest - 1, p, st.fest + 1, p + 1, farbe);
                } else {
                    punkte(dl, o, p, st.fest - 1, p + 1, st.fest + 1, farbe);
                }
            }
            if (p == st.bis) { break; }
        }
    }
}

void ansichtZeichnen(ImVec2 a, ImVec2 c) {
    Editor& ed = e();
    Zustand& s = z();
    karte::TexturBrowser& tb = ed.texBro;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // resizeGL: m_width / m_height in Geraetepunkten
    const int b = static_cast<int>(c.x - a.x);
    const int h = static_cast<int>(c.y - a.y);
    if (b != tb.breite || h != tb.hoehe) {
        tb.breite = b;
        tb.hoehe = h;
        tb.hoeheGeaendert();
    }
    s.lage = TexturBrowserLage{a.x, a.y, b, h, true};
    const auto& f = ed.farben.texBro;   // m_color_textureback
    dl->AddRectFilled(a, c, ImGui::GetColorU32(ImVec4{f[0], f[1], f[2], 1.0F}));
    if (!ed.karteSpiel) { return; }
    if (!s.glInit) {   // initializeGL: TextureBrowser_ShowStartupShaders
        s.glInit = true;
        if (tb.e.startShader == 1) { texturVerzeichnisZeigen(tb.commonVerzeichnis); }
    }
    // Die Schrift der GL-Ansichten (wie die 2D-Ansichten: 8 pt bei 96 dpi)
    const float schriftPx = 8.0F * 96.0F / 72.0F * 1.33F;
    ImGui::PushFont(schrift(), schriftPx / std::max(0.01F, g_app->dpiScale * g_app->uiScale));
    const int schriftHoehe = static_cast<int>(std::lround(ImGui::GetFontSize()));
    if (schriftHoehe != tb.schriftHoehe) {
        tb.schriftHoehe = schriftHoehe;
        tb.hoeheGeaendert();
    }
    const float oberlaenge = ImGui::GetFontBaked()->Ascent;
    texturBilderAbholen();
    const std::vector<karte::TexturLage> sicht = tb.sichtbare(ed.aktiveShader);
    const int o = tb.ursprungY(ed.aktiveShader);
    dl->PushClipRect(a, c, true);
    const bool mischen = tb.e.transparenz;   // g_TextureBrowser_enableAlpha
    for (const karte::TexturLage& l : sicht) {
        // Bildecke oben links im Fenster (Zeile = originy - (y - fontHeight))
        const int px = l.x;
        const int py = o - (l.y - tb.schriftHoehe);
        // Rahmen (glLineWidth 1 bzw. 2), dann die Strichel der Shader
        if (l.rand == karte::TexturLage::Rand::Gewaehlt) {
            rahmen(dl, a, px, py, l.b, l.h, true, false, IM_COL32(255, 0, 0, 255));
        } else if (l.rand == karte::TexturLage::Rand::ImGebrauch) {
            rahmen(dl, a, px, py, l.b, l.h, false, false, IM_COL32(128, 255, 128, 255));
        } else if (l.rand == karte::TexturLage::Rand::Shader) {
            rahmen(dl, a, px, py, l.b, l.h, false, false, IM_COL32(255, 255, 255, 255));
        }
        if (l.strichel) {   // die Breite bleibt die des Rahmens (gewaehlt: 2)
            rahmen(dl, a, px, py, l.b, l.h, l.rand == karte::TexturLage::Rand::Gewaehlt, true, IM_COL32(0, 0, 0, 255));
        }
        if (mischen) {   // checkerboard for transparent textures (von unten nach oben)
            for (int i = 0; i < l.h; i += 8) {
                for (int j = 0; j < l.b; j += 8) {
                    const int wert = ((i + j) / 8) % 2 != 0 ? 0x66 : 0x99;
                    const int unten = std::min(i + 8, l.h);
                    punkte(dl, a, px + j, py + l.h - unten, px + std::min(j + 8, l.b), py + l.h - i,
                           IM_COL32(wert, wert, wert, 255));
                }
            }
        }
        if (void* bild = texturBild(l.shader->name)) {
            if (!mischen && kartegpu::verfuegbar()) { kartegpu::ohneMischen(dl); }   // glDisable( GL_BLEND )
            dl->AddImage(reinterpret_cast<ImTextureID>(bild), ImVec2{a.x + static_cast<float>(px), a.y + static_cast<float>(py)},
                         ImVec2{a.x + static_cast<float>(px + l.b), a.y + static_cast<float>(py + l.h)});
            if (!mischen && kartegpu::verfuegbar()) { dl->AddCallback(ImDrawCallback_ResetRenderState, nullptr); }
        }
        // der Name ohne Verzeichnis; Grundlinie 3 Punkte ueber dem Bild
        const std::string n = karte::pfadDateiname(l.shader->name);
        dl->AddText(ImVec2{a.x + static_cast<float>(px), a.y + static_cast<float>(py - 3) - oberlaenge}, IM_COL32(255, 255, 255, 255),
                    n.c_str());
    }
    dl->PopClipRect();
    ImGui::PopFont();
}

void ansichtMaus(ImVec2 a, ImVec2 c) {
    Editor& ed = e();
    Zustand& s = z();
    karte::TexturBrowser& tb = ed.texBro;
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetCursorScreenPos(a);
    ImGui::InvisibleButton("##texbroAnsicht", ImVec2{std::max(1.0F, c.x - a.x), std::max(1.0F, c.y - a.y)},
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    const bool ueber = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && ImGui::IsMouseHoveringRect(a, c);
    const int mx = static_cast<int>(std::floor(io.MousePos.x - a.x));
    const int my = static_cast<int>(std::floor(io.MousePos.y - a.y));
    const float qt = std::max(1.0F, std::round(g_app->dpiScale));
    const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
    for (int bt = 0; bt < 3; ++bt) {
        if (!(ueber && ImGui::IsMouseClicked(bt))) { continue; }
        const Druck d = druecken(bt);   // mousePressEvent
        if (d == Druck::Links2) {         // mouseDoubleClick: load directory, containing active shader + focus on it
            if (ed.karteSpiel) {
                std::vector<std::string> w;
                std::size_t n = 0;
                if (tb.verzeichnisDesShaders(tb.shader, ed.aktiveShader, ed.karteSpiel->inventar, ed.karteSpiel->fs, &w, &n)) {
                    for (const std::string& m : w) { warnung(m); }   // waehrend des Aufbaus (texwindow.cpp:679)
                    ausgabe("Showing " + std::to_string(n) + " shaders.");
                }
            }
        } else if (d == Druck::Rechts) {   // TextureBrowser_Tracking_MouseDown
            s.verschieben = true;
            s.verschiebeMenge = 0;
            s.anker = io.MousePos;
            s.letzteY = io.MousePos.y;
            s.druck = ImVec2{static_cast<float>(mx), static_cast<float>(my)};
        } else if (d == Druck::Links || d == Druck::Mitte) {
            if (!io.KeyShift) { texturKlick(mx, my, d == Druck::Mitte); }
        }
    }
    if (s.verschieben) {   // FreezePointer: Bewegung in logischen Punkten
        const float dy = ImGui::IsMousePosValid() ? (io.MousePos.y - s.letzteY) / qt : 0.0F;
        const int iy = static_cast<int>(std::lround(dy));
        if (iy != 0) {
            s.verschiebeMenge += std::abs(iy);
            const int skala = io.KeyShift ? 4 : 1;
            tb.setzeUrsprungY(tb.ursprungY(ed.aktiveShader) + iy * skala, ed.aktiveShader);
            s.letzteY += static_cast<float>(iy) * qt;
            if (!imTest) {
                ImGui::TeleportMousePos(s.anker);
                s.letzteY = s.anker.y;
            }
        }
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    }
    for (int bt = 0; bt < 3; ++bt) {
        if (!ImGui::IsMouseReleased(bt) || s.knopf < 0) { continue; }
        const Druck d = loslassen();   // mouseReleaseEvent
        if (d == Druck::Rechts) {
            if (s.verschieben) {   // TextureBrowser_Tracking_MouseUp
                s.verschieben = false;
                if (!imTest) { ImGui::TeleportMousePos(s.anker); }
            }
            if (s.verschiebeMenge < 16) {   // TextureBrowser_ContextMenu: geklickter oder aktiver Shader
                const karte::AktiverShader* sh =
                    tb.texturBei(static_cast<int>(s.druck.x), static_cast<int>(s.druck.y), ed.aktiveShader);
                if (sh != nullptr) {
                    s.menueShader = sh->name;
                } else {
                    s.menueShader = tb.shader;
                    if (inventar() != nullptr) { (void)ed.aktiveShader.holen(tb.shader, inventar()); }   // QERApp_Shader_ForName
                }
                s.menueOeffnen = true;
            }
        } else if (d == Druck::Links && io.KeyShift) {
            shaderBei(mx, my, io.KeyCtrl);   // TextureBrowser_ViewShader
        } else if (s.verschieben) {
            s.verschieben = false;
        }
    }
    if (ueber) {   // wheelEvent
        for (const int r : radEreignisse()) {
            const int schritt = r > 0 ? tb.e.radSchritt : -tb.e.radSchritt;   // copysign( m_mouseWheelScrollIncrement, delta )
            tb.setzeUrsprungY(tb.ursprungY(ed.aktiveShader) + schritt, ed.aktiveShader);
        }
    }
    kontextMenue();
}

// QScrollBar (vertikal) neben der GL-Flaeche: Wert = -originy, Seite = Hoehe.
void rollbalken(ImVec2 a, ImVec2 c, float sc) {
    Editor& ed = e();
    Zustand& s = z();
    karte::TexturBrowser& tb = ed.texBro;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(a, c, stil(ImGuiCol_ScrollbarBg, 1.0F));
    if (!ed.karteSpiel) { return; }
    const karte::TexturBrowser::Rollen r = tb.rollen(ed.aktiveShader);
    const float spur = c.y - a.y;
    const float laenge = r.maximum > 0 ? std::max(std::round(20.0F * sc), spur * static_cast<float>(r.seite) /
                                                                            static_cast<float>(r.maximum + r.seite))
                                       : spur;
    const float lage = r.maximum > 0 ? (spur - laenge) * static_cast<float>(r.wert) / static_cast<float>(r.maximum) : 0.0F;
    const ImVec2 ga{a.x, a.y + lage};
    const ImVec2 gc{c.x, a.y + lage + laenge};
    ImGui::SetCursorScreenPos(a);
    ImGui::InvisibleButton("##texrollbalken", ImVec2{std::max(1.0F, c.x - a.x), std::max(1.0F, spur)});
    const bool ueber = ImGui::IsItemHovered();
    const float my = ImGui::GetIO().MousePos.y;
    const bool aufGriff = my >= ga.y && my < gc.y;
    const auto setzeWert = [&](int wert) { tb.setzeUrsprungY(-std::clamp(wert, 0, r.maximum), ed.aktiveShader); };
    if (ImGui::IsItemActivated()) {
        if (aufGriff) {
            s.rollZiehen = true;
            s.rollPack = my - ga.y;
        }
    }
    if (ImGui::IsItemActive() && !s.rollZiehen && ImGui::IsMouseClicked(0, true) && r.maximum > 0) {   // Seite vor/zurueck
        setzeWert(r.wert + (my < ga.y ? -r.seite : r.seite));
    }
    if (s.rollZiehen) {
        if (ImGui::IsMouseDown(0) && spur > laenge) {
            const float neu = std::clamp(my - s.rollPack - a.y, 0.0F, spur - laenge);
            setzeWert(static_cast<int>(std::lround(neu / (spur - laenge) * static_cast<float>(r.maximum))));
        } else {
            s.rollZiehen = false;
        }
    }
    if (ueber) {   // QAbstractSlider::wheelEvent: 3 x singleStep (20)
        for (const int w : radEreignisse()) { setzeWert(r.wert - (w > 0 ? 60 : -60)); }
    }
    const bool hell = ueber && aufGriff;
    dl->AddRectFilled(ga, gc, stil(s.rollZiehen ? ImGuiCol_ScrollbarGrabActive : hell ? ImGuiCol_ScrollbarGrabHovered : ImGuiCol_ScrollbarGrab));
    dl->AddRect(ga, gc, stil(ImGuiCol_Border));
}

}  // namespace

// --- oeffentlich -----------------------------------------------------------------------

const TexturBrowserLage& texturBrowserLage() { return z().lage; }

void texturGlNeu() { z().glInit = false; }

bool texturMitte(const std::string& shader, ImVec2& mitte) {
    Editor& ed = e();
    const TexturBrowserLage& l = z().lage;
    if (!l.gezeichnet) { return false; }
    const int o = ed.texBro.ursprungY(ed.aktiveShader);
    for (const karte::TexturLage& t : ed.texBro.anordnen(ed.aktiveShader)) {
        if (t.shader->name != shader) { continue; }
        const float x = static_cast<float>(t.x) + static_cast<float>(t.b) * 0.5F;
        const float y = static_cast<float>(o - (t.y - ed.texBro.schriftHoehe)) + static_cast<float>(t.h) * 0.5F;
        if (y < 0.0F || y >= static_cast<float>(l.hoehe)) { return false; }
        mitte = ImVec2{l.ansichtX + x, l.ansichtY + y};
        return true;
    }
    return false;
}

void texturenAktualisieren() {
    Editor& ed = e();
    Zustand& s = z();
    if (ed.karteSpiel != s.spiel) {   // anderes Spiel: die Shader neu (FreeShaders)
        ed.aktiveShader.leeren();
        ed.texBro.neuGeladen();
        s.spiel = ed.karteSpiel;
        s.baumAlt = true;
        s.glInit = false;
        s.gebrauchStand = static_cast<std::size_t>(-1);
    }
    if (!ed.karteSpiel) { return; }
    // common_shaders_dir aus ja.game / jk2.game
    ed.texBro.commonVerzeichnis = "system/";
    // Nach jedem Befehl, Rueckgaengig und neuer Karte: die Shader der Karte
    // anfordern (GlobalShaderCache().capture) und ihren Gebrauch setzen
    // (FaceShader::instanceAttach -> incrementUsed -> SetInUse).
    const std::size_t stand = ed.karte.undo.abschluesse();
    if (stand == s.gebrauchStand && ed.ladeZaehler == s.gebrauchLaden) { return; }
    s.gebrauchStand = stand;
    s.gebrauchLaden = ed.ladeZaehler;
    std::set<std::string, karte::ShaderKleiner> namen;
    bool pinsel = false;
    for (const auto& ent : ed.karte.entitaeten()) {
        for (const auto& p : ent->kinder()) {
            if (p->art() == karte::Primitiv::Art::Pinsel) {
                pinsel = true;
                for (const auto& fl : static_cast<const karte::Pinsel&>(*p).flaechen()) { namen.insert(fl->shader()); }
            } else {
                namen.insert(static_cast<const karte::Patch&>(*p).shader());
            }
        }
    }
    const karte::Texturinventar* inv = inventar();
    if (pinsel) { (void)ed.aktiveShader.holen(karte::kTexturPraefix, inv); }   // Face( FaceObserver* ): texdef_name_default
    for (const std::string& n : namen) { (void)ed.aktiveShader.holen(n, inv); }
    (void)ed.aktiveShader.gebrauchSetzen(namen);
    // TextureGroups_addShader sieht auch die ohne Definition angeforderten:
    // der Baum wird neu gebaut, wenn neue Shader dazukamen.
    if (ed.aktiveShader.alle().size() != s.baumAnzahl) {
        s.baumAnzahl = ed.aktiveShader.alle().size();
        s.baumAlt = true;
    }
}

void texturShaderWaehlen(const std::string& shader) {   // TextureBrowser_SetSelectedShader
    Editor& ed = e();
    ed.texBro.shader = shader;
    if (const karte::Texturinventar* inv = inventar()) {   // TextureBrowser_SetStatus: QERApp_Shader_ForName
        const karte::AktiverShader& s = ed.aktiveShader.holen(shader, inv);
        ed.statusTextur = karte::texturStatus(shader, s);
    }
    ed.texBro.fokus(shader, ed.aktiveShader);
    if (ed.tags.gui && ed.tags.reiter == 1) { tagsShaderGewaehlt(); }   // m_tabs->currentIndex() == 1
    if (ed.findenOffen) {   // FindTextureDialog_selectTexture -> updateTextures
        const std::string kurz = shader.size() >= 9 ? shader.substr(9) : std::string();
        if (ed.findenAktiv) {
            ed.findenText = kurz;
        } else {
            ed.ersetzenText = kurz;
        }
    }
}

void texturKlick(int mx, int my, bool nurWaehlen) {   // SelectTexture
    Editor& ed = e();
    const karte::AktiverShader* s = ed.texBro.texturBei(mx, my, ed.aktiveShader);
    if (s == nullptr) { return; }
    const std::string name = s->name;
    texturShaderWaehlen(name);
    // TextureBrowser_textureSelected = TextureClipboard_textureSelected: die
    // Zwischenablage des Surface Inspectors (Stufe 2).
    if (!ed.findenOffen && !nurWaehlen) { (void)karte::shaderSetzen(ed.karte, ed.bearbeitung.einstellung, name); }
}

void texturVerzeichnisZeigen(const std::string& dir) {
    Editor& ed = e();
    if (!ed.karteSpiel) { return; }
    std::vector<std::string> w;
    const std::size_t n = ed.texBro.verzeichnisZeigen(dir, ed.aktiveShader, ed.karteSpiel->inventar, ed.karteSpiel->fs, &w);
    for (const std::string& m : w) { warnung(m); }   // waehrend des Aufbaus (texwindow.cpp:679)
    ausgabe("Showing " + std::to_string(n) + " shaders.");
}

void texturBrowserZeichnen(int breite, int hoehe) {
    Editor& ed = e();
    Zustand& s = z();
    const float sc = massstab();
    karte::Teiler& t = ed.texTeiler;
    if (t.kinder.size() != 2) {   // splitter->setStretchFactor( 0, 0 ) / ( 1, 1 ), addSplitter( { 100, 800 } )
        t = karte::Teiler{};
        karte::Teiler::Kind links;
        links.hinweis = -1;
        links.dehnfaktor = 0;
        karte::Teiler::Kind rechts;
        rechts.hinweis = -1;
        rechts.dehnfaktor = 1;
        t.kinder = {links, rechts};
        t.setzeGroessen(ed.texGroessen.empty() ? std::vector<int>{100, 800} : ed.texGroessen);
    }
    const ImVec2 o = ImGui::GetWindowPos();
    const auto bp = [&](int x, int y) {
        return ImVec2{o.x + std::round(static_cast<float>(x) * sc), o.y + std::round(static_cast<float>(y) * sc)};
    };
    const TeilerInhalt linksZeichnen = [&](int x, int y, int b, int h) {
        if (b <= 0 || h <= 0) { return; }
        merkeFeld("Texturen:Links", x, y, b, h);
        // QToolBar: View, Find / Replace..., Flush & Reload Shaders
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(bp(x, y), bp(x + b, y + kLeisteH), stil(ImGuiCol_MenuBarBg));
        const auto& knoepfe = befehle::texturKnoepfe();
        int kx = x + 2;
        for (std::size_t i = 0; i < knoepfe.size(); ++i) {
            const befehle::Knopf& k = knoepfe[i];
            if (k.trenner) { continue; }
            merkeFeld(std::string("Texturen:Knopf:") + (k.befehl[0] == 0 ? k.bild : k.befehl), kx, y + 2, kKnopf, kKnopf);
            if (leistenKnopf(k.befehl, k.bild, k.zeilen[0], kx, y + 2, sc, o)) {
                if (k.befehl[0] == 0) {   // View: menu->popup( QCursor::pos() )
                    ImGui::OpenPopup("##texansicht");
                } else {
                    (void)ausfuehren(k.befehl);
                }
            }
            kx += kKnopf + kKnopfAbstand;
        }
        if (qtPopupAnfang("##texansicht", true)) {   // TextureBrowser_constructViewMenu (texwindow.cpp:1237: abreissbar)
            menueEintraege(befehle::texturMenues(), 1);
            qtPopupEnde();
        }
        merkeFeld("Texturen:Filter", x, y + kLeisteH, b, kFilterH);
        filterZeichnen(bp(x, y + kLeisteH), bp(x + b, y + kLeisteH + kFilterH), sc);
        int baumY = y + kLeisteH + kFilterH;
        if (ed.tags.gui) {   // QTabWidget mit tabBarAutoHide: die Leiste nur mit dem Reiter Tags
            merkeFeld("Texturen:Reiter", x, baumY, b, kReiterH);
            tagsReiterZeichnen(bp(x, baumY), bp(x + b, baumY + kReiterH), sc);
            baumY += kReiterH;
        }
        merkeFeld("Texturen:Baum", x, baumY, b, std::max(0, y + h - baumY));
        if (y + h > baumY) {
            if (ed.tags.gui && ed.tags.reiter == 1) {
                tagsListeZeichnen(bp(x, baumY), bp(x + b, y + h), sc);
            } else {
                baumZeichnen(bp(x, baumY), bp(x + b, y + h), sc);
            }
        }
    };
    const TeilerInhalt rechtsZeichnen = [&](int x, int y, int b, int h) {
        if (b <= 0 || h <= 0) { return; }
        const int rb = ed.texBro.e.rollbalken ? kRollbalken : 0;
        merkeFeld("Texturen:Ansicht", x, y, std::max(0, b - rb), h);
        const ImVec2 a = bp(x, y);
        const ImVec2 c = bp(x + std::max(0, b - rb), y + h);
        ansichtZeichnen(a, c);
        ansichtMaus(a, c);
        if (rb > 0) {
            merkeFeld("Texturen:Rollbalken", x + b - rb, y, rb, h);
            rollbalken(bp(x + b - rb, y), bp(x + b, y + h), sc);
        }
    };
    teilerImFenster("tex", t, false, breite, hoehe, {linksZeichnen, rechtsZeichnen});
    // g_guiSettings: die Groessen merken, wenn der Griff gezogen wurde
    if (ImGui::GetActiveID() == ImGui::GetID("##grifftex1")) { s.teilerGezogen = true; }
    if (s.teilerGezogen && !ImGui::IsMouseDown(0)) {
        s.teilerGezogen = false;
        ed.texGroessen = t.groessen();
        einstellungenSpeichern();
    }
}

// --- Befehle (TextureBrowser_Construct, FindTextureDialog_Construct) -----------------------

bool texturBefehlAn(const std::string& name) {
    const karte::TexturBrowser& tb = e().texBro;
    if (name == "ShowInUse") { return tb.unbenutzteVerstecken; }
    if (name == "ToggleShowShaders") { return tb.e.shaderZeigen; }
    if (name == "ToggleShowTextures") { return tb.e.texturenZeigen; }
    if (name == "ToggleShowShaderlistOnly") { return tb.e.nurShaderliste; }
    if (name == "FixedSize") { return tb.e.festeGroesse; }
    if (name == "FilterNotex") { return tb.e.ohneNotex; }
    if (name == "EnableAlpha") { return tb.e.transparenz; }
    if (name == "SearchFromStart") { return tb.e.vonAnfang; }
    return false;
}

void texturBefehl(const std::string& name) {
    Editor& ed = e();
    karte::TexturBrowser& tb = ed.texBro;
    if (name == "ShowInUse") {   // TextureBrowser_ToggleHideUnused
        tb.unbenutzteSetzen(!tb.unbenutzteVerstecken);
    } else if (name == "ShowAllTextures") {   // TextureBrowser_showAll
        tb.alleZeigen();
    } else if (name == "ToggleShowShaders") {
        tb.e.shaderZeigen = !tb.e.shaderZeigen;
        tb.hoeheGeaendert();
    } else if (name == "ToggleShowTextures") {
        tb.e.texturenZeigen = !tb.e.texturenZeigen;
        tb.hoeheGeaendert();
    } else if (name == "ToggleShowShaderlistOnly") {   // TextureBrowser_constructTreeStore
        tb.e.nurShaderliste = !tb.e.nurShaderliste;
        z().baumAlt = true;
    } else if (name == "FixedSize") {
        tb.e.festeGroesse = !tb.e.festeGroesse;
        tb.hoeheGeaendert();
    } else if (name == "FilterNotex") {
        tb.e.ohneNotex = !tb.e.ohneNotex;
        tb.hoeheGeaendert();
    } else if (name == "EnableAlpha") {
        tb.e.transparenz = !tb.e.transparenz;
        tb.hoeheGeaendert();
    } else if (name == "SearchFromStart") {   // TextureBrowser_filter_searchFromStart (+ filterSetModeIcon)
        tb.e.vonAnfang = !tb.e.vonAnfang;
        tb.hoeheGeaendert();
    } else if (name == "RefreshShaders") {   // RefreshShaders: Shader neu laden, Baum neu
        tb.neuGeladen();
        if (ed.karteSpiel) {
            ed.karteSpiel->inventar.laden(ed.karteSpiel->fs);
            ed.aktiveShader.leeren();
            texturBilderNeuLaden();
            Zustand& s = z();
            s.baumAlt = true;
            s.gebrauchStand = static_cast<std::size_t>(-1);
            texturenAktualisieren();   // die Shader der Karte wieder anfordern
        }
    } else if (name == "FindReplaceTextures") {   // FindTextureDialog::show
        ed.findenOffen = true;
        ImGui::SetWindowFocus("###findenersetzen");
    }
    einstellungenSpeichern();
}

// --- Dialoge -------------------------------------------------------------------------------

void texturDialogeZeichnen() {
    Editor& ed = e();
    const float sc = massstab();
    // FindTextureDialog::BuildDialog: QFormLayout (Find, Replace, Checkbox) und
    // daneben senkrecht Apply / Close.
    if (ed.findenOffen) {
        const std::string titel = std::string(trK(Str::KeFrTitle)) + "###findenersetzen";
        bool offen = true;
        if (ImGui::Begin(titel.c_str(), &offen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::BeginGroup();
            const float beschriftung = std::max(ImGui::CalcTextSize(trK(Str::KeFrFind)).x, ImGui::CalcTextSize(trK(Str::KeFrReplace)).x) +
                                       ImGui::GetStyle().CellPadding.x * 2.0F;
            const float feld = std::max(std::round(220.0F * sc), ImGui::CalcTextSize(trK(Str::KeFrSelected)).x + ImGui::GetFrameHeight() * 2.0F);
            if (ImGui::BeginTable("##findenform", 2, ImGuiTableFlags_SizingFixedFit, ImVec2{beschriftung + feld, 0.0F})) {
                ImGui::TableSetupColumn("##l", ImGuiTableColumnFlags_WidthFixed, beschriftung);
                ImGui::TableSetupColumn("##f", ImGuiTableColumnFlags_WidthFixed, feld);
                const auto zeile = [&](Str text, std::string& wert, const char* id, const char* platz, bool suchen) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted(trK(text));
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    char puffer[256];
                    std::snprintf(puffer, sizeof(puffer), "%s", wert.c_str());
                    if (ImGui::InputTextWithHint(id, platz, puffer, sizeof(puffer))) { wert = puffer; }
                    if (ImGui::IsItemActivated()) { ed.findenAktiv = suchen; }   // FindActiveTracker (FocusIn)
                };
                zeile(Str::KeFrFind, ed.findenText, "##finden", "", true);
                zeile(Str::KeFrReplace, ed.ersetzenText, "##ersetzen", trK(Str::KeFrPlaceholder), false);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(1);
                (void)qtHaken(trK(Str::KeFrSelected), &ed.findenNurAuswahl, 17.0F);   // QCheckBox: max(fm 13 + 4, 14)
                ImGui::EndTable();
            }
            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            const float kb = std::round(79.0F * sc);
            if (ImGui::Button(trK(Str::KeFrApply), ImVec2{kb, 0})) {   // FindTextureDialog_apply
                std::string ersetzen = ed.ersetzenText;
                std::replace(ersetzen.begin(), ersetzen.end(), '\\', '/');   // PathCleaned
                std::string meldung;
                if (!karte::texturenErsetzen(ed.karte, ed.bearbeitung.einstellung, "textures/" + ed.findenText, "textures/" + ersetzen,
                                             ed.findenNurAuswahl, &meldung)) {
                    fehler(meldung);
                }
                auswahlGeaendert();
            }
            if (ImGui::Button(trK(Str::KeFrClose), ImVec2{kb, 0})) { offen = false; }   // HideDlg
            ImGui::EndGroup();
        }
        ImGui::End();
        if (!offen) { ed.findenOffen = false; }
    }
    // DoShaderInfoDlg: qt_MessageBox mit OK
    if (ed.shaderInfoOffen) {
        ImGui::OpenPopup("###shaderinfo");
        ed.shaderInfoOffen = false;
    }
    const std::string infoTitel = std::string(trK(Str::KeTexShaderInfo)) + "###shaderinfo";
    if (ImGui::BeginPopupModal(infoTitel.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("  %s", tr(Str::KeSiSelected));
        ImGui::TextUnformatted(ed.shaderInfoName.c_str());
        ImGui::Text("  %s", tr(Str::KeSiLocated));
        ImGui::TextUnformatted(ed.shaderInfoDatei.c_str());
        if (ImGui::Button(trK(Str::KePrefOk), ImVec2{std::round(79.0F * sc), 0})) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    // TextEditor (DoGtkTextEditor): Text, darunter Save, Suchfeld
    Editor::ShaderText& t = ed.shaderText;
    if (t.offen) {
        ImGui::SetNextWindowSize(ImVec2{std::round(550.0F * sc), std::round(700.0F * sc)}, ImGuiCond_FirstUseEver);
        const std::string titel = (t.geaendert ? "*" : "") + t.datei + "###shadertext";
        bool offen = true;
        if (ImGui::Begin(titel.c_str(), &offen, ImGuiWindowFlags_NoCollapse)) {
            const float unten = ImGui::GetFrameHeightWithSpacing();
            const float zeileH = ImGui::GetTextLineHeight();
            if (t.springen >= 0 && t.springenBilder > 0) {   // scroll to shader: die Zeile oben
                ImGui::SetNextWindowScroll(ImVec2{-1.0F, static_cast<float>(t.springen) * zeileH});
                if (--t.springenBilder == 0) { t.springen = -1; }
            }
            if (t.editierbar) {
                std::vector<char> puffer(t.text.begin(), t.text.end());
                puffer.resize(t.text.size() + 4096, '\0');
                if (ImGui::InputTextMultiline("##shadertextfeld", puffer.data(), puffer.size(), ImVec2{-FLT_MIN, -unten},
                                              ImGuiInputTextFlags_AllowTabInput)) {
                    t.text = puffer.data();
                    t.geaendert = true;
                }
            } else {   // setReadOnly( true ): lesen, die Fundstelle hervorgehoben
                ImGui::BeginChild("##shadertextfeld", ImVec2{-FLT_MIN, -unten}, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
                ImDrawList* dl = ImGui::GetWindowDrawList();
                std::size_t p = 0;
                while (p <= t.text.size()) {
                    std::size_t q = t.text.find('\n', p);
                    if (q == std::string::npos) { q = t.text.size(); }
                    std::string zeile = t.text.substr(p, q - p);
                    if (!zeile.empty() && zeile.back() == '\r') { zeile.pop_back(); }
                    const ImVec2 pos = ImGui::GetCursorScreenPos();
                    if (t.auswahlEnde > t.cursor && t.cursor >= p && t.cursor <= q) {   // Auswahl der Suche
                        const float x0 = ImGui::CalcTextSize(zeile.substr(0, t.cursor - p).c_str()).x;
                        const float x1 = ImGui::CalcTextSize(zeile.substr(0, std::min(t.auswahlEnde, q) - p).c_str()).x;
                        dl->AddRectFilled(ImVec2{pos.x + x0, pos.y}, ImVec2{pos.x + x1, pos.y + zeileH}, stil(ImGuiCol_TextSelectedBg));
                    }
                    ImGui::TextUnformatted(zeile.c_str());
                    if (q >= t.text.size()) { break; }
                    p = q + 1;
                }
                ImGui::EndChild();
            }
            ImGui::BeginDisabled(!t.geaendert);
            if (ImGui::Button(trK(Str::KeSave))) {   // editor_save
                std::ofstream f(pfadAus(t.pfad), std::ios::binary);
                if (f) {
                    f << t.text;
                    t.geaendert = false;
                } else {
                    fehler("Error saving file\"" + t.pfad + "\"");
                }
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            char puffer[256];
            std::snprintf(puffer, sizeof(puffer), "%s", t.suchen.c_str());
            const bool rueck = ImGui::IsKeyDown(ImGuiMod_Shift) || ImGui::IsKeyDown(ImGuiMod_Ctrl) || ImGui::IsKeyPressed(ImGuiKey_UpArrow);
            const bool getippt = ImGui::InputText("##shadersuchen", puffer, sizeof(puffer));
            const bool aktiv = ImGui::IsItemActive();
            if (t.suchen.empty() && !aktiv) {   // setPlaceholderText( Lupe )
                const ImVec2 r = ImGui::GetItemRectMin();
                uiIcon(ImGui::GetWindowDrawList(), UiIcon::Suche,
                       ImVec2{r.x + ImGui::GetStyle().FramePadding.x, r.y + ImGui::GetStyle().FramePadding.y}, ImGui::GetTextLineHeight(),
                       stil(ImGuiCol_TextDisabled));
            }
            const bool weiter = aktiv && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter) ||
                                          ImGui::IsKeyPressed(ImGuiKey_DownArrow) || ImGui::IsKeyPressed(ImGuiKey_UpArrow));
            if (getippt) { t.suchen = puffer; }
            if ((getippt || weiter) && !t.suchen.empty()) {   // QLineEdit_search::search (ohne Gross/klein)
                const auto klein = [](std::string s) {
                    for (char& ch : s) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
                    return s;
                };
                const std::string text = klein(t.text);
                const std::string such = klein(t.suchen);
                std::size_t fund = std::string::npos;
                if (getippt) {   // beim Tippen ab dem Anfang der Auswahl
                    fund = text.find(such, t.cursor);
                } else if (weiter && rueck) {
                    fund = t.cursor > 0 ? text.rfind(such, t.cursor - 1) : std::string::npos;
                } else {
                    fund = text.find(such, t.auswahlEnde > t.cursor ? t.cursor + 1 : t.cursor);
                }
                if (fund != std::string::npos) {
                    t.cursor = fund;
                    t.auswahlEnde = fund + such.size();
                    t.springen = static_cast<int>(std::count(t.text.begin(), t.text.begin() + static_cast<std::ptrdiff_t>(fund), '\n'));
                    t.springenBilder = 1;
                }
            }
        }
        ImGui::End();
        if (!offen) {
            if (t.editierbar && t.geaendert) {   // ensure_saved
                t.frageSchliessen = true;
            } else {
                t.offen = false;
            }
        }
        if (t.frageSchliessen) {
            ImGui::OpenPopup("###shadertextfrage");
            t.frageSchliessen = false;
        }
        const std::string frage = std::string(trK(Str::KeSave)) + "###shadertextfrage";
        if (ImGui::BeginPopupModal(frage.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(tr(Str::KeSeModified));
            if (ImGui::Button(trK(Str::KeYes))) {
                std::ofstream f(pfadAus(t.pfad), std::ios::binary);
                if (f) { f << t.text; }
                t.geaendert = false;
                t.offen = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(trK(Str::KeNo))) {   // discard changes
                t.geaendert = false;
                t.offen = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(trK(Str::KeCancel))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    }
}

std::string texturTitel() {   // TextureBrowser_exportTitle
    const Editor& ed = e();
    char z2[512];
    std::snprintf(z2, sizeof(z2), trK(Str::KeTexTitle), ed.texBro.verzeichnis.empty() ? trK(Str::KeTexAll) : ed.texBro.verzeichnis.c_str());
    return z2;
}

}  // namespace bhed::gui::karteneditor
