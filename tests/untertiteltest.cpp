// untertiteltest.cpp - Zeitleiste 2: Untertitel wie das Spiel (bhed/untertitel.h)
//
// Eigene Mini-Dateien (StringEd .str, StripEd .sp) und die Regeln aus
// CG_ParseServerinfo / CG_CaptionText von Jedi Academy, Movie Duels und
// Jedi Outcast.
#include "bhed/menue/menue.h"
#include "bhed/untertitel.h"

#include <cstdio>
#include <set>
#include <string>
#include <vector>

using namespace bhed;

namespace {

int fehler = 0;

void pruefe(bool ok, const char* was) {
    std::printf("  %s  %s\n", ok ? "ok   " : "FEHLER", was);
    if (!ok) { ++fehler; }
}

std::string liste(const std::vector<std::string>& v) {
    std::string s;
    for (const std::string& x : v) { s += (s.empty() ? "" : ",") + x; }
    return s;
}

// Eigene StripEd-Datei wie strip/<karte>.sp von Jedi Outcast (REFERENCE oben = Paket).
const char* const kSp =
    "VERSION 1\r\n"
    "CONFIG W:\\bin\\striped.cfg\r\n"
    "ID 200\r\n"
    "REFERENCE ZL2_MINI\r\n"
    "DESCRIPTION \"zl2_mini\"\r\n"
    "COUNT 2\r\n"
    "INDEX 0\r\n"
    "{\r\n"
    "   REFERENCE BRAKK01\r\n"
    "   TEXT_LANGUAGE1 \"The dune calls.\"\r\n"
    "   TEXT_LANGUAGE3 \"Die Duene ruft.\"\r\n"
    "}\r\n"
    "INDEX 1\r\n"
    "{\r\n"
    "   REFERENCE BRAKK02\r\n"
    "   TEXT_LANGUAGE1 \"Wait\x85 what?. Go.\"\r\n"
    "}\r\n";

// Eigene StringEd-Datei wie strings/english/<karte>.str von Jedi Academy.
const char* const kStr =
    "VERSION \"1\"\r\n"
    "CONFIG W:\\bin\\striped.cfg\r\n"
    "FILENOTES \"\"\r\n"
    "REFERENCE BRAKK01\r\n"
    "LANG_ENGLISH \"First line\\nsecond line\"\r\n"
    "ENDMARKER\r\n";

}  // namespace

int main() {
    using untertitel::Spiel;
    std::printf("Pakete der Karte (CG_ParseServerinfo)\n");
    const std::set<std::string> da{"KEJIM_POST", "ARTUS_MINE", "ZL2_MINI"};
    const auto paketDa = [&da](const std::string& p) { return da.count(p) != 0U; };
    pruefe(liste(untertitel::pakete("maps/yavin1b.bsp", Spiel::JediAcademy)) == "YAVIN1B,YAVIN1", "JKA: yavin1b liest dazu YAVIN1");
    pruefe(liste(untertitel::pakete("maps/kejim_post.bsp", Spiel::JediAcademy)) == "KEJIM_POST", "JKA: der JK2-Teil ist auskommentiert");
    pruefe(liste(untertitel::pakete("maps/kejim_post.bsp", Spiel::MovieDuels)) == "KEJIM_POST,ARTUS_MINE", "MD: kejim_post + ARTUS_MINE");
    pruefe(liste(untertitel::pakete("yavin_swamp", Spiel::MovieDuels)) == "YAVIN_CANYON", "MD: yavin_swamp -> YAVIN_CANYON ohne Nachfrage");
    pruefe(liste(untertitel::pakete("maps/kejim_post.bsp", Spiel::JediOutcast, paketDa)) == "KEJIM_POST,ARTUS_MINE",
           "JK2: kejim_post laedt -> + ARTUS_MINE");
    pruefe(liste(untertitel::pakete("maps/yavin_swamp.bsp", Spiel::JediOutcast, paketDa)) == "YAVIN_CANYON",
           "JK2: yavin_swamp fehlt -> YAVIN_CANYON");
    pruefe(liste(untertitel::pakete("maps/ns_starpad.bsp", Spiel::JediOutcast, [](const std::string&) { return true; })) ==
               "NS_STARPAD,ARTUS_TOPSIDE,BESPIN_UNDERCITY",
           "JK2: ns_starpad mit zwei Zusatzpaketen");
    pruefe(liste(untertitel::pakete("maps/doom_comm.bsp", Spiel::JediOutcast, paketDa)) == "DOOM_COMM",
           "JK2: doom_comm laedt nicht -> keine Zusatzpakete");

    std::printf("Schluessel (PlayIcarusSound, CG_CaptionText)\n");
    pruefe(untertitel::referenz("sound/chars/Mini/BRAKK01.mp3") == "brakk01", "klein, ohne Endung, nach dem letzten /");
    pruefe(untertitel::referenz("brakk01.mp3").empty(), "ohne / kein Untertitel");

    std::printf("Jedi Outcast: eigene StripEd-Datei\n");
    menue::Texte sp;
    sp.ladeSp(kSp, 1);
    for (auto& [k, v] : sp.werte) { v = untertitel::jk2Reinigen(v); }
    std::string k;
    const std::string t1 = untertitel::text("sound/chars/mini/brakk01.mp3", {"ZL2_MINI"}, sp.werte, &k);
    std::printf("  \"%s\" (%s)\n", t1.c_str(), k.c_str());
    pruefe(t1 == "The dune calls." && k == "ZL2_MINI_BRAKK01", "Text und Schluessel aus dem Paket ZL2_MINI");
    pruefe(untertitel::text("sound/chars/mini/brakk02.wav", {"ZL2_MINI"}, sp.werte) == "Wait. what?  Go.",
           "FixIllegalChars: 0x85 -> '.', \"?.\" -> \"? \"");
    k.clear();
    pruefe(untertitel::text("sound/chars/mini/brakk03.mp3", {"ZL2_MINI"}, sp.werte, &k).empty() && k == "ZL2_MINI_BRAKK03",
           "unbekannt: kein Text, Schluessel der erste Versuch");
    pruefe(untertitel::text("sound/chars/mini/#zl2_mini_brakk01.mp3", {"ANDERES"}, sp.werte) == "The dune calls.",
           "# am Anfang: das Paket steht schon im Namen");
    pruefe(untertitel::text("sound/chars/mini/brakk01.mp3", {"KEJIM_POST", "ZL2_MINI"}, sp.werte) == "The dune calls.",
           "zweites Paket, wenn das erste nichts hat");

    std::printf("Jedi Academy: eigene StringEd-Datei\n");
    menue::Texte str;
    str.ladeStr("strings/english/zl2_mini.str", kStr, "ENGLISH");
    pruefe(untertitel::text("sound/chars/mini/brakk01.mp3", untertitel::pakete("maps/zl2_mini.bsp", Spiel::JediAcademy), str.werte) ==
               "First line\nsecond line",
           "Paket = Dateiname, \\n wird Zeilenumbruch");

    std::printf("%s (%d Fehler)\n", fehler == 0 ? "alles gut" : "FEHLER", fehler);
    return fehler == 0 ? 0 : 1;
}
