// Probe: rechnen die Vertex-Shader der Karte und der Figuren deformVertexes bitgleich wie rd-vanilla? (nur Windows/MSVC)
//
// FIX5: Die echten Vertex-Shader (gpu::vertexShaderHlsl mit den deform-Merkmalen, gpu::figurVertexShaderHlsl) laufen so, wie die
// Vorschau sie baut - vs_5_0 (ab Merkmalsstufe 11_0, kartenVsZiel), Fahnen 0 (P35) - ueber Stream-Output (tests/vsstrom.h); die Kamera
// ist die Einheitsmatrix, also kommt die verformte Ecke unveraendert heraus. Der Bytecode wird mitgeprueft ([precise] je Anweisung). Auf WARP (der Software-Grafikkarte von Direct3D 11) und, wenn vorhanden, der Grafikkarte des Rechners. Die Werte packen
// die Wege des Programms (Karte: gpu::packeKonstanten; Figur: gpu::verformung wie deformWerte in gpumap_win32.cpp). Verglichen
// wird Bit fuer Bit mit RB_DeformTessGeometry (code/rd-vanilla/tr_shade_calc.cpp:553, MD9 gleich), hier abgeschrieben:
// RB_CalcDeformVertexes (:129), RB_CalcDeformNormals (:182), RB_CalcBulgeVertexes (:216), RB_CalcMoveVertexes (:263) mit WAVEVALUE,
// EvalWaveForm, R_InitFuncTables, R_NoiseGet4f (rand() der MSVC-Laufzeit) und VectorNormalizeFast/Q_rsqrt. normal wirkt auf die
// Ecke, wenn wave oder bulge danach kommen - so wird es hier sichtbar; die Normale selbst prueft der dritte Teil (verformeEcke).

#include "bhed/gpuconst.h"
#include "bhed/gpudraw.h"
#include "bhed/gpushader.h"
#include "bhed/gpustate.h"
#include "spieldeform.h"
#include "vsstrom.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

using namespace bhed;
using namespace bhed::gpu;
using namespace spieldeform;

namespace {

int fehler = 0;

void erwarte(const std::string& was, bool ok) {
    std::printf("  %-4s  %s\n", ok ? "ok" : "FEHL", was.c_str());
    if (!ok) { ++fehler; }
}

template <class T>
void frei(T*& p) {
    if (p != nullptr) {
        p->Release();
        p = nullptr;
    }
}

// ComputeTexCoords (tr_shade.cpp:1725) fuer die tcMod-Kette, wie tests/turbgpu.cpp: jeder Schritt der Reihe nach in float
void spielSt(const TexMod* m, int n, float zeit, const float ort[2], float st[2]) {
    for (int k = 0; k < n; ++k) {
        const TexMod& t = m[k];
        const float s = st[0];
        const float u = st[1];
        switch (t.kind) {
            case TexModKind::Turb: {
                const float now = t.wave.phase + zeit * t.wave.frequency;
                st[0] = s + sinTable[static_cast<int>((ort[0] * 1.0 / 128 * 0.125 + now) * 1024) & 1023] * t.wave.amplitude;
                st[1] = u + sinTable[static_cast<int>((ort[1] * 1.0 / 128 * 0.125 + now) * 1024) & 1023] * t.wave.amplitude;
                break;
            }
            case TexModKind::Scale:
                st[0] = s * t.a[0];
                st[1] = u * t.a[1];
                break;
            case TexModKind::Scroll: {
                float a = t.a[0] * zeit;
                float b = t.a[1] * zeit;
                a = a - std::floor(a);
                b = b - std::floor(b);
                st[0] = s + a;
                st[1] = u + b;
                break;
            }
            case TexModKind::Rotate: {
                const float degs = -t.a[0] * zeit;
                const auto index = static_cast<int>(degs * (1024 / 360.0F));
                const float sn = sinTable[index & 1023];
                const float cs = sinTable[(index + 1024 / 4) & 1023];
                const auto tr0 = static_cast<float>(0.5 - 0.5 * cs + 0.5 * sn);
                const auto tr1 = static_cast<float>(0.5 - 0.5 * sn - 0.5 * cs);
                st[0] = s * cs + u * -sn + tr0;
                st[1] = s * sn + u * cs + tr1;
                break;
            }
            case TexModKind::Transform:
                st[0] = s * t.a[0] + u * t.a[2] + t.a[4];
                st[1] = s * t.a[1] + u * t.a[3] + t.a[5];
                break;
            default: break;
        }
    }
}

// Ein Fall: ein Shader, gelesen vom Shaderleser des Programms (parseShaderScript) - die Eintraege von shader.deforms[]
struct Fall {
    const char* was;
    ShaderInfo info;
};

// Der Zustand eines Stapels aus dem gelesenen Shader (wie batchStateFor aus TextureSet::Tex)
BatchState stapel(const ShaderInfo& info) {
    BatchState bs{};
    bs.deformWave = info.deformWave;
    bs.deformSpread = info.deformSpread;
    bs.deform2 = info.deform2;
    bs.autosprite = info.autosprite;
    return bs;
}

// --- Direct3D ----------------------------------------------------------------------------------------------------------------
struct Puffer {
    UINT slot;
    std::vector<float> daten;
};

// Kern `kern` auf einem Geraet: gEin (t0, float4), gAus (u0, float4 je Probe), Konstantenpuffer; false: kein Geraet / nicht uebersetzt
bool laufen(D3D_DRIVER_TYPE art, const std::string& quelle, const std::vector<Puffer>& cbs, const std::vector<float>& ein, UINT proben,
            std::vector<std::uint32_t>& aus, std::string& name) {
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    const D3D_FEATURE_LEVEL stufe = D3D_FEATURE_LEVEL_11_0;
    if (FAILED(D3D11CreateDevice(nullptr, art, nullptr, 0, &stufe, 1, D3D11_SDK_VERSION, &dev, nullptr, &ctx))) { return false; }
    IDXGIDevice* dx = nullptr;
    IDXGIAdapter* ad = nullptr;
    DXGI_ADAPTER_DESC ds{};
    if (SUCCEEDED(dev->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dx))) && SUCCEEDED(dx->GetAdapter(&ad))) { ad->GetDesc(&ds); }
    frei(ad);
    frei(dx);
    char n[128];
    std::snprintf(n, sizeof n, "%ls", ds.Description);
    name = n;
    ID3DBlob* blob = nullptr;
    ID3DBlob* err = nullptr;
    if (FAILED(D3DCompile(quelle.data(), quelle.size(), "defgpu", nullptr, nullptr, "kern", "cs_5_0", 0, 0, &blob, &err))) {
        std::printf("%s\n", err != nullptr ? static_cast<const char*>(err->GetBufferPointer()) : "D3DCompile");
        frei(err);
        frei(ctx);
        frei(dev);
        name.clear();
        return false;
    }
    frei(err);
    ID3D11ComputeShader* cs = nullptr;
    dev->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &cs);
    frei(blob);

    std::vector<ID3D11Buffer*> kb;
    for (const Puffer& p : cbs) {
        std::vector<float> d = p.daten;
        d.resize((d.size() + 3U) / 4U * 4U, 0.0F);
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = static_cast<UINT>(d.size() * 4U);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        D3D11_SUBRESOURCE_DATA sd{d.data(), 0, 0};
        ID3D11Buffer* b = nullptr;
        dev->CreateBuffer(&bd, &sd, &b);
        ctx->CSSetConstantBuffers(p.slot, 1, &b);
        kb.push_back(b);
    }
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = static_cast<UINT>(ein.size() * 4U);
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bd.StructureByteStride = 16;
    D3D11_SUBRESOURCE_DATA sd{ein.data(), 0, 0};
    ID3D11Buffer* bEin = nullptr;
    dev->CreateBuffer(&bd, &sd, &bEin);
    D3D11_SHADER_RESOURCE_VIEW_DESC sv{};
    sv.Format = DXGI_FORMAT_UNKNOWN;
    sv.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    sv.Buffer.NumElements = static_cast<UINT>(ein.size() / 4U);
    ID3D11ShaderResourceView* srv = nullptr;
    dev->CreateShaderResourceView(bEin, &sv, &srv);
    D3D11_BUFFER_DESC ba{};
    ba.ByteWidth = proben * 32U;
    ba.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    ba.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    ba.StructureByteStride = 16;
    ID3D11Buffer* bAus = nullptr;
    dev->CreateBuffer(&ba, nullptr, &bAus);
    D3D11_UNORDERED_ACCESS_VIEW_DESC uv{};
    uv.Format = DXGI_FORMAT_UNKNOWN;
    uv.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uv.Buffer.NumElements = proben * 2U;
    ID3D11UnorderedAccessView* uav = nullptr;
    dev->CreateUnorderedAccessView(bAus, &uv, &uav);
    D3D11_BUFFER_DESC bl = ba;
    bl.BindFlags = 0;
    bl.MiscFlags = 0;
    bl.Usage = D3D11_USAGE_STAGING;
    bl.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Buffer* bLesen = nullptr;
    dev->CreateBuffer(&bl, nullptr, &bLesen);
    ctx->CSSetShader(cs, nullptr, 0);
    ctx->CSSetShaderResources(0, 1, &srv);
    ctx->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
    ctx->Dispatch((proben + 63U) / 64U, 1, 1);
    ctx->CopyResource(bLesen, bAus);
    D3D11_MAPPED_SUBRESOURCE map{};
    aus.assign(static_cast<std::size_t>(proben) * 8U, 0U);
    if (SUCCEEDED(ctx->Map(bLesen, 0, D3D11_MAP_READ, 0, &map))) {
        std::memcpy(aus.data(), map.pData, aus.size() * 4U);
        ctx->Unmap(bLesen, 0);
    }
    frei(bLesen);
    frei(uav);
    frei(bAus);
    frei(srv);
    frei(bEin);
    for (ID3D11Buffer*& b : kb) { frei(b); }
    frei(cs);
    frei(ctx);
    frei(dev);
    return true;
}

// Die Kerne: je Probe zwei float4 Eingabe (pos.xyz, s; normal.xyz, t), zwei float4 Ausgabe (verformte Ecke, Normale bzw. 0)
const char* const kKernEcke = R"(
StructuredBuffer<float4> gEin : register(t0);
RWStructuredBuffer<float4> gAus : register(u0);
cbuffer Werte : register(b0) { float4 gD0; float4 gD1; float4 gD2; float4 gD3; };
[numthreads(64, 1, 1)]
void kern(uint3 id : SV_DispatchThreadID) {
    float4 a = gEin[id.x * 2];
    float4 b = gEin[id.x * 2 + 1];
    float3 n = b.xyz;
    float3 p = verformeEcke(a.xyz, n, float2(a.w, b.w), gD0, gD1, gD2, gD3, 0, 3);
    gAus[id.x * 2] = float4(p, 1.0);
    gAus[id.x * 2 + 1] = float4(n, 0.0);
}
)";

// Die Kamera: Einheitsmatrix in JeBild (b0, row_major gViewProj vorn); gZeit (19) fuer Shader, die die Zeit dort lesen; die Achsen
// gVorn (28), gRechts (32), gHoch (36) wie zeichneKarte sie legt (viewParms.ori.axis[0], -axis[1], axis[2])
std::vector<float> jeBild(float zeit, const float* achsen = nullptr) {
    std::vector<float> b(64, 0.0F);
    for (int i = 0; i < 4; ++i) { b[static_cast<std::size_t>(i * 5)] = 1.0F; }
    b[19] = zeit;
    if (achsen != nullptr) {
        for (int k = 0; k < 3; ++k) {
            b[static_cast<std::size_t>(28 + k)] = achsen[k];
            b[static_cast<std::size_t>(32 + k)] = -achsen[3 + k];
            b[static_cast<std::size_t>(36 + k)] = achsen[6 + k];
        }
    }
    return b;
}

// Die Figurenstufe (b3): deformVertexes ab kStufeDefAt wie deformWerte (gpumap_win32.cpp), Lagen aus gpushader.h (P60)
std::vector<float> jeStufe(const Fall& f, float zeit) {
    std::vector<float> sd(kStufeFloats, 0.0F);
    const Verformung v = verformung(f.info.deform2, zeit);
    if (v.an) { std::memcpy(sd.data() + kStufeDefAt, v.d, sizeof v.d); }
    return sd;
}

bool gleichBits(float a, std::uint32_t b) {
    std::uint32_t x = 0;
    std::memcpy(&x, &a, 4);
    return x == b;
}

}  // namespace

int main() {
    std::printf("defgpu\n");
    spielInit();
    std::mt19937 zz(20261108U);
    const auto zahl = [&](double a, double b) { return static_cast<float>(a + (b - a) * (static_cast<double>(zz()) / 4294967296.0)); };

    // Die Faelle als Shadertext (parseShaderScript): je Art, die Folge (normal vor wave), DEF1 P44 zwei Eintraege derselben Art
    const char* const kFaelle[][2] = {
        {"wave sin", "wave 100 sin 0.5 6 0.2 0.7"},
        {"wave triangle", "wave 64 triangle 0 4 0.1 1.3"},
        {"wave square", "wave 180 square 1 3 0 0.5"},
        {"wave sawtooth", "wave 30 sawtooth -1 5 0.3 2"},
        {"wave inversesawtooth", "wave 120 inversesawtooth 0 2.5 0.7 0.25"},
        {"wave sin frequency 0", "wave 100 sin 1 3 0.37 0"},
        {"wave noise frequency 0", "wave 100 noise 0.5 4 0.6 0"},
        {"bulge", "bulge 0.35 4 1.7"},
        {"bulge ohne width und speed", "bulge 0 3 0"},
        {"move", "move 3 -2 7.5 sin 0 1 0.15 0.8"},
        {"normal, dann wave", "normal 0.4 1.2\ndeformVertexes wave 90 sin 2 5 0.1 0.6"},
        {"wave, bulge, move", "wave 50 triangle 0 3 0 0.9\ndeformVertexes bulge 0.2 2 3\ndeformVertexes move 0 0 4 square 0.5 1 0 0.3"},
        {"P44 zwei wave", "wave 100 sin 0.5 6 0.2 0.7\ndeformVertexes wave 37 triangle -1 2 0.4 1.9"},
        {"P44 bulge, wave, bulge", "bulge 0.35 4 1.7\ndeformVertexes wave 64 square 0 2 0.1 0.6\ndeformVertexes bulge 1.5 -2 0.4"},
        {"P44 normal, normal, wave", "normal 0.4 1.2\ndeformVertexes normal 0.2 0.3\ndeformVertexes wave 90 sin 2 5 0.1 0.6"},
        {"P44 zwei move", "move 3 -2 7.5 sin 0 1 0.15 0.8\ndeformVertexes move 0 1 -2 sawtooth 0 2 0.5 0.1"},
    };
    std::vector<Fall> faelle;
    for (const auto& k : kFaelle) {
        ShaderMap sm;
        parseShaderScript(std::string("textures/defgpu/x\n{\ndeformVertexes ") + k[1] + "\n{\nmap x\n}\n}\n", sm);
        faelle.push_back(Fall{k[0], sm["textures/defgpu/x"]});
    }
    erwarte("P44: zwei Eintraege derselben Art bleiben beide (anzahl 2, zwei Wellen)",
            faelle[12].info.deform2.anzahl == 2 && faelle[12].info.deform2.eintraege[1].welle.func == WaveFunc::Triangle &&
                faelle[12].info.deform2.eintraege[0].welle.func == WaveFunc::Sin);

    for (const Fall& f : faelle) {
        // Ecken in Karten-Groessen, Normalen der Laenge 1, st beliebig; Zeiten in ganzen ms (auch nach einer Stunde)
        const int ms = static_cast<int>(zz() % 3600000U);
        const auto zeit = static_cast<float>(ms * 0.001);
        const UINT proben = 8192;
        std::vector<float> ein;
        std::vector<float> soll;
        for (UINT i = 0; i < proben; ++i) {
            float xyz[3] = {zahl(-8000.0, 8000.0), zahl(-8000.0, 8000.0), zahl(-2000.0, 2000.0)};
            float nr[3] = {zahl(-1.0, 1.0), zahl(-1.0, 1.0), zahl(-1.0, 1.0)};
            const float l = std::sqrt(nr[0] * nr[0] + nr[1] * nr[1] + nr[2] * nr[2]);
            for (float& v : nr) { v /= (l > 0.0F ? l : 1.0F); }
            const float s = zahl(-4.0, 4.0);
            const float t = zahl(-4.0, 4.0);
            ein.insert(ein.end(), {xyz[0], xyz[1], xyz[2], s, nr[0], nr[1], nr[2], t});
            spielVerformen(f.info.deform2, ms, xyz, nr, s);
            soll.insert(soll.end(), {xyz[0], xyz[1], xyz[2], 1.0F, nr[0], nr[1], nr[2], 0.0F});
        }
        // Karte: der echte Vertex-Shader mit den Merkmalen aus gpu::pipelineFor und den Konstanten aus packeKonstanten
        const BatchState bs = stapel(f.info);
        std::vector<float> konst(kKonstFloats, 0.0F);
        packeKonstanten(bs, konst.data(), zeit);
        const std::uint32_t merkmale = pipelineFor(bs, false).features & (kDeform | kDeformNormal | kBulge | kDeformMove);
        struct Weg {
            const char* name;
            std::string quelle;
            std::vector<Puffer> cbs;
            bool normale;
            int vs;   // 0 Compute-Kern, 1 Vertex-Shader der Karte, 2 der Figur
        };
        std::vector<float> figur(1568, 0.0F);
        for (int r = 0; r < 3; ++r) { figur[static_cast<std::size_t>(r * 5)] = 1.0F; }        // gKnochen[0..2]: Einheit
        for (int r = 0; r < 4; ++r) { figur[1536U + static_cast<std::size_t>(r * 5)] = 1.0F; }   // gFigurWelt: Einheit
        std::vector<float> werte(16, 0.0F);
        {
            const Verformung v = verformung(f.info.deform2, zeit);
            std::memcpy(werte.data(), v.d, sizeof v.d);
        }
        const Weg wege[] = {
            {"Karte", vertexShaderHlsl(merkmale, 0), {{0, jeBild(zeit)}, {1, konst}}, false, 1},
            {"Figur", figurVertexShaderHlsl(0), {{0, jeBild(zeit)}, {2, figur}, {3, jeStufe(f, zeit)}}, false, 2},
            {"verformeEcke", verformHlsl() + kKernEcke, {{0, werte}}, true, 0},
        };
        for (const Weg& w : wege) {
            for (const D3D_DRIVER_TYPE art : {D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_HARDWARE}) {
                const char* wer = art == D3D_DRIVER_TYPE_WARP ? "WARP" : "Grafikkarte";
                std::vector<std::uint32_t> aus;
                std::string name;
                std::string dis;
                UINT je = 8U;   // Werte je Ecke in aus
                bool lief = false;
                if (w.vs == 0) {
                    lief = laufen(art, w.quelle, w.cbs, ein, proben, aus, name);
                } else {
                    je = 6U;
                    std::vector<vsstrom::Ecke> ecken(proben);
                    for (UINT i = 0; i < proben; ++i) {
                        vsstrom::Ecke& e = ecken[i];
                        const float* q = &ein[i * 8U];
                        e = vsstrom::Ecke{{q[0], q[1], q[2]}, {q[4], q[5], q[6]}, {q[3], q[7]}, {0.0F, 0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F, 0.0F}};
                        if (w.vs == 1) {
                            e.a[2] = e.a[3] = e.b[0] = e.b[1] = 1.0F;   // farbe
                        } else {
                            e.b[0] = 1.0F;   // ein Knochen, Gewicht 1
                        }
                    }
                    std::vector<vsstrom::Puffer> cbs;
                    for (const Puffer& p : w.cbs) { cbs.push_back({p.slot, p.daten}); }
                    lief = vsstrom::laufen(art, w.quelle, "vs_5_0", 0, w.vs == 1 ? vsstrom::Eingabe::Karte : vsstrom::Eingabe::Figur, cbs, ecken, aus, name, &dis);
                }
                if (!lief) {
                    erwarte(std::string(w.name) + " " + wer + ": Geraet nicht da oder Shader nicht uebersetzt", art == D3D_DRIVER_TYPE_HARDWARE && name.empty());
                    continue;
                }
                if (w.vs != 0 && art == D3D_DRIVER_TYPE_WARP) {
                    const std::string u = vsstrom::umstellbar(dis, "vs_5_0");
                    erwarte(std::string("P35 Bytecode ") + w.name + " vs_5_0 (" + f.was + "): precise je Anweisung" + (u.empty() ? "" : " - " + u),
                            u.empty());
                }
                int falsch = 0;
                for (UINT i = 0; i < proben; ++i) {
                    bool ok = true;
                    for (UINT k = 0; k < 3U; ++k) { ok = ok && gleichBits(soll[i * 8U + k], aus[i * je + k]); }
                    if (w.normale) {
                        for (int k = 4; k < 7; ++k) { ok = ok && gleichBits(soll[i * 8U + static_cast<UINT>(k)], aus[i * 8U + static_cast<UINT>(k)]); }
                    }
                    if (!ok) {
                        if (falsch < 2) {
                            float g[3];
                            std::memcpy(g, &aus[i * je], 12);
                            std::printf("        %s %s %s t=%d ms: GPU %.9g %.9g %.9g, Spiel %.9g %.9g %.9g\n", w.name, wer, f.was, ms, static_cast<double>(g[0]),
                                        static_cast<double>(g[1]), static_cast<double>(g[2]), static_cast<double>(soll[i * 8U]),
                                        static_cast<double>(soll[i * 8U + 1U]), static_cast<double>(soll[i * 8U + 2U]));
                        }
                        ++falsch;
                    }
                }
                char z[300];
                std::snprintf(z, sizeof z, "%s %s (%s): %s bitgleich zu RB_DeformTessGeometry: %d von %u Ecken abweichend", w.name, wer, name.c_str(), f.was,
                              falsch, proben);
                erwarte(z, falsch == 0);
            }
        }
    }
    // --- REST3: Shader mit autosprite/autosprite2 rechnet die CPU wie das Spiel (gpu::verformeTess) -------------------------------
    //
    // Gegen RB_DeformTessGeometry mit AutospriteDeform/Autosprite2Deform und RB_AddQuadStampExt (spieldeform.h, abgeschrieben): Ort,
    // Normale, st, Lightmap-st, Farbe und Indizes Bit fuer Bit, an der Welt, an einer gedrehten und skalierten Entity
    // (GlobalVectorToLocal wie zeichneMover, Normale des Plaettchens aus der Welt) und im Spiegel. Danach reicht der echte Vertex-Shader
    // der Karte die Ecken unveraendert durch (WARP und Grafikkarte). Bis REST3 drehte der Vertex-Shader (Rotbeleg
    // Y:\werkzeuge\r3\rot_p123.txt: wave, bulge oder ein zweites autosprite vor autosprite, autosprite2 mit wave, autosprite2 allein
    // 1895/1908 von 8192 Ecken, lange Kanten 7676/7684, Mover mit wave hinter autosprite, Spiegel - je WARP/Intel UHD abweichend).
    {
        float achsen[3][3];
        {
            const float gier = zahl(0.0, 6.2831853);
            const float nick = zahl(-1.2, 1.2);
            const float sy = std::sin(gier);
            const float cy = std::cos(gier);
            const float sp = std::sin(nick);
            const float cp = std::cos(nick);
            const float a[3][3] = {{cp * cy, cp * sy, -sp}, {-sy, cy, 0.0F}, {sp * cy, sp * sy, cp}};   // vorn, links (axis[1]), hoch
            std::memcpy(achsen, a, sizeof a);
        }
        float ent[9];   // eine gedrehte und skalierte Entity (Zeile k = backEnd.ori.axis[k], nonNormalizedAxes)
        {
            const float g = zahl(0.3, 2.9);
            const float c = std::cos(g);
            const float sn = std::sin(g);
            const float e[9] = {1.5F * c, 1.5F * sn, 0.0F, -sn, c, 0.0F, 0.0F, 0.0F, 0.75F};
            std::memcpy(ent, e, sizeof e);
        }
        const auto lese = [](const char* text) {
            ShaderMap sm;
            parseShaderScript(std::string("textures/defgpu/s\n{\ndeformVertexes ") + text + "\n{\nmap x\n}\n}\n", sm);
            return sm["textures/defgpu/s"];
        };
        struct SpriteFall {
            const char* was;
            const char* text;
            bool mover;
            bool spiegel;
            double weite;   // halbe Kantenlaenge der Vierecke; 600: Kanten ueber sqrt(WORLD_SIZE) = 362
        };
        const SpriteFall faelleS[] = {
            {"autosprite", "autosprite", false, false, 64.0},
            {"autosprite, wave, bulge", "autosprite\ndeformVertexes wave 64 sin 1 4 0.2 0.9\ndeformVertexes bulge 0.5 3 1.3", false, false, 64.0},
            {"autosprite, normal, wave", "autosprite\ndeformVertexes normal 0.3 1.1\ndeformVertexes wave 80 triangle 0 3 0.3 0.5", false, false, 64.0},
            {"move, autosprite", "move 3 -2 7.5 sin 0 1 0.15 0.8\ndeformVertexes autosprite", false, false, 64.0},
            {"wave, autosprite", "wave 64 sin 1 4 0.2 0.9\ndeformVertexes autosprite", false, false, 64.0},
            {"bulge, autosprite", "bulge 0.5 3 1.3\ndeformVertexes autosprite", false, false, 64.0},
            {"autosprite, autosprite", "autosprite\ndeformVertexes autosprite", false, false, 64.0},
            {"autosprite2", "autosprite2", false, false, 64.0},
            {"autosprite2, lange Kanten", "autosprite2", false, false, 600.0},
            {"autosprite2, wave", "autosprite2\ndeformVertexes wave 80 triangle 0 3 0.3 0.5", false, false, 64.0},
            {"bulge, autosprite2, normal", "bulge 0.5 3 1.3\ndeformVertexes autosprite2\ndeformVertexes normal 0.3 1.1", false, false, 64.0},
            {"autosprite2, move", "autosprite2\ndeformVertexes move 0 3 -1 sin 0.5 2 0.1 0.7", false, false, 64.0},
            {"Mover: autosprite, wave", "autosprite\ndeformVertexes wave 64 sin 1 4 0.2 0.9", true, false, 64.0},
            {"Mover: autosprite2", "autosprite2", true, false, 64.0},
            {"Spiegel: autosprite", "autosprite", false, true, 64.0},
        };
        const UINT vierecke = 2048;
        for (const SpriteFall& sf : faelleS) {
            BspMesh netz;
            for (UINT q = 0; q < vierecke; ++q) {
                const float mitte[3] = {zahl(-8000.0, 8000.0), zahl(-8000.0, 8000.0), zahl(-2000.0, 2000.0)};
                for (int w = 0; w < 4; ++w) {
                    BspVertex v;
                    for (int k = 0; k < 3; ++k) {
                        v.xyz[k] = mitte[k] + zahl(-sf.weite, sf.weite);
                        v.normal[k] = zahl(-1.0, 1.0);
                    }
                    v.st[0] = zahl(-4.0, 4.0);
                    v.st[1] = zahl(-4.0, 4.0);
                    v.lightmap[0] = zahl(0.0, 1.0);
                    v.lightmap[1] = zahl(0.0, 1.0);
                    for (int k = 0; k < 4; ++k) { v.colour[k] = static_cast<std::uint8_t>(zz() & 255U); }
                    netz.verts.push_back(v);
                }
                const std::uint32_t b0 = q * 4U;
                // q3map2 schreibt die Vierecke verschieden (kor1: 3,0,2 / 2,0,1) - Autosprite2Deform haengt an der Indexfolge
                if ((q & 1U) == 0U) {
                    for (const std::uint32_t i : {b0, b0 + 1U, b0 + 2U, b0, b0 + 2U, b0 + 3U}) { netz.indexes.push_back(i); }
                } else {
                    for (const std::uint32_t i : {b0 + 3U, b0, b0 + 2U, b0 + 2U, b0, b0 + 1U}) { netz.indexes.push_back(i); }
                }
            }
            const ShaderInfo info = lese(sf.text);
            const DeformZusatz& z = info.deform2;
            const int ms = static_cast<int>(zz() % 3600000U);
            const auto zeit = static_cast<float>(ms * 0.001);
            // Das Spiel
            SpielTess t;
            for (const BspVertex& v : netz.verts) {
                t.xyz.push_back({v.xyz[0], v.xyz[1], v.xyz[2]});
                t.normal.push_back({v.normal[0], v.normal[1], v.normal[2]});
                t.st.push_back({v.st[0], v.st[1]});
                t.lm.push_back({v.lightmap[0], v.lightmap[1]});
                t.farbe.push_back({v.colour[0], v.colour[1], v.colour[2], v.colour[3]});
            }
            t.idx = netz.indexes;
            SpielBlick blick{};
            std::memcpy(blick.axis, achsen, sizeof achsen);
            blick.ent = sf.mover ? ent : nullptr;
            blick.isMirror = sf.spiegel;
            spielDeformTess(z, ms, blick, t);
            // Die Vorschau: verformeTess mit der Sicht wie zeichneKarte (links = -gRechts) und zeichneMover (die Achsen mal den Spalten
            // der Stellung), vornWelt aus der Welt
            SpriteSicht sicht;
            for (int k = 0; k < 3; ++k) {
                const auto lokal = [&](int a) {
                    return sf.mover ? achsen[a][0] * ent[3 * k] + achsen[a][1] * ent[3 * k + 1] + achsen[a][2] * ent[3 * k + 2] : achsen[a][k];
                };
                sicht.vorn[k] = lokal(0);
                sicht.links[k] = lokal(1);
                sicht.hoch[k] = lokal(2);
                sicht.vornWelt[k] = achsen[0][k];
            }
            sicht.spiegel = sf.spiegel;
            std::vector<TessEcke> te(netz.verts.size());
            for (std::size_t i = 0; i < te.size(); ++i) {
                const BspVertex& v = netz.verts[i];
                std::copy(v.xyz, v.xyz + 3, te[i].xyz);
                std::copy(v.normal, v.normal + 3, te[i].normal);
                std::copy(v.st, v.st + 2, te[i].st);
                std::copy(v.lightmap, v.lightmap + 2, te[i].lm);
                std::copy(v.colour, v.colour + 4, te[i].farbe);
            }
            std::vector<std::uint32_t> idx = netz.indexes;
            const bool lief = verformeTess(z, zeit, sicht, te, idx);
            int falsch = 0;
            for (std::size_t i = 0; lief && i < te.size() && i < t.xyz.size(); ++i) {
                const bool ok = std::memcmp(te[i].xyz, t.xyz[i].data(), 12) == 0 && std::memcmp(te[i].normal, t.normal[i].data(), 12) == 0 &&
                                std::memcmp(te[i].st, t.st[i].data(), 8) == 0 && std::memcmp(te[i].lm, t.lm[i].data(), 8) == 0 &&
                                std::memcmp(te[i].farbe, t.farbe[i].data(), 4) == 0;
                if (!ok) {
                    if (falsch < 2) {
                        std::printf("        %s: Ecke %zu Vorschau %.9g %.9g %.9g, Spiel %.9g %.9g %.9g\n", sf.was, i, static_cast<double>(te[i].xyz[0]),
                                    static_cast<double>(te[i].xyz[1]), static_cast<double>(te[i].xyz[2]), static_cast<double>(t.xyz[i][0]),
                                    static_cast<double>(t.xyz[i][1]), static_cast<double>(t.xyz[i][2]));
                    }
                    ++falsch;
                }
            }
            char zl[300];
            std::snprintf(zl, sizeof zl, "REST3 %s: verformeTess zu RB_DeformTessGeometry bitgleich (Ort, Normale, st, Lightmap-st, Farbe): %d von %zu Ecken abweichend",
                          sf.was, falsch, te.size());
            erwarte(zl, lief && te.size() == t.xyz.size() && falsch == 0);
            erwarte(std::string("REST3 ") + sf.was + ": Indizes wie im Spiel", idx == t.idx);
            // Der Vertex-Shader der Karte mit den Merkmalen des Stapels reicht die Ecken durch
            BatchState bs{};
            bs.deformWave = info.deformWave;
            bs.deformSpread = info.deformSpread;
            bs.deform2 = info.deform2;
            bs.autosprite = info.autosprite;
            std::vector<float> konst(kKonstFloats, 0.0F);
            packeKonstanten(bs, konst.data(), zeit);
            const std::uint32_t merkmale = pipelineFor(bs, false).features & (kDeform | kDeformNormal | kBulge | kDeformMove | kAutosprite);
            std::vector<vsstrom::Ecke> ein(te.size());
            for (std::size_t i = 0; i < te.size(); ++i) {
                ein[i] = vsstrom::Ecke{{te[i].xyz[0], te[i].xyz[1], te[i].xyz[2]}, {te[i].normal[0], te[i].normal[1], te[i].normal[2]},
                                       {te[i].st[0], te[i].st[1]}, {te[i].lm[0], te[i].lm[1], 1.0F, 1.0F}, {1.0F, 1.0F, 0.0F, 0.0F}};
            }
            const float vsAchsen[9] = {sicht.vorn[0], sicht.vorn[1], sicht.vorn[2], sicht.links[0], sicht.links[1], sicht.links[2],
                                       sicht.hoch[0], sicht.hoch[1], sicht.hoch[2]};
            for (const D3D_DRIVER_TYPE art : {D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_HARDWARE}) {
                const char* wer = art == D3D_DRIVER_TYPE_WARP ? "WARP" : "Grafikkarte";
                std::vector<std::uint32_t> aus;
                std::string name;
                if (!vsstrom::laufen(art, vertexShaderHlsl(merkmale, 0), "vs_5_0", 0, vsstrom::Eingabe::Karte, {{0, jeBild(zeit, vsAchsen)}, {1, konst}},
                                     ein, aus, name, nullptr)) {
                    erwarte(std::string("REST3 ") + sf.was + " " + wer + ": Geraet nicht da oder Shader nicht uebersetzt",
                            art == D3D_DRIVER_TYPE_HARDWARE && name.empty());
                    continue;
                }
                int anders = 0;
                for (std::size_t i = 0; i < te.size(); ++i) {
                    if (std::memcmp(&aus[i * 6], te[i].xyz, 12) != 0) { ++anders; }
                }
                std::snprintf(zl, sizeof zl, "REST3 Karte %s (%s): %s - der Vertex-Shader reicht die Ecken der CPU durch: %d von %zu Ecken veraendert",
                              wer, name.c_str(), sf.was, anders, te.size());
                erwarte(zl, anders == 0);
            }
        }
    }
    // --- FIX5 Tempo: die tcMod-Kette im Vertex-Shader der Karte, nur mit den Schritten der Stufe und turb nur mit kTurb -----------
    {
        TexMod turb;
        turb.kind = TexModKind::Turb;
        turb.wave.func = WaveFunc::Sin;
        turb.wave.amplitude = 0.3F;
        turb.wave.phase = 0.1F;
        turb.wave.frequency = 0.5F;
        TexMod skal;
        skal.kind = TexModKind::Scale;
        skal.a[0] = 2.0F;
        skal.a[1] = 3.0F;
        TexMod dreh;
        dreh.kind = TexModKind::Rotate;
        dreh.a[0] = 30.0F;
        TexMod rollen;
        rollen.kind = TexModKind::Scroll;
        rollen.a[0] = 0.25F;
        rollen.a[1] = 0.5F;
        TexMod trans;
        trans.kind = TexModKind::Transform;
        const float tw[6] = {0.7F, 0.2F, -0.3F, 1.1F, 0.05F, 0.4F};
        for (int j = 0; j < 6; ++j) { trans.a[j] = tw[j]; }
        struct Kette {
            const char* was;
            TexMod m[4];
            int n;
        };
        const Kette ketten[] = {{"scroll", {rollen, {}, {}, {}}, 1},
                                {"rotate 30, scale 2 3", {dreh, skal, {}, {}}, 2},
                                {"scale, scroll, rotate, transform", {skal, rollen, dreh, trans}, 4},
                                {"turb", {turb, {}, {}, {}}, 1},
                                {"scroll, turb, transform", {rollen, turb, trans, {}}, 3}};
        for (const Kette& k : ketten) {
            const UINT proben = 8192;
            const auto zeit = static_cast<float>(static_cast<int>(zz() % 3600000U) * 0.001);
            BatchState bs{};
            bs.numTexMods = k.n;
            for (int j = 0; j < k.n; ++j) { bs.texMods[j] = k.m[j]; }
            std::vector<float> konst(kKonstFloats, 0.0F);
            packeKonstanten(bs, konst.data(), zeit);
            std::vector<float> ein;
            std::vector<float> soll;
            for (UINT i = 0; i < proben; ++i) {
                const float x = zahl(-18000.0, 18000.0);
                const float y = zahl(-9000.0, 9000.0);
                const float z = zahl(-4000.0, 4000.0);
                float st[2] = {zahl(-4.0, 4.0), zahl(-4.0, 4.0)};
                ein.insert(ein.end(), {x, y, z, st[0], 0.0F, 0.0F, 1.0F, st[1]});
                const float ort[2] = {x + z, y};
                spielSt(k.m, k.n, zeit, ort, st);
                soll.insert(soll.end(), {st[0], st[1], 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F});
            }
            const std::uint32_t merkmale = pipelineFor(bs, false).features & (kTexMod | kTurb);
            const std::string quelle = vertexShaderHlsl(merkmale, k.n);
            std::vector<vsstrom::Ecke> ecken(proben);
            for (UINT i = 0; i < proben; ++i) {
                const float* q = &ein[i * 8U];
                ecken[i] = vsstrom::Ecke{{q[0], q[1], q[2]}, {q[4], q[5], q[6]}, {q[3], q[7]}, {0.0F, 0.0F, 1.0F, 1.0F}, {1.0F, 1.0F, 0.0F, 0.0F}};
            }
            for (const D3D_DRIVER_TYPE art : {D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_HARDWARE}) {
                const char* wer = art == D3D_DRIVER_TYPE_WARP ? "WARP" : "Grafikkarte";
                std::vector<std::uint32_t> aus;
                std::string name;
                std::string dis;
                if (!vsstrom::laufen(art, quelle, "vs_5_0", 0, vsstrom::Eingabe::Karte, {{0, jeBild(zeit)}, {1, konst}}, ecken, aus, name, &dis)) {
                    erwarte(std::string("tcMod ") + wer + ": Geraet nicht da oder Shader nicht uebersetzt", art == D3D_DRIVER_TYPE_HARDWARE && name.empty());
                    continue;
                }
                int falsch = 0;
                for (UINT i = 0; i < proben; ++i) {
                    if (!gleichBits(soll[i * 8U], aus[i * 6U + 4U]) || !gleichBits(soll[i * 8U + 1U], aus[i * 6U + 5U])) { ++falsch; }
                }
                char z[300];
                std::snprintf(z, sizeof z, "Karte %s (%s): tcMod %s (%s%d Schritte) bitgleich zu ComputeTexCoords: %d von %u Ecken abweichend", wer,
                              name.c_str(), k.was, (merkmale & kTurb) != 0U ? "turb, " : "", k.n, falsch, proben);
                erwarte(z, falsch == 0);
            }
        }
    }

    if (fehler != 0) {
        std::printf("FEHLGESCHLAGEN (%d)\n", fehler);
        return 1;
    }
    std::printf("alle Gegenproben bestanden (0 Fehlschlaege)\n");
    return 0;
}
