// modell/glmdatei.h - .glm aus dem Modelldokument schreiben (Modell-Block Stufe 2)
//
// Das Format nach mdx_format.h (OpenJK code/rd-common, Movie Duels Update 9 gleich) und dem Aufbau, den Carcass und
// G2C (DennisHerrm/G2C-toolkit, src/mdxm.cpp writeMdxm) schreiben und den alle .glm aus JKA, Jedi Outcast und Movie Duels
// haben (gemessen, NOTIZEN_mb2.md):
//
//   Kopf           mdxmHeader_t, 164 Bytes
//   Verzeichnis    int[numSurfaces], Versatz je Hierarchie-Eintrag ab dem Ende des Kopfs
//   Hierarchie     je Flaeche mdxmSurfHierarchy_t (Name, Flaggen, Shader, shaderIndex, Eltern, Kinder)
//   je LOD         mdxmLOD_t (ofsEnd), int[numSurfaces] ab dem Ende von mdxmLOD_t, dann je Flaeche
//                  mdxmSurface_t, Dreiecke, Ecken (32 Bytes), UV (8 Bytes), Knochenverweise
//
// Was die Datei mehr traegt als das Spiel liest (das letzte Gewicht einer Ecke, unbenutzte Gewichtsfelder, Rest hinter
// der Null eines Namens, die Reihenfolge der Knochenverweise), liest glmRohfelder ins Dokument; der Schreiber nimmt es,
// solange die Werte dazu passen. Ein unveraendertes Modell wird so Byte fuer Byte wieder dieselbe Datei.
#ifndef BHED_MODELL_GLMDATEI_H
#define BHED_MODELL_GLMDATEI_H

#include <string>

#include "bhed/modell/dokument.h"

namespace bhed::modell {

// Die Rohfelder einer .glm in ein Dokument, das ausGlm aus denselben Bytes gebaut hat (ausGlm ruft es selbst).
void glmRohfelder(const std::string& bytes, Inhalt& in);

struct GlmSchreibBericht {
    int flaechenRoh = 0;       // Flaechen (je Stufe) mit den Gewichtsfeldern der Datei
    int flaechenNeu = 0;       // neu kodiert (Gewichte geaendert oder nicht aus einer .glm)
    int eckenNeu = 0;
    bool umgelegt = false;     // ein umgelegtes Modell (JK2 <-> JKA) mit dem Skelett des Dokuments geschrieben
    int knochen = 0;           // numBones im Kopf
    std::string fehler;        // leer = geschrieben
};

// Das Dokument als .glm. false: nicht schreibbar (Grund in bericht.fehler) - kein Ghoul2-Modell, Name laenger als 63
// Zeichen, mehr als 32 Knochen an einer Flaeche (5 Bit je Verweis, mdx_format.h).
[[nodiscard]] bool glmSchreiben(const Inhalt& in, std::string& aus, GlmSchreibBericht* bericht = nullptr);

// Wo zwei .glm zuerst verschieden sind, als Feld beschrieben ("LOD 0, Flaeche 3 hips, Ecke 17: Gewichtsfelder");
// leer = gleich. Gelesen wird der Aufbau von `a`.
[[nodiscard]] std::string glmUnterschied(const std::string& a, const std::string& b);

}  // namespace bhed::modell
#endif
