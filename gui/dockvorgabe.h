// dockvorgabe.h - Map Editor und Menu Editor im DockSpace (Stufe dock4)
//
// Beide Bereiche haben eigene Teiler: der Map Editor NRCs QSplitter (Qts
// Rechnung, Griffe mit Punkten, Einklappen, MaximizeView), der Menu Editor
// seine Spalten mit Ziehkanten. Im DockSpace bleibt diese Rechnung die Vorgabe:
// der Bereich misst je Bild wie bisher, wo jedes Panel steht, und zeichnet
// seine Griffe; die Blaetter des Dock-Baums bekommen genau diese Rechtecke -
// solange der Baum der Vorgabe entspricht. Ein herausgenommenes oder
// ausgeblendetes Panel darf dabei fehlen (ImGui verschmilzt sein Blatt mit dem
// Nachbarn, der Bereich gibt seinen Platz den Nachbarn - wie vorher). Steht ein
// Panel woanders (Reiter, andere Teilung, anderes Panel in seinem Blatt),
// gelten ImGuis Teiler.
//
// Je Vorgabe (NRC-Anordnung samt Belegung der Split-Plaetze, Layout des
// Menue-Editors) ein eigener Baum mit fester Wurzel "bhed.dockraum.<schluessel>".
// ImGui schreibt ihn mit den Panels in die Einstellungen (fenster_dock,
// merkenLaufend); Neustart und gespeicherte Anordnungen bringen ihn zurueck.
// Kein Blatt ist Mittelknoten: ein leeres Blatt gibt seinen Platz immer ab.
#ifndef BHED_GUI_DOCKVORGABE_H
#define BHED_GUI_DOCKVORGABE_H

#include "imgui.h"
#include "imgui_internal.h"

#include <string>
#include <vector>

namespace bhed::gui::dockvorgabe {

struct Knoten {
    int panel = -1;            // Blatt: Index in Vorgabe::panels
    int kind[2] = {-1, -1};    // Teilung: erstes (links bzw. oben) und zweites Kind
    bool untereinander = false;
};

// Der Baum einer Anordnung, von den Blaettern her aufgebaut: der zuletzt
// angelegte Knoten ist die Wurzel.
struct Vorgabe {
    std::string schluessel;
    std::vector<Knoten> knoten;
    std::vector<std::string> panels;   // Kennungen wie in fenster_dock
    int wurzel = -1;
    int blatt(const char* panel);
    int teilung(bool untereinander, int a, int b);
};

// Vor dem Messen, je Bild: den Baum der Vorgabe pruefen (Wechsel der Vorgabe,
// Start, geladene Anordnung). true = der Baum entspricht der Vorgabe - der
// Bereich zeichnet seine Griffe, seine Rechtecke gelten.
bool beginnen(int bereich, const Vorgabe& v);
// Nach dem Messen: der DockSpace auf `flaeche` (Bildschirmpunkte) mit dem
// Knotenabstand `abstand` (der Griff zwischen zwei Feldern); `rechteck[i]` ist
// das gemessene Rechteck von panels[i] (leer: eingeklappt oder nicht da).
void dockspace(int bereich, const Vorgabe& v, const std::vector<ImRect>& rechteck, const ImRect& flaeche, float abstand);
// Ein Panel: zeichnen in diesem Bild? (eingeblendet; im Baum der Vorgabe nicht
// eingeklappt). panelBeginnen nur dann, panelEnde immer danach; der Inhalt nur,
// wenn panelBeginnen true liefert. Polster 0, im DockSpace ohne eigenen
// Hintergrund (wie die Kindfenster vorher).
bool zeigen(int bereich, const Vorgabe& v, int i, const std::vector<ImRect>& rechteck);
bool panelBeginnen(int bereich, const Vorgabe& v, int i);
void panelEnde(int bereich, const Vorgabe& v, int i);
// Nach den Panels eines Bildes.
void fertig(int bereich);
// ImGuis Dreieck der versteckten Reiterleiste (oben links in jedem Panel: Klick
// zeigt die Reiter, Ziehen nimmt das Panel heraus). ImGui zeichnet es in Begin -
// unter dem Inhalt, und der Inhalt bekaeme denselben Klick (die Ansichten fragen
// IsWindowHovered). Daher: dreieckStil vor Begin (ImGuis eigenes unsichtbar,
// PopStyleColor(farben) nach Begin); dreieckBeginnen direkt nach Begin - liegt
// die Maus darauf, wird es ueber dem Inhalt gezeichnet und der Inhalt bekommt
// keine Maus; dreieckEnde vor End.
struct Dreieck {
    ImGuiWindow* vorher = nullptr;
    bool aktiv = false;
};
void dreieckStil(int& farben);
[[nodiscard]] Dreieck dreieckBeginnen();
void dreieckEnde(const Dreieck& d);
// dock4, Randfall Schalter an/aus/an: Panels, die ohne DockSpace (Liste aus
// dock1) herausgenommen wurden, nachdem der Baum gespeichert war, schweben beim
// Uebernehmen des Baums; die Liste ist damit erledigt und wird geleert.
void alteListeSchweben(int bereich, ImGuiID wurzel, const std::vector<std::string>& panels);

// Fuer das Menue "Fenster" (fenster_dock) und den Selbsttest.
[[nodiscard]] bool draussen(int bereich, const char* panel);
void herausnehmen(int bereich, const char* panel);
void andocken(int bereich, const char* panel);
void zuruecksetzen(int bereich);
void neuLesen(int bereich);
[[nodiscard]] bool standard(int bereich);
[[nodiscard]] ImGuiID wurzelVon(int bereich);
[[nodiscard]] std::string schluesselVon(int bereich);

}  // namespace bhed::gui::dockvorgabe
#endif
