// gpudraw.cpp - siehe bhed/gpudraw.h
//
// Braucht kein Direct3D. Geprueft in tests/gpudraw.cpp.

#include "bhed/gpudraw.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "bhed/gpuconst.h"
#include "bhed/grasflaechen.h"
#include "bhed/wave.h"

namespace bhed::gpu {

namespace {

// Deckend zuerst, gemischt danach.
//
// Warum nicht einfach nach Zustand sortieren?
// -------------------------------------------
// Weil die Reihenfolge unter den gemischten NICHT beliebig ist. Additives
// Mischen ist zwar vertauschbar (a+b = b+a), aber Alpha ist es nicht: was
// zuerst gezeichnet wird, liegt hinten. Und der Tiefenpuffer hilft nur bei
// deckenden Flaechen - gemischte schreiben ihn meist gar nicht.
//
// Deshalb: nur die deckenden werden nach Zustand gruppiert. Bei den
// gemischten bleibt die Reihenfolge, in der sie in der Karte stehen - genau
// wie der vorhandene Rasterer es macht. Ein Umsortieren dort waere eine
// Aenderung des Bildes, keine Beschleunigung.
int gruppe(const DrawCall& c) {
    // Eine ZUSATZSTUFE kommt nie in die erste Gruppe, auch wenn sie
    // deckend ist.
    //
    // Sie liegt auf derselben Flaeche wie ihre Grundstufe und muss NACH ihr
    // gezeichnet werden. Die erste Gruppe wird aber nach Zustand
    // umgruppiert, damit gleiche beieinanderliegen - dort koennte eine
    // Zusatzstufe vor ihrer eigenen Grundstufe landen.
    //
    // rc608 Teil 11: Die Zusatzstufen DECKENDER Shader bilden eine eigene zweite Gruppe vor allem
    // Durchscheinenden. Im Spiel kommen alle Stufen eines Shaders mit seiner Sortierstufe (R_RadixSort,
    // tr_main.cpp; RB_IterateStagesGeneric, tr_shade.cpp): u_metalorange02 auf duel_jeditemple (env,
    // dann GL_SRC_ALPHA GL_SRC_ALPHA, dann $lightmap) ist SS_OPAQUE und ganz fertig, bevor env_glass
    // (SS_BLEND0) addiert. Mit der Karte als Reihenfolge lag das Glas zwischen den Stufen und wurde
    // mit Alpha und Lightmap des Metalls malgenommen. Die Tiefe der deckenden Grundstufen steht dann
    // schon fest; eine Zusatzstufe trifft mit GL_LEQUAL nur, wo ihre Flaeche vorn liegt.
    if (c.state.blend == Blend::Opaque && !c.zusatzstufe) { return 0; }
    return (c.zusatzstufe && c.grundDeckend) ? 1 : 2;
}

}  // namespace

namespace {
// rc608: Die Eckenfarbe eines .md3 ist das Licht der Entity (CGEN_LIGHTING_DIFFUSE),
// schon fertig und ohne Helligkeitsfaktor. rgbGen vertex liest im Spiel Reste aus
// tess.vertexColors (RB_SurfaceMesh schreibt keine Farben); das bildet nichts nach, es
// gilt dasselbe Licht. identity, const und wave bleiben (l.fest).
void modellLichtSetzen(PipelineState& state, const BspMesh::Batch& b, const Licht& l) {
    if (b.modellLicht && !l.fest && !l.lightmap && !l.roh) {
        state.features |= kModellLicht | kRohEcke;
    }
}
}  // namespace

std::vector<DrawCall> buildDrawCalls(const BspMesh& mesh,
                                     const TextureSet* textures,
                                     float zeitSekunden) {
    std::vector<DrawCall> calls;
    calls.reserve(mesh.batches.size());

    for (std::size_t i = 0; i < mesh.batches.size(); ++i) {
        const BspMesh::Batch& b = mesh.batches[i];
        if (b.numIndexes == 0U) {
            continue;
        }
        BatchState bs = batchStateFor(textures, b.shader, zeitSekunden);
        if (bs.tex == nullptr) {
            // Kein brauchbares Bild. UEBERSPRINGEN, nicht ersetzen.
            //
            // In rc367 hat ein Rueckfall auf "Platz 0" riesige Lavavierecke
            // ins Bild gemalt, weil Platz 0 zufaellig die Lavatextur war.
            // Eine fehlende Textur soll als Loch auffallen, nicht als etwas
            // anderes durchgehen.
            continue;
        }
        DrawCall c;
        Licht l = lichtFuer(bs, b.lightmap, b.vertexColour, false);
        // REST6: rgbGen lightingDiffuse auf der Welt (weltDiffus, gpustate.h)
        c.weltDiffus = mesh.weltEntity && !b.vertexColour && weltDiffus(bs);
        if (c.weltDiffus) {
            weltDiffusSetzen(bs);
            l = Licht{};
            l.fest = true;
        }
        c.state = pipelineFor(bs, l.lightmap, l.roh, false, l.fest);
        modellLichtSetzen(c.state, b, l);
        c.firstIndex = b.firstIndex;
        c.numIndexes = b.numIndexes;
        c.bild = b.shader;
        c.lightmap = b.lightmap;
        c.batch = i;
        c.grundDeckend = c.state.blend == Blend::Opaque;
        calls.push_back(c);

        // --- Und die Stufen darueber -----------------------------------
        //
        // Dieselbe Geometrie, ein anderer Platz im Texturensatz. Der
        // Rasterer legt dafuer eigene Auftraege an (mapview.cpp:1222,
        // "Die Stufen darueber"); hier sind es eigene Zeichenaufrufe.
        //
        // Eine Stufe ohne brauchbares Bild wird UEBERSPRUNGEN, wie die
        // Grundstufe auch - eine fehlende Textur soll als Loch auffallen.
        if (textures == nullptr ||
            static_cast<std::size_t>(b.shader) >= textures->byShader.size()) {
            continue;
        }
        for (const int extra :
             textures->byShader[static_cast<std::size_t>(b.shader)]
                 .extraStages) {
            if (extra < 0 ||
                static_cast<std::size_t>(extra) >= textures->byShader.size()) {
                continue;
            }
            BatchState es = batchStateFor(textures, extra, zeitSekunden);
            if (es.tex == nullptr) {
                continue;
            }
            DrawCall z;
            Licht lz = lichtFuer(es, b.lightmap, b.vertexColour, true);
            z.weltDiffus = mesh.weltEntity && !b.vertexColour && weltDiffus(es);
            if (z.weltDiffus) {
                weltDiffusSetzen(es);
                lz = Licht{};
                lz.fest = true;
            }
            z.state = pipelineFor(es, lz.lightmap, lz.roh, true, lz.fest);
            modellLichtSetzen(z.state, b, lz);
            z.firstIndex = b.firstIndex;
            z.numIndexes = b.numIndexes;
            z.bild = extra;
            z.lightmap = b.lightmap;
            z.batch = i;
            z.zusatzstufe = true;
            z.grundDeckend = c.grundDeckend;
            calls.push_back(z);
        }
    }

    // Stabil sortieren: gleiche Gruppe behaelt ihre Reihenfolge.
    //
    // std::stable_sort und nicht std::sort - bei den gemischten haengt das
    // Bild an der Reihenfolge, und eine instabile Sortierung wuerde sie je
    // nach Standardbibliothek anders vertauschen. Ein Bild, das sich mit dem
    // Compiler aendert, ist nicht abnehmbar.
    std::stable_sort(calls.begin(), calls.end(),
                     [](const DrawCall& a, const DrawCall& b2) {
                         const int ga = gruppe(a);
                         const int gb = gruppe(b2);
                         if (ga != gb) {
                             return ga < gb;
                         }
                         if (ga != 0) {
                             // Gemischt: Reihenfolge der Karte behalten.
                             return false;
                         }
                         // Deckend: nach Zustand gruppieren, damit gleiche
                         // beieinanderliegen und kein Wechsel noetig ist.
                         if (a.state.features != b2.state.features) {
                             return a.state.features < b2.state.features;
                         }
                         if (a.state.numTexMods != b2.state.numTexMods) {
                             return a.state.numTexMods < b2.state.numTexMods;
                         }
                         return false;
                     });
    return calls;
}

std::vector<DrawCall> nurLage(const std::vector<DrawCall>& calls, Lage lage) {
    if (lage == Lage::Alles) {
        return calls;
    }
    std::vector<DrawCall> raus;
    raus.reserve(calls.size());
    for (const DrawCall& c : calls) {
        // Eine Zusatzstufe zeichnet mit ihrer Grundstufe (siehe gruppe).
        const bool deckend = c.zusatzstufe ? c.grundDeckend : (c.state.blend == Blend::Opaque);
        if ((lage == Lage::Deckend) == deckend) {
            raus.push_back(c);
        }
    }
    return raus;
}

std::vector<DrawCall> nurGluehende(const std::vector<DrawCall>& calls) {
    std::vector<DrawCall> aus;
    for (const DrawCall& c : calls) {
        if ((c.state.features & kGlow) != 0U) {
            aus.push_back(c);
        }
    }
    return aus;
}

namespace {

// VectorNormalize (shared/qcommon/q_math.c:1158 OpenJK, MD9 gleich): sqrtf, ilength = 1/length, je Achse mal ilength; 0 bleibt 0
void vectorNormalize(float v[3]) {
    float laenge = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    laenge = std::sqrt(laenge);
    if (laenge != 0.0F) {
        const float il = 1.0F / laenge;
        v[0] *= il;
        v[1] *= il;
        v[2] *= il;
    }
}

// RB_CalcDeformVertexes, RB_CalcDeformNormals, RB_CalcBulgeVertexes, RB_CalcMoveVertexes (tr_shade_calc.cpp:129-280) ueber alle
// Ecken von tess - dieselben float-Ausdruecke in derselben Reihenfolge. floatTime ist die Zeit der Vorschau in Sekunden (wie
// gpu::verformung fuer den Vertex-Shader), refdef.time die ganzen ms daraus (spielZeitMs).
void verformeJeEcke(const DeformEintrag& e, float zeit, std::vector<TessEcke>& ecken) {
    switch (e.art) {
        case kFolgeWelle:
            if (e.welle.frequency == 0.0F) {
                const float scale = waveValue(e.welle, zeit);   // EvalWaveForm
                for (TessEcke& t : ecken) {
                    for (int k = 0; k < 3; ++k) { t.xyz[k] += t.normal[k] * scale; }   // VectorScale, dann +=
                }
            } else {
                for (TessEcke& t : ecken) {
                    const float off = (t.xyz[0] + t.xyz[1] + t.xyz[2]) * e.spread;
                    // WAVEVALUE (:29): base + table[Q_ftol(((phase + off) + floatTime * freq) * FUNCTABLE_SIZE) & MASK] * amplitude
                    const float scale = e.welle.base + funcTafel(e.welle.func, funcIndex(((e.welle.phase + off) + zeit * e.welle.frequency) * 1024.0F)) *
                                                           e.welle.amplitude;
                    for (int k = 0; k < 3; ++k) { t.xyz[k] += t.normal[k] * scale; }
                }
            }
            break;
        case kFolgeNormal: {
            const float now = zeit * e.werte[1];   // floatTime * deformationWave.frequency
            for (TessEcke& t : ecken) {
                for (int k = 0; k < 3; ++k) {
                    const float scale = 0.98F;
                    const float x = (k == 0) ? t.xyz[0] * scale : static_cast<float>(100 * k) + t.xyz[0] * scale;
                    t.normal[k] += e.werte[0] * rauschen4(x, t.xyz[1] * scale, t.xyz[2] * scale, now);
                }
                // VectorNormalizeFast (q_math.c:1147): ilength = Q_rsqrt(DotProduct)
                const float il = gras::qRsqrt(t.normal[0] * t.normal[0] + t.normal[1] * t.normal[1] + t.normal[2] * t.normal[2]);
                for (float& n : t.normal) { n *= il; }
            }
            break;
        }
        case kFolgeBeule: {
            const float* b = e.werte;   // width, height, speed
            if (b[2] == 0.0F && b[0] == 0.0F) {
                for (TessEcke& t : ecken) {
                    for (int k = 0; k < 3; ++k) { t.xyz[k] += t.normal[k] * b[1]; }
                }
            } else {
                // now = backEnd.refdef.time * bulgeSpeed * 0.001f; off = (float)(FUNCTABLE_SIZE / (M_PI*2)) * (st[0] * width + now),
                // als int abgeschnitten (M_PI ist float, q_math.h:40)
                const float now = static_cast<float>(spielZeitMs(zeit)) * b[2] * 0.001F;
                const float faktor = 1024.0F / (3.14159265358979323846F * 2.0F);
                for (TessEcke& t : ecken) {
                    const float scale = funcTafel(WaveFunc::Sin, funcIndex(faktor * (t.st[0] * b[0] + now))) * b[1];
                    for (int k = 0; k < 3; ++k) { t.xyz[k] += t.normal[k] * scale; }
                }
            }
            break;
        }
        case kFolgeWeg: {
            const float scale = waveValue(e.welle, zeit);   // WAVEVALUE ohne off (TableForFunc)
            float offset[3];
            for (int k = 0; k < 3; ++k) { offset[k] = e.werte[k] * scale; }
            for (TessEcke& t : ecken) {
                for (int k = 0; k < 3; ++k) { t.xyz[k] += offset[k]; }
            }
            break;
        }
        default:
            break;
    }
}

// AutospriteDeform (tr_shade_calc.cpp:377) mit RB_AddQuadStamp/RB_AddQuadStampExt (tr_surface.cpp:77-154)
bool autospriteDeform(const SpriteSicht& s, std::vector<TessEcke>& ecken, std::vector<std::uint32_t>& idx) {
    const std::size_t n = ecken.size();
    if ((n & 3U) != 0U || idx.size() != (n >> 2U) * 6U) { return false; }   // ERR_DROP "had odd vertex/index count"
    std::vector<TessEcke> neu(n);
    for (std::size_t i = 0; i < n; i += 4) {
        const TessEcke* q = &ecken[i];
        float mid[3];
        for (int k = 0; k < 3; ++k) { mid[k] = 0.25F * (q[0].xyz[k] + q[1].xyz[k] + q[2].xyz[k] + q[3].xyz[k]); }
        const float delta[3] = {q[0].xyz[0] - mid[0], q[0].xyz[1] - mid[1], q[0].xyz[2] - mid[2]};
        // VectorLength (q_math.c): sqrt(DotProduct) in float
        const float radius = std::sqrt(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]) * 0.707F;
        float left[3];
        float up[3];
        for (int k = 0; k < 3; ++k) {
            left[k] = s.links[k] * radius;
            up[k] = s.hoch[k] * radius;
        }
        if (s.spiegel) {
            for (float& l : left) { l = 0.0F - l; }   // VectorSubtract(vec3_origin, left, left)
        }
        TessEcke* z = &neu[i];
        for (int k = 0; k < 3; ++k) {
            z[0].xyz[k] = mid[k] + left[k] + up[k];
            z[1].xyz[k] = mid[k] - left[k] + up[k];
            z[2].xyz[k] = mid[k] - left[k] - up[k];
            z[3].xyz[k] = mid[k] + left[k] - up[k];
        }
        static const float kSt[4][2] = {{0.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 1.0F}, {0.0F, 1.0F}};
        for (int w = 0; w < 4; ++w) {
            for (int k = 0; k < 3; ++k) { z[w].normal[k] = 0.0F - s.vornWelt[k]; }   // -backEnd.viewParms.ori.axis[0], ungedreht
            z[w].st[0] = z[w].lm[0] = kSt[w][0];                                      // texCoords[ndx][0] und [1]
            z[w].st[1] = z[w].lm[1] = kSt[w][1];
            std::copy(q[0].farbe, q[0].farbe + 4, z[w].farbe);                         // tess.vertexColors[i]
        }
        const std::uint32_t b = static_cast<std::uint32_t>(i);
        const std::uint32_t wick[6] = {b, b + 1U, b + 3U, b + 3U, b + 1U, b + 2U};
        std::copy(wick, wick + 6, idx.begin() + static_cast<std::ptrdiff_t>((i >> 2U) * 6U));
    }
    ecken.swap(neu);
    return true;
}

// Autosprite2Deform (tr_shade_calc.cpp:444): die beiden kuerzesten Kanten (Startwert WORLD_SIZE = 131072, q_shared.h:804), die
// lange Achse zwischen ihren Mitten (nicht normiert), minor = VectorNormalize(CrossProduct(major, forward)), die Ecken der kurzen
// Kanten auf mid +- l * minor (VectorMA) mit l = 0.5 * sqrt(lengths[j]) in double. Bei falscher Ecken- oder Indexzahl warnt das
// Spiel nur und rechnet weiter.
void autosprite2Deform(const SpriteSicht& s, std::vector<TessEcke>& ecken, const std::vector<std::uint32_t>& idx) {
    static const std::uint32_t kKanten[6][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};
    const std::size_t n = ecken.size();
    for (std::size_t i = 0, indexes = 0; i < n; i += 4, indexes += 6) {
        if (i + 4 > n) { break; }   // tess.xyz hinter der letzten Ecke liest das Spiel aus dem Rest des Puffers
        TessEcke* xyz = &ecken[i];
        float lengths[2] = {131072.0F, 131072.0F};
        int nums[2] = {0, 0};
        for (int j = 0; j < 6; ++j) {
            const float* v1 = xyz[kKanten[j][0]].xyz;
            const float* v2 = xyz[kKanten[j][1]].xyz;
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
            const float* v1 = xyz[kKanten[nums[j]][0]].xyz;
            const float* v2 = xyz[kKanten[nums[j]][1]].xyz;
            for (int k = 0; k < 3; ++k) { mid[j][k] = 0.5F * (v1[k] + v2[k]); }
        }
        const float major[3] = {mid[1][0] - mid[0][0], mid[1][1] - mid[0][1], mid[1][2] - mid[0][2]};
        // CrossProduct (q_math.c)
        float minor[3] = {major[1] * s.vorn[2] - major[2] * s.vorn[1], major[2] * s.vorn[0] - major[0] * s.vorn[2],
                          major[0] * s.vorn[1] - major[1] * s.vorn[0]};
        vectorNormalize(minor);
        for (int j = 0; j < 2; ++j) {
            const std::uint32_t e1 = kKanten[nums[j]][0];
            const std::uint32_t e2 = kKanten[nums[j]][1];
            const auto l = static_cast<float>(0.5 * static_cast<double>(std::sqrt(lengths[j])));
            int k = 0;
            for (; k < 5; ++k) {
                if (indexes + static_cast<std::size_t>(k) + 1U < idx.size() && idx[indexes + static_cast<std::size_t>(k)] == i + e1 &&
                    idx[indexes + static_cast<std::size_t>(k) + 1U] == i + e2) {
                    break;
                }
            }
            const float l1 = (k == 5) ? l : -l;
            float* v1 = xyz[e1].xyz;
            float* v2 = xyz[e2].xyz;
            for (int a = 0; a < 3; ++a) {   // VectorMA(mid, scale, minor, out): out = mid + scale * minor
                v1[a] = mid[j][a] + l1 * minor[a];
                v2[a] = mid[j][a] + (-l1) * minor[a];
            }
        }
    }
}

}  // namespace

bool verformeTess(const DeformZusatz& z, float zeitSekunden, const SpriteSicht& sicht, std::vector<TessEcke>& ecken,
                  std::vector<std::uint32_t>& idx) {
    for (int i = 0; i < z.anzahl; ++i) {
        // TableForFunc bricht im Spiel ab (ERR_DROP) - die Vorschau laesst den Eintrag aus wie der Vertex-Shader (deformLage)
        if (deformLage(z, i) == DeformLage::Abbruch) { continue; }
        const DeformEintrag& e = z.eintraege[i];
        if (e.art == kFolgeSprite) {
            if (!autospriteDeform(sicht, ecken, idx)) { return false; }
        } else if (e.art == kFolgeSprite2) {
            autosprite2Deform(sicht, ecken, idx);
        } else {
            verformeJeEcke(e, zeitSekunden, ecken);
        }
    }
    return true;
}

void gitterTess(const std::vector<TessEcke>& gitter, int breite, const std::vector<int>& spalten, const std::vector<int>& zeilen,
                std::vector<TessEcke>& te, std::vector<std::uint32_t>& idx) {
    te.clear();
    idx.clear();
    const std::size_t lodWidth = spalten.size();
    for (const int z : zeilen) {
        for (const int s : spalten) {
            te.push_back(gitter[static_cast<std::size_t>(z) * static_cast<std::size_t>(breite) + static_cast<std::size_t>(s)]);
        }
    }
    for (std::size_t i = 0; i + 1 < zeilen.size(); ++i) {
        for (std::size_t j = 0; j + 1 < lodWidth; ++j) {
            // "vertex order to be reckognized as tristrips"
            const auto v1 = static_cast<std::uint32_t>(i * lodWidth + j + 1U);
            const std::uint32_t v2 = v1 - 1U;
            const auto v3 = static_cast<std::uint32_t>(v2 + lodWidth);
            const std::uint32_t v4 = v3 + 1U;
            idx.insert(idx.end(), {v2, v3, v1, v1, v3, v4});
        }
    }
}

std::size_t countStateChanges(const std::vector<DrawCall>& calls) {
    if (calls.empty()) {
        return 0U;
    }
    std::size_t n = 1U;
    for (std::size_t i = 1; i < calls.size(); ++i) {
        if (!calls[i].state.gleichWie(calls[i - 1].state)) {
            ++n;
        }
    }
    return n;
}

}  // namespace bhed::gpu
