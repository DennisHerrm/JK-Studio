// pruefung.h - Menue-Editor M2 Punkt 7: Pruefungen vor dem Export.
// Portiert aus OpenJK (die Grenzen aus code/ui/ui_shared.h, codemp/ui/ui_shared.h, Movie Duels); die Pruefungen sind eigener Code.
//
// Was die Engine nicht kann, meldet die Pruefung, statt dass die Vorschau es
// trotzdem zeigt:
//   - Grenzen je Fassung (OpenJK code/ui/ui_shared.h bzw. codemp/ui/ui_shared.h,
//     Movie Duels code/ui/ui_shared.h): MAX_MENUITEMS (Menu_Parse liest weitere
//     itemDefs nicht mehr), MAX_MENUS, die Laenge eines Skripts (PC_Script_Parse:
//     SP 4096, MP 2048 Zeichen, der Rest faellt weg), MAX_MULTI_CVARS,
//     MAX_COLOR_RANGES, MAX_LB_COLUMNS;
//   - Items, die nie zu sehen sind (ausserhalb 640x480, ohne Groesse, oder
//     visible 0 ohne ein Skript, das sie zeigt);
//   - fehlende Dateien (Bilder/Shader, Modelle, Videos, Schriften, Klaenge);
//   - Vertraeglichkeit: Schluesselwoerter und Item-Typen, die eine andere
//     Fassung (JA SP/MP, Jedi Outcast, Movie Duels) nicht kennt.
//
// Grenzwerte:
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//   OpenJK is free software; you can redistribute it and/or modify it under the
//   terms of the GNU General Public License version 2 (LICENSE). Die Pruefungen
//   selbst sind eigener Code.
#ifndef BHED_MENUE_PRUEFUNG_H
#define BHED_MENUE_PRUEFUNG_H

#include "bhed/menue/menue.h"

#include <functional>
#include <string>
#include <vector>

namespace bhed::menue {

struct Grenzen {
    int menuItems = 0, menus = 0, skript = 0, multiCvars = 0, farbBereiche = 0, spalten = 0;
};
// Die Grenzen der Fassung, die die Datei liest.
[[nodiscard]] Grenzen grenzenFuer(Dialekt d, Spiel s);
// Laenge eines Skripts, wie PC_Script_Parse sie zusammensetzt (-1 = kein Skript).
[[nodiscard]] int skriptLaenge(const Eigenschaft& e);
// M6 Punkt 10: wie voll ein Menue gegen die Grenzen ist (live beim Bauen)
struct Auslastung {
    int items = 0, menus = 0;
    int skript = 0;           // laengstes Skript des Menues
    std::string skriptWo;     // "item: action" bzw. "onOpen"
    int multi = 0;            // laengste cvarList
    std::string multiWo;
    Grenzen g;
};
[[nodiscard]] Auslastung auslastung(const Datei& d, int menu);

enum class BefundArt {
    ZuVieleItems,      // a = Items, b = Grenze
    ZuVieleMenues,     // a = Menues, b = Grenze
    SkriptZuLang,      // a = Laenge, b = Grenze, name = Schluessel
    ZuVieleWerte,      // a = Werte, b = Grenze (cvarList/cvarFloatList)
    ZuVieleFarben,     // a, b (addColorRange)
    ZuVieleSpalten,    // a, b (columns)
    NieSichtbar,       // name = Grund ("rect", "visible")
    DateiFehlt,        // name = Datei, wert = wofuer
    UnbekanntIn,       // name = Schluessel oder Typ, wert = Fassung ("JA SP", "JA MP", "JK2", "MD")
    VollbildOhneBild,  // rc608: MP, fullScreen ohne background und ohne deckendes Element: das Spiel zeigt den grauen Kasten
    KeinBild,          // rc608 Teil 3a: Rahmen, in dem noch kein Bild gewaehlt ist (WINDOW_STYLE_SHADER ohne background): das Spiel zeigt den grauen Kasten; name = Element
    CinematicSp,       // M4: WINDOW_STYLE_CINEMATIC im Einzelspieler (Window_Paint zeichnet es nicht)
};
struct Befund {
    BefundArt art = BefundArt::DateiFehlt;
    int menu = -1, item = -1;
    int a = 0, b = 0;
    std::string name, wert;
    bool fehler = false;    // true: die Engine verliert etwas (Grenze), sonst Hinweis
};

// gibt(pfad): gibt es die Datei im Spiel oder im Paket? gibtBild(name): ein Bild oder
// ein Shader mit diesem Namen?
struct PruefKontext {
    std::function<bool(const std::string& pfad)> gibt;
    std::function<bool(const std::string& name)> gibtBild;
    std::vector<Spiel> zielSpiele{Spiel::JA, Spiel::JK2, Spiel::MD};   // fuer die Vertraeglichkeit
};
[[nodiscard]] std::vector<Befund> pruefeFuerSpiel(const Datei& d, const PruefKontext& k);

}  // namespace bhed::menue

#endif  // BHED_MENUE_PRUEFUNG_H
