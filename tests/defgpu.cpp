// Probe: rechnen die Vertex-Shader der Karte und der Figuren deformVertexes bitgleich wie rd-vanilla? (nur Windows/MSVC)
//
// FIX5: Die echten Vertex-Shader (gpu::vertexShaderHlsl mit den deform-Merkmalen, gpu::figurVertexShaderHlsl) laufen als
// Compute-Shader - ihr `main` aus einem Kern gerufen, die Kamera die Einheitsmatrix, also kommt die verformte Ecke unveraendert
// heraus. Auf WARP (der Software-Grafikkarte von Direct3D 11) und, wenn vorhanden, der Grafikkarte des Rechners. Die Werte packen
// die Wege des Programms (Karte: gpu::packeKonstanten; Figur: gpu::verformung wie deformWerte in gpumap_win32.cpp). Verglichen
// wird Bit fuer Bit mit RB_DeformTessGeometry (code/rd-vanilla/tr_shade_calc.cpp:553, MD9 gleich), hier abgeschrieben:
// RB_CalcDeformVertexes (:129), RB_CalcDeformNormals (:182), RB_CalcBulgeVertexes (:216), RB_CalcMoveVertexes (:263) mit WAVEVALUE,
// EvalWaveForm, R_InitFuncTables, R_NoiseGet4f (rand() der MSVC-Laufzeit) und VectorNormalizeFast/Q_rsqrt. normal wirkt auf die
// Ecke, wenn wave oder bulge danach kommen - so wird es hier sichtbar; die Normale selbst prueft der dritte Teil (verformeEcke).

#include "bhed/gpuconst.h"
#include "bhed/gpushader.h"
#include "bhed/gpustate.h"

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

// Die Arten der deformVertexes in der Folge eines Shaders (wie kFolgeWelle ... kFolgeWeg)
constexpr std::uint8_t kWelle = 1;
constexpr std::uint8_t kNormal = 2;
constexpr std::uint8_t kBeule = 3;
constexpr std::uint8_t kWeg = 4;
static_assert(kWelle == kFolgeWelle && kNormal == kFolgeNormal && kBeule == kFolgeBeule && kWeg == kFolgeWeg, "dieselben Arten");

// Ein Fall: der Shader (wave, Zusatz, die deformVertexes in Shader-Reihenfolge)
struct Fall {
    const char* was;
    Wave welle;
    float spread = 0.0F;
    DeformZusatz zusatz;
    std::uint8_t folge[4]{};
};

// RB_DeformTessGeometry an einer Ecke: xyz, normal, st[0]; time in ms, floatTime = time * 0.001 als float
void spielVerformen(const Fall& f, int time, float xyz[3], float normal[3], float s) {
    const auto floatTime = static_cast<float>(time * 0.001);
    for (const std::uint8_t art : f.folge) {
        if (art == kWelle) {
            float scale = 0.0F;
            if (f.welle.frequency == 0) {
                scale = evalWaveForm(f.welle, floatTime);
            } else {
                const float off = (xyz[0] + xyz[1] + xyz[2]) * f.spread;
                scale = waveValueSpiel(f.welle, f.welle.phase + off, floatTime);
            }
            for (int k = 0; k < 3; ++k) { xyz[k] += normal[k] * scale; }
        } else if (art == kNormal) {
            float scale = 0.98F;
            scale = rNoiseGet4f(xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, floatTime * f.zusatz.normal[1]);
            normal[0] += f.zusatz.normal[0] * scale;
            scale = 0.98F;
            scale = rNoiseGet4f(100 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, floatTime * f.zusatz.normal[1]);
            normal[1] += f.zusatz.normal[0] * scale;
            scale = 0.98F;
            scale = rNoiseGet4f(200 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale, floatTime * f.zusatz.normal[1]);
            normal[2] += f.zusatz.normal[0] * scale;
            const float il = qRsqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
            for (int k = 0; k < 3; ++k) { normal[k] *= il; }
        } else if (art == kBeule) {
            const float* b = f.zusatz.bulge;
            if (b[2] == 0.0F && b[0] == 0.0F) {
                for (int k = 0; k < 3; ++k) { xyz[k] += normal[k] * b[1]; }
            } else {
                const float now = static_cast<float>(time) * b[2] * 0.001F;
                const int off = qFtol(static_cast<float>(1024) / (3.14159265358979323846F * 2) * (s * b[0] + now));
                const float scale = sinTable[off & 1023] * b[1];
                for (int k = 0; k < 3; ++k) { xyz[k] += normal[k] * scale; }
            }
        } else if (art == kWeg) {
            const float scale = waveValueSpiel(f.zusatz.moveWave, f.zusatz.moveWave.phase, floatTime);
            for (int k = 0; k < 3; ++k) { xyz[k] += f.zusatz.move[k] * scale; }
        }
    }
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
const char* const kKernKarte = R"(
StructuredBuffer<float4> gEin : register(t0);
RWStructuredBuffer<float4> gAus : register(u0);
[numthreads(64, 1, 1)]
void kern(uint3 id : SV_DispatchThreadID) {
    float4 a = gEin[id.x * 2];
    float4 b = gEin[id.x * 2 + 1];
    VSIn e;
    e.pos = a.xyz;
    e.normal = b.xyz;
    e.uv = float2(a.w, b.w);
    e.uvLm = float2(0.0, 0.0);
    e.farbe = float4(1.0, 1.0, 1.0, 1.0);
    VSOut o = main(e);
    gAus[id.x * 2] = o.pos;
    gAus[id.x * 2 + 1] = float4(0.0, 0.0, 0.0, 0.0);
}
)";
const char* const kKernFigur = R"(
StructuredBuffer<float4> gEin : register(t0);
RWStructuredBuffer<float4> gAus : register(u0);
[numthreads(64, 1, 1)]
void kern(uint3 id : SV_DispatchThreadID) {
    float4 a = gEin[id.x * 2];
    float4 b = gEin[id.x * 2 + 1];
    VSIn e;
    e.pos = a.xyz;
    e.normal = b.xyz;
    e.uv = float2(a.w, b.w);
    e.bones = float4(0.0, 0.0, 0.0, 0.0);
    e.weights = float4(1.0, 0.0, 0.0, 0.0);
    VSOut o = main(e);
    gAus[id.x * 2] = o.pos;
    gAus[id.x * 2 + 1] = float4(0.0, 0.0, 0.0, 0.0);
}
)";
const char* const kKernEcke = R"(
StructuredBuffer<float4> gEin : register(t0);
RWStructuredBuffer<float4> gAus : register(u0);
cbuffer Werte : register(b0) { float4 gD0; float4 gD1; float4 gD2; float4 gD3; };
[numthreads(64, 1, 1)]
void kern(uint3 id : SV_DispatchThreadID) {
    float4 a = gEin[id.x * 2];
    float4 b = gEin[id.x * 2 + 1];
    float3 n = b.xyz;
    float3 p = verformeEcke(a.xyz, n, float2(a.w, b.w), gD0, gD1, gD2, gD3);
    gAus[id.x * 2] = float4(p, 1.0);
    gAus[id.x * 2 + 1] = float4(n, 0.0);
}
)";

// Die Kamera: Einheitsmatrix in JeBild (b0, row_major gViewProj vorn); gZeit (19) fuer Shader, die die Zeit dort lesen
std::vector<float> jeBild(float zeit) {
    std::vector<float> b(64, 0.0F);
    for (int i = 0; i < 4; ++i) { b[static_cast<std::size_t>(i * 5)] = 1.0F; }
    b[19] = zeit;
    return b;
}

// Die Figurenstufe (b3): deformVertexes ab 32 wie deformWerte (gpumap_win32.cpp)
std::vector<float> jeStufe(const Fall& f, float zeit) {
    std::vector<float> sd(68, 0.0F);
    const Verformung v = verformung(f.welle, f.spread, f.zusatz, zeit);
    if (v.an) { std::memcpy(sd.data() + 32, v.d, sizeof v.d); }
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

    const auto welle = [](WaveFunc f, float base, float amp, float phase, float freq) {
        Wave w;
        w.func = f;
        w.base = base;
        w.amplitude = amp;
        w.phase = phase;
        w.frequency = freq;
        return w;
    };
    std::vector<Fall> faelle;
    faelle.reserve(16);
    const auto fall = [&](const char* was, const Wave& w, float spread, std::initializer_list<std::uint8_t> folge) {
        Fall f;
        f.was = was;
        f.welle = w;
        f.spread = spread;
        int i = 0;
        for (const std::uint8_t a : folge) { f.folge[i++] = a; }
        std::copy(f.folge, f.folge + 4, f.zusatz.folge);
        faelle.push_back(f);
        return &faelle.back();
    };
    fall("wave sin", welle(WaveFunc::Sin, 0.5F, 6.0F, 0.2F, 0.7F), 1.0F / 100.0F, {kWelle});
    fall("wave triangle", welle(WaveFunc::Triangle, 0.0F, 4.0F, 0.1F, 1.3F), 1.0F / 64.0F, {kWelle});
    fall("wave square", welle(WaveFunc::Square, 1.0F, 3.0F, 0.0F, 0.5F), 1.0F / 180.0F, {kWelle});
    fall("wave sawtooth", welle(WaveFunc::Sawtooth, -1.0F, 5.0F, 0.3F, 2.0F), 1.0F / 30.0F, {kWelle});
    fall("wave inversesawtooth", welle(WaveFunc::InverseSawtooth, 0.0F, 2.5F, 0.7F, 0.25F), 1.0F / 120.0F, {kWelle});
    fall("wave sin frequency 0", welle(WaveFunc::Sin, 1.0F, 3.0F, 0.37F, 0.0F), 1.0F / 100.0F, {kWelle});
    fall("wave noise frequency 0", welle(WaveFunc::Noise, 0.5F, 4.0F, 0.6F, 0.0F), 1.0F / 100.0F, {kWelle});
    {
        Fall* f = fall("bulge", Wave{}, 0.0F, {kBeule});
        f->zusatz.bulge[0] = 0.35F;
        f->zusatz.bulge[1] = 4.0F;
        f->zusatz.bulge[2] = 1.7F;
    }
    {
        Fall* f = fall("bulge ohne width und speed", Wave{}, 0.0F, {kBeule});
        f->zusatz.bulge[1] = 3.0F;
    }
    {
        Fall* f = fall("move", Wave{}, 0.0F, {kWeg});
        f->zusatz.move[0] = 3.0F;
        f->zusatz.move[1] = -2.0F;
        f->zusatz.move[2] = 7.5F;
        f->zusatz.moveWave = welle(WaveFunc::Sin, 0.0F, 1.0F, 0.15F, 0.8F);
    }
    {
        Fall* f = fall("normal, dann wave", welle(WaveFunc::Sin, 2.0F, 5.0F, 0.1F, 0.6F), 1.0F / 90.0F, {kNormal, kWelle});
        f->zusatz.normal[0] = 0.4F;
        f->zusatz.normal[1] = 1.2F;
    }
    {
        Fall* f = fall("wave, bulge, move", welle(WaveFunc::Triangle, 0.0F, 3.0F, 0.0F, 0.9F), 1.0F / 50.0F,
                       {kWelle, kBeule, kWeg});
        f->zusatz.bulge[0] = 0.2F;
        f->zusatz.bulge[1] = 2.0F;
        f->zusatz.bulge[2] = 3.0F;
        f->zusatz.move[2] = 4.0F;
        f->zusatz.moveWave = welle(WaveFunc::Square, 0.5F, 1.0F, 0.0F, 0.3F);
    }

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
            spielVerformen(f, ms, xyz, nr, s);
            soll.insert(soll.end(), {xyz[0], xyz[1], xyz[2], 1.0F, nr[0], nr[1], nr[2], 0.0F});
        }
        // Karte: der echte Vertex-Shader mit den Merkmalen aus gpu::pipelineFor und den Konstanten aus packeKonstanten
        BatchState bs{};
        bs.deformWave = f.welle;
        bs.deformSpread = f.spread;
        bs.deform2 = f.zusatz;
        std::vector<float> konst(kKonstFloats, 0.0F);
        packeKonstanten(bs, konst.data(), zeit);
        const std::uint32_t merkmale = pipelineFor(bs, false).features & (kDeform | kDeformNormal | kBulge | kDeformMove);
        struct Weg {
            const char* name;
            std::string quelle;
            std::vector<Puffer> cbs;
            bool normale;
        };
        std::vector<float> figur(1568, 0.0F);
        for (int r = 0; r < 3; ++r) { figur[static_cast<std::size_t>(r * 5)] = 1.0F; }        // gKnochen[0..2]: Einheit
        for (int r = 0; r < 4; ++r) { figur[1536U + static_cast<std::size_t>(r * 5)] = 1.0F; }   // gFigurWelt: Einheit
        std::vector<float> werte(16, 0.0F);
        {
            const Verformung v = verformung(f.welle, f.spread, f.zusatz, zeit);
            std::memcpy(werte.data(), v.d, sizeof v.d);
        }
        const Weg wege[] = {
            {"Karte", vertexShaderHlsl(merkmale, 0) + kKernKarte, {{0, jeBild(zeit)}, {1, konst}}, false},
            {"Figur", figurVertexShaderHlsl(0) + kKernFigur, {{0, jeBild(zeit)}, {2, figur}, {3, jeStufe(f, zeit)}}, false},
            {"verformeEcke", verformHlsl() + kKernEcke, {{0, werte}}, true},
        };
        for (const Weg& w : wege) {
            for (const D3D_DRIVER_TYPE art : {D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_HARDWARE}) {
                const char* wer = art == D3D_DRIVER_TYPE_WARP ? "WARP" : "Grafikkarte";
                std::vector<std::uint32_t> aus;
                std::string name;
                if (!laufen(art, w.quelle, w.cbs, ein, proben, aus, name)) {
                    erwarte(std::string(w.name) + " " + wer + ": Geraet nicht da oder Shader nicht uebersetzt", art == D3D_DRIVER_TYPE_HARDWARE && name.empty());
                    continue;
                }
                int falsch = 0;
                for (UINT i = 0; i < proben; ++i) {
                    bool ok = true;
                    for (int k = 0; k < 3; ++k) { ok = ok && gleichBits(soll[i * 8U + static_cast<UINT>(k)], aus[i * 8U + static_cast<UINT>(k)]); }
                    if (w.normale) {
                        for (int k = 4; k < 7; ++k) { ok = ok && gleichBits(soll[i * 8U + static_cast<UINT>(k)], aus[i * 8U + static_cast<UINT>(k)]); }
                    }
                    if (!ok) {
                        if (falsch < 2) {
                            float g[3];
                            std::memcpy(g, &aus[i * 8U], 12);
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
    if (fehler != 0) {
        std::printf("FEHLGESCHLAGEN (%d)\n", fehler);
        return 1;
    }
    std::printf("alle Gegenproben bestanden (0 Fehlschlaege)\n");
    return 0;
}
