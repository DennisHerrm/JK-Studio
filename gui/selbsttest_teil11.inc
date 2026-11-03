// selbsttest_teil11.inc - rc608 Teil 11 (Modus "teil11"): der Karten-Editor gegen das Spiel.
//
// Messung (BHED_K11_PAARE "name|archiv|bsp|x y z nick gier;...", Bilder nach BHED_FOTOS): dieselbe Karte und
// Kamera wie ein Spielbild (cg_fov 80 bei 4:3, Kamerabild 1920 x 1440). Je Paar drei Zustaende:
//   _echt     die dekompilierte Karte (File > Open .bsp) mit Echtzeit-Licht, ohne Bau
//   _gebaut   dieselbe Karte nach dem Schnell-Backen (eigene Lightmaps), Zeichner des Editors
//   _spiel    dasselbe mit "Built map as in the game" (Zeichner der Vorschau)
//   _vorlage / _vorlagespiel  die Original-.bsp als Vorlage: ihre eigenen Lightmaps, so wie sie auch das Spiel zeigt
// Die Editorhilfen (Lichter, Wege, Trigger, Clip, Hint, Portale, Punkt-Entities) sind ausgefiltert; die Bilder
// vergleicht SP/g11 mit den Spielbildern.
namespace teil11 {
namespace ke = karteneditor;

struct Paar {
    std::string name;
    std::string archiv;
    std::string bsp;
    float kam[5] = {0.0F, 0.0F, 0.0F, 0.0F, 0.0F};   // x y z nick gier wie viewpos im Spiel
    std::string daten;
    std::string datei;
};

inline std::vector<Paar> paare() {
    std::vector<Paar> aus;
    const char* e = std::getenv("BHED_K11_PAARE");
    std::istringstream in(e != nullptr ? e : "");
    for (std::string z; std::getline(in, z, ';');) {
        std::vector<std::string> teile;
        std::istringstream zz(z);
        for (std::string t; std::getline(zz, t, '|');) { teile.push_back(t); }
        if (teile.size() != 4U) { continue; }
        Paar p;
        p.name = teile[0];
        p.archiv = teile[1];
        p.bsp = teile[2];
        if (std::sscanf(teile[3].c_str(), "%f %f %f %f %f", &p.kam[0], &p.kam[1], &p.kam[2], &p.kam[3], &p.kam[4]) != 5) { continue; }
        aus.push_back(std::move(p));
    }
    return aus;
}

inline bool ausArchiv(const std::string& archiv, const std::string& name, std::string& aus) {
    Pk3 pk;
    if (!readPk3Directory(archiv, pk)) { return false; }
    for (const Pk3Entry& en : pk.entries) {
        if (fsPathCmp(en.name, name) == 0) { return readPk3File(pk, en, aus); }
    }
    return false;
}

// Die Kamera wie im Spiel: Ort, Blick (der Editor nickt mit umgekehrtem Vorzeichen), cg_fov 80 auf der Breite.
inline void kameraWieImSpiel(const Paar& p) {
    ke::Editor& ed = ke::editor();
    ed.kamFenster.messBreite = 1920;
    ed.kamFenster.messHoehe = 1440;
    ed.kameraE.sichtfeld = 80.0F;
    ed.kameraE.modus = karte::KameraModus::Textur;
    ed.kamera.ursprung = karte::Vektor3f(p.kam[0], p.kam[1], p.kam[2]);
    ed.kamera.winkel = karte::Vektor3f(-p.kam[3], p.kam[4], 0.0F);
    // Editorhilfen aus: was das Spiel nicht zeichnet
    ed.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_PATHS | karte::EXCLUDE_TRIGGERS | karte::EXCLUDE_CLIP | karte::EXCLUDE_HINTSSKIPS |
                     karte::EXCLUDE_AREAPORTALS | karte::EXCLUDE_CLUSTERPORTALS | karte::EXCLUDE_POINT_ENT | karte::EXCLUDE_BOTCLIP |
                     karte::EXCLUDE_LIGHTGRID | karte::EXCLUDE_VISPORTALS | karte::EXCLUDE_CAULK;
}

inline void bildSichern(const std::string& name) {
    stufe9c::Bild b;
    if (!ke::kameraBild(b.rgba, b.breite, b.hoehe)) {
        meldeFehler("Teil 11: Kamerabild nicht lesbar (" + name + ")");
        return;
    }
    const char* o = std::getenv("BHED_FOTOS");
    if (o != nullptr && o[0] != '\0') { stufe9c::bmpSchreiben(std::string(o) + "\\" + name + ".bmp", b); }
    const ke::Editor& ed = ke::editor();
    diag::detail("Knopftest: Teil 11 Bild " + name + " " + std::to_string(b.breite) + " x " + std::to_string(b.hoehe) + ", Spielbild " +
                 (ed.echtLicht.spielbildGezeigt ? "ja" : "nein") + ", gebacken " + (ed.echtLicht.gebackenZeigen ? "ja" : "nein") + ", Echtzeit " +
                 (ed.echtLicht.an ? "an" : "aus") + ", Aufrufe " + std::to_string(ke::spielbildStand().aufrufe) + ", " +
                 std::to_string(ke::spielbildStand().ms) + " ms");
}

inline std::vector<Schritt> messung() {
    std::vector<Schritt> s = stufe9c::einrichten();
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    // rc608 Teil 13b: BHED_K11_NUR_VORLAGE - nur die Original-.bsp als Vorlage (ohne Rueckgewinnung und Schnell-Backen)
    const bool nurVorlage = std::getenv("BHED_K11_NUR_VORLAGE") != nullptr;
    for (const Paar& vorgabe : paare()) {
        auto p = std::make_shared<Paar>(vorgabe);
        if (nurVorlage) {
            add(tu("Teil 11: " + p->name + " lesen", [p] {
                karteDialogeZu();
                if (!ausArchiv(p->archiv, p->bsp, p->daten)) { meldeFehler("Teil 11: " + p->bsp + " nicht in " + p->archiv); }
                p->datei = p->name + ".bsp";
            }));
        }
        if (!nurVorlage) {
            add(tu("Teil 11: " + p->name + " oeffnen", [p] {
                karteDialogeZu();
                ke::Editor& ed = ke::editor();
                ed.echtLicht.an = false;
                ed.echtLicht.backen = false;
                if (!ausArchiv(p->archiv, p->bsp, p->daten)) {
                    meldeFehler("Teil 11: " + p->bsp + " nicht in " + p->archiv);
                    return;
                }
                p->datei = (std::filesystem::current_path() / (p->name + ".bsp")).string();
                std::ofstream(p->datei, std::ios::binary).write(p->daten.data(), static_cast<std::streamsize>(p->daten.size()));
                // q3map2 -convert laesst die Lichter weg: geschaetzt wie in Stufe 9c (Rueckgewinnung, experimentell)
                ed.bspRueck.lichter = true;
                (void)ke::bspOeffnenDatei(p->datei);
                kameraWieImSpiel(*p);
                ed.echtLicht.an = true;
            }));
            add(stufe9c::langeWarten("Teil 11: Rueckgewinnung " + p->name, [] { return !ke::bspRueckLaeuft(); }, 400000));
            if (std::getenv("BHED_K11_OHNE_NEBEL") != nullptr) {
                // rc608 Teil 13b: den globalen Nebel (worldspawn _fog/fog) weglassen - das Echtzeit-Licht zeichnet ihn, der
                // Zeichner des Editors mit Lightmaps nicht; so bleibt im Vergleich Echtzeit/gebaut nur das Licht.
                add(tu("Teil 11: ohne globalen Nebel " + p->name, [] {
                    if (const auto welt = ke::editor().karte.welt()) {
                        welt->setzen("_fog", "");
                        welt->setzen("fog", "");
                    }
                }));
            }
            add(warteBis("Teil 11: Texturen " + p->name, [] { return ke::kameraBilderFertig(); }, 20000));
            add(pause(2.0));
            add(tu("Teil 11: Echtzeit " + p->name, [p] { bildSichern(p->name + "_echt"); }));
            add(tu("Teil 11: Schnell-Backen " + p->name, [] {
                ke::Editor& ed = ke::editor();
                ed.echtLicht.pauseSekunden = 0.2;
                ed.echtLicht.backen = true;
            }));
            add(stufe9c::langeWarten("Teil 11: gebacken " + p->name, [p] {
                const ke::Editor& ed = ke::editor();
                if (ed.echtLicht.bauStand == 5) {
                    meldeFehler("Teil 11: Schnell-Backen gescheitert (" + p->name + ")");
                    return true;
                }
                return ed.echtLicht.gebackenZeigen;
            }, 400000));
            add(warteBis("Teil 11: Texturen gebacken " + p->name, [] { return ke::kameraBilderFertig(); }, 20000));
            add(pause(1.0));
            add(tu("Teil 11: gebaut " + p->name, [] {
                ke::editor().echtLicht.spielbild = false;
            }));
            add(pause(0.5));
            add(stufe9c::langeWarten("Teil 11: gebacken (noch) " + p->name, [] { return ke::editor().echtLicht.gebackenZeigen; }, 400000));
            add(tu("Teil 11: gebaut, eigener Zeichner " + p->name, [p] {
                bildSichern(p->name + "_gebaut");
                ke::editor().echtLicht.spielbild = true;
            }));
            add(pause(1.0));
            add(stufe9c::langeWarten("Teil 11: gebacken (Spielbild) " + p->name, [] { return ke::editor().echtLicht.spielbildGezeigt; }, 400000));
            add(tu("Teil 11: gebaut wie im Spiel " + p->name, [p] { bildSichern(p->name + "_spiel"); }));
            if (std::getenv("BHED_K11_BILDZEIT") != nullptr) {
                // Bildzeit der Anwendung, Kamera dreht 240 Bilder: erst der Zeichner des Editors, dann das Spielbild
                for (const bool spiel : {false, true}) {
                    auto bilder = std::make_shared<std::vector<double>>();
                    auto spielMs = std::make_shared<double>(0.0);
                    add(tu("Teil 11: Bildzeit vorbereiten " + p->name, [spiel] { ke::editor().echtLicht.spielbild = spiel; }));
                    add(pause(1.0));
                    add({"Teil 11: 240 Bilder " + p->name, [bilder, spielMs, spiel](int b) {
                             ke::Editor& ed = ke::editor();
                             if (b > 2) {
                                 bilder->push_back(static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0);
                                 if (spiel) { *spielMs += ke::spielbildStand().ms; }
                             }
                             ed.kamera.freiBewegen(6, (b / 60) % 2 == 0 ? 1 : -1, ed.kameraE);
                             return b >= 242;
                         }});
                    add(tu("Teil 11: Bildzeit " + p->name, [bilder, spielMs, spiel, p] {
                        std::vector<double> v = *bilder;
                        std::sort(v.begin(), v.end());
                        double summe = 0;
                        for (const double x : v) { summe += x; }
                        const double mittel = v.empty() ? 0 : summe / static_cast<double>(v.size());
                        const double p99 = v.empty() ? 0 : v[std::min(v.size() - 1, v.size() * 99 / 100)];
                        char z[300];
                        std::snprintf(z, sizeof(z), "Teil 11 Bildzeit: %s %s Mittel %.2f ms, p99 %.2f ms, Spielbild auf dem Prozessor %.2f ms je Bild",
                                      p->name.c_str(), spiel ? "Spielbild" : "Editor", mittel, p99, v.empty() ? 0.0 : *spielMs / static_cast<double>(v.size()));
                        diag::detail(z);
                    }));
                }
            }
        }
        add(tu("Teil 11: Vorlage " + p->name, [p] {
            ke::Editor& ed = ke::editor();
            ed.echtLicht.backen = false;
            ed.echtLicht.an = false;
            ed.karte.undo.alsGespeichertMarkieren();   // keine Rueckfrage "Save changes?"
            ke::neueKarte();
        }));
        add(pause(0.3));
        add(tu("Teil 11: Vorlage laden " + p->name, [p] {
            std::string f;
            if (!ke::vorlageLaden(p->daten, p->datei, &f)) { meldeFehler("Teil 11: Vorlage " + p->name + ": " + f); }
            kameraWieImSpiel(*p);
        }));
        add(warteBis("Teil 11: Texturen Vorlage " + p->name, [] { return ke::kameraBilderFertig(); }, 20000));
        add(pause(1.0));
        add(tu("Teil 11: Vorlage-Bild " + p->name, [p] {
            bildSichern(p->name + "_vorlagespiel");
            ke::editor().echtLicht.spielbild = false;
        }));
        add(pause(0.5));
        add(tu("Teil 11: Vorlage, eigener Zeichner " + p->name, [p] {
            bildSichern(p->name + "_vorlage");
            ke::editor().echtLicht.spielbild = true;
            ke::vorlageEntfernen();
        }));
    }
    add(tu("Teil 11: Messgroesse zurueck", [] {
        ke::Editor& ed = ke::editor();
        ed.kamFenster.messBreite = 0;
        ed.kamFenster.messHoehe = 0;
        ed.filterMaske = 0;
        ed.karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
    }));
    return s;
}

// Mittelwert eines Rechtecks im Kamerabild (RGBA, Zeilen von oben)
inline std::array<double, 3> mittel(const stufe9c::Bild& b, int x0, int y0, int x1, int y1) {
    std::array<double, 3> s{0.0, 0.0, 0.0};
    long n = 0;
    for (int y = std::max(0, y0); y < std::min(b.hoehe, y1); ++y) {
        for (int x = std::max(0, x0); x < std::min(b.breite, x1); ++x) {
            const std::size_t at = (static_cast<std::size_t>(y) * static_cast<std::size_t>(b.breite) + static_cast<std::size_t>(x)) * 4U;
            for (int k = 0; k < 3; ++k) { s[static_cast<std::size_t>(k)] += b.rgba[at + static_cast<std::size_t>(k)]; }
            ++n;
        }
    }
    for (double& v : s) { v /= static_cast<double>(std::max(n, 1L)); }
    return s;
}

inline std::string text3(const std::array<double, 3>& v) {
    char z[64];
    std::snprintf(z, sizeof(z), "%.1f %.1f %.1f", v[0], v[1], v[2]);
    return z;
}

// Faktor je Kanal gegen einen Sollwert in einer Grenze
inline bool nahAm(const std::array<double, 3>& ist, const std::array<double, 3>& soll, double grenze) {
    for (std::size_t k = 0; k < 3; ++k) {
        if (std::fabs(ist[k] / soll[k] - 1.0) > grenze) { return false; }
    }
    return true;
}

// Ohne BHED_K11_PAARE: duel_jeditemple aus den Spieldaten als Vorlage, dieselbe Kamera wie das Spielbild aus Teil 10
// (g9_jtb, nur ein anderer Spielerstart; Movie Duels, screenshot_tga). Die Sollwerte sind dort gemessen.
inline std::vector<Schritt> pruefungen() {
    std::vector<Schritt> s = stufe9c::einrichten();
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto spiel = std::make_shared<stufe9c::Bild>();
    auto eigen = std::make_shared<stufe9c::Bild>();
    add(tu("Teil 11: duel_jeditemple als Vorlage", [] {
        karteDialogeZu();
        ke::Editor& ed = ke::editor();
        ed.echtLicht.an = false;
        ed.echtLicht.backen = false;
        ed.echtLicht.spielbild = true;
        ed.karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
        std::string daten;
        if (!ed.karteSpiel || !ed.karteSpiel->fs.dateiLesen("maps/duel_jeditemple.bsp", daten)) {
            meldeFehler("Teil 11: maps/duel_jeditemple.bsp nicht in den Spieldaten");
            return;
        }
        std::string f;
        if (!ke::vorlageLaden(daten, "duel_jeditemple.bsp", &f)) { meldeFehler("Teil 11: Vorlage: " + f); }
        Paar p;
        const float kam[5] = {-1250.0F, -20.0F, -1113.75F, 0.0F, 90.0F};
        std::copy(kam, kam + 5, p.kam);
        kameraWieImSpiel(p);
    }));
    add(warteBis("Teil 11: Texturen duel_jeditemple", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(1.5));
    add(stufe9c::langeWarten("Teil 11: Spielbild duel_jeditemple", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    add(tu("Teil 11: Spielbild lesen", [spiel] {
        if (!ke::kameraBild(spiel->rgba, spiel->breite, spiel->hoehe)) { meldeFehler("Teil 11: Kamerabild nicht lesbar"); }
        ke::editor().echtLicht.spielbild = false;
    }));
    add(pause(1.0));
    add(tu("Teil 11: Bild des Editor-Zeichners lesen", [eigen] {
        if (!ke::kameraBild(eigen->rgba, eigen->breite, eigen->hoehe)) { meldeFehler("Teil 11: Kamerabild nicht lesbar"); }
        ke::editor().echtLicht.spielbild = true;
    }));
    // Spiel: Boden rechts (1250,1000)-(1920,1440) 32,1 25,8 13,9
    add(pruefe("Teil 11: Built map as in the game - Boden wie im Spielbild (32,1 25,8 13,9, hoechstens 3 % daneben)", [spiel] {
        const auto b = mittel(*spiel, 1250, 1000, 1920, 1440);
        diag::detail("Knopftest: Teil 11 Boden rechts " + text3(b));
        return spiel->breite == 1920 && nahAm(b, {32.1, 25.8, 13.9}, 0.03);
    }));
    // R_RadixSort: u_metalorange02 mit allen Stufen vor env_glass - das Glas ueber dem Metall (Fenster rechts 102,9 97,3 93,6,
    // Bogen links 69,5 63,8 59,4); R_MarkLeaves: durch das mittlere Fenster der Himmel (24,7 28,1 37,1), nicht der Raum dahinter
    add(pruefe("Teil 11: Fenster wie im Spielbild - Glas nach dem ganzen Metall, hinter dem Fenster nur, was im PVS liegt (je hoechstens 5 % daneben)",
               [spiel] {
                   const auto rechts = mittel(*spiel, 1246, 316, 1255, 325);
                   const auto bogen = mittel(*spiel, 896, 228, 905, 237);
                   const auto mitte = mittel(*spiel, 986, 316, 995, 325);
                   diag::detail("Knopftest: Teil 11 Fenster rechts " + text3(rechts) + ", Bogen " + text3(bogen) + ", Mitte " + text3(mitte) + ", verdeckt " +
                                std::to_string(ke::spielbildStand().flaechenVerdeckt));
                   return nahAm(rechts, {102.9, 97.3, 93.6}, 0.05) && nahAm(bogen, {69.5, 63.8, 59.4}, 0.05) && nahAm(mitte, {24.7, 28.1, 37.1}, 0.05);
               }));
    // Der Zeichner des Editors (Schalter aus) nimmt die Lightmap jetzt einfach (gpu::kSpielLightmapFaktor), vorher doppelt
    // (gemessen 53,7 41,3 17,5, also 1,67 1,60 1,26 mal das Spielbild). Mal 1 gemessen: 26,8 20,6 8,7 (0,83 0,80 0,63 mal das
    // Spielbild - ohne Gamma und Overbright des Spiels). Beidseitig: hoechstens 1,1 mal, mindestens die Haelfte; ein
    // schwarzes oder halb so helles Bild besteht nicht.
    add(pruefe("Teil 11: Zeichner des Editors - Lightmap mal 1: Boden zwischen 0,5 und 1,1 mal dem Spielbild", [eigen] {
        const auto b = mittel(*eigen, 1250, 1000, 1920, 1440);
        diag::detail("Knopftest: Teil 11 Editor-Zeichner Boden rechts " + text3(b));
        const std::array<double, 3> spiel = {32.1, 25.8, 13.9};
        bool ok = eigen->breite == 1920;
        for (std::size_t k = 0; k < 3; ++k) { ok = ok && b[k] <= spiel[k] * 1.1 && b[k] >= spiel[k] * 0.5; }
        return ok;
    }));
    add(tu("Teil 11: aufraeumen", [] {
        ke::vorlageEntfernen();
        ke::Editor& ed = ke::editor();
        ed.kamFenster.messBreite = 0;
        ed.kamFenster.messHoehe = 0;
        ed.filterMaske = 0;
    }));
    return s;
}

inline std::vector<Schritt> schritte() {
    if (std::getenv("BHED_K11_PAARE") != nullptr) { return messung(); }
    return pruefungen();
}

}  // namespace teil11
