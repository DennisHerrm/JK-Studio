// app_karte_befehle.cpp - Karten-Editor: die nachgebauten NRC-Befehle.
//
// NRC haengt jeden Befehl ueber GlobalCommands_insert / GlobalToggles_insert
// (radiant/commands.cpp) an einen Rueckruf; Menues, Knoepfe und Tasten loesen
// nur den Namen aus. Hier dasselbe: ausfuehren(name) sucht den Rueckruf, und
// was in dieser Stufe noch nicht nachgebaut ist, bleibt sichtbar und
// ausgegraut (befehle::Befehl::stufe nennt die Stufe).
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
#include <map>
#include <utility>

namespace bhed::gui::karteneditor {

namespace {

struct Umsetzung {
    std::function<void()> tun;
    std::function<bool()> an;   // nur Umschalter (Toggles)
};

Editor& e() { return editor(); }

// GroupDialog_showPage (groupdialog.cpp:140-149): dieselbe Seite schaltet
// das Fenster um, eine andere Seite holt es nach vorn.
void gruppenSeite(int seite) {
    Editor& ed = e();
    if (ed.gruppenSeite == seite) {
        ed.gruppenDialogOffen = !ed.gruppenDialogOffen;
    } else {
        ed.gruppenSeite = seite;
        ed.gruppenSeiteWechseln = true;
        ed.gruppenDialogOffen = true;
    }
}

std::map<std::string, Umsetzung>& umsetzungen() {
    static std::map<std::string, Umsetzung> m;
    if (!m.empty()) { return m; }
    // --- File (map.cpp:2429-2433, mainframe.cpp:2073) ---
    m["NewMap"] = {[] { neueKarte(); }, nullptr};
    m["OpenMap"] = {[] { karteOeffnenDialog(); }, nullptr};
    m["SaveMap"] = {[] { (void)karteSpeichern(); }, nullptr};
    m["SaveMapAs"] = {[] { (void)karteSpeichernUnterDialog(); }, nullptr};
    // Exit (mainframe.cpp:2073): beendet das Programm - hier behaved, mit
    // den Rueckfragen des Programms.
    m["Exit"] = {[] {
                     if (confirmQuit()) {
                         g_app->quitConfirmed = true;
                         g_app->wantQuit = true;
                     }
                 },
                 nullptr};
    // --- Fenster (mainframe.cpp:2078-2082, groupdialog.cpp) ---
    m["ToggleEntityInspector"] = {[] { gruppenSeite(0); }, nullptr};
    m["ToggleConsole"] = {[] {
                              // Console_ToggleShow: nur im Gruppendialog eine Seite
                              // (Floating, Split); sonst steht die Konsole fest im
                              // Layout und showPage(nullptr) zeigt nur das Fenster.
                              if (e().anordnung == befehle::Anordnung::Floating ||
                                  e().anordnung == befehle::Anordnung::Split) {
                                  gruppenSeite(1);
                              } else {
                                  e().gruppenDialogOffen = true;
                              }
                          },
                          nullptr};
    m["ToggleTextures"] = {[] {
                               if (e().anordnung == befehle::Anordnung::Floating ||
                                   e().anordnung == befehle::Anordnung::Split || e().gruppenDialogEingebaut) {
                                   gruppenSeite(2);
                               } else {
                                   e().gruppenDialogOffen = true;
                               }
                           },
                           nullptr};
    m["ToggleModelBrowser"] = {[] { gruppenSeite(3); }, nullptr};
    m["ToggleLayersBrowser"] = {[] { gruppenSeite(4); }, nullptr};
    // Floating: ToggleShown der vier Ansichtsfenster (camwindow.cpp:2498,
    // xywindow.cpp:2267-2269).
    m["ToggleCamera"] = {[] { e().schwebendSichtbar[3] = !e().schwebendSichtbar[3]; },
                         [] { return e().schwebendSichtbar[3]; }};
    m["ToggleView"] = {[] { e().schwebendSichtbar[2] = !e().schwebendSichtbar[2]; },
                       [] { return e().schwebendSichtbar[2]; }};
    m["ToggleFrontView"] = {[] { e().schwebendSichtbar[1] = !e().schwebendSichtbar[1]; },
                            [] { return e().schwebendSichtbar[1]; }};
    m["ToggleSideView"] = {[] { e().schwebendSichtbar[0] = !e().schwebendSichtbar[0]; },
                           [] { return e().schwebendSichtbar[0]; }};
    // MaximizeView (mainframe.cpp:1550-1600): das Feld unter der Maus.
    m["MaximizeView"] = {[] { maximierenUmschalten(); }, nullptr};
    // --- Edit ---
    m["Preferences"] = {[] { e().einstellungenOffen = true; }, nullptr};
    m["Shortcuts"] = {[] { e().tastenDialogOffen = true; }, nullptr};   // DoCommandListDlg (commands.cpp:332)
    // --- 2D-Ansichten (xywindow.cpp:2266-2278) ---
    for (const char* n : {"NextView", "ViewTop", "ViewFront", "ViewSide", "ZoomIn", "ZoomOut", "Zoom100", "CenterXYView",
                          "XYFocusOnSelected"}) {
        const std::string name = n;
        m[name] = {[name] { xyBefehl(name); }, nullptr};
    }
    // View > Show (xywindow.cpp:2213-2232, 2096-2177; entity.cpp): Schalter
    const auto schalter = [](const char* name, bool Anzeige::*feld) {
        m[name] = {[feld] { e().anzeige.*feld = !(e().anzeige.*feld); }, [feld] { return e().anzeige.*feld; }};
    };
    schalter("ShowSize2d", &Anzeige::groesse2d);
    schalter("ToggleCrosshairs", &Anzeige::fadenkreuz);
    schalter("ToggleGrid", &Anzeige::raster);
    schalter("ShowAngles", &Anzeige::winkel);
    schalter("ShowNames", &Anzeige::namen);
    schalter("ShowBboxes", &Anzeige::kaesten);
    schalter("ShowConnections", &Anzeige::verbindungen);
    schalter("ShowLightRadiuses", &Anzeige::lichtRadien);
    schalter("ShowBlocks", &Anzeige::bloecke);
    schalter("ShowCoordinates", &Anzeige::koordinaten);
    schalter("ShowWindowOutline", &Anzeige::umriss);
    schalter("ShowAxes", &Anzeige::achsen);
    schalter("ShowWorkzone2d", &Anzeige::workzone2d);
    // --- Kamera (camwindow.cpp:2487-2570, 1422-1432) ---
    for (const char* n : {"CenterView", "CameraFocusOnSelected", "CubicClipZoomIn", "CubicClipZoomOut", "UpFloor", "DownFloor",
                          "CameraModeNext", "CameraModePrev", "CameraSpeedInc", "CameraSpeedDec", "CameraForward", "CameraBack",
                          "CameraLeft", "CameraRight", "CameraStrafeRight", "CameraStrafeLeft", "CameraUp", "CameraDown",
                          "CameraAngleUp", "CameraAngleDown"}) {
        const std::string name = n;
        m[name] = {[name] { kameraBefehl(name); }, nullptr};
    }
    m["ToggleCubicClip"] = {[] { kameraBefehl("ToggleCubicClip"); }, [] { return e().kameraE.kubisch; }};
    m["ShowStats"] = {[] { kameraBefehl("ShowStats"); }, [] { return e().kameraE.zeigeStatistik; }};
    m["ShowWorkzone3d"] = {[] { kameraBefehl("ShowWorkzone3d"); }, [] { return e().kameraE.zeigeWorkzone; }};
    m["ShowSize3d"] = {[] { kameraBefehl("ShowSize3d"); }, [] { return e().kameraE.zeigeGroesse; }};
    // --- Grid (grid.cpp:183-203) ---
    m["GridDown"] = {[] { e().raster.kleiner(); }, nullptr};
    m["GridUp"] = {[] { e().raster.groesser(); }, nullptr};
    m["ToggleGridSnap"] = {[] { e().raster.einrastenUmschalten(); }, nullptr};
    for (int p = karte::Raster::kMinPotenz; p <= karte::Raster::kMaxPotenz; ++p) {
        const std::string name = std::string("SetGrid") + karte::Raster::name(p + 3);
        // Grid-Menue: Radio, der aktuelle Eintrag ist angehakt (gridActions_setChecked).
        m[name] = {[p] { e().raster.setzePotenz(p); }, [p] { return e().raster.potenz() == p; }};
    }
    return m;
}

}  // namespace

std::function<bool(int, befehle::TastenBelegung&, bool&, bool&)> tastaturErsatz;

bool tastenBelegung(int imguiKey, befehle::TastenBelegung& out, bool& ziffernblock, bool& erweitertesReturn) {
    if (tastaturErsatz && tastaturErsatz(imguiKey, out, ziffernblock, erweitertesReturn)) { return true; }
    return tastatur::belegung(imguiKey, out, ziffernblock, erweitertesReturn);
}

std::uint32_t qtZusatztasten() {
    const ImGuiIO& io = ImGui::GetIO();
    std::uint32_t m = 0;
    if (io.KeyShift) { m |= befehle::kShift; }
    if (io.KeyCtrl) { m |= befehle::kCtrl; }
    if (io.KeyAlt) { m |= befehle::kAlt; }
    if (io.KeySuper) { m |= befehle::kMeta; }
    return m;
}

void tastenVerarbeiten() {
    Editor& ed = e();
    ImGuiIO& io = ImGui::GetIO();
    // QAction::autoRepeat wiederholt im Takt von Windows (SPI_GETKEYBOARDDELAY
    // und -SPEED) - solange der Karten-Editor die Tastatur hat.
    static bool wiederholungGemerkt = false;
    static float altVerzoegerung = 0.0F;
    static float altAbstand = 0.0F;
    if (aktiv()) {
        float v = 0.0F;
        float a = 0.0F;
        if (!wiederholungGemerkt && tastatur::wiederholung(v, a)) {
            altVerzoegerung = io.KeyRepeatDelay;
            altAbstand = io.KeyRepeatRate;
            io.KeyRepeatDelay = v;
            io.KeyRepeatRate = a;
            wiederholungGemerkt = true;
        }
    } else if (wiederholungGemerkt) {
        io.KeyRepeatDelay = altVerzoegerung;
        io.KeyRepeatRate = altAbstand;
        wiederholungGemerkt = false;
    }
    if (!aktiv() || !ed.belegungGeladen) {
        kameraTastenLoslassen();   // GlobalPressedKeys_releaseAll (Fokuswechsel)
        return;
    }
    // Tippen in ein Feld, ein offenes Menue/Klappfenster und das Aufnehmen
    // einer neuen Taste haben Vorrang.
    if (io.WantTextInput || !ed.tastenDialogBearbeitet.empty() || ed.tasteVerbrauchtBild == ImGui::GetFrameCount() ||
        ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel)) {
        kameraTastenLoslassen();
        return;
    }
    // PressedKeys (accelerator.cpp:135-163): ein Ereignisfilter am Hauptfenster
    // vor allen Kuerzeln - auch bei gedrueckter Maustaste (Freelook + WASD).
    std::vector<bool> verbraucht(static_cast<std::size_t>(ImGuiKey_NamedKey_END), false);
    for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k) {
        const auto key = static_cast<ImGuiKey>(k);
        if ((key >= ImGuiKey_LeftCtrl && key <= ImGuiKey_RightSuper) || key >= ImGuiKey_ReservedForModCtrl) { continue; }
        if (kameraTasteVerbraucht(k)) { verbraucht[static_cast<std::size_t>(k)] = true; }
    }
    // RadiantQMainWindow::event: "block shortcuts while mouse buttons are pressed"
    for (int m = 0; m < 5; ++m) {
        if (io.MouseDown[m]) { return; }
    }
    for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k) {
        const auto key = static_cast<ImGuiKey>(k);
        if (key >= ImGuiKey_LeftCtrl && key <= ImGuiKey_RightSuper) { continue; }   // nur Zusatztasten
        if (key >= ImGuiKey_ReservedForModCtrl) { continue; }
        if (verbraucht[static_cast<std::size_t>(k)]) { continue; }   // ShortcutOverride angenommen
        // QAction-Kuerzel loesen auch bei der Tastenwiederholung aus.
        if (!ImGui::IsKeyPressed(key, true)) { continue; }
        befehle::TastenBelegung kb;
        bool ziffernblock = false;
        bool erweitert = false;
        if (!tastenBelegung(k, kb, ziffernblock, erweitert)) { continue; }
        std::uint32_t mods = qtZusatztasten();
        if (ziffernblock) { mods |= befehle::kNum; }
        // Nur Kuerzel, die gerade an ihrer Taste haengen (command_connect_accelerator).
        std::vector<std::string> t = ed.belegung.treffer(befehle::moeglicheTasten(kb, mods, erweitert));
        t.erase(std::remove_if(t.begin(), t.end(), [](const std::string& n) { return !befehlVerbunden(n); }), t.end());
        if (t.size() == 1) {
            ed.statusBefehl = t[0];
            (void)ausfuehren(t[0]);
        } else if (t.size() > 1) {
            // QAction::event: "Ambiguous shortcut overload" - nichts wird ausgeloest.
            warnung("QAction::event: Ambiguous shortcut overload: " +
                    befehle::tasteAlsText(befehle::moeglicheTasten(kb, mods, erweitert).front()));
        }
    }
}

bool ausfuehren(const std::string& name) {
    auto& m = umsetzungen();
    const auto it = m.find(name);
    if (it == m.end()) { return false; }
    e().letzterBefehl = name;
    it->second.tun();
    return true;
}

bool nachgebaut(const std::string& name) { return umsetzungen().count(name) != 0U; }

bool angehakt(const std::string& name) {
    auto& m = umsetzungen();
    const auto it = m.find(name);
    return it != m.end() && it->second.an && it->second.an();
}

std::vector<std::string> nachgebauteBefehle() {
    std::vector<std::string> v;
    for (const auto& [n, u] : umsetzungen()) {
        (void)u;
        v.push_back(n);
    }
    return v;
}

}  // namespace bhed::gui::karteneditor
