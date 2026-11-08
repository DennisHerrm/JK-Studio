// modell_dateien.cpp - Modell-Block Stufe 2: .glm/.gla schreiben, animation.cfg, Pruefungen, Karten-GLA
//
//   GLM     Probe lesen -> Dokument -> schreiben: Byte fuer Byte gleich; Gewicht geaendert -> die Flaeche neu kodiert,
//           neu gelesen genau die Werte des Dokuments, die anderen Flaechen unveraendert; 32 Knochen an einer Flaeche
//           gehen, 33 nicht; Name mit 63 Zeichen geht, 64 nicht
//   GLA     selbst gebaute .gla (3 Knochen, 4 Bilder, Rest hinter dem Namen) schreiben -> lesen -> schreiben gleich
//   cfg     Kommentare, CRLF, ohne Schlussumbruch Byte fuer Byte; geaendert/neu im Raven-Format
//   Grenzen knapp unter und ueber jeder Grenze (Frames, 65535, loopFrames, Dateigroesse je Zielspiel, 53 Knochen,
//           Name im Kopf, MAX_ANIM_FILES, Plaetze)
//   Karten-GLA  zwei Quellen zusammengefuehrt: Bilder, Vorrat zusammengelegt, Zeilen; andere Grundstellung abgewiesen
//
// Aufruf: modelldateientest <model.glm>
#include "bhed/gla.h"
#include "bhed/glm.h"
#include "bhed/modell/animcfg.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/glmdatei.h"
#include "bhed/modell/xsiaufnahme.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", what.c_str());
    if (!ok) { ++fails; }
}
std::string slurp(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

using namespace bhed;
using namespace bhed::modell;

bool gleicheEcken(const GlmModel& a, const Inhalt& in) {
    const GlmModel b = zuGlm(in, 0);
    if (a.surfaces.size() != b.surfaces.size()) { return false; }
    for (std::size_t s = 0; s < a.surfaces.size(); ++s) {
        const auto& x = a.surfaces[s].verts;
        const auto& y = b.surfaces[s].verts;
        if (x.size() != y.size()) { return false; }
        for (std::size_t v = 0; v < x.size(); ++v) {
            if (std::memcmp(x[v].xyz, y[v].xyz, sizeof x[v].xyz) != 0 || std::memcmp(x[v].bones, y[v].bones, 4) != 0 ||
                std::memcmp(x[v].weights, y[v].weights, sizeof x[v].weights) != 0) {
                return false;
            }
        }
    }
    return true;
}

void glmTeil(const std::string& bytes) {
    std::printf("GLM\n");
    Inhalt in;
    std::string f;
    expect("Probe gelesen", ausGlm(bytes, in, &f));
    std::string aus;
    GlmSchreibBericht br;
    expect("geschrieben", glmSchreiben(in, aus, &br));
    const std::string u = glmUnterschied(bytes, aus);
    expect("unveraendert Byte fuer Byte gleich (" + std::to_string(bytes.size()) + " Bytes)" + (u.empty() ? "" : ": " + u), u.empty());
    expect("alle Flaechen mit den Feldern der Datei", br.flaechenNeu == 0 && br.flaechenRoh > 0);

    // Ein Gewicht aendern: die Ecke 0 der ersten Flaeche mit mehr als einem Gewicht auf 0.75/0.25 ihrer Knochen.
    Inhalt neu = in;
    int flaeche = -1;
    for (std::size_t t = 0; t < neu.teile.size() && flaeche < 0; ++t) {
        for (Ecke& e : neu.teile[t].stufen[0].ecken) {
            if (e.gewicht[1] > 0.0F && e.knochen[0] != e.knochen[1]) {
                e.gewicht[0] = 0.75F;
                e.gewicht[1] = 0.25F;
                e.gewicht[2] = e.gewicht[3] = 0.0F;
                e.knochen[2] = e.knochen[3] = 0;
                flaeche = static_cast<int>(t);
                break;
            }
        }
    }
    expect("eine Ecke mit zwei Knochen gefunden", flaeche >= 0);
    expect("geaendert geschrieben", glmSchreiben(neu, aus, &br));
    expect("genau eine Flaeche neu kodiert", br.flaechenNeu == 1);
    GlmModel wieder;
    expect("neu gelesen", readGlm(aus, wieder));
    // Das Spiel liest 0.75 als 767/1023 und den Rest 1 - das; das Dokument traegt nach Uebernehmen genau das.
    Inhalt erwartet = neu;
    Ecke* e0 = nullptr;
    for (Ecke& e : erwartet.teile[static_cast<std::size_t>(flaeche)].stufen[0].ecken) {
        if (e.gewicht[0] == 0.75F) {
            e0 = &e;
            break;
        }
    }
    if (e0 != nullptr) {
        e0->gewicht[0] = static_cast<float>(1.0F / 1023.0F) * 767.0F;
        e0->gewicht[1] = 1.0F - e0->gewicht[0];
    }
    expect("neu gelesen gleich dem Dokument (Orte, Knochen, Gewichte wie das Spiel sie liest)", e0 != nullptr && gleicheEcken(wieder, erwartet));
    Inhalt in2;
    (void)ausGlm(aus, in2);
    std::string aus2;
    expect("das neu Geschriebene schreibt sich wieder gleich", glmSchreiben(in2, aus2) && aus2 == aus);

    // 32 Knochen an einer Flaeche gehen, 33 nicht (iMAX_G2_BONEREFS_PER_SURFACE, 5 Bit).
    for (int zahl : {32, 33}) {
        Inhalt k = in;
        std::size_t gross = 0;
        for (std::size_t t = 0; t < k.teile.size(); ++t) {
            if (k.teile[t].stufen[0].ecken.size() > k.teile[gross].stufen[0].ecken.size()) { gross = t; }
        }
        Netz& n = k.teile[gross].stufen[0];
        n.ausDatei = false;
        for (std::size_t v = 0; v < n.ecken.size(); ++v) {
            Ecke& e = n.ecken[v];
            std::memset(e.knochen, 0, 4);
            std::memset(e.gewicht, 0, sizeof e.gewicht);
            e.knochen[0] = static_cast<std::uint8_t>(static_cast<int>(v) % zahl);
            e.gewicht[0] = 1.0F;
        }
        const bool ok = glmSchreiben(k, aus, &br);
        expect(std::to_string(zahl) + " Knochen an einer Flaeche: " + (zahl <= 32 ? "geschrieben" : "abgewiesen (" + br.fehler + ")"),
               n.ecken.size() >= 33U && ok == (zahl <= 32));
    }
    // Name: 63 Zeichen gehen, 64 nicht (MAX_QPATH mit der Null).
    for (std::size_t laenge : {63U, 64U}) {
        Inhalt k = in;
        k.teile[0].name.assign(laenge, 'a');
        const bool ok = glmSchreiben(k, aus, &br);
        expect("Flaechenname mit " + std::to_string(laenge) + " Zeichen " + (laenge < 64 ? "geschrieben" : "abgewiesen"), ok == (laenge < 64));
    }
    Inhalt md3;
    md3.herkunft = Herkunft::Md3;
    md3.teile.resize(1);
    expect("ein MD3-Dokument ist keine .glm", !glmSchreiben(md3, aus, &br) && !br.fehler.empty());
}

BoneMatrix matrix(float x) {
    BoneMatrix m = BoneMatrix::identity();
    m.m[0][3] = x;
    return m;
}

GlaAnimation probeGla(float grund, int bilder, std::uint8_t muster) {
    GlaAnimation a;
    a.skeletonName = "models/players/_probe/_probe";
    a.dateiName = a.skeletonName;
    a.dateiName.resize(64, '\0');
    a.dateiName[40] = 'X';   // Rest hinter der Null wie in Raven-Dateien
    a.scale = 1.0F;
    a.numFrames = bilder;
    for (int b = 0; b < 3; ++b) {
        GlaBone k;
        k.name = b == 0 ? "model_root" : b == 1 ? "pelvis" : "lower_lumbar";
        k.parent = b - 1;
        k.flags = b == 0 ? 1U : 0U;
        k.basePose = matrix(grund * static_cast<float>(b));
        k.basePoseInv = matrix(-grund * static_cast<float>(b));
        if (b < 2) { k.kinder.push_back(b + 1); }
        a.bones.push_back(k);
    }
    for (int f = 0; f < bilder; ++f) {
        for (int b = 0; b < 3; ++b) {
            // je Bild und Knochen ein Eintrag; Knochen 0 bleibt gleich (teilt sich einen Eintrag)
            const std::uint32_t idx = static_cast<std::uint32_t>(a.bonePool.size() / 14U);
            if (b == 0 && f > 0) {
                a.indexes.push_back(a.indexes[0]);
                continue;
            }
            for (int k = 0; k < 14; ++k) { a.bonePool.push_back(static_cast<std::uint8_t>(muster + f * 3 + b + k)); }
            a.indexes.push_back(idx);
        }
    }
    return a;
}

void glaTeil() {
    std::printf("GLA\n");
    const GlaAnimation a = probeGla(2.0F, 4, 1);
    std::string bytes;
    std::string f;
    expect("geschrieben", schreibGla(a, bytes, &f));
    GlaAnimation b;
    expect("gelesen", readGla(bytes, b, &f));
    expect("Kopf und Skelett gleich", b.skeletonName == a.skeletonName && b.numFrames == 4 && b.bones.size() == 3U && b.bones[0].flags == 1U &&
                                          b.bones[1].kinder == std::vector<int>{2} && b.bones[2].parent == 1);
    expect("Vorrat und Verweise gleich", b.bonePool == a.bonePool && b.indexes == a.indexes);
    std::string bytes2;
    expect("gelesen und wieder geschrieben Byte fuer Byte gleich (mit dem Rest hinter dem Namen)", schreibGla(b, bytes2, &f) && bytes2 == bytes &&
                                                                                                    bytes[8 + 40] == 'X');
    std::int32_t ofsPool = 0;
    std::memcpy(&ofsPool, bytes.data() + 88, 4);
    expect("Vorrat auf 4 Bytes ausgerichtet (writeMdxa alignTo(4))", ofsPool % 4 == 0 && ofsPool > 0);
    GlaAnimation c = b;
    c.skeletonName = "models/players/_anders/_anders";
    expect("geaenderter Name ohne den alten Rest", schreibGla(c, bytes2, &f) && bytes2[8 + 40] == '\0');
    c.indexes.push_back(0);
    expect("Verweise passen nicht -> abgewiesen", !schreibGla(c, bytes2, &f));
}

void cfgTeil() {
    std::printf("animation.cfg\n");
    const std::string text =
        "// kommentar\r\n//\r\nFACE_ALERT          \t0\t2\t-1\t1\r\n\r\nBOTH_WALK1          \t2\t30\t0\t20   // mit Kommentar\r\nROOT\t32\t2\t-1\t29";
    AnimCfg c = animCfgLesen(text);
    expect("Byte fuer Byte gleich (Kommentare, CRLF, ohne Schlussumbruch)", animCfgSchreiben(c) == text);
    expect("drei Sequenzen", c.eintraege().size() == 3U);
    AnimEntry w = c.eintraege()[1];
    w.numFrames = 31;
    c.setzen(w);
    AnimEntry neu{"BOTH_CIN_1", 34, 10, -1, 20};
    c.setzen(neu);
    const std::string t2 = animCfgSchreiben(c);
    expect("geaenderte Zeile im Raven-Format", t2.find("BOTH_WALK1          \t2\t31\t0\t20\r\n") != std::string::npos);
    expect("neue Zeile am Ende", t2.find("BOTH_CIN_1          \t34\t10\t-1\t20") != std::string::npos);
    expect("unveraenderte Zeilen bleiben", t2.find("FACE_ALERT          \t0\t2\t-1\t1\r\n") == 0U + std::string("// kommentar\r\n//\r\n").size());
    expect("gross/klein egal beim Suchen", c.zeileVon("both_walk1") >= 0);
    const std::string lf = "A 0 1 -1 20\nB 1 1 -1 20\n";
    expect("LF-Datei bleibt LF", animCfgSchreiben(animCfgLesen(lf)) == lf);
}

bool hat(const std::vector<CfgBefund>& b, CfgBefundArt a) {
    for (const CfgBefund& x : b) {
        if (x.art == a) { return true; }
    }
    return false;
}

void grenzenTeil() {
    std::printf("Grenzen\n");
    const auto pr = [](AnimEntry e, int bilder, CfgBefundArt a) { return hat(cfgPruefen({e}, bilder, 100, Zielspiel::JkaSp), a); };
    expect("Frames bis zum Ende der GLA: kein Befund", !pr({"X", 90, 10, -1, 20}, 100, CfgBefundArt::HinterGla));
    expect("ein Frame dahinter: HinterGla (T-Pose)", pr({"X", 91, 10, -1, 20}, 100, CfgBefundArt::HinterGla));
    expect("erster Frame 65535 geht", !pr({"X", 65535, 1, -1, 20}, 0, CfgBefundArt::ErsterFrame));
    expect("erster Frame 65536 nicht", pr({"X", 65536, 1, -1, 20}, 0, CfgBefundArt::ErsterFrame));
    // MR4 Punkt 6, im Spiel gemessen (Movie Duels, Karten-GLA mit 131070 Frames, 118 MB, 6946710 Vorratseintraegen, Laeufe
    // g131070/gkreuz/gunter): sie laedt, BOTH_CIN_16 ab 63801 spielt wie ab 1321, eine Sequenz ueber 65535 hinaus (65400 + 204)
    // wie dieselben Frames ab 59152 - die Grenze ist nur der Anfang (unsigned short), nicht das Ende und nicht die Groesse.
    for (const Zielspiel s : {Zielspiel::MdSp, Zielspiel::JkaSp}) {
        const std::vector<CfgBefund> b = cfgPruefen({{"BOTH_CIN_16", 65400, 204, -1, 20}, {"BOTH_CIN_1", 63801, 65535, -1, 20}}, 131070, 100, s);
        expect(std::string(grenzen(s).name) + ": Karten-GLA mit 131070 Frames, Sequenzen ueber 65535 hinaus - kein Befund", b.empty());
    }
    expect("Anzahl 65536 nicht", pr({"X", 0, 65536, -1, 20}, 0, CfgBefundArt::Anzahl));
    expect("loopFrames 127 und -1 gehen", !pr({"X", 0, 1, 127, 20}, 0, CfgBefundArt::Schleife) && !pr({"X", 0, 1, -1, 20}, 0, CfgBefundArt::Schleife));
    expect("loopFrames 128 und -2 nicht", pr({"X", 0, 1, 128, 20}, 0, CfgBefundArt::Schleife) && pr({"X", 0, 1, -2, 20}, 0, CfgBefundArt::Schleife));
    const struct {
        Zielspiel s;
        std::size_t geht;
    } groessen[] = {{Zielspiel::JkaSp, 79998}, {Zielspiel::JkaMp, 59998}, {Zielspiel::Jk2Sp, 39998}, {Zielspiel::MdMp, 524286}};
    for (const auto& g : groessen) {
        const bool ok = !hat(cfgPruefen({}, 0, g.geht, g.s), CfgBefundArt::ZuGross) && hat(cfgPruefen({}, 0, g.geht + 1, g.s), CfgBefundArt::ZuGross);
        expect(std::string(grenzen(g.s).name) + ": " + std::to_string(g.geht) + " Bytes gehen, eins mehr nicht", ok);
    }
    expect("Movie Duels SP: keine Groessengrenze", !hat(cfgPruefen({}, 0, 10000000, Zielspiel::MdSp), CfgBefundArt::ZuGross));
    expect("unbekannter Name gemeldet, ROOT nicht",
           hat(cfgPruefen({{"BOTH_GIBTSNICHT", 0, 1, -1, 20}}, 0, 1, Zielspiel::JkaSp, {"BOTH_WALK1"}), CfgBefundArt::Unbekannt) &&
               !hat(cfgPruefen({{"ROOT", 0, 1, -1, 20}}, 0, 1, Zielspiel::JkaSp, {"BOTH_WALK1"}), CfgBefundArt::Unbekannt));
    GlaAnimation k;
    k.bones.resize(53);
    const std::string pfad = kartenGlaPfad("_humanoid", "t1_sour");
    k.skeletonName = pfad;
    expect("Pfad der Karten-GLA", pfad == "models/players/_humanoid_t1_sour/_humanoid_t1_sour");
    expect("Karten-GLA mit 53 Knochen und passendem Namen: kein Befund (JKA SP)", kartenGlaPruefen(k, pfad, Zielspiel::JkaSp).empty());
    k.bones.resize(52);
    expect("52 Knochen: Befund", hat(kartenGlaPruefen(k, pfad, Zielspiel::JkaSp), CfgBefundArt::Knochen));
    k.bones.resize(53);
    k.skeletonName = "models/players/_humanoid/_humanoid";
    expect("Name im Kopf nicht der Ordner: Befund", hat(kartenGlaPruefen(k, pfad, Zielspiel::JkaSp), CfgBefundArt::NameKopf));
    expect("Ordner und Datei verschieden: Befund", hat(kartenGlaPruefen(k, "models/players/_humanoid_md/_humanoid", Zielspiel::MdSp), CfgBefundArt::NameKopf));
    k.skeletonName = pfad;
    expect("JKA MP und JK2 laden keine Karten-GLA", hat(kartenGlaPruefen(k, pfad, Zielspiel::JkaMp), CfgBefundArt::KeineKartenGla) &&
                                                     hat(kartenGlaPruefen(k, pfad, Zielspiel::Jk2Sp), CfgBefundArt::KeineKartenGla) &&
                                                     !hat(kartenGlaPruefen(k, pfad, Zielspiel::MdSp), CfgBefundArt::KeineKartenGla));
    expect("MAX_ANIM_FILES: JKA 16 gehen, 17 nicht", animDateienPruefen(16, Zielspiel::JkaSp).empty() && !animDateienPruefen(17, Zielspiel::JkaSp).empty());
    expect("MAX_ANIM_FILES: Movie Duels 64 gehen, 65 nicht", animDateienPruefen(64, Zielspiel::MdSp).empty() && !animDateienPruefen(65, Zielspiel::MdSp).empty());
    expect("Plaetze: JKA 50, Movie Duels 125, JK2 keine", kinoPlaetze(Zielspiel::JkaSp, {}).size() == 50U &&
                                                          kinoPlaetze(Zielspiel::MdSp, {}).size() == 125U && kinoPlaetze(Zielspiel::Jk2Sp, {}).empty());
    const auto p = kinoPlaetze(Zielspiel::JkaSp, {{"BOTH_CIN_3", 0, 5, -1, 20}});
    expect("Platz BOTH_CIN_3 belegt, die anderen frei", p[2].belegt && !p[0].belegt && p[2].name == "BOTH_CIN_3");
}

void karteTeil() {
    std::printf("Karten-GLA\n");
    const GlaAnimation a = probeGla(2.0F, 4, 1);
    const GlaAnimation b = probeGla(2.0F, 6, 100);
    std::vector<Platz> pl = kinoPlaetze(Zielspiel::JkaSp, {});
    pl[0].belegt = true;
    pl[0].quelle = {&a, "a", {"BOTH_WALK1", 1, 3, 0, 20}};
    pl[4].belegt = true;
    pl[4].quelle = {&b, "b", {"BOTH_RUN1", 2, 4, -1, 25}};
    pl[5].belegt = true;
    pl[5].quelle = {&a, "a", {"BOTH_WALK1", 1, 3, 0, 20}};   // dieselben Frames noch einmal
    GlaAnimation k;
    std::vector<AnimEntry> cfg;
    ZusammenBericht br;
    const std::string pfad = kartenGlaPfad("_humanoid", "probe");
    expect("zusammengefuehrt", zusammenfuehren(pl, pfad, k, cfg, &br));
    expect("10 Bilder (3 + 4 + 3)", k.numFrames == 10 && br.bilder == 10);
    expect("Zeilen: BOTH_CIN_1 ab 0, BOTH_CIN_5 ab 3, BOTH_CIN_6 ab 7, Schleife und fps der Quelle",
           cfg.size() == 3U && cfg[0].name == "BOTH_CIN_1" && cfg[0].firstFrame == 0 && cfg[0].loopFrame == 0 && cfg[1].name == "BOTH_CIN_5" &&
               cfg[1].firstFrame == 3 && cfg[1].fps == 25 && cfg[2].firstFrame == 7);
    // Vorrat: a Knochen 0 ein Eintrag, je Bild 2 weitere; b ebenso; die Wiederholung legt nichts dazu
    const int erwartet = 1 + 3 * 2 + 1 + 4 * 2;
    expect("Vorrat zusammengelegt (" + std::to_string(br.vorrat) + " Eintraege, erwartet " + std::to_string(erwartet) + ")", br.vorrat == erwartet);
    bool gleich = true;
    for (int f = 0; f < 3; ++f) {
        for (int kn = 0; kn < 3; ++kn) {
            const auto qi = a.indexes[static_cast<std::size_t>(1 + f) * 3U + static_cast<std::size_t>(kn)];
            const auto zi = k.indexes[static_cast<std::size_t>(7 + f) * 3U + static_cast<std::size_t>(kn)];
            gleich = gleich && std::memcmp(a.bonePool.data() + qi * 14U, k.bonePool.data() + zi * 14U, 14) == 0;
        }
    }
    expect("BOTH_CIN_6 traegt genau die Eintraege der Quelle", gleich);
    std::string bytes;
    std::string f;
    expect("die Karten-GLA schreibt sich", schreibGla(k, bytes, &f));
    expect("Name im Kopf = Ordner", kartenGlaPruefen(k, pfad, Zielspiel::JkaSp).size() == 1U);   // nur: 3 statt 53 Knochen
    const GlaAnimation c = probeGla(3.0F, 4, 1);
    pl[4].quelle.gla = &c;
    pl[4].quelle.sequenz = {"BOTH_RUN1", 0, 2, -1, 20};
    expect("andere Grundstellung abgewiesen", !zusammenfuehren(pl, pfad, k, cfg, &br) && br.fehlerArt.art == FehlerArt::QuellGrundstellung && br.fehler.find("base pose") != std::string::npos);
    pl[4].quelle.gla = &b;
    pl[4].quelle.sequenz = {"BOTH_RUN1", 4, 3, -1, 20};
    expect("Frames hinter dem Ende der Quelle abgewiesen", !zusammenfuehren(pl, pfad, k, cfg, &br));
}

// dotXSI aufnehmen (Carcass-Weg ueber G2C): die Max-Fassung aus G2C tests/tests.cpp (ROTATION-Z 0/15/30 Grad an "a",
// drei Bilder), gegen ein Skelett root/a/b in Grundstellung; "b" fehlt in der XSI.
void xsiTeil() {
    std::printf("XSI\n");
    const std::string xsi =
        "xsi 0300txt 0032\n"
        "SI_Scene s { \"FRAMES\", 1.000000, 3.000000, 30.000000, }\n"
        "SI_Model MDL-root {\n"
        "    SI_Model MDL-a {\n"
        "        SI_FCurve a-ROTATION-Z { \"a\", \"ROTATION-Z\", \"LINEAR\", 1, 1, 3,\n"
        "            1.000000,0.000000,\n"
        "            2.000000,15.000000,\n"
        "            3.000000,30.000000,\n"
        "        }\n"
        "    }\n"
        "}\n";
    GlaAnimation sk;
    sk.scale = 1.0F;
    for (int b = 0; b < 3; ++b) {
        GlaBone k;
        k.name = b == 0 ? "root" : b == 1 ? "a" : "b";
        k.parent = b == 0 ? -1 : 0;
        k.basePose = BoneMatrix::identity();
        k.basePoseInv = BoneMatrix::identity();
        sk.bones.push_back(k);
    }
    sk.bones[0].kinder = {1, 2};
    GlaAnimation g;
    XsiBericht br;
    const bool ok = xsiAufnehmen(xsi, sk, "models/players/_probe/_probe", g, &br);
    expect("aufgenommen (" + br.fehler + ")", ok);
    expect("drei Bilder", g.numFrames == 3 && g.indexes.size() == 9U);
    expect("fehlender Knochen b gemeldet", br.fehlendeKnochen.size() == 1U && br.fehlendeKnochen[0] == "b");
    std::vector<BoneMatrix> w;
    g.worldMatrices(0, w);
    expect("Bild 0: a unrotiert", w.size() == 3U && std::fabs(w[1].m[0][0] - 1.0F) < 1e-3F && std::fabs(w[1].m[0][1]) < 1e-3F);
    g.worldMatrices(2, w);
    // tests.cpp: 2e-3 fuer 30 Grad (14 Bytes je Knochen, MC_UnCompressQuat)
    expect("Bild 2: a um 30 Grad um Z gedreht", std::fabs(w[1].m[0][0] - std::cos(30.0F * 3.14159265F / 180.0F)) < 2e-3F);
    expect("b bleibt in der Grundstellung", std::fabs(w[2].m[0][0] - 1.0F) < 1e-3F && std::fabs(w[2].m[1][1] - 1.0F) < 1e-3F);
    std::string bytes;
    std::string f;
    GlaAnimation zurueck;
    expect("als .gla geschrieben und gelesen", schreibGla(g, bytes, &f) && readGla(bytes, zurueck) && zurueck.numFrames == 3);
    expect("kaputte XSI meldet den Grund", !xsiAufnehmen("keine xsi", sk, "x", g, &br) && !br.fehler.empty());
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Aufruf: modelldateientest <model.glm>\n");
        return 2;
    }
    const std::string bytes = slurp(argv[1]);
    if (bytes.empty()) {
        std::printf("Probe fehlt: %s\n", argv[1]);
        return 2;
    }
    glmTeil(bytes);
    glaTeil();
    cfgTeil();
    grenzenTeil();
    karteTeil();
    xsiTeil();
    std::printf("%s (%d Fehler)\n", fails == 0 ? "ok" : "FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
