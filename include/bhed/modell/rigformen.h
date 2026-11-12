// modell/rigformen.h - AP13K (rc612): die Formen der Rig-Stile in der Grundstellung
//
// shank 12.11.: "gibt es die moeglichkeit das man so ein Rig anbietet vom aussehen her dann noch das CAt Rig" - drei Vorbilder:
// ein Control Rig in 3ds Max (Ringe um die Koerperteile, nach Seite gefaerbt), sein CAT-Rig (Kaesten je Knochen) und sein
// Blender-IK-Rig (Oktaeder, Kreise und Quadrate als Formen). Der Stil aendert NUR das Aussehen: gerechnet wird weiter mit den
// Griffen von rigAnlegen (rig.h) und der IK aus ik.h.
//
// Die Formen entstehen einmal in der Grundstellung (Weltraum der basePose) und haengen starr an einem Knochen: in die Pose
// kommt ein Punkt mit X(knochen) * B(knochen)^-1 wie eine Ecke der Haut (Bindung Knochen). Pol-Ziele stehen, wo die IK-Spur sie
// hat - ihre Form wird nur an diesen Ort verschoben (Bindung Ort, `anker` ist ihr Punkt in der Grundstellung).
//
// Masse der Vorbilder (gelesen, nichts uebernommen ausser Zahlen - die Zeichnung ist eigene):
//   CAT      "Special move zetzel.max" (shanks Rig JK_humanoid), ausgelesen mit 3dsmaxbatch nach wp\werkzeuge\rigs\cat_rig.json:
//            je CATBone length/width/depth, je HubObject length/width/height, Drahtfarbe je Knoten, Fussplatten (IKTarget
//            20 x 10,6), Hand-IK-Ziele (20 x 20), UpVectorNode (Kreuz 10,6), CATParent (Bodenform 112,5 x 101,7).
//            CAT legt die Laenge auf die lokale X-Achse, depth auf Y, width auf Z; an Becken und Bein ist width die Breite
//            quer zur Figur, depth/length die Tiefe nach vorn (Transform der Knoten Hips und LLeg2). 1 Einheit der Szene sind
//            0,64 Einheiten des Spiels (fScale der _humanoid.gla): Bein 11,5 + 11,5 + 22,6003 = 45,6003 gegen 29,19.
//   Blender  "Jedi Academy IK Animation Rig v1.1.blend", ausgelesen mit blender -b nach blender_rig.json: vier Formen
//            (Kreis in der Laengsebene des Knochens Radius 10,30188, Kreis quer Radius 0,28735, Quadrat halbe Kante 0,36716,
//            Kreis quer Radius 4,08902 um 1,0905 versetzt), je mal der Laenge ihres Knochens - in Einheiten des Spiels am
//            _humanoid: Boden 26,37, Becken 10,47 (2,79 ueber dem Gelenk), Wirbel 9,20 und 7,71, Pol Knie 2,46, Pol Ellbogen
//            1,94, Quadrat Fuss 3,56, Quadrat Hand 2,48.
//   Control Rig (3ds Max, nur shanks Bild): Ringe um Kopf, Hals, Brust, Wirbel, Becken und die Glieder, Fussplatten,
//            Pol als Kugel, Bodenkreis. Keine Szene, keine Masse - die Ringe richten sich nach der Figur: der Radius ist der
//            weiteste Abstand der Haut vom Knochen an dieser Stelle mal kRingLuft (ohne Modell: Anteile der Figur, Annahme).
#ifndef BHED_MODELL_RIGFORMEN_H
#define BHED_MODELL_RIGFORMEN_H

#include <array>
#include <cstdint>
#include <vector>

#include "bhed/gla.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/rig.h"

namespace bhed::modell {

// Standard: wie der freigegebene Entwurf je Bedienmodus (bis rc611 der einzige); die anderen nach den drei Vorbildern.
enum class RigStil : std::uint8_t { Standard = 0, ControlRig, Cat, Blender };
inline constexpr int kRigStile = 4;

enum class FormGestalt : std::uint8_t {
    Kasten = 0,   // Quader (CAT-Knochen, Nabe)
    Ring,         // Kreis
    Rechteck,     // Fussplatte, Hand-IK-Ziel, Quadrat
    Kreuz,        // drei Achsen (CAT UpVectorNode)
    Kugel,        // drei Grosskreise (Pol des Control Rigs)
    Bodenform,    // CATParent: Dreieck mit Pfeil nach vorn
    Lutscher,     // Stiel mit Kreis (Schluesselbein des Control Rigs)
    Marke,        // kleine Raute (Drehpunkte der Fussrolle)
    Band,         // Becken des Control Rigs: Ring, Kasten mit Kreuz, zwei Pfeile
};
enum class FormBindung : std::uint8_t { Knochen = 0, Ort };

struct RigForm {
    FormGestalt gestalt = FormGestalt::Kasten;
    FormBindung bindung = FormBindung::Knochen;
    int knochen = -1;          // traegt die Form (Bindung Knochen) bzw. gehoert zum Griff
    int griff = -1;            // der Griff des Rigs, den die Form zeigt (-1: keiner)
    bool skelett = false;      // Kasten des Skeletts: gezeichnet mit "Knochen"; hat der Knochen einen Griff, auch als dessen Form
    bool schmuck = false;      // gehoert zu keinem Griff und keinem Knochen der Auswahl (Bodenkreis, Punkt ueber dem Kopf)
    bool innen = false;        // das Innere zaehlt beim Treffen (Kasten)
    bool gestrichelt = false;
    Rolle rolle = Rolle::Keine;
    Seite seite = Seite::Mitte;
    std::uint8_t farbe[3]{};
    float masse[3]{};          // Kasten: Laenge, Breite, Tiefe; Ring/Kugel: Radius; Rechteck: Laenge, Breite; Kreuz: Laenge
    float anker[3]{};          // Bindung Ort: der Punkt der Grundstellung, der am Ort des Griffs liegt
    std::vector<std::array<float, 3>> punkte;   // Grundstellung
    std::vector<std::array<int, 2>> linien;
};

struct RigFormen {
    std::vector<RigForm> formen;
    float massstab = 1.0F;     // gegen die Figur der Vorbilder (_humanoid: 1)
    float boden = 0.0F;        // Hoehe des Bodens unter der Figur (Grundstellung)
};

// Das Mass eines CAT-Knochens nach Rolle und Seite in Einheiten der Max-Szene (mal kCatEinheit mal rigMassstab = Einheiten
// des Spiels). `wirbel`: 1..wirbelZahl von unten; der oberste Wirbel ist die Brust (Nabe). nabe: Kasten mittig um das Gelenk
// (Becken, Brust) bzw. ab dem Gelenk nach oben (Kopf), sonst ab dem Gelenk entlang des Knochens.
struct CatMass {
    float laenge = 0.0F;
    float breite = 0.0F;
    float tiefe = 0.0F;
    std::uint8_t farbe[3]{};
    bool nabe = false;
};
[[nodiscard]] CatMass catMass(Rolle r, Seite s, int wirbel, int wirbelZahl) noexcept;

inline constexpr float kCatEinheit = 0.64F;          // 1 Einheit des CAT-Rigs in Einheiten des Spiels (fScale der _humanoid.gla)
inline constexpr float kCatBein = 45.6003F;          // LLeg11 11,5 + LLeg12 11,5 + LLeg2 22,6003
inline constexpr float kHumanoidBein = 29.1915F;     // _humanoid: lfemurYZ - ltibia 14,7202 + ltibia - ltalus 14,4713
inline constexpr float kHumanoidHoehe = 63.1472F;    // _humanoid: Gelenke der Grundstellung oben minus unten (Erkennung::hoehe)
inline constexpr float kRingLuft = 1.2F;             // Control Rig: der Ring steht um diesen Faktor weiter als die Haut (Annahme)

// Wie gross die Figur gegen den _humanoid ist: das Bein (Oberschenkel - Unterschenkel - Fuss) gegen kHumanoidBein, ohne
// erkannte Beine die Hoehe der Gelenke gegen kHumanoidHoehe.
[[nodiscard]] float rigMassstab(const GlaAnimation& g, const Erkennung& e);

// Das Skelett als Kaesten wie CAT: je Knochen mit Rolle ein Kasten in den Massen und Farben von catMass; die Laenge reicht bis
// zum Gelenk, an dem das Glied weitergeht (Oberarm - Drehknochen - Unterarm - Hand), an den Enden die Laenge aus der Tabelle.
// `griff` der Form ist der FK-Griff des Knochens (Rig::griffVon), -1 ohne Rig.
[[nodiscard]] RigFormen rigFormenSkelettCat(const GlaAnimation& g, const Rig& rig);

// Die Griffe des Rigs im Stil. ControlRig: jeder Griff hat seine Form. Cat: Fussplatten, Hand-IK-Ziele, Pole, Fussrolle,
// Saebelgriff und die Bodenform - die FK-Griffe sind die Kaesten des Skeletts (rigFormenSkelettCat). Blender: Kreise um Becken
// und Wirbel, Quadrate an Hand und Fuss, Kreise als Pole, der Bodenkreis - die uebrigen FK-Griffe sind die Oktaeder der Knochen.
// Standard: leer. `in` (darf nullptr sein): das Modell fuer die Weite der Ringe.
[[nodiscard]] RigFormen rigFormenGriffe(const GlaAnimation& g, const Rig& rig, const Inhalt* in, int stufe, RigStil stil);

}  // namespace bhed::modell
#endif
