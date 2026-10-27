// app_karte_portale.cpp - Karten-Editor, Stufe 3 Punkt 5: der Portal Viewer
// (das Plugin prtview von NetRadiant-Custom) mit Oberflaeche.
//
// Plugins > prtview: Load .prt file (Dialog "Load .prt": Datei, Show 3D, Show
// 2D), Unload .prt file, Toggle portals (3D) / (2D), Configure Portal Viewer
// (der Dialog "Portal Viewer Configuration", nicht modal, Lagen wie im Foto
// dlg_Plugins_prtview_Configure_Portal_Viewer_0.png), About Portal Viewer.
// Die Portale stehen als Linien in den 2D-Ansichten und als halb
// durchsichtige Polygone in Zufallsfarben mit Umriss in der Kamera.
//
// Portiert aus NetRadiant-Custom 20260114: contrib/prtview/prtview.cpp
// (QERPlug_Dispatch), ConfigDialog.cpp, LoadPortalFileDialog.cpp,
// AboutDialog.cpp, portals.cpp (CPortalsRender, CPortalsDrawSolid,
// CPortalsDrawSolidOutline, CPortalsDrawWireframe, calculateCubicClipVolume,
// Portals_constructShaders).
//
//   PrtView plugin for GtkRadiant
//   Copyright (C) 2001 Geoffrey Dewan, Loki software and qeradiant.com
//
//   Urspruenglich unter der GNU Lesser General Public License 2.1 (oder
//   spaeter); nach deren Abschnitt 3 hier unter der GNU General Public
//   License, Version 2:
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the
//   Free Software Foundation; version 2 of the License. This program is
//   distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
//   without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//   PARTICULAR PURPOSE. See the GNU General Public License for more details
//   (LICENSE).
#include "karteneditor.h"

#include "app_internal.h"
#include "karte_gpu.h"
#include "platform.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace bhed::gui::karteneditor {

namespace {

Editor& e() { return editor(); }

karte::PortalZufall g_zufall;   // rand() der Laufzeit, nur fuer die Portalfarben

std::filesystem::path pfadVon(const std::string& p) {
    const std::u8string u(p.begin(), p.end());
    return std::filesystem::path(u);
}

bool dateiLesen(const std::string& pfad, std::string& text) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(pfadVon(pfad), ec)) { return false; }
    std::ifstream f(pfadVon(pfad), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    text = s.str();
    return true;
}

std::string ohneEndung(const std::string& pfad) {   // PathExtensionless
    const std::size_t punkt = pfad.rfind('.');
    const std::size_t trenner = pfad.find_last_of("/\\");
    if (punkt == std::string::npos || (trenner != std::string::npos && punkt < trenner)) { return pfad; }
    return pfad.substr(0, punkt);
}

void meldung(const std::string& text) { ausgabe(std::string(karte::kPortalMeldungVorn) + text); }

// CPortals::Load (portals.fn)
void portaleLaden() {
    Editor& ed = e();
    std::string text;
    const bool da = dateiLesen(ed.portalDatei, text);
    const auto m = karte::portaleLaden(ed.portalDatei, da ? &text : nullptr, ed.portale, ed.portalHintFlags, g_zufall);
    for (const karte::PortalMeldung& x : m) {
        konsoleSchreiben(x.art == 2 ? KonsolenZeile::Art::Fehler : KonsolenZeile::Art::Ausgabe, x.text);
    }
    ++ed.sichtStand;   // SceneChangeNotify
}

// --- Dialog "Load .prt" (LoadPortalFileDialog.cpp) ----------------------------------

struct LadenDialog {
    bool offen = false;
    char datei[1024] = "";
    bool zeige3d = true;
    bool zeige2d = false;
};
LadenDialog g_laden;

// --- Dialog "Portal Viewer Configuration" (ConfigDialog.cpp) ------------------------
//
// SetFixedSize: alle Lagen fest, gemessen im Foto (Massstab 2, Innenraum
// 386 x 510 logische Punkte). Schieber wie QSlider (Fusion): Griff 13 x 13,
// Rinne 5 hoch, Lage nach QStyle::sliderPositionFromValue.

constexpr int kDialogB = 386;
constexpr int kDialogH = 510;
constexpr int kHaken = 13;
constexpr int kGriff = 13;

float g_sk = 1.0F;
ImVec2 g_o{0.0F, 0.0F};
ImVec2 bp(float x, float y) { return ImVec2{std::round(g_o.x + x * g_sk), std::round(g_o.y + y * g_sk)}; }
float px(float l) { return std::round(l * g_sk); }
ImU32 farbe(ImGuiCol c) { return ImGui::GetColorU32(c); }

// QStyle::sliderPositionFromValue (qstyle.cpp)
int schieberLage(int min, int max, int wert, int spanne) {
    if (spanne <= 0 || wert < min || max <= min) { return 0; }
    if (wert > max) { return spanne; }
    const unsigned bereich = static_cast<unsigned>(max - min);
    const unsigned p = static_cast<unsigned>(wert - min);
    if (bereich > static_cast<unsigned>(spanne)) {
        return static_cast<int>((2U * p * static_cast<unsigned>(spanne) + bereich) / (2U * bereich));
    }
    const unsigned teil = static_cast<unsigned>(spanne) / bereich;
    const unsigned rest = static_cast<unsigned>(spanne) % bereich;
    return static_cast<int>(p * teil + (2U * p * rest + bereich) / (2U * bereich));
}

// QStyle::sliderValueFromPosition (qstyle.cpp)
int schieberWert(int min, int max, int lage, int spanne) {
    if (spanne <= 0 || lage <= 0) { return min; }
    if (lage >= spanne) { return max; }
    const unsigned bereich = static_cast<unsigned>(max - min);
    if (static_cast<unsigned>(spanne) > bereich) {
        return min + static_cast<int>((2U * static_cast<unsigned>(lage) * bereich + static_cast<unsigned>(spanne)) / (2U * static_cast<unsigned>(spanne)));
    }
    const unsigned teil = bereich / static_cast<unsigned>(spanne);
    const unsigned rest = bereich % static_cast<unsigned>(spanne);
    return min + static_cast<int>(static_cast<unsigned>(lage) * teil +
                                  (2U * static_cast<unsigned>(lage) * rest + static_cast<unsigned>(spanne)) / (2U * static_cast<unsigned>(spanne)));
}

struct KonfigDialog {
    bool offen = false;
    int greifer = -1;          // der Schieber, dessen Griff gezogen wird
    float griffVersatz = 0.0F;
    ImVec2 ursprung{0.0F, 0.0F};   // fuer den Selbsttest: die Ecke im letzten Bild
    float massstab = 1.0F;
};
KonfigDialog g_konfig;

void textBei(float x, float mitteY, const char* text, bool aktiv) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = ImGui::GetTextLineHeight();
    const ImVec2 p = bp(x, mitteY);
    dl->AddText(ImVec2{p.x, std::round(p.y - h * 0.5F)}, farbe(aktiv ? ImGuiCol_Text : ImGuiCol_TextDisabled), text);
}

// QCheckBox bzw. der Titel einer ankreuzbaren QGroupBox: Kaestchen 13 x 13 bei
// (x, y), Text 6 dahinter, senkrecht mittig
bool haken(const char* id, const char* text, int x, int y, bool& wert, bool aktiv, const char* feld) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float tb = ImGui::CalcTextSize(text).x / g_sk;
    merkeFeld(feld, x, y, kHaken, kHaken);
    ImGui::SetCursorScreenPos(bp(static_cast<float>(x), static_cast<float>(y)));
    bool geaendert = false;
    if (ImGui::InvisibleButton(id, ImVec2{px(static_cast<float>(kHaken + 6) + tb), px(static_cast<float>(kHaken))}) && aktiv) {
        wert = !wert;
        geaendert = true;
    }
    const bool ueber = aktiv && ImGui::IsItemHovered();
    const ImVec2 a = bp(static_cast<float>(x), static_cast<float>(y));
    const ImVec2 b = bp(static_cast<float>(x + kHaken), static_cast<float>(y + kHaken));
    dl->AddRectFilled(a, b, farbe(ueber ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg));
    dl->AddRect(a, b, farbe(ImGuiCol_Border));
    if (wert) {   // das Haekchen
        const ImU32 c = aktiv ? farbe(ImGuiCol_CheckMark) : farbe(ImGuiCol_TextDisabled);
        const float t = std::max(1.0F, px(1.6F));
        dl->AddLine(bp(x + 3.0F, y + 6.5F), bp(x + 5.5F, y + 9.5F), c, t);
        dl->AddLine(bp(x + 5.5F, y + 9.5F), bp(x + 10.0F, y + 3.0F), c, t);
    }
    textBei(static_cast<float>(x + kHaken + 6), static_cast<float>(y) + kHaken * 0.5F, text, aktiv);
    return geaendert;
}

// Der Rahmen einer QGroupBox (Fusion: eine helle, eine dunkle Linie)
void rahmen(int x, int y, int b, int h, const char* feld) {
    merkeFeld(feld, x, y, b, h);
    ImGui::GetWindowDrawList()->AddRect(bp(static_cast<float>(x), static_cast<float>(y)), bp(static_cast<float>(x + b), static_cast<float>(y + h)),
                                        farbe(ImGuiCol_Border));
}

// QSlider waagerecht: Widget x .. x + b, Griff und Rinne mittig um mitteY
bool schieber(int nr, int x, float mitteY, int b, int& wert, int min, int max, bool aktiv, const char* feld) {
    KonfigDialog& d = g_konfig;
    const int spanne = b - kGriff;
    merkeFeld(feld, x, static_cast<int>(std::lround(mitteY - kGriff * 0.5F)), b, kGriff);
    ImGui::SetCursorScreenPos(bp(static_cast<float>(x), mitteY - kGriff * 0.5F));
    const std::string id = "##prtschieber" + std::to_string(nr);
    ImGui::InvisibleButton(id.c_str(), ImVec2{px(static_cast<float>(b)), px(static_cast<float>(kGriff))},
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
    const int alt = wert;
    int lage = schieberLage(min, max, wert, spanne);
    if (aktiv) {
        const ImGuiIO& io = ImGui::GetIO();
        const float mx = (io.MousePos.x - g_o.x) / g_sk - static_cast<float>(x);
        if (ImGui::IsItemActivated()) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {   // SH_Slider_AbsoluteSetButtons
                wert = schieberWert(min, max, static_cast<int>(std::lround(mx - kGriff * 0.5F)), spanne);
            } else if (mx >= static_cast<float>(lage) && mx < static_cast<float>(lage + kGriff)) {
                d.greifer = nr;   // den Griff gefasst
                d.griffVersatz = mx - static_cast<float>(lage);
            } else {   // SH_Slider_PageSetButtons: ein Seitenschritt (10) zur Maus
                wert = std::clamp(wert + (mx < static_cast<float>(lage) ? -10 : 10), min, max);
            }
        }
        if (d.greifer == nr) {
            if (ImGui::IsItemActive()) {
                wert = schieberWert(min, max, static_cast<int>(std::lround(mx - d.griffVersatz)), spanne);
            } else {
                d.greifer = -1;
            }
        }
        if (ImGui::IsItemHovered() && io.MouseWheel != 0.0F) {   // QAbstractSlider::wheelEvent: 3 Zeilen je Raste
            wert = std::clamp(wert + std::clamp(static_cast<int>(io.MouseWheel * 3.0F), -10, 10), min, max);
        }
        lage = schieberLage(min, max, wert, spanne);
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // Rinne: 5 hoch, bis zur Mitte des Griffs gefuellt
    const float r0 = mitteY - 2.5F;
    dl->AddRectFilled(bp(static_cast<float>(x), r0), bp(static_cast<float>(x + b), r0 + 5.0F), farbe(ImGuiCol_FrameBg));
    dl->AddRectFilled(bp(static_cast<float>(x), r0), bp(static_cast<float>(x + lage) + kGriff * 0.5F, r0 + 5.0F),
                      aktiv ? farbe(ImGuiCol_SliderGrab) : farbe(ImGuiCol_TextDisabled));
    dl->AddRect(bp(static_cast<float>(x), r0), bp(static_cast<float>(x + b), r0 + 5.0F), farbe(ImGuiCol_Border));
    // Griff
    const ImVec2 g0 = bp(static_cast<float>(x + lage), mitteY - kGriff * 0.5F);
    const ImVec2 g1 = bp(static_cast<float>(x + lage + kGriff), mitteY + kGriff * 0.5F);
    dl->AddRectFilled(g0, g1, farbe(aktiv ? (d.greifer == nr ? ImGuiCol_ButtonActive : ImGuiCol_Button) : ImGuiCol_FrameBg), px(2.0F));
    dl->AddRect(g0, g1, farbe(ImGuiCol_Border), px(2.0F));
    return wert != alt;
}

bool farbKnopf(int x, int y, bool aktiv, const char* feld, const char* id) {
    merkeFeld(feld, x, y, 80, 22);
    ImGui::SetCursorScreenPos(bp(static_cast<float>(x), static_cast<float>(y)));
    ImGui::BeginDisabled(!aktiv);
    const bool k = ImGui::Button((std::string(trK(Str::KePrtFarbe)) + id).c_str(), ImVec2{px(80.0F), px(22.0F)});
    ImGui::EndDisabled();
    return k;
}

// OnColor: der Farbdialog, dann RGB_PACK( color * 255 ), Portals_shadersChanged
void farbeWaehlen(karte::GepackteFarbe* ziel) {
    const karte::GepackteFarbe c = *ziel;
    farbDialogOeffnen(karte::Vektor3f(static_cast<float>(c & 0xFFU) / 255.0F, static_cast<float>((c >> 8) & 0xFFU) / 255.0F,
                                      static_cast<float>((c >> 16) & 0xFFU) / 255.0F),
                      [ziel](const karte::Vektor3f& f) {
                          *ziel = karte::rgbPacken(static_cast<int>(f[0] * 255.0F), static_cast<int>(f[1] * 255.0F), static_cast<int>(f[2] * 255.0F));
                          e().portalE.farbenSetzen();
                          ++e().sichtStand;
                      });
}

void konfigZeichnen() {
    Editor& ed = e();
    KonfigDialog& d = g_konfig;
    if (!ed.portalKonfigOffen) {
        d = KonfigDialog{};
        return;
    }
    karte::PortalEinstellungen& p = ed.portalE;
    g_sk = massstab();
    const char* kKennung = "###karteprtkonfig";
    const std::string titel = std::string(trK(Str::KePrtKonfigTitel)) + kKennung;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!d.offen) {
        d.offen = true;
        ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_Always, ImVec2{0.5F, 0.5F});
    }
    ImGui::SetNextWindowSize(ImVec2{px(static_cast<float>(kDialogB)), px(static_cast<float>(kDialogH)) + titelH}, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    bool offen = true;
    const bool sichtbar = ImGui::Begin(titel.c_str(), &offen,
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();
    if (sichtbar) {
        g_o = ImGui::GetCursorScreenPos();
        d.ursprung = g_o;
        d.massstab = g_sk;
        merkeFeld("Prt:Dialog", 0, 0, kDialogB, kDialogH);
        dialogSchriftAn();
        bool neu = false;
        // 3D View (vbox_add_group: ankreuzbare QGroupBox, aus = Inhalt gesperrt)
        neu |= haken("##prt3d", trK(Str::KePrt3d), 12, 11, p.zeige3d, true, "Prt:3DHaken");
        rahmen(12, 29, 363, 328, "Prt:3D");
        const bool a3 = p.zeige3d;
        // Lines: Width = (1..10), Color
        neu |= haken("##prtlinien", trK(Str::KePrtLinien), 24, 40, p.linien, a3, "Prt:LinesHaken");
        rahmen(24, 59, 339, 65, "Prt:Lines");
        const bool aL = a3 && p.linien;
        char z[64];
        std::snprintf(z, sizeof(z), "%s%d", trK(Str::KePrtBreite), p.breite3d);
        textBei(35, 76.5F, z, aL);
        neu |= schieber(0, 97, 76.5F, 253, p.breite3d, 1, 10, aL, "Prt:LinesWidth");
        if (farbKnopf(271, 90, aL, "Prt:LinesColor", "##prtfarbe3d")) { farbeWaehlen(&p.farbe3d); }
        // Polygons: Opacity = (0..100) %
        neu |= haken("##prtpolygone", trK(Str::KePrtPolygone), 24, 130, p.polygone, a3, "Prt:PolygonsHaken");
        rahmen(24, 149, 339, 36, "Prt:Polygons");
        const bool aP = a3 && p.polygone;
        std::snprintf(z, sizeof(z), "%s%d%%", trK(Str::KePrtDeckkraft), p.deckkraft);
        textBei(35, 166.5F, z, aP);
        neu |= schieber(1, 122, 166.5F, 228, p.deckkraft, 0, 100, aP, "Prt:Opacity");
        // Fog: Color
        neu |= haken("##prtnebel", trK(Str::KePrtNebel), 24, 192, p.nebel, a3, "Prt:FogHaken");
        rahmen(24, 211, 339, 43, "Prt:Fog");
        if (farbKnopf(271, 221, a3 && p.nebel, "Prt:FogColor", "##prtfarbenebel")) { farbeWaehlen(&p.farbeNebel); }
        // Cubic clipper: Clip range = (64..8192)
        neu |= haken("##prtausschnitt", trK(Str::KePrtAusschnitt), 24, 261, p.ausschnitt, a3, "Prt:ClipHaken");
        rahmen(24, 280, 339, 36, "Prt:Clip");
        const bool aC = a3 && p.ausschnitt;
        std::snprintf(z, sizeof(z), "%s%d", trK(Str::KePrtWeite), p.ausschnittWeite);
        textBei(35, 297.5F, z, aC);
        neu |= schieber(2, 130, 297.5F, 220, p.ausschnittWeite, 64, 8192, aC, "Prt:ClipRange");
        // Z-Buffer (ComboBox)
        {
            const char* const eintraege[3] = {trK(Str::KePrtZTestSchreiben), trK(Str::KePrtZTest), trK(Str::KePrtZAus)};
            merkeFeld("Prt:ZBuffer", 23, 323, 340, 22);
            ImGui::SetCursorScreenPos(bp(23, 323));
            ImGui::SetNextItemWidth(px(340.0F));
            ImGui::BeginDisabled(!a3);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                                ImVec2{px(4.0F), std::max(0.0F, (px(22.0F) - ImGui::GetTextLineHeight()) * 0.5F)});
            if (ImGui::BeginCombo("##prtzpuffer", eintraege[std::clamp(p.zPuffer, 0, 2)])) {
                for (int i = 0; i < 3; ++i) {
                    if (ImGui::Selectable(eintraege[i], p.zPuffer == i)) {
                        p.zPuffer = i;
                        neu = true;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopStyleVar();
            ImGui::EndDisabled();
        }
        // Draw Hint Portals / Draw Regular Portals (mittig)
        neu |= haken("##prthints", trK(Str::KePrtHints), 138, 366, p.hints, true, "Prt:Hints");
        neu |= haken("##prtregulaer", trK(Str::KePrtRegulaer), 129, 392, p.nichtHints, true, "Prt:Regular");
        // 2D View Lines: Width = (1..10), Color
        neu |= haken("##prt2d", trK(Str::KePrt2d), 12, 415, p.zeige2d, true, "Prt:2DHaken");
        rahmen(12, 433, 363, 66, "Prt:2D");
        const bool a2 = p.zeige2d;
        std::snprintf(z, sizeof(z), "%s%d", trK(Str::KePrtBreite), p.breite2d);
        textBei(23, 451.5F, z, a2);
        neu |= schieber(3, 85, 451.5F, 277, p.breite2d, 1, 10, a2, "Prt:2DWidth");
        if (farbKnopf(283, 465, a2, "Prt:2DColor", "##prtfarbe2d")) { farbeWaehlen(&p.farbe2d); }
        dialogSchriftAus();
        if (neu) { ++ed.sichtStand; }   // SceneChangeNotify (Portals_shadersChanged: die Werte gelten beim Zeichnen)
    }
    ImGui::End();
    if (!offen) {   // WA_DeleteOnClose
        ed.portalKonfigOffen = false;
        d = KonfigDialog{};
    }
}

void ladenZeichnen() {
    Editor& ed = e();
    LadenDialog& d = g_laden;
    const char* kKennung = "###karteprtladen";
    const std::string titel = std::string(trK(Str::KePrtLadenTitel)) + kKennung;
    if (!ed.portalLadenOffen) {
        d.offen = false;
        return;
    }
    if (!d.offen) {
        d.offen = true;
        ImGui::OpenPopup(titel.c_str());
    }
    g_sk = massstab();
    // QVBoxLayout (Rand 11, Abstand 6): QLineEdit 22, zwei QCheckBox 17,
    // QDialogButtonBox 22 - Breite nach den Knoepfen (2 x 80 + 6)
    constexpr int kB = 188;
    constexpr int kH = 11 + 22 + 6 + 17 + 6 + 17 + 6 + 22 + 11;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowSize(ImVec2{px(static_cast<float>(kB)), px(static_cast<float>(kH)) + titelH}, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});   // QDialog::exec dunkelt nicht ab
    bool offen = true;
    const bool sichtbar = ImGui::BeginPopupModal(titel.c_str(), &offen,
                                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                                                     ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    if (!sichtbar) {
        if (d.offen) {
            ed.portalLadenOffen = false;
            d.offen = false;
            meldung("Portal file load aborted.");
        }
        return;
    }
    g_o = ImGui::GetCursorScreenPos();
    merkeFeld("PrtLaden:Dialog", 0, 0, kB, kH);
    dialogSchriftAn();
    int ergebnis = 0;   // 1 OK, 2 Cancel
    // QLineEdit mit der Aktion "Oeffnen" am Ende (SP_DialogOpenButton)
    {
        const int symbolB = 20;
        merkeFeld("PrtLaden:Datei", 11, 11, kB - 22, 22);
        ImGui::SetCursorScreenPos(bp(11, 11));
        ImGui::SetNextItemWidth(px(static_cast<float>(kB - 22 - symbolB)));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (px(22.0F) - ImGui::GetTextLineHeight()) * 0.5F)});
        ImGui::InputText("##prtdatei", d.datei, sizeof(d.datei));
        ImGui::PopStyleVar();
        merkeFeld("PrtLaden:Suchen", kB - 11 - symbolB, 11, symbolB, 22);
        ImGui::SetCursorScreenPos(bp(static_cast<float>(kB - 11 - symbolB), 11));
        if (ImGui::Button("...##prtsuchen", ImVec2{px(static_cast<float>(symbolB)), px(22.0F)})) {
            // m_pfnFileDialog( ..., "Locate portal (.prt) file", line->text(), "*.prt", ... )
            const std::string start = d.datei;
            const std::string f = kartenDateiDialog(true, "Locate portal (.prt) file", "*.prt|*.prt|*.*|*.*",
                                                    start.substr(0, start.find_last_of("/\\") == std::string::npos ? 0 : start.find_last_of("/\\")), "");
            if (!f.empty()) { std::snprintf(d.datei, sizeof(d.datei), "%s", f.c_str()); }
        }
    }
    {
        int y = 11 + 22 + 6;
        ImGui::SetCursorScreenPos(bp(11, static_cast<float>(y)));
        merkeFeld("PrtLaden:3D", 11, y, kB - 22, 17);
        (void)qtHaken((std::string(trK(Str::KePrtZeige3d)) + "##prtzeige3d").c_str(), &d.zeige3d, 17.0F);
        y += 17 + 6;
        ImGui::SetCursorScreenPos(bp(11, static_cast<float>(y)));
        merkeFeld("PrtLaden:2D", 11, y, kB - 22, 17);
        (void)qtHaken((std::string(trK(Str::KePrtZeige2d)) + "##prtzeige2d").c_str(), &d.zeige2d, 17.0F);
    }
    {   // QDialogButtonBox: OK, Cancel rechts
        const int y = kH - 11 - 22;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (px(22.0F) - ImGui::GetTextLineHeight()) * 0.5F)});
        merkeFeld("PrtLaden:OK", kB - 11 - 166, y, 80, 22);
        ImGui::SetCursorScreenPos(bp(static_cast<float>(kB - 11 - 166), static_cast<float>(y)));
        if (ImGui::Button((std::string(trK(Str::KePrefOk)) + "##prtok").c_str(), ImVec2{px(80.0F), px(22.0F)})) { ergebnis = 1; }
        merkeFeld("PrtLaden:Cancel", kB - 11 - 80, y, 80, 22);
        ImGui::SetCursorScreenPos(bp(static_cast<float>(kB - 11 - 80), static_cast<float>(y)));
        if (ImGui::Button((std::string(trK(Str::KeCancel)) + "##prtabbrechen").c_str(), ImVec2{px(80.0F), px(22.0F)})) { ergebnis = 2; }
        ImGui::PopStyleVar();
    }
    dialogSchriftAus();
    if (!offen) { ergebnis = 2; }
    if (ergebnis == 0 && ImGui::IsKeyPressed(ImGuiKey_Escape)) { ergebnis = 2; }
    if (ergebnis == 0 && ImGui::IsKeyPressed(ImGuiKey_Enter) && !ImGui::IsAnyItemActive()) { ergebnis = 1; }
    if (ergebnis != 0) {
        ImGui::CloseCurrentPopup();
        ed.portalLadenOffen = false;
        d.offen = false;
        if (ergebnis == 1) {   // dialog.exec(): fn, Purge, show_3d, show_2d; dann Load
            ed.portalDatei = d.datei;
            ed.portale.clear();
            ed.portalE.zeige3d = d.zeige3d;
            ed.portalE.zeige2d = d.zeige2d;
            portaleLaden();
        } else {
            meldung("Portal file load aborted.");
        }
    }
    ImGui::EndPopup();
}

}  // namespace

std::string portalIniPfad() { return platform::settingsDirectory() + "\\karte_prtview.ini"; }   // <Einstellungen>/prtview.ini

void portalEinstellungenLesen() {   // LoadConfig
    Editor& ed = e();
    if (ed.portalEinstellungenGelesen) { return; }
    ed.portalEinstellungenGelesen = true;
    karte::IniDatei ini;
    std::string text;
    if (dateiLesen(portalIniPfad(), text)) { ini.lesen(text); }
    ed.portalE = karte::portalEinstellungenAus(ini);
}

void portalEinstellungenSchreiben() {   // SaveConfig (PrtView_destroy)
    portalEinstellungenLesen();
    const std::string text = karte::portalEinstellungenIni(e().portalE).schreiben();
    std::string crlf;   // TextFileOutputStream: fopen( name, "wt" )
    for (const char c : text) {
        if (c == '\n') { crlf += '\r'; }
        crlf += c;
    }
    std::ofstream f(pfadVon(portalIniPfad()), std::ios::binary | std::ios::trunc);
    f.write(crlf.data(), static_cast<std::streamsize>(crlf.size()));
}

// QERPlug_Dispatch (prtview.cpp:185-245)
void portalBefehl(const std::string& befehl) {
    Editor& ed = e();
    portalEinstellungenLesen();
    meldung("Command \"" + befehl + "\"");   // Quoted( p )
    if (befehl == "About Portal Viewer") {   // DoAboutDlg
        qtMeldung("About Portal Viewer",
                  "Version 1.000\n\nGtk port by Leonardo Zide\nleo@lokigames.com\n\nWritten by Geoffrey DeWan\ngdewan@prairienet.org\n\n"
                  "Built against NetRadiant 1.6.0n-git-68ecbed\n(JK Studio, " __DATE__ ")",
                  MeldungArt::Info, false, nullptr);
    } else if (befehl == "Load .prt file") {   // DoLoadPortalFileDialog
        ed.portalDatei = ohneEndung(bauKartenPfad()) + ".prt";   // PathExtensionless( getMapName() ) + ".prt"
        std::snprintf(g_laden.datei, sizeof(g_laden.datei), "%s", ed.portalDatei.c_str());
        g_laden.zeige3d = ed.portalE.zeige3d;
        g_laden.zeige2d = ed.portalE.zeige2d;
        g_laden.offen = false;
        ed.portalLadenOffen = true;
    } else if (befehl == "Unload .prt file") {
        ed.portale.clear();   // portals.Purge()
        ++ed.sichtStand;
        meldung("Portals unloaded.");
    } else if (befehl == "Toggle portals (2D)") {
        ed.portalE.zeige2d = !ed.portalE.zeige2d;
        ++ed.sichtStand;
        meldung(ed.portalE.zeige2d ? "Portals will be rendered in 2D view." : "Portals will NOT be rendered in 2D view.");
    } else if (befehl == "Toggle portals (3D)") {
        ed.portalE.zeige3d = !ed.portalE.zeige3d;
        ++ed.sichtStand;
        meldung(ed.portalE.zeige3d ? "Portals will be rendered in 3D view." : "Portals will NOT be rendered in 3D view.");
    } else if (befehl == "Configure Portal Viewer") {   // DoConfigDialog (nicht modal, dialog->show())
        ed.portalKonfigOffen = true;
        g_konfig.offen = false;
        ++ed.sichtStand;
    }
}

ImVec2 portalKonfigPunkt(float x, float y) {
    return ImVec2{std::round(g_konfig.ursprung.x + x * g_konfig.massstab), std::round(g_konfig.ursprung.y + y * g_konfig.massstab)};
}

void portalDialogeZeichnen() {
    ladenZeichnen();
    konfigZeichnen();
}

std::vector<karte::Linie> portalLinien2d(float& breite) {
    const Editor& ed = e();
    std::vector<karte::Linie> l;
    const karte::PortalEinstellungen& p = ed.portalE;
    breite = static_cast<float>(p.breite2d);
    if (!p.zeige2d || ed.portale.empty()) { return l; }   // CPortalsRender::renderWireframe
    const karte::Farbe f = karte::farbeAus(p.fp2d[0], p.fp2d[1], p.fp2d[2], p.fp2d[3]);
    for (const karte::BspPortal& prt : ed.portale) {   // CPortalsDrawWireframe: GL_LINE_LOOP der Punkte
        for (std::size_t i = 0; i < prt.punkte.size(); ++i) { l.push_back(karte::Linie{prt.punkte[i], prt.punkte[(i + 1) % prt.punkte.size()], f}); }
    }
    return l;
}

void portaleKameraZeichnen(const float* matrix, const karte::Matrix4& viewproj, bool fuellen) {
    Editor& ed = e();
    const karte::PortalEinstellungen& p = ed.portalE;
    if (!p.zeige3d || ed.portale.empty()) { return; }   // CPortalsRender::renderSolid
    // calculateCubicClipVolume: das Auge aus der Umkehrung von viewproj (0, 0, -1)
    const karte::Matrix4 inv = karte::vollInvers(viewproj);
    const float vx = inv[8] * -1.0F + inv[0] * 0.0F + inv[4] * 0.0F + inv[12];
    const float vy = inv[9] * -1.0F + inv[1] * 0.0F + inv[5] * 0.0F + inv[13];
    const float vz = inv[10] * -1.0F + inv[2] * 0.0F + inv[6] * 0.0F + inv[14];
    const float vw = inv[11] * -1.0F + inv[3] * 0.0F + inv[7] * 0.0F + inv[15];
    const karte::Vektor3f cam(vx / vw, vy / vw, vz / vw);
    const karte::Vektor3f r(static_cast<float>(p.ausschnittWeite), static_cast<float>(p.ausschnittWeite), static_cast<float>(p.ausschnittWeite));
    const karte::Vektor3f cmin = cam + r;   // clip.min = cam + Vector3( clip_range ) (so in NRC)
    const karte::Vektor3f cmax = cam - r;
    const auto zeigen = [&](const karte::BspPortal& prt) {
        if (!((!prt.hint && p.nichtHints) || (prt.hint && p.hints))) { return false; }
        if (p.ausschnitt && (cmin[0] < prt.min[0] || cmin[1] < prt.min[1] || cmin[2] < prt.min[2] || cmax[0] > prt.max[0] || cmax[1] > prt.max[1] ||
                             cmax[2] > prt.max[2])) {
            return false;
        }
        return true;
    };
    kartegpu::Zustand3 basis;
    basis.matrix = matrix;
    basis.wegschneiden = false;
    if (p.nebel) {   // RENDER_FOG: GL_EXP, Dichte 0.001
        basis.nebel[0] = p.fpNebel[0];
        basis.nebel[1] = p.fpNebel[1];
        basis.nebel[2] = p.fpNebel[2];
        basis.nebel[3] = 0.001F;
    }
    const auto linienPunkt = [](const karte::Vektor3f& v, std::uint32_t farbe) {
        kartegpu::LinienPunkt q{};
        q.pos[0] = v[0];
        q.pos[1] = v[1];
        q.pos[2] = v[2];
        q.farbe = farbe;
        return q;
    };
    // Reihenfolge wie OpenGLState_less (gleiches m_sort eSortOverlayFirst): erst
    // der Umriss (kleinere Zustandsbits), dann die Polygone (RENDER_FILL | BLEND ...)
    if (p.linien) {   // $plugins/prtview/solid_outline: COLOURWRITE | DEPTHWRITE (+ Test je ZBuffer)
        kartegpu::Zustand3 z = basis;
        z.farbe[0] = p.fp3d[0];
        z.farbe[1] = p.fp3d[1];
        z.farbe[2] = p.fp3d[2];
        z.farbe[3] = p.fp3d[3];
        z.tiefe = p.zPuffer == 2 ? kartegpu::TiefenTest::Aus : kartegpu::TiefenTest::Kleiner;
        z.tiefeSchreiben = p.zPuffer != 2;   // ohne GL_DEPTH_TEST schreibt OpenGL keine Tiefe
        z.breite = static_cast<float>(p.breite3d);
        std::vector<kartegpu::LinienPunkt> l;
        for (const karte::BspPortal& prt : ed.portale) {   // CPortalsDrawSolidOutline: GL_LINE_LOOP der inneren Punkte
            if (!zeigen(prt)) { continue; }
            for (std::size_t i = 0; i < prt.innen.size(); ++i) {
                l.push_back(linienPunkt(prt.innen[i], 0xFFFFFFFFU));
                l.push_back(linienPunkt(prt.innen[(i + 1) % prt.innen.size()], 0xFFFFFFFFU));
            }
        }
        kartegpu::linien(l, z);
    }
    if (p.polygone) {   // $plugins/prtview/solid: FILL | BLEND | COLOURWRITE | COLOURCHANGE | SMOOTH (+ Tiefe je ZBuffer)
        kartegpu::Zustand3 z = basis;
        z.mischen = kartegpu::Mischen::Alpha;
        z.tiefe = p.zPuffer == 2 ? kartegpu::TiefenTest::Aus : kartegpu::TiefenTest::Kleiner;
        z.tiefeSchreiben = p.zPuffer == 0;
        const float deckkraft = static_cast<float>(p.deckkraft) / 100.0F;
        std::vector<const karte::BspPortal*> folge;
        for (karte::BspPortal& prt : ed.portale) { folge.push_back(&prt); }
        if (p.zPuffer != 0) {   // nach dem Abstand zum Auge (nahe zuerst, wie NRC)
            for (karte::BspPortal& prt : ed.portale) {
                const karte::Vektor3f d = cam - prt.mitte;
                prt.abstand = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];   // vector3_length_squared
            }
            std::sort(folge.begin(), folge.end(), [](const karte::BspPortal* a, const karte::BspPortal* b) { return a->abstand < b->abstand; });
        }
        std::vector<kartegpu::Punkt3> dreiecke;
        std::vector<kartegpu::LinienPunkt> kanten;
        for (const karte::BspPortal* prt : folge) {
            if (!zeigen(*prt)) { continue; }
            const karte::Farbe f = karte::farbeAus(prt->farbe[0], prt->farbe[1], prt->farbe[2], deckkraft);   // glColor4f
            const auto ecke = [&](const karte::Vektor3f& v) {
                kartegpu::Punkt3 q{};
                q.pos[0] = v[0];
                q.pos[1] = v[1];
                q.pos[2] = v[2];
                q.normale[2] = 1;
                q.farbe = f;
                return q;
            };
            const std::vector<karte::Vektor3f>& pt = prt->punkte;
            if (fuellen) {   // GL_POLYGON (konvex: ein Faecher)
                for (std::size_t i = 1; i + 1 < pt.size(); ++i) {
                    dreiecke.push_back(ecke(pt[0]));
                    dreiecke.push_back(ecke(pt[i]));
                    dreiecke.push_back(ecke(pt[i + 1]));
                }
            } else {   // ohne RENDER_FILL in der Darstellungsart: glPolygonMode GL_LINE
                for (std::size_t i = 0; i < pt.size(); ++i) {
                    kanten.push_back(linienPunkt(pt[i], f));
                    kanten.push_back(linienPunkt(pt[(i + 1) % pt.size()], f));
                }
            }
        }
        if (fuellen) {
            kartegpu::dreiecke(dreiecke, z);
        } else {
            kartegpu::linien(kanten, z);
        }
    }
}

}  // namespace bhed::gui::karteneditor
