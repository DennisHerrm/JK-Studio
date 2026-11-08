// modell/g2.cpp - eine Ghoul2-Instanz wie im Spiel (siehe bhed/modell/g2.h)
//
//   Spawn rules ported from OpenJK (code/game/NPC_spawn.cpp, g_client.cpp, codemp/ghoul2/G2_surfaces.cpp; Copyright (C)
//   1999-2005 Id Software, Inc., 2000-2013 Raven Software, Inc., 2013-2015 OpenJK contributors) and Movie Duels
//   (g_client.cpp), licensed under the GNU General Public License version 2. This file is part of BehavEd-Remake and
//   licensed under the GNU General Public License version 2 or later (LICENSE).
#include "bhed/modell/g2.h"

#include "bhed/gpuskin.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <set>

namespace bhed::modell {
namespace {

// G2_IsSurfaceLegal (G2_surfaces.cpp:137-156): Name ohne Gross/Klein (Q_stricmp) gegen den Namen der
// Hierarchie, wie R_LoadMDXM ihn hinterlaesst (spielName: ohne "_off").
int flaecheNachName(const Inhalt& in, const std::string& name) {
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const std::string n = spielName(in, static_cast<int>(i));
        if (n.size() != name.size()) { continue; }
        bool gleich = true;
        for (std::size_t k = 0; k < n.size() && gleich; ++k) {
            gleich = std::tolower(static_cast<unsigned char>(n[k])) == std::tolower(static_cast<unsigned char>(name[k]));
        }
        if (gleich) { return static_cast<int>(i); }
    }
    return -1;
}

// G2_FindOverrideSurface (G2_surfaces.cpp:86-121): QuickOverride.Set laeuft
// vorwaerts, der letzte Eintrag einer Flaeche gilt.
const FlaechenSchalter* ueberschreibung(const G2Zustand& z, int flaeche) {
    for (auto it = z.slist.rbegin(); it != z.slist.rend(); ++it) {
        if (it->flaeche == flaeche) { return &*it; }
    }
    return nullptr;
}

void zeichneRekursiv(const Inhalt& in, const G2Zustand& z, int flaeche, std::vector<char>& aus, int tiefe) {
    if (flaeche < 0 || static_cast<std::size_t>(flaeche) >= in.teile.size() || tiefe > static_cast<int>(in.teile.size())) { return; }
    const std::uint32_t off = offFlags(in, z, flaeche);
    if (off == 0U) { aus[static_cast<std::size_t>(flaeche)] = 1; }
    if ((off & kG2OhneKinder) != 0U) { return; }
    for (const int k : in.teile[static_cast<std::size_t>(flaeche)].kinder) { zeichneRekursiv(in, z, k, aus, tiefe + 1); }
}

}  // namespace

std::string spielName(const Inhalt& in, int teil) {
    if (teil < 0 || static_cast<std::size_t>(teil) >= in.teile.size()) { return std::string(); }
    std::string n = in.teile[static_cast<std::size_t>(teil)].name;
    // R_LoadMDXM (tr_ghoul2.cpp:3722-3735 OpenJK, MD9 :5397-5402): Q_strlwr, dann "_off" am Ende weg; unter JK2_MODE
    // nur fuer ein JKA-Modell (isANewModelFile) - dieselbe Regel wie applySkin (glm.cpp).
    if (ghoul2Jk2() && !in.jkaUmgewandelt) { return n; }
    for (char& c : n) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    if (n.size() >= 4 && n.compare(n.size() - 4, 4, "_off") == 0) { n.resize(n.size() - 4); }
    return n;
}

G2Zustand spawnZustand(const Inhalt& in, const std::vector<std::string>& hautAus, const SpawnRegeln& r) {
    G2Zustand z;
    // G2API_SetSkin -> G2API_SetSurfaceOnOffFromSkin ("this is going to set the surfs on/off matching the skin file")
    hautAnwenden(in, z, hautAus);
    // G_SetG2PlayerModelInfo (g_client.cpp OpenJK 1128-1160, MD9 1629-1669): surfOff mit 0x2, dann surfOn mit 0
    for (const std::string& n : r.surfOff) { (void)setzeFlaecheAnAus(in, z, n, kG2Aus); }
    for (const std::string& n : r.surfOn) { (void)setzeFlaecheAnAus(in, z, n, 0U); }
    // "carrying a key, turn on the key sleeve surface (assuming we have one)"
    if (r.imperialMitSchluessel) { (void)setzeFlaecheAnAus(in, z, "l_arm_key", 0U); }
    // Movie Duels Update 9 g_set_g2_player_model (g_client.cpp:2381): danach immer
    // G2API_SetSurfaceOnOff(..., "force_shield", TURN_OFF) mit TURN_OFF 0x100 (g_client.cpp:2282). OpenJK hat die Zeile nicht.
    if (r.movieDuelsSp) { (void)setzeFlaecheAnAus(in, z, "force_shield", kG2OhneKinder); }
    return z;
}

std::vector<std::string> npcFlaechenListe(const std::string& text) {
    // NPC_GetSurfaceParms liest surfOff/surfOn als Text, G_SetG2PlayerModelInfo zerlegt ihn mit COM_ParseExt
    // (Woerter zwischen Leerraum).
    std::vector<std::string> l;
    std::string w;
    for (const char c : text) {
        if (static_cast<unsigned char>(c) <= 32U) {   // SkipWhitespace: alles <= ' '
            if (!w.empty()) { l.push_back(w); }
            w.clear();
        } else {
            w += c;
        }
    }
    if (!w.empty()) { l.push_back(w); }
    return l;
}

Inhalt hierarchieAus(const GlmModel& m) {
    Inhalt in;
    in.herkunft = Herkunft::Glm;
    in.name = m.name;
    in.jk2Umgewandelt = m.jk2Umgewandelt;
    in.jkaUmgewandelt = m.jkaUmgewandelt;
    in.stufenZahl = 1;
    in.teile.resize(m.surfaces.size());
    for (std::size_t i = 0; i < m.surfaces.size(); ++i) {
        in.teile[i].name = m.surfaces[i].name;
        in.teile[i].flaggen = m.surfaces[i].flags;
        in.teile[i].eltern = m.surfaces[i].parent;
    }
    for (std::size_t i = 0; i < in.teile.size(); ++i) {   // Kinder aus den Eltern, in der Reihenfolge der Datei
        const int p = in.teile[i].eltern;
        if (p >= 0 && static_cast<std::size_t>(p) < in.teile.size() && static_cast<std::size_t>(p) != i) {
            in.teile[static_cast<std::size_t>(p)].kinder.push_back(static_cast<int>(i));
        }
    }
    return in;
}

std::vector<std::string> hautAusModell(const Inhalt& in, const GlmModel& m) {
    std::vector<std::string> aus;
    for (std::size_t i = 0; i < m.surfaces.size() && i < in.teile.size(); ++i) {
        if (m.surfaces[i].skinnedOff) { aus.push_back(spielName(in, static_cast<int>(i))); }
    }
    return aus;
}

G2Zustand mitKappen(const Inhalt& in, const G2Zustand& z) {
    // Die Leiste "Kappen": G2_SetSurfaceOnOff(..., 0) fuer jede Flaeche mit OFF in der Datei, die keine
    // Ueberschreibung hat (Haut, Spielcode oder Anwender).
    G2Zustand k = z;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if ((in.teile[i].flaggen & kG2Aus) == 0U) { continue; }
        const bool eigen = std::any_of(z.slist.begin(), z.slist.end(), [&](const FlaechenSchalter& s) { return s.flaeche == static_cast<int>(i); });
        if (!eigen) { (void)setzeFlaecheAnAus(in, k, spielName(in, static_cast<int>(i)), 0U); }
    }
    return k;
}

void g2Uebertragen(const Inhalt& in, const G2Zustand& z, GlmModel& m) {
    const std::vector<char> an = gezeichnet(in, z);
    const std::vector<char> kappen = gezeichnet(in, mitKappen(in, z));
    for (std::size_t i = 0; i < m.surfaces.size() && i < an.size(); ++i) {
        m.surfaces[i].g2An = an[i] != 0 ? 1 : 0;
        m.surfaces[i].g2Kappen = kappen[i] != 0 ? 1 : 0;
    }
}

bool setzeFlaecheAnAus(const Inhalt& in, G2Zustand& z, const std::string& name, std::uint32_t off) {
    const int f = flaecheNachName(in, name);
    if (f < 0) { return false; }
    constexpr std::uint32_t kAussen = kG2Aus | kG2OhneKinder;
    // Schon in der Liste (G2_FindSurface sucht von hinten): nur OFF und NODESCENDANTS tauschen.
    for (auto it = z.slist.rbegin(); it != z.slist.rend(); ++it) {
        if (it->flaeche == f) {
            it->offFlags &= ~kAussen;
            it->offFlags |= off & kAussen;
            return true;
        }
    }
    const std::uint32_t flags = in.teile[static_cast<std::size_t>(f)].flaggen;
    std::uint32_t neu = flags;
    neu &= ~kAussen;
    neu |= off & kAussen;
    // "insert here then because it changed, no need to add an override otherwise"
    if (neu != flags) { z.slist.push_back(FlaechenSchalter{f, neu}); }
    return true;
}

bool setzeWurzel(const Inhalt& in, G2Zustand& z, const std::string& name) {
    const int f = flaecheNachName(in, name);
    if (f < 0) { return false; }
    z.wurzel = f;
    return true;
}

void hautAnwenden(const Inhalt& in, G2Zustand& z, const std::vector<std::string>& hautAus) {
    z.slist.clear();   // "remove any overrides we had before."
    for (const std::string& n : hautAus) {   // Namen der Haut wie das Spiel (spielName der Flaeche)
        const int f = flaecheNachName(in, n);
        // Eine Flaeche, die es nicht gibt, hat keine Flaggen; G2_SetSurfaceOnOff findet sie nicht.
        if (f < 0) { continue; }
        if ((in.teile[static_cast<std::size_t>(f)].flaggen & kG2Aus) == 0U) { (void)setzeFlaecheAnAus(in, z, n, kG2Aus); }
    }
}

std::uint32_t offFlags(const Inhalt& in, const G2Zustand& z, int flaeche) {
    if (flaeche < 0 || static_cast<std::size_t>(flaeche) >= in.teile.size()) { return kG2Aus; }
    const FlaechenSchalter* u = ueberschreibung(z, flaeche);
    return u != nullptr ? u->offFlags : in.teile[static_cast<std::size_t>(flaeche)].flaggen;
}

std::vector<char> gezeichnet(const Inhalt& in, const G2Zustand& z) {
    std::vector<char> aus(in.teile.size(), 0);
    if (in.herkunft == Herkunft::Md3) {
        std::fill(aus.begin(), aus.end(), static_cast<char>(1));
        return aus;
    }
    zeichneRekursiv(in, z, z.wurzel, aus, 0);
    return aus;
}

float projiziereRadius(float r, float abstand, float projektion5) noexcept {
    // ProjectRadius mit der Projektion von R_SetupProjection: width = |r| *
    // projectionMatrix[5], depth = dist (projectionMatrix[11] = -1, [15] = 0).
    if (abstand <= 0.0F) { return 0.0F; }
    const float width = std::fabs(r) * projektion5;
    const float pr = width / abstand;
    return pr > 1.0F ? 1.0F : pr;
}

int lodWieImSpiel(const LodLage& l) noexcept {
    if (l.stufenZahl < 2) { return 0; }   // "model has only 1 LOD level"
    int lodBias = l.instanzBias;
    if (l.lodbias > lodBias) { lodBias = l.lodbias; }
    if (lodBias >= l.stufenZahl) { return l.stufenZahl - 1; }
    const float groesste = l.skala != 0.0F ? l.skala : 1.0F;
    float flod = 0.0F;
    const float pr = projiziereRadius(0.75F * groesste * l.radius, l.abstand, l.projektion5);
    if (pr != 0.0F) {
        float lodscale = l.lodscale;
        if (lodscale > 20.0F) { lodscale = 20.0F; }
        flod = 1.0F - pr * lodscale;
    }
    flod *= static_cast<float>(l.stufenZahl);
    int lod = static_cast<int>(flod);   // Q_ftol schneidet ab
    if (lod < 0) {
        lod = 0;
    } else if (lod >= l.stufenZahl) {
        lod = l.stufenZahl - 1;
    }
    lod += lodBias;
    if (lod >= l.stufenZahl) { lod = l.stufenZahl - 1; }
    if (lod < 0) { lod = 0; }
    return lod;
}

const GlaAnimation& standardGla() {
    // FakeGLAFile, code/rd-vanilla/tr_model.cpp:112-131 OpenJK (Movie Duels Update 9 gleich)
    static const unsigned char kDatei[] = {
        0x32, 0x4C, 0x47, 0x41, 0x06, 0x00, 0x00, 0x00, 0x2A, 0x64, 0x65, 0x66, 0x61, 0x75, 0x6C, 0x74,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x01, 0x00, 0x00, 0x00,
        0x14, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x01, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00,
        0x26, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x4D, 0x6F, 0x64, 0x56, 0x69, 0x65, 0x77, 0x20,
        0x69, 0x6E, 0x74, 0x65, 0x72, 0x6E, 0x61, 0x6C, 0x20, 0x64, 0x65, 0x66, 0x61, 0x75, 0x6C, 0x74,
        0x00, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD,
        0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD,
        0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFD, 0xBF, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE, 0x7F,
        0x00, 0x80, 0x00, 0x80, 0x00, 0x80
    };
    static const GlaAnimation gla = [] {
        GlaAnimation a;
        (void)readGla(std::string(reinterpret_cast<const char*>(kDatei), sizeof(kDatei)), a);
        return a;
    }();
    return gla;
}

bool glmStandbild(const GlmModel& stufe, const GlaAnimation& gla, Md3Model& aus) {
    aus = Md3Model{};
    if (gla.empty()) { return false; }   // "missing animation file"
    // R_LoadMDXM ausserhalb von JK2_MODE: andere Knochenzahl als die GLA nur bei einem umgewandelten JK2-Modell.
    if (!ghoul2Jk2() && !stufe.jk2Umgewandelt && stufe.numBones != static_cast<int>(gla.bones.size())) { return false; }
    std::vector<BoneMatrix> welt;
    gla.worldMatrices(0, welt);
    std::vector<BoneMatrix> knochen;
    gpu::baueKnochenmatrizen(welt, gla.bones, knochen);
    const std::vector<char> an = gezeichnet(hierarchieAus(stufe), G2Zustand{});
    aus.name = stufe.name;
    aus.numFrames = 1;
    const BoneMatrix eins = BoneMatrix::identity();
    for (std::size_t si = 0; si < stufe.surfaces.size(); ++si) {
        const GlmSurface& sf = stufe.surfaces[si];
        if (si >= an.size() || an[si] == 0) { continue; }
        Md3Surface m;
        m.name = sf.name;
        m.shader = sf.shader;
        m.numFrames = 1;
        m.dateiFlaeche = static_cast<int>(si);
        m.numVerts = static_cast<int>(sf.verts.size());
        m.verts.reserve(sf.verts.size());
        for (const GlmVertex& v : sf.verts) {
            float p[3] = {0.0F, 0.0F, 0.0F};
            for (int b = 0; b < 4; ++b) {
                if (v.weights[b] == 0.0F) { continue; }
                const BoneMatrix& k = v.bones[b] < knochen.size() ? knochen[v.bones[b]] : eins;
                for (int r = 0; r < 3; ++r) {
                    p[r] += v.weights[b] * (k.m[r][0] * v.xyz[0] + k.m[r][1] * v.xyz[1] + k.m[r][2] * v.xyz[2] + k.m[r][3]);
                }
            }
            const BoneMatrix& k0 = v.bones[0] < knochen.size() ? knochen[v.bones[0]] : eins;
            float n[3];
            for (int r = 0; r < 3; ++r) { n[r] = k0.m[r][0] * v.normal[0] + k0.m[r][1] * v.normal[1] + k0.m[r][2] * v.normal[2]; }
            Md3Vertex mv;
            // rootMatrix (identityMatrix): Zeile 0 = (0 -1 0), Zeile 1 = (1 0 0), Zeile 2 = (0 0 1)
            mv.xyz[0] = -p[1];
            mv.xyz[1] = p[0];
            mv.xyz[2] = p[2];
            mv.normal[0] = -n[1];
            mv.normal[1] = n[0];
            mv.normal[2] = n[2];
            mv.st[0] = v.st[0];
            mv.st[1] = v.st[1];
            m.verts.push_back(mv);
        }
        m.indexes = sf.indexes;
        aus.surfaces.push_back(std::move(m));
    }
    return true;
}

const Grenzen& grenzen(Zielspiel s) noexcept {
    // SHADER_MAX_VERTEXES: OpenJK code/qcommon/qfiles.h:33 und codemp/qcommon/qfiles.h:34 1000, Movie Duels
    // Update 9 code/ und codemp/qcommon/qfiles.h:33/34 4000 ("raised from 1000"); SHADER_MAX_INDEXES = 6 *
    // SHADER_MAX_VERTEXES. RB_CheckOverflow (tr_surface.cpp:53-66): ERR_DROP bei verts >= SHADER_MAX_VERTEXES
    // oder indexes >= SHADER_MAX_INDEXES; RB_SurfaceGhoul gibt numTriangles als Indexzahl (tr_ghoul2.cpp:2906 SP,
    // codemp 3604, MD 4950/5877). iMAX_G2_BONEREFS_PER_SURFACE 32 und iMAX_G2_BONEWEIGHTS_PER_VERT 4
    // (rd-common/mdx_format.h:84-85, MD :81-82). MAX_G2_MODELS: code/rd-vanilla/G2_API.cpp:337 512,
    // codemp/rd-vanilla/G2_API.cpp:297 1024, MD code :333 und codemp :298 1024. r_lodscale: code tr_init.cpp:1559
    // "10", codemp :1650 "5", MD :1747 "10" und codemp :1748 "5". Figurradius: SP CG_Player legt den refEntity
    // mit memset an (cg_players.cpp:6882), MP CG_RadiusForCent 64 (cg_players.c:8338-8358). T-Pose hinter dem
    // Ende der GLA: in Movie Duels (OpenJK) gesehen, im Original-JKA nicht (Stufe 0, nachweise_im_spiel.md).
    // JK2: OpenJK baut Jedi Outcast aus code/ (JK2_MODE) mit denselben Werten.
    static const Grenzen kWerte[kZielspiele] = {
        {"JKA SP", 1000, 6000, 32, 4, 512, 10.0F, 0.0F, false},
        {"JKA MP", 1000, 6000, 32, 4, 1024, 5.0F, 64.0F, false},
        {"JK2 SP", 1000, 6000, 32, 4, 512, 10.0F, 0.0F, false},
        {"Movie Duels SP", 4000, 24000, 32, 4, 1024, 10.0F, 0.0F, true},
        {"Movie Duels MP", 4000, 24000, 32, 4, 1024, 5.0F, 64.0F, true},
    };
    const auto i = static_cast<std::size_t>(s);
    return kWerte[i < static_cast<std::size_t>(kZielspiele) ? i : 0U];
}

FlaechenMass mass(const Netz& n) {
    FlaechenMass m;
    m.ecken = static_cast<int>(n.ecken.size());
    m.dreiecke = n.dreiecke();
    std::set<int> knochen;
    for (const Ecke& e : n.ecken) {
        int g = 0;
        for (int k = 0; k < 4; ++k) {
            if (e.gewicht[k] > 0.0F) {
                ++g;
                knochen.insert(e.knochen[k]);
            }
        }
        m.gewichteMax = std::max(m.gewichteMax, g);
    }
    m.knochen = static_cast<int>(knochen.size());
    return m;
}

std::vector<Befund> pruefeMass(const FlaechenMass& m, const Grenzen& g) {
    std::vector<Befund> b;
    const auto melde = [&](BefundArt art, int wert, int grenze) {
        Befund x;
        x.art = art;
        x.wert = wert;
        x.grenze = grenze;
        b.push_back(x);
    };
    // RB_CheckOverflow: ">=" der Grenze bricht ab.
    if (m.ecken >= g.eckenJeFlaeche) { melde(BefundArt::Ecken, m.ecken, g.eckenJeFlaeche); }
    if (m.dreiecke >= g.dreieckeJeFlaeche) { melde(BefundArt::Dreiecke, m.dreiecke, g.dreieckeJeFlaeche); }
    // Knochenverweise: 5 Bit je Verweis (iG2_BITS_PER_BONEREF), also hoechstens 32.
    if (m.knochen > g.knochenJeFlaeche) { melde(BefundArt::Knochen, m.knochen, g.knochenJeFlaeche); }
    if (m.gewichteMax > g.gewichteJeEcke) { melde(BefundArt::Gewichte, m.gewichteMax, g.gewichteJeEcke); }
    return b;
}

bool animInnerhalb(const AnimEntry& e, int glaBilder) noexcept {
    // pfix N9: ohne Ueberlauf (firstFrame + numFrames aus einer fremden animation.cfg bis INT_MAX)
    return e.firstFrame >= 0 && e.numFrames >= 0 && e.firstFrame < glaBilder && e.numFrames <= glaBilder - e.firstFrame;
}

std::vector<Befund> pruefeModell(const Inhalt& in, Zielspiel s, const std::vector<AnimEntry>& anims, int glaBilder,
                                 int g2Instanzen, const std::vector<char>& hatBild, const std::vector<char>& gez) {
    const Grenzen& g = grenzen(s);
    std::vector<Befund> alle;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const Teil& t = in.teile[i];
        if (t.istTag()) { continue; }
        for (std::size_t st = 0; st < t.stufen.size(); ++st) {
            for (Befund b : pruefeMass(mass(t.stufen[st]), g)) {
                b.flaeche = static_cast<int>(i);
                b.stufe = static_cast<int>(st);
                b.name = t.name;
                alle.push_back(std::move(b));
            }
        }
        if (i < hatBild.size() && i < gez.size() && gez[i] != 0 && hatBild[i] == 0) {
            Befund b;
            b.art = BefundArt::ShaderFehlt;
            b.flaeche = static_cast<int>(i);
            b.name = t.name;
            alle.push_back(std::move(b));
        }
    }
    if (g2Instanzen > g.g2Modelle) {
        Befund b;
        b.art = BefundArt::G2Modelle;
        b.wert = g2Instanzen;
        b.grenze = g.g2Modelle;
        alle.push_back(std::move(b));
    }
    if (glaBilder > 0) {
        for (const AnimEntry& e : anims) {
            if (animInnerhalb(e, glaBilder)) { continue; }
            Befund b;
            b.art = BefundArt::AnimHinterGla;
            b.wert = e.firstFrame + e.numFrames;
            b.grenze = glaBilder;
            b.name = e.name;
            alle.push_back(std::move(b));
        }
    }
    return alle;
}

}  // namespace bhed::modell
