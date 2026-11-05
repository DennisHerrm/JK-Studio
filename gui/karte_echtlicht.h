// karte_echtlicht.h - Karten-Editor Stufe 7 Punkt 5: Echtzeit-Licht in der
// Kamera und Schnell-Backen (app_karte_echtlicht.cpp). Eigene Funktion, nicht
// in NetRadiant-Custom (Build-Menue "Real-time light preview" und "Quick bake
// in the background").
#ifndef BHED_GUI_KARTE_ECHTLICHT_H
#define BHED_GUI_KARTE_ECHTLICHT_H

#include "karte_gpu.h"

#include "bhed/karte/kamera.h"
#include "bhed/karte/mathe.h"
#include "bhed/portalsicht.h"

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
    bool portal = false;         // rc609 P: sort portal (mit mindestens einer Stufe) - zeigt die Spiegel- bzw. Portalsicht unter ihren Stufen
    float portalWeite = 0;       //   ... portalRange (alphaGen portal)
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
// `kartenName`: die Datei der Karte (Movie Duels liest Weather/<name>.cfg).
[[nodiscard]] void* spielbildZeichnen(const std::shared_ptr<const std::string>& daten, std::uint64_t nummer, const std::string& kartenName,
                                      const karte::Kamera& k, int breite, int hoehe);
// Fuer den Selbsttest: Kennzahlen des letzten Spielbilds (Zeichenaufrufe, ms auf dem Prozessor).
struct SpielbildStand {
    int aufrufe = 0;
    double ms = 0.0;
    std::uint64_t nummer = 0;
    std::size_t flaechenVerdeckt = 0;
    int modelle = 0;           // rc608 Teil 13b: Kartenmodelle im Bild (aufgestellt) und ihre Zeichenaufrufe
    int modellAufrufe = 0;
    bool himmelPortal = false; // das Bild des misc_skyportal stand unter der Hauptszene
    int wetterTeilchen = 0;
    int grasVierecke = 0;      // rc608 Teil 13c: Halme (surfaceSprites) des Hauptbilds
    int effektLaeufer = 0;     // rc608 Teil 13c: laufende fx_runner und die Ecken ihres Netzes im Hauptbild
    int effektEcken = 0;
    int portalEffektEcken = 0; // die Effekte der Portal-Entities in der Szene des Himmelsportals
    bool spiegel = false;      // rc609 P: eine Spiegel- oder Portalsicht stand unter den Portalflaechen
    bool portalKamera = false; //   ... und es war eine misc_portal_camera
    float spiegelWelt[16]{};   //   ... ihre Abbildung (PortalSicht::welt) und Schnittebene
    float spiegelSchnitt[4]{};
    // rc610 F23 (B3-1): Plaetze der Szene (Bilder samt Modellbildern, Lightmaps) und der hoechste belegte Platz im Grafikspeicher
    int bildPlaetze = 0;
    int lightmapPlaetze = 0;
    int hoechstesBild = -1;
    int hoechsteLightmap = -1;
};
[[nodiscard]] SpielbildStand spielbildStand();
// rc609 P: die Spiegel- bzw. Portalsicht der Echtzeit-Ansicht (Pinsel des Editors, misc_portal_surface des Editors - live)
// ueber denselben Code wie Vorschau und Spielbild (bhed/portalsicht). false: keine.
[[nodiscard]] bool echtPortalSicht(const karte::Kamera& k, int breite, int hoehe, float sichtfeld, std::size_t geoStand, PortalSicht& aus);
// rc608 Teil 12, fuer den Bildvergleich im Selbsttest: feste Shaderzeit des Spielbilds in Sekunden (< 0: die Uhr).
void spielbildZeitFest(double sekunden);

}  // namespace bhed::gui::karteneditor

#endif
