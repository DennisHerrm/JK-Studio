// abgeber12_geraet_win32.cpp - D3D12-Abgeber: Geraet (ID3D11Device) und Objekte
//
// Aufbau und Gruende in abgeber12.h. Hier: anlegen, was die Zeichenschichten ueber ID3D11Device anlegen - als D3D12-Ressourcen
// und Beschreibungen. Regeln:
//   - D3D11_USAGE_DYNAMIC (Puffer): ein Block im Arbeitsspeicher, den die Grafikkarte liest; Map(WRITE_DISCARD) nimmt einen
//     frischen Block, der alte wird erst wieder vergeben, wenn die Grafikkarte die Listen abgearbeitet hat, die ihn lesen
//     (dasselbe "Umbenennen", das der D3D11-Treiber verborgen tut). Bloecke kommen aus Groessenklassen (256 B bis 64 KB aus
//     Seiten zu 1 MB, groessere einzeln) und liegen in gewoehnlichem Speicher mit Zwischenspeicher (hochladePuffer). Ruht der
//     Inhalt eines grossen Eck- oder Indexpuffers, wird er aus einer Kopie im Grafikspeicher gezeichnet (pufferRuhe).
//   - D3D11_USAGE_DEFAULT/IMMUTABLE: im Grafikspeicher; Anfangsdaten ueber den Hochladespeicher in der Vor-Liste. Freigegebene
//     Puffer werden wiederverwendet (g_vorrat). Grosse, leer angelegte Texturfelder bekommen ihren Speicher je Schicht beim
//     ersten Schreiben (schichtBelegen). Texturen, die lange in keiner Befehlsliste vorkamen, werden ausgelagert
//     (auslagernPruefen: Evict; gebraucht(): MakeResident) - Direct3D 11 haelt ebenso meist nur, was gebraucht wird.
//     BHED_D3D12_AUSLAGERN=budget: nur, wenn der Prozess mehr belegt, als ihm das System zugesteht; =0: nie.
//   - D3D11_USAGE_STAGING (Texturen zum Lesen): ein Lesepuffer; Map(READ) wartet auf die Grafikkarte.
//   - Sichten auf Texturen haben einen festen Deskriptor im gemeinsamen Stapel (den auch die Oberflaeche nutzt).
//   - Zustandsobjekte sind Beschreibungen mit Kennung; gleiche Beschreibung -> dasselbe Objekt (wie Direct3D 11).

#include <windows.h>

#include "abgeber12_intern.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <set>
#include <unordered_set>

#include <dxgi1_4.h>

#include "bhed/diag.h"

namespace bhed::render::d3d12::abgeber {

std::atomic<std::uint64_t> g_lebend{0};
long long g_teilAbgabe = 0;
std::uint32_t g_naechsteKennung = 0;

namespace {

constexpr UINT kZielSichten = 2048;    // Renderziel-Sichten gleichzeitig
constexpr UINT kTiefeSichten = 512;
constexpr UINT kAbtaster = 2048;       // Obergrenze von Direct3D 12 fuer einen sichtbaren Abtaster-Stapel
constexpr UINT kMarken = 2048;         // Zeitmarken im Ring
constexpr UINT kKlassen = 9;           // 256 B << 0..8 = 256 B .. 64 KB
constexpr std::uint64_t kKleinsterBlock = 256;
constexpr std::uint64_t kSeite = 1ULL << 20U;
constexpr std::uint64_t kGrossSchritt = 64ULL << 10U;
constexpr std::uint64_t kRuheAb = 64ULL << 10U;   // ab dieser Groesse bekommt ein ruhender Eck-/Indexpuffer eine Kopie im Grafikspeicher
constexpr std::uint64_t kRuheBilder = 2;          // so viele Bilder ohne Map
constexpr std::uint64_t kDuennAb = 32ULL << 20U;  // leer angelegte Texturfelder ab dieser Groesse (und ab 4 Schichten) belegen je Schicht
constexpr UINT kDuennKacheln = 16;                 // ... wenn eine Schicht mindestens so viele Kacheln hat (1 MB): ein Heap je Schicht
constexpr std::uint64_t kAuslagernAb = 1ULL << 20U;   // Texturen ab dieser Groesse werden ausgelagert, wenn sie ...
constexpr std::uint64_t kAuslagernMs = 4000;          // ... so lange (Millisekunden) ...
constexpr std::uint32_t kBildRing = 8192;             // so viele Bilder zurueck kennt der Abgeber Zaunwert und Anfangszeit

template <typename T>
void frei(T*& p) {
    if (p != nullptr) {
        p->Release();
        p = nullptr;
    }
}

bool g_aktiv = false;
Geraet* g_geraet = nullptr;
Kontext* g_kontext = nullptr;
ID3D12RootSignature* g_wurzel = nullptr;
ID3D12DescriptorHeap* g_zielStapel = nullptr;
ID3D12DescriptorHeap* g_tiefeStapel = nullptr;
ID3D12DescriptorHeap* g_abtasterStapel = nullptr;
UINT g_zielSchritt = 0, g_tiefeSchritt = 0, g_abtasterSchritt = 0;
std::vector<UINT> g_zielFrei, g_tiefeFrei;
UINT g_zielNaechster = 0, g_tiefeNaechster = 0, g_abtasterNaechster = 0;
UINT g_leereSicht = 0;
D3D12_GPU_DESCRIPTOR_HANDLE g_vorgabeAbtaster{};
Block g_nullSeite;
std::unordered_set<std::uint64_t> g_sichten;   // lebende Sichten (Adresse), fuer die Oberflaeche
std::unordered_set<const Textur*> g_texturenLebend;   // fuer den Speicherbericht
std::unordered_set<const Puffer*> g_pufferLebend;
std::set<std::string> g_fehltGemeldet;
Zaehler g_zaehler;
DWORD g_faden = 0;   // der Faden der Grafik (alle Aufrufe kommen von ihm)
std::uint64_t g_bild = 0;   // zaehlt bildAnfang
bool g_blockHeapEigen = true;   // Bloecke im CUSTOM-Heap (false: der Treiber lehnt ihn ab - Hochlade-Heap)
bool g_duennGeht = false;       // reservierte Ressourcen mit leeren Kacheln, die 0 lesen (TiledResourcesTier >= 2)
int g_auslagern = 2;            // 2: nach Zeit (Vorgabe); BHED_D3D12_AUSLAGERN=0: nie, =budget: 1, nur ueber dem Budget des Systems
IDXGIAdapter3* g_karte3 = nullptr;      // fuer das Budget (QueryVideoMemoryInfo)
std::uint64_t g_budgetVorgabe = 0;      // BHED_D3D12_BUDGET_MB: dieses Budget annehmen (Selbsttest), 0: das des Systems
std::uint64_t g_budgetZuletzt = 0;      // bei der letzten Pruefung
std::uint64_t g_belegtZuletzt = 0;
struct BildStand {
    std::uint64_t zaun = 0;   // deckt alle Listen, die vor diesem Bild abgeschickt wurden
    std::uint64_t zeit = 0;   // GetTickCount64 am Anfang des Bildes
};
BildStand g_bildStand[kBildRing];   // je Bild (g_gebrauchBild % kBildRing)
std::uint64_t g_pruefZeit = 0;      // letzte Pruefung (auslagernPruefen)
std::uint64_t g_ausgelagertMal = 0;
std::uint64_t g_eingelagertMal = 0;
double g_eingelagertMs = 0.0;   // Summe der Zeit in MakeResident

// Zustandsobjekte nach Beschreibung
std::map<std::string, Mischung*> g_mischungen;
std::map<std::string, TiefeZustand*> g_tiefeZustaende;
std::map<std::string, Raster*> g_raster;
std::map<std::string, Abtaster*> g_abtaster;
std::map<std::string, UINT> g_abtasterPlatz;   // Deskriptor je Beschreibung, bleibt bis zum Abbau

// Hochladespeicher in Bloecken
std::vector<ID3D12Resource*> g_seiten;
std::vector<Block> g_freiKlein[kKlassen];
struct GrossFrei {
    std::vector<Block> bloecke;
    std::uint64_t genommen = 0;   // Bild, in dem zuletzt einer dieser Groesse genommen oder zurueckgegeben wurde
};
std::map<std::uint64_t, GrossFrei> g_freiGross;
constexpr std::uint64_t kGrossBilder = 600;   // so lange unbenutzt: zurueck an das System
std::vector<Block> g_offen;                                          // freigegeben seit dem letzten Abschicken
std::vector<std::vector<Block>> g_listenLeer;                        // geleerte Listen: ihr Platz wird wieder benutzt
std::deque<std::pair<std::uint64_t, std::vector<Block>>> g_wartend;  // Zaunwert, Bloecke

// Puffer im Grafikspeicher (D3D11_USAGE_DEFAULT/IMMUTABLE) wiederverwenden. Die Zeichenschicht legt Eck- und Indexpuffer
// mancher Netze alle paar Bilder neu an (gpumap zeichneKarte: im Karten-Editor 0,4 Puffer je Bild) - unter Direct3D 11 billig,
// unter Direct3D 12 je Puffer ein Kernaufruf (CreateCommittedResource). Ein freigegebener Puffer wartet auf die Grafikkarte
// wie ein Block und steht dann fuer ein neues CreateBuffer derselben Groesse (auf 64 KB gerundet - so gross ist die Zuteilung
// ohnehin) bereit; er steht dann wieder im Zustand COMMON. Der Vorrat haelt hoechstens kVorratBytes; was kVorratBilder Bilder
// lang niemand nimmt, geht zurueck.
struct VorratPuffer {
    ID3D12Resource* res = nullptr;
    std::uint64_t groesse = 0;
    std::uint64_t bild = 0;   // freigegeben in diesem Bild
};
constexpr std::uint64_t kVorratBytes = 64ULL << 20U;
constexpr std::uint64_t kVorratBilder = 600;
std::vector<VorratPuffer> g_vorrat;                                         // bereit
std::vector<VorratPuffer> g_vorratOffen;                                    // freigegeben seit dem letzten Abschicken
std::deque<std::pair<std::uint64_t, std::vector<VorratPuffer>>> g_vorratWartend;   // Zaunwert, Puffer
std::uint64_t g_vorratJetzt = 0;                                            // Bytes in allen drei Listen

// Zeitmarken
ID3D12QueryHeap* g_markenStapel = nullptr;
ID3D12Resource* g_markenPuffer = nullptr;
const std::uint64_t* g_markenWerte = nullptr;
UINT g_markeNaechste = 0;
std::vector<UINT> g_markenOffen;   // seit dem letzten Abschicken beendet

void fadenPruefen(const char* wo) {
    if (g_faden != 0 && GetCurrentThreadId() != g_faden) {
        static bool gemeldet = false;
        if (!gemeldet) {
            gemeldet = true;
            diag::error(std::string("D3D12-Abgeber: Aufruf von einem anderen Faden (") + wo + ") - der Abgeber ist fuer einen Faden gebaut");
        }
    }
}

// Speicher fuer die Bloecke der dynamischen Puffer: im Arbeitsspeicher, von der Grafikkarte lesbar. Fuer den Prozessor
// gewoehnlicher Speicher mit Zwischenspeicher (CUSTOM-Heap, L0, WRITE_BACK) - so, wie der D3D11-Treiber einen abgebildeten
// dynamischen Puffer herausgibt. Ein Hochlade-Heap ist schreibkombiniert: jedes Lesen daraus geht am Zwischenspeicher vorbei,
// und die Zeichenschichten lesen stellenweise zurueck (gpumap: g_bildDaten nach dem Fuellen von g_cbBild - im Profil 19,5 %
// des Hauptfadens im Karten-Editor, docs/intern/NOTIZEN_d3d12_1011.md). Lehnt ein Treiber den Heap ab: Hochlade-Heap.
ID3D12Resource* hochladePuffer(std::uint64_t groesse, unsigned char** cpu) {
    D3D12_RESOURCE_DESC rd{};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width = groesse;
    rd.Height = 1;
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;
    rd.SampleDesc.Count = 1;
    rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    // D3D12-2 gemessen (Reihe vc, 12.11.): Hochlade-Heap (schreibkombiniert) und GPU_UPLOAD (Grafikspeicher ueber den PCIe-Balken)
    // sind auf diesem Rechner langsamer (Karten-Editor duel_yavin Hauptfaden 2,49 -> 4,03 bzw. 10,5 ms).
    ID3D12Resource* r = nullptr;
    if (g_blockHeapEigen) {
        D3D12_HEAP_PROPERTIES hp{};
        hp.Type = D3D12_HEAP_TYPE_CUSTOM;
        hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
        hp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
        // COMMON: ein Puffer steht ohnehin dort, und Direct3D 12 stellt ihn bei jedem lesenden Gebrauch von selbst lesbar
        if (FAILED(rahmen::geraet()->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&r)))) {
            r = nullptr;
            g_blockHeapEigen = false;
            diag::detail("D3D12-Abgeber: Bloecke im Hochlade-Heap (der eigene Heap mit Zwischenspeicher wurde abgelehnt)");
        }
    }
    if (r == nullptr) {
        D3D12_HEAP_PROPERTIES hp{};
        hp.Type = D3D12_HEAP_TYPE_UPLOAD;
        if (FAILED(rahmen::geraet()->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&r)))) {
            return nullptr;
        }
    }
    ++g_zaehler.angelegt;
    const D3D12_RANGE nichts{0, 0};
    void* z = nullptr;
    if (FAILED(r->Map(0, &nichts, &z))) {
        r->Release();
        return nullptr;
    }
    *cpu = static_cast<unsigned char*>(z);
    return r;
}

UINT klasseFuer(std::uint64_t groesse) {
    UINT k = 0;
    while ((kKleinsterBlock << k) < groesse) { ++k; }
    return k;
}

void blockZurueck(const Block& b) {
    if (b.klasse < kKlassen) {
        g_freiKlein[b.klasse].push_back(b);
        return;
    }
    GrossFrei& gf = g_freiGross[b.groesse];
    if (gf.bloecke.size() >= 3U) {   // mehr als drei gleich grosse braucht kein Bildtakt - zurueck an das System
        g_zaehler.hochladeBytes -= b.groesse;
        b.res->Release();
        return;
    }
    gf.bloecke.push_back(b);
}

void bloeckeEinsammeln() {
    while (!g_vorratWartend.empty() && rahmen::fertig(g_vorratWartend.front().first)) {
        for (const VorratPuffer& v : g_vorratWartend.front().second) { g_vorrat.push_back(v); }
        g_vorratWartend.pop_front();
    }
    while (!g_wartend.empty() && rahmen::fertig(g_wartend.front().first)) {
        std::vector<Block>& liste_ = g_wartend.front().second;
        for (const Block& b : liste_) { blockZurueck(b); }
        liste_.clear();
        g_listenLeer.push_back(std::move(liste_));
        g_wartend.pop_front();
    }
}

// --- Beschreibungen angleichen und als Schluessel -----------------------------------

std::string alsSchluessel(const void* p, std::size_t n) { return std::string(static_cast<const char*>(p), n); }

D3D11_BLEND_DESC angeglichen(const D3D11_BLEND_DESC& e) {
    D3D11_BLEND_DESC a;
    std::memset(&a, 0, sizeof(a));
    a.AlphaToCoverageEnable = e.AlphaToCoverageEnable != FALSE;
    a.IndependentBlendEnable = e.IndependentBlendEnable != FALSE;
    for (UINT i = 0; i < 8U; ++i) {
        const D3D11_RENDER_TARGET_BLEND_DESC& q = e.RenderTarget[a.IndependentBlendEnable != FALSE ? i : 0U];
        D3D11_RENDER_TARGET_BLEND_DESC& z = a.RenderTarget[i];
        z.BlendEnable = q.BlendEnable != FALSE;
        if (z.BlendEnable != FALSE) {
            z.SrcBlend = q.SrcBlend;
            z.DestBlend = q.DestBlend;
            z.BlendOp = q.BlendOp;
            z.SrcBlendAlpha = q.SrcBlendAlpha;
            z.DestBlendAlpha = q.DestBlendAlpha;
            z.BlendOpAlpha = q.BlendOpAlpha;
        } else {   // ohne Mischen zaehlen die Faktoren nicht: die Vorgabe
            z.SrcBlend = z.SrcBlendAlpha = D3D11_BLEND_ONE;
            z.DestBlend = z.DestBlendAlpha = D3D11_BLEND_ZERO;
            z.BlendOp = z.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        }
        z.RenderTargetWriteMask = q.RenderTargetWriteMask;
    }
    return a;
}

D3D11_DEPTH_STENCIL_DESC angeglichen(const D3D11_DEPTH_STENCIL_DESC& e) {
    D3D11_DEPTH_STENCIL_DESC a;
    std::memset(&a, 0, sizeof(a));
    a.DepthEnable = e.DepthEnable != FALSE;
    a.DepthWriteMask = e.DepthWriteMask;
    a.DepthFunc = e.DepthFunc;
    a.StencilEnable = e.StencilEnable != FALSE;
    const D3D11_DEPTH_STENCILOP_DESC vorgabe{D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS};
    if (a.StencilEnable != FALSE) {
        a.StencilReadMask = e.StencilReadMask;
        a.StencilWriteMask = e.StencilWriteMask;
        a.FrontFace = e.FrontFace;
        a.BackFace = e.BackFace;
    } else {
        a.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
        a.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
        a.FrontFace = vorgabe;
        a.BackFace = vorgabe;
    }
    return a;
}

D3D11_RASTERIZER_DESC angeglichen(const D3D11_RASTERIZER_DESC& e) {
    D3D11_RASTERIZER_DESC a;
    std::memset(&a, 0, sizeof(a));
    a.FillMode = e.FillMode;
    a.CullMode = e.CullMode;
    a.FrontCounterClockwise = e.FrontCounterClockwise != FALSE;
    a.DepthBias = e.DepthBias;
    a.DepthBiasClamp = e.DepthBiasClamp;
    a.SlopeScaledDepthBias = e.SlopeScaledDepthBias;
    a.DepthClipEnable = e.DepthClipEnable != FALSE;
    a.ScissorEnable = e.ScissorEnable != FALSE;
    a.MultisampleEnable = e.MultisampleEnable != FALSE;
    a.AntialiasedLineEnable = e.AntialiasedLineEnable != FALSE;
    return a;
}

D3D11_SAMPLER_DESC angeglichen(const D3D11_SAMPLER_DESC& e) {
    D3D11_SAMPLER_DESC a;
    std::memset(&a, 0, sizeof(a));
    a.Filter = e.Filter;
    a.AddressU = e.AddressU;
    a.AddressV = e.AddressV;
    a.AddressW = e.AddressW;
    a.MipLODBias = e.MipLODBias;
    a.MaxAnisotropy = e.MaxAnisotropy;
    a.ComparisonFunc = e.ComparisonFunc;
    for (int i = 0; i < 4; ++i) { a.BorderColor[i] = e.BorderColor[i]; }
    a.MinLOD = e.MinLOD;
    a.MaxLOD = e.MaxLOD;
    return a;
}

D3D12_SAMPLER_DESC abtaster12(const D3D11_SAMPLER_DESC& e) {
    D3D12_SAMPLER_DESC s{};
    s.Filter = static_cast<D3D12_FILTER>(e.Filter);   // dieselben Werte
    s.AddressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(e.AddressU);
    s.AddressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(e.AddressV);
    s.AddressW = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(e.AddressW);
    s.MipLODBias = e.MipLODBias;
    s.MaxAnisotropy = e.MaxAnisotropy;
    // Die Vergleichsfunktion zaehlt nur bei Vergleichsfiltern (Bit 0x80 des Filters); sonst nicht weitergeben.
    s.ComparisonFunc = (static_cast<UINT>(e.Filter) & 0x80U) != 0U ? static_cast<D3D12_COMPARISON_FUNC>(e.ComparisonFunc) : D3D12_COMPARISON_FUNC_NEVER;
    for (int i = 0; i < 4; ++i) { s.BorderColor[i] = e.BorderColor[i]; }
    s.MinLOD = e.MinLOD;
    s.MaxLOD = e.MaxLOD;
    return s;
}

UINT abtasterPlatzFuer(const D3D11_SAMPLER_DESC& angegl) {
    const std::string k = alsSchluessel(&angegl, sizeof(angegl));
    if (const auto it = g_abtasterPlatz.find(k); it != g_abtasterPlatz.end()) { return it->second; }
    if (g_abtasterNaechster >= kAbtaster) {
        diag::error("D3D12-Abgeber: mehr als 2048 verschiedene Abtaster");
        return 0;
    }
    const UINT i = g_abtasterNaechster++;
    D3D12_CPU_DESCRIPTOR_HANDLE h = g_abtasterStapel->GetCPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<SIZE_T>(i) * g_abtasterSchritt;
    const D3D12_SAMPLER_DESC s = abtaster12(angegl);
    rahmen::geraet()->CreateSampler(&s, h);
    g_abtasterPlatz[k] = i;
    return i;
}

D3D12_GPU_DESCRIPTOR_HANDLE abtasterGpu(UINT i) {
    D3D12_GPU_DESCRIPTOR_HANDLE h = g_abtasterStapel->GetGPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<UINT64>(i) * g_abtasterSchritt;
    return h;
}

// --- Formate ---------------------------------------------------------------------

UINT elementBytes(DXGI_FORMAT f) {
    switch (f) {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT: return 16;
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT: return 12;
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT: return 8;
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UINT: return 4;
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R8G8_UNORM: return 2;
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT: return 1;
    default: return 0;
    }
}

// Das Tiefenformat, mit dem eine Textur dieses (auch typlosen) Formats geloescht wird; UNKNOWN: keins.
DXGI_FORMAT tiefeFormatZu(DXGI_FORMAT f) {
    switch (f) {
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_D32_FLOAT;
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_D16_UNORM: return DXGI_FORMAT_D16_UNORM;
    default: return DXGI_FORMAT_UNKNOWN;
    }
}

bool istTyplos(DXGI_FORMAT f) {
    switch (f) {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS: return true;
    default: return false;
    }
}

D3D12_RESOURCE_DESC texturDesc12(const D3D11_TEXTURE2D_DESC& d) {
    D3D12_RESOURCE_DESC rd{};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rd.Width = d.Width;
    rd.Height = d.Height;
    rd.DepthOrArraySize = static_cast<UINT16>(d.ArraySize);
    rd.MipLevels = static_cast<UINT16>(d.MipLevels);
    rd.Format = d.Format;
    rd.SampleDesc = d.SampleDesc;
    rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    if ((d.BindFlags & D3D11_BIND_RENDER_TARGET) != 0U) { rd.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; }
    if ((d.BindFlags & D3D11_BIND_DEPTH_STENCIL) != 0U) {
        rd.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        if ((d.BindFlags & D3D11_BIND_SHADER_RESOURCE) == 0U) { rd.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE; }
    }
    return rd;
}

// Die Anfangsdaten einer Textur (alle Teile) ueber den Hochladespeicher in die Vor-Liste. Die Textur steht in COPY_DEST.
bool texturFuellen(Textur* t, const D3D11_SUBRESOURCE_DATA* daten) {
    const D3D12_RESOURCE_DESC rd = t->res->GetDesc();
    const UINT n = t->teile();
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> fp(n);
    std::vector<UINT> zeilen(n);
    std::vector<UINT64> zeilenBytes(n);
    UINT64 gesamt = 0;
    rahmen::geraet()->GetCopyableFootprints(&rd, 0, n, 0, fp.data(), zeilen.data(), zeilenBytes.data(), &gesamt);
    ID3D12Resource* puffer = nullptr;
    std::uint64_t versatz = 0;
    unsigned char* ziel = nullptr;
    if (!rahmen::hochladePlatz(gesamt, true, puffer, versatz, ziel)) { return false; }
    ID3D12GraphicsCommandList* vl = rahmen::vorListe();
    for (UINT i = 0; i < n; ++i) {
        const auto* quelle = static_cast<const unsigned char*>(daten[i].pSysMem);
        if (quelle == nullptr) { continue; }
        for (UINT y = 0; y < zeilen[i]; ++y) {
            std::memcpy(ziel + fp[i].Offset + static_cast<std::size_t>(y) * fp[i].Footprint.RowPitch,
                        quelle + static_cast<std::size_t>(y) * daten[i].SysMemPitch, static_cast<std::size_t>(zeilenBytes[i]));
        }
        D3D12_TEXTURE_COPY_LOCATION q{};
        q.pResource = puffer;
        q.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        q.PlacedFootprint = fp[i];
        q.PlacedFootprint.Offset += versatz;
        D3D12_TEXTURE_COPY_LOCATION z{};
        z.pResource = t->res;
        z.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        z.SubresourceIndex = i;
        vl->CopyTextureRegion(&z, 0, 0, 0, &q, nullptr);
    }
    return true;
}

void barriere(ID3D12GraphicsCommandList* cl, ID3D12Resource* r, D3D12_RESOURCE_STATES vor, D3D12_RESOURCE_STATES nach) {
    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = r;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = vor;
    b.Transition.StateAfter = nach;
    cl->ResourceBarrier(1, &b);
    ++g_zaehler.barrieren;
}

UINT zielSichtNehmen() {
    if (!g_zielFrei.empty()) {
        const UINT i = g_zielFrei.back();
        g_zielFrei.pop_back();
        return i;
    }
    return g_zielNaechster < kZielSichten ? g_zielNaechster++ : kZielSichten;
}

UINT tiefeSichtNehmen() {
    if (!g_tiefeFrei.empty()) {
        const UINT i = g_tiefeFrei.back();
        g_tiefeFrei.pop_back();
        return i;
    }
    return g_tiefeNaechster < kTiefeSichten ? g_tiefeNaechster++ : kTiefeSichten;
}

D3D12_CPU_DESCRIPTOR_HANDLE zielGriff(UINT i) {
    D3D12_CPU_DESCRIPTOR_HANDLE h = g_zielStapel->GetCPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<SIZE_T>(i) * g_zielSchritt;
    return h;
}

D3D12_CPU_DESCRIPTOR_HANDLE tiefeGriff(UINT i) {
    D3D12_CPU_DESCRIPTOR_HANDLE h = g_tiefeStapel->GetCPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<SIZE_T>(i) * g_tiefeSchritt;
    return h;
}

// Die D3D12-Beschreibung einer Sicht aus der von Direct3D 11 (dieselben Felder). false: Art nicht umgesetzt.
bool sicht12(const D3D11_SHADER_RESOURCE_VIEW_DESC& e, D3D12_SHADER_RESOURCE_VIEW_DESC& s) {
    s = D3D12_SHADER_RESOURCE_VIEW_DESC{};
    s.Format = e.Format;
    s.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    switch (e.ViewDimension) {
    case D3D11_SRV_DIMENSION_TEXTURE2D:
        s.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        s.Texture2D.MostDetailedMip = e.Texture2D.MostDetailedMip;
        s.Texture2D.MipLevels = e.Texture2D.MipLevels;
        return true;
    case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
        s.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        s.Texture2DArray.MostDetailedMip = e.Texture2DArray.MostDetailedMip;
        s.Texture2DArray.MipLevels = e.Texture2DArray.MipLevels;
        s.Texture2DArray.FirstArraySlice = e.Texture2DArray.FirstArraySlice;
        s.Texture2DArray.ArraySize = e.Texture2DArray.ArraySize;
        return true;
    case D3D11_SRV_DIMENSION_TEXTURE2DMS:
        s.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        return true;
    case D3D11_SRV_DIMENSION_TEXTURECUBE:
        s.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        s.TextureCube.MostDetailedMip = e.TextureCube.MostDetailedMip;
        s.TextureCube.MipLevels = e.TextureCube.MipLevels;
        return true;
    case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:
        s.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        s.TextureCubeArray.MostDetailedMip = e.TextureCubeArray.MostDetailedMip;
        s.TextureCubeArray.MipLevels = e.TextureCubeArray.MipLevels;
        s.TextureCubeArray.First2DArrayFace = e.TextureCubeArray.First2DArrayFace;
        s.TextureCubeArray.NumCubes = e.TextureCubeArray.NumCubes;
        return true;
    case D3D11_SRV_DIMENSION_BUFFER:
        s.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        s.Buffer.FirstElement = e.Buffer.FirstElement;
        s.Buffer.NumElements = e.Buffer.NumElements;
        return true;
    default: return false;
    }
}

// IDXGIDevice des Geraets: die Zeichenschichten fragen darueber die Grafikkarte (Name, Speicher).
struct DxgiGeraet final : IDXGIDevice {
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** aus) override {
        if (aus == nullptr) { return E_POINTER; }
        if (iid == __uuidof(IUnknown) || iid == __uuidof(IDXGIObject) || iid == __uuidof(IDXGIDevice)) {
            *aus = static_cast<IDXGIDevice*>(this);
            return S_OK;
        }
        *aus = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return 2; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID, UINT, const void*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID, const IUnknown*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID, UINT*, void*) override { return DXGI_ERROR_NOT_FOUND; }
    HRESULT STDMETHODCALLTYPE GetParent(REFIID iid, void** aus) override {
        IDXGIAdapter* a = nullptr;
        const HRESULT hr = GetAdapter(&a);
        if (FAILED(hr)) { return hr; }
        const HRESULT hq = a->QueryInterface(iid, aus);
        a->Release();
        return hq;
    }
    HRESULT STDMETHODCALLTYPE GetAdapter(IDXGIAdapter** aus) override {
        if (aus == nullptr) { return E_POINTER; }
        *aus = nullptr;
        IDXGIFactory4* fabrik = nullptr;
        if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&fabrik))) || fabrik == nullptr) { return E_FAIL; }
        const HRESULT hr = fabrik->EnumAdapterByLuid(rahmen::geraet()->GetAdapterLuid(), IID_PPV_ARGS(aus));
        fabrik->Release();
        return hr;
    }
    HRESULT STDMETHODCALLTYPE CreateSurface(const DXGI_SURFACE_DESC*, UINT, DXGI_USAGE, const DXGI_SHARED_RESOURCE*, IDXGISurface**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE QueryResourceResidency(IUnknown* const*, DXGI_RESIDENCY*, UINT) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetGPUThreadPriority(INT) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE GetGPUThreadPriority(INT* p) override {
        if (p != nullptr) { *p = 0; }
        return S_OK;
    }
};
DxgiGeraet g_dxgiGeraet;

ID3D12RootSignature* wurzelAnlegen() {
    D3D12_ROOT_PARAMETER p[kWurzelAbtaster + kAbtasterPlaetze] = {};
    D3D12_DESCRIPTOR_RANGE bereich[kSrvPlaetze + kAbtasterPlaetze] = {};
    for (UINT i = 0; i < kCbPlaetze; ++i) {
        p[kWurzelCbVs + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        p[kWurzelCbVs + i].Descriptor.ShaderRegister = i;
        p[kWurzelCbVs + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        p[kWurzelCbPs + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        p[kWurzelCbPs + i].Descriptor.ShaderRegister = i;
        p[kWurzelCbPs + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }
    for (UINT i = 0; i < kGsCbPlaetze; ++i) {
        p[kWurzelCbGs + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        p[kWurzelCbGs + i].Descriptor.ShaderRegister = kGsCbRegister[i];
        p[kWurzelCbGs + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
    }
    for (UINT i = 0; i < kSrvPlaetze; ++i) {
        bereich[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        bereich[i].NumDescriptors = 1;
        bereich[i].BaseShaderRegister = i;
        p[kWurzelSrv + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        p[kWurzelSrv + i].DescriptorTable.NumDescriptorRanges = 1;
        p[kWurzelSrv + i].DescriptorTable.pDescriptorRanges = &bereich[i];
        p[kWurzelSrv + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }
    for (UINT i = 0; i < kAbtasterPlaetze; ++i) {
        D3D12_DESCRIPTOR_RANGE& b = bereich[kSrvPlaetze + i];
        b.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        b.NumDescriptors = 1;
        b.BaseShaderRegister = i;
        p[kWurzelAbtaster + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        p[kWurzelAbtaster + i].DescriptorTable.NumDescriptorRanges = 1;
        p[kWurzelAbtaster + i].DescriptorTable.pDescriptorRanges = &b;
        p[kWurzelAbtaster + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }
    D3D12_ROOT_SIGNATURE_DESC d{};
    d.NumParameters = kWurzelAbtaster + kAbtasterPlaetze;
    d.pParameters = p;
    d.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
              D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
    ID3DBlob* blob = nullptr;
    ID3DBlob* fehler = nullptr;
    if (FAILED(D3D12SerializeRootSignature(&d, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &fehler)) || blob == nullptr) {
        if (fehler != nullptr) {
            diag::error(std::string("D3D12-Abgeber: Wurzelsignatur: ") + static_cast<const char*>(fehler->GetBufferPointer()));
            fehler->Release();
        }
        return nullptr;
    }
    ID3D12RootSignature* rs = nullptr;
    rahmen::geraet()->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rs));
    blob->Release();
    return rs;
}

ID3D12DescriptorHeap* stapel(D3D12_DESCRIPTOR_HEAP_TYPE typ, UINT n, bool sichtbar) {
    D3D12_DESCRIPTOR_HEAP_DESC hd{};
    hd.Type = typ;
    hd.NumDescriptors = n;
    hd.Flags = sichtbar ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ID3D12DescriptorHeap* h = nullptr;
    if (FAILED(rahmen::geraet()->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&h)))) { return nullptr; }
    return h;
}

}  // namespace

// ===================================================================================

HRESULT fehlt(const char* methode) {
    ++g_zaehler.fehlt;
    if (g_fehltGemeldet.insert(methode).second) { diag::error(std::string("D3D12-Abgeber: nicht umgesetzt: ") + methode); }
    return E_NOTIMPL;
}

ID3D11Device* geraetZeiger() { return g_geraet; }
Kontext* kontextZeiger() { return g_kontext; }
ID3D12RootSignature* wurzelSignatur() { return g_wurzel; }
ID3D12DescriptorHeap* abtasterStapel() { return g_abtasterStapel; }
D3D12_GPU_DESCRIPTOR_HANDLE leereSicht() { return rahmen::srvGpu(g_leereSicht); }
D3D12_GPU_DESCRIPTOR_HANDLE vorgabeAbtaster() { return g_vorgabeAbtaster; }
std::uint64_t nullSeite() { return g_nullSeite.va; }
void zaehleBarriere() { ++g_zaehler.barrieren; }
ID3D12QueryHeap* markenStapel() { return g_markenStapel; }

Block blockNehmen(std::uint64_t groesse) {
    if (groesse == 0) { groesse = 1; }
    Block b;
    if (groesse <= (kKleinsterBlock << (kKlassen - 1U))) {
        const UINT k = klasseFuer(groesse);
        std::vector<Block>& frei_ = g_freiKlein[k];
        if (frei_.empty()) {
            unsigned char* cpu = nullptr;
            ID3D12Resource* seite = hochladePuffer(kSeite, &cpu);
            if (seite == nullptr) { return b; }
            g_seiten.push_back(seite);
            g_zaehler.hochladeBytes += kSeite;
            const std::uint64_t va = seite->GetGPUVirtualAddress();
            const std::uint64_t schritt = kKleinsterBlock << k;
            for (std::uint64_t o = kSeite; o >= schritt; o -= schritt) {   // rueckwaerts: der erste Block liegt vorn
                Block n;
                n.res = seite;
                n.versatz = o - schritt;
                n.va = va + n.versatz;
                n.cpu = cpu + n.versatz;
                n.groesse = schritt;
                n.klasse = k;
                frei_.push_back(n);
            }
        }
        b = frei_.back();
        frei_.pop_back();
        return b;
    }
    const std::uint64_t g = (groesse + kGrossSchritt - 1U) / kGrossSchritt * kGrossSchritt;
    GrossFrei& gf = g_freiGross[g];
    gf.genommen = g_bild;   // auch wenn gleich ein neuer angelegt wird: diese Groesse ist gefragt
    if (!gf.bloecke.empty()) {
        b = gf.bloecke.back();
        gf.bloecke.pop_back();
        return b;
    }
    unsigned char* cpu = nullptr;
    b.res = hochladePuffer(g, &cpu);
    if (b.res == nullptr) { return b; }
    g_zaehler.hochladeBytes += g;
    b.versatz = 0;
    b.va = b.res->GetGPUVirtualAddress();
    b.cpu = cpu;
    b.groesse = g;
    return b;
}

Block blockWieNehmen(const Block& vorlage) {
    if (vorlage.klasse < kKlassen && !g_freiKlein[vorlage.klasse].empty()) {
        const Block b = g_freiKlein[vorlage.klasse].back();
        g_freiKlein[vorlage.klasse].pop_back();
        return b;
    }
    return blockNehmen(vorlage.groesse);
}

void blockSpaeterFrei(const Block& b) {
    if (!g_aktiv || b.res == nullptr) { return; }
    g_offen.push_back(b);
}

std::uint32_t g_gebrauchBild = 0;

void einlagern(Textur* t) {
    t->ausgelagert = false;
    if (t->res == nullptr) { return; }
    HRESULT hr = S_OK;
    LARGE_INTEGER vorher{};
    QueryPerformanceCounter(&vorher);
    if (t->schichtHeap.empty()) {
        ID3D12Pageable* einer = t->res;
        hr = rahmen::geraet()->MakeResident(1, &einer);
    } else {
        std::vector<ID3D12Pageable*> heaps;
        for (ID3D12Heap* h : t->schichtHeap) {
            if (h != nullptr) { heaps.push_back(h); }
        }
        if (!heaps.empty()) { hr = rahmen::geraet()->MakeResident(static_cast<UINT>(heaps.size()), heaps.data()); }
    }
    ++g_eingelagertMal;
    LARGE_INTEGER nachher{};
    LARGE_INTEGER takt{};
    QueryPerformanceCounter(&nachher);
    QueryPerformanceFrequency(&takt);
    const double ms = takt.QuadPart > 0 ? static_cast<double>(nachher.QuadPart - vorher.QuadPart) * 1000.0 / static_cast<double>(takt.QuadPart) : 0.0;
    g_eingelagertMs += ms;
    if (ms >= 2.0) {   // das Zurueckholen haelt den Hauptfaden an - was laenger dauert, steht im Protokoll
        char z[160];
        std::snprintf(z, sizeof(z), "D3D12-Abgeber: Textur %ux%u zurueckgeholt in %.1f ms", t->art.Width, t->art.Height, ms);
        diag::detail(z);
    }
    if (FAILED(hr)) {
        static bool gemeldet = false;
        if (!gemeldet) {
            gemeldet = true;
            diag::error("D3D12-Abgeber: kein Grafikspeicher, um eine ausgelagerte Textur zurueckzuholen (MakeResident)");
        }
    }
}

void wechsle(ID3D12GraphicsCommandList* cl, Textur* t, D3D12_RESOURCE_STATES nach) {
    gebraucht(t);
    if (t->zustand == nach) { return; }
    if (protokoll::g_an) {
        protokoll::eintrag(cl, "Wechsel " + protokoll::nenne(t) + ": " + protokoll::zustandName(t->zustand) + " -> " + protokoll::zustandName(nach));
    }
    barriere(cl, t->res, t->zustand, nach);
    t->zustand = nach;
}

void wechsle(ID3D12GraphicsCommandList* cl, const Wechsel* w, UINT n) {
    D3D12_RESOURCE_BARRIER b[kSrvPlaetze];
    UINT m = 0;
    for (UINT i = 0; i < n && m < kSrvPlaetze; ++i) {
        Textur* t = w[i].t;
        if (t == nullptr) { continue; }
        gebraucht(t);
        if (t->zustand == w[i].nach) { continue; }
        if (protokoll::g_an) {
            protokoll::eintrag(cl, "Wechsel " + protokoll::nenne(t) + ": " + protokoll::zustandName(t->zustand) + " -> " + protokoll::zustandName(w[i].nach));
        }
        b[m] = D3D12_RESOURCE_BARRIER{};
        b[m].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b[m].Transition.pResource = t->res;
        b[m].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b[m].Transition.StateBefore = t->zustand;
        b[m].Transition.StateAfter = w[i].nach;
        t->zustand = w[i].nach;   // dieselbe Textur ein zweites Mal in der Liste: von hier weiter
        ++m;
        ++g_zaehler.barrieren;
    }
    if (m != 0U) { cl->ResourceBarrier(m, b); }
}

void pufferLesbar(ID3D12GraphicsCommandList* cl, Puffer* p) {
    const std::uint64_t jetzt = rahmen::ausfuehrung();
    if (p->serie == jetzt) { return; }
    if (p->ausdruecklich) { barriere(cl, p->res, D3D12_RESOURCE_STATE_COMMON, p->lesen); }
    p->serie = jetzt;
}

void pufferRuhe(ID3D12GraphicsCommandList* cl, Puffer* p) {
    if (p->ruheGilt || p->art.ByteWidth < kRuheAb || g_bild - p->geschriebenBild < kRuheBilder ||
        (p->art.BindFlags & (D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER)) == 0U) {
        return;
    }
    if (p->ruhe == nullptr) {
        D3D12_HEAP_PROPERTIES hp{};
        hp.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC rd{};
        rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width = p->art.ByteWidth;
        rd.Height = 1;
        rd.DepthOrArraySize = 1;
        rd.MipLevels = 1;
        rd.SampleDesc.Count = 1;
        rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if (FAILED(rahmen::geraet()->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&p->ruhe)))) {
            p->ruhe = nullptr;
            return;   // dann eben weiter aus dem Block
        }
        ++g_zaehler.angelegt;
    }
    // Die Kopie steht in COMMON (seit dem letzten Map ist sie in keiner Liste gebraucht worden, und jede Ausfuehrung liegt
    // mindestens kRuheBilder Bilder zurueck): das Kopieren stellt sie von selbst auf COPY_DEST, danach ausdruecklich lesbar.
    if (protokoll::g_an) { protokoll::eintrag(cl, "Ruhender Puffer in den Grafikspeicher: " + std::to_string(p->art.ByteWidth) + " Bytes"); }
    cl->CopyBufferRegion(p->ruhe, 0, p->block.res, p->block.versatz, p->art.ByteWidth);
    barriere(cl, p->ruhe, D3D12_RESOURCE_STATE_COPY_DEST, p->lesen);
    p->va = p->ruhe->GetGPUVirtualAddress();
    p->ruheGilt = true;
    ++g_zaehler.ruhend;
}

bool pufferGeweckt(Puffer* p) {
    p->geschriebenBild = g_bild;
    if (!p->ruheGilt) { return false; }
    p->ruheGilt = false;
    p->va = p->block.va;
    return true;
}

void sichtMelden(Sicht* s, bool lebt) {
    const auto k = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(static_cast<ID3D11ShaderResourceView*>(s)));
    if (lebt) {
        g_sichten.insert(k);
    } else {
        g_sichten.erase(k);
    }
}

void sichtAufPufferErneuern(Sicht* s) {
    Puffer* p = s->puffer;
    if (p == nullptr) { return; }
    const UINT eb = elementBytes(s->art12.Format);
    D3D12_SHADER_RESOURCE_VIEW_DESC d = s->art12;
    ID3D12Resource* r = p->res;
    if (p->dynamisch) {
        r = p->block.res;
        d.Buffer.FirstElement += eb != 0U ? p->block.versatz / eb : 0U;
        if (s->fassung != p->fassung || s->index == rahmen::srvKeiner()) {
            if (s->index != rahmen::srvKeiner()) { rahmen::srvSpaeterFrei(s->index); }
            s->index = rahmen::srvNehmen();
        }
    }
    s->fassung = p->fassung;
    if (s->index != rahmen::srvKeiner() && r != nullptr) { rahmen::geraet()->CreateShaderResourceView(r, &d, rahmen::srvCpu(s->index)); }
}

int markeNehmen() {
    if (g_markenStapel == nullptr) { return -1; }
    const UINT i = g_markeNaechste;
    g_markeNaechste = (g_markeNaechste + 1U) % kMarken;
    g_markenOffen.push_back(i);
    return static_cast<int>(i);
}

bool markeLesen(int platz, std::uint64_t& wert) {
    if (platz < 0 || g_markenWerte == nullptr) { return false; }
    wert = g_markenWerte[platz];
    return true;
}

// --- Abbau der Objekte -----------------------------------------------------------

Puffer::~Puffer() {
    g_pufferLebend.erase(this);
    if (dynamisch) {
        blockSpaeterFrei(block);
        if (ruhe != nullptr) { rahmen::spaeterFrei(ruhe); }
    } else if (res != nullptr) {
        const std::uint64_t gross = (static_cast<std::uint64_t>(art.ByteWidth) + kGrossSchritt - 1U) / kGrossSchritt * kGrossSchritt;
        if (g_aktiv && g_vorratJetzt + gross <= kVorratBytes) {
            g_vorratOffen.push_back(VorratPuffer{res, gross, g_bild});
            g_vorratJetzt += gross;
        } else {
            rahmen::spaeterFrei(res);
        }
    }
}

Textur::~Textur() {
    g_texturenLebend.erase(this);
    if (res != nullptr) { rahmen::spaeterFrei(res); }
    for (ID3D12Heap* h : schichtHeap) {
        if (h != nullptr) { rahmen::spaeterFrei(h); }   // nach der Ressource: die Liste gibt in dieser Reihenfolge frei
    }
}

void schichtBelegen(Textur* t, UINT erste, UINT anzahl) {
    if (t->schichtHeap.empty()) { return; }
    if (t->ausgelagert) { einlagern(t); }   // erst alle Heaps zurueck: ein neuer waere sonst als einziger resident
    const UINT ende = anzahl == ~0U ? static_cast<UINT>(t->schichtHeap.size()) : std::min(erste + anzahl, static_cast<UINT>(t->schichtHeap.size()));
    for (UINT i = erste; i < ende; ++i) {
        if (t->schichtHeap[i] != nullptr) { continue; }
        D3D12_HEAP_DESC hd{};
        hd.SizeInBytes = static_cast<UINT64>(t->schichtKacheln) * D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
        hd.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        hd.Flags = (t->art.BindFlags & D3D11_BIND_RENDER_TARGET) != 0U ? D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES : D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
        ID3D12Heap* heap = nullptr;
        if (FAILED(rahmen::geraet()->CreateHeap(&hd, IID_PPV_ARGS(&heap))) || heap == nullptr) {
            static bool gemeldet = false;
            if (!gemeldet) {
                gemeldet = true;
                diag::error("D3D12-Abgeber: kein Grafikspeicher fuer eine Schicht eines Texturfelds - sie bleibt leer");
            }
            return;
        }
        // Die Zuordnung laeuft ueber die Warteschlange und steht damit vor jeder Befehlsliste, die erst noch abgeschickt wird.
        const D3D12_TILED_RESOURCE_COORDINATE ort{0, 0, 0, i};
        D3D12_TILE_REGION_SIZE flaeche{};
        flaeche.NumTiles = t->schichtKacheln;
        const D3D12_TILE_RANGE_FLAGS art = D3D12_TILE_RANGE_FLAG_NONE;
        const UINT anfang = 0;
        const UINT kacheln = t->schichtKacheln;
        rahmen::schlange()->UpdateTileMappings(t->res, 1, &ort, &flaeche, heap, 1, &art, &anfang, &kacheln, D3D12_TILE_MAPPING_FLAG_NONE);
        t->schichtHeap[i] = heap;
        ++g_zaehler.angelegt;
    }
}

Sicht::~Sicht() {
    sichtMelden(this, false);
    if (g_aktiv && index != rahmen::srvKeiner()) { rahmen::srvSpaeterFrei(index); }
    if (g_aktiv && indexOben != ~0U && indexOben != index) { rahmen::srvSpaeterFrei(indexOben); }
    if (textur != nullptr) { textur->Release(); }
    if (puffer != nullptr) { puffer->Release(); }
}

void STDMETHODCALLTYPE Sicht::GetResource(ID3D11Resource** r) {
    if (textur != nullptr) {
        *r = textur;
    } else {
        *r = puffer;
    }
    if (*r != nullptr) { (*r)->AddRef(); }
}

ZielSicht::~ZielSicht() {
    if (g_aktiv && index < kZielSichten) { g_zielFrei.push_back(index); }
    if (textur != nullptr) { textur->Release(); }
}

void STDMETHODCALLTYPE ZielSicht::GetResource(ID3D11Resource** r) {
    *r = textur;
    if (*r != nullptr) { (*r)->AddRef(); }
}

TiefeSicht::~TiefeSicht() {
    if (g_aktiv && index < kTiefeSichten) { g_tiefeFrei.push_back(index); }
    if (textur != nullptr) { textur->Release(); }
}

void STDMETHODCALLTYPE TiefeSicht::GetResource(ID3D11Resource** r) {
    *r = textur;
    if (*r != nullptr) { (*r)->AddRef(); }
}

template <class T>
void ausVerzeichnis(std::map<std::string, T*>& v, const T* objekt) {
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (it->second == objekt) {
            v.erase(it);
            return;
        }
    }
}

Mischung::~Mischung() { ausVerzeichnis(g_mischungen, this); }
TiefeZustand::~TiefeZustand() { ausVerzeichnis(g_tiefeZustaende, this); }
Raster::~Raster() { ausVerzeichnis(g_raster, this); }
Abtaster::~Abtaster() { ausVerzeichnis(g_abtaster, this); }

// --- Geraet ----------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE Geraet::QueryInterface(REFIID iid, void** aus) {
    if (aus == nullptr) { return E_POINTER; }
    if (iid == __uuidof(IUnknown) || iid == __uuidof(ID3D11Device)) {
        *aus = static_cast<ID3D11Device*>(this);
        return S_OK;
    }
    if (iid == __uuidof(IDXGIDevice) || iid == __uuidof(IDXGIObject)) {
        *aus = static_cast<IDXGIDevice*>(&g_dxgiGeraet);
        return S_OK;
    }
    *aus = nullptr;
    return E_NOINTERFACE;   // auch ID3D11InfoQueue: die Debugschicht ist die von Direct3D 12 (backend_d3d12_win32.cpp)
}

HRESULT STDMETHODCALLTYPE Geraet::CreateBuffer(const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Buffer** ppBuffer) {
    fadenPruefen("CreateBuffer");
    if (pDesc == nullptr || pDesc->ByteWidth == 0U) { return E_INVALIDARG; }
    if (ppBuffer == nullptr) { return S_FALSE; }
    *ppBuffer = nullptr;
    auto* p = new Puffer;
    p->art = *pDesc;
    if ((pDesc->BindFlags & (D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_CONSTANT_BUFFER)) != 0U) { p->lesen |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER; }
    if ((pDesc->BindFlags & D3D11_BIND_INDEX_BUFFER) != 0U) { p->lesen |= D3D12_RESOURCE_STATE_INDEX_BUFFER; }
    if ((pDesc->BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0U) { p->lesen |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; }
    // mehr als eine Leseart: ausdruecklich wechseln (ein Bit: Direct3D 12 stellt von selbst genau diesen Zustand her)
    p->ausdruecklich = (static_cast<UINT>(p->lesen) & (static_cast<UINT>(p->lesen) - 1U)) != 0U;
    if (pDesc->Usage == D3D11_USAGE_DYNAMIC) {
        p->dynamisch = true;
        p->block = blockNehmen(pDesc->ByteWidth);
        if (p->block.res == nullptr) {
            p->Release();
            return E_OUTOFMEMORY;
        }
        p->va = p->block.va;
        p->geschriebenBild = g_bild;
        if (pInitialData != nullptr && pInitialData->pSysMem != nullptr) { std::memcpy(p->block.cpu, pInitialData->pSysMem, pDesc->ByteWidth); }
        g_pufferLebend.insert(p);
        *ppBuffer = p;
        return S_OK;
    }
    if (pDesc->Usage == D3D11_USAGE_STAGING) {
        p->Release();
        return fehlt("CreateBuffer(D3D11_USAGE_STAGING)");
    }
    // aus dem Vorrat (dieselbe gerundete Groesse), sonst neu - mit der gerundeten Groesse, damit er spaeter wieder passt
    const std::uint64_t gross = (static_cast<std::uint64_t>(pDesc->ByteWidth) + kGrossSchritt - 1U) / kGrossSchritt * kGrossSchritt;
    for (std::size_t i = 0; i < g_vorrat.size(); ++i) {
        if (g_vorrat[i].groesse == gross) {
            p->res = g_vorrat[i].res;
            g_vorrat[i] = g_vorrat.back();
            g_vorrat.pop_back();
            g_vorratJetzt -= gross;
            ++g_zaehler.ausVorrat;
            break;
        }
    }
    if (p->res == nullptr) {
        D3D12_HEAP_PROPERTIES hp{};
        hp.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC rd{};
        rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width = gross;
        rd.Height = 1;
        rd.DepthOrArraySize = 1;
        rd.MipLevels = 1;
        rd.SampleDesc.Count = 1;
        rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if (FAILED(rahmen::geraet()->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&p->res)))) {
            p->res = nullptr;
            p->Release();
            return E_OUTOFMEMORY;
        }
        ++g_zaehler.angelegt;
    }
    p->va = p->res->GetGPUVirtualAddress();
    if (pInitialData != nullptr && pInitialData->pSysMem != nullptr) {
        ID3D12Resource* puffer = nullptr;
        std::uint64_t versatz = 0;
        unsigned char* ziel = nullptr;
        if (!rahmen::hochladePlatz(pDesc->ByteWidth, true, puffer, versatz, ziel)) {
            p->Release();
            return E_OUTOFMEMORY;
        }
        std::memcpy(ziel, pInitialData->pSysMem, pDesc->ByteWidth);
        ID3D12GraphicsCommandList* vl = rahmen::vorListe();
        // COMMON wird durch das Kopieren von selbst zu COPY_DEST; danach ausdruecklich in den Lesezustand.
        vl->CopyBufferRegion(p->res, 0, puffer, versatz, pDesc->ByteWidth);
        barriere(vl, p->res, D3D12_RESOURCE_STATE_COPY_DEST, p->lesen);
        p->serie = rahmen::ausfuehrung();
    }
    g_pufferLebend.insert(p);
    *ppBuffer = p;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Geraet::CreateTexture2D(const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D) {
    fadenPruefen("CreateTexture2D");
    if (pDesc == nullptr || pDesc->Width == 0U || pDesc->Height == 0U || pDesc->ArraySize == 0U) { return E_INVALIDARG; }
    if (ppTexture2D == nullptr) { return S_FALSE; }
    *ppTexture2D = nullptr;
    auto* t = new Textur;
    t->art = *pDesc;
    if (t->art.MipLevels == 0U) {   // die ganze Pyramide
        UINT n = 1;
        for (UINT g = std::max(pDesc->Width, pDesc->Height); g > 1U; g >>= 1U) { ++n; }
        t->art.MipLevels = n;
    }
    D3D12_RESOURCE_DESC rd = texturDesc12(t->art);
    t->format12 = rd.Format;
    if (pDesc->Usage == D3D11_USAGE_STAGING) {
        if ((pDesc->CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) != 0U) {
            t->Release();
            return fehlt("CreateTexture2D(D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_WRITE)");
        }
        // Ein Lesepuffer in der Aufteilung, in der Direct3D 12 die Teile einer solchen Textur kopiert.
        t->ebenen = (rd.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT || rd.Format == DXGI_FORMAT_D24_UNORM_S8_UINT) ? 2U : 1U;
        const UINT n = t->teile() * t->ebenen;
        t->abdruck.resize(n);
        UINT64 gesamt = 0;
        rd.Flags = t->ebenen == 2U ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_NONE;
        rahmen::geraet()->GetCopyableFootprints(&rd, 0, n, 0, t->abdruck.data(), nullptr, nullptr, &gesamt);
        D3D12_HEAP_PROPERTIES hp{};
        hp.Type = D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC bd{};
        bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bd.Width = gesamt;
        bd.Height = 1;
        bd.DepthOrArraySize = 1;
        bd.MipLevels = 1;
        bd.SampleDesc.Count = 1;
        bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        void* z = nullptr;
        if (FAILED(rahmen::geraet()->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&t->res))) ||
            FAILED(t->res->Map(0, nullptr, &z))) {
            t->Release();
            return E_OUTOFMEMORY;
        }
        ++g_zaehler.angelegt;
        t->cpu = static_cast<unsigned char*>(z);
        t->lesepuffer = true;
        t->zustand = D3D12_RESOURCE_STATE_COPY_DEST;
        g_texturenLebend.insert(t);
        *ppTexture2D = t;
        return S_OK;
    }
    const bool ziel = (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET) != 0U;
    const bool tiefe_ = (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL) != 0U;
    const bool lesbar = (pDesc->BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0U;
    D3D12_RESOURCE_STATES anfang = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    if (pInitialData != nullptr) {
        anfang = D3D12_RESOURCE_STATE_COPY_DEST;
    } else if (tiefe_) {
        anfang = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    } else if (ziel) {
        anfang = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    D3D12_CLEAR_VALUE loeschen{};
    const D3D12_CLEAR_VALUE* lw = nullptr;
    // Der Loeschwert, den die Grafikkarte schnell loescht: Tiefe 1 und Stencil 0, Ziele mit 0, 0, 0, 0 - so loeschen die
    // Zeichenschichten fast ueberall; ein anderer Wert loescht ebenso richtig, nur auf dem allgemeinen Weg.
    if (tiefe_ && tiefeFormatZu(rd.Format) != DXGI_FORMAT_UNKNOWN) {
        loeschen.Format = tiefeFormatZu(rd.Format);
        loeschen.DepthStencil.Depth = 1.0F;
        lw = &loeschen;
    } else if (ziel && !tiefe_ && !istTyplos(rd.Format)) {
        loeschen.Format = rd.Format;
        lw = &loeschen;
    }
    // Ein grosses, leer angelegtes Texturfeld (der Atlas der Schattenwuerfel - er wird Schicht fuer Schicht durch Kopieren
    // gefuellt; ebenso Felder, die Ziel sind): reserviert, Speicher je Schicht beim ersten Schreiben.
    if (g_duennGeht && !tiefe_ && pInitialData == nullptr && pDesc->Usage == D3D11_USAGE_DEFAULT && rd.SampleDesc.Count == 1U && rd.MipLevels == 1U &&
        rd.DepthOrArraySize >= 4U && !istTyplos(rd.Format) && (pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) == 0U) {
        D3D12_RESOURCE_DESC duenn = rd;
        duenn.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
        if (rahmen::geraet()->GetResourceAllocationInfo(0, 1, &rd).SizeInBytes >= kDuennAb &&
            SUCCEEDED(rahmen::geraet()->CreateReservedResource(&duenn, anfang, lw, IID_PPV_ARGS(&t->res))) && t->res != nullptr) {
            UINT kachelnGesamt = 0;
            D3D12_PACKED_MIP_INFO gepackt{};
            D3D12_TILE_SHAPE form{};
            UINT teile = 1;
            D3D12_SUBRESOURCE_TILING teilung{};
            rahmen::geraet()->GetResourceTiling(t->res, &kachelnGesamt, &gepackt, &form, &teile, 0, &teilung);
            t->schichtKacheln = static_cast<UINT>(teilung.WidthInTiles) * teilung.HeightInTiles * teilung.DepthInTiles;
            if (teile == 1U && t->schichtKacheln >= kDuennKacheln && gepackt.NumPackedMips == 0U) {
                t->schichtHeap.assign(rd.DepthOrArraySize, nullptr);
            } else {   // nicht in Kacheln zu teilen: gewoehnlich anlegen
                t->res->Release();
                t->res = nullptr;
                t->schichtKacheln = 0;
            }
        }
    }
    D3D12_HEAP_PROPERTIES hp{};
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    if (t->res == nullptr && FAILED(rahmen::geraet()->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, anfang, lw, IID_PPV_ARGS(&t->res)))) {
        t->Release();
        return E_OUTOFMEMORY;
    }
    ++g_zaehler.angelegt;
    t->zustand = anfang;
    t->bytes = rahmen::geraet()->GetResourceAllocationInfo(0, 1, &rd).SizeInBytes;
    t->gebrauchtBild = g_gebrauchBild;
    if (pInitialData != nullptr) {
        if (!texturFuellen(t, pInitialData)) {
            t->Release();
            return E_OUTOFMEMORY;
        }
        const D3D12_RESOURCE_STATES danach = lesbar ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_COMMON;
        barriere(rahmen::vorListe(), t->res, D3D12_RESOURCE_STATE_COPY_DEST, danach);
        t->zustand = danach;
    }
    g_texturenLebend.insert(t);
    *ppTexture2D = t;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Geraet::CreateShaderResourceView(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, ID3D11ShaderResourceView** ppSRView) {
    fadenPruefen("CreateShaderResourceView");
    if (pResource == nullptr) { return E_INVALIDARG; }
    if (ppSRView == nullptr) { return S_FALSE; }
    *ppSRView = nullptr;
    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pResource->GetType(&dim);
    auto* s = new Sicht;
    s->index = rahmen::srvKeiner();
    if (dim == D3D11_RESOURCE_DIMENSION_BUFFER) {
        s->puffer = static_cast<Puffer*>(static_cast<ID3D11Buffer*>(pResource));
        s->puffer->AddRef();
        if (pDesc == nullptr || pDesc->ViewDimension != D3D11_SRV_DIMENSION_BUFFER || !sicht12(*pDesc, s->art12)) {
            s->Release();
            return fehlt("CreateShaderResourceView(Puffer ohne Beschreibung)");
        }
        s->art = *pDesc;
        if (!s->puffer->dynamisch) { s->index = rahmen::srvNehmen(); }
        s->fassung = s->puffer->fassung + 1U;   // erzwingt das Schreiben
        sichtAufPufferErneuern(s);
    } else {
        Textur* t = static_cast<Textur*>(static_cast<ID3D11Texture2D*>(pResource));
        s->textur = t;
        t->AddRef();
        if (pDesc != nullptr) {
            s->art = *pDesc;
        } else {   // die ganze Textur
            s->art.Format = t->art.Format;
            if ((t->art.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0U) {
                s->art.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                s->art.TextureCube.MipLevels = t->art.MipLevels;
            } else if (t->art.SampleDesc.Count > 1U) {
                s->art.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            } else if (t->art.ArraySize > 1U) {
                s->art.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                s->art.Texture2DArray.MipLevels = t->art.MipLevels;
                s->art.Texture2DArray.ArraySize = t->art.ArraySize;
            } else {
                s->art.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                s->art.Texture2D.MipLevels = t->art.MipLevels;
            }
        }
        if (s->art.Format == DXGI_FORMAT_UNKNOWN) { s->art.Format = t->art.Format; }
        if (t->lesepuffer || !sicht12(s->art, s->art12)) {
            s->Release();
            return fehlt("CreateShaderResourceView(Art)");
        }
        s->index = rahmen::srvNehmen();
        if (s->index == rahmen::srvKeiner()) {
            s->Release();
            diag::error("D3D12-Abgeber: der Deskriptorstapel der Texturen ist voll");
            return E_OUTOFMEMORY;
        }
        rahmen::geraet()->CreateShaderResourceView(t->res, &s->art12, rahmen::srvCpu(s->index));
    }
    sichtMelden(s, true);
    *ppSRView = s;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Geraet::CreateRenderTargetView(ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC* pDesc, ID3D11RenderTargetView** ppRTView) {
    fadenPruefen("CreateRenderTargetView");
    if (pResource == nullptr) { return E_INVALIDARG; }
    if (ppRTView == nullptr) { return S_FALSE; }
    *ppRTView = nullptr;
    Textur* t = static_cast<Textur*>(static_cast<ID3D11Texture2D*>(pResource));
    auto* s = new ZielSicht;
    s->textur = t;
    t->AddRef();
    s->index = zielSichtNehmen();
    if (s->index >= kZielSichten) {
        s->Release();
        diag::error("D3D12-Abgeber: zu viele Renderziel-Sichten");
        return E_OUTOFMEMORY;
    }
    s->griff = zielGriff(s->index);
    D3D12_RENDER_TARGET_VIEW_DESC d{};
    const D3D12_RENDER_TARGET_VIEW_DESC* pd = nullptr;
    if (pDesc != nullptr) {
        s->art = *pDesc;
        d.Format = pDesc->Format != DXGI_FORMAT_UNKNOWN ? pDesc->Format : t->art.Format;
        pd = &d;
        switch (pDesc->ViewDimension) {
        case D3D11_RTV_DIMENSION_TEXTURE2D:
            d.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            d.Texture2D.MipSlice = pDesc->Texture2D.MipSlice;
            break;
        case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
            d.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            d.Texture2DArray.MipSlice = pDesc->Texture2DArray.MipSlice;
            d.Texture2DArray.FirstArraySlice = pDesc->Texture2DArray.FirstArraySlice;
            d.Texture2DArray.ArraySize = pDesc->Texture2DArray.ArraySize;
            break;
        case D3D11_RTV_DIMENSION_TEXTURE2DMS: d.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS; break;
        default:
            s->Release();
            return fehlt("CreateRenderTargetView(Art)");
        }
        s->format = d.Format;
    } else {
        s->art.Format = t->art.Format;
        s->art.ViewDimension = t->art.SampleDesc.Count > 1U ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
        s->format = t->art.Format;
    }
    rahmen::geraet()->CreateRenderTargetView(t->res, pd, s->griff);
    *ppRTView = s;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Geraet::CreateDepthStencilView(ID3D11Resource* pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc, ID3D11DepthStencilView** ppDepthStencilView) {
    fadenPruefen("CreateDepthStencilView");
    if (pResource == nullptr) { return E_INVALIDARG; }
    if (ppDepthStencilView == nullptr) { return S_FALSE; }
    *ppDepthStencilView = nullptr;
    Textur* t = static_cast<Textur*>(static_cast<ID3D11Texture2D*>(pResource));
    auto* s = new TiefeSicht;
    s->textur = t;
    t->AddRef();
    s->index = tiefeSichtNehmen();
    if (s->index >= kTiefeSichten) {
        s->Release();
        diag::error("D3D12-Abgeber: zu viele Tiefen-Sichten");
        return E_OUTOFMEMORY;
    }
    s->griff = tiefeGriff(s->index);
    D3D12_DEPTH_STENCIL_VIEW_DESC d{};
    const D3D12_DEPTH_STENCIL_VIEW_DESC* pd = nullptr;
    if (pDesc != nullptr) {
        s->art = *pDesc;
        d.Format = pDesc->Format != DXGI_FORMAT_UNKNOWN ? pDesc->Format : t->art.Format;
        d.Flags = static_cast<D3D12_DSV_FLAGS>(pDesc->Flags);   // dieselben Werte (nur lesen: Tiefe 1, Stencil 2)
        pd = &d;
        switch (pDesc->ViewDimension) {
        case D3D11_DSV_DIMENSION_TEXTURE2D:
            d.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            d.Texture2D.MipSlice = pDesc->Texture2D.MipSlice;
            break;
        case D3D11_DSV_DIMENSION_TEXTURE2DARRAY:
            d.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            d.Texture2DArray.MipSlice = pDesc->Texture2DArray.MipSlice;
            d.Texture2DArray.FirstArraySlice = pDesc->Texture2DArray.FirstArraySlice;
            d.Texture2DArray.ArraySize = pDesc->Texture2DArray.ArraySize;
            break;
        case D3D11_DSV_DIMENSION_TEXTURE2DMS: d.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS; break;
        default:
            s->Release();
            return fehlt("CreateDepthStencilView(Art)");
        }
        s->format = d.Format;
    } else {
        s->art.Format = t->art.Format;
        s->art.ViewDimension = t->art.SampleDesc.Count > 1U ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
        s->format = t->art.Format;
    }
    rahmen::geraet()->CreateDepthStencilView(t->res, pd, s->griff);
    *ppDepthStencilView = s;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Geraet::CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs, UINT NumElements, const void*, SIZE_T, ID3D11InputLayout** ppInputLayout) {
    if (pInputElementDescs == nullptr && NumElements != 0U) { return E_INVALIDARG; }
    if (ppInputLayout == nullptr) { return S_FALSE; }
    auto* e = new Eingabe;
    e->namen.reserve(NumElements);
    e->elemente.resize(NumElements);
    for (UINT i = 0; i < NumElements; ++i) {
        const D3D11_INPUT_ELEMENT_DESC& q = pInputElementDescs[i];
        e->namen.emplace_back(q.SemanticName != nullptr ? q.SemanticName : "");
        D3D12_INPUT_ELEMENT_DESC& z = e->elemente[i];
        z.SemanticIndex = q.SemanticIndex;
        z.Format = q.Format;
        z.InputSlot = q.InputSlot;
        z.AlignedByteOffset = q.AlignedByteOffset;
        z.InputSlotClass = static_cast<D3D12_INPUT_CLASSIFICATION>(q.InputSlotClass);
        z.InstanceDataStepRate = q.InstanceDataStepRate;
    }
    for (UINT i = 0; i < NumElements; ++i) { e->elemente[i].SemanticName = e->namen[i].c_str(); }
    *ppInputLayout = e;
    return S_OK;
}

template <class S>
HRESULT shaderAnlegen(const void* code, SIZE_T laenge, S** aus) {
    if (code == nullptr || laenge == 0U) { return E_INVALIDARG; }
    if (aus == nullptr) { return S_FALSE; }
    auto* s = new Shader<S>;
    s->code.assign(static_cast<const unsigned char*>(code), static_cast<const unsigned char*>(code) + laenge);
    *aus = s;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Geraet::CreateVertexShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage*, ID3D11VertexShader** ppVertexShader) {
    return shaderAnlegen(pShaderBytecode, BytecodeLength, ppVertexShader);
}

HRESULT STDMETHODCALLTYPE Geraet::CreateGeometryShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage*, ID3D11GeometryShader** ppGeometryShader) {
    return shaderAnlegen(pShaderBytecode, BytecodeLength, ppGeometryShader);
}

HRESULT STDMETHODCALLTYPE Geraet::CreatePixelShader(const void* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage*, ID3D11PixelShader** ppPixelShader) {
    return shaderAnlegen(pShaderBytecode, BytecodeLength, ppPixelShader);
}

template <class T, class D>
HRESULT zustandAnlegen(std::map<std::string, T*>& verzeichnis, const D& angegl, T** neu) {
    const std::string k = alsSchluessel(&angegl, sizeof(angegl));
    if (const auto it = verzeichnis.find(k); it != verzeichnis.end()) {
        it->second->AddRef();
        *neu = it->second;
        return S_OK;
    }
    auto* z = new T;
    z->art = angegl;
    verzeichnis[k] = z;
    *neu = z;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Geraet::CreateBlendState(const D3D11_BLEND_DESC* pBlendStateDesc, ID3D11BlendState** ppBlendState) {
    if (pBlendStateDesc == nullptr) { return E_INVALIDARG; }
    if (ppBlendState == nullptr) { return S_FALSE; }
    Mischung* m = nullptr;
    const HRESULT hr = zustandAnlegen(g_mischungen, angeglichen(*pBlendStateDesc), &m);
    *ppBlendState = m;
    return hr;
}

HRESULT STDMETHODCALLTYPE Geraet::CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc, ID3D11DepthStencilState** ppDepthStencilState) {
    if (pDepthStencilDesc == nullptr) { return E_INVALIDARG; }
    if (ppDepthStencilState == nullptr) { return S_FALSE; }
    TiefeZustand* z = nullptr;
    const HRESULT hr = zustandAnlegen(g_tiefeZustaende, angeglichen(*pDepthStencilDesc), &z);
    *ppDepthStencilState = z;
    return hr;
}

HRESULT STDMETHODCALLTYPE Geraet::CreateRasterizerState(const D3D11_RASTERIZER_DESC* pRasterizerDesc, ID3D11RasterizerState** ppRasterizerState) {
    if (pRasterizerDesc == nullptr) { return E_INVALIDARG; }
    if (ppRasterizerState == nullptr) { return S_FALSE; }
    Raster* r = nullptr;
    const HRESULT hr = zustandAnlegen(g_raster, angeglichen(*pRasterizerDesc), &r);
    *ppRasterizerState = r;
    return hr;
}

HRESULT STDMETHODCALLTYPE Geraet::CreateSamplerState(const D3D11_SAMPLER_DESC* pSamplerDesc, ID3D11SamplerState** ppSamplerState) {
    if (pSamplerDesc == nullptr) { return E_INVALIDARG; }
    if (ppSamplerState == nullptr) { return S_FALSE; }
    const D3D11_SAMPLER_DESC a = angeglichen(*pSamplerDesc);
    Abtaster* s = nullptr;
    const HRESULT hr = zustandAnlegen(g_abtaster, a, &s);
    if (s != nullptr && s->gpu.ptr == 0U) {
        s->index = abtasterPlatzFuer(a);
        s->gpu = abtasterGpu(s->index);
    }
    *ppSamplerState = s;
    return hr;
}

HRESULT STDMETHODCALLTYPE Geraet::CreateQuery(const D3D11_QUERY_DESC* pQueryDesc, ID3D11Query** ppQuery) {
    if (pQueryDesc == nullptr) { return E_INVALIDARG; }
    if (pQueryDesc->Query != D3D11_QUERY_TIMESTAMP && pQueryDesc->Query != D3D11_QUERY_TIMESTAMP_DISJOINT && pQueryDesc->Query != D3D11_QUERY_EVENT) {
        return fehlt("CreateQuery(Art)");
    }
    if (ppQuery == nullptr) { return S_FALSE; }
    auto* q = new Abfrage;
    q->art = *pQueryDesc;
    *ppQuery = q;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Geraet::CheckFormatSupport(DXGI_FORMAT Format, UINT* pFormatSupport) {
    D3D12_FEATURE_DATA_FORMAT_SUPPORT fs{};
    fs.Format = Format;
    const HRESULT hr = rahmen::geraet()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &fs, sizeof(fs));
    if (pFormatSupport != nullptr) { *pFormatSupport = SUCCEEDED(hr) ? static_cast<UINT>(fs.Support1) : 0U; }   // dieselben Bits
    return hr;
}

HRESULT STDMETHODCALLTYPE Geraet::CheckMultisampleQualityLevels(DXGI_FORMAT Format, UINT SampleCount, UINT* pNumQualityLevels) {
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS q{};
    q.Format = Format;
    q.SampleCount = SampleCount;
    const HRESULT hr = rahmen::geraet()->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &q, sizeof(q));
    if (pNumQualityLevels != nullptr) { *pNumQualityLevels = SUCCEEDED(hr) ? q.NumQualityLevels : 0U; }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Geraet::CheckFeatureSupport(D3D11_FEATURE Feature, void* pFeatureSupportData, UINT FeatureSupportDataSize) {
    if (Feature == D3D11_FEATURE_D3D11_OPTIONS && pFeatureSupportData != nullptr && FeatureSupportDataSize == sizeof(D3D11_FEATURE_DATA_D3D11_OPTIONS)) {
        D3D11_FEATURE_DATA_D3D11_OPTIONS o{};
        o.DiscardAPIsSeenByDriver = TRUE;
        o.ClearView = TRUE;
        o.CopyWithOverlap = TRUE;
        o.ConstantBufferPartialUpdate = TRUE;
        o.ConstantBufferOffsetting = TRUE;                 // Root-CBV mit Adresse
        o.MapNoOverwriteOnDynamicConstantBuffer = TRUE;    // Hochladespeicher
        o.MapNoOverwriteOnDynamicBufferSRV = TRUE;
        o.MultisampleRTVWithForcedSampleCountOne = TRUE;
        o.SAD4ShaderInstructions = TRUE;
        o.ExtendedDoublesShaderInstructions = TRUE;
        o.ExtendedResourceSharing = FALSE;
        std::memcpy(pFeatureSupportData, &o, sizeof(o));
        return S_OK;
    }
    return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE Geraet::GetPrivateData(REFGUID, UINT*, void*) { return DXGI_ERROR_NOT_FOUND; }
HRESULT STDMETHODCALLTYPE Geraet::SetPrivateData(REFGUID, UINT, const void*) { return S_OK; }
HRESULT STDMETHODCALLTYPE Geraet::SetPrivateDataInterface(REFGUID, const IUnknown*) { return S_OK; }
D3D_FEATURE_LEVEL STDMETHODCALLTYPE Geraet::GetFeatureLevel() { return D3D_FEATURE_LEVEL_11_0; }   // wie initD3D11 (backend_win32.cpp)
UINT STDMETHODCALLTYPE Geraet::GetCreationFlags() { return 0; }
HRESULT STDMETHODCALLTYPE Geraet::GetDeviceRemovedReason() { return rahmen::geraet()->GetDeviceRemovedReason(); }
void STDMETHODCALLTYPE Geraet::GetImmediateContext(ID3D11DeviceContext** ppImmediateContext) {
    if (ppImmediateContext != nullptr) { *ppImmediateContext = kontext(); }
}
HRESULT STDMETHODCALLTYPE Geraet::SetExceptionMode(UINT) { return S_OK; }
UINT STDMETHODCALLTYPE Geraet::GetExceptionMode() { return 0; }

HRESULT STDMETHODCALLTYPE Geraet::CreateTexture1D(const D3D11_TEXTURE1D_DESC *, const D3D11_SUBRESOURCE_DATA *, ID3D11Texture1D **) { return fehlt("CreateTexture1D"); }
HRESULT STDMETHODCALLTYPE Geraet::CreateTexture3D(const D3D11_TEXTURE3D_DESC *, const D3D11_SUBRESOURCE_DATA *, ID3D11Texture3D **) { return fehlt("CreateTexture3D"); }
HRESULT STDMETHODCALLTYPE Geraet::CreateUnorderedAccessView(ID3D11Resource *, const D3D11_UNORDERED_ACCESS_VIEW_DESC *, ID3D11UnorderedAccessView **) { return fehlt("CreateUnorderedAccessView"); }
HRESULT STDMETHODCALLTYPE Geraet::CreateGeometryShaderWithStreamOutput(const void *, SIZE_T, const D3D11_SO_DECLARATION_ENTRY *, UINT, const UINT *, UINT, UINT, ID3D11ClassLinkage *, ID3D11GeometryShader **) { return fehlt("CreateGeometryShaderWithStreamOutput"); }
HRESULT STDMETHODCALLTYPE Geraet::CreateHullShader(const void *, SIZE_T, ID3D11ClassLinkage *, ID3D11HullShader **) { return fehlt("CreateHullShader"); }
HRESULT STDMETHODCALLTYPE Geraet::CreateDomainShader(const void *, SIZE_T, ID3D11ClassLinkage *, ID3D11DomainShader **) { return fehlt("CreateDomainShader"); }
HRESULT STDMETHODCALLTYPE Geraet::CreateComputeShader(const void *, SIZE_T, ID3D11ClassLinkage *, ID3D11ComputeShader **) { return fehlt("CreateComputeShader"); }
HRESULT STDMETHODCALLTYPE Geraet::CreateClassLinkage(ID3D11ClassLinkage **) { return fehlt("CreateClassLinkage"); }
HRESULT STDMETHODCALLTYPE Geraet::CreatePredicate(const D3D11_QUERY_DESC *, ID3D11Predicate **) { return fehlt("CreatePredicate"); }
HRESULT STDMETHODCALLTYPE Geraet::CreateCounter(const D3D11_COUNTER_DESC *, ID3D11Counter **) { return fehlt("CreateCounter"); }
HRESULT STDMETHODCALLTYPE Geraet::CreateDeferredContext(UINT, ID3D11DeviceContext **) { return fehlt("CreateDeferredContext"); }
HRESULT STDMETHODCALLTYPE Geraet::OpenSharedResource(HANDLE, REFIID, void **) { return fehlt("OpenSharedResource"); }
void STDMETHODCALLTYPE Geraet::CheckCounterInfo(D3D11_COUNTER_INFO *) { (void)fehlt("CheckCounterInfo"); }
HRESULT STDMETHODCALLTYPE Geraet::CheckCounter(const D3D11_COUNTER_DESC *, D3D11_COUNTER_TYPE *, UINT *, LPSTR, UINT *, LPSTR, UINT *, LPSTR, UINT *) { return fehlt("CheckCounter"); }

// ===================================================================================

bool anlegen() {
    ID3D12Device* dev = rahmen::geraet();
    if (dev == nullptr) { return false; }
    g_faden = GetCurrentThreadId();
    g_wurzel = wurzelAnlegen();
    g_zielStapel = stapel(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kZielSichten + 1U, false);   // der letzte: hilfsZiel
    g_tiefeStapel = stapel(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, kTiefeSichten, false);
    g_abtasterStapel = stapel(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, kAbtaster, true);
    if (g_wurzel == nullptr || g_zielStapel == nullptr || g_tiefeStapel == nullptr || g_abtasterStapel == nullptr) {
        abbauen();
        return false;
    }
    g_zielSchritt = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    g_tiefeSchritt = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    g_abtasterSchritt = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    g_aktiv = true;
    // Reservierte Ressourcen, deren leere Kacheln 0 lesen und Schreiben verwerfen: ab Stufe 2 (D3D12_TILED_RESOURCES_TIER_2).
    D3D12_FEATURE_DATA_D3D12_OPTIONS wahl{};
    g_duennGeht = SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &wahl, sizeof(wahl))) && wahl.TiledResourcesTier >= D3D12_TILED_RESOURCES_TIER_2;
    if (const char* v = std::getenv("BHED_D3D12_DUENN"); v != nullptr && v[0] == '0') { g_duennGeht = false; }   // zum Vergleichsmessen
    // Teilabgabe nach 1 ms Aufzeichnung (an einem Wechsel der Ziele): D3D12-2, Messreihen vb/vc vom 12.11. - ohne sie arbeitet
    // die Grafikkarte erst, wenn das ganze Bild aufgezeichnet ist, und der Bildabstand im Karten-Editor lag bei 5,0-6,2 statt 4,2 ms.
    // BHED_D3D12_TEILABGABE=<Mikrosekunden> (0: nur am Ende des Bildes).
    {
        long long mikro = 1000;
        if (const char* v = std::getenv("BHED_D3D12_TEILABGABE"); v != nullptr && v[0] != '\0') { mikro = static_cast<long long>(std::strtoull(v, nullptr, 10)); }
        LARGE_INTEGER f{};
        QueryPerformanceFrequency(&f);
        g_teilAbgabe = mikro * f.QuadPart / 1000000LL;
    }
    g_auslagern = 2;
    if (const char* v = std::getenv("BHED_D3D12_AUSLAGERN"); v != nullptr && v[0] != '\0') { g_auslagern = v[0] == '0' ? 0 : v[0] == 'b' ? 1 : 2; }
    g_budgetVorgabe = 0;
    if (const char* v = std::getenv("BHED_D3D12_BUDGET_MB"); v != nullptr && v[0] != '\0') {
        g_budgetVorgabe = static_cast<std::uint64_t>(std::strtoull(v, nullptr, 10)) << 20U;
    }
    if (g_karte3 == nullptr) {
        IDXGIFactory4* fabrik = nullptr;
        if (SUCCEEDED(CreateDXGIFactory2(0, IID_PPV_ARGS(&fabrik))) && fabrik != nullptr) {
            if (FAILED(fabrik->EnumAdapterByLuid(dev->GetAdapterLuid(), IID_PPV_ARGS(&g_karte3)))) { g_karte3 = nullptr; }
            fabrik->Release();
        }
    }
    // Die leere Sicht: Texture2D ohne Ressource liest 0 - wie ein freier Platz unter Direct3D 11.
    g_leereSicht = rahmen::srvNehmen();
    D3D12_SHADER_RESOURCE_VIEW_DESC leer{};
    leer.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    leer.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    leer.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    leer.Texture2D.MipLevels = 1;
    dev->CreateShaderResourceView(nullptr, &leer, rahmen::srvCpu(g_leereSicht));
    // Der Abtaster ohne gebundenen Zustand: die Vorgabe von Direct3D 11 (D3D11_SAMPLER_DESC ohne Angaben).
    D3D11_SAMPLER_DESC vorgabe{};
    vorgabe.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    vorgabe.AddressU = vorgabe.AddressV = vorgabe.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    vorgabe.MaxAnisotropy = 1;
    vorgabe.ComparisonFunc = D3D11_COMPARISON_NEVER;
    vorgabe.BorderColor[0] = vorgabe.BorderColor[1] = vorgabe.BorderColor[2] = vorgabe.BorderColor[3] = 1.0F;
    vorgabe.MinLOD = -D3D11_FLOAT32_MAX;
    vorgabe.MaxLOD = D3D11_FLOAT32_MAX;
    g_vorgabeAbtaster = abtasterGpu(abtasterPlatzFuer(angeglichen(vorgabe)));
    g_nullSeite = blockNehmen(kKleinsterBlock << (kKlassen - 1U));
    if (g_nullSeite.res == nullptr) {
        abbauen();
        return false;
    }
    std::memset(g_nullSeite.cpu, 0, static_cast<std::size_t>(g_nullSeite.groesse));
    // Zeitmarken: ein Abfragestapel im Ring, aufgeloest in einen Lesepuffer.
    D3D12_QUERY_HEAP_DESC qd{};
    qd.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    qd.Count = kMarken;
    D3D12_HEAP_PROPERTIES hp{};
    hp.Type = D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC bd{};
    bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bd.Width = static_cast<UINT64>(kMarken) * sizeof(std::uint64_t);
    bd.Height = 1;
    bd.DepthOrArraySize = 1;
    bd.MipLevels = 1;
    bd.SampleDesc.Count = 1;
    bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    void* z = nullptr;
    if (SUCCEEDED(dev->CreateQueryHeap(&qd, IID_PPV_ARGS(&g_markenStapel))) &&
        SUCCEEDED(dev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&g_markenPuffer))) &&
        SUCCEEDED(g_markenPuffer->Map(0, nullptr, &z))) {
        g_markenWerte = static_cast<const std::uint64_t*>(z);
    } else {
        frei(g_markenStapel);
        frei(g_markenPuffer);
    }
    // Geraet und Kontext leben bis zum Prozessende: ein spaetes GetDevice oder Release der Zeichenschichten bleibt gueltig.
    if (g_geraet == nullptr) { g_geraet = new Geraet; }
    if (g_kontext == nullptr) { g_kontext = new Kontext; }
    return true;
}

void abbauen() {
    if (g_kontext != nullptr) { g_kontext->allesLoesen(); }
    psoAbbauen();
    protokoll::abbauen();
    if (g_lebend.load() != 0U && g_aktiv) {
        diag::detail("D3D12-Abgeber: beim Abbau leben noch " + std::to_string(g_lebend.load()) + " Objekte der Zeichenschichten");
    }
    g_aktiv = false;
    g_markenWerte = nullptr;
    frei(g_markenPuffer);
    frei(g_markenStapel);
    g_markenOffen.clear();
    g_markeNaechste = 0;
    for (auto& k : g_freiKlein) { k.clear(); }
    for (auto& [g, gf] : g_freiGross) {
        (void)g;
        for (Block& b : gf.bloecke) { b.res->Release(); }
    }
    g_freiGross.clear();
    for (Block& b : g_offen) {
        if (b.groesse > (kKleinsterBlock << (kKlassen - 1U))) { b.res->Release(); }
    }
    g_offen.clear();
    g_listenLeer.clear();
    for (VorratPuffer& v : g_vorrat) { v.res->Release(); }
    g_vorrat.clear();
    for (VorratPuffer& v : g_vorratOffen) { v.res->Release(); }
    g_vorratOffen.clear();
    for (auto& [zaun, liste_] : g_vorratWartend) {
        (void)zaun;
        for (VorratPuffer& v : liste_) { v.res->Release(); }
    }
    g_vorratWartend.clear();
    g_vorratJetzt = 0;
    for (auto& [zaun, liste_] : g_wartend) {
        (void)zaun;
        for (Block& b : liste_) {
            if (b.groesse > (kKleinsterBlock << (kKlassen - 1U))) { b.res->Release(); }
        }
    }
    g_wartend.clear();
    for (ID3D12Resource* s : g_seiten) { s->Release(); }
    g_seiten.clear();
    g_nullSeite = Block{};
    g_zaehler.hochladeBytes = 0;
    g_mischungen.clear();
    g_tiefeZustaende.clear();
    g_raster.clear();
    g_abtaster.clear();
    g_abtasterPlatz.clear();
    g_abtasterNaechster = 0;
    g_zielFrei.clear();
    g_tiefeFrei.clear();
    g_zielNaechster = g_tiefeNaechster = 0;
    g_bild = 0;
    g_blockHeapEigen = true;
    g_duennGeht = false;
    g_gebrauchBild = 0;
    for (BildStand& b : g_bildStand) { b = BildStand{}; }
    g_pruefZeit = 0;
    if (g_karte3 != nullptr) {
        g_karte3->Release();
        g_karte3 = nullptr;
    }
    g_budgetZuletzt = 0;
    g_belegtZuletzt = 0;
    g_ausgelagertMal = 0;
    g_eingelagertMal = 0;
    g_eingelagertMs = 0.0;
    g_sichten.clear();
    g_texturenLebend.clear();
    g_pufferLebend.clear();
    frei(g_abtasterStapel);
    frei(g_tiefeStapel);
    frei(g_zielStapel);
    frei(g_wurzel);
    g_faden = 0;
}

ID3D11Device* geraet() { return g_aktiv ? g_geraet : nullptr; }
bool aktiv() { return g_aktiv; }
D3D12_CPU_DESCRIPTOR_HANDLE hilfsZiel() { return zielGriff(kZielSichten); }

void vorAbschicken(ID3D12GraphicsCommandList* cl) {
    if (protokoll::g_an) { protokoll::eintrag(cl, "Abschicken"); }
    if (g_markenOffen.empty() || g_markenStapel == nullptr) { return; }
    // zusammenhaengende Bereiche aufloesen (im Ring hoechstens zwei)
    std::sort(g_markenOffen.begin(), g_markenOffen.end());
    std::size_t i = 0;
    while (i < g_markenOffen.size()) {
        std::size_t j = i;
        while (j + 1U < g_markenOffen.size() && g_markenOffen[j + 1U] <= g_markenOffen[j] + 1U) { ++j; }
        const UINT erste = g_markenOffen[i];
        const UINT anzahl = g_markenOffen[j] - erste + 1U;
        cl->ResolveQueryData(g_markenStapel, D3D12_QUERY_TYPE_TIMESTAMP, erste, anzahl, g_markenPuffer, static_cast<UINT64>(erste) * sizeof(std::uint64_t));
        i = j + 1U;
    }
    g_markenOffen.clear();
}

void bloeckeStempeln(std::uint64_t zaun) {
    if (!g_vorratOffen.empty()) {
        g_vorratWartend.emplace_back(zaun, std::move(g_vorratOffen));
        g_vorratOffen.clear();
    }
    if (!g_offen.empty()) {
        g_wartend.emplace_back(zaun, std::move(g_offen));
        if (!g_listenLeer.empty()) {
            g_offen = std::move(g_listenLeer.back());
            g_listenLeer.pop_back();
        } else {
            g_offen.clear();
        }
    }
}

void einsammeln() { bloeckeEinsammeln(); }

namespace {
// Alle 64 Bilder oder nach einer Sekunde (ein langes Bild, etwa das Laden einer Karte): Texturen, die seit kAuslagernMs in
// keiner Befehlsliste vorkamen, geben ihren Grafikspeicher ab. Der Inhalt bleibt erhalten.
// Sicher ist das ueber den Zaunwert: g_bildStand merkt je Bild, welcher Zaunwert alle bis dahin abgeschickten Listen deckt.
// Eine Textur, zuletzt gebraucht im Bild U, geht erst, wenn das Bild U + 1 vor mindestens kAuslagernMs begann UND die
// Grafikkarte dessen Zaunwert erreicht hat - jede Liste, die die Textur nennt, ist dann abgearbeitet.
void auslagernPruefen() {
    const std::uint64_t jetzt = GetTickCount64();
    g_bildStand[g_gebrauchBild % kBildRing] = BildStand{rahmen::letzterZaun(), jetzt};   // auch wenn nicht geprueft wird
    if (g_auslagern == 0) { return; }
    if ((g_bild & 63U) != 0U && jetzt - g_pruefZeit < 1000U) { return; }
    g_pruefZeit = jetzt;
    // Vorgabe: alles lange Ungebrauchte geht. Direct3D 11 belegt im Karten-Editor meist 2,1-2,4 GB, wo ohne Auslagern 4,8 GB
    // stehen (Messfenster 11.11.: sechs von sieben Laeufen; das Budget des Systems stand dabei unveraendert bei 7 GB - ein
    // Abgeber, der nur dem Budget folgt, behielte alles). Mit =budget geht nur, was ueber dem Budget liegt.
    std::uint64_t zuviel = ~0ULL;   // so viel soll gehen
    if (g_auslagern == 1) {
        DXGI_QUERY_VIDEO_MEMORY_INFO mi{};
        if (g_karte3 == nullptr || FAILED(g_karte3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &mi))) { return; }
        if (g_budgetVorgabe != 0U) { mi.Budget = g_budgetVorgabe; }
        g_budgetZuletzt = mi.Budget;
        g_belegtZuletzt = mi.CurrentUsage;
        if (mi.Budget == 0U || mi.CurrentUsage <= mi.Budget) { return; }
        zuviel = mi.CurrentUsage - mi.Budget + mi.Budget / 16U;   // bis ein Sechzehntel unter das Budget
    }
    struct Anwaerter {
        std::uint32_t ungebraucht;
        std::uint64_t bytes;
        Textur* t;
    };
    static std::vector<Anwaerter> anwaerter;
    static std::vector<ID3D12Pageable*> weg;
    anwaerter.clear();
    weg.clear();
    for (const Textur* lebt : g_texturenLebend) {
        Textur* t = const_cast<Textur*>(lebt);   // die Menge fuehrt die Texturen nur zum Zaehlen
        const std::uint32_t ungebraucht = g_gebrauchBild - t->gebrauchtBild;
        if (t->ausgelagert || t->lesepuffer || t->res == nullptr || ungebraucht < 2U) { continue; }
        // der Stand am Anfang des Bildes nach dem letzten Gebrauch; ist er ueberschrieben, der aelteste, den es noch gibt
        const BildStand& danach = g_bildStand[(ungebraucht < kBildRing ? t->gebrauchtBild + 1U : g_gebrauchBild + 1U) % kBildRing];
        if (jetzt - danach.zeit < kAuslagernMs || !rahmen::fertig(danach.zaun)) { continue; }
        std::uint64_t b = t->bytes;
        if (!t->schichtHeap.empty()) {
            b = 0;
            for (const ID3D12Heap* h : t->schichtHeap) { b += h != nullptr ? static_cast<std::uint64_t>(t->schichtKacheln) * D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES : 0U; }
        }
        if (b < kAuslagernAb) { continue; }
        anwaerter.push_back(Anwaerter{ungebraucht, b, t});
    }
    // das am laengsten Ungebrauchte zuerst, bei gleichem Alter das Groessere
    std::sort(anwaerter.begin(), anwaerter.end(), [](const Anwaerter& a, const Anwaerter& b) {
        return a.ungebraucht != b.ungebraucht ? a.ungebraucht > b.ungebraucht : a.bytes > b.bytes;
    });
    std::uint64_t bytes = 0;
    std::size_t n = 0;
    for (; n < anwaerter.size() && n < 256U && bytes < zuviel; ++n) {   // der Rest bei der naechsten Pruefung
        Textur* t = anwaerter[n].t;
        if (t->schichtHeap.empty()) {
            weg.push_back(t->res);
        } else {
            for (ID3D12Heap* h : t->schichtHeap) {
                if (h != nullptr) { weg.push_back(h); }
            }
        }
        bytes += anwaerter[n].bytes;
    }
    if (weg.empty() || FAILED(rahmen::geraet()->Evict(static_cast<UINT>(weg.size()), weg.data()))) { return; }
    for (std::size_t i = 0; i < n; ++i) { anwaerter[i].t->ausgelagert = true; }
    g_ausgelagertMal += n;
    char z[200];
    std::snprintf(z, sizeof(z), "D3D12-Abgeber: %u Texturen ausgelagert (%.0f MB, am laengsten ungebraucht: %u Bilder; belegt %.0f MB, Budget %.0f MB)",
                  static_cast<unsigned>(n), static_cast<double>(bytes) / (1024.0 * 1024.0), static_cast<unsigned>(anwaerter[0].ungebraucht),
                  static_cast<double>(g_belegtZuletzt) / (1024.0 * 1024.0), static_cast<double>(g_budgetZuletzt) / (1024.0 * 1024.0));
    diag::detail(z);
}
}  // namespace

void bildAnfang() {
    bloeckeEinsammeln();
    ++g_bild;
    g_gebrauchBild = static_cast<std::uint32_t>(g_bild);
    auslagernPruefen();
    // freie grosse Bloecke, deren Groesse lange niemand genommen hat (hoechstens einer je Bild)
    for (auto it = g_freiGross.begin(); it != g_freiGross.end(); ++it) {
        GrossFrei& gf = it->second;
        if (gf.bloecke.empty()) {
            if (g_bild - gf.genommen > kGrossBilder) {
                g_freiGross.erase(it);
                break;
            }
            continue;
        }
        if (g_bild - gf.genommen > kGrossBilder) {
            g_zaehler.hochladeBytes -= gf.bloecke.back().groesse;
            gf.bloecke.back().res->Release();
            gf.bloecke.pop_back();
            break;
        }
    }
    // Vorrat: was lange niemand genommen hat, geht zurueck (hoechstens einer je Bild)
    for (std::size_t i = 0; i < g_vorrat.size(); ++i) {
        if (g_bild - g_vorrat[i].bild > kVorratBilder) {
            g_vorratJetzt -= g_vorrat[i].groesse;
            g_vorrat[i].res->Release();
            g_vorrat[i] = g_vorrat.back();
            g_vorrat.pop_back();
            break;
        }
    }
    protokoll::bildWechsel();
}

void protokollNotiz(ID3D12GraphicsCommandList* cl, const char* text) {
    if (protokoll::g_an) { protokoll::eintrag(cl, text); }
}

bool istSicht(std::uint64_t kennung) { return g_sichten.count(kennung) != 0U; }

std::uint64_t sichtZeigen(std::uint64_t kennung, ID3D12GraphicsCommandList* cl, const void** res) {
    Sicht* s = static_cast<Sicht*>(reinterpret_cast<ID3D11ShaderResourceView*>(static_cast<std::uintptr_t>(kennung)));
    if (res != nullptr) { *res = s->textur != nullptr ? static_cast<const void*>(static_cast<ID3D11Resource*>(s->textur)) : nullptr; }
    if (s->textur == nullptr || s->index == rahmen::srvKeiner()) { return leereSicht().ptr; }
    wechsle(cl, s->textur, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    // imgui_impl_dx11 tastet mit MaxLOD 0 ab - die Oberflaeche zeigt unter Direct3D 11 immer die oberste Stufe der Sicht, auch
    // verkleinert. imgui_impl_dx12 tastet ueber alle Stufen ab; deshalb bekommt die Oberflaeche eine Sicht mit genau einer Stufe.
    if (s->indexOben == ~0U) {
        s->indexOben = s->index;
        if (s->art12.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2D) {
            const UINT stufen = s->art12.Texture2D.MipLevels == static_cast<UINT>(-1) ? s->textur->art.MipLevels - s->art12.Texture2D.MostDetailedMip
                                                                                       : s->art12.Texture2D.MipLevels;
            if (stufen > 1U) {
                const UINT neu = rahmen::srvNehmen();
                if (neu != rahmen::srvKeiner()) {
                    D3D12_SHADER_RESOURCE_VIEW_DESC d = s->art12;
                    d.Texture2D.MipLevels = 1;
                    rahmen::geraet()->CreateShaderResourceView(s->textur->res, &d, rahmen::srvCpu(neu));
                    s->indexOben = neu;
                }
            }
        }
    }
    return rahmen::srvGpu(s->indexOben).ptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE zielFuerOberflaeche(ID3D11Resource* textur, ID3D12GraphicsCommandList* cl) {
    D3D12_CPU_DESCRIPTOR_HANDLE h{};
    if (textur == nullptr || !g_aktiv) { return h; }
    Textur* t = static_cast<Textur*>(static_cast<ID3D11Texture2D*>(textur));
    if (t->lesepuffer || (t->art.BindFlags & D3D11_BIND_RENDER_TARGET) == 0U || t->art.Format != DXGI_FORMAT_R8G8B8A8_UNORM || t->art.SampleDesc.Count != 1U) {
        return h;
    }
    h = hilfsZiel();
    rahmen::geraet()->CreateRenderTargetView(t->res, nullptr, h);
    wechsle(cl, t, D3D12_RESOURCE_STATE_RENDER_TARGET);
    kontextAllesNeu();
    return h;
}

ID3D12Resource* ressource(ID3D11Resource* r, std::uint64_t* versatz) {
    if (versatz != nullptr) { *versatz = 0; }
    if (r == nullptr) { return nullptr; }
    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    r->GetType(&dim);
    if (dim == D3D11_RESOURCE_DIMENSION_BUFFER) {
        const Puffer* p = static_cast<Puffer*>(static_cast<ID3D11Buffer*>(r));
        if (p->dynamisch) {
            if (versatz != nullptr) { *versatz = p->block.versatz; }
            return p->block.res;
        }
        return p->res;
    }
    Textur* t = static_cast<Textur*>(static_cast<ID3D11Texture2D*>(r));
    gebraucht(t);   // wer die Ressource selbst in eine Liste bringt, fragt in jedem Bild neu (Auslagern)
    return t->res;
}

// DXR: die Sicht haelt weder Textur noch Puffer - nur ihren Deskriptor (Kontext::vorZeichnen bindet ihn ueber s->index).
ID3D11ShaderResourceView* strukturSicht(D3D12_GPU_VIRTUAL_ADDRESS adresse) {
    if (!g_aktiv || adresse == 0U) { return nullptr; }
    auto* s = new Sicht;
    s->index = rahmen::srvNehmen();
    if (s->index == rahmen::srvKeiner()) {
        s->Release();
        diag::error("D3D12-Abgeber: der Deskriptorstapel der Texturen ist voll");
        return nullptr;
    }
    s->art12.Format = DXGI_FORMAT_UNKNOWN;
    s->art12.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    s->art12.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    s->art12.RaytracingAccelerationStructure.Location = adresse;
    rahmen::geraet()->CreateShaderResourceView(nullptr, &s->art12, rahmen::srvCpu(s->index));
    sichtMelden(s, true);
    return s;
}

Zaehler& zaehlerIntern() { return g_zaehler; }

std::vector<std::string> speicherBericht(std::uint64_t& summe) {
    struct Art {
        std::uint64_t n = 0;
        std::uint64_t bytes = 0;
    };
    Art ziele, tiefen, texturen, lesepuffer, puffer, ruhende, dynamisch, fort;
    std::vector<std::pair<std::uint64_t, std::string>> gross;
    ID3D12Device* dev = rahmen::geraet();
    for (const Textur* t : g_texturenLebend) {
        if (t->res == nullptr) { continue; }
        const D3D12_RESOURCE_DESC rd = t->res->GetDesc();
        if (t->lesepuffer) {
            ++lesepuffer.n;
            lesepuffer.bytes += rd.Width;
            continue;
        }
        std::uint64_t b = 0;
        if (!t->schichtHeap.empty()) {
            for (const ID3D12Heap* h : t->schichtHeap) { b += h != nullptr ? static_cast<std::uint64_t>(t->schichtKacheln) * D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES : 0U; }
        } else {
            D3D12_RESOURCE_DESC gewoehnlich = rd;
            b = dev->GetResourceAllocationInfo(0, 1, &gewoehnlich).SizeInBytes;
        }
        const bool istTiefe = (t->art.BindFlags & D3D11_BIND_DEPTH_STENCIL) != 0U;
        const bool istZiel = (t->art.BindFlags & D3D11_BIND_RENDER_TARGET) != 0U;
        if (t->ausgelagert) {   // lebt, belegt aber keinen Grafikspeicher
            ++fort.n;
            fort.bytes += b;
            continue;
        }
        Art& a = istTiefe ? tiefen : istZiel ? ziele : texturen;
        ++a.n;
        a.bytes += b;
        gross.emplace_back(b, protokoll::nenne(t) + (istTiefe ? " Tiefe" : istZiel ? " Ziel" : "") + (t->schichtHeap.empty() ? "" : " (je Schicht belegt)"));
    }
    for (const Puffer* p : g_pufferLebend) {
        if (p->dynamisch) {
            ++dynamisch.n;
            dynamisch.bytes += p->block.groesse;
            if (p->ruhe != nullptr) {
                ++ruhende.n;
                ruhende.bytes += p->art.ByteWidth;
            }
        } else {
            ++puffer.n;
            puffer.bytes += p->art.ByteWidth;
        }
    }
    std::uint64_t frei_ = 0;
    for (UINT k = 0; k < kKlassen; ++k) { frei_ += g_freiKlein[k].size() * (kKleinsterBlock << k); }
    for (const auto& [g, gf] : g_freiGross) { frei_ += g * gf.bloecke.size(); }
    std::uint64_t wartend = 0;
    for (const Block& b : g_offen) { wartend += b.groesse; }
    for (const auto& [zaun, liste_] : g_wartend) {
        (void)zaun;
        for (const Block& b : liste_) { wartend += b.groesse; }
    }
    const rahmen::Statistik st = rahmen::statistik();
    summe = ziele.bytes + tiefen.bytes + texturen.bytes + puffer.bytes + ruhende.bytes + g_vorratJetzt;
    const auto mb = [](std::uint64_t b) { return static_cast<double>(b) / (1024.0 * 1024.0); };
    std::vector<std::string> aus;
    char z[420];
    std::snprintf(z, sizeof(z),
                  "D3D12-Speicher Grafikspeicher %.0f MB: Ziele %llu / %.0f MB, Tiefe %llu / %.0f MB, Texturen %llu / %.0f MB, Puffer %llu / %.0f MB, "
                  "ruhende Kopien %llu / %.0f MB, Vorrat an Puffern %.1f MB",
                  mb(summe), static_cast<unsigned long long>(ziele.n), mb(ziele.bytes), static_cast<unsigned long long>(tiefen.n), mb(tiefen.bytes),
                  static_cast<unsigned long long>(texturen.n), mb(texturen.bytes), static_cast<unsigned long long>(puffer.n), mb(puffer.bytes),
                  static_cast<unsigned long long>(ruhende.n), mb(ruhende.bytes), mb(g_vorratJetzt));
    aus.emplace_back(z);
    std::snprintf(z, sizeof(z),
                  "D3D12-Speicher Arbeitsspeicher: Bloecke %.0f MB (dynamische Puffer %llu / %.0f MB, frei %.0f MB, warten auf die Grafikkarte %.0f MB), "
                  "Hochladepuffer der Bildplaetze %.0f MB, Lesepuffer %llu / %.0f MB; %llu Objekte warten auf ihre Freigabe",
                  mb(g_zaehler.hochladeBytes), static_cast<unsigned long long>(dynamisch.n), mb(dynamisch.bytes), mb(frei_), mb(wartend), mb(st.hochladeJetzt),
                  static_cast<unsigned long long>(lesepuffer.n), mb(lesepuffer.bytes), static_cast<unsigned long long>(st.spaeter));
    aus.emplace_back(z);
    std::snprintf(z, sizeof(z), "D3D12-Speicher ausgelagert: %llu Texturen / %.0f MB (bisher %llu-mal ausgelagert, %llu-mal zurueckgeholt in %.1f ms); "
                  "bei der letzten Pruefung belegt %.0f MB, Budget %.0f MB",
                  static_cast<unsigned long long>(fort.n), mb(fort.bytes), static_cast<unsigned long long>(g_ausgelagertMal),
                  static_cast<unsigned long long>(g_eingelagertMal), g_eingelagertMs, mb(g_belegtZuletzt), mb(g_budgetZuletzt));
    aus.emplace_back(z);
    std::sort(gross.begin(), gross.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    for (std::size_t i = 0; i < gross.size() && i < 12U; ++i) {
        std::snprintf(z, sizeof(z), "D3D12-Speicher   %7.1f MB  %s", mb(gross[i].first), gross[i].second.c_str());
        aus.emplace_back(z);
    }
    return aus;
}

Zaehler zaehler() {
    Zaehler z = g_zaehler;
    z.lebend = g_lebend.load();
    return z;
}

}  // namespace bhed::render::d3d12::abgeber
