// kartebau.cpp - Karten-Editor Stufe 3 Punkt 3: das Build-Menue (bhed/karte/bau.h)
// und der XML-Leser (bhed/karte/xmlstrom.h).
//
// Geprueft wird gegen NRC 20260114 (radiant/build.cpp):
//   - data/karte/ja und jk2 default_build_menu.xml: Eintraege wie im Menue
//     (Foto menu_Build.png), Trenner, Werkzeuge in Namensfolge;
//   - der ExtraResourcePaths-Kniff (ToolXMLConstructor::flush) haengt die
//     Variable genau einmal an das q3map2-Stueck;
//   - "[Final] Fast" mit allen Variablen ausgewertet, Bedingungen an und aus,
//     unbekannte Variable -> leer und gemeldet;
//   - Schreiben wie XMLStreamWriter (Entities, "/>" fuer Leeres) und zurueck;
//   - version_compatible: 2.0 und 2 ja, 2.1 und 3.0 nein;
//   - Werkzeug als XML-Text (Tabelle "Editables") und zurueck;
//   - der XML-Leser liefert Zeichen fuer Zeichen gefuettert dasselbe wie am Stueck.
//
//   - Punkt 4: der Rueckkanal von q3map2 - ein echter, aufgenommener Strom
//     (tests/karte/q3map2/raum_leck.strom.bin, bhed_q3map2 -meta auf
//     raum_leck.map): Nachrichten von l_net zerlegen, Text mit Stufe, die
//     Auswahl "Entity leaked", die Leck-Linie gleich der .lin, Stopp am Ende
//     der Fehlermeldung (Stop Compilation on Leak) bzw. weiter ohne;
//     Fassungspruefung, unbekannte Knoten, pointmsg und windingmsg;
//     Pointfile_Parse (auch der zhlt-Stil).
//
// Aufruf: kartebau <data/karte> [<tests/karte/q3map2>]
#include "bhed/karte/bau.h"
#include "bhed/karte/baufortschritt.h"
#include "bhed/karte/bspoeffnen.h"
#include "bhed/karte/editordaten.h"
#include "bhed/karte/baumonitor.h"
#include "bhed/karte/xmlstrom.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::string lies(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

std::vector<std::string> namen(const BauProjekt& p) {
    std::vector<std::string> n;
    for (const BauEintrag& e : p.eintraege) { n.push_back(e.name); }
    return n;
}

BauVariablen variablen(bool ueberwachen, const std::string& spiel) {
    // build_init_variables (qe3.cpp:152-178)
    BauVariablen v;
    v["RadiantPath"] = "C:/Prog/BehavEd-Remake/";
    v["ExecutableType"] = "exe";
    v["EnginePath"] = "C:/Games/Jedi Academy/GameData/";
    v["UserEnginePath"] = "";
    v["ExtraResourcePaths"] = "";
    v["MonitorAddress"] = ueberwachen ? "127.0.0.1:39000" : "";
    v["GameName"] = spiel;
    v["BspFile"] = "C:/maps/raum.bsp";
    v["MapFile"] = "C:/maps/raum.map";
    v["MapName"] = "raum.map";
    return v;
}

// Alle SAX-Ereignisse als Text, Zeichen zusammengefasst
std::string ereignisse(const std::string& xml, std::size_t stueck) {
    std::string aus;
    std::string text;
    XmlStrom x;
    const auto spuelen = [&] {
        if (!text.empty()) { aus += "T(" + text + ")"; }
        text.clear();
    };
    x.anfang = [&](const std::string& n, const XmlAttribute& a) {
        spuelen();
        aus += "<" + n;
        for (const auto& [k, w] : a) { aus += " " + k + "=" + w; }
        aus += ">";
    };
    x.ende = [&](const std::string& n) {
        spuelen();
        aus += "</" + n + ">";
    };
    x.zeichen = [&](const std::string& t) { text += t; };
    x.fehler = [&](const std::string& f) { aus += "E(" + f + ")"; };
    for (std::size_t i = 0; i < xml.size(); i += stueck) { x.einspeisen(xml.substr(i, stueck)); }
    x.schliessen();
    spuelen();
    return aus;
}


// --- Stufe 8a Punkt 1 ---------------------------------------------------------------
//
// Ein aufgenommener Rueckkanal (bhed_q3map2 -connect, SP/s8/strom_aufnehmen.py) und
// dazu je Empfang "Byte-Ende Sekunden" (.zeit): die Nachrichten mit ihrer Zeit, der
// Text (alle Stufen) wie ihn BauMonitor an Sys_Print gibt.
struct Gespielt {
    std::vector<std::pair<double, std::string>> text;   // Zeit, Text
    double ende = 0.0;
};

Gespielt abspielen(const std::string& bin, const std::string& zeit) {
    Gespielt g;
    const std::string roh = lies(bin);
    std::vector<std::pair<std::size_t, double>> zeiten;
    if (!zeit.empty()) {
        std::istringstream z(lies(zeit));
        std::size_t b = 0;
        double t = 0.0;
        while (z >> b >> t) { zeiten.emplace_back(b, t); }
    }
    BauNetzRahmen rahmen;
    BauMonitor m;
    double jetzt = 0.0;
    m.drucken = [&](int, const std::string& t) { g.text.emplace_back(jetzt, t); };
    m.verbunden();
    std::size_t von = 0;
    for (const auto& [bis, t] : zeiten) {
        if (bis > roh.size() || bis < von) { break; }
        jetzt = t;
        rahmen.einspeisen(roh.data() + von, bis - von);
        von = bis;
        std::string n;
        while (rahmen.naechste(n)) { m.einspeisen(n); }
        g.ende = t;
    }
    if (zeiten.empty()) {   // ohne Zeiten: alles bei 0
        rahmen.einspeisen(roh.data(), roh.size());
        std::string n;
        while (rahmen.naechste(n)) { m.einspeisen(n); }
    }
    return g;
}

BauSchrittBeobachter beobachten(const Gespielt& g, std::size_t stueck = 1U << 20) {
    BauSchrittBeobachter b;
    for (const auto& [t, text] : g.text) {
        for (std::size_t i = 0; i < text.size(); i += stueck) { b.text(text.substr(i, stueck), t); }
    }
    return b;
}

bool hatMarke(const BauSchrittBeobachter& b, const std::string& k) {
    return std::any_of(b.marken().begin(), b.marken().end(), [&k](const BauPhasenMarke& m) { return m.schluessel == k; });
}

void fortschrittPruefen(const std::string& q3) {
    // Stufen der Befehle aus dem Build-Menue
    const std::string q = "\"C:/x/bhed_q3map2.exe\" -v -connect 127.0.0.1:39000 -game ja -fs_basepath \"C:/Spiel/GameData\" ";
    pruefe(bauStufeName(q + "-meta \"C:/m/a.map\"") == "BSP" && bauStufeName(q + "-vis -saveprt \"C:/m/a.map\"") == "VIS" &&
               bauStufeName(q + "-light -fast -bounce 8 \"C:/m/a.map\"") == "LIGHT" && bauStufeName(q + "-onlyents \"C:/m/a.bsp\"") == "BSP" &&
               bauStufeName(q + "-convert -format ase \"C:/m/a.bsp\"") == "convert" && bauStufeName(q + "-mergebsp -world a b") == "mergebsp" &&
               bauStufeName("\"C:/-light/x.exe\" -meta \"C:/-vis/a.map\"") == "BSP",
           "Stufe aus dem Befehl: BSP/VIS/LIGHT, andere Hauptschalter, Pfade mit -light/-vis zaehlen nicht");
    pruefe(bauBounceAusBefehl(q + "-light -fast -bounce 8 x") == 8 && bauBounceAusBefehl(q + "-light x") == 0, "-bounce N gelesen");
    std::string name;
    pruefe(bauPhasenKopf("--- Radiosity (bounce 1 of 2) ---", &name) && name == "Radiosity (bounce 1 of 2)" &&
               bauPhasenKopf("----- FogDrawSurfs -----", &name) && name == "FogDrawSurfs" &&
               bauPhasenKopf("############### model 0 ###############", &name) && name == "model 0" && !bauPhasenKopf("-----", &name) &&
               !bauPhasenKopf("0...1...2", &name) && bauPhasenGrundname("BasePortalVis (20)") == "BasePortalVis" &&
               bauPhasenGrundname("model 12") == "model",
           "Kopfzeilen von q3map2 erkannt, Grundname ohne Klammern und Nummer");

    // Echte Stroeme: BSP, VIS, LIGHT (raum.map, [Final] Fast mit -bounce 2)
    const BauSchrittBeobachter bsp = beobachten(abspielen(q3 + "/fortschritt_meta.bin", ""));
    pruefe(hatMarke(bsp, "BSP#1") && hatMarke(bsp, "FaceBSP#2") && hatMarke(bsp, "model#1") && hatMarke(bsp, "MergeMetaTriangles#1") &&
               hatMarke(bsp, "FogDrawSurfs#1") && bsp.pacifier() < 0.0,
           "BSP-Strom: Phasen (FaceBSP zweimal, model, MergeMetaTriangles, FogDrawSurfs), am Ende ohne Pacifier (" +
               std::to_string(bsp.marken().size()) + " Phasen)");
    const BauSchrittBeobachter vis = beobachten(abspielen(q3 + "/fortschritt_vis.bin", ""));
    pruefe(hatMarke(vis, "BasePortalVis#1") && hatMarke(vis, "PassagePortalFlow#1") && vis.phase() == "PassagePortalFlow" && vis.pacifier() == 1.0,
           "VIS-Strom: BasePortalVis, PassagePortalFlow, dessen Pacifier fertig");
    const BauSchrittBeobachter licht = beobachten(abspielen(q3 + "/fortschritt_licht.bin", ""));
    pruefe(hatMarke(licht, "TraceGrid#1") && hatMarke(licht, "IlluminateRawLightmap#3") && hatMarke(licht, "Radiosity#2") && licht.bounce() == 2 &&
               licht.bounceVon() == 2,
           "LIGHT-Strom: TraceGrid, IlluminateRawLightmap dreimal (Licht + 2 Bounces), Bounce 2 von 2");
    // Zeichen fuer Zeichen gefuettert: dieselben Phasen
    const BauSchrittBeobachter licht1 = beobachten(abspielen(q3 + "/fortschritt_licht.bin", ""), 1);
    bool gleich = licht1.marken().size() == licht.marken().size();
    for (std::size_t i = 0; gleich && i < licht.marken().size(); ++i) { gleich = licht1.marken()[i].schluessel == licht.marken()[i].schluessel; }
    pruefe(gleich && licht1.pacifier() == licht.pacifier(), "LIGHT-Strom Zeichen fuer Zeichen: dieselben " + std::to_string(licht.marken().size()) + " Phasen");

    // Pacifier Zeichen fuer Zeichen: 0 = 0/40, je Punkt/Ziffer 1/40, " (" = fertig
    {
        BauSchrittBeobachter b;
        b.text("--- IlluminateRawLightmap ---\n", 0.0);
        const bool vorher = b.pacifier() < 0.0 && b.phase() == "IlluminateRawLightmap";
        b.text("0", 0.0);
        const double p0 = b.pacifier();
        b.text("...1...2...3...4", 10.0);
        const double p20 = b.pacifier();
        const std::vector<std::string> befehle{q + "-light x"};
        const BauSchaetzung s = bauSchaetzen(nullptr, befehle, 0, b, 10.0, {});
        b.text("...5...6...7...8...9... (12)\n", 12.0);
        pruefe(vorher && p0 == 0.0 && std::fabs(p20 - 0.4) < 1e-9 && b.pacifier() == 1.0, "Pacifier: 0 -> 0, ...4 -> 0,4 (Ziffer = Zehntel), \" (12)\" -> fertig");
        pruefe(!s.ausVerlauf && std::fabs(s.rest - 15.0) < 1e-6 && s.gesamtAnteil == 0.0,
               "erster Bau: Restzeit aus dem Fortschritt der Phase (0,4 nach 10 s -> noch 15 s), Gesamt in ganzen Schritten");
        b.text("--- MakeEntityDecals ---\n (0)\n        0 decal surfaces\n", 13.0);
        pruefe(b.pacifier() < 0.0 && b.phase() == "MakeEntityDecals", "Phase ohne Arbeit (\" (0)\"): kein Pacifier");
    }

    // Restzeit aus dem letzten Bau: Lauf A als Verlauf, Lauf B eingespielt (beide
    // -light -super 4 -samples 4 -filter -bounce 4 auf raum.map, je ~1,4 s)
    const Gespielt a = abspielen(q3 + "/fortschritt_lichtA.bin", q3 + "/fortschritt_lichtA.zeit");
    const Gespielt bb = abspielen(q3 + "/fortschritt_lichtB.bin", q3 + "/fortschritt_lichtB.zeit");
    const std::string befehl = q + "-light -super 4 -samples 4 -filter -bounce 4 \"C:/m/raum.map\"";
    BauLaufZeiten verlauf;
    {
        const BauSchrittBeobachter ba = beobachten(a);
        verlauf.schritte.push_back({befehl, a.ende, ba.marken()});
    }
    pruefe(a.ende > 0.5 && bb.ende > 0.5 && verlauf.schritte[0].phasen.size() > 20,
           "Laeufe A/B mit Zeiten: " + std::to_string(a.ende) + " s / " + std::to_string(bb.ende) + " s, " +
               std::to_string(verlauf.schritte[0].phasen.size()) + " Phasen");
    {
        BauSchrittBeobachter b;
        double schlimmste = 0.0;
        int messungen = 0;
        double letzterAnteil = -1.0;
        bool steigend = true;
        for (const auto& [t, text] : bb.text) {
            b.text(text, t);
            const BauSchaetzung s = bauSchaetzen(&verlauf, {befehl}, 0, b, t, {});
            if (!s.ausVerlauf || s.rest < 0.0) { continue; }
            if (s.gesamtAnteil + 0.15 < letzterAnteil) { steigend = false; }
            letzterAnteil = std::max(letzterAnteil, s.gesamtAnteil);
            if (t < bb.ende * 0.2 || t > bb.ende * 0.9) { continue; }
            schlimmste = std::max(schlimmste, std::fabs(t + s.rest - bb.ende) / bb.ende);
            ++messungen;
        }
        char z[160];
        std::snprintf(z, sizeof(z), "Restzeit aus Lauf A fuer Lauf B: groesster Fehler der Gesamtdauer %.0f %% (%d Messungen zwischen 20 und 90 %%)",
                      schlimmste * 100.0, messungen);
        std::printf("INFO  %s\n", z);
        pruefe(messungen >= 5 && schlimmste < 0.25 && steigend, z);
    }
    // Mehrere Schritte: die spaeteren Schritte zaehlen mit ihrer Dauer aus dem Verlauf
    {
        BauLaufZeiten drei;
        drei.schritte.push_back({q + "-meta x", 2.0, {}});
        drei.schritte.push_back({q + "-vis x", 3.0, {}});
        drei.schritte.push_back({befehl, a.ende, verlauf.schritte[0].phasen});
        BauSchrittBeobachter b;
        const BauSchaetzung s = bauSchaetzen(&drei, {q + "-meta x", q + "-vis x", befehl}, 1, b, 1.0, {2.2});
        pruefe(s.ausVerlauf && std::fabs(s.rest - (2.0 + a.ende)) < 1e-6 && std::fabs(s.gesamtAnteil - 3.2 / (3.2 + 2.0 + a.ende)) < 1e-9,
               "drei Schritte, im zweiten nach 1 s: Rest = 2 s VIS + LIGHT des Verlaufs, Gesamtanteil aus den echten Zeiten");
        const BauSchaetzung anders = bauSchaetzen(&drei, {q + "-meta x", q + "-light x"}, 1, b, 1.0, {2.2});
        pruefe(!anders.ausVerlauf, "anderer Eintrag (andere Stufen): kein Verlauf");
    }
    // Speicher: lesen/schreiben, Schluessel ohne Gross/Klein und mit '/', LRU
    {
        BauZeitenSpeicher sp;
        sp.setzen("C:\\Maps\\Raum.map", "[Final] Fast", verlauf);
        BauZeitenSpeicher sp2;
        const bool ok = sp2.lesen(sp.schreiben());
        const BauLaufZeiten* l = sp2.finden("c:/maps/raum.map", "[Final] Fast");
        pruefe(ok && l != nullptr && l->schritte.size() == 1 && l->schritte[0].phasen.size() == verlauf.schritte[0].phasen.size() &&
                   l->schritte[0].phasen[3].schluessel == verlauf.schritte[0].phasen[3].schluessel &&
                   std::fabs(l->schritte[0].dauer - verlauf.schritte[0].dauer) < 0.001 && sp2.finden("c:/maps/raum.map", "[Final] Extreme") == nullptr,
               "Bauzeiten: JSON hin und zurueck, je Karte (ohne Gross/Klein, / und \\ gleich) und Eintrag");
        for (int i = 0; i < 70; ++i) { sp2.setzen("k" + std::to_string(i) + ".map", "e", verlauf); }
        pruefe(sp2.anzahl() == kBauZeitenMax && sp2.finden("k69.map", "e") != nullptr && sp2.finden("c:/maps/raum.map", "[Final] Fast") == nullptr,
               "Bauzeiten: hoechstens 64 Laeufe, der aelteste faellt weg");
        BauLaufZeiten jetzt;
        jetzt.schritte.push_back({q + "-meta x", 3.0, {}});
        jetzt.schritte.push_back({q + "-vis x", 1.0, {}});
        BauLaufZeiten alt;
        alt.schritte.push_back({q + "-meta x", 4.0, {}});
        alt.schritte.push_back({q + "-vis x", 0.02, {}});
        const std::vector<std::string> v = bauZeitenVergleich(jetzt, &alt, "%d %s: %s s", " (last build %s s, %s)");
        pruefe(v.size() == 2 && v[0] == "1 BSP: 3.0 s (last build 4.0 s, -25 %)" && v[1] == "2 VIS: 1.0 s (last build 0.0 s, +1.0 s)",
               "Zeiten je Schritt gegen den letzten Lauf: " + (v.empty() ? std::string() : v[0]));
        pruefe(bauZeitText(65.4) == "1:05" && bauZeitText(7) == "0:07" && bauZeitText(3723) == "1:02:03", "Zeittext m:ss / h:mm:ss");
    }
}

void editorDatenPruefen(const std::string& q3) {
    const std::string q = "\"C:/x/bhed_q3map2.exe\" -v -game ja -fs_basepath \"C:/Spiel/GameData\" ";
    pruefe(befehlMitLichtern(q + "-meta \"C:/m/a b.map\"") == q + "-meta -keeplights \"C:/m/a b.map\"" &&
               befehlMitLichtern(q + "-onlyents \"C:/m/a.bsp\"") == q + "-onlyents -keeplights \"C:/m/a.bsp\"" &&
               befehlMitLichtern(q + "-vis \"C:/m/a.map\"") == q + "-vis \"C:/m/a.map\"" &&
               befehlMitLichtern(q + "-light -fast \"C:/m/a.map\"") == q + "-light -fast \"C:/m/a.map\"" &&
               befehlMitLichtern(q + "-meta -keepLights \"C:/m/a.map\"") == q + "-meta -keepLights \"C:/m/a.map\"" &&
               befehlMitLichtern(q + "-convert -format ase \"C:/m/a.bsp\"") == q + "-convert -format ase \"C:/m/a.bsp\"",
           "-keeplights nur fuer die BSP-Stufe (auch -onlyents), vor der Karte, nicht doppelt");
    pruefe(befehlMitVisWiederverwendung(q + "-vis -saveprt \"C:/m/a.map\"") == q + "-vis -saveprt -bhedvisreuse \"C:/m/a.map\"" &&
               befehlMitVisWiederverwendung(q + "-meta \"C:/m/a.map\"") == q + "-meta \"C:/m/a.map\"" &&
               befehlMitVisWiederverwendung(q + "-vis -bhedvisreuse \"C:/m/a.map\"") == q + "-vis -bhedvisreuse \"C:/m/a.map\"",
           "Stufe 8a/5a: -bhedvisreuse nur fuer -vis, vor der Karte, nicht doppelt");
    pruefe(editorDatenPfad("C:/maps/a.b/raum.bsp") == "C:/maps/a.b/raum.bhed" && editorDatenPfad("maps/x") == "maps/x.bhed", "Pfad der Begleitdatei");
    const std::string bsp = lies(q3 + "/raum.nrc.bsp");
    const std::string map = lies(q3 + "/raum.map");
    const std::string begleit = editorDatenSchreiben(map, bsp);
    std::string zurueck;
    pruefe(!bsp.empty() && editorDatenLesen(begleit, bsp, zurueck) == EditorDatenStand::Passt && zurueck == map,
           "Begleitdatei: die Quelle Byte fuer Byte zurueck (" + std::to_string(map.size()) + " Byte)");
    pruefe(begleit.rfind("// BehavEd editor data 1", 0) == 0 && begleit.find("\n// bsp-geometry ") != std::string::npos &&
               begleit.substr(begleit.size() - map.size()) == map,
           "Begleitdatei: drei //-Zeilen, dann die .map (selbst eine ladbare Karte)");
    // Entity-Lump geaendert (-onlyents, -keeplights): passt weiter; Geometrie geaendert: andere .bsp
    BspDatei b;
    (void)bspLesen(bsp, b);
    const auto mitLump = [&](std::size_t i, const std::string& neu) {
        // Lumps neu zusammensetzen (Kopf ident, version, je Lump ofs/len)
        std::vector<std::string> l = b.lumps;
        l[i] = neu;
        std::string d = bsp.substr(0, 8);
        std::string daten;
        const std::size_t kopf = 8 + 8 * l.size();
        for (const std::string& x : l) {
            const std::int32_t ofs = static_cast<std::int32_t>(kopf + daten.size());
            const std::int32_t len = static_cast<std::int32_t>(x.size());
            d.append(reinterpret_cast<const char*>(&ofs), 4);
            d.append(reinterpret_cast<const char*>(&len), 4);
            daten += x;
            while (daten.size() % 4 != 0) { daten += '\0'; }
        }
        return d + daten;
    };
    const std::string andereEnts = mitLump(0, b.lumps[0] + "{\n\"classname\" \"light\"\n}\n");
    const std::string andereGeo = mitLump(2, b.lumps[2] + std::string(16, '\0'));
    pruefe(editorDatenLesen(begleit, andereEnts, zurueck) == EditorDatenStand::Passt, "nur der Entity-Lump anders: passt");
    pruefe(editorDatenLesen(begleit, andereGeo, zurueck) == EditorDatenStand::AndereBsp, "eine Ebene mehr (Lump planes): andere .bsp");
    pruefe(editorDatenLesen(begleit.substr(0, begleit.size() - 1), bsp, zurueck) == EditorDatenStand::Kaputt &&
               editorDatenLesen("// irgendwas\n" + map, bsp, zurueck) == EditorDatenStand::Kaputt,
           "abgeschnitten oder fremder Kopf: kaputt");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Aufruf: kartebau <data/karte>\n");
        return 2;
    }
    const std::string daten = argv[1];

    // --- 1. JA: Eintraege, Trenner, Werkzeug
    BauProjekt ja;
    std::vector<std::string> m;
    pruefe(bauMenueLesen(lies(daten + "/ja/default_build_menu.xml"), ja, &m), "ja: default_build_menu.xml gelesen");
    pruefe(m.empty(), "ja: ohne Meldungen");
    const std::vector<std::string> soll = {"Single: bsp stage", "Single: vis stage", "Single: light stage", "-",
                                           "Entity-Only compile", "-", "convert to .ASE model", "convert to .OBJ model", "-",
                                           "Export Lightmaps", "Import Lightmaps", "Create .pk3", "-",
                                           "[Test] Quick and Crude", "[Test] Floodlight", "[Final] Fast",
                                           "[Final] Extreme Quality"};
    pruefe(namen(ja) == soll, "ja: 17 Eintraege wie das Menue (menu_Build.png)");
    int trenner = 0;
    for (const BauEintrag& e : ja.eintraege) { trenner += bauIstTrenner(e) ? 1 : 0; }
    pruefe(trenner == 4, "ja: 4 Trenner");
    pruefe(ja.werkzeuge.size() == 1 && ja.werkzeuge.count("q3map2") == 1, "ja: ein Werkzeug q3map2");
    const BauWerkzeug& q3 = ja.werkzeuge["q3map2"];
    pruefe(q3.teile.size() == 5, "ja: q3map2 hat 5 Teile (Text, [ExtraResourcePaths], cond, Text, cond)");
    if (q3.teile.size() == 5) {
        pruefe(!q3.teile[0].bedingt && q3.teile[0].text == "\"[RadiantPath]bhed_q3map2.[ExecutableType]\" -v",
               "ja: erster Teil bis -v");
        pruefe(!q3.teile[1].bedingt && q3.teile[1].text == "[ExtraResourcePaths]", "ja: ExtraResourcePaths angehaengt");
        pruefe(q3.teile[2].bedingt && q3.teile[2].test == "[MonitorAddress]" && q3.teile[2].text == " -connect [MonitorAddress]",
               "ja: Bedingung MonitorAddress");
        pruefe(q3.teile[4].bedingt && q3.teile[4].test == "[GameName]" && q3.teile[4].text == " -fs_game [GameName]",
               "ja: Bedingung GameName");
    }
    pruefe(ja.eintraege[15].befehle.size() == 3 && bauTooltip(ja.eintraege[15]) ==
               "[q3map2] -meta \"[MapFile]\"\n[q3map2] -vis \"[MapFile]\"\n"
               "[q3map2] -light -fast -patchshadows -samples 2 -filter -bounce 8 \"[MapFile]\"",
           "ja: Tooltip von [Final] Fast");

    // --- 2. Auswerten
    std::vector<std::string> mm;
    const std::vector<std::string> fast = bauBefehle(ja, 15, variablen(true, "base"), &mm);
    const std::string kopf = "\"C:/Prog/BehavEd-Remake/bhed_q3map2.exe\" -v -connect 127.0.0.1:39000 -game ja "
                             "-fs_basepath \"C:/Games/Jedi Academy/GameData/\" -fs_game base";
    pruefe(fast.size() == 3, "[Final] Fast: 3 Befehle");
    if (fast.size() == 3) {
        pruefe(fast[0] == kopf + " -meta \"C:/maps/raum.map\"", "[Final] Fast: -meta (" + fast[0] + ")");
        pruefe(fast[1] == kopf + " -vis \"C:/maps/raum.map\"", "[Final] Fast: -vis");
        pruefe(fast[2] == kopf + " -light -fast -patchshadows -samples 2 -filter -bounce 8 \"C:/maps/raum.map\"",
               "[Final] Fast: -light");
    }
    pruefe(mm.empty(), "[Final] Fast: keine unbekannte Variable");
    BauVariablen ohne = variablen(false, "");
    ohne["ExtraResourcePaths"] = " -fs_pakpath \"D:/pak\"";
    const std::vector<std::string> b0 = bauBefehle(ja, 0, ohne);
    pruefe(b0.size() == 1 && b0[0] == "\"C:/Prog/BehavEd-Remake/bhed_q3map2.exe\" -v -fs_pakpath \"D:/pak\" -game ja "
                                       "-fs_basepath \"C:/Games/Jedi Academy/GameData/\" -meta \"C:/maps/raum.map\"",
           "ohne Ueberwachung und Mod: kein -connect, kein -fs_game, mit -fs_pakpath");
    std::vector<std::string> m3;
    pruefe(bauAuswerten("a[Fehlt]b", BauVariablen{}, &m3) == "ab" && m3.size() == 1 &&
               m3[0] == "undefined build variable: \"Fehlt\"",
           "unbekannte Variable: leer und gemeldet");
    std::vector<std::string> m4;
    pruefe(bauBefehle(ja, 99, variablen(true, "base"), &m4).empty() && !m4.empty() && m4.back() == "build #99 not defined",
           "Eintrag 99: nichts, gemeldet");
    pruefe(bauBefehle(ja, 3, variablen(true, "base")).empty(), "Trenner: keine Befehle");

    // --- 3. JK2: Kommentar, drei Werkzeuge in Namensfolge
    BauProjekt jk2;
    pruefe(bauMenueLesen(lies(daten + "/jk2/default_build_menu.xml"), jk2), "jk2: gelesen (Kommentar vor <project>)");
    std::string wz;
    for (const auto& [n, w] : jk2.werkzeuge) { wz += n + ","; }
    pruefe(wz == "bsp,light,vis,", "jk2: Werkzeuge bsp, light, vis");
    std::size_t finNr = jk2.eintraege.size();
    for (std::size_t i = 0; i < jk2.eintraege.size(); ++i) {
        if (jk2.eintraege[i].name == "Q3Map2: (final) BSP -meta, -vis, -light -fast -filter -super 2") { finNr = std::min(finNr, i); }
    }
    pruefe(jk2.eintraege.size() == 14 && finNr == 10, "jk2: 14 Eintraege, (final) an Stelle 10");
    const std::vector<std::string> fin = bauBefehle(jk2, finNr, variablen(true, "base"));
    pruefe(fin.size() == 3 && fin[1].find(" -vis -saveprt \"C:/maps/raum.map\"") != std::string::npos &&
               fin[1].find("-game jk2") != std::string::npos,
           "jk2: (final) -vis -saveprt");

    // --- 4. Schreiben wie XMLStreamWriter und zurueck
    BauProjekt klein;
    klein.werkzeuge["t"].teile = {BauTeil{false, {}, "\"a<b>&\""}, BauTeil{true, "[X]", " -x"}, BauTeil{true, "[Y]", ""}};
    klein.eintraege = {BauEintrag{"b & c", {"[t] go", ""}}, BauEintrag{"-", {}}, BauEintrag{"leer", {}}};
    const std::string soll4 = "<?xml version=\"1.0\"?>\n<project version=\"2.0\">\n"
                              "<var name=\"t\">&quot;a&lt;b&gt;&amp;&quot;<cond value=\"[X]\"> -x</cond><cond value=\"[Y]\"/></var>\n"
                              "<build name=\"b &amp; c\">\n<command>[t] go</command>\n<command/>\n</build>\n"
                              "<separator/>\n<build name=\"leer\">\n</build>\n</project>\n";
    const std::string ist4 = bauMenueSchreiben(klein);
    pruefe(ist4 == soll4, "schreiben wie XMLStreamWriter:\n" + ist4);
    BauProjekt zurueck;
    pruefe(bauMenueLesen(ist4, zurueck) && namen(zurueck) == namen(klein) && zurueck.eintraege[0].befehle == klein.eintraege[0].befehle &&
               bauMenueSchreiben(zurueck) == ist4,
           "geschrieben und gelesen: gleich");
    BauProjekt ja2;
    pruefe(bauMenueLesen(bauMenueSchreiben(ja), ja2) && bauMenueSchreiben(ja2) == bauMenueSchreiben(ja) && namen(ja2) == soll,
           "ja: schreiben/lesen stabil, Kniff nicht doppelt");
    // Nach dem Schreiben steht [ExtraResourcePaths] im selben Textstueck wie
    // q3map2 (XMLStreamWriter schreibt die Teile ohne Trennung) - beim Lesen ein
    // Teil weniger, wie bei NRC nach Speichern und Laden; ausgewertet gleich.
    pruefe(ja2.werkzeuge["q3map2"].teile.size() == 4 &&
               bauAuswerten(ja2.werkzeuge["q3map2"], variablen(true, "base")) == bauAuswerten(q3, variablen(true, "base")),
           "ja: nach dem Rundlauf 4 Teile, gleich ausgewertet");

    // --- 5. Fassung
    const auto fassung = [](const char* f) {
        BauProjekt p;
        return bauMenueLesen(std::string("<project version=\"") + f + "\"><build name=\"x\"><command>c</command></build></project>", p) &&
               p.eintraege.size() == 1;
    };
    pruefe(fassung("2.0") && fassung("2"), "Fassung 2.0 und 2 passen");
    pruefe(!fassung("2.1") && !fassung("3.0") && !fassung("1.0"), "Fassung 2.1, 3.0, 1.0 passen nicht");
    BauProjekt ohneProjekt;
    pruefe(!bauMenueLesen("<menu/>", ohneProjekt), "ohne <project>: failed to parse");

    // --- 6. Werkzeug als Text (Editables) und zurueck
    const std::string q3xml = bauWerkzeugXml(q3);
    pruefe(q3xml == "&quot;[RadiantPath]bhed_q3map2.[ExecutableType]&quot; -v[ExtraResourcePaths]<cond value=\"[MonitorAddress]\"> "
                    "-connect [MonitorAddress]</cond> -game ja -fs_basepath &quot;[EnginePath]&quot;<cond value=\"[GameName]\"> "
                    "-fs_game [GameName]</cond>",
           "Werkzeug als XML-Text: " + q3xml);
    BauWerkzeug w2;
    pruefe(bauWerkzeugAusXml(q3xml, w2) && w2.teile.size() == 4 && bauAuswerten(w2, variablen(true, "base")) ==
                                                                    bauAuswerten(q3, variablen(true, "base")),
           "XML-Text zurueck: dasselbe Werkzeug");
    BauWerkzeug w3;
    pruefe(bauWerkzeugAusXml("x [y] <cond value=\"[z]\">w</cond>", w3) && w3.teile.size() == 2 && w3.teile[1].bedingt,
           "neues Werkzeug aus Text");
    BauVariablen vv = variablen(true, "base");
    BauProjekt zwei;
    zwei.werkzeuge["a"].teile = {BauTeil{false, {}, "A"}};
    zwei.werkzeuge["b"].teile = {BauTeil{false, {}, "[a]B"}};
    bauWerkzeugeAuswerten(zwei, vv, "b");
    pruefe(vv.count("a") == 1 && vv.count("b") == 0, "Werkzeuge bis ausschliesslich b");
    bauWerkzeugeAuswerten(zwei, vv);
    pruefe(vv["b"] == "AB", "spaeteres Werkzeug nutzt frueheres");

    // --- 7. Trenner
    pruefe(bauIstTrenner(BauEintrag{"-", {"", ""}}) && !bauIstTrenner(BauEintrag{"-", {"x"}}) && !bauIstTrenner(BauEintrag{"x", {}}),
           "is_separator: '-' mit leeren Befehlen");

    // --- 8. XML-Leser
    const std::string xml = "<?xml version=\"1.0\"?>\r\n<!-- k -->\r\n<a x='1' y=\"&lt;&#65;&#x42;\">t&amp;u\r\n<b/>v<![CDATA[<w>]]></a>";
    const std::string e1 = ereignisse(xml, xml.size());
    pruefe(e1 == "<a x=1 y=<AB>T(t&u\n)<b></b>T(v<w>)</a>", "XML-Leser: " + e1);
    pruefe(ereignisse(xml, 1) == e1, "XML-Leser: Zeichen fuer Zeichen dasselbe: [" + ereignisse(xml, 1) + "] statt [" + e1 + "] " +
                                         std::to_string(ereignisse(xml, 1).size()) + "/" + std::to_string(e1.size()));
    pruefe(ereignisse(xml, 3) == e1, "XML-Leser: je 3 Zeichen dasselbe: " + ereignisse(xml, 3));
    pruefe(ereignisse("<a></b>", 7).find("E(") != std::string::npos, "XML-Leser: falsches Ende gemeldet");
    pruefe(xmlEntitiesAufloesen("&quot;&apos;&unbekannt;&#0;") == "\"'&unbekannt;&#0;", "Entities: unbekannte bleiben");

    // --- 9. Rueckkanal von q3map2 (Punkt 4)
    if (argc >= 3) {
        const std::string q3ordner = argv[2];
        const std::string roh = lies(q3ordner + "/raum_leck.strom.bin");
        std::vector<std::string> nachrichten;
        BauNetzRahmen rahmen;
        rahmen.einspeisen(roh.data(), roh.size());
        std::string n;
        while (rahmen.naechste(n)) { nachrichten.push_back(n); }
        pruefe(roh.size() > 1000 && nachrichten.size() > 5 && nachrichten[0].rfind("<?xml version=\"1.0\"?><q3map_feedback version=\"1\">", 0) == 0,
               "Strom: " + std::to_string(nachrichten.size()) + " Nachrichten, die erste der Kopf");
        struct Ablauf {
            std::string text[4];
            std::vector<std::string> warnungen;
            std::vector<std::string> fehler;
            std::vector<BauRueckmeldung> eintraege;
            std::vector<Vektor3f> linie;
            bool beendet = false;
            std::size_t beendetNach = 0;
        };
        const auto lauf = [&](bool leckStopp, std::size_t stueck) {
            Ablauf a;
            BauMonitor m;
            m.leckStopp = leckStopp;
            m.drucken = [&](int s, const std::string& t) { a.text[s >= 0 && s <= 3 ? s : 0] += t; };
            m.warnung = [&](const std::string& t) { a.warnungen.push_back(t); };
            m.fehler = [&](const std::string& t) { a.fehler.push_back(t); };
            m.rueckmeldung = [&](const BauRueckmeldung& r) { a.eintraege.push_back(r); };
            m.leckLinie = [&](const std::vector<Vektor3f>& l) { a.linie = l; };
            m.verbunden();
            for (std::size_t i = 0; i < nachrichten.size() && !a.beendet; ++i) {
                for (std::size_t j = 0; j < nachrichten[i].size(); j += stueck) {
                    m.einspeisen(nachrichten[i].substr(j, stueck));
                }
                if (m.beenden()) {   // EndMonitoringLoop: nichts mehr lesen
                    a.beendet = true;
                    a.beendetNach = i;
                }
            }
            return a;
        };
        const Ablauf s = lauf(true, 1 << 20);
        const std::vector<Vektor3f> lin = punktdateiLesen(lies(q3ordner + "/raum_leck.strom.lin"));
        pruefe(s.text[1].find("--- LoadMapFile ---") != std::string::npos && s.text[2].find("WARNING: Couldn't find image for shader textures/test/boden") != std::string::npos,
               "Strom: Text der Stufe 1 und Warnungen der Stufe 2");
        pruefe(s.eintraege.size() == 2 && s.eintraege[0].art == BauRueckmeldung::Art::Auswahl && s.eintraege[0].entitaet == 3 &&
                   s.eintraege[0].pinsel == 0 && s.eintraege[0].name == "Entity 3, Brush 0: Entity leaked" &&
                   s.eintraege[1].art == BauRueckmeldung::Art::Leck && s.eintraege[1].name == "Map leaked",
               "Strom: debug window bekommt \"Entity 3, Brush 0: Entity leaked\" und \"Map leaked\"");
        pruefe(!s.warnungen.empty() && s.warnungen[0] == "Entity 3, Brush 0: Entity leaked", "CSelectMsg: die Meldung als Warnung in die Konsole");
        pruefe(s.text[3] == "MAP LEAKED\n", "Strom: \"MAP LEAKED\" mit Stufe 3");
        bool linieGleich = lin.size() == 4 && s.linie.size() == 4;
        for (std::size_t i = 0; linieGleich && i < 4; ++i) {
            linieGleich = s.linie[i].x() == lin[i].x() && s.linie[i].y() == lin[i].y() && s.linie[i].z() == lin[i].z();
        }
        pruefe(linieGleich, "Leck-Linie aus dem Strom = die .lin (4 Punkte, 536 140 128 ... 256 160 192)");
        pruefe(s.beendet && s.text[1].find("--- FillOutside") == std::string::npos,
               "Stop Compilation on Leak: Ende am Ende der Fehlermeldung, danach nichts mehr");
        const Ablauf w = lauf(false, 1 << 20);
        pruefe(!w.beendet && w.text[1].find("Disconnecting") != std::string::npos && w.linie.size() == 4,
               "ohne Stop on Leak: weiter bis \"Disconnecting\", die Linie trotzdem da");
        const Ablauf z = lauf(true, 1);
        pruefe(z.text[1] == s.text[1] && z.text[2] == s.text[2] && z.eintraege.size() == 2 && z.beendetNach == s.beendetNach,
               "Strom Zeichen fuer Zeichen gefuettert: dasselbe");
        // Fassung, unbekannte Knoten, pointmsg, windingmsg
        const auto kurz = [&](const std::string& xml, Ablauf& a) {
            BauMonitor m;
            m.drucken = [&](int st, const std::string& t) { a.text[st >= 0 && st <= 3 ? st : 0] += t; };
            m.warnung = [&](const std::string& t) { a.warnungen.push_back(t); };
            m.fehler = [&](const std::string& t) { a.fehler.push_back(t); };
            m.rueckmeldung = [&](const BauRueckmeldung& r) { a.eintraege.push_back(r); };
            m.einspeisen(xml);
            a.beendet = m.beenden();
        };
        Ablauf f1;
        kurz("<?xml version=\"1.0\"?><q3map_feedback version=\"2\"><message level=\"1\">x</message>", f1);
        pruefe(f1.beendet && !f1.fehler.empty() && f1.fehler[0].find("I got an incoming connection with version 2") != std::string::npos &&
                   f1.text[1].empty(),
               "falsche Fassung: Fehler, abgebrochen, nichts mehr gezeigt");
        Ablauf f2;
        kurz("<?xml version=\"1.0\"?><q3map_feedback><message level=\"1\">x</message>", f2);
        pruefe(f2.beendet && !f2.fehler.empty() && f2.fehler[0].rfind("No stream version given", 0) == 0, "ohne Fassung: altes q3map, abgebrochen");
        Ablauf f3;
        kurz("<?xml version=\"1.0\"?><q3map_feedback version=\"1\"><frage>verborgen</frage><message level=\"1\">sichtbar</message>"
             "<pointmsg level=\"3\">Punkt kaputt<point>1 2.5 -3</point></pointmsg>",
             f3);
        pruefe(f3.text[1] == "sichtbar" && !f3.warnungen.empty() && f3.warnungen[0] == "Warning: ignoring unrecognized node in XML stream (frage)",
               "unbekannter Knoten: Warnung, Inhalt uebersprungen");
        pruefe(f3.eintraege.size() == 1 && f3.eintraege[0].art == BauRueckmeldung::Art::Punkt && f3.eintraege[0].name == "Punkt kaputt" &&
                   f3.eintraege[0].punkt.y() == 2.5F && f3.eintraege[0].punkt.z() == -3.0F,
               "pointmsg: Meldung und Punkt");
        Ablauf f4;
        kurz("<?xml version=\"1.0\"?><q3map_feedback version=\"1\"><windingmsg level=\"3\">Wicklung<winding>3 (0 0 0)(64 0 0)(64 64 8)</winding></windingmsg>"
             "<message level=\"3\">************ ERROR ************\nx\n</message><message level=\"1\">danach</message>",
             f4);
        pruefe(f4.eintraege.size() == 1 && f4.eintraege[0].art == BauRueckmeldung::Art::Wicklung && f4.eintraege[0].punkte.size() == 3 &&
                   f4.eintraege[0].punkte[2].z() == 8.0F,
               "windingmsg: drei Punkte");
        pruefe(f4.beendet, "Fehlermeldung (Stufe 3): Ende der Ueberwachung am Ende des Elements");
        std::string kaputt;
        const std::vector<Vektor3f> zhlt = punktdateiLesen("1 2 3 - 4 5 6\n7 8 9\nxx\n", &kaputt);
        pruefe(zhlt.size() == 2 && zhlt[0].x() == 4.0F && zhlt[1].z() == 9.0F && kaputt == "Corrupt point file, line 3",
               "Pointfile_Parse: zhlt-Stil nimmt den zweiten Punkt, kaputte Zeile gemeldet");
    } else {
        pruefe(false, "Aufruf ohne tests/karte/q3map2");
    }

    // --- 10. Stufe 8a Punkt 1: Fortschritt mit Restzeit aus eingespielten Stroemen
    if (argc >= 3) { fortschrittPruefen(argv[2]); }

    // --- 11. Stufe 8a Punkt 2: Editor-Daten behalten (-keeplights, <karte>.bhed)
    if (argc >= 3) { editorDatenPruefen(argv[2]); }

    std::printf("kartebau: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
