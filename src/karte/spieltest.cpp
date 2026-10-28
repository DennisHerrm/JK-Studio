// spieltest.cpp - Karten-Editor, Stufe 9 Punkt 1: "Im Spiel testen" (siehe spieltest.h)
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
#include "bhed/karte/spieltest.h"

#include "bhed/karte/spielbeschreibung.h"

namespace bhed::karte {

const std::vector<TestEngine>& testEngines() {
    static const std::vector<TestEngine> liste = {
        {"jasp.exe", "Jedi Academy SP", false, false},
        {"jamp.exe", "Jedi Academy MP", true, false},
        {"openjk_sp.x86.exe", "OpenJK SP (x86)", false, false},
        {"openjk_sp.x86_64.exe", "OpenJK SP (x64)", false, false},
        {"openjk.x86.exe", "OpenJK MP (x86)", true, false},
        {"openjk.x86_64.exe", "OpenJK MP (x64)", true, false},
        {"MovieDuels-SP.x86_64.exe", "Movie Duels SP", false, false},
        {"MovieDuels-MP.x86_64.exe", "Movie Duels MP", true, false},
        {"jk2sp.exe", "Jedi Outcast SP", false, true},
        {"jk2mp.exe", "Jedi Outcast MP", true, true},
        {"openjo_sp.x86.exe", "OpenJO SP (x86)", false, true},
        {"openjo_sp.x86_64.exe", "OpenJO SP (x64)", false, true},
    };
    return liste;
}

namespace {

bool cvarImBinaer(std::string_view bytes, std::string_view name) {
    // der Name als eigener Text: davor kein Buchstabe/Ziffer/'_', danach die Null
    for (std::size_t p = bytes.find(name); p != std::string_view::npos; p = bytes.find(name, p + 1)) {
        const std::size_t ende = p + name.size();
        if (ende >= bytes.size() || bytes[ende] != '\0') { continue; }
        if (p > 0) {
            const char c = bytes[p - 1];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') { continue; }
        }
        return true;
    }
    return false;
}

std::string mitSchraeg(std::string s) {
    for (char& c : s) {
        if (c == '\\') { c = '/'; }
    }
    while (s.size() > 1 && s.back() == '/') { s.pop_back(); }
    return s;
}

}  // namespace

EngineFaehigkeit engineFaehigkeit(std::string_view exeBytes) {
    EngineFaehigkeit f;
    f.gelesen = !exeBytes.empty();
    f.cdpath = cvarImBinaer(exeBytes, "fs_cdpath");
    f.homepath = cvarImBinaer(exeBytes, "fs_homepath");
    return f;
}

bool alsStartwertTauglich(const std::string& pfad) {
    if (pfad.empty()) { return false; }
    for (const char c : pfad) {
        if (c == '+' || c == '"' || c == ';' || c == '\n' || c == '\r') { return false; }
    }
    return true;
}

SpielTestPlan spielTestPlanen(const SpielTestEingabe& e) {
    SpielTestPlan p;
    if (e.kartenName.empty()) {
        p.grund = "the map has no name - save it first";
        return p;
    }
    for (const char c : e.kartenName) {   // devmap <Name>: ein Wort ohne Anfuehrungszeichen
        if (c == ' ' || c == '"' || c == ';' || c == '+') {
            p.grund = "the map name \"" + e.kartenName + "\" contains a space, '\"', ';' or '+' - the game cannot load it with devmap";
            return p;
        }
    }
    if (e.exe.empty()) {
        p.grund = "no game program set (Preferences > Build > Engine)";
        return p;
    }
    const bool mod = !e.fsGame.empty() && e.fsGame != e.basisSpiel;
    const std::string spielOrdner = mod ? e.fsGame : e.basisSpiel;   // dort sucht das Spiel unter jedem Pfad
    const bool ordnerTaugt = alsStartwertTauglich(e.testOrdner);
    const std::string test = mitSchraeg(e.testOrdner);
    std::string pfadArg;
    std::string fsGame = mod ? e.fsGame : e.basisSpiel;
    if (ordnerTaugt && e.faehigkeit.cdpath && !e.namensKonflikt) {
        p.weg = SpielTestWeg::CdPfad;
        p.pk3 = test + "/" + spielOrdner + "/" + kSpielTestPk3;
        pfadArg = "+set fs_cdpath \"" + test + "\" ";
    } else if (ordnerTaugt && e.faehigkeit.homepath) {
        p.weg = SpielTestWeg::HomePfad;
        p.pk3 = test + "/" + spielOrdner + "/" + kSpielTestPk3;
        pfadArg = "+set fs_homepath \"" + test + "\" ";
    } else {
        p.weg = SpielTestWeg::ModOrdner;
        if (!e.modOrdnerErlaubt) {
            p.bestaetigungNoetig = true;
            return p;
        }
        // ohne Mod: ein eigener Mod-Ordner; mit Mod (Movie Duels): in deren Ordner,
        // weil sich zwei Mods nicht stapeln lassen
        const std::string ordner = mod ? e.fsGame : std::string(kSpielTestMod);
        p.pk3 = mitSchraeg(e.enginePfad) + "/" + ordner + "/" + kSpielTestPk3;
        fsGame = ordner;
    }
    std::string args = engineArgumente(fsGame, e.basisSpiel);   // constructEngineArgs
    const std::size_t q = args.find("%mapname%");
    if (q != std::string::npos) { args.replace(q, 9, e.kartenName); }
    p.befehl = "\"" + e.enginePfad + e.exe + "\" " + pfadArg + args;
    return p;
}

}  // namespace bhed::karte
