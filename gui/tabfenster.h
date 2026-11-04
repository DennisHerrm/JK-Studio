// tabfenster.h - rc609 T1: ganze Tabs in eigenen Fenstern
//
// shank (03.11.): "dann mach auch das man tabs als eigene Fenster öffnen kann
// das man in behaved und in mission gleichzeitig arbeiten kann wenn man die
// auf 2 Bildschirme zieht".
//
// Ein Tab (Events, Mission, Model, Map Editor, Menu Editor) laesst sich aus
// dem Hauptfenster loesen: Er steht dann in einem eigenen ImGui-Fenster mit
// eigenem Viewport, also einem eigenen Windows-Fenster mit eigener Swapchain
// (ImGuiConfigFlags_ViewportsEnable, backend_win32.cpp). Beide Tabs zeichnen
// in jedem Bild und sind gleichzeitig bedienbar; jeder Tab steht in genau
// einem Fenster, das Hauptfenster behaelt immer einen.
//
// `g_app->leftMode` bleibt das Feld, das alle lesen (Bestandsaufnahme in
// docs/intern/NOTIZEN_rc609_t1.md): zwischen zwei Bildern und beim Zeichnen
// des Hauptfensters der Tab des Hauptfensters, beim Zeichnen eines eigenen
// Fensters dessen Tab, bei Kuerzeln der Tab des Fensters mit Fokus.
#ifndef BHED_GUI_TABFENSTER_H
#define BHED_GUI_TABFENSTER_H

#include <string>
#include <vector>

struct ImVec2;
struct ImGuiWindow;

namespace bhed::gui::tabfenster {

// Steht der Tab in einem eigenen Fenster?
[[nodiscard]] bool eigenes(int modus);
// Zeichnet ihn gerade irgendein Fenster (das Hauptfenster oder sein eigenes)?
[[nodiscard]] bool sichtbar(int modus);
// Wie viele Tabs stehen in eigenen Fenstern?
[[nodiscard]] int anzahl();
// Die Tabs in eigenen Fenstern, in der Reihenfolge, in der sie geloest wurden.
[[nodiscard]] std::vector<int> modi();

// Kann der Tab geloest werden? Das Hauptfenster behaelt mindestens einen Tab.
[[nodiscard]] bool loesbar(int modus);
// In ein eigenes Fenster loesen. Ohne Lage etwas versetzt ueber dem
// Hauptfenster; mit Lage (Bildschirmpunkte) dort, wo der Tab abgelegt wurde.
// Ist es der Tab des Hauptfensters, zeigt dieses danach den ersten freien.
bool loesen(int modus);
bool loesenBei(int modus, const ImVec2& lage);
// Fenster schliessen; der Tab kommt ins Hauptfenster zurueck und wird dort gezeigt.
void zurueck(int modus);
void alleZurueck();
// Zum Fenster des Tabs gehen (Klick in der Modusleiste, ein Wechsel aus dem
// Programm). Steht er im Hauptfenster, schaltet dieses um.
void zeigen(int modus);

// Der Tab des Fensters mit dem Fokus (Kuerzel, Undo, Helfer). Ohne eigene
// Fenster immer der des Hauptfensters.
[[nodiscard]] int fokusModus();
// Hat das Fenster, das gerade zeichnet, den Fokus? (NRC- und Menu-Editor-Tasten
// werden beim Zeichnen gelesen.) Ohne eigene Fenster immer wahr.
[[nodiscard]] bool fensterHatFokus();
// Der Tab des Fensters unter der Maus (Strg+Rad).
[[nodiscard]] int modusUnterMaus();

// Einstellungen: die laufende Liste (Settings::fenster["tabfenster"]) und die
// einer benannten Anordnung (fenster_dock.cpp).
[[nodiscard]] std::string kodiert();
void uebernehmen(const std::string& zeile);

// Fuer Selbsttest und Handbuch: das ImGui-Fenster des Tabs (nullptr ohne).
[[nodiscard]] ImGuiWindow* fenster(int modus);

// --- Haken fuer app.cpp ------------------------------------------------------
// Vor dem Hauptfenster: Einstellungen beim ersten Bild, Wuensche ausfuehren
// (Loesen, Zurueck, Zeigen - nie mitten im Zeichnen), den Fokus bestimmen.
void bildAnfang();
// Nach allen Fenstern: die Lagen in die Einstellungen.
void bildEnde();
// Die eigenen Fenster zeichnen: vorbereiten (Lage, Fensterklasse, leftMode und
// die anderen Felder je Fenster getauscht), begin (ImGui::Begin), ende (immer;
// ImGui::End, Zurueckziehen, zurueck getauscht).
[[nodiscard]] int fensterZahl();
void vorbereiten(int i);
[[nodiscard]] bool begin(int i);
void ende(int i);
// Zeichnet gerade ein eigenes Fenster (nicht das Hauptfenster)?
[[nodiscard]] bool zeichnetEigenes();
// Modusleiste: direkt nach dem Knopf eines Tabs (Rechtsklick, Herausziehen),
// und ihr Rechteck nach dem Zeichnen (Ziel beim Zurueckziehen).
void leisteKnopf(int modus);
void leisteGemessen(const ImVec2& a, const ImVec2& b);
// fn mit dem Tab des Fensters mit Fokus in leftMode (Kuerzel, Helfer).
void imFokus(void (*fn)());

}  // namespace bhed::gui::tabfenster
#endif
