// REST612 (rc612): Liest die Einstellungsdatei nach dem Umbau von readSettings (Tabelle Schluessel -> Leser statt der
// else-if-Kette am Compilerlimit C1061) genauso wie vorher?
//
// Jeder Schluessel, den readSettings vor dem Umbau kannte, bekommt in je einem Abschnitt einen der Werte aus kWerte
// (gueltig, an und ueber den Grenzen, Unsinn, leer) - so laeuft jeder Leser durch jeden Zweig. Dazu ein Abschnitt mit
// den Sonderformen (Lesezeichen mit '=' im Pfad, Listen ueber ihrer Obergrenze, Tasten ohne Zahl, Kommentare,
// Leerraum, doppelte Schluessel). Je Abschnitt wird eine frische Settings gelesen und von writeSettings jede Zeile
// behalten, die in der Vorgabe nicht vorkommt (writeSettings schreibt jeden einfachen Schluessel genau einmal, eine
// geaenderte Zeile ersetzt also die der Vorgabe; Listen sind in der Vorgabe leer). Das Ergebnis muss Zeile fuer Zeile
// tests/einstellungen/alle_erwartet.txt gleichen, erzeugt mit der else-if-Kette VOR dem Umbau (--schreiben).
//
// Neue Schluessel brauchen hier keinen Eintrag: Sie stehen in der Vorgabe und im Ergebnis gleich und fallen aus dem
// Vergleich heraus. Die Liste unten ist der Stand vor dem Umbau.
//
//   einstellungenalle <erwartet.txt>              vergleichen (ctest)
//   einstellungenalle --schreiben <erwartet.txt>  Erwartung neu schreiben (nur mit dem alten Leser!)
//   einstellungenalle --datei <behaved.cfg>       writeSettings(readSettings(datei)) ausgeben (echte Dateien alt/neu)
#include "bhed/settings.h"

#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

const char* const kSchluessel[] = {
    "grafikRueckfallFassung", "windowPlacement", "tonEffekte", "tonStimmen", "tonMusik", "ersteSchritteGezeigt", "kiChat",
    "kiModell", "kiOhneNachfrage", "mcpHttp", "mcpPort", "changeHistory", "updateCheck", "grafikD3d12",
    "updateUebersprungen", "highlightSame", "scriptPath", "ibizePath", "commandFile", "sourcePath", "reopenLastFile",
    "legalExit", "alphaSortPulldowns", "alternativeIcons", "queryOnDiscard", "dialogUsesLastDir", "lastDir", "mapPath",
    "language", "theme", "showTypes", "menueReiter", "menueRechtsBreite", "menueTextUnten", "menueGanzeDatei",
    "menueDateiSchluessel", "menueTeileHinweisWeg", "menueTextZoom", "menueLetzteFarben", "menueTextUmbruch", "menueTon",
    "menueLeisteText", "pk3SpeicherWeg", "missionsordnerAblauf", "skriptVorlage", "fs_dirbeforepak", "entTextOffen",
    "insSkriptOffen", "modelBackground", "modelViewCube", "modelViewCubeAll", "modelCompass", "modelCompassNorth",
    "modelOrbit", "modelZoomMouseOrtho", "modelZoomMousePersp", "modelBlenderOrbitSelection", "modelBlenderZoomToMouse",
    "modelGridSpacing", "modelGridMajor", "modelGridExtent", "modelDisplayMovedSeen", "modelPanelWidth", "modelHome",
    "lumpEditOffen", "lumpEditKarteOffen", "texListeOffen", "gFloats", "foldMacros", "uiScale", "splitEvents",
    "splitMap", "splitModel", "splitStatus", "timelineFrac", "splitFracX", "splitFracY", "timelineFrames",
    "schattenArt", "nebel", "dynLicht", "spielerInCutscene", "cgMotionBoneComp", "cgTurnAnims",
    "gActivateAnimationStyle", "mpZiel", "cgSaberLightIntensity", "cgSaberLightRadius", "spielSeiten",
    "fovAspectAdjust", "spielRahmenFrei", "difficulty", "r_picmip", "modelControls", "modelTimelineTracks",
    "modelTimelineAutoKey", "modelBlenderToolNames", "modelBlenderToolbarWidth", "modelTimelineTracksMax",
    "modelTimelineTracksBlender", "modelTimelineDisplay", "modelBoneFins", "modelBoneDisplay", "modelRigStyle",
    "modelBonesInFront", "r_ext_multisample", "regelwerk", "flussAnsicht", "mapSidebar", "splitButtons", "gamePath",
    "gamePathJk2", "key", "recent", "recentMission", "karteZuletzt", "karte", "karteTaste", "fenster",
    // Schluessel mit Vorsatz (der Rest ist frei)
    "breite.Ereignis", "helfer.MoveUp.x", "lesezeichen.C:/a/b.txt", "tour.karte",
};

// Je Abschnitt ein Wert fuer alle Schluessel: Wahrheitswerte in allen Schreibweisen, Zahlen an und neben den Grenzen
// der Leser, Gleitkomma, Unsinn, leer und die Woerter, die einzelne Leser kennen.
const char* const kWerte[] = {
    "yes", "true", "1", "0", "no", "-1", "-2", "2", "3", "4", "7", "8", "12", "50", "100", "300", "301", "1023", "8080",
    "65535", "70000", "0.02", "0.05", "0.1", "0.2", "0.25", "0.35", "0.5", "0.6", "0.85", "0.9", "0.97", "0.98", "1.5",
    "2.5", "26", "44", "200", "201", "359", "360", "-30", "1024.5", "2000", "2001", "1e9", "nan", "abc", "",
    "claude-opus-5-5", "claude-sonnet-5-5", "claude-haiku-5-5", "claude-3", "blender", "max", "openjk", "movieduels",
    "auto", "MoveUp 4194643", "MoveUp x", " MoveUp 1", "Name=Wert", "=Wert", "rc611",
};

// Die Sonderformen, die kein Wert aus kWerte trifft.
const char* const kSonder =
    "# Kommentar\n"
    "   \t\n"
    "ohne Gleichheitszeichen\n"
    "  language  =  de  \r\n"
    "lesezeichen.C:/a=b/skript.txt=3,7,12\n"
    "lesezeichen. C:/x.txt = 1 \n"
    "helfer.Run.zeigen=1\n"
    "helfer.=leer\n"
    "tour.=leer\n"
    "breite.alt=99\n"
    "key=MoveUp 4194643\n"
    "key=MoveDown 12x\n"
    "key=Kaputt\n"
    "key= 5\n"
    "key=Run 99999999999\n"
    "karte=Gitter=8\n"
    "karte=Gitter=16\n"
    "karte==3\n"
    "karte=ohne\n"
    "karteTaste=Rueckgaengig=Ctrl+Z\n"
    "fenster=karte=1,2,3\n"
    "fenster==x\n"
    "splitMap=0.5\n"
    "splitMap=0.2\n"
    "splitModel=0.6\n"
    "splitModel=0.06\n"
    "tonEffekte=0.25\n"
    "tonStimmen=0.5\n"
    "tonMusik=0.75\n"
    "tonMusik=2\n"
    "cgSaberLightIntensity=1.5\n"
    "cgSaberLightRadius=2.25\n"
    "cgSaberLightRadius=9\n"
    "unbekannt=1\n"
    "Language=en\n";

std::vector<std::string> zeilen(const std::string& t) {
    std::vector<std::string> z;
    std::istringstream in(t);
    std::string l;
    while (std::getline(in, l)) {
        if (!l.empty() && l.back() == '\r') { l.pop_back(); }
        z.push_back(l);
    }
    return z;
}

// Die Zeilen des Ergebnisses, die in der Vorgabe nicht (so oft) vorkommen, in der Reihenfolge des Ergebnisses.
void abschnitt(std::string& o, const std::string& titel, const std::string& text, const std::vector<std::string>& vorgabe) {
    bhed::Settings s;
    (void)bhed::readSettings(text, s);
    std::map<std::string, int> rest;
    for (const std::string& l : vorgabe) { ++rest[l]; }
    o += "== " + titel + "\n";
    for (const std::string& l : zeilen(bhed::writeSettings(s))) {
        if (rest[l]-- <= 0) { o += l + "\n"; }
    }
}

std::string ergebnis() {
    const std::vector<std::string> vorgabe = zeilen(bhed::writeSettings(bhed::Settings{}));
    std::string o;
    int n = 0;
    for (const char* w : kWerte) {
        std::string text;
        for (const char* k : kSchluessel) { text += std::string(k) + "=" + w + "\n"; }
        abschnitt(o, "Wert " + std::to_string(++n) + " \"" + w + "\"", text, vorgabe);
    }
    // Listen ueber ihrer Obergrenze (recent 20, recentMission 12, karteZuletzt 9, modelHome 64), Reihenfolge bleibt.
    std::string listen;
    for (int i = 0; i < 70; ++i) {
        const std::string z = std::to_string(i);
        listen += "recent=r" + z + "\nrecentMission=m" + z + "\nkarteZuletzt=k" + z + "\nmodelHome=h" + z + "\ngamePath=g" + z +
                  "\ngamePathJk2=j" + z + "\n";
    }
    abschnitt(o, "Listen", listen, vorgabe);
    abschnitt(o, "Sonderformen", kSonder, vorgabe);
    return o;
}

bool lesen(const char* pfad, std::string& t) {
    std::ifstream f(pfad, std::ios::binary);
    if (!f) { return false; }
    std::ostringstream ss;
    ss << f.rdbuf();
    t = ss.str();
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--datei") {
        std::string t;
        if (!lesen(argv[2], t)) { std::printf("FEHLER %s nicht lesbar\n", argv[2]); return 1; }
        bhed::Settings s;
        (void)bhed::readSettings(t, s);
        std::fputs(bhed::writeSettings(s).c_str(), stdout);
        return 0;
    }
    if (argc == 3 && std::string(argv[1]) == "--schreiben") {
        std::ofstream f(argv[2], std::ios::binary);
        f << ergebnis();
        std::printf("geschrieben: %s\n", argv[2]);
        return f ? 0 : 1;
    }
    if (argc != 2) { std::printf("Aufruf: einstellungenalle <erwartet.txt>\n"); return 2; }
    std::string erwartetText;
    if (!lesen(argv[1], erwartetText)) { std::printf("FEHLER %s nicht lesbar\n", argv[1]); return 1; }
    const std::vector<std::string> soll = zeilen(erwartetText);
    const std::vector<std::string> ist = zeilen(ergebnis());
    int fehler = 0;
    std::string titel;
    for (std::size_t i = 0; i < soll.size() || i < ist.size(); ++i) {
        const std::string a = i < soll.size() ? soll[i] : std::string("(Ende)");
        const std::string b = i < ist.size() ? ist[i] : std::string("(Ende)");
        if (b.rfind("== ", 0) == 0) { titel = b; }
        if (a != b) {
            if (++fehler <= 10) { std::printf("FEHLER Zeile %zu (%s): erwartet \"%s\", gelesen \"%s\"\n", i + 1, titel.c_str(), a.c_str(), b.c_str()); }
        }
    }
    std::printf("%s %zu Zeilen (%zu Abschnitte, %zu Schluessel je Abschnitt), %d verschieden\n", fehler ? "FEHLER" : "OK    ",
                ist.size(), std::size(kWerte) + 2, std::size(kSchluessel), fehler);
    return fehler ? 1 : 0;
}
