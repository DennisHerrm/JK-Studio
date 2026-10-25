// kleineplugins.cpp - Karten-Editor, Stufe 4 Punkt 3: ShaderPlug, SunPlug und
// UFO:AI ohne Oberflaeche (siehe kleineplugins.h).
//
// Portiert aus NetRadiant-Custom 20260114: contrib/shaderplug/shaderplug.cpp,
// contrib/sunplug/sunplug.cpp, contrib/ufoaiplug/ufoai.cpp, ufoai_level.cpp,
// ufoai_filters.cpp. Die Konsolenzeilen nur, wo NRC sie auch ohne _DEBUG
// schreibt.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/kleineplugins.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texturtags.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <sstream>

namespace bhed::karte::plugins {

const char* const kShaderplugAbout = "Shaderplug (1.0)\n\nby Shaderman (shaderman@gmx.net)";
const char* const kSunplugAbout = "SunPlug v1.0 for NetRadiant 1.5\nby Topsun";

// --- ShaderPlug ----------------------------------------------------------------------

ShaderplugListe shaderplugSammeln(const std::vector<std::string>& shaderNamen, const std::vector<std::string>& texturDateien) {
    ShaderplugListe l;
    for (const std::string& s : shaderNamen) {   // LoadShaderList: string_equal_prefix( filename, "textures/" )
        if (s.rfind("textures/", 0) == 0) { l.shader.insert(s); }
    }
    for (const std::string& d : texturDateien) {   // LoadTextureFile
        if (d.size() < 4) { continue; }
        std::string b = "textures/" + d.substr(0, d.size() - 4);
        if (b.size() > 255) { b.resize(255); }   // char buffer[256]
        if (!l.shader.contains(b)) { l.texturen.insert(b); }
    }
    return l;
}

void shaderplugTags(const ShaderplugListe& l, TexturTags& tags) {
    (void)tags.neu();   // TagBuilder.CreateXmlDocument()
    for (auto it = l.texturen.rbegin(); it != l.texturen.rend(); ++it) { (void)tags.shaderDazu(*it, TagTexturArt::Stock, TagKnotenArt::Textur); }
    for (auto it = l.shader.rbegin(); it != l.shader.rend(); ++it) { (void)tags.shaderDazu(*it, TagTexturArt::Stock, TagKnotenArt::Shader); }
}

// --- SunPlug ---------------------------------------------------------------------------

Kartenkoordinaten optimaleKoordinaten(const Kasten& grenzen) {
    const float m = std::max({grenzen.extents[0], grenzen.extents[1], 175.0F});   // the square must be at least 350x350 units
    Kartenkoordinaten c;   // BasicVector2<int>( Vector2 ): Abschneiden
    c.minX = static_cast<int>(grenzen.origin[0] - m);
    c.minY = static_cast<int>(grenzen.origin[1] - m);
    c.maxX = static_cast<int>(grenzen.origin[0] + m);
    c.maxY = static_cast<int>(grenzen.origin[1] + m);
    return c;
}

std::array<int, 4> kartenkoordinatenLesen(const Entitaet& welt) {
    std::array<int, 4> v{0, 0, 0, 0};
    const auto lesen = [](const std::string& text, int& a, int& b) {   // StringTokeniser, atoi( getToken() )
        std::istringstream s(text);
        std::string t1;
        std::string t2;
        s >> t1 >> t2;
        a = std::atoi(t1.c_str());
        b = std::atoi(t2.c_str());
    };
    lesen(welt.wert("mapcoordsmins"), v[0], v[1]);
    lesen(welt.wert("mapcoordsmaxs"), v[2], v[3]);
    return v;
}

// --- UFO:AI ----------------------------------------------------------------------------

namespace {

std::shared_ptr<Entitaet> ersteKlasse(const Karte& k, const char* name) {   // Scene_FindEntityByClass
    for (const auto& e : k.entitaeten()) {
        if (e->klassenname() == name) { return e; }
    }
    return nullptr;
}

// get_team_count
void teams(const Karte& k, const char* klasse, int* anzahl, int* team, std::vector<std::string>& zeilen) {
    for (const auto& e : k.entitaeten()) {
        if (e->klassenname() != klasse) { continue; }
        if (anzahl != nullptr) { ++*anzahl; }
        if (team != nullptr) {
            const std::string t = e->wert("team");
            if (!t.empty() && std::atoi(t.c_str()) > *team) { *team = std::atoi(t.c_str()); }
        }
    }
    zeilen.push_back(std::string("UFO:AI: classname: ") + klasse + ": #" + std::to_string(anzahl != nullptr ? *anzahl : 0));
}

int ohneSchluessel(const Karte& k, const char* klasse, const char* schluessel) {   // check_entity_flags
    int n = 0;
    for (const auto& e : k.entitaeten()) { n += e->klassenname() == klasse && !e->hat(schluessel) ? 1 : 0; }
    return n;
}

void anhaengen(std::string& m, const std::string& t) {   // snprintf/strncat in char message[1024]
    if (m.size() < 1023) { m += t.substr(0, 1023 - m.size()); }
}

// EntityFindByName
void entitaetenNachName(Karte& k, const char* name, int flag, bool verstecken, std::vector<std::string>& zeilen) {
    for (const auto& e : k.entitaeten()) {
        if (e->klassenname() != name) { continue; }
        const std::string sf = e->wert("spawnflags");
        zeilen.push_back(std::string("spawnflags for ") + name + ": " + sf + ".");
        if (!sf.empty()) {
            if ((std::atoi(sf.c_str()) & flag) == 0) { e->versteckt = verstecken; }   // hide_node
        } else {
            zeilen.push_back(std::string("UFO:AI: Warning: no spawnflags for ") + name + ".");
        }
    }
}

// BrushGetLevel: die Flags der letzten Flaeche (ForEachFace ohne _DEBUG)
std::size_t pinselNachFlags(Karte& k, int flag, bool inhalt, bool nichtGesetzt, bool verstecken) {
    std::size_t n = 0;
    for (const auto& e : k.entitaeten()) {
        for (const auto& p : e->kinder()) {
            if (p->art() != Primitiv::Art::Pinsel) { continue; }
            const auto& b = static_cast<const Pinsel&>(*p);
            int inhaltF = -1;
            int flaecheF = -1;
            for (std::size_t i = 0; i < b.anzahl(); ++i) {
                flaecheF = b[i].flags().flaechenFlags;
                inhaltF = b[i].flags().inhaltFlags;
            }
            const int f = inhalt ? inhaltF : flaecheF;
            if (f > 0 && ((nichtGesetzt && (f & flag) == 0) || (!nichtGesetzt && (f & flag) != 0))) {
                p->versteckt = verstecken;
                ++n;
            }
        }
    }
    return n;
}

}  // namespace

std::string ufoaiWeltWerte(Karte& k, bool ueberschreiben, std::vector<std::string>& zeilen) {
    const auto welt = ersteKlasse(k, "worldspawn");
    if (!welt) {
        zeilen.push_back("UFO:AI: Could not find worldspawn.");
        return "Could not find worldspawn";
    }
    std::string m;
    if (ueberschreiben || !welt->hat("maxlevel")) {
        // TODO bei NRC: den hoechsten Pinsel nehmen - eine Ebene hat 64 Einheiten
        welt->setzen("maxlevel", "5");
        anhaengen(m, "Set maxlevel to: 5");
    }
    if (ueberschreiben || !welt->hat("maxteams")) {
        int anzahl = 0;
        int team = 0;
        teams(k, "info_player_start", &anzahl, &team, zeilen);
        if (team != 0) {
            welt->setzen("maxteams", std::to_string(team));
            anhaengen(m, "Set maxteams to: " + std::to_string(team));
        }
        if (anzahl < 16) { anhaengen(m, "You should at least place 16 info_player_start"); }
    }
    return m;
}

std::string ufoaiPruefen(Karte& k, std::vector<std::string>& zeilen) {
    const auto welt = ersteKlasse(k, "worldspawn");
    if (!welt) {
        zeilen.push_back("UFO:AI: Could not find worldspawn.");
        return "Could not find worldspawn";
    }
    std::string m;
    int anzahl = 0;
    int team = 0;
    teams(k, "info_player_start", &anzahl, &team, zeilen);
    if (anzahl == 0) { anhaengen(m, "No multiplayer start positions (info_player_start)\n"); }
    anzahl = 0;
    teams(k, "info_human_start", &anzahl, nullptr, zeilen);
    if (anzahl == 0) { anhaengen(m, "No singleplayer start positions (info_human_start)\n"); }
    anzahl = 0;
    teams(k, "info_2x2_start", &anzahl, nullptr, zeilen);
    if (anzahl == 0) { anhaengen(m, "No singleplayer start positions for 2x2 units (info_2x2_start)\n"); }
    anzahl = 0;
    teams(k, "info_civilian_start", &anzahl, nullptr, zeilen);
    if (anzahl == 0) { anhaengen(m, "No civilian start positions (info_civilian_start)\n"); }
    if (!welt->hat("maxlevel")) {
        anhaengen(m, "Worldspawn: No maxlevel defined\n");
    } else if (std::atoi(welt->wert("maxlevel").c_str()) > 8) {
        anhaengen(m, "Worldspawn: Highest maxlevel is 8\n");
        welt->setzen("maxlevel", "8");
    }
    for (const char* kl : {"func_door", "func_breakable", "misc_sound", "misc_model", "misc_particle"}) {
        const int n = ohneSchluessel(k, kl, "spawnflags");
        if (n != 0) { anhaengen(m, "Found " + std::to_string(n) + " " + kl + " with no spawnflags\n"); }
    }
    const int ohneTeam = ohneSchluessel(k, "info_player_start", "team");
    if (ohneTeam != 0) {
        anhaengen(m, "Found " + std::to_string(ohneTeam) + " info_player_start with no team assigned\n!!Teamcount may change after you've fixed this\n");
    }
    const int ohneFarbe = ohneSchluessel(k, "light", "_color");   // der erste Aufruf ("color") wird ueberschrieben
    if (ohneFarbe != 0) { anhaengen(m, "Found " + std::to_string(ohneFarbe) + " lights with no color value\n"); }
    if (m.empty()) { m = "No errors found - you are ready to compile the map now\n"; }
    return m;
}

void ufoaiLevel(Karte& k, UfoaiZustand& z, int flag, std::vector<std::string>& zeilen) {
    const int level = flag >> 8;
    static const char* const klassen[4] = {"func_door", "func_breakable", "misc_model", "misc_particle"};
    if (z.level != 0) {   // den vorigen Filter aufheben
        (void)pinselNachFlags(k, z.level << 8, true, true, false);
        for (const char* kl : klassen) { entitaetenNachName(k, kl, z.level, false, zeilen); }
        if (z.level == level) {
            z.level = 0;   // just disable level filter
            return;
        }
    }
    z.level = level;
    zeilen.push_back("UFO:AI: level_active: " + std::to_string(z.level) + ", flag: " + std::to_string(flag) + ".");
    (void)pinselNachFlags(k, flag, true, true, true);
    for (const char* kl : klassen) { entitaetenNachName(k, kl, level, true, zeilen); }
}

void ufoaiFilter(Karte& k, UfoaiZustand& z, int art, std::vector<std::string>& zeilen) {
    bool* an[4] = {&z.stepon, &z.actorclip, &z.weaponclip, &z.nodraw};
    static const int flags[4] = {kContentsStepon, kContentsActorclip, kContentsWeaponclip, kSurfNodraw};
    *an[art] = !*an[art];
    const std::size_t n = pinselNachFlags(k, flags[art], art != 3, false, *an[art]);
    if (art == 0) {   // nur filter_stepon schreibt ohne _DEBUG
        zeilen.push_back(n == 0 ? std::string("UFO:AI: No brushes.") : "UFO:AI: Hiding " + std::to_string(n) + " stepon brushes.");
    }
}

std::string ufoaiAbout(const std::string& programmVersion) {
    return std::string("UFO:AI Plugin (http://ufoai.sf.net)\nBuild: ") + __DATE__ + "\nRadiant version: " + programmVersion +
           "\nPlugin version: 0.4\nAuthor: Martin Gerhardy (tlh2000/mattn)\n";
}

}  // namespace bhed::karte::plugins
