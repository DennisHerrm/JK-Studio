// fensteranordnung.h - die Anordnung der Fenster je Bereich (Stufe dock1)
//
// shank: "Jedes Fenster/Panel herausnehmen als eigenes Windows-Fenster
// (zweiter Bildschirm), wieder andocken, Anordnung frei waehlen wie in
// Blender; Layouts speichern, benennen, wechseln, zuruecksetzen."
//
// Eine Anordnung sagt je Bereich (Events, Mission, Model, Map Editor, Menu
// Editor), welche Panels in einem eigenen Fenster stehen, welche ausgeblendet
// sind, und wo die eigenen Fenster liegen - das letzte als Auszug aus ImGuis
// Ini-Text (Fensterlagen, Bildschirm, Dock-Baum der herausgenommenen Panels).
//
// Bewusst ohne ImGui und ohne Windows: Kodieren, Lesen und Filtern sind so
// ohne Oberflaeche pruefbar (tests/fenstertest.cpp).
#ifndef BHED_FENSTERANORDNUNG_H
#define BHED_FENSTERANORDNUNG_H

#include <map>
#include <string>
#include <vector>

namespace bhed::fenster {

struct Anordnung {
    std::vector<std::string> draussen;    // Panels in eigenem Fenster (sortiert, ohne Doppelte)
    std::vector<std::string> versteckt;   // ausgeblendete Panels (sortiert, ohne Doppelte)
    std::string ini;                      // Auszug aus ImGuis Ini-Text (iniFiltern)
    bool operator==(const Anordnung&) const = default;
};

// Kennzeichen der Panelfenster im ImGui-Namen: "<Titel>###dock.<panel>".
inline constexpr const char* kFensterKennung = "###dock.";
// So steht es in ImGuis Ini-Text: der Name ab "###" ("[Window][dock.<panel>]").
inline constexpr const char* kIniKennung = "dock.";

// Eine Zeile fuer die Einstellungsdatei (ohne Zeilenumbruch) und zurueck.
// dekodieren lehnt fremde oder kaputte Zeilen ab und laesst `a` dann leer.
[[nodiscard]] std::string kodieren(const Anordnung& a);
[[nodiscard]] bool dekodieren(const std::string& zeile, Anordnung& a);

// Aus ImGuis Ini-Text nur das, was zu Panelfenstern gehoert: die Abschnitte
// [Window][dock.<praefix>...] und aus [Docking][Data] die Dock-Baeume -
// ohne Praefix alle, mit Praefix nur die, in denen ein Fenster des Bereichs
// steckt. Alles andere (Tabellen, Dialoge) bleibt draussen - behaved haelt
// seine Aufteilung selbst (io.IniFilename = nullptr, siehe main_win32.cpp).
[[nodiscard]] std::string iniFiltern(const std::string& ini, const std::string& praefix = "");

// Den Teil eines Bereichs in einem Ini-Text austauschen: aus `basis` alle
// Fenster mit dem Praefix und ihre Dock-Baeume heraus, die aus `teil` hinein.
// ImGui nummeriert schwebende Dock-Knoten von 1 an (DockContextGenNodeID) -
// Knoten aus `teil`, deren Nummer in `basis` schon vergeben ist, bekommen eine
// freie (samt Parent= und DockId= der Fenster). So bleiben die Baeume der
// anderen Bereiche, wenn eine gespeicherte Anordnung geladen wird
// (ImGui::LoadIniSettingsFromMemory ersetzt den ganzen Dock-Baum).
[[nodiscard]] std::string iniErsetzen(const std::string& basis, const std::string& teil, const std::string& praefix);

// Ein Name fuer eine gespeicherte Anordnung: getrimmt, ohne '=', '/' und
// Steuerzeichen, hoechstens 48 Zeichen. Leer = unbrauchbar.
[[nodiscard]] std::string nameBereinigen(const std::string& name);

// Schluessel in Settings::fenster: "b<bereich>" (die laufende Anordnung) und
// "b<bereich>/<name>" (eine gespeicherte).
[[nodiscard]] std::string schluesselLaufend(int bereich);
[[nodiscard]] std::string schluesselBenannt(int bereich, const std::string& name);
// Die Namen der gespeicherten Anordnungen eines Bereichs, sortiert.
[[nodiscard]] std::vector<std::string> benannte(const std::map<std::string, std::string>& fenster, int bereich);

// --- rc609 T1: ganze Tabs in eigenen Fenstern ---------------------------------
//
// Ein Tab (Bereich 0..4) kann in einem eigenen Windows-Fenster stehen; das
// Hauptfenster behaelt immer mindestens einen. Gespeichert werden je Fenster
// der Tab, die Lage (aeusseres Fenster in Bildschirmpunkten, wie ImGui sie
// mit Viewports fuehrt) und der Arbeitsbereich des Bildschirms, auf dem es
// stand. Zeilenform:  tab1|<m>,<x>,<y>,<b>,<h>,<bx>,<by>,<bb>,<bh>|...
inline constexpr int kTabBereiche = 5;
// Schluessel in Settings::fenster: die laufende Liste und die einer benannten
// Anordnung ("t<bereich>/<name>", gespeichert mit der Anordnung des Bereichs).
inline constexpr const char* kTabSchluessel = "tabfenster";
[[nodiscard]] std::string schluesselTabBenannt(int bereich, const std::string& name);

struct Rechteck {
    int x = 0;
    int y = 0;
    int b = 0;
    int h = 0;
    bool operator==(const Rechteck&) const = default;
};

struct TabFenster {
    int modus = 0;
    Rechteck lage;
    Rechteck bildschirm;
    bool operator==(const TabFenster&) const = default;
};

[[nodiscard]] std::string tabFensterKodieren(const std::vector<TabFenster>& liste);
// Lehnt fremde Zeilen, unbekannte oder doppelte Tabs, leere Groessen und eine
// Liste mit allen Tabs ab (das Hauptfenster haette keinen) - `liste` ist dann leer.
[[nodiscard]] bool tabFensterDekodieren(const std::string& zeile, std::vector<TabFenster>& liste);

// Wohin ein gespeichertes Fenster beim Laden kommt. Gibt es seinen Bildschirm
// noch (gleicher Arbeitsbereich) oder liegt seine Mitte auf einem der
// Bildschirme, bleibt es dort; sonst kommt es mittig auf `ersatz` (der
// Bildschirm des Hauptfensters). Auf dem Ziel wird es, wenn noetig,
// verkleinert und hineingeschoben, damit der ganze Rand erreichbar ist.
// `bildschirm` ist danach der Arbeitsbereich des Ziels.
[[nodiscard]] TabFenster aufBildschirm(const TabFenster& f, const std::vector<Rechteck>& bildschirme, const Rechteck& ersatz);

// Eine Kennung in eine sortierte Liste ohne Doppelte einfuegen / entfernen.
void einfuegen(std::vector<std::string>& liste, const std::string& id);
void entfernen(std::vector<std::string>& liste, const std::string& id);
[[nodiscard]] bool enthaelt(const std::vector<std::string>& liste, const std::string& id);

}  // namespace bhed::fenster
#endif
