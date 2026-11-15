// selbsttest_cwimport.inc - CW-IMPORT (BHED_EDITORTEST=karteneditor, BHED_KARTE_NUR=cwimport; Gruppe cwimport im vollen Lauf): Teil 2
// der Clone-Wars-Kette im Karten-Editor (gui/app_karte_cwimport.cpp, src/karte/cwimport.cpp, plaene/clonewars_kette.md).
//
// Punkt 1 Testfigur: ab Werk aus; View > Testfigur, Taste F7, Werkzeugleiste mit Rechtsklick; Kasten der Engine in 2D und Kamera, das
// Spielermodell des Spiels in der Pose BOTH_STAND1 / BOTH_CROUCH1; Shift+F7 setzt sie unter den Zeiger (2D: zwei Achsen, die Tiefe
// bleibt; Kamera: Fuesse auf der Flaeche). Foto der Ansichten (BHED_FOTOS; je Lauf mit BHED_D3D12=1 und =0).
// Punkt 2 Import: Vorlagen (Vorgabe wie bisher, BF3, BF2, Source, eigene), Umlauf umdrehen, Hoehe = Testfigur, Vorschau, gemerkt.
// Punkte 3 bis 6: Treppe -> Clip-Rampe mit q3map2-Bau vorher/nachher und Pmove (die Figur laeuft glatt), Sicht gegen Kollision (CW-PRUEF)
// schweigt, glatte Clip-Wand, Ebene Collision aus/ein, Raum skalieren mit Marker und Kommentar.
// Eingebunden in gui/selbsttest.cpp nach selbsttest_cwpruef.inc, aufgerufen aus selbsttest_handbuch.inc.
namespace cwimport {

namespace ke = karteneditor;

std::string raumKarte() { return karteTestPfad("cw_import.map"); }

// Ein gezeichneter, fester, undurchsichtiger Shader des Spiels unter textures/ (ohne "textures/"), wie CW-PRUEF ihn sucht
std::string festerShader() {
    const std::shared_ptr<ke::Spielkontext>& k = ke::spielWarten();
    if (!k) { return "system/caulk"; }
    for (const auto& [n, e] : k->q3map2.eintraege) {
        if (n.rfind("textures/", 0) != 0 || n.rfind("textures/common/", 0) == 0 || n.rfind("textures/system/", 0) == 0) { continue; }
        const karte::ShaderEintrag* inv = k->inventar.shaderFinden(n);
        if (inv == nullptr || inv->istStandard() || (inv->flags & (karte::QER_TRANS | karte::QER_ALPHATEST | karte::QER_NODRAW | karte::QER_FOG | karte::QER_LIQUID)) != 0) {
            continue;
        }
        const karte::ShaderSkript sk = karte::shaderSkriptLesen(e.text, n);
        karte::ShaderArt a;
        a.inhalt = sk.inhalt;
        if (sk.gefunden && !sk.durchsichtig && !sk.nichtZeichnen && !sk.himmel && !sk.cFluessig && !sk.cNebel && !sk.versatz && !sk.bilder.empty() &&
            !sk.autosprite && !sk.portal && sk.spielAbbruch.empty() && sk.cSolid && karte::spielerKollision(a)) {
            return n.substr(9);
        }
    }
    return "system/caulk";
}

// Ein Raum 1024 x 1024 x 384 aus einem festen Shader des Spiels, Spielerstart
std::string raumText(const std::string& fest) {
    const char* f = fest.c_str();
    std::string t = "// JK Studio self-test CW-IMPORT\n{\n\"classname\" \"worldspawn\"\n";
    t += stufe5::kastenText(-528, -528, -16, 528, 528, 0, f) + stufe5::kastenText(-528, -528, 384, 528, 528, 400, f) +
         stufe5::kastenText(-528, -528, 0, -512, 528, 384, f) + stufe5::kastenText(512, -528, 0, 528, 528, 384, f) +
         stufe5::kastenText(-512, -528, 0, 512, -512, 384, f) + stufe5::kastenText(-512, 512, 0, 512, 528, 384, f);
    return t + "}\n{\n\"classname\" \"info_player_start\"\n\"origin\" \"-400 0 24\"\n}\n";
}

// Die Mitte einer 2D-Ansicht (die erste ihrer Art)
ImVec2 xyMitte() {
    const ke::Editor& ed = ke::editor();
    for (const ke::XyFenster& f : ed.xy) {
        if (f.ansicht.breite > 0) {
            return ImVec2{f.bildX + static_cast<float>(f.ansicht.breite) * 0.5F + 40.0F, f.bildY + static_cast<float>(f.ansicht.hoehe) * 0.5F + 30.0F};
        }
    }
    return ImVec2{0, 0};
}

std::vector<Schritt> testfigur() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto fest = std::make_shared<std::string>();
    auto zaehler = std::make_shared<std::array<std::size_t, 3>>();
    add(pruefe("CW-IMPORT P1: Testfigur ab Werk aus (Vorgabe der Einstellungen)", [] {
        return !ke::Editor::TestfigurVorgabe().an && !ke::editor().testfigur.an;
    }));
    add(tu("CW-IMPORT P1: Raum schreiben und oeffnen", [fest] {
        *fest = festerShader();
        stufe2::testDateiSchreiben(raumKarte(), raumText(*fest));
        (void)ke::karteOeffnen(raumKarte());
        ke::editor().karte.auswahl.leeren();
    }));
    add(pause(0.3));
    // View > Testfigur
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.25));
    add(tu("Maus auf View > Testfigur", [] {
        if (const Element* el = finde(std::string(ke::trK(Str::KeCwFigurMenue)) + "###cwfigurmenue", "", 0, true)) { setzeMaus(el->rect.GetCenter()); }
    }));
    add(pause(0.4));
    add(pruefe("CW-IMPORT P1: View > Testfigur mit Zeigen (F7), Geduckt, Marken, Spielermodell, Rahmen quer, Hierher (Shift+F7)", [] {
        return finde(std::string(ke::trK(Str::KeCwFigur)) + "###cwfigur", "", 0, true) != nullptr &&
               finde(std::string(ke::trK(Str::KeCwFigurGeduckt)) + "###cwfigurgeduckt", "", 0, true) != nullptr &&
               finde(std::string(ke::trK(Str::KeCwFigurHier)) + "###cwfigurhier", "", 0, true) != nullptr;
    }));
    add(tu("Zaehler merken", [zaehler] { *zaehler = {ke::editor().testfigur.gezeichnet2d, ke::editor().testfigur.gezeichnetKamera, ke::editor().testfigur.xyAufrufe}; }));
    add(klick(std::string(ke::trK(Str::KeCwFigur)) + "###cwfigur", "", 0, true));
    add(warteBis("CW-IMPORT P1: Figur in 2D und Kamera gezeichnet, Spielermodell geladen", [zaehler] {
        const ke::TestfigurZustand& t = ke::editor().testfigur;
        // 2D: nur, wenn ueberhaupt eine 2D-Ansicht gezeichnet wird (Anordnungen frueherer Gruppen zeigen manchmal nur die Kamera)
        const bool zweiD = t.gezeichnet2d > (*zaehler)[0] || t.xyAufrufe == (*zaehler)[2];
        return t.an && zweiD && t.gezeichnetKamera > (*zaehler)[1] && t.modellDreiecke > 0;
    }, 600));
    add(tu("CW-IMPORT P1: Zaehler ins Protokoll", [zaehler] {
        const ke::TestfigurZustand& t = ke::editor().testfigur;
        const ke::KameraFenster& kf = ke::editor().kamFenster;
        diag::detail("Knopftest: CW-IMPORT Zaehler an " + std::to_string(t.an) + ", 2D " + std::to_string((*zaehler)[0]) + " -> " + std::to_string(t.gezeichnet2d) +
                     ", Kamera " + std::to_string((*zaehler)[1]) + " -> " + std::to_string(t.gezeichnetKamera) + ", Modell " + std::to_string(t.modellDreiecke) +
                     ", Kamerafenster " + std::to_string(kf.breite) + " x " + std::to_string(kf.hoehe) + ", maximiert " + std::to_string(ke::editor().maximiert));
    }));
    add(pruefe("CW-IMPORT P1: Kasten wie die Engine (32 x 32 x 64, Fuesse auf dem Boden z 0), Modell in BOTH_STAND1, Einstellung gespeichert", [] {
        const ke::TestfigurZustand& t = ke::editor().testfigur;
        karte::FigurLage l;
        l.ursprung = t.ursprung;
        const karte::FigurZeichnung z = karte::testfigurZeichnung(l, karte::spielfigur(ke::optimiererRegelwerk()), ke::editor().optimierer.masse);
        const auto it = g_app->settings.karte.find("BhedTestFigure");
        diag::detail("Knopftest: CW-IMPORT Figur bei " + std::to_string(t.ursprung.x()) + " " + std::to_string(t.ursprung.y()) + " " +
                     std::to_string(t.ursprung.z()) + ", Modell " + std::to_string(t.modellDreiecke) + " Dreiecke, Bild " + std::to_string(t.modellBild));
        return std::fabs(z.maxs.z() - z.mins.z() - 64.0F) < 0.01F && std::fabs(z.maxs.x() - z.mins.x() - 32.0F) < 0.01F && std::fabs(z.mins.z()) < 0.5F &&
               t.modellBild > 0 && it != g_app->settings.karte.end() && it->second == "1";
    }));
    add(foto("cw_figur_stehend"));
    // Shift+F7 ueber einer 2D-Ansicht: zwei Achsen unter dem Zeiger, die Tiefe bleibt
    auto vorher = std::make_shared<karte::Vektor3f>();
    add(tu("Maus auf eine 2D-Ansicht", [vorher] {
        *vorher = ke::editor().testfigur.ursprung;
        setzeMaus(xyMitte());
    }));
    add(pause(0.2));
    add(tasteMit(ImGuiMod_Shift, ImGuiKey_F7, "Shift+F7 (Testfigur hierher)"));
    add(pause(0.2));
    add(pruefe("CW-IMPORT P1: Shift+F7 setzt die Figur unter den Zeiger (die Tiefe der Ansicht bleibt)", [vorher] {
        const ke::Editor& ed = ke::editor();
        const ImVec2 m = xyMitte();
        for (const ke::XyFenster& f : ed.xy) {
            if (f.ansicht.breite <= 0) { continue; }
            karte::Vektor3f soll = f.ansicht.zuPunkt(static_cast<int>(m.x - f.bildX), static_cast<int>(m.y - f.bildY), ed.raster.einrastGroesse());
            const auto achse = static_cast<std::size_t>(karte::blickachse(f.ansicht.typ));
            soll[achse] = (*vorher)[achse];
            const karte::Vektor3f& ist = ed.testfigur.ursprung;
            return std::fabs(ist.x() - soll.x()) < 0.01F && std::fabs(ist.y() - soll.y()) < 0.01F && std::fabs(ist.z() - soll.z()) < 0.01F &&
                   (std::fabs(ist.x() - vorher->x()) > 1.0F || std::fabs(ist.y() - vorher->y()) > 1.0F || std::fabs(ist.z() - vorher->z()) > 1.0F);
        }
        return false;
    }));
    // Werkzeugleiste: der Knopf, Rechtsklick oeffnet das Menue; dort Geduckt
    add(pruefe("CW-IMPORT P1: Knopf Testfigur in der Werkzeugleiste (eingeschaltet)", [] {
        return finde("##knopfBhedTestFigure", "", 0, true) != nullptr && ke::angehakt("BhedTestFigure");
    }));
    add(klickAn("Rechtsklick auf den Knopf Testfigur", [] {
        const Element* el = finde("##knopfBhedTestFigure", "", 0, true);
        return el != nullptr ? el->rect.GetCenter() : ImVec2{0, 0};
    }, 1));
    add(pause(0.3));
    add(pruefe("CW-IMPORT P1: Rechtsklick auf den Knopf zeigt das Menue der Testfigur", [] {
        return finde(std::string(ke::trK(Str::KeCwFigurGeduckt)) + "###cwfigurgeduckt", "", 0, true) != nullptr;
    }));
    add(tu("Zaehler merken", [zaehler] { *zaehler = {ke::editor().testfigur.gezeichnet2d, ke::editor().testfigur.gezeichnetKamera, ke::editor().testfigur.xyAufrufe}; }));
    add(klick(std::string(ke::trK(Str::KeCwFigurGeduckt)) + "###cwfigurgeduckt", "", 0, true));
    add(warteBis("CW-IMPORT P1: geduckt gezeichnet, Modell in BOTH_CROUCH1 neu geladen", [zaehler] {
        const ke::TestfigurZustand& t = ke::editor().testfigur;
        return t.geduckt && t.gezeichnetKamera > (*zaehler)[1] && t.modellDreiecke > 0;
    }, 600));
    add(pruefe("CW-IMPORT P1: geduckt das gehaltene letzte Bild von BOTH_CROUCH1 (loopFrames -1: FREEZE), Kasten 40 hoch", [] {
        const std::shared_ptr<ke::Spielkontext>& k = ke::spielWarten();
        std::string cfg;
        if (!k || !k->fs.dateiLesen("models/players/_humanoid/animation.cfg", cfg)) { return false; }
        int soll = -1;
        for (const AnimEntry& a : parseAnimationCfg(cfg)) {
            if (a.name == "BOTH_CROUCH1") { soll = a.firstFrame + a.numFrames - 1; }
        }
        karte::FigurLage l;
        l.ursprung = ke::editor().testfigur.ursprung;
        l.geduckt = true;
        const karte::FigurZeichnung z = karte::testfigurZeichnung(l, karte::spielfigur(ke::optimiererRegelwerk()), ke::editor().optimierer.masse);
        diag::detail("Knopftest: CW-IMPORT geduckt Bild " + std::to_string(ke::editor().testfigur.modellBild) + ", soll " + std::to_string(soll));
        return soll > 0 && ke::editor().testfigur.modellBild == soll && std::fabs(z.maxs.z() - z.mins.z() - 40.0F) < 0.01F;
    }));
    add(tu("Figur in den Blick der Kamera (vor die Kamera)", [] {
        ke::TestfigurZustand& t = ke::editor().testfigur;
        t.gesetzt = false;
        t.an = false;
        ke::testfigurUmschalten();
    }));
    add(pause(0.5));
    add(foto("cw_figur_geduckt"));
    add(tasteMit(ImGuiKey_None, ImGuiKey_F7, "F7 (Testfigur aus)"));
    add(pause(0.2));
    add(pruefe("CW-IMPORT P1: F7 schaltet die Figur aus (Einstellung 0)", [] {
        const auto it = g_app->settings.karte.find("BhedTestFigure");
        return !ke::editor().testfigur.an && it != g_app->settings.karte.end() && it->second == "0";
    }));
    add(tu("CW-IMPORT P1: Figur wie ab Werk", [] {
        ke::editor().testfigur = ke::Editor::TestfigurVorgabe();
        ke::einstellungenSpeichern();
    }));
    return s;
}


// --- Punkt 2: Import mit Massstab und Achsen --------------------------------------------------------------------------------------------

std::function<ImVec2()> klappOrtCw(const std::string& id) {
    return [id]() {
        for (const auto& k : ke::editor().klappOrte) {
            if (k.id == id) { return ImVec2{(k.x0 + k.x1) * 0.5F, (k.y0 + k.y1) * 0.5F}; }
        }
        return ImVec2{-50, -50};
    };
}

// Ein Quader 10 x 20 x 30 in der .obj (x 0..10, y 0..20, z 0..30), je Seite zwei Dreiecke gegen den Uhrzeiger von aussen
std::string quaderObj() {
    std::string t = "# JK Studio self-test CW-IMPORT\n";
    const float p[8][3] = {{0, 0, 0}, {10, 0, 0}, {10, 20, 0}, {0, 20, 0}, {0, 0, 30}, {10, 0, 30}, {10, 20, 30}, {0, 20, 30}};
    char z[96];
    for (const auto& q : p) {
        std::snprintf(z, sizeof(z), "v %g %g %g\n", static_cast<double>(q[0]), static_cast<double>(q[1]), static_cast<double>(q[2]));
        t += z;
    }
    t += "f 1 3 2\nf 1 4 3\nf 5 6 7\nf 5 7 8\nf 1 2 6\nf 1 6 5\nf 2 3 7\nf 2 7 6\nf 3 4 8\nf 3 8 7\nf 4 1 5\nf 4 5 8\n";
    return t;
}

// Die Ausdehnung des gelesenen Netzes (Q3) und das erste Dreieck
std::array<double, 3> netzMass(std::array<std::uint32_t, 3>* erstes = nullptr) {
    double mn[3] = {1e30, 1e30, 1e30}, mx[3] = {-1e30, -1e30, -1e30};
    const karte::ImportNetz& n = ke::editor().geoImport.netz;
    for (const karte::ImportTeil& t : n.teile) {
        for (const karte::ImportPunkt& p : t.punkte) {
            const double v[3] = {p.x, p.y, p.z};
            for (int i = 0; i < 3; ++i) {
                mn[i] = std::min(mn[i], v[i]);
                mx[i] = std::max(mx[i], v[i]);
            }
        }
    }
    if (erstes != nullptr && !n.teile.empty() && !n.teile[0].dreiecke.empty()) { *erstes = n.teile[0].dreiecke[0]; }
    return {mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2]};
}
bool massGleich(const std::array<double, 3>& a, double x, double y, double z) {
    return std::fabs(a[0] - x) < 0.01 && std::fabs(a[1] - y) < 0.01 && std::fabs(a[2] - z) < 0.01;
}

std::vector<Schritt> importVorlagen() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto vorher = std::make_shared<std::array<std::uint32_t, 3>>();
    add(pruefe("CW-IMPORT P2: Vorlage ab Werk \"Vorgabe (wie bisher)\", Umlauf nicht umgedreht", [] {
        const ke::GeoImportZustand z;
        return z.vorlage == 0 && !z.umdrehen && ke::editor().geoImport.vorlage == 0 && !ke::editor().geoImport.umdrehen;
    }));
    add(tu("CW-IMPORT P2: Quader 10 x 20 x 30 (.obj) schreiben, Import-Dialog mit der Vorgabe", [] {
        stufe2::testDateiSchreiben(karteTestPfad("cw_quader.obj"), quaderObj());
        ke::GeoImportZustand& z = ke::editor().geoImport;
        z.skala = 1.0F;
        z.yOben = true;
        (void)ke::geoImportLaden(karteTestPfad("cw_quader.obj"));
    }));
    add(pause(0.3));
    add(pruefe("CW-IMPORT P2: Vorgabe wie bisher - Y oben: (x, -z, y), also 10 x 30 x 20", [] { return massGleich(netzMass(), 10, 30, 20); }));
    add(klickAn("Klappliste Vorlage", klappOrtCw("##givorlage")));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeCwVorBf2)), "", 0, true));
    add(pause(0.4));
    add(pruefe("CW-IMPORT P2: Battlefront 2 / Z oben - wie in der Datei, 10 x 20 x 30, gemerkt (BhedImportPreset 2)", [] {
        const auto it = g_app->settings.karte.find("BhedImportPreset");
        return massGleich(netzMass(), 10, 20, 30) && ke::editor().geoImport.vorlage == 2 && it != g_app->settings.karte.end() && it->second == "2";
    }));
    add(tu("Erstes Dreieck merken", [vorher] { (void)netzMass(vorher.get()); }));
    add(klick(std::string(ke::trK(Str::KeCwUmdrehen)) + "##giumdrehen", "", 0, true));
    add(pause(0.4));
    add(pruefe("CW-IMPORT P2: Umlauf umdrehen - das erste Dreieck laeuft andersherum, gemerkt (BhedImportFlip 1)", [vorher] {
        std::array<std::uint32_t, 3> jetzt{};
        (void)netzMass(&jetzt);
        const auto it = g_app->settings.karte.find("BhedImportFlip");
        return jetzt[0] == (*vorher)[0] && jetzt[1] == (*vorher)[2] && jetzt[2] == (*vorher)[1] && it != g_app->settings.karte.end() && it->second == "1";
    }));
    add(klick(std::string(ke::trK(Str::KeCwUmdrehen)) + "##giumdrehen", "", 0, true));
    add(klickAn("Klappliste Vorlage", klappOrtCw("##givorlage")));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeCwVorSource)), "", 0, true));
    add(pause(0.4));
    add(pruefe("CW-IMPORT P2: Source/GMod - Z oben, Faktor 64 / 72", [] {
        return std::fabs(ke::editor().geoImport.skala - 64.0F / 72.0F) < 1e-4F && massGleich(netzMass(), 10 * 64.0 / 72, 20 * 64.0 / 72, 30 * 64.0 / 72);
    }));
    add(klickAn("Klappliste Vorlage", klappOrtCw("##givorlage")));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeCwVorBf3)), "", 0, true));
    add(pause(0.4));
    add(pruefe("CW-IMPORT P2: Battlefield 3 / Y oben - (x, -z, y), die Skala bleibt", [] {
        return massGleich(netzMass(), 10 * 64.0 / 72, 30 * 64.0 / 72, 20 * 64.0 / 72);
    }));
    add(klick(std::string(ke::trK(Str::KeCwAufFigur)) + "##giauffigur", "", 0, true));
    add(pause(0.4));
    add(pruefe("CW-IMPORT P2: Hoehe = Testfigur - das Modell ist 64 hoch, die Vorschau zeigt es neben der Figur", [] {
        const std::array<double, 3> m = netzMass();
        return std::fabs(m[2] - 64.0) < 0.05 && ke::editor().geoImport.vorschauGezeichnet > 0;
    }));
    add(foto("cw_import_vorschau"));
    add(tu("CW-IMPORT P2: Vorlage wie ab Werk, Dialog zu", [] {
        ke::GeoImportZustand& z = ke::editor().geoImport;
        z.vorlage = 0;
        z.umdrehen = false;
        z.skala = 1.0F;
        z.offen = false;
        ke::einstellungenSpeichern();
    }));
    return s;
}

// --- Punkte 3 bis 6: Werkzeuge -------------------------------------------------------------------------------------------------------

// Raum mit Treppe: 4 Stufen 16 hoch, 24 tief, 128 breit in +x ab x 0 (Unterseiten caulk), oben ein Podest bis x 192, Spielerstart davor
std::string treppenText(const std::string& fest) {
    const char* f = fest.c_str();
    std::string t = raumText(fest);
    t = t.substr(0, t.find("}\n{\n\"classname\" \"info_player_start\""));   // ohne das Ende des worldspawn
    for (int i = 1; i <= 4; ++i) {
        t += stufe5::kastenText(24.0 * (i - 1), -64.0, 0.0, 24.0 * i, 64.0, 16.0 * i, f, 0, false);
    }
    t += stufe5::kastenText(96.0, -64.0, 0.0, 192.0, 64.0, 64.0, f, 0, false);
    // eine unregelmaessige Wand: drei Pinsel verschieden tief (Vorderseite bei y -400 + 8, -400 + 20, -400 + 4)
    t += stufe5::kastenText(-256.0, -512.0, 0.0, -192.0, -392.0, 160.0, f, 0, false);
    t += stufe5::kastenText(-192.0, -512.0, 0.0, -128.0, -380.0, 160.0, f, 0, false);
    t += stufe5::kastenText(-128.0, -512.0, 0.0, -64.0, -396.0, 160.0, f, 0, false);
    return t + "}\n{\n\"classname\" \"info_player_start\"\n\"origin\" \"-400 0 24\"\n}\n";
}
std::string treppenKarteCw() { return karteTestPfad("cw_treppe.map"); }

// Die Stufen (die Pinsel, deren Grenzen in x 0 ... 96 und y -64 ... 64 liegen) waehlen
void stufenWaehlenCw() {
    ke::Editor& ed = ke::editor();
    ed.karte.auswahl.leeren();
    const auto welt = ed.karte.welt();
    for (const auto& p : welt->kinder()) {
        if (p->art() != karte::Primitiv::Art::Pinsel) { continue; }
        const karte::Kasten& g = static_cast<const karte::Pinsel&>(*p).grenzen(ed.karte.kontext);
        if (g.origin.x() - g.extents.x() >= -0.5F && g.origin.x() + g.extents.x() <= 96.5F && std::fabs(g.extents.y() - 64.0F) < 0.5F) {
            ed.karte.auswahl.waehlen(welt, p);
        }
    }
}
void wandWaehlenCw() {
    ke::Editor& ed = ke::editor();
    ed.karte.auswahl.leeren();
    const auto welt = ed.karte.welt();
    for (const auto& p : welt->kinder()) {
        if (p->art() != karte::Primitiv::Art::Pinsel) { continue; }
        const karte::Kasten& g = static_cast<const karte::Pinsel&>(*p).grenzen(ed.karte.kontext);
        if (g.origin.x() + g.extents.x() <= -63.5F && g.origin.x() - g.extents.x() >= -256.5F && g.origin.y() < -380.0F && g.extents.y() < 100.0F) {
            ed.karte.auswahl.waehlen(welt, p);
        }
    }
}

// Die Spielfigur laeuft mit Pmove (navSpielerPmove: PM_WalkMove, PM_StepSlideMove ...) gegen die Kollision der gebauten .bsp die
// Treppe hinauf (forwardmove 127, 8 ms je Bild); gemessen: der groesste Hoehensprung in einem Bild und wie hoch sie ankommt
struct LaufMessung {
    bool ok = false;
    float groessterSprung = 0;   // z je Bild
    float oben = 0;              // z am Ende (Ursprung)
    int bilder = 0;
};
LaufMessung treppeHinauf(const std::string& bspPfad) {
    LaufMessung m;
    std::string daten;
    {
        std::ifstream f(std::filesystem::path(bspPfad), std::ios::binary);
        std::ostringstream o;
        o << f.rdbuf();
        daten = o.str();
    }
    KollisionsKarte k;
    if (!leseKollision(daten, k)) { return m; }
    const NavSpurFn spur = [&k](const NavVec& a, const NavVec& mins, const NavVec& maxs, const NavVec& b, int, std::uint32_t maske) {
        const KollisionsSpur t = kollisionsSpur(k, a.data(), mins.data(), maxs.data(), b.data(), maske);
        NavSpur r;
        r.allsolid = t.allsolid;
        r.startsolid = t.startsolid;
        r.fraction = t.fraction;
        r.endpos = t.endpos;
        r.normal = t.normale;
        r.contents = t.inhalt;
        r.entity = t.fraction < 1.0F ? kNavWeltEntity : kNavKeineEntity;
        return r;
    };
    NavSpieler s;
    s.ort = {-150.0F, 0.0F, 24.25F};
    s.gier = 0.0F;   // +x, die Treppe hinauf
    NavSpielerEingabe e;
    e.vor = 127;
    float letzte = s.ort[2];
    for (int i = 0; i < 400 && s.ort[0] < 150.0F; ++i) {
        navSpielerPmove(s, spur, e, 8);
        if (i > 20) { m.groessterSprung = std::max(m.groessterSprung, std::fabs(s.ort[2] - letzte)); }   // erst nach dem Anlaufen
        letzte = s.ort[2];
        m.bilder = i + 1;
    }
    m.oben = s.ort[2];
    m.ok = true;
    return m;
}
std::string bspVon(const std::string& map) { return map.substr(0, map.size() - 4) + ".bsp"; }

std::vector<Schritt> werkzeuge() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto vorher = std::make_shared<LaufMessung>();
    add(tu("CW-IMPORT P4: Raum mit Treppe schreiben, oeffnen, Build > Single: bsp stage (vorher, feste Stufen)", [] {
        stufe2::testDateiSchreiben(treppenKarteCw(), treppenText(festerShader()));
        (void)ke::karteOeffnen(treppenKarteCw());
        stufe3::q3map2Bereitstellen();
        ke::konsoleLeeren();
        ke::bauMenueSicherstellen();
        cwpruef::bauEintragStarten("Single: bsp stage");
    }));
    add(stufe8::warteSek("CW-IMPORT P4: Bau fertig (vorher)", [] { return !ke::bauLaeuft(); }, 300));
    add(pruefe("CW-IMPORT P4: vorher (feste Stufen, q3map2) - die Figur springt je Stufe hoch (PM_StepSlideMove), oben auf 88", [vorher] {
        *vorher = treppeHinauf(bspVon(treppenKarteCw()));
        diag::detail("Knopftest: CW-IMPORT vorher groesster Sprung " + std::to_string(vorher->groessterSprung) + ", oben " + std::to_string(vorher->oben) +
                     ", Bilder " + std::to_string(vorher->bilder));
        return vorher->ok && vorher->groessterSprung >= 15.0F && std::fabs(vorher->oben - 88.0F) < 1.0F;
    }));
    add(pruefe("CW-IMPORT P6: vor den Werkzeugen gibt es keine Ebene Collision", [] { return karte::kollisionsEbeneFinden(ke::editor().karte) < 0; }));
    // Punkt 4 per Menue: Modify > Clone-Wars-Werkzeuge > Treppe zu Clip-Rampe
    add(tu("CW-IMPORT P4: Stufen waehlen", [] { stufenWaehlenCw(); }));
    add(klick(ke::trK(Str::KeLModify), "###main"));
    add(pause(0.25));
    add(tu("Maus auf Modify > Clone-Wars-Werkzeuge", [] {
        if (const Element* el = finde(std::string(ke::trK(Str::KeCwWzMenue)) + "###cwwerkzeuge", "", 0, true)) { setzeMaus(el->rect.GetCenter()); }
    }));
    add(pause(0.4));
    add(klick(std::string(ke::trK(Str::KeCwRampe)) + "###cwrampe", "", 0, true));
    add(pause(0.3));
    add(pruefe("CW-IMPORT P4/P6: eine Clip-Rampe aus textures/system/clip in der Ebene Collision, die 4 Stufen nicht fest (system/nodraw unten)", [] {
        const ke::Editor& ed = ke::editor();
        const int ebene = karte::kollisionsEbeneFinden(ed.karte);
        int rampen = 0;
        int nodraw = 0;
        for (const auto& p : ed.karte.welt()->kinder()) {
            if (p->art() != karte::Primitiv::Art::Pinsel) { continue; }
            const auto& pin = static_cast<const karte::Pinsel&>(*p);
            bool clip = true;
            for (const auto& f : pin.flaechen()) {
                clip = clip && f->shader() == "textures/system/clip";
                nodraw += f->shader() == "textures/system/nodraw" ? 1 : 0;
            }
            rampen += (clip && p->schicht == ebene) ? 1 : 0;
        }
        diag::detail("Knopftest: CW-IMPORT Rampe: Ebene " + std::to_string(ebene) + ", Clip-Pinsel " + std::to_string(rampen) + ", nodraw-Seiten " +
                     std::to_string(nodraw) + ", " + std::to_string(ke::cwWerkzeugStand().rampen) + " - " + ke::cwWerkzeugStand().meldung);
        return ebene >= 0 && rampen == 1 && nodraw == 4;
    }));
    add(tu("CW-IMPORT P4: Karte speichern, Build > Single: bsp stage (nachher)", [] {
        (void)ke::karteSpeichernUnter(treppenKarteCw());
        ke::konsoleLeeren();
        cwpruef::bauEintragStarten("Single: bsp stage");
    }));
    add(stufe8::warteSek("CW-IMPORT P4: Bau fertig (nachher)", [] { return !ke::bauLaeuft(); }, 300));
    add(pruefe("CW-IMPORT P4: nachher (q3map2) - die Figur laeuft glatt hinauf (kein Sprung ueber 4 je Bild), oben auf derselben Hoehe", [vorher] {
        const LaufMessung m = treppeHinauf(bspVon(treppenKarteCw()));
        diag::detail("Knopftest: CW-IMPORT nachher groesster Sprung " + std::to_string(m.groessterSprung) + ", oben " + std::to_string(m.oben) +
                     ", Bilder " + std::to_string(m.bilder) + " (vorher " + std::to_string(vorher->groessterSprung) + ")");
        return m.ok && m.groessterSprung < 4.0F && std::fabs(m.oben - vorher->oben) < 1.0F;
    }));
    add(tu("CW-IMPORT P4: Optimierer mit Pruefprofil Clone-Wars-Kette", [] {
        ke::editor().optimierer.clonewars = true;
        ke::optimiererOeffnen();
    }));
    add(stufe8::warteSek("CW-IMPORT P4: Optimierer fertig", [] { return !ke::optimiererLaeuft() && ke::optimiererBlick().hatErgebnis; }, 120));
    add(pruefe("CW-IMPORT P4: die Pruefung Sicht gegen Kollision (CW-PRUEF) meldet an der Treppe nichts", [] {
        diag::detail("Knopftest: CW-IMPORT Treppe - Befunde " + stufe5::befundListe());
        return !stufe5::hatBefund("laufohnekollision") && !stufe5::hatBefund("kollisionsabstand");
    }));
    add(tu("CW-IMPORT P4: Optimierer zu, Profil wie ab Werk", [] {
        ke::optimiererSchliessen();
        ke::editor().optimierer.clonewars = false;
    }));
    add(foto("cw_rampe"));
    // Punkt 5: glatte Clip-Wand (Ebene) ueber das Rechtsklickmenue der Kamera-Seite: hier ueber dieselbe Funktion wie der Menuepunkt
    add(tu("CW-IMPORT P5: Wand waehlen, Kamera davor", [] {
        wandWaehlenCw();
        ke::Editor& ed = ke::editor();
        ed.kamera.ursprung = karte::Vektor3f(-160, -150, 80);
    }));
    add(klick(ke::trK(Str::KeLModify), "###main"));
    add(pause(0.25));
    add(tu("Maus auf Modify > Clone-Wars-Werkzeuge", [] {
        if (const Element* el = finde(std::string(ke::trK(Str::KeCwWzMenue)) + "###cwwerkzeuge", "", 0, true)) { setzeMaus(el->rect.GetCenter()); }
    }));
    add(pause(0.4));
    add(klick(std::string(ke::trK(Str::KeCwWandEbene)) + "###cwwandebene", "", 0, true));
    add(pause(0.3));
    add(pruefe("CW-IMPORT P5: Clip-Wand vorn am weitesten vorstehenden Punkt (y -380), so lang wie die Wand (x -256 ... -64), in der Ebene Collision", [] {
        const ke::Editor& ed = ke::editor();
        for (const auto& a : ed.karte.auswahl.elemente()) {
            const auto p = a.primitiv.lock();
            if (!p || p->art() != karte::Primitiv::Art::Pinsel) { continue; }
            const karte::Kasten& g = static_cast<const karte::Pinsel&>(*p).grenzen(ed.karte.kontext);
            return std::fabs(g.origin.y() + g.extents.y() + 380.0F) < 0.01F && std::fabs(g.origin.x() - g.extents.x() + 256.0F) < 0.01F &&
                   std::fabs(g.origin.x() + g.extents.x() + 64.0F) < 0.01F && p->schicht == karte::kollisionsEbeneFinden(ed.karte);
        }
        return false;
    }));
    // Punkt 6: Ebene Collision aus- und einblenden (View-Menue der Werkzeuge)
    add(tu("CW-IMPORT P6: Ebene Collision ausblenden", [] { ke::cwKollisionZeigen(false); }));
    add(pruefe("CW-IMPORT P6: ausgeblendet - die Clip-Pinsel sind versteckt (ebeneVersteckt)", [] {
        const ke::Editor& ed = ke::editor();
        const int ebene = karte::kollisionsEbeneFinden(ed.karte);
        bool alle = true;
        int n = 0;
        for (const auto& p : ed.karte.welt()->kinder()) {
            if (p->schicht != ebene) { continue; }
            ++n;
            alle = alle && p->ebeneVersteckt;
        }
        return n == 2 && alle && !ke::cwKollisionSichtbar();
    }));
    add(tu("CW-IMPORT P6: Ebene Collision einblenden", [] { ke::cwKollisionZeigen(true); }));
    // Punkt 3: Raum skalieren (Dialog), Marker und Kommentar
    add(tu("CW-IMPORT P3: Stufen waehlen, Modify > Clone-Wars-Werkzeuge > Raum skalieren...", [] { stufenWaehlenCw(); }));
    add(klick(ke::trK(Str::KeLModify), "###main"));
    add(pause(0.25));
    add(tu("Maus auf Modify > Clone-Wars-Werkzeuge", [] {
        if (const Element* el = finde(std::string(ke::trK(Str::KeCwWzMenue)) + "###cwwerkzeuge", "", 0, true)) { setzeMaus(el->rect.GetCenter()); }
    }));
    add(pause(0.4));
    add(klick(std::string(ke::trK(Str::KeCwRaum)) + "###cwraum", "", 0, true));
    add(pause(0.4));
    add(foto("cw_raum_dialog"));
    add(klick(std::string(ke::trK(Str::KeCwRaumOk)) + "##cwraumok", "", 0, true));
    add(pause(0.3));
    add(pruefe("CW-IMPORT P3: info_notnull mit cw_scale 1.2 in der Karte, Kommentar \"// cw_scale 1.2\" in der gespeicherten .map", [] {
        const ke::Editor& ed = ke::editor();
        bool marker = false;
        for (const auto& en : ed.karte.entitaeten()) { marker = marker || (en->klassenname() == "info_notnull" && en->wert("cw_scale") == "1.2"); }
        (void)ke::karteSpeichernUnter(treppenKarteCw());
        std::ifstream f(std::filesystem::path(treppenKarteCw()), std::ios::binary);
        std::ostringstream o;
        o << f.rdbuf();
        return marker && o.str().find("// cw_scale 1.2") != std::string::npos && ke::cwWerkzeugStand().raeume > 0;
    }));
    add(tu("CW-IMPORT: neue Karte", [] {
        ke::editor().karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
    }));
    return s;
}

// Die Handbuchbilder (BHED_EDITORTEST=handbuch, BHED_HB_NUR=cwimport): ganzes Fenster (mindestens 1920 breit)
std::vector<Schritt> handbuchBilder() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const auto voll = [] { return ImRect{0.0F, 0.0F, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y}; };
    const auto kameraSetzen = [] {   // nah an Figur und Treppe (die Treppe x 0 ... 192, die Figur davor)
        ke::Editor& e = ke::editor();
        e.kamera.ursprung = karte::Vektor3f(-190.0F, -130.0F, 84.0F);
        e.kamera.winkel = karte::Vektor3f(-11.0F, 33.0F, 0.0F);
    };
    // die Maus aus allen Tooltips (leere Flaeche des Texturbrowsers), die 2D-Ansichten auf die Treppe und zwei Stufen naeher
    const auto bildRuhig = [] {
        stufenWaehlenCw();
        (void)ke::ausfuehren("XYFocusOnSelected");
        (void)ke::ausfuehren("ZoomIn");
        (void)ke::ausfuehren("ZoomIn");
        setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x * 0.25F, ImGui::GetIO().DisplaySize.y - 6.0F});   // Statuszeile links: kein Tooltip
    };
    for (Schritt& x : stufe9c::einrichten()) { add(std::move(x)); }
    add(tu("Handbuch CW-IMPORT: Raum mit Treppe, Kamera, Testfigur vor der Treppe", [kameraSetzen] {
        karteDialogeZu();
        stufe2::testDateiSchreiben(treppenKarteCw(), treppenText(festerShader()));
        (void)ke::karteOeffnen(treppenKarteCw());
        ke::editor().karte.auswahl.leeren();
        kameraSetzen();
        ke::TestfigurZustand& t = ke::editor().testfigur;
        t = ke::Editor::TestfigurVorgabe();
        t.an = true;
        t.gesetzt = true;
        t.ursprung = karte::Vektor3f(-48.0F, 0.0F, 24.0F);
        ++ke::editor().sichtStand;
    }));
    add(warteBis("Handbuch CW-IMPORT: Texturen und Spielermodell geladen", [] { return ke::kameraBilderFertig() && ke::editor().testfigur.modellDreiecke > 0; }, 6000));
    add(tu("Handbuch CW-IMPORT: Ansichten auf die Treppe, Auswahl weg", [bildRuhig] {
        bildRuhig();
        ke::editor().karte.auswahl.leeren();
    }));
    add(pause(0.6));
    add(hb::fotoTeil("h_cwimport_figur", voll));
    add(tu("Handbuch CW-IMPORT: Import-Dialog mit Vorlage Battlefield 3, Hoehe = Testfigur", [] {
        stufe2::testDateiSchreiben(karteTestPfad("cw_quader.obj"), quaderObj());
        ke::GeoImportZustand& z = ke::editor().geoImport;
        z.vorlage = static_cast<int>(karte::AchsVorlage::Battlefield3);
        z.skala = 3.2F;
        (void)ke::geoImportLaden(karteTestPfad("cw_quader.obj"));
        setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x * 0.25F, ImGui::GetIO().DisplaySize.y - 6.0F});   // Statuszeile links: kein Tooltip
    }));
    add(pause(0.8));
    add(hb::fotoTeil("h_cwimport_import", voll));
    add(tu("Handbuch CW-IMPORT: Dialog zu, Treppe zu Clip-Rampe, Figur auf der Rampe", [] {
        ke::GeoImportZustand& z = ke::editor().geoImport;
        z.offen = false;
        z.vorlage = 0;
        z.skala = 1.0F;
        stufenWaehlenCw();
        (void)ke::cwTreppeZuRampe();
        ke::TestfigurZustand& t = ke::editor().testfigur;
        t.ursprung = karte::Vektor3f(24.0F, 0.0F, 24.0F + 48.0F * 2.0F / 3.0F);
        ke::Editor& e = ke::editor();
        e.kamera.ursprung = karte::Vektor3f(-150.0F, -150.0F, 110.0F);
        e.kamera.winkel = karte::Vektor3f(-18.0F, 38.0F, 0.0F);
        setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x * 0.25F, ImGui::GetIO().DisplaySize.y - 6.0F});   // Statuszeile links: kein Tooltip
        ++e.sichtStand;
    }));
    add(pause(0.8));
    add(hb::fotoTeil("h_cwimport_rampe", voll));
    add(tu("Handbuch CW-IMPORT: Ende", [] {
        karteDialogeZu();
        ke::editor().testfigur = ke::Editor::TestfigurVorgabe();
        ke::editor().karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
        g_app->leftMode = 0;
    }));
    add(pause(0.3));
    return s;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    for (Schritt& x : testfigur()) { s.push_back(std::move(x)); }
    for (Schritt& x : importVorlagen()) { s.push_back(std::move(x)); }
    for (Schritt& x : werkzeuge()) { s.push_back(std::move(x)); }
    return s;
}

}  // namespace cwimport
