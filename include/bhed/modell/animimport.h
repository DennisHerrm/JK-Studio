// modell/animimport.h - Animationen aus anderen Programmen (glTF, FBX, BVH, COLLADA) als Quelle fuer das Retargeting
//
// Gelesen wird ueber assimp (mit allen Lesern gebaut, wie der Geometrie-Import). Aus der Szene wird ein Skelett mit
// Grundstellung (Ruhepose der Datei) und je Bild die lokalen Stellungen wie in einer .gla - nur unkomprimiert
// (GlaAnimation::roh), damit lange Wege der Wurzel und Meter-Dateien nicht an die 14 Bytes des Spiels (Ort 1/64 bis +-512)
// stossen. Das Retargeting (Stufe 5) und die Vorschau nehmen die Quelle damit wie eine .gla.
//
// Einheiten und Achsen (je Format belegt):
//   FBX      GlobalSettings UpAxis/FrontAxis/CoordAxis mit Vorzeichen und UnitScaleFactor (Zentimeter je Einheit; FBX SDK
//            FbxAxisSystem/FbxSystemUnit, assimp legt sie als Metadaten ab: FBXConverter.cpp SetupGlobalSettings). Die
//            eigene Wurzel-Korrektur von assimp (correctRootTransform, nur mit Netzen) bleibt aus
//            (AI_CONFIG_IMPORT_FBX_IGNORE_UP_DIRECTION), sonst haenge die Lage davon ab, ob die Datei ein Netz hat.
//   glTF     Meter, +Y oben, die Vorderseite schaut nach +Z, -X ist rechts (glTF 2.0, 3.4 "Coordinate System and Units").
//   COLLADA  assimp rechnet <unit meter> und <up_axis> in die Wurzel (ColladaLoader.cpp: mUnitSize, Z_UP -> Y_UP): danach
//            Meter und +Y oben; vorn +Z wie glTF.
//   BVH      Das Format legt weder Einheit noch Achsen fest (Blender schreibt +Z oben, viele Aufnahmen +Y). Oben ist die
//            Achse, entlang der der Rumpf in der Ruhepose steht (Wurzel -> fernster Knochen ohne Seite), links ergibt sich
//            aus den Knochennamen (Left*/Right*, *_l/*_r). Einheiten der Datei bleiben (das Retargeting gleicht ueber die
//            Beinlaenge aus).
//   Ziel     JK Studio rechnet wie JKA: +Z oben, links +X, vorn -Y (retarget.cpp: vorn = links x oben), Laengen in cm
//            (die Einheit der Ziel-Figur gleicht k = Beinlaenge Ziel / Quelle aus).
// Bildrate:  FBX TimeMode (mTicksPerSecond = Bildrate, Schluessel in Bildern: FBXConverter.cpp ConvertAnimationStack),
//            BVH "Frame Time" (BVHLoader.cpp: 1 / mAnimTickDuration), glTF/COLLADA ohne Bildrate (Zeiten in ms,
//            glTF2Importer.cpp/ColladaLoader.cpp: 1000 Ticks je s) - dort aus dem kleinsten Abstand der Schluessel.
#ifndef BHED_MODELL_ANIMIMPORT_H
#define BHED_MODELL_ANIMIMPORT_H

#include <string>
#include <vector>

#include "bhed/gla.h"

namespace bhed::modell {

struct AnimImport {
    GlaAnimation anim;                 // Skelett und Bilder (roh), alle Animationen der Datei hintereinander
    std::vector<AnimEntry> sequenzen;  // je Animation: Name, erstes Bild, Zahl, fps (wie eine animation.cfg)
    std::string format;                // "FBX", "glTF", "BVH", "COLLADA", ...
    float zentimeter = 1.0F;           // cm je Einheit der Datei (BVH: 1, Einheit unbekannt)
    std::string oben;                  // Achse oben in der Datei, z. B. "+Y"
    std::string vorn;                  // Achse vorn (Blickrichtung der Figur) in der Datei
    double ticksJeSekunde = 0.0;       // aus der Datei (0 = nicht angegeben)
    std::vector<std::string> hinweise; // z. B. "ungleichmaessige Skalierung an X entfernt"
};

// Endung .fbx .glb .gltf .bvh .dae (klein oder gross).
[[nodiscard]] bool istAnimImportEndung(const std::string& pfad);
// Die Datei lesen. false: keine Animation/kein Skelett oder nicht lesbar (Grund in fehler). fpsVorgabe > 0 erzwingt die
// Bildrate (sonst aus der Datei).
[[nodiscard]] bool animImportieren(const std::string& pfad, AnimImport& aus, std::string* fehler = nullptr, int fpsVorgabe = 0);

// BVH schreiben (fuer die Preset-Bibliothek): Hierarchie mit den Orten der Grundstellung als OFFSET, Wurzel und Knochen mit
// bewegter Lage 6 Kanaele (Xposition Yposition Zposition Zrotation Xrotation Yrotation), sonst 3 (Zrotation Xrotation
// Yrotation), 4 Nachkommastellen (cm, Grad), Bilder [erstes, erstes + anzahl), Achsen der
// Datei wie beim Lesen (+Y oben, +Z vorn, +X links, Laengen in cm). Ein Lesen mit animImportieren gibt dieselben Welten.
[[nodiscard]] bool schreibBvh(const GlaAnimation& g, int erstes, int anzahl, int fps, std::string& aus, std::string* fehler = nullptr);

}  // namespace bhed::modell
#endif
