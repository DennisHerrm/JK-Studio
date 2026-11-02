// metaflaechen.inc - Abschluss rc607: die Zeichenflaechen der Welt, wenn die Karte mit -meta gebaut wird (die
// Vorgabe von NetRadiant-Custom). Eingebunden in sichtbareseiten.inc (Namensraum sicht). q3map2 zerlegt dann jede
// Seite in Dreiecke und setzt sie neu zu Flaechen zusammen; welche Flaechen die ersten 64 im Kasten einer Marke sind
// (R_BoxSurfaces_r) und aus welchen Dreiecken sie bestehen, haengt davon ab.
//
// Ohne -meta laeuft derselbe Weg ohne die Meta-Stufe: T-Kreuzungen, Streifen - die Flaechen sind dann die Seiten selbst.
//
// Portiert aus q3map2 (third_party/q3map2/tools/quake3/q3map2), Ablauf wie ProcessWorldModel nach AddTriangleModels:
//   surface.cpp       DrawSurfaceForSide, SubdivideFace_r, SubdivideFaceSurfaces, ClassifySurface, CalcLightmapAxis,
//                     OptimizeTriangleSurface, EmitTriangleSurface
//   map.cpp           TextureAxisFromPlane, QuakeTextureVecs (und die Lesefaelle in ParseRawBrush)
//   tjunction.cpp     FixTJunctions, AddEdge, InsertPointOnEdge, AddSurfaceEdges, AddPatchEdges, ColinearEdge,
//                     FixSurfaceJunctions, FixBrokenSurface
//   surface_meta.cpp  MakeEntityMetaTriangles, StripFaceSurface, FanFaceSurface, SurfaceToMetaTriangles,
//                     metaTriangle_insert, metaVertex_findOrInsert, SmoothMetaTriangles, MergeMetaTriangles,
//                     MetaTrianglesToSurface, AddMetaTriangleToSurface, AddMetaVertToSurface
// mit denselben Zahlentypen und Epsilons. FixMetaTJunctions ist in q3map2 abgeschaltet (#if 0).
//
// rc608 Teil 13 dazu: fog.cpp FogDrawSurfaces (ChopFaceSurfaceByBrush, ChopPatchSurfaceByBrush, SplitMeshByPlane),
// FinishSurface/CloneSurface (q3map_cloneShader, q3map_backShader), MaxAreaFaceSurface (-maxarea), TriangulatePatchSurface
// (-patchmeta, _patchMeta), npDegrees (-np). Die Schalter kommen aus dem -bsp-Befehl des Build-Eintrags.
// Nicht nachgebaut: Index-Karten (GetIndexedShader, _indexmap), _decal-Entitaeten (MakeEntityDecals), globaler Nebel
// (_fog des worldspawn).
//
//   Copyright (C) 1999-2007 id Software, Inc. and contributors. GPL v2 (LICENSE).

constexpr int kMaxLmEcken = 64;        // maxLMSurfaceVerts (game_quake3 ... game_ja)
constexpr int kMaxEcken = 999;         // maxSurfaceVerts
constexpr int kMaxIndizes = 6000;      // maxSurfaceIndexes
constexpr int kAbtastung = 16;         // DEFAULT_LIGHTMAP_SAMPLE_SIZE
constexpr float kFlachEpsilon = 0.5F;  // PLANAR_EPSILON (surface.cpp)
constexpr float kRaumEpsilon = kGleichEpsilon * 2;   // c_spatial_EQUAL_EPSILON
const V3f kNullV3{0, 0, 0};

// vector3_dot: das Produkt in float, das Ergebnis als double
double skalar(const V3f& a, const V3f& b) { return static_cast<double>(a[0] * b[0] + a[1] * b[1] + a[2] * b[2]); }
double laenge2(const V3f& v) { return skalar(v, v); }   // vector3_length_squared
V3f plus(const V3f& a, const V3f& b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
bool fastGleich(const V3f& a, const V3f& b) {   // VectorCompare
    return std::fabs(b[0] - a[0]) < kGleichEpsilon && std::fabs(b[1] - a[1]) < kGleichEpsilon && std::fabs(b[2] - a[2]) < kGleichEpsilon;
}
bool stFastGleich(const std::array<float, 2>& a, const std::array<float, 2>& b) {   // vector2_equal_epsilon( ..., 1e-4f )
    return std::fabs(b[0] - a[0]) < 1e-4F && std::fabs(b[1] - a[1]) < 1e-4F;
}
float raumAbstand(const V3f& p) {   // spatial_distance
    return static_cast<float>(0.786868F * p[0] + 0.316861F * p[1] + 0.529564F * p[2]);
}

struct MetaEcke {   // bspDrawVert_t: was der Ablauf davon liest (die Farbe ist ohne Index-Karte ueberall 255)
    V3f xyz{0, 0, 0};
    V3f normal{0, 0, 0};
    std::array<float, 2> st{0, 0};
    int kante = -1;       // bspDrawVert_edge_index
    bool tjunc = false;   // bspDrawVert_mark_tjunc
    std::uint8_t alpha = 255;   // color[ 0 ].alpha() (Decals blenden zum Rand aus, ProjectDecalOntoWinding)
};

struct MetaDs {   // mapDrawSurface_t
    enum class Typ { Schlecht, Seite, Patch, Dreiecke, Erzwungen, Meta, Decal };
    Typ typ = Typ::Schlecht;
    const ShaderDaten* si = nullptr;
    const Zeichenflaeche* quelle = nullptr;
    int nummer = 0;          // surfaceNum
    int entityNum = 0;
    int cast = 0;
    int recv = 0;
    int sampleSize = 0;
    float lmScale = 0;
    float shadeAngle = 0;
    V3f ambient{1, 1, 1};
    int planeNum = -1;
    bool planar = false;
    V3f lmAchse{0, 0, 0};    // lightmapAxis
    V3f lmNormale{0, 0, 0};  // lightmapVecs[ 2 ]
    MinMax kasten;
    std::vector<MetaEcke> ecken;
    std::vector<int> indizes;
    int patchBreite = 0;
    int patchHoehe = 0;
    int nebel = -1;            // fogNum
    bool rueckseite = false;   // backSide (q3map_backShader)
    bool seitenRef = false;    // sideRefs nicht leer (aus einer Pinselseite; CloneSurface leert sie)
    [[nodiscard]] int zahl() const { return static_cast<int>(ecken.size()); }
};

// --- map.cpp: die Texturachsen einer Seite ------------------------------------------------
using TexVecs = std::array<std::array<float, 4>, 2>;

// TextureAxisFromPlane
std::array<V3f, 2> texturAchsen(const Ebene& ebene) {
    static const V3f basis[18] = {{0, 0, 1},  {1, 0, 0}, {0, -1, 0},  {0, 0, -1}, {1, 0, 0}, {0, -1, 0},
                                  {1, 0, 0},  {0, 1, 0}, {0, 0, -1},  {-1, 0, 0}, {0, 1, 0}, {0, 0, -1},
                                  {0, 1, 0},  {1, 0, 0}, {0, 0, -1},  {0, -1, 0}, {1, 0, 0}, {0, 0, -1}};
    float best = 0;
    int bestaxis = 0;
    const V3f n{ebene.n[0], ebene.n[1], ebene.n[2]};
    for (int i = 0; i < 6; ++i) {
        const float dot = static_cast<float>(skalar(n, basis[i * 3]));
        if (dot > best + 0.0001F) {
            best = dot;
            bestaxis = i;
        }
    }
    return {basis[bestaxis * 3 + 1], basis[bestaxis * 3 + 2]};
}

// QuakeTextureVecs, mit dem Teil von ParseRawBrush davor (die Verschiebung um ganze Texturgroessen)
TexVecs quakeVecs(const Ebene& ebene, const MarkenEingabe::Textur& t, const ShaderDaten& si) {
    float shift[2] = {t.quake[0], t.quake[1]};
    const float rotate = t.quake[2];
    float scale[2] = {t.quake[3], t.quake[4]};
    if (!si.globalTexture) {
        shift[0] -= static_cast<float>(std::floor(shift[0] / static_cast<float>(si.breite)) * static_cast<float>(si.breite));
        shift[1] -= static_cast<float>(std::floor(shift[1] / static_cast<float>(si.hoehe)) * static_cast<float>(si.hoehe));
    }
    std::array<V3f, 2> vecs = texturAchsen(ebene);
    if (scale[0] == 0) { scale[0] = 1; }
    if (scale[1] == 0) { scale[1] = 1; }
    float sinv = 0;
    float cosv = 1;
    if (rotate == 90) {
        sinv = 1;
        cosv = 0;
    } else if (rotate == 180) {
        sinv = 0;
        cosv = -1;
    } else if (rotate == 270) {
        sinv = -1;
        cosv = 0;
    } else if (rotate != 0) {
        const double ang = static_cast<double>(rotate) * 3.14159265358979323846 / 180.0;   // degrees_to_radians
        sinv = static_cast<float>(std::sin(ang));
        cosv = static_cast<float>(std::cos(ang));
    }
    const std::size_t sv = vecs[0][0] != 0 ? 0U : (vecs[0][1] != 0 ? 1U : 2U);
    const std::size_t tv = vecs[1][0] != 0 ? 0U : (vecs[1][1] != 0 ? 1U : 2U);
    TexVecs aus{};
    for (std::size_t i = 0; i < 2; ++i) {
        const float ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
        const float nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
        vecs[i][sv] = ns;
        vecs[i][tv] = nt;
        for (std::size_t k = 0; k < 3; ++k) { aus[i][k] = vecs[i][k] / scale[i]; }
    }
    aus[0][3] = shift[0];
    aus[1][3] = shift[1];
    return aus;
}

// g_brushSnap (an): die Ecken von DrawSurfaceForSide auf 1/8 Einheit
V3f eingerastet(V3f p) {
    for (std::size_t i = 0; i < 3; ++i) { p[i] = static_cast<float>(0.125 * std::floor(static_cast<double>(p[i]) * 8.0 + 0.5)); }
    return p;
}

// DrawSurfaceForSide: die Ecken einer Seite (ohne Index-Karte)
std::vector<MetaEcke> eckenDerSeite(const Wk& w, const SichtSeite& s, int planeNum, const Ebenen& ebenen, PinselFormat format, const ShaderDaten& si) {
    std::vector<MetaEcke> aus(w.size());
    const Ebene eigen = ebenen.ebene(s.planenum);
    const Ebene e = ebenen.ebene(planeNum);
    const V3f normale{e.n[0], e.n[1], e.n[2]};
    const TexVecs vecs = format == PinselFormat::Quake3 ? quakeVecs(eigen, s.textur, si) : TexVecs{};
    Vektor3f texX;
    Vektor3f texY;
    if (format == PinselFormat::Quake3BP) { berechneAchsenbasis(Vektor3f(e.n[0], e.n[1], e.n[2]), texX, texY); }
    const V3f tx{texX[0], texX[1], texX[2]};
    const V3f ty{texY[0], texY[1], texY[2]};
    for (std::size_t j = 0; j < w.size(); ++j) {
        MetaEcke& dv = aus[j];
        dv.xyz = w[j];
        dv.xyz = eingerastet(dv.xyz);
        if (si.tcGen) {
            dv.st[0] = static_cast<float>(skalar({si.tcVecs[0][0], si.tcVecs[0][1], si.tcVecs[0][2]}, dv.xyz));
            dv.st[1] = static_cast<float>(skalar({si.tcVecs[1][0], si.tcVecs[1][1], si.tcVecs[1][2]}, dv.xyz));
        } else if (format == PinselFormat::Quake3BP) {
            const float x = static_cast<float>(skalar(dv.xyz, tx));
            const float y = static_cast<float>(skalar(dv.xyz, ty));
            dv.st[0] = s.textur.bp[0][0] * x + s.textur.bp[0][1] * y + s.textur.bp[0][2];
            dv.st[1] = s.textur.bp[1][0] * x + s.textur.bp[1][1] * y + s.textur.bp[1][2];
        } else {
            const TexVecs& v = format == PinselFormat::Quake3 ? vecs : s.textur.valve;
            dv.st[0] = static_cast<float>(static_cast<double>(v[0][3]) + skalar({v[0][0], v[0][1], v[0][2]}, dv.xyz));
            dv.st[1] = static_cast<float>(static_cast<double>(v[1][3]) + skalar({v[1][0], v[1][1], v[1][2]}, dv.xyz));
            dv.st[0] /= static_cast<float>(si.breite);
            dv.st[1] /= static_cast<float>(si.hoehe);
        }
        dv.normal = normale;
    }
    return aus;
}

// --- surface.cpp: SubdivideFaceSurfaces -----------------------------------------------------
void seiteUnterteilen_r(Wk w, float teilung, std::vector<Wk>& aus, const std::atomic<bool>* abbrechen) {
    if (w.empty() || (abbrechen != nullptr && abbrechen->load())) { return; }
    MinMax kasten;
    for (const V3f& p : w) { kasten.dazu(p); }
    for (std::size_t achse = 0; achse < 3; ++achse) {
        const int boden = static_cast<int>(std::floor(kasten.mins[achse] / teilung) * teilung);
        const int decke = static_cast<int>(std::ceil(kasten.maxs[achse] / teilung) * teilung);
        V3f punkt{0, 0, 0};
        punkt[achse] = static_cast<float>(boden) + teilung;
        Ebene ebene;
        ebene.n[achse] = -1;
        ebene.d = static_cast<float>(skalar(punkt, {ebene.n[0], ebene.n[1], ebene.n[2]}));
        if (static_cast<float>(decke - boden) > teilung) {
            auto [vorn, hinten] = teile(w, ebene, 0.1F);   // ClipWindingEpsilon (nicht streng)
            if (vorn.empty()) {
                w.swap(hinten);
            } else if (hinten.empty()) {
                w.swap(vorn);
            } else {
                seiteUnterteilen_r(std::move(vorn), teilung, aus, abbrechen);
                seiteUnterteilen_r(std::move(hinten), teilung, aus, abbrechen);
                return;
            }
        }
    }
    aus.push_back(std::move(w));
}

// --- surface.cpp: ClassifySurface ------------------------------------------------------------
V3f lichtAchse(const V3f& n) {   // CalcLightmapAxis
    if (n == kNullV3) { return kNullV3; }
    const V3f a{std::fabs(n[0]), std::fabs(n[1]), std::fabs(n[2])};
    if (a[2] > a[0] - 0.0001F && a[2] > a[1] - 0.0001F) { return n[2] > 0 ? V3f{0, 0, 1} : V3f{0, 0, -1}; }
    if (a[0] > a[1] - 0.0001F && a[0] > a[2] - 0.0001F) { return n[0] > 0 ? V3f{1, 0, 0} : V3f{-1, 0, 0}; }
    return n[1] > 0 ? V3f{0, 1, 0} : V3f{0, -1, 0};
}

void einordnen(MetaDs& ds, Ebenen& ebenen) {
    using Typ = MetaDs::Typ;
    if (ds.typ == Typ::Schlecht || ds.ecken.empty()) { return; }
    const ShaderDaten& si = *ds.si;
    if (ds.typ != Typ::Patch && ds.typ != Typ::Seite && ds.zahl() > kMaxEcken) { ds.typ = Typ::Erzwungen; }
    ds.kasten = MinMax{};
    for (const MetaEcke& v : ds.ecken) { ds.kasten.dazu(v.xyz); }
    Ebene ebene;
    if (ds.planeNum >= 0) {
        ebene = ebenen.ebene(ds.planeNum);
    } else {
        for (const MetaEcke& v : ds.ecken) {
            if (v.normal != kNullV3) {
                for (std::size_t k = 0; k < 3; ++k) { ebene.n[k] = v.normal[k]; }
                ebene.d = static_cast<float>(skalar(v.xyz, v.normal));
                break;
            }
        }
    }
    const V3f n{ebene.n[0], ebene.n[1], ebene.n[2]};
    if (laenge(n) == 0) {
        ds.planar = false;
        ds.planeNum = -1;
    } else {
        ds.planar = true;
        for (const MetaEcke& v : ds.ecken) {
            if (std::fabs(abstand(ebene, v.xyz)) > static_cast<double>(kFlachEpsilon)) {
                ds.planar = false;
                break;
            }
        }
    }
    if (ds.planar) {
        if (ds.planeNum < 0) {
            const V3d p{ds.ecken[0].xyz[0], ds.ecken[0].xyz[1], ds.ecken[0].xyz[2]};
            ds.planeNum = ebenen.finde(ebene, &p, 1);
        }
        ds.lmNormale = n;
    } else {
        ds.planeNum = -1;
        ds.lmNormale = kNullV3;
    }
    if ((si.flags.compile & C_VERTEXLIT) != 0U || ds.typ == Typ::Dreiecke) {
        ds.lmAchse = kNullV3;
        ds.sampleSize = 0;
        return;
    }
    if (si.lmAchse != std::array<float, 3>{0, 0, 0}) {
        ds.lmAchse = si.lmAchse;
    } else if (ds.typ == Typ::Erzwungen) {
        ds.lmAchse = kNullV3;
    } else if (ds.planar) {
        ds.lmAchse = lichtAchse(n);
    } else {
        static const V3f achsen[6] = {{0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}};
        for (const V3f& a : achsen) {
            if (std::none_of(ds.ecken.begin(), ds.ecken.end(), [&a](const MetaEcke& v) { return skalar(v.normal, a) < 0.25F; })) {
                ds.lmAchse = a;
                break;
            }
        }
    }
    if (si.lmSample > 0) {
        ds.sampleSize = si.lmSample;
    } else if (ds.sampleSize <= 0) {
        ds.sampleSize = kAbtastung;
    }
    if (ds.lmScale > 0) {
        ds.sampleSize = static_cast<int>(ds.lmScale * static_cast<float>(ds.sampleSize));
        ds.lmScale = 0;
    }
    ds.sampleSize = std::clamp(ds.sampleSize, 1, 16384);   // std::max( minSampleSize 0, 1 )
}

// --- tjunction.cpp --------------------------------------------------------------------------
constexpr float kLinienEpsilon = 0.25F;    // LINE_POSITION_EPSILON
constexpr double kAufLinieEpsilon = 0.25;  // POINT_ON_LINE_EPSILON
constexpr int kMaxFlaechenEcken = 256;     // MAX_SURFACE_VERTS

struct KantenLinie {   // edgeLine_t
    struct Punkt {
        float abschnitt = 0;   // intercept
        V3f xyz{0, 0, 0};
    };
    V3f normal1{0, 0, 0};
    V3f normal2{0, 0, 0};
    float dist1 = 0;
    float dist2 = 0;
    V3f origin{0, 0, 0};
    V3f dir{0, 0, 0};
    std::vector<Punkt> punkte;
};

struct Kanten {
    struct Original {   // originalEdge_t
        float length = 0;
        MetaEcke* a = nullptr;
        MetaEcke* b = nullptr;
    };
    struct Anfrage {
        MetaEcke* a = nullptr;
        MetaEcke* b = nullptr;
        bool nichtAchsig = false;
        MetaEcke* ziel = nullptr;   // bspDrawVert_edge_index_write( *ziel, Ergebnis )
    };
    std::vector<Original> originale;
    std::vector<KantenLinie> linien;
    std::vector<Anfrage> anfragen;

    static void punktEinfuegen(const V3f& v, KantenLinie& e) {   // InsertPointOnEdge
        KantenLinie::Punkt p;
        p.abschnitt = static_cast<float>(skalar(minus(v, e.origin), e.dir));
        p.xyz = v;
        for (auto it = e.punkte.begin(); it != e.punkte.end(); ++it) {
            if (std::fabs(it->abschnitt - p.abschnitt) < kLinienEpsilon) { return; }
            if (p.abschnitt < it->abschnitt) {
                e.punkte.insert(it, p);
                return;
            }
        }
        e.punkte.push_back(p);
    }

    static bool aufLinie(const KantenLinie& e, const V3f& v1, const V3f& v2) {
        const auto gleich = [](double a, float b) { return std::fabs(static_cast<double>(b) - a) < kAufLinieEpsilon; };
        return gleich(skalar(v1, e.normal1), e.dist1) && gleich(skalar(v1, e.normal2), e.dist2) && gleich(skalar(v2, e.normal1), e.dist1) &&
               gleich(skalar(v2, e.normal2), e.dist2);
    }

    // AddEdge sucht die ERSTE Linie, auf der beide Punkte liegen, unter allen Linien - auf grossen Karten der groesste
    // Teil der Rechnung. Wie BhedKantenAbarbeiten (tjunction.cpp, Stufe 7, gleiches Ergebnis): die Aufrufe haengen nur
    // von den Ecken ab und werden stapelweise erledigt - die Suche unter den Linien vom Anfang des Stapels auf allen
    // Kernen, dann der Stapel der Reihe nach wie AddEdge (ein Treffer ist die erste passende Linie, sonst nur die im
    // Stapel neu entstandenen durchsuchen, sonst eine neue Linie).
    void abarbeiten(const std::atomic<bool>* abbrechen) {
        constexpr std::size_t kStapel = 4096;
        std::vector<int> art(kStapel);   // 0 entartet, 1 spaeter als schraege Kante, 2 suchen
        std::vector<float> laengen(kStapel);
        std::vector<V3f> richtung(kStapel);
        std::vector<long long> treffer(kStapel);
        std::vector<std::size_t> nummern(kStapel);
        std::iota(nummern.begin(), nummern.end(), std::size_t{0});
        for (std::size_t basis = 0; basis < anfragen.size(); basis += kStapel) {
            if (abbrechen != nullptr && abbrechen->load()) { break; }
            const std::size_t n = std::min(kStapel, anfragen.size() - basis);
            const std::size_t vorher = linien.size();
            std::for_each(std::execution::par, nummern.begin(), nummern.begin() + static_cast<std::ptrdiff_t>(n), [&](std::size_t q) {
                const Anfrage& a = anfragen[basis + q];
                V3f dir = minus(a.b->xyz, a.a->xyz);
                float t[3] = {dir[0], dir[1], dir[2]};
                const float d = static_cast<float>(normieren(t));
                dir = {t[0], t[1], t[2]};
                laengen[q] = d;
                richtung[q] = dir;
                treffer[q] = -1;
                if (d < 0.1F) {
                    art[q] = 0;
                    return;
                }
                if (!a.nichtAchsig && std::fabs(dir[0] + dir[1] + dir[2]) != 1) {
                    art[q] = 1;
                    return;
                }
                art[q] = 2;
                for (std::size_t i = 0; i < vorher; ++i) {
                    if (aufLinie(linien[i], a.a->xyz, a.b->xyz)) {
                        treffer[q] = static_cast<long long>(i);
                        break;
                    }
                }
            });
            for (std::size_t q = 0; q < n; ++q) {
                const Anfrage& a = anfragen[basis + q];
                int ergebnis = -1;
                if (art[q] == 1) {
                    originale.push_back(Original{laengen[q], a.a, a.b});
                } else if (art[q] == 2) {
                    long long i = treffer[q];
                    for (std::size_t j = vorher; i < 0 && j < linien.size(); ++j) {
                        if (aufLinie(linien[j], a.a->xyz, a.b->xyz)) { i = static_cast<long long>(j); }
                    }
                    ergebnis = i >= 0 ? static_cast<int>(i) : neueLinie(a.a->xyz, richtung[q]);
                    punktEinfuegen(a.a->xyz, linien[static_cast<std::size_t>(ergebnis)]);
                    punktEinfuegen(a.b->xyz, linien[static_cast<std::size_t>(ergebnis)]);
                }
                if (a.ziel != nullptr) { a.ziel->kante = ergebnis; }
            }
        }
        anfragen.clear();
    }

    int neueLinie(const V3f& v1, const V3f& dir) {
        KantenLinie& e = linien.emplace_back();
        e.origin = v1;
        e.dir = dir;
        // MakeNormalVectors: vector3_min_abs_component_index
        std::size_t mini = std::fabs(dir[1]) < std::fabs(dir[0]) ? 1U : 0U;
        if (std::fabs(dir[2]) < std::fabs(dir[mini])) { mini = 2U; }
        V3f achse{0, 0, 0};
        achse[mini] = 1;
        e.normal1 = normiert(kreuz(achse, dir));
        e.normal2 = kreuz(e.normal1, dir);
        e.dist1 = static_cast<float>(skalar(e.origin, e.normal1));
        e.dist2 = static_cast<float>(skalar(e.origin, e.normal2));
        return static_cast<int>(linien.size()) - 1;
    }
};

bool kollinear(const V3f& v1, const V3f& v2, const V3f& v3) {   // ColinearEdge
    const V3f mitte = minus(v2, v1);
    float t[3] = {v3[0] - v1[0], v3[1] - v1[1], v3[2] - v1[2]};
    if (normieren(t) == 0) { return false; }
    const V3f dir{t[0], t[1], t[2]};
    const float d = static_cast<float>(skalar(mitte, dir));
    const V3f auf{dir[0] * d, dir[1] * d, dir[2] * d};
    return laenge(minus(mitte, auf)) < 0.1;
}

void patchKanten(MetaDs& ds, Kanten& k) {   // AddPatchEdges
    const auto probe = [&ds, &k](int a, int b, int c) {
        std::vector<MetaEcke>& v = ds.ecken;
        if (kollinear(v[static_cast<std::size_t>(a)].xyz, v[static_cast<std::size_t>(b)].xyz, v[static_cast<std::size_t>(c)].xyz)) {
            k.anfragen.push_back({&v[static_cast<std::size_t>(a)], &v[static_cast<std::size_t>(c)], false, nullptr});
        }
    };
    const int b = ds.patchBreite;
    const int h = ds.patchHoehe;
    if (b < 1 || h < 1 || ds.ecken.size() != static_cast<std::size_t>(b) * static_cast<std::size_t>(h)) { return; }
    for (int i = 0; i < b - 2; i += 2) {
        probe(i, i + 1, i + 2);
        probe((h - 1) * b + i, (h - 1) * b + i + 1, (h - 1) * b + i + 2);
    }
    for (int i = 0; i < h - 2; i += 2) {
        probe(i * b, (i + 1) * b, (i + 2) * b);
        probe((b - 1) + i * b, (b - 1) + (i + 1) * b, (b - 1) + (i + 2) * b);
    }
}

void junktionenFuellen(MetaDs& ds, const Kanten& k) {   // FixSurfaceJunctions
    std::vector<int> originals;
    std::vector<MetaEcke> verts;
    for (std::size_t i = 0; i < ds.ecken.size(); ++i) {
        if (verts.size() >= kMaxFlaechenEcken) { return; }   // "MAX_SURFACE_VERTS": q3map2 bricht hier ab
        verts.push_back(ds.ecken[i]);
        originals.push_back(static_cast<int>(i));
        const MetaEcke& v1 = ds.ecken[i];
        const MetaEcke& v2 = ds.ecken[(i + 1) % ds.ecken.size()];
        if (v1.kante == -1) { continue; }
        const KantenLinie& e = k.linien[static_cast<std::size_t>(v1.kante)];
        const float start = static_cast<float>(skalar(minus(v1.xyz, e.origin), e.dir));
        const float ende = static_cast<float>(skalar(minus(v2.xyz, e.origin), e.dir));
        const auto einfuegen = [&](const KantenLinie::Punkt& p) {
            MetaEcke v;
            v.xyz = p.xyz;
            const float frac = (p.abschnitt - start) / (ende - start);
            v.st = {v1.st[0] + (v2.st[0] - v1.st[0]) * frac, v1.st[1] + (v2.st[1] - v1.st[1]) * frac};
            v.normal = v1.normal;
            v.tjunc = true;
            verts.push_back(v);
            originals.push_back(static_cast<int>(i));
        };
        if (start < ende) {
            for (const KantenLinie::Punkt& p : e.punkte) {
                if (static_cast<double>(p.abschnitt) > static_cast<double>(start) + kOnEpsilon) {
                    if (static_cast<double>(p.abschnitt) > static_cast<double>(ende) - kOnEpsilon) { break; }
                    einfuegen(p);
                }
            }
        } else {
            for (auto p = e.punkte.rbegin(); p != e.punkte.rend(); ++p) {
                if (static_cast<double>(p->abschnitt) < static_cast<double>(start) - kOnEpsilon) {
                    if (static_cast<double>(p->abschnitt) < static_cast<double>(ende) + kOnEpsilon) { break; }
                    einfuegen(*p);
                }
            }
        }
    }
    const std::size_t n = verts.size();
    std::size_t i = 0;
    for (; i < n; ++i) {
        if (originals[(i + 1) % n] == originals[i]) { continue; }
        if (originals[(i + n - 1) % n] == originals[(i + n - 2) % n]) { continue; }
        break;
    }
    if (i == 0) {
        ds.ecken = std::move(verts);
        return;
    }
    // "just rotate the vertexes" (um eine Stelle - auch wenn i groesser ist, so steht es in tjunction.cpp)
    std::rotate(verts.begin(), verts.begin() + 1, verts.end());
    ds.ecken = std::move(verts);
}

V3f schweissenF(const V3f& a, const V3f& b) {   // SnapWeldVector
    V3f out{0, 0, 0};
    for (std::size_t i = 0; i < 3; ++i) {
        const float ai = std::rint(a[i]);
        const float bi = std::rint(b[i]);
        if (ai == a[i]) {
            out[i] = a[i];
        } else if (bi == b[i]) {
            out[i] = b[i];
        } else if (std::fabs(ai - a[i]) < std::fabs(bi - b[i])) {
            out[i] = a[i];
        } else {
            out[i] = b[i];
        }
        const float outi = std::rint(out[i]);
        if (static_cast<double>(std::fabs(outi - out[i])) <= kSnapEpsilon) { out[i] = outi; }
    }
    return out;
}

bool flaecheFlicken(MetaDs& ds) {   // FixBrokenSurface
    for (std::size_t i = 0; i < ds.ecken.size(); ++i) {
        MetaEcke& dv1 = ds.ecken[i];
        MetaEcke& dv2 = ds.ecken[(i + 1) % ds.ecken.size()];
        if (laenge(minus(dv1.xyz, dv2.xyz)) < kDegeneriert) {
            MetaEcke avg;
            avg.xyz = schweissenF(dv1.xyz, dv2.xyz);
            avg.normal = normiert(plus(dv1.normal, dv2.normal));
            avg.st = {(dv1.st[0] + dv2.st[0]) * 0.5F, (dv1.st[1] + dv2.st[1]) * 0.5F};
            avg.tjunc = dv1.tjunc && dv2.tjunc;
            const std::size_t weg = (i + 1) % ds.ecken.size();
            dv1 = avg;
            ds.ecken.erase(ds.ecken.begin() + static_cast<std::ptrdiff_t>(weg));
            --i;   // bei 0 laeuft es wie in q3map2 ueber und die Schleife beginnt mit ++i wieder bei 0
        }
    }
    return ds.ecken.size() >= 3;
}

void tJunktionen(std::vector<MetaDs>& alle, const std::atomic<bool>* abbrechen) {   // FixTJunctions
    Kanten k;
    const auto zaehlt = [](const MetaDs& ds) {
        return (ds.si->flags.compile & C_NODRAW) == 0U && !ds.si->autosprite && !ds.si->notjunc && !ds.ecken.empty();
    };
    for (MetaDs& ds : alle) {
        if (!zaehlt(ds)) { continue; }
        if (ds.typ == MetaDs::Typ::Seite) {   // AddSurfaceEdges
            std::size_t vorher = ds.ecken.size() - 1;
            for (std::size_t n = 0; n < ds.ecken.size(); vorher = n++) { k.anfragen.push_back({&ds.ecken[vorher], &ds.ecken[n], false, &ds.ecken[vorher]}); }
        } else if (ds.typ == MetaDs::Typ::Patch) {
            patchKanten(ds, k);
        }
    }
    k.abarbeiten(abbrechen);
    // wie bhed_q3map2 (src/q3map2/kompat/gcc_sort.h): gleich lange Kanten stehen danach in derselben Reihenfolge
    bhed_gcc::ranges_sort(k.originale, {}, &Kanten::Original::length);
    for (const Kanten::Original& e : k.originale) { k.anfragen.push_back({e.a, e.b, true, e.a}); }
    k.abarbeiten(abbrechen);
    if (abbrechen != nullptr && abbrechen->load()) { return; }
    for (MetaDs& ds : alle) {
        if (!zaehlt(ds) || ds.typ != MetaDs::Typ::Seite) { continue; }
        junktionenFuellen(ds, k);
        if (!flaecheFlicken(ds)) {
            ds.typ = MetaDs::Typ::Schlecht;   // ClearSurface
            ds.ecken.clear();
        }
    }
}

// --- surface_meta.cpp ------------------------------------------------------------------------
void faecher(MetaDs& ds, Ebenen& ebenen) {   // FanFaceSurface
    MetaEcke mitte;
    std::array<double, 3> summe{0, 0, 0};
    float alpha = 0;   // Color4f
    for (const MetaEcke& dv : ds.ecken) {
        for (std::size_t k = 0; k < 3; ++k) {
            summe[k] += static_cast<double>(dv.xyz[k]);
            mitte.normal[k] += dv.normal[k];
        }
        mitte.st[0] += dv.st[0];
        mitte.st[1] += dv.st[1];
        alpha += static_cast<float>(dv.alpha);
    }
    mitte.alpha = static_cast<std::uint8_t>(std::clamp(alpha / static_cast<float>(ds.ecken.size()), 0.0F, 255.0F));   // color_to_byte
    const double iv = 1.0 / static_cast<double>(ds.ecken.size());
    for (std::size_t k = 0; k < 3; ++k) { mitte.xyz[k] = static_cast<float>(summe[k] * iv); }
    float t[3] = {mitte.normal[0], mitte.normal[1], mitte.normal[2]};
    if (normieren(t) == 0) {
        mitte.normal = ds.ecken[0].normal;
    } else {
        mitte.normal = {t[0], t[1], t[2]};
    }
    mitte.st[0] = static_cast<float>(static_cast<double>(mitte.st[0]) * iv);
    mitte.st[1] = static_cast<float>(static_cast<double>(mitte.st[1]) * iv);
    ds.ecken.insert(ds.ecken.begin(), mitte);
    ds.indizes.clear();
    for (std::size_t vorher = ds.ecken.size() - 1, n = 1; n != ds.ecken.size(); vorher = n++) {
        ds.indizes.push_back(0);
        ds.indizes.push_back(static_cast<int>(vorher));
        ds.indizes.push_back(static_cast<int>(n));
    }
    einordnen(ds, ebenen);
}

// triangle_min_angle_squared_sin
double kleinsterWinkel(const V3f& a, const V3f& b, const V3f& c) {
    const V3f d[3] = {minus(b, a), minus(c, a), minus(c, b)};
    const double l[3] = {laenge2(d[0]), laenge2(d[1]), laenge2(d[2])};
    const std::size_t mini = (l[0] < l[1]) ? ((l[0] < l[2]) ? 0U : 2U) : ((l[1] < l[2]) ? 1U : 2U);
    if (l[mini] == 0) { return 0; }
    const std::size_t minj = mini == 2 ? 0U : mini + 1;
    const std::size_t mink = minj == 2 ? 0U : minj + 1;
    return laenge2(kreuz(d[minj], d[mink])) / (l[minj] * l[mink]);
}

void streifen(MetaDs& ds, Ebenen& ebenen) {   // StripFaceSurface
    if (ds.ecken.empty() || (ds.typ != MetaDs::Typ::Seite && ds.typ != MetaDs::Typ::Decal)) { return; }
    std::vector<int> indizes;
    if (ds.zahl() == 3) {
        indizes = {0, 1, 2};
    } else {
        int least = 0;
        if (!ds.si->autosprite) {
            for (int i = 0; i < ds.zahl(); ++i) {
                const V3f& v1 = ds.ecken[static_cast<std::size_t>(i)].xyz;
                const V3f& v2 = ds.ecken[static_cast<std::size_t>(least)].xyz;
                if (v1[0] < v2[0] || (v1[0] == v2[0] && v1[1] < v2[1]) || (v1[0] == v2[0] && v1[1] == v2[1] && v1[2] < v2[2])) { least = i; }
            }
        }
        const int n = ds.zahl();
        const auto idx = [n](int i) { return i < 0 ? i + n : (i < n ? i : i - n); };
        const auto tjunc = [&ds](int i) { return ds.ecken[static_cast<std::size_t>(i)].tjunc ? 1 : 0; };
        bool gut = false;
        for (int r = 0; r < n && !gut; ++r) {
            double winkel = std::numeric_limits<double>::max();
            double flaecheX2 = std::numeric_limits<double>::max();
            const auto ordentlich = [&winkel, &flaecheX2] { return winkel > 1e-5 && flaecheX2 > 1.0; };   // TINY_AREA
            indizes.clear();
            int i = idx(r + least);
            int j = idx(i - 1);
            int k = 0;
            int tausch = 0;
            for (;;) {
                // die Schleifenbedingung von StripFaceSurface: von beiden Seiten des Umrisses abwechselnd
                tausch ^= (tjunc(idx(tausch != 0 ? i + 1 : j - 1)) >= tjunc(idx(tausch != 0 ? j - 1 : i + 1))) ? 1 : 0;
                if (tausch != 0) {
                    k = j;
                    --j;
                    j = idx(j);
                } else {
                    k = i;
                    ++i;
                    i = idx(i);
                }
                if (i == j) { break; }
                const V3f& a = ds.ecken[static_cast<std::size_t>(i)].xyz;
                const V3f& b = ds.ecken[static_cast<std::size_t>(j)].xyz;
                const V3f& c = ds.ecken[static_cast<std::size_t>(k)].xyz;
                winkel = std::min(winkel, kleinsterWinkel(a, b, c));
                flaecheX2 = std::min(flaecheX2, laenge(kreuz(minus(b, a), minus(c, a))));
                if (!ordentlich()) { break; }
                indizes.push_back(i);
                indizes.push_back(j);
                indizes.push_back(k);
            }
            gut = ordentlich();
        }
        if (!gut) {
            faecher(ds, ebenen);
            return;
        }
    }
    ds.indizes = std::move(indizes);
    einordnen(ds, ebenen);
}

struct MetaDreieck;
struct MetaVert : MetaEcke {   // metaVertex_t
    std::vector<MetaDreieck*> dreiecke;
    std::list<MetaVert>* gruppe = nullptr;
};
using MetaGruppen = std::multimap<float, std::list<MetaVert>>;

struct MetaDreieck {   // metaTriangle_t
    const ShaderDaten* si = nullptr;   // nullptr: schon verbraucht
    int entityNum = 0;
    int nummer = 0;       // surfaceNum
    int nebel = -1;       // fogNum
    int planeNum = -1;
    int sampleSize = 0;
    int cast = 0;
    int recv = 0;
    float shadeAngle = 0;
    V3f ambient{1, 1, 1};
    Ebene ebene;
    V3f lmAchse{0, 0, 0};
    std::array<MetaVert*, 3> ecken{};
    float kleinstes = std::numeric_limits<float>::max();   // minmax.min
};

bool eckenGleich(const MetaEcke& a, const MetaEcke& b) {   // bspDrawVert_equal
    return fastGleich(a.xyz, b.xyz) && fastGleich(a.normal, b.normal) && stFastGleich(a.st, b.st) && a.alpha == b.alpha;
}

bool mischbar(const MetaDreieck& a, const MetaDreieck& b) {   // CompareMetaTriangles::equal
    return a.si == b.si && a.nebel == b.nebel && a.entityNum == b.entityNum && a.cast == b.cast && a.recv == b.recv && a.sampleSize == b.sampleSize && a.ambient == b.ambient;
}

bool shaderKleiner(const ShaderDaten* a, const ShaderDaten* b) {   // shaderInfo_less: Q_stricmp
    return a != b && a->name < b->name;   // name ist klein geschrieben
}

struct MetaStufe {
    MetaGruppen gruppen;            // metaVerts
    std::list<MetaDreieck> dreiecke;   // metaTriangles
    Ebenen& ebenen;
    float np = 0;                      // npDegrees (-np)

    MetaStufe(Ebenen& e, float npGrad) : ebenen(e), np(npGrad) {}

    MetaVert* eckeFinden(const MetaEcke& src) {   // metaVertex_findOrInsert
        const float d = raumAbstand(src.xyz);
        const auto anfang = gruppen.lower_bound(d - kRaumEpsilon);
        const auto ende = gruppen.upper_bound(d + kRaumEpsilon);
        for (auto it = anfang; it != ende; ++it) {
            for (MetaVert& v : it->second) {
                if (eckenGleich(src, v)) { return &v; }
            }
        }
        const auto neu = [&src](std::list<MetaVert>& liste) {
            MetaVert& v = liste.emplace_back();
            static_cast<MetaEcke&>(v) = src;
            v.gruppe = &liste;
            return &v;
        };
        for (auto it = anfang; it != ende; ++it) {
            if (fastGleich(src.xyz, it->second.front().xyz)) { return neu(it->second); }
        }
        return neu(gruppen.emplace_hint(anfang, d, std::list<MetaVert>{})->second);
    }

    void einfuegen(MetaDreieck src, std::array<MetaEcke, 3> v, int planeNum) {   // metaTriangle_insert
        if (laenge(minus(v[0].xyz, v[1].xyz)) < 0.125 || laenge(minus(v[1].xyz, v[2].xyz)) < 0.125 || laenge(minus(v[2].xyz, v[0].xyz)) < 0.125) { return; }
        if (planeNum >= 0) {
            src.planeNum = planeNum;
            src.ebene = ebenen.ebene(planeNum);
        } else {
            src.planeNum = -1;   // PlaneFromPoints
            const V3f n = kreuz(minus(v[2].xyz, v[0].xyz), minus(v[1].xyz, v[0].xyz));
            float t[3] = {n[0], n[1], n[2]};
            if (normieren(t) == 0) { return; }
            for (int k = 0; k < 3; ++k) { src.ebene.n[k] = t[k]; }
            src.ebene.d = static_cast<float>(skalar(v[0].xyz, {t[0], t[1], t[2]}));
        }
        const V3f normale{src.ebene.n[0], src.ebene.n[1], src.ebene.n[2]};
        for (MetaEcke& e : v) {
            if (laenge(e.normal) == 0) { e.normal = normale; }
        }
        if ((src.si->flags.compile & C_VERTEXLIT) == 0U && src.lmAchse == kNullV3) {
            src.lmAchse = src.si->lmAchse != std::array<float, 3>{0, 0, 0} ? src.si->lmAchse : lichtAchse(normale);
        }
        for (std::size_t i = 0; i < 3; ++i) { src.ecken[i] = eckeFinden(v[i]); }
        for (const MetaDreieck* t1 : src.ecken[0]->dreiecke) {   // "Duplicate or Flipped triangle"
            if (std::find(src.ecken[1]->dreiecke.begin(), src.ecken[1]->dreiecke.end(), t1) != src.ecken[1]->dreiecke.end() &&
                std::find(src.ecken[2]->dreiecke.begin(), src.ecken[2]->dreiecke.end(), t1) != src.ecken[2]->dreiecke.end() && mischbar(src, *t1)) {
                return;
            }
        }
        MetaDreieck& neu = dreiecke.emplace_back(src);
        for (MetaVert* e : neu.ecken) { e->dreiecke.push_back(&neu); }
    }

    void ausFlaeche(const MetaDs& ds) {   // SurfaceToMetaTriangles
        if (ds.zahl() < 3 || ds.indizes.size() < 3) { return; }
        for (std::size_t i = 0; i + 2 < ds.indizes.size(); i += 3) {
            const int a = ds.indizes[i];
            const int b = ds.indizes[i + 1];
            const int c = ds.indizes[i + 2];
            if (a == b || a == c || b == c) { continue; }
            MetaDreieck src;
            src.si = ds.si;
            src.entityNum = ds.entityNum;
            src.nummer = ds.nummer;
            src.nebel = ds.nebel;
            src.cast = ds.cast;
            src.recv = ds.recv;
            src.sampleSize = ds.sampleSize;
            src.shadeAngle = ds.shadeAngle;
            src.ambient = ds.ambient;
            src.lmAchse = ds.lmAchse;
            einfuegen(src, {ds.ecken[static_cast<std::size_t>(a)], ds.ecken[static_cast<std::size_t>(b)], ds.ecken[static_cast<std::size_t>(c)]}, ds.planeNum);
        }
    }

    void glaetten() {   // SmoothMetaTriangles (npDegrees 0: ohne Winkel aus Shader oder Entitaet geschieht nichts)
        const auto bogen = [](float grad) { return static_cast<float>(static_cast<double>(grad) * 3.14159265358979323846 / 180.0); };   // degrees_to_radians
        const float vorgabe = bogen(np);   // defaultShadeAngle
        const double kThetaEpsilon = 0.000001;
        for (auto& [abstandSchluessel, liste] : gruppen) {
            (void)abstandSchluessel;
            if (liste.size() <= 1 && !(liste.size() == 1 && liste.front().dreiecke.size() > 1)) { continue; }
            struct VT {
                MetaVert* ecke = nullptr;
                MetaDreieck* dreieck = nullptr;
                float winkel = 0;
                bool skipped = false;
                bool smoothed = false;
                V3f neu{0, 0, 0};
            };
            std::vector<VT> verts;
            for (MetaVert& v : liste) {
                for (MetaDreieck* t : v.dreiecke) {
                    VT x;
                    x.ecke = &v;
                    x.dreieck = t;
                    verts.push_back(x);
                }
            }
            float groesster = 0;
            for (VT& v : verts) {
                float rad = vorgabe;
                if (v.dreieck->si->shadeAngle > 0) {
                    rad = bogen(v.dreieck->si->shadeAngle);
                } else if (v.dreieck->shadeAngle > 0) {
                    rad = bogen(v.dreieck->shadeAngle);
                }
                v.winkel = rad;
                v.skipped = rad <= 0;
                groesster = std::max(groesster, rad);
            }
            if (groesster <= 0) { continue; }
            for (std::size_t i = 0; i < verts.size(); ++i) {
                VT& v = verts[i];
                if (v.skipped || v.smoothed) { continue; }
                V3f mittel{0, 0, 0};
                std::vector<VT*> gemeinsam;
                std::vector<V3f> stimmen;
                for (std::size_t j = i; j < verts.size(); ++j) {
                    VT& v2 = verts[j];
                    if (v2.skipped || v2.smoothed) { continue; }
                    const float w = std::min(v.winkel, v2.winkel);
                    const double dot = std::clamp(skalar(v.ecke->normal, v2.ecke->normal), -1.0, 1.0);
                    if (std::acos(dot) + kThetaEpsilon >= static_cast<double>(w)) { continue; }
                    gemeinsam.push_back(&v2);
                    const V3f n2 = v2.ecke->normal;
                    if (std::none_of(stimmen.begin(), stimmen.end(), [&n2](const V3f& s) {
                            return std::fabs(s[0] - n2[0]) < 0.01F && std::fabs(s[1] - n2[1]) < 0.01F && std::fabs(s[2] - n2[2]) < 0.01F;
                        })) {
                        mittel = plus(mittel, n2);
                        stimmen.push_back(n2);
                    }
                }
                float t[3] = {mittel[0], mittel[1], mittel[2]};
                if (gemeinsam.size() > 1 && normieren(t) != 0) {
                    for (VT* x : gemeinsam) {
                        x->smoothed = true;
                        x->neu = {t[0], t[1], t[2]};
                    }
                } else {
                    for (VT* x : gemeinsam) { x->skipped = true; }
                }
            }
            if (std::none_of(verts.begin(), verts.end(), [](const VT& v) { return v.smoothed; })) { continue; }
            std::list<MetaVert> neueListe;
            for (VT& v : verts) {
                MetaEcke n = *v.ecke;
                if (v.smoothed) { n.normal = v.neu; }
                auto it = std::find_if(neueListe.begin(), neueListe.end(), [&n](const MetaVert& x) { return eckenGleich(n, x); });
                if (it == neueListe.end()) {
                    MetaVert& e = neueListe.emplace_back();
                    static_cast<MetaEcke&>(e) = n;
                    e.gruppe = &liste;
                    it = std::prev(neueListe.end());
                }
                it->dreiecke.push_back(v.dreieck);
                *std::find(v.dreieck->ecken.begin(), v.dreieck->ecken.end(), v.ecke) = &*it;
            }
            liste.swap(neueListe);
        }
    }

    // AddMetaVertToSurface; < 0: kein Platz mehr
    static int eckeZurFlaeche(const MetaDs& ds, const MetaEcke& dv1, std::vector<MetaEcke>& verts, const std::multimap<float, int>& sortiert, int& deckungsgleich) {
        const float d = raumAbstand(dv1.xyz);
        const auto anfang = sortiert.lower_bound(d - kRaumEpsilon);
        const auto ende = sortiert.upper_bound(d + kRaumEpsilon);
        for (auto it = anfang; it != ende; ++it) {
            const MetaEcke& dv2 = verts[static_cast<std::size_t>(it->second)];
            if (!fastGleich(dv1.xyz, dv2.xyz) || !fastGleich(dv1.normal, dv2.normal)) { continue; }
            ++deckungsgleich;
            if (!stFastGleich(dv1.st, dv2.st) || dv1.alpha != dv2.alpha) { continue; }   // "compare texture coordinates and color"
            return it->second;
        }
        if (static_cast<int>(verts.size()) >= ((ds.si->flags.compile & C_VERTEXLIT) != 0U ? kMaxEcken : kMaxLmEcken)) { return -1000; }   // VERTS_EXCEEDED
        verts.push_back(dv1);
        return static_cast<int>(verts.size()) - 1;
    }

    // AddMetaTriangleToSurface
    int dreieckZurFlaeche(MetaDs& ds, const MetaDreieck& tri, std::vector<MetaEcke>& verts, std::vector<int>& indizes, MinMax& texKasten,
                          std::multimap<float, int>& sortiert, bool nurProbe) const {
        constexpr int kAchse = 100000;   // AXIS_SCORE
        constexpr int kEcke = 10000;     // VERT_SCORE
        constexpr int kFlaeche = 1000;   // SURFACE_SCORE
        constexpr int kSt = 50;          // ST_SCORE
        if (np == 0 && !ds.si->nonplanar && ds.planeNum >= 0) {   // "planar surfaces will only merge with triangles in the same plane"
            if (tri.planeNum >= 0 && tri.planeNum != ds.planeNum) { return 0; }
            const Ebene e = ebenen.ebene(ds.planeNum);
            if (!fastGleich({e.n[0], e.n[1], e.n[2]}, {tri.ebene.n[0], tri.ebene.n[1], tri.ebene.n[2]}) || e.d != tri.ebene.d) { return 0; }
        }
        int score = tri.nummer == ds.nummer ? kFlaeche : 0;
        if ((ds.si->flags.compile & C_VERTEXLIT) != 0U || fastGleich(ds.lmAchse, tri.lmAchse)) {
            score += kAchse;
        } else {
            score = static_cast<int>(static_cast<double>(score) + kAchse * skalar(ds.lmAchse, {tri.ebene.n[0], tri.ebene.n[1], tri.ebene.n[2]}));
        }
        const std::size_t vorher = verts.size();
        int deckung = 0;
        const int ai = eckeZurFlaeche(ds, *tri.ecken[0], verts, sortiert, deckung);
        const int bi = eckeZurFlaeche(ds, *tri.ecken[1], verts, sortiert, deckung);
        const int ci = eckeZurFlaeche(ds, *tri.ecken[2], verts, sortiert, deckung);
        if (ai < 0 || bi < 0 || ci < 0) {
            verts.resize(vorher);
            return 0;
        }
        score += deckung * kEcke;
        MinMax kasten = ds.kasten;
        for (const MetaVert* v : tri.ecken) { kasten.dazu(v->xyz); }
        if ((ds.si->flags.compile & C_VERTEXLIT) == 0U && !indizes.empty() && ds.lmAchse != kNullV3 &&
            (!fastGleich(ds.kasten.mins, kasten.mins) || !fastGleich(ds.kasten.maxs, kasten.maxs))) {
            const float lmMax = static_cast<float>(ds.sampleSize * (ds.si->lmBreite - 1));
            for (std::size_t i = 0; i < 3; ++i) {
                if ((kasten.maxs[i] - kasten.mins[i]) > lmMax) {
                    verts.resize(vorher);
                    return 0;
                }
            }
        }
        MinMax neuTex = texKasten;
        for (const MetaVert* v : tri.ecken) { neuTex.dazu({v->st[0], v->st[1], 0}); }
        if (vorher == 0 || texKasten.umschliesst(neuTex)) {
            score += 4 * kSt;
        } else {
            const auto ganz = [](float f) {   // bhed_to_int (q3map2.h, Stufe 6)
                if (!(f > -2147483648.0F)) { return std::numeric_limits<int>::min(); }
                if (f >= 2147483648.0F) { return std::numeric_limits<int>::max(); }
                return static_cast<int>(f);
            };
            const float w = static_cast<float>(ds.si->breite);
            const float h = static_cast<float>(ds.si->hoehe);
            const int alt0 = ganz((texKasten.maxs[0] - texKasten.mins[0]) * w);
            const int alt1 = ganz((texKasten.maxs[1] - texKasten.mins[1]) * h);
            const int neu0 = ganz((neuTex.maxs[0] - neuTex.mins[0]) * w);
            const int neu1 = ganz((neuTex.maxs[1] - neuTex.mins[1]) * h);
            if (neu0 <= alt0) {
                score += 2 * kSt;
            } else if (alt1 > alt0) {
                score += kSt;
            }
            if (neu1 <= alt1) {
                score += 2 * kSt;
            } else if (alt0 > alt1) {
                score += kSt;
            }
        }
        if (static_cast<int>(indizes.size()) + 3 > kMaxIndizes) {
            verts.resize(vorher);
            return 0;
        }
        if (nurProbe) {
            verts.resize(vorher);
        } else {
            indizes.push_back(ai);
            indizes.push_back(bi);
            indizes.push_back(ci);
            ds.kasten = kasten;
            texKasten = neuTex;
            for (const int id : {ai, bi, ci}) {
                if (static_cast<std::size_t>(id) >= vorher) { sortiert.emplace(raumAbstand(verts[static_cast<std::size_t>(id)].xyz), id); }
            }
        }
        return score;
    }

    // MergeMetaTriangles: sortieren (Shader, Entitaet, Schatten, Abtastung, Farbe, Lage im Raum), MetaTrianglesToSurface
    void zusammensetzen(std::vector<MetaDs>& aus, int& naechsteNummer, const std::atomic<bool>* abbrechen) {
        for (MetaDreieck& t : dreiecke) {
            for (const MetaVert* v : t.ecken) { t.kleinstes = std::min(t.kleinstes, raumAbstand(v->xyz)); }
        }
        dreiecke.sort([](const MetaDreieck& a, const MetaDreieck& b) {
            if (a.si != b.si) { return shaderKleiner(a.si, b.si); }
            if (a.nebel != b.nebel) { return a.nebel < b.nebel; }
            if (a.entityNum != b.entityNum) { return a.entityNum < b.entityNum; }
            if (a.cast != b.cast) { return a.cast < b.cast; }
            if (a.recv != b.recv) { return a.recv < b.recv; }
            if (a.sampleSize != b.sampleSize) { return a.sampleSize < b.sampleSize; }
            if (a.ambient[0] != b.ambient[0]) { return a.ambient[0] < b.ambient[0]; }
            if (a.ambient[1] != b.ambient[1]) { return a.ambient[1] < b.ambient[1]; }
            if (a.ambient[2] != b.ambient[2]) { return a.ambient[2] < b.ambient[2]; }
            return a.kleinstes < b.kleinstes;
        });
        constexpr int kGut = 100000 + 2 * 10000 + 4 * 50;   // DEFAULT_GOOD_SCORE
        constexpr int kReicht = 100000 + 1 * 10000;         // DEFAULT_ADEQUATE_SCORE
        for (MetaDreieck& saat : dreiecke) {
            if (saat.si == nullptr) { continue; }
            if (abbrechen != nullptr && abbrechen->load()) { return; }
            MetaDs ds;
            ds.typ = MetaDs::Typ::Meta;
            ds.si = saat.si;
            ds.entityNum = saat.entityNum;
            ds.nummer = saat.nummer;
            ds.nebel = saat.nebel;
            ds.cast = saat.cast;
            ds.recv = saat.recv;
            ds.planeNum = saat.planeNum;
            ds.sampleSize = saat.sampleSize;
            ds.shadeAngle = saat.shadeAngle;
            ds.ambient = saat.ambient;
            ds.lmAchse = saat.lmAchse;
            ++naechsteNummer;
            std::vector<MetaEcke> verts;
            std::vector<int> indizes;
            MinMax texKasten;
            std::multimap<float, int> sortiert;
            std::vector<MetaDreieck*> wolke;   // testCloud
            const auto wolkeErweitern = [&wolke](MetaDreieck& dreieck) {   // expand_cloud
                for (MetaVert* ecke : dreieck.ecken) {
                    for (MetaVert& g : *ecke->gruppe) {
                        for (MetaDreieck* t : g.dreiecke) {
                            if (t->si != nullptr && mischbar(*t, dreieck) && std::find(wolke.begin(), wolke.end(), t) == wolke.end()) { wolke.push_back(t); }
                        }
                    }
                }
                dreieck.si = nullptr;
                if (const auto it = std::find(wolke.begin(), wolke.end(), &dreieck); it != wolke.end()) { wolke.erase(it); }
            };
            (void)dreieckZurFlaeche(ds, saat, verts, indizes, texKasten, sortiert, false);
            wolkeErweitern(saat);
            for (bool dazu = true; dazu;) {
                MetaDreieck* best = nullptr;
                int bestScore = 0;
                dazu = false;
                for (MetaDreieck* t : wolke) {
                    if (t->si == nullptr) { continue; }
                    const int score = dreieckZurFlaeche(ds, *t, verts, indizes, texKasten, sortiert, true);
                    if (score > bestScore) {
                        best = t;
                        bestScore = score;
                        if (bestScore >= kGut) { break; }
                    }
                }
                if (best != nullptr && bestScore > kReicht) {
                    if (dreieckZurFlaeche(ds, *best, verts, indizes, texKasten, sortiert, false) != 0) { wolkeErweitern(*best); }
                    dazu = true;
                }
            }
            ds.ecken = std::move(verts);
            ds.indizes = std::move(indizes);
            const V3f achse = ds.lmAchse;
            einordnen(ds, ebenen);
            ds.lmAchse = achse;
            aus.push_back(std::move(ds));
        }
    }
};

// OptimizeTriangleSurface
void dreieckeOrdnen(std::vector<int>& ziel) {
    constexpr int kCache = 16;   // VERTEX_CACHE_SIZE
    if (ziel.size() <= kCache) { return; }
    std::vector<int> indizes = ziel;
    std::array<int, kCache + 1> cache{};
    std::copy_n(indizes.begin(), std::min<std::size_t>(kCache + 1, ziel.size()), cache.begin());
    for (std::size_t i = 0; i < ziel.size(); i += 3) {
        int first = -1;
        int best = -1;
        int bestScore = -1;
        for (std::size_t j = 0; j < ziel.size(); j += 3) {
            if (indizes[j] == -1) { continue; }
            if (first < 0) { first = static_cast<int>(j); }
            int score = 0;
            for (int k = 0; k < kCache; ++k) {
                const int c = cache[static_cast<std::size_t>(k)];
                if (indizes[j] == c || indizes[j + 1] == c || indizes[j + 2] == c) { ++score; }
            }
            if (score > bestScore) {
                bestScore = score;
                best = static_cast<int>(j);
            }
            if (score == 3) { break; }
        }
        if (best < 0) { best = first; }
        if (best < 0) { continue; }
        const std::size_t b = static_cast<std::size_t>(best);
        for (std::size_t j = 0; j < 3; ++j) {
            int k = 0;
            for (; k < kCache; ++k) {
                if (indizes[b + j] == cache[static_cast<std::size_t>(k)]) { break; }
            }
            if (k >= kCache) {
                for (k = kCache; k > 0; --k) { cache[static_cast<std::size_t>(k)] = cache[static_cast<std::size_t>(k - 1)]; }
                cache[0] = indizes[b + j];
            }
        }
        ziel[i] = indizes[b];
        ziel[i + 1] = indizes[b + 1];
        ziel[i + 2] = indizes[b + 2];
        indizes[b] = -1;
        indizes[b + 1] = -1;
        indizes[b + 2] = -1;
        while (ziel[i] > ziel[i + 1] || ziel[i] > ziel[i + 2]) {
            std::swap(ziel[i], ziel[i + 1]);
            std::swap(ziel[i + 1], ziel[i + 2]);
        }
    }
}

// EmitTriangleSurface: was der Renderer aus einer Flaeche liest. q3map_invert dreht die Dreiecke, ihre Reihenfolge
// kommt aus OptimizeTriangleSurface.
SpurFlaeche spurAus(const MetaDs& ds, const std::function<bool(const Q3ShaderFlags&)>& ausgenommen) {
    V3f normale = ds.lmNormale;
    std::vector<int> indizes = ds.indizes;
    if (ds.si->invert || ds.rueckseite) {   // "invert the surface if necessary": backSide oder q3map_invert
        for (std::size_t i = 0; i + 2 < indizes.size(); i += 3) { std::swap(indizes[i], indizes[i + 1]); }
        normale = {-normale[0], -normale[1], -normale[2]};
    }
    if (!ds.planar) { normale = kNullV3; }
    if (!ds.si->autosprite) { dreieckeOrdnen(indizes); }
    std::vector<std::array<float, 3>> punkte;
    punkte.reserve(ds.ecken.size());
    for (const MetaEcke& v : ds.ecken) { punkte.push_back({v.xyz[0], v.xyz[1], v.xyz[2]}); }
    const bool suppe = (ds.lmAchse == kNullV3 && !ds.planar) || ds.zahl() > kMaxLmEcken;   // MST_TRIANGLE_SOUP
    const bool aus = ausgenommen(ds.si->flags);
    return suppe ? spurDreiecke(punkte, indizes, aus) : spurFlaecheIndiziert(punkte, indizes, normale.data(), aus);
}

// --- fog.cpp: FogDrawSurfaces ----------------------------------------------------------------
// Ein Patch mit seinen Ecken (mesh_t aus bspDrawVert_t: xyz, normal, st)
struct NebelNetz {
    int breite = 0;
    int hoehe = 0;
    std::vector<MetaEcke> v;
    MetaEcke& at(int h, int w) { return v[static_cast<std::size_t>(h * breite + w)]; }
    [[nodiscard]] const MetaEcke& at(int h, int w) const { return v[static_cast<std::size_t>(h * breite + w)]; }
};

NebelNetz netzGedreht(const NebelNetz& m) {   // RotateMesh
    NebelNetz o;
    o.breite = m.hoehe;
    o.hoehe = m.breite;
    o.v.resize(m.v.size());
    for (int h = 0; h < m.hoehe; ++h) {
        for (int w = 0; w < m.breite; ++w) { o.at(w, m.hoehe - 1 - h) = m.at(h, w); }
    }
    return o;
}

MetaEcke eckeZwischen(const MetaEcke& a, const MetaEcke& b, float t) {   // LerpDrawVertAmount
    MetaEcke o;
    for (std::size_t k = 0; k < 3; ++k) { o.xyz[k] = a.xyz[k] + (b.xyz[k] - a.xyz[k]) * t; }
    o.st = {a.st[0] + (b.st[0] - a.st[0]) * t, a.st[1] + (b.st[1] - a.st[1]) * t};
    float n[3] = {a.normal[0] + (b.normal[0] - a.normal[0]) * t, a.normal[1] + (b.normal[1] - a.normal[1]) * t, a.normal[2] + (b.normal[2] - a.normal[2]) * t};
    o.normal = normieren(n) == 0 ? a.normal : V3f{n[0], n[1], n[2]};
    return o;
}

// SplitMeshByPlane: {vorn, hinten}, die leere Seite ohne Ecken. Laesst sich das Netz nicht schneiden, kommt es ganz
// (womoeglich gedreht) als vorn zurueck.
std::pair<NebelNetz, NebelNetz> netzTeilen(NebelNetz in, const Ebene& e) {
    std::vector<float> d;
    const auto D = [&d, &in](int h, int w) { return d[static_cast<std::size_t>(h * in.breite + w)]; };
    int split = -1;
    for (int runde = 0; runde < 2; ++runde) {
        d.assign(in.v.size(), 0);
        int vorn = 0;
        int hinten = 0;
        for (std::size_t k = 0; k < in.v.size(); ++k) {
            d[k] = static_cast<float>(abstand(e, in.v[k].xyz));
            if (d[k] > kOnEpsilon) {
                ++vorn;
            } else if (d[k] < -kOnEpsilon) {
                ++hinten;
            }
        }
        if (vorn == 0) { return {NebelNetz{}, std::move(in)}; }
        if (hinten == 0) { return {std::move(in), NebelNetz{}}; }
        split = -1;
        for (int w = 0; w < in.breite - 1; ++w) {
            if ((D(0, w) < 0) != (D(0, w + 1) < 0)) {
                split = w;
                break;
            }
        }
        if (split == -1) {
            if (runde == 1) { return {std::move(in), NebelNetz{}}; }   // "No crossing points in patch"
            in = netzGedreht(in);
            continue;
        }
        for (int h = 1; h < in.hoehe; ++h) {   // "multiple crossing points", "differing crossing points"
            for (int w = 0; w < in.breite - 1; ++w) {
                if ((D(h, w) < 0) != (D(h, w + 1) < 0) && w != split) { return {std::move(in), NebelNetz{}}; }
            }
            if ((D(h, split) < 0) == (D(h, split + 1) < 0)) { return {std::move(in), NebelNetz{}}; }
        }
        break;
    }
    int fBreite = split + 2;
    const bool fNaeherung = (fBreite & 1) == 0;
    if (fNaeherung) { ++fBreite; }
    int bBreite = in.breite - split;
    const bool bNaeherung = (bBreite & 1) == 0;
    if (bNaeherung) { ++bBreite; }
    NebelNetz f;
    f.breite = fBreite;
    f.hoehe = in.hoehe;
    f.v.resize(static_cast<std::size_t>(fBreite * in.hoehe));
    NebelNetz b;
    b.breite = bBreite;
    b.hoehe = in.hoehe;
    b.v.resize(static_cast<std::size_t>(bBreite * in.hoehe));
    for (int w = 0; w < in.breite; ++w) {
        for (int h = 0; h < in.hoehe; ++h) {
            if (w <= split) {
                f.at(h, w) = in.at(h, w);
            } else {
                b.at(h, w - split + (bNaeherung ? 1 : 0)) = in.at(h, w);
            }
        }
    }
    for (int h = 0; h < in.hoehe; ++h) {
        const float frac = D(h, split) / (D(h, split) - D(h, split + 1));
        const MetaEcke dv = eckeZwischen(in.at(h, split), in.at(h, split + 1), frac);
        f.at(h, split + 1) = dv;
        if (fNaeherung) { f.at(h, split + 2) = dv; }
        b.at(h, 0) = dv;
        if (bNaeherung) { b.at(h, 1) = dv; }
    }
    if (D(0, 0) > 0) { return {std::move(f), std::move(b)}; }
    return {std::move(b), std::move(f)};
}

NebelNetz netzAus(const PatchRoh& p) {
    NebelNetz m;
    m.breite = p.breite;
    m.hoehe = p.hoehe;
    m.v.resize(p.punkte.size());
    for (std::size_t i = 0; i < p.punkte.size(); ++i) {
        m.v[i].xyz = p.punkte[i];
        if (i < p.st.size()) { m.v[i].st = p.st[i]; }
        if (i < p.normalen.size()) { m.v[i].normal = p.normalen[i]; }
    }
    return m;
}

PatchRoh patchAus(const PatchRoh& vorlage, const NebelNetz& m) {
    PatchRoh p = vorlage;
    p.breite = m.breite;
    p.hoehe = m.hoehe;
    p.punkte.clear();
    p.st.clear();
    p.normalen.clear();
    for (const MetaEcke& e : m.v) {
        p.punkte.push_back(e.xyz);
        p.st.push_back(e.st);
        p.normalen.push_back(e.normal);
    }
    return p;
}

// ChopPatchSurfaceByBrush: nur an der oberen und unteren Ebene des Nebels ("to avoid some messy patch clipping issues").
// Die Stuecke draussen kommen hinten dazu (AllocDrawSurface( ds ): dieselben Werte), das Stueck drinnen ersetzt den Patch.
bool patchImNebel(std::vector<Zeichenflaeche>& zf, std::size_t i, const SichtPinsel& b, const Ebenen& ebenen, std::deque<PatchRoh>& stuecke,
                  const std::function<bool(const Q3ShaderFlags&)>& ausgenommen) {
    if (b.seiten.size() < 6) { return false; }
    NebelNetz m = netzAus(*zf[i].patch);
    std::vector<NebelNetz> aussen;
    for (std::size_t s = 4; s <= 5; ++s) {
        auto [vorn, hinten] = netzTeilen(std::move(m), ebenen.ebene(b.seiten[s].planenum));
        if (hinten.v.empty()) { return false; }   // "nothing actually contained inside"
        m = std::move(hinten);
        if (!vorn.v.empty()) { aussen.push_back(std::move(vorn)); }
    }
    const Zeichenflaeche vorlage = zf[i];
    const auto mitPatch = [&](Zeichenflaeche z, const NebelNetz& netz) {
        stuecke.push_back(patchAus(*vorlage.patch, netz));
        z.patch = &stuecke.back();
        z.spur = spurPatch(z.patch->breite, z.patch->hoehe, z.patch->punkte, ausgenommen(z.si->flags));
        return z;
    };
    for (const NebelNetz& a : aussen) { zf.push_back(mitPatch(vorlage, a)); }
    if (!aussen.empty()) { zf[i] = mitPatch(vorlage, m); }
    return true;
}

// ChopFaceSurfaceByBrush: die Wicklung aus den Ecken der Flaeche (WindingFromDrawSurf, auf 1/8 eingerastet) an jeder Seite
// des Nebels (auch den Fasen) teilen. Die Stuecke draussen werden neue Flaechen (DrawSurfaceForSide, hinten), das Stueck
// drinnen ersetzt die Flaeche - mit der Nummer, die es beim Anlegen hatte (die Stelle wird gleich wieder frei).
bool seiteImNebel(std::vector<Zeichenflaeche>& zf, std::size_t i, const SichtPinsel& b, const Ebenen& ebenen,
                  const std::function<bool(const Q3ShaderFlags&)>& ausgenommen) {
    const int seitenEbene = zf[i].planeNum;   // sideRef.planenum (die Rueckseite eines Nebels: die umgedrehte)
    Wk w;
    for (const V3f& p : zf[i].w) { w.push_back(eingerastet(p)); }
    std::vector<Wk> aussen;
    for (const SichtSeite& s : b.seiten) {
        if (seitenEbene == s.planenum) { return false; }        // "coplanar outfacing (don't fog)"
        if ((seitenEbene ^ 1) == s.planenum) { continue; }      // "coplanar infacing (keep inside)"
        auto [vorn, hinten] = teileStreng(w, ebenen.ebene(s.planenum), static_cast<float>(kOnEpsilon));
        if (hinten.empty()) { return false; }
        if (!vorn.empty()) { aussen.push_back(std::move(vorn)); }
        w.swap(hinten);
    }
    const Zeichenflaeche vorlage = zf[i];
    const auto stueck = [&](const Wk& wi) {
        Zeichenflaeche z = vorlage;
        z.w = wi;
        z.nummer = static_cast<int>(zf.size());
        z.spur = spurPolygon(wi, ebenen.ebene(z.planeNum).n, ausgenommen(z.si->flags));
        return z;
    };
    for (const Wk& a : aussen) { zf.push_back(stueck(a)); }   // newds->fogNum = ds.fogNum
    zf[i] = stueck(w);
    return true;
}

// FogDrawSurfaces: je Nebel (Pinsel mit fogparms, CreateMapFogs) die Flaechen, die es vor ihm gab
void nebelSchneiden(std::vector<Zeichenflaeche>& zf, const std::vector<const SichtPinsel*>& nebel, const Ebenen& ebenen, std::deque<PatchRoh>& stuecke,
                    const std::function<bool(const Q3ShaderFlags&)>& ausgenommen) {
    for (std::size_t n = 0; n < nebel.size(); ++n) {
        const SichtPinsel& b = *nebel[n];
        const std::size_t basis = zf.size();
        for (std::size_t i = 0; i < basis; ++i) {
            if (zf[i].si == nullptr || zf[i].si->keinNebel) { continue; }
            MinMax k;
            if (zf[i].patch != nullptr) {
                for (const auto& p : zf[i].patch->punkte) { k.dazu(p); }
            } else if (!zf[i].dreiecke.empty()) {
                for (const V3f& p : zf[i].dreiecke) { k.dazu(p); }
            } else {
                for (const V3f& p : zf[i].w) { k.dazu(eingerastet(p)); }
            }
            bool schneidet = true;   // MinMax::test
            for (std::size_t a = 0; a < 3; ++a) { schneidet = schneidet && b.kasten.maxs[a] >= k.mins[a] && b.kasten.mins[a] <= k.maxs[a]; }
            if (!schneidet) { continue; }
            bool drin = false;
            if (zf[i].patch != nullptr) {
                drin = patchImNebel(zf, i, b, ebenen, stuecke, ausgenommen);
            } else if (!zf[i].dreiecke.empty()) {
                drin = true;   // "fixme: split triangle surfaces"
            } else if (zf[i].seite != nullptr) {
                drin = seiteImNebel(zf, i, b, ebenen, ausgenommen);
            }
            if (drin) { zf[i].nebel = static_cast<int>(n); }
        }
    }
}

// --- surface_meta.cpp: MaxAreaFaceSurface (-maxarea) ----------------------------------------------
// rand() der C-Laufzeit (UCRT: je Faden; q3map2 ruft srand( 0 ) in main)
struct CZufall {
    std::uint32_t zustand = 0;
    int naechste() {
        zustand = zustand * 214013U + 2531011U;
        return static_cast<int>((zustand >> 16) & 0x7FFFU);
    }
};

// MaxAreaIndexes; < 0: alles entartet
int maxFlaecheIndizes(const MetaEcke* v, int cnt, int* idx, CZufall& zufall) {
    constexpr double kWinzig = 1.0;   // TINY_AREA
    constexpr int kVersuche = 8;      // MAXAREA_MAXTRIES
    if (cnt < 3) { return 0; }
    const auto flaeche2 = [](const V3f& a, const V3f& b, const V3f& c) { return laenge(kreuz(minus(b, a), minus(c, a))); };   // triangle_area2x
    double A = 0;
    for (int i = 1; i + 1 < cnt; ++i) { A += flaeche2(v[0].xyz, v[i].xyz, v[i + 1].xyz); }
    double V = 0;
    for (int i = 0; i < cnt; ++i) { V += laenge(minus(v[(i + 1) % cnt].xyz, v[i].xyz)); }
    double breite = std::sqrt(A) * 0.0125;   // shiftWidth
    const double stufen = std::ceil(std::log(cnt / 1.5) / std::log(2));
    if (A - stufen * V * breite * 2 < 0) { breite = A / (stufen * V * 2); }
    int bestR = 0;
    int bestS = 1;
    int bestT = 2;
    double bestA = -1;
    double bestV = -1;
    for (int r = 0; r + 2 < cnt; ++r) {
        for (int s = r + 1; s + 1 < cnt; ++s) {
            for (int t = s + 1; t < cnt; ++t) {
                const V3f ab = minus(v[s].xyz, v[r].xyz);
                const V3f ac = minus(v[t].xyz, v[r].xyz);
                const V3f bc = minus(v[t].xyz, v[s].xyz);
                const double a = laenge(kreuz(ab, ac));
                const double wert = a - (laenge(ab) - laenge(ac) - laenge(bc)) * breite;
                if (bestA < 0 || wert > bestV) {
                    bestA = a;
                    bestV = wert;
                    bestR = r;
                    bestS = s;
                    bestT = t;
                }
            }
        }
    }
    for (int versuch = 0; versuch < kVersuche; ++versuch) {
        if (versuch != 0) {
            bestR = zufall.naechste() % cnt;
            bestS = zufall.naechste() % cnt;
            bestT = zufall.naechste() % cnt;
            if (bestR == bestS || bestR == bestT || bestS == bestT) { continue; }
            if (bestR > bestS) { std::swap(bestR, bestS); }
            if (bestS > bestT) { std::swap(bestS, bestT); }
            if (bestR > bestS) { std::swap(bestR, bestS); }
            bestA = flaeche2(v[bestR].xyz, v[bestS].xyz, v[bestT].xyz);
        }
        if (bestA < kWinzig) { continue; }
        int i = 0;
        idx[i++] = bestR;
        idx[i++] = bestS;
        idx[i++] = bestT;
        int j = maxFlaecheIndizes(v + bestR, bestS - bestR + 1, idx + i, zufall);
        if (j < 0) { continue; }
        for (j += i; i < j; ++i) { idx[i] += bestR; }
        j = maxFlaecheIndizes(v + bestS, bestT - bestS + 1, idx + i, zufall);
        if (j < 0) { continue; }
        for (j += i; i < j; ++i) { idx[i] += bestS; }
        if (cnt + bestR - bestT + 1 >= 3) {
            std::vector<MetaEcke> puffer(v + bestT, v + cnt);
            puffer.insert(puffer.end(), v, v + bestR + 1);
            j = maxFlaecheIndizes(puffer.data(), cnt + bestR - bestT + 1, idx + i, zufall);
            if (j < 0) { continue; }
            for (j += i; i < j; ++i) { idx[i] = (idx[i] + bestT) % cnt; }
        }
        return i;
    }
    return -1;
}

void maxFlaeche(MetaDs& ds, Ebenen& ebenen, CZufall& zufall) {   // MaxAreaFaceSurface
    if (ds.ecken.empty() || (ds.typ != MetaDs::Typ::Seite && ds.typ != MetaDs::Typ::Decal)) { return; }
    if (ds.zahl() == 3) {
        ds.indizes = {0, 1, 2};
        return;
    }
    ds.indizes.assign(static_cast<std::size_t>(3 * ds.zahl() - 6), 0);
    const int n = maxFlaecheIndizes(ds.ecken.data(), ds.zahl(), ds.indizes.data(), zufall);
    if (n < 0) {   // "whatever we do, it's degenerate"
        ds.indizes.clear();
        streifen(ds, ebenen);
        return;
    }
    ds.indizes.resize(static_cast<std::size_t>(n));
    einordnen(ds, ebenen);
}

// --- surface_meta.cpp: TriangulatePatchSurface (-patchmeta, _patchMeta des worldspawn) ------------------
MetaDs patchDreiecke(const MetaDs& ds, const MarkenEingabe& ein, Ebenen& ebenen) {
    constexpr int kTeilung = 8;   // game_ja (game_sof2): "default patchMeta subdivisions tolerance"
    const float kurve = ds.quelle->patch->kurve;
    const int iterationen = ein.weltPatchTeilungDa ? echtIterationenFuerKurve(kurve, ein.weltPatchTeilung)
                                                   : echtIterationenFuerKurve(kurve, kTeilung / (ein.weltPatchQualitaet == 0 ? 1 : ein.weltPatchQualitaet));
    std::vector<EchtNetzPunkt> netz;
    for (const MetaEcke& e : ds.ecken) {
        netz.push_back(EchtNetzPunkt{Vektor3f(e.xyz[0], e.xyz[1], e.xyz[2]), Vektor3f(e.normal[0], e.normal[1], e.normal[2]), e.st});
    }
    int breite = ds.patchBreite;
    int hoehe = ds.patchHoehe;
    echtNetzTesselieren(netz, breite, hoehe, iterationen);   // TessellatedMesh
    MetaDs neu = ds;   // AllocDrawSurface( ds )
    neu.typ = MetaDs::Typ::Meta;
    neu.patchBreite = 0;
    neu.patchHoehe = 0;
    neu.ecken.clear();
    for (const EchtNetzPunkt& p : netz) {
        MetaEcke e;
        e.xyz = {p.ort[0], p.ort[1], p.ort[2]};
        e.normal = {p.normale[0], p.normale[1], p.normale[2]};
        e.st = p.st;
        neu.ecken.push_back(e);
    }
    neu.indizes.clear();
    for (int y = 0; y + 1 < hoehe; ++y) {   // MeshQuadIterator
        for (int x = 0; x + 1 < breite; ++x) {
            const int pw[5] = {x + (y * breite), x + ((y + 1) * breite), x + 1 + ((y + 1) * breite), x + 1 + (y * breite), x + (y * breite)};
            const int r = (x + y) & 1;
            neu.indizes.insert(neu.indizes.end(), {pw[r], pw[r + 1], pw[r + 2], pw[r], pw[r + 2], pw[r + 3]});
        }
    }
    einordnen(neu, ebenen);
    return neu;
}

// --- decals.cpp: _decal-Entitaeten (ProcessDecals, MakeEntityDecals) -------------------------------------
constexpr int kTeilungPatch = 8;   // game_ja (game_sof2): "default patchMeta subdivisions tolerance" (patchSubdivisions)

bool ebeneAus(const V3f& p0, const V3f& p1, const V3f& p2, Ebene& e) {   // PlaneFromPoints
    const V3f n = kreuz(minus(p2, p0), minus(p1, p0));
    float t[3] = {n[0], n[1], n[2]};
    if (normieren(t) == 0) {
        e.d = 0;
        return false;
    }
    for (int k = 0; k < 3; ++k) { e.n[k] = t[k]; }
    e.d = static_cast<float>(skalar(p0, {t[0], t[1], t[2]}));
    return true;
}

struct DecalWerfer {   // decalProjector_t
    const ShaderDaten* si = nullptr;
    V3f mitte{0, 0, 0};
    float radius = 0;
    int ebenenZahl = 0;
    std::array<Ebene, 6> ebenen{};
    std::array<std::array<float, 4>, 2> texMat{};
};

struct DecalQuelle {   // eine _decal-Entitaet nach ParseMapEntity
    std::vector<PatchRoh> patches;   // in der Reihenfolge von mapEnt.patches (emplace_front)
    V3f ursprung{0, 0, 0};           // e.origin
    bool zielDa = false;             // FindTargetEntity fand etwas
    V3f ziel{0, 0, 0};               // e2->origin
};

// MakeTextureMatrix ("old code", der Zweig mit #if 1)
bool texturMatrix(DecalWerfer& dp, const Ebene& projektion, const MetaEcke& a, const MetaEcke& b, const MetaEcke& c) {
    const auto projiziert = [&projektion](const V3f& p) {   // plane3_project_point (float)
        const double d = abstand(projektion, p);
        V3d r{};
        for (std::size_t k = 0; k < 3; ++k) { r[k] = static_cast<double>(p[k] - static_cast<float>(projektion.n[k] * d)); }
        return r;
    };
    const V3d pa = projiziert(a.xyz);
    const V3d pb = projiziert(b.xyz);
    const V3d pc = projiziert(c.xyz);
    const double bb = static_cast<double>((b.st[0] - a.st[0]) * (c.st[1] - a.st[1]) - (c.st[0] - a.st[0]) * (b.st[1] - a.st[1]));
    if (std::fabs(bb) < 0.00000001) { return false; }
    std::array<V3d, 3> vecs{};
    for (int achse = 0; achse < 2; ++achse) {   // s, dann t
        const double s = static_cast<double>(a.st[0]) + (achse == 0 ? 1.0 : 0.0);
        const double t = static_cast<double>(a.st[1]) + (achse == 0 ? 0.0 : 1.0);
        const double bary0 = ((b.st[0] - s) * (c.st[1] - t) - (c.st[0] - s) * (b.st[1] - t)) / bb;
        const double bary1 = ((c.st[0] - s) * (a.st[1] - t) - (a.st[0] - s) * (c.st[1] - t)) / bb;
        const double bary2 = ((a.st[0] - s) * (b.st[1] - t) - (b.st[0] - s) * (a.st[1] - t)) / bb;
        for (std::size_t k = 0; k < 3; ++k) { vecs[static_cast<std::size_t>(achse)][k] = bary0 * pa[k] + bary1 * pb[k] + bary2 * pc[k] - pa[k]; }
    }
    for (std::size_t i = 0; i < 2; ++i) {
        V3d achse = vecs[i];
        const double laenge = std::sqrt(achse[0] * achse[0] + achse[1] * achse[1] + achse[2] * achse[2]);   // VectorNormalize( DoubleVector3 )
        if (laenge != 0) {
            for (double& v : achse) { v /= laenge; }
        }
        for (std::size_t j = 0; j < 3; ++j) { dp.texMat[i][j] = laenge != 0 ? static_cast<float>(achse[j] / laenge) : 0.0F; }
    }
    for (std::size_t i = 0; i < 2; ++i) {
        dp.texMat[i][3] = static_cast<float>(static_cast<double>(a.st[i]) - skalar(a.xyz, {dp.texMat[i][0], dp.texMat[i][1], dp.texMat[i][2]}));
    }
    return true;
}

// MakeDecalProjector (ein Dreieck)
bool decalWerfer(std::vector<DecalWerfer>& alle, const ShaderDaten& si, const Ebene& projektion, float weite, const std::array<const MetaEcke*, 3>& tri) {
    constexpr std::size_t kMaxWerfer = 1024;   // MAX_PROJECTORS
    if (alle.size() >= kMaxWerfer) { return false; }
    DecalWerfer dp;
    dp.si = &si;
    dp.ebenenZahl = 5;
    if (!texturMatrix(dp, projektion, *tri[0], *tri[1], *tri[2])) { return false; }
    const V3f weg{projektion.n[0] * weite, projektion.n[1] * weite, projektion.n[2] * weite};
    MinMax kasten;
    for (const MetaEcke* v : tri) {
        kasten.dazu(v->xyz);
        kasten.dazu(plus(v->xyz, weg));
    }
    for (std::size_t k = 0; k < 3; ++k) { dp.mitte[k] = (kasten.mins[k] + kasten.maxs[k]) * 0.5F; }   // minmax.origin()
    dp.radius = static_cast<float>(laenge(minus(kasten.maxs, dp.mitte)));
    if (!ebeneAus(tri[0]->xyz, tri[1]->xyz, tri[2]->xyz, dp.ebenen[0])) { return false; }
    for (int k = 0; k < 3; ++k) { dp.ebenen[1].n[k] = -dp.ebenen[0].n[k]; }
    dp.ebenen[1].d = static_cast<float>(skalar(plus(tri[0]->xyz, weg), {dp.ebenen[1].n[0], dp.ebenen[1].n[1], dp.ebenen[1].n[2]}));
    for (std::size_t i = 0; i < 3; ++i) {
        if (!ebeneAus(tri[(i + 1) % 3]->xyz, tri[i]->xyz, plus(tri[i]->xyz, weg), dp.ebenen[i + 2])) { return false; }
    }
    alle.push_back(dp);
    return true;
}

// Die Dreiecke eines tesselierten Netzes je Viereck (MeshQuadIterator::tris)
template <class F>
void netzDreiecke(const std::vector<MetaEcke>& v, int breite, int hoehe, F&& je) {
    for (int y = 0; y + 1 < hoehe; ++y) {
        for (int x = 0; x + 1 < breite; ++x) {
            const int pw[5] = {x + (y * breite), x + ((y + 1) * breite), x + 1 + ((y + 1) * breite), x + 1 + (y * breite), x + (y * breite)};
            const int r = (x + y) & 1;
            const auto e = [&v](int i) { return &v[static_cast<std::size_t>(i)]; };
            je(std::array<const MetaEcke*, 3>{e(pw[r]), e(pw[r + 1]), e(pw[r + 2])});
            je(std::array<const MetaEcke*, 3>{e(pw[r]), e(pw[r + 2]), e(pw[r + 3])});
        }
    }
}

std::vector<MetaEcke> netzTesseliert(const std::vector<MetaEcke>& ecken, int& breite, int& hoehe, float kurve) {   // TessellatedMesh
    std::vector<EchtNetzPunkt> netz;
    for (const MetaEcke& e : ecken) { netz.push_back(EchtNetzPunkt{Vektor3f(e.xyz[0], e.xyz[1], e.xyz[2]), Vektor3f(e.normal[0], e.normal[1], e.normal[2]), e.st}); }
    echtNetzTesselieren(netz, breite, hoehe, echtIterationenFuerKurve(kurve, kTeilungPatch));
    std::vector<MetaEcke> aus;
    for (const EchtNetzPunkt& p : netz) {
        MetaEcke e;
        e.xyz = {p.ort[0], p.ort[1], p.ort[2]};
        e.normal = {p.normale[0], p.normale[1], p.normale[2]};
        e.st = p.st;
        aus.push_back(e);
    }
    return aus;
}

// ProcessDecals: je Patch einer _decal-Entitaet die Werfer, Richtung zum Ziel
std::vector<DecalWerfer> decalWerferAus(const std::vector<DecalQuelle>& quellen, const std::vector<ShaderDaten>& shader) {
    std::vector<DecalWerfer> werfer;
    for (const DecalQuelle& q : quellen) {
        if (q.patches.empty() || !q.zielDa) { continue; }   // "without any patch meshes", "without a valid target"
        MinMax eKasten;   // SetEntityBounds
        for (const PatchRoh& p : q.patches) {
            for (const auto& v : p.punkte) { eKasten.dazu(v); }
        }
        for (const PatchRoh& p : q.patches) {
            V3f ursprung = q.ursprung;
            if (fastGleich(q.ursprung, kNullV3)) {
                for (std::size_t k = 0; k < 3; ++k) { ursprung[k] = (eKasten.mins[k] + eKasten.maxs[k]) * 0.5F; }
            }
            float n[3] = {q.ziel[0] - ursprung[0], q.ziel[1] - ursprung[1], q.ziel[2] - ursprung[2]};
            const float weite = static_cast<float>(normieren(n));
            Ebene projektion;
            for (int k = 0; k < 3; ++k) { projektion.n[k] = n[k]; }
            projektion.d = static_cast<float>(skalar(ursprung, {n[0], n[1], n[2]}));
            if (weite <= 0.125F) { continue; }
            std::vector<MetaEcke> roh(p.punkte.size());   // p.mesh: xyz und st, keine Normalen
            for (std::size_t i = 0; i < roh.size(); ++i) {
                roh[i].xyz = p.punkte[i];
                if (i < p.st.size()) { roh[i].st = p.st[i]; }
            }
            int breite = p.breite;
            int hoehe = p.hoehe;
            std::vector<MetaEcke> netz = netzTesseliert(roh, breite, hoehe, p.kurve);
            for (MetaEcke& e : netz) { e.xyz = plus(e.xyz, q.ursprung); }
            const ShaderDaten& si = shader[static_cast<std::size_t>(p.shader)];
            netzDreiecke(netz, breite, hoehe, [&](const std::array<const MetaEcke*, 3>& tri) { (void)decalWerfer(werfer, si, projektion, weite, tri); });
        }
    }
    return werfer;
}

// ProjectDecalOntoWinding: das Stueck der Wicklung im Werfer wird eine neue Flaeche (ESurfaceType::Decal) hinten
void decalAufWicklung(const DecalWerfer& dp, const MetaDs& ds, Wk w, std::vector<MetaDs>& alle) {
    if (w.size() < 3) { return; }
    Ebene ebene;
    if (!ebeneAus(w[0], w[1], w[2], ebene)) { return; }
    if (skalar({dp.ebenen[0].n[0], dp.ebenen[0].n[1], dp.ebenen[0].n[2]}, {ebene.n[0], ebene.n[1], ebene.n[2]}) < -0.0001F) { return; }
    for (int i = 0; i < dp.ebenenZahl; ++i) {
        auto [vorn, hinten] = teileStreng(w, dp.ebenen[static_cast<std::size_t>(i)], 0.0625F);
        if (hinten.empty()) { return; }
        w.swap(hinten);
    }
    if (w.size() < 3) { return; }
    MetaDs d;
    d.typ = MetaDs::Typ::Decal;
    d.si = dp.si;
    d.nummer = static_cast<int>(alle.size());
    d.entityNum = ds.entityNum;
    d.cast = ds.cast;
    d.recv = ds.recv;
    d.nebel = ds.nebel;
    d.lmScale = ds.lmScale;
    d.shadeAngle = ds.shadeAngle;
    d.ambient = ds.ambient;
    for (const V3f& p : w) {
        MetaEcke e;
        e.xyz = p;
        e.normal = {ebene.n[0], ebene.n[1], ebene.n[2]};
        e.st[0] = static_cast<float>(skalar(p, {dp.texMat[0][0], dp.texMat[0][1], dp.texMat[0][2]}) + static_cast<double>(dp.texMat[0][3]));
        e.st[1] = static_cast<float>(skalar(p, {dp.texMat[1][0], dp.texMat[1][1], dp.texMat[1][2]}) + static_cast<double>(dp.texMat[1][3]));
        const auto d0 = static_cast<float>(abstand(dp.ebenen[0], p));
        const auto d1 = static_cast<float>(abstand(dp.ebenen[1], p));
        e.alpha = static_cast<std::uint8_t>(std::clamp(255.0F * d1 / (d0 + d1), 0.0F, 255.0F));   // color_to_byte( alpha )
        d.ecken.push_back(e);
    }
    alle.push_back(std::move(d));
}

// MakeEntityDecals: je Werfer alle Flaechen, die es da schon gibt (auch die Decals der Werfer davor)
void decalsWerfen(const std::vector<DecalWerfer>& werfer, std::vector<MetaDs>& alle, Ebenen& ebenen) {
    const auto rueckseite = [&ebenen](const DecalWerfer& dp, const MetaDs& ds) {   // "backface check"
        if (!ds.planar) { return false; }
        const Ebene e = ebenen.ebene(ds.planeNum);
        return skalar({dp.ebenen[0].n[0], dp.ebenen[0].n[1], dp.ebenen[0].n[2]}, {e.n[0], e.n[1], e.n[2]}) < -0.0001F;
    };
    for (const DecalWerfer& dp : werfer) {
        const std::size_t zahl = alle.size();
        for (std::size_t i = 0; i < zahl; ++i) {
            const MetaDs ds = alle[i];
            if (ds.typ == MetaDs::Typ::Schlecht || ds.ecken.empty() || ds.si->autosprite || (ds.si->flags.compile & C_NOMARKS) != 0U) { continue; }
            bool imKasten = true;   // ds.minmax.test( dp.center, dp.radius )
            for (std::size_t k = 0; k < 3; ++k) {
                imKasten = imKasten && dp.mitte[k] >= ds.kasten.mins[k] - dp.radius && dp.mitte[k] <= ds.kasten.maxs[k] + dp.radius;
            }
            if (!imKasten) { continue; }
            if (ds.typ == MetaDs::Typ::Seite) {   // ProjectDecalOntoFace
                if (!ds.seitenRef || rueckseite(dp, ds)) { continue; }
                Wk w;
                for (const MetaEcke& e : ds.ecken) { w.push_back(e.xyz); }
                decalAufWicklung(dp, ds, std::move(w), alle);
            } else if (ds.typ == MetaDs::Typ::Patch) {   // ProjectDecalOntoPatch
                if (rueckseite(dp, ds)) { continue; }
                int breite = ds.patchBreite;
                int hoehe = ds.patchHoehe;
                const std::vector<MetaEcke> netz = netzTesseliert(ds.ecken, breite, hoehe, ds.quelle->patch->kurve);
                netzDreiecke(netz, breite, hoehe, [&](const std::array<const MetaEcke*, 3>& tri) { decalAufWicklung(dp, ds, Wk{tri[0]->xyz, tri[1]->xyz, tri[2]->xyz}, alle); });
            } else if (ds.typ == MetaDs::Typ::Dreiecke || ds.typ == MetaDs::Typ::Erzwungen || ds.typ == MetaDs::Typ::Meta) {   // ProjectDecalOntoTriangles
                if ((ds.typ == MetaDs::Typ::Dreiecke && !ds.si->skript) || rueckseite(dp, ds)) { continue; }
                for (std::size_t t = 0; t + 2 < ds.indizes.size(); t += 3) {
                    const auto e = [&ds](std::size_t k) { return ds.ecken[static_cast<std::size_t>(ds.indizes[k])].xyz; };
                    decalAufWicklung(dp, ds, Wk{e(t), e(t + 1), e(t + 2)}, alle);
                }
            }
        }
    }
}

constexpr std::size_t kMaxZeichenflaechen = 0x20000;   // max_map_draw_surfs (q3map2.h): darueber bricht q3map2 den Bau ab

// Die Flaechen, wie q3map2 sie aus `zf` macht (zf in der Reihenfolge von mapDrawSurfs nach FogDrawSurfaces). Mit -meta
// (sonst nur q3map_forceMeta) gehen die Seiten in die Meta-Stufe und kommen zusammengesetzt hinten dazu
// (AllocDrawSurface); was nicht hineingeht, behaelt seinen Platz (Patches, autosprite, Modelle ohne forceMeta). Hier
// entstehen ausserdem die Kopien von FinishSurface und die Dreiecke von -patchmeta. Die Reihenfolge der Dreiecke eines
// Modells bleibt die der Datei: die Nummern seiner Ecken kommen aus dem Modell-Lader von q3map2 (OptimizeTriangleSurface
// haengt daran).
std::vector<Zeichenflaeche> metaFlaechen(std::vector<Zeichenflaeche> zf, Ebenen& ebenen, const MarkenEingabe& ein, const std::vector<DecalWerfer>& werfer,
                                         const std::function<bool(const Q3ShaderFlags&)>& ausgenommen, const std::atomic<bool>* abbrechen) {
    std::vector<MetaDs> alle;
    const auto wie = [](const Zeichenflaeche& z, MetaDs::Typ typ, int nummer) {
        MetaDs ds;
        ds.typ = typ;
        ds.si = z.si;
        ds.quelle = &z;
        ds.nummer = nummer;
        ds.nebel = z.nebel;
        ds.seitenRef = typ == MetaDs::Typ::Seite && z.seite != nullptr;
        if (z.werte != nullptr) {
            ds.entityNum = z.werte->entityNum;
            ds.cast = z.werte->cast;
            ds.recv = z.werte->recv;
            ds.sampleSize = z.werte->sampleSize;
            ds.lmScale = z.werte->lmScale;
            ds.shadeAngle = z.werte->shadeAngle;
            ds.ambient = z.werte->ambient;
        }
        return ds;
    };
    std::vector<MetaDs> geteilt;   // SubdivideFaceSurfaces: die Stuecke kommen hinter alle anderen
    int naechste = static_cast<int>(zf.size());
    for (const Zeichenflaeche& z : zf) {
        if (z.patch != nullptr) {
            MetaDs ds = wie(z, MetaDs::Typ::Patch, z.nummer);
            ds.patchBreite = z.patch->breite;
            ds.patchHoehe = z.patch->hoehe;
            ds.ecken = netzAus(*z.patch).v;
            if (z.si->tcGen) {   // DrawSurfaceForMesh: "explicit shader texcoord generation" (Ursprung der Welt 0 0 0)
                for (MetaEcke& e : ds.ecken) {
                    e.st[0] = static_cast<float>(skalar({z.si->tcVecs[0][0], z.si->tcVecs[0][1], z.si->tcVecs[0][2]}, e.xyz));
                    e.st[1] = static_cast<float>(skalar({z.si->tcVecs[1][0], z.si->tcVecs[1][1], z.si->tcVecs[1][2]}, e.xyz));
                }
            }
            alle.push_back(std::move(ds));
        } else if (!z.dreiecke.empty()) {
            MetaDs ds = wie(z, z.modell != nullptr && z.modell->erzwungen ? MetaDs::Typ::Erzwungen : MetaDs::Typ::Dreiecke, z.nummer);
            ds.ecken.resize(z.dreiecke.size());
            for (std::size_t i = 0; i < ds.ecken.size(); ++i) {
                ds.ecken[i].xyz = z.dreiecke[i];
                // ohne Normale im Modell (0 durch die Drehung geteilt): keine, wie metaTriangle_insert sie repariert
                if (z.modell != nullptr && i < z.modell->normalen.size() && std::isfinite(z.modell->normalen[i][0])) { ds.ecken[i].normal = z.modell->normalen[i]; }
                if (z.modell != nullptr && i < z.modell->st.size()) { ds.ecken[i].st = z.modell->st[i]; }
                ds.indizes.push_back(static_cast<int>(i));
            }
            alle.push_back(std::move(ds));
        } else if (z.seite != nullptr) {
            const ShaderDaten& si = *z.si;
            if (si.tessSize >= 1.0F && (si.flags.compile & C_SKY) == 0U) {
                Wk w;   // WindingFromDrawSurf: die eingerasteten Ecken
                for (const V3f& p : z.w) { w.push_back(eingerastet(p)); }
                std::vector<Wk> stuecke;
                seiteUnterteilen_r(std::move(w), si.tessSize, stuecke, abbrechen);
                for (const Wk& s : stuecke) {
                    MetaDs ds = wie(z, MetaDs::Typ::Seite, naechste++);
                    ds.planeNum = z.planeNum;
                    ds.ecken = eckenDerSeite(s, *z.seite, z.planeNum, ebenen, ein.format, si);
                    geteilt.push_back(std::move(ds));
                }
                continue;
            }
            MetaDs ds = wie(z, MetaDs::Typ::Seite, z.nummer);
            ds.planeNum = z.planeNum;
            ds.ecken = eckenDerSeite(z.w, *z.seite, z.planeNum, ebenen, ein.format, si);
            alle.push_back(std::move(ds));
        }
    }
    for (MetaDs& ds : geteilt) { alle.push_back(std::move(ds)); }
    tJunktionen(alle, abbrechen);
    if (abbrechen != nullptr && abbrechen->load()) { return {}; }
    // ClassifyEntitySurfaces: FinishSurface (q3map_cloneShader, q3map_backShader - CloneSurface haengt hinten an, die Kopie
    // wird selbst wieder abgearbeitet; Rueckseiten hoeren dort auf), dann ClassifySurface
    for (std::size_t i = 0; i < alle.size(); ++i) {
        if (alle.size() >= kMaxZeichenflaechen) { return {}; }
        if (alle[i].typ != MetaDs::Typ::Schlecht && !alle[i].ecken.empty() && !alle[i].rueckseite) {
            if (alle[i].si->klon != nullptr) {
                MetaDs k = alle[i];
                k.si = alle[i].si->klon;
                k.seitenRef = false;
                alle.push_back(std::move(k));
            }
            if (alle[i].si->rueck != nullptr) {
                MetaDs r = alle[i];
                r.si = alle[i].si->rueck;
                r.rueckseite = true;
                r.seitenRef = false;
                alle.push_back(std::move(r));
            }
        }
        einordnen(alle[i], ebenen);
    }
    decalsWerfen(werfer, alle, ebenen);   // MakeEntityDecals
    // MakeEntityMetaTriangles (die Liste waechst mit -patchmeta), SmoothMetaTriangles, MergeMetaTriangles
    MetaStufe stufe(ebenen, ein.np);
    auto zufall = std::make_shared<CZufall>();
    const bool patchMeta = ein.patchMeta || ein.weltPatchMeta;
    for (std::size_t i = 0; i < alle.size(); ++i) {
        if (alle[i].ecken.empty() || alle[i].si->autosprite || (!ein.meta && !alle[i].si->forceMeta)) { continue; }
        switch (alle[i].typ) {
        case MetaDs::Typ::Seite:
        case MetaDs::Typ::Decal:
            if (ein.maxArea) {
                maxFlaeche(alle[i], ebenen, *zufall);
            } else {
                streifen(alle[i], ebenen);
            }
            stufe.ausFlaeche(alle[i]);
            alle[i].typ = MetaDs::Typ::Schlecht;
            break;
        case MetaDs::Typ::Patch:
            if (patchMeta) {
                MetaDs neu = patchDreiecke(alle[i], ein, ebenen);
                // "if the patch is nonsolid, then discard it"
                if ((alle[i].si->flags.compile & C_SOLID) == 0U && (alle[i].si->flags.inhalt & S_CONT_PLAYERCLIP) == 0U) { alle[i].typ = MetaDs::Typ::Schlecht; }
                alle.push_back(std::move(neu));
            }
            break;
        case MetaDs::Typ::Erzwungen:
        case MetaDs::Typ::Meta:
            stufe.ausFlaeche(alle[i]);
            alle[i].typ = MetaDs::Typ::Schlecht;
            break;
        default:
            break;
        }
    }
    stufe.glaetten();
    std::vector<MetaDs> neu;
    int nummer = naechste;
    stufe.zusammensetzen(neu, nummer, abbrechen);
    if (abbrechen != nullptr && abbrechen->load()) { return {}; }
    // die Flaechen in der Reihenfolge, in der FilterDrawsurfsIntoTree sie sieht (TidyEntitySurfaces)
    std::vector<Zeichenflaeche> aus;
    const auto flaechenSpur = [&](Zeichenflaeche& z, MetaDs ds) {   // EmitFaceSurface: erst nach FilterFaceIntoTree (-maxarea zieht Zufall)
        if (ein.maxArea) {
            z.spaeter = [ds, zufall, &ebenen, ausgenommen]() mutable {
                maxFlaeche(ds, ebenen, *zufall);
                return spurAus(ds, ausgenommen);
            };
        } else {
            streifen(ds, ebenen);
            z.spur = spurAus(ds, ausgenommen);
        }
    };
    for (MetaDs& ds : alle) {
        if (ds.typ == MetaDs::Typ::Decal && !ds.ecken.empty()) {   // ohne Meta-Stufe: FilterFaceIntoTree mit planeNum -1
            Zeichenflaeche z;
            z.si = ds.si;
            z.planeNum = ds.planeNum;
            z.nummer = ds.nummer;
            z.nebel = ds.nebel;
            for (const MetaEcke& v : ds.ecken) { z.w.push_back(v.xyz); }
            flaechenSpur(z, ds);
            aus.push_back(std::move(z));
            continue;
        }
        if (ds.typ == MetaDs::Typ::Schlecht || ds.quelle == nullptr || ds.ecken.empty()) { continue; }
        Zeichenflaeche z = *ds.quelle;
        z.si = ds.si;   // FinishSurface: die Kopien mit ihrem Shader
        z.nummer = ds.nummer;
        z.nebel = ds.nebel;
        const bool aussen = ausgenommen(ds.si->flags);
        if (ds.typ == MetaDs::Typ::Dreiecke) {
            z.planeNum = ds.planeNum;
            if (ds.si != ds.quelle->si || ds.rueckseite) {   // EmitTriangleSurface: backSide dreht die Dreiecke
                std::vector<int> indizes = ds.indizes;
                if (ds.rueckseite || ds.si->invert) {
                    for (std::size_t i = 0; i + 2 < indizes.size(); i += 3) { std::swap(indizes[i], indizes[i + 1]); }
                }
                z.spur = spurDreiecke(z.dreiecke, indizes, aussen);
            }
        }
        if (ds.typ == MetaDs::Typ::Patch) {
            // EmitPatchSurface: mit -patchmeta "nodraw patches for collision", nodraw-Shader ebenso (SF_SKIP im Spiel);
            // eine Rueckseite dreht jede Zeile um
            if (patchMeta || (ds.si->flags.flaeche & S_SURF_NODRAW) != 0U) {
                z.spur = SpurFlaeche{};
                z.spur.ausgenommen = aussen;
            } else {
                std::vector<std::array<float, 3>> punkte = z.patch->punkte;
                if (ds.rueckseite || ds.si->invert) {
                    for (int j = 0; j < z.patch->hoehe; ++j) {
                        const auto zeile = punkte.begin() + static_cast<std::ptrdiff_t>(j) * z.patch->breite;
                        std::reverse(zeile, zeile + z.patch->breite);
                    }
                }
                z.spur = spurPatch(z.patch->breite, z.patch->hoehe, punkte, aussen);
            }
        }
        if (ds.typ == MetaDs::Typ::Seite) {   // EmitFaceSurface; FilterFaceIntoTree nimmt die Ecken davor
            z.planeNum = ds.planeNum;
            z.w.clear();
            for (const MetaEcke& v : ds.ecken) { z.w.push_back(v.xyz); }
            flaechenSpur(z, ds);
        }
        aus.push_back(std::move(z));
    }
    for (MetaDs& ds : neu) {
        if (ds.ecken.empty() || ds.indizes.empty()) { continue; }
        Zeichenflaeche z;
        z.si = ds.si;
        z.planeNum = ds.planeNum;
        z.meta = true;
        for (const int i : ds.indizes) { z.dreiecke.push_back(ds.ecken[static_cast<std::size_t>(i)].xyz); }
        z.spur = spurAus(ds, ausgenommen);
        aus.push_back(std::move(z));
    }
    return aus;
}
