// menuevorschau.cpp - Menue-Editor M1 Punkt 2: die Layout-Rechnungen der
// Engine als Unit-Tests (Referenzwerte von Hand aus den OpenJK-Formeln:
// RE_Font_StrLenPixels, RE_Font_HeightPixels, RE_Font_DrawString,
// Item_SetTextExtents/ToWindowCoords, Window_Paint, Item_Slider_ThumbPosition,
// Item_EnableShowViaCvar, MenuFontToHandle, Abbildung auf das Fenster).
#include "bhed/menue/menue.h"
#include "bhed/menue/vorschau.h"

#include <cmath>
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
    return d;
}
}  // namespace

int main() {
    Schrift f;
    pruefe(ladeFontdat("probe", fontdat(), f), "fontdat 7178 Bytes gelesen");
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

    // Abbildung
    Abbildung ab = abbildung(1280, 720, Seitenverhaeltnis::Strecken);
    pruefe(nah(ab.sx, 2) && nah(ab.sy, 1.5F) && nah(ab.x(320), 640), "Strecken 1280x720");
    ab = abbildung(1280, 720, Seitenverhaeltnis::Balken);
    pruefe(nah(ab.sx, 1.5F) && nah(ab.ox, 160) && nah(ab.oy, 0) && nah(ab.x(640), 1120), "Balken 1280x720: 4:3 mittig");

    std::printf("menuevorschau: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
