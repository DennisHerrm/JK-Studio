// app_karte_tags.cpp - Karten-Editor: die Tags des Texturbrowsers (Reiter
// "Tags", Menue Tags) und ein Meldungsfenster wie qt_MessageBox.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/texwindow.cpp:1267-1290
// (TextureBrowser_constructTagsMenu), :1295-1380 (tagsEnableGui,
// Tags_QListWidget, tagsSetCheckboxesForShader, tagAssignmentChanged,
// Tags_QItemDelegate), :1404-1575 (searchTags, showUntagged, checkTagFile,
// addTag, renameTag, deleteTag, copyTag, pasteTag), :1838-1878 (die Liste:
// sortiert, Mehrfachwahl, Bearbeiten bei SelectedClicked/F2, activated sucht;
// QTabWidget mit documentMode und tabBarAutoHide), :1966-1970
// (tagsToggleGui) und radiant/gtkmisc/messagebox (qt_MessageBox).
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
#include "imgui_internal.h"
#include "platform.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace bhed::gui::karteneditor {

namespace {

Editor& e() { return editor(); }

constexpr int kZeile = 16;   // QListWidget: Zeilenhoehe wie der Baum

std::string tagDateiPfad() {   // LocalRcPath_get() + SHADERTAG_FILE, je Spiel
    const std::string spiel = editor().karteSpiel && editor().karteSpiel->spiel == 2 ? std::string("jk2") : std::string("ja");
    return platform::settingsDirectory() + "\\karte_" + spiel + "_" + karte::kTagDatei;
}

bool dateiDa(const std::string& p) {
    std::error_code ec;
    return std::filesystem::exists(std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(p.data()), p.size())), ec);
}

void speichern() { (void)e().tags.datei.speichern(); }

// IShader::IsDefault: ohne Shader-Definition eine Textur
bool istTextur(const std::string& shader) {
    const Editor& ed = e();
    if (!ed.karteSpiel) { return true; }
    for (const karte::ShaderEintrag* s : ed.karteSpiel->inventar.shader()) {
        if (karte::shaderNamenGleich(s->name, shader)) { return false; }
    }
    return true;
}

// den Eintrag des Shaders anlegen, falls er noch keinen hat
void eintragAnlegen(const std::string& shader) {
    TagZustand& t = e().tags;
    if (!t.datei.hatShader(shader)) {
        t.datei.shaderDazu(shader, karte::TagTexturArt::Eigen, istTextur(shader) ? karte::TagKnotenArt::Textur : karte::TagKnotenArt::Shader);
    }
}

// die gefundenen Shader zeigen (TexturePath_loadTexture, SetHideUnused( false ))
void gefundeneZeigen(const std::string& titel) {
    Editor& ed = e();
    karte::TexturBrowser& tb = ed.texBro;
    if (ed.karteSpiel) {
        for (const std::string& s : ed.tags.gefunden) { (void)ed.aktiveShader.holen(s, &ed.karteSpiel->inventar); }
    }
    tb.unbenutzteSetzen(false);
    tb.tagSuche = true;
    tb.gefundeneShader = ed.tags.gefunden;
    tb.verzeichnis = titel;   // g_TextureBrowser_currentDirectory = tags_searched
    tb.hoeheGeaendert();
}

void suchen() {   // TextureBrowser_searchTags
    TagZustand& t = e().tags;
    if (t.markiert.empty()) { return; }
    std::vector<std::string> tags(t.markiert.begin(), t.markiert.end());
    std::string gesucht = "[TAGS] ";
    for (std::size_t i = 0; i < tags.size(); ++i) { gesucht += (i == 0 ? "" : ", ") + tags[i]; }
    t.gefunden = t.datei.suchen(tags);
    if (!t.gefunden.empty()) { ausgabe("Found " + std::to_string(t.gefunden.size()) + " textures and shaders with " + gesucht); }
    gefundeneZeigen(gesucht);
}

void umbenennenFertig(const std::string& alt, const std::string& neu) {   // Tags_QItemDelegate::setModelData
    TagZustand& t = e().tags;
    if (neu.empty()) {
        qtMeldung(":o", "New tag name is empty :0", MeldungArt::Fehler, false, nullptr);
    } else if (neu != alt) {
        if (t.alle.count(neu) != 0U) {
            qtMeldung(neu, "New tag name is already taken :0", MeldungArt::Fehler, false, nullptr);
        } else {
            t.datei.tagUmbenennen(alt, neu);
            t.alle.erase(alt);
            t.alle.insert(neu);
            if (t.markiert.erase(alt) != 0U) { t.markiert.insert(neu); }
            speichern();
        }
    }
}

}  // namespace

// --- Meldungsfenster (qt_MessageBox) ----------------------------------------------------

void qtMeldung(const std::string& titel, const std::string& text, MeldungArt art, bool jaNein, std::function<void(bool)> danach) {
    Editor& ed = e();
    ed.meldung.offen = true;
    ed.meldung.neu = true;
    ed.meldung.titel = titel;
    ed.meldung.text = text;
    ed.meldung.art = art;
    ed.meldung.jaNein = jaNein;
    ed.meldung.danach = std::move(danach);
}

void zeichneQtMeldung() {
    Editor& ed = e();
    Editor::Meldung& m = ed.meldung;
    if (!m.offen) { return; }
    const std::string id = m.titel + "###kartemeldung";
    if (m.neu) {
        m.neu = false;
        ImGui::OpenPopup(id.c_str());
    }
    dialogSchriftAn();
    bool fertig = false;
    bool ja = false;
    if (ImGui::BeginPopupModal(id.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        static const UiIcon symbole[4] = {UiIcon::Haken, UiIcon::Warnung, UiIcon::Warnung, UiIcon::Warnung};
        const float g = ImGui::GetFontSize() * 2.0F;
        const ImVec2 p = ImGui::GetCursorScreenPos();
        uiIcon(ImGui::GetWindowDrawList(), symbole[static_cast<int>(m.art)], p, g, ImGui::GetColorU32(uiIconFarbe(symbole[static_cast<int>(m.art)])));
        ImGui::SetCursorScreenPos(ImVec2{p.x + g + ImGui::GetStyle().ItemSpacing.x * 2.0F, p.y});
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetFontSize() * 28.0F);
        ImGui::TextUnformatted(m.text.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Dummy(ImVec2{0.0F, std::max(0.0F, p.y + g - ImGui::GetCursorScreenPos().y)});
        ImGui::Spacing();
        const float b = std::round(80.0F * massstab());
        if (m.jaNein) {
            if (ImGui::Button((std::string(trK(Str::KeYes)) + "##meldungja").c_str(), ImVec2{b, 0}) || ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
                fertig = true;
                ja = true;
            }
            ImGui::SameLine();
            if (ImGui::Button((std::string(trK(Str::KeNo)) + "##meldungnein").c_str(), ImVec2{b, 0}) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
                fertig = true;
            }
        } else if (ImGui::Button((std::string(trK(Str::KePrefOk)) + "##meldungok").c_str(), ImVec2{b, 0}) || ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
                   ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            fertig = true;
            ja = true;
        }
        if (fertig) {
            ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    dialogSchriftAus();
    if (fertig) {
        m.offen = false;
        auto danach = std::move(m.danach);
        m.danach = nullptr;
        if (danach) { danach(ja); }
    }
}

// --- Tags -------------------------------------------------------------------------------

std::string tagDatei() { return tagDateiPfad(); }

void tagsNeuLaden() {   // Stufe 4 (ShaderPlug "Create tag file"): die neue Datei gleich lesen
    e().tags.geladen = false;
    tagsLaden();
    tagsShaderGewaehlt();
}

void tagsLaden() {   // TextureBrowser_checkTagFile (texwindow.cpp:1476-1495)
    TagZustand& t = e().tags;
    if (t.geladen) { return; }
    t.geladen = true;
    const std::string datei = tagDateiPfad();
    if (dateiDa(datei) && t.datei.oeffnen(datei)) {
        ausgabe("Loaded tag file " + datei + ".");
    } else {
        t.datei.neu(datei);
        ausgabe("Created empty tag file " + datei + ". Plugins -> ShaderPlug -> Create tag file: to index all textures and shaders, if needed.");
    }
    t.alle = t.datei.alleTags();   // GetAllTags
}

void tagsShaderGewaehlt() {   // TextureBrowser_tagsSetCheckboxesForShader (Reiter 1)
    TagZustand& t = e().tags;
    t.haken.clear();
    for (const std::string& s : t.datei.shaderTags(e().texBro.shader)) { t.haken.insert(s); }
}

bool tagBefehlAn(const std::string& name) { return name == "TagsToggleGui" && e().tags.gui; }

void tagBefehl(const std::string& name) {
    Editor& ed = e();
    TagZustand& t = ed.tags;
    tagsLaden();
    if (name == "TagsToggleGui") {   // TextureBrowser_tagsToggleGui
        t.gui = !t.gui;
        if (!t.gui) { t.reiter = 0; }
        einstellungenSpeichern();
    } else if (name == "TagAdd") {   // TextureBrowser_addTag
        std::string tag = "NewTag";
        int i = 0;
        while (t.alle.count(tag) != 0U) { tag = "NewTag" + std::to_string(++i); }
        t.alle.insert(tag);
        t.markiert = {tag};
        t.bearbeitet = tag;   // scrollToItem, editItem
        t.puffer = tag;
        t.bearbeitenFokus = true;
    } else if (name == "TagRename") {   // TextureBrowser_renameTag
        if (!t.markiert.empty()) {
            t.bearbeitet = *t.markiert.begin();
            t.puffer = t.bearbeitet;
            t.bearbeitenFokus = true;
        }
    } else if (name == "TagDelete") {   // TextureBrowser_deleteTag
        if (!t.markiert.empty()) {
            qtMeldung("Delete Tag", "Are you sure you want to delete the selected tags?", MeldungArt::Frage, true, [](bool ja) {
                TagZustand& tz = editor().tags;
                if (!ja) { return; }
                for (const std::string& tag : tz.markiert) {
                    tz.datei.tagUeberallWeg(tag);
                    tz.alle.erase(tag);
                    tz.haken.erase(tag);
                }
                tz.markiert.clear();
                speichern();
            });
        }
    } else if (name == "TagCopy") {   // TextureBrowser_copyTag
        t.kopiert = t.datei.shaderTags(ed.texBro.shader);
    } else if (name == "TagPaste") {   // TextureBrowser_pasteTag
        const std::string shader = ed.texBro.shader;
        if (!t.datei.hatShader(shader)) {
            eintragAnlegen(shader);
            for (const std::string& tag : t.kopiert) { t.datei.tagDazu(shader, tag); }
        } else {
            for (const std::string& tag : t.kopiert) {
                if (!t.datei.hatTag(shader, tag)) { t.datei.tagDazu(shader, tag); }
            }
        }
        tagsShaderGewaehlt();
        speichern();
    } else if (name == "TagSearch") {
        suchen();
    } else if (name == "TagSearchUntagged") {   // TextureBrowser_showUntagged
        qtMeldung("Show Untagged",
                  "WARNING! This function might need a lot of memory and time.\nIt shows all textures & shaders indexed by ShaderPlug plugin, but having no "
                  "tag.\nAre you sure you want to use it?",
                  MeldungArt::Warnung, true, [](bool ja) {
                      if (!ja) { return; }
                      editor().tags.gefunden = editor().tags.datei.ohneTags();
                      gefundeneZeigen("Untagged");
                  });
    }
}

// Die Reiterleiste (QTabWidget, documentMode, tabBarAutoHide: nur mit Tags)
void tagsReiterZeichnen(ImVec2 a, ImVec2 c, float sc) {
    TagZustand& t = e().tags;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(a, c, ImGui::GetColorU32(ImGuiCol_MenuBarBg));
    const char* namen[2] = {trK(Str::KeTagTabTextures), trK(Str::KeTagTabTags)};
    float x = a.x;
    for (int i = 0; i < 2; ++i) {
        const float b = std::round(ImGui::CalcTextSize(namen[i]).x + 24.0F * sc);
        ImGui::SetCursorScreenPos(ImVec2{x, a.y});
        const bool aktiv = t.reiter == i;
        if (ImGui::InvisibleButton((std::string("##tagreiter") + std::to_string(i)).c_str(), ImVec2{b, c.y - a.y})) {
            if (t.reiter != i) {
                t.reiter = i;
                if (i == 1) { tagsShaderGewaehlt(); }   // currentChanged( 1 )
            }
        }
        const bool ueber = ImGui::IsItemHovered();
        if (aktiv || ueber) {
            dl->AddRectFilled(ImVec2{x, a.y}, ImVec2{x + b, c.y}, ImGui::GetColorU32(aktiv ? ImGuiCol_TabSelected : ImGuiCol_TabHovered));
        }
        if (aktiv) { dl->AddLine(ImVec2{x, c.y - sc}, ImVec2{x + b, c.y - sc}, ImGui::GetColorU32(ImGuiCol_CheckMark), std::max(1.0F, 2.0F * sc)); }
        dl->AddText(ImVec2{x + 12.0F * sc, a.y + (c.y - a.y - ImGui::GetFontSize()) * 0.5F}, ImGui::GetColorU32(ImGuiCol_Text), namen[i]);
        x += b;
    }
}

// Die Liste der Tags (Tags_QListWidget)
void tagsListeZeichnen(ImVec2 a, ImVec2 c, float sc) {
    Editor& ed = e();
    TagZustand& t = ed.tags;
    tagsLaden();
    ImGui::SetCursorScreenPos(a);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{std::round(sc), std::round(sc)});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::BeginChild("##tagliste", ImVec2{c.x - a.x, c.y - a.y}, ImGuiChildFlags_Borders);
    const float zeile = std::round(static_cast<float>(kZeile) * sc);
    const float haken = std::round(13.0F * sc);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImGuiIO& io = ImGui::GetIO();
    const std::vector<std::string> liste(t.alle.begin(), t.alle.end());   // setSortingEnabled( true )
    for (const std::string& tag : liste) {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const bool markiert = t.markiert.count(tag) != 0U;
        if (t.bearbeitet == tag) {   // editItem: das Feld in der Zeile
            ImGui::SetCursorScreenPos(ImVec2{p.x + haken + 6.0F * sc, p.y});
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (t.bearbeitenFokus) {
                ImGui::SetKeyboardFocusHere();
                t.bearbeitenFokus = false;
            }
            char puffer[128];
            std::snprintf(puffer, sizeof(puffer), "%s", t.puffer.c_str());
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{2.0F * sc, 0.0F});
            if (ImGui::InputText("##tagname", puffer, sizeof(puffer))) { t.puffer = puffer; }
            ImGui::PopStyleVar();
            if (ImGui::IsItemDeactivated()) {
                const std::string alt = t.bearbeitet;
                t.bearbeitet.clear();
                if (!ImGui::IsKeyPressed(ImGuiKey_Escape)) { umbenennenFertig(alt, t.puffer); }
            }
            ImGui::SetCursorScreenPos(ImVec2{p.x, p.y + zeile});
            continue;
        }
        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
        const bool klick = ImGui::Selectable(("##tag" + tag).c_str(), markiert, ImGuiSelectableFlags_AllowDoubleClick,
                                             ImVec2{ImGui::GetContentRegionAvail().x, zeile});
        ImGui::PopStyleColor();
        const bool rechts = ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right);
        const float mx = io.MousePos.x - p.x;
        const bool aufHaken = mx >= 2.0F * sc && mx < 2.0F * sc + haken + 2.0F * sc;
        if (klick) {
            if (aufHaken) {   // CheckStateRole: Zuordnung zum gewaehlten Shader
                const std::string shader = ed.texBro.shader;
                if (t.haken.count(tag) != 0U) {
                    t.datei.tagWeg(shader, tag);
                    t.haken.erase(tag);
                } else {
                    eintragAnlegen(shader);
                    t.datei.tagDazu(shader, tag);
                    t.haken.insert(tag);
                }
                speichern();
            } else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {   // activated
                t.markiert = {tag};
                suchen();
            } else if (io.KeyCtrl) {   // ExtendedSelection
                if (markiert) {
                    t.markiert.erase(tag);
                } else {
                    t.markiert.insert(tag);
                }
            } else if (markiert && t.markiert.size() == 1) {   // SelectedClicked: bearbeiten
                t.bearbeitet = tag;
                t.puffer = tag;
                t.bearbeitenFokus = true;
            } else {
                t.markiert = {tag};
            }
        }
        if (rechts) { ImGui::OpenPopup("##tagmenue"); }   // contextMenuEvent: m_tagsMenu->popup
        // Kaestchen und Name
        const ImVec2 k0{p.x + 2.0F * sc, p.y + std::floor((zeile - haken) * 0.5F)};
        dl->AddRectFilled(k0, ImVec2{k0.x + haken, k0.y + haken}, ImGui::GetColorU32(ImGuiCol_FrameBgHovered), 2.0F * sc);
        if (t.haken.count(tag) != 0U) { ImGui::RenderCheckMark(dl, ImVec2{k0.x + 2.0F * sc, k0.y + 2.0F * sc}, ImGui::GetColorU32(ImGuiCol_CheckMark), haken - 4.0F * sc); }
        dl->AddText(ImVec2{p.x + haken + 8.0F * sc, p.y + (zeile - ImGui::GetFontSize()) * 0.5F}, ImGui::GetColorU32(ImGuiCol_Text), tag.c_str());
    }
    // leere Flaeche: Rechtsklick oeffnet ebenfalls das Menue
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { ImGui::OpenPopup("##tagmenue"); }
    // F2 bearbeitet (EditKeyPressed), Return sucht; ShortcutOverride: die Liste nimmt die Tasten
    if (ImGui::IsWindowFocused() && t.bearbeitet.empty()) {
        ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
        if (ImGui::IsKeyPressed(ImGuiKey_F2)) { tagBefehl("TagRename"); }
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) { suchen(); }
    }
    if (qtPopupAnfang("##tagmenue", true)) {   // TextureBrowser_constructTagsMenu
        const auto& m = befehle::texturMenues();   // das zweite Menue der Tabelle: Tags
        for (std::size_t i = 0; i < m.size(); ++i) {
            if (m[i].art == befehle::MenueArt::Menue && std::string(m[i].befehl).empty() && std::string(m[i].text) == "Tags") {
                menueEintraege(m, i + 1);
                break;
            }
        }
        qtPopupEnde();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

}  // namespace bhed::gui::karteneditor
