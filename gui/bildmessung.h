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
// D3D12-5: Speicher des Prozesses auf der Karte, auf der gezeichnet wird, wie Windows ihn zaehlt (QueryVideoMemoryInfo) - unter
// beiden Grafikwegen gleich gefragt. lokal: Grafikspeicher der Karte, nichtLokal: Arbeitsspeicher, den die Karte liest
// (Hochladespeicher, dynamische Puffer, Lesepuffer). Bytes; gueltig false: keine Auskunft.
struct GrafikSpeicherSystem {
    bool gueltig = false;
    unsigned long long lokal = 0;
    unsigned long long nichtLokal = 0;
    unsigned long long budget = 0;   // lokal
    // nach D3DKMTQueryStatistics: Zuteilungen, die nicht in ihrem bevorzugten Segment liegen (z. B. Grafikspeicher, den Windows
    // in den Arbeitsspeicher gelegt hat), und vom Grafikspeicher herabgestufte Bytes
    unsigned long nichtBevorzugt = 0;
    unsigned long long nichtBevorzugtBytes = 0;
    unsigned long long herabgestuft = 0;
    unsigned long long apertur = 0;   // im Arbeitsspeicher-Segment der Karte (Apertur) festgelegte Bytes des Prozesses
    // Adressraum des Prozesses (VirtualQuery): festgelegter privater Speicher, den der Prozess sieht (Heaps, VirtualAlloc von
    // Programm, Laufzeit und Treiber). Messung 16.11.: unter beiden Grafikwegen fast der ganze private Speicher - auch die
    // Zuteilungen der Grafikkarte stehen als festgelegter Adressraum im Prozess.
    unsigned long long privatSichtbar = 0;
};
[[nodiscard]] GrafikSpeicherSystem grafikSpeicherSystem();
// D3D12-5: Direct3D 12 - die meisten Zeichenbefehle einer abgeschickten Befehlsliste seit dem Start und die Grenze je Liste
// (BHED_D3D12_LISTE, 0: keine). Unter Direct3D 11 beides 0.
struct ListenGroesse {
    unsigned long long hoechstens = 0;
    unsigned long long grenze = 0;
};
[[nodiscard]] ListenGroesse listenGroesse();

}  // namespace bhed::render
#endif
