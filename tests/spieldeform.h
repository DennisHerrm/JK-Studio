// spieldeform.h - deformVertexes wie rd-vanilla, abgeschrieben fuer die Proben (tests/defgpu.cpp, gpudraw.cpp; nur lesen, nicht im
// Programm): R_InitFuncTables (tr_init.cpp:1724), R_NoiseInit/R_NoiseGet4f (rd-common/tr_noise.cpp, rand() der MSVC-Laufzeit),
// WAVEVALUE, EvalWaveForm, RB_CalcDeformVertexes/Normals/BulgeVertexes/MoveVertexes und RB_DeformTessGeometry mit AutospriteDeform
// und Autosprite2Deform (code/rd-vanilla/tr_shade_calc.cpp, MD9 gleich). spielInit() vor dem ersten Gebrauch.
#ifndef BHED_TESTS_SPIELDEFORM_H
#define BHED_TESTS_SPIELDEFORM_H

#include "bhed/shaderscript.h"
#include "bhed/wave.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace spieldeform {

using bhed::DeformEintrag;
using bhed::DeformZusatz;
using bhed::kFolgeBeule;
using bhed::kFolgeNormal;
using bhed::kFolgeSprite;
using bhed::kFolgeSprite2;
using bhed::kFolgeWeg;
using bhed::kFolgeWelle;
using bhed::kMaxShaderDeforms;
using bhed::Wave;
using bhed::WaveFunc;

// --- Das Spiel ---------------------------------------------------------------------------------------------------------------
inline float sinTable[1024];
inline float triangleTable[1024];
inline float squareTable[1024];
inline float sawToothTable[1024];
inline float inverseSawToothTable[1024];
inline float noiseTable[256];
inline int noisePerm[256];
inline void spielInit() {
    for (int i = 0; i < 1024; i++) {
        sinTable[i] = std::sin((static_cast<float>(i) * 360.0F / static_cast<float>(1024 - 1)) * 0.017453292519943295769236907684886F);
        squareTable[i] = (i < 512) ? 1.0F : -1.0F;
        sawToothTable[i] = static_cast<float>(i) / 1024;
        inverseSawToothTable[i] = 1.0F - sawToothTable[i];
        if (i < 512) {
            triangleTable[i] = (i < 256) ? static_cast<float>(i) / 256 : 1.0F - triangleTable[i - 256];
        } else {
            triangleTable[i] = -triangleTable[i - 512];
        }
    }
    std::uint32_t halt = 1001U;   // srand(1001), rand() der MSVC-Laufzeit
    const auto zufall = [&halt] {
        halt = halt * 214013U + 2531011U;
        return static_cast<int>((halt >> 16U) & 0x7fffU);
    };
    for (int i = 0; i < 256; i++) {
        noiseTable[i] = static_cast<float>(((static_cast<float>(zufall()) / 32767.0F) * 2.0 - 1.0));
        noisePerm[i] = static_cast<unsigned char>(static_cast<float>(zufall()) / 32767.0F * 255);
    }
}
inline int qFtol(float f) { return (std::fabs(f) < 2147483648.0F) ? static_cast<int>(f) : (-2147483647 - 1); }
inline unsigned val(unsigned a) { return static_cast<unsigned>(noisePerm[a & 255U]); }
inline float noiseValue(int x, int y, int z, int t) {
    const auto u = [](int v) { return static_cast<unsigned>(v); };
    return noiseTable[val(u(x) + val(u(y) + val(u(z) + val(u(t)))))];
}
inline float lerp(float a, float b, float w) { return a * (1.0F - w) + b * w; }
inline float rNoiseGet4f(float x, float y, float z, float t) {
    const int ix = qFtol(std::floor(x));
    const float fx = x - static_cast<float>(ix);
    const int iy = qFtol(std::floor(y));
    const float fy = y - static_cast<float>(iy);
    const int iz = qFtol(std::floor(z));
    const float fz = z - static_cast<float>(iz);
    const int it = qFtol(std::floor(t));
    const float ft = t - static_cast<float>(it);
    float value[2];
    for (int i = 0; i < 2; i++) {
        const float fv = lerp(lerp(noiseValue(ix, iy, iz, it + i), noiseValue(ix + 1, iy, iz, it + i), fx),
                              lerp(noiseValue(ix, iy + 1, iz, it + i), noiseValue(ix + 1, iy + 1, iz, it + i), fx), fy);
        const float bv = lerp(lerp(noiseValue(ix, iy, iz + 1, it + i), noiseValue(ix + 1, iy, iz + 1, it + i), fx),
                              lerp(noiseValue(ix, iy + 1, iz + 1, it + i), noiseValue(ix + 1, iy + 1, iz + 1, it + i), fx), fy);
        value[i] = lerp(fv, bv, fz);
    }
    return lerp(value[0], value[1], ft);
}
inline const float* tableForFunc(WaveFunc f) {
    switch (f) {
        case WaveFunc::Sin: return sinTable;
        case WaveFunc::Triangle: return triangleTable;
        case WaveFunc::Square: return squareTable;
        case WaveFunc::Sawtooth: return sawToothTable;
        default: return inverseSawToothTable;
    }
}
inline float waveValueSpiel(const Wave& w, float phase, float floatTime) {
    return w.base + tableForFunc(w.func)[qFtol((phase + floatTime * w.frequency) * 1024) & 1023] * w.amplitude;
}
inline float evalWaveForm(const Wave& w, float floatTime) {
    if (w.func == WaveFunc::Noise) { return (w.base + rNoiseGet4f(0, 0, 0, (floatTime + w.phase) * w.frequency) * w.amplitude); }
    return waveValueSpiel(w, w.phase, floatTime);
}
inline float qRsqrt(float number) {
    const float x2 = number * 0.5F;
    std::int32_t i = 0;
    std::memcpy(&i, &number, 4);
    i = 0x5f3759df - (i >> 1);
    float y = 0.0F;
    std::memcpy(&y, &i, 4);
    return y * (1.5F - (x2 * y * y));
}

// RB_DeformTessGeometry an einer Ecke, Eintraege von..bis-1 (autosprite/autosprite2 rechnet der Aufrufer): xyz, normal, st[0];
// time in ms, floatTime = time * 0.001 als float
inline void spielVerformen(const DeformZusatz& z, int time, float xyz[3], float normal[3], float s, int von = 0, int bis = kMaxShaderDeforms) {
    const auto floatTime = static_cast<float>(time * 0.001);
    for (int i = von; i < bis && i < z.anzahl; ++i) {
        const DeformEintrag& e = z.eintraege[i];
        if (e.art == kFolgeWelle) {
            float scale = 0.0F;
            if (e.welle.frequency == 0) {
                scale = evalWaveForm(e.welle, floatTime);
            } else {
                const float off = (xyz[0] + xyz[1] + xyz[2]) * e.spread;
                scale = waveValueSpiel(e.welle, e.welle.phase + off, floatTime);
            }
            for (int k = 0; k < 3; ++k) { xyz[k] += normal[k] * scale; }
        } else if (e.art == kFolgeNormal) {
            float scale = 0.98F;
            scale = rNoiseGet4f(xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, floatTime * e.werte[1]);
            normal[0] += e.werte[0] * scale;
            scale = 0.98F;
            scale = rNoiseGet4f(100 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, floatTime * e.werte[1]);
            normal[1] += e.werte[0] * scale;
            scale = 0.98F;
            scale = rNoiseGet4f(200 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, floatTime * e.werte[1]);
            normal[2] += e.werte[0] * scale;
            const float il = qRsqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
            for (int k = 0; k < 3; ++k) { normal[k] *= il; }
        } else if (e.art == kFolgeBeule) {
            const float* b = e.werte;
            if (b[2] == 0.0F && b[0] == 0.0F) {
                for (int k = 0; k < 3; ++k) { xyz[k] += normal[k] * b[1]; }
            } else {
                const float now = static_cast<float>(time) * b[2] * 0.001F;
                const int off = qFtol(static_cast<float>(1024) / (3.14159265358979323846F * 2) * (s * b[0] + now));
                const float scale = sinTable[off & 1023] * b[1];
                for (int k = 0; k < 3; ++k) { xyz[k] += normal[k] * scale; }
            }
        } else if (e.art == kFolgeWeg) {
            const float scale = waveValueSpiel(e.welle, e.welle.phase, floatTime);
            for (int k = 0; k < 3; ++k) { xyz[k] += e.werte[k] * scale; }
        }
    }
}

// REST3: RB_DeformTessGeometry (tr_shade_calc.cpp:549) ueber eine ganze tess - die Eintraege der Reihe nach, AutospriteDeform
// (:377), Autosprite2Deform (:444), GlobalVectorToLocal (:363), RB_AddQuadStampExt (tr_surface.cpp:77), VectorNormalize
// (q_math.c:1158), CrossProduct, VectorMA, WORLD_SIZE (q_shared.h:804) - hier abgeschrieben, unabhaengig von gpu::verformeTess.
struct SpielTess {
    std::vector<std::array<float, 3>> xyz;
    std::vector<std::array<float, 3>> normal;
    std::vector<std::array<float, 2>> st;   // texCoords[i][0]
    std::vector<std::array<float, 2>> lm;   // texCoords[i][1]
    std::vector<std::array<std::uint8_t, 4>> farbe;
    std::vector<std::uint32_t> idx;
};
struct SpielBlick {
    float axis[3][3];               // backEnd.viewParms.ori.axis (Welt)
    const float* ent = nullptr;     // backEnd.ori.axis einer Entity (3 x 3, Zeile k = axis[k]); nullptr: tr.worldEntity
    bool isMirror = false;
};
inline void globalVectorToLocal(const SpielBlick& v, const float in[3], float out[3]) {
    for (int k = 0; k < 3; ++k) { out[k] = in[0] * v.ent[3 * k] + in[1] * v.ent[3 * k + 1] + in[2] * v.ent[3 * k + 2]; }
}
inline void spielAddQuadStampExt(const SpielBlick& v, SpielTess& neu, const float origin[3], const float left[3], const float up[3],
                                 const std::array<std::uint8_t, 4>& color) {
    const auto ndx = static_cast<std::uint32_t>(neu.xyz.size());
    for (const std::uint32_t i : {ndx, ndx + 1, ndx + 3, ndx + 3, ndx + 1, ndx + 2}) { neu.idx.push_back(i); }
    neu.xyz.push_back({origin[0] + left[0] + up[0], origin[1] + left[1] + up[1], origin[2] + left[2] + up[2]});
    neu.xyz.push_back({origin[0] - left[0] + up[0], origin[1] - left[1] + up[1], origin[2] - left[2] + up[2]});
    neu.xyz.push_back({origin[0] - left[0] - up[0], origin[1] - left[1] - up[1], origin[2] - left[2] - up[2]});
    neu.xyz.push_back({origin[0] + left[0] - up[0], origin[1] + left[1] - up[1], origin[2] + left[2] - up[2]});
    const std::array<float, 3> normal = {0.0F - v.axis[0][0], 0.0F - v.axis[0][1], 0.0F - v.axis[0][2]};
    const float s1 = 0.0F, t1 = 0.0F, s2 = 1.0F, t2 = 1.0F;
    const std::array<float, 2> st[4] = {{s1, t1}, {s2, t1}, {s2, t2}, {s1, t2}};
    for (int w = 0; w < 4; ++w) {
        neu.normal.push_back(normal);
        neu.st.push_back(st[w]);
        neu.lm.push_back(st[w]);
        neu.farbe.push_back(color);
    }
}
inline void spielAutospriteDeform(const SpielBlick& v, SpielTess& t) {
    float leftDir[3];
    float upDir[3];
    if (v.ent != nullptr) {
        globalVectorToLocal(v, v.axis[1], leftDir);
        globalVectorToLocal(v, v.axis[2], upDir);
    } else {
        for (int k = 0; k < 3; ++k) {
            leftDir[k] = v.axis[1][k];
            upDir[k] = v.axis[2][k];
        }
    }
    SpielTess neu;
    for (std::size_t i = 0; i < t.xyz.size(); i += 4) {
        const float* xyz[4] = {t.xyz[i].data(), t.xyz[i + 1].data(), t.xyz[i + 2].data(), t.xyz[i + 3].data()};
        float mid[3];
        for (int k = 0; k < 3; ++k) { mid[k] = 0.25F * (xyz[0][k] + xyz[1][k] + xyz[2][k] + xyz[3][k]); }
        const float delta[3] = {xyz[0][0] - mid[0], xyz[0][1] - mid[1], xyz[0][2] - mid[2]};
        const float radius = static_cast<float>(std::sqrt(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2])) * 0.707F;
        float left[3];
        float up[3];
        for (int k = 0; k < 3; ++k) {
            left[k] = leftDir[k] * radius;
            up[k] = upDir[k] * radius;
        }
        if (v.isMirror) {
            for (float& l : left) { l = 0.0F - l; }
        }
        spielAddQuadStampExt(v, neu, mid, left, up, t.farbe[i]);
    }
    t = neu;
}
inline void spielAutosprite2Deform(const SpielBlick& v, SpielTess& t) {
    static const std::uint32_t edgeVerts[6][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};
    float forward[3];
    if (v.ent != nullptr) {
        globalVectorToLocal(v, v.axis[0], forward);
    } else {
        for (int k = 0; k < 3; ++k) { forward[k] = v.axis[0][k]; }
    }
    for (std::size_t i = 0, indexes = 0; i < t.xyz.size(); i += 4, indexes += 6) {
        float lengths[2] = {131072.0F, 131072.0F};   // WORLD_SIZE
        int nums[2] = {0, 0};
        for (int j = 0; j < 6; ++j) {
            const float* v1 = t.xyz[i + edgeVerts[j][0]].data();
            const float* v2 = t.xyz[i + edgeVerts[j][1]].data();
            const float temp[3] = {v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2]};
            const float l = temp[0] * temp[0] + temp[1] * temp[1] + temp[2] * temp[2];
            if (l < lengths[0]) {
                nums[1] = nums[0];
                lengths[1] = lengths[0];
                nums[0] = j;
                lengths[0] = l;
            } else if (l < lengths[1]) {
                nums[1] = j;
                lengths[1] = l;
            }
        }
        float mid[2][3];
        for (int j = 0; j < 2; ++j) {
            const float* v1 = t.xyz[i + edgeVerts[nums[j]][0]].data();
            const float* v2 = t.xyz[i + edgeVerts[nums[j]][1]].data();
            for (int k = 0; k < 3; ++k) { mid[j][k] = 0.5F * (v1[k] + v2[k]); }
        }
        const float major[3] = {mid[1][0] - mid[0][0], mid[1][1] - mid[0][1], mid[1][2] - mid[0][2]};
        float minor[3] = {major[1] * forward[2] - major[2] * forward[1], major[2] * forward[0] - major[0] * forward[2],
                          major[0] * forward[1] - major[1] * forward[0]};
        float length = minor[0] * minor[0] + minor[1] * minor[1] + minor[2] * minor[2];
        length = sqrtf(length);
        if (length != 0.0F) {
            const float ilength = 1 / length;
            for (float& m : minor) { m *= ilength; }
        }
        for (int j = 0; j < 2; ++j) {
            float* v1 = t.xyz[i + edgeVerts[nums[j]][0]].data();
            float* v2 = t.xyz[i + edgeVerts[nums[j]][1]].data();
            const float l = static_cast<float>(0.5 * std::sqrt(static_cast<double>(lengths[j])));
            int k = 0;
            for (; k < 5; k++) {
                if (t.idx[indexes + static_cast<std::size_t>(k)] == i + edgeVerts[nums[j]][0] &&
                    t.idx[indexes + static_cast<std::size_t>(k) + 1] == i + edgeVerts[nums[j]][1]) {
                    break;
                }
            }
            for (int a = 0; a < 3; ++a) {
                if (k == 5) {
                    v1[a] = mid[j][a] + l * minor[a];
                    v2[a] = mid[j][a] + -l * minor[a];
                } else {
                    v1[a] = mid[j][a] + -l * minor[a];
                    v2[a] = mid[j][a] + l * minor[a];
                }
            }
        }
    }
}
inline void spielDeformTess(const DeformZusatz& z, int time, const SpielBlick& v, SpielTess& t) {
    for (int i = 0; i < z.anzahl; ++i) {
        const int art = z.eintraege[i].art;
        if (art == kFolgeSprite) {
            spielAutospriteDeform(v, t);
        } else if (art == kFolgeSprite2) {
            spielAutosprite2Deform(v, t);
        } else {
            for (std::size_t e = 0; e < t.xyz.size(); ++e) { spielVerformen(z, time, t.xyz[e].data(), t.normal[e].data(), t.st[e][0], i, i + 1); }
        }
    }
}

}  // namespace spieldeform

#endif  // BHED_TESTS_SPIELDEFORM_H
