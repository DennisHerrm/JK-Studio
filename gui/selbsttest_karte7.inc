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

// --- Stufe 7 Punkt 5: Echtzeit-Licht und Schnell-Backen (BHED_KARTE_NUR=stufe7) -----------
//
// Wirkung: ein Licht naeher an den Boden -> der Boden in der Bildmitte wird
// heller, weiter weg -> dunkler; eine Wand zwischen Licht und Boden wirft
// Schatten. Schnell-Backen: nach der Pause baut bhed_q3map2 im Hintergrund,
// die Kamera zeigt die echten Lightmaps; dieselben Lightmaps wie ein Lauf auf
// der Kommandozeile mit denselben Schaltern; eine Aenderung waehrend des Baus
// bricht ihn ab. Bildzeiten mit Licht auf der grossen Karte (BHED_KARTE_GROSS).
namespace stufe7 {

namespace ke = karteneditor;

std::string lichtKarte() { return karteTestPfad("s7_licht.map"); }

// Ein Raum 1024 x 1024 x 512 (Boden bei z 0), ein Licht bei (0 0 96), wahlweise eine Wand bei x 32..48
void lichtKarteSchreiben(bool wand) {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    karte::TexturProjektion proj;
    karte::texdefStandard(karte::TexdefArt::Quake, proj);
    const auto quader = [&](const karte::Vektor3f& mitte, const karte::Vektor3f& halb) {
        auto p = std::make_shared<karte::Pinsel>();
        p->quaderBauen(karte::Kasten(mitte, halb), "textures/yavin/temple_stone2", proj, k.kontext);
        welt->kindEinfuegen(p);
    };
    // ein geschlossener Raum (sonst leckt die Karte, und -vis hat keine .prt)
    quader(karte::Vektor3f(0, 0, -8), karte::Vektor3f(512, 512, 8));      // Boden
    quader(karte::Vektor3f(0, 0, 520), karte::Vektor3f(512, 512, 8));     // Decke
    quader(karte::Vektor3f(-520, 0, 256), karte::Vektor3f(8, 528, 272));  // Waende
    quader(karte::Vektor3f(520, 0, 256), karte::Vektor3f(8, 528, 272));
    quader(karte::Vektor3f(0, -520, 256), karte::Vektor3f(512, 8, 272));
    quader(karte::Vektor3f(0, 520, 256), karte::Vektor3f(512, 8, 272));
    if (wand) { quader(karte::Vektor3f(40, 0, 128), karte::Vektor3f(8, 504, 128)); }
    auto licht = k.neueEntitaet("light", false);
    licht->setzen("origin", "0 0 96");
    licht->setzen("light", "300");
    k.entitaetEinfuegen(licht);
    (void)karte::karteSpeichern(lichtKarte(), k, false);
}

std::shared_ptr<karte::Entitaet> erstesLicht() {
    for (const auto& e : ke::editor().karte.entitaeten()) {
        if (e->klassenname() == "light") { return e; }
    }
    return nullptr;
}

// Kamera ueber (x 0 z), Blick fast senkrecht nach unten
void kameraUeber(float x, float z) {
    ke::Editor& e = ke::editor();
    e.kamera.ursprung = karte::Vektor3f(x, 0, z);
    e.kamera.winkel = karte::Vektor3f(-89.0F, 0.0F, 0.0F);
}

int helligkeit() {
    unsigned char c[4] = {0, 0, 0, 0};
    const ke::KameraFenster& f = ke::editor().kamFenster;
    if (!ke::kameraPixel(f.breite / 2, f.hoehe / 2, c)) { return -1; }
    return c[0] + c[1] + c[2];
}

std::vector<Schritt> licht() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto werte = std::make_shared<std::array<int, 6>>();
    add(tu("Stufe 7: Testkarte mit Licht, Kamera textured, Echtzeit-Licht an", [] {
        karteDialogeZu();
        lichtKarteSchreiben(false);
        (void)ke::karteOeffnen(lichtKarte());
        ke::editor().kameraE.modus = karte::KameraModus::Textur;
        ke::editor().echtLicht.an = true;
        ke::editor().echtLicht.backen = false;
        kameraUeber(128, 400);
    }));
    add(warteBis("Stufe 7: Texturen der Testkarte", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(0.5));
    add(pruefe("Stufe 7: Echtzeit-Licht aktiv, ein Licht gezeichnet, Schattenwuerfel gerendert", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Stufe 7 - Lichter " + std::to_string(e.echtLicht.lichter) + ", gezeichnet " + std::to_string(e.echtLicht.gezeichnet) +
                     ", Schattenwuerfel neu " + std::to_string(e.echtLicht.schattenNeu));
        return ke::echtLichtAktiv() && e.echtLicht.lichter == 1 && e.echtLicht.gezeichnet == 1 && e.echtLicht.schattenNeu >= 1;
    }));
    add(tu("Stufe 7: Helligkeit des Bodens bei (128 0 0), Licht bei (0 0 96)", [werte] { (*werte)[0] = helligkeit(); }));
    add(foto("karteneditor_s7_echtlicht"));
    add(tu("Stufe 7: Licht naeher (128 0 48)", [] { erstesLicht()->setzen("origin", "128 0 48"); }));
    add(pause(0.3));
    add(tu("Stufe 7: Helligkeit naeher", [werte] { (*werte)[1] = helligkeit(); }));
    add(tu("Stufe 7: Licht weiter weg (-256 0 200)", [] { erstesLicht()->setzen("origin", "-256 0 200"); }));
    add(pause(0.3));
    add(tu("Stufe 7: Helligkeit weiter weg", [werte] { (*werte)[2] = helligkeit(); }));
    add(pruefe("Stufe 7: Licht verschieben aendert die Helligkeit in die richtige Richtung (naeher heller, weiter dunkler)", [werte] {
        const auto& w = *werte;
        diag::detail("Knopftest: Stufe 7 - Boden " + std::to_string(w[0]) + ", Licht naeher " + std::to_string(w[1]) + ", weiter " + std::to_string(w[2]));
        return w[0] > 0 && w[1] > w[0] && w[2] < w[0];
    }));
    // Schatten: dieselbe Lage mit einer Wand zwischen Licht und Messstelle
    add(tu("Stufe 7: Licht zurueck (0 0 96), ohne Wand", [] { erstesLicht()->setzen("origin", "0 0 96"); }));
    add(pause(0.3));
    add(tu("Stufe 7: Helligkeit ohne Wand", [werte] { (*werte)[3] = helligkeit(); }));
    add(tu("Stufe 7: Karte mit Wand bei x 32..48 oeffnen", [] {
        lichtKarteSchreiben(true);
        (void)ke::karteOeffnen(lichtKarte());
        kameraUeber(128, 400);
    }));
    add(warteBis("Stufe 7: Texturen", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(0.5));
    add(tu("Stufe 7: Helligkeit mit Wand", [werte] { (*werte)[4] = helligkeit(); }));
    add(foto("karteneditor_s7_schatten"));
    add(pruefe("Stufe 7: die Wand wirft Schatten (die Messstelle wird dunkler)", [werte] {
        const auto& w = *werte;
        diag::detail("Knopftest: Stufe 7 - ohne Wand " + std::to_string(w[3]) + ", mit Wand " + std::to_string(w[4]));
        return w[3] > 0 && w[4] >= 0 && w[4] < w[3] * 3 / 4;
    }));
    add(tu("Stufe 7: Echtzeit-Licht aus", [werte] {
        ke::editor().echtLicht.an = false;
    }));
    add(pause(0.3));
    add(pruefe("Stufe 7: ohne Echtzeit-Licht wieder das Licht aus Blickrichtung (anderes Bild)", [werte] {
        const int h = helligkeit();
        (*werte)[5] = h;
        diag::detail("Knopftest: Stufe 7 - ohne Echtzeit-Licht " + std::to_string(h));
        return h > 0 && h != (*werte)[4] && !ke::echtLichtAktiv();
    }));
    return s;
}

// Die Lightmap-Lumps zweier .bsp vergleichen (RBSP Lump 14 Lightmaps, 15 Lichtgitter)
bool lightmapsGleich(const std::string& a, const std::string& b, std::string& was) {
    const std::string x = dateiInhalt(a);
    const std::string y = dateiInhalt(b);
    const auto lump = [](const std::string& d, int i) -> std::string {
        if (d.size() < 8 + 8 * (i + 1)) { return {}; }
        std::int32_t o = 0;
        std::int32_t l = 0;
        std::memcpy(&o, d.data() + 8 + 8 * i, 4);
        std::memcpy(&l, d.data() + 12 + 8 * i, 4);
        if (o < 0 || l < 0 || static_cast<std::size_t>(o) + static_cast<std::size_t>(l) > d.size()) { return {}; }
        return d.substr(static_cast<std::size_t>(o), static_cast<std::size_t>(l));
    };
    bool gleich = !x.empty() && !y.empty();
    for (const int i : {14, 15, 16}) {   // lightmaps, lightgrid, visibility
        const bool g = lump(x, i) == lump(y, i);
        was += " Lump " + std::to_string(i) + (g ? " gleich" : " ANDERS") + " (" + std::to_string(lump(x, i).size()) + " Byte)";
        gleich = gleich && g;
    }
    return gleich;
}

std::vector<Schritt> backen() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Stufe 7: Testkarte mit Wand, Schnell-Backen an (Pause 1 s)", [] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();   // bhed_q3map2.exe neben behaved.exe
        lichtKarteSchreiben(true);
        (void)ke::karteOeffnen(lichtKarte());
        ke::Editor& e = ke::editor();
        e.kameraE.modus = karte::KameraModus::Textur;
        e.echtLicht.an = true;
        e.echtLicht.backen = true;
        e.echtLicht.pauseSekunden = 1.0;
        kameraUeber(128, 400);
    }));
    add({"Stufe 7: Schnell-Backen fertig (hoechstens 120 s)", [](int b) {
             const ke::Editor& e = ke::editor();
             if (e.echtLicht.gebackenZeigen) { return true; }
             if (e.echtLicht.bauStand == 5 || b > 7000) {
                 meldeFehler("Stufe 7: Schnell-Backen nicht fertig (Stand " + std::to_string(e.echtLicht.bauStand) + ")");
                 return true;
             }
             return false;
         }, true});
    add(tu("Stufe 7: Kamera ueber die helle Seite (x -128)", [] { kameraUeber(-128, 400); }));
    add(pause(0.5));
    add(foto("karteneditor_s7_gebacken"));
    add(pruefe("Stufe 7: gebacken - der Boden neben dem Licht ist hell (echte Lightmap)", [] {
        const int h = helligkeit();
        diag::detail("Knopftest: Stufe 7 - gebacken, Boden bei x -128: " + std::to_string(h));
        return h > 30;
    }));
    add(pruefe("Stufe 7: gebacken - die Kamera zeigt die echten Lightmaps, die Karte des Anwenders und eine .bsp neben ihr bleiben unberuehrt", [] {
        const ke::Editor& e = ke::editor();
        std::error_code ec;
        const bool keineBsp = !std::filesystem::exists(std::filesystem::path(karteTestPfad("s7_licht.bsp")), ec);
        char z[200];
        std::snprintf(z, sizeof(z), "Knopftest: Stufe 7 - Schnell-Backen %.2f s, Ordner %s", e.echtLicht.bauSekunden, e.echtLicht.bauOrdner.c_str());
        diag::detail(z);
        char t[64];
        std::snprintf(t, sizeof(t), " = %.2f ms", e.echtLicht.bauSekunden * 1000.0);
        diag::detail(std::string("Tempo: Schnell-Backen (Testkarte mit Licht)") + t);
        return e.echtLicht.gebackenZeigen && keineBsp && !ke::geaendert() && e.echtLicht.bauSekunden > 0;
    }));
    // dieselben Lightmaps wie ein Lauf auf der Kommandozeile mit denselben Schaltern
    add(tu("Stufe 7: dieselbe Karte mit bhed_q3map2 auf der Kommandozeile (-meta, -vis -fast, -light -fast)", [] {
        const ke::Editor& e = ke::editor();
        const std::string ordner = karteTestPfad("s7_kommandozeile");
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(ordner) / "maps", ec);
        std::filesystem::copy_file(std::filesystem::path(e.echtLicht.bauOrdner) / "maps" / "schnell.map",
                                   std::filesystem::path(ordner) / "maps" / "schnell.map", std::filesystem::copy_options::overwrite_existing, ec);
        const std::string karte = ordner + "\\maps\\schnell.map";
        const std::string bsp = ordner + "\\maps\\schnell.bsp";
        std::string exe = platform::executableDirectory();
        if (!exe.empty() && exe.back() != '/' && exe.back() != '\\') { exe += '\\'; }
        exe += "bhed_q3map2.exe";
        const std::string grund = "\"" + exe + "\" -game " + (ke::kartenSpielNummer() == kRegelwerkJediOutcast ? "jk2" : "ja") + " -fs_basepath \"" +
                                  ke::enginePfadAnzeige() + "\" -fs_homepath \"\" -fs_game " + ke::bauSpielName() + " ";
        for (const std::string& b : {grund + "-meta \"" + karte + "\"", grund + "-vis -fast \"" + bsp + "\"", grund + "-light -fast \"" + bsp + "\""}) {
            if (b.find("-vis") != std::string::npos && !std::filesystem::exists(std::filesystem::path(ordner) / "maps" / "schnell.prt", ec)) { continue; }
            bauplattform::Prozess p;
            std::string fehler;
            if (!p.starten(b, std::string(), false, fehler)) { break; }
            while (!p.warten(50)) {}
        }
    }));
    add(pruefe("Stufe 7: Schnell-Backen = Kommandozeile mit denselben Schaltern (Lightmaps, Lichtgitter, Sichtbarkeit)", [] {
        std::string was;
        const bool gleich = lightmapsGleich(ke::editor().echtLicht.bauOrdner + "\\maps\\schnell.bsp", karteTestPfad("s7_kommandozeile") + "\\maps\\schnell.bsp", was);
        diag::detail("Knopftest: Stufe 7 -" + was);
        return gleich;
    }));
    // Abbruch: Aenderung waehrend des Baus
    add(tu("Stufe 7: Licht verschieben (neuer Bau nach der Pause)", [] {
        ke::Editor& e = ke::editor();
        karte::Befehl b(e.karte.undo, "entitySetKeyValue -key \"origin\" -value \"64 0 96\"");
        erstesLicht()->setzen("origin", "64 0 96");
    }));
    add(warteBis("Stufe 7: der neue Bau laeuft", [] { return ke::editor().echtLicht.bauStand == 2; }, 3000));
    auto abgebrochen = std::make_shared<std::size_t>(0);
    add(tu("Stufe 7: waehrend des Baus noch eine Aenderung", [abgebrochen] {
        ke::Editor& e = ke::editor();
        *abgebrochen = e.echtLicht.baueAbgebrochen;
        karte::Befehl b(e.karte.undo, "entitySetKeyValue -key \"origin\" -value \"96 0 96\"");
        erstesLicht()->setzen("origin", "96 0 96");
    }));
    add(warteBis("Stufe 7: abgebrochen, danach wieder gebacken", [] { return ke::editor().echtLicht.gebackenZeigen; }, 3500));
    add(pruefe("Stufe 7: eine Aenderung bricht den laufenden Bau ab, danach wird der neue Stand gebacken", [abgebrochen] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Stufe 7 - abgebrochene Baue " + std::to_string(e.echtLicht.baueAbgebrochen - *abgebrochen));
        return e.echtLicht.baueAbgebrochen > *abgebrochen && e.echtLicht.gebackenZeigen;
    }));
    add(tu("Stufe 7: Schnell-Backen und Echtzeit-Licht aus", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.an = false;
        e.echtLicht.pauseSekunden = 2.0;
    }));
    add(pause(0.3));
    add(pruefe("Stufe 7: aus - keine gebackene Ansicht mehr", [] { return !ke::editor().echtLicht.gebackenZeigen; }));
    return s;
}

// Bildzeiten mit Echtzeit-Licht auf der grossen Karte (BHED_KARTE_GROSS)
std::vector<Schritt> gross() {
    std::vector<Schritt> s;
    const char* pfad = std::getenv("BHED_KARTE_GROSS");
    if (pfad == nullptr || pfad[0] == '\0') { return s; }
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const std::string karte = pfad;
    add(tu("Stufe 7: grosse Karte, Echtzeit-Licht an: " + karte, [karte] {
        karteDialogeZu();
        (void)ke::karteOeffnen(karte);
        ke::editor().kameraE.modus = karte::KameraModus::Textur;
        ke::editor().echtLicht.an = true;
    }));
    add(warteBis("Stufe 7: Texturen der grossen Karte", [] { return ke::kameraBilderFertig(); }, 6000));
    add(pause(1.0));
    auto bilder = std::make_shared<std::vector<double>>();
    auto licht = std::make_shared<std::vector<double>>();
    add({"Stufe 7: grosse Karte mit Echtzeit-Licht, 240 Bilder Kamera dreht", [bilder, licht](int b) {
             ke::Editor& e = ke::editor();
             if (b > 2) {
                 bilder->push_back(static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0);
                 licht->push_back(e.echtLicht.lichtMs);
             }
             e.kamera.freiBewegen(6, (b / 60) % 2 == 0 ? 1 : -1, e.kameraE);
             return b >= 242;
         }});
    add(pruefe("Stufe 7: grosse Karte - keine Ruckler: 99. Perzentil des Bildabstands unter 50 ms", [bilder, licht] {
        std::vector<double> v = *bilder;
        std::sort(v.begin(), v.end());
        double summe = 0;
        for (const double x : v) { summe += x; }
        const double mittel = v.empty() ? 0 : summe / static_cast<double>(v.size());
        const double p99 = v.empty() ? 0 : v[std::min(v.size() - 1, v.size() * 99 / 100)];
        std::vector<double> l = *licht;
        std::sort(l.begin(), l.end());
        char z[300];
        std::snprintf(z, sizeof(z), "Knopftest: Stufe 7 - grosse Karte mit Echtzeit-Licht: %zu Lichter, je Bild %zu, Bild Mittel %.2f ms, p99 %.2f ms, max %.2f ms; Lichtliste+Schatten Median %.2f ms",
                      ke::editor().echtLicht.lichter, ke::editor().echtLicht.gezeichnet, mittel, p99, v.empty() ? 0 : v.back(), l.empty() ? 0 : l[l.size() / 2]);
        diag::detail(z);
        tempo::zeit("Echtzeit-Licht: Bildabstand grosse Karte Mittel", mittel);
        tempo::zeit("Echtzeit-Licht: Bildabstand grosse Karte p99", p99);
        return !v.empty() && p99 < 50.0;
    }));
    add(tu("Stufe 7: Echtzeit-Licht aus, Testkarte", [] {
        ke::editor().echtLicht.an = false;
        (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map"));
    }));
    add(pause(0.3));
    return s;
}

// --- Punkt 6b: Menues im Stil des Programms, Programm-Einstellungen im
// View-Menue, Knopfgroesse (shank: "settings in den Map editor auch einbauen
// das man theme und language aendern kann ... auch sollte man die buttons
// kleiner machen koennen") ---------------------------------------------------
struct MenueZustand {
    bool nrcStil = true;
    int knopf = 1;
    const theme::Theme* farben = nullptr;
    i18n::Language sprache = i18n::Language::English;
    float skala = 1.0F;
    bool mp = false;
    bool nrcBegriffe = false;
    float zoom[3] = {0, 0, 0};
};

const ke::Editor::RahmenImBild* rahmen7(const std::string& feld) {
    for (const auto& r : ke::editor().rahmenImBild) {
        if (r.feld == feld) { return &r; }
    }
    return nullptr;
}

// Ein Feld dieses Bildes in logischen Punkten (merkeFeld); nullptr ohne
const ke::Editor::Rechteck* feld7(const std::string& name) {
    for (const auto& r : ke::editor().felderImBild) {
        if (r.name == name) { return &r; }
    }
    return nullptr;
}

std::string einstellung7(const std::string& k) {
    const auto it = g_app->settings.karte.find(k);
    return it != g_app->settings.karte.end() ? it->second : std::string("(fehlt)");
}

std::vector<Schritt> menue() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<MenueZustand>();
    add(tu("Stufe 7/6b: Zustand merken, Begriffe des Programms (Englisch)", [z] {
        z->nrcStil = ke::editor().menueNrcStil;
        z->knopf = ke::mass::knopfStufe;
        z->farben = g_app->activeTheme;
        z->sprache = i18n::currentLanguage();
        z->skala = g_app->uiScale;
        z->mp = g_app->settings.mpZiel;
        z->nrcBegriffe = ke::nrcBegriffe();
        ke::setzeNrcBegriffe(false);
        i18n::setLanguage(i18n::Language::English);
        karteDialogeZu();
    }));
    // Erster Start (keine Einstellung gespeichert): Menues im Stil des Programms, Knoepfe wie NRC
    add(tu("Stufe 7/6b: erster Start - eigene Einstellungen entfernen, Einstellungen neu lesen", [] {
        g_app->settings.karte.erase("BhedMenuStyle");
        g_app->settings.karte.erase("BhedToolbarButtons");
        ke::einstellungenLaden();
    }));
    add(pause(0.3));
    add(pruefe("Stufe 7/6b: Vorgabe - Menues im Stil des Programms, Knopfgroesse Normal (Knopf 31 wie NRC)", [] {
        const ke::Editor::Rechteck* k = feld7("Knopf:MirrorSelectionHorizontally");
        diag::detail("Knopftest: Stufe 7/6b - Vorgabe Menuestil " + std::string(ke::editor().menueNrcStil ? "NRC" : "Programm") + ", Knopf " +
                     std::to_string(k != nullptr ? k->b : -1) + " (soll 31)");
        return !ke::editor().menueNrcStil && ke::mass::knopfStufe == 1 && k != nullptr && k->b == 31 && k->h == 31;
    }));
    add(pruefe("Stufe 7/6b: Menueleiste im Stil des Programms - Schrift und Hoehe des Programms, Editor direkt darunter", [] {
        const ImGuiWindow* w = ImGui::FindWindowByName("###main");
        const ke::Editor& e = ke::editor();
        const float leisteH = w != nullptr ? w->MenuBarHeight : -1.0F;
        const float soll = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;   // die Schrift des Programms (nicht Segoe UI 21)
        diag::detail("Knopftest: Stufe 7/6b - Menueleiste " + std::to_string(leisteH) + " hoch (Programm " + std::to_string(soll) + ")");
        return w != nullptr && std::fabs(leisteH - soll) < 0.5F && std::fabs(e.editorOben - e.leisteUnten) < 0.5F;
    }));
    // View: am Ende Zielspiel, Farbgebung, Sprache, Skalierung, Knopfgroesse
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(pruefe("Stufe 7/6b: View endet mit Target game, Theme, Language, Interface Scale, Toolbar Button Size", [] {
        return finde(ke::trK(Str::ViewTarget), "##Menu_", 0, false) != nullptr && finde(ke::trK(Str::ViewTheme), "##Menu_", 0, false) != nullptr &&
               finde(ke::trK(Str::ViewLanguage), "##Menu_", 0, false) != nullptr && finde(ke::trK(Str::ViewDpiScale), "##Menu_", 0, false) != nullptr &&
               finde(ke::trK(Str::KeKnopfGroesse), "##Menu_", 0, false) != nullptr &&
               finde(ke::trK(Str::KeLCameraView), "##Menu_", 0, false) == nullptr;   // Regular: Camera View nur bei Floating (wie NRC)
    }));
    add(pruefe("Stufe 7/6b: kein Hinweis auf Strg + Mausrad im Karten-Editor", [] { return finde(tr(Str::ViewScaleHint), "", 0, false) == nullptr; }));
    add(foto("karte7_menue_view_programm"));
    // Farbgebung
    add(klick(ke::trK(Str::ViewTheme), "##Menu_", 0, false));
    add(pause(0.3));
    add({"Stufe 7/6b: View > Theme > Light", [](int b) {
             static Schritt k;
             if (b == 0) { k = klick(theme::findTheme("light")->name(), "##Menu_", 0, false); }
             return k.tun(b);
         }});
    add(pause(0.4));
    add(pruefe("Stufe 7/6b: Farbgebung Light gilt (activeTheme, angewendet)", [] {
        return g_app->activeTheme != nullptr && g_app->activeTheme->id == "light" && !g_app->themePending;
    }));
    add(tu("Menues zu", [] { karteDialogeZu(); }));
    // Sprache
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(ke::trK(Str::ViewLanguage), "##Menu_", 0, false));
    add(pause(0.3));
    add(klick("Deutsch", "##Menu_", 0, false));
    add(pause(0.4));
    add(pruefe("Stufe 7/6b: Sprache Deutsch gilt (die Menues des Editors deutsch)", [] {
        return i18n::currentLanguage() == i18n::Language::German && std::string(ke::trK(Str::KeLView)) != "View";
    }));
    add(tu("Menues zu, Englisch", [] {
        karteDialogeZu();
        i18n::setLanguage(i18n::Language::English);
    }));
    add(pause(0.3));
    // Skalierung
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(ke::trK(Str::ViewDpiScale), "##Menu_", 0, false));
    add(pause(0.3));
    add(klick("125 %", "##Menu_", 0, false));
    add(pause(0.4));
    add(pruefe("Stufe 7/6b: View > Interface Scale > 125 %: uiScale 1,25", [] { return std::fabs(g_app->uiScale - 1.25F) < 0.001F; }));
    add(tu("Menues zu, Skalierung zurueck", [z] {
        karteDialogeZu();
        g_app->uiScale = z->skala;
    }));
    add(pause(0.4));
    // Zielspiel
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(ke::trK(Str::ViewTarget), "##Menu_", 0, false));
    add(pause(0.3));
    add(klick(ke::trK(Str::TargetMp), "##Menu_", 0, false));
    add(pause(0.3));
    add(pruefe("Stufe 7/6b: View > Target game > Multiplayer: settings.mpZiel", [] { return g_app->settings.mpZiel; }));
    add(tu("Menues zu, Zielspiel zurueck", [z] {
        karteDialogeZu();
        g_app->settings.mpZiel = z->mp;
        rebuildTree();
    }));
    add(pause(0.3));
    // Knopfgroesse: Klein, Gross, Normal - wirkt sofort, die Teiler passen sich an, gespeichert
    struct Stufe {
        int stufe;
        Str name;
        int knopf;
    };
    for (const Stufe st : {Stufe{0, Str::KeKnopfKlein, 23}, Stufe{2, Str::KeKnopfGross, 39}, Stufe{1, Str::KeKnopfNormal, 31}}) {
        add(klick(ke::trK(Str::KeLView), "###main"));
        add(pause(0.3));
        add(klick(ke::trK(Str::KeKnopfGroesse), "##Menu_", 0, false));
        add(pause(0.3));
        add(klick(ke::trK(st.name), "##Menu_", 0, false));
        add(pause(0.4));
        add(pruefe("Stufe 7/6b: Knopfgroesse " + std::string(i18n::trIn(i18n::Language::English, st.name)) + " - Knopf " + std::to_string(st.knopf) +
                       ", Leiste " + std::to_string(st.knopf + 8) + ", Ansichten darunter, gespeichert",
                   [st] {
                       // logische Punkte (merkeFeld): Knopf, Leiste oben, Filterleiste rechts
                       // (so breit wie die Leiste hoch, beginnt unter ihr), die Ansichten darunter
                       const ke::Editor::Rechteck* knopf = feld7("Knopf:MirrorSelectionHorizontally");
                       const ke::Editor::Rechteck* leiste = feld7("Werkzeugleiste");
                       const ke::Editor::Rechteck* filter = feld7("Leiste:filter");
                       const int lh = st.knopf + 8;
                       const bool knopfOk = knopf != nullptr && knopf->b == st.knopf && knopf->h == st.knopf;
                       const bool leisteOk = leiste != nullptr && leiste->h == lh;
                       const bool filterOk = filter != nullptr && filter->b == lh && leiste != nullptr && filter->y == leiste->y + lh;
                       bool mitte = leiste != nullptr;
                       const float sk = ke::massstab();
                       for (const auto& r : ke::editor().rahmenImBild) {
                           if (leiste != nullptr && (r.feld == "XY" || r.feld == "Kamera")) {
                               mitte = mitte && r.y0 >= ke::bildpunkt(0, leiste->y + lh).y - 0.5F * sk;
                           }
                       }
                       diag::detail("Knopftest: Stufe 7/6b - Knopfgroesse " + std::to_string(st.stufe) + ": Knopf " +
                                    std::to_string(knopf != nullptr ? knopf->b : -1) + ", Leiste " + std::to_string(leiste != nullptr ? leiste->h : -1) +
                                    ", Filterleiste " + std::to_string(filter != nullptr ? filter->b : -1) + " breit ab " +
                                    std::to_string(filter != nullptr ? filter->y : -1) + ", Ansichten darunter " + (mitte ? "ja" : "nein") +
                                    ", gespeichert " + einstellung7("BhedToolbarButtons"));
                       return ke::mass::knopfStufe == st.stufe && knopfOk && leisteOk && filterOk && mitte &&
                              einstellung7("BhedToolbarButtons") == std::to_string(st.stufe);
                   }));
        if (st.stufe == 0) { add(foto("karte7_knoepfe_klein")); }
        if (st.stufe == 2) { add(foto("karte7_knoepfe_gross")); }
    }
    // Preferences > Interface: "Menus like NetRadiant-Custom" und die Knopfgroesse
    add(tu("Preferences > Interface oeffnen", [] { (void)ke::ausfuehren("Preferences"); }));
    add(pause(0.3));
    add(klick(ke::trK(Str::KePgInterface), "###karteeinstellungen"));
    add(pause(0.3));
    add(foto("karte7_pref_interface"));
    add(klick(ke::trK(Str::KeMenueNrc), "###karteeinstellungen", 0, true));
    add(pause(0.2));
    add(klick(ke::trK(Str::KePrefOk), "###karteeinstellungen"));
    add(pause(0.4));
    add(pruefe("Stufe 7/6b: Preferences > Interface > Menus like NetRadiant-Custom: an und gespeichert, Leiste wieder 21 hoch (Segoe UI)", [] {
        const ImGuiWindow* w = ImGui::FindWindowByName("###main");
        const float sk = ke::massstab();
        return ke::editor().menueNrcStil && einstellung7("BhedMenuStyle") == "nrc" && w != nullptr &&
               std::fabs(w->MenuBarHeight - std::round(21.0F * sk)) < 0.5F;
    }));
    add(tu("Menuestil zurueck: Programm", [] { ke::menueStilSetzen(false); }));
    add(pause(0.3));
    add(pruefe("Stufe 7/6b: menueStilSetzen(false) gespeichert", [] { return !ke::editor().menueNrcStil && einstellung7("BhedMenuStyle") == "program"; }));
    // Strg + Mausrad: im Karten-Editor skaliert es das Programm nicht (NRC: das Rad gehoert den Ansichten)
    add({"Stufe 7/6b: Strg + Rad ueber der XY-Ansicht", [z](int b) {
             ImGuiIO& io = ImGui::GetIO();
             if (b == 0) {
                 const ke::Editor::RahmenImBild* r = rahmen7("XY");
                 if (r != nullptr) { setzeMaus(ImVec2{(r->x0 + r->x1) * 0.5F, (r->y0 + r->y1) * 0.5F}); }
                 z->skala = g_app->uiScale;
                 for (int i = 0; i < 3; ++i) { z->zoom[i] = ke::editor().xy[static_cast<std::size_t>(i)].ansicht.massstab; }
                 return false;
             }
             if (b == 2) { io.AddKeyEvent(ImGuiMod_Ctrl, true); return false; }
             if (b == 4) { io.AddMouseWheelEvent(0.0F, 1.0F); return false; }
             if (b == 7) { io.AddKeyEvent(ImGuiMod_Ctrl, false); return false; }
             return b >= 10;
         }});
    add(pruefe("Stufe 7/6b: Strg + Rad im Karten-Editor - Skalierung bleibt, die XY-Ansicht zoomt (wie NRC)", [z] {
        bool zoomt = false;
        for (int i = 0; i < 3; ++i) { zoomt = zoomt || ke::editor().xy[static_cast<std::size_t>(i)].ansicht.massstab != z->zoom[i]; }
        diag::detail("Knopftest: Stufe 7/6b - Strg+Rad: uiScale " + std::to_string(g_app->uiScale) + " (vorher " + std::to_string(z->skala) + "), XY " +
                     (zoomt ? "zoomt" : "zoomt nicht"));
        return std::fabs(g_app->uiScale - z->skala) < 0.001F && zoomt;
    }));
    // Fotos der Menues in zwei Farbschemata und allen vier Sprachen (View offen)
    for (const char* farbe : {"dark", "light"}) {
        for (const i18n::LanguageInfo& l : i18n::languages()) {
            const i18n::Language sprache = l.language;
            const std::string name = std::string("karte7_menue_") + farbe + "_" + l.code;
            add(tu("Foto " + name + ": Farbgebung und Sprache", [farbe, sprache] {
                karteDialogeZu();
                g_app->activeTheme = theme::findTheme(farbe);
                g_app->themePending = true;
                i18n::setLanguage(sprache);
            }));
            add(pause(0.3));
            add({"View oeffnen (" + name + ")", [](int b) {
                     static Schritt k;
                     if (b == 0) { k = klick(ke::trK(Str::KeLView), "###main"); }
                     return k.tun(b);
                 }});
            add(pause(0.3));
            add(foto(name));
        }
    }
    add(tu("Stufe 7/6b: Zustand zurueck", [z] {
        karteDialogeZu();
        ke::editor().menueNrcStil = z->nrcStil;
        ke::mass::knopfStufe = z->knopf;
        ke::einstellungenSpeichern();
        g_app->activeTheme = z->farben;
        g_app->themePending = true;
        i18n::setLanguage(z->sprache);
        g_app->uiScale = z->skala;
        ke::setzeNrcBegriffe(z->nrcBegriffe);
    }));
    add(pause(0.4));
    return s;
}

// --- Punkt 6c: Mausempfindlichkeit im Freelook (Tester: "terribly low mouse
// sensitivity ... changed to normal after changing to dark or something",
// erster Start ohne Einstellungsdatei). Der Zeiger wird simuliert (ZeigerSim,
// die Hauptschleife spielt Windows), keine echte Maus, keine Fotos. ---------

// NRC 20260114: die Vorgaben, die Maus und Ansichten betreffen (Quelle: Datei:Zeile)
struct NrcVorgabe {
    const char* schluessel;
    const char* wert;
    const char* quelle;
};
const NrcVorgabe kNrcVorgaben[] = {
    {"CamMoveSpeed", "500", "camwindow.cpp:151 m_nMoveSpeed = 500"},
    {"CamMoveTimeToMaxSpeed", "200", "camwindow.cpp:152 m_time_toMaxSpeed = 200"},
    {"ScrollMoveSpeed", "100", "camwindow.cpp:153 m_nScrollMoveSpeed = 100"},
    {"3DZoomInToPointer", "1", "camwindow.cpp:154 m_bZoomToPointer = true"},
    {"CamStrafeSpeed", "1", "camwindow.cpp:155 m_strafeSpeed = 1"},
    {"Sensitivity", "3", "camwindow.cpp:156 m_angleSpeed = 3"},
    {"CamInverseMouse", "0", "camwindow.cpp:157 m_bCamInverseMouse = false"},
    {"CamDiscrete", "1", "camwindow.cpp:158 m_bCamDiscrete = true"},
    {"CubicClipping", "0", "camwindow.cpp:159 m_bCubicClipping = false"},
    {"StrafeMode", "3", "camwindow.cpp:160 m_strafeMode = 3"},
    {"CameraFaceWire", "1", "camwindow.cpp:161 m_bFaceWire = true"},
    {"CameraFaceFill", "1", "camwindow.cpp:162 m_bFaceFill = true"},
    {"CameraMSAA", "8", "camwindow.cpp:163 m_MSAA = 8"},
    {"fieldOfView", "100", "camwindow.cpp:265 camera_t::fieldOfView = 100.0f"},
    {"CubicScale", "14", "camwindow.cpp m_nCubicScale = 14"},
    {"2DZoomInToPointer", "1", "xywindow.cpp g_xywindow_globals.m_bZoomInToPointer = true"},
    {"ChaseMouse", "1", "xywindow.cpp g_xywindow_globals_private.m_bChaseMouse = true"},
    {"XYMSAA", "8", "xywindow.cpp g_xywindow_globals.m_MSAA = 8"},
};

std::map<std::string, std::string>& einstellungenSicher7() {
    static std::map<std::string, std::string> m;
    return m;
}

// Freelook ein: M2 kurz in der Kameramitte (der simulierte Zeiger steht dort)
Schritt freelookUmschalten7(const std::string& was) {
    return {was, [](int b) {
                ImGuiIO& io = ImGui::GetIO();
                ke::ZeigerSim& z = ke::zeigerSim();
                const ke::KameraFenster& f = ke::editor().kamFenster;
                if (b == 0 && !f.frei) {
                    z.x = std::floor(f.bildX + static_cast<float>(f.breite) * 0.5F);
                    z.y = std::floor(f.bildY + static_cast<float>(f.hoehe) * 0.5F);
                }
                if (b == 2) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, true); }
                if (b == 3) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, false); }
                return b >= 6;
            }};
}

// Die Maus um weg Bildpunkte nach rechts, gleichmaessig ueber bilder Bilder
// (bilder = Bildrate bei einer Sekunde Mausweg); Windows meldet ganze Punkte.
Schritt mausWeg7(const std::string& was, int weg, int bilder) {
    return {was, [weg, bilder](int b) {
                ke::ZeigerSim& z = ke::zeigerSim();
                if (b >= 1 && b <= bilder) {
                    const int vorher = (weg * (b - 1)) / bilder;
                    const int jetzt = (weg * b) / bilder;
                    z.x += static_cast<float>(jetzt - vorher);
                }
                return b >= bilder + 6;
            }};
}

std::vector<Schritt> maus() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // Erster Start gegen gespeicherte Werte: jede Einstellung des Karten-Editors
    add(tu("Stufe 7/6c: Einstellungen merken, dann wie beim ersten Start (keine gespeicherten Werte)", [] {
        ke::einstellungenSpeichern();
        einstellungenSicher7() = g_app->settings.karte;
    }));
    add(pruefe("Stufe 7/6c: erster Start - die Kamera vor dem Laden (KameraEinstellungen{}) = nach dem Laden ohne Datei = NRC", [] {
        g_app->settings.karte.clear();
        const karte::KameraEinstellungen vorLaden{};
        ke::einstellungenLaden();
        const karte::KameraEinstellungen& c = ke::editor().kameraE;
        const bool gleich = c.tempo == vorLaden.tempo && c.zeitBisMax == vorLaden.zeitBisMax && c.radTempo == vorLaden.radTempo &&
                            c.zumZeiger == vorLaden.zumZeiger && c.strafeTempo == vorLaden.strafeTempo && c.winkelTempo == vorLaden.winkelTempo &&
                            c.mausUmkehren == vorLaden.mausUmkehren && c.diskret == vorLaden.diskret && c.kubisch == vorLaden.kubisch &&
                            c.strafeModus == vorLaden.strafeModus && c.flaechenDraht == vorLaden.flaechenDraht && c.faerben == vorLaden.faerben &&
                            c.msaa == vorLaden.msaa && c.kubikSkala == vorLaden.kubikSkala && c.sichtfeld == vorLaden.sichtfeld;
        diag::detail("Knopftest: Stufe 7/6c - erster Start: Sensitivity " + std::to_string(c.winkelTempo) + ", Tempo " + std::to_string(c.tempo) +
                     ", Strafe " + std::to_string(c.strafeTempo) + (gleich ? " (gleich vor/nach dem Laden)" : " (VERSCHIEDEN vor/nach dem Laden)"));
        return gleich && c.winkelTempo == 3.0F && c.tempo == 500;
    }));
    add(pruefe("Stufe 7/6c: erster Start - die gespeicherten Vorgaben wie NRC 20260114 (Kamera, 2D)", [] {
        ke::einstellungenSpeichern();
        std::string abweichung;
        for (const NrcVorgabe& v : kNrcVorgaben) {
            const auto it = g_app->settings.karte.find(v.schluessel);
            const std::string ist = it != g_app->settings.karte.end() ? it->second : std::string("(fehlt)");
            if (ist != v.wert) { abweichung += std::string(" ") + v.schluessel + "=" + ist + " (NRC " + v.wert + ", " + v.quelle + ")"; }
        }
        diag::detail("Knopftest: Stufe 7/6c - " + std::to_string(std::size(kNrcVorgaben)) + " NRC-Vorgaben, Abweichungen:" +
                     (abweichung.empty() ? std::string(" keine") : abweichung));
        return abweichung.empty();
    }));
    add(pruefe("Stufe 7/6c: jede Einstellung - erster Start gespeichert, wieder geladen und gespeichert: dieselben Werte", [] {
        const std::map<std::string, std::string> frisch = g_app->settings.karte;
        ke::einstellungenLaden();
        ke::einstellungenSpeichern();
        std::string abweichung;
        int n = 0;
        for (const auto& [k, v] : frisch) {
            ++n;
            const auto it = g_app->settings.karte.find(k);
            if (it == g_app->settings.karte.end() || it->second != v) {
                abweichung += " " + k + "=" + v + "->" + (it == g_app->settings.karte.end() ? std::string("(fehlt)") : it->second);
            }
        }
        diag::detail("Knopftest: Stufe 7/6c - " + std::to_string(n) + " Einstellungen, Abweichungen:" + (abweichung.empty() ? std::string(" keine") : abweichung));
        return abweichung.empty() && n > 50;
    }));
    add(tu("Stufe 7/6c: gespeicherte Einstellungen zurueck", [] {
        g_app->settings.karte = einstellungenSicher7();
        ke::einstellungenLaden();
    }));
    // Freelook: dieselbe Drehung je Mausweg bei jeder Bildrate, vor und nach
    // einem Farbschemawechsel und bei 100 / 125 / 150 % Windows-Skalierung
    add(tu("Stufe 7/6c: Testkarte, Anordnung Regular, Kamera wie NRC, simulierter Zeiger an", [] {
        karteDialogeZu();
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Regular;
        e.teilerFuer = -1;
        e.kameraE = karte::KameraEinstellungen{};
        ke::zeigerSim().an = true;
    }));
    add(pause(0.4));
    auto yaw = std::make_shared<float>(0.0F);
    struct Fall {
        int fps;
        float dpi;
        const char* farbe;
    };
    const Fall faelle[] = {{30, 1.0F, "dark"},  {144, 1.0F, "dark"},  {500, 1.0F, "dark"},  {30, 1.0F, "light"}, {144, 1.0F, "light"},
                           {500, 1.0F, "light"}, {144, 1.25F, "light"}, {500, 1.25F, "light"}, {30, 1.5F, "light"}, {500, 1.5F, "light"}};
    const char* farbeVorher = "";
    for (const Fall& fa : faelle) {
        if (std::strcmp(farbeVorher, fa.farbe) != 0) {
            const char* farbe = fa.farbe;
            add(tu(std::string("Stufe 7/6c: Farbschema ") + farbe, [farbe] {
                g_app->activeTheme = theme::findTheme(farbe);
                g_app->themePending = true;
            }));
            add(pause(0.3));
            farbeVorher = fa.farbe;
        }
        char name[160];
        std::snprintf(name, sizeof(name), "%d fps, Windows %d %%, %s", fa.fps, static_cast<int>(std::lround(fa.dpi * 100.0F)), fa.farbe);
        const std::string n = name;
        const float dpi = fa.dpi;
        add(tu("Stufe 7/6c: " + n + " - Massstab, Kamera merken", [dpi, yaw] {
            ke::freelookDpiErsatz = dpi;
            *yaw = ke::editor().kamera.winkel[1];
        }));
        add(freelookUmschalten7("Stufe 7/6c: M2 kurz - Freelook an (" + n + ")"));
        add(pruefe("Stufe 7/6c: Freelook an (" + n + ")", [] { return ke::editor().kamFenster.frei; }));
        add(mausWeg7("Stufe 7/6c: Maus 300 Bildpunkte nach rechts in einer Sekunde (" + n + ")", 300, fa.fps));
        add(pruefe("Stufe 7/6c: " + n + " - Drehung = 300 / Qt-Massstab x 0,0333333 x Sensitivity 3 (wie NRC)", [yaw, n, dpi] {
            const float qt = std::max(1.0F, std::round(dpi));
            const float soll = (300.0F / qt) * 0.0333333F * 3.0F;
            float ist = *yaw - ke::editor().kamera.winkel[1];
            if (ist < 0.0F) { ist += 360.0F; }
            char z[220];
            std::snprintf(z, sizeof(z), "Knopftest: Stufe 7/6c - %s: Drehung %.4f Grad (soll %.4f), Zeiger %d-mal gesetzt", n.c_str(), static_cast<double>(ist),
                          static_cast<double>(soll), ke::zeigerSim().gesetzt);
            diag::detail(z);
            return std::fabs(ist - soll) < 0.01F;
        }));
        add(pause(0.3));   // nicht als Doppel-M2 (context_menu_show)
        add(freelookUmschalten7("Stufe 7/6c: M2 kurz - Freelook aus (" + n + ")"));
        add(pruefe("Stufe 7/6c: Freelook aus (" + n + ")", [] { return !ke::editor().kamFenster.frei; }));
        add(pause(0.3));
    }
    // 2D: M2 ziehen verschiebt die XY-Ansicht (Move_Begin, FreezePointer) - derselbe Weg
    for (const int fps : {30, 500}) {
        const std::string n = std::to_string(fps) + " fps";
        auto vorher = std::make_shared<float>(0.0F);
        add(tu("Stufe 7/6c: XY verschieben " + n + " - Lage merken", [vorher] {
            ke::freelookDpiErsatz = 1.0F;
            *vorher = ke::editor().xy[0].ansicht.ursprung[0];
        }));
        add({"Stufe 7/6c: M2 in der XY-Ansicht halten, 300 Bildpunkte nach rechts (" + n + ")", [fps](int b) {
                 ImGuiIO& io = ImGui::GetIO();
                 ke::ZeigerSim& z = ke::zeigerSim();
                 if (b == 0) {
                     const ke::Editor::RahmenImBild* r = rahmen7("XY");
                     if (r != nullptr) {
                         z.x = std::floor((r->x0 + r->x1) * 0.5F);
                         z.y = std::floor((r->y0 + r->y1) * 0.5F);
                     }
                 }
                 if (b == 2) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, true); }
                 if (b >= 4 && b < 4 + fps) {
                     const int k = b - 3;
                     z.x += static_cast<float>((300 * k) / fps - (300 * (k - 1)) / fps);
                 }
                 if (b == fps + 8) { io.AddMouseButtonEvent(ImGuiMouseButton_Right, false); }
                 return b >= fps + 12;
             }});
        add(pruefe("Stufe 7/6c: XY verschieben " + n + " - um 300 / Massstab nach links (wie NRC)", [vorher, n] {
            const karte::XyAnsicht& v = ke::editor().xy[0].ansicht;
            const float soll = -300.0F / v.massstab;
            const float ist = v.ursprung[0] - *vorher;
            char z[200];
            std::snprintf(z, sizeof(z), "Knopftest: Stufe 7/6c - XY verschieben %s: %.3f Einheiten (soll %.3f)", n.c_str(), static_cast<double>(ist),
                          static_cast<double>(soll));
            diag::detail(z);
            return std::fabs(ist - soll) < 0.01F;
        }));
        add(tu("Menues zu", [] { karteDialogeZu(); }));
        add(pause(0.3));
    }
    add(tu("Stufe 7/6c: simulierter Zeiger aus, Massstab und Farbschema zurueck", [] {
        ke::zeigerSim().an = false;
        ke::freelookDpiErsatz = 0.0F;
        g_app->activeTheme = theme::findTheme("dark");
        g_app->themePending = true;
    }));
    add(pause(0.3));
    return s;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    const char* nur = std::getenv("BHED_STUFE7_NUR");
    const auto teil = [nur](const char* t) { return nur == nullptr || std::strcmp(nur, t) == 0; };
    const auto dazu = [&s](std::vector<Schritt> x) {
        for (Schritt& y : x) { s.push_back(std::move(y)); }
    };
    if (teil("licht")) { dazu(licht()); }
    if (teil("backen")) { dazu(backen()); }
    if (teil("gross")) { dazu(gross()); }
    if (teil("menue")) { dazu(menue()); }
    if (teil("maus")) { dazu(maus()); }
    return s;
}

}  // namespace stufe7
