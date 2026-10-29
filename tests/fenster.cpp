// Probe fuer bhed/fensteranordnung.h (Stufe dock1): die Anordnung der Fenster
// je Bereich - Kodieren und Lesen, der Auszug aus ImGuis Ini-Text, Namen der
// gespeicherten Anordnungen und der Weg durch die Einstellungsdatei.
#include "bhed/fensteranordnung.h"
#include "bhed/settings.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace bhed;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("%s %s\n", ok ? "OK    " : "FEHLER", was.c_str());
    if (!ok) { ++g_fehler; }
}

// Ein Ini-Text, wie ImGui 1.92.9 (docking) ihn schreibt: Panelfenster mit
// eigenem Bildschirmfenster, ein Dialog, eine Tabelle und der Dock-Baum.
// ImGui schreibt den Fensternamen ab "###" ("[Window][dock.mission.zeitleiste]",
// so im Selbsttest dock1 gesehen), mit io.ConfigDebugIniSettings den ganzen
// ("[Window][Script Flow###dock.ereignisse.fluss]") - beides zaehlt.
const char* const kIni =
    "[Window][Debug##Default]\n"
    "Pos=60,60\n"
    "Size=400,400\n"
    "\n"
    "[Window][Script Flow###dock.ereignisse.fluss]\n"
    "Pos=1930,120\n"
    "Size=640,900\n"
    "ViewportPos=1930,120\n"
    "ViewportId=0x1A2B3C4D\n"
    "\n"
    "[Window][dock.mission.zeitleiste]\n"
    "Pos=10,700\n"
    "Size=1200,300\n"
    "DockId=0x00000002,0\n"
    "\n"
    "[Window][undock.mission.x]\n"
    "Pos=1,1\n"
    "\n"
    "[Window][Mein dock.mission.y]\n"
    "Pos=2,2\n"
    "\n"
    "[Window][Find###find]\n"
    "Pos=300,200\n"
    "Size=230,120\n"
    "\n"
    "[Table][0xABCDEF01,3]\n"
    "Column 0  Width=120\n"
    "\n"
    "[Docking][Data]\n"
    "DockNode ID=0x00000002 Pos=10,700 Size=1200,300 Selected=0x11111111\n";
}  // namespace

int main() {
    // --- Kodieren und Lesen -------------------------------------------------
    fenster::Anordnung a;
    fenster::einfuegen(a.draussen, "mission.zeitleiste");
    fenster::einfuegen(a.draussen, "mission.ansicht");
    fenster::einfuegen(a.draussen, "mission.ansicht");   // doppelt - bleibt einmal
    fenster::einfuegen(a.versteckt, "mission.ebenen");
    a.ini = "[Window][A\\B###dock.x]\nPos=1,2\n\n[Docking][Data]\n";
    pruefe(a.draussen == std::vector<std::string>{"mission.ansicht", "mission.zeitleiste"}, "einfuegen: sortiert, ohne Doppelte");
    const std::string z = fenster::kodieren(a);
    pruefe(z.find('\n') == std::string::npos && z.find('\r') == std::string::npos, "kodiert: eine Zeile");
    fenster::Anordnung b;
    pruefe(fenster::dekodieren(z, b) && b == a, "dekodieren(kodieren(a)) == a (mit \\ und Zeilenumbruechen)");
    fenster::Anordnung leer;
    pruefe(fenster::dekodieren(fenster::kodieren(leer), b) && b == leer, "leere Anordnung");
    pruefe(!fenster::dekodieren("dock0|d=|v=|i=", b) && b == leer, "fremder Kopf abgelehnt, Ergebnis leer");
    pruefe(!fenster::dekodieren("dock1|d=Gross|v=|i=", b), "ungueltige Kennung abgelehnt");
    pruefe(!fenster::dekodieren("dock1|d=a|v=|i=kaputt\\", b), "abgebrochenes Fluchtzeichen abgelehnt");
    pruefe(!fenster::dekodieren("dock1|d=a|i=x", b), "fehlendes Feld v abgelehnt");
    pruefe(fenster::dekodieren("dock1|d=a|v=|i=Titel | mit Strich", b) && b.ini == "Titel | mit Strich",
           "Strich im Ini-Text bleibt (letztes Feld)");

    // --- Auszug aus dem Ini-Text --------------------------------------------
    const std::string alle = fenster::iniFiltern(kIni);
    pruefe(alle.find("###dock.ereignisse.fluss]") != std::string::npos && alle.find("ViewportPos=1930,120") != std::string::npos,
           "Panelfenster mit Bildschirmlage behalten");
    pruefe(alle.find("[Window][dock.mission.zeitleiste]") != std::string::npos && alle.find("DockId=0x00000002,0") != std::string::npos,
           "zweites Panelfenster mit Dock-Kennung behalten");
    pruefe(alle.find("[Docking][Data]") != std::string::npos && alle.find("DockNode ID=0x00000002") != std::string::npos,
           "Dock-Baum behalten");
    pruefe(alle.find("Debug##Default") == std::string::npos && alle.find("###find") == std::string::npos &&
               alle.find("[Table]") == std::string::npos && alle.find("Column 0") == std::string::npos &&
               alle.find("undock.") == std::string::npos && alle.find("Mein dock.") == std::string::npos,
           "Dialoge, ImGuis Debug-Fenster und Tabellen draussen");
    const std::string nurMission = fenster::iniFiltern(kIni, "mission.");
    pruefe(nurMission.find("[Window][dock.mission.zeitleiste]") != std::string::npos &&
               nurMission.find("###dock.ereignisse.fluss]") == std::string::npos,
           "Praefix: nur die Panels eines Bereichs");
    pruefe(fenster::iniFiltern("[Window][X###dock.a]\r\nPos=1,1\r\n") == "[Window][X###dock.a]\nPos=1,1\n\n",
           "CRLF wird zu LF");
    pruefe(nurMission.find("DockNode ID=0x00000002") != std::string::npos, "Praefix: Dock-Baum des Bereichs mit");
    pruefe(fenster::iniFiltern(kIni, "ereignisse.").find("[Docking][Data]") == std::string::npos,
           "Praefix: fremder Dock-Baum draussen");

    // --- Einen Bereich austauschen (gespeicherte Anordnung laden) ---------------
    // Basis: ein Baum der Mission (Knoten 1 und 2) und einer des Model-Tabs
    // (Knoten 3 mit Kindern 4, 5). Teil: eine gespeicherte Mission-Anordnung,
    // deren Knoten ebenfalls 3, 4 und 5 heissen - wie ImGui sie von 1 an zaehlt.
    const char* const basis =
        "[Window][Zeitleiste###dock.mission.zeitleiste]\nPos=1,1\nSize=9,9\nDockId=0x00000002,0\n\n"
        "[Window][Leiste###dock.modell.leiste]\nPos=2,2\nSize=8,8\nDockId=0x00000004,0\n\n"
        "[Window][Ansicht###dock.modell.ansicht]\nPos=3,3\nSize=7,7\nDockId=0x00000005,0\n\n"
        "[Docking][Data]\n"
        "DockNode    ID=0x00000001 Pos=1,1 Size=9,9 Split=Y\n"
        "  DockNode  ID=0x00000002 Parent=0x00000001 SizeRef=9,4 Selected=0x1\n"
        "DockNode    ID=0x00000003 Pos=2,2 Size=8,8 Split=X\n"
        "  DockNode  ID=0x00000004 Parent=0x00000003 SizeRef=4,8 Selected=0x2\n"
        "  DockNode  ID=0x00000005 Parent=0x00000003 SizeRef=4,8 Selected=0x3\n";
    const char* const teil =
        "[Window][Zeitleiste###dock.mission.zeitleiste]\nPos=50,60\nSize=70,80\nDockId=0x00000004,1\n\n"
        "[Window][Ansicht###dock.mission.ansicht]\nPos=50,60\nSize=70,80\nDockId=0x00000005,0\n\n"
        "[Docking][Data]\n"
        "DockNode    ID=0x00000003 Pos=50,60 Size=70,80 Split=X\n"
        "  DockNode  ID=0x00000004 Parent=0x00000003 SizeRef=35,80 Selected=0x9\n"
        "  DockNode  ID=0x00000005 Parent=0x00000003 SizeRef=35,80 Selected=0x8\n";
    const std::string m = fenster::iniErsetzen(basis, teil, "mission.");
    pruefe(m.find("Pos=1,1") == std::string::npos && m.find("ID=0x00000001") == std::string::npos,
           "alter Mission-Baum und alte Lage der Zeitleiste weg");
    pruefe(m.find("ID=0x00000003 Pos=2,2") != std::string::npos && m.find("DockId=0x00000004,0") != std::string::npos &&
               m.find("DockId=0x00000005,0") != std::string::npos,
           "Baum des Model-Tabs unveraendert");
    pruefe(m.find("ID=0x00000006 Pos=50,60") != std::string::npos && m.find("ID=0x00000007 Parent=0x00000006") != std::string::npos &&
               m.find("ID=0x00000008 Parent=0x00000006") != std::string::npos,
           "Knoten der geladenen Anordnung neu nummeriert (3,4,5 -> 6,7,8), Parent mit");
    pruefe(m.find("Pos=50,60\nSize=70,80\nDockId=0x00000007,1") != std::string::npos &&
               m.find("Ansicht###dock.mission.ansicht]\nPos=50,60\nSize=70,80\nDockId=0x00000008,0") != std::string::npos,
           "DockId der geladenen Fenster folgt der neuen Nummer");
    pruefe(fenster::iniFiltern(m, "modell.") == fenster::iniFiltern(basis, "modell."), "Model-Teil bytegleich wie vorher");
    const std::string ohne = fenster::iniErsetzen(basis, "", "mission.");
    pruefe(ohne.find("mission.") == std::string::npos && ohne.find("ID=0x00000003") != std::string::npos,
           "leerer Teil: Bereich entfernt, Rest bleibt");

    // --- Namen gespeicherter Anordnungen ---------------------------------------
    pruefe(fenster::nameBereinigen("  Zwei Bildschirme  ") == "Zwei Bildschirme", "Name getrimmt");
    pruefe(fenster::nameBereinigen("a=b/c|d\te") == "abcde", "'=', '/', '|' und Steuerzeichen entfernt");
    pruefe(fenster::nameBereinigen("   ").empty(), "nur Leerzeichen = unbrauchbar");
    const std::string lang = fenster::nameBereinigen(std::string(47, 'x') + "\xC3\xA4" + "yyyy");
    pruefe(lang.size() == 47, "48 Zeichen, nicht mitten in einem UTF-8-Zeichen geschnitten");

    // --- Durch die Einstellungsdatei ------------------------------------------
    Settings s;
    s.fenster[fenster::schluesselLaufend(1)] = fenster::kodieren(a);
    s.fenster[fenster::schluesselBenannt(1, "Zwei Bildschirme")] = fenster::kodieren(leer);
    s.fenster[fenster::schluesselBenannt(1, "Abend")] = fenster::kodieren(a);
    s.fenster[fenster::schluesselBenannt(3, "Karte")] = fenster::kodieren(leer);
    Settings t;
    pruefe(readSettings(writeSettings(s), t) && t.fenster == s.fenster, "Settings::fenster ueberlebt Schreiben und Lesen");
    fenster::Anordnung c;
    pruefe(fenster::dekodieren(t.fenster[fenster::schluesselLaufend(1)], c) && c == a, "laufende Anordnung nach dem Lesen gleich");
    pruefe(fenster::benannte(t.fenster, 1) == std::vector<std::string>{"Abend", "Zwei Bildschirme"},
           "benannte(): nur die des Bereichs, sortiert");
    pruefe(fenster::benannte(t.fenster, 2).empty(), "benannte(): leerer Bereich");

    std::printf("%s: %d Fehler\n", g_fehler == 0 ? "ALLES OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
