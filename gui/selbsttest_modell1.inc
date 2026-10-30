// selbsttest_modell1.inc - modell1: was der Model-Tab je Bild kostet
// (BHED_EDITORTEST=modellleistung). Eingebunden in selbsttest.cpp.
//
// shank: der Model-Tab wirkt "leicht laggy". Gemessen wird je Figur und Lage
// (ruhig, Animation, Drehteller mit Animation, Knochen, NPC-Browser) ueber
// eine feste Zahl Bilder:
//   - die CPU-Zeit des Bildes: vom Bildanfang (selbsttestVorBild, vor
//     ImGui::NewFrame) bis zum Selbsttest am Ende von draw() - ohne
//     ImGui::Render, Present und das Warten auf den Bildschirm;
//   - der Abstand der Bilder (mit dem Warten; das 99. Perzentil zeigt Ruckler);
//   - das Neuzeichnen des Modellfensters: wie oft, CPU-Zeit
//     (App::modellCpuMsSumme), Grafikkarte (gpu::modellGpuMs), Aufrufe,
//     hochgeladene Figurenbilder.
// Waehrend der Messung meldet sich das Element-Register ab: sonst zahlt jedes
// Element den Haken des Selbsttests, den es im Alltag nicht gibt.
// Je Figur ein Foto in fester Lage (Bild 5 einer Animation, eingepasste
// Kamera, mit und ohne Knochen) fuer den Bildvergleich vorher/nachher.

namespace {

bool g_m1Messen = false;
std::chrono::steady_clock::time_point g_m1BildAnfang{};

// Aus selbsttestVorBild: der Anfang des Bildes (nur waehrend einer Messung).
void modell1BildAnfang() {
    if (g_m1Messen) { g_m1BildAnfang = std::chrono::steady_clock::now(); }
}

// Aus selbsttest(): waehrend einer Messung kein Element-Register.
bool modell1OhneRegister() { return g_m1Messen; }

double m1Mittel(const std::vector<double>& v) {
    double s = 0.0;
    for (const double x : v) { s += x; }
    return v.empty() ? -1.0 : s / static_cast<double>(v.size());
}

double m1Quantil(std::vector<double> v, double q) {
    if (v.empty()) { return -1.0; }
    std::sort(v.begin(), v.end());
    const auto i = static_cast<std::size_t>(q * static_cast<double>(v.size() - 1U) + 0.5);
    return v[std::min(i, v.size() - 1U)];
}

// Eine Lage messen: `lage` stellt sie her, dann kAnlauf Bilder Anlauf und
// kBilder gemessene.
Schritt m1Messblock(const std::string& figur, const std::string& name, std::function<void()> lage) {
    struct Stand {
        std::vector<double> cpu, abstand, modellCpu, gpu;
        int neu = 0;
        int aufrufe = 0;
        int hochgeladen = 0;
        std::chrono::steady_clock::time_point vorher{};
        double cpuVorher = 0.0;
        int neuVorher = 0;
        int hochVorher = 0;
    };
    auto st = std::make_shared<Stand>();
    constexpr int kAnlauf = 12;
    constexpr int kBilder = 120;
    return {"Modell1: " + figur + " | " + name, [=](int b) {
                const auto jetzt = std::chrono::steady_clock::now();
                int hoch = 0;
                int geteilt = 0;
                gpu::figurBilderZahlen(hoch, geteilt);
                if (b == 0) {
                    lage();
                    g_m1Messen = true;
                } else if (b > kAnlauf) {
                    st->cpu.push_back(std::chrono::duration<double, std::milli>(jetzt - g_m1BildAnfang).count());
                    st->abstand.push_back(std::chrono::duration<double, std::milli>(jetzt - st->vorher).count());
                    const int neu = g_app->modellNeuGezeichnet - st->neuVorher;
                    st->neu += neu;
                    if (neu > 0) {
                        st->modellCpu.push_back(g_app->modellCpuMsSumme - st->cpuVorher);
                        st->aufrufe = g_app->modellAufrufe;
                        const float g = gpu::modellGpuMs();
                        if (g >= 0.0F) { st->gpu.push_back(static_cast<double>(g)); }
                    }
                    st->hochgeladen += hoch - st->hochVorher;
                }
                st->vorher = jetzt;
                st->cpuVorher = g_app->modellCpuMsSumme;
                st->neuVorher = g_app->modellNeuGezeichnet;
                st->hochVorher = hoch;
                if (b < kAnlauf + kBilder) { return false; }
                g_m1Messen = false;
                char z[420];
                std::snprintf(z, sizeof(z),
                              "Knopftest: Modell1-Messung %s | %s: Bild-CPU Mittel %.2f p99 %.2f ms, Abstand Mittel %.2f "
                              "p99 %.2f ms, neu gezeichnet %d von %zu, Modell-CPU Mittel %.3f p99 %.3f ms, Grafikkarte "
                              "Median %.3f ms, Aufrufe %d, Bilder hochgeladen %d",
                              figur.c_str(), name.c_str(), m1Mittel(st->cpu), m1Quantil(st->cpu, 0.99),
                              m1Mittel(st->abstand), m1Quantil(st->abstand, 0.99), st->neu, st->cpu.size(),
                              m1Mittel(st->modellCpu), m1Quantil(st->modellCpu, 0.99), m1Quantil(st->gpu, 0.5),
                              st->aufrufe, st->hochgeladen);
                diag::detail(z);
                return true;
            }};
}

// Die Animation fuer Messung und Foto: BOTH_RUN1, sonst die erste Schleife mit
// mindestens zehn Bildern, sonst die erste.
int m1Animation() {
    int schleife = -1;
    for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
        const AnimEntry& e = g_app->animList[i];
        if (e.name == "BOTH_RUN1") { return static_cast<int>(i); }
        if (schleife < 0 && e.loopFrame >= 0 && e.numFrames >= 10) { schleife = static_cast<int>(i); }
    }
    return schleife >= 0 ? schleife : (g_app->animList.empty() ? -1 : 0);
}

// Feste Lage fuer Foto und Messbeginn: Bild 5 der Animation, angehalten,
// eingepasste Kamera, ohne Drehteller und Knochen, NPC-Browser zu.
void m1Grundlage() {
    g_app->modellDrehteller = false;
    g_app->modellKnochen = false;
    g_app->npcBrowserOffen = false;
    g_app->showCaps = false;
    g_app->showModelTextures = true;
    g_app->loopMode = 1;
    g_app->animSpeed = 1.0F;
    const int a = m1Animation();
    if (a >= 0) {
        animWaehlen(a, false);
        g_app->animFrame = g_app->animList[static_cast<std::size_t>(a)].firstFrame + 5;
        g_app->animScrollen = true;
    }
    modellAnsichtEinpassen();
    g_app->modelDirty = true;
}

void m1Abspielen() {
    m1Grundlage();
    g_app->animPlaying = true;
}

}  // namespace

// Ruhe (rc596, darfRuhen): nach einer Sekunde ohne Eingabe wartet die
// Hauptschleife bis zu 250 ms auf eine Nachricht.
//   - Eine laufende Animation und der Drehteller aendern das Bild aber aus sich
//     heraus. Ruhte die Schleife trotzdem, liefen sie mit vier Bildern je
//     Sekunde, sobald die Maus stillstand ("leicht laggy") - so, wenn vorher
//     eine Karte gezeichnet war (mapDirty aus). Erwartet: fluessig (mindestens
//     20 Bilder je Sekunde).
//   - Umgekehrt ruhte der Model-Tab nie, solange die Karte als "neu zu
//     zeichnen" galt - sie wird dort gar nicht gezeichnet, mapDirty blieb
//     stehen, und die Oberflaeche lief ohne Eingabe mit voller Bildrate.
//     Erwartet: steht die Figur, hoechstens 8 Bilder je Sekunde.
std::vector<Schritt> modell1RuheSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct Lage {
        const char* name;
        bool abspielen;
        bool drehteller;
        bool karteNeu;     // mapDirty
        bool fluessig;     // erwartet: fluessig (sonst: ruht)
    };
    static const Lage kLagen[] = {{"Animation, Karte gezeichnet", true, false, false, true},
                                  {"Drehteller, Karte gezeichnet", false, true, false, true},
                                  {"Figur steht, Karte noch nicht gezeichnet", false, false, true, false}};
    for (const Lage& l : kLagen) {
        auto bilder = std::make_shared<int>(0);
        auto seit = std::make_shared<std::chrono::steady_clock::time_point>();
        add(tu(std::string("Modell1 Ruhe: ") + l.name + ", Ruhe im Test erlaubt", [l] {
            m1Grundlage();
            g_app->animPlaying = l.abspielen;
            g_app->modellDrehteller = l.drehteller;
            g_app->mapDirty = l.karteNeu;
            g_app->ruheImTest = true;
        }));
        add(pause(1.6));
        add(tu(std::string("Modell1 Ruhe: ") + l.name + ", Bilder zaehlen", [=] {
            *bilder = ImGui::GetFrameCount();
            *seit = std::chrono::steady_clock::now();
        }));
        add(pause(1.5));
        add(pruefe(std::string("Modell1 Ruhe: ") + l.name + (l.fluessig ? ": fluessig (mindestens 20 Bilder je Sekunde)"
                                                                         : ": die Schleife ruht (hoechstens 8 Bilder je Sekunde)"),
                   [=] {
                       const double sek = std::chrono::duration<double>(std::chrono::steady_clock::now() - *seit).count();
                       const int n = ImGui::GetFrameCount() - *bilder;
                       g_app->ruheImTest = false;
                       g_app->animPlaying = false;
                       g_app->modellDrehteller = false;
                       char z[200];
                       std::snprintf(z, sizeof(z), "Knopftest: Modell1 Ruhe - %s: %d Bilder in %.2f s", l.name, n, sek);
                       diag::detail(z);
                       const double jeSek = static_cast<double>(n) / std::max(sek, 0.001);
                       return sek > 0.5 && (l.fluessig ? jeSek >= 20.0 : jeSek <= 8.0);
                   }));
    }
    return s;
}

// reste2 Punkt 2: Haeute wechseln laesst keine Bilder alter Haeute auf der Grafikkarte zurueck. Die
// Figurenbilder merkt sich gpumap nach der Adresse des Bildes (TextureSet::Tex); eine neue Haut baut neue
// Bilder, die alten verschwinden - ihre Eintraege blieben samt Grafikbild stehen (die geteilten Inhalte
// fuer immer). Sturmtruppler mit model_default gemerkt, dann dreimal model_officer und zurueck: dieselbe
// Zahl Figurenbilder und Inhalte wie mit model_default allein, das Bild Bildpunkt fuer Bildpunkt gleich.
bool m1HautWaehlen(const char* name) {
    for (std::size_t i = 0; i < g_app->modelSkins.size(); ++i) {
        if (g_app->modelSkins[i] == name) {
            applySkinByIndex(static_cast<int>(i));
            g_app->modelDirty = true;
            return true;
        }
    }
    return false;
}

std::string m1Datei(const std::string& pfad) {
    std::ifstream f(std::filesystem::path(pfad), std::ios::binary);
    std::ostringstream o;
    o << f.rdbuf();
    return o.str();
}

std::vector<Schritt> modell1HautSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Modell1 Haut: stormtrooper, model_default", [] {
        std::string fehler;
        if (!oeffneModellAusArchiv("models/players/stormtrooper/model.glm", &fehler)) { meldeFehler("Modell1 Haut: " + fehler); }
        g_app->leftMode = 2;
        (void)m1HautWaehlen("model_default.skin");
        m1Grundlage();
    }));
    add(pause(0.8));
    auto stand = std::make_shared<std::array<int, 4>>();
    add(tu("Modell1 Haut: Stand mit model_default allein", [stand] {
        const gpu::SpeicherStand st = gpu::speicherStand();
        (*stand)[0] = st.figurBilder;
        (*stand)[1] = st.figurBildInhalte;
    }));
    add(foto("modell1_haut_vorher"));
    for (int runde = 0; runde < 3; ++runde) {
        add(tu("Modell1 Haut: model_officer", [] { (void)m1HautWaehlen("model_officer.skin"); }));
        add(pause(0.4));
        add(tu("Modell1 Haut: model_default", [] { (void)m1HautWaehlen("model_default.skin"); }));
        add(pause(0.4));
    }
    add(tu("Modell1 Haut: Stand nach dreimal Hin und Her", [stand] {
        const gpu::SpeicherStand st = gpu::speicherStand();
        (*stand)[2] = st.figurBilder;
        (*stand)[3] = st.figurBildInhalte;
    }));
    add(foto("modell1_haut_nachher"));
    add(pause(0.5));
    add(pruefe("Modell1 Haut: dreimal hin und her - so viele Figurenbilder und Inhalte wie mit model_default allein, das Bild gleich", [stand] {
        const char* ordner = std::getenv("BHED_FOTOS");
        const std::string a = ordner != nullptr ? m1Datei(std::string(ordner) + "\\modell1_haut_vorher.bmp") : std::string();
        const std::string b = ordner != nullptr ? m1Datei(std::string(ordner) + "\\modell1_haut_nachher.bmp") : std::string();
        const bool bildGleich = ordner == nullptr || (!a.empty() && a == b);
        char z[240];
        std::snprintf(z, sizeof(z), "Knopftest: Modell1 Haut - %zu Haeute; Figurenbilder %d -> %d, Inhalte %d -> %d, Bild %s",
                      g_app->modelSkins.size(), (*stand)[0], (*stand)[2], (*stand)[1], (*stand)[3],
                      ordner == nullptr ? "ohne BHED_FOTOS nicht verglichen" : (bildGleich ? "gleich" : "anders"));
        diag::detail(z);
        return (*stand)[0] > 0 && (*stand)[2] == (*stand)[0] && (*stand)[3] == (*stand)[1] && bildGleich;
    }));
    return s;
}

std::vector<Schritt> modellLeistungSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Modell1: Model-Tab, Spielpfade", [] {
        g_app->leftMode = 2;
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "modell1.txt");
    }));
    add(pause(0.5));
    // BHED_MODELL1_NUR=ruhe: nur die erste Figur und die Ruhe (schnell zum
    // Nachstellen).
    const char* nur = std::getenv("BHED_MODELL1_NUR");
    const bool nurRuhe = nur != nullptr && std::strcmp(nur, "ruhe") == 0;
    // BHED_MODELL1_NUR=haut: nur der Hautwechsel (reste2 Punkt 2)
    if (nur != nullptr && std::strcmp(nur, "haut") == 0) {
        for (Schritt& x : modell1HautSchritte()) { s.push_back(std::move(x)); }
        add(tu("Modell1: zurueck", [] {
            g_app->leftMode = 1;
            g_app->doc.markSaved();
        }));
        return s;
    }
    if (!nurRuhe) { add(m1Messblock("ohne Figur", "ruhig", [] {})); }
    // JKA: Sturmtruppler; JKA mit vielen Flaechen (jedi_hf, 214); Movie Duels
    // mit den meisten Flaechen (jedi_hm, 236) und den meisten Ecken in LOD 0
    // (clonetrooper_p2, 28460) - gezaehlt mit einem Blick in alle model.glm.
    static const char* const kFiguren[][2] = {{"stormtrooper", "models/players/stormtrooper/model.glm"},
                                              {"jedi_hf", "models/players/jedi_hf/model.glm"},
                                              {"jedi_hm", "models/players/jedi_hm/model.glm"},
                                              {"clonetrooper_p2", "models/players/clonetrooper_p2/model.glm"}};
    for (const auto& f : kFiguren) {
        const bool erste = &f == &kFiguren[0];
        if (nurRuhe && !erste) { break; }
        const std::string name = f[0];
        const std::string pfad = f[1];
        add(tu("Modell1: " + name + " laden", [=] {
            std::string fehler;
            if (!oeffneModellAusArchiv(pfad, &fehler)) { meldeFehler("Modell1: " + pfad + ": " + fehler); }
            g_app->leftMode = 2;
            m1Grundlage();
        }));
        add(pause(1.0));
        add(pruefe("Modell1: " + name + " geladen, mit Animationen", [=] {
            int flaechen = 0;
            for (const GlmSurface& sf : g_app->model.surfaces) { flaechen += sf.isTag() ? 0 : 1; }
            diag::detail("Knopftest: Modell1 - " + name + ": " + std::to_string(flaechen) + " Flaechen, " +
                         std::to_string(g_app->animList.size()) + " Animationen, Aufrufe " +
                         std::to_string(g_app->modellAufrufe));
            return !g_app->model.empty() && !g_app->animList.empty() && g_app->modellAufrufe > 0;
        }));
        add(foto("modell1_" + name));
        if (nurRuhe) { continue; }
        if (erste) {
            // Die Listen im Bildvergleich: Gruppe TORSO und der NPC-Browser.
            add(klick("TORSO", "", 0, false));
            add(pause(0.3));
            add(foto("modell1_torso"));
            add(klick(tr(Str::ModelAnimAll), "", 0, false));
            add(tu("Modell1: NPC-Browser auf", [] { g_app->npcBrowserOffen = true; }));
            add(pause(0.5));
            add(foto("modell1_npcbrowser"));
            add(tu("Modell1: NPC-Browser zu", [] { g_app->npcBrowserOffen = false; }));
        }
        add(tu("Modell1: Knochen an", [] {
            g_app->modellKnochen = true;
            g_app->modelDirty = true;
        }));
        add(pause(0.3));
        add(foto("modell1_" + name + "_knochen"));
        add(m1Messblock(name, "ruhig", [] { m1Grundlage(); }));
        add(m1Messblock(name, "Animation", [] { m1Abspielen(); }));
        add(m1Messblock(name, "Drehteller und Animation", [] {
            m1Abspielen();
            g_app->modellDrehteller = true;
        }));
        add(m1Messblock(name, "Knochen und Animation", [] {
            m1Abspielen();
            g_app->modellKnochen = true;
        }));
        add(m1Messblock(name, "NPC-Browser offen", [] {
            m1Grundlage();
            g_app->npcBrowserOffen = true;
        }));
    }
    if (!nurRuhe) {
        for (Schritt& x : modell1HautSchritte()) { s.push_back(std::move(x)); }
    }
    for (Schritt& x : modell1RuheSchritte()) { s.push_back(std::move(x)); }
    add(tu("Modell1: zurueck", [] {
        m1Grundlage();
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}
