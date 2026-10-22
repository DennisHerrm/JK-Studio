// app_karte_entitaeten.cpp - Karten-Editor: Entitaeten in der Oberflaeche.
//
// Die Seite "Entities" des Gruppendialogs (Entity Inspector), die Entity
// List, das Entitaetenmenue der 2D-Ansicht und der Kamera, die Dialoge "Set
// Light intensity" und "Choose Color" und die Befehle des Entity-Menues.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/entityinspector.cpp
// (EntityInspector_constructWindow, die Attribut-Klassen, SetComment,
// SpawnFlags_setEntityClass, EntityInspector_updateKeyValues, ...),
// radiant/entitylist.cpp und radiant/treemodel.cpp (Entity List),
// radiant/xywindow.cpp (entitycreate_activated, EntityMenu,
// EntityClassMenuInserter, OnContextMenu, OnEntityCreate, XYWnd_MouseToPoint),
// radiant/camwindow.cpp (context_menu_show), radiant/entity.cpp
// (Entity_createFromSelection, Entity_setColour, Entity_normalizeColor,
// Entity_reloadDefinitions, Entity_Construct, Entity_constructPreferences),
// radiant/gtkdlgs.cpp (DoLightIntensityDlg), radiant/gtkmisc.cpp
// (color_dialog), radiant/mainframe.cpp (FocusAllViews) und radiant/select.cpp
// (Select_ConnectedEntities, ExpandSelectionToEntities). Der Farbdialog folgt
// QColorDialog aus Qt 5.15 (src/widgets/dialogs/qcolordialog.cpp, LGPL),
// den NRCs color_dialog oeffnet.
//
// Masse in logischen Qt-Punkten, gemessen am Foto
// SP\nrc_fotos\dialoge\dlg_View_Entity_Inspector_0.png (Massstab 2) und mit
// Qts Formeln belegt: Grundschrift der Widgets MS Shell Dlg 2 = Tahoma 8 pt
// (Qt 5 systemDefaultFont, fm.height() 13), Reiter unten 25 hoch (13 + 12,
// QTabBar::tabSizeHint), Reiter-Breite Text + 24 (PM_TabBarTabHSpace),
// Inhalt des Gruppendialogs 2 vom Rand (SE_TabWidgetTabContents), Zeilen
// 16, QLineEdit und QPushButton 22, QToolButton mit 16er-Symbol 21,
// Abstaende 6.
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

#include "imgui_internal.h"

#include "bhed/karte/auswahlsystem.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <regex>

namespace bhed::gui::karteneditor {

using karte::EntitaetsKlasse;

namespace {

Editor& e() { return editor(); }

ImFont* g_dialog = nullptr;
ImFont* g_dialogFett = nullptr;
int g_dialogTiefe = 0;

// Masse (logische Punkte)
constexpr int kZeile = 16;          // Zeile eines QTreeWidget (Foto: 32 Bildpunkte)
constexpr int kFeldH = 22;          // QLineEdit::sizeHint (Tahoma 8 pt)
constexpr int kKnopfH = 22;         // QPushButton (Foto: Clear All 44 Bildpunkte)
constexpr int kWerkzeug = 21;       // QToolButton, Symbol 16 (Foto: 42 Bildpunkte)
constexpr int kAbstand = 6;         // PM_LayoutHorizontalSpacing / VerticalSpacing
constexpr int kReiterH = 25;        // QTabBar::tabSizeHint: fm.height() 13 + PM_TabBarTabVSpace 12 (Fusion)
constexpr int kReiterPolster = 24;  // PM_TabBarTabHSpace
constexpr int kRollbalken = 7;      // dark.qss QScrollBar
constexpr int kTextRand = 3;        // Item-Text: PM_FocusFrameHMargin + 1
constexpr int kKindRand = 9;        // PM_DefaultChildMargin (Gitter der Attribute)
constexpr int kEinzug = 20;         // PM_TreeViewIndentation (Fusion)

ImU32 stil(ImGuiCol c, float a = 1.0F) { return ImGui::GetColorU32(c, a); }
float sk() { return massstab(); }
float px(float logisch) { return std::round(logisch * sk()); }

bool gleichOhneFall(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}
std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// QString::localeAwareCompare (Qt 5.15 mit ICU, Wurzel-Kollation) fuer die
// Namen der Klassenliste (QTreeWidget mit setSortingEnabled): primaer
// Satzzeichen vor Ziffern vor Buchstaben ohne Gross/klein, dann klein vor gross.
int kollationsGewicht(unsigned char c) {
    if (std::isalpha(c) != 0) { return 300 + std::tolower(c); }
    if (std::isdigit(c) != 0) { return 200 + c; }
    return 100 + c;
}
bool qtVorher(const std::string& a, const std::string& b) {
    const std::size_t n = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        const int ga = kollationsGewicht(static_cast<unsigned char>(a[i]));
        const int gb = kollationsGewicht(static_cast<unsigned char>(b[i]));
        if (ga != gb) { return ga < gb; }
    }
    if (a.size() != b.size()) { return a.size() < b.size(); }
    for (std::size_t i = 0; i < n; ++i) {   // tertiaer: klein vor gross
        const bool ka = std::islower(static_cast<unsigned char>(a[i])) != 0;
        const bool kb = std::islower(static_cast<unsigned char>(b[i])) != 0;
        if (ka != kb) { return ka; }
    }
    return false;
}

// FocusAllViews (mainframe.cpp:2060-2063): XY_Centralize und
// GlobalCamera_FocusOnSelected - mit den Grenzen der neuen Auswahl.
void alleAnsichtenFokus() {
    Editor& ed = e();
    if (ed.karte.auswahl.anzahl() != 0) {
        const karte::Kasten b = karte::auswahlGrenzenPivot(ed.karte);
        if (karte::kastenGueltig(b)) { ed.workzone = b; }
    }
    xyBefehl("CenterXYView");
    kameraBefehl("CameraFocusOnSelected");
}

// Die Auswahl hat sich geaendert (setSelectedAll ... / Selectable::setSelected).
void auswahlNeu() { auswahlGeaendert(); }

// Ein Werkzeugknopf (QToolButton) an der Stelle a mit Symbol(en).
bool werkzeugKnopf(const char* id, ImVec2 a, float b, float h, const char* bild, bool an, const char* tipp, float bildB = 16.0F) {
    ImGui::SetCursorScreenPos(a);
    const bool klick = ImGui::InvisibleButton(id, ImVec2{b, h});
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 c{a.x + b, a.y + h};
    const bool ueber = ImGui::IsItemHovered();
    // dark.qss QToolButton: hover 128, pressed 120, on = palette(base)
    ImU32 grund = stil(ImGuiCol_Button);
    if (an) { grund = stil(ImGuiCol_FrameBg); }
    if (ueber) { grund = stil(ImGuiCol_ButtonHovered); }
    if (ImGui::IsItemActive()) { grund = stil(ImGuiCol_ButtonActive); }
    dl->AddRectFilled(a, c, grund, ImGui::GetStyle().FrameRounding);
    dl->AddRect(a, c, stil(an ? ImGuiCol_CheckMark : ImGuiCol_Border), ImGui::GetStyle().FrameRounding);
    const float g = px(16.0F);
    const float gb = px(bildB);
    const ImVec2 ia{std::floor(a.x + (b - gb) * 0.5F), std::floor(a.y + (h - g) * 0.5F)};
    if (gb > g) {   // breites Bild (arrow_left_right.png, 32 x 16): im Quadrat gb, senkrecht mittig
        karteIcon(dl, bild, ImVec2{ia.x, ia.y - (gb - g) * 0.5F}, gb, ImGui::GetColorU32(karteIconFarbe(bild)));
    } else {
        karteIcon(dl, bild, ia, g, ImGui::GetColorU32(karteIconFarbe(bild)));
    }
    if (ueber && tipp != nullptr) { ImGui::SetTooltip("%s", tipp); }
    return klick;
}

// Ein Druckknopf (QPushButton) an fester Stelle.
bool druckKnopf(const char* text, const char* id, ImVec2 a, float b, float h) {
    ImGui::SetCursorScreenPos(a);
    return ImGui::Button((std::string(text) + id).c_str(), ImVec2{b, h});
}

// NonModalEntry (gtkutil/nonmodal.h): Enter wendet immer an, Fokus weg nur
// nach einer Aenderung, Escape nimmt zurueck (update). text ist der Inhalt,
// solange das Feld nicht bearbeitet wird; true = anwenden (neu = Text).
bool nichtModal(const char* id, const std::string& text, float breite, std::string& neu, bool zahl = false) {
    EntitaetenOberflaeche& u = e().ent;
    const ImGuiID gid = ImGui::GetID(id);
    const bool aktivVorher = ImGui::GetActiveID() == gid;
    auto it = u.felder.find(id);
    if (!aktivVorher || it == u.felder.end()) {
        u.felder[id] = text;
        it = u.felder.find(id);
    }
    char puffer[1024];
    std::snprintf(puffer, sizeof(puffer), "%s", it->second.c_str());
    ImGui::SetNextItemWidth(breite);
    const ImGuiInputTextFlags fl = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll |
                                   (zahl ? ImGuiInputTextFlags_CharsScientific : 0);
    const bool enter = ImGui::InputText(id, puffer, sizeof(puffer), fl);
    // KeyValueValidator: keine Zeilenumbrueche und Anfuehrungszeichen
    std::string t = puffer;
    const std::size_t vorher = t.size();
    t.erase(std::remove_if(t.begin(), t.end(), [](char c) { return c == '\n' || c == '\r' || c == '"'; }), t.end());
    if (t.size() != vorher) {
        u.hinweis = tr(Str::KeEiNoQuotes);
        u.hinweisBis = ImGui::GetTime() + 3.0;
    }
    it->second = t;
    const bool escape = aktivVorher && ImGui::IsKeyPressed(ImGuiKey_Escape, false);
    if (escape) {
        u.felder.erase(id);
        return false;
    }
    if (enter) {
        neu = t;
        u.felder.erase(id);
        return true;
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        neu = t;
        u.felder.erase(id);
        return true;
    }
    return false;
}

// Rahmen eines QAbstractScrollArea (Fusion: 1 Punkt)
void rahmen(ImVec2 a, ImVec2 c) { ImGui::GetWindowDrawList()->AddRect(a, c, stil(ImGuiCol_Border)); }

// Text mit Umbruch an Wortgrenzen (QPlainTextEdit, WidgetWidth), Teile fett.
struct TextTeil {
    std::string text;
    bool fett = false;
};
std::vector<std::vector<TextTeil>> kommentarZeilen(const std::string& text) {
    // SetComment: "^\s*\w+(?=\s*:)" (mehrzeilig) fett
    std::vector<std::vector<TextTeil>> zeilen;
    std::size_t start = 0;
    static const std::regex muster(R"(^\s*\w+(?=\s*:))");
    while (start <= text.size()) {
        std::size_t ende = text.find('\n', start);
        if (ende == std::string::npos) { ende = text.size(); }
        const std::string zeile = text.substr(start, ende - start);
        std::vector<TextTeil> teile;
        std::smatch m;
        if (std::regex_search(zeile, m, muster) && m.position(0) == 0) {
            teile.push_back({zeile.substr(0, static_cast<std::size_t>(m.length(0))), true});
            teile.push_back({zeile.substr(static_cast<std::size_t>(m.length(0))), false});
        } else {
            teile.push_back({zeile, false});
        }
        zeilen.push_back(std::move(teile));
        if (ende == text.size()) { break; }
        start = ende + 1;
    }
    return zeilen;
}

// --- Aktualisieren (EntityInspector_updateKeyValues) -----------------------------------

std::vector<const EntitaetsKlasse*> klassenListe() {
    std::vector<const EntitaetsKlasse*> v;
    Editor& ed = e();
    if (!ed.karteSpiel) { return v; }
    // EntityClassList_fill (GlobalEntityClassManager().forEach) beim Aufbau:
    // Klassen, die erst die Karte anlegt (findOrInsert, "unknown"), fehlen darin
    for (const EntitaetsKlasse* k : ed.karteSpiel->klassen.alle()) {
        if (!k->unknown) { v.push_back(k); }
    }
    std::stable_sort(v.begin(), v.end(), [](const EntitaetsKlasse* a, const EntitaetsKlasse* b) { return qtVorher(a->name, b->name); });
    return v;
}

const EntitaetsKlasse* klasseNamens(const std::string& name) {
    Editor& ed = e();
    if (!ed.karteSpiel || name.empty()) { return nullptr; }
    return ed.karteSpiel->klassen.finden(name);
}

void aktualisieren() {
    Editor& ed = e();
    EntitaetenOberflaeche& u = ed.ent;
    karte::GewaehlteSchluessel neu = karte::gewaehlteSchluessel(ed.karte);
    std::vector<const void*> stand;
    stand.reserve(ed.karte.auswahl.anzahl() * 2);
    for (const auto& el : ed.karte.auswahl.elemente()) {
        stand.push_back(el.entitaet.lock().get());
        stand.push_back(el.primitiv.lock().get());
    }
    if (u.geladen && neu.werte == u.werte.werte && neu.vorgaben == u.werte.vorgaben && stand == u.auswahlStand) { return; }
    u.geladen = true;
    u.werte = std::move(neu);
    u.auswahlStand = std::move(stand);
    // EntityInspector_setEntityClass( findOrInsert( classname, false ) ):
    // EntityClassList_selectEntityClass - die Zeile mit dem Namen (sonst keine).
    const std::string kn = u.werte.werte.count("classname") != 0U ? u.werte.werte.at("classname") : std::string();
    const EntitaetsKlasse* kl = ed.karte.klasseFuer(kn, false);
    std::string zeile;
    if (kl != nullptr && klasseNamens(kl->name) != nullptr) { zeile = klasseNamens(kl->name)->name; }
    if (zeile != u.klasse) {
        u.klasse = zeile;
        u.kommentarKlasse = zeile;   // currentItemChanged -> SetComment
    }
    u.klasseRollen = true;
    u.zeile.clear();   // g_entprops_store->clear()
    u.felder.clear();  // attr->update()
}

// --- Klassenliste -----------------------------------------------------------------------

void klassenListeZeichnen(ImVec2 a, ImVec2 c) {
    Editor& ed = e();
    EntitaetenOberflaeche& u = ed.ent;
    const std::vector<const EntitaetsKlasse*> liste = klassenListe();
    ImGui::SetCursorScreenPos(a);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, px(kRollbalken));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{px(1.0F), px(1.0F)});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::BeginChild("##eiklassen", ImVec2{c.x - a.x, c.y - a.y}, ImGuiChildFlags_Borders);
    const float zeile = px(kZeile);
    int aktuell = -1;
    for (std::size_t i = 0; i < liste.size(); ++i) {
        if (liste[i]->name == u.klasse) { aktuell = static_cast<int>(i); }
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // ImGuiListClipper: die Liste hat Hunderte Klassen
    ImGuiListClipper clip;
    clip.Begin(static_cast<int>(liste.size()), zeile);
    if (u.klasseRollen && aktuell >= 0) { clip.IncludeItemByIndex(aktuell); }
    while (clip.Step()) {
        for (int i = clip.DisplayStart; i < clip.DisplayEnd; ++i) {
            const EntitaetsKlasse* kl = liste[static_cast<std::size_t>(i)];
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const bool gewaehlt = kl->name == u.klasse;
            ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
            const bool klick = ImGui::Selectable(("##ek" + kl->name).c_str(), gewaehlt, ImGuiSelectableFlags_AllowDoubleClick,
                                                 ImVec2{ImGui::GetContentRegionAvail().x, zeile});
            ImGui::PopStyleColor();
            if (klick) {
                u.klassenFokus = true;
                u.zeilenFokus = false;
                if (u.klasse != kl->name) {   // currentItemChanged -> SetComment
                    u.klasse = kl->name;
                    u.kommentarKlasse = kl->name;
                }
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {   // itemActivated
                    karte::klasseSetzenAuswahl(ed.karte, kl->name);
                    auswahlNeu();
                }
            }
            dl->AddText(ImVec2{p.x + px(kTextRand), p.y + (zeile - ImGui::GetFontSize()) * 0.5F}, stil(ImGuiCol_Text), kl->name.c_str());
            if (u.klasseRollen && i == aktuell) {
                // QAbstractItemView::scrollTo (EnsureVisible)
                const float oben = ImGui::GetScrollY();
                const float y = static_cast<float>(i) * zeile;
                const float sicht = ImGui::GetWindowHeight() - px(2.0F);
                if (y < oben) {
                    ImGui::SetScrollY(y);
                } else if (y + zeile > oben + sicht) {
                    ImGui::SetScrollY(y + zeile - sicht);
                }
            }
        }
    }
    if (u.klasseRollen && ImGui::GetFrameCount() > 1) { u.klasseRollen = false; }
    // Tasten (QTreeWidget): hoch/runter, Return/Enter = itemActivated
    if (u.klassenFokus && !liste.empty()) {
        // g_pressedKeysFilter: Return, Enter, Tab, hoch, runter, Bild hoch/runter gehen nicht an die Kuerzel
        for (const ImGuiKey k : {ImGuiKey_Enter, ImGuiKey_KeypadEnter, ImGuiKey_Tab, ImGuiKey_UpArrow, ImGuiKey_DownArrow,
                                 ImGuiKey_PageUp, ImGuiKey_PageDown}) {
            if (ImGui::IsKeyPressed(k)) { ed.tasteVerbrauchtBild = ImGui::GetFrameCount(); }
        }
        int neu = aktuell;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { neu = std::min(static_cast<int>(liste.size()) - 1, aktuell + 1); }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { neu = std::max(0, aktuell - 1); }
        if (neu != aktuell && neu >= 0) {
            u.klasse = liste[static_cast<std::size_t>(neu)]->name;
            u.kommentarKlasse = u.klasse;
            u.klasseRollen = true;
        }
        if (aktuell >= 0 && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))) {
            karte::klasseSetzenAuswahl(ed.karte, liste[static_cast<std::size_t>(aktuell)]->name);
            auswahlNeu();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
    if (u.klassenFokus && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1) || ImGui::IsMouseClicked(2)) &&
        !ImGui::IsMouseHoveringRect(a, c)) {
        u.klassenFokus = false;
    }
}

// --- Kommentar (QPlainTextEdit, nur lesen) -----------------------------------------------

void kommentarZeichnen(ImVec2 a, ImVec2 c) {
    EntitaetenOberflaeche& u = e().ent;
    const EntitaetsKlasse* kl = klasseNamens(u.kommentarKlasse);
    ImGui::SetCursorScreenPos(a);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, px(kRollbalken));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{px(1.0F + 4.0F), px(1.0F + 4.0F)});   // Rahmen + documentMargin 4
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::BeginChild("##eikommentar", ImVec2{c.x - a.x, c.y - a.y}, ImGuiChildFlags_Borders);
    if (kl != nullptr) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float breite = ImGui::GetContentRegionAvail().x;
        const float zeileH = ImGui::GetTextLineHeight();
        float y = 0.0F;
        const ImVec2 o = ImGui::GetCursorScreenPos();
        for (const auto& teile : kommentarZeilen(kl->kommentar)) {
            float x = 0.0F;
            for (const TextTeil& t : teile) {
                ImFont* f = t.fett && g_dialogFett != nullptr ? g_dialogFett : ImGui::GetFont();
                ImGui::PushFont(f, 0.0F);
                // Wortweise mit Umbruch
                std::size_t i = 0;
                while (i < t.text.size()) {
                    std::size_t j = i;
                    while (j < t.text.size() && t.text[j] != ' ') { ++j; }
                    while (j < t.text.size() && t.text[j] == ' ') { ++j; }
                    const std::string wort = t.text.substr(i, j - i);
                    const float w = ImGui::CalcTextSize(wort.c_str()).x;
                    if (x > 0.0F && x + w > breite) {
                        x = 0.0F;
                        y += zeileH;
                    }
                    dl->AddText(ImVec2{o.x + x, o.y + y}, stil(ImGuiCol_Text), wort.c_str());
                    x += w;
                    i = j;
                }
                ImGui::PopFont();
            }
            y += zeileH;
        }
        ImGui::Dummy(ImVec2{1.0F, y});
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
}

// --- Behaelter: Spawnflags, Schluessel/Werte, Felder, Knoepfe ---------------------------

// SpawnFlags_setEntityClass: die benannten Flags (spawn_table)
std::vector<int> spawnTabelle(const EntitaetsKlasse* kl) {
    std::vector<int> v;
    if (kl == nullptr) { return v; }
    for (std::size_t i = 0; i < karte::kMaxFlags; ++i) {
        if (!kl->flagnamen[i].empty()) { v.push_back(static_cast<int>(i)); }
    }
    return v;
}

void tooltipAttribut(const EntitaetsKlasse* kl, int attribut) {   // EntityAttribute_setTooltip
    if (kl == nullptr || attribut < 0 || attribut >= static_cast<int>(kl->attribute.size())) { return; }
    const karte::KlassenAttribut& a = kl->attribute[static_cast<std::size_t>(attribut)].second;
    if (a.name.empty() && a.beschreibung.empty()) { return; }
    if (ImGui::BeginTooltip()) {
        if (!a.name.empty()) {
            if (g_dialogFett != nullptr) { ImGui::PushFont(g_dialogFett, 0.0F); }
            ImGui::TextUnformatted(("      " + a.name + "    ").c_str());
            if (g_dialogFett != nullptr) { ImGui::PopFont(); }
        }
        if (!a.beschreibung.empty()) {
            ImGui::PushTextWrapPos(px(400.0F));
            ImGui::TextUnformatted(a.beschreibung.c_str());
            ImGui::PopTextWrapPos();
        }
        ImGui::EndTooltip();
    }
}

int checkboxBreite(const char* text) {   // QCheckBox::sizeHint (CT_CheckBox): 14 + 6 + Text
    return static_cast<int>(std::ceil(ImGui::CalcTextSize(text).x / sk())) + 14 + 6;
}
constexpr int kCheckboxH = 17;   // max(fm.height() 13 + 4, PM_IndicatorHeight 14)

const EntitaetsKlasse* aktuelleKlasse() {   // g_current_flags / g_current_attributes
    Editor& ed = e();
    const auto& w = ed.ent.werte.werte;
    const std::string kn = w.count("classname") != 0U ? w.at("classname") : std::string();
    return ed.karte.klasseFuer(kn, false);
}

int spawnflagsHoehe(int breite) {
    (void)breite;
    const std::vector<int> t = spawnTabelle(aktuelleKlasse());
    if (t.empty()) { return 0; }
    const int zeilen = static_cast<int>((t.size() + 3) / 4);
    return zeilen * kCheckboxH + (zeilen - 1) * kAbstand;
}

void spawnflagsZeichnen(ImVec2 a, int breite) {
    (void)breite;
    Editor& ed = e();
    const EntitaetsKlasse* kl = aktuelleKlasse();
    const std::vector<int> t = spawnTabelle(kl);
    if (t.empty()) { return; }
    // EntityInspector_updateSpawnflags
    const auto& w = ed.ent.werte;
    const int flags = std::atoi(w.wert("spawnflags").c_str());
    // Spaltenbreiten (QGridLayout, AlignLeft): je Spalte die breiteste
    std::array<int, 4> spalte{0, 0, 0, 0};
    std::vector<std::string> texte;
    for (const int bit : t) {
        texte.push_back(klein(kl->flagnamen[static_cast<std::size_t>(bit)]));   // LowerCase
    }
    for (std::size_t i = 0; i < t.size(); ++i) { spalte[i % 4] = std::max(spalte[i % 4], checkboxBreite(texte[i].c_str())); }
    for (std::size_t i = 0; i < t.size(); ++i) {
        int x = 0;
        for (std::size_t s = 0; s < i % 4; ++s) { x += spalte[s] + kAbstand; }
        const int y = static_cast<int>(i / 4) * (kCheckboxH + kAbstand);
        ImGui::SetCursorScreenPos(ImVec2{a.x + px(static_cast<float>(x)), a.y + px(static_cast<float>(y))});
        bool an = (flags & (1 << t[i])) != 0;
        if (qtHaken((texte[i] + "##eiflag" + std::to_string(t[i])).c_str(), &an, static_cast<float>(kCheckboxH))) {   // EntityInspector_applySpawnflags
            int f = 0;
            for (std::size_t j = 0; j < t.size(); ++j) {
                const bool v = j == i ? an : (flags & (1 << t[j])) != 0;
                f |= (v ? 1 : 0) << t[j];
            }
            karte::spawnflagsSetzen(ed.karte, f);
        }
        if (ImGui::IsItemHovered()) { tooltipAttribut(kl, kl->flagAttribut[static_cast<std::size_t>(t[i])]); }
    }
}

void schluesselListeZeichnen(ImVec2 a, ImVec2 c) {
    Editor& ed = e();
    EntitaetenOberflaeche& u = ed.ent;
    ImGui::SetCursorScreenPos(a);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, px(kRollbalken));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{px(1.0F), px(1.0F)});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::BeginChild("##eischluessel", ImVec2{c.x - a.x, c.y - a.y}, ImGuiChildFlags_Borders);
    const float zeile = px(kZeile);
    // Spalte 0: ResizeToContents
    float spalte0 = 0.0F;
    for (const auto& [s, w] : u.werte.werte) {
        (void)w;
        spalte0 = std::max(spalte0, ImGui::CalcTextSize(s.c_str()).x + px(2.0F * kTextRand));
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (const auto& [s, w] : u.werte.werte) {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
        // itemPressed: beim Druecken
        (void)ImGui::Selectable(("##ez" + s).c_str(), u.zeile == s, ImGuiSelectableFlags_None,
                                ImVec2{ImGui::GetContentRegionAvail().x, zeile});
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {   // EntityProperties_selection_changed
            u.zeile = s;
            u.schluessel = s;
            u.wert = w;
            u.zeilenFokus = true;
            u.klassenFokus = false;
        }
        const float ty = p.y + (zeile - ImGui::GetFontSize()) * 0.5F;
        dl->AddText(ImVec2{p.x + px(kTextRand), ty}, stil(ImGuiCol_Text), s.c_str());
        dl->AddText(ImVec2{p.x + spalte0 + px(kTextRand), ty}, stil(ImGuiCol_Text), w.c_str());
    }
    // g_EntityProperties_keypress: Entf loescht den Schluessel der Zeile
    if (u.zeilenFokus) {
        for (const ImGuiKey k : {ImGuiKey_Delete, ImGuiKey_Enter, ImGuiKey_KeypadEnter, ImGuiKey_Tab, ImGuiKey_UpArrow, ImGuiKey_DownArrow,
                                 ImGuiKey_PageUp, ImGuiKey_PageDown}) {
            if (ImGui::IsKeyPressed(k)) { ed.tasteVerbrauchtBild = ImGui::GetFrameCount(); }   // ShortcutOverride angenommen
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) && !u.zeile.empty()) { karte::schluesselLoeschenAuswahl(ed.karte, u.zeile); }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
    if (u.zeilenFokus && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1) || ImGui::IsMouseClicked(2)) &&
        !ImGui::IsMouseHoveringRect(a, c)) {
        u.zeilenFokus = false;
    }
}

// EntityInspector_applyKeyValue
void schluesselAnwenden() {
    Editor& ed = e();
    const std::string& s = ed.ent.schluessel;
    const std::string& w = ed.ent.wert;
    if (s.empty()) { return; }   // avoid empty key name; empty value is okay: deletes key
    if (s == "classname") {
        karte::klasseSetzenAuswahl(ed.karte, w);
        auswahlNeu();
    } else {
        karte::schluesselSetzenRueckgaengig(ed.karte, s, w);
    }
}

// Ein Eingabefeld (LineEdit) mit Validator; text wird direkt bearbeitet.
bool zeilenFeld(const char* id, std::string& text, float breite, bool name) {
    EntitaetenOberflaeche& u = e().ent;
    char puffer[1024];
    std::snprintf(puffer, sizeof(puffer), "%s", text.c_str());
    ImGui::SetNextItemWidth(breite);
    const bool enter = ImGui::InputText(id, puffer, sizeof(puffer), ImGuiInputTextFlags_EnterReturnsTrue);
    std::string t = puffer;
    const std::size_t vorher = t.size();
    // KeyNameValidator: " \n\r\t\v\"" / KeyValueValidator: "\n\r\""
    const char* verboten = name ? " \n\r\t\v\"" : "\n\r\"";
    t.erase(std::remove_if(t.begin(), t.end(), [verboten](char c) { return std::strchr(verboten, c) != nullptr && c != 0; }), t.end());
    if (t.size() != vorher) {
        u.hinweis = tr(name ? Str::KeEiNoSpaces : Str::KeEiNoQuotes);
        u.hinweisBis = ImGui::GetTime() + 3.0;
    }
    text = t;
    return enter;
}

void behaelterZeichnen(ImVec2 o, int x, int y, int b, int h) {
    Editor& ed = e();
    EntitaetenOberflaeche& u = ed.ent;
    const auto bp = [&](int lx, int ly) { return ImVec2{o.x + px(static_cast<float>(lx)), o.y + px(static_cast<float>(ly))}; };
    // QVBoxLayout (Raender 0, Abstand 6): Spawnflags, Liste (dehnt sich),
    // Gitter Key/Value (2 x 22 + 6), Knopfzeile (22)
    const int flagsH = spawnflagsHoehe(b);
    const int gitterH = kFeldH * 2 + kAbstand;
    const int unten = gitterH + kAbstand + kKnopfH;
    const int listeY = y + (flagsH > 0 ? flagsH + kAbstand : 0);
    const int listeH = std::max(0, h - (listeY - y) - kAbstand - unten);
    merkeFeld("Entities:Spawnflags", x, y, b, flagsH);
    if (flagsH > 0) { spawnflagsZeichnen(bp(x, y), b); }
    merkeFeld("Entities:Schluessel", x, listeY, b, listeH);
    if (listeH > 0) { schluesselListeZeichnen(bp(x, listeY), bp(x + b, listeY + listeH)); }
    // Gitter: Raender 4, 0, 4, 0; Spalten Beschriftung | Feld | Knopf | grosser Knopf
    const int gy = listeY + listeH + kAbstand;
    const int gx = x + 4;
    const int gb = b - 8;
    const int textB = static_cast<int>(std::ceil(std::max(ImGui::CalcTextSize(trK(Str::KeEiKey)).x,
                                                          ImGui::CalcTextSize(trK(Str::KeEiValue)).x) / sk()));
    const int feldX = gx + textB + kAbstand;
    const int feldB = std::max(1, gb - textB - kAbstand - kWerkzeug - kAbstand - kWerkzeug - kAbstand);
    const int knopfX = feldX + feldB + kAbstand;
    const int grossX = knopfX + kWerkzeug + kAbstand;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float textY = (px(kFeldH) - ImGui::GetFontSize()) * 0.5F;
    dl->AddText(ImVec2{bp(gx, gy).x, bp(gx, gy).y + textY}, stil(ImGuiCol_Text), trK(Str::KeEiKey));
    dl->AddText(ImVec2{bp(gx, gy + kFeldH + kAbstand).x, bp(gx, gy + kFeldH + kAbstand).y + textY}, stil(ImGuiCol_Text),
                trK(Str::KeEiValue));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(4.0F), std::max(0.0F, (px(kFeldH) - ImGui::GetFontSize()) * 0.5F)});
    merkeFeld("Entities:Key", feldX, gy, feldB, kFeldH);
    ImGui::SetCursorScreenPos(bp(feldX, gy));
    if (zeilenFeld("##eikey", u.schluessel, px(static_cast<float>(feldB)), true)) {
        // returnPressed: g_entityValueEntry->setFocus(); selectAll()
        ImGui::SetKeyboardFocusHere(0);
    }
    merkeFeld("Entities:Value", feldX, gy + kFeldH + kAbstand, feldB, kFeldH);
    ImGui::SetCursorScreenPos(bp(feldX, gy + kFeldH + kAbstand));
    if (zeilenFeld("##eivalue", u.wert, px(static_cast<float>(feldB)), false)) { schluesselAnwenden(); }
    ImGui::PopStyleVar();
    // select by key/value buttons
    const float kb = px(kWerkzeug);
    const float zeilenMitte = (px(kFeldH) - kb) * 0.5F;
    merkeFeld("Entities:SelectKey", knopfX, gy, kWerkzeug, kFeldH);
    if (werkzeugKnopf("##eiselkey", ImVec2{bp(knopfX, gy).x, bp(knopfX, gy).y + zeilenMitte}, kb, kb, "select", false,
                      tr(Str::KeEiSelKey))) {
        karte::waehleNachSchluesselWert(ed.karte, u.schluessel.c_str(), nullptr);
        auswahlNeu();
    }
    merkeFeld("Entities:SelectValue", knopfX, gy + kFeldH + kAbstand, kWerkzeug, kFeldH);
    if (werkzeugKnopf("##eiselvalue", ImVec2{bp(knopfX, gy + kFeldH + kAbstand).x, bp(knopfX, gy + kFeldH + kAbstand).y + zeilenMitte},
                      kb, kb, "select", false, tr(Str::KeEiSelValue))) {
        karte::waehleNachSchluesselWert(ed.karte, nullptr, u.wert.c_str());
        auswahlNeu();
    }
    merkeFeld("Entities:SelectKeyValue", grossX, gy, kWerkzeug, gitterH);
    if (werkzeugKnopf("##eiselkeyvalue", ImVec2{bp(grossX, gy).x, bp(grossX, gy).y + (px(static_cast<float>(gitterH)) - kb) * 0.5F}, kb,
                      kb, "select", false, tr(Str::KeEiSelKeyValue))) {
        karte::waehleNachSchluesselWert(ed.karte, u.schluessel.c_str(), u.wert.c_str());
        auswahlNeu();
    }
    // Knopfzeile: Raender 4, 0, 4, 0; Clear All | Delete Key | <- | <-> | -> | 👀
    const int ky = gy + gitterH + kAbstand;
    // QToolButton mit dem Text "👀" (Segoe UI Emoji): am Foto 27 breit (833..887 Bildpunkte)
    const int augeB = 27;
    const int fest = kWerkzeug + kAbstand + (kWerkzeug + 16) + kAbstand + kWerkzeug + kAbstand + augeB;
    const int druckB = std::max(1, (gb - fest - 2 * kAbstand) / 2);
    const int druckB2 = std::max(1, gb - fest - 2 * kAbstand - druckB);
    int kx = gx;
    merkeFeld("Entities:ClearAll", kx, ky, druckB, kKnopfH);
    if (druckKnopf(trK(Str::KeEiClearAll), "##eiclearall", bp(kx, ky), px(static_cast<float>(druckB)), px(kKnopfH))) {
        karte::alleSchluesselLoeschen(ed.karte, u.werte);
    }
    kx += druckB + kAbstand;
    merkeFeld("Entities:DeleteKey", kx, ky, druckB2, kKnopfH);
    if (druckKnopf(trK(Str::KeEiDeleteKey), "##eideletekey", bp(kx, ky), px(static_cast<float>(druckB2)), px(kKnopfH))) {
        if (!u.zeile.empty()) { karte::schluesselLoeschenAuswahl(ed.karte, u.zeile); }   // EntityInspector_clearKeyValue
    }
    kx += druckB2 + kAbstand;
    const auto verbunden = [&](bool zielende, bool ziele) {   // Select_ConnectedEntities( ..., g_focusToggleButton->isChecked() )
        std::string f;
        if (!karte::waehleVerbundene(ed.karte, zielende, ziele, &f)) {
            fehler(f);
            return;
        }
        auswahlNeu();
        if (u.autoFokus) { alleAnsichtenFokus(); }
    };
    const float ky0 = bp(kx, ky).y + (px(kKnopfH) - kb) * 0.5F;
    merkeFeld("Entities:SelectTargeting", kx, ky, kWerkzeug, kKnopfH);
    if (werkzeugKnopf("##eiseltargeting", ImVec2{bp(kx, ky).x, ky0}, kb, kb, "arrow_left", false, tr(Str::KeEiSelTargeting))) {
        verbunden(true, false);
    }
    kx += kWerkzeug + kAbstand;
    merkeFeld("Entities:SelectConnected", kx, ky, kWerkzeug + 16, kKnopfH);
    if (werkzeugKnopf("##eiselconnected", ImVec2{bp(kx, ky).x, ky0}, px(kWerkzeug + 16.0F), kb, "arrow_left_right", false,
                      tr(Str::KeEiSelConnected), 32.0F)) {
        verbunden(true, true);
    }
    kx += kWerkzeug + 16 + kAbstand;
    merkeFeld("Entities:SelectTargets", kx, ky, kWerkzeug, kKnopfH);
    if (werkzeugKnopf("##eiseltargets", ImVec2{bp(kx, ky).x, ky0}, kb, kb, "arrow_right", false, tr(Str::KeEiSelTargets))) {
        verbunden(false, true);
    }
    kx += kWerkzeug + kAbstand;
    merkeFeld("Entities:AutoFocus", kx, ky, augeB, kKnopfH);
    if (werkzeugKnopf("##eiautofocus", ImVec2{bp(kx, ky).x, ky0}, px(static_cast<float>(augeB)), kb, "eyes", u.autoFokus,
                      tr(Str::KeEiAutoFocus))) {
        u.autoFokus = !u.autoFokus;
        if (u.autoFokus) { alleAnsichtenFokus(); }
    }
}

// --- Attribute (EntityAttributeFactory) -------------------------------------------------

// angle_normalised (float_mod( angle, 360.0 )) und StringStream<32>( double )
std::string winkelText(double w) {
    double m = std::fmod(w, 360.0);
    if (m < 0) { m += 360.0; }
    char z[64];
    std::snprintf(z, sizeof(z), "%g", m);
    return z;
}
std::string zahlText(double w) {
    char z[64];
    std::snprintf(z, sizeof(z), "%g", w);
    return z;
}
double alsZahl(const std::string& s) { return std::atof(s.c_str()); }   // entry_get_float

// path_make_relative( filename, GlobalFileSystem().findRoot( filename ) ):
// der Pfad ab dem Spielordner, in dem die Datei liegt; sonst der ganze Pfad
// mit der Warnung von browse_sound / misc_model_dialog.
std::string spielRelativ(const std::string& pfad) {
    Editor& ed = e();
    std::string rel = pfad;
    std::replace(rel.begin(), rel.end(), '\\', '/');
    if (ed.karteSpiel) {
        for (std::string wurzel : ed.karteSpiel->ordner) {
            std::replace(wurzel.begin(), wurzel.end(), '\\', '/');
            if (!wurzel.empty() && wurzel.back() != '/') { wurzel += '/'; }
            if (rel.compare(0, wurzel.size(), wurzel) == 0) { return rel.substr(wurzel.size()); }
        }
    }
    warnung("WARNING: could not extract the relative path, using full path instead");
    return rel;
}

// file_dialog( ..., "sound" ): ja.game ohne soundtypes -> "wav" (SoundTypes_register);
// ModelLoader::Name: die Endungen der Modell-Lader (assimp, modeltypes="*").
const char* const kTonFilter = "wav sound files (*.wav)|*.wav";
const char* const kModellFilter =
    "All supported formats|*.md3;*.mdc;*.md2;*.mdl;*.ase;*.obj;*.lwo;*.3ds;*.fbx;*.dae;*.gltf;*.glb;*.ms3d;*.x;*.ply;*.stl;*.glm|"
    "md3 model|*.md3|ase model|*.ase|obj model|*.obj|lwo model|*.lwo|3ds model|*.3ds|fbx model|*.fbx|glm model|*.glm";

// misc_model_dialog / browse_sound: Startordner <Spiel>/models/ bzw. sound/
std::string startOrdnerIn(const char* unter) {
    Editor& ed = e();
    if (!ed.karteSpiel || ed.karteSpiel->ordner.empty()) { return {}; }
    std::string o = ed.karteSpiel->ordner.front();
    if (!o.empty() && o.back() != '/' && o.back() != '\\') { o += '/'; }
    return o + unter;
}

// CamAnglesButton: Camera_getAngles, Pitch umgedreht (nicht quake)
karte::Vektor3f kameraWinkel() {
    karte::Vektor3f w = e().kamera.winkel;
    w[0] = -w[0];
    return w;
}

void attributZeichnen(const std::string& schluessel, const karte::KlassenAttribut& a, ImVec2 pos, float breite, int index) {
    Editor& ed = e();
    const karte::GewaehlteSchluessel& w = ed.ent.werte;
    const std::string wert = w.wert(schluessel);
    const std::string id = "##eiattr" + std::to_string(index);
    std::string neu;
    const auto setzen = [&](const std::string& v) { karte::schluesselSetzenRueckgaengig(ed.karte, schluessel, v); };
    ImGui::SetCursorScreenPos(pos);
    const std::string& typ = a.typ;
    const float kamB = ImGui::CalcTextSize(trK(Str::KeEiCam)).x + px(16.0F);
    if (typ == "boolean") {   // BooleanAttribute
        bool an = std::atoi(wert.c_str()) != 0;
        if (qtHaken(id.c_str(), &an, static_cast<float>(kFeldH))) { setzen(an ? "1" : ""); }
    } else if (typ == "angle") {   // AngleAttribute: Feld + <-cam
        const std::string anzeige = wert.empty() ? std::string("0") : winkelText(alsZahl(wert));
        if (nichtModal((id + "w").c_str(), anzeige, breite - kamB - px(kAbstand), neu, true)) { setzen(winkelText(alsZahl(neu))); }
        ImGui::SameLine(0.0F, px(kAbstand));
        if (ImGui::Button((std::string(trK(Str::KeEiCam)) + id + "c").c_str(), ImVec2{kamB, 0.0F})) {
            setzen(winkelText(kameraWinkel()[1]));
        }
    } else if (typ == "direction") {   // DirectionAttribute: up/down/yaw + Feld + <-cam
        const float f = wert.empty() ? 0.0F : static_cast<float>(alsZahl(wert));
        int knopf = 2;
        if (!wert.empty() && f == -1.0F) { knopf = 0; }
        if (!wert.empty() && f == -2.0F) { knopf = 1; }
        const Str namen[3] = {Str::KeEiUp, Str::KeEiDown, Str::KeEiYaw};
        float verbraucht = 0.0F;
        for (int i = 0; i < 3; ++i) {
            if (i > 0) { ImGui::SameLine(0.0F, px(kAbstand)); }
            const float x0 = ImGui::GetCursorPosX();
            if (ImGui::RadioButton((std::string(trK(namen[i])) + id + "r" + std::to_string(i)).c_str(), knopf == i)) {   // applyRadio
                if (i == 0) {
                    setzen("-1");
                } else if (i == 1) {
                    setzen("-2");
                } else {
                    setzen(winkelText(alsZahl(ed.ent.felder.count(id + "w") != 0U ? ed.ent.felder[id + "w"] : "0")));
                }
            }
            verbraucht += ImGui::GetCursorPosX() - x0;
        }
        ImGui::SameLine(0.0F, px(kAbstand));
        const float feldB = std::max(px(20.0F), breite - (ImGui::GetCursorScreenPos().x - pos.x) - kamB - px(kAbstand));
        ImGui::BeginDisabled(knopf != 2);
        const std::string anzeige = knopf != 2 ? std::string() : (wert.empty() ? std::string("0") : winkelText(f));
        if (nichtModal((id + "w").c_str(), anzeige, feldB, neu, true)) { setzen(winkelText(alsZahl(neu))); }
        ImGui::EndDisabled();
        ImGui::SameLine(0.0F, px(kAbstand));
        if (ImGui::Button((std::string(trK(Str::KeEiCam)) + id + "c").c_str(), ImVec2{kamB, 0.0F})) {
            setzen(winkelText(kameraWinkel()[1]));
        }
    } else if (typ == "angles") {   // AnglesAttribute: pitch yaw roll + <-cam
        double v[3] = {0, 0, 0};
        if (!wert.empty() && std::sscanf(wert.c_str(), "%lf %lf %lf", &v[0], &v[1], &v[2]) != 3) { v[0] = v[1] = v[2] = 0; }
        const float teilB = (breite - kamB - px(3.0F * kAbstand)) / 3.0F;
        std::string t[3];
        bool anwenden = false;
        for (int i = 0; i < 3; ++i) {
            if (i > 0) { ImGui::SameLine(0.0F, px(kAbstand)); }
            const std::string anzeige = wert.empty() ? std::string("0") : winkelText(v[i]);
            t[i] = anzeige;
            const std::string fid = id + "a" + std::to_string(i);
            if (nichtModal(fid.c_str(), anzeige, teilB, neu, true)) {
                t[i] = neu;
                anwenden = true;
            } else if (ed.ent.felder.count(fid) != 0U) {
                t[i] = ed.ent.felder[fid];
            }
        }
        if (anwenden) { setzen(winkelText(alsZahl(t[0])) + ' ' + winkelText(alsZahl(t[1])) + ' ' + winkelText(alsZahl(t[2]))); }
        ImGui::SameLine(0.0F, px(kAbstand));
        if (ImGui::Button((std::string(trK(Str::KeEiCam)) + id + "c").c_str(), ImVec2{kamB, 0.0F})) {
            const karte::Vektor3f kw = kameraWinkel();
            setzen(winkelText(kw[0]) + ' ' + winkelText(kw[1]) + ' ' + winkelText(0));
        }
    } else if (typ == "vector3" || typ == "real3") {   // Vector3Attribute
        double v[3] = {0, 0, 0};
        if (!wert.empty() && std::sscanf(wert.c_str(), "%lf %lf %lf", &v[0], &v[1], &v[2]) != 3) { v[0] = v[1] = v[2] = 0; }
        const float teilB = (breite - px(2.0F * kAbstand)) / 3.0F;
        std::string t[3];
        bool anwenden = false;
        for (int i = 0; i < 3; ++i) {
            if (i > 0) { ImGui::SameLine(0.0F, px(kAbstand)); }
            const std::string anzeige = wert.empty() ? std::string("0") : zahlText(v[i]);
            t[i] = anzeige;
            const std::string fid = id + "v" + std::to_string(i);
            if (nichtModal(fid.c_str(), anzeige, teilB, neu, true)) {
                t[i] = neu;
                anwenden = true;
            } else if (ed.ent.felder.count(fid) != 0U) {
                t[i] = ed.ent.felder[fid];
            }
        }
        if (anwenden) { setzen(zahlText(alsZahl(t[0])) + ' ' + zahlText(alsZahl(t[1])) + ' ' + zahlText(alsZahl(t[2]))); }
    } else if (typ == "color" || typ == "sound" || typ == "model") {   // Feld mit "..." hinten
        const float knopfB = ImGui::GetFrameHeight();
        if (nichtModal((id + "w").c_str(), wert, breite - knopfB, neu)) { setzen(neu); }
        ImGui::SameLine(0.0F, 0.0F);
        const ImVec2 kp = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton((id + "b").c_str(), ImVec2{knopfB, knopfB})) {
            if (typ == "color") {   // ColorAttribute::browse
                float c[3] = {1, 1, 1};
                if (std::sscanf(wert.c_str(), "%f %f %f", &c[0], &c[1], &c[2]) != 3) { c[0] = c[1] = c[2] = 1; }
                const std::string sch = schluessel;
                farbDialogOeffnen(karte::Vektor3f(c[0], c[1], c[2]), [sch](const karte::Vektor3f& f) {
                    char z[64];
                    std::snprintf(z, sizeof(z), "%g %g %g", static_cast<double>(f[0]), static_cast<double>(f[1]), static_cast<double>(f[2]));
                    karte::schluesselSetzenRueckgaengig(e().karte, sch, z);
                });
            } else {   // browse_sound / misc_model_dialog: Dateiwahl von Windows (file_dialog)
                const bool ton = typ == "sound";
                const std::string pfad = platform::openFileDialog(trK(ton ? Str::KeEiOpenSound : Str::KeEiChooseModel),
                                                                  ton ? kTonFilter : kModellFilter, startOrdnerIn(ton ? "sound/" : "models/"));
                if (!pfad.empty()) { setzen(spielRelativ(pfad)); }
            }
        }
        karteIcon(ImGui::GetWindowDrawList(), "ellipsis", ImVec2{kp.x + (knopfB - px(16.0F)) * 0.5F, kp.y + (knopfB - px(16.0F)) * 0.5F},
                  px(16.0F), stil(ImGui::IsItemHovered() ? ImGuiCol_Text : ImGuiCol_TextDisabled));
    } else if (const karte::ListenTyp* liste = ed.karteSpiel ? ed.karteSpiel->klassen.liste(typ) : nullptr) {   // ListAttribute
        int aktuell = 0;
        for (std::size_t i = 0; i < liste->size(); ++i) {
            if ((*liste)[i].second == wert) { aktuell = static_cast<int>(i); break; }   // findValue
        }
        ImGui::SetNextItemWidth(breite);
        if (ImGui::BeginCombo(id.c_str(), liste->empty() ? "" : (*liste)[static_cast<std::size_t>(aktuell)].first.c_str())) {
            for (std::size_t i = 0; i < liste->size(); ++i) {
                if (ImGui::Selectable(((*liste)[i].first + "##" + std::to_string(i)).c_str(), static_cast<int>(i) == aktuell)) {
                    setzen((*liste)[i].second);   // activated
                }
            }
            ImGui::EndCombo();
        }
    } else {   // string, array, integer, real, target, targetname, shader, texture, skin
        if (nichtModal((id + "w").c_str(), wert, breite, neu)) { setzen(neu); }
    }
}

bool attributBekannt(const std::string& typ) {   // EntityAttributeFactory::create
    static const char* const bekannt[] = {"string", "array", "integer", "boolean", "real", "angle", "direction", "vector3", "real3",
                                          "angles", "color", "target", "targetname", "sound", "shader", "texture", "model", "skin"};
    for (const char* b : bekannt) {
        if (typ == b) { return true; }
    }
    Editor& ed = e();
    return ed.karteSpiel && ed.karteSpiel->klassen.liste(typ) != nullptr;
}

void attributeZeichnen(ImVec2 a, ImVec2 c) {
    Editor& ed = e();
    const EntitaetsKlasse* kl = aktuelleKlasse();
    ImGui::SetCursorScreenPos(a);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, px(kRollbalken));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{px(1.0F + kKindRand), px(1.0F + kKindRand)});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{px(kAbstand), px(kAbstand)});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(4.0F), std::max(0.0F, (px(kFeldH) - ImGui::GetFontSize()) * 0.5F)});
    ImGui::BeginChild("##eiattribute", ImVec2{c.x - a.x, c.y - a.y}, ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
    if (kl != nullptr) {
        // QGridLayout: Spalte 0 Beschriftung (rechtsbuendig), Spalte 1 Feld;
        // setColumnStretch( 0, 111 ), ( 1, 333 )
        std::vector<std::size_t> zeilen;
        float textB = 0.0F;
        for (std::size_t i = 0; i < kl->attribute.size(); ++i) {
            if (!attributBekannt(kl->attribute[i].second.typ)) { continue; }
            zeilen.push_back(i);
            textB = std::max(textB, ImGui::CalcTextSize(kl->attribute[i].first.c_str()).x);
        }
        const float platz = std::max(0.0F, ImGui::GetContentRegionAvail().x - px(kAbstand));
        const float spalte0 = std::max(textB, platz * 111.0F / 444.0F);
        const float spalte1 = std::max(px(20.0F), platz - spalte0);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        for (const std::size_t i : zeilen) {
            const auto& [name, attr] = kl->attribute[i];
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float tb = ImGui::CalcTextSize(name.c_str()).x;
            const float zeileH = px(kFeldH);
            dl->AddText(ImVec2{p.x + spalte0 - tb, p.y + (zeileH - ImGui::GetFontSize()) * 0.5F}, stil(ImGuiCol_Text), name.c_str());
            ImGui::SetCursorScreenPos(p);
            ImGui::InvisibleButton(("##eiattrname" + std::to_string(i)).c_str(), ImVec2{spalte0, zeileH});
            if (ImGui::IsItemHovered()) { tooltipAttribut(kl, static_cast<int>(i)); }
            attributZeichnen(name, attr, ImVec2{p.x + spalte0 + px(kAbstand), p.y}, spalte1, static_cast<int>(i));
            ImGui::SetCursorScreenPos(ImVec2{p.x, p.y + zeileH + px(kAbstand)});
            ImGui::Dummy(ImVec2{1.0F, 0.0F});
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(5);
    (void)ed;
}

}  // namespace

// --- Schriften -----------------------------------------------------------------------------

void setzeDialogSchrift(ImFont* normal, ImFont* fett) {
    g_dialog = normal;
    g_dialogFett = fett;
}

void dialogSchriftAn(bool fett) {
    ImFont* f = fett && g_dialogFett != nullptr ? g_dialogFett : g_dialog;
    if (f == nullptr) { f = ImGui::GetFont(); }
    // Tahoma 8 pt: fm.height() 13 logische Punkte (ImGui-Groesse = Ober- plus Unterlaenge)
    ImGui::PushFont(f, 13.0F * massstab() / std::max(0.01F, g_app->dpiScale * g_app->uiScale));
    ++g_dialogTiefe;
}

void dialogSchriftAus() {
    if (g_dialogTiefe <= 0) { return; }
    --g_dialogTiefe;
    ImGui::PopFont();
}

// --- Seite "Entities" (EntityInspector_constructWindow) ---------------------------------

void entitaetenSeiteZeichnen(int breite, int hoehe) {
    Editor& ed = e();
    EntitaetenOberflaeche& u = ed.ent;
    aktualisieren();
    dialogSchriftAn();
    karte::Teiler& t = u.teiler;
    if (t.kinder.size() != 4) {   // g_guiSettings.addSplitter( splitter, "EntityInspector/splitter", { 55, 175, 255, 255 } )
        t = karte::Teiler{};
        for (int i = 0; i < 4; ++i) {
            karte::Teiler::Kind k;
            k.hinweis = -1;
            t.kinder.push_back(k);
        }
        t.setzeGroessen(u.groessen.size() == 4 ? u.groessen : std::vector<int>{55, 175, 255, 255});
    }
    const ImVec2 o = ImGui::GetWindowPos();
    const auto bp = [&](int x, int y) { return ImVec2{o.x + px(static_cast<float>(x)), o.y + px(static_cast<float>(y))}; };
    const TeilerInhalt klassen = [&](int x, int y, int b, int h) {
        merkeFeld("Entities:Klassen", x, y, b, h);
        if (b > 0 && h > 0) { klassenListeZeichnen(bp(x, y), bp(x + b, y + h)); }
    };
    const TeilerInhalt kommentar = [&](int x, int y, int b, int h) {
        merkeFeld("Entities:Kommentar", x, y, b, h);
        if (b > 0 && h > 0) { kommentarZeichnen(bp(x, y), bp(x + b, y + h)); }
    };
    const TeilerInhalt behaelter = [&](int x, int y, int b, int h) {
        merkeFeld("Entities:Behaelter", x, y, b, h);
        if (b > 0 && h > 0) { behaelterZeichnen(o, x, y, b, h); }
    };
    const TeilerInhalt attribute = [&](int x, int y, int b, int h) {
        merkeFeld("Entities:Attribute", x, y, b, h);
        if (b > 0 && h > 0) { attributeZeichnen(bp(x, y), bp(x + b, y + h)); }
    };
    teilerImFenster("ent", t, true, breite, hoehe, {klassen, kommentar, behaelter, attribute});
    // g_guiSettings: die Groessen merken, wenn ein Griff gezogen wurde
    static bool gezogen = false;
    for (int i = 1; i < 4; ++i) {
        if (ImGui::GetActiveID() == ImGui::GetID(("##griffent" + std::to_string(i)).c_str())) { gezogen = true; }
    }
    if (gezogen && !ImGui::IsMouseDown(0)) {
        gezogen = false;
        u.groessen = t.groessen();
        einstellungenSpeichern();
    }
    // QToolTip der Validatoren
    if (!u.hinweis.empty() && ImGui::GetTime() < u.hinweisBis) {
        ImGui::SetTooltip("%s", u.hinweis.c_str());
    } else {
        u.hinweis.clear();
    }
    dialogSchriftAus();
}

// --- Entity List (entitylist.cpp, treemodel.cpp) -------------------------------------------

namespace {

// Eine Zeile des GraphTreeModel: die Karte, eine Entitaet, ein Primitiv.
struct ListenZeile {
    int tiefe = 0;
    std::string name;                     // node_get_name
    const void* schluessel = nullptr;     // Instanz
    std::shared_ptr<karte::Entitaet> ent;
    std::shared_ptr<karte::Primitiv> prim;
    bool hatKinder = false;
    bool sichtbar = true;                 // tree_view_filter (setRowHidden)
};

// node_get_name: Entitaeten ueber NamedEntity::name() - der Wert des
// Namensschluessels (KeyIsName::m_nameKey: classname, mit "Entity Names =
// Targetnames" targetname), sonst der Klassenname; Pinsel "brush", Patches "patch".
std::string listenName(const karte::Entitaet& ent) {
    if (e().ent.zielnamen) {
        const std::string n = ent.wert("targetname");
        if (!n.empty()) { return n; }
    }
    return ent.klassenname();
}

std::vector<ListenZeile> listenZeilen() {
    Editor& ed = e();
    EntitaetenOberflaeche& u = ed.ent;
    std::vector<ListenZeile> v;
    ListenZeile wurzel;
    wurzel.name = ed.name;   // Wurzel = Kartenname (Map_Name, mit "/" wie Qts Dateiwahl)
    std::replace(wurzel.name.begin(), wurzel.name.end(), '\\', '/');
    wurzel.schluessel = &ed.karte;
    wurzel.hatKinder = !ed.karte.entitaeten().empty();
    v.push_back(wurzel);
    // GraphTreeNode::Compare: string_compare der Namen, dann der Knoten
    // (Zeiger - hier die Reihenfolge der Karte)
    struct Eintrag {
        std::string name;
        std::size_t index;
        std::shared_ptr<karte::Entitaet> ent;
    };
    std::vector<Eintrag> ents;
    for (std::size_t i = 0; i < ed.karte.entitaeten().size(); ++i) {
        const auto& ent = ed.karte.entitaeten()[i];
        ents.push_back({listenName(*ent), i, ent});
    }
    std::stable_sort(ents.begin(), ents.end(), [](const Eintrag& a, const Eintrag& b) {
        const int n = std::strcmp(a.name.c_str(), b.name.c_str());
        return n != 0 ? n < 0 : a.index < b.index;
    });
    const bool wurzelOffen = u.listeOffeneKnoten.count("karte") != 0U;
    for (const Eintrag& en : ents) {
        if (!wurzelOffen) { break; }
        ListenZeile z;
        z.tiefe = 1;
        z.name = en.name;
        z.schluessel = en.ent.get();
        z.ent = en.ent;
        z.hatKinder = !en.ent->kinder().empty();
        v.push_back(z);
        char k[32];
        std::snprintf(k, sizeof(k), "%p", static_cast<const void*>(en.ent.get()));
        if (u.listeOffeneKnoten.count(k) == 0U) { continue; }
        struct Kind {
            std::string name;
            std::size_t index;
            std::shared_ptr<karte::Primitiv> p;
        };
        std::vector<Kind> kinder;
        for (std::size_t i = 0; i < en.ent->kinder().size(); ++i) {
            const auto& p = en.ent->kinder()[i];
            kinder.push_back({p->art() == karte::Primitiv::Art::Pinsel ? "brush" : "patch", i, p});
        }
        std::stable_sort(kinder.begin(), kinder.end(), [](const Kind& a, const Kind& b) {
            const int n = std::strcmp(a.name.c_str(), b.name.c_str());
            return n != 0 ? n < 0 : a.index < b.index;
        });
        for (const Kind& kd : kinder) {
            ListenZeile kz;
            kz.tiefe = 2;
            kz.name = kd.name;
            kz.schluessel = kd.p.get();
            kz.ent = en.ent;
            kz.prim = kd.p;
            v.push_back(kz);
        }
    }
    return v;
}

std::string knotenSchluessel(const ListenZeile& z) {
    if (z.tiefe == 0) { return "karte"; }
    char k[32];
    std::snprintf(k, sizeof(k), "%p", z.schluessel);
    return k;
}

bool zeileGewaehlt(const ListenZeile& z) {   // Instance::isSelected
    const karte::Auswahl& a = e().karte.auswahl;
    if (z.tiefe == 1) { return a.gewaehlt(z.ent.get()); }
    if (z.tiefe == 2) { return a.gewaehlt(z.ent.get(), z.prim.get()); }
    return false;
}
bool kindGewaehlt(const ListenZeile& z) {   // Instance::childSelected
    const karte::Auswahl& a = e().karte.auswahl;
    if (z.tiefe == 0) { return a.anzahl() != 0; }
    if (z.tiefe == 1) {
        return std::any_of(z.ent->kinder().begin(), z.ent->kinder().end(), [&](const auto& p) { return a.gewaehlt(z.ent.get(), p.get()); });
    }
    return false;
}

void zeileWaehlen(const ListenZeile& z, bool an) {   // Instance_getSelectable( *instance )->setSelected
    karte::Auswahl& a = e().karte.auswahl;
    if (z.tiefe == 1) {
        if (an) { a.waehlen(z.ent); } else { a.abwaehlen(z.ent); }
    } else if (z.tiefe == 2) {
        if (an) { a.waehlen(z.ent, z.prim); } else { a.abwaehlen(z.ent, z.prim); }
    }
}

// QColor::darker( faktor ): Wert im HSV-Raum geteilt
ImU32 dunkler(ImVec4 c, float faktor) {
    float h = 0;
    float s = 0;
    float v = 0;
    ImGui::ColorConvertRGBtoHSV(c.x, c.y, c.z, h, s, v);
    v = v * 100.0F / faktor;
    ImGui::ColorConvertHSVtoRGB(h, s, v, c.x, c.y, c.z);
    return ImGui::ColorConvertFloat4ToU32(c);
}

bool enthaeltOhneFall(const std::string& heu, const std::string& nadel) {   // string_in_string_nocase
    return klein(heu).find(klein(nadel)) != std::string::npos;
}
bool beginntOhneFall(const std::string& s, const std::string& anfang) {   // string_equal_prefix_nocase
    return s.size() >= anfang.size() && gleichOhneFall(s.substr(0, anfang.size()), anfang);
}

std::set<const void*> g_listeGewaehlt;   // QItemSelectionModel der Entity List

void entitaetenListeZeichnen() {
    Editor& ed = e();
    EntitaetenOberflaeche& u = ed.ent;
    if (!u.listeOffen) {
        // Filter_QLineEdit::event( Hide ): Filter leeren
        u.listeFilter.clear();
        return;
    }
    dialogSchriftAn();
    const ImGuiStyle& st = ImGui::GetStyle();
    const float titelH = ImGui::GetFontSize() + st.FramePadding.y * 2.0F;
    const ImVec2 vp = ImGui::GetMainViewport()->Pos;
    const ImVec2 bereich = ImGui::GetMainViewport()->Size;
    const ImVec2 groesse{px(350.0F), px(500.0F) + titelH};   // g_guiSettings.addWindow( window, "EntityList/geometry", 350, 500 )
    ImGui::SetNextWindowPos(ImVec2{vp.x + std::max(0.0F, (bereich.x - groesse.x) * 0.5F), vp.y + std::max(0.0F, (bereich.y - groesse.y) * 0.5F)},
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(groesse, ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const std::string titel = std::string(trK(Str::KeElTitle)) + "###karteentliste";
    if (ImGui::Begin(titel.c_str(), &u.listeOffen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::PopStyleVar();
        const ImVec2 o = ImGui::GetCursorScreenPos();
        const ImVec2 platz = ImGui::GetContentRegionAvail();
        const int b = static_cast<int>(std::lround(platz.x / sk()));
        const int h = static_cast<int>(std::lround(platz.y / sk()));
        const auto bp = [&](int x, int y) { return ImVec2{o.x + px(static_cast<float>(x)), o.y + px(static_cast<float>(y))}; };
        // QVBoxLayout( 4, 0, 4, 4 ): Baum, darunter die Zeile (Haken | Filter)
        const int zeileH = kFeldH;
        const int baumH = std::max(0, h - 4 - zeileH - kAbstand);
        // "deselect tree items on deselection in the scene"
        std::vector<ListenZeile> zeilen = listenZeilen();
        for (auto it = g_listeGewaehlt.begin(); it != g_listeGewaehlt.end();) {
            const auto z = std::find_if(zeilen.begin(), zeilen.end(), [&](const ListenZeile& r) { return r.schluessel == *it; });
            if (z == zeilen.end() || !zeileGewaehlt(*z)) {
                it = g_listeGewaehlt.erase(it);
            } else {
                ++it;
            }
        }
        // tree_view_filter: sichtbar, wenn ein Kind sichtbar ist oder der Name passt
        if (!u.listeFilter.empty()) {
            for (std::size_t i = zeilen.size(); i-- > 0;) {
                ListenZeile& z = zeilen[i];
                bool kindSichtbar = false;
                for (std::size_t j = i + 1; j < zeilen.size() && zeilen[j].tiefe > z.tiefe; ++j) {
                    if (zeilen[j].tiefe == z.tiefe + 1 && zeilen[j].sichtbar) { kindSichtbar = true; }
                }
                z.sichtbar = kindSichtbar || (u.listeVonAnfang ? beginntOhneFall(z.name, u.listeFilter) : enthaeltOhneFall(z.name, u.listeFilter));
            }
        }
        merkeFeld("EntityList:Baum", 4, 0, b - 8, baumH);
        ImGui::SetCursorScreenPos(bp(4, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, px(kRollbalken));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{px(1.0F), px(1.0F)});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        ImGui::BeginChild("##elbaum", ImVec2{px(static_cast<float>(b - 8)), px(static_cast<float>(baumH))}, ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_HorizontalScrollbar);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float zeile = px(kZeile);
        const ImVec4 hervor = ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive);   // QPalette::Highlight
        float breitest = 0.0F;
        for (const ListenZeile& z : zeilen) {
            if (z.sichtbar) { breitest = std::max(breitest, px(static_cast<float>(kEinzug * (z.tiefe + 1) + kTextRand)) + ImGui::CalcTextSize(z.name.c_str()).x); }
        }
        std::vector<const ListenZeile*> sichtbar;
        for (const ListenZeile& z : zeilen) {
            if (z.sichtbar) { sichtbar.push_back(&z); }
        }
        ImGuiListClipper clip;
        clip.Begin(static_cast<int>(sichtbar.size()), zeile);
        int geklickt = -1;
        while (clip.Step()) {
            for (int i = clip.DisplayStart; i < clip.DisplayEnd; ++i) {
                const ListenZeile& z = *sichtbar[static_cast<std::size_t>(i)];
                const ImVec2 p = ImGui::GetCursorScreenPos();
                const float einzug = px(static_cast<float>(kEinzug * z.tiefe));
                // BackgroundRole: gewaehlt Highlight.darker( 200 ), Kind gewaehlt .darker( 300 ), sonst Base
                const bool gew = zeileGewaehlt(z);
                const bool kind = !gew && kindGewaehlt(z);
                const float zb = std::max(ImGui::GetContentRegionAvail().x, breitest);
                if (gew || kind) {
                    dl->AddRectFilled(ImVec2{p.x + einzug + px(kEinzug), p.y}, ImVec2{p.x + zb, p.y + zeile}, dunkler(hervor, gew ? 200.0F : 300.0F));
                }
                const bool imModell = g_listeGewaehlt.count(z.schluessel) != 0U;
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4{0, 0, 0, 0});
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
                if (ImGui::Selectable(("##el" + knotenSchluessel(z)).c_str(), imModell, ImGuiSelectableFlags_None, ImVec2{zb, zeile})) {
                    geklickt = i;
                }
                ImGui::PopStyleColor(2);
                if (imModell) { dl->AddRect(ImVec2{p.x + einzug + px(kEinzug), p.y}, ImVec2{p.x + zb, p.y + zeile}, stil(ImGuiCol_NavCursor)); }
                if (z.hatKinder) {   // PE_IndicatorBranch
                    const bool offen = u.listeOffeneKnoten.count(knotenSchluessel(z)) != 0U;
                    ImGui::RenderArrow(dl, ImVec2{p.x + einzug + px(5.0F), p.y + (zeile - ImGui::GetFontSize()) * 0.5F}, stil(ImGuiCol_Text),
                                       offen ? ImGuiDir_Down : ImGuiDir_Right, 0.7F);
                }
                dl->AddText(ImVec2{p.x + einzug + px(kEinzug + kTextRand), p.y + (zeile - ImGui::GetFontSize()) * 0.5F}, stil(ImGuiCol_Text),
                            z.name.c_str());
            }
        }
        if (geklickt >= 0) {
            const ListenZeile& z = *sichtbar[static_cast<std::size_t>(geklickt)];
            const float mx = ImGui::GetIO().MousePos.x - (ImGui::GetWindowPos().x + px(1.0F) - ImGui::GetScrollX());
            const float einzug = px(static_cast<float>(kEinzug * z.tiefe));
            if (z.hatKinder && mx >= einzug && mx < einzug + px(kEinzug)) {   // Klick auf den Pfeil klappt
                const std::string k = knotenSchluessel(z);
                if (u.listeOffeneKnoten.count(k) != 0U) { u.listeOffeneKnoten.erase(k); } else { u.listeOffeneKnoten.insert(k); }
            } else {
                // ExtendedSelection: Klick = nur diese, Strg = umschalten, Umschalt = Bereich
                std::set<const void*> neu;
                const ImGuiIO& io = ImGui::GetIO();
                if (io.KeyCtrl) {
                    neu = g_listeGewaehlt;
                    if (neu.count(z.schluessel) != 0U) { neu.erase(z.schluessel); } else { neu.insert(z.schluessel); }
                    u.listeAnker = z.schluessel;
                } else if (io.KeyShift && u.listeAnker != nullptr) {
                    int a = -1;
                    for (std::size_t i = 0; i < sichtbar.size(); ++i) {
                        if (sichtbar[i]->schluessel == u.listeAnker) { a = static_cast<int>(i); }
                    }
                    if (a < 0) { a = geklickt; }
                    for (int i = std::min(a, geklickt); i <= std::max(a, geklickt); ++i) { neu.insert(sichtbar[static_cast<std::size_t>(i)]->schluessel); }
                } else {
                    neu.insert(z.schluessel);
                    u.listeAnker = z.schluessel;
                }
                // selectionChanged: abgewaehlte Zeilen abwaehlen, neue waehlen
                bool neueGewaehlt = false;
                for (const ListenZeile& r : zeilen) {
                    const bool vorher = g_listeGewaehlt.count(r.schluessel) != 0U;
                    const bool nachher = neu.count(r.schluessel) != 0U;
                    if (vorher && !nachher) { zeileWaehlen(r, false); }
                    if (!vorher && nachher) {
                        zeileWaehlen(r, true);
                        neueGewaehlt = true;
                    }
                }
                g_listeGewaehlt = neu;
                auswahlNeu();
                if (neueGewaehlt && u.listeAutoFokus) { alleAnsichtenFokus(); }
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(4);
        // Zeile: QCheckBox "AutoFocus on Selection" | Filter (QLineEdit, dehnt sich)
        const int zy = baumH + kAbstand;
        const int hakenB = checkboxBreite(trK(Str::KeEiAutoFocus));
        merkeFeld("EntityList:AutoFocus", 4, zy, hakenB, zeileH);
        ImGui::SetCursorScreenPos(bp(4, zy));
        if (qtHaken((std::string(trK(Str::KeEiAutoFocus)) + "##elautofocus").c_str(), &u.listeAutoFokus, static_cast<float>(zeileH))) {
            if (u.listeAutoFokus) { alleAnsichtenFokus(); }   // entitylist_focusSelected
        }
        const int fx = 4 + hakenB + kAbstand;
        const int fb = std::max(1, b - 4 - fx);
        merkeFeld("EntityList:Filter", fx, zy, fb, zeileH);
        const ImVec2 fa = bp(fx, zy);
        const ImVec2 fc = bp(fx + fb, zy + zeileH);
        dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(fa, fc, stil(ImGuiCol_FrameBg), st.FrameRounding);
        dl->AddRect(fa, fc, stil(ImGuiCol_Border), st.FrameRounding);
        const float g = px(16.0F);
        const ImVec2 ia{fa.x + px(4.0F), std::floor(fa.y + (fc.y - fa.y - g) * 0.5F)};
        ImGui::SetCursorScreenPos(ia);
        if (ImGui::InvisibleButton("##elfiltermodus", ImVec2{g, g})) {   // m_search_from_start ^= 1
            u.listeVonAnfang = !u.listeVonAnfang;
            einstellungenSpeichern();
        }
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::KeTexFilterTip)); }
        leistenSymbol(dl, u.listeVonAnfang ? "search_from_start" : "search", ia, g, true);
        const float x0 = ia.x + g + px(3.0F);
        const float loeschB = u.listeFilter.empty() ? 0.0F : g + px(4.0F);
        if (!u.listeFilter.empty()) {   // setClearButtonEnabled
            const ImVec2 la{fc.x - loeschB, ia.y};
            ImGui::SetCursorScreenPos(la);
            if (ImGui::InvisibleButton("##elfilterloeschen", ImVec2{g, g})) { u.listeFilter.clear(); }
            uiIcon(dl, UiIcon::Schliessen, la, g, stil(ImGui::IsItemHovered() ? ImGuiCol_Text : ImGuiCol_TextDisabled));
        }
        ImGui::SetCursorScreenPos(ImVec2{x0, fa.y});
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{0, 0, 0, 0});
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (fc.y - fa.y - ImGui::GetFontSize()) * 0.5F)});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0F);
        ImGui::SetNextItemWidth(std::max(1.0F, fc.x - loeschB - x0 - px(2.0F)));
        char puffer[256];
        std::snprintf(puffer, sizeof(puffer), "%s", u.listeFilter.c_str());
        // Filter_QLineEdit: Escape leert
        if (ImGui::InputText("##elfilter", puffer, sizeof(puffer), ImGuiInputTextFlags_EscapeClearsAll)) { u.listeFilter = puffer; }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    } else {
        ImGui::PopStyleVar();
    }
    ImGui::End();
    dialogSchriftAus();
}

// --- Entitaetenmenue (EntityMenu, EntityClassMenuInserter) -----------------------------

std::vector<karte::EntitaetMenuePunkt> g_menue;      // XYWnd::m_mnuDrop (beim ersten Mal gefuellt)
const void* g_menueKlassen = nullptr;                 // fuer welchen Klassenbestand
int g_menueStand = -1;

void menueFuellen() {
    Editor& ed = e();
    const void* bestand = ed.karteSpiel ? static_cast<const void*>(&ed.karteSpiel->klassen) : nullptr;
    // EntityClassMenu::unrealise: das Menue wird beim naechsten Mal neu gebaut
    if (!g_menue.empty() && g_menueKlassen == bestand && g_menueStand == ed.ent.klassenStand) { return; }
    g_menueKlassen = bestand;
    g_menueStand = ed.ent.klassenStand;
    std::vector<std::string> namen;
    if (ed.karteSpiel) {
        for (const EntitaetsKlasse* k : ed.karteSpiel->klassen.alle()) { namen.push_back(k->name); }   // GlobalEntityClassManager().forEach
    }
    g_menue = karte::entitaetMenue(namen);
}

// EntityMenu::mousePressEvent: Strg+M1 erzeugt ohne zu schliessen,
// M2 wandelt die Auswahl um (Strg+M2: Menue bleibt offen).
void menueEintrag(const std::string& klasse) {
    const ImGuiIO& io = ImGui::GetIO();
    const bool strg = io.KeyCtrl;
    if (strg) { ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false); }
    QtPunkt p;
    p.id = klasse.c_str();
    p.nrc = klasse.c_str();
    if (qtPunkt(p)) { entitaetMenueGewaehlt(klasse); }
    if (strg) { ImGui::PopItemFlag(); }
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        Editor& ed = e();
        karte::klasseSetzenAuswahl(ed.karte, klasse);   // Scene_EntitySetClassname_Selected
        auswahlNeu();
        if (!strg) { ImGui::CloseCurrentPopup(); }
    }
}

void entitaetenMenueZeichnen() {
    Editor& ed = e();
    EntitaetenOberflaeche& u = ed.ent;
    if (u.menueOeffnen) {
        u.menueOeffnen = false;
        menueFuellen();
        ImGui::OpenPopup("##entitaetenmenue");   // m_mnuDrop->popup( QCursor::pos() )
    }
    // QMenu (app_karte_qtmenue.cpp): Escape schliesst das offene Untermenue,
    // das naechste das Menue.
    if (qtPopupAnfang("##entitaetenmenue")) {
        for (const karte::EntitaetMenuePunkt& p : g_menue) {
            if (p.kinder.empty()) {
                menueEintrag(p.name);
            } else if (qtMenueAnfang(p.name.c_str(), true, p.name.c_str(), false)) {
                for (const std::string& k : p.kinder) { menueEintrag(k); }
                qtMenueEnde();
            }
        }
        qtPopupEnde();
    }
}

// --- Set Light intensity (DoLightIntensityDlg) ------------------------------------------

void lichtDialogZeichnen() {
    Editor& ed = e();
    EntitaetenOberflaeche& u = ed.ent;
    if (!u.lichtOffen) { return; }
    const std::string id = std::string(trK(Str::KeLiTitle)) + "###kartelicht";
    if (u.lichtOeffnen) {
        u.lichtOeffnen = false;
        ImGui::OpenPopup(id.c_str());
    }
    dialogSchriftAn();
    bool fertig = false;
    bool ok = false;
    // QFormLayout mit SetFixedSize: Tip, Intensity: [SpinBox], Checkbox, OK/Cancel
    if (ImGui::BeginPopupModal(id.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextUnformatted(trK(Str::KeLiTip));
        ImGui::AlignTextToFramePadding();
        const ImVec2 lp = ImGui::GetCursorScreenPos();
        ImGui::TextUnformatted(trK(Str::KeLiIntensity));
        // SpinBoxLabel (gtkutil/spinbox.h:96-150): waagerecht ziehen, je 20
        // logische Punkte ein Schritt (Strg: zehn), der Zeiger springt zurueck
        static ImVec2 zugStart;
        static float zugLetzte = 0.0F;   // Lage im letzten Bild (im Selbsttest springt der Zeiger nicht)
        static float zugSumme = 0.0F;
        static const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
        bool fokusFeld = ImGui::IsWindowAppearing();
        ImGui::SetCursorScreenPos(lp);
        ImGui::InvisibleButton("##lichtziehen", ImVec2{ImGui::CalcTextSize(trK(Str::KeLiIntensity)).x, ImGui::GetFrameHeight()});
        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            ImGui::SetMouseCursor(ImGui::IsItemActive() ? ImGuiMouseCursor_None : ImGuiMouseCursor_ResizeEW);
        }
        // m_spin->setFocus(); selectAll(): in ImGui erst beim Loslassen (ein
        // aktives Feld wuerde dem Ziehen die Maus nehmen)
        if (ImGui::IsItemDeactivated()) { fokusFeld = true; }
        if (ImGui::IsItemActivated()) {
            zugStart = ImGui::GetIO().MousePos;
            zugLetzte = zugStart.x;
            zugSumme = 0.0F;
        } else if (ImGui::IsItemActive()) {
            const float qt = std::max(1.0F, std::round(g_app->dpiScale));
            zugSumme += (ImGui::GetIO().MousePos.x - zugLetzte) / qt;   // m_dragAccum += pos.x - m_dragStart.x
            zugLetzte = ImGui::GetIO().MousePos.x;
            const int schritte = static_cast<int>(zugSumme / 20.0F);
            if (schritte != 0) {
                zugSumme -= static_cast<float>(schritte) * 20.0F;                    // m_dragAccum %= 20
                u.lichtWert += ImGui::GetIO().KeyCtrl ? schritte * 10 : schritte;   // stepBy
                if (!imTest) {                                                       // QCursor::setPos( m_dragStart )
                    ImGui::TeleportMousePos(zugStart);
                    zugLetzte = zugStart.x;
                }
            }
        }
        ImGui::SameLine(0.0F, px(kAbstand));
        ImGui::SetNextItemWidth(px(120.0F));
        if (fokusFeld) { ImGui::SetKeyboardFocusHere(); }
        ImGui::InputInt("##lichtwert", &u.lichtWert);   // SpinBox( -99999, 99999 )
        u.lichtWert = std::clamp(u.lichtWert, -99999, 99999);
        const bool enterImFeld = ImGui::IsItemDeactivated() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter));
        (void)qtHaken(trK(Str::KeLiDontShow), &u.lichtNichtZeigen, static_cast<float>(kCheckboxH));   // g_dontDoLightIntensityDlg
        // QDialogButtonBox: OK und Cancel rechts, je 79 breit (wie Preferences, gemessen)
        const float knopfB = px(79.0F);
        ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetContentRegionMax().x - knopfB * 2.0F - ImGui::GetStyle().ItemSpacing.x));
        if (ImGui::Button((std::string(trK(Str::KePrefOk)) + "##lichtok").c_str(), ImVec2{knopfB, 0.0F}) || enterImFeld ||
            (!ImGui::IsAnyItemActive() && (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)))) {
            fertig = true;
            ok = true;
            ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
        }
        ImGui::SameLine();
        if (ImGui::Button((std::string(trK(Str::KeCancel)) + "##lichtabbrechen").c_str(), ImVec2{knopfB, 0.0F}) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            fertig = true;   // ESC for default
            ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
        }
        if (fertig) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    dialogSchriftAus();
    if (!fertig) { return; }
    u.lichtOffen = false;
    if (ok) {   // g_iLastLightIntensity = intensity; entity->setKeyValue( "light", buf )
        u.lichtLetzte = u.lichtWert;
        if (u.offen.entitaet) { u.offen.entitaet->setzen("light", std::to_string(u.lichtWert)); }
        einstellungenSpeichern();
    }
    karte::befehlAbschliessen(ed.karte, u.offen);
}

// --- Choose Color (QColorDialog, Qt 5.15) ------------------------------------------------

// QColorDialogStaticData: 48 Grundfarben (g, r, b verschachtelt), 16 eigene (weiss)
unsigned grundFarbe(int i) {
    const int g = i / 12;
    const int r = (i / 3) % 4;
    const int b = i % 3;
    return static_cast<unsigned>((r * 255 / 3) << 16 | (g * 255 / 3) << 8 | (b * 255 / 2));
}
// QColor (Qt 5.15 qcolor.cpp) rechnet intern mit 16 Bit je Kanal (x * 0x101)
// und gibt mit qt_div_257 zurueck; qRound, qFuzzyIsNull, qFuzzyCompare wie Qt.
int div257(int x) { return (x - (x >> 8) + 0x80) >> 8; }
int qRundung(double d) { return d >= 0.0 ? static_cast<int>(d + 0.5) : static_cast<int>(d - static_cast<int>(d - 1) + 0.5) + static_cast<int>(d - 1); }
bool qFastNull(double d) { return std::fabs(d) <= 0.000000000001; }
bool qFastGleich(double a, double b) { return std::fabs(a - b) * 1000000000000.0 <= std::min(std::fabs(a), std::fabs(b)); }

// QColor::setRgb -> toHsv -> getHsv
void rgbZuHsv(int r, int g, int b, int& h, int& s, int& v) {
    const double rr = (r * 0x101) / 65535.0;
    const double gg = (g * 0x101) / 65535.0;
    const double bb = (b * 0x101) / 65535.0;
    const double mx = std::max({rr, gg, bb});
    const double mn = std::min({rr, gg, bb});
    const double d = mx - mn;
    const int wert = qRundung(mx * 65535.0);
    int ton = 65535;
    int satt = 0;
    if (!qFastNull(d)) {
        double hue = 0.0;
        satt = qRundung((d / mx) * 65535.0);
        if (qFastGleich(rr, mx)) {
            hue = (gg - bb) / d;
        } else if (qFastGleich(gg, mx)) {
            hue = 2.0 + (bb - rr) / d;
        } else {
            hue = 4.0 + (rr - gg) / d;
        }
        hue *= 60.0;
        if (hue < 0.0) { hue += 360.0; }
        ton = qRundung(hue * 100.0);
    }
    h = ton == 65535 ? -1 : ton / 100;
    s = div257(satt);
    v = div257(wert);
}

// QColor::setHsv -> toRgb -> rgb()
void hsvZuRgb(int h, int s, int v, int& r, int& g, int& b) {
    const int ton = h == -1 ? 65535 : (h % 360) * 100;
    const int satt = s * 0x101;
    const int wert = v * 0x101;
    if (satt == 0 || ton == 65535) {   // achromatic case
        r = g = b = div257(wert);
        return;
    }
    const double hh = ton == 36000 ? 0.0 : ton / 6000.0;
    const double ss = satt / 65535.0;
    const double vv = wert / 65535.0;
    const int i = static_cast<int>(hh);
    const double f = hh - i;
    const double p = vv * (1.0 - ss);
    double rr = 0;
    double gg = 0;
    double bb = 0;
    if ((i & 1) != 0) {
        const double q = vv * (1.0 - (ss * f));
        switch (i) {
        case 1: rr = q; gg = vv; bb = p; break;
        case 3: rr = p; gg = q; bb = vv; break;
        default: rr = vv; gg = p; bb = q; break;   // 5
        }
    } else {
        const double t = vv * (1.0 - (ss * (1.0 - f)));
        switch (i) {
        case 0: rr = vv; gg = t; bb = p; break;
        case 2: rr = p; gg = vv; bb = t; break;
        default: rr = t; gg = p; bb = vv; break;   // 4
        }
    }
    r = div257(qRundung(rr * 65535.0));
    g = div257(qRundung(gg * 65535.0));
    b = div257(qRundung(bb * 65535.0));
}
std::string htmlName(int r, int g, int b) {   // QColor::name()
    char z[16];
    std::snprintf(z, sizeof(z), "#%02x%02x%02x", r, g, b);
    return z;
}

void farbeRgb(FarbDialog& d, int r, int g, int b) {   // QColorShower::setRgb + _q_newColorTypedIn
    d.rgbOriginal = true;
    d.r = r;
    d.g = g;
    d.b = b;
    rgbZuHsv(r, g, b, d.h, d.s, d.v);
    d.html = htmlName(r, g, b);
}
void farbeHsv(FarbDialog& d, int h, int s, int v) {   // _q_newHsv -> QColorShower::setHsv
    if (h < -1 || static_cast<unsigned>(s) > 255U || static_cast<unsigned>(v) > 255U) { return; }
    d.rgbOriginal = false;
    d.h = h;
    d.s = s;
    d.v = v;
    hsvZuRgb(h, s, v, d.r, d.g, d.b);
    d.html = htmlName(d.r, d.g, d.b);
}
unsigned farbeWert(const FarbDialog& d) { return static_cast<unsigned>(d.r << 16 | d.g << 8 | d.b); }

// Ein QWellArray (Zellen 28 x 24, Rand 3, Spalten zuerst gezaehlt)
bool farbFelder(const char* id, int zeilen, int spalten, const unsigned* werte, int aktuell[2], int gewaehlt[2], int& index) {
    const ImVec2 a = ImGui::GetCursorScreenPos();
    const float zb = px(28.0F);
    const float zh = px(24.0F);
    ImGui::InvisibleButton(id, ImVec2{zb * static_cast<float>(spalten), zh * static_cast<float>(zeilen)});
    bool klick = false;
    if (ImGui::IsItemActivated() || ImGui::IsItemDeactivated()) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        const int c = static_cast<int>((m.x - a.x) / zb);
        const int r = static_cast<int>((m.y - a.y) / zh);
        if (c >= 0 && c < spalten && r >= 0 && r < zeilen) {
            if (ImGui::IsItemActivated()) {   // mousePressEvent: setCurrent
                aktuell[0] = r;
                aktuell[1] = c;
            } else {   // mouseReleaseEvent: setSelected( curRow, curCol )
                gewaehlt[0] = aktuell[0];
                gewaehlt[1] = aktuell[1];
                index = aktuell[0] + aktuell[1] * zeilen;
                klick = true;
            }
        }
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (int c = 0; c < spalten; ++c) {
        for (int r = 0; r < zeilen; ++r) {
            const ImVec2 z0{a.x + zb * static_cast<float>(c), a.y + zh * static_cast<float>(r)};
            const float rand = px(3.0F);
            const ImVec2 f0{z0.x + rand, z0.y + rand};
            const ImVec2 f1{z0.x + zb - rand, z0.y + zh - rand};
            dl->AddRect(f0, f1, stil(ImGuiCol_Border), 0.0F, 0, px(1.0F));   // PE_Frame sunken
            const unsigned w = werte[r + c * zeilen];
            const ImU32 farbe = IM_COL32((w >> 16) & 0xFF, (w >> 8) & 0xFF, w & 0xFF, 255);
            dl->AddRectFilled(ImVec2{f0.x + px(2.0F), f0.y + px(2.0F)}, ImVec2{f1.x - px(2.0F), f1.y - px(2.0F)}, farbe);
            if (gewaehlt[0] == r && gewaehlt[1] == c) { dl->AddRect(z0, ImVec2{z0.x + zb, z0.y + zh}, stil(ImGuiCol_Text), 0.0F, 0, px(1.0F)); }
        }
    }
    return klick;
}

void farbDialogZeichnen() {
    Editor& ed = e();
    FarbDialog& d = ed.ent.farbDialog;
    if (!d.offen) { return; }
    const std::string id = std::string(trK(Str::KeCdTitle)) + "###kartefarbe";
    if (d.oeffnen) {
        d.oeffnen = false;
        ImGui::OpenPopup(id.c_str());
    }
    dialogSchriftAn();
    bool fertig = false;
    bool ok = false;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{px(kAbstand), px(kAbstand)});
    if (ImGui::BeginPopupModal(id.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        auto& eigene = ed.ent.eigeneFarben;
        // links: Grundfarben, Pick Screen Color, Hinweis, eigene Farben, Add
        ImGui::BeginGroup();
        ImGui::TextUnformatted(trK(Str::KeCdBasic));
        unsigned grund[48];
        for (int i = 0; i < 48; ++i) { grund[i] = grundFarbe(i); }
        int index = -1;
        if (farbFelder("##cdgrund", 6, 8, grund, d.standardAktuell, d.standardGewaehlt, index)) {   // _q_newStandard
            const unsigned w = grund[index];
            farbeRgb(d, static_cast<int>((w >> 16) & 0xFF), static_cast<int>((w >> 8) & 0xFF), static_cast<int>(w & 0xFF));
            d.eigeneGewaehlt[0] = d.eigeneGewaehlt[1] = -1;
        }
        const float linksB = px(28.0F * 8.0F);
        ImGui::BeginDisabled(d.picken);
        if (ImGui::Button((std::string(trK(Str::KeCdPick)) + "##cdpick").c_str(), ImVec2{linksB, 0.0F})) {   // _q_pickScreenColor
            d.picken = true;
            d.vorPicken = farbeWert(d);
        }
        ImGui::EndDisabled();
        ImGui::TextUnformatted(d.pickText.c_str());   // lblScreenColorInfo
        ImGui::TextUnformatted(trK(Str::KeCdCustom));
        const int vorher[2] = {d.eigeneAktuell[0], d.eigeneAktuell[1]};
        if (farbFelder("##cdeigene", 2, 8, eigene.data(), d.eigeneAktuell, d.eigeneGewaehlt, index)) {   // _q_newCustom
            const unsigned w = eigene[static_cast<std::size_t>(index)];
            farbeRgb(d, static_cast<int>((w >> 16) & 0xFF), static_cast<int>((w >> 8) & 0xFF), static_cast<int>(w & 0xFF));
            d.standardGewaehlt[0] = d.standardGewaehlt[1] = -1;
        }
        if (vorher[0] != d.eigeneAktuell[0] || vorher[1] != d.eigeneAktuell[1]) {   // _q_nextCustom (currentChanged)
            d.naechsteEigene = d.eigeneAktuell[0] + 2 * d.eigeneAktuell[1];
        }
        ImGui::BeginDisabled(d.picken);
        if (ImGui::Button((std::string(trK(Str::KeCdAdd)) + "##cdadd").c_str(), ImVec2{linksB, 0.0F})) {   // _q_addCustom
            eigene[static_cast<std::size_t>(d.naechsteEigene)] = farbeWert(d);
            d.naechsteEigene = (d.naechsteEigene + 1) % 16;
            einstellungenSpeichern();
        }
        ImGui::EndDisabled();
        ImGui::EndGroup();
        ImGui::SameLine();
        // rechts: Farbfeld 220 x 200 (+ Rahmen), Helligkeit 20 breit; darunter QColorShower
        ImGui::BeginGroup();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 cp = ImGui::GetCursorScreenPos();
        const float pw = px(220.0F);
        const float ph = px(200.0F);
        ImGui::InvisibleButton("##cdfeld", ImVec2{pw + px(2.0F), ph + px(2.0F)});
        const ImVec2 c0{cp.x + px(1.0F), cp.y + px(1.0F)};
        if (ImGui::IsItemActive() && !d.picken) {   // QColorPicker::mousePress/MoveEvent
            const ImVec2 m = ImGui::GetIO().MousePos;
            const int x = static_cast<int>((m.x - c0.x) / sk());
            const int y = static_cast<int>((m.y - c0.y) / sk());
            const int hue = std::clamp(360 - x * 360 / (220 - 1), 0, 359);   // huePt
            const int sat = std::clamp(255 - y * 255 / (200 - 1), 0, 255);   // satPt
            farbeHsv(d, hue, sat, d.v);
        }
        // Farbfeld: setHsv( huePt, satPt, 200 ) - in Streifen gezeichnet
        const int streifen = 36;
        for (int i = 0; i < streifen; ++i) {
            const float x0 = c0.x + pw * static_cast<float>(i) / streifen;
            const float x1 = c0.x + pw * static_cast<float>(i + 1) / streifen;
            int r0 = 0;
            int g0 = 0;
            int b0 = 0;
            int r1 = 0;
            int g1 = 0;
            int b1 = 0;
            hsvZuRgb(360 - i * 360 / streifen, 255, 200, r0, g0, b0);
            hsvZuRgb(360 - (i + 1) * 360 / streifen, 255, 200, r1, g1, b1);
            const ImU32 grau = IM_COL32(200, 200, 200, 255);
            dl->AddRectFilledMultiColor(ImVec2{x0, c0.y}, ImVec2{x1, c0.y + ph}, IM_COL32(r0, g0, b0, 255), IM_COL32(r1, g1, b1, 255), grau, grau);
        }
        dl->AddRect(cp, ImVec2{cp.x + pw + px(2.0F), cp.y + ph + px(2.0F)}, stil(ImGuiCol_Border));   // Panel + Sunken
        {   // Kreuz (colPt)
            const float kx = c0.x + px(static_cast<float>((360 - std::max(0, d.h)) * (220 - 1) / 360));
            const float ky = c0.y + px(static_cast<float>((255 - d.s) * (200 - 1) / 255));
            dl->AddRectFilled(ImVec2{kx - px(9.0F), ky}, ImVec2{kx + px(11.0F), ky + px(2.0F)}, IM_COL32(0, 0, 0, 255));
            dl->AddRectFilled(ImVec2{kx, ky - px(9.0F)}, ImVec2{kx + px(2.0F), ky + px(11.0F)}, IM_COL32(0, 0, 0, 255));
        }
        ImGui::SameLine(0.0F, px(10.0F));
        const ImVec2 lp = ImGui::GetCursorScreenPos();
        const float lh = ph + px(2.0F);
        ImGui::InvisibleButton("##cdhell", ImVec2{px(20.0F), lh});
        if (ImGui::IsItemActive() && !d.picken) {   // QColorLuminancePicker: y2val
            const float dd = lh - px(2.0F * 4.0F) - 1.0F;
            const int val = 255 - static_cast<int>((ImGui::GetIO().MousePos.y - lp.y - px(4.0F)) * 255.0F / dd);
            farbeHsv(d, d.h, d.s, std::clamp(val, 0, 255));
        }
        {
            const float w = px(20.0F - 5.0F);
            int rt = 0;
            int gt = 0;
            int bt = 0;
            int ru = 0;
            int gu = 0;
            int bu = 0;
            hsvZuRgb(d.h, d.s, 255, rt, gt, bt);
            hsvZuRgb(d.h, d.s, 0, ru, gu, bu);
            dl->AddRectFilledMultiColor(ImVec2{lp.x + px(1.0F), lp.y + px(4.0F)}, ImVec2{lp.x + w - px(1.0F), lp.y + lh - px(4.0F)},
                                        IM_COL32(rt, gt, bt, 255), IM_COL32(rt, gt, bt, 255), IM_COL32(ru, gu, bu, 255), IM_COL32(ru, gu, bu, 255));
            dl->AddRect(ImVec2{lp.x, lp.y + px(3.0F)}, ImVec2{lp.x + w, lp.y + lh - px(3.0F)}, stil(ImGuiCol_Border));
            const float y = lp.y + px(4.0F) + static_cast<float>(255 - d.v) * (lh - px(8.0F) - 1.0F) / 255.0F;   // val2y
            dl->AddTriangleFilled(ImVec2{lp.x + w, y}, ImVec2{lp.x + w + px(5.0F), y + px(5.0F)}, ImVec2{lp.x + w + px(5.0F), y - px(5.0F)},
                                  stil(ImGuiCol_Text));
        }
        // QColorShower: Vorschau (mindestens 60 breit) | Hue/Sat/Val | Red/Green/Blue; HTML
        const ImVec2 sp = ImGui::GetCursorScreenPos();
        const float zh = ImGui::GetFrameHeight();
        const float vorschauB = px(60.0F);
        ImGui::Dummy(ImVec2{vorschauB, zh * 4.0F + px(3.0F * kAbstand)});
        dl->AddRectFilled(sp, ImVec2{sp.x + vorschauB, sp.y + zh * 3.0F + px(2.0F * kAbstand)}, IM_COL32(d.r, d.g, d.b, 255));
        dl->AddRect(sp, ImVec2{sp.x + vorschauB, sp.y + zh * 3.0F + px(2.0F * kAbstand)}, stil(ImGuiCol_Border));
        ImGui::SameLine();
        ImGui::BeginGroup();
        const float zahlB = px(60.0F);
        const auto spin = [&](Str text, const char* sid, int& wert, int bis) {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(trK(text));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(zahlB);
            const bool geaendert = ImGui::InputInt(sid, &wert);
            wert = std::clamp(wert, 0, bis);
            return geaendert;
        };
        if (ImGui::BeginTable("##cdzahlen", 4, ImGuiTableFlags_SizingFixedFit)) {
            const Str hsv[3] = {Str::KeCdHue, Str::KeCdSat, Str::KeCdVal};
            const Str rgb[3] = {Str::KeCdRed, Str::KeCdGreen, Str::KeCdBlue};
            int* hw[3] = {&d.h, &d.s, &d.v};
            int* rw[3] = {&d.r, &d.g, &d.b};
            for (int i = 0; i < 3; ++i) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(trK(hsv[i]));
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(zahlB);
                int hv = std::max(0, *hw[i]);
                if (ImGui::InputInt((std::string("##cdh") + std::to_string(i)).c_str(), &hv)) {   // hsvEd
                    hv = std::clamp(hv, 0, i == 0 ? 359 : 255);
                    int h = d.h;
                    int s = d.s;
                    int v = d.v;
                    (i == 0 ? h : i == 1 ? s : v) = hv;
                    farbeHsv(d, std::max(0, h), s, v);
                }
                ImGui::TableSetColumnIndex(2);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(trK(rgb[i]));
                ImGui::TableSetColumnIndex(3);
                ImGui::SetNextItemWidth(zahlB);
                int rv = *rw[i];
                if (ImGui::InputInt((std::string("##cdr") + std::to_string(i)).c_str(), &rv)) {   // rgbEd
                    rv = std::clamp(rv, 0, 255);
                    int r = d.r;
                    int g = d.g;
                    int b = d.b;
                    (i == 0 ? r : i == 1 ? g : b) = rv;
                    farbeRgb(d, r, g, b);
                }
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(trK(Str::KeCdHtml));
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(zahlB * 2.0F);
            char puffer[16];
            std::snprintf(puffer, sizeof(puffer), "%s", d.html.c_str());
            if (ImGui::InputText("##cdhtml", puffer, sizeof(puffer))) {   // htmlEd (#rrggbb / #rgb)
                d.html = puffer;
                const std::string t = d.html;
                const bool sechs = t.size() == 7 && t[0] == '#' && std::all_of(t.begin() + 1, t.end(), [](char c) { return std::isxdigit(static_cast<unsigned char>(c)) != 0; });
                const bool drei = t.size() == 4 && t[0] == '#' && std::all_of(t.begin() + 1, t.end(), [](char c) { return std::isxdigit(static_cast<unsigned char>(c)) != 0; });
                if (sechs || drei) {
                    const unsigned long w = std::strtoul(t.c_str() + 1, nullptr, 16);
                    int r = 0;
                    int g = 0;
                    int b = 0;
                    if (sechs) {
                        r = static_cast<int>((w >> 16) & 0xFF);
                        g = static_cast<int>((w >> 8) & 0xFF);
                        b = static_cast<int>(w & 0xFF);
                    } else {
                        r = static_cast<int>(((w >> 8) & 0xF) * 17);
                        g = static_cast<int>(((w >> 4) & 0xF) * 17);
                        b = static_cast<int>((w & 0xF) * 17);
                    }
                    const std::string h = d.html;
                    farbeRgb(d, r, g, b);
                    d.html = h;   // htEd behaelt den getippten Text
                }
            }
            ImGui::EndTable();
        }
        (void)spin;
        ImGui::EndGroup();
        ImGui::EndGroup();
        // QDialogButtonBox: OK (Vorgabe), Cancel
        ImGui::BeginDisabled(d.picken);
        const float knopfB = px(79.0F);   // QPushButton (Fusion), wie Preferences gemessen
        ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetContentRegionMax().x - knopfB * 2.0F - px(kAbstand)));
        if (ImGui::Button((std::string(trK(Str::KePrefOk)) + "##cdok").c_str(), ImVec2{knopfB, 0.0F})) {
            fertig = true;
            ok = true;
        }
        ImGui::SameLine();
        if (ImGui::Button((std::string(trK(Str::KeCancel)) + "##cdabbrechen").c_str(), ImVec2{knopfB, 0.0F})) { fertig = true; }
        ImGui::EndDisabled();
        // Farbe vom Bildschirm: Zeiger verfolgen (updateTimer 30 ms), Klick = fertig, ESC = zurueck
        if (d.picken) {
            int mx = 0;
            int my = 0;
            unsigned char rgb[3] = {0, 0, 0};
            if (platform::zeigerLage(mx, my) && platform::bildschirmFarbe(mx, my, rgb)) {
                farbeRgb(d, rgb[0], rgb[1], rgb[2]);
                char z[160];
                std::snprintf(z, sizeof(z), trK(Str::KeCdCursor), mx, my);
                d.pickText = z;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {   // handleColorPickingKeyPress
                ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
                farbeRgb(d, static_cast<int>((d.vorPicken >> 16) & 0xFF), static_cast<int>((d.vorPicken >> 8) & 0xFF),
                         static_cast<int>(d.vorPicken & 0xFF));
                d.picken = false;
                d.pickText = "\n";
            } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {   // handleColorPickingMouseButtonRelease
                d.picken = false;
                d.pickText = "\n";
            }
        } else if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            fertig = true;
            ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
        }
        if (fertig) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
    dialogSchriftAus();
    if (!fertig) { return; }
    d.offen = false;
    // color_dialog: color = Vector3( clr.redF(), clr.greenF(), clr.blueF() )
    if (ok && d.fertig) { d.fertig(karte::Vektor3f(static_cast<float>(d.r) / 255.0F, static_cast<float>(d.g) / 255.0F, static_cast<float>(d.b) / 255.0F)); }
    d.fertig = nullptr;
}

}  // namespace

void farbDialogOeffnen(const karte::Vektor3f& farbe, std::function<void(const karte::Vektor3f&)> fertig) {
    EntitaetenOberflaeche& u = e().ent;
    FarbDialog& d = u.farbDialog;
    d.offen = true;
    d.oeffnen = true;
    d.fertig = std::move(fertig);
    d.picken = false;
    d.pickText = "\n";
    d.standardGewaehlt[0] = d.standardGewaehlt[1] = -1;
    d.eigeneGewaehlt[0] = d.eigeneGewaehlt[1] = -1;
    if (!u.eigeneFarbenGeladen) {   // std::fill( customRgb, ..., 0xffffffff )
        u.eigeneFarben.fill(0xFFFFFFU);
        u.eigeneFarbenGeladen = true;
    }
    // QColor::fromRgbF( r, g, b ) -> setCurrentColor: setCurrentRgbColor, selectColor
    const auto kanal = [](float f) { return div257(qRundung(static_cast<double>(std::clamp(f, 0.0F, 1.0F)) * 65535.0)); };
    farbeRgb(d, kanal(farbe[0]), kanal(farbe[1]), kanal(farbe[2]));
    const unsigned w = farbeWert(d);
    for (int i = 0; i < 48; ++i) {
        if (grundFarbe(i) == w) {   // selectColor: Grundfarben (column = index / 6, row = index % 6)
            d.standardAktuell[0] = d.standardGewaehlt[0] = i % 6;
            d.standardAktuell[1] = d.standardGewaehlt[1] = i / 6;
            return;
        }
    }
    for (int i = 0; i < 16; ++i) {
        if ((u.eigeneFarben[static_cast<std::size_t>(i)] & 0xFFFFFFU) == w) {
            d.eigeneAktuell[0] = d.eigeneGewaehlt[0] = i % 2;
            d.eigeneAktuell[1] = d.eigeneGewaehlt[1] = i / 2;
            return;
        }
    }
}

// --- Entitaetenmenue: Erzeugen -------------------------------------------------------------

void entitaetenMenueOeffnen(bool kamera) {
    EntitaetenOberflaeche& u = e().ent;
    u.erzeugtZaehler = 0;      // g_entityCreationOffset = 0
    u.kameraMenue = kamera;    // g_bCamEntityMenu
    u.menueOeffnen = true;
}

std::vector<karte::EntitaetMenuePunkt> entitaetenMenuePunkte() {
    menueFuellen();
    return g_menue;
}

namespace {

// Entity_createFromSelection mit den Nachfragen (Licht, Modell).
void erzeugen(const std::string& name, const karte::Vektor3f& ursprung) {
    Editor& ed = e();
    EntitaetenOberflaeche& u = ed.ent;
    karte::ErzeugtErgebnis r = karte::entitaetErzeugen(ed.karte, name, ursprung);
    auswahlNeu();
    if (!r.fehler.empty()) { fehler(r.fehler); }
    if (!r.offen) { return; }
    if (r.lichtFragen) {   // DoLightIntensityDlg
        if (u.lichtNichtZeigen) {   // g_dontDoLightIntensityDlg: return true (Wert bleibt)
            if (r.entitaet) { r.entitaet->setzen("light", std::to_string(u.lichtLetzte)); }
            karte::befehlAbschliessen(ed.karte, r);
            return;
        }
        u.offen = r;
        u.lichtWert = u.lichtLetzte;
        u.lichtOffen = true;
        u.lichtOeffnen = true;
        return;
    }
    if (r.modellFragen) {   // misc_model_dialog (Dateiwahl von Windows)
        const std::string pfad = platform::openFileDialog(trK(Str::KeEiChooseModel), kModellFilter, startOrdnerIn("models/"));
        if (!pfad.empty() && r.entitaet) {
            const EntitaetsKlasse* kl = r.entitaet->klasse();
            r.entitaet->setzen(kl != nullptr ? kl->miscmodelKey : std::string("model"), spielRelativ(pfad));
        }
        karte::befehlAbschliessen(ed.karte, r);
    }
}

}  // namespace

void entitaetMenueGewaehlt(const std::string& klasse) {   // entitycreate_activated
    Editor& ed = e();
    EntitaetenOberflaeche& u = ed.ent;
    if (u.kameraMenue) {
        const karte::Vektor3f blick = karte::Vektor3f(0, 0, 0) - ed.kamera.vpn;   // -Camera_getViewVector
        const float fang = ed.raster.einrastGroesse();
        const float abstand = std::max(fang, 8.0F) * static_cast<float>(u.erzeugtZaehler);   // offset_for_multiple
        karte::Vektor3f p = blick * (64.0F + abstand) + ed.kamera.ursprung;
        karte::einrasten(p, fang);
        erzeugen(klasse, p);
    } else {   // g_pParentWnd->ActiveXY()->OnEntityCreate( item )
        XyFenster& f = ed.xy[static_cast<std::size_t>(ed.aktivXy)];
        // XYWnd_MouseToPoint: XY_ToPoint( x, y, true ), dritte Achse = Mitte der Workzone auf GetGridSize
        karte::Vektor3f p = f.ansicht.zuPunkt(f.erzeugX, f.erzeugY, ed.raster.einrastGroesse());
        const auto achse = static_cast<std::size_t>(karte::blickachse(f.ansicht.typ));
        const karte::Kasten& wz = ed.workzone;
        const float mitte = karte::zahlMitte(wz.origin[achse] - wz.extents[achse], wz.origin[achse] + wz.extents[achse]);
        p[achse] = karte::zahlEingerastet(mitte, ed.raster.groesse());
        const float abstand = std::max(8.0F, ed.raster.einrastGroesse()) * static_cast<float>(u.erzeugtZaehler);
        p[static_cast<std::size_t>(karte::dim1(f.ansicht.typ))] += abstand;   // g_vector3_axes[nDim1] * offset
        erzeugen(klasse, p);
    }
    ++u.erzeugtZaehler;
}

// --- Dialoge je Bild --------------------------------------------------------------------------

void entitaetenDialogeZeichnen() {
    entitaetenListeZeichnen();
    entitaetenMenueZeichnen();
    lichtDialogZeichnen();
    farbDialogZeichnen();
}

// --- Befehle (Entity_Construct, select.cpp, mainframe.cpp) -----------------------------------

void entitaetBefehl(const std::string& name) {
    Editor& ed = e();
    karte::Karte& k = ed.karte;
    EntitaetenOberflaeche& u = ed.ent;
    std::string f;
    if (name == "EntitiesConnect") {   // Entity_connectSelected
        if (!karte::entitaetenVerbinden(k, 0, &f)) { fehler(f); }
    } else if (name == "EntityMovePrimitivesToLast" || name == "EntityMovePrimitivesToFirst") {
        if (!karte::primitiveZurEntitaet(k, name == "EntityMovePrimitivesToLast", &f) && !f.empty()) { fehler(f); }
        auswahlNeu();
    } else if (name == "EntityUngroup") {   // Scene_EntitySetClassname_Selected( "worldspawn" )
        karte::klasseSetzenAuswahl(k, "worldspawn");
        auswahlNeu();
    } else if (name == "EntityUngroupPrimitives") {   // Entity_ungroupSelectedPrimitives
        karte::ErzeugtErgebnis r = karte::primitiveZurWelt(k);
        if (!r.fehler.empty()) { fehler(r.fehler); }
        karte::befehlAbschliessen(k, r);
        auswahlNeu();
    } else if (name == "EntityColorSet") {   // Entity_setColour
        const auto ent = karte::letzteGewaehlteEntitaet(k);
        if (!ent) { return; }
        float c[3] = {0, 0, 0};
        const std::string s = ent->wert("_color");
        if (!s.empty() && std::sscanf(s.c_str(), "%f %f %f", &c[0], &c[1], &c[2]) == 3) { u.farbe = karte::Vektor3f(c[0], c[1], c[2]); }
        farbDialogOeffnen(u.farbe, [](const karte::Vektor3f& farbe) {
            e().ent.farbe = farbe;
            karte::farbeSetzenAuswahl(e().karte, farbe);
            einstellungenSpeichern();
        });
    } else if (name == "EntityColorNormalize") {   // Entity_normalizeColor
        karte::Vektor3f n;
        if (karte::farbeNormierenAuswahl(k, &n)) {
            u.farbe = n;
            einstellungenSpeichern();
        }
    } else if (name == "EntityReloadDefinitions") {   // Entity_reloadDefinitions
        rueckfrageDann("Reload Entity Definitions", [] { entitaetenKlassenNeuLaden(); });
    } else if (name == "ExpandSelectionToEntities") {   // Scene_ExpandSelectionToEntities
        karte::auswahlAufEntitaetenErweitern(k);
        auswahlNeu();
    } else if (name == "SelectConnectedEntities") {   // Select_ConnectedEntities( true, true, false )
        if (!karte::waehleVerbundene(k, true, true, &f)) {
            fehler(f);
        } else {
            auswahlNeu();
        }
    } else if (name == "ToggleEntityList") {   // EntityList_toggleShown
        u.listeOffen = !u.listeOffen;
        if (u.listeOffen) { u.listeOffeneKnoten.insert("karte"); }   // expand map's root node for convenience
    }
}

}  // namespace bhed::gui::karteneditor

namespace bhed::gui {
// Aus buildFonts (main_win32.cpp): Tahoma 8 pt und fett fuer die Dialoge des Karten-Editors.
void setKartenDialogSchrift(ImFont* normal, ImFont* fett) { karteneditor::setzeDialogSchrift(normal, fett); }
}  // namespace bhed::gui
