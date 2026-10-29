// diagnose.h - Menue-Editor M2: Live-Diagnose wie in einem Code-Editor.
// Eigener Code.
//
// Alles, was an einer .menu nicht stimmt, mit der genauen Stelle im Text:
//   - die Meldungen des Parsers im Wortlaut der Engine (PC_ParseWarning,
//     Com_Printf), dazu ein aehnliches Schluesselwort als Korrektur;
//   - Klammern { } ( ) und Anfuehrungszeichen: fehlende, ueberzaehlige und
//     falsch gepaarte - die oeffnende und die schliessende Stelle; wo die
//     Einrueckung eindeutig sagt, wo die Klammer fehlt, als Korrektur;
//   - Zahlen-Argumente, die keine Zahlen sind (SP liest dann 0: atoi/atof);
//   - doppelte menuDef- und itemDef-Namen;
//   - Ziele von open/close/show/hide/fadein/fadeout/setfocus/... ohne Menue
//     bzw. Item/Gruppe (Menu_ItemsMatchingGroup mit "*" am Ende);
//   - Cvars, die einem bekannten Cvar sehr aehnlich sind (Tippfehler);
//   - die Pruefungen vor dem Export (pruefung.h, gemeinsame Quelle): Grenzen,
//     nie sichtbar, fehlende Dateien (mit Pfad-Vorschlag), Vertraeglichkeit.
// Nichts davon kennt einzelne Menues des Spiels.
#ifndef BHED_MENUE_DIAGNOSE_H
#define BHED_MENUE_DIAGNOSE_H

#include "bhed/menue/menue.h"
#include "bhed/menue/pruefung.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace bhed::menue {

enum class Schwere : std::uint8_t { Fehler, Warnung, Hinweis };

enum class DiagArt : std::uint8_t {
    Engine,            // Meldung des Parsers, text = Wortlaut der Engine
    KlammerOffen,      // { bzw. ( ohne Gegenstueck; partner = wo der Block nach der Einrueckung endet
    KlammerZuViel,     // } bzw. ) ohne oeffnende
    KlammerFalsch,     // ( ... } bzw. { ... ); partner = die oeffnende
    AnfuehrungOffen,   // " ueber das Zeilenende hinaus; partner = das naechste "
    KeineZahl,         // text = Schluessel, wert = das Argument (SP liest 0)
    NameDoppelt,       // text = Name; menuDef (Warnung) bzw. itemDef im selben Menue (Hinweis)
    ZielFehlt,         // text = Ziel, wert = Befehl (open, show ...)
    CvarAehnlich,      // text = Cvar, wert = der bekannte
    Pruefung,          // befund aus pruefeFuerSpiel
};

// Eine Schnellkorrektur: [anfang, ende) durch text ersetzen (anfang == ende: einfuegen).
struct Korrektur {
    std::uint32_t anfang = 0, ende = 0;
    std::string text;
};

struct Diagnose {
    DiagArt art = DiagArt::Engine;
    Schwere schwere = Schwere::Warnung;
    std::uint32_t anfang = 0, ende = 0;   // Byte-Spanne in der Hauptdatei (ende == anfang: keine Stelle)
    int zeile = 0;                         // 1 = erste Zeile, 0 = unbekannt
    bool partner = false;
    std::uint32_t partnerAnfang = 0, partnerEnde = 0;
    int menu = -1, item = -1;
    std::string text, wert;
    Befund befund;                         // bei DiagArt::Pruefung
    std::vector<Korrektur> korrekturen;
};

struct DiagKontext {
    PruefKontext pruefung;
    // open/close: gibt es ein Menue dieses Namens in einer anderen Datei? (leer: nur die Datei)
    std::function<bool(const std::string& name)> menueBekannt;
    std::vector<std::string> bekannteCvars;   // leer: keine Tippfehler-Pruefung
    // eine vorhandene, aehnliche Datei fuer einen fehlenden Pfad ("" = keine)
    std::function<std::string(const std::string& pfad)> pfadVorschlag;
};

[[nodiscard]] std::vector<Diagnose> diagnose(const Datei& d, const DiagKontext& k);

// Levenshtein-Abstand ohne Gross/Klein.
[[nodiscard]] int abstand(const std::string& a, const std::string& b);
// Der eine Kandidat mit dem kleinsten Abstand <= maxAbstand (bei Gleichstand: "").
[[nodiscard]] std::string aehnlichstes(const std::string& wort, const std::vector<std::string>& kandidaten, int maxAbstand);

}  // namespace bhed::menue

#endif  // BHED_MENUE_DIAGNOSE_H
