// update_intern.h - REL-TEMPO: was src/update_einspielen.cpp und src/update_delta.cpp teilen
//
// Der Tausch ueber den Zwischenordner (Stufe 8d, rc610 F78) war bis REL-TEMPO ein Teil von
// einspielen; das Delta (einspielenDelta) tauscht genauso - mit einer Art Eintrag mehr: eine
// Datei, die wegfaellt (sie wandert nur in den Zwischenordner).
#ifndef BHED_UPDATE_INTERN_H
#define BHED_UPDATE_INTERN_H

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "bhed/update.h"

namespace bhed::update::intern {

namespace fs = std::filesystem;

// Der Haken aus testHakenSetzen (leer = aus).
[[nodiscard]] std::function<void(TestPhase, const std::string&)> haken();
[[nodiscard]] std::string klein(std::string s);
[[nodiscard]] bool liesDatei(const fs::path& p, std::string& out);
[[nodiscard]] std::string schmal(const std::wstring& w);
[[nodiscard]] fs::path utf8Pfad(const std::string& s);

// Ein Eintrag des Tauschs: `neu` (im Zwischenordner) kommt an `ziel`, das Alte nach `alt`.
// `weg`: es gibt kein `neu` - die Datei faellt weg (das Alte nach `alt`).
struct Tausch {
    fs::path neu, ziel, alt;
    std::string rel;
    bool programm = false, weg = false, hatteAlt = false, getauscht = false;
};
// Die Pfade fuer `rel`: das Programm als neu.exe, das alte als "<exe>.alt"; sonst neu/<rel>, alt/<rel>.
[[nodiscard]] Tausch tauschFuer(const std::string& rel, bool programm, bool weg, const fs::path& zwischen,
                                const fs::path& exe);
// Schritt 3 von einspielen: tauschen (das Programm zuletzt), bei einem Fehler in umgekehrter
// Reihenfolge zurueckstellen, zuletzt den Zwischenordner wegraeumen. Setzt dateien, geloescht,
// problem und sicherungBleibt in `erg`.
void tauschen(std::vector<Tausch>& liste, const fs::path& zwischen, const fs::path& exe, Einspielergebnis& erg);

}  // namespace bhed::update::intern

#endif  // BHED_UPDATE_INTERN_H
