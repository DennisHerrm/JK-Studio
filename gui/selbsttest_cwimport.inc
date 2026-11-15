// selbsttest_cwimport.inc - CW-IMPORT (BHED_EDITORTEST=karteneditor, BHED_KARTE_NUR=cwimport; Gruppe cwimport im vollen Lauf): Teil 2
// der Clone-Wars-Kette im Karten-Editor (gui/app_karte_cwimport.cpp, src/karte/cwimport.cpp, plaene/clonewars_kette.md).
//
// Punkt 1 Testfigur: ab Werk aus; View > Testfigur, Taste F7, Werkzeugleiste mit Rechtsklick; Kasten der Engine in 2D und Kamera, das
// Spielermodell des Spiels in der Pose BOTH_STAND1 / BOTH_CROUCH1; Shift+F7 setzt sie unter den Zeiger (2D: zwei Achsen, die Tiefe
// bleibt; Kamera: Fuesse auf der Flaeche). Foto der Ansichten (BHED_FOTOS; je Lauf mit BHED_D3D12=1 und =0).
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

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    for (Schritt& x : testfigur()) { s.push_back(std::move(x)); }
    return s;
}

}  // namespace cwimport
