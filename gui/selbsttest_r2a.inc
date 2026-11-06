// selbsttest_r2a.inc - rc610 R2a (BHED_EDITORTEST=karteneditor, BHED_KARTE_NUR=r2a): Reste vor rc610.
//
//   Gras: surfaceSprites in der Echtzeit-Ansicht (Wunsch Tester/shank 05.11.: "sprites ... like when u are on Yavin, those
//         long grasses on the grass textures"). Testraum mit textures/yavin/grasspatchy_sprite (yavin.shader: vertical 32 20 35
//         800, ssFademax 1500) auf vier Bodenpinseln und einem Patch; die Halme der Echtzeit-Ansicht gegen "Built map as in the
//         game" an derselben Kamera (Zahl und bedeckte Bildpunkte), Textur aendern -> naechstes Bild, die Halme einer Flaeche
//         werden nur bei ihrer Aenderung neu gebaut (Zaehler).
//         BHED_R2A_ZEIT=1: nur die Bildzeit (grosse Grasflaeche 4096 x 4096 aus 256 Pinseln, Gras aus/an, allein laufen lassen).
//   Nachfragen: das Kreuz des Hauptfensters (WM_CLOSE -> confirmQuit) fragt nach der ungespeicherten Karte wie File > Exit,
//         auch mit dem Map Editor in einem eigenen Fenster; das eigene Fenster schliessen behaelt die Karte.
// Eingebunden in gui/selbsttest_handbuch.inc (nach MODOBJ).
namespace r2a {

namespace ke = karteneditor;

constexpr const char* kGras = "textures/yavin/ground_grasssprite";
constexpr const char* kStein = "textures/yavin/temple_stone2";

std::string grasKarte() { return karteTestPfad("r2a_gras.map"); }
std::string grossKarte() { return karteTestPfad("r2a_grasgross.map"); }

// Ein Raum 2048 x 2048 x 512: Boden aus vier Pinseln mit Gras (Oberseite z = 0), ein Steinblock mit einem ebenen Patch (Gras)
// obenauf, vier Lichter, etwas Umgebungslicht.
void grasKarteSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    karte::TexturProjektion proj;
    karte::texdefStandard(karte::TexdefArt::Quake, proj);
    const auto quader = [&](const karte::Vektor3f& mitte, const karte::Vektor3f& halb, const char* shader) {
        auto p = std::make_shared<karte::Pinsel>();
        p->quaderBauen(karte::Kasten(mitte, halb), shader, proj, k.kontext);
        welt->kindEinfuegen(p);
    };
    for (const float x : {-512.0F, 512.0F}) {
        for (const float y : {-512.0F, 512.0F}) { quader(karte::Vektor3f(x, y, -8), karte::Vektor3f(512, 512, 8), kGras); }
    }
    quader(karte::Vektor3f(0, 0, 520), karte::Vektor3f(1040, 1040, 8), kStein);
    quader(karte::Vektor3f(-1032, 0, 256), karte::Vektor3f(8, 1024, 256), kStein);
    quader(karte::Vektor3f(1032, 0, 256), karte::Vektor3f(8, 1024, 256), kStein);
    quader(karte::Vektor3f(0, -1032, 256), karte::Vektor3f(1024, 8, 256), kStein);
    quader(karte::Vektor3f(0, 1032, 256), karte::Vektor3f(1024, 8, 256), kStein);
    quader(karte::Vektor3f(300, 300, 32), karte::Vektor3f(128, 128, 32), kStein);
    k.auswahl.leeren();
    karte::patchVorlage(k, karte::Kasten(karte::Vektor3f(300, 300, 64), karte::Vektor3f(128, 128, 0)), kGras, karte::PatchVorlage::Ebene, 2);
    k.auswahl.leeren();
    welt->setzen("_ambient", "120");
    for (const char* o : {"-500 -500 300", "500 -500 300", "-500 500 300", "500 500 300"}) {
        auto l = k.neueEntitaet("light", false);
        l->setzen("origin", o);
        l->setzen("light", "600");
        k.entitaetEinfuegen(l);
    }
    auto start = k.neueEntitaet("info_player_start", false);
    start->setzen("origin", "-900 -900 24");
    k.entitaetEinfuegen(start);
    (void)karte::karteSpeichern(grasKarte(), k, false);
}

// 4096 x 4096 Gras aus 16 x 16 Pinseln (256 x 256), Mauern, 16 Lichter
void grossKarteSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    karte::TexturProjektion proj;
    karte::texdefStandard(karte::TexdefArt::Quake, proj);
    const auto quader = [&](const karte::Vektor3f& mitte, const karte::Vektor3f& halb, const char* shader) {
        auto p = std::make_shared<karte::Pinsel>();
        p->quaderBauen(karte::Kasten(mitte, halb), shader, proj, k.kontext);
        welt->kindEinfuegen(p);
    };
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            quader(karte::Vektor3f(-1920.0F + 256.0F * static_cast<float>(i), -1920.0F + 256.0F * static_cast<float>(j), -8), karte::Vektor3f(128, 128, 8), kGras);
        }
    }
    quader(karte::Vektor3f(0, 0, 1032), karte::Vektor3f(2064, 2064, 8), kStein);
    quader(karte::Vektor3f(-2056, 0, 512), karte::Vektor3f(8, 2048, 512), kStein);
    quader(karte::Vektor3f(2056, 0, 512), karte::Vektor3f(8, 2048, 512), kStein);
    quader(karte::Vektor3f(0, -2056, 512), karte::Vektor3f(2048, 8, 512), kStein);
    quader(karte::Vektor3f(0, 2056, 512), karte::Vektor3f(2048, 8, 512), kStein);
    welt->setzen("_ambient", "120");
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            auto l = k.neueEntitaet("light", false);
            char o[64];
            std::snprintf(o, sizeof(o), "%d %d 400", -1536 + 1024 * i, -1536 + 1024 * j);
            l->setzen("origin", o);
            l->setzen("light", "900");
            k.entitaetEinfuegen(l);
        }
    }
    auto start = k.neueEntitaet("info_player_start", false);
    start->setzen("origin", "-1900 -1900 24");
    k.entitaetEinfuegen(start);
    (void)karte::karteSpeichern(grossKarte(), k, false);
}

struct Gras {
    stufe9c::Bild echtMit;
    stufe9c::Bild echtOhne;
    stufe9c::Bild spielMit;
    stufe9c::Bild spielOhne;
    std::size_t echtHalme = 0;
    int spielHalme = 0;
    std::size_t neuErst = 0;   // Halm-Flaechen gebaut nach dem ersten Bild
    std::size_t halmeVorher = 0;
    std::size_t neuVorher = 0;
};

// Bildpunkte, an denen die Halme das Bild aendern (eine Farbe um mehr als 24)
std::vector<char> halmMaske(const stufe9c::Bild& mit, const stufe9c::Bild& ohne) {
    std::vector<char> m;
    if (mit.breite != ohne.breite || mit.hoehe != ohne.hoehe || mit.rgba.size() != ohne.rgba.size()) { return m; }
    m.assign(mit.rgba.size() / 4U, 0);
    for (std::size_t i = 0; i < m.size(); ++i) {
        int d = 0;
        for (std::size_t c = 0; c < 3; ++c) { d = std::max(d, std::abs(static_cast<int>(mit.rgba[i * 4 + c]) - static_cast<int>(ohne.rgba[i * 4 + c]))); }
        m[i] = d > 24 ? 1 : 0;
    }
    return m;
}

void bmp(const std::string& name, const stufe9c::Bild& b) {
    const char* o = std::getenv("BHED_FOTOS");
    if (o != nullptr && o[0] != '\0') { stufe9c::bmpSchreiben(std::string(o) + "\\" + name + ".bmp", b); }
}

// Die Kamera wie im Spielbild: 1920 x 1080, Sichtfeld 90
void kamera(float x, float y, float z, float nicken, float gier) {
    ke::Editor& e = ke::editor();
    e.kamera.ursprung = karte::Vektor3f(x, y, z);
    e.kamera.winkel = karte::Vektor3f(nicken, gier, 0.0F);
}

std::vector<Schritt> grasSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto g = std::make_shared<Gras>();
    add(tu("R2a Gras: Testraum mit textures/yavin/grasspatchy_sprite, Echtzeit-Licht", [] {
        karteDialogeZu();
        ke::editor().karte.undo.alsGespeichertMarkieren();
        grasKarteSchreiben();
        (void)ke::karteOeffnen(grasKarte());
        ke::Editor& e = ke::editor();
        e.kamFenster.messBreite = 1920;
        e.kamFenster.messHoehe = 1080;
        e.kameraE.sichtfeld = 90.0F;
        e.kameraE.modus = karte::KameraModus::Textur;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        e.echtLicht.effekte = false;
        g_app->zeigeGras = true;
        ke::spielbildZeitFest(0.0);
        kamera(-700.0F, -700.0F, 140.0F, -14.0F, 45.0F);
    }));
    add(warteBis("R2a Gras: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    // erst wenn alle Schattenwuerfel gerechnet sind: sonst aendert sich zwischen den Bildern mit und ohne Halme das Licht
    add(warteBis("R2a Gras: Schattenwuerfel", [] { return ke::editor().echtLicht.wuerfelAusstehend == 0 && ke::editor().echtLicht.gezeichnet > 0; }, 6000));
    add(pause(1.0));
    add(tu("R2a Gras: Bild mit Halmen", [g] {
        const ke::Editor& e = ke::editor();
        g->echtHalme = e.echtLicht.grasVierecke;
        g->neuErst = e.echtLicht.grasNeu;
        (void)stufe9c::bildHolen(g->echtMit);
        bmp("r2a_gras_echt", g->echtMit);
        diag::detail("Knopftest: R2a Gras Echtzeit - " + std::to_string(e.echtLicht.grasVierecke) + " Halme, " + std::to_string(e.echtLicht.grasFlaechen) +
                     " Flaechen mit Halmen (" + std::to_string(e.echtLicht.grasPatches) + " Patches), gebaut " + std::to_string(e.echtLicht.grasNeu) + ", " +
                     std::to_string(e.echtLicht.grasAufrufe) + " Zeichenaufrufe, " + std::to_string(e.echtLicht.grasBytes) + " Bytes, " +
                     std::to_string(e.echtLicht.grasMs) + " ms, Pruefsumme " + std::to_string(e.echtLicht.grasPruefsumme));
    }));
    add(pruefe("R2a Gras: das Echtzeit-Licht zeichnet die Halme der Grasflaechen, auch auf dem Patch", [g] {
        return g->echtHalme > 0 && ke::editor().echtLicht.grasPatches > 0;
    }));
    add(tu("R2a Gras: Halme aus (Schalter der Vorschau)", [] { g_app->zeigeGras = false; }));
    add(pause(0.5));
    add(tu("R2a Gras: Bild ohne Halme", [g] {
        (void)stufe9c::bildHolen(g->echtOhne);
        bmp("r2a_gras_echt_ohne", g->echtOhne);
        g_app->zeigeGras = true;
    }));
    add(pause(0.5));
    add(pruefe("R2a Gras: ohne Aenderung der Flaechen wird nichts neu gebaut (Kamera bewegt, Halme aus und an)", [g] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: R2a Gras - gebaut " + std::to_string(g->neuErst) + " -> " + std::to_string(e.echtLicht.grasNeu));
        return g->neuErst > 0 && e.echtLicht.grasNeu == g->neuErst && e.echtLicht.grasVierecke == g->echtHalme;
    }));
    // Textur aendern: der Pinsel unten links (-512 -512) bekommt Stein - im naechsten Bild weniger Halme, nur er wird neu
    // gebaut, wenn er zurueckkommt (Rueckgaengig)
    add(tu("R2a Gras: ein Bodenpinsel bekommt Stein", [g] {
        ke::Editor& e = ke::editor();
        g->halmeVorher = e.echtLicht.grasVierecke;
        g->neuVorher = e.echtLicht.grasNeu;
        e.karte.auswahl.leeren();
        const auto welt = e.karte.welt();
        for (const auto& p : welt ? welt->kinder() : std::vector<std::shared_ptr<karte::Primitiv>>{}) {
            if (p->art() != karte::Primitiv::Art::Pinsel) { continue; }
            const karte::Vektor3f m = static_cast<karte::Pinsel&>(*p).grenzen(e.karte.kontext).origin;
            if (m[0] < -500.0F && m[0] > -524.0F && m[1] < -500.0F && m[1] > -524.0F && m[2] < 0.0F) {
                e.karte.auswahl.waehlen(welt, p);
            }
        }
        (void)karte::shaderSetzen(e.karte, e.bearbeitung.einstellung, kStein);
        e.karte.auswahl.leeren();
    }));
    add(pause(0.3));
    add(pruefe("R2a Gras: Textur geaendert - im naechsten Bild weniger Halme, nichts neu gebaut", [g] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: R2a Gras Textur - Halme " + std::to_string(g->halmeVorher) + " -> " + std::to_string(e.echtLicht.grasVierecke) + ", gebaut " +
                     std::to_string(g->neuVorher) + " -> " + std::to_string(e.echtLicht.grasNeu));
        return e.echtLicht.grasVierecke < g->halmeVorher && e.echtLicht.grasNeu == g->neuVorher;
    }));
    add(tu("R2a Gras: Rueckgaengig", [] { (void)ke::editor().karte.undo.undo(); }));
    add(pause(0.3));
    add(pruefe("R2a Gras: zurueck - dieselben Halme, neu gebaut nur die sechs Seiten des einen Pinsels", [g] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: R2a Gras Rueckgaengig - Halme " + std::to_string(e.echtLicht.grasVierecke) + " (vorher " + std::to_string(g->halmeVorher) +
                     "), gebaut " + std::to_string(g->neuVorher) + " -> " + std::to_string(e.echtLicht.grasNeu));
        return e.echtLicht.grasVierecke == g->halmeVorher && e.echtLicht.grasNeu == g->neuVorher + 6U;
    }));
    // dieselbe Kamera im Spielbild der gebauten Karte
    add(tu("R2a Gras: Schnell-Backen, Spielbild", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
        e.echtLicht.spielbild = true;
    }));
    add(stufe9c::langeWarten("R2a Gras: Schnell-Backen fertig", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("R2a Gras: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 400000));
    add(stufe9c::langeWarten("R2a Gras: Spielbild da", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    add(pause(1.0));
    add(tu("R2a Gras: Spielbild mit Halmen", [g] {
        g->spielHalme = ke::spielbildStand().grasVierecke;
        (void)stufe9c::bildHolen(g->spielMit);
        bmp("r2a_gras_spielbild", g->spielMit);
        g_app->zeigeGras = false;
    }));
    add(pause(0.5));
    add(tu("R2a Gras: Spielbild ohne Halme", [g] {
        (void)stufe9c::bildHolen(g->spielOhne);
        bmp("r2a_gras_spielbild_ohne", g->spielOhne);
        g_app->zeigeGras = true;
    }));
    add(pruefe("R2a Gras: Echtzeit-Licht und Spielbild - Halme gleich bis auf 10 %, bedeckte Bildpunkte bis auf 25 %, mindestens die Haelfte an denselben Stellen", [g] {
        const std::vector<char> me = halmMaske(g->echtMit, g->echtOhne);
        const std::vector<char> ms = halmMaske(g->spielMit, g->spielOhne);
        std::size_t ne = 0;
        std::size_t ns = 0;
        std::size_t beide = 0;
        for (std::size_t i = 0; i < me.size() && i < ms.size(); ++i) {
            ne += me[i] != 0 ? 1U : 0U;
            ns += ms[i] != 0 ? 1U : 0U;
            beide += (me[i] != 0 && ms[i] != 0) ? 1U : 0U;
        }
        const double halme = g->spielHalme > 0 ? static_cast<double>(g->echtHalme) / static_cast<double>(g->spielHalme) : 0.0;
        const double flaeche = ns > 0 ? static_cast<double>(ne) / static_cast<double>(ns) : 0.0;
        const double gemeinsam = std::max(ne, ns) > 0 ? static_cast<double>(beide) / static_cast<double>(std::max(ne, ns)) : 0.0;
        char z[400];
        std::snprintf(z, sizeof(z), "Knopftest: R2a Gras Vergleich - Halme Echtzeit %zu, Spielbild %d (%.3f); Bildpunkte %zu / %zu (%.3f), gemeinsam %.3f",
                      g->echtHalme, g->spielHalme, halme, ne, ns, flaeche, gemeinsam);
        diag::detail(z);
        // Toleranz: die Halme sind dieselben (Zahl auf 10 %: q3map2 kann Pinselseiten an Ebenen des Baums zerschneiden, dann
        // liegen die Halme dort anders). Ihre Bildpunkte zaehlen, wo sie das Bild um mehr als 24 aendern - das haengt am Licht: hier
        // das Echtzeit-Licht je Bildpunkt, im Spielbild die Eckenfarben von q3map2 -light -fast. Gemessen 0,785 und 0,567 bei
        // gleicher Halmzahl (r2a_g2): dunkle Halme auf dunklem Boden fallen je nach Licht unter die Schwelle.
        return g->spielHalme > 0 && halme > 0.9 && halme < 1.1 && flaeche > 0.75 && flaeche < 1.33 && gemeinsam > 0.5;
    }));
    add(tu("R2a Gras: aufraeumen", [] {
        ke::spielbildZeitFest(-1.0);
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        e.echtLicht.an = false;
        e.echtLicht.effekte = true;
        e.kamFenster.messBreite = 0;
        e.kamFenster.messHoehe = 0;
        e.filterMaske = 0;
        e.karte.undo.alsGespeichertMarkieren();
    }));
    return s;
}

// Bildzeit allein: grosse Grasflaeche, 240 Bilder mit drehender Kamera, Gras aus und an
std::vector<Schritt> zeitSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(tu("R2a Zeit: grosse Grasflaeche, Echtzeit-Licht", [] {
        karteDialogeZu();
        grossKarteSchreiben();
        (void)ke::karteOeffnen(grossKarte());
        ke::Editor& e = ke::editor();
        e.kameraE.modus = karte::KameraModus::Textur;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        kamera(-1800.0F, -1800.0F, 80.0F, -8.0F, 45.0F);
    }));
    add(warteBis("R2a Zeit: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(warteBis("R2a Zeit: Schattenwuerfel", [] { return ke::editor().echtLicht.wuerfelAusstehend == 0 && ke::editor().echtLicht.gezeichnet > 0; }, 6000));
    // zwei Stellen: die Ecke (vorher/nachher vergleichbar) und die Mitte der Flaeche (die meisten Halme in Reichweite)
    for (int fall = 0; fall < 4; ++fall) {
        const int stelle = fall / 2;
        const bool gras = (fall % 2) == 1;
        const std::string name = std::string(stelle == 0 ? "Ecke " : "Mitte ") + (gras ? "mit Gras" : "ohne Gras");
        add(tu("R2a Zeit: " + name, [gras, stelle] {
            g_app->zeigeGras = gras;
            if (stelle == 0) {
                kamera(-1800.0F, -1800.0F, 80.0F, -8.0F, 45.0F);
            } else {
                kamera(0.0F, 0.0F, 80.0F, -8.0F, 45.0F);
            }
        }));
        add(pause(1.0));
        auto bilder = std::make_shared<std::vector<double>>();
        auto grasMs = std::make_shared<double>(0.0);
        add({"R2a Zeit: 240 Bilder " + name, [bilder, grasMs](int b) {
                 ke::Editor& e = ke::editor();
                 if (b > 2) {
                     bilder->push_back(static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0);
                     *grasMs += e.echtLicht.grasMs;
                 }
                 e.kamera.freiBewegen(6, 0, e.kameraE);
                 return b >= 242;
             }});
        add(tu("R2a Zeit: Ergebnis " + name, [bilder, grasMs, name] {
            std::vector<double> v = *bilder;
            std::sort(v.begin(), v.end());
            double summe = 0;
            for (const double x : v) { summe += x; }
            const double mittel = v.empty() ? 0 : summe / static_cast<double>(v.size());
            const double p99 = v.empty() ? 0 : v[std::min(v.size() - 1, v.size() * 99 / 100)];
            const ke::Editor& e = ke::editor();
            char z[400];
            std::snprintf(z, sizeof(z), "R2a-Bildzeit: %s Mittel %.2f ms, p99 %.2f ms, Halme %zu, Halme je Bild %.2f ms, Grafikpuffer %zu Bytes, Lichtliste %.2f ms",
                          name.c_str(), mittel, p99, e.echtLicht.grasVierecke, v.empty() ? 0.0 : *grasMs / static_cast<double>(v.size()), e.echtLicht.grasBytes,
                          e.echtLicht.lichtMs);
            diag::detail(z);
        }));
    }
    add(tu("R2a Zeit: Ende", [] {
        ke::editor().echtLicht.an = false;
        g_app->zeigeGras = true;
    }));
    return s;
}

// --- Nachfragen beim Schliessen ------------------------------------------------------------------------------------------
std::vector<Schritt> schliessenSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto merk = std::make_shared<bool>(false);
    auto antwort = std::make_shared<int>(-1);
    // Endrunde rc610 (voller karteneditor-Lauf): File > Exit, die Karte verworfen, dann "Exit?" mit Nein - das Programm laeuft
    // weiter, die Karte wird wieder geaendert. Das Kreuz muss dann wieder nach der Karte fragen (beendenBeantwortet blieb stehen).
    auto frageKam = std::make_shared<bool>(false);
    add(tu("R2a Schliessen: File > Exit, Karte verwerfen, \"Exit?\" Nein, Karte wieder aendern, Kreuz", [merk, antwort, frageKam] {
        karteDialogeZu();
        ke::Editor& e = ke::editor();
        e.karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
        (void)e.karte.weltHolenOderAnlegen();
        {
            karte::Befehl b(e.karte.undo, "brushCreate");
            (void)stufe2::testPinsel(karte::Vektor3f(0, 0, 0), karte::Vektor3f(64, 64, 64), kStein);
        }
        *merk = g_app->settings.queryOnDiscard;
        g_app->settings.queryOnDiscard = true;
        g_app->quitConfirmed = false;
        g_app->wantQuit = false;
        (void)ke::ausfuehren("Exit");
        std::function<void()> verwerfen = std::move(e.nachFrage);   // Rueckfrage "Exit Radiant": Discard
        e.nachFrage = nullptr;
        e.frageOffen = false;
        if (verwerfen) { verwerfen(); }
        g_app->wantQuit = false;   // falls doch nicht gefragt wurde: nicht wirklich beenden (die Pruefung meldet es)
        g_app->quitConfirmed = false;
        std::function<void()> nein = std::move(g_app->frageNein);   // "Exit?": Nein
        *frageKam = static_cast<bool>(nein);
        g_app->frageJa = nullptr;
        g_app->frageNein = nullptr;
        g_app->frageOffen = false;
        if (nein) { nein(); }
        {
            karte::Befehl b(e.karte.undo, "brushCreate");
            (void)stufe2::testPinsel(karte::Vektor3f(128, 0, 0), karte::Vektor3f(192, 64, 64), kStein);
        }
        g_app->settings.queryOnDiscard = *merk;
        *antwort = confirmQuit() ? 1 : 0;
    }));
    add(pause(0.3));
    add(pruefe("R2a Schliessen: nach abgebrochenem Beenden fragt das Kreuz wieder nach der geaenderten Karte", [antwort, frageKam] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: R2a Schliessen nach Abbruch - \"Exit?\" " + std::string(*frageKam ? "ja" : "nein") + ", confirmQuit " +
                     std::to_string(*antwort) + ", Frage " + (e.frageOffen ? "ja" : "nein"));
        return *frageKam && *antwort == 0 && e.frageOffen && !g_app->wantQuit;
    }));
    add(tu("R2a Schliessen: die Frage schliessen (Cancel), Karte als gespeichert", [] {
        ke::Editor& e = ke::editor();
        e.nachFrage = nullptr;
        e.frageOffen = false;
        e.karte.undo.alsGespeichertMarkieren();
        g_app->quitConfirmed = false;
        g_app->wantQuit = false;
    }));
    add(tu("R2a Schliessen: neue Karte mit einem Pinsel (ungespeichert), Kreuz des Hauptfensters (confirmQuit)", [merk, antwort] {
        karteDialogeZu();
        ke::Editor& e = ke::editor();
        e.karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
        (void)e.karte.weltHolenOderAnlegen();
        {
            karte::Befehl b(e.karte.undo, "brushCreate");
            (void)stufe2::testPinsel(karte::Vektor3f(0, 0, 0), karte::Vektor3f(64, 64, 64), kStein);
        }
        *merk = g_app->settings.queryOnDiscard;
        g_app->settings.queryOnDiscard = false;   // nur die Frage nach der Karte, nicht "Exit?"
        g_app->leftMode = 0;
        g_app->quitConfirmed = false;
        g_app->wantQuit = false;
        *antwort = confirmQuit() ? 1 : 0;
    }));
    add(pause(0.3));
    add(pruefe("R2a Schliessen: das Kreuz fragt nach der ungespeicherten Karte, der Map Editor ist zu sehen, das Fenster bleibt", [antwort] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: R2a Schliessen - confirmQuit " + std::to_string(*antwort) + ", Frage " + (e.frageOffen ? "ja" : "nein") + ", Tab " +
                     std::to_string(g_app->leftMode) + ", Karte geaendert " + (ke::geaendert() ? "ja" : "nein"));
        return *antwort == 0 && e.frageOffen && e.frageArt == 0 && tabfenster::sichtbar(3) && ke::geaendert();
    }));
    add(klick(tr(Str::KeCancel), "kartefrage"));
    add(pause(0.3));
    add(pruefe("R2a Schliessen: Abbrechen - kein Beenden, die Karte bleibt ungespeichert", [] {
        return !g_app->wantQuit && !g_app->quitConfirmed && !ke::editor().frageOffen && ke::geaendert();
    }));
    // Der Map Editor in einem eigenen Fenster: dessen Kreuz bringt den Tab zurueck (ohne Frage, die Karte bleibt); das Kreuz des
    // Hauptfensters fragt im Fenster des Map Editors
    add(tu("R2a Schliessen: Map Editor in ein eigenes Fenster", [] {
        g_app->leftMode = 0;
        (void)tabfenster::loesen(3);
    }));
    add(pause(0.5));
    add(tu("R2a Schliessen: Kreuz des Hauptfensters mit dem Map Editor im eigenen Fenster", [antwort] {
        g_app->quitConfirmed = false;
        *antwort = confirmQuit() ? 1 : 0;
    }));
    add(pause(0.3));
    add(pruefe("R2a Schliessen: auch mit dem eigenen Fenster fragt es nach der Karte", [antwort] {
        diag::detail("Knopftest: R2a Schliessen eigenes Fenster - eigenes " + std::string(tabfenster::eigenes(3) ? "ja" : "nein") + ", confirmQuit " +
                     std::to_string(*antwort) + ", Frage " + (ke::editor().frageOffen ? "ja" : "nein"));
        return tabfenster::eigenes(3) && *antwort == 0 && ke::editor().frageOffen;
    }));
    add(klick(tr(Str::KeCancel), "kartefrage"));
    add(pause(0.3));
    add(tu("R2a Schliessen: das eigene Fenster schliessen (der Tab kommt zurueck)", [] { tabfenster::zurueck(3); }));
    add(pause(0.5));
    add(pruefe("R2a Schliessen: das eigene Fenster zu - keine Frage, kein Beenden, die Karte ist noch da und ungespeichert", [] {
        return !tabfenster::eigenes(3) && !ke::editor().frageOffen && !g_app->wantQuit && ke::geaendert() && !ke::editor().karte.welt()->kinder().empty();
    }));
    add(tu("R2a Schliessen: aufraeumen", [merk] {
        g_app->settings.queryOnDiscard = *merk;
        g_app->quitConfirmed = false;
        ke::editor().karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
    }));
    return s;
}

std::vector<Schritt> schritte() {
    if (const char* z = std::getenv("BHED_R2A_ZEIT"); z != nullptr && z[0] == '1') { return zeitSchritte(); }
    std::vector<Schritt> s = grasSchritte();
    for (Schritt& x : schliessenSchritte()) { s.push_back(std::move(x)); }
    return s;
}

}  // namespace r2a
