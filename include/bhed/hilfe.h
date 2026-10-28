// hilfe.h - der Helfer: "Was willst du erreichen?" (Stufe H1)
//
// shank: "eine KI im Programm, die du fragen kannst und die dir aktiv zeigt,
// wie du das erreichst". Der Kern laeuft OHNE Netz: ein Index ueber die
// Tutorials (handbuch/<sprache>/touren.json, jeder Schritt einzeln), das
// Handbuch (suche.js), alle Menuepunkte und Einstellungen (Ortskatalog, siehe
// orte()) und eine Liste haeufiger Ziele mit Wortlisten in vier Sprachen
// (src/hilfe_daten.inc). Die Antwort ist ein Eintrag mit kurzer Erklaerung
// und dem, was "Zeig es mir" tut: eine Tour starten (ab einem Schritt), ein
// Bedienelement hervorheben (Zielangabe wie in bhed/tour.h) oder eine
// Handbuchseite oeffnen.
//
// Rangfolge: BM25 ueber Titel (x3), Suchwoerter (x2) und Text (x1). Woerter
// werden klein geschrieben, Umlaute gefaltet (ae, oe, ue, ss) und grob
// gestutzt (Kameras -> kamera); CJK zaehlt als Einzel- und Doppelzeichen.
// Begriffe (src/hilfe_daten.inc, BEGRIFFE) verbinden Woerter ueber die
// Sprachen: "Kamerafahrt", "camera path", "镜头" und "カメラ" tragen alle den
// Begriff @kamera - so findet eine deutsche Frage auch einen englischen Text.
//
// Rein (ohne Oberflaeche), geprueft in tests/hilfetest.cpp: mindestens 60
// typische Fragen in vier Sprachen, Ziel Top-3 >= 85 %.
#ifndef BHED_HILFE_H
#define BHED_HILFE_H

#include "bhed/i18n.h"
#include "bhed/tour.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace bhed::hilfe {

enum class Art { Tour, Seite, Ort };

struct Eintrag {
    Art art = Art::Seite;
    // "tour:tut-karte", "tour:tut-karte@4" (ab Schritt 5), "seite:k-pk3.html#schritte",
    // "ort:menue:MenuEdit>AppPrefs"
    std::string id;
    std::string titel;     // in der Sprache des Index
    std::string text;      // kurze Erklaerung (erster Satz)
    std::string woerter;   // weitere Suchwoerter (Englisch, Wortlisten der Ziele)
    std::string bereich;   // skript, mission, vorschau, modell, karte, einstellungen, updates, programm
    std::string pfad;      // Anzeige "Bearbeiten > Einstellungen" (Ort)
    // Zeig es mir
    std::string tour;      // Tour-Kennung
    int ab = 0;            // ab diesem Schritt (0-basiert)
    std::string ziel;      // Zielangabe (bhed/tour.h) eines Orts
    std::string voraus;    // Vorbedingung "<pruefung> < <ziel>"
    std::string seite;     // Handbuchseite "k-pk3.html#anker"
};

struct Treffer {
    std::size_t eintrag = 0;
    double wert = 0.0;
};

// Ein Wort-Zerleger fuer Index und Frage. Liefert die Woerter und die
// Begriffe ("@kamera").
[[nodiscard]] std::vector<std::string> woerter(const std::string& text);

class Index {
public:
    void hinzu(Eintrag e);
    // Nach dem letzten hinzu(): Woerter zaehlen. Ein Eintrag mit einer
    // Kennung, die es schon gibt, ergaenzt nur dessen Suchwoerter.
    void fertig();
    [[nodiscard]] std::vector<Treffer> frage(const std::string& anfrage, std::size_t hoechstens = 5) const;
    [[nodiscard]] const std::vector<Eintrag>& eintraege() const { return eintraege_; }
    [[nodiscard]] const Eintrag* finde(const std::string& id) const;

private:
    struct Doku {
        std::unordered_map<std::string, double> tf;   // gewichtet
        double laenge = 0.0;
    };
    std::vector<Eintrag> eintraege_;
    std::vector<Doku> doku_;
    std::unordered_map<std::string, double> df_;
    std::vector<std::string> vokabel_;   // sortiert, fuer Vorsilben
    double mittel_ = 1.0;
};

// --- Ortskatalog: wo im Programm etwas steht ---------------------------------

struct Ort {
    const char* bereich;
    const char* ziel;      // Zielangabe, z. B. "menue:MenuEdit>AppPrefs"
    const char* kette;     // Kennungen fuer die Anzeige, ">" getrennt
    const char* hinweis;   // Kennung des Hinweistexts ("" = keiner)
    const char* voraus;    // Vorbedingung ("" = keine)
};
// Hauptmenue, Modusleiste und Einstellungen (src/hilfe_orte.inc, erzeugt von
// tools/gen_hilfe_orte.py) und die Menues des Karten-Editors (aus
// karte::befehle::menues()).
[[nodiscard]] const std::vector<Ort>& orte();

// Der ganze Index in einer Sprache. `touren` und `handbuch` sind die Dateien
// dieser Sprache (leer = nicht da).
[[nodiscard]] Index baue(const tour::Touren& touren, const std::vector<tour::Abschnitt>& handbuch, i18n::Language sprache);

// Die Kennungen der haeufigen Ziele (src/hilfe_daten.inc) - der Test prueft,
// dass jede im Index steht.
[[nodiscard]] std::vector<std::string> zielKennungen();

// Der erste fette Teil eines Schritttexts ("**Texture.** Double-click" ->
// "Texture."), sonst der Anfang.
[[nodiscard]] std::string schrittTitel(const std::string& roh);

}  // namespace bhed::hilfe

#endif
