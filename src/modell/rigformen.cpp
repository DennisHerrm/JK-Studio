// modell/rigformen.cpp - AP13K: die Formen der Rig-Stile in der Grundstellung (siehe bhed/modell/rigformen.h)
//
// Die Zahlen der Tabellen stammen aus den ausgelesenen Rigs (wp\werkzeuge\rigs\cat_rig.json, blender_rig.json; der Unit-Test
// tests/modell_rigformen.cpp traegt sie noch einmal von Hand). Die Masse des Control Rigs (kControl) sind Annahmen nach shanks
// Bild: Anteile einer 64 Einheiten hohen Figur, die Ringe weiten sich auf die Haut der Figur (hautWeite), wenn ein Modell da ist.
#include "bhed/modell/rigformen.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace bhed::modell {
namespace {

using V3 = std::array<float, 3>;

constexpr float kPi = 3.14159265358979F;

V3 ortVon(const GlaAnimation& g, int b) {
    const BoneMatrix& m = g.bones[static_cast<std::size_t>(b)].basePose;
    return {m.m[0][3], m.m[1][3], m.m[2][3]};
}
V3 plus(const V3& a, const V3& b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
V3 minus(const V3& a, const V3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
V3 mal(const V3& a, float f) { return {a[0] * f, a[1] * f, a[2] * f}; }
float punkt(const V3& a, const V3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
V3 kreuz(const V3& a, const V3& b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
float laenge(const V3& a) { return std::sqrt(punkt(a, a)); }
V3 norm(const V3& a, const V3& ersatz) {
    const float l = laenge(a);
    return l > 1e-5F ? mal(a, 1.0F / l) : ersatz;
}

struct Rgb {
    std::uint8_t r, g, b;
};

// Drahtfarben des CAT-Rigs je Koerperteil (cat_rig.json "drahtfarbe")
constexpr Rgb kCatBecken{255, 64, 0};      // JK_humanoidHips
constexpr Rgb kCatWirbel1{255, 88, 0};     // Spine1
constexpr Rgb kCatWirbel2{255, 181, 0};    // Spine2
constexpr Rgb kCatBrust{255, 200, 0};      // Chest
constexpr Rgb kCatHals{255, 100, 0};       // Neck
constexpr Rgb kCatKopf{255, 0, 0};         // Head
constexpr Rgb kCatGesicht{255, 144, 0};    // Jaw
constexpr Rgb kCatArmL{64, 255, 0};        // LArm*, LArmCollarbone, LArmIKTarget
constexpr Rgb kCatArmR{0, 64, 255};        // RArm*
constexpr Rgb kCatBeinL{0, 64, 0};         // LLeg*, LLegPlatform
constexpr Rgb kCatBeinR{0, 0, 64};         // RLeg*
constexpr Rgb kCatAnhang{128, 77, 128};    // LTail1, RTail1
constexpr Rgb kCatBoden{135, 59, 8};       // CATParent
constexpr Rgb kCatSonst{160, 160, 160};    // ohne Vorbild (Knochen ohne Entsprechung im CAT-Rig)

// Fussplatte (IKTarget der Beine: length 20, width 10,6; ihre Mitte liegt 3,5 vor dem Fussgelenk: LLegPlatform y -13,63 gegen
// LLegAnkle y -10,13), Hand-IK-Ziel (20 x 20, am Ende der Handflaeche: 4), UpVectorNode (Kreuz 10,6), CATParent (Rahmen
// x +-56,25, vorn 62,17, hinten 39,56), Fussgelenk ueber dem Boden 6,21
constexpr float kCatPlatteL = 20.0F;
constexpr float kCatPlatteB = 10.6F;
constexpr float kCatPlatteVor = 3.5F;
constexpr float kCatHandZiel = 20.0F;
constexpr float kCatKreuz = 10.6F;
constexpr float kCatBodenHalb = 56.25F;
constexpr float kCatBodenVorn = 62.17F;
constexpr float kCatBodenHinten = 39.56F;
constexpr float kCatKnoechel = 6.21F;

// Blender-IK-Rig (blender_rig.json): Form mal Laenge des Knochens, in Einheiten des Spiels am _humanoid
constexpr float kBlBecken = 4.08902F * 2.56F;         // BoneShape4 an pelvis
constexpr float kBlBeckenHoch = 1.0905F * 2.56F;      // ... um diesen Weg entlang des Knochens (nach oben) versetzt
constexpr float kBlWirbelUnten = 10.30188F * 0.893F;  // boneshape.001 an spine
constexpr float kBlWirbelOben = 10.30188F * 0.74844F; // boneshape.001 an upper_spine
constexpr float kBlBoden = 10.30188F * 2.56F;         // boneshape.001 an model_root
constexpr float kBlPolKnie = 0.28735F * 8.57607F;     // boneshape an lknee_targeet
constexpr float kBlPolEllbogen = 0.28735F * 6.74414F; // boneshape an lelbow_target
constexpr float kBlHand = 0.36716F * 6.74414F;        // boneshape3 an lhand_IK (halbe Kante)
constexpr float kBlFuss = 0.36716F * 9.6851F;         // boneshape3 an lheel_IK (halbe Kante)

// Seitenfarben: Blender-Stil wie die Griffe des Programms bis rc611 (links blau, rechts gruen, Mitte gelb - die Regel von Biped);
// Control Rig nach shanks Bild: rechts rot, links blau, Mitte gruen, Becken und Brust gelb, Pole und Fussplatten orange / hellblau
constexpr Rgb kBlLinks{70, 140, 255};
constexpr Rgb kBlRechts{70, 205, 95};
constexpr Rgb kBlMitte{235, 200, 60};
constexpr Rgb kCrRechts{235, 60, 50};
constexpr Rgb kCrLinks{50, 110, 240};
constexpr Rgb kCrMitte{60, 215, 90};
constexpr Rgb kCrGelb{245, 215, 45};
constexpr Rgb kCrPolRechts{240, 140, 40};
constexpr Rgb kCrPolLinks{70, 170, 250};
constexpr Rgb kCrBoden{235, 235, 235};

// Control Rig: Masse in Einheiten des Spiels an einer Figur wie der _humanoid (Annahmen nach dem Bild; die Ringe weiten sich
// mit hautWeite auf die Figur)
struct ControlMasse {
    float becken = 7.2F;
    float wirbel = 7.0F;
    float brust = 8.4F;
    float hals = 3.0F;
    float kopf = 5.4F;
    float kopfHoch = 4.5F;       // der Ring des Kopfs liegt so weit ueber dem Gelenk
    float kopfPunkt = 11.0F;     // die Marke ueber dem Kopf
    float oberarm = 2.8F;
    float unterarm = 2.4F;
    float hand = 2.2F;
    float oberschenkel = 3.8F;
    float unterschenkel = 3.0F;
    float knoechel = 2.6F;
    float zehen = 1.6F;
    float pol = 1.1F;
    float platteL = 10.5F;
    float platteB = 6.0F;
    float stiel = 5.0F;
    float stielKreis = 1.4F;
    float marke = 1.2F;
    float boden = 20.5F;
};
constexpr ControlMasse kControl{};

void farbe(RigForm& f, Rgb c) {
    f.farbe[0] = c.r;
    f.farbe[1] = c.g;
    f.farbe[2] = c.b;
}

bool istIdentitaet(const BoneMatrix& m) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (std::fabs(m.m[i][j] - (i == j ? 1.0F : 0.0F)) > 1e-5F) { return false; }
        }
    }
    return true;
}

// Was die Formen von der Figur wissen muessen
struct Figur {
    const GlaAnimation* g = nullptr;
    const Erkennung* e = nullptr;
    int n = 0;
    V3 seite{1.0F, 0.0F, 0.0F};    // nach links
    V3 vorn{0.0F, -1.0F, 0.0F};
    V3 oben{0.0F, 0.0F, 1.0F};
    float m = 1.0F;                // Massstab gegen den _humanoid
    std::vector<int> wirbel;       // von unten nach oben
    [[nodiscard]] Rolle rolle(int b) const { return e->knochen[static_cast<std::size_t>(b)].rolle; }
    [[nodiscard]] Seite seiteVon(int b) const { return e->knochen[static_cast<std::size_t>(b)].seite; }
    [[nodiscard]] V3 ort(int b) const { return ortVon(*g, b); }
    [[nodiscard]] V3 seiteNach(Seite s) const { return s == Seite::Rechts ? mal(seite, -1.0F) : seite; }
};

// Der erste Knochen mit Rolle und Seite (-1 keiner)
int knochenMit(const Figur& f, Rolle r, Seite s) {
    for (int b = 0; b < f.n; ++b) {
        if (f.rolle(b) == r && f.seiteVon(b) == s) { return b; }
    }
    return -1;
}

// Wo ein Glied nach dem Knochen `b` weitergeht: der naechste Nachfahr mit der Rolle (gleiche Seite), sonst - JKA haengt Hand und
// Unterarm nicht untereinander, den Oberarm nicht unter das Schluesselbein - der raeumlich naechste Knochen mit Rolle und Seite.
int nachfolger(const Figur& f, int b, Rolle r) {
    const Seite s = f.seiteVon(b);
    std::vector<int> offen = f.g->bones[static_cast<std::size_t>(b)].kinder;
    for (std::size_t i = 0; i < offen.size() && i < 4096U; ++i) {
        const int k = offen[i];
        if (k < 0 || k >= f.n) { continue; }
        if (f.rolle(k) == r && f.seiteVon(k) == s) { return k; }
        for (const int kk : f.g->bones[static_cast<std::size_t>(k)].kinder) { offen.push_back(kk); }
    }
    int best = -1;
    float bd = 0.0F;
    const V3 o = f.ort(b);
    for (int k = 0; k < f.n; ++k) {
        if (k == b || f.rolle(k) != r || f.seiteVon(k) != s) { continue; }
        const float d = laenge(minus(f.ort(k), o));
        if (best < 0 || d < bd) {
            best = k;
            bd = d;
        }
    }
    return best;
}

// Liegt der Drehknochen `t` auf der Strecke a - c (zwischen 15 % und 85 %, hoechstens ein Viertel der Strecke daneben)?
bool dazwischen(const V3& a, const V3& c, const V3& t) {
    const V3 ac = minus(c, a);
    const float l2 = punkt(ac, ac);
    if (l2 < 1e-6F) { return false; }
    const float u = punkt(minus(t, a), ac) / l2;
    if (u < 0.15F || u > 0.85F) { return false; }
    return laenge(minus(t, plus(a, mal(ac, u)))) < 0.25F * std::sqrt(l2);
}

Figur figurAus(const GlaAnimation& g, const Erkennung& e) {
    Figur f;
    f.g = &g;
    f.e = &e;
    f.n = static_cast<int>(std::min(g.bones.size(), e.knochen.size()));
    f.seite = {0.0F, 0.0F, 0.0F};
    f.seite[static_cast<std::size_t>(e.seitenAchse == 1 ? 1 : 0)] = e.linksVorzeichen < 0.0F ? -1.0F : 1.0F;
    float v[3];
    figurVorn(g, e, v);
    f.vorn = {v[0], v[1], v[2]};
    f.m = rigMassstab(g, e);
    for (int b = 0; b < f.n; ++b) {
        if (f.rolle(b) == Rolle::Wirbel) { f.wirbel.push_back(b); }
    }
    std::sort(f.wirbel.begin(), f.wirbel.end(), [&](int a, int b) {
        const float za = f.ort(a)[2];
        const float zb = f.ort(b)[2];
        return za != zb ? za < zb : a < b;
    });
    return f;
}

int wirbelNummer(const Figur& f, int b) {
    const auto it = std::find(f.wirbel.begin(), f.wirbel.end(), b);
    return it == f.wirbel.end() ? 0 : static_cast<int>(it - f.wirbel.begin()) + 1;
}

// Das Gelenk, bis zu dem der Kasten des Knochens reicht (-1: ein Ende - die Laenge kommt aus der Tabelle)
int segmentEnde(const Figur& f, int b) {
    const V3 a = f.ort(b);
    const auto mitDreh = [&](Rolle dreh, Rolle weiter) {
        const int w = nachfolger(f, b, weiter);
        const int d = nachfolger(f, b, dreh);
        if (d >= 0 && w >= 0 && dazwischen(a, f.ort(w), f.ort(d))) { return d; }
        return w;
    };
    switch (f.rolle(b)) {
        case Rolle::Wirbel: {
            const int nr = wirbelNummer(f, b);
            if (nr > 0 && static_cast<std::size_t>(nr) < f.wirbel.size()) { return f.wirbel[static_cast<std::size_t>(nr)]; }
            const int hals = knochenMit(f, Rolle::Hals, Seite::Mitte);
            return hals >= 0 ? hals : knochenMit(f, Rolle::Kopf, Seite::Mitte);
        }
        case Rolle::Hals: return knochenMit(f, Rolle::Kopf, Seite::Mitte);
        case Rolle::Schluessel: return nachfolger(f, b, Rolle::Oberarm);
        case Rolle::Oberarm: return mitDreh(Rolle::OberarmTwist, Rolle::Unterarm);
        case Rolle::OberarmTwist: return nachfolger(f, b, Rolle::Unterarm);
        case Rolle::Unterarm: return mitDreh(Rolle::UnterarmTwist, Rolle::Hand);
        case Rolle::UnterarmTwist: return nachfolger(f, b, Rolle::Hand);
        case Rolle::Oberschenkel: return mitDreh(Rolle::OberschenkelTwist, Rolle::Unterschenkel);
        case Rolle::OberschenkelTwist: return nachfolger(f, b, Rolle::Unterschenkel);
        case Rolle::Unterschenkel: return nachfolger(f, b, Rolle::Fuss);
        case Rolle::Fuss: return nachfolger(f, b, Rolle::Zehen);
        case Rolle::Finger: {
            for (const int k : f.g->bones[static_cast<std::size_t>(b)].kinder) {
                if (k >= 0 && k < f.n && f.rolle(k) == Rolle::Finger) { return k; }
            }
            return -1;
        }
        default: return -1;
    }
}

// Die Richtung eines Knochens ohne Nachfolger: seine eigene X-Achse in der Grundstellung (die Skelette der Spiele legen die
// Laenge auf +X - ltalus zeigt nach vorn unten, ltail nach unten, cranium nach oben); ohne Drehung in der Grundstellung (ein
// Skelett nur aus Orten) weiter in der Richtung vom Elterngelenk.
V3 eigeneRichtung(const Figur& f, int b) {
    const BoneMatrix& m = f.g->bones[static_cast<std::size_t>(b)].basePose;
    if (!istIdentitaet(m)) { return norm({m.m[0][0], m.m[1][0], m.m[2][0]}, f.oben); }
    const int p = f.g->bones[static_cast<std::size_t>(b)].parent;
    if (p >= 0 && p < f.n) { return norm(minus(f.ort(b), f.ort(p)), f.oben); }
    return f.oben;
}

// Quer und tief zu einer Laengsrichtung: quer liegt moeglichst auf der Seitenachse der Figur (CAT: width quer zur Figur), bei
// einem Knochen entlang der Seitenachse (Schluesselbein, Arm in T-Pose) auf der Achse nach vorn.
void rahmen(const Figur& f, const V3& x, V3& quer, V3& tief) {
    V3 q = minus(f.seite, mal(x, punkt(f.seite, x)));
    if (laenge(q) < 0.3F) { q = minus(f.vorn, mal(x, punkt(f.vorn, x))); }
    quer = norm(q, f.vorn);
    tief = kreuz(x, quer);
}

constexpr int kKanten[12][2] = {{0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

// Ein Quader: Ecken 0-3 am Anfang, 4-7 am Ende (entlang x um l); Bit 0 die Breite (quer), Bit 1 die Tiefe
RigForm kasten(const V3& anfang, const V3& x, const V3& quer, const V3& tief, float l, float b, float t) {
    RigForm f;
    f.gestalt = FormGestalt::Kasten;
    f.innen = true;
    f.masse[0] = l;
    f.masse[1] = b;
    f.masse[2] = t;
    for (int i = 0; i < 8; ++i) {
        V3 p = i < 4 ? anfang : plus(anfang, mal(x, l));
        p = plus(p, mal(quer, (i & 1) != 0 ? b * 0.5F : -b * 0.5F));
        p = plus(p, mal(tief, (i & 2) != 0 ? t * 0.5F : -t * 0.5F));
        f.punkte.push_back(p);
    }
    for (const auto& k : kKanten) { f.linien.push_back({k[0], k[1]}); }
    return f;
}

// Punkte eines Kreises um `mitte` in der Ebene (a, b) an die Form haengen; liefert den Index des ersten Punkts
int kreisAn(RigForm& f, const V3& mitte, const V3& a, const V3& b, float r, int teile = 32) {
    const int erster = static_cast<int>(f.punkte.size());
    for (int i = 0; i < teile; ++i) {
        const float w = 2.0F * kPi * static_cast<float>(i) / static_cast<float>(teile);
        f.punkte.push_back(plus(mitte, plus(mal(a, std::cos(w) * r), mal(b, std::sin(w) * r))));
        f.linien.push_back({erster + i, erster + (i + 1) % teile});
    }
    return erster;
}

void linieAn(RigForm& f, const V3& p, const V3& q) {
    const int i = static_cast<int>(f.punkte.size());
    f.punkte.push_back(p);
    f.punkte.push_back(q);
    f.linien.push_back({i, i + 1});
}

void zugAn(RigForm& f, const std::vector<V3>& p, bool geschlossen) {
    const int erster = static_cast<int>(f.punkte.size());
    const int n = static_cast<int>(p.size());
    for (int i = 0; i < n; ++i) {
        f.punkte.push_back(p[static_cast<std::size_t>(i)]);
        if (i + 1 < n) {
            f.linien.push_back({erster + i, erster + i + 1});
        } else if (geschlossen && n > 2) {
            f.linien.push_back({erster + i, erster});
        }
    }
}

RigForm ring(const V3& mitte, const V3& a, const V3& b, float r) {
    RigForm f;
    f.gestalt = FormGestalt::Ring;
    f.masse[0] = r;
    (void)kreisAn(f, mitte, a, b, r);
    return f;
}

// Ein Rechteck um `mitte`: l entlang a, br entlang b; fase > 0: mit abgeschraegten Ecken (Fussplatte des Control Rigs)
RigForm rechteck(const V3& mitte, const V3& a, const V3& b, float l, float br, float fase = 0.0F) {
    RigForm f;
    f.gestalt = FormGestalt::Rechteck;
    f.masse[0] = l;
    f.masse[1] = br;
    const float ha = l * 0.5F;
    const float hb = br * 0.5F;
    std::vector<V3> p;
    const auto ecke = [&](float u, float v) { return plus(mitte, plus(mal(a, u), mal(b, v))); };
    if (fase <= 0.0F) {
        p = {ecke(-ha, -hb), ecke(ha, -hb), ecke(ha, hb), ecke(-ha, hb)};
    } else {
        const float c = std::min(fase, std::min(ha, hb) * 0.8F);
        p = {ecke(-ha + c, -hb), ecke(ha - c, -hb), ecke(ha, -hb + c), ecke(ha, hb - c), ecke(ha - c, hb), ecke(-ha + c, hb), ecke(-ha, hb - c), ecke(-ha, -hb + c)};
    }
    zugAn(f, p, true);
    return f;
}

RigForm kreuz3(const V3& mitte, float l) {
    RigForm f;
    f.gestalt = FormGestalt::Kreuz;
    f.masse[0] = l;
    const float h = l * 0.5F;
    linieAn(f, plus(mitte, {-h, 0.0F, 0.0F}), plus(mitte, {h, 0.0F, 0.0F}));
    linieAn(f, plus(mitte, {0.0F, -h, 0.0F}), plus(mitte, {0.0F, h, 0.0F}));
    linieAn(f, plus(mitte, {0.0F, 0.0F, -h}), plus(mitte, {0.0F, 0.0F, h}));
    return f;
}

RigForm kugel(const V3& mitte, float r) {
    RigForm f;
    f.gestalt = FormGestalt::Kugel;
    f.masse[0] = r;
    (void)kreisAn(f, mitte, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, r, 20);
    (void)kreisAn(f, mitte, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, r, 20);
    (void)kreisAn(f, mitte, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, r, 20);
    return f;
}

// Eine Raute in der Ebene (a, b)
RigForm marke(const V3& mitte, const V3& a, const V3& b, float r) {
    RigForm f;
    f.gestalt = FormGestalt::Marke;
    f.masse[0] = r;
    zugAn(f, {plus(mitte, mal(a, r)), plus(mitte, mal(b, r)), plus(mitte, mal(a, -r)), plus(mitte, mal(b, -r))}, true);
    return f;
}

void binden(RigForm& f, int knochen, int griff, const Griff* g) {
    f.knochen = knochen;
    f.griff = griff;
    if (g != nullptr) { f.seite = g->seite; }
}

// --- die Weite der Haut ------------------------------------------------------------------------------------------------

enum class Glied : std::uint8_t { Keiner = 0, Mitte, ArmL, ArmR, BeinL, BeinR };

Glied gliedVon(Rolle r, Seite s) {
    switch (r) {
        case Rolle::Becken:
        case Rolle::Wirbel:
        case Rolle::Hals:
        case Rolle::Kopf:
        case Rolle::Gesicht: return Glied::Mitte;
        case Rolle::Oberarm:
        case Rolle::OberarmTwist:
        case Rolle::Unterarm:
        case Rolle::UnterarmTwist:
        case Rolle::Hand:
        case Rolle::Finger: return s == Seite::Rechts ? Glied::ArmR : (s == Seite::Links ? Glied::ArmL : Glied::Keiner);
        case Rolle::Oberschenkel:
        case Rolle::OberschenkelTwist:
        case Rolle::Unterschenkel:
        case Rolle::Fuss:
        case Rolle::Zehen: return s == Seite::Rechts ? Glied::BeinR : (s == Seite::Links ? Glied::BeinL : Glied::Keiner);
        default: return Glied::Keiner;
    }
}

// Der weiteste Abstand der Haut von der Achse durch `mitte` in einer Scheibe (halbe Dicke `dick`): Ecken der Stufe, deren
// staerkster Knochen zum Koerperteil gehoert. 0: keine Ecke (kein Modell, kein Gewicht).
float hautWeite(const Figur& f, const Inhalt* in, int stufe, Glied teil, const V3& mitte, const V3& achse, float dick) {
    if (in == nullptr || teil == Glied::Keiner) { return 0.0F; }
    float weit = 0.0F;
    for (const Teil& t : in->teile) {
        if (t.istTag() || t.stufen.empty()) { continue; }
        const Netz& n = t.stufen[static_cast<std::size_t>(std::clamp(stufe, 0, static_cast<int>(t.stufen.size()) - 1))];
        for (const Ecke& e : n.ecken) {
            int stark = 0;
            for (int k = 1; k < 4; ++k) {
                if (e.gewicht[k] > e.gewicht[stark]) { stark = k; }
            }
            const int b = e.knochen[stark];
            if (e.gewicht[stark] <= 0.0F || b >= f.n || gliedVon(f.rolle(b), f.seiteVon(b)) != teil) { continue; }
            const V3 d = minus({e.ort[0], e.ort[1], e.ort[2]}, mitte);
            const float u = punkt(d, achse);
            if (std::fabs(u) > dick) { continue; }
            weit = std::max(weit, laenge(minus(d, mal(achse, u))));
        }
    }
    return weit;
}

// Der Radius eines Rings: die Annahme mal Massstab, mit Modell die Weite der Haut mal kRingLuft (zwischen der halben und der
// zweieinhalbfachen Annahme - ein Umhang oder ein Gewicht an der falschen Stelle soll den Ring nicht sprengen)
float ringRadius(const Figur& f, const Inhalt* in, int stufe, Glied teil, const V3& mitte, const V3& achse, float annahme) {
    const float soll = annahme * f.m;
    const float haut = hautWeite(f, in, stufe, teil, mitte, achse, std::max(soll * 0.35F, 0.5F));
    if (haut <= 0.0F) { return soll; }
    return std::clamp(haut * kRingLuft, soll * 0.5F, soll * 2.5F);
}

// --- Boden und Fuss ------------------------------------------------------------------------------------------------------

// Der Boden unter der Figur: die Drehpunkte der Fussrolle (aus dem Modell: Ferse, Ballen, Zehe liegen auf der Sohle), sonst die
// Wurzel, wenn sie unter den Fussgelenken liegt (die Skelette der Spiele: model_root auf dem Boden), sonst so weit unter dem
// tiefsten Fussgelenk wie im CAT-Rig (6,21 Einheiten des Rigs).
float bodenHoehe(const Figur& f, const Rig& rig) {
    bool da = false;
    float z = 0.0F;
    for (const Griff& g : rig.griffe) {
        if (g.art != GriffArt::Ferse && g.art != GriffArt::Ballen && g.art != GriffArt::Zehe) { continue; }
        z = da ? std::min(z, g.ort[2]) : g.ort[2];
        da = true;
    }
    if (da) { return z; }
    float fuss = 0.0F;
    bool fussDa = false;
    float tief = 0.0F;
    for (int b = 0; b < f.n; ++b) {
        const float zb = f.ort(b)[2];
        tief = b == 0 ? zb : std::min(tief, zb);
        if (f.rolle(b) == Rolle::Fuss) {
            fuss = fussDa ? std::min(fuss, zb) : zb;
            fussDa = true;
        }
    }
    if (!fussDa) { return tief; }
    const int wurzel = knochenMit(f, Rolle::Wurzel, Seite::Mitte);
    if (wurzel >= 0 && f.ort(wurzel)[2] < fuss - 0.5F) { return f.ort(wurzel)[2]; }
    return fuss - kCatKnoechel * kCatEinheit * f.m;
}

const Griff* griffMit(const Rig& rig, GriffArt a, Seite s, int nummer) {
    for (const Griff& g : rig.griffe) {
        if (g.art == a && g.seite == s && g.nummer == nummer) { return &g; }
    }
    return nullptr;
}

// Mitte und Laengsrichtung der Platte unter einem Fuss: zwischen Ferse und Zehe der Fussrolle, sonst `vor` vor dem Gelenk
void fussPlatte(const Figur& f, const Rig& rig, const Griff& g, float boden, float vor, V3& mitte, V3& laengs) {
    const Griff* ferse = griffMit(rig, GriffArt::Ferse, g.seite, g.nummer);
    const Griff* zehe = griffMit(rig, GriffArt::Zehe, g.seite, g.nummer);
    const V3 gelenk = f.ort(g.knochen);
    laengs = f.vorn;
    mitte = plus(gelenk, mal(f.vorn, vor));
    if (ferse != nullptr && zehe != nullptr) {
        const V3 a{ferse->ort[0], ferse->ort[1], 0.0F};
        const V3 b{zehe->ort[0], zehe->ort[1], 0.0F};
        laengs = norm(minus(b, a), f.vorn);
        mitte = mal(plus(a, b), 0.5F);
    }
    mitte[2] = boden;
    laengs[2] = 0.0F;
    laengs = norm(laengs, f.vorn);
}

Rgb catGlied(GriffArt a, Seite s) {
    const bool bein = a == GriffArt::PolKnie || a == GriffArt::Fuss || a == GriffArt::Ferse || a == GriffArt::Ballen || a == GriffArt::Zehe || a == GriffArt::Zehen ||
                      a == GriffArt::Oberschenkel || a == GriffArt::Unterschenkel;
    if (s == Seite::Rechts) { return bein ? kCatBeinR : kCatArmR; }
    if (s == Seite::Links) { return bein ? kCatBeinL : kCatArmL; }
    return kCatBrust;
}

V3 griffOrt(const Griff& g) { return {g.ort[0], g.ort[1], g.ort[2]}; }

// Der Mittelpunkt, um den Bodenformen liegen: die Wurzel, sonst das Becken (nur x und y)
V3 bodenMitte(const Figur& f, float boden, int& knochen) {
    knochen = knochenMit(f, Rolle::Wurzel, Seite::Mitte);
    const int becken = knochenMit(f, Rolle::Becken, Seite::Mitte);
    V3 m{0.0F, 0.0F, boden};
    const int von = becken >= 0 ? becken : knochen;
    if (von >= 0) {
        m[0] = f.ort(von)[0];
        m[1] = f.ort(von)[1];
    }
    return m;
}

void griffeCat(const Figur& f, const Rig& rig, RigFormen& aus) {
    const float e = kCatEinheit * f.m;
    const V3 waag = norm(kreuz(f.oben, f.vorn), f.seite);
    for (std::size_t i = 0; i < rig.griffe.size(); ++i) {
        const Griff& g = rig.griffe[i];
        if (g.knochen < 0 || g.knochen >= f.n) { continue; }
        const int gi = static_cast<int>(i);
        switch (g.art) {
            case GriffArt::Fuss: {
                V3 mitte;
                V3 laengs;
                fussPlatte(f, rig, g, aus.boden, kCatPlatteVor * e, mitte, laengs);
                RigForm x = rechteck(mitte, laengs, norm(kreuz(f.oben, laengs), waag), kCatPlatteL * e, kCatPlatteB * e);
                x.innen = true;
                binden(x, g.knochen, gi, &g);
                farbe(x, catGlied(g.art, g.seite));
                aus.formen.push_back(std::move(x));
                break;
            }
            case GriffArt::Hand: {
                const int unter = knochenMit(f, Rolle::Unterarm, g.seite);
                const V3 gelenk = f.ort(g.knochen);
                const V3 richtung = unter >= 0 ? norm(minus(gelenk, f.ort(unter)), eigeneRichtung(f, g.knochen)) : eigeneRichtung(f, g.knochen);
                V3 quer;
                V3 tief;
                rahmen(f, richtung, quer, tief);
                RigForm x = rechteck(plus(gelenk, mal(richtung, catMass(Rolle::Hand, g.seite, 0, 0).laenge * e)), richtung, quer, kCatHandZiel * e, kCatHandZiel * e);
                binden(x, g.knochen, gi, &g);
                farbe(x, catGlied(g.art, g.seite));
                aus.formen.push_back(std::move(x));
                break;
            }
            case GriffArt::PolEllbogen:
            case GriffArt::PolKnie: {
                RigForm x = kreuz3(griffOrt(g), kCatKreuz * e);
                x.bindung = FormBindung::Ort;
                std::memcpy(x.anker, g.ort, sizeof x.anker);
                binden(x, g.knochen, gi, &g);
                farbe(x, catGlied(g.art, g.seite));
                aus.formen.push_back(std::move(x));
                break;
            }
            case GriffArt::Ferse:
            case GriffArt::Ballen:
            case GriffArt::Zehe: {
                RigForm x = marke(griffOrt(g), f.vorn, waag, 1.5F * e);
                binden(x, g.knochen, gi, &g);
                farbe(x, catGlied(g.art, g.seite));
                aus.formen.push_back(std::move(x));
                break;
            }
            case GriffArt::Saebelgriff: {
                RigForm x = kreuz3(griffOrt(g), 4.0F * e);
                binden(x, g.knochen, gi, &g);
                farbe(x, catGlied(GriffArt::Hand, g.seite));
                aus.formen.push_back(std::move(x));
                break;
            }
            default: break;   // die FK-Griffe sind die Kaesten des Skeletts
        }
    }
    // die Bodenform (CATParent): ein Dreieck mit der Spitze nach vorn, darin ein Pfeil nach vorn - eigene Zeichnung in den
    // Aussenmassen des Vorbilds
    int wurzel = -1;
    const V3 m = bodenMitte(f, aus.boden, wurzel);
    const float hb = kCatBodenHalb * e;
    const float vo = kCatBodenVorn * e;
    const float hi = kCatBodenHinten * e;
    const auto p = [&](float quer, float vor) { return plus(m, plus(mal(waag, quer), mal(f.vorn, vor))); };
    RigForm b;
    b.gestalt = FormGestalt::Bodenform;
    b.schmuck = true;
    b.knochen = wurzel;
    b.masse[0] = vo + hi;
    b.masse[1] = hb * 2.0F;
    zugAn(b, {p(-hb, -hi), p(hb, -hi), p(hb * 0.16F, vo), p(-hb * 0.16F, vo)}, true);
    // ... als Band wie im Vorbild: derselbe Umriss noch einmal um ein Achtel der halben Breite nach innen
    const float band = hb * 0.125F;
    zugAn(b, {p(-hb + band * 1.6F, -hi + band), p(hb - band * 1.6F, -hi + band), p(hb * 0.16F - band * 0.2F, vo - band), p(-hb * 0.16F + band * 0.2F, vo - band)}, true);
    zugAn(b, {p(-hb * 0.1F, 0.0F), p(hb * 0.1F, 0.0F), p(hb * 0.1F, vo * 0.45F), p(hb * 0.22F, vo * 0.45F), p(0.0F, vo * 0.72F), p(-hb * 0.22F, vo * 0.45F), p(-hb * 0.1F, vo * 0.45F)},
          true);
    farbe(b, kCatBoden);
    aus.formen.push_back(std::move(b));
}

Rgb blFarbe(Seite s) { return s == Seite::Links ? kBlLinks : (s == Seite::Rechts ? kBlRechts : kBlMitte); }

void griffeBlender(const Figur& f, const Rig& rig, RigFormen& aus) {
    const V3 waag = norm(kreuz(f.oben, f.vorn), f.seite);
    // Kreise um die Wirbel: der unterste (spine) und der oberste (upper_spine) - dazwischen ist der Oktaeder der Griff
    int ersterWirbel = -1;
    int letzterWirbel = -1;
    for (std::size_t i = 0; i < rig.griffe.size(); ++i) {
        if (rig.griffe[i].art != GriffArt::Wirbel) { continue; }
        const int nr = wirbelNummer(f, rig.griffe[i].knochen);
        if (nr == 1) { ersterWirbel = static_cast<int>(i); }
        if (nr > 1 && nr == static_cast<int>(f.wirbel.size())) { letzterWirbel = static_cast<int>(i); }
    }
    for (std::size_t i = 0; i < rig.griffe.size(); ++i) {
        const Griff& g = rig.griffe[i];
        if (g.knochen < 0 || g.knochen >= f.n) { continue; }
        const int gi = static_cast<int>(i);
        const V3 gelenk = f.ort(g.knochen);
        RigForm x;
        bool da = true;
        switch (g.art) {
            case GriffArt::Becken: x = ring(plus(gelenk, mal(f.oben, kBlBeckenHoch * f.m)), waag, f.vorn, kBlBecken * f.m); break;
            case GriffArt::Wirbel:
                if (gi == ersterWirbel) {
                    x = ring(gelenk, waag, f.vorn, kBlWirbelUnten * f.m);
                } else if (gi == letzterWirbel) {
                    x = ring(gelenk, waag, f.vorn, kBlWirbelOben * f.m);
                } else {
                    da = false;
                }
                break;
            case GriffArt::Hand:
            case GriffArt::Fuss: {
                // das Quadrat steht quer zum Glied (boneshape3 liegt in der Ebene quer zur X-Achse seines Knochens, und die zeigt
                // entlang des Unterarms bzw. Unterschenkels)
                const int davor = knochenMit(f, g.art == GriffArt::Hand ? Rolle::Unterarm : Rolle::Unterschenkel, g.seite);
                const V3 richtung = davor >= 0 ? norm(minus(gelenk, f.ort(davor)), eigeneRichtung(f, g.knochen)) : eigeneRichtung(f, g.knochen);
                V3 quer;
                V3 tief;
                rahmen(f, richtung, quer, tief);
                const float kante = 2.0F * (g.art == GriffArt::Hand ? kBlHand : kBlFuss) * f.m;
                x = rechteck(gelenk, quer, tief, kante, kante);
                break;
            }
            case GriffArt::PolEllbogen:
            case GriffArt::PolKnie:
                x = ring(griffOrt(g), waag, f.oben, (g.art == GriffArt::PolKnie ? kBlPolKnie : kBlPolEllbogen) * f.m);
                x.bindung = FormBindung::Ort;
                std::memcpy(x.anker, g.ort, sizeof x.anker);
                break;
            case GriffArt::Ferse:
            case GriffArt::Ballen:
            case GriffArt::Zehe: x = marke(griffOrt(g), f.vorn, waag, 1.0F * f.m); break;
            case GriffArt::Saebelgriff: x = kreuz3(griffOrt(g), 2.6F * f.m); break;
            default: da = false; break;   // der Oktaeder des Knochens ist der Griff
        }
        if (!da) { continue; }
        binden(x, g.knochen, gi, &g);
        farbe(x, blFarbe(g.seite));
        aus.formen.push_back(std::move(x));
    }
    int wurzel = -1;
    RigForm b = ring(bodenMitte(f, aus.boden, wurzel), waag, f.vorn, kBlBoden * f.m);
    b.schmuck = true;
    b.knochen = wurzel;
    farbe(b, kBlMitte);
    aus.formen.push_back(std::move(b));
}

Rgb crFarbe(Seite s) { return s == Seite::Links ? kCrLinks : (s == Seite::Rechts ? kCrRechts : kCrMitte); }
Rgb crPol(Seite s) { return s == Seite::Links ? kCrPolLinks : (s == Seite::Rechts ? kCrPolRechts : kCrGelb); }

void griffeControl(const Figur& f, const Rig& rig, const Inhalt* in, int stufe, RigFormen& aus) {
    const V3 waag = norm(kreuz(f.oben, f.vorn), f.seite);
    const float m = f.m;
    for (std::size_t i = 0; i < rig.griffe.size(); ++i) {
        const Griff& g = rig.griffe[i];
        if (g.knochen < 0 || g.knochen >= f.n) { continue; }
        const int gi = static_cast<int>(i);
        const V3 gelenk = f.ort(g.knochen);
        const int ende = segmentEnde(f, g.knochen);
        const V3 laengs = ende >= 0 ? norm(minus(f.ort(ende), gelenk), eigeneRichtung(f, g.knochen)) : eigeneRichtung(f, g.knochen);
        const Glied teil = gliedVon(f.rolle(g.knochen), f.seiteVon(g.knochen));
        const auto fertig = [&](RigForm x, Rgb c) {
            binden(x, g.knochen, gi, &g);
            farbe(x, c);
            aus.formen.push_back(std::move(x));
        };
        // ein Ring quer zum Glied, ein Stueck hinter dem Gelenk (30 % der Strecke, hoechstens der doppelte Radius)
        const auto gliedRing = [&](float annahme) {
            const float strecke = ende >= 0 ? laenge(minus(f.ort(ende), gelenk)) : 0.0F;
            const V3 mitte = plus(gelenk, mal(laengs, std::min(strecke * 0.3F, annahme * m * 2.0F)));
            V3 quer;
            V3 tief;
            rahmen(f, laengs, quer, tief);
            return ring(mitte, quer, tief, ringRadius(f, in, stufe, teil, mitte, laengs, annahme));
        };
        switch (g.art) {
            case GriffArt::Becken: {
                // ein Band um das Becken: Ring, vorn ein Kasten mit Kreuz, links und rechts ein Pfeil nach aussen
                const float r = ringRadius(f, in, stufe, Glied::Mitte, gelenk, f.oben, kControl.becken);
                RigForm x;
                x.gestalt = FormGestalt::Band;
                x.masse[0] = r;
                (void)kreisAn(x, gelenk, waag, f.vorn, r);
                const float k = r * 0.16F;
                const V3 v = plus(gelenk, mal(f.vorn, r));
                const auto q = [&](float a, float b) { return plus(v, plus(mal(waag, a), mal(f.oben, b))); };
                zugAn(x, {q(-k, -k), q(k, -k), q(k, k), q(-k, k)}, true);
                linieAn(x, q(-k, -k), q(k, k));
                linieAn(x, q(-k, k), q(k, -k));
                for (const float vz : {-1.0F, 1.0F}) {
                    const V3 a = plus(gelenk, mal(waag, vz * (r + k * 0.8F)));
                    zugAn(x, {plus(a, mal(f.oben, k)), plus(a, mal(waag, vz * k * 1.8F)), plus(a, mal(f.oben, -k))}, true);
                }
                fertig(std::move(x), kCrGelb);
                break;
            }
            case GriffArt::Wirbel: {
                const bool brust = wirbelNummer(f, g.knochen) == static_cast<int>(f.wirbel.size()) && f.wirbel.size() >= 2U;
                fertig(ring(gelenk, waag, f.vorn, ringRadius(f, in, stufe, Glied::Mitte, gelenk, f.oben, brust ? kControl.brust : kControl.wirbel)), brust ? kCrGelb : kCrMitte);
                break;
            }
            case GriffArt::Hals: fertig(ring(gelenk, waag, f.vorn, ringRadius(f, in, stufe, Glied::Mitte, gelenk, f.oben, kControl.hals)), kCrMitte); break;
            case GriffArt::Kopf: {
                const V3 mitte = plus(gelenk, mal(f.oben, kControl.kopfHoch * m));
                fertig(ring(mitte, waag, f.vorn, ringRadius(f, in, stufe, Glied::Mitte, mitte, f.oben, kControl.kopf)), kCrMitte);
                RigForm p = marke(plus(gelenk, mal(f.oben, kControl.kopfPunkt * m)), waag, f.oben, 0.7F * m);
                p.schmuck = true;
                p.knochen = g.knochen;
                farbe(p, kCrGelb);
                aus.formen.push_back(std::move(p));
                break;
            }
            case GriffArt::Schluessel: {
                // ein Stiel von der Schulter nach oben aussen, am Ende ein Kreis
                const int arm = nachfolger(f, g.knochen, Rolle::Oberarm);
                const V3 schulter = arm >= 0 ? f.ort(arm) : gelenk;
                const V3 richtung = norm(plus(mal(f.oben, 0.85F), mal(f.seiteNach(g.seite), 0.5F)), f.oben);
                const V3 kopf = plus(schulter, mal(richtung, kControl.stiel * m));
                RigForm x;
                x.gestalt = FormGestalt::Lutscher;
                x.masse[0] = kControl.stielKreis * m;
                linieAn(x, schulter, plus(schulter, mal(richtung, (kControl.stiel - kControl.stielKreis) * m)));
                (void)kreisAn(x, kopf, waag, f.oben, kControl.stielKreis * m, 20);
                fertig(std::move(x), crFarbe(g.seite));
                break;
            }
            case GriffArt::Oberarm: fertig(gliedRing(kControl.oberarm), crFarbe(g.seite)); break;
            case GriffArt::Unterarm: fertig(gliedRing(kControl.unterarm), crFarbe(g.seite)); break;
            case GriffArt::Oberschenkel: fertig(gliedRing(kControl.oberschenkel), crFarbe(g.seite)); break;
            case GriffArt::Unterschenkel: fertig(gliedRing(kControl.unterschenkel), crFarbe(g.seite)); break;
            case GriffArt::Hand: {
                const int unter = knochenMit(f, Rolle::Unterarm, g.seite);
                const V3 richtung = unter >= 0 ? norm(minus(gelenk, f.ort(unter)), laengs) : laengs;
                V3 quer;
                V3 tief;
                rahmen(f, richtung, quer, tief);
                fertig(ring(gelenk, quer, tief, ringRadius(f, in, stufe, teil, gelenk, richtung, kControl.hand)), crFarbe(g.seite));
                break;
            }
            case GriffArt::Fuss: {
                V3 mitte;
                V3 vor;
                fussPlatte(f, rig, g, aus.boden, kControl.platteL * m * 0.2F, mitte, vor);
                RigForm x = rechteck(mitte, vor, norm(kreuz(f.oben, vor), waag), kControl.platteL * m, kControl.platteB * m, kControl.platteB * m * 0.3F);
                x.innen = true;
                fertig(std::move(x), crPol(g.seite));
                const int davor = knochenMit(f, Rolle::Unterschenkel, g.seite);
                const V3 richtung = davor >= 0 ? norm(minus(gelenk, f.ort(davor)), f.oben) : f.oben;
                V3 quer;
                V3 tief;
                rahmen(f, richtung, quer, tief);
                fertig(ring(gelenk, quer, tief, ringRadius(f, in, stufe, teil, gelenk, richtung, kControl.knoechel)), crFarbe(g.seite));
                break;
            }
            case GriffArt::Zehen: fertig(ring(gelenk, waag, f.oben, kControl.zehen * m), crFarbe(g.seite)); break;
            case GriffArt::PolEllbogen:
            case GriffArt::PolKnie: {
                RigForm x = kugel(griffOrt(g), kControl.pol * m);
                x.bindung = FormBindung::Ort;
                std::memcpy(x.anker, g.ort, sizeof x.anker);
                fertig(std::move(x), crPol(g.seite));
                break;
            }
            case GriffArt::Ferse:
            case GriffArt::Ballen:
            case GriffArt::Zehe: fertig(marke(griffOrt(g), f.vorn, waag, kControl.marke * m), crFarbe(g.seite)); break;
            case GriffArt::Saebelgriff: fertig(kreuz3(griffOrt(g), 2.6F * m), crFarbe(g.seite)); break;
        }
    }
    int wurzel = -1;
    RigForm b = ring(bodenMitte(f, aus.boden, wurzel), waag, f.vorn, kControl.boden * m);
    b.schmuck = true;
    b.knochen = wurzel;
    farbe(b, kCrBoden);
    aus.formen.push_back(std::move(b));
}

}  // namespace

CatMass catMass(Rolle r, Seite s, int wirbel, int wirbelZahl) noexcept {
    const auto mass = [](float l, float b, float t, Rgb c, bool nabe = false) {
        CatMass m;
        m.laenge = l;
        m.breite = b;
        m.tiefe = t;
        m.farbe[0] = c.r;
        m.farbe[1] = c.g;
        m.farbe[2] = c.b;
        m.nabe = nabe;
        return m;
    };
    const bool rechts = s == Seite::Rechts;
    const Rgb arm = rechts ? kCatArmR : kCatArmL;
    const Rgb bein = rechts ? kCatBeinR : kCatBeinL;
    switch (r) {
        case Rolle::Becken: return mass(14.0F, 19.0F, 10.0F, kCatBecken, true);                    // Hips: length, width, height
        case Rolle::Wirbel:
            if (wirbelZahl >= 2 && wirbel >= wirbelZahl) { return mass(13.9F, 19.35F, 10.0F, kCatBrust, true); }   // Chest
            return wirbel <= 1 ? mass(8.0F, 12.8F, 13.0286F, kCatWirbel1) : mass(8.0F, 13.072F, 14.4762F, kCatWirbel2);   // Spine1, Spine2
        case Rolle::Hals: return mass(4.52F, 5.46F, 5.46F, kCatHals);                              // Neck
        case Rolle::Kopf: return mass(12.0F, 12.0F, 12.0F, kCatKopf, true);                        // Head
        case Rolle::Gesicht: return mass(1.0F, 1.0F, 1.0F, kCatGesicht);                           // ArbBone (Lippen 1 x 1 x 1)
        case Rolle::Schluessel: return mass(9.328F, 8.0F, 10.0F, arm);                             // Collarbone
        case Rolle::Oberarm:
        case Rolle::OberarmTwist: return rechts ? mass(8.5F, 6.08F, 7.44F, arm) : mass(8.5F, 5.544F, 6.16F, arm);          // Arm11, Arm12
        case Rolle::Unterarm:
        case Rolle::UnterarmTwist: return rechts ? mass(6.82996F, 6.0F, 4.98F, arm) : mass(6.82996F, 6.54F, 4.44F, arm);   // Arm21, Arm22
        case Rolle::Hand: return mass(4.0F, 4.0F, 1.5F, arm);                                      // Palm
        case Rolle::Finger: return mass(2.05F, 0.9F, 1.05F, arm);                                  // Index21 (Fingerglieder 1,0 bis 3,0)
        case Rolle::TagKnochen: return mass(1.0F, 1.0F, 1.0F, arm);
        case Rolle::Oberschenkel:
        case Rolle::OberschenkelTwist: return mass(11.5F, 8.0F, 5.0F, bein);                       // Leg11, Leg12
        case Rolle::Unterschenkel: return mass(22.6003F, 8.0F, 4.0F, bein);                        // Leg2 (depth 3,99994)
        case Rolle::Fuss: return mass(8.6F, 8.0F, 3.0F, bein);                                     // Ankle
        case Rolle::Zehen: return mass(3.0F, 8.0F, 2.1F, bein);                                    // Toe1
        case Rolle::Anhang: return mass(17.0F, 2.25F, 2.25F, kCatAnhang);                          // Tail1
        default: return mass(1.0F, 1.0F, 1.0F, kCatSonst);
    }
}

float rigMassstab(const GlaAnimation& g, const Erkennung& e) {
    Figur f;
    f.g = &g;
    f.e = &e;
    f.n = static_cast<int>(std::min(g.bones.size(), e.knochen.size()));
    float summe = 0.0F;
    int beine = 0;
    for (const Seite s : {Seite::Links, Seite::Rechts}) {
        const int ober = knochenMit(f, Rolle::Oberschenkel, s);
        if (ober < 0) { continue; }
        const int unter = nachfolger(f, ober, Rolle::Unterschenkel);
        const int fuss = unter >= 0 ? nachfolger(f, unter, Rolle::Fuss) : -1;
        if (unter < 0 || fuss < 0) { continue; }
        summe += laenge(minus(f.ort(unter), f.ort(ober))) + laenge(minus(f.ort(fuss), f.ort(unter)));
        ++beine;
    }
    if (beine > 0 && summe > 1e-3F) { return summe / static_cast<float>(beine) / kHumanoidBein; }
    return e.hoehe > 1e-3F ? e.hoehe / kHumanoidHoehe : 1.0F;
}

RigFormen rigFormenSkelettCat(const GlaAnimation& g, const Rig& rig) {
    RigFormen aus;
    if (g.bones.empty() || rig.erkennung.knochen.empty()) { return aus; }
    const Figur f = figurAus(g, rig.erkennung);
    aus.massstab = f.m;
    aus.boden = bodenHoehe(f, rig);
    const float e = kCatEinheit * f.m;
    const int wirbelZahl = static_cast<int>(f.wirbel.size());
    for (int b = 0; b < f.n; ++b) {
        const Rolle r = f.rolle(b);
        if (r == Rolle::Keine || r == Rolle::Wurzel || r == Rolle::Bewegung) { continue; }
        const CatMass cm = catMass(r, f.seiteVon(b), wirbelNummer(f, b), wirbelZahl);
        const V3 gelenk = f.ort(b);
        RigForm x;
        if (cm.nabe) {
            // Nabe: Hoehe entlang oben, die Breite quer, CATs "length" nach vorn. Becken mittig um das Gelenk, die Brust mittig
            // zwischen ihrem Gelenk und dem Hals, der Kopf ab dem Gelenk nach oben.
            V3 quer;
            V3 tief;
            rahmen(f, f.oben, quer, tief);
            V3 anfang = plus(gelenk, mal(f.oben, -cm.tiefe * e * 0.5F));
            if (r == Rolle::Kopf) {
                anfang = gelenk;
            } else if (r == Rolle::Wirbel) {
                const int ende = segmentEnde(f, b);
                if (ende >= 0) { anfang = plus(mal(plus(gelenk, f.ort(ende)), 0.5F), mal(f.oben, -cm.tiefe * e * 0.5F)); }
            }
            x = kasten(anfang, f.oben, quer, tief, cm.tiefe * e, cm.breite * e, cm.laenge * e);
        } else {
            const int ende = segmentEnde(f, b);
            V3 richtung = eigeneRichtung(f, b);
            float l = cm.laenge * e;
            if (ende >= 0 && laenge(minus(f.ort(ende), gelenk)) > 1e-3F) {
                l = laenge(minus(f.ort(ende), gelenk));
                richtung = mal(minus(f.ort(ende), gelenk), 1.0F / l);
            }
            V3 quer;
            V3 tief;
            rahmen(f, richtung, quer, tief);
            x = kasten(gelenk, richtung, quer, tief, l, cm.breite * e, cm.tiefe * e);
        }
        x.skelett = true;
        x.knochen = b;
        x.rolle = r;
        x.seite = f.seiteVon(b);
        std::memcpy(x.farbe, cm.farbe, sizeof x.farbe);
        // der FK-Griff des Knochens; ein Drehknochen zeigt den Griff seines Glieds
        x.griff = rig.griffVon(b);
        for (std::size_t i = 0; x.griff < 0 && i < rig.griffe.size(); ++i) {
            if (rig.griffe[i].twist == b && !rig.griffe[i].ik && rig.griffe[i].art != GriffArt::Saebelgriff) { x.griff = static_cast<int>(i); }
        }
        aus.formen.push_back(std::move(x));
    }
    return aus;
}

RigFormen rigFormenGriffe(const GlaAnimation& g, const Rig& rig, const Inhalt* in, int stufe, RigStil stil) {
    RigFormen aus;
    if (stil == RigStil::Standard || g.bones.empty() || rig.erkennung.knochen.empty()) { return aus; }
    const Figur f = figurAus(g, rig.erkennung);
    aus.massstab = f.m;
    aus.boden = bodenHoehe(f, rig);
    if (stil == RigStil::Cat) {
        griffeCat(f, rig, aus);
    } else if (stil == RigStil::Blender) {
        griffeBlender(f, rig, aus);
    } else {
        griffeControl(f, rig, in != nullptr && !in->teile.empty() ? in : nullptr, stufe, aus);
    }
    return aus;
}

}  // namespace bhed::modell
