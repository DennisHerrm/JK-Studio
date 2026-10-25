// karteportale.cpp - Karten-Editor Stufe 3 Punkt 5: der Portal Viewer ohne
// Oberflaeche (bhed/karte/portale.h).
//
// Geprueft wird gegen NRC 20260114 (contrib/prtview, libs/profile/profile2.h):
//   - IniFile: Kommentare (ganze Zeile, nach Leerraum), Leerraum, Abschnitte,
//     Schluessel vor dem ersten Abschnitt, sortiert geschrieben;
//   - LoadConfig: Vorgaben, Begrenzungen, ZBuffer ausserhalb 0..2 -> 0,
//     Farben nur 24 Bit; SaveConfig: "<Zahl>        ; <Kommentar>", die Datei
//     sortiert wie std::map; FixColors;
//   - rand() wie die C-Laufzeit von Windows (41, 18467, 6334, ...);
//   - CPortals::Load an tests/karte/q3map2/raum.nrc.prt (von NRCs q3map2):
//     10 Portale, Hint-Flags erkannt (die Flaechenzahl als vierte Kopfzeile),
//     Punkte, Mitte, innere Punkte (1 % zur Mitte), Grenzen, Farben aus rand();
//     PRT2 und PRT1-AM; jede Fehlermeldung wortgleich und danach leer.
//
// Aufruf: karteportale <tests/karte/q3map2>
#include "bhed/karte/portale.h"

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

std::string meldungen(const std::vector<PortalMeldung>& m) {
    std::string s;
    for (const PortalMeldung& x : m) { s += std::to_string(x.art) + ":" + x.text; }
    return s;
}

}  // namespace

int main(int argc, char** argv) {
    // --- IniFile
    {
        IniDatei ini;
        ini.lesen("vorher=1\n; Kommentar\n[Configuration]\n  Width2D = 7        ; Width of lines\nRender2D=1;kein Kommentar\n"
                  "[B]\nx=  wert mit Leerraum  \r\n[kaputt\ny=2\n");
        pruefe(!ini.wert("Configuration", "vorher") && !ini.wert("", "vorher"), "IniFile: Schluessel vor dem ersten Abschnitt verworfen");
        pruefe(ini.wert("Configuration", "Width2D").value_or("") == "7", "IniFile: Wert ohne Leerraum und Kommentar");
        pruefe(ini.wert("Configuration", "Render2D").value_or("") == "1;kein Kommentar", "IniFile: ';' ohne Leerraum davor gehoert zum Wert");
        pruefe(ini.wert("B", "x").value_or("") == "wert mit Leerraum", "IniFile: Leerraum vorn und hinten (auch CR) weg");
        pruefe(ini.wert("B", "y").value_or("") == "2", "IniFile: \"[kaputt\" ohne ']' laesst den Abschnitt stehen");
        pruefe(ini.schreiben() == "[B]\nx=wert mit Leerraum\ny=2\n[Configuration]\nRender2D=1;kein Kommentar\nWidth2D=7\n",
               "IniFile: sortiert geschrieben:\n" + ini.schreiben());
    }
    // --- LoadConfig / SaveConfig / FixColors
    {
        const PortalEinstellungen v = portalEinstellungenAus(IniDatei{});
        pruefe(!v.zeige2d && v.breite2d == 3 && v.farbe2d == rgbPacken(0, 0, 255) && v.hints && v.nichtHints && v.zeige3d && v.zPuffer == 1 &&
                   !v.nebel && v.polygone && v.linien && v.breite3d == 3 && v.farbe3d == rgbPacken(255, 255, 0) &&
                   v.farbeNebel == rgbPacken(127, 127, 127) && v.deckkraft == 50 && !v.ausschnitt && v.ausschnittWeite == 1024,
               "LoadConfig: Vorgaben ohne Datei");
        pruefe(v.fp2d[0] == 0.0F && v.fp2d[2] == 1.0F && v.fp3d[0] == 1.0F && v.fp3d[1] == 1.0F && v.fp3d[2] == 0.0F &&
                   v.fpNebel[0] == 127.0F / 255.0F && v.fpNebel[3] == 1.0F,
               "FixColors: R aus Bit 0-7, G 8-15, B 16-23");
        IniDatei ini;
        ini.lesen("[Configuration]\nWidth2D=0\nWidth3D=99\nOpacity=-5\nClipRange=9000\nZBuffer=3\nColor2D=-1\nRender2D=2\n");
        const PortalEinstellungen b = portalEinstellungenAus(ini);
        pruefe(b.breite2d == 1 && b.breite3d == 10 && b.deckkraft == 0 && b.ausschnittWeite == 8192 && b.zPuffer == 0 && b.farbe2d == 0xFFFFFFU &&
                   b.zeige2d,
               "LoadConfig: begrenzt, ZBuffer 3 -> 0, Farbe & 0xFFFFFF, Render2D 2 -> an");
        const std::string soll =
            "[Configuration]\nClip=0        ; Cubic clipper active for portal viewer\n"
            "ClipRange=1024        ; Portal viewer cubic clip distance (in units of 64)\n"
            "Color2D=16711680        ; Color of lines in 2D windows\nColor3D=65535        ; Color of lines/polygons in 3D window\n"
            "ColorFog=8355711        ; Color of distant lines/polygons in 3D window\nDrawHints=1        ; Draw Hint Portals\n"
            "DrawNonHints=1        ; Draw Regular Portals\nFog=0        ; Use depth cueing in 3D window\n"
            "Lines=1        ; Render using lines in 3D window\nOpacity=50        ; Opacity in 3d view (0 = invisible, 100 = solid)\n"
            "Polygons=1        ; Render using polygons in 3D window\nRender2D=0        ; Draw in 2D windows\n"
            "Render3D=1        ; Draw in 3D windows\nWidth2D=3        ; Width of lines in 2D windows\n"
            "Width3D=3        ; Width of lines in 3D window\nZBuffer=1        ; ZBuffer level in 3D window\n";
        const std::string ist = portalEinstellungenIni(v).schreiben();
        pruefe(ist == soll, "SaveConfig: Werte mit Kommentar, sortiert:\n" + ist);
        IniDatei zurueck;
        zurueck.lesen(ist);
        const PortalEinstellungen r = portalEinstellungenAus(zurueck);
        pruefe(r.breite2d == 3 && r.farbe3d == v.farbe3d && r.deckkraft == 50 && r.zPuffer == 1 && r.hints, "SaveConfig -> LoadConfig: gleich");
    }
    // --- rand()
    {
        PortalZufall z;
        const int a = z.naechste();
        const int b = z.naechste();
        const int c = z.naechste();
        const int d = z.naechste();
        pruefe(a == 41 && b == 18467 && c == 6334 && d == 26500, "rand(): 41 18467 6334 26500 wie die C-Laufzeit von Windows");
    }
    if (argc < 2) {
        pruefe(false, "Aufruf ohne tests/karte/q3map2");
        std::printf("karteportale: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
        return 1;
    }
    const std::string daten = argv[1];
    // --- CPortals::Load an einer echten .prt
    {
        const std::string text = lies(daten + "/raum.nrc.prt");
        std::vector<BspPortal> p;
        bool hint = false;
        PortalZufall z;
        const auto m = portaleLaden("x/raum.prt", &text, p, hint, z);
        pruefe(meldungen(m) == "0:Portal Viewer plugin: Loading portal file x/raum.prt.\n0:  10 portals read in.\n", "Load: Meldungen\n" + meldungen(m));
        pruefe(p.size() == 10 && hint, "Load: 10 Portale, Hint-Flags (vierte Kopfzeile: Flaechenzahl)");
        if (p.size() == 10) {
            const BspPortal& a = p[0];
            const Vektor3f mitte = (Vektor3f(8, 0, 0) + Vektor3f(512, 0, 0) + Vektor3f(512, 0, 256) + Vektor3f(8, 0, 256)) / 4;
            pruefe(a.punkte.size() == 4 && a.punkte[1] == Vektor3f(512, 0, 0) && a.punkte[3] == Vektor3f(8, 0, 256) && !a.hint,
                   "Load: Portal 1 \"4 0 2 0 (8 0 0 ) (512 0 0 ) ...\"");
            pruefe(a.mitte == mitte && a.mitte == Vektor3f(260, 0, 128), "Load: Mitte (260 0 128)");
            pruefe(a.innen.size() == 4 && a.innen[0] == mitte * 0.01F + Vektor3f(8, 0, 0) * 0.99F, "Load: innere Punkte 1 % zur Mitte");
            pruefe(a.min == Vektor3f(8, 0, 0) && a.max == Vektor3f(512, 0, 256), "Load: Grenzen");
            pruefe(a.farbe[0] == 41.0F / 255.0F && a.farbe[1] == 35.0F / 255.0F && a.farbe[2] == 190.0F / 255.0F && a.farbe[3] == 1.0F,
                   "Load: Farbe aus rand() & 0xff (41, 35, 190)");
        }
    }
    // --- Kopf PRT2 und PRT1-AM, Hint-Flag 1, ohne Hint-Flags
    {
        const std::string prt2 = "PRT2\n5\n2\n1\n3 0 1 (0 0 0) (1 0 0) (1 1 0)\n";
        std::vector<BspPortal> p;
        bool hint = true;
        PortalZufall z;
        const auto m = portaleLaden("a.prt", &prt2, p, hint, z);
        pruefe(p.size() == 1 && !hint && p[0].punkte[2] == Vektor3f(1, 1, 0), "PRT2: Knoten, Cluster, Portale; ohne Hint-Flags");
        const std::string am = "PRT1-AM\n2\n1\n5\n9\n3 0 1 1 (0 0 0) (1 0 0) (1 1 0)\n";
        const auto m2 = portaleLaden("b.prt", &am, p, hint, z);
        pruefe(p.size() == 1 && hint && p[0].hint, "PRT1-AM: Cluster, Portale, Knoten; Hint-Flag 1");
        (void)m;
        (void)m2;
    }
    // --- Fehler: jede Meldung wortgleich, danach leer
    {
        struct Fall {
            const char* text;
            const char* meldung;
        };
        const Fall faelle[] = {
            {"", "2:  ERROR - File ended prematurely.\n"},
            {"PRT3\n", "2:  ERROR - File header indicates wrong file type (should be \"PRT1\" or \"PRT2\" or \"PRT1-AM\").\n"},
            {"PRT1\n4\n", "2:  ERROR - File ended prematurely.\n"},
            {"PRT1\n4\n0\n", "2:  ERROR - number of portals equals 0, aborting.\n"},
            {"PRT1\n4\n65536\n", "2:  ERROR - Extreme number of portals, aborting.\n"},
            {"PRT1\n4\n2\n3 0 1 (0 0 0) (1 0 0) (1 1 0)\n", "2:  ERROR - Could not find information for portal number 2 of 2.\n"},
            {"PRT1\n4\n2\n3 0 1 (0 0 0) (1 0 0) (1 1 0)\nkaputt\n", "2:  ERROR - Information for portal number 2 of 2 is not formatted correctly.\n"},
            {"PRT1\n4\n1\n3 0 1 (0 0 0) (1 0 0)\n", "2:  ERROR - Information for portal number 1 of 1 is not formatted correctly.\n"},
        };
        for (const Fall& f : faelle) {
            const std::string t = f.text;
            std::vector<BspPortal> p(3);
            bool hint = false;
            PortalZufall z;
            const auto m = portaleLaden("f.prt", &t, p, hint, z);
            const std::string soll = std::string("0:Portal Viewer plugin: Loading portal file f.prt.\n") + f.meldung;
            pruefe(meldungen(m) == soll && p.empty(), std::string("Load-Fehler: ") + f.meldung + "  ist: " + meldungen(m));
        }
        std::vector<BspPortal> p(2);
        bool hint = false;
        PortalZufall z;
        const auto m = portaleLaden("weg.prt", nullptr, p, hint, z);
        pruefe(meldungen(m) == "0:Portal Viewer plugin: Loading portal file weg.prt.\n2:  ERROR - could not open file.\n" && p.empty(),
               "Load: Datei fehlt");
    }
    std::printf("karteportale: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
