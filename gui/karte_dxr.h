// karte_dxr.h - Karten-Editor, Echtzeit-Licht: Schatten per Raytracing (DXR 1.1) als zweite Schattenart neben den
// Schattenkarten (Build > Ray-traced shadows; eigene Funktion, nicht in NetRadiant-Custom).
//
// Nur unter Direct3D 12 und nur, wenn Grafikkarte und Treiber DXR 1.1 (RayQuery) und Shader Model 6.5 koennen - sonst nennt
// grund(), warum nicht, und es bleibt bei den Schattenkarten. Unter Direct3D 11 aendert sich nichts.
//
// Aufbau:
//   - Die Schattengeometrie (app_karte_echtlicht.cpp, schattenGeoBauen: Eimer zu 512 Einheiten je Art nach den Regeln von
//     q3map2) wird zur Beschleunigungsstruktur: je Eimer eine untere Struktur (BLAS), die bleibt, solange sich sein Inhalt
//     nicht aendert; darueber eine obere (TLAS) mit einer Instanz je Eimer, Maske nach der Art. Ein Eimer, der sich in kurzer
//     Folge aendert (ein Pinsel wird gezogen), wird auf schnellen Bau gestellt und nach einer Ruhezeit einmal auf schnelle
//     Strahlen neu gebaut.
//   - Der Shader des Echtzeit-Lichts ist derselbe Text (kShader4, gui/karte_gpu_win32.cpp); an der Stelle der Schattenkarten
//     ruft er dxrSicht (gui/karte_dxr.hlsl). Der Bau uebersetzt ihn mit dxc aus dem Windows SDK zu DXIL
//     (tools/dxr/dxr_quelle.cpp, CMakeLists.txt). Ohne dxc beim Bau gibt es die Schattenart nicht (grund() == KeinShader).
//   - Gezeichnet wird ueber den Abgeber wie alles andere (gui/abgeber12.h): die DXIL-Shader als gewoehnliche Shader-Objekte,
//     die TLAS als Sicht an t11 (abgeber::strukturSicht), der Bau der Strukturen in rahmen::liste().
#ifndef BHED_GUI_KARTE_DXR_H
#define BHED_GUI_KARTE_DXR_H

#include "karte_gpu.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bhed::gui::kartegpu::dxr {

// Warum die Raytracing-Schatten nicht gehen (der Menuepunkt ist dann ausgegraut und nennt es). Keiner: sie gehen.
enum class Grund {
    Keiner,
    NichtD3d12,   // es laeuft Direct3D 11 (oder OpenGL)
    KeinDxr,      // Grafikkarte oder Treiber ohne DXR 1.1 (RayQuery) bzw. ohne Shader Model 6.5
    KeinShader,   // dieses Programm wurde ohne dxc gebaut (der DXIL-Shader fehlt)
    Fehler,       // das Geraet nimmt Shader oder Strukturen nicht (Einzelheiten im Protokoll)
};
// Einmal je Grafikgeraet geprueft (beim ersten Aufruf, also beim Start des Karten-Editors) und gemerkt.
// BHED_DXR_FEHLER=d3d12|geraet|shader|bau spielt je einen Grund (Selbsttest).
[[nodiscard]] Grund grund();
[[nodiscard]] inline bool moeglich() { return grund() == Grund::Keiner; }

// Ein Teil der Schattengeometrie (ein Eimer): Ecken [erstes, erstes + anzahl) als Dreiecke
struct Teil {
    std::uint64_t kennung = 0;   // bleibt, solange es den Eimer gibt (Art, Zelle, Shader)
    std::uint64_t summe = 0;     // Pruefsumme seines Inhalts - gleich: die Struktur bleibt
    std::size_t erstes = 0;
    std::size_t anzahl = 0;
    int art = 0;                 // wie SchattenTeil::art (0 deckend, 1 Volumen, 2 Himmel, 3 alphashadow/lightfilter, 4 Himmelsvolumen)
    void* bild = nullptr;        // art 3: das Bild (ID3D11ShaderResourceView*)
    int filter = 0;              // art 3: 1 alphashadow, 2 lightfilter, 3 beide
};
// Die Schattengeometrie gilt neu: geaenderte Teile neu bauen, die uebrigen behalten, verschwundene freigeben, die Szene
// neu zusammensetzen. Vor beginnen() der Kamera. false: geht nicht (grund() nennt danach Fehler) - dann Schattenkarten.
bool geoSetzen(const std::vector<Punkt3>& ecken, const std::vector<Teil>& teile);
// Steht die Struktur aus geoSetzen noch? (false nach loesen(), einem neuen Grafikgeraet oder einem Fehler - dann neu setzen)
[[nodiscard]] bool szeneSteht();
// Einmal je Bild des Echtzeit-Lichts, vor beginnen() der Kamera: an = in diesem Bild Schattenstrahlen (die Struktur aus
// geoSetzen steht); ruhende Teile mit schnellem Bau werden hier nachgezogen.
void bildSetzen(bool an);
[[nodiscard]] bool aktiv();   // das laufende Bild zeichnet mit Schattenstrahlen
// Wartet ein bewegter Teil seit der Ruhezeit auf das Nachziehen? Dann braucht es noch ein Bild (der ruhende Editor zeichnet
// sonst nicht).
[[nodiscard]] bool nachziehenFaellig();

// Kennzahlen fuer Statuszeile, Messung und Selbsttest
struct Stand {
    std::size_t teile = 0;          // untere Strukturen (Eimer)
    std::size_t dreiecke = 0;
    std::size_t filterTeile = 0;    // davon mit alphashadow/lightfilter
    std::size_t gebaut = 0;         // Baue unterer Strukturen seit dem Start
    std::size_t schnellGebaut = 0;  //   ... davon mit schnellem Bau (bewegte Teile)
    std::size_t nachgezogen = 0;    //   ... davon nach der Ruhezeit neu
    std::size_t szenen = 0;         // Baue der oberen Struktur seit dem Start
    std::size_t bytes = 0;          // Grafikspeicher: Strukturen, Arbeitspuffer, Filterbilder und -daten
    std::size_t strukturBytes = 0;  //   ... davon die Seiten der unteren Strukturen und die obere
    std::size_t gebunden = 0;       // Zeichenaufrufe des Echtzeit-Lichts mit Schattenstrahlen seit dem Start
};
[[nodiscard]] Stand stand();

// --- fuer gui/karte_gpu_win32.cpp ---
// Nach dem Binden des Echtzeit-Lichts (Shader, Puffer, Bilder): mit Schattenstrahlen die DXIL-Fassung derselben Shader und
// die Plaetze t11 bis t13; sonst nichts. kontext: ID3D11DeviceContext*.
void lichtBinden(void* kontext, bool nebelNur);
// Alles freigeben (echtLichtLoesen: Abbau, neues Grafikgeraet).
void loesen();

}  // namespace bhed::gui::kartegpu::dxr

#endif
