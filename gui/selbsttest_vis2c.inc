// selbsttest_vis2c.inc - vis2c: Bauen anhalten und fortsetzen im Karten-Editor (eigene Funktion, nicht in NRC).
//
// Eingebunden in gui/selbsttest.cpp. BHED_EDITORTEST=karteneditor mit BHED_KARTE_NUR=vis2c1, vis2c2, vis2c3: drei
// Programmstarts mit EINEM Laufordner (der Zwischenstand liegt neben der Karte und muss den Neustart ueberstehen):
//   Lauf 1: Bezug bauen (BSP, VIS, LIGHT mit vier Bounces); noch einmal bauen und mitten im VIS mit dem Knopf im
//           Fenster Baufortschritt anhalten - Zwischenstand da, das VIS hat fertige und offene Portale.
//   Lauf 2: der Zwischenstand ist nach dem Neustart da; Build > Bauen fortsetzen... > "Karte wie angehalten": das VIS
//           macht mit dem Zwischenstand weiter, die .bsp ist Lump fuer Lump die des Bezugs. Dann ohne Licht-Cache
//           neu bauen und im Licht nach zwei fertigen Durchgaengen ueber das Rechtsklick-Menue anhalten.
//   Lauf 3: "aktuelle Karte": alle Schritte neu, das Licht nimmt die zwei geretteten Durchgaenge aus dem Cache -
//           gleich dem Bezug; waehrend des Baus bleibt der Editor bedienbar (Duplicate, Undo). Zuletzt anhalten
//           und "Zwischenstand verwerfen".
// BHED_EDITORTEST=handbuch mit BHED_HB_NUR=vis2c: die Bilder h_vis2c_fortsetzen und h_vis2c_fortschritt.
namespace vis2c {

namespace ke = karteneditor;
using stufe3::bspLumps;
using stufe3::konsoleText;
using stufe3::q3map2Bereitstellen;
using stufe8::fensterSichtbar;
using stufe8::vorlageAnfang;
using stufe8::warteSek;

constexpr const char* kEintrag = "Test Pause";
constexpr int kGitter = 9;   // Felder je Seite der Halle: 70 Brushes, rund 900 Portale

std::filesystem::path ordner() { return std::filesystem::current_path() / "vis2c"; }
std::string stamm() { return (ordner() / "gitter").generic_string(); }

std::string quader(int x0, int y0, int z0, int x1, int y1, int z1, const char* tex) {
    const int e[6][9] = {{x1, y1, z1, x1, y0, z1, x0, y1, z1}, {x1, y1, z1, x0, y1, z1, x1, y1, z0}, {x1, y1, z1, x1, y1, z0, x1, y0, z1},
                         {x0, y0, z0, x1, y0, z0, x0, y1, z0}, {x0, y0, z0, x0, y0, z1, x1, y0, z0}, {x0, y0, z0, x0, y1, z0, x0, y0, z1}};
    std::string t = "{\n";
    for (const auto& f : e) {
        char z[200];
        std::snprintf(z, sizeof(z), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n", f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], tex);
        t += z;
    }
    return t + "}\n";
}

// Eine Halle (n x n Felder zu 256 Einheiten) mit niedrigen Bloecken an jedem Gitterpunkt und einem Licht in jedem
// zweiten Feld: offen - jedes Portal sieht fast jedes, ein Thread rechnet das VIS darauf eine halbe Minute (ein Gitter aus
// Raeumen mit Tueren war in unter einer Sekunde fertig). Grobe Lightmaps (_lightmapscale), damit ein Licht-Durchgang
// Sekunden dauert, nicht Minuten. Gemessen mit Y:\werkzeuge\vis2\skripte\halle.py.
std::string gitterKarte(int n) {
    const int r = 256;
    const int t = 16;
    const int h = 256;
    const int w = n * r;
    std::string b = quader(-t, -t, -t, w + t, w + t, 0, "test/boden") + quader(-t, -t, h, w + t, w + t, h + t, "test/decke") +
                    quader(-t, -t, 0, 0, w + t, h, "test/wand") + quader(w, -t, 0, w + t, w + t, h, "test/wand") +
                    quader(0, -t, 0, w, 0, h, "test/wand") + quader(0, w, 0, w, w + t, h, "test/wand");
    for (int i = 1; i < n; ++i) {
        for (int j = 1; j < n; ++j) {
            const int x = i * r - 32 + ((i + j) % 2 != 0 ? 16 : 0);
            const int y = j * r - 32 + (i % 3 == 0 ? 16 : 0);
            b += quader(x, y, 0, x + 64, y + 64, 48 + 16 * ((i * 7 + j * 3) % 6), "test/wand");
        }
    }
    std::string text = "// BehavEd vis2c: Testkarte des Selbsttests (gui/selbsttest_vis2c.inc)\n{\n\"classname\" \"worldspawn\"\n\"_lightmapscale\" \"4\"\n" + b +
                       "}\n{\n\"classname\" \"info_player_start\"\n\"origin\" \"64 64 64\"\n}\n";
    for (int i = 0; i < n; i += 2) {
        for (int j = 0; j < n; j += 2) {
            char z[160];
            std::snprintf(z, sizeof(z), "{\n\"classname\" \"light\"\n\"origin\" \"%d %d 200\"\n\"light\" \"400\"\n}\n", i * r + 128, j * r + 128);
            text += z;
        }
    }
    return text;
}

bool da(const std::string& pfad) {
    std::error_code ec;
    return std::filesystem::exists(std::filesystem::path(pfad), ec);
}

bool konsoleMit(Str id) { return konsoleText().find(vorlageAnfang(id)) != std::string::npos; }

bool gleichBezug() {
    const auto a = bspLumps(stamm() + ".bsp");
    const auto b = bspLumps((ordner() / "bezug.bsp").string());
    std::string anders;
    for (std::size_t i = 0; i < a.size() && i < b.size(); ++i) {
        if (a[i] != b[i]) { anders += " " + std::to_string(i); }
    }
    diag::detail("Knopftest: vis2c - Lumps " + std::to_string(a.size()) + "/" + std::to_string(b.size()) + (anders.empty() ? ", alle gleich" : ", anders:" + anders));
    return a.size() == 18 && b.size() == 18 && anders.empty();
}

// In den Karten-Editor; Build-Menue frisch mit dem Eintrag "Test Pause"; die Karte (in Lauf 1 neu geschrieben) offen
std::vector<Schritt> eintritt(bool neu, int felder = kGitter) {
    std::vector<Schritt> s = stufe8b::splitEintritt();
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(warteBis("vis2c: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu(std::string("vis2c: bhed_q3map2, Build-Menue frisch, Eintrag Test Pause, Gitterkarte ") + (neu ? "neu" : "wie im Lauf davor"), [neu, felder] {
        karteDialogeZu();
        q3map2Bereitstellen();
        ke::Editor& e = ke::editor();
        std::error_code ec;
        if (neu) {
            std::filesystem::remove_all(ordner(), ec);
            std::filesystem::create_directories(ordner(), ec);
            std::ofstream f(ordner() / "gitter.map", std::ios::binary | std::ios::trunc);
            const std::string t = gitterKarte(felder);
            f.write(t.data(), static_cast<std::streamsize>(t.size()));
        }
        std::filesystem::remove(std::filesystem::path(ke::bauMenueDateiPfad()), ec);
        std::filesystem::remove(std::filesystem::path(stufe8::bauZeitenDatei()), ec);
        g_app->settings.karte.erase("BuildMenu");
        e.bauProjektFuer = -2;
        e.bauGeaendert = false;
        ke::bauMenueSicherstellen();
        e.bau = ke::Editor::BauEinstellungen{};
        e.bau.visExakt = true;                     // deterministisch: fortgesetzt Byte fuer Byte gleich
        e.bau.lichtWieder = true;                  // -bhedlightcache: vom Licht bleiben die fertigen Durchgaenge
        e.bau.gpuBacken = karte::GpuBacken::Aus;   // ohne Grafikkarte im Spiel
        ke::einstellungenSpeichern();
        (void)ke::karteOeffnen((ordner() / "gitter.map").string());
        ke::konsoleLeeren();
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{kEintrag,
                                                           {"[q3map2] -meta \"[MapFile]\"", "[q3map2] -vis -saveprt -threads 1 \"[MapFile]\"",
                                                            "[q3map2] -light -samples 2 -bounce 4 -threads 2 \"[MapFile]\""}});
    }));
    add(pause(0.5));
    return s;
}

Schritt bauen() { return stufe3::bauMenuepunkt(kEintrag); }

Schritt fertig(const std::string& was, double sekunden = 600) {
    return warteSek("vis2c: " + was, [] { return !ke::bauLaeuft(); }, sekunden);
}

// Der Tooltip des Elements unter der Maus ist da
bool tooltipDa() {
    const ImGuiWindow* t = ImGui::FindWindowByName("##Tooltip_00");
    return t != nullptr && t->Active;
}

Schritt mausAuf(const std::string& label, const std::string& fenster) {
    return tu("vis2c: Maus auf \"" + label + "\"", [label, fenster] {
        if (const Element* el = finde(label, fenster, 0, false)) {
            setzeMaus(el->rect.GetCenter());
        } else {
            meldeFehler("vis2c: \"" + label + "\" in " + fenster + " nicht gefunden");
        }
    });
}

std::vector<Schritt> lauf1() {
    std::vector<Schritt> s = eintritt(true);
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(pruefe("vis2c 1: ohne Bau nichts anzuhalten, kein Zwischenstand", [] { return !ke::bauAnhaltbar() && !ke::bauPauseStand().da; }));
    // Ein Bauprozess, der erst nach 7 s verbindet (unter Last braucht q3map2 so lange bis zum Verbinden): solange er
    // lebt, kein "timeout" nach 5 s
    add(tu("vis2c 1: Eintrag, dessen Befehl 7 s wartet, bevor q3map2 startet", [] {
        ke::editor().bauProjekt.eintraege.push_back(
            karte::BauEintrag{"Test langsamer Start", {"cmd /c ping -n 8 127.0.0.1 >NUL & [q3map2] -meta \"[MapFile]\""}});
    }));
    add(stufe3::bauMenuepunkt("Test langsamer Start"));
    add(warteSek("vis2c 1: der Bau mit dem langsamen Start ist zu Ende", [] { return !ke::bauLaeuft(); }, 300));
    add(pruefe("vis2c 1: langsamer Start - verbunden nach mehr als 5 s, fertig, kein \"timeout\"", [] {
        const std::string k = konsoleText();
        diag::detail("Knopftest: vis2c 1 - langsamer Start: " + k.substr(k.size() > 200 ? k.size() - 200 : 0));
        return k.find("Connected.") != std::string::npos && k.find("(timeout)") == std::string::npos && konsoleMit(Str::KeBmFertig) && da(stamm() + ".bsp");
    }));
    add(tu("vis2c 1: Konsole leer", [] { ke::konsoleLeeren(); }));
    // Der Bau beginnt, dann steht das Programm 6 s (ein langes Bild): q3map2 wartet laengst am Socket - kein "timeout"
    add(tu("vis2c 1: Build > Test Pause, danach ein Bild von 6 s", [] {
        const int i = stufe3::eintragNr(kEintrag);
        if (i >= 0) { ke::bauStarten(static_cast<std::size_t>(i)); }
        std::this_thread::sleep_for(std::chrono::seconds(6));
    }));
    add(pause(0.5));
    add(pruefe("vis2c 1: nach dem langen Bild laeuft der Bau (die wartende Verbindung wird vor der Zeitgrenze angenommen)", [] {
        return ke::bauLaeuft() && konsoleText().find("(timeout)") == std::string::npos;
    }));
    add(fertig("Bezug gebaut"));
    add(pruefe("vis2c 1: Bezug - Bau fertig, die .bsp da, der VIS-Schritt lief mit -bhedvisstate und liess keinen Zwischenstand", [] {
        std::error_code ec;
        std::filesystem::copy_file(std::filesystem::path(stamm() + ".bsp"), ordner() / "bezug.bsp", std::filesystem::copy_options::overwrite_existing, ec);
        const std::string k = konsoleText();
        diag::detail("Knopftest: vis2c 1 - Ende der Konsole: " + k.substr(k.size() > 300 ? k.size() - 300 : 0));
        return !ec && konsoleMit(Str::KeBmFertig) && k.find("-bhedvisstate") != std::string::npos && k.find("-bhedvisexact") != std::string::npos &&
               k.find("-bhedlightcache") != std::string::npos && !da(stamm() + ".bhedvisstate") && !da(stamm() + ".bhedprelight") &&
               !da(stamm() + ".bhedpause") && bspLumps(stamm() + ".bsp").size() == 18;
    }));
    // noch einmal bauen, mitten im VIS anhalten (Knopf im Fenster Baufortschritt)
    add(tu("vis2c 1: Konsole leer", [] { ke::konsoleLeeren(); }));
    add(bauen());
    add(warteSek("vis2c 1: VIS im Fluss, der Pacifier laeuft trotz der Zeile \"vis state not used\"", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return st.laeuft && st.stufe == "VIS" && st.phase == "PassagePortalFlow" && st.pacifier >= 0.1;
    }, 300));
    add(pruefe("vis2c 1: im VIS anhaltbar, [Bauen anhalten] und [Bauen abbrechen] im Fenster ganz zu sehen, der Balken unter 100 %", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        const Element* a = finde(ke::trK(Str::KeBpAnhalten), "kartebaufortschritt", 0, false);
        const Element* b = finde(ke::trK(Str::KeBmAbbrechen), "kartebaufortschritt", 0, false);
        diag::detail("Knopftest: vis2c 1 - Pacifier " + std::to_string(st.pacifier) + ", anhaltbar " + std::to_string(st.anhaltbar) + ", -bhedvisstate " +
                     std::to_string(st.visStandAn));
        return st.anhaltbar && st.visStandAn && ke::bauAnhaltbar() && a != nullptr && b != nullptr && ganzSichtbar(*a) && ganzSichtbar(*b) && st.pacifier < 1.0;
    }));
    add(mausAuf(ke::trK(Str::KeBpAnhalten), "kartebaufortschritt"));
    add(pause(0.7));
    add(pruefe("vis2c 1: [Bauen anhalten] hat einen Tooltip", [] { return tooltipDa() && std::string(ke::trK(Str::KeBpAnhaltenTip)).size() > 80; }));
    add(klick(ke::trK(Str::KeBpAnhalten), "kartebaufortschritt"));
    add(fertig("im VIS angehalten", 120));
    add(pruefe("vis2c 1: angehalten - Zeile in der Konsole, Zwischenstand mit Karte, .bsp, .prt, .srf und VIS-Stand, fertige und offene Portale", [] {
        const ke::BauPauseStand ps = ke::bauPauseStand();
        const std::string p = stamm() + ".bhedpause/gitter";
        diag::detail("Knopftest: vis2c 1 - Zwischenstand " + std::to_string(ps.da) + ", Schritt " + std::to_string(ps.pause.schritt) + ", Portale " +
                     std::to_string(ps.pause.visFertig) + "/" + std::to_string(ps.pause.visAlle) + ", " + std::to_string(ps.bytes) + " Bytes");
        return konsoleMit(Str::KeBpAngehalten) && !konsoleMit(Str::KeBmFertig) && ps.da && ps.pause.schritt == 1 && ps.pause.eintrag == kEintrag &&
               ps.pause.visFertig > 0 && ps.pause.visFertig < ps.pause.visAlle && !ps.karteGeaendert && da(p + ".map") && da(p + ".bsp") && da(p + ".prt") &&
               da(p + ".srf") && da(p + ".bhedvisstate") && !da(stamm() + ".bhedvisstate") && !da(stamm() + ".bhedvisstop") &&
               !fensterSichtbar("###kartebaufortschritt") && !ke::bauAnhaltbar();
    }));
    return s;
}

std::vector<Schritt> lauf2() {
    std::vector<Schritt> s = eintritt(false);
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(pruefe("vis2c 2: nach dem Neustart ist der Zwischenstand da (Schritt 2 von 3, VIS), die Karte unveraendert", [] {
        const ke::BauPauseStand ps = ke::bauPauseStand();
        return ps.da && ps.pause.schritt == 1 && ps.pause.befehle.size() == 3 && !ps.karteGeaendert && ke::bauPauseZeilen(ps).size() == 3;
    }));
    // Build > Bauen fortsetzen... (Menue, Tooltip), das Fenster mit drei Knoepfen
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.3));
    add(pruefe("vis2c 2: Build > Bauen anhalten (aus) und Bauen fortsetzen... (an) stehen im Menue", [] {
        return finde(ke::trK(Str::KeBpAnhalten), "##Menu_", 0, false) != nullptr && finde(ke::trK(Str::KeBpFortsetzen), "##Menu_", 0, false) != nullptr &&
               !ke::bauAnhaltbar();
    }));
    add(mausAuf(ke::trK(Str::KeBpFortsetzen), "##Menu_"));
    add(pause(0.7));
    add(pruefe("vis2c 2: Bauen fortsetzen... hat einen Tooltip", [] { return tooltipDa(); }));
    add(klick(ke::trK(Str::KeBpFortsetzen), "##Menu_"));
    add(pause(0.5));
    add(pruefe("vis2c 2: Fenster \"Bauen fortsetzen\" - drei Knoepfe ganz zu sehen", [] {
        bool ok = fensterSichtbar("###kartebaufortsetzen") && ke::editor().bauFortsetzenOffen;
        for (const Str k : {Str::KeBpAktuell, Str::KeBpAlt, Str::KeBpVerwerfen}) {
            const Element* el = finde(ke::trK(k), "kartebaufortsetzen", 0, false);
            ok = ok && el != nullptr && ganzSichtbar(*el);
        }
        return ok;
    }));
    add(mausAuf(ke::trK(Str::KeBpAlt), "kartebaufortsetzen"));
    add(pause(0.7));
    add(pruefe("vis2c 2: [Karte wie angehalten] hat einen Tooltip", [] { return tooltipDa(); }));
    add(tu("vis2c 2: Konsole leer", [] { ke::konsoleLeeren(); }));
    add(klick(ke::trK(Str::KeBpAlt), "kartebaufortsetzen"));
    add(warteSek("vis2c 2: das VIS macht mit dem Zwischenstand weiter (Schritt 2 von 3), Anzeige im Fenster", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return st.laeuft && st.schritt == 1 && st.schritte == 3 && st.visStand.zustand == karte::BauVisStand::Zustand::Fortgesetzt &&
               st.visStand.fertig > 0 && st.visStand.exakt && !ke::bauVisStandText(st).empty() && st.pacifier >= st.visStand.anteil - 0.03;
    }, 120));
    add(fertig("\"Karte wie angehalten\" fertig"));
    add(pruefe("vis2c 2: \"Karte wie angehalten\" - .bsp Lump fuer Lump gleich dem Bau ohne Pause, Zwischenstand weg", [] {
        return gleichBezug() && konsoleMit(Str::KeBmFertig) && konsoleMit(Str::KeBpAltFertig) && !da(stamm() + ".bhedpause") && !ke::bauPauseStand().da &&
               !fensterSichtbar("###kartebaufortsetzen");
    }));
    // ohne Licht-Cache neu bauen (wie ein erster Bau), im Licht nach zwei fertigen Durchgaengen anhalten - Rechtsklick im Fenster
    add(tu("vis2c 2: Licht-Cache weg, Konsole leer", [] {
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(stamm() + ".bhedlight"), ec);
        ke::konsoleLeeren();
    }));
    add(bauen());
    add(warteSek("vis2c 2: LIGHT, zwei Durchgaenge geschrieben, der dritte laeuft", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return st.laeuft && st.stufe == "LIGHT" && st.lichtCache.geschrieben >= 2;
    }, 600));
    add(klickAn("vis2c 2: Rechtsklick im Fenster Baufortschritt", [] {
        const ImRect r = hb::fensterRect("###kartebaufortschritt");
        return ImVec2{r.GetCenter().x, r.Min.y + 56.0F * ke::massstab()};
    }, 1));
    add(pause(0.3));
    add(pruefe("vis2c 2: Rechtsklick im Fenster Baufortschritt - Bauen anhalten und Bauen abbrechen", [] {
        return finde(ke::trK(Str::KeBpAnhalten), "##Popup_", 0, false) != nullptr && finde(ke::trK(Str::KeBmAbbrechen), "##Popup_", 0, false) != nullptr;
    }));
    add(klick(ke::trK(Str::KeBpAnhalten), "##Popup_"));
    add(fertig("im Licht angehalten", 120));
    add(pruefe("vis2c 2: im Licht angehalten - der Cache der fertigen Durchgaenge ist gerettet, die .bsp die von vor dem Licht", [] {
        const ke::BauPauseStand ps = ke::bauPauseStand();
        diag::detail("Knopftest: vis2c 2 - Schritt " + std::to_string(ps.pause.schritt) + ", Licht-Durchgaenge " + std::to_string(ps.pause.lichtPaesse) + "/" +
                     std::to_string(ps.pause.lichtPaesseVon));
        return ps.da && ps.pause.schritt == 2 && ps.pause.lichtPaesse >= 2 && ps.pause.lichtPaesse < 5 && ps.pause.lichtPaesseVon == 5 &&
               konsoleMit(Str::KeBpLichtGerettet) && konsoleMit(Str::KeBpAngehalten) && da(stamm() + ".bhedlight") && !da(stamm() + ".bhedlight.neu") &&
               !da(stamm() + ".bhedprelight") && da(stamm() + ".bhedpause/gitter.bsp") && ke::bauPauseZeilen(ps).size() == 3;
    }));
    return s;
}

std::vector<Schritt> lauf3() {
    std::vector<Schritt> s = eintritt(false);
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto bilder = std::make_shared<int>(0);
    auto brushes = std::make_shared<std::size_t>(0);
    add(pruefe("vis2c 3: nach dem Neustart ist der Zwischenstand da (Schritt 3 von 3, LIGHT, Durchgaenge im Cache)", [] {
        const ke::BauPauseStand ps = ke::bauPauseStand();
        return ps.da && ps.pause.schritt == 2 && ps.pause.lichtPaesse >= 2;
    }));
    add(tu("vis2c 3: Fenster \"Bauen fortsetzen\"", [] { ke::bauFortsetzenOeffnen(); }));
    add(pause(0.5));
    add(klick(ke::trK(Str::KeBpAktuell), "kartebaufortsetzen"));
    add(warteSek("vis2c 3: \"aktuelle Karte\" - alle Schritte neu, das Licht findet die geretteten Durchgaenge", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return st.laeuft && st.schritt == 2 && st.lichtCache.zustand == karte::BauLichtCache::Zustand::Gefunden && st.lichtCache.behalten >= 2 &&
               st.lichtCache.pass >= 1;
    }, 600));
    // der Editor bleibt waehrend des Baus bedienbar: eine Aenderung an der Karte und zurueck
    add(tu("vis2c 3: waehrend des Baus alles waehlen, duplizieren", [bilder, brushes] {
        *bilder = ImGui::GetFrameCount();
        *brushes = ke::editor().karte.welt()->kinder().size();
        (void)ke::ausfuehren("InvertSelection");
        (void)ke::ausfuehren("CloneSelection");
    }));
    add(pause(0.5));
    add(pruefe("vis2c 3: der Bau laeuft, die Karte hat doppelt so viele Brushes, der Editor zeichnet weiter", [bilder, brushes] {
        const ke::Editor& e = ke::editor();
        const std::size_t n = e.karte.welt()->kinder().size();
        diag::detail("Knopftest: vis2c 3 - waehrend des Baus " + std::to_string(n) + " Brushes, " + std::to_string(ImGui::GetFrameCount() - *bilder) +
                     " Bilder in 0,5 s");
        // gewaehlt sind die Kopien: die Brushes und die Punkt-Entities (Lichter, Startpunkt)
        return ke::bauLaeuft() && *brushes >= 70 && n == 2 * *brushes && static_cast<std::size_t>(e.karte.auswahl.anzahl()) >= *brushes &&
               ImGui::GetFrameCount() - *bilder >= 3;
    }));
    add(tu("vis2c 3: Undo, Auswahl weg", [] {
        (void)ke::ausfuehren("Undo");
        (void)ke::ausfuehren("UnSelectSelection");
    }));
    add(pruefe("vis2c 3: nach Undo wieder die Brushes von vorher", [brushes] { return ke::editor().karte.welt()->kinder().size() == *brushes; }));
    add(fertig("\"aktuelle Karte\" fertig"));
    add(pruefe("vis2c 3: \"aktuelle Karte\" - .bsp Lump fuer Lump gleich dem Bau ohne Pause, Zwischenstand weg", [] {
        return gleichBezug() && konsoleMit(Str::KeBmFertig) && konsoleMit(Str::KeBpAktuellStart) && !da(stamm() + ".bhedpause") && !da(stamm() + ".bhedprelight");
    }));
    // ohne Build-Ueberwachung (die Ausgabe von q3map2 kommt ueber die Pipe, RunBatch): mitten im VIS anhalten, dann verwerfen
    add(tu("vis2c 3: Konsole leer, Build-Ueberwachung aus", [] {
        ke::konsoleLeeren();
        ke::editor().bau.ueberwachen = false;
    }));
    add(bauen());
    add(warteSek("vis2c 3: ohne Ueberwachung - VIS im Fluss", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return st.laeuft && st.stufe == "VIS" && st.phase == "PassagePortalFlow" && st.pacifier >= 0.1 && ke::bauAnhaltbar();
    }, 300));
    add(tu("vis2c 3: Build > Bauen anhalten", [] { ke::bauAnhalten(); }));
    add(fertig("angehalten", 120));
    add(tu("vis2c 3: Fenster \"Bauen fortsetzen\", Build-Ueberwachung wieder an", [] {
        ke::bauFortsetzenOeffnen();
        ke::editor().bau.ueberwachen = true;
        ke::einstellungenSpeichern();
    }));
    add(pause(0.5));
    add(pruefe("vis2c 3: ohne Ueberwachung im VIS angehalten - Zwischenstand mit fertigen und offenen Portalen", [] {
        const ke::BauPauseStand ps = ke::bauPauseStand();
        diag::detail("Knopftest: vis2c 3 - ohne Ueberwachung: Schritt " + std::to_string(ps.pause.schritt) + ", Portale " + std::to_string(ps.pause.visFertig) +
                     "/" + std::to_string(ps.pause.visAlle));
        return ps.da && da(stamm() + ".bhedpause/stand.json") && ps.pause.schritt == 1 && ps.pause.visFertig > 0 && ps.pause.visFertig < ps.pause.visAlle &&
               konsoleMit(Str::KeBpAngehalten) && !da(stamm() + ".bhedvisstop");
    }));
    add(klick(ke::trK(Str::KeBpVerwerfen), "kartebaufortsetzen"));
    add(pause(0.7));
    add(pruefe("vis2c 3: \"Zwischenstand verwerfen\" - Ordner weg, Meldung, das Fenster hat keine Knoepfe mehr", [] {
        return !ke::bauPauseStand().da && !da(stamm() + ".bhedpause") && konsoleMit(Str::KeBpVerworfen) && da(stamm() + ".map") &&
               fensterSichtbar("###kartebaufortsetzen") && finde(ke::trK(Str::KeBpAktuell), "kartebaufortsetzen", 0, false) == nullptr;
    }));
    add(tu("vis2c 3: Fenster zu, zurueck zu Events", [] {
        karteDialogeZu();
        g_app->leftMode = 0;
    }));
    return s;
}

std::vector<Schritt> schritte(int lauf) { return lauf == 1 ? lauf1() : lauf == 2 ? lauf2() : lauf3(); }

// Handbuch: h_vis2c_fortsetzen (das Fenster "Bauen fortsetzen" nach dem Anhalten im VIS) und h_vis2c_fortschritt
// (der Baufortschritt des fortgesetzten Baus mit [Bauen anhalten])
std::vector<Schritt> handbuchSchritte() {
    // eine groessere Halle als im Selbsttest: das fortgesetzte VIS muss fuer das Foto auch auf einem ruhigen Rechner noch
    // einige Sekunden laufen (mit 9 Feldern zeigten drei von sechs Bildern schon den Licht-Schritt)
    std::vector<Schritt> s = eintritt(true, 11);
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const auto ganz = [] {
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        return haupt != nullptr ? haupt->Rect() : ImRect{};
    };
    add(bauen());
    add(warteSek("Handbuch vis2c: VIS im Fluss", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return st.laeuft && st.stufe == "VIS" && st.phase == "PassagePortalFlow" && st.pacifier >= 0.1;
    }, 300));
    add(tu("Handbuch vis2c: Bauen anhalten", [] { ke::bauAnhalten(); }));
    add(fertig("Handbuch: angehalten", 120));
    add(tu("Handbuch vis2c: Fenster \"Bauen fortsetzen\", Maus beiseite", [] {
        ke::bauFortsetzenOeffnen();
        setzeMaus(ImVec2{-50.0F, -50.0F});
    }));
    add(pause(0.6));
    add(hb::fotoTeil("h_vis2c_fortsetzen", ganz, [] { return std::vector<hb::Marke>{hb::fensterRect("###kartebaufortsetzen")}; }));
    add(klick(ke::trK(Str::KeBpAlt), "kartebaufortsetzen"));
    add(tu("Handbuch vis2c: Maus beiseite", [] { setzeMaus(ImVec2{-50.0F, -50.0F}); }));
    add(warteSek("Handbuch vis2c: das VIS macht mit dem Zwischenstand weiter", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return st.laeuft && st.schritt == 1 && st.visStand.zustand == karte::BauVisStand::Zustand::Fortgesetzt && st.pacifier >= st.visStand.anteil &&
               st.visStandBytes >= 0;
    }, 120));
    add(hb::fotoTeil("h_vis2c_fortschritt", ganz, [] {
        // ein Rahmen um beide Knoepfe: der Pfeil daneben verdeckt sonst [Bauen abbrechen]
        std::vector<hb::Marke> m;
        const Element* a = finde(ke::trK(Str::KeBpAnhalten), "kartebaufortschritt", 0, false);
        const Element* b = finde(ke::trK(Str::KeBmAbbrechen), "kartebaufortschritt", 0, false);
        if (a != nullptr && b != nullptr) { m.emplace_back(hb::vereint(a->rect, b->rect)); }
        return m;
    }));
    add(pruefe("Handbuch vis2c: das Bild zeigt den fortgesetzten VIS-Schritt (er lief beim Foto noch)", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return st.laeuft && st.schritt == 1 && st.visStand.zustand == karte::BauVisStand::Zustand::Fortgesetzt;
    }));
    add(tu("Handbuch vis2c: abbrechen, Zwischenstand verwerfen", [] {
        ke::bauAbbrechen();
        ke::bauPauseVerwerfen();
        karteDialogeZu();
        g_app->leftMode = 0;
    }));
    return s;
}

}  // namespace vis2c
