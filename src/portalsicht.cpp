// portalsicht.cpp - rc609 P: Spiegel und Portalkameras wie die Engine, siehe bhed/portalsicht.h.
//
// Quellen (OpenJK, Movie Duels 9 gleich): shared/qcommon/q_math.c (bytedirs, DirToByte, ByteToDir,
// PerpendicularVector, RotatePointAroundVector, PlaneFromPoints), code/game/g_misc.cpp (SP_misc_portal_surface,
// locateCamera, setCamera, SP_misc_portal_camera), code/game/g_utils.cpp (G_SetMovedir), code/cgame/cg_ents.cpp
// (CG_Portal), code/rd-vanilla/tr_main.cpp (R_SortDrawSurfs, SurfIsOffscreen, IsMirror, R_GetPortalOrientations,
// R_MirrorViewBySurface, R_MirrorPoint, R_MirrorVector), code/rd-vanilla/tr_backend.cpp (RB_BeginDrawingView).
#include "bhed/portalsicht.h"

#include "bhed/bsp.h"
#include "bhed/geschuetz.h"   // winkelVektoren (AngleVectors)
#include "bhed/scene.h"       // engineNamenGleich (G_Find)

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace bhed {

namespace {

// q_math.c bytedirs[NUMVERTEXNORMALS]
constexpr int kRichtungen = 162;
constexpr float kByteDirs[kRichtungen][3] = {
    {-0.525731F, 0.000000F, 0.850651F}, {-0.442863F, 0.238856F, 0.864188F}, {-0.295242F, 0.000000F, 0.955423F},
    {-0.309017F, 0.500000F, 0.809017F}, {-0.162460F, 0.262866F, 0.951056F}, {0.000000F, 0.000000F, 1.000000F},
    {0.000000F, 0.850651F, 0.525731F}, {-0.147621F, 0.716567F, 0.681718F}, {0.147621F, 0.716567F, 0.681718F},
    {0.000000F, 0.525731F, 0.850651F}, {0.309017F, 0.500000F, 0.809017F}, {0.525731F, 0.000000F, 0.850651F},
    {0.295242F, 0.000000F, 0.955423F}, {0.442863F, 0.238856F, 0.864188F}, {0.162460F, 0.262866F, 0.951056F},
    {-0.681718F, 0.147621F, 0.716567F}, {-0.809017F, 0.309017F, 0.500000F}, {-0.587785F, 0.425325F, 0.688191F},
    {-0.850651F, 0.525731F, 0.000000F}, {-0.864188F, 0.442863F, 0.238856F}, {-0.716567F, 0.681718F, 0.147621F},
    {-0.688191F, 0.587785F, 0.425325F}, {-0.500000F, 0.809017F, 0.309017F}, {-0.238856F, 0.864188F, 0.442863F},
    {-0.425325F, 0.688191F, 0.587785F}, {-0.716567F, 0.681718F, -0.147621F}, {-0.500000F, 0.809017F, -0.309017F},
    {-0.525731F, 0.850651F, 0.000000F}, {0.000000F, 0.850651F, -0.525731F}, {-0.238856F, 0.864188F, -0.442863F},
    {0.000000F, 0.955423F, -0.295242F}, {-0.262866F, 0.951056F, -0.162460F}, {0.000000F, 1.000000F, 0.000000F},
    {0.000000F, 0.955423F, 0.295242F}, {-0.262866F, 0.951056F, 0.162460F}, {0.238856F, 0.864188F, 0.442863F},
    {0.262866F, 0.951056F, 0.162460F}, {0.500000F, 0.809017F, 0.309017F}, {0.238856F, 0.864188F, -0.442863F},
    {0.262866F, 0.951056F, -0.162460F}, {0.500000F, 0.809017F, -0.309017F}, {0.850651F, 0.525731F, 0.000000F},
    {0.716567F, 0.681718F, 0.147621F}, {0.716567F, 0.681718F, -0.147621F}, {0.525731F, 0.850651F, 0.000000F},
    {0.425325F, 0.688191F, 0.587785F}, {0.864188F, 0.442863F, 0.238856F}, {0.688191F, 0.587785F, 0.425325F},
    {0.809017F, 0.309017F, 0.500000F}, {0.681718F, 0.147621F, 0.716567F}, {0.587785F, 0.425325F, 0.688191F},
    {0.955423F, 0.295242F, 0.000000F}, {1.000000F, 0.000000F, 0.000000F}, {0.951056F, 0.162460F, 0.262866F},
    {0.850651F, -0.525731F, 0.000000F}, {0.955423F, -0.295242F, 0.000000F}, {0.864188F, -0.442863F, 0.238856F},
    {0.951056F, -0.162460F, 0.262866F}, {0.809017F, -0.309017F, 0.500000F}, {0.681718F, -0.147621F, 0.716567F},
    {0.850651F, 0.000000F, 0.525731F}, {0.864188F, 0.442863F, -0.238856F}, {0.809017F, 0.309017F, -0.500000F},
    {0.951056F, 0.162460F, -0.262866F}, {0.525731F, 0.000000F, -0.850651F}, {0.681718F, 0.147621F, -0.716567F},
    {0.681718F, -0.147621F, -0.716567F}, {0.850651F, 0.000000F, -0.525731F}, {0.809017F, -0.309017F, -0.500000F},
    {0.864188F, -0.442863F, -0.238856F}, {0.951056F, -0.162460F, -0.262866F}, {0.147621F, 0.716567F, -0.681718F},
    {0.309017F, 0.500000F, -0.809017F}, {0.425325F, 0.688191F, -0.587785F}, {0.442863F, 0.238856F, -0.864188F},
    {0.587785F, 0.425325F, -0.688191F}, {0.688191F, 0.587785F, -0.425325F}, {-0.147621F, 0.716567F, -0.681718F},
    {-0.309017F, 0.500000F, -0.809017F}, {0.000000F, 0.525731F, -0.850651F}, {-0.525731F, 0.000000F, -0.850651F},
    {-0.442863F, 0.238856F, -0.864188F}, {-0.295242F, 0.000000F, -0.955423F}, {-0.162460F, 0.262866F, -0.951056F},
    {0.000000F, 0.000000F, -1.000000F}, {0.295242F, 0.000000F, -0.955423F}, {0.162460F, 0.262866F, -0.951056F},
    {-0.442863F, -0.238856F, -0.864188F}, {-0.309017F, -0.500000F, -0.809017F}, {-0.162460F, -0.262866F, -0.951056F},
    {0.000000F, -0.850651F, -0.525731F}, {-0.147621F, -0.716567F, -0.681718F}, {0.147621F, -0.716567F, -0.681718F},
    {0.000000F, -0.525731F, -0.850651F}, {0.309017F, -0.500000F, -0.809017F}, {0.442863F, -0.238856F, -0.864188F},
    {0.162460F, -0.262866F, -0.951056F}, {0.238856F, -0.864188F, -0.442863F}, {0.500000F, -0.809017F, -0.309017F},
    {0.425325F, -0.688191F, -0.587785F}, {0.716567F, -0.681718F, -0.147621F}, {0.688191F, -0.587785F, -0.425325F},
    {0.587785F, -0.425325F, -0.688191F}, {0.000000F, -0.955423F, -0.295242F}, {0.000000F, -1.000000F, 0.000000F},
    {0.262866F, -0.951056F, -0.162460F}, {0.000000F, -0.850651F, 0.525731F}, {0.000000F, -0.955423F, 0.295242F},
    {0.238856F, -0.864188F, 0.442863F}, {0.262866F, -0.951056F, 0.162460F}, {0.500000F, -0.809017F, 0.309017F},
    {0.716567F, -0.681718F, 0.147621F}, {0.525731F, -0.850651F, 0.000000F}, {-0.238856F, -0.864188F, -0.442863F},
    {-0.500000F, -0.809017F, -0.309017F}, {-0.262866F, -0.951056F, -0.162460F}, {-0.850651F, -0.525731F, 0.000000F},
    {-0.716567F, -0.681718F, -0.147621F}, {-0.716567F, -0.681718F, 0.147621F}, {-0.525731F, -0.850651F, 0.000000F},
    {-0.500000F, -0.809017F, 0.309017F}, {-0.238856F, -0.864188F, 0.442863F}, {-0.262866F, -0.951056F, 0.162460F},
    {-0.864188F, -0.442863F, 0.238856F}, {-0.809017F, -0.309017F, 0.500000F}, {-0.688191F, -0.587785F, 0.425325F},
    {-0.681718F, -0.147621F, 0.716567F}, {-0.442863F, -0.238856F, 0.864188F}, {-0.587785F, -0.425325F, 0.688191F},
    {-0.309017F, -0.500000F, 0.809017F}, {-0.147621F, -0.716567F, 0.681718F}, {-0.425325F, -0.688191F, 0.587785F},
    {-0.162460F, -0.262866F, 0.951056F}, {0.442863F, -0.238856F, 0.864188F}, {0.162460F, -0.262866F, 0.951056F},
    {0.309017F, -0.500000F, 0.809017F}, {0.147621F, -0.716567F, 0.681718F}, {0.000000F, -0.525731F, 0.850651F},
    {0.425325F, -0.688191F, 0.587785F}, {0.587785F, -0.425325F, 0.688191F}, {0.688191F, -0.587785F, 0.425325F},
    {-0.955423F, 0.295242F, 0.000000F}, {-0.951056F, 0.162460F, 0.262866F}, {-1.000000F, 0.000000F, 0.000000F},
    {-0.850651F, 0.000000F, 0.525731F}, {-0.955423F, -0.295242F, 0.000000F}, {-0.951056F, -0.162460F, 0.262866F},
    {-0.864188F, 0.442863F, -0.238856F}, {-0.951056F, 0.162460F, -0.262866F}, {-0.809017F, 0.309017F, -0.500000F},
    {-0.864188F, -0.442863F, -0.238856F}, {-0.951056F, -0.162460F, -0.262866F}, {-0.809017F, -0.309017F, -0.500000F},
    {-0.681718F, 0.147621F, -0.716567F}, {-0.681718F, -0.147621F, -0.716567F}, {-0.850651F, 0.000000F, -0.525731F},
    {-0.688191F, 0.587785F, -0.425325F}, {-0.587785F, 0.425325F, -0.688191F}, {-0.425325F, 0.688191F, -0.587785F},
    {-0.425325F, -0.688191F, -0.587785F}, {-0.587785F, -0.425325F, -0.688191F}, {-0.688191F, -0.587785F, -0.425325F},
};

float punkt(const float a[3], const float b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

void kreuz(const float a[3], const float b[3], float c[3]) {
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
}

// VectorNormalize: Laenge 0 bleibt 0
void normieren(float v[3]) {
    const float l = std::sqrt(punkt(v, v));
    if (l > 0.0F) {
        const float il = 1.0F / l;
        for (int k = 0; k < 3; ++k) { v[k] *= il; }
    }
}

// G_SpawnVector: sscanf "%f %f %f"; ohne Schluessel bleibt der Ursprung
void ortLesen(const std::string& text, float o[3]) {
    o[0] = o[1] = o[2] = 0.0F;
    (void)std::sscanf(text.c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
}

bool gleichOhneGross(const std::string& a, const char* b) {
    std::size_t i = 0;
    for (; i < a.size() && b[i] != '\0'; ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return i == a.size() && b[i] == '\0';
}

// s.angles nach G_ParseField: "angles" (F_VECTOR) und "angle" (F_ANGLEHACK: 0 v 0) schreiben dasselbe Feld, das letzte
// Paar der Entity gewinnt (Q_stricmp).
void winkelDerEntity(const MapEntity& e, float w[3]) {
    w[0] = w[1] = w[2] = 0.0F;
    for (const auto& [k, v] : e.keys) {
        if (gleichOhneGross(k, "angles")) {
            w[0] = w[1] = w[2] = 0.0F;
            (void)std::sscanf(v.c_str(), "%f %f %f", &w[0], &w[1], &w[2]);
        } else if (gleichOhneGross(k, "angle")) {
            w[0] = 0.0F;
            w[1] = static_cast<float>(std::atof(v.c_str()));
            w[2] = 0.0F;
        }
    }
}

// G_Find(NULL, FOFS(targetname), name): die erste angelegte Entity mit diesem Namen
const MapEntity* findeName(const std::vector<MapEntity>& ents, const std::function<bool(const MapEntity&)>& gespawnt, const std::string& name) {
    if (name.empty()) { return nullptr; }
    for (const MapEntity& x : ents) {
        if (engineNamenGleich(x.targetname, name) && (!gespawnt || gespawnt(x))) { return &x; }
    }
    return nullptr;
}

// IsMirror / R_GetPortalOrientations: das erste misc_portal_surface (refEntity-Reihenfolge) in 64 Einheiten der Ebene
int schirmFuer(const float ebene[4], const std::vector<PortalSchirm>& schirme, const std::function<bool(std::size_t)>& imBild) {
    for (std::size_t i = 0; i < schirme.size(); ++i) {
        if (imBild && !imBild(i)) { continue; }
        const float d = punkt(schirme[i].ort, ebene) - ebene[3];
        if (d > 64.0F || d < -64.0F) { continue; }
        return static_cast<int>(i);
    }
    return -1;
}

// SurfIsOffscreen; `kuerzest` bekommt das kleinste Abstandsquadrat (fuer den Gleichstand).
bool ausserhalb(const PortalAuge& a, const PortalFlaeche& f, const std::vector<PortalSchirm>& schirme, const std::function<bool(std::size_t)>& imBild,
                float& kuerzest) {
    unsigned und = ~0U;
    // R_TransformModelToClip mit der Projektion aus R_SetupProjection (symmetrisch, zNear, zFar)
    const float zf = a.fern;
    const float zn = a.nah;
    for (const std::array<float, 3>& p : f.ecken) {
        const float d[3] = {p[0] - a.ort[0], p[1] - a.ort[1], p[2] - a.ort[2]};
        const float ex = punkt(d, a.rechts);
        const float ey = punkt(d, a.oben);
        const float ez = punkt(d, a.vorn);
        const float clip[4] = {ex / a.tanX, ey / a.tanY, ((zf + zn) * ez - 2.0F * zf * zn) / (zf - zn), ez};
        unsigned flags = 0U;
        for (int j = 0; j < 3; ++j) {
            if (clip[j] >= clip[3]) {
                flags |= 1U << (j * 2);
            } else if (clip[j] <= -clip[3]) {
                flags |= 1U << (j * 2 + 1);
            }
        }
        und &= flags;
    }
    if (und != 0U) { return true; }
    // abgewandt und naechste Ecke: nur die erste Ecke jedes Dreiecks (tess.indexes[i], i += 3)
    kuerzest = 1000000000.0F;
    int dreiecke = static_cast<int>(f.indizes.size() / 3U);
    for (std::size_t i = 0; i + 2 < f.indizes.size(); i += 3) {
        const std::size_t e = static_cast<std::size_t>(f.indizes[i]);
        if (e >= f.ecken.size() || e >= f.normalen.size()) { continue; }
        const float d[3] = {f.ecken[e][0] - a.ort[0], f.ecken[e][1] - a.ort[1], f.ecken[e][2] - a.ort[2]};
        kuerzest = std::min(kuerzest, punkt(d, d));
        if (punkt(d, f.normalen[e].data()) >= 0.0F) { --dreiecke; }
    }
    if (dreiecke <= 0) { return true; }
    // Spiegel blenden nicht nach Abstand aus (IsMirror)
    const int s = schirmFuer(f.ebene, schirme, imBild);
    if (s >= 0 && schirme[static_cast<std::size_t>(s)].spiegel()) { return false; }
    return kuerzest > f.portalWeite * f.portalWeite;
}

}  // namespace

int dirZuByte(const float dir[3]) {
    float bestd = 0.0F;
    int best = 0;
    for (int i = 0; i < kRichtungen; ++i) {
        const float d = punkt(dir, kByteDirs[i]);
        if (d > bestd) {
            bestd = d;
            best = i;
        }
    }
    return best;
}

void byteZuDir(int b, float dir[3]) {
    if (b < 0 || b >= kRichtungen) {
        dir[0] = dir[1] = dir[2] = 0.0F;
        return;
    }
    for (int k = 0; k < 3; ++k) { dir[k] = kByteDirs[b][k]; }
}

void senkrechteZu(const float src[3], float dst[3]) {
    int pos = 0;
    float minelem = 1.0F;
    for (int i = 0; i < 3; ++i) {
        if (std::fabs(src[i]) < minelem) {
            pos = i;
            minelem = std::fabs(src[i]);
        }
    }
    float t[3] = {0.0F, 0.0F, 0.0F};
    t[pos] = 1.0F;
    // ProjectPointOnPlane
    const float inv = 1.0F / punkt(src, src);
    const float d = punkt(src, t) * inv;
    for (int i = 0; i < 3; ++i) { dst[i] = t[i] - d * (src[i] * inv); }
    normieren(dst);
}

void dreheUmAchse(float dst[3], const float achse[3], const float p[3], float grad) {
    const float w = -grad * 3.14159265358979323846F / 180.0F;   // -DEG2RAD( degrees )
    const float s = std::sin(w);
    const float c = std::cos(w);
    const float t = 1.0F - c;
    const float* a = achse;
    const float m[3][3] = {{t * a[0] * a[0] + c, t * a[0] * a[1] + s * a[2], t * a[0] * a[2] - s * a[1]},
                           {t * a[0] * a[1] - s * a[2], t * a[1] * a[1] + c, t * a[1] * a[2] + s * a[0]},
                           {t * a[0] * a[2] + s * a[1], t * a[1] * a[2] - s * a[0], t * a[2] * a[2] + c}};
    const float q[3] = {p[0], p[1], p[2]};
    for (int i = 0; i < 3; ++i) { dst[i] = punkt(q, m[i]); }   // VectorRotate
}

bool ebeneAusPunkten(const float a[3], const float b[3], const float c[3], float ebene[4]) {
    const float d1[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const float d2[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    kreuz(d2, d1, ebene);
    if (punkt(ebene, ebene) <= 0.0F) { return false; }
    normieren(ebene);
    ebene[3] = punkt(a, ebene);
    return true;
}

std::vector<PortalSchirm> portalSchirme(const std::vector<MapEntity>& ents, const std::function<bool(const MapEntity&)>& gespawnt) {
    std::vector<PortalSchirm> aus;
    for (std::size_t i = 0; i < ents.size(); ++i) {
        const MapEntity& e = ents[i];
        if (e.classname != "misc_portal_surface" || (gespawnt && !gespawnt(e))) { continue; }
        PortalSchirm s;
        s.entity = i;
        ortLesen(e.origin, s.ort);
        const std::string* ziel = e.find("target");
        if (ziel == nullptr || ziel->empty()) {
            // SP_misc_portal_surface ohne target: ein Spiegel (origin2 = origin)
            for (int k = 0; k < 3; ++k) { s.kamOrt[k] = s.ort[k]; }
            aus.push_back(s);
            continue;
        }
        // locateCamera: G_Find nach dem target, sonst G_FreeEntity
        const MapEntity* kam = findeName(ents, gespawnt, *ziel);
        if (kam == nullptr) { continue; }
        // setCamera
        const std::string* sf = kam->find("spawnflags");
        const int flags = sf != nullptr ? std::atoi(sf->c_str()) : 0;
        s.frame = (flags & 1) != 0 ? 25 : ((flags & 2) != 0 ? 75 : 0);
        int clientNum = 0;
        if (kam->classname == "misc_portal_camera") {
            // SP_misc_portal_camera: G_SpawnFloat("roll", "0"), s.clientNum = roll / 360.0 * 256 (int)
            const std::string* r = kam->find("roll");
            const float roll = r != nullptr ? static_cast<float>(std::atof(r->c_str())) : 0.0F;
            clientNum = static_cast<int>(roll / 360.0 * 256);
        }
        ortLesen(kam->origin, s.kamOrt);
        // G_PickTarget(owner->target) - die Engine waehlt unter mehreren zufaellig, hier die erste
        float dir[3] = {0.0F, 0.0F, 0.0F};
        const std::string* kz = kam->find("target");
        const MapEntity* blick = kz != nullptr ? findeName(ents, gespawnt, *kz) : nullptr;
        if (blick != nullptr) {
            float o[3];
            ortLesen(blick->origin, o);
            for (int k = 0; k < 3; ++k) { dir[k] = o[k] - s.kamOrt[k]; }
            normieren(dir);
        } else {
            // G_SetMovedir (g_utils.cpp)
            float w[3];
            winkelDerEntity(*kam, w);
            if (w[0] == 0.0F && w[1] == -1.0F && w[2] == 0.0F) {
                dir[2] = 1.0F;
            } else if (w[0] == 0.0F && w[1] == -2.0F && w[2] == 0.0F) {
                dir[2] = -1.0F;
            } else {
                winkelVektoren(w, dir, nullptr, nullptr);
            }
        }
        // CG_Portal: axis[0] aus dem Byte, axis[1] die negierte Senkrechte, axis[2] das Kreuzprodukt
        byteZuDir(dirZuByte(dir), s.achse[0]);
        senkrechteZu(s.achse[0], s.achse[1]);
        for (float& q : s.achse[1]) { q = -q; }
        kreuz(s.achse[0], s.achse[1], s.achse[2]);
        s.skinNum = static_cast<int>(clientNum / 256.0 * 360);
        aus.push_back(s);
    }
    return aus;
}

bool portalSichtWaehlen(const PortalAuge& auge, const std::vector<PortalFlaeche>& flaechen, const std::vector<PortalSchirm>& schirme, double zeitMs,
                        PortalSicht& aus, const std::function<bool(std::size_t)>& imBild, const std::function<bool(std::size_t)>& flaecheImBild) {
    if (schirme.empty()) { return false; }
    int beste = -1;
    int besterSchirm = -1;
    float besteWeite = 0.0F;
    for (std::size_t i = 0; i < flaechen.size(); ++i) {
        const PortalFlaeche& f = flaechen[i];
        if (flaecheImBild && !flaecheImBild(i)) { continue; }
        float kuerzest = 0.0F;
        if (ausserhalb(auge, f, schirme, imBild, kuerzest)) { continue; }
        const int s = schirmFuer(f.ebene, schirme, imBild);
        if (s < 0) { continue; }   // "bad portal, no portalentity"
        if (beste >= 0) {
            const PortalFlaeche& b = flaechen[static_cast<std::size_t>(beste)];
            if (f.sortierung > b.sortierung || (f.sortierung == b.sortierung && kuerzest >= besteWeite)) { continue; }
        }
        beste = static_cast<int>(i);
        besterSchirm = s;
        besteWeite = kuerzest;
    }
    if (beste < 0) { return false; }
    const PortalFlaeche& f = flaechen[static_cast<std::size_t>(beste)];
    const PortalSchirm& e = schirme[static_cast<std::size_t>(besterSchirm)];
    aus = PortalSicht{};
    aus.flaeche = static_cast<std::size_t>(beste);
    aus.schirm = static_cast<std::size_t>(besterSchirm);
    for (int k = 0; k < 4; ++k) { aus.ebene[k] = f.ebene[k]; }
    for (int k = 0; k < 3; ++k) { aus.pvsOrt[k] = e.kamOrt[k]; }   // VectorCopy( e->e.oldorigin, pvsOrigin )
    // surface: axis[0] die Ebenennormale, axis[1] PerpendicularVector, axis[2] das Kreuzprodukt
    float sa[3][3];
    for (int k = 0; k < 3; ++k) { sa[0][k] = f.ebene[k]; }
    senkrechteZu(sa[0], sa[1]);
    kreuz(sa[0], sa[1], sa[2]);
    float so[3];
    float co[3];
    float ca[3][3];
    if (e.spiegel()) {
        // Spiegel: surface->origin = normal * dist, camera = surface mit negierter axis[0]
        for (int k = 0; k < 3; ++k) {
            so[k] = f.ebene[k] * f.ebene[3];
            co[k] = so[k];
            ca[0][k] = -sa[0][k];
            ca[1][k] = sa[1][k];
            ca[2][k] = sa[2][k];
        }
        aus.umkehren = true;
    } else {
        // Portalkamera: der Ort des Schirms auf die Ebene projiziert, die Kamera aus oldorigin und axis
        const float d = punkt(e.ort, f.ebene) - f.ebene[3];
        for (int k = 0; k < 3; ++k) {
            so[k] = e.ort[k] - d * sa[0][k];
            co[k] = e.kamOrt[k];
            ca[0][k] = -e.achse[0][k];
            ca[1][k] = -e.achse[1][k];
            ca[2][k] = e.achse[2][k];
        }
        float grad = 0.0F;
        bool drehen = false;
        if (e.frame != 0) {
            grad = static_cast<float>(zeitMs) / 1000.0F * static_cast<float>(e.frame);   // (tr.refdef.time/1000.0f) * e->e.frame
            drehen = true;
        } else if (e.skinNum != 0) {
            grad = static_cast<float>(e.skinNum);
            drehen = true;
        }
        if (drehen) {
            const float alt[3] = {ca[1][0], ca[1][1], ca[1][2]};
            dreheUmAchse(ca[1], ca[0], alt, grad);
            kreuz(ca[0], ca[1], ca[2]);
        }
        aus.portal = true;
    }
    // R_MirrorPoint/R_MirrorVector: M(x) = C_o + sum_i <x - S_o, S_i> C_i. Gezeichnet wird mit der Umkehrung auf der Welt:
    // L = sum_i S_i C_i^T, t = S_o - L C_o.
    float lin[3][3];
    for (int z = 0; z < 3; ++z) {
        for (int sp = 0; sp < 3; ++sp) { lin[z][sp] = sa[0][z] * ca[0][sp] + sa[1][z] * ca[1][sp] + sa[2][z] * ca[2][sp]; }
    }
    for (int z = 0; z < 3; ++z) {
        for (int sp = 0; sp < 3; ++sp) {
            aus.welt[z * 4 + sp] = lin[z][sp];
            aus.achsen[z * 3 + sp] = lin[sp][z];
        }
        aus.welt[z * 4 + 3] = so[z] - (lin[z][0] * co[0] + lin[z][1] * co[1] + lin[z][2] * co[2]);
    }
    aus.welt[15] = 1.0F;
    const float rel[3] = {auge.ort[0] - so[0], auge.ort[1] - so[1], auge.ort[2] - so[2]};
    for (int k = 0; k < 3; ++k) {
        aus.auge[k] = co[k];
        aus.vorn[k] = 0.0F;
    }
    for (int i = 0; i < 3; ++i) {
        const float dp = punkt(rel, sa[i]);
        const float dv = punkt(auge.vorn, sa[i]);
        for (int k = 0; k < 3; ++k) {
            aus.auge[k] += dp * ca[i][k];
            aus.vorn[k] += dv * ca[i][k];
        }
    }
    // portalPlane: Normale -camera.axis[0], Abstand camera.origin . Normale; glClipPlane genau darauf (RB_BeginDrawingView)
    const float pn[3] = {-ca[0][0], -ca[0][1], -ca[0][2]};
    for (int k = 0; k < 3; ++k) { aus.schnitt[k] = pn[k]; }
    aus.schnitt[3] = -punkt(co, pn);
    return true;
}

}  // namespace bhed
