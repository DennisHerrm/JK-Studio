// abgeber12.h - D3D12: der zweite Abgeber der Zeichenbefehle (nur Windows; fuer backend_d3d12_win32.cpp und spaetere D3D12-Durchgaenge)
//
// Die Zeichenschichten (gpumap, kartegpu, Echtzeit-Licht, Model-Tab) gibt es EINMAL. Ihre Befehlsfolge - Zustaende setzen,
// Puffer fuellen, zeichnen - geht an die Schnittstelle ID3D11Device/ID3D11DeviceContext. Zwei Abgeber fuehren sie aus:
//   - Direct3D 11: die Laufzeit selbst (render::d3dDevice() ist das echte Geraet),
//   - Direct3D 12: dieser Abgeber. Er bedient dieselbe Schnittstelle und schreibt jeden Befehl sofort als D3D12-Befehl in die
//     eine Befehlsliste des Bildes (kein D3D11On12, kein zweites Geraet): Zustandsobjekte werden zu Pipeline-Zustaenden (PSO,
//     nach Schluessel gemerkt), Konstantenpuffer zu Root-CBVs mit Adresse, Texturen zu festen Deskriptoren, DISCARD zu einem
//     frischen Block im Hochladespeicher.
// Dieselben DXBC-Shader, dieselben Zustaende, dieselbe Reihenfolge - deshalb dasselbe Bild. Plan und Messwerte:
// docs/intern/NOTIZEN_d3d12_1011.md.
//
// Der Rahmen (Geraet, Warteschlange, Swapchain, Bildplaetze, die Befehlsliste) gehoert backend_d3d12_win32.cpp; was der Abgeber
// davon braucht, steht unter rahmen::. Was der Rahmen und spaetere eigene D3D12-Durchgaenge (DXR-Schatten) vom Abgeber
// brauchen, unter abgeber::.
#ifndef BHED_GUI_ABGEBER12_H
#define BHED_GUI_ABGEBER12_H

#include <cstdint>
#include <string>
#include <vector>

#include <d3d11_1.h>
#include <d3d12.h>

namespace bhed::render::d3d12 {

namespace rahmen {

[[nodiscard]] ID3D12Device* geraet();
[[nodiscard]] ID3D12CommandQueue* schlange();
// Die Befehlsliste des Bildes. Ist sie zu, wird sie geoeffnet (dann ruft der Rahmen abgeber::listeNeu).
[[nodiscard]] ID3D12GraphicsCommandList* liste();
// Die Liste fuer das Hochladen neuer Texturen und Puffer; sie laeuft VOR der Befehlsliste des Bildes.
[[nodiscard]] ID3D12GraphicsCommandList* vorListe();
// Platz im Hochladespeicher. vor: fuer die Vor-Liste (sonst fuer die Befehlsliste des Bildes). Gilt, bis die Grafikkarte die
// Liste abgearbeitet hat. Ausrichtung 512 (D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT).
bool hochladePlatz(std::uint64_t groesse, bool vor, ID3D12Resource*& puffer, std::uint64_t& versatz, unsigned char*& zeiger);
// Der Deskriptorstapel der Texturen (einer fuer Oberflaeche und 3D-Ansichten). srvNehmen: Platz oder srvKeiner().
[[nodiscard]] ID3D12DescriptorHeap* srvStapel();
[[nodiscard]] UINT srvNehmen();
[[nodiscard]] UINT srvKeiner();
void srvSpaeterFrei(UINT index);   // erst wieder vergeben, wenn keine Liste in Arbeit ihn mehr liest
[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE srvCpu(UINT index);
[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE srvGpu(UINT index);
// Ein D3D12-Objekt freigeben, sobald die Grafikkarte alle Listen abgearbeitet hat, die es nennen koennen.
void spaeterFrei(IUnknown* objekt);
// Die Befehlsliste des Bildes (mit der Vor-Liste davor) jetzt abschicken; gibt den Zaunwert danach zurueck.
std::uint64_t abschicken();
// Der Zaunwert der letzten abgeschickten Liste (0: noch keine).
[[nodiscard]] std::uint64_t letzterZaun();
void warten(std::uint64_t zaun);
[[nodiscard]] bool fertig(std::uint64_t zaun);
// Zaehlt jedes ExecuteCommandLists: Puffer im Grafikspeicher fallen danach auf den Zustand COMMON zurueck.
[[nodiscard]] std::uint64_t ausfuehrung();
// D3D12-4: Leerlauf der Grafikkarte zwischen den Abgaben (Takte der Zeitmarken), aufsummiert bis einschliesslich der Abgabe
// `serie` (ausfuehrung() beim Aufzeichnen). Nur fuer Abgaben, deren Zaunwert erreicht ist. Eine Zeitmarke in Abgabe b minus
// leerlaufBis(b) ist eine Uhr, die nur laeuft, solange die Grafikkarte Listen dieses Programms abarbeitet: Die Teilabgabe laesst
// sie zwischen den Teilen auf den Prozessor warten, der D3D11-Treiber zeigt das in seinen Zeitmarken nicht.
[[nodiscard]] std::uint64_t leerlaufBis(std::uint64_t serie);
// Zaehler des Rahmens fuer Messung und Bildprotokoll (seit dem Start).
struct Statistik {
    std::uint64_t abgeschickt = 0;      // ExecuteCommandLists
    std::uint64_t gewartet = 0;         // Warten auf einen Zaunwert, das die Grafikkarte noch nicht erreicht hatte
    double warteMs = 0.0;
    double abwartenMs = 0.0;            // D3D12-4: Hauptfaden wartet auf den Abschickfaden (abschickenAbwarten)
    std::uint64_t hochladeBytes = 0;    // ueber hochladePlatz vergeben
    std::uint64_t hochladePuffer = 0;   // dafuer angelegte Hochladepuffer
    std::uint64_t hochladeJetzt = 0;    // Bytes der Hochladepuffer, die die Bildplaetze gerade halten
    std::uint64_t spaeter = 0;          // Objekte, die auf ihren Zaunwert warten (noch nicht freigegeben)
};
[[nodiscard]] Statistik statistik();

}  // namespace rahmen

namespace abgeber {

// Wurzelsignatur, Deskriptorstapel (Ziele, Tiefe, Abtaster), Geraet- und Kontextobjekt. false: geht nicht (dann Direct3D 11).
bool anlegen();
// Alles freigeben. Die Zeichenschichten haben ihre Objekte vorher freigegeben (gpu::shutdown); was noch lebt, wird gezaehlt
// und gemeldet.
void abbauen();
[[nodiscard]] ID3D11Device* geraet();
[[nodiscard]] ID3D11DeviceContext* kontext();

// --- vom Rahmen gerufen ---
// Die Befehlsliste des Bildes wurde geoeffnet: Deskriptorstapel und Wurzelsignatur setzen, alle Zustaende gelten als neu.
void listeNeu(ID3D12GraphicsCommandList* l);
// Jemand anders (imgui_impl_dx12) hat in die Befehlsliste gezeichnet: Zustaende vor dem naechsten eigenen Befehl neu setzen.
void fremdGezeichnet();
// Unmittelbar vor dem Abschicken der offenen Liste (Zeitmarken aufloesen).
void vorAbschicken(ID3D12GraphicsCommandList* l);
// Nach dem Abschicken: die Liste ist zu; freigegebene Bloecke und Abfragen haengen an diesem Zaunwert.
void nachAbschicken(std::uint64_t zaun);
// Einmal je Bild: einsammeln, was die Grafikkarte abgearbeitet hat.
void bildAnfang();
// Nur einsammeln (kein neues Bild).
void einsammeln();
// Ein Eintrag des Rahmens im Bildprotokoll (abgeber12_protokoll_win32.cpp), mit einer Zeitmarke in l.
void protokollNotiz(ID3D12GraphicsCommandList* l, const char* text);

// --- Bruecke zur Oberflaeche (imgui_impl_dx12) ---
// Ist diese Kennung eine Sicht (ID3D11ShaderResourceView*) des Abgebers?
[[nodiscard]] bool istSicht(std::uint64_t kennung);
// Die Sicht in der Oberflaeche zeigen: ihr Deskriptor (GPU-Adresse im gemeinsamen Stapel); die Textur wird in der Liste l
// lesbar gestellt. ressource: die Textur dahinter (ID3D11Resource*, fuer "in diesem Bild bemalt").
[[nodiscard]] std::uint64_t sichtZeigen(std::uint64_t kennung, ID3D12GraphicsCommandList* l, const void** ressource);
// Eine Textur als Renderziel fuer die Oberflaeche (render::zeichneInD3d11Ziel): stellt sie in l als Ziel und gibt den
// Deskriptor zurueck; ptr == 0: geht nicht (kein Renderziel, anderes Format als R8G8B8A8_UNORM).
[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE zielFuerOberflaeche(ID3D11Resource* textur, ID3D12GraphicsCommandList* l);

// --- fuer eigene D3D12-Durchgaenge (DXR-Schatten, Rechen-Shader) ---
// Die D3D12-Ressource hinter einer Textur oder einem Puffer des Abgebers (ohne AddRef; nullptr: keine). Bei Puffern mit
// D3D11_USAGE_DYNAMIC gilt sie bis zum naechsten Map(WRITE_DISCARD); versatz ist ihr Anfang in der Ressource.
[[nodiscard]] ID3D12Resource* ressource(ID3D11Resource* r, std::uint64_t* versatz = nullptr);
// DXR (gui/karte_dxr_win32.cpp): eine Sicht auf die Beschleunigungsstruktur (TLAS) an dieser Adresse - sie laesst sich mit
// PSSetShaderResources binden wie jede andere Sicht (eigener Deskriptor im gemeinsamen Stapel). Die Struktur gehoert dem
// Rufer und muss leben, solange die Sicht gebunden werden kann; Release() gibt nur den Deskriptor zurueck. nullptr: Abgeber
// nicht aktiv oder kein Platz im Stapel.
[[nodiscard]] ID3D11ShaderResourceView* strukturSicht(D3D12_GPU_VIRTUAL_ADDRESS adresse);

// Zaehler fuer Messung und Selbsttest (seit dem Start).
struct Zaehler {
    std::uint64_t zeichnen = 0;       // Draw*-Aufrufe
    std::uint64_t pso = 0;            // angelegte Pipeline-Zustaende
    std::uint64_t psoWechsel = 0;     // SetPipelineState
    std::uint64_t umbenannt = 0;      // Map(WRITE_DISCARD): frischer Block
    std::uint64_t barrieren = 0;      // Zustandswechsel von Ressourcen
    std::uint64_t fehlt = 0;          // Aufrufe nicht umgesetzter Methoden (jede einmal im Protokoll)
    std::uint64_t lebend = 0;         // lebende Objekte des Abgebers
    std::uint64_t lebendFrueher = 0;  // davon Objekte frueherer Geraete (REST612: vergessen statt freigegeben?)
    std::uint64_t hochladeBytes = 0;  // belegter Hochladespeicher der Bloecke
    std::uint64_t angelegt = 0;       // angelegte D3D12-Ressourcen (Texturen, Puffer, Seiten und grosse Bloecke)
    std::uint64_t ruhend = 0;         // dynamische Puffer, deren ruhender Inhalt in den Grafikspeicher kopiert wurde
    std::uint64_t ausVorrat = 0;      // CreateBuffer aus dem Vorrat freigegebener Puffer bedient (ohne neue Ressource)
    std::uint64_t teilAbgaben = 0;    // Teilabgaben mitten im Bild (BHED_D3D12_TEILABGABE)
    std::uint64_t psoAusSpeicher = 0; // davon aus dem Zwischenspeicher auf der Platte geholt (ID3D12PipelineLibrary)
    std::uint64_t psoMikro = 0;       // Zeit fuer alle Pipeline-Zustaende, Mikrosekunden
    std::uint64_t inGrafik = 0;       // Kopien vom Shader gelesener dynamischer Puffer in den Grafikspeicher
    std::uint64_t langeBilder = 0;    // Bilder ueber der Grenze (BHED_D3D12_LANGBILD)
    std::uint64_t listeHoechstens = 0;   // D3D12-5: die meisten Zeichenbefehle (und UpdateSubresource) einer abgeschickten Liste
    std::uint64_t listeGrenze = 0;       // D3D12-5: die Grenze je Liste (BHED_D3D12_LISTE; 0: keine)
};
[[nodiscard]] Zaehler zaehler();
// Was der Abgeber belegt, als Zeilen fuer das Protokoll (Grafikspeicher und Arbeitsspeicher nach Art, die groessten Texturen).
[[nodiscard]] std::vector<std::string> speicherZeilen();
// Bytes im Vorrat freigegebener Puffer im Grafikspeicher (hoechstens 64 MB; was 600 Bilder lang niemand nimmt, geht zurueck).
[[nodiscard]] std::uint64_t vorratBytes();

}  // namespace abgeber

}  // namespace bhed::render::d3d12
#endif
