// spieltest.h - Karten-Editor, Stufe 9 Punkt 1: "Im Spiel testen"
// (eigene Funktion, nicht in NetRadiant-Custom).
//
// Portiert aus OpenJK (code/qcommon/files.cpp FS_Startup: Reihenfolge fs_cdpath, fs_basepath,
// fs_homepath; common.cpp Com_ParseCommandLine) und NetRadiant-Custom 20260114
// (radiant/watchbsp.cpp constructEngineArgs) - die Regeln, nicht der Code.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
//
// Ein Knopf startet das gewaehlte Spiel mit der aktuellen Karte (+devmap). Die
// gebaute .bsp kommt als zzz_jkstudio_test.pk3 in einen eigenen Ordner ausserhalb
// des Spielordners, den das Spiel ueber einen Startparameter als Suchpfad kennt:
//
//   fs_cdpath   (alle Engines dieser Familie: Jedi Academy SP/MP, Jedi Outcast
//                SP/MP, OpenJK, Movie Duels - FS_Startup: FS_AddGameDirectory(
//                fs_cdpath, ...) vor fs_basepath). Der Ordner hat dort den
//                niedrigsten Rang - gut, solange das Spiel die Karte nicht schon hat.
//   fs_homepath (jamp, jk2mp, OpenJK, Movie Duels; nicht jasp/jk2sp): hoechster
//                Rang (nach fs_basepath hinzugefuegt). Fuer einen Namen, den das
//                Spiel schon kennt.
//   Mod-Ordner  sonst (oder wenn der eigene Ordner nicht als Parameter taugt):
//                <Engine>/jkstudio_test (bzw. der Ordner der aktiven Mod) - nur
//                nach Bestaetigung im Dialog.
//
// Welche Variable eine Engine kennt, steht in ihrer .exe (die Namen der Cvars
// als Text mit Null am Ende) - gelesen, nie gestartet.
#ifndef BHED_KARTE_SPIELTEST_H
#define BHED_KARTE_SPIELTEST_H

#include <string>
#include <string_view>
#include <vector>

namespace bhed::karte {

inline constexpr const char* kSpielTestPk3 = "zzz_jkstudio_test.pk3";   // paksort: zuletzt = gewinnt im Ordner
inline constexpr const char* kSpielTestMod = "jkstudio_test";

// Bekannte Programme im Engine-Ordner (fuer die Auswahl im Menue).
struct TestEngine {
    const char* exe;    // Dateiname
    const char* name;   // fuer das Menue
    bool mp;            // Mehrspieler
    bool jk2;           // Jedi Outcast
};
[[nodiscard]] const std::vector<TestEngine>& testEngines();

struct EngineFaehigkeit {
    bool gelesen = false;    // die .exe war lesbar
    bool cdpath = false;     // "fs_cdpath"
    bool homepath = false;   // "fs_homepath"
};
// Sucht die Cvar-Namen als Text mit Null am Ende in den Bytes der .exe.
[[nodiscard]] EngineFaehigkeit engineFaehigkeit(std::string_view exeBytes);

enum class SpielTestWeg { Keiner, CdPfad, HomePfad, ModOrdner };

struct SpielTestEingabe {
    std::string enginePfad;     // mit '/' am Ende
    std::string exe;            // Dateiname im Engine-Pfad
    std::string fsGame;         // gamename (leer oder = basisSpiel: keine Mod)
    std::string basisSpiel;     // "base"
    std::string testOrdner;     // der eigene Ordner (ausserhalb des Spielordners)
    std::string kartenName;     // Stamm der .bsp
    EngineFaehigkeit faehigkeit;
    bool namensKonflikt = false;    // maps/<Karte>.bsp hat das Spiel schon
    bool modOrdnerErlaubt = false;  // der Anwender hat bestaetigt
};

struct SpielTestPlan {
    SpielTestWeg weg = SpielTestWeg::Keiner;
    bool bestaetigungNoetig = false;   // Mod-Ordner: erst fragen
    std::string pk3;                   // wohin das Archiv kommt (voller Pfad, '/')
    std::string befehl;                // die ganze Befehlszeile
    std::string grund;                 // Keiner: warum (englisch, fuer die Konsole)
};

[[nodiscard]] SpielTestPlan spielTestPlanen(const SpielTestEingabe& e);

// Taugt der Text als Wert von "+set <cvar> "..."" auf der Befehlszeile?
// Com_ParseCommandLine trennt an '+', Cmd_TokenizeString an '"' und ';' beendet
// den Befehl - der Pfad darf keins davon enthalten (und nicht leer sein).
[[nodiscard]] bool alsStartwertTauglich(const std::string& pfad);

}  // namespace bhed::karte

#endif
