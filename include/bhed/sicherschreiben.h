// sicherschreiben.h - rc609 W2a Punkt 5 (Fund F5): eine Datei sicher ersetzen
//
// Bis rc608 oeffneten spew (gui/app.cpp) und schreibe (gui/app_mission.cpp) das
// Ziel mit trunc und gaben static_cast<bool>(f) zurueck, bevor der Destruktor
// den Puffer leerte. Kleine Dateien (fast jede .ent, .IBI, mission.txt) wurden
// erst beim Schliessen geschrieben - ein Fehler dort blieb unbemerkt, und bei
// einem Fehler mitten im Schreiben war die alte Datei schon abgeschnitten.
//
// Jetzt: erst in "<pfad>.tmp" daneben, Schreiben UND Schliessen pruefen, dann
// atomar umbenennen (MoveFileExW mit MOVEFILE_REPLACE_EXISTING). Scheitert ein
// Schritt, bleibt die alte Datei unveraendert, die Temp-Datei verschwindet und
// das Ergebnis ist false.
#ifndef BHED_SICHERSCHREIBEN_H
#define BHED_SICHERSCHREIBEN_H

#include <string>

namespace bhed {

// Fehlerarten fuer die Meldung im Programm.
enum class SchreibFehler {
    Keiner,
    Schreiben,    // Temp-Datei nicht anlegbar oder Schreiben/Schliessen gescheitert
    Gesperrt,     // das Ziel ist von einem anderen Programm geoeffnet (das Spiel laeuft)
    Umbenennen,   // Ersetzen aus einem anderen Grund gescheitert
};

// Legt fehlende Ordner an. fehler: englischer Text fuer Protokoll und Meldung.
[[nodiscard]] bool sicherSchreiben(const std::string& pfad, const std::string& inhalt, std::string* fehler = nullptr,
                                   SchreibFehler* art = nullptr);

// true, wenn ein anderes Programm die Datei so geoeffnet hat, dass sie sich
// nicht ersetzen laesst (fopen der Engine teilt kein Loeschen). false auch,
// wenn es sie nicht gibt.
[[nodiscard]] bool dateiGesperrt(const std::string& pfad);

// Atomar umbenennen (MoveFileExW, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH).
[[nodiscard]] bool dateiErsetzen(const std::string& von, const std::string& nach, SchreibFehler* art = nullptr);

// Nur fuer ctest und Selbsttest: der naechste sicherSchreiben-Aufruf scheitert
// an dieser Stelle - 1 beim Schreiben (nach der Haelfte), 2 beim Schliessen,
// 3 beim Umbenennen; 0 = aus. Gilt fuer genau einen Aufruf.
void sicherSchreibenFehlerEinbauen(int stufe);

}  // namespace bhed
#endif
