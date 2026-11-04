// backend_win32.cpp - Direct3D 11 und OpenGL 3.3
//
// Beide in einer Datei, weil sie sich denselben Zustand und dieselbe
// Umschaltlogik teilen. Getrennt waeren es zwei Dateien mit je zehn Zeilen
// Eigenleben und einer gemeinsamen Kopfdatei fuer den Rest.

#include <chrono>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <cstdlib>

#include "bhed/diag.h"

#include "backend.h"
#include <string>
#include <cstdint>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"

#include <windows.h>
#include <dwmapi.h>

#include <GL/gl.h>

// GL_CLAMP_TO_EDGE gibt es erst seit OpenGL 1.2; das mitgelieferte gl.h von
// Windows steht bei 1.1 stehen. Der Wert ist fest und aendert sich nicht.
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// Direct3D 11 ist wahlweise.
//
// Wird es fest eingebunden, stehen d3d11.dll UND d3dcompiler_47.dll als
// harte Importe in der .exe - und Windows laedt das Programm dann gar nicht
// erst, wenn eine davon fehlt. Der Rueckfall auf OpenGL kaeme nie zum Zug,
// also genau in dem Fall nicht, fuer den er gedacht ist.
//
// Mit MSVC loest /DELAYLOAD das (siehe CMakeLists). Ohne verzoegertes Laden
// wird nur OpenGL gebaut. ImGui warnt uebrigens selbst davor, siehe den
// Kommentar in imgui_impl_dx11.cpp ueber D3DCOMPILER_DLL_A.
#ifdef BHED_WITH_D3D11
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <dxgi1_6.h>
#include "bemalt.h"   // rc609 T1: Leerlauf je Fenster
#endif
#include <cstring>

#include <cstdio>
#include <initializer_list>
#include <vector>

namespace bhed::render {
#ifdef BHED_WITH_D3D11
IDXGIAdapter1* holeKarte(bool leistung);   // weiter unten
#endif
namespace {

Backend g_backend = Backend::Direct3D11;
bool g_ready = false;
HWND g_hwnd = nullptr;

// --- Direct3D 11 -------------------------------------------------------
#ifdef BHED_WITH_D3D11
ID3D11Device* g_device = nullptr;
ID3D11BlendState* g_additivMischung = nullptr;   // rc601 saber review: zeichneAdditiv
std::map<int, ID3D11BlendState*> g_stufenMischung;   // Menue-Editor M2: zeichneStufe, je blendFunc
ID3D11SamplerState* g_stufeWieder = nullptr;         // wiederholt (map)
ID3D11SamplerState* g_stufeKlemm = nullptr;          // geklemmt (clampMap)
ID3D11DeviceContext* g_context = nullptr;
IDXGISwapChain* g_swap = nullptr;
UINT g_swapFlags = 0;   // dieselben Flags fuer ResizeBuffers wie beim Anlegen
bool g_swapFlip = false;
HANDLE g_bildFrei = nullptr;   // wartbare Swapchain (rc596)
bool g_geraetVerloren = false;
bool g_verlustSpielen = false;
ID3D11RenderTargetView* g_rtv = nullptr;

// rc608 Teil 12: Zeitmarken um das ganze Bild (newFrame bis vor Present), nur
// waehrend einer Leistungsmessung. Ring ueber vier Bilder, abgeholt wird nur,
// was fertig ist (GetData mit DO_NOT_FLUSH) - wie die Abschnitte in gpumap.
struct BildMarken {
    ID3D11Query* takt = nullptr;
    ID3D11Query* anfang = nullptr;
    ID3D11Query* ende = nullptr;
    bool offen = false;
};
BildMarken g_bildMarken[4];
unsigned g_bildMarkeJetzt = 0;
bool g_bildMarkeLaeuft = false;

void bildMarkenFrei() {
    for (BildMarken& m : g_bildMarken) {
        if (m.takt != nullptr) { m.takt->Release(); m.takt = nullptr; }
        if (m.anfang != nullptr) { m.anfang->Release(); m.anfang = nullptr; }
        if (m.ende != nullptr) { m.ende->Release(); m.ende = nullptr; }
        m.offen = false;
    }
    g_bildMarkeLaeuft = false;
}
#endif

// rc608 Teil 12: Bildzeiten der Leistungsmessung (render::bildMessung)
bool g_bildMessung = false;
std::chrono::steady_clock::time_point g_bildAnfang{};
BildZeiten g_bildZeiten{};
#ifdef BHED_WITH_D3D11

void createRtv() {
    if (g_swap == nullptr) {
        return;
    }
    ID3D11Texture2D* back = nullptr;
    if (SUCCEEDED(g_swap->GetBuffer(0, IID_PPV_ARGS(&back))) && back != nullptr) {
        g_device->CreateRenderTargetView(back, nullptr, &g_rtv);
        back->Release();
    }
}

void releaseRtv() {
    if (g_rtv != nullptr) {
        g_rtv->Release();
        g_rtv = nullptr;
    }
}

bool initD3D11(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    // --- Flip-Modell (rc596) ------------------------------------------------
    //
    // Bis rc595 DXGI_SWAP_EFFECT_DISCARD, das alte "Blt"-Modell: jedes Bild
    // wird in eine Flaeche des Desktop-Compositors kopiert. Microsoft
    // ("For best performance, use DXGI flip model"): auf Laptops mit zwei
    // Grafikchips kostet das seit Windows 10 1803 zusaetzliche Arbeit - also
    // genau dann, wenn behaved jetzt auf der NVIDIA zeichnet. FLIP_DISCARD
    // mit drei Puffern; vor Windows 10 gibt es das nicht, dann wie bisher.
    sd.BufferCount = 3;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    // Wartbar (rc596): vor jedem Bild auf einen freien Puffer warten statt
    // in Present - hoechstens zwei Bilder in der Warteschlange, damit die
    // Eingabe nicht drei Bilder hinter dem Bildschirm herlaeuft (NVIDIA,
    // "Advanced API Performance: Swap Chains"; Microsoft, "Reduce latency
    // with DXGI 1.3 swap chains").
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    const D3D_FEATURE_LEVEL wanted[] = {D3D_FEATURE_LEVEL_11_0,
                                        D3D_FEATURE_LEVEL_10_1,
                                        D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL got{};

    // --- Welche Karte (rc596) ----------------------------------------------
    //
    // Zuerst die, die Windows als leistungsstaerkste nennt. Auf einem Laptop
    // mit Intel und NVIDIA (Optimus) bekam behaved sonst die Intel - bei
    // shank lief alles auf "Intel(R) UHD Graphics", die RTX 4070 daneben
    // stand still. Auf einem Rechner mit nur einer Karte ist das dieselbe
    // wie vorher. Geht es dort nicht (alter Treiber, Karte abgeschaltet,
    // Windows ohne IDXGIFactory6), wie bisher: Windows waehlt, dann WARP.
    // BHED_GPU=sparsam nimmt ausdruecklich die sparsamste (Akku) - nur
    // "Windows waehlen lassen" reicht nicht, denn NvOptimusEnablement (siehe
    // main_win32.cpp) lenkt auch die Standardwahl auf die NVIDIA.
    IDXGIAdapter1* stark = holeKarte(!sparsamGewuenscht());
    struct Versuch {
        IDXGIAdapter* karte;
        D3D_DRIVER_TYPE typ;
    };
    std::vector<Versuch> versuche;
    if (stark != nullptr) {
        versuche.push_back({stark, D3D_DRIVER_TYPE_UNKNOWN});
    }
    // Dann die Grafikkarte, die Windows gibt. Nur wenn die nichts liefert,
    // WARP - das rechnet auf der Hauptrechnereinheit und ist langsam, aber
    // es laeuft ueberall.
    versuche.push_back({nullptr, D3D_DRIVER_TYPE_HARDWARE});
    versuche.push_back({nullptr, D3D_DRIVER_TYPE_WARP});
    for (int modell = 0; modell < 2; ++modell) {
        if (modell == 1) {
            // Kein Flip-Modell (vor Windows 10): das alte, wie bis rc595.
            diag::detail("Flip-Modell nicht verfuegbar - altes Blt-Modell");
            sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            sd.BufferCount = 2;
            sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;   // wartbar gibt es nur mit Flip
        }
        for (const Versuch& versuch : versuche) {
            const D3D_DRIVER_TYPE driver = versuch.typ;
            // --- Die Direct3D-Debugschicht ---------------------------------
            //
            // Sie prueft JEDEN Aufruf und meldet im Klartext, was nicht stimmt:
            // "Konstantenpuffer zu klein", "Eingabelayout passt nicht zum
            // Shader", "Textur gleichzeitig gebunden und beschrieben".
            //
            // Der GPU-Weg ist blind geschrieben worden - ohne Windows laesst er
            // sich hier nicht ausfuehren -, und ich habe dieses Werkzeug
            // dreissig Runden lang nicht eingeschaltet. Mehrere Umwege dieser
            // Sitzung haette es beim ersten Bild beendet.
            //
            // Nur mit BHED_D3D_DEBUG=1 in der Umgebung: die Schicht kostet
            // Leistung und ist auf Rechnern ohne "Grafiktools" gar nicht da.
            // Schlaegt das Anlegen damit fehl, wird es ohne sie wiederholt.
            // Immer AN versuchen, nicht nur auf Anforderung.
            //
            // shanks Frage: "warum muss ich ueberhaupt einen extra Knopf
            // aktivieren fuer das Debuggen? kann es das nicht immer automatisch
            // machen?" - Sie ist berechtigt. Ein Werkzeug, das man erst
            // einschalten muss, ist genau dann aus, wenn man es braucht.
            //
            // Die Schicht kostet Leistung, aber der Rasterer laeuft ohnehin bei
            // 240 fps und der GPU-Weg bei ueber 800. Gemessen wird mit
            // BHED_D3D_DEBUG=0, wenn es je darauf ankommt.
            //
            // Auf Rechnern ohne "Grafiktools" gibt es sie nicht - dann wird
            // stillschweigend ohne sie wiederholt.
            //
            // rc594: nur noch im Selbsttest (BHED_EDITORTEST) oder mit
            // BHED_D3D_DEBUG=1. Gemessen an den Kamerafahrten: mit der Schicht
            // 33 statt 18 ms je Bild in md_am_sith, 19 statt 11 in kor1 - die
            // "800 fps" oben stammen aus einer Zeit mit weit weniger Aufrufen.
            // shank: "Nur in Tests an". Die Selbsttests finden D3D-Fehler damit
            // weiter von selbst; BHED_D3D_DEBUG=0 schaltet sie auch dort ab.
            const char* dbg = std::getenv("BHED_D3D_DEBUG");
            const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
            UINT flags = 0;
            if (dbg != nullptr ? dbg[0] == '1' : imTest) {
                flags = D3D11_CREATE_DEVICE_DEBUG;
            }
            HRESULT hr = D3D11CreateDeviceAndSwapChain(
                versuch.karte, driver, nullptr, flags, wanted,
                static_cast<UINT>(sizeof(wanted) / sizeof(wanted[0])),
                D3D11_SDK_VERSION, &sd, &g_swap, &g_device, &got, &g_context);
            if (FAILED(hr) && flags != 0) {
                // Ohne installierte Grafiktools gibt es die Schicht nicht.
                diag::detail("D3D-Debugschicht nicht verfuegbar - ohne sie");
                flags = 0;
                hr = D3D11CreateDeviceAndSwapChain(
                    versuch.karte, driver, nullptr, 0, wanted,
                    static_cast<UINT>(sizeof(wanted) / sizeof(wanted[0])),
                    D3D11_SDK_VERSION, &sd, &g_swap, &g_device, &got,
                    &g_context);
            } else if (SUCCEEDED(hr) && flags != 0) {
                diag::detail("D3D-Debugschicht AN - Meldungen im Debugfenster");
            }
            if (SUCCEEDED(hr)) {
                createRtv();
                if (stark != nullptr) { stark->Release(); }
                g_swapFlags = sd.Flags;
                g_swapFlip = (sd.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD);
                if ((sd.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT) != 0U) {
                    IDXGISwapChain2* sc2 = nullptr;
                    if (SUCCEEDED(g_swap->QueryInterface(__uuidof(IDXGISwapChain2), reinterpret_cast<void**>(&sc2))) &&
                        sc2 != nullptr) {
                        sc2->SetMaximumFrameLatency(2);
                        g_bildFrei = sc2->GetFrameLatencyWaitableObject();
                        sc2->Release();
                    }
                }
                diag::detail("Grafikkarte: " + geraeteKarte() + (g_swapFlip ? " (Flip-Modell)" : " (Blt-Modell)"));
                return ImGui_ImplDX11_Init(g_device, g_context);
            }
            if (versuch.karte != nullptr) {
                diag::detail("Grafikkarte: auf der gewuenschten Karte kein Geraet - Windows waehlt");
            }
        }
    }
    if (stark != nullptr) { stark->Release(); }
    return false;
}

void shutdownD3D11() {
    if (g_bildFrei != nullptr) { CloseHandle(g_bildFrei); g_bildFrei = nullptr; }
    bildMarkenFrei();
    g_geraetVerloren = false;
    // rc601 saber review: die additive Mischung gehoert zum alten Geraet.
    if (g_additivMischung != nullptr) { g_additivMischung->Release(); g_additivMischung = nullptr; }
    for (auto& [k, b] : g_stufenMischung) {
        (void)k;
        if (b != nullptr) { b->Release(); }
    }
    g_stufenMischung.clear();
    if (g_stufeWieder != nullptr) { g_stufeWieder->Release(); g_stufeWieder = nullptr; }
    if (g_stufeKlemm != nullptr) { g_stufeKlemm->Release(); g_stufeKlemm = nullptr; }
    ImGui_ImplDX11_Shutdown();
    releaseRtv();
    // D3D11 zerstoert verzoegert: ohne ClearState und Flush haengt die alte
    // Swapchain noch am Fenster, und eine neue im Flip-Modell wird abgelehnt
    // (Microsoft, "DXGI flip model": deferred destruction). Gesehen beim
    // Neuaufbau nach einem Geraeteverlust - er fiel auf das Blt-Modell zurueck.
    if (g_context != nullptr) {
        g_context->ClearState();
        g_context->Flush();
    }
    if (g_swap != nullptr) { g_swap->Release(); g_swap = nullptr; }
    if (g_context != nullptr) { g_context->Release(); g_context = nullptr; }
    if (g_device != nullptr) { g_device->Release(); g_device = nullptr; }
}

#else
bool initD3D11(HWND) { return false; }
void shutdownD3D11() {}
#endif

// --- OpenGL 3.3 --------------------------------------------------------
HDC g_dc = nullptr;
HGLRC g_rc = nullptr;

// wglCreateContextAttribsARB gibt es nur ueber einen bereits bestehenden
// Kontext. Also: einfachen Kontext bauen, Funktion holen, richtigen Kontext
// bauen, einfachen wegwerfen.
using CreateCtxAttribs = HGLRC(WINAPI*)(HDC, HGLRC, const int*);

PIXELFORMATDESCRIPTOR glPixelformat() {
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    return pfd;
}

bool initGL3(HWND hwnd) {
    g_dc = GetDC(hwnd);
    if (g_dc == nullptr) {
        return false;
    }
    PIXELFORMATDESCRIPTOR pfd = glPixelformat();
    const int pf = ChoosePixelFormat(g_dc, &pfd);
    if (pf == 0 || SetPixelFormat(g_dc, pf, &pfd) == FALSE) {
        return false;
    }

    const HGLRC temp = wglCreateContext(g_dc);
    if (temp == nullptr || wglMakeCurrent(g_dc, temp) == FALSE) {
        return false;
    }

    auto createAttribs = reinterpret_cast<CreateCtxAttribs>(
        reinterpret_cast<void*>(wglGetProcAddress("wglCreateContextAttribsARB")));
    if (createAttribs != nullptr) {
        const int attribs[] = {
            0x2091 /*MAJOR_VERSION*/, 3,
            0x2092 /*MINOR_VERSION*/, 3,
            0x9126 /*PROFILE_MASK*/,  0x00000001 /*CORE*/,
            0};
        if (HGLRC core = createAttribs(g_dc, nullptr, attribs)) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(temp);
            g_rc = core;
            wglMakeCurrent(g_dc, g_rc);
        }
    }
    if (g_rc == nullptr) {
        // Kein 3.3 zu bekommen: mit dem einfachen Kontext weitermachen.
        // ImGui kommt mit GLSL 130 auch auf OpenGL 3.0 zurecht.
        g_rc = temp;
    }
    return ImGui_ImplOpenGL3_Init(g_rc != temp ? "#version 330 core" : "#version 130");
}

void shutdownGL3() {
    ImGui_ImplOpenGL3_Shutdown();
    if (g_rc != nullptr) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(g_rc);
        g_rc = nullptr;
    }
    if (g_dc != nullptr && g_hwnd != nullptr) {
        ReleaseDC(g_hwnd, g_dc);
        g_dc = nullptr;
    }
}

// --- dock1: eigene Fenster (Multi-Viewport) unter OpenGL ----------------------
//
// Wie imgui/examples/example_win32_opengl3: jedes Plattformfenster bekommt das
// Pixelformat des Hauptfensters und zeichnet mit dem einen Kontext g_rc - vor
// dem Zeichnen auf sein Fenster umgestellt, danach zurueck aufs Hauptfenster.
// Direct3D 11 braucht nichts davon: imgui_impl_dx11 legt je Fenster eine
// eigene Swapchain an.
struct GlFenster {
    HDC dc = nullptr;
};

void glFensterAnlegen(ImGuiViewport* v) {
    auto* f = new GlFenster;
    const HWND h = static_cast<HWND>(v->PlatformHandle);
    f->dc = GetDC(h);
    PIXELFORMATDESCRIPTOR pfd = glPixelformat();
    const int pf = ChoosePixelFormat(f->dc, &pfd);
    if (pf == 0 || SetPixelFormat(f->dc, pf, &pfd) == FALSE) { diag::detail("OpenGL: Pixelformat fuer ein eigenes Fenster abgelehnt"); }
    v->RendererUserData = f;
}

void glFensterWeg(ImGuiViewport* v) {
    auto* f = static_cast<GlFenster*>(v->RendererUserData);
    if (f == nullptr) { return; }
    wglMakeCurrent(g_dc, g_rc);
    ReleaseDC(static_cast<HWND>(v->PlatformHandle), f->dc);
    delete f;
    v->RendererUserData = nullptr;
}

void glFensterZeichnen(ImGuiViewport* v, void*) {
    if (const auto* f = static_cast<GlFenster*>(v->RendererUserData)) { wglMakeCurrent(f->dc, g_rc); }
}

void glFensterTauschen(ImGuiViewport* v, void*) {
    if (const auto* f = static_cast<GlFenster*>(v->RendererUserData)) { SwapBuffers(f->dc); }
}

}  // namespace

namespace {

#ifdef BHED_WITH_D3D11
std::vector<ID3D11ShaderResourceView*> g_d3dTextures;
#endif
std::vector<unsigned int> g_glTextures;

}  // namespace

void* d3dDevice() {
#ifdef BHED_WITH_D3D11
    return (g_backend == Backend::Direct3D11) ? g_device : nullptr;
#else
    return nullptr;
#endif
}

void* d3dContext() {
#ifdef BHED_WITH_D3D11
    return (g_backend == Backend::Direct3D11) ? g_context : nullptr;
#else
    return nullptr;
#endif
}

// --- rc601 saber review: additiv zeichnen (Klirrblitz) ------------------------
namespace {
#ifdef BHED_WITH_D3D11
void additivD3d(const ImDrawList* /*liste*/, const ImDrawCmd* /*befehl*/) {
    if (g_device == nullptr || g_context == nullptr) {
        return;
    }
    if (g_additivMischung == nullptr) {
        D3D11_BLEND_DESC d{};
        d.RenderTarget[0].BlendEnable = TRUE;
        d.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;     // GL_ONE
        d.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;    // GL_ONE
        d.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        d.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        d.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        d.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        d.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(g_device->CreateBlendState(&d, &g_additivMischung))) {
            g_additivMischung = nullptr;
            return;
        }
    }
    const float faktor[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    g_context->OMSetBlendState(g_additivMischung, faktor, 0xffffffffU);
}
#endif
void additivGl(const ImDrawList* /*liste*/, const ImDrawCmd* /*befehl*/) { glBlendFunc(GL_ONE, GL_ONE); }

// --- Menue-Editor M2: zeichneStufe -----------------------------------------------
#ifdef BHED_WITH_D3D11
D3D11_BLEND d3dFaktor(int f) {
    static const D3D11_BLEND t[] = {D3D11_BLEND_ONE,       D3D11_BLEND_ZERO,           D3D11_BLEND_SRC_COLOR,  D3D11_BLEND_INV_SRC_COLOR,
                                    D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA,  D3D11_BLEND_DEST_COLOR, D3D11_BLEND_INV_DEST_COLOR,
                                    D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_INV_DEST_ALPHA, D3D11_BLEND_SRC_ALPHA};
    return (f >= 0 && f < 11) ? t[f] : D3D11_BLEND_ONE;
}
void stufeD3d(const ImDrawList* /*liste*/, const ImDrawCmd* befehl) {
    if (g_device == nullptr || g_context == nullptr) { return; }
    const int code = static_cast<int>(reinterpret_cast<std::intptr_t>(befehl->UserCallbackData));
    const int misch = code & 0xffff;
    ID3D11BlendState*& b = g_stufenMischung[misch];
    if (b == nullptr) {
        D3D11_BLEND_DESC d{};
        d.RenderTarget[0].BlendEnable = TRUE;
        d.RenderTarget[0].SrcBlend = d3dFaktor(misch & 0xff);
        d.RenderTarget[0].DestBlend = d3dFaktor((misch >> 8) & 0xff);
        d.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        d.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        d.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        d.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        d.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(g_device->CreateBlendState(&d, &b))) { b = nullptr; }
    }
    auto sampler = [](ID3D11SamplerState*& s, D3D11_TEXTURE_ADDRESS_MODE m) {
        if (s != nullptr) { return; }
        D3D11_SAMPLER_DESC d{};
        d.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        d.AddressU = d.AddressV = d.AddressW = m;
        d.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        d.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(g_device->CreateSamplerState(&d, &s))) { s = nullptr; }
    };
    const bool klemmen = (code & 0x10000) != 0;
    sampler(g_stufeWieder, D3D11_TEXTURE_ADDRESS_WRAP);
    sampler(g_stufeKlemm, D3D11_TEXTURE_ADDRESS_CLAMP);
    ID3D11SamplerState* s = klemmen ? g_stufeKlemm : g_stufeWieder;
    if (s != nullptr) { g_context->PSSetSamplers(0, 1, &s); }
    const float faktor[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    if (b != nullptr) { g_context->OMSetBlendState(b, faktor, 0xffffffffU); }
}
#endif
void stufeGl(const ImDrawList* /*liste*/, const ImDrawCmd* befehl) {
    static const GLenum t[] = {GL_ONE,       GL_ZERO,           GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                               GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA};
    const int code = static_cast<int>(reinterpret_cast<std::intptr_t>(befehl->UserCallbackData));
    const int q = code & 0xff, z = (code >> 8) & 0xff;
    glBlendFunc(q < 11 ? t[q] : GL_ONE, z < 11 ? t[z] : GL_ZERO);
}
}  // namespace

unsigned g_texturStand = 0;
unsigned texturStand() { return g_texturStand; }

void zeichneAdditiv(ImDrawList* dl, void* textur, ImVec2 a, ImVec2 b, unsigned int farbe) {
    if (dl == nullptr || textur == nullptr || !g_ready) {
        return;
    }
#ifdef BHED_WITH_D3D11
    if (g_backend == Backend::Direct3D11) {
        dl->AddCallback(additivD3d, nullptr);
    } else {
        dl->AddCallback(additivGl, nullptr);
    }
#else
    dl->AddCallback(additivGl, nullptr);
#endif
    dl->AddImage(reinterpret_cast<ImTextureID>(textur), a, b, ImVec2{0.0F, 0.0F}, ImVec2{1.0F, 1.0F}, farbe);
    dl->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
}

void zeichneStufe(ImDrawList* dl, void* textur, const ImVec2 p[4], const ImVec2 uv[4], unsigned int farbe, int quelle, int ziel,
                  bool klemmen) {
    if (dl == nullptr || textur == nullptr || !g_ready) { return; }
    void* code = reinterpret_cast<void*>(static_cast<std::intptr_t>((quelle & 0xff) | ((ziel & 0xff) << 8) | (klemmen ? 0x10000 : 0)));
#ifdef BHED_WITH_D3D11
    dl->AddCallback(g_backend == Backend::Direct3D11 ? stufeD3d : stufeGl, code);
#else
    dl->AddCallback(stufeGl, code);
#endif
    dl->AddImageQuad(reinterpret_cast<ImTextureID>(textur), p[0], p[1], p[2], p[3], uv[0], uv[1], uv[2], uv[3], farbe);
    dl->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
}

void* createTexture(const unsigned char* rgba, int width, int height) {
    if (!g_ready) {
        return nullptr;
    }
#ifdef BHED_WITH_D3D11
    if (g_backend == Backend::Direct3D11) {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(width);
        desc.Height = static_cast<UINT>(height);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA data{};
        data.pSysMem = rgba;
        data.SysMemPitch = static_cast<UINT>(width) * 4U;

        ID3D11Texture2D* texture = nullptr;
        if (FAILED(g_device->CreateTexture2D(&desc, &data, &texture)) || texture == nullptr) {
            return nullptr;
        }
        D3D11_SHADER_RESOURCE_VIEW_DESC view{};
        view.Format = desc.Format;
        view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        view.Texture2D.MipLevels = 1;
        ID3D11ShaderResourceView* srv = nullptr;
        const HRESULT hr = g_device->CreateShaderResourceView(texture, &view, &srv);
        bemaltMerken(texture);   // rc609 T1: eine neue Textur kann die Adresse einer alten haben
        texture->Release();
        if (FAILED(hr) || srv == nullptr) {
            return nullptr;
        }
        g_d3dTextures.push_back(srv);
        return srv;
    }
#endif
    GLuint id = 0;
    glGenTextures(1, &id);
    if (id == 0) {
        return nullptr;
    }
    GLint previous = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, rgba);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous));
    g_glTextures.push_back(id);
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(id));
}

void updateTexture(void* handle, const unsigned char* rgba, int width, int height) {
    if (handle == nullptr || !g_ready) {
        return;
    }
#ifdef BHED_WITH_D3D11
    if (g_backend == Backend::Direct3D11) {
        // Ueber die Sicht an die Textur kommen und den ganzen Inhalt
        // ersetzen. D3D11_USAGE_DEFAULT erlaubt UpdateSubresource.
        auto* srv = static_cast<ID3D11ShaderResourceView*>(handle);
        ID3D11Resource* res = nullptr;
        srv->GetResource(&res);
        if (res != nullptr) {
            g_context->UpdateSubresource(res, 0, nullptr, rgba,
                                         static_cast<UINT>(width) * 4U, 0);
            bemaltMerken(res);   // rc609 T1
            res->Release();
        }
        return;
    }
#endif
    const auto id = static_cast<GLuint>(reinterpret_cast<std::uintptr_t>(handle));
    GLint previous = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA,
                    GL_UNSIGNED_BYTE, rgba);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous));
}

void updateTextureBereich(void* handle, const unsigned char* rgba, int width, int height,
                          int x0, int y0, int x1, int y1) {
    x0 = std::clamp(x0, 0, width);
    x1 = std::clamp(x1, 0, width);
    y0 = std::clamp(y0, 0, height);
    y1 = std::clamp(y1, 0, height);
    if (handle == nullptr || !g_ready || x1 <= x0 || y1 <= y0) {
        return;
    }
#ifdef BHED_WITH_D3D11
    if (g_backend == Backend::Direct3D11) {
        auto* srv = static_cast<ID3D11ShaderResourceView*>(handle);
        ID3D11Resource* res = nullptr;
        srv->GetResource(&res);
        if (res != nullptr) {
            D3D11_BOX box{};
            box.left = static_cast<UINT>(x0);
            box.right = static_cast<UINT>(x1);
            box.top = static_cast<UINT>(y0);
            box.bottom = static_cast<UINT>(y1);
            box.front = 0;
            box.back = 1;
            const unsigned char* anfang =
                rgba + (static_cast<std::size_t>(y0) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x0)) * 4U;
            g_context->UpdateSubresource(res, 0, &box, anfang, static_cast<UINT>(width) * 4U, 0);
            bemaltMerken(res);   // rc609 T1
            res->Release();
        }
        return;
    }
#endif
    updateTexture(handle, rgba, width, height);
}

#ifdef BHED_WITH_D3D11
namespace {
std::string kartenName(IDXGIAdapter* ad) {
    std::string aus;
    DXGI_ADAPTER_DESC beschr{};
    if (ad != nullptr && SUCCEEDED(ad->GetDesc(&beschr))) {
        for (const WCHAR* p = beschr.Description; *p != 0; ++p) {
            aus += (*p < 128) ? static_cast<char>(*p) : '?';
        }
    }
    return aus;
}
}  // namespace

// Die leistungsstaerkste Karte laut Windows (IDXGIFactory6::
// EnumAdapterByGpuPreference), ohne Software-Rasterer. dxgi.dll wird erst
// hier geladen - fest gelinkt stuende sie als harter Import in der .exe
// (siehe CMakeLists, DELAYLOAD). nullptr, wenn Windows es nicht sagen kann.
IDXGIAdapter1* holeKarte(bool leistung) {
    HMODULE dxgi = LoadLibraryW(L"dxgi.dll");
    if (dxgi == nullptr) {
        return nullptr;
    }
    using Fabrik = HRESULT(WINAPI*)(REFIID, void**);
    const auto anlegen = reinterpret_cast<Fabrik>(reinterpret_cast<void*>(GetProcAddress(dxgi, "CreateDXGIFactory1")));
    if (anlegen == nullptr) {
        return nullptr;
    }
    IDXGIFactory6* fabrik = nullptr;
    if (FAILED(anlegen(__uuidof(IDXGIFactory6), reinterpret_cast<void**>(&fabrik))) || fabrik == nullptr) {
        return nullptr;
    }
    IDXGIAdapter1* gefunden = nullptr;
    for (UINT i = 0;; ++i) {
        IDXGIAdapter1* ad = nullptr;
        const DXGI_GPU_PREFERENCE vorzug =
            leistung ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_MINIMUM_POWER;
        if (FAILED(fabrik->EnumAdapterByGpuPreference(i, vorzug, __uuidof(IDXGIAdapter1), reinterpret_cast<void**>(&ad))) ||
            ad == nullptr) {
            break;
        }
        DXGI_ADAPTER_DESC1 d1{};
        if (SUCCEEDED(ad->GetDesc1(&d1)) && (d1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) {
            gefunden = ad;
            break;
        }
        ad->Release();
    }
    fabrik->Release();
    return gefunden;
}

std::string geraeteKarte() {
    if (g_device == nullptr) {
        return "";
    }
    std::string aus;
    IDXGIDevice* dxdev = nullptr;
    if (SUCCEEDED(g_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxdev))) &&
        dxdev != nullptr) {
        IDXGIAdapter* ad = nullptr;
        if (SUCCEEDED(dxdev->GetAdapter(&ad)) && ad != nullptr) {
            aus = kartenName(ad);
            ad->Release();
        }
        dxdev->Release();
    }
    return aus;
}

namespace {
std::string karteNach(bool leistung) {
    IDXGIAdapter1* ad = holeKarte(leistung);
    if (ad == nullptr) {
        return "";
    }
    std::string aus = kartenName(ad);
    ad->Release();
    return aus;
}
}  // namespace

std::string staerksteKarte() { return karteNach(true); }
std::string sparsamsteKarte() { return karteNach(false); }

GeraeteInfo geraeteInfo() {
    GeraeteInfo i;
    if (g_device == nullptr) { return i; }
    IDXGIDevice* dxdev = nullptr;
    if (SUCCEEDED(g_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxdev))) && dxdev != nullptr) {
        IDXGIAdapter* ad = nullptr;
        if (SUCCEEDED(dxdev->GetAdapter(&ad)) && ad != nullptr) {
            DXGI_ADAPTER_DESC d{};
            if (SUCCEEDED(ad->GetDesc(&d))) {
                switch (d.VendorId) {
                case 0x8086: i.hersteller = "Intel"; break;
                case 0x10DE: i.hersteller = "NVIDIA Corporation"; break;
                case 0x1002: i.hersteller = "ATI Technologies Inc."; break;
                case 0x1414: i.hersteller = "Microsoft"; break;
                default: {
                    char z[32];
                    std::snprintf(z, sizeof(z), "0x%04X", d.VendorId);
                    i.hersteller = z;
                }
                }
                char z[64];
                std::snprintf(z, sizeof(z), "%u MB", static_cast<unsigned>(d.DedicatedVideoMemory / (1024 * 1024)));
                i.merkmale.push_back(std::string("DedicatedVideoMemory ") + z);
            }
            i.karte = kartenName(ad);
            ad->Release();
        }
        dxdev->Release();
    }
    const D3D_FEATURE_LEVEL fl = g_device->GetFeatureLevel();
    const char* stufe = fl >= D3D_FEATURE_LEVEL_11_1 ? "11_1" : fl >= D3D_FEATURE_LEVEL_11_0 ? "11_0" : fl >= D3D_FEATURE_LEVEL_10_1 ? "10_1" : "10_0";
    i.version = std::string("Direct3D 11 - feature level ") + stufe;
    i.merkmale.push_back(std::string("D3D_FEATURE_LEVEL_") + stufe);
    std::string msaa = "MSAA R8G8B8A8_UNORM:";
    for (UINT n = 2; n <= 32; n *= 2) {
        UINT q = 0;
        if (SUCCEEDED(g_device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, n, &q)) && q > 0) { msaa += " " + std::to_string(n); }
    }
    i.merkmale.push_back(msaa);
    for (const auto& [format, name] : {std::pair{DXGI_FORMAT_BC1_UNORM, "BC1_UNORM"}, std::pair{DXGI_FORMAT_BC2_UNORM, "BC2_UNORM"},
                                       std::pair{DXGI_FORMAT_BC3_UNORM, "BC3_UNORM"}, std::pair{DXGI_FORMAT_D24_UNORM_S8_UINT, "D24_UNORM_S8_UINT"}}) {
        UINT unterstuetzt = 0;
        if (SUCCEEDED(g_device->CheckFormatSupport(format, &unterstuetzt)) && unterstuetzt != 0) { i.merkmale.push_back(std::string("DXGI_FORMAT_") + name); }
    }
    D3D11_FEATURE_DATA_THREADING t{};
    if (SUCCEEDED(g_device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &t, sizeof(t)))) {
        if (t.DriverConcurrentCreates != 0) { i.merkmale.emplace_back("DriverConcurrentCreates"); }
        if (t.DriverCommandLists != 0) { i.merkmale.emplace_back("DriverCommandLists"); }
    }
    D3D11_FEATURE_DATA_DOUBLES dbl{};
    if (SUCCEEDED(g_device->CheckFeatureSupport(D3D11_FEATURE_DOUBLES, &dbl, sizeof(dbl))) && dbl.DoublePrecisionFloatShaderOps != 0) {
        i.merkmale.emplace_back("DoublePrecisionFloatShaderOps");
    }
    return i;
}
#else
GeraeteInfo geraeteInfo() { return {}; }
std::string geraeteKarte() { return ""; }
std::string staerksteKarte() { return ""; }
std::string sparsamsteKarte() { return ""; }
#endif

void warteAufBild() {
#ifdef BHED_WITH_D3D11
    if (g_ready && g_backend == Backend::Direct3D11 && g_bildFrei != nullptr) {
        // Hoechstens eine Sekunde - haengt die Grafik, soll die Oberflaeche
        // nicht mit ihr stehen bleiben.
        WaitForSingleObjectEx(g_bildFrei, 1000, TRUE);
    }
#endif
}

bool geraetVerloren() {
#ifdef BHED_WITH_D3D11
    return g_geraetVerloren;
#else
    return false;
#endif
}

void geraetVerlorenSpielen() {
#ifdef BHED_WITH_D3D11
    g_verlustSpielen = true;
#endif
}

bool flipModell() {
#ifdef BHED_WITH_D3D11
    return g_swapFlip;
#else
    return false;
#endif
}

bool sparsamGewuenscht() {
    const char* wahl = std::getenv("BHED_GPU");
    return wahl != nullptr && std::string(wahl) == "sparsam";
}

void destroyTextures() {
    ++g_texturStand;   // rc601 saber review
#ifdef BHED_WITH_D3D11
    for (ID3D11ShaderResourceView* t : g_d3dTextures) {
        if (t != nullptr) { t->Release(); }
    }
    g_d3dTextures.clear();
#endif
    if (!g_glTextures.empty()) {
        glDeleteTextures(static_cast<GLsizei>(g_glTextures.size()), g_glTextures.data());
        g_glTextures.clear();
    }
}

const char* backendName(Backend b) {
    return b == Backend::Direct3D11 ? "Direct3D 11" : "OpenGL 3.3";
}

bool init(Backend b, HWNDHandle hwnd) {
    g_hwnd = reinterpret_cast<HWND>(hwnd);
    g_backend = b;
    g_ready = (b == Backend::Direct3D11) ? initD3D11(g_hwnd) : initGL3(g_hwnd);
    if (!g_ready) {
        // Halb aufgebaute Zustaende wieder abraeumen, sonst scheitert der
        // zweite Versuch an Resten des ersten.
        if (b == Backend::Direct3D11) { shutdownD3D11(); } else { shutdownGL3(); }
    }
    return g_ready;
}

void shutdown() {
    if (!g_ready) {
        return;
    }
    destroyTextures();
    if (g_backend == Backend::Direct3D11) { shutdownD3D11(); } else { shutdownGL3(); }
    g_ready = false;
}

#ifdef BHED_WITH_D3D11
// rc608 Teil 12: fertige Zeitmarken abholen, dann die naechste beginnen.
void bildMarkenAnfang() {
    for (BildMarken& m : g_bildMarken) {
        if (!m.offen) { continue; }
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT dj{};
        UINT64 a = 0;
        UINT64 e = 0;
        if (g_context->GetData(m.takt, &dj, sizeof(dj), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK ||
            g_context->GetData(m.anfang, &a, sizeof(a), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK ||
            g_context->GetData(m.ende, &e, sizeof(e), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK) {
            continue;
        }
        m.offen = false;
        if (dj.Disjoint == FALSE && dj.Frequency > 0 && e >= a) {
            g_bildZeiten.gpuMs = static_cast<double>(e - a) * 1000.0 / static_cast<double>(dj.Frequency);
            ++g_bildZeiten.gpuBilder;
        }
    }
    BildMarken& m = g_bildMarken[g_bildMarkeJetzt];
    if (m.offen) { return; }
    if (m.takt == nullptr) {
        D3D11_QUERY_DESC qd{};
        qd.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        if (FAILED(g_device->CreateQuery(&qd, &m.takt))) { return; }
        qd.Query = D3D11_QUERY_TIMESTAMP;
        if (FAILED(g_device->CreateQuery(&qd, &m.anfang)) || FAILED(g_device->CreateQuery(&qd, &m.ende))) { return; }
    }
    g_context->Begin(m.takt);
    g_context->End(m.anfang);
    g_bildMarkeLaeuft = true;
}

void bildMarkenEnde() {
    if (!g_bildMarkeLaeuft) { return; }
    BildMarken& m = g_bildMarken[g_bildMarkeJetzt];
    g_context->End(m.ende);
    g_context->End(m.takt);
    m.offen = true;
    g_bildMarkeLaeuft = false;
    g_bildMarkeJetzt = (g_bildMarkeJetzt + 1U) % 4U;
}
#endif

void bildMessung(bool an) {
    g_bildMessung = an;
    g_bildZeiten = BildZeiten{};
}

BildZeiten bildZeiten() { return g_bildZeiten; }

void newFrame() {
    if (!g_ready) {
        return;
    }
    if (g_bildMessung) { g_bildAnfang = std::chrono::steady_clock::now(); }
#ifdef BHED_WITH_D3D11
    if (g_backend == Backend::Direct3D11) {
        if (g_bildMessung) { bildMarkenAnfang(); }
        ImGui_ImplDX11_NewFrame();
        return;
    }
#endif
    ImGui_ImplOpenGL3_NewFrame();
}

std::string g_fotoPfad;   // leer = kein Foto angefordert
bool g_fotoMitFenstern = false;   // rc609 T1: die eigenen Fenster ueber dem Hauptfenster mit ins Bild

void fotoAnfordern(const std::string& pfad) { g_fotoPfad = pfad; }

void fotoMitFensternAnfordern(const std::string& pfad) {
    g_fotoPfad = pfad;
    g_fotoMitFenstern = true;
}

#ifdef BHED_WITH_D3D11
// Ein Bild im Speicher (RGBA, Zeile fuer Zeile von oben).
struct Bild {
    std::uint32_t w = 0;
    std::uint32_t h = 0;
    std::vector<unsigned char> rgba;
};
Bild g_fotoHaupt;   // rc609 T1: das Hauptfenster, bis die eigenen Fenster dazukommen

// Ein Renderziel (Hintergrundpuffer) in den Speicher lesen.
bool zielLesen(ID3D11Texture2D* ziel, Bild& aus) {
    D3D11_TEXTURE2D_DESC d{};
    ziel->GetDesc(&d);
    d.Usage = D3D11_USAGE_STAGING;
    d.BindFlags = 0;
    d.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    d.MiscFlags = 0;
    ID3D11Texture2D* kopie = nullptr;
    if (FAILED(g_device->CreateTexture2D(&d, nullptr, &kopie)) || kopie == nullptr) { return false; }
    g_context->CopyResource(kopie, ziel);
    D3D11_MAPPED_SUBRESOURCE m{};
    const bool ok = SUCCEEDED(g_context->Map(kopie, 0, D3D11_MAP_READ, 0, &m));
    if (ok) {
        aus.w = d.Width;
        aus.h = d.Height;
        aus.rgba.resize(static_cast<std::size_t>(d.Width) * d.Height * 4U);
        for (std::uint32_t y = 0; y < d.Height; ++y) {
            std::memcpy(aus.rgba.data() + static_cast<std::size_t>(y) * d.Width * 4U, static_cast<const unsigned char*>(m.pData) + y * m.RowPitch,
                        static_cast<std::size_t>(d.Width) * 4U);
        }
        g_context->Unmap(kopie, 0);
    }
    kopie->Release();
    return ok;
}

// Als 32-Bit-BMP schreiben.
void bmpSchreiben(const std::string& pfad, const Bild& b) {
    std::FILE* f = std::fopen(pfad.c_str(), "wb");
    if (f == nullptr) { return; }
    const std::uint32_t w = b.w, hgt = b.h;
    const std::uint32_t bild = w * hgt * 4U;
    unsigned char kopf[54] = {'B', 'M'};
    auto u32 = [&](int at, std::uint32_t v) {
        for (int k = 0; k < 4; ++k) { kopf[at + k] = static_cast<unsigned char>(v >> (8 * k)); }
    };
    u32(2, 54U + bild); u32(10, 54U); u32(14, 40U); u32(18, w);
    u32(22, static_cast<std::uint32_t>(-static_cast<std::int32_t>(hgt)));   // von oben
    kopf[26] = 1; kopf[28] = 32; u32(34, bild);
    std::fwrite(kopf, 1, sizeof(kopf), f);
    std::vector<unsigned char> zeile(w * 4U);
    for (std::uint32_t y = 0; y < hgt; ++y) {
        const unsigned char* q = b.rgba.data() + static_cast<std::size_t>(y) * w * 4U;
        for (std::uint32_t x = 0; x < w; ++x) {   // RGBA -> BGRA
            zeile[x * 4U + 0U] = q[x * 4U + 2U];
            zeile[x * 4U + 1U] = q[x * 4U + 1U];
            zeile[x * 4U + 2U] = q[x * 4U + 0U];
            zeile[x * 4U + 3U] = 255;
        }
        std::fwrite(zeile.data(), 1, zeile.size(), f);
    }
    std::fclose(f);
}

// Den Hintergrundpuffer auslesen und als 32-Bit-BMP schreiben. Mit den eigenen
// Fenstern (rc609 T1) wartet das Bild, bis plattformFenster sie gezeichnet und
// an ihrer Stelle eingesetzt hat - so, wie sie auf dem Bildschirm ueber dem
// Hauptfenster liegen, aber nur aus den eigenen Grafikpuffern.
void fotoSchreiben() {
    ID3D11Texture2D* back = nullptr;
    if (FAILED(g_swap->GetBuffer(0, IID_PPV_ARGS(&back))) || back == nullptr) {
        g_fotoPfad.clear();
        g_fotoMitFenstern = false;
        return;
    }
    Bild b;
    const bool ok = zielLesen(back, b);
    back->Release();
    if (ok && g_fotoMitFenstern) {
        g_fotoHaupt = std::move(b);
        return;
    }
    if (ok) { bmpSchreiben(g_fotoPfad, b); }
    g_fotoPfad.clear();
}

// Das gerade gezeichnete eigene Fenster `v` an seiner Stelle ins Bild des Hauptfensters.
void fensterInsFoto(const ImGuiViewport* v) {
    ID3D11RenderTargetView* rtv = nullptr;
    g_context->OMGetRenderTargets(1, &rtv, nullptr);
    if (rtv == nullptr) { return; }
    ID3D11Resource* res = nullptr;
    rtv->GetResource(&res);
    rtv->Release();
    ID3D11Texture2D* tex = nullptr;
    if (res == nullptr || FAILED(res->QueryInterface(IID_PPV_ARGS(&tex))) || tex == nullptr) {
        if (res != nullptr) { res->Release(); }
        return;
    }
    res->Release();
    Bild f;
    const bool ok = zielLesen(tex, f);
    tex->Release();
    if (!ok || g_fotoHaupt.rgba.empty()) { return; }
    const ImGuiViewport* haupt = ImGui::GetMainViewport();
    const int x0 = static_cast<int>(std::lround(v->Pos.x - haupt->Pos.x));
    const int y0 = static_cast<int>(std::lround(v->Pos.y - haupt->Pos.y));
    for (int y = 0; y < static_cast<int>(f.h); ++y) {
        const int zy = y0 + y;
        if (zy < 0 || zy >= static_cast<int>(g_fotoHaupt.h)) { continue; }
        for (int x = 0; x < static_cast<int>(f.w); ++x) {
            const int zx = x0 + x;
            if (zx < 0 || zx >= static_cast<int>(g_fotoHaupt.w)) { continue; }
            std::memcpy(g_fotoHaupt.rgba.data() + (static_cast<std::size_t>(zy) * g_fotoHaupt.w + static_cast<std::size_t>(zx)) * 4U,
                        f.rgba.data() + (static_cast<std::size_t>(y) * f.w + static_cast<std::size_t>(x)) * 4U, 4U);
        }
    }
}
#endif

// --- dock1: die eigenen Fenster der Panels (Multi-Viewport) -------------------
//
// Wie ImGui::RenderPlatformWindowsDefault: erst alle zeichnen, dann alle
// zeigen. Ausgeschrieben, damit der Selbsttest das fertige Bild eines Fensters
// lesen kann, bevor es gezeigt wird (Direct3D 11: das Renderziel, das
// ImGui_ImplDX11_RenderWindow gesetzt hat).
namespace {
unsigned g_vpBildFuer = 0;   // angefordert fuer dieses Fenster (ImGuiViewport::ID)
ViewportBild g_vpBild;

#ifdef BHED_WITH_D3D11
void viewportBildLesen(unsigned id) {
    ViewportBild ergebnis;
    ergebnis.id = id;
    ID3D11RenderTargetView* rtv = nullptr;
    g_context->OMGetRenderTargets(1, &rtv, nullptr);
    if (rtv == nullptr) { return; }
    ID3D11Resource* res = nullptr;
    rtv->GetResource(&res);
    rtv->Release();
    ID3D11Texture2D* tex = nullptr;
    if (res == nullptr || FAILED(res->QueryInterface(IID_PPV_ARGS(&tex))) || tex == nullptr) {
        if (res != nullptr) { res->Release(); }
        return;
    }
    res->Release();
    D3D11_TEXTURE2D_DESC d{};
    tex->GetDesc(&d);
    d.Usage = D3D11_USAGE_STAGING;
    d.BindFlags = 0;
    d.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    d.MiscFlags = 0;
    ID3D11Texture2D* kopie = nullptr;
    if (SUCCEEDED(g_device->CreateTexture2D(&d, nullptr, &kopie)) && kopie != nullptr) {
        g_context->CopyResource(kopie, tex);
        D3D11_MAPPED_SUBRESOURCE m{};
        if (SUCCEEDED(g_context->Map(kopie, 0, D3D11_MAP_READ, 0, &m))) {
            std::vector<std::uint32_t> farben;
            for (std::uint32_t y = 0; y < d.Height; y += 2U) {
                const auto* zeile = reinterpret_cast<const std::uint32_t*>(static_cast<const unsigned char*>(m.pData) + y * m.RowPitch);
                for (std::uint32_t x = 0; x < d.Width; x += 2U) {
                    const std::uint32_t c = zeile[x] & 0x00FFFFFFU;
                    if (farben.size() < 4096U && std::find(farben.begin(), farben.end(), c) == farben.end()) { farben.push_back(c); }
                }
            }
            ergebnis.breite = static_cast<int>(d.Width);
            ergebnis.hoehe = static_cast<int>(d.Height);
            ergebnis.farben = static_cast<int>(farben.size());
            ergebnis.da = true;
            g_context->Unmap(kopie, 0);
        }
        kopie->Release();
    }
    tex->Release();
    g_vpBild = ergebnis;
}
#endif

// OpenGL: der hintere Puffer des Fensters, dessen Kontext gerade gilt
// (glFensterZeichnen hat ihn gesetzt, ImGui_ImplOpenGL3_RenderWindow gezeichnet).
void viewportBildLesenGl(const ImGuiViewport* v) {
    ViewportBild ergebnis;
    ergebnis.id = v->ID;
    RECT innen{};
    if (GetClientRect(static_cast<HWND>(v->PlatformHandle), &innen) == FALSE) { return; }
    const int w = innen.right - innen.left;   // der Puffer des Fensters ist so gross wie seine Innenflaeche
    const int h = innen.bottom - innen.top;
    if (w <= 0 || h <= 0) { return; }
    std::vector<std::uint32_t> punkte(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    while (glGetError() != GL_NO_ERROR) {}   // alte Fehler anderer Aufrufe, sie gelten nicht dem Lesen
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, punkte.data());
    if (const GLenum fehler = glGetError(); fehler != GL_NO_ERROR) {
        diag::detail("OpenGL: Bild eines eigenen Fensters nicht lesbar (Fehler " + std::to_string(fehler) + ")");
        return;
    }
    std::vector<std::uint32_t> farben;
    for (int y = 0; y < h; y += 2) {
        for (int x = 0; x < w; x += 2) {
            const std::uint32_t c = punkte[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)] & 0x00FFFFFFU;
            if (farben.size() < 4096U && std::find(farben.begin(), farben.end(), c) == farben.end()) { farben.push_back(c); }
        }
    }
    ergebnis.breite = w;
    ergebnis.hoehe = h;
    ergebnis.farben = static_cast<int>(farben.size());
    ergebnis.da = true;
    g_vpBild = ergebnis;
}

double g_plattformMs = 0.0;

// --- rc609 T1: Leerlauf je Fenster -------------------------------------------
//
// "Ein ruhendes Fenster zeichnet nicht": ein eigenes Fenster (Panel aus dock1,
// Tab aus T1) zeigt nur dann ein neues Bild, wenn es anders aussaehe als das
// zuletzt gezeigte - seine Zeichenliste (Ecken, Indizes, Befehle, Lage, Groesse,
// Massstab) hat eine andere Pruefsumme, eine Textur darin wurde in diesem Bild
// bemalt (bemalt.h, Hochladen hier), ImGui hat Texturen geaendert (Schrift),
// oder ein Befehl ruft zurueck (dessen Ergebnis kennt nur der Aufrufer). Sonst
// bleibt das gezeigte Bild stehen - es ist dasselbe, also kein Unterschied auf
// dem Schirm. Das Hauptfenster zeichnet weiter jedes Bild: Sein Present taktet
// die Schleife (Present(1, 0)), und ohne Eingabe ruht ohnehin alles (rc596,
// main_win32.cpp).
std::unordered_map<unsigned, unsigned long long> g_vpBilder;   // gezeigte Bilder je eigenem Fenster
std::unordered_map<unsigned, std::uint64_t> g_vpSumme;         // Pruefsumme des zuletzt gezeigten
std::vector<const void*> g_bemalt;                             // in diesem Bild bemalt (Ressourcen)
bool g_texturenNeu = false;
enum Grund : std::uint8_t { kGrundErstes, kGrundSumme, kGrundBemalt, kGrundTextur, kGrundAnders, kGruende };
std::unordered_map<unsigned, std::array<unsigned long long, kGruende>> g_vpGruende;
std::unordered_map<unsigned, std::map<std::string, std::uint64_t>> g_vpListen;   // Pruefsumme je Zeichenliste
std::unordered_map<unsigned, std::string> g_vpAnders;                            // zuletzt geaenderte Zeichenlisten

// xxHash64-Runde je acht Bytes: jedes Bit des Eingangs wirkt auf alle der Summe.
struct Pruefsumme {
    std::uint64_t h = 0x27D4EB2F165667C5ULL;
    void wort(std::uint64_t w) {
        w *= 0xC2B2AE3D27D4EB4FULL;
        w = (w << 31U) | (w >> 33U);
        w *= 0x9E3779B185EBCA87ULL;
        h ^= w;
        h = ((h << 27U) | (h >> 37U)) * 0x9E3779B185EBCA87ULL + 0x85EBCA77C2B2AE63ULL;
    }
    void bytes(const void* p, std::size_t n) {
        const auto* b = static_cast<const unsigned char*>(p);
        std::size_t i = 0;
        for (; i + 8U <= n; i += 8U) {
            std::uint64_t w = 0;
            std::memcpy(&w, b + i, 8U);
            wort(w);
        }
        std::uint64_t rest = 0;
        if (i < n) { std::memcpy(&rest, b + i, n - i); }
        wort(rest ^ (static_cast<std::uint64_t>(n) << 56U));
    }
    void zahl(float f) {
        std::uint32_t u = 0;
        std::memcpy(&u, &f, 4U);
        wort(u);
    }
};

bool bemaltIst(const void* p) { return std::find(g_bemalt.begin(), g_bemalt.end(), p) != g_bemalt.end(); }

// Muss das Fenster neu gezeichnet werden? `summe` ist danach die des neuen Bildes.
bool brauchtBild(const ImGuiViewport* v, std::uint64_t& summe) {
    const ImDrawData* d = v->DrawData;
    auto& gruende = g_vpGruende[v->ID];
    if (d == nullptr || !d->Valid || g_backend != Backend::Direct3D11) {
        ++gruende[kGrundAnders];
        return true;
    }
    if (g_texturenNeu) {
        ++gruende[kGrundTextur];
        return true;
    }
    Pruefsumme p;
    p.zahl(d->DisplayPos.x);
    p.zahl(d->DisplayPos.y);
    p.zahl(d->DisplaySize.x);
    p.zahl(d->DisplaySize.y);
    p.zahl(d->FramebufferScale.x);
    p.zahl(d->FramebufferScale.y);
    bool bemaltDrin = false;
    auto& listen = g_vpListen[v->ID];
    std::string anders;
    for (const ImDrawList* l : d->CmdLists) {
        Pruefsumme eine;
        eine.bytes(l->VtxBuffer.Data, static_cast<std::size_t>(l->VtxBuffer.Size) * sizeof(ImDrawVert));
        eine.bytes(l->IdxBuffer.Data, static_cast<std::size_t>(l->IdxBuffer.Size) * sizeof(ImDrawIdx));
        for (const ImDrawCmd& c : l->CmdBuffer) {
            eine.bytes(&c.ClipRect, sizeof(c.ClipRect));
            eine.wort((static_cast<std::uint64_t>(c.VtxOffset) << 32U) | c.IdxOffset);
            eine.wort(c.ElemCount);
            if (c.UserCallback != nullptr) {
                // Die Rueckrufe des Programms setzen nur Zeichenzustand (zeichneAdditiv,
                // zeichneStufe, kartegpu::ohneMischen, ImDrawCallback_ResetRenderState):
                // gleicher Rueckruf mit gleichen Daten zeichnet gleich. Ein Rueckruf, der
                // selbst etwas malt, muss sein Ziel ueber bemalt.h melden.
                eine.wort(static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(c.UserCallback)));
                eine.wort(static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(c.UserCallbackData)));
                continue;
            }
            const ImTextureID t = c.GetTexID();
            eine.wort(static_cast<std::uint64_t>(t));
#ifdef BHED_WITH_D3D11
            if (!bemaltDrin && t != ImTextureID_Invalid && !g_bemalt.empty()) {
                auto* srv = reinterpret_cast<ID3D11ShaderResourceView*>(static_cast<std::uintptr_t>(t));
                ID3D11Resource* r = nullptr;
                srv->GetResource(&r);
                bemaltDrin = bemaltIst(r);
                if (r != nullptr) { r->Release(); }
            }
#endif
        }
        // Je Zeichenliste (ein ImGui-Fenster): welche hat sich geaendert - fuers Protokoll
        const std::string besitzer = (l->_OwnerName != nullptr) ? l->_OwnerName : "?";
        if (const auto alt = listen.find(besitzer); alt != listen.end() && alt->second != eine.h && anders.size() < 200U) {
            anders += (anders.empty() ? "" : ", ") + besitzer;
        }
        listen[besitzer] = eine.h;
        p.wort(eine.h);
    }
    summe = p.h;
    const auto it = g_vpSumme.find(v->ID);
    if (it == g_vpSumme.end()) {
        ++gruende[kGrundErstes];
    } else if (it->second != summe) {
        ++gruende[kGrundSumme];
        if (!anders.empty()) { g_vpAnders[v->ID] = anders; }
    } else if (bemaltDrin) {
        ++gruende[kGrundBemalt];
    }
    return bemaltDrin || it == g_vpSumme.end() || it->second != summe;
}

void (*g_zeigenVorher)(ImGuiViewport*) = nullptr;
void verborgenZeigen(ImGuiViewport* v) {
    if (const HWND h = static_cast<HWND>(v->PlatformHandleRaw); h != nullptr) {
        const BOOL an = TRUE;
        (void)DwmSetWindowAttribute(h, DWMWA_CLOAK, &an, sizeof(an));
    }
    if (g_zeigenVorher != nullptr) { g_zeigenVorher(v); }
}

void plattformFenster() {
    if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0) { return; }
    const auto anfang = std::chrono::steady_clock::now();
    ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    // Endrunde 4: im Selbsttest auch die eigenen Fenster der Panels verborgen (wie das Hauptfenster, main_win32.cpp),
    // vor dem ersten Zeigen - sonst blitzte ein neues Fenster ein Bild lang auf
    static const bool verbergen = std::getenv("BHED_EDITORTEST") != nullptr || std::getenv("BHED_EINZEL") != nullptr;
    if (verbergen && pio.Platform_ShowWindow != &verborgenZeigen) {
        g_zeigenVorher = pio.Platform_ShowWindow;
        pio.Platform_ShowWindow = &verborgenZeigen;
    }
    ImGui::UpdatePlatformWindows();
    static std::vector<unsigned> zeigen;   // Fenster mit neuem Bild (rc609 T1)
    static std::vector<unsigned> klein;    // im letzten Bild verkleinert: danach neu zeichnen
    zeigen.clear();
    std::vector<unsigned> jetztKlein;
    for (int i = 1; i < pio.Viewports.Size; ++i) {
        ImGuiViewport* v = pio.Viewports[i];
        if ((v->Flags & ImGuiViewportFlags_IsMinimized) != 0) {
            jetztKlein.push_back(v->ID);
            continue;
        }
        std::uint64_t summe = 0;
#ifdef BHED_WITH_D3D11
        const bool fuerFoto = !g_fotoHaupt.rgba.empty();
#else
        const bool fuerFoto = false;
#endif
        const bool angefordert = (g_vpBildFuer != 0 && v->ID == g_vpBildFuer) || fuerFoto;
        const bool warKlein = std::find(klein.begin(), klein.end(), v->ID) != klein.end();
        if (!brauchtBild(v, summe) && !angefordert && !warKlein) { continue; }
        g_vpSumme[v->ID] = summe;
        zeigen.push_back(v->ID);
        if (pio.Platform_RenderWindow != nullptr) { pio.Platform_RenderWindow(v, nullptr); }
        if (pio.Renderer_RenderWindow != nullptr) { pio.Renderer_RenderWindow(v, nullptr); }
#ifdef BHED_WITH_D3D11
        if (g_backend == Backend::Direct3D11 && g_vpBildFuer != 0 && v->ID == g_vpBildFuer) {
            g_vpBildFuer = 0;
            viewportBildLesen(v->ID);
        }
        if (g_backend == Backend::Direct3D11 && !g_fotoHaupt.rgba.empty()) { fensterInsFoto(v); }
#endif
        if (g_backend == Backend::OpenGL3 && g_vpBildFuer != 0 && v->ID == g_vpBildFuer) {
            g_vpBildFuer = 0;
            viewportBildLesenGl(v);
        }
    }
    for (int i = 1; i < pio.Viewports.Size; ++i) {
        ImGuiViewport* v = pio.Viewports[i];
        if (std::find(zeigen.begin(), zeigen.end(), v->ID) == zeigen.end()) { continue; }
        if (pio.Platform_SwapBuffers != nullptr) { pio.Platform_SwapBuffers(v, nullptr); }
        if (pio.Renderer_SwapBuffers != nullptr) { pio.Renderer_SwapBuffers(v, nullptr); }
        ++g_vpBilder[v->ID];
    }
    klein.swap(jetztKlein);
    g_bemalt.clear();
    g_texturenNeu = false;
#ifdef BHED_WITH_D3D11
    if (!g_fotoHaupt.rgba.empty()) {   // rc609 T1: das Bild mit den eigenen Fenstern ist fertig
        bmpSchreiben(g_fotoPfad, g_fotoHaupt);
        g_fotoHaupt = Bild{};
        g_fotoPfad.clear();
        g_fotoMitFenstern = false;
    }
#endif
    if (g_backend == Backend::OpenGL3) { wglMakeCurrent(g_dc, g_rc); }
    g_plattformMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - anfang).count();
}
}  // namespace

double plattformMs() { return g_plattformMs; }

void viewportBildAnfordern(unsigned id) {
    g_vpBildFuer = id;
    g_vpBild = ViewportBild{};
}

ViewportBild viewportBild() { return g_vpBild; }

#ifdef BHED_WITH_D3D11
void bemaltMerken(ID3D11Resource* ziel) {
    if (ziel != nullptr && !bemaltIst(ziel)) { g_bemalt.push_back(ziel); }
}
#endif

std::string viewportGruende(unsigned id) {
    const auto it = g_vpGruende.find(id);
    if (it == g_vpGruende.end()) { return "-"; }
    const auto& z = it->second;
    return "erstes " + std::to_string(z[kGrundErstes]) + ", Zeichenliste " + std::to_string(z[kGrundSumme]) + ", bemalt " +
           std::to_string(z[kGrundBemalt]) + ", Texturen " + std::to_string(z[kGrundTextur]) + ", sonst " +
           std::to_string(z[kGrundAnders]) + (g_vpAnders.count(id) != 0U ? " (zuletzt geaendert: " + g_vpAnders.at(id) + ")" : std::string());
}

unsigned long long viewportBilder(unsigned id) {
    const auto it = g_vpBilder.find(id);
    return it != g_vpBilder.end() ? it->second : 0ULL;
}

void plattformHakenSetzen() {
    if (g_backend != Backend::OpenGL3) { return; }
    ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    pio.Renderer_CreateWindow = glFensterAnlegen;
    pio.Renderer_DestroyWindow = glFensterWeg;
    pio.Renderer_SwapBuffers = glFensterTauschen;
    pio.Platform_RenderWindow = glFensterZeichnen;
}

void present(float r, float g, float bl) {
    if (!g_ready) {
        return;
    }
#ifdef BHED_WITH_D3D11
    if (g_backend == Backend::Direct3D11) {
        const float clear[4] = {r, g, bl, 1.0F};
        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_context->ClearRenderTargetView(g_rtv, clear);
        // rc609 T1: aendert ImGui Texturen (Schrift), sehen alle Fenster neu aus
        if (const ImDrawData* d = ImGui::GetDrawData(); d != nullptr && d->Textures != nullptr) {
            for (const ImTextureData* t : *d->Textures) {
                if (t->Status != ImTextureStatus_OK) { g_texturenNeu = true; }
            }
        }
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        if (!g_fotoPfad.empty()) { fotoSchreiben(); }
        plattformFenster();
        if (g_bildMessung) {
            bildMarkenEnde();
            const auto vor = std::chrono::steady_clock::now();
            g_bildZeiten.cpuMs = std::chrono::duration<double, std::milli>(vor - g_bildAnfang).count();
            ++g_bildZeiten.cpuBilder;
        }
        const HRESULT hr = g_swap->Present(1, 0);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET || g_verlustSpielen) {
            // Den Grund VOR dem Freigeben fragen - danach gibt es ihn nicht mehr.
            const HRESULT grund = g_verlustSpielen ? S_OK : g_device->GetDeviceRemovedReason();
            char z[160];
            std::snprintf(z, sizeof(z), "Grafikgeraet verloren: Present 0x%08lX, Grund 0x%08lX%s",
                          static_cast<unsigned long>(hr), static_cast<unsigned long>(grund),
                          g_verlustSpielen ? " (Selbsttest)" : "");
            diag::error(z);
            g_verlustSpielen = false;
            g_geraetVerloren = true;
        }
        return;
    }
#endif
    {
        const ImGuiIO& io = ImGui::GetIO();
        glViewport(0, 0, static_cast<int>(io.DisplaySize.x),
                   static_cast<int>(io.DisplaySize.y));
        glClearColor(r, g, bl, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        plattformFenster();
        SwapBuffers(g_dc);
    }
}

void resize(int width, int height) {
    if (!g_ready || width <= 0 || height <= 0) {
        return;
    }
#ifdef BHED_WITH_D3D11
    if (g_backend == Backend::Direct3D11) {
        releaseRtv();
        g_swap->ResizeBuffers(0, static_cast<UINT>(width), static_cast<UINT>(height),
                              DXGI_FORMAT_UNKNOWN, g_swapFlags);
        createRtv();
    }
#else
    (void)width;
    (void)height;
#endif
    // Bei OpenGL genuegt der Aufruf von glViewport beim Zeichnen.
}

Backend current() { return g_backend; }
bool ready() { return g_ready; }

}  // namespace bhed::render
