// app_karte_kleineplugins.cpp - Karten-Editor, Stufe 4 Punkt 3: die
// Oberflaeche von ShaderPlug, SunPlug und UFO:AI (die Logik:
// src/karte/kleineplugins.cpp).
//
// Portiert aus NetRadiant-Custom 20260114:
//   contrib/shaderplug/shaderplug.cpp:71-192 (dispatch, CreateTagFile: die
//   Meldungen und Kaesten),
//   contrib/sunplug/sunplug.cpp:72-278 (getCommandList "About...;ET-MapCoordinator",
//   about_plugin_window, MapCoordinator: der Dialog "ET-MapCoordinator" als
//   QFormLayout mit "Get optimal mapcoords", Trennlinie, MapCoordsMins /
//   MapCoordsMaxs je zwei SpinBox mit "X: " / "Y: ", Set / Cancel),
//   contrib/ufoaiplug/ufoai.cpp:66-130 (dispatch, Kasten "Note").
//
// Abweichungen: ShaderPlug laedt die Tag-Datei nach dem Schreiben gleich neu
// (NRC: "Please restart Radiant now."); der Kasten sagt das. Die Version im
// About von UFO:AI ist die des Programms.
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
#include "bhed/fassung.h"
#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/rueckgaengig.h"
#include "bhed/karte/texturtags.h"
#include "karte_qtdialog.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>

namespace bhed::gui::karteneditor {

namespace {

namespace kp = karte::plugins;
using i18n::Str;

Editor& e() { return editor(); }

void zeilenAusgeben(const std::vector<std::string>& zeilen, bool warnungen = false) {
    for (const std::string& z : zeilen) {
        if (warnungen && (z.find("Warning") != std::string::npos || z.find("Could not find") != std::string::npos)) {
            warnung(z);
        } else {
            ausgabe(z);
        }
    }
}

// --- ShaderPlug: CreateTagFile -------------------------------------------------------

void tagDateiSchreiben(const kp::ShaderplugListe& l) {
    const std::string datei = tagDatei();
    karte::TexturTags tags;
    kp::shaderplugTags(l, tags);
    if (!tags.speichern(datei)) {
        fehler("Shaderplug: " + datei);
        return;
    }
    tagsNeuLaden();   // Abweichung: statt "Please restart Radiant now."
    char z[1024];
    std::snprintf(z, sizeof(z), trK(Str::KeSpGespeichert), datei.c_str());
    qtMeldung("INFO", z, MeldungArt::Info, false, nullptr);
}

void tagDateiAnlegen() {
    Editor& ed = e();
    std::vector<std::string> namen;   // GlobalShaderSystem().foreachShaderName
    std::vector<std::string> dateien;
    if (ed.karteSpiel) {
        for (const karte::ShaderEintrag* s : ed.karteSpiel->inventar.shader()) { namen.push_back(s->name); }
        for (const auto& [n, s] : ed.aktiveShader.alle()) {
            if (s.standard && ed.karteSpiel->inventar.shaderFinden(n) == nullptr) { namen.push_back(n); }
        }
    }
    kp::ShaderplugListe l = kp::shaderplugSammeln(namen, {});
    ausgabe("Shaderplug: " + std::to_string(l.shader.size()) + " shaders found.");
    // shader_type "quake3" (Jedi Academy / Jedi Outcast): GetTextures( jpg, tga, png )
    if (ed.karteSpiel) {
        for (const char* endung : {"jpg", "tga", "png"}) {
            for (const std::string& d : ed.karteSpiel->fs.dateien("textures/", endung, 1000)) {
                dateien.push_back(d.rfind("textures/", 0) == 0 ? d.substr(9) : d);
            }
        }
    }
    l = kp::shaderplugSammeln(namen, dateien);
    ausgabe("Shaderplug: " + std::to_string(l.texturen.size()) + " textures found.");
    if (l.shader.empty() && l.texturen.empty()) {
        qtMeldung("ERROR", "No shaders or textures found. No XML tag file created!\n", MeldungArt::Fehler, false, nullptr);
        return;
    }
    ausgabe("Shaderplug: Creating XML tag file.");
    std::error_code ec;
    const std::string datei = tagDatei();
    if (std::filesystem::exists(std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(datei.data()), datei.size())), ec)) {
        qtMeldung("Overwrite tag file?", "WARNING! A tag file already exists! Overwrite it?", MeldungArt::Warnung, true, [l](bool ja) {
            if (ja) { tagDateiSchreiben(l); }
        });
    } else {
        tagDateiSchreiben(l);
    }
}

// --- SunPlug: ET-MapCoordinator --------------------------------------------------------

void kartenkoordinator() {
    Editor& ed = e();
    const auto welt = ed.karte.welt();
    if (!welt) {
        fehler("SunPlug: no worldspawn found!");
        qtMeldung("ET-MapCoordinator", "No worldspawn was found in the map!\nIn order to use this tool the map must have at least one brush in the worldspawn.",
                  MeldungArt::Fehler, false, nullptr);
        return;
    }
    SunplugZustand& s = ed.sunplug;
    s.werte = kp::kartenkoordinatenLesen(*welt);
    ausgabe("SunPlug: calculating optimal coordinates");
    s.optimal = kp::optimaleKoordinaten(karte::kartenGrenzen(ed.karte));
    ausgabe("SunPlug: advised mapcoordsmins=" + std::to_string(s.optimal.minX) + " " + std::to_string(s.optimal.maxY));
    ausgabe("SunPlug: advised mapcoordsmaxs=" + std::to_string(s.optimal.maxX) + " " + std::to_string(s.optimal.minY));
    s.offen = true;
    s.oeffnen = true;
}

void kartenkoordinatorZeichnen() {
    Editor& ed = e();
    SunplugZustand& s = ed.sunplug;
    if (!s.offen) { return; }
    const std::string knopf = trK(Str::KeSunOptimal);
    const std::string lMin = trK(Str::KeSunMins);
    const std::string lMax = trK(Str::KeSunMaxs);
    const std::vector<std::string> kn{trK(Str::KeSunSet), trK(Str::KeCancel)};
    const float spin = qtd::spinHinweis(-65536, 65536, 0, "X: ");
    const float labelB = std::max(qtd::textBreite(lMin), qtd::textBreite(lMax));
    const float feldB = std::max({qtd::knopfHinweis(knopf), spin + 6.0F + spin, qtd::knoepfeHinweis(kn)});
    const float x1 = 11.0F + labelB + 6.0F;
    const float innenB = x1 + feldB + 11.0F;
    const float innenH = 11 + 22 + 6 + 3 + 6 + 22 + 6 + 22 + 6 + 22 + 11;   // 137
    if (!qtd::beginnen(std::string(trK(Str::KeSunTitel)) + "###kartesunplug", s.oeffnen, innenB, innenH)) {
        if (qtd::geschlossen()) { s.offen = false; }
        return;
    }
    qtd::merke("SunDialog", 0, 0, innenB, innenH);
    // "Get optimal mapcoords" (QPushButton in der Feldspalte, so breit wie sie)
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (qtd::px(22.0F) - ImGui::GetTextLineHeight()) * 0.5F)});
    ImGui::SetCursorScreenPos(qtd::bp(x1, 11));
    if (ImGui::Button((knopf + "##sunoptimal").c_str(), ImVec2{qtd::px(feldB), qtd::px(22.0F)})) {
        s.werte = {s.optimal.minX, s.optimal.maxY, s.optimal.maxX, s.optimal.minY};
    }
    ImGui::PopStyleVar();
    qtd::merke("sunoptimal", x1, 11, feldB, 22);
    qtd::hLinie(11, 39, innenB - 22.0F);   // addRow( line ): ueber beide Spalten
    const float halb = std::floor((feldB - 6.0F) * 0.5F);
    static const char* const ids[4] = {"sunminx", "sunminy", "sunmaxx", "sunmaxy"};
    for (int i = 0; i < 4; ++i) {
        const float y = i < 2 ? 48.0F : 76.0F;
        const float x = (i % 2) == 0 ? x1 : x1 + halb + 6.0F;
        if ((i % 2) == 0) { qtd::text(i < 2 ? lMin : lMax, 11, y, 22); }
        double w = s.werte[static_cast<std::size_t>(i)];   // SpinBox( -65536, 65536 ) mit setPrefix( "X: " / "Y: " )
        (void)qtd::zahlFeld("", ids[i], w, -65536, 65536, 0, 1, -1.0F, x, (i % 2) == 0 ? halb : feldB - halb - 6.0F, y, true, (i % 2) == 0 ? "X: " : "Y: ");
        s.werte[static_cast<std::size_t>(i)] = static_cast<int>(w);
    }
    const int k = qtd::knoepfe(kn, "sunok", innenB - 11.0F, 104, 0, 1);
    qtd::beenden(k >= 0);
    if (k == 0) {   // UndoableCommand( "SunPlug.entitySetMapcoords" )
        karte::Befehl b(ed.karte.undo, "SunPlug.entitySetMapcoords");
        ed.karte.welt()->setzen("mapcoordsmins", std::to_string(s.werte[0]) + " " + std::to_string(s.werte[1]));
        ed.karte.welt()->setzen("mapcoordsmaxs", std::to_string(s.werte[2]) + " " + std::to_string(s.werte[3]));
        ++ed.sichtStand;
    }
    if (k >= 0) { s.offen = false; }
}

// --- UFO:AI ------------------------------------------------------------------------------

void ufoai(const std::function<std::string(std::vector<std::string>&)>& f) {
    Editor& ed = e();
    std::vector<std::string> zeilen;
    std::string nachricht;
    {
        karte::Befehl b(ed.karte.undo, "UFO:AI");   // Abweichung: NRC setzt die Schluessel ohne UndoableCommand
        nachricht = f(zeilen);
    }
    zeilenAusgeben(zeilen, true);
    if (!nachricht.empty()) { qtMeldung("Note", nachricht, MeldungArt::Info, false, nullptr); }
    ++ed.sichtStand;   // SceneChangeNotify
    auswahlGeaendert();
}

}  // namespace

std::vector<PluginUmsetzung> kleinePluginUmsetzungen() {
    std::vector<PluginUmsetzung> v;
    v.push_back({"shaderPlug::About", [] { qtMeldung("About", kp::kShaderplugAbout, MeldungArt::Info, false, nullptr); }, nullptr});
    v.push_back({"shaderPlug::CreateTagFile", [] { tagDateiAnlegen(); }, nullptr});
    v.push_back({"sunPlug::About", [] { qtMeldung("About SunPlug", kp::kSunplugAbout, MeldungArt::Info, false, nullptr); }, nullptr});
    v.push_back({"sunPlug::ET-MapCoordinator", [] { kartenkoordinator(); }, nullptr});
    v.push_back({"uFO:AI::About", [] {
                     qtMeldung("About", kp::ufoaiAbout(std::string("behaved ") + bhed::kFassung), MeldungArt::Info, false, nullptr);
                 },
                 nullptr});
    v.push_back({"uFO:AI::Worldspawn", [] { ufoai([](std::vector<std::string>& z) { return kp::ufoaiWeltWerte(e().karte, false, z); }); }, nullptr});
    v.push_back({"uFO:AI::WorldspawnReset", [] { ufoai([](std::vector<std::string>& z) { return kp::ufoaiWeltWerte(e().karte, true, z); }); }, nullptr});
    v.push_back({"uFO:AI::PerformCheck", [] { ufoai([](std::vector<std::string>& z) { return kp::ufoaiPruefen(e().karte, z); }); }, nullptr});
    for (int i = 1; i <= 8; ++i) {
        v.push_back({"uFO:AI::Level" + std::to_string(i), [i] {
                         ufoai([i](std::vector<std::string>& z) {
                             kp::ufoaiLevel(e().karte, e().ufoai, kp::kContentsLevel1 << (i - 1), z);
                             return std::string();
                         });
                     },
                     nullptr});
    }
    const std::pair<const char*, int> filter[4] = {{"uFO:AI::StepOn", 0}, {"uFO:AI::ActorClip", 1}, {"uFO:AI::WeaponClip", 2}, {"uFO:AI::NoDraw", 3}};
    for (const auto& [name, art] : filter) {
        const int a = art;
        v.push_back({name, [a] {
                         ufoai([a](std::vector<std::string>& z) {
                             kp::ufoaiFilter(e().karte, e().ufoai, a, z);
                             return std::string();
                         });
                     },
                     nullptr});
    }
    return v;
}

void kleinePluginDialogeZeichnen() { kartenkoordinatorZeichnen(); }

}  // namespace bhed::gui::karteneditor
