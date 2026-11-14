// bildmessung.h - D3D12-4: Teile der Bildmessung (render::bildMessung), die nur der Selbsttest leistung liest
#ifndef BHED_GUI_BILDMESSUNG_H
#define BHED_GUI_BILDMESSUNG_H

namespace bhed::render {

// Grafikkarte fuer die Oberflaeche des Hauptfensters im zuletzt abgeholten Bild (Zeitmarke vor dem Loeschen des Hauptfensters bis
// zur Endmarke des Bildes), unter beiden Grafikwegen gleich gesetzt. bilder zaehlt die abgeholten Bilder; -1: keins.
struct OberflaecheZeit {
    double gpuMs = -1.0;
    int bilder = 0;
};
[[nodiscard]] OberflaecheZeit oberflaecheZeit();
// Direct3D 12: Millisekunden, die der Hauptfaden seit dem Start auf Zaunwerte und den Abschickfaden gewartet hat (0 sonst).
[[nodiscard]] double hauptfadenWartenMs();

}  // namespace bhed::render
#endif
