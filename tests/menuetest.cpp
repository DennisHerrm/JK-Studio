// menuetest.cpp - Menue-Editor M1 Punkt 1: Parser und Modell wie die Engine.
//
// Prueft ohne Spieldaten:
//   - SP (COM_ParseExt): Namenstabellen, Flags, Farben, Skripte im Wortlaut
//     von PC_Script_Parse, Warnungen mit Zeile wie PC_ParseWarning,
//     typeData-Pflicht (cvarFloat vor type scheitert wie im Spiel).
//   - MP (botlib): globale Defines, #include, Makros mit Parametern,
//     Stringverkettung, "-5" als zwei Woerter, Fehler wie PC_SourceError.
//   - Menu_PostParse/Item_SetScreenCoords (fullscreen, border).
//   - Spannen: Schreiben ohne Aenderung byte-gleich; rect setzen ersetzt nur
//     die Zahlen (Kommentare bleiben); fehlendes rect wird eingefuegt;
//     rect aus einem Makro wird nicht angefasst.
//   - @MENUS-Texte aus .str und .sp.
#include "bhed/menue/werkzeuge.h"
#include "bhed/menue/kodierung.h"
#include "bhed/menue/menue.h"
#include "bhed/menue/vorschau.h"

#include <cstdio>
#include <map>
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
bool hatMeldung(const Datei& d, const std::string& teil, int zeile = -1) {
    for (const Meldung& m : d.meldungen) {
        if (m.text.find(teil) != std::string::npos && (zeile < 0 || m.zeile == zeile)) { return true; }
    }
    return false;
}
void zeigeMeldungen(const Datei& d) {
    for (const Meldung& m : d.meldungen) { std::printf("      %s\n", meldungText(d, m).c_str()); }
}
}  // namespace

// rc608: Die Reihenfolge von asset_model und model_g2anim entscheidet im Spiel, ob die Animation laeuft
void g2animReihenfolge() {
    const auto lesen = [](const char* text, Dialekt dia, Spiel spiel) {
        LeseOptionen o;
        o.dialekt = dia;
        o.spiel = spiel;
        return lies("ui/x.menu", text, o);
    };
    const char* const davor = "{ menuDef { name m itemDef { name a type 7 model_g2anim \"BOTH_STAND1\" asset_model \"models/players/kyle/model.glm\" } } }";
    const char* const dahinter = "{ menuDef { name m itemDef { name a type 7 asset_model \"models/players/kyle/model.glm\" model_g2anim \"BOTH_STAND1\" } } }";
    const auto wirkungslos = [&](const char* t, Dialekt dia, Spiel sp) {
        const Datei d = lesen(t, dia, sp);
        return !d.menus.empty() && !d.menus[0].items.empty() && d.menus[0].items[0].g2animOhneWirkung;
    };
    pruefe(!wirkungslos(davor, Dialekt::SP, Spiel::JA), "g2anim vor asset_model: SP spielt sie");
    pruefe(wirkungslos(dahinter, Dialekt::SP, Spiel::JA), "g2anim nach asset_model: SP zeigt die T-Pose");
    pruefe(!wirkungslos(davor, Dialekt::MP, Spiel::MD), "g2anim vor asset_model: MD-MP spielt sie");
    pruefe(wirkungslos(davor, Dialekt::MP, Spiel::JA), "OpenJK-MP: g2anim allein spielt nie");
}

// rc608 Punkt 14: die Tabelle Typ gegen Felder (NOTIZEN_rc608.md), gegen ui_shared SP und MP
void felderJeTyp() {
    struct Z { const char* k; int typ; int style; bool soll; };
    static const Z kZeilen[] = {
        {"text", ITEM_TYPE_MODEL, 0, false},       {"textalign", ITEM_TYPE_MODEL, 0, false},  {"textscale", ITEM_TYPE_MODEL, 0, false},
        {"forecolor", ITEM_TYPE_MODEL, 0, false},  {"forecolor", ITEM_TYPE_MODEL, WINDOW_STYLE_SHADER, true},
        {"cvar", ITEM_TYPE_MODEL, 0, false},       {"asset_model", ITEM_TYPE_MODEL, 0, true},  {"model_g2anim", ITEM_TYPE_MODEL, 0, true},
        {"text", ITEM_TYPE_LISTBOX, 0, false},     {"textscale", ITEM_TYPE_LISTBOX, 0, true}, {"textalign", ITEM_TYPE_LISTBOX, 0, false},
        {"feeder", ITEM_TYPE_LISTBOX, 0, true},    {"feeder", ITEM_TYPE_BUTTON, 0, false},
        {"textalign", ITEM_TYPE_TEXTSCROLL, 0, false}, {"textalignx", ITEM_TYPE_TEXTSCROLL, 0, true}, {"lineHeight", ITEM_TYPE_TEXTSCROLL, 0, true},
        {"text", ITEM_TYPE_BUTTON, 0, true},       {"cvar", ITEM_TYPE_BUTTON, 0, false},      {"cvar", ITEM_TYPE_YESNO, 0, true},
        {"cvar", ITEM_TYPE_TEXT, 0, true},         {"maxchars", ITEM_TYPE_EDITFIELD, 0, true}, {"maxchars", ITEM_TYPE_TEXT, 0, false},
        {"asset_model", ITEM_TYPE_BUTTON, 0, false}, {"name", ITEM_TYPE_MODEL, 0, true},      {"backcolor", ITEM_TYPE_MODEL, 0, true},
    };
    for (const Z& z : kZeilen) { pruefe(feldGilt(z.k, z.typ, z.style, Spiel::JA) == z.soll, std::string("Felder: ") + z.k + " bei Typ " + std::to_string(z.typ)); }
    pruefe(!feldGilt("model_g2anim", ITEM_TYPE_MODEL, 0, Spiel::JK2), "Felder: JK2 kennt kein model_g2anim");
    pruefe(feldGilt("model_origin", ITEM_TYPE_MODEL, 0, Spiel::JK2) && !feldGilt("model_origin", ITEM_TYPE_MODEL, 0, Spiel::MD), "Felder: model_origin nur JK2");
}

// rc608 Punkt 9: der Grund eines fullScreen-Menues, wie im Spiel fotografiert
void leeresMenue() {
    const auto prims = [](const std::string& kopf, Dialekt dia) {
        LeseOptionen o;
        o.dialekt = dia;
        o.spiel = Spiel::JA;
        const Datei d = lies("ui/x.menu", "{ menuDef { name m rect 0 0 640 480 visible 1 " + kopf + " } }", o);
        Umgebung u;
        u.dialekt = dia;
        std::vector<Prim> v;
        zeichneMenu(d, 0, u, v);
        return v;
    };
    pruefe(prims("fullScreen 1", Dialekt::SP).empty(), "SP fullScreen ohne background zeichnet nichts (schwarz)");
    const std::vector<Prim> farbe = prims("fullScreen 1 backcolor 0.2 0.3 0.8 1", Dialekt::SP);
    pruefe(farbe.size() == 1 && farbe[0].art == PrimArt::Fuellen && farbe[0].farbe.b > 0.7F && farbe[0].w == 640.0F, "SP fullScreen mit backcolor fuellt");
    // rc608 Teil 3b, im Spiel fotografiert: jk2sp.exe fuellt mit backcolor ohne Bild nichts, jasp schon
    {
        LeseOptionen o;
        o.dialekt = Dialekt::SP;
        o.spiel = Spiel::JK2;
        const Datei d = lies("ui/x.menu", "{ menuDef { name m rect 0 0 640 480 visible 1 fullScreen 1 backcolor 0.2 0.3 0.8 1 } }", o);
        Umgebung u;
        u.dialekt = Dialekt::SP;
        std::vector<Prim> v;
        zeichneMenu(d, 0, u, v);
        pruefe(v.empty(), "JK2 SP fullScreen mit backcolor fuellt nicht");
    }
    // rc608 Teil 3b, im Spiel fotografiert: Menu_Paint ruft Window_Paint immer
    {
        const std::vector<Prim> v = prims("fullScreen 1 style 1 backcolor 0 0 0 1", Dialekt::MP);
        pruefe(v.size() == 2 && v[1].art == PrimArt::Fuellen && v[1].farbe.a > 0.9F, "MP fullScreen mit style FILLED: die Farbe liegt ueber dem grauen Kasten");
    }
    // rc608 Teil 3b, im Spiel gemessen: Rahmendicke = size * Aufloesung / 640
    {
        LeseOptionen o;
        o.dialekt = Dialekt::MP;
        const Datei d = lies("ui/x.menu", "{ menuDef { name m rect 0 0 640 480 visible 1 itemDef { name k rect 10 10 100 50 border 1 bordercolor 1 1 1 1 visible 1 } } }", o);
        Umgebung u;
        u.dialekt = Dialekt::MP;
        u.rahmenX = 2.0F;
        u.rahmenY = 3.0F;
        std::vector<Prim> v;
        zeichneMenu(d, 0, u, v);
        bool seite = false, oben = false;
        for (const Prim& p : v) {
            seite = seite || (p.art == PrimArt::Fuellen && p.w == 2.0F && p.h == 50.0F);
            oben = oben || (p.art == PrimArt::Fuellen && p.h == 3.0F && p.w == 100.0F);
        }
        pruefe(seite && oben, "Rahmen rechnen mit der Aufloesung des Spiels");
    }
    const std::vector<Prim> bildSp = prims("fullScreen 1 background \"x\"", Dialekt::SP);
    pruefe(bildSp.size() == 1 && bildSp[0].shader == "x" && bildSp[0].farbe.a == 0.0F, "SP background ohne backcolor: Alpha 0");
    const std::vector<Prim> mp = prims("fullScreen 1", Dialekt::MP);
    pruefe(mp.size() == 1 && mp[0].shader == kDefaultShader && mp[0].w == 640.0F, "MP fullScreen ohne background: der graue Kasten");
    const std::vector<Prim> bildMp = prims("fullScreen 1 background \"x\" backcolor 1 0 0 0", Dialekt::MP);
    pruefe(bildMp.size() == 1 && bildMp[0].shader == "x" && bildMp[0].farbe.a == 1.0F, "MP background: unabhaengig von backcolor");
    pruefe(prims("fullScreen 0", Dialekt::MP).empty() && prims("fullScreen 0", Dialekt::SP).empty(), "ohne fullScreen nichts");
}

int main() {
    leeresMenue();
    g2animReihenfolge();
    felderJeTyp();
    // --- SP ------------------------------------------------------------------
    const std::string sp =
        "// Kopf\r\n"
        "{\r\n"
        "\tmenuDef\r\n"
        "\t{\r\n"
        "\t\tname\t\t\"testMenu\"\r\n"
        "\t\tfullScreen\t1\r\n"
        "\t\trect\t\t100 50 200 100\r\n"
        "\t\tdescAlignment\tITEM_ALIGN_CENTER\r\n"
        "\t\tonESC { play \"sound/interface/esc.wav\" ; close testMenu ; }\r\n"
        "\t\titemDef\r\n"
        "\t\t{\r\n"
        "\t\t\tname\t\tknopf\r\n"
        "\t\t\tstyle\t\tWINDOW_STYLE_SHADER\r\n"
        "\t\t\ttype\t\tITEM_TYPE_BUTTON\r\n"
        "\t\t\trect\t\t10 20 30 40\t// x y w h\r\n"
        "\t\t\tforecolor\t1 .5 0 1\r\n"
        "\t\t\ttextalign\tITEM_ALIGN_RIGHT\r\n"
        "\t\t\ttext\t\t@MENUS_QUIT\r\n"
        "\t\t\tvisible\t\t1\r\n"
        "\t\t\tdecoration\r\n"
        "\t\t\tfoo\t\t1\r\n"
        "\t\t}\r\n"
        "\t\titemDef\r\n"
        "\t\t{\r\n"
        "\t\t\tname\t\tohneRect\r\n"
        "\t\t\tvisible\t\t1\r\n"
        "\t\t}\r\n"
        "\t\titemDef\r\n"
        "\t\t{\r\n"
        "\t\t\tname\t\tschieber\r\n"
        "\t\t\ttype\t\tITEM_TYPE_BUTTON\r\n"
        "\t\t\tcvarFloat\tvolume 0.5 0 1\r\n"
        "\t\t}\r\n"
        "\t}\r\n"
        "}\r\n";
    LeseOptionen o;
    o.dialekt = Dialekt::SP;
    Datei d = lies("ui/test.menu", sp, o);
    pruefe(schreib(d) == sp, "SP: Schreiben ohne Aenderung byte-gleich");
    pruefe(d.menus.size() == 1, "SP: ein Menue");
    if (d.menus.size() == 1) {
        const Menu& m = d.menus[0];
        pruefe(m.window.name == "testMenu", "SP: name");
        pruefe(m.fullScreen == 1 && m.window.rect.x == 100.0F,
               "SP: gescheitertes Menue: kein Menu_PostParse (das Spiel laedt es nicht)");
        pruefe(m.descAlignment == ITEM_ALIGN_CENTER, "SP: descAlignment als Name");
        pruefe(m.onESC == "\"play\" \"sound/interface/esc.wav\" ; \"close\" \"testMenu\" ; ", "SP: onESC wie PC_Script_Parse: '" + m.onESC + "'");
        pruefe(!m.ok, "SP: cvarFloat nach type BUTTON: kein typeData -> Menue scheitert wie im Spiel");
        pruefe(m.items.size() == 3, "SP: drei Items gelesen (das dritte scheitert)");
        if (m.items.size() == 3) {
            const Item& a = m.items[0];
            pruefe(a.window.name == "knopf", "SP: item name");
            pruefe(a.window.style == WINDOW_STYLE_SHADER && a.type == ITEM_TYPE_BUTTON, "SP: style/type als Namen");
            pruefe(a.window.rectClient.x == 10.0F && a.window.rectClient.h == 40.0F, "SP: rect");
            pruefe(a.window.foreColor.g == 0.5F && (a.window.flags & WINDOW_FORECOLORSET) != 0, "SP: forecolor .5");
            pruefe(a.textalignment == ITEM_ALIGN_RIGHT, "SP: textalign als Name");
            pruefe(a.text == "@MENUS_QUIT", "SP: text");
            pruefe((a.window.flags & (WINDOW_VISIBLE | WINDOW_DECORATION)) == (WINDOW_VISIBLE | WINDOW_DECORATION),
                   "SP: visible + decoration");
            pruefe(!m.items[2].ok, "SP: drittes Item gescheitert");
        }
    }
    pruefe(hatMeldung(d, "Unknown item keyword 'foo'", 21), "SP: Warnung 'Unknown item keyword' in Zeile 21");
    {   // rc608: Jedi Outcast hat eigene Werte, unbekannte Cvars sind leer wie im Spiel
        pruefe(spielCvarVorgabe(Dialekt::MP, Spiel::JK2, "sv_hostname") == "noname", "JK2-MP: sv_hostname wie im Spiel");
        pruefe(spielCvarVorgabe(Dialekt::MP, Spiel::JK2, "ui_Name") == "Padawan", "JK2-MP: ui_Name wie nach dem Oeffnen im Spiel");
        pruefe(spielCvarVorgabe(Dialekt::MP, Spiel::JK2, "ui_about_gametype").empty(), "JK2-MP: ui_about_gametype kennt das Spiel im Hauptmenue nicht");
        pruefe(spielCvarVorgabe(Dialekt::MP, Spiel::JA, "sv_hostname") != "noname", "JKA-MP: bleibt bei seinen Werten");
    }
    {   // rc608: was dem Spiel das Menue nimmt, laesst dem Editor die uebrigen Elemente
        const std::string probe = "menuDef\n{\n name \"m\"\n rect 0 0 640 480\n itemDef\n {\n  name \"a\"\n  type ITEM_TYPE_CHECKBOX\n  rect 10 20 30 40\n  asset_model \"x.glm\"\n  visible 1\n }\n itemDef\n {\n  name \"b\"\n  type ITEM_TYPE_BUTTON\n  rect 50 60 70 80\n  visible 1\n }\n}\n";
        LeseOptionen spiel;
        const Datei g = lies("ui/p.menu", probe, spiel);
        pruefe(g.menus.size() == 1 && !g.menus[0].ok && g.menus[0].items.size() == 1 && g.menus[0].abbruchZeile == 0, "Wie im Spiel: das Menue bricht beim ungueltigen Schluessel ab");
        LeseOptionen ed = spiel;
        ed.weiterlesen = true;
        const Datei e = lies("ui/p.menu", probe, ed);
        pruefe(e.menus.size() == 1 && !e.menus[0].ok && e.menus[0].items.size() == 2 && e.menus[0].abbruchZeile == 10, "Editor: gleiches Urteil, die Zeile wird genannt, das zweite Element ist da");
        pruefe(e.menus[0].items.size() == 2 && e.menus[0].items[1].window.rect.x == 50.0F && e.menus[0].items[0].window.rect.y == 20.0F && (e.menus[0].items[0].window.flags & WINDOW_VISIBLE) != 0,
               "Editor: Bildschirmlage auch nach der ungueltigen Zeile, die uebrigen Schluessel des Elements gelten");
        pruefe(hatMeldung(e, "Couldn't parse item keyword 'asset_model'"), "Editor: dieselbe Meldung wie das Spiel");
    }
    pruefe(hatMeldung(d, "Couldn't parse item keyword 'cvarFloat'"), "SP: Warnung 'Couldn't parse item keyword'");
    if (g_fehler != 0) { zeigeMeldungen(d); }
    {
        const Meldung* m = nullptr;
        for (const Meldung& x : d.meldungen) {
            if (x.text.find("foo") != std::string::npos) { m = &x; }
        }
        pruefe(m != nullptr && meldungText(d, *m) == "WARNING: Unknown item keyword 'foo' Line #21 of File 'ui/test.menu'",
               "SP: Wortlaut wie PC_ParseWarning");
    }

    // Spannen: rect des ersten Items setzen
    {
        std::vector<Ersetzung> e;
        pruefe(setzeRect(d, 0, 0, Rechteck{12, 20, 30.5F, 40}, e), "SP: setzeRect");
        const std::string neu = anwenden(d.text, e);
        pruefe(neu.find("\t\t\trect\t\t12 20 30.5 40\t// x y w h\r\n") != std::string::npos,
               "SP: nur die Zahlen ersetzt, Kommentar bleibt");
        pruefe(neu.size() == sp.size() + 2, "SP: sonst nichts geaendert");
        Datei d2 = lies("ui/test.menu", neu, o);
        pruefe(d2.menus.size() == 1 && d2.menus[0].items[0].window.rectClient.x == 12.0F, "SP: neu gelesen x = 12");
    }
    {
        std::vector<Ersetzung> e;
        pruefe(setzeRect(d, 0, 1, Rechteck{1, 2, 3, 4}, e), "SP: setzeRect ohne rect fuegt ein");
        const std::string neu = anwenden(d.text, e);
        pruefe(neu.find("\t\t\tvisible\t\t1\r\n\t\t\trect\t\t1 2 3 4\r\n\t\t}") != std::string::npos,
               "SP: neue Zeile mit Einrueckung und CRLF vor der }");
        Datei d2 = lies("ui/test.menu", neu, o);
        pruefe(d2.menus.size() == 1 && d2.menus[0].items.size() >= 2 && d2.menus[0].items[1].window.rectClient.w == 3.0F,
               "SP: eingefuegtes rect gelesen");
    }

    // --- MP ------------------------------------------------------------------
    std::map<std::string, std::string> dateien;
    dateien["ui/jamp/menudef.h"] =
        "#define ITEM_TYPE_BUTTON 1\n#define WINDOW_STYLE_FILLED 1\n#define ITEM_ALIGN_CENTER 1\n"
        "#define MENU_TRUE 1\n#define LINKS 20\n";
    dateien["ui/jamp/teil.h"] = "#define BREITE 300\n#define KNOPF(n, y) name n rect LINKS y BREITE 30\n";
    LeseOptionen mo;
    mo.dialekt = Dialekt::MP;
    mo.globaleDefines = "ui/jamp/menudef.h";
    mo.lader = [&](const std::string& p, std::string& out) {
        const auto it = dateien.find(p);
        if (it == dateien.end()) { return false; }
        out = it->second;
        return true;
    };
    const std::string mp =
        "#include \"ui/jamp/teil.h\"\n"
        "{\n"
        "  menuDef {\n"
        "    name \"haupt\" \"menu\"\n"
        "    fullscreen MENU_TRUE\n"
        "    rect 0 0 640 480\n"
        "    border 1 borderSize 2\n"
        "    itemDef {\n"
        "      KNOPF(eins, 100)\n"
        "      type ITEM_TYPE_BUTTON\n"
        "      textalignx -5\n"
        "      style WINDOW_STYLE_FILLED\n"
        "      backcolor 0 0 0 .5\n"
        "      action { open \"setup\" ; setcvar ui_x 1 }\n"
        "      gibtsnicht 3\n"
        "    }\n"
        "    itemDef {\n"
        "      name zwei\n"
        "      rect 10 20 30 40\n"
        "      type 0\n"
        "    }\n"
        "  }\n"
        "}\n";
    Datei m = lies("ui/jamp/test.menu", mp, mo);
    pruefe(schreib(m) == mp, "MP: Schreiben ohne Aenderung byte-gleich");
    pruefe(m.menus.size() == 1, "MP: ein Menue");
    if (m.menus.size() == 1 && m.menus[0].items.size() == 2) {
        const Menu& mm = m.menus[0];
        pruefe(mm.window.name == "hauptmenu", "MP: aufeinanderfolgende Strings verkettet: '" + mm.window.name + "'");
        pruefe(mm.fullScreen == 1, "MP: globales Define MENU_TRUE");
        const Item& a = mm.items[0];
        pruefe(a.window.name == "eins", "MP: Makro mit Parametern: name");
        pruefe(a.window.rectClient.x == 20.0F && a.window.rectClient.y == 100.0F && a.window.rectClient.w == 300.0F,
               "MP: Makro mit Parametern: rect aus LINKS/BREITE");
        pruefe(a.window.rect.x == 22.0F && a.window.rect.y == 102.0F, "MP: Item_SetScreenCoords mit border/borderSize");
        pruefe(a.type == ITEM_TYPE_BUTTON && a.window.style == WINDOW_STYLE_FILLED, "MP: type/style ueber Defines");
        pruefe(a.textalignx == -5.0F, "MP: -5 als '-' und '5' (PC_Float_Parse)");
        pruefe(a.window.backColor.a == 0.5F, "MP: backcolor .5");
        pruefe(a.action == "\"open\" \"setup\" ; \"setcvar\" \"ui_x\" 1 ", "MP: action wie PC_Script_Parse: '" + a.action + "'");
        pruefe(mm.ok, "MP: Menue gelesen");
        std::vector<Ersetzung> e;
        pruefe(!setzeRect(m, 0, 0, Rechteck{1, 2, 3, 4}, e), "MP: rect aus einem Makro wird nicht ersetzt");
        e.clear();
        pruefe(setzeEigenschaft(m, 0, 0, "textalignx", {"7"}, e) && anwenden(m.text, e).find("textalignx 7\n") != std::string::npos,
               "MP: '-5' (zwei Woerter) als ein Wert ersetzt");
    } else {
        pruefe(false, "MP: zwei Items");
    }
    pruefe(hatMeldung(m, "unknown menu item keyword gibtsnicht", 15), "MP: 'unknown menu item keyword' Zeile 15");
    {
        Datei f = lies("ui/jamp/x.menu", "{ menuDef { name x\n itemDef {\n type falsch } } }\n", mo);
        pruefe(f.menus.size() == 1 && !f.menus[0].ok && hatMeldung(f, "expected integer but found falsch", 3),
               "MP: 'type falsch' bricht das Menue ab (expected integer, Zeile 3)");
    }
    pruefe(m.defines.count("ITEM_TYPE_BUTTON") == 1, "MP: Zahl-Defines fuer die Vervollstaendigung");
    if (g_fehler != 0) { zeigeMeldungen(m); }
    {
        Datei f = lies("ui/jamp/x.menu", "#include \"fehlt.h\"\n{ menuDef { name x } }\n", mo);
        pruefe(f.menus.empty() && hatMeldung(f, "file fehlt.h not found"), "MP: fehlendes #include beendet das Lesen");
        Datei g2 = lies("ui/jamp/x.menu", "#ifdef MENU_TRUE\n{ menuDef { name ja } }\n#else\n{ menuDef { name nein } }\n#endif\n", mo);
        pruefe(g2.menus.size() == 1 && g2.menus[0].window.name == "ja", "MP: #ifdef/#else/#endif");
    }

    // --- M2: assetGlobalDef so streng wie Asset_Parse ---------------------------
    {
        LeseOptionen so;
        so.dialekt = Dialekt::SP;
        // unbekannter Schluessel: die Engine liest das naechste Wort als Schluessel -
        // ein "}" als Argument beendet assetGlobalDef, menuDef danach wird gelesen
        Datei a = lies("ui/a.menu", "{ assetGlobalDef { foo \"}\" menuDef { name m } }\n", so);
        pruefe(a.assets.vorhanden && a.assets.eigenschaften.size() == 1 && !a.assets.eigenschaften[0].ok,
               "SP Asset_Parse: unbekannter Schluessel ohne Argumente");
        pruefe(a.menus.size() == 1 && a.menus[0].window.name == "m", "SP Asset_Parse: \"}\" nach unbekanntem Schluessel beendet assets");
        // bekannte: smallFont "x" 16 (Zahl wird gelesen)
        Datei b = lies("ui/b.menu", "{ assetGlobalDef { smallFont \"ergoec\" 16 fadeClamp 1.0 mediumFont \"anewhope\" 20 } }\n", so);
        pruefe(b.assets.eigenschaften.size() == 3 && b.assets.eigenschaften[2].args.size() == 2, "SP Asset_Parse: mediumFont mit 2 Argumenten");
        // stripedFile gibt es nur in JK2 (JK2_MODE); in JA ein unbekanntes Wort
        LeseOptionen jk = so;
        jk.spiel = Spiel::JK2;
        Datei c = lies("ui/c.menu", "{ assetGlobalDef { stripedFile \"menus\" } }\n", jk);
        pruefe(c.assets.eigenschaften.size() == 1 && c.assets.eigenschaften[0].ok, "SP Asset_Parse: stripedFile in JK2");
        Datei c2 = lies("ui/c.menu", "{ assetGlobalDef { stripedFile \"menus\" } }\n", so);
        pruefe(c2.assets.eigenschaften.size() == 2 && !c2.assets.eigenschaften[0].ok, "SP Asset_Parse: stripedFile in JA unbekannt");
        // Movie Duels: cursor_vader
        LeseOptionen md = so;
        md.spiel = Spiel::MD;
        Datei e = lies("ui/e.menu", "{ assetGlobalDef { cursor_vader \"gfx/x\" } }\n", md);
        pruefe(e.assets.eigenschaften.size() == 1 && e.assets.eigenschaften[0].ok, "SP Asset_Parse: cursor_vader in Movie Duels");
        // MP: kein mediumFont - unbekannt, die Argumente sind eigene Woerter
        LeseOptionen mo2;
        mo2.dialekt = Dialekt::MP;
        Datei f = lies("ui/jamp/f.menu", "{ assetGlobalDef { mediumFont \"ergoec\" 20 font \"anewhope\" 16 } }\n", mo2);
        pruefe(f.assets.eigenschaften.size() == 4 && !f.assets.eigenschaften[0].ok && f.assets.eigenschaften[3].ok,
               "MP Asset_Parse: mediumFont unbekannt, font bekannt");
    }

    // --- Texte -----------------------------------------------------------------
    Texte t;
    t.ladeStr("strings/English/MENUS.str", "VERSION \"1\"\r\nREFERENCE QUIT\r\nLANG_ENGLISH \"Quit\\nGame\"\r\n");
    pruefe(t.aufloesen("@MENUS_QUIT") == "Quit\nGame", "Texte: .str");
    pruefe(t.aufloesen("@MENUS_FEHLT") == "@MENUS_FEHLT", "Texte: unbekannt bleibt");
    t.ladeSp("VERSION 1\nREFERENCE MENUS0\nINDEX 0\n{\n   REFERENCE ABOUT\n   TEXT_LANGUAGE1 \"About\"\n   TEXT_LANGUAGE3 \"Ueber\"\n}\n", 3);
    pruefe(t.aufloesen("@MENUS0_ABOUT") == "Ueber", "Texte: .sp Sprache 3");
    // M2 (Bildvergleich mit dem Spiel): StringEd nimmt nach REFERENCE den REST der Zeile
    // (InsideQuotes) - Ravens MENUS.str hat "REFERENCE NEW MISSION_DESC", das darf NEW
    // nicht ueberschreiben. LANG_-Texte behalten innere Anfuehrungszeichen.
    {
        Texte r;
        r.ladeStr("strings/English/MENUS.str",
                  "REFERENCE           NEW\r\nNOTES               \"Used in first line of NEW GAME button\"\r\nLANG_ENGLISH        \"NEW\"\r\n\r\n"
                  "REFERENCE           NEW MISSION_DESC\r\nLANG_ENGLISH        \"Return to the mission selection screen\"\r\n"
                  "REFERENCE ZITAT // Kommentar\r\nLANG_ENGLISH \"Er sagte \"ja\" // kein Kommentar\"\r\n");
        pruefe(r.aufloesen("@MENUS_NEW") == "NEW", "Texte: REFERENCE NEW MISSION_DESC ueberschreibt NEW nicht (InsideQuotes)");
        pruefe(r.aufloesen("@MENUS_ZITAT") == "Er sagte \"ja\" // kein Kommentar", "Texte: innere Anfuehrungszeichen, // in Anfuehrung bleibt");
    }

    // M2 Punkt 6: eigene Texte schreiben und wie die Engine wieder lesen
    {
        const std::vector<TextEintrag> e = {{"MEIN_KNOPF", "Start\nnow", "Los\njetzt"}, {"ZITAT", "say \"hi\"", ""}};
        Texte de;
        de.ladeStr("strings/German/MEINMOD.str", schreibeStr("German", e), "GERMAN");
        pruefe(de.aufloesen("@MEINMOD_MEIN_KNOPF") == "Los\njetzt" && de.aufloesen("@MEINMOD_ZITAT") == "say 'hi'",
               "Texte: .str geschrieben und gelesen (Uebersetzung, #same, Zeilenumbruch)");
        Texte en;
        en.ladeStr("strings/English/MEINMOD.str", schreibeStr("English", e), "ENGLISH");
        pruefe(en.aufloesen("@MEINMOD_MEIN_KNOPF") == "Start\nnow", "Texte: englische .str");
        Texte stripEd;
        stripEd.ladeSp(schreibeSp("MEINMOD", 3, e), 3);
        pruefe(stripEd.aufloesen("@MEINMOD_MEIN_KNOPF") == "Los\njetzt", "Texte: .sp (StripEd, TEXT_LANGUAGE3)");
    }

    // M3 Punkt 7: asiatische Kodierungen wie tr_font.cpp sie liest
    {
        std::string fehlt;
        const std::string ko = ausUtf8("\xEC\x95\x88\xEB\x85\x95 \xEB\x98\xA0", Kodierung::Koreanisch, &fehlt);   // U+C548 U+B155, U+B620
        pruefe(ko == "\xBE\xC8\xB3\xE7 ?" && fehlt == "\xEB\x98\xA0", "Kodierung: KSC5601-Hangul, CP949-Erweiterung nicht darstellbar");
        pruefe(zuUtf8(ko, Kodierung::Koreanisch) == "\xEC\x95\x88\xEB\x85\x95 ?", "Kodierung: KSC5601 zurueck nach UTF-8");
        fehlt.clear();
        pruefe(ausUtf8("\xE3\x81\x93\xEF\xBD\xB1", Kodierung::Japanisch, &fehlt) == "\x82\xB1?" && fehlt == "\xEF\xBD\xB1",
               "Kodierung: Shift-JIS, Halbbreiten-Katakana nicht darstellbar");
        pruefe(ausUtf8("\xE4\xBD\xA0\xE5\xA5\xBD", Kodierung::Chinesisch) == "\xC4\xE3\xBA\xC3", "Kodierung: GB2312");
        pruefe(ausUtf8("\xE4\xBD\xA0\xE5\xA5\xBD", Kodierung::Taiwanesisch) == "\xA7\x41\xA6\x6E", "Kodierung: Big5");
        pruefe(ausUtf8("K\xC3\xA4se \xE2\x82\xAC", Kodierung::Latin1) == "K\xE4se ?", "Kodierung: Latin-1 wie bisher");
        pruefe(kodierungDerSprache("KOREAN") == Kodierung::Koreanisch && kodierungDerSprache("german") == Kodierung::Latin1 &&
                   kodierungDerSprache("thai") == Kodierung::Latin1,
               "Kodierung: se_language ohne Gross/Klein, Thai bleibt Latin-1");
        int n = 0;
        const unsigned char ein[] = {0xBE, 0xC8, 0x41};
        const unsigned c1 = zeichenLesen(Kodierung::Koreanisch, ein, 3, n);
        const int n1 = n;
        const unsigned c2 = zeichenLesen(Kodierung::Koreanisch, ein + 2, 1, n);
        pruefe(c1 == 0xBEC8U && n1 == 2 && c2 == 0x41U && n == 1, "Kodierung: AnyLanguage_ReadCharFromString");
        pruefe(zeichenLesen(Kodierung::Latin1, ein, 3, n) == 0xBEU && n == 1, "Kodierung: westlich ein Byte");
        pruefe(asienIndex(Kodierung::Koreanisch, 0xB0A1) == 1 && asienIndex(Kodierung::Koreanisch, 0xB0A0) == 0 &&
                   asienIndex(Kodierung::Japanisch, 0x8140) == 0 && asienIndex(Kodierung::Japanisch, 0x8180) == 63 &&
                   asienIndex(Kodierung::Taiwanesisch, 0xA1A1) == 65,
               "Kodierung: Collapse-Codes wie die Engine");
        Schrift w;
        w.name = "ergoec";
        w.height = 20;
        w.ascender = 15;
        w.pointSize = 20;
        Glyph g;
        std::string sh;
        pruefe(asienGlyph(Kodierung::Koreanisch, 0xBEC8, w, g, sh) && sh == "fonts/kor_32_1024_1" && g.s == 0.25F && g.t == 11.0F / 32.0F &&
                   g.width == 20 && g.horizAdvance == 19 && g.baseline == 15,
               "Kodierung: Hangul-Glyphe aus Seite 1 (GetLetter)");
        pruefe(asienGlyph(Kodierung::Chinesisch, 0xC4E3, w, g, sh) && sh.compare(0, 18, "fonts/chi_16_1024_") == 0 && g.horizAdvance == 23,
               "Kodierung: GB-Glyphe, Vorschub Hoehe + 3");
        w.name = "aurabesh";
        pruefe(!asienGlyph(Kodierung::Koreanisch, 0xBEC8, w, g, sh), "Kodierung: aurabesh ohne asiatische Glyphen");
        w.name = "ergoec";
        pruefe(schriftBreite(w, "\xBE\xC8", 1.0F, Kodierung::Koreanisch, true) == 15, "Kodierung: Breite mit fScaleAsian 0.75");
        pruefe(schriftBreite(w, "\xBE\xC8", 0.5F, Kodierung::Koreanisch, true) == 10, "Kodierung: unter 0.7 ohne Verkleinerung");
    }

    std::printf("menuetest: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
