// selbsttest_karte9d.inc - Stufe 9d des Karten-Editors: Echtzeit-Ansicht mit Nebel, Effekten und
// Skyportal, misc_model werfen Schatten, Shader live bearbeiten, Patch-Bruecke.
//
// Eingebunden in gui/selbsttest_handbuch.inc (nach Stufe 9c). Allein: BHED_KARTE_NUR=stufe9d.
// Die Testkarten schreibt der Lauf selbst; der Testshader (Nebel) liegt im Import-Ordner
// (<Einstellungen>/import/shaders/bhed9d_test.shader) - nie im Spielordner.
namespace stufe9d {

namespace ke = karteneditor;

// JA liest die Shader aus shaders/ (shaderpath in ja.game)
std::string importShaderPfad() { return platform::settingsDirectory() + "\\import\\shaders\\bhed9d_test.shader"; }

std::string nebelShaderText(float r, float g, float b) {
    char z[600];
    std::snprintf(z, sizeof(z),
                  "// Stufe 9d, Selbsttest: Nebel wie im Spiel\n"
                  "textures/bhed9d/nebel\n{\n\tqer_editorimage textures/system/caulk\n\tsurfaceparm nolightmap\n\tsurfaceparm nonsolid\n"
                  "\tsurfaceparm trans\n\tsurfaceparm fog\n\tfogparms ( %.2f %.2f %.2f ) 256\n}\n",
                  static_cast<double>(r), static_cast<double>(g), static_cast<double>(b));
    return z;
}

void nebelShaderSchreiben(float r, float g, float b) {
    std::error_code ec;
    std::filesystem::create_directories(bhed::utf8Pfad(platform::settingsDirectory() + "\\import\\shaders"), ec);
    std::ofstream f(bhed::utf8Pfad(importShaderPfad()), std::ios::binary | std::ios::trunc);
    f << nebelShaderText(r, g, b);
}

std::string raumKarte() { return karteTestPfad("s9d_raum.map"); }

// Ein geschlossener Steinraum (1024 x 1024, 512 hoch) mit einem Punktlicht; nebel: ein Nebelpinsel
// ueber dem Boden (128 hoch); global: worldspawn "_fog"
void raumSchreiben(bool nebel, bool global, const std::string& extra = std::string()) {
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
    quader(karte::Vektor3f(0, 0, -8), karte::Vektor3f(528, 528, 8), stein);
    quader(karte::Vektor3f(0, 0, 520), karte::Vektor3f(528, 528, 8), stein);
    quader(karte::Vektor3f(-520, 0, 256), karte::Vektor3f(8, 512, 256), stein);
    quader(karte::Vektor3f(520, 0, 256), karte::Vektor3f(8, 512, 256), stein);
    quader(karte::Vektor3f(0, -520, 256), karte::Vektor3f(512, 8, 256), stein);
    quader(karte::Vektor3f(0, 520, 256), karte::Vektor3f(512, 8, 256), stein);
    if (nebel) { quader(karte::Vektor3f(0, 0, 64), karte::Vektor3f(512, 512, 64), "textures/bhed9d/nebel"); }
    if (global) { welt->setzen("_fog", "textures/bhed9d/nebel"); }
    welt->setzen("_ambient", "60");
    auto licht = k.neueEntitaet("light", false);
    licht->setzen("origin", "0 0 400");
    licht->setzen("light", "800");
    k.entitaetEinfuegen(licht);
    auto start = k.neueEntitaet("info_player_start", false);
    start->setzen("origin", "-400 -400 24");
    k.entitaetEinfuegen(start);
    std::string text;
    (void)karte::karteSpeichern(raumKarte(), k, false);
    if (!extra.empty()) {
        std::ofstream f(bhed::utf8Pfad(raumKarte()), std::ios::binary | std::ios::app);
        f << extra;
    }
}

// Mittel der Farbe (0..255) in einem Fenster um die Bildmitte
std::array<double, 3> mittelFarbe(int halb = 8, int dx = 0, int dy = 0) {
    std::array<double, 3> r{-1, -1, -1};
    std::vector<unsigned char> b;
    int w = 0;
    int h = 0;
    if (!ke::kameraBild(b, w, h) || w < 64 || h < 64) { return r; }
    double s[3] = {0, 0, 0};
    int n = 0;
    for (int y = h / 2 + dy - halb; y < h / 2 + dy + halb; ++y) {
        for (int x = w / 2 + dx - halb; x < w / 2 + dx + halb; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4U;
            for (int c = 0; c < 3; ++c) { s[c] += b[i + static_cast<std::size_t>(c)]; }
            ++n;
        }
    }
    for (int c = 0; c < 3; ++c) { r[static_cast<std::size_t>(c)] = s[c] / n; }
    return r;
}

std::string farbText(const std::array<double, 3>& f) {
    char z[96];
    std::snprintf(z, sizeof(z), "%.1f %.1f %.1f", f[0], f[1], f[2]);
    return z;
}

Schritt raumOeffnen(const std::string& was, bool nebel, bool global, const std::string& extra = std::string()) {
    return tu("Stufe 9d: " + was, [nebel, global, extra] {
        karteDialogeZu();
        raumSchreiben(nebel, global, extra);
        (void)ke::karteOeffnen(raumKarte());
        ke::Editor& e = ke::editor();
        e.kameraE.modus = karte::KameraModus::Textur;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        e.kamera.ursprung = karte::Vektor3f(0, 0, 300);
        e.kamera.winkel = karte::Vektor3f(-89.0F, 0.0F, 0.0F);
    });
}

// --- Punkt 3: Nebel ------------------------------------------------------------------------
std::vector<Schritt> nebel() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto f = std::make_shared<std::array<std::array<double, 3>, 4>>();   // ohne, Nebelpinsel, global, Shader geaendert
    add(tu("Stufe 9d/3: Testshader mit fogParms in den Import-Ordner, Suchpfad neu", [] {
        nebelShaderSchreiben(0.1F, 0.9F, 0.1F);
        ke::editor().spielKennung.clear();
    }));
    add(warteBis("Stufe 9d/3: Spieldaten neu geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit() && !ke::editor().spielKennung.empty();
    }, 20000));
    add(raumOeffnen("Raum ohne Nebel, Echtzeit-Licht, Blick von oben", false, false));
    add(warteBis("Stufe 9d/3: Texturen", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(1.0));
    add(tu("Stufe 9d/3: Bodenfarbe ohne Nebel", [f] { (*f)[0] = mittelFarbe(); }));
    add(raumOeffnen("derselbe Raum mit Nebelpinsel (fogParms 0.1 0.9 0.1, 256) ueber dem Boden", true, false));
    add(warteBis("Stufe 9d/3: Texturen (Nebel)", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(1.0));
    add(tu("Stufe 9d/3: Bodenfarbe im Nebel", [f] { (*f)[1] = mittelFarbe(); }));
    add(foto("karteneditor_s9d_nebel"));
    add(pruefe("Stufe 9d/3: Nebelpinsel wie im Spiel - der Boden hat die Farbe von RB_FogPass (Nebelfarbe mit R_FogFactor gemischt)", [f] {
        const ke::Editor& e = ke::editor();
        // erwartet: lerp(Boden, Nebelfarbe, Dichte) mit der Dichte der Bibliothek an der Bodenmitte
        std::vector<karte::EchtNebel> nb = karte::echtNebelSammeln(e.karte, [](const std::string& sh) {
            karte::EchtNebelShader r;
            if (sh == "textures/bhed9d/nebel") {
                r.nebel = true;
                r.farbe = karte::Vektor3f(0.1F, 0.9F, 0.1F);
                r.tiefe = 256;
            }
            return r;
        });
        const karte::Vektor3f vorn(0, 0, -1);
        const float d = nb.empty() ? -1.0F : karte::echtNebelDichte(nb[0], karte::Vektor3f(0, 0, 0), e.kamera.ursprung, vorn);
        const std::array<double, 3>& a = (*f)[0];
        const std::array<double, 3>& b = (*f)[1];
        const double soll[3] = {a[0] + (25.5 - a[0]) * d, a[1] + (229.5 - a[1]) * d, a[2] + (25.5 - a[2]) * d};
        double abw = 0;
        for (int c = 0; c < 3; ++c) { abw = std::max(abw, std::fabs(b[static_cast<std::size_t>(c)] - soll[c])); }
        diag::detail("Knopftest: Stufe 9d/3 - Nebel " + std::to_string(e.echtLicht.nebel) + ", Flaeche " + (nb.empty() ? "-" : (nb[0].hatFlaeche ? "ja" : "nein")) +
                     ", Dichte " + std::to_string(d) + ", Boden ohne " + farbText(a) + ", mit " + farbText(b) + ", erwartet " +
                     std::to_string(soll[0]) + " " + std::to_string(soll[1]) + " " + std::to_string(soll[2]) + ", Abweichung " + std::to_string(abw));
        return e.echtLicht.nebel == 1 && !nb.empty() && nb[0].hatFlaeche && d > 0.2F && abw <= 12.0 && b[1] > a[1] + 10.0;
    }));
    add(raumOeffnen("derselbe Raum mit globalem Nebel (worldspawn _fog)", false, true));
    add(warteBis("Stufe 9d/3: Texturen (global)", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(1.0));
    add(tu("Stufe 9d/3: Bodenfarbe im globalen Nebel", [f] { (*f)[2] = mittelFarbe(); }));
    add(pruefe("Stufe 9d/3: globaler Nebel (worldspawn _fog) - die ganze Ansicht im Nebel", [f] {
        const ke::Editor& e = ke::editor();
        std::vector<karte::EchtNebel> nb = karte::echtNebelSammeln(e.karte, [](const std::string& sh) {
            karte::EchtNebelShader r;
            if (sh == "textures/bhed9d/nebel") {
                r.nebel = true;
                r.farbe = karte::Vektor3f(0.1F, 0.9F, 0.1F);
                r.tiefe = 256;
            }
            return r;
        });
        const float d = nb.empty() ? -1.0F : karte::echtNebelDichte(nb.back(), karte::Vektor3f(0, 0, 0), e.kamera.ursprung, karte::Vektor3f(0, 0, -1));
        const std::array<double, 3>& a = (*f)[0];
        const std::array<double, 3>& b = (*f)[2];
        const double soll[3] = {a[0] + (25.5 - a[0]) * d, a[1] + (229.5 - a[1]) * d, a[2] + (25.5 - a[2]) * d};
        double abw = 0;
        for (int c = 0; c < 3; ++c) { abw = std::max(abw, std::fabs(b[static_cast<std::size_t>(c)] - soll[c])); }
        diag::detail("Knopftest: Stufe 9d/3 - globaler Nebel: Dichte " + std::to_string(d) + ", Boden " + farbText(b) + ", Abweichung " + std::to_string(abw));
        return !nb.empty() && nb.back().global && abw <= 12.0 && b[1] > a[1] + 10.0;
    }));
    // --- Punkt 4: Shader live bearbeiten (am Nebelshader) ---
    add(raumOeffnen("Raum mit Nebelpinsel fuer den Shader-Editor", true, false));
    add(warteBis("Stufe 9d/4: Texturen", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(0.5));
    add(tu("Stufe 9d/4: Shader-Editor oeffnen (Kontextmenue des Texturbrowsers), fogParms gruen -> rot, Save", [] {
        ke::shaderEditorOeffnen("textures/bhed9d/nebel");
        std::string& t = ke::shaderEditorText();
        const std::size_t p = t.find("( 0.10 0.90 0.10 )");
        if (p != std::string::npos) { t.replace(p, 18, "( 0.90 0.10 0.10 )"); }
        ke::shaderEditorTextGeaendert();
        (void)ke::shaderEditorSpeichern();
    }));
    add(warteBis("Stufe 9d/4: neu geladen", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(1.0));
    add(tu("Stufe 9d/4: Bodenfarbe nach dem Speichern", [f] { (*f)[3] = mittelFarbe(); }));
    add(foto("karteneditor_s9d_shaderlive"));
    add(pruefe("Stufe 9d/4: Shader geaendert und gespeichert (Import-Ordner) - das Bild aendert sich ohne Neustart", [f] {
        const std::array<double, 3>& vor = (*f)[1];
        const std::array<double, 3>& nach = (*f)[3];
        std::string ziel = ke::shaderEditorZiel();
        std::replace(ziel.begin(), ziel.end(), '/', '\\');
        const bool imImport = ziel.find("\\import\\") != std::string::npos;
        const std::string inhalt = dateiInhalt(importShaderPfad());
        diag::detail("Knopftest: Stufe 9d/4 - Ziel " + ziel + ", vorher " + farbText(vor) + ", nachher " + farbText(nach) + ", neu geladen " +
                     std::to_string(ke::shaderNeuGeladen()));
        return imImport && inhalt.find("( 0.90 0.10 0.10 )") != std::string::npos && nach[0] > vor[0] + 20.0 && nach[1] < vor[1] - 20.0;
    }));
    add(tu("Stufe 9d/4: die Datei von aussen aendern (blau), der Waechter laedt neu", [] { nebelShaderSchreiben(0.1F, 0.1F, 0.9F); }));
    add(warteBis("Stufe 9d/4: Waechter", [] { return dateiInhalt(importShaderPfad()).find("0.90 )") != std::string::npos; }, 100));
    add(pause(2.0));
    add(warteBis("Stufe 9d/4: Texturen (blau)", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(1.0));
    add(pruefe("Stufe 9d/4: Aenderung an der .shader-Datei erkannt - Kamera zeigt den blauen Nebel", [] {
        const std::array<double, 3> c = mittelFarbe();
        diag::detail("Knopftest: Stufe 9d/4 - nach der Aenderung von aussen " + farbText(c));
        return c[2] > c[0] + 20.0 && c[2] > c[1] + 10.0;
    }));
    add(pruefe("Stufe 9d/4: ein Spiel-Shader wird nie in den Spielordner gespeichert (Ziel im Import-Ordner)", [] {
        const auto& sp = ke::editor().spiel;
        if (!sp) { return false; }
        std::string spielShader;
        for (const karte::ShaderEintrag* e : sp->inventar.shader()) {
            if (e->name.rfind("textures/", 0) == 0 && !e->istStandard() && e->name.find("bhed9d") == std::string::npos) {
                spielShader = e->name;
                break;
            }
        }
        ke::shaderEditorOeffnen(spielShader);
        std::string ziel = ke::shaderEditorZiel();
        std::replace(ziel.begin(), ziel.end(), '/', '\\');
        diag::detail("Knopftest: Stufe 9d/4 - " + spielShader + " -> " + ziel);
        return !spielShader.empty() && ziel.find("\\import\\") != std::string::npos;
    }));
    add(tu("Stufe 9d/4: Testshader zuruecksetzen (gruen), Shader-Editor zu", [] {
        nebelShaderSchreiben(0.1F, 0.9F, 0.1F);
        ke::shaderEditorSchliessen();
    }));
    add(pause(1.5));
    return s;
}

// --- Punkt 3: Effekte (fx_runner) ----------------------------------------------------------
std::vector<Schritt> effekte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto fx = std::make_shared<std::string>();
    auto f = std::make_shared<std::array<std::array<double, 3>, 2>>();
    add(tu("Stufe 9d/3: ein Effekt aus dem Spiel (effects/env)", [fx] {
        const auto& sp = ke::editor().karteSpiel;
        if (!sp) { return; }
        for (const char* wunsch : {"env/fire", "env/small_fire", "env/med_fire", "env/torch_fire", "env/sparks"}) {
            std::string t;
            if (sp->fs.textLesen(std::string("effects/") + wunsch + ".efx", t)) {
                *fx = wunsch;
                return;
            }
        }
        for (const std::string& p : sp->fs.dateien("effects/env/", "efx", 4)) {
            *fx = "env/" + p.substr(0, p.size() - 4);
            return;
        }
    }));
    add(raumOeffnen("Raum ohne Effekt, Blick waagerecht", false, false));
    add(tu("Stufe 9d/3: Kamera waagerecht auf die Mitte", [] {
        ke::Editor& e = ke::editor();
        e.kamera.ursprung = karte::Vektor3f(-300, 0, 96);
        e.kamera.winkel = karte::Vektor3f(0, 0, 0);
    }));
    add(warteBis("Stufe 9d/3: Texturen (Effekt)", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(1.0));
    add(tu("Stufe 9d/3: Bild ohne Effekt", [f] { (*f)[0] = mittelFarbe(24, 0, 0); }));
    add(tu("Stufe 9d/3: fx_runner in die Mitte (Echtzeit-Effekte an)", [fx] {
        ke::Editor& e = ke::editor();
        e.echtLicht.effekte = true;
        e.karte.undo.start();
        auto r = e.karte.neueEntitaet("fx_runner", false);
        r->setzen("origin", "0 0 48");
        r->setzen("fxFile", *fx);
        e.karte.entitaetEinfuegen(r);
        e.karte.undo.finish("Stufe 9d: fx_runner");
    }));
    add(pause(3.0));
    add(warteBis("Stufe 9d/3: Effektbilder", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(0.5));
    add(tu("Stufe 9d/3: Bild mit Effekt", [f] { (*f)[1] = mittelFarbe(24, 0, 0); }));
    add(foto("karteneditor_s9d_effekt"));
    add(pruefe("Stufe 9d/3: fx_runner spielt in der Echtzeit-Ansicht (Effektsystem der Vorschau)", [fx, f] {
        const ke::Editor& e = ke::editor();
        double d = 0;
        for (int c = 0; c < 3; ++c) { d = std::max(d, std::fabs((*f)[1][static_cast<std::size_t>(c)] - (*f)[0][static_cast<std::size_t>(c)])); }
        diag::detail("Knopftest: Stufe 9d/3 - Effekt " + *fx + ", Laeufer " + std::to_string(e.echtLicht.effektLaeufer) + ", Ecken " +
                     std::to_string(e.echtLicht.effektEcken) + ", ohne " + farbText((*f)[0]) + ", mit " + farbText((*f)[1]));
        return !fx->empty() && e.echtLicht.effektLaeufer == 1 && e.echtLicht.effektEcken > 0 && d > 3.0;
    }));
    add(tu("Stufe 9d/3: Echtzeit-Effekte aus (Build-Menue)", [] { ke::editor().echtLicht.effekte = false; }));
    add(pause(0.5));
    add(pruefe("Stufe 9d/3: abschaltbar - ohne Effekte keine Ecken", [] { return ke::editor().echtLicht.effektEcken == 0 || !ke::editor().echtLicht.effekte; }));
    add(tu("Stufe 9d/3: Echtzeit-Effekte wieder an", [] { ke::editor().echtLicht.effekte = true; }));
    return s;
}

// --- Punkt 3: Skyportal --------------------------------------------------------------------
std::vector<Schritt> portal() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto f = std::make_shared<std::array<std::array<double, 3>, 2>>();
    const auto oeffnen = [](bool mitPortal) {
        return tu(std::string("Stufe 9d/3: Himmelsraum ") + (mitPortal ? "mit misc_skyportal (Portalraum mit Licht)" : "ohne Skyportal"), [mitPortal] {
            karteDialogeZu();
            stufe9c::himmelKarteSchreiben();
            std::string t = dateiInhalt(stufe9c::himmelKarte());
            if (mitPortal) {
                // der Portalraum weit weg: ein Steinkasten mit hellem Licht und dem misc_skyportal in der Mitte
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
                const karte::Vektor3f o(4096, 0, 256);
                quader(o + karte::Vektor3f(0, 0, -264), karte::Vektor3f(264, 264, 8));
                quader(o + karte::Vektor3f(0, 0, 264), karte::Vektor3f(264, 264, 8));
                quader(o + karte::Vektor3f(-264, 0, 0), karte::Vektor3f(8, 256, 256));
                quader(o + karte::Vektor3f(264, 0, 0), karte::Vektor3f(8, 256, 256));
                quader(o + karte::Vektor3f(0, -264, 0), karte::Vektor3f(256, 8, 256));
                quader(o + karte::Vektor3f(0, 264, 0), karte::Vektor3f(256, 8, 256));
                std::string pt = karte::karteSchreiben(k);
                // nur die Pinsel der Welt (ohne die worldspawn-Klammer) an die Welt der Himmelskarte haengen ist
                // umstaendlich - als eigene func_group geht es einfacher
                const std::size_t a = pt.find("// brush");
                const std::size_t e = pt.rfind('}');
                std::string pinsel = a != std::string::npos && e != std::string::npos ? pt.substr(a, e - a) : std::string();
                t += "{\n\"classname\" \"func_group\"\n" + pinsel + "}\n";
                t += "{\n\"classname\" \"misc_skyportal\"\n\"origin\" \"4096 0 256\"\n}\n";
                t += "{\n\"classname\" \"light\"\n\"origin\" \"4096 0 400\"\n\"light\" \"1500\"\n}\n";
            }
            std::ofstream fo(bhed::utf8Pfad(karteTestPfad("s9d_portal.map")), std::ios::binary);
            fo << t;
            fo.close();
            (void)ke::karteOeffnen(karteTestPfad("s9d_portal.map"));
            ke::Editor& e = ke::editor();
            e.kameraE.modus = karte::KameraModus::Textur;
            e.echtLicht.an = true;
            e.echtLicht.backen = false;
            e.kamera.ursprung = karte::Vektor3f(0, 0, 100);
            e.kamera.winkel = karte::Vektor3f(70.0F, 0.0F, 0.0F);   // schraeg nach oben in den Himmel
        });
    };
    add(oeffnen(false));
    add(warteBis("Stufe 9d/3: Texturen (Himmel)", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(3.0));
    add(foto("karteneditor_s9d_himmel_ohne_portal"));
    add(tu("Stufe 9d/3: Himmel ohne Portal", [f] { (*f)[0] = mittelFarbe(); }));
    add(oeffnen(true));
    add(warteBis("Stufe 9d/3: Texturen (Portal)", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(3.0));
    add(tu("Stufe 9d/3: Himmel mit Portal", [f] { (*f)[1] = mittelFarbe(); }));
    add(foto("karteneditor_s9d_skyportal"));
    add(pruefe("Stufe 9d/3: misc_skyportal - der Himmel zeigt den Portalraum (CG_DrawSkyBoxPortal)", [f] {
        const ke::Editor& e = ke::editor();
        double d = 0;
        for (int c = 0; c < 3; ++c) { d = std::max(d, std::fabs((*f)[1][static_cast<std::size_t>(c)] - (*f)[0][static_cast<std::size_t>(c)])); }
        diag::detail("Knopftest: Stufe 9d/3 - Himmel ohne Portal " + farbText((*f)[0]) + ", mit " + farbText((*f)[1]) + ", Portal " + (e.echtLicht.portal ? "ja" : "nein"));
        const double hell1 = (*f)[1][0] + (*f)[1][1] + (*f)[1][2];
        return e.echtLicht.portal && hell1 > 30.0 && d > 10.0;
    }));
    return s;
}

// --- Punkt 2: misc_model werfen Schatten -----------------------------------------------------
// Ein Modell des Spiels zwischen Punktlicht (0 0 400) und Boden. Der Messpunkt liegt nachweislich im
// Schatten: die Strecke vom Licht zu ihm (und zu acht Nachbarn im Abstand 8) trifft ein Dreieck des
// Modells (Moeller-Trumbore auf den Dreiecken von echtModellFlaechen, also denen von q3map2).
bool streckeTrifft(const karte::Vektor3f& a, const karte::Vektor3f& b, const std::vector<karte::Vektor3f>& d) {
    const karte::Vektor3f r = b - a;
    for (std::size_t i = 0; i + 2 < d.size(); i += 3) {
        const karte::Vektor3f e1 = d[i + 1] - d[i];
        const karte::Vektor3f e2 = d[i + 2] - d[i];
        const karte::Vektor3f p(r[1] * e2[2] - r[2] * e2[1], r[2] * e2[0] - r[0] * e2[2], r[0] * e2[1] - r[1] * e2[0]);
        const float det = e1[0] * p[0] + e1[1] * p[1] + e1[2] * p[2];
        if (std::fabs(det) < 1e-6F) { continue; }
        const karte::Vektor3f s = a - d[i];
        const float u = (s[0] * p[0] + s[1] * p[1] + s[2] * p[2]) / det;
        if (u < 0 || u > 1) { continue; }
        const karte::Vektor3f q(s[1] * e1[2] - s[2] * e1[1], s[2] * e1[0] - s[0] * e1[2], s[0] * e1[1] - s[1] * e1[0]);
        const float v = (r[0] * q[0] + r[1] * q[1] + r[2] * q[2]) / det;
        if (v < 0 || u + v > 1) { continue; }
        const float tt = (e2[0] * q[0] + e2[1] * q[1] + e2[2] * q[2]) / det;
        if (tt > 0.001F && tt < 0.999F) { return true; }
    }
    return false;
}

std::vector<Schritt> modellSchatten() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto modell = std::make_shared<std::string>();
    auto mitte = std::make_shared<karte::Vektor3f>(0.0F, 0.0F, 0.0F);   // Mitte seiner Grenzen (ohne Skala)
    auto punkt = std::make_shared<karte::Vektor3f>(0.0F, 0.0F, 0.0F);   // Messpunkt auf dem Boden
    auto gefunden = std::make_shared<bool>(false);
    auto f = std::make_shared<std::array<double, 2>>();
    auto schatten = std::make_shared<std::size_t>(0);   // Modelldreiecke in der Schattengeometrie
    add(tu("Stufe 9d/2: ein Modell aus dem Spiel (models/map_objects, .md3, das dickste der ersten 48)", [modell, mitte] {
        const auto& sp = ke::editor().karteSpiel;
        if (!sp) { return; }
        float bestes = 0;
        int n = 0;
        for (const std::string& p : sp->fs.dateien("models/map_objects/", "md3", 16)) {
            if (++n > 48) { break; }
            std::string bytes;
            karte::Modell m;
            if (!sp->fs.dateiLesen("models/map_objects/" + p, bytes) || !karte::md3Lesen(bytes, m)) { continue; }
            const float dicke = std::min({m.grenzen.extents[0], m.grenzen.extents[1], m.grenzen.extents[2]});
            if (dicke > bestes) {
                bestes = dicke;
                *modell = "models/map_objects/" + p;
                *mitte = m.grenzen.origin;
            }
        }
    }));
    const auto oeffnen = [modell, mitte, punkt](bool an) {
        return tu(std::string("Stufe 9d/2: Raum mit Punktlicht ") + (an ? "und misc_model darunter" : "ohne Modell"), [modell, mitte, punkt, an] {
            std::string extra;
            if (an) {
                char ort[96];
                std::snprintf(ort, sizeof(ort), "%g %g %g", static_cast<double>(-3.0F * (*mitte)[0]), static_cast<double>(-3.0F * (*mitte)[1]),
                              static_cast<double>(300.0F - 3.0F * (*mitte)[2]));
                extra = "{\n\"classname\" \"misc_model\"\n\"origin\" \"" + std::string(ort) + "\"\n\"modelscale\" \"3\"\n\"model\" \"" + *modell + "\"\n}\n";
            }
            karteDialogeZu();
            raumSchreiben(false, false, extra);
            (void)ke::karteOeffnen(raumKarte());
            ke::Editor& e = ke::editor();
            e.kameraE.modus = karte::KameraModus::Textur;
            e.echtLicht.an = true;
            e.echtLicht.backen = false;
            e.kamera.ursprung = karte::Vektor3f((*punkt)[0], (*punkt)[1], 120.0F);
            e.kamera.winkel = karte::Vektor3f(-89.0F, 0.0F, 0.0F);
        });
    };
    add(oeffnen(true));
    add(tu("Stufe 9d/2: Messpunkt im Schatten des Modells suchen (Strecke Licht - Boden trifft ein Dreieck)", [punkt, gefunden] {
        std::vector<karte::Vektor3f> d;
        karte::echtModellFlaechen(ke::editor().karte, [&d](const karte::EchtModellFlaeche& x) {
            if (x.wirft > 0) { d.insert(d.end(), x.ort.begin(), x.ort.end()); }
        });
        const karte::Vektor3f licht(0, 0, 400);
        for (int r = 0; r <= 160 && !*gefunden; r += 8) {
            for (int y = -r; y <= r && !*gefunden; y += 8) {
                for (int x = -r; x <= r && !*gefunden; x += 8) {
                    if (std::max(std::abs(x), std::abs(y)) != r) { continue; }
                    bool alle = true;
                    for (int dy = -8; dy <= 8 && alle; dy += 8) {
                        for (int dx = -8; dx <= 8 && alle; dx += 8) {
                            alle = streckeTrifft(licht, karte::Vektor3f(static_cast<float>(x + dx), static_cast<float>(y + dy), 0.0F), d);
                        }
                    }
                    if (alle) {
                        *punkt = karte::Vektor3f(static_cast<float>(x), static_cast<float>(y), 0.0F);
                        *gefunden = true;
                    }
                }
            }
        }
        ke::Editor& e = ke::editor();
        e.kamera.ursprung = karte::Vektor3f((*punkt)[0], (*punkt)[1], 120.0F);
    }));
    add(warteBis("Stufe 9d/2: Texturen (Modell)", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(1.5));
    add(tu("Stufe 9d/2: Boden am Messpunkt mit Modell", [f, schatten] {
        (*f)[1] = mittelFarbe(4)[1];
        *schatten = ke::editor().echtLicht.schattenModellDreiecke;
    }));
    add(foto("karteneditor_s9d_modellschatten"));
    add(oeffnen(false));
    add(warteBis("Stufe 9d/2: Texturen", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(1.5));
    add(tu("Stufe 9d/2: Boden am Messpunkt ohne Modell", [f] { (*f)[0] = mittelFarbe(4)[1]; }));
    add(pruefe("Stufe 9d/2: misc_model wirft Schatten wie in q3map2 (castShadows in der Welt 1)", [modell, punkt, gefunden, f, schatten] {
        diag::detail("Knopftest: Stufe 9d/2 - Modell " + *modell + ", Dreiecke in der Schattengeometrie " + std::to_string(*schatten) + ", Messpunkt " + (*gefunden ? "" : "(keiner) ") + std::to_string((*punkt)[0]) + " " +
                     std::to_string((*punkt)[1]) + ", Boden ohne " + std::to_string((*f)[0]) + ", mit " + std::to_string((*f)[1]));
        return !modell->empty() && *gefunden && *schatten > 0 && (*f)[1] < (*f)[0] - 5.0;
    }));
    return s;
}

// --- Punkt 5: Patch-Bruecke ----------------------------------------------------------------
std::vector<Schritt> bruecke() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(pruefe("Stufe 9d/5: Patch-Bruecke zwischen zwei Patches - Kanten passgenau, ein Rueckgaengig-Schritt", [] {
        ke::neueKarte();
        ke::Editor& e = ke::editor();
        karte::Karte& k = e.karte;
        auto welt = k.weltHolenOderAnlegen();
        const auto flach = [&](float x0) {
            auto p = std::make_shared<karte::Patch>();
            p->setzeGroesse(3, 3);
            std::vector<karte::PatchPunkt> pk(9);
            for (std::size_t r = 0; r < 3; ++r) {
                for (std::size_t c = 0; c < 3; ++c) {
                    pk[r * 3 + c].vertex = karte::Vektor3f(x0 + static_cast<float>(c) * 64.0F, static_cast<float>(r) * 64.0F, 0.0F);
                    pk[r * 3 + c].texcoord = karte::Vektor2f(static_cast<float>(c) * 0.5F, static_cast<float>(r) * 0.5F);
                }
            }
            p->punkteSetzen(pk);
            p->setzeShader("textures/yavin/temple_stone2");
            welt->kindEinfuegen(p);
            return p;
        };
        auto a = flach(0);
        auto b = flach(256);   // Luecke von 128 zwischen x 128 und x 256
        k.auswahl.leeren();
        k.auswahl.waehlen(welt, a);
        k.auswahl.waehlen(welt, b);
        const std::size_t vorher = welt->kinder().size();
        const std::size_t undoVorher = k.undo.undoAnzahl();
        karte::Protokoll pr;
        const bool ok = karte::patchBruecke(k, pr);
        std::shared_ptr<karte::Patch> neu;
        for (const auto& c : welt->kinder()) {
            if (c != a && c != b && c->art() == karte::Primitiv::Art::Patch) { neu = std::static_pointer_cast<karte::Patch>(c); }
        }
        bool passt = neu != nullptr && neu->breite() == 3 && neu->hoehe() == 3;
        if (passt) {
            // eine Zeile der Bruecke = die Kante x 128 von a, die andere = die Kante x 256 von b (gleiche Punkte)
            const auto zeile = [&neu](std::size_t r, float x) {
                bool g = true;
                for (std::size_t c = 0; c < 3; ++c) { g = g && neu->punkt(r, c).vertex[0] == x; }
                return g;
            };
            passt = (zeile(0, 128) && zeile(2, 256)) || (zeile(0, 256) && zeile(2, 128));
        }
        const std::size_t undoNachher = k.undo.undoAnzahl();
        (void)k.undo.undo();
        const std::size_t nachUndo = welt->kinder().size();
        (void)k.undo.redo();
        const std::size_t nachRedo = welt->kinder().size();
        diag::detail("Knopftest: Stufe 9d/5 - Bruecke " + std::string(ok ? "ja" : "nein") + ", Patches " + std::to_string(vorher) + " -> " +
                     std::to_string(nachRedo) + ", Kanten " + (passt ? "passgenau" : "falsch") + ", Undo-Schritte +" + std::to_string(undoNachher - undoVorher) +
                     ", nach Undo " + std::to_string(nachUndo));
        return ok && passt && undoNachher == undoVorher + 1 && nachUndo == vorher && nachRedo == vorher + 1;
    }));
    add(pruefe("Stufe 9d/5: Patch-Bruecke mit nur einem Patch: abgelehnt, nichts geaendert", [] {
        ke::neueKarte();
        karte::Karte& k = ke::editor().karte;
        auto welt = k.weltHolenOderAnlegen();
        auto p = std::make_shared<karte::Patch>();
        p->setzeGroesse(3, 3);
        welt->kindEinfuegen(p);
        k.auswahl.leeren();
        k.auswahl.waehlen(welt, p);
        karte::Protokoll pr;
        const std::size_t u = k.undo.undoAnzahl();
        const std::size_t vorher = welt->kinder().size();
        const bool ok = karte::patchBruecke(k, pr);
        diag::detail("Knopftest: Stufe 9d/5 - ein Patch: Bruecke " + std::string(ok ? "ja" : "nein") + ", Undo " + std::to_string(u) + " -> " +
                     std::to_string(k.undo.undoAnzahl()) + ", Kinder " + std::to_string(vorher) + " -> " + std::to_string(welt->kinder().size()));
        return !ok && k.undo.undoAnzahl() == u && welt->kinder().size() == vorher;
    }));
    return s;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    for (Schritt& x : modellSchatten()) { s.push_back(std::move(x)); }
    for (Schritt& x : nebel()) { s.push_back(std::move(x)); }
    for (Schritt& x : effekte()) { s.push_back(std::move(x)); }
    for (Schritt& x : portal()) { s.push_back(std::move(x)); }
    for (Schritt& x : bruecke()) { s.push_back(std::move(x)); }
    s.push_back(tu("Stufe 9d: Echtzeit-Licht aus, neue Karte", [] {
        ke::editor().echtLicht.an = false;
        ke::neueKarte();
    }));
    return s;
}

}  // namespace stufe9d
