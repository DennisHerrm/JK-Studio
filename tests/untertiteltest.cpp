// untertiteltest.cpp - Zeitleiste 2: Untertitel wie das Spiel (bhed/untertitel.h)
//
// Eigene Mini-Dateien (StringEd .str, StripEd .sp) und die Regeln aus
// CG_ParseServerinfo / CG_CaptionText von Jedi Academy, Movie Duels und
// Jedi Outcast.
#include "bhed/menue/menue.h"
#include "bhed/untertitel.h"

#include <array>
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

    std::printf("Untertitel im Bild (CG_CaptionText, CG_DrawCaptionText)\n");
    // Eine eigene Schrift: jedes Zeichen 10 Punkte breit, Hoehe wie ergoec (20/22/5).
    menue::Schrift f;
    f.ok = true;
    f.shader = "fonts/zl2";
    f.pointSize = 20;
    f.height = 22;
    f.ascender = 17;
    f.descender = 5;
    for (menue::Glyph& g : f.glyph) {
        g.horizAdvance = 10;
        g.width = 8;
        g.height = 16;
        g.baseline = 14;
    }
    // 7 x "abcdefghi" mit Leerzeichen = 69 Zeichen: die Zeile erreicht 640 Punkte
    // beim 64. Zeichen, gebrochen wird nach dem letzten Leerzeichen (Stelle 60).
    std::string t;
    for (int n = 0; n < 7; ++n) { t += (n == 0 ? "" : " ") + std::string("abcdefghi"); }
    const untertitel::Untertitel u = untertitel::setzen(t, 6900, 1000.0, true, 48.0F, f, Spiel::JediAcademy);
    std::printf("  %zu Zeilen: \"%s\" | \"%s\", %d ms je Buchstabe, Mitte %d, erste Seite bis %.0f\n", u.zeilen.size(),
                u.zeilen[0].c_str(), u.zeilen.size() > 1 ? u.zeilen[1].c_str() : "", u.buchstabenMs, u.textY, u.ersteSeiteBis);
    pruefe(u.zeilen.size() == 2 && u.zeilen[0].size() == 60 && u.zeilen[1] == "abcdefghi", "gebrochen nach dem letzten Leerzeichen vor 640 Punkten");
    pruefe(u.buchstabenMs == 100 && u.ersteSeiteBis == 1000.0 + 69 * 100, "Buchstabenzeit = Klanglaenge / strlen, Seite = Zeichen der zwei Zeilen");
    pruefe(u.textY == 456, "in der Kamera: Mitte des unteren Balkens (480 - 48/2)");
    const std::vector<untertitel::Zeile> z = untertitel::zeigen(u, 1500.0, f, Spiel::JediAcademy, true, 48.0F);
    pruefe(z.size() == 2 && z[0].x == 20 && z[0].y == 432 && z[1].x == 275 && z[1].y == 452,
           "zwei Zeilen: y = Mitte - Schrifthoehe - 4, mittig (640 - Breite) / 2");
    pruefe(untertitel::zeigen(u, 999.0, f, Spiel::JediAcademy, true, 48.0F).empty(), "vor dem Klang nichts");
    // Clientbilder alle 8 ms ab der Ankunft (1000): das erste nach 7900 ist 7904.
    pruefe(!untertitel::zeigen(u, 7903.0, f, Spiel::JediAcademy, true, 48.0F).empty() &&
               untertitel::zeigen(u, 7904.0, f, Spiel::JediAcademy, true, 48.0F).empty(),
           "nach der Buchstabenzeit beider Zeilen vorbei - im ersten Clientbild danach (captionNextTextTime < cg.time)");
    const untertitel::Untertitel draussen = untertitel::setzen("abc\n def\nghi", 1100, 0.0, false, 0.0F, f, Spiel::JediAcademy);
    pruefe(draussen.zeilen.size() == 3 && draussen.zeilen[1] == "def" && draussen.textY == 396,
           "\\n bricht, Leerzeichen am Zeilenanfang faellt weg, ausserhalb: 0.88 * (480 - 1.5 * Hoehe)");
    const std::vector<untertitel::Zeile> s1 = untertitel::zeigen(draussen, 100.0, f, Spiel::JediAcademy, false, 0.0F);
    const std::vector<untertitel::Zeile> s2 = untertitel::zeigen(draussen, 700.0, f, Spiel::JediAcademy, false, 0.0F);
    std::printf("  Seite 1: %zu Zeilen, Seite 2: %zu Zeile(n) \"%s\" bei y %d\n", s1.size(), s2.size(), s2.empty() ? "" : s2[0].text.c_str(),
                s2.empty() ? 0 : s2[0].y);
    pruefe(s1.size() == 2 && s2.size() == 1 && s2[0].text == "ghi" && s2[0].y == 396 - 10 - 4,
           "zweite Seite: die dritte Zeile allein, y = Mitte - halbe Hoehe - 4");
    // 12 Zeichen, 1100 ms: 91 ms je Buchstabe. Seite 1 (6 Zeichen) bis 546, das
    // Spiel blaettert im Clientbild 552 und rechnet von dort: Seite 2 (3 Zeichen)
    // bis 552 + 273 = 825, vorbei im Bild 832.
    pruefe(untertitel::zeigen(draussen, 551.0, f, Spiel::JediAcademy, false, 0.0F).size() == 2 &&
               untertitel::zeigen(draussen, 552.0, f, Spiel::JediAcademy, false, 0.0F).size() == 1 &&
               untertitel::zeigen(draussen, 831.0, f, Spiel::JediAcademy, false, 0.0F).size() == 1 &&
               untertitel::zeigen(draussen, 832.0, f, Spiel::JediAcademy, false, 0.0F).empty(),
           "Seitenwechsel im ersten Clientbild nach captionNextTextTime, die naechste Seite zaehlt ab diesem Bild");

    std::printf("Wann das Spiel den Untertitel schickt (PlayIcarusSound)\n");
    untertitel::Klangfall kf;
    kf.imKino = true;
    pruefe(!untertitel::wirdGesendet(kf), "g_subtitles 0: nicht");
    kf.gSubtitles = 2;
    pruefe(untertitel::wirdGesendet(kf), "g_subtitles 2 in der Kamera: ja");
    kf.imKino = false;
    pruefe(!untertitel::wirdGesendet(kf), "g_subtitles 2 ohne Kamera: nicht");
    kf.gSubtitles = 0;
    kf.npcMitUntertiteln = true;
    kf.abstandQuadrat = 1199.0 * 1199.0;
    pruefe(untertitel::wirdGesendet(kf), "SET_USE_SUBTITLES, ohne Kamera, naeher als 1200: ja");
    kf.gedaempft = true;
    pruefe(!untertitel::wirdGesendet(kf), "CHAN_VOICE_ATTEN weiter als 350: nicht");
    kf.rundfunk = true;
    pruefe(untertitel::wirdGesendet(kf), "Rundfunk (Ansager, VOICE_GLOBAL, scriptrunner): immer");
    pruefe(untertitel::gSubtitlesVorgabe(Spiel::JediAcademy) == 0 && untertitel::gSubtitlesVorgabe(Spiel::MovieDuels) == 0 &&
               untertitel::gSubtitlesVorgabe(Spiel::JediOutcast) == 2,
           "Vorgabe g_subtitles: JKA 0, MD 0, JK2 2");
    const std::array<float, 4> gelb = untertitel::textFarbe("yellow");
    const std::array<float, 4> weiss = untertitel::textFarbe("PURPLE");
    pruefe(gelb[0] == 1 && gelb[1] == 1 && gelb[2] == 0 && weiss[2] == 1 && weiss[3] == 1, "SET_CAPTIONTEXTCOLOR: YELLOW, Unbekanntes weiss");

    std::printf("Movie Duels und Jedi Outcast\n");
    // Vorgabe aus der installierten MovieDuels-gamex86_64.dll (27.10.2026,
    // Cvar-Tabelle: "cg_textprintscale", "0.66f", CVAR_ARCHIVE).
    pruefe(untertitel::mdGroesse(untertitel::kMdTextprintscale) == 0.66F && untertitel::mdGroesse(5.0F) == 2.0F &&
               untertitel::mdGroesse(0.1F) == 0.25F,
           "MD: cg_textprintscale 0.66, begrenzt auf 0.25 .. 2");
    const untertitel::Untertitel md = untertitel::setzen(t, 6900, 0.0, true, 48.0F, f, Spiel::MovieDuels, 0.5F);
    const std::vector<untertitel::Zeile> mz = untertitel::zeigen(md, 10.0, f, Spiel::MovieDuels, true, 48.0F);
    pruefe(md.zeilen.size() == 1 && md.schriftGroesse == 0.5F && mz.size() == 1 && mz[0].y == 456 - 5 - 2 && mz[0].x == (640 - 345) / 2,
           "MD mit 0.5: halbe Schrift - eine Zeile, y = Mitte - halbe Hoehe - 4 * 0.5");
    pruefe(untertitel::breite(f, "^1ab", 1.0F, Spiel::JediAcademy) == 20 && untertitel::breite(f, "^1ab", 1.0F, Spiel::JediOutcast) == 40,
           "Breite: JKA ueberspringt ^1, JK2 zaehlt es");
    std::vector<menue::Prim> glyphen;
    menue::schriftZeichnen(f, 20, 432, "ab c", menue::Farbe{1, 1, 1, 1}, -1, 1.0F, false, glyphen);
    pruefe(glyphen.size() == 3 && glyphen[0].x == 20.0F && glyphen[0].y == 432.0F + (22 - 2) - 14 && glyphen[2].x == 50.0F,
           "RE_Font_DrawString: je Zeichen ein Bild, Grundlinie oy + Hoehe - Unterlaenge/2");

    std::printf("%s (%d Fehler)\n", fehler == 0 ? "alles gut" : "FEHLER", fehler);
    return fehler == 0 ? 0 : 1;
}
