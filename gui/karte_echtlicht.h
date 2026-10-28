// karte_echtlicht.h - Karten-Editor Stufe 7 Punkt 5: Echtzeit-Licht in der
// Kamera und Schnell-Backen (app_karte_echtlicht.cpp). Eigene Funktion, nicht
// in NetRadiant-Custom (Build-Menue "Real-time light preview" und "Quick bake
// in the background").
#ifndef BHED_GUI_KARTE_ECHTLICHT_H
#define BHED_GUI_KARTE_ECHTLICHT_H

#include "karte_gpu.h"

#include "bhed/karte/mathe.h"

#include <cstddef>
#include <string>
#include <vector>

namespace bhed::gui::karteneditor {

// Die Vorschau gilt in diesem Bild (an, Textur-Modus, Direct3D 11 mit Shader-Modell 4.1)
[[nodiscard]] bool echtLichtAktiv();
// Vor beginnen() der Kamera: Lichter der Karte (bei Aenderung neu), Auswahl der
// Lichter fuer dieses Bild, Schattenwuerfel (einige je Bild), lichtSetzen.
// teile: die Dreiecke, die Schatten werfen (ohne Himmel und Durchsichtiges).
void echtLichtBildVorbereiten(const karte::Vektor3f& auge, std::size_t szeneStand, std::size_t geoStand,
                              const std::vector<kartegpu::SchattenTeil>& teile);
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

}  // namespace bhed::gui::karteneditor

#endif
