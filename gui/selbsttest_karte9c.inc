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
std::vector<Schritt> karteMessen(const std::string& eintrag, const std::shared_ptr<std::vector<std::string>>& summen) {
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
    if (liste == nullptr || liste[0] == '\0') { return s; }
    auto summen = std::make_shared<std::vector<std::string>>();
    std::vector<std::string> eintraege;
    std::istringstream in(liste);
    for (std::string x; std::getline(in, x, ';');) {
        if (!x.empty()) { eintraege.push_back(x); }
    }
    for (const std::string& e : eintraege) {
        for (Schritt& x : karteMessen(e, summen)) { s.push_back(std::move(x)); }
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

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    return s;
}

}  // namespace stufe9c
