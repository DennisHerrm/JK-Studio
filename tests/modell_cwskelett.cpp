// modell_cwskelett.cpp - CW-MODELL (15.11.2026): Bolts fuer Sitze und Muendungen, eigenes Skelett fuer ein Fahrzeug, animation.cfg
//
//   Bolts      das Dreieck zu einer Lage liest das Spiel wieder als diese Lage (G2_ProcessSurfaceBolt2 ueber boltMatrixRigid), Seite 0
//              die laengste, Seite 2 die kuerzeste; anlegen an model.glm, schreiben, lesen; Namen frei/doppelt/zu lang; umbenennen,
//              an anderen Knochen; Pruefung Fahrersitz/Muendung; Vorlagen je Zielspiel (OpenJK 10/4, Movie Duels 12/12)
//   Skelett    der ganze Weg ohne Handarbeit: Skelett anlegen -> Knochen -> Flaechen starr binden -> Bilder -> GLA schreiben/lesen ->
//              GLM schreiben/lesen (Kopf: Pfad, Knochenzahl) -> animation.cfg schreiben/lesen; die Pose im Bild 2 wie gekeyt
//   cfg        unbekannte Namen je Zielspiel (animTable von OpenJK und Movie Duels Update 9) als Befund, nicht still verworfen
//
// Aufruf: modellcwskelettest <model.glm>
#include "bhed/gla.h"
#include "bhed/glm.h"
#include "bhed/modell/animcfg.h"
#include "bhed/modell/animnamen.h"
#include "bhed/modell/bolts.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/glmdatei.h"
#include "bhed/modell/skelett.h"
#include "bhed/modell/stapel.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
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

bool nah(const BoneMatrix& a, const BoneMatrix& b, float eps = 1e-4F) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) {
            if (std::abs(a.m[r][c] - b.m[r][c]) > eps) { return false; }
        }
    }
    return true;
}

BoneMatrix drehungZ(float grad, float x, float y, float z) {
    const float w = grad * 3.14159265F / 180.0F;
    BoneMatrix m = BoneMatrix::identity();
    m.m[0][0] = std::cos(w);
    m.m[0][1] = -std::sin(w);
    m.m[1][0] = std::sin(w);
    m.m[1][1] = std::cos(w);
    m.m[0][3] = x;
    m.m[1][3] = y;
    m.m[2][3] = z;
    return m;
}

// Drehung um Y (das Bein schwingt nach vorn)
BoneMatrix drehungY(float grad) {
    const float w = grad * 3.14159265F / 180.0F;
    BoneMatrix m = BoneMatrix::identity();
    m.m[0][0] = std::cos(w);
    m.m[0][2] = std::sin(w);
    m.m[2][0] = -std::sin(w);
    m.m[2][2] = std::cos(w);
    return m;
}

bool hat(const std::vector<CfgBefund>& b, CfgBefundArt a, const std::string& name) {
    return std::any_of(b.begin(), b.end(), [&](const CfgBefund& x) { return x.art == a && x.name == name; });
}

void boltTeil(const std::string& pfad) {
    std::printf("Bolts\n");
    // Lage -> Dreieck -> Lage
    const BoneMatrix lage = drehungZ(30.0F, 12.0F, -4.0F, 40.0F);
    float p[3][3];
    boltDreieck(lage, 1.5F, p);
    const auto laenge = [&](int a, int b) {
        return std::sqrt((p[b][0] - p[a][0]) * (p[b][0] - p[a][0]) + (p[b][1] - p[a][1]) * (p[b][1] - p[a][1]) + (p[b][2] - p[a][2]) * (p[b][2] - p[a][2]));
    };
    expect("Seite 0 die laengste, Seite 2 die kuerzeste (mdx_format.h)", laenge(0, 1) > laenge(1, 2) && laenge(1, 2) > laenge(2, 0));
    Teil t;
    t.name = "*probe";
    t.flaggen = GlmSurface::kFlagBolt;
    std::vector<Ecke> e(3);
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) { e[static_cast<std::size_t>(i)].ort[k] = p[i][k]; }
    }
    t.stufen.resize(1);
    t.stufen[0].aufbauen(e, {0, 1, 2});
    BoneMatrix zurueck;
    expect("das Spiel liest dieselbe Lage (G2_ProcessSurfaceBolt2)", boltLage(t, zurueck) && nah(zurueck, lage));

    Inhalt in;
    expect("model.glm gelesen", ausGlm(slurp(pfad), in));
    const int vorher = static_cast<int>(in.teile.size());
    const int knochen = 5;
    const int neu = boltAnlegen(in, "*driver", knochen, lage);
    expect("*driver angelegt", neu == vorher && in.teile[static_cast<std::size_t>(neu)].istTag() &&
                                   in.teile[static_cast<std::size_t>(neu)].stufen.size() == static_cast<std::size_t>(in.stufenZahl));
    expect("ein Dreieck je Stufe, Gewicht 1 am Knochen", [&] {
        for (const Netz& n : in.teile[static_cast<std::size_t>(neu)].stufen) {
            if (n.dreiecke() != 1 || n.ecken.size() != 3U) { return false; }
            for (const Ecke& x : n.ecken) {
                if (x.knochen[0] != knochen || x.gewicht[0] != 1.0F) { return false; }
            }
        }
        return boltKnochen(in.teile[static_cast<std::size_t>(neu)]) == knochen;
    }());
    const int eltern = in.teile[static_cast<std::size_t>(neu)].eltern;
    expect("Eltern: eine Flaeche am Knochen, das Kind steht in ihrer Liste", eltern >= 0 &&
                                                                              std::count(in.teile[static_cast<std::size_t>(eltern)].kinder.begin(),
                                                                                         in.teile[static_cast<std::size_t>(eltern)].kinder.end(), neu) == 1);
    expect("doppelter Name abgewiesen (gross/klein egal)", boltAnlegen(in, "*DRIVER", 0, lage) == -1);
    expect("Name ohne * abgewiesen", boltAnlegen(in, "driver2", 0, lage) == -1);
    expect("Name ueber 63 Zeichen abgewiesen", boltAnlegen(in, "*" + std::string(63, 'x'), 0, lage) == -1);
    expect("Pruefung: Fahrersitz da, Muendung fehlt", fahrzeugBoltsPruefen(in) == std::vector<BoltBefundArt>{BoltBefundArt::KeineMuendung});
    const int m1 = boltAnlegen(in, "*muzzle1", 6, drehungZ(0.0F, 30.0F, 0.0F, 50.0F));
    expect("*muzzle1 angelegt, Pruefung ohne Befund", m1 > 0 && fahrzeugBoltsPruefen(in).empty());
    expect("umbenennen in einen freien Namen", boltUmbenennen(in, m1, "*flash2") && in.teile[static_cast<std::size_t>(m1)].name == "*flash2");
    expect("*flash2 zaehlt als Muendung (Rueckfall von *muzzle2)", fahrzeugBoltsPruefen(in).empty());
    expect("umbenennen in einen vergebenen Namen abgewiesen", !boltUmbenennen(in, m1, "*driver"));
    expect("an einen anderen Knochen", boltKnochenSetzen(in, neu, 7) && boltKnochen(in.teile[static_cast<std::size_t>(neu)]) == 7);
    const BoneMatrix lage2 = drehungZ(-45.0F, 1.0F, 2.0F, 3.0F);
    BoneMatrix l2;
    expect("neue Lage (alle Stufen)", boltLageSetzen(in, neu, lage2) && boltLage(in.teile[static_cast<std::size_t>(neu)], l2) && nah(l2, lage2) &&
                                          netzGleich(in.teile[static_cast<std::size_t>(neu)].stufen.back(), in.teile[static_cast<std::size_t>(neu)].stufen[0]));
    std::string bytes;
    expect("geschrieben", glmSchreiben(in, bytes));
    GlmModel m;
    expect("gelesen", readGlm(bytes, m));
    const int s = surfaceIndex(m, "*driver");
    BoneMatrix gelesen;
    expect("*driver in der Datei: Bolt-Flagge, Lage wie gesetzt", s >= 0 && (m.surfaces[static_cast<std::size_t>(s)].flags & GlmSurface::kFlagBolt) != 0U &&
                                                                      boltMatrixRigid(m, s, gelesen) && nah(gelesen, lage2));
    Inhalt leer;
    (void)ausGlm(slurp(pfad), leer);
    expect("model.glm ohne Fahrzeug-Bolts: beide Befunde", fahrzeugBoltsPruefen(leer).size() == 2U);
    // Vorlagen
    const auto sucht = [](Zielspiel z, const std::string& n) {
        const auto v = boltVorlagen(z);
        return std::any_of(v.begin(), v.end(), [&](const BoltVorlage& x) { return x.name == n; });
    };
    expect("Vorlagen Jedi Academy: *driver, *droidunit, *muzzle1..10, *exhaust1..4, *seat, *flash", sucht(Zielspiel::JkaSp, "*driver") &&
                                                                                                         sucht(Zielspiel::JkaSp, "*droidunit") &&
                                                                                                         sucht(Zielspiel::JkaSp, "*muzzle10") &&
                                                                                                         !sucht(Zielspiel::JkaSp, "*muzzle11") &&
                                                                                                         sucht(Zielspiel::JkaSp, "*exhaust4") &&
                                                                                                         !sucht(Zielspiel::JkaSp, "*exhaust5") &&
                                                                                                         sucht(Zielspiel::JkaSp, "*seat") && sucht(Zielspiel::JkaSp, "*flash"));
    expect("Vorlagen Movie Duels: *muzzle12, *exhaust12", sucht(Zielspiel::MdSp, "*muzzle12") && sucht(Zielspiel::MdSp, "*exhaust12"));
    expect("Vorlagen Mehrspieler: *trail1..4", sucht(Zielspiel::JkaMp, "*trail4") && !sucht(Zielspiel::JkaSp, "*trail1"));
    expect("Vorlagen Figur: *r_hand", sucht(Zielspiel::JkaSp, "*r_hand"));
}

// Ein kleines Fahrzeug: Rumpf (Kasten) und ein Bein (Kasten darunter)
Teil kasten(const std::string& name, float x0, float y0, float z0, float x1, float y1, float z1) {
    Teil t;
    t.name = name;
    std::vector<Ecke> e(8);
    for (int i = 0; i < 8; ++i) {
        e[static_cast<std::size_t>(i)].ort[0] = (i & 1) != 0 ? x1 : x0;
        e[static_cast<std::size_t>(i)].ort[1] = (i & 2) != 0 ? y1 : y0;
        e[static_cast<std::size_t>(i)].ort[2] = (i & 4) != 0 ? z1 : z0;
        e[static_cast<std::size_t>(i)].normale[2] = 1.0F;
        e[static_cast<std::size_t>(i)].gewicht[0] = 1.0F;
    }
    const std::vector<std::uint32_t> idx = {0, 2, 1, 1, 2, 3, 4, 5, 6, 5, 7, 6, 0, 1, 4, 1, 5, 4, 2, 6, 3, 3, 6, 7, 0, 4, 2, 2, 4, 6, 1, 3, 5, 3, 7, 5};
    t.stufen.resize(1);
    t.stufen[0].aufbauen(e, idx);
    return t;
}

void skelettTeil() {
    std::printf("Eigenes Skelett (Fahrzeug)\n");
    const std::string kopf = "models/players/cw_atrt/cw_atrt";
    GlaAnimation a = skelettNeu(kopf);
    expect("neues Skelett: model_root, ein Bild", a.bones.size() == 1U && a.numFrames == 1 && a.bones[0].name == "model_root");
    const float rumpfOrt[3] = {0.0F, 0.0F, 48.0F};
    const float beinOrt[3] = {0.0F, 8.0F, 40.0F};
    const int rumpf = knochenAnlegen(a, "body", 0, rumpfOrt);
    const int bein = knochenAnlegen(a, "leg_l", rumpf, beinOrt);
    expect("Knochen angelegt", rumpf == 1 && bein == 2 && a.bones[2].parent == 1 && a.bones[1].kinder == std::vector<int>{2});
    expect("doppelter Name abgewiesen", knochenAnlegen(a, "BODY", 0, rumpfOrt) == -1);
    expect("keine Schleife", !knochenEltern(a, rumpf, bein));
    const float tmp[3] = {1, 1, 1};
    const int weg = knochenAnlegen(a, "weg", bein, tmp);
    Inhalt in;
    in.herkunft = Herkunft::Glm;
    in.name = "cw_atrt.glm";
    in.stufenZahl = 1;
    in.teile.push_back(kasten("body", -16, -16, 40, 16, 16, 60));
    in.teile.push_back(kasten("leg_l", -4, 4, 0, 4, 12, 40));
    in.teile[1].eltern = 0;
    in.teile[0].kinder = {1};
    flaechenAnKnochen(in, {0}, rumpf);
    flaechenAnKnochen(in, {1}, weg);
    expect("Knochen mit Kindern laesst sich nicht entfernen", !knochenEntfernen(a, in, bein));
    expect("Blatt entfernt, die Ecken gehen an den Eltern", knochenEntfernen(a, in, weg) && a.bones.size() == 3U && in.teile[1].stufen[0].ecken[0].knochen[0] == bein);
    modellAnSkelett(in, a);
    expect("Modell gebunden: Pfad und Knochenzahl", in.glaPfad == kopf && in.knochenZahl == 3);
    // zwei gekeyte Bilder: das Bein dreht sich um die Huefte (lokale A = B X B^-1 wie beim Backen)
    std::vector<BoneMatrix> eins(3, BoneMatrix::identity());
    std::vector<BoneMatrix> zwei = eins;
    const BoneMatrix dreh = drehungY(20.0F);
    zwei[static_cast<std::size_t>(bein)] = a.bones[static_cast<std::size_t>(bein)].basePose * dreh * a.bones[static_cast<std::size_t>(bein)].basePoseInv;
    expect("zwei Bilder angehaengt", bildAnhaengen(a, eins) && bildAnhaengen(a, zwei) && a.numFrames == 3);
    std::string gla;
    std::string fehler;
    expect("GLA geschrieben", schreibGla(a, gla, &fehler));
    GlaAnimation b;
    expect("GLA gelesen: Knochen, Eltern, Bilder", readGla(gla, b) && b.bones.size() == 3U && b.bones[2].parent == 1 && b.numFrames == 3 &&
                                                       b.skeletonName == kopf);
    std::vector<BoneMatrix> w;
    b.worldMatrices(2, w);
    // die Spitze des Beins (0, 8, 0) in Bild 2: um die Huefte (0, 8, 40) gedreht
    float spitze[3] = {0.0F, 8.0F, 0.0F};
    float lokal[3];
    b.bones[2].basePoseInv.transform(spitze, lokal);
    float welt[3];
    w[2].transform(lokal, welt);
    const BoneMatrix soll = a.bones[2].basePose * dreh * a.bones[2].basePoseInv;
    float erwartet[3];
    soll.transform(spitze, erwartet);
    expect("Bild 2: das Bein steht wie gekeyt", std::abs(welt[0] - erwartet[0]) < 0.05F && std::abs(welt[1] - erwartet[1]) < 0.05F &&
                                                    std::abs(welt[2] - erwartet[2]) < 0.05F && std::abs(welt[0]) > 5.0F);
    b.worldMatrices(0, w);
    w[2].transform(lokal, welt);
    expect("Bild 0: Grundstellung", std::abs(welt[0]) < 0.05F && std::abs(welt[1] - 8.0F) < 0.05F && std::abs(welt[2]) < 0.05F);
    std::string glm;
    expect("GLM geschrieben", glmSchreiben(in, glm));
    GlmModel m;
    expect("GLM gelesen: Pfad der GLA und Knochenzahl im Kopf", readGlm(glm, m) && m.animFile == kopf && m.numBones == 3);
    expect("die Ecken des Beins am Bein", !m.surfaces.empty() && m.surfaces[1].verts[0].bones[0] == bein && m.surfaces[1].verts[0].weights[0] == 1.0F);
    expect("eigene GLA: Name im Kopf = Ordner (kein Befund im Einzelspieler)", eigeneGlaPruefen(b, kopf, Zielspiel::JkaSp).empty());
    // animation.cfg im Format ANIM_NAME erstesBild anzahlBilder schleifenBilder bilderJeSekunde
    AnimCfg c;
    c.setzen(AnimEntry{"BOTH_STAND1", 0, 1, 0, 20});
    c.setzen(AnimEntry{"BOTH_WALK1", 1, 2, 0, 10});
    c.setzen(AnimEntry{"ATRT_HOP", 1, 2, -1, 10});
    const std::string text = animCfgSchreiben(c);
    const AnimCfg d = animCfgLesen(text);
    expect("animation.cfg geschrieben und gelesen", d.eintraege().size() == 3U && d.eintraege()[1].name == "BOTH_WALK1" && d.eintraege()[1].fps == 10);
    const auto bJa = cfgPruefen(d.eintraege(), b.numFrames, text.size(), Zielspiel::JkaSp, animNamenSpiel(Zielspiel::JkaSp));
    expect("unbekannter Name gemeldet (das Spiel uebergeht die Zeile)", hat(bJa, CfgBefundArt::Unbekannt, "ATRT_HOP") && !hat(bJa, CfgBefundArt::Unbekannt, "BOTH_WALK1"));
    expect("alle Bilder in der GLA", std::none_of(bJa.begin(), bJa.end(), [](const CfgBefund& x) { return x.art == CfgBefundArt::HinterGla; }));
}

void namenTeil() {
    std::printf("Namen der animTable\n");
    const auto& ja = animNamenSpiel(Zielspiel::JkaSp);
    const auto& md = animNamenSpiel(Zielspiel::MdSp);
    expect("Jedi Academy SP 1544, MP 1543, JK2 1210, Movie Duels 3213 Namen", ja.size() == 1544U && animNamenSpiel(Zielspiel::JkaMp).size() == 1543U &&
                                                                                  animNamenSpiel(Zielspiel::Jk2Sp).size() == 1210U && md.size() == 3213U &&
                                                                                  animNamenSpiel(Zielspiel::MdMp).size() == 3213U);
    expect("erste und bekannte Namen", ja.front() == "FACE_TALK0" && std::find(ja.begin(), ja.end(), "BOTH_VS_IDLE") != ja.end());
    // ein Name, den nur Movie Duels kennt: bei Jedi Academy ein Befund, bei Movie Duels keiner
    std::string nurMd;
    for (const std::string& n : md) {
        if (std::find(ja.begin(), ja.end(), n) == ja.end()) {
            nurMd = n;
            break;
        }
    }
    const std::vector<AnimEntry> z = {AnimEntry{nurMd, 0, 1, 0, 20}};
    expect("nur in Movie Duels: " + nurMd, !nurMd.empty() && hat(cfgPruefen(z, 0, 10, Zielspiel::JkaSp, ja), CfgBefundArt::Unbekannt, nurMd) &&
                                              !hat(cfgPruefen(z, 0, 10, Zielspiel::MdSp, md), CfgBefundArt::Unbekannt, nurMd));
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1) { boltTeil(argv[1]); }
    skelettTeil();
    namenTeil();
    std::printf("%d Fehlschlaege\n", fails);
    return fails == 0 ? 0 : 1;
}
