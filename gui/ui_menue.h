// ui_menue.h - Menues im ganzen Programm (KLEIN2, 13.11.)
//
// Zwei Befunde von AP14b beim Aufnehmen der Handbuchbilder (docs/intern/NOTIZEN_klein2_1311.md):
//
//   1. Das Untermenue Animation > Animation tools ragte im Hauptfenster 2538 x 1472 unten hinaus. Mit ImGuiConfigFlags_ViewportsEnable
//      darf ein Popup bis an den Rand des BILDSCHIRMS reichen (GetPopupAllowedExtentRect: die Arbeitsflaeche des Monitors); was ueber
//      das Hauptfenster hinausgeht, wird ein eigenes Windows-Fenster. Im Programm war es zu sehen, auf einem Foto des Hauptfensters
//      nicht - und neben einem kleineren Hauptfenster hing das Menue frei auf dem Desktop. Wie ein Windows-Menue am Rand soll ein
//      Untermenue, das unten nicht passt, nach oben aufgehen und im Fenster bleiben.
//   2. Esc schloss das Rechtsklick-Menue des Szenen-Explorers nicht (ENTSCHEID3A C4 galt nur dem Menue der Ansicht; einzelne Menues
//      hatten eine eigene Esc-Zeile). In 3ds Max und Blender schliesst Esc jedes Kontextmenue.
//
// Beides steht hier an EINER Stelle statt in jedem Menue:
//
//   menue(text, an)  statt ImGui::BeginMenu (alle Menues der Menueleiste und alle Untermenues). Das Menue bleibt im Fenster, aus dem
//                    es aufgeht - Hauptfenster, eigenes Tab-Fenster oder schwebendes Panel: ImGui legt es dann in diesem Viewport an
//                    (SetNextWindowViewport) und rechnet die Lage in dessen Rechteck statt im Bildschirm (FindBestWindowPosForPopup;
//                    ein Untermenue, das unten nicht passt, rueckt nach oben, bis es ganz im Fenster steht). Nur ein Menue, das hoeher
//                    oder breiter als das ganze Fenster ist, geht wie bisher darueber hinaus (eigenes Fenster) - sonst waere es
//                    abgeschnitten. Hat der Aufrufer den Viewport schon gewaehlt (dock::eigenesFensterWennVerdeckt), bleibt es dabei.
//   popup(id, flags), popupKontext(id, flags), popupKontextFenster(id, flags)
//                    statt ImGui::BeginPopup / BeginPopupContextItem / BeginPopupContextWindow: dasselbe fuer die Rechtsklick-Menues
//                    der ersten Ebene (sonst ragte z. B. das Menue der Zeitleiste unten hinaus, und seine Untermenues mit ihm). Ein
//                    Popup mit gesetzter Lage (SetNextWindowPos) bleibt wie bisher - ImGui rueckt sie nicht ins Fenster.
//                    tools/lint_menue.py prueft, dass es kein direktes BeginMenu/BeginPopup* mehr gibt. Ausgenommen ist der
//                    Karten-Editor (app_karte_*, karte_qtdialog.cpp): seine Menues oeffnen wie Qt in NetRadiant-Custom bis zum
//                    Bildschirmrand (UseFullScreenForPopupMenu); im Stil des Programms nimmt er ui::menue.
//   menueEsc()       Esc schliesst alle offenen Menues und Rechtsklick-Menues ueber dem obersten modalen Dialog, auch aus einem
//                    Untermenue (wie 3ds Max und Blender), und verbraucht die Taste bis zum Loslassen - kein Werkzeug, keine
//                    Navigation und keine Auswahl sieht sie. Nicht, waehrend ein Textfeld schreibt (Esc gehoert dem Feld; die
//                    Suchfenster schliessen dann selbst) und nicht ohne offenes Menue. Rueckgabe: so viele Ebenen geschlossen.
//                    Aufruf am Anfang des Bildes, bevor jemand Esc liest (der Model-Tab: modellMenueEscBildAnfang, gui/app.cpp).
//                    Der Karten-Editor ruft es nicht: seine Menues schliessen wie Qt eine Ebene je Esc (app_karte_qtmenue.cpp).
#ifndef BHED_GUI_UI_MENUE_H
#define BHED_GUI_UI_MENUE_H

#include "imgui.h"

namespace bhed::gui::ui {

[[nodiscard]] bool menue(const char* text, bool an = true);
// TEST1411c: wohin die Untermenues von menue() aufgehen (ImGuiDir_Left / _Right; ImGuiDir_None: wie ImGui, rechts vom Menue, sonst links).
// Das Quad-Menue wie 3ds Max: die linken Viertel klappen nach links, die rechten nach rechts aus - nie ueber das Nachbarviertel.
void menueRichtung(ImGuiDir richtung);
[[nodiscard]] bool popup(const char* id, ImGuiWindowFlags flags = 0);
[[nodiscard]] bool popupKontext(const char* id = nullptr, ImGuiPopupFlags flags = 0);
[[nodiscard]] bool popupKontextFenster(const char* id = nullptr, ImGuiPopupFlags flags = 0);
int menueEsc();

}  // namespace bhed::gui::ui

#endif
