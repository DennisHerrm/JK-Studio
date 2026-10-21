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

struct Entwurf {
    bool offen = false;
    int seite = kSeiteLayout;
    int anordnung = 0;
    bool eingebaut = false;
    bool abtrennbar = true;
    bool nrc = false;
};
Entwurf g_entwurf;

void layoutSeite() {
    // Layout_constructPreferences: appendRadioIcons "Window Layout"
    ImGui::TextUnformatted(trK(Str::KePrWindowLayout));
    static const char* const kBilder[4] = {"window1", "window2", "window3", "window4"};
    static const Str kNamen[4] = {Str::KeLayoutRegular, Str::KeLayoutFloating, Str::KeLayoutSplit, Str::KeLayoutRegularLeft};
    const float s = std::round(48.0F * g_app->dpiScale * g_app->uiScale);
    for (int k = 0; k < 4; ++k) {
        ImGui::BeginGroup();
        (void)symbol(kBilder[k], s, true);
        ImGui::RadioButton((std::string(trK(kNamen[k])) + "##layout").c_str(), &g_entwurf.anordnung, k);
        ImGui::EndGroup();
        if (k < 3) { ImGui::SameLine(); }
    }
    if (g_entwurf.anordnung == 1) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", tr(Str::KeFloatingHinweis));
        ImGui::PopTextWrapPos();
    }
    ImGui::Spacing();
    // Detachable Menus: ImGui-Menues lassen sich nicht abreissen (Qt: tear-off).
    ImGui::BeginDisabled();
    ImGui::Checkbox(trK(Str::KePrDetachableMenus), &g_entwurf.abtrennbar);
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        char z[120];
        std::snprintf(z, sizeof(z), tr(Str::KeStufe), 2);
        ImGui::SetTooltip("%s", z);
    }
    ImGui::Checkbox(trK(Str::KePrBuiltInGroupDialog), &g_entwurf.eingebaut);
}

void interfaceSeite() {
    // Interface Preferences: "Shader Editor Command" (preferences.cpp:64),
    // dazu die Einstellung dieses Programms "Begriffe wie NetRadiant".
    static char befehl[260] = "";
    ImGui::BeginDisabled();
    ImGui::InputText(trK(Str::KePrShaderEditor), befehl, sizeof(befehl));
    ImGui::EndDisabled();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Checkbox(trK(Str::KeNrcTerms), &g_entwurf.nrc);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::KeNrcTermsHint)); }
}

}  // namespace

void zeichneEinstellungen() {
    Editor& ed = editor();
    if (ed.einstellungenOffen && !g_entwurf.offen) {
        g_entwurf.offen = true;
        g_entwurf.anordnung = static_cast<int>(ed.anordnung);
        g_entwurf.eingebaut = ed.gruppenDialogEingebaut;
        g_entwurf.nrc = nrcBegriffe();
    }
    if (!ed.einstellungenOffen) {
        g_entwurf.offen = false;
        return;
    }
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowSize(ImVec2{vp->WorkSize.x * 0.5F, vp->WorkSize.y * 0.6F}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F},
                            ImGuiCond_FirstUseEver, ImVec2{0.5F, 0.5F});
    const std::string titel = std::string(trK(Str::KePrefsTitle)) + "###karteeinstellungen";
    bool offen = true;
    if (ImGui::Begin(titel.c_str(), &offen, ImGuiWindowFlags_NoCollapse)) {
        const float knopfH = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
        ImGui::BeginChild("##baum", ImVec2{ImGui::GetFontSize() * 11.0F, -knopfH}, ImGuiChildFlags_Borders);
        for (int i = 0; i < static_cast<int>(std::size(kSeiten)); ++i) {
            const Seite& s = kSeiten[i];
            if (s.tiefe > 0) { ImGui::Indent(); }
            if (ImGui::Selectable((std::string(trK(s.baum)) + "##seite" + std::to_string(i)).c_str(), g_entwurf.seite == i)) {
                g_entwurf.seite = i;
            }
            if (s.tiefe > 0) { ImGui::Unindent(); }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("##seite", ImVec2{0.0F, -knopfH}, ImGuiChildFlags_Borders);
        ImGui::TextUnformatted(trK(kSeiten[g_entwurf.seite].titel));
        ImGui::Separator();
        if (g_entwurf.seite == kSeiteLayout) {
            layoutSeite();
        } else if (g_entwurf.seite == kSeiteInterface) {
            interfaceSeite();
        } else {
            char z[120];
            std::snprintf(z, sizeof(z), tr(Str::KeStufe), 2);
            ImGui::TextDisabled("%s", z);
        }
        ImGui::EndChild();
        if (ImGui::Button(trK(Str::KePrefOk))) {
            ed.anordnung = static_cast<befehle::Anordnung>(g_entwurf.anordnung);
            ed.gruppenDialogEingebaut = g_entwurf.eingebaut;
            setzeNrcBegriffe(g_entwurf.nrc);
            ed.maximiert = false;
            einstellungenSpeichern();
            offen = false;
        }
        ImGui::SameLine();
        if (ImGui::Button(trK(Str::KeCancel))) { offen = false; }
        ImGui::SameLine();
        ImGui::BeginDisabled();
        (void)ImGui::Button(trK(Str::KePrefClean));
        ImGui::EndDisabled();
    }
    ImGui::End();
    if (!offen) { ed.einstellungenOffen = false; }
}

}  // namespace bhed::gui::karteneditor
