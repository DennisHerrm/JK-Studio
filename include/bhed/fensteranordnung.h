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

// Eine Kennung in eine sortierte Liste ohne Doppelte einfuegen / entfernen.
void einfuegen(std::vector<std::string>& liste, const std::string& id);
void entfernen(std::vector<std::string>& liste, const std::string& id);
[[nodiscard]] bool enthaelt(const std::vector<std::string>& liste, const std::string& id);

}  // namespace bhed::fenster
#endif
