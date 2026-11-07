// modell/retarget.cpp - Modell-Block Stufe 5: Retargeting (Rechnung und Herleitung in bhed/modell/retarget.h)
#include "bhed/modell/retarget.h"

#include "bhed/modell/ik.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>

#include "g2/compress.h"

namespace bhed::modell {
namespace {

using V3 = std::array<float, 3>;
constexpr float kPi = 3.14159265358979F;

std::size_t uz(int i) { return static_cast<std::size_t>(i); }
V3 ort(const BoneMatrix& m) { return {m.m[0][3], m.m[1][3], m.m[2][3]}; }
V3 sub(const V3& a, const V3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
V3 add(const V3& a, const V3& b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
V3 mul(const V3& a, float s) { return {a[0] * s, a[1] * s, a[2] * s}; }
float dot(const V3& a, const V3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
V3 kreuz(const V3& a, const V3& b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
float laenge(const V3& a) { return std::sqrt(dot(a, a)); }
V3 norm(const V3& a) {
    const float l = laenge(a);
    return l > 1e-9F ? mul(a, 1.0F / l) : V3{0.0F, 0.0F, 0.0F};
}
std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

BoneMatrix nurDrehung(const BoneMatrix& m) {
    BoneMatrix r = starr(m);
    r.m[0][3] = r.m[1][3] = r.m[2][3] = 0.0F;
    return r;
}
BoneMatrix mitOrt(BoneMatrix r, const V3& p) {
    r.m[0][3] = p[0];
    r.m[1][3] = p[1];
    r.m[2][3] = p[2];
    return r;
}
BoneMatrix transponiert(const BoneMatrix& r) {
    BoneMatrix t = BoneMatrix::identity();
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { t.m[i][j] = r.m[j][i]; }
    }
    return t;
}
V3 drehe(const BoneMatrix& r, const V3& v) {
    return {r.m[0][0] * v[0] + r.m[0][1] * v[1] + r.m[0][2] * v[2], r.m[1][0] * v[0] + r.m[1][1] * v[1] + r.m[1][2] * v[2],
            r.m[2][0] * v[0] + r.m[2][1] * v[1] + r.m[2][2] * v[2]};
}
// Drehung um die Achse a (Einheit) um w Bogenmass (Rodrigues)
BoneMatrix achsDrehung(const V3& a, float w) {
    const float c = std::cos(w);
    const float s = std::sin(w);
    const float t = 1.0F - c;
    BoneMatrix r = BoneMatrix::identity();
    r.m[0][0] = t * a[0] * a[0] + c;
    r.m[0][1] = t * a[0] * a[1] - s * a[2];
    r.m[0][2] = t * a[0] * a[2] + s * a[1];
    r.m[1][0] = t * a[0] * a[1] + s * a[2];
    r.m[1][1] = t * a[1] * a[1] + c;
    r.m[1][2] = t * a[1] * a[2] - s * a[0];
    r.m[2][0] = t * a[0] * a[2] - s * a[1];
    r.m[2][1] = t * a[1] * a[2] + s * a[0];
    r.m[2][2] = t * a[2] * a[2] + c;
    return r;
}
// Die kleinste Drehung von Richtung `von` nach `nach`
BoneMatrix kleinsteDrehung(const V3& von, const V3& nach) {
    const V3 a = norm(von);
    const V3 b = norm(nach);
    if (laenge(a) < 0.5F || laenge(b) < 0.5F) { return BoneMatrix::identity(); }
    const float c = std::clamp(dot(a, b), -1.0F, 1.0F);
    V3 ax = kreuz(a, b);
    if (laenge(ax) < 1e-6F) {
        if (c > 0.0F) { return BoneMatrix::identity(); }
        ax = std::fabs(a[0]) < 0.9F ? kreuz(a, V3{1.0F, 0.0F, 0.0F}) : kreuz(a, V3{0.0F, 1.0F, 0.0F});
    }
    return achsDrehung(norm(ax), std::acos(c));
}
float winkelGrad(const BoneMatrix& a, const BoneMatrix& b) {
    const BoneMatrix d = transponiert(nurDrehung(a)) * nurDrehung(b);
    const float tr = std::clamp((d.m[0][0] + d.m[1][1] + d.m[2][2] - 1.0F) * 0.5F, -1.0F, 1.0F);
    return std::acos(tr) * 180.0F / kPi;
}
// Vorgaenger in der Ankerkette: dem der Knochen folgt, sonst sein Eltern
int anker(const GlaAnimation& g, const RtCharakter& c, int b) {
    if (uz(b) < c.folgt.size() && c.folgt[uz(b)] >= 0) { return c.folgt[uz(b)]; }
    return g.bones[uz(b)].parent;
}

// Reihenfolge nach Ankern (Anker vor Folger)
std::vector<int> ankerFolge(const GlaAnimation& g, const RtCharakter& c) {
    const int n = static_cast<int>(g.bones.size());
    std::vector<int> aus;
    std::vector<std::uint8_t> da(uz(n), 0);
    for (int runde = 0; runde <= n && static_cast<int>(aus.size()) < n; ++runde) {
        for (int b = 0; b < n; ++b) {
            if (da[uz(b)] != 0) { continue; }
            const int a = anker(g, c, b);
            if (a < 0 || a >= n || da[uz(a)] != 0 || runde == n) {
                da[uz(b)] = 1;
                aus.push_back(b);
            }
        }
    }
    return aus;
}

// Teilbaum nach Ankern (b und alles, was ihm folgt)
std::vector<std::uint8_t> ankerTeilbaum(const GlaAnimation& g, const RtCharakter& c, const std::vector<int>& folge, int b) {
    std::vector<std::uint8_t> drin(g.bones.size(), 0);
    if (b < 0) { return drin; }
    drin[uz(b)] = 1;
    for (const int d : folge) {
        const int a = anker(g, c, d);
        if (a >= 0 && drin[uz(a)] != 0) { drin[uz(d)] = 1; }
    }
    return drin;
}

// --- Vorlagen ------------------------------------------------------------------------------------------------------
// art: P Becken, W Wurzel, M Bewegung, S Wirbel, H Hals, K Kopf, C Schluesselbein, A Arm, B Bein, F Finger, T Anhang,
// t Drehknochen (Segment ueber die Lage). seite: M L R. muster: genau (Spiel) oder Endung (Mixamo/HumanIK, Biped, CAT).
struct Eintrag {
    const char* name;
    char art;
    char seite;
    int nr;
    int stelle;
};
struct Vorlage {
    const char* name;
    const char* datei;
    int knochen;
    float massstab;
    bool endung;
    const Eintrag* e;
    int zahl;
};

#include "rig_vorlagen.inc"

bool passt(const Vorlage& v, const std::string& kn, const char* muster) {
    const std::string m = klein(muster);
    if (!v.endung) { return kn == m; }
    return kn.size() >= m.size() && kn.compare(kn.size() - m.size(), m.size(), m) == 0;
}

int findeKnochen(const GlaAnimation& g, const Vorlage& v, const char* muster) {
    for (std::size_t i = 0; i < g.bones.size(); ++i) {
        if (passt(v, klein(g.bones[i].name), muster)) { return static_cast<int>(i); }
    }
    return -1;
}

Seite seiteVon(char c) { return c == 'L' ? Seite::Links : (c == 'R' ? Seite::Rechts : Seite::Mitte); }

bool artAus(char c, KettenArt& a) {
    switch (c) {
        case 'S': a = KettenArt::Wirbel; return true;
        case 'H': a = KettenArt::Hals; return true;
        case 'K': a = KettenArt::Kopf; return true;
        case 'C': a = KettenArt::Schluessel; return true;
        case 'A': a = KettenArt::Arm; return true;
        case 'B': a = KettenArt::Bein; return true;
        case 'F': a = KettenArt::Finger; return true;
        case 'T': a = KettenArt::Anhang; return true;
        default: return false;
    }
}

std::string dateiSchwanz(const std::string& d) {
    std::string k = klein(d);
    for (char& c : k) {
        if (c == '\\') { c = '/'; }
    }
    const std::size_t p = k.find("models/");
    return p == std::string::npos ? k : k.substr(p);
}

// Drehknochen: das Segment (Anfang, Ende) der naechsten Arm-/Beinkette der Seite, Anteil ueber die Lage
void twistZuordnen(const GlaAnimation& g, RtCharakter& c, int t, Seite seite) {
    float best = 1e30F;
    int bk = -1;
    int bi = -1;
    const V3 p = ort(g.bones[uz(t)].basePose);
    for (std::size_t k = 0; k < c.ketten.size(); ++k) {
        const RtKette& ke = c.ketten[k];
        if ((ke.art != KettenArt::Arm && ke.art != KettenArt::Bein) || ke.seite != seite) { continue; }
        for (std::size_t i = 0; i + 1 < ke.knochen.size() && i < 2; ++i) {
            const V3 a = ort(g.bones[uz(ke.knochen[i])].basePose);
            const V3 e = ort(g.bones[uz(ke.knochen[i + 1])].basePose);
            const V3 ae = sub(e, a);
            const float l2 = dot(ae, ae);
            if (l2 < 1e-9F) { continue; }
            const float u = std::clamp(dot(sub(p, a), ae) / l2, 0.0F, 1.0F);
            const float d = laenge(sub(p, add(a, mul(ae, u))));
            if (d < best) {
                best = d;
                bk = static_cast<int>(k);
                bi = static_cast<int>(i);
            }
        }
    }
    if (bk < 0) { return; }
    RtKette& ke = c.ketten[uz(bk)];
    const int a = ke.knochen[uz(bi)];
    const int e = ke.knochen[uz(bi + 1)];
    const V3 pa = ort(g.bones[uz(a)].basePose);
    const V3 ae = sub(ort(g.bones[uz(e)].basePose), pa);
    c.twistAnfang[uz(t)] = a;
    c.twistEnde[uz(t)] = e;
    c.twistAnteil[uz(t)] = std::clamp(dot(sub(p, pa), ae) / std::max(dot(ae, ae), 1e-9F), 0.0F, 1.0F);
    ke.twist.push_back(t);
}

// Gemeinsamer Abschluss: Ketten sortieren, Drehknochen, Folger (Finger und Tags der Hand unter dem Unterarm, Glieder einer
// Fingerkette im flachen JK2-Skelett).
void abschliessen(const GlaAnimation& g, RtCharakter& c, const std::vector<std::pair<int, Seite>>& twists) {
    const std::size_t n = g.bones.size();
    c.folgt.assign(n, -1);
    c.twistAnteil.assign(n, 0.0F);
    c.twistEnde.assign(n, -1);
    c.twistAnfang.assign(n, -1);
    std::sort(c.ketten.begin(), c.ketten.end(), [](const RtKette& a, const RtKette& b) {
        return std::tie(a.art, a.seite, a.nummer) < std::tie(b.art, b.seite, b.nummer);
    });
    for (const auto& [t, s] : twists) { twistZuordnen(g, c, t, s); }
    const auto istNachfahre = [&](int b, int vorfahr) {
        int schritte = 0;
        for (int p = g.bones[uz(b)].parent; p >= 0 && schritte <= static_cast<int>(n); p = g.bones[uz(p)].parent, ++schritte) {
            if (p == vorfahr) { return true; }
        }
        return false;
    };
    const auto setze = [&](int b, int vor) {
        if (b < 0 || vor < 0 || b == vor || c.folgt[uz(b)] >= 0 || istNachfahre(b, vor) || istNachfahre(vor, b)) { return; }
        c.folgt[uz(b)] = vor;
    };
    const auto letzter = [&](KettenArt a, int sonst) {
        for (const RtKette& k : c.ketten) {
            if (k.art == a && k.seite == Seite::Mitte && k.nummer == 0 && !k.knochen.empty()) { return k.knochen.back(); }
        }
        return sonst;
    };
    // das naechste Ende (Hand/Fuss: drittes Glied, sonst das letzte) einer Arm-/Beinkette der Seite
    const auto naechstesEnde = [&](KettenArt a, Seite s, int b) {
        int best = -1;
        float d = 1e30F;
        for (const RtKette& k : c.ketten) {
            if (k.art != a || k.seite != s || k.knochen.empty()) { continue; }
            const int e = k.knochen[std::min<std::size_t>(2, k.knochen.size() - 1)];
            const float x = laenge(sub(ort(g.bones[uz(e)].basePose), ort(g.bones[uz(b)].basePose)));
            if (x < d) {
                d = x;
                best = e;
            }
        }
        return best;
    };
    const int wirbelEnde = letzter(KettenArt::Wirbel, c.becken);
    const int halsEnde = letzter(KettenArt::Hals, wirbelEnde);
    // Im flachen Skelett (JK2: Arme und Finger unter thoracic; Kreaturen: Fuesse unter model_root) folgt jedes Glied dem
    // vorigen der Kette und jeder Ansatz seinem Platz am Koerper - so bleibt die Kette zusammen, auch wenn die Datei sie
    // nicht verbindet. Knochen, die schon darunter haengen, behalten ihr Eltern.
    for (const RtKette& k : c.ketten) {
        for (std::size_t i = 1; i < k.knochen.size(); ++i) { setze(k.knochen[i], k.knochen[i - 1]); }
        if (k.knochen.empty()) { continue; }
        const int a = k.knochen[0];
        switch (k.art) {
            case KettenArt::Wirbel: case KettenArt::Bein: setze(a, c.becken); break;
            case KettenArt::Hals: case KettenArt::Schluessel: setze(a, wirbelEnde); break;
            case KettenArt::Kopf: setze(a, halsEnde); break;
            case KettenArt::Arm: {
                int sch = -1;
                for (const RtKette& o : c.ketten) {
                    if (o.art == KettenArt::Schluessel && o.seite == k.seite && o.nummer == k.nummer && !o.knochen.empty()) { sch = o.knochen.back(); }
                }
                setze(a, sch >= 0 ? sch : wirbelEnde);
                break;
            }
            case KettenArt::Finger: setze(a, naechstesEnde(KettenArt::Arm, k.seite, a)); break;
            case KettenArt::Anhang: break;
        }
    }
    // uebrige Knochen: Tags und Finger der naechsten Hand, Zehen und Fersen dem naechsten Fuss der Seite; ein Knochen am selben
    // Ort wie ein Kettenglied (Wampa L_foot_bone = ltalus, Rancor L_heel_bone = L_foot_bone) folgt ihm
    const Erkennung er = erkenneKnochen(g);
    for (std::size_t b = 0; b < n; ++b) {
        const int bi = static_cast<int>(b);
        if (c.folgt[b] >= 0 || c.ketteVon(bi) >= 0 || bi == c.becken || bi == c.wurzel || bi == c.bewegung || c.twistEnde[b] >= 0) { continue; }
        for (const RtKette& k : c.ketten) {
            for (const int kb : k.knochen) {
                if (laenge(sub(ort(g.bones[uz(kb)].basePose), ort(g.bones[b].basePose))) < 0.01F) { setze(bi, kb); }
            }
        }
        if (c.folgt[b] >= 0 || b >= er.knochen.size()) { continue; }
        const Rolle r = er.knochen[b].rolle;
        if (r == Rolle::TagKnochen || r == Rolle::Finger) { setze(bi, naechstesEnde(KettenArt::Arm, er.knochen[b].seite, bi)); }
        if (r == Rolle::Zehen || r == Rolle::Fuss) { setze(bi, naechstesEnde(KettenArt::Bein, er.knochen[b].seite, bi)); }
    }
}

RtCharakter ausVorlage(const GlaAnimation& g, const Vorlage& v) {
    RtCharakter c;
    c.vorlage = v.name;
    c.massstab = g.scale;
    std::map<std::tuple<KettenArt, Seite, int>, std::vector<std::pair<int, int>>> ketten;
    std::vector<std::pair<int, Seite>> twists;
    for (int i = 0; i < v.zahl; ++i) {
        const Eintrag& e = v.e[i];
        const int b = findeKnochen(g, v, e.name);
        if (b < 0) { continue; }
        KettenArt a = KettenArt::Wirbel;
        if (e.art == 'P') {
            c.becken = b;
        } else if (e.art == 'W') {
            c.wurzel = b;
        } else if (e.art == 'M') {
            c.bewegung = b;
        } else if (e.art == 't') {
            twists.emplace_back(b, seiteVon(e.seite));
        } else if (artAus(e.art, a)) {
            ketten[{a, seiteVon(e.seite), e.nr}].emplace_back(e.stelle, b);
        }
    }
    for (auto& [k, v2] : ketten) {
        std::sort(v2.begin(), v2.end());
        RtKette ke;
        std::tie(ke.art, ke.seite, ke.nummer) = k;
        for (const auto& [s, b] : v2) { ke.knochen.push_back(b); }
        c.ketten.push_back(std::move(ke));
    }
    abschliessen(g, c, twists);
    return c;
}

// Fingergruppe aus dem Namen: Thumb, Index, Middle, Ring, Pinky wie HumanIK/Biped (Finger0..4 = 1..5), die Ziffer nach "d"
// (JKA/JK2 l_d4_j1: d1 Daumen, d2 Zeigefinger), sonst 0
int fingerNummer(const std::string& name) {
    const std::string k = klein(name);
    const char* const worte[] = {"thumb", "index", "mid", "ring", "pinky"};
    for (int i = 0; i < 5; ++i) {
        if (k.find(worte[i]) != std::string::npos) { return i + 1; }
    }
    for (std::size_t i = 0; i + 1 < k.size(); ++i) {
        if (k[i] == 'd' && std::isdigit(static_cast<unsigned char>(k[i + 1])) != 0 && (i == 0 || !std::isalpha(static_cast<unsigned char>(k[i - 1])))) {
            return k[i + 1] - '0';
        }
    }
    return 0;
}

}  // namespace

const char* kettenSchluessel(KettenArt a) noexcept {
    switch (a) {
        case KettenArt::Wirbel: return "spine";
        case KettenArt::Hals: return "neck";
        case KettenArt::Kopf: return "head";
        case KettenArt::Schluessel: return "clavicle";
        case KettenArt::Arm: return "arm";
        case KettenArt::Bein: return "leg";
        case KettenArt::Finger: return "finger";
        case KettenArt::Anhang: return "tail";
    }
    return "?";
}

std::string RtKette::schluessel() const {
    std::string s = kettenSchluessel(art);
    if (seite != Seite::Mitte) { s += seite == Seite::Links ? "_l" : "_r"; }
    return s + "_" + std::to_string(nummer);
}

int RtCharakter::ketteVon(int knochen) const noexcept {
    for (std::size_t k = 0; k < ketten.size(); ++k) {
        for (const int b : ketten[k].knochen) {
            if (b == knochen) { return static_cast<int>(k); }
        }
    }
    return -1;
}

std::vector<RtVorlageInfo> rtVorlagen() {
    std::vector<RtVorlageInfo> aus;
    for (const Vorlage& v : kVorlagen) {
        RtVorlageInfo i;
        i.name = v.name;
        i.datei = v.datei;
        i.knochen = v.knochen;
        i.massstab = v.massstab;
        std::vector<std::tuple<char, char, int>> k;
        for (int j = 0; j < v.zahl; ++j) {
            if (std::strchr("SHKCABFT", v.e[j].art) != nullptr) { k.emplace_back(v.e[j].art, v.e[j].seite, v.e[j].nr); }
        }
        std::sort(k.begin(), k.end());
        i.ketten = static_cast<int>(std::unique(k.begin(), k.end()) - k.begin());
        aus.push_back(std::move(i));
    }
    return aus;
}

int rtVorlageFuer(const GlaAnimation& g, const std::string& datei) {
    const std::string d = dateiSchwanz(datei.empty() ? g.skeletonName + ".gla" : datei);
    const int n = static_cast<int>(sizeof kVorlagen / sizeof kVorlagen[0]);
    for (int i = 0; i < n; ++i) {
        const Vorlage& v = kVorlagen[i];
        if (v.datei[0] != '\0' && dateiSchwanz(v.datei) == d && v.knochen == static_cast<int>(g.bones.size())) { return i; }
    }
    int best = -1;
    int bestTreffer = 0;
    float bestAnteil = 0.0F;
    for (int i = 0; i < n; ++i) {
        const Vorlage& v = kVorlagen[i];
        int treffer = 0;
        for (int j = 0; j < v.zahl; ++j) { treffer += findeKnochen(g, v, v.e[j].name) >= 0 ? 1 : 0; }
        // Anteil an den Knochen, die beide haben koennen: eine Vorlage mit Fingern erkennt auch ein Skelett ohne sie
        const int nenner = std::min(v.zahl, static_cast<int>(g.bones.size()));
        const float anteil = nenner > 0 ? static_cast<float>(treffer) / static_cast<float>(nenner) : 0.0F;
        // bei gleichem Anteil die Vorlage mit mehr gefundenen Knochen (JK2 vor JKA und Protokolldroide im JK2-Skelett)
        if (anteil > bestAnteil + 1e-6F || (std::fabs(anteil - bestAnteil) <= 1e-6F && best >= 0 && treffer > bestTreffer)) {
            bestAnteil = anteil;
            bestTreffer = treffer;
            best = i;
        }
    }
    return bestAnteil >= 0.8F ? best : -1;
}

RtCharakter rtVorschlag(const GlaAnimation& g) {
    RtCharakter c;
    c.massstab = g.scale;
    const Rig r = rigAnlegen(g, Inhalt{}, 0, {});
    const Erkennung& er = r.erkennung;
    std::map<std::tuple<KettenArt, Seite, int>, std::vector<std::pair<int, int>>> ketten;
    std::vector<std::pair<int, Seite>> twists;
    for (const Griff& gr : r.griffe) {
        if (gr.ik || gr.knochen < 0) { continue; }
        KettenArt a = KettenArt::Wirbel;
        int stelle = 0;
        int nr = gr.nummer;
        switch (gr.art) {
            case GriffArt::Becken: c.becken = gr.knochen; continue;
            case GriffArt::Wirbel: a = KettenArt::Wirbel; stelle = gr.nummer; nr = 0; break;
            case GriffArt::Hals: a = KettenArt::Hals; stelle = gr.nummer; nr = 0; break;
            case GriffArt::Kopf: a = KettenArt::Kopf; stelle = gr.nummer; nr = 0; break;
            case GriffArt::Schluessel: a = KettenArt::Schluessel; nr = gr.nummer - 1; break;
            case GriffArt::Oberarm: a = KettenArt::Arm; stelle = 0; nr = gr.nummer - 1; break;
            case GriffArt::Unterarm: a = KettenArt::Arm; stelle = 1; nr = gr.nummer - 1; break;
            case GriffArt::Hand: a = KettenArt::Arm; stelle = 2; nr = gr.nummer - 1; break;
            case GriffArt::Oberschenkel: a = KettenArt::Bein; stelle = 0; nr = gr.nummer - 1; break;
            case GriffArt::Unterschenkel: a = KettenArt::Bein; stelle = 1; nr = gr.nummer - 1; break;
            case GriffArt::Fuss: a = KettenArt::Bein; stelle = 2; nr = gr.nummer - 1; break;
            case GriffArt::Zehen: a = KettenArt::Bein; stelle = 3; nr = gr.nummer - 1; break;
            default: continue;
        }
        ketten[{a, gr.seite, nr}].emplace_back(stelle, gr.knochen);
    }
    // Finger (Gruppe aus dem Namen, sonst verbundene Folgen), Anhaenge (verbundene Folgen), Drehknochen, Wurzel/Bewegung
    const std::size_t n = g.bones.size();
    std::map<std::pair<Seite, int>, std::vector<int>> finger;
    std::vector<int> anhangAnfang;
    for (std::size_t b = 0; b < n && b < er.knochen.size(); ++b) {
        const KnochenErkennung& k = er.knochen[b];
        const int bi = static_cast<int>(b);
        const int p = g.bones[b].parent;
        const Rolle pr = p >= 0 && uz(p) < er.knochen.size() ? er.knochen[uz(p)].rolle : Rolle::Keine;
        switch (k.rolle) {
            case Rolle::Wurzel: if (c.wurzel < 0) { c.wurzel = bi; } break;
            case Rolle::Bewegung: if (c.bewegung < 0) { c.bewegung = bi; } break;
            case Rolle::OberarmTwist: case Rolle::UnterarmTwist: case Rolle::OberschenkelTwist: twists.emplace_back(bi, k.seite); break;
            case Rolle::Finger: {
                int nr = fingerNummer(g.bones[b].name);
                if (nr == 0) {
                    // ohne Ziffer: die Folge ab dem ersten Fingerknochen
                    int a = bi;
                    for (int q = p; q >= 0 && uz(q) < er.knochen.size() && er.knochen[uz(q)].rolle == Rolle::Finger; q = g.bones[uz(q)].parent) { a = q; }
                    nr = 100 + a;
                }
                finger[{k.seite, nr}].push_back(bi);
                break;
            }
            case Rolle::Anhang: if (pr != Rolle::Anhang) { anhangAnfang.push_back(bi); } break;
            default: break;
        }
    }
    for (auto& [k, v] : finger) {
        // Reihenfolge: Tiefe in der Hierarchie, dann Abstand von der Hand der Seite (Grundstellung)
        int hand = -1;
        for (const Griff& gr : r.griffe) {
            if (gr.art == GriffArt::Hand && gr.seite == k.first && hand < 0) { hand = gr.knochen; }
        }
        const auto tiefe = [&](int b) {
            int t = 0;
            for (int p = g.bones[uz(b)].parent; p >= 0 && t <= static_cast<int>(n); p = g.bones[uz(p)].parent) { ++t; }
            return t;
        };
        const auto abstand = [&](int b) { return hand < 0 ? 0.0F : laenge(sub(ort(g.bones[uz(b)].basePose), ort(g.bones[uz(hand)].basePose))); };
        std::sort(v.begin(), v.end(), [&](int a, int b) { return std::make_pair(tiefe(a), abstand(a)) < std::make_pair(tiefe(b), abstand(b)); });
        auto& z = ketten[{KettenArt::Finger, k.first, k.second}];
        for (std::size_t i = 0; i < v.size(); ++i) { z.emplace_back(static_cast<int>(i), v[i]); }
    }
    std::map<Seite, int> anhangNr;
    for (const int a : anhangAnfang) {
        const Seite s = er.knochen[uz(a)].seite;
        auto& z = ketten[{KettenArt::Anhang, s, anhangNr[s]++}];
        int b = a;
        for (int stelle = 0; b >= 0 && stelle < static_cast<int>(n); ++stelle) {
            z.emplace_back(stelle, b);
            int naechster = -1;
            for (const int k : g.bones[uz(b)].kinder) {
                if (uz(k) < er.knochen.size() && er.knochen[uz(k)].rolle == Rolle::Anhang) {
                    naechster = k;
                    break;
                }
            }
            b = naechster;
        }
    }
    // Fingergruppen fortlaufend je Seite (Ziffern bleiben, Folgen ohne Ziffer zaehlen hinter der hoechsten weiter)
    for (auto& [k, v2] : ketten) {
        std::sort(v2.begin(), v2.end());
        RtKette ke;
        std::tie(ke.art, ke.seite, ke.nummer) = k;
        for (const auto& [s, b] : v2) {
            if (std::find(ke.knochen.begin(), ke.knochen.end(), b) == ke.knochen.end()) { ke.knochen.push_back(b); }
        }
        c.ketten.push_back(std::move(ke));
    }
    std::map<Seite, int> fingerZaehler;
    for (RtKette& ke : c.ketten) {
        if (ke.art != KettenArt::Finger || ke.nummer < 100) { continue; }
        int& z = fingerZaehler[ke.seite];
        if (z == 0) {
            for (const RtKette& o : c.ketten) {
                if (o.art == KettenArt::Finger && o.seite == ke.seite && o.nummer < 100) { z = std::max(z, o.nummer); }
            }
        }
        ke.nummer = ++z;
    }
    c.vorgeschlagen = static_cast<int>(c.ketten.size());
    abschliessen(g, c, twists);
    return c;
}

RtCharakter rtCharakter(const GlaAnimation& g, const std::string& datei, int vorlage) {
    const int n = static_cast<int>(sizeof kVorlagen / sizeof kVorlagen[0]);
    const int v = vorlage == -2 ? rtVorlageFuer(g, datei) : vorlage;
    if (v >= 0 && v < n) {
        RtCharakter c = ausVorlage(g, kVorlagen[v]);
        if (c.becken >= 0 && !c.ketten.empty()) { return c; }
    }
    return rtVorschlag(g);
}

// --- Zuordnung -------------------------------------------------------------------------------------------------------

RtZuordnung rtZuordnen(const RtCharakter& q, const RtCharakter& z, const RtZuordnung* alt) {
    RtZuordnung zu;
    zu.quelle.assign(z.ketten.size(), -1);
    zu.vonHand.assign(z.ketten.size(), 0);
    if (alt != nullptr && alt->quelle.size() == z.ketten.size()) {
        for (std::size_t i = 0; i < z.ketten.size(); ++i) {
            if (i < alt->vonHand.size() && alt->vonHand[i] != 0) {
                zu.quelle[i] = alt->quelle[i] < static_cast<int>(q.ketten.size()) ? alt->quelle[i] : -1;
                zu.vonHand[i] = 1;
            }
        }
    }
    for (std::size_t i = 0; i < z.ketten.size(); ++i) {
        if (zu.vonHand[i] != 0) { continue; }
        const std::string s = z.ketten[i].schluessel();
        for (std::size_t j = 0; j < q.ketten.size(); ++j) {
            if (q.ketten[j].schluessel() == s) { zu.quelle[i] = static_cast<int>(j); }
        }
    }
    return zu;
}

std::vector<int> rtKnochenZuordnung(const GlaAnimation& gq, const RtCharakter& q, const GlaAnimation& gz, const RtCharakter& z,
                                    const RtZuordnung& zu) {
    std::vector<int> m(gz.bones.size(), -1);
    // Lage auf der Kette: Bogenlaenge in der Grundstellung, normiert (0 Ansatz, 1 Spitze); einzelne Knochen 0
    const auto bogen = [](const GlaAnimation& g, const RtKette& k) {
        std::vector<float> s(k.knochen.size(), 0.0F);
        for (std::size_t i = 1; i < k.knochen.size(); ++i) {
            s[i] = s[i - 1] + laenge(sub(ort(g.bones[uz(k.knochen[i])].basePose), ort(g.bones[uz(k.knochen[i - 1])].basePose)));
        }
        const float l = s.empty() ? 0.0F : s.back();
        for (float& v : s) { v = l > 1e-6F ? v / l : 0.0F; }
        return s;
    };
    for (std::size_t i = 0; i < z.ketten.size() && i < zu.quelle.size(); ++i) {
        const int j = zu.quelle[i];
        if (j < 0 || uz(j) >= q.ketten.size()) { continue; }
        const RtKette& kz = z.ketten[i];
        const RtKette& kq = q.ketten[uz(j)];
        if (kq.knochen.empty()) { continue; }
        // gleiche Zahl: Glied fuer Glied; sonst der gleiche Name in der Kette (Finger l_d4_j2 in JK2 und JKA), sonst ueber die
        // Lage auf der Kette (der naechste Quellknochen)
        if (kz.knochen.size() == kq.knochen.size()) {
            for (std::size_t a = 0; a < kz.knochen.size(); ++a) { m[uz(kz.knochen[a])] = kq.knochen[a]; }
        } else {
            const std::vector<float> sz = bogen(gz, kz);
            const std::vector<float> sq = bogen(gq, kq);
            for (std::size_t a = 0; a < kz.knochen.size(); ++a) {
                const std::string nz = klein(gz.bones[uz(kz.knochen[a])].name);
                const auto gleich = std::find_if(kq.knochen.begin(), kq.knochen.end(), [&](int b) { return klein(gq.bones[uz(b)].name) == nz; });
                if (gleich != kq.knochen.end()) {
                    m[uz(kz.knochen[a])] = *gleich;
                    continue;
                }
                std::size_t best = 0;
                float d = 2.0F;
                for (std::size_t b = 0; b < kq.knochen.size(); ++b) {
                    const float e = std::fabs(sq[b] - sz[a]);
                    if (e < d - 1e-6F) {
                        d = e;
                        best = b;
                    }
                }
                m[uz(kz.knochen[a])] = kq.knochen[best];
            }
        }
    }
    if (z.becken >= 0 && q.becken >= 0) { m[uz(z.becken)] = q.becken; }
    if (z.wurzel >= 0 && q.wurzel >= 0) { m[uz(z.wurzel)] = q.wurzel; }
    if (z.bewegung >= 0 && q.bewegung >= 0) { m[uz(z.bewegung)] = q.bewegung; }
    // uebrige ueber den gleichen Namen (Gesicht, Drehknochen, Tags), wenn der Quellknochen nicht schon einer Kette gehoert
    std::unordered_map<std::string, int> namen;
    for (std::size_t b = 0; b < gq.bones.size(); ++b) { namen.emplace(klein(gq.bones[b].name), static_cast<int>(b)); }
    for (std::size_t b = 0; b < gz.bones.size(); ++b) {
        if (m[b] >= 0 || z.ketteVon(static_cast<int>(b)) >= 0) { continue; }
        const auto it = namen.find(klein(gz.bones[b].name));
        if (it != namen.end() && q.ketteVon(it->second) < 0) { m[b] = it->second; }
    }
    return m;
}

// --- Retarget-Pose -----------------------------------------------------------------------------------------------------

namespace {
// Seitenachse und Vorzeichen (links) aus der Erkennung
void seitenAchse(const GlaAnimation& g, int& achse, float& links) {
    const Erkennung e = erkenneKnochen(g);
    achse = e.seitenAchse;
    links = e.linksVorzeichen;
}

V3 sollRichtung(const RtKette& k, std::size_t glied, RtHaltung h, int achse, float links) {
    if (k.art == KettenArt::Wirbel || k.art == KettenArt::Hals) { return {0.0F, 0.0F, 1.0F}; }
    if (k.art == KettenArt::Bein) { return {0.0F, 0.0F, -1.0F}; }
    V3 s{0.0F, 0.0F, 0.0F};
    s[uz(achse)] = (k.seite == Seite::Links ? 1.0F : -1.0F) * links;
    (void)glied;
    if (h == RtHaltung::A) { return norm(V3{s[0], s[1], -1.0F}); }   // 45 Grad nach unten
    return s;
}
}  // namespace

void rtPoseWelt(const GlaAnimation& g, const RtCharakter& c, const RtPose& p, std::vector<BoneMatrix>& welt) {
    const std::size_t n = g.bones.size();
    welt.resize(n);
    for (std::size_t b = 0; b < n; ++b) { welt[b] = g.bones[b].basePose; }   // Grundstellung: X(b) = B(b)
    const std::vector<int> folge = ankerFolge(g, c);
    const auto drehTeilbaum = [&](int b, const BoneMatrix& w) {
        const std::vector<std::uint8_t> drin = ankerTeilbaum(g, c, folge, b);
        const V3 o = ort(welt[uz(b)]);
        for (std::size_t d = 0; d < n; ++d) {
            if (drin[d] == 0) { continue; }
            welt[d] = mitOrt(w * nurDrehung(welt[d]), add(o, drehe(w, sub(ort(welt[d]), o)))) * starrInvers(starr(welt[d])) * welt[d];
        }
    };
    if (p.haltung != RtHaltung::Bindepose) {
        int achse = 0;
        float links = 1.0F;
        seitenAchse(g, achse, links);
        // Ketten von oben nach unten (Wirbel vor Arm), je Glied die kleinste Drehung zur Sollrichtung
        std::vector<std::size_t> reihe;
        for (std::size_t k = 0; k < c.ketten.size(); ++k) {
            const KettenArt a = c.ketten[k].art;
            if (a == KettenArt::Wirbel || a == KettenArt::Hals || a == KettenArt::Arm || a == KettenArt::Bein) { reihe.push_back(k); }
        }
        std::stable_sort(reihe.begin(), reihe.end(), [&](std::size_t x, std::size_t y) {
            const auto rang = [](KettenArt a) { return a == KettenArt::Wirbel ? 0 : (a == KettenArt::Hals ? 1 : 2); };
            return rang(c.ketten[x].art) < rang(c.ketten[y].art);
        });
        for (const std::size_t k : reihe) {
            const RtKette& ke = c.ketten[k];
            // Arme und Beine: Ober- und Unterglied; Wirbel/Hals: jedes Glied zum naechsten (der letzte Wirbel zum Hals)
            const std::size_t glieder = (ke.art == KettenArt::Arm || ke.art == KettenArt::Bein) ? std::min<std::size_t>(2, ke.knochen.size() - 1)
                                                                                                 : ke.knochen.size() - 1;
            for (std::size_t i = 0; i < glieder && i + 1 < ke.knochen.size(); ++i) {
                const V3 d = sub(ort(welt[uz(ke.knochen[i + 1])]), ort(welt[uz(ke.knochen[i])]));
                if (laenge(d) < 1e-4F) { continue; }
                drehTeilbaum(ke.knochen[i], kleinsteDrehung(d, sollRichtung(ke, i, p.haltung, achse, links)));
            }
        }
    }
    if (p.vonHand.size() == n) {
        for (const int b : folge) {
            const std::array<float, 4>& q = p.vonHand[uz(b)];
            if (std::fabs(q[0] - 1.0F) < 1e-7F && std::fabs(q[1]) + std::fabs(q[2]) + std::fabs(q[3]) < 1e-7F) { continue; }
            const BoneMatrix m = matrixAusQuat(q.data());
            const BoneMatrix r = nurDrehung(welt[uz(b)]);
            drehTeilbaum(b, r * m * transponiert(r));   // M im Rahmen des Knochens -> Weltdrehung X M X^-1
        }
    }
}

float rtPoseAbweichung(const GlaAnimation& gq, const RtCharakter& q, const std::vector<BoneMatrix>& wq, const GlaAnimation& gz,
                       const RtCharakter& z, const std::vector<BoneMatrix>& wz, const RtZuordnung& zu) {
    (void)gq;
    (void)gz;
    float groesste = 0.0F;
    for (std::size_t i = 0; i < z.ketten.size() && i < zu.quelle.size(); ++i) {
        const int j = zu.quelle[i];
        if (j < 0 || uz(j) >= q.ketten.size()) { continue; }   // pfix M5: wie rtKnochenZuordnung
        const RtKette& kz = z.ketten[i];
        const RtKette& kq = q.ketten[uz(j)];
        if (kz.art != KettenArt::Arm && kz.art != KettenArt::Bein) { continue; }
        for (std::size_t g = 0; g < 2 && g + 1 < kz.knochen.size() && g + 1 < kq.knochen.size(); ++g) {
            const V3 a = norm(sub(ort(wz[uz(kz.knochen[g + 1])]), ort(wz[uz(kz.knochen[g])])));
            const V3 b = norm(sub(ort(wq[uz(kq.knochen[g + 1])]), ort(wq[uz(kq.knochen[g])])));
            groesste = std::max(groesste, std::acos(std::clamp(dot(a, b), -1.0F, 1.0F)) * 180.0F / kPi);
        }
    }
    return groesste;
}

// --- Uebertragen -------------------------------------------------------------------------------------------------------

void rtWeltQuelle(const GlaAnimation& g, int bild, std::vector<BoneMatrix>& welt) { g.worldMatrices(bild, welt); }

namespace {

// Hoehe des Beckens ueber dem Knoechel (Fuss = drittes Glied der Beinkette, sonst ihr letztes; ohne Beine der tiefste Knochen).
// Der Knoechel, nicht der tiefste Fusspunkt: JK2 hat Zehenknochen (ltarsal 3,2 unter dem Knoechel), JKA nicht - bei gleichen
// Beinen gab der tiefste Punkt k = 0,906 und die Fuss-IK musste das Knie fast gestreckt nachfuehren (Spruenge an lfemurYZ).
float beinLaenge(const RtCharakter& c, const std::vector<BoneMatrix>& w) {
    if (c.becken < 0) { return 0.0F; }
    float tief = 1e30F;
    for (const RtKette& k : c.ketten) {
        if (k.art != KettenArt::Bein || k.knochen.empty()) { continue; }
        tief = std::min(tief, ort(w[uz(k.knochen[std::min<std::size_t>(2, k.knochen.size() - 1)])])[2]);
    }
    if (tief > 1e29F) {
        for (const BoneMatrix& m : w) { tief = std::min(tief, m.m[2][3]); }
    }
    return ort(w[uz(c.becken)])[2] - tief;
}

// Fusspunkte einer Beinkette: Fuss und Zehen (Ursprungsorte)
std::vector<int> fussKnochen(const RtKette& k) {
    std::vector<int> v;
    for (std::size_t i = 2; i < k.knochen.size(); ++i) { v.push_back(k.knochen[i]); }
    if (v.empty() && !k.knochen.empty()) { v.push_back(k.knochen.back()); }
    return v;
}

struct Zustand {
    const GlaAnimation* g = nullptr;
    const RtCharakter* c = nullptr;
    std::vector<int> folge;
    std::vector<BoneMatrix> pose;      // Retarget-Pose des Ziels, starr (Drehung und Ort)
    std::vector<BoneMatrix> mass;      // je Knochen das Mass der Grundstellung (fScale: B = 0,64 R in den GLAs des Spiels)
};

// Die Pose des Ziels starr rechnen und das Mass merken: X = E B traegt das Mass von B (mdxaSkel_t BasePoseMat mit fScale, die
// _humanoid.gla 0,64) - innen wird mit starren Lagen gerechnet, am Ende X = starr * Mass (sonst gaebe das Backen A = B(p) X(p)^-1
// X(b) B(b)^-1 mit falschem Mass und falscher Lage).
void starrMitMass(Zustand& st) {
    st.mass.resize(st.pose.size());
    for (std::size_t b = 0; b < st.pose.size(); ++b) {
        const BoneMatrix r = starr(st.pose[b]);
        st.mass[b] = starrInvers(r) * st.pose[b];
        st.pose[b] = r;
    }
}
void mitMass(const Zustand& st, std::vector<BoneMatrix>& w) {
    for (std::size_t b = 0; b < w.size() && b < st.mass.size(); ++b) { w[b] = w[b] * st.mass[b]; }
}

// Was einem Knochen folgt, starr mitnehmen: X'(d) = X'(a) X(a)^-1 X(d) fuer jeden Folger d eines geaenderten Ankers
void mitnehmen(const Zustand& z, const std::vector<BoneMatrix>& vorher, std::vector<BoneMatrix>& w, std::vector<std::uint8_t>& geaendert) {
    for (const int d : z.folge) {
        if (geaendert[uz(d)] != 0) { continue; }
        const int a = anker(*z.g, *z.c, d);
        if (a < 0 || geaendert[uz(a)] == 0) { continue; }
        w[uz(d)] = w[uz(a)] * starrInvers(vorher[uz(a)]) * vorher[uz(d)];
        geaendert[uz(d)] = 1;
    }
}

// Drehknochen ohne Quelle: Anteil der Drehung des Endknochens um die Achse des Segments (Swing-Twist der Abweichung vom
// starren Mitgehen)
void drehknochen(const Zustand& z, const std::vector<int>& mq, std::vector<BoneMatrix>& w) {
    const RtCharakter& c = *z.c;
    for (std::size_t t = 0; t < c.twistEnde.size(); ++t) {
        const int e = c.twistEnde[t];
        const int a = c.twistAnfang[t];
        if (e < 0 || a < 0 || mq[t] >= 0) { continue; }
        const BoneMatrix ra = nurDrehung(w[uz(a)]);
        const BoneMatrix starrE = ra * transponiert(nurDrehung(z.pose[uz(a)])) * nurDrehung(z.pose[uz(e)]);
        const BoneMatrix d = nurDrehung(w[uz(e)]) * transponiert(starrE);   // Weltdrehung: starr -> tatsaechlich
        float q[4];
        quatAusMatrix(d, q);
        const V3 achse = norm(sub(ort(w[uz(e)]), ort(w[uz(a)])));
        // Twist um die Achse: q_t = normiert(w, (v . a) a)
        const float p = q[1] * achse[0] + q[2] * achse[1] + q[3] * achse[2];
        const float l = std::sqrt(q[0] * q[0] + p * p);
        float winkel = 0.0F;
        if (l > 1e-8F) { winkel = 2.0F * std::atan2(p, q[0]); }
        if (winkel > kPi) { winkel -= 2.0F * kPi; }
        if (winkel < -kPi) { winkel += 2.0F * kPi; }
        const BoneMatrix tw = achsDrehung(achse, winkel * c.twistAnteil[t]);
        const BoneMatrix r = tw * ra * transponiert(nurDrehung(z.pose[uz(a)])) * nurDrehung(z.pose[t]);
        // Ort: starr mit dem Anfang
        const BoneMatrix rel = starrInvers(z.pose[uz(a)]) * z.pose[t];
        w[t] = mitOrt(r, ort(w[uz(a)] * rel));
    }
}

// Zwei-Glied-IK auf der Welt (Stufe 4 zweiGliedIk): Ende c nach `ziel` (mit Anteil reach), Pol = das Gelenk der jetzigen
// Pose weit genug von der Linie; gestreckt die Beugerichtung der Pose. Ober- und Unterglied drehen um die kleinste Drehung,
// das Ende behaelt seine Drehung; was den Gliedern folgt, geht starr mit. `beuge`: die Richtung des Gelenks, wenn Bild und Pose
// gestreckt sind (Knie nach vorn, Ellbogen nach hinten).
void ikKette(const Zustand& st, std::vector<BoneMatrix>& w, int ia, int ib, int ic, V3 ziel, float reach, const V3& beuge) {
    const V3 pa = ort(w[uz(ia)]);
    const V3 pb = ort(w[uz(ib)]);
    const V3 pc = ort(w[uz(ic)]);
    ziel = add(pc, mul(sub(ziel, pc), reach));
    const V3 mitte = mul(add(pa, pc), 0.5F);
    V3 polR = sub(pb, mitte);
    if (laenge(polR) < 1e-3F) {
        polR = sub(ort(st.pose[uz(ib)]), mul(add(ort(st.pose[uz(ia)]), ort(st.pose[uz(ic)])), 0.5F));
        polR = drehe(nurDrehung(w[uz(ia)]) * transponiert(nurDrehung(st.pose[uz(ia)])), polR);
    }
    if (laenge(polR) < 1e-3F) { polR = beuge; }   // auch die Pose gestreckt (T/A: Beine senkrecht)
    const V3 pol = add(pb, mul(norm(polR), laenge(sub(pb, pa)) + 1.0F));
    float bn[3];
    float cn[3];
    const float fa[3] = {pa[0], pa[1], pa[2]};
    const float fb[3] = {pb[0], pb[1], pb[2]};
    const float fc[3] = {pc[0], pc[1], pc[2]};
    const float fz[3] = {ziel[0], ziel[1], ziel[2]};
    const float fp[3] = {pol[0], pol[1], pol[2]};
    (void)zweiGliedIk(fa, fb, fc, fz, fp, bn, cn);
    const V3 nb{bn[0], bn[1], bn[2]};
    const V3 nc{cn[0], cn[1], cn[2]};
    const std::vector<BoneMatrix> vorher = w;
    std::vector<std::uint8_t> ge(w.size(), 0);
    const BoneMatrix r1 = kleinsteDrehung(sub(pb, pa), sub(nb, pa));
    w[uz(ia)] = mitOrt(r1 * nurDrehung(vorher[uz(ia)]), pa);
    const BoneMatrix r2 = kleinsteDrehung(drehe(r1, sub(pc, pb)), sub(nc, nb));
    w[uz(ib)] = mitOrt(r2 * r1 * nurDrehung(vorher[uz(ib)]), nb);
    w[uz(ic)] = mitOrt(nurDrehung(vorher[uz(ic)]), nc);
    ge[uz(ia)] = ge[uz(ib)] = ge[uz(ic)] = 1;
    mitnehmen(st, vorher, w, ge);
}

// Den Teilbaum (nach Ankern) von b in der Welt um den Punkt um drehen (Weltdrehung r) bzw. verschieben
void ankerDrehen(const Zustand& st, std::vector<BoneMatrix>& w, int b, const BoneMatrix& r, const V3& um) {
    if (b < 0) { return; }
    const std::vector<std::uint8_t> drin = ankerTeilbaum(*st.g, *st.c, st.folge, b);
    for (std::size_t d = 0; d < w.size(); ++d) {
        if (drin[d] == 0) { continue; }
        w[d] = mitOrt(r * nurDrehung(w[d]), add(um, drehe(r, sub(ort(w[d]), um)))) * starrInvers(starr(w[d])) * w[d];
    }
}
void ankerSchieben(const Zustand& st, std::vector<BoneMatrix>& w, int b, const V3& weg) {
    if (b < 0) { return; }
    const std::vector<std::uint8_t> drin = ankerTeilbaum(*st.g, *st.c, st.folge, b);
    for (std::size_t d = 0; d < w.size(); ++d) {
        if (drin[d] != 0) {
            for (int k = 0; k < 3; ++k) { w[d].m[k][3] += weg[uz(k)]; }
        }
    }
}

}  // namespace

bool rtUebertragen(const GlaAnimation& gq, const RtCharakter& q, const GlaAnimation& gz, const RtCharakter& z, const RtZuordnung& zu,
                   const RtEinstellung& e, int erstes, int anzahl, std::vector<std::vector<BoneMatrix>>& welt, RtBericht* bericht) {
    RtBericht leer;
    RtBericht& br = bericht != nullptr ? *bericht : leer;
    br = RtBericht{};
    welt.clear();
    if (q.becken < 0 || z.becken < 0) {
        br.fehler = q.becken < 0 ? "source without pelvis" : "target without pelvis";
        return false;
    }
    if (anzahl <= 0 || erstes < 0 || erstes + anzahl > gq.bildZahl()) {
        br.fehler = "frames outside the source";
        return false;
    }
    const std::size_t nz = gz.bones.size();
    Zustand st;
    st.g = &gz;
    st.c = &z;
    st.folge = ankerFolge(gz, z);
    std::vector<BoneMatrix> pq;
    rtPoseWelt(gq, q, e.poseQuelle, pq);
    rtPoseWelt(gz, z, e.poseZiel, st.pose);
    const std::vector<int> mq = rtKnochenZuordnung(gq, q, gz, z, zu);
    for (const int m : mq) { br.knochenMitQuelle += m >= 0 ? 1 : 0; }
    for (const int j : zu.quelle) { br.kettenZugeordnet += j >= 0 ? 1 : 0; }
    br.poseAbweichung = rtPoseAbweichung(gq, q, pq, gz, z, st.pose, zu);
    br.beinQuelle = beinLaenge(q, pq);
    br.beinZiel = beinLaenge(z, st.pose);
    starrMitMass(st);
    for (BoneMatrix& m : pq) { m = starr(m); }
    float k = e.laengeVonHand > 0.0F ? e.laengeVonHand : (br.beinQuelle > 1e-3F ? br.beinZiel / br.beinQuelle : 1.0F);
    if (!e.becken) { k = 1.0F; }
    br.k = k;

    // Quelle: alle Welten vorab (Kontakte brauchen die ganze Folge)
    std::vector<std::vector<BoneMatrix>> wq(uz(anzahl));
    for (int f = 0; f < anzahl; ++f) { gq.worldMatrices(erstes + f, wq[uz(f)]); }

    // IK-Ketten des Ziels (Arm/Bein mit mindestens drei Gliedern) und ihre Quelle
    struct IkKette {
        int a = -1, b = -1, c = -1;     // Ziel: Ober-, Unterglied, Ende
        int qe = -1;                    // Quelle des Endes
        bool bein = false;
        int seite = 0;                  // 0 links, 1 rechts (Kontakte)
        std::vector<int> fussQ;         // Fusspunkte der Quelle
    };
    std::vector<IkKette> ikk;
    for (std::size_t i = 0; i < z.ketten.size() && i < zu.quelle.size(); ++i) {
        const RtKette& kz = z.ketten[i];
        if ((kz.art != KettenArt::Arm && kz.art != KettenArt::Bein) || kz.knochen.size() < 3 || zu.quelle[i] < 0 || uz(zu.quelle[i]) >= q.ketten.size()) { continue; }   // pfix M5
        IkKette ik;
        ik.a = kz.knochen[0];
        ik.b = kz.knochen[1];
        ik.c = kz.knochen[2];
        ik.qe = mq[uz(ik.c)];
        ik.bein = kz.art == KettenArt::Bein;
        ik.seite = kz.seite == Seite::Rechts ? 1 : 0;
        ik.fussQ = fussKnochen(q.ketten[uz(zu.quelle[i])]);
        if (ik.qe >= 0) { ikk.push_back(ik); }
    }
    // Fusskontakte der Quelle (Regel aus Stufe 6) je Beinkette: Bild des Kontaktbeginns und -endes, sonst -1; dazu die
    // gleichmaessige Bewegung des Fusses der Quelle ueber den Kontakt (waagrecht, je Bild). Im Stand bewegt sich der Fuss mit dem
    // Boden: bei einer Animation mit Wurzelbewegung steht er (0), bei einem Gang auf der Stelle (alle Gaenge des Spiels) gleitet
    // der Boden gleichmaessig nach hinten - beides ist "kein Rutschen".
    std::vector<std::vector<int>> kontaktAb(ikk.size(), std::vector<int>(uz(anzahl), -1));
    std::vector<std::vector<int>> kontaktBis(ikk.size(), std::vector<int>(uz(anzahl), -1));
    std::vector<std::vector<V3>> boden(ikk.size(), std::vector<V3>(uz(anzahl), V3{0.0F, 0.0F, 0.0F}));
    {
        for (std::size_t i = 0; i < ikk.size(); ++i) {
            if (!ikk[i].bein) { continue; }
            std::vector<float> tief(uz(anzahl), 1e30F);
            float minimum = 1e30F;
            for (int f = 0; f < anzahl; ++f) {
                for (const int b : ikk[i].fussQ) { tief[uz(f)] = std::min(tief[uz(f)], wq[uz(f)][uz(b)].m[2][3]); }
                minimum = std::min(minimum, tief[uz(f)]);
            }
            int beginn = -1;
            for (int f = 0; f < anzahl; ++f) {
                if (tief[uz(f)] <= minimum + e.kontaktAbstand) {
                    if (beginn < 0) { beginn = f; }
                    kontaktAb[i][uz(f)] = beginn;
                } else {
                    beginn = -1;
                }
            }
            for (int f = anzahl - 1, ende = -1; f >= 0; --f) {
                if (kontaktAb[i][uz(f)] < 0) {
                    ende = -1;
                    continue;
                }
                if (ende < 0) { ende = f; }
                kontaktBis[i][uz(f)] = ende;
                const int a = kontaktAb[i][uz(f)];
                const V3 pa = ort(wq[uz(a)][uz(ikk[i].qe)]);
                const V3 pe = ort(wq[uz(ende)][uz(ikk[i].qe)]);
                boden[i][uz(f)] = ende > a ? mul(V3{pe[0] - pa[0], pe[1] - pa[1], 0.0F}, 1.0F / static_cast<float>(ende - a)) : V3{0.0F, 0.0F, 0.0F};
            }
        }
    }
    // Nach einem Kontakt: der gehaltene Fuss liegt beim Abheben um D = H(fe) - S(fe) neben dem Ort der Quelle (H gehalten, S
    // skaliert); D klingt ueber den Schwung bis zum naechsten Aufsetzen linear ab (dort beginnt wieder S) - ohne Sprung beim
    // Abheben (die Pruefung fand sonst Spruenge bis 1,3 Einheiten je Bild, JK2 BOTH_WALK1 auf die MD-Figur).
    std::vector<std::vector<V3>> nachKontakt(ikk.size(), std::vector<V3>(uz(anzahl), V3{0.0F, 0.0F, 0.0F}));
    for (std::size_t i = 0; i < ikk.size() && e.kontakte; ++i) {
        if (!ikk[i].bein) { continue; }
        const auto quelleSkaliert = [&](int f) { return add(ort(st.pose[uz(ikk[i].c)]), mul(sub(ort(wq[uz(f)][uz(ikk[i].qe)]), ort(pq[uz(ikk[i].qe)])), k)); };
        for (int f = 0; f < anzahl; ++f) {
            if (kontaktAb[i][uz(f)] >= 0) { continue; }
            int fe = f - 1;
            while (fe >= 0 && kontaktAb[i][uz(fe)] < 0) { --fe; }
            if (fe < 0) { continue; }
            int fn = f + 1;
            while (fn < anzahl && kontaktAb[i][uz(fn)] < 0) { ++fn; }
            const int f0 = kontaktAb[i][uz(fe)];
            const V3 h = add(quelleSkaliert(f0), mul(boden[i][uz(fe)], k * static_cast<float>(fe - f0)));
            const V3 sfe = quelleSkaliert(fe);
            const V3 d{h[0] - sfe[0], h[1] - sfe[1], 0.0F};
            const float rest = static_cast<float>(fn - f) / static_cast<float>(fn - fe);   // fn = anzahl: bis zum Ende
            nachKontakt[i][uz(f)] = mul(d, rest);
        }
    }
    std::vector<V3> ikZielAnfang(ikk.size());
    // vorn des Ziels = links x oben (JKA: links +X, vorn -Y), fuer gestreckte Glieder
    V3 vornZ{0.0F, 0.0F, 0.0F};
    {
        int achse = 0;
        float links = 1.0F;
        seitenAchse(gz, achse, links);
        V3 li{0.0F, 0.0F, 0.0F};
        li[uz(achse)] = links;
        vornZ = kreuz(li, V3{0.0F, 0.0F, 1.0F});
    }

    welt.assign(uz(anzahl), std::vector<BoneMatrix>{});
    for (int f = 0; f < anzahl; ++f) {
        const std::vector<BoneMatrix>& xs = wq[uz(f)];
        std::vector<BoneMatrix> w = st.pose;
        const auto skaliert = [&](int zb, int qb) { return add(ort(st.pose[uz(zb)]), mul(sub(ort(xs[uz(qb)]), ort(pq[uz(qb)])), k)); };
        for (const RtSchritt s : e.reihenfolge) {
            if (s == RtSchritt::Becken && e.becken) {
                const std::vector<BoneMatrix> vorher = w;
                std::vector<std::uint8_t> ge(nz, 0);
                const V3 neu = skaliert(z.becken, q.becken);   // pfix N12: e.becken ist hier immer wahr
                w[uz(z.becken)] = mitOrt(nurDrehung(w[uz(z.becken)]), neu);
                ge[uz(z.becken)] = 1;
                mitnehmen(st, vorher, w, ge);
            } else if (s == RtSchritt::Fk && e.fk) {
                for (const int b : st.folge) {
                    const int a = anker(gz, z, b);
                    const int m = mq[uz(b)];
                    BoneMatrix r;
                    if (m >= 0) {
                        r = nurDrehung(xs[uz(m)]) * transponiert(nurDrehung(pq[uz(m)])) * nurDrehung(st.pose[uz(b)]);
                    } else if (a >= 0) {
                        r = nurDrehung(w[uz(a)]) * transponiert(nurDrehung(st.pose[uz(a)])) * nurDrehung(st.pose[uz(b)]);
                    } else {
                        r = nurDrehung(w[uz(b)]);
                    }
                    V3 o = ort(w[uz(b)]);
                    if (a >= 0 && b != z.becken) { o = ort(w[uz(a)] * (starrInvers(st.pose[uz(a)]) * st.pose[uz(b)])); }
                    w[uz(b)] = mitOrt(r, o);
                }
                drehknochen(st, mq, w);
            } else if (s == RtSchritt::Ik && e.ik) {
                for (std::size_t i = 0; i < ikk.size(); ++i) {
                    const IkKette& ik = ikk[i];
                    const float reach = ik.bein ? e.reachFuss : e.reachHand;
                    if (reach <= 0.0F) { continue; }
                    V3 ziel = skaliert(ik.c, ik.qe);
                    if (ik.bein && e.kontakte) {
                        // im Stand gleichmaessig mit dem Boden der Quelle (skaliert), ab dem Ort beim Aufsetzen
                        const int ab = kontaktAb[i][uz(f)];
                        if (ab == f) { ikZielAnfang[i] = ziel; }
                        if (ab >= 0 && ab < f) {
                            const V3 v = mul(boden[i][uz(f)], k * static_cast<float>(f - ab));
                            ziel[0] = ikZielAnfang[i][0] + v[0];
                            ziel[1] = ikZielAnfang[i][1] + v[1];
                        }
                        if (ab < 0) { ziel = add(ziel, nachKontakt[i][uz(f)]); }
                    }
                    ikKette(st, w, ik.a, ik.b, ik.c, ziel, std::min(reach, 1.0F), ik.bein ? vornZ : mul(vornZ, -1.0F));
                }
                drehknochen(st, mq, w);
            } else if (s == RtSchritt::Wurzel && e.wurzel) {
                for (const auto& [zb, qb] : {std::pair<int, int>{z.wurzel, q.wurzel}, std::pair<int, int>{z.bewegung, q.bewegung}}) {
                    if (zb < 0 || qb < 0) { continue; }
                    w[uz(zb)] = mitOrt(nurDrehung(w[uz(zb)]), skaliert(zb, qb));
                }
            }
        }
        mitMass(st, w);
        welt[uz(f)] = std::move(w);
    }
    // Pruefen: Fussrutschen = waagrechte Abweichung des Fusses im Stand von der gleichmaessigen Bewegung des Bodens der Quelle
    // (skaliert), beim Ziel und - als Mass der Quelle selbst - bei der Quelle (mit k)
    br.bilder.assign(uz(anzahl), RtBild{});
    for (std::size_t i = 0; i < ikk.size(); ++i) {
        if (!ikk[i].bein) { continue; }
        const int sd = ikk[i].seite;
        for (int f = 0; f < anzahl; ++f) {
            const int a = kontaktAb[i][uz(f)];
            if (a < 0) { continue; }
            const V3 v = boden[i][uz(f)];
            const float n = static_cast<float>(f - a);
            const V3 dz = sub(ort(welt[uz(f)][uz(ikk[i].c)]), ort(welt[uz(a)][uz(ikk[i].c)]));
            const V3 dq = sub(ort(wq[uz(f)][uz(ikk[i].qe)]), ort(wq[uz(a)][uz(ikk[i].qe)]));
            const float rz = std::hypot(dz[0] - k * v[0] * n, dz[1] - k * v[1] * n);
            const float rq = k * std::hypot(dq[0] - v[0] * n, dq[1] - v[1] * n);
            RtBild& b = br.bilder[uz(f)];
            b.kontakt[uz(sd)] = true;
            b.rutschen[uz(sd)] = std::max(b.rutschen[uz(sd)], rz);
            br.rutschenMax = std::max(br.rutschenMax, rz);
            br.rutschenQuelle = std::max(br.rutschenQuelle, rq);
        }
    }
    br.spruenge = rtSpruenge(wq, welt, mq, k, &br.sprungKnochen);
    for (const int s : br.spruenge) {
        if (uz(s) < br.bilder.size()) { br.bilder[uz(s)].sprung = true; }
    }
    return true;
}

namespace {
// A(b) = B(p) X(p)^-1 X(b) B(b)^-1, Wurzel A = X B^-1 (aus X(b) = X(p) B(p)^-1 A(b) B(b), gla.h)
BoneMatrix lokal(const GlaAnimation& gz, const std::vector<BoneMatrix>& w, std::size_t b) {
    const GlaBone& kb = gz.bones[b];
    BoneMatrix a = w[b] * kb.basePoseInv;
    if (kb.parent >= 0 && uz(kb.parent) < gz.bones.size()) { a = gz.bones[uz(kb.parent)].basePose * starrInvers(w[uz(kb.parent)]) * a; }
    return a;
}
}  // namespace

RigSchicht rtAlsSchicht(const GlaAnimation& gz, const std::vector<std::vector<BoneMatrix>>& welt) {
    RigSchicht s;
    s.bindeBasis = true;
    s.knochen.assign(gz.bones.size(), {});
    for (std::size_t f = 0; f < welt.size(); ++f) {
        if (welt[f].size() != gz.bones.size()) { continue; }
        for (std::size_t b = 0; b < gz.bones.size(); ++b) {
            // Bindebasis: A' = B M B^-1 (rig.h) -> M = B^-1 A B
            const BoneMatrix m = gz.bones[b].basePoseInv * lokal(gz, welt[f], b) * gz.bones[b].basePose;
            RigSchluessel k;
            k.bild = static_cast<int>(f);
            quatAusMatrix(m, k.q);
            k.t[0] = m.m[0][3];
            k.t[1] = m.m[1][3];
            k.t[2] = m.m[2][3];
            k.interp = Interp::Linear;
            s.knochen[b].push_back(k);
        }
    }
    return s;
}

bool rtBacken(const GlaAnimation& gz, const std::vector<std::vector<BoneMatrix>>& welt, const std::string& kopfName, GlaAnimation& aus,
              std::string* fehler) {
    aus = GlaAnimation{};
    const std::size_t nb = gz.bones.size();
    if (nb == 0 || welt.empty()) {
        if (fehler != nullptr) { *fehler = "nothing to bake"; }
        return false;
    }
    aus.name = kopfName;
    aus.skeletonName = kopfName;
    aus.scale = gz.scale;
    aus.bones = gz.bones;
    aus.numFrames = static_cast<int>(welt.size());
    std::unordered_map<std::string, std::uint32_t> vorrat;
    g2::CompressStats st;
    const g2::CompressOptions co{};   // wie rigBacken/xsiAufnehmen (G2C-Vorgabe)
    for (const std::vector<BoneMatrix>& w : welt) {
        if (w.size() != nb) {
            if (fehler != nullptr) { *fehler = "world of another skeleton"; }
            return false;
        }
        for (std::size_t b = 0; b < nb; ++b) {
            const BoneMatrix a = lokal(gz, w, b);
            g2::Mat3x4 m;
            std::memcpy(m.m, a.m, sizeof m.m);
            const g2::fmt::CompQuatBone cb = g2::compressBone(m, co, st);
            const std::string key(reinterpret_cast<const char*>(cb.comp), 14U);
            const auto [it, neu] = vorrat.emplace(key, static_cast<std::uint32_t>(aus.bonePool.size() / 14U));
            if (neu) { aus.bonePool.insert(aus.bonePool.end(), cb.comp, cb.comp + 14); }
            aus.indexes.push_back(it->second);
        }
    }
    if (aus.bonePool.size() / 14U > 0xFFFFFFU) {
        if (fehler != nullptr) { *fehler = "pool larger than a 24-bit index"; }
        return false;
    }
    return true;
}

std::vector<int> rtSpruenge(const std::vector<std::vector<BoneMatrix>>& quelle, const std::vector<std::vector<BoneMatrix>>& ziel,
                            const std::vector<int>& knochenQuelle, float k, std::vector<int>* knochen) {
    std::vector<int> aus;
    if (knochen != nullptr) { knochen->clear(); }
    for (std::size_t f = 1; f < ziel.size() && f < quelle.size(); ++f) {
        bool sprung = false;
        int welcher = -1;
        for (std::size_t b = 0; b < ziel[f].size() && b < knochenQuelle.size() && !sprung; ++b) {
            const int s = knochenQuelle[b];
            if (s < 0) { continue; }
            const float dz = winkelGrad(ziel[f - 1][b], ziel[f][b]);
            const float dq = winkelGrad(quelle[f - 1][uz(s)], quelle[f][uz(s)]);
            const float wz = laenge(sub(ort(ziel[f][b]), ort(ziel[f - 1][b])));
            const float wq = laenge(sub(ort(quelle[f][uz(s)]), ort(quelle[f - 1][uz(s)])));
            sprung = dz > 2.0F * dq + 1.0F || wz > 2.0F * k * wq + 1.0F;
            if (sprung) { welcher = static_cast<int>(b); }
        }
        if (sprung) {
            aus.push_back(static_cast<int>(f));
            if (knochen != nullptr) { knochen->push_back(welcher); }
        }
    }
    return aus;
}

// --- Stufe 5b: Geh-/Laufzyklus ---------------------------------------------------------------------------------------------

float gangZyklus(const GangRegler& r) noexcept { return r.tempo > 1e-3F ? 2.0F * r.schritt / r.tempo : 1.0F; }

namespace {

// Was eine Haltung vorgibt (Regler, kein Spielwert): Beckenhoehe in Beinlaengen ueber dem gestreckten Stand, Vorlage (Grad),
// Wippen und Hub des Schwungfusses (Beinlaengen), Armschwung und Ellbogen (Grad), Spurbreite, Standphase links/rechts,
// seitliche Neigung (Grad)
struct HaltungsWerte {
    float hoehe, vorlage, wippen, hub, arm, ellbogen, spur, standL, standR, seitlich;
};
HaltungsWerte haltungsWerte(const GangRegler& r) {
    const bool lauf = r.art == GangArt::Laufen;
    // Standphase: Gehen 60 % (Perry/Burnfield), Laufen 35 % (Novacheck: 39 % beim Joggen, kuerzer beim Sprint)
    HaltungsWerte h{lauf ? 0.95F : 0.97F, lauf ? 8.0F : 3.0F, lauf ? 0.04F : 0.02F, lauf ? 0.22F : 0.08F, lauf ? 30.0F : 15.0F,
                    lauf ? 85.0F : 15.0F, 0.0F, lauf ? 0.35F : 0.60F, lauf ? 0.35F : 0.60F, 0.0F};
    switch (r.haltung) {
        case GangHaltung::Normal: break;
        case GangHaltung::Schwer:   // tief, breit, mehr Wippen, Arme weg vom Koerper
            h.hoehe -= 0.05F;
            h.vorlage += 3.0F;
            h.wippen *= 1.6F;
            h.spur = 0.12F;
            h.arm *= 0.6F;
            h.seitlich = 12.0F;
            break;
        case GangHaltung::Schleichend:   // geduckt, vorgebeugt, Fuss kaum gehoben, Arme vorn
            h.hoehe -= 0.17F;
            h.vorlage += 15.0F;
            h.wippen *= 0.5F;
            h.hub *= 0.6F;
            h.arm *= 0.4F;
            h.ellbogen = 60.0F;
            break;
        case GangHaltung::Verletzt:   // rechts kurz belastet (Hinken), zur gesunden Seite geneigt
            h.standR = h.standL - 0.15F;
            h.wippen *= 1.4F;
            h.arm *= 0.5F;
            h.seitlich = 6.0F;
            break;
        case GangHaltung::Saebel:   // leicht geduckt, rechte Hand vorn am Griff, linker Arm schwingt halb
            h.hoehe -= 0.04F;
            h.vorlage += 4.0F;
            h.arm *= 0.5F;
            break;
    }
    return h;
}

float glatt(float u) { return 0.5F - 0.5F * std::cos(kPi * std::clamp(u, 0.0F, 1.0F)); }

}  // namespace

bool rtGangBild(const GlaAnimation& g, const RtCharakter& c, const GangRegler& r, float phase, std::vector<BoneMatrix>& welt, std::string* fehler) {
    const RtKette* bein[2] = {nullptr, nullptr};
    const RtKette* arm[2] = {nullptr, nullptr};
    const RtKette* wirbel = nullptr;
    for (const RtKette& k : c.ketten) {
        const int s = k.seite == Seite::Links ? 0 : (k.seite == Seite::Rechts ? 1 : -1);
        if (k.art == KettenArt::Bein && s >= 0 && k.nummer == 0 && k.knochen.size() >= 3) { bein[s] = &k; }
        if (k.art == KettenArt::Arm && s >= 0 && k.nummer == 0 && k.knochen.size() >= 2) { arm[s] = &k; }
        if (k.art == KettenArt::Wirbel && k.nummer == 0 && !k.knochen.empty()) { wirbel = &k; }
    }
    if (c.becken < 0 || bein[0] == nullptr || bein[1] == nullptr) {
        if (fehler != nullptr) { *fehler = "no pelvis and two legs"; }
        return false;
    }
    Zustand st;
    st.g = &g;
    st.c = &c;
    st.folge = ankerFolge(g, c);
    RtPose p;
    p.haltung = RtHaltung::A;
    rtPoseWelt(g, c, p, st.pose);
    starrMitMass(st);
    std::vector<BoneMatrix>& w = welt;
    w = st.pose;
    // Achsen: oben Z, links aus der Erkennung, vorn = links x oben (JKA: links +X, vorn -Y)
    int achse = 0;
    float links = 1.0F;
    seitenAchse(g, achse, links);
    V3 li{0.0F, 0.0F, 0.0F};
    li[uz(achse)] = links;
    const V3 oben{0.0F, 0.0F, 1.0F};
    const V3 vorn = kreuz(li, oben);
    const HaltungsWerte h = haltungsWerte(r);
    const float bein0 = ort(st.pose[uz(bein[0]->knochen[0])])[2] - ort(st.pose[uz(bein[0]->knochen[2])])[2];
    const float schritt = r.schritt;
    const float ph = phase - std::floor(phase);
    const float zwei = 2.0F * kPi;
    const float grad = kPi / 180.0F;
    // Becken: Hoehe, Wippen (zweimal je Zyklus; Gehen tief im Doppelstand, Laufen hoch im Flug), seitlich zum Standbein
    const float wippen = std::cos(2.0F * zwei * ph);
    const float dz = -(1.0F - h.hoehe) * bein0 + (r.art == GangArt::Laufen ? -1.0F : 1.0F) * h.wippen * bein0 * 0.5F * wippen;
    const V3 dSeite = mul(li, 0.02F * bein0 * std::cos(zwei * ph));
    // die Ziele der Fuesse (Stand flach am Boden der Pose und gleitend, Schwung vorwaerts und hoch)
    V3 ziele[2];
    bool imStand[2] = {false, false};
    for (int s = 0; s < 2; ++s) {
        const RtKette& b = *bein[s];
        const float stand = s == 0 ? h.standL : h.standR;
        const float lp = ph + (s == 0 ? 0.0F : 0.5F);
        const float u = lp - std::floor(lp);
        // Weg des Fusses im Stand: er gleitet mit dem Tempo (das Spiel bewegt die Figur, der Fuss steht dann in der Welt) ueber die
        // Standzeit stand * zyklus: weg = tempo * stand * zyklus = 2 * stand * schritt
        const float weg = 2.0F * stand * schritt;
        float f = 0.0F;
        float hoch = 0.0F;
        if (u < stand) {
            f = weg * 0.5F - weg * (u / stand);
            imStand[s] = true;
        } else {
            const float v = (u - stand) / (1.0F - stand);
            f = -weg * 0.5F + weg * glatt(v);
            hoch = h.hub * r.hub * bein0 * std::sin(kPi * v);
        }
        const V3 knoechel = ort(st.pose[uz(b.knochen[2])]);
        const float spur = (s == 0 ? 1.0F : -1.0F) * h.spur * bein0;
        ziele[s] = add(add(add(knoechel, mul(vorn, f)), mul(li, spur)), V3{0.0F, 0.0F, hoch});
    }
    ankerSchieben(st, w, c.becken, add(V3{0.0F, 0.0F, dz}, dSeite));
    // Drehen (+-4 Grad) und Kippen (+-4 Grad) wie Perry/Burnfield, mit dem Regler
    const V3 pb = ort(w[uz(c.becken)]);
    const BoneMatrix dreh = achsDrehung(oben, -4.0F * r.hueftschwung * std::sin(zwei * ph) * grad);
    const BoneMatrix kipp = achsDrehung(vorn, 4.0F * r.hueftschwung * std::cos(zwei * ph) * grad);
    ankerDrehen(st, w, c.becken, kipp * dreh, pb);
    // das Becken nur so hoch, dass jeder Standfuss erreichbar bleibt (das Knie um 0,5 % gebeugt, damit die Ebene der IK bestimmt
    // ist): Huefte hoechstens sqrt((0,995 L)^2 - waagrecht^2) ueber dem Ziel - daraus das Wippen des umgekehrten Pendels
    float tiefer = 0.0F;
    for (int s = 0; s < 2; ++s) {
        if (!imStand[s]) { continue; }
        const RtKette& b = *bein[s];
        const float lang = 0.995F * (laenge(sub(ort(st.pose[uz(b.knochen[1])]), ort(st.pose[uz(b.knochen[0])]))) +
                                      laenge(sub(ort(st.pose[uz(b.knochen[2])]), ort(st.pose[uz(b.knochen[1])]))));
        const V3 huefte = ort(w[uz(b.knochen[0])]);
        const float waag = std::hypot(huefte[0] - ziele[s][0], huefte[1] - ziele[s][1]);
        if (waag < lang) { tiefer = std::min(tiefer, ziele[s][2] + std::sqrt(lang * lang - waag * waag) - huefte[2]); }
    }
    ankerSchieben(st, w, c.becken, V3{0.0F, 0.0F, tiefer});
    // Oberkoerper: nach vorn geneigt, seitlich (Hinken, schwer), der Brustkorb gegen das Becken gedreht
    if (wirbel != nullptr) {
        const int w0 = wirbel->knochen.front();
        const int w1 = wirbel->knochen.back();
        ankerDrehen(st, w, w0, achsDrehung(li, h.vorlage * grad) * achsDrehung(vorn, h.seitlich * 0.3F * grad), ort(w[uz(w0)]));
        ankerDrehen(st, w, w1, achsDrehung(oben, 8.0F * r.hueftschwung * std::sin(zwei * ph) * grad), ort(w[uz(w1)]));
    }
    // Beine auf ihre Ziele (Knie nach vorn)
    for (int s = 0; s < 2; ++s) { ikKette(st, w, bein[s]->knochen[0], bein[s]->knochen[1], bein[s]->knochen[2], ziele[s], 1.0F, vorn); }
    // Arme gegengleich (Schulter um die Seitenachse), Ellbogen gebeugt; Saebel: rechte Hand vorn am Griff
    for (int s = 0; s < 2; ++s) {
        if (arm[s] == nullptr) { continue; }
        const RtKette& a = *arm[s];
        const bool saebel = r.haltung == GangHaltung::Saebel && s == 1;
        const float schwung = saebel ? 30.0F : h.arm * r.armschwung * std::sin(zwei * (ph + (s == 0 ? 0.5F : 0.0F)));
        // vom 45-Grad-A der Pose naeher an den Koerper (um die Vorwaertsachse), dann schwingen (um die Seitenachse)
        const float anlegen = ((saebel ? 20.0F : 30.0F) - h.seitlich) * (s == 0 ? -1.0F : 1.0F) * links;
        const V3 schulter = ort(w[uz(a.knochen[0])]);
        ankerDrehen(st, w, a.knochen[0], achsDrehung(li, -schwung * grad) * achsDrehung(vorn, anlegen * grad), schulter);
        const int e = a.knochen[1];
        ankerDrehen(st, w, e, achsDrehung(li, -(saebel ? 60.0F : h.ellbogen) * grad), ort(w[uz(e)]));
    }
    drehknochen(st, std::vector<int>(g.bones.size(), -1), w);
    mitMass(st, w);
    return true;
}

bool rtGang(const GlaAnimation& g, const RtCharakter& c, const GangRegler& r, int fps, std::vector<std::vector<BoneMatrix>>& welt, std::string* fehler) {
    welt.clear();
    const int n = std::max(2, static_cast<int>(std::lround(gangZyklus(r) * static_cast<float>(std::max(fps, 1)))));
    welt.resize(uz(n));
    for (int i = 0; i < n; ++i) {
        if (!rtGangBild(g, c, r, static_cast<float>(i) / static_cast<float>(n), welt[uz(i)], fehler)) {
            welt.clear();
            return false;
        }
    }
    return true;
}

GangBefund rtGangPruefen(const GlaAnimation& g, const RtCharakter& c, const GangRegler& r, int fps) {
    GangBefund b;
    std::vector<std::vector<BoneMatrix>> w;
    if (!rtGang(g, c, r, fps, w)) { return b; }
    std::vector<BoneMatrix> null;
    std::vector<BoneMatrix> eins;
    (void)rtGangBild(g, c, r, 0.0F, null);
    (void)rtGangBild(g, c, r, 1.0F, eins);
    for (std::size_t k = 0; k < null.size(); ++k) { b.naht = std::max(b.naht, laenge(sub(ort(null[k]), ort(eins[k])))); }
    // Stand: der Knoechel auf der Hoehe der Pose, gleitet je Bild um schritt / (stand * n)
    std::vector<BoneMatrix> pose;
    RtPose p;
    p.haltung = RtHaltung::A;
    rtPoseWelt(g, c, p, pose);
    const HaltungsWerte h = haltungsWerte(r);
    const int n = static_cast<int>(w.size());
    int achse = 0;
    float links = 1.0F;
    seitenAchse(g, achse, links);
    V3 li{0.0F, 0.0F, 0.0F};
    li[uz(achse)] = links;
    const V3 vorn = kreuz(li, V3{0.0F, 0.0F, 1.0F});
    for (const RtKette& k : c.ketten) {
        if (k.art != KettenArt::Bein || k.nummer != 0 || k.knochen.size() < 3 || k.seite == Seite::Mitte) { continue; }
        const int si = k.seite == Seite::Rechts ? 1 : 0;
        const float stand = si == 0 ? h.standL : h.standR;
        const float soll = 2.0F * stand * r.schritt / (stand * static_cast<float>(n));   // Weg im Stand / Bilder im Stand = Tempo je Bild
        const float z0 = ort(pose[uz(k.knochen[2])])[2];
        for (int f = 0; f + 1 < n; ++f) {
            const float u0 = std::fmod(static_cast<float>(f) / static_cast<float>(n) + (si == 0 ? 0.0F : 0.5F), 1.0F);
            const float u1 = u0 + 1.0F / static_cast<float>(n);
            if (u1 >= stand) { continue; }
            ++b.standBilder;
            const V3 a = ort(w[uz(f)][uz(k.knochen[2])]);
            const V3 e = ort(w[uz(f + 1)][uz(k.knochen[2])]);
            b.bodenAbw = std::max(b.bodenAbw, std::fabs(a[2] - z0));
            b.gleitAbw = std::max(b.gleitAbw, std::fabs(-dot(sub(e, a), vorn) - soll));
        }
    }
    return b;
}

bool gangMessen(const GlaAnimation& g, const RtCharakter& c, int erstes, int anzahl, double bildMs, float& tempo, float& schritt) {
    if (anzahl < 2 || erstes < 0 || erstes + anzahl > g.bildZahl() || bildMs <= 0.0) { return false; }
    std::vector<std::vector<BoneMatrix>> w(uz(anzahl));
    for (int f = 0; f < anzahl; ++f) { g.worldMatrices(erstes + f, w[uz(f)]); }
    // Kontakte wie rtUebertragen (Regel aus Stufe 6, kontaktAbstand der Vorgabe); im Stand gleitet der Knoechel der Gaenge des
    // Spiels (auf der Stelle) mit dem Tempo nach hinten - gemessen der Mittelwert des waagrechten Wegs je Bild
    const float kontakt = RtEinstellung{}.kontaktAbstand;
    double weg = 0.0;
    int bilder = 0;
    int zyklen = 0;
    for (const RtKette& k : c.ketten) {
        if (k.art != KettenArt::Bein || k.nummer != 0 || k.knochen.size() < 3 || k.seite == Seite::Mitte) { continue; }
        const std::vector<int> fp = fussKnochen(k);
        std::vector<float> tief(uz(anzahl), 1e30F);
        float minimum = 1e30F;
        for (int f = 0; f < anzahl; ++f) {
            for (const int b : fp) { tief[uz(f)] = std::min(tief[uz(f)], w[uz(f)][uz(b)].m[2][3]); }
            minimum = std::min(minimum, tief[uz(f)]);
        }
        bool vorher = false;
        for (int f = 0; f < anzahl; ++f) {
            const bool k0 = tief[uz(f)] <= minimum + kontakt;
            if (k.seite == Seite::Links && k0 && !vorher) { ++zyklen; }
            vorher = k0;
        }
        for (int f = 0; f + 1 < anzahl; ++f) {
            if (tief[uz(f)] > minimum + kontakt || tief[uz(f + 1)] > minimum + kontakt) { continue; }
            const V3 a = ort(w[uz(f)][uz(k.knochen[2])]);
            const V3 e = ort(w[uz(f + 1)][uz(k.knochen[2])]);
            weg += std::hypot(e[0] - a[0], e[1] - a[1]);
            ++bilder;
        }
    }
    if (bilder == 0) { return false; }
    tempo = static_cast<float>(weg / bilder / (bildMs / 1000.0));
    // ein Doppelschritt je Kontakt des linken Fusses (ein Kontakt, der ueber das Ende in den Anfang laeuft, zaehlt einmal)
    schritt = tempo * static_cast<float>(anzahl * bildMs / 1000.0) * 0.5F / static_cast<float>(std::max(zyklen, 1));
    return true;
}

}  // namespace bhed::modell
