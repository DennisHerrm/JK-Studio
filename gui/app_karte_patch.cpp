// app_karte_patch.cpp - Karten-Editor: die Dialoge des Curve-Menues ("Patch
// density", "Patch deform", "Cap", "Patch thicken").
//
// Portiert aus NetRadiant-Custom 20260114: radiant/patchmanip.cpp:837-1004
// (DoNewPatchDlg, DoPatchDeformDlg, DoCapDlg, DoPatchThickenDlg: modale
// QDialogs mit QFormLayout, SetFixedSize), libs/gtkutil/dialog.cpp:34-46
// (RadioHBox_new: QHBoxLayout) und libs/gtkutil/spinbox.h (SpinBoxLabel).
//
// Masse an den Fotos dlg_Curve_*.png (doppelte Groesse): Rand 11, Abstand 6;
// das Feld 166 breit (QComboBox / QSpinBox), 22 hoch; Radio- und
// Checkboxzeilen 20; die RadioHBox braucht je Knopf Kreis 14 + 4 + 6 + Text + 1
// (CT_RadioButton, SH_EtchDisabledText) und verteilt Platz, der uebrig ist, gleichmaessig
// (Deform: X 79, Y 136, Z 194); bei Thicken ist sie breiter als 166 und
// bestimmt die Feldspalte (174). OK/Cancel rechts, je 80 x 22. Fuer "Cap"
// gibt es kein Foto: Zeilen aus Symbol 32 (PM_LargeIconSize) und
// Radioknopf, senkrecht mittig (setAlignment AlignVCenter).
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
#include "bhed/karte/patchbefehle.h"
#include "platform.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

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

// QRadioButton::sizeHint: itemTextRect(fm, QRect(), TextShowMnemonic, enabled =
// false, text) - mit SH_EtchDisabledText (Fusion: an) einen Punkt breiter -,
// dazu CT_RadioButton: Kreis 14 + 4 + PM_RadioButtonLabelSpacing 6. Am Foto
// dlg_Curve_Thicken_0.png: die Kreise X, Y, Z, Normal je 38 auseinander =
// 24 + 8 ("X" 6,39 -> 7, + 1) + 6, die Zeile 174 breit.
float radioHinweis(const char* text) { return 14.0F + 4.0F + 6.0F + textBreite(text) + 1.0F; }

void zeilenText(const char* text, float x, float y, float h) {
    ImGui::GetWindowDrawList()->AddText(ImVec2{bp(x, 0.0F).x, std::floor(bp(0.0F, y).y + (px(h) - ImGui::GetTextLineHeight()) * 0.5F)},
                                        ImGui::GetColorU32(ImGuiCol_Text), text);
}

// QRadioButton: Kreis 14 senkrecht mittig in der Zeile h, Text 6 dahinter.
bool radio(const std::string& id, int& wert, int eigener, float x, float y, float h) {
    const float r = px(14.0F);
    ImGui::SetCursorScreenPos(ImVec2{bp(x, 0.0F).x, std::floor(bp(0.0F, y).y + (px(h) - r) * 0.5F)});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (r - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2{px(6.0F), 0.0F});
    const bool r2 = ImGui::RadioButton(id.c_str(), &wert, eigener);
    ImGui::PopStyleVar(2);
    return r2;
}

// RadioHBox in der Feldspalte (x, Breite b): je Knopf sein Hinweis plus ein
// gleicher Anteil am Rest.
void radioZeile(const std::vector<std::string>& texte, const char* id, int& wert, float x, float b, float y) {
    float summe = 0.0F;
    for (const auto& t : texte) { summe += radioHinweis(t.c_str()); }
    summe += 6.0F * static_cast<float>(texte.size() - 1);
    const float rest = std::max(0.0F, b - summe) / static_cast<float>(texte.size());
    float xi = x;
    for (std::size_t i = 0; i < texte.size(); ++i) {
        (void)radio(texte[i] + "##" + id + std::to_string(i), wert, static_cast<int>(i), xi, y, 20.0F);
        xi += radioHinweis(texte[i].c_str()) + rest + 6.0F;
    }
}

float radioZeileBreite(const std::vector<std::string>& texte) {
    float summe = 0.0F;
    for (const auto& t : texte) { summe += radioHinweis(t.c_str()); }
    return summe + 6.0F * static_cast<float>(texte.size() - 1);
}

// SpinBox mit SpinBoxLabel (int, Strg zehn Schritte), Feld x1..x1+b in Zeile y
void ganzzahlFeld(const char* beschriftung, const char* id, int& wert, int min, int max, int schritt, float x1, float b, float y) {
    static ImVec2 zugStart;
    static float summe = 0.0F;
    static const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
    ImGuiIO& io = ImGui::GetIO();
    const float feldH = px(22.0F);
    zeilenText(beschriftung, 11.0F, y, 22.0F);
    ImGui::SetCursorScreenPos(bp(11.0F, y));
    ImGui::InvisibleButton((std::string("##") + id + "zug").c_str(), ImVec2{px(textBreite(beschriftung)), feldH});
    bool fokus = ImGui::IsWindowAppearing();
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
        ImGui::SetMouseCursor(ImGui::IsItemActive() ? ImGuiMouseCursor_None : ImGuiMouseCursor_ResizeEW);
    }
    const auto weiter = [&](int n) { wert = std::clamp(wert + n * schritt, min, max); };
    if (ImGui::IsItemActivated()) {
        zugStart = io.MousePos;
        summe = 0.0F;
    } else if (ImGui::IsItemActive() && (io.MouseDelta.x != 0.0F || io.MouseDelta.y != 0.0F)) {
        const float qt = std::max(1.0F, std::round(g_app->dpiScale));
        summe += (io.MousePos.x - zugStart.x) / qt;
        const int delta = static_cast<int>(summe / 20.0F);
        if (delta != 0) {
            summe -= static_cast<float>(delta) * 20.0F;
            weiter(io.KeyCtrl ? delta * 10 : delta);
            if (!imTest) { ImGui::TeleportMousePos(zugStart); }
        }
    }
    if (ImGui::IsItemDeactivated()) { fokus = true; }
    const float pfeilB = px(16.0F);
    const ImVec2 fa = bp(x1, y);
    ImGui::SetCursorScreenPos(fa);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::SetNextItemWidth(px(b) - pfeilB);
    if (fokus) { ImGui::SetKeyboardFocusHere(); }
    int v = wert;
    if (ImGui::InputInt((std::string("##") + id).c_str(), &v, 0, 0)) { wert = std::clamp(v, min, max); }
    ImGui::PopStyleVar();
    if (ImGui::IsItemActive()) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { weiter(1); }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { weiter(-1); }
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
    for (int k = 0; k < 2; ++k) {
        const ImVec2 p0{fa.x + px(b) - pfeilB, fa.y + (k == 0 ? 0.0F : std::floor(feldH * 0.5F))};
        const ImVec2 p1{fa.x + px(b), k == 0 ? fa.y + std::floor(feldH * 0.5F) : fa.y + feldH};
        ImGui::SetCursorScreenPos(p0);
        if (ImGui::InvisibleButton((std::string("##") + id + (k == 0 ? "hoch" : "runter")).c_str(), ImVec2{p1.x - p0.x, p1.y - p0.y})) { weiter(k == 0 ? 1 : -1); }
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
    if (ImGui::IsMouseHoveringRect(fa, ImVec2{fa.x + px(b), fa.y + feldH}) && io.MouseWheel != 0.0F) {
        const int n = static_cast<int>(io.MouseWheel);
        weiter(io.KeyCtrl ? n * 10 : n);
    }
}

// QDialogButtonBox OK / Cancel rechts; true = fertig, ok gesetzt bei OK
bool knoepfe(float innenB, float y, bool& ok) {
    Editor& ed = editor();
    const float feldH = px(22.0F);
    bool fertig = false;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
    const ImVec2 kg{px(80.0F), feldH};
    ImGui::SetCursorScreenPos(bp(innenB - 11.0F - 80.0F - 6.0F - 80.0F, y));
    if (ImGui::Button((std::string(trK(Str::KePrefOk)) + "##patchok").c_str(), kg) ||
        (!ImGui::IsAnyItemActive() && (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)))) {
        fertig = true;
        ok = true;
        ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
    }
    ImGui::SetCursorScreenPos(bp(innenB - 11.0F - 80.0F, y));
    if (ImGui::Button((std::string(trK(Str::KeCancel)) + "##patchabbrechen").c_str(), kg) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
        fertig = true;
        ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
    }
    ImGui::PopStyleVar();
    return fertig;
}

const char* const kDichten[] = {"3", "5", "7", "9", "11", "13", "15", "17", "19", "21", "23", "25", "27", "29", "31"};

// QComboBox mit den ungeraden Groessen von min bis max (D_ITEM)
void dichteFeld(const char* beschriftung, const char* id, int& index, int min, int max, float x1, float y) {
    zeilenText(beschriftung, 11.0F, y, 22.0F);
    std::vector<const char*> eintraege;
    for (const char* d : kDichten) {
        const int v = std::atoi(d);
        if (v >= min && (max == 0 || v <= max)) { eintraege.push_back(d); }
    }
    const float feldH = px(22.0F);
    ImGui::SetCursorScreenPos(bp(x1, y));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::SetNextItemWidth(px(166.0F));
    ImGui::Combo((std::string("##") + id).c_str(), &index, eintraege.data(), static_cast<int>(eintraege.size()));
    ImGui::PopStyleVar();
}

}  // namespace

void patchDialogOeffnen(int art, karte::PatchVorlage vorlage) {
    Editor& ed = editor();
    Editor::PatchDialog& d = ed.patchDialog;
    d = Editor::PatchDialog{};   // jedes Mal neu gebaut
    d.art = art;
    d.vorlage = vorlage;
    d.oeffnen = true;
    if (art == 1) {   // "Initialize dialog": ( def - min ) / 2
        const karte::PatchDichte p = karte::patchDichte(vorlage);
        d.spalten = (p.vorgabeSpalten - p.minSpalten) / 2;
        d.zeilen = (p.vorgabeZeilen - p.minZeilen) / 2;
    } else if (art == 2) {   // SpinBox( -9999, 9999, 64 ), Z
        d.wert = 64;
        d.achse = 2;
    } else if (art == 4) {   // SpinBox( -9999, 9999, grid, 2, grid ), Normal, Side walls
        d.wert = static_cast<int>(std::max(ed.raster.groesse(), 1.0F));
        d.achse = 3;
        d.waende = true;
    }
}

void zeichnePatchDialog() {
    Editor& ed = editor();
    Editor::PatchDialog& d = ed.patchDialog;
    if (d.art == 0) { return; }
    static const Str kTitel[5] = {Str::Count, Str::KePdDensityTitle, Str::KePdDeformTitle, Str::KePdCapTitle, Str::KePdThickenTitle};
    const std::string id = std::string(trK(kTitel[d.art])) + "###kartepatchdialog";
    if (d.oeffnen) {
        d.oeffnen = false;
        ImGui::OpenPopup(id.c_str());
    }
    g_sk = massstab();
    dialogSchriftAn();
    // Masse je Dialog
    float innenB = 0.0F;
    float innenH = 0.0F;
    float x1 = 0.0F;
    float feldB = 166.0F;
    const std::vector<std::string> achsen3{"X", "Y", "Z"};
    const std::vector<std::string> achsen4{"X", "Y", "Z", trK(Str::KePdNormal)};
    if (d.art == 1) {
        x1 = 11.0F + std::max(textBreite(trK(Str::KePdWidth)), textBreite(trK(Str::KePdHeight))) + 6.0F;
        innenB = x1 + feldB + 11.0F;
        innenH = d.vorlage == karte::PatchVorlage::Ebene ? 100.0F : 126.0F;
    } else if (d.art == 2) {
        x1 = 11.0F + textBreite(trK(Str::KePdMaxDeform)) + 6.0F;
        feldB = std::max(166.0F, radioZeileBreite(achsen3));
        innenB = x1 + feldB + 11.0F;
        innenH = 98.0F;
    } else if (d.art == 3) {
        x1 = 11.0F + 32.0F + 6.0F;
        float rb = 0.0F;
        for (const Str s : {Str::KePdCapBevel, Str::KePdCapEndcap, Str::KePdCapIBevel, Str::KePdCapIEndcap, Str::KePdCapCylinder}) {
            rb = std::max(rb, radioHinweis(trK(s)));
        }
        innenB = std::max(x1 + rb, 11.0F + 166.0F) + 11.0F;
        innenH = 11.0F + 5.0F * 38.0F + 22.0F + 11.0F;
    } else {
        x1 = 11.0F + textBreite(trK(Str::KePdThickness)) + 6.0F;
        feldB = std::max(166.0F, radioZeileBreite(achsen4));
        innenB = x1 + feldB + 11.0F;
        innenH = 124.0F;
    }
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
        if (d.art == 1) {   // DoNewPatchDlg
            const karte::PatchDichte p = karte::patchDichte(d.vorlage);
            dichteFeld(trK(Str::KePdWidth), "patchbreite", d.spalten, p.minSpalten, p.maxSpalten, x1, 11.0F);
            dichteFeld(trK(Str::KePdHeight), "patchhoehe", d.zeilen, p.minZeilen, p.maxZeilen, x1, 39.0F);
            float y = 67.0F;
            if (d.vorlage != karte::PatchVorlage::Ebene) {
                ImGui::SetCursorScreenPos(bp(x1, 67.0F));
                (void)qtHaken((std::string(trK(Str::KePdSquare)) + "##patchquadrat").c_str(), &d.quadrat, 20.0F);
                if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", trK(Str::KePdSquareTip)); }
                y = 93.0F;
            }
            fertig = knoepfe(innenB, y, ok);
        } else if (d.art == 2) {   // DoPatchDeformDlg
            ganzzahlFeld(trK(Str::KePdMaxDeform), "patchverformen", d.wert, -9999, 9999, 1, x1, feldB, 11.0F);
            radioZeile(achsen3, "patchachse", d.achse, x1, feldB, 39.0F);
            fertig = knoepfe(innenB, 65.0F, ok);
        } else if (d.art == 3) {   // DoCapDlg
            static const char* const bilder[5] = {"cap_bevel", "cap_endcap", "cap_ibevel", "cap_iendcap", "cap_cylinder"};
            static const Str texte[5] = {Str::KePdCapBevel, Str::KePdCapEndcap, Str::KePdCapIBevel, Str::KePdCapIEndcap, Str::KePdCapCylinder};
            for (int i = 0; i < 5; ++i) {
                const float y = 11.0F + static_cast<float>(i) * 38.0F;
                const ImVec2 a = bp(11.0F, y);
                const float g = px(32.0F);
                (void)karteIcon(ImGui::GetWindowDrawList(), bilder[i], a, g, ImGui::GetColorU32(karteIconFarbe(bilder[i])));
                (void)radio(std::string(trK(texte[i])) + "##patchkappe" + std::to_string(i), d.kappe, i, x1, y, 32.0F);
            }
            fertig = knoepfe(innenB, 11.0F + 5.0F * 38.0F, ok);
        } else {   // DoPatchThickenDlg
            const int raster = static_cast<int>(std::max(ed.raster.groesse(), 1.0F));
            ganzzahlFeld(trK(Str::KePdThickness), "patchdicke", d.wert, -9999, 9999, raster, x1, feldB, 11.0F);
            radioZeile(achsen4, "patchdickeachse", d.achse, x1, feldB, 39.0F);
            ImGui::SetCursorScreenPos(bp(x1, 65.0F));
            (void)qtHaken((std::string(trK(Str::KePdSideWalls)) + "##patchwaende").c_str(), &d.waende, 20.0F);
            fertig = knoepfe(innenB, 91.0F, ok);
        }
        ImGui::SetCursorScreenPos(bp(0.0F, innenH));
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
        if (fertig) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    dialogSchriftAus();
    if (!fertig) { return; }
    const int art = d.art;
    d.art = 0;
    if (!ok) { return; }
    karte::Karte& k = ed.karte;
    const auto achse = static_cast<int>(ed.xy[static_cast<std::size_t>(ed.aktivXy)].ansicht.typ);
    if (art == 1) {   // Scene_PatchConstructPrefab( bounds, shader, prefab, viewtype, w, h, redisperse )
        const karte::PatchDichte p = karte::patchDichte(d.vorlage);
        const int w = d.spalten * 2 + p.minSpalten;
        const int h = d.zeilen * 2 + p.minZeilen;
        karte::patchVorlageBefehl(k, karte::patchGrenzen(ed.workzone, ed.raster.groesse()), ed.bearbeitung.browserShader, d.vorlage, achse,
                                  static_cast<std::size_t>(w), static_cast<std::size_t>(h), d.quadrat);
    } else if (art == 2) {
        karte::patchVerformen(k, d.wert, d.achse);
    } else if (art == 3) {
        karte::Protokoll pr;
        karte::patchKappen(k, ed.bearbeitung.browserShader, static_cast<karte::PatchKappe>(d.kappe), pr);
        konsoleProtokoll(pr);
    } else {
        karte::patchVerdicken(k, d.wert, d.waende, d.achse);
    }
    auswahlGeaendert();
}

}  // namespace bhed::gui::karteneditor
