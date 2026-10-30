// --- dock1: Fenster frei anordnen, mehrere Bildschirme (BHED_EDITORTEST=dock1) ---
//
// Prueft im laufenden Programm (fenster_dock.h):
//   - Standardanordnung: Spalten, Band und Felder unveraendert, solange nichts
//     verschoben ist - und nach "Anordnung zuruecksetzen" wieder genau so.
//   - Panel herausnehmen (Menue "Fenster"): eigenes ImGui-Fenster, aus dem
//     Hauptfenster gezogen ein eigenes Windows-Fenster, das zeichnet (das
//     fertige Bild seiner Swapchain gelesen); sein Platz geht an die Nachbarn.
//   - Zusammensetzen zweier Panels (Ziehen auf das andere Fenster), Andocken
//     ueber den Schliessknopf, das Menue und Ziehen auf den alten Platz.
//   - Anordnung speichern unter / laden / loeschen / zuruecksetzen, und ueber
//     die Einstellungsdatei (BHED_EDITORTEST=dock1neustart im selben Ordner:
//     der zweite Start findet die Anordnung des ersten).
//   - Fehlerfaelle: Bildschirm abgezogen, Bildschirm zu klein, anderer
//     Massstab - nichts geht verloren, die Fenster kommen zurueck.
//   - Map Editor: Kamera herausgenommen zeichnet in ihrem Fenster, Klick
//     macht sie zum aktiven Feld, Tasten gelten dort, Freelook in Bildschirm-
//     punkten; Menu Editor und Events.
//   - Bildzeit mit 1 und 3 Fenstern (Viewports).
// Das Hauptfenster wird dafuer verkleinert und verschoben (ImGuis Ursprung ist
// dann nicht mehr 0,0) und am Ende zurueckgesetzt.
namespace dock1 {

// Vorgetaeuschte Bildschirme und Massstab (vor ImGui::NewFrame, dock1VorBild).
struct Sim {
    bool bildschirme = false;
    std::vector<ImGuiPlatformMonitor> monitore;
    float dpi = 0.0F;               // > 0: Massstab der eigenen Fenster
    float (*dpiOriginal)(ImGuiViewport*) = nullptr;
};
Sim& sim() {
    static Sim s;
    return s;
}

float dpiSim(ImGuiViewport* v) {
    if (v != ImGui::GetMainViewport() && sim().dpi > 0.0F) { return sim().dpi; }
    return sim().dpiOriginal != nullptr ? sim().dpiOriginal(v) : 1.0F;
}

}  // namespace dock1

void dock1VorBild() {
    dock1::Sim& s = dock1::sim();
    ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    if (s.bildschirme) {
        pio.Monitors.resize(0);
        for (const ImGuiPlatformMonitor& m : s.monitore) { pio.Monitors.push_back(m); }
    }
    if (s.dpi > 0.0F && pio.Platform_GetWindowDpiScale != dock1::dpiSim) {
        s.dpiOriginal = pio.Platform_GetWindowDpiScale;
        pio.Platform_GetWindowDpiScale = dock1::dpiSim;
    } else if (s.dpi <= 0.0F && s.dpiOriginal != nullptr) {
        pio.Platform_GetWindowDpiScale = s.dpiOriginal;
        s.dpiOriginal = nullptr;
    }
}

namespace dock1 {

// Das Fenster des Panels: sein Name endet auf "###dock.<id>" (Kindfenster
// heissen "<Name>/<Kind>"; angedockt traegt es ImGuiWindowFlags_ChildWindow).
ImGuiWindow* panelFenster(const char* panel) {
    ImGuiContext& g = *GImGui;
    const std::string k = std::string(fenster::kFensterKennung) + panel;
    for (ImGuiWindow* w : g.Windows) {
        const std::string n = w->Name;
        if (n.size() >= k.size() && n.compare(n.size() - k.size(), k.size(), k) == 0) { return w; }
    }
    return nullptr;
}

// Gezeichnet (Begin) im letzten Bild - auch als verdeckter Reiter eines
// Dock-Knotens.
bool aktiv(const char* panel) {
    const ImGuiWindow* w = panelFenster(panel);
    return w != nullptr && w->WasActive;
}

// Hat das Fenster ein eigenes Windows-Fenster (nicht das Hauptfenster)?
bool eigenesOsFenster(const char* panel) {
    const ImGuiWindow* w = panelFenster(panel);
    if (w == nullptr || w->Viewport == nullptr || w->Viewport == ImGui::GetMainViewport()) { return false; }
    return w->Viewport->PlatformWindowCreated && platform::fensterSichtbar(w->Viewport->PlatformHandle);
}

std::string rechteck(const ImRect& r) {
    char t[96];
    std::snprintf(t, sizeof(t), "%.0f,%.0f %.0fx%.0f", static_cast<double>(r.Min.x), static_cast<double>(r.Min.y),
                  static_cast<double>(r.GetWidth()), static_cast<double>(r.GetHeight()));
    return t;
}

}  // namespace dock1

namespace dock1 {

// Die Anordnung des Hauptfensters in Zahlen - Spalten, Band, Tabelle.
struct Masse {
    int modus = -1;
    float spalten[3] = {0, 0, 0};
    float oben = 0.0F;
    float unten = 0.0F;
    int toolH = -1;
    int frameW = -1;
    int frameH = -1;
    float ueberhang = 0.0F;
};

Masse messen() {
    Masse m;
    m.modus = g_app->leftMode;
    for (int k = 0; k < 3; ++k) { m.spalten[k] = g_app->spaltenB[k]; }
    m.oben = g_app->tabelleOben - ImGui::GetMainViewport()->Pos.y;
    m.unten = g_app->tabelleUnten - ImGui::GetMainViewport()->Pos.y;
    m.toolH = g_app->lastLayoutLog.toolH;
    m.frameW = g_app->lastLayoutLog.frameW;
    m.frameH = g_app->lastLayoutLog.frameH;
    m.ueberhang = g_app->spaltenUeberhang;
    return m;
}

std::string text(const Masse& m) {
    char t[200];
    std::snprintf(t, sizeof(t), "Modus %d: Spalten %.0f/%.0f/%.0f, Tabelle %.0f..%.0f, Band %d, Fenster %dx%d, Ueberhang %.0f", m.modus,
                  static_cast<double>(m.spalten[0]), static_cast<double>(m.spalten[1]), static_cast<double>(m.spalten[2]),
                  static_cast<double>(m.oben), static_cast<double>(m.unten), m.toolH, m.frameW, m.frameH, static_cast<double>(m.ueberhang));
    return t;
}

bool gleich(const Masse& a, const Masse& b) {
    for (int k = 0; k < 3; ++k) {
        if (std::fabs(a.spalten[k] - b.spalten[k]) > 0.5F) { return false; }
    }
    return a.modus == b.modus && std::fabs(a.oben - b.oben) <= 0.5F && std::fabs(a.unten - b.unten) <= 0.5F && a.toolH == b.toolH &&
           a.frameW == b.frameW && a.frameH == b.frameH;
}

struct Lauf {
    std::array<Masse, 3> vorher;              // Events, Mission, Model vor allem Verschieben
    std::vector<ImGuiPlatformMonitor> echt;  // die echten Bildschirme
    int hx = 0, hy = 0, hb = 0, hh = 0;      // Lage des Hauptfensters vor dem Test
    ImRect zeitRect;                          // Zeitleistenfenster beim Speichern
    ImRect flussRect;
    double ms1 = 0.0, ms3 = 0.0, dt1 = 0.0, dt3 = 0.0;
    float yaw = 0.0F;
    karte::Vektor3f kamPos;
    float zeitBreite = 0.0F;
    int felderTexturenH = 0;
    int hellHeim = -1;
};

ImRect rect(const char* panel) {
    const ImGuiWindow* w = panelFenster(panel);
    return w != nullptr ? w->Rect() : ImRect{};
}

const ImGuiPlatformMonitor& hauptMonitor() {
    const ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
    return pio.Monitors[std::clamp(static_cast<int>(static_cast<ImGuiViewportP*>(ImGui::GetMainViewport())->PlatformMonitor), 0,
                                   std::max(0, pio.Monitors.Size - 1))];
}

// Ein Panelfenster an eine Stelle (Bildschirmpunkte) und auf eine Groesse.
void setze(const char* panel, ImVec2 pos, ImVec2 gr) {
    ImGuiWindow* w = panelFenster(panel);
    if (w == nullptr) { return; }
    ImGuiWindow* wurzel = w->RootWindowDockTree;
    ImGui::SetWindowPos(wurzel, pos, ImGuiCond_Always);
    ImGui::SetWindowSize(wurzel, gr, ImGuiCond_Always);
    if (wurzel->DockNodeAsHost != nullptr) {
        wurzel->DockNodeAsHost->Pos = pos;
        wurzel->DockNodeAsHost->Size = gr;
    }
}

// Rechts neben dem (verkleinerten) Hauptfenster, auf dem Bildschirm.
ImVec2 nebenHaupt(float dy) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    return ImVec2{vp->Pos.x + vp->Size.x + 30.0F, vp->Pos.y + dy};
}

// Ein Klick auf den Schliessknopf eines Panelfensters.
Schritt schliessknopf(const char* panel) {
    auto ziel = std::make_shared<ImVec2>();
    const std::string p = panel;
    return {"Klick Schliessknopf von " + p, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    const ImGuiWindow* w = panelFenster(p.c_str());
                    const Element* e = (w != nullptr) ? findeKennung(ImHashStr("#CLOSE", 0, w->ID)) : nullptr;
                    if (e == nullptr) {
                        meldeFehler("Schliessknopf von " + p + " nicht gefunden");
                        return true;
                    }
                    *ziel = e->rect.GetCenter();
                    setzeMaus(*ziel);
                    return false;
                }
                if (b == 1) { io.AddMouseButtonEvent(0, true); return false; }
                if (b == 2) { io.AddMouseButtonEvent(0, false); return false; }
                return b >= 5;
            }};
}

// Die Mitte der Titelleiste eines Panelfensters.
ImVec2 titel(const char* panel) {
    const ImGuiWindow* w = panelFenster(panel);
    if (w == nullptr) { return ImVec2{}; }
    return ImVec2{w->Pos.x + std::min(w->Size.x * 0.5F, ImGui::GetFontSize() * 6.0F), w->Pos.y + w->TitleBarHeight * 0.5F};
}

// Das fertige Bild des eigenen Fensters lesen (Swapchain, backend_win32.cpp).
Schritt bildLesen(const std::string& was, const char* panel, int mindestFarben) {
    const std::string p = panel;
    return {"Bild von " + was, [=](int b) {
                const ImGuiWindow* w = panelFenster(p.c_str());
                if (b == 0) {
                    if (w == nullptr || w->Viewport == nullptr) {
                        meldeFehler(was + ": kein Fenster");
                        return true;
                    }
                    render::viewportBildAnfordern(w->Viewport->ID);
                    return false;
                }
                if (b < 4) { return false; }
                const render::ViewportBild bild = render::viewportBild();
                char t[220];
                std::snprintf(t, sizeof(t), "%s: eigenes Fenster %dx%d (ImGui %.0fx%.0f), %d Farben", was.c_str(), bild.breite, bild.hoehe,
                              w != nullptr ? static_cast<double>(w->Viewport->Size.x) : 0.0,
                              w != nullptr ? static_cast<double>(w->Viewport->Size.y) : 0.0, bild.farben);
                const bool ok = w != nullptr && bild.da && bild.farben >= mindestFarben &&
                                std::abs(static_cast<float>(bild.breite) - w->Viewport->Size.x) < 1.5F &&
                                std::abs(static_cast<float>(bild.hoehe) - w->Viewport->Size.y) < 1.5F;
                pruefeWahr(ok, std::string(t) + " - zeichnet");
                return true;
            }};
}

// Mittlere Bildzeit und Zeit fuer die eigenen Fenster ueber `n` Bilder.
Schritt bildzeit(const std::string& was, int n, double* dt, double* ms) {
    auto summe = std::make_shared<std::pair<double, double>>(0.0, 0.0);
    return {"Bildzeit " + was, [=](int b) {
                if (b < 10) { return false; }   // einschwingen
                summe->first += static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0;
                summe->second += render::plattformMs();
                if (b < 10 + n) { return false; }
                *dt = summe->first / n;
                *ms = summe->second / n;
                return true;
            }};
}

}  // namespace dock1

// --- BHED_EDITORTEST=dock1 mit BHED_DOCK_NUR=fotos: die Bilder der Handbuchseite ---
// (BHED_FOTOS, BHED_SPRACHE wie beim Handbuch; danach tools/handbuch/bilder.py).
namespace dock1 {

// Die Maus an einer Stelle druecken und dorthin ziehen, gedrueckt lassen.
Schritt ziehenUndHalten(const std::string& was, std::function<ImVec2()> von, std::function<ImVec2()> nach) {
    auto a = std::make_shared<ImVec2>();
    auto z = std::make_shared<ImVec2>();
    return {was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    *a = von();
                    *z = nach();
                    setzeMaus(*a);
                    return false;
                }
                if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                if (b >= 3 && b <= 23) {
                    const float f = static_cast<float>(b - 3) / 20.0F;
                    setzeMaus(ImVec2{a->x + (z->x - a->x) * f, a->y + (z->y - a->y) * f});
                    return false;
                }
                return b >= 28;
            }};
}

Schritt loslassen(const std::string& was) {
    return {was, [](int b) {
                if (b == 0) { ImGui::GetIO().AddMouseButtonEvent(0, false); }
                return b >= 6;
            }};
}

ImRect hauptRect() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    return ImRect{vp->Pos, ImVec2{vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y}};
}

}  // namespace dock1

std::vector<Schritt> dock1FotoSchritte() {
    if (const char* sp = std::getenv("BHED_SPRACHE")) {
        if (const i18n::LanguageInfo* l = i18n::findLanguage(sp)) { i18n::setLanguage(l->language); }
    }
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("dock1 Fotos: Mission, Standardanordnung", [] {
        for (int b = 0; b < dock::kBereiche; ++b) { dock::zuruecksetzen(b); }
        g_app->leftMode = 1;
    }));
    add(stabil("dock1 Fotos: Mission", 30));
    add(klick(tr(Str::DockMenu), "###main"));
    add(pause(0.4));
    add(hb::fotoTeil("h_dock_menue", [] {
        const ImRect m = hb::fensterRect("##Menu_00");
        const ImRect h = dock1::hauptRect();
        return ImRect{h.Min.x, h.Min.y, std::min(h.Max.x, m.Max.x + 40.0F), std::min(h.Max.y, m.Max.y + 20.0F)};
    }));
    add(taste(ImGuiKey_Escape, "Escape"));
    add(tu("dock1 Fotos: Zeitleiste und Script Flow in eigene Fenster", [] {
        dock::herausnehmen("mission.zeitleiste");
        dock::herausnehmen("mission.fluss");
    }));
    add(warteBis("dock1 Fotos: beide Fenster da", [] { return dock1::aktiv("mission.zeitleiste") && dock1::aktiv("mission.fluss"); }, 120));
    add(tu("dock1 Fotos: beide ins rechte untere Viertel", [] {
        const ImRect h = dock1::hauptRect();
        dock1::setze("mission.zeitleiste", ImVec2{h.Min.x + h.GetWidth() * 0.42F, h.Min.y + h.GetHeight() * 0.42F},
                     ImVec2{h.GetWidth() * 0.54F, h.GetHeight() * 0.52F});
        dock1::setze("mission.fluss", ImVec2{h.Min.x + h.GetWidth() * 0.62F, h.Min.y + h.GetHeight() * 0.12F},
                     ImVec2{h.GetWidth() * 0.34F, h.GetHeight() * 0.25F});
    }));
    add(stabil("dock1 Fotos: zwei schwebende Fenster", 20));
    add(dock1::ziehenUndHalten("dock1 Fotos: Script Flow auf die Zeitleiste", [] { return dock1::titel("mission.fluss"); },
                               [] { return dock1::rect("mission.zeitleiste").GetCenter(); }));
    add(dock1::loslassen("dock1 Fotos: loslassen"));
    add(stabil("dock1 Fotos: zusammengesetzt", 20));
    add(hb::fotoTeil("h_dock_fenster", [] { return dock1::hauptRect(); }));
    add(tu("dock1 Fotos: Ebenenliste in ein eigenes Fenster", [] { dock::herausnehmen("mission.ebenen"); }));
    add(warteBis("dock1 Fotos: Ebenenliste da", [] { return dock1::aktiv("mission.ebenen"); }, 60));
    add(tu("dock1 Fotos: Ebenenliste in die Mitte", [] {
        const ImRect h = dock1::hauptRect();
        dock1::setze("mission.ebenen", ImVec2{h.Min.x + h.GetWidth() * 0.30F, h.Min.y + h.GetHeight() * 0.20F},
                     ImVec2{h.GetWidth() * 0.2F, h.GetHeight() * 0.45F});
    }));
    add(stabil("dock1 Fotos: Ebenenliste schwebt", 10));
    add(dock1::ziehenUndHalten("dock1 Fotos: Ebenenliste zurueck ziehen (gehalten)", [] { return dock1::titel("mission.ebenen"); },
                               [] {
                                   ImVec2 a;
                                   ImVec2 b;
                                   dock::zielPlatz("mission.ebenen", a, b);
                                   return ImVec2{(a.x + b.x) * 0.5F, (a.y + b.y) * 0.5F};
                               }));
    add(hb::fotoTeil("h_dock_zurueck", [] { return dock1::hauptRect(); }));
    add(dock1::loslassen("dock1 Fotos: loslassen - angedockt"));
    add(warteBis("dock1 Fotos: Ebenenliste angedockt", [] { return dock::zuhause("mission.ebenen"); }, 60));
    add(tu("dock1 Fotos: zuruecksetzen", [] { dock::zuruecksetzen(1); }));
    add(stabil("dock1 Fotos: Ende", 10));
    return s;
}

std::vector<Schritt> dock1Schritte() {
    namespace ke = karteneditor;
    if (const char* nur = std::getenv("BHED_DOCK_NUR"); nur != nullptr && std::strcmp(nur, "fotos") == 0) { return dock1FotoSchritte(); }
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto L = std::make_shared<dock1::Lauf>();
    const auto menueKlick = [&](const std::string& eintrag) {
        add(klick(tr(Str::DockMenu), "###main"));
        add(klick(eintrag));
        add(pause(0.2));
    };
    const auto fensterEintrag = [](Str t, const char* id) { return std::string(tr(t)) + "##dockfenster." + id; };
    const auto zeigenEintrag = [](Str t, const char* id) { return std::string(tr(t)) + "##dockzeigen." + id; };

    // --- 1. Standardanordnung: messen, solange nichts verschoben ist ------------
    add(tu("dock1: alle Panels an ihrem Platz, Events", [] {
        for (int b = 0; b < dock::kBereiche; ++b) { dock::zuruecksetzen(b); }
        g_app->leftMode = 0;
    }));
    // Einmal durch alle drei Modi: die Spaltenraender (spaltenUeberhang) stehen
    // erst danach fest - ab Start gilt der Anfangswert 40, gemessen 39, und
    // erst eine Aenderung um zwei Punkte wird uebernommen (app.cpp).
    for (int m : {1, 2, 0}) {
        add(tu("dock1: Modus " + std::to_string(m) + " (einmal durch)", [m] { g_app->leftMode = m; }));
        add(stabil("dock1: Modus " + std::to_string(m) + " eingeschwungen", 20));
    }
    for (int m = 0; m < 3; ++m) {
        add(tu("dock1: Modus " + std::to_string(m), [m] { g_app->leftMode = m; }));
        add(stabil("dock1: Standardanordnung Modus " + std::to_string(m), 30));
        add(tu("dock1: Masse merken", [L, m] {
            L->vorher[static_cast<std::size_t>(m)] = dock1::messen();
            diag::detail("Knopftest: dock1 Standard " + dock1::text(L->vorher[static_cast<std::size_t>(m)]));
        }));
    }
    add(pruefe("dock1: Standardanordnung ohne eigenes Fenster", [] { return ImGui::GetPlatformIO().Viewports.Size == 1; }));

    // --- 2. Hauptfenster verkleinern und verschieben (Ursprung nicht 0,0) --------
    add(tu("dock1: Hauptfenster verkleinern, verschieben; echte Bildschirme merken", [L] {
        void* h = ImGui::GetMainViewport()->PlatformHandle;
        (void)platform::fensterLage(h, L->hx, L->hy, L->hb, L->hh);
        L->echt.clear();
        for (const ImGuiPlatformMonitor& m : ImGui::GetPlatformIO().Monitors) { L->echt.push_back(m); }
        const ImGuiPlatformMonitor& m = dock1::hauptMonitor();
        (void)platform::fensterLageSetzen(h, static_cast<int>(m.WorkPos.x) + 60, static_cast<int>(m.WorkPos.y) + 40,
                                          static_cast<int>(m.WorkSize.x * 0.6F), static_cast<int>(m.WorkSize.y * 0.75F));
        g_app->leftMode = 1;
    }));
    add(stabil("dock1: Hauptfenster verschoben", 20));
    add(pruefe("dock1: Ursprung des Hauptfensters nicht 0,0", [] { return ImGui::GetMainViewport()->Pos.x > 1.0F; }));

    // --- 3. Mission: Zeitleiste in eigenem Fenster ---------------------------------
    menueKlick(fensterEintrag(Str::DockPanelTimeline, "mission.zeitleiste"));
    add(warteBis("dock1: Zeitleiste in eigenem Fenster", [] { return dock1::aktiv("mission.zeitleiste"); }, 120));
    add(pruefe("dock1: Zeitleiste draussen - das Band im Hauptfenster ist weg (toolH 0)",
               [] { return dock::draussen("mission.zeitleiste") && g_app->lastLayoutLog.toolH == 0; }));
    add(pruefe("dock1: Abspielknopf der Zeitleiste steht im eigenen Fenster", [] {
        for (const Element& e : g_elemente) {
            if (e.fenster.find("###dock.mission.zeitleiste") != std::string::npos) { return true; }
        }
        return false;
    }));
    add(tu("dock1: Zeitleiste neben das Hauptfenster ziehen (eigenes Windows-Fenster)", [] {
        const ImGuiPlatformMonitor& m = dock1::hauptMonitor();
        dock1::setze("mission.zeitleiste", dock1::nebenHaupt(0.0F), ImVec2{m.WorkSize.x * 0.3F, m.WorkSize.y * 0.3F});
    }));
    add(warteBis("dock1: eigenes Windows-Fenster", [] { return dock1::eigenesOsFenster("mission.zeitleiste"); }, 120));
    add(dock1::bildLesen("Zeitleiste", "mission.zeitleiste", 6));
    add(pruefe("dock1: ein zweiter Viewport", [] { return ImGui::GetPlatformIO().Viewports.Size >= 2; }));

    // --- 4. Script Flow dazu, auf die Zeitleiste gezogen: zusammengesetzt ----------
    menueKlick(fensterEintrag(Str::GroupScriptFlow, "mission.fluss"));
    add(warteBis("dock1: Script Flow in eigenem Fenster", [] { return dock1::aktiv("mission.fluss"); }, 120));
    add(pruefe("dock1: Script Flow draussen - die Missionsansicht nimmt die Breite (freie Spalten)", [] {
        return dock::draussen("mission.fluss") && g_app->leftMode == 1;
    }));
    add(tu("dock1: Script Flow neben das Hauptfenster", [] {
        const ImGuiPlatformMonitor& m = dock1::hauptMonitor();
        dock1::setze("mission.fluss", dock1::nebenHaupt(m.WorkSize.y * 0.4F), ImVec2{m.WorkSize.x * 0.3F, m.WorkSize.y * 0.3F});
    }));
    add(warteBis("dock1: Script Flow eigenes Windows-Fenster", [] { return dock1::eigenesOsFenster("mission.fluss"); }, 120));
    add(dock1::bildzeit("mit 3 Viewports", 120, &L->dt3, &L->ms3));
    add({"dock1: Script Flow auf die Mitte der Zeitleiste ziehen", [](int b) {
             ImGuiIO& io = ImGui::GetIO();
             static ImVec2 von;
             static ImVec2 nach;
             if (b == 0) {
                 von = dock1::titel("mission.fluss");
                 nach = dock1::rect("mission.zeitleiste").GetCenter();
                 setzeMaus(von);
                 return false;
             }
             if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
             if (b >= 3 && b <= 23) {
                 const float f = static_cast<float>(b - 3) / 20.0F;
                 setzeMaus(ImVec2{von.x + (nach.x - von.x) * f, von.y + (nach.y - von.y) * f});
                 return false;
             }
             if (b == 28) { io.AddMouseButtonEvent(0, false); return false; }
             return b >= 34;
         }});
    add(pruefe("dock1: Script Flow und Zeitleiste in einem Fenster (Dock-Knoten)", [] {
        const ImGuiWindow* f = dock1::panelFenster("mission.fluss");
        const ImGuiWindow* z = dock1::panelFenster("mission.zeitleiste");
        const bool ok = f != nullptr && z != nullptr && f->DockNode != nullptr && f->RootWindowDockTree == z->RootWindowDockTree;
        diag::detail(std::string("Knopftest: dock1 zusammengesetzt: ") + (ok ? "ja" : "nein") +
                     (f != nullptr && f->DockNode != nullptr ? " (Knoten)" : " (kein Knoten)"));
        return ok;
    }));

    // --- 5. Anordnung speichern unter, zuruecksetzen, laden ------------------------
    add(tu("dock1: Lagen merken", [L] {
        L->zeitRect = dock1::rect("mission.zeitleiste");
        L->flussRect = dock1::rect("mission.fluss");
        diag::detail("Knopftest: dock1 gespeichert mit Zeitleiste " + dock1::rechteck(L->zeitRect) + ", Script Flow " +
                     dock1::rechteck(L->flussRect));
    }));
    menueKlick(tr(Str::DockSaveAs));
    add(warteBis("dock1: Fenster \"Anordnung speichern\"", [] { return ImGui::FindWindowByName("###docksichern") != nullptr; }, 60));
    add(tippe("Zwei Bildschirme"));
    add(klick(tr(Str::DockSave), "###docksichern"));
    add(pruefe("dock1: gespeichert in Settings::fenster (b1/Zwei Bildschirme)", [] {
        return g_app->settings.fenster.count(fenster::schluesselBenannt(1, "Zwei Bildschirme")) == 1U &&
               dock::gespeicherte(1) == std::vector<std::string>{"Zwei Bildschirme"};
    }));
    menueKlick(tr(Str::DockReset));
    add(stabil("dock1: zurueckgesetzt", 20));
    add(pruefe("dock1: zurueckgesetzt - alle Panels der Mission zuhause, kein eigenes Fenster",
               [] { return dock::zuhause("mission.zeitleiste") && dock::zuhause("mission.fluss") && !dock1::aktiv("mission.zeitleiste"); }));
    add(tu("dock1: Hauptfenster zurueck auf die Testgroesse (Masse vergleichbar)", [L] {
        (void)platform::fensterLageSetzen(ImGui::GetMainViewport()->PlatformHandle, L->hx, L->hy, L->hb, L->hh);
    }));
    add(stabil("dock1: Hauptfenster wie am Anfang", 30));
    add(pruefe("dock1: Mission nach dem Zuruecksetzen = Standardanordnung", [L] {
        const dock1::Masse m = dock1::messen();
        diag::detail("Knopftest: dock1 nach Zuruecksetzen " + dock1::text(m));
        return dock1::gleich(m, L->vorher[1]);
    }));
    add(tu("dock1: Hauptfenster wieder verkleinern", [] {
        const ImGuiPlatformMonitor& m = dock1::hauptMonitor();
        (void)platform::fensterLageSetzen(ImGui::GetMainViewport()->PlatformHandle, static_cast<int>(m.WorkPos.x) + 60,
                                          static_cast<int>(m.WorkPos.y) + 40, static_cast<int>(m.WorkSize.x * 0.6F),
                                          static_cast<int>(m.WorkSize.y * 0.75F));
    }));
    add(stabil("dock1: verkleinert", 20));
    add(dock1::bildzeit("mit 1 Viewport", 120, &L->dt1, &L->ms1));
    add(tu("dock1: Bildzeiten", [L] {
        char t[240];
        std::snprintf(t, sizeof(t),
                      "Tempo: Bildzeit mit 1 Viewport %.2f ms (eigene Fenster %.3f ms), mit 3 Viewports %.2f ms (eigene Fenster %.3f ms)",
                      L->dt1, L->ms1, L->dt3, L->ms3);
        diag::detail(t);
    }));
    add(klick(tr(Str::DockMenu), "###main"));
    add(klick(tr(Str::DockLoad)));
    add(klick("Zwei Bildschirme##dockladen"));
    add(warteBis("dock1: Anordnung geladen - beide Panels wieder draussen",
                 [] { return dock1::aktiv("mission.zeitleiste") && dock1::aktiv("mission.fluss"); }, 120));
    add(stabil("dock1: geladen", 10));
    add(pruefe("dock1: geladen - Lage, Groesse und Zusammensetzung wie gespeichert", [L] {
        const ImRect z = dock1::rect("mission.zeitleiste");
        const ImRect f = dock1::rect("mission.fluss");
        const ImGuiWindow* fw = dock1::panelFenster("mission.fluss");
        const ImGuiWindow* zw = dock1::panelFenster("mission.zeitleiste");
        diag::detail("Knopftest: dock1 geladen: Zeitleiste " + dock1::rechteck(z) + ", Script Flow " + dock1::rechteck(f) + ", Knoten " +
                     std::to_string(fw != nullptr && fw->DockNode != nullptr ? fw->DockNode->ID : 0U) + "/" +
                     std::to_string(zw != nullptr && zw->DockNode != nullptr ? zw->DockNode->ID : 0U) + ", Wurzel " +
                     (fw != nullptr ? fw->RootWindowDockTree->Name : "-") + " / " + (zw != nullptr ? zw->RootWindowDockTree->Name : "-"));
        const auto nah = [](const ImRect& a, const ImRect& b) {
            return std::fabs(a.Min.x - b.Min.x) <= 1.0F && std::fabs(a.Min.y - b.Min.y) <= 1.0F && std::fabs(a.Max.x - b.Max.x) <= 1.0F &&
                   std::fabs(a.Max.y - b.Max.y) <= 1.0F;
        };
        return nah(z, L->zeitRect) && nah(f, L->flussRect) && fw != nullptr && zw != nullptr && fw->RootWindowDockTree == zw->RootWindowDockTree;
    }));
    menueKlick(tr(Str::DockSaveAs));
    add(tippe("Weg damit"));
    add(klick(tr(Str::DockSave), "###docksichern"));
    add(klick(tr(Str::DockMenu), "###main"));
    add(klick(tr(Str::DockDelete)));
    add(klick("Weg damit##dockloeschen"));
    add(pruefe("dock1: geloescht - nur noch \"Zwei Bildschirme\"",
               [] { return dock::gespeicherte(1) == std::vector<std::string>{"Zwei Bildschirme"}; }));

    // --- 6. Andocken: Schliessknopf, Menue, Ziehen auf den alten Platz -------------
    add(klick(std::string(tr(Str::GroupScriptFlow)) + "###dock.mission.fluss"));   // den Reiter waehlen - sein x erscheint
    add(dock1::schliessknopf("mission.fluss"));
    add(warteBis("dock1: Schliessknopf - Script Flow wieder im Hauptfenster", [] { return dock::zuhause("mission.fluss"); }, 60));
    menueKlick(fensterEintrag(Str::DockPanelTimeline, "mission.zeitleiste"));
    add(warteBis("dock1: Menue - Zeitleiste wieder im Hauptfenster", [] { return dock::zuhause("mission.zeitleiste"); }, 60));
    add(stabil("dock1: Band wieder da", 20));
    add(pruefe("dock1: Band wieder da (toolH > 0)", [] { return g_app->lastLayoutLog.toolH > 0; }));
    menueKlick(fensterEintrag(Str::EntityLayers, "mission.ebenen"));
    add(warteBis("dock1: Ebenenliste in eigenem Fenster", [] { return dock1::aktiv("mission.ebenen"); }, 60));
    add(tu("dock1: Ebenenliste mitten ins Hauptfenster (zieht man sie, zeigt das Hauptfenster ihren Platz)", [] {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        dock1::setze("mission.ebenen", ImVec2{vp->Pos.x + vp->Size.x * 0.45F, vp->Pos.y + vp->Size.y * 0.3F},
                     ImVec2{vp->Size.x * 0.25F, vp->Size.y * 0.4F});
    }));
    add(stabil("dock1: Ebenenliste schwebt", 10));
    add({"dock1: Ebenenliste auf ihren alten Platz ziehen", [](int b) {
             ImGuiIO& io = ImGui::GetIO();
             static ImVec2 von;
             static ImVec2 nach;
             if (b == 0) {
                 von = dock1::titel("mission.ebenen");
                 ImVec2 a;
                 ImVec2 z;
                 dock::zielPlatz("mission.ebenen", a, z);
                 nach = ImVec2{(a.x + z.x) * 0.5F, (a.y + z.y) * 0.5F};
                 setzeMaus(von);
                 return false;
             }
             if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
             if (b >= 3 && b <= 23) {
                 const float f = static_cast<float>(b - 3) / 20.0F;
                 setzeMaus(ImVec2{von.x + (nach.x - von.x) * f, von.y + (nach.y - von.y) * f});
                 return false;
             }
             if (b == 28) { io.AddMouseButtonEvent(0, false); return false; }
             return b >= 34;
         }});
    add(warteBis("dock1: Ebenenliste durch Ziehen wieder angedockt", [] { return dock::zuhause("mission.ebenen"); }, 60));

    // --- 7. Fehlerfaelle: Bildschirm fehlt, zu klein, anderer Massstab -------------
    menueKlick(fensterEintrag(Str::DockPanelTimeline, "mission.zeitleiste"));
    add(warteBis("dock1: Zeitleiste wieder draussen", [] { return dock1::aktiv("mission.zeitleiste"); }, 60));
    add(tu("dock1: ein zweiter Bildschirm rechts (vorgetaeuscht), Zeitleiste darauf", [L] {
        float rechts = 0.0F;
        for (const ImGuiPlatformMonitor& m : L->echt) { rechts = std::max(rechts, m.MainPos.x + m.MainSize.x); }
        ImGuiPlatformMonitor zweiter = L->echt.front();
        zweiter.MainPos = ImVec2{rechts, 0.0F};
        zweiter.WorkPos = ImVec2{rechts, 0.0F};
        zweiter.MainSize = ImVec2{1920.0F, 1080.0F};
        zweiter.WorkSize = ImVec2{1920.0F, 1040.0F};
        zweiter.PlatformHandle = nullptr;
        dock1::sim().monitore = L->echt;
        dock1::sim().monitore.push_back(zweiter);
        dock1::sim().bildschirme = true;
        dock1::setze("mission.zeitleiste", ImVec2{rechts + 300.0F, 200.0F}, ImVec2{900.0F, 400.0F});
    }));
    add(stabil("dock1: Zeitleiste auf dem zweiten Bildschirm", 20));
    add(pruefe("dock1: auf dem zweiten Bildschirm bleibt sie, wo sie ist", [] {
        const ImRect r = dock1::rect("mission.zeitleiste");
        return dock1::eigenesOsFenster("mission.zeitleiste") && std::fabs(r.Min.y - 200.0F) <= 1.0F && r.Min.x > 300.0F;
    }));
    add(tu("dock1: zweiter Bildschirm abgezogen", [L] { dock1::sim().monitore = L->echt; }));
    add(stabil("dock1: Bildschirm fehlt", 20));
    add(pruefe("dock1: Zeitleiste zurueck, ganz auf dem Bildschirm des Hauptfensters", [] {
        const ImRect r = dock1::rect("mission.zeitleiste");
        const ImGuiPlatformMonitor& m = dock1::hauptMonitor();
        const ImRect mr{m.WorkPos, ImVec2{m.WorkPos.x + m.WorkSize.x, m.WorkPos.y + m.WorkSize.y}};
        diag::detail("Knopftest: dock1 nach dem Abziehen: Zeitleiste " + dock1::rechteck(r));
        return mr.Contains(r) && std::fabs(r.GetWidth() - 900.0F) <= 1.0F && std::fabs(r.GetHeight() - 400.0F) <= 1.0F;
    }));
    add(tu("dock1: ein kleiner Bildschirm (640x480), Zeitleiste darauf", [L] {
        ImGuiPlatformMonitor klein = L->echt.front();
        klein.MainPos = ImVec2{-640.0F, 0.0F};
        klein.WorkPos = ImVec2{-640.0F, 0.0F};
        klein.MainSize = ImVec2{640.0F, 480.0F};
        klein.WorkSize = ImVec2{640.0F, 480.0F};
        klein.PlatformHandle = nullptr;
        dock1::sim().monitore = L->echt;
        dock1::sim().monitore.push_back(klein);
        dock1::setze("mission.zeitleiste", ImVec2{-620.0F, 20.0F}, ImVec2{900.0F, 400.0F});
    }));
    add(stabil("dock1: kleiner Bildschirm", 20));
    add(pruefe("dock1: Zeitleiste passend verkleinert, ganz auf dem kleinen Bildschirm", [] {
        const ImRect r = dock1::rect("mission.zeitleiste");
        diag::detail("Knopftest: dock1 kleiner Bildschirm: Zeitleiste " + dock1::rechteck(r));
        return r.Min.x >= -640.0F && r.Max.x <= 0.0F && r.Min.y >= 0.0F && r.Max.y <= 480.0F && r.GetWidth() < 640.0F;
    }));
    add(tu("dock1: echte Bildschirme wiederherstellen, Zeitleiste neben das Hauptfenster", [L] {
        dock1::sim().bildschirme = false;
        ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
        pio.Monitors.resize(0);
        for (const ImGuiPlatformMonitor& m : L->echt) { pio.Monitors.push_back(m); }
        dock1::setze("mission.zeitleiste", dock1::nebenHaupt(0.0F), ImVec2{600.0F, 300.0F});
    }));
    add(stabil("dock1: echte Bildschirme", 20));
    add(tu("dock1: Massstab des eigenen Fensters 150 % des Hauptfensters", [L] {
        L->zeitBreite = dock1::rect("mission.zeitleiste").GetWidth();
        dock1::sim().dpi = ImGui::GetMainViewport()->DpiScale * 1.5F;
    }));
    add(stabil("dock1: 150 %", 20));
    add(pruefe("dock1: 150 % - Fenster und Schrift wachsen mit (ConfigDpiScaleViewports/-Fonts)", [L] {
        const ImGuiWindow* w = dock1::panelFenster("mission.zeitleiste");
        const float haupt = ImGui::GetMainViewport()->DpiScale;
        char t[200];
        std::snprintf(t, sizeof(t),
                      "Knopftest: dock1 150 %%: Breite %.0f -> %.0f, Schrift %.1f (Hauptfenster %.1f), Massstab %.3f (Hauptfenster %.2f)",
                      static_cast<double>(L->zeitBreite), w != nullptr ? static_cast<double>(w->Size.x) : 0.0,
                      w != nullptr ? static_cast<double>(w->FontRefSize) : 0.0,
                      ImGui::FindWindowByName("###main") != nullptr ? static_cast<double>(ImGui::FindWindowByName("###main")->FontRefSize) : 0.0,
                      w != nullptr ? static_cast<double>(w->Viewport->DpiScale) : 0.0, static_cast<double>(haupt));
        diag::detail(t);
        const ImGuiWindow* h = ImGui::FindWindowByName("###main");
        return w != nullptr && h != nullptr && std::fabs(w->Viewport->DpiScale - haupt * 1.5F) < 0.001F &&
               std::fabs(w->Size.x - L->zeitBreite * 1.5F) <= 2.0F && std::fabs(w->FontRefSize - h->FontRefSize * 1.5F) <= 1.0F;
    }));
    add(tu("dock1: Massstab zurueck", [] { dock1::sim().dpi = 0.0F; }));
    add(stabil("dock1: 100 %", 20));
    add(pruefe("dock1: zurueck - Fenster wieder so breit wie vorher (+-2)", [L] {
        return std::fabs(dock1::rect("mission.zeitleiste").GetWidth() - L->zeitBreite) <= 2.0F;
    }));

    // --- 8. Map Editor: Kamera in eigenem Fenster ------------------------------
    add(tu("dock1: Map Editor, Anordnung Regular", [] {
        g_app->leftMode = 3;
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Regular;
        e.teilerFuer = -1;
    }));
    add(warteBis("dock1: Spiel bereit (Karten-Editor)", [] { return ke::spielBereit(); }, 6000));
    add(tu("dock1: Testkarte mit Licht, Kamera textured, Echtzeit-Licht an (wie Stufe 7)", [] {
        stufe7::lichtKarteSchreiben(false);
        (void)ke::karteOeffnen(stufe7::lichtKarte());
        ke::editor().kameraE.modus = karte::KameraModus::Textur;
        ke::editor().echtLicht.an = true;
        ke::editor().echtLicht.backen = false;
        stufe7::kameraUeber(128, 400);
    }));
    add(warteBis("dock1: Texturen der Testkarte", [] { return ke::kameraBilderFertig(); }, 3000));
    add(stabil("dock1: Map Editor", 30));
    add(tu("dock1: Hoehe des Texturbrowsers merken", [L] {
        for (const auto& f : ke::editor().felderImBild) {
            if (f.name == "Texturen") { L->felderTexturenH = f.h; }
        }
    }));
    menueKlick(fensterEintrag(Str::KeViewCamera, "karte.kamera"));
    add(warteBis("dock1: Kamera in eigenem Fenster", [] { return dock1::aktiv("karte.kamera"); }, 120));
    add(stabil("dock1: Kamera draussen", 10));
    add(pruefe("dock1: Kamera fehlt im Hauptfenster, der Texturbrowser nimmt die Hoehe (QSplitter wie NRC)", [L] {
        int texturenH = 0;
        bool kameraDa = false;
        bool dockKamera = false;
        for (const auto& f : ke::editor().felderImBild) {
            if (f.name == "Texturen") { texturenH = f.h; }
            if (f.name == "Kamera" && f.b > 0 && f.h > 0) { kameraDa = true; }
            if (f.name == "Dock:Kamera" && f.b > 0 && f.h > 0) { dockKamera = true; }
        }
        diag::detail("Knopftest: dock1 Texturbrowser " + std::to_string(L->felderTexturenH) + " -> " + std::to_string(texturenH));
        return !kameraDa && dockKamera && texturenH > L->felderTexturenH;
    }));
    add(tu("dock1: Kamerafenster neben das Hauptfenster", [] {
        const ImGuiPlatformMonitor& m = dock1::hauptMonitor();
        dock1::setze("karte.kamera", dock1::nebenHaupt(0.0F), ImVec2{m.WorkSize.x * 0.3F, m.WorkSize.y * 0.4F});
    }));
    add(warteBis("dock1: Kamera eigenes Windows-Fenster", [] { return dock1::eigenesOsFenster("karte.kamera"); }, 120));
    add(pause(0.5));
    // Die Mitte der Ansicht trifft im eigenen Fenster (andere Groesse) einen
    // anderen Texel des Steinbodens - verglichen wird deshalb wie in Stufe 7: das
    // Licht naeher macht den Boden heller, im eigenen Fenster.
    add(tu("dock1: Helligkeit im eigenen Fenster, dann Licht naeher (128 0 48)", [L] {
        L->hellHeim = stufe7::helligkeit();
        stufe7::erstesLicht()->setzen("origin", "128 0 48");
    }));
    add(pause(0.4));
    add(pruefe("dock1: Echtzeit-Licht zeichnet im eigenen Fenster - ein Licht, naeher ist der Boden heller", [L] {
        const ke::Editor& e = ke::editor();
        const int h = stufe7::helligkeit();
        diag::detail("Knopftest: dock1 Echtzeit-Licht im eigenen Fenster: Lichter " + std::to_string(e.echtLicht.lichter) + ", gezeichnet " +
                     std::to_string(e.echtLicht.gezeichnet) + ", Helligkeit " + std::to_string(L->hellHeim) + " -> " + std::to_string(h) +
                     " (Licht naeher)");
        return ke::echtLichtAktiv() && e.echtLicht.gezeichnet == 1 && L->hellHeim > 0 && h > L->hellHeim + 10;
    }));
    add(tu("dock1: Licht zurueck (0 0 96)", [] { stufe7::erstesLicht()->setzen("origin", "0 0 96"); }));
    add(dock1::bildLesen("Kamera mit Echtzeit-Licht", "karte.kamera", 40));
    add(klickAn("in die Kamera (eigenes Fenster)", [] { return dock1::rect("karte.kamera").GetCenter(); }));
    add(pruefe("dock1: Klick macht die Kamera zum aktiven Feld", [] { return ke::editor().aktivesFeld == 3; }));
    add(tu("dock1: Kamera merken, simulierter Zeiger an", [L] {
        L->kamPos = ke::editor().kamera.ursprung;
        L->yaw = ke::editor().kamera.winkel[1];
        ke::zeigerSim().an = true;
    }));
    add(halteTaste("Pfeil hoch (CameraForward) im eigenen Fenster", ImGuiKey_UpArrow, 20));
    add(pruefe("dock1: Taste gilt im aktiven Fenster - Kamera faehrt vor", [L] {
        const karte::Vektor3f p = ke::editor().kamera.ursprung;
        return std::fabs(p[0] - L->kamPos[0]) + std::fabs(p[1] - L->kamPos[1]) + std::fabs(p[2] - L->kamPos[2]) > 1.0F;
    }));
    add({"dock1: M2 kurz - Freelook im eigenen Fenster", [L](int b) {
             ImGuiIO& io = ImGui::GetIO();
             ke::ZeigerSim& z = ke::zeigerSim();
             if (b == 0) {
                 const ImVec2 c = dock1::rect("karte.kamera").GetCenter();
                 z.x = std::floor(c.x);
                 z.y = std::floor(c.y);
                 L->yaw = ke::editor().kamera.winkel[1];
             }
             if (b == 2) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, true); }
             if (b == 3) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, false); }
             return b >= 6;
         }});
    add(pruefe("dock1: Freelook an", [] { return ke::editor().kamFenster.frei; }));
    add({"dock1: Maus 300 Punkte nach rechts", [](int b) {
             if (b >= 1 && b <= 30) { ke::zeigerSim().x += 10.0F; }
             return b >= 36;
         }});
    add(pruefe("dock1: Freelook dreht im eigenen Fenster (300 Punkte = 30 Grad wie NRC)", [L] {
        const float qt = std::max(1.0F, std::round(g_app->dpiScale));
        const float soll = (300.0F / qt) * 0.0333333F * 3.0F;
        float ist = L->yaw - ke::editor().kamera.winkel[1];
        if (ist < 0.0F) { ist += 360.0F; }
        char t[160];
        std::snprintf(t, sizeof(t), "Knopftest: dock1 Freelook im eigenen Fenster: %.3f Grad (soll %.3f)", static_cast<double>(ist),
                      static_cast<double>(soll));
        diag::detail(t);
        return std::fabs(ist - soll) < 0.05F;
    }));
    add(pause(0.3));
    add({"dock1: M2 kurz - Freelook aus", [](int b) {
             ImGuiIO& io = ImGui::GetIO();
             if (b == 2) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, true); }
             if (b == 3) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, false); }
             return b >= 6;
         }});
    add(tu("dock1: simulierter Zeiger aus", [] { ke::zeigerSim().an = false; }));
    menueKlick(fensterEintrag(Str::KeViewCamera, "karte.kamera"));
    add(stabil("dock1: Kamera wieder im Hauptfenster", 20));
    add(pruefe("dock1: Kamera zurueck, Texturbrowser wieder wie vorher", [L] {
        int texturenH = 0;
        bool kameraDa = false;
        for (const auto& f : ke::editor().felderImBild) {
            if (f.name == "Texturen") { texturenH = f.h; }
            if (f.name == "Kamera" && f.b > 0 && f.h > 0) { kameraDa = true; }
        }
        return kameraDa && texturenH == L->felderTexturenH && dock::zuhause("karte.kamera");
    }));

    // --- 8b. Jedes Panel einmal heraus und zurueck ------------------------------
    add(tu("dock1: ein Menue im Menue-Editor (fuer seine Panels)", [] {
        (void)menueeditor::oeffneText("dock1.menu",
                                      "menuDef\n{\n  name \"dock1\"\n  rect 0 0 640 480\n  itemDef\n  {\n    name \"knopf\"\n"
                                      "    rect 100 100 200 40\n    type ITEM_TYPE_BUTTON\n    text \"Hallo\"\n    visible 1\n  }\n}\n");
    }));
    for (const dock::Panel& p : dock::panels()) {
        if (p.bereich == 3) { continue; }
        const std::string id = p.id;
        const int bereich = p.bereich;
        add(tu("dock1: " + id + " in eigenem Fenster", [id, bereich] {
            g_app->leftMode = bereich;
            dock::herausnehmen(id.c_str());
        }));
        add(warteBis("dock1: " + id + " gezeichnet", [id] { return dock1::aktiv(id.c_str()); }, 120));
        add(pause(0.2));
        add(pruefe("dock1: " + id + " zeichnet im eigenen Fenster (Elemente oder Dreiecke)", [id] {
            int elemente = 0;
            for (const Element& e : g_elemente) {
                if (e.fenster.find("###dock." + id) != std::string::npos) { ++elemente; }
            }
            int ecken = 0;
            for (const ImGuiWindow* w : GImGui->Windows) {
                if (w->WasActive && std::string(w->Name).find("###dock." + id) != std::string::npos) { ecken += w->DrawList->VtxBuffer.Size; }
            }
            diag::detail("Knopftest: dock1 " + id + ": " + std::to_string(elemente) + " Elemente, " + std::to_string(ecken) + " Ecken");
            return elemente > 0 || ecken > 200;
        }));
        add(tu("dock1: " + id + " zurueck", [id] { dock::andocken(id.c_str()); }));
        add(warteBis("dock1: " + id + " wieder im Hauptfenster", [id] { return dock::zuhause(id.c_str()) && !dock1::aktiv(id.c_str()); }, 60));
    }
    add(tu("dock1: Map Editor, Anordnung Split", [] {
        g_app->leftMode = 3;
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Split;
        e.teilerFuer = -1;
    }));
    add(stabil("dock1: Split", 20));
    for (const char* f : {"karte.kamera", "karte.xy", "karte.xz", "karte.yz"}) {
        const std::string id = f;
        const std::string kurz = id == "karte.kamera" ? "Kamera" : id == "karte.xy" ? "XY" : id == "karte.xz" ? "XZ" : "YZ";
        add(tu("dock1: Split - " + id + " in eigenem Fenster", [id] { dock::herausnehmen(id.c_str()); }));
        add(warteBis("dock1: Split - " + id + " gezeichnet", [id] { return dock1::aktiv(id.c_str()); }, 120));
        add(pause(0.2));
        add(pruefe("dock1: Split - " + id + " im eigenen Fenster, nicht mehr im Hauptfenster", [kurz] {
            bool heim = false;
            bool draussen = false;
            for (const auto& r : ke::editor().felderImBild) {
                if (r.name == kurz && r.b > 0 && r.h > 0) { heim = true; }
                if (r.name == "Dock:" + kurz && r.b > 0 && r.h > 0) { draussen = true; }
            }
            return !heim && draussen;
        }));
        add(tu("dock1: Split - " + id + " zurueck", [id] { dock::andocken(id.c_str()); }));
        add(warteBis("dock1: Split - " + id + " wieder im Hauptfenster", [id] { return dock::zuhause(id.c_str()) && !dock1::aktiv(id.c_str()); }, 60));
    }
    add(tu("dock1: Map Editor, Anordnung Regular", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Regular;
        e.teilerFuer = -1;
        e.echtLicht.an = false;
    }));
    for (const char* f : {"karte.xy", "karte.konsole", "karte.texturen"}) {
        const std::string id = f;
        add(tu("dock1: Regular - " + id + " in eigenem Fenster", [id] { dock::herausnehmen(id.c_str()); }));
        add(warteBis("dock1: Regular - " + id + " gezeichnet", [id] { return dock1::aktiv(id.c_str()); }, 120));
        add(tu("dock1: Regular - " + id + " zurueck", [id] { dock::andocken(id.c_str()); }));
        add(warteBis("dock1: Regular - " + id + " wieder im Hauptfenster", [id] { return dock::zuhause(id.c_str()); }, 60));
    }

    // --- 9. Menu Editor und Events ------------------------------------------------
    add(tu("dock1: Menu Editor", [] { g_app->leftMode = 4; }));
    add(stabil("dock1: Menu Editor", 20));
    auto reiterB = std::make_shared<float>(0.0F);
    add(tu("dock1: Breite der Reiter-Spalte merken", [reiterB] {
        ImVec2 a;
        ImVec2 z;
        dock::zielPlatz("menue.reiter", a, z);
        *reiterB = z.x - a.x;
    }));
    menueKlick(fensterEintrag(Str::DockPanelCanvas, "menue.flaeche"));
    add(warteBis("dock1: Flaeche des Menue-Editors in eigenem Fenster", [] { return dock1::aktiv("menue.flaeche"); }, 60));
    add(tu("dock1: Flaeche neben das Hauptfenster (eigenes Windows-Fenster)", [] {
        const ImGuiPlatformMonitor& m = dock1::hauptMonitor();
        dock1::setze("menue.flaeche", dock1::nebenHaupt(0.0F), ImVec2{m.WorkSize.x * 0.3F, m.WorkSize.y * 0.5F});
    }));
    add(warteBis("dock1: Flaeche eigenes Windows-Fenster", [] { return dock1::eigenesOsFenster("menue.flaeche"); }, 120));
    add(dock1::bildLesen("Flaeche des Menue-Editors", "menue.flaeche", 4));
    add(pruefe("dock1: ohne die Flaeche nimmt die Reiter-Spalte den Platz (breiter als vorher)", [reiterB] {
        ImVec2 a;
        ImVec2 z;
        dock::zielPlatz("menue.reiter", a, z);
        diag::detail("Knopftest: dock1 Reiter-Spalte " + std::to_string(static_cast<int>(*reiterB)) + " -> " +
                     std::to_string(static_cast<int>(z.x - a.x)));
        return dock::draussen("menue.flaeche") && z.x - a.x > *reiterB + 100.0F;
    }));
    add(dock1::schliessknopf("menue.flaeche"));
    add(warteBis("dock1: Schliessknopf - Flaeche wieder im Hauptfenster", [] { return dock::zuhause("menue.flaeche"); }, 60));
    add(tu("dock1: Events", [] { g_app->leftMode = 0; }));
    add(stabil("dock1: Events", 20));
    menueKlick(zeigenEintrag(Str::GroupActions, "ereignisse.aktionen"));
    add(stabil("dock1: Actions ausgeblendet", 20));
    add(pruefe("dock1: Actions ausgeblendet - zwei Spalten, kein Fenster", [] {
        return dock::versteckt("ereignisse.aktionen") && !dock1::aktiv("ereignisse.aktionen") && g_app->spaltenB[2] == 0.0F;
    }));
    menueKlick(fensterEintrag(Str::GroupEvents, "ereignisse.liste"));
    add(warteBis("dock1: Ereignisliste in eigenem Fenster", [] { return dock1::aktiv("ereignisse.liste"); }, 60));
    add(stabil("dock1: Ereignisliste draussen", 10));
    menueKlick(tr(Str::DockReset));
    add(stabil("dock1: Events zurueckgesetzt", 20));

    // --- 10. Standardanordnung nach allem: unveraendert ---------------------------
    add(tu("dock1: Hauptfenster zurueck", [L] {
        (void)platform::fensterLageSetzen(ImGui::GetMainViewport()->PlatformHandle, L->hx, L->hy, L->hb, L->hh);
    }));
    for (int m = 0; m < 3; ++m) {
        add(tu("dock1: Modus " + std::to_string(m) + " zuruecksetzen", [m] {
            dock::zuruecksetzen(m);
            g_app->leftMode = m;
        }));
        add(stabil("dock1: Standardanordnung Modus " + std::to_string(m) + " am Ende", 30));
        add(pruefe("dock1: Modus " + std::to_string(m) + " am Ende = Standardanordnung vom Anfang", [L, m] {
            const dock1::Masse jetzt = dock1::messen();
            diag::detail("Knopftest: dock1 Ende " + dock1::text(jetzt));
            return dock1::gleich(jetzt, L->vorher[static_cast<std::size_t>(m)]);
        }));
    }

    // --- 11. Fuer den zweiten Start: Zeitleiste draussen an fester Stelle ---------
    add(tu("dock1: Mission, Zeitleiste draussen fuer den Neustart", [] {
        g_app->leftMode = 1;
        dock::herausnehmen("mission.zeitleiste");
    }));
    add(warteBis("dock1: Zeitleiste fuer den Neustart draussen", [] { return dock1::aktiv("mission.zeitleiste"); }, 60));
    add(tu("dock1: Lage setzen", [] {
        const ImGuiPlatformMonitor& m = dock1::hauptMonitor();
        dock1::setze("mission.zeitleiste", ImVec2{m.WorkPos.x + 200.0F, m.WorkPos.y + 150.0F}, ImVec2{700.0F, 260.0F});
    }));
    add(stabil("dock1: Zeitleiste fuer den Neustart", 20));
    add(tu("dock1: Script Flow dazu", [] { dock::herausnehmen("mission.fluss"); }));
    add(warteBis("dock1: Script Flow fuer den Neustart draussen", [] { return dock1::aktiv("mission.fluss"); }, 60));
    add(tu("dock1: Script Flow neben die Zeitleiste", [] {
        const ImGuiPlatformMonitor& m = dock1::hauptMonitor();
        dock1::setze("mission.fluss", ImVec2{m.WorkPos.x + 1000.0F, m.WorkPos.y + 150.0F}, ImVec2{400.0F, 300.0F});
    }));
    add(stabil("dock1: Script Flow daneben", 10));
    add(dock1::ziehenUndHalten("dock1: Script Flow auf die Zeitleiste (fuer den Neustart)", [] { return dock1::titel("mission.fluss"); },
                               [] { return dock1::rect("mission.zeitleiste").GetCenter(); }));
    add(dock1::loslassen("dock1: loslassen"));
    add(stabil("dock1: zusammengesetzt fuer den Neustart", 20));
    add(tu("dock1: Erwartung fuer dock1neustart schreiben", [] {
        dock::jetztMerken();
        const ImRect r = dock1::rect("mission.zeitleiste");
        const ImGuiWindow* f = dock1::panelFenster("mission.fluss");
        const ImGuiWindow* z = dock1::panelFenster("mission.zeitleiste");
        const int zusammen = f != nullptr && z != nullptr && f->DockNode != nullptr && f->RootWindowDockTree == z->RootWindowDockTree ? 1 : 0;
        std::ofstream o(::bhed::utf8Pfad(platform::executableDirectory() + "\\dock1_erwartet.txt"));
        o << static_cast<int>(r.Min.x) << " " << static_cast<int>(r.Min.y) << " " << static_cast<int>(r.GetWidth()) << " "
          << static_cast<int>(r.GetHeight()) << " " << zusammen << "\n";
        diag::detail("Knopftest: dock1 fuer den Neustart: Zeitleiste " + dock1::rechteck(r) + (zusammen != 0 ? ", mit Script Flow zusammengesetzt" : ""));
    }));
    return s;
}

// --- BHED_EDITORTEST=dock1neustart: der zweite Start im selben Ordner -----------
std::vector<Schritt> dock1NeustartSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("dock1neustart: Mission", [] { g_app->leftMode = 1; }));
    add(stabil("dock1neustart: Mission", 30));
    add(pruefe("dock1neustart: Anordnung aus den Einstellungen - Zeitleiste und Script Flow draussen, zusammengesetzt, an der gespeicherten Stelle",
               [] {
        std::ifstream f(::bhed::utf8Pfad(platform::executableDirectory() + "\\dock1_erwartet.txt"));
        int x = 0, y = 0, b = 0, h = 0, zusammen = 0;
        if (!(f >> x >> y >> b >> h >> zusammen)) { return false; }
        const ImRect r = dock1::rect("mission.zeitleiste");
        const ImGuiWindow* fw = dock1::panelFenster("mission.fluss");
        const ImGuiWindow* zw = dock1::panelFenster("mission.zeitleiste");
        const bool jetztZusammen = fw != nullptr && zw != nullptr && fw->DockNode != nullptr && fw->RootWindowDockTree == zw->RootWindowDockTree;
        diag::detail("Knopftest: dock1neustart: erwartet " + std::to_string(x) + "," + std::to_string(y) + " " + std::to_string(b) + "x" +
                     std::to_string(h) + (zusammen != 0 ? " zusammengesetzt" : "") + ", gefunden " + dock1::rechteck(r) +
                     (jetztZusammen ? " zusammengesetzt" : ""));
        return zusammen == 1 && jetztZusammen && dock::draussen("mission.zeitleiste") && dock::draussen("mission.fluss") &&
               std::abs(static_cast<int>(r.Min.x) - x) <= 1 && std::abs(static_cast<int>(r.Min.y) - y) <= 1 &&
               std::abs(static_cast<int>(r.GetWidth()) - b) <= 1 && std::abs(static_cast<int>(r.GetHeight()) - h) <= 1 &&
               g_app->lastLayoutLog.toolH == 0;
    }));
    add(pruefe("dock1neustart: gespeicherte Anordnung \"Zwei Bildschirme\" noch da",
               [] { return dock::gespeicherte(1) == std::vector<std::string>{"Zwei Bildschirme"}; }));
    add(tu("dock1neustart: zuruecksetzen", [] { dock::zuruecksetzen(1); }));
    add(stabil("dock1neustart: zurueckgesetzt", 20));
    add(pruefe("dock1neustart: zurueckgesetzt - Band wieder da, keine Panelfenster", [] {
        return g_app->lastLayoutLog.toolH > 0 && !dock1::aktiv("mission.zeitleiste") && !dock1::aktiv("mission.fluss");
    }));
    return s;
}

// --- BHED_EDITORTEST=dock2mass: DockSpace gegen die alte Anordnung (dock2) -------
//
// Misst in Events, Mission und Model die Kindfenster der Panels (Modusleiste,
// Listen, Ansicht, Script Flow, Actions, Band) einmal mit den Tabellen und
// einmal im durchgehenden DockSpace (dockraumAn) - die Standardanordnung muss
// Punkt fuer Punkt dieselbe sein.
namespace dock2 {

using Masse = std::map<std::string, ImRect>;

Masse messen() {
    static const char* const kNamen[] = {"/modebar_", "/events_", "/##flowarea_", "/buttons_", "/toolbar_", "/mapview_",
                                         "/modelview_", "/animliste_", "/toolband_", "/entlayers_"};
    Masse m;
    const ImVec2 o = ImGui::GetMainViewport()->Pos;
    for (const ImGuiWindow* w : GImGui->Windows) {
        if (!w->WasActive) { continue; }
        const std::string n = w->Name;
        for (const char* k : kNamen) {
            if (n.find(k) == std::string::npos) { continue; }
            const std::string schluessel = std::to_string(g_app->leftMode) + std::string(k).substr(1, std::string(k).size() - 2);
            m[schluessel] = ImRect{ImVec2{w->Pos.x - o.x, w->Pos.y - o.y}, ImVec2{w->Pos.x + w->Size.x - o.x, w->Pos.y + w->Size.y - o.y}};
        }
    }
    return m;
}

}  // namespace dock2

std::vector<Schritt> dock2MassSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto alt = std::make_shared<dock2::Masse>();
    auto neu = std::make_shared<dock2::Masse>();
    const bool vorher = dockraumAn();
    add(tu("dock2mass: alte Anordnung", [] { dockraumAn() = false; }));
    for (const int runde : {0, 1, 2}) {
        for (const int modus : {1, 2, 0}) {
            add(tu("dock2mass: Modus " + std::to_string(modus), [=] {
                if (runde == 2) { dockraumAn() = true; }
                g_app->leftMode = modus;
            }));
            add(stabil("dock2mass: Modus " + std::to_string(modus), 20));
            if (runde == 0) { continue; }
            add(tu("dock2mass: messen", [=] {
                const dock2::Masse m = dock2::messen();
                (runde == 1 ? *alt : *neu).insert(m.begin(), m.end());
            }));
        }
    }
    add(pruefe("dock2mass: Standardanordnung im DockSpace = alte Anordnung (alle Kindfenster der Panels)", [=] {
        int abweichungen = 0;
        for (const auto& [k, a] : *alt) {
            const auto it = neu->find(k);
            const bool gleich = it != neu->end() && std::fabs(it->second.Min.x - a.Min.x) <= 0.5F && std::fabs(it->second.Min.y - a.Min.y) <= 0.5F &&
                                std::fabs(it->second.Max.x - a.Max.x) <= 0.5F && std::fabs(it->second.Max.y - a.Max.y) <= 0.5F;
            diag::detail("Knopftest: dock2mass: " + k + " alt " + dock1::rechteck(a) + " neu " +
                         (it == neu->end() ? std::string("fehlt") : dock1::rechteck(it->second)) + (gleich ? "" : "  ABWEICHUNG"));
            if (!gleich) { ++abweichungen; }
        }
        return !alt->empty() && abweichungen == 0 && alt->size() == neu->size();
    }));
    // Im DockSpace: Actions als Reiter zu Script Flow (fremde Stelle), dann
    // Zuruecksetzen = wieder die Standardmasse; der linke Griff wie vorher.
    add(tu("dock2mass: Events", [] { g_app->leftMode = 0; }));
    add(stabil("dock2mass: Events", 10));
    add(pruefe("dock2mass: Events steht in der Standardanordnung", [] { return dockraumStandard(0); }));
    add(tu("dock2mass: Actions als Reiter zu Script Flow", [] {
        const ImGuiWindow* w = dock1::panelFenster("ereignisse.aktionen");
        if (w != nullptr) { ImGui::DockBuilderDockWindow(w->Name, dockraumHeim("ereignisse.fluss")); }
    }));
    add(stabil("dock2mass: Reiter", 15));
    add(pruefe("dock2mass: Actions und Script Flow in einem Knoten (Reiter), keine Standardanordnung mehr", [] {
        const ImGuiWindow* a = dock1::panelFenster("ereignisse.aktionen");
        const ImGuiWindow* f = dock1::panelFenster("ereignisse.fluss");
        const bool reiter = a != nullptr && f != nullptr && a->DockNode != nullptr && a->DockNode == f->DockNode && a->DockNode->Windows.Size == 2 &&
                            a->DockNode->HostWindow != nullptr && a->DockNode->HostWindow->RootWindowDockTree == ImGui::FindWindowByName("###main");
        diag::detail(std::string("Knopftest: dock2mass: Reiter ") + (reiter ? "ja" : "nein") + ", Standard " + (dockraumStandard(0) ? "ja" : "nein"));
        return reiter && !dockraumStandard(0);
    }));
    add(tu("dock2mass: Anordnung zuruecksetzen (Menue Fenster)", [] { dock::zuruecksetzen(0); }));
    add(stabil("dock2mass: zurueckgesetzt", 15));
    add(pruefe("dock2mass: nach dem Zuruecksetzen Standardanordnung mit den alten Massen", [=] {
        const dock2::Masse m = dock2::messen();
        int gleich = 0;
        int gesamt = 0;
        for (const auto& [k, a] : *alt) {
            if (k.empty() || k[0] != '0') { continue; }
            ++gesamt;
            const auto it = m.find(k);
            if (it != m.end() && std::fabs(it->second.Min.x - a.Min.x) <= 0.5F && std::fabs(it->second.Max.x - a.Max.x) <= 0.5F &&
                std::fabs(it->second.Min.y - a.Min.y) <= 0.5F && std::fabs(it->second.Max.y - a.Max.y) <= 0.5F) {
                ++gleich;
            }
        }
        diag::detail("Knopftest: dock2mass: nach Zuruecksetzen " + std::to_string(gleich) + "/" + std::to_string(gesamt) + " Kindfenster gleich");
        return dockraumStandard(0) && gesamt > 0 && gleich == gesamt;
    }));
    auto breite = std::make_shared<std::array<float, 3>>();
    add(tu("dock2mass: Breiten merken", [=] {
        const dock2::Masse m = dock2::messen();
        (*breite)[0] = m.count("0events") != 0U ? m.at("0events").GetWidth() : 0.0F;
        (*breite)[1] = m.count("0##flowarea") != 0U ? m.at("0##flowarea").GetWidth() : 0.0F;
        (*breite)[2] = m.count("0buttons") != 0U ? m.at("0buttons").GetWidth() : 0.0F;
    }));
    add(ziehenHalten("dock2mass: linker Griff 80 nach rechts", [] { return ImVec2{g_app->teilerX[0], g_app->teilerY}; }, 80.0F, 0.0F, 20));
    add(pause(0.3));
    add(pruefe("dock2mass: linker Griff im DockSpace - Ereignisliste 80 breiter, Script Flow 80 schmaler, Actions bleibt", [=] {
        const dock2::Masse m = dock2::messen();
        if (m.count("0events") == 0U || m.count("0##flowarea") == 0U || m.count("0buttons") == 0U) { return false; }
        const float e = m.at("0events").GetWidth() - (*breite)[0];
        const float f = m.at("0##flowarea").GetWidth() - (*breite)[1];
        const float k = m.at("0buttons").GetWidth() - (*breite)[2];
        diag::detail("Knopftest: dock2mass: Griff: Ereignisliste " + std::to_string(e) + ", Script Flow " + std::to_string(f) + ", Actions " +
                     std::to_string(k));
        return std::fabs(e - 80.0F) <= 3.0F && std::fabs(f + 80.0F) <= 3.0F && std::fabs(k) <= 0.5F;
    }));
    add(ziehenHalten("dock2mass: linker Griff 80 zurueck", [] { return ImVec2{g_app->teilerX[0], g_app->teilerY}; }, -80.0F, 0.0F, 20));
    add(pause(0.3));
    add(tu("dock2mass: Schalter zurueck", [=] { dockraumAn() = vorher; }));
    return s;
}
