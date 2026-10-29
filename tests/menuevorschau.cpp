// menuevorschau.cpp - Menue-Editor M1 Punkt 2: die Layout-Rechnungen der
// Engine als Unit-Tests (Referenzwerte von Hand aus den OpenJK-Formeln:
// RE_Font_StrLenPixels, RE_Font_HeightPixels, RE_Font_DrawString,
// Item_SetTextExtents/ToWindowCoords, Window_Paint, Item_Slider_ThumbPosition,
// Item_EnableShowViaCvar, MenuFontToHandle, Abbildung auf das Fenster).
#include "bhed/menue/feeder.h"
#include "bhed/menue/hud.h"
#include "bhed/menue/menue.h"
#include "bhed/menue/modell.h"
#include "bhed/menue/vorschau.h"

#include <cctype>
#include <cmath>
#include <functional>
#include <cstdio>
#include <cstring>
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
bool nah(float a, float b) { return std::fabs(a - b) < 1e-3F; }

// Eine kuenstliche Schrift: jedes Zeichen 10 breit (Vorschub), Bild 8x16,
// baseline 14, horizOffset 1; pointSize 20, height 20, descender 4.
std::string fontdat() {
    std::string d;
    auto put = [&](const void* p, std::size_t n) { d.append(static_cast<const char*>(p), n); };
    for (int i = 0; i < 256; ++i) {
        const short w = 8, h = 16, adv = 10, off = 1;
        const int base = 14;
        const float s = static_cast<float>(i) / 256.0F, t = 0, s2 = s + 1.0F / 256.0F, t2 = 0.5F;
        put(&w, 2); put(&h, 2); put(&adv, 2); put(&off, 2); put(&base, 4);
        put(&s, 4); put(&t, 4); put(&s2, 4); put(&t2, 4);
    }
    const short ps = 20, hh = 20, asc = 16, desc = 4, kor = 0;
    put(&ps, 2); put(&hh, 2); put(&asc, 2); put(&desc, 2); put(&kor, 2);
    d.append(2, '\0');   // sizeof(dfontdat_t) = 7180
    return d;
}
}  // namespace

int main() {
    Schrift f;
    pruefe(ladeFontdat("probe", fontdat(), f), "fontdat 7180 Bytes gelesen");
    Schrift falsch;
    pruefe(!ladeFontdat("probe", fontdat() + "x", falsch), "fontdat mit falscher Groesse abgelehnt (wie CFontInfo)");
    pruefe(f.shader == "fonts/probe" && f.pointSize == 20, "Schrift: Shader und pointSize");
    pruefe(schriftBreite(f, "abc", 1.0F) == 30, "StrLenPixels abc = 30");
    pruefe(schriftBreite(f, "^1ab", 1.0F) == 20, "StrLenPixels: ^1 zaehlt nicht");
    pruefe(schriftBreite(f, "abc", 0.55F) == 17, "StrLenPixels: ceil(16.5) = 17");
    pruefe(schriftBreite(f, "abcd\nab", 1.0F) == 40, "StrLenPixels: laengste Zeile");
    pruefe(schriftHoehe(f, 0.55F) == 11, "HeightPixels: (int)(20 * 0.55)");

    Umgebung u;
    u.schrift = [&](int) -> const Schrift* { return &f; };
    std::map<std::string, std::string> cv{{"volume", "0.5"}, {"ui_x", "1"}};
    u.cvar = [&](const std::string& n) {
        const auto it = cv.find(n);
        return it == cv.end() ? std::string() : it->second;
    };

    // Item_SetTextExtents + ToWindowCoords
    Item it;
    it.window.rect = Rechteck{100, 50, 200, 40};
    it.textalignx = 20;
    it.textaligny = 5;
    it.textscale = 1.0F;
    it.textalignment = ITEM_ALIGN_CENTER;
    Rechteck r = textRechteck(it, "abcd", u);
    pruefe(nah(r.x, 100) && nah(r.y, 55) && nah(r.w, 40) && nah(r.h, 20), "textRect zentriert: 100,55 40x20");
    it.textalignment = ITEM_ALIGN_RIGHT;
    r = textRechteck(it, "abcd", u);
    pruefe(nah(r.x, 80), "textRect rechts: 80");
    it.window.border = 1;
    it.window.borderSize = 2;
    r = textRechteck(it, "abcd", u);
    pruefe(nah(r.x, 82) && nah(r.y, 57), "textRect mit border: +borderSize");

    // RE_Font_DrawString ueber eine Datei
    LeseOptionen o;
    o.dialekt = Dialekt::SP;
    const std::string text =
        "{ menuDef { name m rect 0 0 640 480 visible 1 focuscolor 1 1 1 1\n"
        " itemDef { name t type ITEM_TYPE_TEXT rect 100.7 50 200 40 textalignx 0 textaligny 0 textscale 1 text \"ab\" visible 1 }\n"
        " itemDef { name f style WINDOW_STYLE_FILLED border 1 bordersize 2 rect 10 10 100 50 backcolor 0 0 1 1 bordercolor 1 0 0 1 visible 1 }\n"
        " itemDef { name s type ITEM_TYPE_SLIDER rect 300 200 100 20 cvarFloat volume 0.5 0 1 visible 1 }\n"
        " itemDef { name z cvarTest ui_x showCvar { \"1\" } rect 0 0 10 10 style WINDOW_STYLE_FILLED visible 1 }\n"
        " itemDef { name zz cvarTest ui_x hideCvar { \"1\" } rect 0 0 10 10 style WINDOW_STYLE_FILLED visible 1 }\n"
        " itemDef { name unsichtbar rect 0 0 10 10 style WINDOW_STYLE_FILLED visible 0 }\n"
        "} }\n";
    Datei d = lies("ui/probe.menu", text, o);
    pruefe(d.menus.size() == 1 && d.menus[0].items.size() == 6, "Probe-Menue gelesen");
    u.dialekt = Dialekt::SP;
    std::vector<Prim> p;
    zeichneMenu(d, 0, u, p);
    // Text "ab": ox = (int)100.7 = 100; foy = 50 + (20 - (4>>1)) = 68; fy = 68 - 14 = 54; x = 100 + 1
    const Prim* g0 = nullptr;
    for (const Prim& x : p) {
        if (x.item == 0 && x.art == PrimArt::Bild) {
            g0 = &x;
            break;
        }
    }
    pruefe(g0 != nullptr && nah(g0->x, 101) && nah(g0->y, 54) && nah(g0->w, 8) && nah(g0->h, 16) && g0->shader == "fonts/probe",
           "DrawString: erstes Zeichen bei (101, 54) 8x16, (int)x abgeschnitten");
    pruefe(g0 != nullptr && nah(g0->s0, 97.0F / 256.0F), "DrawString: Glyph-Koordinaten s des 'a'");
    // Item_SetScreenCoords: border 1 -> rect 12,12; Window_Paint: fillRect(14, 14, 97, 47), dann drawRect
    int fuell = 0;
    bool innen = false;
    for (const Prim& x : p) {
        if (x.item == 1 && x.art == PrimArt::Fuellen) {
            ++fuell;
            if (nah(x.x, 14) && nah(x.y, 14) && nah(x.w, 97) && nah(x.h, 47) && nah(x.farbe.b, 1)) { innen = true; }
        }
    }
    pruefe(innen && fuell == 5, "Window_Paint: Fuellung (x+bs, y+bs, w-bs-1, h-bs-1) und vier Rahmenstreifen");
    // Slider: ohne Text x = rect.x = 300; Daumen bei 300 + 0.5 * 96 = 348 -> Bild bei 342, y - 2
    bool bar = false, thumb = false;
    for (const Prim& x : p) {
        if (x.item == 2 && x.shader == "menu/new/slider" && nah(x.x, 300) && nah(x.w, 96)) { bar = true; }
        if (x.item == 2 && x.shader == "menu/new/sliderthumb" && nah(x.x, 342) && nah(x.y, 198)) { thumb = true; }
    }
    pruefe(bar && thumb, "Slider: Leiste bei 300, Daumen bei 342/198");
    bool z = false, zz = false, unsicht = false;
    for (const Prim& x : p) {
        if (x.item == 3) { z = true; }
        if (x.item == 4) { zz = true; }
        if (x.item == 5) { unsicht = true; }
    }
    pruefe(z && !zz, "showCvar/hideCvar mit cvarTest ui_x = 1");
    pruefe(!unsicht, "visible 0 wird nicht gezeichnet");
    cv["ui_x"] = "0";
    p.clear();
    zeichneMenu(d, 0, u, p);
    z = zz = false;
    for (const Prim& x : p) {
        if (x.item == 3) { z = true; }
        if (x.item == 4) { zz = true; }
    }
    pruefe(!z && zz, "showCvar/hideCvar mit ui_x = 0");
    // M4: WINDOW_STYLE_CINEMATIC zeichnet nur die MP-Fassung von Window_Paint (codemp ui_shared.c);
    // OpenJK code/ui und Movie Duels (SP) kennen es nicht - ebenso die TEAMCOLOR-Flaeche
    {
        // Zahlen statt Namen (5 CINEMATIC, 4 TEAMCOLOR): MP-Menues kennen die Namen nur ueber menudef.h
        const std::string kt = "{ menuDef { name k rect 0 0 640 480 visible 1\n"
                               " itemDef { name kino style 5 cinematic \"probe\" rect 0 0 320 240 visible 1 }\n"
                               " itemDef { name team style 4 rect 320 0 100 100 visible 1 }\n"
                               "} }\n";
        for (int mp = 0; mp < 2; ++mp) {
            LeseOptionen ok;
            ok.dialekt = mp != 0 ? Dialekt::MP : Dialekt::SP;
            const Datei dk = lies("ui/kino.menu", kt, ok);
            Umgebung uk = u;
            uk.dialekt = ok.dialekt;
            std::vector<Prim> pk;
            zeichneMenu(dk, 0, uk, pk);
            bool kino = false, team = false;
            for (const Prim& x : pk) {
                kino = kino || (x.item == 0 && x.art == PrimArt::Kino);
                team = team || x.item == 1;
            }
            pruefe(mp != 0 ? (kino && team) : (!kino && !team),
                   mp != 0 ? "MP: cinematic und TEAMCOLOR gezeichnet (DC->drawCinematic, getTeamColor)"
                           : "SP: cinematic und TEAMCOLOR-Flaeche nicht gezeichnet (Window_Paint der SP-Fassung)");
        }
    }
    // M4: ITEM_TYPE_TEXTSCROLL wie Item_TextScroll_BuildLines/Item_TextScroll_Paint (Probeschrift:
    // Vorschub 10 je Zeichen; Breite 120 -> iBoxWidth 94: "aaa bbb cc" erreicht 100 -> Umbruch nach "bbb ")
    {
        const std::string tt = "{ menuDef { name ts rect 0 0 640 480 visible 1\n"
                               " itemDef { name scroll type ITEM_TYPE_TEXTSCROLL rect 10 10 120 20 lineHeight 16 textscale 1 text \"aaa bbb ccc ddd\" forecolor 1 1 1 1 visible 1 }\n"
                               " itemDef { name zeilen type ITEM_TYPE_TEXTSCROLL rect 10 100 120 200 lineHeight 16 textscale 1 text \"x\" cvar ui_desc forecolor 1 1 1 1 visible 1 }\n"
                               "} }\n";
        LeseOptionen ot;
        ot.dialekt = Dialekt::SP;
        const Datei dt = lies("ui/ts.menu", tt, ot);
        Umgebung ut = u;
        ut.dialekt = Dialekt::SP;
        const TextScrollZeilen zt = textScrollZeilen(dt.menus[0].items[0], ut);
        pruefe(zt.zeilen.size() == 2 && zt.zeilen[0] == "aaa bbb " && zt.zeilen[1] == "ccc ddd", "TextScroll: Umbruch nach Breite hinter dem Leerzeichen");
        cv["ui_desc"] = "eins\nzwei";
        const TextScrollZeilen zc = textScrollZeilen(dt.menus[0].items[1], ut);
        pruefe(zc.zeilen.size() == 2 && zc.zeilen[0] == "eins" && zc.zeilen[1] == "zwei", "TextScroll: Text aus der cvar (ersetzt text), \\n bricht um");
        std::vector<Prim> pt;
        zeichneMenu(dt, 0, ut, pt);
        bool platzhalter = false, leiste = false;
        int glyphen = 0;
        float x0 = 1e9F;
        for (const Prim& x : pt) {
            platzhalter = platzhalter || (x.item == 0 && x.art == PrimArt::Platzhalter);
            leiste = leiste || (x.item == 0 && x.shader == "gfx/menus/scrollbar_arrow_up_a.tga");
            if (x.item == 0 && x.shader == "fonts/probe") {
                ++glyphen;
                x0 = std::min(x0, x.x);
            }
        }
        pruefe(!platzhalter && glyphen >= 6 && nah(x0, 16), "TextScroll: Text statt Platzhalter, x = rect.x + textalignx + 1 + 4");
        pruefe(leiste, "TextScroll: Bildlaufleiste, wenn 2 Zeilen * 16 > Hoehe - 2");
        pruefe(glyphen <= 8, "TextScroll: nur eine Zeile passt (Rest 18 - 16 < lineHeight)");
        cv.erase("ui_desc");
    }
    // M5: fehlender @-Schluessel (MD twjMenu: @MD_MENU_MAIN_TWJ) - SE_GetString (OpenJK SP/MP, MD) und
    // JK2SP_GetStringTextString liefern "", Item_Text_Paint kehrt bei '\0' zurueck: nichts zu sehen
    {
        const std::string tf = "{ menuDef { name sf rect 0 0 640 480 visible 1\n"
                               " itemDef { name fehlt type 0 rect 10 10 200 20 textscale 1 text \"@MENUS_FEHLT\" forecolor 1 1 1 1 visible 1 }\n"
                               " itemDef { name da type 0 rect 10 50 200 20 textscale 1 text \"@MENUS_DA\" forecolor 1 1 1 1 visible 1 }\n"
                               " itemDef { name scroll type 14 rect 10 100 120 200 lineHeight 16 textscale 1 text \"@MENUS_FEHLT\" forecolor 1 1 1 1 visible 1 }\n"
                               "} }\n";
        Texte tx;
        tx.werte["MENUS_DA"] = "ok";
        for (const Dialekt dl : {Dialekt::SP, Dialekt::MP}) {
            LeseOptionen of;
            of.dialekt = dl;
            const Datei df = lies("ui/sf.menu", tf, of);
            Umgebung uf = u;
            uf.dialekt = dl;
            uf.texte = &tx;
            std::vector<Prim> pf;
            zeichneMenu(df, 0, uf, pf);
            int gl[3] = {0, 0, 0};
            for (const Prim& x : pf) {
                if (x.item >= 0 && x.item < 3 && x.art == PrimArt::Bild) { ++gl[x.item]; }
            }
            pruefe(gl[0] == 0 && gl[1] > 0 && gl[2] == 0 && textScrollText(df.menus[0].items[2], uf).empty(),
                   std::string(dl == Dialekt::SP ? "SP" : "MP") + ": fehlender @-Schluessel zeichnet nichts (SE_GetString liefert \"\"), bekannter schon");
        }
    }
    // M4: SP-Ownerdraws ohne laufendes Spiel (UI_OwnerDraw): 250 zeichnet nichts, 236 menu/art/unknownmap
    {
        const std::string to = "{ menuDef { name od rect 0 0 640 480 visible 1\n"
                               " itemDef { name taste type ITEM_TYPE_OWNERDRAW ownerdraw 250 rect 10 10 100 20 visible 1 }\n"
                               " itemDef { name stand type ITEM_TYPE_OWNERDRAW ownerdraw 236 rect 200 10 160 120 visible 1 }\n"
                               "} }\n";
        LeseOptionen oo;
        oo.dialekt = Dialekt::SP;
        const Datei dd = lies("ui/od.menu", to, oo);
        Umgebung uo = u;
        uo.dialekt = Dialekt::SP;
        std::vector<Prim> po;
        zeichneMenu(dd, 0, uo, po);
        bool taste = false, stand = false;
        for (const Prim& x : po) {
            taste = taste || x.item == 0;
            stand = stand || (x.item == 1 && x.art == PrimArt::Bild && x.shader == "menu/art/unknownmap" && nah(x.w, 160));
        }
        pruefe(!taste && stand, "Ownerdraw SP: UI_KEYBINDSTATUS ohne wartende Taste leer, UI_ALLMAPS_SELECTION = unknownmap");
    }
    u.alleItems = true;
    p.clear();
    zeichneMenu(d, 0, u, p);
    unsicht = false;
    for (const Prim& x : p) {
        if (x.item == 5 && x.farbe.a < 0.5F) { unsicht = true; }
    }
    pruefe(unsicht, "Editor 'alle Items': unsichtbare gedimmt");

    // MenuFontToHandle / Asset_Parse
    LeseOptionen mo;
    mo.dialekt = Dialekt::MP;
    Datei a = lies("ui/jamp/x.menu",
                   "assetGlobalDef { font \"ergoec\" 18 smallFont \"ocr_a\" 18 bigFont \"anewhope\" 20 small2Font \"arialnb\" 14 }", mo);
    const std::vector<std::string> mp = schriftNamen(a, Dialekt::MP);
    pruefe(mp.size() == 5 && mp[0] == "ergoec" && mp[1] == "ocr_a" && mp[2] == "ergoec" && mp[3] == "anewhope" && mp[4] == "arialnb",
           "MP: 0/2 font, 1 smallFont, 3 bigFont, 4 small2Font");
    Datei s = lies("ui/x.menu",
                   "assetGlobalDef { smallFont \"aurabesh\" 18 mediumFont \"ergoec\" 18 bigFont \"anewhope\" 20 smallFont \"arialnb\" 14 }", o);
    const std::vector<std::string> sp = schriftNamen(s, Dialekt::SP);
    pruefe(sp.size() == 5 && sp[0] == "ergoec" && sp[1] == "aurabesh" && sp[2] == "ergoec" && sp[3] == "anewhope" && sp[4] == "arialnb",
           "SP: Griffe in Anmeldereihenfolge, 0 = mediumFont");
    // M2: SP-Schriftnummern wie der Code (tr_font.cpp RE_RegisterFont, ui_atoms.cpp
    // UI_RegisterFont mit registeredFonts, ui_shared.cpp UI_MenuFontToReal):
    // eine fehlende Schrift belegt im Renderer trotzdem einen Griff, UI_RegisterFont
    // nimmt dann "ergoec". OpenJK bildet die Nummer ueber registeredFonts ab,
    // Movie Duels nimmt sie als Renderer-Griff.
    {
        Datei fs = lies("ui/x.menu", "assetGlobalDef { smallFont \"fehlt\" 16 mediumFont \"ergoec\" 18 bigFont \"anewhope\" 20 }", o);
        auto gibt = [](const std::string& n) { return n != "fehlt"; };
        const std::vector<std::string> ojk = schriftNamen(fs, Dialekt::SP, Spiel::JA, gibt);
        pruefe(ojk.size() == 3 && ojk[0] == "ergoec" && ojk[1] == "ergoec" && ojk[2] == "anewhope",
               "SP OpenJK: fehlende Schrift -> ergoec, registeredFonts ohne Doppelte");
        const std::vector<std::string> md = schriftNamen(fs, Dialekt::SP, Spiel::MD, gibt);
        pruefe(md.size() == 4 && md[0] == "ergoec" && md[1].empty() && md[2] == "ergoec" && md[3] == "anewhope",
               "SP Movie Duels: Renderer-Griffe, fehlende Schrift belegt Griff 1 (zeichnet nichts)");
        // OpenJK-Renderer: <name>_sharp1 .. _sharp8 belegen eigene Griffe
        Datei v = lies("ui/x.menu", "assetGlobalDef { mediumFont \"ergoec\" 18 bigFont \"anewhope\" 20 }", o);
        auto scharf = [](const std::string& n) { return n == "ergoec" || n == "ergoec_sharp1" || n == "anewhope"; };
        const std::vector<std::string> sv = schriftNamen(v, Dialekt::SP, Spiel::JA, scharf);
        pruefe(sv.size() == 3 && sv[1] == "ergoec" && sv[2] == "anewhope", "SP OpenJK: _sharp-Varianten verschieben die Nummern nicht");
        // menuDef font meldet nur an, wenn noch keine mediumFont angemeldet ist
        Datei m = lies("ui/x.menu", "assetGlobalDef { smallFont \"ocr_a\" 16 } menuDef { name a font \"anewhope\" }", o);
        const std::vector<std::string> mv = schriftNamen(m, Dialekt::SP, Spiel::JA, {});
        pruefe(mv.size() == 3 && mv[0] == "anewhope" && mv[1] == "ocr_a" && mv[2] == "anewhope",
               "SP: menuDef font wird mediumFont (MenuParse_font, fontRegistered)");
    }

    // M2: Item_ListBox_Paint mit Feeder-Daten (SP: Spalten, Zeile cursorPos in focusColor,
    // optionalImage statt Text; LISTBOX_IMAGE mit feederItemImage)
    {
        Datei lb = lies("ui/lb.menu",
                        "{ menuDef { name m rect 0 0 640 480 focusColor 1 0 0 1 visible 1\n"
                        "  itemDef { name l type ITEM_TYPE_LISTBOX rect 10 10 200 100 feeder 16 elementheight 20 elementwidth 20\n"
                        "    columns 2 0 10 20 100 10 20 forecolor 0 1 0 1 visible 1 }\n"
                        "  itemDef { name b type ITEM_TYPE_LISTBOX rect 300 10 30 100 feeder 17 elementheight 30 elementwidth 30\n"
                        "    elementtype 1 visible 1 } } }\n",   // SP: PC_ParseInt, keine Namen
                        o);
        Umgebung ul;
        ul.dialekt = Dialekt::SP;
        ul.schrift = [&](int) -> const Schrift* { return &f; };
        ul.feeder = [](float fe) {
            std::vector<FeederEintrag> v;
            for (int i = 0; i < 3; ++i) {
                FeederEintrag e;
                e.spalten = {"Zeile" + std::to_string(i), "B"};
                e.bild = fe == 17.0F ? "gfx/x" + std::to_string(i) : std::string();
                if (i == 2) { e.spaltenBild = {"", "gfx/icon"}; }
                v.push_back(e);
            }
            return v;
        };
        std::vector<Prim> pl;
        zeichneMenu(lb, 0, ul, pl);
        int texte = 0, rot = 0, icon = 0, bilder = 0;
        for (const Prim& x : pl) {
            if (x.item == 0 && x.art == PrimArt::Bild && x.shader == "fonts/probe") {
                ++texte;
                if (x.farbe.r > 0.9F && x.farbe.g < 0.1F) { ++rot; }
            }
            if (x.item == 0 && x.shader == "gfx/icon") { ++icon; }
            if (x.item == 1 && x.shader.compare(0, 5, "gfx/x") == 0) { ++bilder; }
        }
        pruefe(texte > 0 && rot > 0, "Listbox SP: Spaltentext, Zeile cursorPos in focusColor");
        pruefe(icon == 1, "Listbox SP: optionalImage der Spalte statt Text");
        pruefe(bilder == 3, "Listbox LISTBOX_IMAGE: feederItemImage je Zeile");
    }

    // M2: Mehrfachwahl mit "cvarStrList feeder" (Spezies in character.menu): der Text kommt
    // aus der Feeder-Liste wie ItemParse_cvarStrList
    {
        Datei md = lies("ui/c.menu",
                        "{ menuDef { name m rect 0 0 640 480 visible 1 itemDef { name species type ITEM_TYPE_MULTI rect 0 0 100 20\n"
                        "  feeder 16 cvar \"ui_char_model\" cvarStrList feeder visible 1 } } }\n",
                        o);
        Umgebung um;
        um.dialekt = Dialekt::SP;
        um.feeder = [](float) {
            std::vector<FeederEintrag> v(2);
            v[0].spalten = {"jedi_hf"};
            v[1].spalten = {"jedi_tf"};
            return v;
        };
        // nach uiScript getcharcvars: ui_char_model = g_char_model (Vorgabe von UI_Init)
        um.cvar = [](const std::string& n) { return spielCvarVorgabe(Dialekt::SP, Spiel::JA, n == "ui_char_model" ? "g_char_model" : n); };
        pruefe(!md.menus.empty() && !md.menus[0].items.empty() && multiText(md.menus[0].items[0], um) == "@MENUS_JEDI_TF",
               "Mehrfachwahl cvarStrList feeder: @MENUS_JEDI_TF (g_char_model jedi_tf)");
    }

    // M2: Item_Model_Paint - die Mitte der Grenzen liegt in der Bildmitte, die
    // Oberkante bei len / 0.268 / tan(fov_y / 2); das Licht am Augpunkt kommt von vorn
    // (R_SetupEntityLighting ohne Weltmodell: Umgebung 150 + 32).
    {
        Item mi;
        mi.window.rect = Rechteck{0, 0, 202, 402};
        const MenueModell leer;
        std::vector<bhed::BoneMatrix> welt;
        const MenueModellBild mb = menueModellBild(leer, mi, 0, 0, 200, 400, welt);
        auto projiziere = [&](float x, float y, float z, float& nx, float& ny) {
            const float* m = mb.viewProj;
            const float w = m[12] * x + m[13] * y + m[14] * z + m[15];
            nx = (m[0] * x + m[1] * y + m[2] * z + m[3]) / w;
            ny = (m[4] * x + m[5] * y + m[6] * z + m[7]) / w;
        };
        float nx = 0, ny = 0;
        projiziere(0, 0, 4, nx, ny);   // Mitte der Vorgabegrenzen -16 -16 -24 / 16 16 32
        pruefe(std::fabs(nx) < 1e-4F && std::fabs(ny) < 1e-4F, "Modell: Mitte der Grenzen in der Bildmitte");
        const float fovX = static_cast<float>(static_cast<int>(200.0F / 640.0F * 90.0F));
        const float fovY = std::atan2(400.0F, 200.0F / std::tan(fovX / 360.0F * 3.14159265F)) * (360.0F / 3.14159265F);
        projiziere(0, 0, 32, nx, ny);
        pruefe(std::fabs(ny - 0.268F / std::tan(fovY / 360.0F * 3.14159265F)) < 1e-3F, "Modell: Oberkante wie len / 0.268 und fov_y aus atan2");
        projiziere(0, 16, 4, nx, ny);   // +y (links) erscheint links im Bild
        pruefe(nx < 0, "Modell: Quake-Achsen (links = +y)");
        pruefe(std::fabs(mb.umgebung[0] - 182.0F / 255.0F) < 1e-4F && mb.richtung[0] < -0.9F, "Modell: Licht 500 am Augpunkt, Umgebung 182");
        // Saeber (isSaber, model_angle 180): "hack to put saber on it's side" - angles (curYaw, 0, 90);
        // die Hochachse des Griffs liegt waagrecht (-y), vorn zeigt nach -x
        Item sb = mi;
        sb.itemFlags = ITF_ISSABER;
        sb.angle = 180;
        const MenueModellBild ms = menueModellBild(leer, sb, 0, 0, 200, 400, welt);
        pruefe(std::fabs(ms.achsen[0][0] + 1) < 1e-4F && std::fabs(ms.achsen[2][1] + 1) < 1e-4F && std::fabs(ms.achsen[1][2] + 1) < 1e-4F,
               "Saeber: AnglesToAxis(180, 0, 90) - Griff liegt auf der Seite");
        Item fi = sb;   // isSaber mit isCharacter: normale Gier
        fi.itemFlags = ITF_ISSABER | ITF_ISCHARACTER;
        const MenueModellBild mf = menueModellBild(leer, fi, 0, 0, 200, 400, welt);
        pruefe(std::fabs(mf.achsen[2][2] - 1) < 1e-4F, "Saeber mit isCharacter: aufrecht (nur Gier)");
        // die Kamera ohne Entity bildet ent.origin dorthin ab, wo viewProj den Modellursprung hinlegt
        auto proj = [](const float* m, float x, float y, float z, float& px, float& py) {
            const float w = m[12] * x + m[13] * y + m[14] * z + m[15];
            px = (m[0] * x + m[1] * y + m[2] * z + m[3]) / w;
            py = (m[4] * x + m[5] * y + m[6] * z + m[7]) / w;
        };
        float ax1 = 0, ay1 = 0, ax2 = 0, ay2 = 0;
        proj(ms.viewProj, 0, 0, 0, ax1, ay1);
        proj(ms.kamera, ms.ursprung[0], ms.ursprung[1], ms.ursprung[2], ax2, ay2);
        pruefe(std::fabs(ax1 - ax2) < 1e-4F && std::fabs(ay1 - ay2) < 1e-4F, "Saeber: Kamera ohne Entity passt zu viewProj (Klingen in Weltkoordinaten)");
    }

    // Klingen wie UI_DoSaber / RB_SurfaceSaberGlow / RB_SurfaceLine (Vergleich mit dem Spiel:
    // vorher ein breites Band mit dem Glanzbild ueber die ganze Klinge)
    {
        const float org[3] = {100, 0, 0}, dir[3] = {0, -1, 0}, auge[3] = {0, 0, 0}, links[3] = {0, 1, 0}, oben[3] = {0, 0, 1};
        std::vector<UiKlingenViereck> v;
        uiSaberVierecke(org, dir, 40, uiKlingenMasseOpenJK(40, 40, 3), auge, links, oben, v);
        int glanz = 0;
        for (const UiKlingenViereck& q : v) { glanz += q.kern ? 0 : 1; }
        pruefe(v.size() >= 3 && v.back().kern && glanz == static_cast<int>(v.size()) - 1, "Klinge: Glanz-Sprites, dann ein Kern (RT_SABER_GLOW, RT_LINE)");
        // erstes Sprite an der Spitze, halbe Kante (3 - 7.5 %) = 2.775, Ecke 0 = +links +oben
        pruefe(v.size() >= 3 && nah(v[0].ecke[0][0], 100) && nah(v[0].ecke[0][1], -40 + 2.775F) && nah(v[0].ecke[0][2], 2.775F) &&
                   nah(v[0].ecke[2][1], -40 - 2.775F) && nah(v[0].ecke[2][2], -2.775F),
               "Klinge: erstes Glanz-Sprite an der Spitze, Radius radius - 7.5 %");
        // Radius waechst je Sprite um 0.017, der Schritt (radius * 0.65) nimmt schon den neuen
        pruefe(v.size() >= 3 && nah(v[1].ecke[0][1] - v[1].ecke[0][2], -40 + (2.775F + 0.017F) * 0.65F), "Klinge: naechstes Sprite radius * 0.65 weiter");
        pruefe(v.size() >= 3 && nah(v[1].ecke[0][2], 2.775F + 0.017F), "Klinge: Radius waechst je Sprite um 0.017");
        const UiKlingenViereck& griff = v[v.size() - 2];
        pruefe(nah(griff.ecke[0][1], 5.625F) && nah(griff.ecke[0][2], 5.625F), "Klinge: Griff-Klecks 5.5 + 0.25 / 2 am Ursprung");
        // Kern: Spitze bis 1 hinter die Wurzel, halbe Breite radius / 3, quer zu Auge und Klinge (hier z)
        const UiKlingenViereck& k = v.back();
        pruefe(nah(k.ecke[0][1], -40) && nah(k.ecke[1][1], -40) && nah(k.ecke[2][1], 1) && nah(k.ecke[3][1], 1) &&
                   nah(std::fabs(k.ecke[0][2] - k.ecke[1][2]), 2) && nah(k.ecke[0][0], 100),
               "Klinge: Kern von der Spitze bis 1 hinter die Wurzel, Breite 2 x radius / 3");
        v.clear();
        uiSaberVierecke(org, dir, 0.4F, uiKlingenMasseOpenJK(0.4F, 40, 3), auge, links, oben, v);
        pruefe(v.empty(), "Klinge: unter 0.5 nichts");
        v.clear();
        uiSaberVierecke(org, dir, 20, uiKlingenMasseOpenJK(20, 40, 3), auge, links, oben, v);
        pruefe(!v.empty() && nah(v[0].ecke[0][2], 2.775F * 1.1F), "Klinge: kuerzer als die volle Laenge - radiusmult 1 + 2 / Laenge");
        // Movie Duels: cg_SFXSabers 0 (Raven blade) und 4 (SFX, Vorgabe)
        const UiKlingenMasse m0 = uiKlingenMasseMD(0, 40, 40, 3, 1, 1);
        pruefe(m0.da && m0.glanz && nah(m0.glanzRadius, 3.6F) && nah(m0.glanzLaenge, 38.2F) && nah(m0.glanzStaerke, 0.8F) && nah(m0.kernRadius, 1.2F) &&
                   m0.kernMal == 1 && nah(m0.griffRadius, 2.5375F),
               "Klinge MD 0: Glanz radius * 1.6 * 0.75, Laenge - Glanz / 2, Staerke 0.8, Kern radius * 0.4, Griff 2.5 + 0.0375");
        const UiKlingenMasse m4 = uiKlingenMasseMD(4, 40, 40, 3, 1, 1);
        pruefe(m4.da && nah(m4.glanzRadius, 3.6F) && nah(m4.kernRadius, 1.38F) && m4.kernMal == 2, "Klinge MD 4 (SFX): Kern radius * 0.4 * 1.15, zweimal");
        v.clear();
        uiSaberVierecke(org, dir, 40, m4, auge, links, oben, v);
        pruefe(v.size() >= 4 && v.back().kern && v[v.size() - 2].kern && nah(v[0].ecke[0][1], -38.2F + 3.6F), "Klinge MD 4: Glanz ab laenge - Glanz / 2, Kern doppelt");
        // M3: Start-Cvars wie UI_Init/cvarTable (saber2 mit hideCvar { single staff } bleibt versteckt)
        pruefe(spielCvarVorgabe(Dialekt::SP, Spiel::MD, "g_saber_type") == "single" && spielCvarVorgabe(Dialekt::SP, Spiel::JA, "G_SABER_TYPE") == "single" &&
                   spielCvarVorgabe(Dialekt::SP, Spiel::MD, "ui_char_anim") == "BOTH_MENUIDLE1" && spielCvarVorgabe(Dialekt::SP, Spiel::JA, "g_char_model") == "jedi_tf" &&
                   spielCvarVorgabe(Dialekt::SP, Spiel::MD, "g_char_model") == "jedi_hm",
               "Start-Cvars: UI_Init und cvarTable je Spiel (OpenJK, Movie Duels)");
        // M3 Punkt 5: HUD wie CG_DrawHUD (Striche, Zahlenfelder, Saebelstil, Fadenkreuz)
        {
            LeseOptionen ho;
            ho.dialekt = Dialekt::SP;
            const Datei hud = lies("ui/hud.menu",
                                   "{ menuDef { name lefthud rect 0 380 100 100 visible 1\n"
                                   " itemDef { name frame rect 0 380 100 100 background \"gfx/hud/l\" forecolor 1 1 1 1 visible 1 }\n"
                                   " itemDef { name health_tic1 rect 10 400 8 8 background \"gfx/hud/h1\" forecolor 1 0 0 1 }\n"
                                   " itemDef { name health_tic2 rect 20 400 8 8 background \"gfx/hud/h2\" forecolor 1 0 0 1 }\n"
                                   " itemDef { name health_tic3 rect 30 400 8 8 background \"gfx/hud/h3\" forecolor 1 0 0 1 }\n"
                                   " itemDef { name health_tic4 rect 40 400 8 8 background \"gfx/hud/h4\" forecolor 1 0 0 0.8 }\n"
                                   " itemDef { name healthamount rect 50 440 6 8 forecolor 0 1 0 1 }\n"
                                   " itemDef { name armor_tic4 rect 40 420 8 8 background \"gfx/hud/a4\" forecolor 0 1 0 1 }\n"
                                   " itemDef { name armoramount rect 50 450 6 8 forecolor 0 1 0 1 } }\n"
                                   " menuDef { name righthud rect 540 380 100 100 visible 1\n"
                                   " itemDef { name saberstyle_fast rect 560 400 20 20 background \"gfx/hud/fast\" forecolor 1 1 1 1 }\n"
                                   " itemDef { name saberstyle_medium rect 560 400 20 20 background \"gfx/hud/medium\" forecolor 1 1 1 1 }\n"
                                   " itemDef { name ammoamount rect 580 440 6 8 forecolor 1 1 0 1 } } }\n",
                                   ho);
            pruefe(istHud(hud) && !istHud(d), "HUD: lefthud/righthud erkannt");
            HudWerte w;
            w.gesundheit = 70;   // inc 25: tic4 und tic3 voll, tic2 zu 0.8, tic1 nicht
            w.ruestung = 10;     // unter einem Viertel: der oberste Strich blinkt
            HudZustand hz;
            std::vector<Prim> hp;
            Umgebung hu;
            hu.dialekt = Dialekt::SP;
            zeichneHud(hud, 0, hu, w, hz, hp);
            auto zaehle = [&](const char* sh) {
                int n = 0;
                for (const Prim& q : hp) { n += q.shader == sh ? 1 : 0; }
                return n;
            };
            const Prim* h2 = nullptr;
            const Prim* h4 = nullptr;
            for (const Prim& q : hp) {
                if (q.shader == "gfx/hud/h2") { h2 = &q; }
                if (q.shader == "gfx/hud/h4") { h4 = &q; }
            }
            pruefe(h2 != nullptr && h4 != nullptr && zaehle("gfx/hud/h1") == 0 && nah(h4->farbe.r, 1) && nah(h4->farbe.g, 1) && nah(h2->farbe.g, 0) &&
                       nah(h2->farbe.a, 0.8F),
                   "HUD: volle Striche weiss, der angebrochene in der Itemfarbe mit Alpha * Anteil (CG_DrawHealth)");
            pruefe(zaehle("gfx/2d/numbers/t_seven") == 1 && zaehle("gfx/2d/numbers/t_zero") == 2 && zaehle("gfx/2d/numbers/t_one") == 1 && zaehle("gfx/hud/a4") == 1,
                   "HUD: Zahlenfelder 70 und 10, angebrochener oberster Ruestungsstrich");
            pruefe(zaehle("gfx/hud/medium") == 1 && zaehle("gfx/hud/fast") == 0 && zaehle("gfx/2d/crosshairb") == 1,
                   "HUD: Saebelstil mittel (SS_MEDIUM), Fadenkreuz b (cg_drawCrosshair 1)");
            pruefe(zaehle("gfx/hud/l") == 1, "HUD: frame ohne WINDOW_STYLE_SHADER nur von cgame gezeichnet");
            float fx = -1;
            for (const Prim& q : hp) {
                if (q.shader == "gfx/2d/crosshairb") { fx = q.x; }
                if (q.shader == "gfx/2d/numbers/t_seven" && fx < 0) { fx = -2; }
            }
            pruefe(nah(fx, 308), "HUD: Fadenkreuz in der Mitte (0.5 * (640 - 24)), nach dem HUD");
            // Blinken: nach dem Zeichnen schaltet cg.HUDArmorFlag um, sobald cg.time die Marke ueberholt
            w.zeitMs = 1;
            hp.clear();
            zeichneHud(hud, 0, hu, w, hz, hp);
            w.zeitMs = 2;
            hp.clear();
            zeichneHud(hud, 0, hu, w, hz, hp);
            pruefe(zaehle("gfx/hud/a4") == 0, "HUD: Ruestung unter einem Viertel blinkt (cg.HUDArmorFlag alle 400 ms)");
            w.saebel = false;
            w.munition = 0;
            hp.clear();
            zeichneHud(hud, 0, hu, w, hz, hp);
            const Prim* null0 = nullptr;
            for (const Prim& q : hp) {
                if (q.shader == "gfx/2d/numbers/t_zero" && q.x > 500) { null0 = &q; }
            }
            pruefe(null0 != nullptr && nah(null0->farbe.r, 1) && nah(null0->farbe.g, 0) && zaehle("gfx/hud/medium") == 0,
                   "HUD: Waffe statt Saebel - Munition 0 in Rot (CG_DrawAmmo)");
        }
        // M3 Punkt 2: UI_G2SetAnim + G2_TimingModel (20 fps -> frameLerp 50, animSpeed 1)
        {
            bhed::AnimEntry an;
            an.firstFrame = 10;
            an.numFrames = 5;
            an.loopFrame = 0;
            an.fps = 20;
            const G2Bild h0 = g2Bild(an, 0), h1 = g2Bild(an, 75), h2 = g2Bild(an, 230), h3 = g2Bild(an, 260);
            pruefe(h0.bild == 10 && h0.folge == 11 && nah(h0.anteil, 0) && h1.bild == 11 && h1.folge == 12 && nah(h1.anteil, 0.5F),
                   "Figur: G2-Takt ab dem Setzen, Anteil des Folgebildes");
            pruefe(h2.bild == 14 && h2.folge == 10 && nah(h2.anteil, 0.6F) && h3.bild == 10 && h3.folge == 11 && nah(h3.anteil, 0.2F) && !h3.aus,
                   "Figur: geschleift - vom letzten Bild zurueck zum ersten gemischt");
            an.loopFrame = -1;
            const G2Bild h4 = g2Bild(an, 260), h5 = g2Bild(an, 150);
            pruefe(h4.aus && h4.bild == 0 && !h5.aus && h5.bild == 13 && h5.folge == 14, "Figur: ohne Schleife zu Ende - G2 nimmt die Animation weg");
            an.fps = 30;   // frameLerp ceil(1000 / 30) = 34
            pruefe(nah(g2Bild(an, 34).anteil, 0) && g2Bild(an, 34).bild == 11, "Figur: frameLerp in ganzen Millisekunden (ceil)");
        }
        // M3 Punkt 6: alle Arten von cg_SFXSabers (UI_Do*Saber in Movie Duels ui_saber.cpp)
        const UiKlingenMasse m7 = uiKlingenMasseMD(7, 40, 40, 3, 1, 1);
        pruefe(m7.da && nah(m7.glanzRadius, 3.84F) && nah(m7.glanzLaenge, 38.08F) && nah(m7.kernRadius, 1.2F) && m7.kernMal == 2,
               "Klinge MD 7 (TFA): Glanz * 0.8, Kern ohne Faktor, zweimal");
        const UiKlingenMasse m5 = uiKlingenMasseMD(5, 40, 40, 3, 1, 1);
        pruefe(m5.da && nah(m5.glanzRadius, 5.85F) && nah(m5.kernRadius, 0.875F), "Klinge MD 5 (OT): flrand-Mitten, Kern * 0.5");
        const UiKlingenMasse m9 = uiKlingenMasseMD(9, 40, 40, 3, 1, 1);
        pruefe(m9.da && nah(m9.glanzRadius, 2.4F) && nah(m9.kernRadius, 1.02F) && nah(m9.glanzVersatz, 0.6F) && m9.kernMal == 1,
               "Klinge MD 9 (Rebels): Glanz * 0.5 um ein Viertel vorn, Kern * 0.85 einmal");
        const UiKlingenMasse m12 = uiKlingenMasseMD(kMdKlingeInstabil, 40, 40, 3, 2, 2);
        pruefe(m12.da && nah(m12.glanzRadius, 4.8F) && nah(m12.kernRadius, 1.2F) && m12.kernMal == 2,
               "Klinge MD instabil: ohne Groessen-Cvars, ohne Faktoren");
        pruefe(nah(uiKlingenMasseMD(1, 40, 40, 3, 1, 1).kernRadius, 1.68F) && nah(uiKlingenMasseMD(2, 40, 40, 3, 1, 1).kernRadius, 1.56F) &&
                   nah(uiKlingenMasseMD(3, 40, 40, 3, 1, 1).kernRadius, 1.02F) && nah(uiKlingenMasseMD(6, 40, 40, 3, 1, 1).kernRadius, 0.84F) &&
                   nah(uiKlingenMasseMD(10, 40, 40, 3, 1, 1).glanzRadius, 2.016F) && nah(uiKlingenMasseMD(11, 40, 40, 3, 1, 1).glanzRadius, 2.016F),
               "Klinge MD 1/2/3/6/10/11: Kernfaktoren 1.4/1.3/0.85/0.7, Glanz * 0.42");
        pruefe(!uiKlingenMasseMD(-1, 40, 40, 3, 1, 1).da, "Klinge MD: keine Art zeichnet nichts");
        pruefe(mdKlingenArt(4, "SABER_UNSTABLE") == kMdKlingeInstabil && mdKlingenArt(0, "SABER_SINGLE") == 0 && mdKlingenArt(-1, "SABER_SINGLE") == 0 &&
                   mdKlingenArt(12, "SABER_SINGLE") == -1 && mdKlingenArt(5, "SABER_SITH_SWORD") == -1 && mdKlingenArt(11, "saber_staff") == 11,
               "Klinge MD: Art aus cg_SFXSabers und Saeberart (UI_SaberDrawBlade)");
        pruefe(mdGroessenCvar(1, false) == "cg_SFXSabersGlowSizeEP1" && mdGroessenCvar(9, true) == "cg_SFXSabersCoreSizeRebels" &&
                   mdGroessenCvar(0, true) == "cg_SFXSabersCoreSize" && mdGroessenCvar(kMdKlingeInstabil, true).empty(),
               "Klinge MD: Groessen-Cvars je Art");
        const MdKlingenShader s1 = mdKlingenShader(1, "red");
        const MdKlingenShader s9 = mdKlingenShader(9, "rgb");
        const MdKlingenShader s4 = mdKlingenShader(4, "black");
        const MdKlingenShader s8 = mdKlingenShader(8, "red");
        const MdKlingenShader s0 = mdKlingenShader(0, "0.2 0.5 1");
        pruefe(s1.glanz == "gfx/effects/sabers_ep1/red_glow" && s1.kern == "gfx/effects/sabers_ep1/rgb_line" && s1.kern2.empty() && !s1.rgbWerte &&
                   s9.glanz == "gfx/effects/sabers_rebels/rgb_glow" && s9.kern2 == "gfx/effects/sabers_rebels/rgb_line" && s9.rgbWerte &&
                   s4.glanz == "gfx/effects/sabers/black_glow" && s4.kern == "SFX_Sabers/saber_blade_black" &&
                   s8.kern == "gfx/effects/sabers_tfa/red_unstable_line" && s0.rgbWerte && s0.glanz == "gfx/effects/sabers/rgb_glow" &&
                   s0.kern == "gfx/effects/sabers/rgb_line" && s0.kern2.empty(),
               "Klinge MD: Shader je Art und Farbe (TranslateSaberColor, switch (color))");
        v.clear();
        uiSaberVierecke(org, dir, 40, m9, auge, links, oben, v);
        pruefe(v.size() >= 3 && v.back().kern && !v[v.size() - 2].kern && nah(v[0].ecke[0][1], -(0.6F + 38.8F) + 2.4F),
               "Klinge MD 9: Glanz ab Versatz + Laenge, Kern einmal");
        pruefe(!uiKlingeZeichnen(2, 2, false, true) && uiKlingeZeichnen(1, 2, false, true) && !uiKlingeZeichnen(0, 0, true, false),
               "Klinge: noBlade bzw. noBlade2 ab bladeStyle2Start (UI_SaberShouldDrawBlade)");
        // tagHack ohne Bolzen: Einheitsmatrix, Versaetze mal xscale
        const bhed::GlmModel ohne;
        float ko[3], kr[3];
        pruefe(uiKlingenStrecke(ohne, "SABER_SINGLE", 0, 2, ko, kr) && nah(ko[0], 0) && nah(kr[0], -1), "Klinge: ohne Bolzen Einheitsmatrix (NEGATIVE_X)");
        pruefe(uiKlingenStrecke(ohne, "saber_staff", 1, 2, ko, kr) && nah(ko[0], 32) && nah(kr[0], 1), "Klinge: Stab, Klinge 2 umgedreht, 16 x xscale weiter");
        pruefe(uiKlingenStrecke(ohne, "SABER_TRIDENT", 3, 1, ko, kr) && nah(ko[0], 32) && nah(kr[0], 1), "Klinge: Dreizack, Klinge 4");
        pruefe(uiKlingenStrecke(ohne, "SABER_PRONG", 1, 2, ko, kr) && nah(ko[1], -6), "Klinge: Zinke, 3 x xscale quer");
        pruefe(!uiKlingenStrecke(ohne, "SABER_SITH_SWORD", 0, 1, ko, kr), "Klinge: Sith-Schwert ohne Klinge");
    }

    // Abbildung
    Abbildung ab = abbildung(1280, 720, Seitenverhaeltnis::Strecken);
    pruefe(nah(ab.sx, 2) && nah(ab.sy, 1.5F) && nah(ab.x(320), 640), "Strecken 1280x720");
    ab = abbildung(1280, 720, Seitenverhaeltnis::Balken);
    pruefe(nah(ab.sx, 1.5F) && nah(ab.ox, 160) && nah(ab.oy, 0) && nah(ab.x(640), 1120), "Balken 1280x720: 4:3 mittig");

    // M5: Spezies der Figurenlisten aus ui_char_model (UI_GetCharacterCvars) - vorher immer die erste
    // (Original-JKA character.menu: Farben/Rumpf von models/players/<erste> statt jedi_tf)
    {
        FeederDaten fd;
        fd.gebaut = true;
        for (const char* n : {"humanf", "jedi_tf", "Jedi_TF"}) {
            Spezies sz;
            sz.name = n;
            sz.farbShader.push_back(std::string("farbe_") + n);
            sz.farbAktion.push_back("x");
            fd.spezies.push_back(sz);
        }
        fd.dialekt = Dialekt::SP;
        pruefe(fd.speziesIndex("jedi_tf") == 2 && fd.speziesIndex("gibtsnicht") == 0, "Spezies SP: letzter passender Ordner (ohne break)");
        fd.dialekt = Dialekt::MP;
        pruefe(fd.speziesIndex("JEDI_TF") == 1, "Spezies MP: erster passender Ordner (break)");
        const auto farben = fd.eintraege(static_cast<float>(feeder_mp::COLORCHOICES), fd.speziesIndex("jedi_tf"));
        pruefe(!farben.empty() && farben[0].bild == "farbe_jedi_tf", "Farbliste der gewaehlten Spezies");
    }
    // M5: MP-Ownerdraws wie UI_OwnerDraw (Original-JKA createserver/quickgame2/playerMenu zeigten Platzhalter)
    {
        FeederDaten fd;
        fd.gebaut = true;
        fd.dialekt = Dialekt::MP;
        fd.spieltypen = {{"Free For All", 0}, {"Duel", 3}};
        fd.bots = {"Kyle", "Jan"};
        fd.kartenDatei = {"mp/ffa1"};
        std::map<std::string, std::string> cvm{{"ui_netgametype", "1"}, {"sv_maxclients", "8"}, {"ui_blueteam1", "1"}, {"ui_blueteam2", "3"},
                                               {"ui_redteam5", "1"}, {"g_spskill", "2"}};
        const std::function<std::string(const std::string&)> cvf = [&](const std::string& n) {
            std::string k = n;
            for (char& ch : k) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
            const auto it = cvm.find(k);
            return it == cvm.end() ? std::string() : it->second;
        };
        pruefe(fd.ownerDraw(245, cvf).text == "@MENUS_DUEL", "UI_NETGAMETYPE: gameTypes[ui_netGametype].gtEnum -> UI_GetGameTypeName");
        pruefe(fd.ownerDraw(210, cvf).text == "@MENUS_HUMAN" && !fd.ownerDraw(210, cvf).halb, "UI_BLUETEAM1: Wert 1 = Mensch");
        pruefe(fd.ownerDraw(211, cvf).text == "Jan", "UI_BLUETEAM2: Wert 3 = Bot Nr. 1");
        const OwnerDrawInhalt zu = fd.ownerDraw(219, cvf);
        pruefe(zu.text == "@MENUS_CLOSED" && zu.halb, "UI_REDTEAM5: Platz 10 > sv_maxClients 8 -> geschlossen, halbe Farbe");
        pruefe(fd.ownerDraw(207, cvf).text == "@MP_INGAME_SKILL2" && fd.ownerDraw(287, cvf).text == "@MENUS_DEFAULT", "UI_SKILL, UI_SKIN_COLOR");
        pruefe(fd.ownerDraw(255, cvf).bild == "menu/art/unknownmap_mp" && !fd.ownerDraw(9999, cvf).bekannt,
               "UI_STARTMAPCINEMATIC ohne Levelshot -> unknownmap_mp; unbekannte Nummer bleibt Platzhalter");
        fd.dialekt = Dialekt::SP;
        pruefe(!fd.ownerDraw(245, cvf).bekannt, "SP: MP-Ownerdraws gelten nicht");
        // Bildliste der MP-Fassung: mehr als doppelt so breit wie ein Element -> Zeilen und Spalten
        LeseOptionen og;
        og.dialekt = Dialekt::MP;
        const Datei dg = lies("ui/jamp/g.menu",
                              "{ menuDef { name g rect 0 0 640 480 visible 1\n"
                              " itemDef { name l type 6 rect 0 0 400 200 elementwidth 64 elementheight 64 elementtype 1 feeder 12 visible 1 } } }\n",
                              og);
        Umgebung ug = u;
        ug.dialekt = Dialekt::MP;
        ug.feeder = [](float) {
            std::vector<FeederEintrag> v(20);
            for (FeederEintrag& e : v) { e.bild = "kopf"; }
            return v;
        };
        std::vector<Prim> pg;
        zeichneMenu(dg, 0, ug, pg);
        int koepfe = 0;
        bool zweiteZeile = false;
        for (const Prim& x : pg) {
            if (x.shader != "kopf") { continue; }
            ++koepfe;
            zweiteZeile = zweiteZeile || (std::fabs(x.x - 2) < 0.01F && std::fabs(x.y - 66) < 0.01F);
        }
        pruefe(koepfe == 18 && zweiteZeile, "MP-Bildliste: 6 je Zeile, 3 Zeilen (" + std::to_string(koepfe) + ")");
    }
    std::printf("menuevorschau: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
