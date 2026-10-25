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
// Aufruf: kartebau <data/karte>
#include "bhed/karte/bau.h"
#include "bhed/karte/xmlstrom.h"

#include <algorithm>
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

    std::printf("kartebau: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
