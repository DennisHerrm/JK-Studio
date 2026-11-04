// wegpunkte.cpp - Wegnetz lesen, erzeugen, pruefen. Warum so, steht in
// bhed/wegpunkte.h; die Engine-Stellen in code/game/g_navigator.cpp.
#include "bhed/wegpunkte.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <functional>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <tuple>
#include <unordered_map>

namespace bhed {
namespace {

// g_navigator.cpp, enum am Anfang von NAV
constexpr float kMaxAutoLaenge = 500.0F;    // MAX_EDGE_AUTO_LEN
constexpr float kMaxEtage = 60.0F;          // MAX_EDGE_FLOOR_DIST
constexpr int kNachbarn = 4;                // MIN_WAY_NEIGHBORS
constexpr int kMaxNichtWeg = 1;             // MAX_NONWP_NEIGHBORS

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

int flags(const MapEntity& e) {
    const std::string* f = e.find("spawnflags");
    return f != nullptr ? std::atoi(f->c_str()) : 0;
}

float abstand(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    const float dx = a[0] - b[0];
    const float dy = a[1] - b[1];
    const float dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool istWegKlasseKlein(const std::string& k) {
    return k == "waypoint" || k == "waypoint_small" || k.rfind("waypoint_navgoal", 0) == 0 || k == "point_combat";
}

}  // namespace

bool istWegKlasse(const std::string& klasse) { return istWegKlasseKlein(klein(klasse)); }

WegNetz baueWegNetz(const std::vector<MapEntity>& ents, const WegSicht& sicht) {
    WegNetz netz;
    std::map<std::string, std::vector<int>> namen;   // targetname -> Knoten (auch mehrere gleichen Namens)
    for (std::size_t i = 0; i < ents.size(); ++i) {
        const MapEntity& e = ents[i];
        const std::string k = klein(e.classname);
        if (!istWegKlasseKlein(k) || e.origin.empty()) { continue; }
        WegKnoten n;
        n.art = (k == "point_combat") ? WegKnoten::Art::Kampf
                : (k.rfind("waypoint_navgoal", 0) == 0) ? WegKnoten::Art::Ziel
                                                        : WegKnoten::Art::Wegpunkt;
        n.entity = static_cast<int>(i);
        std::istringstream is(e.origin);
        is >> n.ort[0] >> n.ort[1] >> n.ort[2];
        n.name = e.targetname;
        n.keinAuto = (flags(e) & 4) != 0;
        if (!n.name.empty()) { namen[n.name].push_back(static_cast<int>(netz.knoten.size())); }
        netz.knoten.push_back(std::move(n));
    }
    std::set<std::pair<int, int>> da;   // ungerichtet: (min, max)
    const auto verbinde = [&](int a, int b, bool designer, bool sprung) {
        if (a == b) { return; }
        const auto schl = std::make_pair(std::min(a, b), std::max(a, b));
        if (da.count(schl) != 0U) {
            if (sprung) {
                for (WegKante& k : netz.kanten) {
                    if (std::min(k.a, k.b) == schl.first && std::max(k.a, k.b) == schl.second) { k.sprung = true; }
                }
            }
            return;
        }
        da.insert(schl);
        netz.kanten.push_back(WegKante{a, b, designer, sprung});
    };
    // Designerkanten: target, target2, target3, target4, targetJump (das letzte = Sprung)
    static const char* const kZiele[5] = {"target", "target2", "target3", "target4", "targetJump"};
    for (std::size_t i = 0; i < netz.knoten.size(); ++i) {
        const MapEntity& e = ents[static_cast<std::size_t>(netz.knoten[i].entity)];
        for (int z = 0; z < 5; ++z) {
            const std::string* t = e.find(kZiele[z]);
            if (t == nullptr || t->empty()) { continue; }
            const auto it = namen.find(*t);
            if (it == namen.end()) {
                netz.fehlendeZiele.emplace_back(static_cast<int>(i), *t);
                continue;
            }
            for (const int ziel : it->second) { verbinde(static_cast<int>(i), ziel, true, z == 4); }
        }
    }
    // Automatisch: nur Kampf- und Zielpunkte, zu den naechsten vier.
    const std::size_t designerKanten = netz.kanten.size();
    for (std::size_t i = 0; i < netz.knoten.size(); ++i) {
        const WegKnoten& at = netz.knoten[i];
        if (at.keinAuto || at.art == WegKnoten::Art::Wegpunkt) { continue; }
        std::vector<std::pair<float, int>> kandidaten;
        int nichtWeg = 0;
        for (std::size_t j = 0; j < netz.knoten.size(); ++j) {
            if (j == i) { continue; }
            const WegKnoten& n = netz.knoten[j];
            const auto schl = std::make_pair(std::min<int>(static_cast<int>(i), static_cast<int>(j)),
                                             std::max<int>(static_cast<int>(i), static_cast<int>(j)));
            if (da.count(schl) != 0U) { continue; }
            if (std::fabs(n.ort[2] - at.ort[2]) > kMaxEtage) { continue; }
            const float d = abstand(n.ort, at.ort);
            if (d > kMaxAutoLaenge) { continue; }
            if (n.art != WegKnoten::Art::Wegpunkt) {
                ++nichtWeg;
                if (nichtWeg > kMaxNichtWeg || d > kMaxAutoLaenge / 2.0F) { continue; }
            }
            kandidaten.emplace_back(d, static_cast<int>(j));
        }
        std::sort(kandidaten.begin(), kandidaten.end());
        int genommen = 0;
        for (const auto& [d, j] : kandidaten) {
            if (genommen >= kNachbarn) { break; }
            if (sicht && !sicht(at.ort, netz.knoten[static_cast<std::size_t>(j)].ort)) { continue; }
            verbinde(static_cast<int>(i), j, false, false);
            ++genommen;
        }
    }
    (void)designerKanten;
    return netz;
}

// --- Erzeugen -------------------------------------------------------------------
//
// rc609 W3a: Mike (03.11.) - auf Jedi Temple Garden fehlten alle Innenraeume, Treppen und Rampen bekamen keine Punkte.
// Gemessen (Selbsttest wegerzeugung): 0 von 1138 zu Fuss erreichbaren Bodenproben in den drei Raeumen hinter den Tueren
// abgedeckt. Ursachen: das alte Fluten ging in festen Schritten der Rasterweite (192) und kam nur durch eine Tuer, wenn
// eine Rasterlinie genau hindurchfuehrte (ein Durchgang ist schmaler als das Raster); es pruefte Strahlen gegen die
// gezeichneten Dreiecke statt der Kollision des Spiels (Tueren und Clip gab es fuer es nicht, Flaechen ohne Koerper
// waren Waende), den Hoehenunterschied als einen Sprung von hoechstens 60 je Rasterschritt (keine Treppe, die zwischen
// zwei Rasterpunkten mehr steigt) und keine Kante mit der Kantenpruefung des Spiels.
//
// Jetzt in drei Schritten, alles mit der Kollision des Spiels (WegWelt::spur):
//   1. Laufgitter: von den Startpunkten aus in Schritten der Kastenbreite einer Figur (2 x DEFAULT_MAXS_0 = 32) zu Fuss
//      wie Pmove - je Bild runSpeed 300 x 50 ms = 15 Einheiten (NPC_stats.cpp:1997, NPC_Think alle FRAMETIME/2), dabei
//      Stufen bis STEPSIZE 18 hinauf und hinab wie PM_StepSlideMove (bg_slidemove.cpp), Boden bis MIN_WALK_NORMAL 0,7,
//      sonst zu steil oder ein Sturz. So kommt das Gitter durch jede Tuer, durch die eine Figur passt.
//   2. Punkte: die Startpunkte, die Rasterpunkte (Rasterweite `abstand`) auf dem Laufgitter, dann jede Stelle, die zu
//      Fuss weiter als eine Rasterweite vom naechsten Punkt liegt. Nur, wo das Spiel den Wegpunkt behaelt (WegWelt::knoten).
//   3. Kanten: zwischen Punkten, die auf dem Laufgitter hoechstens zwei Rasterweiten auseinander liegen, wenn eine Figur
//      die gerade Linie gehen kann (wie oben) und die Kantenpruefung des Spiels sie behaelt (WegWelt::kante). Eine
//      Kante, die fast genau ueber einen gemeinsamen Nachbarn laeuft, entfaellt (die vier target-Felder reichen sonst nicht).
//   4. Ketten: Steigt eine Treppe mehr als eine Stufe und verbindet das Netz ihren Fuss nicht schon ueber bestandene
//      Kanten mit ihrem Kopf, kommt eine Kette von Punkten ueber die Treppe. Ein Teil des Netzes, der nicht am Teil des
//      ersten Startpunkts haengt (ein Raum hinter einer Tuer, durch die keine Rasterkante gerade hindurchgeht), bekommt
//      eine Kette auf dem kuerzesten Weg des Laufgitters, also in der Flucht der Tuer. Jede Kante der Kette wie in 3.
namespace {

constexpr float kKastenBreite = 16.0F;      // DEFAULT_MAXS_0 (bg_public.h), mins -16
constexpr float kKastenUnten = -24.0F;      // DEFAULT_MINS_2
constexpr float kKastenOben = 40.0F;        // DEFAULT_MAXS_2
constexpr float kStufe = 18.0F;             // STEPSIZE
constexpr float kBodenNormale = 0.7F;       // MIN_WALK_NORMAL
constexpr float kBildWeg = 300.0F * 0.05F;  // runSpeed 300 (NPC_stats.cpp:1997) x 50 ms (NPC_Think: FRAMETIME/2)
constexpr float kGitter = 2.0F * kKastenBreite;

using V = std::array<float, 3>;

struct Laeufer {
    const WegWelt& w;
    V mins{-kKastenBreite, -kKastenBreite, kKastenUnten};
    V maxs{kKastenBreite, kKastenBreite, kKastenOben};

    // PM_GroundTrace nach PM_StepSlideMove ("push down the final amount"): eine Stufe tief Boden, nicht zu steil.
    bool aufsetzen(V& o) const {
        const WegSpurTreffer g = w.spur(o, mins, maxs, {o[0], o[1], o[2] - kStufe});
        if (g.imFesten || g.anteil >= 1.0F || g.normale[2] < kBodenNormale) { return false; }
        o = g.ende;
        return true;
    }
    // Ein Bild: von o waagerecht nach (x, y); erst gerade (PM_SlideMove), sonst eine Stufe hoch, vor, hinab.
    bool bild(V& o, float x, float y) const {
        const V ziel{x, y, o[2]};
        WegSpurTreffer t = w.spur(o, mins, maxs, ziel);
        V neu{};
        if (!t.imFesten && t.anteil >= 1.0F) {
            neu = ziel;
        } else {
            const WegSpurTreffer hoch = w.spur(o, mins, maxs, {o[0], o[1], o[2] + kStufe});
            if (hoch.imFesten || hoch.anteil <= 0.0F) { return false; }   // "can't step up"
            const V oben = hoch.ende;
            t = w.spur(oben, mins, maxs, {x, y, oben[2]});
            if (t.imFesten || t.anteil < 1.0F) { return false; }
            neu = {x, y, oben[2]};
        }
        if (!aufsetzen(neu)) { return false; }
        o = neu;
        return true;
    }
    // Von o (Kastenursprung auf dem Boden) die gerade Linie bis (x, y), Bild fuer Bild.
    bool gehen(V& o, float x, float y) const {
        for (int runde = 0; runde < 4096; ++runde) {
            const float dx = x - o[0];
            const float dy = y - o[1];
            const float d = std::hypot(dx, dy);
            if (d <= kBildWeg) { return bild(o, x, y); }
            if (!bild(o, o[0] + dx / d * kBildWeg, o[1] + dy / d * kBildWeg)) { return false; }
        }
        return false;
    }
    // Der Boden unter einem Startpunkt (bis 512 tief), Kasten frei. Der Kasten beginnt 25 ueber dem Startpunkt - so
    // passt ein Punkt auf dem Boden ebenso wie ein origin (der eines Wegpunkts liegt 25 ueber seinem Boden).
    bool boden(const V& von, V& o) const {
        const V oben{von[0], von[1], von[2] + 25.0F};
        const WegSpurTreffer t = w.spur(oben, mins, maxs, {oben[0], oben[1], oben[2] - 512.0F});
        if (t.imFesten || t.anteil >= 1.0F || t.normale[2] < kBodenNormale) { return false; }
        o = t.ende;
        return true;
    }
};

struct Zelle {
    V o{};                 // Kastenursprung (Boden = o[2] + kKastenUnten)
    int ix = 0;
    int iy = 0;
    bool gitter = true;    // auf dem Laufgitter (sonst ein Startpunkt)
    std::vector<int> nb;   // zu Fuss erreichbare Nachbarn
};

long long spalteSchluessel(int ix, int iy) {
    return static_cast<long long>((static_cast<unsigned long long>(static_cast<unsigned int>(ix)) << 32U) |
                                  static_cast<unsigned long long>(static_cast<unsigned int>(iy)));
}

float waagerecht(const V& a, const V& b) { return std::hypot(a[0] - b[0], a[1] - b[1]); }

}  // namespace

std::vector<NeuerWegpunkt> erzeugeWegpunkte(const std::vector<std::array<float, 3>>& starts, const WegWelt& welt,
                                            const WegErzeugung& p, WegErzeugungInfo* info) {
    std::vector<NeuerWegpunkt> aus;
    if (!welt.spur) { return aus; }
    const Laeufer lauf{welt};
    const float S0 = std::max(kGitter * 2.0F, p.abstand);
    constexpr float kNie = std::numeric_limits<float>::max();
    // Mehr Flaeche, als maxKnoten Punkte bei der dreifachen Rasterweite abdecken (je Punkt 3k x 3k Zellen), braucht das
    // Gitter nicht (weiter wird das Raster nicht, siehe unten).
    const long long k2 = 3LL * std::max(1L, std::lround(S0 / kGitter));
    const long long maxZellen = static_cast<long long>(std::max(1, p.maxKnoten)) * k2 * k2;

    // --- 1. Laufgitter ---------------------------------------------------------------
    std::vector<Zelle> z;
    std::unordered_map<long long, std::vector<int>> spalten;
    std::deque<int> offen;
    const auto verknuepfe = [&](int a, int b) {
        if (a == b) { return; }
        auto& na = z[static_cast<std::size_t>(a)].nb;
        if (std::find(na.begin(), na.end(), b) != na.end()) { return; }
        na.push_back(b);
        z[static_cast<std::size_t>(b)].nb.push_back(a);
    };
    // Die Zelle des Gitters an (ix, iy) auf dieser Hoehe (weniger als eine Stufe auseinander), sonst eine neue.
    const auto zelleAn = [&](int ix, int iy, const V& o) {
        std::vector<int>& sp = spalten[spalteSchluessel(ix, iy)];
        for (const int i : sp) {
            if (std::fabs(z[static_cast<std::size_t>(i)].o[2] - o[2]) < kStufe) { return i; }
        }
        if (static_cast<long long>(z.size()) >= maxZellen) { return -1; }
        Zelle c;
        c.o = o;
        c.ix = ix;
        c.iy = iy;
        z.push_back(std::move(c));
        const int i = static_cast<int>(z.size()) - 1;
        sp.push_back(i);
        offen.push_back(i);
        return i;
    };
    std::vector<int> startZelle(starts.size(), -1);
    for (std::size_t si = 0; si < starts.size(); ++si) {
        V o{};
        if (!lauf.boden(starts[si], o)) { continue; }
        Zelle c;
        c.o = o;
        c.gitter = false;
        z.push_back(std::move(c));
        const int i = static_cast<int>(z.size()) - 1;
        startZelle[si] = i;
        // Zu den vier umliegenden Gitterpunkten.
        const int x0 = static_cast<int>(std::floor(o[0] / kGitter));
        const int y0 = static_cast<int>(std::floor(o[1] / kGitter));
        for (int dx = 0; dx <= 1; ++dx) {
            for (int dy = 0; dy <= 1; ++dy) {
                V q = o;
                if (!lauf.gehen(q, static_cast<float>(x0 + dx) * kGitter, static_cast<float>(y0 + dy) * kGitter)) { continue; }
                const int j = zelleAn(x0 + dx, y0 + dy, q);
                if (j >= 0) { verknuepfe(i, j); }
            }
        }
    }
    static const int kVier[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    while (!offen.empty()) {
        const int i = offen.front();
        offen.pop_front();
        for (const auto& r : kVier) {
            const int ix = z[static_cast<std::size_t>(i)].ix + r[0];
            const int iy = z[static_cast<std::size_t>(i)].iy + r[1];
            V q = z[static_cast<std::size_t>(i)].o;
            if (!lauf.gehen(q, static_cast<float>(ix) * kGitter, static_cast<float>(iy) * kGitter)) { continue; }
            const int j = zelleAn(ix, iy, q);
            if (j >= 0) { verknuepfe(i, j); }
        }
    }

    // --- 2. Punkte ---------------------------------------------------------------------
    std::vector<int> knotenZelle;
    std::vector<int> knotenVon(z.size(), -1);
    const int maxK = std::max(0, p.maxKnoten);
    int grenze = std::numeric_limits<int>::max();
    // Der origin eines Punkts in Zelle c (Start: der Startpunkt selbst, wenn er ein Wegpunkt ist), wenn das Spiel ihn dort
    // behaelt.
    const auto ortFuer = [&](int c, int start, V& ort) {
        const V& o = z[static_cast<std::size_t>(c)].o;
        // origin = Boden + 25 (der Kasten -24..45 beginnt eine Einheit ueber dem Boden), gerundet wie in der .ent.
        ort = {std::round(o[0]), std::round(o[1]), std::round(o[2] + kKastenUnten + 25.0F)};
        if (start >= 0 && static_cast<std::size_t>(start) < p.startIstWegpunkt.size() && p.startIstWegpunkt[static_cast<std::size_t>(start)] != 0) {
            ort = starts[static_cast<std::size_t>(start)];
        }
        // Boden unter der Mitte: sonst markiert ihn das Spiel als schwebend ("Drop To Floor And Mark Floating",
        // 1,5 x MAX_EDGE_FLOOR_DIST tief, g_navigator.cpp:1573-1595 MD) und seine Kanten gelten nur fuer Flieger.
        if (welt.spur(ort, {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F}, {ort[0], ort[1], ort[2] - 90.0F}).anteil >= 1.0F) { return false; }
        return !welt.knoten || welt.knoten(ort);
    };
    const auto setze = [&](int c, int start) {
        if (knotenVon[static_cast<std::size_t>(c)] >= 0 || static_cast<int>(aus.size()) >= grenze) { return false; }
        V ort{};
        if (!ortFuer(c, start, ort)) { return false; }
        const bool fest = start >= 0 && static_cast<std::size_t>(start) < p.startIstWegpunkt.size() &&
                          p.startIstWegpunkt[static_cast<std::size_t>(start)] != 0;
        NeuerWegpunkt w;
        w.ort = ort;
        w.boden = fest ? V{ort[0], ort[1], z[static_cast<std::size_t>(c)].o[2] + kKastenUnten} : V{ort[0], ort[1], ort[2] - 25.0F};
        w.start = start;
        knotenVon[static_cast<std::size_t>(c)] = static_cast<int>(aus.size());
        knotenZelle.push_back(c);
        aus.push_back(std::move(w));
        return true;
    };
    // Treppen und Rampen: Zellen mit einem Nachbarn auf anderer Hoehe. Wo sie an Boden ohne solchen Nachbarn stossen,
    // bilden die Randzellen gleicher Hoehe einen Anschluss (die Seite einer freien Treppe stoesst an keinen solchen
    // Boden, dort ist jede Zelle auf anderer Hoehe). Je Treppe, die mehr als eine Stufe steigt, zaehlt jedes Paar ihrer
    // Anschluesse, die mehr als eine Stufe auseinander liegen (je die Zelle in ihrer Mitte): unten und oben, ein Absatz.
    struct Treppe {
        int unten = -1;
        int oben = -1;
    };
    std::vector<Treppe> treppen;
    {
        std::vector<char> anTreppe(z.size(), 0);
        for (std::size_t i = 0; i < z.size(); ++i) {
            for (const int j : z[i].nb) {
                if (std::fabs(z[static_cast<std::size_t>(j)].o[2] - z[i].o[2]) > 1.0F) { anTreppe[i] = 1; }
            }
        }
        std::vector<int> teil(z.size(), -1);
        std::vector<float> spanne;
        for (std::size_t i0 = 0; i0 < z.size(); ++i0) {
            if (anTreppe[i0] == 0 || teil[i0] >= 0) { continue; }
            const int t = static_cast<int>(spanne.size());
            float lo = z[i0].o[2];
            float hi = lo;
            std::vector<int> st{static_cast<int>(i0)};
            teil[i0] = t;
            while (!st.empty()) {
                const int i = st.back();
                st.pop_back();
                lo = std::min(lo, z[static_cast<std::size_t>(i)].o[2]);
                hi = std::max(hi, z[static_cast<std::size_t>(i)].o[2]);
                for (const int j : z[static_cast<std::size_t>(i)].nb) {
                    if (anTreppe[static_cast<std::size_t>(j)] != 0 && teil[static_cast<std::size_t>(j)] < 0) {
                        teil[static_cast<std::size_t>(j)] = t;
                        st.push_back(j);
                    }
                }
            }
            spanne.push_back(hi - lo);
        }
        const auto amRand = [&](int i) {
            if (teil[static_cast<std::size_t>(i)] < 0 || spanne[static_cast<std::size_t>(teil[static_cast<std::size_t>(i)])] <= kStufe) { return false; }
            for (const int j : z[static_cast<std::size_t>(i)].nb) {
                if (anTreppe[static_cast<std::size_t>(j)] == 0) { return true; }
            }
            return false;
        };
        std::vector<std::vector<int>> mitten(spanne.size());   // je Treppe die Mittelzellen ihrer Anschluesse
        std::vector<char> gesehen(z.size(), 0);
        for (std::size_t i0 = 0; i0 < z.size(); ++i0) {
            if (gesehen[i0] != 0 || !amRand(static_cast<int>(i0))) { continue; }
            std::vector<int> gruppe;
            std::vector<int> st{static_cast<int>(i0)};
            gesehen[i0] = 1;
            while (!st.empty()) {
                const int i = st.back();
                st.pop_back();
                gruppe.push_back(i);
                for (const int j : z[static_cast<std::size_t>(i)].nb) {
                    if (gesehen[static_cast<std::size_t>(j)] == 0 && amRand(j) &&
                        std::fabs(z[static_cast<std::size_t>(j)].o[2] - z[static_cast<std::size_t>(i)].o[2]) <= 1.0F) {
                        gesehen[static_cast<std::size_t>(j)] = 1;
                        st.push_back(j);
                    }
                }
            }
            float mx = 0.0F;
            float my = 0.0F;
            for (const int i : gruppe) {
                mx += z[static_cast<std::size_t>(i)].o[0];
                my += z[static_cast<std::size_t>(i)].o[1];
            }
            const V mitte{mx / static_cast<float>(gruppe.size()), my / static_cast<float>(gruppe.size()), 0.0F};
            int best = gruppe.front();
            for (const int i : gruppe) {
                const float di = waagerecht(z[static_cast<std::size_t>(i)].o, mitte);
                const float db = waagerecht(z[static_cast<std::size_t>(best)].o, mitte);
                if (di < db || (di == db && i < best)) { best = i; }
            }
            mitten[static_cast<std::size_t>(teil[static_cast<std::size_t>(best)])].push_back(best);
        }
        for (const std::vector<int>& m : mitten) {
            for (std::size_t a = 0; a < m.size(); ++a) {
                for (std::size_t b = 0; b < m.size(); ++b) {
                    const V& oa = z[static_cast<std::size_t>(m[a])].o;
                    const V& ob = z[static_cast<std::size_t>(m[b])].o;
                    if (ob[2] - oa[2] > kStufe) { treppen.push_back(Treppe{m[a], m[b]}); }
                }
            }
        }
    }
    // Weg auf dem Laufgitter zum naechsten Punkt (bis `bis`).
    std::vector<float> weit(z.size(), kNie);
    const auto ausbreiten = [&](int c, float bis) {
        using E = std::pair<float, int>;
        std::priority_queue<E, std::vector<E>, std::greater<>> q;
        weit[static_cast<std::size_t>(c)] = 0.0F;
        q.emplace(0.0F, c);
        while (!q.empty()) {
            const auto [d, i] = q.top();
            q.pop();
            if (d > weit[static_cast<std::size_t>(i)]) { continue; }
            for (const int j : z[static_cast<std::size_t>(i)].nb) {
                const float nd = d + waagerecht(z[static_cast<std::size_t>(i)].o, z[static_cast<std::size_t>(j)].o);
                if (nd <= bis && nd < weit[static_cast<std::size_t>(j)]) {
                    weit[static_cast<std::size_t>(j)] = nd;
                    q.emplace(nd, j);
                }
            }
        }
    };
    const auto punkte = [&](float raster) {
        aus.clear();
        knotenZelle.clear();
        std::fill(knotenVon.begin(), knotenVon.end(), -1);
        std::fill(weit.begin(), weit.end(), kNie);
        // a) die Startpunkte selbst (ein vorhandener Wegpunkt bleibt dieser Wegpunkt)
        for (std::size_t si = 0; si < starts.size(); ++si) {
            if (startZelle[si] >= 0) { (void)setze(startZelle[si], static_cast<int>(si)); }
        }
        // b) die Rasterpunkte, in der Reihenfolge des Flutens (nahe den Startpunkten zuerst)
        const int kr = std::max(1, static_cast<int>(std::lround(raster / kGitter)));
        const std::size_t vorRaster = knotenZelle.size();
        for (std::size_t i = 0; i < z.size(); ++i) {
            const Zelle& c = z[i];
            if (!c.gitter || ((c.ix % kr) + kr) % kr != 0 || ((c.iy % kr) + kr) % kr != 0) { continue; }
            bool nah = false;   // schon ein Startpunkt dicht daneben
            for (std::size_t n = 0; n < vorRaster && !nah; ++n) {
                const V& o = z[static_cast<std::size_t>(knotenZelle[n])].o;
                nah = waagerecht(o, c.o) < raster / 3.0F && std::fabs(o[2] - c.o[2]) <= kStufe;
            }
            if (!nah) { (void)setze(static_cast<int>(i), -1); }
        }
        for (const int c : knotenZelle) { ausbreiten(c, raster); }
        // c) was zu Fuss weiter als eine Rasterweite vom naechsten Punkt liegt (Durchgaenge, Raeume neben dem Raster)
        for (std::size_t i = 0; i < z.size() && static_cast<int>(aus.size()) < grenze; ++i) {
            if (weit[i] <= raster) { continue; }
            if (setze(static_cast<int>(i), -1)) { ausbreiten(static_cast<int>(i), raster); }
        }
    };

    // --- 3. Kanten ---------------------------------------------------------------------
    const auto laenge = [&](int a, int b) {
        const V& oa = aus[static_cast<std::size_t>(a)].ort;
        const V& ob = aus[static_cast<std::size_t>(b)].ort;
        return std::hypot(waagerecht(oa, ob), oa[2] - ob[2]);
    };
    // Eine Kante zwischen den Zellen ca und cb mit diesen origins: eine Figur geht die gerade Linie, und die
    // Kantenpruefung des Spiels behaelt sie in beide Richtungen - das target-Feld kommt spaeter an einen der beiden
    // (verteileZiele), und das Spiel spurt vom Traeger zum Ziel (CWayEdge mNodeA -> mNodeB).
    // 0: geht, 1: keine gerade Linie zu Fuss, 2: die Kantenpruefung des Spiels verwirft sie.
    const auto kanteGrund = [&](int ca, const V& oa, int cb, const V& ob) {
        V o = z[static_cast<std::size_t>(ca)].o;
        const V& ziel = z[static_cast<std::size_t>(cb)].o;
        if (!lauf.gehen(o, ziel[0], ziel[1]) || std::fabs(o[2] - ziel[2]) > 1.0F) { return 1; }
        return (!welt.kante || (welt.kante(oa, ob) && welt.kante(ob, oa))) ? 0 : 2;
    };
    const auto kanteGeht = [&](int ca, const V& oa, int cb, const V& ob) { return kanteGrund(ca, oa, cb, ob) == 0; };
    std::vector<WegErzeugungInfo::Luecke> luecken;
    const auto verbinde = [&](int a, int b) {
        aus[static_cast<std::size_t>(a)].nachbarn.push_back(b);
        aus[static_cast<std::size_t>(b)].nachbarn.push_back(a);
    };
    // Die Punkte, die von a aus auf dem Laufgitter hoechstens `bis` entfernt liegen (Weg, Punkt).
    std::vector<float> dist(z.size(), kNie);
    std::vector<int> beruehrt;
    const auto umkreis = [&](int c0, float bis, const std::function<bool(int zelle, float d)>& halt) {
        for (const int i : beruehrt) { dist[static_cast<std::size_t>(i)] = kNie; }
        beruehrt.clear();
        using E = std::pair<float, int>;
        std::priority_queue<E, std::vector<E>, std::greater<>> q;
        dist[static_cast<std::size_t>(c0)] = 0.0F;
        beruehrt.push_back(c0);
        q.emplace(0.0F, c0);
        while (!q.empty()) {
            const auto [d, i] = q.top();
            q.pop();
            if (d > dist[static_cast<std::size_t>(i)]) { continue; }
            if (halt(i, d)) { return i; }
            for (const int j : z[static_cast<std::size_t>(i)].nb) {
                const float nd = d + waagerecht(z[static_cast<std::size_t>(i)].o, z[static_cast<std::size_t>(j)].o);
                if (nd <= bis && nd < dist[static_cast<std::size_t>(j)]) {
                    if (dist[static_cast<std::size_t>(j)] == kNie) { beruehrt.push_back(j); }
                    dist[static_cast<std::size_t>(j)] = nd;
                    q.emplace(nd, j);
                }
            }
        }
        return -1;
    };
    const auto kanten = [&](float raster) {
        struct Kandidat {
            float d;
            int a;
            int b;
        };
        std::vector<Kandidat> kand;
        for (std::size_t a = 0; a < aus.size(); ++a) {
            (void)umkreis(knotenZelle[a], 2.0F * raster, [&](int i, float) {
                const int b = knotenVon[static_cast<std::size_t>(i)];
                if (b > static_cast<int>(a)) { kand.push_back(Kandidat{laenge(static_cast<int>(a), b), static_cast<int>(a), b}); }
                return false;
            });
        }
        std::sort(kand.begin(), kand.end(), [](const Kandidat& x, const Kandidat& y) {
            return x.d != y.d ? x.d < y.d : (x.a != y.a ? x.a < y.a : x.b < y.b);
        });
        for (const Kandidat& kd : kand) {
            const auto& na = aus[static_cast<std::size_t>(kd.a)].nachbarn;
            const auto& nbB = aus[static_cast<std::size_t>(kd.b)].nachbarn;
            // Fast genau ueber einen gemeinsamen Nachbarn (Umweg hoechstens ein Fuenftel laenger): entfaellt.
            bool umweg = false;
            for (const int c : na) {
                umweg = umweg || (std::find(nbB.begin(), nbB.end(), c) != nbB.end() && laenge(kd.a, c) + laenge(c, kd.b) <= kd.d * 1.2F);
            }
            if (umweg) { continue; }
            if (kanteGeht(knotenZelle[static_cast<std::size_t>(kd.a)], aus[static_cast<std::size_t>(kd.a)].ort,
                          knotenZelle[static_cast<std::size_t>(kd.b)], aus[static_cast<std::size_t>(kd.b)].ort)) {
                verbinde(kd.a, kd.b);
            }
        }
    };
    // Eine Kette von Punkten vom Punkt in Zelle `von` zur Zelle `ziel`: je Schritt unter den Zellen bis zwei Rasterweiten
    // Laufweg (wie in 3) die, die auf dem Laufgitter dem Ziel am naechsten liegt und zu der die Kante geht (dort ein
    // Punkt, falls keiner da ist). So weicht die Kette seitlich aus, wo eine Tuer nur einen schmalen Spalt offen laesst
    // und dicht an der geschlossenen Tuer kein Punkt stehen kann (G_CheckInSolid und waypoint_getRadius beim Spawnen).
    std::vector<float> zielWeit(z.size(), kNie);
    const auto kette = [&](int von, int ziel, float raster) {
        if (knotenVon[static_cast<std::size_t>(von)] < 0) { return false; }
        // Weg auf dem Laufgitter zum Ziel
        std::fill(zielWeit.begin(), zielWeit.end(), kNie);
        {
            using E = std::pair<float, int>;
            std::priority_queue<E, std::vector<E>, std::greater<>> q;
            zielWeit[static_cast<std::size_t>(ziel)] = 0.0F;
            q.emplace(0.0F, ziel);
            while (!q.empty()) {
                const auto [d, i] = q.top();
                q.pop();
                if (d > zielWeit[static_cast<std::size_t>(i)]) { continue; }
                for (const int j : z[static_cast<std::size_t>(i)].nb) {
                    const float nd = d + waagerecht(z[static_cast<std::size_t>(i)].o, z[static_cast<std::size_t>(j)].o);
                    if (nd < zielWeit[static_cast<std::size_t>(j)]) {
                        zielWeit[static_cast<std::size_t>(j)] = nd;
                        q.emplace(nd, j);
                    }
                }
            }
        }
        int anker = von;
        for (int schritt = 0; schritt < 4096 && anker != ziel; ++schritt) {
            const int a = knotenVon[static_cast<std::size_t>(anker)];
            std::vector<std::pair<float, int>> wahl;
            (void)umkreis(anker, 2.0F * raster, [&](int i, float) {
                if (zielWeit[static_cast<std::size_t>(i)] < zielWeit[static_cast<std::size_t>(anker)]) { wahl.emplace_back(zielWeit[static_cast<std::size_t>(i)], i); }
                return false;
            });
            std::sort(wahl.begin(), wahl.end());
            WegErzeugungInfo::Luecke l;
            l.anker = aus[static_cast<std::size_t>(a)].ort;
            l.weiter = wahl.empty() ? z[static_cast<std::size_t>(anker)].o : z[static_cast<std::size_t>(wahl.front().second)].o;
            int neu = -1;
            for (const auto& [d, c] : wahl) {
                (void)d;
                V ort{};
                const int b = knotenVon[static_cast<std::size_t>(c)];
                if (b >= 0) {
                    ort = aus[static_cast<std::size_t>(b)].ort;
                } else if (!ortFuer(c, -1, ort)) {
                    ++l.ohnePunkt;
                    continue;
                }
                const int grund = kanteGrund(anker, aus[static_cast<std::size_t>(a)].ort, c, ort);
                if (grund != 0) {
                    ++(grund == 1 ? l.ohneGehen : l.ohneKante);
                    continue;
                }
                if (b < 0 && !setze(c, -1)) { continue; }
                const int bb = knotenVon[static_cast<std::size_t>(c)];
                const auto& na = aus[static_cast<std::size_t>(a)].nachbarn;
                if (std::find(na.begin(), na.end(), bb) == na.end()) { verbinde(a, bb); }
                neu = c;
                break;
            }
            if (neu < 0) {
                luecken.push_back(l);
                return false;
            }
            anker = neu;
        }
        return anker == ziel;
    };
    std::vector<char> abgehaengt;   // Punkte eines Teils, der sich nicht verbinden liess
    // Teile des Netzes (Zusammenhang ueber die Kanten).
    const auto teile = [&]() {
        std::vector<int> t(aus.size(), -1);
        int zahl = 0;
        for (std::size_t s0 = 0; s0 < aus.size(); ++s0) {
            if (t[s0] >= 0) { continue; }
            std::vector<int> st{static_cast<int>(s0)};
            t[s0] = zahl;
            while (!st.empty()) {
                const int i = st.back();
                st.pop_back();
                for (const int j : aus[static_cast<std::size_t>(i)].nachbarn) {
                    if (t[static_cast<std::size_t>(j)] < 0) {
                        t[static_cast<std::size_t>(j)] = zahl;
                        st.push_back(j);
                    }
                }
            }
            ++zahl;
        }
        return t;
    };
    const auto ketten = [&](float raster) {
        // a) Treppen: oben und unten ein Punkt, die beiden ueber das Netz in so vielen Kanten verbunden, wie die
        //    Rasterweite auf dem Weg zwischen ihnen verlangt. Sonst eine Kette ueber die Treppe.
        for (const Treppe& t : treppen) {
            // Nur Anschluesse, zwischen denen Kanten in Frage kommen (zwei Rasterweiten, wie in 3).
            const float lauf = waagerecht(z[static_cast<std::size_t>(t.unten)].o, z[static_cast<std::size_t>(t.oben)].o);
            if (lauf > 2.0F * raster) { continue; }
            // Der naechste Punkt auf der Hoehe des Anschlusses, hoechstens eine halbe Rasterweite entfernt.
            const auto punktBei = [&](int zelle) {
                int k = -1;
                (void)umkreis(zelle, raster * 0.5F, [&](int i, float) {
                    const int b = knotenVon[static_cast<std::size_t>(i)];
                    if (b < 0 || std::fabs(z[static_cast<std::size_t>(i)].o[2] - z[static_cast<std::size_t>(zelle)].o[2]) > kStufe) { return false; }
                    k = b;
                    return true;
                });
                return k;
            };
            const int nu = punktBei(t.unten);
            const int no = punktBei(t.oben);
            const int vonZelle = nu >= 0 ? knotenZelle[static_cast<std::size_t>(nu)] : t.unten;
            const int bisZelle = no >= 0 ? knotenZelle[static_cast<std::size_t>(no)] : t.oben;
            if (umkreis(vonZelle, 4.0F * raster, [&](int i, float) { return i == bisZelle; }) < 0) { continue; }
            if (nu >= 0 && no >= 0) {
                // so viele Kanten, wie die Rasterweite auf dem waagerechten Abstand verlangt, und eine
                const int grenzeKanten = 1 + static_cast<int>(std::ceil(lauf / raster));
                std::vector<int> tiefe(aus.size(), -1);
                std::deque<int> q{nu};
                tiefe[static_cast<std::size_t>(nu)] = 0;
                while (!q.empty() && tiefe[static_cast<std::size_t>(no)] < 0) {
                    const int i = q.front();
                    q.pop_front();
                    if (tiefe[static_cast<std::size_t>(i)] >= grenzeKanten) { continue; }
                    for (const int j : aus[static_cast<std::size_t>(i)].nachbarn) {
                        if (tiefe[static_cast<std::size_t>(j)] < 0) {
                            tiefe[static_cast<std::size_t>(j)] = tiefe[static_cast<std::size_t>(i)] + 1;
                            q.push_back(j);
                        }
                    }
                }
                if (tiefe[static_cast<std::size_t>(no)] >= 0) { continue; }   // die Kantenpruefung besteht schon einen Weg
            }
            if (knotenVon[static_cast<std::size_t>(vonZelle)] < 0 && !setze(vonZelle, -1)) { continue; }
            (void)kette(vonZelle, bisZelle, raster);
        }
        // b) Teile des Netzes ohne Verbindung zum Teil des ersten Startpunkts (ein Raum hinter einer Tuer, durch die keine
        //    Rasterkante gerade hindurchgeht): der kuerzeste Weg auf dem Laufgitter zum Hauptteil, darauf eine Kette.
        for (int runde = 0; runde < static_cast<int>(aus.size()); ++runde) {
            const std::vector<int> t = teile();
            if (aus.empty()) { return; }
            abgehaengt.resize(aus.size(), 0);
            const int haupt = t.front();
            int fremd = -1;
            for (std::size_t k = 0; k < aus.size() && fremd < 0; ++k) {
                if (t[k] != haupt && abgehaengt[k] == 0) { fremd = t[k]; }
            }
            if (fremd < 0) { return; }
            bool verbunden = false;
            for (std::size_t k = 0; k < aus.size() && !verbunden; ++k) {
                if (t[k] != fremd) { continue; }
                const int ziel = umkreis(knotenZelle[k], kNie, [&](int i, float) {
                    const int b = knotenVon[static_cast<std::size_t>(i)];
                    return b >= 0 && t[static_cast<std::size_t>(b)] == haupt;
                });
                if (ziel >= 0) { verbunden = kette(knotenZelle[k], ziel, raster); }
                if (!verbunden) {
                    // Nicht zu verbinden (zu Fuss erreichbar, aber keine Kante besteht): der ganze Teil, auch mit den
                    // Punkten, die die Kette dazu gesetzt hat, kommt nicht noch einmal dran.
                    const std::vector<int> t2 = teile();
                    abgehaengt.resize(aus.size(), 0);
                    for (std::size_t j = 0; j < aus.size(); ++j) {
                        if (t2[j] == t2[k]) { abgehaengt[j] = 1; }
                    }
                }
                break;   // ein Versuch je Teil: vom ersten Punkt des Teils aus
            }
        }
    };
    const auto alles = [&](float raster) {
        abgehaengt.clear();
        luecken.clear();
        punkte(raster);
        kanten(raster);
        ketten(raster);
    };
    // Das Spiel nimmt hoechstens maxKnoten Punkte ("Max Nav points reached", NUM_NODES). Braucht das Raster mehr, wird
    // es weiter (die Flaeche je Punkt im Verhaeltnis), bis zur dreifachen Rasterweite; dann gilt die Grenze in der
    // Reihenfolge des Flutens.
    float S = S0;
    alles(S);
    for (int runde = 0; runde < 8 && static_cast<int>(aus.size()) > maxK && S < 3.0F * S0; ++runde) {
        S = std::min(3.0F * S0, S * std::sqrt(static_cast<float>(aus.size()) / static_cast<float>(std::max(1, maxK))));
        alles(S);
    }
    if (static_cast<int>(aus.size()) > maxK) {
        grenze = maxK;
        alles(S);
    }
    if (info != nullptr) {
        info->abstand = S;
        info->luecken = luecken;
    }
    return aus;
}

// rc609 W3a: Jede Kante braucht ein target-Feld an einem ihrer Enden (das Spiel verbindet in beide Richtungen,
// connect_node, g_navigator.cpp:1652-1684 MD), jeder Punkt hat vier. Vorher bekam jede Kante das freiere Ende und fiel
// weg, wenn beide voll waren - so fiel auf jeditemple_garden auch die einzige Kante durch eine Tuer weg, und der Raum
// dahinter hing nicht mehr am Netz. Jetzt zuerst ein aufspannender Wald (jeder Punkt traegt die Kante zu dem, von dem
// aus er erreicht wurde: ein Feld je Punkt - so bleibt alles verbunden), dann die uebrigen Kanten, gerade vor
// diagonalen; ist ein Ende voll, wandert eine seiner Kanten ueber eine Kette zu einem Punkt mit freiem Feld.
std::vector<std::vector<int>> verteileZiele(const std::vector<NeuerWegpunkt>& wp, int felder) {
    const std::size_t n = wp.size();
    std::vector<std::vector<int>> ziele(n);
    if (felder <= 0) { return ziele; }
    const auto voll = [&](int i) { return static_cast<int>(ziele[static_cast<std::size_t>(i)].size()) >= felder; };
    std::set<std::pair<int, int>> vergeben;   // (min, max)
    const auto traegt = [&](int halter, int ziel) {
        ziele[static_cast<std::size_t>(halter)].push_back(ziel);
        vergeben.insert({std::min(halter, ziel), std::max(halter, ziel)});
    };
    // 1. aufspannender Wald in Breitensuche (Startreihenfolge der Punkte)
    std::vector<char> gesehen(n, 0);
    for (std::size_t s0 = 0; s0 < n; ++s0) {
        if (gesehen[s0] != 0) { continue; }
        gesehen[s0] = 1;
        std::deque<int> q{static_cast<int>(s0)};
        while (!q.empty()) {
            const int i = q.front();
            q.pop_front();
            for (const int j : wp[static_cast<std::size_t>(i)].nachbarn) {
                if (gesehen[static_cast<std::size_t>(j)] != 0) { continue; }
                gesehen[static_cast<std::size_t>(j)] = 1;
                traegt(j, i);   // genau ein Feld je Punkt
                q.push_back(j);
            }
        }
    }
    // Ein Feld an `u` freimachen: eine Kette u -> v1 -> ... bis zu einem Punkt mit freiem Feld; jede Kante der Kette
    // wechselt den Traeger (Breitensuche, hoechstens 256 Punkte).
    const auto freimachen = [&](int u) {
        std::map<int, std::pair<int, int>> von;   // Punkt -> (vorheriger Traeger, Index des Ziels dort)
        std::deque<int> q{u};
        von[u] = {-1, -1};
        while (!q.empty() && von.size() < 256U) {
            const int x = q.front();
            q.pop_front();
            const auto& zx = ziele[static_cast<std::size_t>(x)];
            for (std::size_t k = 0; k < zx.size(); ++k) {
                const int y = zx[k];
                if (von.count(y) != 0U) { continue; }
                von[y] = {x, static_cast<int>(k)};
                if (!voll(y)) {
                    // Kette umdrehen: von y zurueck bis u
                    int c = y;
                    while (von[c].first >= 0) {
                        const int p = von[c].first;
                        auto& zp = ziele[static_cast<std::size_t>(p)];
                        zp.erase(std::find(zp.begin(), zp.end(), c));
                        ziele[static_cast<std::size_t>(c)].push_back(p);
                        c = p;
                    }
                    return true;
                }
                q.push_back(y);
            }
        }
        return false;
    };
    // 2. die uebrigen Kanten: gerade zuerst, damit bei vollen Feldern eher Diagonalen wegfallen
    std::vector<std::tuple<int, int, int>> kanten;   // (diagonal, a, b)
    for (std::size_t a = 0; a < n; ++a) {
        for (const int b : wp[a].nachbarn) {
            if (static_cast<int>(a) < b && vergeben.count({static_cast<int>(a), b}) == 0U) {
                const auto& pa = wp[a].boden;
                const auto& pb = wp[static_cast<std::size_t>(b)].boden;
                const bool diag = std::fabs(pa[0] - pb[0]) > 1.0F && std::fabs(pa[1] - pb[1]) > 1.0F;
                kanten.emplace_back(diag ? 1 : 0, static_cast<int>(a), b);
            }
        }
    }
    std::stable_sort(kanten.begin(), kanten.end(),
                     [](const auto& x, const auto& y) { return std::get<0>(x) < std::get<0>(y); });
    for (const auto& [diag, a, b] : kanten) {
        (void)diag;
        const int halter = ziele[static_cast<std::size_t>(a)].size() <= ziele[static_cast<std::size_t>(b)].size() ? a : b;
        const int ziel = halter == a ? b : a;
        if (!voll(halter)) {
            traegt(halter, ziel);
        } else if (freimachen(halter)) {
            traegt(halter, ziel);
        } else if (freimachen(ziel)) {
            traegt(ziel, halter);
        }
    }
    return ziele;
}

// --- Pruefen -------------------------------------------------------------------

std::vector<WegBefund> pruefeWegNetz(const WegNetz& netz, const std::vector<MapEntity>& ents, const WegStrahl& strahl) {
    std::vector<WegBefund> aus;
    std::vector<int> grad(netz.knoten.size(), 0);
    for (const WegKante& k : netz.kanten) {
        ++grad[static_cast<std::size_t>(k.a)];
        ++grad[static_cast<std::size_t>(k.b)];
    }
    char z[200];
    for (std::size_t i = 0; i < netz.knoten.size(); ++i) {
        const WegKnoten& n = netz.knoten[i];
        const MapEntity& e = ents[static_cast<std::size_t>(n.entity)];
        const std::string nm = n.name.empty() ? std::string("(") + e.classname + ")" : n.name;
        if (grad[i] == 0 && n.art == WegKnoten::Art::Wegpunkt) {
            std::snprintf(z, sizeof(z), "%s: not connected to any other point", nm.c_str());
            aus.push_back(WegBefund{WegBefund::Art::Allein, static_cast<int>(i), z});
        }
        if (strahl && n.art != WegKnoten::Art::Kampf) {
            const int f = flags(e);
            std::array<float, 3> t{};
            std::array<float, 3> nn{};
            // Im Festen? Der Kasten reicht von -24 bis +45 (Kampfpunkte und
            // SOLID_OK prueft die Engine nicht).
            if ((f & 1) == 0) {
                const std::array<float, 3> unten{n.ort[0], n.ort[1], n.ort[2] - 22.0F};
                const std::array<float, 3> oben{n.ort[0], n.ort[1], n.ort[2] + 44.0F};
                bool fest = strahl(unten, oben, t, nn) || strahl(oben, unten, t, nn);
                // Ganz im Festen trifft eine kurze Strecke nichts. Ein langer
                // Strahl trifft dann eine Flaeche von INNEN: nach oben ihre
                // Oberseite (Normale nach oben), nach unten ihre Unterseite.
                if (!fest && strahl(n.ort, {n.ort[0], n.ort[1], n.ort[2] + 8192.0F}, t, nn) && nn[2] > 0.5F) {
                    fest = true;
                }
                if (!fest && strahl(n.ort, {n.ort[0], n.ort[1], n.ort[2] - 8192.0F}, t, nn) && nn[2] < -0.5F) {
                    fest = true;
                }
                for (int r = 0; r < 4 && !fest; ++r) {
                    const float dx = (r == 0) ? 19.0F : (r == 1) ? -19.0F : 0.0F;
                    const float dy = (r == 2) ? 19.0F : (r == 3) ? -19.0F : 0.0F;
                    fest = strahl(n.ort, {n.ort[0] + dx, n.ort[1] + dy, n.ort[2]}, t, nn);
                }
                if (fest) {
                    std::snprintf(z, sizeof(z), "%s: in solid - the game removes it (\"Waypoint in solid\")", nm.c_str());
                    aus.push_back(WegBefund{WegBefund::Art::ImFesten, static_cast<int>(i), z});
                }
            }
            // Schwebend? Die Engine sucht den Boden bis 90 darunter (1,5 x 60).
            if ((f & 2) == 0) {
                const std::array<float, 3> tief{n.ort[0], n.ort[1], n.ort[2] - kMaxEtage * 1.5F};
                if (!strahl(n.ort, tief, t, nn)) {
                    std::snprintf(z, sizeof(z), "%s: floating - no floor within 90 units below", nm.c_str());
                    aus.push_back(WegBefund{WegBefund::Art::Schwebend, static_cast<int>(i), z});
                }
            }
        }
    }
    for (const auto& [k, name] : netz.fehlendeZiele) {
        const WegKnoten& n = netz.knoten[static_cast<std::size_t>(k)];
        std::snprintf(z, sizeof(z), "%s: target \"%s\" does not exist", n.name.empty() ? "?" : n.name.c_str(),
                      name.c_str());
        aus.push_back(WegBefund{WegBefund::Art::ZielFehlt, k, z});
    }
    return aus;
}

}  // namespace bhed
