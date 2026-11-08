// gpudraw.h - aus Stapeln eine Liste von Zeichenaufrufen machen
//
// Die Aufteilung
// --------------
// Ein Weg ueber die Grafikkarte zerfaellt in zwei Teile:
//
//   WAS gezeichnet wird - welche Stapel, in welcher Reihenfolge, mit welchem
//   Zustand, und welche davon sich zusammenfassen lassen.
//
//   WIE es hingeht - Puffer anlegen, Shader uebersetzen, Zustaende setzen,
//   DrawIndexed rufen.
//
// Der zweite Teil braucht Direct3D und laesst sich auf diesem Rechner nicht
// pruefen. Der erste ist reine Datenverarbeitung - und dort sitzen die
// Fehler, die man spaeter im Bild sucht und nicht findet: eine falsche
// Reihenfolge, ein vergessener Zustandswechsel, ein Stapel der zweimal
// gezeichnet wird.
//
// Deshalb steht der erste Teil hier, getrennt und geprueft
// (tests/gpudraw.cpp).
//
// Die Reihenfolge ist das Wichtigste
// ----------------------------------
// Deckende Flaechen zuerst, gemischte danach - und die gemischten von hinten
// nach vorn. Wer das vertauscht, bekommt Rauch, der hinter Felsen liegt, oder
// Lava, die durch Waende scheint. Genau diese Klasse von Fehlern hat in
// diesem Projekt mehrere Runden gekostet (rc377: der Teilchenpuffer hatte die
// Szenentiefe nie).
#ifndef BHED_GPUDRAW_H
#define BHED_GPUDRAW_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "bhed/bspgeo.h"
#include "bhed/gpustate.h"
#include "bhed/mapview.h"

namespace bhed::gpu {

// Ein Zeichenaufruf: ein Zustand, ein Bereich im Indexpuffer, zwei Texturen.
struct DrawCall {
    PipelineState state;
    std::uint32_t firstIndex = 0;
    std::uint32_t numIndexes = 0;
    // Platz im Texturensatz. -1 heisst: keine.
    int bild = -1;
    int lightmap = -1;
    // Der Stapel, aus dem er stammt - fuer Protokolle und zum Nachschlagen
    // der tcMod-Werte.
    std::size_t batch = 0;
    // --- Eine Stufe UEBER der ersten? ---------------------------------
    //
    // Ein JKA-Shader hat mehrere Stufen: Grundbild, Lightmap, und darueber
    // das additive Leuchten oder ein zweites Bild. Die Engine geht sie der
    // Reihe nach durch (tr_shade.cpp:1624) und ueberspringt im
    // Gluehdurchgang die ohne `glow` - Stufe fuer Stufe, nicht Flaeche fuer
    // Flaeche.
    //
    // Der GPU-Weg zeichnete bis rc435 nur die ERSTE Stufe. Das additive
    // Leuchten von `table_blue` und die vier- bis fuenfstufigen Bildschirme
    // sitzen in den Stufen darueber; deshalb fand der Gluehdurchgang nur
    // zwei Aufrufe, waehrend der Rasterer 61529 Bildpunkte gluehen liess.
    //
    // `bild` zeigt dann auf den EIGENEN Platz der Stufe, nicht auf den des
    // Stapels - genau wie `texId = jobs[jobIdx].stage` im Rasterer
    // (mapview.cpp:1396).
    bool zusatzstufe = false;
    // Ist die GRUNDSTUFE des Shaders deckend? Die Engine zeichnet alle Stufen eines Shaders
    // nacheinander (RB_IterateStagesGeneric) an der Stelle, die die Sortierstufe des Shaders
    // vorgibt (R_RadixSort, SS_OPAQUE vor SS_BLEND0) - eine gemischte Zusatzstufe eines deckenden
    // Shaders gehoert also zu den deckenden, vor jede durchscheinende Flaeche davor.
    bool grundDeckend = false;
};

// Baut die Liste fuer ein Bild.
//
// `zeitSekunden` geht in die zeitabhaengigen Wellen. Stapel ohne brauchbare
// Textur werden UEBERSPRUNGEN, nicht mit einer Ersatztextur gezeichnet - ein
// fehlendes Bild soll auffallen, nicht wie Lava aussehen (das war der
// Fehlschlag aus rc368).
[[nodiscard]] std::vector<DrawCall> buildDrawCalls(const BspMesh& mesh,
                                                   const TextureSet* textures,
                                                   float zeitSekunden);

// Wie viele ZUSTANDSWECHSEL enthaelt die Liste? Aufeinanderfolgende Aufrufe
// mit gleichem Zustand kosten keinen.
//
// Das ist die Zahl, um die es beim Umbau geht: gemessen kommen je Bild nie
// mehr als 24 verschiedene Zustaende vor, aber nur wenn die Liste so sortiert
// ist, dass gleiche beieinanderliegen.
[[nodiscard]] std::size_t countStateChanges(const std::vector<DrawCall>& calls);

// Die Aufrufe, die GLUEHEN - aus einer fertigen Liste herausgefiltert.
//
// Der Gluehdurchgang zeichnet dieselben Stapel ein zweites Mal, aber nur die
// leuchtenden, in einen kleineren Puffer. Der Rasterer macht es genauso
// (`stufeGlueht`, mapview.cpp:1206).
//
// Getrennt und nicht als Schalter in buildDrawCalls, weil die Reihenfolge
// dieselbe bleiben muss: derselbe Stapel soll im Gluehdurchgang an derselben
// relativen Stelle stehen wie im Hauptdurchgang. Sonst leuchtet etwas an
// einer Stelle, an der es im Hauptbild verdeckt ist.
[[nodiscard]] std::vector<DrawCall> nurGluehende(
    const std::vector<DrawCall>& calls);

// --- deformVertexes autosprite/autosprite2 auf der CPU (REST3) -------------
//
// Das Spiel rechnet RB_DeformTessGeometry (code/rd-vanilla/tr_shade_calc.cpp:549, MD9 gleich) auf der CPU ueber tess: die
// Eintraege der Reihe nach, AutospriteDeform (:377) klappt je vier Ecken zu einem kamerazugewandten Plaettchen aus Mitte und
// Radius der schon VERFORMTEN Ecken (RB_AddQuadStampExt, tr_surface.cpp:77: Normale -axis[0] der Welt, st und Lightmap-st 0..1,
// Farbe der ersten Ecke, Wicklung 0,1,3 / 3,1,2), Autosprite2Deform (:444) dreht um die lange Achse mit sqrt und VectorNormalize.
// Beides braucht alle vier Ecken und genaue Wurzeln; der Vertex-Shader sieht eine Ecke, und sqrt/Division sind in D3D11 nicht
// genau gerundet. Deshalb rechnet die Vorschau Flaechen mit autosprite oder autosprite2 hier mit denselben float-Ausdruecken wie
// das Spiel, und der Vertex-Shader reicht die Ecken nur durch (gpumap_win32.cpp, zeichneKarte; die Merkmale kAutosprite und
// kAutosprite2 tragen dann keine Verformung, gpustate.cpp). Bis REST3 drehte der Vertex-Shader die beim Hochladen vorbereiteten
// Ecken (baueAutosprites/baueAutosprites2): wave/bulge vor autosprite, wave/bulge/normal an autosprite2 und ein zweites autosprite
// fehlten, autosprite2 war nicht bitgleich (Achse normiert beim Hochladen, Startwert 999999 statt WORLD_SIZE), an Movern stand die
// Normale des Plaettchens im Raum des Modells, im Spiegel fehlte left = -left, die Ecken folgten dem ersten Auftreten in den
// Indizes statt der Reihenfolge der Flaeche.
struct SpriteSicht {
    float links[3]{};      // backEnd.viewParms.ori.axis[1], an einer Entity im Raum des Modells (GlobalVectorToLocal :363)
    float hoch[3]{};       // axis[2], ebenso
    float vorn[3]{};       // axis[0], ebenso (Autosprite2Deform :457)
    float vornWelt[3]{};   // axis[0] in der Welt: RB_AddQuadStampExt (tr_surface.cpp:112) nimmt sie ungedreht als Normale
    bool spiegel = false;  // viewParms.isMirror: left = -left (AutospriteDeform :400)
};
struct TessEcke {
    float xyz[3]{};
    float normal[3]{};
    float st[2]{};
    float lm[2]{};
    std::uint8_t farbe[4]{};
};
// Eine Flaeche (ein Abschnitt von tess): `ecken` in der Reihenfolge der Flaeche, `idx` lokal (0..n-1). Danach stehen Ecken und
// Indizes wie in tess nach RB_DeformTessGeometry. false: AutospriteDeform bricht im Spiel ab (ERR_DROP: Eckenzahl nicht durch vier
// teilbar oder nicht sechs Indizes je Viereck) - der Aufrufer zeichnet die Flaeche dann unverformt. Ein Eintrag, an dem
// TableForFunc abbricht (gpu::deformLage Abbruch), bleibt aus wie im Vertex-Shader.
[[nodiscard]] bool verformeTess(const DeformZusatz& z, float zeitSekunden, const SpriteSicht& sicht, std::vector<TessEcke>& ecken,
                                std::vector<std::uint32_t>& idx);

// --- Die Lage: deckend oder gemischt -----------------------------------
//
// `buildDrawCalls` sortiert deckend vor gemischt - aber nur INNERHALB EINES
// NETZES. Der GPU-Weg zeichnet nacheinander Karte, Effekte, Mover, Figuren;
// jeder Durchgang ist in sich richtig sortiert, und trotzdem landen die
// gemischten Kartenflaechen vor den deckenden Tueren. Genau das ist der
// Fehler "Tueren verschwinden hinter durchsichtigen Flaechen".
//
// Die Engine hat dieses Problem nicht, weil sie gar nicht nach Kategorien
// zeichnet: `R_AddDrawSurf` legt Weltflaechen, Mover und Figuren in EINE
// Liste, gibt jedem Eintrag einen Sortierschluessel aus der Sortierstufe des
// Shaders (SS_OPAQUE, SS_DECAL, SS_SEE_THROUGH, SS_BLEND0 ...) und sortiert
// einmal ueber alles (`R_RadixSort`, tr_main.cpp).
//
// Diesen Schluessel hier nachzubauen hiesse, den GPU-Weg von Grund auf
// umzustellen: jeder Aufruf muesste seine Weltmatrix mitfuehren, statt sie
// vor dem Zeichnen in die Kameramatrix zu rechnen. Das ist die richtige
// Bauform, aber nicht in einem Zug auf einem Rechner, auf dem man das Bild
// nicht sehen kann.
//
// Deshalb der Schritt davor, der denselben Fehler behebt: das BILD laeuft
// zweimal ueber alle Quellen - erst alles Deckende, dann alles Gemischte.
// Das ist derselbe Sortierschluessel, nur mit zwei Stufen statt acht, und es
// braucht keine einzige Zeile im Direct3D-Teil.
//
// Die Reihenfolge INNERHALB einer Lage bleibt unangetastet - bei den
// gemischten haengt das Bild daran (siehe oben).
enum class Lage : std::uint8_t {
    Alles,      // wie bisher - fuer den Gluehdurchgang und zum Vergleichen
    Deckend,
    Gemischt,
};

[[nodiscard]] std::vector<DrawCall> nurLage(const std::vector<DrawCall>& calls,
                                            Lage lage);

}  // namespace bhed::gpu

#endif  // BHED_GPUDRAW_H
