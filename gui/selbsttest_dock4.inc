// --- dock4: Map Editor und Menu Editor im DockSpace (BHED_EDITORTEST=dock4) -----
//
// Prueft im laufenden Programm (dockvorgabe.h, app_menue_dock.inc):
//   - Map Editor: je NRC-Anordnung (Regular, Regular Left, Split, Split frei
//     belegt, eingebauter Gruppendialog, MaximizeView, ein Feld in einem eigenen
//     Fenster, Griff gezogen) stehen alle Felder im DockSpace Punkt fuer Punkt
//     wie ohne - im selben Lauf, der Schalter wird umgelegt. Ein Feld als Reiter
//     auf einem anderen: keine Vorgabe mehr, NRCs Griffe fehlen; Zuruecksetzen =
//     wieder die Masse.
//   - Menu Editor: "Reiter rechts" und "Text unten" ebenso; der Text in einem
//     eigenen Fenster - die obere Reihe nimmt die ganze Hoehe.
//   - Randfall Schalter an/aus/an: Baum gespeichert, ohne DockSpace ein Panel
//     herausgenommen (Liste aus dock1), wieder an.
//   - Die Hover-Linie der alten Griffe von Events ist ueber den Panels zu sehen
//     (Foto des Hauptfensters).
//   - ImGuis Dreieck oben links: unter der Maus ueber dem Inhalt gezeichnet, ein
//     Klick darauf gilt nur ihm (nicht zugleich der Ansicht darunter), Ziehen
//     nimmt das Panel heraus.
//   - Dateien: jedes Windows-Fenster nimmt Dateien an (WS_EX_ACCEPTFILES - das
//     braucht der Explorer); ein Modell auf die XY-Ansicht in einem eigenen
//     Fenster abgelegt landet unter dem Zeiger.
namespace dock4 {

using Masse = std::map<std::string, ImRect>;

// Die Kindfenster der Felder und Spalten im Hauptfenster (auf die Ecke des
// Hauptfensters bezogen). Panels in eigenen Fenstern zaehlen nicht.
Masse messen(const std::string& wo) {
    static const char* const kNamen[] = {"##feldXY_",       "##feldXZ_", "##feldYZ_",   "##feldKamera_", "##feldKonsole_", "##feldTexturen_",
                                         "##feldGruppe_",   "##links_",  "##mitte_",    "##rechts_",     "##codeBereich_"};
    Masse m;
    const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
    const ImVec2 o = ImGui::GetMainViewport()->Pos;
    for (const ImGuiWindow* w : GImGui->Windows) {
        if (!w->WasActive || w->Hidden || haupt == nullptr || w->RootWindowDockTree != haupt) { continue; }
        const std::string n = w->Name;
        const std::size_t strich = n.rfind('/');
        if (strich == std::string::npos) { continue; }
        for (const char* k : kNamen) {
            if (n.compare(strich + 1, std::strlen(k), k) != 0) { continue; }
            m[wo + " " + std::string(k, std::strlen(k) - 1)] =
                ImRect{ImVec2{w->Pos.x - o.x, w->Pos.y - o.y}, ImVec2{w->Pos.x + w->Size.x - o.x, w->Pos.y + w->Size.y - o.y}};
        }
    }
    return m;
}

bool gleich(const ImRect& a, const ImRect& b) {
    return std::fabs(a.Min.x - b.Min.x) <= 0.5F && std::fabs(a.Min.y - b.Min.y) <= 0.5F && std::fabs(a.Max.x - b.Max.x) <= 0.5F &&
           std::fabs(a.Max.y - b.Max.y) <= 0.5F;
}

// Jedes Kindfenster ohne DockSpace (alt) gegen dasselbe im DockSpace (neu).
bool vergleichen(const Masse& alt, const Masse& neu) {
    int abweichungen = 0;
    for (const auto& [k, a] : alt) {
        const auto it = neu.find(k);
        const bool ok = it != neu.end() && gleich(a, it->second);
        diag::detail("Knopftest: dock4: " + k + " ohne " + dock1::rechteck(a) + ", im DockSpace " +
                     (it == neu.end() ? std::string("fehlt") : dock1::rechteck(it->second)) + (ok ? "" : "  ABWEICHUNG"));
        if (!ok) { ++abweichungen; }
    }
    for (const auto& [k, n] : neu) {
        if (alt.count(k) == 0U) {
            diag::detail("Knopftest: dock4: " + k + " nur im DockSpace " + dock1::rechteck(n) + "  ABWEICHUNG");
            ++abweichungen;
        }
    }
    return !alt.empty() && abweichungen == 0;
}

// Die Anordnungen des Map Editors, die verglichen werden.
struct KartenArt {
    const char* name;
    karte::befehle::Anordnung anordnung;
    bool gruppe;        // Gruppendialog eingebaut (drittes Kind von m_hSplit)
    bool frei;          // Split-Plaetze frei belegt (Stufe 8b)
    bool maximiert;     // MaximizeView ueber XY
    const char* heraus; // dieses Feld in einem eigenen Fenster
};

const std::vector<KartenArt>& kartenArten() {
    using karte::befehle::Anordnung;
    static const std::vector<KartenArt> a = {
        {"Regular", Anordnung::Regular, false, false, false, nullptr},
        {"RegularLeft", Anordnung::RegularLeft, false, false, false, nullptr},
        {"Split", Anordnung::Split, false, false, false, nullptr},
        {"SplitFrei", Anordnung::Split, false, true, false, nullptr},
        {"RegularGruppe", Anordnung::Regular, true, false, false, nullptr},
        {"SplitGruppe", Anordnung::Split, true, false, false, nullptr},
        {"RegularMaximiert", Anordnung::Regular, false, false, true, nullptr},
        {"RegularOhneXY", Anordnung::Regular, false, false, false, "karte.xy"},
        {"SplitOhneKamera", Anordnung::Split, false, false, false, "karte.kamera"},
        {"RegularLeftOhneTexturen", Anordnung::RegularLeft, false, false, false, "karte.texturen"},
    };
    return a;
}

// Was der Test am Map Editor verstellt - am Ende zurueck.
struct KartenStand {
    karte::befehle::Anordnung anordnung = karte::befehle::Anordnung::Regular;
    bool gruppe = false;
    bool frei = false;
    std::array<int, 4> belegung{};
    std::vector<int> h, v, v2;
};

void karteSetzen(const KartenArt& k, const KartenStand& alt) {
    namespace ke = karteneditor;
    ke::Editor& e = ke::editor();
    if (e.maximiert) { ke::maximierenUmschalten(); }
    e.anordnung = k.anordnung;
    e.gruppenDialogEingebaut = k.gruppe;
    e.splitFrei = k.frei;
    e.splitBelegung = k.frei ? std::array<int, 4>{1, 3, 0, 2} : alt.belegung;
    e.hGroessen = alt.h;
    e.vGroessen = alt.v;
    e.v2Groessen = alt.v2;
    e.teilerFuer = -1;
}

// Rahmen eines Feldes im letzten Bild (Bildschirmpunkte).
const karteneditor::Editor::RahmenImBild* rahmen(const char* feld) {
    for (const auto& r : karteneditor::editor().rahmenImBild) {
        if (r.feld == feld) { return &r; }
    }
    return nullptr;
}

// Ein Foto des Hauptfensters (render::fotoAnfordern, 32 Bit, von oben).
struct Bild {
    int breite = 0;
    int hoehe = 0;
    std::vector<unsigned char> bgra;
    [[nodiscard]] bool da() const { return breite > 0 && hoehe > 0; }
    [[nodiscard]] const unsigned char* punkt(int x, int y) const {
        x = std::clamp(x, 0, breite - 1);
        y = std::clamp(y, 0, hoehe - 1);
        return &bgra[(static_cast<std::size_t>(y) * static_cast<std::size_t>(breite) + static_cast<std::size_t>(x)) * 4U];
    }
};

Bild bildLesen(const std::string& pfad) {
    Bild b;
    std::ifstream f(::bhed::utf8Pfad(pfad), std::ios::binary);
    unsigned char kopf[54] = {};
    if (!f.read(reinterpret_cast<char*>(kopf), sizeof(kopf))) { return b; }
    const auto i32 = [&](int at) {
        return static_cast<std::int32_t>(static_cast<std::uint32_t>(kopf[at]) | (static_cast<std::uint32_t>(kopf[at + 1]) << 8U) |
                                         (static_cast<std::uint32_t>(kopf[at + 2]) << 16U) | (static_cast<std::uint32_t>(kopf[at + 3]) << 24U));
    };
    const int w = i32(18);
    const int h = -i32(22);
    if (w <= 0 || h <= 0 || kopf[28] != 32) { return b; }
    b.bgra.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
    if (!f.read(reinterpret_cast<char*>(b.bgra.data()), static_cast<std::streamsize>(b.bgra.size()))) { return Bild{}; }
    b.breite = w;
    b.hoehe = h;
    return b;
}

int abstand(const unsigned char* a, const unsigned char* b) { return std::abs(a[0] - b[0]) + std::abs(a[1] - b[1]) + std::abs(a[2] - b[2]); }

// Ein Foto schreiben und warten, bis es da ist.
Schritt fotoNach(const std::string& was, const std::string& datei, std::function<void()> jedesBild) {
    return {"dock4: Foto " + was, [=](int b) {
                if (jedesBild) { jedesBild(); }
                if (b == 3) { render::fotoAnfordern(platform::executableDirectory() + "\\" + datei); }
                return b >= 8;
            }};
}

}  // namespace dock4

std::vector<Schritt> dock4Schritte() {
    namespace ke = karteneditor;
    using karte::befehle::Anordnung;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto alt = std::make_shared<dock4::Masse>();
    auto neu = std::make_shared<dock4::Masse>();
    auto stand = std::make_shared<dock4::KartenStand>();
    const bool vorher = dockraumAn();

    // Menu Editor ohne Menue: kein DockSpace, aber ein Panel laesst sich herausnehmen
    // (es steht mit dem Hinweis in einem eigenen Fenster) und spaeter andocken.
    add(tu("dock4: Menu Editor ohne Menue im DockSpace, Flaeche herausnehmen", [] {
        dockraumAn() = true;
        g_app->leftMode = 4;
        dock::herausnehmen("menue.flaeche");
    }));
    add(stabil("dock4: Menu Editor ohne Menue", 20));
    add(pruefe("dock4: ohne Menue - die Flaeche steht in einem eigenen Fenster", [] {
        const ImGuiWindow* w = dock1::panelFenster("menue.flaeche");
        return menueeditor::stand().dokumente == 0 && w != nullptr && w->WasActive && dock::draussen("menue.flaeche");
    }));
    add(tu("dock4: Ausgangslage - alle Anordnungen zurueck, ein Menue offen, Map Editor", [stand] {
        for (int b = 0; b < dock::kBereiche; ++b) { dock::zuruecksetzen(b); }
        (void)menueeditor::oeffneText("dock4.menu",
                                      "menuDef\n{\n  name \"dock4\"\n  rect 0 0 640 480\n  itemDef\n  {\n    name \"knopf\"\n"
                                      "    rect 100 100 200 40\n    type ITEM_TYPE_BUTTON\n    text \"Hallo\"\n    visible 1\n  }\n}\n");
        g_app->leftMode = 3;
        const ke::Editor& e = ke::editor();
        stand->anordnung = e.anordnung;
        stand->gruppe = e.gruppenDialogEingebaut;
        stand->frei = e.splitFrei;
        stand->belegung = e.splitBelegung;
        stand->h = e.hGroessen;
        stand->v = e.vGroessen;
        stand->v2 = e.v2Groessen;
    }));
    add(warteBis("dock4: Spiel bereit (Karten-Editor)", [] { return ke::spielBereit(); }, 6000));
    add(tu("dock4: mit Menue - Flaeche andocken", [] {
        g_app->leftMode = 4;
        dock::andocken("menue.flaeche");
    }));
    add(stabil("dock4: Flaeche angedockt", 20));
    add(pruefe("dock4: mit Menue - die Flaeche ist wieder an ihrem Platz, Vorgabe", [] {
        return dock::zuhause("menue.flaeche") && dockvorgabe::standard(4);
    }));
    add(tu("dock4: Map Editor", [] { g_app->leftMode = 3; }));

    // --- 1. und 2.: dieselben Masse ohne und mit DockSpace ---------------------
    for (const int runde : {0, 1}) {
        auto& ziel = runde == 0 ? alt : neu;
        const std::string wie = runde == 0 ? "ohne DockSpace" : "im DockSpace";
        add(tu("dock4: Schalter " + std::string(runde == 0 ? "aus" : "an"), [runde] {
            dockraumAn() = runde == 1;
            g_app->leftMode = 3;
        }));
        for (const dock4::KartenArt& k : dock4::kartenArten()) {
            const std::string name = k.name;
            add(tu("dock4: Map Editor " + name + " " + wie, [k, stand] { dock4::karteSetzen(k, *stand); }));
            add(stabil("dock4: " + name, 20));
            if (k.maximiert) {
                add(tu("dock4: MaximizeView ueber XY", [] {
                    ke::Editor& e = ke::editor();
                    e.mausX = 40;
                    e.mausY = 40;
                    ke::maximierenUmschalten();
                }));
                add(stabil("dock4: maximiert", 15));
            }
            if (k.heraus != nullptr) {
                const std::string id = k.heraus;
                add(tu("dock4: " + id + " in ein eigenes Fenster", [id] { dock::herausnehmen(id.c_str()); }));
                add(warteBis("dock4: " + id + " draussen", [id] { return dock::draussen(id.c_str()); }, 60));
                add(stabil("dock4: " + id + " draussen", 15));
            }
            add(tu("dock4: messen " + name, [ziel, name] {
                const dock4::Masse m = dock4::messen(name);
                ziel->insert(m.begin(), m.end());
            }));
            if (runde == 1) {
                add(pruefe("dock4: " + name + " - der Baum entspricht NRCs Anordnung (Griffe und Masse der Teiler)",
                           [] { return dockvorgabe::standard(3); }));
            }
            if (k.heraus != nullptr) {
                const std::string id = k.heraus;
                add(tu("dock4: " + id + " zurueck", [id] { dock::andocken(id.c_str()); }));
                add(warteBis("dock4: " + id + " wieder an seinem Platz", [id] { return dock::zuhause(id.c_str()); }, 60));
            }
            if (k.maximiert) { add(tu("dock4: MaximizeView zurueck", [] { ke::maximierenUmschalten(); })); }
        }
        // Griff zwischen XY und Konsole (m_vSplit) ziehen - wie QSplitterHandle.
        add(tu("dock4: Regular fuer den Griff " + wie, [stand] { dock4::karteSetzen(dock4::kartenArten()[0], *stand); }));
        add(stabil("dock4: Regular", 20));
        add(ziehenHalten("dock4: Griff XY/Konsole 60 nach oben " + wie,
                         [] {
                             const auto* xy = dock4::rahmen("XY");
                             const auto* ko = dock4::rahmen("Konsole");
                             return xy != nullptr && ko != nullptr ? ImVec2{(xy->x0 + xy->x1) * 0.5F, (xy->y1 + ko->y0) * 0.5F} : ImVec2{5, 5};
                         },
                         0.0F, -60.0F, 5));
        add(stabil("dock4: Griff gezogen", 15));
    if (runde == 1) {
        add({"dock4: Maus auf den Griff XY/Konsole", [](int b) {
                 const auto* xy = dock4::rahmen("XY");
                 const auto* ko = dock4::rahmen("Konsole");
                 setzeMaus(xy != nullptr && ko != nullptr ? ImVec2{(xy->x0 + xy->x1) * 0.5F, (xy->y1 + ko->y0) * 0.5F} : ImVec2{5, 5});
                 return b >= 6;
             }});
        add(pruefe("dock4: ueber NRCs Griff im DockSpace der Zeiger zum Ziehen (wie ohne)", [] { return ImGui::GetMouseCursor() == ImGuiMouseCursor_ResizeNS; }));
    }
        add(tu("dock4: messen RegularGriff", [ziel] {
            const dock4::Masse m = dock4::messen("RegularGriff");
            ziel->insert(m.begin(), m.end());
        }));
        // Menu Editor in beiden Layouts.
        for (const bool unten : {false, true}) {
            const std::string name = unten ? "MenuTextUnten" : "MenuReiterRechts";
            add(tu("dock4: " + name + " " + wie, [unten] {
                g_app->leftMode = 4;
                g_app->settings.menueTextUnten = unten;
            }));
            add(stabil("dock4: " + name, 25));
            add(tu("dock4: messen " + name, [ziel, name] {
                const dock4::Masse m = dock4::messen(name);
                ziel->insert(m.begin(), m.end());
            }));
            if (runde == 1) {
                add(pruefe("dock4: " + name + " - der Baum entspricht der Vorgabe", [] { return dockvorgabe::standard(4); }));
                add({"dock4: Maus auf den Teiler links der Flaeche", [](int b) {
                         const ImGuiWindow* w = dock1::panelFenster("menue.flaeche");
                         setzeMaus(w != nullptr ? ImVec2{w->Pos.x - 3.0F, w->Pos.y + w->Size.y * 0.5F} : ImVec2{5, 5});
                         return b >= 6;
                     }});
                add(pruefe("dock4: " + name + " - ueber dem Teiler im DockSpace der Zeiger zum Ziehen (wie ohne)",
                           [] { return ImGui::GetMouseCursor() == ImGuiMouseCursor_ResizeEW; }));
            }
        }
        add(tu("dock4: Menu Editor wieder Reiter rechts", [] { g_app->settings.menueTextUnten = false; }));
    }
    add(pruefe("dock4: Map Editor (10 Anordnungen, Griff) und Menu Editor (2 Layouts) im DockSpace Punkt fuer Punkt wie ohne",
               [alt, neu] { return dock4::vergleichen(*alt, *neu); }));

    // --- Reiter: keine Vorgabe mehr, Zuruecksetzen = wieder die Masse ----------
    add(tu("dock4: Map Editor Regular im DockSpace", [stand] {
        dockraumAn() = true;
        g_app->leftMode = 3;
        dock4::karteSetzen(dock4::kartenArten()[0], *stand);
    }));
    add(stabil("dock4: Regular", 20));
    add(tu("dock4: XY abdocken", [] { dock::herausnehmen("karte.xy"); }));
    add(stabil("dock4: XY schwebt", 5));
    add(tu("dock4: XY als Reiter auf die Kamera (wie ImGuis Ziehen)", [] {
        ImGuiWindow* xy = dock1::panelFenster("karte.xy");
        ImGuiWindow* kam = dock1::panelFenster("karte.kamera");
        if (xy != nullptr && kam != nullptr && kam->DockNode != nullptr) {
            ImGui::DockContextQueueDock(GImGui, kam->DockNode->HostWindow, kam->DockNode, xy, ImGuiDir_None, 0.0F, false);
        }
    }));
    add(stabil("dock4: Reiter", 15));
    add(pruefe("dock4: XY und Kamera ein Knoten im Hauptfenster - keine Vorgabe mehr, NRCs Griffe fehlen, ImGuis Teiler gelten", [] {
        bool griffe = false;   // NRCs Teiler-Griffe ("Griff:h:", "Griff:v:", "Griff:v2:"), nicht die der Werkzeugleisten
        for (const auto& f : ke::editor().felderImBild) {
            if (f.name.rfind("Griff:h:", 0) == 0 || f.name.rfind("Griff:v:", 0) == 0 || f.name.rfind("Griff:v2:", 0) == 0) { griffe = true; }
        }
        const bool reiter = dock1::reiterImHauptfenster("karte.xy", "karte.kamera");
        const ImGuiWindow* xy = dock1::panelFenster("karte.xy");
        const ImGuiWindow* kam = dock1::panelFenster("karte.kamera");
        diag::detail(std::string("Knopftest: dock4: Reiter ") + (reiter ? "ja" : "nein") + " (XY-Knoten " +
                     std::to_string(xy != nullptr && xy->DockNode != nullptr ? xy->DockNode->ID : 0U) + ", Kamera-Knoten " +
                     std::to_string(kam != nullptr && kam->DockNode != nullptr ? kam->DockNode->ID : 0U) + "), Vorgabe " +
                     (dockvorgabe::standard(3) ? "ja" : "nein") + ", Griffe " + (griffe ? "ja" : "nein"));
        return reiter && !dockvorgabe::standard(3) && !griffe;
    }));
    add(tu("dock4: Anordnung zuruecksetzen (Menue Fenster)", [] { dock::zuruecksetzen(3); }));
    add(stabil("dock4: zurueckgesetzt", 15));
    add(pruefe("dock4: zurueckgesetzt - Vorgabe und die Masse von Regular wie ohne DockSpace", [alt] {
        const dock4::Masse m = dock4::messen("Regular");
        dock4::Masse soll;
        for (const auto& [k, r] : *alt) {
            if (k.rfind("Regular ", 0) == 0) { soll[k] = r; }
        }
        return dockvorgabe::standard(3) && dock4::vergleichen(soll, m);
    }));

    // --- Text unten: der Text in einem eigenen Fenster ---------------------------
    add(tu("dock4: Menu Editor, Text unten, im DockSpace", [] {
        g_app->leftMode = 4;
        g_app->settings.menueTextUnten = true;
    }));
    add(stabil("dock4: Text unten", 20));
    add(tu("dock4: Text in ein eigenes Fenster", [] { dock::herausnehmen("menue.text"); }));
    add(warteBis("dock4: Text draussen", [] { return dock::draussen("menue.text"); }, 60));
    add(stabil("dock4: Text draussen", 15));
    add(pruefe("dock4: ohne den Text reicht die obere Reihe bis unten, der Text zeichnet im eigenen Fenster", [alt] {
        const dock4::Masse m = dock4::messen("x");
        const auto links = m.find("x ##links");
        const auto vorher = alt->find("MenuTextUnten ##links");
        const ImGuiWindow* t = dock1::panelFenster("menue.text");
        bool code = false;
        for (const ImGuiWindow* w : GImGui->Windows) {
            if (w->WasActive && w->RootWindowDockTree == t && std::strstr(w->Name, "/##codeBereich_") != nullptr) { code = true; }
        }
        diag::detail("Knopftest: dock4: Text draussen - Bausteine " + (links != m.end() ? dock1::rechteck(links->second) : std::string("fehlt")) +
                     ", vorher " + (vorher != alt->end() ? dock1::rechteck(vorher->second) : std::string("fehlt")));
        return links != m.end() && vorher != alt->end() && links->second.GetHeight() > vorher->second.GetHeight() + 50.0F && code &&
               dockvorgabe::standard(4);
    }));
    add(tu("dock4: Text zurueck", [] { dock::andocken("menue.text"); }));
    add(warteBis("dock4: Text wieder unten", [] { return dock::zuhause("menue.text"); }, 60));
    add(stabil("dock4: Text unten wieder", 15));
    add(pruefe("dock4: Text zurueck - Masse von Text unten wie ohne DockSpace", [alt] {
        const dock4::Masse m = dock4::messen("MenuTextUnten");
        dock4::Masse soll;
        for (const auto& [k, r] : *alt) {
            if (k.rfind("MenuTextUnten ", 0) == 0) { soll[k] = r; }
        }
        return dockvorgabe::standard(4) && dock4::vergleichen(soll, m);
    }));
    add(tu("dock4: Menu Editor wieder Reiter rechts", [] { g_app->settings.menueTextUnten = false; }));

    // --- 3. Randfall Schalter an/aus/an mit Liste aus dock1 und Baum -----------
    add(tu("dock4: Mission im DockSpace, Standardanordnung", [] {
        dockraumAn() = true;
        g_app->leftMode = 1;
        dock::zuruecksetzen(1);
    }));
    add(stabil("dock4: Mission", 20));
    add(pruefe("dock4: Mission in der Standardanordnung", [] { return dockraumStandard(1); }));
    add(tu("dock4: Schalter aus, Ebenenliste herausnehmen (Liste aus dock1)", [] {
        dockraumAn() = false;
        dock::herausnehmen("mission.ebenen");
    }));
    add(stabil("dock4: Ebenenliste ohne DockSpace draussen", 20));
    add(tu("dock4: Schalter wieder an - wie nach einem Neustart den gespeicherten Baum lesen", [] {
        dock::jetztMerken();
        dockraumAn() = true;
        dockraumNeuLesen(1);
    }));
    add(stabil("dock4: Schalter an", 20));
    add(pruefe("dock4: an/aus/an - Ebenenliste schwebt, die Liste aus dock1 ist erledigt (leer)", [] {
        diag::detail(std::string("Knopftest: dock4: Ebenenliste draussen ") + (dock::draussen("mission.ebenen") ? "ja" : "nein") + ", Liste " +
                     (dock::listeDraussen(1) ? "noch da" : "leer"));
        return dock::draussen("mission.ebenen") && !dock::listeDraussen(1);
    }));
    add(tu("dock4: Ebenenliste andocken", [] { dock::andocken("mission.ebenen"); }));
    add(stabil("dock4: angedockt", 20));
    add(tu("dock4: Script Flow heraus und wieder zurueck (baut die Standardanordnung neu)", [] { dock::herausnehmen("mission.fluss"); }));
    add(warteBis("dock4: Script Flow draussen", [] { return dock::draussen("mission.fluss"); }, 60));
    add(tu("dock4: Script Flow andocken", [] { dock::andocken("mission.fluss"); }));
    add(stabil("dock4: Script Flow zurueck", 20));
    add(pruefe("dock4: an/aus/an - Ebenenliste und Script Flow im Hauptfenster, Standardanordnung", [] {
        diag::detail(std::string("Knopftest: dock4: Ebenenliste ") + (dock::zuhause("mission.ebenen") ? "zuhause" : "draussen") + ", Script Flow " +
                     (dock::zuhause("mission.fluss") ? "zuhause" : "draussen") + ", Standard " + (dockraumStandard(1) ? "ja" : "nein"));
        return dock::zuhause("mission.ebenen") && dock::zuhause("mission.fluss") && dockraumStandard(1);
    }));

    // --- 3. Hover-Linie der alten Griffe von Events -----------------------------
    add(tu("dock4: Events im DockSpace", [] {
        g_app->leftMode = 0;
        dock::zuruecksetzen(0);
    }));
    add(stabil("dock4: Events", 20));
    const auto griff = [] { return ImVec2{g_app->teilerX[0], g_app->teilerY}; };
    add(dock4::fotoNach("ohne Hover", "dock4_ohne.bmp", [] { setzeMaus(ImVec2{g_app->teilerX[0] + 200.0F, g_app->teilerY}); }));
    add(dock4::fotoNach("Maus auf dem linken Griff", "dock4_linie.bmp", [griff] { setzeMaus(griff()); }));
    add(pruefe("dock4: Hover-Linie des linken Griffs ueber den Panels in voller Breite zu sehen", [] {
        const dock4::Bild a = dock4::bildLesen(platform::executableDirectory() + "\\dock4_ohne.bmp");
        const dock4::Bild b = dock4::bildLesen(platform::executableDirectory() + "\\dock4_linie.bmp");
        if (!a.da() || !b.da() || a.breite != b.breite || a.hoehe != b.hoehe) { return false; }
        const ImVec2 o = ImGui::GetMainViewport()->Pos;
        const int x = static_cast<int>(std::floor(g_app->teilerX[0] - o.x));
        int zeilen = 0;
        int sichtbar = 0;
        for (float y = g_app->tabelleOben + 30.0F; y < g_app->tabelleUnten - 30.0F; y += 12.0F) {
            const int yy = static_cast<int>(y - o.y);
            int anders = 0;
            for (int dx = -3; dx <= 3; ++dx) {
                if (dock4::abstand(a.punkt(x + dx, yy), b.punkt(x + dx, yy)) > 40) { ++anders; }
            }
            ++zeilen;
            if (anders >= 2) { ++sichtbar; }
        }
        diag::detail("Knopftest: dock4: Hover-Linie in " + std::to_string(sichtbar) + " von " + std::to_string(zeilen) +
                     " Zeilen mindestens 2 Punkte breit");
        return zeilen > 10 && sichtbar * 10 >= zeilen * 9;
    }));

    // --- 3. ImGuis Dreieck oben links ------------------------------------------
    add(tu("dock4: Map Editor Regular, Kamera aktiv", [stand] {
        g_app->leftMode = 3;
        dock4::karteSetzen(dock4::kartenArten()[0], *stand);
    }));
    add(stabil("dock4: Regular", 20));
    add(klickAn("in die Kamera", [] {
        const auto* k = dock4::rahmen("Kamera");
        return k != nullptr ? ImVec2{(k->x0 + k->x1) * 0.5F, (k->y0 + k->y1) * 0.5F} : ImVec2{5, 5};
    }));
    add(pruefe("dock4: Kamera ist das aktive Feld", [] { return ke::editor().aktivesFeld == 3; }));
    const auto ecke = [] {
        const ImGuiWindow* w = dock1::panelFenster("karte.xy");
        return w != nullptr && w->DockNode != nullptr ? ImVec2{w->DockNode->Pos.x + 3.0F, w->DockNode->Pos.y + 3.0F} : ImVec2{5, 5};
    };
    const auto kameraMitte = [] {
        const auto* k = dock4::rahmen("Kamera");
        return k != nullptr ? ImVec2{(k->x0 + k->x1) * 0.5F, (k->y0 + k->y1) * 0.5F} : ImVec2{5, 5};
    };
    add(dock4::fotoNach("Maus nicht auf der Ecke", "dock4_ecke_ohne.bmp", [kameraMitte] { setzeMaus(kameraMitte()); }));
    add(dock4::fotoNach("Maus auf der Ecke von XY", "dock4_ecke.bmp", [ecke] { setzeMaus(ecke()); }));
    add(pruefe("dock4: Dreieck unter der Maus ueber der Ansicht zu sehen (Foto), sonst nicht", [] {
        const dock4::Bild a = dock4::bildLesen(platform::executableDirectory() + "\\dock4_ecke_ohne.bmp");
        const dock4::Bild b = dock4::bildLesen(platform::executableDirectory() + "\\dock4_ecke.bmp");
        const ImGuiWindow* w = dock1::panelFenster("karte.xy");
        if (!a.da() || !b.da() || w == nullptr || w->DockNode == nullptr) { return false; }
        const ImVec2 o = ImGui::GetMainViewport()->Pos;
        const int x0 = static_cast<int>(w->DockNode->Pos.x - o.x);
        const int y0 = static_cast<int>(w->DockNode->Pos.y - o.y);
        const int gross = static_cast<int>(std::trunc(GImGui->FontSize * 0.70F));
        int anders = 0;
        int flaeche = 0;
        for (int y = 0; y < gross; ++y) {
            for (int x = 0; x + y < gross; ++x) {   // die Dreiecksflaeche
                ++flaeche;
                if (dock4::abstand(a.punkt(x0 + x, y0 + y), b.punkt(x0 + x, y0 + y)) > 30) { ++anders; }
            }
        }
        diag::detail("Knopftest: dock4: Dreieck - " + std::to_string(anders) + " von " + std::to_string(flaeche) + " Punkten anders");
        return flaeche > 10 && anders * 2 >= flaeche;
    }));
    add(klickAn("auf das Dreieck von XY", ecke));
    add(stabil("dock4: nach dem Klick", 10));
    add(pruefe("dock4: Klick auf das Dreieck zeigt die Reiter und gilt nicht zugleich der Ansicht (Kamera bleibt aktiv)", [] {
        const ImGuiWindow* w = dock1::panelFenster("karte.xy");
        const bool reiter = w != nullptr && w->DockNode != nullptr && !w->DockNode->IsHiddenTabBar();
        diag::detail(std::string("Knopftest: dock4: Reiterleiste ") + (reiter ? "sichtbar" : "versteckt") + ", aktives Feld " +
                     std::to_string(ke::editor().aktivesFeld));
        return reiter && ke::editor().aktivesFeld == 3;
    }));
    add(tu("dock4: Reiterleiste wieder verstecken", [] {
        ImGuiWindow* w = dock1::panelFenster("karte.xy");
        if (w != nullptr && w->DockNode != nullptr) { w->DockNode->WantHiddenTabBarToggle = true; }
    }));
    add(stabil("dock4: versteckt", 10));
    add(ziehenHalten("dock4: vom Dreieck von XY wegziehen", ecke, 260.0F, 180.0F, 5));
    add(stabil("dock4: gezogen", 15));
    add(pruefe("dock4: Ziehen vom Dreieck nimmt XY heraus (eigenes Fenster), der Platz geht an die Konsole", [] {
        bool xyDa = false;
        for (const auto& f : ke::editor().felderImBild) {
            if (f.name == "XY" && f.b > 0 && f.h > 0) { xyDa = true; }
        }
        return dock::draussen("karte.xy") && !xyDa;
    }));
    add(tu("dock4: XY zurueck", [] { dock::andocken("karte.xy"); }));
    add(warteBis("dock4: XY wieder an seinem Platz", [] { return dock::zuhause("karte.xy"); }, 60));
    add(stabil("dock4: XY zurueck", 10));

    // --- 3. Dateien auf eigene Fenster ------------------------------------------
    // Das Hauptfenster kleiner (wie dock1), damit daneben Platz fuer ein eigenes Windows-Fenster ist.
    auto lage = std::make_shared<std::array<int, 4>>();
    add(tu("dock4: Hauptfenster verkleinern", [lage] {
        void* h = ImGui::GetMainViewport()->PlatformHandle;
        (void)platform::fensterLage(h, (*lage)[0], (*lage)[1], (*lage)[2], (*lage)[3]);
        const ImGuiPlatformMonitor& m = dock1::hauptMonitor();
        (void)platform::fensterLageSetzen(h, static_cast<int>(m.WorkPos.x) + 60, static_cast<int>(m.WorkPos.y) + 40,
                                          static_cast<int>(m.WorkSize.x * 0.6F), static_cast<int>(m.WorkSize.y * 0.75F));
    }));
    add(stabil("dock4: Hauptfenster verkleinert", 20));
    add(tu("dock4: XY in ein eigenes Windows-Fenster neben dem Hauptfenster", [] { dock::herausnehmen("karte.xy"); }));
    add(warteBis("dock4: XY draussen", [] { return dock::draussen("karte.xy"); }, 60));
    add(tu("dock4: XY neben das Hauptfenster", [] {
        const ImGuiPlatformMonitor& m = dock1::hauptMonitor();
        dock1::setze("karte.xy", dock1::nebenHaupt(0.0F), ImVec2{m.WorkSize.x * 0.3F, m.WorkSize.y * 0.4F});
    }));
    add(warteBis("dock4: XY eigenes Windows-Fenster", [] { return dock1::eigenesOsFenster("karte.xy"); }, 120));
    add(stabil("dock4: XY im eigenen Fenster", 15));
    add(pruefe("dock4: Hauptfenster und jedes eigene Windows-Fenster nehmen Dateien an (WS_EX_ACCEPTFILES, wie der Explorer es braucht)", [] {
        int fenster = 0;
        int annehmend = 0;
        for (const ImGuiViewport* v : ImGui::GetPlatformIO().Viewports) {
            if (v->PlatformHandle == nullptr) { continue; }
            ++fenster;
            if (platform::nimmtDateien(v->PlatformHandle)) { ++annehmend; }
        }
        diag::detail("Knopftest: dock4: " + std::to_string(annehmend) + " von " + std::to_string(fenster) + " Windows-Fenstern nehmen Dateien an");
        return fenster >= 2 && annehmend == fenster;
    }));
    auto modelle = std::make_shared<int>(0);
    auto soll = std::make_shared<std::array<float, 2>>();
    add(tu("dock4: ein Modell (.obj) auf die Mitte der XY-Ansicht im eigenen Fenster ablegen", [modelle, soll] {
        const std::string pfad = platform::executableDirectory() + "\\dock4ablage.obj";
        {
            std::ofstream o(::bhed::utf8Pfad(pfad));
            o << "v 0 0 0\nv 16 0 0\nv 0 16 0\nf 1 2 3\n";
        }
        *modelle = 0;
        for (const auto& e : ke::editor().karte.entitaeten()) {
            if (e->klassenname() == "misc_model") { ++*modelle; }
        }
        const ImGuiWindow* w = dock1::panelFenster("karte.xy");
        const auto* xy = dock4::rahmen("XY");
        if (w == nullptr || xy == nullptr || w->Viewport == nullptr) { return; }
        const ImVec2 mitte{std::floor((xy->x0 + xy->x1) * 0.5F), std::floor((xy->y0 + xy->y1) * 0.5F)};
        const ke::XyFenster& f = ke::editor().xy[0];
        const karte::Vektor3f p = f.ansicht.zuPunkt(static_cast<int>(mitte.x - f.bildX), static_cast<int>(mitte.y - f.bildY),
                                                    ke::editor().raster.einrastGroesse());
        (*soll)[0] = p.x();
        (*soll)[1] = p.y();
        // Fensterpunkte des Windows-Fensters (DROPFILES.pt wie vom Explorer)
        const bool gesendet = platform::dateiAblegen(w->Viewport->PlatformHandle, pfad, static_cast<int>(mitte.x - w->Viewport->Pos.x),
                                                     static_cast<int>(mitte.y - w->Viewport->Pos.y));
        diag::detail(std::string("Knopftest: dock4: Modell auf XY im eigenen Fenster abgelegt: ") + (gesendet ? "ja" : "nein"));
    }));
    add(stabil("dock4: abgelegt", 20));
    add(pruefe("dock4: abgelegtes Modell als misc_model unter dem Zeiger (XY im eigenen Fenster)", [modelle, soll] {
        int n = 0;
        std::string ort;
        for (const auto& e : ke::editor().karte.entitaeten()) {
            if (e->klassenname() == "misc_model") {
                ++n;
                ort = e->wert("origin");
            }
        }
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
        const bool gelesen = std::sscanf(ort.c_str(), "%f %f %f", &x, &y, &z) == 3;
        char t[200];
        std::snprintf(t, sizeof(t), "Knopftest: dock4: misc_model %d -> %d, origin \"%s\", soll x %.0f y %.0f", *modelle, n, ort.c_str(),
                      static_cast<double>((*soll)[0]), static_cast<double>((*soll)[1]));
        diag::detail(t);
        return n == *modelle + 1 && gelesen && std::fabs(x - (*soll)[0]) < 0.5F && std::fabs(y - (*soll)[1]) < 0.5F;
    }));
    add(tu("dock4: XY zurueck, Map Editor wie am Anfang", [stand] {
        dock::andocken("karte.xy");
        ke::Editor& e = ke::editor();
        e.anordnung = stand->anordnung;
        e.gruppenDialogEingebaut = stand->gruppe;
        e.splitFrei = stand->frei;
        e.splitBelegung = stand->belegung;
        e.hGroessen = stand->h;
        e.vGroessen = stand->v;
        e.v2Groessen = stand->v2;
        e.teilerFuer = -1;
    }));
    add(tu("dock4: Hauptfenster zurueck", [lage] {
        (void)platform::fensterLageSetzen(ImGui::GetMainViewport()->PlatformHandle, (*lage)[0], (*lage)[1], (*lage)[2], (*lage)[3]);
    }));
    add(stabil("dock4: Ende", 15));
    add(tu("dock4: Schalter zurueck", [vorher] { dockraumAn() = vorher; }));
    return s;
}
