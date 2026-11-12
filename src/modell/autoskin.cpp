// modell/autoskin.cpp - Auto-Skin (siehe bhed/modell/autoskin.h)
#include "bhed/modell/autoskin.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <functional>
#include <map>
#include <queue>
#include <unordered_map>
#include <utility>

#include "bhed/arbeiter.h"
#include "bhed/modell/gewichte.h"

namespace bhed::modell {
namespace {

using V3 = std::array<double, 3>;

V3 minus(const V3& a, const V3& b) noexcept { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
V3 plus(const V3& a, const V3& b) noexcept { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
V3 mal(const V3& a, double s) noexcept { return {a[0] * s, a[1] * s, a[2] * s}; }
double punkt(const V3& a, const V3& b) noexcept { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
V3 kreuz(const V3& a, const V3& b) noexcept { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
double laenge(const V3& a) noexcept { return std::sqrt(punkt(a, a)); }
V3 ortVon(const Ecke& e) noexcept { return {static_cast<double>(e.ort[0]), static_cast<double>(e.ort[1]), static_cast<double>(e.ort[2])}; }

// Der naechste Punkt der Strecke a-b zu p.
V3 naechsterAufStrecke(const V3& p, const V3& a, const V3& b) noexcept {
    const V3 ab = minus(b, a);
    const double l2 = punkt(ab, ab);
    if (l2 <= 0.0) { return a; }
    const double t = std::clamp(punkt(minus(p, a), ab) / l2, 0.0, 1.0);
    return plus(a, mal(ab, t));
}

// Der gerade Abstand eines Punkts zur Form eines Knochens; `naechster` der Punkt der Form dazu.
double abstandForm(const KnochenForm& f, const V3& p, V3* naechster = nullptr) noexcept {
    double best = std::numeric_limits<double>::max();
    for (const std::array<double, 6>& s : f.strecken) {
        const V3 q = naechsterAufStrecke(p, V3{s[0], s[1], s[2]}, V3{s[3], s[4], s[5]});
        const double d = laenge(minus(p, q));
        if (d < best) {
            best = d;
            if (naechster != nullptr) { *naechster = q; }
        }
    }
    return best;
}

bool waehlbar(const Teil& t) noexcept { return !t.istTag() && !t.stufen.empty(); }

// --- Knochenformen ---------------------------------------------------------------------------------------------------

// Rang in der Kette einer Gliedmasse (wie rig.cpp `rang`): Schluesselbein < Oberarm < Drehknochen < Unterarm <
// Drehknochen < Hand < Finger; Oberschenkel < Drehknochen < Unterschenkel < Fuss < Zehen. -1: gehoert zu keiner.
double kettenRang(Rolle r) noexcept {
    switch (r) {
        case Rolle::Schluessel: return 0.0;
        case Rolle::Oberarm: case Rolle::Oberschenkel: return 1.0;
        case Rolle::OberarmTwist: case Rolle::OberschenkelTwist: return 1.5;
        case Rolle::Unterarm: case Rolle::Unterschenkel: return 2.0;
        case Rolle::UnterarmTwist: return 2.5;
        case Rolle::Hand: case Rolle::Fuss: return 3.0;
        case Rolle::Finger: case Rolle::Zehen: return 4.0;
        default: return -1.0;
    }
}
int kettenArt(Rolle r) noexcept { return armRolle(r) ? 1 : beinRolle(r) ? 2 : 0; }
bool drehRolle(Rolle r) noexcept { return r == Rolle::OberarmTwist || r == Rolle::UnterarmTwist || r == Rolle::OberschenkelTwist; }

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Zaehlnamen: der Name endet auf eine Zahl n >= 1, und ein Geschwister heisst gleich mit n - 1 (r_d1_j2 -> r_d1_j1).
int zaehlVorgaenger(const GlaAnimation& g, int b, const std::vector<KnochenForm>& f) {
    const std::string name = klein(g.bones[static_cast<std::size_t>(b)].name);
    std::size_t ziffern = 0;
    while (ziffern < name.size() && std::isdigit(static_cast<unsigned char>(name[name.size() - 1U - ziffern])) != 0) { ++ziffern; }
    if (ziffern == 0U || ziffern > 6U || ziffern == name.size()) { return -1; }
    const std::string stamm = name.substr(0, name.size() - ziffern);
    const int n = std::atoi(name.c_str() + (name.size() - ziffern));
    if (n < 1) { return -1; }
    const std::string kurz = stamm + std::to_string(n - 1);
    std::string breit = std::to_string(n - 1);
    while (breit.size() < ziffern) { breit.insert(breit.begin(), '0'); }
    breit = stamm + breit;
    const int eltern = g.bones[static_cast<std::size_t>(b)].parent;
    for (std::size_t c = 0; c < g.bones.size(); ++c) {
        if (static_cast<int>(c) == b || g.bones[c].parent != eltern || !f[c].traegt) { continue; }
        const std::string nc = klein(g.bones[c].name);
        if (nc == kurz || nc == breit) { return static_cast<int>(c); }
    }
    return -1;
}

}  // namespace

double autoSkinExponent(float steifigkeit) noexcept {
    // Linear von 2 (weich: bei doppeltem Abstand ein Viertel) bis 32 (fast starr: bei 10 % mehr Abstand noch 5 %). Die
    // Vorgabe 0,3 ist Exponent 11. Gemessen (modellautoskintest <JA base> --suche; kyle, luke, stormtrooper gegen Ravens
    // Gewichte, Glaetten 2): mittlere Verformungsabweichung 0,398 Einheiten bei 0,05, 0,340 bei 0,1, 0,293 bei 0,2, 0,278
    // bei 0,3, 0,272 bei 0,4, 0,268 bei 0,65 - ab 0,3 gewinnt das Mittel kaum noch, der groesste Fehler steigt aber weiter
    // (6,09 / 6,23 / 6,52 / 6,82 / 7,09 / 7,58).
    return 2.0 + 30.0 * static_cast<double>(std::clamp(steifigkeit, 0.0F, 1.0F));
}

std::vector<KnochenForm> knochenFormen(const GlaAnimation& g, const Erkennung& e, const AutoSkinOptionen& o, const Inhalt& in) {
    const std::size_t n = g.bones.size();
    std::vector<KnochenForm> f(n);
    const auto rolle = [&](std::size_t b) { return b < e.knochen.size() ? e.knochen[b].rolle : Rolle::Keine; };
    const auto seite = [&](std::size_t b) { return b < e.knochen.size() ? e.knochen[b].seite : Seite::Mitte; };
    const auto eltern = [&](std::size_t b) {
        const int p = g.bones[b].parent;
        return p >= 0 && static_cast<std::size_t>(p) < n && static_cast<std::size_t>(p) != b ? p : -1;
    };
    for (std::size_t b = 0; b < n; ++b) {
        const BoneMatrix& m = g.bones[b].basePose;
        f[b].ursprung = {static_cast<double>(m.m[0][3]), static_cast<double>(m.m[1][3]), static_cast<double>(m.m[2][3])};
    }
    std::vector<char> hatKind(n, 0);
    for (std::size_t b = 0; b < n; ++b) {
        if (eltern(b) >= 0) { hatKind[static_cast<std::size_t>(eltern(b))] = 1; }
    }
    const bool eigene = o.knochen.size() == n;
    for (std::size_t b = 0; b < n; ++b) {
        const Rolle r = rolle(b);
        bool t = false;
        if (eigene) {
            t = o.knochen[b] != 0;
        } else {
            t = r != Rolle::Wurzel && r != Rolle::Bewegung && r != Rolle::TagKnochen;
            if (!o.detailknochen && (r == Rolle::Gesicht || r == Rolle::Anhang)) { t = false; }
            // ein Gruppenknoten: genau im Ursprung seines Elternknochens und mit eigenen Kindern (face unter cranium)
            const int p = eltern(b);
            if (t && p >= 0 && hatKind[b] != 0 && f[b].ursprung == f[static_cast<std::size_t>(p)].ursprung) { t = false; }
        }
        if (!o.drehknochen && drehRolle(r)) { t = false; }
        f[b].traegt = t;
    }
    // Formeltern: das Skelett, ausser bei Ketten, die flach nebeneinander haengen
    std::vector<int> fe(n, -1);
    for (std::size_t b = 0; b < n; ++b) { fe[b] = eltern(b); }
    for (std::size_t b = 0; b < n; ++b) {
        if (!f[b].traegt) { continue; }
        const int z = zaehlVorgaenger(g, static_cast<int>(b), f);
        if (z >= 0) {
            fe[b] = z;
            continue;
        }
        const int art = kettenArt(rolle(b));
        if (art == 0) { continue; }
        const double rb = kettenRang(rolle(b));
        const int p = eltern(b);
        // Geschwister derselben Gliedmasse, die in der Kette vor b liegen und hinter dem Elternknochen im Skelett: das
        // letzte davon (bei gleichem Rang das naechste) ist der Formelternknochen
        double elternRang = -1.0;
        if (p >= 0 && kettenArt(rolle(static_cast<std::size_t>(p))) == art && seite(static_cast<std::size_t>(p)) == seite(b)) {
            elternRang = kettenRang(rolle(static_cast<std::size_t>(p)));
        }
        int best = -1;
        double bestRang = -1.0;
        double bestAbstand = 0.0;
        for (std::size_t c = 0; c < n; ++c) {
            if (c == b || eltern(c) != p || !f[c].traegt || kettenArt(rolle(c)) != art || seite(c) != seite(b)) { continue; }
            const double rc = kettenRang(rolle(c));
            if (rc >= rb || rc <= elternRang) { continue; }
            const double d = laenge(minus(f[c].ursprung, f[b].ursprung));
            if (best < 0 || rc > bestRang || (rc == bestRang && d < bestAbstand)) {
                best = static_cast<int>(c);
                bestRang = rc;
                bestAbstand = d;
            }
        }
        if (best >= 0) { fe[b] = best; }
    }
    // Knochen, die nichts tragen, sind durchsichtig: ihre Kinder haengen am naechsten tragenden Vorfahren
    for (std::size_t b = 0; b < n; ++b) {
        if (!f[b].traegt) { continue; }
        int x = fe[b];
        std::size_t schritte = 0;
        while (x >= 0 && !f[static_cast<std::size_t>(x)].traegt && schritte++ <= n) { x = fe[static_cast<std::size_t>(x)]; }
        f[b].formEltern = schritte > n ? -1 : x;
    }
    for (std::size_t b = 0; b < n; ++b) {
        const int p = f[b].formEltern;
        if (!f[b].traegt || p < 0) { continue; }
        const V3& a = f[static_cast<std::size_t>(p)].ursprung;
        const V3& z = f[b].ursprung;
        if (a == z) { continue; }
        f[static_cast<std::size_t>(p)].strecken.push_back({a[0], a[1], a[2], z[0], z[1], z[2]});
    }
    std::vector<std::size_t> blaetter;
    for (std::size_t b = 0; b < n; ++b) {
        if (!f[b].traegt || !f[b].strecken.empty()) { continue; }
        f[b].blatt = true;
        const V3& a = f[b].ursprung;
        f[b].strecken.push_back({a[0], a[1], a[2], a[0], a[1], a[2]});
        blaetter.push_back(b);
    }
    if (blaetter.empty()) { return f; }
    // Blattknochen: Strecke durch den flaechengewichteten Schwerpunkt ihrer Zelle. Bei Gleichstand mit einem anderen
    // Knochen (jenseits des Gelenks ist der Blattknochen genau so weit wie das Ende seines Vorgaengers) zaehlt der Punkt
    // zum Blatt; die Toleranz deckt die Rundung der Abstandsrechnung (double, Koordinaten bis 64 * 1024).
    std::vector<V3> summe(n, V3{0.0, 0.0, 0.0});
    std::vector<double> flaeche(n, 0.0);
    std::vector<int> zahl(n, 0);
    for (const Teil& t : in.teile) {
        if (!waehlbar(t)) { continue; }
        const Netz& netz = t.stufen[0];
        const std::vector<std::uint32_t> idx = netz.indizes();
        std::vector<double> eckFlaeche(netz.ecken.size(), 0.0);
        for (std::size_t k = 0; k + 2U < idx.size(); k += 3U) {
            if (idx[k] >= netz.ecken.size() || idx[k + 1U] >= netz.ecken.size() || idx[k + 2U] >= netz.ecken.size()) { continue; }
            const V3 a = ortVon(netz.ecken[idx[k]]);
            const double fl = 0.5 * laenge(kreuz(minus(ortVon(netz.ecken[idx[k + 1U]]), a), minus(ortVon(netz.ecken[idx[k + 2U]]), a)));
            for (std::size_t j = 0; j < 3U; ++j) { eckFlaeche[idx[k + j]] += fl / 3.0; }
        }
        for (std::size_t v = 0; v < netz.ecken.size(); ++v) {
            if (!(eckFlaeche[v] > 0.0)) { continue; }
            const V3 p = ortVon(netz.ecken[v]);
            double dmin = std::numeric_limits<double>::max();
            for (std::size_t b = 0; b < n; ++b) {
                if (f[b].traegt) { dmin = std::min(dmin, abstandForm(f[b], p)); }
            }
            const double grenze = dmin * (1.0 + 1e-9) + 1e-9;
            for (const std::size_t b : blaetter) {
                if (abstandForm(f[b], p) > grenze) { continue; }
                summe[b] = plus(summe[b], mal(p, eckFlaeche[v]));
                flaeche[b] += eckFlaeche[v];
                ++zahl[b];
                break;
            }
        }
    }
    for (const std::size_t b : blaetter) {
        if (zahl[b] < 3 || !(flaeche[b] > 0.0)) { continue; }
        const V3& a = f[b].ursprung;
        const V3 ende = plus(a, mal(minus(mal(summe[b], 1.0 / flaeche[b]), a), 2.0));
        if (ende == a) { continue; }
        f[b].strecken[0] = {a[0], a[1], a[2], ende[0], ende[1], ende[2]};
    }
    return f;
}

namespace {

// --- Punkte: Ecken gleichen Orts ------------------------------------------------------------------------------------

struct OrtSchluessel {
    std::int32_t stufe = 0;
    std::int32_t teil = 0;   // -1: ueber Flaechen hinweg
    std::uint32_t x = 0, y = 0, z = 0;
    bool operator==(const OrtSchluessel& o) const noexcept { return stufe == o.stufe && teil == o.teil && x == o.x && y == o.y && z == o.z; }
};
struct OrtHash {
    std::size_t operator()(const OrtSchluessel& k) const noexcept {
        std::uint64_t h = 1469598103934665603ULL;
        for (const std::uint32_t v : {static_cast<std::uint32_t>(k.stufe), static_cast<std::uint32_t>(k.teil), k.x, k.y, k.z}) {
            h = (h ^ v) * 1099511628211ULL;
        }
        return static_cast<std::size_t>(h);
    }
};
std::uint32_t bits(float f) noexcept {
    f += 0.0F;   // -0 wird +0
    std::uint32_t u = 0;
    std::memcpy(&u, &f, sizeof u);
    return u;
}

struct Sammlung {
    std::vector<char> gewaehlt;                             // je Teil
    std::vector<std::vector<std::vector<int>>> punktVon;    // [teil][stufe][ecke]
    std::vector<V3> ort;                                    // je Punkt
    std::vector<std::vector<int>> nachbarn;                 // je Punkt
    std::vector<std::vector<int>> teileVon;                 // je Punkt die Teile, in denen er vorkommt
    int ecken = 0;
};

Sammlung sammeln(const Inhalt& in, const AutoSkinOptionen& o) {
    Sammlung s;
    s.gewaehlt.assign(in.teile.size(), 0);
    s.punktVon.resize(in.teile.size());
    std::unordered_map<OrtSchluessel, int, OrtHash> karte;
    for (std::size_t t = 0; t < in.teile.size(); ++t) {
        const Teil& teil = in.teile[t];
        if (!waehlbar(teil) || (o.teile.size() == in.teile.size() && o.teile[t] == 0)) { continue; }
        s.gewaehlt[t] = 1;
        s.punktVon[t].resize(teil.stufen.size());
        for (std::size_t st = 0; st < teil.stufen.size(); ++st) {
            const Netz& netz = teil.stufen[st];
            std::vector<int>& pv = s.punktVon[t][st];
            pv.resize(netz.ecken.size());
            for (std::size_t v = 0; v < netz.ecken.size(); ++v) {
                const Ecke& e = netz.ecken[v];
                const OrtSchluessel k{static_cast<std::int32_t>(st), o.naehte ? -1 : static_cast<std::int32_t>(t), bits(e.ort[0]), bits(e.ort[1]),
                                      bits(e.ort[2])};
                const auto [it, neu] = karte.try_emplace(k, static_cast<int>(s.ort.size()));
                if (neu) {
                    s.ort.push_back(ortVon(e));
                    s.nachbarn.emplace_back();
                    s.teileVon.emplace_back();
                }
                pv[v] = it->second;
                std::vector<int>& tv = s.teileVon[static_cast<std::size_t>(it->second)];
                if (tv.empty() || tv.back() != static_cast<int>(t)) { tv.push_back(static_cast<int>(t)); }
                ++s.ecken;
            }
            const std::vector<std::uint32_t> idx = netz.indizes();
            for (std::size_t k = 0; k + 2U < idx.size(); k += 3U) {
                if (idx[k] >= pv.size() || idx[k + 1U] >= pv.size() || idx[k + 2U] >= pv.size()) { continue; }
                const int p[3] = {pv[idx[k]], pv[idx[k + 1U]], pv[idx[k + 2U]]};
                for (int a = 0; a < 3; ++a) {
                    for (int b = 0; b < 3; ++b) {
                        if (p[a] != p[b]) { s.nachbarn[static_cast<std::size_t>(p[a])].push_back(p[b]); }
                    }
                }
            }
        }
    }
    for (std::vector<int>& nb : s.nachbarn) {
        std::sort(nb.begin(), nb.end());
        nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
    }
    return s;
}

// --- Voxelgitter -----------------------------------------------------------------------------------------------------

// Mehr Voxel als das legt das Gitter nicht an (je Voxel 5 Byte): die Voxel werden dann groesser.
constexpr double kVoxelHoechstens = 48.0e6;
constexpr int kRand = 2;   // leere Voxel um das Netz, damit die Flutfuellung aussen herumkommt

struct Gitter {
    V3 min{};
    double h = 0.0;
    int n[3]{};
    std::vector<std::uint8_t> art;      // 0 aussen, 1 Oberflaeche, 2 innen
    std::vector<std::int32_t> fest;     // je Voxel sein Index unter den festen, -1 aussen
    std::vector<std::int32_t> zelle;    // je festem Voxel sein Index im Gitter
    int innen = 0;

    [[nodiscard]] bool da() const noexcept { return h > 0.0; }
    [[nodiscard]] std::size_t index(int i, int j, int k) const noexcept {
        return (static_cast<std::size_t>(k) * static_cast<std::size_t>(n[1]) + static_cast<std::size_t>(j)) * static_cast<std::size_t>(n[0]) +
               static_cast<std::size_t>(i);
    }
    [[nodiscard]] bool drin(int i, int j, int k) const noexcept { return i >= 0 && j >= 0 && k >= 0 && i < n[0] && j < n[1] && k < n[2]; }
    void zelleVon(const V3& p, int ijk[3]) const noexcept {
        for (int a = 0; a < 3; ++a) { ijk[a] = static_cast<int>(std::floor((p[static_cast<std::size_t>(a)] - min[static_cast<std::size_t>(a)]) / h)); }
    }
    [[nodiscard]] V3 mitte(int i, int j, int k) const noexcept { return {min[0] + (i + 0.5) * h, min[1] + (j + 0.5) * h, min[2] + (k + 0.5) * h}; }
    [[nodiscard]] std::int32_t festBei(int i, int j, int k) const noexcept { return drin(i, j, k) ? fest[index(i, j, k)] : -1; }
};

// Schneidet das Dreieck (um die Mitte des Wuerfels verschoben) den Wuerfel mit halber Kantenlaenge r? Trennachsen:
// die drei Achsen des Wuerfels, die Normale des Dreiecks, die neun Kreuzprodukte Kante x Achse.
bool dreieckImWuerfel(const V3& v0, const V3& v1, const V3& v2, double r) noexcept {
    const V3 e[3] = {minus(v1, v0), minus(v2, v1), minus(v0, v2)};
    for (std::size_t a = 0; a < 3U; ++a) {
        const double lo = std::min({v0[a], v1[a], v2[a]});
        const double hi = std::max({v0[a], v1[a], v2[a]});
        if (lo > r || hi < -r) { return false; }
    }
    const V3 nrm = kreuz(e[0], e[1]);
    if (std::abs(punkt(nrm, v0)) > r * (std::abs(nrm[0]) + std::abs(nrm[1]) + std::abs(nrm[2]))) { return false; }
    for (const V3& k : e) {
        for (std::size_t a = 0; a < 3U; ++a) {
            V3 achse{0.0, 0.0, 0.0};
            achse[a] = 1.0;
            const V3 x = kreuz(k, achse);
            const double p0 = punkt(x, v0);
            const double p1 = punkt(x, v1);
            const double p2 = punkt(x, v2);
            const double rad = r * (std::abs(x[0]) + std::abs(x[1]) + std::abs(x[2]));
            if (std::min({p0, p1, p2}) > rad || std::max({p0, p1, p2}) < -rad) { return false; }
        }
    }
    return true;
}

Gitter gitterBauen(const Inhalt& in, int aufloesung) {
    Gitter g;
    V3 lo{0.0, 0.0, 0.0};
    V3 hi{0.0, 0.0, 0.0};
    bool hat = false;
    for (const Teil& t : in.teile) {
        if (!waehlbar(t)) { continue; }
        for (const Ecke& e : t.stufen[0].ecken) {
            const V3 p = ortVon(e);
            for (std::size_t a = 0; a < 3U; ++a) {
                lo[a] = hat ? std::min(lo[a], p[a]) : p[a];
                hi[a] = hat ? std::max(hi[a], p[a]) : p[a];
            }
            hat = true;
        }
    }
    const double groesste = std::max({hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]});
    if (!hat || !(groesste > 0.0) || !std::isfinite(groesste)) { return g; }
    double h = groesste / std::clamp(aufloesung, kAutoSkinAufloesungMin, kAutoSkinAufloesungMax);
    for (int runde = 0; runde < 2; ++runde) {
        double zahl = 1.0;
        for (std::size_t a = 0; a < 3U; ++a) {
            g.n[a] = static_cast<int>(std::ceil((hi[a] - lo[a]) / h)) + 1 + 2 * kRand;
            zahl *= g.n[a];
        }
        if (zahl <= kVoxelHoechstens) { break; }
        h *= std::cbrt(zahl / kVoxelHoechstens) * 1.01;
    }
    g.h = h;
    g.min = {lo[0] - kRand * h, lo[1] - kRand * h, lo[2] - kRand * h};
    const std::size_t alle = static_cast<std::size_t>(g.n[0]) * static_cast<std::size_t>(g.n[1]) * static_cast<std::size_t>(g.n[2]);
    g.art.assign(alle, 0);
    // Oberflaeche: jedes Voxel, das ein Dreieck schneidet (der Wuerfel um einen Bruchteil groesser, damit ein Dreieck
    // genau auf einer Voxelgrenze beide Seiten trifft)
    const double r = 0.5 * h * (1.0 + 1e-6);
    for (const Teil& t : in.teile) {
        if (!waehlbar(t)) { continue; }
        const Netz& netz = t.stufen[0];
        const std::vector<std::uint32_t> idx = netz.indizes();
        for (std::size_t k = 0; k + 2U < idx.size(); k += 3U) {
            if (idx[k] >= netz.ecken.size() || idx[k + 1U] >= netz.ecken.size() || idx[k + 2U] >= netz.ecken.size()) { continue; }
            const V3 p[3] = {ortVon(netz.ecken[idx[k]]), ortVon(netz.ecken[idx[k + 1U]]), ortVon(netz.ecken[idx[k + 2U]])};
            int von[3];
            int bis[3];
            for (std::size_t a = 0; a < 3U; ++a) {
                const double l = std::min({p[0][a], p[1][a], p[2][a]});
                const double o = std::max({p[0][a], p[1][a], p[2][a]});
                von[a] = std::clamp(static_cast<int>(std::floor((l - g.min[a]) / h - 1e-6)), 0, g.n[a] - 1);
                bis[a] = std::clamp(static_cast<int>(std::floor((o - g.min[a]) / h + 1e-6)), 0, g.n[a] - 1);
            }
            for (int z = von[2]; z <= bis[2]; ++z) {
                for (int y = von[1]; y <= bis[1]; ++y) {
                    for (int x = von[0]; x <= bis[0]; ++x) {
                        std::uint8_t& a = g.art[g.index(x, y, z)];
                        if (a != 0U) { continue; }
                        const V3 m = g.mitte(x, y, z);
                        if (dreieckImWuerfel(minus(p[0], m), minus(p[1], m), minus(p[2], m), r)) { a = 1U; }
                    }
                }
            }
        }
    }
    // Flutfuellung von aussen (6 Nachbarn) durch alles, was nicht Oberflaeche ist; der Rest ist innen
    constexpr std::uint8_t kAussen = 3U;
    std::vector<std::size_t> stapel;
    const auto besuchen = [&](int x, int y, int z) {
        std::uint8_t& a = g.art[g.index(x, y, z)];
        if (a != 0U) { return; }
        a = kAussen;
        stapel.push_back(g.index(x, y, z));
    };
    for (int z = 0; z < g.n[2]; ++z) {
        for (int y = 0; y < g.n[1]; ++y) {
            for (int x = 0; x < g.n[0]; ++x) {
                if (x == 0 || y == 0 || z == 0 || x == g.n[0] - 1 || y == g.n[1] - 1 || z == g.n[2] - 1) { besuchen(x, y, z); }
            }
        }
    }
    const std::size_t nx = static_cast<std::size_t>(g.n[0]);
    const std::size_t nxy = nx * static_cast<std::size_t>(g.n[1]);
    while (!stapel.empty()) {
        const std::size_t i = stapel.back();
        stapel.pop_back();
        const int x = static_cast<int>(i % nx);
        const int y = static_cast<int>((i % nxy) / nx);
        const int z = static_cast<int>(i / nxy);
        if (x > 0) { besuchen(x - 1, y, z); }
        if (x + 1 < g.n[0]) { besuchen(x + 1, y, z); }
        if (y > 0) { besuchen(x, y - 1, z); }
        if (y + 1 < g.n[1]) { besuchen(x, y + 1, z); }
        if (z > 0) { besuchen(x, y, z - 1); }
        if (z + 1 < g.n[2]) { besuchen(x, y, z + 1); }
    }
    g.fest.assign(alle, -1);
    for (std::size_t i = 0; i < alle; ++i) {
        std::uint8_t& a = g.art[i];
        if (a == kAussen) {
            a = 0U;
            continue;
        }
        if (a == 0U) {
            a = 2U;
            ++g.innen;
        }
        g.fest[i] = static_cast<std::int32_t>(g.zelle.size());
        g.zelle.push_back(static_cast<std::int32_t>(i));
    }
    return g;
}

struct Schritt {
    int d[3];
    double weg;
};
std::array<Schritt, 26> nachbarSchritte(double h) {
    std::array<Schritt, 26> s{};
    std::size_t z = 0;
    for (int k = -1; k <= 1; ++k) {
        for (int j = -1; j <= 1; ++j) {
            for (int i = -1; i <= 1; ++i) {
                if (i == 0 && j == 0 && k == 0) { continue; }
                s[z++] = Schritt{{i, j, k}, h * std::sqrt(static_cast<double>(i * i + j * j + k * k))};
            }
        }
    }
    return s;
}

// Freie Sicht von p nach q durch feste Voxel (Abtastung im halben Voxelabstand).
bool sicht(const Gitter& g, const V3& p, const V3& q) noexcept {
    const V3 pq = minus(q, p);
    const int schritte = std::max(1, static_cast<int>(std::ceil(laenge(pq) / (0.5 * g.h))));
    for (int s = 0; s <= schritte; ++s) {
        int ijk[3];
        g.zelleVon(plus(p, mal(pq, static_cast<double>(s) / schritte)), ijk);
        if (g.festBei(ijk[0], ijk[1], ijk[2]) < 0) { return false; }
    }
    return true;
}

// Der kuerzeste Weg jedes festen Voxels zur Form eines Knochens. Zurueck: lief die Form irgendwo durch ein festes Voxel?
bool voxelAbstand(const Gitter& g, const KnochenForm& f, const std::array<Schritt, 26>& schritte, std::vector<float>& dist) {
    dist.assign(g.zelle.size(), kAutoSkinUnerreicht);
    using Eintrag = std::pair<float, std::int32_t>;
    std::priority_queue<Eintrag, std::vector<Eintrag>, std::greater<>> offen;
    // Start: die festen Voxel um die Form mit ihrem geraden Abstand zur Form
    const auto startUm = [&](int x, int y, int z) {
        for (int k = z - 1; k <= z + 1; ++k) {
            for (int j = y - 1; j <= y + 1; ++j) {
                for (int i = x - 1; i <= x + 1; ++i) {
                    const std::int32_t v = g.festBei(i, j, k);
                    if (v < 0) { continue; }
                    const float d = static_cast<float>(abstandForm(f, g.mitte(i, j, k)));
                    if (d < dist[static_cast<std::size_t>(v)]) {
                        dist[static_cast<std::size_t>(v)] = d;
                        offen.emplace(d, v);
                    }
                }
            }
        }
    };
    bool innen = false;
    int letzte[3] = {-1000000, 0, 0};
    for (const std::array<double, 6>& s : f.strecken) {
        const V3 a{s[0], s[1], s[2]};
        const V3 ab = minus(V3{s[3], s[4], s[5]}, a);
        const int teile = std::max(1, static_cast<int>(std::ceil(laenge(ab) / (0.5 * g.h))));
        for (int t = 0; t <= teile; ++t) {
            int ijk[3];
            g.zelleVon(plus(a, mal(ab, static_cast<double>(t) / teile)), ijk);
            if (ijk[0] == letzte[0] && ijk[1] == letzte[1] && ijk[2] == letzte[2]) { continue; }
            std::copy(ijk, ijk + 3, letzte);
            if (g.festBei(ijk[0], ijk[1], ijk[2]) >= 0) {
                innen = true;
                startUm(ijk[0], ijk[1], ijk[2]);
                continue;
            }
            // Der Knochen liegt hier ausserhalb des Netzes (offenes Netz, Knochen daneben): er erreicht das Netz ueber
            // die naechsten festen Voxel - die erste Schale um ihn, die welche enthaelt, und die naechste dahinter. Liegt
            // er ausserhalb des Gitters, sucht er vom naechsten Voxel des Gitters aus.
            for (int achse = 0; achse < 3; ++achse) { ijk[achse] = std::clamp(ijk[achse], 0, g.n[achse] - 1); }
            const int weit = std::max({g.n[0], g.n[1], g.n[2]});
            int gefunden = -1;
            for (int r = 1; r <= weit && (gefunden < 0 || r <= gefunden + 1); ++r) {
                for (int k = ijk[2] - r; k <= ijk[2] + r; ++k) {
                    for (int j = ijk[1] - r; j <= ijk[1] + r; ++j) {
                        const bool rand = k == ijk[2] - r || k == ijk[2] + r || j == ijk[1] - r || j == ijk[1] + r;
                        for (int i = ijk[0] - r; i <= ijk[0] + r; i += rand ? 1 : 2 * r) {
                            const std::int32_t v = g.festBei(i, j, k);
                            if (v < 0) { continue; }
                            if (gefunden < 0) { gefunden = r; }
                            const float d = static_cast<float>(abstandForm(f, g.mitte(i, j, k)));
                            if (d < dist[static_cast<std::size_t>(v)]) {
                                dist[static_cast<std::size_t>(v)] = d;
                                offen.emplace(d, v);
                            }
                        }
                    }
                }
            }
        }
    }
    const std::size_t nx = static_cast<std::size_t>(g.n[0]);
    const std::size_t nxy = nx * static_cast<std::size_t>(g.n[1]);
    while (!offen.empty()) {
        const Eintrag e = offen.top();
        offen.pop();
        if (e.first > dist[static_cast<std::size_t>(e.second)]) { continue; }
        const auto i = static_cast<std::size_t>(g.zelle[static_cast<std::size_t>(e.second)]);
        const int x = static_cast<int>(i % nx);
        const int y = static_cast<int>((i % nxy) / nx);
        const int z = static_cast<int>(i / nxy);
        for (const Schritt& s : schritte) {
            const std::int32_t v = g.festBei(x + s.d[0], y + s.d[1], z + s.d[2]);
            if (v < 0) { continue; }
            const float d = static_cast<float>(static_cast<double>(e.first) + s.weg);
            if (d < dist[static_cast<std::size_t>(v)]) {
                dist[static_cast<std::size_t>(v)] = d;
                offen.emplace(d, v);
            }
        }
    }
    return innen;
}

// --- Gewichtslisten der Punkte ---------------------------------------------------------------------------------------

// So viele Knochen fuehrt ein Punkt vor dem Begrenzen (doppelt so viele, wie die .glm tragen kann: was beim Glaetten
// vom fuenften bis achten Platz nachrueckt, bleibt erhalten).
constexpr std::size_t kVorlaeufig = 8;

void ordnen(GewichtListe& l, std::size_t hoechstens) {
    std::sort(l.begin(), l.end(), [](const KnochenGewicht& a, const KnochenGewicht& b) {
        return a.gewicht != b.gewicht ? a.gewicht > b.gewicht : a.knochen < b.knochen;
    });
    while (!l.empty() && !(l.back().gewicht > 0.0F)) { l.pop_back(); }
    if (l.size() > hoechstens) { l.resize(hoechstens); }
    double summe = 0.0;
    for (const KnochenGewicht& g : l) { summe += static_cast<double>(g.gewicht); }
    if (summe > 0.0) {
        for (KnochenGewicht& g : l) { g.gewicht = static_cast<float>(static_cast<double>(g.gewicht) / summe); }
    }
}

void dazu(GewichtListe& l, int knochen, float w) {
    for (KnochenGewicht& g : l) {
        if (g.knochen == knochen) {
            g.gewicht += w;
            return;
        }
    }
    l.push_back(KnochenGewicht{knochen, w});
}

GewichtListe ausAbstand(const AutoSkinFeld& f, int p, double exponent) {
    GewichtListe l;
    float dmin = kAutoSkinUnerreicht;
    for (int b = 0; b < f.knochen; ++b) { dmin = std::min(dmin, f.d(p, b)); }
    if (!(dmin < kAutoSkinUnerreicht)) { return l; }
    for (int b = 0; b < f.knochen; ++b) {
        const float d = f.d(p, b);
        if (!(d < kAutoSkinUnerreicht)) { continue; }
        // Verhaeltnis zum naechsten Knochen: unabhaengig vom Massstab der Figur
        const double w = d <= 0.0F ? 1.0 : dmin <= 0.0F ? 0.0 : std::pow(static_cast<double>(dmin) / static_cast<double>(d), exponent);
        if (w > 1e-6) { l.push_back(KnochenGewicht{b, static_cast<float>(w)}); }
    }
    ordnen(l, kVorlaeufig);
    return l;
}

// Ein Durchgang: jeder freie Punkt wird zum Mittel aus sich und dem Mittel seiner Nachbarn.
void glaetten(std::vector<GewichtListe>& listen, const std::vector<std::vector<int>>& nachbarn, const std::vector<char>& fest) {
    std::vector<GewichtListe> neu(listen.size());
    arbeiter::verteilt(listen.size(), [&](std::size_t p) {
        const std::vector<int>& nb = nachbarn[p];
        if (nb.empty() || (!fest.empty() && fest[p] != 0)) {
            neu[p] = listen[p];
            return;
        }
        GewichtListe l;
        for (const KnochenGewicht& g : listen[p]) { dazu(l, g.knochen, 0.5F * g.gewicht); }
        const float anteil = 0.5F / static_cast<float>(nb.size());
        for (const int q : nb) {
            for (const KnochenGewicht& g : listen[static_cast<std::size_t>(q)]) { dazu(l, g.knochen, anteil * g.gewicht); }
        }
        ordnen(l, kVorlaeufig);
        neu[p] = std::move(l);
    }, 512);
    listen.swap(neu);
}

// Die Liste in die Ecke, wie das Spiel sie liest (fuersSpiel). Runden die ersten Gewichte zusammen auf 1023 von 1023, bliebe
// dem letzten als Rest genau 0 - ein Knochen ohne Gewicht, den der Schreiber weglaesst (glmdatei.cpp, paare), worauf das
// Spiel das vorletzte als Rest liest. Darum faellt er hier schon weg: in der Ecke steht dann, was aus der Datei kommt.
bool inEcke(GewichtListe l, Ecke& e) {
    SpielGewichte s;
    for (;;) {
        if (!fuersSpiel(l, s)) { return false; }
        if (s.zahl <= 1 || s.gewicht[s.zahl - 1] > 0.0F) { break; }
        const int weg = s.knochen[s.zahl - 1];
        l.erase(std::remove_if(l.begin(), l.end(), [weg](const KnochenGewicht& g) { return g.knochen == weg; }), l.end());
    }
    for (int k = 0; k < 4; ++k) {
        e.knochen[k] = k < s.zahl ? static_cast<std::uint8_t>(std::clamp(s.knochen[k], 0, 255)) : 0U;
        e.gewicht[k] = k < s.zahl ? s.gewicht[k] : 0.0F;
    }
    return true;
}

// --- Uebertragen -----------------------------------------------------------------------------------------------------

// Der naechste Punkt des Dreiecks abc zu p mit seinen Anteilen (u, v, w) an a, b, c: die sieben Bereiche um das Dreieck
// (drei Ecken, drei Kanten, das Innere) ueber die Skalarprodukte der Kanten.
V3 naechsterImDreieck(const V3& p, const V3& a, const V3& b, const V3& c, double uvw[3]) noexcept {
    const V3 ab = minus(b, a);
    const V3 ac = minus(c, a);
    const V3 ap = minus(p, a);
    const double d1 = punkt(ab, ap);
    const double d2 = punkt(ac, ap);
    const auto gib = [&](double u, double v, double w) {
        uvw[0] = u;
        uvw[1] = v;
        uvw[2] = w;
        return plus(plus(mal(a, u), mal(b, v)), mal(c, w));
    };
    if (d1 <= 0.0 && d2 <= 0.0) { return gib(1.0, 0.0, 0.0); }
    const V3 bp = minus(p, b);
    const double d3 = punkt(ab, bp);
    const double d4 = punkt(ac, bp);
    if (d3 >= 0.0 && d4 <= d3) { return gib(0.0, 1.0, 0.0); }
    const double vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
        const double v = d1 - d3 != 0.0 ? d1 / (d1 - d3) : 0.0;
        return gib(1.0 - v, v, 0.0);
    }
    const V3 cp = minus(p, c);
    const double d5 = punkt(ab, cp);
    const double d6 = punkt(ac, cp);
    if (d6 >= 0.0 && d5 <= d6) { return gib(0.0, 0.0, 1.0); }
    const double vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
        const double w = d2 - d6 != 0.0 ? d2 / (d2 - d6) : 0.0;
        return gib(1.0 - w, 0.0, w);
    }
    const double va = d3 * d6 - d5 * d4;
    if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
        const double n = (d4 - d3) + (d5 - d6);
        const double w = n != 0.0 ? (d4 - d3) / n : 0.0;
        return gib(0.0, 1.0 - w, w);
    }
    const double summe = va + vb + vc;
    if (!(summe != 0.0)) { return gib(1.0, 0.0, 0.0); }
    return gib(va / summe, vb / summe, vc / summe);
}

struct VorlageDreieck {
    V3 p[3];
    V3 mitte;
    double radius = 0.0;
    int teil = 0;
    std::uint32_t ecke[3]{};
};

struct VorlageNetz {
    std::vector<VorlageDreieck> dreiecke;
    std::vector<std::vector<GewichtListe>> listen;          // [teil][ecke], Knochen schon umgelegt
    std::map<std::string, std::vector<int>> teileNachName;   // kleingeschrieben
    std::vector<std::pair<std::size_t, std::size_t>> bereich; // je Teil [von, bis) in dreiecke
};

VorlageNetz vorlageLesen(const Inhalt& v, const std::vector<int>& knochenKarte, int knochenZahl) {
    VorlageNetz n;
    n.listen.resize(v.teile.size());
    n.bereich.assign(v.teile.size(), {0U, 0U});
    for (std::size_t t = 0; t < v.teile.size(); ++t) {
        if (!waehlbar(v.teile[t])) { continue; }
        const Netz& netz = v.teile[t].stufen[0];
        n.listen[t].resize(netz.ecken.size());
        for (std::size_t e = 0; e < netz.ecken.size(); ++e) {
            for (const KnochenGewicht& g : listeAus(netz.ecken[e])) {
                const int k = knochenKarte.empty() ? g.knochen
                              : static_cast<std::size_t>(g.knochen) < knochenKarte.size() ? knochenKarte[static_cast<std::size_t>(g.knochen)] : -1;
                if (k >= 0 && k < knochenZahl) { dazu(n.listen[t][e], k, g.gewicht); }
            }
        }
        n.bereich[t].first = n.dreiecke.size();
        const std::vector<std::uint32_t> idx = netz.indizes();
        for (std::size_t k = 0; k + 2U < idx.size(); k += 3U) {
            if (idx[k] >= netz.ecken.size() || idx[k + 1U] >= netz.ecken.size() || idx[k + 2U] >= netz.ecken.size()) { continue; }
            VorlageDreieck d;
            for (std::size_t j = 0; j < 3U; ++j) {
                d.ecke[j] = idx[k + j];
                d.p[j] = ortVon(netz.ecken[idx[k + j]]);
            }
            d.mitte = mal(plus(plus(d.p[0], d.p[1]), d.p[2]), 1.0 / 3.0);
            for (const V3& q : d.p) { d.radius = std::max(d.radius, laenge(minus(q, d.mitte))); }
            d.teil = static_cast<int>(t);
            n.dreiecke.push_back(d);
        }
        n.bereich[t].second = n.dreiecke.size();
        if (n.bereich[t].second > n.bereich[t].first) { n.teileNachName[klein(v.teile[t].name)].push_back(static_cast<int>(t)); }
    }
    return n;
}

// Die Gewichte des naechsten Punkts der Vorlage; `abstand` der Weg dorthin (unerreicht: die Vorlage hat kein Dreieck).
GewichtListe vonVorlage(const VorlageNetz& n, const V3& p, const std::vector<int>& vorlageTeile, double& abstand) {
    double best = std::numeric_limits<double>::max();
    const VorlageDreieck* treffer = nullptr;
    double anteile[3] = {1.0, 0.0, 0.0};
    const auto pruefen = [&](std::size_t von, std::size_t bis) {
        for (std::size_t i = von; i < bis; ++i) {
            const VorlageDreieck& d = n.dreiecke[i];
            if (laenge(minus(p, d.mitte)) - d.radius >= best) { continue; }
            double uvw[3];
            const double a = laenge(minus(p, naechsterImDreieck(p, d.p[0], d.p[1], d.p[2], uvw)));
            if (a < best) {
                best = a;
                treffer = &d;
                std::copy(uvw, uvw + 3, anteile);
            }
        }
    };
    if (vorlageTeile.empty()) {
        pruefen(0U, n.dreiecke.size());
    } else {
        for (const int t : vorlageTeile) { pruefen(n.bereich[static_cast<std::size_t>(t)].first, n.bereich[static_cast<std::size_t>(t)].second); }
    }
    abstand = best;
    GewichtListe l;
    if (treffer == nullptr) { return l; }
    for (std::size_t j = 0; j < 3U; ++j) {
        for (const KnochenGewicht& g : n.listen[static_cast<std::size_t>(treffer->teil)][treffer->ecke[j]]) {
            dazu(l, g.knochen, static_cast<float>(anteile[j] * static_cast<double>(g.gewicht)));
        }
    }
    ordnen(l, kVorlaeufig);
    return l;
}

// --- Grenzen ---------------------------------------------------------------------------------------------------------

// Hoechstens `grenze` Knochen je Flaeche: der Knochen mit dem kleinsten Gesamtgewicht der Flaeche faellt weg, bis es
// passt. Die Listen gehoeren den Punkten - an einer Naht aendert sich die Nachbarflaeche mit, die Naht bleibt zu. Ein
// Punkt, dem dabei nichts bleibt, haengt sich an den naechsten Knochen, den die Flaeche behaelt (`abstand`; ohne
// Abstaende - reines Uebertragen - an ihren staerksten).
int knochenBegrenzen(std::vector<GewichtListe>& listen, const Sammlung& s, int grenze, int knochenZahl,
                     const std::function<float(std::size_t, int)>& abstand) {
    std::vector<char> gekuerzt;
    std::size_t flaechen = 0;
    for (const auto& je : s.punktVon) { flaechen += je.size(); }
    gekuerzt.assign(flaechen, 0);
    const std::size_t runden = flaechen * static_cast<std::size_t>(std::max(knochenZahl, 1)) + 1U;
    bool geaendert = true;
    for (std::size_t runde = 0; geaendert && runde < runden; ++runde) {
        geaendert = false;
        std::size_t nr = 0;
        for (const auto& je : s.punktVon) {
            for (const std::vector<int>& pv : je) {
                const std::size_t diese = nr++;
                std::vector<double> summe(static_cast<std::size_t>(knochenZahl), 0.0);
                int zahl = 0;
                for (const int p : pv) {
                    for (const KnochenGewicht& g : listen[static_cast<std::size_t>(p)]) {
                        if (g.knochen < 0 || g.knochen >= knochenZahl) { continue; }
                        if (summe[static_cast<std::size_t>(g.knochen)] == 0.0) { ++zahl; }
                        summe[static_cast<std::size_t>(g.knochen)] += static_cast<double>(g.gewicht) + 1e-30;
                    }
                }
                if (zahl <= grenze) { continue; }
                int weg = -1;
                int staerkster = -1;
                for (int b = 0; b < knochenZahl; ++b) {
                    const double w = summe[static_cast<std::size_t>(b)];
                    if (w == 0.0) { continue; }
                    if (weg < 0 || w < summe[static_cast<std::size_t>(weg)]) { weg = b; }
                    if (staerkster < 0 || w > summe[static_cast<std::size_t>(staerkster)]) { staerkster = b; }
                }
                for (const int p : pv) {
                    GewichtListe& l = listen[static_cast<std::size_t>(p)];
                    const auto alt = l.size();
                    l.erase(std::remove_if(l.begin(), l.end(), [&](const KnochenGewicht& g) { return g.knochen == weg; }), l.end());
                    if (l.size() == alt) { continue; }
                    if (l.empty()) {
                        int ersatz = staerkster;
                        if (abstand) {
                            float best = kAutoSkinUnerreicht;
                            for (int b = 0; b < knochenZahl; ++b) {
                                if (b == weg || summe[static_cast<std::size_t>(b)] == 0.0) { continue; }
                                const float d = abstand(static_cast<std::size_t>(p), b);
                                if (d < best) {
                                    best = d;
                                    ersatz = b;
                                }
                            }
                        }
                        l.push_back(KnochenGewicht{ersatz, 1.0F});
                    }
                    ordnen(l, kVorlaeufig);
                }
                gekuerzt[diese] = 1;
                geaendert = true;
            }
        }
    }
    return static_cast<int>(std::count(gekuerzt.begin(), gekuerzt.end(), static_cast<char>(1)));
}

}  // namespace

AutoSkinFeld autoSkinFeld(const Inhalt& in, const GlaAnimation& g, const AutoSkinOptionen& o) {
    AutoSkinFeld f;
    f.knochen = static_cast<int>(g.bones.size());
    Sammlung s = sammeln(in, o);
    f.punkte = static_cast<int>(s.ort.size());
    f.punktVon = std::move(s.punktVon);
    f.ort = std::move(s.ort);
    f.formen = knochenFormen(g, erkenneKnochen(g), o, in);
    const std::size_t nk = g.bones.size();
    const std::size_t np = f.ort.size();
    f.abstand.assign(np * nk, kAutoSkinUnerreicht);
    std::vector<std::size_t> tragend;
    for (std::size_t b = 0; b < nk; ++b) {
        if (f.formen[b].traegt) { tragend.push_back(b); }
    }
    if (tragend.empty() || np == 0U) { return f; }
    const auto gerade = [&](std::size_t b) {
        for (std::size_t p = 0; p < np; ++p) { f.abstand[p * nk + b] = static_cast<float>(abstandForm(f.formen[b], f.ort[p])); }
    };
    if (o.verfahren == AutoSkinVerfahren::Huelle) {
        for (const std::size_t b : tragend) { gerade(b); }
        return f;
    }
    const Gitter gitter = gitterBauen(in, o.aufloesung);
    if (!gitter.da()) {   // kein Dreieck mit Ausdehnung: nur der gerade Abstand
        for (const std::size_t b : tragend) { gerade(b); }
        f.punkteUnerreicht = f.punkte;
        return f;
    }
    std::copy(gitter.n, gitter.n + 3, f.voxel);
    f.voxelGroesse = gitter.h;
    f.voxelFest = static_cast<int>(gitter.zelle.size());
    f.voxelInnen = gitter.innen;
    const std::array<Schritt, 26> schritte = nachbarSchritte(gitter.h);
    std::vector<char> innen(nk, 0);
    std::vector<int> sichtZahl(nk, 0);
    arbeiter::verteilt(tragend.size(), [&](std::size_t nr) {
        const std::size_t b = tragend[nr];
        std::vector<float> dist;
        innen[b] = voxelAbstand(gitter, f.formen[b], schritte, dist) ? 1 : 0;
        int frei = 0;
        for (std::size_t p = 0; p < np; ++p) {
            const V3& ort = f.ort[p];
            int ijk[3];
            gitter.zelleVon(ort, ijk);
            // der kuerzeste Weg ueber eines der festen Voxel um den Punkt
            double weg = static_cast<double>(kAutoSkinUnerreicht);
            for (int k = ijk[2] - 1; k <= ijk[2] + 1; ++k) {
                for (int j = ijk[1] - 1; j <= ijk[1] + 1; ++j) {
                    for (int i = ijk[0] - 1; i <= ijk[0] + 1; ++i) {
                        const std::int32_t v = gitter.festBei(i, j, k);
                        if (v < 0 || !(dist[static_cast<std::size_t>(v)] < kAutoSkinUnerreicht)) { continue; }
                        weg = std::min(weg, static_cast<double>(dist[static_cast<std::size_t>(v)]) + laenge(minus(ort, gitter.mitte(i, j, k))));
                    }
                }
            }
            // mit freier Sicht durch das Netz ist der gerade Abstand der kuerzeste Weg
            V3 q{};
            const double direkt = abstandForm(f.formen[b], ort, &q);
            if (direkt < weg && sicht(gitter, ort, q)) {
                weg = direkt;
                ++frei;
            }
            if (weg < static_cast<double>(kAutoSkinUnerreicht)) { f.abstand[p * nk + b] = static_cast<float>(weg); }
        }
        sichtZahl[b] = frei;
    }, 2);
    for (const std::size_t b : tragend) {
        f.knochenAussen += innen[b] != 0 ? 0 : 1;
        f.punkteSicht += sichtZahl[b];
    }
    // Punkte ohne Weg zu irgendeinem Knochen (ein loses Teil ohne Knochen darin): der gerade Abstand
    for (std::size_t p = 0; p < np; ++p) {
        bool erreicht = false;
        for (const std::size_t b : tragend) { erreicht = erreicht || f.abstand[p * nk + b] < kAutoSkinUnerreicht; }
        if (erreicht) { continue; }
        ++f.punkteUnerreicht;
        for (const std::size_t b : tragend) { f.abstand[p * nk + b] = static_cast<float>(abstandForm(f.formen[b], f.ort[p])); }
    }
    return f;
}

bool autoSkin(Inhalt& in, const GlaAnimation& g, const AutoSkinOptionen& o, AutoSkinBericht& bericht, const Inhalt* vorlage,
              const std::vector<int>& vorlageKnochen, const AutoSkinFeld* fertigesFeld) {
    bericht = AutoSkinBericht{};
    const Grenzen& grenze = grenzen(o.ziel);
    bericht.gewichteGrenze = grenze.gewichteJeEcke;
    bericht.knochenGrenze = grenze.knochenJeFlaeche;
    const int nk = static_cast<int>(g.bones.size());
    // die Ecke traegt den Knochen in einem Byte (Ecke::knochen)
    if (nk <= 0 || nk > 256) {
        bericht.fehler = "skelett";
        return false;
    }
    const Sammlung s = sammeln(in, o);
    if (s.ort.empty()) {
        bericht.fehler = "flaechen";
        return false;
    }
    const std::size_t np = s.ort.size();
    const bool uebertragen = o.verfahren == AutoSkinVerfahren::Uebertragen;
    if (uebertragen && (vorlage == nullptr || vorlage->leer())) {
        bericht.fehler = "vorlage";
        return false;
    }
    std::vector<GewichtListe> listen(np);
    std::vector<char> fest;   // Uebertragen: Punkte mit Gewichten des Originals (das Glaetten laesst sie)
    AutoSkinFeld eigenesFeld;
    const AutoSkinFeld* feldZeiger = nullptr;
    bool hatFeld = false;
    const auto feldHolen = [&] {
        if (hatFeld) { return; }
        if (fertigesFeld != nullptr && fertigesFeld->knochen == nk && fertigesFeld->ort.size() == np &&
            fertigesFeld->abstand.size() == np * static_cast<std::size_t>(nk) && fertigesFeld->formen.size() == static_cast<std::size_t>(nk)) {
            feldZeiger = fertigesFeld;
        } else {
            AutoSkinOptionen fo = o;
            if (uebertragen) { fo.verfahren = AutoSkinVerfahren::Voxel; }
            eigenesFeld = autoSkinFeld(in, g, fo);
            feldZeiger = &eigenesFeld;
        }
        const AutoSkinFeld& feld = *feldZeiger;
        hatFeld = true;
        std::copy(feld.voxel, feld.voxel + 3, bericht.voxel);
        bericht.voxelGroesse = feld.voxelGroesse;
        bericht.voxelFest = feld.voxelFest;
        bericht.voxelInnen = feld.voxelInnen;
        bericht.knochenAussen = feld.knochenAussen;
        bericht.punkteUnerreicht = feld.punkteUnerreicht;
    };
    const double exponent = autoSkinExponent(o.steifigkeit);
    if (uebertragen) {
        const VorlageNetz vn = vorlageLesen(*vorlage, vorlageKnochen, nk);
        if (vn.dreiecke.empty()) {
            bericht.fehler = "vorlage";
            return false;
        }
        fest.assign(np, 0);
        arbeiter::verteilt(np, [&](std::size_t p) {
            // Flaechen gleichen Namens in der Vorlage (der Arm sucht im Arm); ohne solche in allen
            std::vector<int> teile;
            for (const int t : s.teileVon[p]) {
                const auto it = vn.teileNachName.find(klein(in.teile[static_cast<std::size_t>(t)].name));
                if (it != vn.teileNachName.end()) { teile.insert(teile.end(), it->second.begin(), it->second.end()); }
            }
            double abstand = 0.0;
            GewichtListe l = vonVorlage(vn, s.ort[p], teile, abstand);
            if (!l.empty() && abstand <= static_cast<double>(o.uebertragenAbstand)) {
                listen[p] = std::move(l);
                fest[p] = 1;
            }
        }, 64);
        bericht.uebertragen = static_cast<int>(std::count(fest.begin(), fest.end(), static_cast<char>(1)));
        bericht.fern = static_cast<int>(np) - bericht.uebertragen;
        if (bericht.fern > 0) {
            feldHolen();
            for (std::size_t p = 0; p < np; ++p) {
                if (fest[p] == 0) { listen[p] = ausAbstand(*feldZeiger, static_cast<int>(p), exponent); }
            }
        }
    } else {
        feldHolen();
        arbeiter::verteilt(np, [&](std::size_t p) { listen[p] = ausAbstand(*feldZeiger, static_cast<int>(p), exponent); }, 512);
    }
    if (hatFeld) {
        for (const KnochenForm& f : feldZeiger->formen) { bericht.knochenTragend += f.traegt ? 1 : 0; }
        if (bericht.knochenTragend == 0) {
            bericht.fehler = "knochen";
            return false;
        }
    }
    for (int d = 0; d < std::clamp(o.glaetten, 0, kAutoSkinGlaettenMax); ++d) { glaetten(listen, s.nachbarn, fest); }
    // hoechstens so viele Gewichte je Ecke, wie die Datei traegt
    std::vector<char> gekuerzt(np, 0);
    for (std::size_t p = 0; p < np; ++p) {
        // gekuerzt ist, wer mehr Gewichte hatte, als die 10 Bit der Datei auch getragen haetten (ab einem halben 1023stel)
        std::size_t tragbar = 0;
        for (const KnochenGewicht& x : listen[p]) { tragbar += static_cast<double>(x.gewicht) * 1023.0 >= 0.5 ? 1U : 0U; }
        gekuerzt[p] = tragbar > static_cast<std::size_t>(grenze.gewichteJeEcke) ? 1 : 0;
        ordnen(listen[p], static_cast<std::size_t>(grenze.gewichteJeEcke));
        // Gewichte unter der Schwelle fallen weg (das staerkste bleibt immer)
        GewichtListe& l = listen[p];
        const std::size_t vorher = l.size();
        while (l.size() > 1U && l.back().gewicht < o.schwelle) { l.pop_back(); }
        if (l.size() != vorher) { ordnen(l, l.size()); }
    }
    std::function<float(std::size_t, int)> abstand;
    if (hatFeld) {
        abstand = [feldZeiger](std::size_t p, int b) { return feldZeiger->d(static_cast<int>(p), b); };
    }
    bericht.flaechenGekuerzt = knochenBegrenzen(listen, s, grenze.knochenJeFlaeche, nk, abstand);
    // in die Ecken, wie das Spiel sie liest
    for (std::size_t t = 0; t < in.teile.size(); ++t) {
        if (s.gewaehlt[t] == 0) { continue; }
        for (std::size_t st = 0; st < in.teile[t].stufen.size(); ++st) {
            Netz& netz = in.teile[t].stufen[st];
            const std::vector<int>& pv = s.punktVon[t][st];
            for (std::size_t e = 0; e < netz.ecken.size() && e < pv.size(); ++e) {
                const auto p = static_cast<std::size_t>(pv[e]);
                if (inEcke(listen[p], netz.ecken[e])) {
                    ++bericht.ecken;
                    bericht.eckenGekuerzt += gekuerzt[p] != 0 ? 1 : 0;
                }
            }
            netz.mal.clear();
        }
    }
    bericht.punkte = static_cast<int>(np);
    if (o.spiegeln) {
        std::vector<std::string> namen;
        namen.reserve(g.bones.size());
        for (const GlaBone& b : g.bones) { namen.push_back(b.name); }
        // FIX612B P2-1: mit "Nur Auswahl" schreibt das Spiegeln nur in die gewaehlten Flaechen (gelesen wird ueberall), und
        // uebernommen werden nur die Mallisten, die das Spiegeln selbst angelegt hat. Vorher lief es ueber das Schweissnetz
        // aller Flaechen - jede Ecke bei x < 0 bekam die Gewichte ihres Gegenstuecks, auch in nicht gewaehlten Flaechen - und
        // uebernehmen(in) nahm jede offene Malliste des Modells mit.
        const bool nurWahl = o.teile.size() == in.teile.size();
        std::vector<std::vector<char>> malte(in.teile.size());
        for (std::size_t t = 0; t < in.teile.size(); ++t) {
            for (const Netz& netz : in.teile[t].stufen) { malte[t].push_back(malt(netz) ? 1 : 0); }
        }
        for (int st = 0; st < in.stufenZahl; ++st) {
            const Schweissnetz sn = schweissen(in, st);
            bericht.gespiegelt += spiegelnSeite(in, st, namen, sn, spiegelPartner(in, st, sn, o.spiegelToleranz), 1, o.spiegelToleranz, nullptr,
                                                nurWahl ? &s.gewaehlt : nullptr);
        }
        for (std::size_t t = 0; t < in.teile.size(); ++t) {
            for (std::size_t st = 0; st < in.teile[t].stufen.size(); ++st) {
                Netz& netz = in.teile[t].stufen[st];
                if (malte[t][st] != 0 || !malt(netz)) { continue; }   // offene Mallisten von vorher bleiben offen
                for (std::size_t e = 0; e < netz.mal.size() && e < netz.ecken.size(); ++e) { (void)aufEcke(netz.mal[e], netz.ecken[e]); }
                netz.mal.clear();
            }
        }
    }
    // was jetzt in den Ecken steht
    std::vector<char> benutzt(static_cast<std::size_t>(nk), 0);
    for (std::size_t t = 0; t < in.teile.size(); ++t) {
        if (!waehlbar(in.teile[t])) { continue; }
        for (const Netz& netz : in.teile[t].stufen) {
            const FlaechenMass m = mass(netz);
            if (s.gewaehlt[t] != 0) { bericht.gewichteMax = std::max(bericht.gewichteMax, m.gewichteMax); }
            if (m.knochen > bericht.knochenJeFlaecheMax) {
                bericht.knochenJeFlaecheMax = m.knochen;
                bericht.knochenJeFlaecheName = in.teile[t].name;
            }
            if (s.gewaehlt[t] == 0) { continue; }
            for (const Ecke& e : netz.ecken) {
                for (int k = 0; k < 4; ++k) {
                    if (e.gewicht[k] > 0.0F && e.knochen[k] < nk) { benutzt[e.knochen[k]] = 1; }
                }
            }
        }
    }
    bericht.knochenBenutzt = static_cast<int>(std::count(benutzt.begin(), benutzt.end(), static_cast<char>(1)));
    bericht.ok = true;
    return true;
}

GewichtVergleich gewichteVergleichen(const Inhalt& a, const Inhalt& b, int stufe, int knochenZahl, const std::vector<char>& teile) {
    GewichtVergleich v;
    const auto nk = static_cast<std::size_t>(std::max(knochenZahl, 1));
    v.eckenJeKnochen.assign(nk, 0);
    v.hauptGleichJeKnochen.assign(nk, 0);
    double summe = 0.0;
    std::vector<double> wa(nk);
    std::vector<double> wb(nk);
    for (std::size_t t = 0; t < a.teile.size() && t < b.teile.size(); ++t) {
        if (!waehlbar(a.teile[t]) || (teile.size() == a.teile.size() && teile[t] == 0)) { continue; }
        if (static_cast<std::size_t>(stufe) >= a.teile[t].stufen.size() || static_cast<std::size_t>(stufe) >= b.teile[t].stufen.size()) { continue; }
        const Netz& na = a.teile[t].stufen[static_cast<std::size_t>(stufe)];
        const Netz& nb = b.teile[t].stufen[static_cast<std::size_t>(stufe)];
        if (na.ecken.size() != nb.ecken.size()) { continue; }
        for (std::size_t e = 0; e < na.ecken.size(); ++e) {
            std::fill(wa.begin(), wa.end(), 0.0);
            std::fill(wb.begin(), wb.end(), 0.0);
            for (int k = 0; k < 4; ++k) {
                if (na.ecken[e].gewicht[k] > 0.0F && na.ecken[e].knochen[k] < nk) { wa[na.ecken[e].knochen[k]] += static_cast<double>(na.ecken[e].gewicht[k]); }
                if (nb.ecken[e].gewicht[k] > 0.0F && nb.ecken[e].knochen[k] < nk) { wb[nb.ecken[e].knochen[k]] += static_cast<double>(nb.ecken[e].gewicht[k]); }
            }
            std::size_t ha = 0;
            std::size_t hb = 0;
            double ab = 0.0;
            for (std::size_t k = 0; k < nk; ++k) {
                if (wa[k] > wa[ha]) { ha = k; }
                if (wb[k] > wb[hb]) { hb = k; }
                ab += std::abs(wa[k] - wb[k]);
            }
            ++v.ecken;
            ++v.eckenJeKnochen[ha];
            if (ha == hb) {
                ++v.hauptGleich;
                ++v.hauptGleichJeKnochen[ha];
            }
            summe += 0.5 * ab;
        }
    }
    v.abweichungMittel = v.ecken > 0 ? summe / v.ecken : 0.0;
    return v;
}

VerformungsFehler verformungsFehler(const Inhalt& a, const Inhalt& b, const GlaAnimation& g, const std::vector<int>& bilder, int stufe,
                                    const std::vector<char>& teile) {
    VerformungsFehler f;
    std::vector<float> alle;
    std::vector<BoneMatrix> welt;
    std::vector<BoneMatrix> haut(g.bones.size());
    double summe = 0.0;
    for (const int bild : bilder) {
        if (bild < 0 || bild >= g.bildZahl()) { continue; }
        g.worldMatrices(bild, welt);
        if (welt.size() != g.bones.size()) { continue; }
        for (std::size_t k = 0; k < g.bones.size(); ++k) { haut[k] = welt[k] * g.bones[k].basePoseInv; }
        ++f.bilder;
        int ecken = 0;
        for (std::size_t t = 0; t < a.teile.size() && t < b.teile.size(); ++t) {
            if (!waehlbar(a.teile[t]) || (teile.size() == a.teile.size() && teile[t] == 0)) { continue; }
            if (static_cast<std::size_t>(stufe) >= a.teile[t].stufen.size() || static_cast<std::size_t>(stufe) >= b.teile[t].stufen.size()) { continue; }
            const Netz& na = a.teile[t].stufen[static_cast<std::size_t>(stufe)];
            const Netz& nb = b.teile[t].stufen[static_cast<std::size_t>(stufe)];
            if (na.ecken.size() != nb.ecken.size()) { continue; }
            for (std::size_t e = 0; e < na.ecken.size(); ++e) {
                double pa[3] = {0.0, 0.0, 0.0};
                double pb[3] = {0.0, 0.0, 0.0};
                const auto verformen = [&](const Ecke& ecke, double aus[3]) {
                    for (int k = 0; k < 4; ++k) {
                        if (!(ecke.gewicht[k] > 0.0F) || ecke.knochen[k] >= haut.size()) { continue; }
                        float o[3];
                        haut[ecke.knochen[k]].transform(ecke.ort, o);
                        for (int r = 0; r < 3; ++r) { aus[r] += static_cast<double>(ecke.gewicht[k]) * static_cast<double>(o[r]); }
                    }
                };
                verformen(na.ecken[e], pa);
                verformen(nb.ecken[e], pb);
                const double d = std::sqrt((pa[0] - pb[0]) * (pa[0] - pb[0]) + (pa[1] - pb[1]) * (pa[1] - pb[1]) + (pa[2] - pb[2]) * (pa[2] - pb[2]));
                alle.push_back(static_cast<float>(d));
                summe += d;
                f.max = std::max(f.max, d);
                ++ecken;
            }
        }
        f.ecken = ecken;
    }
    if (!alle.empty()) {
        f.mittel = summe / static_cast<double>(alle.size());
        const std::size_t k = std::min(alle.size() - 1U, static_cast<std::size_t>(0.95 * static_cast<double>(alle.size())));
        std::nth_element(alle.begin(), alle.begin() + static_cast<std::ptrdiff_t>(k), alle.end());
        f.p95 = static_cast<double>(alle[k]);
    }
    return f;
}

}  // namespace bhed::modell
