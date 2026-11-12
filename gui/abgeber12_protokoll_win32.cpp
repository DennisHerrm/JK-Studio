// abgeber12_protokoll_win32.cpp - D3D12-Abgeber: das Bildprotokoll (Messhilfe)
//
// BHED_D3D12_PROTOKOLL=<n> schreibt fuer jedes n-te Bild auf, was der Abgeber in die Befehlsliste gibt: Ziele setzen, Loeschen,
// Aufloesen, Kopieren, Hochladen, Mip-Stufen, Zustandswechsel der Texturen, Abschicken - je Eintrag mit einer Zeitmarke der
// Grafikkarte davor. Im Protokoll (jkstudio-detail.log) steht dann je Eintrag, wann der Hauptfaden ihn schrieb, wann die
// Grafikkarte ihn erreichte, wie lange sie bis zum naechsten Eintrag brauchte und wie viele Zeichenbefehle dazwischen lagen.
// Dazu je Protokoll die Zaehler des Abgebers und des Rahmens je Bild seit dem vorigen Protokoll. Ohne die Variable kostet das
// Protokoll einen Vergleich je Eintragsstelle (nicht je Zeichenbefehl).
// Damit wurden die Ursachen in docs/intern/NOTIZEN_d3d12_1011.md ("Zweite Runde") gefunden; Auswertung:
// Y:\werkzeuge\dx12\protokoll.py.

#include <windows.h>

#include "abgeber12_intern.h"

#include <cstdio>
#include <cstdlib>
#include <deque>

#include "bhed/diag.h"

namespace bhed::render::d3d12::abgeber::protokoll {

bool g_an = false;

namespace {

constexpr std::size_t kHoechstens = 1500;   // Eintraege mit Zeitmarke je Bild (der Ring der Zeitmarken hat 2048 Plaetze)

struct Eintrag {
    std::string text;
    int marke = -1;             // Platz der Zeitmarke davor (-1: keine)
    double cpuMs = 0.0;         // seit dem Anfang des Bildes
    std::uint64_t zeichnen = 0; // Stand der Zaehler beim Eintrag
    std::uint64_t pso = 0;
    std::uint64_t gewartet = 0;
    double warteMs = 0.0;
};

struct Bild {
    std::uint64_t nummer = 0;
    std::uint64_t zaun = 0;
    std::vector<Eintrag> eintraege;
    std::string kopf;
};

int g_jedes = -1;   // -1: die Umgebung ist noch nicht gefragt
std::uint64_t g_bildNummer = 0;
std::uint64_t g_bilderSeit = 0;
Bild g_jetzt;
std::deque<Bild> g_wartend;
LARGE_INTEGER g_bildAnfang{};
LARGE_INTEGER g_takt{};
Zaehler g_zaehlerVor;
rahmen::Statistik g_statistikVor;
std::uint64_t g_gewartetVor = 0;   // Stand beim vorigen Eintrag
double g_warteVor = 0.0;
std::uint64_t g_speicherVor = 0;   // Grafikspeicher beim letzten Speicherbericht

double seitAnfangMs() {
    LARGE_INTEGER t{};
    QueryPerformanceCounter(&t);
    return g_takt.QuadPart > 0 ? static_cast<double>(t.QuadPart - g_bildAnfang.QuadPart) * 1000.0 / static_cast<double>(g_takt.QuadPart) : 0.0;
}

const char* formatName(DXGI_FORMAT f) {
    switch (f) {
    case DXGI_FORMAT_R8G8B8A8_UNORM: return "RGBA8";
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return "RGBA8s";
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: return "RGBA8t";
    case DXGI_FORMAT_B8G8R8A8_UNORM: return "BGRA8";
    case DXGI_FORMAT_R16G16B16A16_FLOAT: return "RGBA16F";
    case DXGI_FORMAT_R32G32B32A32_FLOAT: return "RGBA32F";
    case DXGI_FORMAT_R32_FLOAT: return "R32F";
    case DXGI_FORMAT_R32_TYPELESS: return "R32t";
    case DXGI_FORMAT_D32_FLOAT: return "D32F";
    case DXGI_FORMAT_R24G8_TYPELESS: return "R24G8t";
    case DXGI_FORMAT_D24_UNORM_S8_UINT: return "D24S8";
    case DXGI_FORMAT_R32G8X24_TYPELESS: return "R32G8X24t";
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return "D32FS8";
    case DXGI_FORMAT_R8_UNORM: return "R8";
    case DXGI_FORMAT_R16_FLOAT: return "R16F";
    case DXGI_FORMAT_BC1_UNORM: return "BC1";
    case DXGI_FORMAT_BC3_UNORM: return "BC3";
    default: return nullptr;
    }
}

void schreibe(const Bild& b) {
    UINT64 frequenz = 0;
    if (rahmen::schlange() == nullptr || FAILED(rahmen::schlange()->GetTimestampFrequency(&frequenz)) || frequenz == 0U) { return; }
    std::uint64_t erste = 0;
    std::uint64_t letzte = 0;
    bool hatErste = false;
    std::vector<std::uint64_t> wert(b.eintraege.size(), 0);
    for (std::size_t i = 0; i < b.eintraege.size(); ++i) {
        if (b.eintraege[i].marke < 0 || !markeLesen(b.eintraege[i].marke, wert[i])) { continue; }
        if (!hatErste) {
            erste = wert[i];
            hatErste = true;
        }
        letzte = wert[i];
    }
    const double jeTick = 1000.0 / static_cast<double>(frequenz);
    char z[160];
    std::snprintf(z, sizeof(z), "D3D12-Protokoll Bild %llu: %zu Eintraege, Grafikkarte von der ersten bis zur letzten Marke %.3f ms; ",
                  static_cast<unsigned long long>(b.nummer), b.eintraege.size(), static_cast<double>(letzte - erste) * jeTick);
    diag::detail(z + b.kopf);
    for (std::size_t i = 0; i < b.eintraege.size(); ++i) {
        const Eintrag& e = b.eintraege[i];
        // die naechste Marke: bis dahin rechnet die Grafikkarte an diesem Eintrag und den Zeichenbefehlen danach
        std::size_t n = i + 1U;
        while (n < b.eintraege.size() && b.eintraege[n].marke < 0) { ++n; }
        const std::uint64_t zeichnen = (i + 1U < b.eintraege.size() ? b.eintraege[i + 1U].zeichnen : e.zeichnen) - e.zeichnen;
        const std::uint64_t pso = (i + 1U < b.eintraege.size() ? b.eintraege[i + 1U].pso : e.pso) - e.pso;
        char kopf[160];
        if (e.marke >= 0 && n < b.eintraege.size()) {
            std::snprintf(kopf, sizeof(kopf), "D3D12-P %5llu cpu %7.3f gpu %7.3f +%6.3f z %4llu p %3llu | ", static_cast<unsigned long long>(b.nummer), e.cpuMs,
                          static_cast<double>(wert[i] - erste) * jeTick, static_cast<double>(wert[n] - wert[i]) * jeTick,
                          static_cast<unsigned long long>(zeichnen), static_cast<unsigned long long>(pso));
        } else if (e.marke >= 0) {
            std::snprintf(kopf, sizeof(kopf), "D3D12-P %5llu cpu %7.3f gpu %7.3f +     - z %4llu p %3llu | ", static_cast<unsigned long long>(b.nummer), e.cpuMs,
                          static_cast<double>(wert[i] - erste) * jeTick, static_cast<unsigned long long>(zeichnen), static_cast<unsigned long long>(pso));
        } else {
            std::snprintf(kopf, sizeof(kopf), "D3D12-P %5llu cpu %7.3f gpu       - +     - z %4llu p %3llu | ", static_cast<unsigned long long>(b.nummer), e.cpuMs,
                          static_cast<unsigned long long>(zeichnen), static_cast<unsigned long long>(pso));
        }
        std::string zeile = kopf + e.text;
        if (e.gewartet != 0U) {
            char w[80];
            std::snprintf(w, sizeof(w), "  [davor gewartet: %llu-mal, %.3f ms]", static_cast<unsigned long long>(e.gewartet), e.warteMs);
            zeile += w;
        }
        diag::detail(zeile);
    }
}

}  // namespace

const char* zustandName(D3D12_RESOURCE_STATES z) {
    switch (static_cast<UINT>(z)) {
    case D3D12_RESOURCE_STATE_COMMON: return "COMMON";
    case D3D12_RESOURCE_STATE_RENDER_TARGET: return "ZIEL";
    case D3D12_RESOURCE_STATE_DEPTH_WRITE: return "TIEFE";
    case D3D12_RESOURCE_STATE_DEPTH_READ: return "TIEFE_LESEN";
    case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE: return "LESBAR";
    case D3D12_RESOURCE_STATE_COPY_DEST: return "KOPIE_ZIEL";
    case D3D12_RESOURCE_STATE_COPY_SOURCE: return "KOPIE_QUELLE";
    case D3D12_RESOURCE_STATE_RESOLVE_DEST: return "AUFLOESEN_ZIEL";
    case D3D12_RESOURCE_STATE_RESOLVE_SOURCE: return "AUFLOESEN_QUELLE";
    default: return "?";
    }
}

std::string nenne(const Textur* t) {
    if (t == nullptr) { return "-"; }
    char z[120];
    const char* f = formatName(t->art.Format);
    char fz[24];
    if (f == nullptr) {
        std::snprintf(fz, sizeof(fz), "f%u", static_cast<unsigned>(t->art.Format));
        f = fz;
    }
    std::snprintf(z, sizeof(z), "#%u %ux%u %s x%u%s%s", t->kennung, t->art.Width, t->art.Height, f, t->art.SampleDesc.Count,
                  t->art.MipLevels > 1U ? " mips" : "", t->art.ArraySize > 1U ? " feld" : "");
    return z;
}

void eintrag(ID3D12GraphicsCommandList* cl, std::string text) {
    if (!g_an) { return; }
    Eintrag e;
    e.text = std::move(text);
    e.cpuMs = seitAnfangMs();
    const Zaehler& zz = zaehlerIntern();
    e.zeichnen = zz.zeichnen;
    e.pso = zz.psoWechsel;
    const rahmen::Statistik s = rahmen::statistik();
    e.gewartet = s.gewartet - g_gewartetVor;
    e.warteMs = s.warteMs - g_warteVor;
    g_gewartetVor = s.gewartet;
    g_warteVor = s.warteMs;
    if (cl != nullptr && markenStapel() != nullptr && g_jetzt.eintraege.size() < kHoechstens) {
        e.marke = markeNehmen();
        if (e.marke >= 0) { cl->EndQuery(markenStapel(), D3D12_QUERY_TYPE_TIMESTAMP, static_cast<UINT>(e.marke)); }
    }
    g_jetzt.eintraege.push_back(std::move(e));
}

void bildWechsel() {
    if (g_jedes < 0) {
        const char* v = std::getenv("BHED_D3D12_PROTOKOLL");
        g_jedes = v != nullptr ? std::atoi(v) : 0;
        QueryPerformanceFrequency(&g_takt);
        QueryPerformanceCounter(&g_bildAnfang);
        g_zaehlerVor = zaehler();
        g_statistikVor = rahmen::statistik();
    }
    if (g_jedes <= 0) { return; }
    while (!g_wartend.empty() && rahmen::fertig(g_wartend.front().zaun)) {
        schreibe(g_wartend.front());
        g_wartend.pop_front();
    }
    ++g_bildNummer;
    ++g_bilderSeit;
    if (g_an) {
        // das Bild ist abgeschickt: seine Marken stehen, sobald die Grafikkarte den letzten Zaunwert erreicht hat
        const Zaehler z = zaehler();
        const rahmen::Statistik s = rahmen::statistik();
        const double n = static_cast<double>(g_bilderSeit);
        char k[560];
        std::snprintf(k, sizeof(k),
                      "Hauptfaden %.3f ms; seit dem vorigen Protokoll %llu Bilder, je Bild: %.1f Zeichenbefehle, %.1f PSO-Wechsel, %.1f frische Bloecke, "
                      "%.1f Zustandswechsel, %.2f-mal abgeschickt, %.2f-mal gewartet (%.3f ms), %.1f KB hochgeladen, %.2f Hochladepuffer angelegt, "
                      "%.2f Ressourcen angelegt (%.2f Puffer aus dem Vorrat), %.2f ruhende Puffer in den Grafikspeicher",
                      seitAnfangMs(), static_cast<unsigned long long>(g_bilderSeit), static_cast<double>(z.zeichnen - g_zaehlerVor.zeichnen) / n,
                      static_cast<double>(z.psoWechsel - g_zaehlerVor.psoWechsel) / n, static_cast<double>(z.umbenannt - g_zaehlerVor.umbenannt) / n,
                      static_cast<double>(z.barrieren - g_zaehlerVor.barrieren) / n, static_cast<double>(s.abgeschickt - g_statistikVor.abgeschickt) / n,
                      static_cast<double>(s.gewartet - g_statistikVor.gewartet) / n, (s.warteMs - g_statistikVor.warteMs) / n,
                      static_cast<double>(s.hochladeBytes - g_statistikVor.hochladeBytes) / n / 1024.0,
                      static_cast<double>(s.hochladePuffer - g_statistikVor.hochladePuffer) / n, static_cast<double>(z.angelegt - g_zaehlerVor.angelegt) / n,
                      static_cast<double>(z.ausVorrat - g_zaehlerVor.ausVorrat) / n,
                      static_cast<double>(z.ruhend - g_zaehlerVor.ruhend) / n);
        g_jetzt.kopf = k;
        std::uint64_t summe = 0;
        const std::vector<std::string> bericht = speicherBericht(summe);
        if (summe != g_speicherVor) {
            g_speicherVor = summe;
            for (const std::string& zeile : bericht) { diag::detail(zeile); }
        }
        g_jetzt.zaun = rahmen::letzterZaun();
        g_wartend.push_back(std::move(g_jetzt));
        g_jetzt = Bild{};
        g_zaehlerVor = z;
        g_statistikVor = s;
        g_bilderSeit = 0;
    }
    g_an = g_bildNummer % static_cast<std::uint64_t>(g_jedes) == 0U;
    g_jetzt.nummer = g_bildNummer;
    QueryPerformanceCounter(&g_bildAnfang);
}

void abbauen() {
    g_an = false;
    g_jedes = -1;
    g_jetzt = Bild{};
    g_wartend.clear();
    g_bildNummer = 0;
    g_bilderSeit = 0;
    g_gewartetVor = 0;
    g_warteVor = 0.0;
    g_speicherVor = 0;
}

}  // namespace bhed::render::d3d12::abgeber::protokoll
