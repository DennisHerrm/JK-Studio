// ui_formular.h - der Formular-Baustein und die Mindestgroessen der Fenster (UIFORM, rc612)
//
// shank 10.11. zum Fenster "Into script": "das muss sauberer gestaltet werden das Fenster. Damit es gleichmaessiger
// aussieht" - und mit dem Bild des auf ~135 px zusammengeschobenen Fensters: "und mindestgroessen muessen gecheckt werden
// von solchen fenstern. Das bei allen".
//
// 1. Mindestgroessen. Jedes Fenster des Programms (ImGui::Begin, BeginPopupModal, die Panels aus fenster_dock.h und die
//    Tab-Fenster) beginnt ueber fensterBegin/modalBegin. Die setzen vor dem Begin die Mindestgroesse
//    (SetNextWindowSizeConstraints; ImGui wendet sie in jedem Bild an, Begin -> CalcWindowSizeAfterConstraint, also auch auf
//    eine zu kleine Groesse aus alten Einstellungen). Mindestgroesse heisst: die Bedienelemente bleiben benutzbar - Listen,
//    Baeume und Text duerfen weiter rollen bzw. am Rand enden. Sie ist das Groesste aus
//      - Titel: die Titelzeile (Knoepfe, Rand, vom Text hoechstens 12 Zeichenhoehen - laenger endet er wie bei Windows mit
//        "..."), wenigstens 10 Zeichenhoehen breit und eine Zeile Inhalt hoch,
//      - Inhalt: was der Inhalt meldet (mindestGroesse/mindestInhalt; der Formular-Baustein und die Knopfreihe melden sich
//        selbst: laengste Beschriftung + Mindest-Feldbreite + Rand, Hoehe bis unter die Knopfreihe),
//      - Bedienreihen: ragt in einem Fenster mit Fliesslayout (mit senkrechtem, ohne waagrechten Rollbalken) ein
//        Bedienelement ueber den rechten Rand, das RECHTS NEBEN einem anderen steht (Knopfreihe, Werkzeugleiste) oder das als
//        gerahmtes Element (Knopf, Feld, Ausklapper, Haken) nicht am Zeilenanfang beginnt (hinter seiner Beschriftung,
//        rechtsbuendig), ist das Fenster ab dem naechsten Bild mindestens so breit, dass es ganz hineinpasst; ein Eingabefeld,
//        das mit dem Fenster schrumpft, behaelt 6 Zeichenhoehen. Listeneintraege, Baumknoten und Text verlangen nichts.
//        Gemessen wird nur in den Bildern, in denen etwas hinausragt und der Zustand noch nicht gemessen ist (ueber die Haken
//        des Element-Registers, elemente.h) - sonst kostet es nichts.
//    Was die Bedienreihen verlangen, gilt nie ueber die groesste Groesse hinaus, die das Fenster in dieser Sitzung schon
//    hatte: kein Fenster wird dadurch groesser, als es war - es laesst sich nur nicht mehr kleiner ziehen, als seine
//    Bedienreihen brauchen. Waechst der Bedarf mit dem Fenster mit (Inhalt, der sich an der Fensterbreite ausrichtet und
//    trotzdem hinausragt), wird fuer dieses Fenster nichts mehr angehoben.
//    Keine Mindestgroesse bekommen Fenster, deren Groesse das Programm bestimmt: die sich dem Inhalt anpassen
//    (AlwaysAutoResize), die sich nicht ziehen lassen (NoResize - die Dialoge nach NetRadiant-Custom mit ihren Qt-Massen)
//    und die ihre Groesse in jedem Bild setzen. Fenster ohne Rollbalken, die ihren Platz selbst aufteilen (schwebende Panels,
//    Tabs in eigenen Fenstern), bekommen die Titel-Mindestgroesse und was ihr Inhalt meldet.
//    Alles in Bildpunkten der laufenden Schrift; wechseln Sprache oder Schriftgroesse, wird neu gemessen. Hoechstens 90 % des
//    Hauptfensters. Keine Mindestgroesse bekommt ein Fenster, das ein Dock-Knoten anlegt (angedockt, im DockSpace der Bereiche,
//    auch in dem Bild, in dem es in einen neu gebauten Knoten kommt): die Anordnung bestimmt die Groesse - ein schmal oder
//    eingeklappt gestelltes Panel bleibt so. Ist der Knoten schmaler als Inhalt oder Bedienreihen brauchen, bekommt ein Fenster
//    mit Fliesslayout einen waagrechten Rollbalken statt zu zerquetschen (die Felder des Bausteins behalten ihre Mindestbreite).
// 2. Formular-Baustein: eine Beschriftungsspalte (Breite aus der laengsten Beschriftung in der laufenden Sprache), die
//    Beschriftung auf die Feldmitte (AlignTextToFramePadding), alle Felder mit gleicher linker Kante und gleicher Breite bis
//    zum rechten Rand, Eingabe mit Vorschlagsliste als EIN Feld (Pfeil am Feld wie bei den Ausklappern), ein kleiner Knopf im
//    Feld, Hinweise und Fehler unter dem Feld, Code-Vorschau im Kasten mit Festbreitenschrift (rollt waagrecht, bricht nie
//    zeichenweise um), Knopfreihe unten rechts (Hauptknopf in der Betonungsfarbe des Themas, Abbrechen; Enter/Esc).
#ifndef BHED_GUI_UI_FORMULAR_H
#define BHED_GUI_UI_FORMULAR_H

#include "imgui.h"
#include "imgui_internal.h"

#include <cstddef>
#include <initializer_list>
#include <string>
#include <vector>

namespace bhed::gui {
enum class UiIcon : int;
}

namespace bhed::gui::ui {

// --- Mindestgroessen ------------------------------------------------------------------------------------
// Statt ImGui::Begin / ImGui::BeginPopupModal (gleiche Argumente).
bool fensterBegin(const char* name, bool* offen = nullptr, ImGuiWindowFlags flags = 0);
bool modalBegin(const char* name, bool* offen = nullptr, ImGuiWindowFlags flags = 0);
// Im Fenster: es braucht mindestens diese Fenstergroesse (mit Rand und Titel) bzw. diese Inhaltsgroesse - ab dem naechsten Bild.
void mindestGroesse(const ImVec2& fenster);
void mindestInhalt(const ImVec2& inhalt);
// Zu Beginn jedes Bildes vor dem ersten Fenster (app.cpp): die Fenster des letzten Bildes auswerten.
void bildAnfang();
// Der Haken des Element-Registers (elemente.cpp, ImGuiTestEngineHook_ItemAdd): nur in den Bildern, in denen die Bedienreihen
// eines Fensters gemessen werden, sonst sofort zurueck.
void hakenElement(ImGuiContext* ctx, ImGuiID id, const ImRect& bb);
// Selbsttest: die Untergrenze des Fensters, wie sein letztes Begin sie gesetzt hat (0/0: keine), und der Schalter (aus: wie
// vor UIFORM - fuer den Vergleich "in Ruhe nicht groesser"). Im Selbsttest steht jede Anhebung im Protokoll ("UIF hebt": das
// Begin hat das Fenster auf die Untergrenze vergroessert).
[[nodiscard]] ImVec2 mindestVon(const char* name);
void mindestSchalter(bool an);
// Selbsttest: wie oft die Mindestgroesse ein Fenster angehoben hat, dessen Groesse ein Dock-Knoten bestimmt (muss 0 sein: die
// Anordnung legt die Groesse fest), und welche (liste).
[[nodiscard]] int knotenAnhebungen(std::string* liste = nullptr);

// --- Formular-Baustein ----------------------------------------------------------------------------------
// Die Mindest-Feldbreite der Formulare in Zeichenhoehen.
inline constexpr float kMindestFeld = 14.0F;

enum class Knopf { Keiner, Haupt, Abbrechen };
// Knopfreihe unten rechts (auch ohne Formular): [haupt] in der Betonungsfarbe (nullptr: keiner), [abbrechen]. Enter loest haupt
// aus (wenn aktiv), Esc abbrechen, solange das Fenster (nicht eine offene Liste) den Fokus hat; Esc in einem Eingabefeld nimmt
// erst die Eingabe zurueck. mindestBreite: so breit steht die Reihe wenigstens (Formular: Beschriftung + Feld). Meldet ihre Breite
// als Mindestinhalt des Fensters.
Knopf knopfReihe(const char* haupt, const char* abbrechen, bool hauptAktiv = true, const char* hauptHinweis = nullptr,
                 float mindestBreite = 0.0F);

class Formular {
public:
    using Knopf = ui::Knopf;
    // beschriftungen: alle Zeilenbeschriftungen (fuer die gemeinsame Spalte); mindestFeld in Zeichenhoehen.
    Formular(std::initializer_list<const char*> beschriftungen, float mindestFeld = kMindestFeld);
    ~Formular();
    Formular(const Formular&) = delete;
    Formular& operator=(const Formular&) = delete;

    // Eine Zeile: Beschriftung auf der Feldmitte, danach steht der Cursor an der Feldkante, die Feldbreite ist gesetzt.
    void zeile(const char* beschriftung);
    // Eine Zeile ohne Beschriftung, Feld an der Feldkante (z. B. die Dauer unter dem Befehl).
    void zeileOhne();
    [[nodiscard]] float feldBreite() const { return feld_; }
    // Ein kleiner Knopf rechts IM Feld dieser Zeile (Symbol, Tooltip); imAusklapper: links vom Pfeil des Ausklappers. Nach dem
    // Feld rufen (auch nach EndCombo), vor dem Feld feldMitKnopf() (SetNextItemAllowOverlap).
    bool feldKnopf(const char* id, UiIcon icon, const char* titel, const char* hinweis, bool imAusklapper);
    static void feldMitKnopf() { ImGui::SetNextItemAllowOverlap(); }
    // Eingabe mit Vorschlagsliste als EIN Feld (Eingabe und Pfeil ohne Abstand wie ein Ausklapper, die Liste darunter in
    // Feldbreite). true: geaendert.
    bool eingabeMitListe(const char* id, char* puffer, std::size_t groesse, const char* platzhalter,
                         const std::vector<std::string>& liste);
    // Unter dem Feld: Hinweis (gedaempft) bzw. Fehler (rot), umbrochen in Feldbreite.
    void hinweis(const char* text);
    void fehler(const char* text);
    // Ueberschrift und Code-Kasten in voller Breite (Festbreitenschrift, so hoch wie die Zeilen; zu lange Zeilen rollen
    // waagrecht).
    void codeVorschau(const char* id, const char* ueberschrift, const std::string& text);
    // Knopfreihe unten rechts: [haupt] in der Betonungsfarbe, [abbrechen]. Enter loest haupt aus (wenn aktiv), Esc abbrechen,
    // solange das Fenster den Fokus hat und keine Liste offen ist.
    Knopf knoepfe(const char* haupt, const char* abbrechen, bool hauptAktiv = true, const char* hauptHinweis = nullptr);

private:
    float spalte_ = 0.0F;       // Breite der Beschriftungsspalte (mit Abstand)
    float mindestFeld_ = 0.0F;  // Bildpunkte
    float feld_ = 0.0F;         // Feldbreite dieser Zeile
    ImVec2 feldOrt_{0.0F, 0.0F};  // linke obere Ecke des Felds dieser Zeile (Bildschirm)
    float start_ = 0.0F;        // linke Kante des Inhalts (Fensterkoordinaten)
    float knopfBreite_ = 0.0F;  // Knopfreihe
};

}  // namespace bhed::gui::ui

#endif  // BHED_GUI_UI_FORMULAR_H
