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
#include <cstdio>
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
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowSize(ImVec2{vp->WorkSize.x * 0.35F, vp->WorkSize.y * 0.7F}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F},
                            ImGuiCond_FirstUseEver, ImVec2{0.5F, 0.5F});
    const std::string titel = std::string(trK(Str::KeMappedCommands)) + "###kartetasten";
    bool offen = true;
    if (ImGui::Begin(titel.c_str(), &offen, ImGuiWindowFlags_NoCollapse)) {
        const float knopfB = ImGui::GetFontSize() * 7.0F;
        const float listeB = ImGui::GetContentRegionAvail().x - knopfB - ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetNextItemWidth(listeB * 0.5F - ImGui::GetStyle().ItemSpacing.x * 0.5F);
        ImGui::InputTextWithHint("##suchname", trK(Str::KeByName), g_td.suchName, sizeof(g_td.suchName));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(listeB * 0.5F - ImGui::GetStyle().ItemSpacing.x * 0.5F);
        ImGui::InputTextWithHint("##suchtaste", trK(Str::KeByKeys), g_td.suchTaste, sizeof(g_td.suchTaste));
        const auto enthaelt = [](const std::string& text, const char* such) {
            if (such[0] == 0) { return true; }
            std::string a = text;
            std::string b = such;
            std::transform(a.begin(), a.end(), a.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return a.find(b) != std::string::npos;
        };
        ImGui::BeginChild("##tastenliste", ImVec2{listeB, 0.0F});
        if (ImGui::BeginTable("##tasten", 2,
                              ImGuiTableFlags_Sortable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                                  ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable,
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
                std::stable_sort(zeilen.begin(), zeilen.end(), [spalte, ab](const auto& x, const auto& y) {
                    const std::string& a = spalte == 0 ? x.first : x.second;
                    const std::string& b = spalte == 0 ? y.first : y.second;
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
