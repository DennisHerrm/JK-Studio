// selbsttest_rd.inc - p_render (BHED_EDITORTEST=karteneditor, BHED_KARTE_NUR=rd): offene Punkte des Echtzeit-Lichts.
//
//   Stufen: fuenf Platten, deren Stufen sich bewegen - tcMod scroll + scale, rgbGen wave, animMap, tcGen environment (je als
//         GL_ONE GL_ONE ueber { $lightmap } { Bild GL_DST_COLOR GL_ZERO }) und eine Grundstufe ohne Lightmap mit tcMod scroll. Zur
//         festen Zeit 1,75 s (spielbildZeitFest) das Echtzeit-Licht gegen "Built map as in the game" an derselben Kamera: je Platte
//         24 Spalten, mittlere Farbe; die Stufen auf der dunklen Grundstufe gleich bis auf 8 % (Spalte fuer Spalte), die Grundstufe
//         (anderes Licht: je Bildpunkt gegen die Vollhelligkeit des Spiels) als Verlauf (Spalten durch ihren Mittelwert).
//   Spiegel: die Glow-Platte von R3 ueber einem Spiegelboden (textures/rd/spiegel, portal, misc_portal_surface): das Verhaeltnis
//         Schein / Gelb der Platte im Spiegel gegen das direkt gesehene (RB_DrawSurfs glueht jede Sicht, auch die des Spiegels).
//   Effekt: ein fx_runner mit einem eigenen Teilchen (Shader mit glow): der Schein um das Teilchen mit gegen ohne Effekte.
// Eingebunden in gui/selbsttest.cpp nach selbsttest_r3.inc (nutzt dessen Raum, Quader und Rechtecke), aufgerufen aus
// selbsttest_handbuch.inc.
namespace rd {

namespace ke = karteneditor;

constexpr const char* kZeitText = "1.75";
constexpr double kZeit = 1.75;

std::string quelle() { return stufe2::laufDatei("rd_quelle"); }
std::string stufenKarte() { return karteTestPfad("rd_stufen.map"); }
std::string spiegelKarte() { return karteTestPfad("rd_spiegel.map"); }
std::string fxKarte() { return karteTestPfad("rd_fx.map"); }

struct Platte {
    const char* name;
    const char* shader;
    float x;
    bool verlauf;   // nur der Verlauf zaehlt (Grundstufe mit anderem Licht)
};
constexpr Platte kPlatten[5] = {{"scroll", "textures/rd/scroll", -320, false},
                                {"welle", "textures/rd/welle", -160, false},
                                {"anim", "textures/rd/anim", 0, false},
                                {"umwelt", "textures/rd/umwelt", 160, false},
                                {"grund", "textures/rd/grund", 320, true}};
constexpr float kPlatteY = 300;   // Vorderseite (y 300 .. 316), z 96 .. 224, je 120 breit
constexpr const char* kSpiegel = "textures/rd/spiegel";
constexpr const char* kFxGlow = "gfx/rd/fxglow";

std::string stufe(const std::string& rumpf) {
    return "\n{\n\tqer_editorimage textures/r3/licht\n\t{\n\t\tmap $lightmap\n\t\trgbGen identity\n\t}\n"
           "\t{\n\t\tmap textures/r3/licht\n\t\tblendFunc GL_DST_COLOR GL_ZERO\n\t\trgbGen identity\n\t}\n\t{\n" +
           rumpf + "\t}\n}\n";
}

void pk3Schreiben() {
    std::vector<Pk3Datei> pk3;
    std::string sh;
    sh += std::string("textures/rd/scroll") + stufe("\t\tmap textures/rd/streifen\n\t\tblendFunc GL_ONE GL_ONE\n\t\ttcMod scroll 0.25 0\n\t\ttcMod scale 2 1\n");
    sh += std::string("textures/rd/welle") + stufe("\t\tmap textures/rd/gelb\n\t\tblendFunc GL_ONE GL_ONE\n\t\trgbGen wave sin 0.5 0.5 0 0.5\n");
    sh += std::string("textures/rd/anim") + stufe("\t\tanimMap 2 textures/rd/rot textures/rd/gruen\n\t\tblendFunc GL_ONE GL_ONE\n");
    sh += std::string("textures/rd/umwelt") + stufe("\t\tmap textures/rd/streifen\n\t\tblendFunc GL_ONE GL_ONE\n\t\ttcGen environment\n");
    sh += "textures/rd/grund\n{\n\tqer_editorimage textures/rd/streifen\n\t{\n\t\tmap textures/rd/streifen\n\t\ttcMod scroll 0.25 0\n\t}\n}\n";
    // ein Spiegel ohne alphaGen portal (die Sicht ganz, darueber nur ein fast schwarzes GL_ONE GL_ONE)
    sh += std::string(kSpiegel) + "\n{\n\tportal\n\t{\n\t\tmap textures/r3/dunkel\n\t\tblendFunc GL_ONE GL_ONE\n\t}\n}\n";
    sh += std::string(kFxGlow) + "\n{\n\t{\n\t\tmap textures/r3/licht_glw\n\t\tblendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA\n\t\tglow\n\t}\n}\n";
    pk3.push_back({"shaders/rd.shader", sh});
    // die Grundstufe mit qer_editorimage: ohne bestimmt der Editor die Texturkoordinaten nach seinem Ersatzbild, q3map2 nach dem
    // Bild der Stufe - dann waeren es zwei verschiedene Flaechen
    // Streifen: 128 breit, links gelb, rechts schwarz (je 64 Einheiten auf der Platte)
    pk3.push_back({"textures/rd/streifen.tga", editorportal::tga32(128, 16, [](int x, int) {
                       return x < 64 ? std::array<unsigned char, 4>{255, 210, 40, 255} : std::array<unsigned char, 4>{0, 0, 0, 255};
                   })});
    pk3.push_back({"textures/rd/gelb.tga", editorportal::tga32(16, 16, [](int, int) { return std::array<unsigned char, 4>{255, 210, 40, 255}; })});
    pk3.push_back({"textures/rd/rot.tga", editorportal::tga32(16, 16, [](int, int) { return std::array<unsigned char, 4>{230, 20, 20, 255}; })});
    pk3.push_back({"textures/rd/gruen.tga", editorportal::tga32(16, 16, [](int, int) { return std::array<unsigned char, 4>{20, 230, 20, 255}; })});
    // die Glow-Shader und Bilder von R3 (textures/r3/licht, _glw, dunkel)
    pk3.push_back({"shaders/r3.shader", std::string(r3::kLicht) +
                                            "\n{\n\tqer_editorimage textures/r3/licht\n\t{\n\t\tmap $lightmap\n\t\trgbGen identity\n\t}\n"
                                            "\t{\n\t\tmap textures/r3/licht\n\t\tblendFunc GL_DST_COLOR GL_ZERO\n\t\trgbGen identity\n\t}\n"
                                            "\t{\n\t\tmap textures/r3/licht_glw\n\t\tblendFunc GL_ONE GL_ONE\n\t\trgbGen identity\n\t\tglow\n\t}\n}\n"});
    pk3.push_back({"textures/r3/licht.tga", editorportal::tga32(16, 16, [](int, int) { return std::array<unsigned char, 4>{40, 40, 40, 255}; })});
    pk3.push_back({"textures/r3/licht_glw.tga", editorportal::tga32(16, 16, [](int, int) { return std::array<unsigned char, 4>{255, 210, 40, 255}; })});
    pk3.push_back({"textures/r3/wand.tga", editorportal::tga32(16, 16, [](int, int) { return std::array<unsigned char, 4>{150, 150, 150, 255}; })});
    pk3.push_back({"textures/r3/dunkel.tga", editorportal::tga32(16, 16, [](int, int) { return std::array<unsigned char, 4>{12, 12, 12, 255}; })});
    // ein Teilchen, das stehen bleibt (keine Geschwindigkeit, lange Lebenszeit)
    pk3.push_back({"effects/rd/glow.efx", std::string("Particle\n{\n\tname\tglow\n\tcount\t1\n\tlife\t100000\n\tsize\n\t{\n\t\tstart\t48\n\t}\n"
                                                      "\tshaders\n\t[\n\t\t") +
                                              kFxGlow + "\n\t]\n}\n"});
    // der Wuerfel von R3 (Vorderseite rot) als model2
    const karte::Md3Paket w = r3::wuerfel();
    if (!w.fehler.empty()) { meldeFehler("RD: Wuerfel nicht gebaut: " + w.fehler); }
    r3::wuerfelPfad() = w.modellPfad;
    for (const karte::Md3Paket::Datei& d : w.dateien) { pk3.push_back({d.pfad, d.inhalt}); }
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(quelle()), ec);
    std::string err;
    if (!writePk3(quelle() + "\\rd.pk3", pk3, &err)) { meldeFehler("RD: .pk3 nicht geschrieben: " + err); }
}

void kameraFest(const karte::Vektor3f& ort, const karte::Vektor3f& winkel) {
    ke::Editor& e = ke::editor();
    e.kamFenster.messBreite = 1920;
    e.kamFenster.messHoehe = 1080;
    e.kameraE.sichtfeld = 90.0F;
    e.kameraE.modus = karte::KameraModus::Textur;
    e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT;
    e.kamera.ursprung = ort;
    e.kamera.winkel = winkel;
}

void bmp(const std::string& name, const stufe9c::Bild& b) { r3::bmp(name, b); }

// --- Stufen ----------------------------------------------------------------------------------------

void stufenKarteSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    r3::raum(k, welt, r3::kDunkel);
    for (const Platte& p : kPlatten) { r3::quader(k, welt, karte::Vektor3f(p.x, kPlatteY + 8, 160), karte::Vektor3f(60, 8, 64), p.shader); }
    welt->setzen("_ambient", "40");
    r3::ent(k, "light", {{"origin", "0 100 300"}, {"light", "600"}});
    r3::ent(k, "info_player_start", {{"origin", "0 -300 24"}});
    (void)karte::karteSpeichern(stufenKarte(), k, false);
}

using Spalten = std::array<std::array<double, 3>, 24>;
using StufenMessung = std::array<Spalten, 5>;

Spalten spalten(const stufe9c::Bild& b, const Platte& p) {
    Spalten s{};
    const r3::Rechteck r = r3::rechteck(karte::Vektor3f(p.x, kPlatteY, 160), karte::Vektor3f(60, 0, 64), b.breite, b.hoehe, 0.08F);
    const int w = r.x1 - r.x0 + 1;
    if (w < 48) { return s; }
    for (int i = 0; i < 24; ++i) {
        const int xa = r.x0 + w * i / 24;
        const int xb = r.x0 + w * (i + 1) / 24;
        long n = 0;
        for (int y = r.y0; y <= r.y1; ++y) {
            for (int x = xa; x < xb; ++x) {
                const std::size_t k = (static_cast<std::size_t>(y) * static_cast<std::size_t>(b.breite) + static_cast<std::size_t>(x)) * 4U;
                for (int c = 0; c < 3; ++c) { s[static_cast<std::size_t>(i)][static_cast<std::size_t>(c)] += b.rgba[k + static_cast<std::size_t>(c)]; }
                ++n;
            }
        }
        for (int c = 0; c < 3; ++c) { s[static_cast<std::size_t>(i)][static_cast<std::size_t>(c)] /= static_cast<double>(std::max(1L, n)); }
    }
    return s;
}

Schritt stufenBild(const std::string& name, std::shared_ptr<StufenMessung> m) {
    return tu("RD Stufen: " + name + " messen", [name, m] {
        stufe9c::Bild b;
        if (!stufe9c::bildHolen(b) || b.breite < 640) {
            meldeFehler("RD Stufen: Kamerabild " + name + " nicht lesbar");
            return;
        }
        bmp("rd_stufen_" + name, b);
        for (std::size_t i = 0; i < 5; ++i) { (*m)[i] = spalten(b, kPlatten[i]); }
        std::string z = "Knopftest: RD Stufen " + name + " - Spalten 0/6/12/18 je Platte (R G B):";
        for (std::size_t i = 0; i < 5; ++i) {
            z += std::string(" ") + kPlatten[i].name;
            for (const std::size_t sp : {0U, 6U, 12U, 18U}) {
                char t[64];
                std::snprintf(t, sizeof(t), " %.0f/%.0f/%.0f", (*m)[i][sp][0], (*m)[i][sp][1], (*m)[i][sp][2]);
                z += t;
            }
            z += ";";
        }
        diag::detail(z);
    });
}

// Abweichung je Platte: Mittel ueber die Spalten von |Echtzeit - Spielbild| (RGB, 0..1); verlauf: beide durch ihren Mittelwert
double abweichung(const Spalten& a, const Spalten& b, bool verlauf) {
    const auto mittel = [](const Spalten& s) {
        double m = 0;
        for (const auto& c : s) { m += (c[0] + c[1] + c[2]) / 3.0; }
        return m / 24.0;
    };
    const double ma = verlauf ? std::max(1.0, mittel(a)) : 255.0;
    const double mb = verlauf ? std::max(1.0, mittel(b)) : 255.0;
    double d = 0;
    for (std::size_t i = 0; i < 24; ++i) {
        for (std::size_t c = 0; c < 3; ++c) { d += std::abs(a[i][c] / ma - b[i][c] / mb); }
    }
    d /= 72.0;
    return verlauf ? d / 2.0 : d;   // Verlauf: Werte um 1 statt 0..1 - halbiert, damit 8 % dasselbe heissen wie bei den Stufen
}

// MR7 P40: das Bild, wie die Vorschau es hochlaedt (ladeMitStufen): die Pyramide aus dem unkomprimierten Bild (R_MipMap,
// TextureSet::Tex::buildMips), jede Stufe durch DXT5 (Upload32, rd-vanilla tr_image.cpp:629-689 unter Jedi Academy / Outcast).
bool vorschauStufen(const std::string& pfad, std::vector<std::vector<unsigned char>>& stufen, std::vector<std::array<int, 2>>& groessen) {
    std::vector<unsigned char> rgba;
    TextureSet::Tex t;
    if (!ke::kartenBildRgba(pfad, rgba, t.width, t.height)) { return false; }
    t.rgba.assign(rgba.begin(), rgba.end());
    t.buildMips();
    std::vector<dxt::Ebene> ebenen{{t.rgba.data(), t.width, t.height}};
    for (TextureSet::Tex::Mip& m : t.mips) { ebenen.push_back({m.rgba.data(), m.width, m.height}); }
    dxt::dxt5Rundlauf(ebenen.data(), ebenen.size());
    stufen.clear();
    groessen.clear();
    for (const dxt::Ebene& e : ebenen) {
        stufen.emplace_back(e.rgba, e.rgba + static_cast<std::size_t>(e.breite) * static_cast<std::size_t>(e.hoehe) * 4U);
        groessen.push_back({e.breite, e.hoehe});
    }
    return true;
}

// MR7 P40: unter den Regeln von Jedi Academy / Jedi Outcast laedt das Spiel jedes Bild einer Stufe ohne noTC als DXT5 hoch - das
// Echtzeit-Licht im Spielbild wie die Vorschau: dieselben Stufen Byte fuer Byte (Stufenbilder "?" und die Shaderbilder, deren
// qer_editorimage es fuer die Grundstufe zeichnet), danach dieselben Platten wie oben gegen "Built map as in the game".
std::vector<Schritt> dxtSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto regelVorher = std::make_shared<int>(0);
    auto echt = std::make_shared<StufenMessung>();
    auto spiel = std::make_shared<StufenMessung>();
    add(tu("RD DXT: Regelwerk OpenJK, Echtzeit-Licht", [regelVorher] {
        *regelVorher = g_app->settings.regelwerk;
        g_app->settings.regelwerk = kRegelwerkOpenJK;
        (void)regelwerkPruefen();
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;   // die gebackene Karte ersetzte sonst die Pinsel (spielGebacken), das Echtzeit-Licht ruhte
        e.echtLicht.spielbild = false;
    }));
    add(warteBis("RD DXT: Texturen (Echtzeit)", [] { return ke::kameraBilderFertig(); }, 20000));
    add(warteBis("RD DXT: Schattenwuerfel", [] { return ke::editor().echtLicht.wuerfelAusstehend == 0 && ke::editor().echtLicht.gezeichnet > 0; }, 6000));
    add(pause(1.0));
    add(stufenBild("echtzeit_ojk", echt));
    // Bild im Lager -> Datei, aus der die Vorschau es laedt (Shader: ihr qer_editorimage, das Bild der Grundstufe)
    const std::array<std::pair<const char*, const char*>, 4> bilder = {{{"?textures/rd/streifen", "textures/rd/streifen"},
                                                                        {"?textures/rd/gelb", "textures/rd/gelb"},
                                                                        {"textures/rd/grund", "textures/rd/streifen"},
                                                                        {"textures/rd/scroll", "textures/r3/licht"}}};
    for (const auto& [bild, datei] : bilder) {
        const std::string b = bild;
        const std::string d = datei;
        add(warteBis("RD DXT: " + b + " geladen", [b] {
            std::vector<std::vector<unsigned char>> st;
            std::vector<std::array<int, 2>> gr;
            ke::texturBilderAbholen();
            return ke::echtBildStufen(b, false, st, gr);
        }, 10000));
        add(pruefe("RD DXT: " + b + " - im Echtzeit-Licht dieselben Mip-Stufen wie die Vorschau (R_MipMap, jede Stufe DXT5)", [b, d] {
            std::vector<std::vector<unsigned char>> echtSt;
            std::vector<std::array<int, 2>> echtGr;
            std::vector<std::vector<unsigned char>> vorSt;
            std::vector<std::array<int, 2>> vorGr;
            if (!ke::echtBildStufen(b, false, echtSt, echtGr) || !vorschauStufen(d, vorSt, vorGr)) {
                diag::detail("Knopftest: RD DXT " + b + " - nicht lesbar");
                return false;
            }
            int gleich = 0;
            std::string erste;
            for (std::size_t i = 0; i < std::min(echtSt.size(), vorSt.size()); ++i) {
                if (echtGr[i] == vorGr[i] && echtSt[i] == vorSt[i]) {
                    ++gleich;
                } else if (erste.empty()) {
                    std::size_t k = 0;
                    while (k < std::min(echtSt[i].size(), vorSt[i].size()) && echtSt[i][k] == vorSt[i][k]) { ++k; }
                    char z[200];
                    std::snprintf(z, sizeof(z), ", erste Abweichung Stufe %zu (%d x %d) Byte %zu: %d statt %d", i, echtGr[i][0], echtGr[i][1], k,
                                  k < echtSt[i].size() ? echtSt[i][k] : -1, k < vorSt[i].size() ? vorSt[i][k] : -1);
                    erste = z;
                }
            }
            diag::detail("Knopftest: RD DXT " + b + " - Stufen Echtzeit " + std::to_string(echtSt.size()) + ", Vorschau " + std::to_string(vorSt.size()) +
                         ", gleich " + std::to_string(gleich) + erste);
            return echtSt.size() == vorSt.size() && gleich == static_cast<int>(vorSt.size());
        }));
    }
    add(tu("RD DXT: Schnell-Backen, Spielbild", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = true;
        e.echtLicht.spielbild = true;
    }));
    add(stufe9c::langeWarten("RD DXT: Schnell-Backen fertig", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("RD DXT: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 400000));
    add(stufe9c::langeWarten("RD DXT: Spielbild da", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    add(pause(1.5));
    add(stufenBild("spielbild_ojk", spiel));
    for (std::size_t i = 0; i < 5; ++i) {
        const Platte& p = kPlatten[i];
        add(pruefe(std::string("RD DXT: ") + p.name + " - unter OpenJK-Regeln Echtzeit-Licht wie das Spielbild (" + (p.verlauf ? "Verlauf" : "Farbe") +
                       " je Spalte bis auf 8 %)",
                   [i, echt, spiel] {
                       const double d = abweichung((*echt)[i], (*spiel)[i], kPlatten[i].verlauf);
                       char z[160];
                       std::snprintf(z, sizeof(z), "Knopftest: RD DXT %s - Abweichung %.4f", kPlatten[i].name, d);
                       diag::detail(z);
                       return d < 0.08;
                   }));
    }
    add(tu("RD DXT: Regelwerk zurueck", [regelVorher] {
        g_app->settings.regelwerk = *regelVorher;
        (void)regelwerkPruefen();
    }));
    return s;
}

std::vector<Schritt> stufenSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto echt = std::make_shared<StufenMessung>();
    auto spiel = std::make_shared<StufenMessung>();
    add(tu("RD: Testdaten (Shader, Bilder, Effekt) als Extra Resource Path", [] {
        karteDialogeZu();
        ke::editor().karte.undo.alsGespeichertMarkieren();
        pk3Schreiben();
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0] = karte::ordnerBereinigt(quelle()); });
    }));
    add(warteBis("RD: Spieldaten mit dem Extra Resource Path", [] {
        ke::spielPruefen();
        const ke::Editor& e = ke::editor();
        return ke::spielBereit() && e.karteSpiel && e.karteSpiel->inventar.shaderFinden("textures/rd/scroll") != nullptr;
    }, 20000));
    add(tu(std::string("RD Stufen: fuenf Platten, Echtzeit-Licht zur Zeit ") + kZeitText + " s", [] {
        stufenKarteSchreiben();
        (void)ke::karteOeffnen(stufenKarte());
        kameraFest(karte::Vektor3f(0.0F, -160.0F, 160.0F), karte::Vektor3f(0.0F, 90.0F, 0.0F));
        ke::Editor& e = ke::editor();
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        ke::spielbildZeitFest(kZeit);
    }));
    add(warteBis("RD Stufen: Texturen (Echtzeit)", [] { return ke::kameraBilderFertig(); }, 20000));
    add(warteBis("RD Stufen: Schattenwuerfel", [] { return ke::editor().echtLicht.wuerfelAusstehend == 0 && ke::editor().echtLicht.gezeichnet > 0; }, 6000));
    add(pause(1.0));
    add(stufenBild("echtzeit", echt));
    add(tu("RD Stufen: Zaehler", [] {
        diag::detail("Knopftest: RD Stufen Echtzeit - weitere Stufen " + std::to_string(ke::editor().echtLicht.weitereStufen));
    }));
    add(tu("RD Stufen: Schnell-Backen, Spielbild (die .pk3 als Spielordner dieser Sitzung)", [] {
        std::vector<std::string> ordner = aktiveSpielordner();
        ordner.push_back(quelle());
        setzeSitzungsSpielordner(std::move(ordner));
        rescanGamePaths();
        ke::Editor& e = ke::editor();
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
        e.echtLicht.spielbild = true;
    }));
    add(stufe9c::langeWarten("RD Stufen: Schnell-Backen fertig", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("RD Stufen: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 400000));
    add(stufe9c::langeWarten("RD Stufen: Spielbild da", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    add(pause(1.5));
    add(stufenBild("spielbild", spiel));
    for (std::size_t i = 0; i < 5; ++i) {
        const Platte& p = kPlatten[i];
        add(pruefe(std::string("RD Stufen: ") + p.name + " (" + p.shader + ") - Echtzeit-Licht wie das Spielbild (" + (p.verlauf ? "Verlauf" : "Farbe") +
                       " je Spalte bis auf 8 %)",
                   [i, echt, spiel] {
                       const double d = abweichung((*echt)[i], (*spiel)[i], kPlatten[i].verlauf);
                       char z[160];
                       std::snprintf(z, sizeof(z), "Knopftest: RD Stufen %s - Abweichung %.4f", kPlatten[i].name, d);
                       diag::detail(z);
                       return d < 0.08;
                   }));
    }
    for (Schritt& x : dxtSchritte()) { add(std::move(x)); }
    add(tu("RD Stufen: aufraeumen", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        e.karte.undo.alsGespeichertMarkieren();
    }));
    return s;
}

// --- Gluehen im Spiegel ----------------------------------------------------------------------------

constexpr float kSpiegelPlatteZ = 192;   // Platte z 128 .. 256, Vorderseite y 300; Spiegelboden oben z 0

void spiegelKarteSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    r3::quader(k, welt, karte::Vektor3f(0, 250, -8), karte::Vektor3f(544, 666, 8), kSpiegel);
    r3::quader(k, welt, karte::Vektor3f(0, 250, 408), karte::Vektor3f(544, 666, 8), r3::kDunkel);
    r3::quader(k, welt, karte::Vektor3f(-536, 250, 200), karte::Vektor3f(8, 650, 200), r3::kDunkel);
    r3::quader(k, welt, karte::Vektor3f(536, 250, 200), karte::Vektor3f(8, 650, 200), r3::kDunkel);
    r3::quader(k, welt, karte::Vektor3f(0, -408, 200), karte::Vektor3f(528, 8, 200), r3::kDunkel);
    r3::quader(k, welt, karte::Vektor3f(0, 908, 200), karte::Vektor3f(528, 8, 200), r3::kDunkel);
    r3::quader(k, welt, karte::Vektor3f(0, kPlatteY + 8, kSpiegelPlatteZ), karte::Vektor3f(96, 8, 64), r3::kLicht);
    welt->setzen("_ambient", "40");
    r3::ent(k, "light", {{"origin", "0 100 300"}, {"light", "600"}});
    r3::ent(k, "misc_portal_surface", {{"origin", "0 250 8"}});
    r3::ent(k, "info_player_start", {{"origin", "0 -300 24"}});
    (void)karte::karteSpeichern(spiegelKarte(), k, false);
}

struct Schein {
    double gelb = 0;
    double schein = 0;
    [[nodiscard]] double verhaeltnis() const { return gelb > 1.0 ? schein / gelb : 0.0; }
};

// wie r3::glowMessen, aber fuer einen beliebigen Kasten (Mitte, halbe Groesse); Ring 6 .. 40 Bildpunkte links, rechts, oben
Schein scheinMessen(const stufe9c::Bild& b, const karte::Vektor3f& mitte, const karte::Vektor3f& halb) {
    Schein g;
    const r3::Rechteck r = r3::rechteck(mitte, halb, b.breite, b.hoehe, 0.0F);
    if (r.x1 <= r.x0 || r.y1 <= r.y0) { return g; }
    const auto px = [&b](int x, int y) {
        const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(b.breite) + static_cast<std::size_t>(x)) * 4U;
        return std::array<int, 3>{b.rgba[i], b.rgba[i + 1], b.rgba[i + 2]};
    };
    long n = 0;
    const int w = r.x1 - r.x0;
    const int h = r.y1 - r.y0;
    for (int y = r.y0 + h / 4; y <= r.y1 - h / 4; ++y) {
        for (int x = r.x0 + w / 4; x <= r.x1 - w / 4; ++x) {
            const auto c = px(x, y);
            g.gelb += (c[0] + c[1]) / 2.0 - c[2];
            ++n;
        }
    }
    if (n > 0) { g.gelb /= static_cast<double>(n); }
    n = 0;
    for (int y = r.y0 - 40; y <= r.y1; ++y) {
        for (int x = r.x0 - 40; x <= r.x1 + 40; ++x) {
            if (x < 0 || y < 0 || x >= b.breite || y >= b.hoehe) { continue; }
            const int dx = x < r.x0 ? r.x0 - x : x > r.x1 ? x - r.x1 : 0;
            const int dy = y < r.y0 ? r.y0 - y : 0;
            const int d = std::max(dx, dy);
            if (d < 6 || d > 40) { continue; }
            const auto c = px(x, y);
            g.schein += (c[0] + c[1]) / 2.0;
            ++n;
        }
    }
    if (n > 0) { g.schein /= static_cast<double>(n); }
    return g;
}

Schritt scheinBild(const std::string& name, const karte::Vektor3f& mitte, const karte::Vektor3f& halb, std::shared_ptr<Schein> g) {
    return tu("RD Spiegel: " + name + " messen", [name, mitte, halb, g] {
        stufe9c::Bild b;
        if (!stufe9c::bildHolen(b) || b.breite < 640) {
            meldeFehler("RD Spiegel: Kamerabild " + name + " nicht lesbar");
            return;
        }
        bmp("rd_spiegel_" + name, b);
        *g = scheinMessen(b, mitte, halb);
        char z[220];
        std::snprintf(z, sizeof(z), "Knopftest: RD Spiegel %s - Gelb %.1f, Schein %.1f, Verhaeltnis %.3f, Spiegel gezeichnet %s", name.c_str(), g->gelb,
                      g->schein, g->verhaeltnis(), ke::editor().echtLicht.spiegel ? "ja" : "nein");
        diag::detail(z);
    });
}

std::vector<Schritt> spiegelSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto direkt = std::make_shared<Schein>();
    auto gespiegelt = std::make_shared<Schein>();
    add(tu("RD Spiegel: Glow-Platte ueber dem Spiegelboden, Echtzeit-Licht, direkt", [] {
        spiegelKarteSchreiben();
        (void)ke::karteOeffnen(spiegelKarte());
        kameraFest(karte::Vektor3f(0.0F, -60.0F, kSpiegelPlatteZ), karte::Vektor3f(0.0F, 90.0F, 0.0F));
        ke::Editor& e = ke::editor();
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
    }));
    add(warteBis("RD Spiegel: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(warteBis("RD Spiegel: Schattenwuerfel", [] { return ke::editor().echtLicht.wuerfelAusstehend == 0 && ke::editor().echtLicht.gezeichnet > 0; }, 6000));
    add(pause(1.0));
    add(scheinBild("direkt", karte::Vektor3f(0, kPlatteY + 8, kSpiegelPlatteZ), karte::Vektor3f(96, 8, 64), direkt));   // der ganze Brush
    // das Bild der Platte im Spiegel (z an der Ebene z = 0 gespiegelt), Blick 60 Grad nach unten
    add(tu("RD Spiegel: Blick in den Spiegel", [] {
        kameraFest(karte::Vektor3f(0.0F, 100.0F, 150.0F), karte::Vektor3f(-60.0F, 90.0F, 0.0F));
    }));
    add(pause(1.0));
    add(scheinBild("gespiegelt", karte::Vektor3f(0, kPlatteY + 8, -kSpiegelPlatteZ), karte::Vektor3f(96, 8, 64), gespiegelt));
    add(pruefe("RD Spiegel: die Platte glueht auch im Spiegel (Schein / Gelb im Spiegel mindestens 60 % des direkten, RB_DrawSurfs je Sicht)", [direkt, gespiegelt] {
        const double vd = direkt->verhaeltnis();
        const double vs = gespiegelt->verhaeltnis();
        char z[160];
        std::snprintf(z, sizeof(z), "Knopftest: RD Spiegel - Verhaeltnis gespiegelt / direkt %.3f", vd > 0 ? vs / vd : 0.0);
        diag::detail(z);
        return ke::editor().echtLicht.spiegel && direkt->gelb > 60.0 && gespiegelt->gelb > 10.0 && vd > 0 && vs / vd > 0.6;
    }));
    return s;
}

// --- Gluehen am Effekt -----------------------------------------------------------------------------

constexpr float kFxY = 300;
constexpr float kFxZ = 160;

void fxKarteSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    r3::raum(k, welt, r3::kDunkel);
    welt->setzen("_ambient", "40");
    r3::ent(k, "light", {{"origin", "0 100 300"}, {"light", "600"}});
    char o[64];
    std::snprintf(o, sizeof(o), "0 %g %g", static_cast<double>(kFxY), static_cast<double>(kFxZ));
    r3::ent(k, "fx_runner", {{"origin", o}, {"fxFile", "rd/glow"}, {"delay", "100000"}});
    r3::ent(k, "info_player_start", {{"origin", "0 -300 24"}});
    (void)karte::karteSpeichern(fxKarte(), k, false);
}

// das Teilchen ist ein Quadrat mit der halben Seite "size" (48) in der Ebene y = kFxY: Gelb innen, Schein 6 .. 40 Bildpunkte daneben
Schein fxMessen(const stufe9c::Bild& b) { return scheinMessen(b, karte::Vektor3f(0, kFxY, kFxZ), karte::Vektor3f(48, 0, 48)); }

Schritt fxBild(const std::string& name, std::shared_ptr<Schein> g) {
    return tu("RD Effekt: " + name + " messen", [name, g] {
        stufe9c::Bild b;
        if (!stufe9c::bildHolen(b) || b.breite < 640) {
            meldeFehler("RD Effekt: Kamerabild " + name + " nicht lesbar");
            return;
        }
        bmp("rd_fx_" + name, b);
        *g = fxMessen(b);
        char z[220];
        std::snprintf(z, sizeof(z), "Knopftest: RD Effekt %s - Gelb der Mitte %.1f, Schein im Ring %.1f, Effektecken %zu", name.c_str(), g->gelb, g->schein,
                      ke::editor().echtLicht.effektEcken);
        diag::detail(z);
    });
}

std::vector<Schritt> fxSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto mit = std::make_shared<Schein>();
    auto ohne = std::make_shared<Schein>();
    add(tu("RD Effekt: fx_runner mit leuchtendem Teilchen, Echtzeit-Licht mit Effekten", [] {
        fxKarteSchreiben();
        (void)ke::karteOeffnen(fxKarte());
        kameraFest(karte::Vektor3f(0.0F, -60.0F, kFxZ), karte::Vektor3f(0.0F, 90.0F, 0.0F));
        ke::Editor& e = ke::editor();
        e.echtLicht.an = true;
        e.echtLicht.effekte = true;
    }));
    add(warteBis("RD Effekt: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(warteBis("RD Effekt: Schattenwuerfel", [] { return ke::editor().echtLicht.wuerfelAusstehend == 0 && ke::editor().echtLicht.gezeichnet > 0; }, 6000));
    add(warteBis("RD Effekt: das Teilchen ist da", [] { return ke::editor().echtLicht.effektEcken > 0; }, 6000));
    add(pause(1.0));
    add(fxBild("mit", mit));
    add(tu("RD Effekt: Effekte aus", [] { ke::editor().echtLicht.effekte = false; }));
    add(pause(1.0));
    add(fxBild("ohne", ohne));
    add(pruefe("RD Effekt: das Teilchen mit glow glueht (Schein im Ring mit Effekt mindestens 4 heller als ohne)", [mit, ohne] {
        diag::detail("Knopftest: RD Effekt - Gluehaufrufe " + std::to_string(ke::editor().echtLicht.gluehAufrufe));
        return mit->gelb > 60.0 && mit->schein - ohne->schein > 4.0;
    }));
    add(tu("RD: aufraeumen", [] {
        ke::spielbildZeitFest(-1.0);
        ke::Editor& e = ke::editor();
        e.echtLicht.an = false;
        e.echtLicht.effekte = true;
        e.kamFenster.messBreite = 0;
        e.kamFenster.messHoehe = 0;
        e.filterMaske = 0;
        e.karte.undo.alsGespeichertMarkieren();
        std::vector<std::string> ordner = aktiveSpielordner();
        if (!ordner.empty() && ordner.back() == quelle()) { ordner.pop_back(); }
        setzeSitzungsSpielordner(std::move(ordner));
        rescanGamePaths();
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0].clear(); });
    }));
    add(warteBis("RD: Spieldaten ohne den Extra Resource Path", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    return s;
}

// --- CJK-Umbruch im Rollout-Hinweis -----------------------------------------------------------------
//
// Der Hinweis des IK-Rollouts (Modell-Block 4, MbIkGliedWaehlen) als Meldung im Befehlsfeld des Model-Tabs (mtrollouts, umbrochen
// am Rand). Gemessen: wie voll die Zeilen ausser der letzten sind - nach der Regel von ImGui (ImFont::CalcWordWrapPosition) und
// nach dem gezeichneten Umbruch. Laeuft in der Sprache des Laufs (SPRACHE=ja / zh-Hans).

ImGuiWindow* rolloutFenster() {
    for (ImGuiWindow* w : GImGui->Windows) {
        if (std::strstr(w->Name, "mtrollouts") != nullptr && !w->Hidden) { return w; }
    }
    return nullptr;
}

// Fuellung der vollsten und der leersten Zeile ausser der letzten (Anteil der Breite) nach der ImGui-Regel; zeilen: Anzahl
void imguiRegel(const std::string& t, float breite, double& leerste, int& zeilen) {
    ImFont* f = ImGui::GetFont();
    const float gr = ImGui::GetFontSize();
    const char* s = t.c_str();
    const char* ende = s + t.size();
    leerste = 1.0;
    zeilen = 0;
    while (s < ende) {
        const char* e = f->CalcWordWrapPosition(gr, s, ende, breite);
        if (e == s) { ++e; }
        ++zeilen;
        const double w = ImGui::CalcTextSize(s, e).x;
        s = e;
        while (s < ende && *s == ' ') { ++s; }
        if (s < ende) { leerste = std::min(leerste, w / static_cast<double>(breite)); }
    }
}

std::vector<Schritt> cjkSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto breite = std::make_shared<float>(0.0F);
    add(tu("RD CJK: Model-Tab mit einer Figur (das Befehlsfeld gibt es nur mit Modell)", [] {
        g_app->leftMode = 2;
        std::string fehler;
        if (!oeffneModellAusArchiv("models/players/imperial/model.glm", &fehler)) { meldeFehler("RD CJK: imperial: " + fehler); }
    }));
    add(pause(1.5));
    add(tu("RD CJK: Hinweis des IK-Rollouts als Meldung im Befehlsfeld", [] { g_app->mtab.meldung = tr(Str::MbIkGliedWaehlen); }));
    add(pause(0.6));
    add(tu("RD CJK: Breite des Befehlsfelds", [breite] {
        ImGuiWindow* w = rolloutFenster();
        if (w == nullptr) {
            meldeFehler("RD CJK: Befehlsfeld (mtrollouts) nicht gefunden");
            return;
        }
        *breite = w->WorkRect.Max.x - w->DC.CursorStartPos.x;
    }));
    add(foto("rd_cjk_rollout"));
    add(pruefe("RD CJK: der Hinweis fuellt jede Zeile ausser der letzten zu mindestens 75 % (Umbruch zwischen CJK-Zeichen)", [breite] {
        const std::string t = tr(Str::MbIkGliedWaehlen);
        double leerste = 1.0;
        int zeilen = 0;
        imguiRegel(t, *breite, leerste, zeilen);
        char z[300];
        std::snprintf(z, sizeof(z), "Knopftest: RD CJK - Breite %.0f, Text %.0f; ImGui-Regel: %d Zeilen, leerste %.2f", static_cast<double>(*breite),
                      static_cast<double>(ImGui::CalcTextSize(t.c_str()).x), zeilen, leerste);
        diag::detail(z);
        // gezeichnet wird mit cjkTextDisabled (gui/textumbruch.h): die Zeilen des letzten umbrochenen CJK-Texts
        const CjkUmbruchStand& st = cjkUmbruchStand();
        double eigen = 1.0;
        for (std::size_t i = 0; i + 1 < st.zeilenBreiten.size(); ++i) {
            eigen = std::min(eigen, static_cast<double>(st.zeilenBreiten[i] / std::max(1.0F, st.breite)));
        }
        std::snprintf(z, sizeof(z), "Knopftest: RD CJK - gezeichnet: %d Zeilen in %.0f, leerste %.2f, breiteste %.0f (%s)", st.zeilen,
                      static_cast<double>(st.breite), eigen, static_cast<double>(st.breitesteZeile), st.text == t ? "dieser Text" : "anderer Text");
        diag::detail(z);
        return *breite > 50.0F && st.text == t && st.zeilen >= 1 && eigen >= 0.75 && st.breitesteZeile <= st.breite + 0.5F;
    }));
    add(tu("RD CJK: aufraeumen", [] {
        g_app->mtab.meldung.clear();
        g_app->leftMode = 0;
    }));
    return s;
}

// --- model2 an Brush-Entities ---------------------------------------------------------------------------
//
// Ein func_static (kleiner Brush oben, Ursprungs-Brush mit textures/system/origin in der Mitte des Wuerfels) mit model2 = der
// Wuerfel von R3: CG_Mover zeichnet das Modell am Ursprung der Entity. Gemessen der Anteil Rot (Vorderseite) im Bild des Wuerfels
// im Editor-Licht und in "Built map as in the game".

constexpr float kM2Y = 300;
constexpr float kM2Z = 160;

std::string model2Karte() { return karteTestPfad("rd_model2.map"); }

void model2KarteSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    r3::raum(k, welt, r3::kWand);
    welt->setzen("_ambient", "80");
    r3::ent(k, "light", {{"origin", "0 0 300"}, {"light", "1500"}});
    r3::ent(k, "info_player_start", {{"origin", "0 -300 24"}});
    auto e = k.neueEntitaet("func_static", true);
    e->setzen("model2", r3::wuerfelPfad());
    e->setzen("targetname", "rd_m2");
    k.entitaetEinfuegen(e);
    r3::quader(k, e, karte::Vektor3f(0, kM2Y + 200, 380), karte::Vektor3f(8, 8, 8), r3::kDunkel);
    r3::quader(k, e, karte::Vektor3f(0, kM2Y, kM2Z), karte::Vektor3f(8, 8, 8), "textures/system/origin");
    (void)karte::karteSpeichern(model2Karte(), k, false);
}

double model2Rot(const stufe9c::Bild& b, long& n) {
    const r3::Rechteck r = r3::rechteck(karte::Vektor3f(0, kM2Y - 32.0F, kM2Z), karte::Vektor3f(32, 0, 32), b.breite, b.hoehe, 0.2F);
    long rot = 0;
    n = 0;
    for (int y = r.y0; y <= r.y1; ++y) {
        for (int x = r.x0; x <= r.x1; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(b.breite) + static_cast<std::size_t>(x)) * 4U;
            if (b.rgba[i] > b.rgba[i + 1] + 30 && b.rgba[i] > b.rgba[i + 2] + 30) { ++rot; }
            ++n;
        }
    }
    return n > 0 ? static_cast<double>(rot) / static_cast<double>(n) : 0.0;
}

Schritt model2Messen(const std::string& name, std::shared_ptr<double> wert) {
    return tu("RD model2: " + name + " messen", [name, wert] {
        stufe9c::Bild b;
        if (!stufe9c::bildHolen(b) || b.breite < 640) {
            meldeFehler("RD model2: Kamerabild " + name + " nicht lesbar");
            return;
        }
        bmp("rd_model2_" + name, b);
        long n = 0;
        *wert = model2Rot(b, n);
        char z[160];
        std::snprintf(z, sizeof(z), "Knopftest: RD model2 %s - Anteil Rot %.3f (%ld Bildpunkte)", name.c_str(), *wert, n);
        diag::detail(z);
    });
}

std::vector<Schritt> model2Schritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto editor = std::make_shared<double>(0.0);
    auto spiel = std::make_shared<double>(0.0);
    add(tu("RD model2: func_static mit model2 (Wuerfel), Editor-Licht", [] {
        model2KarteSchreiben();
        (void)ke::karteOeffnen(model2Karte());
        kameraFest(karte::Vektor3f(0.0F, 100.0F, kM2Z), karte::Vektor3f(0.0F, 90.0F, 0.0F));
        ke::Editor& e = ke::editor();
        e.echtLicht.an = false;
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
    }));
    add(warteBis("RD model2: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(1.0));
    add(model2Messen("editor", editor));
    add(tu("RD model2: Schnell-Backen, Spielbild", [] {
        std::vector<std::string> ordner = aktiveSpielordner();
        if (ordner.empty() || ordner.back() != quelle()) {
            ordner.push_back(quelle());
            setzeSitzungsSpielordner(std::move(ordner));
            rescanGamePaths();
        }
        ke::Editor& e = ke::editor();
        e.echtLicht.an = true;
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
        e.echtLicht.spielbild = true;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT | karte::EXCLUDE_MODELS;
    }));
    add(stufe9c::langeWarten("RD model2: Schnell-Backen fertig", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("RD model2: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 400000));
    add(stufe9c::langeWarten("RD model2: Spielbild da", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    add(pause(1.5));
    add(model2Messen("spielbild", spiel));
    add(pruefe("RD model2: der Editor zeichnet das model2 am Ursprung des func_static wie das Spiel (Rot der Vorderseite beidemal ueber 80 %)",
               [editor, spiel] { return *spiel > 0.8 && *editor > 0.8; }));
    add(tu("RD model2: aufraeumen", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        e.echtLicht.an = false;
        e.filterMaske = 0;
        e.karte.undo.alsGespeichertMarkieren();
    }));
    return s;
}

// --- MRESTE 4: Ghoul2 als model2 und misc_model_ghoul -------------------------------------------------------
//
// Dunkler Raum, ein Licht. A: func_static mit model2 = turret_canon.glm (InitMover: G2API_InitGhoul2Model), B: misc_model_ghoul
// mit demselben Modell, C: func_breakable mit demselben model2 (InitBBrush: nur s.modelindex2 - das Spiel zeichnet nichts).
// Gemessen der Anteil der Bildpunkte, die sich vom Raum um das Rechteck abheben, im Rechteck des erwarteten Netzes
// (modell::glmStandbild: Bild 0 der GLA, Wurzel) im Editor-Licht und in "Built map as in the game".

constexpr const char* kGlmModell = "models/map_objects/imp_mine/turret_canon.glm";
constexpr float kGlmY = 320;
constexpr float kGlmZ = 150;
const karte::Vektor3f kGlmA(-170, kGlmY, kGlmZ);
const karte::Vektor3f kGlmB(170, kGlmY, kGlmZ);
const karte::Vektor3f kGlmC(0, kGlmY + 330, kGlmZ);

std::string glmKarte() { return karteTestPfad("rd_glm.map"); }

void glmKarteSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    r3::raum(k, welt, r3::kDunkel);
    welt->setzen("_ambient", "60");
    r3::ent(k, "light", {{"origin", "0 100 360"}, {"light", "1200"}});
    r3::ent(k, "info_player_start", {{"origin", "0 -300 24"}});
    for (const auto& [klasse, ort] : {std::pair<const char*, karte::Vektor3f>{"func_static", kGlmA}, {"func_breakable", kGlmC}}) {
        auto e = k.neueEntitaet(klasse, true);
        e->setzen("model2", kGlmModell);
        k.entitaetEinfuegen(e);
        r3::quader(k, e, karte::Vektor3f(ort[0], ort[1] + 250, 380), karte::Vektor3f(8, 8, 8), r3::kDunkel);
        // ein kleiner Ursprungs-Brush (q3map2 setzt damit "origin"; der Schluessel allein verschiebt die Brushes nicht, das
        // Spiel stellte das Brush-Modell dann ausserhalb der Karte auf und schickte die Entity nie)
        r3::quader(k, e, ort, karte::Vektor3f(1, 1, 1), "textures/system/origin");
    }
    char o[64];
    std::snprintf(o, sizeof(o), "%g %g %g", static_cast<double>(kGlmB[0]), static_cast<double>(kGlmB[1]), static_cast<double>(kGlmB[2]));
    r3::ent(k, "misc_model_ghoul", {{"origin", o}, {"model", kGlmModell}});
    (void)karte::karteSpeichern(glmKarte(), k, false);
}

// Mitte und halbe Groesse des Netzes, das das Spiel zeichnet (Stufe 0), im Raum der Entity
bool glmKasten(karte::Vektor3f& mitte, karte::Vektor3f& halb) {
    std::string data;
    std::map<std::string, Md3Model> dateien;
    std::string fehler;
    if (!readFromArchives(kGlmModell, data) || glmStufenLaden(data, kGlmModell, dateien, fehler) == 0) {
        meldeFehler(std::string("RD glm: ") + kGlmModell + " nicht geladen: " + fehler);
        return false;
    }
    float mn[3] = {1e30F, 1e30F, 1e30F};
    float mx[3] = {-1e30F, -1e30F, -1e30F};
    for (const Md3Surface& sf : dateien[kGlmModell].surfaces) {
        for (const Md3Vertex& v : sf.verts) {
            for (int i = 0; i < 3; ++i) {
                mn[i] = std::min(mn[i], v.xyz[i]);
                mx[i] = std::max(mx[i], v.xyz[i]);
            }
        }
    }
    mitte = karte::Vektor3f((mn[0] + mx[0]) * 0.5F, (mn[1] + mx[1]) * 0.5F, (mn[2] + mx[2]) * 0.5F);
    halb = karte::Vektor3f((mx[0] - mn[0]) * 0.5F, (mx[1] - mn[1]) * 0.5F, (mx[2] - mn[2]) * 0.5F);
    return mx[0] >= mn[0];
}

// Anteil der Bildpunkte im inneren Rechteck des Netzes, deren Helligkeit um mehr als 8 vom Median des Randstreifens (bis 40 %
// ausserhalb des Rechtecks: nur Raum) abweicht - ohne Modell ist das Rechteck Raum wie sein Rand; `mittel`: die mittlere
// Helligkeit dieser Bildpunkte
double glmHell(const stufe9c::Bild& b, const karte::Vektor3f& ort, const karte::Vektor3f& mitte, const karte::Vektor3f& halb, long& n,
               double& mittel) {
    const r3::Rechteck innen = r3::rechteck(ort + mitte, halb, b.breite, b.hoehe, 0.2F);
    const r3::Rechteck kasten = r3::rechteck(ort + mitte, halb, b.breite, b.hoehe, 0.0F);
    const r3::Rechteck aussen = r3::rechteck(ort + mitte, halb, b.breite, b.hoehe, -0.4F);
    const auto hell = [&b](int x, int y) {
        const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(b.breite) + static_cast<std::size_t>(x)) * 4U;
        return (static_cast<int>(b.rgba[i]) + b.rgba[i + 1] + b.rgba[i + 2]) / 3;
    };
    std::vector<int> rand;
    for (int y = aussen.y0; y <= aussen.y1; ++y) {
        for (int x = aussen.x0; x <= aussen.x1; ++x) {
            if (x >= kasten.x0 && x <= kasten.x1 && y >= kasten.y0 && y <= kasten.y1) { continue; }
            rand.push_back(hell(x, y));
        }
    }
    n = 0;
    mittel = 0.0;
    if (rand.empty()) { return 0.0; }
    std::nth_element(rand.begin(), rand.begin() + static_cast<std::ptrdiff_t>(rand.size() / 2), rand.end());
    const int median = rand[rand.size() / 2];
    long anders = 0;
    for (int y = innen.y0; y <= innen.y1; ++y) {
        for (int x = innen.x0; x <= innen.x1; ++x) {
            if (std::abs(hell(x, y) - median) > 8) {
                ++anders;
                mittel += hell(x, y);
            }
            ++n;
        }
    }
    if (anders > 0) { mittel /= static_cast<double>(anders); }
    return n > 0 ? static_cast<double>(anders) / static_cast<double>(n) : 0.0;
}

Schritt glmMessen(const std::string& name, std::shared_ptr<std::array<double, 6>> wert) {
    return tu("RD glm: " + name + " messen", [name, wert] {
        stufe9c::Bild b;
        if (!stufe9c::bildHolen(b) || b.breite < 640) {
            meldeFehler("RD glm: Kamerabild " + name + " nicht lesbar");
            return;
        }
        bmp("rd_glm_" + name, b);
        karte::Vektor3f mitte;
        karte::Vektor3f halb;
        if (!glmKasten(mitte, halb)) { return; }
        long n[3] = {0, 0, 0};
        const karte::Vektor3f orte[3] = {kGlmA, kGlmB, kGlmC};
        for (int i = 0; i < 3; ++i) {
            const auto u = static_cast<std::size_t>(i);
            (*wert)[u] = glmHell(b, orte[i], mitte, halb, n[i], (*wert)[u + 3]);
        }
        char z[320];
        std::snprintf(z, sizeof(z), "Knopftest: RD glm %s - abgehoben A (func_static) %.3f (%ld, Helligkeit %.1f), B (misc_model_ghoul) %.3f (%ld, %.1f), "
                      "C (func_breakable) %.3f (%ld)", name.c_str(), (*wert)[0], n[0], (*wert)[3], (*wert)[1], n[1], (*wert)[4], (*wert)[2], n[2]);
        diag::detail(z);
    });
}

std::vector<Schritt> glmSchritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto editor = std::make_shared<std::array<double, 6>>();
    auto spiel = std::make_shared<std::array<double, 6>>();
    add(tu("RD glm: Testdaten (Raum-Bilder) als Extra Resource Path", [] {
        karteDialogeZu();
        ke::editor().karte.undo.alsGespeichertMarkieren();
        pk3Schreiben();
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0] = karte::ordnerBereinigt(quelle()); });
    }));
    add(warteBis("RD glm: Spieldaten mit dem Extra Resource Path", [] {
        ke::spielPruefen();
        const ke::Editor& e = ke::editor();
        return ke::spielBereit() && e.karteSpiel && e.karteSpiel->inventar.shaderFinden("textures/rd/scroll") != nullptr;
    }, 20000));
    add(tu("RD glm: model2 und misc_model_ghoul mit .glm, Editor-Licht", [] {
        glmKarteSchreiben();
        (void)ke::karteOeffnen(glmKarte());
        kameraFest(karte::Vektor3f(0.0F, -40.0F, kGlmZ + 40.0F), karte::Vektor3f(0.0F, 90.0F, 0.0F));
        ke::Editor& e = ke::editor();
        e.echtLicht.an = false;
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
    }));
    add(warteBis("RD glm: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(1.0));
    add(glmMessen("editor", editor));
    add(tu("RD glm: Schnell-Backen, Spielbild", [] {
        std::vector<std::string> ordner = aktiveSpielordner();
        if (ordner.empty() || ordner.back() != quelle()) {
            ordner.push_back(quelle());
            setzeSitzungsSpielordner(std::move(ordner));
            rescanGamePaths();
        }
        ke::Editor& e = ke::editor();
        e.echtLicht.an = true;
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
        e.echtLicht.spielbild = true;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT | karte::EXCLUDE_MODELS;
    }));
    add(stufe9c::langeWarten("RD glm: Schnell-Backen fertig", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("RD glm: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 400000));
    add(stufe9c::langeWarten("RD glm: Spielbild da", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    add(pause(1.5));
    add(glmMessen("spielbild", spiel));
    add(pruefe("RD glm: die Kamera des Editors zeichnet das .glm am func_static (A ueber 25 % abgehoben), nicht am func_breakable (C unter 5 %)",
               [editor] { return (*editor)[0] > 0.25 && (*editor)[2] < 0.05; }));
    add(pruefe("RD glm: das Spielbild zeichnet A und den misc_model_ghoul B (ueber 25 % abgehoben), C nicht (unter 5 %) - wie das Spiel",
               [spiel] { return (*spiel)[0] > 0.25 && (*spiel)[1] > 0.25 && (*spiel)[2] < 0.05; }));
    // Movie Duels mit derselben Karte und Kamera (Y:/werkzeuge/mr/spiel_glm.py): A 15,0, B 14,5 - gleich hell
    add(pruefe("RD glm: im Spielbild sind A und B gleich hell wie im Spiel (Verhaeltnis 0,8 bis 1,25; das model2 liegt nicht noch einmal im "
               "Editor-Licht darueber)",
               [spiel] { return (*spiel)[4] > 0.0 && (*spiel)[3] / (*spiel)[4] > 0.8 && (*spiel)[3] / (*spiel)[4] < 1.25; }));
    add(tu("RD glm: aufraeumen", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.spielbild = false;
        e.echtLicht.an = false;
        e.filterMaske = 0;
        e.karte.undo.alsGespeichertMarkieren();
    }));
    return s;
}

// --- Movie-Duels-Figuren: doppelte NPC-Namen ----------------------------------------------------------
//
// StOfficerAlt steht in Movie Duels dreimal: MD_Assets.pk3 ext_data/npcs/z_JO_NPCs.npc (customSkin officer, surfOn/surfOff
// auskommentiert), base/assets1.pk3 stofficer.npc (ebenso) und stofficeralt.npc (surfOn torso_pauldron, surfOff ...). Das Spiel
// haengt die Dateien in der Reihenfolge der Suchpfade hintereinander und nimmt den ersten Block (NPC_LoadParms, NPC_ParseParms).

std::vector<Schritt> npcSchritte() {
    std::vector<Schritt> s;
    s.push_back(pruefe("RD NPC: StOfficerAlt wie im Spiel - der erste Block (customSkin officer, ohne surfOn/surfOff)", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        NpcMap m;
        const int n = npcDateienLesen(m);
        const auto it = m.find("stofficeralt");
        if (it == m.end()) {
            diag::detail("Knopftest: RD NPC - StOfficerAlt fehlt (" + std::to_string(n) + " Dateien)");
            return false;
        }
        diag::detail("Knopftest: RD NPC - StOfficerAlt aus " + std::to_string(n) + " Dateien: customSkin \"" + it->second.customSkin + "\", surfOff \"" +
                     it->second.surfOff + "\", surfOn \"" + it->second.surfOn + "\"");
        return it->second.customSkin == "officer" && it->second.surfOff.empty() && it->second.surfOn.empty();
    }));
    // Movie Duels imperial: die Haut belegt "hips" mit "*off", die Hierarchie hat zwei Flaechen "hips" (die erste OFF) - die zweite
    // bleibt nach den Ghoul2-Regeln an, ihr Shader "*off" hat keine Stufe (R_FindShader ohne Bild): das Spiel zeigt sie nicht
    // Gegenprobe: jedi_zm (dieselbe Bauart, nicht zum Finden benutzt)
    for (const char* figur : {"imperial", "jedi_zm"}) {
        const std::string f = figur;
        s.push_back(tu("RD NPC: Model-Tab, Movie Duels " + f, [f] {
            g_app->leftMode = 2;
            std::string fehler;
            if (!oeffneModellAusArchiv("models/players/" + f + "/model.glm", &fehler)) { meldeFehler("RD NPC: " + f + ": " + fehler); }
        }));
        s.push_back(pause(2.0));
        s.push_back(foto("rd_" + f));
        s.push_back(pruefe("RD NPC: " + f + " - keine Flaeche mit \"*off\" in der Haut wird gezeichnet (Shader ohne Stufe)", [f] {
            int n = 0;
            std::string namen;
            const auto& anz = g_app->mtab.anzeige.surfaces;
            for (std::size_t i = 0; i < anz.size() && i < g_app->model.surfaces.size(); ++i) {
                if (g_app->model.surfaces[i].skinnedOff && !anz[i].skinnedOff) {
                    ++n;
                    namen += " " + g_app->model.surfaces[i].name;
                }
            }
            diag::detail("Knopftest: RD NPC - " + f + ": " + std::to_string(anz.size()) + " Flaechen, mit \"*off\" gezeichnet " + std::to_string(n) + ":" + namen);
            return !anz.empty() && n == 0;
        }));
    }
    s.push_back(tu("RD NPC: zurueck", [] { g_app->leftMode = 0; }));
    return s;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s = stufenSchritte();
    for (Schritt& x : spiegelSchritte()) { s.push_back(std::move(x)); }
    for (Schritt& x : model2Schritte()) { s.push_back(std::move(x)); }
    for (Schritt& x : fxSchritte()) { s.push_back(std::move(x)); }
    return s;
}

}  // namespace rd
