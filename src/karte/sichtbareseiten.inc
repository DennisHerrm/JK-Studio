// sichtbareseiten.inc - Liste reste2 Punkt 4: der sichtbare Teil der Brushseiten der Welt,
// so wie q3map2 ihn als Zeichenflaeche in die .bsp schreibt (side_t::visibleHull), und ob eine
// Flaeche ohne eigene Seite (Patch, misc_model) ein offenes Blatt beruehrt. Eingebunden in
// spielkollision.cpp (namenloser Namensraum: Ebene, V3f, normieren, normaleEinrasten,
// ebeneEinrasten, ebenenGleich, Seite, bevelsDazu, klein, die Flags von games.cpp).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114 mit den Aenderungen der Stufe 6 aus PR #316,
// third_party/q3map2/tools/quake3): Ablauf wie ProcessWorldModel (bsp.cpp) bis ClipSidesIntoTree:
//   map.cpp      PlaneEqual, CreateNewFloatPlane, FindFloatPlane (USE_HASHING), MapPlaneFromPoints,
//                ParseRawBrush (Detail-Bit der Flaeche), RemoveDuplicateBrushPlanes, SetBrushContents,
//                FinishBrush (opake Pinsel vorn, durchsichtige hinten), AdjustBrushesForOrigin,
//                MoveBrushesToWorld, ParseMapEntity (func_group/group_info fallen weg)
//   brush.cpp    CreateBrushWindings (HIGH_PRECISION_MATH_FIXES, OFFSET_WINDING_CREATION),
//                brushMinMaxFromPlanes, BoundBrush, SnapWeldVectorAccu, FixWindingAccu,
//                WindingIsTiny, BrushMostlyOnSide, SplitBrush, BrushVolume, FilterBrushIntoTree_r,
//                FilterStructuralBrushesIntoTree
//   facebsp.cpp  SelectSplitPlaneNum, BuildFaceTree_r, FaceBSP, MakeStructuralBSPFaceList
//   portals.cpp  MakeHeadnodePortals, BaseWindingForNode, MakeNodePortal, SplitNodePortals,
//                MakeTreePortals, FloodPortals, PlaceOccupant, FloodEntities, FillOutside
//   surface.cpp  SideInBrush, CullSides, ClipSideIntoTree_r, ClipSidesIntoTree,
//                FilterWindingIntoTree_r (fuer FilterPatchIntoTree / FilterTrianglesIntoTree)
//   polylib.cpp  BaseWindingForPlane(Accu), ChopWindingInPlace(Accu), ClipWindingEpsilon(Strict),
//                WindingOnPlaneSide, WindingArea, AddWindingToConvexHull
// mit denselben Zahlentypen (float-Wicklungen, double-Wicklungen in CreateBrushWindings) und
// Epsilons. Weggelassen, weil ohne Wirkung auf visibleHull: die Areaportal-Markierung der Portale
// (FloodPortals fragt sie nicht ab), skybox, Zaehler und Meldungen.
//
//   Copyright (C) 1999-2007 id Software, Inc. and contributors. GPL v2 (LICENSE).

namespace sicht {

using V3d = std::array<double, 3>;
using Wk = std::vector<V3f>;    // winding_t
using WkA = std::vector<V3d>;   // winding_accu_t

constexpr int kBlatt = -1;                 // PLANENUM_LEAF
constexpr double kOnEpsilon = 0.1;         // polylib.h ON_EPSILON
constexpr float kClipEpsilon = 0.1F;       // CLIP_EPSILON
constexpr float kWeltMin = -65536.0F;      // MIN_WORLD_COORD
constexpr float kWeltMax = 65536.0F;       // MAX_WORLD_COORD
constexpr int kEbenenHashes = 8192;        // PLANE_HASHES
constexpr double kDegeneriert = 0.1;       // DEGENERATE_EPSILON
constexpr double kSnapEpsilon = 0.01;      // SNAP_EPSILON
constexpr double kKante = 0.2;             // EDGE_LENGTH (WindingIsTiny)

enum { kVorn = 0, kHinten = 1, kAuf = 2, kQuer = 3 };   // eSideFront, eSideBack, eSideOn, eSideCross

// --- Vektoren wie libs/math (float bleibt float, gemischt rechnet double) ---------------
// plane3_distance_to_point( Plane3f, Vector3 ): vector3_dot in float, dann double
double abstand(const Ebene& e, const V3f& p) {
    return static_cast<double>(p[0] * e.n[0] + p[1] * e.n[1] + p[2] * e.n[2]) - static_cast<double>(e.d);
}
// plane3_distance_to_point( Plane3f, DoubleVector3 ): double * float
double abstand(const Ebene& e, const V3d& p) {
    return (p[0] * e.n[0] + p[1] * e.n[1] + p[2] * e.n[2]) - static_cast<double>(e.d);
}
struct EbeneD {   // Plane3
    V3d n{};
    double d = 0;
};
double abstand(const EbeneD& e, const V3d& p) { return (p[0] * e.n[0] + p[1] * e.n[1] + p[2] * e.n[2]) - e.d; }
Ebene umgedreht(const Ebene& e) {   // plane3_flipped
    Ebene r;
    for (int i = 0; i < 3; ++i) { r.n[i] = -e.n[i]; }
    r.d = -e.d;
    return r;
}
V3f kreuz(const V3f& a, const V3f& b) {   // vector3_cross
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
double laenge(const V3f& v) {   // vector3_length: sqrt( vector3_dot( v, v ) ), das Punktprodukt in float
    return std::sqrt(static_cast<double>(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
}
V3f minus(const V3f& a, const V3f& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
V3f normiert(const V3f& v) {   // VectorNormalized (HIGH_PRECISION: Laenge in double)
    V3f r = v;
    float t[3] = {r[0], r[1], r[2]};
    (void)normieren(t);
    return {t[0], t[1], t[2]};
}

struct MinMax {   // MinMax (float)
    V3f mins{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    V3f maxs{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
    [[nodiscard]] bool gueltig() const { return mins[0] < maxs[0] && mins[1] < maxs[1] && mins[2] < maxs[2]; }
    void dazu(const V3f& p) {
        for (std::size_t i = 0; i < 3; ++i) {
            if (p[i] < mins[i]) { mins[i] = p[i]; }
            if (p[i] > maxs[i]) { maxs[i] = p[i]; }
        }
    }
    [[nodiscard]] bool trifft(const MinMax& o) const {   // test( MinMax )
        return o.maxs[0] >= mins[0] && o.maxs[1] >= mins[1] && o.maxs[2] >= mins[2] && o.mins[0] <= maxs[0] && o.mins[1] <= maxs[1] &&
               o.mins[2] <= maxs[2];
    }
    [[nodiscard]] bool umschliesst(const MinMax& o) const {   // surrounds
        return o.mins[0] >= mins[0] && o.mins[1] >= mins[1] && o.mins[2] >= mins[2] && o.maxs[0] <= maxs[0] && o.maxs[1] <= maxs[1] &&
               o.maxs[2] <= maxs[2];
    }
};
const MinMax& weltKasten() {   // c_worldMinmax
    static const MinMax m{{kWeltMin, kWeltMin, kWeltMin}, {kWeltMax, kWeltMax, kWeltMax}};
    return m;
}

// --- polylib.cpp ----------------------------------------------------------------------
float flaeche(const Wk& w) {   // WindingArea
    float total = 0;
    for (std::size_t i = 2; i < w.size(); ++i) {
        const V3f c = kreuz(minus(w[i - 1], w[0]), minus(w[i], w[0]));
        total = static_cast<float>(static_cast<double>(total) + 0.5 * laenge(c));   // total += 0.5 * vector3_length( ... )
    }
    return total;
}

WkA grundWicklungAccu(const EbeneD& e, const V3d& mins, const V3d& maxs) {   // BaseWindingForPlaneAccu( plane, minmax )
    const V3d& n = e.n;
    int z = 0;   // vector3_max_abs_component_index
    for (int i = 1; i < 3; ++i) {
        if (std::fabs(n[static_cast<std::size_t>(i)]) > std::fabs(n[static_cast<std::size_t>(z)])) { z = i; }
    }
    const int x = (z == 2) ? 0 : z + 1;
    const int y = (x == 2) ? 0 : x + 1;
    if (std::fabs(n[static_cast<std::size_t>(z)]) < 0.56) { return {}; }   // Error( "... normal is too short" )
    const auto ux = static_cast<std::size_t>(x);
    const auto uy = static_cast<std::size_t>(y);
    const auto uz = static_cast<std::size_t>(z);
    V3d a{};
    V3d b{};
    V3d c{};
    V3d d{};
    a[ux] = maxs[ux];
    a[uy] = maxs[uy];
    a[uz] = (e.d - a[ux] * n[ux] - a[uy] * n[uy]) / n[uz];
    b[ux] = maxs[ux];
    b[uy] = mins[uy];
    b[uz] = (e.d - b[ux] * n[ux] - b[uy] * n[uy]) / n[uz];
    c[ux] = mins[ux];
    c[uy] = mins[uy];
    c[uz] = (e.d - c[ux] * n[ux] - c[uy] * n[uy]) / n[uz];
    d[ux] = mins[ux];
    d[uy] = maxs[uy];
    d[uz] = (e.d - d[ux] * n[ux] - d[uy] * n[uy]) / n[uz];
    return n[uz] > 0 ? WkA{a, b, c, d} : WkA{d, c, b, a};
}

Wk grundWicklung(const Ebene& e) {   // BaseWindingForPlane
    float max = -(kWeltMax - kWeltMin);   // -BOGUS_RANGE
    int x = -1;
    for (int i = 0; i < 3; ++i) {
        const float v = std::fabs(e.n[i]);
        if (v > max) {
            x = i;
            max = v;
        }
    }
    if (x == -1) { return {}; }   // Error( "BaseWindingForPlane: no axis found" )
    V3f vup{0, 0, 0};
    if (x == 2) {
        vup[0] = 1;
    } else {
        vup[2] = 1;
    }
    const V3f n{e.n[0], e.n[1], e.n[2]};
    const double dot = static_cast<double>(vup[0] * n[0] + vup[1] * n[1] + vup[2] * n[2]);   // vector3_dot( vup, normal )
    for (std::size_t i = 0; i < 3; ++i) { vup[i] = vup[i] - static_cast<float>(n[i] * dot); }   // vup -= normal * dot
    vup = normiert(vup);
    V3f org;
    for (std::size_t i = 0; i < 3; ++i) { org[i] = n[i] * e.d; }
    V3f vright = kreuz(vup, n);
    constexpr float kGross = kWeltMax * 2;   // MAX_WORLD_COORD * 2
    for (std::size_t i = 0; i < 3; ++i) {
        vup[i] *= kGross;
        vright[i] *= kGross;
    }
    Wk w(4);
    for (std::size_t i = 0; i < 3; ++i) {
        w[0][i] = org[i] - vright[i] + vup[i];
        w[1][i] = org[i] + vright[i] + vup[i];
        w[2][i] = org[i] + vright[i] - vup[i];
        w[3][i] = org[i] - vright[i] - vup[i];
    }
    return w;
}

// Den Schnittpunkt wie polylib: auf achsparallelen Ebenen genau der Abstand
float schnitt(const Ebene& e, std::size_t j, float p1, float p2, double dot) {
    if (e.n[j] == 1) { return e.d; }
    if (e.n[j] == -1) { return -e.d; }
    return static_cast<float>(p1 + dot * static_cast<double>(p2 - p1));
}

// ClipWindingEpsilonStrict: beide leer, wenn alle Punkte auf der Ebene liegen
std::pair<Wk, Wk> teileStreng(const Wk& in, const Ebene& e, float epsilon) {
    std::vector<float> dists(in.size() + 1);
    std::vector<int> sides(in.size() + 1);
    int counts[3] = {0, 0, 0};
    for (std::size_t i = 0; i < in.size(); ++i) {
        dists[i] = static_cast<float>(abstand(e, in[i]));
        if (dists[i] > epsilon) {
            sides[i] = kVorn;
        } else if (dists[i] < -epsilon) {
            sides[i] = kHinten;
        } else {
            sides[i] = kAuf;
        }
        ++counts[sides[i]];
    }
    if (in.empty()) { return {}; }
    sides[in.size()] = sides[0];
    dists[in.size()] = dists[0];
    if (counts[kVorn] == 0 && counts[kHinten] == 0) { return {}; }
    if (counts[kVorn] == 0) { return {Wk{}, in}; }
    if (counts[kHinten] == 0) { return {in, Wk{}}; }
    Wk vorn;
    Wk hinten;
    vorn.reserve(in.size() + 4);
    hinten.reserve(in.size() + 4);
    for (std::size_t i = 0; i < in.size(); ++i) {
        const V3f& p1 = in[i];
        if (sides[i] == kAuf) {
            vorn.push_back(p1);
            hinten.push_back(p1);
            continue;
        }
        if (sides[i] == kVorn) { vorn.push_back(p1); }
        if (sides[i] == kHinten) { hinten.push_back(p1); }
        if (sides[i + 1] == kAuf || sides[i + 1] == sides[i]) { continue; }
        const V3f& p2 = in[(i + 1) % in.size()];   // winding_next_point
        const double dot = static_cast<double>(dists[i] / (dists[i] - dists[i + 1]));
        V3f mid;
        for (std::size_t j = 0; j < 3; ++j) { mid[j] = schnitt(e, j, p1[j], p2[j], dot); }
        vorn.push_back(mid);
        hinten.push_back(mid);
    }
    return {std::move(vorn), std::move(hinten)};
}

std::pair<Wk, Wk> teile(const Wk& in, const Ebene& e, float epsilon) {   // ClipWindingEpsilon
    auto r = teileStreng(in, e, epsilon);
    if (r.first.empty() && r.second.empty()) { r.second = in; }   // "the back winding is the original winding"
    return r;
}

void abschneiden(Wk& inout, const Ebene& e, float epsilon) {   // ChopWindingInPlace
    const Wk& in = inout;
    std::vector<float> dists(in.size() + 1);
    std::vector<int> sides(in.size() + 1);
    int counts[3] = {0, 0, 0};
    for (std::size_t i = 0; i < in.size(); ++i) {
        dists[i] = static_cast<float>(abstand(e, in[i]));
        if (dists[i] > epsilon) {
            sides[i] = kVorn;
        } else if (dists[i] < -epsilon) {
            sides[i] = kHinten;
        } else {
            sides[i] = kAuf;
        }
        ++counts[sides[i]];
    }
    if (in.empty()) { return; }
    sides[in.size()] = sides[0];
    dists[in.size()] = dists[0];
    if (counts[kVorn] == 0) {
        inout.clear();
        return;
    }
    if (counts[kHinten] == 0) { return; }
    Wk f;
    f.reserve(in.size() + 4);
    for (std::size_t i = 0; i < in.size(); ++i) {
        const V3f& p1 = in[i];
        if (sides[i] == kAuf) {
            f.push_back(p1);
            continue;
        }
        if (sides[i] == kVorn) { f.push_back(p1); }
        if (sides[i + 1] == kAuf || sides[i + 1] == sides[i]) { continue; }
        const V3f& p2 = in[(i + 1) % in.size()];
        const double dot = static_cast<double>(dists[i] / (dists[i] - dists[i + 1]));
        V3f mid;
        for (std::size_t j = 0; j < 3; ++j) { mid[j] = schnitt(e, j, p1[j], p2[j], dot); }
        f.push_back(mid);
    }
    inout.swap(f);
}

void abschneidenAccu(WkA& inout, const EbeneD& e, float grobEpsilon) {   // ChopWindingInPlaceAccu
    constexpr double kKleinst = static_cast<double>(FLT_EPSILON) * 0.5;   // VEC_SMALLEST_EPSILON_AROUND_ONE * 0.5
    const double fein = std::max(kKleinst, static_cast<double>(grobEpsilon));
    std::vector<double> dists(inout.size() + 1);
    std::vector<int> sides(inout.size() + 1);
    std::size_t counts[3] = {0, 0, 0};
    for (std::size_t i = 0; i < inout.size(); ++i) {
        dists[i] = abstand(e, inout[i]);
        if (dists[i] > fein) {
            sides[i] = kVorn;
        } else if (dists[i] < -fein) {
            sides[i] = kHinten;
        } else {
            sides[i] = kAuf;
        }
        ++counts[sides[i]];
    }
    if (inout.empty()) { return; }
    sides[inout.size()] = sides[0];
    dists[inout.size()] = dists[0];
    if (counts[kVorn] == 0) {
        inout.clear();
        return;
    }
    if (counts[kHinten] == 0) { return; }
    WkA f;
    f.reserve(counts[kVorn] + 2);
    for (std::size_t i = 0; i < inout.size(); ++i) {
        const V3d& p1 = inout[i];
        if (sides[i] == kAuf || sides[i] == kVorn) {
            f.push_back(p1);
            if (sides[i] == kAuf) { continue; }
        }
        if (sides[i + 1] == kAuf || sides[i + 1] == sides[i]) { continue; }
        const V3d& p2 = inout[(i + 1) % inout.size()];
        const double w = dists[i] / (dists[i] - dists[i + 1]);
        V3d mid;
        for (std::size_t j = 0; j < 3; ++j) {
            if (e.n[j] == 1) {
                mid[j] = e.d;
            } else if (e.n[j] == -1) {
                mid[j] = -e.d;
            } else {
                mid[j] = p1[j] + (w * (p2[j] - p1[j]));
            }
        }
        f.push_back(mid);
    }
    inout.swap(f);
}

int seiteVon(const Wk& w, const Ebene& e) {   // WindingOnPlaneSide
    bool vorn = false;
    bool hinten = false;
    for (const V3f& p : w) {
        const double d = abstand(e, p);
        if (d < -kOnEpsilon) {
            if (vorn) { return kQuer; }
            hinten = true;
            continue;
        }
        if (d > kOnEpsilon) {
            if (hinten) { return kQuer; }
            vorn = true;
        }
    }
    if (hinten) { return kHinten; }
    if (vorn) { return kVorn; }
    return kAuf;
}

void zurHuelle(const Wk& w, Wk& huelle, const V3f& normal) {   // AddWindingToConvexHull
    constexpr int kMaxHuelle = 128;   // MAX_HULL_POINTS
    if (huelle.empty()) {
        huelle = w;
        return;
    }
    if (huelle.size() > static_cast<std::size_t>(kMaxHuelle)) { return; }   // Error( "MAX_HULL_POINTS" )
    std::vector<V3f> punkte(huelle.begin(), huelle.end());
    std::vector<V3f> neu;
    std::vector<V3f> richtungen;
    std::vector<char> seite;
    for (const V3f& p : w) {
        const int n = static_cast<int>(punkte.size());
        const auto wrap = [n](int i) { return i >= n ? i - n : i; };
        richtungen.assign(punkte.size(), V3f{});
        for (int j = 0; j < n; ++j) {
            richtungen[static_cast<std::size_t>(j)] =
                kreuz(normal, normiert(minus(punkte[static_cast<std::size_t>(wrap(j + 1))], punkte[static_cast<std::size_t>(j)])));
        }
        bool draussen = false;
        seite.assign(punkte.size(), 0);
        for (int j = 0; j < n; ++j) {
            const V3f diff = minus(p, punkte[static_cast<std::size_t>(j)]);
            const V3f& r = richtungen[static_cast<std::size_t>(j)];
            const double d = static_cast<double>(diff[0] * r[0] + diff[1] * r[1] + diff[2] * r[2]);
            if (d >= kOnEpsilon) { draussen = true; }
            seite[static_cast<std::size_t>(j)] = d >= -kOnEpsilon ? 1 : 0;
        }
        if (!draussen) { continue; }
        int j = 0;
        for (; j < n; ++j) {
            if (seite[static_cast<std::size_t>(j)] == 0 && seite[static_cast<std::size_t>(wrap(j + 1))] != 0) { break; }
        }
        if (j == n) { continue; }
        neu.clear();
        neu.push_back(p);
        j = wrap(j + 1);
        for (int k = 0; k < n; ++k) {
            if (seite[static_cast<std::size_t>(wrap(j + k))] != 0 && seite[static_cast<std::size_t>(wrap(j + k + 1))] != 0) { continue; }
            neu.push_back(punkte[static_cast<std::size_t>(wrap(j + k + 1))]);
        }
        punkte.swap(neu);
    }
    huelle.assign(punkte.begin(), punkte.end());
}

void kastenDazu(const Wk& w, MinMax& m) {   // WindingExtendBounds
    for (const V3f& p : w) { m.dazu(p); }
}

// --- map.cpp: die Ebenentabelle ---------------------------------------------------------
int typVon(const float n[3]) {   // PlaneTypeForNormal: 0..2 achsparallel, 3 sonst
    if (n[0] == 1 || n[0] == -1) { return 0; }
    if (n[1] == 1 || n[1] == -1) { return 1; }
    if (n[2] == 1 || n[2] == -1) { return 2; }
    return 3;
}

int hashVon(float d) {   // ( PLANE_HASHES - 1 ) & bhed_to_int( std::fabs( dist ) )
    const double x = std::fabs(static_cast<double>(d));
    const int i = (x >= -2147483648.0 && x < 2147483648.0) ? static_cast<int>(x) : std::numeric_limits<int>::min();
    return (kEbenenHashes - 1) & i;
}

struct Ebenen {
    struct Eintrag {   // plane_t
        Ebene e;
        int typ = 3;
        int kette = 0;
    };
    std::vector<Eintrag> p;
    std::vector<int> hash = std::vector<int>(kEbenenHashes, 0);

    void inHash(std::size_t i) {   // AddPlaneToHash
        const int h = hashVon(p[i].e.d);
        p[i].kette = hash[static_cast<std::size_t>(h)];
        hash[static_cast<std::size_t>(h)] = static_cast<int>(i) + 1;
    }
    int neu(const Ebene& e) {   // CreateNewFloatPlane
        const V3f n{e.n[0], e.n[1], e.n[2]};
        if (laenge(n) < 0.5) { return -1; }   // "FloatPlane: bad normal"
        Eintrag a;
        a.e = e;
        Eintrag b;
        b.e = umgedreht(e);
        a.typ = b.typ = typVon(e.n);
        p.push_back(a);
        p.push_back(b);
        const std::size_t i = p.size() - 2;
        if (a.typ < 3 && (e.n[0] < 0 || e.n[1] < 0 || e.n[2] < 0)) {
            std::swap(p[i], p[i + 1]);
            inHash(i);
            inHash(i + 1);
            return static_cast<int>(i + 1);
        }
        inHash(i);
        inHash(i + 1);
        return static_cast<int>(i);
    }
    // FindFloatPlane___ (USE_HASHING, Q3MAP2_EXPERIMENTAL_SNAP_PLANE_FIX); Punkte float (Vector3) oder double
    template <class P>
    int finde(Ebene e, const P* punkte, std::size_t zahl) {
        std::vector<Vektor3d> pv;
        for (std::size_t i = 0; i < zahl; ++i) { pv.emplace_back(punkte[i][0], punkte[i][1], punkte[i][2]); }
        ebeneEinrasten(e, pv.empty() ? nullptr : pv.data(), pv.size());
        const int h0 = hashVon(e.d);
        for (int i = -1; i <= 1; ++i) {
            const int h = (h0 + i) & (kEbenenHashes - 1);
            for (int idx = hash[static_cast<std::size_t>(h)] - 1; idx != -1; idx = p[static_cast<std::size_t>(idx)].kette - 1) {
                const Eintrag& q = p[static_cast<std::size_t>(idx)];
                if (!ebenenGleich(q.e, e)) { continue; }
                bool alle = true;
                for (std::size_t k = 0; k < zahl && alle; ++k) {
                    const double d = std::fabs(abstand(q.e, punkte[k]));
                    alle = d == 0 || d < kDistanceEpsilon;
                }
                if (alle) { return idx; }
            }
        }
        return neu(e);
    }
    [[nodiscard]] const Ebene& ebene(int i) const { return p[static_cast<std::size_t>(i)].e; }
};

// --- brush.cpp --------------------------------------------------------------------------
struct ShaderDaten {   // shaderInfo_t (der Teil, den der Ablauf braucht)
    Q3ShaderFlags flags;
    bool autosprite = false;
    bool polygonOffset = false;
    bool noClip = false;
    bool indexed = false;
    bool bewegt = false;   // minmax.valid() (deformVertexes move)
    std::array<float, 3> bewegtMin{};
    std::array<float, 3> bewegtMax{};
};

struct SichtSeite : Seite {   // side_t; Seite::shader: Index in die Shaderliste, -1 ohne shaderInfo
    int planenum = -1;
    EbeneD ebeneD;            // side.plane aus den Punkten (ohne Einrasten); Normale 0: keine
    Wk huelle;                // visibleHull
    bool culled = false;
};

struct SichtPinsel {   // brush_t
    std::vector<SichtSeite> seiten;
    MinMax kasten;
    bool detail = false;
    bool opaque = true;
    std::uint32_t compile = 0;
    const SichtPinsel* original = nullptr;
};

bool begrenzen(SichtPinsel& b) {   // BoundBrush
    b.kasten = MinMax{};
    for (const SichtSeite& s : b.seiten) { kastenDazu(s.wicklung, b.kasten); }
    return b.kasten.gueltig() && weltKasten().umschliesst(b.kasten);
}

V3d schweissen(const V3d& a, const V3d& b) {   // SnapWeldVectorAccu
    V3d out{};
    for (std::size_t i = 0; i < 3; ++i) {
        const double ai = std::rint(a[i]);
        const double bi = std::rint(b[i]);
        const double ad = std::fabs(ai - a[i]);
        const double bd = std::fabs(bi - b[i]);
        if (ad < bd) {
            out[i] = ad < kSnapEpsilon ? ai : a[i];
        } else {
            out[i] = bd < kSnapEpsilon ? bi : b[i];
        }
    }
    return out;
}

void reparieren(WkA& w) {   // FixWindingAccu
    while (w.size() > 1) {
        bool fertig = true;
        for (std::size_t j = 0, i = w.size() - 1; j < w.size(); i = j++) {
            const V3d d{w[i][0] - w[j][0], w[i][1] - w[j][1], w[i][2] - w[j][2]};
            if (std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) < kDegeneriert) {
                w[i] = schweissen(w[i], w[j]);
                w.erase(w.begin() + static_cast<std::ptrdiff_t>(j));
                fertig = false;
                break;
            }
        }
        if (fertig) { break; }
    }
}

bool dreiEbenen(const EbeneD& p1, const EbeneD& p2, const EbeneD& p3, V3d& out) {   // solve3Planes
    const double m[3][3] = {{p1.n[0], p1.n[1], p1.n[2]}, {p2.n[0], p2.n[1], p2.n[2]}, {p3.n[0], p3.n[1], p3.n[2]}};
    const double b[3] = {p1.d, p2.d, p3.d};
    const auto det3 = [](const double a[3][3]) {
        return a[0][0] * (a[1][1] * a[2][2] - a[2][1] * a[1][2]) - a[0][1] * (a[1][0] * a[2][2] - a[2][0] * a[1][2]) +
               a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
    };
    const double det = det3(m);
    if (std::fabs(det) < 1e-9) { return false; }
    const double mx[3][3] = {{b[0], m[0][1], m[0][2]}, {b[1], m[1][1], m[1][2]}, {b[2], m[2][1], m[2][2]}};
    const double my[3][3] = {{m[0][0], b[0], m[0][2]}, {m[1][0], b[1], m[1][2]}, {m[2][0], b[2], m[2][2]}};
    const double mz[3][3] = {{m[0][0], m[0][1], b[0]}, {m[1][0], m[1][1], b[1]}, {m[2][0], m[2][1], b[2]}};
    out[0] = det3(mx) / det;
    out[1] = det3(my) / det;
    out[2] = det3(mz) / det;
    return true;
}

// CreateBrushWindings (HIGH_PRECISION_MATH_FIXES, OFFSET_WINDING_CREATION)
bool wicklungenBauen(SichtPinsel& b, const Ebenen& ebenen) {
    std::vector<EbeneD> planes;
    planes.reserve(b.seiten.size());
    for (const SichtSeite& s : b.seiten) {
        if (s.ebeneD.n[0] != 0 || s.ebeneD.n[1] != 0 || s.ebeneD.n[2] != 0) {
            planes.push_back(s.ebeneD);
        } else {
            const Ebene& e = ebenen.ebene(s.planenum);
            planes.push_back(EbeneD{{e.n[0], e.n[1], e.n[2]}, e.d});
        }
    }
    // brushMinMaxFromPlanes
    V3d mins{std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max()};
    V3d maxs{std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest()};
    for (std::size_t i = 0; i < planes.size(); ++i) {
        for (std::size_t j = i + 1; j < planes.size(); ++j) {
            for (std::size_t k = j + 1; k < planes.size(); ++k) {
                V3d v;
                if (!dreiEbenen(planes[i], planes[j], planes[k], v)) { continue; }
                bool innen = true;
                for (const EbeneD& p : planes) {
                    if (abstand(p, v) >= kOnEpsilon) {
                        innen = false;
                        break;
                    }
                }
                if (!innen) { continue; }
                for (std::size_t a = 0; a < 3; ++a) {
                    if (v[a] < mins[a]) { mins[a] = v[a]; }
                    if (v[a] > maxs[a]) { maxs[a] = v[a]; }
                }
            }
        }
    }
    if (!(mins[0] < maxs[0] && mins[1] < maxs[1] && mins[2] < maxs[2])) { return false; }
    const V3d offset{(mins[0] + maxs[0]) * 0.5, (mins[1] + maxs[1]) * 0.5, (mins[2] + maxs[2]) * 0.5};
    for (std::size_t a = 0; a < 3; ++a) {
        maxs[a] -= offset[a];
        mins[a] -= offset[a];
    }
    for (EbeneD& p : planes) {   // plane3_translated( plane3_flipped( p ), -offset )
        p.n = {-p.n[0], -p.n[1], -p.n[2]};
        p.d = -p.d + (p.n[0] * -offset[0] + p.n[1] * -offset[1] + p.n[2] * -offset[2]);
    }
    for (std::size_t i = 0; i < b.seiten.size(); ++i) {
        SichtSeite& s = b.seiten[i];
        const EbeneD& pi = planes[i];
        WkA w = grundWicklungAccu(EbeneD{{-pi.n[0], -pi.n[1], -pi.n[2]}, -pi.d}, mins, maxs);
        for (std::size_t j = 0; j < b.seiten.size() && !w.empty(); ++j) {
            if (i == j || b.seiten[j].planenum == (s.planenum ^ 1)) { continue; }   // back side clipaway
            abschneidenAccu(w, planes[j], 0);
        }
        reparieren(w);
        s.wicklung.clear();
        if (w.size() >= 3) {
            for (const V3d& v : w) {
                s.wicklung.push_back(V3f{static_cast<float>(v[0] + offset[0]), static_cast<float>(v[1] + offset[1]), static_cast<float>(v[2] + offset[2])});
            }
        }
    }
    return begrenzen(b);
}

bool winzig(const Wk& w) {   // WindingIsTiny
    int kanten = 0;
    for (std::size_t j = 0, i = w.size() - 1; j < w.size(); i = j++) {
        if (laenge(minus(w[j], w[i])) > kKante && ++kanten == 3) { return false; }
    }
    return true;
}

float volumen(const SichtPinsel& b, const Ebenen& ebenen) {   // BrushVolume
    float v = 0;
    for (auto i = b.seiten.cbegin(); i != b.seiten.cend(); ++i) {
        if (i->wicklung.empty()) { continue; }
        const V3f ecke = i->wicklung[0];
        for (++i; i != b.seiten.cend(); ++i) {
            if (!i->wicklung.empty()) {
                v = static_cast<float>(static_cast<double>(v) + -abstand(ebenen.ebene(i->planenum), ecke) * static_cast<double>(flaeche(i->wicklung)));
            }
        }
        break;
    }
    return v / 3;
}

int meistAufSeite(const SichtPinsel& b, const Ebene& e) {   // BrushMostlyOnSide
    float max = 0;
    int seite = kVorn;
    for (const SichtSeite& s : b.seiten) {
        for (const V3f& p : s.wicklung) {
            const double d = abstand(e, p);
            if (d > max) {
                max = static_cast<float>(d);
                seite = kVorn;
            }
            if (-d > max) {
                max = static_cast<float>(-d);
                seite = kHinten;
            }
        }
    }
    return seite;
}

// SplitBrush: zwei neue Pinsel, der alte bleibt; leer = auf dieser Seite nichts
std::pair<SichtPinsel, SichtPinsel> teilePinsel(const SichtPinsel& b, int planenum, const Ebenen& ebenen) {
    const Ebene& e = ebenen.ebene(planenum);
    float vorn = 0;
    float hinten = 0;
    for (const SichtSeite& s : b.seiten) {
        for (const V3f& p : s.wicklung) {
            const auto d = static_cast<float>(abstand(e, p));
            if (d > 0) { vorn = std::max(vorn, d); }
            if (d < 0) { hinten = std::min(hinten, d); }
        }
    }
    if (vorn < 0.1F) { return {SichtPinsel{}, b}; }    // PLANESIDE_EPSILON: nur hinten
    if (hinten > -0.1F) { return {b, SichtPinsel{}}; }  // nur vorn
    Wk mitte = grundWicklung(e);
    for (const SichtSeite& s : b.seiten) {
        abschneiden(mitte, ebenen.ebene(s.planenum ^ 1), 0);
        if (mitte.empty()) { break; }
    }
    if (mitte.empty()) {   // nicht wirklich geteilt (PR #316 110bc292e8: auch ein kleiner Schnitt teilt)
        if (meistAufSeite(b, e) == kHinten) { return {SichtPinsel{}, b}; }
        return {b, SichtPinsel{}};
    }
    SichtPinsel t[2] = {b, b};
    t[0].seiten.clear();
    t[1].seiten.clear();
    for (const SichtSeite& s : b.seiten) {
        if (s.wicklung.empty()) { continue; }
        auto [w0, w1] = teileStreng(s.wicklung, e, 0);
        Wk* cw[2] = {&w0, &w1};
        for (int i = 0; i < 2; ++i) {
            if (cw[i]->empty()) { continue; }
            SichtSeite& cs = t[i].seiten.emplace_back(s);
            cs.wicklung.swap(*cw[i]);
        }
    }
    for (SichtPinsel& x : t) {
        if (x.seiten.size() < 3 || !begrenzen(x)) { x.seiten.clear(); }
    }
    if (t[0].seiten.empty() || t[1].seiten.empty()) {
        if (!t[0].seiten.empty()) { return {b, SichtPinsel{}}; }
        if (!t[1].seiten.empty()) { return {SichtPinsel{}, b}; }
        return {};
    }
    for (int i = 0; i < 2; ++i) {
        SichtSeite& cs = t[i].seiten.emplace_back();
        cs.planenum = planenum ^ i ^ 1;
        cs.ebene = ebenen.ebene(cs.planenum);
        cs.shader = -1;
        cs.wicklung = mitte;
    }
    for (SichtPinsel& x : t) {
        if (volumen(x, ebenen) <= 0.0F) { x.seiten.clear(); }   // g_microVolume 0 (PR #316 d95377cc26)
    }
    return {std::move(t[0]), std::move(t[1])};
}

// --- facebsp.cpp ------------------------------------------------------------------------
constexpr int kBlockGroesse = 1024;   // blockSize (worldspawn "_blocksize" aendert sie)
constexpr int kHinweisVorrang = 1000;          // HINT_PRIORITY
constexpr int kAntiportalVorrang = -1000;      // ANTIPORTAL_PRIORITY
constexpr int kAreaportalVorrang = -1000;      // AREAPORTAL_PRIORITY
constexpr int kDetailVorrang = -3000;          // DETAIL_PRIORITY

struct Flaeche3 {   // face_t
    Wk w;
    int planenum = 0;
    int vorrang = 0;
    std::uint32_t compile = 0;
    const SichtPinsel* pinsel = nullptr;
    int seitenPlanenum = 0;
};
using FlaechenListe = std::forward_list<Flaeche3>;

struct Knoten {   // node_t
    int planenum = kBlatt;
    int eltern = -1;
    int kinder[2] = {-1, -1};
    MinMax kasten;
    std::uint32_t compile = 0;
    bool opaque = false;
    int occupied = 0;
    int portale = -1;
    std::vector<std::pair<const SichtPinsel*, int>> koplanar;   // coplanarBrushes (PR #316 7e109335f7)
};

struct Portal {   // portal_t
    Ebene ebene;
    int onnode = -1;
    int knoten[2] = {-1, -1};
    int weiter[2] = {-1, -1};
    Wk w;
    std::uint32_t compile = 0;
};

struct Baum {   // tree_t
    const std::atomic<bool>* abbrechen = nullptr;   // ein anderer Faden bittet um das Ende (leere Welt)
    [[nodiscard]] bool abgebrochen() const { return abbrechen != nullptr && abbrechen->load(std::memory_order_relaxed); }
    std::vector<Knoten> k;
    std::vector<Portal> p;
    int kopf = -1;
    int aussen = -1;   // outside_node
    MinMax kasten;
    int blockGroesse[3] = {kBlockGroesse, kBlockGroesse, kBlockGroesse};

    int neuerKnoten() {
        k.emplace_back();
        return static_cast<int>(k.size()) - 1;
    }
};

// SelectSplitPlaneNum (bspAlternateSplitWeights aus); der Zaehler counter dient nur jener Gewichtung
std::pair<int, std::uint32_t> teilerWaehlen(const Baum& t, int knoten, const FlaechenListe& liste, Ebenen& ebenen) {
    const Knoten& n = t.k[static_cast<std::size_t>(knoten)];
    for (int i = 0; i < 3; ++i) {   // ueber eine Blockgrenze: dort teilen
        const int bg = t.blockGroesse[i];
        if (bg <= 0) { continue; }
        const float dist = static_cast<float>(bg) * (std::floor(n.kasten.mins[static_cast<std::size_t>(i)] / static_cast<float>(bg)) + 1);
        if (n.kasten.maxs[static_cast<std::size_t>(i)] > dist) {
            Ebene e;
            e.n[i] = 1;
            e.d = dist;
            return {ebenen.finde(e, static_cast<const V3f*>(nullptr), 0), 0U};
        }
    }
    // Gleiches Ergebnis, schneller: splits und facing haengen nur an der Ebene des Teilers - je Ebene einmal
    // gerechnet. An einer achsparallelen Ebene ist der Abstand eines Punktes genau seine Koordinate (mal +-1)
    // minus dist (die anderen Anteile der Normalen sind 0) - die Seite der Wicklung folgt dann aus ihren Grenzen.
    struct Grenzen {
        const Flaeche3* f;
        V3f mins;
        V3f maxs;
    };
    std::vector<Grenzen> grenzen;
    for (const Flaeche3& f : liste) {
        Grenzen g{&f, {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
                  {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()}};
        for (const V3f& q : f.w) {
            for (std::size_t i = 0; i < 3; ++i) {
                g.mins[i] = std::min(g.mins[i], q[i]);
                g.maxs[i] = std::max(g.maxs[i], q[i]);
            }
        }
        grenzen.push_back(g);
    }
    std::unordered_map<int, std::pair<int, int>> jeEbene;   // planenum -> ( splits, facing )
    const auto zaehlen = [&](int planenum) {
        const Ebene& e = ebenen.ebene(planenum);
        int achse = -1;
        for (int i = 0; i < 3; ++i) {
            if ((e.n[i] == 1 || e.n[i] == -1) && e.n[(i + 1) % 3] == 0 && e.n[(i + 2) % 3] == 0) { achse = i; }
        }
        int splits = 0;
        int facing = 0;
        for (const Grenzen& g : grenzen) {
            if (g.f->planenum == planenum) {
                ++facing;
                continue;
            }
            if (achse < 0) {
                if (seiteVon(g.f->w, e) == kQuer) { ++splits; }
                continue;
            }
            const auto a = static_cast<std::size_t>(achse);
            const float lo = e.n[achse] == 1 ? g.mins[a] : -g.maxs[a];
            const float hi = e.n[achse] == 1 ? g.maxs[a] : -g.mins[a];
            if (static_cast<double>(lo) - static_cast<double>(e.d) < -kOnEpsilon && static_cast<double>(hi) - static_cast<double>(e.d) > kOnEpsilon) {
                ++splits;
            }
        }
        return std::make_pair(splits, facing);
    };
    int besterWert = -99999;
    const Flaeche3* bester = nullptr;
    for (const Flaeche3& split : liste) {
        const Ebenen::Eintrag& ebene = ebenen.p[static_cast<std::size_t>(split.planenum)];
        auto it = jeEbene.find(split.planenum);
        if (it == jeEbene.end()) { it = jeEbene.emplace(split.planenum, zaehlen(split.planenum)).first; }
        const int splits = it->second.first;
        const int facing = it->second.second;
        int wert = 5 * facing - 5 * splits;
        if (ebene.typ < 3) { wert += 5; }   // axial is better
        wert += split.vorrang;
        if (wert > besterWert) {
            besterWert = wert;
            bester = &split;
        }
    }
    if (besterWert == -99999 || bester == nullptr) { return {kBlatt, 0U}; }
    return {bester->planenum, bester->compile};
}

void baumBauen_r(Baum& t, int knoten, FlaechenListe& liste, Ebenen& ebenen) {   // BuildFaceTree_r
    FlaechenListe kind[2];
    const auto [teiler, compile] = t.abgebrochen() ? std::pair<int, std::uint32_t>{kBlatt, 0U} : teilerWaehlen(t, knoten, liste, ebenen);
    if (teiler == kBlatt) {
        t.k[static_cast<std::size_t>(knoten)].planenum = kBlatt;
        return;
    }
    t.k[static_cast<std::size_t>(knoten)].planenum = teiler;
    t.k[static_cast<std::size_t>(knoten)].compile = compile;
    const Ebene ebene = ebenen.ebene(teiler);
    while (!liste.empty()) {
        Flaeche3& split = liste.front();
        if (split.planenum == teiler) {   // don't split by identical plane
            liste.pop_front();
            continue;
        }
        const int seite = seiteVon(split.w, ebene);
        if (seite == kQuer) {   // PR #316 4c7c216fd5: streng teilen
            auto [vorn, hinten] = teileStreng(split.w, ebene, static_cast<float>(kOnEpsilon));
            Wk* teile[2] = {&vorn, &hinten};
            for (int i = 0; i < 2; ++i) {
                if (teile[i]->empty()) { continue; }
                Flaeche3& neu = kind[i].emplace_front();
                neu.w.swap(*teile[i]);
                neu.planenum = split.planenum;
                neu.vorrang = split.vorrang;
                neu.compile = split.compile;
                neu.pinsel = split.pinsel;
                neu.seitenPlanenum = split.seitenPlanenum;
            }
            liste.pop_front();
        } else if (seite == kVorn) {
            kind[0].splice_after(kind[0].cbefore_begin(), liste, liste.cbefore_begin());
        } else if (seite == kHinten) {
            kind[1].splice_after(kind[1].cbefore_begin(), liste, liste.cbefore_begin());
        } else {   // eSideOn: eine Seite einer ANDEREN Ebene innerhalb ON_EPSILON (PR #316 7e109335f7)
            if (split.pinsel != nullptr && (split.planenum | 1) != (teiler | 1)) {
                const Ebene& sp = ebenen.ebene(split.seitenPlanenum);
                const double dot = static_cast<double>(sp.n[0] * ebene.n[0] + sp.n[1] * ebene.n[1] + sp.n[2] * ebene.n[2]);
                t.k[static_cast<std::size_t>(knoten)].koplanar.emplace_back(split.pinsel, dot > 0 ? kHinten : kVorn);
            }
            liste.pop_front();
        }
    }
    for (int i = 0; i < 2; ++i) {
        const int c = t.neuerKnoten();
        t.k[static_cast<std::size_t>(knoten)].kinder[i] = c;
        t.k[static_cast<std::size_t>(c)].eltern = knoten;
        t.k[static_cast<std::size_t>(c)].kasten = t.k[static_cast<std::size_t>(knoten)].kasten;
    }
    Knoten& n = t.k[static_cast<std::size_t>(knoten)];
    for (std::size_t i = 0; i < 3; ++i) {
        if (ebene.n[i] == 1) {
            t.k[static_cast<std::size_t>(n.kinder[0])].kasten.mins[i] = ebene.d;
            t.k[static_cast<std::size_t>(n.kinder[1])].kasten.maxs[i] = ebene.d;
            break;
        }
        if (ebene.n[i] == -1) {
            t.k[static_cast<std::size_t>(n.kinder[0])].kasten.maxs[i] = -ebene.d;
            t.k[static_cast<std::size_t>(n.kinder[1])].kasten.mins[i] = -ebene.d;
            break;
        }
    }
    const int k0 = n.kinder[0];
    const int k1 = n.kinder[1];
    baumBauen_r(t, k0, kind[0], ebenen);
    baumBauen_r(t, k1, kind[1], ebenen);
}

FlaechenListe strukturFlaechen(std::list<SichtPinsel>& welt) {   // MakeStructuralBSPFaceList (deepBSP aus)
    FlaechenListe liste;
    for (SichtPinsel& b : welt) {
        if (b.detail) { continue; }
        if (std::count_if(b.seiten.begin(), b.seiten.end(), [](const SichtSeite& s) { return !s.wicklung.empty(); }) < 4) {
            b.detail = true;   // "ignoring malformed structural brush: sides < 4"
            continue;
        }
        for (const SichtSeite& s : b.seiten) {
            if (s.wicklung.empty() || (s.flags.compile & C_SKIP) != 0U) { continue; }
            Flaeche3& f = liste.emplace_front();
            f.w = s.wicklung;
            f.planenum = s.planenum & ~1;
            f.compile = s.flags.compile;
            f.pinsel = b.original;
            f.seitenPlanenum = s.planenum;
            f.vorrang = 0;
            if ((f.compile & C_HINT) != 0U) { f.vorrang += kHinweisVorrang; }
            if ((f.compile & C_ANTIPORTAL) != 0U) { f.vorrang += kAntiportalVorrang; }
            if ((f.compile & C_AREAPORTAL) != 0U) { f.vorrang += kAreaportalVorrang; }
            if ((f.compile & C_DETAIL) != 0U) { f.vorrang += kDetailVorrang; }
        }
    }
    return liste;
}

Baum flaechenBsp(FlaechenListe& liste, Ebenen& ebenen, const int blockGroesse[3], const std::atomic<bool>* abbrechen) {   // FaceBSP
    Baum t;
    t.abbrechen = abbrechen;
    for (int i = 0; i < 3; ++i) { t.blockGroesse[i] = blockGroesse[i]; }
    for (const Flaeche3& f : liste) { kastenDazu(f.w, t.kasten); }
    t.aussen = t.neuerKnoten();
    t.kopf = t.neuerKnoten();
    t.k[static_cast<std::size_t>(t.kopf)].kasten = t.kasten;
    baumBauen_r(t, t.kopf, liste, ebenen);
    return t;
}

// --- portals.cpp ------------------------------------------------------------------------
constexpr int kSeitenraum = 8;               // SIDESPACE
constexpr float kGrundEpsilon = 0.001F;      // BASE_WINDING_EPSILON
constexpr float kTeilEpsilon = 0.001F;       // SPLIT_WINDING_EPSILON

int seiteIn(const Portal& p, int knoten) { return p.knoten[1] == knoten ? 1 : 0; }   // thisSide

void portalDazu(Baum& t, int p, int vorn, int hinten) {   // AddPortalToNodes
    Portal& q = t.p[static_cast<std::size_t>(p)];
    q.knoten[0] = vorn;
    q.weiter[0] = t.k[static_cast<std::size_t>(vorn)].portale;
    t.k[static_cast<std::size_t>(vorn)].portale = p;
    q.knoten[1] = hinten;
    q.weiter[1] = t.k[static_cast<std::size_t>(hinten)].portale;
    t.k[static_cast<std::size_t>(hinten)].portale = p;
}

void portalWeg(Baum& t, int p, int l) {   // RemovePortalFromNode
    int* pp = &t.k[static_cast<std::size_t>(l)].portale;
    while (*pp != -1 && *pp != p) {
        Portal& q = t.p[static_cast<std::size_t>(*pp)];
        pp = &q.weiter[q.knoten[0] == l ? 0 : 1];
    }
    if (*pp == -1) { return; }   // Error( "RemovePortalFromNode: portal not in leaf" )
    Portal& q = t.p[static_cast<std::size_t>(p)];
    if (q.knoten[0] == l) {
        *pp = q.weiter[0];
        q.knoten[0] = -1;
    } else if (q.knoten[1] == l) {
        *pp = q.weiter[1];
        q.knoten[1] = -1;
    }
}

void kopfPortale(Baum& t) {   // MakeHeadnodePortals
    MinMax grenzen = t.kasten;
    for (std::size_t i = 0; i < 3; ++i) {
        grenzen.mins[i] -= static_cast<float>(kSeitenraum);
        grenzen.maxs[i] += static_cast<float>(kSeitenraum);
    }
    int portale[6];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 2; ++j) {
            Portal q;
            if (j != 0) {
                q.ebene.n[i] = -1;
                q.ebene.d = -grenzen.maxs[static_cast<std::size_t>(i)];
            } else {
                q.ebene.n[i] = 1;
                q.ebene.d = grenzen.mins[static_cast<std::size_t>(i)];
            }
            q.w = grundWicklung(q.ebene);
            t.p.push_back(std::move(q));
            portale[j * 3 + i] = static_cast<int>(t.p.size()) - 1;
            portalDazu(t, portale[j * 3 + i], t.kopf, t.aussen);
        }
    }
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 6; ++j) {
            if (j == i) { continue; }
            abschneiden(t.p[static_cast<std::size_t>(portale[i])].w, t.p[static_cast<std::size_t>(portale[j])].ebene, static_cast<float>(kOnEpsilon));
        }
    }
}

Wk knotenWicklung(const Baum& t, int knoten, const Ebenen& ebenen) {   // BaseWindingForNode
    Wk w = grundWicklung(ebenen.ebene(t.k[static_cast<std::size_t>(knoten)].planenum));
    int node = knoten;
    for (int n = t.k[static_cast<std::size_t>(node)].eltern; n != -1 && !w.empty();) {
        const Ebene& e = ebenen.ebene(t.k[static_cast<std::size_t>(n)].planenum);
        if (t.k[static_cast<std::size_t>(n)].kinder[0] == node) {
            abschneiden(w, e, kGrundEpsilon);
        } else {
            abschneiden(w, umgedreht(e), kGrundEpsilon);
        }
        node = n;
        n = t.k[static_cast<std::size_t>(n)].eltern;
    }
    return w;
}

void knotenPortal(Baum& t, int knoten, const Ebenen& ebenen) {   // MakeNodePortal
    Wk w = knotenWicklung(t, knoten, ebenen);
    for (int p = t.k[static_cast<std::size_t>(knoten)].portale; p != -1 && !w.empty();) {
        const Portal& q = t.p[static_cast<std::size_t>(p)];
        int seite = 0;
        if (q.knoten[0] == knoten) {
            abschneiden(w, q.ebene, kClipEpsilon);
        } else if (q.knoten[1] == knoten) {
            seite = 1;
            abschneiden(w, umgedreht(q.ebene), kClipEpsilon);
        } else {
            return;   // Error( "CutNodePortals_r: mislinked portal" )
        }
        p = q.weiter[seite];
    }
    if (w.empty() || winzig(w)) { return; }
    const Knoten& n = t.k[static_cast<std::size_t>(knoten)];
    Portal neu;
    neu.ebene = ebenen.ebene(n.planenum);
    neu.onnode = knoten;
    neu.w.swap(w);
    neu.compile = n.compile;
    t.p.push_back(std::move(neu));
    portalDazu(t, static_cast<int>(t.p.size()) - 1, n.kinder[0], n.kinder[1]);
}

void knotenPortaleTeilen(Baum& t, int knoten, const Ebenen& ebenen) {   // SplitNodePortals
    const Ebene ebene = ebenen.ebene(t.k[static_cast<std::size_t>(knoten)].planenum);
    const int vorn = t.k[static_cast<std::size_t>(knoten)].kinder[0];
    const int hinten = t.k[static_cast<std::size_t>(knoten)].kinder[1];
    for (int p = t.k[static_cast<std::size_t>(knoten)].portale, naechstes = -1; p != -1; p = naechstes) {
        int seite = 0;
        if (t.p[static_cast<std::size_t>(p)].knoten[0] == knoten) {
            seite = 0;
        } else if (t.p[static_cast<std::size_t>(p)].knoten[1] == knoten) {
            seite = 1;
        } else {
            return;   // Error( "SplitNodePortals: mislinked portal" )
        }
        naechstes = t.p[static_cast<std::size_t>(p)].weiter[seite];
        const int anderer = t.p[static_cast<std::size_t>(p)].knoten[seite == 0 ? 1 : 0];
        portalWeg(t, p, t.p[static_cast<std::size_t>(p)].knoten[0]);
        portalWeg(t, p, t.p[static_cast<std::size_t>(p)].knoten[1]);
        auto [vw, hw] = teile(t.p[static_cast<std::size_t>(p)].w, ebene, kTeilEpsilon);   // nicht streng
        if (!vw.empty() && winzig(vw)) { vw.clear(); }
        if (!hw.empty() && winzig(hw)) { hw.clear(); }
        if (vw.empty() && hw.empty()) { continue; }   // winzig auf beiden Seiten
        if (vw.empty()) {
            if (seite == 0) {
                portalDazu(t, p, hinten, anderer);
            } else {
                portalDazu(t, p, anderer, hinten);
            }
            continue;
        }
        if (hw.empty()) {
            if (seite == 0) {
                portalDazu(t, p, vorn, anderer);
            } else {
                portalDazu(t, p, anderer, vorn);
            }
            continue;
        }
        Portal neu = t.p[static_cast<std::size_t>(p)];   // new portal_t( *p )
        neu.w.swap(hw);
        t.p[static_cast<std::size_t>(p)].w.swap(vw);
        t.p.push_back(std::move(neu));
        const int np = static_cast<int>(t.p.size()) - 1;
        if (seite == 0) {
            portalDazu(t, p, vorn, anderer);
            portalDazu(t, np, hinten, anderer);
        } else {
            portalDazu(t, p, anderer, vorn);
            portalDazu(t, np, anderer, hinten);
        }
    }
    t.k[static_cast<std::size_t>(knoten)].portale = -1;
}

void baumPortale_r(Baum& t, int knoten, const Ebenen& ebenen) {   // MakeTreePortals_r (ohne die Warnungen)
    if (t.k[static_cast<std::size_t>(knoten)].planenum == kBlatt) { return; }
    knotenPortal(t, knoten, ebenen);
    knotenPortaleTeilen(t, knoten, ebenen);
    const int k0 = t.k[static_cast<std::size_t>(knoten)].kinder[0];
    const int k1 = t.k[static_cast<std::size_t>(knoten)].kinder[1];
    baumPortale_r(t, k0, ebenen);
    baumPortale_r(t, k1, ebenen);
}

void baumPortale(Baum& t, const Ebenen& ebenen) {   // MakeTreePortals
    kopfPortale(t);
    baumPortale_r(t, t.kopf, ebenen);
}

// FilterBrushIntoTree_r (nur die Wirkung auf opaque; die Pinsel-Liste der Blaetter braucht der Ablauf nicht)
void pinselInBaum_r(SichtPinsel b, Baum& t, int knoten, const Ebenen& ebenen) {
    if (b.seiten.empty() || t.abgebrochen()) { return; }
    Knoten& n = t.k[static_cast<std::size_t>(knoten)];
    if (n.planenum == kBlatt) {
        if (!b.detail && b.opaque) { n.opaque = true; }
        return;
    }
    int innen = -1;   // PR #316 7e109335f7 (M3, sliver)
    for (const auto& [pinsel, seite] : n.koplanar) {
        if (pinsel == b.original) { innen = (innen == -1 || innen == seite) ? seite : -2; }
    }
    if (innen >= 0) {
        pinselInBaum_r(std::move(b), t, n.kinder[innen], ebenen);
        return;
    }
    const int k0 = n.kinder[0];
    const int k1 = n.kinder[1];
    auto [vorn, hinten] = teilePinsel(b, n.planenum, ebenen);
    pinselInBaum_r(std::move(vorn), t, k0, ebenen);
    pinselInBaum_r(std::move(hinten), t, k1, ebenen);
}

void strukturInBaum(const std::list<SichtPinsel>& welt, Baum& t, const Ebenen& ebenen) {   // FilterStructuralBrushesIntoTree
    for (const SichtPinsel& b : welt) {
        if (!b.detail) { pinselInBaum_r(b, t, t.kopf, ebenen); }
    }
}

void flutePortale(Baum& t, int start) {   // FloodPortals
    int dist = 1;
    std::vector<int> knoten{start};
    std::vector<int> naechste;
    while (!knoten.empty()) {
        for (const int k : knoten) {
            Knoten& n = t.k[static_cast<std::size_t>(k)];
            if (n.opaque || (n.occupied != 0 && n.occupied <= dist)) { continue; }
            n.occupied = dist;
            for (int p = n.portale; p != -1; p = t.p[static_cast<std::size_t>(p)].weiter[seiteIn(t.p[static_cast<std::size_t>(p)], k)]) {
                const Portal& q = t.p[static_cast<std::size_t>(p)];
                naechste.push_back(q.knoten[seiteIn(q, k) == 0 ? 1 : 0]);
            }
        }
        knoten.swap(naechste);
        naechste.clear();
        ++dist;
    }
}

bool bewohnerSetzen(Baum& t, const V3f& ursprung, const Ebenen& ebenen) {   // PlaceOccupant
    int k = t.kopf;
    while (t.k[static_cast<std::size_t>(k)].planenum != kBlatt) {
        const Knoten& n = t.k[static_cast<std::size_t>(k)];
        k = abstand(ebenen.ebene(n.planenum), ursprung) >= 0 ? n.kinder[0] : n.kinder[1];
    }
    if (t.k[static_cast<std::size_t>(k)].opaque) { return false; }
    flutePortale(t, k);
    return true;
}

enum class Flut { Gut, Leck, Leer };   // EFloodEntities

Flut entitaetenFluten(Baum& t, const std::vector<V3f>& bewohner, const Ebenen& ebenen) {   // FloodEntities
    bool innen = false;
    t.k[static_cast<std::size_t>(t.aussen)].occupied = 0;
    for (V3f o : bewohner) {
        o[2] += 1;   // nudge off floor
        if (bewohnerSetzen(t, o, ebenen)) { innen = true; }
    }
    if (!innen) { return Flut::Leer; }
    if (t.k[static_cast<std::size_t>(t.aussen)].occupied != 0) { return Flut::Leck; }
    return Flut::Gut;
}

void aussenFuellen_r(Baum& t, int knoten) {   // FillOutside_r
    Knoten& n = t.k[static_cast<std::size_t>(knoten)];
    if (n.planenum != kBlatt) {
        const int k0 = n.kinder[0];
        const int k1 = n.kinder[1];
        aussenFuellen_r(t, k0);
        aussenFuellen_r(t, k1);
        return;
    }
    if (n.occupied == 0) { n.opaque = true; }
}

// --- surface.cpp ------------------------------------------------------------------------
constexpr float kCullEpsilon = 0.1F;   // CULL_EPSILON
constexpr float kGleichEpsilon = 0.001F;   // EQUAL_EPSILON (VectorCompare)

bool gleichEps(const V3f& a, const V3f& b, float eps) {   // vector3_equal_epsilon
    return std::fabs(a[0] - b[0]) < eps && std::fabs(a[1] - b[1]) < eps && std::fabs(a[2] - b[2]) < eps;
}

bool seiteImPinsel(SichtSeite& s, const SichtPinsel& b, const Ebenen& ebenen, const std::vector<ShaderDaten>& shader) {   // SideInBrush
    if (s.wicklung.empty() || s.shader < 0) { return true; }
    if (s.culled || (b.compile & C_TRANSLUCENT) != 0U) { return false; }
    for (const SichtSeite& bs : b.seiten) {
        if ((bs.flags.compile & C_NODRAW) != 0U) { return false; }
        const Ebene& e = ebenen.ebene(bs.planenum);
        const int lage = seiteVon(s.wicklung, e);
        if (lage == kVorn || lage == kQuer) { return false; }
        const Ebene& eigen = ebenen.ebene(s.planenum);
        if (lage == kAuf && bs.culled && static_cast<double>(eigen.n[0] * e.n[0] + eigen.n[1] * e.n[1] + eigen.n[2] * e.n[2]) > 0) { return false; }
    }
    const ShaderDaten& si = shader[static_cast<std::size_t>(s.shader)];
    if (si.autosprite || si.polygonOffset) { return false; }
    s.culled = true;
    return true;
}

void seitenVerdecken(std::list<SichtPinsel>& welt, const Ebenen& ebenen, const std::vector<ShaderDaten>& shader, const Baum& t) {   // CullSides
    for (auto b1 = welt.begin(); b1 != welt.end() && !t.abgebrochen(); ++b1) {
        if (b1->seiten.empty()) { continue; }
        for (auto b2 = std::next(b1); b2 != welt.end(); ++b2) {
            if (b2->seiten.empty()) { continue; }
            if (b1->original == b2->original && b1->original != nullptr) { continue; }
            if (!b1->kasten.trifft(b2->kasten)) { continue; }
            for (SichtSeite& s : b1->seiten) { (void)seiteImPinsel(s, *b2, ebenen, shader); }
            for (SichtSeite& s : b2->seiten) { (void)seiteImPinsel(s, *b1, ebenen, shader); }
            for (SichtSeite& s1 : b1->seiten) {
                const Wk& w1 = s1.wicklung;
                if (w1.empty() || s1.shader < 0) { continue; }
                const int zahl = static_cast<int>(w1.size());
                for (SichtSeite& s2 : b2->seiten) {
                    const Wk& w2 = s2.wicklung;
                    if (w2.empty() || s2.shader < 0 || w1.size() != w2.size()) { continue; }
                    if (s1.culled && s2.culled) { continue; }
                    if ((s1.planenum & ~1) != (s2.planenum & ~1)) { continue; }
                    const ShaderDaten& si1 = shader[static_cast<std::size_t>(s1.shader)];
                    const ShaderDaten& si2 = shader[static_cast<std::size_t>(s2.shader)];
                    if (si1.autosprite || si1.polygonOffset || si2.autosprite || si2.polygonOffset) { continue; }
                    int erster = -1;
                    for (int k = 0; k < zahl; ++k) {
                        if (gleichEps(w1[0], w2[static_cast<std::size_t>(k)], kGleichEpsilon)) {
                            erster = k;
                            break;
                        }
                    }
                    if (erster == -1) { continue; }
                    int zweiter = erster + 1 < zahl ? erster + 1 : 0;
                    int richtung = 0;
                    if (gleichEps(w1[1], w2[static_cast<std::size_t>(zweiter)], kCullEpsilon)) {
                        richtung = 1;
                    } else {
                        zweiter = erster > 0 ? erster - 1 : zahl - 1;
                        if (gleichEps(w1[1], w2[static_cast<std::size_t>(zweiter)], kCullEpsilon)) { richtung = -1; }
                    }
                    if (richtung == 0) { continue; }
                    int l = erster;
                    bool alle = true;
                    for (int k = 0; k < zahl; ++k) {
                        if (!gleichEps(w1[static_cast<std::size_t>(k)], w2[static_cast<std::size_t>(l)], kCullEpsilon)) {
                            alle = false;
                            break;
                        }
                        l += richtung;
                        if (l < 0) {
                            l = zahl - 1;
                        } else if (l >= zahl) {
                            l = 0;
                        }
                    }
                    if (!alle) { continue; }
                    if (!s2.culled && (s2.flags.compile & C_TRANSLUCENT) == 0U && (s2.flags.compile & C_NODRAW) == 0U) { s1.culled = true; }
                    if (s1.planenum == s2.planenum && s1.culled) { continue; }
                    if (!s1.culled && (s1.flags.compile & C_TRANSLUCENT) == 0U && (s1.flags.compile & C_NODRAW) == 0U) { s2.culled = true; }
                }
            }
        }
    }
}

void seiteInBaum_r(const Wk& w, SichtSeite& s, const Baum& t, int knoten, const Ebenen& ebenen) {   // ClipSideIntoTree_r
    if (w.empty()) { return; }
    const Knoten& n = t.k[static_cast<std::size_t>(knoten)];
    if (n.planenum != kBlatt) {
        if (s.planenum == n.planenum) {
            seiteInBaum_r(w, s, t, n.kinder[0], ebenen);
            return;
        }
        if (s.planenum == (n.planenum ^ 1)) {
            seiteInBaum_r(w, s, t, n.kinder[1], ebenen);
            return;
        }
        auto [vorn, hinten] = teileStreng(w, ebenen.ebene(n.planenum), static_cast<float>(kOnEpsilon));
        if (vorn.empty() && hinten.empty()) {   // "in doubt, register it in both nodes"
            seiteInBaum_r(w, s, t, n.kinder[0], ebenen);
            seiteInBaum_r(w, s, t, n.kinder[1], ebenen);
        } else {
            seiteInBaum_r(vorn, s, t, n.kinder[0], ebenen);
            seiteInBaum_r(hinten, s, t, n.kinder[1], ebenen);
        }
        return;
    }
    if (!n.opaque) {
        const Ebene& e = ebenen.ebene(s.planenum);
        zurHuelle(w, s.huelle, V3f{e.n[0], e.n[1], e.n[2]});
    }
}


// --- map.cpp / bsp.cpp: Karte lesen, ProcessWorldModel ----------------------------------
struct PatchRoh {   // parseMesh_t (der Teil fuer die Marken)
    int breite = 0;
    int hoehe = 0;
    std::vector<std::array<float, 3>> punkte;
    int shader = -1;
};

struct EntBau {   // entity_t beim Lesen
    std::list<SichtPinsel> pinsel;
    std::vector<PatchRoh> patches;
    V3f ursprungsPinsel{0, 0, 0};   // originbrush_origin
    V3f ursprung{0, 0, 0};          // "origin" (MergeOrigin schreibt ihn um)
};

V3f vektorAus(const std::string& s) {   // entity_t::vectorForKey: fehlt der Schluessel, 0 0 0
    V3f v{0, 0, 0};
    float x = 0;
    float y = 0;
    float z = 0;
    if (std::sscanf(s.c_str(), "%f %f %f", &x, &y, &z) == 3) { v = {x, y, z}; }
    return v;
}

// AdjustBrushesForOrigin
void ursprungVerschieben(EntBau& ent, const V3f& offset, Ebenen& ebenen) {
    if (offset[0] == 0 && offset[1] == 0 && offset[2] == 0) { return; }
    for (SichtPinsel& b : ent.pinsel) {
        for (SichtSeite& s : b.seiten) {
            Ebene e = ebenen.ebene(s.planenum);   // plane3_translated( Plane3f, Vector3 )
            e.d = static_cast<float>(static_cast<double>(e.d) + static_cast<double>(e.n[0] * offset[0] + e.n[1] * offset[1] + e.n[2] * offset[2]));
            s.planenum = ebenen.finde(e, static_cast<const V3f*>(nullptr), 0);
            s.ebene = ebenen.ebene(s.planenum);
            s.ebeneD.d = s.ebeneD.d + (s.ebeneD.n[0] * offset[0] + s.ebeneD.n[1] * offset[1] + s.ebeneD.n[2] * offset[2]);
            for (V3f& v : s.wicklung) {
                for (std::size_t i = 0; i < 3; ++i) { v[i] += offset[i]; }
            }
        }
        for (std::size_t i = 0; i < 3; ++i) {
            b.kasten.mins[i] += offset[i];
            b.kasten.maxs[i] += offset[i];
        }
    }
    for (PatchRoh& p : ent.patches) {
        for (auto& v : p.punkte) {
            for (std::size_t i = 0; i < 3; ++i) { v[i] += offset[i]; }
        }
    }
}

// Die Ebenenpunkte, wie der Editor sie in die .map schreibt (ebenenPunkteSchreiben: auf Wunsch
// ganzzahlig, sonst "%10.10lf") und q3map2 sie liest (atof)
V3d punktWieGeschrieben(const std::array<double, 3>& p, bool ganz) {
    V3d r{};
    for (std::size_t i = 0; i < 3; ++i) {
        const double v = ganz ? static_cast<double>(std::lrint(p[i])) : p[i];
        r[i] = std::strtod(dezimal(v).c_str(), nullptr);
    }
    return r;
}

// ParseRawBrush (Ebene und Flags einer Seite), RemoveDuplicateBrushPlanes, SetBrushContents, FinishBrush
void pinselLesen(const std::vector<MarkenEingabe::Seite>& seiten, bool ganz, bool welt, bool funcGroup, EntBau& ent, Ebenen& ebenen,
                 const std::vector<ShaderDaten>& shader) {
    SichtPinsel b;
    for (const MarkenEingabe::Seite& f : seiten) {
        const V3d pt[3] = {punktWieGeschrieben(f.punkte[0], ganz), punktWieGeschrieben(f.punkte[1], ganz), punktWieGeschrieben(f.punkte[2], ganz)};
        // MapPlaneFromPoints: PlaneFromPoints in double, FindFloatPlane( Plane3f( plane ), p )
        EbeneD pd;
        const V3d a{pt[2][0] - pt[0][0], pt[2][1] - pt[0][1], pt[2][2] - pt[0][2]};
        const V3d c{pt[1][0] - pt[0][0], pt[1][1] - pt[0][1], pt[1][2] - pt[0][2]};
        pd.n = {a[1] * c[2] - a[2] * c[1], a[2] * c[0] - a[0] * c[2], a[0] * c[1] - a[1] * c[0]};
        const double l = std::sqrt(pd.n[0] * pd.n[0] + pd.n[1] * pd.n[1] + pd.n[2] * pd.n[2]);
        if (l == 0) {
            pd.n = {0, 0, 0};
            pd.d = 0;
        } else {
            for (double& v : pd.n) { v /= l; }
            pd.d = pt[0][0] * pd.n[0] + pt[0][1] * pd.n[1] + pt[0][2] * pd.n[2];
        }
        Ebene pf;
        for (std::size_t k = 0; k < 3; ++k) { pf.n[k] = static_cast<float>(pd.n[k]); }
        pf.d = static_cast<float>(pd.d);
        SichtSeite s;
        s.planenum = ebenen.finde(pf, pt, 3);
        if (s.planenum >= 0) { s.ebene = ebenen.ebene(s.planenum); }
        s.ebeneD = pd;
        s.shader = f.shader;
        s.flags = shader[static_cast<std::size_t>(s.shader)].flags;
        if ((f.inhalt & C_DETAIL) != 0U) { s.flags.compile |= C_DETAIL; }   // Detail-Bit der .map
        b.seiten.push_back(std::move(s));
    }
    // RemoveDuplicateBrushPlanes
    b.seiten.erase(std::remove_if(b.seiten.begin(), b.seiten.end(), [](const SichtSeite& s) { return s.planenum == -1; }), b.seiten.end());
    for (std::size_t i = 0; i < b.seiten.size(); ++i) {
        for (std::size_t j = i + 1; j < b.seiten.size();) {
            if (b.seiten[i].planenum == b.seiten[j].planenum) {
                b.seiten.erase(b.seiten.begin() + static_cast<std::ptrdiff_t>(j));
            } else if (b.seiten[i].planenum == (b.seiten[j].planenum ^ 1)) {
                return;   // mirrored plane, brush is invalid
            } else {
                ++j;
            }
        }
    }
    if (b.seiten.empty()) { return; }
    // SetBrushContents (nur compileFlags, detail und opaque)
    std::uint32_t compile = b.seiten[0].flags.compile;
    for (std::size_t i = 1; i < b.seiten.size(); ++i) { compile |= b.seiten[i].flags.compile; }
    if ((compile & C_DETAIL) != 0U && (compile & C_STRUCTURAL) != 0U) { compile &= ~C_DETAIL; }
    if ((compile & C_TRANSLUCENT) != 0U && (compile & C_STRUCTURAL) == 0U) { compile |= C_DETAIL; }
    b.detail = (compile & C_DETAIL) != 0U;
    b.opaque = (compile & C_TRANSLUCENT) == 0U;
    b.compile = compile;
    // FinishBrush
    if (!wicklungenBauen(b, ebenen)) { return; }
    if ((b.compile & C_ORIGIN) != 0U) {
        if (welt) { return; }   // "origin brushes not allowed in world"
        // MergeOrigin( mapEnt, buildBrush.minmax.origin() ): origin = origin + o - originbrush_origin
        V3f o;
        for (std::size_t i = 0; i < 3; ++i) { o[i] = static_cast<float>((static_cast<double>(b.kasten.mins[i]) + b.kasten.maxs[i]) * 0.5); }
        for (std::size_t i = 0; i < 3; ++i) { ent.ursprung[i] = ent.ursprung[i] + o[i] - ent.ursprungsPinsel[i]; }
        ent.ursprungsPinsel = o;
        return;
    }
    if ((b.compile & C_AREAPORTAL) != 0U) {
        if (!welt && !funcGroup) { return; }   // "areaportals only allowed in world"
        // FixAreaportalBrush: von mehreren Areaportal-Seiten bleibt die groesste
        if (std::count_if(b.seiten.begin(), b.seiten.end(), [](const SichtSeite& s) { return (s.flags.compile & C_AREAPORTAL) != 0U; }) > 1) {
            SichtSeite* beste = nullptr;
            float besteFlaeche = 0;
            for (SichtSeite& s : b.seiten) {
                if ((s.flags.compile & C_AREAPORTAL) == 0U) { continue; }
                const float f = flaeche(s.wicklung);
                if (besteFlaeche < f) {
                    besteFlaeche = f;
                    beste = &s;
                }
            }
            for (SichtSeite& s : b.seiten) {
                if (&s != beste) { s.flags.compile &= ~C_AREAPORTAL; }
            }
        }
    }
    const float mins[3] = {b.kasten.mins[0], b.kasten.mins[1], b.kasten.mins[2]};
    const float maxs[3] = {b.kasten.maxs[0], b.kasten.maxs[1], b.kasten.maxs[2]};
    (void)bevelsDazu(b.seiten, mins, maxs, [&ebenen](SichtSeite& s, Ebene& plane, const V3f* punkt) {
        s.planenum = ebenen.finde(plane, punkt, punkt != nullptr ? 1 : 0);
        s.ebene = ebenen.ebene(s.planenum);
        s.shader = -1;
    });
    // opake Pinsel vorn, durchsichtige hinten
    SichtPinsel& neu = b.opaque ? ent.pinsel.emplace_front(std::move(b)) : ent.pinsel.emplace_back(std::move(b));
    neu.original = &neu;
}

// --- surface.cpp: FilterDrawsurfsIntoTree -------------------------------------------------
// Die Verweise der Blaetter auf die Flaechen (marksurfaces): AddReferenceToLeaf - nur offene Blaetter,
// jede Flaeche einmal je Blatt.
struct Verweise {
    std::vector<std::vector<int>> jeKnoten;
    int flaeche = -1;   // die gerade gefilterte Flaeche

    int dazu(const Baum& t, int knoten) {
        const Knoten& n = t.k[static_cast<std::size_t>(knoten)];
        if (n.planenum != kBlatt || n.opaque) { return 0; }
        std::vector<int>& l = jeKnoten[static_cast<std::size_t>(knoten)];
        if (std::find(l.begin(), l.end(), flaeche) != l.end()) { return 0; }
        l.push_back(flaeche);
        return 1;
    }
};

// WindingOnNodePlane (PR #316 35043d1d8a, M4)
bool aufKnotenEbene(const Wk& w, const Ebene& eigen, const Ebene& knoten) {
    if (w.empty()) { return false; }
    for (const V3f& v : w) {
        const double d = abstand(eigen, v);
        V3f p;
        for (std::size_t i = 0; i < 3; ++i) { p[i] = v[i] - static_cast<float>(eigen.n[i] * d); }   // v - normal * dist
        if (std::fabs(abstand(knoten, p)) > kOnEpsilon) { return false; }
    }
    return true;
}

// FilterWindingIntoTree_r am Kopfknoten (Abschluss rc607): "'fatten' the winding by the shader mins/maxs
// (parsed from vertexDeform move)" - die Wicklung, dazu um mins und um maxs verschoben, jede der drei
// mit ihrem ersten Punkt als Schluss. bhed_q3map2 schrieb zwei Schlusspunkte eine Stelle zu frueh (die
// Stellen 2n+1 und 3n+2 blieben uninitialisiert, die Wahl der Blaetter zufaellig) - dort jetzt behoben, hier gleich.
Wk verdickt(const Wk& w, const ShaderDaten& si) {
    if (!si.bewegt || w.empty()) { return w; }
    const std::size_t n = w.size();
    const auto plus = [](const V3f& a, const std::array<float, 3>& b) { return V3f{a[0] + b[0], a[1] + b[1], a[2] + b[2]}; };
    Wk fett(n * 3 + 3);
    for (std::size_t i = 0; i < n; ++i) {
        fett[i] = w[i];
        fett[i + (n + 1)] = plus(w[i], si.bewegtMin);
        fett[i + (n + 1) * 2] = plus(w[i], si.bewegtMax);
    }
    fett[n] = w[0];
    fett[n * 2 + 1] = plus(w[0], si.bewegtMin);
    fett[n * 3 + 2] = plus(w[0], si.bewegtMax);
    return fett;
}

// FilterWindingIntoTree_r (die Verdickung am Kopfknoten macht der Aufrufer mit verdickt())
int wicklungInBaum_r(Wk w, int planeNum, const Baum& t, int knoten, const Ebenen& ebenen, Verweise& v) {
    const Knoten& n = t.k[static_cast<std::size_t>(knoten)];
    if (n.planenum == kBlatt) { return v.dazu(t, knoten); }
    const Ebene ebene1 = ebenen.ebene(n.planenum);
    if (planeNum >= 0) {   // die Flaeche hat eine Ebene: hier zwingend filtern
        if (planeNum == n.planenum) { return wicklungInBaum_r(std::move(w), planeNum, t, n.kinder[0], ebenen, v); }
        if (planeNum == (n.planenum ^ 1)) { return wicklungInBaum_r(std::move(w), planeNum, t, n.kinder[1], ebenen, v); }
    }
    int refs = 0;
    if (planeNum >= 0 && aufKnotenEbene(w, ebenen.ebene(planeNum), ebene1)) {
        const Ebene& eigen = ebenen.ebene(planeNum);
        const double dot = static_cast<double>(eigen.n[0] * ebene1.n[0] + eigen.n[1] * ebene1.n[1] + eigen.n[2] * ebene1.n[2]);
        refs += wicklungInBaum_r(w, planeNum, t, n.kinder[dot > 0 ? 0 : 1], ebenen, v);
    }
    auto [vorn, hinten] = teileStreng(w, ebene1, static_cast<float>(kOnEpsilon));
    if (vorn.empty() && hinten.empty()) {   // "same plane, this is an ugly hack"
        refs += wicklungInBaum_r(w, planeNum, t, n.kinder[0], ebenen, v);
        refs += wicklungInBaum_r(std::move(w), planeNum, t, n.kinder[1], ebenen, v);
    }
    if (!vorn.empty()) { refs += wicklungInBaum_r(std::move(vorn), planeNum, t, n.kinder[0], ebenen, v); }
    if (!hinten.empty()) { refs += wicklungInBaum_r(std::move(hinten), planeNum, t, n.kinder[1], ebenen, v); }
    return refs;
}

int punktInBaum_r(const V3f& p, const Baum& t, int knoten, const Ebenen& ebenen, Verweise& v) {   // FilterPointIntoTree_r
    const Knoten& n = t.k[static_cast<std::size_t>(knoten)];
    if (n.planenum == kBlatt) { return v.dazu(t, knoten); }
    const auto d = static_cast<float>(abstand(ebenen.ebene(n.planenum), p));
    int refs = 0;
    if (d >= -kOnEpsilon) { refs += punktInBaum_r(p, t, n.kinder[0], ebenen, v); }
    if (d <= kOnEpsilon) { refs += punktInBaum_r(p, t, n.kinder[1], ebenen, v); }
    return refs;
}

int huelleInBaum_r(const std::array<V3f, 16>& punkte, const Baum& t, int knoten, const Ebenen& ebenen, Verweise& v) {   // FilterPointConvexHullIntoTree_r
    const Knoten& n = t.k[static_cast<std::size_t>(knoten)];
    if (n.planenum == kBlatt) { return v.dazu(t, knoten); }
    const Ebene& e = ebenen.ebene(n.planenum);
    float dmin = std::numeric_limits<float>::max();
    float dmax = std::numeric_limits<float>::lowest();
    for (const V3f& p : punkte) {
        const auto d = static_cast<float>(abstand(e, p));
        dmax = std::max(dmax, d);
        dmin = std::min(dmin, d);
    }
    int refs = 0;
    if (dmax >= -kOnEpsilon) { refs += huelleInBaum_r(punkte, t, n.kinder[0], ebenen, v); }
    if (dmin <= kOnEpsilon) { refs += huelleInBaum_r(punkte, t, n.kinder[1], ebenen, v); }
    return refs;
}

V3f mitteVon(const V3f& a, const V3f& b) {   // vector3_mid
    return {(a[0] + b[0]) * 0.5F, (a[1] + b[1]) * 0.5F, (a[2] + b[2]) * 0.5F};
}

// FilterPatchIntoTree: je 3x3-Block die Huelle aus 16 Punkten, die jede Stufe des Patches umschliesst
int patchInBaum(const PatchRoh& pa, const Baum& t, const Ebenen& ebenen, Verweise& v) {
    int refs = 0;
    const auto& vp = pa.punkte;
    for (int y = 0; y + 2 < pa.hoehe; y += 2) {
        for (int x = 0; x + 2 < pa.breite; x += 2) {
            const auto p = [&](int dy, int dx) { return vp[static_cast<std::size_t>((y + dy) * pa.breite + (x + dx))]; };
            const V3f p0 = p(0, 0);
            const V3f p1 = p(0, 1);
            const V3f p2 = p(0, 2);
            const V3f p3 = p(1, 0);
            const V3f p4 = p(1, 1);
            const V3f p5 = p(1, 2);
            const V3f p6 = p(2, 0);
            const V3f p7 = p(2, 1);
            const V3f p8 = p(2, 2);
            std::array<V3f, 16> pt = {p0, p2, p6, p8, mitteVon(p0, p1), mitteVon(p1, p2), mitteVon(p6, p7), mitteVon(p7, p8),
                                      mitteVon(p0, p3), mitteVon(p3, p6), mitteVon(p2, p5), mitteVon(p5, p8)};
            const V3f p1_ = mitteVon(pt[4], pt[5]);
            const V3f p7_ = mitteVon(pt[6], pt[7]);
            const V3f p3_ = mitteVon(pt[8], pt[9]);
            const V3f p5_ = mitteVon(pt[10], pt[11]);
            V3f p4_;
            for (std::size_t i = 0; i < 3; ++i) { p4_[i] = static_cast<float>(static_cast<double>(p1[i] + p4[i] * 2 + p7[i]) * .25); }
            pt[12] = mitteVon(p3_, p4_);
            pt[13] = mitteVon(p4_, p5_);
            const V3f p4__ = mitteVon(pt[12], pt[13]);
            const V3f m17 = mitteVon(p1_, p7_);
            const V3f p4o{p4__[0] * 2 - m17[0], p4__[1] * 2 - m17[1], p4__[2] * 2 - m17[2]};
            pt[14] = mitteVon(p1_, p4o);
            pt[15] = mitteVon(p4o, p7_);
            refs += huelleInBaum_r(pt, t, t.kopf, ebenen, v);
        }
    }
    return refs;
}

// MakeVisibleBSPFaceList (deepBSP aus)
FlaechenListe sichtbareFlaechen(const std::list<SichtPinsel>& welt) {
    FlaechenListe liste;
    for (const SichtPinsel& b : welt) {
        if (b.detail) { continue; }
        for (const SichtSeite& s : b.seiten) {
            if (s.huelle.empty() || (s.flags.compile & C_SKIP) != 0U) { continue; }
            Flaeche3& f = liste.emplace_front();
            f.w = s.huelle;
            f.planenum = s.planenum & ~1;
            f.compile = s.flags.compile;
            f.pinsel = b.original;
            f.seitenPlanenum = s.planenum;
            f.vorrang = 0;
            if ((f.compile & C_HINT) != 0U) { f.vorrang += kHinweisVorrang; }
            if ((f.compile & C_ANTIPORTAL) != 0U) { f.vorrang += kAntiportalVorrang; }
            if ((f.compile & C_AREAPORTAL) != 0U) { f.vorrang += kAreaportalVorrang; }
            if ((f.compile & C_DETAIL) != 0U) { f.vorrang += kDetailVorrang; }
        }
    }
    return liste;
}

// Ein Baum mit Portalen, den Strukturpinseln und der Flut von den Entitaeten
Baum baumMitFlut(FlaechenListe& flaechen, Ebenen& ebenen, const int blockGroesse[3], const std::list<SichtPinsel>& welt,
                 const std::vector<V3f>& bewohner, Flut& flut, const std::atomic<bool>* abbrechen) {
    Baum t = flaechenBsp(flaechen, ebenen, blockGroesse, abbrechen);
    baumPortale(t, ebenen);
    strukturInBaum(welt, t, ebenen);
    flut = entitaetenFluten(t, bewohner, ebenen);
    return t;
}

struct Zeichenflaeche {   // mapDrawSurface_t (der Teil fuer die Marken)
    SpurFlaeche spur;
    Wk w;                  // Seite: das Polygon
    int planeNum = -1;
    const PatchRoh* patch = nullptr;
    std::vector<V3f> dreiecke;   // misc_model
    const ShaderDaten* si = nullptr;   // shaderInfo (Seiten, Modelle): die Verdickung von deformVertexes move
};

// Die Abschrift der Karte (Hauptfaden): was der Editor in die .map schreiben wuerde
MarkenEingabe eingabeAusKarte(const Karte& k, const Q3ShaderInfoFn& info, bool ja) {
    MarkenEingabe ein;
    ein.ja = ja;
    ein.ganzzahlig = ebenenPunkteGanzzahlig();
    std::map<std::string, int> nummern;
    const auto shaderNr = [&](const std::string& name) {
        const std::string schluessel = klein(name);
        const auto it = nummern.find(schluessel);
        if (it != nummern.end()) { return it->second; }
        ein.shader.push_back(info ? info(name) : Q3ShaderInfo{});
        const int nr = static_cast<int>(ein.shader.size()) - 1;
        nummern.emplace(schluessel, nr);
        return nr;
    };
    const auto weltE = k.welt();
    for (const auto& ep : k.entitaeten()) {
        const Entitaet& e = *ep;
        const std::string klasse = klein(e.klassenname());
        // was der Editor nicht schreibt (kartenformat: leere Gruppen ohne origin)
        if (e.istBehaelter() && e.kinder().empty() && klasse != "worldspawn" && !e.hat("origin")) { continue; }
        MarkenEingabe::Ent ent;
        ent.klasse = klasse;
        ent.ursprung = e.wert("origin");
        ent.welt = ep == weltE;
        for (const char* key : {"_blocksize", "blocksize", "chopsize"}) {
            ent.blockGroesse = e.wert(key);
            if (!ent.blockGroesse.empty()) { break; }
        }
        for (const auto& prim : e.kinder()) {
            if (prim->art() == Primitiv::Art::Pinsel) {
                const auto& p = static_cast<const Pinsel&>(*prim);
                p.bRep(k.kontext);
                if (!p.hatBeitragendeFlaechen(k.kontext)) { continue; }   // pinselSchreiben
                std::vector<MarkenEingabe::Seite> seiten;
                for (std::size_t i = 0; i < p.anzahl(); ++i) {
                    const Flaeche& f = p[i];
                    if (!f.traegtBei()) { continue; }
                    MarkenEingabe::Seite s;
                    for (std::size_t a = 0; a < 3; ++a) {
                        for (std::size_t b = 0; b < 3; ++b) { s.punkte[a][b] = f.ebenenPunkte()[a][b]; }
                    }
                    s.shader = shaderNr(f.shader());
                    s.inhalt = f.flags().inhaltFlags;
                    seiten.push_back(s);
                }
                ent.pinsel.push_back(std::move(seiten));
            } else if (prim->art() == Primitiv::Art::Patch) {
                const auto& pa = static_cast<const Patch&>(*prim);
                MarkenEingabe::Patch r;
                r.breite = static_cast<int>(pa.breite());
                r.hoehe = static_cast<int>(pa.hoehe());
                for (const PatchPunkt& pp : pa.punkte()) { r.punkte.push_back({pp.vertex[0], pp.vertex[1], pp.vertex[2]}); }
                r.shader = shaderNr(pa.shader());
                ent.patches.push_back(std::move(r));
            }
        }
        ein.entitaeten.push_back(std::move(ent));
    }
    // AddTriangleModels: misc_model ohne target in der Welt
    echtModellFlaechen(k, [&](const EchtModellFlaeche& f) {
        if (!f.inWelt || f.ort.size() < 3) { return; }
        MarkenEingabe::Modell m;
        m.shader = shaderNr(f.shader);
        for (const Vektor3f& v : f.ort) { m.dreiecke.push_back({v[0], v[1], v[2]}); }
        ein.modelle.push_back(std::move(m));
    });
    return ein;
}

SpurWelt weltBauen(const MarkenEingabe& ein, MarkenWeltBericht* bericht, const std::atomic<bool>* abbrechen) {
    // R_BoxSurfaces_r (tr_marks.cpp:150 OJ): SURF_NOIMPACT, SURF_NOMARKS, CONTENTS_FOG
    const auto ausgenommen = [](const Q3ShaderFlags& f) {
        return (f.flaeche & (S_SURF_NOIMPACT | S_SURF_NOMARKS)) != 0U || (f.inhalt & S_CONT_FOG) != 0U;
    };
    std::vector<ShaderDaten> shader;
    for (const Q3ShaderInfo& qi : ein.shader) {
        ShaderDaten d;
        d.flags = q3map2ShaderFlags(qi.surfaceparms, ein.ja);
        if (qi.autosprite) {   // deformVertexes autosprite: detail und noClip
            (void)surfaceParmAnwenden("detail", ein.ja, d.flags);
            d.noClip = true;
        }
        d.autosprite = qi.autosprite;
        d.polygonOffset = qi.polygonOffset;
        d.noClip = d.noClip || qi.noClip;
        d.indexed = qi.indexed;
        d.bewegt = qi.bewegt;
        d.bewegtMin = qi.bewegtMin;
        d.bewegtMax = qi.bewegtMax;
        shader.push_back(d);
    }
    Ebenen ebenen;
    EntBau welt;
    std::vector<V3f> bewohner;
    int blockGroesse[3] = {kBlockGroesse, kBlockGroesse, kBlockGroesse};
    for (const MarkenEingabe::Ent& e : ein.entitaeten) {
        const bool funcGroup = e.klasse == "func_group";
        EntBau ent;
        ent.ursprung = vektorAus(e.ursprung);
        for (const auto& seiten : e.pinsel) { pinselLesen(seiten, ein.ganzzahlig, e.welt, funcGroup, ent, ebenen, shader); }
        for (const MarkenEingabe::Patch& pa : e.patches) {
            PatchRoh r;
            r.breite = pa.breite;
            r.hoehe = pa.hoehe;
            r.punkte = pa.punkte;
            r.shader = pa.shader;
            ent.patches.push_back(std::move(r));
        }
        // ParseMapEntity: die Pinsel um den Ursprungs-Pinsel zurueck
        const V3f zurueck{-ent.ursprungsPinsel[0], -ent.ursprungsPinsel[1], -ent.ursprungsPinsel[2]};
        ursprungVerschieben(ent, zurueck, ebenen);
        if (e.klasse == "group_info") { continue; }
        if (e.welt) {
            welt.pinsel.splice(welt.pinsel.end(), ent.pinsel);
            welt.patches.insert(welt.patches.end(), ent.patches.begin(), ent.patches.end());
            if (!e.blockGroesse.empty()) {
                const int n = std::sscanf(e.blockGroesse.c_str(), "%d %d %d", &blockGroesse[0], &blockGroesse[1], &blockGroesse[2]);
                if (n == 1 || n == 2) { blockGroesse[1] = blockGroesse[2] = blockGroesse[0]; }
            }
            continue;
        }
        if (funcGroup) {   // MoveBrushesToWorld
            ursprungVerschieben(ent, ent.ursprung, ebenen);
            for (auto b = ent.pinsel.begin(); b != ent.pinsel.end();) {
                const auto naechster = std::next(b);
                welt.pinsel.splice(b->opaque ? welt.pinsel.begin() : welt.pinsel.end(), ent.pinsel, b);
                b = naechster;
            }
            welt.patches.insert(welt.patches.begin(), ent.patches.begin(), ent.patches.end());
            continue;
        }
        // FloodEntities: Punkt-Entitaeten (Pinsel- und Patch-Entitaeten duerfen draussen sein)
        if (ent.pinsel.empty() && ent.patches.empty() && e.klasse != "_decal") { bewohner.push_back(vektorAus(e.ursprung)); }
    }
    // ProcessWorldModel: der erste Baum aus allen Strukturseiten
    Flut flut = Flut::Leer;
    FlaechenListe flaechen = strukturFlaechen(welt.pinsel);
    Baum baum = baumMitFlut(flaechen, ebenen, blockGroesse, welt.pinsel, bewohner, flut, abbrechen);
    const Flut ersteFlut = flut;
    int verdeckt = 0;
    std::vector<Zeichenflaeche> zf;
    // PatchMapDrawSurfs: die Patches kommen zuerst (ParseMesh: NODRAW wird SF_SKIP)
    for (const PatchRoh& p : welt.patches) {
        const ShaderDaten& si = shader[static_cast<std::size_t>(p.shader)];
        if ((si.flags.flaeche & S_SURF_NODRAW) != 0U) { continue; }
        Zeichenflaeche z;
        z.spur = spurPatch(p.breite, p.hoehe, p.punkte, ausgenommen(si.flags));
        z.patch = &p;
        zf.push_back(std::move(z));
    }
    if (flut != Flut::Leer) {
        aussenFuellen_r(baum, baum.kopf);
        // ClipSidesIntoTree
        seitenVerdecken(welt.pinsel, ebenen, shader, baum);
        for (SichtPinsel& b : welt.pinsel) {
            for (SichtSeite& s : b.seiten) {
                if (s.wicklung.empty()) { continue; }
                s.huelle.clear();
                seiteInBaum_r(s.wicklung, s, baum, baum.kopf, ebenen);
                if (s.huelle.empty() || s.shader < 0) { continue; }
                const ShaderDaten& si = shader[static_cast<std::size_t>(s.shader)];
                // C_NODRAW: ClipSidesIntoTree laesst nur indizierte und Nebel durch, EmitDrawSurfaces
                // (surface.cpp:2977) dann keine Flaeche ausser Patches
                if ((si.flags.compile & C_NODRAW) != 0U) { continue; }
                if (s.culled) {   // DrawSurfaceForSide: keine Flaeche fuer verdeckte Seiten
                    ++verdeckt;
                    continue;
                }
                const Wk& w = (si.autosprite || si.noClip) ? s.wicklung : s.huelle;
                const Ebene& e = ebenen.ebene(s.planenum);
                Zeichenflaeche z;
                z.spur = spurPolygon(w, e.n, ausgenommen(si.flags));
                z.w = w;
                z.planeNum = s.planenum;
                z.si = &si;
                zf.push_back(std::move(z));
                if ((si.flags.compile & C_FOG) != 0U) {   // "make a back side for fog"
                    Zeichenflaeche r;
                    r.w.assign(w.rbegin(), w.rend());
                    r.planeNum = s.planenum ^ 1;
                    r.si = &si;
                    r.spur = spurPolygon(r.w, ebenen.ebene(r.planeNum).n, ausgenommen(si.flags));
                    zf.push_back(std::move(r));
                }
            }
        }
        // der Baum aus den sichtbaren Seiten, noch einmal geflutet (wirft die Portale im Leeren weg)
        FlaechenListe sichtbar = sichtbareFlaechen(welt.pinsel);
        baum = baumMitFlut(sichtbar, ebenen, blockGroesse, welt.pinsel, bewohner, flut, abbrechen);
        aussenFuellen_r(baum, baum.kopf);
    }
    // AddTriangleModels: misc_model ohne target in der Welt, ohne C_NODRAW-Flaechen
    for (const MarkenEingabe::Modell& m : ein.modelle) {
        const ShaderDaten& si = shader[static_cast<std::size_t>(m.shader)];
        if ((si.flags.compile & C_NODRAW) != 0U) { continue; }
        Zeichenflaeche z;
        std::vector<int> indizes;
        for (std::size_t i = 0; i < m.dreiecke.size(); ++i) { indizes.push_back(static_cast<int>(i)); }
        z.dreiecke.assign(m.dreiecke.begin(), m.dreiecke.end());
        z.si = &si;
        z.spur = spurDreiecke(m.dreiecke, indizes, ausgenommen(si.flags));
        zf.push_back(std::move(z));
    }
    if (baum.abgebrochen()) { return SpurWelt{}; }
    // FilterDrawsurfsIntoTree: nur, was ein offenes Blatt beruehrt, kommt in die .bsp
    Verweise v;
    v.jeKnoten.resize(baum.k.size());
    SpurWelt aus;
    MarkenWeltBericht b;
    for (Zeichenflaeche& z : zf) {
        if (z.spur.art == SpurFlaeche::Art::Keine) { continue; }
        v.flaeche = static_cast<int>(aus.flaechen.size());
        int refs = 0;
        if (z.patch != nullptr) {
            refs = patchInBaum(*z.patch, baum, ebenen, v);
        } else if (!z.dreiecke.empty()) {   // FilterTrianglesIntoTree
            for (std::size_t i = 0; i + 2 < z.dreiecke.size(); i += 3) {
                const Wk d{z.dreiecke[i], z.dreiecke[i + 1], z.dreiecke[i + 2]};
                refs += wicklungInBaum_r(z.si != nullptr ? verdickt(d, *z.si) : d, -1, baum, baum.kopf, ebenen, v);
            }
            for (const V3f& p : z.dreiecke) { refs += punktInBaum_r(p, baum, baum.kopf, ebenen, v); }
        } else {   // FilterFaceIntoTree
            refs = wicklungInBaum_r(z.si != nullptr ? verdickt(z.w, *z.si) : z.w, z.planeNum, baum, baum.kopf, ebenen, v);
        }
        if (refs == 0) { continue; }
        if (z.patch != nullptr) {
            ++b.patches;
        } else if (!z.dreiecke.empty()) {
            ++b.modelle;
        } else {
            ++b.seiten;
        }
        aus.flaechen.push_back(std::move(z.spur));
    }
    // der Baum, wie EmitDrawNode/EmitLeaf ihn schreiben (Knoten mit Ebene, Blaetter mit ihren Flaechen)
    std::vector<int> nr(baum.k.size(), 0);
    std::map<int, int> ebenenNr;
    std::vector<int> stapel{baum.kopf};
    // zuerst die Nummern: Knoten >= 0, Blaetter -(n + 1)
    std::vector<int> knotenFolge;
    while (!stapel.empty()) {
        const int k2 = stapel.back();
        stapel.pop_back();
        const Knoten& n = baum.k[static_cast<std::size_t>(k2)];
        if (n.planenum == kBlatt) {
            nr[static_cast<std::size_t>(k2)] = -static_cast<int>(aus.blaetter.size()) - 1;
            SpurWelt::Blatt bl;
            bl.erste = static_cast<int>(aus.blattFlaechen.size());
            const std::vector<int>& l = v.jeKnoten[static_cast<std::size_t>(k2)];
            aus.blattFlaechen.insert(aus.blattFlaechen.end(), l.begin(), l.end());
            bl.zahl = static_cast<int>(l.size());
            aus.blaetter.push_back(bl);
            continue;
        }
        nr[static_cast<std::size_t>(k2)] = static_cast<int>(knotenFolge.size());
        knotenFolge.push_back(k2);
        stapel.push_back(n.kinder[1]);
        stapel.push_back(n.kinder[0]);
    }
    for (const int k2 : knotenFolge) {
        const Knoten& n = baum.k[static_cast<std::size_t>(k2)];
        auto it = ebenenNr.find(n.planenum);
        if (it == ebenenNr.end()) {
            const Ebene& e = ebenen.ebene(n.planenum);
            aus.ebenen.push_back(spurEbene(e.n, e.d));
            it = ebenenNr.emplace(n.planenum, static_cast<int>(aus.ebenen.size()) - 1).first;
        }
        SpurWelt::Knoten sk;
        sk.ebene = it->second;
        sk.kinder[0] = nr[static_cast<std::size_t>(n.kinder[0])];
        sk.kinder[1] = nr[static_cast<std::size_t>(n.kinder[1])];
        aus.knoten.push_back(sk);
    }
    if (aus.knoten.empty()) {   // kein Strukturpinsel: ein Knoten, beide Kinder das eine Blatt (R_BoxSurfaces_r beginnt an Knoten 0)
        const float n[3] = {1, 0, 0};
        aus.ebenen.push_back(spurEbene(n, 0));
        SpurWelt::Knoten sk;
        sk.ebene = 0;
        sk.kinder[0] = -1;
        sk.kinder[1] = -1;
        aus.knoten.push_back(sk);
    }
    aus.gesehen.assign(aus.flaechen.size(), 0);
    if (bericht != nullptr) {
        b.pinsel = static_cast<int>(welt.pinsel.size());
        b.verdeckt = verdeckt;
        b.leck = ersteFlut == Flut::Leck;
        b.leer = ersteFlut == Flut::Leer;
        *bericht = b;
    }
    return aus;
}

}  // namespace sicht
