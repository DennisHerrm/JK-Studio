// kartenformat.h - Karten-Editor: .map lesen und schreiben wie NetRadiant-Custom.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): plugins/mapq3/parse.cpp
// (Map_Read, Entity_parseTokens, LayersParser), plugins/mapq3/write.cpp
// (Map_Write, Layers_Write, WriteTokensWalker), plugins/mapq3/plugin.cpp
// (MapQ3API: Formaterkennung), radiant/brushtokens.h (Flaechen lesen/
// schreiben in Quake3, Brush Primitives, Valve 220), radiant/patch.h
// (patchDef2/patchDef3-Tokens), libs/script/scripttokeniser.h (der
// Kartentokenizer mit "//@$&"-Sonderkommentaren), libs/script/
// scripttokenwriter.h (SimpleTokenWriter), libs/stream/textstream.h
// (Decimal: "%10.10lf" ohne Nullen am Ende), libs/stringio.h (Tokeniser_*),
// libs/stream/textfilestream.h (Textmodus "rt"/"wt").
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   Copyright (C) 2001-2006, William Joseph. All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
//
// Zeilenenden: NRC fuer Windows (MinGW, msvcrt) oeffnet .map im Textmodus.
// Beim Lesen wird CR LF zu LF und ein Strg+Z beendet die Datei, beim
// Schreiben wird jedes LF zu CR LF. karteSchreiben() liefert den Text mit LF
// (wie die Tokens); karteSpeichern() schreibt ihn wie NRC unter Windows.
#ifndef BHED_KARTE_KARTENFORMAT_H
#define BHED_KARTE_KARTENFORMAT_H

#include "bhed/karte/karte.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace bhed::karte {

struct LeseErgebnis {
    // false: NRC hat das Lesen mit "entity N: parse error" abgebrochen; die
    // bis dahin gelesenen Entitaeten sind trotzdem in der Karte (wie NRC).
    bool vollstaendig = true;
    std::vector<std::string> meldungen;   // Wortlaut wie globalErrorStream/globalWarningStream
};

// MapQ3API::readGraph: Text (mit LF) in eine leere Karte lesen. Das erkannte
// Pinselformat landet in karte.kontext.format.
LeseErgebnis karteLesen(const std::string& text, Karte& karte);

// MapQ3API::writeGraph: die Karte im NRC-Format (LF).
std::string karteSchreiben(const Karte& karte, std::vector<std::string>* meldungen = nullptr);
// Nur ein Teil (Map_ExportSelected mit IncludeSelectedWalker): Entitaeten, fuer
// die "entitaet" wahr ist, mit den Kindern, fuer die "kind" wahr ist.
std::string karteSchreiben(const Karte& karte, const std::function<bool(const Entitaet&)>& entitaet,
                           const std::function<bool(const Entitaet&, const Primitiv&)>& kind, std::vector<std::string>* meldungen = nullptr);

// Textmodus von msvcrt (TextFileInputStream "rt" / TextFileOutputStream "wt").
std::string textmodusLesen(const std::string& roh);
std::string textmodusSchreiben(const std::string& text);

// MapResource_loadFile / MapResource_saveFile (mit Sicherung als .bak wie
// MapResource_save, wenn sicherung gesetzt ist).
bool karteLaden(const std::string& pfad, Karte& karte, LeseErgebnis& ergebnis);
bool karteSpeichern(const std::string& pfad, const Karte& karte, bool sicherung, std::string* fehler = nullptr);

// Decimal (libs/stream/textstream.h:254-282): "%10.10lf" in 22 Zeichen,
// Leerzeichen vorne und Nullen hinten weg, ein "." am Ende auch.
std::string dezimal(double f);

// Face::m_quantise (brush.h:826-837, brushmodule.cpp:77-83): die Ebenenpunkte
// beim Schreiben (FacePlane_exportTokens) unveraendert (quantiseFloating) oder
// auf ganze Zahlen gerundet (quantiseInteger = float_to_integer = lrint) -
// Einstellung "Snap plane points to integer grid" ("BrushSnapPlanes").
void setzeEbenenPunkteGanzzahlig(bool an);   // Face_importSnapPlanes
[[nodiscard]] bool ebenenPunkteGanzzahlig();   // Face_exportSnapPlanes

// patchDef3 gibt es in NRC nur in den Doom3/Quake4-Formaten (mapdoom3,
// mapquake4); in Quake3-Karten (mapq3) ist es ein Lesefehler, und q3map2 kennt
// es nicht. Fuer Vollstaendigkeit hier die Token-Leser/-Schreiber von
// PatchDoom3TokenImporter / PatchDoom3TokenExporter (patch.h:1134-1261) fuer
// den Teil ab "{" nach dem Schluesselwort.
bool patchDoom3Lesen(const std::string& text, Patch& patch, bool patchDef3, std::vector<std::string>* meldungen = nullptr);
std::string patchDoom3Schreiben(const Patch& patch);

}  // namespace bhed::karte
#endif
