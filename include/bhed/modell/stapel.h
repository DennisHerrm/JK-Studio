// modell/stapel.h - der Modifier-Stapel einer Flaeche rechnen (AP13S, MBENTWURF Teil B, rc612)
//
// Die Eintraege stehen im Dokument (Teil::stapel, bhed/modell/stapeldaten.h) in der Reihenfolge, in der sie rechnen. Jeder
// Eintrag ist eine Abbildung Netz -> Netz in der Bindepose (die Ecken tragen Knochen und Gewichte weiter, die Figur wird
// danach verformt wie immer):
//
//   Spiegeln   wof::symmetrisieren (3ds Max "Symmetry", Blender "Mirror" mit Bisect): an der Achsenebene schneiden, die
//              Quellseite auf die andere spiegeln, Punkte auf der Ebene teilen; Knochen der Seiten tauschen (l_ <-> r_)
//   Glaetten   Unterteilen nach Catmull und Clark (E. Catmull, J. Clark: "Recursively generated B-spline surfaces on
//              arbitrary topological meshes", Computer-Aided Design 10(6), 1978) - das Verfahren hinter 3ds Max
//              "TurboSmooth" und Blender "Subdivision Surface": je Vieleck ein Flaechenpunkt (Mittel seiner Punkte), je
//              Kante ein Kantenpunkt (innen: Mittel aus den zwei Enden und den zwei Flaechenpunkten; am Rand: die Mitte),
//              jeder alte Punkt nach (Q + 2R + (n-3)P) / n (Q Mittel der Flaechenpunkte, R Mittel der Kantenmitten, n Zahl
//              der Kanten am Punkt); ein Punkt an einem offenen Rand nach (A + 6P + B) / 8 mit seinen zwei Nachbarn am Rand -
//              oder er bleibt (Modifikator::raenderHalten). UV, Knochen und Gewichte der neuen Ecken sind das Mittel der
//              Ecken ihres Vielecks (linear, je Vieleck - eine UV-Naht bleibt eine Naht); hoechstens vier Knochen je Ecke,
//              Summe 1 (iMAX_G2_BONEWEIGHTS_PER_VERT, mdx_format.h). Die Normalen danach wie nach jedem Werkzeug
//              (wof::normalenNeu).
//
// Die Vorschau und das Anwenden rechnen mit DERSELBEN Funktion auf DEMSELBEN Netz: stapelNetz(...) ist das Netz, das die
// Ansicht zeigt, und stapelAnwenden(...) legt genau dieses Netz ins Dokument - die Vorschau stimmt mit dem angewendeten
// Ergebnis ueberein (Ecke fuer Ecke, Dreieck fuer Dreieck). Jeder Eintrag rechnet auf dem fertigen Netz seines Vorgaengers
// (Netz -> Arbeitsnetz -> Netz), damit "erst anwenden, dann der naechste" dasselbe ergibt wie "beide in der Vorschau".
//
// Ohne wirksamen Eintrag aendert nichts hier das Dokument oder das, was Speichern und Export schreiben: stapelFuerDatei gibt
// dann denselben Inhalt zurueck (der Aufrufer schreibt das Dokument selbst).
#ifndef BHED_MODELL_STAPEL_H
#define BHED_MODELL_STAPEL_H

#include <vector>

#include "bhed/modell/dokument.h"
#include "bhed/modell/wof/poly.h"

namespace bhed::modell {

enum class StapelZweck : std::uint8_t { Ansicht = 0, Datei = 1 };

// Was die Eintraege von aussen brauchen: je Knochen sein Spiegel (l_ <-> r_; leer: die Knochen bleiben beim Spiegeln).
struct StapelUmgebung {
    std::vector<int> knochenSpiegel;
};

// Rechnet der Eintrag fuer diesen Zweck? Ansicht: das Auge; Datei: das Auge und "in die Datei".
[[nodiscard]] bool stapelWirkt(const Modifikator& m, StapelZweck z);
[[nodiscard]] bool stapelWirkt(const Teil& t, StapelZweck z);
[[nodiscard]] bool stapelWirkt(const Inhalt& in, StapelZweck z);
// Hat irgendeine Flaeche einen Eintrag (auch einen ausgeschalteten)?
[[nodiscard]] bool stapelLeer(const Inhalt& in);
// Eine freie Kennung fuer einen neuen Eintrag (groesser als jede im Inhalt).
[[nodiscard]] std::uint32_t stapelNeueKennung(const Inhalt& in);
// Der Eintrag mit der Kennung in der Flaeche, -1 keiner.
[[nodiscard]] int stapelIndex(const Teil& t, std::uint32_t kennung);

// EIN Eintrag auf ein Netz (eine LOD-Stufe der Flaeche `teil`). true: `aus` ist das Ergebnis. false: der Eintrag kann hier
// nicht rechnen (Spiegeln: auf der Quellseite der Ebene liegt nichts; ein Tag) - `aus` bleibt unberuehrt, `grund` nennt es.
// Malt jemand gerade Gewichte (Netz::mal), rechnet der Eintrag mit dem, was das Spiel daraus machte (wie die Ansicht).
[[nodiscard]] bool modifikatorAnwenden(const Teil& teil, const Netz& netz, const Modifikator& m, const StapelUmgebung& u, Netz& aus,
                                       wof::Ablehnung* grund = nullptr);

// Ecken und Dreiecke eines Netzes (fuer die Anzeige "812 -> 4870").
struct NetzZahlen {
    int ecken = 0;
    int dreiecke = 0;
};
[[nodiscard]] NetzZahlen netzZahlen(const Netz& n);

// Das Netz der Stufe nach allen Eintraegen der Flaeche, die fuer den Zweck rechnen (in ihrer Reihenfolge). `status` je Eintrag
// der Flaeche: Keine = gerechnet oder nicht wirksam, sonst der Grund, aus dem er uebersprungen wurde. `nach` je Eintrag die
// Zahlen des Netzes hinter ihm (ein Eintrag, der nicht rechnet, gibt die seines Vorgaengers weiter).
[[nodiscard]] Netz stapelNetz(const Teil& teil, int stufe, StapelZweck z, const StapelUmgebung& u, std::vector<wof::Ablehnung>* status = nullptr,
                              std::vector<NetzZahlen>* nach = nullptr);

// Was Speichern und Export schreiben: jede Flaeche mit einem Eintrag, der in die Datei geht, in allen LOD-Stufen ausgerechnet
// (die Stapel im Ergebnis sind leer). Der Aufrufer fragt vorher stapelWirkt(in, StapelZweck::Datei) - ohne wirksamen Eintrag
// schreibt er das Dokument selbst, Byte fuer Byte wie ohne Stapel.
[[nodiscard]] Inhalt stapelFuerDatei(const Inhalt& in, const StapelUmgebung& u);

// Eintraege anwenden: die Flaeche bekommt in JEDER LOD-Stufe das Netz nach den Eintraegen `eintraege` (Indizes in Teil::stapel,
// aufsteigend; sie rechnen in dieser Reihenfolge auf dem Netz des Dokuments), die Eintraege verschwinden aus dem Stapel.
// false: nichts geaendert (ein Index ungueltig, oder ein Eintrag kann auf der feinsten Stufe nicht rechnen - `grund`).
[[nodiscard]] bool stapelAnwenden(Inhalt& in, int teil, const std::vector<int>& eintraege, const StapelUmgebung& u, wof::Ablehnung* grund = nullptr);

// Sind zwei Netze dieselben - gleiche Ecken (Ort, Normale, UV, Knochen, Gewichte bitgleich) in gleicher Folge, gleiche
// Dreiecke? Fuer die Pruefung "Vorschau = angewendet".
[[nodiscard]] bool netzGleich(const Netz& a, const Netz& b);

}  // namespace bhed::modell
#endif
