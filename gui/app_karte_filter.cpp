// app_karte_filter.cpp - Karten-Editor: die Filter (Menue Filter und die
// Filterleiste mit ihren Rechtsklick-Handlungen).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/filters.cpp
// (ToggleFilterFlag::toggle/reset, PerformFiltering, InvertFilters,
// ResetFilters, "SI_Exclude"), radiant/filterbar.cpp:18-118 (GetCommonShader,
// SetCommonShader: "textureNameSetSelected", CommonFunc_tex mit
// m_toggleTexNum und CommonFunc::m_recentFunc, CommonFunc_command,
// CommonFunc_group: Entity_createFromSelection( "func_group" ),
// FilterToolbarHandler: Rechtsklick beim Druecken und beim Doppelklick,
// QEvent::Enter setzt m_recentFunc zurueck).
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
#include "bhed/karte/entitaetbefehle.h"
#include "bhed/karte/filter.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace bhed::gui::karteneditor {

namespace {

Editor& e() { return editor(); }

// PerformFiltering: UpdateFilters + SceneChangeNotify
void filternJetzt() {
    Editor& ed = e();
    karte::filterAnwenden(ed.karte, ed.filterMaske);
    ++ed.sichtStand;
    ed.filterSchluessel = filterSchluesselJetzt();
}

}  // namespace

std::size_t filterSchluesselJetzt() {
    const Editor& ed = e();
    std::size_t h = ed.karte.undo.aenderungen() * 1315423911U;
    h ^= ed.karte.undo.undoAnzahl() * 2654435761U + (h << 6) + (h >> 2);
    h ^= ed.karte.undo.redoAnzahl() * 40503U + (h << 6) + (h >> 2);
    h ^= ed.ladeZaehler * 97U + (h << 6) + (h >> 2);
    h ^= static_cast<std::size_t>(ed.filterMaske) * 7919U + (h << 6) + (h >> 2);
    h ^= reinterpret_cast<std::uintptr_t>(ed.karte.kontext.texturen) + (h << 6) + (h >> 2);
    return h;
}

void filterPruefen() {   // Filterable::updateFiltered bei jeder Aenderung (Instanz, Shader, Flaggen)
    Editor& ed = e();
    const std::size_t s = filterSchluesselJetzt();
    if (s == ed.filterSchluessel) { return; }
    ed.filterSchluessel = s;
    karte::filterAnwenden(ed.karte, ed.filterMaske);
    if (ed.filterMaske != 0) { ++ed.sichtStand; }
}

bool filterBefehlAn(const std::string& name) {   // ToggleFilterFlag::active
    const karte::FilterBefehl* b = karte::filterBefehl(name);
    return b != nullptr && (e().filterMaske & b->maske) != 0;
}

void filterBefehl(const std::string& name) {
    Editor& ed = e();
    if (name == "InvertFilters") {   // jeder ToggleFilterFlag::toggle
        ed.filterMaske = karte::filterUmkehren(ed.filterMaske);
    } else if (name == "ResetFilters") {   // jeder ToggleFilterFlag::reset
        ed.filterMaske = 0;
    } else if (const karte::FilterBefehl* b = karte::filterBefehl(name)) {   // ToggleFilterFlag::toggle
        ed.filterMaske ^= b->maske;
    } else {
        return;
    }
    filternJetzt();
    einstellungenSpeichern();   // "SI_Exclude"
}

// --- Filterleiste: Rechtsklick (FilterToolbarHandler) ---------------------------------------

namespace {

const void* g_zuletzt = nullptr;            // CommonFunc::m_recentFunc
std::map<std::string, std::size_t> g_texNr;   // CommonFunc_tex::m_toggleTexNum je Knopf

}  // namespace

void filterleisteBetreten() { g_zuletzt = nullptr; }   // QEvent::Enter

void filterleisteRechtsklick(const befehle::Knopf& k) {
    Editor& ed = e();
    const std::string r = k.rechtsklick;
    if (r.empty()) { return; }
    const void* ich = &k;
    if (r.rfind("tex:", 0) == 0) {   // CommonFunc_tex::exec
        std::vector<std::string> namen;
        std::stringstream ss(r.substr(4));
        std::string n;
        while (std::getline(ss, n, ',')) { namen.push_back(n); }
        if (namen.empty()) { return; }
        std::size_t& nr = g_texNr[k.befehl];
        if (g_zuletzt != ich) {
            nr = 0;
            g_zuletzt = ich;
        } else {
            nr %= namen.size();
        }
        // SetCommonShader: UndoableCommand "textureNameSetSelected" + Select_SetShader
        (void)karte::shaderSetzen(ed.karte, ed.bearbeitung.einstellung, karte::gemeinsamerShader(namen[nr]));
        ++nr;
        return;
    }
    if (r == "func_group") {   // CommonFunc_group: Entity_createFromSelection( "func_group", g_vector3_identity )
        entitaetErzeugenAusAuswahl("func_group", karte::Vektor3f(0, 0, 0));
        g_zuletzt = ich;
        return;
    }
    (void)ausfuehren(r);   // CommonFunc_command
    g_zuletzt = ich;
}

}  // namespace bhed::gui::karteneditor
