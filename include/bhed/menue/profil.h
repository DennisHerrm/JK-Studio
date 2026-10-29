// profil.h - Menue-Editor M5b Punkt 4: Mod-Profile.
// Eigener Code.
//
// Ein Mod-Profil sagt dem Editor, was die Engine einer Mod zusaetzlich kennt: Schluesselwoerter
// in itemDef/menuDef (mit der Art ihrer Werte), Ownerdraw- und Feeder-Nummern, uiScripts (mit der
// Zahl der Woerter, die UI_RunMenuScript nach dem Namen liest), Vorgaben fuer Cvars und welche
// Menuelisten (ui/menus.txt ...) die UI laedt. Die Tabellen von Jedi Academy, Jedi Outcast und
// Movie Duels stehen im Parser; ein Profil ergaenzt sie. Was das Profil nennt, liest der Parser
// ohne Meldung und die Vorschau zeigt es als beschrifteten Platzhalter; der Text bleibt
// unveraendert (Rundlauf byte-gleich). Zeilen, die der Editor nicht versteht, bleiben im Profil.
//
// Textformat (*.jksprofile), eine Angabe je Zeile, // Kommentar:
//   jksprofile 1
//   name "Galactic Legacy"
//   game ja|jk2|md                      (dessen Tabellen gelten)
//   menulist sp|mp ui/menus.txt         (mehrfach, in dieser Reihenfolge gesucht)
//   itemkey <name> <art>                (art: int float string token rect color vec3 script none)
//   menukey <name> <art>
//   ownerdraw <nummer> <NAME>
//   feeder <nummer> <NAME>
//   uiscript <name> <woerter>
//   cvar <name> "<vorgabe>"
#ifndef BHED_MENUE_PROFIL_H
#define BHED_MENUE_PROFIL_H

#include "bhed/menue/menue.h"

#include <map>
#include <string>
#include <vector>

namespace bhed::menue {

inline constexpr const char* kProfilEndung = ".jksprofile";

struct ModProfil {
    std::string name;
    Spiel spiel = Spiel::JA;
    std::vector<std::string> menuelistenSp, menuelistenMp;
    std::map<std::string, std::string> itemSchluessel, menuSchluessel;   // Name -> Art (klein geschrieben)
    std::map<int, std::string> ownerdraws, feeder;
    std::map<std::string, int> uiSkripte;
    std::map<std::string, std::string> cvars;
    std::vector<std::string> unbekannt;   // nicht verstandene Zeilen (bleiben beim Schreiben erhalten)
};

[[nodiscard]] std::string profilText(const ModProfil& p);
// false: keine Profildatei (erste Angabe nicht "jksprofile") bzw. neuere Fassung; fehlerZeile = Zeile
[[nodiscard]] bool liesProfil(const std::string& text, ModProfil& out, int& fehlerZeile);

// Die eingebauten Profile: Jedi Academy, Jedi Outcast, Movie Duels (Menuelisten wie UI_Load bzw.
// ui_menuFilesMP der Spiele; die Schluesselwoerter stehen im Parser).
[[nodiscard]] const std::vector<ModProfil>& eingebauteProfile();
[[nodiscard]] const ModProfil& eingebautesProfil(Spiel s);

// Menuelisten zum Suchen: die des Profils zuerst, dann die der eingebauten Profile (ohne Doppelte)
[[nodiscard]] std::vector<std::string> menuelisten(const ModProfil& p, Dialekt d);

// Art eines Profil-Schluessels; "" = das Profil kennt ihn nicht (Gross/Klein egal)
[[nodiscard]] std::string profilSchluesselArt(const ModProfil& p, bool item, const std::string& name, const std::string** kanon);

// Vorlage zum Ausfuellen (fuer eine Mod ohne verfuegbaren Quellcode): alle Angaben mit Erklaerung
[[nodiscard]] std::string profilVorlage(const std::string& name, Spiel basis);

}  // namespace bhed::menue

#endif
