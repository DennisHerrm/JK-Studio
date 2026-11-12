// gui/app_modelltab_knochen.cpp - AP13K (rc612): Knochen-Darstellung (Abgleich R14), Rig-Stile, Durchsicht - siehe app_modelltab_knochen.h
//
// Aufbau:
//   Szene       was dieses Bild an Formen zeigt, in Weltkoordinaten: je Form (ein Knochen, ein Griff, ein Schmuckstueck) ihre
//               Linien, gefuellten Dreiecke, Scheiben zum Schirm und - fuer das Treffen - die Dreiecke ihres Koerpers. Die Knochen der
//               Darstellungsarten entstehen wie bis rc611 je Bild vom Gelenk des Elternknochens zum Gelenk des Knochens; die Formen
//               der Stile kommen aus bhed/modell/rigformen.h (Grundstellung) und gehen mit X(knochen) * B(knochen)^-1 in die Pose.
//   Zeichner    die Szene je Ansicht als Dreiecke im Clip-Raum (gpu::ModellAnsichtArt::formen): eine Linie ist ein Streifen in
//               Bildpunkten mit einem Bildpunkt weichem Rand, an der nahen Ebene gekappt. Dieselben Ecken fuer Direct3D 12 und 11.
//   Treffen     die sichtbare Form unter der Maus: zuerst die naechste Linie (Toleranz), sonst der Koerper, in dem die Maus liegt;
//               verdeckt die Figur die Formen (nicht im Vordergrund, keine Durchsicht), zaehlt nur, was vor der Haut liegt.
#include "app_internal.h"
#include "app_modelltab_knochen.h"

#include "gpumap.h"
#include "bhed/modell/ik.h"
#include "bhed/modell/rig.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }

using V3 = std::array<float, 3>;

V3 plus(const V3& a, const V3& b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
V3 minus(const V3& a, const V3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
V3 mal(const V3& a, float f) { return {a[0] * f, a[1] * f, a[2] * f}; }
float punkt(const V3& a, const V3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
V3 kreuz(const V3& a, const V3& b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
float laenge(const V3& a) { return std::sqrt(punkt(a, a)); }
V3 ortVon(const BoneMatrix& m) { return {m.m[0][3], m.m[1][3], m.m[2][3]}; }

// --- Farben -----------------------------------------------------------------------------------------------------------------
//
// Knochen nach dem Entwurf (entwurf_modelltab.html, knochenSvg): wie 3ds Max der Bone-Kasten gruen (Rand #b9e08a, Flaeche #5d7a3a zu
// 60 %), wie Blender blau (Rand #9cc3ff, Flaeche #3d5f8f zu 55 %); Knochen, die der Spielcode sucht, mit Rand #e8b04a. Gewaehlt:
// wie 3ds Max gelb (bis rc611), wie Blender die Farben der Pose aus Blenders Thema (bone_pose 80/200/255, bone_pose_active
// 140/255/255, ausgelesen).
constexpr ImU32 kMaxRand = IM_COL32(185, 224, 138, 235);
constexpr ImU32 kMaxFlaeche = IM_COL32(93, 122, 58, 153);
constexpr ImU32 kMaxWahlRand = IM_COL32(255, 230, 60, 255);
constexpr ImU32 kMaxWahlFlaeche = IM_COL32(255, 230, 60, 110);
constexpr ImU32 kBlRand = IM_COL32(156, 195, 255, 235);
constexpr ImU32 kBlFlaeche = IM_COL32(61, 95, 143, 140);
constexpr ImU32 kBlWahlRand = IM_COL32(140, 255, 255, 255);
constexpr ImU32 kBlWahlFlaeche = IM_COL32(80, 200, 255, 140);
constexpr ImU32 kSpielRand = IM_COL32(232, 176, 74, 240);
constexpr ImU32 kSpielPunkt = IM_COL32(255, 150, 40, 230);
constexpr ImU32 kWahlWeiss = IM_COL32(255, 255, 255, 255);

// --- die Szene --------------------------------------------------------------------------------------------------------------

struct Linie {
    V3 a;
    V3 b;
    ImU32 farbe = 0;
    float dicke = 1.0F;        // Bildpunkte (mal Massstab der Oberflaeche)
    bool gestrichelt = false;
};
struct Flaeche {
    V3 p[3];
    ImU32 farbe = 0;
};
struct Scheibe {
    V3 p;
    float radius = 3.0F;       // Bildpunkte (mal Massstab der Oberflaeche)
    ImU32 farbe = 0;
    bool gefuellt = true;
};
struct Beschriftung {
    V3 p;
    ImU32 farbe = 0;
    const char* text = "";
};
struct Form {
    KdFormInfo info;
    bool skelett = false;      // ein Knochen des Skeletts (fuer "Knochen waehlen")
    int linienVon = 0;
    int linienBis = 0;
    int flaechenVon = 0;
    int flaechenBis = 0;
    int scheibenVon = 0;
    int scheibenBis = 0;
    int koerperVon = 0;        // Dreiecke des Koerpers fuer das Treffen (nicht gezeichnet)
    int koerperBis = 0;
};
struct Szene {
    std::vector<Form> formen;
    std::vector<Linie> linien;
    std::vector<Flaeche> flaechen;
    std::vector<Scheibe> scheiben;
    std::vector<Flaeche> koerper;
    std::vector<Beschriftung> texte;
    float massstab = 1.0F;
    int knochen = 0;           // gezeichnete Knochen (Zaehler modellKnochenLinien: zwoelf je Knochen wie bis rc611)
    int griffe = 0;
    std::uint64_t summe = 0;   // Pruefsumme des Inhalts (fuer das Neuzeichnen)
};

struct Zustand {
    Szene szene;
    std::uint64_t schluessel = 0;
    bool gebaut = false;
    // die Formen der Stile in der Grundstellung (einmal je Skelett, Rig und Stil)
    modell::RigFormen stilSkelett;
    modell::RigFormen stilGriffe;
    std::string stilFuer;
    modell::Rig nurErkennung;      // ohne Control Rig: die Erkennung fuer die Kaesten des Skeletts
    std::string erkennungFuer;
};
Zustand g_kd;

std::uint64_t fnv(std::uint64_t h, const void* daten, std::size_t n) {
    const auto* p = static_cast<const unsigned char*>(daten);
    for (std::size_t i = 0; i < n; ++i) { h = (h ^ p[i]) * 1099511628211ULL; }
    return h;
}
template <typename T> std::uint64_t fnvWert(std::uint64_t h, const T& w) { return fnv(h, &w, sizeof w); }

// Masse in Bildpunkten wachsen mit der Oberflaeche: 1 bei 16 Punkten Zeichenhoehe
float oberflaeche() { return std::max(1.0F, ImGui::GetFontSize() / 16.0F); }

int bildJetzt() { return std::clamp(g_app->animFrame, 0, std::max(g_app->anim.bildZahl() - 1, 0)); }

bool rigGezeigt() { return mt().bereich == ModellBereich::Animation && modellRigDa() && mt().rigZeigen; }

// Die Spur des Glieds, zu dem der Griff gehoert (nullptr: keine)
const modell::IkSpur* ikSpurVon(const modell::Griff& g) {
    using modell::GriffArt;
    const bool arm = g.art == GriffArt::Oberarm || g.art == GriffArt::Unterarm || g.art == GriffArt::Hand || g.art == GriffArt::PolEllbogen;
    const bool bein = g.art == GriffArt::Oberschenkel || g.art == GriffArt::Unterschenkel || g.art == GriffArt::Fuss || g.art == GriffArt::Zehen ||
                      g.art == GriffArt::PolKnie || g.art == GriffArt::Ferse || g.art == GriffArt::Ballen || g.art == GriffArt::Zehe;
    if ((!arm && !bein) || g.seite == modell::Seite::Mitte || g.nummer > 1) { return nullptr; }   // nummer zaehlt ab 1: das erste Glied je Seite
    const modell::IkArt art = arm ? (g.seite == modell::Seite::Links ? modell::IkArt::ArmLinks : modell::IkArt::ArmRechts)
                                  : (g.seite == modell::Seite::Links ? modell::IkArt::BeinLinks : modell::IkArt::BeinRechts);
    const modell::RigSchicht* s = modellRigSchichtJetzt();
    const int sp = s != nullptr ? modell::ikSpurIndex(*s, art) : -1;
    return sp >= 0 ? &s->ik[static_cast<std::size_t>(sp)] : nullptr;
}

// Wo das IK-Ziel von Hand oder Fuss steht (die Weltlage des Endknochens, die die IK anstrebt): dort liegen Fussplatte und
// Hand-IK-Ziel wie bei CAT - auch wenn das Glied es nicht erreicht oder die Fussrolle den Fuss kippt. false: das Glied laeuft unter FK.
bool ikZielLage(const modell::Griff& g, BoneMatrix& lage) {
    if (g.art != modell::GriffArt::Hand && g.art != modell::GriffArt::Fuss) { return false; }
    const modell::IkSpur* sp = ikSpurVon(g);
    if (sp == nullptr || g_app->modellWelt.size() != g_app->anim.bones.size()) { return false; }
    const modell::IkZiel z = modell::ikZielAm(*sp, static_cast<float>(bildJetzt()), g_app->modellWelt);
    if (!z.da || z.ik <= 0.0F) { return false; }
    lage = z.ziel;
    return true;
}

// Der IK-Anteil des Glieds, zu dem der Griff gehoert, am laufenden Bild (-1: der Griff gehoert zu keinem der vier Glieder)
float ikAnteilVon(const modell::Griff& g) {
    using modell::GriffArt;
    const bool glied = g.art == GriffArt::Oberarm || g.art == GriffArt::Unterarm || g.art == GriffArt::Hand || g.art == GriffArt::PolEllbogen ||
                       g.art == GriffArt::Oberschenkel || g.art == GriffArt::Unterschenkel || g.art == GriffArt::Fuss || g.art == GriffArt::Zehen ||
                       g.art == GriffArt::PolKnie || g.art == GriffArt::Ferse || g.art == GriffArt::Ballen || g.art == GriffArt::Zehe;
    if (!glied || g.seite == modell::Seite::Mitte || g.nummer > 1) { return -1.0F; }
    const modell::IkSpur* sp = ikSpurVon(g);
    return sp != nullptr ? modell::ikAnteilAm(*sp, static_cast<float>(bildJetzt())) : 0.0F;
}

ImU32 mitAlpha(ImU32 c, float a) {
    const auto alt = static_cast<float>((c >> IM_COL32_A_SHIFT) & 0xFFU);
    const auto neu = static_cast<ImU32>(std::clamp(alt * a, 0.0F, 255.0F));
    return (c & ~IM_COL32_A_MASK) | (neu << IM_COL32_A_SHIFT);
}

// Baut eine Form in die Szene: zwischen beginn() und ende() angehaengte Linien, Flaechen, Scheiben gehoeren ihr.
struct Bauer {
    Szene& s;
    Form f;
    explicit Bauer(Szene& szene) : s(szene) {}
    void beginn() {
        f = Form{};
        f.linienVon = static_cast<int>(s.linien.size());
        f.flaechenVon = static_cast<int>(s.flaechen.size());
        f.scheibenVon = static_cast<int>(s.scheiben.size());
        f.koerperVon = static_cast<int>(s.koerper.size());
    }
    void linie(const V3& a, const V3& b, ImU32 farbe, float dicke, bool gestrichelt = false) { s.linien.push_back(Linie{a, b, farbe, dicke, gestrichelt}); }
    void flaeche(const V3& a, const V3& b, const V3& c, ImU32 farbe) { s.flaechen.push_back(Flaeche{{a, b, c}, farbe}); }
    void koerper(const V3& a, const V3& b, const V3& c) { s.koerper.push_back(Flaeche{{a, b, c}, 0}); }
    void scheibe(const V3& p, float radius, ImU32 farbe, bool gefuellt) { s.scheiben.push_back(Scheibe{p, radius, farbe, gefuellt}); }
    void ende() {
        f.linienBis = static_cast<int>(s.linien.size());
        f.flaechenBis = static_cast<int>(s.flaechen.size());
        f.scheibenBis = static_cast<int>(s.scheiben.size());
        f.koerperBis = static_cast<int>(s.koerper.size());
        f.info.linien = f.linienBis - f.linienVon;
        f.info.flaechen = f.flaechenBis - f.flaechenVon;
        f.info.punkte = f.scheibenBis - f.scheibenVon;
        s.formen.push_back(f);
    }
};

void infoFarbe(KdFormInfo& i, ImU32 c) {
    i.farbe[0] = static_cast<float>((c >> IM_COL32_R_SHIFT) & 0xFFU) / 255.0F;
    i.farbe[1] = static_cast<float>((c >> IM_COL32_G_SHIFT) & 0xFFU) / 255.0F;
    i.farbe[2] = static_cast<float>((c >> IM_COL32_B_SHIFT) & 0xFFU) / 255.0F;
}

constexpr int kKanten[12][2] = {{0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
// die sechs Seiten eines Kastens aus acht Ecken (0-3 vorn, 4-7 hinten; Bit 0 und Bit 1 die Quermasse)
constexpr int kSeiten[6][4] = {{0, 1, 3, 2}, {4, 6, 7, 5}, {0, 4, 5, 1}, {2, 3, 7, 6}, {0, 2, 6, 4}, {1, 5, 7, 3}};

void kastenFlaechen(Bauer& b, const V3 e[8], ImU32 farbe, bool zeichnen) {
    for (const auto& q : kSeiten) {
        if (zeichnen) {
            b.flaeche(e[q[0]], e[q[1]], e[q[2]], farbe);
            b.flaeche(e[q[0]], e[q[2]], e[q[3]], farbe);
        }
        b.koerper(e[q[0]], e[q[1]], e[q[2]]);
        b.koerper(e[q[0]], e[q[2]], e[q[3]]);
    }
}

// Ein Kreis im Raum als Linienzug
void kreis(Bauer& b, const V3& mitte, const V3& u, const V3& v, float r, ImU32 farbe, float dicke, int teile = 16) {
    constexpr float kPi2 = 6.2831853F;
    V3 vor = plus(mitte, mal(u, r));
    for (int i = 1; i <= teile; ++i) {
        const float w = kPi2 * static_cast<float>(i) / static_cast<float>(teile);
        const V3 p = plus(mitte, plus(mal(u, std::cos(w) * r), mal(v, std::sin(w) * r)));
        b.linie(vor, p, farbe, dicke);
        vor = p;
    }
}

// --- die Knochen der Darstellungsarten (vom Gelenk des Elternknochens zum Gelenk des Knochens, wie bis rc611) --------------------

// Der FK-Griff, den der Abschnitt vom Elternknochen `p` aus zeigt (Stil Blender-Oktaeder: der Knochen ist sein eigener Griff):
// der Griff von p, bei einem Drehknochen der seines Glieds.
int griffVonKnochen(int p) {
    const modell::Rig& rig = mt().rig;
    int g = rig.griffVon(p);
    for (std::size_t i = 0; g < 0 && i < rig.griffe.size(); ++i) {
        if (rig.griffe[i].twist == p && !rig.griffe[i].ik && rig.griffe[i].art != modell::GriffArt::Saebelgriff) { g = static_cast<int>(i); }
    }
    return g;
}

void knochenBauen(Szene& s, bool wieBlender, KnochenArt art, bool alsGriffe) {
    const ModellTab& t = mt();
    const std::vector<BoneMatrix>& w = g_app->modellWelt;
    const GlaAnimation& an = g_app->anim;
    const std::size_t n = std::min(w.size(), an.bones.size());
    const bool flossen = g_app->settings.modellFlossen;
    Bauer b(s);
    for (std::size_t i = 0; i < n; ++i) {
        const int nr = static_cast<int>(i);
        if (modellKnochenVersteckt(nr)) { continue; }   // mb1a2: Auge im Explorer
        const int p = an.bones[i].parent;
        const V3 e = ortVon(w[i]);
        const bool spiel = modellSpielKnochen(an.bones[i].name);
        if (p < 0 || static_cast<std::size_t>(p) >= n) {
            if (spiel) { s.scheiben.push_back(Scheibe{e, 3.0F, kSpielPunkt, true}); }
            continue;
        }
        const V3 o = ortVon(w[static_cast<std::size_t>(p)]);
        V3 x = minus(e, o);
        const float len = laenge(x);
        if (len < 0.05F) {
            if (spiel) { s.scheiben.push_back(Scheibe{e, 3.0F, kSpielPunkt, true}); }
            continue;
        }
        x = mal(x, 1.0F / len);
        // Quer: die z-Achse des Elternknochens, senkrecht zur Laenge gemacht.
        const BoneMatrix& pm = w[static_cast<std::size_t>(p)];
        V3 up{pm.m[0][2], pm.m[1][2], pm.m[2][2]};
        V3 y = minus(up, mal(x, punkt(up, x)));
        if (laenge(y) < 1e-3F) {
            up = {0.0F, 0.0F, 1.0F};
            y = minus(up, mal(x, punkt(up, x)));
            if (laenge(y) < 1e-3F) { continue; }
        }
        y = mal(y, 1.0F / laenge(y));
        const V3 z = kreuz(x, y);
        const int griff = alsGriffe ? griffVonKnochen(p) : -1;
        const bool gewaehlt = t.knochenWahl == nr || (griff >= 0 && t.griffWahl == griff);
        const float dick = gewaehlt ? 2.0F : 1.2F;
        b.beginn();
        b.f.skelett = true;
        b.f.info.knochen = nr;
        b.f.info.griff = griff;
        b.f.info.gewaehlt = gewaehlt;
        b.f.info.masse[0] = len;
        if (!wieBlender) {
            // wie 3ds Max: Bone-Objekt - ein Kasten, zum Ende hin auf 10 % verjuengt (Taper 90 %), Flossen schaltbar
            const ImU32 rand = gewaehlt ? kMaxWahlRand : (spiel ? kSpielRand : kMaxRand);
            const ImU32 voll = gewaehlt ? kMaxWahlFlaeche : kMaxFlaeche;
            const float w0 = std::clamp(len * 0.12F, 0.3F, 2.5F) * 0.5F;
            const float w1 = w0 * 0.1F;
            V3 ecken[8];
            for (int k = 0; k < 8; ++k) {
                const float r = k < 4 ? w0 : w1;
                ecken[k] = plus(k < 4 ? o : e, plus(mal(y, (k & 1) != 0 ? r : -r), mal(z, (k & 2) != 0 ? r : -r)));
            }
            for (const auto& k : kKanten) { b.linie(ecken[k[0]], ecken[k[1]], rand, dick); }
            kastenFlaechen(b, ecken, voll, true);
            if (flossen) {
                // Seiten-, Vorder- und Hinterflosse: ein Dreieck je Seite, 10 % bis 90 % der Laenge
                const V3 richtungen[4] = {y, mal(y, -1.0F), z, mal(z, -1.0F)};
                for (const V3& r : richtungen) {
                    const V3 a0 = plus(o, mal(x, len * 0.1F));
                    const V3 a1 = plus(o, mal(x, len * 0.9F));
                    const V3 spitze = plus(plus(o, mal(x, len * 0.3F)), mal(r, w0 * 2.5F));
                    b.linie(a0, spitze, rand, 1.0F);
                    b.linie(spitze, a1, rand, 1.0F);
                }
            }
            b.f.info.gestalt = static_cast<int>(KdGestalt::BoneKasten);
            infoFarbe(b.f.info, spiel ? kSpielRand : kMaxRand);
        } else {
            const ImU32 rand = gewaehlt ? kBlWahlRand : (spiel ? kSpielRand : kBlRand);
            const ImU32 voll = gewaehlt ? kBlWahlFlaeche : kBlFlaeche;
            infoFarbe(b.f.info, spiel ? kSpielRand : kBlRand);
            switch (art) {
                case KnochenArt::Oktaeder: {
                    // Blender OCTAHEDRAL: der Kopf am Gelenk des Elternteils, der Ring der vier Ecken bei 10 % der Laenge mit 10 %
                    // Abstand von der Achse, die Spitze am Gelenk des Knochens; das Gelenk als Kreis
                    static const float kRing[4][2] = {{0.1F, 0.1F}, {0.1F, -0.1F}, {-0.1F, -0.1F}, {-0.1F, 0.1F}};
                    V3 ring[4];
                    for (int r = 0; r < 4; ++r) { ring[r] = plus(plus(o, mal(x, len * 0.1F)), plus(mal(y, len * kRing[r][0]), mal(z, len * kRing[r][1]))); }
                    for (int r = 0; r < 4; ++r) {
                        b.linie(o, ring[r], rand, dick);
                        b.linie(ring[r], ring[(r + 1) % 4], rand, dick);
                        b.linie(ring[r], e, rand, dick);
                        b.flaeche(o, ring[r], ring[(r + 1) % 4], voll);
                        b.flaeche(ring[r], e, ring[(r + 1) % 4], voll);
                        b.koerper(o, ring[r], ring[(r + 1) % 4]);
                        b.koerper(ring[r], e, ring[(r + 1) % 4]);
                    }
                    b.scheibe(o, 2.5F, rand, false);
                    b.f.info.gestalt = static_cast<int>(KdGestalt::Oktaeder);
                    break;
                }
                case KnochenArt::Stick:
                    // Blender STICK ("simple 2D lines with dots"): eine Linie fester Breite auf dem Schirm, an beiden Gelenken ein Punkt
                    b.linie(o, e, rand, gewaehlt ? 5.0F : 4.0F);
                    b.scheibe(o, 3.5F, rand, true);
                    b.scheibe(e, 3.5F, rand, true);
                    b.f.info.gestalt = static_cast<int>(KdGestalt::Stick);
                    break;
                case KnochenArt::BBone: {
                    // Blender BBONE ("boxes"): ein Kasten gleicher Breite, halbe Breite 10 % der Laenge wie der Ring des Oktaeders
                    const float hb = std::clamp(len * 0.1F, 0.15F, 1.6F);
                    V3 ecken[8];
                    for (int k = 0; k < 8; ++k) { ecken[k] = plus(k < 4 ? o : e, plus(mal(y, (k & 1) != 0 ? hb : -hb), mal(z, (k & 2) != 0 ? hb : -hb))); }
                    for (const auto& k : kKanten) { b.linie(ecken[k[0]], ecken[k[1]], rand, dick); }
                    kastenFlaechen(b, ecken, voll, true);
                    b.f.info.gestalt = static_cast<int>(KdGestalt::BBone);
                    break;
                }
                case KnochenArt::Huelle: {
                    // Blender ENVELOPE ("extruded spheres"): an beiden Gelenken eine Kugel (drei Kreise), dazwischen vier Linien; die
                    // Radien wie bei einem neuen Knochen in Blender (Kopf 10 %, Ende 5 % der Laenge)
                    const float rk = len * 0.1F;
                    const float re = len * 0.05F;
                    kreis(b, o, y, z, rk, rand, dick);
                    kreis(b, o, x, y, rk, rand, dick);
                    kreis(b, o, x, z, rk, rand, dick);
                    kreis(b, e, y, z, re, rand, dick);
                    kreis(b, e, x, y, re, rand, dick);
                    kreis(b, e, x, z, re, rand, dick);
                    const V3 quer[4] = {y, mal(y, -1.0F), z, mal(z, -1.0F)};
                    V3 ecken[8];
                    for (int k = 0; k < 4; ++k) {
                        b.linie(plus(o, mal(quer[k], rk)), plus(e, mal(quer[k], re)), rand, dick);
                        ecken[k] = plus(o, plus(mal(y, (k & 1) != 0 ? rk : -rk), mal(z, (k & 2) != 0 ? rk : -rk)));
                        ecken[k + 4] = plus(e, plus(mal(y, (k & 1) != 0 ? re : -re), mal(z, (k & 2) != 0 ? re : -re)));
                    }
                    kastenFlaechen(b, ecken, voll, false);
                    b.f.info.gestalt = static_cast<int>(KdGestalt::Huelle);
                    break;
                }
                case KnochenArt::Draht:
                    // Blender WIRE ("thin wires")
                    b.linie(o, e, rand, gewaehlt ? 2.0F : 1.0F);
                    b.f.info.gestalt = static_cast<int>(KdGestalt::Draht);
                    break;
            }
        }
        if (spiel) { b.scheibe(e, 3.0F, kSpielPunkt, true); }
        b.ende();
        if (spiel) { --s.formen.back().info.punkte; }   // die Marke des Spielcodes zaehlt nicht zu den Punkten der Form (Stick: zwei, Draht: keiner)
        ++s.knochen;
    }
}

// --- die Formen der Stile ----------------------------------------------------------------------------------------------------

// Das Rig fuer die Kaesten des Skeletts: das Control Rig, sonst nur die Erkennung dieses Skeletts
const modell::Rig& rigFuerSkelett() {
    if (modellRigDa()) { return mt().rig; }
    const GlaAnimation& an = g_app->anim;
    const std::string fuer = an.skeletonName + "|" + std::to_string(an.bones.size()) + "|" + g_app->modelPath;
    if (g_kd.erkennungFuer != fuer) {
        g_kd.nurErkennung = modell::Rig{};
        g_kd.nurErkennung.erkennung = modell::erkenneKnochen(an, {}, &mt().dok.inhalt(), std::max(mt().lodJetzt, 0));
        g_kd.erkennungFuer = fuer;
    }
    return g_kd.nurErkennung;
}

void stilFormenHolen(modell::RigStil stil) {
    const GlaAnimation& an = g_app->anim;
    const bool rig = modellRigDa();
    const std::string fuer = std::to_string(static_cast<int>(stil)) + "|" + an.skeletonName + "|" + std::to_string(an.bones.size()) + "|" + g_app->modelPath + "|" +
                             (rig ? mt().rigFuer : std::string("-")) + "|" + std::to_string(mt().rig.griffe.size()) + "|" + std::to_string(mt().lodJetzt);
    if (g_kd.stilFuer == fuer) { return; }
    g_kd.stilFuer = fuer;
    g_kd.stilSkelett = stil == modell::RigStil::Cat ? modell::rigFormenSkelettCat(an, rigFuerSkelett()) : modell::RigFormen{};
    g_kd.stilGriffe = rig ? modell::rigFormenGriffe(an, mt().rig, &mt().dok.inhalt(), std::max(mt().lodJetzt, 0), stil) : modell::RigFormen{};
}

// Ein Punkt der Grundstellung in der Pose: X(knochen) * B(knochen)^-1 * p
V3 inPose(int knochen, const V3& p) {
    const std::vector<BoneMatrix>& w = g_app->modellWelt;
    const GlaAnimation& an = g_app->anim;
    if (knochen < 0 || static_cast<std::size_t>(knochen) >= w.size() || static_cast<std::size_t>(knochen) >= an.bones.size()) { return p; }
    float lokal[3];
    float welt[3];
    an.bones[static_cast<std::size_t>(knochen)].basePoseInv.transform(p.data(), lokal);
    w[static_cast<std::size_t>(knochen)].transform(lokal, welt);
    return {welt[0], welt[1], welt[2]};
}

void stilFormBauen(Szene& s, const modell::RigForm& q, bool alsGriff) {
    const ModellTab& t = mt();
    Bauer b(s);
    V3 versatz{0.0F, 0.0F, 0.0F};
    const bool frei = q.bindung == modell::FormBindung::Ort;
    if (frei) {
        float ort[3];
        if (q.griff < 0 || !modellRigGriffWelt(q.griff, g_app->modellWelt, ort)) { return; }
        versatz = {ort[0] - q.anker[0], ort[1] - q.anker[1], ort[2] - q.anker[2]};
    }
    const int griff = alsGriff ? q.griff : -1;
    const bool gewaehlt = (griff >= 0 && t.griffWahl == griff) || (q.skelett && t.knochenWahl == q.knochen && q.knochen >= 0);
    float blass = 1.0F;
    if (griff >= 0 && static_cast<std::size_t>(griff) < t.rig.griffe.size() && !gewaehlt) {
        // wie die Vorbilder zeigen: was am Glied gerade nicht wirkt, tritt zurueck - die Ziele der IK an einem Glied unter FK,
        // die FK-Ringe des Control Rigs an einem Glied unter IK
        const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(griff)];
        const float ik = ikAnteilVon(g);
        const bool istEnde = g.art == modell::GriffArt::Hand || g.art == modell::GriffArt::Fuss;
        if (ik >= 0.0F && g.ik && ik < 0.5F) { blass = 0.4F; }
        if (ik >= 0.5F && !g.ik && !istEnde && !q.skelett && modellRigStil() == modell::RigStil::ControlRig) { blass = 0.4F; }
    }
    const ImU32 grund = IM_COL32(q.farbe[0], q.farbe[1], q.farbe[2], 255);
    const ImU32 farbe = gewaehlt ? kWahlWeiss : mitAlpha(grund, blass);
    const float dick = gewaehlt ? 2.6F : (q.gestalt == modell::FormGestalt::Bodenform ? 2.2F : (q.schmuck ? 1.2F : 1.6F));
    // Fussplatte und Hand-IK-Ziel (Rechtecke an Hand und Fuss) liegen am Ziel der IK, solange das Glied unter IK laeuft
    BoneMatrix ziel;
    const bool amZiel = !frei && griff >= 0 && static_cast<std::size_t>(griff) < t.rig.griffe.size() && q.gestalt == modell::FormGestalt::Rechteck &&
                        q.knochen >= 0 && static_cast<std::size_t>(q.knochen) < g_app->anim.bones.size() && ikZielLage(t.rig.griffe[static_cast<std::size_t>(griff)], ziel);
    std::vector<V3> p;
    p.reserve(q.punkte.size());
    for (const auto& pt : q.punkte) {
        if (amZiel) {
            float lokal[3];
            float welt[3];
            g_app->anim.bones[static_cast<std::size_t>(q.knochen)].basePoseInv.transform(pt.data(), lokal);
            ziel.transform(lokal, welt);
            p.push_back({welt[0], welt[1], welt[2]});
        } else {
            p.push_back(frei ? plus(pt, versatz) : inPose(q.knochen, pt));
        }
    }
    b.beginn();
    b.f.skelett = q.skelett;
    b.f.info.knochen = q.skelett ? q.knochen : -1;
    b.f.info.griff = griff;
    b.f.info.schmuck = q.schmuck;
    b.f.info.gestalt = static_cast<int>(KdGestalt::Stil) + static_cast<int>(q.gestalt);
    b.f.info.gewaehlt = gewaehlt;
    std::memcpy(b.f.info.masse, q.masse, sizeof b.f.info.masse);
    infoFarbe(b.f.info, grund);
    for (const auto& l : q.linien) {
        if (l[0] < 0 || l[1] < 0 || static_cast<std::size_t>(l[0]) >= p.size() || static_cast<std::size_t>(l[1]) >= p.size()) { continue; }
        b.linie(p[static_cast<std::size_t>(l[0])], p[static_cast<std::size_t>(l[1])], farbe, dick, q.gestrichelt);
    }
    if (q.innen && q.gestalt == modell::FormGestalt::Kasten && p.size() == 8U) {
        kastenFlaechen(b, p.data(), 0, false);
    } else if (q.innen && p.size() >= 3U) {   // ein ebenes Vieleck (Fussplatte): Faecher um die erste Ecke
        for (std::size_t i = 1; i + 1 < p.size(); ++i) { b.koerper(p[0], p[i], p[i + 1]); }
    }
    b.ende();
    // der Pol mit seinem Gelenk verbunden: wie Blender gestrichelt in der Farbe des Pols, im Control Rig eine duenne graue Linie
    if (frei && griff >= 0 && static_cast<std::size_t>(griff) < t.rig.griffe.size()) {
        const modell::Griff& g = t.rig.griffe[static_cast<std::size_t>(griff)];
        const modell::RigStil stil = modellRigStil();
        if (stil != modell::RigStil::Cat && g.knochen >= 0 && static_cast<std::size_t>(g.knochen) < g_app->modellWelt.size()) {
            const V3 pol = plus({q.anker[0], q.anker[1], q.anker[2]}, versatz);
            const V3 gelenk = ortVon(g_app->modellWelt[static_cast<std::size_t>(g.knochen)]);
            s.linien.push_back(Linie{pol, gelenk, stil == modell::RigStil::Blender ? mitAlpha(farbe, 0.8F) : mitAlpha(IM_COL32(200, 200, 200, 255), 0.7F * blass), 1.0F,
                                     stil == modell::RigStil::Blender});
        }
    }
}

// IK oder FK an jedem der vier Glieder (Stil Control Rig, wie in shanks Bild): neben Hand und Fuss, in der Farbe der Seite
void beschriftungen(Szene& s) {
    const ModellTab& t = mt();
    const float m = g_kd.stilGriffe.massstab;
    for (const modell::Griff& g : t.rig.griffe) {
        if ((g.art != modell::GriffArt::Hand && g.art != modell::GriffArt::Fuss) || g.seite == modell::Seite::Mitte || g.nummer > 1) { continue; }
        if (g.knochen < 0 || static_cast<std::size_t>(g.knochen) >= g_app->modellWelt.size()) { continue; }
        const float ik = ikAnteilVon(g);
        if (ik < 0.0F) { continue; }
        const modell::Erkennung& e = t.rig.erkennung;
        V3 aussen{0.0F, 0.0F, 0.0F};
        aussen[static_cast<std::size_t>(e.seitenAchse == 1 ? 1 : 0)] = (g.seite == modell::Seite::Links ? 1.0F : -1.0F) * (e.linksVorzeichen < 0.0F ? -1.0F : 1.0F);
        Beschriftung x;
        x.p = plus(ortVon(g_app->modellWelt[static_cast<std::size_t>(g.knochen)]), plus(mal(aussen, 6.0F * m), V3{0.0F, 0.0F, 3.0F * m}));
        // rechts rot, links blau wie die Ringe (rigformen.cpp kCrRechts / kCrLinks)
        x.farbe = g.seite == modell::Seite::Rechts ? IM_COL32(235, 60, 50, 255) : IM_COL32(50, 110, 240, 255);
        x.text = ik >= 0.5F ? "IK" : "FK";
        s.texte.push_back(x);
    }
}

std::uint64_t szeneSchluessel() {
    const ModellTab& t = mt();
    const Settings& st = g_app->settings;
    std::uint64_t h = 1469598103934665603ULL;
    const std::vector<BoneMatrix>& w = g_app->modellWelt;
    if (!w.empty()) { h = fnv(h, w.data(), w.size() * sizeof(BoneMatrix)); }
    const int werte[] = {t.knochenWahl, t.griffWahl, g_app->modellKnochen ? 1 : 0, rigGezeigt() ? 1 : 0, static_cast<int>(t.bereich), st.modellKnochenArt, st.modellRigStil,
                         st.modellFlossen ? 1 : 0, st.modellBedienung, g_app->animFrame, t.dok.stand(), static_cast<int>(g_app->anim.bones.size()),
                         static_cast<int>(t.rig.griffe.size()), t.lodJetzt, static_cast<int>(ImGui::GetFontSize() * 16.0F)};
    h = fnv(h, werte, sizeof werte);
    h = fnv(h, t.rigFuer.data(), t.rigFuer.size());
    h = fnv(h, g_app->modelPath.data(), g_app->modelPath.size());
    for (std::size_t i = 0; i < g_app->anim.bones.size(); ++i) { h = fnvWert(h, static_cast<unsigned char>(modellKnochenVersteckt(static_cast<int>(i)) ? 1 : 0)); }
    for (const unsigned char v : t.griffVersteckt) { h = fnvWert(h, v); }
    return h;
}

const Szene& szene() {
    const std::uint64_t schluessel = szeneSchluessel();
    if (g_kd.gebaut && g_kd.schluessel == schluessel) { return g_kd.szene; }
    Szene s;
    const ModellTab& t = mt();
    const modell::RigStil stil = modellRigStil();
    const bool griffe = rigGezeigt() && stil != modell::RigStil::Standard;
    const bool knochen = g_app->modellKnochen && !g_app->anim.bones.empty() && g_app->modellWelt.size() == g_app->anim.bones.size();
    const bool welt = g_app->modellWelt.size() == g_app->anim.bones.size() && !g_app->anim.bones.empty();
    if (stil != modell::RigStil::Standard && welt && (knochen || griffe)) { stilFormenHolen(stil); }
    if (stil == modell::RigStil::Cat && welt) {
        // die Kaesten: mit "Knochen" alle, sonst die der Griffe (der Kasten ist der FK-Griff seines Knochens)
        for (const modell::RigForm& q : g_kd.stilSkelett.formen) {
            if (modellKnochenVersteckt(q.knochen)) { continue; }
            const bool griffDa = griffe && q.griff >= 0 && !(static_cast<std::size_t>(q.griff) < t.griffVersteckt.size() && t.griffVersteckt[static_cast<std::size_t>(q.griff)] != 0);
            if (!knochen && !griffDa) { continue; }
            stilFormBauen(s, q, griffDa);
            if (knochen) { ++s.knochen; }
        }
        s.massstab = g_kd.stilSkelett.massstab;
    } else if (knochen) {
        const bool wieBlender = modellKnochenWieBlender();
        knochenBauen(s, wieBlender, modellKnochenArt(), griffe && stil == modell::RigStil::Blender);
    } else if (griffe && stil == modell::RigStil::Blender && welt) {
        // ohne "Knochen" bleiben im Stil Blender-Oktaeder die Oktaeder, die Griffe sind - sonst waeren diese Griffe nicht zu fassen
        Szene alle;
        knochenBauen(alle, true, KnochenArt::Oktaeder, true);
        Bauer b(s);
        for (const Form& f : alle.formen) {
            if (f.info.griff < 0) { continue; }
            b.beginn();
            b.f.info = f.info;
            b.f.skelett = false;
            b.f.info.knochen = -1;
            for (int i = f.linienVon; i < f.linienBis; ++i) { s.linien.push_back(alle.linien[static_cast<std::size_t>(i)]); }
            for (int i = f.flaechenVon; i < f.flaechenBis; ++i) { s.flaechen.push_back(alle.flaechen[static_cast<std::size_t>(i)]); }
            for (int i = f.scheibenVon; i < f.scheibenBis; ++i) { s.scheiben.push_back(alle.scheiben[static_cast<std::size_t>(i)]); }
            for (int i = f.koerperVon; i < f.koerperBis; ++i) { s.koerper.push_back(alle.koerper[static_cast<std::size_t>(i)]); }
            const int punkte = f.info.punkte;
            b.ende();
            s.formen.back().info.punkte = punkte;
        }
    }
    if (griffe && welt) {
        for (const modell::RigForm& q : g_kd.stilGriffe.formen) {
            if (q.griff >= 0 && static_cast<std::size_t>(q.griff) < t.griffVersteckt.size() && t.griffVersteckt[static_cast<std::size_t>(q.griff)] != 0) { continue; }
            stilFormBauen(s, q, true);
        }
        s.massstab = g_kd.stilGriffe.massstab;
        if (stil == modell::RigStil::ControlRig) { beschriftungen(s); }
        // Child Of: vom Ziel der IK eine gestrichelte Linie zu seinem Raum (wie im Stil Standard, gui/app_modelltab_ik.cpp)
        if (const modell::RigSchicht* sch = modellRigSchichtJetzt()) {
            for (const modell::IkSpur& sp : sch->ik) {
                const modell::IkZiel z = modell::ikZielAm(sp, static_cast<float>(bildJetzt()), g_app->modellWelt);
                if (!z.da || z.ik <= 0.0F || z.raum < 0 || static_cast<std::size_t>(z.raum) >= g_app->modellWelt.size()) { continue; }
                s.linien.push_back(Linie{ortVon(z.ziel), ortVon(g_app->modellWelt[static_cast<std::size_t>(z.raum)]), IM_COL32(120, 220, 255, 200), 1.5F, true});
            }
        }
        std::vector<char> gesehen(t.rig.griffe.size(), 0);
        for (const Form& f : s.formen) {
            if (f.info.griff >= 0 && static_cast<std::size_t>(f.info.griff) < gesehen.size()) { gesehen[static_cast<std::size_t>(f.info.griff)] = 1; }
        }
        for (const char g : gesehen) { s.griffe += g != 0 ? 1 : 0; }
    }
    std::uint64_t h = 1469598103934665603ULL;
    if (!s.linien.empty()) { h = fnv(h, s.linien.data(), s.linien.size() * sizeof(Linie)); }
    if (!s.flaechen.empty()) { h = fnv(h, s.flaechen.data(), s.flaechen.size() * sizeof(Flaeche)); }
    if (!s.scheiben.empty()) { h = fnv(h, s.scheiben.data(), s.scheiben.size() * sizeof(Scheibe)); }
    s.summe = h;
    g_kd.szene = std::move(s);
    g_kd.schluessel = schluessel;
    g_kd.gebaut = true;
    return g_kd.szene;
}

// --- Projektion ---------------------------------------------------------------------------------------------------------------

struct Clip {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 1.0F;
};

Clip clipVon(const float* m, const V3& p) {
    return Clip{m[0] * p[0] + m[1] * p[1] + m[2] * p[2] + m[3], m[4] * p[0] + m[5] * p[1] + m[6] * p[2] + m[7], m[8] * p[0] + m[9] * p[1] + m[10] * p[2] + m[11],
                m[12] * p[0] + m[13] * p[1] + m[14] * p[2] + m[15]};
}

Clip mischen(const Clip& a, const Clip& b, float u) { return Clip{a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u, a.z + (b.z - a.z) * u, a.w + (b.w - a.w) * u}; }

// Die nahe Ebene fuer Linien und Scheiben (w ist der Abstand vor dem Auge; orthographisch 1)
constexpr float kNah = 0.05F;

// Eine Strecke an der nahen Ebene kappen; false: ganz dahinter
bool kappen(Clip& a, Clip& b) {
    if (a.w < kNah && b.w < kNah) { return false; }
    if (a.w < kNah) { a = mischen(a, b, (kNah - a.w) / (b.w - a.w)); }
    if (b.w < kNah) { b = mischen(b, a, (kNah - b.w) / (a.w - b.w)); }
    return true;
}

// Eine Sicht: Matrix und Groesse des Ziels in Bildpunkten; px: Faktor fuer Masse in Bildpunkten (Oberflaeche, groesseres Foto)
struct Sicht {
    const float* m = nullptr;
    float w = 1.0F;
    float h = 1.0F;
    float px = 1.0F;
    [[nodiscard]] ImVec2 schirm(const Clip& c) const { return ImVec2{(c.x / c.w * 0.5F + 0.5F) * w, (0.5F - c.y / c.w * 0.5F) * h}; }
};

void eckeAn(std::vector<gpu::ModellFormEcke>& v, const Sicht& s, const Clip& c, float dx, float dy, ImU32 farbe, float alpha) {
    gpu::ModellFormEcke e;
    e.pos[0] = c.x + dx * 2.0F / s.w * c.w;
    e.pos[1] = c.y - dy * 2.0F / s.h * c.w;
    e.pos[2] = c.z;
    e.pos[3] = c.w;
    e.farbe[0] = static_cast<std::uint8_t>((farbe >> IM_COL32_R_SHIFT) & 0xFFU);
    e.farbe[1] = static_cast<std::uint8_t>((farbe >> IM_COL32_G_SHIFT) & 0xFFU);
    e.farbe[2] = static_cast<std::uint8_t>((farbe >> IM_COL32_B_SHIFT) & 0xFFU);
    e.farbe[3] = static_cast<std::uint8_t>(std::clamp(static_cast<float>((farbe >> IM_COL32_A_SHIFT) & 0xFFU) * alpha, 0.0F, 255.0F));
    v.push_back(e);
}

// Ein Streifen von a nach b (schon gekappt): `dicke` Bildpunkte deckend in der Mitte, aussen je ein Bildpunkt Auslauf auf Alpha 0
void streifen(std::vector<gpu::ModellFormEcke>& v, const Sicht& s, const Clip& a, const Clip& b, ImU32 farbe, float dicke) {
    const ImVec2 pa = s.schirm(a);
    const ImVec2 pb = s.schirm(b);
    const float dx = pb.x - pa.x;
    const float dy = pb.y - pa.y;
    const float l = std::sqrt(dx * dx + dy * dy);
    if (l < 1e-3F) { return; }
    const float nx = -dy / l;
    const float ny = dx / l;
    // unter einem Bildpunkt Breite wird die Linie nicht schmaler, sondern blasser
    const float deckung = std::min(dicke, 1.0F);
    const float innen = std::max(dicke - 1.0F, 0.0F) * 0.5F;
    const float aussen = innen + 1.0F;
    const float quer[4] = {-aussen, -innen, innen, aussen};
    const float alpha[4] = {0.0F, deckung, deckung, 0.0F};
    for (int k = 0; k < 3; ++k) {
        if (k == 1 && innen <= 0.0F) { continue; }
        const float q0 = quer[k];
        const float q1 = quer[k + 1];
        eckeAn(v, s, a, nx * q0, ny * q0, farbe, alpha[k]);
        eckeAn(v, s, b, nx * q0, ny * q0, farbe, alpha[k]);
        eckeAn(v, s, b, nx * q1, ny * q1, farbe, alpha[k + 1]);
        eckeAn(v, s, a, nx * q0, ny * q0, farbe, alpha[k]);
        eckeAn(v, s, b, nx * q1, ny * q1, farbe, alpha[k + 1]);
        eckeAn(v, s, a, nx * q1, ny * q1, farbe, alpha[k + 1]);
    }
}

void linieProjizieren(std::vector<gpu::ModellFormEcke>& v, const Sicht& s, const Linie& q) {
    Clip a = clipVon(s.m, q.a);
    Clip b = clipVon(s.m, q.b);
    if (!kappen(a, b)) { return; }
    const float dicke = q.dicke * s.px;
    if (!q.gestrichelt) {
        streifen(v, s, a, b, q.farbe, dicke);
        return;
    }
    // gestrichelt: 6 Bildpunkte Strich, 4 Luecke (auf dem Schirm gemessen, im Raum richtig geteilt)
    const ImVec2 pa = s.schirm(a);
    const ImVec2 pb = s.schirm(b);
    const float l = std::hypot(pb.x - pa.x, pb.y - pa.y);
    const float strich = 6.0F * s.px;
    const float schritt = 10.0F * s.px;
    const auto teil = [&](float t) {
        const float u = std::clamp(t / std::max(l, 1e-3F), 0.0F, 1.0F);
        const float nenner = (1.0F - u) * b.w + u * a.w;
        return mischen(a, b, std::fabs(nenner) > 1e-6F ? u * a.w / nenner : u);
    };
    for (float t = 0.0F; t < l && l < 20000.0F; t += schritt) { streifen(v, s, teil(t), teil(std::min(t + strich, l)), q.farbe, dicke); }
}

void scheibeProjizieren(std::vector<gpu::ModellFormEcke>& v, const Sicht& s, const Scheibe& q) {
    const Clip c = clipVon(s.m, q.p);
    if (c.w < kNah) { return; }
    constexpr int kTeile = 14;
    constexpr float kPi2 = 6.2831853F;
    const float r = q.radius * s.px;
    for (int i = 0; i < kTeile; ++i) {
        const float w0 = kPi2 * static_cast<float>(i) / static_cast<float>(kTeile);
        const float w1 = kPi2 * static_cast<float>(i + 1) / static_cast<float>(kTeile);
        const float c0 = std::cos(w0);
        const float s0 = std::sin(w0);
        const float c1 = std::cos(w1);
        const float s1 = std::sin(w1);
        // der Rand: von r - 0,5 bis r + 0,5 auf Alpha 0 (gefuellt) bzw. ein Ring von einem Bildpunkt mit Auslauf
        const float ri = q.gefuellt ? std::max(r - 0.5F, 0.0F) : std::max(r - 1.2F * s.px, 0.0F);
        const float rm = q.gefuellt ? ri : std::max(r - 0.5F * s.px, 0.0F);
        const float ra = r + 0.5F;
        if (q.gefuellt) {
            eckeAn(v, s, c, 0.0F, 0.0F, q.farbe, 1.0F);
            eckeAn(v, s, c, c0 * ri, s0 * ri, q.farbe, 1.0F);
            eckeAn(v, s, c, c1 * ri, s1 * ri, q.farbe, 1.0F);
        } else {
            eckeAn(v, s, c, c0 * ri, s0 * ri, q.farbe, 0.0F);
            eckeAn(v, s, c, c0 * rm, s0 * rm, q.farbe, 1.0F);
            eckeAn(v, s, c, c1 * rm, s1 * rm, q.farbe, 1.0F);
            eckeAn(v, s, c, c0 * ri, s0 * ri, q.farbe, 0.0F);
            eckeAn(v, s, c, c1 * rm, s1 * rm, q.farbe, 1.0F);
            eckeAn(v, s, c, c1 * ri, s1 * ri, q.farbe, 0.0F);
        }
        eckeAn(v, s, c, c0 * rm, s0 * rm, q.farbe, 1.0F);
        eckeAn(v, s, c, c0 * ra, s0 * ra, q.farbe, 0.0F);
        eckeAn(v, s, c, c1 * ra, s1 * ra, q.farbe, 0.0F);
        eckeAn(v, s, c, c0 * rm, s0 * rm, q.farbe, 1.0F);
        eckeAn(v, s, c, c1 * ra, s1 * ra, q.farbe, 0.0F);
        eckeAn(v, s, c, c1 * rm, s1 * rm, q.farbe, 1.0F);
    }
}

// Welche der vier Ansichten `a` ist (das Foto arbeitet auf einer Kopie der aktiven)
int ansichtNr(const ModellAnsicht& a) {
    const ModellTab& t = mt();
    for (int i = 0; i < 4; ++i) {
        if (&t.ansichten[i] == &a) { return i; }
    }
    return std::clamp(t.aktiv, 0, 3);
}

Sicht sichtVon(const ModellAnsicht& a, int w, int h) {
    Sicht s;
    s.m = a.viewProj;
    s.w = static_cast<float>(std::max(w, 1));
    s.h = static_cast<float>(std::max(h, 1));
    // das Foto ist groesser als die Ansicht: Masse in Bildpunkten wachsen mit
    const ModellAnsicht& echt = mt().ansichten[ansichtNr(a)];
    const float foto = (&echt != &a && echt.groesse[0] > 1.0F) ? std::max(s.w / echt.groesse[0], 1.0F) : 1.0F;
    s.px = oberflaeche() * foto;
    return s;
}

// --- Treffen --------------------------------------------------------------------------------------------------------------

float abstandZurStrecke(ImVec2 p, ImVec2 a, ImVec2 b, float& u) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float l2 = dx * dx + dy * dy;
    u = l2 > 1e-6F ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / l2, 0.0F, 1.0F) : 0.0F;
    return std::hypot(p.x - (a.x + dx * u), p.y - (a.y + dy * u));
}

// Die Tiefe (z/w) der Haut der Figur unter der Maus; 2: keine Haut dort
float hautTiefe(const ModellAnsicht& a, ImVec2 maus) {
    int teil = -1;
    int dreieck = -1;
    if (!modellDreieckUnter(a, maus, teil, dreieck) || teil < 0) { return 2.0F; }
    const ModellTab& t = mt();
    const auto& h = modellHautOrte();
    if (static_cast<std::size_t>(teil) >= h.size() || static_cast<std::size_t>(teil) >= t.anzeige.surfaces.size()) { return 2.0F; }
    const GlmSurface& fl = t.anzeige.surfaces[static_cast<std::size_t>(teil)];
    const std::size_t k = static_cast<std::size_t>(dreieck) * 3U;
    if (k + 2U >= fl.indexes.size()) { return 2.0F; }
    float tief = 2.0F;
    for (std::size_t e = 0; e < 3; ++e) {
        const std::size_t i = fl.indexes[k + e];
        if (i >= h[static_cast<std::size_t>(teil)].size()) { return 2.0F; }
        const auto& o = h[static_cast<std::size_t>(teil)][i];
        const Clip c = clipVon(a.viewProj, {o[0], o[1], o[2]});
        if (c.w > 1e-3F) { tief = std::min(tief, c.z / c.w); }   // die naechste Ecke des Dreiecks: was davor liegt, liegt sicher vor der Haut
    }
    return tief;
}

struct Treffer {
    int form = -1;
    float abstand = 1.0e9F;   // zur naechsten Linie (Bildpunkte); im Koerper 0
    float tiefe = 2.0F;
    bool linie = false;
};

// Die Form unter der Maus unter denen, die `waehlbar` annimmt
Treffer formUnter(const ModellAnsicht& a, ImVec2 maus, const std::function<bool(const Form&)>& waehlbar) {
    const Szene& s = szene();
    Treffer best;
    const float tol = std::max(5.0F, ImGui::GetFontSize() * 0.3F);
    const ImVec2 lo{a.lo[0], a.lo[1]};
    Sicht sicht;
    sicht.m = a.viewProj;
    sicht.w = a.groesse[0];
    sicht.h = a.groesse[1];
    const ImVec2 m{maus.x - lo.x, maus.y - lo.y};
    // verdeckt die Figur die Formen, zaehlt nur, was vor ihrer Haut liegt
    const bool verdeckt = !g_app->settings.modellKnochenVorn && !modellDurchsicht(ansichtNr(a));
    const float haut = verdeckt ? hautTiefe(a, maus) : 2.0F;
    for (std::size_t fi = 0; fi < s.formen.size(); ++fi) {
        const Form& f = s.formen[fi];
        if (!waehlbar(f)) { continue; }
        Treffer t;
        t.form = static_cast<int>(fi);
        for (int i = f.linienVon; i < f.linienBis; ++i) {
            const Linie& q = s.linien[static_cast<std::size_t>(i)];
            Clip ca = clipVon(sicht.m, q.a);
            Clip cb = clipVon(sicht.m, q.b);
            if (!kappen(ca, cb)) { continue; }
            float u = 0.0F;
            const float d = abstandZurStrecke(m, sicht.schirm(ca), sicht.schirm(cb), u);
            const float halb = q.dicke * oberflaeche() * 0.5F;
            if (d - halb < t.abstand && d - halb <= tol) {
                const Clip c = mischen(ca, cb, u);
                t.abstand = std::max(d - halb, 0.0F);
                t.tiefe = c.z / c.w;
                t.linie = true;
            }
        }
        for (int i = f.scheibenVon; i < f.scheibenBis; ++i) {
            const Scheibe& q = s.scheiben[static_cast<std::size_t>(i)];
            const Clip c = clipVon(sicht.m, q.p);
            if (c.w < kNah) { continue; }
            const ImVec2 p = sicht.schirm(c);
            const float d = std::max(std::hypot(m.x - p.x, m.y - p.y) - q.radius * oberflaeche(), 0.0F);
            if (d < t.abstand && d <= tol) {
                t.abstand = d;
                t.tiefe = c.z / c.w;
                t.linie = true;
            }
        }
        if (!t.linie) {
            for (int i = f.koerperVon; i < f.koerperBis; ++i) {
                const Flaeche& q = s.koerper[static_cast<std::size_t>(i)];
                const Clip c0 = clipVon(sicht.m, q.p[0]);
                const Clip c1 = clipVon(sicht.m, q.p[1]);
                const Clip c2 = clipVon(sicht.m, q.p[2]);
                if (c0.w < kNah || c1.w < kNah || c2.w < kNah) { continue; }
                const ImVec2 p0 = sicht.schirm(c0);
                const ImVec2 p1 = sicht.schirm(c1);
                const ImVec2 p2 = sicht.schirm(c2);
                const float fl = (p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y);
                if (std::fabs(fl) < 1e-6F) { continue; }
                const float u = ((p1.x - m.x) * (p2.y - m.y) - (p2.x - m.x) * (p1.y - m.y)) / fl;
                const float v = ((p2.x - m.x) * (p0.y - m.y) - (p0.x - m.x) * (p2.y - m.y)) / fl;
                const float w = 1.0F - u - v;
                if (u < 0.0F || v < 0.0F || w < 0.0F) { continue; }
                const float tiefe = u * c0.z / c0.w + v * c1.z / c1.w + w * c2.z / c2.w;
                if (t.abstand > 0.0F || tiefe < t.tiefe) {
                    t.abstand = 0.0F;
                    t.tiefe = tiefe;
                }
            }
            if (t.abstand > 0.0F) { continue; }
        }
        if (verdeckt && t.tiefe > haut) { continue; }
        // eine Linie schlaegt einen Koerper; unter Linien die naechste, bei gleichem Abstand (halber Bildpunkt) die vordere;
        // unter Koerpern der vordere
        const bool besser = best.form < 0 || (t.linie && !best.linie) ||
                            (t.linie == best.linie && (t.linie ? (t.abstand < best.abstand - 0.5F || (t.abstand <= best.abstand + 0.5F && t.tiefe < best.tiefe)) : t.tiefe < best.tiefe));
        if (besser) { best = t; }
    }
    return best;
}

bool griffWaehlbar(const Form& f) { return f.info.griff >= 0 && !f.info.schmuck; }
bool knochenWaehlbar(const Form& f) { return f.skelett && f.info.knochen >= 0 && !modellKnochenEingefroren(f.info.knochen); }

// --- Bedienung ----------------------------------------------------------------------------------------------------------------

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

void geaendert(const char* was) {
    g_app->modelDirty = true;
    ++mt().tastenZaehler;
    mt().letzteTaste = was;
}

constexpr Str kStilText[modell::kRigStile] = {Str::KdStilStandard, Str::KdStilControl, Str::KdStilCat, Str::KdStilBlender};
constexpr Str kStilHinweis[modell::kRigStile] = {Str::KdStilStandardHint, Str::KdStilControlHint, Str::KdStilCatHint, Str::KdStilBlenderHint};
constexpr Str kArtText[kKnochenArten] = {Str::KdArtOktaeder, Str::KdArtStick, Str::KdArtBBone, Str::KdArtHuelle, Str::KdArtDraht};
constexpr Str kArtHinweis[kKnochenArten] = {Str::KdArtOktaederHint, Str::KdArtStickHint, Str::KdArtBBoneHint, Str::KdArtHuelleHint, Str::KdArtDrahtHint};

}  // namespace

ModellKd& modellKd() {
    static ModellKd k;
    return k;
}

KnochenArt modellKnochenArt() { return static_cast<KnochenArt>(std::clamp(g_app->settings.modellKnochenArt, 0, kKnochenArten - 1)); }

void modellKnochenArtSetzen(KnochenArt a) {
    g_app->settings.modellKnochenArt = std::clamp(static_cast<int>(a), 0, kKnochenArten - 1);
    geaendert(tr(kArtText[g_app->settings.modellKnochenArt]));
}

modell::RigStil modellRigStil() { return static_cast<modell::RigStil>(std::clamp(g_app->settings.modellRigStil, 0, modell::kRigStile - 1)); }

void modellRigStilSetzen(modell::RigStil s) {
    g_app->settings.modellRigStil = std::clamp(static_cast<int>(s), 0, modell::kRigStile - 1);
    geaendert(tr(kStilText[g_app->settings.modellRigStil]));
}

bool modellKnochenWieBlender() { return modellBlender() || modellRigStil() == modell::RigStil::Blender; }

void modellDurchsichtUmschalten(int ansicht) {
    ModellKd& k = modellKd();
    const int n = std::clamp(ansicht, 0, 3);
    if (modellBlender()) {
        k.durchsicht[n] = !k.durchsicht[n];   // Blender: View3DShading.show_xray gehoert der einen 3D-Ansicht
    } else {
        const bool an = !k.durchsicht[n];     // 3ds Max: See-Through ist eine Eigenschaft des Objekts - jede Ansicht zeigt es
        for (bool& d : k.durchsicht) { d = an; }
    }
    geaendert(tr(modellBlender() ? Str::KdRoentgen : Str::KdDurchsicht));
}

bool modellDurchsicht(int ansicht) { return modellKd().durchsicht[std::clamp(ansicht, 0, 3)]; }

bool modellKdGriffeAlsFormen() { return rigGezeigt() && modellRigStil() != modell::RigStil::Standard; }

void modellKdFormen(const ModellAnsicht& a, int w, int h, gpu::ModellAnsichtArt& art) {
    const int nr = ansichtNr(a);
    const bool durch = modellDurchsicht(nr);
    art.durchsicht = durch ? kDurchsichtAlpha : 0.0F;
    // Blender zeigt Knochen mit X-Ray vor der Figur; 3ds Max laesst sie in der durchscheinenden Figur liegen
    art.formenVorn = g_app->settings.modellKnochenVorn || (durch && modellBlender());
    art.formen.clear();
    ModellKd& k = modellKd();
    const bool aktiv = nr == std::clamp(mt().aktiv, 0, 3);
    if (modellPr().overlaysAus[nr]) {   // AP5 P10: Overlays aus wie Blender - nichts ueber dem Modell
        if (aktiv) {
            k.formenGezeichnet = 0;
            k.eckenGezeichnet = 0;
        }
        return;
    }
    const Szene& s = szene();
    const Sicht sicht = sichtVon(a, w, h);
    art.formen.reserve(s.flaechen.size() * 3U + s.linien.size() * 18U + s.scheiben.size() * 14U * 9U);
    for (const Flaeche& f : s.flaechen) {
        for (const V3& p : f.p) { eckeAn(art.formen, sicht, clipVon(sicht.m, p), 0.0F, 0.0F, f.farbe, 1.0F); }
    }
    for (const Linie& l : s.linien) { linieProjizieren(art.formen, sicht, l); }
    for (const Scheibe& q : s.scheiben) { scheibeProjizieren(art.formen, sicht, q); }
    if (aktiv) {
        k.formenGezeichnet = static_cast<int>(s.formen.size());
        k.eckenGezeichnet = static_cast<int>(art.formen.size());
    }
}

std::string modellKdStand(const ModellAnsicht& a, int w, int h) {
    const int nr = ansichtNr(a);
    const Szene& s = szene();
    // die Zaehler des Bilds (bis rc611 je Bild in knochenZeichnen): zwoelf Linien je gezeichnetem Knochen
    if (!modellPr().overlaysAus[nr]) {
        g_app->modellKnochenLinien += 12 * s.knochen;
        mt().ueber[0] += 12 * s.knochen;
        if (nr == std::clamp(mt().aktiv, 0, 3)) { mt().griffeGezeichnet += s.griffe; }
    }
    const Sicht sicht = sichtVon(a, w, h);
    char z[96];
    std::snprintf(z, sizeof z, "|kd%016llx|%d%d|%.2f", static_cast<unsigned long long>(s.summe), g_app->settings.modellKnochenVorn ? 1 : 0, modellDurchsicht(nr) ? 1 : 0,
                  static_cast<double>(sicht.px));
    return z;
}

void modellKdUeberlagerung(const ModellAnsicht& a, ImDrawList* dl) {
    const Szene& s = szene();
    const int nr = ansichtNr(a);
    int n = 0;
    ImFont* font = ImGui::GetFont();
    const float groesse = ImGui::GetFontSize() * 1.15F;
    for (const Beschriftung& b : s.texte) {
        ImVec2 p;
        if (!modellAufSchirm(a, b.p.data(), p)) { continue; }
        const ImVec2 g = font->CalcTextSizeA(groesse, FLT_MAX, 0.0F, b.text);
        const ImVec2 o{std::floor(p.x - g.x * 0.5F), std::floor(p.y - g.y * 0.5F)};
        dl->AddText(font, groesse, ImVec2{o.x + 1.0F, o.y + 1.0F}, IM_COL32(0, 0, 0, 200), b.text);
        dl->AddText(font, groesse, o, b.farbe, b.text);
        ++n;
    }
    if (nr == std::clamp(mt().aktiv, 0, 3)) { modellKd().beschriftungen = n; }
}

int modellKdGriffUnter(const ModellAnsicht& a, ImVec2 maus) {
    if (!modellKdGriffeAlsFormen()) { return -1; }
    const Treffer t = formUnter(a, maus, griffWaehlbar);
    return t.form >= 0 ? szene().formen[static_cast<std::size_t>(t.form)].info.griff : -1;
}

int modellKdKnochenUnter(const ModellAnsicht& a, ImVec2 maus) {
    if (!g_app->modellKnochen) { return -1; }
    const Treffer t = formUnter(a, maus, knochenWaehlbar);
    return t.form >= 0 ? szene().formen[static_cast<std::size_t>(t.form)].info.knochen : -1;
}

bool modellKdTasten() {
    const bool bl = modellBlender();
    if (!ImGui::IsKeyChordPressed(bl ? (ImGuiMod_Alt | ImGuiKey_Z) : (ImGuiMod_Alt | ImGuiKey_X))) { return false; }
    modellDurchsichtUmschalten(mt().aktiv);
    return true;
}

bool modellKdAktionenSuche(const std::string& such) {
    bool ausgeloest = false;
    const bool bl = modellBlender();
    const std::string ort = tr(Str::KdOrt);
    const auto zeile = [&](const std::string& text, bool an, const std::string& tip, const std::function<void()>& tun, int id, bool geht = true) {
        if (!such.empty() && klein(text).find(such) == std::string::npos) { return; }
        ImGui::PushID(id);
        const std::string z = std::string(an ? "[x] " : "[  ] ") + text;
        if (ImGui::Selectable(z.c_str()) && !ausgeloest && geht) {
            tun();
            ausgeloest = true;
        }
        ImGui::SetItemTooltip("%s\n%s", tip.c_str(), ort.c_str());
        ImGui::PopID();
    };
    const int n = std::clamp(mt().aktiv, 0, 3);
    zeile(std::string(tr(bl ? Str::KdRoentgen : Str::KdDurchsicht)) + (bl ? " (Alt+Z)" : " (Alt+X)"), modellDurchsicht(n), tr(bl ? Str::KdRoentgenHint : Str::KdDurchsichtHint),
          [n] { modellDurchsichtUmschalten(n); }, 13000);
    zeile(tr(Str::KdVorn), g_app->settings.modellKnochenVorn, tr(Str::KdVornHint), [] {
        g_app->settings.modellKnochenVorn = !g_app->settings.modellKnochenVorn;
        geaendert(tr(Str::KdVorn));
    }, 13001);
    for (int i = 0; i < modell::kRigStile; ++i) {
        zeile(std::string(tr(Str::KdRigStil)) + ": " + tr(kStilText[i]), static_cast<int>(modellRigStil()) == i, tr(kStilHinweis[i]),
              [i] { modellRigStilSetzen(static_cast<modell::RigStil>(i)); }, 13010 + i);
    }
    const bool wieBl = modellKnochenWieBlender();
    for (int i = 0; i < kKnochenArten; ++i) {
        zeile(std::string(tr(Str::KdAnzeigenAls)) + ": " + tr(kArtText[i]), wieBl && static_cast<int>(modellKnochenArt()) == i, tr(wieBl ? kArtHinweis[i] : Str::KdArtBraucht),
              [i] { modellKnochenArtSetzen(static_cast<KnochenArt>(i)); }, 13020 + i, wieBl);
    }
    zeile(tr(Str::KdGruppe), false, tr(Str::KdRigStilHint), [] {
        mt().bereich = ModellBereich::Animation;
        modellKd().oeffnen = true;
        g_app->modelDirty = true;
    }, 13030);
    return ausgeloest;
}

void modellKdMenue(int) {
    Settings& st = g_app->settings;
    modellKd().menueBild = ImGui::GetFrameCount();
    ImGui::Separator();
    if (ImGui::MenuItem(tr(Str::KdVorn), nullptr, st.modellKnochenVorn)) {
        st.modellKnochenVorn = !st.modellKnochenVorn;
        geaendert(tr(Str::KdVorn));
    }
    ImGui::SetItemTooltip("%s", tr(Str::KdVornHint));
    const bool artOffen = ImGui::BeginMenu(tr(Str::KdAnzeigenAls));
    if (!artOffen) { ImGui::SetItemTooltip("%s", tr(Str::KdAnzeigenAlsHint)); }   // offen verdeckte er die Arten
    if (artOffen) {
        const bool wieBl = modellKnochenWieBlender();
        for (int i = 0; i < kKnochenArten; ++i) {
            if (ImGui::MenuItem(tr(kArtText[i]), nullptr, wieBl && static_cast<int>(modellKnochenArt()) == i, wieBl)) { modellKnochenArtSetzen(static_cast<KnochenArt>(i)); }
            ImGui::SetItemTooltip("%s", tr(wieBl ? kArtHinweis[i] : Str::KdArtBraucht));
        }
        ImGui::EndMenu();
    }
    const bool stilOffen = ImGui::BeginMenu(tr(Str::KdRigStil));
    if (!stilOffen) { ImGui::SetItemTooltip("%s", tr(Str::KdRigStilHint)); }
    if (stilOffen) {
        for (int i = 0; i < modell::kRigStile; ++i) {
            if (ImGui::MenuItem(tr(kStilText[i]), nullptr, static_cast<int>(modellRigStil()) == i)) { modellRigStilSetzen(static_cast<modell::RigStil>(i)); }
            ImGui::SetItemTooltip("%s", tr(kStilHinweis[i]));
        }
        ImGui::EndMenu();
    }
}

void modellKdMenueSchattierung(int ansicht) {
    const bool bl = modellBlender();
    const int n = std::clamp(ansicht, 0, 3);
    if (ImGui::MenuItem(tr(bl ? Str::KdRoentgen : Str::KdDurchsicht), bl ? "Alt+Z" : "Alt+X", modellDurchsicht(n))) { modellDurchsichtUmschalten(n); }
    ImGui::SetItemTooltip("%s", tr(bl ? Str::KdRoentgenHint : Str::KdDurchsichtHint));
}

void modellKdRollout() {
    ModellKd& k = modellKd();
    ModellTab& t = mt();
    Settings& st = g_app->settings;
    const bool bl = modellBlender();
    const int stil = static_cast<int>(modellRigStil());
    if (k.oeffnen) {
        ui::rolloutSetzen(tr(Str::KdGruppe), true);
        ui::rolloutZeigen(tr(Str::KdGruppe));
        k.oeffnen = false;
    }
    if (!ui::rolloutBeginn(tr(Str::KdGruppe), {.kurzinfo = tr(kStilText[stil]), .zu = true, .reiter = modellReiter(MbReiter::Anzeige, MbReiterBl::Daten),
                                               .hinweis = tr(Str::KdRigStilHint)})) {
        return;
    }
    k.rolloutBild = ImGui::GetFrameCount();
    ui::absatz(tr(Str::KdRigStil));
    const char* stile[modell::kRigStile];
    const char* stilHinweise[modell::kRigStile];
    for (int i = 0; i < modell::kRigStile; ++i) {
        stile[i] = tr(kStilText[i]);
        stilHinweise[i] = tr(kStilHinweis[i]);
    }
    int wahl = stil;
    if (ui::segment("##kdstil", stile, modell::kRigStile, &wahl, 2, -1, stilHinweise, 0.0F, true)) { modellRigStilSetzen(static_cast<modell::RigStil>(wahl)); }
    ui::rasterBeginn(2);
    if (ui::schalter(tr(Str::MbAnKnochen), &g_app->modellKnochen, tr(Str::MkKnochenHint))) { geaendert(tr(Str::MbAnKnochen)); }
    if (ui::schalter(tr(Str::MbRgGriffeZeigen), &t.rigZeigen, tr(Str::MbRgGriffeZeigenHint))) { geaendert(tr(Str::MbRgGriffeZeigen)); }
    if (ui::schalter(tr(Str::KdVorn), &st.modellKnochenVorn, tr(Str::KdVornHint))) { geaendert(tr(Str::KdVorn)); }
    const int n = std::clamp(t.aktiv, 0, 3);
    bool durch = modellDurchsicht(n);
    if (ui::schalter(tr(bl ? Str::KdRoentgen : Str::KdDurchsicht), &durch, tr(bl ? Str::KdRoentgenHint : Str::KdDurchsichtHint))) { modellDurchsichtUmschalten(n); }
    if (!modellKnochenWieBlender()) {
        if (ui::schalter(tr(Str::MkFlossen), &st.modellFlossen, tr(Str::MkFlossenHint))) { geaendert(tr(Str::MkFlossen)); }
    }
    ui::rasterEnde();
    if (modellKnochenWieBlender()) {
        ui::absatz(tr(Str::KdAnzeigenAls));
        const char* arten[kKnochenArten];
        const char* artHinweise[kKnochenArten];
        for (int i = 0; i < kKnochenArten; ++i) {
            arten[i] = tr(kArtText[i]);
            artHinweise[i] = tr(kArtHinweis[i]);
        }
        int art = static_cast<int>(modellKnochenArt());
        if (ui::segment("##kdart", arten, kKnochenArten, &art, 3, -1, artHinweise, 0.0F, true)) { modellKnochenArtSetzen(static_cast<KnochenArt>(art)); }
    }
    ui::tipp(tr(bl ? Str::KdTippBl : Str::KdTippMax));
    ui::rolloutEnde();
}

// --- Selbsttest -----------------------------------------------------------------------------------------------------------------

int modellKdFormZahl() { return static_cast<int>(szene().formen.size()); }

KdFormInfo modellKdFormInfo(int form) {
    const Szene& s = szene();
    return form >= 0 && static_cast<std::size_t>(form) < s.formen.size() ? s.formen[static_cast<std::size_t>(form)].info : KdFormInfo{};
}

int modellKdFormVonGriff(int griff) {
    const Szene& s = szene();
    for (std::size_t i = 0; i < s.formen.size(); ++i) {
        if (griff >= 0 && s.formen[i].info.griff == griff) { return static_cast<int>(i); }
    }
    return -1;
}

int modellKdFormVonKnochen(int knochen) {
    const Szene& s = szene();
    for (std::size_t i = 0; i < s.formen.size(); ++i) {
        if (knochen >= 0 && s.formen[i].skelett && s.formen[i].info.knochen == knochen) { return static_cast<int>(i); }
    }
    return -1;
}

bool modellKdFormPunkt(int form, int ansicht, ImVec2& out) {
    const Szene& s = szene();
    if (form < 0 || static_cast<std::size_t>(form) >= s.formen.size() || ansicht < 0 || ansicht > 3) { return false; }
    const Form& f = s.formen[static_cast<std::size_t>(form)];
    const ModellAnsicht& a = mt().ansichten[ansicht];
    if (!a.sichtbar) { return false; }
    const bool alsGriff = griffWaehlbar(f);
    const float rand = 10.0F;
    // die Mitte jeder Linie, dann ihre Viertel: der erste Punkt im Bild, an dem ein Klick genau diese Form trifft
    for (const float u : {0.5F, 0.25F, 0.75F}) {
        for (int i = f.linienVon; i < f.linienBis; ++i) {
            const Linie& q = s.linien[static_cast<std::size_t>(i)];
            const V3 p3 = plus(q.a, mal(minus(q.b, q.a), u));
            ImVec2 p;
            if (!modellAufSchirm(a, p3.data(), p)) { continue; }
            if (p.x < a.lo[0] + rand || p.y < std::max(a.lo[1], a.kopfUnten) + rand || p.x > a.lo[0] + a.groesse[0] - rand || p.y > a.lo[1] + a.groesse[1] - rand) { continue; }
            if (modellKopfTreffer(a, p)) { continue; }
            const Treffer t = formUnter(a, p, alsGriff ? std::function<bool(const Form&)>(griffWaehlbar) : std::function<bool(const Form&)>(knochenWaehlbar));
            const bool derselbe = t.form == form || (t.form >= 0 && alsGriff && s.formen[static_cast<std::size_t>(t.form)].info.griff == f.info.griff);
            if (derselbe || (!alsGriff && !f.skelett && t.form < 0)) {
                out = p;
                return true;
            }
        }
    }
    return false;
}

float modellKdMassstab() {
    (void)szene();
    if (!g_kd.stilGriffe.formen.empty()) { return g_kd.stilGriffe.massstab; }
    if (!g_kd.stilSkelett.formen.empty()) { return g_kd.stilSkelett.massstab; }
    return modell::rigMassstab(g_app->anim, rigFuerSkelett().erkennung);
}

}  // namespace bhed::gui
