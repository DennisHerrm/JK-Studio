// gpumap_formen.inc - AP13K (rc612): Knochen und Griffe als Formen im Bild des Modellfensters, Durchsicht der Figur
// (eingebunden in gpumap_win32.cpp im unbenannten Namensraum, vor zeichneModellAnsicht)
//
// Formen: ModellAnsichtArt::formen sind fertige Dreiecke im Clip-Raum mit Farbe je Ecke (die Oberflaeche rechnet Linien zu
// Streifen in Bildpunkten mit weichem Rand - gui/app_modelltab_knochen.cpp). Der Durchgang reicht sie durch, mischt mit Alpha in
// die Farbe (das Ziel behaelt Alpha 1) und prueft die Tiefe gar nicht (im Vordergrund) oder mit LESS_EQUAL, ohne zu schreiben
// (die Figur verdeckt). Um wenige Schritte der Tiefe zur Kamera gezogen wie das Raster, damit eine Form auf dem grauen Boden
// nicht flimmert.
//
// Durchsicht (3ds Max See-Through, Blender X-Ray): vor der Figur wird das Ziel in eine zweite Textur kopiert ("hinten"), die
// Tiefe geloescht und die Figur wie immer deckend gezeichnet - alle Stufen ihrer Shader, Anbauten, Kanten. Danach legt ein
// Vollbilddreieck bei z = 1 mit Tiefentest GREATER die Kopie mit Alpha 1 - Deckkraft darueber: nur wo die Figur Tiefe schrieb,
// jeder Bildpunkt einmal, also Farbe = Figur * Deckkraft + hinten * (1 - Deckkraft). Dieselben Befehle unter Direct3D 11 und
// Direct3D 12 (CopyResource, ein Zeichenaufruf). Das Mischen setzt dabei das Bit kStencilFigur im Stencil: Formen, die nicht im
// Vordergrund liegen, stehen schon vor der Kopie im Ziel (hinter der Figur getoent); danach kommen sie nur noch dort dazu, wo die
// Figur steht und sie davor liegen - neben der Figur waeren sie sonst zweimal gemischt.

ID3D11VertexShader* g_formVs = nullptr;
ID3D11PixelShader* g_formPs = nullptr;
ID3D11InputLayout* g_formLayout = nullptr;
ID3D11Buffer* g_formVb = nullptr;
std::size_t g_formVbEcken = 0;
ID3D11BlendState* g_formMischen = nullptr;          // mit Alpha, nur RGB
ID3D11DepthStencilState* g_dsFormVorn = nullptr;    // ohne Tiefentest
ID3D11DepthStencilState* g_dsFormTiefe = nullptr;   // LESS_EQUAL, schreibt nicht
ID3D11DepthStencilState* g_dsFormFigur = nullptr;   // LESS_EQUAL, nur wo das Mischen der Durchsicht kStencilFigur setzte
constexpr UINT kStencilFigur = 0x80U;               // ein eigenes Bit; die Ghosts loeschen den Stencil vor ihrem Gebrauch selbst
ID3D11RasterizerState* g_rsForm = nullptr;
ID3D11VertexShader* g_durchVs = nullptr;
ID3D11PixelShader* g_durchPs = nullptr;
ID3D11Buffer* g_cbDurch = nullptr;
ID3D11DepthStencilState* g_dsDurch = nullptr;       // GREATER gegen z = 1: nur wo die Figur Tiefe schrieb
ID3D11Texture2D* g_durchTex = nullptr;              // das Ziel vor der Figur
ID3D11ShaderResourceView* g_durchSrv = nullptr;
int g_durchW = 0;
int g_durchH = 0;
bool g_formVersucht = false;

bool formenAnlegen(ID3D11Device* d, std::string* fehler) {
    const auto fertig = [] {
        return g_formVs != nullptr && g_formPs != nullptr && g_formLayout != nullptr && g_formMischen != nullptr && g_dsFormVorn != nullptr &&
               g_dsFormTiefe != nullptr && g_dsFormFigur != nullptr && g_rsForm != nullptr && g_durchVs != nullptr && g_durchPs != nullptr && g_cbDurch != nullptr && g_dsDurch != nullptr;
    };
    if (g_formVersucht) { return fertig(); }
    g_formVersucht = true;
    const std::string formVs = R"(
struct VSIn  { float4 pos : POSITION;    float4 farbe : COLOR0; };
struct VSOut { float4 pos : SV_POSITION; float4 farbe : COLOR0; };
VSOut main(VSIn ein) {
    VSOut aus;
    aus.pos = ein.pos;
    aus.farbe = ein.farbe;
    return aus;
}
)";
    const std::string formPs = R"(
struct VSOut { float4 pos : SV_POSITION; float4 farbe : COLOR0; };
float4 main(VSOut ein) : SV_TARGET { return ein.farbe; }
)";
    // ein Dreieck ueber das ganze Bild bei z = 1 (die ferne Ebene): der Tiefentest GREATER laesst es nur, wo die Figur steht
    const std::string durchVs = R"(
float4 main(uint nr : SV_VertexID) : SV_POSITION {
    float2 p = float2((nr << 1) & 2, nr & 2);
    return float4(p * float2(2.0, -2.0) + float2(-1.0, 1.0), 1.0, 1.0);
}
)";
    const std::string durchPs = R"(
cbuffer Durch : register(b7) { float4 gDurch; };   // x: 1 - Deckkraft der Figur
Texture2D gHinten : register(t0);
float4 main(float4 pos : SV_POSITION) : SV_TARGET { return float4(gHinten.Load(int3(pos.xy, 0)).rgb, gDurch.x); }
)";
    ID3DBlob* vs = uebersetze(formVs, "vs_4_0", "modellform", fehler);
    ID3DBlob* ps = uebersetze(formPs, "ps_4_0", "modellform", fehler);
    if (vs != nullptr) {
        d->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &g_formVs);
        const D3D11_INPUT_ELEMENT_DESC ein[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        d->CreateInputLayout(ein, 2, vs->GetBufferPointer(), vs->GetBufferSize(), &g_formLayout);
        vs->Release();
    }
    if (ps != nullptr) {
        d->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &g_formPs);
        ps->Release();
    }
    vs = uebersetze(durchVs, "vs_4_0", "modelldurch", fehler);
    ps = uebersetze(durchPs, "ps_4_0", "modelldurch", fehler);
    if (vs != nullptr) {
        d->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &g_durchVs);
        vs->Release();
    }
    if (ps != nullptr) {
        d->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &g_durchPs);
        ps->Release();
    }
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = 16;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    d->CreateBuffer(&bd, nullptr, &g_cbDurch);
    D3D11_BLEND_DESC bl{};
    bl.RenderTarget[0].BlendEnable = TRUE;
    bl.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bl.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bl.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bl.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bl.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    bl.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bl.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
    d->CreateBlendState(&bl, &g_formMischen);
    D3D11_DEPTH_STENCIL_DESC dd{};
    dd.DepthEnable = FALSE;
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dd.DepthFunc = D3D11_COMPARISON_ALWAYS;
    d->CreateDepthStencilState(&dd, &g_dsFormVorn);
    dd.DepthEnable = TRUE;
    dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    d->CreateDepthStencilState(&dd, &g_dsFormTiefe);
    dd.StencilEnable = TRUE;
    dd.StencilReadMask = static_cast<UINT8>(kStencilFigur);
    dd.StencilWriteMask = 0;
    dd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    dd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dd.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
    dd.BackFace = dd.FrontFace;
    d->CreateDepthStencilState(&dd, &g_dsFormFigur);
    dd.DepthFunc = D3D11_COMPARISON_GREATER;
    dd.StencilReadMask = 0;
    dd.StencilWriteMask = static_cast<UINT8>(kStencilFigur);
    dd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    dd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    dd.BackFace = dd.FrontFace;
    d->CreateDepthStencilState(&dd, &g_dsDurch);
    // wie das Raster (g_rsRaster): um 16 Schritte der Tiefe (D24: 16 / 2^24) und die Neigung eines Bildpunkts zur Kamera
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthBias = -16;
    rd.SlopeScaledDepthBias = -1.0F;
    rd.DepthClipEnable = TRUE;
    d->CreateRasterizerState(&rd, &g_rsForm);
    return fertig();
}

// Wie die Formen gegen die Figur stehen
enum class FormTiefe { Vorn, Verdeckt, NurAufFigur };

// Die Dreiecke der Formen in das gesetzte Ziel. Vorn: ohne Tiefentest; Verdeckt: die Figur verdeckt sie; NurAufFigur: wie
// Verdeckt, aber nur, wo die Durchsicht die Figur gemischt hat (kStencilFigur).
int zeichneFormen(ID3D11Device* d, ID3D11DeviceContext* c, const std::vector<ModellFormEcke>& ecken, FormTiefe tiefe) {
    const std::size_t n = ecken.size() - ecken.size() % 3U;
    if (n == 0) { return 0; }
    if (g_formVb == nullptr || g_formVbEcken < n) {
        if (g_formVb != nullptr) {
            g_formVb->Release();
            g_formVb = nullptr;
        }
        g_formVbEcken = std::max<std::size_t>(n + n / 2U, 4096U);
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = static_cast<UINT>(g_formVbEcken * sizeof(ModellFormEcke));
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(d->CreateBuffer(&bd, nullptr, &g_formVb))) {
            g_formVb = nullptr;
            g_formVbEcken = 0;
            return 0;
        }
    }
    D3D11_MAPPED_SUBRESOURCE m{};
    if (FAILED(c->Map(g_formVb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) { return 0; }
    std::memcpy(m.pData, ecken.data(), n * sizeof(ModellFormEcke));
    c->Unmap(g_formVb, 0);
    const UINT schritt = sizeof(ModellFormEcke);
    const UINT versatz = 0;
    c->IASetInputLayout(g_formLayout);
    c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    c->IASetVertexBuffers(0, 1, &g_formVb, &schritt, &versatz);
    c->VSSetShader(g_formVs, nullptr, 0);
    c->GSSetShader(nullptr, nullptr, 0);
    c->PSSetShader(g_formPs, nullptr, 0);
    const float faktor[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    c->OMSetBlendState(g_formMischen, faktor, 0xFFFFFFFFU);
    if (tiefe == FormTiefe::NurAufFigur) {
        c->OMSetDepthStencilState(g_dsFormFigur, kStencilFigur);
    } else {
        c->OMSetDepthStencilState(tiefe == FormTiefe::Verdeckt ? g_dsFormTiefe : g_dsFormVorn, 0);
    }
    c->RSSetState(g_rsForm);
    c->Draw(static_cast<UINT>(n), 0);
    return 1;
}

// Die zweite Textur der Durchsicht in der Groesse des gewaehlten Modellziels
bool durchsichtZiel(ID3D11Device* d) {
    if (g_durchTex != nullptr && g_durchW == g_modellW && g_durchH == g_modellH) { return true; }
    if (g_durchSrv != nullptr) {
        g_durchSrv->Release();
        g_durchSrv = nullptr;
    }
    if (g_durchTex != nullptr) {
        g_durchTex->Release();
        g_durchTex = nullptr;
    }
    g_durchW = 0;
    g_durchH = 0;
    D3D11_TEXTURE2D_DESC td{};
    td.Width = static_cast<UINT>(g_modellW);
    td.Height = static_cast<UINT>(g_modellH);
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(d->CreateTexture2D(&td, nullptr, &g_durchTex)) || FAILED(::bhed::render::sichtAnlegen(d, g_durchTex, nullptr, &g_durchSrv))) {
        if (g_durchTex != nullptr) {
            g_durchTex->Release();
            g_durchTex = nullptr;
        }
        return false;
    }
    g_durchW = g_modellW;
    g_durchH = g_modellH;
    return true;
}

// Die Kopie von vor der Figur mit Alpha 1 - deckkraft ueber das Ziel, nur wo die Figur Tiefe schrieb
int durchsichtMischen(ID3D11DeviceContext* c, float deckkraft) {
    const float werte[4] = {1.0F - std::clamp(deckkraft, 0.0F, 1.0F), 0.0F, 0.0F, 0.0F};
    D3D11_MAPPED_SUBRESOURCE m{};
    if (FAILED(c->Map(g_cbDurch, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) { return 0; }
    std::memcpy(m.pData, werte, sizeof werte);
    c->Unmap(g_cbDurch, 0);
    c->IASetInputLayout(nullptr);
    c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    c->VSSetShader(g_durchVs, nullptr, 0);
    c->GSSetShader(nullptr, nullptr, 0);
    c->PSSetShader(g_durchPs, nullptr, 0);
    c->PSSetConstantBuffers(7, 1, &g_cbDurch);
    c->PSSetShaderResources(0, 1, &g_durchSrv);
    const float faktor[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    c->OMSetBlendState(g_formMischen, faktor, 0xFFFFFFFFU);
    c->OMSetDepthStencilState(g_dsDurch, kStencilFigur);
    ID3D11RasterizerState* rs = holeRaster(kCullNone);
    if (rs != nullptr) { c->RSSetState(rs); }
    c->Draw(3, 0);
    ID3D11ShaderResourceView* keine = nullptr;
    c->PSSetShaderResources(0, 1, &keine);
    return 1;
}

void gibFormenFrei() {
    for (IUnknown** p : {reinterpret_cast<IUnknown**>(&g_formVs), reinterpret_cast<IUnknown**>(&g_formPs), reinterpret_cast<IUnknown**>(&g_formLayout),
                         reinterpret_cast<IUnknown**>(&g_formVb), reinterpret_cast<IUnknown**>(&g_formMischen), reinterpret_cast<IUnknown**>(&g_dsFormVorn),
                         reinterpret_cast<IUnknown**>(&g_dsFormTiefe), reinterpret_cast<IUnknown**>(&g_dsFormFigur), reinterpret_cast<IUnknown**>(&g_rsForm), reinterpret_cast<IUnknown**>(&g_durchVs),
                         reinterpret_cast<IUnknown**>(&g_durchPs), reinterpret_cast<IUnknown**>(&g_cbDurch), reinterpret_cast<IUnknown**>(&g_dsDurch),
                         reinterpret_cast<IUnknown**>(&g_durchSrv), reinterpret_cast<IUnknown**>(&g_durchTex)}) {
        if (*p != nullptr) {
            (*p)->Release();
            *p = nullptr;
        }
    }
    g_formVbEcken = 0;
    g_durchW = 0;
    g_durchH = 0;
    g_formVersucht = false;
}
