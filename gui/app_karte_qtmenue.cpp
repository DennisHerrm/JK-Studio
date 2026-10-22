// app_karte_qtmenue.cpp - Karten-Editor: Menues wie QMenu in NetRadiant-Custom.
//
// NRC 20260114 zeichnet seine Menues mit Qt 5.15 (Stil Fusion, dazu
// themes/dark.qss). Hier dieselben Masse und Lagen in logischen Punkten
// (mal massstab() = Bildpunkte), die Farben aus dem Farbschema des
// Programms:
//   Zeile         fm.height() + 8 = 24              qcommonstyle.cpp:4960 (CT_MenuItem)
//   Trenner       1 + margin 3 oben, 3 unten = 7    dark.qss "QMenu::separator",
//                 Linie von 5 bis Breite - 7        qstylesheetstyle.cpp:5178 (size())
//   Abreisskante  10 (PM_MenuTearoffHeight), oben   qmenu.cpp:385, qcommonstyle.cpp:1487
//                 (g_Layout_enableDetachableMenus, mainframe.cpp:863: an)
//   Breite        updateActionRects (qmenu.cpp:358-470): je Eintrag
//                 QCommonStyle (qcommonstyle.cpp:4951-4977) und QFusionStyle
//                 (qfusionstyle.cpp:3198-3229) auf die Textbreite, das
//                 Groesste plus die breiteste Taste (tabWidth)
//   Eintrag       QFusionStyle CE_MenuItem (qfusionstyle.cpp:1548-1763):
//                 Haken/Punkt in (8, 5, 14, 14), Symbol 16 x 16 mittig in
//                 der Spalte (4, 0, 21, 24), Text ab 28, die Taste ab
//                 Breite - 15 - tabWidth, der Pfeil (Breite - 13, 7, 10, 10)
//   Mnemonik      unterstrichen (SH_UnderlineShortcut in Fusion immer an),
//                 Buchstabe waehlt (QMenu::keyPressEvent, qmenu.cpp:3460-3530)
// Untermenues gehen rechts buendig auf (PM_SubMenuOverlap 0), ihr erster
// Eintrag auf der Hoehe des oeffnenden (qmenu.cpp popup: -actionGeometry(first).top()).
//
// ImGui macht Auswahl, Hervorhebung, Tastatur und das Oeffnen; der eigene
// Text der ImGui-Eintraege bleibt unsichtbar (die Beschriftung bleibt fuer
// den Selbsttest), gezeichnet wird nach Qts Lagen.
//
// Portiert aus Qt 5.15 (qtbase: src/widgets/widgets/qmenu.cpp,
// src/widgets/styles/qfusionstyle.cpp, qcommonstyle.cpp, qstylesheetstyle.cpp),
// Copyright (C) The Qt Company Ltd., unter der GNU General Public License
// Version 2 oder spaeter (Wahl der qtbase-Lizenz), und NetRadiant-Custom
// 20260114 (themes/dark.qss, radiant/theme.cpp).
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
#include "platform.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <map>
#include <unordered_map>
#include <vector>

namespace bhed::gui::karteneditor {

namespace {

// Logische Punkte (Quellen oben)
constexpr int kZeileZusatz = 8;      // CT_MenuItem: fm.height() + 8
constexpr int kTrennerH = 7;         // dark.qss: height 1 + margin 3 + 3
constexpr int kTrennerOben = 3;
constexpr int kTrennerLinks = 5;
constexpr int kTrennerRechts = 7;
constexpr int kAbriss = 10;          // PM_MenuTearoffHeight
constexpr int kSymbol = 16;          // PM_SmallIconSize
constexpr int kSpalteX = 4;          // checkColHOffset = windowsItemHMargin 3 + windowsItemFrame 2 - 1
constexpr int kSpalteB = 21;         // checkcol = max(24 * 0.79, max(maxIconWidth, 21))
constexpr int kTextX = 28;           // xm = checkColHOffset + checkcol + windowsItemHMargin
constexpr int kRechterRand = 15;     // windowsRightBorder

struct Masse {
    float sk = 1.0F;
    float lh = 16.0F;       // Zeilenhoehe des Textes (Bildpunkte)
    float zeile = 24.0F;    // Hoehe eines Eintrags
    float abstand = 8.0F;   // ItemSpacing.y = zeile - lh
    float trenner = 7.0F;
    float abriss = 10.0F;
};

Masse masse() {
    Masse m;
    m.sk = massstab();
    m.lh = ImGui::GetTextLineHeight();
    m.zeile = std::round(static_cast<float>(mass::kSchrift + kZeileZusatz) * m.sk);
    m.abstand = std::max(0.0F, m.zeile - m.lh);
    m.trenner = std::round(static_cast<float>(kTrennerH) * m.sk);
    m.abriss = std::round(static_cast<float>(kAbriss) * m.sk);
    return m;
}

float px(const Masse& m, float logisch) { return std::round(logisch * m.sk); }

// Textbreite in logischen Punkten (fm.boundingRect / horizontalAdvance): Qt 5
// misst mit der GDI-Schrift (QWindowsFontEngine) in logischen Punkten -
// Segoe UI 9 pt = 12 px, gehintet. Am Foto: Help > General ist 335 breit =
// "Complete list of Q3map2 command line parameters" (GDI 274) + 61; ImGuis
// ungehintete Breite gaebe 275, je Zeichen gerundet 268.
int qtBreite(const std::string& text, float sk) {
    static std::unordered_map<std::string, int> gemerkt;
    const auto it = gemerkt.find(text);
    if (it != gemerkt.end()) { return it->second; }
    int b = platform::gdiTextBreite(text, L"Segoe UI", mass::kSchrift * 3 / 4, false);
    if (b < 0) { b = static_cast<int>(std::lround(ImGui::CalcTextSize(text.c_str()).x / sk)); }
    gemerkt.emplace(text, b);
    return b;
}

struct Roh {
    int text = 0;
    bool tab = false;     // mit Taste (opt.text enthaelt '\t')
    bool unter = false;   // QStyleOptionMenuItem::SubMenu
};
struct Taste {
    unsigned zeichen = 0;
    ImGuiID eintrag = 0;   // Kennung des ImGui-Eintrags (Hervorhebung)
    ImGuiID menue = 0;     // bei Untermenues: die Kennung des Popups
    bool aktiv = true;
};
struct Menue {
    ImGuiID id = 0;
    bool abriss = true;
    std::vector<Roh> roh;
    int tab = 0;            // tabWidth
    bool symbole = false;   // maxIconWidth = 16 + 4
    bool pruefbar = false;  // hasCheckableItems
    int zeilen = 0;
    int trenner = 0;
    std::vector<Taste> tasten;
};
struct Groesse {
    float b = 0.0F;
    float h = 0.0F;
    int tab = 0;
};

std::vector<Menue> g_stapel;
std::map<ImGuiID, Groesse> g_groessen;   // aus dem letzten Bild (die Breite haengt an allen Eintraegen)
ImGuiID g_wahl = 0;                      // per Mnemonik ausgeloest (im naechsten Bild)
int g_wurzelStapel = 0;                  // OpenPopupStack am Ende des letzten Bildes (offene Menueebenen)
int g_wurzelBild = -1;
ImGuiID g_oeffnen = 0;                   // per Mnemonik geoeffnetes Untermenue (im naechsten Bild)

Menue* oben() { return g_stapel.empty() ? nullptr : &g_stapel.back(); }

// Der sichtbare Teil einer ImGui-Beschriftung (bis "##").
std::string sichtbar(const char* id) {
    const char* ende = ImGui::FindRenderedTextEnd(id);
    return std::string(id, ende);
}

// qt_findMnemonic (qkeysequence.cpp): das Zeichen nach dem ersten '&', das
// nicht zu "&&" gehoert - als Lage im Text ohne '&'; -1 ohne.
int mnemonikLage(const char* mitUnd) {
    if (mitUnd == nullptr) { return -1; }
    int lage = 0;
    for (const char* c = mitUnd; *c != 0; ++c) {
        if (*c == '&' && c[1] != 0) {
            if (c[1] == '&') {
                ++c;
                ++lage;
                continue;
            }
            return lage;
        }
        ++lage;
    }
    return -1;
}

// Die Mnemonik gilt nur, wenn der Text NRCs Wortlaut ist (sonst uebersetzt).
int mnemonik(const char* nrc, const std::string& text) {
    if (nrc == nullptr || karte::ohneMnemonik(nrc) != text) { return -1; }
    const int l = mnemonikLage(nrc);
    return l >= 0 && l < static_cast<int>(text.size()) ? l : -1;
}

unsigned zeichenGross(char c) { return static_cast<unsigned>(std::toupper(static_cast<unsigned char>(c))); }

// Der Pfeil eines Untermenues: qt_fusion_draw_arrow (qfusionstyle.cpp:157) in
// (Breite - 13, 7, 10, 10): 8 x 14 auf 10 begrenzt, quer 4 x 8, mittig;
// WindowText mit Alpha 160 (PE_IndicatorArrowRight).
void pfeil(ImDrawList* dl, const ImRect& r, const Masse& m, bool aktiv) {
    const float x = r.Max.x - px(m, 13.0F) + px(m, 3.0F);
    const float y = r.Min.y + px(m, 7.0F) + px(m, 1.0F);
    ImVec4 f = ImGui::GetStyleColorVec4(aktiv ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    f.w *= 160.0F / 255.0F;
    dl->AddTriangleFilled(ImVec2{x, y}, ImVec2{x, y + px(m, 8.0F)}, ImVec2{x + px(m, 4.0F), y + px(m, 4.0F)}, ImGui::GetColorU32(f));
}

// Ein Eintrag nach CE_MenuItem; r = Qts Rechteck des Eintrags (Bildpunkte).
void eintragZeichnen(ImDrawList* dl, const ImRect& r, const Masse& m, const std::string& text, int mn, const char* taste,
                     int tab, const QtPunkt* p, bool unter) {
    const bool aktiv = p != nullptr ? p->aktiv : true;
    const ImU32 schrift = ImGui::GetColorU32(aktiv ? ImGuiCol_Text : ImGuiCol_TextDisabled);
    const float y0 = r.Min.y;
    if (p != nullptr) {
        const bool symbol = (p->bild != nullptr && p->bild[0] != 0) || p->farbe != nullptr;
        if (symbol) {
            // Symbol 16 x 16 mittig in (4, 0, 21, 24): moveCenter -> (7, 4)
            const ImVec2 a{r.Min.x + px(m, 7.0F), y0 + px(m, 4.0F)};
            const float g = px(m, static_cast<float>(kSymbol));
            if (p->art != QtArt::Normal && p->haken) {
                // checkable und checked: PE_PanelButtonCommand eingedrueckt um die Spalte
                const ImVec2 s0{r.Min.x + px(m, static_cast<float>(kSpalteX)), y0};
                const ImVec2 s1{s0.x + px(m, static_cast<float>(kSpalteB)), r.Max.y};
                dl->AddRectFilled(s0, s1, ImGui::GetColorU32(ImGuiCol_ButtonActive), ImGui::GetStyle().FrameRounding);
                dl->AddRect(s0, s1, ImGui::GetColorU32(ImGuiCol_Border), ImGui::GetStyle().FrameRounding);
            }
            if (p->farbe != nullptr) {
                dl->AddRectFilled(a, ImVec2{a.x + g, a.y + g},
                                  ImGui::GetColorU32(ImVec4{p->farbe[0], p->farbe[1], p->farbe[2], 1.0F}));
            } else {
                const ImVec4 f = aktiv ? karteIconFarbe(p->bild) : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
                (void)karteIcon(dl, p->bild, a, g, ImGui::GetColorU32(f));
            }
        } else if (p->art == QtArt::Pruefbar) {
            // PE_IndicatorCheckBox in (8, 5, 14, 14) - Kaestchen im Stil des Programms
            const ImVec2 a{r.Min.x + px(m, 8.0F), y0 + px(m, 5.0F)};
            const float g = px(m, 14.0F);
            const float rund = ImGui::GetStyle().FrameRounding;
            dl->AddRectFilled(a, ImVec2{a.x + g, a.y + g}, ImGui::GetColorU32(ImGuiCol_FrameBg), rund);
            dl->AddRect(a, ImVec2{a.x + g, a.y + g}, ImGui::GetColorU32(aktiv ? ImGuiCol_Border : ImGuiCol_TextDisabled), rund);
            if (p->haken) {
                ImGui::RenderCheckMark(dl, ImVec2{a.x + g * 0.15F, a.y + g * 0.15F},
                                       ImGui::GetColorU32(aktiv ? ImGuiCol_CheckMark : ImGuiCol_TextDisabled), g * 0.7F);
            }
        } else if (p->art == QtArt::Exklusiv && p->haken) {
            // Radio: drawEllipse( (8, 5, 14, 14).adjusted(4, 4, -4, -4) ) -> Mitte (15, 12), Radius 3
            dl->AddCircleFilled(ImVec2{r.Min.x + px(m, 15.0F), y0 + px(m, 12.0F)}, px(m, 3.0F), schrift);
        }
    }
    // Text ab xm = 28, senkrecht mittig (AlignVCenter)
    const ImVec2 t{r.Min.x + px(m, static_cast<float>(kTextX)), std::floor(y0 + (r.GetHeight() - m.lh) * 0.5F)};
    dl->AddText(t, schrift, text.c_str());
    qtUnterstreichen(dl, t, text, mn, schrift);
    if (taste != nullptr && taste[0] != 0) {
        // vShortcutRect: ab textRect.topRight() = Breite - 15 - tabWidth, linksbuendig
        const float x = r.Max.x - px(m, static_cast<float>(kRechterRand + tab));
        dl->AddText(ImVec2{std::round(x), t.y}, schrift, taste);
    }
    if (unter) { pfeil(dl, r, m, aktiv); }
}

// Die Abreisskante (CE_MenuTearoff, qcommonstyle.cpp:1487-1499): Flaeche in
// der Knopffarbe, zwei gestrichelte Linien (dark, light) auf h/2 - 1 und
// h/2 von x + 2 bis Breite - 4. Strich 4, Luecke 2 (Qt::DashLine) mit
// SquareCap: 5 an, 1 aus. Das Abreissen selbst (QTornOffMenu) kommt mit der
// Einstellung "Detachable Menus" in Stufe 2.
void abrissZeichnen(const Masse& m) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    ImDrawList* dl = w->DrawList;
    const ImVec2 a = w->Pos;
    const ImVec2 b{w->Pos.x + w->Size.x, w->Pos.y + m.abriss};
    dl->AddRectFilled(a, b, ImGui::GetColorU32(ImGuiCol_Button));
    const float d = std::max(1.0F, std::round(m.sk));
    const float y = a.y + std::round((static_cast<float>(kAbriss) * 0.5F - 1.5F) * m.sk);
    const float x0 = a.x + std::round(1.5F * m.sk);
    const float x1 = b.x - std::round(3.5F * m.sk);
    const float an = px(m, 5.0F);
    const float aus = std::max(1.0F, px(m, 1.0F));
    const ImU32 dunkel = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    const ImU32 hell = ImGui::GetColorU32(ImGuiCol_Text);
    for (float x = x0; x < x1; x += an + aus) {
        const float xe = std::min(x + an, x1);
        dl->AddRectFilled(ImVec2{x, y}, ImVec2{xe, y + d}, dunkel);
        dl->AddRectFilled(ImVec2{x, y + d}, ImVec2{xe, y + 2.0F * d}, hell);
    }
    if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(a, b)) {
        char z[160];
        std::snprintf(z, sizeof(z), tr(Str::KeStufe), 2);
        ImGui::SetTooltip("%s\n%s", trK(Str::KePrDetachableMenus), z);
    }
}

void groesseSetzen(ImGuiID key, const Masse& m, bool abriss) {
    const auto it = g_groessen.find(key);
    if (it == g_groessen.end()) { return; }
    const float rand = (abriss ? m.abriss : 0.0F) + m.abstand * 0.5F;
    ImGui::SetNextWindowSize(ImVec2{it->second.b, it->second.h});
    ImGui::SetNextWindowContentSize(ImVec2{0.0F, std::max(0.0F, it->second.h - 2.0F * rand)});
}

// QMenu::keyPressEvent Key_Escape: Escape schliesst das innerste Menue, das
// vor diesem Bild offen war (ImGui nimmt Escape teils selbst - schliesst ein
// Untermenue oder nur ClearActiveID); das Menue hat die Taste, kein Kuerzel.
void escapeAmAnfang() {
    ImGuiContext& g = *GImGui;
    if (!ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { return; }
    const int ebene = g.BeginPopupStack.Size - 1;   // die Wurzel im Stapel der offenen Popups
    const int vorher = g_wurzelBild == ImGui::GetFrameCount() - 1 ? std::max(g_wurzelStapel, ebene + 1) : ebene + 1;
    editor().tasteVerbrauchtBild = ImGui::GetFrameCount();
    if (vorher <= ebene + 1) {
        ImGui::CloseCurrentPopup();
    } else if (g.OpenPopupStack.Size > vorher - 1) {
        ImGui::ClosePopupToLevel(vorher - 1, true);
    }
}

void menueBeginnen(ImGuiID key, bool abriss, const Masse& m) {
    if (g_stapel.empty()) { escapeAmAnfang(); }
    Menue neu;
    neu.id = key;
    neu.abriss = abriss;
    g_stapel.push_back(neu);
    // Waagerecht kein Abstand: die Zeile reicht genau von Rand zu Rand (Qt:
    // PM_MenuHMargin 0), senkrecht der Rest der Qt-Zeile ueber dem Text.
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, m.abstand});
    if (abriss) { abrissZeichnen(m); }
}

// QMenu::keyPressEvent (qmenu.cpp:3460-3530): ein Buchstabe ohne Strg waehlt
// den Eintrag mit dieser Mnemonik; gibt es mehrere, wandert die Hervorhebung
// zum naechsten; ein Untermenue geht auf. Sonst mit Alt die Menueleiste.
void tastenAuswerten(const Menue& mn) {
    ImGuiContext& g = *GImGui;
    if (g.OpenPopupStack.Size == 0 || g.OpenPopupStack.back().Window != g.CurrentWindow) { return; }
    const ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl || io.KeySuper) { return; }
    unsigned z = 0;
    for (int k = ImGuiKey_A; k <= ImGuiKey_Z; ++k) {
        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(k), false)) { z = 'A' + static_cast<unsigned>(k - ImGuiKey_A); }
    }
    for (int k = ImGuiKey_0; k <= ImGuiKey_9; ++k) {
        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(k), false)) { z = '0' + static_cast<unsigned>(k - ImGuiKey_0); }
    }
    if (z == 0) { return; }
    int treffer = 0;
    const Taste* erster = nullptr;
    const Taste* nachAktuell = nullptr;
    bool aktuell = false;
    for (const Taste& t : mn.tasten) {
        if (t.zeichen != z) { continue; }
        ++treffer;
        if (erster == nullptr) { erster = &t; }
        if (t.eintrag == g.NavId) {
            aktuell = true;
        } else if (nachAktuell == nullptr && aktuell) {
            nachAktuell = &t;
        }
    }
    if (treffer == 0) {
        if (io.KeyAlt) { (void)leistenMnemonik(z); }
        return;
    }
    const Taste* naechster = (treffer == 1 || !aktuell || nachAktuell == nullptr) ? erster : nachAktuell;
    ImGui::SetFocusID(naechster->eintrag, g.CurrentWindow);   // setCurrentAction
    g.NavCursorVisible = true;
    g.NavHighlightItemUnderNav = true;
    if (treffer == 1) {
        if (naechster->menue != 0) {
            g_oeffnen = naechster->menue;
        } else if (naechster->aktiv) {
            g_wahl = naechster->eintrag;   // activateAction( Trigger )
        }
    }
}

void menueAbschliessen() {
    if (g_stapel.empty()) { return; }
    const Menue mn = g_stapel.back();
    g_stapel.pop_back();
    ImGui::PopStyleVar();   // ItemSpacing
    const Masse m = masse();
    // updateActionRects: sizeFromContents(CT_MenuItem) je Eintrag
    const int maxpmw = mn.symbole ? kSymbol + 4 : 0;
    int breite = 0;
    for (const Roh& r : mn.roh) {
        int w = r.text;
        // QCommonStyle
        if (r.tab) { w += 12; }
        if (maxpmw > 0) { w += maxpmw + 6; }
        if (mn.pruefbar && maxpmw < 20) { w += 20 - maxpmw; }
        if (mn.pruefbar || maxpmw > 0) { w += 2; }
        w += 12;
        // QFusionStyle
        if (r.tab) {
            w += 20;   // tabSpacing
        } else if (r.unter) {
            w += 2 * 6;   // menuArrowHMargin
        }
        w += std::max(maxpmw, 12);   // checkcol (menuCheckMarkWidth)
        w += 15 + 10;                // menuRightBorder + 10
        w += 12;
        w = std::max(w, 120);
        breite = std::max(breite, w);
    }
    breite += mn.tab;   // max_column_width += tabWidth
    Groesse gr;
    gr.b = px(m, static_cast<float>(breite));
    gr.h = (mn.abriss ? m.abriss : 0.0F) + static_cast<float>(mn.zeilen) * m.zeile + static_cast<float>(mn.trenner) * m.trenner;
    gr.tab = mn.tab;
    g_groessen[mn.id] = gr;
    tastenAuswerten(mn);
    if (g_stapel.empty()) {
        g_wurzelStapel = GImGui->OpenPopupStack.Size;
        g_wurzelBild = ImGui::GetFrameCount();
    }
}

int tabWidth() {
    const Menue* mn = oben();
    if (mn == nullptr) { return 0; }
    const auto it = g_groessen.find(mn->id);
    return it != g_groessen.end() ? it->second.tab : 0;
}

}  // namespace

void qtUnterstreichen(ImDrawList* dl, ImVec2 textPos, const std::string& text, int lage, unsigned int farbe) {
    if (lage < 0 || lage >= static_cast<int>(text.size())) { return; }
    const float sk = massstab();
    const float x0 = textPos.x + ImGui::CalcTextSize(text.c_str(), text.c_str() + lage).x;
    const float x1 = textPos.x + ImGui::CalcTextSize(text.c_str(), text.c_str() + lage + 1).x;
    // Unterstrich 1 unter der Grundlinie, 1 dick (am Foto: Zeilen 56-57 bei Grundlinie 54)
    const float d = std::max(1.0F, std::round(sk));
    const float y = std::round(textPos.y + ImGui::GetFontBaked()->Ascent) + d;
    dl->AddRectFilled(ImVec2{std::floor(x0), y}, ImVec2{std::floor(x1), y + d}, farbe);
}

int qtMnemonik(const char* nrc, const std::string& text) { return mnemonik(nrc, text); }

bool qtMenueAnfang(const char* id, bool aktiv, const char* nrc, bool abriss) {
    ImGuiWindow* fenster = ImGui::GetCurrentWindow();
    const bool leiste = fenster->DC.LayoutType == ImGuiLayoutType_Horizontal;
    const Masse m = masse();
    const ImGuiID key = fenster->GetID(id);
    if (!leiste && g_oeffnen == key) {
        ImGui::OpenPopup(id);
        g_oeffnen = 0;
    }
    groesseSetzen(key, m, abriss);
    // Die Ruecklage des Popups (BeginMenuEx: popup_pos.y = Text - WindowPadding.y)
    // ist damit die Oberkante des Eintrags minus Abreisskante.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, (abriss ? m.abriss : 0.0F) + m.abstand * 0.5F});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2{0.0F, ImGui::GetStyle().ItemInnerSpacing.y});   // PM_SubMenuOverlap 0
    if (!leiste) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.0F, 0.0F, 0.0F, 0.0F}); }
    ImDrawList* dl = fenster->DrawList;
    const bool offen = ImGui::BeginMenuEx(id, nullptr, aktiv);
    if (!leiste) { ImGui::PopStyleColor(); }
    ImGui::PopStyleVar(2);
    if (!leiste) {
        Menue* eltern = oben();
        const std::string text = sichtbar(id);
        const int mn = mnemonik(nrc, text);
        const ImRect r{ImVec2{fenster->Pos.x, ImGui::GetItemRectMin().y}, ImVec2{fenster->Pos.x + fenster->Size.x, ImGui::GetItemRectMax().y}};
        if (ImGui::IsItemVisible()) {
            QtPunkt p;
            p.aktiv = aktiv;
            eintragZeichnen(dl, r, m, text, mn, nullptr, 0, &p, true);
        }
        if (eltern != nullptr) {
            eltern->roh.push_back({qtBreite(text, m.sk), false, true});
            ++eltern->zeilen;
            if (mn >= 0) { eltern->tasten.push_back({zeichenGross(text[static_cast<std::size_t>(mn)]), ImGui::GetItemID(), key, aktiv}); }
        }
    }
    if (offen) { menueBeginnen(key, abriss, m); }
    return offen;
}

void qtMenueEnde() {
    menueAbschliessen();
    ImGui::EndMenu();
}

bool qtPopupAnfang(const char* id, bool abriss) {
    const Masse m = masse();
    const ImGuiID key = ImGui::GetCurrentWindow()->GetID(id);
    groesseSetzen(key, m, abriss);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, (abriss ? m.abriss : 0.0F) + m.abstand * 0.5F});
    const bool offen = ImGui::BeginPopup(id);
    ImGui::PopStyleVar();
    if (offen) { menueBeginnen(key, abriss, m); }
    return offen;
}

void qtPopupEnde() {
    menueAbschliessen();
    ImGui::EndPopup();
}

bool qtPunkt(const QtPunkt& p) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    const Masse m = masse();
    const std::string text = sichtbar(p.id);
    const int mn = mnemonik(p.nrc, text);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});
    bool gewaehlt = ImGui::MenuItemEx(p.id, nullptr, nullptr, false, p.aktiv);
    ImGui::PopStyleColor();
    const ImGuiID eintrag = ImGui::GetItemID();
    if (g_wahl != 0 && eintrag == g_wahl) {
        g_wahl = 0;
        if (p.aktiv && !gewaehlt) {
            gewaehlt = true;
            ImGui::CloseCurrentPopup();
        }
    }
    const ImRect r{ImVec2{w->Pos.x, ImGui::GetItemRectMin().y}, ImVec2{w->Pos.x + w->Size.x, ImGui::GetItemRectMax().y}};
    if (ImGui::IsItemVisible()) { eintragZeichnen(w->DrawList, r, m, text, mn, p.taste, tabWidth(), &p, false); }
    if (!p.tooltip.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) { ImGui::SetTooltip("%s", p.tooltip.c_str()); }
    if (Menue* menue = oben()) {
        const bool tab = p.taste != nullptr && p.taste[0] != 0;
        menue->roh.push_back({qtBreite(text, m.sk), tab, false});
        if (tab) { menue->tab = std::max(menue->tab, qtBreite(p.taste, m.sk)); }
        if ((p.bild != nullptr && p.bild[0] != 0) || p.farbe != nullptr) { menue->symbole = true; }
        if (p.art != QtArt::Normal) { menue->pruefbar = true; }
        ++menue->zeilen;
        if (mn >= 0) { menue->tasten.push_back({zeichenGross(text[static_cast<std::size_t>(mn)]), eintrag, 0, p.aktiv}); }
    }
    return gewaehlt;
}

void qtTrenner() {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    const Masse m = masse();
    // Qts Rechteck des Trenners beginnt am Ende des vorigen Eintrags.
    const float oben0 = w->DC.CursorPos.y - m.abstand * 0.5F;
    const float d = std::max(1.0F, std::round(m.sk));
    const float y = std::round(oben0 + px(m, static_cast<float>(kTrennerOben)));
    w->DrawList->AddRectFilled(ImVec2{w->Pos.x + px(m, static_cast<float>(kTrennerLinks)), y},
                               ImVec2{w->Pos.x + w->Size.x - px(m, static_cast<float>(kTrennerRechts)), y + d},
                               ImGui::GetColorU32(ImGuiCol_Separator));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + m.trenner - m.abstand);
    ImGui::Dummy(ImVec2{0.0F, 0.0F});
    if (Menue* mn = oben()) { ++mn->trenner; }
}

}  // namespace bhed::gui::karteneditor
