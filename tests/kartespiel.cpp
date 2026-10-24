// kartespiel.cpp - Karten-Editor Stufe 2, Punkt 11: die Spielbeschreibungen
// (ja.game, jk2.game), die Mod-Auswahl der Seite "Game", die Vorgaben der
// Seite "Build", die Suchordner (QE_InitVFS) und "Snap plane points to
// integer grid" wie NetRadiant-Custom 20260114.
//
// Sollwerte von Hand aus dem NRC-Quelltext:
//   - gamepacks/games/ja.game, jk2.game: name, engine_win32, mp_engine_win32,
//     basegamename, unknowngamename, knowngame(name), default_scale.
//   - gtkdlgs.cpp:119-143: basegame -> 0 (gesperrt), knowngame -> 1 (gesperrt),
//     sonst 2 bzw. 1 ohne knowngame (bearbeitbar); Namen ebenso, eigene Mod ""
//     als fs_game.
//   - watchbsp.cpp:136-196: "default:" am Anfang = leer; Argumente
//     "+set sv_pure 0[ +set fs_game X] +devmap %mapname%".
//   - qe3.cpp:68-102: zusaetzliche Pfade, <Engine><fs_game>/, <Engine><basegame>/,
//     doppelte (ohne Gross/Klein) nur einmal.
//   - brush.h:826-837: quantiseInteger = lrint, 64.75 -> 65, 0.25 -> 0.
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/spielbeschreibung.h"
#include "bhed/karte/texdef.h"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("%s %s\n", ok ? "OK    " : "FEHLER", was.c_str());
    if (!ok) { ++g_fehler; }
}

bool gleich(const SpielAuswahl& a, int spiel, const std::string& fs, bool bearbeitbar) {
    return a.spiel == spiel && a.fsGame == fs && a.bearbeitbar == bearbeitbar;
}

}  // namespace

int main() {
    // --- ScanForGames / CGameDescription ---------------------------------------
    const auto& spiele = spielBeschreibungen();
    pruefe(spiele.size() == 2 && std::string(spiele[0].datei) == "ja.game" && std::string(spiele[1].datei) == "jk2.game",
           "zwei .game-Dateien in Ordnerreihenfolge: ja.game, jk2.game");
    pruefe(std::string(spiele[0].name) == "Jedi Knight Jedi Academy" && std::string(spiele[1].name) == "Jedi Knight II: Jedi Outcast",
           "\"Select the game\": die name-Attribute");
    pruefe(std::string(spiele[0].engine) == "jaSP.exe" && std::string(spiele[0].mpEngine) == "jaMP.exe" &&
               std::string(spiele[1].engine) == "jk2SP.exe" && std::string(spiele[1].mpEngine) == "jk2MP.exe",
           "engine_win32 / mp_engine_win32");
    pruefe(spiele[0].standardSkala == 0.25F && spiele[1].standardSkala == 0.25F && spiele[0].spielModusZeigen && spiele[1].spielModusZeigen,
           "default_scale 0.25, show_gamemode");
    pruefe(std::string(spielBeschreibung("JK2.GAME").typ) == "jk2" && std::string(spielBeschreibung("q3.game").typ) == "ja",
           "Datei ohne Gross/Klein; unbekannt -> ja.game");

    // --- Game_constructPreferences: Select mod -----------------------------------
    const SpielBeschreibung& ja = spielBeschreibung("ja.game");
    const SpielBeschreibung& jk2 = spielBeschreibung("jk2.game");
    const std::vector<std::string> nJa = modNamen(ja);
    pruefe(nJa == std::vector<std::string>{"Jedi Knight Jedi Academy", "Movie Battles II", "Custom JA modification"},
           "ja: basegamename, knowngamename, unknowngamename");
    pruefe(modNamen(jk2) == std::vector<std::string>{"Jedi Knight II Outcast", "Custom JK2 modification"}, "jk2: ohne knowngame zwei Eintraege");
    pruefe(gleich(spielAuswahlFuerOrdner(ja, "base"), 0, "base", false) && gleich(spielAuswahlFuerOrdner(ja, "BASE"), 0, "BASE", false),
           "gamecombo_for_dir: base -> 0, gesperrt (path_equal ohne Gross/Klein)");
    pruefe(gleich(spielAuswahlFuerOrdner(ja, "mbii"), 1, "mbii", false), "gamecombo_for_dir: knowngame MBII -> 1, gesperrt");
    pruefe(gleich(spielAuswahlFuerOrdner(ja, "meinmod"), 2, "meinmod", true) && gleich(spielAuswahlFuerOrdner(jk2, "meinmod"), 1, "meinmod", true),
           "gamecombo_for_dir: eigene Mod -> 2 (jk2: 1), bearbeitbar");
    pruefe(gleich(spielAuswahlFuerName(ja, ""), 0, "base", false) && gleich(spielAuswahlFuerName(ja, "Jedi Knight Jedi Academy"), 0, "base", false),
           "gamecombo_for_gamename: leer / basegamename -> base");
    pruefe(gleich(spielAuswahlFuerName(ja, "Movie Battles II"), 1, "MBII", false), "gamecombo_for_gamename: knowngamename -> MBII");
    pruefe(gleich(spielAuswahlFuerName(ja, "Custom JA modification"), 2, "", true) &&
               gleich(spielAuswahlFuerName(jk2, "Custom JK2 modification"), 1, "", true),
           "gamecombo_for_gamename: eigene -> leeres fs_game, bearbeitbar");

    // --- DefaultableString, constructEngineArgs ----------------------------------
    VorgabeText t;
    t.importieren("default:jaSP.exe");
    pruefe(t.text.empty() && t.mitVorgabe("jaSP.exe") == "default:jaSP.exe" && t.wert("jaSP.exe") == "jaSP.exe",
           "DefaultableString: \"default:...\" gilt als leer, Feld zeigt default:<Vorgabe>");
    t.importieren("default:");
    pruefe(t.text.empty(), "DefaultableString: auch \"default:\" allein ist leer");
    t.importieren("openjk.x86.exe");
    pruefe(t.text == "openjk.x86.exe" && t.mitVorgabe("jaSP.exe") == "openjk.x86.exe" && t.wert("jaSP.exe") == "openjk.x86.exe",
           "DefaultableString: eigener Text bleibt");
    t.importieren(" default:x");
    pruefe(t.text == " default:x", "DefaultableString: string_equal_prefix - nur ganz vorn");
    pruefe(engineArgumente("base", "base") == "+set sv_pure 0 +devmap %mapname%", "constructEngineArgs ohne Mod");
    pruefe(engineArgumente("meinmod", "base") == "+set sv_pure 0 +set fs_game meinmod +devmap %mapname%", "constructEngineArgs mit Mod");

    // --- DirectoryCleaned, QE_InitVFS ------------------------------------------------
    pruefe(ordnerBereinigt("C:\\Spiele\\JA\\GameData") == "C:/Spiele/JA/GameData/" && ordnerBereinigt("x/") == "x/" && ordnerBereinigt("").empty(),
           "DirectoryCleaned: '\\\\' -> '/', '/' am Ende, leer bleibt leer");
    pruefe(vfsOrdner("C:/G/", "base", "base", {}) == std::vector<std::string>{"C:/G/base/"}, "QE_InitVFS ohne Mod: nur <Engine>base/");
    pruefe(vfsOrdner("C:/G/", "meinmod", "base", {"D:/x/", "", "d:/X/"}) == std::vector<std::string>{"D:/x/", "C:/G/meinmod/", "C:/G/base/"},
           "QE_InitVFS: zusaetzliche Pfade zuerst, leere und doppelte (ohne Gross/Klein) nicht, Mod vor base");

    // --- Face::m_quantise ----------------------------------------------------------------
    Karte k;
    auto welt = k.weltHolenOderAnlegen();
    auto p = std::make_shared<Pinsel>();
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    p->quaderBauen(Kasten(Vektor3f(32.5F, 32.5F, 32.5F), Vektor3f(32.25F, 32.25F, 32.25F)), "textures/common/caulk", proj, k.kontext);
    welt->kindEinfuegen(p);
    pruefe(!ebenenPunkteGanzzahlig(), "Vorgabe: quantiseFloating (Brush_Construct)");
    const std::string fliess = karteSchreiben(k);
    setzeEbenenPunkteGanzzahlig(true);
    const std::string ganz = karteSchreiben(k);
    setzeEbenenPunkteGanzzahlig(false);
    std::printf("%s", ganz.c_str());
    pruefe(fliess.find("( 0.25 ") != std::string::npos && fliess.find("64.75") != std::string::npos, "quantiseFloating: 0.25 und 64.75 bleiben");
    pruefe(ganz.find("0.25") == std::string::npos && ganz.find("64.75") == std::string::npos && ganz.find("( 65 ") != std::string::npos &&
               ganz.find("( 0 ") != std::string::npos,
           "quantiseInteger: lrint - 0.25 -> 0, 64.75 -> 65");
    pruefe(karteSchreiben(k) == fliess, "wieder aus: dieselbe Ausgabe wie vorher");

    std::printf("%s: %d Fehler\n", g_fehler == 0 ? "karte_spiel bestanden" : "karte_spiel FEHLGESCHLAGEN", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
