// Probe: rechnet der HLSL-Code der tcMod-Kette auf der Grafikkarte bitgleich wie rd-vanilla? (nur Windows/MSVC)
//
// FIX4 P33: tests/gpuconst.cpp prueft die Werte, die der Packer schreibt, und eine Nachbildung des Shaders auf der CPU. Hier laeuft
// der echte Text (gpu::tcModHlsl, derselbe in Karte, Figurenstufe und Echtzeit-Licht) als Compute-Shader: auf WARP (der
// Software-Grafikkarte von Direct3D 11, auf jedem Windows) und, wenn vorhanden, auf der Grafikkarte des Rechners. Die Eingaben
// kommen aus dem Packer (gpu::packeKonstanten); verglichen wird Bit fuer Bit mit rd-vanilla ComputeTexCoords
// (code/rd-vanilla/tr_shade.cpp:1725): RB_CalcTurbulentTexCoords (tr_shade_calc.cpp:936-951) mit der Tafel aus R_InitFuncTables
// (tr_init.cpp:1727), RB_CalcScaleTexCoords, RB_CalcScrollTexCoords, RB_CalcRotateTexCoords und RB_CalcTransformTexCoords.

#include "bhed/gpuconst.h"
#include "bhed/gpushader.h"
#include "vsstrom.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <windows.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

using namespace bhed;
using namespace bhed::gpu;

namespace {

int fehler = 0;

void erwarte(const char* was, bool ok) {
    std::printf("  %-4s  %s\n", ok ? "ok" : "FEHL", was);
    if (!ok) { ++fehler; }
}

template <class T>
void frei(T*& p) {
    if (p != nullptr) {
        p->Release();
        p = nullptr;
    }
}

// Das Spiel (wie tests/gpuconst.cpp): Tafel in float, turb-Index aus dem double-Ausdruck, jeder Schritt der Reihe nach in float
float sinTab(int i) {
    return std::sin(static_cast<float>(i & 1023) * 360.0F / static_cast<float>(1024 - 1) * 0.017453292519943295769236907684886F);
}
void spielSt(const TexMod* m, int n, float zeit, const float ort[2], float st[2]) {
    for (int k = 0; k < n; ++k) {
        const TexMod& t = m[k];
        const float s = st[0];
        const float u = st[1];
        switch (t.kind) {
            case TexModKind::Turb: {
                const float now = t.wave.phase + zeit * t.wave.frequency;
                st[0] = s + sinTab(static_cast<int>((ort[0] * 1.0 / 128 * 0.125 + now) * 1024)) * t.wave.amplitude;
                st[1] = u + sinTab(static_cast<int>((ort[1] * 1.0 / 128 * 0.125 + now) * 1024)) * t.wave.amplitude;
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
                const float sn = sinTab(index);
                const float cs = sinTab(index + 1024 / 4);
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

// Je Probe acht float4: (ort.x, ort.y, st.s, st.t), dann gTcM0, gTcO01, gTcM1, gTcM2, gTcM3, gTcO23, gTcArt
struct Eingabe {
    float v[32];
};

const char* const kKern = R"(
StructuredBuffer<float4> gEin : register(t0);
RWStructuredBuffer<uint2> gAus : register(u0);
[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint b = id.x * 8;
    float4 a = gEin[b];
    gAus[id.x] = asuint(tcKette(a.zw, gEin[b + 1], gEin[b + 2], gEin[b + 3], gEin[b + 4], gEin[b + 5], gEin[b + 6], gEin[b + 7], a.xy, true));
}
)";

// FIX5 P35: dieselbe Kette als Vertex-Shader mit den Profilen und Fahnen des Echtzeit-Lichts (karte_gpu_win32.cpp: vs_4_0 und
// vs_4_1/vs_5_0 mit D3DCOMPILE_OPTIMIZATION_LEVEL3) und der Vorschau (vs_4_0, Fahnen 0); die Werte je Ecke aus Buffer<float4> t0
const char* const kVsHuelle = R"(
Buffer<float4> gEin : register(t0);
struct VSIn { float3 pos : POSITION; };
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
VSOut main(VSIn e, uint id : SV_VertexID) {
    uint b = id * 8;
    float4 a = gEin[b];
    VSOut o;
    o.pos = float4(e.pos, 1.0);
    o.uv = tcKette(a.zw, gEin[b + 1], gEin[b + 2], gEin[b + 3], gEin[b + 4], gEin[b + 5], gEin[b + 6], gEin[b + 7], a.xy, true);
    return o;
}
)";
// ab Merkmalsstufe 11_0 (jede Grafikkarte, auf der das hier laeuft, und WARP) vs_5_0; vs_4_x nur auf aelteren Karten (unter 11_0) -
// dort gemessen auf WARP, auf der Grafikkarte nur als Hinweis (sie nimmt es nicht)
struct Profil {
    const char* profil;
    UINT fahnen;
    const char* wo;
    bool ab11;
};
const Profil kProfile[] = {{"vs_5_0", 0, "Vorschau", true},
                           {"vs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, "Echtzeit-Licht vs3/vs4", true},
                           {"vs_4_0", 0, "Vorschau unter 11_0", false},
                           {"vs_4_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, "Echtzeit-Licht vs3 unter 11_0", false},
                           {"vs_4_1", D3DCOMPILE_OPTIMIZATION_LEVEL3, "Echtzeit-Licht vs4 unter 11_0", false}};

// Den Kern auf einem Geraet laufen lassen; false: Geraet nicht da (name leer) oder Shader nicht uebersetzt
bool laufen(D3D_DRIVER_TYPE art, const std::vector<Eingabe>& ein, std::vector<std::uint32_t>& aus, std::string& name) {
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

    const std::string quelle = tcModHlsl() + kKern;
    ID3DBlob* blob = nullptr;
    ID3DBlob* err = nullptr;
    if (FAILED(D3DCompile(quelle.data(), quelle.size(), "turbgpu", nullptr, nullptr, "main", "cs_5_0", 0, 0, &blob, &err))) {
        std::printf("%s\n", err != nullptr ? static_cast<const char*>(err->GetBufferPointer()) : "D3DCompile");
        frei(err);
        frei(ctx);
        frei(dev);
        return false;
    }
    frei(err);
    ID3D11ComputeShader* cs = nullptr;
    dev->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &cs);
    frei(blob);

    const auto anzahl = static_cast<UINT>(ein.size());
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = anzahl * static_cast<UINT>(sizeof(Eingabe));
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bd.StructureByteStride = 16;
    D3D11_SUBRESOURCE_DATA sd{ein.data(), 0, 0};
    ID3D11Buffer* bEin = nullptr;
    dev->CreateBuffer(&bd, &sd, &bEin);
    D3D11_SHADER_RESOURCE_VIEW_DESC sv{};
    sv.Format = DXGI_FORMAT_UNKNOWN;
    sv.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    sv.Buffer.NumElements = anzahl * 8U;
    ID3D11ShaderResourceView* srv = nullptr;
    dev->CreateShaderResourceView(bEin, &sv, &srv);

    D3D11_BUFFER_DESC ba{};
    ba.ByteWidth = anzahl * 8U;
    ba.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    ba.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    ba.StructureByteStride = 8;
    ID3D11Buffer* bAus = nullptr;
    dev->CreateBuffer(&ba, nullptr, &bAus);
    D3D11_UNORDERED_ACCESS_VIEW_DESC uv{};
    uv.Format = DXGI_FORMAT_UNKNOWN;
    uv.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uv.Buffer.NumElements = anzahl;
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
    ctx->Dispatch((anzahl + 63U) / 64U, 1, 1);
    ctx->CopyResource(bLesen, bAus);
    D3D11_MAPPED_SUBRESOURCE map{};
    aus.assign(static_cast<std::size_t>(anzahl) * 2U, 0U);
    if (SUCCEEDED(ctx->Map(bLesen, 0, D3D11_MAP_READ, 0, &map))) {
        std::memcpy(aus.data(), map.pData, aus.size() * 4U);
        ctx->Unmap(bLesen, 0);
    }
    frei(bLesen);
    frei(uav);
    frei(bAus);
    frei(srv);
    frei(bEin);
    frei(cs);
    frei(ctx);
    frei(dev);
    return true;
}

}  // namespace

// DEF1 P46: die tcMod-Kette der Wolkenstufe rechnet im Pixel-Shader (pixelShaderHlsl mit kWolke: st erst je Bildpunkt, turb aussen vor).
// Ein Bildpunkt je Probe: der Pixel-Shader liest seine Werte aus Buffer<float4> t0 (Index aus SV_Position) und schreibt st in ein
// R32G32-Ziel; Profil wie die Vorschau (wolkenPsZiel, gpushader.h) oder ps_4_0 (bis DEF1).
const char* const kPsHuelle = R"(
Buffer<float4> gEin : register(t0);
float4 main(float4 pos : SV_POSITION) : SV_Target {
    uint b = ((uint)pos.x + (uint)pos.y * 256u) * 8u;
    float4 a = gEin[b];
    return float4(tcKette(a.zw, gEin[b + 1], gEin[b + 2], gEin[b + 3], gEin[b + 4], gEin[b + 5], gEin[b + 6], gEin[b + 7], float2(0.0, 0.0), false), 0.0, 0.0);
}
)";
const char* const kPsDreieck = R"(
float4 main(uint id : SV_VertexID) : SV_POSITION {
    float2 p = float2((id == 1) ? 3.0 : -1.0, (id == 2) ? -3.0 : 1.0);
    return float4(p, 0.0, 1.0);
}
)";
bool psLaufen(D3D_DRIVER_TYPE art, const char* profil, const std::vector<float>& zusatz, UINT zeilen, std::vector<std::uint32_t>& aus,
              std::string& name, std::string* zerlegung) {
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    const D3D_FEATURE_LEVEL stufe = D3D_FEATURE_LEVEL_11_0;
    name.clear();
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
    const std::string ps = tcModHlsl() + kPsHuelle;
    ID3DBlob* vb = nullptr;
    ID3DBlob* pb = nullptr;
    ID3DBlob* err = nullptr;
    const bool ueb = SUCCEEDED(D3DCompile(kPsDreieck, std::strlen(kPsDreieck), "vs", nullptr, nullptr, "main", "vs_4_0", 0, 0, &vb, &err)) &&
                     SUCCEEDED(D3DCompile(ps.data(), ps.size(), "ps", nullptr, nullptr, "main", profil, 0, 0, &pb, &err));
    if (!ueb) {
        std::printf("%s\n", err != nullptr ? static_cast<const char*>(err->GetBufferPointer()) : "D3DCompile");
        frei(err);
        frei(vb);
        frei(pb);
        frei(ctx);
        frei(dev);
        return false;
    }
    frei(err);
    if (zerlegung != nullptr) { *zerlegung = vsstrom::zerlegt(pb); }
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* pxs = nullptr;
    dev->CreateVertexShader(vb->GetBufferPointer(), vb->GetBufferSize(), nullptr, &vs);
    dev->CreatePixelShader(pb->GetBufferPointer(), pb->GetBufferSize(), nullptr, &pxs);
    frei(vb);
    frei(pb);
    D3D11_TEXTURE2D_DESC td{};
    td.Width = 256;
    td.Height = zeilen;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32G32_FLOAT;
    td.SampleDesc.Count = 1;
    td.BindFlags = D3D11_BIND_RENDER_TARGET;
    ID3D11Texture2D* ziel = nullptr;
    dev->CreateTexture2D(&td, nullptr, &ziel);
    td.BindFlags = 0;
    td.Usage = D3D11_USAGE_STAGING;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* lesen = nullptr;
    dev->CreateTexture2D(&td, nullptr, &lesen);
    ID3D11RenderTargetView* rtv = nullptr;
    if (ziel != nullptr) { dev->CreateRenderTargetView(ziel, nullptr, &rtv); }
    D3D11_BUFFER_DESC bz{};
    bz.ByteWidth = static_cast<UINT>(zusatz.size() * 4U);
    bz.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA sz{zusatz.data(), 0, 0};
    ID3D11Buffer* zb = nullptr;
    dev->CreateBuffer(&bz, &sz, &zb);
    D3D11_SHADER_RESOURCE_VIEW_DESC sr{};
    sr.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    sr.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    sr.Buffer.NumElements = static_cast<UINT>(zusatz.size() / 4U);
    ID3D11ShaderResourceView* srv = nullptr;
    if (zb != nullptr) { dev->CreateShaderResourceView(zb, &sr, &srv); }
    const bool gut0 = vs != nullptr && pxs != nullptr && rtv != nullptr && srv != nullptr && lesen != nullptr;
    if (gut0) {
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(vs, nullptr, 0);
        ctx->PSSetShader(pxs, nullptr, 0);
        ctx->PSSetShaderResources(0, 1, &srv);
        ctx->OMSetRenderTargets(1, &rtv, nullptr);
        const D3D11_VIEWPORT vp{0.0F, 0.0F, 256.0F, static_cast<float>(zeilen), 0.0F, 1.0F};
        ctx->RSSetViewports(1, &vp);
        ctx->Draw(3, 0);
        ctx->CopyResource(lesen, ziel);
    }
    D3D11_MAPPED_SUBRESOURCE map{};
    const bool gut = gut0 && SUCCEEDED(ctx->Map(lesen, 0, D3D11_MAP_READ, 0, &map));
    if (gut) {
        aus.assign(static_cast<std::size_t>(256U * zeilen) * 2U, 0U);
        for (UINT y = 0; y < zeilen; ++y) {
            std::memcpy(aus.data() + static_cast<std::size_t>(y) * 512U, static_cast<const char*>(map.pData) + static_cast<std::size_t>(y) * map.RowPitch, 2048);
        }
        ctx->Unmap(lesen, 0);
    }
    frei(srv);
    frei(zb);
    frei(rtv);
    frei(lesen);
    frei(ziel);
    frei(pxs);
    frei(vs);
    frei(ctx);
    frei(dev);
    return gut;
}

int main() {
    std::printf("turbgpu\n");
    std::mt19937 zz(20261109U);
    const auto zahl = [&](double a, double b) { return static_cast<float>(a + (b - a) * (static_cast<double>(zz()) / 4294967296.0)); };

    // Die Ketten: turb allein (mit den schwierigen Orten und Zeiten wie tests/gpuconst.cpp), dazu Ketten aus mehreren Schritten
    TexMod turb;
    turb.kind = TexModKind::Turb;
    turb.wave.func = WaveFunc::Sin;
    turb.wave.amplitude = 0.3F;
    turb.wave.phase = 0.1F;
    turb.wave.frequency = 0.5F;
    TexMod turb2 = turb;
    turb2.wave.amplitude = 0.07F;
    turb2.wave.phase = 0.6F;
    turb2.wave.frequency = 2.25F;
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
    const Kette ketten[] = {{"turb (Orte und Zeiten wie gpuconst)", {turb, {}, {}, {}}, 1},
                            {"turb, scale 2 3", {turb, skal, {}, {}}, 2},
                            {"turb, rotate 30, scale 2 3", {turb, dreh, skal, {}}, 3},
                            {"scroll, turb, transform", {rollen, turb, trans, {}}, 3},
                            {"turb, rotate 30, turb, scale 2 3", {turb, dreh, turb2, skal}, 4},
                            {"scale 2 3, scroll, rotate 30, transform", {skal, rollen, dreh, trans}, 4}};
    for (const Kette& k : ketten) {
        std::vector<Eingabe> ein;
        std::vector<float> soll;
        const bool allein = k.n == 1;
        for (int i = 0; i < (allein ? 65536 : 16384); ++i) {
            float ort[2];
            float zeit = 0.0F;
            TexMod m[4] = {k.m[0], k.m[1], k.m[2], k.m[3]};
            if (allein) {
                // now = phase (frequency 0): beliebig, ganze und kleine Orte, 1024 now ueber 2^24, Vorzeichenwechsel, dicht an ganz
                const int art = i % 6;
                for (float& x : ort) {
                    x = art == 1 ? std::round(zahl(-9000.0, 9000.0)) : art == 2 ? zahl(-1.0, 1.0) : art == 4 ? zahl(-5000.0, -200.0) : zahl(-70000.0, 70000.0);
                }
                float now = zahl(-50.0, 50.0);
                if (art == 3) {
                    now = zahl(16384.0, 2.0e6);
                } else if (art == 4) {
                    now = zahl(0.2, 6.0);
                } else if (art == 5) {
                    const double ganz = std::round(static_cast<double>(ort[0]) + zahl(-3000.0, 3000.0));
                    now = static_cast<float>((ganz - static_cast<double>(ort[0])) / 1024.0);
                    const int schritte = static_cast<int>(zz() % 7U) - 3;
                    for (int s = 0; s < std::abs(schritte); ++s) { now = std::nextafter(now, schritte > 0 ? 1.0e30F : -1.0e30F); }
                }
                m[0].wave.phase = now;
                m[0].wave.frequency = 0.0F;
                m[0].wave.amplitude = zahl(0.01, 1.0);
            } else {
                ort[0] = zahl(-18000.0, 18000.0);
                ort[1] = zahl(-9000.0, 9000.0);
                zeit = (i % 2 == 0) ? zahl(0.0, 100.0) : zahl(0.0, 50000.0);
            }
            float st[2] = {zahl(-4.0, 4.0), zahl(-4.0, 4.0)};
            BatchState bs{};
            bs.numTexMods = k.n;
            for (int j = 0; j < k.n; ++j) { bs.texMods[j] = m[j]; }
            float konst[kKonstFloats];
            packeKonstanten(bs, konst, zeit);
            Eingabe e{};
            e.v[0] = ort[0];
            e.v[1] = ort[1];
            e.v[2] = st[0];
            e.v[3] = st[1];
            std::memcpy(e.v + 4, konst + kTcM0At, 8 * sizeof(float));
            std::memcpy(e.v + 12, konst + kTcM1At, 20 * sizeof(float));
            ein.push_back(e);
            spielSt(m, k.n, zeit, ort, st);
            soll.push_back(st[0]);
            soll.push_back(st[1]);
        }
        // FIX5 P35: als Vertex-Shader mit den echten Profilen und Fahnen, dazu der Bytecode (kein Umstellen/Verschmelzen erlaubt)
        {
            std::vector<float> zusatz;
            for (const Eingabe& e : ein) { zusatz.insert(zusatz.end(), e.v, e.v + 32); }
            const std::vector<vsstrom::Ecke> ecken(ein.size(), vsstrom::Ecke{});
            for (const Profil& pr : kProfile) {
                for (const D3D_DRIVER_TYPE art : {D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_HARDWARE}) {
                    const char* wer = art == D3D_DRIVER_TYPE_WARP ? "WARP" : "Grafikkarte";
                    std::vector<std::uint32_t> aus;
                    std::string name;
                    std::string dis;
                    if (!vsstrom::laufen(art, tcModHlsl() + kVsHuelle, pr.profil, pr.fahnen, vsstrom::Eingabe::Karte, {}, ecken, aus, name, &dis,
                                         &zusatz)) {
                        erwarte((std::string(wer) + " " + pr.profil + ": Geraet nicht da oder Shader nicht uebersetzt").c_str(),
                                art == D3D_DRIVER_TYPE_HARDWARE && name.empty());
                        continue;
                    }
                    int falsch = 0;
                    for (std::size_t i = 0; i < ein.size(); ++i) {
                        for (std::size_t a = 0; a < 2U; ++a) {
                            std::uint32_t sb = 0;
                            std::memcpy(&sb, &soll[i * 2U + a], 4);
                            if (aus[i * 6U + 4U + a] != sb) { ++falsch; }
                        }
                    }
                    char z[300];
                    std::snprintf(z, sizeof z, "P35 %s (%s), %s %s%s: %s bitgleich zu rd-vanilla: %d von %zu Werten abweichend", wer, name.c_str(),
                                  pr.wo, pr.profil, pr.fahnen != 0U ? " O3" : "", k.was, falsch, soll.size());
                    if (!pr.ab11 && art == D3D_DRIVER_TYPE_HARDWARE) {
                        std::printf("  Hinweis  %s (nimmt diese Karte nicht)\n", z);
                    } else {
                        erwarte(z, falsch == 0);
                    }
                    if (art == D3D_DRIVER_TYPE_WARP) {
                        const std::string u = vsstrom::umstellbar(dis, pr.profil, true);
                        erwarte((std::string("P35 Bytecode ") + pr.profil + (pr.fahnen != 0U ? " O3" : "") + " (" + k.was +
                                 "): der Treiber darf nichts umstellen" + (u.empty() ? "" : " - " + u))
                                    .c_str(),
                                u.empty());
                    }
                }
            }
        }
        for (const D3D_DRIVER_TYPE art : {D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_HARDWARE}) {
            const char* wer = art == D3D_DRIVER_TYPE_WARP ? "WARP" : "Grafikkarte";
            std::vector<std::uint32_t> aus;
            std::string name;
            if (!laufen(art, ein, aus, name)) {
                erwarte((std::string(wer) + ": Geraet nicht da oder Shader nicht uebersetzt").c_str(), art == D3D_DRIVER_TYPE_HARDWARE && name.empty());
                continue;
            }
            int falsch = 0;
            for (std::size_t i = 0; i < soll.size(); ++i) {
                std::uint32_t s = 0;
                std::memcpy(&s, &soll[i], 4);
                if (aus[i] != s) {
                    if (falsch < 3) {
                        float g = 0.0F;
                        std::memcpy(&g, &aus[i], 4);
                        std::printf("        %s %s: ort %.9g q %.9g: GPU %.9g, Spiel %.9g\n", wer, k.was, static_cast<double>(ein[i / 2].v[i % 2]),
                                    static_cast<double>(ein[i / 2].v[4]), static_cast<double>(g), static_cast<double>(soll[i]));
                    }
                    ++falsch;
                }
            }
            char z[260];
            std::snprintf(z, sizeof z, "P33 %s (%s): %s bitgleich zu rd-vanilla: %d von %zu Werten abweichend", wer, name.c_str(), k.was, falsch,
                          soll.size());
            erwarte(z, falsch == 0);
        }
    }
    // --- DEF1 P46: die Kette der Wolkenstufe im Pixel-Shader (ohne turb, st beliebig wie die Kuppel sie liefert) ---------------------
    {
        const Kette wolken[] = {{"scroll", {rollen, {}, {}, {}}, 1},
                                {"rotate 30, scale 2 3", {dreh, skal, {}, {}}, 2},
                                {"scale 2 3, scroll, rotate 30, transform", {skal, rollen, dreh, trans}, 4}};
        for (const Kette& k : wolken) {
            const UINT zeilen = 64;
            std::vector<float> zusatz;
            std::vector<float> soll;
            for (UINT i = 0; i < 256U * zeilen; ++i) {
                const float zeit = (i % 2 == 0) ? zahl(0.0, 100.0) : zahl(0.0, 50000.0);
                float st[2] = {zahl(0.0, 3.2), zahl(0.0, 3.2)};   // acos: 0 .. pi
                BatchState bs{};
                bs.numTexMods = k.n;
                for (int j = 0; j < k.n; ++j) { bs.texMods[j] = k.m[j]; }
                float konst[kKonstFloats];
                packeKonstanten(bs, konst, zeit);
                const float vorn[4] = {0.0F, 0.0F, st[0], st[1]};
                zusatz.insert(zusatz.end(), vorn, vorn + 4);
                zusatz.insert(zusatz.end(), konst + kTcM0At, konst + kTcM0At + 8);
                zusatz.insert(zusatz.end(), konst + kTcM1At, konst + kTcM1At + 20);
                const float ort[2] = {0.0F, 0.0F};
                spielSt(k.m, k.n, zeit, ort, st);
                soll.push_back(st[0]);
                soll.push_back(st[1]);
            }
            for (int welches = 0; welches < 2; ++welches) {
                const char* const profil = welches == 0 ? "ps_4_0" : wolkenPsZiel(true);
                for (const D3D_DRIVER_TYPE art : {D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_HARDWARE}) {
                    const char* wer = art == D3D_DRIVER_TYPE_WARP ? "WARP" : "Grafikkarte";
                    std::vector<std::uint32_t> aus;
                    std::string name;
                    std::string dis;
                    if (!psLaufen(art, profil, zusatz, zeilen, aus, name, &dis)) {
                        erwarte((std::string("P46 ") + wer + " " + profil + ": Geraet nicht da oder Shader nicht uebersetzt").c_str(),
                                art == D3D_DRIVER_TYPE_HARDWARE && name.empty());
                        continue;
                    }
                    int falsch = 0;
                    for (std::size_t i = 0; i < soll.size(); ++i) {
                        std::uint32_t s = 0;
                        std::memcpy(&s, &soll[i], 4);
                        if (aus[i] != s) { ++falsch; }
                    }
                    char z[300];
                    std::snprintf(z, sizeof z, "P46 Wolkenstufe %s (%s), %s: %s bitgleich zu rd-vanilla: %d von %zu Werten abweichend", wer, name.c_str(),
                                  profil, k.was, falsch, soll.size());
                    if (welches == 0) {
                        std::printf("  Hinweis  %s (bis DEF1)\n", z);
                    } else {
                        erwarte(z, falsch == 0);
                    }
                }
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
