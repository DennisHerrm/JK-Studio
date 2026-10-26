// app_karte_bsp.cpp - Karten-Editor, Stufe 4 Punkte 1 und 2: fertige Karten
// oeffnen und das Originallicht behalten.
//
// File > Open .bsp... (eigener Eintrag nach Import..., Abweichung: NRC kann
// eine .bsp nur importieren - mapq3/plugin.cpp:239 "quake3 compiled maps" mit
// can_load = false): der Dialog "Open compiled map" listet jede maps/**.bsp
// der Spielordner (in .pk3 und lose, wie das Spiel sie sieht) oder nimmt eine
// Datei von der Platte. Die .bsp kommt in einen Arbeitsordner
// (<Einstellungen>/karte_bsp/<Name>/, nie in den Spielordner), q3map2 wandelt
// sie wie Map_ImportFile tryDecompile (map.cpp:1580-1597) um, dann wird die
// Karte geladen. File > Import... nimmt wie bei NRC auch eine .bsp (dann wird
// sie umgewandelt und eingefuegt).
//
// Build > Update entities only (Abweichung, eigener Eintrag): wurden seit dem
// Oeffnen nur Entities geaendert (Fingerabdruck der Geometrie gleich), laeuft
// q3map2 -bsp -onlyents auf eine Kopie der Original-.bsp; danach wird der
// Entity-Lump in die Original-Datei eingesetzt - Geometrie, Lightmaps und
// Lichtgitter bleiben Byte fuer Byte. Sonst ein Hinweis, dass neu gebaut und
// belichtet werden muss.
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
#include "bhed/karte/bspoeffnen.h"
#include "bhed/karte/editordaten.h"
#include "bhed/karte/kartenformat.h"
#include "karte_bau_plattform.h"
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

namespace fs = std::filesystem;

Editor& e() { return editor(); }

fs::path pfadVon(const std::string& p) {
    const std::u8string u(p.begin(), p.end());
    return fs::path(u);
}

std::string mitSchraeg(std::string p) {
    std::replace(p.begin(), p.end(), '\\', '/');
    return p;
}

bool dateiLesen(const std::string& pfad, std::string& daten) {
    std::ifstream f(pfadVon(pfad), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    daten = s.str();
    return true;
}

bool dateiSchreibenRoh(const std::string& pfad, const std::string& daten) {
    std::ofstream f(pfadVon(pfad), std::ios::binary | std::ios::trunc);
    return f && f.write(daten.data(), static_cast<std::streamsize>(daten.size()));
}

std::string stammVon(const std::string& pfad) {
    const std::u8string s = pfadVon(pfad).stem().u8string();
    return std::string(s.begin(), s.end());
}

// Der Arbeitsordner einer fertigen Karte (nie im Spielordner)
std::string arbeitsOrdner(const std::string& stamm) {
    return mitSchraeg(platform::settingsDirectory()) + "/karte_bsp/" + stamm;
}

// Der Aufruf von bhed_q3map2 (Map_ImportFile: AppPath + "q3map2." + RADIANT_EXECUTABLE)
karte::Q3map2Aufruf q3map2Aufruf() {
    karte::Q3map2Aufruf a;
    std::string programm = mitSchraeg(platform::executableDirectory());
    if (!programm.empty() && programm.back() != '/') { programm += '/'; }
    a.programm = programm + "bhed_q3map2.exe";
    a.spielTyp = kartenSpielNummer() == kRegelwerkJediOutcast ? "jk2" : "ja";   // q3map2_type der .game
    a.enginePfad = enginePfadAnzeige();
    a.benutzerPfad.clear();   // g_qeglobals.m_userEnginePath (ja/jk2.game ohne prefix)
    a.fsGame = bauSpielName();   // gamename_get
    return a;
}

// q3map2 ohne Fenster starten und warten (Q_Exec( ..., waitfor = true ));
// Abweichung: hoechstens 10 Minuten.
int q3map2Warten(const std::string& befehl, const std::string& ordner) {
    ausgabe(befehl);
    bauplattform::Prozess p;
    std::string grund;
    if (!p.starten(befehl, ordner, false, grund)) {
        fehler("Failed to execute the following command: " + befehl);
        return -1;
    }
    if (!p.warten(600000)) {
        p.beenden();
        return -1;
    }
    return p.rueckgabe();
}

// Die .bsp in den Arbeitsordner, umwandeln; liefert die _converted.map ("" bei Fehler).
std::string umwandeln(const std::string& daten, const std::string& stamm, std::string& ordner, bool brushPrimitives) {
    ordner = arbeitsOrdner(stamm);
    std::error_code ec;
    fs::create_directories(pfadVon(ordner), ec);
    const std::string bsp = ordner + "/" + stamm + ".bsp";
    const std::string original = ordner + "/" + stamm + "_original.bsp";
    if (!dateiSchreibenRoh(bsp, daten) || !dateiSchreibenRoh(original, daten)) {
        fehler("writing " + bsp + " failure");
        return {};
    }
    const std::string ziel = karte::convertZiel(bsp);
    fs::remove(pfadVon(ziel), ec);
    const int rc = q3map2Warten(karte::convertBefehl(q3map2Aufruf(), brushPrimitives, bsp), ordner);
    if (rc != 0 || !fs::is_regular_file(pfadVon(ziel), ec)) {
        fehler("q3map2 -convert failed (" + std::to_string(rc) + "): " + bsp);
        return {};
    }
    return ziel;
}

struct BspDialog {
    bool offen = false;
    bool init = false;
    std::vector<karte::BspEintrag> liste;
    char filter[128] = {};
    int wahl = -1;
};
BspDialog g_dlg;

constexpr int kB = 520;   // Innenraum (Qt-Regeln: Rand 11, Abstand 6, Knoepfe 80 x 22)
constexpr int kH = 388;
float g_sk = 1.0F;
ImVec2 g_o{0.0F, 0.0F};
ImVec2 bp(float x, float y) { return ImVec2{std::round(g_o.x + x * g_sk), std::round(g_o.y + y * g_sk)}; }
float px(float l) { return std::round(l * g_sk); }

std::string kleinVon(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

}  // namespace

// --- Punkt 1: oeffnen ---------------------------------------------------------------

bool bspOeffnenAus(const std::string& anzeige, const std::string& daten, const std::string& begleitName, const std::string& begleit) {
    Editor& ed = e();
    const std::string stamm = stammVon(anzeige);
    ausgabe("Opening compiled map " + anzeige);
    std::string ordner;
    // Stufe 8a Punkt 2 (Abweichung, eigene Funktion): liegt <karte>.bhed daneben und passt
    // ihr Fingerabdruck zur Geometrie dieser .bsp, kommt statt des Dekompilats die genaue Quelle
    std::string quelle;
    if (!begleit.empty()) {
        char z[1024];
        switch (karte::editorDatenLesen(begleit, daten, quelle)) {
        case karte::EditorDatenStand::Passt:
            std::snprintf(z, sizeof(z), trK(Str::KeEdGefunden), begleitName.c_str());
            ausgabe(z);
            break;
        case karte::EditorDatenStand::AndereBsp:
            std::snprintf(z, sizeof(z), trK(Str::KeEdAndere), begleitName.c_str());
            warnung(z);
            break;
        case karte::EditorDatenStand::Kaputt:
            std::snprintf(z, sizeof(z), trK(Str::KeEdKaputt), begleitName.c_str());
            warnung(z);
            break;
        }
    }
    std::string text;
    if (!quelle.empty()) {
        ordner = arbeitsOrdner(stamm);
        std::error_code ec;
        fs::create_directories(pfadVon(ordner), ec);
        if (!dateiSchreibenRoh(ordner + "/" + stamm + ".bsp", daten) || !dateiSchreibenRoh(ordner + "/" + stamm + "_original.bsp", daten)) {
            fehler("writing " + ordner + "/" + stamm + ".bsp failure");
            return false;
        }
        text = quelle;
    } else {
        // Wie Open: die neue Karte bekommt das Format "New map Brush Type" (Map_Free:
        // Brush_unlatchPreferences); q3map2 schreibt map bzw. map_bp danach
        // (map.cpp:1589: TEXDEFTYPEID_QUAKE ? "map" : "map_bp").
        const std::string konvertiert = umwandeln(daten, stamm, ordner, std::clamp(ed.pinselTyp, 0, 2) != 0);
        if (konvertiert.empty()) { return false; }
        if (!dateiLesen(konvertiert, text)) {
            fehler("reading " + konvertiert + " failure");
            return false;
        }
    }
    // <Arbeitsordner>/<Name>.map: dort sucht -onlyents die Entitaeten
    const std::string karteDatei = ordner + "/" + stamm + ".map";
    if (!dateiSchreibenRoh(karteDatei, text)) {
        fehler("writing " + karteDatei + " failure");
        return false;
    }
    zuletztEintragen(karteDatei);   // MRU_AddFile vor dem Laden (map.cpp:1848)
    const bool ok = karteOeffnen(karteDatei);
    // Stufe 8a Punkt 3: ohne Editor-Daten die misc_model aus der Geometrie (im Hintergrund)
    if (ok && quelle.empty()) { bspRueckStarten(daten, ordner, stamm, karteDatei); }
    ed.bspHerkunft = BspHerkunft{};
    if (ok) {
        ed.bspHerkunft.aktiv = true;
        ed.bspHerkunft.quelle = anzeige;
        ed.bspHerkunft.ordner = ordner;
        ed.bspHerkunft.stamm = stamm;
        ed.bspHerkunft.fingerabdruck = karte::geometrieFingerabdruck(ed.karte);
    }
    return ok;
}

bool bspOeffnenDatei(const std::string& pfad) {
    std::string daten;
    if (!dateiLesen(pfad, daten)) {
        fehler("Open file " + mitSchraeg(pfad) + " for read... failure");
        return false;
    }
    std::string begleit;
    const std::string begleitPfad = karte::editorDatenPfad(pfad);
    if (!dateiLesen(begleitPfad, begleit)) { begleit.clear(); }
    return bspOeffnenAus(mitSchraeg(pfad), daten, mitSchraeg(begleitPfad), begleit);
}

bool bspOeffnenSpiel(const std::string& pfad) {
    const Editor& ed = e();
    std::string daten;
    if (!ed.karteSpiel || !ed.karteSpiel->fs.dateiLesen(pfad, daten)) {
        fehler("Open file " + pfad + " for read... failure");
        return false;
    }
    std::string begleit;
    const std::string begleitPfad = karte::editorDatenPfad(pfad);
    if (!ed.karteSpiel->fs.dateiLesen(begleitPfad, begleit)) { begleit.clear(); }
    return bspOeffnenAus(pfad, daten, begleitPfad, begleit);
}

// File > Import... mit einer .bsp: tryDecompile (map.cpp:1580-1622) - umwandeln, einfuegen
bool bspImportieren(const std::string& pfad) {
    std::string daten;
    if (!dateiLesen(pfad, daten)) { return false; }
    std::string ordner;
    // Import: das Format der offenen Karte (GlobalBrushCreator().getFormat())
    const std::string konvertiert = umwandeln(daten, stammVon(pfad), ordner, e().karte.kontext.format != karte::PinselFormat::Quake3);
    if (konvertiert.empty()) { return false; }
    std::string text;
    if (!dateiLesen(konvertiert, text)) { return false; }
    karte::karteImportieren(e().karte, karte::textmodusLesen(text));
    auswahlGeaendert();
    return true;
}

void bspOeffnenDialog() {
    rueckfrageDann("Open Map", [] {
        g_dlg = BspDialog{};
        g_dlg.offen = true;
        if (e().karteSpiel) { g_dlg.liste = karte::bspListe(e().karteSpiel->fs); }
    });
}

std::vector<karte::BspEintrag> bspDialogListe() { return g_dlg.liste; }
bool bspDialogOffen() { return g_dlg.offen; }

void zeichneBspDialog() {
    if (!g_dlg.offen) { return; }
    g_sk = massstab();
    const std::string titel = std::string(trK(Str::KeBspTitel)) + "###kartebspoeffnen";
    if (!g_dlg.init) {
        ImGui::OpenPopup(titel.c_str());
        g_dlg.init = true;
    }
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    ImGui::SetNextWindowSize(ImVec2{px(static_cast<float>(kB)), px(static_cast<float>(kH)) + titelH}, ImGuiCond_Always);
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});
    bool offen = true;
    const bool sichtbar = ImGui::BeginPopupModal(titel.c_str(), &offen,
                                                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                                                     ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    if (!sichtbar) {
        g_dlg.offen = false;
        return;
    }
    g_o = ImGui::GetCursorScreenPos();
    merkeFeld("BspOeffnen:Dialog", 0, 0, kB, kH);
    dialogSchriftAn();
    int ergebnis = 0;   // 1 Oeffnen, 2 Abbrechen, 3 Datei
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float zeileH = px(22.0F);
    const float textY = (zeileH - ImGui::GetTextLineHeight()) * 0.5F;
    // Filter
    const char* filterText = trK(Str::KeBspFilter);
    const float filterB = std::ceil(ImGui::CalcTextSize(filterText).x / g_sk);
    dl->AddText(ImVec2{bp(11, 0).x, bp(0, 11).y + textY}, ImGui::GetColorU32(ImGuiCol_Text), filterText);
    ImGui::SetCursorScreenPos(bp(11.0F + filterB + 6.0F, 11.0F));
    ImGui::SetNextItemWidth(px(static_cast<float>(kB) - 22.0F - filterB - 6.0F));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, textY)});
    merkeFeld("BspOeffnen:Filter", 11 + static_cast<int>(filterB) + 6, 11, kB - 22 - static_cast<int>(filterB) - 6, 22);
    (void)ImGui::InputText("##bspfilter", g_dlg.filter, sizeof(g_dlg.filter));
    ImGui::PopStyleVar();
    // Liste: Karte | Aus
    const int listeY = 11 + 22 + 6;
    const int listeH = kH - listeY - 6 - 30 - 6 - 22 - 11 - 3 * 22;
    merkeFeld("BspOeffnen:Liste", 11, listeY, kB - 22, listeH);
    ImGui::SetCursorScreenPos(bp(11, static_cast<float>(listeY)));
    ImGui::BeginChild("##bspliste", ImVec2{px(static_cast<float>(kB - 22)), px(static_cast<float>(listeH))}, ImGuiChildFlags_Borders);
    const std::string f = kleinVon(g_dlg.filter);
    const float spalte = px(260.0F);
    bool doppelt = false;
    int sichtbare = 0;
    for (int i = 0; i < static_cast<int>(g_dlg.liste.size()); ++i) {
        const karte::BspEintrag& b = g_dlg.liste[static_cast<std::size_t>(i)];
        if (!f.empty() && kleinVon(b.pfad).find(f) == std::string::npos) { continue; }
        ++sichtbare;
        const std::string name = b.pfad.size() > 5 ? b.pfad.substr(5) : b.pfad;   // ohne "maps/"
        const ImVec2 z = ImGui::GetCursorScreenPos();
        if (ImGui::Selectable(("##bsp" + std::to_string(i)).c_str(), g_dlg.wahl == i, ImGuiSelectableFlags_AllowDoubleClick,
                              ImVec2{0.0F, px(16.0F)})) {
            g_dlg.wahl = i;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { doppelt = true; }
        }
        ImDrawList* ldl = ImGui::GetWindowDrawList();
        ldl->AddText(ImVec2{z.x + px(3.0F), z.y + (px(16.0F) - ImGui::GetTextLineHeight()) * 0.5F}, ImGui::GetColorU32(ImGuiCol_Text), name.c_str());
        std::string quelle = mitSchraeg(b.archiv);
        const std::size_t s = quelle.find_last_of('/', quelle.size() >= 2 ? quelle.size() - 2 : 0);
        if (s != std::string::npos) { quelle = quelle.substr(s + 1); }
        ldl->AddText(ImVec2{z.x + spalte, z.y + (px(16.0F) - ImGui::GetTextLineHeight()) * 0.5F}, ImGui::GetColorU32(ImGuiCol_TextDisabled),
                     quelle.c_str());
    }
    if (sichtbare == 0) { ImGui::TextDisabled("%s", trK(Str::KeBspKeine)); }
    ImGui::EndChild();
    if (doppelt) { ergebnis = 1; }
    // Stufe 8a (eigene Funktion, nicht in NRC): was beim Oeffnen zurueckgewonnen wird
    {
        Editor& ed = e();
        const int hakenY = listeY + listeH + 6;
        merkeFeld("BspOeffnen:Modelle", 11, hakenY, kB - 22, 22);
        ImGui::SetCursorScreenPos(bp(11, static_cast<float>(hakenY)));
        if (ImGui::Checkbox((std::string(trK(Str::KeRkModelle)) + "##bsprueckmodelle").c_str(), &ed.bspRueck.modelle)) { einstellungenSpeichern(); }
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", trK(Str::KeRkModelleTip)); }
        merkeFeld("BspOeffnen:Flaechen", 11, hakenY + 22, kB - 22, 22);
        ImGui::SetCursorScreenPos(bp(11, static_cast<float>(hakenY + 22)));
        if (!ed.bspRueck.modelle) { ImGui::BeginDisabled(); }
        if (ImGui::Checkbox((std::string(trK(Str::KeRkFlaechenOpt)) + "##bsprueckflaechen").c_str(), &ed.bspRueck.flaechen)) { einstellungenSpeichern(); }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) { ImGui::SetTooltip("%s", trK(Str::KeRkFlaechenOptTip)); }
        if (!ed.bspRueck.modelle) { ImGui::EndDisabled(); }
        merkeFeld("BspOeffnen:Lichter", 11, hakenY + 44, kB - 22, 22);
        ImGui::SetCursorScreenPos(bp(11, static_cast<float>(hakenY + 44)));
        if (ImGui::Checkbox((std::string(trK(Str::KeLsOpt)) + "##bsprueckLichter").c_str(), &ed.bspRueck.lichter)) { einstellungenSpeichern(); }
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", trK(Str::KeLsOptTip)); }
    }
    // Hinweis
    const int hinweisY = listeY + listeH + 6 + 3 * 22;
    ImGui::SetCursorScreenPos(bp(11, static_cast<float>(hinweisY)));
    ImGui::PushTextWrapPos(bp(static_cast<float>(kB - 11), 0).x);
    ImGui::TextDisabled("%s", trK(Str::KeBspHinweis));
    ImGui::PopTextWrapPos();
    // Knoepfe: "Aus Datei..." links, Oeffnen / Abbrechen rechts
    const int knopfY = kH - 11 - 22;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, textY)});
    const float dateiB = std::max(80.0F, std::ceil(ImGui::CalcTextSize(trK(Str::KeBspDatei)).x / g_sk) + 8.0F);
    merkeFeld("BspOeffnen:Datei", 11, knopfY, static_cast<int>(dateiB), 22);
    ImGui::SetCursorScreenPos(bp(11, static_cast<float>(knopfY)));
    if (ImGui::Button((std::string(trK(Str::KeBspDatei)) + "##bspdatei").c_str(), ImVec2{px(dateiB), zeileH})) { ergebnis = 3; }
    merkeFeld("BspOeffnen:Oeffnen", kB - 11 - 166, knopfY, 80, 22);
    ImGui::SetCursorScreenPos(bp(static_cast<float>(kB - 11 - 166), static_cast<float>(knopfY)));
    if (g_dlg.wahl < 0) { ImGui::BeginDisabled(); }
    if (ImGui::Button((std::string(trK(Str::KeBspOeffnen)) + "##bspoeffnen").c_str(), ImVec2{px(80.0F), zeileH})) { ergebnis = 1; }
    if (g_dlg.wahl < 0) { ImGui::EndDisabled(); }
    merkeFeld("BspOeffnen:Abbrechen", kB - 11 - 80, knopfY, 80, 22);
    ImGui::SetCursorScreenPos(bp(static_cast<float>(kB - 11 - 80), static_cast<float>(knopfY)));
    if (ImGui::Button((std::string(trK(Str::KeCancel)) + "##bspabbrechen").c_str(), ImVec2{px(80.0F), zeileH})) { ergebnis = 2; }
    ImGui::PopStyleVar();
    dialogSchriftAus();
    if (!offen) { ergebnis = 2; }
    if (ergebnis == 0 && ImGui::IsKeyPressed(ImGuiKey_Escape)) { ergebnis = 2; }
    if (ergebnis == 0 && g_dlg.wahl >= 0 && ImGui::IsKeyPressed(ImGuiKey_Enter) && !ImGui::IsAnyItemActive()) { ergebnis = 1; }
    if (ergebnis != 0) {
        ImGui::CloseCurrentPopup();
        g_dlg.offen = false;
        if (ergebnis == 1 && g_dlg.wahl >= 0 && g_dlg.wahl < static_cast<int>(g_dlg.liste.size())) {
            (void)bspOeffnenSpiel(g_dlg.liste[static_cast<std::size_t>(g_dlg.wahl)].pfad);
        } else if (ergebnis == 3) {
            const std::string pfad = kartenDateiDialog(true, "Open compiled map", "*.bsp|*.bsp", kartenStartOrdner());
            if (!pfad.empty()) { (void)bspOeffnenDatei(pfad); }
        }
    }
    ImGui::EndPopup();
}

// --- Punkt 2: Originallicht -----------------------------------------------------------

namespace {

std::string g_nurEntitaetenBsp;   // die Datei, die q3map2 gerade schreibt (fuer den Abschluss)

}  // namespace

bool bspNurEntitaetenMoeglich() { return e().bspHerkunft.aktiv; }

bool bspGeometrieGeaendert() {
    const Editor& ed = e();
    return !ed.bspHerkunft.aktiv || karte::geometrieFingerabdruck(ed.karte) != ed.bspHerkunft.fingerabdruck;
}

void bspNurEntitaeten() {
    Editor& ed = e();
    if (!ed.bspHerkunft.aktiv) {
        fehler(trK(Str::KeBmNurEntitiesAus));
        return;
    }
    if (bspGeometrieGeaendert()) {
        warnung(std::string(trK(Str::KeBmGeometrie)));
        qtMeldung(trK(Str::KeBmGeometrieTitel), trK(Str::KeBmGeometrie), MeldungArt::Warnung, false, nullptr);
        return;
    }
    const BspHerkunft& h = ed.bspHerkunft;
    const std::string karteDatei = h.ordner + "/" + h.stamm + ".map";
    const std::string bsp = h.ordner + "/" + h.stamm + ".bsp";
    std::string original;
    if (!dateiLesen(h.ordner + "/" + h.stamm + "_original.bsp", original)) {
        fehler("reading " + h.ordner + "/" + h.stamm + "_original.bsp failure");
        return;
    }
    // Die Entitaeten fuer q3map2: <Arbeitsordner>/<Name>.map (die Karte des
    // Editors bleibt, wo sie ist); eine frische Kopie der Original-.bsp
    if (!karte::karteSpeichern(karteDatei, ed.karte, false) || !dateiSchreibenRoh(bsp, original)) {
        fehler("writing " + karteDatei + " failure");
        return;
    }
    karte::BspDatei b;
    const bool lichter = karte::bspLesen(original, b) && karte::bspHatLichter(karte::bspEntitaetenText(b));
    // Stufe 6 (JKHub, Lichtstile): die schaltbaren Lichter muessen dieselben sein wie in der
    // Original-.bsp, in derselben Reihenfolge - ihr Licht steckt je Stil in den Lightmaps, und
    // -onlyents vergibt die Stile nach der Reihenfolge neu (SetLightStyles).
    if (karte::schaltlichter(karte::bspEntitaetenText(b), true) != karte::schaltlichter(karte::karteSchreiben(ed.karte), false)) {
        warnung(trK(Str::KeBmSchaltlichter));
        qtMeldung(trK(Str::KeBmGeometrieTitel), trK(Str::KeBmSchaltlichter), MeldungArt::Warnung, false, nullptr);
        return;
    }
    g_nurEntitaetenBsp = bsp;
    bauEigeneBefehle(karte::ohneMnemonik(trK(Str::KeBmNurEntities)), {karte::onlyentsBefehl(q3map2Aufruf(), karteDatei, lichter)},
                     [](int art) {
                         if (art != 0) { return; }
                         Editor& ed2 = e();
                         std::string orig;
                         std::string neu;
                         const BspHerkunft& h2 = ed2.bspHerkunft;
                         if (!dateiLesen(h2.ordner + "/" + h2.stamm + "_original.bsp", orig) || !dateiLesen(g_nurEntitaetenBsp, neu)) { return; }
                         const std::string fertig = karte::bspEntitaetenEinsetzen(orig, neu);
                         karte::BspDatei a;
                         karte::BspDatei c;
                         if (fertig.empty() || !karte::bspLesen(orig, a) || !karte::bspLesen(fertig, c) || !dateiSchreibenRoh(g_nurEntitaetenBsp, fertig)) {
                             fehler("bspEntitaetenEinsetzen failed: " + g_nurEntitaetenBsp);
                             return;
                         }
                         int gleich = 0;
                         for (std::size_t i = 1; i < a.lumps.size() && i < c.lumps.size(); ++i) { gleich += a.lumps[i] == c.lumps[i] ? 1 : 0; }
                         char z[1024];
                         std::snprintf(z, sizeof(z), trK(Str::KeBmNurEntitiesFertig), g_nurEntitaetenBsp.c_str(), gleich,
                                       static_cast<int>(a.lumps.size()) - 1);
                         ausgabe(z);
                         ed2.bspNurEntitaetenFertig = g_nurEntitaetenBsp;
                     });
}

}  // namespace bhed::gui::karteneditor
