// app_karte_brushexport.cpp - Karten-Editor, Stufe 4 Punkt 3: das Fenster des
// Plugins brushexport2 ("BrushExport-Plugin 3.0 by namespace").
//
// Portiert aus NetRadiant-Custom 20260114, contrib/brushexport:
// interface.cpp:36-122 (create_w_plugplug2: QGridLayout mit drei Radioknoepfen,
// Save / Save As / Cancel, "Ignored materials:", der Liste, dem Eingabefeld,
// Add / Remove und vier Haken; CreateWindow: einmal gebaut, show/hide),
// callbacks.cpp (OnExportClicked, OnAddMaterial, OnRemoveMaterial),
// export.cpp:367-388 (ExportSelection: "Nothing is selected.").
//
// Masse am Foto dlg_Plugins_brushexport2_Export_selected_as_Wavefront_Object_0.png
// (Massstab 2): Innenraum 278 x 478; die Liste bestimmt die Breite
// (QAbstractScrollArea::sizeHint 256 x 192), beide Spalten 125 breit
// (Abstand 6); Zeilen 11 / 39 / 67 (Radioknopf und Knopf, 22), das Etikett
// bei 95 (14, mittig), die Liste bei 115 (192), das Eingabefeld bei 313,
// Add / Remove bei 341, die Haken bei 369 / 395 / 421 / 447.
//
// Abweichung: "Save As" schlaegt nicht <Spiel>/base/models/ vor, sondern den
// Ordner karte_export in den Einstellungen des Programms (nie in den
// Spielordner); das Fenster ist nicht in der Groesse ziehbar.
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
#include "karte_qtdialog.h"
#include "platform.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace bhed::gui::karteneditor {

namespace {

namespace be = karte::brushexport;
using i18n::Str;

Editor& e() { return editor(); }

std::filesystem::path pfadVon(const std::string& p) { return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(p.data()), p.size())); }

bool schreiben(const std::string& pfad, const std::string& text) {
    std::ofstream f(pfadVon(pfad), std::ios::binary | std::ios::trunc);   // TextFileOutputStream
    if (!f) { return false; }
    f.write(text.data(), static_cast<std::streamsize>(text.size()));
    return static_cast<bool>(f);
}

// qtexture_t::color: aus der Kamera, sonst das Bild selbst gemittelt
std::array<float, 3> texturFarbe(const std::string& shader) {
    float c[3] = {0, 0, 0};
    if (kameraTexturFarbe(shader, c)) { return {c[0], c[1], c[2]}; }
    std::vector<unsigned char> rgba;
    int b = 0;
    int h = 0;
    if (kartenBildRgba(shader, rgba, b, h) && b > 0 && h > 0) {
        double s[3] = {0, 0, 0};
        const std::size_t n = static_cast<std::size_t>(b) * static_cast<std::size_t>(h);
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < 3; ++j) { s[j] += rgba[(i * 4) + j]; }
        }
        return {static_cast<float>(s[0] / (n * 255.0)), static_cast<float>(s[1] / (n * 255.0)), static_cast<float>(s[2] / (n * 255.0))};
    }
    return {0, 0, 0};
}

// ExportSelection
void exportieren() {
    Editor& ed = e();
    BrushExportZustand& z = ed.brushExport;
    if (ed.karte.auswahl.anzahl() == 0) {
        fehler("Nothing is selected.");
        qtMeldung("brushexport", "Nothing is selected.", MeldungArt::Fehler, false, nullptr);
        return;
    }
    be::Optionen o;
    o.modus = z.modus == 0 ? be::Zusammen::alle : (z.modus == 1 ? be::Zusammen::nachMaterial : be::Zusammen::keine);
    o.materialien = z.materialien;
    o.kurzeNamen = z.kurzeNamen;
    o.objekte = z.objekte;
    o.verschweissen = z.verschweissen;
    o.ignoriert = z.ignoriert;
    const be::Ergebnis r = be::exportieren(ed.karte, o, z.pfad, texturFarbe);
    if (!schreiben(r.objPfad, r.obj)) {
        fehler("Unable to open file");
        return;
    }
    if (o.materialien && !schreiben(r.mtlPfad, r.mtl)) {
        fehler("Unable to open material file");
        return;
    }
    ausgabe("brushexport::ExportSelection " + z.pfad);
    z.zuletzt = r.objPfad;
}

// OnExportClicked( choose_path )
void speichern(bool waehlen) {
    BrushExportZustand& z = e().brushExport;
    if (waehlen) {
        std::string start = z.pfad;
        if (start.empty()) {   // Abweichung: nie <Spiel>/base/models/
            start = (std::filesystem::path(platform::settingsDirectory()) / "karte_export").string();
            std::error_code ec;
            std::filesystem::create_directories(pfadVon(start), ec);
            start += "\\";
        }
        std::string p = kartenDateiDialog(false, "Save as Obj", "*.obj|*.obj", start);
        if (p.empty()) { return; }
        std::replace(p.begin(), p.end(), '\\', '/');   // QFileDialog liefert '/' (mtllib: find_last_of( "//" ))
        if (p.size() < 4 || (p.substr(p.size() - 4) != ".obj" && p.substr(p.size() - 4) != ".OBJ" && p.substr(p.size() - 4) != ".Obj")) { p += ".obj"; }
        z.pfad = p;   // "Save" wird frei, Tooltip "ReExport to <Pfad>"
    } else if (z.pfad.empty()) {
        return;
    }
    exportieren();
}

void materialDazu() {   // OnAddMaterial
    BrushExportZustand& z = e().brushExport;
    const std::string name = be::materialName(z.eingabe);
    if (!name.empty()) { z.ignoriert.push_back(name); }
}

void materialWeg() {   // OnRemoveMaterial: die markierten Zeilen
    BrushExportZustand& z = e().brushExport;
    if (z.gewaehlt >= 0 && z.gewaehlt < static_cast<int>(z.ignoriert.size())) {
        z.ignoriert.erase(z.ignoriert.begin() + z.gewaehlt);
        z.gewaehlt = -1;
        z.bearbeitet = -1;
    }
}

}  // namespace

void brushExportZeigen() {   // CreateWindow: show
    BrushExportZustand& z = e().brushExport;
    z.oeffnen = true;
    z.offen = true;
}

void brushExportZeichnen() {
    BrushExportZustand& z = e().brushExport;
    if (!z.offen) { return; }
    const std::string r0 = trK(Str::KeBeCollapse);
    const std::string r1 = trK(Str::KeBeCollapseMat);
    const std::string r2 = trK(Str::KeBeNoCollapse);
    const std::string kSave = trK(Str::KeBeSave);
    const std::string kSaveAs = trK(Str::KeBeSaveAs);
    const std::string kCancel = trK(Str::KeCancel);
    const std::string kAdd = trK(Str::KeBeAdd);
    const std::string kRemove = trK(Str::KeBeRemove);
    const std::string etikett = trK(Str::KeBeIgnored);
    const std::string h0 = trK(Str::KeBeShortNames);
    const std::string h1 = trK(Str::KeBeObjects);
    const std::string h2 = trK(Str::KeBeWeld);
    const std::string h3 = trK(Str::KeBeMtl);
    // QGridLayout: Spalte 0 Radioknoepfe / Add, Spalte 1 die Knoepfe; ueber beide
    // die Liste (256), das Etikett, das Eingabefeld und die Haken
    float s0 = std::max({qtd::radioHinweis(r0), qtd::radioHinweis(r1), qtd::radioHinweis(r2), qtd::knopfHinweis(kAdd)});
    float s1 = std::max({qtd::knopfHinweis(kSave), qtd::knopfHinweis(kSaveAs), qtd::knopfHinweis(kCancel), qtd::knopfHinweis(kRemove)});
    const float ueber = std::max({256.0F, qtd::textBreite(etikett), qtd::zeileHinweis(), qtd::hakenHinweis(h0), qtd::hakenHinweis(h1),
                                  qtd::hakenHinweis(h2), qtd::hakenHinweis(h3)});
    if (s0 + 6.0F + s1 < ueber) {   // QGridLayout::distributeMultiBox: qGeomCalc ueber die Spalten (gleich verteilt, wenigstens der Hinweis)
        std::vector<karte::LayoutFeld> sp(2);
        sp[0].hinweis = sp[0].minimum = static_cast<int>(s0);
        sp[1].hinweis = sp[1].minimum = static_cast<int>(s1);
        for (auto& f : sp) { f.leer = false; }
        karte::geomCalc(sp, 0, 2, 0, static_cast<int>(ueber), 6);
        s0 = static_cast<float>(sp[0].groesse);
        s1 = static_cast<float>(sp[1].groesse);
    }
    const float x1 = 11.0F + s0 + 6.0F;
    const float innenB = x1 + s1 + 11.0F;
    const float breit = innenB - 22.0F;
    const float innenH = 11 + 22 + 6 + 22 + 6 + 22 + 6 + 14 + 6 + 192 + 6 + 22 + 6 + 22 + 6 + 20 + 6 + 20 + 6 + 20 + 6 + 20 + 11;   // 478
    if (!qtd::beginnen(std::string(trK(Str::KeBeTitel)) + "###kartebrushexport", z.oeffnen, innenB, innenH, false)) {
        if (qtd::geschlossen()) { z.offen = false; }
        return;
    }
    qtd::merke("BeDialog", 0, 0, innenB, innenH);
    const char* tips[3] = {trK(Str::KeBeCollapseTip), trK(Str::KeBeCollapseMatTip), trK(Str::KeBeNoCollapseTip)};
    const std::string texte[3] = {r0, r1, r2};
    for (int i = 0; i < 3; ++i) {
        (void)qtd::radio(texte[i], "bemodus" + std::to_string(i), z.modus, i, 11.0F, 11.0F + static_cast<float>(i) * 28.0F, 22.0F);
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tips[i]); }
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (qtd::px(22.0F) - ImGui::GetTextLineHeight()) * 0.5F)});
    const auto knopf = [&](const std::string& t, const char* id, float x, float y, float b, bool aktiv) {
        ImGui::SetCursorScreenPos(qtd::bp(x, y));
        ImGui::BeginDisabled(!aktiv);
        const bool r = ImGui::Button((t + "##" + id).c_str(), ImVec2{qtd::px(b), qtd::px(22.0F)});
        ImGui::EndDisabled();
        qtd::merke(id, x, y, b, 22.0F);
        return r;
    };
    bool sp = knopf(kSave, "besave", x1, 11, s1, !z.pfad.empty());
    if (!z.pfad.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) { ImGui::SetTooltip("ReExport to %s", z.pfad.c_str()); }
    const bool spAls = knopf(kSaveAs, "besaveas", x1, 39, s1, true);
    const bool zu = knopf(kCancel, "becancel", x1, 67, s1, true);
    ImGui::PopStyleVar();
    // "Ignored materials:" mittig ueber beide Spalten
    qtd::text(etikett, 11.0F + std::floor((breit - qtd::textBreite(etikett)) * 0.5F), 95, 14);
    // QListWidget: Doppelklick / F2 bearbeitet, Entf entfernt
    ImGui::SetCursorScreenPos(qtd::bp(11, 115));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    if (ImGui::BeginChild("##beliste", ImVec2{qtd::px(breit), qtd::px(192)}, ImGuiChildFlags_Borders)) {
        for (int i = 0; i < static_cast<int>(z.ignoriert.size()); ++i) {
            ImGui::PushID(i);
            if (z.bearbeitet == i) {
                char puffer[256];
                std::strncpy(puffer, z.ignoriert[static_cast<std::size_t>(i)].c_str(), sizeof(puffer) - 1);
                puffer[sizeof(puffer) - 1] = 0;
                ImGui::SetNextItemWidth(-1.0F);
                if (ImGui::IsWindowAppearing() || ImGui::GetActiveID() == 0) { ImGui::SetKeyboardFocusHere(); }
                if (ImGui::InputText("##bearbeiten", puffer, sizeof(puffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    z.ignoriert[static_cast<std::size_t>(i)] = puffer;
                    z.bearbeitet = -1;
                } else {
                    z.ignoriert[static_cast<std::size_t>(i)] = puffer;
                    if (ImGui::IsItemDeactivated()) { z.bearbeitet = -1; }
                }
            } else if (ImGui::Selectable(z.ignoriert[static_cast<std::size_t>(i)].c_str(), z.gewaehlt == i, ImGuiSelectableFlags_AllowDoubleClick)) {
                z.gewaehlt = i;
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { z.bearbeitet = i; }
            }
            ImGui::PopID();
        }
        if (ImGui::IsWindowFocused() && z.bearbeitet < 0) {
            if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) { materialWeg(); }   // Del_QListWidget::keyPressEvent
            if (ImGui::IsKeyPressed(ImGuiKey_F2, false) && z.gewaehlt >= 0) { z.bearbeitet = z.gewaehlt; }
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    qtd::merke("beliste", 11, 115, breit, 192);
    // ed_materialname: Enter = Add
    (void)qtd::zeileFeld("beeingabe", z.eingabe, 11, breit, 313, 255);
    const bool enter = ImGui::IsItemDeactivated() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (qtd::px(22.0F) - ImGui::GetTextLineHeight()) * 0.5F)});
    if (knopf(kAdd, "beadd", 11, 341, s0, true) || enter) { materialDazu(); }
    if (knopf(kRemove, "beremove", x1, 341, s1, true)) { materialWeg(); }
    ImGui::PopStyleVar();
    (void)qtd::haken(h0, "beshort", z.kurzeNamen, 11, 369);
    (void)qtd::haken(h1, "beobjects", z.objekte, 11, 395);
    (void)qtd::haken(h2, "beweld", z.verschweissen, 11, 421);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", trK(Str::KeBeWeldTip)); }
    (void)qtd::haken(h3, "bemtl", z.materialien, 11, 447);
    qtd::beenden(false);
    if (sp) { speichern(false); }
    if (spAls) { speichern(true); }
    if (zu) { z.offen = false; }   // QWidget::hide
}

}  // namespace bhed::gui::karteneditor
