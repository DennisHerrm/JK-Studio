// karte_gpu_win32.cpp - Karten-Editor: Zeichnen ueber Direct3D 11 (siehe
// karte_gpu.h). Je Ansicht ein Renderziel mit Mehrfachabtastung (NRC: FBO
// mit MSAA, Vorgabe 8 Abtastungen, xywindow.cpp:2291 "XYMSAA"), Linien als
// Linienliste, breite Linien (glLineWidth 2) ueber einen Geometrie-Shader.
#include "karte_gpu.h"

#include "backend.h"

#include <d3d11.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <cstring>
#include <map>
#include <string>

namespace bhed::gui::kartegpu {

namespace {

struct Punkt {
    float x, y, z;
    std::uint32_t farbe;   // R8G8B8A8_UNORM (Rot im niedrigsten Byte)
};

struct Konstanten {
    float mvp[16];
    float groesse[2];
    float breite;
    float frei;
};

struct Ziel {
    ID3D11Texture2D* tex = nullptr;        // mit Mehrfachabtastung
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11Texture2D* aufgeloest = nullptr; // ohne, fuer ImGui
    ID3D11ShaderResourceView* srv = nullptr;
    int breite = 0;
    int hoehe = 0;
    int msaa = 1;
};

const char* const kShader = R"(
cbuffer B : register(b0) { float4x4 mvp; float2 groesse; float breite; float frei; };
struct VSI { float3 pos : POSITION; float4 farbe : COLOR; };
struct VSO { float4 pos : SV_Position; float4 farbe : COLOR; };
VSO vs(VSI i) {
    VSO o;
    o.pos = mul(mvp, float4(i.pos, 1.0));
    o.pos.z = (o.pos.z + o.pos.w) * 0.5;   // OpenGL-Tiefe -1..1 -> Direct3D 0..1
    o.farbe = i.farbe;
    return o;
}
[maxvertexcount(4)]
void gs(line VSO e[2], inout TriangleStream<VSO> s) {
    float2 h = groesse * 0.5;
    float2 a = e[0].pos.xy / e[0].pos.w * h;
    float2 b = e[1].pos.xy / e[1].pos.w * h;
    float2 d = b - a;
    float l = length(d);
    d = (l > 1e-6) ? d / l : float2(1.0, 0.0);
    float2 n = float2(-d.y, d.x) * (breite * 0.5) / h;
    VSO v;
    v = e[0]; v.pos.xy += n * e[0].pos.w; s.Append(v);
    v = e[0]; v.pos.xy -= n * e[0].pos.w; s.Append(v);
    v = e[1]; v.pos.xy += n * e[1].pos.w; s.Append(v);
    v = e[1]; v.pos.xy -= n * e[1].pos.w; s.Append(v);
}
float4 ps(VSO i) : SV_Target { return i.farbe; }
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
    std::map<int, Ziel> ziele;
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

void zielLoesen(Ziel& z) {
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
    los(g.einmal);
    g.einmalPlatz = 0;
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

void ziehen(ID3D11Buffer* vb, std::size_t anzahl, const float* matrix, float breite) {
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
    k->frei = 0;
    c->Unmap(g.cb, 0);
    const UINT schritt = sizeof(Punkt);
    const UINT versatz = 0;
    c->IASetVertexBuffers(0, 1, &vb, &schritt, &versatz);
    c->IASetInputLayout(g.layout);
    c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    c->VSSetShader(g.vs, nullptr, 0);
    c->VSSetConstantBuffers(0, 1, &g.cb);
    if (breite > 1.0F) {
        c->GSSetShader(g.gs, nullptr, 0);
        c->GSSetConstantBuffers(0, 1, &g.cb);
    } else {
        c->GSSetShader(nullptr, nullptr, 0);
    }
    c->PSSetShader(g.ps, nullptr, 0);
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

bool beginnen(int kennung, int breite, int hoehe, int msaa, const float* hinten) {
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
    if (z.tex == nullptr || z.breite != breite || z.hoehe != hoehe || z.msaa != stufe) {
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
        z.breite = breite;
        z.hoehe = hoehe;
        z.msaa = stufe;
    }
    if (z.rtv == nullptr) { return false; }
    g.altVpAnzahl = 1;
    c->RSGetViewports(&g.altVpAnzahl, &g.altVp);
    c->OMGetRenderTargets(1, &g.altRtv, &g.altDsv);
    c->OMSetRenderTargets(1, &z.rtv, nullptr);
    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(breite);
    vp.Height = static_cast<float>(hoehe);
    vp.MaxDepth = 1.0F;
    c->RSSetViewports(1, &vp);
    c->ClearRenderTargetView(z.rtv, hinten);
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

void freigeben() { allesLoesen(); }

}  // namespace bhed::gui::kartegpu
