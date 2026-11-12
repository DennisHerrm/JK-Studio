// backend_d3d12_win32.cpp - Direct3D 12: Geraet, Swapchain, Bildtakt, die Befehlsliste des Bildes, ImGui (imgui_impl_dx12)
//
// Aufbau und Gruende in backend_d3d12.h, abgeber12.h und docs/intern/NOTIZEN_d3d12_1011.md. Kurz:
//   - ein Geraet (Merkmalsstufe 11_0) auf der Karte, die auch Direct3D 11 nimmt (holeKarte), eine direkte Warteschlange,
//   - Swapchain wie bei Direct3D 11: Flip-Modell, drei Puffer, wartbar mit hoechstens zwei Bildern Vorlauf, Present(1, 0),
//   - drei Bildplaetze (Befehlsspeicher, Hochladepuffer, Zaun): ein Platz wird erst wieder beschrieben, wenn die Grafikkarte
//     das Bild darin fertig hat - dasselbe, was Direct3D 11 im Treiber verborgen tut,
//   - EINE Befehlsliste je Bild: zuerst schreibt der Abgeber (abgeber12_*.cpp) die Befehle der 3D-Ansichten hinein, am Ende die
//     Oberflaeche (imgui_impl_dx12) das Hauptfenster. Davor laeuft die Vor-Liste mit dem Hochladen neuer Texturen und Puffer,
//   - abgeschickt und gezeigt wird auf einem eigenen Faden (Abschickfaden: ExecuteCommandLists, Present, Signal in der
//     Reihenfolge der Auftraege) - unter Direct3D 11 tut das der Faden des Treibers,
//   - ein Deskriptorstapel fuer alle Texturen (ImGui-Schrift, eigene Bilder, Bilder der 3D-Ansichten): die Oberflaeche zeigt ein
//     Bild der 3D-Ansichten ueber dessen festen Deskriptor - keine Kopie, keine Bruecke.

#include <windows.h>

#include "backend_d3d12.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>

#include <d3d12.h>
#include <d3dcompiler.h>

#include "abgeber12.h"
#include "bhed/diag.h"
#include "imgui_impl_dx12.h"

namespace bhed::render::d3d12 {
namespace {

constexpr UINT kPuffer = 3;            // Swapchain-Puffer und Bildplaetze
constexpr UINT kDeskriptoren = 65536;  // Texturen gleichzeitig (ImGui-Schrift, eigene Bilder, Sichten der 3D-Ansichten)
constexpr DXGI_FORMAT kFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr UINT64 kVorGrenze = 256ULL << 20U;   // so viel Hochladespeicher der Vor-Liste, dann abschicken und freigeben
constexpr UINT64 kHochladeSeite = 2ULL << 20U; // uebliche Groesse eines Hochladepuffers je Bildplatz (groessere nach Bedarf)

template <typename T>
void frei(T*& p) {
    if (p != nullptr) {
        p->Release();
        p = nullptr;
    }
}

struct Hochlade {
    ID3D12Resource* puffer = nullptr;
    UINT64 groesse = 0;
    UINT64 belegt = 0;
    unsigned char* zeiger = nullptr;
};

struct Platz {
    ID3D12CommandAllocator* haupt = nullptr;   // die Befehlsliste des Bildes
    ID3D12CommandAllocator* vor = nullptr;     // Hochladen neuer Texturen und Puffer vor dem Bild
    UINT64 zaun = 0;
    std::vector<Hochlade> hoch;      // von der Befehlsliste des Bildes gelesen
    std::vector<Hochlade> hochVor;   // von der Vor-Liste gelesen
    UINT64 vorBytes = 0;
};

struct Textur {
    ID3D12Resource* res = nullptr;
    UINT index = 0;
    int w = 0;
    int h = 0;
};

ID3D12Device* g_dev = nullptr;
ID3D12CommandQueue* g_queue = nullptr;
ID3D12Fence* g_zaun = nullptr;
HANDLE g_zaunEreignis = nullptr;
UINT64 g_zaunWert = 0;
UINT64 g_letzterZaun = 0;     // nach der letzten abgeschickten Liste
UINT64 g_ausfuehrung = 0;     // zaehlt ExecuteCommandLists
IDXGISwapChain3* g_swap = nullptr;
UINT g_swapFlags = 0;
HANDLE g_bildFrei = nullptr;
ID3D12DescriptorHeap* g_rtvHeap = nullptr;   // kPuffer Hintergrundpuffer + 1 fuer fensterLesen
UINT g_rtvSchritt = 0;
ID3D12Resource* g_hinten[kPuffer] = {};
ID3D12DescriptorHeap* g_srvHeap = nullptr;
UINT g_srvSchritt = 0;
D3D12_CPU_DESCRIPTOR_HANDLE g_srvCpu0{};   // Anfang des Stapels (je gebundener Textur gebraucht - nicht jedes Mal den Stapel fragen)
D3D12_GPU_DESCRIPTOR_HANDLE g_srvGpu0{};
std::vector<UINT> g_srvFrei;
UINT g_srvNaechster = 0;
Platz g_plaetze[kPuffer];
UINT g_platz = 0;
ID3D12GraphicsCommandList* g_liste = nullptr;      // die offene Befehlsliste des Bildes (nullptr: keine offen)
ID3D12GraphicsCommandList* g_vorListe = nullptr;   // die offene Vor-Liste
bool g_hauptOffen = false;
bool g_vorOffen = false;
// Der Abschickfaden: ExecuteCommandLists, Present und Signal - je ein Kernaufruf - laufen auf einem eigenen Faden, in der
// Reihenfolge der Auftraege. Der Hauptfaden schliesst eine Liste, gibt sie als Auftrag ab und schreibt in einer anderen weiter;
// Zaunwerte vergibt er selbst (g_zaunWert), signalisiert werden sie vom Abschickfaden hinter den Listen des Auftrags. Wer die
// Warteschlange oder die Swapchain selbst braucht (Hintergrundpuffer waehlen, Groesse aendern, eigene Fenster von
// imgui_impl_dx12, Abbau), wartet vorher mit abschickenAbwarten. Unter Direct3D 11 tut dasselbe der Faden des Treibers.
struct Auftrag {
    ID3D12CommandList* listen[2] = {};
    UINT n = 0;
    bool zeigen = false;   // Present(1, 0) nach den Listen
    UINT64 zaun = 0;       // danach signalisieren (0: nichts)
};
std::thread g_abschickFaden;
std::mutex g_abschickSperre;
std::condition_variable g_abschickWecken;   // ein Auftrag liegt bereit, oder Ende
std::condition_variable g_abschickLeer;     // alle Auftraege sind ausgefuehrt
std::deque<Auftrag> g_auftraege;
bool g_abschickArbeitet = false;
bool g_abschickEnde = false;
bool g_abschickEigen = false;   // der Faden laeuft; sonst fuehrt der Hauptfaden jeden Auftrag sofort selbst aus
std::vector<ID3D12GraphicsCommandList*> g_listenFrei;   // ausgefuehrte (geschlossene) Listen, wieder zu vergeben
std::atomic<long> g_zeigenErgebnis{0};   // HRESULT des letzten fehlgeschlagenen Present (S_OK: keins)
// Spaeter freigeben: was seit der letzten abgeschickten Liste freigegeben wurde (offen), und was an einem Zaunwert haengt.
std::vector<IUnknown*> g_objekteOffen;
std::vector<UINT> g_srvOffen;
struct Spaeter {
    UINT64 zaun = 0;
    std::vector<IUnknown*> objekte;
    std::vector<UINT> srv;
};
std::deque<Spaeter> g_spaeter;
std::unordered_map<UINT64, Textur> g_texturen;   // GPU-Kennung -> eigene Textur
std::set<UINT64> g_frisch;
// Mischarten (zeichneAdditiv, zeichneStufe, ohneMischen)
ID3DBlob* g_vs = nullptr;
ID3DBlob* g_ps = nullptr;
ID3D12RootSignature* g_rsKlemm = nullptr;
ID3D12RootSignature* g_rsWieder = nullptr;
std::map<int, ID3D12PipelineState*> g_pso;
// Debugschicht
ID3D12InfoQueue* g_info = nullptr;
std::set<std::string> g_infoGesehen;
unsigned g_debugFehler = 0;   // Fehler der Debugschicht seit dem Start (Selbsttest)
std::string g_kartenName;
LUID g_luid{};
rahmen::Statistik g_statistik;

// --- Abschickfaden ---------------------------------------------------------------

void auftragAusfuehren(const Auftrag& a) {
    if (a.n != 0U) { g_queue->ExecuteCommandLists(a.n, a.listen); }
    if (a.zeigen) {
        const HRESULT hr = g_swap->Present(1, 0);
        if (FAILED(hr)) { g_zeigenErgebnis.store(hr); }
    }
    if (a.zaun != 0U) { g_queue->Signal(g_zaun, a.zaun); }
}

void abschickSchleife() {
    std::unique_lock<std::mutex> sperre(g_abschickSperre);
    for (;;) {
        g_abschickWecken.wait(sperre, [] { return g_abschickEnde || !g_auftraege.empty(); });
        if (g_auftraege.empty()) { return; }
        const Auftrag a = g_auftraege.front();
        g_auftraege.pop_front();
        g_abschickArbeitet = true;
        sperre.unlock();
        auftragAusfuehren(a);
        sperre.lock();
        for (UINT i = 0; i < a.n; ++i) { g_listenFrei.push_back(static_cast<ID3D12GraphicsCommandList*>(a.listen[i])); }
        g_abschickArbeitet = false;
        if (g_auftraege.empty()) { g_abschickLeer.notify_all(); }
    }
}

void auftragGeben(const Auftrag& a) {
    if (!g_abschickEigen) {
        auftragAusfuehren(a);
        for (UINT i = 0; i < a.n; ++i) { g_listenFrei.push_back(static_cast<ID3D12GraphicsCommandList*>(a.listen[i])); }
        return;
    }
    {
        const std::lock_guard<std::mutex> sperre(g_abschickSperre);
        g_auftraege.push_back(a);
    }
    g_abschickWecken.notify_one();
}

// Einen Zaunwert hinter alles bisher Abgegebene setzen.
UINT64 zaunSetzen() {
    Auftrag a;
    a.zaun = ++g_zaunWert;
    auftragGeben(a);
    return a.zaun;
}

// Eine offene Liste auf diesem Befehlsspeicher: eine ausgefuehrte wieder oeffnen, sonst eine neue (der Abschickfaden haelt
// die letzte noch).
ID3D12GraphicsCommandList* listeNehmen(ID3D12CommandAllocator* speicher) {
    ID3D12GraphicsCommandList* l = nullptr;
    {
        const std::lock_guard<std::mutex> sperre(g_abschickSperre);
        if (!g_listenFrei.empty()) {
            l = g_listenFrei.back();
            g_listenFrei.pop_back();
        }
    }
    if (l != nullptr) {
        l->Reset(speicher, nullptr);
        return l;
    }
    if (SUCCEEDED(g_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, speicher, nullptr, IID_PPV_ARGS(&l))) && l != nullptr) { return l; }
    // nicht anlegbar: auf die ausgefuehrten warten
    abschickenAbwarten();
    const std::lock_guard<std::mutex> sperre(g_abschickSperre);
    if (g_listenFrei.empty()) { return nullptr; }
    l = g_listenFrei.back();
    g_listenFrei.pop_back();
    l->Reset(speicher, nullptr);
    return l;
}

// Der Hauptfaden wartet hoechstens zwei Sekunden am Stueck auf den Abschickfaden oder einen Zaunwert. DXGI kann aus Present
// heraus eine Nachricht an den Faden des Fensters SENDEN (Microsoft, "Multithreading and DXGI") - der wartet dann gerade auf
// eben dieses Present. Deshalb nach jedem abgelaufenen Warten die gesendeten Nachrichten abarbeiten (nur die: PM_QS_SENDMESSAGE).
void gesendeteNachrichten() {
    MSG m;
    (void)PeekMessageW(&m, nullptr, 0, 0, PM_NOREMOVE | PM_QS_SENDMESSAGE);
}

void abschickFadenBeenden() {
    if (!g_abschickEigen) { return; }
    {
        const std::lock_guard<std::mutex> sperre(g_abschickSperre);
        g_abschickEnde = true;
    }
    g_abschickWecken.notify_one();
    if (g_abschickFaden.joinable()) { g_abschickFaden.join(); }
    g_abschickEigen = false;
    g_abschickEnde = false;
}

// --- Warten ------------------------------------------------------------------

// Geraet verloren: Present meldet es (zeigen), und ein Warten, das zwei Sekunden lang keinen Zaunwert sieht, fragt den Treiber.
// Nicht bei jeder Frage nach einem Zaunwert fragen - GetDeviceRemovedReason ist ein Kernaufruf, und gefragt wird mehrmals je Bild.
bool g_geraetWeg = false;
bool geraetWeg() { return g_geraetWeg; }
bool geraetWegFragen() {
    if (!g_geraetWeg && g_dev != nullptr && g_dev->GetDeviceRemovedReason() != S_OK) { g_geraetWeg = true; }
    return g_geraetWeg;
}

void wartenAuf(UINT64 wert) {
    if (g_zaun == nullptr || g_geraetWeg || g_zaun->GetCompletedValue() >= wert) { return; }
    LARGE_INTEGER t0{};
    QueryPerformanceCounter(&t0);
    g_zaun->SetEventOnCompletion(wert, g_zaunEreignis);
    // Hoechstens zwei Sekunden am Stueck: ein verlorenes Geraet erreicht den Zaun nie (Present meldet es dann).
    while (WaitForSingleObject(g_zaunEreignis, 2000) == WAIT_TIMEOUT) {
        if (geraetWegFragen()) { return; }
        gesendeteNachrichten();
    }
    LARGE_INTEGER t1{};
    LARGE_INTEGER takt{};
    QueryPerformanceCounter(&t1);
    QueryPerformanceFrequency(&takt);
    ++g_statistik.gewartet;
    const double ms = static_cast<double>(t1.QuadPart - t0.QuadPart) * 1000.0 / static_cast<double>(takt.QuadPart);
    g_statistik.warteMs += ms;
    if (ms > 250.0) {
        char z[200];
        std::snprintf(z, sizeof(z), "D3D12: %.0f ms auf den Zaunwert %llu gewartet (zuletzt abgeschickt %llu)", ms, static_cast<unsigned long long>(wert),
                      static_cast<unsigned long long>(g_letzterZaun));
        diag::detail(z);
    }
}

// Was an einem erreichten Zaunwert haengt, freigeben.
void einsammeln() {
    if (g_zaun == nullptr) { return; }
    const UINT64 erreicht = g_zaun->GetCompletedValue();
    while (!g_spaeter.empty() && (g_spaeter.front().zaun <= erreicht || geraetWeg())) {
        for (IUnknown* o : g_spaeter.front().objekte) { o->Release(); }
        for (const UINT i : g_spaeter.front().srv) { g_srvFrei.push_back(i); }
        g_spaeter.pop_front();
    }
}

// --- Deskriptoren und Hochladen -------------------------------------------------

UINT srvNehmen() {
    if (!g_srvFrei.empty()) {
        const UINT i = g_srvFrei.back();
        g_srvFrei.pop_back();
        return i;
    }
    return g_srvNaechster < kDeskriptoren ? g_srvNaechster++ : kDeskriptoren;
}

D3D12_CPU_DESCRIPTOR_HANDLE srvCpu(UINT i) {
    D3D12_CPU_DESCRIPTOR_HANDLE h = g_srvCpu0;
    h.ptr += static_cast<SIZE_T>(i) * g_srvSchritt;
    return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE srvGpu(UINT i) {
    D3D12_GPU_DESCRIPTOR_HANDLE h = g_srvGpu0;
    h.ptr += static_cast<UINT64>(i) * g_srvSchritt;
    return h;
}

UINT srvIndex(D3D12_CPU_DESCRIPTOR_HANDLE h) { return static_cast<UINT>((h.ptr - g_srvCpu0.ptr) / g_srvSchritt); }

void imguiSrvNehmen(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu) {
    const UINT i = srvNehmen();
    *cpu = srvCpu(i);
    *gpu = srvGpu(i);
}

void imguiSrvFrei(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE) { g_srvFrei.push_back(srvIndex(cpu)); }

void hochladeFrei(std::vector<Hochlade>& v) {
    // Den ersten Hochladepuffer behalten (der Normalfall passt hinein), die uebrigen freigeben. Ein uebergrosser erster
    // (eine einzelne grosse Textur beim Laden) bleibt nicht liegen.
    std::size_t behalten = 0;
    if (!v.empty() && v[0].groesse <= kHochladeSeite) {
        v[0].belegt = 0;
        behalten = 1;
    }
    for (std::size_t i = behalten; i < v.size(); ++i) { v[i].puffer->Release(); }
    v.resize(behalten);
}

// Platz im Hochladepuffer des laufenden Bildplatzes (Ausrichtung 512, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT).
bool hochladePlatzIn(std::vector<Hochlade>& v, UINT64 groesse, ID3D12Resource*& puffer, UINT64& versatz, unsigned char*& zeiger) {
    constexpr UINT64 kAusrichtung = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
    if (v.empty() || ((v.back().belegt + kAusrichtung - 1) & ~(kAusrichtung - 1)) + groesse > v.back().groesse) {
        Hochlade neu;
        neu.groesse = std::max<UINT64>(groesse, kHochladeSeite);
        D3D12_HEAP_PROPERTIES hp{};
        hp.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC rd{};
        rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width = neu.groesse;
        rd.Height = 1;
        rd.DepthOrArraySize = 1;
        rd.MipLevels = 1;
        rd.SampleDesc.Count = 1;
        rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if (FAILED(g_dev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                  IID_PPV_ARGS(&neu.puffer)))) {
            return false;
        }
        const D3D12_RANGE nichts{0, 0};
        void* z = nullptr;
        if (FAILED(neu.puffer->Map(0, &nichts, &z))) {
            neu.puffer->Release();
            return false;
        }
        neu.zeiger = static_cast<unsigned char*>(z);
        ++g_statistik.hochladePuffer;
        v.push_back(neu);
    }
    Hochlade& h = v.back();
    versatz = (h.belegt + kAusrichtung - 1) & ~(kAusrichtung - 1);
    h.belegt = versatz + groesse;
    puffer = h.puffer;
    zeiger = h.zeiger + versatz;
    return true;
}

void barriere(ID3D12GraphicsCommandList* l, ID3D12Resource* r, D3D12_RESOURCE_STATES vor, D3D12_RESOURCE_STATES nach) {
    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = r;
    b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = vor;
    b.Transition.StateAfter = nach;
    l->ResourceBarrier(1, &b);
}

// Rechteck [x0, x1) x [y0, y1) aus rgba (Breite w) in die oberste Stufe von res (Zustand vorher/nachher: Pixel-Shader).
bool hochladen(ID3D12Resource* res, const unsigned char* rgba, int w, int x0, int y0, int x1, int y1, bool neu) {
    const UINT bw = static_cast<UINT>(x1 - x0);
    const UINT bh = static_cast<UINT>(y1 - y0);
    const UINT zeile = (bw * 4U + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1U) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1U);
    ID3D12Resource* puffer = nullptr;
    std::uint64_t versatz = 0;
    unsigned char* ziel = nullptr;
    if (!rahmen::hochladePlatz(static_cast<UINT64>(zeile) * bh, true, puffer, versatz, ziel)) { return false; }
    for (UINT y = 0; y < bh; ++y) {
        std::memcpy(ziel + static_cast<std::size_t>(y) * zeile,
                    rgba + ((static_cast<std::size_t>(y0) + y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x0)) * 4U,
                    static_cast<std::size_t>(bw) * 4U);
    }
    ID3D12GraphicsCommandList* l = rahmen::vorListe();
    if (!neu) { barriere(l, res, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST); }
    D3D12_TEXTURE_COPY_LOCATION q{};
    q.pResource = puffer;
    q.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    q.PlacedFootprint.Offset = versatz;
    q.PlacedFootprint.Footprint.Format = kFormat;
    q.PlacedFootprint.Footprint.Width = bw;
    q.PlacedFootprint.Footprint.Height = bh;
    q.PlacedFootprint.Footprint.Depth = 1;
    q.PlacedFootprint.Footprint.RowPitch = zeile;
    D3D12_TEXTURE_COPY_LOCATION z{};
    z.pResource = res;
    z.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    z.SubresourceIndex = 0;
    l->CopyTextureRegion(&z, static_cast<UINT>(x0), static_cast<UINT>(y0), 0, &q, nullptr);
    barriere(l, res, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    return true;
}

ID3D12Resource* textur2d(UINT w, UINT h, DXGI_FORMAT f, D3D12_RESOURCE_FLAGS fahnen, D3D12_RESOURCE_STATES zustand,
                         const D3D12_CLEAR_VALUE* loeschen = nullptr) {
    D3D12_HEAP_PROPERTIES hp{};
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC rd{};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rd.Width = w;
    rd.Height = h;
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;
    rd.Format = f;
    rd.SampleDesc.Count = 1;
    rd.Flags = fahnen;
    ID3D12Resource* r = nullptr;
    if (FAILED(g_dev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, zustand, loeschen, IID_PPV_ARGS(&r)))) { return nullptr; }
    return r;
}

void sichtAnlegen(ID3D12Resource* r, DXGI_FORMAT f, UINT index) {
    D3D12_SHADER_RESOURCE_VIEW_DESC s{};
    s.Format = f;
    s.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    s.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    s.Texture2D.MipLevels = 1;   // wie imgui_impl_dx11: MaxLOD 0 - nur die oberste Stufe
    g_dev->CreateShaderResourceView(r, &s, srvCpu(index));
}

// --- Hintergrundpuffer --------------------------------------------------------

void hintenHolen() {
    for (UINT i = 0; i < kPuffer; ++i) {
        if (FAILED(g_swap->GetBuffer(i, IID_PPV_ARGS(&g_hinten[i])))) { continue; }
        D3D12_CPU_DESCRIPTOR_HANDLE h = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        h.ptr += static_cast<SIZE_T>(i) * g_rtvSchritt;
        g_dev->CreateRenderTargetView(g_hinten[i], nullptr, h);
    }
}

void hintenFrei() {
    for (ID3D12Resource*& r : g_hinten) { frei(r); }
}

D3D12_CPU_DESCRIPTOR_HANDLE rtv(UINT i) {
    D3D12_CPU_DESCRIPTOR_HANDLE h = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<SIZE_T>(i) * g_rtvSchritt;
    return h;
}

// --- Bildplaetze ----------------------------------------------------------------

void platzFrei(Platz& p) {
    hochladeFrei(p.hoch);
    hochladeFrei(p.hochVor);
    p.vorBytes = 0;
}

void naechsterPlatz() {
    g_platz = (g_platz + 1U) % kPuffer;
    Platz& p = g_plaetze[g_platz];
    wartenAuf(p.zaun);
    p.haupt->Reset();
    p.vor->Reset();
    platzFrei(p);
}

// --- Debugschicht -----------------------------------------------------------------

void debugMeldungen() {
    if (g_info == nullptr) { return; }
    const UINT64 n = g_info->GetNumStoredMessages();
    for (UINT64 i = 0; i < n; ++i) {
        SIZE_T laenge = 0;
        if (FAILED(g_info->GetMessage(i, nullptr, &laenge)) || laenge == 0) { continue; }
        std::vector<unsigned char> puffer(laenge);
        auto* m = reinterpret_cast<D3D12_MESSAGE*>(puffer.data());
        if (FAILED(g_info->GetMessage(i, m, &laenge))) { continue; }
        if (m->Severity > D3D12_MESSAGE_SEVERITY_WARNING) { continue; }
        const std::string text(m->pDescription, m->DescriptionByteLength > 0 ? m->DescriptionByteLength - 1 : 0);
        if (!g_infoGesehen.insert(text).second || g_infoGesehen.size() > 200U) { continue; }
        if (m->Severity <= D3D12_MESSAGE_SEVERITY_ERROR) {
            ++g_debugFehler;
            diag::error("D3D12-Debugschicht: " + text);
        } else {
            diag::detail("D3D12-Debugschicht (Warnung): " + text);
        }
    }
    g_info->ClearStoredMessages();
}

// --- Mischarten -----------------------------------------------------------------
//
// Die Shader und Zustaende von imgui_impl_dx12 (CreateDeviceObjects), nur die Mischung und der Abtaster anders - wie die
// D3D11-Rueckrufe in backend_win32.cpp (additivD3d, stufeD3d) und kartegpu::ohneMischen.

const char* kVs =
    "cbuffer vertexBuffer : register(b0) { float4x4 ProjectionMatrix; };"
    "struct VS_INPUT { float2 pos : POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };"
    "struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };"
    "PS_INPUT main(VS_INPUT input) { PS_INPUT output;"
    " output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.0f, 1.0f)); output.col = input.col; output.uv = input.uv;"
    " return output; }";
const char* kPs =
    "struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };"
    "SamplerState sampler0 : register(s0); Texture2D texture0 : register(t0);"
    "float4 main(PS_INPUT input) : SV_Target { float4 out_col = input.col * texture0.Sample(sampler0, input.uv); return out_col; }";

ID3D12RootSignature* wurzel(D3D12_TEXTURE_ADDRESS_MODE art) {
    D3D12_DESCRIPTOR_RANGE bereich{};
    bereich.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    bereich.NumDescriptors = 1;
    D3D12_ROOT_PARAMETER p[2] = {};
    p[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    p[0].Constants.Num32BitValues = 16;
    p[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    p[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    p[1].DescriptorTable.NumDescriptorRanges = 1;
    p[1].DescriptorTable.pDescriptorRanges = &bereich;
    p[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_STATIC_SAMPLER_DESC s{};
    s.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    s.AddressU = s.AddressV = s.AddressW = art;
    s.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    s.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    s.MaxLOD = D3D12_FLOAT32_MAX;
    s.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC d{};
    d.NumParameters = 2;
    d.pParameters = p;
    d.NumStaticSamplers = 1;
    d.pStaticSamplers = &s;
    d.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
              D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    ID3DBlob* blob = nullptr;
    if (FAILED(D3D12SerializeRootSignature(&d, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr)) || blob == nullptr) { return nullptr; }
    ID3D12RootSignature* rs = nullptr;
    g_dev->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rs));
    blob->Release();
    return rs;
}

D3D12_BLEND faktor12(int f) {
    static const D3D12_BLEND t[] = {D3D12_BLEND_ONE,        D3D12_BLEND_ZERO,           D3D12_BLEND_SRC_COLOR,  D3D12_BLEND_INV_SRC_COLOR,
                                    D3D12_BLEND_SRC_ALPHA,  D3D12_BLEND_INV_SRC_ALPHA,  D3D12_BLEND_DEST_COLOR, D3D12_BLEND_INV_DEST_COLOR,
                                    D3D12_BLEND_DEST_ALPHA, D3D12_BLEND_INV_DEST_ALPHA, D3D12_BLEND_SRC_ALPHA};
    return (f >= 0 && f < 11) ? t[f] : D3D12_BLEND_ONE;
}

// Schluessel: Bits 0-15 Rueckrufdaten (Art in 16-17, Quelle 0-7, Ziel 8-15, klemmen 18)
ID3D12PipelineState* pso(int schluessel) {
    if (const auto it = g_pso.find(schluessel); it != g_pso.end()) { return it->second; }
    ID3D12PipelineState*& aus = g_pso[schluessel];
    if (g_vs == nullptr && FAILED(D3DCompile(kVs, std::strlen(kVs), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &g_vs, nullptr))) {
        return nullptr;
    }
    if (g_ps == nullptr && FAILED(D3DCompile(kPs, std::strlen(kPs), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &g_ps, nullptr))) {
        return nullptr;
    }
    const int art = (schluessel >> 16) & 3;
    const bool wieder = art == 1 && (schluessel & (1 << 18)) == 0;
    static const D3D12_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, static_cast<UINT>(offsetof(ImDrawVert, pos)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, static_cast<UINT>(offsetof(ImDrawVert, uv)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, static_cast<UINT>(offsetof(ImDrawVert, col)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC d{};
    d.NodeMask = 1;
    d.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    d.pRootSignature = wieder ? g_rsWieder : g_rsKlemm;
    d.SampleMask = UINT_MAX;
    d.NumRenderTargets = 1;
    d.RTVFormats[0] = kFormat;
    d.SampleDesc.Count = 1;
    d.VS = {g_vs->GetBufferPointer(), g_vs->GetBufferSize()};
    d.PS = {g_ps->GetBufferPointer(), g_ps->GetBufferSize()};
    d.InputLayout = {layout, 3};
    D3D12_RENDER_TARGET_BLEND_DESC& b = d.BlendState.RenderTarget[0];
    b.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    b.BlendOp = D3D12_BLEND_OP_ADD;
    b.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    b.LogicOp = D3D12_LOGIC_OP_NOOP;
    if (art == 0) {   // additivD3d: GL_ONE GL_ONE, Alpha ebenso
        b.BlendEnable = TRUE;
        b.SrcBlend = b.DestBlend = b.SrcBlendAlpha = b.DestBlendAlpha = D3D12_BLEND_ONE;
    } else if (art == 1) {   // stufeD3d: blendFunc der Stufe, Alpha ONE / INV_SRC_ALPHA
        b.BlendEnable = TRUE;
        b.SrcBlend = faktor12(schluessel & 0xff);
        b.DestBlend = faktor12((schluessel >> 8) & 0xff);
        b.SrcBlendAlpha = D3D12_BLEND_ONE;
        b.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    } else {   // ohneMischen
        b.BlendEnable = FALSE;
        b.SrcBlend = b.SrcBlendAlpha = D3D12_BLEND_ONE;
        b.DestBlend = b.DestBlendAlpha = D3D12_BLEND_ZERO;
    }
    D3D12_RASTERIZER_DESC& r = d.RasterizerState;
    r.FillMode = D3D12_FILL_MODE_SOLID;
    r.CullMode = D3D12_CULL_MODE_NONE;
    r.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    r.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    r.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    r.DepthClipEnable = TRUE;
    D3D12_DEPTH_STENCIL_DESC& t = d.DepthStencilState;
    t.DepthEnable = FALSE;
    t.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    t.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    t.FrontFace.StencilFailOp = t.FrontFace.StencilDepthFailOp = t.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    t.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    t.BackFace = t.FrontFace;
    if (FAILED(g_dev->CreateGraphicsPipelineState(&d, IID_PPV_ARGS(&aus)))) { aus = nullptr; }
    return aus;
}

// Der Rueckruf in der ImGui-Liste: Wurzelsignatur und Zustand setzen, dann die Projektion wie
// ImGui_ImplDX12_SetupRenderState (dieselbe Rechnung, dieselben Zahlen).
void mischRueckruf(const ImDrawList* liste, const ImDrawCmd* befehl) {
    const auto* zustand = static_cast<const ImGui_ImplDX12_RenderState*>(ImGui::GetPlatformIO().Renderer_RenderState);
    if (zustand == nullptr) { return; }
    const int schluessel = static_cast<int>(reinterpret_cast<std::intptr_t>(befehl->UserCallbackData));
    ID3D12PipelineState* p = pso(schluessel);
    if (p == nullptr) { return; }
    const ImDrawData* dd = nullptr;
    for (const ImGuiViewport* v : ImGui::GetPlatformIO().Viewports) {
        if (v->DrawData == nullptr) { continue; }
        for (const ImDrawList* l : v->DrawData->CmdLists) {
            if (l == liste) { dd = v->DrawData; }
        }
    }
    if (dd == nullptr) { return; }
    const int art = (schluessel >> 16) & 3;
    const bool wieder = art == 1 && (schluessel & (1 << 18)) == 0;
    ID3D12GraphicsCommandList* l = zustand->CommandList;
    l->SetGraphicsRootSignature(wieder ? g_rsWieder : g_rsKlemm);
    l->SetPipelineState(p);
    const float L = dd->DisplayPos.x;
    const float R = dd->DisplayPos.x + dd->DisplaySize.x;
    const float T = dd->DisplayPos.y;
    const float B = dd->DisplayPos.y + dd->DisplaySize.y;
    const float mvp[4][4] = {
        {2.0F / (R - L), 0.0F, 0.0F, 0.0F},
        {0.0F, 2.0F / (T - B), 0.0F, 0.0F},
        {0.0F, 0.0F, 0.5F, 0.0F},
        {(R + L) / (L - R), (T + B) / (B - T), 0.5F, 1.0F},
    };
    l->SetGraphicsRoot32BitConstants(0, 16, mvp, 0);
}

// --- Lesen ------------------------------------------------------------------------

// Eine Textur (Zustand `zustand`) ganz in den Speicher lesen: ans Ende der Befehlsliste, abschicken, auf die Grafikkarte warten.
bool lesen(ID3D12Resource* r, D3D12_RESOURCE_STATES zustand, Bild& aus) {
    const D3D12_RESOURCE_DESC rd = r->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};
    UINT64 groesse = 0;
    g_dev->GetCopyableFootprints(&rd, 0, 1, 0, &fp, nullptr, nullptr, &groesse);
    D3D12_HEAP_PROPERTIES hp{};
    hp.Type = D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC bd{};
    bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bd.Width = groesse;
    bd.Height = 1;
    bd.DepthOrArraySize = 1;
    bd.MipLevels = 1;
    bd.SampleDesc.Count = 1;
    bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ID3D12Resource* puffer = nullptr;
    if (FAILED(g_dev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&puffer)))) {
        return false;
    }
    ID3D12GraphicsCommandList* l = rahmen::liste();
    barriere(l, r, zustand, D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION z{};
    z.pResource = puffer;
    z.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    z.PlacedFootprint = fp;
    D3D12_TEXTURE_COPY_LOCATION q{};
    q.pResource = r;
    q.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    l->CopyTextureRegion(&z, 0, 0, 0, &q, nullptr);
    barriere(l, r, D3D12_RESOURCE_STATE_COPY_SOURCE, zustand);
    wartenAuf(rahmen::abschicken());
    void* daten = nullptr;
    const D3D12_RANGE alles{0, static_cast<SIZE_T>(groesse)};
    const bool ok = SUCCEEDED(puffer->Map(0, &alles, &daten));
    if (ok) {
        aus.w = static_cast<std::uint32_t>(rd.Width);
        aus.h = rd.Height;
        aus.rgba.resize(static_cast<std::size_t>(aus.w) * aus.h * 4U);
        for (std::uint32_t y = 0; y < aus.h; ++y) {
            std::memcpy(aus.rgba.data() + static_cast<std::size_t>(y) * aus.w * 4U,
                        static_cast<const unsigned char*>(daten) + fp.Offset + static_cast<std::size_t>(y) * fp.Footprint.RowPitch,
                        static_cast<std::size_t>(aus.w) * 4U);
        }
        const D3D12_RANGE nichts{0, 0};
        puffer->Unmap(0, &nichts);
    }
    puffer->Release();
    return ok;
}

// --- Bilder der 3D-Ansichten in der Oberflaeche ---------------------------------------
//
// Die 3D-Ansichten geben ImGui ihre Sicht (ID3D11ShaderResourceView* des Abgebers) als Texturkennung. imgui_impl_dx12 erwartet
// die GPU-Adresse eines Deskriptors im gemeinsamen Stapel - die hat jede Sicht fest. Hier wird nur die Kennung ersetzt und die
// Textur in der Befehlsliste lesbar gestellt (sie war eben noch Renderziel). bemalt: Ressourcen, die in diesem Bild bemalt
// wurden (backend_win32.cpp) - fuer frisch().
void umschreiben(ImDrawData* dd, const std::vector<const void*>& bemalt) {
    if (dd == nullptr || !dd->Valid) { return; }
    ID3D12GraphicsCommandList* l = nullptr;
    for (ImDrawList* liste : dd->CmdLists) {
        for (ImDrawCmd& c : liste->CmdBuffer) {
            if (c.UserCallback != nullptr || c.TexRef._TexData != nullptr) { continue; }
            const ImTextureID id = c.TexRef._TexID;
            if (id == ImTextureID_Invalid || !abgeber::istSicht(static_cast<std::uint64_t>(id))) { continue; }
            if (l == nullptr) { l = rahmen::liste(); }
            const void* res = nullptr;
            const std::uint64_t kennung = abgeber::sichtZeigen(static_cast<std::uint64_t>(id), l, &res);
            if (res != nullptr && std::find(bemalt.begin(), bemalt.end(), res) != bemalt.end()) { g_frisch.insert(kennung); }
            c.TexRef._TexID = static_cast<ImTextureID>(kennung);
        }
    }
}

void texturenLoesen() {
    if (g_queue != nullptr) {
        wartenAuf(rahmen::abschicken());
        abschickenAbwarten();
    }
    for (auto& [k, t] : g_texturen) {
        (void)k;
        t.res->Release();
        g_srvFrei.push_back(t.index);
    }
    g_texturen.clear();
}

// Die Einstiege, die dieser Weg braucht: d3d12.dll (Windows 10), CreateDXGIFactory2 in dxgi.dll (Windows 8.1). Beide sind
// verzoegert gebunden (CMakeLists.txt) - ein fehlender Einstieg waere beim ersten Aufruf eine Ausnahme statt eines Rueckfalls
// auf Direct3D 11. Deshalb vorher fragen.
bool einstiegeDa() {
    const auto hat = [](const wchar_t* dll, const char* einstieg) {
        const HMODULE m = LoadLibraryExW(dll, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        return m != nullptr && GetProcAddress(m, einstieg) != nullptr;
    };
    return hat(L"d3d12.dll", "D3D12CreateDevice") && hat(L"d3d12.dll", "D3D12SerializeRootSignature") && hat(L"dxgi.dll", "CreateDXGIFactory2");
}

// Selbsttest des Rueckfalls: BHED_D3D12_FEHLER=<stelle> laesst den Start an dieser Stelle scheitern, als fehlte dem
// Rechner das Noetige (dll, geraet, swap, abgeber, imgui) - danach muss Direct3D 11 tragen.
bool fehlerSpielen(const char* stelle) {
    const char* f = std::getenv("BHED_D3D12_FEHLER");
    if (f == nullptr || std::strcmp(f, stelle) != 0) { return false; }
    diag::detail(std::string("D3D12: Fehler gespielt an der Stelle \"") + stelle + "\" (BHED_D3D12_FEHLER)");
    return true;
}

}  // namespace

// ===================================================================================
// Dienste fuer den Abgeber (abgeber12.h)

namespace rahmen {

ID3D12Device* geraet() { return g_dev; }
ID3D12CommandQueue* schlange() { return g_queue; }

ID3D12GraphicsCommandList* liste() {
    if (!g_hauptOffen) {
        g_liste = listeNehmen(g_plaetze[g_platz].haupt);
        g_hauptOffen = g_liste != nullptr;
        if (g_hauptOffen) { abgeber::listeNeu(g_liste); }
    }
    return g_liste;
}

ID3D12GraphicsCommandList* vorListe() {
    if (!g_vorOffen) {
        g_vorListe = listeNehmen(g_plaetze[g_platz].vor);
        g_vorOffen = g_vorListe != nullptr;
    }
    return g_vorListe;
}

bool hochladePlatz(std::uint64_t groesse, bool vor, ID3D12Resource*& puffer, std::uint64_t& versatz, unsigned char*& zeiger) {
    Platz& p = g_plaetze[g_platz];
    if (vor) {
        // Viel hochgeladen, ohne dass ein Bild gezeigt wurde (Laden einer Karte): abschicken, warten, den Speicher wiederverwenden.
        if (p.vorBytes > kVorGrenze) {
            wartenAuf(abschicken());
            platzFrei(p);
        }
        p.vorBytes += groesse;
    }
    g_statistik.hochladeBytes += groesse;
    return hochladePlatzIn(vor ? p.hochVor : p.hoch, groesse, puffer, versatz, zeiger);
}

ID3D12DescriptorHeap* srvStapel() { return g_srvHeap; }
UINT srvNehmen() { return ::bhed::render::d3d12::srvNehmen(); }
UINT srvKeiner() { return kDeskriptoren; }
void srvSpaeterFrei(UINT index) {
    if (index < kDeskriptoren && g_srvHeap != nullptr) { g_srvOffen.push_back(index); }
}
D3D12_CPU_DESCRIPTOR_HANDLE srvCpu(UINT index) { return ::bhed::render::d3d12::srvCpu(index); }
D3D12_GPU_DESCRIPTOR_HANDLE srvGpu(UINT index) { return ::bhed::render::d3d12::srvGpu(index); }

void spaeterFrei(IUnknown* objekt) {
    if (objekt == nullptr) { return; }
    if (g_queue == nullptr) {   // nach dem Abbau: niemand liest es mehr
        objekt->Release();
        return;
    }
    // Keine Liste offen: nur schon abgeschickte Listen koennen es nennen. Ist die Grafikkarte damit fertig (der Normalfall im
    // Leerlauf, wo kein Bild mehr kommt, das spaeter einsammeln koennte), sofort freigeben - wie Direct3D 11.
    // g_zaunWert ist das letzte Signal in der Warteschlange - es liegt hinter allen bisher abgeschickten Listen, auch denen der
    // eigenen Fenster (imgui_impl_dx12).
    if (!g_hauptOffen && !g_vorOffen && g_objekteOffen.empty()) {
        if (g_zaun == nullptr || g_zaun->GetCompletedValue() >= g_zaunWert) {
            objekt->Release();
            return;
        }
        if (g_spaeter.empty() || g_spaeter.back().zaun != g_zaunWert) {
            Spaeter s;
            s.zaun = g_zaunWert;
            g_spaeter.push_back(std::move(s));
        }
        g_spaeter.back().objekte.push_back(objekt);
        return;
    }
    g_objekteOffen.push_back(objekt);
}

std::uint64_t abschicken() {
    Auftrag a;
    if (g_vorOffen) {
        g_vorListe->Close();
        a.listen[a.n++] = g_vorListe;
        g_vorListe = nullptr;
        g_vorOffen = false;
    }
    if (g_hauptOffen) {
        abgeber::vorAbschicken(g_liste);
        g_liste->Close();
        a.listen[a.n++] = g_liste;
        g_liste = nullptr;
        g_hauptOffen = false;
    }
    if (a.n == 0U) { return g_letzterZaun; }
    a.zaun = ++g_zaunWert;
    auftragGeben(a);
    g_letzterZaun = g_zaunWert;
    ++g_ausfuehrung;
    ++g_statistik.abgeschickt;
    if (!g_objekteOffen.empty() || !g_srvOffen.empty()) {
        Spaeter s;
        s.zaun = g_zaunWert;
        s.objekte.swap(g_objekteOffen);
        s.srv.swap(g_srvOffen);
        g_spaeter.push_back(std::move(s));
    }
    abgeber::nachAbschicken(g_zaunWert);
    return g_zaunWert;
}

std::uint64_t letzterZaun() { return g_letzterZaun; }
void warten(std::uint64_t zaun) { wartenAuf(zaun); }
bool fertig(std::uint64_t zaun) { return g_zaun == nullptr || g_zaun->GetCompletedValue() >= zaun || geraetWeg(); }
std::uint64_t ausfuehrung() { return g_ausfuehrung; }
Statistik statistik() {
    Statistik s = g_statistik;
    for (const Platz& p : g_plaetze) {
        for (const Hochlade& h : p.hoch) { s.hochladeJetzt += h.groesse; }
        for (const Hochlade& h : p.hochVor) { s.hochladeJetzt += h.groesse; }
    }
    s.spaeter = g_objekteOffen.size();
    for (const Spaeter& sp : g_spaeter) { s.spaeter += sp.objekte.size(); }
    return s;
}

}  // namespace rahmen

// ===================================================================================

namespace {
std::thread g_vorFaden;
ID3D12Device* g_vorGeraet = nullptr;   // haelt das vorgewaermte Geraet, bis init() seines hat
bool g_vorDebug = false;               // der Faden hat die Debugschicht eingeschaltet
double g_vorMs = 0.0;
}  // namespace

void vorwaermen(IDXGIAdapter1* (*karteHolen)(), bool debug) {
    if (g_dev != nullptr || g_vorGeraet != nullptr || g_vorFaden.joinable()) { return; }
    g_vorFaden = std::thread([karteHolen, debug] {
        LARGE_INTEGER t0{};
        LARGE_INTEGER t1{};
        LARGE_INTEGER takt{};
        QueryPerformanceFrequency(&takt);
        QueryPerformanceCounter(&t0);
        if (!einstiegeDa()) { return; }
        if (debug) {   // die Debugschicht muss vor dem Geraet stehen
            ID3D12Debug* dbg = nullptr;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg))) && dbg != nullptr) {
                dbg->EnableDebugLayer();
                dbg->Release();
                g_vorDebug = true;
            }
        }
        IDXGIAdapter1* karte = karteHolen != nullptr ? karteHolen() : nullptr;
        if (FAILED(D3D12CreateDevice(karte, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_vorGeraet)))) { g_vorGeraet = nullptr; }
        if (karte != nullptr) { karte->Release(); }
        QueryPerformanceCounter(&t1);
        g_vorMs = static_cast<double>(t1.QuadPart - t0.QuadPart) * 1000.0 / static_cast<double>(takt.QuadPart);
    });
}

bool init(HWND hwnd, IDXGIAdapter1* karte, bool debug) {
    // Vorgewaermt (vorwaermen): auf den Faden warten. Sein Geraet bleibt gehalten, bis das eigene D3D12CreateDevice unten es
    // bekommen hat (dasselbe Einzelstueck), und geht am Ende von init() in jedem Fall zurueck.
    if (g_vorFaden.joinable()) { g_vorFaden.join(); }
    struct VorHalter {
        ~VorHalter() { frei(g_vorGeraet); }
    } vorHalter;
    const double vorMs = g_vorMs;
    g_vorMs = 0.0;
    if (debug != g_vorDebug) { frei(g_vorGeraet); }   // andere Wahl der Debugschicht: nicht dieses Geraet
    // Zeiten des Starts (Messhilfe): je Schritt die Millisekunden seit dem vorigen, am Ende eine Zeile im Protokoll.
    LARGE_INTEGER startTakt{};
    LARGE_INTEGER startZuletzt{};
    QueryPerformanceFrequency(&startTakt);
    QueryPerformanceCounter(&startZuletzt);
    const LARGE_INTEGER startAnfang = startZuletzt;
    std::string startZeiten;
    const auto schritt = [&](const char* was) {
        LARGE_INTEGER jetzt{};
        QueryPerformanceCounter(&jetzt);
        char z[64];
        std::snprintf(z, sizeof(z), "%s%s %.1f", startZeiten.empty() ? "" : ", ", was,
                      static_cast<double>(jetzt.QuadPart - startZuletzt.QuadPart) * 1000.0 / static_cast<double>(startTakt.QuadPart));
        startZeiten += z;
        startZuletzt = jetzt;
    };
    if (fehlerSpielen("dll") || !einstiegeDa()) {
        diag::detail("D3D12: d3d12.dll oder DXGI 1.3 fehlen");
        return false;
    }
    if (debug && !g_vorDebug) {   // (vorgewaermt: die Debugschicht steht schon - ein zweites Einschalten naehme das Geraet weg)
        ID3D12Debug* dbg = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg))) && dbg != nullptr) {
            dbg->EnableDebugLayer();
            dbg->Release();
        } else {
            debug = false;
        }
    }
    g_vorDebug = false;
    if (fehlerSpielen("geraet") || FAILED(D3D12CreateDevice(karte, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_dev))) || g_dev == nullptr) {
        diag::detail("D3D12: kein Geraet mit Merkmalsstufe 11_0");
        return false;
    }
    schritt("Geraet");
    g_luid = g_dev->GetAdapterLuid();
    if (debug && SUCCEEDED(g_dev->QueryInterface(IID_PPV_ARGS(&g_info))) && g_info != nullptr) {
        diag::detail("D3D12-Debugschicht AN");
    }
    D3D12_COMMAND_QUEUE_DESC qd{};
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(g_dev->CreateCommandQueue(&qd, IID_PPV_ARGS(&g_queue)))) { return false; }
    if (FAILED(g_dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_zaun)))) { return false; }
    g_zaunEreignis = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    // Swapchain wie Direct3D 11 (backend_win32.cpp, initD3D11): Flip, drei Puffer, wartbar, hoechstens zwei Bilder Vorlauf.
    IDXGIFactory4* fabrik = nullptr;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&fabrik))) || fabrik == nullptr) { return false; }
    DXGI_SWAP_CHAIN_DESC1 sd{};
    sd.BufferCount = kPuffer;
    sd.Format = kFormat;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.SampleDesc.Count = 1;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    IDXGISwapChain1* sc1 = nullptr;
    const HRESULT hrSwap = fabrik->CreateSwapChainForHwnd(g_queue, hwnd, &sd, nullptr, nullptr, &sc1);
    IDXGIAdapter1* ad = nullptr;
    if (SUCCEEDED(fabrik->EnumAdapterByLuid(g_luid, IID_PPV_ARGS(&ad))) && ad != nullptr) {
        DXGI_ADAPTER_DESC1 d{};
        if (SUCCEEDED(ad->GetDesc1(&d))) {
            for (const WCHAR* p = d.Description; *p != 0; ++p) { g_kartenName += (*p < 128) ? static_cast<char>(*p) : '?'; }
        }
        ad->Release();
    }
    fabrik->Release();
    if (FAILED(hrSwap) || sc1 == nullptr || fehlerSpielen("swap")) {
        if (sc1 != nullptr) { sc1->Release(); }
        diag::detail("D3D12: Swapchain abgelehnt");
        return false;
    }
    const HRESULT hrSc3 = sc1->QueryInterface(IID_PPV_ARGS(&g_swap));
    sc1->Release();
    if (FAILED(hrSc3)) { return false; }
    g_swapFlags = sd.Flags;
    g_swap->SetMaximumFrameLatency(2);
    g_bildFrei = g_swap->GetFrameLatencyWaitableObject();
    schritt("Warteschlange und Swapchain");

    D3D12_DESCRIPTOR_HEAP_DESC hd{};
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    hd.NumDescriptors = kPuffer + 1U;
    if (FAILED(g_dev->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&g_rtvHeap)))) { return false; }
    g_rtvSchritt = g_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    hd.NumDescriptors = kDeskriptoren;
    hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(g_dev->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&g_srvHeap)))) { return false; }
    g_srvSchritt = g_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    g_srvCpu0 = g_srvHeap->GetCPUDescriptorHandleForHeapStart();
    g_srvGpu0 = g_srvHeap->GetGPUDescriptorHandleForHeapStart();
    hintenHolen();

    for (Platz& p : g_plaetze) {
        if (FAILED(g_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&p.haupt))) ||
            FAILED(g_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&p.vor)))) {
            return false;
        }
    }
    // Zwei Listen im Vorrat (Bild und Vor-Liste); weitere entstehen, wenn der Abschickfaden beide noch haelt.
    for (int i = 0; i < 2; ++i) {
        ID3D12GraphicsCommandList* l = nullptr;
        if (FAILED(g_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, i == 0 ? g_plaetze[0].haupt : g_plaetze[0].vor, nullptr, IID_PPV_ARGS(&l))) ||
            l == nullptr) {
            return false;
        }
        l->Close();
        g_listenFrei.push_back(l);
    }

    schritt("Stapel und Listen");
    // Der Abgeber fuer die 3D-Ansichten: bedient ID3D11Device/ID3D11DeviceContext auf diesem Geraet (abgeber12.h).
    if (fehlerSpielen("abgeber") || !abgeber::anlegen()) {
        diag::detail("D3D12: der Abgeber der 3D-Ansichten liess sich nicht anlegen");
        return false;
    }

    g_rsKlemm = wurzel(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    g_rsWieder = wurzel(D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    if (g_rsKlemm == nullptr || g_rsWieder == nullptr) { return false; }

    ImGui_ImplDX12_InitInfo ii;
    ii.Device = g_dev;
    ii.CommandQueue = g_queue;
    ii.NumFramesInFlight = static_cast<int>(kPuffer);
    ii.RTVFormat = kFormat;
    ii.DSVFormat = DXGI_FORMAT_UNKNOWN;
    ii.SrvDescriptorHeap = g_srvHeap;
    ii.SrvDescriptorAllocFn = imguiSrvNehmen;
    ii.SrvDescriptorFreeFn = imguiSrvFrei;
    schritt("Abgeber");
    if (!ImGui_ImplDX12_Init(&ii) || fehlerSpielen("imgui")) { return false; }
    schritt("Oberflaeche");
    // BHED_D3D12_ABSCHICKEN=direkt: ohne den Abschickfaden (zum Vergleichsmessen)
    const char* direkt = std::getenv("BHED_D3D12_ABSCHICKEN");
    if (direkt == nullptr || std::strcmp(direkt, "direkt") != 0) {
        g_abschickEnde = false;
        g_abschickFaden = std::thread(abschickSchleife);
        g_abschickEigen = true;
    }
    diag::detail("Grafikkarte: " + g_kartenName + " (Direct3D 12, Flip-Modell)");
    {
        LARGE_INTEGER jetzt{};
        QueryPerformanceCounter(&jetzt);
        char z[64];
        std::snprintf(z, sizeof(z), "D3D12-Start %.1f ms: ", static_cast<double>(jetzt.QuadPart - startAnfang.QuadPart) * 1000.0 / static_cast<double>(startTakt.QuadPart));
        char v[80] = "";
        if (vorMs > 0.0) { std::snprintf(v, sizeof(v), " (Geraet vorgewaermt in einem eigenen Faden: %.1f ms)", vorMs); }
        diag::detail(z + startZeiten + v);
    }
    return true;
}

void imguiAbbauen() {
    if (g_queue != nullptr) {
        wartenAuf(rahmen::abschicken());
        abschickenAbwarten();
    }
    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().BackendRendererUserData != nullptr) { ImGui_ImplDX12_Shutdown(); }
}

void shutdown() {
    if (g_vorFaden.joinable()) { g_vorFaden.join(); }
    frei(g_vorGeraet);
    g_vorDebug = false;
    imguiAbbauen();
    texturenLoesen();
    abgeber::abbauen();
    if (g_queue != nullptr && g_zaun != nullptr) {
        // alles abgeschickt? dann warten, bis die Grafikkarte fertig ist, und freigeben, was daran haengt
        wartenAuf(rahmen::abschicken());
        wartenAuf(zaunSetzen());
    }
    abschickFadenBeenden();
    for (IUnknown* o : g_objekteOffen) { o->Release(); }
    g_objekteOffen.clear();
    g_srvOffen.clear();
    for (Spaeter& s : g_spaeter) {
        for (IUnknown* o : s.objekte) { o->Release(); }
    }
    g_spaeter.clear();
    for (auto& [k, p] : g_pso) {
        (void)k;
        if (p != nullptr) { p->Release(); }
    }
    g_pso.clear();
    frei(g_vs);
    frei(g_ps);
    frei(g_rsKlemm);
    frei(g_rsWieder);
    frei(g_liste);
    frei(g_vorListe);
    for (ID3D12GraphicsCommandList*& l : g_listenFrei) { frei(l); }
    g_listenFrei.clear();
    g_auftraege.clear();
    g_zeigenErgebnis.store(0);
    g_hauptOffen = false;
    g_vorOffen = false;
    for (Platz& p : g_plaetze) {
        frei(p.haupt);
        frei(p.vor);
        for (Hochlade& h : p.hoch) { h.puffer->Release(); }
        p.hoch.clear();
        for (Hochlade& h : p.hochVor) { h.puffer->Release(); }
        p.hochVor.clear();
        p.vorBytes = 0;
        p.zaun = 0;
    }
    g_platz = 0;
    hintenFrei();
    if (g_bildFrei != nullptr) {
        CloseHandle(g_bildFrei);
        g_bildFrei = nullptr;
    }
    frei(g_swap);
    frei(g_rtvHeap);
    frei(g_srvHeap);
    g_srvFrei.clear();
    g_srvNaechster = 0;
    frei(g_zaun);
    if (g_zaunEreignis != nullptr) {
        CloseHandle(g_zaunEreignis);
        g_zaunEreignis = nullptr;
    }
    g_zaunWert = 0;
    g_letzterZaun = 0;
    g_ausfuehrung = 0;
    frei(g_queue);
    frei(g_info);
    g_infoGesehen.clear();
    frei(g_dev);
    g_geraetWeg = false;
    g_frisch.clear();
    g_kartenName.clear();
}

void abschickenAbwarten() {
    if (!g_abschickEigen) { return; }
    std::unique_lock<std::mutex> sperre(g_abschickSperre);
    while (!g_abschickLeer.wait_for(sperre, std::chrono::seconds(2), [] { return g_auftraege.empty() && !g_abschickArbeitet; })) {
        sperre.unlock();
        gesendeteNachrichten();
        sperre.lock();
    }
}

ID3D11Device* geraet11() { return abgeber::geraet(); }
ID3D11DeviceContext* kontext11() { return abgeber::kontext(); }
HANDLE bildFrei() { return g_bildFrei; }

void newFrame() {
    // Was die Grafikkarte seit dem letzten Bild abgearbeitet hat, geht jetzt zurueck (freigegebene Texturen und Puffer, Bloecke):
    // am Ende des vorigen Bildes war dessen Befehlsliste eben erst abgeschickt. So ist der Speicher ein Bild frueher frei.
    einsammeln();
    abgeber::einsammeln();
    static bool erstes = true;   // das erste Bild legt die Shader und den Pipeline-Zustand der Oberflaeche an (Messhilfe)
    LARGE_INTEGER t0{};
    if (erstes) { QueryPerformanceCounter(&t0); }
    ImGui_ImplDX12_NewFrame();
    if (erstes) {
        erstes = false;
        LARGE_INTEGER t1{};
        LARGE_INTEGER takt{};
        QueryPerformanceCounter(&t1);
        QueryPerformanceFrequency(&takt);
        char z[96];
        std::snprintf(z, sizeof(z), "D3D12-Start: Shader und Zustand der Oberflaeche %.1f ms", static_cast<double>(t1.QuadPart - t0.QuadPart) * 1000.0 / static_cast<double>(takt.QuadPart));
        diag::detail(z);
    }
}

void vorbereiten(const std::vector<const void*>& bemalt) {
    for (ImGuiViewport* v : ImGui::GetPlatformIO().Viewports) { umschreiben(v->DrawData, bemalt); }
}

bool frisch(ImTextureID id) { return g_frisch.count(static_cast<UINT64>(id)) != 0U; }

void hauptfensterZeichnen(const float farbe[4], void (*vorAbschicken)()) {
    // Das vorige Present muss ausgefuehrt sein (der Hintergrundpuffer wechselt mit ihm), und imgui_impl_dx12 schickt neue
    // Schrifttexturen selbst ueber die Warteschlange. In aller Regel ist der Abschickfaden laengst fertig.
    abschickenAbwarten();
    const UINT i = g_swap->GetCurrentBackBufferIndex();
    ID3D12GraphicsCommandList* l = rahmen::liste();
    barriere(l, g_hinten[i], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    const D3D12_CPU_DESCRIPTOR_HANDLE ziel = rtv(i);
    l->OMSetRenderTargets(1, &ziel, FALSE, nullptr);
    abgeber::protokollNotiz(l, "Oberflaeche: Hauptfenster loeschen und zeichnen");
    l->ClearRenderTargetView(ziel, farbe, 0, nullptr);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), l);
    abgeber::fremdGezeichnet();
    abgeber::protokollNotiz(l, "Oberflaeche fertig");
    if (vorAbschicken != nullptr) { vorAbschicken(); }
    barriere(l, g_hinten[i], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    (void)rahmen::abschicken();
}

bool hauptfensterLesen(Bild& aus) {
    abschickenAbwarten();
    return lesen(g_hinten[g_swap->GetCurrentBackBufferIndex()], D3D12_RESOURCE_STATE_PRESENT, aus);
}

bool fensterLesen(ImGuiViewport* v, Bild& aus) {
    ImDrawData* dd = v->DrawData;
    if (dd == nullptr || !dd->Valid) { return false; }
    const UINT w = static_cast<UINT>(dd->DisplaySize.x * dd->FramebufferScale.x);
    const UINT h = static_cast<UINT>(dd->DisplaySize.y * dd->FramebufferScale.y);
    if (w == 0 || h == 0) { return false; }
    wartenAuf(rahmen::abschicken());   // imgui_impl_dx12 schreibt die Eckpuffer des Fensters im Ring - keiner darf noch in Arbeit sein
    if (g_queue != nullptr) { wartenAuf(zaunSetzen()); }   // auch die Listen der eigenen Fenster (imgui_impl_dx12)
    D3D12_CLEAR_VALUE cv{};
    cv.Format = kFormat;
    cv.Color[3] = 1.0F;
    ID3D12Resource* r = textur2d(w, h, kFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET, &cv);
    if (r == nullptr) { return false; }
    const D3D12_CPU_DESCRIPTOR_HANDLE ziel = rtv(kPuffer);
    g_dev->CreateRenderTargetView(r, nullptr, ziel);
    ID3D12GraphicsCommandList* l = rahmen::liste();
    l->OMSetRenderTargets(1, &ziel, FALSE, nullptr);
    if ((v->Flags & ImGuiViewportFlags_NoRendererClear) == 0) { l->ClearRenderTargetView(ziel, cv.Color, 0, nullptr); }   // wie ImGui_ImplDX12_RenderWindow
    ImGui_ImplDX12_RenderDrawData(dd, l);
    abgeber::fremdGezeichnet();
    const bool ok = lesen(r, D3D12_RESOURCE_STATE_RENDER_TARGET, aus);
    r->Release();
    return ok;
}

bool inZielZeichnen(ID3D11Resource* ziel, ImDrawData* dd) {
    if (g_queue == nullptr || ziel == nullptr || dd == nullptr) { return false; }
    // imgui_impl_dx12 schreibt die Eckpuffer des Hauptfensters im Ring - keiner darf noch in Arbeit sein
    wartenAuf(rahmen::abschicken());
    umschreiben(dd, {});
    ID3D12GraphicsCommandList* l = rahmen::liste();
    const D3D12_CPU_DESCRIPTOR_HANDLE rt = abgeber::zielFuerOberflaeche(ziel, l);
    if (rt.ptr == 0U) { return false; }
    l->OMSetRenderTargets(1, &rt, FALSE, nullptr);
    ImGui_ImplDX12_RenderDrawData(dd, l);
    abgeber::fremdGezeichnet();
    wartenAuf(rahmen::abschicken());
    return true;
}

long zeigen() {
    (void)rahmen::abschicken();   // was nach dem Hauptfenster noch kam (Zeitmarken, Hochladen), gehoert zu diesem Bild
    Auftrag a;
    a.zeigen = true;
    a.zaun = ++g_zaunWert;
    auftragGeben(a);
    // Das Ergebnis des letzten ausgefuehrten Present: mit dem Abschickfaden kann das noch das vorige Bild sein - ein verlorenes
    // Geraet faellt dann ein Bild spaeter auf.
    const HRESULT hr = static_cast<HRESULT>(g_zeigenErgebnis.exchange(0));
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) { g_geraetWeg = true; }
    g_plaetze[g_platz].zaun = g_zaunWert;
    g_frisch.clear();
    debugMeldungen();
    if (SUCCEEDED(hr)) { naechsterPlatz(); }
    einsammeln();
    abgeber::bildAnfang();
    return hr;
}

void trimmen() {
    // Direct3D 11 gibt hier die Zwischenpuffer des Treibers frei (IDXGIDevice3::Trim). Unter Direct3D 12 gehoert der Speicher
    // dem Programm: freigegebene Texturen und Puffer gehen zurueck, sobald die Grafikkarte sie nicht mehr liest - hier darauf
    // warten (selten: nach dem Laden einer Karte) und alles einsammeln.
    (void)rahmen::abschicken();
    wartenAuf(g_zaunWert);
    einsammeln();
    abgeber::einsammeln();
}

long verlustGrund() { return g_dev != nullptr ? g_dev->GetDeviceRemovedReason() : S_OK; }

void resize(int w, int h) {
    if (g_swap == nullptr) { return; }
    wartenAuf(rahmen::abschicken());
    wartenAuf(zaunSetzen());   // auch das zuletzt gezeigte Bild
    abschickenAbwarten();
    hintenFrei();
    g_swap->ResizeBuffers(0, static_cast<UINT>(w), static_cast<UINT>(h), DXGI_FORMAT_UNKNOWN, g_swapFlags);
    hintenHolen();
}

void* texturAnlegen(const unsigned char* rgba, int w, int h) {
    if (g_dev == nullptr || w <= 0 || h <= 0) { return nullptr; }
    Textur t;
    t.w = w;
    t.h = h;
    t.res = textur2d(static_cast<UINT>(w), static_cast<UINT>(h), kFormat, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
    if (t.res == nullptr) { return nullptr; }
    t.index = srvNehmen();
    if (t.index >= kDeskriptoren || !hochladen(t.res, rgba, w, 0, 0, w, h, true)) {
        if (t.index < kDeskriptoren) { g_srvFrei.push_back(t.index); }
        t.res->Release();
        return nullptr;
    }
    sichtAnlegen(t.res, kFormat, t.index);
    const UINT64 k = srvGpu(t.index).ptr;
    g_texturen[k] = t;
    g_frisch.insert(k);
    return reinterpret_cast<void*>(k);
}

void texturErsetzen(void* kennung, const unsigned char* rgba, int w, int h, int x0, int y0, int x1, int y1) {
    const auto it = g_texturen.find(reinterpret_cast<UINT64>(kennung));
    if (it == g_texturen.end() || it->second.w != w || it->second.h != h) { return; }
    if (hochladen(it->second.res, rgba, w, x0, y0, x1, y1, false)) { g_frisch.insert(it->first); }
}

void texturenFrei() { texturenLoesen(); }

void mischart(ImDrawList* dl, int art, int quelle, int ziel, bool klemmen) {
    const int schluessel = (art << 16) | (art == 1 ? ((quelle & 0xff) | ((ziel & 0xff) << 8) | (klemmen ? (1 << 18) : 0)) : 0);
    dl->AddCallback(mischRueckruf, reinterpret_cast<void*>(static_cast<std::intptr_t>(schluessel)));
}

std::string karte() { return g_kartenName; }

bool befund(std::string& text) {
    debugMeldungen();
    const abgeber::Zaehler z = abgeber::zaehler();
    char t[640];
    std::snprintf(t, sizeof(t),
                  "Direct3D 12: Abgeber %llu Zeichenbefehle, %llu Pipeline-Zustaende in %.0f ms (%llu aus dem Zwischenspeicher, %llu Wechsel), "
                  "%llu frische Bloecke, %llu Kopien gelesener Puffer, %llu lange Bilder, %llu Zustandswechsel, "
                  "Hochladespeicher %llu MB, %llu Objekte, %llu Ressourcen angelegt, %llu Teilabgaben, %llu-mal auf die Grafikkarte gewartet "
                  "(%.0f ms); nicht umgesetzte Aufrufe %llu, Fehler der Debugschicht %u",
                  static_cast<unsigned long long>(z.zeichnen), static_cast<unsigned long long>(z.pso), static_cast<double>(z.psoMikro) / 1000.0,
                  static_cast<unsigned long long>(z.psoAusSpeicher), static_cast<unsigned long long>(z.psoWechsel),
                  static_cast<unsigned long long>(z.umbenannt), static_cast<unsigned long long>(z.inGrafik), static_cast<unsigned long long>(z.langeBilder),
                  static_cast<unsigned long long>(z.barrieren),
                  static_cast<unsigned long long>(z.hochladeBytes >> 20U), static_cast<unsigned long long>(z.lebend),
                  static_cast<unsigned long long>(z.angelegt), static_cast<unsigned long long>(z.teilAbgaben),
                  static_cast<unsigned long long>(g_statistik.gewartet), g_statistik.warteMs, static_cast<unsigned long long>(z.fehlt), g_debugFehler);
    text = t;
    return z.fehlt == 0U && g_debugFehler == 0U;
}

std::vector<std::string> speicherZeilen() { return abgeber::speicherZeilen(); }

void info(std::string& hersteller, std::string& version, std::string& kartenName, std::vector<std::string>& merkmale) {
    if (g_dev == nullptr) { return; }
    IDXGIFactory4* fabrik = nullptr;
    if (SUCCEEDED(CreateDXGIFactory2(0, IID_PPV_ARGS(&fabrik))) && fabrik != nullptr) {
        IDXGIAdapter1* ad = nullptr;
        if (SUCCEEDED(fabrik->EnumAdapterByLuid(g_luid, IID_PPV_ARGS(&ad))) && ad != nullptr) {
            DXGI_ADAPTER_DESC1 d{};
            if (SUCCEEDED(ad->GetDesc1(&d))) {
                switch (d.VendorId) {
                case 0x8086: hersteller = "Intel"; break;
                case 0x10DE: hersteller = "NVIDIA Corporation"; break;
                case 0x1002: hersteller = "ATI Technologies Inc."; break;
                case 0x1414: hersteller = "Microsoft"; break;
                default: {
                    char z[32];
                    std::snprintf(z, sizeof(z), "0x%04X", d.VendorId);
                    hersteller = z;
                }
                }
                char z[64];
                std::snprintf(z, sizeof(z), "%u MB", static_cast<unsigned>(d.DedicatedVideoMemory / (1024 * 1024)));
                merkmale.push_back(std::string("DedicatedVideoMemory ") + z);
            }
            ad->Release();
        }
        fabrik->Release();
    }
    kartenName = g_kartenName;
    D3D12_FEATURE_DATA_FEATURE_LEVELS fl{};
    const D3D_FEATURE_LEVEL stufen[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_12_1,
                                        D3D_FEATURE_LEVEL_12_2};
    fl.NumFeatureLevels = static_cast<UINT>(sizeof(stufen) / sizeof(stufen[0]));
    fl.pFeatureLevelsRequested = stufen;
    const char* stufe = "11_0";
    if (SUCCEEDED(g_dev->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &fl, sizeof(fl)))) {
        stufe = fl.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_12_2   ? "12_2"
                : fl.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_12_1 ? "12_1"
                : fl.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_12_0 ? "12_0"
                : fl.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_11_1 ? "11_1"
                                                                        : "11_0";
    }
    version = std::string("Direct3D 12 - feature level ") + stufe;
    merkmale.push_back(std::string("D3D_FEATURE_LEVEL_") + stufe);
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 o5{};
    if (SUCCEEDED(g_dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &o5, sizeof(o5))) && o5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
        merkmale.push_back(o5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1 ? "RaytracingTier 1.1" : "RaytracingTier 1.0");
    }
    D3D12_FEATURE_DATA_SHADER_MODEL sm{D3D_SHADER_MODEL_6_6};
    if (SUCCEEDED(g_dev->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm)))) {
        char z[32];
        std::snprintf(z, sizeof(z), "ShaderModel %d.%d", (sm.HighestShaderModel >> 4) & 0xF, sm.HighestShaderModel & 0xF);
        merkmale.emplace_back(z);
    }
    D3D12_FEATURE_DATA_D3D12_OPTIONS op{};
    if (SUCCEEDED(g_dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &op, sizeof(op)))) {
        merkmale.push_back("ResourceBindingTier " + std::to_string(static_cast<int>(op.ResourceBindingTier)));
    }
}

}  // namespace bhed::render::d3d12
