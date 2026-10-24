// app_karte_formen.cpp - Karten-Editor: der Dialog "Arbitrary sides" fuer
// die Pinsel-Grundformen (Prism, Cone, Sphere, Rock, Icosahedron).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/gtkdlgs.cpp:272-327
// (DoSides: QDialog mit QFormLayout, SetFixedSize; SpinBoxLabel "Sides:" bzw.
// "Subdivisions:", SpinBox, versteckte Checkbox "Truncate", OK/Cancel) und
// libs/gtkutil/spinbox.h (SpinBoxLabel).
//
// Masse an den Fotos dlg_Brush_Prism_0.png und dlg_Brush_Icosahedron_0.png
// (doppelte Groesse): Rand 11, Abstand 6; das Feld 166 breit und 22 hoch
// hinter der Beschriftung (Spalte 0 so breit wie der Text); die versteckte
// Checkbox laesst ihren Zeilenabstand stehen (QFormLayout), darum stehen
// die Knoepfe bei Prism auf 45, bei Icosahedron (Checkbox 20 hoch) auf 65;
// QDialogButtonBox rechts, je 80 x 22.
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
#include <cmath>
#include <cstdlib>
#include <string>

namespace bhed::gui::karteneditor {

namespace {

float g_sk = 1.0F;
ImVec2 g_innen{0.0F, 0.0F};

ImVec2 bp(float x, float y) { return ImVec2{std::round(g_innen.x + x * g_sk), std::round(g_innen.y + y * g_sk)}; }
float px(float l) { return std::round(l * g_sk); }

// QLabel::sizeHint in Tahoma 8 pt (GDI wie Qt)
float textBreite(const char* text) {
    const int b = platform::gdiTextBreite(text, L"Tahoma", 11, false);
    return b >= 0 ? static_cast<float>(b) : std::round(ImGui::CalcTextSize(text).x / g_sk);
}

void schritt(Editor::SeitenDialog& d, int n) {   // QAbstractSpinBox::stepBy (ohne Umlauf)
    const karte::SeitenBereich b = karte::seitenBereich(d.art);
    d.wert = std::clamp(d.wert + n, b.min, b.max);
}

}  // namespace

void seitenDialogOeffnen(karte::Grundform art) {
    Editor::SeitenDialog& d = editor().seitenDialog;
    d = Editor::SeitenDialog{};   // DoSides baut den Dialog jedes Mal neu
    d.art = art;
    d.wert = karte::seitenBereich(art).vorgabe;
    d.offen = true;
    d.oeffnen = true;
}

void zeichneSeitenDialog() {
    Editor& ed = editor();
    Editor::SeitenDialog& d = ed.seitenDialog;
    if (!d.offen) { return; }
    const std::string id = std::string(trK(Str::KeSidesTitle)) + "###karteseiten";
    if (d.oeffnen) {
        d.oeffnen = false;
        ImGui::OpenPopup(id.c_str());
    }
    g_sk = massstab();
    const bool iko = d.art == karte::Grundform::Ikosaeder;
    dialogSchriftAn();
    const char* beschriftung = trK(iko ? Str::KeSidesSubdivisions : Str::KeSidesSides);
    const float labelB = textBreite(beschriftung);
    const float x1 = 11.0F + labelB + 6.0F;
    const float innenB = x1 + 166.0F + 11.0F;
    const float knopfY = iko ? 65.0F : 45.0F;
    const float innenH = knopfY + 22.0F + 11.0F;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    ImGui::SetNextWindowSize(ImVec2{px(innenB), px(innenH) + titelH}, ImGuiCond_Always);
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    bool fertig = false;
    bool ok = false;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool sichtbar = ImGui::BeginPopupModal(id.c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    if (sichtbar) {
        g_innen = ImGui::GetCursorScreenPos();
        ImGuiIO& io = ImGui::GetIO();
        const float feldH = px(22.0F);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        // SpinBoxLabel: die Beschriftung, waagerecht ziehen (spinbox.h:96-150)
        dl->AddText(ImVec2{bp(11.0F, 0.0F).x, std::floor(bp(0.0F, 11.0F).y + (feldH - ImGui::GetTextLineHeight()) * 0.5F)},
                    ImGui::GetColorU32(ImGuiCol_Text), beschriftung);
        static ImVec2 zugStart;
        static float summe = 0.0F;
        static const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
        bool fokus = ImGui::IsWindowAppearing();   // spin->selectAll()
        ImGui::SetCursorScreenPos(bp(11.0F, 11.0F));
        ImGui::InvisibleButton("##seitenziehen", ImVec2{px(labelB), feldH});
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
                summe -= static_cast<float>(delta) * 20.0F;   // m_dragAccum %= 20
                schritt(d, io.KeyCtrl ? delta * 10 : delta);  // SpinBox: kein Teilschritt
                if (!imTest) { ImGui::TeleportMousePos(zugStart); }
            }
        }
        if (ImGui::IsItemDeactivated()) { fokus = true; }
        // SpinBox: Text, rechts die Pfeile uebereinander
        const float pfeilB = px(16.0F);
        const ImVec2 fa = bp(x1, 11.0F);
        ImGui::SetCursorScreenPos(fa);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
        ImGui::SetNextItemWidth(px(166.0F) - pfeilB);
        if (fokus) { ImGui::SetKeyboardFocusHere(); }
        int wert = d.wert;
        if (ImGui::InputInt("##seitenwert", &wert, 0, 0)) {
            const karte::SeitenBereich b = karte::seitenBereich(d.art);
            d.wert = std::clamp(wert, b.min, b.max);
        }
        ImGui::PopStyleVar();
        const bool feldAktiv = ImGui::IsItemActive();
        const bool enterImFeld = ImGui::IsItemDeactivated() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter));
        if (feldAktiv) {
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { schritt(d, 1); }
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { schritt(d, -1); }
            if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) { schritt(d, 10); }
            if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) { schritt(d, -10); }
        }
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        for (int i = 0; i < 2; ++i) {
            const ImVec2 a{fa.x + px(166.0F) - pfeilB, fa.y + (i == 0 ? 0.0F : std::floor(feldH * 0.5F))};
            const ImVec2 b{fa.x + px(166.0F), i == 0 ? fa.y + std::floor(feldH * 0.5F) : fa.y + feldH};
            ImGui::SetCursorScreenPos(a);
            if (ImGui::InvisibleButton(i == 0 ? "##seitenhoch" : "##seitenrunter", ImVec2{b.x - a.x, b.y - a.y})) { schritt(d, i == 0 ? 1 : -1); }
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
        if (ImGui::IsMouseHoveringRect(fa, ImVec2{fa.x + px(166.0F), fa.y + feldH}) && io.MouseWheel != 0.0F) {
            const int n = static_cast<int>(io.MouseWheel);
            schritt(d, io.KeyCtrl ? n * 10 : n);
        }
        if (iko) {   // check->show(): "Truncate" in der Feldspalte, Zeile 20 hoch
            ImGui::SetCursorScreenPos(bp(x1, 39.0F));
            (void)qtHaken((std::string(trK(Str::KeSidesTruncate)) + "##seitenstutzen").c_str(), &d.stutzen, 20.0F);
        }
        // QDialogButtonBox: OK, Cancel rechts
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
        const ImVec2 kg{px(80.0F), feldH};
        ImGui::SetCursorScreenPos(bp(innenB - 11.0F - 80.0F - 6.0F - 80.0F, knopfY));
        if (ImGui::Button((std::string(trK(Str::KePrefOk)) + "##seitenok").c_str(), kg) || enterImFeld ||
            (!ImGui::IsAnyItemActive() && (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)))) {
            fertig = true;
            ok = true;
            ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
        }
        ImGui::SetCursorScreenPos(bp(innenB - 11.0F - 80.0F, knopfY));
        if (ImGui::Button((std::string(trK(Str::KeCancel)) + "##seitenabbrechen").c_str(), kg) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            fertig = true;
            ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
        }
        ImGui::PopStyleVar();
        ImGui::SetCursorScreenPos(bp(0.0F, innenH));
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
        if (fertig) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    dialogSchriftAus();
    if (!fertig) { return; }
    d.offen = false;
    if (ok) {   // Scene_BrushConstructPrefab( type, sides, option, TextureBrowser_GetSelectedShader() )
        karte::Protokoll p;
        const auto achse = static_cast<std::size_t>(ed.xy[static_cast<std::size_t>(ed.aktivXy)].ansicht.typ);
        karte::grundformAuswahl(ed.karte, d.art, static_cast<std::size_t>(d.wert), d.stutzen, achse, ed.bearbeitung.browserShader, p);
        konsoleProtokoll(p);
        auswahlGeaendert();
    }
}

}  // namespace bhed::gui::karteneditor
