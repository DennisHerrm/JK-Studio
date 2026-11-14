// selbsttest_cwpruef.inc - CW-PRUEF (BHED_EDITORTEST=karteneditor, BHED_KARTE_NUR=cwpruef): die Pruefungen der Clone-Wars-Kette im
// Optimierer des Karten-Editors (src/karte/optimierer_cw.cpp, plaene/clonewars_kette.md).
//
// Erst ein sauberer Raum (keine der neuen Pruefungen meldet etwas), dann ein Raum mit je einem Fehler: ein nicht fester, sichtbarer
// Deckel ueber einer Grube (laufohnekollision), eine nicht feste Platte 4 ueber dem Boden (kollisionsabstand), eine duenne Platte
// 1 ueber dem Boden (fastdeckungsgleich), eine Stufe 24 (stufehoch), eine kleine func_door (tuerklein), ein Gang 120 x 80
// (gangeng), ein schwebendes Modell des Spiels (requisitschwebt) und ein Clip-Pinsel frei im Raum (unsichtbarekollision: die
// Spielfigur kommt vom Start aus hin; nach dem Bau dasselbe mit den Clustern der .bsp). Mit echten Klicks: der Befund waehlt sein Objekt aus und zentriert die
// Ansichten; der JSON-Bericht ist gueltiges JSON mit Koordinaten und den Shadern der Karte; die Masse stehen in den Einstellungen.
// Shader und Modell kommen aus dem Spiel (kein Name als Bedingung: gesucht wird nach den Eigenschaften).
// Eingebunden in gui/selbsttest.cpp nach selbsttest_rest8.inc, aufgerufen aus selbsttest_handbuch.inc.
namespace cwpruef {

namespace ke = karteneditor;

struct Stand {
    std::string fest;      // ein gezeichneter, fester, undurchsichtiger Shader des Spiels (ohne "textures/")
    std::string luft;      // ein gezeichneter, nicht fester, undurchsichtiger Shader des Spiels
    std::string modell;    // ein .md3 des Spiels
    std::string json;      // der geschriebene JSON-Bericht
};

std::string sauberKarte() { return karteTestPfad("cw_sauber.map"); }
std::string fehlerKarte() { return karteTestPfad("cw_fehler.map"); }

// Ein geschlossener Raum 2048 x 1024 x 512 aus dem festen Shader, Spielerstart; mitGrube: im Boden ein Loch 256 x 256 ueber einer
// Grube 256 tief (x 100 ... 356, y -400 ... -144)
std::string raum(const std::string& fest, bool mitGrube = false) {
    const char* f = fest.c_str();
    std::string boden = stufe5::kastenText(-1040, -528, -16, 1040, 528, 0, f);
    if (mitGrube) {
        boden = stufe5::kastenText(-1040, -528, -16, 100, 528, 0, f) + stufe5::kastenText(356, -528, -16, 1040, 528, 0, f) +
                stufe5::kastenText(100, -528, -16, 356, -400, 0, f) + stufe5::kastenText(100, -144, -16, 356, 528, 0, f) +
                stufe5::kastenText(84, -416, -288, 372, -128, -272, f) + stufe5::kastenText(84, -416, -272, 100, -128, -16, f) +
                stufe5::kastenText(356, -416, -272, 372, -128, -16, f) + stufe5::kastenText(100, -416, -272, 356, -400, -16, f) +
                stufe5::kastenText(100, -144, -272, 356, -128, -16, f);
    }
    return boden + stufe5::kastenText(-1040, -528, 512, 1040, 528, 528, f) +
           stufe5::kastenText(-1040, -528, 0, -1024, 528, 512, f) + stufe5::kastenText(1024, -528, 0, 1040, 528, 512, f) +
           stufe5::kastenText(-1024, -528, 0, 1024, -512, 512, f) + stufe5::kastenText(-1024, 512, 0, 1024, 528, 512, f);
}
std::string start() { return "{\n\"classname\" \"info_player_start\"\n\"origin\" \"-900 0 24\"\n}\n"; }

std::string fehlerText(const Stand& st) {
    const char* f = st.fest.c_str();
    const char* l = st.luft.c_str();
    std::string t = "// JK Studio self-test CW-PRUEF\n{\n\"classname\" \"worldspawn\"\n" + raum(st.fest, true);
    t += stufe5::kastenText(100, -400, -16, 356, -144, 0, l);        // laufohnekollision: nicht fester Deckel ueber der Grube
    t += stufe5::kastenText(300, -100, 0, 400, 100, 4, l);           // kollisionsabstand: 4 ueber der Kollision
    t += stufe5::kastenText(300, 150, 0.5, 400, 250, 1, l);          // fastdeckungsgleich: 1 ueber dem Boden, ein anderer Shader
    t += stufe5::kastenText(-700, -400, 0, -500, -200, 24, f);       // stufehoch: 24
    t += stufe5::kastenText(500, 300, 0, 1000, 340, 120, f);         // gangeng: Waende 80 auseinander, Decke bei 120
    t += stufe5::kastenText(500, 420, 0, 1000, 460, 120, f);
    t += stufe5::kastenText(500, 300, 120, 1000, 460, 136, f);
    t += stufe5::kastenText(-300, 200, 0, -200, 300, 96, "common/clip");   // unsichtbarekollision: Clip frei im Raum
    t += "}\n{\n\"classname\" \"func_door\"\n";
    t += stufe5::kastenText(0, -400, 0, 8, -352, 96, f);             // tuerklein: 96 x 48
    t += "}\n";
    if (!st.modell.empty()) {   // requisitschwebt: frei im Raum, 160 ueber dem Boden
        t += "{\n\"classname\" \"misc_model_static\"\n\"origin\" \"-400 0 160\"\n\"model\" \"" + st.modell + "\"\n}\n";
    }
    return t + start();
}

// Ein kleiner JSON-Leser (Syntax pruefen): Objekte, Felder, Zeichenketten, Zahlen, true/false/null
struct JsonLeser {
    const std::string& t;
    std::size_t i = 0;
    void leer() {
        while (i < t.size() && (t[i] == ' ' || t[i] == '\n' || t[i] == '\r' || t[i] == '\t')) { ++i; }
    }
    bool text() {
        if (i >= t.size() || t[i] != '"') { return false; }
        for (++i; i < t.size(); ++i) {
            if (t[i] == '\\') {
                ++i;
                continue;
            }
            if (t[i] == '"') {
                ++i;
                return true;
            }
            if (static_cast<unsigned char>(t[i]) < 0x20) { return false; }
        }
        return false;
    }
    bool wert() {
        leer();
        if (i >= t.size()) { return false; }
        const char c = t[i];
        if (c == '{' || c == '[') {
            const char zu = c == '{' ? '}' : ']';
            ++i;
            leer();
            if (i < t.size() && t[i] == zu) {
                ++i;
                return true;
            }
            for (;;) {
                if (c == '{') {
                    leer();
                    if (!text()) { return false; }
                    leer();
                    if (i >= t.size() || t[i] != ':') { return false; }
                    ++i;
                }
                if (!wert()) { return false; }
                leer();
                if (i < t.size() && t[i] == ',') {
                    ++i;
                    continue;
                }
                if (i < t.size() && t[i] == zu) {
                    ++i;
                    return true;
                }
                return false;
            }
        }
        if (c == '"') { return text(); }
        for (const char* w : {"true", "false", "null"}) {
            if (t.compare(i, std::strlen(w), w) == 0) {
                i += std::strlen(w);
                return true;
            }
        }
        const std::size_t a = i;
        while (i < t.size() && (std::isdigit(static_cast<unsigned char>(t[i])) != 0 || t[i] == '-' || t[i] == '.' || t[i] == 'e' || t[i] == 'E' || t[i] == '+')) {
            ++i;
        }
        return i > a;
    }
    bool ganz() {
        if (!wert()) { return false; }
        leer();
        return i == t.size();
    }
};

const char* const kNeu[] = {"laufohnekollision", "unsichtbarekollision", "kollisionsabstand", "fastdeckungsgleich", "stufehoch", "tuerklein",
                            "durchgangeng", "gangeng", "bildnpot", "bildfehlt", "bildgroesse", "modellteilflaeche", "modellweltkoord",
                            "md3grenzen", "requisitschwebt", "requisitsteckt"};

// Build > <name> starten
void bauEintragStarten(const char* name) {
    const auto& v = ke::editor().bauProjekt.eintraege;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i].name == name) {
            ke::bauStarten(i);
            return;
        }
    }
    meldeFehler(std::string("CW-PRUEF: Bau-Eintrag ") + name + " fehlt");
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<Stand>();
    auto laeufe = std::make_shared<int>(0);
    add(pruefe("CW-PRUEF: Shader des Spiels (fest und nicht fest, beide gezeichnet und undurchsichtig) und ein .md3 gefunden", [st] {
        const std::shared_ptr<ke::Spielkontext>& k = ke::spielWarten();
        if (!k) { return false; }
        for (const auto& [n, e] : k->q3map2.eintraege) {
            if (n.rfind("textures/", 0) != 0 || n.rfind("textures/common/", 0) == 0 || n.rfind("textures/system/", 0) == 0) { continue; }
            const karte::ShaderEintrag* inv = k->inventar.shaderFinden(n);
            if (inv == nullptr || inv->istStandard() || (inv->flags & (karte::QER_TRANS | karte::QER_ALPHATEST | karte::QER_NODRAW | karte::QER_FOG | karte::QER_LIQUID)) != 0) {
                continue;
            }
            const karte::ShaderSkript sk = karte::shaderSkriptLesen(e.text, n);
            if (!sk.gefunden || sk.durchsichtig || sk.nichtZeichnen || sk.himmel || sk.cFluessig || sk.cNebel || sk.versatz || sk.bilder.empty() ||
                sk.autosprite || sk.portal || !sk.spielAbbruch.empty()) {
                continue;
            }
            karte::ShaderArt a;
            a.inhalt = sk.inhalt;
            const bool kollision = karte::spielerKollision(a);
            if (kollision && sk.cSolid && st->fest.empty()) { st->fest = n.substr(9); }
            if (!kollision && st->luft.empty()) { st->luft = n.substr(9); }
            if (!st->fest.empty() && !st->luft.empty()) { break; }
        }
        // ein kleines Modell (hoechstens 64 je Seite), damit es frei im Raum haengt
        int versuche = 0;
        for (const std::string& m : k->fs.dateien("models/map_objects/", "md3", 3)) {
            if (++versuche > 60) { break; }
            const karte::ModellAnsicht an = ke::kartenModellPfad("models/map_objects/" + m);
            if (!an.modell || an.modell->null || !karte::kastenGueltig(an.modell->grenzen)) { continue; }
            const karte::Vektor3f& ex = an.modell->grenzen.extents;
            if (ex[0] > 0.5F && ex[1] > 0.5F && ex[2] > 0.5F && ex[0] <= 32 && ex[1] <= 32 && ex[2] <= 32) {
                st->modell = "models/map_objects/" + m;
                break;
            }
        }
        diag::detail("Knopftest: CW-PRUEF - fest " + st->fest + ", nicht fest " + st->luft + ", Modell " + st->modell);
        return !st->fest.empty() && !st->luft.empty() && !st->modell.empty();
    }));
    // 1. der saubere Raum: keine der neuen Pruefungen meldet etwas
    add(tu("CW-PRUEF: sauberer Raum schreiben und oeffnen, Optimierer", [st, laeufe] {
        stufe2::testDateiSchreiben(sauberKarte(), "// JK Studio self-test CW-PRUEF\n{\n\"classname\" \"worldspawn\"\n" + raum(st->fest) + "}\n" + start());
        (void)ke::karteOeffnen(sauberKarte());
        ke::editor().karte.auswahl.leeren();
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererOeffnen();
    }));
    add(stufe5::warteOptimierer("CW-PRUEF: Optimierer fertig (sauber)", laeufe));
    add(pruefe("CW-PRUEF: sauberer Raum - keine der 16 neuen Pruefungen meldet etwas", [] {
        diag::detail("Knopftest: CW-PRUEF sauber - Befunde " + stufe5::befundListe());
        for (const char* id : kNeu) {
            if (stufe5::hatBefund(id)) { return false; }
        }
        return ke::optimiererBlick().hatErgebnis;
    }));
    // CW-IMPORT (shank 14.11.: drin, aber nicht Standard): ohne das Pruefprofil meldet der Fehlerraum keine der 16 Pruefungen, und der
    // Knopf JSON fehlt; das Profil kommt per Klick auf Build > Clone-Wars-Kette
    auto profilVorher = std::make_shared<bool>(false);
    add(tu("CW-IMPORT: Pruefprofil Standard (Vorgabe), Raum mit Fehlern, Optimierer", [st, laeufe, profilVorher] {
        *profilVorher = ke::editor().optimierer.clonewars;
        ke::editor().optimierer.clonewars = false;
        stufe2::testDateiSchreiben(fehlerKarte(), fehlerText(*st));
        (void)ke::karteOeffnen(fehlerKarte());
        ke::editor().karte.auswahl.leeren();
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererStarten();
    }));
    add(stufe5::warteOptimierer("CW-IMPORT: Optimierer fertig (Profil Standard)", laeufe));
    add(pruefe("CW-IMPORT: Profil Standard - keine der 16 Clone-Wars-Pruefungen meldet etwas, alte Befunde da, kein Knopf JSON", [] {
        diag::detail("Knopftest: CW-IMPORT Profil Standard - Befunde " + stufe5::befundListe());
        for (const char* id : kNeu) {
            if (stufe5::hatBefund(id)) { return false; }
        }
        return ke::optimiererBlick().hatErgebnis && finde(std::string(ke::trK(Str::KeOptJson)) + "##optjson", "", 0, true) == nullptr;
    }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.25));
    add(pruefe("CW-IMPORT: Build-Menue - \"Clone-Wars-Kette (erweiterte Pruefungen)\" nach \"Optimizer...\", ohne Haken", [] {
        return finde(std::string(ke::trK(Str::KeCwMenue)) + "##bauclonewars", "", 0, true) != nullptr && !ke::editor().optimierer.clonewars;
    }));
    add(tu("CW-IMPORT: Klick auf Build > Clone-Wars-Kette", [laeufe] { *laeufe = ke::optimiererBlick().laeufe; }));
    add(klick(std::string(ke::trK(Str::KeCwMenue)) + "##bauclonewars", "", 0, true));
    add(stufe5::warteOptimierer("CW-IMPORT: der offene Optimierer prueft neu (Profil Clone-Wars-Kette)", laeufe));
    add(pruefe("CW-IMPORT: Profil an (Einstellung OptimizerCloneWars 1), Knopf JSON da", [] {
        const auto it = g_app->settings.karte.find("OptimizerCloneWars");
        return ke::editor().optimierer.clonewars && it != g_app->settings.karte.end() && it->second == "1" && finde(std::string(ke::trK(Str::KeOptJson)) + "##optjson", "", 0, true) != nullptr;
    }));
    // 2. der Raum mit den Fehlern
    add(tu("CW-PRUEF: Raum mit Fehlern schreiben und oeffnen, Optimierer", [st, laeufe] {
        stufe2::testDateiSchreiben(fehlerKarte(), fehlerText(*st));
        (void)ke::karteOeffnen(fehlerKarte());
        ke::editor().karte.auswahl.leeren();
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererStarten();
    }));
    add(stufe5::warteOptimierer("CW-PRUEF: Optimierer fertig (Fehler)", laeufe));
    add(pruefe("CW-PRUEF: Befunde laufohnekollision, kollisionsabstand, fastdeckungsgleich, stufehoch, tuerklein 1, gangeng, requisitschwebt, "
               "unsichtbarekollision (der Clip, vom Spielerstart aus erreichbar)",
               [] {
                   long long tuer = 0;
                   diag::detail("Knopftest: CW-PRUEF Fehler - Befunde " + stufe5::befundListe());
                   return stufe5::hatBefund("laufohnekollision") && stufe5::hatBefund("kollisionsabstand") && stufe5::hatBefund("fastdeckungsgleich") &&
                          stufe5::hatBefund("stufehoch") && stufe5::hatBefund("tuerklein", &tuer) && tuer == 1 && stufe5::hatBefund("gangeng") &&
                          stufe5::hatBefund("requisitschwebt") && stufe5::hatBefund("unsichtbarekollision");
               }));
    // ein echter Klick auf den Befund waehlt sein Objekt und zentriert die Ansichten
    add(tu("CW-PRUEF: Reiter Pruefungen", [] {
        ke::optimiererReiter(1);
        ke::editor().karte.auswahl.leeren();
    }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeOptPrStufe)) + "##bfstufehoch", "###karteoptimierer"));
    add(pause(0.3));
    add(pruefe("CW-PRUEF: Klick auf \"Stufe hoeher als STEPSIZE\" waehlt den Boden vor der Stufe (ein Pinsel)", [] {
        diag::detail("Knopftest: CW-PRUEF - Auswahl " + std::to_string(ke::editor().karte.auswahl.anzahl()));
        return ke::editor().karte.auswahl.anzahl() == 1;
    }));
    add(pruefe("CW-PRUEF: Knopf JSON... im Fenster des Optimierers", [] {
        return finde(std::string(ke::trK(Str::KeOptJson)) + "##optjson", "###karteoptimierer", 0, false) != nullptr;
    }));
    // der JSON-Bericht (direkt geschrieben - der Dateidialog gehoert Windows)
    add(pruefe("CW-PRUEF: JSON-Bericht gueltig, mit Koordinaten der Stufe (an der oberen Kante: x -700 ... -500, y -400 ... -200, z 24), den Grenzen und den Shadern der Karte", [st] {
        const std::string pfad = stufe2::laufDatei("cw_bericht.json");
        if (!ke::optimiererBerichtJsonSchreiben(pfad)) { return false; }
        st->json = dateiInhalt(pfad);
        JsonLeser j{st->json};
        const bool gueltig = j.ganz();
        const std::size_t stufe = st->json.find("\"id\": \"stufehoch\"");
        const std::size_t pos = stufe == std::string::npos ? std::string::npos : st->json.find("\"position\": [", stufe);
        float x = 0;
        float y = 0;
        float z = 1;
        const bool ort = pos != std::string::npos && std::sscanf(st->json.c_str() + pos + 13, "%f, %f, %f", &x, &y, &z) == 3;
        const bool shader = st->json.find("\"textures/" + st->fest + "\"") != std::string::npos;
        const bool grenzen = st->json.find("\"limits\": [") != std::string::npos && st->json.find("\"state\": \"ok\"") != std::string::npos;
        diag::detail("Knopftest: CW-PRUEF JSON - " + std::to_string(st->json.size()) + " Bytes, gueltig " + std::to_string(gueltig ? 1 : 0) +
                     ", Stufe bei " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z) + ", Shader " + std::to_string(shader ? 1 : 0));
        return gueltig && ort && x >= -700.5F && x <= -499.5F && y >= -400.5F && y <= -199.5F && std::fabs(z - 24.0F) < 0.5F && shader && grenzen;
    }));
    add(pruefe("CW-PRUEF: Textbericht nennt die Shader und Bilder der Karte", [st] {
        const std::string t = ke::optimiererBericht();
        return t.find(ke::trK(Str::KeOptBerichtTexturen)) != std::string::npos && t.find("textures/" + st->fest) != std::string::npos;
    }));
    // die Masse stehen in den Einstellungen und wirken
    add(tu("CW-PRUEF: Einstellung Stufe - Gangbreite 64 (statt 96), speichern und laden, Optimierer", [laeufe] {
        ke::editor().optimierer.masse.gangBreite = 64;
        ke::einstellungenSpeichern();
        ke::editor().optimierer.masse.gangBreite = 1;
        ke::einstellungenLaden();
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererStarten();
    }));
    add(stufe5::warteOptimierer("CW-PRUEF: Optimierer fertig (Gangbreite 64)", laeufe));
    add(pruefe("CW-PRUEF: Gangbreite 64 aus den Einstellungen (OptimizerCorridorWidth) - der Gang 120 x 80 ist breit genug, Hoehe 120 < 128 bleibt", [] {
        const auto it = g_app->settings.karte.find("OptimizerCorridorWidth");
        diag::detail("Knopftest: CW-PRUEF Gangbreite " + std::to_string(ke::editor().optimierer.masse.gangBreite) + " - " + stufe5::befundListe());
        return it != g_app->settings.karte.end() && it->second == "64" && ke::editor().optimierer.masse.gangBreite == 64 && stufe5::hatBefund("gangeng");
    }));
    add(tu("CW-PRUEF: Gangbreite zurueck auf 96", [] {
        ke::editor().optimierer.masse.gangBreite = 96;
        ke::einstellungenSpeichern();
    }));
    // 3. nach dem Bau (nur BSP): die unsichtbare Kollision
    add(tu("CW-PRUEF: Karte speichern, Build > Single: bsp stage", [] {
        ke::optimiererSchliessen();
        stufe3::q3map2Bereitstellen();
        (void)ke::karteSpeichernUnter(fehlerKarte());
        ke::konsoleLeeren();
        ke::bauMenueSicherstellen();
        bauEintragStarten("Single: bsp stage");
    }));
    add(stufe8::warteSek("CW-PRUEF: Bau fertig", [] { return !ke::bauLaeuft(); }, 300));
    add(tu("CW-PRUEF: Optimierer mit der .bsp", [laeufe] {
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererOeffnen();
    }));
    add(stufe5::warteOptimierer("CW-PRUEF: Optimierer fertig (mit .bsp)", laeufe));
    add(pruefe("CW-PRUEF: mit der .bsp meldet unsichtbarekollision den Clip-Pinsel (Wand), die Spielfigur kommt hin (Cluster)", [] {
        const ke::OptimiererBlick b = ke::optimiererBlick();
        diag::detail("Knopftest: CW-PRUEF mit .bsp (" + b.bsp + ") - Befunde " + stufe5::befundListe());
        return !b.bsp.empty() && stufe5::hatBefund("unsichtbarekollision") && ke::optimiererBericht().find("(wall)") != std::string::npos;
    }));
    add(tu("CW-PRUEF: Optimierer zu, neue Karte", [] {
        ke::optimiererSchliessen();
        ke::editor().karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
    }));
    // 4. nur mit BHED_CWPRUEF_KARTEN="maps/a.bsp;maps/b.bsp": Originalkarten des Spiels (nur lesen) - File > Open .bsp mit der
    // Rueckgewinnung der misc_model, Optimierer mit der Original-.bsp, Text- und JSON-Bericht in den Laufordner (cw_<name>.txt/.json)
    if (const char* liste = std::getenv("BHED_CWPRUEF_KARTEN"); liste != nullptr && liste[0] != 0) {
        std::vector<std::string> karten;
        std::string rest = liste;
        while (!rest.empty()) {
            const std::size_t p = rest.find(';');
            if (!rest.substr(0, p).empty()) { karten.push_back(rest.substr(0, p)); }
            if (p == std::string::npos) { break; }
            rest = rest.substr(p + 1);
        }
        for (const std::string& k : karten) {
            std::string name = k.substr(k.find_last_of('/') + 1);
            name = name.substr(0, name.find('.'));
            add(tu("CW-PRUEF Originalkarte " + k + " oeffnen", [k] {
                karteDialogeZu();
                stufe3::q3map2Bereitstellen();
                ke::optimiererSchliessen();
                ke::editor().bspRueck.modelle = true;
                ke::editor().karte.undo.alsGespeichertMarkieren();
                (void)ke::bspOeffnenSpiel(k);
            }));
            add(warteBis("CW-PRUEF " + k + " geladen", [] { return ke::editor().bspHerkunft.aktiv; }, 12000));
            add(stufe9c::langeWarten("CW-PRUEF " + k + ": Rueckgewinnung der Modelle", [] { return !ke::bspRueckLaeuft(); }, 600000));
            add(tu("CW-PRUEF " + k + ": Optimierer", [laeufe] {
                *laeufe = ke::optimiererBlick().laeufe;
                ke::optimiererOeffnen();
            }));
            add(stufe5::warteOptimierer("CW-PRUEF " + k + ": Optimierer fertig", laeufe));
            add(pruefe("CW-PRUEF " + k + ": Berichte geschrieben (Befunde im Protokoll)", [name] {
                const ke::OptimiererBlick b = ke::optimiererBlick();
                const bool ok = ke::optimiererBerichtSchreiben(stufe2::laufDatei(("cw_" + name + ".txt").c_str())) &&
                                ke::optimiererBerichtJsonSchreiben(stufe2::laufDatei(("cw_" + name + ".json").c_str()));
                diag::detail("Knopftest: CW-PRUEF Originalkarte " + name + " (" + b.bsp + ", " + std::to_string(static_cast<long long>(b.ms)) + " ms) - " +
                             stufe5::befundListe());
                return ok && b.hatErgebnis;
            }));
        }
        add(tu("CW-PRUEF: Optimierer zu, neue Karte (nach den Originalkarten)", [] {
            ke::optimiererSchliessen();
            ke::editor().karte.undo.alsGespeichertMarkieren();
            ke::neueKarte();
        }));
    }
    add(tu("CW-IMPORT: Pruefprofil wie vorher", [profilVorher] { ke::clonewarsProfilSetzen(*profilVorher); }));
    return s;
}

}  // namespace cwpruef
