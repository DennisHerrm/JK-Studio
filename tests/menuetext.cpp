// menuetext.cpp - Menue-Editor M1 Punkt 3: der Textpuffer des Code-Editors.
//
// Mehrfachcursor, Rueckgaengig (Tippen zusammengefasst), CRLF bleibt,
// Suchen mit Regex/Wort/Gross-Klein, Ersetzen mit $1, Klammerpaare ueber
// Strings und Kommentare hinweg, Falten mit Namen, Aenderungsrand,
// Vervollstaendigung, Syntaxfarben.
#include "bhed/menue/textpuffer.h"

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
    const std::string t0 = "menuDef\r\n{\r\n\tname \"a{b\" // }\r\n\titemDef\r\n\t{\r\n\t\tname knopf\r\n\t}\r\n}\r\n";
    Textpuffer p(t0);
    pruefe(p.text() == t0 && p.zeilen() == 9, "Text und Zeilen");
    pruefe(p.zeile(2) == "\tname \"a{b\" // }", "Zeile ohne CRLF");
    pruefe(p.zeilenUmbruch() == "\r\n", "CRLF erkannt");

    // Mehrfachcursor tippen
    p.einCursor(p.off(Pos{5, 2}));
    p.cursorDazu(p.off(Pos{2, 1}));
    p.tippe("X");
    p.tippe("Y");
    pruefe(p.zeile(5) == "\t\tXYname knopf" && p.zeile(2) == "\tXYname \"a{b\" // }", "zwei Cursor tippen");
    pruefe(p.rueckgaengig() && p.rueckgaengig() && p.text() == t0, "Rueckgaengig mit zwei Cursorn");
    pruefe(p.wiederholen() && p.wiederholen() && p.zeile(2) == "	XYname \"a{b\" // }", "Wiederholen");
    p.rueckgaengig();
    p.rueckgaengig();
    p.einCursor(p.off(Pos{5, 2}));
    p.tippe("a");
    p.tippe("b");
    p.tippe("c");
    pruefe(p.rueckgaengig() && p.text() == t0 && !p.kannRueckgaengig(), "Tippen ist EIN Rueckgaengig-Schritt");

    // Ruecktaste an einem Zeilenanfang nimmt \r\n zusammen weg
    p.einCursor(p.zeilenAnfang(1));
    p.loescheZurueck();
    pruefe(p.zeile(0) == "menuDef{", "Ruecktaste loescht CRLF als eins");
    p.rueckgaengig();

    // neue Zeile mit Einrueckung, nach { eine Stufe tiefer
    p.einCursor(p.zeilenEnde(4));
    p.neueZeile();
    pruefe(p.zeile(5) == "\t\t" && p.pos(p.cursor[0].pos).zeile == 5, "neue Zeile nach { eingerueckt");
    p.rueckgaengig();

    // Suchen
    SuchOptionen o;
    pruefe(p.suche("name", o).size() == 2, "suche 'name' 2x");
    o.wort = true;
    pruefe(p.suche("item", o).empty(), "ganzes Wort: 'item' nicht in itemDef");
    o.wort = false;
    o.gross = true;
    pruefe(p.suche("MENUDEF", o).empty(), "Gross/Klein beachten");
    o.gross = false;
    o.regex = true;
    std::string fehler;
    pruefe(p.suche("(", o, &fehler).empty() && !fehler.empty(), "ungueltiger Regex meldet Fehler");
    pruefe(p.ersetzeAlle("name (\w+)", "name \"$1\"", o) == 1 && p.zeile(5) == "\t\tname \"knopf\"", "Regex-Ersetzen mit $1");
    p.rueckgaengig();

    // Klammerpaare: die { im String und die } im Kommentar zaehlen nicht
    const std::uint32_t auf = p.zeilenAnfang(1);
    const auto zu = p.partnerKlammer(auf);
    pruefe(zu.has_value() && p.pos(*zu).zeile == 7, "Klammerpaar ueber String und Kommentar");
    // Falten
    const auto f = p.falten();
    pruefe(f.size() == 2 && f[0].kopf == "menuDef" && f[0].von == 0 && f[0].bis == 7 && f[1].kopf == "itemDef" &&
               f[1].name == "knopf",
           "Falten: menuDef 0-7, itemDef mit Namen");

    // Strg+D
    p.einCursor(p.off(Pos{2, 2}));
    pruefe(p.naechstesVorkommenDazu() && p.auswahlText() == "name", "Strg+D waehlt das Wort");
    pruefe(p.naechstesVorkommenDazu() && p.cursor.size() == 2, "Strg+D: naechstes Vorkommen");

    // Aenderungsrand
    const ZeilenDiff d = zeilenDiff("a\nb\nc\nd\n", "a\nB\nc\nx\nd\n");
    pruefe(d.geaendert.size() == 6 && d.geaendert[0] == 0 && d.geaendert[1] == 1 && d.geaendert[2] == 0 &&
               d.geaendert[3] == 1 && d.geaendert[4] == 0,
           "Aenderungsrand: geaendert/neu");
    const ZeilenDiff d2 = zeilenDiff("a\nb\nc\n", "a\nc\n");
    pruefe(d2.geloeschtVor.size() == 1 && d2.geloeschtVor[0] == 1, "Aenderungsrand: geloescht vor Zeile 1");

    // Vervollstaendigung
    const auto v = vorschlaege("back", {"background", "backcolor", "cvar", "outlinecolor", "Backcolor"});
    pruefe(v.size() == 2 && v[0] == "backcolor" && v[1] == "background", "Vorschlaege mit Praefix, ohne Doppelte");

    // Farben
    const std::string m = "#include \"x.h\"\nitemDef { name \"a\" type ITEM_TYPE_BUTTON rect 1 2 3 4 text @MENUS_X action { open main } } // k\n";
    const auto fa = faerbe(m, Dialekt::MP, Spiel::JA);
    auto klasse = [&](const std::string& w) {
        const std::size_t a = m.find(w);
        for (const Farbstueck& s : fa) {
            if (s.anfang == a) { return s.klasse; }
        }
        return Farbklasse::Text;
    };
    pruefe(klasse("#include") == Farbklasse::Praeprozessor, "Farbe: Praeprozessor");
    pruefe(klasse("itemDef") == Farbklasse::Block, "Farbe: Block");
    pruefe(klasse("rect") == Farbklasse::MenuWort || klasse("rect") == Farbklasse::ItemWort, "Farbe: Schluesselwort");
    pruefe(klasse("ITEM_TYPE_BUTTON") == Farbklasse::Konstante, "Farbe: Konstante");
    pruefe(klasse("@MENUS_X") == Farbklasse::MenuText, "Farbe: @MENUS");
    pruefe(klasse("open") == Farbklasse::Skript, "Farbe: Skriptbefehl");
    pruefe(klasse("// k") == Farbklasse::Kommentar, "Farbe: Kommentar");
    pruefe(klasse("1 2") == Farbklasse::Zahl, "Farbe: Zahl");

    std::printf("menuetext: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
