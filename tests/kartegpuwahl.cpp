// kartegpuwahl.cpp - Stufe 8d Punkt 1b: "GPU bake: Auto" ohne GPU (ctest karte_gpuwahl)
//
// Die Entscheidungstabelle (bhed/gpuwahl.h) mit den gemessenen Karten aus
// NOTIZEN_karte8g.md, der Befehl (-bhedgpubakeauto -bhedgpubakezeiten), der Schluessel der
// gemerkten Zeiten, ihr Speicher und das Lesen der Zeile "BehavEd GPU bake auto: ...".
// Die Bitgleichheit bei Auto prueft ctest q3map2_gpubake (Punkt 7d).
#include <cstdio>
#include <string>

#include "bhed/karte/editordaten.h"
#include "bhed/gpuwahl.h"

namespace {
int fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++fehler; }
}
}  // namespace

int main() {
    using namespace bhed::karte;
    const auto w = [](GpuGeraet g, long long luxel, int bounce, int samples, long long cpu = 0, long long gpu = 0) {
        GpuWahlEingabe e;
        e.geraet = g;
        e.superLuxel = luxel;
        e.bounce = bounce;
        e.samples = samples;
        e.cpuMs = cpu;
        e.gpuMs = gpu;
        return gpuWahl(e);
    };
    const auto ist = [](GpuWahl x, bool gpu, GpuGrund g) { return x.gpu == gpu && x.grund == g && !x.nachproben; };
    // vis4: -randomsamples mit dem dritten Kandidaten GPU + Nachproben
    const auto wz = [](long long luxel, long long cpu, long long gpu, long long gpuz, bool nachproben = true) {
        GpuWahlEingabe e;
        e.geraet = GpuGeraet::MitDouble;
        e.superLuxel = luxel;
        e.bounce = 2;
        e.samples = 256;
        e.cpuMs = cpu;
        e.gpuMs = gpu;
        e.nachproben = nachproben;
        e.gpuzMs = gpuz;
        return gpuWahl(e);
    };
    const auto istZ = [](GpuWahl x, GpuGrund g) { return x.gpu && x.nachproben && x.grund == g; };
    constexpr GpuGeraet D = GpuGeraet::MitDouble;
    // Super-Luxel der Messkarten (bhed_q3map2 -bhedgpubakeauto)
    constexpr long long duel1 = 101149, ben = 34490, kejim = 6086248, sith = 1286512, mygeeto = 1393026;

    std::printf("--- Entscheidungstabelle (Messungen NOTIZEN_karte8g.md) ---\n");
    expect("keine GPU -> CPU (nogpu), auch bei grosser Karte", ist(w(GpuGeraet::Keins, sith, 8, 2), false, GpuGrund::KeineGpu));
    expect("GPU ohne double -> CPU (nodouble), auch wenn die GPU als schneller gemerkt ist",
           ist(w(GpuGeraet::OhneDouble, sith, 8, 2, 60000, 30000), false, GpuGrund::OhneDouble));
    expect("mp__duel1 ohne Bounce (0,4 / 0,4 s) -> CPU (small)", ist(w(D, duel1, 0, 2), false, GpuGrund::Klein));
    expect("mp__duel1 -bounce 8 (1,8 / 1,2 s) -> GPU (large)", ist(w(D, duel1, 8, 2), true, GpuGrund::Gross));
    expect("mp__duel1 -samples 4 -bounce 2 (1,1 / 0,8 s, 0,3 s Gewinn) -> CPU (small)", ist(w(D, duel1, 2, 4), false, GpuGrund::Klein));
    expect("md_ch_ben (Gewinn hoechstens 0,3 s) -> CPU (small), auch mit -bounce 8",
           ist(w(D, ben, 0, 2), false, GpuGrund::Klein) && ist(w(D, ben, 8, 2), false, GpuGrund::Klein));
    expect("kejim_post -super 2 (1,2-2,4x) -> GPU (large), auch -samples 3",
           ist(w(D, kejim, 0, 1), true, GpuGrund::Gross) && ist(w(D, kejim, 2, 1), true, GpuGrund::Gross) &&
               ist(w(D, kejim, 0, 3), true, GpuGrund::Gross));
    expect("md_am_sith (2,0x / 2,6x) -> GPU (large)", ist(w(D, sith, 8, 2), true, GpuGrund::Gross) && ist(w(D, sith, 0, 2), true, GpuGrund::Gross));
    expect("duel_mygeeto (1,6x / 1,9x) -> GPU (large)", ist(w(D, mygeeto, 8, 2), true, GpuGrund::Gross) && ist(w(D, mygeeto, 0, 2), true, GpuGrund::Gross));
    expect("duel_mygeeto -samples 4 (GPU 101 s gegen 67 s) -> CPU (samples)", ist(w(D, mygeeto, 2, 4), false, GpuGrund::Samples));
    expect("-samples 4, CPU-Zeit gemerkt und lang (67 s), GPU unbekannt -> einmal die GPU messen",
           ist(w(D, mygeeto, 2, 4, 67200, 0), true, GpuGrund::Erkunden));
    expect("-samples 4, CPU-Zeit kurz (20 s) -> CPU (samples)", ist(w(D, mygeeto, 2, 4, 20000, 0), false, GpuGrund::Samples));
    // vis1: "[Final] Extreme Quality" (-samples 4 -randomsamples = 256 Proben): zuerst der Prozessor, nach einem
    // langen CPU-Lauf einmal die GPU, danach das Schnellere (duel_mos_espa voll: CPU 582 s, GPU 717 s)
    expect("-samples 4 -randomsamples (256): CPU (samples), nach langem CPU-Lauf einmal die GPU, dann gelernt",
           ist(w(D, sith, 8, 256), false, GpuGrund::Samples) && ist(w(D, sith, 8, 256, 582000, 0), true, GpuGrund::Erkunden) &&
               ist(w(D, sith, 8, 256, 582000, 717000), false, GpuGrund::GelerntCpu));
    expect("gelernt: duel_mygeeto -samples 4 (67 / 101 s) -> CPU (learnedcpu)",
           ist(w(D, mygeeto, 2, 4, 67200, 101300), false, GpuGrund::GelerntCpu));
    expect("gelernt: md_am_sith -samples 4 (52,9 / 28,0 s) -> GPU (learnedgpu)",
           ist(w(D, sith, 2, 4, 52900, 28000), true, GpuGrund::GelerntGpu));
    expect("gelernt schlaegt die Groesse: kleine Karte, GPU gemerkt schneller -> GPU; grosse, CPU schneller -> CPU",
           ist(w(D, ben, 0, 2, 1700, 1600), true, GpuGrund::GelerntGpu) && ist(w(D, kejim, 0, 1, 1000, 2000), false, GpuGrund::GelerntCpu));
    expect("gleich schnell gemerkt -> CPU (keine Geraete-Einrichtung)", ist(w(D, sith, 0, 2, 5000, 5000), false, GpuGrund::GelerntCpu));
    expect("nur eine Zeit gemerkt -> die Regeln nach der Groesse", ist(w(D, sith, 0, 2, 0, 9000), true, GpuGrund::Gross) &&
                                                                  ist(w(D, ben, 0, 2, 9000, 0), false, GpuGrund::Klein));
    std::printf("--- vis4: dritter Kandidat GPU + Nachproben (NOTIZEN_vis1.md, vis4) ---\n");
    expect("taspir1: GPU (68,2 s) schneller als CPU (86,6 s), Nachproben unbekannt -> einmal messen (measuregpuz)",
           istZ(wz(sith, 86600, 68200, 0), GpuGrund::ErkundenZ));
    expect("taspir1 gelernt: mit Nachproben 61,9 s am schnellsten -> GPU + Nachproben (learnedgpuz)",
           istZ(wz(sith, 86600, 68200, 61900), GpuGrund::GelerntGpuZ));
    expect("kejim_post gelernt: 33,3 gegen GPU 36,1 / CPU 39,1 s -> GPU + Nachproben",
           istZ(wz(kejim, 39100, 36100, 33300), GpuGrund::GelerntGpuZ));
    expect("mit Nachproben gemerkt langsamer als die GPU -> GPU (learnedgpu), auch gleich schnell",
           ist(wz(sith, 90000, 60000, 70000), true, GpuGrund::GelerntGpu) && ist(wz(sith, 90000, 60000, 60000), true, GpuGrund::GelerntGpu));
    expect("duel_mos_espa: CPU 121,9 s schneller als GPU 150,4 s -> CPU, die Nachproben werden nie gemessen",
           ist(wz(sith, 121900, 150400, 0), false, GpuGrund::GelerntCpu));
    expect("alle drei gemerkt, die CPU am schnellsten -> CPU", ist(wz(sith, 50000, 60000, 55000), false, GpuGrund::GelerntCpu));
    expect("ohne -randomsamples gibt es den dritten Kandidaten nicht (gemerkte Zeit egal)",
           ist(wz(sith, 86600, 68200, 0, false), true, GpuGrund::GelerntGpu) && ist(wz(sith, 86600, 68200, 1000, false), true, GpuGrund::GelerntGpu));
    expect("nur eine oder keine Zeit gemerkt -> die Regeln wie bisher (samples, measuregpu)",
           ist(wz(sith, 0, 0, 0), false, GpuGrund::Samples) && ist(wz(sith, 582000, 0, 0), true, GpuGrund::Erkunden));
    expect("ohne double nie die GPU, auch mit gemerkt schnellen Nachproben", [&] {
        GpuWahlEingabe e;
        e.geraet = GpuGeraet::OhneDouble;
        e.superLuxel = sith;
        e.cpuMs = 90000;
        e.gpuMs = 60000;
        e.nachproben = true;
        e.gpuzMs = 1000;
        const GpuWahl x = gpuWahl(e);
        return !x.gpu && !x.nachproben && x.grund == GpuGrund::OhneDouble;
    }());
    expect("Kurznamen fuer die Ausgabezeile", std::string(gpuGrundName(GpuGrund::Gross)) == "large" &&
                                                  std::string(gpuGrundName(GpuGrund::OhneDouble)) == "nodouble" &&
                                                  std::string(gpuGrundName(GpuGrund::Erkunden)) == "measuregpu" &&
                                                  std::string(gpuGrundName(GpuGrund::GelerntGpuZ)) == "learnedgpuz" &&
                                                  std::string(gpuGrundName(GpuGrund::ErkundenZ)) == "measuregpuz");

    std::printf("--- Befehl, Schluessel, Speicher, Ausgabezeile ---\n");
    const std::string licht = "\"C:/x/bhed_q3map2.exe\" -game ja -fs_basepath \"C:/Spiel\" -light -fast -samples 2 -bounce 8 \"C:/k/maps/a b.map\"";
    const std::string mitAuto = befehlMitGpuBackenAuto(licht, 1234, 0);
    expect("befehlMitGpuBackenAuto: Schalter vor der Karte",
           mitAuto == "\"C:/x/bhed_q3map2.exe\" -game ja -fs_basepath \"C:/Spiel\" -light -fast -samples 2 -bounce 8 -bhedgpubakeauto "
                      "-bhedgpubakezeiten 1234 0 \"C:/k/maps/a b.map\"");
    expect("befehlMitGpuBackenAuto: zweimal = einmal; -vis bleibt unberuehrt",
           befehlMitGpuBackenAuto(mitAuto, 1, 2) == mitAuto &&
               befehlMitGpuBackenAuto("q3map2 -vis \"a.bsp\"", 1, 2) == "q3map2 -vis \"a.bsp\"");
    expect("lichtSchalter: ab -light, ohne Karte und ohne GPU-Wahl",
           lichtSchalter(mitAuto) == "-light -fast -samples 2 -bounce 8" && lichtSchalter(licht) == lichtSchalter(mitAuto) &&
               lichtSchalter(befehlMitGpuBacken(licht) ) == lichtSchalter(licht));
    expect("lichtSchalter: -bhedgpubakecheck N und -bhedgpuadapter X zaehlen nicht, andere Schalter schon",
           lichtSchalter("q -light -fast -bhedgpubakecheck 0 -bhedgpuadapter 1 m.map") == "-light -fast" &&
               lichtSchalter("q -light -fast -bounce 2 m.map") != lichtSchalter("q -light -fast m.map") &&
               lichtSchalter("q -vis m.bsp").empty());
    const std::string mitZ = befehlMitGpuBackenAuto(licht, 1234, 567, 89);
    expect("befehlMitGpuBackenAuto: mit gemerkter Zeit der Nachproben -bhedgpubakezeitenz, lichtSchalter ohne sie",
           mitZ.find("-bhedgpubakezeiten 1234 567 -bhedgpubakezeitenz 89 \"C:/k/maps/a b.map\"") != std::string::npos &&
               lichtSchalter(mitZ) == lichtSchalter(licht) && befehlMitGpuBackenAuto(licht, 1, 2, 0).find("zeitenz") == std::string::npos);
    GpuZeitenSpeicher sp;
    sp.merken("C:/k/a.map", "-light -fast", GpuWeg::Cpu, 5000);
    sp.merken("C:/k/a.map", "-light -fast", GpuWeg::Gpu, 2500);
    sp.merken("C:/k/a.map", "-light -fast -bounce 2", GpuWeg::Gpu, 9000);
    sp.merken("C:/k/a.map", "-light -fast -bounce 2", GpuWeg::GpuNachproben, 7000);   // vis4
    sp.merken("C:/k/b.map", "-light -fast", GpuWeg::Cpu, 0);   // ungueltig: nicht gemerkt
    GpuZeitenSpeicher sp2;
    (void)sp2.lesen(sp.schreiben());
    const GpuZeiten z = sp2.holen("C:/k/a.map", "-light -fast");
    expect("GpuZeitenSpeicher: je Karte und Schalter CPU und GPU, geschrieben und gelesen",
           z.cpuMs == 5000 && z.gpuMs == 2500 && sp2.holen("C:/k/a.map", "-light -fast -bounce 2").gpuMs == 9000 &&
               sp2.holen("C:/k/a.map", "-light -fast -bounce 2").cpuMs == 0 && sp2.groesse() == 2 &&
               sp2.holen("C:/k/b.map", "-light -fast").cpuMs == 0);
    GpuZeitenSpeicher alt;   // vis4: eine Datei ohne die dritte Zeit (vier Felder) liest sich wie bisher
    (void)alt.lesen("100\t200\tC:/k/a.map\t-light -fast\n300\t400\tC:/k/c.map\t-light -randomsamples\t500\n");
    expect("GpuZeitenSpeicher: die Zeit mit Nachproben als fuenftes Feld, alte Zeilen ohne sie",
           sp2.holen("C:/k/a.map", "-light -fast -bounce 2").gpuzMs == 7000 && z.gpuzMs == 0 &&
               alt.holen("C:/k/a.map", "-light -fast").gpuMs == 200 && alt.holen("C:/k/a.map", "-light -fast").gpuzMs == 0 &&
               alt.holen("C:/k/c.map", "-light -randomsamples").gpuzMs == 500 && alt.holen("C:/k/c.map", "-light -randomsamples").cpuMs == 300);
    GpuBackBericht b;
    expect("gpuBackZeile: Auto-Zeile (GPU, Grund, Super-Luxel)",
           gpuBackZeile("BehavEd GPU bake auto: GPU (large) - 1286512 super luxels, bounce 8, samples 2, adapter with double, last CPU 0 ms, last GPU 0 ms", b) &&
               b.autoGesehen && b.autoGpu && b.autoGrund == "large" && b.autoLuxel == 1286512);
    GpuBackBericht c;
    expect("gpuBackZeile: Auto-Zeile (CPU, nodouble)",
           gpuBackZeile("BehavEd GPU bake auto: CPU (nodouble) - 5 super luxels, bounce 0, samples 1, adapter without double, last CPU 1 ms, last GPU 2 ms", c) &&
               c.autoGesehen && !c.autoGpu && c.autoGrund == "nodouble" && c.autoLuxel == 5 && !c.keineGpu && !c.autoNachproben);
    GpuBackBericht d;
    expect("gpuBackZeile: Auto-Zeile GPU+subsamples (vis4), die GPU-Zeile ohne",
           gpuBackZeile("BehavEd GPU bake auto: GPU+subsamples (learnedgpuz) - 7 super luxels, bounce 2, samples 256, adapter with double, "
                        "last CPU 3 ms, last GPU 2 ms, last GPU+subsamples 1 ms", d) &&
               d.autoGpu && d.autoNachproben && d.autoGrund == "learnedgpuz" && !b.autoNachproben);
    expect("Einstellung: Namen off/on/auto, Unbekanntes -> Vorgabe; Vorgabe ist Auto",
           gpuBackenAusName("on", GpuBacken::Aus) == GpuBacken::Ein && gpuBackenAusName("x", GpuBacken::Auto) == GpuBacken::Auto &&
               std::string(gpuBackenName(GpuBacken::Aus)) == "off" && kGpuBackenVorgabe == GpuBacken::Auto);

    std::printf(fehler == 0 ? "\nalle GPU-Wahl-Proben bestanden\n" : "\nFEHLGESCHLAGEN (%d)\n", fehler);
    return fehler == 0 ? 0 : 1;
}
