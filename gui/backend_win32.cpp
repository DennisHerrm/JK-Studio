// backend_win32.cpp - Direct3D 11 und OpenGL 3.3
//
// Beide in einer Datei, weil sie sich denselben Zustand und dieselbe
// Umschaltlogik teilen. Getrennt waeren es zwei Dateien mit je zehn Zeilen
// Eigenleben und einer gemeinsamen Kopfdatei fuer den Rest.

#include <chrono>
#include <cstdint>
#include <map>
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
#endif

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

void newFrame() {
    if (!g_ready) {
        return;
    }
#ifdef BHED_WITH_D3D11
    if (g_backend == Backend::Direct3D11) {
        ImGui_ImplDX11_NewFrame();
        return;
    }
#endif
    ImGui_ImplOpenGL3_NewFrame();
}

std::string g_fotoPfad;   // leer = kein Foto angefordert

void fotoAnfordern(const std::string& pfad) { g_fotoPfad = pfad; }

#ifdef BHED_WITH_D3D11
// Den Hintergrundpuffer auslesen und als 32-Bit-BMP schreiben.
void fotoSchreiben() {
    const std::string pfad = g_fotoPfad;
    g_fotoPfad.clear();
    ID3D11Texture2D* back = nullptr;
    if (FAILED(g_swap->GetBuffer(0, IID_PPV_ARGS(&back))) || back == nullptr) { return; }
    D3D11_TEXTURE2D_DESC d{};
    back->GetDesc(&d);
    d.Usage = D3D11_USAGE_STAGING;
    d.BindFlags = 0;
    d.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    d.MiscFlags = 0;
    ID3D11Texture2D* kopie = nullptr;
    if (SUCCEEDED(g_device->CreateTexture2D(&d, nullptr, &kopie)) && kopie != nullptr) {
        g_context->CopyResource(kopie, back);
        D3D11_MAPPED_SUBRESOURCE m{};
        if (SUCCEEDED(g_context->Map(kopie, 0, D3D11_MAP_READ, 0, &m))) {
            std::FILE* f = std::fopen(pfad.c_str(), "wb");
            if (f != nullptr) {
                const std::uint32_t w = d.Width, hgt = d.Height;
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
                    const unsigned char* q = static_cast<const unsigned char*>(m.pData) + y * m.RowPitch;
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
            g_context->Unmap(kopie, 0);
        }
        kopie->Release();
    }
    back->Release();
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

double g_plattformMs = 0.0;

void plattformFenster() {
    if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0) { return; }
    const auto anfang = std::chrono::steady_clock::now();
    ImGui::UpdatePlatformWindows();
    ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    for (int i = 1; i < pio.Viewports.Size; ++i) {
        ImGuiViewport* v = pio.Viewports[i];
        if ((v->Flags & ImGuiViewportFlags_IsMinimized) != 0) { continue; }
        if (pio.Platform_RenderWindow != nullptr) { pio.Platform_RenderWindow(v, nullptr); }
        if (pio.Renderer_RenderWindow != nullptr) { pio.Renderer_RenderWindow(v, nullptr); }
#ifdef BHED_WITH_D3D11
        if (g_backend == Backend::Direct3D11 && g_vpBildFuer != 0 && v->ID == g_vpBildFuer) {
            g_vpBildFuer = 0;
            viewportBildLesen(v->ID);
        }
#endif
    }
    for (int i = 1; i < pio.Viewports.Size; ++i) {
        ImGuiViewport* v = pio.Viewports[i];
        if ((v->Flags & ImGuiViewportFlags_IsMinimized) != 0) { continue; }
        if (pio.Platform_SwapBuffers != nullptr) { pio.Platform_SwapBuffers(v, nullptr); }
        if (pio.Renderer_SwapBuffers != nullptr) { pio.Renderer_SwapBuffers(v, nullptr); }
    }
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
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        if (!g_fotoPfad.empty()) { fotoSchreiben(); }
        plattformFenster();
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
