// menuepaket.cpp - Menue-Editor M2 Punkt 8 und 9: Im Spiel testen (Befehlszeile,
// nie gestartet) und eine Schrift fuer das Spiel bauen (Rundlauf mit dem Leser
// der Vorschau, ladeFontdat = CFontInfo).
#include "bhed/menue/imspiel.h"
#include "bhed/menue/schriftbau.h"
#include "bhed/menue/vorschau.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace bhed::menue;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
}  // namespace

int main() {
    // --- Punkt 8: Im Spiel testen -----------------------------------------------------------
    {
        MenueTestEingabe e;
        e.enginePfad = "C:/Spiele/JKA/";
        e.exe = "openjk_sp.x86_64.exe";
        e.testOrdner = "C:/Users/x/AppData/jkstudio/menuetest";
        e.faehigkeit.gelesen = true;
        e.faehigkeit.homepath = true;
        e.faehigkeit.cdpath = true;
        MenueTestPlan p = menueTestPlanen(e);
        pruefe(p.weg == bhed::karte::SpielTestWeg::HomePfad && p.pk3 == "C:/Users/x/AppData/jkstudio/menuetest/base/zzz_jkstudio_menue.pk3",
               "Test: fs_homepath (hoechster Rang - ersetzt gleichnamige Menues des Spiels) " + p.pk3);
        pruefe(p.befehl == "\"C:/Spiele/JKA/openjk_sp.x86_64.exe\" +set fs_homepath \"C:/Users/x/AppData/jkstudio/menuetest\" +echo jkstudio_menu_test",
               "Test: Befehlszeile ohne devmap, ein Befehl gegen den Vorspann: " + p.befehl);
        e.fsGame = "MD";
        p = menueTestPlanen(e);
        pruefe(p.pk3.find("/MD/zzz_jkstudio_menue.pk3") != std::string::npos && p.befehl.find("+set fs_game MD ") != std::string::npos,
               "Test: Mod (Movie Duels) - Archiv in <test>/MD, fs_game MD");
        // M3 Punkt 4 (echter Start): das Movie-Duels-Programm nimmt ohne fs_game seine Mod MD - deren
        // Menues (basepath/MD) stehen dann ueber dem Testpaket in <test>/base; also fs_game base
        e.fsGame.clear();
        e.exe = "MovieDuels-SP.x86_64.exe";
        p = menueTestPlanen(e);
        pruefe(p.befehl.find("+set fs_game base ") != std::string::npos && p.pk3.find("/base/zzz_jkstudio_menue.pk3") != std::string::npos,
               "Test: Movie-Duels-Programm mit den Daten von base - fs_game base ausdruecklich: " + p.befehl);
        // M5b Punkt 5: ohne fs_homepath, mit fs_cdpath (jasp, jk2sp): eigener Ordner als fs_basepath, die
        // Spieldaten ueber fs_cdpath (niedrigster Rang) - das Testpaket steht darueber, das Spiel schreibt
        // nur in den eigenen Ordner (so in M5/M5b mit jasp und jk2sp aus Steam gestartet)
        e.exe = "jk2sp.exe";
        e.faehigkeit.homepath = false;
        p = menueTestPlanen(e);
        pruefe(p.weg == bhed::karte::SpielTestWeg::CdPfad && !p.bestaetigungNoetig &&
                   p.pk3 == "C:/Users/x/AppData/jkstudio/menuetest/base/zzz_jkstudio_menue.pk3" &&
                   p.befehl == "\"C:/Spiele/JKA/jk2sp.exe\" +set fs_basepath \"C:/Users/x/AppData/jkstudio/menuetest\" +set fs_cdpath \"C:/Spiele/JKA\" +echo jkstudio_menu_test",
               "Test: ohne fs_homepath ueber fs_basepath + fs_cdpath: " + p.befehl);
        // jk2sp.exe 1.04 stuerzt mit fs_basepath ab 105 Zeichen ab (gemessen: 100 laeuft, 110 Stapelueberlauf)
        e.testOrdner = "C:/" + std::string(110, 'x');
        p = menueTestPlanen(e);
        pruefe(p.weg == bhed::karte::SpielTestWeg::ModOrdner && p.bestaetigungNoetig, "Test: Jedi Outcast mit zu langem eigenem Ordner - Mod-Ordner nach Frage");
        e.testOrdner = "C:/Users/x/AppData/jkstudio/menuetest";
        e.exe = "openjk_sp.x86_64.exe";
        e.faehigkeit.cdpath = false;   // weder fs_homepath noch fs_cdpath
        p = menueTestPlanen(e);
        pruefe(p.weg == bhed::karte::SpielTestWeg::ModOrdner && p.bestaetigungNoetig && p.befehl.empty(),
               "Test: ohne fs_homepath nur der Mod-Ordner, erst nach Bestaetigung");
        e.modOrdnerErlaubt = true;
        p = menueTestPlanen(e);
        pruefe(p.pk3 == "C:/Spiele/JKA/jkstudio_test/zzz_jkstudio_menue.pk3" && p.befehl.find("+set fs_game jkstudio_test") != std::string::npos,
               "Test: bestaetigt - eigener Mod-Ordner");
        e.exe.clear();
        pruefe(menueTestPlanen(e).weg == bhed::karte::SpielTestWeg::Keiner, "Test: ohne Programm kein Plan");
    }

    // --- Punkt 9: Schrift bauen und wie die Engine lesen ---------------------------------------
    {
        std::vector<GlyphBild> z;
        for (int c = 32; c < 127; ++c) {
            GlyphBild g;
            g.code = c;
            g.breite = c == ' ' ? 0 : 6 + c % 5;
            g.hoehe = c == ' ' ? 0 : (c == 'g' ? 12 : 9);
            g.x0 = c % 2;
            g.y0 = c == 'g' ? -8 : -9;
            g.vorschub = g.breite + 1 + (c == ' ' ? 4 : 0);
            g.deckung.assign(static_cast<std::size_t>(g.breite * g.hoehe), static_cast<std::uint8_t>(c));
            z.push_back(g);
        }
        SchriftMasse m;
        m.punkt = 12;
        m.aufstieg = 10;
        m.abstieg = 3;
        const SchriftDateien f = baueSchrift(z, m);
        pruefe(f.fontdat.size() == 7180, "Schrift: .fontdat hat sizeof(dfontdat_t) = 7180 Bytes");
        Schrift s;
        pruefe(ladeFontdat("probe", f.fontdat, s) && s.ok, "Schrift: CFontInfo liest sie");
        pruefe(s.pointSize == 12 && s.height == 13 && s.ascender == 10 && s.descender == 3, "Schrift: Punkt, Hoehe = Auf- + Abstieg");
        const Glyph& a = s.glyph[static_cast<unsigned char>('A')];
        pruefe(a.width == 6 + 'A' % 5 && a.height == 9 && a.baseline == 8 && a.horizOffset == 'A' % 2 && a.horizAdvance == a.width + 1,
               "Schrift: Masse von 'A' (baseline = Zeilen ueber der Grundlinie - 1 wie Ravens Schriften)");
        pruefe(s.glyph[static_cast<unsigned char>('g')].baseline == 7, "Schrift: 'g' reicht unter die Grundlinie");
        pruefe(f.texBreite == 256 && (f.texHoehe & (f.texHoehe - 1)) == 0, "Schrift: Grafik 256 breit, Hoehe Zweierpotenz");
        pruefe(f.tga.size() == 18U + static_cast<std::size_t>(f.texBreite * f.texHoehe * 4) && f.tga[2] == 2 && static_cast<unsigned char>(f.tga[16]) == 32,
               "Schrift: TGA 32 Bit unkomprimiert");
        // die Deckung des Zeichens steht dort, wohin s/t zeigen (Ursprung oben links)
        const int px = static_cast<int>(std::lround(a.s * static_cast<float>(f.texBreite)));
        const int py = static_cast<int>(std::lround(a.t * static_cast<float>(f.texHoehe)));
        const std::size_t q = 18U + (static_cast<std::size_t>(py) * static_cast<std::size_t>(f.texBreite) + static_cast<std::size_t>(px)) * 4U;
        pruefe(q + 3 < f.tga.size() && static_cast<unsigned char>(f.tga[q + 3]) == 'A' && static_cast<unsigned char>(f.tga[q]) == 255,
               "Schrift: weiss, Deckung im Alpha an s/t");
        pruefe(schriftBreite(s, "AA", 1.0F) == 2 * a.horizAdvance, "Schrift: Breite wie RE_Font_StrLenPixels");
        pruefe(cp1252ZuUnicode(0x80) == 0x20AC && cp1252ZuUnicode('e') == 'e' && cp1252ZuUnicode(0xE4) == 0xE4 && cp1252ZuUnicode(0x81) == 0,
               "Schrift: Windows-1252");
    }
    std::printf("menuepaket: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
