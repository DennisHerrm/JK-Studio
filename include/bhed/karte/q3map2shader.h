// q3map2shader.h - REST8: welcher Eintrag eines Shadernamens beim Kompilieren gilt (bhed_q3map2), fuer das Echtzeit-Licht
// des Karten-Editors und den Vergleich mit dem Spiel.
//
// Steht ein Shadername in mehreren .shader-Dateien (oder zweimal in einer), entscheidet q3map2 anders als das Spiel
// (bhed/shaderscript.h shaderTextWieSpiel) und anders als das Shader-Inventar von NetRadiant-Custom (texturen.h). Das
// Echtzeit-Licht zeigt das Kompilierergebnis und nimmt deshalb die Regel von q3map2; das Spielbild ("Built map as in the game")
// und die Vorschau bleiben bei der Regel des Spiels. Portiert aus q3map2 (NetRadiant-Custom 20260114, third_party/q3map2):
//   - InitPaths (q3map2/path_init.cpp:290-428): vfsInitDirectory fuer <fs_basepath>/<fs_game>, dann <fs_basepath>/<basegame>
//     (je mit '/' am Ende, insert_unique ohne Gross/klein), dann jedes -fs_pakpath; -fs_homepath "" (das Build-Menue) fuegt
//     unter Windows nichts hinzu (AddHomeBasePath nur unter Unix).
//   - vfsInitDirectory (common/vfs.cpp:140-191): das Verzeichnis und jedes *.pk3dir darin (Folge von FindFirstFile) in
//     g_strDirs; die *.pk3 aufsteigend nach string_compare_nocase_upper, ihre Eintraege (Folge des Zip-Verzeichnisses, nur
//     Namen unter 256 Zeichen, klein, '\' als '/') per emplace_front in g_pakFiles - am Ende steht der letzte Eintrag der
//     letzten .pk3 des LETZTEN Verzeichnisses vorn: die .pk3 von basegame liegen vor denen des Mods.
//   - LoadShaderInfo (q3map2/shaders.cpp:1806-1856): gibt es <shaderPath>/shaderlist.txt (vfsGetFileCount: lose in jedem
//     g_strDirs, dann jeder Treffer in g_pakFiles), nur die dort genannten Dateien ("<name>.shader", eindeutig ohne
//     Gross/klein, jede Liste der Reihe nach); sonst vfsListShaderFiles (vfs.cpp:194-228): erst die losen <shaderPath>/*.shader
//     je g_strDirs, dann g_pakFiles (nur genau <shaderPath>/x.shader), eindeutig ohne Gross/klein.
//   - LoadScriptFile -> vfsLoadFile(name, 0) (vfs.cpp:281-330): die erste lose Datei in g_strDirs, sonst der erste Treffer in
//     g_pakFiles - eine lose Datei gewinnt immer gegen jede .pk3, die .pk3 von basegame gegen die des Mods.
//   - ParseShaderFile (shaders.cpp:823-1735) mit dem Zerleger aus common/scriplib.cpp (GetToken: Kommentare ; # // /* */,
//     "char" mit Vorzeichen wie unter MSVC und clang-cl - Bytes ab 0x80 trennen Woerter wie Leerraum, $include): jedes Wort auf
//     oberster Ebene ist ein Name (":q3map" am Ende faellt weg), danach muss "{" kommen, sonst bricht q3map2 ab. Im Block:
//     "{" oeffnet eine Stufe bis zum naechsten "}" (map/clampMap/animMap/clampAnimMap/mapComp/mapNoComp nehmen ihr Bild als
//     Wort derselben Zeile, solange qer_editorimage, q3map_lightImage und implicit* leer sind); jede andere Anweisung und jede
//     Stufe beendet der Rest ihrer Zeile ("ignore all other tokens on the line"), die Matrizen von q3map_tcGen
//     vector/ivector, q3map_colorMod & Co. und q3map_fogDir duerfen Zeilen ueberspannen. AllocShaderInfo legt jeden Eintrag an,
//     ShaderInfoForShader nimmt den ERSTEN mit dem Namen (UnsortedSet::find_first, ohne Gross/klein).
// Nicht nachgebildet: die Abbrueche, wenn einer anderen Anweisung ihr Wert auf derselben Zeile fehlt ("Line is incomplete") -
// dann baut q3map2 die Karte gar nicht; abbruch nennt nur die Abbrueche des Aufbaus (fehlendes "{", Matrix, Wort zu lang).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant. GtkRadiant is free software; you can redistribute it and/or modify it under the terms of
//   the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your
//   option) any later version.
#ifndef BHED_KARTE_Q3MAP2SHADER_H
#define BHED_KARTE_Q3MAP2SHADER_H

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace bhed::karte {

// InitPaths: die Verzeichnisse von vfsInitDirectory in ihrer Folge (Pfade mit '/' am Ende, ohne doppelte).
[[nodiscard]] std::vector<std::string> q3map2Verzeichnisse(const std::string& basisPfad, const std::string& fsGame,
                                                           const std::string& basisSpiel, const std::vector<std::string>& pakPfade);

struct Q3map2ShaderEintrag {
    std::string datei;    // "<shaderPath>/x.shader"
    std::string quelle;   // die .pk3 oder das Verzeichnis (mit '/') der geladenen Datei
    std::string text;     // shaderEintragText(Name klein, Block) - fuer parseShaderScript und shaderSkriptLesen
};

struct Q3map2Shader {
    std::map<std::string, Q3map2ShaderEintrag> eintraege;   // klein -> der Eintrag, den q3map2 nimmt
    std::vector<std::string> dateien;                        // die gelesenen Dateien in ihrer Folge
    bool shaderliste = false;                                // aus shaderlist.txt
    std::size_t verdeckt = 0;                                // weitere Eintraege schon gesehener Namen
    std::string abbruch;                                     // q3map2 bricht ab (Error); danach keine Eintraege mehr
};

[[nodiscard]] Q3map2Shader q3map2ShaderLesen(const std::vector<std::string>& verzeichnisse, const std::string& shaderPfad = "shaders");

// Gleicher Inhalt zweier Eintraege (q3map2 gegen das Spiel): dieselben Woerter ohne Kommentare (// und /* */), ohne Gross/klein -
// das Spiel und q3map2 lesen Schluesselwoerter und Pfade ohne Gross/klein.
[[nodiscard]] bool shaderEintraegeGleich(const std::string& a, const std::string& b);
// Der letzte Teil eines Pfads (".../assets1.pk3" -> "assets1.pk3", ".../base/" -> "base/") fuer Meldungen.
[[nodiscard]] std::string pfadBlatt(const std::string& pfad);

}  // namespace bhed::karte

#endif
