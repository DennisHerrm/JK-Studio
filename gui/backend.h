// backend.h - Grafikschnittstelle, austauschbar
//
// Zwei Umsetzungen, wie bei efxed: Direct3D 11 und OpenGL 3.3. Der Rueckfall
// ist kein Uebereifer - auf Rechnern ohne Hardware-Direct3D (etwa in einer
// virtuellen Maschine oder mit reinem WARP) startet die D3D-Fassung entweder
// gar nicht oder zeichnet unertraeglich langsam.
#ifndef BHED_BACKEND_H
#define BHED_BACKEND_H

#include <cstdint>
#include <string>
#include <vector>

struct HWND__;
using HWNDHandle = HWND__*;

#include "imgui.h"   // rc601 saber review: ImDrawList, ImVec2 (zeichneAdditiv)

namespace bhed::render {

enum class Backend : std::uint8_t {
    Direct3D11,
    OpenGL3,
};

// Was tatsaechlich lief - fuer die Anzeige und das Fehlerprotokoll.
const char* backendName(Backend b);

// Startet die Schnittstelle am Fenster. false, wenn sie nicht verfuegbar ist;
// dann darf der Aufrufer die andere versuchen.
bool init(Backend b, HWNDHandle hwnd);
void shutdown();

void newFrame();
// Zeichnet die Daten von ImGui::GetDrawData() und tauscht die Puffer.
void present(float r, float g, float bl);

// rc608 Teil 12: Bildzeiten fuer die Leistungsmessung (Selbsttest "leistung").
// cpuMs: newFrame bis vor Present (Oberflaeche, Szene, Zeichenbefehle; ohne das
// Warten auf den Bildschirm). gpuMs: Zeitmarken der Grafikkarte ueber dieselbe
// Strecke, zwei bis drei Bilder spaeter. -1, solange kein Wert da ist. Ausser
// waehrend einer Messung kostet es nichts.
struct BildZeiten {
    double cpuMs = -1.0;
    double gpuMs = -1.0;
    int cpuBilder = 0;
    int gpuBilder = 0;
};
void bildMessung(bool an);
[[nodiscard]] BildZeiten bildZeiten();
void resize(int width, int height);

// --- Zugang zum Direct3D-Geraet ----------------------------------------
//
// Als void*, damit dieser Kopf ohne Windows-Kopfdateien auskommt - er wird
// auch auf Linux gelesen (tools/check_gui.sh uebersetzt die Oberflaeche mit
// g++). Der Aufrufer castet auf ID3D11Device* bzw. ID3D11DeviceContext*.
//
// Gibt nullptr zurueck, wenn nicht Direct3D laeuft (also unter OpenGL oder
// vor init). Wer das nicht prueft, stuerzt beim Wechsel der Schnittstelle
// ab - und der Wechsel passiert automatisch, wenn D3D nicht verfuegbar ist
// (siehe die Notiz oben zu WARP).
[[nodiscard]] void* d3dDevice();
[[nodiscard]] void* d3dContext();

// Eine eigene Textur fuer die Symbolleiste anlegen. RGBA, 8 Bit je Kanal.
//
// Bewusst NICHT ueber ImFontAtlas::AddCustomRect: das ist in ImGui 1.92 als
// "[ALPHA] Custom Rectangles/Glyphs API" gekennzeichnet, GetCustomRect
// dereferenziert TexData ungeprueft, und ein io.Fonts->Clear() beim
// Schriftneubau macht alle Kennungen ungueltig. Eine eigene Textur ueberlebt
// beides und haengt an nichts, was sich zwischen zwei Fassungen aendert.
[[nodiscard]] void* createTexture(const unsigned char* rgba, int width, int height);

// Inhalt einer bestehenden Textur ersetzen. Fuer die Kartenansicht: dort
// entsteht jedes Bild neu, und jedes Mal eine Textur anzulegen und die alte
// wegzuwerfen waere Verschwendung.
void updateTexture(void* handle, const unsigned char* rgba, int width, int height);
// Nur das Rechteck [x0, x1) x [y0, y1) aus demselben Bild (rgba, width x
// height) hochladen - fuer die Gizmo-Ueberlagerung, die meist nur einen
// kleinen Teil der Ansicht belegt (rc594).
void updateTextureBereich(void* handle, const unsigned char* rgba, int width, int height,
                          int x0, int y0, int x1, int y1);
void destroyTextures();

// rc601 saber review: ein Bild mit additiver Mischung (blendFunc GL_ONE GL_ONE)
// in eine ImGui-Zeichenliste - fuer den Klirrblitz, den das Spiel als 2D-Bild
// NACH den Kinobalken zeichnet (CG_SaberClashFlare aus CG_Draw2D). farbe ist
// die Eckfarbe (R_SetColor), RGBA wie IM_COL32.
void zeichneAdditiv(ImDrawList* dl, void* textur, ImVec2 a, ImVec2 b, unsigned int farbe);
// Menue-Editor M2: eine Stufe eines Shaderskripts als Viereck (Ecken p, st uv)
// mit ihrer blendFunc. quelle/ziel sind bhed::BlendFactor (0 One, 1 Zero,
// 2 SrcColor, 3 OneMinusSrcColor, 4 SrcAlpha, 5 OneMinusSrcAlpha, 6 DstColor,
// 7 OneMinusDstColor, 8 DstAlpha, 9 OneMinusDstAlpha, 10 SrcAlphaSaturate);
// klemmen = clampMap, sonst wiederholt (tcMod scroll).
void zeichneStufe(ImDrawList* dl, void* textur, const ImVec2 p[4], const ImVec2 uv[4], unsigned int farbe, int quelle, int ziel,
                  bool klemmen);
// Zaehlt jedes destroyTextures (Geraeteverlust, Wechsel der Schnittstelle) -
// wer eine Textur aus createTexture zwischenspeichert, legt sie neu an, wenn
// sich der Stand aendert.
[[nodiscard]] unsigned texturStand();

Backend current();
bool ready();

// --- Welche Grafikkarte (rc596) -------------------------------------------
//
// Laptops mit Intel im Prozessor und einer NVIDIA-Karte daneben (Optimus):
// ohne ausdrueckliche Bitte gibt Windows einem Programm die sparsame Intel.
// Name der Karte, auf der das Direct3D-Geraet laeuft ("" ohne Geraet), und
// Name der Karte, die Windows als leistungsstaerkste nennt ("" wenn es das
// nicht sagen kann - aelter als Windows 10 1803).
[[nodiscard]] std::string geraeteKarte();
[[nodiscard]] std::string staerksteKarte();
// Karten-Editor, "About NetRadiant" (gtkdlgs.cpp:385-404 zeigt GL_VENDOR,
// GL_VERSION, GL_RENDERER, GL_EXTENSIONS): dasselbe fuer das Direct3D-Geraet -
// Hersteller (VendorId), Version mit Feature-Level, Karte und die Merkmale.
struct GeraeteInfo {
    std::string hersteller;
    std::string version;
    std::string karte;
    std::vector<std::string> merkmale;
};
[[nodiscard]] GeraeteInfo geraeteInfo();
// --- Verlorenes Geraet (rc596) ---------------------------------------------
//
// Treiber-Update, Wechsel der Grafikkarte, ein haengender Shader (TDR): dann
// meldet Present DXGI_ERROR_DEVICE_REMOVED oder _RESET, und das Geraet ist
// unbrauchbar. Bis rc595 lief behaved danach mit einem toten Geraet weiter
// (schwarzes Bild). Jetzt merkt present() es sich, schreibt den Grund ins
// Protokoll, und die Hauptschleife baut die Grafik neu auf.
[[nodiscard]] bool geraetVerloren();
// Fuer den Selbsttest: so tun, als waere das Geraet verloren.
void geraetVerlorenSpielen();
// Vor jedem Bild: warten, bis die Swapchain einen Puffer frei hat (wartbare
// Swapchain, rc596). Ohne sie sofort zurueck.
void warteAufBild();
// Zeigt die Swapchain im Flip-Modell (ab Windows 10)?
[[nodiscard]] bool flipModell();
// Die sparsamste Karte (BHED_GPU=sparsam), "" wenn Windows es nicht sagt.
[[nodiscard]] std::string sparsamsteKarte();
// true, wenn BHED_GPU=sparsam gesetzt ist.
[[nodiscard]] bool sparsamGewuenscht();

// Selbsttest: das naechste fertig gezeichnete Bild als BMP speichern (nur
// Direct3D 11). Liest den eigenen Grafikpuffer - also genau das, was
// behaved zeichnet, und nichts vom restlichen Bildschirm.
void fotoAnfordern(const std::string& pfad);
// rc609 T1, Selbsttest: wie fotoAnfordern, dazu die eigenen Fenster (Tabs,
// Panels) an ihrer Stelle ueber dem Hauptfenster - aus ihren eigenen Puffern.
void fotoMitFensternAnfordern(const std::string& pfad);

// dock1: die Haken fuer eigene Fenster (Multi-Viewport) setzen - unter OpenGL
// die Kontextumschaltung je Fenster; nach ImGui_ImplWin32_Init aufrufen.
void plattformHakenSetzen();
// dock1, Selbsttest: das naechste fertige Bild des eigenen Fensters `id`
// (ImGuiViewport::ID) lesen - Groesse und Zahl der Farben (Stichprobe jedes
// zweiten Punkts, hoechstens 4096). Direct3D 11 und OpenGL (dock4).
struct ViewportBild {
    unsigned id = 0;
    int breite = 0;
    int hoehe = 0;
    int farben = 0;
    bool da = false;
};
void viewportBildAnfordern(unsigned id);
// dock1: wie lange das Zeichnen und Zeigen der eigenen Fenster im letzten Bild
// gedauert hat (Millisekunden, 0 ohne eigene Fenster).
[[nodiscard]] double plattformMs();
[[nodiscard]] ViewportBild viewportBild();
// rc609 T1: wie viele Bilder das eigene Fenster `id` (ImGuiViewport::ID) bisher
// gezeigt hat - Leerlauf je Fenster (Selbsttest tabfenster).
[[nodiscard]] unsigned long long viewportBilder(unsigned id);
// Warum es neu gezeichnet hat (Zaehler je Grund, fuers Protokoll des Selbsttests).
[[nodiscard]] std::string viewportGruende(unsigned id);

}  // namespace bhed::render
#endif
