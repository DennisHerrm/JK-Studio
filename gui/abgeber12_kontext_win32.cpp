// abgeber12_kontext_win32.cpp - D3D12-Abgeber: der Kontext (ID3D11DeviceContext1)
//
// Aufbau und Gruende in abgeber12.h. Hier: jeder Befehl der Zeichenschichten wird sofort zu Befehlen der einen D3D12-Liste des
// Bildes. Bindungen merkt sich der Kontext und schreibt vor jedem Zeichenbefehl nur das Geaenderte (vorZeichnen):
//   - Shader, Eingabe, Misch-, Tiefen- und Rasterzustand, Zielformate, Probenzahl -> ein Pipeline-Zustand (PSO), nach Schluessel
//     gemerkt und beim ersten Gebrauch angelegt,
//   - Konstantenpuffer -> Root-CBV mit Adresse (Block + erste Konstante * 16; ohne Bindung eine Seite Nullen),
//   - Texturen und Abtaster -> je Platz eine Tabelle mit einem festen Deskriptor (ohne Bindung die leere Sicht),
//   - Renderziele -> Zustandswechsel der Texturen (Ziel <-> lesbar) und OMSetRenderTargets.
// Wie Direct3D 11: eine Textur, die Ziel wird, faellt aus den Textur-Plaetzen, und umgekehrt; gebundene Objekte leben weiter,
// bis sie geloest sind.

#include <windows.h>

#include "abgeber12_intern.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <d3dcompiler.h>
#include <intrin.h>

#include "bhed/diag.h"
#include "bhed/utf8pfad.h"
#include "platform.h"

namespace bhed::render::d3d12::abgeber {

namespace {

enum : std::uint32_t {
    kSZiele = 1U << 0U,
    kSPso = 1U << 1U,
    kSTopologie = 1U << 2U,
    kSVb = 1U << 3U,
    kSIb = 1U << 4U,
    kSSichtfeld = 1U << 5U,
    kSSchere = 1U << 6U,
    kSFaktor = 1U << 7U,
    kSStencil = 1U << 8U,
    kSSrvZustand = 1U << 9U,   // eine gebundene Textur steht vielleicht nicht im Lesezustand
    kSWurzel = 1U << 10U,
};

constexpr D3D12_RESOURCE_STATES kLesbar = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
// 9 Vertex (Bits 0..8), 9 Pixel (10..18), Geometrie b0 und b5 (20, 25)
constexpr std::uint32_t kCbMaske = 0x1FFU | (0x1FFU << kCbSchritt) | (1U << (2U * kCbSchritt)) | (1U << (2U * kCbSchritt + 5U));

// Wurzelparameter eines Konstantenpuffer-Registers; ~0U: die Wurzelsignatur hat dort keinen Platz
constexpr UINT wurzelCb(UINT stufe, UINT reg) {
    if (reg >= kCbPlaetze) { return ~0U; }
    if (stufe == 0U) { return kWurzelCbVs + reg; }
    if (stufe == 1U) { return kWurzelCbPs + reg; }
    for (UINT i = 0; i < kGsCbPlaetze; ++i) {
        if (kGsCbRegister[i] == reg) { return kWurzelCbGs + i; }
    }
    return ~0U;
}
static_assert(wurzelCb(2, 0) == kWurzelCbGs && wurzelCb(2, 5) == kWurzelCbGs + 1U && wurzelCb(2, 1) == ~0U && wurzelCb(1, 8) == kWurzelCbPs + 8U &&
                  kWurzelCbGs + kGsCbPlaetze == kWurzelSrv,
              "Konstantenpuffer-Plaetze und Wurzelsignatur passen zusammen");
constexpr std::uint32_t kSrvMaske = (1U << kSrvPlaetze) - 1U;
constexpr std::uint32_t kAbtasterMaske = (1U << kAbtasterPlaetze) - 1U;

struct PsoHash {
    std::size_t operator()(const PsoSchluessel& k) const noexcept {
        // Sechs 64-Bit-Worte (der Schluessel hat keine Luecken), je Wort gemischt wie FNV-1a - ein Sechstel der Schritte von
        // Byte fuer Byte (im Profil des Karten-Editors waren das 2,5 % des Hauptfadens bei 1143 Wechseln je Bild).
        std::uint64_t w[6];
        std::memcpy(w, &k, sizeof(w));
        std::uint64_t h = 1469598103934665603ULL;
        for (const std::uint64_t x : w) {
            h ^= x;
            h *= 1099511628211ULL;
            h ^= h >> 29U;
        }
        return static_cast<std::size_t>(h);
    }
};
static_assert(sizeof(PsoSchluessel) == 48, "PsoSchluessel ohne Luecken");

std::unordered_map<PsoSchluessel, ID3D12PipelineState*, PsoHash> g_pso;
constexpr std::size_t kZaunRing = 4096;   // so viele Ausfuehrungen zurueck findet eine Abfrage oder ein Lesepuffer ihren Zaunwert
std::uint64_t g_zaunVon[kZaunRing]{};     // Zaunwert der Ausfuehrung (rahmen::ausfuehrung() % kZaunRing) ...
std::uint64_t g_zaunSerie[kZaunRing]{};   // ... und welche Ausfuehrung dort steht (+1; 0 = keine)
// GenerateMips
ID3DBlob* g_mipVs = nullptr;
ID3DBlob* g_mipPs = nullptr;
std::map<int, ID3D12PipelineState*> g_mipPso;
UINT64 g_markenFrequenz = 0;   // Schritte der Zeitmarken je Sekunde (fest je Warteschlange; die Frage ist ein Kernaufruf)

// --- Zwischenspeicher der Pipeline-Zustaende auf der Platte ------------------------------------------
// Ein Pipeline-Zustand entsteht beim ersten Gebrauch (CreateGraphicsPipelineState: der Treiber uebersetzt die Shader fuer die
// Grafikkarte; Messwerte in docs/intern/NOTIZEN_dx12c_1211.md). ID3D12PipelineLibrary haelt die
// uebersetzten Zustaende; die Datei <Einstellungen>\shader\pso12.bin traegt davor die Namen, die sie enthaelt (das Lager selbst
// laesst sich nicht fragen, und ein vergeblicher Ladeversuch ist fuer die Debugschicht ein Fehler). Der Name ist eine Pruefsumme
// ueber alles, was den Zustand bestimmt (Shader-Code, Eingabe, Misch-, Tiefen-, Rasterzustand, Formate, Proben, Topologie) -
// unabhaengig vom Lauf. Anderer Treiber oder andere Karte: das Lager lehnt die Datei ab und beginnt leer.
// BHED_D3D12_PSOLAGER=0: ohne (zum Vergleichsmessen).
struct PsoName {
    std::uint64_t h[2];
    bool operator==(const PsoName& o) const noexcept { return h[0] == o.h[0] && h[1] == o.h[1]; }
};
struct PsoNameHash {
    std::size_t operator()(const PsoName& n) const noexcept { return static_cast<std::size_t>(n.h[0] ^ (n.h[1] * 0x9E3779B97F4A7C15ULL)); }
};
constexpr std::uint32_t kLagerMarke = 0x31504842U;   // "BHP1"
ID3D12PipelineLibrary* g_psoLager = nullptr;
std::vector<unsigned char> g_psoLagerDatei;   // die geladene Datei: das Lager liest daraus, solange es lebt
std::unordered_set<PsoName, PsoNameHash> g_psoLagerNamen;
std::wstring g_psoLagerPfad;
bool g_psoLagerVersucht = false;
unsigned g_psoLagerNeu = 0;   // seit dem Laden dazugekommen

void psoLagerOeffnen() {
    g_psoLagerVersucht = true;
    if (const char* v = std::getenv("BHED_D3D12_PSOLAGER"); v != nullptr && v[0] == '0') { return; }
    ID3D12Device1* d1 = nullptr;
    if (FAILED(rahmen::geraet()->QueryInterface(IID_PPV_ARGS(&d1))) || d1 == nullptr) { return; }
    std::string ordner = platform::settingsDirectory();
    if (!ordner.empty()) {
        ordner += "\\shader";
        CreateDirectoryW(::bhed::utf8Pfad(ordner).wstring().c_str(), nullptr);
        g_psoLagerPfad = ::bhed::utf8Pfad(ordner + "\\pso12.bin").wstring();
        FILE* f = nullptr;
        if (_wfopen_s(&f, g_psoLagerPfad.c_str(), L"rb") == 0 && f != nullptr) {
            std::fseek(f, 0, SEEK_END);
            const long n = std::ftell(f);
            std::fseek(f, 0, SEEK_SET);
            if (n > 8 && n < (512L << 20)) {
                g_psoLagerDatei.resize(static_cast<std::size_t>(n));
                if (std::fread(g_psoLagerDatei.data(), 1, g_psoLagerDatei.size(), f) != g_psoLagerDatei.size()) { g_psoLagerDatei.clear(); }
            }
            std::fclose(f);
        }
    }
    // Kopf: Marke, Zahl der Namen, die Namen; danach das Lager
    std::size_t kopf = 0;
    if (g_psoLagerDatei.size() >= 8U) {
        std::uint32_t marke = 0;
        std::uint32_t zahl = 0;
        std::memcpy(&marke, g_psoLagerDatei.data(), 4);
        std::memcpy(&zahl, g_psoLagerDatei.data() + 4, 4);
        kopf = 8U + static_cast<std::size_t>(zahl) * sizeof(PsoName);
        if (marke == kLagerMarke && kopf < g_psoLagerDatei.size()) {
            for (std::uint32_t i = 0; i < zahl; ++i) {
                PsoName n{};
                std::memcpy(&n, g_psoLagerDatei.data() + 8U + static_cast<std::size_t>(i) * sizeof(PsoName), sizeof(PsoName));
                g_psoLagerNamen.insert(n);
            }
        } else {
            kopf = 0;
        }
    }
    HRESULT hr = E_FAIL;
    if (kopf != 0U) { hr = d1->CreatePipelineLibrary(g_psoLagerDatei.data() + kopf, g_psoLagerDatei.size() - kopf, IID_PPV_ARGS(&g_psoLager)); }
    if (FAILED(hr) || g_psoLager == nullptr) {   // keine Datei, anderer Treiber, andere Karte, beschaedigt: leer beginnen
        g_psoLager = nullptr;
        const bool gab = !g_psoLagerNamen.empty();
        g_psoLagerNamen.clear();
        g_psoLagerDatei.clear();
        g_psoLagerDatei.shrink_to_fit();
        if (FAILED(d1->CreatePipelineLibrary(nullptr, 0, IID_PPV_ARGS(&g_psoLager)))) { g_psoLager = nullptr; }
        if (gab) { diag::detail("D3D12-Abgeber: der Zwischenspeicher der Pipeline-Zustaende passt nicht zu Treiber oder Karte - neu begonnen"); }
    }
    d1->Release();
    if (g_psoLager != nullptr) {
        diag::detail("D3D12-Abgeber: Zwischenspeicher der Pipeline-Zustaende: " + std::to_string(g_psoLagerNamen.size()) + " Zustaende von der Platte");
    }
}

// Beim Abbau: kam etwas dazu, die Datei neu schreiben (erst daneben, dann umbenennen).
void psoLagerSchliessen() {
    if (g_psoLager != nullptr && g_psoLagerNeu != 0U && !g_psoLagerPfad.empty()) {
        const SIZE_T n = g_psoLager->GetSerializedSize();
        std::vector<unsigned char> aus(8U + g_psoLagerNamen.size() * sizeof(PsoName) + n);
        const std::uint32_t zahl = static_cast<std::uint32_t>(g_psoLagerNamen.size());
        std::memcpy(aus.data(), &kLagerMarke, 4);
        std::memcpy(aus.data() + 4, &zahl, 4);
        std::size_t o = 8;
        for (const PsoName& name : g_psoLagerNamen) {
            std::memcpy(aus.data() + o, &name, sizeof(PsoName));
            o += sizeof(PsoName);
        }
        if (n != 0U && SUCCEEDED(g_psoLager->Serialize(aus.data() + o, n))) {
            const std::wstring neben = g_psoLagerPfad + L".neu";
            FILE* f = nullptr;
            if (_wfopen_s(&f, neben.c_str(), L"wb") == 0 && f != nullptr) {
                const bool ganz = std::fwrite(aus.data(), 1, aus.size(), f) == aus.size();
                std::fclose(f);
                if (ganz) {
                    MoveFileExW(neben.c_str(), g_psoLagerPfad.c_str(), MOVEFILE_REPLACE_EXISTING);
                } else {
                    DeleteFileW(neben.c_str());
                }
            }
        }
    }
    if (g_psoLager != nullptr) {
        g_psoLager->Release();
        g_psoLager = nullptr;
    }
    g_psoLagerDatei.clear();
    g_psoLagerDatei.shrink_to_fit();
    g_psoLagerNamen.clear();
    g_psoLagerPfad.clear();
    g_psoLagerVersucht = false;
    g_psoLagerNeu = 0;
}

PsoName psoNameVon(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& d, const Kontext& c) {
    PsoName n{{0x452821E638D01377ULL, 0xBE5466CF34E90C6CULL}};
    const std::uint64_t leer[2] = {0, 0};
    mische(n.h, c.vs->pruef, sizeof(leer));
    mische(n.h, c.ps != nullptr ? c.ps->pruef : leer, sizeof(leer));
    mische(n.h, c.gs != nullptr ? c.gs->pruef : leer, sizeof(leer));
    mische(n.h, &d.BlendState, sizeof(d.BlendState));
    mische(n.h, &d.SampleMask, sizeof(d.SampleMask));
    mische(n.h, &d.RasterizerState, sizeof(d.RasterizerState));
    mische(n.h, &d.DepthStencilState, sizeof(d.DepthStencilState));
    for (UINT i = 0; i < d.InputLayout.NumElements; ++i) {
        const D3D12_INPUT_ELEMENT_DESC& e = d.InputLayout.pInputElementDescs[i];
        const std::uint32_t felder[6] = {e.SemanticIndex, static_cast<std::uint32_t>(e.Format), e.InputSlot, e.AlignedByteOffset,
                                         static_cast<std::uint32_t>(e.InputSlotClass), e.InstanceDataStepRate};
        mische(n.h, e.SemanticName, std::strlen(e.SemanticName));
        mische(n.h, felder, sizeof(felder));
    }
    const std::uint32_t rest[4] = {static_cast<std::uint32_t>(d.PrimitiveTopologyType), d.NumRenderTargets, static_cast<std::uint32_t>(d.DSVFormat), d.SampleDesc.Count};
    mische(n.h, rest, sizeof(rest));
    mische(n.h, d.RTVFormats, sizeof(d.RTVFormats));
    return n;
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE topologieTyp(D3D11_PRIMITIVE_TOPOLOGY t) {
    switch (t) {
    case D3D11_PRIMITIVE_TOPOLOGY_POINTLIST: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case D3D11_PRIMITIVE_TOPOLOGY_LINELIST:
    case D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP:
    case D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ:
    case D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
    case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
    case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ:
    case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    default: return t >= D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH : D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    }
}

D3D12_DEPTH_STENCILOP_DESC stencilOp12(const D3D11_DEPTH_STENCILOP_DESC& e) {
    D3D12_DEPTH_STENCILOP_DESC s{};
    s.StencilFailOp = static_cast<D3D12_STENCIL_OP>(e.StencilFailOp);   // dieselben Werte
    s.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(e.StencilDepthFailOp);
    s.StencilPassOp = static_cast<D3D12_STENCIL_OP>(e.StencilPassOp);
    s.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(e.StencilFunc);
    return s;
}

// Der Pipeline-Zustand zum Schluessel: die Beschreibungen von Direct3D 11 Feld fuer Feld (die Aufzaehlungen haben dieselben
// Werte), ohne gebundenes Objekt die Vorgabe von Direct3D 11.
ID3D12PipelineState* psoAnlegen(const PsoSchluessel& k, const Kontext& c) {
    if (c.vs == nullptr) { return nullptr; }
    D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};
    d.pRootSignature = wurzelSignatur();
    d.VS = {c.vs->code.data(), c.vs->code.size()};
    if (c.ps != nullptr) { d.PS = {c.ps->code.data(), c.ps->code.size()}; }
    if (c.gs != nullptr) { d.GS = {c.gs->code.data(), c.gs->code.size()}; }
    // Mischung
    D3D11_BLEND_DESC m{};
    if (c.mischung != nullptr) {
        m = c.mischung->art;
    } else {
        for (auto& rt : m.RenderTarget) {
            rt.SrcBlend = rt.SrcBlendAlpha = D3D11_BLEND_ONE;
            rt.DestBlend = rt.DestBlendAlpha = D3D11_BLEND_ZERO;
            rt.BlendOp = rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
            rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        }
    }
    d.BlendState.AlphaToCoverageEnable = m.AlphaToCoverageEnable;
    d.BlendState.IndependentBlendEnable = m.IndependentBlendEnable;
    for (UINT i = 0; i < 8U; ++i) {
        const D3D11_RENDER_TARGET_BLEND_DESC& q = m.RenderTarget[i];
        D3D12_RENDER_TARGET_BLEND_DESC& z = d.BlendState.RenderTarget[i];
        z.BlendEnable = q.BlendEnable;
        z.LogicOpEnable = FALSE;
        z.SrcBlend = static_cast<D3D12_BLEND>(q.SrcBlend);
        z.DestBlend = static_cast<D3D12_BLEND>(q.DestBlend);
        z.BlendOp = static_cast<D3D12_BLEND_OP>(q.BlendOp);
        z.SrcBlendAlpha = static_cast<D3D12_BLEND>(q.SrcBlendAlpha);
        z.DestBlendAlpha = static_cast<D3D12_BLEND>(q.DestBlendAlpha);
        z.BlendOpAlpha = static_cast<D3D12_BLEND_OP>(q.BlendOpAlpha);
        z.LogicOp = D3D12_LOGIC_OP_NOOP;
        z.RenderTargetWriteMask = q.RenderTargetWriteMask;
    }
    d.SampleMask = k.maske;
    // Raster
    D3D12_RASTERIZER_DESC& r = d.RasterizerState;
    if (c.raster != nullptr) {
        const D3D11_RASTERIZER_DESC& q = c.raster->art;
        r.FillMode = static_cast<D3D12_FILL_MODE>(q.FillMode);
        r.CullMode = static_cast<D3D12_CULL_MODE>(q.CullMode);
        r.FrontCounterClockwise = q.FrontCounterClockwise;
        r.DepthBias = q.DepthBias;
        r.DepthBiasClamp = q.DepthBiasClamp;
        r.SlopeScaledDepthBias = q.SlopeScaledDepthBias;
        r.DepthClipEnable = q.DepthClipEnable;
        r.MultisampleEnable = q.MultisampleEnable;
        r.AntialiasedLineEnable = q.AntialiasedLineEnable;
    } else {
        r.FillMode = D3D12_FILL_MODE_SOLID;
        r.CullMode = D3D12_CULL_MODE_BACK;
        r.DepthClipEnable = TRUE;
    }
    r.ForcedSampleCount = 0;
    r.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    // Tiefe
    D3D12_DEPTH_STENCIL_DESC& t = d.DepthStencilState;
    if (c.tiefe != nullptr) {
        const D3D11_DEPTH_STENCIL_DESC& q = c.tiefe->art;
        t.DepthEnable = q.DepthEnable;
        t.DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(q.DepthWriteMask);
        t.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(q.DepthFunc);
        t.StencilEnable = q.StencilEnable;
        t.StencilReadMask = q.StencilReadMask;
        t.StencilWriteMask = q.StencilWriteMask;
        t.FrontFace = stencilOp12(q.FrontFace);
        t.BackFace = stencilOp12(q.BackFace);
    } else {
        t.DepthEnable = TRUE;
        t.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        t.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        t.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        t.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        const D3D12_DEPTH_STENCILOP_DESC vorgabe{D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};
        t.FrontFace = vorgabe;
        t.BackFace = vorgabe;
    }
    if (k.tiefeFormat == 0U) {   // ohne Tiefenziel prueft und schreibt Direct3D 11 keine Tiefe
        t.DepthEnable = FALSE;
        t.StencilEnable = FALSE;
    }
    if (c.eingabe != nullptr) { d.InputLayout = {c.eingabe->elemente.data(), static_cast<UINT>(c.eingabe->elemente.size())}; }
    d.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(k.topologie);
    d.NumRenderTargets = k.ziele;
    for (UINT i = 0; i < k.ziele; ++i) { d.RTVFormats[i] = static_cast<DXGI_FORMAT>(k.zielFormat[i]); }
    d.DSVFormat = static_cast<DXGI_FORMAT>(k.tiefeFormat);
    d.SampleDesc.Count = k.proben;
    const Stoppuhr uhr(zeitenBild().psoMs);
    ++zeitenBild().pso;
    if (!g_psoLagerVersucht) { psoLagerOeffnen(); }
    ID3D12PipelineState* p = nullptr;
    PsoName name{};
    wchar_t nameText[36] = L"";
    bool imLager = false;
    if (g_psoLager != nullptr) {
        name = psoNameVon(d, c);
        swprintf_s(nameText, L"%016llx%016llx", static_cast<unsigned long long>(name.h[0]), static_cast<unsigned long long>(name.h[1]));
        imLager = g_psoLagerNamen.count(name) != 0U;
        if (imLager && SUCCEEDED(g_psoLager->LoadGraphicsPipeline(nameText, &d, IID_PPV_ARGS(&p))) && p != nullptr) {
            ++zeitenBild().psoAusSpeicher;
            ++zaehlerIntern().pso;
            return p;
        }
        p = nullptr;
    }
    const HRESULT hr = rahmen::geraet()->CreateGraphicsPipelineState(&d, IID_PPV_ARGS(&p));
    if (SUCCEEDED(hr) && p != nullptr && g_psoLager != nullptr && !imLager && SUCCEEDED(g_psoLager->StorePipeline(nameText, p))) {
        g_psoLagerNamen.insert(name);
        ++g_psoLagerNeu;
    }
    if (FAILED(hr) || p == nullptr) {
        char z[200];
        std::snprintf(z, sizeof(z), "D3D12-Abgeber: Pipeline-Zustand abgelehnt (0x%08lX): vs %u ps %u gs %u Eingabe %u Mischung %u Tiefe %u Raster %u Ziele %u Proben %u",
                      static_cast<unsigned long>(hr), k.vs, k.ps, k.gs, k.eingabe, k.mischung, k.tiefe, k.raster, static_cast<unsigned>(k.ziele),
                      static_cast<unsigned>(k.proben));
        diag::error(z);
        return nullptr;
    }
    ++zaehlerIntern().pso;
    return p;
}

void barriereTeil(ID3D12GraphicsCommandList* cl, ID3D12Resource* r, UINT teil, D3D12_RESOURCE_STATES vor, D3D12_RESOURCE_STATES nach) {
    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = r;
    b.Transition.Subresource = teil;
    b.Transition.StateBefore = vor;
    b.Transition.StateAfter = nach;
    cl->ResourceBarrier(1, &b);
    zaehleBarriere();
}

Textur* alsTextur(ID3D11Resource* r) {
    if (r == nullptr) { return nullptr; }
    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    r->GetType(&dim);
    return dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D ? jetzig(static_cast<Textur*>(static_cast<ID3D11Texture2D*>(r)), "Textur") : nullptr;
}

Puffer* alsPuffer(ID3D11Resource* r) {
    if (r == nullptr) { return nullptr; }
    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    r->GetType(&dim);
    return dim == D3D11_RESOURCE_DIMENSION_BUFFER ? jetzig(static_cast<Puffer*>(static_cast<ID3D11Buffer*>(r)), "Puffer") : nullptr;
}

// Der Zaunwert der Liste, in der die Ausfuehrung `serie` lief; false: sie ist noch nicht abgeschickt.
bool zaunDer(std::uint64_t serie, std::uint64_t& zaun) {
    if (serie == rahmen::ausfuehrung()) { return false; }
    const std::size_t i = static_cast<std::size_t>(serie % kZaunRing);
    zaun = g_zaunSerie[i] == serie + 1U ? g_zaunVon[i] : rahmen::letzterZaun();
    return true;
}

// Zustandswechsel einer Textur durch den Kontext: wer eine gebundene Textur aus dem Lesezustand oder ein gebundenes Ziel aus dem
// Zielzustand nimmt, laesst vor dem naechsten Zeichenbefehl nachsehen.
// Die Schichten, in die eine Zielsicht schreibt, brauchen Speicher (duenne Texturfelder).
void zielSchichtenBelegen(const ZielSicht* z) {
    Textur* t = z->textur;
    if (t->schichtHeap.empty()) { return; }
    if (z->art.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY) {
        schichtBelegen(t, z->art.Texture2DArray.FirstArraySlice, z->art.Texture2DArray.ArraySize);
    } else {   // eine Sicht ohne Angabe der Schichten: alle
        schichtBelegen(t, 0, ~0U);
    }
}

void nachWechsel(Kontext& c, const Textur* t, D3D12_RESOURCE_STATES nach) {
    if (nach != kLesbar) { c.schmutz |= kSSrvZustand; }
    if (nach != D3D12_RESOURCE_STATE_RENDER_TARGET && nach != D3D12_RESOURCE_STATE_DEPTH_WRITE && c.istZiel(t)) { c.schmutz |= kSZiele; }
}

void stelle(Kontext& c, ID3D12GraphicsCommandList* cl, Textur* t, D3D12_RESOURCE_STATES nach) {
    gebraucht(t);
    if (t->zustand == nach) { return; }
    wechsle(cl, t, nach);
    nachWechsel(c, t, nach);
}

// Zwei Texturen (Quelle und Ziel eines Kopierens oder Aufloesens) in einem Aufruf.
void stelleZwei(Kontext& c, ID3D12GraphicsCommandList* cl, Textur* a, D3D12_RESOURCE_STATES nachA, Textur* b, D3D12_RESOURCE_STATES nachB) {
    gebraucht(a);
    gebraucht(b);
    const bool wa = a->zustand != nachA;
    const bool wb = b->zustand != nachB;
    if (!wa && !wb) { return; }
    const Wechsel w[2] = {{a, nachA}, {b, nachB}};
    wechsle(cl, w, 2);
    if (wa) { nachWechsel(c, a, nachA); }
    if (wb) { nachWechsel(c, b, nachB); }
}

}  // namespace

bool PsoSchluessel::operator==(const PsoSchluessel& o) const noexcept { return std::memcmp(this, &o, sizeof(o)) == 0; }

void kontextAllesNeu() {
    Kontext* c = kontextZeiger();
    if (c == nullptr) { return; }
    c->schmutz = ~0U;
    c->cbSchmutz = kCbMaske;
    c->srvSchmutz = kSrvMaske;
    c->abtasterSchmutz = kAbtasterMaske;
    c->psoJetzt = nullptr;
}

ID3D11DeviceContext* kontext() { return aktiv() ? kontextZeiger() : nullptr; }

void listeNeu(ID3D12GraphicsCommandList* cl) {
    Kontext* c = kontextZeiger();
    if (c == nullptr || !aktiv()) { return; }
    c->offeneListe = cl;
    c->kopierZiele.clear();   // eine neue Ausfuehrung: Puffer stehen wieder in COMMON
    LARGE_INTEGER t{};
    QueryPerformanceCounter(&t);
    c->listeSeit = t.QuadPart;
    ID3D12DescriptorHeap* beide[2] = {rahmen::srvStapel(), abtasterStapel()};
    cl->SetDescriptorHeaps(2, beide);
    kontextAllesNeu();
}

void fremdGezeichnet() { kontextAllesNeu(); }

void nachAbschicken(std::uint64_t zaun) {
    if (Kontext* c = kontextZeiger(); c != nullptr) {
        c->offeneListe = nullptr;
        c->kopierZiele.clear();   // Puffer fallen nach der Ausfuehrung auf COMMON zurueck
    }
    // rahmen::ausfuehrung() zaehlt schon die naechste; die eben abgeschickte ist die davor
    const std::uint64_t serie = rahmen::ausfuehrung() - 1U;
    g_zaunVon[serie % kZaunRing] = zaun;
    g_zaunSerie[serie % kZaunRing] = serie + 1U;
    bloeckeStempeln(zaun);
}

void psoAbbauen() {
    psoLagerSchliessen();
    for (auto& [k, p] : g_pso) {
        (void)k;
        if (p != nullptr) { p->Release(); }
    }
    g_pso.clear();
    for (auto& [k, p] : g_mipPso) {
        (void)k;
        if (p != nullptr) { p->Release(); }
    }
    g_mipPso.clear();
    if (g_mipVs != nullptr) {
        g_mipVs->Release();
        g_mipVs = nullptr;
    }
    if (g_mipPs != nullptr) {
        g_mipPs->Release();
        g_mipPs = nullptr;
    }
    for (auto& z : g_zaunSerie) { z = 0; }
    g_markenFrequenz = 0;
}

// ID3D11DeviceContext::GenerateMips: jede Stufe aus der davor, vier Bildpunkte gemittelt (bilinear in der Mitte zwischen ihnen).
void mipsErzeugen(ID3D12GraphicsCommandList* cl, Sicht* s) {
    Textur* t = s != nullptr ? s->textur : nullptr;
    if (t == nullptr || t->art.MipLevels <= 1U || (t->art.BindFlags & D3D11_BIND_RENDER_TARGET) == 0U || t->art.ArraySize != 1U) { return; }
    static const char* kVs = "void main(uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD0) {"
                             " uv = float2((id << 1) & 2, id & 2); pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1); }";
    static const char* kPs = "Texture2D t : register(t0); SamplerState s : register(s0);"
                             "float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target { return t.SampleLevel(s, uv, 0); }";
    if (g_mipVs == nullptr && FAILED(D3DCompile(kVs, std::strlen(kVs), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &g_mipVs, nullptr))) { return; }
    if (g_mipPs == nullptr && FAILED(D3DCompile(kPs, std::strlen(kPs), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &g_mipPs, nullptr))) { return; }
    ID3D12PipelineState*& pso = g_mipPso[static_cast<int>(t->art.Format)];
    if (pso == nullptr) {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};
        d.pRootSignature = wurzelSignatur();
        d.VS = {g_mipVs->GetBufferPointer(), g_mipVs->GetBufferSize()};
        d.PS = {g_mipPs->GetBufferPointer(), g_mipPs->GetBufferSize()};
        d.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        d.SampleMask = 0xFFFFFFFFU;
        d.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        d.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        d.RasterizerState.DepthClipEnable = TRUE;
        d.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        d.NumRenderTargets = 1;
        d.RTVFormats[0] = t->art.Format;
        d.SampleDesc.Count = 1;
        if (FAILED(rahmen::geraet()->CreateGraphicsPipelineState(&d, IID_PPV_ARGS(&pso)))) {
            pso = nullptr;
            diag::error("D3D12-Abgeber: GenerateMips: Pipeline-Zustand abgelehnt");
            return;
        }
    }
    D3D11_SAMPLER_DESC ab{};
    ab.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    ab.AddressU = ab.AddressV = ab.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    ab.MaxAnisotropy = 1;
    ab.ComparisonFunc = D3D11_COMPARISON_NEVER;
    ab.MaxLOD = D3D11_FLOAT32_MAX;
    ID3D11SamplerState* abtasterObjekt = nullptr;
    if (FAILED(geraetZeiger()->CreateSamplerState(&ab, &abtasterObjekt)) || abtasterObjekt == nullptr) { return; }
    wechsle(cl, t, kLesbar);
    cl->SetGraphicsRootSignature(wurzelSignatur());
    cl->SetPipelineState(pso);
    cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cl->SetGraphicsRootDescriptorTable(kWurzelAbtaster, static_cast<Abtaster*>(abtasterObjekt)->gpu);
    const D3D12_CPU_DESCRIPTOR_HANDLE ziel = hilfsZiel();
    for (UINT stufe = 1; stufe < t->art.MipLevels; ++stufe) {
        const UINT quelle = rahmen::srvNehmen();
        if (quelle == rahmen::srvKeiner()) { break; }
        D3D12_SHADER_RESOURCE_VIEW_DESC sd{};
        sd.Format = t->art.Format;
        sd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        sd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        sd.Texture2D.MostDetailedMip = stufe - 1U;
        sd.Texture2D.MipLevels = 1;
        rahmen::geraet()->CreateShaderResourceView(t->res, &sd, rahmen::srvCpu(quelle));
        D3D12_RENDER_TARGET_VIEW_DESC rd{};
        rd.Format = t->art.Format;
        rd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rd.Texture2D.MipSlice = stufe;
        rahmen::geraet()->CreateRenderTargetView(t->res, &rd, ziel);
        barriereTeil(cl, t->res, stufe, kLesbar, D3D12_RESOURCE_STATE_RENDER_TARGET);
        const UINT w = std::max(1U, t->art.Width >> stufe);
        const UINT h = std::max(1U, t->art.Height >> stufe);
        const D3D12_VIEWPORT vp{0.0F, 0.0F, static_cast<float>(w), static_cast<float>(h), 0.0F, 1.0F};
        const D3D12_RECT sr{0, 0, static_cast<LONG>(w), static_cast<LONG>(h)};
        cl->OMSetRenderTargets(1, &ziel, FALSE, nullptr);
        cl->RSSetViewports(1, &vp);
        cl->RSSetScissorRects(1, &sr);
        cl->SetGraphicsRootDescriptorTable(kWurzelSrv, rahmen::srvGpu(quelle));
        cl->DrawInstanced(3, 1, 0, 0);
        barriereTeil(cl, t->res, stufe, D3D12_RESOURCE_STATE_RENDER_TARGET, kLesbar);
        rahmen::srvSpaeterFrei(quelle);
    }
    abtasterObjekt->Release();
    kontextAllesNeu();
}

// ===================================================================================
// Kontext

HRESULT STDMETHODCALLTYPE Kontext::QueryInterface(REFIID iid, void** aus) {
    if (aus == nullptr) { return E_POINTER; }
    if (iid == __uuidof(IUnknown) || iid == __uuidof(ID3D11DeviceChild) || iid == __uuidof(ID3D11DeviceContext) || iid == __uuidof(ID3D11DeviceContext1)) {
        *aus = static_cast<ID3D11DeviceContext1*>(this);
        return S_OK;
    }
    *aus = nullptr;
    return E_NOINTERFACE;
}

void STDMETHODCALLTYPE Kontext::GetDevice(ID3D11Device** d) {
    if (d != nullptr) { *d = geraetZeiger(); }
}

bool Kontext::istZiel(const Textur* t) const {
    for (UINT i = 0; i < ziele; ++i) {
        if (ziel[i] != nullptr && ziel[i]->textur == t) { return true; }
    }
    return tiefeZiel != nullptr && tiefeZiel->textur == t;
}

void Kontext::pufferUmbenannt(const Puffer* p) {
    cbSchmutz |= p->cbGebunden;
    srvSchmutz |= p->srvGebunden;
    if (p->vbGebunden != 0U) { schmutz |= kSVb; }
    if (p->ibGebunden) { schmutz |= kSIb; }
}

// Einen Platz neu belegen und im Puffer merken, wo er gebunden ist (vor binde: das kann den alten Puffer freigeben).
void Kontext::bindeCb(UINT stufe, UINT platz, Puffer* neu) {
    const std::uint32_t bit = 1U << (stufe * kCbSchritt + platz);
    if (cb[stufe][platz] != nullptr) { cb[stufe][platz]->cbGebunden &= ~bit; }
    if (neu != nullptr) { neu->cbGebunden |= bit; }
    binde(cb[stufe][platz], neu);
}

void Kontext::bindeVb(UINT platz, Puffer* neu) {
    const auto bit = static_cast<std::uint8_t>(1U << platz);
    if (vb[platz] != nullptr) { vb[platz]->vbGebunden = static_cast<std::uint8_t>(vb[platz]->vbGebunden & ~bit); }
    if (neu != nullptr) { neu->vbGebunden = static_cast<std::uint8_t>(neu->vbGebunden | bit); }
    binde(vb[platz], neu);
}

void Kontext::bindeIb(Puffer* neu) {
    if (ib != nullptr) { ib->ibGebunden = false; }
    if (neu != nullptr) { neu->ibGebunden = true; }
    binde(ib, neu);
}

void Kontext::bindeSrv(UINT platz, Sicht* neu) {
    const auto bit = static_cast<std::uint16_t>(1U << platz);
    if (srv[platz] != nullptr && srv[platz]->puffer != nullptr) {
        srv[platz]->puffer->srvGebunden = static_cast<std::uint16_t>(srv[platz]->puffer->srvGebunden & ~bit);
    }
    if (neu != nullptr && neu->puffer != nullptr) { neu->puffer->srvGebunden = static_cast<std::uint16_t>(neu->puffer->srvGebunden | bit); }
    binde(srv[platz], neu);
}

void Kontext::allesLoesen() {
    for (UINT st = 0; st < 3U; ++st) {
        for (UINT i = 0; i < kCbPlaetze; ++i) { bindeCb(st, i, nullptr); }
    }
    for (auto& st : cbErste) {
        for (UINT& e : st) { e = 0; }
    }
    for (UINT i = 0; i < kSrvPlaetze; ++i) { bindeSrv(i, nullptr); }
    for (Abtaster*& a : abtaster) { binde(a, static_cast<Abtaster*>(nullptr)); }
    binde(vs, static_cast<VsObjekt*>(nullptr));
    binde(ps, static_cast<PsObjekt*>(nullptr));
    binde(gs, static_cast<GsObjekt*>(nullptr));
    binde(eingabe, static_cast<Eingabe*>(nullptr));
    binde(mischung, static_cast<Mischung*>(nullptr));
    binde(tiefe, static_cast<TiefeZustand*>(nullptr));
    binde(raster, static_cast<Raster*>(nullptr));
    for (ZielSicht*& z : ziel) { binde(z, static_cast<ZielSicht*>(nullptr)); }
    ziele = 0;
    binde(tiefeZiel, static_cast<TiefeSicht*>(nullptr));
    for (UINT i = 0; i < kVbPlaetze; ++i) { bindeVb(i, nullptr); }
    bindeIb(nullptr);
    mischFaktor[0] = mischFaktor[1] = mischFaktor[2] = mischFaktor[3] = 1.0F;
    probenMaske = 0xFFFFFFFFU;
    stencilWert = 0;
    topologie = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    sichtfelder = 0;
    scheren = 0;
    kontextAllesNeu();
}

void Kontext::zieleAnwenden() {
    ID3D12GraphicsCommandList* cl = offeneListe;
    D3D12_CPU_DESCRIPTOR_HANDLE griffe[kZiele]{};
    Wechsel w[kZiele + 1U];
    UINT n = 0;
    for (UINT i = 0; i < ziele; ++i) {
        if (ziel[i] == nullptr) {
            (void)fehlt("OMSetRenderTargets(Luecke in den Zielen)");
            continue;
        }
        zielSchichtenBelegen(ziel[i]);
        w[n++] = Wechsel{ziel[i]->textur, D3D12_RESOURCE_STATE_RENDER_TARGET};
        griffe[i] = ziel[i]->griff;
    }
    if (tiefeZiel != nullptr) { w[n++] = Wechsel{tiefeZiel->textur, D3D12_RESOURCE_STATE_DEPTH_WRITE}; }
    wechsle(cl, w, n);
    if (protokoll::g_an) {
        std::string t = "Ziele";
        for (UINT i = 0; i < ziele; ++i) { t += " [" + protokoll::nenne(ziel[i] != nullptr ? ziel[i]->textur : nullptr) + "]"; }
        if (tiefeZiel != nullptr) { t += " Tiefe [" + protokoll::nenne(tiefeZiel->textur) + "]"; }
        protokoll::eintrag(cl, std::move(t));
    }
    cl->OMSetRenderTargets(ziele, ziele != 0U ? griffe : nullptr, FALSE, tiefeZiel != nullptr ? &tiefeZiel->griff : nullptr);
}

// D3D12-4: alle Puffer, die seit dem letzten Zeichenbefehl kopiert wurden, in einem Aufruf zurueck in ihren Lesezustand.
void Kontext::kopierZieleLesbar() {
    if (offeneListe == nullptr || kopierZiele.empty()) {
        kopierZiele.clear();
        return;
    }
    const std::uint64_t jetzt = rahmen::ausfuehrung();
    std::vector<D3D12_RESOURCE_BARRIER> b;
    b.reserve(kopierZiele.size());
    for (Puffer* p : kopierZiele) {
        if (p->kopierSerie != jetzt) { continue; }
        D3D12_RESOURCE_BARRIER e{};
        e.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        e.Transition.pResource = p->res;
        e.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        e.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        e.Transition.StateAfter = p->lesen;
        b.push_back(e);
        p->kopierSerie = ~0ULL;
        p->serie = jetzt;   // im Lesezustand
        zaehleBarriere();
    }
    kopierZiele.clear();
    if (!b.empty()) { offeneListe->ResourceBarrier(static_cast<UINT>(b.size()), b.data()); }
}

void kopierZieleLesbar() {
    if (Kontext* c = kontextZeiger(); c != nullptr && aktiv()) { c->kopierZieleLesbar(); }
}

// Alles Geaenderte in die Liste. false: so kann nicht gezeichnet werden (kein Shader, Pipeline-Zustand abgelehnt).
bool Kontext::vorZeichnen() {
    if (offeneListe == nullptr) { (void)rahmen::liste(); }   // ruft listeNeu: offeneListe gesetzt, alles neu
    ID3D12GraphicsCommandList* cl = offeneListe;
    if (cl == nullptr) { return false; }
    if (listeSeit == 0) {   // die Liste trug bisher nur Zeitmarken (D3D12-4)
        LARGE_INTEGER t{};
        QueryPerformanceCounter(&t);
        listeSeit = t.QuadPart;
    }
    if (!kopierZiele.empty()) { kopierZieleLesbar(); }
    if (schmutz == 0U && cbSchmutz == 0U && srvSchmutz == 0U && abtasterSchmutz == 0U) { return true; }
    if ((schmutz & kSWurzel) != 0U) { cl->SetGraphicsRootSignature(wurzelSignatur()); }
    if ((schmutz & kSZiele) != 0U) { zieleAnwenden(); }
    if ((schmutz & kSSrvZustand) != 0U) {
        Wechsel w[kSrvPlaetze];
        UINT n = 0;
        // Am Anfang jeder Liste ist alles geaendert: hier kommt jede gebundene Textur einmal je Bild vorbei (Auslagern).
        for (Sicht* s : srv) {
            if (s == nullptr || s->textur == nullptr) { continue; }
            gebraucht(s->textur);
            if (s->textur->zustand != kLesbar) { w[n++] = Wechsel{s->textur, kLesbar}; }
        }
        wechsle(cl, w, n);
    }
    if ((schmutz & kSPso) != 0U) {
        PsoSchluessel k;
        k.vs = vs != nullptr ? vs->kennung : 0U;
        k.ps = ps != nullptr ? ps->kennung : 0U;
        k.gs = gs != nullptr ? gs->kennung : 0U;
        k.eingabe = eingabe != nullptr ? eingabe->kennung : 0U;
        k.mischung = mischung != nullptr ? mischung->kennung : 0U;
        k.tiefe = tiefe != nullptr ? tiefe->kennung : 0U;
        k.raster = raster != nullptr ? raster->kennung : 0U;
        k.maske = probenMaske;
        k.ziele = static_cast<std::uint8_t>(ziele);
        for (UINT i = 0; i < ziele; ++i) {
            if (ziel[i] != nullptr) {
                k.zielFormat[i] = static_cast<std::uint16_t>(ziel[i]->format);
                k.proben = static_cast<std::uint8_t>(ziel[i]->textur->art.SampleDesc.Count);
            }
        }
        if (tiefeZiel != nullptr) {
            k.tiefeFormat = static_cast<std::uint16_t>(tiefeZiel->format);
            k.proben = static_cast<std::uint8_t>(tiefeZiel->textur->art.SampleDesc.Count);
        }
        k.topologie = static_cast<std::uint8_t>(topologieTyp(topologie));
        if (psoJetzt == nullptr || !(k == schluesselJetzt)) {
            ID3D12PipelineState* p = nullptr;
            if (const auto it = g_pso.find(k); it != g_pso.end()) {
                p = it->second;
            } else {
                p = psoAnlegen(k, *this);
                g_pso[k] = p;   // auch nullptr: nicht bei jedem Aufruf neu versuchen
            }
            if (p == nullptr) { return false; }
            cl->SetPipelineState(p);
            ++zaehlerIntern().psoWechsel;
            psoJetzt = p;
            schluesselJetzt = k;
        }
    }
    if ((schmutz & kSTopologie) != 0U) { cl->IASetPrimitiveTopology(static_cast<D3D12_PRIMITIVE_TOPOLOGY>(topologie)); }
    if ((schmutz & kSVb) != 0U) {
        D3D12_VERTEX_BUFFER_VIEW sichten[kVbPlaetze]{};
        UINT n = 1;
        for (UINT i = 0; i < kVbPlaetze; ++i) {
            Puffer* p = vb[i];
            if (p == nullptr) { continue; }
            if (p->dynamisch) {
                pufferRuhe(cl, p);
            } else {
                pufferLesbar(cl, p);
            }
            sichten[i].BufferLocation = p->va + vbVersatz[i];
            sichten[i].SizeInBytes = p->art.ByteWidth > vbVersatz[i] ? p->art.ByteWidth - vbVersatz[i] : 0U;
            sichten[i].StrideInBytes = vbSchritt[i];
            n = i + 1U;
        }
        cl->IASetVertexBuffers(0, n, sichten);
    }
    if ((schmutz & kSIb) != 0U) {
        D3D12_INDEX_BUFFER_VIEW sicht{};
        if (ib != nullptr) {
            if (ib->dynamisch) {
                pufferRuhe(cl, ib);
            } else {
                pufferLesbar(cl, ib);
            }
            sicht.BufferLocation = ib->va + ibVersatz;
            sicht.SizeInBytes = ib->art.ByteWidth > ibVersatz ? ib->art.ByteWidth - ibVersatz : 0U;
            sicht.Format = ibFormat;
            cl->IASetIndexBuffer(&sicht);
        } else {
            cl->IASetIndexBuffer(nullptr);
        }
    }
    if ((schmutz & kSSichtfeld) != 0U && sichtfelder != 0U) { cl->RSSetViewports(sichtfelder, sichtfeld); }
    if ((schmutz & kSSchere) != 0U) {
        // Direct3D 12 schneidet immer; ohne ScissorEnable (Direct3D 11) das groesste Rechteck
        if (raster != nullptr && raster->art.ScissorEnable != FALSE && scheren != 0U) {
            cl->RSSetScissorRects(scheren, schere);
        } else {
            const D3D12_RECT alles{0, 0, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION};
            cl->RSSetScissorRects(1, &alles);
        }
    }
    if ((schmutz & kSFaktor) != 0U) { cl->OMSetBlendFactor(mischFaktor); }
    if ((schmutz & kSStencil) != 0U) { cl->OMSetStencilRef(stencilWert); }
    schmutz = 0;
    std::uint32_t bits = cbSchmutz & kCbMaske;
    cbSchmutz = 0;
    while (bits != 0U) {
        unsigned long b = 0;
        _BitScanForward(&b, bits);
        bits &= bits - 1U;
        const UINT st = b / kCbSchritt;
        const UINT platz = b % kCbSchritt;
        Puffer* p = cb[st][platz];
        std::uint64_t adresse = nullSeite();
        if (p != nullptr) {
            if (!p->dynamisch) { pufferLesbar(cl, p); }
            adresse = p->va + static_cast<std::uint64_t>(cbErste[st][platz]) * 16U;
        }
        cl->SetGraphicsRootConstantBufferView(wurzelCb(st, platz), adresse);
    }
    bits = srvSchmutz & kSrvMaske;
    srvSchmutz = 0;
    while (bits != 0U) {
        unsigned long b = 0;
        _BitScanForward(&b, bits);
        bits &= bits - 1U;
        Sicht* s = srv[b];
        D3D12_GPU_DESCRIPTOR_HANDLE h = leereSicht();
        if (s != nullptr) {
            if (s->puffer != nullptr) {
                if (s->puffer->dynamisch) {
                    // Der Pixel-Shader liest diesen Puffer je Bildpunkt (Lichtliste und Kacheln des Echtzeit-Lichts): aus einer
                    // Kopie im Grafikspeicher statt ueber den Bus aus dem Arbeitsspeicher (einmal je Map kopiert).
                    const bool kopie = pufferInGrafik(cl, s->puffer);
                    if (kopie != s->aufKopie || s->fassung != s->puffer->fassung) { sichtAufPufferErneuern(s); }
                } else {
                    pufferLesbar(cl, s->puffer);
                }
            }
            if (s->index != rahmen::srvKeiner()) { h = rahmen::srvGpu(s->index); }
        }
        cl->SetGraphicsRootDescriptorTable(kWurzelSrv + b, h);
    }
    bits = abtasterSchmutz & kAbtasterMaske;
    abtasterSchmutz = 0;
    while (bits != 0U) {
        unsigned long b = 0;
        _BitScanForward(&b, bits);
        bits &= bits - 1U;
        cl->SetGraphicsRootDescriptorTable(kWurzelAbtaster + b, abtaster[b] != nullptr ? abtaster[b]->gpu : vorgabeAbtaster());
    }
    return true;
}

void Kontext::setzeCb(UINT stufe, UINT anfang, UINT anzahl, ID3D11Buffer* const* puffer, const UINT* erste) {
    for (UINT i = 0; i < anzahl; ++i) {
        const UINT platz = anfang + i;
        if (wurzelCb(stufe, platz) == ~0U) {
            if (puffer != nullptr && puffer[i] != nullptr) { (void)fehlt("SetConstantBuffers(Platz ausserhalb der Wurzelsignatur)"); }
            continue;
        }
        Puffer* neu = puffer != nullptr ? jetzig(static_cast<Puffer*>(puffer[i]), "SetConstantBuffers") : nullptr;
        const UINT e = (erste != nullptr && neu != nullptr) ? erste[i] : 0U;
        if (cb[stufe][platz] == neu && cbErste[stufe][platz] == e) { continue; }
        if (cb[stufe][platz] != neu) { bindeCb(stufe, platz, neu); }
        cbErste[stufe][platz] = e;
        cbSchmutz |= 1U << (stufe * kCbSchritt + platz);
    }
}

void STDMETHODCALLTYPE Kontext::VSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers) {
    setzeCb(0, StartSlot, NumBuffers, ppConstantBuffers, nullptr);
}
void STDMETHODCALLTYPE Kontext::PSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers) {
    setzeCb(1, StartSlot, NumBuffers, ppConstantBuffers, nullptr);
}
void STDMETHODCALLTYPE Kontext::GSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers) {
    setzeCb(2, StartSlot, NumBuffers, ppConstantBuffers, nullptr);
}
void STDMETHODCALLTYPE Kontext::VSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT*) {
    setzeCb(0, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant);
}
void STDMETHODCALLTYPE Kontext::PSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT*) {
    setzeCb(1, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant);
}
void STDMETHODCALLTYPE Kontext::GSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, const UINT* pFirstConstant, const UINT*) {
    setzeCb(2, StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant);
}

void STDMETHODCALLTYPE Kontext::PSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews) {
    for (UINT i = 0; i < NumViews; ++i) {
        const UINT platz = StartSlot + i;
        if (platz >= kSrvPlaetze) {
            if (ppShaderResourceViews != nullptr && ppShaderResourceViews[i] != nullptr) { (void)fehlt("PSSetShaderResources(Platz ueber 15)"); }
            continue;
        }
        Sicht* neu = ppShaderResourceViews != nullptr ? jetzig(static_cast<Sicht*>(ppShaderResourceViews[i]), "PSSetShaderResources") : nullptr;
        if (neu != nullptr && neu->textur != nullptr) {
            gebraucht(neu->textur);
            if (istZiel(neu->textur)) {
                neu = nullptr;   // Direct3D 11 bindet eine Textur nicht zum Lesen, solange sie Ziel ist
            } else if (neu->textur->zustand != kLesbar) {
                schmutz |= kSSrvZustand;
            }
        }
        if (srv[platz] == neu) { continue; }
        bindeSrv(platz, neu);
        srvSchmutz |= 1U << platz;
    }
}

void STDMETHODCALLTYPE Kontext::PSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers) {
    for (UINT i = 0; i < NumSamplers; ++i) {
        const UINT platz = StartSlot + i;
        if (platz >= kAbtasterPlaetze) {
            if (ppSamplers != nullptr && ppSamplers[i] != nullptr) { (void)fehlt("PSSetSamplers(Platz ueber 3)"); }
            continue;
        }
        Abtaster* neu = ppSamplers != nullptr ? jetzig(static_cast<Abtaster*>(ppSamplers[i]), "PSSetSamplers") : nullptr;
        if (abtaster[platz] == neu) { continue; }
        binde(abtaster[platz], neu);
        abtasterSchmutz |= 1U << platz;
    }
}

void STDMETHODCALLTYPE Kontext::VSSetShader(ID3D11VertexShader* pVertexShader, ID3D11ClassInstance* const*, UINT) {
    auto* neu = static_cast<VsObjekt*>(pVertexShader);
    if (vs == neu) { return; }
    binde(vs, neu);
    schmutz |= kSPso;
}

void STDMETHODCALLTYPE Kontext::PSSetShader(ID3D11PixelShader* pPixelShader, ID3D11ClassInstance* const*, UINT) {
    auto* neu = static_cast<PsObjekt*>(pPixelShader);
    if (ps == neu) { return; }
    binde(ps, neu);
    schmutz |= kSPso;
}

void STDMETHODCALLTYPE Kontext::GSSetShader(ID3D11GeometryShader* pShader, ID3D11ClassInstance* const*, UINT) {
    auto* neu = static_cast<GsObjekt*>(pShader);
    if (gs == neu) { return; }
    binde(gs, neu);
    schmutz |= kSPso;
}

void STDMETHODCALLTYPE Kontext::IASetInputLayout(ID3D11InputLayout* pInputLayout) {
    auto* neu = static_cast<Eingabe*>(pInputLayout);
    if (eingabe == neu) { return; }
    binde(eingabe, neu);
    schmutz |= kSPso;
}

void STDMETHODCALLTYPE Kontext::IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppVertexBuffers, const UINT* pStrides, const UINT* pOffsets) {
    for (UINT i = 0; i < NumBuffers; ++i) {
        const UINT platz = StartSlot + i;
        if (platz >= kVbPlaetze) {
            if (ppVertexBuffers != nullptr && ppVertexBuffers[i] != nullptr) { (void)fehlt("IASetVertexBuffers(Platz ueber 3)"); }
            continue;
        }
        Puffer* neu = ppVertexBuffers != nullptr ? jetzig(static_cast<Puffer*>(ppVertexBuffers[i]), "IASetVertexBuffers") : nullptr;
        const UINT schritt = pStrides != nullptr ? pStrides[i] : 0U;
        const UINT versatz = pOffsets != nullptr ? pOffsets[i] : 0U;
        if (vb[platz] == neu && vbSchritt[platz] == schritt && vbVersatz[platz] == versatz) { continue; }
        if (vb[platz] != neu) { bindeVb(platz, neu); }
        vbSchritt[platz] = schritt;
        vbVersatz[platz] = versatz;
        schmutz |= kSVb;
    }
}

void STDMETHODCALLTYPE Kontext::IASetIndexBuffer(ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, UINT Offset) {
    auto* neu = jetzig(static_cast<Puffer*>(pIndexBuffer), "IASetIndexBuffer");
    if (ib == neu && ibFormat == Format && ibVersatz == Offset) { return; }
    if (ib != neu) { bindeIb(neu); }
    ibFormat = Format;
    ibVersatz = Offset;
    schmutz |= kSIb;
}

void STDMETHODCALLTYPE Kontext::IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology) {
    if (topologie == Topology) { return; }
    if (topologieTyp(topologie) != topologieTyp(Topology)) { schmutz |= kSPso; }
    topologie = Topology;
    schmutz |= kSTopologie;
}

void STDMETHODCALLTYPE Kontext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) {
    if (!vorZeichnen()) { return; }
    offeneListe->DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
    ++zaehlerIntern().zeichnen;
}

void STDMETHODCALLTYPE Kontext::Draw(UINT VertexCount, UINT StartVertexLocation) {
    if (!vorZeichnen()) { return; }
    offeneListe->DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
    ++zaehlerIntern().zeichnen;
}

void STDMETHODCALLTYPE Kontext::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation,
                                                     UINT StartInstanceLocation) {
    if (!vorZeichnen()) { return; }
    offeneListe->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
    ++zaehlerIntern().zeichnen;
}

void STDMETHODCALLTYPE Kontext::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation) {
    if (!vorZeichnen()) { return; }
    offeneListe->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
    ++zaehlerIntern().zeichnen;
}

HRESULT STDMETHODCALLTYPE Kontext::Map(ID3D11Resource* pResource, UINT Subresource, D3D11_MAP MapType, UINT, D3D11_MAPPED_SUBRESOURCE* pMappedResource) {
    if (pResource == nullptr || pMappedResource == nullptr) { return E_INVALIDARG; }
    {
        // Ein Objekt des frueheren Geraets: sein Block liegt in freigegebenem Speicher - ablehnen (alsPuffer/alsTextur melden es)
        D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        pResource->GetType(&dim);
        if (dim == D3D11_RESOURCE_DIMENSION_BUFFER ? alsPuffer(pResource) == nullptr : alsTextur(pResource) == nullptr) { return DXGI_ERROR_DEVICE_REMOVED; }
    }
    if (Puffer* p = alsPuffer(pResource); p != nullptr) {
        if (!p->dynamisch) { return fehlt("Map(Puffer ohne D3D11_USAGE_DYNAMIC)"); }
        if (MapType == D3D11_MAP_WRITE_DISCARD) {
            const Block neu = blockWieNehmen(p->block);
            if (neu.res == nullptr) { return E_OUTOFMEMORY; }
            blockSpaeterFrei(p->block);
            p->block = neu;
            (void)pufferGeweckt(p);
            p->va = neu.va;
            ++p->fassung;
            pufferUmbenannt(p);
            ++zaehlerIntern().umbenannt;
        } else if (MapType == D3D11_MAP_WRITE_NO_OVERWRITE) {
            if (pufferGeweckt(p)) { pufferUmbenannt(p); }
        } else {
            return fehlt("Map(Puffer, andere Art als DISCARD/NO_OVERWRITE)");
        }
        pMappedResource->pData = p->block.cpu;
        pMappedResource->RowPitch = p->art.ByteWidth;
        pMappedResource->DepthPitch = p->art.ByteWidth;
        return S_OK;
    }
    Textur* t = alsTextur(pResource);
    if (t == nullptr || !t->lesepuffer || MapType != D3D11_MAP_READ || Subresource >= t->abdruck.size()) { return fehlt("Map(Textur, nicht STAGING/READ)"); }
    if (t->kopiert) {   // die Kopie muss auf der Grafikkarte gelaufen sein
        std::uint64_t zaun = 0;
        if (!zaunDer(t->kopieSerie, zaun)) { zaun = rahmen::abschicken(); }
        rahmen::warten(zaun);
        t->kopiert = false;
    }
    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& fp = t->abdruck[Subresource];
    if (t->ebenen == 2U) {
        // Direct3D 11 bildet Tiefe und Stencil in einem Bildpunkt ab: D32_FLOAT_S8X24 als float + 32 Bit (Stencil im untersten
        // Byte), D24_UNORM_S8 als 24 Bit Tiefe + 8 Bit Stencil. Direct3D 12 liefert zwei Ebenen - hier zusammensetzen.
        const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& fs = t->abdruck[Subresource + t->teile()];
        const bool d32 = t->art.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        const UINT w = fp.Footprint.Width;
        const UINT h = fp.Footprint.Height;
        const UINT punkt = d32 ? 8U : 4U;
        t->zusammen.resize(static_cast<std::size_t>(w) * h * punkt);
        for (UINT y = 0; y < h; ++y) {
            const unsigned char* tz = t->cpu + fp.Offset + static_cast<std::size_t>(y) * fp.Footprint.RowPitch;
            const unsigned char* sz = t->cpu + fs.Offset + static_cast<std::size_t>(y) * fs.Footprint.RowPitch;
            unsigned char* z = t->zusammen.data() + static_cast<std::size_t>(y) * w * punkt;
            for (UINT x = 0; x < w; ++x) {
                std::uint32_t tiefeBits = 0;
                std::memcpy(&tiefeBits, tz + static_cast<std::size_t>(x) * 4U, 4U);
                if (d32) {
                    const std::uint32_t st = sz[x];
                    std::memcpy(z + static_cast<std::size_t>(x) * 8U, &tiefeBits, 4U);
                    std::memcpy(z + static_cast<std::size_t>(x) * 8U + 4U, &st, 4U);
                } else {
                    const std::uint32_t beide = (tiefeBits & 0x00FFFFFFU) | (static_cast<std::uint32_t>(sz[x]) << 24U);
                    std::memcpy(z + static_cast<std::size_t>(x) * 4U, &beide, 4U);
                }
            }
        }
        pMappedResource->pData = t->zusammen.data();
        pMappedResource->RowPitch = w * punkt;
        pMappedResource->DepthPitch = w * punkt * h;
        return S_OK;
    }
    pMappedResource->pData = t->cpu + fp.Offset;
    pMappedResource->RowPitch = fp.Footprint.RowPitch;
    pMappedResource->DepthPitch = fp.Footprint.RowPitch * fp.Footprint.Height;
    return S_OK;
}

void STDMETHODCALLTYPE Kontext::Unmap(ID3D11Resource*, UINT) {}

void STDMETHODCALLTYPE Kontext::Begin(ID3D11Asynchronous* pAsync) {
    if (pAsync == nullptr) { return; }
    static_cast<Abfrage*>(static_cast<ID3D11Query*>(pAsync))->beendet = false;
}

void STDMETHODCALLTYPE Kontext::End(ID3D11Asynchronous* pAsync) {
    if (pAsync == nullptr) { return; }
    auto* q = static_cast<Abfrage*>(static_cast<ID3D11Query*>(pAsync));
    if (q->art.Query == D3D11_QUERY_TIMESTAMP) {
        q->platz = markeNehmen();
        if (q->platz >= 0) {
            // D3D12-4: oeffnet erst die Marke die Liste (Bildmessung am Anfang des Bildes), startet sie die Uhr der Teilabgabe nicht -
            // sonst ginge eine Liste mit nichts als der Marke beim ersten Zielwechsel ab, nur weil gemessen wird.
            const bool warZu = offeneListe == nullptr;
            ID3D12GraphicsCommandList* const cl = liste();
            if (warZu) { listeSeit = 0; }
            cl->EndQuery(markenStapel(), D3D12_QUERY_TYPE_TIMESTAMP, static_cast<UINT>(q->platz));
        }
    }
    q->serie = rahmen::ausfuehrung();
    q->beendet = true;
}

HRESULT STDMETHODCALLTYPE Kontext::GetData(ID3D11Asynchronous* pAsync, void* pData, UINT DataSize, UINT GetDataFlags) {
    if (pAsync == nullptr) { return E_INVALIDARG; }
    auto* q = static_cast<Abfrage*>(static_cast<ID3D11Query*>(pAsync));
    if (!q->beendet) { return S_FALSE; }
    std::uint64_t zaun = 0;
    if (!zaunDer(q->serie, zaun)) {
        if ((GetDataFlags & D3D11_ASYNC_GETDATA_DONOTFLUSH) != 0U) { return S_FALSE; }
        zaun = rahmen::abschicken();
    }
    if (!rahmen::fertig(zaun)) { return S_FALSE; }
    if (pData == nullptr) { return S_OK; }
    switch (q->art.Query) {
    case D3D11_QUERY_TIMESTAMP: {
        std::uint64_t wert = 0;
        if (DataSize < sizeof(wert) || !markeLesen(q->platz, wert)) { return E_FAIL; }
        if (g_markenArbeitsuhr) { wert -= std::min(wert, rahmen::leerlaufBis(q->serie)); }
        std::memcpy(pData, &wert, sizeof(wert));
        return S_OK;
    }
    case D3D11_QUERY_TIMESTAMP_DISJOINT: {
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT dj{};
        if (g_markenFrequenz == 0U && FAILED(rahmen::schlange()->GetTimestampFrequency(&g_markenFrequenz))) { g_markenFrequenz = 0; }
        if (DataSize < sizeof(dj) || g_markenFrequenz == 0U) { return E_FAIL; }
        dj.Frequency = g_markenFrequenz;
        dj.Disjoint = FALSE;
        std::memcpy(pData, &dj, sizeof(dj));
        return S_OK;
    }
    default: {
        const BOOL ja = TRUE;
        if (DataSize < sizeof(ja)) { return E_FAIL; }
        std::memcpy(pData, &ja, sizeof(ja));
        return S_OK;
    }
    }
}

void STDMETHODCALLTYPE Kontext::OMSetRenderTargets(UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView) {
    // Teilabgabe: zeichnet die offene Liste schon g_teilAbgabe Takte lang auf, geht der bisherige Teil des Bildes an der Grenze
    // zum naechsten Ziel an die Grafikkarte - sie arbeitet dann schon, waehrend der Hauptfaden weiter aufzeichnet (so schickt
    // auch der D3D11-Treiber in Teilen ab). Die neue Liste setzt alle Zustaende neu (listeNeu).
    // Hoechstens g_teilAbgabenMax je Bild: unter fremder Last laeuft die Uhr weiter, waehrend der Hauptfaden wartet.
    if (offeneListe != nullptr && g_teilAbgabe != 0) {
        if (teilBild != g_gebrauchBild) {
            teilBild = g_gebrauchBild;
            teilImBild = 0;
        }
        LARGE_INTEGER t{};
        QueryPerformanceCounter(&t);
        if (listeSeit == 0) {
            listeSeit = t.QuadPart;   // die Liste trug bisher nur Zeitmarken: die Uhr beginnt jetzt
        } else if (teilImBild < g_teilAbgabenMax && t.QuadPart - listeSeit >= g_teilAbgabe) {
            (void)rahmen::abschicken();
            ++zaehlerIntern().teilAbgaben;
            ++teilImBild;
        }
    }
    if (NumViews > kZiele) {
        (void)fehlt("OMSetRenderTargets(mehr als 4 Ziele)");
        NumViews = kZiele;
    }
    bool anders = NumViews != ziele;
    for (UINT i = 0; i < kZiele; ++i) {
        ZielSicht* neu = (i < NumViews && ppRenderTargetViews != nullptr) ? jetzig(static_cast<ZielSicht*>(ppRenderTargetViews[i]), "OMSetRenderTargets") : nullptr;
        if (ziel[i] == neu) { continue; }
        binde(ziel[i], neu);
        anders = true;
    }
    // wie Direct3D 11: leere Ziele am Ende zaehlen nicht
    while (NumViews > 0U && ziel[NumViews - 1U] == nullptr) { --NumViews; }
    ziele = NumViews;
    auto* neuTiefe = jetzig(static_cast<TiefeSicht*>(pDepthStencilView), "OMSetRenderTargets");
    if (tiefeZiel != neuTiefe) {
        binde(tiefeZiel, neuTiefe);
        anders = true;
    }
    if (!anders) { return; }
    schmutz |= kSZiele | kSPso;
    // was jetzt Ziel ist, faellt aus den Textur-Plaetzen (Direct3D 11 setzt sie dort auf leer)
    for (UINT i = 0; i < kSrvPlaetze; ++i) {
        if (srv[i] != nullptr && srv[i]->textur != nullptr && istZiel(srv[i]->textur)) {
            bindeSrv(i, nullptr);
            srvSchmutz |= 1U << i;
        }
    }
}

void STDMETHODCALLTYPE Kontext::OMSetBlendState(ID3D11BlendState* pBlendState, const FLOAT BlendFactor[4], UINT SampleMask) {
    auto* neu = static_cast<Mischung*>(pBlendState);
    if (mischung != neu) {
        binde(mischung, neu);
        schmutz |= kSPso;
    }
    if (probenMaske != SampleMask) {
        probenMaske = SampleMask;
        schmutz |= kSPso;
    }
    const float eins[4] = {1.0F, 1.0F, 1.0F, 1.0F};
    const float* f = BlendFactor != nullptr ? BlendFactor : eins;   // ohne Angabe 1, 1, 1, 1 (Direct3D 11)
    if (std::memcmp(mischFaktor, f, sizeof(mischFaktor)) != 0) {
        std::memcpy(mischFaktor, f, sizeof(mischFaktor));
        schmutz |= kSFaktor;
    }
}

void STDMETHODCALLTYPE Kontext::OMSetDepthStencilState(ID3D11DepthStencilState* pDepthStencilState, UINT StencilRef) {
    auto* neu = static_cast<TiefeZustand*>(pDepthStencilState);
    if (tiefe != neu) {
        binde(tiefe, neu);
        schmutz |= kSPso;
    }
    if (stencilWert != StencilRef) {
        stencilWert = StencilRef;
        schmutz |= kSStencil;
    }
}

void STDMETHODCALLTYPE Kontext::RSSetState(ID3D11RasterizerState* pRasterizerState) {
    auto* neu = static_cast<Raster*>(pRasterizerState);
    if (raster == neu) { return; }
    binde(raster, neu);
    schmutz |= kSPso | kSSchere;
}

void STDMETHODCALLTYPE Kontext::RSSetViewports(UINT NumViewports, const D3D11_VIEWPORT* pViewports) {
    if (NumViewports > 4U) { NumViewports = 4; }
    sichtfelder = pViewports != nullptr ? NumViewports : 0U;
    for (UINT i = 0; i < sichtfelder; ++i) {
        // D3D11_VIEWPORT und D3D12_VIEWPORT: dieselben sechs Felder
        sichtfeld[i] = D3D12_VIEWPORT{pViewports[i].TopLeftX, pViewports[i].TopLeftY, pViewports[i].Width, pViewports[i].Height, pViewports[i].MinDepth, pViewports[i].MaxDepth};
    }
    schmutz |= kSSichtfeld;
}

void STDMETHODCALLTYPE Kontext::RSSetScissorRects(UINT NumRects, const D3D11_RECT* pRects) {
    if (NumRects > 4U) { NumRects = 4; }
    scheren = pRects != nullptr ? NumRects : 0U;
    for (UINT i = 0; i < scheren; ++i) { schere[i] = pRects[i]; }
    schmutz |= kSSchere;
}

void STDMETHODCALLTYPE Kontext::CopySubresourceRegion(ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource* pSrcResource,
                                                      UINT SrcSubresource, const D3D11_BOX* pSrcBox) {
    Textur* nach = alsTextur(pDstResource);
    Textur* von = alsTextur(pSrcResource);
    if (nach == nullptr || von == nullptr || von->lesepuffer) {
        (void)fehlt("CopySubresourceRegion(Puffer oder aus einem Lesepuffer)");
        return;
    }
    if (!nach->lesepuffer && !nach->schichtHeap.empty()) {
        // duenne Texturfelder (eine Stufe: Teil = Schicht): aus einer leeren Schicht in eine leere ist nichts zu kopieren - beide
        // lesen 0; sonst bekommt die Zielschicht jetzt Speicher
        if (!von->schichtHeap.empty() && !schichtBelegt(von, SrcSubresource) && !schichtBelegt(nach, DstSubresource)) { return; }
        schichtBelegen(nach, DstSubresource, 1);
    }
    ID3D12GraphicsCommandList* cl = liste();
    D3D12_TEXTURE_COPY_LOCATION q{};
    q.pResource = von->res;
    q.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    q.SubresourceIndex = SrcSubresource;
    D3D12_TEXTURE_COPY_LOCATION z{};
    z.pResource = nach->res;
    if (nach->lesepuffer) {
        if (DstSubresource >= nach->abdruck.size()) { return; }
        stelle(*this, cl, von, D3D12_RESOURCE_STATE_COPY_SOURCE);
        z.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        z.PlacedFootprint = nach->abdruck[DstSubresource];
        nach->kopiert = true;
        nach->kopieSerie = rahmen::ausfuehrung();
    } else {
        stelleZwei(*this, cl, von, D3D12_RESOURCE_STATE_COPY_SOURCE, nach, D3D12_RESOURCE_STATE_COPY_DEST);
        z.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        z.SubresourceIndex = DstSubresource;
    }
    D3D12_BOX kasten{};
    if (pSrcBox != nullptr) { kasten = D3D12_BOX{pSrcBox->left, pSrcBox->top, pSrcBox->front, pSrcBox->right, pSrcBox->bottom, pSrcBox->back}; }
    if (protokoll::g_an) { protokoll::eintrag(cl, "Kopieren (Teil) " + protokoll::nenne(von) + " -> " + (nach->lesepuffer ? "Lesepuffer " : "") + protokoll::nenne(nach)); }
    cl->CopyTextureRegion(&z, DstX, DstY, DstZ, &q, pSrcBox != nullptr ? &kasten : nullptr);
}

void STDMETHODCALLTYPE Kontext::CopyResource(ID3D11Resource* pDstResource, ID3D11Resource* pSrcResource) {
    Textur* nach = alsTextur(pDstResource);
    Textur* von = alsTextur(pSrcResource);
    if (nach == nullptr || von == nullptr || von->lesepuffer) {
        (void)fehlt("CopyResource(Puffer oder aus einem Lesepuffer)");
        return;
    }
    ID3D12GraphicsCommandList* cl = liste();
    if (!nach->lesepuffer) {
        schichtBelegen(nach, 0, ~0U);
        stelleZwei(*this, cl, von, D3D12_RESOURCE_STATE_COPY_SOURCE, nach, D3D12_RESOURCE_STATE_COPY_DEST);
        if (protokoll::g_an) { protokoll::eintrag(cl, "Kopieren " + protokoll::nenne(von) + " -> " + protokoll::nenne(nach)); }
        cl->CopyResource(nach->res, von->res);
        return;
    }
    stelle(*this, cl, von, D3D12_RESOURCE_STATE_COPY_SOURCE);
    if (protokoll::g_an) { protokoll::eintrag(cl, "Kopieren " + protokoll::nenne(von) + " -> Lesepuffer " + protokoll::nenne(nach)); }
    const UINT n = std::min(von->teile() * nach->ebenen, static_cast<UINT>(nach->abdruck.size()));
    for (UINT i = 0; i < n; ++i) {
        D3D12_TEXTURE_COPY_LOCATION q{};
        q.pResource = von->res;
        q.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        q.SubresourceIndex = i;
        D3D12_TEXTURE_COPY_LOCATION z{};
        z.pResource = nach->res;
        z.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        z.PlacedFootprint = nach->abdruck[i];
        cl->CopyTextureRegion(&z, 0, 0, 0, &q, nullptr);
    }
    nach->kopiert = true;
    nach->kopieSerie = rahmen::ausfuehrung();
}

void STDMETHODCALLTYPE Kontext::UpdateSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch,
                                                  UINT) {
    if (pDstResource == nullptr || pSrcData == nullptr) { return; }
    ID3D12GraphicsCommandList* cl = liste();
    if (Puffer* p = alsPuffer(pDstResource); p != nullptr) {
        if (p->dynamisch) {
            (void)fehlt("UpdateSubresource(Puffer mit D3D11_USAGE_DYNAMIC)");
            return;
        }
        const UINT anfang = pDstBox != nullptr ? pDstBox->left : 0U;
        const UINT ende = pDstBox != nullptr ? pDstBox->right : p->art.ByteWidth;
        if (ende <= anfang || ende > p->art.ByteWidth) { return; }
        ID3D12Resource* puffer = nullptr;
        std::uint64_t versatz = 0;
        unsigned char* zielZeiger = nullptr;
        if (!rahmen::hochladePlatz(ende - anfang, false, puffer, versatz, zielZeiger)) { return; }
        std::memcpy(zielZeiger, pSrcData, ende - anfang);
        if (protokoll::g_an) { protokoll::eintrag(cl, "Hochladen in einen Puffer: " + std::to_string(ende - anfang) + " Bytes von " + std::to_string(p->art.ByteWidth)); }
        // In dieser Ausfuehrung schon gelesen: aus dem Lesezustand nach COPY_DEST. Sonst steht der Puffer in COMMON, und das
        // Kopieren stellt ihn von selbst auf COPY_DEST. Er bleibt dort fuer weitere Kopien (D3D12-4: je UpdateSubresource zwei
        // Barrieren liessen die Grafikkarte bei jedem Stueck leerlaufen); zurueck geht er vor dem naechsten Zeichenbefehl,
        // zusammen mit allen anderen so beschriebenen (kopierZieleLesbar).
        const std::uint64_t jetzt = rahmen::ausfuehrung();
        if (p->kopierSerie != jetzt) {
            if (p->serie == jetzt) {
                D3D12_RESOURCE_BARRIER b{};
                b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                b.Transition.pResource = p->res;
                b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                b.Transition.StateBefore = p->lesen;
                b.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                cl->ResourceBarrier(1, &b);
                zaehleBarriere();
            }
            p->kopierSerie = jetzt;
            kopierZiele.push_back(p);
        }
        cl->CopyBufferRegion(p->res, anfang, puffer, versatz, ende - anfang);
        p->serie = ~0ULL;   // nicht im Lesezustand, bis kopierZieleLesbar
        return;
    }
    Textur* t = alsTextur(pDstResource);
    if (t == nullptr || t->lesepuffer || DstSubresource >= t->teile()) {
        (void)fehlt("UpdateSubresource(Lesepuffer)");
        return;
    }
    const UINT stufe = DstSubresource % t->art.MipLevels;
    const UINT ganzW = std::max(1U, t->art.Width >> stufe);
    const UINT ganzH = std::max(1U, t->art.Height >> stufe);
    const UINT x0 = pDstBox != nullptr ? pDstBox->left : 0U;
    const UINT y0 = pDstBox != nullptr ? pDstBox->top : 0U;
    const UINT w = pDstBox != nullptr ? pDstBox->right - pDstBox->left : ganzW;
    const UINT h = pDstBox != nullptr ? pDstBox->bottom - pDstBox->top : ganzH;
    if (w == 0U || h == 0U) { return; }
    // Aufteilung des Rechtecks im Hochladespeicher wie eine eigene Textur dieser Groesse (Zeilen, Zeilenlaenge, Bloecke bei DXT)
    D3D12_RESOURCE_DESC rd{};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rd.Width = w;
    rd.Height = h;
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;
    rd.Format = t->format12;
    rd.SampleDesc.Count = 1;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};
    UINT zeilen = 0;
    UINT64 zeilenBytes = 0;
    UINT64 gesamt = 0;
    rahmen::geraet()->GetCopyableFootprints(&rd, 0, 1, 0, &fp, &zeilen, &zeilenBytes, &gesamt);
    ID3D12Resource* puffer = nullptr;
    std::uint64_t versatz = 0;
    unsigned char* zielZeiger = nullptr;
    if (gesamt == 0U || !rahmen::hochladePlatz(gesamt, false, puffer, versatz, zielZeiger)) { return; }
    const auto* quelle = static_cast<const unsigned char*>(pSrcData);
    for (UINT y = 0; y < zeilen; ++y) {
        std::memcpy(zielZeiger + static_cast<std::size_t>(y) * fp.Footprint.RowPitch, quelle + static_cast<std::size_t>(y) * SrcRowPitch,
                    static_cast<std::size_t>(zeilenBytes));
    }
    schichtBelegen(t, DstSubresource, 1);
    stelle(*this, cl, t, D3D12_RESOURCE_STATE_COPY_DEST);
    if (protokoll::g_an) { protokoll::eintrag(cl, "Hochladen in eine Textur: " + std::to_string(gesamt) + " Bytes nach " + protokoll::nenne(t)); }
    D3D12_TEXTURE_COPY_LOCATION q{};
    q.pResource = puffer;
    q.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    q.PlacedFootprint = fp;
    q.PlacedFootprint.Offset = versatz;
    D3D12_TEXTURE_COPY_LOCATION z{};
    z.pResource = t->res;
    z.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    z.SubresourceIndex = DstSubresource;
    cl->CopyTextureRegion(&z, x0, y0, 0, &q, nullptr);
}

void STDMETHODCALLTYPE Kontext::ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4]) {
    if (jetzig(static_cast<ZielSicht*>(pRenderTargetView), "ClearRenderTargetView") == nullptr) { return; }
    auto* s = static_cast<ZielSicht*>(pRenderTargetView);
    ID3D12GraphicsCommandList* cl = liste();
    zielSchichtenBelegen(s);
    stelle(*this, cl, s->textur, D3D12_RESOURCE_STATE_RENDER_TARGET);
    if (protokoll::g_an) {
        char f[80];
        std::snprintf(f, sizeof(f), " mit %.3g %.3g %.3g %.3g", ColorRGBA[0], ColorRGBA[1], ColorRGBA[2], ColorRGBA[3]);
        protokoll::eintrag(cl, "Loeschen Ziel " + protokoll::nenne(s->textur) + f);
    }
    cl->ClearRenderTargetView(s->griff, ColorRGBA, 0, nullptr);
}

void STDMETHODCALLTYPE Kontext::ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil) {
    if (jetzig(static_cast<TiefeSicht*>(pDepthStencilView), "ClearDepthStencilView") == nullptr || ClearFlags == 0U) { return; }
    auto* s = static_cast<TiefeSicht*>(pDepthStencilView);
    ID3D12GraphicsCommandList* cl = liste();
    stelle(*this, cl, s->textur, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    // D3D11_CLEAR_DEPTH 1, D3D11_CLEAR_STENCIL 2 - dieselben Werte; Stencil nur, wenn das Format einen hat
    UINT was = ClearFlags & 3U;
    if (s->format != DXGI_FORMAT_D32_FLOAT_S8X24_UINT && s->format != DXGI_FORMAT_D24_UNORM_S8_UINT) { was &= 1U; }
    if (protokoll::g_an) {
        char f[80];
        std::snprintf(f, sizeof(f), " (Teile %u) mit %.3g / %u", was, Depth, static_cast<unsigned>(Stencil));
        protokoll::eintrag(cl, "Loeschen Tiefe " + protokoll::nenne(s->textur) + f);
    }
    if (was != 0U) { cl->ClearDepthStencilView(s->griff, static_cast<D3D12_CLEAR_FLAGS>(was), Depth, Stencil, 0, nullptr); }
}

void STDMETHODCALLTYPE Kontext::GenerateMips(ID3D11ShaderResourceView* pShaderResourceView) {
    if (jetzig(static_cast<Sicht*>(pShaderResourceView), "GenerateMips") == nullptr) { return; }
    if (protokoll::g_an) { protokoll::eintrag(liste(), "Mip-Stufen " + protokoll::nenne(static_cast<Sicht*>(pShaderResourceView)->textur)); }
    mipsErzeugen(liste(), static_cast<Sicht*>(pShaderResourceView));
}

void STDMETHODCALLTYPE Kontext::ResolveSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, ID3D11Resource* pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format) {
    Textur* nach = alsTextur(pDstResource);
    Textur* von = alsTextur(pSrcResource);
    if (nach == nullptr || von == nullptr || nach->lesepuffer || von->lesepuffer) { return; }
    ID3D12GraphicsCommandList* cl = liste();
    schichtBelegen(nach, DstSubresource, 1);
    stelleZwei(*this, cl, von, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, nach, D3D12_RESOURCE_STATE_RESOLVE_DEST);
    if (protokoll::g_an) { protokoll::eintrag(cl, "Aufloesen " + protokoll::nenne(von) + " -> " + protokoll::nenne(nach)); }
    cl->ResolveSubresource(nach->res, DstSubresource, von->res, SrcSubresource, Format);
}

void STDMETHODCALLTYPE Kontext::OMGetRenderTargets(UINT NumViews, ID3D11RenderTargetView** ppRenderTargetViews, ID3D11DepthStencilView** ppDepthStencilView) {
    for (UINT i = 0; i < NumViews && ppRenderTargetViews != nullptr; ++i) {
        ppRenderTargetViews[i] = (i < kZiele) ? ziel[i] : nullptr;
        if (ppRenderTargetViews[i] != nullptr) { ppRenderTargetViews[i]->AddRef(); }
    }
    if (ppDepthStencilView != nullptr) {
        *ppDepthStencilView = tiefeZiel;
        if (tiefeZiel != nullptr) { tiefeZiel->AddRef(); }
    }
}

void STDMETHODCALLTYPE Kontext::RSGetViewports(UINT* pNumViewports, D3D11_VIEWPORT* pViewports) {
    if (pNumViewports == nullptr) { return; }
    if (pViewports == nullptr) {
        *pNumViewports = sichtfelder;
        return;
    }
    const UINT n = std::min(*pNumViewports, sichtfelder);
    for (UINT i = 0; i < n; ++i) {
        pViewports[i] = D3D11_VIEWPORT{sichtfeld[i].TopLeftX, sichtfeld[i].TopLeftY, sichtfeld[i].Width, sichtfeld[i].Height, sichtfeld[i].MinDepth, sichtfeld[i].MaxDepth};
    }
    *pNumViewports = n;
}

void STDMETHODCALLTYPE Kontext::ClearState() { allesLoesen(); }

void STDMETHODCALLTYPE Kontext::Flush() {
    if (offeneListe != nullptr) { (void)rahmen::abschicken(); }
}

D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE Kontext::GetType() { return D3D11_DEVICE_CONTEXT_IMMEDIATE; }
UINT STDMETHODCALLTYPE Kontext::GetContextFlags() { return 0; }

// Stufen, die die Zeichenschichten nicht benutzen: leeren heisst nichts tun.
void STDMETHODCALLTYPE Kontext::VSSetShaderResources(UINT, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews) {
    for (UINT i = 0; i < NumViews && ppShaderResourceViews != nullptr; ++i) {
        if (ppShaderResourceViews[i] != nullptr) { (void)fehlt("VSSetShaderResources"); }
    }
}
void STDMETHODCALLTYPE Kontext::GSSetShaderResources(UINT, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews) {
    for (UINT i = 0; i < NumViews && ppShaderResourceViews != nullptr; ++i) {
        if (ppShaderResourceViews[i] != nullptr) { (void)fehlt("GSSetShaderResources"); }
    }
}
void STDMETHODCALLTYPE Kontext::VSSetSamplers(UINT, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers) {
    for (UINT i = 0; i < NumSamplers && ppSamplers != nullptr; ++i) {
        if (ppSamplers[i] != nullptr) { (void)fehlt("VSSetSamplers"); }
    }
}
void STDMETHODCALLTYPE Kontext::GSSetSamplers(UINT, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers) {
    for (UINT i = 0; i < NumSamplers && ppSamplers != nullptr; ++i) {
        if (ppSamplers[i] != nullptr) { (void)fehlt("GSSetSamplers"); }
    }
}
void STDMETHODCALLTYPE Kontext::HSSetShader(ID3D11HullShader* pHullShader, ID3D11ClassInstance* const*, UINT) {
    if (pHullShader != nullptr) { (void)fehlt("HSSetShader"); }
}
void STDMETHODCALLTYPE Kontext::DSSetShader(ID3D11DomainShader* pDomainShader, ID3D11ClassInstance* const*, UINT) {
    if (pDomainShader != nullptr) { (void)fehlt("DSSetShader"); }
}
void STDMETHODCALLTYPE Kontext::CSSetShader(ID3D11ComputeShader* pComputeShader, ID3D11ClassInstance* const*, UINT) {
    if (pComputeShader != nullptr) { (void)fehlt("CSSetShader"); }
}

void STDMETHODCALLTYPE Kontext::SetPredication(ID3D11Predicate *, BOOL) { (void)fehlt("SetPredication"); }
void STDMETHODCALLTYPE Kontext::OMSetRenderTargetsAndUnorderedAccessViews(UINT, ID3D11RenderTargetView *const *, ID3D11DepthStencilView *, UINT, UINT, ID3D11UnorderedAccessView *const *, const UINT *) { (void)fehlt("OMSetRenderTargetsAndUnorderedAccessViews"); }
void STDMETHODCALLTYPE Kontext::SOSetTargets(UINT, ID3D11Buffer *const *, const UINT *) { (void)fehlt("SOSetTargets"); }
void STDMETHODCALLTYPE Kontext::DrawAuto() { (void)fehlt("DrawAuto"); }
void STDMETHODCALLTYPE Kontext::DrawIndexedInstancedIndirect(ID3D11Buffer *, UINT) { (void)fehlt("DrawIndexedInstancedIndirect"); }
void STDMETHODCALLTYPE Kontext::DrawInstancedIndirect(ID3D11Buffer *, UINT) { (void)fehlt("DrawInstancedIndirect"); }
void STDMETHODCALLTYPE Kontext::Dispatch(UINT, UINT, UINT) { (void)fehlt("Dispatch"); }
void STDMETHODCALLTYPE Kontext::DispatchIndirect(ID3D11Buffer *, UINT) { (void)fehlt("DispatchIndirect"); }
void STDMETHODCALLTYPE Kontext::CopyStructureCount(ID3D11Buffer *, UINT, ID3D11UnorderedAccessView *) { (void)fehlt("CopyStructureCount"); }
void STDMETHODCALLTYPE Kontext::ClearUnorderedAccessViewUint(ID3D11UnorderedAccessView *, const UINT [ 4 ]) { (void)fehlt("ClearUnorderedAccessViewUint"); }
void STDMETHODCALLTYPE Kontext::ClearUnorderedAccessViewFloat(ID3D11UnorderedAccessView *, const FLOAT [ 4 ]) { (void)fehlt("ClearUnorderedAccessViewFloat"); }
void STDMETHODCALLTYPE Kontext::SetResourceMinLOD(ID3D11Resource *, FLOAT) { (void)fehlt("SetResourceMinLOD"); }
FLOAT STDMETHODCALLTYPE Kontext::GetResourceMinLOD(ID3D11Resource *) { (void)fehlt("GetResourceMinLOD"); return {}; }
void STDMETHODCALLTYPE Kontext::ExecuteCommandList(ID3D11CommandList *, BOOL) { (void)fehlt("ExecuteCommandList"); }
void STDMETHODCALLTYPE Kontext::HSSetShaderResources(UINT, UINT, ID3D11ShaderResourceView *const *) { (void)fehlt("HSSetShaderResources"); }
void STDMETHODCALLTYPE Kontext::HSSetSamplers(UINT, UINT, ID3D11SamplerState *const *) { (void)fehlt("HSSetSamplers"); }
void STDMETHODCALLTYPE Kontext::HSSetConstantBuffers(UINT, UINT, ID3D11Buffer *const *) { (void)fehlt("HSSetConstantBuffers"); }
void STDMETHODCALLTYPE Kontext::DSSetShaderResources(UINT, UINT, ID3D11ShaderResourceView *const *) { (void)fehlt("DSSetShaderResources"); }
void STDMETHODCALLTYPE Kontext::DSSetSamplers(UINT, UINT, ID3D11SamplerState *const *) { (void)fehlt("DSSetSamplers"); }
void STDMETHODCALLTYPE Kontext::DSSetConstantBuffers(UINT, UINT, ID3D11Buffer *const *) { (void)fehlt("DSSetConstantBuffers"); }
void STDMETHODCALLTYPE Kontext::CSSetShaderResources(UINT, UINT, ID3D11ShaderResourceView *const *) { (void)fehlt("CSSetShaderResources"); }
void STDMETHODCALLTYPE Kontext::CSSetUnorderedAccessViews(UINT, UINT, ID3D11UnorderedAccessView *const *, const UINT *) { (void)fehlt("CSSetUnorderedAccessViews"); }
void STDMETHODCALLTYPE Kontext::CSSetSamplers(UINT, UINT, ID3D11SamplerState *const *) { (void)fehlt("CSSetSamplers"); }
void STDMETHODCALLTYPE Kontext::CSSetConstantBuffers(UINT, UINT, ID3D11Buffer *const *) { (void)fehlt("CSSetConstantBuffers"); }
void STDMETHODCALLTYPE Kontext::VSGetConstantBuffers(UINT, UINT, ID3D11Buffer **) { (void)fehlt("VSGetConstantBuffers"); }
void STDMETHODCALLTYPE Kontext::PSGetShaderResources(UINT, UINT, ID3D11ShaderResourceView **) { (void)fehlt("PSGetShaderResources"); }
void STDMETHODCALLTYPE Kontext::PSGetShader(ID3D11PixelShader **, ID3D11ClassInstance **, UINT *) { (void)fehlt("PSGetShader"); }
void STDMETHODCALLTYPE Kontext::PSGetSamplers(UINT, UINT, ID3D11SamplerState **) { (void)fehlt("PSGetSamplers"); }
void STDMETHODCALLTYPE Kontext::VSGetShader(ID3D11VertexShader **, ID3D11ClassInstance **, UINT *) { (void)fehlt("VSGetShader"); }
void STDMETHODCALLTYPE Kontext::PSGetConstantBuffers(UINT, UINT, ID3D11Buffer **) { (void)fehlt("PSGetConstantBuffers"); }
void STDMETHODCALLTYPE Kontext::IAGetInputLayout(ID3D11InputLayout **) { (void)fehlt("IAGetInputLayout"); }
void STDMETHODCALLTYPE Kontext::IAGetVertexBuffers(UINT, UINT, ID3D11Buffer **, UINT *, UINT *) { (void)fehlt("IAGetVertexBuffers"); }
void STDMETHODCALLTYPE Kontext::IAGetIndexBuffer(ID3D11Buffer **, DXGI_FORMAT *, UINT *) { (void)fehlt("IAGetIndexBuffer"); }
void STDMETHODCALLTYPE Kontext::GSGetConstantBuffers(UINT, UINT, ID3D11Buffer **) { (void)fehlt("GSGetConstantBuffers"); }
void STDMETHODCALLTYPE Kontext::GSGetShader(ID3D11GeometryShader **, ID3D11ClassInstance **, UINT *) { (void)fehlt("GSGetShader"); }
void STDMETHODCALLTYPE Kontext::IAGetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY *) { (void)fehlt("IAGetPrimitiveTopology"); }
void STDMETHODCALLTYPE Kontext::VSGetShaderResources(UINT, UINT, ID3D11ShaderResourceView **) { (void)fehlt("VSGetShaderResources"); }
void STDMETHODCALLTYPE Kontext::VSGetSamplers(UINT, UINT, ID3D11SamplerState **) { (void)fehlt("VSGetSamplers"); }
void STDMETHODCALLTYPE Kontext::GetPredication(ID3D11Predicate **, BOOL *) { (void)fehlt("GetPredication"); }
void STDMETHODCALLTYPE Kontext::GSGetShaderResources(UINT, UINT, ID3D11ShaderResourceView **) { (void)fehlt("GSGetShaderResources"); }
void STDMETHODCALLTYPE Kontext::GSGetSamplers(UINT, UINT, ID3D11SamplerState **) { (void)fehlt("GSGetSamplers"); }
void STDMETHODCALLTYPE Kontext::OMGetRenderTargetsAndUnorderedAccessViews(UINT, ID3D11RenderTargetView **, ID3D11DepthStencilView **, UINT, UINT, ID3D11UnorderedAccessView **) { (void)fehlt("OMGetRenderTargetsAndUnorderedAccessViews"); }
void STDMETHODCALLTYPE Kontext::OMGetBlendState(ID3D11BlendState **, FLOAT [ 4 ], UINT *) { (void)fehlt("OMGetBlendState"); }
void STDMETHODCALLTYPE Kontext::OMGetDepthStencilState(ID3D11DepthStencilState **, UINT *) { (void)fehlt("OMGetDepthStencilState"); }
void STDMETHODCALLTYPE Kontext::SOGetTargets(UINT, ID3D11Buffer **) { (void)fehlt("SOGetTargets"); }
void STDMETHODCALLTYPE Kontext::RSGetState(ID3D11RasterizerState **) { (void)fehlt("RSGetState"); }
void STDMETHODCALLTYPE Kontext::RSGetScissorRects(UINT *, D3D11_RECT *) { (void)fehlt("RSGetScissorRects"); }
void STDMETHODCALLTYPE Kontext::HSGetShaderResources(UINT, UINT, ID3D11ShaderResourceView **) { (void)fehlt("HSGetShaderResources"); }
void STDMETHODCALLTYPE Kontext::HSGetShader(ID3D11HullShader **, ID3D11ClassInstance **, UINT *) { (void)fehlt("HSGetShader"); }
void STDMETHODCALLTYPE Kontext::HSGetSamplers(UINT, UINT, ID3D11SamplerState **) { (void)fehlt("HSGetSamplers"); }
void STDMETHODCALLTYPE Kontext::HSGetConstantBuffers(UINT, UINT, ID3D11Buffer **) { (void)fehlt("HSGetConstantBuffers"); }
void STDMETHODCALLTYPE Kontext::DSGetShaderResources(UINT, UINT, ID3D11ShaderResourceView **) { (void)fehlt("DSGetShaderResources"); }
void STDMETHODCALLTYPE Kontext::DSGetShader(ID3D11DomainShader **, ID3D11ClassInstance **, UINT *) { (void)fehlt("DSGetShader"); }
void STDMETHODCALLTYPE Kontext::DSGetSamplers(UINT, UINT, ID3D11SamplerState **) { (void)fehlt("DSGetSamplers"); }
void STDMETHODCALLTYPE Kontext::DSGetConstantBuffers(UINT, UINT, ID3D11Buffer **) { (void)fehlt("DSGetConstantBuffers"); }
void STDMETHODCALLTYPE Kontext::CSGetShaderResources(UINT, UINT, ID3D11ShaderResourceView **) { (void)fehlt("CSGetShaderResources"); }
void STDMETHODCALLTYPE Kontext::CSGetUnorderedAccessViews(UINT, UINT, ID3D11UnorderedAccessView **) { (void)fehlt("CSGetUnorderedAccessViews"); }
void STDMETHODCALLTYPE Kontext::CSGetShader(ID3D11ComputeShader **, ID3D11ClassInstance **, UINT *) { (void)fehlt("CSGetShader"); }
void STDMETHODCALLTYPE Kontext::CSGetSamplers(UINT, UINT, ID3D11SamplerState **) { (void)fehlt("CSGetSamplers"); }
void STDMETHODCALLTYPE Kontext::CSGetConstantBuffers(UINT, UINT, ID3D11Buffer **) { (void)fehlt("CSGetConstantBuffers"); }
HRESULT STDMETHODCALLTYPE Kontext::FinishCommandList(BOOL, ID3D11CommandList **) { return fehlt("FinishCommandList"); }
void STDMETHODCALLTYPE Kontext::CopySubresourceRegion1(ID3D11Resource *, UINT, UINT, UINT, UINT, ID3D11Resource *, UINT, const D3D11_BOX *, UINT) { (void)fehlt("CopySubresourceRegion1"); }
void STDMETHODCALLTYPE Kontext::UpdateSubresource1(ID3D11Resource *, UINT, const D3D11_BOX *, const void *, UINT, UINT, UINT) { (void)fehlt("UpdateSubresource1"); }
void STDMETHODCALLTYPE Kontext::DiscardResource(ID3D11Resource *) { (void)fehlt("DiscardResource"); }
void STDMETHODCALLTYPE Kontext::DiscardView(ID3D11View *) { (void)fehlt("DiscardView"); }
void STDMETHODCALLTYPE Kontext::HSSetConstantBuffers1(UINT, UINT, ID3D11Buffer *const *, const UINT *, const UINT *) { (void)fehlt("HSSetConstantBuffers1"); }
void STDMETHODCALLTYPE Kontext::DSSetConstantBuffers1(UINT, UINT, ID3D11Buffer *const *, const UINT *, const UINT *) { (void)fehlt("DSSetConstantBuffers1"); }
void STDMETHODCALLTYPE Kontext::CSSetConstantBuffers1(UINT, UINT, ID3D11Buffer *const *, const UINT *, const UINT *) { (void)fehlt("CSSetConstantBuffers1"); }
void STDMETHODCALLTYPE Kontext::VSGetConstantBuffers1(UINT, UINT, ID3D11Buffer **, UINT *, UINT *) { (void)fehlt("VSGetConstantBuffers1"); }
void STDMETHODCALLTYPE Kontext::HSGetConstantBuffers1(UINT, UINT, ID3D11Buffer **, UINT *, UINT *) { (void)fehlt("HSGetConstantBuffers1"); }
void STDMETHODCALLTYPE Kontext::DSGetConstantBuffers1(UINT, UINT, ID3D11Buffer **, UINT *, UINT *) { (void)fehlt("DSGetConstantBuffers1"); }
void STDMETHODCALLTYPE Kontext::GSGetConstantBuffers1(UINT, UINT, ID3D11Buffer **, UINT *, UINT *) { (void)fehlt("GSGetConstantBuffers1"); }
void STDMETHODCALLTYPE Kontext::PSGetConstantBuffers1(UINT, UINT, ID3D11Buffer **, UINT *, UINT *) { (void)fehlt("PSGetConstantBuffers1"); }
void STDMETHODCALLTYPE Kontext::CSGetConstantBuffers1(UINT, UINT, ID3D11Buffer **, UINT *, UINT *) { (void)fehlt("CSGetConstantBuffers1"); }
void STDMETHODCALLTYPE Kontext::SwapDeviceContextState(ID3DDeviceContextState *, ID3DDeviceContextState **) { (void)fehlt("SwapDeviceContextState"); }
void STDMETHODCALLTYPE Kontext::ClearView(ID3D11View *, const FLOAT [ 4 ], const D3D11_RECT *, UINT) { (void)fehlt("ClearView"); }
void STDMETHODCALLTYPE Kontext::DiscardView1(ID3D11View *, const D3D11_RECT *, UINT) { (void)fehlt("DiscardView1"); }

}  // namespace bhed::render::d3d12::abgeber
