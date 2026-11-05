// menuef6b.cpp - rc610 F6b: Funde B6b-1 bis B6b-5 im Menue-Editor, gegen die Engine-Quellen
// (OpenJK code/ui/ui_shared.cpp = SP, codemp/ui/ui_shared.c = MP; Movie Duels Update 9 ebenso):
//   B6b-1 Movie Duels MP hat eigene Tabellen (md9 codemp/ui/ui_shared.c itemParseKeywords :9617 ff.):
//         kein exec/ownerdrawID, dafuer cvarSubString; ITEM_TYPE_MODEL_ITEM 17 bekommt modelDef
//         (Item_ValidateTypeData :7868); ItemParse_cvarInt (:9257) prueft die Zahlen nicht;
//         PC_Script_Parse (:656-674) setzt "-" und eine Zahl wieder zusammen
//   B6b-2 Item_EnableShowViaCvar mit cvarSubString (OJ SP :6120, MD MP :2334)
//   B6b-3 PC_Script_Parse: char script[4096] (SP) bzw. [2048] (MP) mit Q_strcat (shared/qcommon/q_string.c:199)
//   B6b-4 ItemParse_cvar: minVal/maxVal/defVal -1 je nach Typ (OJ SP :4394, OJ MP :7956, MD SP :4311)
//   B6b-5 paketPfad: keine pk3 im Paket
#include "bhed/menue/menue.h"
#include "bhed/menue/projekt.h"
#include "bhed/menue/vorschau.h"

#include <cstdio>
#include <optional>
#include <string>
#include <vector>

using namespace bhed::menue;

namespace {
int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

Datei lesen(const std::string& t, Dialekt dl, Spiel sp) {
    LeseOptionen o;
    o.dialekt = dl;
    o.spiel = sp;
    return lies("ui/f6b.menu", t, o);
}

std::string menu(const std::string& items) { return "{ menuDef { name m rect 0 0 640 480 visible 1\n" + items + "\n} }\n"; }

bool meldet(const Datei& d, const std::string& teil) {
    for (const Meldung& m : d.meldungen) {
        if (m.text.find(teil) != std::string::npos) { return true; }
    }
    return false;
}

// eine Kopie: lesen() gibt eine temporaere Datei
std::optional<Item> erstes(const Datei& d) {
    if (d.menus.empty() || d.menus[0].items.empty()) { return std::nullopt; }
    return d.menus[0].items[0];
}

void b6b1() {
    std::printf("--- B6b-1: Movie Duels MP ---\n");
    {   // (a) ITEM_TYPE_MODEL_ITEM mit asset_model
        const std::string t = menu("itemDef { name a type 17 asset_model \"models/players/kyle/model.glm\" rect 0 0 64 64 visible 1 }");
        const Datei md = lesen(t, Dialekt::MP, Spiel::MD);
        const std::optional<Item> it = erstes(md);
        pruefe(md.menus.size() == 1 && md.menus[0].ok && it.has_value() && it->typeData && it->assetModel == "models/players/kyle/model.glm",
               "(a) MD MP: type 17 bekommt modelDef, asset_model geht");
        const Datei ja = lesen(t, Dialekt::MP, Spiel::JA);
        pruefe(ja.menus.size() == 1 && !ja.menus[0].ok, "(a) Gegenprobe OpenJK MP: type 17 ohne typeData, asset_model bricht ab");
    }
    {   // (b) ownerdrawID im Item und im Menue
        const Datei md = lesen("{ menuDef { name m ownerdrawID 3 rect 0 0 640 480 visible 1\n"
                               "itemDef { name a ownerdrawID 5 rect 0 0 64 64 visible 1 } } }\n",
                               Dialekt::MP, Spiel::MD);
        pruefe(md.menus.size() == 1 && md.menus[0].ok && md.menus[0].items.size() == 1 && meldet(md, "unknown menu item keyword ownerdrawID"),
               "(b) MD MP: ownerdrawID ist unbekannt, das Menue laedt");
        const Datei sp = lesen(menu("itemDef { name a ownerdrawID 5 rect 0 0 64 64 visible 1 }"), Dialekt::SP, Spiel::MD);
        pruefe(sp.menus.size() == 1 && !sp.menus[0].ok, "(b) Gegenprobe MD SP: ownerdrawID gibt qfalse");
    }
    {   // (c) exec: unbekannt, die erste } beendet das Item
        const std::string t = menu("itemDef { name a exec { setcvar x 1 } text \"t\" }");
        const Datei md = lesen(t, Dialekt::MP, Spiel::MD);
        const std::optional<Item> it = erstes(md);
        pruefe(it.has_value() && it->window.name == "a" && it->text.empty() && meldet(md, "unknown menu item keyword exec"),
               "(c) MD MP: exec unbekannt, Item endet an der } von exec");
        const std::optional<Item> sp = erstes(lesen(t, Dialekt::SP, Spiel::MD));
        pruefe(sp.has_value() && sp->text == "t", "(c) Gegenprobe MD SP: exec liest ein Skript");
    }
    {   // (d) cvarSubString
        const std::optional<Item> it = erstes(lesen(menu("itemDef { name a cvarTest \"c\" showCvar { \"x\" } cvarSubString rect 0 0 64 64 visible 1 }"),
                                      Dialekt::MP, Spiel::MD));
        pruefe(it.has_value() && (it->cvarFlags & CVAR_SUBSTRING) != 0, "(d) MD MP: cvarSubString setzt CVAR_SUBSTRING");
    }
    {   // (e) cvarInt: die Zahlen ohne Pruefung (fehlt eine, bleibt 0 und das Item geht weiter)
        const Datei md = lesen(menu("itemDef { name a type 15 cvarInt \"c\" 3 5 visible 1 rect 0 0 64 64 }"), Dialekt::MP, Spiel::MD);
        const std::optional<Item> it = erstes(md);
        pruefe(md.menus.size() == 1 && md.menus[0].ok && it.has_value() && it->defVal == 3 && it->minVal == 5 && it->maxVal == 0 &&
                   it->cvar == "c" && meldet(md, "expected integer but found visible"),
               "(e) MD MP: cvarInt mit zwei Zahlen - max 0, das Menue laedt");
    }
    {   // (f) PC_Script_Parse: "-204" bleibt im MD-MP-Skript eine Zahl
        const std::string t = menu("itemDef { name a action { transition2 character -204 114 } rect 0 0 64 64 visible 1 }");
        const std::optional<Item> md = erstes(lesen(t, Dialekt::MP, Spiel::MD));
        pruefe(md.has_value() && md->action == "\"transition2\" \"character\" \"-204\" \"114\" ", "(f) MD MP: \"-204\" zusammengesetzt");
        const std::optional<Item> ja = erstes(lesen(t, Dialekt::MP, Spiel::JA));
        pruefe(ja.has_value() && ja->action == "\"transition2\" \"character\" - \"204\" \"114\" ", "(f) Gegenprobe OpenJK MP: - und 204 getrennt");
    }
    const std::vector<std::string> w = itemSchluessel(Dialekt::MP, Spiel::MD);
    auto hat = [&w](const char* n) {
        for (const std::string& x : w) {
            if (x == n) { return true; }
        }
        return false;
    };
    pruefe(hat("cvarSubString") && hat("cvarInt") && hat("cvarRotateScale") && !hat("exec") && !hat("ownerdrawID"), "Liste MD MP");
}

void b6b2() {
    std::printf("--- B6b-2: cvarSubString ---\n");
    Umgebung u;
    std::string wert;
    u.cvar = [&wert](const std::string&) { return wert; };
    const std::string t = "itemDef { name a cvarTest \"ui_kraefte\" showCvar { \"push\" } cvarSubString rect 0 0 64 64 visible 1 }";
    const std::optional<Item> sp = erstes(lesen(menu(t), Dialekt::SP, Spiel::JA));
    u.dialekt = Dialekt::SP;
    wert = "pull push grip";
    pruefe(sp.has_value() && itemSichtbar(*sp, sp->window.flags, u), "SP: \"push\" ist ein Wort von \"pull push grip\" - sichtbar");
    wert = "pull pushgrip";
    pruefe(sp.has_value() && !itemSichtbar(*sp, sp->window.flags, u), "SP: Woerter ganz verglichen - \"pushgrip\" ist nicht \"push\"");
    const std::optional<Item> mp = erstes(lesen(menu(t), Dialekt::MP, Spiel::MD));
    u.dialekt = Dialekt::MP;
    wert = "pull pushgrip";
    pruefe(mp.has_value() && itemSichtbar(*mp, mp->window.flags, u), "MD MP: strstr findet \"push\" in \"pull pushgrip\"");
    wert = "pull grip";
    pruefe(mp.has_value() && !itemSichtbar(*mp, mp->window.flags, u), "MD MP: ohne \"push\" verborgen");
    // disableCvar im SP: der erste Wert der Liste wird in der cvar gesucht
    const std::optional<Item> ab = erstes(lesen(menu("itemDef { name a cvarTest \"c\" disableCvar { \"b\" ; \"x\" } cvarSubString rect 0 0 64 64 visible 1 }"),
                                  Dialekt::SP, Spiel::JA));
    u.dialekt = Dialekt::SP;
    wert = "a b c";
    pruefe(ab.has_value() && !itemZeigtUeberCvar(*ab, CVAR_ENABLE, u), "SP disableCvar: \"b\" in \"a b c\" - abgeschaltet");
    wert = "x";
    pruefe(ab.has_value() && itemZeigtUeberCvar(*ab, CVAR_ENABLE, u), "SP disableCvar: nur der erste Wert zaehlt - \"x\" schaltet nicht ab");
}

void b6b3() {
    std::printf("--- B6b-3: Skriptgrenze ---\n");
    // Jedes Wort "abcdefgh" kommt als "\"abcdefgh\" " (11 Zeichen); Q_strcat haengt nur an, was ganz passt.
    auto skript = [](int n) {
        std::string s;
        for (int i = 0; i < n; ++i) { s += " abcdefgh"; }
        return s + " yyyyyyyy z";
    };
    auto worte = [](int n) {
        std::string s;
        for (int i = 0; i < n; ++i) { s += "\"abcdefgh\" "; }
        return s;
    };
    {   // SP 4096: 372 Woerter = 4092, "yyyyyyyy" passt nicht (nur sein " "), "z" und " " passen = 4095
        const std::optional<Item> it = erstes(lesen(menu("itemDef { name a action {" + skript(372) + " } rect 0 0 64 64 visible 1 }"), Dialekt::SP, Spiel::JA));
        pruefe(it.has_value() && it->action == worte(372) + " z ", "SP: 4096-Puffer wie Q_strcat (Laenge " + std::to_string(it ? it->action.size() : 0) + ")");
    }
    {   // MP 2048: 186 Woerter = 2046, dann nur noch ein " " = 2047
        const std::optional<Item> it = erstes(lesen(menu("itemDef { name a action {" + skript(186) + " } rect 0 0 64 64 visible 1 }"), Dialekt::MP, Spiel::JA));
        pruefe(it.has_value() && it->action == worte(186) + " ", "MP: 2048-Puffer wie Q_strcat (Laenge " + std::to_string(it ? it->action.size() : 0) + ")");
    }
}

void b6b4() {
    std::printf("--- B6b-4: ItemParse_cvar ---\n");
    {
        const std::optional<Item> it = erstes(lesen(menu("itemDef { name a type ITEM_TYPE_SLIDER cvarfloat \"s_volume\" 0.5 0 1 cvar \"s_volume\" rect 0 0 64 64 visible 1 }"),
                                      Dialekt::SP, Spiel::JA));
        pruefe(it.has_value() && it->minVal == -1 && it->maxVal == -1 && it->defVal == -1, "SP: cvar nach cvarfloat setzt -1");
    }
    {
        const std::optional<Item> it = erstes(lesen(menu("itemDef { name a type ITEM_TYPE_TEXTSCROLL cvar \"c\" rect 0 0 64 64 visible 1 }"), Dialekt::SP, Spiel::JA));
        const std::optional<Item> mp = erstes(lesen(menu("itemDef { name a type 14 cvar \"c\" rect 0 0 64 64 visible 1 }"), Dialekt::MP, Spiel::JA));
        pruefe(it.has_value() && it->minVal == -1 && mp.has_value() && mp->minVal == 0, "TEXTSCROLL: SP -1, MP nicht");
    }
    {
        const std::optional<Item> mp = erstes(lesen(menu("itemDef { name a type 12 cvar \"c\" rect 0 0 64 64 visible 1 }"), Dialekt::MP, Spiel::JA));
        const std::optional<Item> lb = erstes(lesen(menu("itemDef { name a type 6 cvar \"c\" rect 0 0 64 64 visible 1 }"), Dialekt::MP, Spiel::JA));
        pruefe(mp.has_value() && mp->minVal == 0 && lb.has_value() && lb->minVal == 0, "MP: MULTI und LISTBOX bleiben 0");
    }
    {
        const std::optional<Item> it = erstes(lesen(menu("itemDef { name a type ITEM_TYPE_SLIDER_ROTATE cvarRotateScale \"r\" 90 rect 0 0 64 64 visible 1 }"),
                                      Dialekt::SP, Spiel::MD));
        const std::optional<Item> rc = erstes(lesen(menu("itemDef { name a type ITEM_TYPE_SLIDER_ROTATE cvarRotateScale \"r\" 90 cvar \"r\" rect 0 0 64 64 visible 1 }"),
                                      Dialekt::SP, Spiel::MD));
        pruefe(it.has_value() && it->range == 90 && it->maxVal == 0 && rc.has_value() && rc->range == 720 && rc->maxVal == -1 && rc->defVal == -1,
               "MD SP: cvarRotateScale setzt range, cvar danach range 720 und -1");
        const std::optional<Item> si = erstes(lesen(menu("itemDef { name a type 15 cvarInt \"i\" 1 0 9 cvar \"i\" rect 0 0 64 64 visible 1 }"), Dialekt::MP, Spiel::MD));
        pruefe(si.has_value() && si->minVal == -1 && si->maxVal == -1, "MD MP: INTSLIDER -1");
    }
}

void b6b5() {
    std::printf("--- B6b-5: pk3 im Projektordner ---\n");
    pruefe(!paketPfad("zz_menu.pk3") && !paketPfad("ui/Alt.PK3") && paketPfad("gfx/menus/knopf.tga") && paketPfad("ui/pk3.menu"),
           "paketPfad: keine pk3");
}
}  // namespace

int main() {
    b6b1();
    b6b2();
    b6b3();
    b6b4();
    b6b5();
    std::printf("menuef6b: %d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
