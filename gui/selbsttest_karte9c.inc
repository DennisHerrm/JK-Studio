// selbsttest_karte9c.inc - Stufe 9c des Karten-Editors: Werkzeugleisten und
// Echtzeit-Licht "wie im Spiel" (Tester-Feedback).
//
// Eingebunden in gui/selbsttest_handbuch.inc (nach Stufe 9).
//
// Lichtmessung (BHED_KARTE_NUR=licht9c, Karten aus BHED_LICHT_KARTEN, durch ';'
// getrennt, je Eintrag "karte.map" oder "karte.map|karte.bsp" - aus der .bsp
// werden die Lichter geschaetzt, weil dekompilierte Karten keine haben): an
// festen Kamerapunkten (die ersten Startpunkte der Karte) das Bild der
// Echtzeit-Vorschau gegen das Bild der echt gebackenen Lightmaps (Schnell-Backen,
// -light -fast, gezeichnet wie "wie im Spiel"). Je Punkt die mittlere und die
// groesste Abweichung der Helligkeit (0..255) ueber Bloecke von 16 x 16
// Bildpunkten. Die Vorschau hat keinen Bounce - die Messung auch nicht.
namespace stufe9c {

namespace ke = karteneditor;

struct Bild {
    std::vector<unsigned char> rgba;
    int breite = 0;
    int hoehe = 0;
};

struct Abweichung {
    bool gueltig = false;
    double mittel = 0;   // Mittel |a - b| der Blockhelligkeiten
    double max = 0;      // groesste Blockabweichung
    double pixel = 0;    // Mittel |a - b| je Bildpunkt
};

double helligkeit(const unsigned char* p) { return 0.299 * p[0] + 0.587 * p[1] + 0.114 * p[2]; }

Abweichung vergleichen(const Bild& a, const Bild& b, int block = 16) {
    Abweichung r;
    if (a.breite != b.breite || a.hoehe != b.hoehe || a.breite < block || a.hoehe < block) { return r; }
    double summe = 0;
    std::size_t bloecke = 0;
    double pixelSumme = 0;
    for (int by = 0; by + block <= a.hoehe; by += block) {
        for (int bx = 0; bx + block <= a.breite; bx += block) {
            double sa = 0;
            double sb = 0;
            for (int y = by; y < by + block; ++y) {
                for (int x = bx; x < bx + block; ++x) {
                    const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(a.breite) + static_cast<std::size_t>(x)) * 4U;
                    const double ha = helligkeit(&a.rgba[i]);
                    const double hb = helligkeit(&b.rgba[i]);
                    sa += ha;
                    sb += hb;
                    pixelSumme += std::fabs(ha - hb);
                }
            }
            const double d = std::fabs(sa - sb) / static_cast<double>(block * block);
            summe += d;
            r.max = std::max(r.max, d);
            ++bloecke;
        }
    }
    r.gueltig = bloecke > 0;
    r.mittel = bloecke > 0 ? summe / static_cast<double>(bloecke) : 0;
    r.pixel = pixelSumme / (static_cast<double>(a.breite) * static_cast<double>(a.hoehe));
    return r;
}

void bmpSchreiben(const std::string& pfad, const Bild& b) {
    std::ofstream f(bhed::utf8Pfad(pfad), std::ios::binary);
    if (!f || b.breite <= 0 || b.hoehe <= 0) { return; }
    const std::uint32_t zeile = (static_cast<std::uint32_t>(b.breite) * 3U + 3U) & ~3U;
    const std::uint32_t groesse = 54U + zeile * static_cast<std::uint32_t>(b.hoehe);
    unsigned char kopf[54] = {'B', 'M'};
    const auto u32 = [&kopf](int at, std::uint32_t v) {
        for (int i = 0; i < 4; ++i) { kopf[at + i] = static_cast<unsigned char>((v >> (8 * i)) & 0xFFU); }
    };
    u32(2, groesse);
    u32(10, 54);
    u32(14, 40);
    u32(18, static_cast<std::uint32_t>(b.breite));
    u32(22, static_cast<std::uint32_t>(b.hoehe));
    kopf[26] = 1;
    kopf[28] = 24;
    f.write(reinterpret_cast<const char*>(kopf), 54);
    std::vector<unsigned char> z(zeile, 0);
    for (int y = b.hoehe - 1; y >= 0; --y) {
        for (int x = 0; x < b.breite; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(b.breite) + static_cast<std::size_t>(x)) * 4U;
            z[static_cast<std::size_t>(x) * 3U + 0U] = b.rgba[i + 2];
            z[static_cast<std::size_t>(x) * 3U + 1U] = b.rgba[i + 1];
            z[static_cast<std::size_t>(x) * 3U + 2U] = b.rgba[i + 0];
        }
        f.write(reinterpret_cast<const char*>(z.data()), static_cast<std::streamsize>(zeile));
    }
}

// warteBis ohne die Grenze von 4000 Bildern je Schritt (Backen grosser Karten)
Schritt langeWarten(const std::string& was, std::function<bool()> bedingung, int maxBilder) {
    Schritt x = warteBis(was, std::move(bedingung), maxBilder);
    x.eigeneZeitgrenze = true;
    return x;
}

std::string bilderOrdner() {
    const char* o = std::getenv("BHED_LICHT_BILDER");
    return o != nullptr ? std::string(o) : std::string();
}

// Ein Kamerapunkt: Ort und Blick (pitch, yaw)
struct Punkt {
    karte::Vektor3f ort{0, 0, 0};
    float pitch = -12.0F;
    float yaw = 0;
};

// Die ersten (hoechstens drei) Startpunkte der Karte, mindestens 256 Einheiten
// auseinander; ohne Startpunkt die Mitte der Welt.
std::vector<Punkt> kameraPunkte() {
    std::vector<Punkt> r;
    for (const auto& e : ke::editor().karte.entitaeten()) {
        const std::string& k = e->klassenname();
        if (k.rfind("info_player", 0) != 0) { continue; }
        float o[3] = {0, 0, 0};
        std::istringstream in(e->wert("origin"));
        if (!(in >> o[0] >> o[1] >> o[2])) { continue; }
        Punkt p;
        p.ort = karte::Vektor3f(o[0], o[1], o[2] + 40.0F);
        p.yaw = static_cast<float>(std::atof(e->wert("angle").c_str()));
        bool nah = false;
        for (const Punkt& q : r) {
            const karte::Vektor3f d = q.ort - p.ort;
            nah = nah || (d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) < 256.0F * 256.0F;
        }
        if (!nah) { r.push_back(p); }
        if (r.size() == 3) { break; }
    }
    // zu wenige Startpunkte: unter den ersten Lichtern (48 tiefer, gedreht)
    for (const auto& e : ke::editor().karte.entitaeten()) {
        if (r.size() >= 3) { break; }
        if (e->klassenname() != "light") { continue; }
        float o[3] = {0, 0, 0};
        std::istringstream in(e->wert("origin"));
        if (!(in >> o[0] >> o[1] >> o[2])) { continue; }
        Punkt p;
        p.ort = karte::Vektor3f(o[0], o[1], o[2] - 48.0F);
        p.yaw = static_cast<float>(r.size()) * 120.0F;
        bool nah = false;
        for (const Punkt& q : r) {
            const karte::Vektor3f d = q.ort - p.ort;
            nah = nah || (d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) < 256.0F * 256.0F;
        }
        if (!nah) { r.push_back(p); }
    }
    if (r.empty()) {
        Punkt p;
        p.ort = karte::Vektor3f(0, 0, 64);
        r.push_back(p);
    }
    return r;
}

void kameraAuf(const Punkt& p) {
    ke::Editor& e = ke::editor();
    e.kamera.ursprung = p.ort;
    e.kamera.winkel = karte::Vektor3f(p.pitch, p.yaw, 0.0F);
}

bool bildHolen(Bild& b) { return ke::kameraBild(b.rgba, b.breite, b.hoehe); }

struct Lauf {
    std::string name;
    std::string karte;
    std::string bsp;
    std::vector<Punkt> punkte;
    std::vector<Bild> echt;
    std::vector<Abweichung> ergebnis;
    double schaetzMs = 0;
};

std::string dateiName(const std::string& pfad) {
    std::string n = bhed::utf8Pfad(pfad).stem().string();
    for (char& c : n) {
        if (std::isalnum(static_cast<unsigned char>(c)) == 0) { c = '_'; }
    }
    return n;
}

// Eine Karte messen: Echtzeit-Bilder an allen Punkten, dann Schnell-Backen, gebackene Bilder.
// grenzeMittel/grenzeMax > 0: der Grenzwert der Abweichung je Punkt (Pruefschritt)
std::vector<Schritt> karteMessen(const std::string& eintrag, const std::shared_ptr<std::vector<std::string>>& summen, double grenzeMittel = 0,
                                 double grenzeMax = 0) {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto lauf = std::make_shared<Lauf>();
    const std::size_t strich = eintrag.find('|');
    lauf->karte = eintrag.substr(0, strich);
    lauf->bsp = strich == std::string::npos ? std::string() : eintrag.substr(strich + 1);
    lauf->name = dateiName(lauf->karte);
    add(tu("Licht 9c: Karte oeffnen " + lauf->karte, [lauf] {
        karteDialogeZu();
        ke::Editor& e = ke::editor();
        e.echtLicht.an = false;
        e.echtLicht.backen = false;
        (void)ke::karteOeffnen(lauf->karte);
        if (!lauf->bsp.empty()) {
            // dekompilierte Karten haben keine Lichter (q3map2 entfernt sie): aus der .bsp schaetzen
            const auto t0 = std::chrono::steady_clock::now();
            const std::string daten = dateiInhalt(lauf->bsp);
            karte::LichtSchaetzOptionen o;
            const std::vector<karte::GeschaetztesLicht> l = karte::lichterSchaetzen(daten, o);
            for (const karte::GeschaetztesLicht& g : l) {
                auto ent = e.karte.neueEntitaet("light", false);
                for (const auto& [k, w] : karte::lichtSchluessel(g)) { ent->setzen(k, w); }
                e.karte.entitaetEinfuegen(ent);
            }
            lauf->schaetzMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
            diag::detail("Knopftest: Licht 9c - " + lauf->name + ": " + std::to_string(l.size()) + " Lichter geschaetzt (" +
                         std::to_string(static_cast<int>(lauf->schaetzMs)) + " ms)");
        }
        lauf->punkte = kameraPunkte();
        e.kameraE.modus = karte::KameraModus::Textur;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        kameraAuf(lauf->punkte.front());
    }));
    add(warteBis("Licht 9c: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(1.0));
    for (std::size_t i = 0; i < 3; ++i) {
        add(tu("Licht 9c: Echtzeit-Punkt " + std::to_string(i), [lauf, i] {
            if (i < lauf->punkte.size()) { kameraAuf(lauf->punkte[i]); }
        }));
        add(pause(1.2));   // Schattenwuerfel: hoechstens einige neu je Bild
        add(tu("Licht 9c: Echtzeit-Bild " + std::to_string(i), [lauf, i] {
            if (i >= lauf->punkte.size()) { return; }
            Bild b;
            if (!bildHolen(b)) { meldeFehler("Licht 9c: Kamerabild nicht lesbar"); }
            const std::string o = bilderOrdner();
            if (!o.empty()) { bmpSchreiben(o + "\\" + lauf->name + "_p" + std::to_string(i) + "_echt.bmp", b); }
            lauf->echt.push_back(std::move(b));
        }));
    }
    add(tu("Licht 9c: Schnell-Backen an", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
    }));
    add(langeWarten("Licht 9c: gebacken", [lauf] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("Licht 9c: Schnell-Backen gescheitert (" + lauf->name + ")");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 400000));
    add(warteBis("Licht 9c: Texturen (gebacken)", [] { return ke::kameraBilderFertig(); }, 20000));
    for (std::size_t i = 0; i < 3; ++i) {
        add(tu("Licht 9c: gebackener Punkt " + std::to_string(i), [lauf, i] {
            if (i < lauf->punkte.size()) { kameraAuf(lauf->punkte[i]); }
        }));
        add(pause(0.5));
        add(tu("Licht 9c: gebackenes Bild " + std::to_string(i), [lauf, i] {
            if (i >= lauf->punkte.size() || i >= lauf->echt.size()) { return; }
            Bild b;
            if (!bildHolen(b)) { meldeFehler("Licht 9c: Kamerabild nicht lesbar"); }
            const std::string o = bilderOrdner();
            if (!o.empty()) { bmpSchreiben(o + "\\" + lauf->name + "_p" + std::to_string(i) + "_gebacken.bmp", b); }
            const Abweichung a = vergleichen(lauf->echt[i], b);
            lauf->ergebnis.push_back(a);
            const Punkt& p = lauf->punkte[i];
            char z[400];
            std::snprintf(z, sizeof(z), "Licht9c: %s P%zu (%.0f %.0f %.0f / %.0f) Block-Mittel %.2f, Block-Max %.2f, Pixel-Mittel %.2f%s", lauf->name.c_str(), i,
                          p.ort[0], p.ort[1], p.ort[2], p.yaw, a.mittel, a.max, a.pixel, a.gueltig ? "" : " (ungueltig)");
            diag::detail(z);
        }));
    }
    add(tu("Licht 9c: Summe " + lauf->name, [lauf, summen] {
        const ke::Editor& e = ke::editor();
        double m = 0;
        double x = 0;
        for (const Abweichung& a : lauf->ergebnis) {
            m += a.mittel;
            x = std::max(x, a.max);
        }
        if (!lauf->ergebnis.empty()) { m /= static_cast<double>(lauf->ergebnis.size()); }
        char z[400];
        std::snprintf(z, sizeof(z), "Licht9c-Summe: %s Punkte %zu, Lichter %zu, Mittel %.2f, Max %.2f, Backen %.1f s", lauf->name.c_str(), lauf->ergebnis.size(),
                      e.echtLicht.lichter, m, x, e.echtLicht.bauSekunden);
        diag::detail(z);
        summen->emplace_back(z);
        ke::editor().echtLicht.backen = false;
        ke::editor().echtLicht.an = false;
        ke::editor().echtLicht.pauseSekunden = 2.0;
    }));
    if (grenzeMittel > 0) {
        char t[200];
        std::snprintf(t, sizeof(t), "Licht 9c: %s - je Punkt Abweichung Mittel <= %.1f und Max <= %.1f (Echtzeit gegen gebacken)", lauf->name.c_str(),
                      grenzeMittel, grenzeMax);
        add(pruefe(t, [lauf, grenzeMittel, grenzeMax] {
            bool gut = !lauf->ergebnis.empty();
            for (const Abweichung& a : lauf->ergebnis) { gut = gut && a.gueltig && a.mittel <= grenzeMittel && a.max <= grenzeMax; }
            return gut;
        }));
    }
    return s;
}

// Bildzeit mit Echtzeit-Licht auf einer Karte (Kamera dreht 240 Bilder)
std::vector<Schritt> bildzeit(const std::string& eintrag) {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const std::string karte = eintrag.substr(0, eintrag.find('|'));
    add(tu("Licht 9c: Bildzeit " + karte, [karte] {
        karteDialogeZu();
        (void)ke::karteOeffnen(karte);
        ke::Editor& e = ke::editor();
        e.kameraE.modus = karte::KameraModus::Textur;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        kameraAuf(kameraPunkte().front());
    }));
    add(warteBis("Licht 9c: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(1.5));
    auto bilder = std::make_shared<std::vector<double>>();
    add({"Licht 9c: 240 Bilder, Kamera dreht", [bilder](int b) {
             ke::Editor& e = ke::editor();
             if (b > 2) { bilder->push_back(static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0); }
             e.kamera.freiBewegen(6, (b / 60) % 2 == 0 ? 1 : -1, e.kameraE);
             return b >= 242;
         }});
    add(tu("Licht 9c: Bildzeit Ergebnis", [bilder, karte] {
        std::vector<double> v = *bilder;
        std::sort(v.begin(), v.end());
        double summe = 0;
        for (const double x : v) { summe += x; }
        const double mittel = v.empty() ? 0 : summe / static_cast<double>(v.size());
        const double p99 = v.empty() ? 0 : v[std::min(v.size() - 1, v.size() * 99 / 100)];
        char z[400];
        std::snprintf(z, sizeof(z), "Licht9c-Bildzeit: %s Lichter %zu, je Bild %zu, Mittel %.2f ms, p99 %.2f ms, Lichtliste %.2f ms",
                      dateiName(karte).c_str(), ke::editor().echtLicht.lichter, ke::editor().echtLicht.gezeichnet, mittel, p99, ke::editor().echtLicht.lichtMs);
        diag::detail(z);
        ke::editor().echtLicht.an = false;
    }));
    return s;
}

// In den Karten-Editor wechseln, bis die Spieldaten bereit sind (wie karteneditorSchritte)
std::vector<Schritt> einrichten() {
    std::vector<Schritt> s;
    s.push_back(tu("Licht 9c: Spielordner, Modus Events", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
    }));
    s.push_back(pause(0.3));
    s.push_back(klick(tr(Str::ViewMapEditor), "###main"));
    s.push_back(pause(0.3));
    s.push_back(warteBis("Licht 9c: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    return s;
}

std::vector<Schritt> messung() {
    std::vector<Schritt> s = einrichten();
    const char* liste = std::getenv("BHED_LICHT_KARTEN");
    auto summen = std::make_shared<std::vector<std::string>>();
    std::vector<std::string> eintraege;
    std::istringstream in(liste != nullptr ? liste : "");
    for (std::string x; std::getline(in, x, ';');) {
        if (x.find_first_not_of(' ') != std::string::npos) { eintraege.push_back(x); }
    }
    for (const std::string& e : eintraege) {
        for (Schritt& x : karteMessen(e, summen, 15.0, 200.0)) { s.push_back(std::move(x)); }
    }
    if (const char* zeit = std::getenv("BHED_LICHT_BILDZEIT"); zeit != nullptr && zeit[0] != '\0') {
        std::istringstream z(zeit);
        for (std::string x; std::getline(z, x, ';');) {
            if (!x.empty()) {
                for (Schritt& y : bildzeit(x)) { s.push_back(std::move(y)); }
            }
        }
    }
    s.push_back(tu("Licht 9c: alle Summen", [summen] {
        for (const std::string& z : *summen) { diag::detail(z); }
        ke::neueKarte();
    }));
    return s;
}

// --- Punkt 1: Werkzeugleisten wie NRCs QToolBars ---------------------------------------
//
// Save/Open/Undo/Redo/Select Touching sind wie in NRC (gtkmisc.cpp:147
// g_toolbarHiddenButtons) ab Werk versteckt und ueber das Kontextmenue der
// Leiste einblendbar; der Griff traegt den Hinweis darauf. Neu: der Griff
// verschiebt eine Leiste an einen anderen Rand, Rand und Reihenfolge bleiben
// ueber einen Neustart (BhedToolbarDock).

ImVec2 griffMitte(const char* id) {
    const auto* r = karteFeld(std::string("Griff:") + id);
    if (r == nullptr) { return ImVec2{0, 0}; }
    return ke::bildpunkt(r->x + r->b / 2, r->y + r->h / 2);
}

// Den Griff einer Leiste an eine Stelle ziehen (Ziel erst beim Start bestimmt)
Schritt griffZiehen(const std::string& was, const char* id, std::function<ImVec2()> ziel) {
    auto von = std::make_shared<ImVec2>();
    auto nach = std::make_shared<ImVec2>();
    return {"Ziehe " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    *von = griffMitte(id);
                    *nach = ziel();
                    setzeMaus(*von);
                    return false;
                }
                if (b == 2) {
                    io.AddMouseButtonEvent(0, true);
                    return false;
                }
                if (b >= 3 && b <= 14) {
                    const float f = static_cast<float>(b - 2) / 12.0F;
                    setzeMaus(ImVec2{von->x + (nach->x - von->x) * f, von->y + (nach->y - von->y) * f});
                    return false;
                }
                if (b == 15) {
                    io.AddMouseButtonEvent(0, false);
                    return false;
                }
                return b >= 20;
            }};
}

// eine Stelle nahe einem Rand des Leistenbereichs (zwischen Menue- und Statuszeile)
ImVec2 randStelle(int rand) {
    const auto* f = karteFeld("Bereich");
    if (f == nullptr) { return ImVec2{0, 0}; }
    const int oben = ke::mass::kMenueH;
    const int unten = f->h - ke::mass::kStatusH;
    const int mx = f->b / 2;
    const int my = (oben + unten) / 2;
    switch (rand) {
    case 0: return ke::bildpunkt(mx, oben + 4);
    case 1: return ke::bildpunkt(4, my);
    case 2: return ke::bildpunkt(f->b - 4, my);
    default: return ke::bildpunkt(mx, unten - 4);
    }
}

void leistenVorgabe() {
    ke::Editor& e = ke::editor();
    e.leistenPlaetze[0] = {karte::LeistenRand::Oben, 0, true, 0, 0};
    e.leistenPlaetze[1] = {karte::LeistenRand::Rechts, 0, true, 0, 0};
    e.leistenPlaetze[2] = {karte::LeistenRand::Rechts, 1, true, 0, 0};
    e.hauptleiste = e.filterleiste = e.pluginleiste = true;
}

bool leisteAm(const char* feld, int rand) {
    const auto* r = karteFeld(feld);
    const auto* f = karteFeld("Bereich");
    if (r == nullptr || f == nullptr) { return false; }
    const int d = ke::mass::leisteH();
    switch (rand) {
    case 0: return r->y == ke::mass::kMenueH && r->h == d && r->b > d;
    case 1: return r->x == 0 && r->b == d && r->h > d;
    case 2: return r->x + r->b == f->b && r->b == d && r->h > d;
    default: return r->y + r->h == f->h - ke::mass::kStatusH && r->h == d && r->b > d;
    }
}

std::vector<Schritt> leisten() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Stufe 9c/1: Leisten wie NRC (oben, rechts, rechts)", [] {
        karteDialogeZu();
        leistenVorgabe();
        ke::einstellungenSpeichern();
    }));
    add(pause(0.3));
    add(pruefe("Stufe 9c/1: Vorgabe wie NRC - Hauptleiste oben, Filterleiste rechts; Open/Save/Undo/Redo/Select Touching versteckt", [] {
        const auto& v = ke::editor().versteckteKnoepfe;
        return leisteAm("Werkzeugleiste", 0) && leisteAm("Filterleiste", 2) && v.count("OpenMap") == 1 && v.count("SaveMap") == 1 &&
               v.count("Undo") == 1 && v.count("Redo") == 1 && v.count("SelectTouching") == 1;
    }));
    add(pruefe("Stufe 9c/1: der Griff sagt, wie man die versteckten Knoepfe einblendet (vier Sprachen)", [] {
        bool alle = true;
        for (int sp = 0; sp < 4; ++sp) {
            const std::string t = i18n::trIn(static_cast<i18n::Language>(sp), Str::KeLeisteGriffTipp);
            alle = alle && t.size() > 40;
        }
        return alle && karteFeld("Griff:haupt") != nullptr && karteFeld("Griff:filter") != nullptr;
    }));
    add(griffZiehen("Hauptleiste am Griff an den linken Rand", "haupt", [] { return randStelle(1); }));
    add(pause(0.3));
    add(pruefe("Stufe 9c/1: Hauptleiste links (senkrecht), die Ansichten rechts daneben, gespeichert", [] {
        const ke::Editor& e = ke::editor();
        const auto it = g_app->settings.karte.find("BhedToolbarDock");
        const auto* xy = karteFeld("XY");
        diag::detail("Knopftest: Stufe 9c/1 - " + (it != g_app->settings.karte.end() ? it->second : std::string("-")));
        return e.leistenPlaetze[0].rand == karte::LeistenRand::Links && leisteAm("Werkzeugleiste", 1) && xy != nullptr &&
               xy->x >= ke::mass::leisteH() && it != g_app->settings.karte.end() && it->second.find("haupt=links") != std::string::npos;
    }));
    add(griffZiehen("Plugin-Leiste am Griff an den unteren Rand", "plugin", [] { return randStelle(3); }));
    add(pause(0.3));
    add(pruefe("Stufe 9c/1: Plugin-Leiste unten (waagerecht), Filterleiste bleibt rechts", [] {
        return ke::editor().leistenPlaetze[2].rand == karte::LeistenRand::Unten && leisteAm("Filterleiste", 2);
    }));
    add(tu("Stufe 9c/1: wie nach einem Neustart - Leisten auf Vorgabe, Einstellungen neu lesen", [] {
        ke::einstellungenSpeichern();
        leistenVorgabe();
        ke::einstellungenLaden();
    }));
    add(pause(0.3));
    add(pruefe("Stufe 9c/1: nach dem Lesen wieder links und unten", [] {
        const ke::Editor& e = ke::editor();
        return e.leistenPlaetze[0].rand == karte::LeistenRand::Links && e.leistenPlaetze[2].rand == karte::LeistenRand::Unten && leisteAm("Werkzeugleiste", 1);
    }));
    add(griffZiehen("Hauptleiste zurueck an den oberen Rand", "haupt", [] { return randStelle(0); }));
    add(pause(0.2));
    add(griffZiehen("Plugin-Leiste zurueck an den rechten Rand (unter die Filterleiste)", "plugin", [] {
        const auto* f = karteFeld("Bereich");
        return f != nullptr ? ke::bildpunkt(f->b - 4, f->h - ke::mass::kStatusH - 40) : ImVec2{0, 0};
    }));
    add(pause(0.3));
    add(pruefe("Stufe 9c/1: zurueck wie NRC (oben, rechts, rechts darunter)", [] {
        const ke::Editor& e = ke::editor();
        return leisteAm("Werkzeugleiste", 0) && leisteAm("Filterleiste", 2) && e.leistenPlaetze[2].rand == karte::LeistenRand::Rechts &&
               e.leistenPlaetze[2].reihenfolge > e.leistenPlaetze[1].reihenfolge;
    }));
    add(tu("Stufe 9c/1: Leisten auf Vorgabe", [] {
        leistenVorgabe();
        ke::einstellungenSpeichern();
    }));
    return s;
}

// Zwei echte Laeufe mit einem Einstellungsordner (BHED_KARTE_NUR=leisten1 / leisten2)
std::vector<Schritt> leistenNeustart(int lauf) {
    std::vector<Schritt> s = einrichten();
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(pause(0.5));
    if (lauf == 1) {
        add(tu("Lauf 1: Leisten auf Vorgabe", [] {
            leistenVorgabe();
            ke::einstellungenSpeichern();
        }));
        add(pause(0.3));
        add(griffZiehen("Lauf 1: Hauptleiste an den linken Rand", "haupt", [] { return randStelle(1); }));
        add(pause(0.2));
        add(griffZiehen("Lauf 1: Filterleiste an den unteren Rand", "filter", [] { return randStelle(3); }));
        add(pause(0.3));
        add(pruefe("Leisten > Neustart Lauf 1: links und unten, gespeichert", [] {
            const ke::Editor& e = ke::editor();
            return e.leistenPlaetze[0].rand == karte::LeistenRand::Links && e.leistenPlaetze[1].rand == karte::LeistenRand::Unten &&
                   leisteAm("Werkzeugleiste", 1) && leisteAm("Filterleiste", 3);
        }));
        return s;
    }
    add(pruefe("Leisten > Neustart Lauf 2: nach dem echten Neustart links und unten wie in Lauf 1", [] {
        const ke::Editor& e = ke::editor();
        const auto it = g_app->settings.karte.find("BhedToolbarDock");
        diag::detail("Knopftest: Stufe 9c/1 Lauf 2 - " + (it != g_app->settings.karte.end() ? it->second : std::string("-")));
        return e.leistenPlaetze[0].rand == karte::LeistenRand::Links && e.leistenPlaetze[1].rand == karte::LeistenRand::Unten &&
               leisteAm("Werkzeugleiste", 1) && leisteAm("Filterleiste", 3);
    }));
    add(tu("Lauf 2: zurueck auf die Vorgabe", [] {
        leistenVorgabe();
        ke::einstellungenSpeichern();
    }));
    return s;
}

// --- Punkt 2: Himmel und Sonne wie q3map2 ---------------------------------------------
//
// Ein Raum mit einem Himmelspinsel als Decke (unten textures/fur/sky mit q3map_sun und
// q3map_surfacelight, sonst caulk - wie Mapper es bauen) und einem caulk-Deckel
// darueber. q3map2 (TraceLine mit testAll) laesst den Strahl zur Sonne am Himmel
// enden: der Boden ist sonnig. Rot vorher (gemeldet vom Tester): der Himmelspinsel
// warf selbst Schatten (Vorher-Messung md_eitd_jedi: Boden schwarz, Block-Mittel 95).
std::string himmelKarte() { return karteTestPfad("s9c_himmel.map"); }

void himmelKarteSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    karte::TexturProjektion proj;
    karte::texdefStandard(karte::TexdefArt::Quake, proj);
    const auto quader = [&](const karte::Vektor3f& mitte, const karte::Vektor3f& halb, const char* shader) {
        auto p = std::make_shared<karte::Pinsel>();
        p->quaderBauen(karte::Kasten(mitte, halb), shader, proj, k.kontext);
        welt->kindEinfuegen(p);
        return p;
    };
    const char* stein = "textures/yavin/temple_stone2";
    quader(karte::Vektor3f(0, 0, -8), karte::Vektor3f(512, 512, 8), stein);
    quader(karte::Vektor3f(-520, 0, 256), karte::Vektor3f(8, 528, 272), stein);
    quader(karte::Vektor3f(520, 0, 256), karte::Vektor3f(8, 528, 272), stein);
    quader(karte::Vektor3f(0, -520, 256), karte::Vektor3f(512, 8, 272), stein);
    quader(karte::Vektor3f(0, 520, 256), karte::Vektor3f(512, 8, 272), stein);
    // Himmelspinsel: alle Seiten caulk, die untere Himmel
    auto himmel = quader(karte::Vektor3f(0, 0, 528), karte::Vektor3f(528, 528, 16), "textures/system/caulk");
    for (const auto& fl : himmel->flaechen()) {
        if (fl->ebene().normal()[2] < -0.5) { fl->setzeShader("textures/fur/sky", k.kontext); }
    }
    quader(karte::Vektor3f(0, 0, 560), karte::Vektor3f(528, 528, 16), "textures/system/caulk");   // Deckel darueber
    // ein Startpunkt im Raum (ohne Entity darin fuellt q3map2 nichts und behaelt keine Flaeche)
    auto start = k.neueEntitaet("info_player_start", false);
    start->setzen("origin", "-400 -400 24");
    k.entitaetEinfuegen(start);
    (void)karte::karteSpeichern(himmelKarte(), k, false);
}

int mittelHelligkeit() {
    std::vector<unsigned char> b;
    int w = 0;
    int h = 0;
    if (!ke::kameraBild(b, w, h) || w < 32 || h < 32) { return -1; }
    long summe = 0;
    int n = 0;
    for (int y = h / 2 - 8; y < h / 2 + 8; ++y) {
        for (int x = w / 2 - 8; x < w / 2 + 8; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4U;
            summe += b[i] + b[i + 1] + b[i + 2];
            ++n;
        }
    }
    return static_cast<int>(summe / n);
}

std::vector<Schritt> himmel() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto werte = std::make_shared<std::array<int, 2>>();
    add(tu("Stufe 9c/2: Raum mit Himmelspinsel (caulk ausser unten) und caulk-Deckel, Echtzeit-Licht", [] {
        karteDialogeZu();
        himmelKarteSchreiben();
        (void)ke::karteOeffnen(himmelKarte());
        ke::Editor& e = ke::editor();
        e.kameraE.modus = karte::KameraModus::Textur;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        e.kamera.ursprung = karte::Vektor3f(0, 0, 300);
        e.kamera.winkel = karte::Vektor3f(-89.0F, 0.0F, 0.0F);
    }));
    add(warteBis("Stufe 9c/2: Texturen", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(1.0));
    add(tu("Stufe 9c/2: Helligkeit des Bodens (Echtzeit)", [werte] { (*werte)[0] = mittelHelligkeit(); }));
    add(foto("karteneditor_s9c_himmel_echt"));
    add(pruefe("Stufe 9c/2: wie im Spiel - caulk unsichtbar, die Sonne des Himmels da", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Stufe 9c/2 - Lichter " + std::to_string(e.echtLicht.lichter) + ", gezeichnet " + std::to_string(e.echtLicht.gezeichnet));
        return ke::spielDarstellung("textures/system/caulk").unsichtbar && e.echtLicht.gezeichnet >= 2;
    }));
    add(tu("Stufe 9c/2: Schnell-Backen", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
    }));
    add(langeWarten("Stufe 9c/2: gebacken", [] { return ke::editor().echtLicht.gebackenZeigen || ke::editor().echtLicht.bauStand == 5; }, 30000));
    add(pause(0.3));
    add(tu("Stufe 9c/2: Helligkeit des Bodens (gebacken)", [werte] { (*werte)[1] = mittelHelligkeit(); }));
    add(foto("karteneditor_s9c_himmel_gebacken"));
    add(pruefe("Stufe 9c/2: der Himmelspinsel wirft keinen Schatten - Boden sonnig wie gebacken (hoechstens 20 % Abweichung)", [werte] {
        const int echt = (*werte)[0];
        const int geb = (*werte)[1];
        diag::detail("Knopftest: Stufe 9c/2 - Boden echt " + std::to_string(echt) + ", gebacken " + std::to_string(geb));
        return geb > 90 && echt > 0 && std::abs(echt - geb) * 100 <= geb * 20;
    }));
    add(tu("Stufe 9c/2: Echtzeit-Licht und Backen aus", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.an = false;
        e.echtLicht.pauseSekunden = 2.0;
    }));
    add(pause(0.3));
    // Grenzwerte der Messung (Bloecke 16 x 16, vom Startpunkt aus): Himmel mit Sonne und
    // Flaechenlicht, derselbe Raum mit Steindecke und einem Punktlicht 300
    auto summen = std::make_shared<std::vector<std::string>>();
    for (Schritt& x : karteMessen(himmelKarte(), summen, 4.0, 16.0)) { add(std::move(x)); }
    add(tu("Stufe 9c/2: Raum mit Steindecke und Punktlicht schreiben", [] {
        std::string t = dateiInhalt(himmelKarte());
        for (std::size_t p = t.find("fur/sky"); p != std::string::npos; p = t.find("fur/sky")) { t.replace(p, 7, "yavin/temple_stone2"); }
        t += "{\n\"classname\" \"light\"\n\"origin\" \"0 0 128\"\n\"light\" \"300\"\n}\n";
        std::ofstream f(bhed::utf8Pfad(karteTestPfad("s9c_punkt.map")), std::ios::binary);
        f << t;
    }));
    for (Schritt& x : karteMessen(karteTestPfad("s9c_punkt.map"), summen, 1.0, 4.0)) { add(std::move(x)); }
    return s;
}

// Durchsichtige Shader wie im Spiel: die erste Stufe der Spiel-Shader (alphaFunc, blendFunc)
// kommt in der Darstellung an; caulk (nodraw) ist unsichtbar.
std::vector<Schritt> durchsichtig() {
    std::vector<Schritt> s;
    auto gitter = std::make_shared<std::string>();
    s.push_back(pruefe("Stufe 9c/2: Spiel-Shader mit alphaFunc und blendFunc werden wie im Spiel gezeichnet (erste Stufe gelesen)", [gitter] {
        const auto& sp = ke::editor().spiel;
        if (!sp) { return false; }
        int alpha = 0;
        int mischen = 0;
        std::string beispiel;
        for (const karte::ShaderEintrag* e : sp->inventar.shader()) {
            if (e->name.rfind("textures/", 0) != 0 || e->istStandard()) { continue; }
            const ke::SpielDarstellung d = ke::spielDarstellung(e->name);
            if (d.alphaFunktion != 0 && !d.bild.empty()) {
                if (alpha == 0) {
                    beispiel += e->name + " (alphaFunc, Bild " + d.bild + ") ";
                    *gitter = e->name;
                }
                ++alpha;
            }
            if (d.mischQuelle != 0) {
                if (mischen == 0) { beispiel += e->name + " (blendFunc)"; }
                ++mischen;
            }
            if (alpha > 0 && mischen > 0) { break; }
        }
        diag::detail("Knopftest: Stufe 9c/2 - " + beispiel);
        return alpha > 0 && mischen > 0 && !ke::spielDarstellung("textures/yavin/temple_stone2").unsichtbar;
    }));
    // zum Ansehen: ein Pinsel mit diesem Shader im Himmelsraum, Echtzeit-Licht an
    s.push_back(tu("Stufe 9c/2: Gitter-Pinsel im Himmelsraum (Foto)", [gitter] {
        (void)ke::karteOeffnen(himmelKarte());
        ke::Editor& e = ke::editor();
        karte::TexturProjektion proj;
        karte::texdefStandard(karte::TexdefArt::Quake, proj);
        auto p = std::make_shared<karte::Pinsel>();
        p->quaderBauen(karte::Kasten(karte::Vektor3f(-300, -300, 96), karte::Vektor3f(4, 96, 96)), *gitter, proj, e.karte.kontext);
        e.karte.welt()->kindEinfuegen(p);
        e.kameraE.modus = karte::KameraModus::Textur;
        e.echtLicht.an = true;
        e.kamera.ursprung = karte::Vektor3f(-420.0F, -300.0F, 100.0F);
        e.kamera.winkel = karte::Vektor3f(0.0F, 0.0F, 0.0F);
    }));
    s.push_back(warteBis("Stufe 9c/2: Texturen (Gitter)", [] { return ke::kameraBilderFertig(); }, 3000));
    s.push_back(pause(1.0));
    s.push_back(foto("karteneditor_s9c_gitter"));
    s.push_back(tu("Stufe 9c/2: Echtzeit-Licht aus, neue Karte", [] {
        ke::editor().echtLicht.an = false;
        ke::neueKarte();
    }));
    return s;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    for (Schritt& x : leisten()) { s.push_back(std::move(x)); }
    for (Schritt& x : himmel()) { s.push_back(std::move(x)); }
    for (Schritt& x : durchsichtig()) { s.push_back(std::move(x)); }
    // Lichtmessung auf Karten nur mit BHED_LICHT_KARTEN (lang)
    if (const char* k = std::getenv("BHED_LICHT_KARTEN"); k != nullptr && k[0] != '\0') {
        for (Schritt& x : messung()) { s.push_back(std::move(x)); }
    }
    return s;
}

}  // namespace stufe9c
