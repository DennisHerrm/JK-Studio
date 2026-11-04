// enttexttest.cpp - rc609 W2b: Entities als Text, Leser wie das Spiel (F11, F12, F13)
//
//   - Text -> lesen -> schreiben ist Byte-gleich (Kommentare, Leerzeilen, Reihenfolge)
//   - eine Aenderung wie im Inspektor aendert genau eine Zeile
//   - kaputter Text gibt einen Fehler mit Zeilennummer
//   - doppelte Schluessel und Gross/Klein wie im Spiel, je Fall mit Quellstelle
//   - Latin-1 bleibt erhalten, ein Zeichen jenseits davon wird gemeldet
#include "bhed/bsp.h"
#include "bhed/enttext.h"
#include "bhed/regelwerk.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was.c_str());
    if (!ok) { ++g_fehler; }
}
std::vector<std::string> zeilen(const std::string& t) {
    std::vector<std::string> z;
    std::stringstream s(t);
    std::string l;
    while (std::getline(s, l)) { z.push_back(l); }
    return z;
}
// Wie viele Zeilen sind anders (gleich lange Texte zeilenweise, sonst Laengenunterschied + Abweichungen).
int andereZeilen(const std::string& a, const std::string& b) {
    const auto za = zeilen(a);
    const auto zb = zeilen(b);
    int n = 0;
    std::size_t i = 0;
    std::size_t j = 0;
    while (i < za.size() && j < zb.size() && za[i] == zb[j]) { ++i; ++j; }
    std::size_t ea = za.size();
    std::size_t eb = zb.size();
    while (ea > i && eb > j && za[ea - 1] == zb[eb - 1]) { --ea; --eb; }
    n = static_cast<int>(std::max(ea - i, eb - j));
    return n;
}
std::string w(const MapEntity& e, const char* k) {
    const std::string* v = e.find(k);
    return v != nullptr ? *v : std::string("<keiner>");
}

const char* kText =
    "// Kopf: eine Mission\n"
    "{\n"
    "\"classname\" \"worldspawn\"\n"
    "\"message\" \"T\xFC" "re\"   // Latin-1\n"
    "}\n"
    "\n"
    "/* ein Block-\n"
    "   kommentar */\n"
    "{\n"
    "  \"classname\" \"info_player_start\"\n"
    "  \"origin\" \"0 0 24\"\n"
    "\n"
    "  \"angle\" \"90\"\n"
    "}\n"
    "{ classname info_null \"targetname\" \"ziel\" }\n"
    "{\n"
    "\"classname\" \"light\"\n"
    "\"light\" \"300\"\n"
    "}\n";
}  // namespace

int main() {
    regelwerkAnwendenSpiel(kRegelwerkOpenJK, -1);

    std::printf("Lesen und Schreiben\n");
    std::vector<MapEntity> e;
    std::vector<EntBlockSpanne> sp;
    EntTextFehler f;
    const bool ok = parseEntitiesGenau(kText, e, &sp, &f);
    pruefe(ok && e.size() == 4, "Text gelesen: 4 Entities (" + std::to_string(e.size()) + ")");
    pruefe(entTextAbgleichen(kText, e) == kText, "Text -> lesen -> schreiben Byte-gleich (Kommentare, Leerzeilen, Reihenfolge)");
    pruefe(sp.size() == 4 && entTextZeile(kText, sp[1].anf) == 8 && entTextZeile(kText, sp[1].ende) == 13,
           "Spanne der zweiten Entity: Zeile 9 bis 14");

    std::printf("Inspektor aendert genau eine Zeile\n");
    {
        std::vector<MapEntity> n = e;
        setKey(n[1], "origin", "5 6 7");
        const std::string r = entTextAbgleichen(kText, n);
        pruefe(andereZeilen(kText, r) == 1 && r.find("  \"origin\" \"5 6 7\"\n") != std::string::npos, "origin geaendert: eine Zeile");
        std::vector<MapEntity> zurueck;
        pruefe(parseEntitiesGenau(r, zurueck) && entListeGleich(zurueck, n), "der neue Text liest sich als die neuen Entities");
        n = e;
        setKey(n[1], "spawnflags", "1");
        const std::string r2 = entTextAbgleichen(kText, n);
        pruefe(andereZeilen(kText, r2) == 1 && r2.find("  \"angle\" \"90\"\n  \"spawnflags\" \"1\"\n") != std::string::npos,
               "neuer Schluessel: eine eingefuegte Zeile hinter dem letzten Paar, gleich eingerueckt");
        n = e;
        setKey(n[1], "angle", "");
        const std::string r3 = entTextAbgleichen(kText, n);
        pruefe(andereZeilen(kText, r3) == 1 && r3.find("angle") == std::string::npos, "Schluessel entfernt: eine Zeile weniger");
        n = e;
        setKey(n[2], "targetname", "neu");
        const std::string r4 = entTextAbgleichen(kText, n);
        pruefe(andereZeilen(kText, r4) == 1 && r4.find("{ classname info_null \"targetname\" \"neu\" }\n") != std::string::npos,
               "Einzeiler: nur der Wert, Rest der Zeile bleibt");
        n = e;
        n.erase(n.begin() + 2);
        const std::string r5 = entTextAbgleichen(kText, n);
        pruefe(andereZeilen(kText, r5) == 1 && r5.find("info_null") == std::string::npos, "Entity geloescht: ihr Block faellt weg");
        n = e;
        MapEntity neu;
        setKey(neu, "classname", "info_null");
        n.push_back(neu);
        const std::string r6 = entTextAbgleichen(kText, n);
        pruefe(r6.rfind(kText, 0) == 0 && r6.substr(std::string(kText).size()) == "{\n\"classname\" \"info_null\"\n}\n", "neue Entity hinten angehaengt");
    }

    std::printf("Fehler mit Zeilennummer\n");
    struct Fall {
        const char* text;
        EntFehler art;
        int zeile;
        const char* was;
    };
    const Fall faelle[] = {
        {"{\n\"classname\" \"worldspawn\"\n}\n{\n\"classname\" \"info_null\"\n", EntFehler::OhneSchliessendeKlammer, 4, "fehlende '}' (Zeile der '{')"},
        {"{\n\"classname\" \"worldspawn\"\n\"message\" \"offen\n}\n", EntFehler::OffenesAnfuehrungszeichen, 3, "offenes Anfuehrungszeichen"},
        {"{\n\"classname\" \"worldspawn\"\n\"message\" }\n", EntFehler::KlammerOhneWert, 3, "Schluessel ohne Wert"},
        {"\"classname\" \"worldspawn\"\n", EntFehler::KlammerErwartet, 1, "Text ohne '{'"},
        {"{\n\"classname\" \"worldspawn\"\n}\n}\n", EntFehler::KlammerErwartet, 4, "'}' zu viel"},
    };
    for (const Fall& c : faelle) {
        std::vector<MapEntity> x;
        EntTextFehler fe;
        const bool g = parseEntitiesGenau(c.text, x, nullptr, &fe);
        pruefe(!g && fe.art == c.art && fe.zeile == c.zeile, std::string(c.was) + ": Zeile " + std::to_string(fe.zeile));
    }
    {
        std::string t = "{\n\"classname\" \"worldspawn\"\n";
        for (int i = 0; i < 64; ++i) { t += "\"k" + std::to_string(i) + "\" \"1\"\n"; }
        t += "}\n";
        std::vector<MapEntity> x;
        EntTextFehler fe;
        pruefe(!parseEntitiesGenau(t, x, nullptr, &fe) && fe.art == EntFehler::ZuVielePaare && fe.zeile == 66,
               "65 Paare: MAX_SPAWN_VARS beim 65. Paar (Zeile 66)");
        std::string lang = "{\n\"classname\" \"worldspawn\"\n\"message\" \"" + std::string(1000, 'a') + "\"\n\"m2\" \"" + std::string(1000, 'b') +
                           "\"\n}\n";
        // 10 + 11 + 8 + 1001 + 3 + 1001 = 2034 <= 2048 erlaubt; 15 Zeichen mehr nicht.
        pruefe(parseEntitiesGenau(lang, x), "2034 Zeichen erlaubt");
        lang.insert(lang.find("\"m2\" \"") + 6, std::string(15, 'c'));
        pruefe(!parseEntitiesGenau(lang, x, nullptr, &fe) && fe.art == EntFehler::ZuVieleZeichen && fe.zeile == 4,
               "2049 Zeichen: MAX_SPAWN_VARS_CHARS in Zeile 4");
        pruefe(!entTextPruefen("{\n\"classname\" \"info_null\"\n}\n", x, nullptr, &fe) && fe.art == EntFehler::ErsteNichtWorldspawn && fe.zeile == 1,
               "erste Entity nicht worldspawn (SP_worldspawn, g_spawn.cpp:1481)");
        pruefe(entTextPruefen("{\n\"CLASSNAME\" \"WorldSpawn\"\n}\n", x, nullptr, &fe), "worldspawn ohne Gross/Klein (Q_stricmp)");
        // Mitten im Text: das Spiel liest "offen bis zum naechsten '"' - ohne Fehler; der Editor meldet Zeile 3.
        const char* mitte = "{\n\"classname\" \"worldspawn\"\n\"message\" \"offen\n\"x\" \"y\"\n}\n";
        pruefe(parseEntitiesGenau(mitte, x) && x[0].keys.size() == 3 && x[0].keys[1].second == "offen\n", "Spiel: Wert ueber das Zeilenende ohne Fehler (COM_Parse)");
        pruefe(!entTextPruefen(mitte, x, nullptr, &fe) && fe.art == EntFehler::OffenesAnfuehrungszeichen && fe.zeile == 3,
               "Editor: offenes Anfuehrungszeichen in Zeile 3 gemeldet");
        const char* spaeter = "{\n\"classname\" \"worldspawn\"\n\"origin\" \"1 2 3\" \"offen\n\"message\" \"m\"\n}\n";
        pruefe(!entTextPruefen(spaeter, x, nullptr, &fe) && fe.art == EntFehler::OffenesAnfuehrungszeichen && fe.zeile == 3,
               "Editor: Spiel bricht spaeter ab, gemeldet wird die Zeile des offenen Anfuehrungszeichens (3)");
    }

    std::printf("Doppelte Schluessel und Gross/Klein wie im Spiel\n");
    {
        // F11a: G_ParseField fuer jedes Paar der Reihe nach (OJK g_spawn.cpp:1093) - origin ist ein Feld: der letzte gilt.
        std::vector<MapEntity> x;
        parseEntities("{\n\"classname\" \"info_null\"\n\"origin\" \"0 0 0\"\n\"origin\" \"100 0 0\"\n}\n", x);
        pruefe(x.size() == 1 && x[0].origin == "100 0 0" && w(x[0], "origin") == "100 0 0", "doppeltes origin: das zweite (G_ParseField)");
        setKey(x[0], "origin", "5 5 5");
        const std::string t = writeEntities(x);
        pruefe(x[0].origin == "5 5 5" && t.find("\"origin\" \"0 0 0\"") != std::string::npos && t.find("\"origin\" \"5 5 5\"") != std::string::npos,
               "setKey aendert das origin, das das Spiel liest (das zweite)");
        // F11b: Q_stricmp in G_ParseField (OJK g_spawn.cpp:965)
        parseEntities("{\n\"Classname\" \"NPC_Stormtrooper\"\n\"ORIGIN\" \"1 2 3\"\n}\n", x);
        pruefe(x.size() == 1 && x[0].classname == "NPC_Stormtrooper" && x[0].origin == "1 2 3", "\"Classname\"/\"ORIGIN\": ohne Gross/Klein");
        // Kein Feld: G_SpawnString (OJK g_spawn.cpp:89; "skin" in NPC_spawn.cpp:2115) - der erste, Q_stricmp.
        parseEntities("{\n\"classname\" \"NPC_spawner\"\n\"SKIN\" \"a\"\n\"skin\" \"b\"\n}\n", x);
        pruefe(w(x[0], "skin") == "a", "skin (kein Feld): der erste ohne Gross/Klein (G_SpawnString)");
        setKey(x[0], "skin", "c");
        pruefe(x[0].keys[1].second == "c" && x[0].keys[2].second == "b", "setKey aendert dort ebenfalls den ersten");
        // JK2: G_SpawnString mit strcmp (codeJK2/game/g_spawn.cpp:54); "music" ist dort kein Feld (SP_worldspawn :1255).
        parseEntities("{\n\"classname\" \"worldspawn\"\n\"MUSIC\" \"a\"\n\"music\" \"b\"\n}\n", x);
        regelwerkAnwendenSpiel(kRegelwerkJediOutcast, -1);
        pruefe(w(x[0], "music") == "b", "JK2: music genau geschrieben (strcmp)");
        pruefe(w(x[0], "Music") == "<keiner>", "JK2: andere Schreibweise findet nichts");
        regelwerkAnwendenSpiel(kRegelwerkOpenJK, -1);
        pruefe(w(x[0], "Music") == "b", "OpenJK: music ist ein Feld - der letzte, ohne Gross/Klein");
        // Movie Duels: fullName ist ein Feld (MD9 g_spawn.cpp:373), in OpenJK nicht.
        parseEntities("{\n\"classname\" \"NPC_spawner\"\n\"fullName\" \"eins\"\n\"fullname\" \"zwei\"\n}\n", x);
        regelwerkAnwendenSpiel(kRegelwerkMovieDuels, -1);
        pruefe(w(x[0], "fullName") == "zwei", "Movie Duels: fullName ist ein Feld - der letzte");
        regelwerkAnwendenSpiel(kRegelwerkOpenJK, -1);
        pruefe(w(x[0], "fullName") == "eins", "OpenJK: fullName kein Feld - der erste");
        setKey(x[0], "fullName", "");
        pruefe(x[0].keys.size() == 1, "leerer Wert entfernt alle Paare des Namens");
    }

    std::printf("Leser wie COM_Parse (F12)\n");
    {
        std::vector<MapEntity> x;
        parseEntities("{\n// \"origin\" \"9 9 9\"\n\"classname\" \"info_null\"\n}", x);
        pruefe(x.size() == 1 && x[0].keys.size() == 1, "// Kommentar ueberlesen: ein Schluessel (" + std::to_string(x.empty() ? 0 : x[0].keys.size()) + ")");
        parseEntities("{ classname info_null \"origin\" \"1 2 3\" }", x);
        pruefe(x.size() == 1 && x[0].classname == "info_null" && x[0].origin == "1 2 3", "Token ohne Anfuehrungszeichen");
        parseEntities("{\n/* \"a\" \"b\" */ \"classname\" \"info_null\"\n}", x);
        pruefe(x.size() == 1 && x[0].keys.size() == 1, "/* */ ueberlesen");
        std::vector<MapEntity> y;
        EntTextFehler fe;
        // Ein Wort endet am ersten Byte ab 0x80 (int c = *data, char signed): message T + \xFCre - dann fehlt ein Wert.
        const bool g = parseEntitiesGenau("{\n\"classname\" \"info_null\"\nmessage T\xFC" "re\n}\n", y, nullptr, &fe);
        pruefe(!g && fe.art == EntFehler::KlammerOhneWert && fe.zeile == 4, "Latin-1 im Wort ohne Anfuehrungszeichen trennt es (q_shared.cpp:398)");
        std::string lump = "{\n\"classname\" \"worldspawn\"\n}\n";
        lump += '\0';
        lump += "{ \"classname\" \"info_null\" }";
        parseEntities(lump, x);
        pruefe(x.size() == 1, "NUL beendet den Text (BSP-Lump)");
    }

    std::printf("Latin-1 (F13)\n");
    {
        const std::string l1 = "T\xFC" "re";
        const std::string u8 = latin1NachUtf8(l1);
        pruefe(u8 == "T\xC3\xBC" "re", "Latin-1 FC wird in der Oberflaeche U+00FC");
        std::string zurueck;
        pruefe(utf8NachLatin1(u8 + "n", zurueck) && zurueck == "T\xFC" "ren", "\"n\" angehaengt: T FC ren - das Original-Byte bleibt");
        std::size_t stelle = 0;
        std::string zeichen;
        pruefe(!utf8NachLatin1("Preis \xE2\x82\xAC", zurueck, &stelle, &zeichen) && stelle == 6 && zeichen == "\xE2\x82\xAC",
               "Euro-Zeichen (nicht Latin-1): gemeldet an Stelle 6");
        std::vector<MapEntity> n = e;
        setKey(n[0], "message", "T\xFC" "ren");
        const std::string r = entTextAbgleichen(kText, n);
        pruefe(r.find("\"message\" \"T\xFC" "ren\"   // Latin-1\n") != std::string::npos && andereZeilen(kText, r) == 1,
               "geaenderter Latin-1-Wert: nur sein Token, Kommentar dahinter bleibt");
    }

    std::printf("Live-Schluessel\n");
    {
        MapEntity a = e[1];
        MapEntity b = a;
        setKey(b, "origin", "1 1 1");
        pruefe(entTextNurLive(a, b), "origin geaendert: live");
        setKey(b, "spawnflags", "2");
        pruefe(!entTextNurLive(a, b), "neuer Schluessel: nicht live");
    }

    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "enttext: alles gut" : "enttext: FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
