// projekt.h - Menue-Editor M5b Punkt 3: Projektordner und Projektdatei *.jksmenu.
// Eigener Code.
//
// Ein Projekt ist ein Ordner wie die fertige pk3 (ui/, gfx/, video/, shaders/, strings/ ...)
// mit einer Projektdatei <name>.jksmenu. Die Datei merkt sich, was nicht in den Menues selbst
// steht: Zielspiel und Dialekt, Mod-Profil, die Menues, offene Reiter, Vorschau (Aufloesung,
// Balken, Sprache, Cvars) und Export (pk3, nur Aenderungen). Die Layouts stehen als Kommentar in
// den .menu-Dateien (layout.h) und werden nicht doppelt gefuehrt. Alle anderen Dateien unter dem
// Ordner sind das Paket (Bilder, Videos, Shader, Texte, Schriften).
//
// Textformat, eine Angabe je Zeile ("schluessel wert"; Werte mit Leerzeichen in ""):
//   jksmenu 1
//   game ja|jk2|md
//   dialect sp|mp
//   profile "Jedi Academy"
//   menu ui/main.menu          (mehrfach)
//   asset ui/main.menu gfx/menus/knopf.tga   (Paketdatei und das Menue, zu dem sie gehoert)
//   open ui/main.menu          (mehrfach, offene Reiter)
//   active 0
//   preview.resolution 0
//   preview.letterbox 0|1
//   preview.language English
//   cvar ui_tab 1              (mehrfach)
//   export.pk3 zz_menu.pk3
//   export.changesOnly 0|1
#ifndef BHED_MENUE_PROJEKT_H
#define BHED_MENUE_PROJEKT_H

#include "bhed/menue/menue.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace bhed::menue {

inline constexpr const char* kProjektEndung = ".jksmenu";
inline constexpr const char* kSicherungsOrdner = ".jksautosave";   // im Projektordner

struct Projekt {
    Spiel spiel = Spiel::JA;
    Dialekt dialekt = Dialekt::SP;
    std::string profil;                       // Mod-Profil (Name), "" = das des Spiels
    std::vector<std::string> menues;          // Spielpfade, relativ zum Projektordner
    std::map<std::string, std::string> dateien;   // Paketdatei -> Menue, zu dem sie gehoert (Bilder, Texte ...)
    std::vector<std::string> offen;           // offene Reiter (Spielpfade)
    int aktiv = 0;                            // Index in offen
    int aufloesung = 0;                       // 0 = Fenster, sonst Voreinstellung der Leiste
    bool balken = false;                      // 4:3 mit Balken statt gestreckt
    std::string sprache = "English";
    std::map<std::string, std::string> cvars; // Vorschau-Cvars
    std::string exportPk3 = "zz_menu.pk3";    // relativ zum Projektordner oder absolut
    bool nurAenderungen = false;
};

[[nodiscard]] std::string projektText(const Projekt& p);
// Warum eine Projektdatei nicht gelesen wurde (die Oberflaeche sagt es in normaler Sprache)
struct ProjektFehler {
    enum class Art { Keiner, KeinProjekt, ZuNeu, PfadAusserhalb } art = Art::Keiner;
    int zeile = 0;
    std::string wert;   // ZuNeu: Fassung, PfadAusserhalb: der Pfad
};
[[nodiscard]] bool liesProjekt(const std::string& text, Projekt& out, ProjektFehler& fehler);

// Die Unterordner eines neuen Projekts (wie in der pk3)
[[nodiscard]] const std::vector<std::string>& projektUnterordner();

// Gehoert die Datei (Pfad relativ zum Projektordner, mit /) ins Paket? Nicht die Projektdatei,
// nicht die Sicherung, keine versteckten Dateien.
[[nodiscard]] bool paketPfad(const std::string& rel);

// Sicherung (Autosave): je Dokument Spielpfad, Datei auf der Platte und der Dateiname in der
// Sicherung; dazu die Zeit (Sekunden seit 1970).
struct SicherungsEintrag {
    std::string name;     // Spielpfad (ui/main.menu)
    std::string platte;   // Datei auf der Platte ("" = aus dem Spiel bzw. neu)
    std::string datei;    // Dateiname in der Sicherung (1.menu ...)
};
[[nodiscard]] std::string sicherungText(const std::vector<SicherungsEintrag>& e, std::int64_t zeit);
[[nodiscard]] bool liesSicherung(const std::string& text, std::vector<SicherungsEintrag>& out, std::int64_t& zeit);

// Windows-Dateizuordnung fuer *.jksmenu unter HKCU\Software\Classes (nur fuer den Anwender, ohne
// Adminrechte): Schluessel, Wertname ("" = Standardwert) und Wert. Geschrieben wird nur nach
// Rueckfrage im Programm.
struct RegEintrag {
    std::string schluessel, name, wert;
};
[[nodiscard]] std::vector<RegEintrag> dateizuordnung(const std::string& exePfad, const std::string& beschreibung);

}  // namespace bhed::menue

#endif
