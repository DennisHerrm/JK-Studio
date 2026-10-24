// app_karte_transform.cpp - Karten-Editor: die Dialoge "Arbitrary rotation"
// und "Arbitrary scale".
//
// Portiert aus NetRadiant-Custom 20260114: radiant/select.cpp:1580-1757
// (RotateDialog, ScaleDialog: QWidget mit Qt::Dialog, nicht modal; QGridLayout
// mit SetFixedSize: Spalte 0 SpinBoxLabel "  X  " usw., Spalte 1 drei
// DoubleSpinBox, Spalte 2 eine senkrechte QDialogButtonBox OK, Cancel, Apply;
// eventFilter: Esc = cancel, Return/Enter = ok, Tab und Leertaste
// verschluckt, Schliessen = cancel) und libs/gtkutil/spinbox.h.
//
// Masse an den Fotos dlg_Modify_Arbitrary_rotation_0.png und
// dlg_Modify_Arbitrary_scale_0.png (doppelte Groesse): Rand 11, Abstand 6,
// Zeilen 22 hoch bei y 11, 39, 67; Spinbox bei x 38, 89 (Drehung) bzw.
// 101 (Skalierung) breit; die Knoepfe 80 x 22 dahinter.
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
#include "imgui_internal.h"
#include "platform.h"

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

float textBreite(const char* text) {
    const int b = platform::qtTextBreite(text, L"Tahoma", 11.0F, false);
    return b >= 0 ? static_cast<float>(b) : std::round(ImGui::CalcTextSize(text).x / g_sk);
}

struct Art {
    const char* id;          // ImGui-Kennung
    Str titel;
    double min;
    double max;
    double vorgabe;          // Wert nach Cancel
    bool umlauf;             // setWrapping
    float feldB;             // gemessene Breite der Spinbox
};
const Art kDrehen{"kartedrehen", Str::KeArbRotTitle, -360.0, 360.0, 0.0, true, 89.0F};       // DoubleSpinBox( -360, 360, 0, 6, 1, true )
const Art kSkalieren{"karteskalieren", Str::KeArbScaleTitle, -32768.0, 32768.0, 1.0, false, 101.0F};   // ( -32768, 32768, 1, 6, 1, false )

// QAbstractSpinBoxPrivate::bound (Qt 5.15) fuer stepBy mit und ohne Umlauf
double begrenzt(double v, double alt, int schritte, const Art& a) {
    if (!a.umlauf || schritte == 0) {
        if (v < a.min) { v = a.umlauf ? a.max : a.min; }
        if (v > a.max) { v = a.umlauf ? a.min : a.max; }
        return v;
    }
    const bool warMin = alt == a.min;
    const bool warMax = alt == a.max;
    const bool umgelaufen = (v > alt && schritte < 0) || (v < alt && schritte > 0);
    if (v > a.max) {
        v = ((warMax && !umgelaufen && schritte > 0) || (schritte < 0 && !warMin && umgelaufen)) ? a.min : a.max;
    } else if (umgelaufen && (v > a.max || v < a.min)) {
        v = ((warMax && schritte > 0) || (!warMin && schritte < 0)) ? a.min : a.max;
    } else if (v < a.min) {
        v = (!warMax && !warMin ? a.min : a.max);
    }
    return v;
}

void schritt(double& wert, double groesse, int n, const Art& a) {   // stepBy, 6 Nachkommastellen
    const double neu = begrenzt(wert + groesse * static_cast<double>(n), wert, n, a);
    wert = std::round(neu * 1e6) / 1e6;
}

// Ein Feld mit SpinBoxLabel davor; Zeile y.
void feld(Editor::ArbitraerDialog& d, int i, float y, float labelB, const Art& a) {
    static const char* const namen[3] = {"  X  ", "  Y  ", "  Z  "};
    static ImVec2 zugStart;
    static float summe = 0.0F;
    static const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float feldH = px(22.0F);
    double& wert = d.werte[static_cast<std::size_t>(i)];
    dl->AddText(ImVec2{bp(11.0F, 0.0F).x, std::floor(bp(0.0F, y).y + (feldH - ImGui::GetTextLineHeight()) * 0.5F)}, ImGui::GetColorU32(ImGuiCol_Text),
                namen[i]);
    const std::string zid = std::string("##") + a.id + "zug" + std::to_string(i);
    ImGui::SetCursorScreenPos(bp(11.0F, y));
    ImGui::InvisibleButton(zid.c_str(), ImVec2{px(labelB), feldH});
    bool fokus = false;
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
        ImGui::SetMouseCursor(ImGui::IsItemActive() ? ImGuiMouseCursor_None : ImGuiMouseCursor_ResizeEW);
    }
    if (ImGui::IsItemActivated()) {
        zugStart = io.MousePos;
        summe = 0.0F;
    } else if (ImGui::IsItemActive() && (io.MouseDelta.x != 0.0F || io.MouseDelta.y != 0.0F)) {
        const float qt = std::max(1.0F, std::round(g_app->dpiScale));
        summe += (io.MousePos.x - zugStart.x) / qt;   // m_dragAccum += pos.x - m_dragStart.x
        const int delta = static_cast<int>(summe / 20.0F);
        if (delta != 0) {
            summe -= static_cast<float>(delta) * 20.0F;
            if (io.KeyShift) {   // DoubleSpinBox: Teilschritt 1/10, mit Strg 1/100
                schritt(wert, io.KeyCtrl ? 0.01 : 0.1, delta, a);
            } else {
                schritt(wert, 1.0, io.KeyCtrl ? delta * 10 : delta, a);
            }
            if (!imTest) { ImGui::TeleportMousePos(zugStart); }
        }
    }
    if (ImGui::IsItemDeactivated()) { fokus = true; }
    const float pfeilB = px(16.0F);
    const ImVec2 fa = bp(11.0F + labelB + 6.0F, y);
    ImGui::SetCursorScreenPos(fa);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::SetNextItemWidth(px(a.feldB) - pfeilB);
    if (fokus) { ImGui::SetKeyboardFocusHere(); }
    const std::string fid = std::string("##") + a.id + "wert" + std::to_string(i);
    double v = wert;
    if (ImGui::InputDouble(fid.c_str(), &v, 0.0, 0.0, "%.6f")) { wert = std::round(begrenzt(v, wert, 0, a) * 1e6) / 1e6; }
    ImGui::PopStyleVar();
    if (ImGui::IsItemActive()) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { schritt(wert, 1.0, 1, a); }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { schritt(wert, 1.0, -1, a); }
        if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) { schritt(wert, 1.0, 10, a); }
        if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) { schritt(wert, 1.0, -10, a); }
    }
    ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
    for (int k = 0; k < 2; ++k) {
        const ImVec2 p0{fa.x + px(a.feldB) - pfeilB, fa.y + (k == 0 ? 0.0F : std::floor(feldH * 0.5F))};
        const ImVec2 p1{fa.x + px(a.feldB), k == 0 ? fa.y + std::floor(feldH * 0.5F) : fa.y + feldH};
        ImGui::SetCursorScreenPos(p0);
        const std::string pid = std::string("##") + a.id + (k == 0 ? "hoch" : "runter") + std::to_string(i);
        if (ImGui::InvisibleButton(pid.c_str(), ImVec2{p1.x - p0.x, p1.y - p0.y})) { schritt(wert, 1.0, k == 0 ? 1 : -1, a); }
        const bool u = ImGui::IsItemHovered();
        dl->AddRectFilled(p0, p1, ImGui::GetColorU32(u ? (ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered) : ImGuiCol_FrameBg));
        const float m = (p0.x + p1.x) * 0.5F;
        const float h = std::max(2.0F, px(2.5F));
        const float c = (p0.y + p1.y) * 0.5F;
        const ImU32 f = ImGui::GetColorU32(ImGuiCol_Text);
        if (k == 0) {
            dl->AddTriangleFilled(ImVec2{m - h * 1.4F, c + h * 0.6F}, ImVec2{m + h * 1.4F, c + h * 0.6F}, ImVec2{m, c - h * 0.8F}, f);
        } else {
            dl->AddTriangleFilled(ImVec2{m - h * 1.4F, c - h * 0.6F}, ImVec2{m, c + h * 0.8F}, ImVec2{m + h * 1.4F, c - h * 0.6F}, f);
        }
    }
    ImGui::PopItemFlag();
    if (ImGui::IsMouseHoveringRect(fa, ImVec2{fa.x + px(a.feldB), fa.y + feldH}) && io.MouseWheel != 0.0F) {   // QSpinBox_mod::wheelEvent
        const int n = static_cast<int>(io.MouseWheel);
        if (io.KeyShift) {
            schritt(wert, io.KeyCtrl ? 0.001 : 0.1, io.KeyCtrl ? n * 10 : n, a);
        } else {
            schritt(wert, 1.0, io.KeyCtrl ? n * 10 : n, a);
        }
    }
}

void anwenden(const Art& a, const Editor::ArbitraerDialog& d) {
    Editor& ed = editor();
    const karte::Vektor3f v(static_cast<float>(d.werte[0]), static_cast<float>(d.werte[1]), static_cast<float>(d.werte[2]));
    if (&a == &kDrehen) {
        karte::auswahlDrehenEuler(ed.bearbeitung, ed.karte, v);   // RotateDialog::apply
    } else {
        karte::auswahlSkalieren(ed.bearbeitung, ed.karte, v);     // ScaleDialog::apply
    }
    ed.bearbeitung.pivotGeaendert();
}

void abbrechen(const Art& a, Editor::ArbitraerDialog& d) {   // cancel: verstecken, Werte zuruecksetzen
    d.offen = false;
    d.werte = {a.vorgabe, a.vorgabe, a.vorgabe};
}

void dialog(const Art& a, Editor::ArbitraerDialog& d) {
    if (!d.offen) { return; }
    Editor& ed = editor();
    g_sk = massstab();
    dialogSchriftAn();
    const float labelB = std::max({textBreite("  X  "), textBreite("  Y  "), textBreite("  Z  ")});
    const float knopfX = 11.0F + labelB + 6.0F + a.feldB + 6.0F;
    const float innenB = knopfX + 80.0F + 11.0F;
    const float innenH = 100.0F;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    ImGui::SetNextWindowSize(ImVec2{px(innenB), px(innenH) + titelH}, ImGuiCond_Always);
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_FirstUseEver, ImVec2{0.5F, 0.5F});
    if (d.zeigen) {   // show(): raise(), activateWindow()
        d.zeigen = false;
        ImGui::SetNextWindowFocus();
    }
    const std::string titel = std::string(trK(a.titel)) + "###" + a.id;
    bool offen = true;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool sichtbar = ImGui::Begin(titel.c_str(), &offen,
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    bool ok = false;
    bool weg = !offen;   // QEvent::Close: cancel
    if (sichtbar) {
        g_innen = ImGui::GetCursorScreenPos();
        for (int i = 0; i < 3; ++i) { feld(d, i, 11.0F + static_cast<float>(i) * 28.0F, labelB, a); }
        const float feldH = px(22.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
        const ImVec2 kg{px(80.0F), feldH};
        ImGui::SetCursorScreenPos(bp(knopfX, 11.0F));
        if (ImGui::Button((std::string(trK(Str::KePrefOk)) + "##" + a.id + "ok").c_str(), kg)) { ok = true; }
        ImGui::SetCursorScreenPos(bp(knopfX, 39.0F));
        if (ImGui::Button((std::string(trK(Str::KeCancel)) + "##" + a.id + "abbrechen").c_str(), kg)) { weg = true; }
        ImGui::SetCursorScreenPos(bp(knopfX, 67.0F));
        if (ImGui::Button((std::string(trK(Str::KeArbApply)) + "##" + a.id + "anwenden").c_str(), kg)) { anwenden(a, d); }
        ImGui::PopStyleVar();
        // eventFilter (ShortcutOverride): Esc, Return/Enter, Tab, Leertaste
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            const bool esc = ImGui::IsKeyPressed(ImGuiKey_Escape, false);
            const bool enter = ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false);
            if (esc) { weg = true; }
            if (enter) { ok = true; }
            if (esc || enter || ImGui::IsKeyPressed(ImGuiKey_Tab, false) || ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
                ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
            }
        }
        ImGui::SetCursorScreenPos(bp(0.0F, innenH));
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
    }
    ImGui::End();
    dialogSchriftAus();
    if (ok) {   // ok(): apply(), verstecken (die Werte bleiben)
        anwenden(a, d);
        d.offen = false;
    } else if (weg) {
        abbrechen(a, d);
    }
}

}  // namespace

void arbitraerDialogZeigen(bool drehen) {
    Editor::ArbitraerDialog& d = drehen ? editor().drehDialog : editor().skalierDialog;
    d.offen = true;
    d.zeigen = true;
}

void zeichneArbitraerDialoge() {
    dialog(kDrehen, editor().drehDialog);
    dialog(kSkalieren, editor().skalierDialog);
}

}  // namespace bhed::gui::karteneditor
