// karte_gpu_win32.cpp - Karten-Editor: Zeichnen ueber Direct3D 11 (siehe
// karte_gpu.h). Je Ansicht ein Renderziel mit Mehrfachabtastung (NRC: FBO
// mit MSAA, Vorgabe 8 Abtastungen, xywindow.cpp:2291 "XYMSAA"), Linien als
// Linienliste, breite Linien (glLineWidth 2) ueber einen Geometrie-Shader.
#include "karte_gpu.h"

#include "bhed/karte/texturen.h"

#include "backend.h"

#include "imgui.h"

#include <d3d11.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace bhed::gui::kartegpu {

namespace {

std::vector<std::pair<ID3D11ShaderResourceView*, int>> g_spaeter;   // bildFreigeben: Bild, ImGui-Bild der Freigabe

struct Punkt {
    float x, y, z;
    std::uint32_t farbe;   // R8G8B8A8_UNORM (Rot im niedrigsten Byte)
};

struct Konstanten {
    float mvp[16];
    float groesse[2];
    float breite;
    float frei;
    float muster[4];   // x glLineStipple-Muster (0 = aus), y Faktor
};

struct Ziel {
    ID3D11Texture2D* tex = nullptr;        // mit Mehrfachabtastung
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11Texture2D* aufgeloest = nullptr; // ohne, fuer ImGui
    ID3D11ShaderResourceView* srv = nullptr;
    ID3D11Texture2D* tiefe = nullptr;      // D24S8 (nur die Kamera)
    ID3D11DepthStencilView* dsv = nullptr;
    int breite = 0;
    int hoehe = 0;
    int msaa = 1;
};

const char* const kShader = R"(
cbuffer B : register(b0) { float4x4 mvp; float2 groesse; float breite; float frei; float4 muster; };
struct VSI { float3 pos : POSITION; float4 farbe : COLOR; };
struct VSO { float4 pos : SV_Position; float4 farbe : COLOR; nointerpolation float2 start : TEXCOORD0; };
VSO vs(VSI i) {
    VSO o;
    o.pos = mul(mvp, float4(i.pos, 1.0));
    o.pos.z = (o.pos.z + o.pos.w) * 0.5;   // OpenGL-Tiefe -1..1 -> Direct3D 0..1
    o.farbe = i.farbe;
    o.start = float2(0.0, 0.0);
    return o;
}
[maxvertexcount(4)]
void gs(line VSO e[2], inout TriangleStream<VSO> s) {
    float2 h = groesse * 0.5;
    if (frei > 0.5) {
        // glPointSize: ein Quadrat breite x breite um den Punkt
        float2 q = (breite * 0.5) / h;
        VSO p;
        p = e[0]; p.pos.xy += float2(-q.x, q.y) * e[0].pos.w; s.Append(p);
        p = e[0]; p.pos.xy += float2(-q.x, -q.y) * e[0].pos.w; s.Append(p);
        p = e[0]; p.pos.xy += float2(q.x, q.y) * e[0].pos.w; s.Append(p);
        p = e[0]; p.pos.xy += float2(q.x, -q.y) * e[0].pos.w; s.Append(p);
        return;
    }
    float2 a = e[0].pos.xy / e[0].pos.w * h;
    float2 b = e[1].pos.xy / e[1].pos.w * h;
    float2 d = b - a;
    float l = length(d);
    d = (l > 1e-6) ? d / l : float2(1.0, 0.0);
    float2 n = float2(-d.y, d.x) * (breite * 0.5) / h;
    // glLineStipple: der Zaehler beginnt an jedem Anfang einer GL_LINES-Strecke
    float2 st = float2(a.x + h.x, h.y - a.y);
    VSO v;
    v = e[0]; v.pos.xy += n * e[0].pos.w; v.start = st; s.Append(v);
    v = e[0]; v.pos.xy -= n * e[0].pos.w; v.start = st; s.Append(v);
    v = e[1]; v.pos.xy += n * e[1].pos.w; v.start = st; s.Append(v);
    v = e[1]; v.pos.xy -= n * e[1].pos.w; v.start = st; s.Append(v);
}
// GL_LINE_STIPPLE: Bit (Bildpunkte entlang der Hauptachse / Faktor) mod 16 des Musters
float4 ps(VSO i) : SV_Target {
    if (muster.x > 0.5) {
        float2 d = abs(i.pos.xy - i.start);
        uint zaehler = (uint)floor(max(d.x, d.y));
        uint bit = (zaehler / (uint)muster.y) & 15u;
        if (((((uint)muster.x) >> bit) & 1u) == 0u) { discard; }
    }
    return i.farbe;
}
)";

struct Zustand {
    unsigned stand = ~0U;
    bool gescheitert = false;
    ID3D11VertexShader* vs = nullptr;
    ID3D11GeometryShader* gs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11InputLayout* layout = nullptr;
    ID3D11Buffer* cb = nullptr;
    ID3D11BlendState* blend = nullptr;
    ID3D11RasterizerState* rsMsaa = nullptr;
    ID3D11RasterizerState* rsEinfach = nullptr;
    ID3D11DepthStencilState* dsAus = nullptr;
    ID3D11Buffer* einmal = nullptr;   // gemeinsamer Puffer fuer Einmal-Linien
    std::size_t einmalPlatz = 0;
    ID3D11BlendState* blendRechteck[2] = {nullptr, nullptr};   // XORRectangle: additiv, Filter
    // 3D (Kamera)
    bool gescheitert3 = false;
    ID3D11VertexShader* vs3 = nullptr;
    ID3D11PixelShader* ps3 = nullptr;
    ID3D11VertexShader* vsl = nullptr;
    ID3D11GeometryShader* gsl = nullptr;
    ID3D11GeometryShader* gslBreit = nullptr;   // glLineWidth > 1
    ID3D11PixelShader* psl = nullptr;
    ID3D11InputLayout* layout3 = nullptr;
    ID3D11InputLayout* layoutl = nullptr;
    ID3D11Buffer* cb3 = nullptr;
    ID3D11BlendState* blendAus = nullptr;
    ID3D11BlendState* blendAdd = nullptr;
    ID3D11RasterizerState* rs3[4] = {nullptr, nullptr, nullptr, nullptr};   // [msaa][wegschneiden]
    std::map<int, ID3D11DepthStencilState*> tiefen;   // Test * 2 + Schreiben
    ID3D11SamplerState* abtaster = nullptr;
    ID3D11SamplerState* wuerfelAbtaster = nullptr;
    ID3D11Buffer* einmal3 = nullptr;   // Einmal-Dreiecke
    std::size_t einmal3Platz = 0;
    ID3D11Buffer* einmalL = nullptr;   // Einmal-Linien (3D)
    std::size_t einmalLPlatz = 0;
    std::map<int, Ziel> ziele;
    // Stufe 7: Echtzeit-Licht
    bool gescheitert4 = false;
    ID3D11VertexShader* vs4 = nullptr;
    ID3D11PixelShader* ps4 = nullptr;
    ID3D11VertexShader* vsSchatten = nullptr;
    ID3D11PixelShader* psSchatten = nullptr;
    ID3D11PixelShader* psSonne = nullptr;
    ID3D11Buffer* cb4 = nullptr;          // Lichter
    ID3D11Buffer* cbSchatten = nullptr;   // Seite eines Wuerfels
    ID3D11Texture2D* wuerfelTex = nullptr;            // 6 * kSchattenMax Seiten, R16F
    ID3D11RenderTargetView* wuerfelRtv[6 * kSchattenMax] = {};
    ID3D11ShaderResourceView* wuerfelSrv = nullptr;   // TextureCubeArray
    ID3D11Texture2D* wuerfelTiefe = nullptr;
    ID3D11DepthStencilView* wuerfelDsv = nullptr;
    ID3D11Texture2D* sonneTex = nullptr;              // R32F
    ID3D11RenderTargetView* sonneRtv = nullptr;
    ID3D11ShaderResourceView* sonneSrv = nullptr;
    ID3D11Texture2D* sonneTiefe = nullptr;
    ID3D11DepthStencilView* sonneDsv = nullptr;
    ID3D11SamplerState* punktAbtaster = nullptr;
    ID3D11RasterizerState* rsSchatten = nullptr;
    float sonneVp[16] = {};
    bool sonneAn = false;
    // Stufe 7 Punkt 5b: gebackene Flaechen mit Lightmap
    bool gescheitertG = false;
    ID3D11VertexShader* vsG = nullptr;
    ID3D11PixelShader* psG = nullptr;
    ID3D11InputLayout* layoutG = nullptr;
    // waehrend beginnen/beenden
    int aktiv = -1;
    ID3D11RenderTargetView* altRtv = nullptr;
    ID3D11DepthStencilView* altDsv = nullptr;
    D3D11_VIEWPORT altVp{};
    UINT altVpAnzahl = 0;
};
Zustand g;

template<typename T>
void los(T*& p) {
    if (p != nullptr) {
        p->Release();
        p = nullptr;
    }
}

void echtLichtLoesen() {
    los(g.vs4);
    los(g.ps4);
    los(g.vsSchatten);
    los(g.psSchatten);
    los(g.psSonne);
    los(g.cb4);
    los(g.cbSchatten);
    for (auto& v : g.wuerfelRtv) { los(v); }
    los(g.wuerfelSrv);
    los(g.wuerfelTex);
    los(g.wuerfelDsv);
    los(g.wuerfelTiefe);
    los(g.sonneRtv);
    los(g.sonneSrv);
    los(g.sonneTex);
    los(g.sonneDsv);
    los(g.sonneTiefe);
    los(g.punktAbtaster);
    los(g.rsSchatten);
    los(g.vsG);
    los(g.psG);
    los(g.layoutG);
    g.gescheitertG = false;
    g.gescheitert4 = false;
    g.sonneAn = false;
}

void zielLoesen(Ziel& z) {
    los(z.dsv);
    los(z.tiefe);
    los(z.srv);
    los(z.aufgeloest);
    los(z.rtv);
    los(z.tex);
}

void allesLoesen() {
    for (auto& [k, z] : g.ziele) {
        (void)k;
        zielLoesen(z);
    }
    g.ziele.clear();
    echtLichtLoesen();
    los(g.einmal);
    g.einmalPlatz = 0;
    los(g.blendRechteck[0]);
    los(g.blendRechteck[1]);
    los(g.einmal3);
    g.einmal3Platz = 0;
    los(g.einmalL);
    g.einmalLPlatz = 0;
    for (auto& [k, t] : g.tiefen) {
        (void)k;
        los(t);
    }
    g.tiefen.clear();
    for (auto& r : g.rs3) { los(r); }
    los(g.wuerfelAbtaster);
    los(g.abtaster);
    los(g.blendAdd);
    los(g.blendAus);
    los(g.cb3);
    los(g.layoutl);
    los(g.layout3);
    los(g.psl);
    los(g.gslBreit);
    los(g.gsl);
    los(g.vsl);
    los(g.ps3);
    los(g.vs3);
    g.gescheitert3 = false;
    los(g.dsAus);
    los(g.rsEinfach);
    los(g.rsMsaa);
    los(g.blend);
    los(g.cb);
    los(g.layout);
    los(g.ps);
    los(g.gs);
    los(g.vs);
}

ID3D11Device* geraet() { return static_cast<ID3D11Device*>(render::d3dDevice()); }
ID3D11DeviceContext* kontext() { return static_cast<ID3D11DeviceContext*>(render::d3dContext()); }

// Shader und Zustaende anlegen (einmal je Geraet).
bool bereit() {
    ID3D11Device* d = geraet();
    if (d == nullptr) { return false; }
    if (g.stand != render::texturStand()) {
        // Geraet neu (Wechsel, Verlust): alles von vorn.
        allesLoesen();
        g.stand = render::texturStand();
        g.gescheitert = false;
    }
    if (g.vs != nullptr) { return true; }
    if (g.gescheitert) { return false; }
    g.gescheitert = true;
    const auto uebersetzen = [](const char* einstieg, const char* ziel, ID3DBlob** code) {
        ID3DBlob* fehler = nullptr;
        const HRESULT hr = D3DCompile(kShader, std::strlen(kShader), "karte", nullptr, nullptr, einstieg, ziel,
                                      D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, code, &fehler);
        los(fehler);
        return SUCCEEDED(hr);
    };
    ID3DBlob* vsCode = nullptr;
    ID3DBlob* gsCode = nullptr;
    ID3DBlob* psCode = nullptr;
    if (!uebersetzen("vs", "vs_4_0", &vsCode) || !uebersetzen("gs", "gs_4_0", &gsCode) || !uebersetzen("ps", "ps_4_0", &psCode)) {
        los(vsCode);
        los(gsCode);
        los(psCode);
        return false;
    }
    d->CreateVertexShader(vsCode->GetBufferPointer(), vsCode->GetBufferSize(), nullptr, &g.vs);
    d->CreateGeometryShader(gsCode->GetBufferPointer(), gsCode->GetBufferSize(), nullptr, &g.gs);
    d->CreatePixelShader(psCode->GetBufferPointer(), psCode->GetBufferSize(), nullptr, &g.ps);
    const D3D11_INPUT_ELEMENT_DESC elemente[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    d->CreateInputLayout(elemente, 2, vsCode->GetBufferPointer(), vsCode->GetBufferSize(), &g.layout);
    los(vsCode);
    los(gsCode);
    los(psCode);
    D3D11_BUFFER_DESC cb{};
    cb.ByteWidth = sizeof(Konstanten);
    cb.Usage = D3D11_USAGE_DYNAMIC;
    cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    d->CreateBuffer(&cb, nullptr, &g.cb);
    D3D11_BLEND_DESC bd{};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    d->CreateBlendState(&bd, &g.blend);
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    rd.MultisampleEnable = TRUE;
    d->CreateRasterizerState(&rd, &g.rsMsaa);
    rd.MultisampleEnable = FALSE;
    d->CreateRasterizerState(&rd, &g.rsEinfach);
    D3D11_DEPTH_STENCIL_DESC dd{};
    dd.DepthEnable = FALSE;
    dd.StencilEnable = FALSE;
    d->CreateDepthStencilState(&dd, &g.dsAus);
    g.gescheitert = g.vs == nullptr || g.gs == nullptr || g.ps == nullptr || g.layout == nullptr || g.cb == nullptr;
    return !g.gescheitert;
}

// Eine Linienliste in einen dynamischen Puffer schreiben (bei Bedarf groesser).
bool fuellen(ID3D11Buffer*& vb, std::size_t& platz, const std::vector<karte::Linie>& linien) {
    ID3D11Device* d = geraet();
    if (d == nullptr) { return false; }
    if (linien.size() > platz || vb == nullptr) {
        los(vb);
        platz = std::max<std::size_t>(linien.size() + linien.size() / 2, 1024);
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = static_cast<UINT>(platz * 2 * sizeof(Punkt));
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(d->CreateBuffer(&bd, nullptr, &vb))) {
            platz = 0;
            return false;
        }
    }
    if (linien.empty()) { return true; }
    D3D11_MAPPED_SUBRESOURCE m{};
    if (FAILED(kontext()->Map(vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) { return false; }
    auto* p = static_cast<Punkt*>(m.pData);
    for (const karte::Linie& l : linien) {
        *p++ = {l.a[0], l.a[1], l.a[2], l.farbe};
        *p++ = {l.b[0], l.b[1], l.b[2], l.farbe};
    }
    kontext()->Unmap(vb, 0);
    return true;
}

ID3D11DepthStencilState* tiefenZustand(TiefenTest t, bool schreiben);

void ziehen(ID3D11Buffer* vb, std::size_t anzahl, const float* matrix, float breite, bool punktModus = false,
            ID3D11DepthStencilState* tiefe = nullptr, unsigned muster = 0, int musterFaktor = 1) {
    if (g.aktiv < 0 || vb == nullptr || anzahl == 0) { return; }
    ID3D11DeviceContext* c = kontext();
    const Ziel& z = g.ziele[g.aktiv];
    D3D11_MAPPED_SUBRESOURCE m{};
    if (FAILED(c->Map(g.cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) { return; }
    auto* k = static_cast<Konstanten*>(m.pData);
    std::memcpy(k->mvp, matrix, sizeof(k->mvp));
    k->groesse[0] = static_cast<float>(z.breite);
    k->groesse[1] = static_cast<float>(z.hoehe);
    k->breite = breite;
    k->frei = punktModus ? 1.0F : 0.0F;
    k->muster[0] = static_cast<float>(muster);
    k->muster[1] = static_cast<float>(std::max(1, musterFaktor));
    k->muster[2] = 0.0F;
    k->muster[3] = 0.0F;
    c->Unmap(g.cb, 0);
    const UINT schritt = sizeof(Punkt);
    const UINT versatz = 0;
    // Zustand der 2D-Linien (nach einem 3D-Durchgang im selben Ziel noetig)
    const float faktor[4] = {0, 0, 0, 0};
    c->OMSetBlendState(g.blend, faktor, 0xFFFFFFFFU);
    c->OMSetDepthStencilState(tiefe != nullptr ? tiefe : g.dsAus, 0);
    c->RSSetState(z.msaa > 1 ? g.rsMsaa : g.rsEinfach);
    c->IASetVertexBuffers(0, 1, &vb, &schritt, &versatz);
    c->IASetInputLayout(g.layout);
    c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    c->VSSetShader(g.vs, nullptr, 0);
    c->VSSetConstantBuffers(0, 1, &g.cb);
    if (breite > 1.0F || punktModus || muster != 0) {   // das Muster braucht den Anfang der Strecke
        c->GSSetShader(g.gs, nullptr, 0);
        c->GSSetConstantBuffers(0, 1, &g.cb);
    } else {
        c->GSSetShader(nullptr, nullptr, 0);
    }
    c->PSSetShader(g.ps, nullptr, 0);
    c->PSSetConstantBuffers(0, 1, &g.cb);   // das Strichmuster (glLineStipple)
    c->Draw(static_cast<UINT>(anzahl * 2), 0);
    c->GSSetShader(nullptr, nullptr, 0);
}

}  // namespace

bool verfuegbar() { return render::current() == render::Backend::Direct3D11 && bereit(); }

Puffer::~Puffer() {
    if (m_vb != nullptr && m_stand == render::texturStand()) { static_cast<ID3D11Buffer*>(m_vb)->Release(); }
}

void Puffer::setzen(const std::vector<karte::Linie>& linien) {
    if (!bereit()) { return; }
    if (m_stand != render::texturStand()) {   // altes Geraet: Zeiger ist ungueltig
        m_vb = nullptr;
        m_platz = 0;
        m_stand = render::texturStand();
    }
    auto* vb = static_cast<ID3D11Buffer*>(m_vb);
    if (fuellen(vb, m_platz, linien)) {
        m_vb = vb;
        m_anzahl = linien.size();
    } else {
        m_vb = vb;
        m_anzahl = 0;
    }
}

bool beginnen(int kennung, int breite, int hoehe, int msaa, const float* hinten, bool tiefe) {
    if (!bereit() || breite <= 0 || hoehe <= 0) { return false; }
    ID3D11Device* d = geraet();
    ID3D11DeviceContext* c = kontext();
    // Mehrfachabtastung: die verlangte Stufe oder die naechstkleinere, die
    // das Geraet kann.
    int stufe = std::max(1, msaa);
    while (stufe > 1) {
        UINT q = 0;
        if (SUCCEEDED(d->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT>(stufe), &q)) && q > 0) { break; }
        stufe /= 2;
    }
    Ziel& z = g.ziele[kennung];
    if (z.tex == nullptr || z.breite != breite || z.hoehe != hoehe || z.msaa != stufe || (tiefe && z.dsv == nullptr)) {
        zielLoesen(z);
        D3D11_TEXTURE2D_DESC td{};
        td.Width = static_cast<UINT>(breite);
        td.Height = static_cast<UINT>(hoehe);
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = static_cast<UINT>(stufe);
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET;
        if (FAILED(d->CreateTexture2D(&td, nullptr, &z.tex))) { return false; }
        d->CreateRenderTargetView(z.tex, nullptr, &z.rtv);
        td.SampleDesc.Count = 1;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(d->CreateTexture2D(&td, nullptr, &z.aufgeloest))) { return false; }
        d->CreateShaderResourceView(z.aufgeloest, nullptr, &z.srv);
        if (tiefe) {
            td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            td.SampleDesc.Count = static_cast<UINT>(stufe);
            td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
            if (FAILED(d->CreateTexture2D(&td, nullptr, &z.tiefe))) { return false; }
            d->CreateDepthStencilView(z.tiefe, nullptr, &z.dsv);
        }
        z.breite = breite;
        z.hoehe = hoehe;
        z.msaa = stufe;
    }
    if (z.rtv == nullptr) { return false; }
    g.altVpAnzahl = 1;
    c->RSGetViewports(&g.altVpAnzahl, &g.altVp);
    c->OMGetRenderTargets(1, &g.altRtv, &g.altDsv);
    c->OMSetRenderTargets(1, &z.rtv, tiefe ? z.dsv : nullptr);
    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(breite);
    vp.Height = static_cast<float>(hoehe);
    vp.MaxDepth = 1.0F;
    c->RSSetViewports(1, &vp);
    c->ClearRenderTargetView(z.rtv, hinten);
    if (tiefe && z.dsv != nullptr) { c->ClearDepthStencilView(z.dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0F, 0); }
    const float faktor[4] = {0, 0, 0, 0};
    c->OMSetBlendState(g.blend, faktor, 0xFFFFFFFFU);
    c->OMSetDepthStencilState(g.dsAus, 0);
    c->RSSetState(stufe > 1 ? g.rsMsaa : g.rsEinfach);
    g.aktiv = kennung;
    return true;
}

void zeichnen(const Puffer& p, const float* matrix, float breite) {
    if (p.m_stand != render::texturStand()) { return; }
    ziehen(static_cast<ID3D11Buffer*>(p.m_vb), p.m_anzahl, matrix, breite);
}

void zeichnen(const Puffer& p, const float* matrix, float breite, unsigned muster, int faktor) {
    if (p.m_stand != render::texturStand()) { return; }
    ziehen(static_cast<ID3D11Buffer*>(p.m_vb), p.m_anzahl, matrix, breite, false, nullptr, muster, faktor);
}

void punkte(const std::vector<karte::Linie>& punkte, const float* matrix, float groesse) {
    if (g.aktiv < 0 || punkte.empty()) { return; }
    if (!fuellen(g.einmal, g.einmalPlatz, punkte)) { return; }
    ziehen(g.einmal, punkte.size(), matrix, groesse, true);
}

void punkte(const std::vector<karte::Linie>& punkte, const float* matrix, float groesse, TiefenTest test) {
    if (g.aktiv < 0 || punkte.empty()) { return; }
    if (!fuellen(g.einmal, g.einmalPlatz, punkte)) { return; }
    ziehen(g.einmal, punkte.size(), matrix, groesse, true, tiefenZustand(test, false));
}

void auswahlRechteck(float x0, float y0, float x1, float y1, int art) {
    if (g.aktiv < 0 || !bereit()) { return; }
    if (x1 == x0 || y1 == y0) { return; }
    ID3D11Device* d = geraet();
    if (g.blendRechteck[0] == nullptr) {
        D3D11_BLEND_DESC bd{};
        bd.RenderTarget[0].BlendEnable = TRUE;
        bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        // additiv fuer dunklen Hintergrund: glBlendFunc( GL_ONE, GL_ONE )
        bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        d->CreateBlendState(&bd, &g.blendRechteck[0]);
        // Filter fuer hellen Hintergrund: glBlendFunc( GL_ZERO, GL_SRC_COLOR )
        bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
        bd.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
        bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
        d->CreateBlendState(&bd, &g.blendRechteck[1]);
    }
    const auto farbe = [](float r, float gr, float b, float a) {
        const auto k = [](float v) { return static_cast<std::uint32_t>(std::lround(std::clamp(v, 0.0F, 1.0F) * 255.0F)); };
        return k(r) | (k(gr) << 8U) | (k(b) << 16U) | (k(a) << 24U);
    };
    const float r = 10.F;
    std::uint32_t f[4];
    switch (art) {
    case 1:
        f[0] = farbe(0.F, 0.F, 1.F / r, 1);
        f[1] = farbe(.8F, .8F, 1.F, 1);
        f[2] = farbe(0.F, 0.F, 1.F, .3F);
        break;
    case 2:
        f[0] = farbe(1.F / r, 1.F / r, 1.F / r, 1);
        f[1] = farbe(.8F, .8F, .8F, 1);
        f[2] = farbe(1.F, 1.F, 1.F, .3F);
        break;
    default:
        f[0] = farbe(1.F / r, .5F / r, 0.F, 1);
        f[1] = farbe(1.F, .9F, .7F, 1);
        f[2] = farbe(1.F, .5F, 0.F, .3F);
        break;
    }
    f[3] = farbe(1.F, .5F, 0.F, 1);   // der Rand
    ID3D11DeviceContext* c = kontext();
    const Ziel& z = g.ziele[g.aktiv];
    D3D11_MAPPED_SUBRESOURCE m{};
    if (FAILED(c->Map(g.cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) { return; }
    auto* k = static_cast<Konstanten*>(m.pData);
    static const float kEins[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};   // glOrtho( -1, 1, -1, 1, ... )
    std::memcpy(k->mvp, kEins, sizeof(k->mvp));
    k->groesse[0] = static_cast<float>(z.breite);
    k->groesse[1] = static_cast<float>(z.hoehe);
    k->breite = 1;
    k->frei = 0;
    c->Unmap(g.cb, 0);
    const float faktor[4] = {0, 0, 0, 0};
    c->OMSetDepthStencilState(g.dsAus, 0);
    c->RSSetState(g.rsEinfach);   // glDisable( GL_MULTISAMPLE )
    c->IASetInputLayout(g.layout);
    c->VSSetShader(g.vs, nullptr, 0);
    c->VSSetConstantBuffers(0, 1, &g.cb);
    c->GSSetShader(nullptr, nullptr, 0);
    c->PSSetShader(g.ps, nullptr, 0);
    const UINT schritt = sizeof(Punkt);
    const UINT versatz = 0;
    for (int pass = 0; pass < 4; ++pass) {
        Punkt p[8];
        std::size_t n = 0;
        if (pass < 3) {   // GL_QUADS (x0,y1) (x1,y1) (x1,y0) (x0,y0) als zwei Dreiecke
            const Punkt q[4] = {{x0, y1, 0, f[pass]}, {x1, y1, 0, f[pass]}, {x1, y0, 0, f[pass]}, {x0, y0, 0, f[pass]}};
            p[0] = q[0];
            p[1] = q[1];
            p[2] = q[2];
            p[3] = q[0];
            p[4] = q[2];
            p[5] = q[3];
            n = 6;
        } else {   // GL_LINE_LOOP
            const Punkt q[4] = {{x0, y1, 0, f[3]}, {x1, y1, 0, f[3]}, {x1, y0, 0, f[3]}, {x0, y0, 0, f[3]}};
            for (std::size_t i = 0; i < 4; ++i) {
                p[i * 2] = q[i];
                p[i * 2 + 1] = q[(i + 1) % 4];
            }
            n = 8;
        }
        std::vector<karte::Linie> dummy;
        if (g.einmal == nullptr || g.einmalPlatz < 8) {
            dummy.resize(8);
            if (!fuellen(g.einmal, g.einmalPlatz, dummy)) { return; }
        }
        D3D11_MAPPED_SUBRESOURCE vm{};
        if (FAILED(c->Map(g.einmal, 0, D3D11_MAP_WRITE_DISCARD, 0, &vm))) { return; }
        std::memcpy(vm.pData, p, n * sizeof(Punkt));
        c->Unmap(g.einmal, 0);
        ID3D11BlendState* b = pass == 0 ? g.blendRechteck[0] : pass == 1 ? g.blendRechteck[1] : g.blend;
        if (pass == 3) { b = nullptr; }   // glDisable( GL_BLEND )
        c->OMSetBlendState(b, faktor, 0xFFFFFFFFU);
        c->IASetVertexBuffers(0, 1, &g.einmal, &schritt, &versatz);
        c->IASetPrimitiveTopology(pass < 3 ? D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST : D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        c->Draw(static_cast<UINT>(n), 0);
    }
}

void zeichnen(const std::vector<karte::Linie>& linien, const float* matrix, float breite) {
    if (g.aktiv < 0 || linien.empty()) { return; }
    if (!fuellen(g.einmal, g.einmalPlatz, linien)) { return; }
    ziehen(g.einmal, linien.size(), matrix, breite);
}

void beenden() {
    if (g.aktiv < 0) { return; }
    ID3D11DeviceContext* c = kontext();
    Ziel& z = g.ziele[g.aktiv];
    if (z.msaa > 1) {
        c->ResolveSubresource(z.aufgeloest, 0, z.tex, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    } else {
        c->CopyResource(z.aufgeloest, z.tex);
    }
    c->OMSetRenderTargets(1, &g.altRtv, g.altDsv);
    if (g.altVpAnzahl > 0) { c->RSSetViewports(1, &g.altVp); }
    los(g.altRtv);
    los(g.altDsv);
    g.aktiv = -1;
}

void* textur(int kennung) {
    const auto it = g.ziele.find(kennung);
    if (it == g.ziele.end() || g.stand != render::texturStand()) { return nullptr; }
    return it->second.srv;
}

void freigeben() {
    allesLoesen();
    for (auto& s : g_spaeter) { s.first->Release(); }
    g_spaeter.clear();
}

// --- 3D (Kamera) ---------------------------------------------------------------

namespace {

struct Konstanten3 {
    float mvp[16];
    float farbe[4];
    float vpnLicht[4];     // xyz Lichtrichtung, w Licht an
    float augeHimmel[4];   // xyz Auge, w Himmel
    float alpha[4];        // x Funktion, y Bezug, z Bild, w Wegschneiden (Linien)
    float versatz[4];      // x Faktor, y Einheiten, z an, w r
    float muster[4];       // x Muster, y Faktor, z/w Bildgroesse
    float linie[4];        // x Breite
    float nebel[4];        // rgb Farbe, a Dichte (GL_EXP; 0 = aus)
};

// renderstate.cpp: Licht (abgeleitet aus Cam_Draw: Umgebung 0,2 + 0,4,
// diffus 0,4 aus Blickrichtung), GL_MODULATE, Alpha-Test, Skybox
// (gl/skybox_vp.glsl, skybox_fp.glsl), glPolygonOffset und glLineStipple.
const char* const kShader3 = R"(
cbuffer B3 : register(b0) {
    float4x4 mvp; float4 farbe; float4 vpnLicht; float4 augeHimmel; float4 alpha; float4 versatz; float4 muster; float4 linie; float4 nebel;
};
Texture2D bild : register(t0);
TextureCube wuerfel : register(t1);
SamplerState abtaster : register(s0);
SamplerState wuerfelAbtaster : register(s1);
float licht(float3 n) { return saturate(0.6 + 0.4 * max(0.0, dot(normalize(n), vpnLicht.xyz))); }
float4 clip3(float3 p) { float4 c = mul(mvp, float4(p, 1.0)); c.z = (c.z + c.w) * 0.5; return c; }
struct VI3 { float3 pos : POSITION; float3 n : NORMAL; float2 st : TEXCOORD0; float4 farbe : COLOR; };
struct VO3 { float4 pos : SV_Position; float4 farbe : COLOR; float2 st : TEXCOORD0; float3 richtung : TEXCOORD1; float tiefe : TEXCOORD2; };
// GL_FOG_MODE GL_EXP: C = f * C + (1 - f) * Nebelfarbe, f = exp(-Dichte * Abstand)
float3 genebelt(float3 c, float abstand) {
    if (nebel.w <= 0.0) { return c; }
    float f = saturate(exp(-nebel.w * abs(abstand)));
    return lerp(nebel.rgb, c, f);
}
VO3 vs3(VI3 i) {
    VO3 o;
    o.pos = clip3(i.pos);
    float4 c = i.farbe * farbe;
    if (vpnLicht.w > 0.5) { c.rgb *= licht(i.n); }
    o.farbe = c;
    o.st = i.st;
    o.richtung = i.pos - augeHimmel.xyz;
    o.tiefe = o.pos.w;
    return o;
}
bool alphaGut(float a) {
    int f = (int)alpha.x;
    float r = alpha.y;
    if (f == 1) { return a == r; }
    if (f == 2) { return a < r; }
    if (f == 3) { return a > r; }
    if (f == 4) { return a <= r; }
    if (f == 5) { return a >= r; }
    return true;
}
float4 ps3(VO3 i) : SV_Target {
    if (augeHimmel.w > 0.5) {
        // doing rotation/flip to fix skybox orientation
        return wuerfel.Sample(wuerfelAbtaster, float3(-i.richtung.y, i.richtung.z, i.richtung.x));
    }
    float4 c = i.farbe;
    if (alpha.z > 0.5) { c *= bild.Sample(abtaster, i.st); }
    c.rgb = genebelt(c.rgb, i.tiefe);
    if (!alphaGut(c.a)) { discard; }
    return c;
}
struct VIL { float3 pos : POSITION; float4 farbe : COLOR; float4 extra : TEXCOORD0; };
struct VOL { float4 pos : SV_Position; float4 farbe : COLOR; float3 welt : TEXCOORD0; float4 extra : TEXCOORD1; float4 dritte : TEXCOORD2; };
VOL vsl(VIL i) {
    VOL o;
    o.pos = clip3(i.pos);
    float4 c = i.farbe * farbe;
    if (vpnLicht.w > 0.5 && i.extra.w < 0.5) { c.rgb *= licht(i.extra.xyz); }
    o.farbe = c;
    o.welt = i.pos;
    o.extra = i.extra;
    o.dritte = i.extra.w > 0.5 ? clip3(i.extra.xyz) : float4(0.0, 0.0, 0.0, 1.0);
    return o;
}
struct GOL { float4 pos : SV_Position; float4 farbe : COLOR; nointerpolation float2 start : TEXCOORD0; float tiefe : TEXCOORD1; };
float3 fenster(float4 c) {
    float2 h = muster.zw * 0.5;
    return float3((c.x / c.w + 1.0) * h.x, (1.0 - c.y / c.w) * h.y, c.z / c.w);
}
[maxvertexcount(2)]
void gsl(line VOL e[2], inout LineStream<GOL> s) {
    float o = 0.0;
    if (e[0].extra.w > 0.5) {
        float3 a = e[0].welt;
        float3 n = cross(e[1].welt - a, e[0].extra.xyz - a);
        // glFrontFace( GL_CW ), glCullFace( GL_BACK )
        if (alpha.w > 0.5 && dot(n, augeHimmel.xyz - a) >= 0.0) { return; }
        if (versatz.z > 0.5 && e[0].pos.w > 0.0 && e[1].pos.w > 0.0 && e[0].dritte.w > 0.0) {
            float3 p0 = fenster(e[0].pos);
            float3 m = cross(fenster(e[1].pos) - p0, fenster(e[0].dritte) - p0);
            float steigung = abs(m.z) > 1e-20 ? max(abs(m.x / m.z), abs(m.y / m.z)) : 0.0;
            o = versatz.x * steigung + versatz.y * versatz.w;
        }
    }
    float2 st = e[0].pos.w > 0.0 ? fenster(e[0].pos).xy : fenster(e[1].pos).xy;
    for (int k = 0; k < 2; ++k) {
        GOL v;
        v.pos = e[k].pos;
        v.pos.z += o * v.pos.w;
        v.farbe = e[k].farbe;
        v.start = st;
        v.tiefe = e[k].pos.w;
        s.Append(v);
    }
}
// glLineWidth > 1: dasselbe wie gsl, die Linie als Viereck breite Bildpunkte breit
[maxvertexcount(4)]
void gslBreit(line VOL e[2], inout TriangleStream<GOL> s) {
    float o = 0.0;
    if (e[0].extra.w > 0.5) {
        float3 a = e[0].welt;
        float3 n = cross(e[1].welt - a, e[0].extra.xyz - a);
        if (alpha.w > 0.5 && dot(n, augeHimmel.xyz - a) >= 0.0) { return; }
        if (versatz.z > 0.5 && e[0].pos.w > 0.0 && e[1].pos.w > 0.0 && e[0].dritte.w > 0.0) {
            float3 p0 = fenster(e[0].pos);
            float3 m = cross(fenster(e[1].pos) - p0, fenster(e[0].dritte) - p0);
            float steigung = abs(m.z) > 1e-20 ? max(abs(m.x / m.z), abs(m.y / m.z)) : 0.0;
            o = versatz.x * steigung + versatz.y * versatz.w;
        }
    }
    float2 st = e[0].pos.w > 0.0 ? fenster(e[0].pos).xy : fenster(e[1].pos).xy;
    float2 h = muster.zw * 0.5;
    float2 pa = e[0].pos.xy / e[0].pos.w * h;
    float2 pb = e[1].pos.xy / e[1].pos.w * h;
    float2 d = pb - pa;
    float l = length(d);
    d = (l > 1e-6) ? d / l : float2(1.0, 0.0);
    float2 q = float2(-d.y, d.x) * (linie.x * 0.5) / h;
    for (int k = 0; k < 2; ++k) {
        for (int seite = 0; seite < 2; ++seite) {
            GOL v;
            v.pos = e[k].pos;
            v.pos.z += o * v.pos.w;
            v.pos.xy += (seite == 0 ? q : -q) * e[k].pos.w;
            v.farbe = e[k].farbe;
            v.start = st;
            v.tiefe = e[k].pos.w;
            s.Append(v);
        }
    }
}
float4 psl(GOL i) : SV_Target {
    if (muster.x > 0.5) {
        float2 d = abs(i.pos.xy - i.start);
        uint zaehler = (uint)floor(max(d.x, d.y));
        uint bit = (zaehler / (uint)muster.y) & 15u;
        if (((((uint)muster.x) >> bit) & 1u) == 0u) { discard; }
    }
    float4 c = i.farbe;
    c.rgb = genebelt(c.rgb, i.tiefe);
    return c;
}
)";

bool bereit3() {
    if (!bereit()) { return false; }
    if (g.vs3 != nullptr) { return true; }
    if (g.gescheitert3) { return false; }
    g.gescheitert3 = true;
    ID3D11Device* d = geraet();
    const auto uebersetzen = [](const char* einstieg, const char* ziel, ID3DBlob** code) {
        ID3DBlob* fehler = nullptr;
        const HRESULT hr = D3DCompile(kShader3, std::strlen(kShader3), "karte3", nullptr, nullptr, einstieg, ziel,
                                      D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, code, &fehler);
        los(fehler);
        return SUCCEEDED(hr);
    };
    ID3DBlob* b[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    if (!uebersetzen("vs3", "vs_4_0", &b[0]) || !uebersetzen("ps3", "ps_4_0", &b[1]) || !uebersetzen("vsl", "vs_4_0", &b[2]) ||
        !uebersetzen("gsl", "gs_4_0", &b[3]) || !uebersetzen("psl", "ps_4_0", &b[4]) || !uebersetzen("gslBreit", "gs_4_0", &b[5])) {
        for (auto*& x : b) { los(x); }
        return false;
    }
    d->CreateVertexShader(b[0]->GetBufferPointer(), b[0]->GetBufferSize(), nullptr, &g.vs3);
    d->CreatePixelShader(b[1]->GetBufferPointer(), b[1]->GetBufferSize(), nullptr, &g.ps3);
    d->CreateVertexShader(b[2]->GetBufferPointer(), b[2]->GetBufferSize(), nullptr, &g.vsl);
    d->CreateGeometryShader(b[3]->GetBufferPointer(), b[3]->GetBufferSize(), nullptr, &g.gsl);
    d->CreatePixelShader(b[4]->GetBufferPointer(), b[4]->GetBufferSize(), nullptr, &g.psl);
    d->CreateGeometryShader(b[5]->GetBufferPointer(), b[5]->GetBufferSize(), nullptr, &g.gslBreit);
    const D3D11_INPUT_ELEMENT_DESC e3[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    d->CreateInputLayout(e3, 4, b[0]->GetBufferPointer(), b[0]->GetBufferSize(), &g.layout3);
    const D3D11_INPUT_ELEMENT_DESC el[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    d->CreateInputLayout(el, 3, b[2]->GetBufferPointer(), b[2]->GetBufferSize(), &g.layoutl);
    for (auto*& x : b) { los(x); }
    D3D11_BUFFER_DESC cb{};
    cb.ByteWidth = sizeof(Konstanten3);
    cb.Usage = D3D11_USAGE_DYNAMIC;
    cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    d->CreateBuffer(&cb, nullptr, &g.cb3);
    D3D11_BLEND_DESC bd{};
    bd.RenderTarget[0].BlendEnable = FALSE;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    d->CreateBlendState(&bd, &g.blendAus);
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    d->CreateBlendState(&bd, &g.blendAdd);
    for (int i = 0; i < 4; ++i) {
        D3D11_RASTERIZER_DESC rd{};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = (i & 1) != 0 ? D3D11_CULL_BACK : D3D11_CULL_NONE;
        rd.FrontCounterClockwise = FALSE;   // glFrontFace( GL_CW )
        rd.DepthClipEnable = TRUE;
        rd.MultisampleEnable = (i & 2) != 0 ? TRUE : FALSE;
        d->CreateRasterizerState(&rd, &g.rs3[i]);
    }
    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_ANISOTROPIC;   // GL_LINEAR_MIPMAP_LINEAR + GL_TEXTURE_MAX_ANISOTROPY_EXT
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.MaxAnisotropy = 16;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    d->CreateSamplerState(&sd, &g.abtaster);
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxAnisotropy = 1;
    sd.MaxLOD = 0;
    d->CreateSamplerState(&sd, &g.wuerfelAbtaster);
    g.gescheitert3 = g.vs3 == nullptr || g.ps3 == nullptr || g.vsl == nullptr || g.gsl == nullptr || g.psl == nullptr ||
                     g.layout3 == nullptr || g.layoutl == nullptr || g.cb3 == nullptr || g.abtaster == nullptr;
    return !g.gescheitert3;
}

ID3D11DepthStencilState* tiefenZustand(TiefenTest t, bool schreiben) {
    const int schl = (static_cast<int>(t) * 2) + (schreiben ? 1 : 0);
    const auto it = g.tiefen.find(schl);
    if (it != g.tiefen.end()) { return it->second; }
    D3D11_DEPTH_STENCIL_DESC dd{};
    dd.DepthEnable = t != TiefenTest::Aus ? TRUE : FALSE;
    dd.DepthWriteMask = schreiben ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    dd.DepthFunc = t == TiefenTest::KleinerGleich ? D3D11_COMPARISON_LESS_EQUAL
                   : t == TiefenTest::Groesser    ? D3D11_COMPARISON_GREATER
                                                  : D3D11_COMPARISON_LESS;
    dd.StencilEnable = FALSE;
    ID3D11DepthStencilState* z = nullptr;
    geraet()->CreateDepthStencilState(&dd, &z);
    g.tiefen[schl] = z;
    return z;
}

bool bereit4();

// Zustand und Konstanten setzen; false: nichts zu zeichnen.
bool zustandSetzen(const Zustand3& z, bool linie) {
    if (g.aktiv < 0 || !bereit3() || z.matrix == nullptr) { return false; }
    ID3D11DeviceContext* c = kontext();
    const Ziel& ziel = g.ziele[g.aktiv];
    D3D11_MAPPED_SUBRESOURCE m{};
    if (FAILED(c->Map(g.cb3, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) { return false; }
    auto* k = static_cast<Konstanten3*>(m.pData);
    std::memcpy(k->mvp, z.matrix, sizeof(k->mvp));
    std::memcpy(k->farbe, z.farbe, sizeof(k->farbe));
    k->vpnLicht[0] = z.vpn[0];
    k->vpnLicht[1] = z.vpn[1];
    k->vpnLicht[2] = z.vpn[2];
    k->vpnLicht[3] = z.licht ? 1.0F : 0.0F;
    k->augeHimmel[0] = z.auge[0];
    k->augeHimmel[1] = z.auge[1];
    k->augeHimmel[2] = z.auge[2];
    k->augeHimmel[3] = (!linie && z.himmel != nullptr) ? 1.0F : 0.0F;
    k->alpha[0] = static_cast<float>(z.alphaFunktion);
    k->alpha[1] = z.alphaRef;
    k->alpha[2] = z.bild != nullptr ? 1.0F : 0.0F;
    k->alpha[3] = z.wegschneiden ? 1.0F : 0.0F;
    k->versatz[0] = z.versatzFaktor;
    k->versatz[1] = z.versatzEinheiten;
    k->versatz[2] = z.versatz ? 1.0F : 0.0F;
    k->versatz[3] = 1.0F / 16777215.0F;   // r fuer GL_DEPTH_COMPONENT24
    k->muster[0] = static_cast<float>(z.muster);
    k->muster[1] = static_cast<float>(std::max(1, z.musterFaktor));
    k->muster[2] = static_cast<float>(ziel.breite);
    k->muster[3] = static_cast<float>(ziel.hoehe);
    k->linie[0] = z.breite;
    k->linie[1] = 0;
    k->linie[2] = 0;
    k->linie[3] = 0;
    std::memcpy(k->nebel, z.nebel, sizeof(k->nebel));
    c->Unmap(g.cb3, 0);
    const float faktor[4] = {0, 0, 0, 0};
    c->OMSetBlendState(z.mischen == Mischen::Aus ? g.blendAus : z.mischen == Mischen::Alpha ? g.blend : g.blendAdd, faktor, 0xFFFFFFFFU);
    c->OMSetDepthStencilState(tiefenZustand(z.tiefe, z.tiefeSchreiben), 0);
    c->RSSetState(g.rs3[((ziel.msaa > 1) ? 2 : 0) + ((!linie && z.wegschneiden) ? 1 : 0)]);
    if (linie) {
        c->IASetInputLayout(g.layoutl);
        c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        c->VSSetShader(g.vsl, nullptr, 0);
        c->GSSetShader(z.breite > 1.0F && g.gslBreit != nullptr ? g.gslBreit : g.gsl, nullptr, 0);
        c->PSSetShader(g.psl, nullptr, 0);
        c->VSSetConstantBuffers(0, 1, &g.cb3);
        c->GSSetConstantBuffers(0, 1, &g.cb3);
        c->PSSetConstantBuffers(0, 1, &g.cb3);
    } else if (z.echtLicht && !linie && z.himmel == nullptr && bereit4()) {
        // Stufe 7: Echtzeit-Licht
        c->IASetInputLayout(g.layout3);
        c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        c->VSSetShader(g.vs4, nullptr, 0);
        c->GSSetShader(nullptr, nullptr, 0);
        c->PSSetShader(g.ps4, nullptr, 0);
        ID3D11Buffer* cbs[2] = {g.cb3, g.cb4};
        c->VSSetConstantBuffers(0, 2, cbs);
        c->PSSetConstantBuffers(0, 2, cbs);
        ID3D11ShaderResourceView* srv[4] = {static_cast<ID3D11ShaderResourceView*>(z.bild), nullptr, g.wuerfelSrv, g.sonneSrv};
        c->PSSetShaderResources(0, 4, srv);
        ID3D11SamplerState* sm[3] = {g.abtaster, g.wuerfelAbtaster, g.punktAbtaster};
        c->PSSetSamplers(0, 3, sm);
    } else {
        c->IASetInputLayout(g.layout3);
        c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        c->VSSetShader(g.vs3, nullptr, 0);
        c->GSSetShader(nullptr, nullptr, 0);
        c->PSSetShader(g.ps3, nullptr, 0);
        c->VSSetConstantBuffers(0, 1, &g.cb3);
        c->PSSetConstantBuffers(0, 1, &g.cb3);
        ID3D11ShaderResourceView* srv[2] = {static_cast<ID3D11ShaderResourceView*>(z.bild),
                                            static_cast<ID3D11ShaderResourceView*>(z.himmel)};
        c->PSSetShaderResources(0, 2, srv);
        ID3D11SamplerState* sm[2] = {g.abtaster, g.wuerfelAbtaster};
        c->PSSetSamplers(0, 2, sm);
    }
    return true;
}

bool dynamischFuellen(ID3D11Buffer*& vb, std::size_t& platz, const void* daten, std::size_t bytes) {
    ID3D11Device* d = geraet();
    if (d == nullptr) { return false; }
    if (bytes > platz || vb == nullptr) {
        los(vb);
        platz = std::max<std::size_t>(bytes + bytes / 2, 64 * 1024);
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = static_cast<UINT>(platz);
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(d->CreateBuffer(&bd, nullptr, &vb))) {
            platz = 0;
            return false;
        }
    }
    if (bytes == 0) { return true; }
    D3D11_MAPPED_SUBRESOURCE m{};
    if (FAILED(kontext()->Map(vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) { return false; }
    std::memcpy(m.pData, daten, bytes);
    kontext()->Unmap(vb, 0);
    return true;
}

void ziehen3(ID3D11Buffer* vb, UINT schritt, std::size_t erstes, std::size_t anzahl, const Zustand3& z, bool linie) {
    if (vb == nullptr || anzahl == 0 || !zustandSetzen(z, linie)) { return; }
    ID3D11DeviceContext* c = kontext();
    const UINT versatz = 0;
    c->IASetVertexBuffers(0, 1, &vb, &schritt, &versatz);
    c->Draw(static_cast<UINT>(anzahl), static_cast<UINT>(erstes));
    c->GSSetShader(nullptr, nullptr, 0);
}

}  // namespace

struct Zugriff3 {
    static ID3D11Buffer* vb(const Puffer3& p) {
        return p.m_stand == render::texturStand() ? static_cast<ID3D11Buffer*>(p.m_vb) : nullptr;
    }
};

Puffer3::~Puffer3() {
    if (m_vb != nullptr && m_stand == render::texturStand()) { static_cast<ID3D11Buffer*>(m_vb)->Release(); }
}

bool Puffer3::gueltig() const { return m_vb != nullptr && m_stand == render::texturStand(); }

void Puffer3::setzen(const void* daten, std::size_t bytes) {
    if (!bereit()) { return; }
    if (m_stand != render::texturStand()) {
        m_vb = nullptr;
        m_platz = 0;
        m_stand = render::texturStand();
    }
    auto* vb = static_cast<ID3D11Buffer*>(m_vb);
    // Neu anlegen, wenn er zu klein ist - oder viel zu gross (Punkt 12: nach
    // dem Drahtmodell blieb der Linienpuffer sonst in voller Groesse).
    if (vb == nullptr || bytes > m_platz || bytes < m_platz / 2) {
        los(vb);
        m_vb = nullptr;
        m_platz = 0;
        if (bytes == 0) { return; }
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = static_cast<UINT>(bytes);
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = daten;
        if (FAILED(geraet()->CreateBuffer(&bd, &sd, &vb))) { return; }
        m_platz = bytes;
        m_vb = vb;
        return;
    }
    if (bytes > 0) {
        D3D11_BOX box{0, 0, 0, static_cast<UINT>(bytes), 1, 1};
        kontext()->UpdateSubresource(vb, 0, &box, daten, 0, 0);
    }
}

void dreiecke(const Puffer3& p, std::size_t erstes, std::size_t anzahl, const Zustand3& z) {
    ziehen3(Zugriff3::vb(p), sizeof(Punkt3), erstes, anzahl, z, false);
}

void linien(const Puffer3& p, std::size_t erstes, std::size_t anzahl, const Zustand3& z) {
    ziehen3(Zugriff3::vb(p), sizeof(LinienPunkt), erstes, anzahl, z, true);
}

void dreiecke(const std::vector<Punkt3>& p, const Zustand3& z) {
    if (p.empty() || g.aktiv < 0) { return; }
    if (!dynamischFuellen(g.einmal3, g.einmal3Platz, p.data(), p.size() * sizeof(Punkt3))) { return; }
    ziehen3(g.einmal3, sizeof(Punkt3), 0, p.size(), z, false);
}

void linien(const std::vector<LinienPunkt>& p, const Zustand3& z) {
    if (p.empty() || g.aktiv < 0) { return; }
    if (!dynamischFuellen(g.einmalL, g.einmalLPlatz, p.data(), p.size() * sizeof(LinienPunkt))) { return; }
    ziehen3(g.einmalL, sizeof(LinienPunkt), 0, p.size(), z, true);
}


// --- Stufe 7: Echtzeit-Licht ----------------------------------------------------------
//
// Die Formeln von LightContributionToSample (q3map2 light.cpp:777-1177, siehe
// bhed/karte/echtlicht.cpp echtLichtBeitrag - der Unit-Test karte_echtlicht
// prueft dieselben Werte auf dem Prozessor). Der Luxel wie StoreSurfaceLightmaps/
// ColorToBytes: Umgebung + Summe, mindestens _minlight, ueber 255 auf 255
// normiert; im Spiel mit der Textur * 2 (r_mapOverBrightBits). Vorschau: kein
// Bounce, kein Filter, kein Supersampling, Schatten aus Wuerfeln statt Strahlen.
namespace {

struct Konstanten4 {
    float umgebung[4];
    float minimum[4];
    float sonne[4];      // x an
    float sonneVp[16];   // zeilenweise
    int anzahl[4];
    LichtGpu lichter[kLichtMax];
};
struct KonstantenSchatten {
    float vp[16];        // zeilenweise
    float licht[4];      // xyz Ursprung, w Weite
};

const char* const kShader4 = R"(
cbuffer B3 : register(b0) {
    float4x4 mvp; float4 farbe; float4 vpnLicht; float4 augeHimmel; float4 alpha; float4 versatz; float4 muster; float4 linie; float4 nebel;
};
struct LichtD { float4 pos; float4 farbe; float4 richtung; float4 werte; float4 mehr; };
cbuffer B4 : register(b1) {
    float4 umgebung; float4 minimum; float4 sonne; row_major float4x4 sonneVp; int4 anzahl; LichtD lichter[64];
};
Texture2D bild : register(t0);
TextureCubeArray schattenW : register(t2);
Texture2D sonneK : register(t3);
SamplerState abtaster : register(s0);
SamplerState punktAbtaster : register(s2);
float4 clip3(float3 p) { float4 c = mul(mvp, float4(p, 1.0)); c.z = (c.z + c.w) * 0.5; return c; }
struct VI3 { float3 pos : POSITION; float3 n : NORMAL; float2 st : TEXCOORD0; float4 farbe : COLOR; };
struct VO4 { float4 pos : SV_Position; float4 farbe : COLOR; float2 st : TEXCOORD0; float3 welt : TEXCOORD1; float3 n : TEXCOORD2; float tiefe : TEXCOORD3; };
VO4 vs4(VI3 i) {
    VO4 o;
    o.pos = clip3(i.pos);
    o.farbe = i.farbe * farbe;
    o.st = i.st;
    o.welt = i.pos;
    o.n = i.n;
    o.tiefe = o.pos.w;
    return o;
}
bool alphaGut(float a) {
    int f = (int)alpha.x;
    float r = alpha.y;
    if (f == 1) { return a == r; }
    if (f == 2) { return a < r; }
    if (f == 3) { return a > r; }
    if (f == 4) { return a <= r; }
    if (f == 5) { return a >= r; }
    return true;
}
float3 genebelt(float3 c, float abstand) {
    if (nebel.w <= 0.0) { return c; }
    float f = saturate(exp(-nebel.w * abs(abstand)));
    return lerp(nebel.rgb, c, f);
}
// LightContributionToSample ohne Strahl
float beitrag(LichtD L, float3 p, float3 n) {
    int art = (int)L.farbe.a;
    if (art == 2) {
        float w = L.werte.y > 0.5 ? dot(n, L.richtung.xyz) : 1.0;
        return max(0.0, L.pos.w * w);
    }
    if (dot(L.pos.xyz, n) - dot(p, n) < 0.0) { return 0.0; }
    float3 d = L.pos.xyz - p;
    float abstand = length(d);
    if (abstand >= L.werte.w) { return 0.0; }
    float3 richtung = abstand > 0.0 ? d / abstand : float3(0, 0, 1);
    abstand = max(16.0, sqrt(abstand * abstand + L.mehr.w * L.mehr.w));
    float w = 1.0;
    if (L.werte.y > 0.5) { w = dot(n, richtung); }
    if (L.mehr.z != 0.0) { w = min(w / L.mehr.z, 1.0); }
    float add;
    if (art == 3) {
        w *= -dot(L.richtung.xyz, richtung);
        if (w <= 0.0) { return 0.0; }
        add = L.pos.w / (abstand * abstand) * w;
    } else if (L.werte.x > 0.5) {
        add = max(0.0, w * L.pos.w * (1.0 / 8000.0) - abstand * L.werte.z);
    } else {
        add = max(0.0, (L.pos.w / (abstand * abstand)) * w);
    }
    if (art == 1) {
        float entlang = dot(p - L.pos.xyz, L.richtung.xyz);
        if (entlang < 0.0) { return 0.0; }
        float3 dort = L.pos.xyz + L.richtung.xyz * entlang;
        float rd = L.richtung.w * entlang;
        float r = length(p - dort);
        if (r >= rd) { return 0.0; }
        if (r > rd - 32.0) { add = max(0.0, add * ((rd - r) / 32.0)); }
    }
    if (add <= 1.0) { return 0.0; }
    return add;
}
float schatten(LichtD L, float3 p, float3 n) {
    int art = (int)L.farbe.a;
    if (art == 2) {
        if (sonne.x < 0.5) { return 1.0; }
        float4 c = mul(float4(p + n * 2.0, 1.0), sonneVp);
        float2 uv = float2(c.x * 0.5 + 0.5, 0.5 - c.y * 0.5);
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) { return 1.0; }
        float gespeichert = sonneK.SampleLevel(punktAbtaster, uv, 0).r;
        return c.z - 0.002 <= gespeichert ? 1.0 : 0.0;
    }
    if (L.mehr.x < 0.0) { return 1.0; }
    float3 d = (p + n * 1.0) - L.pos.xyz;
    float abstand = length(d);
    float gespeichert = schattenW.SampleLevel(punktAbtaster, float4(d, L.mehr.x), 0).r * L.mehr.y;
    return abstand - 3.0 <= gespeichert ? 1.0 : 0.0;
}
float4 ps4(VO4 i) : SV_Target {
    float3 n = normalize(i.n);
    float3 lm = umgebung.rgb;
    [loop] for (int k = 0; k < anzahl.x; ++k) {
        LichtD L = lichter[k];
        float add = beitrag(L, i.welt, n);
        if (add > 0.0) {
            float s = schatten(L, i.welt, n);
            lm += L.farbe.rgb * add * s * (L.mehr.x < -1.5 ? -1.0 : 1.0);
        }
    }
    lm = max(lm, minimum.rgb);
    float m = max(lm.r, max(lm.g, lm.b));
    if (m > 255.0) { lm *= 255.0 / m; }
    float4 c = i.farbe;
    if (alpha.z > 0.5) { c *= bild.Sample(abtaster, i.st); }
    c.rgb = saturate(c.rgb * (lm / 255.0) * 2.0);
    c.rgb = genebelt(c.rgb, i.tiefe);
    if (!alphaGut(c.a)) { discard; }
    return c;
}
cbuffer BS : register(b2) { row_major float4x4 seiteVp; float4 lichtPos; };
struct VOS { float4 pos : SV_Position; float3 welt : TEXCOORD0; };
VOS vss(VI3 i) {
    VOS o;
    o.pos = mul(float4(i.pos, 1.0), seiteVp);
    o.welt = i.pos;
    return o;
}
float pss(VOS i) : SV_Target { return length(i.welt - lichtPos.xyz) / lichtPos.w; }
float pssonne(VOS i) : SV_Target { return i.pos.z; }
)";

bool bereit4() {
    if (!bereit3()) { return false; }
    if (g.vs4 != nullptr) { return true; }
    if (g.gescheitert4) { return false; }
    g.gescheitert4 = true;
    ID3D11Device* d = geraet();
    if (d->GetFeatureLevel() < D3D_FEATURE_LEVEL_10_1) { return false; }   // TextureCubeArray
    const auto uebersetzen = [](const char* einstieg, const char* ziel, ID3DBlob** code) {
        ID3DBlob* fehler = nullptr;
        const HRESULT hr = D3DCompile(kShader4, std::strlen(kShader4), "karte4", nullptr, nullptr, einstieg, ziel,
                                      D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, code, &fehler);
        los(fehler);
        return SUCCEEDED(hr);
    };
    ID3DBlob* b[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    if (!uebersetzen("vs4", "vs_4_1", &b[0]) || !uebersetzen("ps4", "ps_4_1", &b[1]) || !uebersetzen("vss", "vs_4_1", &b[2]) ||
        !uebersetzen("pss", "ps_4_1", &b[3]) || !uebersetzen("pssonne", "ps_4_1", &b[4])) {
        for (auto*& x : b) { los(x); }
        return false;
    }
    d->CreateVertexShader(b[0]->GetBufferPointer(), b[0]->GetBufferSize(), nullptr, &g.vs4);
    d->CreatePixelShader(b[1]->GetBufferPointer(), b[1]->GetBufferSize(), nullptr, &g.ps4);
    d->CreateVertexShader(b[2]->GetBufferPointer(), b[2]->GetBufferSize(), nullptr, &g.vsSchatten);
    d->CreatePixelShader(b[3]->GetBufferPointer(), b[3]->GetBufferSize(), nullptr, &g.psSchatten);
    d->CreatePixelShader(b[4]->GetBufferPointer(), b[4]->GetBufferSize(), nullptr, &g.psSonne);
    for (auto*& x : b) { los(x); }
    D3D11_BUFFER_DESC cb{};
    cb.ByteWidth = sizeof(Konstanten4);
    cb.Usage = D3D11_USAGE_DYNAMIC;
    cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    d->CreateBuffer(&cb, nullptr, &g.cb4);
    cb.ByteWidth = sizeof(KonstantenSchatten);
    d->CreateBuffer(&cb, nullptr, &g.cbSchatten);
    // Schattenwuerfel: 6 * kSchattenMax Seiten je 256 x 256 (Abstand / Weite, R16F)
    D3D11_TEXTURE2D_DESC td{};
    td.Width = 256;
    td.Height = 256;
    td.MipLevels = 1;
    td.ArraySize = 6 * kSchattenMax;
    td.Format = DXGI_FORMAT_R16_FLOAT;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    td.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    if (FAILED(d->CreateTexture2D(&td, nullptr, &g.wuerfelTex))) { return false; }
    for (int i = 0; i < 6 * kSchattenMax; ++i) {
        D3D11_RENDER_TARGET_VIEW_DESC rd{};
        rd.Format = td.Format;
        rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rd.Texture2DArray.FirstArraySlice = static_cast<UINT>(i);
        rd.Texture2DArray.ArraySize = 1;
        d->CreateRenderTargetView(g.wuerfelTex, &rd, &g.wuerfelRtv[i]);
        const float eins[4] = {1, 1, 1, 1};
        if (g.wuerfelRtv[i] != nullptr) { kontext()->ClearRenderTargetView(g.wuerfelRtv[i], eins); }
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC sv{};
    sv.Format = td.Format;
    sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
    sv.TextureCubeArray.MipLevels = 1;
    sv.TextureCubeArray.NumCubes = kSchattenMax;
    d->CreateShaderResourceView(g.wuerfelTex, &sv, &g.wuerfelSrv);
    td.ArraySize = 1;
    td.MiscFlags = 0;
    td.Format = DXGI_FORMAT_D32_FLOAT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    d->CreateTexture2D(&td, nullptr, &g.wuerfelTiefe);
    if (g.wuerfelTiefe != nullptr) { d->CreateDepthStencilView(g.wuerfelTiefe, nullptr, &g.wuerfelDsv); }
    // Sonne: 2048 x 2048, Tiefe (R32F)
    td.Width = 2048;
    td.Height = 2048;
    td.Format = DXGI_FORMAT_R32_FLOAT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    d->CreateTexture2D(&td, nullptr, &g.sonneTex);
    if (g.sonneTex != nullptr) {
        d->CreateRenderTargetView(g.sonneTex, nullptr, &g.sonneRtv);
        d->CreateShaderResourceView(g.sonneTex, nullptr, &g.sonneSrv);
    }
    td.Format = DXGI_FORMAT_D32_FLOAT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    d->CreateTexture2D(&td, nullptr, &g.sonneTiefe);
    if (g.sonneTiefe != nullptr) { d->CreateDepthStencilView(g.sonneTiefe, nullptr, &g.sonneDsv); }
    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    d->CreateSamplerState(&sd, &g.punktAbtaster);
    D3D11_RASTERIZER_DESC rs{};
    rs.FillMode = D3D11_FILL_SOLID;
    rs.CullMode = D3D11_CULL_NONE;
    rs.DepthClipEnable = TRUE;
    d->CreateRasterizerState(&rs, &g.rsSchatten);
    g.gescheitert4 = g.vs4 == nullptr || g.ps4 == nullptr || g.vsSchatten == nullptr || g.psSchatten == nullptr || g.psSonne == nullptr ||
                     g.cb4 == nullptr || g.cbSchatten == nullptr || g.wuerfelSrv == nullptr || g.wuerfelDsv == nullptr ||
                     g.sonneSrv == nullptr || g.sonneDsv == nullptr || g.punktAbtaster == nullptr || g.rsSchatten == nullptr;
    return !g.gescheitert4;
}

// Zeilenweise Matrizen (wie row_major im Shader: clip = p * M)
void blickLH(const float* auge, const float* vorn, const float* oben, float* m) {
    const auto norm = [](float* v) {
        const float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (l > 0) { v[0] /= l, v[1] /= l, v[2] /= l; }
    };
    float z[3] = {vorn[0], vorn[1], vorn[2]};
    norm(z);
    float x[3] = {oben[1] * z[2] - oben[2] * z[1], oben[2] * z[0] - oben[0] * z[2], oben[0] * z[1] - oben[1] * z[0]};
    norm(x);
    const float y[3] = {z[1] * x[2] - z[2] * x[1], z[2] * x[0] - z[0] * x[2], z[0] * x[1] - z[1] * x[0]};
    const float m0[16] = {x[0], y[0], z[0], 0,
                          x[1], y[1], z[1], 0,
                          x[2], y[2], z[2], 0,
                          -(x[0] * auge[0] + x[1] * auge[1] + x[2] * auge[2]), -(y[0] * auge[0] + y[1] * auge[1] + y[2] * auge[2]),
                          -(z[0] * auge[0] + z[1] * auge[1] + z[2] * auge[2]), 1};
    std::memcpy(m, m0, sizeof(m0));
}
void mal(const float* a, const float* b, float* r) {   // r = a * b (zeilenweise)
    float t[16];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            t[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + a[i * 4 + 3] * b[12 + j];
        }
    }
    std::memcpy(r, t, sizeof(t));
}

struct RtSicherung {
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    D3D11_VIEWPORT vp{};
    UINT vpAnzahl = 1;
    RtSicherung() {
        kontext()->OMGetRenderTargets(1, &rtv, &dsv);
        kontext()->RSGetViewports(&vpAnzahl, &vp);
    }
    ~RtSicherung() {
        kontext()->OMSetRenderTargets(1, &rtv, dsv);
        if (vpAnzahl > 0) { kontext()->RSSetViewports(1, &vp); }
        los(rtv);
        los(dsv);
    }
    RtSicherung(const RtSicherung&) = delete;
    RtSicherung& operator=(const RtSicherung&) = delete;
};

void schattenZiehen(const float* vp, const float* licht, ID3D11PixelShader* ps, const std::vector<SchattenTeil>& teile) {
    ID3D11DeviceContext* c = kontext();
    D3D11_MAPPED_SUBRESOURCE m{};
    if (FAILED(c->Map(g.cbSchatten, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) { return; }
    auto* k = static_cast<KonstantenSchatten*>(m.pData);
    std::memcpy(k->vp, vp, sizeof(k->vp));
    std::memcpy(k->licht, licht, sizeof(k->licht));
    c->Unmap(g.cbSchatten, 0);
    c->IASetInputLayout(g.layout3);
    c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    c->VSSetShader(g.vsSchatten, nullptr, 0);
    c->GSSetShader(nullptr, nullptr, 0);
    c->PSSetShader(ps, nullptr, 0);
    c->VSSetConstantBuffers(2, 1, &g.cbSchatten);
    c->PSSetConstantBuffers(2, 1, &g.cbSchatten);
    const float faktor[4] = {0, 0, 0, 0};
    c->OMSetBlendState(g.blendAus, faktor, 0xFFFFFFFFU);
    c->OMSetDepthStencilState(tiefenZustand(TiefenTest::Kleiner, true), 0);
    c->RSSetState(g.rsSchatten);
    for (const SchattenTeil& t : teile) {
        if (t.puffer == nullptr || t.anzahl == 0) { continue; }
        ID3D11Buffer* vb = Zugriff3::vb(*t.puffer);
        if (vb == nullptr) { continue; }
        const UINT schritt = sizeof(Punkt3);
        const UINT versatz = 0;
        c->IASetVertexBuffers(0, 1, &vb, &schritt, &versatz);
        c->Draw(static_cast<UINT>(t.anzahl), static_cast<UINT>(t.erstes));
    }
}

}  // namespace

bool echtLichtVerfuegbar() { return verfuegbar() && bereit4(); }

void lichtSetzen(const std::vector<LichtGpu>& lichter, const float* umgebung, const float* minimum) {
    if (!bereit4()) { return; }
    ID3D11DeviceContext* c = kontext();
    D3D11_MAPPED_SUBRESOURCE m{};
    if (FAILED(c->Map(g.cb4, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) { return; }
    auto* k = static_cast<Konstanten4*>(m.pData);
    std::memset(k, 0, sizeof(Konstanten4));
    for (int i = 0; i < 3; ++i) {
        k->umgebung[i] = umgebung[i];
        k->minimum[i] = minimum[i];
    }
    k->sonne[0] = g.sonneAn ? 1.0F : 0.0F;
    std::memcpy(k->sonneVp, g.sonneVp, sizeof(k->sonneVp));
    const std::size_t n = std::min<std::size_t>(lichter.size(), kLichtMax);
    k->anzahl[0] = static_cast<int>(n);
    if (n > 0) { std::memcpy(k->lichter, lichter.data(), n * sizeof(LichtGpu)); }
    c->Unmap(g.cb4, 0);
}

bool schattenWuerfel(int slot, const float* pos, float weite, const std::vector<SchattenTeil>& teile) {
    if (slot < 0 || slot >= kSchattenMax || !bereit4() || weite <= 1.0F) { return false; }
    ID3D11DeviceContext* c = kontext();
    RtSicherung sicher;
    D3D11_VIEWPORT vp{};
    vp.Width = 256;
    vp.Height = 256;
    vp.MaxDepth = 1;
    c->RSSetViewports(1, &vp);
    // Seiten wie TextureCube: +X -X +Y -Y +Z -Z (Blickrichtung, oben)
    static const float vorn[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    static const float oben[6][3] = {{0, 1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {0, 1, 0}, {0, 1, 0}};
    const float nah = 1.0F;
    const float fern = weite;
    // perspektivisch, 90 Grad, Seitenverhaeltnis 1, D3D-Tiefe 0..1 (zeilenweise, LH)
    const float q = fern / (fern - nah);
    const float proj[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, q, 1, 0, 0, -q * nah, 0};
    const float licht[4] = {pos[0], pos[1], pos[2], weite};
    for (int s = 0; s < 6; ++s) {
        ID3D11RenderTargetView* rtv = g.wuerfelRtv[slot * 6 + s];
        if (rtv == nullptr) { continue; }
        c->OMSetRenderTargets(1, &rtv, g.wuerfelDsv);
        const float eins[4] = {1, 1, 1, 1};
        c->ClearRenderTargetView(rtv, eins);
        c->ClearDepthStencilView(g.wuerfelDsv, D3D11_CLEAR_DEPTH, 1.0F, 0);
        float blick[16];
        blickLH(pos, vorn[s], oben[s], blick);
        float m[16];
        mal(blick, proj, m);
        schattenZiehen(m, licht, g.psSchatten, teile);
    }
    return true;
}

bool sonnenSchatten(const float* richtung, const float* mitte, float halb, const std::vector<SchattenTeil>& teile) {
    if (!bereit4() || g.sonneRtv == nullptr || halb <= 0) { return false; }
    ID3D11DeviceContext* c = kontext();
    RtSicherung sicher;
    D3D11_VIEWPORT vp{};
    vp.Width = 2048;
    vp.Height = 2048;
    vp.MaxDepth = 1;
    c->RSSetViewports(1, &vp);
    c->OMSetRenderTargets(1, &g.sonneRtv, g.sonneDsv);
    const float eins[4] = {1, 1, 1, 1};
    c->ClearRenderTargetView(g.sonneRtv, eins);
    c->ClearDepthStencilView(g.sonneDsv, D3D11_CLEAR_DEPTH, 1.0F, 0);
    // von der Sonne her auf die Karte: Auge vor der Mitte, Blick gegen die Sonnenrichtung
    const float auge[3] = {mitte[0] + richtung[0] * halb * 2, mitte[1] + richtung[1] * halb * 2, mitte[2] + richtung[2] * halb * 2};
    const float vorn[3] = {-richtung[0], -richtung[1], -richtung[2]};
    const float oben[3] = {std::fabs(richtung[2]) > 0.99F ? 1.0F : 0.0F, 0.0F, std::fabs(richtung[2]) > 0.99F ? 0.0F : 1.0F};
    float blick[16];
    blickLH(auge, vorn, oben, blick);
    const float tief = halb * 4;
    const float ortho[16] = {1.0F / halb, 0, 0, 0, 0, 1.0F / halb, 0, 0, 0, 0, 1.0F / tief, 0, 0, 0, 0, 1};
    mal(blick, ortho, g.sonneVp);
    const float licht[4] = {0, 0, 0, 1};
    schattenZiehen(g.sonneVp, licht, g.psSonne, teile);
    g.sonneAn = true;
    return true;
}

void sonnenSchattenAus() { g.sonneAn = false; }


// --- Stufe 7 Punkt 5b: gebackene Flaechen ---------------------------------------------
namespace {

const char* const kShaderG = R"(
cbuffer B3 : register(b0) {
    float4x4 mvp; float4 farbe; float4 vpnLicht; float4 augeHimmel; float4 alpha; float4 versatz; float4 muster; float4 linie; float4 nebel;
};
Texture2D bild : register(t0);
Texture2D lightmap : register(t2);
SamplerState abtaster : register(s0);
SamplerState lmAbtaster : register(s1);
struct VIG { float3 pos : POSITION; float2 st : TEXCOORD0; float2 lm : TEXCOORD1; float3 n : NORMAL; float4 farbe : COLOR; };
struct VOG { float4 pos : SV_Position; float4 farbe : COLOR; float2 st : TEXCOORD0; float2 lm : TEXCOORD1; };
VOG vsg(VIG i) {
    VOG o;
    float4 c = mul(mvp, float4(i.pos, 1.0));
    c.z = (c.z + c.w) * 0.5;
    o.pos = c;
    o.farbe = i.farbe;
    o.st = i.st;
    o.lm = i.lm;
    return o;
}
float4 psg(VOG i) : SV_Target {
    float4 c = farbe;
    if (alpha.z > 0.5) { c *= bild.Sample(abtaster, i.st); }
    float3 licht = versatz.w > 0.5 ? lightmap.Sample(lmAbtaster, i.lm).rgb : i.farbe.rgb;
    c.rgb = saturate(c.rgb * licht * 2.0);
    int f = (int)alpha.x;
    if (f == 3 && !(c.a > alpha.y)) { discard; }
    if (f == 2 && !(c.a < alpha.y)) { discard; }
    if (f == 5 && !(c.a >= alpha.y)) { discard; }
    return c;
}
)";

bool bereitG() {
    if (!bereit3()) { return false; }
    if (g.vsG != nullptr) { return true; }
    if (g.gescheitertG) { return false; }
    g.gescheitertG = true;
    ID3D11Device* d = geraet();
    ID3DBlob* b[2] = {nullptr, nullptr};
    ID3DBlob* fehler = nullptr;
    if (FAILED(D3DCompile(kShaderG, std::strlen(kShaderG), "karteG", nullptr, nullptr, "vsg", "vs_4_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &b[0], &fehler)) ||
        FAILED(D3DCompile(kShaderG, std::strlen(kShaderG), "karteG", nullptr, nullptr, "psg", "ps_4_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &b[1], &fehler))) {
        los(fehler);
        for (auto*& x : b) { los(x); }
        return false;
    }
    los(fehler);
    d->CreateVertexShader(b[0]->GetBufferPointer(), b[0]->GetBufferSize(), nullptr, &g.vsG);
    d->CreatePixelShader(b[1]->GetBufferPointer(), b[1]->GetBufferSize(), nullptr, &g.psG);
    const D3D11_INPUT_ELEMENT_DESC e[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    d->CreateInputLayout(e, 5, b[0]->GetBufferPointer(), b[0]->GetBufferSize(), &g.layoutG);
    for (auto*& x : b) { los(x); }
    g.gescheitertG = g.vsG == nullptr || g.psG == nullptr || g.layoutG == nullptr;
    return !g.gescheitertG;
}

}  // namespace

struct ZugriffGebacken {
    static ID3D11Buffer* vb(const GebackenNetz& n) { return n.m_stand == render::texturStand() ? static_cast<ID3D11Buffer*>(n.m_vb) : nullptr; }
    static ID3D11Buffer* ib(const GebackenNetz& n) { return n.m_stand == render::texturStand() ? static_cast<ID3D11Buffer*>(n.m_ib) : nullptr; }
};

GebackenNetz::~GebackenNetz() { leeren(); }

void GebackenNetz::leeren() {
    if (m_stand == render::texturStand()) {
        if (m_vb != nullptr) { static_cast<ID3D11Buffer*>(m_vb)->Release(); }
        if (m_ib != nullptr) { static_cast<ID3D11Buffer*>(m_ib)->Release(); }
    }
    m_vb = nullptr;
    m_ib = nullptr;
    m_stand = ~0U;
}

bool GebackenNetz::gueltig() const { return m_vb != nullptr && m_ib != nullptr && m_stand == render::texturStand(); }

bool GebackenNetz::setzen(const std::vector<GebackenPunkt>& punkte, const std::vector<std::uint32_t>& indizes) {
    leeren();
    if (!bereit() || punkte.empty() || indizes.empty()) { return false; }
    static_assert(sizeof(GebackenPunkt) == 44, "wie das Eingabeformat des Shaders");
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = static_cast<UINT>(punkte.size() * sizeof(GebackenPunkt));
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = punkte.data();
    ID3D11Buffer* vb = nullptr;
    if (FAILED(geraet()->CreateBuffer(&bd, &sd, &vb))) { return false; }
    bd.ByteWidth = static_cast<UINT>(indizes.size() * sizeof(std::uint32_t));
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    sd.pSysMem = indizes.data();
    ID3D11Buffer* ib = nullptr;
    if (FAILED(geraet()->CreateBuffer(&bd, &sd, &ib))) {
        vb->Release();
        return false;
    }
    m_vb = vb;
    m_ib = ib;
    m_stand = render::texturStand();
    return true;
}

void* lightmapAnlegen(const std::uint8_t* rgb, int breite, int hoehe) {
    if (!bereit() || rgb == nullptr || breite <= 0 || hoehe <= 0) { return nullptr; }
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(breite) * static_cast<std::size_t>(hoehe) * 4);
    for (std::size_t i = 0; i < static_cast<std::size_t>(breite) * static_cast<std::size_t>(hoehe); ++i) {
        rgba[i * 4] = rgb[i * 3];
        rgba[i * 4 + 1] = rgb[i * 3 + 1];
        rgba[i * 4 + 2] = rgb[i * 3 + 2];
        rgba[i * 4 + 3] = 255;
    }
    D3D11_TEXTURE2D_DESC td{};
    td.Width = static_cast<UINT>(breite);
    td.Height = static_cast<UINT>(hoehe);
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = rgba.data();
    sd.SysMemPitch = static_cast<UINT>(breite) * 4;
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(geraet()->CreateTexture2D(&td, &sd, &tex))) { return nullptr; }
    ID3D11ShaderResourceView* srv = nullptr;
    geraet()->CreateShaderResourceView(tex, nullptr, &srv);
    tex->Release();
    return srv;
}

void gebackenZeichnen(const GebackenNetz& netz, std::uint32_t erster, std::uint32_t anzahl, void* lightmap, const Zustand3& z) {
    ID3D11Buffer* vb = ZugriffGebacken::vb(netz);
    ID3D11Buffer* ib = ZugriffGebacken::ib(netz);
    if (vb == nullptr || ib == nullptr || anzahl == 0 || !bereitG()) { return; }
    Zustand3 zz = z;
    zz.echtLicht = false;
    zz.licht = false;
    zz.versatz = false;
    zz.versatzEinheiten = 1;
    if (!zustandSetzen(zz, false)) { return; }
    ID3D11DeviceContext* c = kontext();
    // versatz.w traegt hier "mit Lightmap" (Konstanten3 neu schreiben)
    D3D11_MAPPED_SUBRESOURCE m{};
    if (SUCCEEDED(c->Map(g.cb3, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
        auto* k = static_cast<Konstanten3*>(m.pData);
        std::memset(k, 0, sizeof(Konstanten3));
        std::memcpy(k->mvp, zz.matrix, sizeof(k->mvp));
        std::memcpy(k->farbe, zz.farbe, sizeof(k->farbe));
        k->alpha[0] = static_cast<float>(zz.alphaFunktion);
        k->alpha[1] = zz.alphaRef;
        k->alpha[2] = zz.bild != nullptr ? 1.0F : 0.0F;
        k->versatz[3] = lightmap != nullptr ? 1.0F : 0.0F;
        c->Unmap(g.cb3, 0);
    }
    c->IASetInputLayout(g.layoutG);
    c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    c->VSSetShader(g.vsG, nullptr, 0);
    c->PSSetShader(g.psG, nullptr, 0);
    c->VSSetConstantBuffers(0, 1, &g.cb3);
    c->PSSetConstantBuffers(0, 1, &g.cb3);
    ID3D11ShaderResourceView* srv[3] = {static_cast<ID3D11ShaderResourceView*>(zz.bild), nullptr, static_cast<ID3D11ShaderResourceView*>(lightmap)};
    c->PSSetShaderResources(0, 3, srv);
    ID3D11SamplerState* sm[2] = {g.abtaster, g.wuerfelAbtaster};
    c->PSSetSamplers(0, 2, sm);
    const UINT schritt = sizeof(GebackenPunkt);
    const UINT versatz = 0;
    c->IASetVertexBuffers(0, 1, &vb, &schritt, &versatz);
    c->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    c->DrawIndexed(anzahl, erster, 0);
}

void ohneMischen(void* dl) {
    // Der ImGui-Rueckruf setzt einen Mischzustand ohne Mischen; ImGui setzt
    // ihn erst mit ImDrawCallback_ResetRenderState zurueck.
    static_cast<ImDrawList*>(dl)->AddCallback(
        [](const ImDrawList*, const ImDrawCmd*) {
            static ID3D11BlendState* zustand = nullptr;
            static unsigned stand = ~0U;
            if (stand != render::texturStand()) {
                if (zustand != nullptr) { zustand->Release(); }
                zustand = nullptr;
                stand = render::texturStand();
            }
            if (zustand == nullptr && geraet() != nullptr) {
                D3D11_BLEND_DESC bd{};
                bd.RenderTarget[0].BlendEnable = FALSE;
                bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
                geraet()->CreateBlendState(&bd, &zustand);
            }
            if (zustand != nullptr && kontext() != nullptr) {
                const float faktor[4] = {0, 0, 0, 0};
                kontext()->OMSetBlendState(zustand, faktor, 0xFFFFFFFFU);
            }
        },
        nullptr);
}

void* bildAnlegen(const unsigned char* rgba, int breite, int hoehe, int mipStufe) {
    if (!bereit() || rgba == nullptr || breite <= 0 || hoehe <= 0) { return nullptr; }
    ID3D11Device* d = geraet();
    // GL_GENERATE_MIPMAP: alle Stufen aus der ersten
    D3D11_TEXTURE2D_DESC td{};
    td.Width = static_cast<UINT>(breite);
    td.Height = static_cast<UINT>(hoehe);
    td.MipLevels = 0;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(d->CreateTexture2D(&td, nullptr, &tex))) { return nullptr; }
    kontext()->UpdateSubresource(tex, 0, nullptr, rgba, static_cast<UINT>(breite) * 4U, 0);
    ID3D11ShaderResourceView* voll = nullptr;   // fuer GenerateMips alle Stufen
    d->CreateShaderResourceView(tex, nullptr, &voll);
    if (voll != nullptr) { kontext()->GenerateMips(voll); }
    const int basis = karte::mipBasis(mipStufe, breite, hoehe);
    if (basis <= 0 || voll == nullptr) {
        tex->Release();
        return voll;
    }
    voll->Release();
    D3D11_SHADER_RESOURCE_VIEW_DESC sv{};   // GL_TEXTURE_BASE_LEVEL
    sv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sv.Texture2D.MostDetailedMip = static_cast<UINT>(basis);
    sv.Texture2D.MipLevels = static_cast<UINT>(-1);
    ID3D11ShaderResourceView* srv = nullptr;
    d->CreateShaderResourceView(tex, &sv, &srv);
    tex->Release();
    return srv;
}

void abtasterSetzen(int modus, bool anisotropie) {
    if (!bereit()) { return; }
    D3D11_SAMPLER_DESC sd{};
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.MaxAnisotropy = 1;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    switch (modus) {
    case 0: sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; sd.MaxLOD = 0; break;           // GL_NEAREST
    case 1: sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; break;                          // GL_NEAREST_MIPMAP_NEAREST
    case 2: sd.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR; break;                   // GL_NEAREST_MIPMAP_LINEAR
    case 3: sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT; sd.MaxLOD = 0; break;    // GL_LINEAR
    case 4: sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT; break;                   // GL_LINEAR_MIPMAP_NEAREST
    default: sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; break;                        // GL_LINEAR_MIPMAP_LINEAR
    }
    if (anisotropie && modus == 5) {   // GL_TEXTURE_MAX_ANISOTROPY_EXT = maxAniso (D3D11: nur mit Trilinear)
        sd.Filter = D3D11_FILTER_ANISOTROPIC;
        sd.MaxAnisotropy = 16;
    }
    ID3D11SamplerState* neu = nullptr;
    if (SUCCEEDED(geraet()->CreateSamplerState(&sd, &neu)) && neu != nullptr) {
        if (g.abtaster != nullptr) { g.abtaster->Release(); }
        g.abtaster = neu;
    }
}

void* wuerfelAnlegen(const unsigned char* const seiten[6], int groesse) {
    if (!bereit() || groesse <= 0) { return nullptr; }
    ID3D11Device* d = geraet();
    D3D11_TEXTURE2D_DESC td{};
    td.Width = static_cast<UINT>(groesse);
    td.Height = static_cast<UINT>(groesse);
    td.MipLevels = 1;   // GL_TEXTURE_MAX_LEVEL 0
    td.ArraySize = 6;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    td.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    D3D11_SUBRESOURCE_DATA sd[6]{};
    for (int i = 0; i < 6; ++i) {
        sd[i].pSysMem = seiten[i];
        sd[i].SysMemPitch = static_cast<UINT>(groesse) * 4U;
    }
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(d->CreateTexture2D(&td, sd, &tex))) { return nullptr; }
    ID3D11ShaderResourceView* srv = nullptr;
    d->CreateShaderResourceView(tex, nullptr, &srv);
    tex->Release();
    return srv;
}

void bildFreigeben(void* bild) {
    if (bild != nullptr) { g_spaeter.emplace_back(static_cast<ID3D11ShaderResourceView*>(bild), ImGui::GetFrameCount()); }
}

void aufraeumen() {
    const int jetzt = ImGui::GetFrameCount();
    const auto ende = std::remove_if(g_spaeter.begin(), g_spaeter.end(), [jetzt](const auto& s) {
        if (s.second >= jetzt) { return false; }
        s.first->Release();
        return true;
    });
    g_spaeter.erase(ende, g_spaeter.end());
}

unsigned geraetStand() { return render::texturStand(); }

int abtastungen(int kennung) {
    const auto it = g.ziele.find(kennung);
    return it == g.ziele.end() ? 0 : it->second.msaa;
}

bool pixel(int kennung, int x, int y, unsigned char rgba[4]) {
    const auto it = g.ziele.find(kennung);
    if (it == g.ziele.end() || g.stand != render::texturStand() || it->second.aufgeloest == nullptr) { return false; }
    const Ziel& z = it->second;
    if (x < 0 || y < 0 || x >= z.breite || y >= z.hoehe) { return false; }
    D3D11_TEXTURE2D_DESC td{};
    td.Width = 1;
    td.Height = 1;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_STAGING;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(geraet()->CreateTexture2D(&td, nullptr, &tex))) { return false; }
    const D3D11_BOX box{static_cast<UINT>(x), static_cast<UINT>(y), 0, static_cast<UINT>(x) + 1, static_cast<UINT>(y) + 1, 1};
    kontext()->CopySubresourceRegion(tex, 0, 0, 0, 0, z.aufgeloest, 0, &box);
    D3D11_MAPPED_SUBRESOURCE m{};
    const bool ok = SUCCEEDED(kontext()->Map(tex, 0, D3D11_MAP_READ, 0, &m));
    if (ok) {
        std::memcpy(rgba, m.pData, 4);
        kontext()->Unmap(tex, 0);
    }
    tex->Release();
    return ok;
}

}  // namespace bhed::gui::kartegpu
