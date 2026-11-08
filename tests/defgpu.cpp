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

// --- Das Spiel ---------------------------------------------------------------------------------------------------------------
float sinTable[1024];
float triangleTable[1024];
float squareTable[1024];
float sawToothTable[1024];
float inverseSawToothTable[1024];
float noiseTable[256];
int noisePerm[256];
void spielInit() {
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
int qFtol(float f) { return (std::fabs(f) < 2147483648.0F) ? static_cast<int>(f) : (-2147483647 - 1); }
unsigned val(unsigned a) { return static_cast<unsigned>(noisePerm[a & 255U]); }
float noiseValue(int x, int y, int z, int t) {
    const auto u = [](int v) { return static_cast<unsigned>(v); };
    return noiseTable[val(u(x) + val(u(y) + val(u(z) + val(u(t)))))];
}
float lerp(float a, float b, float w) { return a * (1.0F - w) + b * w; }
float rNoiseGet4f(float x, float y, float z, float t) {
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
const float* tableForFunc(WaveFunc f) {
    switch (f) {
        case WaveFunc::Sin: return sinTable;
        case WaveFunc::Triangle: return triangleTable;
        case WaveFunc::Square: return squareTable;
        case WaveFunc::Sawtooth: return sawToothTable;
        default: return inverseSawToothTable;
    }
}
float waveValueSpiel(const Wave& w, float phase, float floatTime) {
    return w.base + tableForFunc(w.func)[qFtol((phase + floatTime * w.frequency) * 1024) & 1023] * w.amplitude;
}
float evalWaveForm(const Wave& w, float floatTime) {
    if (w.func == WaveFunc::Noise) { return (w.base + rNoiseGet4f(0, 0, 0, (floatTime + w.phase) * w.frequency) * w.amplitude); }
    return waveValueSpiel(w, w.phase, floatTime);
}
float qRsqrt(float number) {
    const float x2 = number * 0.5F;
    std::int32_t i = 0;
    std::memcpy(&i, &number, 4);
    i = 0x5f3759df - (i >> 1);
    float y = 0.0F;
    std::memcpy(&y, &i, 4);
    return y * (1.5F - (x2 * y * y));
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

// RB_DeformTessGeometry an einer Ecke, Eintraege von..bis-1 (autosprite/autosprite2 rechnet der Aufrufer): xyz, normal, st[0];
// time in ms, floatTime = time * 0.001 als float
void spielVerformen(const DeformZusatz& z, int time, float xyz[3], float normal[3], float s, int von = 0, int bis = kMaxShaderDeforms) {
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
    // --- DEF1 P44: autosprite/autosprite2 an ihrer Stelle der Folge (RB_DeformTessGeometry, tr_shade_calc.cpp:553) ---------------
    //
    // Die Ecken kommen wie im Programm aus baueAutosprites/baueAutosprites2 (gpudraw.cpp) in den echten Vertex-Shader der Karte. Das
    // Spiel: AutospriteDeform (:377) auf den vier Ecken, RB_AddQuadStampExt (tr_surface.cpp:78: Normale -axis[0], st 0..1), danach die
    // Eintraege hinter dem autosprite. "move, autosprite": die Mitte rechnet die Vorschau aus den unverschobenen Ecken (beim Hochladen),
    // das Spiel aus den verschobenen - gleich bis auf die Rundung (hoechstens 16 ulp der groessten Koordinate verlangt).
    {
        float achsen[9];
        {
            const float gier = zahl(0.0, 6.2831853);
            const float nick = zahl(-1.2, 1.2);
            const float sy = std::sin(gier);
            const float cy = std::cos(gier);
            const float sp = std::sin(nick);
            const float cp = std::cos(nick);
            const float a[9] = {cp * cy, cp * sy, -sp, -sy, cy, 0.0F, sp * cy, sp * sy, cp};   // vorn, links (axis[1]), hoch
            std::copy(a, a + 9, achsen);
        }
        const UINT vierecke = 2048;
        BspMesh netz;
        for (UINT q = 0; q < vierecke; ++q) {
            const float mitte[3] = {zahl(-8000.0, 8000.0), zahl(-8000.0, 8000.0), zahl(-2000.0, 2000.0)};
            for (int w = 0; w < 4; ++w) {
                BspVertex v;
                for (int k = 0; k < 3; ++k) {
                    v.xyz[k] = mitte[k] + zahl(-64.0, 64.0);
                    v.normal[k] = zahl(-1.0, 1.0);
                }
                v.st[0] = zahl(-4.0, 4.0);
                v.st[1] = zahl(-4.0, 4.0);
                netz.verts.push_back(v);
            }
            const std::uint32_t b0 = q * 4U;
            for (const std::uint32_t i : {b0, b0 + 1U, b0 + 2U, b0, b0 + 2U, b0 + 3U}) { netz.indexes.push_back(i); }
        }
        BspMesh::Batch stp;
        stp.numIndexes = static_cast<std::uint32_t>(netz.indexes.size());
        netz.batches.push_back(stp);
        std::vector<std::uint32_t> idx = netz.indexes;
        const std::vector<SpriteEcke> sprite = baueAutosprites(netz, {1}, idx);
        const std::vector<SpriteEcke2> sprite2 = baueAutosprites2(netz, {2});
        erwarte("P44: baueAutosprites/baueAutosprites2 liefern jede Ecke", sprite.size() == netz.verts.size() && sprite2.size() == netz.verts.size());
        const auto ecken = [&](bool zwei) {
            std::vector<vsstrom::Ecke> e(netz.verts.size());
            for (std::size_t i = 0; i < e.size(); ++i) {
                const BspVertex& v = netz.verts[i];
                e[i] = vsstrom::Ecke{{v.xyz[0], v.xyz[1], v.xyz[2]}, {v.normal[0], v.normal[1], v.normal[2]}, {v.st[0], v.st[1]},
                                     {0.0F, 0.0F, 1.0F, 1.0F}, {1.0F, 1.0F, 0.0F, 0.0F}};
            }
            if (zwei) {
                for (const SpriteEcke2& s : sprite2) {
                    std::copy(s.mitte, s.mitte + 3, e[s.ecke].pos);
                    std::copy(s.achse, s.achse + 3, e[s.ecke].normal);
                }
            } else {
                for (const SpriteEcke& s : sprite) {
                    std::copy(s.mitte, s.mitte + 3, e[s.ecke].pos);
                    e[s.ecke].normal[0] = s.links;
                    e[s.ecke].normal[1] = s.hoch;
                    e[s.ecke].normal[2] = s.radius;
                    std::copy(s.st, s.st + 2, e[s.ecke].uv);
                }
            }
            return e;
        };
        const auto lese = [](const char* text) {
            ShaderMap sm;
            parseShaderScript(std::string("textures/defgpu/s\n{\ndeformVertexes ") + text + "\n{\nmap x\n}\n}\n", sm);
            return sm["textures/defgpu/s"];
        };
        constexpr std::uint32_t kMaske = kDeform | kDeformNormal | kBulge | kDeformMove | kAutosprite | kAutosprite2;
        // 16 ulp der groessten Koordinate der Ecke (die Mitte rundet in der Groesse der Koordinaten, nicht der einzelnen Achse)
        const auto ulp = [](float a, std::uint32_t b, float gross) {
            float g = 0.0F;
            std::memcpy(&g, &b, 4);
            return std::fabs(a - g) <= 16.0F * (std::nextafter(gross, 1e30F) - gross);
        };
        struct SpriteFall {
            const char* was;
            const char* text;
            bool genau;
        };
        const SpriteFall faelleS[] = {
            {"autosprite", "autosprite", true},
            {"autosprite, wave, bulge", "autosprite\ndeformVertexes wave 64 sin 1 4 0.2 0.9\ndeformVertexes bulge 0.5 3 1.3", true},
            {"autosprite, normal, wave", "autosprite\ndeformVertexes normal 0.3 1.1\ndeformVertexes wave 80 triangle 0 3 0.3 0.5", true},
            {"move, autosprite", "move 3 -2 7.5 sin 0 1 0.15 0.8\ndeformVertexes autosprite", false},
        };
        for (const SpriteFall& sf : faelleS) {
            const ShaderInfo info = lese(sf.text);
            const DeformZusatz& z = info.deform2;
            int stelle = 0;
            while (stelle < z.anzahl && z.eintraege[stelle].art != kFolgeSprite) { ++stelle; }
            const int ms = static_cast<int>(zz() % 3600000U);
            const auto zeit = static_cast<float>(ms * 0.001);
            // Das Spiel
            std::vector<float> soll;
            for (UINT q = 0; q < vierecke; ++q) {
                float xyz[4][3];
                for (int w = 0; w < 4; ++w) {
                    BspVertex v = netz.verts[q * 4U + static_cast<UINT>(w)];
                    spielVerformen(z, ms, v.xyz, v.normal, v.st[0], 0, stelle);
                    std::copy(v.xyz, v.xyz + 3, xyz[w]);
                }
                float mid[3];
                for (int k = 0; k < 3; ++k) { mid[k] = 0.25F * (xyz[0][k] + xyz[1][k] + xyz[2][k] + xyz[3][k]); }
                const float d[3] = {xyz[0][0] - mid[0], xyz[0][1] - mid[1], xyz[0][2] - mid[2]};
                const float radius = static_cast<float>(std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2])) * 0.707F;
                float left[3];
                float up[3];
                for (int k = 0; k < 3; ++k) {
                    left[k] = achsen[3 + k] * radius;
                    up[k] = achsen[6 + k] * radius;
                }
                static const float kRicht[4][2] = {{1, 1}, {-1, 1}, {-1, -1}, {1, -1}};
                static const float kSt[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
                for (int w = 0; w < 4; ++w) {
                    float p[3];
                    float n[3];
                    for (int k = 0; k < 3; ++k) {
                        p[k] = (kRicht[w][0] > 0 ? mid[k] + left[k] : mid[k] - left[k]);
                        p[k] = (kRicht[w][1] > 0 ? p[k] + up[k] : p[k] - up[k]);
                        n[k] = -achsen[k];
                    }
                    spielVerformen(z, ms, p, n, kSt[w][0], stelle + 1, kMaxShaderDeforms);
                    soll.insert(soll.end(), {p[0], p[1], p[2]});
                }
            }
            // Die Vorschau: die Ecken in der Reihenfolge von baueAutosprites (je Viereck die erste Ecke zuerst)
            const BatchState bs = stapel(info);
            std::vector<float> konst(kKonstFloats, 0.0F);
            packeKonstanten(bs, konst.data(), zeit);
            const std::uint32_t merkmale = pipelineFor(bs, false).features & kMaske;
            for (const D3D_DRIVER_TYPE art : {D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_HARDWARE}) {
                const char* wer = art == D3D_DRIVER_TYPE_WARP ? "WARP" : "Grafikkarte";
                std::vector<std::uint32_t> aus;
                std::string name;
                std::string dis;
                if (!vsstrom::laufen(art, vertexShaderHlsl(merkmale, 0), "vs_5_0", 0, vsstrom::Eingabe::Karte,
                                     {{0, jeBild(zeit, achsen)}, {1, konst}}, ecken(false), aus, name, &dis)) {
                    erwarte(std::string("P44 ") + sf.was + " " + wer + ": Geraet nicht da oder Shader nicht uebersetzt",
                            art == D3D_DRIVER_TYPE_HARDWARE && name.empty());
                    continue;
                }
                int falsch = 0;
                for (std::size_t i = 0; i < netz.verts.size(); ++i) {
                    bool ok = true;
                    const float gross = std::max({std::fabs(soll[i * 3]), std::fabs(soll[i * 3 + 1]), std::fabs(soll[i * 3 + 2])});
                    for (std::size_t k = 0; k < 3; ++k) {
                        ok = ok && (sf.genau ? gleichBits(soll[i * 3 + k], aus[i * 6 + k]) : ulp(soll[i * 3 + k], aus[i * 6 + k], gross));
                    }
                    if (!ok) {
                        if (falsch < 2) {
                            float g[3];
                            std::memcpy(g, &aus[i * 6], 12);
                            std::printf("        P44 %s %s: GPU %.9g %.9g %.9g, Spiel %.9g %.9g %.9g\n", sf.was, wer, static_cast<double>(g[0]),
                                        static_cast<double>(g[1]), static_cast<double>(g[2]), static_cast<double>(soll[i * 3]),
                                        static_cast<double>(soll[i * 3 + 1]), static_cast<double>(soll[i * 3 + 2]));
                        }
                        ++falsch;
                    }
                }
                char zl[300];
                std::snprintf(zl, sizeof zl, "P44 Karte %s (%s): %s %s zu AutospriteDeform und RB_DeformTessGeometry: %d von %zu Ecken abweichend",
                              wer, name.c_str(), sf.was, sf.genau ? "bitgleich" : "bis 16 ulp gleich", falsch, netz.verts.size());
                erwarte(zl, falsch == 0);
            }
        }
        // autosprite2 und move dahinter: dieselben Ecken mit und ohne move - der Unterschied ist genau der Versatz des Spiels
        {
            const ShaderInfo info = lese("autosprite2\ndeformVertexes move 0 3 -1 sin 0.5 2 0.1 0.7");
            const ShaderInfo ohne = lese("autosprite2");
            const int ms = static_cast<int>(zz() % 3600000U);
            const auto zeit = static_cast<float>(ms * 0.001);
            float versatz[3] = {0.0F, 0.0F, 0.0F};
            float nullN[3] = {0.0F, 0.0F, 0.0F};
            spielVerformen(info.deform2, ms, versatz, nullN, 0.0F, 1, kMaxShaderDeforms);
            for (const D3D_DRIVER_TYPE art : {D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_HARDWARE}) {
                const char* wer = art == D3D_DRIVER_TYPE_WARP ? "WARP" : "Grafikkarte";
                std::vector<std::uint32_t> aus[2];
                std::string name;
                bool lief = true;
                for (int j = 0; j < 2 && lief; ++j) {
                    const BatchState bs = stapel(j == 0 ? info : ohne);
                    std::vector<float> konst(kKonstFloats, 0.0F);
                    packeKonstanten(bs, konst.data(), zeit);
                    lief = vsstrom::laufen(art, vertexShaderHlsl(pipelineFor(bs, false).features & kMaske, 0), "vs_5_0", 0, vsstrom::Eingabe::Karte,
                                           {{0, jeBild(zeit, achsen)}, {1, konst}}, ecken(true), aus[j], name, nullptr);
                }
                if (!lief) {
                    erwarte(std::string("P44 autosprite2, move ") + wer + ": Geraet nicht da oder Shader nicht uebersetzt",
                            art == D3D_DRIVER_TYPE_HARDWARE && name.empty());
                    continue;
                }
                int falsch = 0;
                for (std::size_t i = 0; i < netz.verts.size(); ++i) {
                    for (std::size_t k = 0; k < 3; ++k) {
                        float o = 0.0F;
                        std::memcpy(&o, &aus[1][i * 6 + k], 4);
                        if (!gleichBits(o + versatz[k], aus[0][i * 6 + k])) {
                            ++falsch;
                            break;
                        }
                    }
                }
                char zl[300];
                std::snprintf(zl, sizeof zl, "P44 Karte %s (%s): autosprite2, move - move nach Autosprite2Deform: %d von %zu Ecken abweichend", wer,
                              name.c_str(), falsch, netz.verts.size());
                erwarte(zl, falsch == 0);
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
