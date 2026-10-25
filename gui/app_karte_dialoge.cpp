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

#include "bhed/karte/grenzen.h"

#include "app_internal.h"
#include "backend.h"
#include "karte_gpu.h"
#include "platform.h"

#include "bhed/fassung.h"
#include "tastatur.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <cstring>

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
    // Stufe 5 (Abweichung, eigene Seite): die Schwellen des Optimierers
    {Str::KePgOptimizer, Str::KePgOptimizerT, 1},
};
constexpr int kSeiteGlobal = 0;
constexpr int kSeiteGlobalGame = 1;
constexpr int kSeiteGame = 2;
constexpr int kSeitePaths = 3;
constexpr int kSeiteDisplay = 6;
constexpr int kSeiteSettings = 10;
constexpr int kSeiteUndo = 11;
constexpr int kSeiteBrush = 13;
constexpr int kSeiteGrid = 15;
constexpr int kSeiteOrtho = 18;
constexpr int kSeiteBuild = 19;
constexpr int kSeiteInterface = 4;
constexpr int kSeiteLayout = 5;
constexpr int kSeiteTexturen = 7;
constexpr int kSeitePatches = 8;
constexpr int kSeiteEntities = 9;
constexpr int kSeiteAuswahl = 12;
constexpr int kSeiteClipper = 14;
constexpr int kSeiteSurface = 16;
constexpr int kSeiteKamera = 17;
constexpr int kSeiteTexBro = 20;
constexpr int kSeiteModBro = 21;
constexpr int kSeiteAutosave = 22;
constexpr int kSeiteOptimierer = 23;

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
    int patchSchwelle = 4;   // g_PatchSubdivideThreshold
    bool siRaster = false;   // g_si_globals.m_bSnapTToGrid
    Editor::TexturQualitaet texQual;
    // ModelBrowser_constructPage (modelwindow.cpp:1363-1371)
    int modZelle = 80;
    char modOrdner[256] = "*models/99*";
    // Autosave_constructPreferences (autosave.cpp:167-173)
    bool autosaveAn = true;
    int autosaveMinuten = 15;
    bool schnappschuesse = false;
    // Punkt 11: Nudge_constructPreferences, Map_constructPreferences (General Settings)
    bool nachKlonen = false;
    bool letzteKarte = true;
    int undoStufen = 512;           // Undo_constructPreferences
    float texSkala = 0.25F;         // Brush_constructPreferences
    int pinselTyp = 1;
    bool immerCaulk = false;
    bool ganzzahlig = false;
    int rasterStandard = 7;         // Grid_constructPreferences
    int rasterMax = 4;
    bool keineStrichelung = true;   // Orthographic_constructPreferences
    bool jagen = true;
    bool zumZeiger2d = true;
    int xyMsaa = 8;
    Editor::BauEinstellungen bau;   // Build_constructPreferences
    char bauEngine[260] = "";
    char bauArgumente[260] = "";
    char bauEngineMp[260] = "";
    char bauArgumenteMp[260] = "";
    char regionShader[260] = "";
    char bauZiel[512] = "";         // Abweichung: Ziel-Ordner
    // Punkt 11b: CGameDialog::CreateGlobalFrame, Game_constructPreferences, Paths_constructPage
    int spielWahl = 0;              // m_nComboSelect ("Select the game")
    bool spielFrage = false;        // m_bGamePrompt
    int modWahl = 0;                // s_gameCombo.game_select
    char fsGame[128] = "";          // s_gameCombo.fsgame_entry (editierbar)
    bool fsGameAktiv = false;       // setEnabled( gamecombo.sensitive )
    int modusWahl = 0;              // Mapping mode: 0 sp, 1 mp
    char enginePfad[512] = "";      // g_strEnginePath
    char zusatzPfade[5][512] = {};  // g_strExtraResourcePaths
    char shaderEditor[512] = "";    // g_TextEditor_editorCommand
    // Stufe 5: Optimierer
    int optKnapp = 90;
    bool optNachBau = false;
    int optRegelwerk = -1;
    int optPortale = 8000;
    int optDreiecke = 20000;
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

// QLabel::sizeHint in der Dialogschrift (Tahoma 11, Textmass wie Qt).
// Stufe 4 Punkt 0c: Schriftzeichen, die Tahoma nicht hat (Chinesisch,
// Japanisch), zeichnet ImGui mit der Schrift der Sprache - breiter als
// DirectWrites Ersatzschrift, mit der qtTextBreite misst. Dann gilt das
// Groessere, sonst ragt die rechtsbuendige Beschriftung links aus der Spalte
// (auf der Seite Build hinter den Seitenbaum: "Region Box Shader" auf
// Japanisch).
int labelBreite(const char* text) {
    const int b = platform::qtTextBreite(text, L"Tahoma", 11.0F, false);
    const int gezeichnet = static_cast<int>(std::ceil(ImGui::CalcTextSize(text).x / g_sk - 0.01F));
    if (b < 0) { return gezeichnet; }
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p != 0; ++p) {
        if (*p >= 0xE0) { return std::max(b, gezeichnet); }   // UTF-8 ab U+0800 (kein Latein, Griechisch, Kyrillisch)
    }
    return b;
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

// QSpinBoxPrivate::calculateAdaptiveDecimalStep (Qt 5.15 qspinbox.cpp): auch
// die ganzzahligen Spinner der Seiten stehen auf AdaptiveDecimalStepType
// (Dialog::addSpinner, dialog.cpp:533-539) - unter 100 Schritt 1, sonst eine
// Zehnerpotenz unter dem Wert.
int ganzSchritt(int wert, int schritte) {
    const int betrag = std::abs(wert);
    if (betrag < 100) { return 1; }
    const int ausgleich = ((wert < 0) == (schritte < 0)) ? 0 : 1;
    const int lg = static_cast<int>(std::log10(static_cast<double>(betrag - ausgleich))) - 1;
    return static_cast<int>(std::pow(10.0, lg));
}

// SpinBox (appendSpinner mit int): Feld mit den Pfeilen der adaptiven Schritte
bool ganzSpinner(const char* id, int& wert, int von, int bis) {
    const ImGuiStyle& st = ImGui::GetStyle();
    const float knopf = ImGui::GetFrameHeight();
    const float gesamt = ImGui::CalcItemWidth();
    bool geaendert = false;
    ImGui::PushID(id);
    ImGui::SetNextItemWidth(std::max(1.0F, gesamt - 2.0F * (knopf + st.ItemInnerSpacing.x)));
    geaendert = ImGui::InputInt(id, &wert, 0, 0);
    for (const int r : {-1, 1}) {
        ImGui::SameLine(0.0F, st.ItemInnerSpacing.x);
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        if (ImGui::Button(r < 0 ? "-" : "+", ImVec2{knopf, knopf})) {
            wert += r * ganzSchritt(wert, r);
            geaendert = true;
        }
        ImGui::PopItemFlag();
    }
    ImGui::PopID();
    wert = std::clamp(wert, von, bis);
    return geaendert;
}

// Stufe 5 (Abweichung, NRC hat keinen Optimierer): die Seite "Optimizer" im
// Stil der Seiten oben - ab wann "knapp", nach jedem Bau pruefen, das
// Regelwerk und die Schwellen der Faustregel-Hinweise (VIS, Dreiecke).
void optimiererSeite() {
    formAnfang({Str::KePrOptKnapp, Str::KePrOptRegelwerk, Str::KePrOptPortale, Str::KePrOptDreiecke});
    feldAnfang(Str::KePrOptKnapp);
    (void)ganzSpinner("##optknapp", g_entwurf.optKnapp, 50, 100);
    feldEnde();
    (void)haken(Str::Count, (std::string(trK(Str::KePrOptNachBau)) + "##optnachbau").c_str(), g_entwurf.optNachBau);
    feldAnfang(Str::KePrOptRegelwerk);
    {
        const std::string vorschau = g_entwurf.optRegelwerk < 0 ? std::string(trK(Str::KeOptAuto))
                                                                : karte::grenzSpielName(static_cast<karte::GrenzSpiel>(g_entwurf.optRegelwerk));
        if (ImGui::BeginCombo("##optregelwerkseite", vorschau.c_str())) {
            if (ImGui::Selectable(trK(Str::KeOptAuto), g_entwurf.optRegelwerk < 0)) { g_entwurf.optRegelwerk = -1; }
            for (int i = 0; i < karte::kGrenzSpiele; ++i) {
                if (ImGui::Selectable(karte::grenzSpielName(static_cast<karte::GrenzSpiel>(i)), g_entwurf.optRegelwerk == i)) { g_entwurf.optRegelwerk = i; }
            }
            ImGui::EndCombo();
        }
    }
    feldEnde();
    feldAnfang(Str::KePrOptPortale);
    (void)ganzSpinner("##optportale", g_entwurf.optPortale, 100, 1000000);
    feldEnde();
    feldAnfang(Str::KePrOptDreiecke);
    (void)ganzSpinner("##optdreiecke", g_entwurf.optDreiecke, 1000, 10000000);
    feldEnde();
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
    // Detachable Menus (g_Layout_enableDetachableMenus): Abreisskante und QTornOffMenu
    (void)haken(Str::Count, (std::string(trK(Str::KePrDetachableMenus)) + "##abreissbar").c_str(), g_entwurf.abtrennbar);
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
        (void)ganzSpinner((std::string("##") + trK(text)).c_str(), wert, von, bis);
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
    (void)ganzSpinner("##waehlergroesse", g_entwurf.waehlEpsilon, 2, 64);
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
    (void)ganzSpinner("##texmax", t.einheitsGroesse, 16, 8192);
    feldEnde();
    feldAnfang(Str::KePrTbMin);
    (void)ganzSpinner("##texmin", t.einheitsMin, 16, 8192);
    feldEnde();
    feldAnfang(Str::KePrTbWheel);
    (void)ganzSpinner("##texrad", t.radSchritt, 0, 8192);
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
    (void)ganzSpinner("##namenabstand", g_entwurf.namenAbstand, 0, 200500);
    feldEnde();
    feldAnfang(Str::KePrEnRatio);
    (void)ganzSpinner("##namenverhaeltnis", g_entwurf.namenVerhaeltnis, 0, 100500);
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

// Patch_constructPreferences (patchmanip.cpp:670-672): appendSpinner
// "Patch Subdivide Threshold" 0..128 (Foto pref_Patches.png).
void patchSeite() {
    formAnfang({Str::KePrPatchThreshold});
    feldAnfang(Str::KePrPatchThreshold);
    (void)ganzSpinner("##patchschwelle", g_entwurf.patchSchwelle, 0, 128);
    feldEnde();
}

// Textures_constructPreferences (textures.cpp:761-829; Foto pref_Textures.png):
// appendRadio "Texture Quality" (RadioHBox in der Feldspalte), appendSpinner
// "Texture Gamma" 0..5, appendCombo "Texture Render Mode", appendCombo
// "Hardware Texture Compression" (D3D11 meldet weder OpenGL- noch S3TC-
// Kompression: nur "None", wie NRC ohne die Erweiterungen), Checkbox
// "Anisotropy".
void texturenSeite() {
    Editor::TexturQualitaet& q = g_entwurf.texQual;
    formAnfang({Str::KePrTexQuality, Str::KePrTexGamma, Str::KePrTexMode, Str::KePrTexCompression});
    formZeile(Str::KePrTexQuality, kHakenZeile);
    {
        static const char* const prozent[4] = {"100%", "50%", "25%", "12.5%"};
        float summe = 0.0F;
        for (const char* p : prozent) { summe += 14.0F + 10.0F + static_cast<float>(labelBreite(p)); }
        summe += 18.0F;
        const float rest = std::max(0.0F, static_cast<float>(g_form.b1) - summe) / 4.0F;
        float x = static_cast<float>(g_form.x1);
        for (int i = 0; i < 4; ++i) {
            radioKnopf((std::string(prozent[i]) + "##texqual" + std::to_string(i)).c_str(), q.mipStufe, i, x, static_cast<float>(kHakenZeile));
            x += 24.0F + static_cast<float>(labelBreite(prozent[i])) + rest + 6.0F;
        }
    }
    formWeiter(kHakenZeile);
    feldAnfang(Str::KePrTexGamma);
    ImGui::InputFloat("##texgamma", &q.gamma, 0.1F, 1.0F, "%.2f");
    q.gamma = std::clamp(q.gamma, 0.0F, 5.0F);
    feldEnde();
    feldAnfang(Str::KePrTexMode);
    const char* modi[6] = {trK(Str::KePrTexNearest), trK(Str::KePrTexNearestMip), trK(Str::KePrTexLinear), trK(Str::KePrTexBilinear),
                           trK(Str::KePrTexBilinearMip), trK(Str::KePrTexTrilinear)};
    int index = karte::texturModusZuListe(q.modus);
    if (ImGui::Combo("##texmodus", &index, modi, 6)) { q.modus = karte::texturModusAusListe(index); }
    feldEnde();
    feldAnfang(Str::KePrTexCompression);
    const char* keine[1] = {trK(Str::KePrTexNone)};
    int k = 0;
    (void)ImGui::Combo("##texkompression", &k, keine, 1);
    q.kompression = 0;
    feldEnde();
    (void)haken(Str::Count, trK(Str::KePrTexAnisotropy), q.anisotropie);
}

// ModelBrowser_constructPage (modelwindow.cpp:1363-1371; Foto pref_Model_Browser.png):
// appendSpinner "Model View Size" 16..8192, appendEntry "List of *folderToLoad/depth*".
void modBroSeite() {
    formAnfang({Str::KePrModViewSize, Str::KePrModFolders});
    feldAnfang(Str::KePrModViewSize);
    (void)ganzSpinner("##modzelle", g_entwurf.modZelle, 16, 8192);
    feldEnde();
    feldAnfang(Str::KePrModFolders);
    ImGui::InputText("##modordner", g_entwurf.modOrdner, sizeof(g_entwurf.modOrdner));
    feldEnde();
}

// Autosave_constructPreferences (autosave.cpp:167-173; Foto pref_Autosave.png):
// appendCheckBox "Enable Autosave", appendSpinner "Autosave Frequency
// (minutes)" 1..60 (Widget_connectToggleDependency: nur mit dem Haken),
// appendCheckBox "Save Snapshots".
void autosaveSeite() {
    formAnfang({Str::KePrAutosaveFreq});
    (void)haken(Str::Count, (std::string(trK(Str::KePrAutosaveEnable)) + "##autosavean").c_str(), g_entwurf.autosaveAn);
    feldAnfang(Str::KePrAutosaveFreq);
    if (!g_entwurf.autosaveAn) { ImGui::BeginDisabled(); }
    (void)ganzSpinner("##autosaveminuten", g_entwurf.autosaveMinuten, 1, 60);
    if (!g_entwurf.autosaveAn) { ImGui::EndDisabled(); }
    feldEnde();
    (void)haken(Str::Count, (std::string(trK(Str::KePrAutosaveSnapshots)) + "##schnappschuesse").c_str(), g_entwurf.schnappschuesse);
}

// SurfaceInspector_constructPreferences (surfacedialog.cpp:1734-1736): eine Checkbox
void surfaceSeite() {
    formAnfang({});
    (void)haken(Str::Count, trK(Str::KePrSiMatchGrid), g_entwurf.siRaster);
}

// QComboBox: ImGui::Combo, die Lage fuer den Selbsttest gemerkt
bool klappe(const char* id, int* wert, const char* const* eintraege, int anzahl) {
    const bool r = ImGui::Combo(id, wert, eintraege, anzahl);
    const ImVec2 a = ImGui::GetItemRectMin();
    const ImVec2 b = ImGui::GetItemRectMax();
    editor().klappOrte.push_back({id, a.x, a.y, b.x, b.y});
    return r;
}

// QDoubleSpinBoxPrivate::calculateAdaptiveDecimalStep (Qt 5.15 qspinbox.cpp):
// addSpinner setzt QAbstractSpinBox::AdaptiveDecimalStepType (dialog.cpp:521).
double adaptiverSchritt(double wert, int schritte, int stellen) {
    const double minSchritt = std::pow(10.0, -stellen);
    double betrag = std::fabs(wert);
    if (betrag < minSchritt) { return minSchritt; }
    if ((wert < 0) != (schritte < 0)) { betrag /= 1.01; }
    const double verschiebung = std::pow(10.0, 1.0 - std::floor(std::log10(betrag)));
    const double gerundet = std::round(betrag * verschiebung) / verschiebung;
    const double lg = static_cast<double>(std::floor(static_cast<float>(std::log10(gerundet)))) - 1.0;   // floorf
    return std::max(minSchritt, std::pow(10.0, lg));
}

// DoubleSpinBox (appendSpinner mit float): Feld mit "stellen" Nachkommastellen,
// Pfeile mit dem anpassenden Schritt; QDoubleSpinBox rundet auf die Stellen.
bool kommaSpinner(const char* id, float& wert, float von, float bis, int stellen) {
    const ImGuiStyle& st = ImGui::GetStyle();
    const float knopf = ImGui::GetFrameHeight();
    const float gesamt = px(static_cast<float>(g_form.b1));
    bool geaendert = false;
    ImGui::PushID(id);
    ImGui::SetNextItemWidth(std::max(1.0F, gesamt - 2.0F * (knopf + st.ItemInnerSpacing.x)));
    char format[16];
    std::snprintf(format, sizeof(format), "%%.%df", stellen);
    geaendert = ImGui::InputFloat("##wert", &wert, 0.0F, 0.0F, format);
    for (const int r : {-1, 1}) {
        ImGui::SameLine(0.0F, st.ItemInnerSpacing.x);
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        if (ImGui::Button(r < 0 ? "-" : "+", ImVec2{knopf, knopf})) {
            wert = static_cast<float>(static_cast<double>(wert) + r * adaptiverSchritt(wert, r, stellen));
            geaendert = true;
        }
        ImGui::PopItemFlag();
    }
    ImGui::PopID();
    const double faktor = std::pow(10.0, stellen);
    wert = static_cast<float>(std::round(static_cast<double>(std::clamp(wert, von, bis)) * faktor) / faktor);
    return geaendert;
}

// PathEntry_new (gtkutil/dialog.cpp:49-53): QLineEdit mit der Aktion
// "ellipsis.png" am Ende; button_clicked_entry_browse_directory waehlt einen Ordner.
void pfadEintrag(Str text, const char* id, char* puffer, std::size_t groesse, bool ordner = true) {
    feldAnfang(text);
    const float knopf = ImGui::GetFrameHeight();
    const float gesamt = px(static_cast<float>(g_form.b1));
    // Die Aktion liegt im Rahmen des QLineEdit: das Textfeld endet vor ihr, der
    // Knopf hat den Feldhintergrund. QLineEdit::setText setzt den Cursor ans
    // Ende - ohne Fokus ist das Ende eines langen Pfads zu sehen.
    const bool aktiv = ImGui::GetActiveID() == ImGui::GetID(id);
    if (!aktiv) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0, 0, 0, 0}); }
    ImGui::SetNextItemWidth(gesamt - knopf);
    ImGui::InputText(id, puffer, groesse);
    if (!aktiv) { ImGui::PopStyleColor(); }
    const ImVec2 anfang = ImGui::GetItemRectMin();
    const ImVec2 ende = ImGui::GetItemRectMax();
    if (!aktiv) {
        const ImGuiStyle& st = ImGui::GetStyle();
        const float platz = ende.x - anfang.x - 2.0F * st.FramePadding.x;
        const float breite = ImGui::CalcTextSize(puffer).x;
        const float x = breite > platz ? ende.x - st.FramePadding.x - breite : anfang.x + st.FramePadding.x;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->PushClipRect(ImVec2{anfang.x + st.FramePadding.x, anfang.y}, ImVec2{ende.x - st.FramePadding.x, ende.y}, true);
        dl->AddText(ImVec2{x, anfang.y + st.FramePadding.y}, ImGui::GetColorU32(ImGuiCol_Text), puffer);
        dl->PopClipRect();
    }
    ImGui::SetCursorScreenPos(ImVec2{ende.x, anfang.y});
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0F);
    const ImVec2 knopfA{ende.x, anfang.y};
    if (ImGui::Button((std::string("##") + id + "knopf").c_str(), ImVec2{knopf, ende.y - anfang.y})) {
        // button_clicked_entry_browse_directory / _file (gtkutil/dialog.cpp)
        // ueber kartenOrdnerDialog/kartenDateiDialog: im Selbsttest ersetzbar (nie ein Windows-Dialog im Test)
        const std::string neu = ordner ? kartenOrdnerDialog(trK(text), puffer) : kartenDateiDialog(true, trK(text), "*.*|*.*", puffer);
        if (!neu.empty()) { std::snprintf(puffer, groesse, "%s", neu.c_str()); }
    }
    {   // ellipsis.svg: drei Kreise (24/64/104 bei r 12 auf 128), #C0C0C0
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float h = ende.y - anfang.y;
        const float k = std::min(knopf, h) * 0.6F;
        const float x0 = knopfA.x + (knopf - k) * 0.5F;
        for (const float cx : {24.0F, 64.0F, 104.0F}) {
            dl->AddCircleFilled(ImVec2{x0 + cx / 128.0F * k, anfang.y + h * 0.5F}, 12.0F / 128.0F * k, IM_COL32(0xC0, 0xC0, 0xC0, 255));
        }
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    feldEnde();
}

// CGameDialog::CreateGlobalFrame (preferences.cpp:252-268; Foto pref_Global_Game.png):
// appendCombo "Select the game" (die name-Attribute), appendCheckBox "Startup".
void globalGameSeite() {
    formAnfang({Str::KePrSelectGame, Str::KePrStartup});
    feldAnfang(Str::KePrSelectGame);
    std::vector<const char*> namen;
    for (const karte::SpielBeschreibung& b : karte::spielBeschreibungen()) { namen.push_back(b.name); }
    (void)klappe("##spielwahl", &g_entwurf.spielWahl, namen.data(), static_cast<int>(namen.size()));
    feldEnde();
    (void)haken(Str::KePrStartup, (std::string(trK(Str::KePrShowGlobal)) + "##spielfrage").c_str(), g_entwurf.spielFrage);
}

// Game_constructPreferences (gtkdlgs.cpp:226-271; Foto pref_Game.png): "Select mod",
// "fs_game" (editierbar, die Ordner im Engine-Pfad), "Mapping mode" (show_gamemode).
void gameSeite() {
    Entwurf& w = g_entwurf;
    const karte::SpielBeschreibung& sb = kartenSpiel();
    formAnfang({Str::KePrSelectMod, Str::KePrFsGame, Str::KePrMappingMode});
    feldAnfang(Str::KePrSelectMod);
    const std::vector<std::string> mods = karte::modNamen(sb);
    std::vector<const char*> modZeiger;
    for (const std::string& m : mods) { modZeiger.push_back(m.c_str()); }
    if (klappe("##modwahl", &w.modWahl, modZeiger.data(), static_cast<int>(modZeiger.size()))) {
        // currentTextChanged: gamecombo_for_gamename
        const karte::SpielAuswahl a = karte::spielAuswahlFuerName(sb, mods[static_cast<std::size_t>(w.modWahl)]);
        std::snprintf(w.fsGame, sizeof(w.fsGame), "%s", a.fsGame.c_str());
        w.fsGameAktiv = a.bearbeitbar;
    }
    feldEnde();
    feldAnfang(Str::KePrFsGame);
    {
        const float knopf = ImGui::GetFrameHeight();
        const float gesamt = px(static_cast<float>(g_form.b1));
        if (!w.fsGameAktiv) { ImGui::BeginDisabled(); }
        ImGui::SetNextItemWidth(gesamt - knopf);
        ImGui::InputText("##fsgame", w.fsGame, sizeof(w.fsGame));
        ImGui::SameLine(0.0F, 0.0F);
        if (ImGui::ArrowButton("##fsgameliste", ImGuiDir_Down)) { ImGui::OpenPopup("##fsgameordner"); }
        if (ImGui::BeginPopup("##fsgameordner")) {
            // for( entry : directory_iterator( EnginePath_get() ) ) if( is_directory ) addItem
            std::error_code ec;
            const std::string engine = enginePfadAnzeige();
            for (const auto& d : std::filesystem::directory_iterator(std::filesystem::path(std::u8string(
                                                                         reinterpret_cast<const char8_t*>(engine.data()), engine.size())),
                                                                     std::filesystem::directory_options::skip_permission_denied, ec)) {
                std::error_code ec2;
                if (!d.is_directory(ec2)) { continue; }
                const std::u8string n = d.path().filename().u8string();
                const std::string name(n.begin(), n.end());
                if (ImGui::Selectable((name + "##fsgame" + name).c_str())) { std::snprintf(w.fsGame, sizeof(w.fsGame), "%s", name.c_str()); }
            }
            ImGui::EndPopup();
        }
        if (!w.fsGameAktiv) { ImGui::EndDisabled(); }
    }
    feldEnde();
    if (sb.spielModusZeigen) {   // globalMappingMode().do_mapping_mode
        feldAnfang(Str::KePrMappingMode);
        const char* modi[2] = {trK(Str::KePrSpMode), trK(Str::KePrMpMode)};
        (void)klappe("##spielmodus", &w.modusWahl, modi, 2);
        feldEnde();
    }
}

// Paths_constructPreferences / Paths_constructPage (mainframe.cpp:396-411; Foto
// pref_Paths.png): appendPathEntry "Engine Path", fuenfmal "Extra Resource Path".
void pathsSeite() {
    formAnfang({Str::KePrEnginePath, Str::KePrExtraPath});
    pfadEintrag(Str::KePrEnginePath, "##enginepfad", g_entwurf.enginePfad, sizeof(g_entwurf.enginePfad));
    for (int i = 0; i < 5; ++i) {
        const std::string id = "##zusatzpfad" + std::to_string(i);
        pfadEintrag(Str::KePrExtraPath, id.c_str(), g_entwurf.zusatzPfade[i], sizeof(g_entwurf.zusatzPfade[i]));
    }
}

// Nudge_constructPreferences (select.cpp:1920-1922) und Map_constructPreferences
// (map.cpp:2369-2371) auf der Seite "Settings" (Foto pref_Settings.png).
void settingsSeite() {
    formAnfang({});
    (void)haken(Str::Count, (std::string(trK(Str::KePrNudgeClone)) + "##nachklonen").c_str(), g_entwurf.nachKlonen);
    (void)haken(Str::Count, (std::string(trK(Str::KePrLoadLastMap)) + "##letztekarte").c_str(), g_entwurf.letzteKarte);
}

// Undo_constructPreferences (undo.cpp:399-401): appendSpinner 0..4096 (Foto pref_Undo.png)
void undoSeite() {
    formAnfang({Str::KePrUndoSize});
    feldAnfang(Str::KePrUndoSize);
    (void)ganzSpinner("##undostufen", g_entwurf.undoStufen, 0, 4096);
    feldEnde();
}

// Brush_constructPreferences (brushmodule.cpp:85-108; Foto pref_Brush.png):
// appendSpinner "Default texture scale" 0.0625..64 mit 4 Stellen, appendCombo
// "New map Brush Type" (g_multipleBrushTypes), zwei Checkboxen.
void brushSeite() {
    formAnfang({Str::KePrBrushScale, Str::KePrBrushType, Str::KePrBrushDanger});
    feldAnfang(Str::KePrBrushScale);
    (void)kommaSpinner("##texskalavorgabe", g_entwurf.texSkala, 0.0625F, 64.0F, 4);
    feldEnde();
    feldAnfang(Str::KePrBrushType);
    // BrushType_getName: Axial Projection, Brush Primitives, Valve 220
    const char* typen[3] = {trK(Str::KePrBrushAxial), trK(Str::KePrBrushBp), trK(Str::KePrBrushValve)};
    (void)klappe("##pinseltyp", &g_entwurf.pinselTyp, typen, 3);
    feldEnde();
    (void)haken(Str::Count, (std::string(trK(Str::KePrBrushCaulk)) + "##immercaulk").c_str(), g_entwurf.immerCaulk);
    (void)haken(Str::KePrBrushDanger, (std::string(trK(Str::KePrBrushSnap)) + "##ganzzahlig").c_str(), g_entwurf.ganzzahlig);
}

// Grid_constructPreferences (grid.cpp:239-256; Foto pref_Grid.png): appendCombo
// "Default grid spacing" (g_gridnames), "Max grid coordinate" 4096..65536.
void gridSeite() {
    formAnfang({Str::KePrGridDefault, Str::KePrGridMax});
    feldAnfang(Str::KePrGridDefault);
    const char* namen[14];
    for (int i = 0; i < 14; ++i) { namen[i] = karte::Raster::name(i); }
    (void)klappe("##rasterstandard", &g_entwurf.rasterStandard, namen, 14);
    feldEnde();
    feldAnfang(Str::KePrGridMax);
    static const char* const koordinaten[5] = {"4096", "8192", "16384", "32768", "65536"};
    int index = std::clamp(g_entwurf.rasterMax, 0, 4);   // maxGridCoordPowerExport
    if (klappe("##rastermax", &index, koordinaten, 5)) { g_entwurf.rasterMax = index; }
    feldEnde();
}

// Orthographic_constructPreferences (xywindow.cpp:2236-2251; Foto pref_Orthographic.png)
void orthoSeite() {
    formAnfang({Str::KePrCamMsaa});
    (void)haken(Str::Count, (std::string(trK(Str::KePrOrthoStipple)) + "##keinestrichelung").c_str(), g_entwurf.keineStrichelung);
    (void)haken(Str::Count, (std::string(trK(Str::KePrOrthoChase)) + "##jagen").c_str(), g_entwurf.jagen);
    (void)haken(Str::Count, (std::string(trK(Str::KePrCamZoomPointer)) + "##zumzeiger2d").c_str(), g_entwurf.zumZeiger2d);
    feldAnfang(Str::KePrCamMsaa);
    static const char* const abtastungen[6] = {"0", "2", "4", "8", "16", "32"};
    int index = 0;   // MSAAExport
    for (int i = 1; i < 6; ++i) {
        if (g_entwurf.xyMsaa == (1 << i)) { index = i; }
    }
    if (klappe("##xymsaa", &index, abtastungen, 6)) { g_entwurf.xyMsaa = index != 0 ? 1 << index : 0; }   // MSAAImport
    feldEnde();
}

// Ein QLineEdit der Seite (appendEntry), gesperrt mit Widget_connectToggleDependency.
void eintrag(Str text, const char* id, char* puffer, std::size_t groesse, bool aktiv = true) {
    feldAnfang(text);
    if (!aktiv) { ImGui::BeginDisabled(); }
    ImGui::InputText(id, puffer, groesse);
    if (!aktiv) { ImGui::EndDisabled(); }
    feldEnde();
}

// Build_constructPreferences (watchbsp.cpp:220-241; Foto pref_Build.png): Leak
// Stop haengt an der Ueberwachung, die vier Felder an "Run Engine After
// Compile"; die MP-Felder, weil ja.game/jk2.game show_gamemode setzen.
void buildSeite() {
    Entwurf& w = g_entwurf;
    formAnfang({Str::KePrBuildEngine, Str::KePrBuildArgs, Str::KePrBuildMpEngine, Str::KePrBuildMpArgs, Str::KePrBuildRegion, Str::KePrBuildZiel});
    (void)haken(Str::Count, (std::string(trK(Str::KePrBuildMonitor)) + "##bauueberwachen").c_str(), w.bau.ueberwachen);
    (void)haken(Str::Count, (std::string(trK(Str::KePrBuildLeak)) + "##bauleck").c_str(), w.bau.leckStopp, w.bau.ueberwachen);
    (void)haken(Str::Count, (std::string(trK(Str::KePrBuildRun)) + "##baustarten").c_str(), w.bau.spielStarten);
    eintrag(Str::KePrBuildEngine, "##bauengine", w.bauEngine, sizeof(w.bauEngine), w.bau.spielStarten);
    eintrag(Str::KePrBuildArgs, "##bauargumente", w.bauArgumente, sizeof(w.bauArgumente), w.bau.spielStarten);
    if (kartenSpiel().spielModusZeigen) {
        eintrag(Str::KePrBuildMpEngine, "##bauenginemp", w.bauEngineMp, sizeof(w.bauEngineMp), w.bau.spielStarten);
        eintrag(Str::KePrBuildMpArgs, "##bauargumentemp", w.bauArgumenteMp, sizeof(w.bauArgumenteMp), w.bau.spielStarten);
    }
    (void)haken(Str::Count, (std::string(trK(Str::KePrBuildDump)) + "##baulog").c_str(), w.bau.logAusgeben);
    eintrag(Str::KePrBuildRegion, "##regionshader", w.regionShader, sizeof(w.regionShader));
    // Abweichung (Auftrag): wohin gebaut wird - nie in den Spielordner
    eintrag(Str::KePrBuildZiel, "##bauziel", w.bauZiel, sizeof(w.bauZiel));
    {
        const float b = px(static_cast<float>(g_form.b1));
        ImVec2 p = bp(static_cast<float>(g_form.x1), static_cast<float>(g_form.y));
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), p, ImGui::GetColorU32(ImGuiCol_Text),
                                            trK(Str::KePrBuildZielHinweis), nullptr, b);
        ImGui::PopStyleColor();
        (void)p;
    }
}

void interfaceSeite() {
    // Interface Preferences: "Shader Editor Command" (preferences.cpp:64,
    // appendPathEntry( browse_directory = false ): Feld mit Dateiwahl; Foto
    // pref_Interface.png), dazu die Einstellung dieses Programms "Begriffe wie NetRadiant".
    formAnfang({Str::KePrShaderEditor});
    pfadEintrag(Str::KePrShaderEditor, "##shadereditor", g_entwurf.shaderEditor, sizeof(g_entwurf.shaderEditor), false);
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

// CGameDialog::BuildDialog (preferences.cpp:271-295): "Global Preferences", feste
// Groesse (SetFixedSize), QGroupBox "Game settings" mit dem Gitter 111 : 333 und
// CreateGlobalFrame, darunter QDialogButtonBox mit OK. Kein Foto - die Lagen aus
// den Qt-Massen der Einstellungen (Rand 11, Abstand 6, Rahmen ab 17, Inhalt ab
// 28, innen 12) und den sizeHints (QComboBox: Text + 30, QCheckBox: 14 + 5 + Text).
void zeichneGlobaleEinstellungen() {
    Editor& ed = editor();
    static bool offen = false;
    static int wahl = 0;
    static bool frage = false;
    if (!ed.globaleEinstellungenOffen) {
        offen = false;
        return;
    }
    if (!offen) {
        offen = true;
        frage = ed.spielFrage;
        wahl = 0;
        const auto& alle = karte::spielBeschreibungen();
        for (std::size_t i = 0; i < alle.size(); ++i) {
            if (std::string(alle[i].datei) == kartenSpiel().datei) { wahl = static_cast<int>(i); }
        }
    }
    const float sk = massstab();
    g_sk = sk;
    int b0 = std::max(labelBreite(trK(Str::KePrSelectGame)), labelBreite(trK(Str::KePrStartup)));
    int b1 = 14 + 5 + labelBreite(trK(Str::KePrShowGlobal));
    for (const karte::SpielBeschreibung& b : karte::spielBeschreibungen()) { b1 = std::max(b1, labelBreite(b.name) + 30); }
    const int gruppeB = kInhaltRand + b0 + kAbstand + b1 + kInhaltRand;
    const int gruppeH = kInhaltOben + kFeldH + kAbstand + kHakenZeile + kInhaltRand;
    const int innenB = kRand + gruppeB + kRand;
    const int innenH = kRand + gruppeH + kAbstand + kKnopfH + kRand;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowSize(ImVec2{px(static_cast<float>(innenB)), px(static_cast<float>(innenH)) + titelH}, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_Appearing,
                            ImVec2{0.5F, 0.5F});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool sichtbar = ImGui::Begin((std::string(trK(Str::KeGlobalPrefsTitle)) + "###karteglobal").c_str(), nullptr,
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    if (sichtbar) {
        g_innen = ImGui::GetCursorScreenPos();
        dialogSchriftAn();
        ImDrawList* fl = ImGui::GetWindowDrawList();
        fl->AddText(bp(static_cast<float>(kRand + kTitelX), static_cast<float>(kRand)), ImGui::GetColorU32(ImGuiCol_Text), trK(Str::KeGameSettingsGroup));
        fl->AddRect(bp(static_cast<float>(kRand), static_cast<float>(kRand + kRahmenOben)),
                    bp(static_cast<float>(kRand + gruppeB), static_cast<float>(kRand + gruppeH)), ImGui::GetColorU32(ImGuiCol_Border),
                    ImGui::GetStyle().FrameRounding);
        Form f;
        f.x0 = kRand + kInhaltRand;
        f.b0 = b0;
        f.x1 = f.x0 + b0 + kAbstand;
        f.b1 = b1;
        f.y = kRand + kInhaltOben;
        g_form = f;
        feldAnfang(Str::KePrSelectGame);
        std::vector<const char*> namen;
        for (const karte::SpielBeschreibung& b : karte::spielBeschreibungen()) { namen.push_back(b.name); }
        (void)klappe("##globalspielwahl", &wahl, namen.data(), static_cast<int>(namen.size()));
        feldEnde();
        (void)haken(Str::KePrStartup, (std::string(trK(Str::KePrShowGlobal)) + "##globalspielfrage").c_str(), frage);
        const float knopfH = px(static_cast<float>(kKnopfH));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (knopfH - ImGui::GetTextLineHeight()) * 0.5F)});
        ImGui::SetCursorScreenPos(bp(static_cast<float>(innenB - kRand - kKnopfB), static_cast<float>(innenH - kRand - kKnopfH)));
        if (ImGui::Button((std::string(trK(Str::KePrefOk)) + "##globalok").c_str(), ImVec2{px(static_cast<float>(kKnopfB)), knopfH})) {
            // GameFileAssign (global = true): sofort, kein Neustart; DoGameDialog -> SavePrefs
            ed.globaleEinstellungenOffen = false;
            ed.spielFrage = frage;
            const std::string neu = karte::spielBeschreibungen()[static_cast<std::size_t>(wahl)].datei;
            einstellungenSpeichern();
            if (neu != kartenSpiel().datei) {
                spielNeuRealisieren("Global Preferences", [neu] {
                    editor().spielDatei = neu;
                    editor().spielDateiNeu.clear();
                    einstellungenSpeichern();
                });
            }
        }
        ImGui::PopStyleVar();
        dialogSchriftAus();
        ImGui::SetCursorScreenPos(bp(0.0F, static_cast<float>(innenH)));
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
    }
    ImGui::End();
}

// --- DoAbout (gtkdlgs.cpp:333-405; Foto dlg_Help_About_0.png) -----------------------
//
// QVBoxLayout: QHBoxLayout (Logo, Text, senkrechte QDialogButtonBox OK / Credits
// (gesperrt) / Changelog / About Qt), QGroupBox "OpenGL Properties" (QFormLayout
// Vendor, Version, Renderer), QGroupBox "OpenGL Extensions" (QPlainTextEdit). Am
// Foto (2x) gemessen: Innenraum 391 x 488, Rand 11, Logo 128 (halbe Bildgroesse),
// Text ab 145, Knoepfe 80 x 22 alle 28 bei x 300, Gruppen bei 145 und 253.
// Dieses Programm zeichnet mit Direct3D 11: die Gruppen zeigen das Geraet.
namespace {

void sonnenLogo(ImDrawList* dl, ImVec2 o, float g) {
    // logo.svg ("sun"): acht Strahlen um einen Kreis, Orange #FF8000 mit dunklem
    // Rand - im Strichstil des Programms
    const ImVec2 m{o.x + g * 0.5F, o.y + g * 0.5F};
    const ImU32 orange = IM_COL32(0xFF, 0x80, 0x00, 255);
    const ImU32 rand = IM_COL32(0, 0, 0, 255);
    for (int i = 0; i < 8; ++i) {
        const float w = static_cast<float>(i) * 3.14159265F / 4.0F + 0.32F;
        const float c = std::cos(w);
        const float s = std::sin(w);
        const ImVec2 spitze{m.x + c * g * 0.47F, m.y + s * g * 0.47F};
        const ImVec2 l{m.x - s * g * 0.09F + c * g * 0.12F, m.y + c * g * 0.09F + s * g * 0.12F};
        const ImVec2 r{m.x + s * g * 0.09F + c * g * 0.12F, m.y - c * g * 0.09F + s * g * 0.12F};
        const ImVec2 eck{m.x + c * g * 0.36F - s * g * 0.12F, m.y + s * g * 0.36F + c * g * 0.12F};
        const ImVec2 pts[4] = {l, eck, spitze, r};
        dl->AddConvexPolyFilled(pts, 4, orange);
        dl->AddPolyline(pts, 4, rand, ImDrawFlags_Closed, std::max(1.0F, g / 48.0F));
    }
    dl->AddCircleFilled(m, g * 0.15F, orange);
    dl->AddCircle(m, g * 0.15F, rand, 0, std::max(1.0F, g / 48.0F));
}

}  // namespace

void zeichneUeber() {
    Editor& ed = editor();
    if (!ed.ueberOffen) { return; }
    const float sk = massstab();
    g_sk = sk;
    constexpr int kB = 391;
    constexpr int kH = 488;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowSize(ImVec2{px(static_cast<float>(kB)), px(static_cast<float>(kH)) + titelH}, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_Appearing,
                            ImVec2{0.5F, 0.5F});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    bool offen = true;
    const bool sichtbar = ImGui::Begin((std::string(trK(Str::KeAboutTitle)) + "###karteueber").c_str(), &offen,
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();
    if (sichtbar) {
        g_innen = ImGui::GetCursorScreenPos();
        dialogSchriftAn();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        sonnenLogo(dl, bp(static_cast<float>(kRand), static_cast<float>(kRand)), px(128.0F));
        // RADIANT_VERSION, __DATE__, RADIANT_ABOUTMSG, GPL
        const std::string text = std::string("NetRadiant-Custom 20260114\n") + __DATE__ + "\n\nbehaved " + std::string(bhed::kFassung) +
                                 "\n\nThis program is free software\nlicensed under the GNU GPL.\n";
        const float textH = ImGui::CalcTextSize(text.c_str()).y;
        dl->AddText(ImVec2{bp(145.0F, 0.0F).x, std::round(bp(0.0F, static_cast<float>(kRand)).y + (px(128.0F) - textH) * 0.5F)},
                    ImGui::GetColorU32(ImGuiCol_Text), text.c_str());
        const float knopfH = px(static_cast<float>(kKnopfH));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (knopfH - ImGui::GetTextLineHeight()) * 0.5F)});
        const auto knopf = [&](Str t, const char* id, int zeile, bool aktiv) {
            ImGui::SetCursorScreenPos(bp(static_cast<float>(kB - kRand - kKnopfB), static_cast<float>(kRand + zeile * (kKnopfH + kAbstand))));
            if (!aktiv) { ImGui::BeginDisabled(); }
            const bool r = ImGui::Button((std::string(trK(t)) + id).c_str(), ImVec2{px(static_cast<float>(kKnopfB)), knopfH});
            if (!aktiv) { ImGui::EndDisabled(); }
            return r;
        };
        if (knopf(Str::KePrefOk, "##ueberok", 0, true)) { offen = false; }
        (void)knopf(Str::KeAboutCredits, "##uebercredits", 1, false);   // credits.html: button->setEnabled( false )
        if (knopf(Str::KeAboutChangelog, "##uebernotizen", 2, true)) {   // docs/changelog-custom.txt: hier die Aenderungen des Programms
            kartenUrlOeffnen(directoryOf(g_app->dataDir) + "/AENDERUNGEN.md");
        }
        if (knopf(Str::KeAboutQt, "##ueberqt", 3, true)) {   // QApplication::aboutQt (QMessageBox): hier die Oberflaechenbibliothek
            qtMeldung(trK(Str::KeAboutQt),
                      std::string("Dear ImGui ") + IMGUI_VERSION + "\n\nCopyright (c) 2014-2025 Omar Cornut\nMIT License\nhttps://github.com/ocornut/imgui",
                      MeldungArt::Info, false, nullptr);
        }
        ImGui::PopStyleVar();
        // QGroupBox "OpenGL Properties"
        const render::GeraeteInfo info = render::geraeteInfo();
        const auto gruppe = [&](Str titel, int y, int h) {
            dl->AddText(bp(static_cast<float>(kRand + kTitelX), static_cast<float>(y)), ImGui::GetColorU32(ImGuiCol_Text), trK(titel));
            dl->AddRect(bp(static_cast<float>(kRand), static_cast<float>(y + kRahmenOben)), bp(static_cast<float>(kB - kRand), static_cast<float>(y + h)),
                        ImGui::GetColorU32(ImGuiCol_Border), ImGui::GetStyle().FrameRounding);
        };
        gruppe(Str::KeAboutGlProps, 145, 94);
        const Str namen[3] = {Str::KeAboutVendor, Str::KeAboutVersion, Str::KeAboutRenderer};
        const std::string werte[3] = {info.hersteller, info.version, info.karte};
        int b0 = 0;
        for (const Str n : namen) { b0 = std::max(b0, labelBreite(trK(n))); }
        for (int i = 0; i < 3; ++i) {   // QFormLayout: Zeilen 20, Beschriftung links, Abstand 6
            const float y = static_cast<float>(145 + kInhaltOben + 2 + i * 20);
            dl->AddText(bp(static_cast<float>(kRand + kInhaltRand), y), ImGui::GetColorU32(ImGuiCol_Text), trK(namen[i]));
            dl->AddText(bp(static_cast<float>(kRand + kInhaltRand + b0 + kAbstand), y), ImGui::GetColorU32(ImGuiCol_Text), werte[i].c_str());
        }
        // QGroupBox "OpenGL Extensions" mit dem QPlainTextEdit (nur lesen, umbrochen)
        gruppe(Str::KeAboutGlExt, 253, 223);
        std::string merkmale;
        for (const std::string& m : info.merkmale) { merkmale += (merkmale.empty() ? "" : " ") + m; }
        ImGui::SetCursorScreenPos(bp(static_cast<float>(kRand + kInhaltRand), 272.0F));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        ImGui::BeginChild("##uebermerkmale", ImVec2{px(static_cast<float>(kB - 2 * kRand - 2 * kInhaltRand)), px(191.0F)}, ImGuiChildFlags_Borders);
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextUnformatted(merkmale.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        dialogSchriftAus();
        ImGui::SetCursorScreenPos(bp(0.0F, static_cast<float>(kH)));
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
    }
    ImGui::End();
    if (!offen) { ed.ueberOffen = false; }
}

// --- PathsDialog "Engine Path Configuration" (mainframe.cpp:417-458) ------------------
//
// QGroupBox "Path settings" mit dem Gitter 111 : 333: der Hinweis ueber beide
// Spalten, dann Paths_constructPreferences (Engine Path); QDialogButtonBox OK. Kein
// Foto - Lagen aus den Qt-Massen der Einstellungen.
void zeichneEnginePfadDialog() {
    Editor& ed = editor();
    static bool offen = false;
    static char pfad[512] = "";
    if (!ed.enginePfadDialogOffen) {
        offen = false;
        return;
    }
    if (!offen) {
        offen = true;
        std::snprintf(pfad, sizeof(pfad), "%s", enginePfadAnzeige().c_str());
    }
    const float sk = massstab();
    g_sk = sk;
    // "Select directory, where game executable sits (typically "jaSP.exe")\n"
    const std::string hinweis = std::string("Select directory, where game executable sits (typically \"") + kartenSpiel().engine + "\")";
    const int hinweisB = labelBreite(hinweis.c_str());
    const int b0 = labelBreite(trK(Str::KePrEnginePath));
    const int b1 = std::max(hinweisB - b0 - kAbstand, 200);
    const int gruppeB = kInhaltRand + b0 + kAbstand + b1 + kInhaltRand;
    const int gruppeH = kInhaltOben + 26 + kAbstand + kFeldH + kInhaltRand;   // zwei Textzeilen (mit '\n'), das Feld
    const int innenB = kRand + gruppeB + kRand;
    const int innenH = kRand + gruppeH + kAbstand + kKnopfH + kRand;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowSize(ImVec2{px(static_cast<float>(innenB)), px(static_cast<float>(innenH)) + titelH}, ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_Appearing,
                            ImVec2{0.5F, 0.5F});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool sichtbar = ImGui::Begin((std::string(trK(Str::KeEnginePathTitle)) + "###karteenginepfad").c_str(), nullptr,
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();
    if (sichtbar) {
        g_innen = ImGui::GetCursorScreenPos();
        dialogSchriftAn();
        ImDrawList* fl = ImGui::GetWindowDrawList();
        fl->AddText(bp(static_cast<float>(kRand + kTitelX), static_cast<float>(kRand)), ImGui::GetColorU32(ImGuiCol_Text), trK(Str::KePathSettingsGroup));
        fl->AddRect(bp(static_cast<float>(kRand), static_cast<float>(kRand + kRahmenOben)),
                    bp(static_cast<float>(kRand + gruppeB), static_cast<float>(kRand + gruppeH)), ImGui::GetColorU32(ImGuiCol_Border),
                    ImGui::GetStyle().FrameRounding);
        fl->AddText(bp(static_cast<float>(kRand + kInhaltRand), static_cast<float>(kRand + kInhaltOben)), ImGui::GetColorU32(ImGuiCol_Text), hinweis.c_str());
        Form f;
        f.x0 = kRand + kInhaltRand;
        f.b0 = b0;
        f.x1 = f.x0 + b0 + kAbstand;
        f.b1 = b1;
        f.y = kRand + kInhaltOben + 26 + kAbstand;
        g_form = f;
        pfadEintrag(Str::KePrEnginePath, "##enginepfaddialog", pfad, sizeof(pfad));
        const float knopfH = px(static_cast<float>(kKnopfH));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (knopfH - ImGui::GetTextLineHeight()) * 0.5F)});
        ImGui::SetCursorScreenPos(bp(static_cast<float>(innenB - kRand - kKnopfB), static_cast<float>(innenH - kRand - kKnopfH)));
        if (ImGui::Button((std::string(trK(Str::KePrefOk)) + "##enginepfadok").c_str(), ImVec2{px(static_cast<float>(kKnopfB)), knopfH})) {
            ed.enginePfadDialogOffen = false;
            const std::string neu = karte::ordnerBereinigt(pfad);
            if (neu != enginePfadAnzeige()) {   // setEnginePath
                spielNeuRealisieren("Engine Path Configuration", [neu] {
                    editor().enginePfad = neu;
                    einstellungenSpeichern();
                });
            }
        }
        ImGui::PopStyleVar();
        dialogSchriftAus();
        ImGui::SetCursorScreenPos(bp(0.0F, static_cast<float>(innenH)));
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
    }
    ImGui::End();
}

// --- QFontDialog::getFont (OpenGLFont_dialog, gtkmisc.cpp:339-347) ----------------------
//
// Qt 5.15 qfontdialog.cpp (QFontDialogPrivate::init): "Select Font", 500 x 360;
// Gitter mit den Spalten 38 : 24 : 10 (dazwischen je 6): "&Font", "Font st&yle",
// "&Size" mit Feld und Liste darunter; "Effects" (Stri&keout, &Underline), "Sample"
// ("AaBbYyZz"), "Wr&iting System"; OK / Cancel. Die Familien: QFontDatabase unter
// Windows (EnumFontFamiliesEx), die Groessen: QFontDatabase::standardSizes().
namespace {

struct SchriftWahl {
    bool offen = false;
    std::vector<std::string> familien;
    int familie = 0;
    int stil = 0;
    int groesse = 8;
    char familieText[128] = "";
    char groesseText[16] = "";
    bool durch = false;
    bool unter = false;
    int schriftsystem = 0;
};
SchriftWahl g_schrift;

const int kStandardGroessen[] = {6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72};

// Die Stile einer Familie: die Dateien, die Windows dafuer kennt (Regular, Italic, Bold, Bold Italic)
std::vector<std::pair<std::string, std::pair<bool, bool>>> schriftStile(const std::string& familie) {
    std::vector<std::pair<std::string, std::pair<bool, bool>>> s;
    const std::pair<const char*, std::pair<bool, bool>> alle[4] = {{"Regular", {false, false}}, {"Italic", {false, true}}, {"Bold", {true, false}},
                                                                    {"Bold Italic", {true, true}}};
    for (const auto& [name, art] : alle) {
        if (!platform::schriftDatei(familie, art.first, art.second).empty()) { s.emplace_back(name, art); }
    }
    if (s.empty()) { s.emplace_back("Regular", std::pair<bool, bool>{false, false}); }
    return s;
}

}  // namespace

void zeichneSchriftDialog() {
    Editor& ed = editor();
    SchriftWahl& w = g_schrift;
    if (!ed.schriftDialogOffen) {
        w.offen = false;
        return;
    }
    if (!w.offen) {   // QFontDialog::getFont( &ok, QFont( font, size ) )
        w.offen = true;
        w.familien = platform::schriftFamilien();
        w.familie = 0;
        // QFont( "Myriad Pro" ) ohne die Schrift: Windows ersetzt sie (hier wie die Kartenschrift: Segoe UI)
        const std::string gesucht = ed.glSchriftName;
        for (std::size_t i = 0; i < w.familien.size(); ++i) {
            if (w.familien[i] == gesucht || (w.familie == 0 && w.familien[i] == "Segoe UI")) { w.familie = static_cast<int>(i); }
        }
        w.stil = 0;
        w.groesse = ed.glSchriftGroesse;
        std::snprintf(w.familieText, sizeof(w.familieText), "%s", w.familien.empty() ? "" : w.familien[static_cast<std::size_t>(w.familie)].c_str());
        std::snprintf(w.groesseText, sizeof(w.groesseText), "%d", w.groesse);
    }
    const float sk = massstab();
    g_sk = sk;
    constexpr int kB = 500;
    constexpr int kH = 360;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowSize(ImVec2{px(static_cast<float>(kB)), px(static_cast<float>(kH)) + titelH}, ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_Appearing,
                            ImVec2{0.5F, 0.5F});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    bool offen = true;
    const bool sichtbar = ImGui::Begin((std::string(trK(Str::KeFontTitle)) + "###karteschrift").c_str(), &offen,
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                           ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();
    if (sichtbar) {
        g_innen = ImGui::GetCursorScreenPos();
        dialogSchriftAn();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float breite = ImGui::GetContentRegionAvail().x / sk;
        const float hoehe = ImGui::GetContentRegionAvail().y / sk;
        // Spalten 38 : 24 : 10 im Platz ohne Rand (11) und Zwischenraeume (2 x 6)
        const float platz = breite - 2.0F * kRand - 2.0F * kAbstand;
        const float s0 = platz * 38.0F / 72.0F;
        const float s1 = platz * 24.0F / 72.0F;
        const float s2 = platz - s0 - s1;
        const float x0 = static_cast<float>(kRand);
        const float x1 = x0 + s0 + kAbstand;
        const float x2 = x1 + s1 + kAbstand;
        // Zeilen: Beschriftung 13, Feld 22, Liste (Rest), 11, Gruppen 66, 13, 2, Feld 22, 11, Knoepfe 22
        const float listeH = std::max(40.0F, hoehe - 2.0F * kRand - 13.0F - 22.0F - 11.0F - 66.0F - 13.0F - 2.0F - 22.0F - 11.0F - 22.0F);
        const float yBeschr = static_cast<float>(kRand);
        const float yFeld = yBeschr + 13.0F;
        const float yListe = yFeld + 22.0F;
        const float yGruppe = yListe + listeH + 11.0F;
        dl->AddText(bp(x0, yBeschr), ImGui::GetColorU32(ImGuiCol_Text), trK(Str::KeFontFamily));
        dl->AddText(bp(x1, yBeschr), ImGui::GetColorU32(ImGuiCol_Text), trK(Str::KeFontStyle));
        dl->AddText(bp(x2, yBeschr), ImGui::GetColorU32(ImGuiCol_Text), trK(Str::KeFontSize));
        const auto stile = schriftStile(w.familien.empty() ? std::string() : w.familien[static_cast<std::size_t>(w.familie)]);
        w.stil = std::clamp(w.stil, 0, static_cast<int>(stile.size()) - 1);
        // Felder ueber den Listen
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (px(22.0F) - ImGui::GetTextLineHeight()) * 0.5F)});
        ImGui::SetCursorScreenPos(bp(x0, yFeld));
        ImGui::SetNextItemWidth(px(s0));
        if (ImGui::InputText("##schriftfamilie", w.familieText, sizeof(w.familieText))) {   // familyEdit: passende Familie waehlen
            for (std::size_t i = 0; i < w.familien.size(); ++i) {
                if (_strnicmp(w.familien[i].c_str(), w.familieText, std::strlen(w.familieText)) == 0) {
                    w.familie = static_cast<int>(i);
                    break;
                }
            }
        }
        ImGui::SetCursorScreenPos(bp(x1, yFeld));
        ImGui::SetNextItemWidth(px(s1));
        ImGui::BeginDisabled();
        char stilText[32];
        std::snprintf(stilText, sizeof(stilText), "%s", stile[static_cast<std::size_t>(w.stil)].first.c_str());
        ImGui::InputText("##schriftstilfeld", stilText, sizeof(stilText), ImGuiInputTextFlags_ReadOnly);   // styleEdit: setReadOnly( true )
        ImGui::EndDisabled();
        ImGui::SetCursorScreenPos(bp(x2, yFeld));
        ImGui::SetNextItemWidth(px(s2));
        if (ImGui::InputText("##schriftgroesse", w.groesseText, sizeof(w.groesseText), ImGuiInputTextFlags_CharsDecimal)) {
            const int g = std::atoi(w.groesseText);
            if (g > 0 && g <= 512) { w.groesse = g; }   // QIntValidator( 1, 512 )
        }
        ImGui::PopStyleVar();
        // Listen
        const auto liste = [&](const char* id, float x, float b, const std::vector<std::string>& eintraege, int& wahl) {
            ImGui::SetCursorScreenPos(bp(x, yListe));
            ImGui::BeginChild(id, ImVec2{px(b), px(listeH)}, ImGuiChildFlags_Borders);
            bool geaendert = false;
            for (std::size_t i = 0; i < eintraege.size(); ++i) {
                const bool gewaehlt = static_cast<int>(i) == wahl;
                if (ImGui::Selectable((eintraege[i] + id + std::to_string(i)).c_str(), gewaehlt)) {
                    wahl = static_cast<int>(i);
                    geaendert = true;
                }
                if (gewaehlt && ImGui::IsWindowAppearing()) { ImGui::SetScrollHereY(); }
            }
            ImGui::EndChild();
            return geaendert;
        };
        if (liste("##schriftfamilien", x0, s0, w.familien, w.familie)) {
            std::snprintf(w.familieText, sizeof(w.familieText), "%s", w.familien[static_cast<std::size_t>(w.familie)].c_str());
        }
        std::vector<std::string> stilNamen;
        for (const auto& st : stile) { stilNamen.push_back(st.first); }
        (void)liste("##schriftstile", x1, s1, stilNamen, w.stil);
        std::vector<std::string> groessen;
        int groesseWahl = -1;
        for (std::size_t i = 0; i < std::size(kStandardGroessen); ++i) {
            groessen.push_back(std::to_string(kStandardGroessen[i]));
            if (kStandardGroessen[i] == w.groesse) { groesseWahl = static_cast<int>(i); }
        }
        if (liste("##schriftgroessen", x2, s2, groessen, groesseWahl)) {
            w.groesse = kStandardGroessen[static_cast<std::size_t>(groesseWahl)];
            std::snprintf(w.groesseText, sizeof(w.groesseText), "%d", w.groesse);
        }
        // "Effects" und "Sample"
        const auto gruppe = [&](Str titel, float x, float y, float b, float h) {
            dl->AddText(bp(x + kTitelX, y), ImGui::GetColorU32(ImGuiCol_Text), trK(titel));
            dl->AddRect(bp(x, y + kRahmenOben), bp(x + b, y + h), ImGui::GetColorU32(ImGuiCol_Border), ImGui::GetStyle().FrameRounding);
        };
        gruppe(Str::KeFontEffects, x0, yGruppe, s0, 66.0F);
        Form f;
        f.x0 = static_cast<int>(x0) + kInhaltRand;
        f.x1 = f.x0;
        f.b0 = 0;
        f.b1 = static_cast<int>(s0) - 2 * kInhaltRand;
        f.y = static_cast<int>(yGruppe) + kInhaltOben - 4;
        g_form = f;
        (void)haken(Str::Count, (std::string(trK(Str::KeFontStrikeout)) + "##schriftdurch").c_str(), w.durch);
        g_form.y -= kAbstand;
        (void)haken(Str::Count, (std::string(trK(Str::KeFontUnderline)) + "##schriftunter").c_str(), w.unter);
        const float sampleH = 66.0F + 13.0F + 2.0F + 22.0F;
        gruppe(Str::KeFontSample, x1, yGruppe, s1 + kAbstand + s2, sampleH);
        {   // QLineEdit "AaBbYyZz", mittig, in der gewaehlten Schrift (hier: die Schriftgroesse als Mass)
            const char* probe = "AaBbYyZz";
            const ImVec2 ta = ImGui::CalcTextSize(probe);
            const ImVec2 mitte = bp(x1 + (s1 + kAbstand + s2) * 0.5F, yGruppe + kRahmenOben + (sampleH - kRahmenOben) * 0.5F);
            dl->AddText(nullptr, ImGui::GetFontSize() * static_cast<float>(w.groesse) / 8.0F,
                        ImVec2{mitte.x - ta.x * static_cast<float>(w.groesse) / 16.0F, mitte.y - ta.y * static_cast<float>(w.groesse) / 16.0F},
                        ImGui::GetColorU32(ImGuiCol_Text), probe);
        }
        const float ySystem = yGruppe + 66.0F;
        dl->AddText(bp(x0, ySystem), ImGui::GetColorU32(ImGuiCol_Text), trK(Str::KeFontWriting));
        ImGui::SetCursorScreenPos(bp(x0, ySystem + 13.0F + 2.0F));
        ImGui::SetNextItemWidth(px(s0));
        static const char* const systeme[] = {"Any"};
        (void)klappe("##schriftsystem", &w.schriftsystem, systeme, 1);
        // QDialogButtonBox OK / Cancel unten rechts
        const float knopfH = px(static_cast<float>(kKnopfH));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (knopfH - ImGui::GetTextLineHeight()) * 0.5F)});
        const float yKnopf = hoehe - kRand - kKnopfH;
        ImGui::SetCursorScreenPos(bp(breite - kRand - 2 * kKnopfB - kAbstand, yKnopf));
        if (ImGui::Button((std::string(trK(Str::KePrefOk)) + "##schriftok").c_str(), ImVec2{px(static_cast<float>(kKnopfB)), knopfH})) {
            ed.schriftDialogOffen = false;
            if (!w.familien.empty()) { glSchriftSetzen(w.familien[static_cast<std::size_t>(w.familie)], w.groesse); }   // OpenGLFont_select
        }
        ImGui::SetCursorScreenPos(bp(breite - kRand - kKnopfB, yKnopf));
        if (ImGui::Button((std::string(trK(Str::KeCancel)) + "##schriftabbrechen").c_str(), ImVec2{px(static_cast<float>(kKnopfB)), knopfH})) {
            ed.schriftDialogOffen = false;
        }
        ImGui::PopStyleVar();
        dialogSchriftAus();
    }
    ImGui::End();
    if (!offen) { ed.schriftDialogOffen = false; }
}

// Fuer den Selbsttest: der Text eines Feldes im offenen Einstellungsdialog
// Selbsttest (Stufe 4 Punkt 0c): jede Seite der Einstellungen zeigen.
int einstellungsSeitenAnzahl() { return static_cast<int>(std::size(kSeiten)); }
void einstellungsSeiteZeigen(int seite) {
    if (seite >= 0 && seite < einstellungsSeitenAnzahl()) { g_entwurf.seite = seite; }
}

std::string einstellungsFeld(const std::string& id) {
    if (id == "bauengine") { return g_entwurf.bauEngine; }
    if (id == "bauargumente") { return g_entwurf.bauArgumente; }
    if (id == "bauenginemp") { return g_entwurf.bauEngineMp; }
    if (id == "bauargumentemp") { return g_entwurf.bauArgumenteMp; }
    if (id == "regionshader") { return g_entwurf.regionShader; }
    if (id == "bauziel") { return g_entwurf.bauZiel; }
    if (id == "fsgame") { return g_entwurf.fsGame; }
    if (id == "fsgameaktiv") { return g_entwurf.fsGameAktiv ? "1" : "0"; }
    if (id == "enginepfad") { return g_entwurf.enginePfad; }
    if (id == "undostufen") { return std::to_string(g_entwurf.undoStufen); }
    if (id == "schriftfamilie") { return g_schrift.familieText; }
    if (id == "schriftgroesse") { return g_schrift.groesseText; }
    if (id == "schriftfamilien") {
        std::string alle;
        for (const std::string& f : g_schrift.familien) { alle += f + "\n"; }
        return alle;
    }
    return {};
}

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
        g_entwurf.patchSchwelle = karte::patchUnterteilungSchwelle();
        g_entwurf.siRaster = ed.siWerte.rasterAnpassen;
        g_entwurf.texQual = ed.texQual;
        g_entwurf.modZelle = ed.modBroE.zelle;
        std::snprintf(g_entwurf.modOrdner, sizeof(g_entwurf.modOrdner), "%s", ed.modBroE.ordner.c_str());
        g_entwurf.autosaveAn = ed.autosave.an;
        g_entwurf.autosaveMinuten = ed.autosave.minuten;
        g_entwurf.optKnapp = ed.optimierer.knappProzent;
        g_entwurf.optNachBau = ed.optimierer.nachBau;
        g_entwurf.optRegelwerk = ed.optimierer.regelwerk;
        g_entwurf.optPortale = ed.optimierer.portalSchwelle;
        g_entwurf.optDreiecke = ed.optimierer.dreieckSchwelle;
        g_entwurf.schnappschuesse = ed.autosave.schnappschuesse;
        g_entwurf.clipperCaulk = ed.bearbeitung.clipperCaulk;
        g_entwurf.clipperUmdrehen = ed.bearbeitung.clipperUmdrehenZuruecksetzen;
        g_entwurf.clipperPunkte = ed.bearbeitung.clipperPunkteZuruecksetzen;
        g_entwurf.clipper2Punkte = ed.bearbeitung.clipper2Punkte2d;
        g_entwurf.clipperDoppelklick = ed.bearbeitung.clipperDoppelklickTeilen;
        g_entwurf.nachKlonen = ed.nachKlonenSchubsen;
        g_entwurf.letzteKarte = ed.letzteKarteLaden;
        g_entwurf.undoStufen = static_cast<int>(ed.karte.undo.stufen());
        g_entwurf.texSkala = karte::standardTexturSkalierung();
        g_entwurf.pinselTyp = ed.pinselTyp;
        g_entwurf.immerCaulk = ed.bearbeitung.immerCaulk;
        g_entwurf.ganzzahlig = karte::ebenenPunkteGanzzahlig();
        g_entwurf.rasterStandard = ed.rasterStandard;
        g_entwurf.rasterMax = ed.rasterMaxPotenz;
        g_entwurf.keineStrichelung = ed.anzeige.keineStrichelung;
        g_entwurf.jagen = ed.anzeige.jagen;
        g_entwurf.zumZeiger2d = ed.anzeige.zumZeiger;
        g_entwurf.xyMsaa = ed.anzeige.msaa;
        g_entwurf.bau = ed.bau;
        {   // getExportWithDefaultCaller: leer -> "default:<Vorgabe>"
            const karte::SpielBeschreibung& sb = kartenSpiel();
            const std::string args = karte::engineArgumente(bauSpielName(), sb.basisSpiel);   // gamename_get (Movie Duels: MD)
            std::snprintf(g_entwurf.bauEngine, sizeof(g_entwurf.bauEngine), "%s", ed.bau.engine.mitVorgabe(sb.engine).c_str());
            std::snprintf(g_entwurf.bauArgumente, sizeof(g_entwurf.bauArgumente), "%s", ed.bau.argumente.mitVorgabe(args).c_str());
            std::snprintf(g_entwurf.bauEngineMp, sizeof(g_entwurf.bauEngineMp), "%s", ed.bau.engineMp.mitVorgabe(sb.mpEngine).c_str());
            std::snprintf(g_entwurf.bauArgumenteMp, sizeof(g_entwurf.bauArgumenteMp), "%s", ed.bau.argumenteMp.mitVorgabe(args).c_str());
        }
        std::snprintf(g_entwurf.regionShader, sizeof(g_entwurf.regionShader), "%s", ed.regionKastenShader.c_str());
        std::snprintf(g_entwurf.bauZiel, sizeof(g_entwurf.bauZiel), "%s", ed.bau.zielOrdner.c_str());
        {   // GameFileExport: m_sGameFile.m_latched
            const std::string datei = !ed.spielDateiNeu.empty() ? ed.spielDateiNeu : std::string(kartenSpiel().datei);
            const auto& alle = karte::spielBeschreibungen();
            g_entwurf.spielWahl = 0;
            for (std::size_t i = 0; i < alle.size(); ++i) {
                if (datei == alle[i].datei) { g_entwurf.spielWahl = static_cast<int>(i); }
            }
        }
        g_entwurf.spielFrage = ed.spielFrage;
        {   // GameExport: gamecombo_for_dir( gamename_get() )
            const karte::SpielAuswahl a = karte::spielAuswahlFuerOrdner(kartenSpiel(), kartenFsGame());
            g_entwurf.modWahl = a.spiel;
            std::snprintf(g_entwurf.fsGame, sizeof(g_entwurf.fsGame), "%s", a.fsGame.c_str());
            g_entwurf.fsGameAktiv = a.bearbeitbar;
        }
        g_entwurf.modusWahl = ed.spielModus == "mp" ? 1 : 0;   // GameModeExport
        g_entwurf.abtrennbar = ed.menueAbreissbar;
        std::snprintf(g_entwurf.shaderEditor, sizeof(g_entwurf.shaderEditor), "%s", ed.shaderEditorBefehl.c_str());
        std::snprintf(g_entwurf.enginePfad, sizeof(g_entwurf.enginePfad), "%s", enginePfadAnzeige().c_str());
        for (std::size_t i = 0; i < 5; ++i) {
            std::snprintf(g_entwurf.zusatzPfade[i], sizeof(g_entwurf.zusatzPfade[i]), "%s", ed.zusatzPfade[i].c_str());
        }
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
        } else if (g_entwurf.seite == kSeiteTexturen) {
            texturenSeite();
        } else if (g_entwurf.seite == kSeiteSurface) {
            surfaceSeite();
        } else if (g_entwurf.seite == kSeitePatches) {
            patchSeite();
        } else if (g_entwurf.seite == kSeiteClipper) {
            clipperSeite();
        } else if (g_entwurf.seite == kSeiteAutosave) {
            autosaveSeite();
        } else if (g_entwurf.seite == kSeiteModBro) {
            modBroSeite();
        } else if (g_entwurf.seite == kSeiteSettings) {
            settingsSeite();
        } else if (g_entwurf.seite == kSeiteGlobalGame) {
            globalGameSeite();
        } else if (g_entwurf.seite == kSeiteGame) {
            gameSeite();
        } else if (g_entwurf.seite == kSeitePaths) {
            pathsSeite();
        } else if (g_entwurf.seite == kSeiteUndo) {
            undoSeite();
        } else if (g_entwurf.seite == kSeiteBrush) {
            brushSeite();
        } else if (g_entwurf.seite == kSeiteGrid) {
            gridSeite();
        } else if (g_entwurf.seite == kSeiteOrtho) {
            orthoSeite();
        } else if (g_entwurf.seite == kSeiteBuild) {
            buildSeite();
        } else if (g_entwurf.seite == kSeiteOptimierer) {
            optimiererSeite();
        } else if (g_entwurf.seite == kSeiteGlobal || g_entwurf.seite == kSeiteDisplay) {
            // Global Preferences, Display Preferences: Gruppen ohne Felder (Fotos
            // pref_Global.png, pref_Display.png)
        } else {
            char z[120];
            std::snprintf(z, sizeof(z), tr(Str::KeStufe), 2);
            fl->AddText(bp(static_cast<float>(g_form.x0), static_cast<float>(g_form.y)), ImGui::GetColorU32(ImGuiCol_TextDisabled), z);
        }
        // QDialogButtonBox: "Clean" (ResetRole) links, "OK" und "Cancel" rechts
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (knopfH - ImGui::GetTextLineHeight()) * 0.5F)});
        ImGui::SetCursorScreenPos(bp(static_cast<float>(kGruppeX), static_cast<float>(kKnopfY)));
        // OnButtonClean (preferences.cpp:407-419): Rueckfrage "Reset Registry", Yes setzt
        // alle Einstellungen des Karten-Editors zurueck. Abweichung: NRC beendet sich
        // danach (QCoreApplication::quit); hier gelten die Vorgaben sofort, das Programm
        // laeuft weiter (der Karten-Editor ist ein Reiter von BehavEd). Stufe 6: der
        // Knopf war ausgegraut.
        if (ImGui::Button(trK(Str::KePrefClean), ImVec2{knopfB, knopfH})) {
            qtMeldung("Reset Registry",
                      "This will clean the map editor preferences.\nThey will be as good as new. Do you wish to continue?",
                      MeldungArt::Warnung, true, [](bool ja) {
                          if (!ja) { return; }
                          editor().einstellungenOffen = false;
                          g_app->settings.karte.clear();
                          g_app->settings.karteTasten.clear();
                          einstellungenLaden();
                          einstellungenSpeichern();
                          ausgabe("Preferences reset to defaults");
                      });
        }
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
            karte::setzePatchUnterteilungSchwelle(g_entwurf.patchSchwelle);
            ed.siWerte.rasterAnpassen = g_entwurf.siRaster;
            {   // TextureMiplevelImport / TextureGammaImport: unrealise + realise; Textures_SetMode
                const Editor::TexturQualitaet alt = ed.texQual;
                ed.texQual = g_entwurf.texQual;
                if (alt.mipStufe != ed.texQual.mipStufe || alt.gamma != ed.texQual.gamma) { texturBilderNeuLaden(); }
                if (alt.modus != ed.texQual.modus || alt.anisotropie != ed.texQual.anisotropie) {
                    kartegpu::abtasterSetzen(ed.texQual.modus, ed.texQual.anisotropie);
                }
            }
            ed.bearbeitung.clipperCaulk = g_entwurf.clipperCaulk;
            ed.bearbeitung.clipperUmdrehenZuruecksetzen = g_entwurf.clipperUmdrehen;
            ed.bearbeitung.clipperPunkteZuruecksetzen = g_entwurf.clipperPunkte;
            ed.bearbeitung.clipper2Punkte2d = g_entwurf.clipper2Punkte;
            ed.bearbeitung.clipperDoppelklickTeilen = g_entwurf.clipperDoppelklick;
            if (ed.modBroE.zelle != g_entwurf.modZelle) {   // CellSizeImport
                ed.modBroE.zelle = g_entwurf.modZelle;
                modellBrowserLagenNeu();
            }
            if (ed.modBroE.ordner != g_entwurf.modOrdner) {   // FoldersToLoadImport
                ed.modBroE.ordner = g_entwurf.modOrdner;
                modellBrowserBaumBauen();
            }
            ed.autosave.an = g_entwurf.autosaveAn;
            ed.autosave.minuten = g_entwurf.autosaveMinuten;
            ed.optimierer.knappProzent = g_entwurf.optKnapp;
            ed.optimierer.nachBau = g_entwurf.optNachBau;
            ed.optimierer.regelwerk = g_entwurf.optRegelwerk;
            ed.optimierer.portalSchwelle = g_entwurf.optPortale;
            ed.optimierer.dreieckSchwelle = g_entwurf.optDreiecke;
            ed.autosave.schnappschuesse = g_entwurf.schnappschuesse;
            ed.nachKlonenSchubsen = g_entwurf.nachKlonen;
            ed.letzteKarteLaden = g_entwurf.letzteKarte;
            ed.karte.undo.setzeStufen(static_cast<std::size_t>(g_entwurf.undoStufen));   // UndoLevelsImport: setLevels
            karte::setzeStandardTexturSkalierung(g_entwurf.texSkala);
            ed.pinselTyp = g_entwurf.pinselTyp;   // gilt ab der naechsten Karte (Map_Free)
            ed.bearbeitung.immerCaulk = g_entwurf.immerCaulk;
            karte::setzeEbenenPunkteGanzzahlig(g_entwurf.ganzzahlig);   // Face_importSnapPlanes
            ed.rasterStandard = g_entwurf.rasterStandard;   // g_grid_default: gilt beim naechsten Start
            if (g_entwurf.rasterMax != ed.rasterMaxPotenz) {   // maxGridCoordPowerImport: Region_defaultMinMax, GridChangeNotify
                ed.rasterMaxPotenz = g_entwurf.rasterMax;
                ed.raster.setzeMaxKoordPotenz(ed.rasterMaxPotenz);
                karte::regionStandard(ed.region, ed.raster.maxKoord());
            }
            ed.anzeige.keineStrichelung = g_entwurf.keineStrichelung;
            ed.anzeige.jagen = g_entwurf.jagen;
            ed.anzeige.zumZeiger = g_entwurf.zumZeiger2d;
            ed.anzeige.msaa = g_entwurf.xyMsaa;
            {
                Editor::BauEinstellungen b = g_entwurf.bau;
                b.engine.importieren(g_entwurf.bauEngine);   // DefaultableString::Import
                b.argumente.importieren(g_entwurf.bauArgumente);
                b.engineMp.importieren(g_entwurf.bauEngineMp);
                b.argumenteMp.importieren(g_entwurf.bauArgumenteMp);
                b.zielOrdner = g_entwurf.bauZiel;
                ed.bau = b;
            }
            ed.regionKastenShader = g_entwurf.regionShader;
            ed.spielFrage = g_entwurf.spielFrage;
            ed.menueAbreissbar = g_entwurf.abtrennbar;   // LatchedImportCaller: hier sofort
            if (!ed.menueAbreissbar) { ed.abrisse.clear(); }
            ed.shaderEditorBefehl = g_entwurf.shaderEditor;
            {   // GameFileImport: LatchedValue - wirkt erst nach dem Neustart
                const std::string neu = karte::spielBeschreibungen()[static_cast<std::size_t>(g_entwurf.spielWahl)].datei;
                if (neu != kartenSpiel().datei || !ed.spielDateiNeu.empty()) {
                    ed.spielDateiNeu = neu == kartenSpiel().datei ? std::string() : neu;
                    if (!ed.spielDateiNeu.empty()) { ed.neustartFragen.emplace_back("Selected Game"); }   // m_sGameFile( "", "Selected Game" ): restartRequired
                }
            }
            {   // GameImport, GameModeImport, setEnginePath (EnginePath_Unrealise / Realise)
                const karte::SpielBeschreibung& sb = kartenSpiel();
                const std::string fs = g_entwurf.fsGame[0] == '\0' ? std::string(sb.basisSpiel) : std::string(g_entwurf.fsGame);
                const std::string modus = g_entwurf.modusWahl != 0 ? "mp" : "sp";
                const std::string engine = karte::ordnerBereinigt(g_entwurf.enginePfad);
                std::array<std::string, 5> zusatz;
                for (std::size_t i = 0; i < 5; ++i) { zusatz[i] = karte::ordnerBereinigt(g_entwurf.zusatzPfade[i]); }
                const auto gleich = [](const std::string& a, const std::string& b) {   // path_equal
                    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
                               return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
                           });
                };
                const bool fsNeu = !gleich(fs, kartenFsGame());
                const bool modusNeu = modus != ed.spielModus;
                const bool engineNeu = !gleich(engine, enginePfadAnzeige());
                bool zusatzNeu = false;
                for (std::size_t i = 0; i < 5; ++i) { zusatzNeu = zusatzNeu || !gleich(zusatz[i], ed.zusatzPfade[i]); }
                if (fsNeu || modusNeu || engineNeu || zusatzNeu) {
                    spielNeuRealisieren("Edit Project Settings", [=] {
                        Editor& e2 = editor();
                        if (fsNeu) { e2.spielName = fs; }               // gamename_set
                        if (modusNeu) { e2.spielModus = modus; }        // gamemode_set
                        if (engineNeu) { e2.enginePfad = engine; }      // setEnginePath
                        if (zusatzNeu) { e2.zusatzPfade = zusatz; }
                        einstellungenSpeichern();
                    });
                }
            }
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
            const int b2 = platform::qtTextBreite(tx, L"Segoe UI", 12.0F, false);
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
