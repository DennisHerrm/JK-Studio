// modell/detailstufen.h - Detailstufen (LOD) eines Modells erzeugen, zaehlen und pruefen (CW-MODELL, 15.11.2026)
//
// Clone-Wars-Kette Teil 3 (Liste des Bruders, Abschnitte 2, 7, 8): drei Detailstufen fuer Figuren, Waffen und Ausruestung,
// .md3 als name_1.md3 / name_2.md3 neben name.md3, .glm mit den Stufen in der Datei. Vorbild der Bedienung: 3ds Max ProOptimizer
// und MultiRes, Blender Decimate (Collapse).
//
// Verfahren: Kantenkollaps nach der Quadric Error Metric (M. Garland, P. Heckbert: "Surface Simplification Using Quadric Error
// Metrics", SIGGRAPH 1997) mit Attributen (H. Hoppe: "New Quadric Metric for Simplifying Meshes with Appearance Attributes",
// 1999) aus meshoptimizer (third_party/meshoptimizer, Arseny Kapoulkine, MIT, v1.3: meshopt_simplifyWithAttributes):
//
//   Ecken bleiben    jede Ecke einer groeberen Stufe ist eine Ecke der feinsten (Kollaps auf ein Ende der Kante) - Ort, Normale,
//                    UV, Knochen und Gewichte bleiben Bit fuer Bit die der Datei; eine .md3 behaelt so alle Bilder (jedes Bild
//                    nimmt den Ort seiner Ecke), eine .glm hoechstens vier Gewichte je Ecke mit Summe 1 wie gelesen
//   Attribute        Normale, UV (Gewicht nach der mittleren UV-Dichte der Flaeche) und die Knochengewichte der Flaeche (je
//                    benutzter Knochen ein Wert, hoechstens 27) gehen in den Fehler ein
//   Naehte           Ecken am selben Ort mit anderer UV oder Normale (die Datei teilt sie dort) kollabieren nur entlang der Naht
//   Raender          die Raender einer Flaeche bleiben, wo sie sind (meshopt_SimplifyLockBorder): Flaechen einer Figur stossen an
//                    ihren Raendern aneinander, ein Rand, der sich mitvereinfacht, reisst dort auf
//   unberuehrt       Tags/Bolts (G2SURFACEFLAG_ISBOLT oder "*name"), Flaechen mit "_off" am Ende und mit "_cap_" im Namen:
//                    in jeder Stufe dieselbe Flaeche wie in Stufe 0
//   gleiche Bytes    keine Zufaelle, keine Faeden: dieselbe Eingabe gibt dieselben Stufen
//
// Grenzen wie im Spiel (je Flaeche): SHADER_MAX_VERTEXES des Zielspiels (bhed/modell/g2.h grenzen()), in der letzten Stufe einer
// .glm die Schattengrenze SHADER_MAX_VERTEXES / 2 (schattenEckenGrenze: RB_RenderSurfaces nimmt fuer den Stencil-Schatten einer
// groesseren Flaeche die der letzten Stufe, tr_ghoul2.cpp:2307). Eine .md3 hat hoechstens drei Stufen (MD3_MAX_LODS,
// qfiles.h:100); RE_RegisterModel laedt name_2.md3 nur mit name_1.md3 (sonst ERR_DROP, tr_model.cpp:650-653).
//
// Mit den Vorgaben aendert nichts hier ein Modell: erzeugt wird nur auf Aufruf.
#ifndef BHED_MODELL_DETAILSTUFEN_H
#define BHED_MODELL_DETAILSTUFEN_H

#include <cstdint>
#include <string>
#include <vector>

#include "bhed/md3.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/md3datei.h"

namespace bhed::modell {

// MD3_MAX_LODS (qfiles.h:100); eine .glm traegt beliebig viele, JKA hat 3 oder 4 (gemessen, NOTIZEN_cwmodell_1511.md)
inline constexpr int kMd3StufenMax = 3;
inline constexpr int kGlmStufenMax = 4;

// Was eine Flaeche beim Erzeugen ist.
enum class StufenRolle : std::uint8_t {
    Vereinfachen = 0,
    Tag,        // Bolt: G2SURFACEFLAG_ISBOLT oder "*name"
    Aus,        // "_off" am Ende (Kappen und Varianten, ab Werk abgeschaltet)
    Kappe,      // "_cap_" im Namen
    Leer,       // ohne Dreiecke
};
[[nodiscard]] StufenRolle stufenRolle(const Teil& t);

// Wofuer das Modell ist - fuer die Vorgaben der Anteile und den Richtwert der Clone-Wars-Kette.
enum class ModellArt : std::uint8_t { Figur = 0, Ausruestung, Requisit };
inline constexpr int kModellArten = 3;
// models/players/... oder eine .glm an einem Skelett mit mehr als einem Knochen: Figur (auch Fahrzeuge und Droiden auf eigenem
// Skelett); models/weapons..., models/items/... oder eine .glm an *default: Ausruestung; sonst Requisit.
[[nodiscard]] ModellArt modellArtVon(const Inhalt& in, const std::string& pfad);

// Die Vorgabe der Anteile (Ecken der Stufe / Ecken von Stufe 0) je Art - die Mediane der Spieldateien von Jedi Academy
// (assets0-3.pk3, gemessen mit Y:\werkzeuge\cwmodell\lod_verhaeltnis.py und md3_lod.py): Figuren auf _humanoid 0,67 / 0,41
// (51 Modelle; mit vier Stufen 0,74 / 0,43 / 0,28), Waffen (.glm) 0,56 / 0,31, .md3 mit Stufen 0,75 / 0,49.
struct StufenVorgabe {
    float anteil[kGlmStufenMax - 1];
};
[[nodiscard]] StufenVorgabe stufenVorgabe(ModellArt a, int stufen);

// Clone-Wars-Kette (nur mit optClonewars): hoechstens so viele gezeichnete Ecken in der letzten Stufe ("niedrigste Stufe nur
// wenige hundert Ecken je Figur", Abschnitt 8) - Figur 500, Ausruestung 300, Requisit kein Richtwert (0).
[[nodiscard]] int cwRichtwert(ModellArt a) noexcept;

struct DetailOptionen {
    int stufen = 3;                    // Stufen samt Stufe 0 (2..4 bei .glm, 2..3 bei .md3)
    float anteil[kGlmStufenMax - 1] = {0.65F, 0.40F, 0.28F};
    // Gewichte der Attribute gegen den Ort (meshoptimizer: eine Aenderung um 1/Gewicht wiegt wie eine Verschiebung um die Laenge der
    // Kante). Vorgabe 0,1: an model.glm (_humanoid, 4 Stufen) bei den Eckenzahlen der Datei der kleinste Abstand zur Flaeche (Stufe 1
    // groesster 0,49 / mittel 0,04 gegen 2,32 / 0,11 der Datei, Stufe 2 1,63 / 0,24 gegen 2,23 / 0,22; mit 1 je Attribut 0,84 / 0,06
    // und 2,58 / 0,32) - die Naehte bleiben ohnehin (Topologie), die Gewichte bleiben die der Datei (Ecken bleiben).
    float gewichtNormale = 0.1F;
    float gewichtUv = 0.1F;            // Faktor auf das Gewicht aus der UV-Dichte
    float gewichtKnochen = 0.1F;       // Knochengewichte (0..1 wie Normalen)
    bool raenderHalten = true;
    int eckenGrenze = 0;               // SHADER_MAX_VERTEXES des Zielspiels (0: keine)
    int schattenGrenze = 0;            // nur letzte Stufe einer .glm (0: keine)
    int richtwertLetzte = 0;           // Clone-Wars-Richtwert fuer die ganze letzte Stufe (0: keiner)
};

// Zahlen einer Stufe: gezeichnete Ecken und Dreiecke (ohne Tags und ohne ab Werk abgeschaltete Flaechen - was das Spiel ohne Haut
// zeichnet), Teilflaechen (ohne Tags), die groesste Teilflaeche.
struct StufenZahlen {
    int ecken = 0;
    int dreiecke = 0;
    int flaechen = 0;
    int groesste = 0;
    std::string groessteName;
};
[[nodiscard]] StufenZahlen stufenZahlen(const Inhalt& in, int stufe);
[[nodiscard]] StufenZahlen stufenZahlen(const Md3Model& m);

// Eine Flaeche, die eine Grenze nicht erreicht hat (die Raender halten mehr Ecken als die Grenze erlaubt).
struct StufenRest {
    int teil = -1;
    int stufe = 0;
    int ecken = 0;
    int ziel = 0;
};

struct DetailBericht {
    std::vector<StufenZahlen> vorher;   // je Stufe des Dokuments vor dem Erzeugen
    std::vector<StufenZahlen> nachher;  // je neuer Stufe
    int unberuehrt = 0;                 // Flaechen, die in jeder Stufe gleich bleiben (Tags, _off, _cap_)
    std::vector<StufenRest> verfehlt;   // Ziel nicht erreicht (teil -1: das Ziel der ganzen Stufe)
    float abweichung = 0.0F;            // groesster Abstand einer Ecke von Stufe 0 zu ihrer Flaeche in einer Stufe (Einheiten)
};

// Ein Netz auf hoechstens `zielEcken` Ecken. Die Ecken des Ergebnisses sind Kopien von Ecken des Eingangs in ihrer Reihenfolge
// (samt der Felder der Datei), die Dreiecke in der Folge des Vereinfachers. Erreicht der Vereinfacher das Ziel nicht (Raender,
// Naehte), das kleinste erreichbare Netz. `fehler`: groesster Abstand einer Ecke des Eingangs zum Ergebnis (Einheiten).
struct VereinfachOptionen {
    int zielEcken = 0;
    float gewichtNormale = 0.1F;
    float gewichtUv = 0.1F;
    float gewichtKnochen = 0.1F;
    bool raenderHalten = true;
};
[[nodiscard]] Netz netzVereinfachen(const Netz& n, const VereinfachOptionen& o, float* fehler = nullptr);

// Wie weit eine Stufe von der Quelle abweicht: der groesste Abstand einer Ecke der Quelle zu den Dreiecken des Ergebnisses
// (Einheiten des Spiels), `mittel` der mittlere.
[[nodiscard]] double netzAbweichung(const Netz& quelle, const Netz& ergebnis, double* mittel = nullptr);

// Alle Stufen neu: Stufe 0 bleibt, die Stufen 1.. je aus Stufe 0, unberuehrte Flaechen in jeder Stufe gleich Stufe 0;
// in.stufenZahl = o.stufen (begrenzt). Die vereinfachten Flaechen rechnen GEMEINSAM (eine Kante an der Grenze zweier Flaechen
// kollabiert in beiden, ihre Raender bleiben aufeinander; Orte, die eine unberuehrte Flaeche mitbenutzt, bleiben); das Ziel gilt
// fuer die ganze Stufe (der Fehler entscheidet, wo Ecken wegfallen). Danach je Flaeche die Grenzen. Gibt false ohne Teile.
bool detailstufenErzeugen(Inhalt& in, const DetailOptionen& o, DetailBericht* bericht = nullptr);

// Nur Stufe 0 behalten (zurueck auf eine Stufe).
void detailstufenEntfernen(Inhalt& in);

// RE_RegisterModel (tr_model.cpp:640-648): "name.md3" -> "name_1.md3" (alles ab dem letzten Punkt weg, "_<stufe>.md3" dazu).
[[nodiscard]] std::string md3StufenPfad(const std::string& pfad, int stufe);

// Die .md3 einer Stufe >= 1 (Bilder, Tags und Kopf wie die gelesene Datei, die Flaechen aus der Stufe). false wie md3Schreiben.
[[nodiscard]] bool md3StufeSchreiben(const Inhalt& in, int stufe, std::string& aus, int eckenGrenze, Md3SchreibBericht* bericht = nullptr);

// Befunde der Stufen gegen Grenzen und Richtwert (fuer die Anzeige je Stufe und die Pruefung vor dem Speichern).
enum class StufenBefundArt : std::uint8_t {
    Ecken,          // Teilflaeche ueber SHADER_MAX_VERTEXES (ERR_DROP)
    Schatten,       // letzte Stufe: Teilflaeche ab SHADER_MAX_VERTEXES / 2 (Stencil-Schatten der Flaeche fehlt bzw. laeuft ueber)
    Md3Ecken,       // .md3: Teilflaeche ueber MD3_MAX_VERTS 4096
    Md3Flaechen,    // .md3: mehr als MD3_MAX_SURFACES 64 Teilflaechen
    Richtwert,      // Clone-Wars-Kette: letzte Stufe ueber dem Richtwert
    KeineStufen,    // Clone-Wars-Kette: Figur/Ausruestung mit nur einer Stufe
};
struct StufenBefund {
    StufenBefundArt art = StufenBefundArt::Ecken;
    int stufe = 0;
    int wert = 0;
    int grenze = 0;
    std::string name;
};
[[nodiscard]] std::vector<StufenBefund> stufenPruefen(const Inhalt& in, int eckenGrenze, int schattenGrenze, int richtwert);
// Dasselbe fuer eine Liste von Stufen-Zahlen (die Dateien name.md3, name_1.md3, ... von der Platte oder aus einem Archiv).
[[nodiscard]] std::vector<StufenBefund> stufenPruefen(const std::vector<Md3Model>& stufen, int eckenGrenze, int richtwert);

// MD3_MAX_VERTS / MD3_MAX_SURFACES (qfiles.h:101-105)
inline constexpr int kMd3MaxEcken = 4096;
inline constexpr int kMd3MaxFlaechen = 64;

}  // namespace bhed::modell
#endif
