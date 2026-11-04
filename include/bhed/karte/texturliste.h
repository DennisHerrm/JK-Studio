// texturliste.h - Karten-Editor (rc609 K): die Texturen der Karte - welche Shader
// benutzt werden, wie oft (Brush-Seiten, Brushes, Patches), alle Flaechen damit
// waehlen, ersetzen (auch ganze Ordner) in einem Rueckgaengig-Schritt.
//
// Eigene Funktion (nicht in NetRadiant-Custom). Sie ergaenzt "Find / Replace
// Texture(s)" (FindReplaceTextures, auswahlbefehle.h), das unveraendert bleibt.
// Abweichung davon mit Absicht: NRC ersetzt auf allen Brushes, aber nur auf
// sichtbaren Patches (Scene_BrushFindReplaceShader / Scene_PatchFindReplaceShader);
// hier gilt der gewaehlte Umfang fuer Brushes und Patches gleich.
//
// Das Setzen des Shaders ist Face::SetShader (Flaeche::setzeShader: der Texdef
// bleibt, bei Brush Primitives umgerechnet mit removeScale/addScale) und
// Patch::SetShader. "Gleiche Weltgroesse" rechnet danach so um, dass eine
// Wiederholung der neuen Textur dieselbe Flaeche deckt wie die alte:
// q3map2 nimmt fuer Quake- und Valve-220-Texdefs vecs / scale und shift
// (map.cpp: QuakeTextureVecs, ParseRawBrush) und teilt dann durch die
// Bildgroesse (surface.cpp:725, st /= shaderWidth bzw. shaderHeight):
// s = (P.v / scale + shift) / Breite. Brush Primitives stehen in der .map
// schon normiert (addScale) und bleiben dafuer unveraendert.
// Patches tragen normierte Texturkoordinaten (st der Kontrollpunkte); ihre
// Weltgroesse haengt nicht an der Bildgroesse und bleibt in beiden Faellen.
//
// Portiert aus NetRadiant-Custom 20260114 (radiant/brushmanip.cpp:
// FaceSelectByShader, Scene_BrushFindReplaceShader; radiant/patchmanip.cpp:
// Scene_PatchSelectByShader, Scene_PatchFindReplaceShader; radiant/brush.h:
// Face::SetShader; libs/shaderlib.h: shader_less_t, texdef_name_valid) und aus
// q3map2 (map.cpp: QuakeTextureVecs, ParseRawBrush; surface.cpp).
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
#ifndef BHED_KARTE_TEXTURLISTE_H
#define BHED_KARTE_TEXTURLISTE_H

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

class Karte;

// Welche Flaechen zaehlen: die ganze Karte (auch versteckte), nur die Auswahl
// (gewaehlte Brushes, Patches, Entitaeten als Ganzes und gewaehlte Flaechen)
// oder nur, was zu sehen ist (nicht versteckt, gefiltert oder in einer
// versteckten Ebene; bei Flaechen auch nicht vom Flaechenfilter verborgen).
enum class TexturUmfang { Alle, Auswahl, Sichtbar };

struct TexturZeile {
    std::string shader;   // wie in der Karte (erste Schreibweise), mit "textures/"
    int seiten = 0;       // Brush-Seiten
    int pinsel = 0;       // Brushes mit mindestens einer solchen Seite
    int patches = 0;
};
// Alle benutzten Shader im Umfang, ohne Gross/klein zusammengefasst und
// sortiert (shader_less_t).
[[nodiscard]] std::vector<TexturZeile> texturListe(const Karte& k, TexturUmfang u);

// Die Auswahl leeren (ohne dazu: dazu = true) und alle Brush-Seiten und
// Patches im Umfang mit diesem Shader waehlen. Ergebnis: Seiten, Patches.
std::pair<int, int> texturListeWaehlen(Karte& k, const std::string& shader, TexturUmfang u, bool dazu);

struct TexturErsatz {
    std::string alt;
    std::string neu;
};
struct ErsetzenZahl {
    int seiten = 0;
    int patches = 0;
};
// Alle Paare in einem Rueckgaengig-Schritt ("textureListReplace") auf Brush-
// Seiten und Patches im Umfang. weltGroesse: siehe oben. Ohne Treffer kein
// Schritt. Ungueltige Namen (texdef_name_valid) werden uebergangen.
ErsetzenZahl texturListeErsetzen(Karte& k, const std::vector<TexturErsatz>& paare, TexturUmfang u, bool weltGroesse);

// Ordner ersetzen: zu jedem Shader der Liste unter altOrdner der gleichnamige
// unter neuOrdner (Unterordner bleiben, Gross/klein egal). fehlt = es gibt ihn
// nicht (vorhanden liefert false) - eine Luecke, die beim Ersetzen bleibt.
struct OrdnerZuordnung {
    std::string alt;
    std::string neu;
    bool fehlt = false;
};
// Ordnernamen mit oder ohne "textures/" und abschliessendes '/'.
[[nodiscard]] std::string texturOrdnerName(const std::string& ordner);
[[nodiscard]] std::vector<OrdnerZuordnung> texturOrdnerZuordnung(const std::vector<TexturZeile>& liste, const std::string& altOrdner,
                                                                 const std::string& neuOrdner,
                                                                 const std::function<bool(const std::string&)>& vorhanden);

}  // namespace bhed::karte

#endif
