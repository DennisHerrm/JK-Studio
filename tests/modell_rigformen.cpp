// modell_rigformen.cpp - AP13K: die Formen der Rig-Stile (bhed/modell/rigformen.h)
//
//   Masse      catMass gegen die Zahlen aus shanks CAT-Rig (wp\werkzeuge\rigs\cat_rig.json, hier von Hand eingetragen - nicht
//              aus der Tabelle des Programms gelesen): Laenge/Breite/Tiefe und Drahtfarbe je Koerperteil und Seite
//   Massstab   der _humanoid der Hand-Tabelle ist 1 (Bein 29,19); doppelt so gross ist 2
//   CAT        je Knochen mit Rolle ein Kasten: Oberschenkel in zwei Stuecken ueber den Drehknochen (wie LLeg11/LLeg12), der
//              Unterschenkel endet am Fussgelenk, Breite quer zur Figur, Becken und Brust als Nabe um das Gelenk, der Kopf ab dem
//              Gelenk nach oben; Fussplatten 20 x 10,6, Hand-IK-Ziele 20 x 20, Pole als Kreuze (verschoben, nicht am Knochen),
//              die Bodenform als Schmuck
//   Blender    Kreise um Becken und Wirbel, Quadrate an Hand und Fuss, Kreise als Pole, der Bodenkreis - die Masse aus
//              blender_rig.json (Form mal Laenge ihres Knochens)
//   Control    jeder Griff hat eine Form, rechts rot, links blau, Mitte gruen oder gelb; Pole als Kugeln, Fussplatten, Bodenkreis
//   Standard   keine Formen (der Entwurf zeichnet Zeichen am Gelenk)
//
// Aufruf: modellrigformentest
#include "bhed/gla.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/rig.h"
#include "bhed/modell/rigformen.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "rig_handtabelle.inc"

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", what.c_str());
    if (!ok) { ++fails; }
}

using namespace bhed;
using namespace bhed::modell;

// Ein Skelett aus der Hand-Tabelle (wie tests/modell_rig.cpp): Grundstellung = Verschiebung an den Ort, mal `faktor`.
GlaAnimation skelett(const RigHandSkelett& h, float faktor) {
    GlaAnimation g;
    g.name = h.name;
    g.skeletonName = h.name;
    g.scale = 0.64F;
    for (int i = 0; i < h.zahl; ++i) {
        GlaBone b;
        b.name = h.knochen[i].name;
        b.parent = h.knochen[i].eltern;
        b.basePose = BoneMatrix::identity();
        b.basePose.m[0][3] = h.knochen[i].x * faktor;
        b.basePose.m[1][3] = h.knochen[i].y * faktor;
        b.basePose.m[2][3] = h.knochen[i].z * faktor;
        b.basePoseInv = BoneMatrix::identity();
        b.basePoseInv.m[0][3] = -h.knochen[i].x * faktor;
        b.basePoseInv.m[1][3] = -h.knochen[i].y * faktor;
        b.basePoseInv.m[2][3] = -h.knochen[i].z * faktor;
        g.bones.push_back(b);
    }
    for (int i = 0; i < h.zahl; ++i) {
        if (g.bones[static_cast<std::size_t>(i)].parent >= 0) { g.bones[static_cast<std::size_t>(g.bones[static_cast<std::size_t>(i)].parent)].kinder.push_back(i); }
    }
    const std::uint16_t e[7] = {3 * 16383, 2 * 16383, 2 * 16383, 2 * 16383, 512 * 64, 512 * 64, 512 * 64};
    for (const std::uint16_t v : e) {
        g.bonePool.push_back(static_cast<std::uint8_t>(v & 0xFF));
        g.bonePool.push_back(static_cast<std::uint8_t>(v >> 8));
    }
    g.numFrames = 1;
    g.indexes.assign(g.bones.size(), 0U);
    return g;
}

const RigHandSkelett& tabelle(const char* name) {
    for (const RigHandSkelett& s : kRigHandSkelette) {
        if (std::strcmp(s.name, name) == 0) { return s; }
    }
    return kRigHandSkelette[0];
}

int knochen(const GlaAnimation& g, const char* n) { return boneIndex(g, n); }

std::array<float, 3> ort(const GlaAnimation& g, int b) {
    const BoneMatrix& m = g.bones[static_cast<std::size_t>(b)].basePose;
    return {m.m[0][3], m.m[1][3], m.m[2][3]};
}

float abstand(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

bool nah(float a, float b, float tol = 0.01F) { return std::fabs(a - b) <= tol; }

bool farbe(const std::uint8_t* f, int r, int g, int b) { return f[0] == r && f[1] == g && f[2] == b; }

bool mass(const CatMass& m, float l, float b, float t, int fr, int fg, int fb, bool nabe) {
    return nah(m.laenge, l, 0.001F) && nah(m.breite, b, 0.001F) && nah(m.tiefe, t, 0.001F) && farbe(m.farbe, fr, fg, fb) && m.nabe == nabe;
}

// Die Form eines Knochens (Skelett) bzw. die n-te Form eines Griffs
const RigForm* formVonKnochen(const RigFormen& f, int b) {
    for (const RigForm& x : f.formen) {
        if (x.skelett && x.knochen == b) { return &x; }
    }
    return nullptr;
}

std::vector<const RigForm*> formenVonGriff(const RigFormen& f, int griff) {
    std::vector<const RigForm*> aus;
    for (const RigForm& x : f.formen) {
        if (x.griff == griff) { aus.push_back(&x); }
    }
    return aus;
}

int griffNr(const Rig& rig, GriffArt a, Seite s) {
    for (std::size_t i = 0; i < rig.griffe.size(); ++i) {
        if (rig.griffe[i].art == a && rig.griffe[i].seite == s) { return static_cast<int>(i); }
    }
    return -1;
}

std::array<float, 3> mitte(const RigForm& f) {
    std::array<float, 3> m{};
    for (const auto& p : f.punkte) {
        for (int k = 0; k < 3; ++k) { m[static_cast<std::size_t>(k)] += p[static_cast<std::size_t>(k)]; }
    }
    for (float& c : m) { c /= static_cast<float>(std::max<std::size_t>(f.punkte.size(), 1U)); }
    return m;
}

// Ein Kasten: Ecken 0-3 am Anfang, 4-7 am Ende; Bit 0 die Breite, Bit 1 die Tiefe
bool kastenMasse(const RigForm& f, float l, float b, float t, float tol = 0.02F) {
    if (f.gestalt != FormGestalt::Kasten || f.punkte.size() != 8U || f.linien.size() != 12U) { return false; }
    return nah(abstand(f.punkte[0], f.punkte[4]), l, tol) && nah(abstand(f.punkte[0], f.punkte[1]), b, tol) && nah(abstand(f.punkte[0], f.punkte[2]), t, tol) &&
           nah(f.masse[0], l, tol) && nah(f.masse[1], b, tol) && nah(f.masse[2], t, tol);
}

std::array<float, 3> endMitte(const RigForm& f) {
    std::array<float, 3> m{};
    for (std::size_t i = 4; i < 8 && i < f.punkte.size(); ++i) {
        for (std::size_t k = 0; k < 3; ++k) { m[k] += f.punkte[i][k] * 0.25F; }
    }
    return m;
}

}  // namespace

int main() {
    // --- Masse und Farben aus cat_rig.json (von Hand: Knoten, length / width / depth bzw. height, Drahtfarbe) -------------------
    expect("CAT Hips: Nabe 14 x 19 x 10, 255/64/0", mass(catMass(Rolle::Becken, Seite::Mitte, 0, 3), 14.0F, 19.0F, 10.0F, 255, 64, 0, true));
    expect("CAT Spine1: 8 x 12,8 x 13,0286, 255/88/0", mass(catMass(Rolle::Wirbel, Seite::Mitte, 1, 3), 8.0F, 12.8F, 13.0286F, 255, 88, 0, false));
    expect("CAT Spine2: 8 x 13,072 x 14,4762, 255/181/0", mass(catMass(Rolle::Wirbel, Seite::Mitte, 2, 3), 8.0F, 13.072F, 14.4762F, 255, 181, 0, false));
    expect("CAT Chest (oberster Wirbel): Nabe 13,9 x 19,35 x 10, 255/200/0", mass(catMass(Rolle::Wirbel, Seite::Mitte, 3, 3), 13.9F, 19.35F, 10.0F, 255, 200, 0, true));
    expect("CAT Neck: 4,52 x 5,46 x 5,46, 255/100/0", mass(catMass(Rolle::Hals, Seite::Mitte, 0, 3), 4.52F, 5.46F, 5.46F, 255, 100, 0, false));
    expect("CAT Head: Nabe 12 x 12 x 12, 255/0/0", mass(catMass(Rolle::Kopf, Seite::Mitte, 0, 3), 12.0F, 12.0F, 12.0F, 255, 0, 0, true));
    expect("CAT Collarbone L: 9,328 x 8 x 10, 64/255/0", mass(catMass(Rolle::Schluessel, Seite::Links, 0, 3), 9.328F, 8.0F, 10.0F, 64, 255, 0, false));
    expect("CAT Collarbone R: 0/64/255", farbe(catMass(Rolle::Schluessel, Seite::Rechts, 0, 3).farbe, 0, 64, 255));
    expect("CAT LArm11: 8,5 x 5,544 x 6,16, 64/255/0", mass(catMass(Rolle::Oberarm, Seite::Links, 0, 3), 8.5F, 5.544F, 6.16F, 64, 255, 0, false));
    expect("CAT RArm11: 8,5 x 6,08 x 7,44, 0/64/255", mass(catMass(Rolle::Oberarm, Seite::Rechts, 0, 3), 8.5F, 6.08F, 7.44F, 0, 64, 255, false));
    expect("CAT LArm12 (Drehknochen) wie LArm11", mass(catMass(Rolle::OberarmTwist, Seite::Links, 0, 3), 8.5F, 5.544F, 6.16F, 64, 255, 0, false));
    expect("CAT LArm21: 6,82996 x 6,54 x 4,44", mass(catMass(Rolle::Unterarm, Seite::Links, 0, 3), 6.82996F, 6.54F, 4.44F, 64, 255, 0, false));
    expect("CAT RArm21: 6,82996 x 6,0 x 4,98", mass(catMass(Rolle::Unterarm, Seite::Rechts, 0, 3), 6.82996F, 6.0F, 4.98F, 0, 64, 255, false));
    expect("CAT Palm: 4 x 4 x 1,5", mass(catMass(Rolle::Hand, Seite::Links, 0, 3), 4.0F, 4.0F, 1.5F, 64, 255, 0, false));
    expect("CAT LLeg11: 11,5 x 8 x 5, 0/64/0", mass(catMass(Rolle::Oberschenkel, Seite::Links, 0, 3), 11.5F, 8.0F, 5.0F, 0, 64, 0, false));
    expect("CAT RLeg11: 11,5 x 8 x 5, 0/0/64", mass(catMass(Rolle::Oberschenkel, Seite::Rechts, 0, 3), 11.5F, 8.0F, 5.0F, 0, 0, 64, false));
    expect("CAT LLeg2: 22,6003 x 8 x 4", mass(catMass(Rolle::Unterschenkel, Seite::Links, 0, 3), 22.6003F, 8.0F, 4.0F, 0, 64, 0, false));
    expect("CAT Ankle: 8,6 x 8 x 3", mass(catMass(Rolle::Fuss, Seite::Rechts, 0, 3), 8.6F, 8.0F, 3.0F, 0, 0, 64, false));
    expect("CAT Toe: 3 x 8 x 2,1", mass(catMass(Rolle::Zehen, Seite::Links, 0, 3), 3.0F, 8.0F, 2.1F, 0, 64, 0, false));
    expect("CAT Tail: 17 x 2,25 x 2,25, 128/77/128", mass(catMass(Rolle::Anhang, Seite::Links, 0, 3), 17.0F, 2.25F, 2.25F, 128, 77, 128, false));

    const RigHandSkelett& h = tabelle("jka");
    const GlaAnimation g = skelett(h, 1.0F);
    const Rig rig = rigAnlegen(g, Inhalt{}, 0, {});
    const float m = rigMassstab(g, rig.erkennung);
    std::printf("  Massstab jka %.4f, Hoehe %.4f, Griffe %zu\n", static_cast<double>(m), static_cast<double>(rig.erkennung.hoehe), rig.griffe.size());
    expect("Massstab: der _humanoid ist 1", nah(m, 1.0F, 0.002F));
    {
        const GlaAnimation g2 = skelett(h, 2.0F);
        const Rig rig2 = rigAnlegen(g2, Inhalt{}, 0, {});
        expect("Massstab: doppelt so gross ist 2", nah(rigMassstab(g2, rig2.erkennung), 2.0F, 0.004F));
    }

    // --- CAT: das Skelett als Kaesten ----------------------------------------------------------------------------------------
    const RigFormen cat = rigFormenSkelettCat(g, rig);
    const float e = kCatEinheit * m;   // eine Einheit des CAT-Rigs an dieser Figur
    int mitRolle = 0;
    for (const KnochenErkennung& k : rig.erkennung.knochen) {
        mitRolle += k.rolle != Rolle::Keine && k.rolle != Rolle::Wurzel && k.rolle != Rolle::Bewegung ? 1 : 0;
    }
    int kaesten = 0;
    bool alleSkelett = true;
    for (const RigForm& f : cat.formen) {
        kaesten += f.gestalt == FormGestalt::Kasten ? 1 : 0;
        alleSkelett = alleSkelett && f.skelett && f.knochen >= 0 && f.innen && f.bindung == FormBindung::Knochen;
    }
    std::printf("  CAT-Skelett: %zu Formen, %d Kaesten, %d Knochen mit Rolle\n", cat.formen.size(), kaesten, mitRolle);
    expect("CAT: je Knochen mit Rolle (ohne Wurzel und Bewegung) genau ein Kasten, starr am Knochen, das Innere zaehlt",
           mitRolle > 40 && kaesten == mitRolle && static_cast<int>(cat.formen.size()) == mitRolle && alleSkelett);
    {
        const int femur = knochen(g, "lfemurYZ");
        const int twist = knochen(g, "lfemurX");
        const int tibia = knochen(g, "ltibia");
        const int talus = knochen(g, "ltalus");
        const RigForm* f1 = formVonKnochen(cat, femur);
        const RigForm* f2 = formVonKnochen(cat, twist);
        const RigForm* f3 = formVonKnochen(cat, tibia);
        const float l1 = abstand(ort(g, femur), ort(g, twist));
        const float l2 = abstand(ort(g, twist), ort(g, tibia));
        const float l3 = abstand(ort(g, tibia), ort(g, talus));
        expect("CAT Oberschenkel L: zwei Stuecke ueber den Drehknochen (wie LLeg11/LLeg12), Breite 8 und Tiefe 5 Einheiten des Rigs",
               f1 != nullptr && f2 != nullptr && kastenMasse(*f1, l1, 8.0F * e, 5.0F * e) && kastenMasse(*f2, l2, 8.0F * e, 5.0F * e) && nah(l1 + l2, 14.72F, 0.02F));
        expect("CAT Unterschenkel L: vom Knie bis zum Fussgelenk, 8 x 4", f3 != nullptr && kastenMasse(*f3, l3, 8.0F * e, 4.0F * e) &&
                                                                               abstand(endMitte(*f3), ort(g, talus)) < 0.02F);
        expect("CAT Bein L dunkelgruen 0/64/0, Bein R dunkelblau 0/0/64",
               f1 != nullptr && farbe(f1->farbe, 0, 64, 0) && formVonKnochen(cat, knochen(g, "rfemurYZ")) != nullptr &&
                   farbe(formVonKnochen(cat, knochen(g, "rfemurYZ"))->farbe, 0, 0, 64));
        // die Breite liegt quer zur Figur (Seitenachse X): Ecke 1 gegen Ecke 0
        expect("CAT Oberschenkel: die Breite liegt quer zur Figur", f1 != nullptr && std::fabs(f1->punkte[1][0] - f1->punkte[0][0]) > 0.9F * 8.0F * e);
        expect("CAT: der Kasten zeigt den FK-Griff seines Knochens", f1 != nullptr && f1->griff == rig.griffVon(femur) && f1->griff >= 0);
    }
    {
        const int pelvis = knochen(g, "pelvis");
        const RigForm* f = formVonKnochen(cat, pelvis);
        // Nabe: 10 hoch (oben), 19 breit (quer), 14 tief (vorn) - Laenge der Tabelle ist die Tiefe
        expect("CAT Becken: Nabe mittig um das Gelenk, 19 breit, 14 tief, 10 hoch, 255/64/0",
               f != nullptr && f->punkte.size() == 8U && abstand(mitte(*f), ort(g, pelvis)) < 0.02F && nah(abstand(f->punkte[0], f->punkte[4]), 10.0F * e, 0.02F) &&
                   nah(abstand(f->punkte[0], f->punkte[1]), 19.0F * e, 0.02F) && nah(abstand(f->punkte[0], f->punkte[2]), 14.0F * e, 0.02F) && farbe(f->farbe, 255, 64, 0));
        const RigForm* brust = formVonKnochen(cat, knochen(g, "thoracic"));
        expect("CAT Brust (thoracic): Nabe 19,35 breit, 255/200/0", brust != nullptr && nah(abstand(brust->punkte[0], brust->punkte[1]), 19.35F * e, 0.02F) &&
                                                                         farbe(brust->farbe, 255, 200, 0));
        const RigForm* w1 = formVonKnochen(cat, knochen(g, "lower_lumbar"));
        const RigForm* w2 = formVonKnochen(cat, knochen(g, "upper_lumbar"));
        expect("CAT Wirbel: lower_lumbar 255/88/0 bis upper_lumbar, upper_lumbar 255/181/0 bis thoracic",
               w1 != nullptr && w2 != nullptr && farbe(w1->farbe, 255, 88, 0) && farbe(w2->farbe, 255, 181, 0) &&
                   abstand(endMitte(*w1), ort(g, knochen(g, "upper_lumbar"))) < 0.02F && abstand(endMitte(*w2), ort(g, knochen(g, "thoracic"))) < 0.02F);
        const RigForm* kopf = formVonKnochen(cat, knochen(g, "cranium"));
        const std::array<float, 3> ko = ort(g, knochen(g, "cranium"));
        expect("CAT Kopf: Wuerfel 12 ab dem Gelenk nach oben, rot",
               kopf != nullptr && farbe(kopf->farbe, 255, 0, 0) && nah(abstand(kopf->punkte[0], kopf->punkte[4]), 12.0F * e, 0.02F) &&
                   nah(mitte(*kopf)[2], ko[2] + 6.0F * e, 0.05F));
        const RigForm* la = formVonKnochen(cat, knochen(g, "lhumerus"));
        const RigForm* ra = formVonKnochen(cat, knochen(g, "rhumerus"));
        expect("CAT Arm L hellgruen 64/255/0 (5,544 breit), Arm R blau 0/64/255 (6,08 breit)",
               la != nullptr && ra != nullptr && farbe(la->farbe, 64, 255, 0) && farbe(ra->farbe, 0, 64, 255) && nah(la->masse[1], 5.544F * e, 0.02F) &&
                   nah(ra->masse[1], 6.08F * e, 0.02F));
    }
    // --- CAT: IK-Ziele, Pole, Bodenform ------------------------------------------------------------------------------------
    {
        const RigFormen gr = rigFormenGriffe(g, rig, nullptr, 0, RigStil::Cat);
        const int fl = griffNr(rig, GriffArt::Fuss, Seite::Links);
        const int hr = griffNr(rig, GriffArt::Hand, Seite::Rechts);
        const int pk = griffNr(rig, GriffArt::PolKnie, Seite::Links);
        const int pe = griffNr(rig, GriffArt::PolEllbogen, Seite::Rechts);
        const auto platte = formenVonGriff(gr, fl);
        const auto hand = formenVonGriff(gr, hr);
        const auto polK = formenVonGriff(gr, pk);
        const auto polE = formenVonGriff(gr, pe);
        expect("CAT Fussplatte L: Rechteck 20 x 10,6, 0/64/0, am Fuss",
               fl >= 0 && platte.size() == 1U && platte[0]->gestalt == FormGestalt::Rechteck && nah(platte[0]->masse[0], 20.0F * e, 0.02F) &&
                   nah(platte[0]->masse[1], 10.6F * e, 0.02F) && farbe(platte[0]->farbe, 0, 64, 0) && platte[0]->bindung == FormBindung::Knochen);
        expect("CAT Hand-IK-Ziel R: Rechteck 20 x 20, 0/64/255",
               hr >= 0 && hand.size() == 1U && hand[0]->gestalt == FormGestalt::Rechteck && nah(hand[0]->masse[0], 20.0F * e, 0.02F) &&
                   nah(hand[0]->masse[1], 20.0F * e, 0.02F) && farbe(hand[0]->farbe, 0, 64, 255));
        expect("CAT Pole: Kreuze 10,6 in der Farbe des Glieds, an den Ort des Pols verschoben",
               pk >= 0 && pe >= 0 && polK.size() == 1U && polE.size() == 1U && polK[0]->gestalt == FormGestalt::Kreuz && polK[0]->bindung == FormBindung::Ort &&
                   nah(polK[0]->masse[0], 10.6F * e, 0.02F) && farbe(polK[0]->farbe, 0, 64, 0) && farbe(polE[0]->farbe, 0, 64, 255) &&
                   nah(polK[0]->anker[0], rig.griffe[static_cast<std::size_t>(pk)].ort[0], 1e-4F) && nah(polK[0]->anker[2], rig.griffe[static_cast<std::size_t>(pk)].ort[2], 1e-4F));
        int boden = 0;
        int fk = 0;
        for (const RigForm& f : gr.formen) {
            // ENT3B (B17): die Bodenform ist der Griff der Wurzel (bis dahin Schmuck)
            boden += f.gestalt == FormGestalt::Bodenform && f.wurzel && !f.schmuck && f.griff >= 0 && rig.griffe[static_cast<std::size_t>(f.griff)].art == GriffArt::Wurzel ? 1 : 0;
            const bool fkArt = f.griff >= 0 && !rig.griffe[static_cast<std::size_t>(f.griff)].ik && rig.griffe[static_cast<std::size_t>(f.griff)].art != GriffArt::Hand &&
                               rig.griffe[static_cast<std::size_t>(f.griff)].art != GriffArt::Fuss && rig.griffe[static_cast<std::size_t>(f.griff)].art != GriffArt::Saebelgriff &&
                               rig.griffe[static_cast<std::size_t>(f.griff)].art != GriffArt::Wurzel;
            fk += fkArt ? 1 : 0;
        }
        expect("CAT: die Bodenform als Griff der Wurzel (ENT3B), 135/59/8; keine eigene Form fuer die uebrigen FK-Griffe (das sind die Kaesten)", boden == 1 && fk == 0);
        expect("CAT: der Boden liegt unter dem Fussgelenk", gr.boden < ort(g, knochen(g, "ltalus"))[2] - 1.0F);
    }
    // --- Blender ------------------------------------------------------------------------------------------------------------
    {
        const RigFormen gr = rigFormenGriffe(g, rig, nullptr, 0, RigStil::Blender);
        const auto becken = formenVonGriff(gr, griffNr(rig, GriffArt::Becken, Seite::Mitte));
        const auto handL = formenVonGriff(gr, griffNr(rig, GriffArt::Hand, Seite::Links));
        const auto fussR = formenVonGriff(gr, griffNr(rig, GriffArt::Fuss, Seite::Rechts));
        const auto polE = formenVonGriff(gr, griffNr(rig, GriffArt::PolEllbogen, Seite::Links));
        const auto polK = formenVonGriff(gr, griffNr(rig, GriffArt::PolKnie, Seite::Rechts));
        const auto arm = formenVonGriff(gr, griffNr(rig, GriffArt::Oberarm, Seite::Links));
        expect("Blender Becken: Kreis Radius 10,47 (BoneShape4 4,08902 x 2,56)", becken.size() == 1U && becken[0]->gestalt == FormGestalt::Ring &&
                                                                                       nah(becken[0]->masse[0], 10.468F * m, 0.02F));
        expect("Blender Hand: Quadrat halbe Kante 2,476 (boneshape3 0,36716 x 6,74414)", handL.size() == 1U && handL[0]->gestalt == FormGestalt::Rechteck &&
                                                                                               nah(handL[0]->masse[0], 2.0F * 2.4762F * m, 0.02F));
        expect("Blender Fuss: Quadrat halbe Kante 3,556 (boneshape3 0,36716 x 9,6851)", fussR.size() == 1U && fussR[0]->gestalt == FormGestalt::Rechteck &&
                                                                                              nah(fussR[0]->masse[0], 2.0F * 3.5560F * m, 0.02F));
        expect("Blender Pole: Kreis 1,938 am Ellbogen, 2,464 am Knie, gestrichelt verbunden, verschoben",
               polE.size() == 1U && polK.size() == 1U && polE[0]->gestalt == FormGestalt::Ring && nah(polE[0]->masse[0], 1.9379F * m, 0.01F) &&
                   nah(polK[0]->masse[0], 2.4643F * m, 0.01F) && polE[0]->bindung == FormBindung::Ort);
        expect("Blender: Oberarm ohne eigene Form (der Oktaeder des Knochens ist der Griff)", arm.empty());
        int wirbel = 0;
        int boden = 0;
        float bodenR = 0.0F;
        float wirbelR[2] = {0.0F, 0.0F};
        for (const RigForm& f : gr.formen) {
            if (f.wurzel && !f.schmuck && f.griff >= 0 && f.gestalt == FormGestalt::Ring) {   // ENT3B: Griff der Wurzel
                ++boden;
                bodenR = f.masse[0];
            }
            if (f.griff >= 0 && rig.griffe[static_cast<std::size_t>(f.griff)].art == GriffArt::Wirbel && f.gestalt == FormGestalt::Ring) {
                if (wirbel < 2) { wirbelR[wirbel] = f.masse[0]; }
                ++wirbel;
            }
        }
        std::printf("  Blender: Wirbel-Kreise %d (%.3f, %.3f), Bodenkreis %.3f\n", wirbel, static_cast<double>(wirbelR[0]), static_cast<double>(wirbelR[1]),
                    static_cast<double>(bodenR));
        expect("Blender Bodenkreis: Radius 26,37 (boneshape.001 10,30188 x 2,56), Griff der Wurzel (ENT3B)", boden == 1 && nah(bodenR, 26.3728F * m, 0.02F));
        expect("Blender Wirbel: Kreise 9,20 (spine 0,893) und 7,71 (upper_spine 0,74844)", wirbel >= 2 && nah(wirbelR[0], 9.1996F * m, 0.02F) &&
                                                                                                 nah(wirbelR[1], 7.7103F * m, 0.02F));
    }
    // --- Control Rig -----------------------------------------------------------------------------------------------------------
    {
        const RigFormen gr = rigFormenGriffe(g, rig, nullptr, 0, RigStil::ControlRig);
        std::vector<int> jeGriff(rig.griffe.size(), 0);
        bool farbenOk = true;
        int boden = 0;
        for (const RigForm& f : gr.formen) {
            if (f.schmuck) { continue; }   // die Marke ueber dem Kopf
            if (f.wurzel) {   // ENT3B: der Bodenkreis ist der Griff der Wurzel (weiss wie bisher, nicht nach Seite gefaerbt)
                boden += f.gestalt == FormGestalt::Ring ? 1 : 0;
                if (f.griff >= 0 && static_cast<std::size_t>(f.griff) < rig.griffe.size()) { ++jeGriff[static_cast<std::size_t>(f.griff)]; }
                continue;
            }
            if (f.griff < 0 || static_cast<std::size_t>(f.griff) >= rig.griffe.size()) {
                farbenOk = false;
                continue;
            }
            ++jeGriff[static_cast<std::size_t>(f.griff)];
            const Seite s = rig.griffe[static_cast<std::size_t>(f.griff)].seite;
            // rechts rot (Rot ueberwiegt Blau), links blau (Blau ueberwiegt Rot), Mitte gruen oder gelb (Gruen stark, wenig Blau)
            const bool ok = s == Seite::Rechts ? f.farbe[0] > f.farbe[2] + 60 : (s == Seite::Links ? f.farbe[2] > f.farbe[0] + 60 : (f.farbe[1] > 180 && f.farbe[2] < 120));
            farbenOk = farbenOk && ok;
        }
        const bool jeder = std::all_of(jeGriff.begin(), jeGriff.end(), [](int n) { return n >= 1; });
        std::printf("  Control Rig: %zu Formen fuer %zu Griffe, Bodenkreise %d\n", gr.formen.size(), rig.griffe.size(), boden);
        expect("Control Rig: jeder Griff hat seine Form", !rig.griffe.empty() && jeder);
        expect("Control Rig: rechts rot, links blau, Mitte gruen oder gelb", farbenOk);
        const auto kopf = formenVonGriff(gr, griffNr(rig, GriffArt::Kopf, Seite::Mitte));
        const auto pol = formenVonGriff(gr, griffNr(rig, GriffArt::PolKnie, Seite::Links));
        const auto fuss = formenVonGriff(gr, griffNr(rig, GriffArt::Fuss, Seite::Links));
        const auto schl = formenVonGriff(gr, griffNr(rig, GriffArt::Schluessel, Seite::Rechts));
        const auto becken = formenVonGriff(gr, griffNr(rig, GriffArt::Becken, Seite::Mitte));
        expect("Control Rig Kopf: Ring um das Gelenk", !kopf.empty() && kopf[0]->gestalt == FormGestalt::Ring && kopf[0]->masse[0] > 2.0F);
        expect("Control Rig Pol: Kugel, verschoben", pol.size() == 1U && pol[0]->gestalt == FormGestalt::Kugel && pol[0]->bindung == FormBindung::Ort);
        bool platte = false;
        bool ring = false;
        for (const RigForm* f : fuss) {
            platte = platte || f->gestalt == FormGestalt::Rechteck;
            ring = ring || f->gestalt == FormGestalt::Ring;
        }
        expect("Control Rig Fuss: Platte auf dem Boden und Ring um das Gelenk", platte && ring);
        expect("Control Rig Schluesselbein: Stiel mit Kreis", schl.size() == 1U && schl[0]->gestalt == FormGestalt::Lutscher);
        expect("Control Rig Becken: Band (Ring, Kasten mit Kreuz, Pfeile), gelb", becken.size() == 1U && becken[0]->gestalt == FormGestalt::Band && becken[0]->farbe[0] > 200 &&
                                                                                      becken[0]->farbe[1] > 180);
        expect("Control Rig: ein Bodenkreis als Griff der Wurzel (ENT3B)", boden == 1);
    }
    expect("Standard: keine Formen", rigFormenGriffe(g, rig, nullptr, 0, RigStil::Standard).formen.empty());
    std::printf("%s (%d)\n", fails == 0 ? "modell_rigformen: alles ok" : "modell_rigformen: FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
