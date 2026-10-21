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
#include "tastatur.h"

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

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
constexpr int kSeiteKamera = 17;

struct Entwurf {
    bool offen = false;
    int seite = 0;   // PreferencesDialog: beim ersten Oeffnen die erste Seite (Global)
    int anordnung = 0;
    bool eingebaut = false;
    bool abtrennbar = true;
    bool nrc = false;
    karte::KameraEinstellungen kamera;
};
Entwurf g_entwurf;

// Eine Seite ist eine QGroupBox mit einem QFormLayout (PreferencesPage,
// radiant/dialog.cpp: addSpinner/addCheckBox/addCombo/addRadioIcons/
// addPathEntry): Beschriftung rechtsbuendig links, Feld ueber die ganze
// rechte Spalte; Checkboxen haben links eine leere Beschriftung. Vorlage:
// die Fotos SP\nrc_fotos\dialoge\pref_<Seite>.png.
bool formBeginnen(const char* id) {
    return ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingFixedFit);
}
void formSpalten() {
    ImGui::TableSetupColumn("##beschriftung", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("##feld", ImGuiTableColumnFlags_WidthStretch);
}
// Neue Zeile; die Beschriftung rechtsbuendig (Str::Count: keine).
void formZeile(Str text) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    if (text != Str::Count) {
        const char* t = trK(text);
        const float b = ImGui::CalcTextSize(t).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0F, ImGui::GetColumnWidth() - b));
        ImGui::TextUnformatted(t);
    }
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-FLT_MIN);
}

void layoutSeite() {
    // Layout_constructPreferences (mainframe.cpp): appendRadioIcons "Window
    // Layout" (vier Bilder, darunter Radioknoepfe ohne Text), appendCheckBox
    // "Detachable Menus", "Built-In Group Dialog" (Foto pref_Layout.png).
    if (!formBeginnen("##layoutseite")) { return; }
    formSpalten();
    static const char* const kBilder[4] = {"window1", "window2", "window3", "window4"};
    static const Str kNamen[4] = {Str::KeLayoutRegular, Str::KeLayoutFloating, Str::KeLayoutSplit, Str::KeLayoutRegularLeft};
    const float s = std::round(48.0F * g_app->dpiScale * g_app->uiScale);
    // QFormLayout: die Beschriftung mittig zur Zeile (Bilder und Knoepfe)
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    {
        const char* t = trK(Str::KePrWindowLayout);
        const float zeileH = s + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::max(0.0F, (zeileH - ImGui::GetTextLineHeight()) * 0.5F));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0F, ImGui::GetColumnWidth() - ImGui::CalcTextSize(t).x));
        ImGui::TextUnformatted(t);
    }
    ImGui::TableSetColumnIndex(1);
    for (int k = 0; k < 4; ++k) {
        ImGui::BeginGroup();
        (void)symbol(kBilder[k], s, true);
        const float r = ImGui::GetFrameHeight();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (s - r) * 0.5F);
        ImGui::RadioButton((std::string("##layout") + std::to_string(k)).c_str(), &g_entwurf.anordnung, k);
        ImGui::EndGroup();
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", trK(kNamen[k])); }
        if (k < 3) { ImGui::SameLine(); }
    }
    if (g_entwurf.anordnung == 1) {
        formZeile(Str::Count);
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", tr(Str::KeFloatingHinweis));
        ImGui::PopTextWrapPos();
    }
    // Detachable Menus: ImGui-Menues lassen sich nicht abreissen (Qt: tear-off).
    formZeile(Str::Count);
    ImGui::BeginDisabled();
    ImGui::Checkbox(trK(Str::KePrDetachableMenus), &g_entwurf.abtrennbar);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        char z[120];
        std::snprintf(z, sizeof(z), tr(Str::KeStufe), 2);
        ImGui::SetTooltip("%s", z);
    }
    formZeile(Str::Count);
    ImGui::Checkbox(trK(Str::KePrBuiltInGroupDialog), &g_entwurf.eingebaut);
    ImGui::EndTable();
}

// Camera_constructPreferences (camwindow.cpp:2398-2460): appendSpinner,
// appendCheckBox, appendCombo in dieser Reihenfolge (Foto pref_Camera.png).
void kameraSeite() {
    karte::KameraEinstellungen& c = g_entwurf.kamera;
    if (!formBeginnen("##kameraseite")) { return; }
    formSpalten();
    const auto ganz = [&](Str text, int& wert, int von, int bis) {
        formZeile(text);
        ImGui::InputInt((std::string("##") + trK(text)).c_str(), &wert);
        wert = std::clamp(wert, von, bis);
    };
    const auto komma = [&](Str text, float& wert, float von, float bis, const char* format) {
        formZeile(text);
        ImGui::InputFloat((std::string("##") + trK(text)).c_str(), &wert, 0.1F, 1.0F, format);
        wert = std::clamp(wert, von, bis);
    };
    const auto haken = [&](Str text, bool& wert) {
        formZeile(Str::Count);
        ImGui::Checkbox(trK(text), &wert);
    };
    const auto auswahl = [&](Str text, int& wert, const char* const* eintraege, int anzahl) {
        formZeile(text);
        return ImGui::Combo((std::string("##") + trK(text)).c_str(), &wert, eintraege, anzahl);
    };
    ganz(Str::KePrCamMoveSpeed, c.tempo, 1, karte::kTempoMax);
    ganz(Str::KePrCamTimeMax, c.zeitBisMax, 0, 5000);
    ganz(Str::KePrCamScrollSpeed, c.radTempo, 0, 999);
    komma(Str::KePrCamStrafeSpeed, c.strafeTempo, 0.1F, 10.0F, "%.2f");
    komma(Str::KePrCamSensitivity, c.winkelTempo, 0.1F, 30.0F, "%.2f");
    haken(Str::KePrCamInvert, c.mausUmkehren);
    haken(Str::KePrCamZoomPointer, c.zumZeiger);
    haken(Str::KePrCamDiscrete, c.diskret);
    haken(Str::KePrCamFarClip, c.kubisch);
    haken(Str::KePrCamColorize, c.faerben);
    haken(Str::KePrCamFacesWire, c.flaechenDraht);
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
    ImGui::EndTable();
}

void interfaceSeite() {
    // Interface Preferences: "Shader Editor Command" (preferences.cpp:64,
    // appendPathEntry: Feld mit "..."-Knopf; Foto pref_Interface.png), dazu
    // die Einstellung dieses Programms "Begriffe wie NetRadiant".
    static char befehl[260] = "";
    if (!formBeginnen("##interfaceseite")) { return; }
    formSpalten();
    formZeile(Str::KePrShaderEditor);
    ImGui::BeginDisabled();
    ImGui::InputText("##shadereditor", befehl, sizeof(befehl));
    ImGui::EndDisabled();
    formZeile(Str::Count);
    ImGui::Checkbox(trK(Str::KeNrcTerms), &g_entwurf.nrc);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::KeNrcTermsHint)); }
    ImGui::EndTable();
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
    }
    if (!ed.einstellungenOffen) {
        g_entwurf.offen = false;
        return;
    }
    // Groesse und Aufteilung wie NRC 20260114 (Foto dlg_Edit_Preferences_0:
    // Innenmass 597 x 492 logische Punkte, Baum links 145 breit, unten
    // "Clean" unter der Seite links, "OK" und "Cancel" rechts, Knoepfe 79).
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float sk = massstab();
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    ImGui::SetNextWindowSize(ImVec2{std::round(597.0F * sk), std::round(492.0F * sk) + titelH}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F},
                            ImGuiCond_FirstUseEver, ImVec2{0.5F, 0.5F});
    const std::string titel = std::string(trK(Str::KePrefsTitle)) + "###karteeinstellungen";
    bool offen = true;
    if (ImGui::Begin(titel.c_str(), &offen, ImGuiWindowFlags_NoCollapse)) {
        const float knopfH = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
        const float knopfB = std::round(79.0F * sk);
        const float baumB = std::round(145.0F * sk);
        ImGui::BeginChild("##baum", ImVec2{baumB, 0.0F}, ImGuiChildFlags_Borders);
        // QTreeWidget, aufgeklappt: Pfeil vor den Oberseiten, Unterseiten
        // eingerueckt.
        const float pfeil = ImGui::GetFontSize();
        for (int i = 0; i < static_cast<int>(std::size(kSeiten)); ++i) {
            const Seite& s = kSeiten[i];
            const ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::Indent(pfeil * (s.tiefe > 0 ? 2.0F : 1.0F));
            if (ImGui::Selectable((std::string(trK(s.baum)) + "##seite" + std::to_string(i)).c_str(), g_entwurf.seite == i)) {
                g_entwurf.seite = i;
            }
            ImGui::Unindent(pfeil * (s.tiefe > 0 ? 2.0F : 1.0F));
            if (s.tiefe == 0) {
                ImGui::RenderArrow(ImGui::GetWindowDrawList(), ImVec2{p.x + pfeil * 0.1F, p.y}, ImGui::GetColorU32(ImGuiCol_Text),
                                   ImGuiDir_Down, 0.8F);
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginGroup();
        // QGroupBox: Titel ueber dem Rahmen, darin das Formular
        ImGui::BeginChild("##seite", ImVec2{0.0F, -knopfH}, ImGuiChildFlags_None);
        ImGui::TextUnformatted(trK(kSeiten[g_entwurf.seite].titel));
        ImGui::BeginChild("##seitenrahmen", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_Borders);
        if (g_entwurf.seite == kSeiteLayout) {
            layoutSeite();
        } else if (g_entwurf.seite == kSeiteInterface) {
            interfaceSeite();
        } else if (g_entwurf.seite == kSeiteKamera) {
            kameraSeite();
        } else {
            char z[120];
            std::snprintf(z, sizeof(z), tr(Str::KeStufe), 2);
            ImGui::TextDisabled("%s", z);
        }
        ImGui::EndChild();
        ImGui::EndChild();
        // "Clean" links unter der Seite (ausgegraut, Stufe 2), "OK" und
        // "Cancel" rechts.
        ImGui::BeginDisabled();
        (void)ImGui::Button(trK(Str::KePrefClean), ImVec2{knopfB, 0.0F});
        ImGui::EndDisabled();
        ImGui::SameLine();
        const float rechts = ImGui::GetContentRegionMax().x - knopfB * 2.0F - ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), rechts));
        if (ImGui::Button(trK(Str::KePrefOk), ImVec2{knopfB, 0.0F})) {
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
            ed.maximiert = false;
            einstellungenSpeichern();
            offen = false;
        }
        ImGui::SameLine();
        if (ImGui::Button(trK(Str::KeCancel), ImVec2{knopfB, 0.0F})) { offen = false; }
        ImGui::EndGroup();
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
    // Groesse wie NRC 20260114 (Foto dlg_Edit_Shortcuts_0: Innenmass 399 x
    // 533 logische Punkte - QDialog ohne feste Groesse, sizeHint des
    // Rasters), mittig.
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float s = massstab();
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    ImGui::SetNextWindowSize(ImVec2{std::round(399.0F * s), std::round(533.0F * s) + titelH}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F},
                            ImGuiCond_FirstUseEver, ImVec2{0.5F, 0.5F});
    const std::string titel = std::string(trK(Str::KeMappedCommands)) + "###kartetasten";
    bool offen = true;
    if (ImGui::Begin(titel.c_str(), &offen, ImGuiWindowFlags_NoCollapse)) {
        // QDialogButtonBox senkrecht rechts: Knoepfe 79 breit (Foto)
        const float knopfB = std::round(79.0F * s);
        const float listeB = ImGui::GetContentRegionAvail().x - knopfB - ImGui::GetStyle().ItemSpacing.x;
        // Suchfelder mit Lupe vorn (NRC 20260114: Emoji "🔍" im Platzhalter;
        // hier die Lupe des Programms).
        const auto suchfeld = [&](const char* id, const char* hinweis, char* puffer, std::size_t groesse, float breite) {
            const float lupe = ImGui::GetFontSize();
            const ImGuiStyle& st = ImGui::GetStyle();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{st.FramePadding.x * 2.0F + lupe, st.FramePadding.y});
            ImGui::SetNextItemWidth(breite);
            const ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::InputTextWithHint(id, hinweis, puffer, groesse);
            ImGui::PopStyleVar();
            const float h = ImGui::GetItemRectSize().y;
            uiIcon(ImGui::GetWindowDrawList(), UiIcon::Suche, ImVec2{std::floor(p.x + st.FramePadding.x), std::floor(p.y + (h - lupe) * 0.5F)},
                   lupe, ImGui::GetColorU32(uiIconFarbe(UiIcon::Suche)));
        };
        suchfeld("##suchname", trK(Str::KeByName), g_td.suchName, sizeof(g_td.suchName),
                 listeB * 0.5F - ImGui::GetStyle().ItemSpacing.x * 0.5F);
        ImGui::SameLine();
        suchfeld("##suchtaste", trK(Str::KeByKeys), g_td.suchTaste, sizeof(g_td.suchTaste),
                 listeB * 0.5F - ImGui::GetStyle().ItemSpacing.x * 0.5F);
        const auto enthaelt = [](const std::string& text, const char* such) {
            if (such[0] == 0) { return true; }
            std::string a = text;
            std::string b = such;
            std::transform(a.begin(), a.end(), a.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return a.find(b) != std::string::npos;
        };
        ImGui::BeginChild("##tastenliste", ImVec2{listeB, 0.0F});
        // header()->setSectionResizeMode( ResizeToContents ), letzte Spalte
        // nicht gestreckt (commands.cpp:345-346): beide Spalten so breit wie
        // ihr Inhalt.
        if (ImGui::BeginTable("##tasten", 2,
                              ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                                  ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit,
                              ImVec2{0.0F, 0.0F})) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn(trK(Str::KeColCommand), ImGuiTableColumnFlags_DefaultSort);
            ImGui::TableSetupColumn(trK(Str::KeColKey));
            ImGui::TableHeadersRow();
            std::vector<std::pair<std::string, std::string>> zeilen;
            for (const std::string& n : ed.belegung.namen()) {
                const std::string t = ed.tastenDialogBearbeitet == n ? std::string(trK(Str::KePressKey))
                                                                      : befehle::tasteAlsText(ed.belegung.taste(n));
                if (!enthaelt(n, g_td.suchName) || !enthaelt(t, g_td.suchTaste)) { continue; }
                zeilen.emplace_back(n, t);
            }
            if (ImGuiTableSortSpecs* sp = ImGui::TableGetSortSpecs(); sp != nullptr && sp->SpecsCount > 0) {
                const int spalte = sp->Specs[0].ColumnIndex;
                const bool ab = sp->Specs[0].SortDirection == ImGuiSortDirection_Descending;
                // Reihenfolge wie im Foto: Gross-/Kleinschreibung zaehlt nicht
                // ("bobToolz::About" zwischen "AutoCaulkSelected" und "Brush3Sided").
                const auto klein = [](const std::string& t) {
                    std::string k = t;
                    std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    return k;
                };
                std::stable_sort(zeilen.begin(), zeilen.end(), [spalte, ab, &klein](const auto& x, const auto& y) {
                    const std::string a = klein(spalte == 0 ? x.first : x.second);
                    const std::string b = klein(spalte == 0 ? y.first : y.second);
                    return ab ? b < a : a < b;
                });
            }
            for (const auto& [n, t] : zeilen) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                const bool sel = g_td.gewaehlt == n;
                if (ImGui::Selectable((n + "##tz").c_str(), sel, ImGuiSelectableFlags_SpanAllColumns |
                                                                    ImGuiSelectableFlags_AllowDoubleClick)) {
                    g_td.gewaehlt = n;
                    // itemActivated (Doppelklick/Enter) -> accelerator_edit
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { ed.tastenDialogBearbeitet = n; }
                }
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(t.c_str());
                // accelerator_item_set_icon: geaendert gegenueber der Vorgabe
                if (ed.belegung.taste(n) != ed.belegung.standard(n)) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4{0.85F, 0.2F, 0.2F, 1.0F}, "%s", "(x)");
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s: %s", tr(Str::KeKeyChanged),
                                          befehle::tasteAlsText(ed.belegung.standard(n)).c_str());
                    }
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginGroup();
        const ImVec2 kg{knopfB, 0.0F};
        const bool hatWahl = !g_td.gewaehlt.empty();
        ImGui::BeginDisabled(!hatWahl);
        if (ImGui::Button(trK(Str::KeEdit), kg)) { ed.tastenDialogBearbeitet = g_td.gewaehlt; }
        if (ImGui::Button(trK(Str::KeClear), kg)) {   // accelerator_clear_button_clicked
            ed.belegung.setzen(g_td.gewaehlt, 0);
            einstellungenSpeichern();
        }
        if (ImGui::Button(trK(Str::KeReset), kg)) { tasteAendern(g_td.gewaehlt, 0); }
        ImGui::EndDisabled();
        if (ImGui::Button(trK(Str::KeResetAll), kg)) {
            g_td.frage = true;
            g_td.frageAlle = true;
        }
        ImGui::EndGroup();
        frageZeichnen();
    }
    ImGui::End();
    if (!offen) { ed.tastenDialogOffen = false; }
}

}  // namespace bhed::gui::karteneditor
