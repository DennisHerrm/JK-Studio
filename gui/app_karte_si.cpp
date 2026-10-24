// app_karte_si.cpp - Karten-Editor: der "Surface Inspector" (S).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/surfacedialog.cpp:76-140
// (Increment), :226-282 (SurfaceInspector_SetCurrent_FromSelected), :285-334
// (si_globals, DoSnapTToGrid, SurfaceInspector_GridChange, OnBtnMatchGrid),
// :337-350 (DoSurface, SurfaceInspector_toggleShown), :594-628
// (g_pressedKeysFilter), :632-929 (BuildDialog), :940-1003 (Update),
// :1011-1078 (ApplyShader, ApplyTexdef_*) und libs/gtkutil/nonmodal.h
// (NonModalEntry, NonModalSpinner: Uebernehmen mit Enter/Fokuswechsel,
// Esc verwirft, Pfeile/Rad/Ziehen uebernehmen sofort).
//
// Masse am Foto dlg_View_Surface_Inspector_0.png (doppelte Groesse, NRC zieht
// das Fenster auf die kleinste Groesse: Innenraum 285 x 364): Rand 11,
// Abstand 6. "Texture" 22 hoch bei y 11, das Feld ab 53; das Raster 5 x 5 mit
// den Zeilen 39, 67, 95, 123, 151: Beschriftung bei 11, die Knoepfe "-" bei
// 103 (15 breit), die Spinbox bei 124 (89), "Step" bei 219, das Feld bei 247
// (27); "Match Grid" rechts bei 194 (80 x 22) in Zeile 179; die Gruppe
// "Texturing" (Titel 211, Rahmen 12..274 x 225..353) mit den Zeilen 235, 263,
// 291, 319 und den Spalten 23 (43), 72 (44), 122 (67), 195 (67). Jedi Academy
// und Jedi Outcast haben kein "si_flags" (ja.game, jk2.game): ohne die
// Flaggen-Gruppen, wie NRC.
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
#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/patchbefehle.h"
#include "bhed/karte/texturbefehle.h"
#include "imgui_internal.h"
#include "platform.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace bhed::gui::karteneditor {

namespace {

float g_sk = 1.0F;
ImVec2 g_innen{0.0F, 0.0F};

ImVec2 bp(float x, float y) { return ImVec2{std::round(g_innen.x + x * g_sk), std::round(g_innen.y + y * g_sk)}; }
float px(float l) { return std::round(l * g_sk); }

float textBreite(const char* text) {
    const int b = platform::gdiTextBreite(text, L"Tahoma", 11, false);
    return b >= 0 ? static_cast<float>(b) : std::round(ImGui::CalcTextSize(text).x / g_sk);
}

void zeilenText(const char* text, float x, float y) {
    ImGui::GetWindowDrawList()->AddText(ImVec2{bp(x, 0.0F).x, std::floor(bp(0.0F, y).y + (px(22.0F) - ImGui::GetTextLineHeight()) * 0.5F)},
                                        ImGui::GetColorU32(ImGuiCol_Text), text);
}

// Werte der Felder (Dialog Data, AddDialogData) und die Anzeige aus der Auswahl
struct SiZustand {
    float felder[5] = {0, 0, 0.5F, 0.5F, 0};   // H/V shift, H/V stretch, Rotate
    float einpassen[2] = {1, 1};                // m_fitHorizontal / m_fitVertical
    std::string textur;                         // ohne "textures/"
    std::size_t stand = static_cast<std::size_t>(-1);
    float raster = -1;
};
SiZustand g_si;

// Bereiche der fuenf Spinboxen (BuildDialog): Nachkommastellen, min, max, Umlauf
struct SpinArt {
    double min;
    double max;
    int stellen;
    bool umlauf;
};
const SpinArt kSpins[5] = {{-8192, 8192, 2, false}, {-8192, 8192, 2, false}, {-8192, 8192, 5, false}, {-8192, 8192, 5, false}, {-360, 360, 2, true}};

float& schrittVon(karte::SiWerte& w, int i) {
    switch (i) {
    case 0: return w.verschiebung[0];
    case 1: return w.verschiebung[1];
    case 2: return w.skalierung[0];
    case 3: return w.skalierung[1];
    default: return w.drehung;
    }
}

// SurfaceInspector::Update (surfacedialog.cpp:940-1003): die Felder aus der Auswahl
void ausAuswahl() {
    Editor& ed = editor();
    const karte::SiAnzeige a = karte::siAusAuswahl(ed.karte);
    const std::string& name = a.shader;
    const std::size_t praefix = std::strlen(karte::kTexturPraefix);
    // shader_is_texture / shader_get_textureName
    if (name.size() >= praefix && karte::shaderNamenGleich(name.substr(0, praefix), karte::kTexturPraefix)) {
        g_si.textur = name.substr(praefix);
    } else {
        g_si.textur.clear();
    }
    karte::Texdef ssr;
    if (a.patchModus) {
        karte::schiebSkalDrehAusPatch(ssr, a.projektion);
    } else {
        karte::schiebSkalDrehAusFlaeche(ed.karte.kontext.art(), ssr, a.projektion);
    }
    g_si.felder[0] = ssr.shift[0];
    g_si.felder[1] = ssr.shift[1];
    g_si.felder[2] = ssr.scale[0];
    g_si.felder[3] = ssr.scale[1];
    g_si.felder[4] = ssr.rotate;
}

// QAbstractSpinBox::stepBy mit Begrenzung bzw. Umlauf, gerundet auf die Stellen
float gestuft(float wert, float schritt, int n, const SpinArt& a) {
    double v = static_cast<double>(wert) + static_cast<double>(schritt) * n;
    if (a.umlauf) {
        if (v > a.max) { v = wert >= a.max ? a.min : a.max; }
        if (v < a.min) { v = wert <= a.min ? a.max : a.min; }
    } else {
        v = std::clamp(v, a.min, a.max);
    }
    const double f = std::pow(10.0, a.stellen);
    return static_cast<float>(std::round(v * f) / f);
}

void anwenden(int i, float wert) {   // ApplyTexdef_HShift ... ApplyTexdef_Rotation
    Editor& ed = editor();
    karte::texdefWertBefehl(ed.karte, ed.bearbeitung.einstellung, i, wert);
    ausAuswahl();
}

// NonModalSpinner mit SpinBoxLabel davor
void spinZeile(int i, const char* beschriftung, float y, float x0, float labelB, float spinX, float spinB) {
    Editor& ed = editor();
    ImGuiIO& io = ImGui::GetIO();
    static ImVec2 zugStart;
    static float summe = 0.0F;
    static const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
    const SpinArt& art = kSpins[i];
    const float schritt = schrittVon(ed.siWerte, i);
    const float feldH = px(22.0F);
    zeilenText(beschriftung, x0, y);
    ImGui::SetCursorScreenPos(bp(x0, y));
    ImGui::InvisibleButton((std::string("##sizug") + std::to_string(i)).c_str(), ImVec2{px(labelB), feldH});
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
        ImGui::SetMouseCursor(ImGui::IsItemActive() ? ImGuiMouseCursor_None : ImGuiMouseCursor_ResizeEW);
    }
    if (ImGui::IsItemActivated()) {
        zugStart = io.MousePos;
        summe = 0.0F;
    } else if (ImGui::IsItemActive() && (io.MouseDelta.x != 0.0F || io.MouseDelta.y != 0.0F)) {
        const float qt = std::max(1.0F, std::round(g_app->dpiScale));
        summe += (io.MousePos.x - zugStart.x) / qt;
        const int delta = static_cast<int>(summe / 20.0F);
        if (delta != 0) {
            summe -= static_cast<float>(delta) * 20.0F;
            float neu = 0;
            if (io.KeyShift) {
                neu = gestuft(g_si.felder[i], io.KeyCtrl ? schritt / 100.0F : schritt / 10.0F, delta, art);
            } else {
                neu = gestuft(g_si.felder[i], schritt, io.KeyCtrl ? delta * 10 : delta, art);
            }
            anwenden(i, neu);   // NonModalSpinner::stepBy: sofort
            if (!imTest) { ImGui::TeleportMousePos(zugStart); }
        }
    }
    const float pfeilB = px(16.0F);
    const ImVec2 fa = bp(spinX, y);
    ImGui::SetCursorScreenPos(fa);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::SetNextItemWidth(px(spinB) - pfeilB);
    char format[8];
    std::snprintf(format, sizeof(format), "%%.%df", art.stellen);
    float v = g_si.felder[i];
    ImGui::InputFloat((std::string("##siwert") + std::to_string(i)).c_str(), &v, 0.0F, 0.0F, format);
    ImGui::PopStyleVar();
    if (ImGui::IsItemDeactivatedAfterEdit() || (ImGui::IsItemActive() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)))) {
        anwenden(i, static_cast<float>(std::clamp(static_cast<double>(v), art.min, art.max)));   // editingFinished
    } else if (ImGui::IsItemActive()) {
        g_si.felder[i] = v;
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { anwenden(i, gestuft(g_si.felder[i], schritt, 1, art)); }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { anwenden(i, gestuft(g_si.felder[i], schritt, -1, art)); }
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
    for (int k = 0; k < 2; ++k) {
        const ImVec2 p0{fa.x + px(spinB) - pfeilB, fa.y + (k == 0 ? 0.0F : std::floor(feldH * 0.5F))};
        const ImVec2 p1{fa.x + px(spinB), k == 0 ? fa.y + std::floor(feldH * 0.5F) : fa.y + feldH};
        ImGui::SetCursorScreenPos(p0);
        if (ImGui::InvisibleButton((std::string(k == 0 ? "##sihoch" : "##sirunter") + std::to_string(i)).c_str(), ImVec2{p1.x - p0.x, p1.y - p0.y})) {
            anwenden(i, gestuft(g_si.felder[i], schritt, k == 0 ? 1 : -1, art));
        }
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
    if (ImGui::IsMouseHoveringRect(fa, ImVec2{fa.x + px(spinB), fa.y + feldH}) && io.MouseWheel != 0.0F) {
        const int n = static_cast<int>(io.MouseWheel);
        if (io.KeyShift) {
            anwenden(i, gestuft(g_si.felder[i], io.KeyCtrl ? schritt / 1000.0F : schritt / 10.0F, io.KeyCtrl ? n * 10 : n, art));
        } else {
            anwenden(i, gestuft(g_si.felder[i], schritt, io.KeyCtrl ? n * 10 : n, art));
        }
    }
}

// Increment: NonModalEntry fuer den Schritt
void schrittFeld(int i, float x, float y, float b) {
    Editor& ed = editor();
    float& s = schrittVon(ed.siWerte, i);
    const float feldH = px(22.0F);
    ImGui::SetCursorScreenPos(bp(x, y));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::SetNextItemWidth(px(b));
    float v = s;
    ImGui::InputFloat((std::string("##sischritt") + std::to_string(i)).c_str(), &v, 0.0F, 0.0F, "%g");
    ImGui::PopStyleVar();
    if (ImGui::IsItemDeactivatedAfterEdit()) {   // Increment::apply
        s = v;
        einstellungenSpeichern();
    }
}

bool knopf(const char* text, const char* id, float x, float y, float b, Str tipp = Str::Count) {
    const float feldH = px(22.0F);
    ImGui::SetCursorScreenPos(bp(x, y));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
    const bool r = ImGui::Button((std::string(text) + "##" + id).c_str(), ImVec2{px(b), feldH});
    ImGui::PopStyleVar();
    if (tipp != Str::Count && ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", trK(tipp)); }
    return r;
}

// DoubleSpinBox( 0, 1 << 9, 1, 3, 1 ) fuer das Einpassen (Dialog Data)
void einpassFeld(int i, float x, float y, float b) {
    const float feldH = px(22.0F);
    const float pfeilB = px(16.0F);
    const ImVec2 fa = bp(x, y);
    ImGui::SetCursorScreenPos(fa);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::SetNextItemWidth(px(b) - pfeilB);
    float v = g_si.einpassen[i];
    if (ImGui::InputFloat((std::string("##sieinpassen") + std::to_string(i)).c_str(), &v, 0.0F, 0.0F, "%.3f")) {
        g_si.einpassen[i] = std::clamp(v, 0.0F, 512.0F);
    }
    ImGui::PopStyleVar();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
    for (int k = 0; k < 2; ++k) {
        const ImVec2 p0{fa.x + px(b) - pfeilB, fa.y + (k == 0 ? 0.0F : std::floor(feldH * 0.5F))};
        const ImVec2 p1{fa.x + px(b), k == 0 ? fa.y + std::floor(feldH * 0.5F) : fa.y + feldH};
        ImGui::SetCursorScreenPos(p0);
        if (ImGui::InvisibleButton((std::string(k == 0 ? "##sieinhoch" : "##sieinrunter") + std::to_string(i)).c_str(), ImVec2{p1.x - p0.x, p1.y - p0.y})) {
            g_si.einpassen[i] = std::clamp(g_si.einpassen[i] + (k == 0 ? 1.0F : -1.0F), 0.0F, 512.0F);
        }
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
}

// SurfaceInspector_ProjectTexture( type, isGuiClick = true )
void projizieren(int art) {
    Editor& ed = editor();
    karte::Texdef td;
    td.shift[0] = g_si.felder[0];
    td.shift[1] = g_si.felder[1];
    td.scale[0] = g_si.felder[2];
    td.scale[1] = g_si.felder[3];
    td.rotate = g_si.felder[4];
    karte::Vektor3f richtung(0, 0, 0);
    if (art == 1) {   // g_vector3_axes[GlobalXYWnd_getCurrentViewType()]
        const int typ = static_cast<int>(ed.xy[static_cast<std::size_t>(ed.aktivXy)].ansicht.typ);
        richtung = karte::Vektor3f(typ == 0 ? 1.0F : 0.0F, typ == 1 ? 1.0F : 0.0F, typ == 2 ? 1.0F : 0.0F);
    } else if (art == 2) {   // -Camera_getViewVector
        richtung = -ed.kamera.vpn;
    }
    karte::Protokoll p;
    karte::texturProjizierenBefehl(ed.karte, ed.bearbeitung.einstellung, td, art, richtung, p);
    konsoleProtokoll(p);
    ausAuswahl();
}

}  // namespace

// Increments Match Grid (SurfaceInspector_GridChange) und die Anzeige nach jeder Aenderung
void siAktualisieren() {
    Editor& ed = editor();
    if (g_si.raster != ed.raster.groesse()) {
        g_si.raster = ed.raster.groesse();
        if (ed.siWerte.rasterAnpassen) {   // DoSnapTToGrid( default scale, default scale )
            karte::schritteAmRaster(ed.siWerte, g_si.raster, karte::standardTexturSkalierung(), karte::standardTexturSkalierung());
        }
    }
}

// SurfaceInspector_FitTexture usw. mit den Feldern m_fitHorizontal/m_fitVertical
void siEinpassenBefehl(int art) {
    Editor& ed = editor();
    karte::texturEinpassenBefehl(ed.karte, ed.bearbeitung.einstellung, g_si.einpassen[0], g_si.einpassen[1], art);
    ausAuswahl();
}

// SurfaceInspector_ProjectTexture( type, isGuiClick = false ): scale = Standardskalierung
void siProjizierenBefehl(int art) {
    Editor& ed = editor();
    karte::Texdef td;
    td.scale[0] = td.scale[1] = karte::standardTexturSkalierung();
    karte::Vektor3f richtung(0, 0, 0);
    if (art == 1) {
        const int typ = static_cast<int>(ed.xy[static_cast<std::size_t>(ed.aktivXy)].ansicht.typ);
        richtung = karte::Vektor3f(typ == 0 ? 1.0F : 0.0F, typ == 1 ? 1.0F : 0.0F, typ == 2 ? 1.0F : 0.0F);
    } else if (art == 2) {
        richtung = -ed.kamera.vpn;
    }
    karte::Protokoll p;
    karte::texturProjizierenBefehl(ed.karte, ed.bearbeitung.einstellung, td, art, richtung, p);
    konsoleProtokoll(p);
}

void siUmschalten() {   // SurfaceInspector_toggleShown
    Editor& ed = editor();
    ed.siOffen = !ed.siOffen;
    if (ed.siOffen) {
        ausAuswahl();   // DoSurface: Update + ShowDlg
        g_si.stand = static_cast<std::size_t>(-1);
    }
}

void zeichneSurfaceInspector() {
    Editor& ed = editor();
    siAktualisieren();
    if (!ed.siOffen) { return; }
    // queueDraw: nach jeder Aenderung der Szene oder Auswahl neu lesen (ausser
    // ein Feld wird gerade bearbeitet)
    const std::size_t stand = ed.karte.undo.aenderungen() * 31U + ed.karte.auswahl.anzahl() * 7U + ed.karte.auswahl.flaechen().size() +
                              (ed.karte.auswahl.anzahl() != 0 ? reinterpret_cast<std::uintptr_t>(ed.karte.auswahl.elemente().back().primitiv.lock().get()) : 0U);
    if (stand != g_si.stand && !ImGui::IsAnyItemActive()) {
        g_si.stand = stand;
        ausAuswahl();
    }
    g_sk = massstab();
    dialogSchriftAn();
    const char* labels[5] = {trK(Str::KeSiHShift), trK(Str::KeSiVShift), trK(Str::KeSiHStretch), trK(Str::KeSiVStretch), trK(Str::KeSiRotate)};
    float labelB = 0.0F;
    for (const char* l : labels) { labelB = std::max(labelB, textBreite(l)); }
    const float d = std::max(0.0F, labelB - 86.0F);   // breitere Beschriftung: alles dahinter rueckt
    // die Gruppe "Texturing": Spalten nach den Texten
    const auto knopfB = [](Str s) { return textBreite(trK(s)) + 16.0F; };
    const float g0 = std::max({43.0F, textBreite(trK(Str::KeSiProject)), textBreite(trK(Str::KeSiPatch)), knopfB(Str::KeSiReset)});
    const float g1 = std::max({44.0F, knopfB(Str::KeSiFit), knopfB(Str::KeSiAxial), knopfB(Str::KeSiNatural)});
    const float g2 = std::max({67.0F, knopfB(Str::KeSiWidth), knopfB(Str::KeSiOrtho), knopfB(Str::KeSiCam)});
    const float g3 = std::max({67.0F, knopfB(Str::KeSiHeight)});
    const float gruppeB = 11.0F + g0 + 6.0F + g1 + 6.0F + g2 + 6.0F + g3 + 11.0F + 2.0F;   // mit Rahmen
    const float innenB = std::max(285.0F + d, 11.0F + gruppeB - 1.0F + 11.0F);
    const float rechts = innenB - 11.0F;
    const float innenH = 364.0F;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    ImGui::SetNextWindowSize(ImVec2{px(innenB), px(innenH) + titelH}, ImGuiCond_Always);
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_FirstUseEver, ImVec2{0.5F, 0.5F});
    const std::string titel = std::string(trK(Str::KeSiTitle)) + "###kartesi";
    bool offen = true;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool sichtbar = ImGui::Begin(titel.c_str(), &offen,
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    if (sichtbar) {
        g_innen = ImGui::GetCursorScreenPos();
        // Texture (NonModalEntry)
        const char* texturText = trK(Str::KeSiTexture);
        zeilenText(texturText, 11.0F, 11.0F);
        const float tx = 11.0F + textBreite(texturText) + 6.0F;
        {
            const float feldH = px(22.0F);
            ImGui::SetCursorScreenPos(bp(tx, 11.0F));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
            ImGui::SetNextItemWidth(px(rechts - tx));
            char puffer[256];
            std::snprintf(puffer, sizeof(puffer), "%s", g_si.textur.c_str());
            if (ImGui::InputText("##sitextur", puffer, sizeof(puffer)) && ImGui::IsItemActive()) { g_si.textur = puffer; }
            ImGui::PopStyleVar();
            if (ImGui::IsItemDeactivatedAfterEdit()) {   // ApplyShader
                std::string name = std::string(karte::kTexturPraefix) + puffer;
                std::replace(name.begin(), name.end(), '\\', '/');   // PathCleaned
                if (!karte::texturNameGueltig(name)) {
                    fehler("invalid texture name '" + name + "'");
                } else {
                    (void)karte::shaderSetzen(ed.karte, ed.bearbeitung.einstellung, name);
                }
                ausAuswahl();
            }
        }
        const float spinX = 124.0F + d;
        for (int i = 0; i < 5; ++i) {
            const float y = 39.0F + static_cast<float>(i) * 28.0F;
            spinZeile(i, labels[i], y, 11.0F, labelB, spinX, 89.0F);
            zeilenText(trK(Str::KeSiStep), 219.0F + d, y);
            schrittFeld(i, 247.0F + d, y, rechts - (247.0F + d));
        }
        // "-": Invert horizontally / vertically
        if (knopf("-", "siumkehrenh", 103.0F + d, 95.0F, 15.0F)) {
            karte::texturUmkehrenBefehl(ed.karte, ed.bearbeitung.einstellung, true, ed.siWerte, g_si.felder[0], g_si.felder[2]);
            ausAuswahl();
        }
        if (knopf("-", "siumkehrenv", 103.0F + d, 123.0F, 15.0F)) {
            karte::texturUmkehrenBefehl(ed.karte, ed.bearbeitung.einstellung, false, ed.siWerte, g_si.felder[1], g_si.felder[3]);
            ausAuswahl();
        }
        // Match Grid (OnBtnMatchGrid)
        if (knopf(trK(Str::KeSiMatchGrid), "simatchgrid", rechts - 80.0F, 179.0F, 80.0F)) {
            if (g_si.felder[2] == 0 || g_si.felder[3] == 0) {
                fehler("ERROR: unexpected scale == 0.0f");
            } else {
                karte::schritteAmRaster(ed.siWerte, ed.raster.groesse(), g_si.felder[2], g_si.felder[3]);
                einstellungenSpeichern();
            }
        }
        // Gruppe "Texturing"
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            zeilenText(trK(Str::KeSiTexturing), 11.0F, 204.0F);
            dl->AddRect(bp(12.0F, 225.0F), bp(rechts, 353.0F), ImGui::GetColorU32(ImGuiCol_Border), 2.0F);
            const float c0 = 23.0F;
            const float c1 = c0 + g0 + 6.0F;
            const float c2 = c1 + g1 + 6.0F;
            const float c3 = c2 + g2 + 6.0F;
            if (knopf(trK(Str::KeSiWidth), "siwidth", c2, 235.0F, g2, Str::KeSiWidthTip)) {
                karte::texturEinpassenBefehl(ed.karte, ed.bearbeitung.einstellung, g_si.einpassen[0], g_si.einpassen[1], 1);
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {   // RightClick: fit width, keep height
                karte::texturEinpassenBefehl(ed.karte, ed.bearbeitung.einstellung, g_si.einpassen[0], g_si.einpassen[1], 3);
            }
            if (knopf(trK(Str::KeSiHeight), "siheight", c3, 235.0F, g3, Str::KeSiHeightTip)) {
                karte::texturEinpassenBefehl(ed.karte, ed.bearbeitung.einstellung, g_si.einpassen[0], g_si.einpassen[1], 2);
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                karte::texturEinpassenBefehl(ed.karte, ed.bearbeitung.einstellung, g_si.einpassen[0], g_si.einpassen[1], 4);
            }
            if (knopf(trK(Str::KeSiReset), "sireset", c0, 263.0F, g0)) {   // SurfaceInspector_ResetTexture
                karte::texturZuruecksetzenKappen(ed.karte, ed.bearbeitung.einstellung);
            }
            if (knopf(trK(Str::KeSiFit), "sifit", c1, 263.0F, g1)) {   // SurfaceInspector_FitTexture
                karte::texturEinpassenBefehl(ed.karte, ed.bearbeitung.einstellung, g_si.einpassen[0], g_si.einpassen[1], 0);
            }
            einpassFeld(0, c2, 263.0F, g2);
            einpassFeld(1, c3, 263.0F, g3);
            zeilenText(trK(Str::KeSiProject), c0, 291.0F);
            if (knopf(trK(Str::KeSiAxial), "siaxial", c1, 291.0F, g1, Str::KeSiAxialTip)) { projizieren(0); }
            if (knopf(trK(Str::KeSiOrtho), "siortho", c2, 291.0F, g2, Str::KeSiOrthoTip)) { projizieren(1); }
            if (knopf(trK(Str::KeSiCam), "sicam", c3, 291.0F, g3, Str::KeSiCamTip)) { projizieren(2); }
            zeilenText(trK(Str::KeSiPatch), c0, 319.0F);
            if (knopf(trK(Str::KeSiNatural), "sinatural", c1, 319.0F, g1)) {   // Patch_NaturalTexture
                karte::patchBefehl(ed.karte, karte::PatchBefehl::TexturNatuerlich);
            }
        }
        // g_pressedKeysFilter: Return, Enter, Esc, Tab und ohne Zusatztasten
        // Pfeil hoch/runter, Bild hoch/runter gehen nicht an die Kuerzel
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            const ImGuiIO& io = ImGui::GetIO();
            const bool ohne = !io.KeyCtrl && !io.KeyShift && !io.KeyAlt;
            if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false) || ImGui::IsKeyPressed(ImGuiKey_Escape, false) ||
                ImGui::IsKeyPressed(ImGuiKey_Tab, false) ||
                (ohne && (ImGui::IsKeyPressed(ImGuiKey_UpArrow) || ImGui::IsKeyPressed(ImGuiKey_DownArrow) || ImGui::IsKeyPressed(ImGuiKey_PageUp) ||
                          ImGui::IsKeyPressed(ImGuiKey_PageDown)))) {
                ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
            }
        }
        ImGui::SetCursorScreenPos(bp(0.0F, innenH));
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
    }
    ImGui::End();
    dialogSchriftAus();
    if (!offen) { ed.siOffen = false; }
}

}  // namespace bhed::gui::karteneditor
