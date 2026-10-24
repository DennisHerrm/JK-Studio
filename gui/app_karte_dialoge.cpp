// app_karte_dialoge.cpp - Karten-Editor: die Dialoge (Einstellungen, ...).
//
// Portiert aus NetRadiant-Custom (Stand master 2026): radiant/preferences.cpp
// (PrefsDlg "NetRadiant Preferences": Baum links, Seite rechts als
// QGroupBox, OK / Cancel / Clean; die festen Gruppen Global, Game,
// Interface, Display, Settings und die Unterseiten in der Reihenfolge ihrer
// Registrierung), radiant/mainframe.cpp (Layout_constructPreferences:
// Window Layout mit vier Bildern, Detachable Menus, Built-In Group Dialog).
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
#include "tastatur.h"

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>

namespace bhed::gui::karteneditor {

namespace {

// Eine Seite des Baums: Text im Baum, Rahmentitel, Tiefe (0 = Gruppe).
struct Seite {
    Str baum;
    Str titel;
    int tiefe;
};

// PrefsDlg::BuildDialog (preferences.cpp:604-694) und die Unterseiten in
// Registrierungsreihenfolge.
const Seite kSeiten[] = {
    {Str::KePgGlobal, Str::KePgGlobalT, 0},       {Str::KePgGame, Str::KePgGame, 1},
    {Str::KePgGame, Str::KePgGameSettingsT, 0},   {Str::KePgPaths, Str::KePgPathsT, 1},
    {Str::KePgInterface, Str::KePgInterfaceT, 0}, {Str::KePgLayout, Str::KePgLayoutT, 1},
    {Str::KePgDisplay, Str::KePgDisplayT, 0},     {Str::KePgTextures, Str::KePgTexturesT, 1},
    {Str::KePgPatches, Str::KePgPatchesT, 1},     {Str::KePgEntities, Str::KePgEntitiesT, 1},
    {Str::KePgSettings, Str::KePgSettingsT, 0},   {Str::KePgUndo, Str::KePgUndoT, 1},
    {Str::KePgSelection, Str::KePgSelectionT, 1}, {Str::KePgBrush, Str::KePgBrushT, 1},
    {Str::KePgClipper, Str::KePgClipperT, 1},     {Str::KePgGrid, Str::KePgGridT, 1},
    {Str::KePgSurface, Str::KePgSurfaceT, 1},     {Str::KePgCamera, Str::KePgCameraT, 1},
    {Str::KePgOrtho, Str::KePgOrthoT, 1},         {Str::KePgBuild, Str::KePgBuildT, 1},
    {Str::KePgTexBro, Str::KePgTexBroT, 1},       {Str::KePgModBro, Str::KePgModBroT, 1},
    {Str::KePgAutosave, Str::KePgAutosaveT, 1},
};
constexpr int kSeiteInterface = 4;
constexpr int kSeiteLayout = 5;
constexpr int kSeiteEntities = 9;
constexpr int kSeiteAuswahl = 12;
constexpr int kSeiteClipper = 14;
constexpr int kSeiteKamera = 17;
constexpr int kSeiteTexBro = 20;

struct Entwurf {
    bool offen = false;
    int seite = 0;   // PreferencesDialog: beim ersten Oeffnen die erste Seite (Global)
    int anordnung = 0;
    bool eingebaut = false;
    bool abtrennbar = true;
    bool nrc = false;
    karte::KameraEinstellungen kamera;
    int waehlEpsilon = 12;          // g_SELECT_EPSILON
    bool punktEntitaeten2d = true;  // m_bPreferPointEntsIn2D
    bool pinselIn3d = true;         // g_3DCreateBrushes
    int bewegStil3d = 0;            // TranslateFreeXY_Z::m_viewdependent
    karte::TexturBrowserEinstellungen texBro;   // TextureBrowser_constructPreferences
    int namenAbstand = 512;         // Entity_constructPreferences: g_showNamesDist
    int namenVerhaeltnis = 64;      // g_showNamesRatio
    bool zielnamen = false;         // g_showTargetNames
    // Clipper_constructPreferences (clippertool.cpp:163-180)
    bool clipperCaulk = true;
    bool clipperUmdrehen = true;
    bool clipperPunkte = true;
    bool clipper2Punkte = true;
    int clipperDoppelklick = 1;
};
Entwurf g_entwurf;

// --- Masse wie Qt 5.15 / Fusion (logische Punkte) -----------------------------
//
// Gemessen an pref_Camera.png und pref_Layout.png (Innenraum ab 11/45 im
// Foto, 2x) und mit Qts Werten belegt:
//   QGridLayout des Dialogs: Rand 11 (PM_DefaultTopLevelMargin), Abstand 6
//   (PM_DefaultLayoutSpacing); SetFixedSize: 597 x 492 (die groesste Seite)
//   Baum (QTreeView, AdjustToContents) 11, 11, 147 x 470; Zeilen 16 (Segoe
//   UI 9 pt: ItemViewFont = lfIconTitleFont), Einzug 20 (PM_TreeViewIndentation),
//   Text 3 nach dem Einzug (PM_FocusFrameHMargin + 1), Pfeil 8 x 4 in der Mitte
//   des ersten Einzugs (qt_fusion_draw_arrow)
//   Seite (QGroupBox) 164, 11, 422 x 442: Titel bei 166, Rahmen ab 28
//   (max(14, fm 13) + 3), Inhalt ab 39 und 176..574 (margin 3 + Layoutrand 9)
//   QDialogButtonBox: Zeile 459..481, Knoepfe 80 breit - Clean 164, OK 420, Cancel 506
// Die Widgets (Beschriftungen, Felder, Knoepfe, Titel) in Tahoma 8 pt
// (SystemFont = MS Shell Dlg 2), der Baum in Segoe UI 9 pt.
constexpr int kInnenB = 597;
constexpr int kInnenH = 492;
constexpr int kRand = 11;
constexpr int kAbstand = 6;
constexpr int kBaumB = 147;
constexpr int kBaumZeile = 16;
constexpr int kEinzug = 20;
constexpr int kTextRand = 3;
constexpr int kKnopfB = 80;
constexpr int kKnopfH = 22;
constexpr int kGruppeX = kRand + kBaumB + kAbstand;    // 164
constexpr int kGruppeB = kInnenB - kRand - kGruppeX;   // 422
constexpr int kKnopfY = kInnenH - kRand - kKnopfH;     // 459
constexpr int kGruppeH = kKnopfY - kAbstand - kRand;   // 442
constexpr int kRahmenOben = 17;    // PE_FrameGroupBox: topMargin
constexpr int kInhaltOben = 28;    // erste Zeile bei 39
constexpr int kInhaltRand = 12;
constexpr int kTitelX = 2;
constexpr int kFeldH = 22;         // QSpinBox, QComboBox, QLineEdit
constexpr int kHakenZeile = 20;    // QCheckBox
constexpr int kHaken = 14;         // PM_IndicatorWidth / PM_ExclusiveIndicatorWidth
constexpr int kBildB = 72;         // addRadioIcons: Bild mit Rahmen (halbe Groesse)
constexpr int kBildH = 40;

float g_sk = 1.0F;
ImVec2 g_innen{0.0F, 0.0F};   // Innenraum oben links (Bildpunkte)

ImVec2 bp(float x, float y) { return ImVec2{std::round(g_innen.x + x * g_sk), std::round(g_innen.y + y * g_sk)}; }
float px(float l) { return std::round(l * g_sk); }

// QLabel::sizeHint in Tahoma 8 pt (GDI wie Qt)
int labelBreite(const char* text) {
    const int b = platform::gdiTextBreite(text, L"Tahoma", 11, false);
    return b >= 0 ? b : static_cast<int>(std::lround(ImGui::CalcTextSize(text).x / g_sk));
}

// Eine Seite ist eine QGroupBox mit einem QGridLayout (PreferencePages_addPage,
// preferences.cpp:552-559: Spalten 111 : 333, oben ausgerichtet); jede
// Zeile ist DialogGrid_packRow (gtkutil/dialog.cpp:56-74): Beschriftung
// rechtsbuendig in Spalte 0, das Feld in Spalte 1, Abstand 6.
struct Form {
    int x0 = kGruppeX + kInhaltRand;   // 176
    int b0 = 98;
    int x1 = 280;
    int b1 = 294;
    int y = kRand + kInhaltOben;       // 39
};
Form g_form;

// qGeomCalc mit Dehnung 111 : 333: die Spalten im Verhaeltnis, aber
// wenigstens die breiteste Beschriftung bzw. das breiteste Feld (hint1).
void formAnfang(std::initializer_list<Str> beschriftungen, int hint1 = 0) {
    Form f;
    const int platz = kGruppeB - 2 * kInhaltRand - kAbstand;   // 392
    int hint0 = 0;
    for (Str s : beschriftungen) { hint0 = std::max(hint0, labelBreite(trK(s))); }
    int b0 = static_cast<int>(std::lround(static_cast<double>(platz) * 111.0 / 444.0));
    if (b0 < hint0) {
        b0 = hint0;
    } else if (platz - b0 < hint1) {
        b0 = platz - hint1;
    }
    f.b0 = b0;
    f.x1 = f.x0 + b0 + kAbstand;
    f.b1 = platz - b0;
    g_form = f;
}

// Neue Zeile der Hoehe h; die Beschriftung senkrecht mittig (QLabel AlignVCenter).
void formZeile(Str text, int h) {
    if (text != Str::Count) {
        const char* s = trK(text);
        ImVec2 p = bp(static_cast<float>(g_form.x0 + g_form.b0), static_cast<float>(g_form.y));
        p.x -= ImGui::CalcTextSize(s).x;   // rechtsbuendig (Qt::AlignRight)
        ImGui::GetWindowDrawList()->AddText(ImVec2{p.x, std::floor(p.y + (px(static_cast<float>(h)) - ImGui::GetTextLineHeight()) * 0.5F)},
                                            ImGui::GetColorU32(ImGuiCol_Text), s);
    }
}

void formWeiter(int h) { g_form.y += h + kAbstand; }

// Ein Feld (QSpinBox/QComboBox/QLineEdit): Rahmen 22 hoch, volle Spalte.
void feldAnfang(Str text) {
    formZeile(text, kFeldH);
    ImGui::SetCursorScreenPos(bp(static_cast<float>(g_form.x1), static_cast<float>(g_form.y)));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2{px(3.0F), std::max(0.0F, (px(static_cast<float>(kFeldH)) - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::SetNextItemWidth(px(static_cast<float>(g_form.b1)));
}
void feldEnde() {
    ImGui::PopStyleVar();
    formWeiter(kFeldH);
}

// QCheckBox: Kaestchen 14 senkrecht mittig in der Zeile 20, Text 5 dahinter.
bool haken(Str beschriftung, const char* text, bool& wert, bool aktiv = true) {
    formZeile(beschriftung, kHakenZeile);
    ImGui::SetCursorScreenPos(bp(static_cast<float>(g_form.x1), static_cast<float>(g_form.y)));
    if (!aktiv) { ImGui::BeginDisabled(); }
    const bool geaendert = qtHaken(text, &wert, static_cast<float>(kHakenZeile));
    if (!aktiv) { ImGui::EndDisabled(); }
    formWeiter(kHakenZeile);
    return geaendert;
}

void layoutSeite() {
    // Layout_constructPreferences (mainframe.cpp): appendRadioIcons "Window
    // Layout" (vier Bilder, darunter Radioknoepfe ohne Text; ein Untergitter,
    // Abstand 6 - Spalte 1 wird 4 x 72 + 3 x 6 = 306 breit), appendCheckBox
    // "Detachable Menus", "Built-In Group Dialog" (Foto pref_Layout.png).
    static const char* const kBilder[4] = {"window1", "window2", "window3", "window4"};
    static const Str kNamen[4] = {Str::KeLayoutRegular, Str::KeLayoutFloating, Str::KeLayoutSplit, Str::KeLayoutRegularLeft};
    formAnfang({Str::KePrWindowLayout}, 4 * kBildB + 3 * kAbstand);
    const int zeileH = kBildH + kAbstand + kHaken;   // 60
    formZeile(Str::KePrWindowLayout, zeileH);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (int k = 0; k < 4; ++k) {
        const float x = static_cast<float>(g_form.x1 + k * (kBildB + kAbstand));
        const ImVec2 a = bp(x, static_cast<float>(g_form.y));
        const ImVec2 e = bp(x + static_cast<float>(kBildB), static_cast<float>(g_form.y + kBildH));
        dl->AddRect(a, e, ImGui::GetColorU32(ImGuiCol_Border));
        // Das Bild (eigene Vektorzeichnung) mittig im Rahmen
        const float g = px(static_cast<float>(kBildH - 4));
        (void)karteIcon(dl, kBilder[k], ImVec2{std::floor((a.x + e.x - g) * 0.5F), std::floor((a.y + e.y - g) * 0.5F)}, g,
                        ImGui::GetColorU32(karteIconFarbe(kBilder[k])));
        // QRadioButton ohne Text, mittig darunter (AlignHCenter)
        const float r = px(static_cast<float>(kHaken));
        ImGui::SetCursorScreenPos(ImVec2{std::floor((a.x + e.x - r) * 0.5F), bp(0.0F, static_cast<float>(g_form.y + kBildH + kAbstand)).y});
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (r - ImGui::GetTextLineHeight()) * 0.5F)});
        ImGui::RadioButton((std::string("##layout") + std::to_string(k)).c_str(), &g_entwurf.anordnung, k);
        ImGui::PopStyleVar();
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", trK(kNamen[k])); }
    }
    formWeiter(zeileH);
    if (g_entwurf.anordnung == 1) {
        // Hinweis dieses Programms zu Floating (keine Zeile bei NRC)
        ImGui::SetCursorScreenPos(bp(static_cast<float>(g_form.x1), static_cast<float>(g_form.y)));
        ImGui::PushTextWrapPos(bp(static_cast<float>(g_form.x1 + g_form.b1), 0.0F).x - ImGui::GetWindowPos().x);
        ImGui::TextDisabled("%s", tr(Str::KeFloatingHinweis));
        ImGui::PopTextWrapPos();
        g_form.y += static_cast<int>(std::lround((ImGui::GetItemRectSize().y) / g_sk)) + kAbstand;
    }
    // Detachable Menus: das Abreissen (QTornOffMenu) kommt in Stufe 2; die
    // Menues zeigen die Abreisskante wie NRC (Vorgabe an).
    (void)haken(Str::Count, trK(Str::KePrDetachableMenus), g_entwurf.abtrennbar, false);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        char z[120];
        std::snprintf(z, sizeof(z), tr(Str::KeStufe), 2);
        ImGui::SetTooltip("%s", z);
    }
    (void)haken(Str::Count, trK(Str::KePrBuiltInGroupDialog), g_entwurf.eingebaut);
}

// Camera_constructPreferences (camwindow.cpp:2398-2460): appendSpinner,
// appendCheckBox, appendCombo in dieser Reihenfolge (Foto pref_Camera.png).
void kameraSeite() {
    karte::KameraEinstellungen& c = g_entwurf.kamera;
    formAnfang({Str::KePrCamMoveSpeed, Str::KePrCamTimeMax, Str::KePrCamScrollSpeed, Str::KePrCamStrafeSpeed, Str::KePrCamSensitivity,
                Str::KePrCamRenderMode, Str::KePrCamMsaa, Str::KePrCamStrafeMode, Str::KePrCamFov});
    const auto ganz = [&](Str text, int& wert, int von, int bis) {
        feldAnfang(text);
        ImGui::InputInt((std::string("##") + trK(text)).c_str(), &wert);
        wert = std::clamp(wert, von, bis);
        feldEnde();
    };
    const auto komma = [&](Str text, float& wert, float von, float bis, const char* format) {
        feldAnfang(text);
        ImGui::InputFloat((std::string("##") + trK(text)).c_str(), &wert, 0.1F, 1.0F, format);
        wert = std::clamp(wert, von, bis);
        feldEnde();
    };
    const auto auswahl = [&](Str text, int& wert, const char* const* eintraege, int anzahl) {
        feldAnfang(text);
        const bool r = ImGui::Combo((std::string("##") + trK(text)).c_str(), &wert, eintraege, anzahl);
        feldEnde();
        return r;
    };
    ganz(Str::KePrCamMoveSpeed, c.tempo, 1, karte::kTempoMax);
    ganz(Str::KePrCamTimeMax, c.zeitBisMax, 0, 5000);
    ganz(Str::KePrCamScrollSpeed, c.radTempo, 0, 999);
    komma(Str::KePrCamStrafeSpeed, c.strafeTempo, 0.1F, 10.0F, "%.2f");
    komma(Str::KePrCamSensitivity, c.winkelTempo, 0.1F, 30.0F, "%.2f");
    (void)haken(Str::Count, trK(Str::KePrCamInvert), c.mausUmkehren);
    (void)haken(Str::Count, trK(Str::KePrCamZoomPointer), c.zumZeiger);
    (void)haken(Str::Count, trK(Str::KePrCamDiscrete), c.diskret);
    (void)haken(Str::Count, trK(Str::KePrCamFarClip), c.kubisch);
    (void)haken(Str::Count, trK(Str::KePrCamColorize), c.faerben);
    (void)haken(Str::Count, trK(Str::KePrCamFacesWire), c.flaechenDraht);
    // Render Mode: ohne "Lighting" (nur doom3)
    const char* modi[4] = {trK(Str::KePrCamWireframe), trK(Str::KePrCamFlatshade), trK(Str::KePrCamTextured),
                           trK(Str::KePrCamTexturedWire)};
    int modus = std::clamp(static_cast<int>(c.modus), 0, 3);
    if (auswahl(Str::KePrCamRenderMode, modus, modi, 4)) { c.modus = static_cast<karte::KameraModus>(modus); }
    // MSAA: Index 0..5 = 0, 2, 4, 8, 16, 32 (CamMSAAImport: 1 << Index)
    static const char* const abtastungen[6] = {"0", "2", "4", "8", "16", "32"};
    int index = 0;
    for (int i = 0; i < 6; ++i) {
        if (std::atoi(abtastungen[i]) == c.msaa) { index = i; }
    }
    if (auswahl(Str::KePrCamMsaa, index, abtastungen, 6)) { c.msaa = index != 0 ? 1 << index : 0; }
    const char* strafe[5] = {trK(Str::KePrCamStrafeNone), trK(Str::KePrCamStrafeUp), trK(Str::KePrCamStrafeForward),
                             trK(Str::KePrCamStrafeBoth), trK(Str::KePrCamStrafeBothInv)};
    (void)auswahl(Str::KePrCamStrafeMode, c.strafeModus, strafe, 5);
    // Field Of View: 1 .. 175, 0 Nachkommastellen
    komma(Str::KePrCamFov, c.sichtfeld, 1.0F, 175.0F, "%.0f");
}

// SelectionSystem_constructPreferences (selection.cpp:2313-2325): appendSpinner
// "Selector size (pixels)" 2..64, zwei Checkboxen, appendCombo "Move style in 3D"
// (Foto pref_Selection.png).
void auswahlSeite() {
    formAnfang({Str::KePrSelSize, Str::KePrSelMoveStyle});
    feldAnfang(Str::KePrSelSize);
    ImGui::InputInt("##waehlergroesse", &g_entwurf.waehlEpsilon);
    g_entwurf.waehlEpsilon = std::clamp(g_entwurf.waehlEpsilon, 2, 64);
    feldEnde();
    (void)haken(Str::Count, trK(Str::KePrSelPreferPoint), g_entwurf.punktEntitaeten2d);
    (void)haken(Str::Count, trK(Str::KePrSel3dBrushes), g_entwurf.pinselIn3d);
    feldAnfang(Str::KePrSelMoveStyle);
    const char* stile[2] = {trK(Str::KePrSelMoveXY), trK(Str::KePrSelMoveView)};
    (void)ImGui::Combo("##bewegstil", &g_entwurf.bewegStil3d, stile, 2);
    feldEnde();
}

// TextureBrowser_constructPreferences (texwindow.cpp:2044-2073): Checkbox
// "Texture scrollbar", Klappfeld der Skala, drei Spinner, Klappfeld "Load
// Shaders at Startup" (None / System), Checkbox "Hide nonShaders in system/
// folder" (Foto pref_Texture_Browser.png).
void texBroSeite() {
    karte::TexturBrowserEinstellungen& t = g_entwurf.texBro;
    formAnfang({Str::KePrTbScale, Str::KePrTbMax, Str::KePrTbMin, Str::KePrTbWheel, Str::KePrTbStartup});
    (void)haken(Str::Count, trK(Str::KePrTbScrollbar), t.rollbalken);
    feldAnfang(Str::KePrTbScale);
    static const char* const skalen[5] = {"10%", "25%", "50%", "100%", "200%"};
    int index = std::max(0, karte::texturSkalaIndex(t.skala));
    if (ImGui::Combo("##texskala", &index, skalen, 5)) { t.skala = karte::texturSkalaAusIndex(index); }
    feldEnde();
    feldAnfang(Str::KePrTbMax);
    ImGui::InputInt("##texmax", &t.einheitsGroesse);
    t.einheitsGroesse = std::clamp(t.einheitsGroesse, 16, 8192);
    feldEnde();
    feldAnfang(Str::KePrTbMin);
    ImGui::InputInt("##texmin", &t.einheitsMin);
    t.einheitsMin = std::clamp(t.einheitsMin, 16, 8192);
    feldEnde();
    feldAnfang(Str::KePrTbWheel);
    ImGui::InputInt("##texrad", &t.radSchritt);
    t.radSchritt = std::clamp(t.radSchritt, 0, 8192);
    feldEnde();
    feldAnfang(Str::KePrTbStartup);
    const char* start[2] = {trK(Str::KePrTbNone), "System"};   // TextureBrowser_getCommonShadersName
    (void)ImGui::Combo("##texstart", &t.startShader, start, 2);
    feldEnde();
    char text[160];
    std::snprintf(text, sizeof(text), trK(Str::KePrTbHideCommon), editor().texBro.commonVerzeichnis.c_str());
    (void)haken(Str::Count, text, t.commonOhneTexturen);
}

// Entity_constructPreferences (entity.cpp:663-679): zwei Spinner, eine
// Checkbox "Entity Names" mit dem Text "= Targetnames" (Foto pref_Entities.png).
void entitaetenSeite() {
    formAnfang({Str::KePrEnDist, Str::KePrEnRatio, Str::KePrEnNames});
    feldAnfang(Str::KePrEnDist);
    ImGui::InputInt("##namenabstand", &g_entwurf.namenAbstand);
    g_entwurf.namenAbstand = std::clamp(g_entwurf.namenAbstand, 0, 200500);
    feldEnde();
    feldAnfang(Str::KePrEnRatio);
    ImGui::InputInt("##namenverhaeltnis", &g_entwurf.namenVerhaeltnis);
    g_entwurf.namenVerhaeltnis = std::clamp(g_entwurf.namenVerhaeltnis, 0, 100500);
    feldEnde();
    (void)haken(Str::KePrEnNames, trK(Str::KePrEnTargetnames), g_entwurf.zielnamen);
}

// QRadioButton wie Fusion: Kreis 14 senkrecht mittig, Text 6 dahinter
// (PM_ExclusiveIndicatorWidth, PM_RadioButtonLabelSpacing).
void radioKnopf(const char* text, int& wert, int eigener, float x, float zeileH) {
    ImGui::SetCursorScreenPos(bp(x, static_cast<float>(g_form.y)));
    const float r = px(static_cast<float>(kHaken));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (r - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2{px(6.0F), 0.0F});
    const ImVec2 vorher = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2{vorher.x, std::floor(vorher.y + (px(zeileH) - r) * 0.5F)});
    ImGui::RadioButton(text, &wert, eigener);
    ImGui::PopStyleVar(2);
}

// Clipper_constructPreferences (clippertool.cpp:163-180): vier Checkboxen,
// "Reset Flipped State" haengt an "Reset Points on Split"
// (Widget_connectToggleDependency, preferences.cpp:517-542), dazu appendRadio
// "On DoubleClick do: " (RadioHBox_new: QHBoxLayout, die Knoepfe teilen die
// Spalte; Foto pref_Clipper.png).
void clipperSeite() {
    formAnfang({Str::KePrClipDouble});
    (void)haken(Str::Count, trK(Str::KePrClipCaulk), g_entwurf.clipperCaulk);
    (void)haken(Str::Count, trK(Str::KePrClipResetFlip), g_entwurf.clipperUmdrehen, g_entwurf.clipperPunkte);
    (void)haken(Str::Count, trK(Str::KePrClipResetPoints), g_entwurf.clipperPunkte);
    (void)haken(Str::Count, trK(Str::KePrClip2Points), g_entwurf.clipper2Punkte);
    formZeile(Str::KePrClipDouble, kHakenZeile);
    const float halb = static_cast<float>(g_form.b1) * 0.5F;
    radioKnopf((std::string(trK(Str::KePrClipClip)) + "##clipperdoppelt0").c_str(), g_entwurf.clipperDoppelklick, 0, static_cast<float>(g_form.x1),
               static_cast<float>(kHakenZeile));
    radioKnopf((std::string(trK(Str::KePrClipSplit)) + "##clipperdoppelt1").c_str(), g_entwurf.clipperDoppelklick, 1,
               static_cast<float>(g_form.x1) + halb, static_cast<float>(kHakenZeile));
    formWeiter(kHakenZeile);
}

void interfaceSeite() {
    // Interface Preferences: "Shader Editor Command" (preferences.cpp:64,
    // appendPathEntry: Feld mit "..."-Knopf; Foto pref_Interface.png), dazu
    // die Einstellung dieses Programms "Begriffe wie NetRadiant".
    static char befehl[260] = "";
    formAnfang({Str::KePrShaderEditor});
    feldAnfang(Str::KePrShaderEditor);
    ImGui::BeginDisabled();
    ImGui::InputText("##shadereditor", befehl, sizeof(befehl));
    ImGui::EndDisabled();
    feldEnde();
    (void)haken(Str::Count, trK(Str::KeNrcTerms), g_entwurf.nrc);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::KeNrcTermsHint)); }
}

// Die Pfeile des Baums (PE_IndicatorBranch, Fusion: qt_fusion_draw_arrow
// 8 x 4 nach unten, aufgeklappt) in der Mitte des ersten Einzugs.
void baumPfeil(ImDrawList* dl, float zeileY) {
    const ImVec2 a = bp(18.0F, zeileY + 6.0F);
    ImVec4 f = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    f.w *= 160.0F / 255.0F;
    dl->AddTriangleFilled(a, ImVec2{a.x + px(8.0F), a.y}, ImVec2{a.x + px(4.0F), a.y + px(4.0F)}, ImGui::GetColorU32(f));
}

}  // namespace

void zeichneEinstellungen() {
    Editor& ed = editor();
    if (ed.einstellungenOffen && !g_entwurf.offen) {
        g_entwurf.offen = true;
        g_entwurf.anordnung = static_cast<int>(ed.anordnung);
        g_entwurf.eingebaut = ed.gruppenDialogEingebaut;
        g_entwurf.nrc = nrcBegriffe();
        g_entwurf.kamera = ed.kameraE;
        g_entwurf.waehlEpsilon = static_cast<int>(ed.bearbeitung.waehlEpsilon);
        g_entwurf.punktEntitaeten2d = ed.bearbeitung.einstellung.punktEntitaetenIn2d;
        g_entwurf.pinselIn3d = ed.bearbeitung.pinselIn3d;
        g_entwurf.bewegStil3d = ed.bearbeitung.bewegStil3d;
        g_entwurf.texBro = ed.texBro.e;
        g_entwurf.namenAbstand = ed.ent.namenAbstand;
        g_entwurf.namenVerhaeltnis = ed.anzeige.namenVerhaeltnis;
        g_entwurf.zielnamen = ed.ent.zielnamen;
        g_entwurf.clipperCaulk = ed.bearbeitung.clipperCaulk;
        g_entwurf.clipperUmdrehen = ed.bearbeitung.clipperUmdrehenZuruecksetzen;
        g_entwurf.clipperPunkte = ed.bearbeitung.clipperPunkteZuruecksetzen;
        g_entwurf.clipper2Punkte = ed.bearbeitung.clipper2Punkte2d;
        g_entwurf.clipperDoppelklick = ed.bearbeitung.clipperDoppelklickTeilen;
    }
    if (!ed.einstellungenOffen) {
        g_entwurf.offen = false;
        return;
    }
    // Groesse und Aufteilung wie NRC 20260114 (Masse oben, Fotos
    // dlg_Edit_Preferences_0 und pref_<Seite>.png): fester Innenraum 597 x 492.
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float sk = massstab();
    g_sk = sk;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    ImGui::SetNextWindowSize(ImVec2{px(static_cast<float>(kInnenB)), px(static_cast<float>(kInnenH)) + titelH}, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F},
                            ImGuiCond_FirstUseEver, ImVec2{0.5F, 0.5F});
    const std::string titel = std::string(trK(Str::KePrefsTitle)) + "###karteeinstellungen";
    bool offen = true;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool sichtbar = ImGui::Begin(titel.c_str(), &offen,
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    if (sichtbar) {
        g_innen = ImGui::GetCursorScreenPos();
        const float knopfB = px(static_cast<float>(kKnopfB));
        const float knopfH = px(static_cast<float>(kKnopfH));
        // QTreeView: Zeilen 16, Text 3 nach dem Einzug, der Pfeil vor den
        // Oberseiten; die Auswahl ueber die ganze Zeile.
        ImGui::SetCursorScreenPos(bp(static_cast<float>(kRand), static_cast<float>(kRand)));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, px(1.0F)});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
        ImGui::BeginChild("##baum", ImVec2{px(static_cast<float>(kBaumB)), px(static_cast<float>(kInnenH - 2 * kRand))}, ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_NoScrollbar);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        for (int i = 0; i < static_cast<int>(std::size(kSeiten)); ++i) {
            const Seite& s = kSeiten[i];
            const float zeileY = static_cast<float>(kRand + 1 + i * kBaumZeile);
            ImGui::SetCursorScreenPos(bp(static_cast<float>(kRand + 1), zeileY));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});
            if (ImGui::Selectable((std::string(trK(s.baum)) + "##seite" + std::to_string(i)).c_str(), g_entwurf.seite == i,
                                  ImGuiSelectableFlags_None, ImVec2{0.0F, px(static_cast<float>(kBaumZeile))})) {
                g_entwurf.seite = i;
            }
            ImGui::PopStyleColor();
            const float textX = static_cast<float>(kRand + 1 + kEinzug * (s.tiefe + 1) + kTextRand);
            const ImVec2 tp = bp(textX, zeileY);
            dl->AddText(ImVec2{tp.x, std::floor(tp.y + (px(static_cast<float>(kBaumZeile)) - ImGui::GetTextLineHeight()) * 0.5F)},
                        ImGui::GetColorU32(ImGuiCol_Text), trK(s.baum));
            if (s.tiefe == 0) { baumPfeil(dl, zeileY); }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        // QGroupBox: Titel, Rahmen, darin die Seite (Tahoma 8 pt)
        dialogSchriftAn();
        ImDrawList* fl = ImGui::GetWindowDrawList();
        fl->AddText(bp(static_cast<float>(kGruppeX + kTitelX), static_cast<float>(kRand)), ImGui::GetColorU32(ImGuiCol_Text),
                    trK(kSeiten[g_entwurf.seite].titel));
        fl->AddRect(bp(static_cast<float>(kGruppeX), static_cast<float>(kRand + kRahmenOben)),
                    bp(static_cast<float>(kGruppeX + kGruppeB), static_cast<float>(kRand + kGruppeH)), ImGui::GetColorU32(ImGuiCol_Border),
                    ImGui::GetStyle().FrameRounding);
        g_form = Form{};
        if (g_entwurf.seite == kSeiteLayout) {
            layoutSeite();
        } else if (g_entwurf.seite == kSeiteInterface) {
            interfaceSeite();
        } else if (g_entwurf.seite == kSeiteKamera) {
            kameraSeite();
        } else if (g_entwurf.seite == kSeiteAuswahl) {
            auswahlSeite();
        } else if (g_entwurf.seite == kSeiteTexBro) {
            texBroSeite();
        } else if (g_entwurf.seite == kSeiteEntities) {
            entitaetenSeite();
        } else if (g_entwurf.seite == kSeiteClipper) {
            clipperSeite();
        } else {
            char z[120];
            std::snprintf(z, sizeof(z), tr(Str::KeStufe), 2);
            fl->AddText(bp(static_cast<float>(g_form.x0), static_cast<float>(g_form.y)), ImGui::GetColorU32(ImGuiCol_TextDisabled), z);
        }
        // QDialogButtonBox: "Clean" (ResetRole) links, "OK" und "Cancel" rechts
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (knopfH - ImGui::GetTextLineHeight()) * 0.5F)});
        ImGui::SetCursorScreenPos(bp(static_cast<float>(kGruppeX), static_cast<float>(kKnopfY)));
        ImGui::BeginDisabled();
        (void)ImGui::Button(trK(Str::KePrefClean), ImVec2{knopfB, knopfH});
        ImGui::EndDisabled();
        ImGui::SetCursorScreenPos(bp(static_cast<float>(kGruppeX + kGruppeB - 2 * kKnopfB - kAbstand), static_cast<float>(kKnopfY)));
        if (ImGui::Button(trK(Str::KePrefOk), ImVec2{knopfB, knopfH})) {
            ed.anordnung = static_cast<befehle::Anordnung>(g_entwurf.anordnung);
            ed.gruppenDialogEingebaut = g_entwurf.eingebaut;
            setzeNrcBegriffe(g_entwurf.nrc);
            // CamWnd_Move_Discrete_Import / Camera_SetFarClip / fieldOfViewImport;
            // Cubic-Skala, Show-Schalter und das Tempo-Kuerzel bleiben.
            if (g_entwurf.kamera.diskret != ed.kameraE.diskret && ed.kamera.flagsLoeschen(karte::kBewegAlle)) {
                ed.kamFenster.tastenUhr = -1.0;
            }
            karte::KameraEinstellungen neu = g_entwurf.kamera;
            neu.kubikSkala = ed.kameraE.kubikSkala;
            neu.zeigeStatistik = ed.kameraE.zeigeStatistik;
            neu.zeigeWorkzone = ed.kameraE.zeigeWorkzone;
            neu.zeigeGroesse = ed.kameraE.zeigeGroesse;
            ed.kameraE = neu;
            ed.kamera.projektionAktualisieren(ed.kameraE);
            ed.bearbeitung.waehlEpsilon = static_cast<float>(g_entwurf.waehlEpsilon);
            ed.bearbeitung.einstellung.punktEntitaetenIn2d = g_entwurf.punktEntitaeten2d;
            ed.bearbeitung.pinselIn3d = g_entwurf.pinselIn3d;
            ed.bearbeitung.bewegStil3d = g_entwurf.bewegStil3d;
            // TextureBrowserImportShowScrollbar, TextureScaleImport,
            // UniformTextureSizeImport (heightChanged), die uebrigen direkt.
            {
                karte::TexturBrowserEinstellungen& t = ed.texBro.e;
                const karte::TexturBrowserEinstellungen& n = g_entwurf.texBro;
                const bool hoehe = t.rollbalken != n.rollbalken || t.skala != n.skala || t.einheitsGroesse != n.einheitsGroesse ||
                                   t.einheitsMin != n.einheitsMin;
                t.rollbalken = n.rollbalken;
                t.skala = n.skala;
                t.einheitsGroesse = n.einheitsGroesse;
                t.einheitsMin = n.einheitsMin;
                t.radSchritt = n.radSchritt;
                t.startShader = n.startShader;
                t.commonOhneTexturen = n.commonOhneTexturen;
                if (hoehe) { ed.texBro.hoeheGeaendert(); }
            }
            // ShowNamesDistImport / ShowNamesRatioImport (UpdateAllWindows),
            // ShowTargetNamesImport: NRC verlangt dafuer einen Neustart
            // (PreferencesDialog_restartRequired) - hier gilt es sofort, die
            // Namen werden jedes Bild neu gebaut.
            ed.ent.namenAbstand = g_entwurf.namenAbstand;
            ed.anzeige.namenVerhaeltnis = g_entwurf.namenVerhaeltnis;
            ed.ent.zielnamen = g_entwurf.zielnamen;
            ed.bearbeitung.clipperCaulk = g_entwurf.clipperCaulk;
            ed.bearbeitung.clipperUmdrehenZuruecksetzen = g_entwurf.clipperUmdrehen;
            ed.bearbeitung.clipperPunkteZuruecksetzen = g_entwurf.clipperPunkte;
            ed.bearbeitung.clipper2Punkte2d = g_entwurf.clipper2Punkte;
            ed.bearbeitung.clipperDoppelklickTeilen = g_entwurf.clipperDoppelklick;
            ed.maximiert = false;
            einstellungenSpeichern();
            offen = false;
        }
        ImGui::SetCursorScreenPos(bp(static_cast<float>(kGruppeX + kGruppeB - kKnopfB), static_cast<float>(kKnopfY)));
        if (ImGui::Button(trK(Str::KeCancel), ImVec2{knopfB, knopfH})) { offen = false; }
        ImGui::PopStyleVar();
        dialogSchriftAus();
        // Platz belegen (sonst meldet ImGui die gesetzte Schreibstelle als Ausdehnung)
        ImGui::SetCursorScreenPos(bp(0.0F, static_cast<float>(kInnenH)));
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
    }
    ImGui::End();
    if (!offen) { ed.einstellungenOffen = false; }
}

// --- Mapped Commands (commands.cpp:332-428, DoCommandListDlg) -------------------
//
// Baum mit "Command" und "Key" (sortierbar), Suchfelder "by command name" und
// "by keys", Knoepfe Edit / Clear / Reset / Reset All. Edit nimmt EINE Taste
// auf (Single_QKeySequenceEdit, commands.cpp:292-322: Ziffernblock setzt
// KeypadModifier); eine schon belegte Taste fragt nach
// (VerifyAcceleratorNotTaken: Yes = beim anderen loeschen, No = doppelt
// lassen, Cancel = abbrechen).

namespace {

struct TastenDialog {
    bool offen = false;
    char suchName[64] = "";
    char suchTaste[64] = "";
    std::string gewaehlt;
    // Rueckfrage "Key already used"
    bool frage = false;
    std::string frageBefehl;     // der bearbeitete Befehl
    std::string frageAnderer;    // der, der die Taste schon hat
    befehle::Taste frageTaste = 0;
    bool frageAlle = false;      // "Boo!" - Reset All
    int sortSpalte = 0;          // sortByColumn( 0, AscendingOrder )
    bool absteigend = false;
};
TastenDialog g_td;

// QKeySequenceEdit::keyPressEvent + translateModifiers (Qt 5.15): Shift zaehlt
// nur, wenn das Zeichen kein druckbares Satzzeichen ist; e->key() ist das
// erzeugte Zeichen.
befehle::Taste tasteAufnehmen() {
    const ImGuiIO& io = ImGui::GetIO();
    for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k) {
        const auto key = static_cast<ImGuiKey>(k);
        if (key >= ImGuiKey_LeftCtrl && key <= ImGuiKey_RightSuper) { continue; }
        if (key >= ImGuiKey_ReservedForModCtrl) { continue; }
        if (!ImGui::IsKeyPressed(key, false)) { continue; }
        befehle::TastenBelegung kb;
        bool ziffernblock = false;
        bool erweitert = false;
        if (!tastenBelegung(k, kb, ziffernblock, erweitert)) { continue; }
        const bool shift = io.KeyShift;
        const bool ctrl = io.KeyCtrl;
        const bool alt = io.KeyAlt;
        // e->key(): das Zeichen fuer Umschalt und AltGr; Strg allein aendert
        // das Zeichen nicht (Qt rechnet dann ohne Strg).
        std::uint32_t key2 = kb.qtKey[(ctrl && alt) ? (shift ? 7U : 6U) : (shift ? 1U : 0U)];
        if (key2 == 0) { key2 = kb.qtKey[0]; }
        if (erweitert && key2 == befehle::kKeyReturn) { key2 = befehle::kKeyEnter; }
        std::uint32_t mods = 0;
        const bool druckbar = key2 >= 0x21 && key2 < 0x01000000;
        const bool buchstabeZiffer = (key2 >= 'A' && key2 <= 'Z') || (key2 >= '0' && key2 <= '9') || key2 >= 0x80;
        if (shift && (!druckbar || buchstabeZiffer)) { mods |= befehle::kShift; }
        // AltGr erzeugt das Zeichen selbst - Strg+Alt sind dann verbraucht.
        const bool altGrZeichen = ctrl && alt && kb.qtKey[shift ? 7U : 6U] != 0 &&
                                  kb.qtKey[shift ? 7U : 6U] != kb.qtKey[shift ? 1U : 0U];
        if (ctrl && !altGrZeichen) { mods |= befehle::kCtrl; }
        if (alt && !altGrZeichen) { mods |= befehle::kAlt; }
        if (io.KeySuper) { mods |= befehle::kMeta; }
        if (ziffernblock) { mods |= befehle::kNum; }
        return key2 | mods;
    }
    return 0;
}

// accelerator_alter (commands.cpp:224-256)
void tasteAendern(const std::string& befehl, befehle::Taste neu) {
    Editor& ed = editor();
    const befehle::Taste t = neu != 0 ? neu : ed.belegung.standard(befehl);
    const std::string anderer = ed.belegung.belegtVon(t, befehl);
    if (!anderer.empty()) {
        g_td.frage = true;
        g_td.frageAlle = false;
        g_td.frageBefehl = befehl;
        g_td.frageAnderer = anderer;
        g_td.frageTaste = t;
        return;
    }
    ed.belegung.setzen(befehl, t);
    einstellungenSpeichern();
}

void frageZeichnen() {
    Editor& ed = editor();
    if (!g_td.frage) { return; }
    const std::string titel = std::string(g_td.frageAlle ? trK(Str::KeBooTitle) : trK(Str::KeKeyUsedTitle)) + "###kartetastenfrage";
    if (!ImGui::IsPopupOpen(titel.c_str())) { ImGui::OpenPopup(titel.c_str()); }
    if (!ImGui::BeginPopupModal(titel.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        return;
    }
    int wahl = -1;   // 0 Yes, 1 No, 2 Cancel
    if (g_td.frageAlle) {
        ImGui::TextUnformatted(trK(Str::KeResetAllAsk));
        if (ImGui::Button(trK(Str::KeYes))) { wahl = 0; }
        ImGui::SameLine();
        if (ImGui::Button(trK(Str::KeNo))) { wahl = 1; }
        if (wahl == 0) {
            ed.belegung.alleZuruecksetzen();   // accelerator_reset_all_button_clicked
            einstellungenSpeichern();
        }
    } else {
        const std::string tt = befehle::tasteAlsText(g_td.frageTaste);
        char z[600];
        std::snprintf(z, sizeof(z), trK(Str::KeKeyUsed), g_td.frageAnderer.c_str(), tt.c_str(), g_td.frageAnderer.c_str());
        ImGui::TextUnformatted(z);
        if (ImGui::Button(trK(Str::KeYes))) { wahl = 0; }
        ImGui::SameLine();
        if (ImGui::Button(trK(Str::KeNo))) { wahl = 1; }
        ImGui::SameLine();
        if (ImGui::Button(trK(Str::KeCancel))) { wahl = 2; }
        if (wahl == 0) {
            // "clear the ACTUAL accelerator too" beim anderen, dann setzen
            ed.belegung.setzen(g_td.frageAnderer, 0);
            ed.belegung.setzen(g_td.frageBefehl, g_td.frageTaste);
            einstellungenSpeichern();
        } else if (wahl == 1) {
            // eIDNO : keep duplicate key
            ed.belegung.setzen(g_td.frageBefehl, g_td.frageTaste);
            einstellungenSpeichern();
        }
    }
    if (wahl >= 0) {
        g_td.frage = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

// Beim Oeffnen: "Writing the command list to <pfad>" (commands.cpp:357-375).
void befehlslisteSchreiben() {
    Editor& ed = editor();
    const std::string pfad = platform::settingsDirectory() + "\\karte_commandlist.txt";
    ausgabe("Writing the command list to " + pfad);
    std::ofstream f(std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(pfad.data()), pfad.size())));
    if (!f) { return; }
    for (const std::string& n : ed.belegung.namen()) {
        std::string zeile = n;
        while (zeile.size() < 32) { zeile += ' '; }
        f << zeile << befehle::tasteAlsText(ed.belegung.taste(n)) << '\n';
    }
}

}  // namespace

void zeichneTastenDialog() {
    Editor& ed = editor();
    if (!ed.tastenDialogOffen) {
        g_td.offen = false;
        ed.tastenDialogBearbeitet.clear();
        return;
    }
    if (!g_td.offen) {
        // DoCommandListDlg baut den Dialog jedes Mal neu: leere Suchfelder,
        // nichts gewaehlt, nach Command aufsteigend sortiert.
        g_td = TastenDialog{};
        g_td.offen = true;
        befehlslisteSchreiben();
    }
    // Eine Taste aufnehmen, solange ein Befehl bearbeitet wird.
    if (!ed.tastenDialogBearbeitet.empty() && !g_td.frage) {
        const befehle::Taste t = tasteAufnehmen();
        if (t != 0) {
            const std::string b = ed.tastenDialogBearbeitet;
            ed.tastenDialogBearbeitet.clear();
            // Single_QKeySequenceEdit::event: "consume ALL key presses"
            ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
            tasteAendern(b, t);
        }
    }
    // DoCommandListDlg (commands.cpp:332-420) mit Qts Massen, am Foto
    // dlg_Edit_Shortcuts_0 gemessen (logische Punkte, Innenraum 399 x 533):
    //   QGridLayout Rand 11, Abstand 6; Zeile 0 die zwei Suchfelder 22 hoch
    //   (11..153, 159..301), Zeile 1 der Baum (11, 39, 290 x 483) und rechts
    //   die senkrechte QDialogButtonBox (308, 39 + n x 28, je 80 x 22)
    //   QTreeWidget: Kopf 20 hoch (Tahoma, Text 2 nach dem Rand -
    //   PM_HeaderMargin), Zeilen 16 (Segoe UI 9 pt), Text 3 nach dem Rand,
    //   Spalten ResizeToContents, Rollbalken 7 (dark.qss)
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float s = massstab();
    g_sk = s;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    ImGui::SetNextWindowSize(ImVec2{px(399.0F), px(533.0F) + titelH}, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F},
                            ImGuiCond_FirstUseEver, ImVec2{0.5F, 0.5F});
    const std::string titel = std::string(trK(Str::KeMappedCommands)) + "###kartetasten";
    bool offen = true;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool sichtbar = ImGui::Begin(titel.c_str(), &offen,
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    if (sichtbar) {
        g_innen = ImGui::GetCursorScreenPos();
        const float knopfB = px(80.0F);
        const float feldH = px(22.0F);
        dialogSchriftAn();   // QLineEdit, QPushButton, QHeaderView: Tahoma 8 pt
        // Suchfelder mit Lupe vorn (NRC 20260114: Emoji im Platzhalter, hier
        // die Lupe des Programms).
        const auto suchfeld = [&](const char* id, const char* hinweis, char* puffer, std::size_t groesse, float x, float breite) {
            const float lupe = ImGui::GetFontSize();
            ImGui::SetCursorScreenPos(bp(x, 11.0F));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                                ImVec2{px(4.0F) + lupe, std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
            ImGui::SetNextItemWidth(px(breite));
            const ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::InputTextWithHint(id, hinweis, puffer, groesse);
            ImGui::PopStyleVar();
            uiIcon(ImGui::GetWindowDrawList(), UiIcon::Suche, ImVec2{std::floor(p.x + px(3.0F)), std::floor(p.y + (feldH - lupe) * 0.5F)}, lupe,
                   ImGui::GetColorU32(uiIconFarbe(UiIcon::Suche)));
        };
        suchfeld("##suchname", trK(Str::KeByName), g_td.suchName, sizeof(g_td.suchName), 11.0F, 142.0F);
        suchfeld("##suchtaste", trK(Str::KeByKeys), g_td.suchTaste, sizeof(g_td.suchTaste), 159.0F, 142.0F);
        const auto enthaelt = [](const std::string& text, const char* such) {
            if (such[0] == 0) { return true; }
            std::string a2 = text;
            std::string b2 = such;
            std::transform(a2.begin(), a2.end(), a2.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::transform(b2.begin(), b2.end(), b2.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return a2.find(b2) != std::string::npos;
        };
        std::vector<std::pair<std::string, std::string>> zeilen;
        for (const std::string& n : ed.belegung.namen()) {
            const std::string tx = ed.tastenDialogBearbeitet == n ? std::string(trK(Str::KePressKey))
                                                                   : befehle::tasteAlsText(ed.belegung.taste(n));
            if (!enthaelt(n, g_td.suchName) || !enthaelt(tx, g_td.suchTaste)) { continue; }
            zeilen.emplace_back(n, tx);
        }
        // QTreeWidget::sortItems: Gross-/Kleinschreibung zaehlt nicht
        // ("bobToolz::About" zwischen "AutoCaulkSelected" und "Brush3Sided").
        const auto klein = [](const std::string& tx) {
            std::string k = tx;
            std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return k;
        };
        std::stable_sort(zeilen.begin(), zeilen.end(), [&klein](const auto& x, const auto& y) {
            const std::string a2 = klein(g_td.sortSpalte == 0 ? x.first : x.second);
            const std::string b2 = klein(g_td.sortSpalte == 0 ? y.first : y.second);
            return g_td.absteigend ? b2 < a2 : a2 < b2;
        });
        // ResizeToContents: Spalte = breiteste Zeile (Segoe UI) + 2 x 3, wenigstens
        // der Kopf (Tahoma) + 2 x 2, beim sortierten mit Sortierpfeil.
        const auto segoe = [](const std::string& tx) {
            const int b2 = platform::gdiTextBreite(tx, L"Segoe UI", 12, false);
            return b2 >= 0 ? b2 : 0;
        };
        int w[2] = {labelBreite(trK(Str::KeColCommand)) + 4, labelBreite(trK(Str::KeColKey)) + 4};
        w[g_td.sortSpalte] += 16;   // PM_HeaderMarkSize + Rand
        for (const auto& [n, tx] : zeilen) {
            w[0] = std::max(w[0], segoe(n) + 6);
            w[1] = std::max(w[1], segoe(tx) + 6);
        }
        ImDrawList* dl = ImGui::GetWindowDrawList();
        // Rahmen des Baums (QFrame, 1) und der Kopf 40..60
        dl->AddRect(bp(11.0F, 39.0F), bp(301.0F, 522.0F), ImGui::GetColorU32(ImGuiCol_Border));
        const char* koepfe[2] = {trK(Str::KeColCommand), trK(Str::KeColKey)};
        float kx = 12.0F;
        for (int sp = 0; sp < 2; ++sp) {
            const float kb = std::min(static_cast<float>(w[sp]), 300.0F - kx);
            if (kb <= 0.0F) { break; }
            const ImVec2 k0 = bp(kx, 40.0F);
            const ImVec2 k1 = bp(kx + kb, 59.0F);
            ImGui::SetCursorScreenPos(k0);
            if (ImGui::InvisibleButton((std::string("##kopf") + std::to_string(sp)).c_str(), ImVec2{k1.x - k0.x, k1.y - k0.y})) {
                // QHeaderView: Klick sortiert nach der Spalte, nochmal dreht um
                if (g_td.sortSpalte == sp) {
                    g_td.absteigend = !g_td.absteigend;
                } else {
                    g_td.sortSpalte = sp;
                    g_td.absteigend = false;
                }
            }
            dl->AddRectFilled(k0, k1, ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_HeaderHovered : ImGuiCol_TableHeaderBg));
            dl->AddLine(ImVec2{k1.x, k0.y}, ImVec2{k1.x, k1.y}, ImGui::GetColorU32(ImGuiCol_Border));
            dl->AddText(ImVec2{k0.x + px(2.0F), std::floor(k0.y + (k1.y - k0.y - ImGui::GetTextLineHeight()) * 0.5F)},
                        ImGui::GetColorU32(ImGuiCol_Text), koepfe[sp]);
            if (sp == g_td.sortSpalte) {
                // PE_IndicatorHeaderArrow: aufsteigend nach oben (am Foto 173..181)
                const float ax = k1.x - px(13.0F);
                const float ay = std::floor((k0.y + k1.y) * 0.5F);
                const ImU32 f = ImGui::GetColorU32(ImGuiCol_Text);
                if (g_td.absteigend) {
                    dl->AddTriangleFilled(ImVec2{ax, ay - px(2.0F)}, ImVec2{ax + px(8.0F), ay - px(2.0F)}, ImVec2{ax + px(4.0F), ay + px(2.0F)}, f);
                } else {
                    dl->AddTriangleFilled(ImVec2{ax + px(4.0F), ay - px(2.0F)}, ImVec2{ax + px(8.0F), ay + px(2.0F)}, ImVec2{ax, ay + px(2.0F)}, f);
                }
            }
            kx += kb;
        }
        dl->AddLine(bp(12.0F, 59.0F), bp(300.0F, 59.0F), ImGui::GetColorU32(ImGuiCol_Border));
        dialogSchriftAus();
        // Die Zeilen (Segoe UI 9 pt, ItemViewFont), Rollbalken 7
        ImGui::SetCursorScreenPos(bp(12.0F, 60.0F));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, px(7.0F));
        ImGui::BeginChild("##tastenliste", ImVec2{px(288.0F), px(461.0F)}, ImGuiChildFlags_None);
        ImDrawList* zl = ImGui::GetWindowDrawList();
        const float zeileH = px(16.0F);
        for (const auto& [n, tx] : zeilen) {
            const ImVec2 zp = ImGui::GetCursorScreenPos();
            const bool sel = g_td.gewaehlt == n;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});
            if (ImGui::Selectable((n + "##tz").c_str(), sel, ImGuiSelectableFlags_AllowDoubleClick, ImVec2{0.0F, zeileH})) {
                g_td.gewaehlt = n;
                // itemActivated (Doppelklick/Enter) -> accelerator_edit
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { ed.tastenDialogBearbeitet = n; }
            }
            ImGui::PopStyleColor();
            if (!ImGui::IsItemVisible()) { continue; }
            const float ty = std::floor(zp.y + (zeileH - ImGui::GetTextLineHeight()) * 0.5F);
            zl->AddText(ImVec2{zp.x + px(3.0F), ty}, ImGui::GetColorU32(ImGuiCol_Text), n.c_str());
            zl->AddText(ImVec2{zp.x + px(static_cast<float>(w[0]) + 3.0F), ty}, ImGui::GetColorU32(ImGuiCol_Text), tx.c_str());
            // accelerator_item_set_icon: geaendert gegenueber der Vorgabe
            if (ed.belegung.taste(n) != ed.belegung.standard(n)) {
                const float mx = zp.x + px(static_cast<float>(w[0]) + 3.0F) + ImGui::CalcTextSize(tx.c_str()).x + px(4.0F);
                zl->AddText(ImVec2{mx, ty}, ImGui::GetColorU32(ImVec4{0.85F, 0.2F, 0.2F, 1.0F}), "(x)");
                if (ImGui::IsMouseHoveringRect(ImVec2{mx, zp.y}, ImVec2{mx + ImGui::CalcTextSize("(x)").x, zp.y + zeileH})) {
                    ImGui::SetTooltip("%s: %s", tr(Str::KeKeyChanged), befehle::tasteAlsText(ed.belegung.standard(n)).c_str());
                }
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(3);
        // QDialogButtonBox senkrecht: Edit, Clear, Reset, Reset All (je 80 x 22, Abstand 6)
        dialogSchriftAn();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
        const ImVec2 kg{knopfB, feldH};
        // Die Knoepfe sind immer da; ohne Wahl tun sie nichts
        // (tree->selectedItems().isEmpty()).
        const bool hatWahl = !g_td.gewaehlt.empty();
        ImGui::SetCursorScreenPos(bp(308.0F, 39.0F));
        if (ImGui::Button(trK(Str::KeEdit), kg) && hatWahl) { ed.tastenDialogBearbeitet = g_td.gewaehlt; }
        ImGui::SetCursorScreenPos(bp(308.0F, 67.0F));
        if (ImGui::Button(trK(Str::KeClear), kg) && hatWahl) {   // accelerator_clear_button_clicked
            ed.belegung.setzen(g_td.gewaehlt, 0);
            einstellungenSpeichern();
        }
        ImGui::SetCursorScreenPos(bp(308.0F, 95.0F));
        if (ImGui::Button(trK(Str::KeReset), kg) && hatWahl) { tasteAendern(g_td.gewaehlt, 0); }
        ImGui::SetCursorScreenPos(bp(308.0F, 123.0F));
        if (ImGui::Button(trK(Str::KeResetAll), kg)) {
            g_td.frage = true;
            g_td.frageAlle = true;
        }
        ImGui::PopStyleVar();
        dialogSchriftAus();
        ImGui::SetCursorScreenPos(bp(0.0F, 533.0F));
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
        frageZeichnen();
    }
    ImGui::End();
    if (!offen) { ed.tastenDialogOffen = false; }
}

}  // namespace bhed::gui::karteneditor
