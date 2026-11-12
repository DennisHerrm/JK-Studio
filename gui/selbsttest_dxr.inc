// selbsttest_dxr.inc - DXR (rc612): Raytracing-Schatten im Echtzeit-Licht des Karten-Editors (BHED_KARTE_NUR=dxr).
//
// Was der Lauf prueft, haengt davon ab, was er erwartet (BHED_DXR_ERWARTET, Vorgabe "keiner" = es geht):
//   keiner  Direct3D 12 mit DXR 1.1: Vorgabe sind die Schattenkarten (nichts wird gebaut); umgeschaltet rechnen die Strahlen -
//           der Schatten liegt an derselben Stelle wie mit den Schattenkarten und im gebauten Licht (Kante gemessen: Ort und
//           Breite), nur schaerfer; ein bewegter Pinsel baut nur seinen Eimer neu (schnell, nach der Ruhezeit nachgezogen);
//           alphashadow filtert; die Sonne endet am Himmel; der Menuepunkt schaltet.
//   d3d12   Direct3D 11 laeuft (BHED_D3D12=0): der Menuepunkt ist ausgegraut mit dem Grund, die Einstellung aendert kein
//           Bild (Bildpunkt fuer Bildpunkt wie die Vorgabe).
//   geraet  das Geraet kann kein DXR (gespielt mit BHED_DXR_FEHLER=geraet): ebenso.
//   shader, fehler  ohne DXIL-Shader bzw. der Bau scheitert (BHED_DXR_FEHLER=shader / bau): ebenso.
// Stimmt die Erwartung nicht mit dem Geraet ueberein, ist der Lauf rot - so faellt auf, wenn die Schattenart auf dem
// Testrechner unbemerkt nicht mehr ginge. Mit BHED_LICHT_BILDER die Kamerabilder als .bmp (Fotos nebeneinander:
// Y:\werkzeuge\dxr\nebeneinander.py). Eingebunden in gui/selbsttest_handbuch.inc.
namespace dxrtest {

namespace ke = karteneditor;

// 0 keiner, 1 d3d12, 2 geraet, 3 shader, 4 fehler - wie kartegpu::dxr::Grund
int erwartet() {
    const char* e = std::getenv("BHED_DXR_ERWARTET");
    if (e == nullptr || std::strcmp(e, "keiner") == 0) { return 0; }
    if (std::strcmp(e, "d3d12") == 0) { return 1; }
    if (std::strcmp(e, "geraet") == 0) { return 2; }
    if (std::strcmp(e, "shader") == 0) { return 3; }
    return 4;
}

std::string raumKarte() { return karteTestPfad("dxr_raum.map"); }
std::string sonnenKarte() { return karteTestPfad("dxr_sonne.map"); }

// Ein helles, gleichmaessiges Bild (Mittel 180 von 255; yavin/temple_stone2 der anderen Lichttests hat 44 - darauf ist eine
// Schattenkante kaum zu messen)
const char* const kWand = "textures/hoth/h_basicwall";

std::shared_ptr<karte::Pinsel> quaderIn(karte::Karte& k, const karte::Vektor3f& mitte, const karte::Vektor3f& halb, const std::string& shader) {
    karte::TexturProjektion proj;
    karte::texdefStandard(karte::TexdefArt::Quake, proj);
    auto p = std::make_shared<karte::Pinsel>();
    p->quaderBauen(karte::Kasten(mitte, halb), shader, proj, k.kontext);
    k.welt()->kindEinfuegen(p);
    return p;
}

// Ein geschlossener Raum, ein Punktlicht in der Mitte (0 0 96) und eine niedrige Mauer bei x 32..48 bis z 48: ihre obere
// Kante wirft eine gerade Schattengrenze auf den Boden. Aus dem Licht durch die Kante (48, 48): z = 96 - x, also am Boden
// bei x 96; der Luxel von q3map2 liegt 1 ueber dem Boden (DEFAULT_LIGHTMAP_SAMPLE_OFFSET), dort ist die Grenze x 95.
void raumSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    (void)k.weltHolenOderAnlegen();
    const auto quader = [&k](const karte::Vektor3f& mitte, const karte::Vektor3f& halb) { (void)quaderIn(k, mitte, halb, kWand); };
    quader(karte::Vektor3f(0, 0, -8), karte::Vektor3f(512, 512, 8));
    quader(karte::Vektor3f(0, 0, 520), karte::Vektor3f(512, 512, 8));
    quader(karte::Vektor3f(-520, 0, 256), karte::Vektor3f(8, 528, 272));
    quader(karte::Vektor3f(520, 0, 256), karte::Vektor3f(8, 528, 272));
    quader(karte::Vektor3f(0, -520, 256), karte::Vektor3f(512, 8, 272));
    quader(karte::Vektor3f(0, 520, 256), karte::Vektor3f(512, 8, 272));
    quader(karte::Vektor3f(40, 0, 24), karte::Vektor3f(8, 400, 24));          // die Mauer
    quader(karte::Vektor3f(-200, -160, 96), karte::Vektor3f(24, 24, 96));     // eine Saeule
    quader(karte::Vektor3f(-160, 220, 60), karte::Vektor3f(96, 40, 6));       // eine schwebende Platte unter dem Licht
    auto licht = k.neueEntitaet("light", false);
    licht->setzen("origin", "0 0 96");
    licht->setzen("light", "600");
    k.entitaetEinfuegen(licht);
    auto start = k.neueEntitaet("info_player_start", false);
    start->setzen("origin", "-400 -400 24");
    k.entitaetEinfuegen(start);
    (void)karte::karteSpeichern(raumKarte(), k, false);
}

// Der Himmelsraum der Stufe 9c (Sonne und Himmelslicht aus textures/fur/sky, caulk-Deckel) mit dem hellen Bild, dazu eine
// schwebende Platte und eine Saeule
void sonnenKarteSchreiben() {
    stufe9c::himmelKarteSchreiben();
    std::string t = dateiInhalt(stufe9c::himmelKarte());
    const std::string alt = "yavin/temple_stone2";
    const std::string neu = std::string(kWand).substr(9);
    for (std::size_t p = t.find(alt); p != std::string::npos; p = t.find(alt, p + neu.size())) { t.replace(p, alt.size(), neu); }
    std::ofstream f(bhed::utf8Pfad(sonnenKarte()), std::ios::binary);
    f << t;
}

// Kamera senkrecht ueber (x y) in der Hoehe z
stufe9c::Punkt ueber(float x, float y, float z) { return stufe9c::Punkt{karte::Vektor3f(x, y, z), -89.0F, 0.0F}; }
const stufe9c::Punkt kUebersicht{karte::Vektor3f(-430.0F, -330.0F, 300.0F), -28.0F, 38.0F};
// Ueber der Schattengrenze: aus 106 Hoehe sieht die Kamera den Boden von x 52 bis 190 - die Mauer (bis x 48) bleibt draussen,
// beide Seiten der Grenze (x 95) sind breit im Bild, auch fuer die ueber einen Luxel verlaufene gebackene.
constexpr float kKanteX = 121.0F;
constexpr float kKanteZ = 106.0F;
constexpr float kKanteSchub = 16.0F;   // die zweite Aufnahme steht so viel weiter: daraus der Massstab (Zeilen je Einheit)

// Helligkeit je Zeile, ueber die mittleren 60 % der Spalten gemittelt (das nimmt die Zeichnung der Textur heraus)
std::vector<double> zeilenProfil(const stufe9c::Bild& b) {
    std::vector<double> p(static_cast<std::size_t>(std::max(0, b.hoehe)), 0.0);
    const int x0 = b.breite / 5;
    const int x1 = b.breite - b.breite / 5;
    for (int y = 0; y < b.hoehe && x1 > x0; ++y) {
        double s = 0;
        for (int x = x0; x < x1; ++x) {
            s += stufe9c::helligkeit(&b.rgba[(static_cast<std::size_t>(y) * static_cast<std::size_t>(b.breite) + static_cast<std::size_t>(x)) * 4U]);
        }
        p[static_cast<std::size_t>(y)] = s / static_cast<double>(x1 - x0);
    }
    return p;
}

// Die Schattengrenze im Bild einer Kamera senkrecht ueber der Grenze: die Stelle der groessten Stufe im Zeilenprofil;
// Ort = wo die Haelfte zwischen dunkel und hell erreicht ist, Breite = Abstand der Stellen mit 10 % und 90 % (Zeilen, mit
// Bruchteil). dunkel und hell: die Mittel der Abschnitte 24 bis 40 Zeilen neben der Stufe.
struct Kante {
    bool gueltig = false;
    bool steigt = false;   // die helle Seite liegt bei groesseren Zeilen
    double ort = 0;
    double breite = 0;
    double dunkel = 0;
    double hell = 0;
};

// Die erste Zeile ab `von` in Richtung hell, an der das Profil `ziel` ueberschreitet (mit Bruchteil); < 0: keine
double kreuzung(const std::vector<double>& p, int von, bool steigt, double ziel, int weite) {
    const int h = static_cast<int>(p.size());
    for (int i = 0; i < weite; ++i) {
        const int y0 = steigt ? von + i : von - i;
        const int y1 = steigt ? y0 + 1 : y0 - 1;
        if (y0 < 0 || y1 < 0 || y0 >= h || y1 >= h) { continue; }
        const double v0 = p[static_cast<std::size_t>(y0)];
        const double v1 = p[static_cast<std::size_t>(y1)];
        if (v0 <= ziel && v1 > ziel) { return static_cast<double>(y0) + (steigt ? 1.0 : -1.0) * (ziel - v0) / (v1 - v0); }
    }
    return -1.0;
}

Kante kanteMessen(const stufe9c::Bild& b) {
    Kante k;
    if (b.breite < 64 || b.hoehe < 128) { return k; }
    const std::vector<double> p = zeilenProfil(b);
    const int w = 8;
    int ort = -1;
    double stufe = 0;
    for (int y = w; y + w < b.hoehe; ++y) {
        const double d = std::fabs(p[static_cast<std::size_t>(y + w)] - p[static_cast<std::size_t>(y - w)]);
        if (d > stufe) {
            stufe = d;
            ort = y;
        }
    }
    if (ort < 0 || stufe < 8.0) { return k; }
    const auto mittel = [&p, &b](int von, int bis) {
        double s = 0;
        int n = 0;
        for (int y = std::max(0, von); y < std::min(b.hoehe, bis); ++y) {
            s += p[static_cast<std::size_t>(y)];
            ++n;
        }
        return n > 0 ? s / n : 0.0;
    };
    const double a = mittel(ort - 40, ort - 24);
    const double z = mittel(ort + 24, ort + 40);
    k.dunkel = std::min(a, z);
    k.hell = std::max(a, z);
    if (k.hell - k.dunkel < 8.0) { return k; }
    k.steigt = z > a;
    const int von = k.steigt ? ort - 40 : ort + 40;
    const double s10 = kreuzung(p, von, k.steigt, k.dunkel + (k.hell - k.dunkel) * 0.1, 80);
    const double s50 = kreuzung(p, von, k.steigt, k.dunkel + (k.hell - k.dunkel) * 0.5, 80);
    const double s90 = kreuzung(p, von, k.steigt, k.dunkel + (k.hell - k.dunkel) * 0.9, 80);
    if (s10 < 0 || s50 < 0 || s90 < 0) { return k; }
    k.gueltig = true;
    k.ort = s50;
    k.breite = std::fabs(s90 - s10);
    return k;
}

// Die gebackene Grenze verlaeuft ueber einen Luxel (16 Einheiten) - ihre Flanken reichen weiter, als kanteMessen ansieht.
// Ihre Mitte: wo das gebackene Profil, von der dunklen Seite her, die halbe Helligkeit der Strahlen-Kante erreicht (dunkel
// und hell der scharfen Kante `r`). < 0: nicht gefunden.
double halbStelle(const stufe9c::Bild& g, const Kante& r) {
    if (!r.gueltig) { return -1.0; }
    const std::vector<double> p = zeilenProfil(g);
    return kreuzung(p, r.steigt ? 0 : g.hoehe - 1, r.steigt, r.dunkel + (r.hell - r.dunkel) * 0.5, g.hoehe);
}

void bildSchreiben(const stufe9c::Bild& b, const std::string& name) {
    const std::string o = stufe9c::bilderOrdner();
    if (!o.empty()) { stufe9c::bmpSchreiben(o + "\\dxr_" + name + ".bmp", b); }
}

// Mittlere Helligkeit des ganzen Bilds bzw. seiner Mitte (ein Achtel der Breite und Hoehe um den Mittelpunkt)
double mittelHell(const stufe9c::Bild& b, bool nurMitte = false) {
    const int x0 = nurMitte ? b.breite / 2 - b.breite / 16 : 0;
    const int x1 = nurMitte ? b.breite / 2 + b.breite / 16 : b.breite;
    const int y0 = nurMitte ? b.hoehe / 2 - b.hoehe / 16 : 0;
    const int y1 = nurMitte ? b.hoehe / 2 + b.hoehe / 16 : b.hoehe;
    double s = 0;
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            s += stufe9c::helligkeit(&b.rgba[(static_cast<std::size_t>(y) * static_cast<std::size_t>(b.breite) + static_cast<std::size_t>(x)) * 4U]);
        }
    }
    const double n = static_cast<double>(std::max(0, x1 - x0)) * static_cast<double>(std::max(0, y1 - y0));
    return n > 0 ? s / n : 0.0;
}

// Bildpunkte, die sich unterscheiden
std::size_t verschieden(const stufe9c::Bild& a, const stufe9c::Bild& b) {
    if (a.breite != b.breite || a.hoehe != b.hoehe) { return static_cast<std::size_t>(-1); }
    std::size_t n = 0;
    for (std::size_t i = 0; i + 3 < a.rgba.size(); i += 4) {
        n += (a.rgba[i] != b.rgba[i] || a.rgba[i + 1] != b.rgba[i + 1] || a.rgba[i + 2] != b.rgba[i + 2]) ? 1U : 0U;
    }
    return n;
}

struct Lauf {
    stufe9c::Bild kKante, rKante, rKante2, gKante;   // Schattenkarten, Strahlen (und 16 weiter), gebacken: ueber der Schattengrenze
    stufe9c::Bild kWeit, rWeit, gWeit;               // dieselben in der Uebersicht
    stufe9c::Bild kSonne, rSonne, gSonne;
    ke::StrahlenStand vor;                           // vor dem Umschalten
    ke::StrahlenStand vorPinsel;
    std::size_t wuerfelVor = 0;
    std::size_t wuerfelBytesVor = 0;
    std::shared_ptr<karte::Pinsel> pinsel;
    double hellFrei = 0;                             // Stelle im Schatten der Saeule: ohne, mit, wieder ohne
    double hellSchatten = 0;
    double hellWieder = 0;
    std::string gitter;
    double bodenFrei = 0;                            // Boden unter dem Licht: frei, unter dem Gitter mit Strahlen und mit Karten
    double bodenGitterR = 0;
    double bodenGitterK = 0;
    double bodenGitterG = 0;                         // ... und gebacken (q3map2 -light)
    std::size_t filterTeile = 0;
    std::shared_ptr<karte::Pinsel> gitterPinsel;
    // B3: dasselbe Gitter mit dem Shader von Jedi Academy (q3map_alphashadow) - Strahlen, Schattenkarten, Filter-Teile
    double q3R = 0, q3K = 0;
    std::size_t q3Filter = 0;
    // Zeitpunkte der Aenderungen am Pinsel (eingesetzt, dreimal gezogen): ob ein Bau "schnell" sein muss, haengt am Abstand
    std::array<std::chrono::steady_clock::time_point, 4> pinselZeit{};
};

// Abstand zweier Aenderungen am Pinsel in Sekunden
double abstand(const Lauf& l, int i) { return std::chrono::duration<double>(l.pinselZeit[static_cast<std::size_t>(i)] - l.pinselZeit[static_cast<std::size_t>(i - 1)]).count(); }

void strahlenSetzen(bool an) {
    ke::editor().echtLicht.schattenStrahlen = an;
    ke::neuZeichnen();
}

Schritt bildNehmen(const std::string& was, stufe9c::Bild Lauf::* ziel, const std::shared_ptr<Lauf>& lauf, const std::string& datei) {
    return tu("DXR: Bild " + was, [ziel, lauf, datei] {
        stufe9c::Bild b;
        if (!stufe9c::bildHolen(b)) { meldeFehler("DXR: Kamerabild nicht lesbar (" + datei + ")"); }
        bildSchreiben(b, datei);
        (*lauf).*ziel = std::move(b);
    });
}

// Das Bild ist gerechnet: mit Strahlen sofort, mit Karten, wenn alle Wuerfel stehen
Schritt gerechnet(const std::string& was) {
    return stufe9c::langeWarten("DXR: gerechnet (" + was + ")", [] {
        const ke::Editor& e = ke::editor();
        return e.echtLicht.gezeichnet > 0 && e.echtLicht.wuerfelAusstehend == 0 && ke::kameraBilderFertig();
    }, 20000);
}

// Nach dem Zurueckschalten: das erste Bild rechnet wieder mit Schattenkarten (die Wuerfel sind verteilt) - erst danach sagt
// gerechnet() etwas (vorher waeren 0 Wuerfel auch 0 ausstehende)
Schritt wiederKarten(const std::string& was) {
    return warteBis("DXR: wieder Schattenkarten (" + was + ")", [] {
        const ke::Editor& e = ke::editor();
        return !e.echtLicht.strahlenAktiv && e.echtLicht.wuerfel > 0;
    }, 600);
}

std::vector<Schritt> backen(const std::string& was) {
    std::vector<Schritt> s;
    s.push_back(tu("DXR: Schnell-Backen (" + was + ")", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
    }));
    s.push_back(stufe9c::langeWarten("DXR: gebacken (" + was + ")", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("DXR: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 60000));
    s.push_back(warteBis("DXR: Texturen (gebacken, " + was + ")", [] { return ke::kameraBilderFertig(); }, 20000));
    s.push_back(pause(0.5));
    return s;
}

Schritt backenAus() {
    return tu("DXR: Backen aus", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.pauseSekunden = 2.0;
    });
}

// Das Gitter fuer alphashadow ist ein eigener Shader im Import-Ordner (nie im Spielordner): Die Gitter von Jedi Academy
// schreiben "q3map_alphashadow" (53-mal in den Spieldaten, aus Ravens q3map) - q3map2 kennt nur "surfaceparm alphashadow"
// (games.cpp:150 und 811) und uebergeht das Wort; als "trans" ohne alphashadow werfen sie in -light keinen Schatten
// (light_trace.cpp:839-846), im Echtzeit-Licht ebenso. Die beiden Shader der Spieldaten mit "surfaceparm alphashadow"
// (textures/fur/pink_fur, textures/jnegretetest/pink_fur) sind Fell ohne Alpha-Test. Das Bild ist das Gitter von Kejim
// (textures/kejim/grate02_broke, mit Loechern im Alpha).
const char* const kGitter = "textures/bhed_dxr/gitter";
const char* const kGitterJka = "textures/kejim/grate02_broke";   // B3: dasselbe Bild mit q3map_alphashadow (imperial.shader)
std::string gitterShaderPfad() { return platform::settingsDirectory() + "\\import\\shaders\\bhed_dxr_test.shader"; }

void gitterShaderSchreiben() {
    std::error_code ec;
    std::filesystem::create_directories(bhed::utf8Pfad(platform::settingsDirectory() + "\\import\\shaders"), ec);
    std::ofstream f(bhed::utf8Pfad(gitterShaderPfad()), std::ios::binary | std::ios::trunc);
    f << "// DXR, Selbsttest: ein Gitter mit alphashadow\n"
         "textures/bhed_dxr/gitter\n{\n\tqer_editorimage textures/kejim/grate02_broke\n\tsurfaceparm nonopaque\n\tsurfaceparm trans\n"
         "\tsurfaceparm nonsolid\n\tsurfaceparm alphashadow\n\t{\n\t\tmap textures/kejim/grate02_broke\n\t\talphaFunc LT128\n"
         "\t\tdepthWrite\n\t\trgbGen identity\n\t}\n\t{\n\t\tmap $lightmap\n\t\tblendFunc GL_DST_COLOR GL_ZERO\n\t\tdepthFunc equal\n\t}\n}\n";
}

// Der Testshader ist geladen, q3map2 liest alphashadow daraus, und das Spiel hat ein Bild mit Alpha-Test dafuer
std::string gitterShader() {
    const ke::Editor& e = ke::editor();
    if (!e.karteSpiel) { return std::string(); }
    const auto it = e.karteSpiel->q3map2.eintraege.find(kGitter);
    if (it == e.karteSpiel->q3map2.eintraege.end()) { return std::string(); }
    const ke::SpielDarstellung d = ke::spielDarstellung(kGitter);
    return karte::shaderSkriptLesen(it->second.text, kGitter).alphaSchatten && d.alphaFunktion != 0 && !d.bild.empty() && !d.unsichtbar ? kGitter
                                                                                                                                       : std::string();
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto lauf = std::make_shared<Lauf>();
    const int erw = erwartet();
    if (erw == 0) {
        add(tu("DXR: Testshader (Gitter mit alphashadow) in den Import-Ordner, Suchpfad neu", [] {
            gitterShaderSchreiben();
            ke::editor().spielKennung.clear();
        }));
        add(warteBis("DXR: Spieldaten neu geladen", [] {
            ke::spielPruefen();
            return ke::spielBereit() && !ke::editor().spielKennung.empty();
        }, 20000));
    }
    add(tu("DXR: Testraum (Punktlicht, Mauer, Saeule, Platte), Echtzeit-Licht mit der Vorgabe", [] {
        karteDialogeZu();
        raumSchreiben();
        ke::Editor& e = ke::editor();
        e.echtLicht.an = false;
        e.echtLicht.backen = false;
        e.echtLicht.effekte = false;
        (void)ke::karteOeffnen(raumKarte());
        e.kameraE.modus = karte::KameraModus::Textur;
        stufe9c::kameraAuf(ueber(kKanteX, 0.0F, kKanteZ));
        e.echtLicht.an = true;
    }));
    add(pruefe("DXR: die Vorgabe der Schattenart ist die bisherige (Schattenkarten; Einstellung aus)", [] { return !ke::editor().echtLicht.schattenStrahlen; }));
    add(warteBis("DXR: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(0.5));
    add(gerechnet("Schattenkarten"));
    add(pause(0.3));
    add(bildNehmen("Schattenkarten, ueber der Schattengrenze", &Lauf::kKante, lauf, "kante_karten"));
    add(tu("DXR: Uebersicht", [] { stufe9c::kameraAuf(kUebersicht); }));
    add(pause(0.4));
    add(bildNehmen("Schattenkarten, Uebersicht", &Lauf::kWeit, lauf, "weit_karten"));
    add(pruefe("DXR: mit der Vorgabe wird keine Beschleunigungsstruktur gebaut und nichts mit Strahlen gezeichnet", [lauf] {
        const ke::Editor& e = ke::editor();
        lauf->vor = ke::strahlenStand();
        lauf->wuerfelVor = e.echtLicht.wuerfel;
        lauf->wuerfelBytesVor = e.echtLicht.wuerfelBytes;
        diag::detail("Knopftest: DXR - Vorgabe: Grund " + std::to_string(lauf->vor.grund) + ", Strukturen " + std::to_string(lauf->vor.teile) + ", Baue " +
                     std::to_string(lauf->vor.gebaut) + ", gebunden " + std::to_string(lauf->vor.gebunden) + ", Schattenwuerfel " + std::to_string(lauf->wuerfelVor) +
                     " (" + std::to_string(lauf->wuerfelBytesVor / (1024U * 1024U)) + " MB)");
        return !e.echtLicht.strahlenAktiv && lauf->vor.teile == 0 && lauf->vor.gebaut == 0 && lauf->vor.gebunden == 0 && lauf->wuerfelVor > 0;
    }));
    add(pruefe("DXR: das Geraet ist, was der Lauf erwartet (BHED_DXR_ERWARTET)", [erw] {
        const int g = ke::strahlenStand().grund;
        diag::detail("Knopftest: DXR - Grund " + std::to_string(g) + " (erwartet " + std::to_string(erw) + "): " + ke::strahlenGrundText());
        // der gespielte Fehler beim Bau zeigt sich erst beim Umschalten
        return erw == 4 ? (g == 0 || g == 4) : g == erw;
    }));
    // --- umschalten ---
    add(tu("DXR: Schattenart Raytracing waehlen", [] { strahlenSetzen(true); }));
    if (erw == 0) {   // das erste Bild mit Strahlen (unter Leerlauf-Prioritaet kann es dauern); ohne DXR kommt es nie
        add(warteBis("DXR: erstes Bild mit Strahlen", [] { return ke::editor().echtLicht.strahlenAktiv; }, 600));
    }
    add(pause(0.5));
    add(gerechnet("nach dem Umschalten"));
    add(pause(0.3));
    add(bildNehmen("nach dem Umschalten, Uebersicht", &Lauf::rWeit, lauf, erw == 0 ? "weit_strahlen" : "weit_ohne"));
    add(tu("DXR: ueber die Schattengrenze", [] { stufe9c::kameraAuf(ueber(kKanteX, 0.0F, kKanteZ)); }));
    add(pause(0.4));
    add(bildNehmen("nach dem Umschalten, ueber der Schattengrenze", &Lauf::rKante, lauf, erw == 0 ? "kante_strahlen" : "kante_ohne"));
    if (erw != 0) {
        // --- es geht nicht: nichts aendert sich ---
        add(pruefe("DXR: ohne Raytracing rechnet das Echtzeit-Licht weiter mit Schattenkarten (Grund genannt)", [lauf, erw] {
            const ke::Editor& e = ke::editor();
            const ke::StrahlenStand st = ke::strahlenStand();
            diag::detail("Knopftest: DXR - Grund " + std::to_string(st.grund) + ": " + ke::strahlenGrundText() + "; Strukturen " + std::to_string(st.teile) +
                         ", gebunden " + std::to_string(st.gebunden) + ", Schattenwuerfel " + std::to_string(e.echtLicht.wuerfel));
            return st.grund == erw && !ke::strahlenGrundText().empty() && !e.echtLicht.strahlenAktiv && st.teile == 0 && st.gebunden == 0 &&
                   e.echtLicht.wuerfel == lauf->wuerfelVor;
        }));
        add(pruefe("DXR: das Bild ist Bildpunkt fuer Bildpunkt dasselbe wie mit der Vorgabe (Schattengrenze und Uebersicht)", [lauf] {
            const std::size_t a = verschieden(lauf->kKante, lauf->rKante);
            const std::size_t b = verschieden(lauf->kWeit, lauf->rWeit);
            diag::detail("Knopftest: DXR - verschiedene Bildpunkte: Schattengrenze " + std::to_string(a) + ", Uebersicht " + std::to_string(b));
            return a == 0 && b == 0;
        }));
        add(klick(ke::trK(Str::KeLBuild), "###main"));
        add(pause(0.4));
        add(tu("DXR: Maus auf den Menuepunkt (Tooltip mit dem Grund)", [] {
            if (const Element* el = finde(ke::trK(Str::KeStrahlenMenue), "##Menu_", 0, true)) { setzeMaus(el->rect.GetCenter()); }
        }));
        add(pause(0.8));
        add(foto("dxr_menue_grund"));
        add(pruefe("DXR: der Menuepunkt steht im Build-Menue", [] { return finde(ke::trK(Str::KeStrahlenMenue), "##Menu_", 0, true) != nullptr; }));
        add(tu("DXR: Klick auf den ausgegrauten Menuepunkt", [] {
            ke::editor().echtLicht.schattenStrahlen = false;
            if (const Element* el = finde(ke::trK(Str::KeStrahlenMenue), "##Menu_", 0, true)) {
                ImGuiIO& io = ImGui::GetIO();
                io.AddMousePosEvent(el->rect.GetCenter().x, el->rect.GetCenter().y);
                io.AddMouseButtonEvent(0, true);
            }
        }));
        add(tu("DXR: Maustaste los", [] { ImGui::GetIO().AddMouseButtonEvent(0, false); }));
        add(pause(0.3));
        add(pruefe("DXR: ausgegraut - der Klick schaltet nichts", [] { return !ke::editor().echtLicht.schattenStrahlen; }));
        add(tu("DXR: Menue zu, Echtzeit-Licht aus", [] {
            karteDialogeZu();
            ke::editor().echtLicht.schattenStrahlen = false;
            ke::editor().echtLicht.an = false;
            ke::editor().echtLicht.effekte = true;
        }));
        add(pause(0.3));
        return s;
    }
    // --- es geht: Strahlen statt Karten ---
    add(pruefe("DXR: umgeschaltet rechnen Strahlen - Strukturen gebaut, gezeichnet mit DXIL, kein Schattenwuerfel mehr im Grafikspeicher", [lauf] {
        const ke::Editor& e = ke::editor();
        const ke::StrahlenStand st = ke::strahlenStand();
        diag::detail("Knopftest: DXR - Strahlen: Strukturen " + std::to_string(st.teile) + " (" + std::to_string(st.dreiecke) + " Dreiecke), Baue " +
                     std::to_string(st.gebaut) + ", Szenen " + std::to_string(st.szenen) + ", gebunden " + std::to_string(st.gebunden) + ", Grafikspeicher " +
                     std::to_string(st.bytes / 1024U) + " KB (Strukturen " + std::to_string(st.strukturBytes / 1024U) + " KB) gegen Schattenwuerfel " +
                     std::to_string(lauf->wuerfelBytesVor / 1024U) + " KB; Wuerfel jetzt " + std::to_string(e.echtLicht.wuerfel) + " (" +
                     std::to_string(e.echtLicht.wuerfelBytes / 1024U) + " KB), ohne Schatten " + std::to_string(e.echtLicht.ohneSchatten) + ", fehlend " +
                     std::to_string(e.echtLicht.fehlend));
        return e.echtLicht.strahlenAktiv && st.teile > 0 && st.dreiecke > 0 && st.gebaut == st.teile && st.szenen >= 1 && st.gebunden > 0 &&
               e.echtLicht.wuerfel == 0 && e.echtLicht.wuerfelBytes == 0 && e.echtLicht.ohneSchatten == 0 && e.echtLicht.fehlend == 0;
    }));
    add(pruefe("DXR: der Hinweis in der Kamera nennt die Schattenart", [] {
        return ke::echtLichtHinweis().find(ke::trK(Str::KeStrahlenHinweis)) != std::string::npos;
    }));
    add(tu("DXR: dieselbe Grenze von 16 Einheiten weiter (Massstab)", [] { stufe9c::kameraAuf(ueber(kKanteX + kKanteSchub, 0.0F, kKanteZ)); }));
    add(pause(0.4));
    add(bildNehmen("Strahlen, 16 weiter", &Lauf::rKante2, lauf, "kante_strahlen2"));
    add(pruefe("DXR: die Schattengrenze liegt an derselben Stelle wie mit den Schattenkarten und ist schaerfer", [lauf] {
        const Kante k = kanteMessen(lauf->kKante);
        const Kante r = kanteMessen(lauf->rKante);
        const Kante r2 = kanteMessen(lauf->rKante2);
        const double jeEinheit = std::fabs(r2.ort - r.ort) / static_cast<double>(kKanteSchub);   // Zeilen je Einheit, gemessen
        char z[500];
        std::snprintf(z, sizeof(z), "Knopftest: DXR - Schattengrenze (Zeilen): Karten Ort %.2f Breite %.2f (dunkel %.1f hell %.1f)%s, Strahlen Ort %.2f Breite %.2f "
                                    "(dunkel %.1f hell %.1f)%s; 16 Einheiten weiter Ort %.2f%s: %.3f Zeilen je Einheit - Abstand %.2f Einheiten, Breite Karten %.2f, "
                                    "Strahlen %.2f Einheiten; Bild %d x %d",
                      k.ort, k.breite, k.dunkel, k.hell, k.gueltig ? "" : " UNGUELTIG", r.ort, r.breite, r.dunkel, r.hell, r.gueltig ? "" : " UNGUELTIG", r2.ort,
                      r2.gueltig ? "" : " UNGUELTIG", jeEinheit, jeEinheit > 0 ? std::fabs(k.ort - r.ort) / jeEinheit : 0.0, jeEinheit > 0 ? k.breite / jeEinheit : 0.0,
                      jeEinheit > 0 ? r.breite / jeEinheit : 0.0, lauf->rKante.breite, lauf->rKante.hoehe);
        diag::detail(z);
        // Ort: die Schattenkarte hebt die Stelle um 1,5 Texel + 0,5 Einheiten an (wuerfelSicht), der Strahl um 1 Einheit (der
        // Luxel von q3map2) - bei 45 Grad Einfall liegen die Grenzen um diese Differenz auseinander, hoechstens eine Einheit.
        // Breite: der Strahl ist eine Kante (mit der Mehrfachabtastung hoechstens zwei Zeilen), die Schattenkarte mittelt
        // fuenf Proben ueber drei Texel.
        return k.gueltig && r.gueltig && r2.gueltig && jeEinheit > 1.0 && std::fabs(k.ort - r.ort) <= 1.0 * jeEinheit && r.breite <= 2.0 &&
               r.breite * 3.0 < k.breite && std::fabs(k.hell - r.hell) <= 2.0 && std::fabs(k.dunkel - r.dunkel) <= 2.0;
    }));
    add(pruefe("DXR: die Uebersicht stimmt mit den Schattenkarten ueberein (Bloecke 16 x 16)", [lauf] {
        const stufe9c::Abweichung a = stufe9c::vergleichen(lauf->kWeit, lauf->rWeit);
        char z[300];
        std::snprintf(z, sizeof(z), "Knopftest: DXR - Uebersicht Strahlen gegen Schattenkarten: Block-Mittel %.2f, Block-Max %.2f, Pixel-Mittel %.2f, Helligkeit %.2f / %.2f",
                      a.mittel, a.max, a.pixel, mittelHell(lauf->rWeit), mittelHell(lauf->kWeit));
        diag::detail(z);
        // Helligkeit: nur, dass nicht zwei schwarze Bilder verglichen werden (g2: 7,90 - die Uebersicht ist meist Raum im Dunkel)
        return a.gueltig && a.mittel <= 1.0 && mittelHell(lauf->rWeit) > 4.0;
    }));
    // --- gegen das gebaute Licht ---
    add(tu("DXR: ueber die Schattengrenze (zum Backen)", [] { stufe9c::kameraAuf(ueber(kKanteX, 0.0F, kKanteZ)); }));
    for (Schritt& x : backen("Raum")) { add(std::move(x)); }
    add(bildNehmen("gebacken, ueber der Schattengrenze", &Lauf::gKante, lauf, "kante_gebacken"));
    add(tu("DXR: Uebersicht (gebacken)", [] { stufe9c::kameraAuf(kUebersicht); }));
    add(pause(0.5));
    add(bildNehmen("gebacken, Uebersicht", &Lauf::gWeit, lauf, "weit_gebacken"));
    add(backenAus());
    add(pause(0.3));
    add(pruefe("DXR: gegen das gebaute Licht sind die Strahlen nicht schlechter als die Schattenkarten, die Grenze liegt in der gebackenen", [lauf] {
        const stufe9c::Abweichung rk = stufe9c::vergleichen(lauf->rWeit, lauf->gWeit);
        const stufe9c::Abweichung kk = stufe9c::vergleichen(lauf->kWeit, lauf->gWeit);
        const Kante r = kanteMessen(lauf->rKante);
        const Kante r2 = kanteMessen(lauf->rKante2);
        const double jeEinheit = std::fabs(r2.ort - r.ort) / static_cast<double>(kKanteSchub);
        const double g = halbStelle(lauf->gKante, r);
        char z[400];
        std::snprintf(z, sizeof(z), "Knopftest: DXR - gegen gebacken (Uebersicht): Strahlen Block-Mittel %.2f Max %.2f, Schattenkarten Block-Mittel %.2f Max %.2f; "
                                    "Schattengrenze: gebacken halb hell in Zeile %.2f, Strahlen %.2f - Abstand %.2f Einheiten",
                      rk.mittel, rk.max, kk.mittel, kk.max, g, r.ort, jeEinheit > 0 ? std::fabs(g - r.ort) / jeEinheit : 0.0);
        diag::detail(z);
        // Ein Luxel (16 Einheiten) ist hell oder dunkel nach seinem Messpunkt; dazwischen verlaeuft die Lightmap linear. Die Mitte
        // des Verlaufs liegt deshalb hoechstens einen halben Luxel (8 Einheiten) neben der wahren Grenze.
        return rk.gueltig && kk.gueltig && rk.mittel <= kk.mittel + 0.5 && r.gueltig && g >= 0 && jeEinheit > 1.0 && std::fabs(g - r.ort) <= 8.0 * jeEinheit;
    }));
    // --- bewegte Teile ---
    add(tu("DXR: Kamera ueber eine freie Stelle (260 260)", [] { stufe9c::kameraAuf(ueber(260.0F, 260.0F, 80.0F)); }));
    add(pause(0.4));
    add(tu("DXR: Helligkeit frei, dann ein Pinsel zwischen Licht und Stelle (200 200)", [lauf] {
        stufe9c::Bild b;
        (void)stufe9c::bildHolen(b);
        lauf->hellFrei = mittelHell(b, true);
        lauf->vorPinsel = ke::strahlenStand();
        ke::Editor& e = ke::editor();
        lauf->pinsel = quaderIn(e.karte, karte::Vektor3f(200, 200, 64), karte::Vektor3f(16, 16, 64), kWand);
        lauf->pinselZeit[0] = std::chrono::steady_clock::now();
        ke::neuZeichnen();
    }));
    add(warteBis("DXR: der neue Pinsel ist gebaut", [lauf] { return ke::strahlenStand().gebaut > lauf->vorPinsel.gebaut; }, 600));
    add(pause(0.3));
    add(pruefe("DXR: ein neuer Pinsel baut nur seinen Eimer neu, und sein Schatten faellt auf die Stelle", [lauf] {
        stufe9c::Bild b;
        (void)stufe9c::bildHolen(b);
        bildSchreiben(b, "pinsel_schatten");
        lauf->hellSchatten = mittelHell(b, true);
        const ke::StrahlenStand st = ke::strahlenStand();
        char z[300];
        std::snprintf(z, sizeof(z), "Knopftest: DXR - neuer Pinsel: Baue +%zu von %zu Strukturen, Szenen +%zu; Helligkeit der Stelle %.1f -> %.1f", st.gebaut - lauf->vorPinsel.gebaut,
                      st.teile, st.szenen - lauf->vorPinsel.szenen, lauf->hellFrei, lauf->hellSchatten);
        diag::detail(z);
        // die Stelle liegt 380 Einheiten vom Licht (g2: frei 5,4, im Schatten 0,0) - die Schwelle sagt nur "nicht schon dunkel"
        return st.gebaut - lauf->vorPinsel.gebaut >= 1 && st.gebaut - lauf->vorPinsel.gebaut <= 2 && st.gebaut - lauf->vorPinsel.gebaut < st.teile &&
               st.szenen > lauf->vorPinsel.szenen && lauf->hellFrei > 2.0 && lauf->hellSchatten < lauf->hellFrei * 0.2;
    }));
    // der Pinsel wird gezogen: drei Bilder nacheinander je 8 Einheiten weiter
    for (int i = 1; i <= 3; ++i) {
        add(tu("DXR: Pinsel ziehen " + std::to_string(i), [lauf, i] {
            ke::Editor& e = ke::editor();
            if (i == 1) { lauf->vorPinsel = ke::strahlenStand(); }
            (void)e.karte.welt()->kindEntfernen(lauf->pinsel.get());
            lauf->pinsel = quaderIn(e.karte, karte::Vektor3f(200.0F + 8.0F * static_cast<float>(i), 200, 64), karte::Vektor3f(16, 16, 64), kWand);
            lauf->pinselZeit[static_cast<std::size_t>(i)] = std::chrono::steady_clock::now();
            ke::neuZeichnen();
        }));
    }
    add(pause(0.1));
    // "Schnell" baut ein Eimer, der sich binnen 0,5 s nach seinem letzten Bau wieder aendert (kBewegtSekunden, karte_dxr_win32.cpp).
    // Wie viele der drei Zuege das sind, haengt am Bildabstand: unter Last (Leerlauf-Prioritaet, g4) lagen die Schritte ueber
    // 0,5 s auseinander, dann baut nichts schnell. Erwartet wird deshalb, was die gemessenen Abstaende ergeben; Abstaende
    // zwischen 0,4 und 0,6 s zaehlen in beide Richtungen (der Bau folgt dem Schritt erst im naechsten Bild).
    add(pruefe("DXR: ein gezogener Pinsel wird schnell gebaut (bewegter Teil), wenn er sich binnen 0,5 s wieder aendert", [lauf] {
        const ke::StrahlenStand st = ke::strahlenStand();
        std::size_t sicher = 0;
        std::size_t moeglich = 0;
        std::string abst;
        for (int i = 1; i <= 3; ++i) {
            const double a = abstand(*lauf, i);
            abst += (i > 1 ? ", " : "") + std::to_string(a).substr(0, 5);
            sicher += a < 0.4 ? 1U : 0U;
            moeglich += a < 0.6 ? 1U : 0U;
        }
        const std::size_t schnell = st.schnellGebaut - lauf->vorPinsel.schnellGebaut;
        diag::detail("Knopftest: DXR - gezogen: Baue +" + std::to_string(st.gebaut - lauf->vorPinsel.gebaut) + ", davon schnell +" + std::to_string(schnell) +
                     " (erwartet " + std::to_string(sicher) + " bis " + std::to_string(moeglich) + ", Abstaende " + abst + " s), nachgezogen " +
                     std::to_string(st.nachgezogen));
        return st.gebaut - lauf->vorPinsel.gebaut == 3 && schnell >= sicher && schnell <= moeglich;
    }));
    add(pause(1.0));
    // nachgezogen wird der letzte Bau, wenn er schnell war (der dritte Zug)
    add(pruefe("DXR: nach der Ruhezeit ist ein schnell gebauter Eimer einmal auf schnelle Strahlen nachgezogen", [lauf] {
        const ke::StrahlenStand st = ke::strahlenStand();
        const std::size_t nach = st.nachgezogen - lauf->vorPinsel.nachgezogen;
        const double a = abstand(*lauf, 3);
        diag::detail("Knopftest: DXR - Ruhe: nachgezogen +" + std::to_string(nach) + " (letzter Abstand " + std::to_string(a).substr(0, 5) + " s)");
        return a < 0.4 ? nach == 1 : a > 0.6 ? nach == 0 : nach <= 1;
    }));
    add(tu("DXR: Pinsel wieder weg", [lauf] {
        lauf->vorPinsel = ke::strahlenStand();
        (void)ke::editor().karte.welt()->kindEntfernen(lauf->pinsel.get());
        lauf->pinsel.reset();
        ke::neuZeichnen();
    }));
    add(warteBis("DXR: ohne den Pinsel gebaut", [lauf] { return ke::strahlenStand().gebaut > lauf->vorPinsel.gebaut; }, 600));
    add(pause(0.3));
    add(pruefe("DXR: ohne den Pinsel ist die Stelle wieder hell wie vorher", [lauf] {
        stufe9c::Bild b;
        (void)stufe9c::bildHolen(b);
        lauf->hellWieder = mittelHell(b, true);
        diag::detail("Knopftest: DXR - Stelle wieder frei: " + std::to_string(lauf->hellWieder) + " (vorher " + std::to_string(lauf->hellFrei) + ")");
        return std::fabs(lauf->hellWieder - lauf->hellFrei) <= 0.5;
    }));
    // --- alphashadow: ein Gitter waagerecht unter dem Licht, die Kamera darunter auf den Boden ---
    // Die Stelle (-40 150) liegt schraeg unter dem Gitter (der Strahl zum Licht kreuzt z 40 bei y 64 bis 111), weit genug vom
    // Licht, dass der Boden nicht ueberstrahlt (direkt darunter ist er so hell wie das Bild), und links der Mauer (x 32 bis 48),
    // die sonst einen Teil des Blicks beschattet. Die Kamera unter dem Gitter sieht es nicht - sein Bild laedt nur, weil die
    // Schattengeometrie es anfordert.
    add(pruefe("DXR: der Testshader ist geladen (surfaceparm alphashadow, Bild mit Alpha-Test)", [lauf] {
        lauf->gitter = gitterShader();
        return !lauf->gitter.empty();
    }));
    add(tu("DXR: Kamera unter das Gitter (-40 150 30), Blick auf den Boden", [] { stufe9c::kameraAuf(ueber(-40.0F, 150.0F, 30.0F)); }));
    add(pause(0.4));
    add(tu("DXR: Boden frei messen, Gitter einsetzen (z 38 bis 42)", [lauf] {
        stufe9c::Bild b;
        (void)stufe9c::bildHolen(b);
        lauf->bodenFrei = mittelHell(b);
        if (lauf->gitter.empty()) { return; }
        lauf->gitterPinsel = quaderIn(ke::editor().karte, karte::Vektor3f(0, 0, 40), karte::Vektor3f(128, 128, 2), lauf->gitter);
        ke::neuZeichnen();
    }));
    add(warteBis("DXR: Texturen (Gitter)", [] { return ke::kameraBilderFertig(); }, 20000));
    // das Bild des Gitters kommt erst nach dem ersten Bau (p07: dann wird neu gebaut) - auf den Filter-Teil warten statt einer
    // festen Pause (unter Leerlauf-Prioritaet liegen die Bilder weit auseinander); bleibt er aus, ist die folgende Pruefung rot
    add(warteBis("DXR: Filter-Teil des Gitters gebaut", [] { return ke::strahlenStand().filterTeile >= 1; }, 600));
    add(pause(0.5));
    add(tu("DXR: Boden unter dem Gitter (Strahlen)", [lauf] {
        stufe9c::Bild b;
        (void)stufe9c::bildHolen(b);
        bildSchreiben(b, "gitter_strahlen");
        lauf->bodenGitterR = mittelHell(b);
        lauf->filterTeile = ke::strahlenStand().filterTeile;
    }));
    add(tu("DXR: zurueck zu den Schattenkarten", [] { strahlenSetzen(false); }));
    add(wiederKarten("mit Gitter"));
    add(pause(0.5));
    add(gerechnet("Schattenkarten mit Gitter"));
    add(pause(0.3));
    add(tu("DXR: Boden unter dem Gitter (Schattenkarten)", [lauf] {
        stufe9c::Bild b;
        (void)stufe9c::bildHolen(b);
        bildSchreiben(b, "gitter_karten");
        lauf->bodenGitterK = mittelHell(b);
    }));
    for (Schritt& x : backen("Gitter")) { add(std::move(x)); }
    add(tu("DXR: Boden unter dem Gitter (gebacken)", [lauf] {
        stufe9c::Bild b;
        (void)stufe9c::bildHolen(b);
        bildSchreiben(b, "gitter_gebacken");
        lauf->bodenGitterG = mittelHell(b);
    }));
    add(backenAus());
    add(pause(0.3));
    // Massstab ist das gebaute Licht: q3map2 laesst je Gitterflaeche (255 - Alpha) / 255 durch (light_trace.cpp:1351-1356; das
    // Bild hat im Mittel 0,39), der Pinsel hat zwei Gitterflaechen uebereinander und vier Raender, und die Helligkeit ist
    // nicht linear im Licht - eine Zahl "Anteil von frei" laesst sich deshalb nicht vorhersagen, nur gegen -light messen.
    // Ohne Bild (vor p07) war der Boden mit Strahlen schwarz (g3: 0,0 bei frei 52,2) - das Gitter deckte ganz.
    // Gemessen (g6): Strahlen 4,90, Schattenkarten 5,05, gebacken 6,61 - beide Echtzeit-Arten liegen 24 bis 26 % unter der
    // Lightmap (offen, siehe NOTIZEN_dxr_1211.md; die Lightmap mittelt je Luxel ueber das Gittermuster). Die Schranke 35 % laesst
    // diese Abweichung mit Rand zu und trennt sicher von "deckt ganz" (0,0) und "laesst alles durch" (52,2).
    add(pruefe("DXR: alphashadow filtert das Licht wie im gebauten Licht - der Boden unter dem Gitter ist dunkler als frei, aber nicht schwarz", [lauf] {
        char z[300];
        std::snprintf(z, sizeof(z), "Knopftest: DXR - Boden frei %.1f, unter dem Gitter: Strahlen %.2f, Schattenkarten %.2f, gebacken %.2f; Filter-Teile %zu",
                      lauf->bodenFrei, lauf->bodenGitterR, lauf->bodenGitterK, lauf->bodenGitterG, lauf->filterTeile);
        diag::detail(z);
        if (lauf->gitter.empty() || lauf->bodenFrei < 10.0 || lauf->bodenGitterG <= 0.5) { return false; }
        return lauf->filterTeile >= 1 && lauf->bodenGitterR > 0.5 && lauf->bodenGitterR < lauf->bodenFrei * 0.9 &&
               std::fabs(lauf->bodenGitterR - lauf->bodenGitterG) <= 0.35 * lauf->bodenGitterG;
    }));
    // --- B3 (12.11.): q3map_alphashadow wie im Compiler ---
    // Dasselbe Gitter mit dem Shader von Jedi Academy: textures/kejim/grate02_broke (imperial.shader) hat dasselbe Bild und
    // dieselbe erste Stufe (alphaFunc LT128) wie der Testshader, schreibt aber "q3map_alphashadow" statt "surfaceparm
    // alphashadow". q3map2 nimmt jedes q3map_<x>, das es nicht ausdruecklich kennt, als surfaceparm <x> (shaders.cpp:1706-1714)
    // und baeckt das Gitter wie das Testgitter (Probe b3lm.py: Lightmap-Mittel 4,451 / 4,451, deckend 4,081). Das Echtzeit-Licht
    // uebersah das Wort (g6: Boden 52,15 = frei, 0 Filter-Teile) - jetzt filtert es mit Strahlen und mit Schattenkarten wie unter
    // dem Testgitter. Doppelt definiert ist der Shader auch (bespin.shader ohne alphashadow): Editor und Compiler nehmen beide die
    // durchscheinende Fassung (der Compiler macht daraus einen Detail-Pinsel; die Fassung aus bespin.shader waere strukturell).
    add(pruefe("DXR B3: das Gitter von Jedi Academy schreibt q3map_alphashadow statt surfaceparm alphashadow", [] {
        const ke::Editor& e = ke::editor();
        if (!e.karteSpiel) { return false; }
        const auto it = e.karteSpiel->q3map2.eintraege.find(kGitterJka);
        if (it == e.karteSpiel->q3map2.eintraege.end()) { return false; }
        std::string klein = it->second.text;
        for (char& c : klein) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return klein.find("q3map_alphashadow") != std::string::npos && klein.find("surfaceparm alphashadow") == std::string::npos &&
               klein.find("surfaceparm\talphashadow") == std::string::npos;
    }));
    add(tu("DXR B3: das Gitter mit dem Shader von Jedi Academy (Schattenkarten)", [lauf] {
        ke::Editor& e = ke::editor();
        if (lauf->gitterPinsel) { (void)e.karte.welt()->kindEntfernen(lauf->gitterPinsel.get()); }
        lauf->gitterPinsel = quaderIn(e.karte, karte::Vektor3f(0, 0, 40), karte::Vektor3f(128, 128, 2), kGitterJka);
        ke::neuZeichnen();
    }));
    add(warteBis("DXR B3: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(0.5));
    add(gerechnet("B3, Schattenkarten"));
    add(pause(0.3));
    add(tu("DXR B3: Boden (Schattenkarten), dann Strahlen", [lauf] {
        stufe9c::Bild b;
        (void)stufe9c::bildHolen(b);
        bildSchreiben(b, "b3_karten");
        lauf->q3K = mittelHell(b);
        strahlenSetzen(true);
    }));
    // rot (vor B3): das Gitter ist durchscheinend ohne alphashadow, es gibt keinen Filter-Teil - die Wartezeit laeuft ab
    add(warteBis("DXR B3: Filter-Teil des Gitters (Strahlen)", [] { return ke::editor().echtLicht.strahlenAktiv && ke::strahlenStand().filterTeile >= 1; }, 300));
    add(pause(0.5));
    add(tu("DXR B3: Boden (Strahlen), dann zurueck zu den Schattenkarten", [lauf] {
        stufe9c::Bild b;
        (void)stufe9c::bildHolen(b);
        bildSchreiben(b, "b3_strahlen");
        lauf->q3R = mittelHell(b);
        lauf->q3Filter = ke::strahlenStand().filterTeile;
        strahlenSetzen(false);
    }));
    add(wiederKarten("B3"));
    add(pause(0.3));
    // Gleiches Bild, gleiche Lage, gleiche erste Stufe: nur das Wort unterscheidet die Shader. Der Rest kommt aus dem Neubau
    // (die Strukturen werden neu gebaut, die Bildpunkte gleich gerechnet) - Schranke 1,0 von 255.
    add(pruefe("DXR B3: q3map_alphashadow filtert im Echtzeit-Licht wie surfaceparm alphashadow (wie im Compiler)", [lauf] {
        char z[400];
        std::snprintf(z, sizeof(z), "Knopftest: DXR B3 - Boden frei %.2f; surfaceparm alphashadow: Strahlen %.2f, Karten %.2f; q3map_alphashadow: Strahlen %.2f (Filter %zu), "
                                    "Karten %.2f",
                      lauf->bodenFrei, lauf->bodenGitterR, lauf->bodenGitterK, lauf->q3R, lauf->q3Filter, lauf->q3K);
        diag::detail(z);
        return lauf->q3Filter >= 1 && std::fabs(lauf->q3R - lauf->bodenGitterR) <= 1.0 && std::fabs(lauf->q3K - lauf->bodenGitterK) <= 1.0;
    }));
    add(pruefe("DXR: zurueckgeschaltet sind die Strukturen freigegeben und die Schattenwuerfel wieder da", [] {
        const ke::Editor& e = ke::editor();
        const ke::StrahlenStand st = ke::strahlenStand();
        diag::detail("Knopftest: DXR - zurueck: Strukturen " + std::to_string(st.teile) + ", Grafikspeicher " + std::to_string(st.bytes) + ", Schattenwuerfel " +
                     std::to_string(e.echtLicht.wuerfel));
        return !e.echtLicht.strahlenAktiv && st.teile == 0 && st.bytes == 0 && e.echtLicht.wuerfel > 0;
    }));
    // --- Sonne ---
    add(tu("DXR: Himmelsraum (Sonne und Himmelslicht) mit Platte und Saeule, Schattenkarten", [] {
        sonnenKarteSchreiben();
        (void)ke::karteOeffnen(sonnenKarte());
        ke::Editor& e = ke::editor();
        (void)quaderIn(e.karte, karte::Vektor3f(0, 0, 220), karte::Vektor3f(128, 96, 8), kWand);
        (void)quaderIn(e.karte, karte::Vektor3f(-220, 180, 96), karte::Vektor3f(24, 24, 96), kWand);
        e.kameraE.modus = karte::KameraModus::Textur;
        e.echtLicht.an = true;
        stufe9c::kameraAuf(kUebersicht);
    }));
    add(warteBis("DXR: Texturen (Himmelsraum)", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(1.5));   // die Sonnenkarten kommen ueber mehrere Bilder
    add(gerechnet("Sonne, Schattenkarten"));
    add(pause(0.5));
    add(bildNehmen("Sonne, Schattenkarten", &Lauf::kSonne, lauf, "sonne_karten"));
    add(tu("DXR: Sonne mit Strahlen", [] { strahlenSetzen(true); }));
    add(warteBis("DXR: Sonne mit Strahlen gerechnet", [] { return ke::editor().echtLicht.strahlenAktiv && ke::strahlenStand().teile > 0; }, 600));
    add(pause(0.6));
    add(bildNehmen("Sonne, Strahlen", &Lauf::rSonne, lauf, "sonne_strahlen"));
    for (Schritt& x : backen("Himmelsraum")) { add(std::move(x)); }
    add(bildNehmen("Sonne, gebacken", &Lauf::gSonne, lauf, "sonne_gebacken"));
    add(backenAus());
    add(pause(0.3));
    add(pruefe("DXR: Sonne und Himmelslicht - der Strahl endet am Himmel; das Bild stimmt mit den Schattenkarten ueberein und liegt nicht weiter vom gebauten Licht", [lauf] {
        const stufe9c::Abweichung a = stufe9c::vergleichen(lauf->kSonne, lauf->rSonne);
        const stufe9c::Abweichung rg = stufe9c::vergleichen(lauf->rSonne, lauf->gSonne);
        const stufe9c::Abweichung kg = stufe9c::vergleichen(lauf->kSonne, lauf->gSonne);
        const ke::Editor& e = ke::editor();
        char z[400];
        std::snprintf(z, sizeof(z), "Knopftest: DXR - Sonne Strahlen gegen Schattenkarten: Block-Mittel %.2f, Block-Max %.2f, Helligkeit %.2f / %.2f; gegen gebacken (%.2f): "
                                    "Strahlen Block-Mittel %.2f Max %.2f, Schattenkarten %.2f Max %.2f; Lichter %zu, gezeichnet %zu",
                      a.mittel, a.max, mittelHell(lauf->rSonne), mittelHell(lauf->kSonne), mittelHell(lauf->gSonne), rg.mittel, rg.max, kg.mittel, kg.max,
                      e.echtLicht.lichter, e.echtLicht.gezeichnet);
        diag::detail(z);
        return e.echtLicht.strahlenAktiv && a.gueltig && a.mittel <= 1.0 && mittelHell(lauf->rSonne) > 10.0 && rg.gueltig && kg.gueltig && rg.mittel <= kg.mittel + 0.5;
    }));
    // --- der Menuepunkt ---
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.4));
    add(tu("DXR: Maus auf den Menuepunkt", [] {
        if (const Element* el = finde(ke::trK(Str::KeStrahlenMenue), "##Menu_", 0, true)) { setzeMaus(el->rect.GetCenter()); }
    }));
    add(pause(0.8));
    add(foto("dxr_menue"));
    add(pruefe("DXR: der Menuepunkt steht im Build-Menue bei den Schatten-Einstellungen und nennt keinen Hinderungsgrund", [] {
        const Element* el = finde(ke::trK(Str::KeStrahlenMenue), "##Menu_", 0, true);
        const Element* ps = finde(ke::trK(Str::KePatchSchatten), "##Menu_", 0, true);
        return el != nullptr && ps != nullptr && el->rect.Min.y > ps->rect.Min.y && el->rect.Min.y - ps->rect.Max.y < ps->rect.GetHeight() &&
               ke::strahlenGrundText().empty();
    }));
    add(klick(ke::trK(Str::KeStrahlenMenue), "##Menu_", 0, true));
    add(pause(0.4));
    add(pruefe("DXR: ein Klick schaltet die Schattenart um (hier: zurueck zu den Schattenkarten)", [] { return !ke::editor().echtLicht.schattenStrahlen; }));
    add(pruefe("DXR: der Helfer (Aktionssuche) findet den Menuepunkt", [] {
        std::string frage = ke::trK(Str::KeStrahlenMenue);
        const std::size_t klammer = frage.find(" (");
        if (klammer != std::string::npos) { frage.resize(klammer); }
        std::string ids;
        for (const HilfeTreffer& t : hilfeFrage(frage)) {
            ids += " " + t.id;
            if (t.id == "ort:menue:KeLBuild>KeStrahlenMenue") { return true; }
        }
        diag::detail("Knopftest: DXR - Helfer zu \"" + frage + "\":" + ids);
        return false;
    }));
    add(tu("DXR: aufraeumen", [] {
        std::error_code ec;
        std::filesystem::remove(bhed::utf8Pfad(gitterShaderPfad()), ec);
        karteDialogeZu();
        ke::Editor& e = ke::editor();
        e.echtLicht.schattenStrahlen = false;
        e.echtLicht.an = false;
        e.echtLicht.effekte = true;
        ke::einstellungenSpeichern();
        e.karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
    }));
    add(pause(0.3));
    return s;
}

// Die Bilder der Handbuchseite (BHED_HB_NUR=dxr): derselbe Blick mit Schattenkarten und mit Raytracing-Schatten, das
// Build-Menue mit dem Menuepunkt. Ohne DXR auf dem Rechner entstehen die Bilder mit Schattenkarten und dem Grund im Tooltip.
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s = stufe9c::einrichten();
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const auto voll = [] { return ImRect{0.0F, 0.0F, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y}; };
    const auto blick = [] { stufe9c::kameraAuf(stufe9c::Punkt{karte::Vektor3f(150.0F, -150.0F, 110.0F), -38.0F, 118.0F}); };
    add(tu("Handbuch DXR: Testraum, Echtzeit-Licht mit Schattenkarten", [blick] {
        karteDialogeZu();
        ke::setzeNrcBegriffe(false);
        raumSchreiben();
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.schattenStrahlen = false;
        (void)ke::karteOeffnen(raumKarte());
        e.kameraE.modus = karte::KameraModus::Textur;
        blick();
        e.echtLicht.an = true;
    }));
    add(warteBis("Handbuch DXR: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(0.5));
    add(gerechnet("Handbuch, Schattenkarten"));
    add(tu("Handbuch DXR: Maus in die Kamera", [] { setzeMaus(hb::fensterRect("##feldKamera").GetCenter()); }));
    add(pause(0.8));
    add(hb::fotoTeil("h_dxr_karten", voll));
    add(tu("Handbuch DXR: Raytracing-Schatten", [] { strahlenSetzen(true); }));
    add(pause(0.8));
    add(gerechnet("Handbuch, Strahlen"));
    add(pause(0.5));
    add(hb::fotoTeil("h_dxr_strahlen", voll));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.4));
    add(tu("Handbuch DXR: Maus auf den Menuepunkt", [] {
        if (const Element* el = finde(ke::trK(Str::KeStrahlenMenue), "##Menu_", 0, true)) { setzeMaus(el->rect.GetCenter()); }
    }));
    add(pause(0.8));
    add(hb::fotoTeil("h_dxr_menue", voll));
    add(tu("Handbuch DXR: aufraeumen", [] {
        karteDialogeZu();
        ke::Editor& e = ke::editor();
        e.echtLicht.schattenStrahlen = false;
        e.echtLicht.an = false;
        e.karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
    }));
    add(pause(0.3));
    return s;
}

}  // namespace dxrtest
