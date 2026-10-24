// app_karte_datei.cpp - Karten-Editor: Auswahl und Datei (Stufe 2, Punkt 8):
// Import, Save selected, Save region, Select inside/touching, Expand
// Selection To Primitives, das Menue Region, Find brush, Map Info und
// Autosave.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/map.cpp:1855-1935
// (ImportMap, ExportMap, SaveRegion, RegionOff, RegionXY mit NDIM1NDIM2,
// RegionBrush, RegionSelected), 2085-2131 (DoFind: QDialog "Find Brush" mit
// QFormLayout SetFixedSize, zwei SpinBox 0..999999 ohne Knoepfe, SpinBoxLabel
// "Entity number"/"Brush number", QDialogButtonBox Close + "Find"), 906-978
// (DoMapInfo: QDialog "Map Info" mit QGridLayout, QTreeWidget "Entity" /
// "Count", sortiert, ResizeToContents, AdjustToContents), 2429-2445 (die
// Befehle), radiant/select.cpp:1877-1879, radiant/autosave.cpp:52-158
// (Map_Snapshot, QE_CheckAutoSave), radiant/mainframe.cpp:727-746
// (EverySecondTimer: jede Sekunde, wenn keine Maustaste gedrueckt ist).
//
// Masse an den Fotos dlg_Misc_Find_brush_0.png und dlg_Misc_Map_Info_0.png
// (doppelte Groesse):
//   Find Brush: Innenraum 263 x 100; Zeilen 22 hoch bei 11 und 39 (Abstand 6),
//     die Felder (SpinBox ohne Knoepfe) 166 x 20 darin mittig; die Felder
//     beginnen hinter der breiteren Beschriftung + 6; Knoepfe 80 x 22 bei 67,
//     "Find" (Standardknopf) vor "Close", rechtsbuendig.
//   Map Info: Zeilen 14 hoch bei 11, 31, 51, die Zeile "*** Entity breakdown
//     ***" bei 71 mittig, der Baum bei 91 ueber alle vier Spalten; Kopf 20,
//     Zeilen 16 (Segoe UI 9 pt), Rahmen 1; Spalten nach Inhalt (Text + 6,
//     der Kopf Text + 4 + 20 + 4 mit Sortierpfeil); Werte fett kursiv.
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

#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/teiler.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace bhed::gui::karteneditor {

namespace {

Editor& e() { return editor(); }

float g_sk = 1.0F;
ImVec2 g_innen{0.0F, 0.0F};

ImVec2 bp(float x, float y) { return ImVec2{std::round(g_innen.x + x * g_sk), std::round(g_innen.y + y * g_sk)}; }
float px(float l) { return std::round(l * g_sk); }

// QLabel::sizeHint in Tahoma 8 pt (GDI wie Qt)
float textBreite(const std::string& text, bool fett = false) {
    const int b = platform::gdiTextBreite(text, L"Tahoma", 11, fett);
    return b >= 0 ? static_cast<float>(b) : std::round(ImGui::CalcTextSize(text.c_str()).x / g_sk);
}
float segoeBreite(const std::string& text) {
    const int b = platform::gdiTextBreite(text, L"Segoe UI", 12, false);
    return b >= 0 ? static_cast<float>(b) : std::round(ImGui::CalcTextSize(text.c_str()).x / g_sk);
}

// "<b><i>...</i></b>": Tahoma fett, kursiv von GDI geschert (Tahoma hat
// keinen kursiven Schnitt) - die Punkte der Zeichen um die Grundlinie kippen.
void fettKursiv(ImDrawList* dl, ImVec2 p, ImU32 farbe, const std::string& text) {
    const int von = dl->VtxBuffer.Size;
    dl->AddText(p, farbe, text.c_str());
    const float grund = p.y + ImGui::GetFontSize() * 0.8F;
    for (int i = von; i < dl->VtxBuffer.Size; ++i) { dl->VtxBuffer[i].pos.x += (grund - dl->VtxBuffer[i].pos.y) * 0.2F; }
}

// Eine Zahl wie QSpinBox ohne Knoepfe mit SpinBoxLabel davor (spinbox.h):
// die Beschriftung waagerecht ziehen (20 Punkte je Schritt, Strg x 10),
// Pfeiltasten, Bild hoch/runter (x 10) und das Mausrad.
struct ZiehZustand {
    ImVec2 start;
    float summe = 0.0F;
};

void zahlFeld(const char* id, int& wert, int minimum, int maximum, float labelX, float labelB, const char* beschriftung, float feldX, float zeileY,
              float feldB, ZiehZustand& zug, bool fokus) {
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float zeileH = px(22.0F);
    dl->AddText(ImVec2{bp(labelX, 0.0F).x, std::floor(bp(0.0F, zeileY).y + (zeileH - ImGui::GetTextLineHeight()) * 0.5F)},
                ImGui::GetColorU32(ImGuiCol_Text), beschriftung);
    const auto schritt = [&](int n) { wert = std::clamp(wert + n, minimum, maximum); };   // QAbstractSpinBox::stepBy
    static const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
    ImGui::SetCursorScreenPos(bp(labelX, zeileY));
    ImGui::InvisibleButton((std::string("##ziehen") + (id + 2)).c_str(), ImVec2{px(labelB), zeileH});
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
        ImGui::SetMouseCursor(ImGui::IsItemActive() ? ImGuiMouseCursor_None : ImGuiMouseCursor_ResizeEW);
    }
    if (ImGui::IsItemActivated()) {
        zug.start = io.MousePos;
        zug.summe = 0.0F;
    } else if (ImGui::IsItemActive() && (io.MouseDelta.x != 0.0F || io.MouseDelta.y != 0.0F)) {
        const float qt = std::max(1.0F, std::round(g_app->dpiScale));
        zug.summe += (io.MousePos.x - zug.start.x) / qt;   // m_dragAccum += pos.x - m_dragStart.x
        const int delta = static_cast<int>(zug.summe / 20.0F);
        if (delta != 0) {
            zug.summe -= static_cast<float>(delta) * 20.0F;
            schritt(io.KeyCtrl ? delta * 10 : delta);
            if (!imTest) { ImGui::TeleportMousePos(zug.start); }
        }
    }
    const float feldH = px(20.0F);
    const ImVec2 fa = bp(feldX, zeileY + 1.0F);
    ImGui::SetCursorScreenPos(fa);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (feldH - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::SetNextItemWidth(px(feldB));
    if (fokus) { ImGui::SetKeyboardFocusHere(); }
    int neu = wert;
    if (ImGui::InputInt(id, &neu, 0, 0)) { wert = std::clamp(neu, minimum, maximum); }
    ImGui::PopStyleVar();
    if (ImGui::IsItemActive()) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { schritt(1); }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { schritt(-1); }
        if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) { schritt(10); }
        if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) { schritt(-10); }
    }
    if (ImGui::IsMouseHoveringRect(fa, ImVec2{fa.x + px(feldB), fa.y + feldH}) && io.MouseWheel != 0.0F) {
        const int n = static_cast<int>(io.MouseWheel);
        schritt(io.KeyCtrl ? n * 10 : n);
    }
}

// MapResource_saveFile (referencecache.cpp:98-111) / Map_SaveFile
bool dateiSchreiben(const std::string& pfad, const std::string& text) { return karteDateiSchreiben(pfad, text); }

// Die Szene hat sich in ihrer Sichtbarkeit geaendert (SceneChangeNotify).
void sichtGeaendert() {
    ++e().sichtStand;
    auswahlGeaendert();
}

// SelectBrush: XYWnd::SetOrigin fuer jede 2D-Ansicht
void ansichtenAuf(const karte::Vektor3f& mitte) {
    for (auto& x : e().xy) { x.ansicht.setzeUrsprung(mitte); }
}

}  // namespace

// --- Datei --------------------------------------------------------------------------------

bool karteImportierenAus(const std::string& pfad) {   // ImportMap -> Map_ImportFile
    std::string text;
    if (!karteDateiLesen(pfad, text)) { return false; }   // tryDecompile (q3map2 -convert) ist Stufe 3
    karte::karteImportieren(e().karte, text);
    auswahlGeaendert();
    return true;
}

bool auswahlSpeichernUnter(const std::string& pfad) {   // ExportMap -> Map_SaveSelected
    return dateiSchreiben(pfad, karte::kopieren(e().karte));
}

bool regionSpeichernUnter(const std::string& pfad) {   // SaveRegion -> Map_SaveRegion
    Editor& ed = e();
    std::string w;
    const std::string shader = karte::regionKastenShader(ed.regionKastenShader, &w);
    if (!w.empty()) { warnung(w); }
    std::vector<std::string> meldungen;
    const std::string text = karte::regionText(ed.karte, ed.region, ed.kamera.ursprung, ed.kamera.winkel[1], shader, &meldungen);
    return dateiSchreiben(pfad, text);
}

bool dateiBefehlAn(const std::string& name) {
    if (name == "RegionSetSelection") { return e().region.aktiv; }   // g_region_item (BoolExportCaller( g_region_active ))
    return false;
}

void dateiBefehl(const std::string& name) {
    Editor& ed = e();
    karte::Karte& k = ed.karte;
    const float maxRaster = ed.raster.maxKoord();
    if (name == "ImportMap") {   // map_import( "Import Map" )
        const std::string pfad = platform::openFileDialog("Import Map", kartenDateiFilter(), kartenStartOrdner());
        if (!pfad.empty()) { (void)karteImportierenAus(pfad); }
    } else if (name == "SaveSelected") {   // map_save( "Export Selection" )
        const std::string pfad = platform::saveFileDialog("Export Selection", kartenDateiFilter(), kartenStartOrdner(), "");
        if (!pfad.empty()) { (void)auswahlSpeichernUnter(pfad); }
    } else if (name == "SaveRegion") {   // map_save( "Export Region" )
        const std::string pfad = platform::saveFileDialog("Export Region", kartenDateiFilter(), kartenStartOrdner(), "");
        if (!pfad.empty()) { (void)regionSpeichernUnter(pfad); }
    } else if (name == "SelectInside") {   // Select_Inside
        karte::auswahlInnen(k, ed.bearbeitung.einstellung);
        auswahlGeaendert();
    } else if (name == "SelectTouching") {   // Select_Touching
        karte::auswahlBeruehrend(k, ed.bearbeitung.einstellung);
        auswahlGeaendert();
    } else if (name == "ExpandSelectionToPrimitives") {   // Scene_ExpandSelectionToPrimitives
        karte::auswahlAufPrimitiveErweitern(k);
        auswahlGeaendert();
    } else if (name == "RegionOff") {   // RegionOff
        karte::regionAus(k, ed.region, maxRaster);
        sichtGeaendert();
    } else if (name == "RegionSetXY") {   // RegionXY: die aktive 2D-Ansicht
        const karte::XyAnsicht& v = ed.xy[static_cast<std::size_t>(ed.aktivXy)].ansicht;
        const int nDim = static_cast<int>(v.typ);
        const int nDim1 = nDim == 0 ? 1 : 0;   // NDIM1NDIM2
        const int nDim2 = nDim == 2 ? 1 : 2;
        karte::Vektor3f mn;
        karte::Vektor3f mx;
        mn[static_cast<std::size_t>(nDim1)] = v.ursprung[static_cast<std::size_t>(nDim1)] - 0.5F * static_cast<float>(v.breite) / v.massstab;
        mn[static_cast<std::size_t>(nDim2)] = v.ursprung[static_cast<std::size_t>(nDim2)] - 0.5F * static_cast<float>(v.hoehe) / v.massstab;
        mn[static_cast<std::size_t>(nDim)] = karte::kMinWeltKoord + 64;
        mx[static_cast<std::size_t>(nDim1)] = v.ursprung[static_cast<std::size_t>(nDim1)] + 0.5F * static_cast<float>(v.breite) / v.massstab;
        mx[static_cast<std::size_t>(nDim2)] = v.ursprung[static_cast<std::size_t>(nDim2)] + 0.5F * static_cast<float>(v.hoehe) / v.massstab;
        mx[static_cast<std::size_t>(nDim)] = karte::kMaxWeltKoord - 64;
        karte::regionXY(k, ed.region, mn, mx);
        sichtGeaendert();
    } else if (name == "RegionSetBrush") {   // RegionBrush
        if (!karte::regionPinsel(k, ed.region, maxRaster, ed.bearbeitung.einstellung)) { fehler("Nothing is selected!"); }
        sichtGeaendert();
    } else if (name == "RegionSetSelection") {   // RegionSelected
        karte::regionAuswahl(k, ed.region, maxRaster);
        sichtGeaendert();
    } else if (name == "FindBrush") {   // DoFind
        Editor::PinselSuche& d = ed.pinselSuche;
        d = Editor::PinselSuche{};
        karte::auswahlNummern(k, d.entitaet, d.pinsel);   // GetSelectionIndex
        d.offen = true;
        d.oeffnen = true;
    } else if (name == "MapInfo") {   // DoMapInfo
        Editor::KartenInfoDialog& d = ed.kartenInfo;
        d = Editor::KartenInfoDialog{};
        d.info = karte::karteInfo(k);
        d.offen = true;
        d.oeffnen = true;
    }
}

// --- Find Brush ---------------------------------------------------------------------------

void zeichnePinselSuche() {
    Editor& ed = e();
    Editor::PinselSuche& d = ed.pinselSuche;
    if (!d.offen) { return; }
    const std::string id = std::string(trK(Str::KeFindBrushTitle)) + "###kartepinselsuche";
    if (d.oeffnen) {
        d.oeffnen = false;
        ImGui::OpenPopup(id.c_str());
    }
    g_sk = massstab();
    dialogSchriftAn();
    const char* ent = trK(Str::KeFindBrushEntity);
    const char* pin = trK(Str::KeFindBrushBrush);
    const float labelB = std::max(textBreite(ent), textBreite(pin));
    const float x1 = 11.0F + labelB + 6.0F;
    const float innenB = x1 + 166.0F + 11.0F;
    const float innenH = 67.0F + 22.0F + 11.0F;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    ImGui::SetNextWindowSize(ImVec2{px(innenB), px(innenH) + titelH}, ImGuiCond_Always);
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    bool fertig = false;
    bool finden = false;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    bool offen = true;
    const bool sichtbar = ImGui::BeginPopupModal(id.c_str(), &offen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    if (sichtbar) {
        g_innen = ImGui::GetCursorScreenPos();
        static ZiehZustand zug[2];
        const bool erst = ImGui::IsWindowAppearing();   // der erste Fokus: das erste Feld
        zahlFeld("##suchentitaet", d.entitaet, 0, 999999, 11.0F, textBreite(ent), ent, x1, 11.0F, 166.0F, zug[0], erst);
        const bool enter1 = ImGui::IsItemDeactivated() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter));
        zahlFeld("##suchpinsel", d.pinsel, 0, 999999, 11.0F, textBreite(pin), pin, x1, 39.0F, 166.0F, zug[1], false);
        const bool enter2 = ImGui::IsItemDeactivated() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter));
        // QDialogButtonBox: "Find" (AcceptRole, Standardknopf) vor "Close"
        const float knopfH = px(22.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (knopfH - ImGui::GetTextLineHeight()) * 0.5F)});
        const ImVec2 kg{px(80.0F), knopfH};
        ImGui::SetCursorScreenPos(bp(innenB - 11.0F - 80.0F - 6.0F - 80.0F, 67.0F));
        if (ImGui::Button((std::string(trK(Str::KeFindBrushFind)) + "##suchefinden").c_str(), kg) || enter1 || enter2 ||
            (!ImGui::IsAnyItemActive() && (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)))) {
            fertig = true;
            finden = true;
            ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
        }
        ImGui::SetCursorScreenPos(bp(innenB - 11.0F - 80.0F, 67.0F));
        if (ImGui::Button((std::string(trK(Str::KeFrClose)) + "##sucheschliessen").c_str(), kg) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            fertig = true;
            ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
        }
        ImGui::PopStyleVar();
        ImGui::SetCursorScreenPos(bp(0.0F, innenH));
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
        if (fertig) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    dialogSchriftAus();
    if (!offen) { fertig = true; }
    if (!fertig) { return; }
    d.offen = false;
    if (finden) {   // SelectBrush( entity->value(), brush->value() )
        karte::Vektor3f mitte;
        if (karte::pinselFinden(ed.karte, d.entitaet, d.pinsel, &mitte)) {
            ansichtenAuf(mitte);
            auswahlGeaendert();
        }
    }
}

// --- Map Info -----------------------------------------------------------------------------

void zeichneKartenInfo() {
    Editor& ed = e();
    Editor::KartenInfoDialog& d = ed.kartenInfo;
    if (!d.offen) { return; }
    const std::string id = std::string(trK(Str::KeMapInfoTitle)) + "###kartekarteninfo";
    if (d.oeffnen) {
        d.oeffnen = false;
        ImGui::OpenPopup(id.c_str());
    }
    g_sk = massstab();
    dialogSchriftAn();
    const karte::KartenInfo& info = d.info;
    // QGridLayout: Spalte 0 und 2 die Beschriftungen, 1 und 3 die Werte (fett kursiv)
    const std::string beschriftung[6] = {trK(Str::KeMapInfoBrushes), trK(Str::KeMapInfoPatches), trK(Str::KeMapInfoEntities),
                                         trK(Str::KeMapInfoIngame), trK(Str::KeMapInfoGroups), trK(Str::KeMapInfoIngameGroups)};
    const std::string werte[6] = {std::to_string(info.pinsel), std::to_string(info.patches), std::to_string(info.entitaeten),
                                  std::to_string(info.imSpiel), std::to_string(info.gruppen), std::to_string(info.gruppenImSpiel)};
    float spalte[4] = {0, 0, 0, 0};
    for (int i = 0; i < 6; ++i) {
        const int sp = i < 3 ? 0 : 2;
        spalte[sp] = std::max(spalte[sp], textBreite(beschriftung[i]));
        spalte[sp + 1] = std::max(spalte[sp + 1], textBreite(werte[i], true) + 1.0F);   // kursiv: ein Punkt Ueberhang
    }
    // Der Baum (AdjustToContents): Kopf mit Sortierpfeil, Zeilen nach Inhalt
    std::vector<std::pair<std::string, std::size_t>> zeilen(info.aufschluesselung.begin(), info.aufschluesselung.end());
    std::stable_sort(zeilen.begin(), zeilen.end(), [&d](const auto& a, const auto& b) {
        if (d.sortSpalte == 1) { return d.absteigend ? b.second < a.second : a.second < b.second; }
        return d.absteigend ? b.first < a.first : a.first < b.first;
    });
    const char* koepfe[2] = {trK(Str::KeMapInfoColEntity), trK(Str::KeMapInfoColCount)};
    float baumSp[2] = {textBreite(koepfe[0]) + 4.0F + 24.0F, textBreite(koepfe[1]) + 4.0F + 24.0F};   // + size.height() + margin
    for (const auto& [n, z] : zeilen) {
        baumSp[0] = std::max(baumSp[0], segoeBreite(n) + 6.0F);
        baumSp[1] = std::max(baumSp[1], segoeBreite(std::to_string(z)) + 6.0F);
    }
    const float zeilenH = 16.0F;
    const float baumHinweisB = baumSp[0] + baumSp[1] + 2.0F;
    const float baumH = std::min(2.0F + 20.0F + zeilenH * static_cast<float>(zeilen.size()), 24.0F * 16.0F);   // boundedTo( 36h x 24h )
    const std::string ueber = trK(Str::KeMapInfoBreakdown);
    // distributeMultiCell: was ueber alle Spalten geht, verteilt qGeomCalc
    std::vector<karte::LayoutFeld> kette(4);
    for (int i = 0; i < 4; ++i) {
        kette[static_cast<std::size_t>(i)].hinweis = static_cast<int>(spalte[i]);
        kette[static_cast<std::size_t>(i)].leer = false;
    }
    float summe = spalte[0] + spalte[1] + spalte[2] + spalte[3] + 18.0F;
    const float breit = std::max({summe, baumHinweisB, textBreite(ueber)});
    if (breit > summe) {
        karte::geomCalc(kette, 0, 4, 0, static_cast<int>(breit), 6);
        for (int i = 0; i < 4; ++i) { spalte[i] = std::max(spalte[i], static_cast<float>(kette[static_cast<std::size_t>(i)].groesse)); }
        summe = spalte[0] + spalte[1] + spalte[2] + spalte[3] + 18.0F;
    }
    const float innenB = 11.0F + std::max(summe, breit) + 11.0F;
    const float innenH = 91.0F + baumH + 11.0F;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    ImGui::SetNextWindowSize(ImVec2{px(innenB), px(innenH) + titelH}, ImGuiCond_Always);
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    bool offen = true;
    const bool sichtbar = ImGui::BeginPopupModal(id.c_str(), &offen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    bool fertig = !offen;
    if (sichtbar) {
        g_innen = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImU32 textFarbe = ImGui::GetColorU32(ImGuiCol_Text);
        const float zeileH = px(14.0F);
        const float x[4] = {11.0F, 11.0F + spalte[0] + 6.0F, 11.0F + spalte[0] + spalte[1] + 12.0F, 11.0F + spalte[0] + spalte[1] + spalte[2] + 18.0F};
        for (int i = 0; i < 6; ++i) {
            const int sp = i < 3 ? 0 : 2;
            const float y = 11.0F + 20.0F * static_cast<float>(i % 3);
            const float ty = std::floor(bp(0.0F, y).y + (zeileH - ImGui::GetTextLineHeight()) * 0.5F);
            dl->AddText(ImVec2{bp(x[sp], 0.0F).x, ty}, textFarbe, beschriftung[i].c_str());
            dialogSchriftAus();
            dialogSchriftAn(true);
            fettKursiv(dl, ImVec2{bp(x[sp + 1], 0.0F).x, ty}, textFarbe, werte[i]);
            dialogSchriftAus();
            dialogSchriftAn();
        }
        {   // "*** Entity breakdown ***" mittig (AlignCenter ueber vier Spalten)
            const float b = ImGui::CalcTextSize(ueber.c_str()).x;
            const float mitte = (bp(11.0F, 0.0F).x + bp(innenB - 11.0F, 0.0F).x) * 0.5F;
            dl->AddText(ImVec2{std::floor(mitte - b * 0.5F), std::floor(bp(0.0F, 71.0F).y + (zeileH - ImGui::GetTextLineHeight()) * 0.5F)}, textFarbe,
                        ueber.c_str());
        }
        // Der Baum: Rahmen, Kopf (Tahoma), Zeilen (Segoe UI 9 pt)
        const float bx0 = 11.0F;
        const float bx1 = innenB - 11.0F;
        dl->AddRect(bp(bx0, 91.0F), bp(bx1, 91.0F + baumH), ImGui::GetColorU32(ImGuiCol_Border));
        float kx = bx0 + 1.0F;
        for (int sp = 0; sp < 2; ++sp) {
            const float kb = std::min(baumSp[sp], bx1 - 1.0F - kx);
            if (kb <= 0.0F) { break; }
            const ImVec2 k0 = bp(kx, 92.0F);
            const ImVec2 k1 = bp(kx + kb, 112.0F);
            ImGui::SetCursorScreenPos(k0);
            if (ImGui::InvisibleButton((std::string("##infokopf") + std::to_string(sp)).c_str(), ImVec2{k1.x - k0.x, k1.y - k0.y})) {
                if (d.sortSpalte == sp) {   // flipSortIndicator
                    d.absteigend = !d.absteigend;
                } else {
                    d.sortSpalte = sp;
                    d.absteigend = false;
                }
            }
            dl->AddRectFilled(k0, k1, ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_HeaderHovered : ImGuiCol_TableHeaderBg));
            dl->AddLine(ImVec2{k1.x, k0.y}, ImVec2{k1.x, k1.y}, ImGui::GetColorU32(ImGuiCol_Border));
            dl->AddText(ImVec2{k0.x + px(2.0F), std::floor(k0.y + (k1.y - k0.y - ImGui::GetTextLineHeight()) * 0.5F)}, textFarbe, koepfe[sp]);
            if (sp == d.sortSpalte) {   // PE_IndicatorHeaderArrow
                const float ax = k1.x - px(13.0F);
                const float ay = std::floor((k0.y + k1.y) * 0.5F);
                if (d.absteigend) {
                    dl->AddTriangleFilled(ImVec2{ax, ay - px(2.0F)}, ImVec2{ax + px(8.0F), ay - px(2.0F)}, ImVec2{ax + px(4.0F), ay + px(2.0F)}, textFarbe);
                } else {
                    dl->AddTriangleFilled(ImVec2{ax + px(4.0F), ay - px(2.0F)}, ImVec2{ax + px(8.0F), ay + px(2.0F)}, ImVec2{ax, ay + px(2.0F)}, textFarbe);
                }
            }
            kx += kb;
        }
        dialogSchriftAus();
        ImGui::SetCursorScreenPos(bp(bx0 + 1.0F, 112.0F));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, px(7.0F));
        ImGui::BeginChild("##infozeilen", ImVec2{px(bx1 - bx0 - 2.0F), px(baumH - 22.0F)}, ImGuiChildFlags_None);
        ImDrawList* zl = ImGui::GetWindowDrawList();
        const float zh = px(zeilenH);
        for (std::size_t i = 0; i < zeilen.size(); ++i) {
            const ImVec2 zp = ImGui::GetCursorScreenPos();
            const bool sel = d.gewaehlt == zeilen[i].first;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.0F, 0.0F, 0.0F, 0.0F});
            if (ImGui::Selectable((zeilen[i].first + "##iz").c_str(), sel, ImGuiSelectableFlags_None, ImVec2{0.0F, zh})) { d.gewaehlt = zeilen[i].first; }
            ImGui::PopStyleColor();
            if (!ImGui::IsItemVisible()) { continue; }
            // das aktuelle Element (currentIndex) mit Fokusrahmen: anfangs die erste Zeile
            if ((d.gewaehlt.empty() && i == 0) || sel) {
                zl->AddRect(zp, ImVec2{zp.x + px(baumSp[0]), zp.y + zh}, ImGui::GetColorU32(ImGuiCol_NavHighlight));
            }
            const float ty = std::floor(zp.y + (zh - ImGui::GetTextLineHeight()) * 0.5F);
            zl->AddText(ImVec2{zp.x + px(3.0F), ty}, textFarbe, zeilen[i].first.c_str());
            zl->AddText(ImVec2{zp.x + px(baumSp[0] + 3.0F), ty}, textFarbe, std::to_string(zeilen[i].second).c_str());
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(3);
        dialogSchriftAn();
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {   // QDialog::reject
            fertig = true;
            ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
        }
        ImGui::SetCursorScreenPos(bp(0.0F, innenH));
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
        if (fertig) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    dialogSchriftAus();
    if (fertig) { d.offen = false; }
}

// --- Autosave -----------------------------------------------------------------------------

void autosaveJedeSekunde() {   // EverySecondTimer -> QE_CheckAutoSave
    Editor& ed = e();
    if (!ed.gezeichnet) { return; }   // Map_Valid: der Editor hat eine Karte
    const double jetzt = ImGui::GetTime();
    if (jetzt - ed.autosaveTakt < 1.0) { return; }
    ed.autosaveTakt = jetzt;
    const ImGuiIO& io = ImGui::GetIO();
    for (const bool b : io.MouseDown) {
        if (b) { return; }   // QGuiApplication::mouseButtons() == NoButton
    }
    const bool unbenannt = ed.name == "unnamed.map";
    const auto aktion = ed.autosave.pruefen(std::time(nullptr) + ed.autosaveVersatz, ed.karte.undo.stand(), unbenannt);
    using A = karte::AutoSpeichern::Aktion;
    if (aktion == A::Nichts) { return; }
    if (aktion == A::Uebersprungen) {
        ausgabe("Autosave skipped...");
        return;
    }
    ausgabe(aktion == A::Schnappschuss ? "Autosaving snapshot..." : "Autosaving...");
    const std::string text = karte::karteSchreiben(ed.karte);
    if (aktion == A::Schnappschuss) {   // Map_Snapshot
        const std::string ordner = karte::schnappschussOrdner(ed.name);
        std::error_code ec;
        const std::filesystem::path p(std::u8string(reinterpret_cast<const char8_t*>(ordner.data()), ordner.size()));
        if (std::filesystem::exists(p, ec) || std::filesystem::create_directory(p, ec)) {
            std::uintmax_t summe = 0;
            const std::string pfad = karte::schnappschussPfad(ed.name, &summe);
            (void)dateiSchreiben(pfad, text);
            if (summe > 50U * 1024U * 1024U) {   // total size of saves > 50 mb
                konsoleSchreiben(KonsolenZeile::Art::Ausgabe,
                                 "The snapshot files in " + ordner + " total more than 50 megabytes. You might consider cleaning up.");
            }
        } else {
            qtMeldung("NetRadiant", "Snapshot save failed.. unabled to create directory\n" + ordner, MeldungArt::Info, false, nullptr);
        }
    } else if (unbenannt) {   // g_qeglobals.m_userGamePath + "maps/autosave.map"
        const std::string ordner = kartenBenutzerOrdner();
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(ordner.data()), ordner.size())), ec);
        (void)dateiSchreiben(ordner + "autosave.map", text);
    } else {
        (void)dateiSchreiben(karte::autosavePfad(ed.name), text);
    }
}

}  // namespace bhed::gui::karteneditor
