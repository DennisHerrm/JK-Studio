// abgeber12_intern.h - D3D12-Abgeber: die Objekte hinter der Schnittstelle ID3D11* (nur fuer abgeber12_*.cpp)
//
// Jedes Objekt, das die Zeichenschichten ueber ID3D11Device anlegen, ist hier eine eigene Klasse mit der D3D12-Ressource oder
// der Beschreibung dahinter. Der Kontext (abgeber12_kontext_win32.cpp) merkt sich, was gebunden ist, und schreibt vor jedem
// Zeichenbefehl nur das Geaenderte in die Befehlsliste.
#ifndef BHED_GUI_ABGEBER12_INTERN_H
#define BHED_GUI_ABGEBER12_INTERN_H

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "abgeber12.h"

namespace bhed::render::d3d12::abgeber {

inline constexpr UINT kCbPlaetze = 9;       // b0..b8 je Stufe (MBRASTER: JeRaster an b8)
// Geometrie-Shader: nur die Register, die die Zeichenschichten binden - b0 (Bild) und b5 (Schattenvolumen, g_cbSchatten)
inline constexpr UINT kGsCbRegister[] = {0, 5};
inline constexpr UINT kGsCbPlaetze = 2;
// Bit eines Konstantenpuffer-Platzes in cbSchmutz/cbGebunden: Stufe * kCbSchritt + Register
inline constexpr UINT kCbSchritt = 10;
inline constexpr UINT kSrvPlaetze = 16;     // t0..t15 (Pixel-Shader)
inline constexpr UINT kAbtasterPlaetze = 4; // s0..s3 (Pixel-Shader)
inline constexpr UINT kZiele = 4;           // Renderziele gleichzeitig
inline constexpr UINT kVbPlaetze = 4;

// Wurzelsignatur: 0..8 CBV Vertex-Shader (b0..b8), 9..17 CBV Pixel-Shader (b0..b8), 18..19 CBV Geometrie-Shader (b0, b5),
// 20..35 je eine Tabelle mit einer Textur (t0..t15, Pixel-Shader), 36..39 je eine Tabelle mit einem Abtaster (s0..s3).
// Zusammen 60 von 64 Worten.
inline constexpr UINT kWurzelCbVs = 0;
inline constexpr UINT kWurzelCbPs = 9;
inline constexpr UINT kWurzelCbGs = 18;
inline constexpr UINT kWurzelSrv = 20;
inline constexpr UINT kWurzelAbtaster = 36;

extern std::atomic<std::uint64_t> g_lebend;
// REST612: davon die Objekte des jetzigen Geraets (Generation g_generation); der Rest gehoert frueheren Geraeten - waechst er mit
// jedem Geraeteverlust, vergisst eine Zeichenschicht ihre Objekte, statt sie freizugeben (Selbsttest grafik, verlustreihe).
extern std::atomic<std::uint64_t> g_lebendJetzt;
// Teilabgabe: zeichnet die offene Liste so viele Takte (QueryPerformanceCounter) auf, geht sie beim naechsten Wechsel der Ziele
// ab (0: nur am Ende des Bildes). BHED_D3D12_TEILABGABE=<Mikrosekunden>.
extern long long g_teilAbgabe;
// Hoechstens so viele Teilabgaben je Bild (BHED_D3D12_TEILABGABEN=<n>). Unter fremder Last zaehlt die Uhr auch die Zeit, in der
// der Hauptfaden nicht rechnet: dann ging jeder Zielwechsel einzeln ab (bis 364 Abgaben je Bild, Bilder 10- bis 15-mal laenger
// als unter Direct3D 11, 12.11. abends). Ohne Last sind es im Mittel rund eine je Bild.
extern int g_teilAbgabenMax;
// D3D12-5: so viele Zeichenbefehle (und UpdateSubresource) hoechstens in einer Liste - danach geht sie am naechsten Zeichenbefehl
// ab. Ohne Grenze nahm ein Bild mit 355 000 Zeichenbefehlen (Karten-Editor beim Oeffnen der .map) rund 100 MB Befehlsspeicher, den
// der Treiber danach behaelt (rahmen: Befehlsspeicher im Kreis). Die gemessenen Ansichten haben hoechstens 4768 je Bild - sie gehen
// ungeteilt ab wie bisher. BHED_D3D12_LISTE=<n> (0: ohne Grenze, zum Vergleichen).
extern std::uint32_t g_listeBefehle;
// D3D12-5: so viel Zeit je Bild fuer das Zurueckgeben der Vorraete (Millisekunden, mindestens eine Freigabe; BHED_D3D12_KUERZEN_MS,
// 0 = ohne Grenze). Ein Release kostet im Systemkern ~0,09 ms; nach dem Umschalten im Karten-Editor wurden 153 bzw. 382 auf einmal
// faellig (13,6 / 34,9 ms in einem Bild).
extern double g_kuerzenMs;
// D3D12-4: Zeitmarken (GetData) ohne den Leerlauf der Grafikkarte zwischen den Abgaben (rahmen::leerlaufBis) - so misst die
// Strecke zwischen zwei Marken, was die Grafikkarte dazwischen gerechnet hat, auch ueber eine Teilabgabe hinweg.
// BHED_D3D12_MARKEN=roh: die Zeitmarken wie geschrieben (Leerlauf zwischen den Teilen zaehlt mit).
extern bool g_markenArbeitsuhr;
extern std::uint32_t g_naechsteKennung;
// Zaehlt anlegen(): jedes Objekt traegt die Generation des Abgebers, unter dem es entstand. Nach einem Geraeteverlust geben die
// Zeichenschichten manche Objekte erst frei, wenn der neue Abgeber schon steht (karte_gpu: Stempel render::texturStand) - ein
// Objekt einer frueheren Generation gibt dann nichts in die Vorraete des neuen zurueck (sein Block liegt in einer freigegebenen
// Seite, sein Deskriptor im alten Stapel, sein Puffer gehoert dem alten Geraet) und wird beim Gebrauch uebergangen.
extern std::uint32_t g_generation;

// Eine nicht umgesetzte Methode wurde gerufen: einmal je Methode ins Protokoll (Fehler), zaehlen. Gibt E_NOTIMPL.
HRESULT fehlt(const char* methode);
// Ein Objekt eines frueheren Geraets wurde benutzt: einmal je Stelle ins Protokoll (Fehler), zaehlt wie eine nicht umgesetzte
// Methode (der Befund des Selbsttests wird rot).
void altesObjekt(const char* wo);
// Das Objekt, wenn es zum jetzigen Abgeber gehoert; sonst nullptr (gemeldet).
template <class T>
[[nodiscard]] inline T* jetzig(T* o, const char* wo) {
    if (o != nullptr && o->generation != g_generation) {
        altesObjekt(wo);
        return nullptr;
    }
    return o;
}

struct Geraet;
struct Kontext;
[[nodiscard]] ID3D11Device* geraetZeiger();

// Gemeinsame Basis aller Objekte: Verweise zaehlen, ID3D11DeviceChild. `gebunden` zaehlt die Bindungen im Kontext - wie unter
// Direct3D 11 lebt ein gebundenes Objekt weiter, auch wenn sein Besitzer es schon freigegeben hat.
template <class Ich, class Schnittstelle>
struct Kind : Schnittstelle {
    std::atomic<ULONG> verweise{1};
    ULONG gebunden = 0;
    std::uint32_t kennung;
    std::uint32_t generation;

    Kind() : kennung(++g_naechsteKennung), generation(g_generation) {
        ++g_lebend;
        ++g_lebendJetzt;
    }
    Kind(const Kind&) = delete;
    Kind& operator=(const Kind&) = delete;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** aus) override {
        if (aus == nullptr) { return E_POINTER; }
        if (iid == __uuidof(IUnknown) || iid == __uuidof(ID3D11DeviceChild) || Ich::istSchnittstelle(iid)) {
            *aus = static_cast<Schnittstelle*>(this);
            ++verweise;
            return S_OK;
        }
        *aus = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++verweise; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG r = --verweise;
        if (r == 0 && gebunden == 0) { delete static_cast<Ich*>(this); }
        return r;
    }
    void STDMETHODCALLTYPE GetDevice(ID3D11Device** d) override {
        if (d != nullptr) {
            *d = geraetZeiger();
            if (*d != nullptr) { (*d)->AddRef(); }
        }
    }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID, UINT*, void*) override { return DXGI_ERROR_NOT_FOUND; }
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID, UINT, const void*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID, const IUnknown*) override { return S_OK; }

protected:
    ~Kind() {
        --g_lebend;
        if (generation == g_generation) { --g_lebendJetzt; }
    }
};

// Einen Platz des Kontexts neu belegen (zaehlt die Bindung; gibt ein schon freigegebenes Objekt frei, wenn es die letzte war).
template <class T>
inline void binde(T*& platz, T* neu) {
    if (neu != nullptr) { ++neu->gebunden; }
    T* alt = platz;
    platz = neu;
    if (alt != nullptr && --alt->gebunden == 0 && alt->verweise.load() == 0) { delete alt; }
}

// Ein Stueck Hochladespeicher (immer lesbar fuer die Grafikkarte, beschreibbar fuer den Prozessor).
struct Block {
    ID3D12Resource* res = nullptr;
    std::uint64_t versatz = 0;    // Anfang in res
    std::uint64_t va = 0;         // Adresse fuer die Grafikkarte
    unsigned char* cpu = nullptr;
    std::uint64_t groesse = 0;    // des Blocks (Groessenklasse)
    std::uint32_t klasse = ~0U;   // Groessenklasse (256 B << klasse); ~0: ein grosser Block, einzeln angelegt
    std::uint32_t seite = ~0U;    // D3D12-5: Seite (kleine Bloecke), aus der er stammt
    std::uint32_t seiteGen = 0;   // D3D12-5: Generation der Seite beim Anlegen (anders: Seite inzwischen zurueckgegeben)
};
[[nodiscard]] Block blockNehmen(std::uint64_t groesse);
// Ein frischer Block derselben Groesse wie `vorlage` (Map(WRITE_DISCARD): ohne die Klasse neu zu suchen).
[[nodiscard]] Block blockWieNehmen(const Block& vorlage);
void blockSpaeterFrei(const Block& b);   // wieder vergeben, wenn die Grafikkarte die Listen bis hierher abgearbeitet hat

// --- Puffer ---------------------------------------------------------------------
struct Puffer final : Kind<Puffer, ID3D11Buffer> {
    D3D11_BUFFER_DESC art{};
    bool dynamisch = false;        // D3D11_USAGE_DYNAMIC: im Hochladespeicher, DISCARD nimmt einen frischen Block
    Block block;
    std::uint32_t fassung = 0;     // zaehlt die Bloecke (fuer Sichten auf den Puffer)
    ID3D12Resource* res = nullptr; // sonst: im Grafikspeicher
    std::uint64_t va = 0;          // Adresse fuer die Grafikkarte (beide Arten)
    D3D12_RESOURCE_STATES lesen = D3D12_RESOURCE_STATE_COMMON;   // Lesezustand nach den Bindungsarten
    // Puffer im Grafikspeicher stehen nach jedem ExecuteCommandLists wieder in COMMON, und Direct3D 12 stellt sie beim ersten
    // lesenden Gebrauch von selbst lesbar. Ausdruecklich gewechselt wird nur, was in derselben Ausfuehrung beschrieben wurde
    // (Anfangsdaten, UpdateSubresource) - und Puffer mit mehreren Lesearten (`ausdruecklich`), deren von selbst erreichter Zustand
    // von der Reihenfolge des Gebrauchs abhinge.
    bool ausdruecklich = false;
    std::uint64_t serie = ~0ULL;   // rahmen::ausfuehrung(), in der res gebraucht wurde und im Lesezustand steht
    std::uint64_t kopierSerie = ~0ULL;   // D3D12-4: rahmen::ausfuehrung(), in der res nach UpdateSubresource in COPY_DEST steht
    // Ruhende dynamische Eck- und Indexpuffer (seit kRuheBilder Bildern kein Map, mindestens kRuheAb Bytes): der Inhalt liegt als
    // Kopie im Grafikspeicher, va zeigt dorthin. Sonst liest die Grafikkarte ihn bei jedem Zeichenbefehl aus dem Arbeitsspeicher.
    // Puffer, die der Pixel-Shader liest (D3D11_BIND_SHADER_RESOURCE: Lichtliste, Kacheln des Echtzeit-Lichts), bekommen die Kopie
    // sofort beim ersten Gebrauch nach jedem Map (pufferInGrafik): der Shader liest sie je Bildpunkt, und aus dem Arbeitsspeicher
    // kostet jeder Zugriff den Weg ueber den Bus.
    ID3D12Resource* ruhe = nullptr;      // die Kopie (bleibt angelegt, solange der Puffer lebt)
    bool ruheGilt = false;
    std::uint64_t ruheSerie = ~0ULL;     // rahmen::ausfuehrung(), in der die Kopie zuletzt beschrieben und lesbar gestellt wurde
    std::uint64_t geschriebenBild = 0;   // Bild des letzten Map
    // Wo der Kontext ihn gerade gebunden hat - ein Map(WRITE_DISCARD) aendert seine Adresse, und genau diese Plaetze muessen neu
    // in die Liste (Kontext::pufferUmbenannt).
    std::uint32_t cbGebunden = 0;    // Bit = Stufe * kCbSchritt + Register
    std::uint16_t srvGebunden = 0;   // Bit = Platz (ueber eine Sicht auf den Puffer)
    std::uint8_t vbGebunden = 0;     // Bit = Platz
    bool ibGebunden = false;

    ~Puffer();
    static bool istSchnittstelle(REFIID iid) { return iid == __uuidof(ID3D11Resource) || iid == __uuidof(ID3D11Buffer); }
    void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION* t) override { *t = D3D11_RESOURCE_DIMENSION_BUFFER; }
    void STDMETHODCALLTYPE SetEvictionPriority(UINT) override {}
    UINT STDMETHODCALLTYPE GetEvictionPriority() override { return 0; }
    void STDMETHODCALLTYPE GetDesc(D3D11_BUFFER_DESC* d) override { *d = art; }
};

// --- Textur ---------------------------------------------------------------------
struct Textur final : Kind<Textur, ID3D11Texture2D> {
    D3D11_TEXTURE2D_DESC art{};
    ID3D12Resource* res = nullptr;   // die Textur; bei D3D11_USAGE_STAGING der Lesepuffer
    D3D12_RESOURCE_STATES zustand = D3D12_RESOURCE_STATE_COMMON;   // aller Teile (nach den bisher geschriebenen Befehlen)
    DXGI_FORMAT format12 = DXGI_FORMAT_UNKNOWN;   // Format der D3D12-Ressource
    bool lesepuffer = false;
    // nur Lesepuffer
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> abdruck;   // je Teil
    unsigned char* cpu = nullptr;
    std::uint64_t kopieSerie = ~0ULL;   // rahmen::ausfuehrung() beim letzten Kopieren hinein
    bool kopiert = false;               // seit dem letzten Warten wurde hineinkopiert
    // Tiefe mit Stencil (D32_FLOAT_S8X24_UINT, D24_UNORM_S8_UINT): Direct3D 12 kopiert zwei Ebenen (Tiefe, Stencil), abdruck hat
    // dann erst alle Teile der Tiefe, danach alle des Stencils. Map setzt sie zur Aufteilung von Direct3D 11 zusammen.
    UINT ebenen = 1;
    std::vector<unsigned char> zusammen;
    // Grosse, leer angelegte Texturfelder (kDuennAb) sind reservierte Ressourcen: eine Schicht bekommt ihren Speicher erst mit
    // dem ersten Schreiben (schichtBelegen). Bis dahin liest sie 0 und kostet keinen Grafikspeicher - so belegt auch der
    // D3D11-Treiber. Leer: eine gewoehnliche Textur.
    std::vector<ID3D12Heap*> schichtHeap;   // je Schicht; nullptr: noch ohne Speicher
    UINT schichtKacheln = 0;                // Kacheln zu 64 KB je Schicht
    // Auslagern: eine Textur, die lange in keiner Befehlsliste vorkam, gibt ihren Grafikspeicher ab (Evict) und holt ihn vor
    // dem naechsten Gebrauch zurueck (MakeResident, in gebraucht()) - der D3D11-Treiber haelt ebenso nur, was gebraucht wird.
    std::uint64_t bytes = 0;           // Groesse der Zuteilung (duenne Felder zaehlen nach ihren Heaps)
    std::uint32_t gebrauchtBild = 0;   // g_gebrauchBild beim letzten Gebrauch
    bool ausgelagert = false;
    bool jeGebraucht = false;          // D3D12-5 (Speicherbericht): kam je in eine Befehlsliste

    ~Textur();
    static bool istSchnittstelle(REFIID iid) { return iid == __uuidof(ID3D11Resource) || iid == __uuidof(ID3D11Texture2D); }
    void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION* t) override { *t = D3D11_RESOURCE_DIMENSION_TEXTURE2D; }
    void STDMETHODCALLTYPE SetEvictionPriority(UINT) override {}
    UINT STDMETHODCALLTYPE GetEvictionPriority() override { return 0; }
    void STDMETHODCALLTYPE GetDesc(D3D11_TEXTURE2D_DESC* d) override { *d = art; }
    [[nodiscard]] UINT teile() const { return art.MipLevels * art.ArraySize; }
};

// --- Sichten --------------------------------------------------------------------
struct Sicht final : Kind<Sicht, ID3D11ShaderResourceView> {
    Textur* textur = nullptr;   // eines von beiden, gehalten
    Puffer* puffer = nullptr;
    D3D11_SHADER_RESOURCE_VIEW_DESC art{};
    D3D12_SHADER_RESOURCE_VIEW_DESC art12{};
    UINT index = 0;                    // Deskriptor im gemeinsamen Stapel
    UINT indexOben = ~0U;              // fuer die Oberflaeche: dieselbe Sicht mit nur ihrer obersten Stufe (~0: noch keiner)
    std::uint32_t fassung = 0;         // Puffer: Fassung, fuer die der Deskriptor geschrieben ist
    bool aufKopie = false;             // Puffer: der Deskriptor zeigt auf die Kopie im Grafikspeicher (Puffer::ruhe) - er bleibt

    ~Sicht();
    static bool istSchnittstelle(REFIID iid) { return iid == __uuidof(ID3D11View) || iid == __uuidof(ID3D11ShaderResourceView); }
    void STDMETHODCALLTYPE GetResource(ID3D11Resource** r) override;
    void STDMETHODCALLTYPE GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC* d) override { *d = art; }
};

struct ZielSicht final : Kind<ZielSicht, ID3D11RenderTargetView> {
    Textur* textur = nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC art{};
    UINT index = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE griff{};
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

    ~ZielSicht();
    static bool istSchnittstelle(REFIID iid) { return iid == __uuidof(ID3D11View) || iid == __uuidof(ID3D11RenderTargetView); }
    void STDMETHODCALLTYPE GetResource(ID3D11Resource** r) override;
    void STDMETHODCALLTYPE GetDesc(D3D11_RENDER_TARGET_VIEW_DESC* d) override { *d = art; }
};

struct TiefeSicht final : Kind<TiefeSicht, ID3D11DepthStencilView> {
    Textur* textur = nullptr;
    D3D11_DEPTH_STENCIL_VIEW_DESC art{};
    UINT index = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE griff{};
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

    ~TiefeSicht();
    static bool istSchnittstelle(REFIID iid) { return iid == __uuidof(ID3D11View) || iid == __uuidof(ID3D11DepthStencilView); }
    void STDMETHODCALLTYPE GetResource(ID3D11Resource** r) override;
    void STDMETHODCALLTYPE GetDesc(D3D11_DEPTH_STENCIL_VIEW_DESC* d) override { *d = art; }
};

// --- Shader, Eingabe, Zustaende --------------------------------------------------
// Pruefsumme ueber Bytes, acht auf einmal (fuer die Namen der Pipeline-Zustaende im Zwischenspeicher auf der Platte).
inline void mische(std::uint64_t h[2], const void* daten, std::size_t n) {
    const auto* c = static_cast<const unsigned char*>(daten);
    while (n != 0U) {
        std::uint64_t w = 0;
        const std::size_t m = n < 8U ? n : 8U;
        for (std::size_t i = 0; i < m; ++i) { w |= static_cast<std::uint64_t>(c[i]) << (8U * i); }
        if (m < 8U) { w ^= static_cast<std::uint64_t>(m) << 56U; }
        h[0] = (h[0] ^ w) * 0x9E3779B97F4A7C15ULL;
        h[0] ^= h[0] >> 32U;
        h[1] = (h[1] + w) * 0xC2B2AE3D27D4EB4FULL;
        h[1] ^= h[1] >> 29U;
        c += m;
        n -= m;
    }
}

template <class S>
struct Shader final : Kind<Shader<S>, S> {
    std::vector<unsigned char> code;   // DXBC, unveraendert
    std::uint64_t pruef[2] = {0x243F6A8885A308D3ULL, 0x13198A2E03707344ULL};   // Pruefsumme des Codes (shaderAnlegen)
    static bool istSchnittstelle(REFIID iid) { return iid == __uuidof(S); }
};
using VsObjekt = Shader<ID3D11VertexShader>;
using PsObjekt = Shader<ID3D11PixelShader>;
using GsObjekt = Shader<ID3D11GeometryShader>;

struct Eingabe final : Kind<Eingabe, ID3D11InputLayout> {
    std::vector<D3D12_INPUT_ELEMENT_DESC> elemente;
    std::vector<std::string> namen;   // SemanticName der Elemente zeigt hier hinein
    static bool istSchnittstelle(REFIID iid) { return iid == __uuidof(ID3D11InputLayout); }
};

struct Mischung final : Kind<Mischung, ID3D11BlendState> {
    D3D11_BLEND_DESC art{};
    ~Mischung();
    static bool istSchnittstelle(REFIID iid) { return iid == __uuidof(ID3D11BlendState); }
    void STDMETHODCALLTYPE GetDesc(D3D11_BLEND_DESC* d) override { *d = art; }
};

struct TiefeZustand final : Kind<TiefeZustand, ID3D11DepthStencilState> {
    D3D11_DEPTH_STENCIL_DESC art{};
    ~TiefeZustand();
    static bool istSchnittstelle(REFIID iid) { return iid == __uuidof(ID3D11DepthStencilState); }
    void STDMETHODCALLTYPE GetDesc(D3D11_DEPTH_STENCIL_DESC* d) override { *d = art; }
};

struct Raster final : Kind<Raster, ID3D11RasterizerState> {
    D3D11_RASTERIZER_DESC art{};
    ~Raster();
    static bool istSchnittstelle(REFIID iid) { return iid == __uuidof(ID3D11RasterizerState); }
    void STDMETHODCALLTYPE GetDesc(D3D11_RASTERIZER_DESC* d) override { *d = art; }
};

struct Abtaster final : Kind<Abtaster, ID3D11SamplerState> {
    D3D11_SAMPLER_DESC art{};
    UINT index = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
    ~Abtaster();
    static bool istSchnittstelle(REFIID iid) { return iid == __uuidof(ID3D11SamplerState); }
    void STDMETHODCALLTYPE GetDesc(D3D11_SAMPLER_DESC* d) override { *d = art; }
};

struct Abfrage final : Kind<Abfrage, ID3D11Query> {
    D3D11_QUERY_DESC art{};
    int platz = -1;                 // Zeitmarke: Platz im Abfragestapel
    std::uint64_t serie = ~0ULL;    // rahmen::ausfuehrung() beim End
    std::uint64_t zaun = 0;         // Zaunwert der Liste mit dem End (0: noch nicht abgeschickt)
    bool beendet = false;
    static bool istSchnittstelle(REFIID iid) { return iid == __uuidof(ID3D11Asynchronous) || iid == __uuidof(ID3D11Query); }
    UINT STDMETHODCALLTYPE GetDataSize() override {
        return art.Query == D3D11_QUERY_TIMESTAMP_DISJOINT ? static_cast<UINT>(sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT)) : static_cast<UINT>(sizeof(UINT64));
    }
    void STDMETHODCALLTYPE GetDesc(D3D11_QUERY_DESC* d) override { *d = art; }
};

// --- Dienste zwischen Geraet und Kontext ------------------------------------------
[[nodiscard]] ID3D12RootSignature* wurzelSignatur();
[[nodiscard]] ID3D12DescriptorHeap* abtasterStapel();
[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE leereSicht();        // Texture2D ohne Ressource (liest 0 wie unter Direct3D 11)
[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE vorgabeAbtaster();   // der Abtaster von Direct3D 11 ohne gebundenen Zustand
[[nodiscard]] std::uint64_t nullSeite();                       // 64 KB Nullen: Konstantenpuffer ohne Bindung
// Den Deskriptor einer Sicht auf einen dynamischen Puffer fuer dessen jetzigen Block schreiben (neuer Platz im Stapel).
void sichtAufPufferErneuern(Sicht* s);
// Vor dem Schreiben in Schichten eines duennen Texturfelds (Textur::schichtHeap): ihnen Speicher geben. anzahl ~0U: bis zur letzten.
void schichtBelegen(Textur* t, UINT erste, UINT anzahl);
[[nodiscard]] inline bool schichtBelegt(const Textur* t, UINT schicht) {
    return t->schichtHeap.empty() || (schicht < t->schichtHeap.size() && t->schichtHeap[schicht] != nullptr);
}
// Auslagern: JEDE Stelle, die eine Textur in eine Befehlsliste bringt, ruft vorher gebraucht() - sonst zeichnet die Liste mit
// einer ausgelagerten Ressource. stelle, stelleZwei und wechsle tun es selbst.
extern std::uint32_t g_gebrauchBild;   // das laufende Bild
void einlagern(Textur* t);             // MakeResident nach dem Auslagern
inline void gebraucht(Textur* t) {
    t->gebrauchtBild = g_gebrauchBild;
    t->jeGebraucht = true;
    if (t->ausgelagert) { einlagern(t); }
}
// Zustandswechsel einer Textur in die Liste (alle Teile); zaehlt.
void wechsle(ID3D12GraphicsCommandList* l, Textur* t, D3D12_RESOURCE_STATES nach);
// Mehrere Texturen in EINEM ResourceBarrier-Aufruf (hoechstens kSrvPlaetze; wer schon so steht, faellt heraus).
struct Wechsel {
    Textur* t = nullptr;
    D3D12_RESOURCE_STATES nach = D3D12_RESOURCE_STATE_COMMON;
};
void wechsle(ID3D12GraphicsCommandList* l, const Wechsel* w, UINT n);
// D3D12-4: wie wechsle, gibt die Wechsel aber nicht ab, sondern haengt sie an b an (hoechstens frei); Rueckgabe: wie viele.
UINT wechselSammeln(ID3D12GraphicsCommandList* l, const Wechsel* w, UINT n, D3D12_RESOURCE_BARRIER* b, UINT frei);
// Einen Puffer im Grafikspeicher lesend gebrauchen (merkt die Ausfuehrung; Zustandswechsel nur, wo Direct3D 12 ihn verlangt).
void pufferLesbar(ID3D12GraphicsCommandList* l, Puffer* p);
// Einen dynamischen Eck- oder Indexpuffer binden: ruht sein Inhalt, zeigt va danach auf die Kopie im Grafikspeicher.
void pufferRuhe(ID3D12GraphicsCommandList* l, Puffer* p);
// Ein Map hebt die Ruhe auf (va zeigt wieder auf den Block); true: va hat sich geaendert.
bool pufferGeweckt(Puffer* p);
// Ein dynamischer Puffer, den der Pixel-Shader liest: den Inhalt seines Blocks jetzt in die Kopie im Grafikspeicher (einmal je
// Map). false: keine Kopie (zu klein, abgeschaltet, kein Speicher) - dann liest der Shader weiter aus dem Block.
bool pufferInGrafik(ID3D12GraphicsCommandList* l, Puffer* p);
void zaehleBarriere();
// Der Kontext vergisst Bindungen nicht von selbst; die Sicht-Kennungen fuer die Oberflaeche.
void sichtMelden(Sicht* s, bool lebt);
// Zeitmarken (Abfragestapel): Platz nehmen, aufloesen, lesen.
[[nodiscard]] ID3D12QueryHeap* markenStapel();
[[nodiscard]] int markeNehmen();
bool markeLesen(int platz, std::uint64_t& wert);
// Mip-Stufen einer Textur aus Stufe 0 rechnen (ID3D11DeviceContext::GenerateMips).
void mipsErzeugen(ID3D12GraphicsCommandList* l, Sicht* s);
// Der Zustand des Kontexts in der Liste ist ungueltig (anderer Zeichner, eigener Sonderdurchgang).
void kontextAllesNeu();
// Ein Renderziel-Deskriptor fuer Sonderdurchgaenge (GenerateMips, Oberflaeche in ein Ziel); gilt bis zum naechsten Gebrauch.
[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE hilfsZiel();
void psoAbbauen();
void bloeckeStempeln(std::uint64_t zaun);
Zaehler& zaehlerIntern();
[[nodiscard]] bool aktiv();

// --- Zeiten (Messhilfe): was der Abgeber selbst an Wartezeit in ein Bild bringt --------------------
// Je Bild gesammelt; ein Bild ueber der Grenze (BHED_D3D12_LANGBILD=<ms>, Vorgabe 100) schreibt seine Posten ins Protokoll -
// so stehen die Ursachen eines langen Bildes (erstes Bild nach dem Laden, Optimierer auf einer grossen Karte) als Zahlen da.
struct Zeiten {
    double psoMs = 0.0;          // Pipeline-Zustaende anlegen oder aus dem Zwischenspeicher holen
    double texturMs = 0.0;       // CreateTexture2D (anlegen und Anfangsdaten kopieren)
    double pufferMs = 0.0;       // CreateBuffer im Grafikspeicher
    double einlagernMs = 0.0;    // MakeResident
    double auslagernMs = 0.0;    // Evict
    std::uint32_t pso = 0;
    std::uint32_t psoAusSpeicher = 0;
    std::uint32_t textur = 0;
    std::uint32_t puffer = 0;
    std::uint32_t einlagern = 0;
    std::uint64_t einlagernBytes = 0;
    std::uint64_t texturBytes = 0;   // Anfangsdaten
    double kuerzenMs = 0.0;          // D3D12-5: Vorraete kuerzen (Bloecke, Puffer, Seiten an das System zurueck)
    double seitenMs = 0.0;           // D3D12-5: neue Seiten fuer kleine Bloecke anlegen
    std::uint32_t gekuerzt = 0;      // davon zurueckgegeben (Stueck)
    double releaseMs = 0.0;          // davon fuer das Uebergeben an den Freigabefaden (vorher: Release im Systemkern)
    double releaseHoechstMs = 0.0;   // das laengste einzelne Uebergeben
    std::uint32_t seiten = 0;        // davon angelegt (Stueck)
};
[[nodiscard]] Zeiten& zeitenBild();
extern double g_msJeTakt;   // Millisekunden je Takt von QueryPerformanceCounter
struct Stoppuhr {
    double& ziel;
    LARGE_INTEGER t0;
    explicit Stoppuhr(double& z) : ziel(z) { QueryPerformanceCounter(&t0); }
    Stoppuhr(const Stoppuhr&) = delete;
    Stoppuhr& operator=(const Stoppuhr&) = delete;
    ~Stoppuhr() {
        LARGE_INTEGER t1;
        QueryPerformanceCounter(&t1);
        ziel += static_cast<double>(t1.QuadPart - t0.QuadPart) * g_msJeTakt;
    }
};

// --- Bildprotokoll (Messhilfe, abgeber12_protokoll_win32.cpp) ------------------------
// BHED_D3D12_PROTOKOLL=<n>: jedes n-te Bild schreibt auf, was in die Befehlsliste geht, mit Zeitmarken der Grafikkarte.
namespace protokoll {
extern bool g_an;   // dieses Bild wird aufgeschrieben - Eintragsstellen fragen das, bevor sie einen Text bauen
// Ein Eintrag; mit cl eine Zeitmarke der Grafikkarte davor.
void eintrag(ID3D12GraphicsCommandList* cl, std::string text);
[[nodiscard]] std::string nenne(const Textur* t);
[[nodiscard]] const char* zustandName(D3D12_RESOURCE_STATES z);
void bildWechsel();   // einmal je Bild, nach dem Abschicken
void abbauen();
}  // namespace protokoll
// Was der Abgeber belegt, als Zeilen fuer das Protokoll: Grafikspeicher und Arbeitsspeicher nach Art, die groessten Texturen.
// summe: Bytes im Grafikspeicher (der Bericht erscheint nur, wenn sie sich geaendert hat).
[[nodiscard]] std::vector<std::string> speicherBericht(std::uint64_t& summe);

// --- Geraet -----------------------------------------------------------------------
struct Geraet final : ID3D11Device {
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** aus) override;
    ULONG STDMETHODCALLTYPE AddRef() override { return 2; }    // lebt von anlegen() bis abbauen()
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE CreateBuffer(const D3D11_BUFFER_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Buffer **ppBuffer) override;
    HRESULT STDMETHODCALLTYPE CreateTexture1D(const D3D11_TEXTURE1D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture1D **ppTexture1D) override;
    HRESULT STDMETHODCALLTYPE CreateTexture2D(const D3D11_TEXTURE2D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture2D **ppTexture2D) override;
    HRESULT STDMETHODCALLTYPE CreateTexture3D(const D3D11_TEXTURE3D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture3D **ppTexture3D) override;
    HRESULT STDMETHODCALLTYPE CreateShaderResourceView(ID3D11Resource *pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc, ID3D11ShaderResourceView **ppSRView) override;
    HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView(ID3D11Resource *pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc, ID3D11UnorderedAccessView **ppUAView) override;
    HRESULT STDMETHODCALLTYPE CreateRenderTargetView(ID3D11Resource *pResource, const D3D11_RENDER_TARGET_VIEW_DESC *pDesc, ID3D11RenderTargetView **ppRTView) override;
    HRESULT STDMETHODCALLTYPE CreateDepthStencilView(ID3D11Resource *pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc, ID3D11DepthStencilView **ppDepthStencilView) override;
    HRESULT STDMETHODCALLTYPE CreateInputLayout(const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs, UINT NumElements, const void *pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, ID3D11InputLayout **ppInputLayout) override;
    HRESULT STDMETHODCALLTYPE CreateVertexShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11VertexShader **ppVertexShader) override;
    HRESULT STDMETHODCALLTYPE CreateGeometryShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11GeometryShader **ppGeometryShader) override;
    HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput(const void *pShaderBytecode, SIZE_T BytecodeLength, const D3D11_SO_DECLARATION_ENTRY *pSODeclaration, UINT NumEntries, const UINT *pBufferStrides, UINT NumStrides, UINT RasterizedStream, ID3D11ClassLinkage *pClassLinkage, ID3D11GeometryShader **ppGeometryShader) override;
    HRESULT STDMETHODCALLTYPE CreatePixelShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11PixelShader **ppPixelShader) override;
    HRESULT STDMETHODCALLTYPE CreateHullShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11HullShader **ppHullShader) override;
    HRESULT STDMETHODCALLTYPE CreateDomainShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11DomainShader **ppDomainShader) override;
    HRESULT STDMETHODCALLTYPE CreateComputeShader(const void *pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage *pClassLinkage, ID3D11ComputeShader **ppComputeShader) override;
    HRESULT STDMETHODCALLTYPE CreateClassLinkage(ID3D11ClassLinkage **ppLinkage) override;
    HRESULT STDMETHODCALLTYPE CreateBlendState(const D3D11_BLEND_DESC *pBlendStateDesc, ID3D11BlendState **ppBlendState) override;
    HRESULT STDMETHODCALLTYPE CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC *pDepthStencilDesc, ID3D11DepthStencilState **ppDepthStencilState) override;
    HRESULT STDMETHODCALLTYPE CreateRasterizerState(const D3D11_RASTERIZER_DESC *pRasterizerDesc, ID3D11RasterizerState **ppRasterizerState) override;
    HRESULT STDMETHODCALLTYPE CreateSamplerState(const D3D11_SAMPLER_DESC *pSamplerDesc, ID3D11SamplerState **ppSamplerState) override;
    HRESULT STDMETHODCALLTYPE CreateQuery(const D3D11_QUERY_DESC *pQueryDesc, ID3D11Query **ppQuery) override;
    HRESULT STDMETHODCALLTYPE CreatePredicate(const D3D11_QUERY_DESC *pPredicateDesc, ID3D11Predicate **ppPredicate) override;
    HRESULT STDMETHODCALLTYPE CreateCounter(const D3D11_COUNTER_DESC *pCounterDesc, ID3D11Counter **ppCounter) override;
    HRESULT STDMETHODCALLTYPE CreateDeferredContext(UINT ContextFlags, ID3D11DeviceContext **ppDeferredContext) override;
    HRESULT STDMETHODCALLTYPE OpenSharedResource(HANDLE hResource, REFIID ReturnedInterface, void **ppResource) override;
    HRESULT STDMETHODCALLTYPE CheckFormatSupport(DXGI_FORMAT Format, UINT *pFormatSupport) override;
    HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels(DXGI_FORMAT Format, UINT SampleCount, UINT *pNumQualityLevels) override;
    void STDMETHODCALLTYPE CheckCounterInfo(D3D11_COUNTER_INFO *pCounterInfo) override;
    HRESULT STDMETHODCALLTYPE CheckCounter(const D3D11_COUNTER_DESC *pDesc, D3D11_COUNTER_TYPE *pType, UINT *pActiveCounters, LPSTR szName, UINT *pNameLength, LPSTR szUnits, UINT *pUnitsLength, LPSTR szDescription, UINT *pDescriptionLength) override;
    HRESULT STDMETHODCALLTYPE CheckFeatureSupport(D3D11_FEATURE Feature, void *pFeatureSupportData, UINT FeatureSupportDataSize) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT *pDataSize, void *pData) override;
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void *pData) override;
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown *pData) override;
    D3D_FEATURE_LEVEL STDMETHODCALLTYPE GetFeatureLevel() override;
    UINT STDMETHODCALLTYPE GetCreationFlags() override;
    HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason() override;
    void STDMETHODCALLTYPE GetImmediateContext(ID3D11DeviceContext **ppImmediateContext) override;
    HRESULT STDMETHODCALLTYPE SetExceptionMode(UINT RaiseFlags) override;
    UINT STDMETHODCALLTYPE GetExceptionMode() override;
};

// --- Kontext ----------------------------------------------------------------------
struct PsoSchluessel {
    std::uint32_t vs = 0, ps = 0, gs = 0, eingabe = 0, mischung = 0, tiefe = 0, raster = 0;
    std::uint32_t maske = 0xFFFFFFFFU;
    std::uint16_t zielFormat[kZiele]{};
    std::uint16_t tiefeFormat = 0;
    std::uint8_t ziele = 0;
    std::uint8_t proben = 1;
    std::uint8_t topologie = 0;   // D3D12_PRIMITIVE_TOPOLOGY_TYPE
    std::uint8_t frei = 0;
    std::uint16_t frei2 = 0;
    bool operator==(const PsoSchluessel& o) const noexcept;
};

struct Kontext final : ID3D11DeviceContext1 {
    // gebunden (Stufe 0 Vertex-, 1 Pixel-, 2 Geometrie-Shader)
    Puffer* cb[3][kCbPlaetze]{};         // je Register (Geometrie-Shader: nur kGsCbRegister belegt)
    UINT cbErste[3][kCbPlaetze]{};       // erste Konstante (zu 16 Bytes), VSSetConstantBuffers1
    Sicht* srv[kSrvPlaetze]{};
    Abtaster* abtaster[kAbtasterPlaetze]{};
    VsObjekt* vs = nullptr;
    PsObjekt* ps = nullptr;
    GsObjekt* gs = nullptr;
    Eingabe* eingabe = nullptr;
    Mischung* mischung = nullptr;
    TiefeZustand* tiefe = nullptr;
    Raster* raster = nullptr;
    float mischFaktor[4]{1.0F, 1.0F, 1.0F, 1.0F};
    UINT probenMaske = 0xFFFFFFFFU;
    UINT stencilWert = 0;
    ZielSicht* ziel[kZiele]{};
    UINT ziele = 0;
    TiefeSicht* tiefeZiel = nullptr;
    Puffer* vb[kVbPlaetze]{};
    UINT vbSchritt[kVbPlaetze]{};
    UINT vbVersatz[kVbPlaetze]{};
    Puffer* ib = nullptr;
    DXGI_FORMAT ibFormat = DXGI_FORMAT_R32_UINT;
    UINT ibVersatz = 0;
    D3D11_PRIMITIVE_TOPOLOGY topologie = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    D3D12_VIEWPORT sichtfeld[4]{};
    UINT sichtfelder = 0;
    D3D12_RECT schere[4]{};
    UINT scheren = 0;
    // was vor dem naechsten Zeichenbefehl in die Liste muss
    std::uint32_t schmutz = ~0U;
    std::uint32_t cbSchmutz = ~0U;        // Bit = Stufe * kCbSchritt + Register
    std::uint32_t srvSchmutz = ~0U;
    std::uint32_t abtasterSchmutz = ~0U;
    ID3D12GraphicsCommandList* offeneListe = nullptr;   // nullptr: beim naechsten Befehl vom Rahmen holen
    std::uint32_t teilBild = ~0U;                      // Teilabgaben: das Bild (g_gebrauchBild) ...
    int teilImBild = 0;                                // ... und wie viele darin schon abgingen
    long long listeSeit = 0;                           // QueryPerformanceCounter beim Oeffnen der Liste (Teilabgabe); 0: noch keine Arbeit
    std::uint32_t befehleInListe = 0;                  // D3D12-5: Zeichenbefehle und UpdateSubresource in der offenen Liste (g_listeBefehle)
    // D3D12-4: Puffer, die in der offenen Liste nach UpdateSubresource in COPY_DEST stehen; vor dem naechsten Zeichenbefehl alle
    // zusammen zurueck in ihren Lesezustand (kopierZieleLesbar).
    std::vector<Puffer*> kopierZiele;
    ID3D12PipelineState* psoJetzt = nullptr;   // in der Liste gesetzt
    PsoSchluessel schluesselJetzt;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** aus) override;
    ULONG STDMETHODCALLTYPE AddRef() override { return 2; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    void STDMETHODCALLTYPE GetDevice(ID3D11Device** d) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID, UINT*, void*) override { return DXGI_ERROR_NOT_FOUND; }
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID, UINT, const void*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID, const IUnknown*) override { return S_OK; }
    void STDMETHODCALLTYPE VSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) override;
    void STDMETHODCALLTYPE PSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) override;
    void STDMETHODCALLTYPE PSSetShader(ID3D11PixelShader *pPixelShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) override;
    void STDMETHODCALLTYPE PSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers) override;
    void STDMETHODCALLTYPE VSSetShader(ID3D11VertexShader *pVertexShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) override;
    void STDMETHODCALLTYPE DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) override;
    void STDMETHODCALLTYPE Draw(UINT VertexCount, UINT StartVertexLocation) override;
    HRESULT STDMETHODCALLTYPE Map(ID3D11Resource *pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE *pMappedResource) override;
    void STDMETHODCALLTYPE Unmap(ID3D11Resource *pResource, UINT Subresource) override;
    void STDMETHODCALLTYPE PSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) override;
    void STDMETHODCALLTYPE IASetInputLayout(ID3D11InputLayout *pInputLayout) override;
    void STDMETHODCALLTYPE IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppVertexBuffers, const UINT *pStrides, const UINT *pOffsets) override;
    void STDMETHODCALLTYPE IASetIndexBuffer(ID3D11Buffer *pIndexBuffer, DXGI_FORMAT Format, UINT Offset) override;
    void STDMETHODCALLTYPE DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation) override;
    void STDMETHODCALLTYPE DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation) override;
    void STDMETHODCALLTYPE GSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) override;
    void STDMETHODCALLTYPE GSSetShader(ID3D11GeometryShader *pShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) override;
    void STDMETHODCALLTYPE IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology) override;
    void STDMETHODCALLTYPE VSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) override;
    void STDMETHODCALLTYPE VSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers) override;
    void STDMETHODCALLTYPE Begin(ID3D11Asynchronous *pAsync) override;
    void STDMETHODCALLTYPE End(ID3D11Asynchronous *pAsync) override;
    HRESULT STDMETHODCALLTYPE GetData(ID3D11Asynchronous *pAsync, void *pData, UINT DataSize, UINT GetDataFlags) override;
    void STDMETHODCALLTYPE SetPredication(ID3D11Predicate *pPredicate, BOOL PredicateValue) override;
    void STDMETHODCALLTYPE GSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) override;
    void STDMETHODCALLTYPE GSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers) override;
    void STDMETHODCALLTYPE OMSetRenderTargets(UINT NumViews, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView) override;
    void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView *const *ppRenderTargetViews, ID3D11DepthStencilView *pDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts) override;
    void STDMETHODCALLTYPE OMSetBlendState(ID3D11BlendState *pBlendState, const FLOAT BlendFactor[ 4 ], UINT SampleMask) override;
    void STDMETHODCALLTYPE OMSetDepthStencilState(ID3D11DepthStencilState *pDepthStencilState, UINT StencilRef) override;
    void STDMETHODCALLTYPE SOSetTargets(UINT NumBuffers, ID3D11Buffer *const *ppSOTargets, const UINT *pOffsets) override;
    void STDMETHODCALLTYPE DrawAuto() override;
    void STDMETHODCALLTYPE DrawIndexedInstancedIndirect(ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs) override;
    void STDMETHODCALLTYPE DrawInstancedIndirect(ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs) override;
    void STDMETHODCALLTYPE Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ) override;
    void STDMETHODCALLTYPE DispatchIndirect(ID3D11Buffer *pBufferForArgs, UINT AlignedByteOffsetForArgs) override;
    void STDMETHODCALLTYPE RSSetState(ID3D11RasterizerState *pRasterizerState) override;
    void STDMETHODCALLTYPE RSSetViewports(UINT NumViewports, const D3D11_VIEWPORT *pViewports) override;
    void STDMETHODCALLTYPE RSSetScissorRects(UINT NumRects, const D3D11_RECT *pRects) override;
    void STDMETHODCALLTYPE CopySubresourceRegion(ID3D11Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource *pSrcResource, UINT SrcSubresource, const D3D11_BOX *pSrcBox) override;
    void STDMETHODCALLTYPE CopyResource(ID3D11Resource *pDstResource, ID3D11Resource *pSrcResource) override;
    void STDMETHODCALLTYPE UpdateSubresource(ID3D11Resource *pDstResource, UINT DstSubresource, const D3D11_BOX *pDstBox, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) override;
    void STDMETHODCALLTYPE CopyStructureCount(ID3D11Buffer *pDstBuffer, UINT DstAlignedByteOffset, ID3D11UnorderedAccessView *pSrcView) override;
    void STDMETHODCALLTYPE ClearRenderTargetView(ID3D11RenderTargetView *pRenderTargetView, const FLOAT ColorRGBA[ 4 ]) override;
    void STDMETHODCALLTYPE ClearUnorderedAccessViewUint(ID3D11UnorderedAccessView *pUnorderedAccessView, const UINT Values[ 4 ]) override;
    void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat(ID3D11UnorderedAccessView *pUnorderedAccessView, const FLOAT Values[ 4 ]) override;
    void STDMETHODCALLTYPE ClearDepthStencilView(ID3D11DepthStencilView *pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil) override;
    void STDMETHODCALLTYPE GenerateMips(ID3D11ShaderResourceView *pShaderResourceView) override;
    void STDMETHODCALLTYPE SetResourceMinLOD(ID3D11Resource *pResource, FLOAT MinLOD) override;
    FLOAT STDMETHODCALLTYPE GetResourceMinLOD(ID3D11Resource *pResource) override;
    void STDMETHODCALLTYPE ResolveSubresource(ID3D11Resource *pDstResource, UINT DstSubresource, ID3D11Resource *pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format) override;
    void STDMETHODCALLTYPE ExecuteCommandList(ID3D11CommandList *pCommandList, BOOL RestoreContextState) override;
    void STDMETHODCALLTYPE HSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) override;
    void STDMETHODCALLTYPE HSSetShader(ID3D11HullShader *pHullShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) override;
    void STDMETHODCALLTYPE HSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers) override;
    void STDMETHODCALLTYPE HSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) override;
    void STDMETHODCALLTYPE DSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) override;
    void STDMETHODCALLTYPE DSSetShader(ID3D11DomainShader *pDomainShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) override;
    void STDMETHODCALLTYPE DSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers) override;
    void STDMETHODCALLTYPE DSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) override;
    void STDMETHODCALLTYPE CSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) override;
    void STDMETHODCALLTYPE CSSetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, const UINT *pUAVInitialCounts) override;
    void STDMETHODCALLTYPE CSSetShader(ID3D11ComputeShader *pComputeShader, ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances) override;
    void STDMETHODCALLTYPE CSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState *const *ppSamplers) override;
    void STDMETHODCALLTYPE CSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) override;
    void STDMETHODCALLTYPE VSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers) override;
    void STDMETHODCALLTYPE PSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews) override;
    void STDMETHODCALLTYPE PSGetShader(ID3D11PixelShader **ppPixelShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances) override;
    void STDMETHODCALLTYPE PSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers) override;
    void STDMETHODCALLTYPE VSGetShader(ID3D11VertexShader **ppVertexShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances) override;
    void STDMETHODCALLTYPE PSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers) override;
    void STDMETHODCALLTYPE IAGetInputLayout(ID3D11InputLayout **ppInputLayout) override;
    void STDMETHODCALLTYPE IAGetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppVertexBuffers, UINT *pStrides, UINT *pOffsets) override;
    void STDMETHODCALLTYPE IAGetIndexBuffer(ID3D11Buffer **pIndexBuffer, DXGI_FORMAT *Format, UINT *Offset) override;
    void STDMETHODCALLTYPE GSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers) override;
    void STDMETHODCALLTYPE GSGetShader(ID3D11GeometryShader **ppGeometryShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances) override;
    void STDMETHODCALLTYPE IAGetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY *pTopology) override;
    void STDMETHODCALLTYPE VSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews) override;
    void STDMETHODCALLTYPE VSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers) override;
    void STDMETHODCALLTYPE GetPredication(ID3D11Predicate **ppPredicate, BOOL *pPredicateValue) override;
    void STDMETHODCALLTYPE GSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews) override;
    void STDMETHODCALLTYPE GSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers) override;
    void STDMETHODCALLTYPE OMGetRenderTargets(UINT NumViews, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView) override;
    void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView **ppRenderTargetViews, ID3D11DepthStencilView **ppDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView **ppUnorderedAccessViews) override;
    void STDMETHODCALLTYPE OMGetBlendState(ID3D11BlendState **ppBlendState, FLOAT BlendFactor[ 4 ], UINT *pSampleMask) override;
    void STDMETHODCALLTYPE OMGetDepthStencilState(ID3D11DepthStencilState **ppDepthStencilState, UINT *pStencilRef) override;
    void STDMETHODCALLTYPE SOGetTargets(UINT NumBuffers, ID3D11Buffer **ppSOTargets) override;
    void STDMETHODCALLTYPE RSGetState(ID3D11RasterizerState **ppRasterizerState) override;
    void STDMETHODCALLTYPE RSGetViewports(UINT *pNumViewports, D3D11_VIEWPORT *pViewports) override;
    void STDMETHODCALLTYPE RSGetScissorRects(UINT *pNumRects, D3D11_RECT *pRects) override;
    void STDMETHODCALLTYPE HSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews) override;
    void STDMETHODCALLTYPE HSGetShader(ID3D11HullShader **ppHullShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances) override;
    void STDMETHODCALLTYPE HSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers) override;
    void STDMETHODCALLTYPE HSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers) override;
    void STDMETHODCALLTYPE DSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews) override;
    void STDMETHODCALLTYPE DSGetShader(ID3D11DomainShader **ppDomainShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances) override;
    void STDMETHODCALLTYPE DSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers) override;
    void STDMETHODCALLTYPE DSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers) override;
    void STDMETHODCALLTYPE CSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView **ppShaderResourceViews) override;
    void STDMETHODCALLTYPE CSGetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView **ppUnorderedAccessViews) override;
    void STDMETHODCALLTYPE CSGetShader(ID3D11ComputeShader **ppComputeShader, ID3D11ClassInstance **ppClassInstances, UINT *pNumClassInstances) override;
    void STDMETHODCALLTYPE CSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState **ppSamplers) override;
    void STDMETHODCALLTYPE CSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers) override;
    void STDMETHODCALLTYPE ClearState() override;
    void STDMETHODCALLTYPE Flush() override;
    D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType() override;
    UINT STDMETHODCALLTYPE GetContextFlags() override;
    HRESULT STDMETHODCALLTYPE FinishCommandList(BOOL RestoreDeferredContextState, ID3D11CommandList **ppCommandList) override;
    void STDMETHODCALLTYPE CopySubresourceRegion1(ID3D11Resource *pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource *pSrcResource, UINT SrcSubresource, const D3D11_BOX *pSrcBox, UINT CopyFlags) override;
    void STDMETHODCALLTYPE UpdateSubresource1(ID3D11Resource *pDstResource, UINT DstSubresource, const D3D11_BOX *pDstBox, const void *pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, UINT CopyFlags) override;
    void STDMETHODCALLTYPE DiscardResource(ID3D11Resource *pResource) override;
    void STDMETHODCALLTYPE DiscardView(ID3D11View *pResourceView) override;
    void STDMETHODCALLTYPE VSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) override;
    void STDMETHODCALLTYPE HSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) override;
    void STDMETHODCALLTYPE DSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) override;
    void STDMETHODCALLTYPE GSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) override;
    void STDMETHODCALLTYPE PSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) override;
    void STDMETHODCALLTYPE CSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers, const UINT *pFirstConstant, const UINT *pNumConstants) override;
    void STDMETHODCALLTYPE VSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants) override;
    void STDMETHODCALLTYPE HSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants) override;
    void STDMETHODCALLTYPE DSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants) override;
    void STDMETHODCALLTYPE GSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants) override;
    void STDMETHODCALLTYPE PSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants) override;
    void STDMETHODCALLTYPE CSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer **ppConstantBuffers, UINT *pFirstConstant, UINT *pNumConstants) override;
    void STDMETHODCALLTYPE SwapDeviceContextState(ID3DDeviceContextState *pState, ID3DDeviceContextState **ppPreviousState) override;
    void STDMETHODCALLTYPE ClearView(ID3D11View *pView, const FLOAT Color[ 4 ], const D3D11_RECT *pRect, UINT NumRects) override;
    void STDMETHODCALLTYPE DiscardView1(ID3D11View *pResourceView, const D3D11_RECT *pRects, UINT NumRects) override;

    // eigene Helfer
    ID3D12GraphicsCommandList* liste() { return offeneListe != nullptr ? offeneListe : rahmen::liste(); }
    bool vorZeichnen();
    void kopierZieleLesbar();
    UINT kopierZieleSammeln(D3D12_RESOURCE_BARRIER* b, UINT frei);
    UINT zieleSammeln(D3D12_CPU_DESCRIPTOR_HANDLE* griffe, D3D12_RESOURCE_BARRIER* b, UINT frei);
    void setzeCb(UINT stufe, UINT anfang, UINT anzahl, ID3D11Buffer* const* puffer, const UINT* erste);
    void allesLoesen();
    bool istZiel(const Textur* t) const;
    void pufferUmbenannt(const Puffer* p);
    void bindeCb(UINT stufe, UINT platz, Puffer* neu);
    void bindeVb(UINT platz, Puffer* neu);
    void bindeIb(Puffer* neu);
    void bindeSrv(UINT platz, Sicht* neu);
};
[[nodiscard]] Kontext* kontextZeiger();
// D3D12-4: steht dieser Puffer nach UpdateSubresource noch in COPY_DEST, jetzt (mit allen anderen) zurueck in den Lesezustand.
void kopierZieleLesbar();

}  // namespace bhed::render::d3d12::abgeber
#endif
