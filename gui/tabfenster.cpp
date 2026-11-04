// tabfenster.cpp - rc609 T1: ganze Tabs in eigenen Fenstern (tabfenster.h)
//
// Warum ImGui-Viewports und kein eigenes Win32-Fenster je Tab: Die Panels aus
// dock1 stehen schon so in eigenen Windows-Fenstern (main_win32.cpp,
// ImGuiConfigFlags_ViewportsEnable; backend_win32.cpp legt je Viewport eine
// Swapchain auf demselben Geraet an). Ein ganzer Tab ist dasselbe, nur mit dem
// ganzen Inhalt: dieselben Zeichenfunktionen, dieselben Texturen (Kartenbild,
// Modell, Map-Editor-Ansichten) ohne Kopie, ein ImGui-Kontext - also gehen
// Ziehen und Ablegen, Klappmenues und Tooltips auch zwischen den Fenstern, und
// die Schrift folgt dem Bildschirm (ConfigDpiScaleFonts). Ein eigenes
// Win32-Fenster haette einen zweiten ImGui-Kontext gebraucht: zweite
// Schriftatlanten, zweite Tastatur- und Mauswege, und kein Ziehen zwischen den
// Fenstern.
#include "tabfenster.h"

#include "app_internal.h"
#include "fenster_dock.h"
#include "bhed/diag.h"
#include "bhed/fensteranordnung.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace bhed::gui::tabfenster {

namespace {

// Was je Fenster gilt (Bestandsaufnahme, "Folgerung fuer Stufe 2"): Felder des
// Programms, die beim Zeichnen gelesen und geschrieben werden und sonst
// zwischen zwei Fenstern im selben Bild hin und her liefen. Beim Zeichnen
// eines eigenen Fensters stehen seine Werte in g_app, danach wieder die des
// Hauptfensters.
struct Felder {
    int leftMode = 0;
    int letzterModus = -1;
    bool moduswechselBild = false;
    float letzteFensterB = 0.0F;
    int fensterStabilSeit = 0;
    float modeBarW = 0.0F;
    int reiterWahl = -1;
};

void tauschen(Felder& f) {
    std::swap(g_app->leftMode, f.leftMode);
    std::swap(g_app->letzterModus, f.letzterModus);
    std::swap(g_app->moduswechselBild, f.moduswechselBild);
    std::swap(g_app->letzteFensterB, f.letzteFensterB);
    std::swap(g_app->fensterStabilSeit, f.fensterStabilSeit);
    std::swap(g_app->modeBarW, f.modeBarW);
    std::swap(g_app->reiterWahl, f.reiterWahl);
}

struct Fenster {
    int nr = 0;                  // "###tabfenster.<nr>": bleibt, solange das Fenster lebt
    Felder felder;               // felder.leftMode ist der Tab des Fensters
    bool lageSetzen = true;      // im naechsten Bild Lage und Groesse vorgeben
    bool fokusWunsch = false;    // im naechsten Bild nach vorn
    bool offen = true;           // Schliessknopf
    fenster::Rechteck lage;      // zuletzt gesehen (Bildschirmpunkte)
    fenster::Rechteck bildschirm;
    ImGuiID fensterId = 0;
    bool wirdBewegt = false;     // Titelleiste gezogen (Zurueckziehen)
    bool ueberLeiste = false;
    int tabVorher = 0;           // Tab beim Beginn des Zeichnens
};

enum class Art { Loesen, Zurueck, AlleZurueck, Zeigen };
struct Wunsch {
    Art art = Art::Zeigen;
    int modus = 0;
    bool mitLage = false;
    ImVec2 lage;
};

struct Stand {
    std::vector<Fenster> fenster;
    std::vector<Wunsch> wuensche;
    int naechsteNr = 1;
    int fokus = -1;              // Index in `fenster`, -1 = Hauptfenster
    int zeichnet = -1;           // Index des gerade zeichnenden Fensters
    int hauptLetzter = 0;        // letzter gueltiger Tab des Hauptfensters
    bool geladen = false;
    std::string gemerkt;         // zuletzt in die Einstellungen geschrieben
    int ziehtModus = -1;         // Knopf der Modusleiste wird gezogen
    ImRect hauptLeiste;          // Modusleiste des Hauptfensters (Bildschirmpunkte)
    int hauptLeisteBild = -1;
    bool ladenOffen = false;     // eine Liste uebernehmen (benannte Anordnung), im naechsten Bild
    std::string laden;
};

Stand& st() {
    static Stand s;
    return s;
}

bool imSelbsttest() {
    static const bool an = std::getenv("BHED_EDITORTEST") != nullptr || std::getenv("BHED_EINZEL") != nullptr;
    return an;
}

// Der Tab des Hauptfensters, auch waehrend ein eigenes Fenster zeichnet.
int hauptModus() {
    const Stand& s = st();
    if (s.zeichnet >= 0 && s.zeichnet < static_cast<int>(s.fenster.size())) {
        return s.fenster[static_cast<std::size_t>(s.zeichnet)].felder.leftMode;   // getauscht: dort steht der des Hauptfensters
    }
    return g_app->leftMode;
}

int indexVon(int modus) {
    const Stand& s = st();
    for (std::size_t i = 0; i < s.fenster.size(); ++i) {
        const int m = (static_cast<int>(i) == s.zeichnet) ? g_app->leftMode : s.fenster[i].felder.leftMode;
        if (m == modus) { return static_cast<int>(i); }
    }
    return -1;
}

int modusVon(int index) {
    const Stand& s = st();
    if (index < 0 || index >= static_cast<int>(s.fenster.size())) { return hauptModus(); }
    return (index == s.zeichnet) ? g_app->leftMode : s.fenster[static_cast<std::size_t>(index)].felder.leftMode;
}

const char* name(int modus) {
    switch (modus) {
        case 1: return tr(Str::ViewMap);
        case 2: return tr(Str::ViewModel);
        case 3: return tr(Str::ViewMapEditor);
        case 4: return tr(Str::ViewMenuEditor);
        default: return tr(Str::ViewEvents);
    }
}

// Der Titel nennt den Tab; die Kennung bleibt, solange das Fenster lebt.
std::string fensterName(int nr, int modus) { return std::string(name(modus)) + " - JK Studio###tabfenster." + std::to_string(nr); }

// Der erste Tab, der weder im Hauptfenster zu sehen ist noch ein eigenes Fenster hat.
int ersterFreier(int ausser) {
    for (int m = 0; m < fenster::kTabBereiche; ++m) {
        if (m != ausser && indexVon(m) < 0) { return m; }
    }
    return -1;
}

fenster::Rechteck rechteck(const ImVec2& pos, const ImVec2& groesse) {
    return fenster::Rechteck{static_cast<int>(std::lround(pos.x)), static_cast<int>(std::lround(pos.y)), static_cast<int>(std::lround(groesse.x)),
                             static_cast<int>(std::lround(groesse.y))};
}

// Arbeitsbereiche aller Bildschirme und der des Hauptfensters (Ersatz).
std::vector<fenster::Rechteck> bildschirme() {
    std::vector<fenster::Rechteck> r;
    for (const ImGuiPlatformMonitor& m : ImGui::GetPlatformIO().Monitors) { r.push_back(rechteck(m.WorkPos, m.WorkSize)); }
    return r;
}
fenster::Rechteck hauptBildschirm() {
    const auto* v = static_cast<const ImGuiViewportP*>(ImGui::GetMainViewport());
    const ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    const int nr = (v->PlatformMonitor >= 0 && v->PlatformMonitor < pio.Monitors.Size) ? v->PlatformMonitor : 0;
    if (pio.Monitors.Size > 0) { return rechteck(pio.Monitors[nr].WorkPos, pio.Monitors[nr].WorkSize); }
    return rechteck(v->WorkPos, v->WorkSize);
}

void protokoll(const std::string& text) { diag::detail("Tabfenster: " + text); }

// --- Wuensche ausfuehren (am Anfang eines Bildes, nie mitten im Zeichnen) -----

void hauptZeigt(int modus) {
    Stand& s = st();
    g_app->leftMode = modus;
    s.hauptLetzter = modus;
}

void loesenJetzt(const Wunsch& w) {
    Stand& s = st();
    if (!loesbar(w.modus)) { return; }
    const ImGuiViewport* v = ImGui::GetMainViewport();
    const float fs = ImGui::GetFontSize();
    Fenster f;
    f.nr = s.naechsteNr++;
    f.felder.leftMode = w.modus;
    // Etwa so gross wie zwei Drittel des Hauptfensters, mindestens 64 x 36 Zeichen
    const ImVec2 groesse{std::max(fs * 64.0F, std::floor(v->WorkSize.x * 0.66F)), std::max(fs * 36.0F, std::floor(v->WorkSize.y * 0.66F))};
    // Ohne Lage rechts neben der Modusleiste des Hauptfensters: Sie bleibt frei - zum
    // Umschalten und als Ziel beim Zurueckziehen.
    const float links = (s.hauptLeisteBild >= 0) ? s.hauptLeiste.Max.x + fs : v->WorkPos.x + fs * 8.0F;
    const ImVec2 pos = w.mitLage ? ImVec2{w.lage.x - fs * 4.0F, w.lage.y - fs * 0.5F} : ImVec2{links, v->WorkPos.y + fs * 3.0F};
    fenster::TabFenster t;
    t.modus = w.modus;
    t.lage = rechteck(pos, groesse);
    t = fenster::aufBildschirm(t, bildschirme(), hauptBildschirm());
    f.lage = t.lage;
    f.bildschirm = t.bildschirm;
    f.fokusWunsch = true;
    if (w.modus == g_app->leftMode) {
        const int frei = ersterFreier(w.modus);
        if (frei < 0) { return; }
        hauptZeigt(frei);
    }
    s.fenster.push_back(f);
    protokoll(std::string("Tab ") + std::to_string(w.modus) + " in eigenem Fenster " + std::to_string(f.nr) + " bei " +
              std::to_string(f.lage.x) + "," + std::to_string(f.lage.y) + " " + std::to_string(f.lage.b) + "x" + std::to_string(f.lage.h) +
              ", Hauptfenster zeigt " + std::to_string(g_app->leftMode));
}

void zurueckJetzt(int modus) {
    Stand& s = st();
    const int i = indexVon(modus);
    if (i < 0) { return; }
    s.fenster.erase(s.fenster.begin() + i);
    if (s.fokus == i) {
        s.fokus = -1;
    } else if (s.fokus > i) {
        --s.fokus;
    }
    hauptZeigt(modus);
    protokoll(std::string("Tab ") + std::to_string(modus) + " zurueck im Hauptfenster");
}

void zeigenJetzt(int modus) {
    Stand& s = st();
    const int i = indexVon(modus);
    if (i >= 0) {
        Fenster& f = s.fenster[static_cast<std::size_t>(i)];
        f.fokusWunsch = true;
        s.fokus = i;
        return;
    }
    if (modus != g_app->leftMode) { hauptZeigt(modus); }
    s.fokus = -1;
    if (ImGuiWindow* h = ImGui::FindWindowByName("###main"); h != nullptr) { ImGui::FocusWindow(h); }
    // Das Windows-Fenster nach vorn - nur auf einen Klick hin, nie im Selbsttest (kein Fokusstehlen).
    ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    if (!imSelbsttest() && pio.Platform_SetWindowFocus != nullptr) { pio.Platform_SetWindowFocus(ImGui::GetMainViewport()); }
}

// --- Welches Fenster ist gemeint? ---------------------------------------------
//
// Ein Fenster gehoert zu einem Tab, wenn es sein Fenster ist, darin steckt
// (Kindfenster, Klappmenue, angedocktes Panel) oder ein Panel dieses Tabs ist
// (dock1, "###dock.<bereich>.<name>"). -2: gehoert zu keinem (ein Dialog) -
// dann bleibt es beim bisherigen Fenster.
int fensterIndex(const ImGuiWindow* w) {
    const Stand& s = st();
    const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");   // Name "<Datei>###main", Kennung aus "###main"
    for (const ImGuiWindow* x = w; x != nullptr; x = x->ParentWindow) {
        const ImGuiWindow* wurzel = (x->RootWindowDockTree != nullptr) ? x->RootWindowDockTree : x;
        for (const ImGuiWindow* y : {x, wurzel}) {
            if (y == haupt) { return -1; }
            for (std::size_t i = 0; i < s.fenster.size(); ++i) {
                if (s.fenster[i].fensterId != 0 && y->ID == s.fenster[i].fensterId) { return static_cast<int>(i); }
            }
            if (const char* p = std::strstr(y->Name, fenster::kFensterKennung); p != nullptr) {
                if (const dock::Panel* pan = dock::panel(p + std::strlen(fenster::kFensterKennung)); pan != nullptr) {
                    return indexVon(pan->bereich);   // -1 = Hauptfenster
                }
            }
        }
    }
    return -2;
}

void fokusBestimmen() {
    Stand& s = st();
    const ImGuiContext& g = *GImGui;
    if (g.NavWindow != nullptr) {
        const int i = fensterIndex(g.NavWindow);
        if (i != -2) { s.fokus = i; }
    }
    if (s.fokus >= static_cast<int>(s.fenster.size())) { s.fokus = -1; }
}

void uebernehmenJetzt(const std::string& zeile);

// Ein modaler Dialog des Hauptfensters (Speichernfrage, "Exit?") sperrt alle
// anderen Fenster - also kommt das Hauptfenster nach vorn, sonst laege der
// Dialog hinter einem eigenen Fenster und waere nicht zu erreichen. Einmal je
// Dialog: ImGui fuehrt die Reihenfolge der Fenster ueber LastFocusedStampCount
// (FindHoveredViewportFromPlatformWindowStack), Windows ueber den Fokus.
void hauptNachVornBeiDialog() {
    ImGuiContext& g = *GImGui;
    const ImGuiWindow* modal = ImGui::GetTopMostPopupModal();
    auto* haupt = static_cast<ImGuiViewportP*>(ImGui::GetMainViewport());
    if (st().fenster.empty() || modal == nullptr || modal->Viewport != haupt || haupt->LastFocusedStampCount == g.ViewportFocusedStampCount) {
        return;
    }
    haupt->LastFocusedStampCount = ++g.ViewportFocusedStampCount;
    ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    if (!imSelbsttest() && pio.Platform_SetWindowFocus != nullptr) { pio.Platform_SetWindowFocus(haupt); }
    protokoll(std::string("Dialog ") + modal->Name + " im Hauptfenster - Hauptfenster nach vorn");
}

void einstellungenLaden() {
    Stand& s = st();
    s.geladen = true;
    const auto it = g_app->settings.fenster.find(fenster::kTabSchluessel);
    if (it == g_app->settings.fenster.end()) { return; }
    s.gemerkt = it->second;
    uebernehmenJetzt(it->second);
}

void einstellungenMerken() {
    Stand& s = st();
    const std::string z = kodiert();
    if (z == s.gemerkt) { return; }
    s.gemerkt = z;
    if (s.fenster.empty()) {
        g_app->settings.fenster.erase(fenster::kTabSchluessel);
    } else {
        g_app->settings.fenster[fenster::kTabSchluessel] = z;
    }
}

}  // namespace

// --- Abfragen -------------------------------------------------------------------

bool eigenes(int modus) { return g_app != nullptr && indexVon(modus) >= 0; }

bool sichtbar(int modus) { return g_app != nullptr && (hauptModus() == modus || indexVon(modus) >= 0); }

int anzahl() { return static_cast<int>(st().fenster.size()); }

std::vector<int> modi() {
    std::vector<int> m;
    for (int i = 0; i < anzahl(); ++i) { m.push_back(modusVon(i)); }
    return m;
}

bool loesbar(int modus) {
    if (g_app == nullptr || modus < 0 || modus >= fenster::kTabBereiche || eigenes(modus)) { return false; }
    // Das Hauptfenster behaelt einen: es muss danach noch einen anderen zeigen koennen.
    return modus != hauptModus() || ersterFreier(modus) >= 0;
}

int fokusModus() {
    if (g_app == nullptr) { return 0; }
    const Stand& s = st();
    if (s.fenster.empty()) { return hauptModus(); }
    return modusVon(s.fokus);
}

bool fensterHatFokus() {
    const Stand& s = st();
    return s.fenster.empty() || s.zeichnet == s.fokus;
}

int modusUnterMaus() {
    if (g_app == nullptr) { return 0; }
    const Stand& s = st();
    if (s.fenster.empty()) { return hauptModus(); }
    const ImGuiContext& g = *GImGui;
    const int i = (g.HoveredWindow != nullptr) ? fensterIndex(g.HoveredWindow) : -1;
    return modusVon(i == -2 ? s.fokus : i);
}

ImGuiWindow* fenster(int modus) {
    const int i = indexVon(modus);
    if (i < 0) { return nullptr; }
    return ImGui::FindWindowByID(st().fenster[static_cast<std::size_t>(i)].fensterId);
}

// --- Wuensche ---------------------------------------------------------------------

bool loesen(int modus) {
    if (!loesbar(modus)) { return false; }
    st().wuensche.push_back(Wunsch{Art::Loesen, modus, false, ImVec2{}});
    return true;
}

bool loesenBei(int modus, const ImVec2& lage) {
    if (!loesbar(modus)) { return false; }
    st().wuensche.push_back(Wunsch{Art::Loesen, modus, true, lage});
    return true;
}

void zurueck(int modus) { st().wuensche.push_back(Wunsch{Art::Zurueck, modus, false, ImVec2{}}); }

void alleZurueck() { st().wuensche.push_back(Wunsch{Art::AlleZurueck, 0, false, ImVec2{}}); }

void zeigen(int modus) { st().wuensche.push_back(Wunsch{Art::Zeigen, modus, false, ImVec2{}}); }

// --- Einstellungen ------------------------------------------------------------------

std::string kodiert() {
    std::vector<fenster::TabFenster> liste;
    for (int i = 0; i < anzahl(); ++i) {
        const Fenster& f = st().fenster[static_cast<std::size_t>(i)];
        liste.push_back(fenster::TabFenster{modusVon(i), f.lage, f.bildschirm});
    }
    return fenster::tabFensterKodieren(liste);
}

void uebernehmen(const std::string& zeile) {
    st().laden = zeile;
    st().ladenOffen = true;
}

namespace {
void uebernehmenJetzt(const std::string& zeile) {
    std::vector<fenster::TabFenster> liste;
    if (!fenster::tabFensterDekodieren(zeile, liste)) {
        protokoll("Liste aus den Einstellungen unbrauchbar: \"" + zeile + "\"");
        return;
    }
    Stand& s = st();
    // Die bisherigen Fenster zu, dann die Liste - jeder Tab an seine Lage.
    for (const int m : modi()) { zurueckJetzt(m); }
    const std::vector<fenster::Rechteck> alle = bildschirme();
    const fenster::Rechteck ersatz = hauptBildschirm();
    for (const fenster::TabFenster& t : liste) {
        const fenster::TabFenster n = fenster::aufBildschirm(t, alle, ersatz);
        if (t.modus == g_app->leftMode) {
            const int frei = ersterFreier(t.modus);
            if (frei < 0) { continue; }
            hauptZeigt(frei);
        }
        Fenster f;
        f.nr = s.naechsteNr++;
        f.felder.leftMode = t.modus;
        f.lage = n.lage;
        f.bildschirm = n.bildschirm;
        s.fenster.push_back(f);
        protokoll(std::string("Tab ") + std::to_string(t.modus) + " aus den Einstellungen bei " + std::to_string(n.lage.x) + "," +
                  std::to_string(n.lage.y) + " " + std::to_string(n.lage.b) + "x" + std::to_string(n.lage.h) +
                  (n.lage == t.lage ? "" : " (Bildschirm geaendert)"));
    }
}
}  // namespace

// --- Haken fuer app.cpp ---------------------------------------------------------------

void bildAnfang() {
    if (g_app == nullptr) { return; }
    Stand& s = st();
    if (!s.geladen) { einstellungenLaden(); }
    // Ein Wechsel aus dem Programm (Selbsttest, "Mission oeffnen", Tour) zu einem
    // Tab in eigenem Fenster: das Hauptfenster bleibt, das Fenster kommt nach vorn.
    if (indexVon(g_app->leftMode) >= 0) {
        s.wuensche.push_back(Wunsch{Art::Zeigen, g_app->leftMode, false, ImVec2{}});
        g_app->leftMode = s.hauptLetzter;
    }
    if (s.ladenOffen) {
        s.ladenOffen = false;
        uebernehmenJetzt(s.laden);
    }
    std::vector<Wunsch> w;
    w.swap(s.wuensche);
    for (const Wunsch& x : w) {
        switch (x.art) {
            case Art::Loesen: loesenJetzt(x); break;
            case Art::Zurueck: zurueckJetzt(x.modus); break;
            case Art::AlleZurueck:
                for (const int m : modi()) { zurueckJetzt(m); }
                break;
            case Art::Zeigen: zeigenJetzt(x.modus); break;
        }
    }
    if (indexVon(g_app->leftMode) < 0) { s.hauptLetzter = g_app->leftMode; }
    hauptNachVornBeiDialog();
    fokusBestimmen();
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseReleased(ImGuiMouseButton_Left)) { s.ziehtModus = -1; }
    einstellungenMerken();
}

int fensterZahl() { return anzahl(); }

void vorbereiten(int i) {
    Stand& s = st();
    Fenster& f = s.fenster[static_cast<std::size_t>(i)];
    if (f.lageSetzen) {
        ImGui::SetNextWindowPos(ImVec2{static_cast<float>(f.lage.x), static_cast<float>(f.lage.y)});
        ImGui::SetNextWindowSize(ImVec2{static_cast<float>(f.lage.b), static_cast<float>(f.lage.h)});
        f.lageSetzen = false;
    }
    if (f.fokusWunsch) {
        f.fokusWunsch = false;
        ImGui::SetNextWindowFocus();
        s.fokus = i;
        ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
        if (ImGuiWindow* w = ImGui::FindWindowByID(f.fensterId);
            !imSelbsttest() && w != nullptr && w->Viewport != nullptr && pio.Platform_SetWindowFocus != nullptr && w->Viewport != ImGui::GetMainViewport()) {
            pio.Platform_SetWindowFocus(w->Viewport);
        }
    }
    // Ein eigenes Windows-Fenster, auch ueber dem Hauptfenster (NoAutoMerge),
    // ohne Besitzer (ParentViewportId 0): es liegt nicht immer vor dem
    // Hauptfenster und hat einen eigenen Eintrag in der Taskleiste - zwei
    // gleichrangige Fenster. Im Selbsttest erscheint es, ohne den Fokus zu
    // nehmen (verborgen zeigt es backend_win32.cpp).
    ImGuiWindowClass k;
    k.ClassId = ImHashStr("bhed.tabfenster");
    k.ParentViewportId = 0;
    k.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoAutoMerge | (imSelbsttest() ? ImGuiViewportFlags_NoFocusOnAppearing : 0);
    k.DockingAllowUnclassed = false;
    ImGui::SetNextWindowClass(&k);
    f.tabVorher = f.felder.leftMode;
    tauschen(f.felder);
    s.zeichnet = i;
}

bool begin(int i) {
    Fenster& f = st().fenster[static_cast<std::size_t>(i)];
    f.offen = true;
    // Wie das Hauptfenster: eine Menueleiste, kein Rollen, nie vor die
    // schwebenden Fenster (Panels, Dialoge) - dafuer ist es selbst eines.
    return ImGui::Begin(fensterName(f.nr, g_app->leftMode).c_str(), &f.offen,
                        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar |
                            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
}

void ende(int i) {
    Stand& s = st();
    ImGuiContext& g = *GImGui;
    Fenster& f = s.fenster[static_cast<std::size_t>(i)];
    ImGuiWindow* w = g.CurrentWindow;
    f.fensterId = w->ID;
    f.lage = rechteck(w->Pos, w->Size);
    if (w->Viewport != nullptr && w->Viewport->PlatformMonitor >= 0 && w->Viewport->PlatformMonitor < g.PlatformIO.Monitors.Size) {
        const ImGuiPlatformMonitor& m = g.PlatformIO.Monitors[w->Viewport->PlatformMonitor];
        f.bildschirm = rechteck(m.WorkPos, m.WorkSize);
    }
    // --- Zurueckziehen: die Titelleiste auf die Modusleiste des Hauptfensters --
    const bool bewegt = g.MovingWindow != nullptr && g.MovingWindow->RootWindow == w;
    const bool leisteDa = s.hauptLeisteBild + 1 >= ImGui::GetFrameCount();
    if (bewegt && leisteDa) {
        f.ueberLeiste = s.hauptLeiste.Contains(g.IO.MousePos);
        ImDrawList* dl = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
        const ImVec4 farbe = ImGui::GetStyleColorVec4(ImGuiCol_DragDropTarget);
        const ImRect& r = s.hauptLeiste;
        dl->AddRectFilled(r.Min, r.Max, ImGui::GetColorU32(ImVec4{farbe.x, farbe.y, farbe.z, f.ueberLeiste ? 0.35F : 0.12F}));
        dl->AddRect(r.Min, r.Max, ImGui::GetColorU32(farbe), 0.0F, 0, f.ueberLeiste ? 3.0F : 1.0F);
    } else if (f.wirdBewegt && f.ueberLeiste && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        s.wuensche.push_back(Wunsch{Art::Zurueck, g_app->leftMode, false, ImVec2{}});
        f.ueberLeiste = false;
    } else if (!bewegt) {
        f.ueberLeiste = false;
    }
    f.wirdBewegt = bewegt;
    ImGui::End();
    tauschen(f.felder);
    s.zeichnet = -1;
    // Ein Wechsel, den der Inhalt angestossen hat ("Mission oeffnen", ein Knopf
    // der Modusleiste): das Fenster bleibt bei seinem Tab, der andere kommt nach vorn.
    if (f.felder.leftMode != f.tabVorher) {
        s.wuensche.push_back(Wunsch{Art::Zeigen, f.felder.leftMode, false, ImVec2{}});
        f.felder.leftMode = f.tabVorher;
    }
    if (!f.offen) { s.wuensche.push_back(Wunsch{Art::Zurueck, f.felder.leftMode, false, ImVec2{}}); }
}

void bildEnde() {
    Stand& s = st();
    s.zeichnet = -1;
    einstellungenMerken();
}

bool zeichnetEigenes() { return st().zeichnet >= 0; }

void leisteGemessen(const ImVec2& a, const ImVec2& b) {
    Stand& s = st();
    if (s.zeichnet >= 0) { return; }
    s.hauptLeiste = ImRect{a, b};
    s.hauptLeisteBild = ImGui::GetFrameCount();
}

void leisteKnopf(int modus) {
    if (g_app == nullptr) { return; }
    Stand& s = st();
    // Rechtsklick: in ein eigenes Fenster bzw. zurueck
    if (ImGui::BeginPopupContextItem()) {
        if (eigenes(modus)) {
            if (ImGui::MenuItem(tr(Str::TfZurueck))) { zurueck(modus); }
        } else {
            if (ImGui::MenuItem(tr(Str::TfLoesen), nullptr, false, loesbar(modus))) { (void)loesen(modus); }
            if (!loesbar(modus) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) { ImGui::SetTooltip("%s", tr(Str::TfLetzter)); }
        }
        ImGui::EndPopup();
    }
    // Herausziehen: den Knopf aus dem Fenster ziehen und draussen loslassen
    const ImGuiViewport* v = ImGui::GetWindowViewport();
    const ImRect vp{v->Pos, ImVec2{v->Pos.x + v->Size.x, v->Pos.y + v->Size.y}};
    const ImVec2 maus = ImGui::GetIO().MousePos;
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, ImGui::GetFontSize() * 0.5F) && !eigenes(modus)) {
        s.ziehtModus = modus;
        if (!vp.Contains(maus)) { ImGui::SetTooltip("%s", tr(loesbar(modus) ? Str::TfZiehHint : Str::TfLetzter)); }
    }
    if (s.ziehtModus == modus && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        s.ziehtModus = -1;
        if (!vp.Contains(maus)) { (void)loesenBei(modus, maus); }
    }
}

void imFokus(void (*fn)()) {
    Stand& s = st();
    if (s.fenster.empty() || s.fokus < 0 || s.fokus >= static_cast<int>(s.fenster.size())) {
        fn();
        return;
    }
    const int i = s.fokus;
    Fenster& f = s.fenster[static_cast<std::size_t>(i)];
    f.tabVorher = f.felder.leftMode;
    tauschen(f.felder);
    s.zeichnet = i;
    fn();
    tauschen(f.felder);
    s.zeichnet = -1;
    if (f.felder.leftMode != f.tabVorher) {
        s.wuensche.push_back(Wunsch{Art::Zeigen, f.felder.leftMode, false, ImVec2{}});
        f.felder.leftMode = f.tabVorher;
    }
}

}  // namespace bhed::gui::tabfenster
