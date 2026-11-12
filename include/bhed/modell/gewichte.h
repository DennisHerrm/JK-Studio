// modell/gewichte.h - Gewichte malen und fuers Spiel festlegen (Modell-Block 1a-2)
//
// Waehrend des Malens traegt jede Ecke eine eigene Liste (Netz::mal) mit
// beliebig vielen Knochen - wie die Vertexgruppen in Blender oder die
// Huellen des Skin-Modifiers in 3ds Max. Das Spiel kann das nicht: eine Ecke
// der .glm traegt hoechstens vier Knochen (iMAX_G2_BONEWEIGHTS_PER_VERT,
// OpenJK code/rd-common/mdx_format.h:85), gepackt in mdxmVertex_t:
//
//   uiNmWeightsAndBoneIndexes  Bit 30-31 Anzahl - 1, Bit 0-19 je Gewicht 5 Bit
//                              Knochenverweis, Bit 20-27 die oberen zwei Bit
//                              jedes Gewichts (mdx_format.h:302-308)
//   BoneWeightings[4]          die unteren acht Bit
//
// G2_GetVertBoneWeight (mdx_format.h:334-352) liest ein Gewicht als
// fG2_BONEWEIGHT_RECIPROCAL_MULT * iTemp (1/1023, 10 Bit) und das LETZTE nicht
// aus der Datei, sondern als 1 - Summe der vorigen. Die Summe ist im Spiel
// also immer 1.
//
// fuersSpiel() macht aus einer Malliste genau das: die vier groessten
// Gewichte (Blender "Limit Total" 4), auf 1 normiert, absteigend sortiert
// (so liegen sie in den Raven-Dateien: gemessen an allen 77 Modellen unter
// models/players von JKA, 116729 von 117100 Ecken mit mehr als einem Gewicht
// absteigend, die uebrigen bei gleichen Werten), die ersten n-1 auf 10 Bit
// gerundet, das letzte als Rest - und gelesen wie G2_GetVertBoneWeight. Was in
// der Ecke steht, ist danach bitgleich das, womit das Spiel die Ecke verformt.
//
//   Ported from OpenJK (codemp/rd-vanilla/G2_misc.cpp, tr_ghoul2.cpp, mdx_format.h; Copyright (C) 1999-2005 Id Software,
//   Inc., 2000-2013 Raven Software, Inc., 2013-2015 OpenJK contributors) and Blender (paint_vertex_weight_utils,
//   brush falloff; Copyright (C) Blender Authors). Both are licensed under the GNU General Public License version 2 (Blender:
//   or later). This file is part of BehavEd-Remake and licensed under the GNU General Public License version 2 or later
//   (LICENSE).
#ifndef BHED_MODELL_GEWICHTE_H
#define BHED_MODELL_GEWICHTE_H

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "bhed/modell/dokument.h"

namespace bhed::modell {

// Ein Gewicht wie G2_GetVertBoneWeight: fG2_BONEWEIGHT_RECIPROCAL_MULT * iTemp
// (mit dem Kehrwert multipliziert, nicht geteilt - 24 der 1024 Werte waeren
// sonst um ein Bit anders).
[[nodiscard]] float g2Gewicht(int bits10) noexcept;

// Die Liste einer Ecke, wie die .glm sie traegt (nur Gewichte > 0).
[[nodiscard]] GewichtListe listeAus(const Ecke& e);

// Was die Datei fuer eine Ecke traegt: Anzahl, Knochen, die 10-Bit-Werte der
// ersten n-1 Gewichte und die Gewichte, wie das Spiel sie liest.
struct SpielGewichte {
    int zahl = 0;
    int knochen[4]{};
    int bits[4]{};
    float gewicht[4]{};
};
// false: keine Gewichte > 0 (die Ecke bleibt dann, wie sie ist).
[[nodiscard]] bool fuersSpiel(GewichtListe l, SpielGewichte& aus);
// Die Ecke bekommt die Gewichte des Spiels (Knochen 0..255).
bool aufEcke(const GewichtListe& l, Ecke& e);

// Die Mallisten eines Netzes anlegen (aus den Ecken), falls noch keine da sind.
void malAnlegen(Netz& n);
[[nodiscard]] inline bool malt(const Netz& n) noexcept { return !n.mal.empty(); }
// Ecken eines Netzes mit mehr als vier Gewichten in der Malliste.
[[nodiscard]] int eckenUeberVier(const Netz& n);
// Das Gewicht eines Knochens an einer Ecke (aus der Malliste, sonst aus der Ecke).
[[nodiscard]] float gewichtVon(const Netz& n, int ecke, int knochen);

enum class PinselArt : std::uint8_t { Hinzufuegen = 0, Abziehen, Glaetten, Setzen };
inline constexpr int kPinselArten = 4;

struct Tupfer {
    int ecke = -1;
    float staerke = 0.0F;   // Pinselstaerke mal Abfall (0..1)
};

// AP8 (W4, W12): was fuer jedes Werkzeug gilt.
//   gesperrt            je Knochen != 0: seine Gewichte aendert kein Pinsel und kein Befehl (Blender: gesperrte
//                       Vertexgruppe, DG_LOCK_WEIGHT). Der gesperrte Knochen selbst laesst sich nicht malen
//                       (wpaint_stroke_test_start: "Active group is locked, aborting").
//   autoNormalisieren   nach jeder Aenderung ist die Summe der Gewichte der Ecke 1 (Blender "Auto Normalize", 3ds Max
//                       Skin "Normalize"): der gemalte Knochen behaelt seinen Wert, die uebrigen freien teilen sich den
//                       Rest im Verhaeltnis (normalisierenNachMalen).
struct MalRegeln {
    std::vector<char> gesperrt;
    bool autoNormalisieren = false;
    [[nodiscard]] bool sperrt(int knochen) const noexcept {
        return knochen >= 0 && static_cast<std::size_t>(knochen) < gesperrt.size() && gesperrt[static_cast<std::size_t>(knochen)] != 0;
    }
};

// Ein Tupfer auf ein Netz (legt die Mallisten an):
//   Hinzufuegen  w + wert * s (hoechstens 1)      Blender "Add"
//   Abziehen     w - wert * s (mindestens 0)      Blender "Subtract"
//   Setzen       w + (wert - w) * s               Blender "Mix", Max "Set"
//   Glaetten     w + (Mittel der Nachbarn - w) * s, Nachbarn ueber die Punkte
//                (Ecken gleichen Orts zaehlen als eine), Werte von vor dem Tupfer
// Ecken desselben Punkts (UV-Naht) bekommen dasselbe. Rueckgabe: geaenderte Ecken.
int malen(Netz& n, const std::vector<Tupfer>& t, int knochen, PinselArt art, float wert, const MalRegeln& regeln = {});

// Die Abfallkurve des Pinsels: Blender "Smooth" (3p^2 - 2p^3, p = 1 - d/r).
[[nodiscard]] float abfall(float abstand, float radius) noexcept;

// Spiegeln ueber die Knochennamen: l_/r_ und _l/_r an Anfang und Ende, sonst
// ein einzelnes l/r am Anfang, wenn es den Partner im Skelett gibt (lhand <->
// rhand, lower_lumbar bleibt). -1: kein Partner (der Knochen bleibt).
[[nodiscard]] int spiegelKnochen(const std::vector<std::string>& namen, int knochen);

// Je Ecke einer Stufe ihr Spiegelpartner (x -> -x, Abstand <= toleranz) ueber
// alle Teile; {-1,-1} ohne Partner.
struct EckOrt {
    int teil = -1;
    int ecke = -1;
};
[[nodiscard]] std::vector<std::vector<EckOrt>> spiegelKarte(const Inhalt& in, int stufe, float toleranz);

// Die Gewichte der Seite `quelle` (+1: x > 0, -1: x < 0) auf die andere
// spiegeln (Knochen nach spiegelKnochen); Ecken naeher als `toleranz` an der
// Spiegelebene bleiben. Rueckgabe: beschriebene Ecken (siehe spiegelnSeite).
int spiegeln(Inhalt& in, int stufe, const std::vector<std::string>& knochenNamen, int quelle, float toleranz);

// Uebernehmen: jede Malliste aller Teile und Stufen nach fuersSpiel in die
// Ecken, die Mallisten verschwinden. Rueckgabe: Ecken.
int uebernehmen(Inhalt& in);

// FIX612B P2-2: traegt irgendeine Flaeche irgendeiner Stufe eine Malliste?
[[nodiscard]] bool malt(const Inhalt& in) noexcept;
// FIX612B P2-2: der Inhalt, wie die Ansicht ihn zeichnet (anzeigeVertex) und wie eine Datei ihn tragen kann - eine Kopie,
// in der jede Malliste uebernommen ist. Wer speichert oder die Grenzen des Spiels misst, nimmt diese Kopie: die Datei
// enthaelt dann, was man sieht, und das Dokument behaelt seine Mallisten.
[[nodiscard]] Inhalt wieGezeigt(const Inhalt& in);

// =====================================================================================================================
// AP8 (MBENTWURF Teil B, Abgleich W2-W6, W12): die Werkzeuge des Bereichs Weights wie 3ds Max (Skin: Weight Tool, Mirror
// Parameters; Autodesk-Hilfe "Weight Tool Dialog (Skin Modifier)", "Parameters Rollout (Skin Modifier)", "Mirror Parameters
// Rollout") und wie Blender 5.0 (Weight Paint: Smear, Gradient; Weights > Normalize All, Clean, Smooth, Limit Total, Mirror;
// Sperren). Die Rechnung der Blender-Befehle ist aus dem Quelltext uebertragen (object_vgroup.cc, deform.cc, mesh_mirror.cc,
// paint_weight.cc, paint_vertex_weight_ops.cc; GPL v2 oder spaeter). Die Autodesk-Hilfe nennt fuer "Blend" und fuer das
// Skalieren unter Normalisierung keine Formel - dort steht, was hier gerechnet wird.
//
// In JKA sind linker und rechter Arm, Rumpf und Kopf eigene Flaechen; an ihren Grenzen und an UV-Naehten liegen mehrere
// Ecken am selben Ort. Alle Befehle hier arbeiten deshalb auf PUNKTEN ueber alle Teile hinweg (Schweissnetz): Ecken
// gleichen Orts sind ein Punkt und bekommen dasselbe - sonst reisst das Netz an der Grenze, sobald es sich bewegt.

// Die Auswahl von Ecken einer Stufe: je Teil je Ecke != 0 (ein Teil ohne Eintrag oder mit zu kurzer Liste: nichts).
using EckenMaske = std::vector<std::vector<char>>;
[[nodiscard]] EckenMaske eckenMaske(const Inhalt& in, int stufe, const std::vector<EckOrt>& ecken);
[[nodiscard]] std::vector<EckOrt> eckenListe(const EckenMaske& m);

// Die Punkte einer Stufe ueber alle Teile (ohne Tags): Ecken mit bitgleichem Ort sind EIN Punkt.
struct Schweissnetz {
    std::vector<std::vector<int>> punktVon;     // je Teil je Ecke ihr Punkt (Tags und Teile ohne die Stufe: leer)
    std::vector<std::vector<EckOrt>> ecken;     // je Punkt seine Ecken
    std::vector<std::vector<int>> nachbarn;     // je Punkt die Punkte, mit denen er eine Dreieckskante teilt (aufsteigend)
    [[nodiscard]] int zahl() const noexcept { return static_cast<int>(ecken.size()); }
    [[nodiscard]] const float* ort(const Inhalt& in, int stufe, int punkt) const;
};
[[nodiscard]] Schweissnetz schweissen(const Inhalt& in, int stufe);
// Die Punkte einer Auswahl (ein Punkt ist gewaehlt, sobald eine seiner Ecken es ist) und zurueck (alle Ecken der Punkte).
[[nodiscard]] std::vector<char> punktWahl(const Schweissnetz& s, const EckenMaske& m);
[[nodiscard]] EckenMaske eckenVonPunkten(const Inhalt& in, int stufe, const Schweissnetz& s, const std::vector<char>& punkte);

// --- Normalisieren mit Sperren (Blender paint_weight.cc) ----------------------------------------------------------------
// do_weight_paint_normalize_all_locked: die Summe wird 1, geaendert werden nur freie Knochen; gesperrte zaehlen mit.
// false: die Sperren lassen es nicht zu (kein freier Knochen an der Ecke, oder die gesperrten tragen schon 1 - dann werden
// die freien 0).
bool normalisierenGesperrt(GewichtListe& l, const std::vector<char>& gesperrt);
// do_weight_paint_normalize_all_locked_try_active: erst bleibt auch der eben gemalte Knochen `aktiv` stehen und die
// uebrigen freien teilen sich den Rest; geht das nicht (er ist der einzige freie), wird er selbst angepasst.
void normalisierenNachMalen(GewichtListe& l, int aktiv, const std::vector<char>& gesperrt);

// --- Pinsel wie Blender ----------------------------------------------------------------------------------------------------
// Verwischen (Blender "Smear", do_wpaint_brush_smear): jede getupfte Ecke nimmt das Gewicht des Nachbarn (ueber ihre
// Dreiecke), der am genauesten ENTGEGEN der Pinselbewegung liegt - das Gewicht wandert mit dem Pinsel. `ebene` je Ecke: ihr
// Ort in der Bildebene (zwei Achsen der Ansicht, ohne Perspektive: Blender nimmt die Richtungen ohne ihren Anteil laengs
// der Blickrichtung), `richtung` die Bewegung des Pinsels in denselben Achsen. Ohne Bewegung oder ohne Nachbarn in dieser
// Richtung bleibt die Ecke. neu = w + (Gewicht des Nachbarn - w) * s.
int verwischen(Netz& n, const std::vector<Tupfer>& t, int knochen, const std::vector<std::array<float, 2>>& ebene, const float richtung[2],
               const MalRegeln& regeln = {});

// Verlauf (Blender paint.weight_gradient, gradientVert_update): eine Strecke im Bild von `start` nach `ende`. Linear: am
// Anfang und davor wirkt der Pinsel ganz, zum Ende hin faellt er nach der Pinselkurve (abfall) auf 0, dahinter bleibt die
// Ecke. Radial: dasselbe mit dem Abstand zum Anfang. neu = Mischung(vorher, wert, Abfall * staerke) mit der Mischung des
// Pinsels (Hinzufuegen, Abziehen, sonst Setzen). `schirm` je Ecke ihr Ort im Bild, `sichtbar` je Ecke != 0 (verdeckte Teile
// und Ecken ausserhalb des Bilds bleiben). Rueckgabe: geaenderte Ecken.
enum class VerlaufArt : std::uint8_t { Linear = 0, Radial };
int verlauf(Netz& n, const std::vector<std::array<float, 2>>& schirm, const std::vector<char>& sichtbar, const float start[2], const float ende[2],
            VerlaufArt art, int knochen, PinselArt mischung, float wert, float staerke, const MalRegeln& regeln = {});

// --- Gewichts-Werkzeug wie 3ds Max (Weight Tool) -------------------------------------------------------------------------
// Auswahl erweitern (Skin: Grow, Shrink, Ring, Loop) - ueber die Punkte aller Teile.
//   Wachsen     dazu kommt jede Ecke, die ueber eine Kante an einer gewaehlten haengt
//   Schrumpfen  weg faellt jede gewaehlte Ecke mit einem nicht gewaehlten Nachbarn (alle gewaehlt: nichts geschieht)
//   Ring        "all vertices in parallel edges": die Kanten zwischen zwei gewaehlten Ecken, je Kante der Kantenring
//   Schleife    "all vertices in continuing edges": je Kante die Kantenschleife
// Ring und Schleife laufen ueber Vierecke (Netz::polyVon aus dem Modellieren; ein Netz aus Dreiecken wird dafuer wie mit
// "Zu Vierecken" gepaart, ohne das Dokument zu aendern). Weniger als zwei benachbarte gewaehlte Ecken: die Auswahl bleibt.
[[nodiscard]] std::vector<EckOrt> eckenWachsen(const Inhalt& in, int stufe, const std::vector<EckOrt>& wahl);
[[nodiscard]] std::vector<EckOrt> eckenSchrumpfen(const Inhalt& in, int stufe, const std::vector<EckOrt>& wahl);
[[nodiscard]] std::vector<EckOrt> eckenRing(const Inhalt& in, int stufe, const std::vector<EckOrt>& wahl);
[[nodiscard]] std::vector<EckOrt> eckenSchleife(const Inhalt& in, int stufe, const std::vector<EckOrt>& wahl);

// "Set Weight" / die Festwerte: "Assigns an absolute weight value between 0 and 1 for the active bone to the selected
// vertices"; "Scale Weight": "Multiplies each selected vertex's weight value by the field value" (Vorgabe 0.95, hoechstens
// 1). Rueckgabe: geaenderte Ecken.
int gewichtSetzen(Inhalt& in, int stufe, const EckenMaske& wahl, int knochen, float wert, const MalRegeln& regeln = {});
int gewichtSkalieren(Inhalt& in, int stufe, const EckenMaske& wahl, int knochen, float faktor, const MalRegeln& regeln = {});

// --- Bereinigen wie Blender (Menue Weights) ------------------------------------------------------------------------------
// Jeder Befehl gilt den gewaehlten Ecken (`wahl` nullptr: allen) der Stufe; Rueckgabe: geaenderte Ecken.
// Normalize All (vertex_group_normalize_all, BKE_defvert_normalize_ex): Summe 1 je Ecke; gesperrte Knochen bleiben, mit
// aktivHalten ("Lock Active", Vorgabe an) auch der Knochen `aktiv`. Sind alle Knochen gesperrt, geschieht nichts (-1).
int normalisierenAlle(Inhalt& in, int stufe, const EckenMaske* wahl, int aktiv, bool aktivHalten, const MalRegeln& regeln = {});
// Clean (vertex_group_clean): Gewichte <= grenze fallen weg (Vorgabe 0; 3ds Max "Remove Zero Weights" mit "Remove Zero
// Limit"); einenBehalten ("Keep Single"): die letzte Zuordnung einer Ecke bleibt. Gesperrte Knochen bleiben (Blender
// prueft das hier nicht - eine Sperre soll halten). Rueckgabe: entfernte Gewichte.
int saeubern(Inhalt& in, int stufe, const EckenMaske* wahl, float grenze, bool einenBehalten, const MalRegeln& regeln = {});
// Smooth (vertex_group_smooth, vgroup_smooth_subset mit Expand 0): je freiem Knochen neu = w * (1 - faktor) + Mittel der
// Nachbarn * faktor, `wiederholungen` Mal; gelesen werden alle Nachbarn, geschrieben nur gewaehlte. 3ds Max "Blend"
// ("smooth out the transition between them and their surrounding vertices") ist derselbe Schritt mit faktor 0,5.
int glaettenAlle(Inhalt& in, int stufe, const EckenMaske* wahl, float faktor, int wiederholungen, const MalRegeln& regeln = {});
// Limit Total (vertex_group_limit_total): hoechstens `grenze` Knochen je Ecke, die kleinsten fallen weg (bei gleichem
// Gewicht bleibt der kleinere Knochen). Rueckgabe: entfernte Gewichte.
int begrenzen(Inhalt& in, int stufe, const EckenMaske* wahl, int grenze, const MalRegeln& regeln = {});

// --- Spiegeln -------------------------------------------------------------------------------------------------------------
// Die Seite eines Knochens aus seinem Namen (wie spiegelKnochen): +1 links, -1 rechts, 0 keine. Ein einzelnes l/r am
// Anfang zaehlt nur mit Partner (lhand, aber nicht lower_lumbar).
[[nodiscard]] int knochenSeite(const std::vector<std::string>& namen, int knochen);
// Wo die linke Seite der Figur liegt: +1 bei x > 0, -1 bei x < 0, 0 ohne Befund - gemessen am Schwerpunkt der Gewichte
// linker gegen rechter Knochen.
[[nodiscard]] int linkeSeite(const Inhalt& in, int stufe, const std::vector<std::string>& namen);

// Je Punkt sein Spiegelpartner (x -> -x): der naechste Punkt am gespiegelten Ort mit Abstand <= toleranz; ein Punkt auf
// der Spiegelebene ist sein eigener Partner. -1: kein Gegenstueck.
[[nodiscard]] std::vector<int> spiegelPartner(const Inhalt& in, int stufe, const Schweissnetz& s, float toleranz);
// Dasselbe ohne Lage ueber die Netzstruktur (Blender "Topology Mirror", ED_mesh_mirrtopo_init): jeder Punkt bekommt aus
// seinen Kanten eine Kennzahl, die mit jeder Runde die der Nachbarn aufnimmt; zwei Punkte mit derselben Kennzahl sind ein
// Paar, ein Punkt mit einmaliger Kennzahl liegt in der Mitte. Braucht ein Netz, dessen Haelften gleich gebaut sind.
[[nodiscard]] std::vector<int> spiegelPartnerTopologie(const Schweissnetz& s);

struct SpiegelBefund {
    int knochenPaare = 0;        // gefundene Paare linker und rechter Knochen (ueber die Namen)
    int knochenPaareSoll = 0;    // dazu die Knochen, deren Name eine Seite traegt (l_/r_, _l/_r), aber keinen Partner hat
    int punkte = 0;
    int ohneGegenstueck = 0;     // Punkte ohne Partner
    int mitte = 0;               // Punkte, die ihr eigener Partner sind
    int eckenOhne = 0;           // Ecken an Punkten ohne Partner
    std::vector<std::pair<int, int>> teilPaare;   // Teile, deren Punkte einander spiegeln (kleinerer Index zuerst, je einmal)
};
[[nodiscard]] SpiegelBefund spiegelBefund(const std::vector<std::string>& namen, const Schweissnetz& s, const std::vector<int>& partner);

// Wie 3ds Max (Mirror Parameters): "Paste Blue to Green Verts" / "Paste Green to Blue Verts" kopieren die Gewichte einer
// Seite auf die andere (Knochen nach spiegelKnochen), Tauschen wechselt beide Seiten (Punkte der Mitte tauschen nur ihre
// Knochen l <-> r). quelle +1: die Seite x > 0 ist die Quelle, -1: x < 0, 0: tauschen. Ein Punkt mit |x| <= mitte gehoert
// zu keiner Seite. Jeder Punkt der Zielseite nimmt die Gewichte SEINES Partners (so bekommt jeder Punkt mit Gegenstueck
// genau eine Zuweisung, auch wenn das Netz nicht genau spiegelgleich ist). Mit `wahl` nur Paare, von denen ein Punkt
// gewaehlt ist ("Mirror Paste" der Auswahl). Rueckgabe: beschriebene Ecken.
int spiegelnSeite(Inhalt& in, int stufe, const std::vector<std::string>& namen, const Schweissnetz& s, const std::vector<int>& partner, int quelle,
                  float mitte, const EckenMaske* wahl = nullptr);

// Wie Blender (object.vertex_group_mirror, vgroup_mirror / dvert_mirror_op): je Paar - sind beide gewaehlt (ohne Auswahl:
// alle), tauschen sie; ist nur einer gewaehlt, bekommt er die Gewichte des anderen. gewichte: "Mirror Weights";
// namenTauschen: "Flip Group Names" (Knochen l <-> r); alleGruppen: "All Groups", sonst nur der Knochen `aktiv` (und beim
// Namentausch sein Partner). Punkte der Mitte bleiben, wie sie sind.
struct BlenderSpiegel {
    bool gewichte = true;
    bool namenTauschen = true;
    bool alleGruppen = false;
    int aktiv = -1;
};
struct SpiegelZahl {
    int gespiegelt = 0;    // Paare ("%d vertex weights mirrored")
    int ohnePartner = 0;   // Punkte ohne Gegenstueck ("%d vertices not found")
};
SpiegelZahl spiegelnBlender(Inhalt& in, int stufe, const std::vector<std::string>& namen, const Schweissnetz& s, const std::vector<int>& partner,
                            const BlenderSpiegel& o, const EckenMaske* wahl = nullptr);

// Spiegeln beim Malen wie Blender (X Mirror, do_weight_paint_vertex_single: "dw_mirr->weight = dw->weight"): nach einem
// Tupfer bekommt der Spiegelpartner jedes getupften Punkts am Spiegelknochen (`spiegelKn`, -1: derselbe Knochen) genau das
// Gewicht, das der getupfte Punkt jetzt am gemalten Knochen traegt - an ALLEN Ecken des Partnerpunkts. Ein Punkt der
// Mitte bekommt nichts doppelt (gleicher Knochen: nichts; Knochenpaar: beide Knochen gleich). Rueckgabe: geaenderte Ecken.
int spiegelMalen(Inhalt& in, int stufe, const Schweissnetz& s, const std::vector<int>& partner, const std::vector<std::vector<Tupfer>>& jeTeil,
                 int knochen, int spiegelKn, const MalRegeln& regeln = {});

}  // namespace bhed::modell
#endif
