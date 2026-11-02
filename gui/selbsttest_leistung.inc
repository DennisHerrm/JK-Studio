// selbsttest_leistung.inc - rc608 Teil 12 (Modus "leistung"): was kostet das Programm, und bleibt das Bild gleich?
//
// Misst ruhig und wiederholbar, je Zeile "Leistung: <was> | ...":
//   - Tab-Wechsel (Prozessorzeit des Wechselbildes und der zwei folgenden), erste und zweite Runde
//   - Karte laden (mp/duel1 klein, md_toaj_jedi gross) bis zum ersten fertigen Bild
//   - Szene mit vielen Figuren aufbauen (md_am_sith, md_am/intro2_sith)
//   - Bildzeit je Tab ruhend und in Bewegung: Prozessor (newFrame bis vor Present) und Grafikkarte
//     (Zeitmarken ueber dieselbe Strecke, render::bildMessung), je 240 Bilder nach 12 Bildern Anlauf
//   - Leerlauf: Prozessorzeit des ganzen Prozesses bei stillstehendem Fenster (darfRuhen), 8 s je Tab
//   - Speicher (Spitze, Arbeitssatz, privat, Grafikkarte) nach jedem Abschnitt
// Bilder fuer den Vergleich vorher/nachher (BHED_FOTOS): feste Lage, feste Zeit, die 3D-Bilder direkt aus ihren
// Zielen (Vorschau, Kamera des Karten-Editors), Model-Tab und Menue-Editor als ganzes Fenster.
// BHED_LEISTUNG_MAP: eine .map fuer den Karten-Editor mit Echtzeit-Licht (ohne: dieser Teil faellt weg).
// BHED_LEISTUNG_NUR: nur diese Abschnitte (Buchstaben: w Wechsel, k Karten, s Szene, e Editor, m Model, n Menue, g die
// Gegenproben zum Bildvorrat - sie kopieren texCache und treiben die Speicherspitze, fuer Speichervergleiche weglassen).
namespace leistung {
namespace ke = karteneditor;

inline bool abschnitt(char c) {
    const char* nur = std::getenv("BHED_LEISTUNG_NUR");
    return nur == nullptr || nur[0] == '\0' || std::strchr(nur, c) != nullptr;
}

inline std::string zahlen(std::vector<double> v, const char* format) {
    if (v.empty()) { return "-"; }
    std::sort(v.begin(), v.end());
    double s = 0.0;
    for (const double x : v) { s += x; }
    const auto q = [&v](double a) { return v[std::min(v.size() - 1U, static_cast<std::size_t>(a * static_cast<double>(v.size() - 1U) + 0.5))]; };
    char z[160];
    std::snprintf(z, sizeof(z), format, s / static_cast<double>(v.size()), q(0.5), q(0.99));
    return z;
}

inline void speicher(const std::string& wann) {
    const platform::ProzessStand p = platform::prozessStand();
    const gpu::SpeicherStand g = gpu::speicherStand();
    char z[260];
    std::snprintf(z, sizeof(z), "Leistung: Speicher %s | Spitze %lld MB, Arbeitssatz %lld MB, privat %lld MB, Grafikkarte %lld MB", wann.c_str(),
                  p.spitzeBytes / 1048576, p.arbeitBytes / 1048576, p.privatBytes / 1048576, g.grafikBytes / 1048576);
    diag::detail(z);
}

// 12 Bilder Anlauf, dann `bilder` gemessene. `lage` einmal am Anfang, `bewegen` jedes Bild (Bildnummer ab 0).
inline Schritt bildzeit(const std::string& was, std::function<void()> lage, std::function<void(int)> bewegen, int bilder = 240) {
    struct Stand {
        std::vector<double> cpu, gpu, abstand;
        int cpuBilder = 0;
        int gpuBilder = 0;
        std::chrono::steady_clock::time_point vorher{};
    };
    auto st = std::make_shared<Stand>();
    constexpr int kAnlauf = 12;
    return {"Leistung: Bildzeit " + was, [=](int b) {
                const auto jetzt = std::chrono::steady_clock::now();
                if (b == 0) {
                    lage();
                    g_m1Messen = true;   // ohne Element-Register, wie im Alltag
                    render::bildMessung(true);
                } else if (b > kAnlauf) {
                    const render::BildZeiten z = render::bildZeiten();
                    if (z.cpuBilder != st->cpuBilder) { st->cpu.push_back(z.cpuMs); }
                    if (z.gpuBilder != st->gpuBilder) { st->gpu.push_back(z.gpuMs); }
                    st->abstand.push_back(std::chrono::duration<double, std::milli>(jetzt - st->vorher).count());
                }
                const render::BildZeiten z = render::bildZeiten();
                st->cpuBilder = z.cpuBilder;
                st->gpuBilder = z.gpuBilder;
                st->vorher = jetzt;
                if (bewegen) { bewegen(b); }
                if (b < kAnlauf + bilder) { return false; }
                render::bildMessung(false);
                g_m1Messen = false;
                diag::detail("Leistung: Bildzeit " + was + " | CPU " + zahlen(st->cpu, "Mittel %.2f p50 %.2f p99 %.2f ms") + " | GPU " +
                             zahlen(st->gpu, "Mittel %.3f p50 %.3f p99 %.3f ms") + " | Abstand " + zahlen(st->abstand, "Mittel %.2f p50 %.2f p99 %.2f ms") +
                             " | Bilder " + std::to_string(st->cpu.size()) + "/" + std::to_string(st->gpu.size()));
                return true;
            }};
}

// Leerlauf: keine Eingabe, Ruhe erlaubt (darfRuhen), 8 s; Prozessorzeit aller Faeden je Sekunde Wanduhr.
inline Schritt leerlauf(const std::string& was) {
    struct Stand {
        double cpu0 = 0.0;
        int bild0 = 0;
        bool misst = false;
        std::chrono::steady_clock::time_point t0{};
    };
    auto st = std::make_shared<Stand>();
    return {"Leistung: Leerlauf " + was, [=](int b) {
                const auto jetzt = std::chrono::steady_clock::now();
                if (b == 0) {
                    g_app->ruheImTest = true;
                    g_m1Messen = true;
                    st->t0 = jetzt;
                    return false;
                }
                // 1,5 s, bis die Schleife ruht (kStille 1 s), dann 8 s messen
                if (!st->misst) {
                    if (jetzt - st->t0 < std::chrono::milliseconds(1500)) { return false; }
                    st->misst = true;
                    st->cpu0 = platform::prozessStand().cpuSekunden;
                    st->bild0 = ImGui::GetFrameCount();
                    st->t0 = jetzt;
                    return false;
                }
                if (jetzt - st->t0 < std::chrono::seconds(8)) { return false; }
                const double wand = std::chrono::duration<double>(jetzt - st->t0).count();
                const double cpu = platform::prozessStand().cpuSekunden - st->cpu0;
                char z[200];
                std::snprintf(z, sizeof(z), "Leistung: Leerlauf %s | Prozessor %.1f %% eines Kerns, %.1f Bilder/s", was.c_str(), 100.0 * cpu / wand,
                              static_cast<double>(ImGui::GetFrameCount() - st->bild0) / wand);
                diag::detail(z);
                g_app->ruheImTest = false;
                g_m1Messen = false;
                return true;
            }};
}

// Tab-Wechsel: Prozessorzeit des Wechselbildes und der beiden danach (das groesste zaehlt).
inline Schritt wechsel(const std::string& was, int modus) {
    auto werte = std::make_shared<std::vector<double>>();
    auto zaehler = std::make_shared<int>(0);
    auto t0 = std::make_shared<std::chrono::steady_clock::time_point>();
    return {"Leistung: Wechsel " + was, [=](int b) {
                if (b == 0) {
                    g_m1Messen = true;
                    render::bildMessung(true);
                    return false;
                }
                if (b == 2) {
                    *t0 = std::chrono::steady_clock::now();
                    g_app->leftMode = modus;
                    *zaehler = render::bildZeiten().cpuBilder;
                    return false;
                }
                if (b > 2) {
                    const render::BildZeiten z = render::bildZeiten();
                    if (z.cpuBilder != *zaehler) {
                        werte->push_back(z.cpuMs);
                        *zaehler = z.cpuBilder;
                    }
                }
                if (werte->size() < 3U && b < 60) { return false; }
                render::bildMessung(false);
                g_m1Messen = false;
                double gross = 0.0;
                double summe = 0.0;
                for (const double v : *werte) {
                    gross = std::max(gross, v);
                    summe += v;
                }
                char z[200];
                std::snprintf(z, sizeof(z), "Leistung: Wechsel %s | groesstes Bild %.2f ms, drei Bilder zusammen %.2f ms", was.c_str(), gross, summe);
                diag::detail(z);
                return true;
            }};
}

inline void bmpAusRgba(const std::string& name, const std::vector<std::uint8_t>& rgba, int breite, int hoehe) {
    const char* o = std::getenv("BHED_FOTOS");
    if (o == nullptr || o[0] == '\0') { return; }
    stufe9c::Bild b;
    b.rgba.assign(rgba.begin(), rgba.end());
    b.breite = breite;
    b.hoehe = hoehe;
    stufe9c::bmpSchreiben(std::string(o) + "\\leistung_" + name + ".bmp", b);
}

inline Schritt vorschauBild(const std::string& name) {
    return tu("Leistung: Vorschaubild " + name, [name] {
        std::vector<std::uint8_t> rgba;
        int w = 0;
        int h = 0;
        if (!gpu::leseFarbe(rgba, w, h)) {
            meldeFehler("Leistung: Vorschaubild " + name + " nicht lesbar");
            return;
        }
        bmpAusRgba(name, rgba, w, h);
        diag::detail("Leistung: Bild " + name + " " + std::to_string(w) + " x " + std::to_string(h));
    });
}

inline Schritt kameraBild(const std::string& name) {
    return tu("Leistung: Kamerabild " + name, [name] {
        std::vector<unsigned char> rgba;
        int w = 0;
        int h = 0;
        if (!ke::kameraBild(rgba, w, h)) {
            meldeFehler("Leistung: Kamerabild " + name + " nicht lesbar");
            return;
        }
        bmpAusRgba(name, rgba, w, h);
        diag::detail("Leistung: Bild " + name + " " + std::to_string(w) + " x " + std::to_string(h));
    });
}

// Eine Karte in der Vorschau laden: Zeit bis loadMapFromArchive zurueck ist und bis das erste Bild fertig ist.
inline void karteLaden(std::vector<Schritt>& s, const std::string& bsp) {
    auto t0 = std::make_shared<std::chrono::steady_clock::time_point>();
    auto geladen = std::make_shared<double>(0.0);
    s.push_back(tu("Leistung: Karte " + bsp, [=] {
        *t0 = std::chrono::steady_clock::now();
        if (!loadMapFromArchive(bsp)) { meldeFehler("Leistung: " + bsp + " nicht geladen"); }
        *geladen = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - *t0).count();
    }));
    s.push_back(stufe9c::langeWarten("Leistung: " + bsp + " gezeichnet", [] { return g_app->mapBereit && g_app->camTrackValid && !g_app->mapDirty; }, 20000));
    s.push_back(tu("Leistung: Karte " + bsp + " Zeit", [=] {
        char z[200];
        std::snprintf(z, sizeof(z), "Leistung: Karte laden %s | geladen %.0f ms, erstes fertiges Bild %.0f ms", bsp.c_str(), *geladen,
                      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - *t0).count());
        diag::detail(z);
        speicher("nach " + bsp);
    }));
}

// Kamera der Vorschau frei, fester Ort und Blick; mapDirty, bis gezeichnet.
inline void vorschauKamera(const std::array<float, 6>& k) {
    g_app->playing = false;
    g_app->throughCamera = false;
    g_app->camViewActive = false;
    g_app->followCam = false;
    g_app->playMs = 0.0;   // Wetter und Shader rechnen mit der Szenenzeit: fest fuer den Bildvergleich
    for (int i = 0; i < 3; ++i) {
        g_app->cam.pos[i] = k[static_cast<std::size_t>(i)];
        g_app->cam.angles[i] = k[static_cast<std::size_t>(i + 3)];
    }
    g_app->mapDirty = true;
}

// Bildpunkte zweier Texturen gleich (Bild, Pyramide, Verweise auf Stufen, Folgen, Himmelsseiten)?
inline bool gleicheBilder(const TextureSet::Tex& a, const TextureSet::Tex& b) {
    if (a.width != b.width || a.height != b.height || a.rgba != b.rgba || a.mips.size() != b.mips.size() || a.extraStages != b.extraStages ||
        a.animFrames != b.animFrames || a.skyFaces != b.skyFaces) {
        return false;
    }
    for (std::size_t m = 0; m < a.mips.size(); ++m) {
        if (a.mips[m].width != b.mips[m].width || a.mips[m].height != b.mips[m].height || a.mips[m].rgba != b.mips[m].rgba) { return false; }
    }
    return true;
}

// rc608 Teil 12, Gegenprobe zum Bildvorrat (bildVorratFuellen): dieselben Texturen einmal mit, einmal ohne Vorrat, jeweils
// mit leerem texCache - die Bildpunkte muessen gleich sein. Die Karte: loadMapTexturesFuer mit der Geometrie der Vorschau;
// die Figuren: texturesFor fuer jedes geladene Modell der Szene. texCache und die Stufenbilder werden danach zurueckgelegt.
inline Schritt vorratGegenprobe(const std::string& was, bool figuren) {
    if (!abschnitt('g')) { return tu("Leistung: ohne Gegenprobe " + was, [] {}); }
    return pruefe("Leistung: Bildvorrat " + was + " - mit und ohne Vorrat dieselben Bildpunkte", [figuren] {
        const auto merkCache = g_app->texCache;
        const auto merkStufen = g_app->figurStufenBilder;
        std::size_t texturen = 0;
        std::size_t anders = 0;
        if (!figuren) {
            TextureSet mit;
            TextureSet ohne;
            std::unordered_set<std::string> bilderMit;
            std::unordered_set<std::string> bilderOhne;
            g_app->texCache.clear();
            loadMapTexturesFuer(g_app->geo, mit, bilderMit);
            g_app->texCache.clear();
            bildVorratAbschalten(true);
            loadMapTexturesFuer(g_app->geo, ohne, bilderOhne);
            bildVorratAbschalten(false);
            anders += mit.byShader.size() != ohne.byShader.size() || mit.found != ohne.found || bilderMit != bilderOhne ? 1U : 0U;
            for (std::size_t i = 0; i < std::min(mit.byShader.size(), ohne.byShader.size()); ++i) {
                ++texturen;
                anders += gleicheBilder(mit.byShader[i], ohne.byShader[i]) ? 0U : 1U;
            }
        } else {
            std::set<const GlmModel*> gesehen;
            for (const auto& assets : g_app->actorAssets) {
                if (!assets || assets->model.empty() || !gesehen.insert(&assets->model).second) { continue; }
                g_app->texCache.clear();
                g_app->figurStufenBilder.clear();
                const ModelTextures mit = texturesFor(assets->model);
                g_app->texCache.clear();
                g_app->figurStufenBilder.clear();
                bildVorratAbschalten(true);
                const ModelTextures ohne = texturesFor(assets->model);
                bildVorratAbschalten(false);
                ++texturen;
                bool gleich = mit.bySurface.size() == ohne.bySurface.size() && mit.gluehStufen.size() == ohne.gluehStufen.size() &&
                              mit.stufen.size() == ohne.stufen.size() && mit.found == ohne.found && mit.missing == ohne.missing;
                for (std::size_t i = 0; gleich && i < mit.bySurface.size(); ++i) {
                    gleich = gleicheBilder(mit.bySurface[i], ohne.bySurface[i]) && mit.gluehStufen[i].size() == ohne.gluehStufen[i].size() &&
                             mit.stufen[i].size() == ohne.stufen[i].size();
                    for (std::size_t k = 0; gleich && k < mit.gluehStufen[i].size(); ++k) { gleich = gleicheBilder(mit.gluehStufen[i][k], ohne.gluehStufen[i][k]); }
                    for (std::size_t k = 0; gleich && k < mit.stufen[i].size(); ++k) {
                        const auto& a = mit.stufen[i][k].pixel;
                        const auto& b = ohne.stufen[i][k].pixel;
                        gleich = a.size() == b.size();
                        for (std::size_t j = 0; gleich && j < a.size(); ++j) { gleich = a[j] && b[j] && gleicheBilder(*a[j], *b[j]); }
                    }
                }
                anders += gleich ? 0U : 1U;
            }
        }
        g_app->texCache = merkCache;
        g_app->figurStufenBilder = merkStufen;
        diag::detail("Knopftest: Leistung Bildvorrat - " + std::to_string(texturen) + (figuren ? " Modelle" : " Texturen") + ", davon anders " +
                     std::to_string(anders));
        return texturen > 0U && anders == 0U;
    });
}

inline std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Leistung: Spielordner", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
        speicher("Start");
    }));
    add(pause(1.0));
    if (abschnitt('w')) {
        for (const char* runde : {"1", "2"}) {
            const std::string r = runde;
            add(wechsel("Events -> Mission (Runde " + r + ")", 1));
            add(wechsel("Mission -> Model (Runde " + r + ")", 2));
            add(wechsel("Model -> Map Editor (Runde " + r + ")", 3));
            add(warteBis("Leistung: Spieldaten", [] {
                ke::spielPruefen();
                return ke::spielBereit();
            }, 20000));
            add(wechsel("Map Editor -> Menu Editor (Runde " + r + ")", 4));
            add(wechsel("Menu Editor -> Events (Runde " + r + ")", 0));
        }
        add(tu("Leistung: Wechsel fertig", [] { speicher("nach den Wechseln"); }));
    }
    if (abschnitt('k')) {
        add(tu("Leistung: Mission-Tab", [] { g_app->leftMode = 1; }));
        add(pause(0.5));
        karteLaden(s, "maps/mp/duel1.bsp");
        add(bildzeit("Mission mp/duel1 ruhend", [] {}, nullptr));
        karteLaden(s, "maps/md_toaj_jedi.bsp");
        add(pause(1.0));
        add(bildzeit("Mission md_toaj_jedi ruhend", [] {}, nullptr));
        // Flug: Start zur fernsten Entity wie der Szenenlauf (-flug), 240 Bilder
        auto weg = std::make_shared<std::array<float, 6>>();
        add(bildzeit(
            "Mission md_toaj_jedi Flug",
            [weg] {
                std::vector<std::array<float, 3>> orte;
                for (const MapEntity& e : g_app->map.entities) {
                    float p[3];
                    if (entityOrt(e, p)) { orte.push_back({p[0], p[1], p[2]}); }
                }
                if (orte.size() < 2U) { return; }
                const std::array<float, 3> a0 = orte[0];
                std::array<float, 3> z = a0;
                float best = -1.0F;
                for (const auto& o : orte) {
                    const float d = (o[0] - a0[0]) * (o[0] - a0[0]) + (o[1] - a0[1]) * (o[1] - a0[1]);
                    if (d > best) {
                        best = d;
                        z = o;
                    }
                }
                *weg = {a0[0], a0[1], a0[2] + 64.0F, z[0], z[1], z[2] + 64.0F};
                g_app->playing = false;
                g_app->throughCamera = false;
                g_app->camViewActive = false;
            },
            [weg](int b) {
                const float t = static_cast<float>(b) / 252.0F;
                const auto& w = *weg;
                for (std::size_t i = 0; i < 3U; ++i) { g_app->cam.pos[i] = w[i] + (w[i + 3U] - w[i]) * t; }
                const float ziel[3] = {w[3], w[4], w[5]};
                aimAngles(g_app->cam.pos, ziel, g_app->cam.angles);
                g_app->cam.angles[0] = 0.0F;
                g_app->mapDirty = true;
            }));
        add(tu("Leistung: md_toaj_jedi feste Kamera", [weg] {
            const auto& w = *weg;
            vorschauKamera({w[0], w[1], w[2], 0.0F, 30.0F, 0.0F});
        }));
        add(warteBis("Leistung: gezeichnet", [] { return !g_app->mapDirty; }, 600));
        add(pause(0.3));
        add(vorschauBild("toaj"));
        add(leerlauf("Mission md_toaj_jedi"));
        add(vorratGegenprobe("md_toaj_jedi", false));   // nach den Bildern: sie aendert texCache und die Bilder der Szene auf der Grafikkarte
        add(tu("Leistung: Karten fertig", [] { speicher("nach md_toaj_jedi"); }));
    }
    if (abschnitt('s')) {
        // Szene mit vielen Figuren: md_am_sith, md_am/intro2_sith (wie tempo.py vorschau)
        auto t0 = std::make_shared<std::chrono::steady_clock::time_point>();
        auto dauer = std::make_shared<double>(0.0);
        add(tu("Leistung: Mission-Tab", [] { g_app->leftMode = 1; }));
        karteLaden(s, "maps/md_am_sith.bsp");
        add(tu("Leistung: Szene md_am/intro2_sith oeffnen", [t0] {
            std::string data;
            std::string gefunden;
            for (const char* ext : {".txt", ".icarus", ".ibi", ".IBI"}) {
                if (readFromArchives(std::string("scripts/md_am/intro2_sith") + ext, data)) {
                    gefunden = ext;
                    break;
                }
            }
            if (gefunden.empty()) {
                meldeFehler("Leistung: md_am/intro2_sith nicht im Archiv");
                return;
            }
            *t0 = std::chrono::steady_clock::now();
            openScriptFromMemory(data, "intro2_sith" + gefunden);
            g_app->camTrackValid = false;
            g_app->throughCamera = true;
            g_app->playing = false;
            g_app->playMs = 0.0;
            g_app->audioUpTo = -1.0;
        }));
        add(stufe9c::langeWarten("Leistung: Szene aufgebaut", [] { return g_app->camTrackValid && !g_app->mapDirty; }, 20000));
        add(tu("Leistung: Szene Zeit", [t0, dauer] {
            *dauer = std::max(g_app->camTrack.durationMs, std::min(g_app->timeline.practicalEndMs, 600000.0));
            char z[200];
            std::snprintf(z, sizeof(z), "Leistung: Szene aufbauen md_am/intro2_sith | %.0f ms bis zum ersten Bild, %zu Figuren, Dauer %.0f ms",
                          std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - *t0).count(), g_app->scene.actors.size(), *dauer);
            diag::detail(z);
            speicher("nach der Szene");
        }));
        // Feste Zeit, angehalten: ein Bild durch die Szenenkamera
        add(tu("Leistung: Szene auf 40 %", [dauer] {
            g_app->throughCamera = true;
            g_app->playMs = *dauer * 0.4;
            g_app->playing = false;
            g_app->mapDirty = true;
        }));
        add(warteBis("Leistung: gezeichnet", [] { return !g_app->mapDirty; }, 600));
        add(pause(0.3));
        add(vorschauBild("szene40"));
        add(foto("leistung_mission"));
        add(bildzeit("Mission Szene angehalten", [] {}, nullptr));
        add(bildzeit(
            "Mission Szene spielt",
            [dauer] {
                g_app->throughCamera = true;
                g_app->playMs = *dauer * 0.2;
                g_app->playing = true;
            },
            [](int) { g_app->playing = true; }));
        add(tu("Leistung: Szene anhalten", [] { g_app->playing = false; }));
        add(leerlauf("Mission Szene angehalten"));
        add(vorratGegenprobe("Figuren md_am/intro2_sith", true));   // nach den Bildern der Szene
        // Wetter und Nebel: duel_mustafar (Weather/duel_mustafar.cfg), feste Kamera
        karteLaden(s, "maps/duel_mustafar.bsp");
        add(tu("Leistung: duel_mustafar feste Kamera", [] { vorschauKamera({-1080.57F, 7376.57F, 76.25F, 0.0F, 315.0F, 0.0F}); }));
        add(warteBis("Leistung: gezeichnet", [] { return !g_app->mapDirty; }, 600));
        add(pause(0.3));
        add(vorschauBild("mustafar"));
        add(bildzeit("Mission duel_mustafar ruhend", [] {}, nullptr));
    }
    if (abschnitt('e')) {
        for (Schritt& x : stufe9c::einrichten()) { s.push_back(std::move(x)); }
        // Built map as in the game: duel_jeditemple als Vorlage, Kamera wie das Spielbild aus Teil 11
        add(tu("Leistung: Editor duel_jeditemple als Vorlage", [] {
            karteDialogeZu();
            ke::Editor& ed = ke::editor();
            ed.echtLicht.an = false;
            ed.echtLicht.backen = false;
            ed.echtLicht.spielbild = true;
            ed.karte.undo.alsGespeichertMarkieren();
            ke::neueKarte();
            std::string daten;
            if (!ed.karteSpiel || !ed.karteSpiel->fs.dateiLesen("maps/duel_jeditemple.bsp", daten)) {
                meldeFehler("Leistung: maps/duel_jeditemple.bsp nicht in den Spieldaten");
                return;
            }
            std::string f;
            if (!ke::vorlageLaden(daten, "duel_jeditemple.bsp", &f)) { meldeFehler("Leistung: Vorlage: " + f); }
            teil11::Paar p;
            const float kam[5] = {-1250.0F, -20.0F, -1113.75F, 0.0F, 90.0F};
            std::copy(kam, kam + 5, p.kam);
            teil11::kameraWieImSpiel(p);
        }));
        add(warteBis("Leistung: Texturen duel_jeditemple", [] { return ke::kameraBilderFertig(); }, 20000));
        add(pause(1.5));
        add(stufe9c::langeWarten("Leistung: Spielbild duel_jeditemple", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
        add(tu("Leistung: feste Shaderzeit", [] { ke::spielbildZeitFest(10.0); }));
        add(pause(0.3));
        add(kameraBild("editor_vorlage_spiel"));
        add(tu("Leistung: Uhr", [] { ke::spielbildZeitFest(-1.0); }));
        add(bildzeit("Map Editor Vorlage wie im Spiel ruhend", [] {}, nullptr));
        add(bildzeit("Map Editor Vorlage wie im Spiel Kamera dreht", [] {}, [](int b) {
            ke::Editor& ed = ke::editor();
            ed.kamera.freiBewegen(6, (b / 60) % 2 == 0 ? 1 : -1, ed.kameraE);
        }));
        add(tu("Leistung: Vorlage, Zeichner des Editors", [] {
            teil11::Paar p;
            const float kam[5] = {-1250.0F, -20.0F, -1113.75F, 0.0F, 90.0F};
            std::copy(kam, kam + 5, p.kam);
            teil11::kameraWieImSpiel(p);
            ke::editor().echtLicht.spielbild = false;
        }));
        add(pause(1.0));
        add(kameraBild("editor_vorlage"));
        add(leerlauf("Map Editor Vorlage"));
        add(tu("Leistung: Vorlage weg", [] {
            ke::editor().echtLicht.spielbild = true;
            ke::vorlageEntfernen();
            ke::Editor& ed = ke::editor();
            ed.kamFenster.messBreite = 0;
            ed.kamFenster.messHoehe = 0;
            ed.filterMaske = 0;
        }));
        if (const char* m = std::getenv("BHED_LEISTUNG_MAP"); m != nullptr && m[0] != '\0') {
            const std::string karte = m;
            auto t0 = std::make_shared<std::chrono::steady_clock::time_point>();
            add(tu("Leistung: Editor " + karte, [karte, t0] {
                karteDialogeZu();
                *t0 = std::chrono::steady_clock::now();
                if (!ke::karteOeffnen(karte)) { meldeFehler("Leistung: " + karte + " nicht geoeffnet"); }
                ke::Editor& e = ke::editor();
                e.kameraE.modus = karte::KameraModus::Textur;
                e.echtLicht.an = true;
                e.echtLicht.backen = false;
                const std::vector<stufe9c::Punkt> punkte = stufe9c::kameraPunkte();
                if (!punkte.empty()) { stufe9c::kameraAuf(punkte.front()); }
            }));
            add(warteBis("Leistung: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
            add(tu("Leistung: Editor Zeit", [karte, t0] {
                char z[300];
                std::snprintf(z, sizeof(z), "Leistung: Map Editor oeffnen %s | %.0f ms bis die Texturen fertig sind", karte.c_str(),
                              std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - *t0).count());
                diag::detail(z);
                speicher("nach der .map");
            }));
            add(pause(1.5));
            add(kameraBild("editor_echtlicht"));
            add(foto("leistung_editor"));
            add(bildzeit("Map Editor Echtzeit-Licht ruhend", [] {}, nullptr));
            add(bildzeit("Map Editor Echtzeit-Licht Kamera dreht", [] {}, [](int b) {
                ke::Editor& ed = ke::editor();
                ed.kamera.freiBewegen(6, (b / 60) % 2 == 0 ? 1 : -1, ed.kameraE);
            }));
            add(leerlauf("Map Editor Echtzeit-Licht"));
            add(tu("Leistung: Editor leeren", [] {
                ke::Editor& e = ke::editor();
                e.echtLicht.an = false;
                e.karte.undo.alsGespeichertMarkieren();
                ke::neueKarte();
            }));
        }
    }
    if (abschnitt('m')) {
        add(tu("Leistung: Model-Tab, stormtrooper", [] {
            g_app->leftMode = 2;
            std::string fehler;
            if (!oeffneModellAusArchiv("models/players/stormtrooper/model.glm", &fehler)) { meldeFehler("Leistung: stormtrooper: " + fehler); }
            m1Grundlage();
        }));
        add(pause(1.0));
        add(foto("leistung_modell"));
        add(bildzeit("Model stormtrooper ruhend", [] { m1Grundlage(); }, nullptr));
        add(bildzeit("Model stormtrooper Animation und Drehteller", [] {
            m1Abspielen();
            g_app->modellDrehteller = true;
        }, nullptr));
        add(tu("Leistung: Model ruhig", [] { m1Grundlage(); }));
        add(leerlauf("Model"));
    }
    if (abschnitt('n')) {
        add(tu("Leistung: Menue-Editor, ui/main.menu", [] {
            g_app->leftMode = 4;
            if (!menueeditor::oeffneAusSpiel("ui/main.menu")) { meldeFehler("Leistung: ui/main.menu nicht geoeffnet"); }
        }));
        add(pause(2.0));
        add(foto("leistung_menue"));
        add(bildzeit("Menu Editor ui/main.menu ruhend", [] {}, nullptr));
        add(bildzeit("Menu Editor ui/main.menu Maus bewegt", [] {}, [](int b) {
            const menueeditor::Stand st = menueeditor::stand();
            const float t = static_cast<float>(b % 120) / 120.0F;
            setzeMaus(ImVec2{st.flaeche[0] + st.flaeche[2] * (0.1F + 0.8F * t), st.flaeche[1] + st.flaeche[3] * 0.5F});
        }));
        add(leerlauf("Menu Editor"));
        add(tu("Leistung: Menue-Editor zu", [] {
            menueeditor::schliesseAlleOhneSpeichern();
            g_app->leftMode = 0;
        }));
    }
    add(tu("Leistung: Ende", [] {
        speicher("Ende");
        g_app->doc.markSaved();
    }));
    return s;
}

}  // namespace leistung
