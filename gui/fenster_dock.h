// fenster_dock.h - Panels herausnehmen, andocken, Anordnungen (Stufe dock1)
//
// shank: "Jedes Fenster/Panel herausnehmen als eigenes Windows-Fenster
// (zweiter Bildschirm), wieder andocken, Anordnung frei waehlen wie in
// Blender; Layouts speichern, benennen, wechseln, zuruecksetzen."
//
// Jeder Bereich (Events, Mission, Model, Map Editor, Menu Editor) besteht aus
// Panels mit stabiler Kennung. Vorgabe: jedes Panel steht an seinem Platz im
// Hauptfenster - genau die Anordnung von vorher. Herausgenommen zeichnet
// dieselbe Zeichenfunktion in ein eigenes ImGui-Fenster ("<Titel>###dock.<id>"),
// das man aus dem Hauptfenster ziehen kann (eigenes Windows-Fenster, auch auf
// einem zweiten Bildschirm, ImGuiConfigFlags_ViewportsEnable) und mit anderen
// herausgenommenen Panels zu Reitern und Teilungen zusammensetzen kann
// (ImGuiConfigFlags_DockingEnable). Sein Platz im Hauptfenster geht an die
// Nachbarn; zurueck kommt es ueber das Menue "Fenster", den Schliessknopf des
// Fensters oder durch Ziehen auf seinen alten Platz.
//
// Die Anordnung je Bereich (bhed/fensteranordnung.h) steht in
// Settings::fenster: die laufende und beliebig viele benannte.
#ifndef BHED_GUI_FENSTER_DOCK_H
#define BHED_GUI_FENSTER_DOCK_H

#include "bhed/i18n.h"

#include <string>
#include <vector>

struct ImVec2;

namespace bhed::gui::dock {

// Die Bereiche, gezaehlt wie g_app->leftMode.
inline constexpr int kBereiche = 5;

struct Panel {
    const char* id;    // stabil, "<bereich>.<name>" - Kennung des Fensters und Schluessel der Anordnung
    int bereich;       // 0 Events, 1 Mission, 2 Model, 3 Map Editor, 4 Menu Editor
    i18n::Str titel;
};

[[nodiscard]] const std::vector<Panel>& panels();
[[nodiscard]] const Panel* panel(const std::string& id);
// Das Kennungspraefix der Panels eines Bereichs ("mission." usw.).
[[nodiscard]] const char* praefix(int bereich);
// Der ImGui-Name des Panelfensters ("<Titel>###dock.<id>").
[[nodiscard]] std::string fensterName(const char* id);

// Steht das Panel an seinem Platz im Hauptfenster (Vorgabe)?
[[nodiscard]] bool zuhause(const char* id);
[[nodiscard]] bool draussen(const char* id);
// rc609 W2b/J: Panels ohne Platz im Hauptfenster (Entity-Text, Shader und Flaechen) - ihr Schalter.
[[nodiscard]] bool* zusatzOffen(const char* id);
[[nodiscard]] bool versteckt(const char* id);
void herausnehmen(const char* id);
void andocken(const char* id);
void ausblenden(const char* id, bool aus);

// dock3: die Liste "in eigenem Fenster" einer Anordnung aus dock1. Im DockSpace
// (Events, Mission, Model) gilt ImGuis Baum; eine alte Liste wird beim ersten
// Aufbau uebernommen (die Panels schweben an ihrer Stelle) und dann geleert.
[[nodiscard]] bool listeDraussen(int bereich);
[[nodiscard]] bool inListeDraussen(const char* id);
void listeLeeren(int bereich);

// Anordnungen des Bereichs.
void zuruecksetzen(int bereich);
bool speichernUnter(int bereich, const std::string& name);
bool laden(int bereich, const std::string& name);
bool loeschen(int bereich, const std::string& name);
[[nodiscard]] std::vector<std::string> gespeicherte(int bereich);

// Zu Beginn jedes Bildes, vor dem ersten Fenster: ImGuis Fensterlagen in die
// Einstellungen schreiben und Fenster von fehlenden oder zu kleinen
// Bildschirmen zurueckholen.
void bildAnfang();
// Die Fensterlagen jetzt in die Einstellungen schreiben (sonst geschieht es
// spaetestens eine Sekunde nach einer Aenderung, io.IniSavingRate).
void jetztMerken();
// Vor ImGui::NewFrame (main_win32.cpp): die gespeicherte Anordnung beim Start
// und eine geladene Anordnung uebernehmen - zwischen zwei Bildern, wie ImGui es
// fuer LoadIniSettingsFromMemory vorsieht.
void vorBild();

// Das eigene Fenster eines herausgenommenen Panels; nur zeichnen, wenn
// fensterBeginnen true liefert, fensterEnde immer.
bool fensterBeginnen(const char* id);
void fensterEnde();

// Der Platz des Panels im Hauptfenster in diesem Bild (Bildschirmpunkte) -
// Startlage beim Herausnehmen und Ziel beim Zurueckziehen.
void heimPlatz(const char* id, const ImVec2& a, const ImVec2& b);
// Wohin das Panel beim Zurueckziehen muss: sein zuletzt gemeldeter Platz, sonst
// die Mitte des Hauptfensters (Bildschirmpunkte).
void zielPlatz(const char* id, ImVec2& a, ImVec2& b);
// Startlage eines neuen Panelfensters (vor Begin, nur fuer ein Fenster, das es
// noch nicht gibt): wo das Panel im Hauptfenster stand, etwas versetzt - man
// sieht, dass es jetzt ein eigenes Fenster ist - und ganz im Hauptfenster.
// SetNextWindowPos loest ein angedocktes Fenster aus seinem Dock-Knoten
// (BeginDocked, PosUndock), daher nur fuer neue Fenster.
void startLage(const char* id);

// Abschluss rc607: vor einem Menue der Menueleiste oder einem Dialog des Hauptfensters. Liegt ein eigenes
// Fenster (Panel, schwebendes Feld oder Gruppendialog des Karten-Editors) ueber dem Hauptfenster, bekommt
// das Menue bzw. der Dialog dort, wo es verdeckt waere, ein eigenes Fenster: ImGui legt es sonst in das
// Hauptfenster, und Windows zeigt die eigenen Fenster immer darueber (wie bei NRC liegen Menues und
// Dialoge oben).
void eigenesFensterWennVerdeckt();
// Der Inhalt des Menues "Fenster" fuer den Bereich und das Menue selbst.
void menueInhalt(int bereich);
void menue(int bereich);

// Map Editor: gibt es das Panel in der aktuellen NRC-Anordnung? (Regular hat
// kein XZ und YZ, Floating laesst alle Ansichten ohnehin schweben.)
using VerfuegbarFn = bool (*)(const char* id);
void setzeVerfuegbar(int bereich, VerfuegbarFn fn);

}  // namespace bhed::gui::dock
#endif
