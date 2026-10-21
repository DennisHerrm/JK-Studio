// karteklassen.cpp - Karten-Editor Stufe 1a, Punkt 2f: Entitaetsklassen aus
// .def (QUAKED) und .ent (XML), wie radiant/eclass_def.cpp und eclass_xml.cpp.
//
//   - QUAKED-Kopf: Name, Farbe, "?" (Gruppe) oder Kasten, Spawnflags nach
//     Bit ("x", "-", "unused*" lassen die Stelle frei), Kommentar ab der
//     zweiten Zeile, model=, die drei Standardattribute angle/model/noise;
//     fehlende Farbe -> Klasse nur mit Namen; kaputter Kasten -> Fehler;
//     die erste Klasse eines Namens gewinnt.
//   - .ent: group/point, color/box/model, Attribute mit Beschreibung,
//     flag bit, Listen, Kommentar "key : text", Entitaeten ersetzt.
//   - data/karte/ja und data/karte/jk2 (aus OpenJK erzeugt): Modus sp/mp,
//     bekannte Klassen mit ihren Werten.
//   - Movie Duels: JA-Klassen plus bhed/entdefs.h.
//   - Optional (zweites Argument): NRCs Gamepack-Ordner (gamepacks) - Vergleich
//     auf Vollstaendigkeit, die Unterschiede werden ausgegeben.
//
// Aufruf: karteklassen <data/karte> [<NRC gamepacks>]
#include "bhed/karte/eklasse.h"

#include <cstdio>
#include <filesystem>
#include <set>
#include <string>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

void vergleichen(const char* titel, const EntitaetsKlassen& wir, const EntitaetsKlassen& nrc) {
    std::set<std::string> a, b;
    for (const auto* k : wir.alle()) { a.insert(klein(k->name)); }
    for (const auto* k : nrc.alle()) { b.insert(klein(k->name)); }
    std::string fehlen, mehr;
    std::size_t nFehlen = 0, nMehr = 0, flagUnterschiede = 0, artUnterschiede = 0;
    for (const auto& n : b) {
        if (!a.count(n)) {
            fehlen += " " + n;
            ++nFehlen;
        }
    }
    for (const auto& n : a) {
        if (!b.count(n)) {
            mehr += " " + n;
            ++nMehr;
        }
    }
    for (const auto* k : wir.alle()) {
        const EntitaetsKlasse* o = nrc.finden(k->name);
        if (!o) { continue; }
        if (o->fixedsize != k->fixedsize) { ++artUnterschiede; }
        for (std::size_t i = 0; i < kMaxFlags; ++i) {
            if (klein(o->flagnamen[i]) != klein(k->flagnamen[i])) {
                ++flagUnterschiede;
                break;
            }
        }
    }
    std::printf("%s: wir %zu, NRC %zu, gemeinsam %zu; nur NRC %zu, nur wir %zu; "
                "Punkt/Gruppe verschieden %zu, Spawnflags verschieden %zu\n",
                titel, a.size(), b.size(), a.size() - nMehr, nFehlen, nMehr, artUnterschiede, flagUnterschiede);
    std::printf("  nur NRC:%s\n  nur wir:%s\n", fehlen.c_str(), mehr.c_str());
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Aufruf: karteklassen <data/karte> [<NRC gamepacks>]\n");
        return 2;
    }
    const std::filesystem::path daten = argv[1];

    // --- QUAKED (eclass_def.cpp) ------------------------------------------
    {
        EntitaetsKlassen kl;
        const std::string def =
            "// Kommentar mit /*QUAKED falsch (1 1 1) ? */ wird uebergangen\n"
            "/*QUAKED func_door (0 .5 .8) ? START_OPEN x CRUSHER unused_bit LOCKED\n"
            "Eine Tuer.\n"
            "model=\"models/door.md3\"\n"
            "*/\n"
            "/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32) KEEP_PREV - SPAWN_WEAPON\n"
            "Startpunkt * mit Stern\n"
            "*/\n"
            "/*QUAKED nur_name\n"
            "*/\n"
            "/*QUAKED kaputt (1 0 0) (1 2 3\n"
            "*/\n"
            "/*QUAKED func_door (1 1 1) ?\n"
            "zweite\n"
            "*/\n";
        const std::size_t fehler = kl.defLesen(def, "test.def");
        const EntitaetsKlasse* tuer = kl.finden("func_door");
        pruefe(fehler == 1, "defLesen: ein Fehler (kaputter Kasten -> Eclass_InitFromText gibt 0)");
        pruefe(tuer && !tuer->fixedsize && tuer->farbe == Vektor3f(0, 0.5f, 0.8f), "func_door: Gruppe ('?'), Farbe (0 .5 .8)");
        pruefe(tuer && tuer->flagnamen[0] == "START_OPEN" && tuer->flagnamen[1].empty() && tuer->flagnamen[2] == "CRUSHER"
                   && tuer->flagnamen[3].empty() && tuer->flagnamen[4] == "LOCKED",
               "func_door: Spawnflags nach Bit, 'x' und 'unused*' freigelassen");
        pruefe(tuer && tuer->kommentar == "Eine Tuer.\nmodel=\"models/door.md3\"\n" && tuer->modell == "models/door.md3",
               "func_door: Kommentar ab der zweiten Zeile, model= (setSpecialLoad)");
        pruefe(tuer && tuer->attribute.size() == 3 && tuer->attribute[0].first == "angle" && tuer->attribute[0].second.typ == "direction"
                   && tuer->attribute[1].first == "model" && tuer->attribute[2].first == "noise",
               "Gruppe: Standardattribute angle (direction), model, noise");
        const EntitaetsKlasse* start = kl.finden("INFO_PLAYER_START");
        pruefe(start && start->fixedsize && start->mins == Vektor3f(-16, -16, -24) && start->maxs == Vektor3f(16, 16, 32)
                   && start->attribute[0].second.typ == "angle" && start->attribute[0].second.wert == "0",
               "info_player_start: fester Kasten, angle 'Yaw Angle' 0, Suche ohne Gross/klein");
        pruefe(start && start->flagnamen[1].empty() && start->flagnamen[2] == "SPAWN_WEAPON" && start->kommentar == "Startpunkt * mit Stern\n",
               "info_player_start: '-' freigelassen, '*' im Kommentar bleibt");
        const EntitaetsKlasse* nur = kl.finden("nur_name");
        pruefe(nur && nur->attribute.empty() && nur->kommentar.empty() && !nur->fixedsize,
               "ohne Farbe: nur der Name (Eclass_InitFromText kehrt frueh zurueck)");
        pruefe(kl.anzahl() == 3, "die zweite func_door wird verworfen (erste gewinnt)");
    }

    // --- .ent (eclass_xml.cpp) --------------------------------------------
    {
        EntitaetsKlassen kl;
        const std::string ent =
            "<?xml version=\"1.0\"?>\n<classes>\n"
            "<list name=\"team\">\n<item name=\"Rot\" value=\"1\"/>\n<item name=\"Blau\" value=\"2\"/>\n</list>\n"
            "<group name=\"func_bobbing\" color=\"0 .5 .8\">\nBewegt sich &amp; schwingt.\n"
            "<real key=\"speed\" name=\"Speed\" value=\"4\">Sekunden &lt;je&gt; Zyklus</real>\n"
            "<flag key=\"X_AXIS\" name=\"X Axis\" bit=\"0\">entlang x</flag>\n"
            "<team key=\"team\" name=\"Team\"/>\n"
            "</group>\n"
            "<point name=\"misc_model\" color=\"1 .5 .25\" box=\"-16 -16 -16 16 16 16\" model=\"models\\map_objects\\x.md3\">\n"
            "<model key=\"model\" name=\"Model\">die Datei</model>\n"
            "<angle key=\"angle\" name=\"Yaw\"/>\n"
            "</point>\n"
            "</classes>\n";
        const std::size_t fehler = kl.entLesen(ent, "test.ent");
        const EntitaetsKlasse* b = kl.finden("func_bobbing");
        pruefe(fehler == 0, ".ent ohne Fehler gelesen");
        pruefe(b && !b->fixedsize && b->farbe == Vektor3f(0, 0.5f, 0.8f), ".ent group: Farbe, Gruppe");
        pruefe(b && b->attribute.size() == 3 && b->attribute[0].first == "speed" && b->attribute[0].second.typ == "real"
                   && b->attribute[0].second.wert == "4" && b->attribute[0].second.beschreibung == "Sekunden <je> Zyklus",
               ".ent: Attribut mit Typ, Wert, Beschreibung (Entitaeten ersetzt)");
        pruefe(b && b->flagnamen[0] == "X_AXIS" && b->flagAttribut[0] == 1, ".ent: flag bit 0");
        pruefe(b && b->attribute[2].second.typ == "team" && kl.liste("team") && kl.liste("team")->size() == 2,
               ".ent: Listentyp als Attributtyp, Liste eingetragen");
        pruefe(b && b->kommentar == "\nBewegt sich & schwingt.\nspeed : Sekunden <je> Zyklus\nX_AXIS : entlang x\nteam : \n",
               ".ent: Kommentar = Text + 'key : Beschreibung' mit dem Leerraum dazwischen");
        const EntitaetsKlasse* m = kl.finden("misc_model");
        pruefe(m && m->fixedsize && m->mins == Vektor3f(-16, -16, -16) && m->maxs == Vektor3f(16, 16, 16)
                   && m->modell == "models/map_objects/x.md3" && m->miscmodelIs && m->miscmodelKey == "model" && m->hasAngles,
               ".ent point: box, model (PathCleaned), miscmodel, angle");
    }

    // --- erzeugte Definitionen (OpenJK) ------------------------------------
    {
        EntitaetsKlassen sp;
        const std::size_t fehler = sp.ordnerLaden((daten / "ja").string(), "sp");
        std::printf("JA SP: %zu Klassen, %zu Fehler\n", sp.anzahl(), fehler);
        pruefe(fehler == 0 && sp.anzahl() > 200, "data/karte/ja (sp): ueber 200 Klassen ohne Fehler");
        const EntitaetsKlasse* w = sp.finden("worldspawn");
        const EntitaetsKlasse* npc = sp.finden("NPC_spawner");
        const EntitaetsKlasse* tuer = sp.finden("func_door");
        pruefe(w && !w->fixedsize && tuer && !tuer->fixedsize, "JA SP: worldspawn und func_door sind Gruppen");
        pruefe(npc && npc->fixedsize && npc->mins == Vektor3f(-16, -16, -24) && npc->maxs == Vektor3f(16, 16, 40)
                   && npc->flagnamen[4] == "DROPTOFLOOR" && npc->flagnamen[5] == "CINEMATIC",
               "JA SP: NPC_spawner (-16 -16 -24)-(16 16 40), Spawnflags 16 DROPTOFLOOR, 32 CINEMATIC");
        pruefe(sp.finden("misc_model_breakable") != nullptr && sp.finden("target_level_change") != nullptr,
               "JA SP: misc_model_breakable, target_level_change da");
        EntitaetsKlassen mp;
        (void)mp.ordnerLaden((daten / "ja").string(), "mp");
        std::printf("JA MP: %zu Klassen\n", mp.anzahl());
        pruefe(mp.anzahl() > 200 && mp.finden("team_CTF_redflag") != nullptr && sp.finden("team_CTF_redflag") == nullptr,
               "JA MP: eigene Datei (sp_ wird uebergangen), CTF-Flaggen nur in MP");
        EntitaetsKlassen jk2;
        (void)jk2.ordnerLaden((daten / "jk2").string(), "sp");
        std::printf("JK2 SP: %zu Klassen\n", jk2.anzahl());
        pruefe(jk2.anzahl() > 150 && jk2.finden("NPC_Kyle") != nullptr && jk2.finden("NPC_Tavion") != nullptr,
               "JK2 SP: ueber 150 Klassen, NPC_Kyle und NPC_Tavion da");
        // Movie Duels: JA + entdefs.h
        EntitaetsKlassen md;
        (void)md.ordnerLaden((daten / "ja").string(), "sp");
        const std::size_t vorher = md.anzahl();
        const std::size_t mdFehler = md.entdefsErgaenzen();
        std::printf("Movie Duels: %zu Klassen (JA %zu + %zu aus entdefs.h), %zu Fehler\n", md.anzahl(), vorher,
                    md.anzahl() - vorher, mdFehler);
        pruefe(mdFehler == 0 && md.anzahl() >= vorher, "Movie Duels: entdefs.h ohne Fehler ergaenzt");

        if (argc >= 3 && std::filesystem::is_directory(argv[2])) {
            const std::filesystem::path g = argv[2];
            EntitaetsKlassen nrcSp, nrcMp, nrcJk2;
            (void)nrcSp.ordnerLaden((g / "ja.game" / "base").string(), "sp");
            (void)nrcMp.ordnerLaden((g / "ja.game" / "base").string(), "mp");
            (void)nrcJk2.ordnerLaden((g / "jk2.game" / "base").string(), "sp");
            vergleichen("JA SP gegen NRC sp_entities.def", sp, nrcSp);
            vergleichen("JA MP gegen NRC mp_entities.ent", mp, nrcMp);
            vergleichen("JK2 SP gegen NRC jk2 sp_entities.def", jk2, nrcJk2);
        }
    }

    std::printf("karteklassen: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
