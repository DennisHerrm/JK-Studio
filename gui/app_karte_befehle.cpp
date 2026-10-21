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
    // MaximizeView (mainframe.cpp:1568-1625): das Feld unter der Maus.
    m["MaximizeView"] = {[] { e().maximiert = !e().maximiert; }, nullptr};
    // --- Edit ---
    m["Preferences"] = {[] { e().einstellungenOffen = true; }, nullptr};
    return m;
}

}  // namespace

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
