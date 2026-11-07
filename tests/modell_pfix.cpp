// modell_pfix.cpp - Funde der Code-Pruefung Modell-Block (plaene/codepruefung_modellblock.md), Leser und Schreiber mit
// selbst erzeugten kaputten Dateien: kein Absturz, klare Ablehnung.
//   M2  .md3 mit ofsXyzNormals hinter ofsEnd der Flaeche: md3Schreiben lehnt ab (vorher std::out_of_range)
//   M3  .gla mit Elternindex >= numBones: readGla lehnt ab
//   M4  Signalname mit Anfuehrungszeichen: der Figurenblock bleibt im Skript (vorher geloescht)
//   M5  Zuordnung mit Kettennummer hinter der Quelle: rtPoseAbweichung/rtUebertragen ueberspringen sie
//   N2  .md3 mit mehr als SHADER_MAX_INDEXES/3 Dreiecken: md3Schreiben lehnt ab (R_LoadMD3, tr_model.cpp:903)
//   N3  Figur-Kommentar "k 0 2000000000" / "bilder 2000000000": kein bad_alloc
//   N9  animation.cfg firstFrame + numFrames ueber INT_MAX: HinterGla
//   H1  Dokument: abbrechen(Schritt) nimmt nur den eigenen Schritt
//
// Aufruf: modellpfixtest
#include "bhed/gla.h"
#include "bhed/md3.h"
#include "bhed/modell/animcfg.h"
#include "bhed/modell/animkurven.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/figuranim.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/md3datei.h"
#include "bhed/modell/retarget.h"
#include "bhed/modell/rig.h"
#include "bhed/script.h"

#include <array>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <vector>

using namespace bhed;
using namespace bhed::modell;

namespace {

int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", what.c_str());
    if (!ok) { ++fails; }
}

std::int32_t lies(const std::string& b, std::size_t at) {
    std::int32_t v = 0;
    std::memcpy(&v, b.data() + at, 4);
    return v;
}
void setze(std::string& b, std::size_t at, std::int32_t v) { std::memcpy(b.data() + at, &v, 4); }

// Ein Teil mit n Dreiecken ueber drei Ecken (entartet erlaubt: die Datei zaehlt Dreiecke, nicht Flaeche)
Inhalt md3Inhalt(int dreiecke) {
    Inhalt in;
    in.herkunft = Herkunft::Md3;
    in.name = "pfix";
    in.stufenZahl = 1;
    Teil t;
    t.name = "pfix";
    t.shader = "textures/common/white";
    t.stufen.resize(1);
    std::vector<Ecke> e(3);
    e[1].ort[0] = 8.0F;
    e[2].ort[1] = 8.0F;
    for (Ecke& x : e) { x.normale[2] = 1.0F; }
    std::vector<std::uint32_t> idx;
    for (int i = 0; i < dreiecke; ++i) {
        idx.push_back(0);
        idx.push_back(1);
        idx.push_back(2);
    }
    t.stufen[0].aufbauen(e, idx);
    in.teile.push_back(t);
    return in;
}

void md3Test() {
    // M2: eine gueltige .md3, dann ofsXyzNormals der Flaeche so weit nach hinten, dass die Ecken hinter ofsEnd liegen (das
    // Dateiende reicht noch: readMd3 prueft nur dagegen)
    Inhalt in = md3Inhalt(1);
    std::string bytes;
    Md3SchreibBericht br;
    const bool ok = md3Schreiben(in, bytes, 1000, &br);
    expect("M2 Vorlage: .md3 geschrieben", ok);
    if (ok) {
        constexpr std::size_t kName = 64;
        const std::size_t ofsFlaechen = static_cast<std::size_t>(lies(bytes, 8 + kName + 28));
        const std::size_t n = ofsFlaechen + 4 + kName;
        const std::int32_t sEnde = lies(bytes, n + 36);
        std::string kaputt = bytes;
        setze(kaputt, n + 32, sEnde - 8);   // ofsXyzNormals: 3 Ecken * 8 Bytes, nur 8 Bytes vor dem Ende der Flaeche
        kaputt.append(64, '\0');            // die Datei geht weiter (readMd3 liest bis zum Dateiende)
        Md3Model m;
        std::string err;
        const bool gelesen = readMd3(kaputt, m, &err);
        Inhalt doc;
        if (gelesen) {
            ausMd3(m, doc, 0);
            doc.md3Datei = std::make_shared<const std::string>(kaputt);
            if (!doc.teile.empty() && !doc.teile[0].stufen.empty() && !doc.teile[0].stufen[0].ecken.empty()) {
                doc.teile[0].stufen[0].ecken[0].uv[0] = 0.5F;   // eine UV-Aenderung: der Schreiber nimmt die Abschnitte der Datei
            }
        }
        bool abgelehnt = false;
        std::string fehler;
        try {
            std::string aus;
            Md3SchreibBericht b2;
            abgelehnt = !md3Schreiben(doc, aus, 1000, &b2);
            fehler = b2.fehler;
        } catch (const std::exception& e) {
            fehler = std::string("Ausnahme: ") + e.what();
        }
        std::printf("        M2: gelesen %d, %s\n", gelesen ? 1 : 0, fehler.c_str());
        expect("M2 .md3 mit Ecken hinter ofsEnd der Flaeche: abgelehnt mit Meldung, keine Ausnahme", gelesen && abgelehnt && fehler.find("Ausnahme") == std::string::npos);
    }
    // N2: R_LoadMD3 (OpenJK code/rd-vanilla/tr_model.cpp:903, MD9 tr_model.cpp:919): numTriangles*3 > SHADER_MAX_INDEXES
    // (6*SHADER_MAX_VERTEXES, qfiles.h:34) -> ERR_DROP; 2001 Dreiecke ueber drei Ecken
    {
        std::string aus;
        Md3SchreibBericht b;
        const bool ja = md3Schreiben(md3Inhalt(2000), aus, 1000, &b);
        expect("N2 .md3 mit 2000 Dreiecken: geschrieben (Grenze 6000 Indizes)", ja);
        const bool nein = !md3Schreiben(md3Inhalt(2001), aus, 1000, &b);
        std::printf("        N2: %s\n", b.fehler.c_str());
        expect("N2 .md3 mit 2001 Dreiecken: abgelehnt wie R_LoadMD3", nein);
    }
}

// M1: eine .md3 mit einer leeren Flaeche in der Mitte (wie engi_wall3.md3 aus MD_Maps_Ep5): readMd3 ueberspringt sie
void leereFlaecheTest() {
    Inhalt in = md3Inhalt(1);
    in.teile[0].name = "a";
    Teil leer;
    leer.name = "leer";
    leer.shader = "textures/common/white";
    leer.stufen.resize(1);
    Teil c = in.teile[0];
    c.name = "c";
    for (Ecke& e : c.stufen[0].ecken) { e.ort[2] = 4.0F; }
    c.stufen[0].aufbauen(c.stufen[0].ecken, c.stufen[0].indizes());
    in.teile.push_back(leer);
    in.teile.push_back(c);
    std::string bytes;
    Md3SchreibBericht br;
    if (!md3Schreiben(in, bytes, 1000, &br)) {
        expect("M1 Vorlage geschrieben: " + br.fehler, false);
        return;
    }
    Md3Model m;
    std::string err;
    const bool gelesen = readMd3(bytes, m, &err);
    Inhalt doc;
    ausMd3(m, doc, 0);
    doc.md3Datei = std::make_shared<const std::string>(bytes);
    std::printf("        M1: Flaechen der Datei 3, gelesen %zu\n", m.surfaces.size());
    std::string wieder;
    Md3SchreibBericht b2;
    const bool roh = md3Schreiben(doc, wieder, 1000, &b2);
    std::printf("        M1 Rundlauf: %s\n", b2.fehler.c_str());
    expect("M1 .md3 mit leerer Flaeche: Rundlauf ohne Aenderung Byte fuer Byte", gelesen && roh && wieder == bytes);
    // eine UV-Aenderung an "c" (Teil 1 im Dokument, Flaeche 2 der Datei) und ein neuer Teil dahinter (Detach in einen neuen Teil)
    if (doc.teile.size() == 2U) {
        doc.teile[1].stufen[0].ecken[0].uv[0] = 0.25F;
        Teil neu = in.teile[0];
        neu.name = "neu";
        neu.dateiFlaeche = -1;
        doc.teile.push_back(neu);
    }
    std::string geaendert;
    Md3SchreibBericht b3;
    const bool ok = md3Schreiben(doc, geaendert, 1000, &b3);
    Md3Model n;
    const bool wiederGelesen = ok && readMd3(geaendert, n, &err);
    std::string namen;
    for (const Md3Surface& s : n.surfaces) { namen += s.name + " "; }
    const bool uvDa = wiederGelesen && n.surfaces.size() == 3U && n.surfaces[1].name == "c" && !n.surfaces[1].verts.empty() && n.surfaces[1].verts[0].st[0] == 0.25F &&
                      n.surfaces[1].verts[0].xyz[2] == 4.0F;
    std::printf("        M1 geaendert: ok %d, %s, Flaechen %s\n", ok ? 1 : 0, b3.fehler.c_str(), namen.c_str());
    expect("M1 UV an c und ein neuer Teil: c bleibt c (Ort, neue UV), der neue Teil kommt hinten dazu", uvDa && n.surfaces[2].name == "neu");
}

GlaAnimation zweiKnochen() {
    GlaAnimation g;
    g.name = "pfix";
    g.skeletonName = "pfix";
    g.scale = 1.0F;
    g.numFrames = 1;
    g.bones.resize(2);
    g.bones[0].name = "wurzel";
    g.bones[0].parent = -1;
    g.bones[1].name = "kind";
    g.bones[1].parent = 0;
    for (GlaBone& b : g.bones) {
        b.basePose = BoneMatrix::identity();
        b.basePoseInv = BoneMatrix::identity();
    }
    g.bones[0].kinder = {1};
    g.bonePool.assign(14, 0);
    g.indexes = {0, 0};
    return g;
}

void glaTest() {
    GlaAnimation g = zweiKnochen();
    std::string bytes;
    std::string f;
    const bool ok = schreibGla(g, bytes, &f);
    expect("M3 Vorlage: .gla geschrieben " + f, ok);
    if (!ok) { return; }
    GlaAnimation a;
    expect("M3 Vorlage gelesen", readGla(bytes, a, &f) && a.bones.size() == 2U && a.bones[1].parent == 0);
    // den Elternindex von Knochen 1 auf 7 (nur zwei Knochen)
    std::string kaputt = bytes;
    constexpr std::size_t kHeaderEnd = 8 + 64 + 4 + 6 * 4;
    const std::size_t base = kHeaderEnd + static_cast<std::size_t>(lies(kaputt, kHeaderEnd + 4));
    setze(kaputt, base + 64 + 4, 7);
    GlaAnimation b;
    std::string fehler;
    const bool gelesen = readGla(kaputt, b, &fehler);
    std::printf("        M3: gelesen %d, Elternteil %d, %s\n", gelesen ? 1 : 0, b.bones.size() > 1U ? b.bones[1].parent : -99, fehler.c_str());
    expect("M3 .gla mit Elternindex 7 bei zwei Knochen: abgelehnt", !gelesen && !fehler.empty());
}

void figurTest() {
    const GlaAnimation g = zweiKnochen();
    const Rig r;
    FigurAnim a;
    a.figur = "kyle";
    a.basis = "BOTH_STAND1";
    a.platz = "BOTH_CIN_1";
    a.bilder = 10;
    a.fps = 20;
    a.schicht.knochen.assign(g.bones.size(), {});
    // M4: ein Anfuehrungszeichen im Warte-Signal
    Script s;
    std::vector<Diag> d;
    (void)readScript("affect ( \"kyle\", /*@AFFECT_TYPE*/ FLUSH )\n{\n}\n", s, d);
    const Script mit = mitFigurBlock(s, a.figur, &a, {0.0F, 0.0F, 0.0F});
    const bool da = findeFigurBlock(mit, a.figur).has_value();
    FigurAnim b = a;
    b.warteSignal = "a\"b";
    const Script neu = mitFigurBlock(mit, b.figur, &b, {0.0F, 0.0F, 0.0F});
    const bool bleibt = findeFigurBlock(neu, b.figur).has_value();
    std::printf("        M4: Block vorher %d, nachher %d, Knoten %zu -> %zu, Name gueltig %d\n", da ? 1 : 0, bleibt ? 1 : 0, mit.nodes.size(), neu.nodes.size(),
                figurNameGueltig(b.warteSignal) ? 1 : 0);
    expect("M4 Warte-Signal a\"b: der Figurenblock bleibt im Skript, der Name gilt als ungueltig", da && bleibt && neu.nodes.size() == mit.nodes.size() && !figurNameGueltig(b.warteSignal));
    // N3: Anzahl aus dem Text riesig
    std::vector<std::string> z = figurZeilen(a);
    z.push_back("//(BehavEd) figur+ \"kyle\" k 0 2000000000 0 0 0 0 0 1");
    bool ohneAbsturz = true;
    FigurAnim aus;
    std::string fehler;
    try {
        (void)figurAusZeilen(z, g, r, aus, &fehler);
    } catch (const std::exception& e) {
        ohneAbsturz = false;
        fehler = std::string("Ausnahme: ") + e.what();
    }
    std::printf("        N3 k: %s\n", fehler.c_str());
    expect("N3 Figur-Kommentar k 0 2000000000: keine Ausnahme, abgelehnt", ohneAbsturz && !fehler.empty());
    std::vector<std::string> z2 = figurZeilen(a);
    const std::size_t at = z2[0].find(" bilder 10 ");
    if (at != std::string::npos) { z2[0].replace(at, 11, " bilder 2000000000 "); }
    FigurAnim aus2;
    std::string f2;
    const bool ok2 = figurAusZeilen(z2, g, r, aus2, &f2);
    std::printf("        N3 bilder: ok %d, bilder %d, %s\n", ok2 ? 1 : 0, aus2.bilder, f2.c_str());
    // animation_t numFrames ist unsigned short, NPC_stats.cpp:952-979 assert(< 65536)
    expect("N3 Markenzeile bilder 2000000000: abgelehnt oder hoechstens 65535", at != std::string::npos && (!ok2 || aus2.bilder <= 65535));
}

void retargetTest() {
    // M5: Ziel mit einer Armkette, Zuordnung auf Kette 5 einer Quelle ohne Ketten
    const GlaAnimation g = zweiKnochen();
    RtCharakter q;
    RtCharakter z;
    RtKette k;
    k.art = KettenArt::Arm;
    k.knochen = {0, 1, 1};
    z.ketten.push_back(k);
    RtZuordnung zu;
    zu.quelle = {5};
    const std::vector<BoneMatrix> w(2, BoneMatrix::identity());
    const float a = rtPoseAbweichung(g, q, w, g, z, w, zu);
    std::printf("        M5: Abweichung %.3f\n", static_cast<double>(a));
    expect("M5 rtPoseAbweichung mit Kettennummer hinter der Quelle: uebersprungen", a == 0.0F);
}

// H5: Umkehren / Spiegeln am Cursor mit IK-Schluesseln in der Wahl (Dope Sheet: IK-Zeile)
void ikWahlTest() {
    for (const WertArt art : {WertArt::Umkehren, WertArt::SpiegelnCursor}) {
        RigSchicht s;
        s.knochen.assign(2, {});
        s.ik.resize(1);
        for (const int b : {0, 10}) {
            RigSchluessel k;
            k.bild = b;
            k.t[0] = static_cast<float>(b);
            s.ik[0].ziel.push_back(k);
            s.ik[0].wert.push_back(IkWert{});
        }
        RigSchluessel f;
        f.bild = 0;
        s.knochen[1].push_back(f);
        f.bild = 10;
        s.knochen[1].push_back(f);
        const Wahl w = {{ikWahlCode(0), 0}, {ikWahlCode(0), 10}, {1, 0}, {1, 10}};
        const Wahl neu = werteAendern(s, w, art, 5);
        std::printf("        H5 %s: Wahl %zu, IK %.0f/%.0f\n", art == WertArt::Umkehren ? "Umkehren" : "Spiegeln", neu.size(), static_cast<double>(s.ik[0].ziel[0].t[0]),
                    static_cast<double>(s.ik[0].ziel.back().t[0]));
        expect(std::string("H5 ") + (art == WertArt::Umkehren ? "Umkehren" : "Spiegeln am Cursor") + " mit IK-Schluesseln: kein Absturz, beide Spuren umgesetzt",
               neu.size() == 4U && s.ik[0].ziel.size() == 2U);
    }
}

void cfgTest() {
    AnimEntry e;
    e.name = "BOTH_X";
    e.firstFrame = 10;
    e.numFrames = INT_MAX;
    expect("N9 animInnerhalb(10, INT_MAX) bei 100 Bildern: nicht innerhalb", !animInnerhalb(e, 100));
    const std::vector<CfgBefund> b = cfgPruefen({e}, 100, 10, Zielspiel::JkaSp, {});
    bool hinter = false;
    for (const CfgBefund& x : b) { hinter = hinter || x.art == CfgBefundArt::HinterGla; }
    expect("N9 cfgPruefen: HinterGla-Befund", hinter);
}

void dokumentTest() {
    // H1: Schritt A uebernommen, Schritt B offen, Rueckgaengig (B auf Wiederholen), dann B abbrechen: A bleibt
    Dokument d;
    d.laden(Inhalt{});
    d.aendern("A", [](Inhalt& x) { x.name = "A"; });
    const std::uint64_t b = d.aendern("B", [](Inhalt&) {});
    (void)d.rueckgaengig();
    (void)d.abbrechen(b);
    std::printf("        H1: Name \"%s\", Rueckgaengig \"%s\"\n", d.inhalt().name.c_str(), d.rueckgaengigName().c_str());
    expect("H1 abbrechen(B) nach Rueckgaengig nimmt A nicht weg", d.inhalt().name == "A" && d.rueckgaengigName() == "A");
    Dokument e;
    e.laden(Inhalt{});
    const std::uint64_t c = e.aendern("C", [](Inhalt& x) { x.name = "C"; });
    expect("H1 abbrechen(C) nimmt den eigenen offenen Schritt", e.abbrechen(c) && e.inhalt().name.empty() && !e.kannRueckgaengig());
}

}  // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    md3Test();
    leereFlaecheTest();
    glaTest();
    figurTest();
    retargetTest();
    cfgTest();
    ikWahlTest();
    dokumentTest();
    std::printf("%s (%d Fehler)\n", fails == 0 ? "OK" : "FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
