// bau.h - Karten-Editor: das Build-Menue (Stufe 3 Punkt 3).
//
// Ein Build-Menue ist eine Liste von Eintraegen ("Builds") mit je einer Liste
// von Befehlszeilen, dazwischen Trenner, und eine Tabelle von Werkzeugen
// ("var", z. B. q3map2 = "[RadiantPath]q3map2.[ExecutableType]" -v ...). In
// Befehlen und Werkzeugen stehen Variablen in eckigen Klammern; ein Werkzeug
// kann Bedingungen enthalten (<cond value="[MonitorAddress]"> -connect
// [MonitorAddress]</cond>: nur, wenn der Wert nicht leer ist). Gespeichert
// als XML (<project version="2.0">), wie die default_build_menu.xml der
// Spielpakete.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/build.cpp
// (VariableString::evaluate, Conditional, Tool, ToolXMLConstructor::flush mit
// dem ExtraResourcePaths-Kniff, BuildXMLConstructor, ProjectXMLConstructor,
// RootXMLConstructor, build_construct_commands, build_commands_parse,
// ProjectXMLExporter/BuildXMLExporter, is_separator, Project_find,
// CustomDelegate1::setModelData), libs/versionlib.h (version_compatible),
// libs/xml/xmlwriter.h (XMLStreamWriter).
//
//   Copyright (C) 2001-2006, William Joseph.
//   All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_KARTE_BAU_H
#define BHED_KARTE_BAU_H

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace bhed::karte {

// g_build_variables (std::map: sortiert nach Namen)
using BauVariablen = std::map<std::string, std::string>;

// Ein Stueck eines Werkzeugs: VariableString (bedingt = false) oder
// Conditional (test, text).
struct BauTeil {
    bool bedingt = false;
    std::string test;   // Conditional::m_test
    std::string text;   // VariableString / Conditional::m_result
};
struct BauWerkzeug {    // Tool
    std::vector<BauTeil> teile;
};
struct BauEintrag {     // BuildPair: Name und Befehle (Build = std::list<BuildCommand>)
    std::string name;
    std::vector<std::string> befehle;
};
struct BauProjekt {
    std::vector<BauEintrag> eintraege;              // Project
    std::map<std::string, BauWerkzeug> werkzeuge;   // Tools (std::map)
};

inline constexpr const char* kBauTrenner = "-";       // SEPARATOR_STRING
inline constexpr const char* kBauMenueFassung = "2.0";   // BUILDMENU_VERSION

// is_separator: Name "-" und keine (oder nur leere) Befehle
[[nodiscard]] bool bauIstTrenner(const BauEintrag& e);

// VariableString::evaluate: [Name] -> Wert; eine unbekannte Variable wird
// leer und gemeldet ("undefined build variable: \"Name\"").
[[nodiscard]] std::string bauAuswerten(const std::string& text, const BauVariablen& v, std::vector<std::string>* meldungen = nullptr);
// Tool::evaluate: die Teile hintereinander, eine Bedingung nur, wenn ihr Test
// nicht leer ausgewertet wird.
[[nodiscard]] std::string bauAuswerten(const BauWerkzeug& w, const BauVariablen& v, std::vector<std::string>* meldungen = nullptr);

// build_commands_parse: liest ein Build-Menue (ein Projekt kann schon Eintraege
// haben - sie bleiben, wie bei NRC). false, wenn kein <project> mit einer
// passenden Fassung (2.x, x <= 0) darin war ("failed to parse build menu").
bool bauMenueLesen(const std::string& xml, BauProjekt& p, std::vector<std::string>* meldungen = nullptr);
// build_commands_write: wie NRCs XMLStreamWriter (Entities fuer < > " &).
[[nodiscard]] std::string bauMenueSchreiben(const BauProjekt& p);

// build_init_tools + build_construct_commands: die Werkzeuge (in der Reihenfolge
// der Namen) als Variablen, dann die Befehle des Eintrags `index`, ausgewertet;
// leere Befehle fallen weg. index ausserhalb: leer ("build #n not defined").
[[nodiscard]] std::vector<std::string> bauBefehle(const BauProjekt& p, std::size_t index, BauVariablen v,
                                                  std::vector<std::string>* meldungen = nullptr);
// build_init_tools allein (fuer die Tabelle "Editables" im Dialog)
void bauWerkzeugeAuswerten(const BauProjekt& p, BauVariablen& v, const std::string& bis = std::string());

// Die XML-Darstellung eines Werkzeugs ohne Kopf (CustomDelegate1: Inhalt des
// <var>, Text maskiert) und zurueck (setModelData: "<var name=\"DUMMY\">" +
// Text + "</var>" gelesen).
[[nodiscard]] std::string bauWerkzeugXml(const BauWerkzeug& w);
bool bauWerkzeugAusXml(const std::string& text, BauWerkzeug& w);

// Tooltip eines Menuepunkts (Build_constructMenu): die Befehle, durch '\n' getrennt.
[[nodiscard]] std::string bauTooltip(const BauEintrag& e);

}  // namespace bhed::karte

#endif
