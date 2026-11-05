// selbsttest_kartelicht1.inc - rc610 LICHT1: das Echtzeit-Licht zeigt die ganze Karte (BHED_KARTE_NUR=licht1).
//
// Meldung (Nutzer, 05.11.): "when I go back for example to the start of the hangar I do not see the light at the end of
// the hangar". Testkarte "langer Hangar" (erzeugt): 4096 lang, 1024 breit, 512 hoch, Saeulen in zwei Reihen, 576
// Punktlichter in neun Reihen (mehr als die 512, die bis rc609 je Bild hoechstens kamen). Die Kamera am Anfang, in der Mitte und am Ende; je Punkt der ferne Teil des Bilds (die
// Mitte, dort liegt das Ende des Hangars) und der nahe (unten) gegen das gebackene Licht (Schnell-Backen, wie Stufe 9c),
// dazu die Zaehler der Vorschau: kein Licht ohne Schatten, keins fehlt, schon im ersten Bild; ein Rundgang rechnet keinen
// Schattenwuerfel neu. Mit BHED_LICHT_BILDER die Bilder als .bmp (Vergleich vorher/nachher ausserhalb).
// Eingebunden in gui/selbsttest_handbuch.inc (nach Stufe 9d).
namespace licht1 {

namespace ke = karteneditor;

constexpr float kLaenge = 4096.0F;
constexpr float kHalbBreite = 512.0F;
constexpr float kHoehe = 512.0F;

std::string hangarKarte() { return karteTestPfad("li1_hangar.map"); }

void hangarSchreiben() {
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
    const float mx = kLaenge * 0.5F;
    quader(karte::Vektor3f(mx, 0, -8), karte::Vektor3f(mx + 16, kHalbBreite + 16, 8));                       // Boden
    quader(karte::Vektor3f(mx, 0, kHoehe + 8), karte::Vektor3f(mx + 16, kHalbBreite + 16, 8));               // Decke
    quader(karte::Vektor3f(mx, -(kHalbBreite + 8), kHoehe * 0.5F), karte::Vektor3f(mx + 16, 8, kHoehe * 0.5F));
    quader(karte::Vektor3f(mx, kHalbBreite + 8, kHoehe * 0.5F), karte::Vektor3f(mx + 16, 8, kHoehe * 0.5F));
    quader(karte::Vektor3f(-8, 0, kHoehe * 0.5F), karte::Vektor3f(8, kHalbBreite, kHoehe * 0.5F));
    quader(karte::Vektor3f(kLaenge + 8, 0, kHoehe * 0.5F), karte::Vektor3f(8, kHalbBreite, kHoehe * 0.5F));
    for (float x = 256.0F; x < kLaenge; x += 256.0F) {   // Saeulen in zwei Reihen
        for (const float y : {-(kHalbBreite - 160.0F), kHalbBreite - 160.0F}) { quader(karte::Vektor3f(x, y, kHoehe * 0.5F), karte::Vektor3f(16, 16, kHoehe * 0.5F)); }
    }
    for (const float y : {-448.0F, -336.0F, -224.0F, -112.0F, 0.0F, 112.0F, 224.0F, 336.0F, 448.0F}) {   // 9 Reihen zu 64 Lichtern
        for (float x = 32.0F; x < kLaenge; x += 64.0F) {
            auto l = k.neueEntitaet("light", false);
            char o[64];
            std::snprintf(o, sizeof(o), "%.0f %.0f %.0f", x, y, kHoehe - 64.0F);
            l->setzen("origin", o);
            l->setzen("light", "120");
            k.entitaetEinfuegen(l);
        }
    }
    auto start = k.neueEntitaet("info_player_start", false);
    start->setzen("origin", "64 0 24");
    k.entitaetEinfuegen(start);
    (void)karte::karteSpeichern(hangarKarte(), k, false);
}

// Kamerapunkte: am Anfang zum Ende, in der Mitte zum Ende, am Ende zum Anfang (Blick waagerecht)
const std::array<stufe9c::Punkt, 3>& punkte() {
    static const std::array<stufe9c::Punkt, 3> p = {stufe9c::Punkt{karte::Vektor3f(96, 0, 160), 0.0F, 0.0F},
                                                     stufe9c::Punkt{karte::Vektor3f(kLaenge * 0.5F, 0, 160), 0.0F, 0.0F},
                                                     stufe9c::Punkt{karte::Vektor3f(kLaenge - 96, 0, 160), 0.0F, 180.0F}};
    return p;
}

// Ein Teil des Bilds gegen das gebackene: Bloecke 16 x 16 wie Stufe 9c, dazu die mittlere Helligkeit beider
struct Teil {
    stufe9c::Abweichung a;
    double echt = 0;
    double gebacken = 0;
};

Teil bereich(const stufe9c::Bild& e, const stufe9c::Bild& g, double fx0, double fy0, double fx1, double fy1) {
    Teil t;
    if (e.breite != g.breite || e.hoehe != g.hoehe || e.breite < 64 || e.hoehe < 64) { return t; }
    const int x0 = static_cast<int>(fx0 * e.breite);
    const int y0 = static_cast<int>(fy0 * e.hoehe);
    const int w = (static_cast<int>(fx1 * e.breite) - x0) / 16 * 16;
    const int h = (static_cast<int>(fy1 * e.hoehe) - y0) / 16 * 16;
    stufe9c::Bild a;
    stufe9c::Bild b;
    a.breite = b.breite = w;
    a.hoehe = b.hoehe = h;
    for (int y = 0; y < h; ++y) {
        const std::size_t von = (static_cast<std::size_t>(y0 + y) * static_cast<std::size_t>(e.breite) + static_cast<std::size_t>(x0)) * 4U;
        a.rgba.insert(a.rgba.end(), e.rgba.begin() + static_cast<std::ptrdiff_t>(von), e.rgba.begin() + static_cast<std::ptrdiff_t>(von + static_cast<std::size_t>(w) * 4U));
        b.rgba.insert(b.rgba.end(), g.rgba.begin() + static_cast<std::ptrdiff_t>(von), g.rgba.begin() + static_cast<std::ptrdiff_t>(von + static_cast<std::size_t>(w) * 4U));
    }
    t.a = stufe9c::vergleichen(a, b);
    for (std::size_t i = 0; i + 3 < a.rgba.size(); i += 4) {
        t.echt += stufe9c::helligkeit(&a.rgba[i]);
        t.gebacken += stufe9c::helligkeit(&b.rgba[i]);
    }
    const double n = std::max(1.0, static_cast<double>(w) * static_cast<double>(h));
    t.echt /= n;
    t.gebacken /= n;
    return t;
}

struct Lauf {
    std::vector<stufe9c::Bild> echt;
    std::vector<Teil> fern;
    std::vector<Teil> nah;
    std::size_t ersteFehlend = 0;
    std::size_t ersteOhneSchatten = 0;
    std::size_t ersteGezeichnet = 0;
    std::size_t neuVorRundgang = 0;
    std::size_t neuNachRundgang = 0;
    std::size_t fehlendMax = 0;      // im Rundgang
    std::size_t ohneSchattenMax = 0;
};

void bildSchreiben(const stufe9c::Bild& b, const std::string& name) {
    const std::string o = stufe9c::bilderOrdner();
    if (!o.empty()) { stufe9c::bmpSchreiben(o + "\\li1_" + name + ".bmp", b); }
}

std::vector<Schritt> hangar() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto lauf = std::make_shared<Lauf>();
    add(tu("Licht1: Hangar schreiben und oeffnen (4096 lang, 576 Lichter, Saeulen)", [] {
        karteDialogeZu();
        hangarSchreiben();
        ke::Editor& e = ke::editor();
        e.echtLicht.an = false;
        e.echtLicht.backen = false;
        (void)ke::karteOeffnen(hangarKarte());
        e.kameraE.modus = karte::KameraModus::Textur;
        stufe9c::kameraAuf(punkte()[0]);
    }));
    add(warteBis("Licht1: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(0.3));
    add(tu("Licht1: Echtzeit-Licht einschalten", [] { ke::editor().echtLicht.an = true; }));
    // das erste Bild nach dem Einschalten (bzw. nach dem Vorberechnen): kein Licht fehlt, keins ohne Schatten
    add(warteBis("Licht1: Schattenwuerfel vorberechnet", [] { return ke::editor().echtLicht.wuerfelAusstehend == 0 && ke::editor().echtLicht.gezeichnet > 0; }, 6000));
    add(tu("Licht1: das erste vollstaendige Bild", [lauf] {
        const ke::Editor& e = ke::editor();
        lauf->ersteFehlend = e.echtLicht.fehlend;
        lauf->ersteOhneSchatten = e.echtLicht.ohneSchatten;
        lauf->ersteGezeichnet = e.echtLicht.gezeichnet;
        char z[200];
        std::snprintf(z, sizeof(z), "Knopftest: Licht1 - Lichter %zu, Grafikkarte %lld MB", e.echtLicht.lichter, gpu::speicherStand().grafikBytes / 1048576);
        diag::detail(z);
    }));
    add(pause(1.2));
    for (std::size_t i = 0; i < punkte().size(); ++i) {
        add(tu("Licht1: Echtzeit-Punkt " + std::to_string(i), [i] { stufe9c::kameraAuf(punkte()[i]); }));
        add(pause(1.2));
        add(tu("Licht1: Echtzeit-Bild " + std::to_string(i), [lauf, i] {
            stufe9c::Bild b;
            if (!stufe9c::bildHolen(b)) { meldeFehler("Licht1: Kamerabild nicht lesbar"); }
            bildSchreiben(b, "p" + std::to_string(i) + "_echt");
            lauf->echt.push_back(std::move(b));
        }));
    }
    // Rundgang: vom Anfang zum Ende und zurueck (240 Bilder), dabei die Zaehler
    add(tu("Licht1: Rundgang beginnt", [lauf] {
        lauf->neuVorRundgang = ke::editor().echtLicht.schattenNeu;
        stufe9c::kameraAuf(punkte()[0]);
    }));
    add({"Licht1: Rundgang 240 Bilder", [lauf](int b) {
             ke::Editor& e = ke::editor();
             if (b > 0) {
                 lauf->fehlendMax = std::max(lauf->fehlendMax, e.echtLicht.fehlend);
                 lauf->ohneSchattenMax = std::max(lauf->ohneSchattenMax, e.echtLicht.ohneSchatten);
             }
             const float t = static_cast<float>(b <= 120 ? b : 240 - b) / 120.0F;
             e.kamera.ursprung = karte::Vektor3f(96.0F + t * (kLaenge - 192.0F), 0.0F, 160.0F);
             e.kamera.winkel = karte::Vektor3f(0.0F, b <= 120 ? 0.0F : 180.0F, 0.0F);
             return b >= 240;
         }});
    add(tu("Licht1: Rundgang Ende", [lauf] {
        lauf->neuNachRundgang = ke::editor().echtLicht.schattenNeu;
        char z[300];
        std::snprintf(z, sizeof(z), "Knopftest: Licht1 - erstes Bild: gezeichnet %zu, fehlend %zu, ohne Schatten %zu; Rundgang: Wuerfel neu %zu, fehlend max %zu, ohne Schatten max %zu",
                      lauf->ersteGezeichnet, lauf->ersteFehlend, lauf->ersteOhneSchatten, lauf->neuNachRundgang - lauf->neuVorRundgang, lauf->fehlendMax,
                      lauf->ohneSchattenMax);
        diag::detail(z);
    }));
    add(tu("Licht1: Schnell-Backen an", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
    }));
    add(stufe9c::langeWarten("Licht1: gebacken", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("Licht1: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 400000));
    add(warteBis("Licht1: Texturen (gebacken)", [] { return ke::kameraBilderFertig(); }, 20000));
    for (std::size_t i = 0; i < punkte().size(); ++i) {
        add(tu("Licht1: gebackener Punkt " + std::to_string(i), [i] { stufe9c::kameraAuf(punkte()[i]); }));
        add(pause(0.5));
        add(tu("Licht1: gebackenes Bild " + std::to_string(i), [lauf, i] {
            if (i >= lauf->echt.size()) { return; }
            stufe9c::Bild b;
            if (!stufe9c::bildHolen(b)) { meldeFehler("Licht1: Kamerabild nicht lesbar"); }
            bildSchreiben(b, "p" + std::to_string(i) + "_gebacken");
            // fern: die Wand am Ende des Hangars (Blick waagerecht in 160 Hoehe: um die Bildmitte, 4000 bzw. 2000 Einheiten weit);
            // nah: das untere Viertel (der Boden vor der Kamera)
            lauf->fern.push_back(bereich(lauf->echt[i], b, 0.46, 0.43, 0.54, 0.53));
            lauf->nah.push_back(bereich(lauf->echt[i], b, 0.0, 0.75, 1.0, 1.0));
            const Teil& f = lauf->fern.back();
            const Teil& n = lauf->nah.back();
            char z[400];
            std::snprintf(z, sizeof(z), "Licht1: P%zu fern Block-Mittel %.2f Max %.2f (Helligkeit echt %.1f, gebacken %.1f) | nah Block-Mittel %.2f Max %.2f (echt %.1f, gebacken %.1f)", i,
                          f.a.mittel, f.a.max, f.echt, f.gebacken, n.a.mittel, n.a.max, n.echt, n.gebacken);
            diag::detail(z);
        }));
    }
    add(tu("Licht1: Backen und Echtzeit-Licht aus", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.an = false;
        e.echtLicht.pauseSekunden = 2.0;
    }));
    // die Grenzen der Lichtmessung (Stufe 9c: je Punkt Mittel <= 15, Max <= 200), hier je Teil des Bilds
    add(pruefe("Licht1: fern und nah wie gebacken (je Punkt und Teil Block-Mittel <= 15, Max <= 200)", [lauf] {
        bool gut = lauf->fern.size() == punkte().size() && lauf->nah.size() == punkte().size();
        for (const std::vector<Teil>* v : {&lauf->fern, &lauf->nah}) {
            for (const Teil& t : *v) { gut = gut && t.a.gueltig && t.a.mittel <= 15.0 && t.a.max <= 200.0; }
        }
        return gut;
    }));
    add(pruefe("Licht1: im ersten vollstaendigen Bild fehlt kein Licht und keins ist ohne Schatten", [lauf] {
        return lauf->ersteGezeichnet > 0 && lauf->ersteFehlend == 0 && lauf->ersteOhneSchatten == 0;
    }));
    add(pruefe("Licht1: Rundgang - kein Schattenwuerfel neu, kein Licht fehlt oder ist ohne Schatten", [lauf] {
        return lauf->neuNachRundgang == lauf->neuVorRundgang && lauf->fehlendMax == 0 && lauf->ohneSchattenMax == 0;
    }));
    return s;
}

// Unterschied zum grossen Wuerfel (Anteil der Bildpunkte in Prozent): ein Bildpunkt zaehlt, wenn seine Helligkeit um
// mehr als 2 von der im Bild mit lauter 512er Wuerfeln abweicht und auch keiner seiner Nachbarn (3 x 3) dort so hell
// ist - der Schatten liegt dann mehr als einen Bildpunkt anders.
double ueberEinemBildpunkt(const stufe9c::Bild& a, const stufe9c::Bild& b) {
    if (a.breite != b.breite || a.hoehe != b.hoehe || a.breite < 3 || a.hoehe < 3) { return 100.0; }
    const auto h = [](const stufe9c::Bild& x, int px, int py) {
        return stufe9c::helligkeit(&x.rgba[(static_cast<std::size_t>(py) * static_cast<std::size_t>(x.breite) + static_cast<std::size_t>(px)) * 4U]);
    };
    std::size_t weit = 0;
    for (int y = 1; y + 1 < a.hoehe; ++y) {
        for (int x = 1; x + 1 < a.breite; ++x) {
            const double v = h(a, x, y);
            if (std::fabs(v - h(b, x, y)) <= 2.0) { continue; }
            bool nachbar = false;
            for (int dy = -1; dy <= 1 && !nachbar; ++dy) {
                for (int dx = -1; dx <= 1 && !nachbar; ++dx) { nachbar = std::fabs(v - h(b, x + dx, y + dy)) <= 2.0; }
            }
            weit += nachbar ? 0U : 1U;
        }
    }
    return 100.0 * static_cast<double>(weit) / (static_cast<double>(a.breite - 2) * static_cast<double>(a.hoehe - 2));
}

// Die Groesse der Wuerfel nach der Huelle gegen lauter 512er (BHED_LICHT1_KARTEN, Eintraege wie Stufe 9c), an den
// Kamerapunkten der Stufe 9c: Grenze 0,1 % der Bildpunkte ueber einem Bildpunkt Versatz
std::vector<Schritt> groessenVergleich(const std::string& eintrag) {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    struct Stand {
        std::vector<stufe9c::Punkt> punkte;
        std::vector<stufe9c::Bild> huelle;
        std::vector<double> anteil;
        std::size_t bytesHuelle = 0;
        std::size_t bytes512 = 0;
        bool passt = true;
    };
    auto st = std::make_shared<Stand>();
    const std::string name = stufe9c::dateiName(eintrag.substr(0, eintrag.find('|')));
    add(tu("Licht1: Groessenvergleich " + name, [st, eintrag] {
        (void)stufe9c::karteMitLichternOeffnen(eintrag);
        ke::Editor& e = ke::editor();
        st->punkte = stufe9c::kameraPunkte();
        e.kameraE.modus = karte::KameraModus::Textur;
        ke::echtLichtWuerfelFest(0);
        e.echtLicht.an = true;
        e.echtLicht.effekte = false;   // die Teilchen bewegen sich zwischen den Bildern
        stufe9c::kameraAuf(st->punkte.front());
    }));
    add(warteBis("Licht1: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    for (const int fest : {0, 512}) {
        add(tu("Licht1: Wuerfel " + std::string(fest == 0 ? "nach der Huelle" : "alle 512"), [fest] { ke::echtLichtWuerfelFest(fest); }));
        add(pause(0.3));
        add(stufe9c::langeWarten("Licht1: Wuerfel gerechnet", [] { return ke::editor().echtLicht.wuerfelAusstehend == 0; }, 20000));
        add(tu("Licht1: Grafikspeicher", [st, fest] {
            const ke::Editor& e = ke::editor();
            (fest == 0 ? st->bytesHuelle : st->bytes512) = e.echtLicht.wuerfelBytes;
            st->passt = st->passt && e.echtLicht.wuerfelOhnePlatz == 0;
        }));
        for (std::size_t i = 0; i < 3; ++i) {
            add(tu("Licht1: Punkt " + std::to_string(i), [st, i] {
                if (i < st->punkte.size()) { stufe9c::kameraAuf(st->punkte[i]); }
            }));
            add(pause(0.5));
            add(tu("Licht1: Bild " + std::to_string(i), [st, i, fest, name] {
                if (i >= st->punkte.size()) { return; }
                stufe9c::Bild b;
                if (!stufe9c::bildHolen(b)) { meldeFehler("Licht1: Kamerabild nicht lesbar"); }
                bildSchreiben(b, name + "_p" + std::to_string(i) + (fest == 0 ? "_huelle" : "_512"));
                if (fest == 0) {
                    st->huelle.push_back(std::move(b));
                } else if (i < st->huelle.size()) {
                    st->anteil.push_back(ueberEinemBildpunkt(st->huelle[i], b));
                }
            }));
        }
    }
    add(tu("Licht1: Wuerfel wieder nach der Huelle", [st, name] {
        ke::echtLichtWuerfelFest(0);
        ke::editor().echtLicht.an = false;
        ke::editor().echtLicht.effekte = true;
        std::string z = "Knopftest: Licht1 - " + name + ": Atlas nach der Huelle " + std::to_string(st->bytesHuelle / 1048576) + " MB, alle 512 " +
                        std::to_string(st->bytes512 / 1048576) + " MB; ueber einem Bildpunkt (%):";
        for (const double a : st->anteil) {
            char t[32];
            std::snprintf(t, sizeof(t), " %.4f", a);
            z += t;
        }
        diag::detail(z + (st->passt ? "" : " (alle 512 passen nicht ins Budget)"));
    }));
    add(pruefe("Licht1: " + name + " - Wuerfel nach der Huelle wie lauter 512er (je Punkt hoechstens 0,1 % der Bildpunkte ueber einem Bildpunkt)", [st] {
        bool gut = st->passt && !st->anteil.empty() && st->anteil.size() == std::min<std::size_t>(3, st->punkte.size());
        for (const double a : st->anteil) { gut = gut && a <= 0.1; }
        return gut;
    }));
    return s;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    for (Schritt& x : mitEditorZeichner("Licht1", hangar())) { s.push_back(std::move(x)); }
    if (const char* k = std::getenv("BHED_LICHT1_KARTEN"); k != nullptr && k[0] != '\0') {
        std::istringstream in(k);
        for (std::string x; std::getline(in, x, ';');) {
            if (x.empty()) { continue; }
            for (Schritt& y : mitEditorZeichner("Licht1", groessenVergleich(x))) { s.push_back(std::move(y)); }
        }
    }
    s.push_back(tu("Licht1: neue Karte", [] { ke::neueKarte(); }));
    return s;
}

}  // namespace licht1
