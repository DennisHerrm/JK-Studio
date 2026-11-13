// ui_menue.h - Menues im ganzen Programm (KLEIN2, 13.11.)
//
// Befund von AP14b beim Aufnehmen der Handbuchbilder (docs/intern/NOTIZEN_klein2_1311.md):
//
//   Esc schloss das Rechtsklick-Menue des Szenen-Explorers nicht (ENTSCHEID3A C4 galt nur dem Menue der Ansicht; einzelne Menues
//      hatten eine eigene Esc-Zeile). In 3ds Max und Blender schliesst Esc jedes Kontextmenue.
//
// Das steht hier an EINER Stelle statt in jedem Menue:
//
//   menueEsc()       Esc schliesst alle offenen Menues und Rechtsklick-Menues ueber dem obersten modalen Dialog, auch aus einem
//                    Untermenue (wie 3ds Max und Blender), und verbraucht die Taste bis zum Loslassen - kein Werkzeug, keine
//                    Navigation und keine Auswahl sieht sie. Nicht, waehrend ein Textfeld schreibt (Esc gehoert dem Feld; die
//                    Suchfenster schliessen dann selbst) und nicht ohne offenes Menue. Rueckgabe: so viele Ebenen geschlossen.
//                    Aufruf am Anfang des Bildes, bevor jemand Esc liest (der Model-Tab: modellMenueEscBildAnfang, gui/app.cpp).
//                    Der Karten-Editor ruft es nicht: seine Menues schliessen wie Qt eine Ebene je Esc (app_karte_qtmenue.cpp).
#ifndef BHED_GUI_UI_MENUE_H
#define BHED_GUI_UI_MENUE_H

namespace bhed::gui::ui {

int menueEsc();

}  // namespace bhed::gui::ui

#endif
