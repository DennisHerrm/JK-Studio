// karte_echtlicht.h - Karten-Editor Stufe 7 Punkt 5: Echtzeit-Licht in der
// Kamera und Schnell-Backen (app_karte_echtlicht.cpp). Eigene Funktion, nicht
// in NetRadiant-Custom (Build-Menue "Real-time light preview" und "Quick bake
// in the background").
#ifndef BHED_GUI_KARTE_ECHTLICHT_H
#define BHED_GUI_KARTE_ECHTLICHT_H

#include "karte_gpu.h"

#include "bhed/karte/kamera.h"
#include "bhed/karte/mathe.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace bhed::gui::karteneditor {

// Die Vorschau gilt in diesem Bild (an, Textur-Modus, Direct3D 11 mit Shader-Modell 4.1)
[[nodiscard]] bool echtLichtAktiv();
// Vor beginnen() der Kamera: Lichter der Karte (bei Aenderung neu), Auswahl der
// Lichter fuer dieses Bild, Schattenwuerfel (einige je Bild), lichtSetzen.
// teile: die Dreiecke, die Schatten werfen (ohne Himmel und Durchsichtiges).
// Stufe 9c: vp = Kameramatrix (spaltenweise, wie Zustand3::matrix), breite/hoehe
// des Bilds (Kacheln); die Schattengeometrie baut die Funktion selbst aus der Karte.
void echtLichtBildVorbereiten(const karte::Vektor3f& auge, const float* vp, int breite, int hoehe, std::size_t geoStand);
// "Wie im Spiel" (Stufe 9c, Echtzeit-Licht und gebacken): unsichtbar (nodraw),
// Alpha-Test und blendFunc der ersten Stufe; ohne Licht bei blendFunc.
struct SpielDarstellung {
    bool unsichtbar = false;
    int alphaFunktion = 0;   // wie Zustand3 (3 groesser, 2 kleiner, 5 groesser gleich)
    float alphaRef = 0;
    int mischQuelle = 0;
    int mischZiel = 0;
    std::string bild;        // mit alphaFunc/blendFunc: das Bild der ersten Stufe ("?" + Pfad fuer kameraTexturBild)
    bool nebelFlaeche = false;   // Stufe 9d: fogParms ohne Stufe: nur der Nebel (RB_FogPass)
    bool himmel = false;         // Stufe 9d: C_SKY (surfaceparm sky, Sonne) - mit misc_skyportal zeigt sie das Portalbild
    bool eckLicht = false;       // rc608 Teil 11: C_VERTEXLIT - q3map2 gibt ihr Eckfarben mit _minvertexlight statt einer Lightmap
};
[[nodiscard]] SpielDarstellung spielDarstellung(const std::string& shader);
// Der Hinweis in der Kamera-Ecke (Programmsprache): "Vorschau, ohne Bounce und Filter" usw.
[[nodiscard]] std::string echtLichtHinweis();
// Jedes Bild (auch ohne Kamera): Schnell-Backen nach einer Pause ohne Aenderung
// starten, abbrechen, Ergebnis abholen.
void echtLichtSchritt(std::size_t kartenStand);
void echtLichtBeenden();   // beim Beenden: einen laufenden Bau abbrechen
// Stufe 9 Punkt 1: die .bsp des Schnell-Backens, wenn sie zum Stand passt (sonst leer)
[[nodiscard]] std::string schnellBspFuer(std::size_t kartenStand);
// Die gebackenen Flaechen zeichnen (statt der Pinsel und Patches); false: nichts gebacken/aktuell.
bool gebackenZeichnen(const float* matrix);

// --- rc608 Teil 11: "Gebaute Karte wie im Spiel" ---------------------------------------
//
// Die gebaute Karte (das Schnell-Backen, sonst die Vorlage) laeuft durch denselben Zeichner wie die Vorschau im
// Mission-Tab (gpumap: alle Shader-Stufen, tcGen, rgbGen, Blend, Himmelsbox, Gluehen, Nebel, PVS, Spielgamma, die
// Lightmap mal gpu::kSpielLightmapFaktor). Vor beginnen() der Kamera: das fertige Bild in Kameragroesse
// (ID3D11ShaderResourceView*) oder nullptr (Schalter aus, nichts gebaut, kein Direct3D).
[[nodiscard]] void* spielbildFuerKamera(const karte::Kamera& k, int breite, int hoehe);
// Nach kartegpu::bildUnterlegen: die Tiefe der gebauten Flaechen, damit Auswahl, Griffe und Linien davor und
// dahinter liegen wie bisher.
void spielbildTiefe(const float* matrix);
// app_karte_spielbild.cpp: die Szene aus der .bsp (bei neuer Nummer neu) und ihr Bild.
[[nodiscard]] void* spielbildZeichnen(const std::shared_ptr<const std::string>& daten, std::uint64_t nummer, const karte::Kamera& k, int breite,
                                      int hoehe);
// Fuer den Selbsttest: Kennzahlen des letzten Spielbilds (Zeichenaufrufe, ms auf dem Prozessor).
struct SpielbildStand {
    int aufrufe = 0;
    double ms = 0.0;
    std::uint64_t nummer = 0;
    std::size_t flaechenVerdeckt = 0;
};
[[nodiscard]] SpielbildStand spielbildStand();
// rc608 Teil 12, fuer den Bildvergleich im Selbsttest: feste Shaderzeit des Spielbilds in Sekunden (< 0: die Uhr).
void spielbildZeitFest(double sekunden);

}  // namespace bhed::gui::karteneditor

#endif
