// optimierer.cpp - Karten-Editor, Stufe 5 Punkt 2: der Optimierer zaehlt,
// was eine Karte gegen die Grenzen der Engine verbraucht (siehe optimierer.h).
//
// Portiert aus OpenJK (code/game: g_spawn.cpp, g_utils.cpp, g_fx.cpp,
// g_misc.cpp, g_mover.cpp, g_nav.cpp, g_rail.cpp, g_trigger.cpp,
// NPC_combat.cpp, NPC_senses.cpp, NPC_stats.cpp; codeJK2/game dieselben;
// code/qcommon: cm_load.cpp; code/rd-vanilla: tr_bsp.cpp, tr_light.cpp), aus
// Movie Duels (code/game, dieselben Dateien) und aus q3map2 (NetRadiant-Custom
// 20260114: map.cpp, bsp.cpp, writebsp.cpp, bspfile_abstract.cpp,
// bspfile_rbsp.cpp, light.cpp).
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
#include "bhed/karte/optimierer.h"

#include "bhed/gegenstaende.h"
#include "bhed/kartenpruefung.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/patchkollision.h"
#include "bhed/npcklasse.h"
#include "bhed/regelwerk.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <map>
#include <set>
#include <tuple>

namespace bhed::karte {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
bool gleichOhneFall(const std::string& a, const std::string& b) { return klein(a) == klein(b); }
bool endetMit(const std::string& s, const char* e) {
    const std::size_t n = std::strlen(e);
    return s.size() >= n && klein(s.substr(s.size() - n)) == e;
}
std::string wertVon(const EntSchluessel& e, const char* k) {
    for (const auto& [a, b] : e) {
        if (gleichOhneFall(a, k)) { return b; }
    }
    return std::string();
}
bool hatSchluessel(const EntSchluessel& e, const char* k) {
    for (const auto& kv : e) {
        if (gleichOhneFall(kv.first, k)) { return true; }
    }
    return false;
}
int ganz(const std::string& s) { return std::atoi(s.c_str()); }

// COM_StripExtension (q_shared.c): ab dem letzten '.' nach dem letzten '/'
std::string ohneEndung(const std::string& s) {
    const std::size_t punkt = s.rfind('.');
    const std::size_t strich = s.find_last_of("/\\");
    if (punkt == std::string::npos || (strich != std::string::npos && punkt < strich)) { return s; }
    return s.substr(0, punkt);
}

// Beitraege nach Groesse, hoechstens kMaxBeitraege
void beitraegeOrdnen(std::vector<Beitrag>& b) {
    std::stable_sort(b.begin(), b.end(), [](const Beitrag& x, const Beitrag& y) { return x.wert > y.wert; });
    if (b.size() > kMaxBeitraege) { b.resize(kMaxBeitraege); }
}

Zaehlung neueZaehlung(const char* id, ZaehlQuelle q, long long wert) {
    Zaehlung z;
    z.id = id;
    z.quelle = q;
    z.wert = wert;
    return z;
}

// --- Was sich beim Spawn selbst freigibt -------------------------------------
//
// Die SP_*-Funktionen, die G_FreeEntity( self ) rufen, je Spiel (ohne die
// Bedingungen der Schwierigkeit: gezaehlt wird der schlimmste Fall).
struct FreiRegel {
    const char* klasse;
    int art;   // 0 immer, 1 nur ohne targetname, 2 nur mit spawnflags & 1
};
// OpenJK code/game (JKA SP): g_misc.cpp:57 (info_null, func_group ->
// g_spawn.cpp:684-685), :167 light, :384 misc_model, :425 misc_model_static,
// :670 terrain, :3154 misc_weather_zone, :3159 misc_cubemap; g_mover.cpp:2037
// path_corner; NPC_combat.cpp:2624 point_combat; NPC_senses.cpp:1123
// target_interest; g_nav.cpp:210/251/299 waypoint, waypoint_small,
// waypoint_navgoal; g_rail.cpp:294/387 rail_track, rail_lane; g_trigger.cpp:549
// trigger_bidirectional.
constexpr FreiRegel kFreiSp[] = {
    {"info_null", 0}, {"func_group", 0}, {"light", 1}, {"misc_model", 0}, {"misc_model_static", 0}, {"terrain", 0},
    {"misc_weather_zone", 0}, {"misc_cubemap", 0}, {"path_corner", 1}, {"point_combat", 0}, {"target_interest", 0},
    {"waypoint", 0}, {"waypoint_small", 0}, {"waypoint_navgoal", 0}, {"rail_track", 0}, {"rail_lane", 0},
    {"trigger_bidirectional", 0},
};
// Movie Duels code/game: g_misc.cpp:55 info_null nur mit spawnflags 1, :170 light,
// :385 misc_model, :448 misc_model_static, :699 terrain, :3254 misc_weather_zone,
// :3259 misc_cubemap; g_mover.cpp:2134 path_corner; NPC_combat.cpp:3378;
// NPC_senses.cpp:1200; g_nav.cpp:197/238/283; g_rail.cpp:279/371; g_trigger.cpp:619.
constexpr FreiRegel kFreiMd[] = {
    {"info_null", 2}, {"func_group", 2}, {"light", 1}, {"misc_model", 0}, {"misc_model_static", 0}, {"terrain", 0},
    {"misc_weather_zone", 0}, {"misc_cubemap", 0}, {"path_corner", 1}, {"point_combat", 0}, {"target_interest", 0},
    {"waypoint", 0}, {"waypoint_small", 0}, {"waypoint_navgoal", 0}, {"rail_track", 0}, {"rail_lane", 0},
    {"trigger_bidirectional", 0},
};
// codeJK2/game: g_misc.cpp:150 light, :363 misc_model, :2790 misc_cubemap
// (info_null gibt sich erst im naechsten think frei, g_misc.cpp:55);
// g_mover.cpp:2010 path_corner; NPC_combat.cpp:2398; NPC_senses.cpp:957;
// g_nav.cpp:1291/1335/1374/1410/1446/1482/1518; g_trigger.cpp:522.
constexpr FreiRegel kFreiJk2[] = {
    {"light", 1}, {"misc_model", 0}, {"misc_cubemap", 0}, {"path_corner", 1}, {"point_combat", 0}, {"target_interest", 0},
    {"waypoint", 0}, {"waypoint_small", 0}, {"waypoint_navgoal", 0}, {"waypoint_navgoal_8", 0}, {"waypoint_navgoal_4", 0},
    {"waypoint_navgoal_2", 0}, {"waypoint_navgoal_1", 0}, {"trigger_bidirectional", 0},
};

// Weltfx, Modelle, Klaenge, Effekte - die Configstrings einer Karte.
struct Configstrings {
    std::map<std::string, std::pair<std::string, ObjektRef>> modelle;    // klein -> (Name, erste Stelle)
    std::map<std::string, std::pair<std::string, ObjektRef>> klaenge;
    std::map<std::string, std::pair<std::string, ObjektRef>> effekte;
    std::map<std::string, std::pair<std::string, ObjektRef>> weltfx;
    std::map<std::string, std::pair<std::string, ObjektRef>> unterBsp;
    std::map<std::string, int> anzahl;   // "m:" / "s:" / "e:" / "w:" + klein -> Verwendungen
    long long textZeichen = 0;           // music / message
    void add(std::map<std::string, std::pair<std::string, ObjektRef>>& m, const char* art, const std::string& name, const ObjektRef& r) {
        if (name.empty()) { return; }
        const std::string k = klein(name);
        m.emplace(k, std::make_pair(name, r));
        ++anzahl[std::string(art) + k];
    }
};

std::string fmtF(float f) {   // sprintf %f
    char z[64];
    std::snprintf(z, sizeof(z), "%f", static_cast<double>(f));
    return z;
}

// AngleVectors (q_math.c) - nur forward; M_PI in double wie dort
void winkelVorwaerts(const float* angles, float* forward) {
    float angle = static_cast<float>(angles[1] * (kPi * 2 / 360));   // YAW
    const float sy = static_cast<float>(std::sin(static_cast<double>(angle)));
    const float cy = static_cast<float>(std::cos(static_cast<double>(angle)));
    angle = static_cast<float>(angles[0] * (kPi * 2 / 360));   // PITCH
    const float sp = static_cast<float>(std::sin(static_cast<double>(angle)));
    const float cp = static_cast<float>(std::cos(static_cast<double>(angle)));
    forward[0] = cp * cy;
    forward[1] = cp * sy;
    forward[2] = -sp;
}

void vektorLesen(const std::string& s, float* v) {
    v[0] = v[1] = v[2] = 0;
    (void)std::sscanf(s.c_str(), "%f %f %f", &v[0], &v[1], &v[2]);
}

// Was eine Entitaet beim Spawn an Configstrings anlegt (nur Karten-
// Schluessel und die festen Namen der Spawnfunktionen; NPC-Waffen,
// Gegenstaende und Skripte legen zur Laufzeit mehr an - Untergrenze).
void configstringsVon(const EntSchluessel& e, const ObjektRef& r, bool bleibt, const float* mins, const float* maxs, Configstrings& cs) {
    const std::string klasse = wertVon(e, "classname");
    const int flags = ganz(wertVon(e, "spawnflags"));
    // Welteffekte (g_fx.cpp:278-414, 537-610, 681-740; g_misc.cpp:3141-3155)
    if (klasse == "fx_snow") {   // SP_CreateSnow
        if (flags & 1) { cs.add(cs.weltfx, "w:", "lightsnow", r); }
        else if (flags & 2) { cs.add(cs.weltfx, "w:", "snow", r); }
        else if (flags & 4) { cs.add(cs.weltfx, "w:", "heavysnow", r); }
        else {
            cs.add(cs.weltfx, "w:", "snow", r);
            cs.add(cs.weltfx, "w:", "fog", r);
        }
        if (flags & 8) { cs.add(cs.weltfx, "w:", "fog", r); }
    } else if (klasse == "fx_wind") {   // SP_CreateWind
        if (flags & 1) { cs.add(cs.weltfx, "w:", "wind", r); }
        if (flags & 2) {
            float ang[3];
            vektorLesen(wertVon(e, "angles"), ang);
            if (!hatSchluessel(e, "angles")) {
                ang[0] = ang[2] = 0;
                ang[1] = static_cast<float>(std::atof(wertVon(e, "angle").c_str()));
            }
            float dir[3];
            winkelVorwaerts(ang, dir);
            const std::string sp = wertVon(e, "speed");
            const float speed = sp.empty() ? 500.0F : static_cast<float>(std::atof(sp.c_str()));
            cs.add(cs.weltfx, "w:", "constantwind ( " + fmtF(dir[0] * speed) + " " + fmtF(dir[1] * speed) + " " + fmtF(dir[2] * speed) + " )", r);
        }
        if (flags & 4) { cs.add(cs.weltfx, "w:", "gustingwind", r); }
        if (flags & 8) { cs.add(cs.weltfx, "w:", "swirlingwind", r); }
        if (flags & 32) { cs.add(cs.weltfx, "w:", "fog", r); }
        if (flags & 64) { cs.add(cs.weltfx, "w:", "light_fog", r); }
    } else if (klasse == "fx_wind_zone" && mins != nullptr) {   // SP_CreateWindZone
        float ang[3];
        vektorLesen(wertVon(e, "angles"), ang);
        float dir[3];
        winkelVorwaerts(ang, dir);
        const std::string sp = wertVon(e, "speed");
        const float speed = sp.empty() ? 500.0F : static_cast<float>(std::atof(sp.c_str()));
        cs.add(cs.weltfx, "w:", "windzone ( " + fmtF(mins[0]) + " " + fmtF(mins[1]) + " " + fmtF(mins[2]) + " ) ( " + fmtF(maxs[0]) + " " +
                                    fmtF(maxs[1]) + " " + fmtF(maxs[2]) + " ) ( " + fmtF(dir[0] * speed) + " " + fmtF(dir[1] * speed) + " " +
                                    fmtF(dir[2] * speed) + " )", r);
    } else if (klasse == "fx_rain") {   // SP_CreateRain
        int f = flags;
        if (f & 1) { cs.add(cs.weltfx, "w:", "lightrain", r); }
        else if (f & 2) { cs.add(cs.weltfx, "w:", "rain", r); }
        else if (f & 4) {
            cs.add(cs.weltfx, "w:", "heavyrain", r);
            cs.add(cs.weltfx, "w:", "heavyrainfog", r);
            f |= 64;
        } else if (f & 8) {
            cs.add(cs.effekte, "e:", "world/acid_fizz", r);
            cs.add(cs.weltfx, "w:", "acidrain", r);
        } else {
            cs.add(cs.weltfx, "w:", "lightrain", r);
            cs.add(cs.weltfx, "w:", "wind", r);
        }
        if (f & 16) { cs.add(cs.weltfx, "w:", "outsideShake", r); }
        if (f & 32) { cs.add(cs.weltfx, "w:", "fog", r); }
        if (f & 64) {
            for (const char* s : {"sound/ambience/thunder1", "sound/ambience/thunder2", "sound/ambience/thunder3", "sound/ambience/thunder4",
                                  "sound/ambience/thunder_close1", "sound/ambience/thunder_close2"}) {
                cs.add(cs.klaenge, "s:", s, r);
            }
            cs.add(cs.effekte, "e:", "env/huge_lightning", r);
        }
    } else if (klasse == "fx_puff") {   // SP_CreatePuffSystem (r_weatherScale 1)
        const std::string c = wertVon(e, "count");
        const int count = c.empty() ? 1000 : ganz(c);
        int system = ganz(wertVon(e, "whichsystem"));
        if (system < 0 || system > 1) { system = 0; }
        cs.add(cs.weltfx, "w:", "puff" + std::to_string(system) + " init " + std::to_string(count), r);
        for (std::size_t i = 0; i < e.size() && i < 20; ++i) {   // G_SpawnField( i ) fuer i < 20
            const std::string k = klein(e[i].first);
            if (k == "origin" || k == "classname" || k == "count" || k == "targetname" || k == "sabersparks" || k == "whichsystem") { continue; }
            std::string t = "puff" + std::to_string(system) + " " + e[i].first + " " + e[i].second;
            if (t.size() > 127) { t.resize(127); }   // Com_sprintf( temp, 128, ... )
            cs.add(cs.weltfx, "w:", t, r);
        }
    } else if (klasse == "misc_weather_zone" && mins != nullptr) {   // SP_misc_weather_zone (vor G_FreeEntity)
        cs.add(cs.weltfx, "w:", "zone ( " + fmtF(mins[0]) + " " + fmtF(mins[1]) + " " + fmtF(mins[2]) + " ) ( " + fmtF(maxs[0]) + " " +
                                    fmtF(maxs[1]) + " " + fmtF(maxs[2]) + " )", r);
    } else if (klasse == "misc_bsp") {   // SP_misc_bsp: G_BSPIndex( "maps/<bspmodel>.bsp" )
        const std::string b = wertVon(e, "bspmodel");
        if (!b.empty()) { cs.add(cs.unterBsp, "b:", "maps/" + b + ".bsp", r); }
    }
    if (!bleibt) { return; }
    // Modelle, Klaenge, Effekte aus den Schluesseln
    for (const auto& [k0, v] : e) {
        const std::string k = klein(k0);
        if (v.empty()) { continue; }
        if ((k == "model" || k == "model2") && v[0] != '*') {
            if (klasse != "misc_model" && klasse != "misc_model_static") { cs.add(cs.modelle, "m:", v, r); }   // G_ModelIndex
            continue;
        }
        if (k == "noise" || k == "sound" || k == "startsound" || k == "endsound" || k == "loopsound" || endetMit(v, ".wav") || endetMit(v, ".mp3")) {
            cs.add(cs.klaenge, "s:", ohneEndung(v), r);   // G_SoundIndex: ohne Endung
            continue;
        }
        if (k == "fxfile" || k == "fxfile2" || endetMit(v, ".efx")) {
            cs.add(cs.effekte, "e:", ohneEndung(v), r);   // G_EffectIndex: ohne Endung
            continue;
        }
    }
}

}  // namespace

// --- Momentaufnahme -------------------------------------------------------------

std::string AbbildEntitaet::wert(const std::string& s) const { return wertVon(schluessel, s.c_str()); }
bool AbbildEntitaet::hat(const std::string& s) const { return hatSchluessel(schluessel, s.c_str()); }

KartenAbbild kartenAbbild(const Karte& k) {
    KartenAbbild a;
    a.stand = k.undo.stand();
    a.ents.reserve(k.entitaeten().size());
    for (const auto& e : k.entitaeten()) {
        AbbildEntitaet ae;
        ae.schluessel = e->schluessel();
        ae.klasse = e->wert("classname");
        if (ae.klasse.empty()) { ae.klasse = e->klassenname(); }
        ae.behaelter = e->istBehaelter();
        ae.prims.reserve(e->kinder().size());
        for (const auto& p : e->kinder()) {
            AbbildPrimitiv ap;
            if (p->art() == Primitiv::Art::Pinsel) {
                const auto& pin = static_cast<const Pinsel&>(*p);
                ap.detail = pin.istDetail();
                ap.entartet = pin.entartet(k.kontext);
                const Kasten& g = pin.grenzen(k.kontext);
                ap.mins = g.origin - g.extents;
                ap.maxs = g.origin + g.extents;
                ap.flaechen.reserve(pin.anzahl());
                for (std::size_t i = 0; i < pin.anzahl(); ++i) {
                    const Flaeche& f = pin[i];
                    AbbildFlaeche af;
                    af.shader = f.shader();
                    af.ebene = f.ebene();
                    af.inhalt = f.flags().inhaltFlags;
                    af.flaechenFlags = f.flags().flaechenFlags;
                    const Wicklung& w = f.wicklung();
                    af.punkte.reserve(w.numpoints);
                    for (std::size_t j = 0; j < w.numpoints; ++j) { af.punkte.push_back(w[j].vertex); }
                    ap.flaechen.push_back(std::move(af));
                }
            } else {
                const auto& pa = static_cast<const Patch&>(*p);
                ap.patch = true;
                ap.breite = static_cast<int>(pa.breite());
                ap.hoehe = static_cast<int>(pa.hoehe());
                ap.shader = pa.shader();
                ap.kontrollpunkte.reserve(pa.punkte().size());
                for (const PatchPunkt& pp : pa.punkte()) { ap.kontrollpunkte.push_back({pp.vertex[0], pp.vertex[1], pp.vertex[2]}); }
                const Kasten& g = pa.grenzen();
                ap.mins = g.origin - g.extents;
                ap.maxs = g.origin + g.extents;
            }
            ae.prims.push_back(std::move(ap));
        }
        a.ents.push_back(std::move(ae));
    }
    return a;
}

// --- Spawn ------------------------------------------------------------------------

bool bleibtNachSpawn(const EntSchluessel& e, GrenzSpiel spiel, const Gegenstandsliste* gegenstaende) {
    const std::string klasse = wertVon(e, "classname");
    if (klasse.empty() || klasse == "worldspawn") { return false; }   // worldspawn ist ENTITYNUM_WORLD
    // G_SpawnInt( "notsingle", "0", &i ); if ( i ) G_FreeEntity
    if (ganz(wertVon(e, "notsingle")) != 0) { return false; }
    // G_CallSpawn: ohne Spawnfunktion frei
    const bool md = spiel == GrenzSpiel::MovieDuels;
    if (!kartenpruefung::imSpiel(klasse, md, gegenstaende)) { return false; }
    const FreiRegel* anfang = kFreiSp;
    const FreiRegel* ende = kFreiSp + std::size(kFreiSp);
    if (md) {
        anfang = kFreiMd;
        ende = kFreiMd + std::size(kFreiMd);
    } else if (spiel == GrenzSpiel::Jk2) {
        anfang = kFreiJk2;
        ende = kFreiJk2 + std::size(kFreiJk2);
    }
    for (const FreiRegel* r = anfang; r != ende; ++r) {
        if (klasse != r->klasse) { continue; }
        if (r->art == 0) { return false; }
        if (r->art == 1 && wertVon(e, "targetname").empty()) { return false; }
        if (r->art == 2 && (ganz(wertVon(e, "spawnflags")) & 1) != 0) { return false; }
    }
    return true;
}

std::pair<long long, long long> spawnVariablen(const EntSchluessel& e) {
    long long zeichen = 0;
    for (const auto& [k, v] : e) { zeichen += static_cast<long long>(k.size()) + 1 + static_cast<long long>(v.size()) + 1; }
    return {static_cast<long long>(e.size()), zeichen};
}

// --- Zaehlen: gemeinsamer Teil fuer Entitaeten (Karte und Entity-Lump) -----------

namespace {

struct EntEingabe {
    EntSchluessel schluessel;
    ObjektRef ref;
    bool hatGrenzen = false;
    float mins[3]{0, 0, 0};
    float maxs[3]{0, 0, 0};
};

void zaehleEntitaeten(const std::vector<EntEingabe>& ents, const ZaehlOptionen& o, ZaehlQuelle q, std::vector<Zaehlung>& out) {
    const GrenzSpiel spiel = o.spiel;
    const bool md = spiel == GrenzSpiel::MovieDuels;
    // laufzeit.entitaeten
    Zaehlung ze = neueZaehlung("laufzeit.entitaeten", q, 0);
    std::map<std::string, long long> jeKlasse;
    std::map<std::string, ObjektRef> ersteDerKlasse;
    long long npcs = 0;
    Configstrings cs;
    Zaehlung sv = neueZaehlung("spawn.variablen", q, 0);
    Zaehlung sz = neueZaehlung("spawn.zeichen", q, 0);
    std::vector<Beitrag> svB;
    std::vector<Beitrag> szB;
    std::map<std::string, std::pair<std::string, ObjektRef>> roffs;
    std::map<std::string, ObjektRef> npcModelle;
    std::set<std::string> npcModelleSicher;   // Spawner ohne targetname: spawnen beim Kartenstart
    bool npcBedingt = false;                  // ein Spawner wartet auf use (targetname)
    bool npcZufall = false;
    bool npcUnbekannt = false;
    for (const EntEingabe& ee : ents) {
        const EntSchluessel& e = ee.schluessel;
        const std::string klasse = wertVon(e, "classname");
        const bool bleibt = bleibtNachSpawn(e, spiel, o.gegenstaende);
        if (bleibt) {
            ++ze.wert;
            ++jeKlasse[klasse];
            ersteDerKlasse.emplace(klasse, ee.ref);
            if (klasse.rfind("NPC_", 0) == 0) { ++npcs; }
        }
        configstringsVon(e, ee.ref, bleibt, ee.hatGrenzen ? ee.mins : nullptr, ee.hatGrenzen ? ee.maxs : nullptr, cs);
        if (klasse == "worldspawn") {
            cs.textZeichen += static_cast<long long>(wertVon(e, "music").size()) + 1;     // CS_MUSIC
            cs.textZeichen += static_cast<long long>(wertVon(e, "message").size()) + 1;   // CS_MESSAGE
        }
        // Schluessel je Entitaet (G_ParseSpawnVars) - auch worldspawn und
        // alles, was sich spaeter freigibt: geparst wird jede Entitaet
        const auto [paare, zeichen] = spawnVariablen(e);
        svB.push_back({ee.ref, paare});
        szB.push_back({ee.ref, zeichen});
        sv.wert = std::max(sv.wert, paare);
        sz.wert = std::max(sz.wert, zeichen);
        // ROFFs: der Schluessel "roff" (G_LoadRoff beim Spawn)
        const std::string roff = wertVon(e, "roff");
        if (!roff.empty() && bleibt) { roffs.emplace(klein(roff), std::make_pair(roff, ee.ref)); }
        // NPC-Modelle (NPC_ParseParms -> G_ParseAnimFileSet, modelsAlreadyDone)
        if (bleibt && klasse.rfind("NPC_", 0) == 0) {
            const int regelwerk = md ? kRegelwerkMovieDuels : (spiel == GrenzSpiel::Jk2 ? kRegelwerkJediOutcast : kRegelwerkOpenJK);
            const NpcKlassenTyp t = npcKlassenTypSpiel(klasse, ganz(wertVon(e, "spawnflags")), wertVon(e, "NPC_type"), hatSchluessel(e, "message"), regelwerk);
            if (!t.spawnt) { continue; }
            std::vector<std::string> typen = t.zufall.empty() ? std::vector<std::string>{t.typ} : t.zufall;
            if (!t.zufall.empty()) { npcZufall = true; }
            // SP_NPC_spawner: ohne targetname spawnt er beim Kartenstart
            // (NPC_spawn.cpp, START_TIME_REMOVE_ENTS + 50), sonst erst auf use
            const bool sofort = wertVon(e, "targetname").empty() && t.zufall.empty();
            if (!sofort) { npcBedingt = true; }
            for (const std::string& typ : typen) {
                if (typ.empty()) { continue; }
                std::string modell = o.npcModell ? o.npcModell(typ) : std::string();
                if (modell.empty()) {
                    npcUnbekannt = true;
                    modell = "?" + klein(typ);
                }
                ObjektRef r = ee.ref;
                r.text = modell + " (" + typ + ")";
                npcModelle.emplace(klein(modell), r);
                if (sofort) { npcModelleSicher.insert(klein(modell)); }
            }
        }
    }
    for (const auto& [k, n] : jeKlasse) {
        ObjektRef r = ersteDerKlasse[k];
        r.text = k;
        ze.beitraege.push_back({r, n});
    }
    beitraegeOrdnen(ze.beitraege);
    ze.hinweis = "entities kept after G_SpawnGEntityFromSpawnVars (worst skill); NPC spawners: " + std::to_string(npcs) +
                 " (each spawned NPC adds one more at runtime)";
    ze.schaetzung = spiel == GrenzSpiel::Jk2 || spiel == GrenzSpiel::JkaMp;
    out.push_back(ze);

    beitraegeOrdnen(svB);
    beitraegeOrdnen(szB);
    sv.beitraege = svB;
    sz.beitraege = szB;
    sv.hinweis = "largest number of key/value pairs of one entity";
    sz.hinweis = "largest sum of (length + 1) of keys and values of one entity";
    out.push_back(sv);
    out.push_back(sz);

    auto csZaehlung = [&](const char* id, const std::map<std::string, std::pair<std::string, ObjektRef>>& m, const char* art, const char* text) {
        Zaehlung z = neueZaehlung(id, q, static_cast<long long>(m.size()));
        z.schaetzung = true;
        z.hinweis = text;
        for (const auto& [k, v] : m) {
            ObjektRef r = v.second;
            r.text = v.first;
            const auto it = cs.anzahl.find(std::string(art) + k);
            z.beitraege.push_back({r, it != cs.anzahl.end() ? it->second : 1});
        }
        beitraegeOrdnen(z.beitraege);
        out.push_back(z);
    };
    csZaehlung("laufzeit.modelle", cs.modelle, "m:", "lower bound: model/model2 keys of entities kept after spawn (items, weapons and scripts add more)");
    csZaehlung("laufzeit.klaenge", cs.klaenge, "s:", "lower bound: sound keys of the map and fixed sounds of the spawn functions");
    csZaehlung("laufzeit.effekte", cs.effekte, "e:", "lower bound: fxFile/.efx keys and fixed effects of the spawn functions");
    if (spiel != GrenzSpiel::JkaMp) {
        csZaehlung("laufzeit.welteffekte", cs.weltfx, "w:", "world effect strings of fx_rain/snow/wind/wind_zone/puff and misc_weather_zone (OpenJK g_fx.cpp rules)");
        out.back().schaetzung = spiel != GrenzSpiel::JkaSp;
        if (spiel != GrenzSpiel::Jk2) { csZaehlung("laufzeit.subbsp", cs.unterBsp, "b:", "misc_bsp (G_BSPIndex)"); }
        out.back().schaetzung = false;
    }
    {
        long long zeichen = cs.textZeichen;
        for (const auto* m : {&cs.modelle, &cs.klaenge, &cs.effekte, &cs.weltfx, &cs.unterBsp}) {
            for (const auto& [k, v] : *m) { zeichen += static_cast<long long>(v.first.size()) + 1; }
        }
        Zaehlung z = neueZaehlung("laufzeit.gamestate", q, zeichen);
        z.schaetzung = true;
        z.hinweis = "lower bound: configstrings known from the map (models, sounds, effects, world effects, music, message)";
        out.push_back(z);
    }
    if (spiel != GrenzSpiel::JkaMp) {
        Zaehlung z = neueZaehlung("laufzeit.roffs", q, static_cast<long long>(roffs.size()));
        z.schaetzung = true;
        z.hinweis = "lower bound: \"roff\" keys (scripts can load more)";
        for (const auto& [k, v] : roffs) {
            ObjektRef r = v.second;
            r.text = v.first;
            z.beitraege.push_back({r, 1});
        }
        beitraegeOrdnen(z.beitraege);
        out.push_back(z);
    }
    if (spiel == GrenzSpiel::JkaSp || spiel == GrenzSpiel::MovieDuels) {
        Zaehlung z = neueZaehlung("laufzeit.npcmodelle", q, static_cast<long long>(npcModelle.size()) + 1);   // + Spieler
        z.schaetzung = npcUnbekannt || npcZufall || npcBedingt;
        z.obergrenze = npcZufall || npcBedingt;
        z.sicher = static_cast<long long>(npcModelleSicher.size()) + 1;
        z.hinweis = std::string("distinct playerModel of the NPC spawners plus the player (") + std::to_string(z.sicher) +
                    " certain: spawners without targetname spawn at map start)" +
                    (npcZufall ? "; random NPC types counted with every choice" : "") +
                    (npcUnbekannt ? "; NPC files not loaded - NPC_type used as model" : "");
        for (const auto& [k, r] : npcModelle) { z.beitraege.push_back({r, 1}); }
        beitraegeOrdnen(z.beitraege);
        out.push_back(z);
    }
}

}  // namespace

// --- Zaehlen aus der Karte -----------------------------------------------------------

std::vector<Zaehlung> zaehleKarte(const KartenAbbild& a, const ZaehlOptionen& o) {
    std::vector<Zaehlung> out;
    const ZaehlQuelle q = ZaehlQuelle::Karte;
    // Was q3map2 aus der Karte macht: func_group und group_info fallen weg
    // (map.cpp:1646-1656), Brush-Entities bekommen "model" "*N" (bsp.cpp
    // SetModelNumbers), light ohne targetname wird entfernt (writebsp.cpp
    // SetLightStyles, ohne _keepLights), misc_model/_decal/_skybox kommen nicht
    // in den Entity-Lump (bspfile_abstract.cpp UnparseEntities).
    const auto* welt = a.ents.empty() ? nullptr : &a.ents.front();
    const bool lichterBehalten = o.lichterBehalten || (welt != nullptr && ganz(welt->wert("_keepLights")) != 0);
    std::vector<EntEingabe> lump;
    long long modelle = 1;
    long long pinsel = 0;
    long long seiten = 0;
    long long enttext = 1;   // Endnull
    std::set<std::string> shader;
    std::map<std::string, std::pair<int, long long>> shaderZahl;   // klein -> (erste Entitaet, Flaechen)
    Zaehlung zModelle = neueZaehlung("kollision.submodelle", q, 0);
    std::vector<std::tuple<std::string, int, int>> lichtStile;   // (targetname, style, Entitaet)
    std::map<std::pair<std::string, int>, int> lichtStilErste;
    double groesste = 0;
    ObjektRef groessteRef;
    for (std::size_t i = 0; i < a.ents.size(); ++i) {
        const AbbildEntitaet& e = a.ents[i];
        const std::string klasse = e.klasse;
        const bool gruppe = gleichOhneFall(klasse, "func_group") || gleichOhneFall(klasse, "group_info");
        for (std::size_t p = 0; p < e.prims.size(); ++p) {
            const AbbildPrimitiv& pr = e.prims[p];
            // entartete Pinsel (ohne gueltige Wicklung) laesst q3map2 fallen; ihre
            // Grenzen sind die der unbegrenzten Welt
            for (int k = 0; k < 3 && !pr.entartet; ++k) {
                const double m = std::max(std::fabs(static_cast<double>(pr.mins[static_cast<std::size_t>(k)])), std::fabs(static_cast<double>(pr.maxs[static_cast<std::size_t>(k)])));
                if (m > groesste) {
                    groesste = m;
                    groessteRef = ObjektRef{static_cast<int>(i), static_cast<int>(p), -1, klasse};
                }
            }
            if (pr.patch) {
                shader.insert(klein(pr.shader));
                continue;
            }
            ++pinsel;
            seiten += static_cast<long long>(pr.flaechen.size());
            for (const AbbildFlaeche& f : pr.flaechen) {
                const std::string s = klein(f.shader);
                shader.insert(s);
                auto& sz = shaderZahl[s];
                if (sz.second == 0) { sz.first = static_cast<int>(i); }
                ++sz.second;
            }
        }
        if (gruppe) { continue; }
        EntEingabe ee;
        ee.schluessel = e.schluessel;
        ee.ref = ObjektRef{static_cast<int>(i), -1, -1, klasse};
        const bool brushEnt = i > 0 && !e.prims.empty() && !e.hat("_ins") && !e.hat("_instance") && !e.hat("_clone");
        if (!e.prims.empty()) {
            ee.hatGrenzen = true;
            ee.mins[0] = ee.mins[1] = ee.mins[2] = 1e30F;
            ee.maxs[0] = ee.maxs[1] = ee.maxs[2] = -1e30F;
            for (const AbbildPrimitiv& pr : e.prims) {
                for (std::size_t k = 0; k < 3; ++k) {
                    ee.mins[k] = std::min(ee.mins[k], pr.mins[k]);
                    ee.maxs[k] = std::max(ee.maxs[k], pr.maxs[k]);
                }
            }
        }
        if (brushEnt) {
            ee.schluessel.emplace_back("model", "*" + std::to_string(modelle));   // SetModelNumbers
            ++modelle;
            ObjektRef r = ee.ref;
            r.text = klasse + " (" + std::to_string(e.prims.size()) + ")";
            zModelle.beitraege.push_back({r, static_cast<long long>(e.prims.size())});
        }
        if (klasse.rfind("light", 0) == 0) {
            const std::string t = e.wert("targetname");
            if (t.empty()) {
                if (!lichterBehalten) { continue; }   // aus dem Entity-Lump entfernt
            } else {
                const int stil = ganz(e.wert("style"));
                const auto schl = std::make_pair(t, stil);
                if (lichtStilErste.find(schl) == lichtStilErste.end()) {
                    lichtStilErste[schl] = static_cast<int>(i);
                    lichtStile.emplace_back(t, stil, static_cast<int>(i));
                }
                if (!e.hat("style")) { ee.schluessel.emplace_back("style", "32"); }   // e.setKeyValue( "style", ... )
            }
        }
        if (gleichOhneFall(klasse, "misc_model") || gleichOhneFall(klasse, "_decal") || gleichOhneFall(klasse, "_skybox")) {
            // nicht im Entity-Lump, aber misc_model kennt die Spawn-Zaehlung
            // ohnehin als frei; fuer die Configstrings unerheblich
            continue;
        }
        enttext += 4;   // "{\n" "}\n"
        for (const auto& [k, v] : ee.schluessel) { enttext += static_cast<long long>(k.size() + v.size()) + 6; }   // "k" "v"\n
        lump.push_back(std::move(ee));
    }
    zaehleEntitaeten(lump, o, q, out);
    // Brush-Modelle samt Welt
    zModelle.wert = modelle;
    zModelle.hinweis = "1 (worldspawn) + entities with brushes or patches (func_group merged into worldspawn)";
    beitraegeOrdnen(zModelle.beitraege);
    out.push_back(zModelle);
    {
        Zaehlung z = zModelle;
        z.id = "bsp.modelle";
        out.push_back(z);
    }
    {
        Zaehlung z = neueZaehlung("bsp.entitaeten", q, static_cast<long long>(lump.size()));
        z.schaetzung = true;
        z.hinweis = "entities q3map2 writes (without func_group, misc_model, lights without targetname)";
        out.push_back(z);
    }
    {
        Zaehlung z = neueZaehlung("bsp.enttext", q, enttext);
        z.schaetzung = true;
        z.hinweis = "entity text as q3map2 writes it (UnparseEntities)";
        out.push_back(z);
    }
    {
        Zaehlung z = neueZaehlung("bsp.pinsel", q, pinsel);
        z.schaetzung = true;
        z.hinweis = "brushes of the map";
        out.push_back(z);
        Zaehlung s = neueZaehlung("bsp.pinselseiten", q, seiten);
        s.schaetzung = true;
        s.hinweis = "brush sides of the map";
        out.push_back(s);
    }
    {
        Zaehlung z = neueZaehlung("bsp.shader", q, static_cast<long long>(shader.size()));
        z.schaetzung = true;
        z.hinweis = "lower bound: shaders on brushes and patches (models add theirs)";
        for (const auto& [s, v] : shaderZahl) { z.beitraege.push_back({ObjektRef{v.first, -1, -1, s}, v.second}); }
        beitraegeOrdnen(z.beitraege);
        out.push_back(z);
    }
    {   // Weltgroesse
        Zaehlung z = neueZaehlung("kollision.weltgrenze", q, static_cast<long long>(std::ceil(groesste)));
        if (groessteRef.entitaet >= 0) { z.beitraege.push_back({groessteRef, z.wert}); }
        z.hinweis = "largest absolute coordinate of a brush or patch";
        out.push_back(z);
    }
    {   // schaltbare Lichter (SetLightStyles)
        Zaehlung z = neueZaehlung("compiler.lichtschalter", q, static_cast<long long>(lichtStile.size()));
        for (const auto& [t, s, i] : lichtStile) { z.beitraege.push_back({ObjektRef{i, -1, -1, t + " (style " + std::to_string(s) + ")"}, 1}); }
        beitraegeOrdnen(z.beitraege);
        z.hinweis = "distinct (targetname, style) of light entities with a targetname";
        out.push_back(z);
    }
    // Patches: Groesse, Kontrollpunkte, Kollisionsebenen und -facetten
    Zaehlung zg = neueZaehlung("renderer.patchgroesse", q, 0);
    Zaehlung zp = neueZaehlung("kollision.patchpunkte", q, 0);
    Zaehlung ze = neueZaehlung("kollision.patchebenen", q, 0);
    Zaehlung zf = neueZaehlung("kollision.patchfacetten", q, 0);
    for (std::size_t i = 0; i < a.ents.size(); ++i) {
        for (std::size_t p = 0; p < a.ents[i].prims.size(); ++p) {
            const AbbildPrimitiv& pr = a.ents[i].prims[p];
            if (!pr.patch) { continue; }
            const ObjektRef r{static_cast<int>(i), static_cast<int>(p), -1, pr.shader + " " + std::to_string(pr.breite) + "x" + std::to_string(pr.hoehe)};
            const long long g = std::max(pr.breite, pr.hoehe);
            zg.wert = std::max(zg.wert, g);
            zg.beitraege.push_back({r, g});
            const long long n = static_cast<long long>(pr.breite) * pr.hoehe;
            zp.wert = std::max(zp.wert, n);
            zp.beitraege.push_back({r, n});
            const PatchKollision k = patchKollision(pr.breite, pr.hoehe, pr.kontrollpunkte);
            if (!k.gueltig) { continue; }
            ze.wert = std::max(ze.wert, static_cast<long long>(k.ebenen));
            ze.beitraege.push_back({r, k.ebenen});
            zf.wert = std::max(zf.wert, static_cast<long long>(k.facetten));
            zf.beitraege.push_back({r, k.facetten});
        }
    }
    for (Zaehlung* z : {&zg, &zp, &ze, &zf}) { beitraegeOrdnen(z->beitraege); }
    zg.hinweis = "largest patch (control points per direction)";
    zp.hinweis = "largest patch (width * height)";
    ze.hinweis = "most collision planes of one patch (CM_GeneratePatchCollide)";
    zf.hinweis = "most collision facet slots of one patch (CM_PatchCollideFromGrid)";
    out.push_back(zg);
    out.push_back(zp);
    out.push_back(ze);
    out.push_back(zf);
    return out;
}

// --- Zaehlen aus der BSP ----------------------------------------------------------------

namespace {

// RBSP-Lumps (q3map2 bspfile_rbsp.cpp, qfiles.h)
enum Lump {
    kEntities = 0, kShaders, kPlanes, kNodes, kLeafs, kLeafSurfaces, kLeafBrushes, kModels, kBrushes, kBrushSides,
    kDrawVerts, kDrawIndexes, kFogs, kSurfaces, kLightmaps, kLightGrid, kVisibility, kLightArray, kLumps
};
constexpr int kGroesse[kLumps] = {1, 72, 16, 36, 48, 4, 4, 40, 12, 12, 80, 4, 72, 148, 1, 30, 1, 2};
constexpr int kMstPlanar = 1;
constexpr int kMstPatch = 2;
constexpr int kMstTriangleSoup = 3;
constexpr int kLightmapBytes = 128 * 128 * 3;   // LIGHTMAP_SIZE (tr_bsp.cpp:173)

std::int32_t i32(const std::string& d, std::size_t o) {
    std::int32_t v = 0;
    if (o + 4 <= d.size()) { std::memcpy(&v, d.data() + o, 4); }
    return v;
}
float f32(const std::string& d, std::size_t o) {
    float v = 0;
    if (o + 4 <= d.size()) { std::memcpy(&v, d.data() + o, 4); }
    return v;
}

// COM_Parse-artig: Entitaeten des Entity-Lumps mit allen Paaren in der Reihenfolge
std::vector<EntSchluessel> entitaetenLesen(const std::string& t) {
    std::vector<EntSchluessel> out;
    std::size_t i = 0;
    auto token = [&](std::string& tok) -> bool {
        while (true) {
            while (i < t.size() && static_cast<unsigned char>(t[i]) <= ' ') { ++i; }
            if (i + 1 < t.size() && t[i] == '/' && t[i + 1] == '/') {
                while (i < t.size() && t[i] != '\n') { ++i; }
                continue;
            }
            break;
        }
        if (i >= t.size() || t[i] == 0) { return false; }
        tok.clear();
        if (t[i] == '"') {
            ++i;
            while (i < t.size() && t[i] != '"') { tok += t[i++]; }
            if (i < t.size()) { ++i; }
            return true;
        }
        while (i < t.size() && static_cast<unsigned char>(t[i]) > ' ') { tok += t[i++]; }
        return true;
    };
    std::string tok;
    while (token(tok)) {
        if (tok != "{") { break; }
        EntSchluessel e;
        while (true) {
            std::string k;
            if (!token(k) || k == "}") { break; }
            std::string v;
            if (!token(v)) { break; }
            e.emplace_back(k, v);
        }
        out.push_back(std::move(e));
    }
    return out;
}

}  // namespace

BspAnalyse analysiereBsp(const std::string& d, const ZaehlOptionen& o, const std::string& prtDaten) {
    BspAnalyse an;
    if (d.size() < 8 + kLumps * 8 || d.compare(0, 4, "RBSP") != 0) {
        an.fehler = "not an RBSP file (Jedi Academy / Jedi Outcast)";
        return an;
    }
    std::size_t ofs[kLumps];
    std::size_t len[kLumps];
    for (int l = 0; l < kLumps; ++l) {
        const std::int32_t of = i32(d, 8 + static_cast<std::size_t>(l) * 8);
        const std::int32_t ln = i32(d, 12 + static_cast<std::size_t>(l) * 8);
        if (of < 0 || ln < 0 || static_cast<std::size_t>(of) + static_cast<std::size_t>(ln) > d.size()) {
            an.fehler = "lump " + std::to_string(l) + " outside the file";
            return an;
        }
        ofs[l] = static_cast<std::size_t>(of);
        len[l] = static_cast<std::size_t>(ln);
    }
    auto anzahl = [&](int l) { return static_cast<long long>(len[l] / static_cast<std::size_t>(kGroesse[l])); };
    const ZaehlQuelle q = ZaehlQuelle::Bsp;
    std::vector<Zaehlung>& out = an.werte;
    // Entity-Lump
    std::string enttext = d.substr(ofs[kEntities], len[kEntities]);
    const std::vector<EntSchluessel> ents = entitaetenLesen(enttext);
    {
        std::vector<EntEingabe> ein;
        ein.reserve(ents.size());
        // Brush-Modelle: die Grenzen aus dem Modell-Lump fuer die Wetterzonen
        for (std::size_t i = 0; i < ents.size(); ++i) {
            EntEingabe ee;
            ee.schluessel = ents[i];
            ee.ref = ObjektRef{static_cast<int>(i), -1, -1, wertVon(ents[i], "classname")};
            const std::string m = wertVon(ents[i], "model");
            if (!m.empty() && m[0] == '*') {
                const long long mi = std::atoll(m.c_str() + 1);
                if (mi >= 0 && mi < anzahl(kModels)) {
                    const std::size_t b = ofs[kModels] + static_cast<std::size_t>(mi) * 40;
                    for (std::size_t k = 0; k < 3; ++k) {
                        ee.mins[k] = f32(d, b + k * 4);
                        ee.maxs[k] = f32(d, b + 12 + k * 4);
                    }
                    ee.hatGrenzen = true;
                }
            }
            ein.push_back(std::move(ee));
        }
        zaehleEntitaeten(ein, o, q, out);
    }
    out.push_back(neueZaehlung("bsp.entitaeten", q, static_cast<long long>(ents.size())));
    out.push_back(neueZaehlung("bsp.enttext", q, static_cast<long long>(len[kEntities])));
    out.push_back(neueZaehlung("bsp.shader", q, anzahl(kShaders)));
    {
        Zaehlung z = neueZaehlung("kollision.submodelle", q, anzahl(kModels));
        z.hinweis = "models in the BSP (model 0 is the world)";
        // Beitraege: Brush-Entities nach Zahl ihrer Pinsel
        for (std::size_t i = 0; i < ents.size(); ++i) {
            const std::string m = wertVon(ents[i], "model");
            if (m.empty() || m[0] != '*') { continue; }
            const long long mi = std::atoll(m.c_str() + 1);
            if (mi < 0 || mi >= anzahl(kModels)) { continue; }
            const long long n = i32(d, ofs[kModels] + static_cast<std::size_t>(mi) * 40 + 36) + i32(d, ofs[kModels] + static_cast<std::size_t>(mi) * 40 + 28);
            z.beitraege.push_back({ObjektRef{static_cast<int>(i), -1, -1, wertVon(ents[i], "classname") + " " + m}, n});
        }
        beitraegeOrdnen(z.beitraege);
        out.push_back(z);
        Zaehlung b = z;
        b.id = "bsp.modelle";
        out.push_back(b);
    }
    out.push_back(neueZaehlung("bsp.pinsel", q, anzahl(kBrushes)));
    out.push_back(neueZaehlung("bsp.pinselseiten", q, anzahl(kBrushSides)));
    out.push_back(neueZaehlung("bsp.ebenen", q, anzahl(kPlanes)));
    out.push_back(neueZaehlung("bsp.knoten", q, anzahl(kNodes)));
    out.push_back(neueZaehlung("bsp.blaetter", q, anzahl(kLeafs)));
    out.push_back(neueZaehlung("bsp.blattflaechen", q, anzahl(kLeafSurfaces)));
    out.push_back(neueZaehlung("bsp.blattpinsel", q, anzahl(kLeafBrushes)));
    out.push_back(neueZaehlung("bsp.flaechen", q, anzahl(kSurfaces)));
    out.push_back(neueZaehlung("bsp.punkte", q, anzahl(kDrawVerts)));
    out.push_back(neueZaehlung("bsp.indizes", q, anzahl(kDrawIndexes)));
    out.push_back(neueZaehlung("bsp.nebel", q, anzahl(kFogs)));
    {
        Zaehlung z = neueZaehlung("renderer.nebel", q, anzahl(kFogs));
        z.hinweis = "fogs in the BSP; the renderer adds the night vision slot (R_LoadFogs)";
        out.push_back(z);
    }
    out.push_back(neueZaehlung("bsp.licht", q, static_cast<long long>(len[kLightmaps])));
    out.push_back(neueZaehlung("bsp.sicht", q, static_cast<long long>(len[kVisibility])));
    out.push_back(neueZaehlung("compiler.sicht", q, static_cast<long long>(len[kVisibility])));
    an.sichtBytes = static_cast<long long>(len[kVisibility]);
    // Flaechen: Punkte und Indizes je Flaeche, Lightmaps, Patches
    const long long nFl = anzahl(kSurfaces);
    Zaehlung zv = neueZaehlung("renderer.flaechenpunkte", q, 0);
    Zaehlung zi = neueZaehlung("renderer.flaechenindizes", q, 0);
    Zaehlung zg = neueZaehlung("renderer.patchgroesse", q, 0);
    Zaehlung zp = neueZaehlung("kollision.patchpunkte", q, 0);
    Zaehlung ze = neueZaehlung("kollision.patchebenen", q, 0);
    Zaehlung zf = neueZaehlung("kollision.patchfacetten", q, 0);
    long long lmMax = -1;
    const long long nShader = anzahl(kShaders);
    auto shaderName = [&](std::int32_t s) {
        if (s < 0 || s >= nShader) { return std::string("?"); }
        const char* p = d.data() + ofs[kShaders] + static_cast<std::size_t>(s) * 72;
        return std::string(p, strnlen(p, 64));
    };
    const long long nVerts = anzahl(kDrawVerts);
    for (long long s = 0; s < nFl; ++s) {
        const std::size_t b = ofs[kSurfaces] + static_cast<std::size_t>(s) * 148;
        const std::int32_t shaderNum = i32(d, b);
        const std::int32_t typ = i32(d, b + 8);
        const std::int32_t firstVert = i32(d, b + 12);
        const std::int32_t numVerts = i32(d, b + 16);
        const std::int32_t numIdx = i32(d, b + 24);
        for (int k = 0; k < 4; ++k) {
            const std::int32_t lm = i32(d, b + 36 + static_cast<std::size_t>(k) * 4);
            lmMax = std::max<long long>(lmMax, lm);
        }
        const ObjektRef r{-1, -1, static_cast<int>(s), shaderName(shaderNum)};
        if (typ == kMstPlanar || typ == kMstTriangleSoup) {
            zv.beitraege.push_back({r, numVerts});
            zi.beitraege.push_back({r, numIdx});
            zv.wert = std::max<long long>(zv.wert, numVerts);
            zi.wert = std::max<long long>(zi.wert, numIdx);
        } else if (typ == kMstPatch) {
            const std::int32_t w = i32(d, b + 140);
            const std::int32_t h = i32(d, b + 144);
            ObjektRef rp = r;
            rp.text += " " + std::to_string(w) + "x" + std::to_string(h);
            zg.wert = std::max<long long>(zg.wert, std::max(w, h));
            zg.beitraege.push_back({rp, std::max(w, h)});
            const long long n = static_cast<long long>(w) * h;
            zp.wert = std::max(zp.wert, n);
            zp.beitraege.push_back({rp, n});
            if (firstVert < 0 || static_cast<long long>(firstVert) + n > nVerts || w <= 0 || h <= 0) { continue; }
            std::vector<std::array<float, 3>> pts(static_cast<std::size_t>(n));
            for (long long j = 0; j < n; ++j) {
                const std::size_t vb = ofs[kDrawVerts] + static_cast<std::size_t>(firstVert + j) * 80;
                pts[static_cast<std::size_t>(j)] = {f32(d, vb), f32(d, vb + 4), f32(d, vb + 8)};
            }
            const PatchKollision k = patchKollision(w, h, pts);
            if (!k.gueltig) { continue; }
            ze.wert = std::max<long long>(ze.wert, k.ebenen);
            ze.beitraege.push_back({rp, k.ebenen});
            zf.wert = std::max<long long>(zf.wert, k.facetten);
            zf.beitraege.push_back({rp, k.facetten});
        }
    }
    for (Zaehlung* z : {&zv, &zi, &zg, &zp, &ze, &zf}) { beitraegeOrdnen(z->beitraege); }
    zv.hinweis = "largest planar/triangle surface (patches are tessellated by the renderer)";
    zi.hinweis = "largest planar/triangle surface";
    out.push_back(zv);
    out.push_back(zi);
    out.push_back(zg);
    out.push_back(zp);
    out.push_back(ze);
    out.push_back(zf);
    {
        const long long intern = static_cast<long long>(len[kLightmaps]) / kLightmapBytes;
        Zaehlung z = neueZaehlung("renderer.lightmaps", q, std::max(intern, lmMax + 1));
        z.hinweis = "lightmaps in the BSP (128 x 128); highest lightmap number used: " + std::to_string(lmMax);
        out.push_back(z);
    }
    // Lichtgitter (tr_bsp.cpp R_LoadLightGrid / R_LoadLightGridArray)
    {
        const long long punkte = anzahl(kLightGrid);
        const long long feld = anzahl(kLightArray);
        out.push_back(neueZaehlung("bsp.lichtgitter", q, punkte));
        out.push_back(neueZaehlung("bsp.lichtgitterfeld", q, feld));
        for (long long j = 0; j < feld; ++j) {
            std::uint16_t v = 0;
            std::memcpy(&v, d.data() + ofs[kLightArray] + static_cast<std::size_t>(j) * 2, 2);
            if (static_cast<long long>(v) >= punkte) { ++an.lichtgitterUeberlauf; }
        }
        // gridsize aus worldspawn (R_LoadEntities: "gridsize", Vorgabe 64 64 128)
        if (!ents.empty()) {
            const std::string gs = wertVon(ents.front(), "gridsize");
            if (!gs.empty()) {
                float g[3] = {64, 64, 128};
                (void)std::sscanf(gs.c_str(), "%f %f %f", &g[0], &g[1], &g[2]);
                an.gitterGroesse = {g[0], g[1], g[2]};
            }
        }
        if (anzahl(kModels) > 0) {
            long long prod = 1;
            for (std::size_t k = 0; k < 3; ++k) {
                const float mn = f32(d, ofs[kModels] + k * 4);
                const float mx = f32(d, ofs[kModels] + 12 + k * 4);
                const float gsz = an.gitterGroesse[k];
                const float origin = gsz * std::ceil(mn / gsz);
                const float maxs = gsz * std::floor(mx / gsz);
                const int bounds = static_cast<int>((maxs - origin) / gsz + 1);
                prod *= bounds;
            }
            an.lichtgitterErwartet = prod;
            an.lichtgitterFehlpassung = feld > 0 && prod != feld;
        }
    }
    // Sicht (Cluster) und Flaechen je Ansicht (geschaetzt aus dem PVS)
    {
        const std::int32_t numClusters = len[kVisibility] >= 8 ? i32(d, ofs[kVisibility]) : 0;
        const std::int32_t clusterBytes = len[kVisibility] >= 8 ? i32(d, ofs[kVisibility] + 4) : 0;
        an.cluster = numClusters;
        const long long nLeafs = anzahl(kLeafs);
        if (numClusters > 0 && clusterBytes > 0 &&
            8 + static_cast<std::size_t>(numClusters) * static_cast<std::size_t>(clusterBytes) <= len[kVisibility]) {
            std::vector<std::vector<std::int32_t>> flJeCluster(static_cast<std::size_t>(numClusters));
            const long long nLs = anzahl(kLeafSurfaces);
            for (long long l = 0; l < nLeafs; ++l) {
                const std::size_t b = ofs[kLeafs] + static_cast<std::size_t>(l) * 48;
                const std::int32_t c = i32(d, b);
                const std::int32_t first = i32(d, b + 32);
                const std::int32_t num = i32(d, b + 36);
                if (c < 0 || c >= numClusters) { continue; }
                for (std::int32_t k = 0; k < num; ++k) {
                    if (first + k < 0 || first + k >= nLs) { break; }
                    flJeCluster[static_cast<std::size_t>(c)].push_back(i32(d, ofs[kLeafSurfaces] + static_cast<std::size_t>(first + k) * 4));
                }
            }
            for (auto& v : flJeCluster) {
                std::sort(v.begin(), v.end());
                v.erase(std::unique(v.begin(), v.end()), v.end());
            }
            // ohne Doppelte grob: Summe der Flaechen der sichtbaren Cluster
            std::vector<long long> grob(static_cast<std::size_t>(numClusters), 0);
            for (std::int32_t c = 0; c < numClusters; ++c) {
                const std::size_t zeile = ofs[kVisibility] + 8 + static_cast<std::size_t>(c) * static_cast<std::size_t>(clusterBytes);
                long long s = 0;
                for (std::int32_t v = 0; v < numClusters; ++v) {
                    if ((static_cast<unsigned char>(d[zeile + static_cast<std::size_t>(v >> 3)]) & (1U << (v & 7))) != 0) {
                        s += static_cast<long long>(flJeCluster[static_cast<std::size_t>(v)].size());
                    }
                }
                grob[static_cast<std::size_t>(c)] = s;
            }
            // genau (ohne Doppelte) fuer die 64 Cluster mit der groessten Summe
            std::vector<std::int32_t> reihe(static_cast<std::size_t>(numClusters));
            for (std::int32_t c = 0; c < numClusters; ++c) { reihe[static_cast<std::size_t>(c)] = c; }
            std::partial_sort(reihe.begin(), reihe.begin() + std::min<std::ptrdiff_t>(64, numClusters), reihe.end(),
                              [&](std::int32_t x, std::int32_t y) { return grob[static_cast<std::size_t>(x)] > grob[static_cast<std::size_t>(y)]; });
            std::vector<std::int32_t> marke(static_cast<std::size_t>(nFl), -1);
            // Dreiecke je Flaeche: Indizes / 3; ein Patch grob mit vier
            // Teilungen je Feld (r_subdivisions 4, tr_curve.cpp) - Schaetzung
            std::vector<long long> dreiecke(static_cast<std::size_t>(nFl), 0);
            for (long long s2 = 0; s2 < nFl; ++s2) {
                const std::size_t b = ofs[kSurfaces] + static_cast<std::size_t>(s2) * 148;
                const std::int32_t typ = i32(d, b + 8);
                if (typ == kMstPatch) {
                    const std::int32_t w = i32(d, b + 140);
                    const std::int32_t h = i32(d, b + 144);
                    dreiecke[static_cast<std::size_t>(s2)] = w > 1 && h > 1 ? 2LL * ((w - 1) / 2 * 4) * ((h - 1) / 2 * 4) : 0;
                } else {
                    dreiecke[static_cast<std::size_t>(s2)] = i32(d, b + 24) / 3;
                }
            }
            Zaehlung z = neueZaehlung("renderer.drawsurfs", q, 0);
            z.schaetzung = true;
            z.obergrenze = true;
            for (std::size_t r = 0; r < reihe.size() && r < 64; ++r) {
                const std::int32_t c = reihe[r];
                const std::size_t zeile = ofs[kVisibility] + 8 + static_cast<std::size_t>(c) * static_cast<std::size_t>(clusterBytes);
                long long n = 0;
                long long tri = 0;
                for (std::int32_t v = 0; v < numClusters; ++v) {
                    if ((static_cast<unsigned char>(d[zeile + static_cast<std::size_t>(v >> 3)]) & (1U << (v & 7))) == 0) { continue; }
                    for (const std::int32_t s : flJeCluster[static_cast<std::size_t>(v)]) {
                        if (s < 0 || s >= nFl) { continue; }
                        if (marke[static_cast<std::size_t>(s)] != c) {
                            marke[static_cast<std::size_t>(s)] = c;
                            ++n;
                            tri += dreiecke[static_cast<std::size_t>(s)];
                        }
                    }
                }
                z.wert = std::max(z.wert, n);
                if (tri > an.dreieckeJeAnsicht) {
                    an.dreieckeJeAnsicht = tri;
                    an.dreieckeCluster = c;
                }
                z.beitraege.push_back({ObjektRef{-1, -1, -1, "cluster " + std::to_string(c)}, n});
            }
            beitraegeOrdnen(z.beitraege);
            z.hinweis = "estimate: world surfaces in the PVS of the worst cluster (no view frustum, no models or effects)";
            out.push_back(z);
        }
    }
    // Portale aus der .prt (PRT1: Zeile 1 "PRT1", 2 Cluster, 3 Portale)
    if (!prtDaten.empty()) {
        long long cl = 0;
        long long po = 0;
        char kopf[16] = {0};
        if (std::sscanf(prtDaten.c_str(), "%15s %lld %lld", kopf, &cl, &po) == 3) {
            an.portale = po;
            Zaehlung z = neueZaehlung("bsp.portale", q, po);
            z.hinweis = "portals in the .prt (" + std::to_string(cl) + " clusters)";
            out.push_back(z);
        }
    }
    an.ok = true;
    return an;
}

// CM_PointLeafnum_r (cm_test.cpp): von Knoten 0 absteigen, bis ein Blatt kommt
int bspCluster(const std::string& d, const Vektor3f& p) {
    if (d.size() < 8 + kLumps * 8 || d.compare(0, 4, "RBSP") != 0) { return -2; }
    auto lump = [&](int l, std::size_t& of, std::size_t& ln) {
        of = static_cast<std::size_t>(std::max(0, i32(d, 8 + static_cast<std::size_t>(l) * 8)));
        ln = static_cast<std::size_t>(std::max(0, i32(d, 12 + static_cast<std::size_t>(l) * 8)));
        return of + ln <= d.size();
    };
    std::size_t oN = 0;
    std::size_t lN = 0;
    std::size_t oP = 0;
    std::size_t lP = 0;
    std::size_t oL = 0;
    std::size_t lL = 0;
    if (!lump(kNodes, oN, lN) || !lump(kPlanes, oP, lP) || !lump(kLeafs, oL, lL) || lN < 36) { return -2; }
    const long long nNodes = static_cast<long long>(lN / 36);
    const long long nPlanes = static_cast<long long>(lP / 16);
    const long long nLeafs = static_cast<long long>(lL / 48);
    std::int32_t num = 0;
    int schritte = 0;
    while (num >= 0) {
        if (num >= nNodes || ++schritte > 100000) { return -2; }
        const std::size_t b = oN + static_cast<std::size_t>(num) * 36;
        const std::int32_t plane = i32(d, b);
        if (plane < 0 || plane >= nPlanes) { return -2; }
        const std::size_t pb = oP + static_cast<std::size_t>(plane) * 16;
        const float dist = f32(d, pb) * p[0] + f32(d, pb + 4) * p[1] + f32(d, pb + 8) * p[2] - f32(d, pb + 12);
        num = dist >= 0 ? i32(d, b + 4) : i32(d, b + 8);
    }
    const std::int32_t leaf = -1 - num;
    if (leaf < 0 || leaf >= nLeafs) { return -2; }
    return i32(d, oL + static_cast<std::size_t>(leaf) * 48);
}

// --- Bewerten -------------------------------------------------------------------------

std::vector<Bewertung> bewerten(GrenzSpiel s, const std::vector<Zaehlung>& z, double knappAb) {
    std::vector<Bewertung> out;
    for (const Zaehlung& x : z) {
        const Grenze* g = grenzeSuchen(s, x.id);
        if (g == nullptr) { continue; }
        Bewertung b;
        b.grenze = g;
        b.zaehlung = x;
        b.anteil = g->erlaubt > 0 ? static_cast<double>(x.wert) / static_cast<double>(g->erlaubt) : 0.0;
        const bool ueber = x.wert > g->erlaubt;
        if (ueber) {
            const bool sicherUeber = !x.obergrenze || x.sicher > g->erlaubt;
            b.ampel = (g->hart && sicherUeber) ? Ampel::Ueber : Ampel::Hinweis;
        } else if (b.anteil >= knappAb) {
            b.ampel = g->hart ? Ampel::Knapp : Ampel::Hinweis;
        } else {
            b.ampel = Ampel::Ok;
        }
        out.push_back(std::move(b));
    }
    return out;
}

const char* ampelName(Ampel a) {
    switch (a) {
    case Ampel::Ok: return "ok";
    case Ampel::Knapp: return "near";
    case Ampel::Ueber: return "over";
    case Ampel::Hinweis: return "note";
    }
    return "";
}

}  // namespace bhed::karte
