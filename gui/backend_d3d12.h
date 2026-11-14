// backend_d3d12.h - Direct3D 12 fuer Fenster, Swapchain, Oberflaeche und 3D-Ansichten (nur Windows, nur fuer backend_win32.cpp)
//
// Die Oberflaeche zeichnet mit imgui_impl_dx12 auf eine D3D12-Swapchain. Die 3D-Ansichten (gpumap, kartegpu, Echtzeit-Licht,
// Model-Tab) gibt es einmal; ihre Zeichenbefehle gehen an die Schnittstelle ID3D11Device/ID3D11DeviceContext, und unter
// Direct3D 12 fuehrt sie der Abgeber (abgeber12.h) als D3D12-Befehle in derselben Befehlsliste aus - render::d3dDevice() und
// d3dContext() liefern ihn. Kein D3D11On12, kein zweites Geraet. Plan und Stufen: docs/intern/NOTIZEN_d3d12_1011.md.
#ifndef BHED_GUI_BACKEND_D3D12_H
#define BHED_GUI_BACKEND_D3D12_H

#include <cstdint>
#include <string>
#include <vector>

#include <d3d11.h>
#include <dxgi1_6.h>

#include "imgui.h"

namespace bhed::render::d3d12 {

// Ein gelesenes Bild (RGBA, Zeile fuer Zeile von oben).
struct Bild {
    std::uint32_t w = 0;
    std::uint32_t h = 0;
    std::vector<unsigned char> rgba;
};

// Geraet (Merkmalsstufe 11_0), Warteschlange, Swapchain (Flip, drei Puffer, wartbar), Abgeber, ImGui. karte: die gewuenschte
// Grafikkarte (holeKarte) oder nullptr (Windows waehlt). debug: D3D12- und D3D11-Debugschicht. false, wenn eines fehlt - dann
// hat shutdown() alles wieder abgeraeumt und der Aufrufer nimmt Direct3D 11.
bool init(HWND hwnd, IDXGIAdapter1* karte, bool debug);
void shutdown();
// Nur den ImGui-Teil abbauen (imgui_impl_dx12), VOR ImGui_ImplWin32_Shutdown: der Win32-Teil ruft beim Abbau fuer jedes
// Fenster den Abbau des Zeichenteils, auch fuer das Hauptfenster - und imgui_impl_dx12 haelt dort Daten ohne Warteschlange
// (Absturz in ImGui_WaitForPendingOperations). Reihenfolge wie in den ImGui-Beispielen. shutdown() ruft es selbst.
void imguiAbbauen();

ID3D11Device* geraet11();
ID3D11DeviceContext* kontext11();
HANDLE bildFrei();   // wartbare Swapchain (wie Direct3D 11, rc596)
// D3D12-4 (Messung): die Grafikkarte des Geraets und die Zahl der Abgaben (ExecuteCommandLists) seit dem Start.
bool karteLuid(LUID& luid);
std::uint64_t abgabenBisher();
double hauptfadenWartenMs();   // D3D12-4: Hauptfaden wartete auf Zaunwerte und den Abschickfaden, seit dem Start

void newFrame();
// Vor dem Zeichnen aller Fenster: die Sichten der 3D-Ansichten in den Zeichenlisten aller Fenster durch ihre Deskriptoren
// ersetzen und ihre Texturen lesbar stellen. bemalt: Ressourcen, die in diesem Bild bemalt wurden (backend_win32.cpp) - nur
// fuer frisch().
void vorbereiten(const std::vector<const void*>& bemalt);
// Wurde der Inhalt dieser Textur (Kennung, wie ImGui sie sieht) in diesem Bild neu? Fuer den Leerlauf je Fenster.
[[nodiscard]] bool frisch(ImTextureID id);
// Das Hauptfenster zeichnen (loeschen, Zeichenliste) und abschicken. vorAbschicken: wird gerufen, wenn das Hauptfenster
// gezeichnet und die Liste noch offen ist (die Endmarke der Bildmessung gehoert in die Befehlsliste des Bildes).
void hauptfensterZeichnen(const float farbe[4], void (*vorAbschicken)() = nullptr, void (*vorOberflaeche)() = nullptr);
// Warten, bis der Abschickfaden alles Abgegebene ausgefuehrt hat (ExecuteCommandLists, Present): danach gehoeren Warteschlange
// und Swapchain dem Rufer - noetig, bevor imgui_impl_dx12 eigene Fenster ueber dieselbe Warteschlange zeichnet.
void abschickenAbwarten();
// Den eben gezeichneten Hintergrundpuffer lesen (wartet auf die Grafikkarte; nur Selbsttest).
bool hauptfensterLesen(Bild& aus);
// Ein eigenes Fenster (Multi-Viewport) noch einmal in ein eigenes Ziel zeichnen und lesen - dieselbe Zeichenliste, derselbe
// Weg wie imgui_impl_dx12 (dessen Ziele sind privat). Nur Selbsttest.
bool fensterLesen(ImGuiViewport* v, Bild& aus);
// Eine ImGui-Zeichenliste in eine Textur der 3D-Ansichten (Renderziel, R8G8B8A8_UNORM) zeichnen - der Weg von
// imgui_impl_dx12, wo Direct3D 11 ImGui_ImplDX11_RenderDrawData auf das gesetzte Ziel ruft (gpu::zeichneUeberlagerung).
// Wartet auf die Grafikkarte (nur fuer Fotos).
bool inZielZeichnen(ID3D11Resource* ziel, ImDrawData* dd);
// Present(1, 0); HRESULT.
long zeigen();
void trimmen();
[[nodiscard]] long verlustGrund();
void resize(int w, int h);

void* texturAnlegen(const unsigned char* rgba, int w, int h);
void texturErsetzen(void* kennung, const unsigned char* rgba, int w, int h, int x0, int y0, int x1, int y1);
void texturenFrei();

// Mischart fuer die folgenden Zeichenbefehle einer ImGui-Liste (bis ImDrawCallback_ResetRenderState): 0 additiv (GL_ONE GL_ONE),
// 1 eine Shaderstufe (quelle/ziel wie zeichneStufe, klemmen), 2 ohne Mischen.
void mischart(ImDrawList* dl, int art, int quelle, int ziel, bool klemmen);

[[nodiscard]] std::string karte();
// Programmstart: das D3D12-Geraet schon vor init() in einem eigenen Faden anlegen - es entsteht, waehrend das Programm noch
// anderes laedt (das Geraet ist je Karte ein Einzelstueck: init() bekommt dasselbe sofort). karteHolen liefert im Faden die
// gewuenschte Karte (oder nullptr: Windows waehlt); debug wie bei init(). init() und shutdown() warten auf den Faden.
void vorwaermen(IDXGIAdapter1* (*karteHolen)(), bool debug);
// Selbsttest: hat der Abgeber jede gerufene Methode umgesetzt und die Debugschicht keinen Fehler gemeldet? text: die Zaehler.
[[nodiscard]] bool befund(std::string& text);
// Selbsttest leistung: was der Abgeber an Grafikspeicher und Arbeitsspeicher belegt, als Zeilen.
[[nodiscard]] std::vector<std::string> speicherZeilen();
// Selbsttest allekarten: Bytes im Vorrat freigegebener Puffer des Abgebers (abgeber::vorratBytes).
[[nodiscard]] std::uint64_t vorratBytes();
void info(std::string& hersteller, std::string& version, std::string& kartenName, std::vector<std::string>& merkmale);

}  // namespace bhed::render::d3d12
#endif
