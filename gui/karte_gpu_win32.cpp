// karte_gpu_win32.cpp - Karten-Editor: Zeichnen ueber Direct3D 11 (siehe
// karte_gpu.h). Je Ansicht ein Renderziel mit Mehrfachabtastung (NRC: FBO
// mit MSAA, Vorgabe 8 Abtastungen, xywindow.cpp:2291 "XYMSAA"), Linien als
// Linienliste, breite Linien (glLineWidth 2) ueber einen Geometrie-Shader.
#include "karte_gpu.h"

#include "bhed/diag.h"
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
    // Stufe 7/9c: Echtzeit-Licht
    bool gescheitert4 = false;
    ID3D11VertexShader* vs4 = nullptr;
    ID3D11PixelShader* ps4 = nullptr;
    ID3D11VertexShader* vsSchatten = nullptr;
    ID3D11PixelShader* psSchatten = nullptr;     // Wuerfel: Abstand / Weite
    ID3D11PixelShader* psSchattenA = nullptr;    // dasselbe mit Alpha-Test (alphashadow)
    ID3D11PixelShader* psHimmel = nullptr;       // Sonne: Tiefe der Himmelsflaechen, die von der Sonne wegzeigen (S)
    ID3D11PixelShader* psUnter = nullptr;        // Sonne: Tiefe der Verdecker unter dem Himmel (O)
    ID3D11PixelShader* psUnterA = nullptr;       // dasselbe mit Alpha-Test
    ID3D11Buffer* cb4 = nullptr;          // Umgebung, Kaskaden, Himmelskarten
    ID3D11Buffer* cbSchatten = nullptr;   // eine Seite bzw. eine Sonnenkarte
    ID3D11Buffer* lichtBuf = nullptr;     // Buffer<float4>: 7 je Licht
    ID3D11ShaderResourceView* lichtSrv = nullptr;
    ID3D11Buffer* eckenBuf = nullptr;     // Buffer<float4>: Ecken der Flaechenlichter
    ID3D11ShaderResourceView* eckenSrv = nullptr;
    std::size_t eckenPlatz = 0;
    ID3D11Buffer* kachelBuf = nullptr;    // Buffer<uint>: Bitmasken je Kachel
    ID3D11ShaderResourceView* kachelSrv = nullptr;
    std::size_t kachelPlatz = 0;
    struct Pool {                          // Schattenwuerfel einer Groesse
        int groesse = 0;
        int anzahl = 0;
        ID3D11Texture2D* tex = nullptr;
        std::vector<ID3D11RenderTargetView*> rtv;
        ID3D11ShaderResourceView* srv = nullptr;
        ID3D11Texture2D* tiefe = nullptr;
        ID3D11DepthStencilView* dsv = nullptr;
    };
    Pool pool[2];
    struct Karten {                        // Sonnenkarten: Schichten mit O und S
        int groesse = 0;
        int schichten = 0;
        ID3D11Texture2D* texO = nullptr;
        ID3D11Texture2D* texS = nullptr;
        std::vector<ID3D11RenderTargetView*> rtvO;
        std::vector<ID3D11RenderTargetView*> rtvS;
        ID3D11ShaderResourceView* srvO = nullptr;
        ID3D11ShaderResourceView* srvS = nullptr;
        ID3D11Texture2D* tiefe = nullptr;
        ID3D11DepthStencilView* dsv = nullptr;
    };
    Karten kaskaden;                       // 2 Schichten kKaskadeGroesse
    Karten himmelK;                        // kHimmelSonnenMax Schichten 1024
    float kaskadeVp[2][16] = {};
    float kaskadeTexel[2] = {0, 0};
    bool kaskadeAn[2] = {false, false};
    float himmelVp[kHimmelSonnenMax][16] = {};
    float himmelTexel = 0;
    ID3D11SamplerState* punktAbtaster = nullptr;
    ID3D11RasterizerState* rsSchatten = nullptr;
    std::map<int, ID3D11BlendState*> spielMischen;   // Stufe 9c: blendFunc der ersten Stufe
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

void poolLoesen(Zustand::Pool& p) {
    for (auto& v : p.rtv) { los(v); }
    p.rtv.clear();
    los(p.srv);
    los(p.tex);
    los(p.dsv);
    los(p.tiefe);
}
void kartenLoesen(Zustand::Karten& k) {
    for (auto& v : k.rtvO) { los(v); }
    for (auto& v : k.rtvS) { los(v); }
    k.rtvO.clear();
    k.rtvS.clear();
    los(k.srvO);
    los(k.srvS);
    los(k.texO);
    los(k.texS);
    los(k.dsv);
    los(k.tiefe);
}

void echtLichtLoesen() {
    los(g.vs4);
    los(g.ps4);
    los(g.vsSchatten);
    los(g.psSchatten);
    los(g.psSchattenA);
    los(g.psHimmel);
    los(g.psUnter);
    los(g.psUnterA);
    los(g.cb4);
    los(g.cbSchatten);
    los(g.lichtBuf);
    los(g.lichtSrv);
    los(g.eckenBuf);
    los(g.eckenSrv);
    g.eckenPlatz = 0;
    los(g.kachelBuf);
    los(g.kachelSrv);
    g.kachelPlatz = 0;
    poolLoesen(g.pool[0]);
    poolLoesen(g.pool[1]);
    kartenLoesen(g.kaskaden);
    kartenLoesen(g.himmelK);
    los(g.punktAbtaster);
    los(g.rsSchatten);
    for (auto& [k, v] : g.spielMischen) { los(v); }
    g.spielMischen.clear();
    los(g.vsG);
    los(g.psG);
    los(g.layoutG);
    g.gescheitertG = false;
    g.gescheitert4 = false;
    g.kaskadeAn[0] = g.kaskadeAn[1] = false;
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
ID3D11BlendState* spielMischung(int q, int z);   // Stufe 9c: blendFunc der ersten Stufe

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
    c->OMSetBlendState(z.mischen == Mischen::Spiel ? spielMischung(z.mischQuelle, z.mischZiel)
                       : z.mischen == Mischen::Aus  ? g.blendAus
                       : z.mischen == Mischen::Alpha ? g.blend
                                                     : g.blendAdd,
                       faktor, 0xFFFFFFFFU);
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
        ID3D11ShaderResourceView* srv[11] = {static_cast<ID3D11ShaderResourceView*>(z.bild),
                                             nullptr,
                                             g.pool[0].srv,
                                             g.pool[1].srv,
                                             g.kaskaden.srvO,
                                             g.kaskaden.srvS,
                                             g.himmelK.srvO,
                                             g.himmelK.srvS,
                                             g.lichtSrv,
                                             g.eckenSrv,
                                             g.kachelSrv};
        c->PSSetShaderResources(0, 11, srv);
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


// --- Stufe 7/9c: Echtzeit-Licht ----------------------------------------------------------
//
// Die Formeln von LightContributionToSample (q3map2 light.cpp:777-1177 mit -fast,
// siehe bhed/karte/echtlicht.cpp echtLichtBeitrag - der Unit-Test karte_echtlicht
// prueft dieselben Werte auf dem Prozessor). Der Luxel wie StoreSurfaceLightmaps/
// ColorToBytes: Umgebung + Summe, mindestens _minlight, ueber 255 auf 255
// normiert; im Spiel mit der Textur * 2 (r_mapOverBrightBits). Vorschau: kein
// Bounce, kein Filter, kein Supersampling, Schatten aus Karten statt Strahlen.
//
// Stufe 9c: Lichter in einem Puffer (bis kLichtMax), je Kachel eine Bitmaske;
// Flaechenlichter mit PointToPolygonFormFactor ueber ihre Ecken; Schatten mit
// 5 bzw. 9 Proben (PCF) und Versatz entlang der Normale. Sonne wie TraceLine mit
// testAll (light_trace.cpp:1702-1755): der Strahl zur Sonne endet am ersten festen
// Blatt; hell nur, wenn er davor eine Himmelsflaeche traf und kein deckendes
// Dreieck. Nachgebildet mit zwei Karten je Sonne: S = Tiefe der Himmelsflaechen,
// die von der Sonne wegzeigen (die naechste vor der Stelle), O = die naechsten
// Verdecker darunter (deckende Dreiecke und die Volumen strukturell deckender
// Pinsel ausser den Himmelspinseln).
namespace {

struct Konstanten4 {
    float umgebung[4];
    float minimum[4];
    float sonne[4];        // x Kaskade 0 an, y Kaskade 1 an, z Texel 0 (Einheiten), w Texel 1
    float himmelInfo[4];   // x Texel der Himmelskarten
    int anzahl[4];         // x Lichter, y Kacheln x, z Kacheln y, w Worte je Kachel
    float kaskadeVp[2][16];
    float himmelVp[kHimmelSonnenMax][16];
};
struct KonstantenSchatten {
    float vp[16];          // zeilenweise
    float licht[4];        // xyz Ursprung, w Weite (Wuerfel)
    float zurSonne[4];     // xyz zur Sonne (Sonnenkarten)
    float werte[4];        // y Schicht (S), z Spielraum (Einheiten)
};

const char* const kShader4 = R"(
cbuffer B3 : register(b0) {
    float4x4 mvp; float4 farbe; float4 vpnLicht; float4 augeHimmel; float4 alpha; float4 versatz; float4 muster; float4 linie; float4 nebel;
};
cbuffer B4 : register(b1) {
    float4 umgebung; float4 minimum; float4 sonne; float4 himmelInfo; int4 anzahl;
    row_major float4x4 kaskadeVp[2];
    row_major float4x4 himmelVp[16];
};
Texture2D bild : register(t0);
TextureCubeArray schattenG : register(t2);
TextureCubeArray schattenK : register(t3);
Texture2DArray kaskO : register(t4);
Texture2DArray kaskS : register(t5);
Texture2DArray himO : register(t6);
Texture2DArray himS : register(t7);
Buffer<float4> lichtB : register(t8);
Buffer<float4> eckenB : register(t9);
Buffer<uint> kachelB : register(t10);
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
struct LichtD { float4 pos; float4 farbe; float4 richtung; float4 werte; float4 mehr; float4 flaeche; float4 zusatz; };
LichtD lichtLesen(uint k) {
    LichtD L;
    uint b = k * 7u;
    L.pos = lichtB[b];
    L.farbe = lichtB[b + 1u];
    L.richtung = lichtB[b + 2u];
    L.werte = lichtB[b + 3u];
    L.mehr = lichtB[b + 4u];
    L.flaeche = lichtB[b + 5u];
    L.zusatz = lichtB[b + 6u];
    return L;
}
float3 einheit(float3 d) { float l = length(d); return l > 0.0 ? d / l : float3(0, 0, 0); }
// PointToPolygonFormFactor (light.cpp:734)
float formFaktor(float3 p, float3 n, uint erste, uint zahl) {
    float3 d0 = einheit(eckenB[erste].xyz - p);
    float3 a = d0;
    float total = 0.0;
    [loop] for (uint i = 1u; i <= zahl; ++i) {
        float3 b = i < zahl ? einheit(eckenB[erste + i].xyz - p) : d0;
        float3 tri = cross(a, b);
        float l = length(tri);
        if (l >= 0.0001) {
            tri /= l;
            total += dot(n, tri) * acos(clamp(dot(a, b), -1.0, 1.0));
            if (total > 6.3 || total < -6.3) { return 0.0; }
        }
        a = b;
    }
    return total * 0.15915494;
}
// LightContributionToSample ohne Strahl
float beitrag(LichtD L, float3 p, float3 n) {
    int art = (int)L.farbe.a;
    if (art == 2) {
        float w = L.werte.y > 0.5 ? dot(n, L.richtung.xyz) : 1.0;
        return max(0.0, L.pos.w * w);
    }
    if (art == 3 && L.flaeche.w > 2.5) {
        float d = dot(p, L.richtung.xyz) - dot(L.pos.xyz, L.richtung.xyz);
        bool zwei = L.flaeche.y > 0.5;
        // Fernfeld (Abstand ueber dem Vierfachen des Stuecks): der Grenzwert von
        // PointToPolygonFormFactor, A cos1 cos2 / (pi d^2) - spart die Winkel je Ecke
        float3 zum = L.pos.xyz - p;
        float dd = length(zum);
        if (dd > 4.0 * L.zusatz.z && d >= 8.0) {
            if (dd >= L.werte.w) { return 0.0; }
            float3 dir = zum / dd;
            float c1 = dot(n, dir);
            float c2 = -dot(L.richtung.xyz, dir);
            if (c1 <= 0.0 || c2 <= 0.0) { return 0.0; }
            float addF = L.zusatz.y * c1 * c2 / (3.14159265 * dd * dd) * L.flaeche.x;
            return addF <= 1.0 ? 0.0 : addF;
        }
        if (d < 3.0) {
            if (!zwei && d < -1.0) { return 0.0; }
            if (d > -3.0 && dot(n, L.richtung.xyz) > 0.9) { return 0.0; }
        }
        float3 q = (d > -8.0 && d < 8.0) ? p + L.richtung.xyz * (8.0 - d) : p;
        if (length(L.pos.xyz - p) >= L.werte.w) { return 0.0; }
        float f = formFaktor(q, n, (uint)L.flaeche.z, (uint)L.flaeche.w);
        if (f == 0.0) { return 0.0; }
        if (f < 0.0) {
            if (!zwei) { return 0.0; }
            f = -f;
            if (length(L.pos.xyz - L.richtung.xyz * 2.0 - p) >= L.werte.w) { return 0.0; }
        }
        float add = f * L.flaeche.x;
        return add <= 1.0 ? 0.0 : add;
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
// Schattenwuerfel: 5 Proben, Versatz entlang der Normale um gut einen Texel
float wuerfelSicht(LichtD L, float3 p, float3 n) {
    float slot = L.mehr.x;
    if (slot < 0.0) { return 1.0; }
    bool gross = slot < 31.5;
    float groesse = gross ? 512.0 : 256.0;
    float texel = 2.0 * length(p - L.pos.xyz) / groesse;
    float3 q = p + n * (texel * 1.5 + 0.5);
    float3 d = q - L.pos.xyz;
    float ab = length(d);
    float3 dn = d / max(ab, 0.001);
    float3 t1 = normalize(cross(dn, abs(dn.z) < 0.9 ? float3(0, 0, 1) : float3(1, 0, 0)));
    float3 t2 = cross(dn, t1);
    float r = 1.5 * 2.0 / groesse;
    float bias = 1.0 + ab * 0.003;
    float summe = 0.0;
    const float2 versatz5[5] = {float2(0, 0), float2(1, 0.3), float2(-1, -0.3), float2(-0.3, 1), float2(0.3, -1)};
    [unroll] for (int k = 0; k < 5; ++k) {
        float3 dir = dn + (t1 * versatz5[k].x + t2 * versatz5[k].y) * r;
        float g = gross ? schattenG.SampleLevel(punktAbtaster, float4(dir, slot), 0).r
                        : schattenK.SampleLevel(punktAbtaster, float4(dir, slot - 32.0), 0).r;
        summe += (ab - bias <= g * L.mehr.y) ? 1.0 : 0.0;
    }
    return summe * 0.2;
}
// Sonne: 3 x 3 Proben; hell, wenn ein Himmel ueber der Stelle liegt (t >= S) und kein Verdecker dazwischen (t <= O)
float sonnenProben(Texture2DArray O, Texture2DArray S, float schicht, float2 uv, float t, float du, float spiel) {
    float summe = 0.0;
    [unroll] for (int y = -1; y <= 1; ++y) {
        [unroll] for (int x = -1; x <= 1; ++x) {
            float3 q = float3(uv + float2(x, y) * du, schicht);
            float s = S.SampleLevel(punktAbtaster, q, 0).r;
            float o = O.SampleLevel(punktAbtaster, q, 0).r;
            summe += (s > -1.0e29 && t >= s - spiel && t <= o + spiel) ? 1.0 : 0.0;
        }
    }
    return summe / 9.0;
}
float sonnenSicht(float3 p, float3 n, float3 zurSonne) {
    if (sonne.x < 0.5) { return 1.0; }
    [unroll] for (int k = 1; k >= 0; --k) {
        float an = k == 1 ? sonne.y : sonne.x;
        if (an < 0.5) { continue; }
        float texel = k == 1 ? sonne.w : sonne.z;
        float3 q = p + n * (texel * 1.5 + 0.25);
        float4 c = mul(float4(q, 1.0), kaskadeVp[k]);
        float2 uv = float2(c.x * 0.5 + 0.5, 0.5 - c.y * 0.5);
        if (k == 1 && (uv.x < 0.01 || uv.y < 0.01 || uv.x > 0.99 || uv.y > 0.99)) { continue; }
        if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0) { return 0.0; }
        return sonnenProben(kaskO, kaskS, (float)k, uv, -dot(q, zurSonne), 1.0 / 2048.0, texel * 2.0 + 1.0);
    }
    return 1.0;
}
float himmelSicht(LichtD L, float3 p, float3 n) {
    int s = (int)L.mehr.x;
    float texel = himmelInfo.x;
    float3 q = p + n * (texel * 1.5 + 0.25);
    float4 c = mul(float4(q, 1.0), himmelVp[s]);
    float2 uv = float2(c.x * 0.5 + 0.5, 0.5 - c.y * 0.5);
    if (uv.x < 0.0 || uv.y < 0.0 || uv.x > 1.0 || uv.y > 1.0) { return 0.0; }
    return sonnenProben(himO, himS, (float)s, uv, -dot(q, L.richtung.xyz), 1.0 / 1024.0, texel * 2.0 + 1.0);
}
float3 lichtVon(uint k, float3 p, float3 n) {
    // erst Ort und Huelle (2 von 7 Werten): die meisten Lichter enden hier
    float4 pos = lichtB[k * 7u];
    float4 werte = lichtB[k * 7u + 3u];
    if (length(pos.xyz - p) >= werte.w) { return float3(0, 0, 0); }
    LichtD L = lichtLesen(k);
    float add = beitrag(L, p, n);
    if (add <= 0.0) { return float3(0, 0, 0); }
    float s;
    if ((int)L.farbe.a == 2) {
        s = L.mehr.x < -2.5 ? sonnenSicht(p, n, L.richtung.xyz) : (L.mehr.x >= 0.0 ? himmelSicht(L, p, n) : 1.0);
    } else {
        s = wuerfelSicht(L, p, n);
    }
    return L.farbe.rgb * add * s * (L.zusatz.x > 0.5 ? -1.0 : 1.0);
}
float4 ps4(VO4 i) : SV_Target {
    float3 n = normalize(i.n);
    float3 lm = umgebung.rgb;
    uint kx = min((uint)i.pos.x / 32u, (uint)max(anzahl.y - 1, 0));
    uint ky = min((uint)i.pos.y / 32u, (uint)max(anzahl.z - 1, 0));
    uint worte = (uint)anzahl.w;
    uint basis = (ky * (uint)anzahl.y + kx) * worte;
    [loop] for (uint w = 0u; w < worte; ++w) {
        uint m = kachelB[basis + w];
#ifdef OHNE_FBL
        [loop] for (uint bit = 0u; bit < 32u && m != 0u; ++bit) {
            if ((m & (1u << bit)) == 0u) { continue; }
            m &= ~(1u << bit);
            lm += lichtVon(w * 32u + bit, i.welt, n);
        }
#else
        [loop] while (m != 0u) {
            uint bit = firstbitlow(m);
            m &= m - 1u;
            lm += lichtVon(w * 32u + bit, i.welt, n);
        }
#endif
    }
    lm = max(lm, minimum.rgb);
    float mx = max(lm.r, max(lm.g, lm.b));
    if (mx > 255.0) { lm *= 255.0 / mx; }
    float4 c = i.farbe;
    if (alpha.z > 0.5) { c *= bild.Sample(abtaster, i.st); }
    c.rgb = saturate(c.rgb * (lm / 255.0) * 2.0);
    c.rgb = genebelt(c.rgb, i.tiefe);
    if (!alphaGut(c.a)) { discard; }
    return c;
}
cbuffer BS : register(b2) { row_major float4x4 seiteVp; float4 lichtPos; float4 zurSonne; float4 werteS; };
Texture2DArray hS : register(t1);
struct VOS { float4 pos : SV_Position; float3 welt : TEXCOORD0; float3 n : TEXCOORD1; float2 st : TEXCOORD2; };
VOS vss(VI3 i) {
    VOS o;
    o.pos = mul(float4(i.pos, 1.0), seiteVp);
    o.welt = i.pos;
    o.n = i.n;
    o.st = i.st;
    return o;
}
float pss(VOS i) : SV_Target { return length(i.welt - lichtPos.xyz) / lichtPos.w; }
float pssA(VOS i) : SV_Target {
    if (bild.Sample(abtaster, i.st).a < 0.5) { discard; }
    return length(i.welt - lichtPos.xyz) / lichtPos.w;
}
float psHimmel(VOS i) : SV_Target {
    if (dot(i.n, zurSonne.xyz) >= 0.0) { discard; }
    return -dot(i.welt, zurSonne.xyz);
}
float psUnter(VOS i) : SV_Target {
    float t = -dot(i.welt, zurSonne.xyz);
    float s = hS.Load(int4((int)i.pos.x, (int)i.pos.y, (int)werteS.y, 0)).r;
    if (t < s - werteS.z) { discard; }
    return t;
}
float psUnterA(VOS i) : SV_Target {
    if (bild.Sample(abtaster, i.st).a < 0.5) { discard; }
    float t = -dot(i.welt, zurSonne.xyz);
    float s = hS.Load(int4((int)i.pos.x, (int)i.pos.y, (int)werteS.y, 0)).r;
    if (t < s - werteS.z) { discard; }
    return t;
}
)";

bool poolAnlegen(Zustand::Pool& p, int groesse, int anzahl) {
    ID3D11Device* d = geraet();
    p.groesse = groesse;
    p.anzahl = anzahl;
    D3D11_TEXTURE2D_DESC td{};
    td.Width = static_cast<UINT>(groesse);
    td.Height = static_cast<UINT>(groesse);
    td.MipLevels = 1;
    td.ArraySize = static_cast<UINT>(6 * anzahl);
    td.Format = DXGI_FORMAT_R16_FLOAT;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    td.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    if (FAILED(d->CreateTexture2D(&td, nullptr, &p.tex))) { return false; }
    p.rtv.assign(static_cast<std::size_t>(6 * anzahl), nullptr);
    for (int i = 0; i < 6 * anzahl; ++i) {
        D3D11_RENDER_TARGET_VIEW_DESC rd{};
        rd.Format = td.Format;
        rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rd.Texture2DArray.FirstArraySlice = static_cast<UINT>(i);
        rd.Texture2DArray.ArraySize = 1;
        d->CreateRenderTargetView(p.tex, &rd, &p.rtv[static_cast<std::size_t>(i)]);
        const float eins[4] = {1, 1, 1, 1};
        if (p.rtv[static_cast<std::size_t>(i)] != nullptr) { kontext()->ClearRenderTargetView(p.rtv[static_cast<std::size_t>(i)], eins); }
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC sv{};
    sv.Format = td.Format;
    sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
    sv.TextureCubeArray.MipLevels = 1;
    sv.TextureCubeArray.NumCubes = static_cast<UINT>(anzahl);
    d->CreateShaderResourceView(p.tex, &sv, &p.srv);
    td.ArraySize = 1;
    td.MiscFlags = 0;
    td.Format = DXGI_FORMAT_D32_FLOAT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    d->CreateTexture2D(&td, nullptr, &p.tiefe);
    if (p.tiefe != nullptr) { d->CreateDepthStencilView(p.tiefe, nullptr, &p.dsv); }
    return p.srv != nullptr && p.dsv != nullptr;
}

bool kartenAnlegen(Zustand::Karten& k, int groesse, int schichten) {
    ID3D11Device* d = geraet();
    k.groesse = groesse;
    k.schichten = schichten;
    D3D11_TEXTURE2D_DESC td{};
    td.Width = static_cast<UINT>(groesse);
    td.Height = static_cast<UINT>(groesse);
    td.MipLevels = 1;
    td.ArraySize = static_cast<UINT>(schichten);
    td.Format = DXGI_FORMAT_R32_FLOAT;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(d->CreateTexture2D(&td, nullptr, &k.texO)) || FAILED(d->CreateTexture2D(&td, nullptr, &k.texS))) { return false; }
    k.rtvO.assign(static_cast<std::size_t>(schichten), nullptr);
    k.rtvS.assign(static_cast<std::size_t>(schichten), nullptr);
    for (int i = 0; i < schichten; ++i) {
        D3D11_RENDER_TARGET_VIEW_DESC rd{};
        rd.Format = td.Format;
        rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rd.Texture2DArray.FirstArraySlice = static_cast<UINT>(i);
        rd.Texture2DArray.ArraySize = 1;
        d->CreateRenderTargetView(k.texO, &rd, &k.rtvO[static_cast<std::size_t>(i)]);
        d->CreateRenderTargetView(k.texS, &rd, &k.rtvS[static_cast<std::size_t>(i)]);
    }
    d->CreateShaderResourceView(k.texO, nullptr, &k.srvO);
    d->CreateShaderResourceView(k.texS, nullptr, &k.srvS);
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_D32_FLOAT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    d->CreateTexture2D(&td, nullptr, &k.tiefe);
    if (k.tiefe != nullptr) { d->CreateDepthStencilView(k.tiefe, nullptr, &k.dsv); }
    return k.srvO != nullptr && k.srvS != nullptr && k.dsv != nullptr;
}

ID3D11Buffer* pufferAnlegen(UINT bytes, DXGI_FORMAT format, UINT elemente, ID3D11ShaderResourceView** srv) {
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = bytes;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Buffer* b = nullptr;
    if (FAILED(geraet()->CreateBuffer(&bd, nullptr, &b))) { return nullptr; }
    D3D11_SHADER_RESOURCE_VIEW_DESC sv{};
    sv.Format = format;
    sv.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    sv.Buffer.FirstElement = 0;
    sv.Buffer.NumElements = elemente;
    geraet()->CreateShaderResourceView(b, &sv, srv);
    return b;
}

// blendFunc der ersten Stufe (Stufe 9c): Zustand je Faktorenpaar, einmal angelegt
ID3D11BlendState* spielMischung(int q, int z) {
    const int schluessel = q * 16 + z;
    const auto it = g.spielMischen.find(schluessel);
    if (it != g.spielMischen.end()) { return it->second; }
    static const D3D11_BLEND faktoren[11] = {D3D11_BLEND_ONE,       D3D11_BLEND_ONE,           D3D11_BLEND_ZERO,          D3D11_BLEND_SRC_ALPHA,
                                             D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_DEST_COLOR, D3D11_BLEND_INV_DEST_COLOR, D3D11_BLEND_SRC_COLOR,
                                             D3D11_BLEND_INV_SRC_COLOR, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_INV_DEST_ALPHA};
    D3D11_BLEND_DESC bd{};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = faktoren[std::clamp(q, 0, 10)];
    bd.RenderTarget[0].DestBlend = faktoren[std::clamp(z, 0, 10)];
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ID3D11BlendState* b = nullptr;
    geraet()->CreateBlendState(&bd, &b);
    g.spielMischen[schluessel] = b;
    return b != nullptr ? b : g.blend;
}

bool bereit4() {
    if (!bereit3()) { return false; }
    if (g.vs4 != nullptr) { return true; }
    if (g.gescheitert4) { return false; }
    g.gescheitert4 = true;
    ID3D11Device* d = geraet();
    if (d->GetFeatureLevel() < D3D_FEATURE_LEVEL_10_1) { return false; }   // TextureCubeArray
    const bool sm5 = d->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0;      // firstbitlow
    const D3D_SHADER_MACRO ohne[] = {{"OHNE_FBL", "1"}, {nullptr, nullptr}};
    const auto uebersetzen = [&](const char* einstieg, const char* ziel, ID3DBlob** code) {
        ID3DBlob* fehler = nullptr;
        const HRESULT hr = D3DCompile(kShader4, std::strlen(kShader4), "karte4", sm5 ? nullptr : ohne, nullptr, einstieg, ziel,
                                      D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, code, &fehler);
        if (fehler != nullptr && FAILED(hr)) {
            diag::detail(std::string("Karten-Editor: Echtzeit-Licht-Shader: ") + static_cast<const char*>(fehler->GetBufferPointer()));
        }
        los(fehler);
        return SUCCEEDED(hr);
    };
    const char* const psZiel = sm5 ? "ps_5_0" : "ps_4_1";
    const char* const vsZiel = sm5 ? "vs_5_0" : "vs_4_1";
    ID3DBlob* b[8] = {};
    if (!uebersetzen("vs4", vsZiel, &b[0]) || !uebersetzen("ps4", psZiel, &b[1]) || !uebersetzen("vss", vsZiel, &b[2]) ||
        !uebersetzen("pss", psZiel, &b[3]) || !uebersetzen("pssA", psZiel, &b[4]) || !uebersetzen("psHimmel", psZiel, &b[5]) ||
        !uebersetzen("psUnter", psZiel, &b[6]) || !uebersetzen("psUnterA", psZiel, &b[7])) {
        for (auto*& x : b) { los(x); }
        return false;
    }
    d->CreateVertexShader(b[0]->GetBufferPointer(), b[0]->GetBufferSize(), nullptr, &g.vs4);
    d->CreatePixelShader(b[1]->GetBufferPointer(), b[1]->GetBufferSize(), nullptr, &g.ps4);
    d->CreateVertexShader(b[2]->GetBufferPointer(), b[2]->GetBufferSize(), nullptr, &g.vsSchatten);
    d->CreatePixelShader(b[3]->GetBufferPointer(), b[3]->GetBufferSize(), nullptr, &g.psSchatten);
    d->CreatePixelShader(b[4]->GetBufferPointer(), b[4]->GetBufferSize(), nullptr, &g.psSchattenA);
    d->CreatePixelShader(b[5]->GetBufferPointer(), b[5]->GetBufferSize(), nullptr, &g.psHimmel);
    d->CreatePixelShader(b[6]->GetBufferPointer(), b[6]->GetBufferSize(), nullptr, &g.psUnter);
    d->CreatePixelShader(b[7]->GetBufferPointer(), b[7]->GetBufferSize(), nullptr, &g.psUnterA);
    for (auto*& x : b) { los(x); }
    D3D11_BUFFER_DESC cb{};
    cb.ByteWidth = (sizeof(Konstanten4) + 15U) & ~15U;
    cb.Usage = D3D11_USAGE_DYNAMIC;
    cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    d->CreateBuffer(&cb, nullptr, &g.cb4);
    cb.ByteWidth = sizeof(KonstantenSchatten);
    d->CreateBuffer(&cb, nullptr, &g.cbSchatten);
    g.lichtBuf = pufferAnlegen(static_cast<UINT>(kLichtMax * sizeof(LichtGpu)), DXGI_FORMAT_R32G32B32A32_FLOAT,
                               static_cast<UINT>(kLichtMax * (sizeof(LichtGpu) / 16)), &g.lichtSrv);
    const bool pools = poolAnlegen(g.pool[0], 512, kSchattenGross) && poolAnlegen(g.pool[1], 256, kSchattenKlein);
    const bool kask = kartenAnlegen(g.kaskaden, kKaskadeGroesse, 2);
    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    d->CreateSamplerState(&sd, &g.punktAbtaster);
    D3D11_RASTERIZER_DESC rs{};
    rs.FillMode = D3D11_FILL_SOLID;
    rs.CullMode = D3D11_CULL_NONE;   // Rueckseiten mitzeichnen (Stufe 9c: kein Lecken nach innen)
    rs.DepthClipEnable = TRUE;
    d->CreateRasterizerState(&rs, &g.rsSchatten);
    g.gescheitert4 = g.vs4 == nullptr || g.ps4 == nullptr || g.vsSchatten == nullptr || g.psSchatten == nullptr || g.psSchattenA == nullptr ||
                     g.psHimmel == nullptr || g.psUnter == nullptr || g.psUnterA == nullptr || g.cb4 == nullptr || g.cbSchatten == nullptr ||
                     g.lichtSrv == nullptr || !pools || !kask || g.punktAbtaster == nullptr || g.rsSchatten == nullptr;
    return !g.gescheitert4;
}

// Zeilenweise Matrizen (wie row_major im Shader: clip = p * M)
void achsen(const float* vorn, const float* oben, float* x, float* y, float* z) {
    const auto norm = [](float* v) {
        const float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (l > 0) { v[0] /= l, v[1] /= l, v[2] /= l; }
    };
    z[0] = vorn[0];
    z[1] = vorn[1];
    z[2] = vorn[2];
    norm(z);
    x[0] = oben[1] * z[2] - oben[2] * z[1];
    x[1] = oben[2] * z[0] - oben[0] * z[2];
    x[2] = oben[0] * z[1] - oben[1] * z[0];
    norm(x);
    y[0] = z[1] * x[2] - z[2] * x[1];
    y[1] = z[2] * x[0] - z[0] * x[2];
    y[2] = z[0] * x[1] - z[1] * x[0];
}
void blickLH(const float* auge, const float* vorn, const float* oben, float* m) {
    float x[3];
    float y[3];
    float z[3];
    achsen(vorn, oben, x, y, z);
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

std::size_t g_schattenDreiecke = 0;   // gezeichnete Dreiecke aller Schattenkarten (Budget je Bild)

// Wuerfelseite (Achse, Vorzeichen) vom Licht aus: der Kasten liegt (teilweise) im 90-Grad-Stumpf
bool seiteTrifft(const SchattenTeil& t, const float* seite) {
    if (seite == nullptr) { return true; }
    const int a = static_cast<int>(seite[3]);
    const float vz = seite[4];
    const float vorn = vz > 0 ? t.mx[a] - seite[a] : seite[a] - t.mn[a];
    if (vorn <= 0) { return false; }
    for (int i = 0; i < 3; ++i) {
        if (i == a) { continue; }
        const float m = seite[i] < t.mn[i] ? t.mn[i] - seite[i] : seite[i] > t.mx[i] ? seite[i] - t.mx[i] : 0.0F;
        if (m > vorn) { return false; }
    }
    return true;
}

bool kugelTrifft(const SchattenTeil& t, const float* kugel) {
    if (kugel == nullptr) { return true; }
    float d2 = 0;
    for (int i = 0; i < 3; ++i) {
        const float v = kugel[i] < t.mn[i] ? t.mn[i] - kugel[i] : kugel[i] > t.mx[i] ? kugel[i] - t.mx[i] : 0.0F;
        d2 += v * v;
    }
    return d2 <= kugel[3] * kugel[3];
}

// Die Teile der Arten in maske (Bit je Art) zeichnen; ps fuer deckende, psA fuer alphashadow.
void schattenZiehen(const KonstantenSchatten& k, ID3D11PixelShader* ps, ID3D11PixelShader* psA, unsigned maske, const std::vector<SchattenTeil>& teile,
                    const float* kugel, ID3D11DepthStencilState* tiefe, ID3D11ShaderResourceView* srvS, const float* seite = nullptr) {
    ID3D11DeviceContext* c = kontext();
    D3D11_MAPPED_SUBRESOURCE m{};
    if (FAILED(c->Map(g.cbSchatten, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) { return; }
    std::memcpy(m.pData, &k, sizeof(k));
    c->Unmap(g.cbSchatten, 0);
    c->IASetInputLayout(g.layout3);
    c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    c->VSSetShader(g.vsSchatten, nullptr, 0);
    c->GSSetShader(nullptr, nullptr, 0);
    c->VSSetConstantBuffers(2, 1, &g.cbSchatten);
    c->PSSetConstantBuffers(2, 1, &g.cbSchatten);
    ID3D11ShaderResourceView* srv[2] = {nullptr, srvS};
    c->PSSetShaderResources(0, 2, srv);
    c->PSSetSamplers(0, 1, &g.abtaster);
    const float faktor[4] = {0, 0, 0, 0};
    c->OMSetBlendState(g.blendAus, faktor, 0xFFFFFFFFU);
    c->OMSetDepthStencilState(tiefe, 0);
    c->RSSetState(g.rsSchatten);
    ID3D11PixelShader* aktuell = nullptr;
    for (const SchattenTeil& t : teile) {
        if (t.puffer == nullptr || t.anzahl == 0 || ((1U << t.art) & maske) == 0U || !kugelTrifft(t, kugel) || !seiteTrifft(t, seite)) { continue; }
        ID3D11Buffer* vb = Zugriff3::vb(*t.puffer);
        if (vb == nullptr) { continue; }
        ID3D11PixelShader* soll = t.art == 3 ? psA : ps;
        if (soll != aktuell) {
            c->PSSetShader(soll, nullptr, 0);
            aktuell = soll;
        }
        if (t.art == 3) {
            ID3D11ShaderResourceView* b = static_cast<ID3D11ShaderResourceView*>(t.bild);
            c->PSSetShaderResources(0, 1, &b);
        }
        const UINT schritt = sizeof(Punkt3);
        const UINT versatz = 0;
        c->IASetVertexBuffers(0, 1, &vb, &schritt, &versatz);
        c->Draw(static_cast<UINT>(t.anzahl), static_cast<UINT>(t.erstes));
        g_schattenDreiecke += t.anzahl / 3;
    }
    ID3D11ShaderResourceView* leer[2] = {nullptr, nullptr};
    c->PSSetShaderResources(0, 2, leer);
}

// Eine Sonnenkarte (S, dann O) fuer die Schicht schicht von k: richtung zur Sonne
bool sonnenKarte(Zustand::Karten& k, int schicht, const float* richtung, const float* mitte, float halb, float tiefe,
                 const std::vector<SchattenTeil>& teile, float* vpAus) {
    if (schicht < 0 || schicht >= k.schichten || halb <= 0 || tiefe <= 0) { return false; }
    ID3D11DeviceContext* c = kontext();
    RtSicherung sicher;
    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(k.groesse);
    vp.Height = static_cast<float>(k.groesse);
    vp.MaxDepth = 1;
    c->RSSetViewports(1, &vp);
    const float auge[3] = {mitte[0] + richtung[0] * tiefe, mitte[1] + richtung[1] * tiefe, mitte[2] + richtung[2] * tiefe};
    const float vorn[3] = {-richtung[0], -richtung[1], -richtung[2]};
    const float oben[3] = {std::fabs(richtung[2]) > 0.99F ? 1.0F : 0.0F, 0.0F, std::fabs(richtung[2]) > 0.99F ? 0.0F : 1.0F};
    float blick[16];
    blickLH(auge, vorn, oben, blick);
    const float ortho[16] = {1.0F / halb, 0, 0, 0, 0, 1.0F / halb, 0, 0, 0, 0, 1.0F / (2.0F * tiefe), 0, 0, 0, 0, 1};
    KonstantenSchatten ks{};
    mal(blick, ortho, ks.vp);
    std::memcpy(vpAus, ks.vp, sizeof(ks.vp));
    for (int i = 0; i < 3; ++i) { ks.zurSonne[i] = richtung[i]; }
    ks.werte[1] = static_cast<float>(schicht);
    ks.werte[2] = 2.0F * halb / static_cast<float>(k.groesse) + 1.0F;
    // S: die Himmelsflaechen, die von der Sonne wegzeigen - die groesste Tiefe (die der Stelle naechste)
    ID3D11RenderTargetView* rs = k.rtvS[static_cast<std::size_t>(schicht)];
    c->OMSetRenderTargets(1, &rs, k.dsv);
    const float keinHimmel[4] = {-1.0e30F, 0, 0, 0};
    c->ClearRenderTargetView(rs, keinHimmel);
    c->ClearDepthStencilView(k.dsv, D3D11_CLEAR_DEPTH, 0.0F, 0);
    schattenZiehen(ks, g.psHimmel, g.psHimmel, 1U << 2, teile, nullptr, tiefenZustand(TiefenTest::Groesser, true), nullptr);
    // O: deckende Dreiecke, Volumen und alphashadow unter dem Himmel - die kleinste Tiefe
    ID3D11RenderTargetView* ro = k.rtvO[static_cast<std::size_t>(schicht)];
    c->OMSetRenderTargets(1, &ro, k.dsv);
    const float nichts[4] = {1.0e30F, 0, 0, 0};
    c->ClearRenderTargetView(ro, nichts);
    c->ClearDepthStencilView(k.dsv, D3D11_CLEAR_DEPTH, 1.0F, 0);
    schattenZiehen(ks, g.psUnter, g.psUnterA, (1U << 0) | (1U << 1) | (1U << 3), teile, nullptr, tiefenZustand(TiefenTest::Kleiner, true), k.srvS);
    return true;
}

}  // namespace

bool echtLichtVerfuegbar() { return verfuegbar() && bereit4(); }

void lichtSetzen(const std::vector<LichtGpu>& lichter, const std::vector<float>& ecken, const std::vector<std::uint32_t>& kacheln, int kachelnX,
                 int kachelnY, int worte, const float* umgebung, const float* minimum) {
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
    k->sonne[0] = g.kaskadeAn[0] ? 1.0F : 0.0F;
    k->sonne[1] = g.kaskadeAn[0] && g.kaskadeAn[1] ? 1.0F : 0.0F;
    k->sonne[2] = g.kaskadeTexel[0];
    k->sonne[3] = g.kaskadeTexel[1];
    k->himmelInfo[0] = g.himmelTexel;
    const std::size_t n = std::min<std::size_t>(lichter.size(), kLichtMax);
    k->anzahl[0] = static_cast<int>(n);
    k->anzahl[1] = kachelnX;
    k->anzahl[2] = kachelnY;
    k->anzahl[3] = kacheln.empty() ? 0 : worte;
    std::memcpy(k->kaskadeVp, g.kaskadeVp, sizeof(k->kaskadeVp));
    std::memcpy(k->himmelVp, g.himmelVp, sizeof(k->himmelVp));
    c->Unmap(g.cb4, 0);
    if (n > 0 && SUCCEEDED(c->Map(g.lichtBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
        std::memcpy(m.pData, lichter.data(), n * sizeof(LichtGpu));
        c->Unmap(g.lichtBuf, 0);
    }
    // Ecken der Flaechenlichter (float4 je Ecke)
    const std::size_t eckenZahl = std::max<std::size_t>(1, ecken.size() / 3);
    if (g.eckenBuf == nullptr || g.eckenPlatz < eckenZahl) {
        los(g.eckenBuf);
        los(g.eckenSrv);
        g.eckenPlatz = std::max<std::size_t>(eckenZahl * 2, 1024);
        g.eckenBuf = pufferAnlegen(static_cast<UINT>(g.eckenPlatz * 16), DXGI_FORMAT_R32G32B32A32_FLOAT, static_cast<UINT>(g.eckenPlatz), &g.eckenSrv);
    }
    if (g.eckenBuf != nullptr && ecken.size() >= 3 && SUCCEEDED(c->Map(g.eckenBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
        auto* f = static_cast<float*>(m.pData);
        for (std::size_t i = 0; i < ecken.size() / 3; ++i) {
            f[i * 4 + 0] = ecken[i * 3 + 0];
            f[i * 4 + 1] = ecken[i * 3 + 1];
            f[i * 4 + 2] = ecken[i * 3 + 2];
            f[i * 4 + 3] = 1.0F;
        }
        c->Unmap(g.eckenBuf, 0);
    }
    const std::size_t kz = std::max<std::size_t>(1, kacheln.size());
    if (g.kachelBuf == nullptr || g.kachelPlatz < kz) {
        los(g.kachelBuf);
        los(g.kachelSrv);
        g.kachelPlatz = std::max<std::size_t>(kz * 2, 4096);
        g.kachelBuf = pufferAnlegen(static_cast<UINT>(g.kachelPlatz * 4), DXGI_FORMAT_R32_UINT, static_cast<UINT>(g.kachelPlatz), &g.kachelSrv);
    }
    if (g.kachelBuf != nullptr && !kacheln.empty() && SUCCEEDED(c->Map(g.kachelBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) {
        std::memcpy(m.pData, kacheln.data(), kacheln.size() * sizeof(std::uint32_t));
        c->Unmap(g.kachelBuf, 0);
    }
}

bool schattenWuerfel(int slot, const float* pos, float weite, const std::vector<SchattenTeil>& teile) {
    if (slot < 0 || slot >= kSchattenMax || !bereit4() || weite <= 1.0F) { return false; }
    Zustand::Pool& p = g.pool[slot < kSchattenGross ? 0 : 1];
    const int platz = slot < kSchattenGross ? slot : slot - kSchattenGross;
    ID3D11DeviceContext* c = kontext();
    RtSicherung sicher;
    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(p.groesse);
    vp.Height = static_cast<float>(p.groesse);
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
    KonstantenSchatten ks{};
    ks.licht[0] = pos[0];
    ks.licht[1] = pos[1];
    ks.licht[2] = pos[2];
    ks.licht[3] = weite;
    const float kugel[4] = {pos[0], pos[1], pos[2], weite};
    for (int s = 0; s < 6; ++s) {
        ID3D11RenderTargetView* rtv = p.rtv[static_cast<std::size_t>(platz * 6 + s)];
        if (rtv == nullptr) { continue; }
        c->OMSetRenderTargets(1, &rtv, p.dsv);
        const float eins[4] = {1, 1, 1, 1};
        c->ClearRenderTargetView(rtv, eins);
        c->ClearDepthStencilView(p.dsv, D3D11_CLEAR_DEPTH, 1.0F, 0);
        float blick[16];
        blickLH(pos, vorn[s], oben[s], blick);
        mal(blick, proj, ks.vp);
        // Punkt-, Spot- und Flaechenlichter: TraceLine ohne testAll - auch feste Blaetter halten auf
        // die Himmelspinsel sind feste Blaetter (Art 2 und 4) - fuer Punktlichter halten sie auf
        const float seite[5] = {pos[0], pos[1], pos[2], static_cast<float>(s / 2), (s % 2) == 0 ? 1.0F : -1.0F};
        schattenZiehen(ks, g.psSchatten, g.psSchattenA, (1U << 0) | (1U << 1) | (1U << 2) | (1U << 3) | (1U << 4), teile, kugel,
                       tiefenZustand(TiefenTest::Kleiner, true), nullptr, seite);
    }
    return true;
}

bool sonnenKaskade(int stufe, const float* richtung, const float* mitte, float halb, float tiefe, const std::vector<SchattenTeil>& teile) {
    if (stufe < 0 || stufe > 1 || !bereit4()) { return false; }
    float m[3] = {mitte[0], mitte[1], mitte[2]};
    const float texel = 2.0F * halb / static_cast<float>(kKaskadeGroesse);
    if (stufe == 1) {
        // auf ganze Texel der Sonnenebene runden (kein Flimmern beim Bewegen)
        const float vorn[3] = {-richtung[0], -richtung[1], -richtung[2]};
        const float oben[3] = {std::fabs(richtung[2]) > 0.99F ? 1.0F : 0.0F, 0.0F, std::fabs(richtung[2]) > 0.99F ? 0.0F : 1.0F};
        float x[3];
        float y[3];
        float z[3];
        achsen(vorn, oben, x, y, z);
        const float px = m[0] * x[0] + m[1] * x[1] + m[2] * x[2];
        const float py = m[0] * y[0] + m[1] * y[1] + m[2] * y[2];
        const float dx = std::round(px / texel) * texel - px;
        const float dy = std::round(py / texel) * texel - py;
        for (int i = 0; i < 3; ++i) { m[i] += x[i] * dx + y[i] * dy; }
    }
    if (!sonnenKarte(g.kaskaden, stufe, richtung, m, halb, tiefe, teile, g.kaskadeVp[stufe])) { return false; }
    g.kaskadeTexel[stufe] = texel;
    g.kaskadeAn[stufe] = true;
    return true;
}

bool himmelsSonne(int schicht, const float* richtung, const float* mitte, float halb, float tiefe, const std::vector<SchattenTeil>& teile) {
    if (schicht < 0 || schicht >= kHimmelSonnenMax || !bereit4()) { return false; }
    if (g.himmelK.texO == nullptr && !kartenAnlegen(g.himmelK, 1024, kHimmelSonnenMax)) {
        kartenLoesen(g.himmelK);
        return false;
    }
    if (!sonnenKarte(g.himmelK, schicht, richtung, mitte, halb, tiefe, teile, g.himmelVp[schicht])) { return false; }
    g.himmelTexel = 2.0F * halb / 1024.0F;
    return true;
}

std::size_t schattenDreiecke() { return g_schattenDreiecke; }

void sonnenSchattenAus() {
    g.kaskadeAn[0] = false;
    g.kaskadeAn[1] = false;
}



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

// Stufe 9c: das ganze aufgeloeste Renderziel (RGBA, zeilenweise von oben) fuer die Lichtmessung
bool bildLesen(int kennung, std::vector<unsigned char>& rgba, int& breite, int& hoehe) {
    const auto it = g.ziele.find(kennung);
    if (it == g.ziele.end() || g.stand != render::texturStand() || it->second.aufgeloest == nullptr) { return false; }
    const Ziel& z = it->second;
    D3D11_TEXTURE2D_DESC td{};
    z.aufgeloest->GetDesc(&td);
    td.BindFlags = 0;
    td.MiscFlags = 0;
    td.Usage = D3D11_USAGE_STAGING;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    if (td.Format != DXGI_FORMAT_R8G8B8A8_UNORM) { return false; }
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(geraet()->CreateTexture2D(&td, nullptr, &tex))) { return false; }
    kontext()->CopyResource(tex, z.aufgeloest);
    D3D11_MAPPED_SUBRESOURCE m{};
    const bool ok = SUCCEEDED(kontext()->Map(tex, 0, D3D11_MAP_READ, 0, &m));
    if (ok) {
        breite = static_cast<int>(td.Width);
        hoehe = static_cast<int>(td.Height);
        rgba.resize(static_cast<std::size_t>(breite) * static_cast<std::size_t>(hoehe) * 4U);
        for (int y = 0; y < hoehe; ++y) {
            std::memcpy(rgba.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(breite) * 4U,
                        static_cast<const unsigned char*>(m.pData) + static_cast<std::size_t>(y) * m.RowPitch, static_cast<std::size_t>(breite) * 4U);
        }
        kontext()->Unmap(tex, 0);
    }
    tex->Release();
    return ok;
}

}  // namespace bhed::gui::kartegpu
