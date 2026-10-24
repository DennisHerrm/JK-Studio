// app_karte_csg.cpp - Karten-Editor: der Dialog "CSG Tool".
//
// Portiert aus NetRadiant-Custom 20260114: radiant/csg.cpp:1372-1588
// (CSGToolDialog, CSG_SpinBoxLabel, CSGdlg_getSettings, CSG_Hollow,
// CSGdlg_BrushShrink, CSGdlg_BrushExpand, CSG_Tool) und
// libs/gtkutil/spinbox.h (QSpinBox_mod: Rad mit Umschalt/Strg;
// SpinBoxLabel: waagerecht ziehen).
//
// Masse am Foto dlg_Brush_CSG_CSG_Tool_0.png (doppelte Groesse, Innenraum
// 902 x 206 Bildpunkte = 451 x 103 logische Punkte): QGridLayout 3 x 8 mit
// Rand 11 und Abstand 6, SetFixedSize. Zeile 0 (y 11, 29 hoch): "Grid->"
// (Spalte 0, 37 breit), DoubleSpinBox 73 x 20 bei x 54, die vier
// Radioknoepfe bei 133, 193, 257, 312 (Kreis 14), die zwei pruefbaren
// QToolButtons 29 x 29 (Symbol PM_ToolBarIconSize 24) bei 368 und 411;
// Zeile 1: die Linie (QFrame::HLine, Raised) bei y 46; Zeile 2 (y 55):
// sechs QToolButtons 37 x 37 (Symbol PM_LargeIconSize 32) in den Spalten
// 0, 1, 3, 4, 5, 6 (x 11, 54, 193, 257, 312, 368).
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
#include "bhed/karte/csg.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace bhed::gui::karteneditor {

namespace {

float g_sk = 1.0F;
ImVec2 g_innen{0.0F, 0.0F};

ImVec2 bp(float x, float y) { return ImVec2{std::round(g_innen.x + x * g_sk), std::round(g_innen.y + y * g_sk)}; }
float px(float l) { return std::round(l * g_sk); }

constexpr float kInnenB = 451.0F;
constexpr float kInnenH = 103.0F;
constexpr float kZeile0 = 11.0F;
constexpr float kZeile0H = 29.0F;
constexpr float kZeile2 = 55.0F;
constexpr float kGross = 37.0F;    // QToolButton mit PM_LargeIconSize
constexpr float kKlein = 29.0F;    // QToolButton mit PM_ToolBarIconSize

// QAbstractSpinBox::stepBy fuer QDoubleSpinBox: Wert + n * Schritt, in den
// Bereich 0 .. 9999 (kein Umlauf), gerundet auf 3 Stellen (QDoubleSpinBoxPrivate::round).
void schrittWeiter(Editor::CsgWerkzeug& w, double schritt, int n) {
    const double v = std::clamp(w.dicke + schritt * static_cast<double>(n), 0.0, 9999.0);
    w.dicke = std::round(v * 1000.0) / 1000.0;
}

// CSGdlg_getSettings (csg.cpp:1416-1436)
karte::HohlEinstellung einstellung() {
    const Editor& ed = editor();
    const Editor::CsgWerkzeug& w = ed.csgWerkzeug;
    karte::HohlEinstellung s;
    s.versatz = static_cast<float>(w.dicke);
    if (w.ausschluss == 2) {   // radProj: g_vector3_axes[GlobalXYWnd_getCurrentViewType()]
        s.nachAchse = true;
        const int typ = static_cast<int>(ed.xy[static_cast<std::size_t>(ed.aktivXy)].ansicht.typ);
        s.achse = karte::Vektor3f(typ == 0 ? 1.0F : 0.0F, typ == 1 ? 1.0F : 0.0F, typ == 2 ? 1.0F : 0.0F);
    } else if (w.ausschluss == 3) {   // radCam: Camera_getViewVector (camera.vpn)
        s.nachAchse = true;
        s.achse = ed.kamera.vpn;
    } else {   // either + or - faces
        s.nachAchse = false;
        s.gewaehlteAusschliessen = w.ausschluss == 0;
    }
    s.caulk = w.caulk;
    s.innenEntfernen = w.innenWeg;
    return s;
}

// CSG_Hollow (csg.cpp:1438-1447)
void aushoehlen(karte::HohlArt art, const char* rueckgaengig) {
    karte::HohlEinstellung s = einstellung();
    s.art = art;
    karte::csgAushoehlen(editor().karte, csgUmgebung(), s, rueckgaengig);
    auswahlGeaendert();
}

// Ein QToolButton: Flaeche im Knopfstil des Programms, das Symbol mittig.
// Pruefbare (setCheckable) zeigen "an" in der Farbe gedrueckter Knoepfe.
bool werkzeugKnopf(const char* id, const char* bild, float x, float y, float groesse, float symbol, Str tipp, bool* an = nullptr) {
    const ImVec2 a = bp(x, y);
    const ImVec2 b = bp(x + groesse, y + groesse);
    ImGui::SetCursorScreenPos(a);
    const bool geklickt = ImGui::InvisibleButton(id, ImVec2{b.x - a.x, b.y - a.y});
    const bool ueber = ImGui::IsItemHovered();
    const ImGuiStyle& st = ImGui::GetStyle();
    ImVec4 grund = st.Colors[ImGuiCol_Button];
    if (an != nullptr && *an) {
        grund = st.Colors[ImGuiCol_ButtonActive];
        grund.w = std::max(grund.w, 0.85F);
    }
    if (ueber) { grund = st.Colors[ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered]; }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(a, b, ImGui::GetColorU32(grund), st.FrameRounding + 1.0F);
    if (an != nullptr && *an) { dl->AddRect(a, b, ImGui::GetColorU32(ImGuiCol_CheckMark), st.FrameRounding + 1.0F, 0, std::max(1.0F, g_sk)); }
    const float g = px(symbol);
    (void)karteIcon(dl, bild, ImVec2{std::floor((a.x + b.x - g) * 0.5F), std::floor((a.y + b.y - g) * 0.5F)}, g,
                    ImGui::GetColorU32(karteIconFarbe(bild)));
    if (ueber) { ImGui::SetTooltip("%s", trK(tipp)); }
    if (geklickt && an != nullptr) { *an = !*an; }
    return geklickt;
}

// QRadioButton (Fusion): Kreis 14 senkrecht mittig in der Zeile, Text 6 dahinter.
void radio(const char* id, Str text, Str tipp, int eigener, float x) {
    Editor::CsgWerkzeug& w = editor().csgWerkzeug;
    const float r = px(14.0F);
    ImGui::SetCursorScreenPos(ImVec2{bp(x, 0.0F).x, std::floor(bp(0.0F, kZeile0).y + (px(kZeile0H) - r) * 0.5F)});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (r - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2{px(6.0F), 0.0F});
    ImGui::RadioButton((std::string(trK(text)) + id).c_str(), &w.ausschluss, eigener);
    ImGui::PopStyleVar(2);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", trK(tipp)); }
}

// DoubleSpinBox (gtkutil/spinbox.h:31-90) mit dem CSG_SpinBoxLabel davor
// (csg.cpp:1453-1465, SpinBoxLabel spinbox.h:96-150).
void dickeFeld() {
    Editor& ed = editor();
    Editor::CsgWerkzeug& w = ed.csgWerkzeug;
    ImGuiIO& io = ImGui::GetIO();
    static bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
    static ImVec2 zugStart;
    static float summe = 0.0F;
    static bool gezogen = false;   // m_dragOccured
    bool fokus = false;
    // "Grid->": die ganze Zelle (QLabel in Spalte 0, AlignVCenter)
    const char* gitter = trK(Str::KeCsgGrid);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddText(ImVec2{bp(11.0F, 0.0F).x, std::floor(bp(0.0F, kZeile0).y + (px(kZeile0H) - ImGui::GetTextLineHeight()) * 0.5F)},
                ImGui::GetColorU32(ImGuiCol_Text), gitter);
    ImGui::SetCursorScreenPos(bp(11.0F, kZeile0));
    const ImVec2 zelle{px(37.0F), px(kZeile0H)};
    ImGui::InvisibleButton("##csggitter", zelle);
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
        ImGui::SetMouseCursor(ImGui::IsItemActive() ? ImGuiMouseCursor_None : ImGuiMouseCursor_ResizeEW);   // BlankCursor / SizeHorCursor
    }
    if (ImGui::IsItemActivated()) {   // mousePressEvent
        zugStart = io.MousePos;
        summe = 0.0F;
        gezogen = false;
    } else if (ImGui::IsItemActive() && (io.MouseDelta.x != 0.0F || io.MouseDelta.y != 0.0F)) {   // mouseMoveEvent
        const float qt = std::max(1.0F, std::round(g_app->dpiScale));
        summe += (io.MousePos.x - zugStart.x) / qt;   // m_dragAccum += pos.x - m_dragStart.x
        const int delta = static_cast<int>(summe / 20.0F);
        if (delta != 0) {
            gezogen = true;
            summe -= static_cast<float>(delta) * 20.0F;   // m_dragAccum %= 20
            if (io.KeyShift) {   // substep: Strg+Umschalt 1/100, Umschalt 1/10
                schrittWeiter(w, io.KeyCtrl ? w.schritt / 100.0 : w.schritt / 10.0, delta);
            } else {
                schrittWeiter(w, w.schritt, io.KeyCtrl ? delta * 10 : delta);
            }
            if (!imTest) { ImGui::TeleportMousePos(zugStart); }   // QCursor::setPos( m_dragStart )
        }
    }
    if (ImGui::IsItemDeactivated()) {   // mouseReleaseEvent
        if (!gezogen) {   // CSG_SpinBoxLabel: ohne Ziehen das Raster als Wert und Schritt
            w.dicke = std::round(static_cast<double>(ed.raster.groesse()) * 1000.0) / 1000.0;
            w.schritt = static_cast<double>(ed.raster.groesse());
        }
        fokus = true;   // m_spin->setFocus(); selectAll()
    }
    // Das Feld: Text 73 - 16 breit, rechts die Pfeile uebereinander (Fusion)
    const float feldH = px(20.0F);
    const float pfeilB = px(16.0F);
    const ImVec2 fa = bp(54.0F, 15.0F);
    ImGui::SetCursorScreenPos(fa);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::SetNextItemWidth(px(73.0F) - pfeilB);
    if (fokus) { ImGui::SetKeyboardFocusHere(); }
    double wert = w.dicke;
    if (ImGui::InputDouble("##csgdicke", &wert, 0.0, 0.0, "%.3f")) { w.dicke = std::round(std::clamp(wert, 0.0, 9999.0) * 1000.0) / 1000.0; }
    ImGui::PopStyleVar();
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", trK(Str::KeCsgThickness)); }
    const bool feldAktiv = ImGui::IsItemActive();
    const bool feldUeber = ImGui::IsItemHovered();
    if (feldAktiv) {   // Pfeiltasten und Bild auf/ab (QAbstractSpinBox::keyPressEvent)
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { schrittWeiter(w, w.schritt, 1); }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { schrittWeiter(w, w.schritt, -1); }
        if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) { schrittWeiter(w, w.schritt, 10); }
        if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) { schrittWeiter(w, w.schritt, -10); }
    }
    // Pfeile: je halbe Hoehe, gedrueckt halten wiederholt (setAccelerated)
    ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
    for (int i = 0; i < 2; ++i) {
        const ImVec2 a{fa.x + px(73.0F) - pfeilB, fa.y + (i == 0 ? 0.0F : std::floor(feldH * 0.5F))};
        const ImVec2 b{fa.x + px(73.0F), i == 0 ? fa.y + std::floor(feldH * 0.5F) : fa.y + feldH};
        ImGui::SetCursorScreenPos(a);
        if (ImGui::InvisibleButton(i == 0 ? "##csgdickehoch" : "##csgdickerunter", ImVec2{b.x - a.x, b.y - a.y})) {
            schrittWeiter(w, w.schritt, i == 0 ? 1 : -1);
        }
        const bool u = ImGui::IsItemHovered();
        dl->AddRectFilled(a, b, ImGui::GetColorU32(u ? (ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered) : ImGuiCol_FrameBg));
        const float m = (a.x + b.x) * 0.5F;
        const float h = std::max(2.0F, px(2.5F));
        const float c = (a.y + b.y) * 0.5F;
        const ImU32 f = ImGui::GetColorU32(ImGuiCol_Text);
        if (i == 0) {
            dl->AddTriangleFilled(ImVec2{m - h * 1.4F, c + h * 0.6F}, ImVec2{m + h * 1.4F, c + h * 0.6F}, ImVec2{m, c - h * 0.8F}, f);
        } else {
            dl->AddTriangleFilled(ImVec2{m - h * 1.4F, c - h * 0.6F}, ImVec2{m, c + h * 0.8F}, ImVec2{m + h * 1.4F, c - h * 0.6F}, f);
        }
    }
    ImGui::PopItemFlag();
    // Rad (QSpinBox_mod::wheelEvent): Umschalt 1/10, Strg+Umschalt 1/100, Strg zehn Schritte
    if ((feldUeber || ImGui::IsMouseHoveringRect(fa, ImVec2{fa.x + px(73.0F), fa.y + feldH})) && io.MouseWheel != 0.0F) {
        const int n = static_cast<int>(io.MouseWheel);
        if (io.KeyShift) {
            schrittWeiter(w, io.KeyCtrl ? w.schritt / 1000.0 : w.schritt / 10.0, io.KeyCtrl ? n * 10 : n);
        } else {
            schrittWeiter(w, w.schritt, io.KeyCtrl ? n * 10 : n);
        }
    }
}

}  // namespace

// CSG_Tool (csg.cpp:1470-1588)
void zeichneCsgWerkzeug() {
    Editor& ed = editor();
    Editor::CsgWerkzeug& w = ed.csgWerkzeug;
    if (!w.offen) { return; }
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    g_sk = massstab();
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    ImGui::SetNextWindowSize(ImVec2{px(kInnenB), px(kInnenH) + titelH}, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_FirstUseEver,
                            ImVec2{0.5F, 0.5F});
    const std::string titel = std::string(trK(Str::KeCsgTitle)) + "###kartecsgtool";
    bool offen = true;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool sichtbar = ImGui::Begin(titel.c_str(), &offen,
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    if (sichtbar) {
        g_innen = ImGui::GetCursorScreenPos();
        dialogSchriftAn();
        dickeFeld();
        radio("##csgminusfaces", Str::KeCsgMinusFaces, Str::KeCsgTipMinusFaces, 0, 133.0F);
        radio("##csgplusfaces", Str::KeCsgPlusFaces, Str::KeCsgTipPlusFaces, 1, 193.0F);
        radio("##csgminusproj", Str::KeCsgMinusProj, Str::KeCsgTipProj, 2, 257.0F);
        radio("##csgminuscam", Str::KeCsgMinusCam, Str::KeCsgTipCam, 3, 312.0F);
        (void)werkzeugKnopf("##csgcaulk", "f-caulk", 368.0F, kZeile0, kKlein, 24.0F, Str::KeCsgTipCaulk, &w.caulk);
        (void)werkzeugKnopf("##csginnenweg", "csgtool_removeinner", 411.0F, kZeile0, kKlein, 24.0F, Str::KeCsgTipRemoveInner, &w.innenWeg);
        {   // QFrame HLine, Raised: dunkle Linie, darunter die helle (Spalten 0..7)
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 a = bp(11.0F, 46.0F);
            const ImVec2 b = bp(440.0F, 46.0F);
            dl->AddLine(a, b, ImGui::GetColorU32(ImGuiCol_Separator), std::max(1.0F, g_sk));
        }
        if (werkzeugKnopf("##csgschrumpfen", "csgtool_shrink", 11.0F, kZeile2, kGross, 32.0F, Str::KeCsgTipShrink)) {
            karte::csgPinselVersetzen(ed.karte, einstellung(), true);   // CSGdlg_BrushShrink
            auswahlGeaendert();
        }
        if (werkzeugKnopf("##csgerweitern", "csgtool_expand", 54.0F, kZeile2, kGross, 32.0F, Str::KeCsgTipExpand)) {
            karte::csgPinselVersetzen(ed.karte, einstellung(), false);   // CSGdlg_BrushExpand
            auswahlGeaendert();
        }
        if (werkzeugKnopf("##csgdiag", "csgtool_diagonal", 193.0F, kZeile2, kGross, 32.0F, Str::KeCsgTipDiag)) {
            aushoehlen(karte::HohlArt::Diagonal, "brushHollow::Diag");
        }
        if (werkzeugKnopf("##csgwrap", "csgtool_wrap", 257.0F, kZeile2, kGross, 32.0F, Str::KeCsgTipWrap)) {
            aushoehlen(karte::HohlArt::Huelle, "brushHollow::Wrap");
        }
        if (werkzeugKnopf("##csgextrude", "csgtool_extrude", 312.0F, kZeile2, kGross, 32.0F, Str::KeCsgTipExtrude)) {
            aushoehlen(karte::HohlArt::Extrudieren, "brushHollow::Extrude");
        }
        if (werkzeugKnopf("##csgpull", "csgtool_pull", 368.0F, kZeile2, kGross, 32.0F, Str::KeCsgTipPull)) {
            aushoehlen(karte::HohlArt::Ziehen, "brushHollow::Pull");
        }
        dialogSchriftAus();
        ImGui::SetCursorScreenPos(bp(0.0F, kInnenH));
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
    }
    ImGui::End();
    if (!offen) { w.offen = false; }
}

}  // namespace bhed::gui::karteneditor
