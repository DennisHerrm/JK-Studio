// gpuwahl.h - Stufe 8d Punkt 1b: "GPU bake: Auto" - CPU oder GPU, je nachdem was schneller ist
//
// shank: "Das Backen soll automatisch die schnellere Variante nehmen, CPU oder GPU."
//
// Die Entscheidung faellt in bhed_q3map2 (-bhedgpubake auto), denn erst dort ist bekannt, wie gross
// die Lightmaps sind (nach SetupSurfaceLightmaps) und ob die GPU double kann. Der Editor gibt
// die gemerkten Zeiten des letzten CPU- und GPU-Laufs dieser Karte mit (-bhedgpubakezeiten), liest
// die Zeile "BehavEd GPU bake auto: ..." und zeigt Wahl und Grund. Nur Kopfdatei: bhed_q3map2
// bindet bhed_core nicht, und der Unit-Test (ctest karte_gpuwahl) prueft dieselbe Tabelle.
//
// Die Regeln (Messungen aus NOTIZEN_karte8g.md, RTX 4070 Laptop gegen 32 CPU-Threads):
//   1. keine GPU                      -> CPU
//   2. GPU ohne double                -> CPU (sonst nicht bitgleich: Intel UHD 44-90 % gleiche Luxel)
//   3. beide Zeiten gemerkt           -> die schnellere (gelernt, gilt fuer Karte und Licht-Schalter)
//   4. wenig Arbeit                   -> CPU: Super-Luxel x (1 + Bounce) unter kGpuWahlKleinLuxel
//                                         (md_ch_ben 34 490, mp__duel1 101 149 Super-Luxel: GPU
//                                         hoechstens 0,3 s schneller; mp__duel1 mit -bounce 8 = 910 341
//                                         -> GPU, gemessen 1,2 gegen 1,8 s)
//   5. -samples 4 und mehr, gross     -> CPU (duel_mygeeto: GPU 101 s gegen 67 s); ist die CPU-Zeit
//                                         gemerkt, lang und die GPU-Zeit nicht, misst Auto einmal die
//                                         GPU (md_am_sith ist damit 1,9x schneller) - danach Regel 3
//   6. sonst                          -> GPU (kejim_post 1,2-2,4x, md_am_sith 1,9-2,6x, duel_mygeeto 1,6-1,9x)
#ifndef BHED_KARTE_GPUWAHL_H
#define BHED_KARTE_GPUWAHL_H

namespace bhed::karte {

// Arbeit (Super-Luxel = Lightmap-Flaeche samt -super, mal 1 + Bounce) ab der die GPU lohnt.
// Gemessen (bhed_q3map2 -bhedgpubakeauto): md_ch_ben 34 490, mp__duel1 101 149, md_am_sith
// 1 286 512, duel_mygeeto 1 393 026, kejim_post (-super 2) 6 086 248 Super-Luxel.
inline constexpr long long kGpuWahlKleinLuxel = 400000;
// Ab dieser gemerkten CPU-Zeit misst Auto bei -samples >= 4 einmal die GPU (Regel 5).
inline constexpr long long kGpuWahlErkundenMs = 30000;

enum class GpuGeraet { Keins, OhneDouble, MitDouble };
enum class GpuGrund { KeineGpu, OhneDouble, GelerntGpu, GelerntCpu, Klein, Samples, Erkunden, Gross };

struct GpuWahlEingabe {
    GpuGeraet geraet = GpuGeraet::Keins;
    long long superLuxel = 0;   // Summe sw*sh aller Roh-Lightmaps
    int bounce = 0;
    int samples = 1;            // -samples
    long long cpuMs = 0;        // gemerkte Zeit des letzten CPU-Laufs (0: unbekannt)
    long long gpuMs = 0;        // gemerkte Zeit des letzten GPU-Laufs (0: unbekannt)
};

struct GpuWahl {
    bool gpu = false;
    GpuGrund grund = GpuGrund::KeineGpu;
};

[[nodiscard]] inline GpuWahl gpuWahl(const GpuWahlEingabe& e) {
    if (e.geraet == GpuGeraet::Keins) { return {false, GpuGrund::KeineGpu}; }
    if (e.geraet == GpuGeraet::OhneDouble) { return {false, GpuGrund::OhneDouble}; }
    if (e.cpuMs > 0 && e.gpuMs > 0) {
        return e.gpuMs < e.cpuMs ? GpuWahl{true, GpuGrund::GelerntGpu} : GpuWahl{false, GpuGrund::GelerntCpu};
    }
    if (e.superLuxel * (1 + (e.bounce > 0 ? e.bounce : 0)) < kGpuWahlKleinLuxel) { return {false, GpuGrund::Klein}; }
    if (e.samples >= 4) {
        if (e.cpuMs >= kGpuWahlErkundenMs && e.gpuMs == 0) { return {true, GpuGrund::Erkunden}; }
        return {false, GpuGrund::Samples};
    }
    return {true, GpuGrund::Gross};
}

// Kurzname des Grundes in der Ausgabezeile von bhed_q3map2 (der Editor uebersetzt ihn)
[[nodiscard]] inline const char* gpuGrundName(GpuGrund g) {
    switch (g) {
    case GpuGrund::KeineGpu: return "nogpu";
    case GpuGrund::OhneDouble: return "nodouble";
    case GpuGrund::GelerntGpu: return "learnedgpu";
    case GpuGrund::GelerntCpu: return "learnedcpu";
    case GpuGrund::Klein: return "small";
    case GpuGrund::Samples: return "samples";
    case GpuGrund::Erkunden: return "measuregpu";
    case GpuGrund::Gross: return "large";
    }
    return "?";
}

}  // namespace bhed::karte

#endif
