// umzug.h - Stufe 8c (Umbenennung in JK Studio): die Einstellungen aus dem
// alten Ordner uebernehmen.
//
// Bis rc605 hiess der Einstellungsordner %APPDATA%\behaved, jetzt
// %APPDATA%\jkstudio (ohne Leerzeichen: der Ordner steht in Befehlszeilen von
// q3map2 - der Bau-Arbeitsordner karte_bau liegt darin - und in Skripten; der
// Name gleicht dem der Programmdatei jkstudio.exe). Beim ersten Start, wenn der
// neue Ordner noch nicht existiert und der alte schon, wird der alte KOPIERT -
// das Original bleibt unveraendert, eine aeltere Fassung findet ihre Einstellungen
// weiter. Was neu entstehen darf, wird nicht kopiert: der Shader-Zwischenspeicher
// (shader\), der Modell-Index (karte_modellindex_*.txt), die ausgepackten
// eingebauten Daten (data\, gehoeren zu einer Fassung), die Arbeitsordner des
// Karten-Editors (karte_bau, karte_bsp, karte_zusammen) und Protokolle.
//
// Ohne Fenster und Windows pruefbar (tests/umzugtest.cpp).
#ifndef BHED_UMZUG_H
#define BHED_UMZUG_H

#include <filesystem>
#include <string>

namespace bhed::umzug {

inline constexpr const char* kAlterOrdner = "behaved";
inline constexpr const char* kNeuerOrdner = "jkstudio";

// Gehoert ein Eintrag (relativ zum alten Ordner, mit '/') nicht in die Kopie?
[[nodiscard]] bool auslassen(const std::string& relativ);

struct Ergebnis {
    bool uebernommen = false;   // es wurde kopiert
    int dateien = 0;
    int ausgelassen = 0;
    std::string fehler;         // leer = alles gut
};

// Wenn `neu` nicht existiert und `alt` ein Ordner ist: `alt` nach `neu`
// kopieren (ohne die Eintraege von auslassen). Das Original bleibt. Die Kopie
// entsteht erst unter "<neu>.umzug" und wird dann umbenannt: ein abgebrochener
// Umzug hinterlaesst keinen halben neuen Ordner (der naechste Start versucht es
// wieder). Existiert `neu` schon, geschieht nichts.
[[nodiscard]] Ergebnis uebernehmen(const std::filesystem::path& alt, const std::filesystem::path& neu);

}  // namespace bhed::umzug

#endif  // BHED_UMZUG_H
