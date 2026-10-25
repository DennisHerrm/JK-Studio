// karte_qtdialog.cpp - Karten-Editor, Stufe 4: Bausteine fuer die Dialoge der
// Plugins (siehe karte_qtdialog.h).
//
// Nach Qt 5.15 (qabstractspinbox.cpp sizeHint, qlineedit.cpp sizeHint,
// qcommonstyle.cpp / qfusionstyle.cpp sizeFromContents, qdialogbuttonbox.cpp)
// und NRC 20260114 libs/gtkutil/spinbox.h (SpinBoxLabel: Ziehen 20 Punkte je
// Schritt, Strg x 10; QSpinBox_mod: setAccelerated, Mausrad).
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
#include "karte_qtdialog.h"

#include "app_internal.h"
#include "karteneditor.h"
#include "platform.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace bhed::gui::karteneditor::qtd {

namespace {

float g_sk = 1.0F;
ImVec2 g_innen{0.0F, 0.0F};
bool g_geschlossen = false;

std::string zahlText(double v, int stellen) {
    char z[64];
    std::snprintf(z, sizeof(z), "%.*f", stellen, v);
    return z;
}

}  // namespace

ImVec2 bp(float x, float y) { return ImVec2{std::round(g_innen.x + x * g_sk), std::round(g_innen.y + y * g_sk)}; }
float px(float l) { return std::round(l * g_sk); }
float sk() { return g_sk; }

float textBreite(const char* text) {
    const int b = platform::qtTextBreite(text, L"Tahoma", 11.0F, false);
    return b >= 0 ? static_cast<float>(b) : std::round(ImGui::CalcTextSize(text).x / std::max(g_sk, 0.01F));
}

// QCommonStyle CT_CheckBox wie CT_RadioButton: Kaestchen 14 + 4 (Raender) + 6 + Text + 1
float hakenHinweis(const std::string& text) { return 14.0F + 4.0F + 6.0F + textBreite(text) + 1.0F; }
float radioHinweis(const std::string& text) { return 14.0F + 4.0F + 6.0F + textBreite(text) + 1.0F; }
float knopfHinweis(const std::string& text) { return std::max(80.0F, textBreite(text) + 8.0F); }

float knoepfeHinweis(const std::vector<std::string>& texte) {
    float b = 0.0F;
    for (const auto& t : texte) { b += knopfHinweis(t); }
    return b + 6.0F * static_cast<float>(texte.empty() ? 0 : texte.size() - 1);
}

float zeileHinweis() { return (17.0F * textBreite("x")) + 4.0F + 2.0F; }

float spinHinweis(double min, double max, int stellen, const std::string& vorsatz) {
    const float w = std::max(textBreite(vorsatz + zahlText(min, stellen) + " "), textBreite(vorsatz + zahlText(max, stellen) + " "));
    return w + 2.0F + 16.0F + 4.0F;
}

float kombiHinweis(const std::vector<std::string>& eintraege) {
    float w = 0.0F;
    for (const auto& e : eintraege) { w = std::max(w, textBreite(e)); }
    return w + 8.0F + 30.0F;
}

float radioReiheHinweis(const std::vector<std::string>& texte) {
    float b = 0.0F;
    for (const auto& t : texte) { b += radioHinweis(t); }
    return b + 6.0F * static_cast<float>(texte.empty() ? 0 : texte.size() - 1);
}

bool beginnen(const std::string& id, bool& oeffnen, float innenB, float innenH, bool modal) {
    if (oeffnen && modal) {
        oeffnen = false;
        ImGui::OpenPopup(id.c_str());
    }
    g_sk = massstab();
    dialogSchriftAn();
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    ImGui::SetNextWindowSize(ImVec2{px(innenB), px(innenH) + titelH}, ImGuiCond_Always);
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F},
                            modal ? ImGuiCond_Appearing : (oeffnen ? ImGuiCond_Always : ImGuiCond_Appearing), ImVec2{0.5F, 0.5F});
    if (!modal) { oeffnen = false; }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    bool offen = true;
    const ImGuiWindowFlags f = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse;
    const bool sichtbar = modal ? ImGui::BeginPopupModal(id.c_str(), &offen, f) : ImGui::Begin(id.c_str(), &offen, f);
    ImGui::PopStyleVar();
    g_geschlossen = !offen || (modal && !sichtbar && !ImGui::IsPopupOpen(id.c_str()));
    if (!sichtbar) {
        if (!modal) { ImGui::End(); }
        dialogSchriftAus();
        return false;
    }
    g_innen = ImGui::GetCursorScreenPos();
    return true;
}

void beenden(bool schliessen) {
    ImGui::SetCursorScreenPos(g_innen);
    ImGui::Dummy(ImVec2{0.0F, 0.0F});
    if (ImGui::GetCurrentWindow()->Flags & ImGuiWindowFlags_Modal) {
        if (schliessen) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    } else {
        ImGui::End();
    }
    dialogSchriftAus();
}

bool geschlossen() { return g_geschlossen; }

void text(const std::string& t, float x, float y, float h, bool aktiv) {
    ImGui::GetWindowDrawList()->AddText(ImVec2{bp(x, 0.0F).x, std::floor(bp(0.0F, y).y + (px(h) - ImGui::GetTextLineHeight()) * 0.5F)},
                                        ImGui::GetColorU32(aktiv ? ImGuiCol_Text : ImGuiCol_TextDisabled), t.c_str());
}

void merke(const std::string& name, float x, float y, float b, float h) {
    merkeFeld("Plugin:" + name, static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), static_cast<int>(std::lround(b)),
              static_cast<int>(std::lround(h)));
}

bool haken(const std::string& text, const std::string& id, bool& wert, float x, float y, float h, bool aktiv) {
    ImGui::SetCursorScreenPos(bp(x, y));
    ImGui::BeginDisabled(!aktiv);
    const bool r = qtHaken((text + "##" + id).c_str(), &wert, h);
    ImGui::EndDisabled();
    merke(id, x, y, hakenHinweis(text), h);
    return r;
}

// QRadioButton: Kreis 14 senkrecht mittig in der Zeile h, Text 6 dahinter
bool radio(const std::string& text, const std::string& id, int& wert, int eigener, float x, float y, float h, bool aktiv) {
    const float r = px(14.0F);
    ImGui::SetCursorScreenPos(ImVec2{bp(x, 0.0F).x, std::floor(bp(0.0F, y).y + (px(h) - r) * 0.5F)});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (r - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2{px(6.0F), 0.0F});
    ImGui::BeginDisabled(!aktiv);
    const bool r2 = ImGui::RadioButton((text + "##" + id).c_str(), &wert, eigener);
    ImGui::EndDisabled();
    ImGui::PopStyleVar(2);
    merke(id, x, y, radioHinweis(text), h);
    return r2;
}

void radioReihe(const std::vector<std::string>& texte, const std::string& id, int& wert, float x, float b, float y, const std::vector<int>* werte) {
    const float rest = std::max(0.0F, b - radioReiheHinweis(texte)) / static_cast<float>(std::max<std::size_t>(texte.size(), 1));
    float xi = x;
    for (std::size_t i = 0; i < texte.size(); ++i) {
        const int eigener = werte != nullptr ? (*werte)[i] : static_cast<int>(i);
        (void)radio(texte[i], id + std::to_string(i), wert, eigener, xi, y, 20.0F);
        xi += radioHinweis(texte[i]) + rest + 6.0F;
    }
}

bool zahlFeld(const std::string& beschriftung, const std::string& id, double& wert, double min, double max, int stellen, double schritt,
              float beschriftungX, float x, float b, float y, bool aktiv, const std::string& vorsatz, const char* tip) {
    struct Zug {
        ImVec2 start;
        float summe = 0.0F;
    };
    static Zug zug;
    static const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
    ImGuiIO& io = ImGui::GetIO();
    const float feldH = px(22.0F);
    const double alt = wert;
    const auto weiter = [&](double n) { wert = std::clamp(wert + n * schritt, min, max); };
    ImGui::BeginDisabled(!aktiv);
    if (beschriftungX >= 0.0F) {
        text(beschriftung, beschriftungX, y, 22.0F, aktiv);
        ImGui::SetCursorScreenPos(bp(beschriftungX, y));
        ImGui::InvisibleButton(("##" + id + "zug").c_str(), ImVec2{std::max(1.0F, px(textBreite(beschriftung))), feldH});
        if (tip != nullptr && ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tip); }
        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            ImGui::SetMouseCursor(ImGui::IsItemActive() ? ImGuiMouseCursor_None : ImGuiMouseCursor_ResizeEW);
        }
        if (ImGui::IsItemActivated()) {   // SpinBoxLabel::mousePressEvent
            zug.start = io.MousePos;
            zug.summe = 0.0F;
        } else if (ImGui::IsItemActive() && (io.MouseDelta.x != 0.0F || io.MouseDelta.y != 0.0F)) {
            const float qt = std::max(1.0F, std::round(g_app->dpiScale));
            zug.summe += (io.MousePos.x - zug.start.x) / qt;   // m_dragAccum += pos.x - m_dragStart.x
            const int delta = static_cast<int>(zug.summe / 20.0F);
            if (delta != 0) {
                zug.summe -= static_cast<float>(delta) * 20.0F;
                // Shift nur bei QDoubleSpinBox: Schritt / 10 (mit Strg / 100)
                if (stellen > 0 && io.KeyShift) {
                    weiter(delta * (io.KeyCtrl ? 0.01 : 0.1));
                } else {
                    weiter(io.KeyCtrl ? delta * 10.0 : delta);
                }
                if (!imTest) { ImGui::TeleportMousePos(zug.start); }
            }
        }
    }
    const float pfeilB = px(16.0F);
    const ImVec2 fa = bp(x, y);
    ImGui::SetCursorScreenPos(fa);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::SetNextItemWidth(px(b) - pfeilB);
    char format[24];
    std::snprintf(format, sizeof(format), "%s%%.%df", vorsatz.c_str(), stellen);
    double v = wert;
    if (ImGui::InputScalar(("##" + id).c_str(), ImGuiDataType_Double, &v, nullptr, nullptr, format)) { wert = std::clamp(v, min, max); }
    ImGui::PopStyleVar();
    if (ImGui::IsItemActive()) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { weiter(1); }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { weiter(-1); }
        if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) { weiter(10); }
        if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) { weiter(-10); }
    }
    if (tip != nullptr && ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tip); }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
    for (int k = 0; k < 2; ++k) {
        const ImVec2 p0{fa.x + px(b) - pfeilB, fa.y + (k == 0 ? 0.0F : std::floor(feldH * 0.5F))};
        const ImVec2 p1{fa.x + px(b), k == 0 ? fa.y + std::floor(feldH * 0.5F) : fa.y + feldH};
        ImGui::SetCursorScreenPos(p0);
        if (ImGui::InvisibleButton(("##" + id + (k == 0 ? "hoch" : "runter")).c_str(), ImVec2{p1.x - p0.x, p1.y - p0.y})) { weiter(k == 0 ? 1 : -1); }
        const bool u = ImGui::IsItemHovered();
        dl->AddRectFilled(p0, p1, ImGui::GetColorU32(u ? (ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered) : ImGuiCol_FrameBg));
        const float m = (p0.x + p1.x) * 0.5F;
        const float h = std::max(2.0F, px(2.5F));
        const float c = (p0.y + p1.y) * 0.5F;
        const ImU32 f = ImGui::GetColorU32(aktiv ? ImGuiCol_Text : ImGuiCol_TextDisabled);
        if (k == 0) {
            dl->AddTriangleFilled(ImVec2{m - h * 1.4F, c + h * 0.6F}, ImVec2{m + h * 1.4F, c + h * 0.6F}, ImVec2{m, c - h * 0.8F}, f);
        } else {
            dl->AddTriangleFilled(ImVec2{m - h * 1.4F, c - h * 0.6F}, ImVec2{m, c + h * 0.8F}, ImVec2{m + h * 1.4F, c - h * 0.6F}, f);
        }
    }
    ImGui::PopItemFlag();
    if (aktiv && ImGui::IsMouseHoveringRect(fa, ImVec2{fa.x + px(b), fa.y + feldH}) && io.MouseWheel != 0.0F) {
        const double n = static_cast<double>(io.MouseWheel);
        weiter(io.KeyCtrl ? n * 10.0 : n);
    }
    ImGui::EndDisabled();
    merke(id, x, y, b, 22.0F);
    return wert != alt;
}

bool ganzFeld(const std::string& beschriftung, const std::string& id, int& wert, int min, int max, int schritt, float beschriftungX, float x, float b,
              float y, bool aktiv, const char* tip) {
    double v = wert;
    const bool r = zahlFeld(beschriftung, id, v, min, max, 0, schritt, beschriftungX, x, b, y, aktiv, "", tip);
    wert = static_cast<int>(std::lround(v));
    return r;
}

bool zeileFeld(const std::string& id, std::string& wert, float x, float b, float y, int maxLaenge, bool aktiv) {
    char puffer[512];
    const std::size_t n = std::min<std::size_t>({wert.size(), sizeof(puffer) - 1, static_cast<std::size_t>(std::max(maxLaenge, 0))});
    std::memcpy(puffer, wert.data(), n);
    puffer[n] = 0;
    const float feldH = px(22.0F);
    ImGui::SetCursorScreenPos(bp(x, y));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::SetNextItemWidth(px(b));
    ImGui::BeginDisabled(!aktiv);
    const bool r = ImGui::InputText(("##" + id).c_str(), puffer, static_cast<std::size_t>(std::clamp(maxLaenge + 1, 1, static_cast<int>(sizeof(puffer)))));
    ImGui::EndDisabled();
    ImGui::PopStyleVar();
    if (r) { wert = puffer; }
    merke(id, x, y, b, 22.0F);
    return r;
}

bool kombiFeld(const std::string& id, std::string& wert, const std::vector<std::string>& liste, float x, float b, float y) {
    const float pfeil = 20.0F;
    bool r = zeileFeld(id, wert, x, b - pfeil, y);
    const ImVec2 a = bp(x + b - pfeil, y);
    ImGui::SetCursorScreenPos(a);
    const std::string popup = "##" + id + "liste";
    if (ImGui::InvisibleButton(("##" + id + "pfeil").c_str(), ImVec2{px(pfeil), px(22.0F)})) { ImGui::OpenPopup(popup.c_str()); }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 c{a.x + px(pfeil), a.y + px(22.0F)};
    dl->AddRectFilled(a, c, ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_ButtonHovered : ImGuiCol_FrameBg));
    const float m = (a.x + c.x) * 0.5F;
    const float h = std::max(2.0F, px(3.0F));
    const float my = (a.y + c.y) * 0.5F;
    dl->AddTriangleFilled(ImVec2{m - h * 1.3F, my - h * 0.6F}, ImVec2{m, my + h * 0.7F}, ImVec2{m + h * 1.3F, my - h * 0.6F},
                          ImGui::GetColorU32(ImGuiCol_Text));
    ImGui::SetNextWindowPos(ImVec2{bp(x, 0.0F).x, c.y});
    ImGui::SetNextWindowSizeConstraints(ImVec2{px(b), 0.0F}, ImVec2{px(b), px(22.0F) * 12.0F});
    if (ImGui::BeginPopup(popup.c_str())) {
        for (std::size_t i = 0; i < liste.size(); ++i) {
            if (ImGui::Selectable((liste[i] + "##" + id + std::to_string(i)).c_str(), liste[i] == wert)) {
                wert = liste[i];
                r = true;
            }
        }
        ImGui::EndPopup();
    }
    merke(id, x, y, b, 22.0F);
    return r;
}

int knoepfe(const std::vector<std::string>& texte, const std::string& id, float rechts, float y, int standard, int abbrechen) {
    Editor& ed = editor();
    const float feldH = px(22.0F);
    int gedrueckt = -1;
    float x = rechts - knoepfeHinweis(texte);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
    for (std::size_t i = 0; i < texte.size(); ++i) {
        const float b = knopfHinweis(texte[i]);
        ImGui::SetCursorScreenPos(bp(x, y));
        if (ImGui::Button((texte[i] + "##" + id + std::to_string(i)).c_str(), ImVec2{px(b), feldH})) { gedrueckt = static_cast<int>(i); }
        merke(id + std::to_string(i), x, y, b, 22.0F);
        x += b + 6.0F;
    }
    ImGui::PopStyleVar();
    if (gedrueckt < 0 && standard >= 0 && !ImGui::IsAnyItemActive() &&
        (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))) {
        gedrueckt = standard;
    }
    if (gedrueckt < 0 && abbrechen >= 0 && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { gedrueckt = abbrechen; }
    if (gedrueckt >= 0) { ed.tasteVerbrauchtBild = ImGui::GetFrameCount(); }
    return gedrueckt;
}

void hLinie(float x, float y, float b) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float d = std::max(1.0F, px(1.0F));
    const ImVec2 a = bp(x, y + 1.0F);
    dl->AddRectFilled(a, ImVec2{bp(x + b, 0.0F).x, a.y + d}, ImGui::GetColorU32(ImGuiCol_Border));
    dl->AddRectFilled(ImVec2{a.x, a.y + d}, ImVec2{bp(x + b, 0.0F).x, a.y + 2.0F * d}, ImGui::GetColorU32(ImGuiCol_BorderShadow, 1.0F));
}

void rahmen(float x, float y, float b, float h) {
    ImGui::GetWindowDrawList()->AddRect(bp(x, y), bp(x + b, y + h), ImGui::GetColorU32(ImGuiCol_Border));
}

}  // namespace bhed::gui::karteneditor::qtd
