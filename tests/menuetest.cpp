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
#include "bhed/menue/menue.h"

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

int main() {
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

    // --- Texte -----------------------------------------------------------------
    Texte t;
    t.ladeStr("strings/English/MENUS.str", "VERSION \"1\"\r\nREFERENCE QUIT\r\nLANG_ENGLISH \"Quit\\nGame\"\r\n");
    pruefe(t.aufloesen("@MENUS_QUIT") == "Quit\nGame", "Texte: .str");
    pruefe(t.aufloesen("@MENUS_FEHLT") == "@MENUS_FEHLT", "Texte: unbekannt bleibt");
    t.ladeSp("VERSION 1\nREFERENCE MENUS0\nINDEX 0\n{\n   REFERENCE ABOUT\n   TEXT_LANGUAGE1 \"About\"\n   TEXT_LANGUAGE3 \"Ueber\"\n}\n", 3);
    pruefe(t.aufloesen("@MENUS0_ABOUT") == "Ueber", "Texte: .sp Sprache 3");

    std::printf("menuetest: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
