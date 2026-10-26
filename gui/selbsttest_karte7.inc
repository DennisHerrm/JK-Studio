// selbsttest_karte7.inc - Stufe 7 des Karten-Editors: Tempo und Echtzeit-Licht.
//
// Eingebunden in gui/selbsttest.cpp (namenloser Namensraum, nach Stufe 6).
// Auftrag shank: "dass es schneller ist aber genauso gut? so das wir komplett
// optimiert sind". Jede Beschleunigung wird vorher und nachher gemessen
// (tools/karte/tempo.py, Median aus 3, Prioritaet Idle) und muss dieselbe
// Ausgabe liefern.
//
// BHED_KARTE_NUR=tempo: die Messliste des Editors (tempo::schritte). Je Karte
// aus BHED_TEMPO_KARTEN (Namen, Komma) aus BHED_TEMPO_DEKOMP/<spiel>/q3/:
// Laden, Texturen, Bildzeiten von Kamera und 2D (Mittel, 99. Perzentil),
// Alles auswaehlen, grosse Auswahl verschieben, Rueckgaengig/Wiederholen, CSG,
// Clipper, Speichern, Optimierer-Pruefung, .bsp oeffnen (dekompilieren). Jede
// Zeit als Zeile "Tempo: <was> = <wert> ms"; dazu "Tempo-Summe: <was> = <hash>"
// der gespeicherten .map-Dateien - der Gleichheitsnachweis fuer Punkt 4
// (vorher/nachher dieselben Bytes).

namespace tempo {

namespace ke = karteneditor;

double msSeit(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

void zeit(const std::string& was, double ms) {
    char z[64];
    std::snprintf(z, sizeof(z), " = %.2f ms", ms);
    diag::detail("Tempo: " + was + z);
}

// Mittel und 99. Perzentil einer Messreihe
void reihe(const std::string& was, std::vector<double> v) {
    if (v.empty()) { return; }
    std::sort(v.begin(), v.end());
    double summe = 0.0;
    for (const double x : v) { summe += x; }
    zeit(was + " Mittel", summe / static_cast<double>(v.size()));
    zeit(was + " p99", v[std::min(v.size() - 1, (v.size() * 99) / 100)]);
}

std::uint64_t fnv(const std::string& s) {
    std::uint64_t h = 1469598103934665603ULL;
    for (const unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

void summe(const std::string& was, const std::string& datei) {
    const std::string inhalt = dateiInhalt(datei);
    char z[64];
    std::snprintf(z, sizeof(z), " = %016llx (%zu Byte)", static_cast<unsigned long long>(fnv(inhalt)), inhalt.size());
    diag::detail("Tempo-Summe: " + was + z);
}

struct Karte {
    std::string name;
    std::string spiel;   // Unterordner in karte_dekomp: ja, jk2, md
};

std::vector<Karte> karten() {
    std::vector<Karte> k;
    const char* liste = std::getenv("BHED_TEMPO_KARTEN");
    std::string l = liste != nullptr ? liste : "duel_pit,mp__duel1,t1_inter,kejim_post,duel_mos_espa,md_am_sith";
    std::size_t a = 0;
    while (a <= l.size()) {
        const std::size_t e = std::min(l.find(',', a), l.size());
        const std::string n = l.substr(a, e - a);
        if (!n.empty()) {
            const std::string spiel = (n == "duel_pit" || n == "kejim_post" || n.rfind("duel_jedi", 0) == 0) ? "jk2"
                                      : (n.rfind("md_", 0) == 0 || n.rfind("duel_", 0) == 0)        ? "md"
                                                                                                    : "ja";
            k.push_back({n, spiel});
        }
        a = e + 1;
    }
    return k;
}

// Das laengste der naechsten n Bilder (der Neubau nach einem Befehl faellt in eines davon)
Schritt laengstes(const std::string& was, int n) {
    auto m = std::make_shared<double>(0.0);
    return {"Tempo: laengstes der naechsten Bilder - " + was, [m, was, n](int b) {
                if (b == 0) {
                    *m = 0.0;
                    return false;
                }
                *m = std::max(*m, ke::editor().zeichenMs);
                if (b >= n) {
                    zeit(was + " - Bild danach", *m);
                    return true;
                }
                return false;
            }};
}

// Einen Editor-Befehl messen
Schritt befehl(const std::string& was, const std::string& name) {
    return tu("Tempo: " + was, [was, name] {
        const auto t0 = std::chrono::steady_clock::now();
        (void)ke::ausfuehren(name);
        zeit(was, msSeit(t0));
    });
}

std::shared_ptr<karte::Primitiv> mittlererPinsel() {
    const auto& kinder = ke::editor().karte.welt()->kinder();
    for (std::size_t i = kinder.size() / 2; i < kinder.size(); ++i) {
        if (kinder[i]->art() != karte::Primitiv::Art::Pinsel) { continue; }
        const karte::Kasten b = std::static_pointer_cast<karte::Pinsel>(kinder[i])->grenzen(ke::editor().karte.kontext);
        if (b.extents[0] > 32 && b.extents[1] > 32 && b.extents[2] > 32) { return kinder[i]; }
    }
    return kinder.front();
}

// Spiel des Editors umstellen (ja.game / jk2.game) und die Ladezeit messen
std::vector<Schritt> spielWechsel(const std::string& datei, const std::string& was) {
    std::vector<Schritt> s;
    auto t0 = std::make_shared<std::chrono::steady_clock::time_point>();
    s.push_back(tu("Tempo: Spiel " + datei + " laden", [datei, t0] {
        ke::Editor& e = ke::editor();
        ke::neueKarte();
        // Die Spielordner des Editors kommen aus dem Regelwerk (eingerichteteSpielordner):
        // fuer jk2.game die Ordner von Jedi Outcast, sonst wieder automatisch.
        g_app->settings.regelwerk = datei == "jk2.game" ? kRegelwerkJediOutcast : kRegelwerkAuto;
        e.spielDatei = datei;
        e.spielKennung.clear();
        *t0 = std::chrono::steady_clock::now();
        ke::spielPruefen();
    }));
    s.push_back(warteBis("Tempo: Spiel " + datei + " geladen", [t0, was] {
        ke::spielPruefen();
        if (!ke::spielBereit()) { return false; }
        zeit(was, msSeit(*t0));
        return true;
    }, 3000));
    return s;
}

std::vector<Schritt> karteMessen(const Karte& k) {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const char* dek = std::getenv("BHED_TEMPO_DEKOMP");
    if (dek == nullptr) { return s; }
    const std::string quelle = std::string(dek) + "\\" + k.spiel + "\\q3\\" + k.name + ".map";
    const std::string quelleBsp = std::string(dek) + "\\" + k.spiel + "\\bsp\\" + k.name + ".bsp";
    const std::string ordner = stufe3::laufOrdner("s7tempo");
    const std::string ziel = ordner + "\\" + k.name + ".map";
    const std::string n = k.name;
    auto t0 = std::make_shared<std::chrono::steady_clock::time_point>();
    add(tu("Tempo: " + n + " kopieren und laden", [quelle, ziel, n, ordner] {
        karteDialogeZu();
        ke::optimiererSchliessen();
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(ordner), ec);
        std::filesystem::copy_file(std::filesystem::path(quelle), std::filesystem::path(ziel), std::filesystem::copy_options::overwrite_existing, ec);
        const auto t = std::chrono::steady_clock::now();
        (void)ke::karteOeffnen(ziel);
        zeit(".map laden (" + n + ")", msSeit(t));
        ke::editor().karte.auswahl.leeren();
    }));
    add(pruefe("Tempo: " + n + " offen", [ziel] { return ke::editor().name == ziel && ke::editor().karte.welt() != nullptr; }));
    // erstes Bild und Texturen (qer_editorimage im Hintergrund)
    add({"Tempo: " + n + " erstes Bild, Texturen", [t0, n](int b) {
             if (b == 0) {
                 *t0 = std::chrono::steady_clock::now();
                 return false;
             }
             if (b == 1) { zeit("erstes Bild nach dem Laden (" + n + ")", ke::editor().zeichenMs); }
             if (ke::kameraBilderFertig()) {
                 zeit("Texturen bis fertig (" + n + ")", msSeit(*t0));
                 return true;
             }
             if (b > 3900) {
                 meldeFehler("Tempo: Texturen von " + n + " nicht fertig");
                 return true;
             }
             return false;
         }});
    add(pause(0.3));
    // Kamera: 120 Bilder drehen (2D steht)
    {
        auto ed = std::make_shared<std::vector<double>>();
        auto ka = std::make_shared<std::vector<double>>();
        auto bi = std::make_shared<std::vector<double>>();
        add({"Tempo: " + n + " Kamera 120 Bilder", [ed, ka, bi, n](int b) {
                 ke::Editor& e = ke::editor();
                 if (b > 1) {
                     ed->push_back(e.zeichenMs);
                     ka->push_back(e.kamFenster.zeichenMs);
                     bi->push_back(static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0);
                 }
                 e.kamera.freiBewegen(6, (b / 60) % 2 == 0 ? 1 : -1, e.kameraE);
                 if (b >= 121) {
                     reihe("Kamera dreht: Editor je Bild (" + n + ")", *ed);
                     reihe("Kamera dreht: Kamera je Bild (" + n + ")", *ka);
                     reihe("Kamera dreht: Bildabstand (" + n + ")", *bi);
                     return true;
                 }
                 return false;
             }});
    }
    // 2D: 120 Bilder rollen (Kamera steht)
    {
        auto ed = std::make_shared<std::vector<double>>();
        auto bi = std::make_shared<std::vector<double>>();
        add({"Tempo: " + n + " 2D 120 Bilder", [ed, bi, n](int b) {
                 ke::Editor& e = ke::editor();
                 if (b > 1) {
                     ed->push_back(e.zeichenMs - e.kamFenster.zeichenMs);
                     bi->push_back(static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0);
                 }
                 e.xy[0].ansicht.rollen((b / 60) % 2 == 0 ? 8 : -8, 0);
                 if (b >= 121) {
                     reihe("2D rollt: Editor ohne Kamera je Bild (" + n + ")", *ed);
                     reihe("2D rollt: Bildabstand (" + n + ")", *bi);
                     return true;
                 }
                 return false;
             }});
    }
    // Alles auswaehlen (Invert Selection bei leerer Auswahl), verschieben, Rueckgaengig/Wiederholen
    add(tu("Tempo: " + n + " Auswahl leeren", [] { ke::editor().karte.auswahl.leeren(); }));
    add(befehl("Alles auswaehlen (" + n + ")", "InvertSelection"));
    add(laengstes("Alles auswaehlen (" + n + ")", 6));
    add(befehl("Auswahl verschieben, alles (" + n + ")", "MoveSelectionUP"));
    add(laengstes("Auswahl verschieben, alles (" + n + ")", 6));
    add(befehl("Rueckgaengig Verschieben, alles (" + n + ")", "Undo"));
    add(laengstes("Rueckgaengig Verschieben, alles (" + n + ")", 6));
    add(befehl("Wiederholen Verschieben, alles (" + n + ")", "Redo"));
    add(befehl("Rueckgaengig Verschieben 2, alles (" + n + ")", "Undo"));
    add(tu("Tempo: " + n + " Auswahl leeren", [] { ke::editor().karte.auswahl.leeren(); }));
    add(pause(0.1));
    // CSG Subtract und Clipper mit einem Pinsel aus der Mitte
    add(tu("Tempo: " + n + " CSG Subtract", [n] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), mittlererPinsel());
        const auto t = std::chrono::steady_clock::now();
        (void)ke::ausfuehren("CSGSubtract");
        zeit("CSG Subtract (" + n + ")", msSeit(t));
    }));
    add(laengstes("CSG Subtract (" + n + ")", 6));
    add(befehl("Rueckgaengig CSG Subtract (" + n + ")", "Undo"));
    add(pause(0.1));
    add(tu("Tempo: " + n + " Clipper", [n] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        const auto p = mittlererPinsel();
        e.karte.auswahl.waehlen(e.karte.welt(), p);
        (void)ke::ausfuehren("ToggleClipper");
        const karte::Kasten b = std::static_pointer_cast<karte::Pinsel>(p)->grenzen(e.karte.kontext);
        const karte::Vektor3d c(b.origin[0], b.origin[1], b.origin[2]);
        e.bearbeitung.clipperPunkte.punkte[0] = c + karte::Vektor3d(0, -64, 0);
        e.bearbeitung.clipperPunkte.punkte[1] = c + karte::Vektor3d(0, 64, 0);
        e.bearbeitung.clipperPunkte.punkte[2] = c + karte::Vektor3d(0, 0, 64);
        e.bearbeitung.clipperPunkte.anzahl = 3;
        const auto t = std::chrono::steady_clock::now();
        (void)ke::ausfuehren("ClipperClip");
        zeit("Clipper Clip (" + n + ")", msSeit(t));
    }));
    add(laengstes("Clipper Clip (" + n + ")", 6));
    add(tu("Tempo: " + n + " Rueckgaengig, Clipper aus", [] {
        (void)ke::ausfuehren("Undo");
        (void)ke::ausfuehren("ToggleClipper");
        ke::editor().karte.auswahl.leeren();
    }));
    add(pause(0.1));
    // Speichern (die Bytes sind der Gleichheitsnachweis)
    add(tu("Tempo: " + n + " speichern", [ordner, n] {
        const std::string z = ordner + "\\" + n + "_gespeichert.map";
        const auto t = std::chrono::steady_clock::now();
        (void)ke::karteSpeichernUnter(z);
        zeit(".map speichern (" + n + ")", msSeit(t));
        summe(".map gespeichert (" + n + ")", z);
    }));
    // Optimierer: Start (Momentaufnahme, Shader, beim ersten Mal die NPC-Dateien) und Pruefung
    {
        auto laeufe = std::make_shared<int>(0);
        auto m = std::make_shared<stufe5::OptLauf>();
        add(stufe5::optimiererGemessen("Tempo: " + n + " Optimierer", laeufe, m, 3900));
        add(tu("Tempo: " + n + " Optimierer-Zeiten", [m, n] {
            const ke::OptimiererBlick b = ke::optimiererBlick();
            zeit("Optimierer Start (" + n + ")", m->startMs);
            zeit("Optimierer Pruefung (" + n + ")", b.ms);
            zeit("Optimierer groesstes Bild waehrend der Pruefung (" + n + ")", m->groesstes());
            ke::optimiererSchliessen();
        }));
    }
    // .bsp oeffnen (dekompilieren, wie File > Open .bsp...)
    add(tu("Tempo: " + n + " .bsp oeffnen", [quelleBsp, ordner, n] {
        std::error_code ec;
        if (!std::filesystem::exists(std::filesystem::path(quelleBsp), ec)) { return; }
        const auto t = std::chrono::steady_clock::now();
        const bool ok = ke::bspOeffnenDatei(quelleBsp);
        zeit(".bsp oeffnen, dekompilieren (" + n + ")", msSeit(t));
        if (!ok) { meldeFehler("Tempo: .bsp von " + n + " nicht geoeffnet"); }
    }));
    add(pause(0.3));
    add(tu("Tempo: " + n + " dekompilierte Karte speichern", [ordner, n] {
        const std::string z = ordner + "\\" + n + "_ausbsp.map";
        (void)ke::karteSpeichernUnter(z);
        summe(".bsp dekompiliert, gespeichert (" + n + ")", z);
    }));
    return s;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto t0 = std::make_shared<std::chrono::steady_clock::time_point>();
    add(tu("Tempo: Spielordner, Modus Events", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
    }));
    add(pause(0.5));
    // Wechsel in den Karten-Editor: bis die Spieldaten (Klassen, Shader, Texturliste) bereit sind
    add({"Tempo: Wechsel in den Karten-Editor", [t0](int b) {
             if (b == 0) {
                 *t0 = std::chrono::steady_clock::now();
                 g_app->leftMode = 3;
                 return false;
             }
             if (b == 1) { zeit("Moduswechsel: erstes Bild", msSeit(*t0)); }
             ke::spielPruefen();
             if (ke::spielBereit()) {
                 zeit("Moduswechsel bis Spieldaten bereit", msSeit(*t0));
                 return true;
             }
             return b > 3900;
         }});
    add(pruefe("Tempo: Karten-Editor aktiv, Spiel bereit", [] { return g_app->leftMode == 3 && ke::aktiv() && ke::spielBereit(); }));
    // Stufe 7 Punkt 4: die NPC-Dateien liest jetzt ein Faden (npcLeseAuftrag + npcAuftragLesen).
    // Gleichheit mit dem alten Weg (readFromArchives je .npc-Datei auf dem Hauptfaden).
    add(pruefe("Tempo: NPC-Dateien im Faden = alter Weg (readFromArchives, gleiche Typen und Modelle)", [] {
        NpcMap alt;
        int altDateien = 0;
        for (const GamePath& gp : g_app->gamePaths) {
            for (const FoundFile& f : findByExtension(gp, {".npc"})) {
                std::string text;
                if (readFromArchives(f.name, text)) {
                    parseNpcFile(text, alt);
                    ++altDateien;
                }
            }
        }
        NpcMap neu;
        std::vector<std::string> meldungen;
        const int neuDateien = npcAuftragLesen(npcLeseAuftrag(), neu, meldungen);
        bool gleich = alt.size() == neu.size() && altDateien == neuDateien;
        for (auto a = alt.begin(), b = neu.begin(); gleich && a != alt.end(); ++a, ++b) {
            gleich = a->first == b->first && a->second.playerModel == b->second.playerModel && a->second.legsModel == b->second.legsModel;
        }
        diag::detail("Knopftest: Tempo - NPC " + std::to_string(altDateien) + " Dateien, " + std::to_string(alt.size()) + " Typen");
        return gleich && altDateien > 0;
    }));
    add(tu("Tempo: Axial Projection (wie der Selbsttest)", [] { ke::editor().pinselTyp = 0; }));
    std::string spiel = "ja";
    for (const Karte& k : karten()) {
        const std::string soll = k.spiel == "jk2" ? "jk2" : "ja";
        if (soll != spiel) {
            for (Schritt& x : spielWechsel(soll + ".game", "Spiel laden " + soll + ".game (Klassen, Shader, Texturliste)")) { add(std::move(x)); }
            spiel = soll;
        }
        for (Schritt& x : karteMessen(k)) { add(std::move(x)); }
    }
    if (spiel != "ja") {
        for (Schritt& x : spielWechsel("ja.game", "Spiel laden ja.game zurueck")) { add(std::move(x)); }
    }
    add(tu("Tempo: neue Karte", [] { ke::neueKarte(); }));
    return s;
}

}  // namespace tempo
