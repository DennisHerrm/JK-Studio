// imspiel.h - Menue-Editor M2 Punkt 8: "Im Spiel testen".
//
// Die Menues (geaenderte Dateien und das Paket) kommen als zzz_jkstudio_menue.pk3
// in einen eigenen Ordner ausserhalb des Spiels. Weil sie gleichnamige Dateien
// des Spiels (ui/main.menu, ui/menus.txt ...) ersetzen sollen, braucht das Archiv
// den hoechsten Rang: fs_homepath (OpenJK code/qcommon/files.cpp FS_Startup legt
// fs_homepath nach fs_basepath an; fs_cdpath hat den niedrigsten Rang und taugt
// hier nicht). Kennt das Programm fs_homepath nicht (jasp, jk2sp), bleibt nur der
// Mod-Ordner - und das nur nach Bestaetigung. Ein Befehl ausser +set ueberspringt
// den Vorspann (Com_AddStartupCommands). Im Selbsttest wird nie gestartet.
//
// Die Regeln wie karte::spielTestPlanen (spieltest.h), ohne +devmap.
#ifndef BHED_MENUE_IMSPIEL_H
#define BHED_MENUE_IMSPIEL_H

#include "bhed/karte/spieltest.h"

#include <string>

namespace bhed::menue {

inline constexpr const char* kMenueTestPk3 = "zzz_jkstudio_menue.pk3";

struct MenueTestEingabe {
    std::string enginePfad;   // Ordner des Programms, mit '/' am Ende
    std::string exe;
    std::string fsGame;       // leer oder = basisSpiel: keine Mod
    std::string basisSpiel = "base";
    std::string testOrdner;   // eigener Ordner (ausserhalb des Spiels)
    karte::EngineFaehigkeit faehigkeit;
    bool modOrdnerErlaubt = false;
};

struct MenueTestPlan {
    karte::SpielTestWeg weg = karte::SpielTestWeg::Keiner;
    bool bestaetigungNoetig = false;
    std::string pk3;      // wohin das Archiv kommt ('/')
    std::string befehl;   // ganze Befehlszeile
    std::string grund;    // Keiner: warum (englisch)
};

[[nodiscard]] MenueTestPlan menueTestPlanen(const MenueTestEingabe& e);

}  // namespace bhed::menue

#endif  // BHED_MENUE_IMSPIEL_H
