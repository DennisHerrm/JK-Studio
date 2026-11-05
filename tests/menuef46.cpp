// menuef46.cpp - rc610 F46: Funde der Codepruefung im Menue-Editor, gegen die Engine-Quellen
// (OpenJK code/ui/ui_shared.cpp = SP, codemp/ui/ui_shared.c = MP; Movie Duels Update 9 ebenso):
//   B6-4 Menu_HandleMouseMove: mouseEnter fuer jedes Element unter dem Zeiger (auch Dekorationen), der Fokus
//        dem ersten, im SP ohne WINDOW_INACTIVE; ein Klick trifft das Element mit dem Fokus (Menu_HandleKey)
//   B6-3 Item_SetTextExtents: Zugabe fuer zentrierte Eingabefelder (Wert der cvar) und ownerdraws (ownerDrawWidth);
//        Movie Duels SP teilt originalWidth / 2.0f
//   B6-2 Item_TextColor: SP ohne BLINK, SP-Ausnahme fuer autowrapped Text beim Fokus; der Puls
//        sin((float)(realTime / PULSE_DIVISOR)) ganzzahlig (OpenJK SP/MP, Movie Duels MP), Movie Duels SP in float
#include "bhed/menue/menue.h"
#include "bhed/menue/ownerdraw.h"
#include "bhed/menue/skript.h"
#include "bhed/menue/vorschau.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <map>
#include <string>
#include <utility>
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
bool nah(float a, float b) { return std::fabs(a - b) < 1e-3F; }

// Kuenstliche Schrift wie in menuevorschau.cpp: jedes Zeichen 10 breit, Bild 8x16, horizOffset 1, pointSize 20
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
    d.append(2, '\0');
    return d;
}

// Das erste Zeichen (Bild der Schrift) eines Items
const Prim* erstesZeichen(const std::vector<Prim>& p, int item) {
    for (const Prim& x : p) {
        if (x.item == item && x.art == PrimArt::Bild && x.shader == "fonts/probe") { return &x; }
    }
    return nullptr;
}

// SP liest Typen und Ausrichtungen beim Namen, MP (ohne menudef.h) als Zahl
Datei lesen(std::string t, Dialekt dl, Spiel sp) {
    if (dl == Dialekt::MP) {
        const std::vector<std::pair<std::string, std::string>> zahlen = {{"ITEM_TYPE_TEXT", "0"},    {"ITEM_TYPE_BUTTON", "1"},  {"ITEM_TYPE_EDITFIELD", "4"},
                                                                         {"ITEM_TYPE_OWNERDRAW", "8"}, {"ITEM_ALIGN_LEFT", "0"},  {"ITEM_ALIGN_CENTER", "1"},
                                                                         {"ITEM_ALIGN_RIGHT", "2"}};
        for (const auto& [n, z] : zahlen) {
            for (std::size_t p = t.find(n); p != std::string::npos; p = t.find(n, p)) { t.replace(p, n.size(), z); }
        }
    }
    LeseOptionen o;
    o.dialekt = dl;
    o.spiel = sp;
    return lies("ui/f46.menu", t, o);
}

void maus() {
    std::printf("--- B6-4: Menu_HandleMouseMove ---\n");
    const std::string t =
        "{ menuDef { name m rect 0 0 640 480 visible 1\n"
        "  itemDef { name a type ITEM_TYPE_BUTTON rect 10 10 100 40 visible 1 mouseEnter { setcvar inA 1 } mouseExit { setcvar outA 1 } action { setcvar klickA 1 } }\n"
        "  itemDef { name b type ITEM_TYPE_BUTTON rect 50 10 100 40 visible 1 mouseEnter { setcvar inB 1 } action { setcvar klickB 1 } }\n"
        "  itemDef { name deko rect 300 10 100 40 decoration visible 1 mouseEnter { setcvar inDeko 1 } }\n"
        "  itemDef { name aus type ITEM_TYPE_BUTTON rect 300 200 100 40 visible 1 flag WINDOW_INACTIVE mouseEnter { setcvar inAus 1 } }\n"
        "  itemDef { name c type ITEM_TYPE_BUTTON rect 500 400 100 40 visible 1 leaveFocus { setcvar lfC 1 } }\n"
        "} }\n";
    for (const Dialekt dl : {Dialekt::SP, Dialekt::MP}) {
        const std::string w = dl == Dialekt::SP ? "SP: " : "MP: ";
        const Datei d = lesen(t, dl, Spiel::JA);
        pruefe(d.meldungen.empty() && d.menus.size() == 1 && d.menus[0].items.size() == 5, w + "gelesen ohne Meldung");
        Laufzeit l;
        l.start(d);
        l.oeffne("m");
        const auto flags = [&](int i) { return l.items[0][static_cast<std::size_t>(i)].flags; };
        l.maus(0, 70, 20);   // ueber a und b
        pruefe(l.cvar("inA") == "1" && l.cvar("inB") == "1", w + "mouseEnter fuer beide Elemente unter dem Zeiger");
        pruefe((flags(0) & WINDOW_MOUSEOVER) != 0 && (flags(1) & WINDOW_MOUSEOVER) != 0, w + "beide WINDOW_MOUSEOVER");
        pruefe((flags(0) & WINDOW_HASFOCUS) != 0 && (flags(1) & WINDOW_HASFOCUS) == 0, w + "den Fokus bekommt das erste (a, Index 0)");
        pruefe(l.cvar("lfC") == (dl == Dialekt::MP ? "1" : ""), w + "Menu_ClearFocus: leaveFocus im MP fuer jedes Element, im SP nur fuer das mit Fokus");
        l.klick(0, 70, 20);
        pruefe(l.cvar("klickA") == "1" && l.cvar("klickB").empty(), w + "der Klick trifft das Element mit dem Fokus (Menu_HandleKey)");
        l.maus(0, 350, 20);   // ueber der Dekoration
        pruefe(l.cvar("inDeko") == "1" && (flags(2) & WINDOW_MOUSEOVER) != 0, w + "die Dekoration bekommt mouseEnter");
        pruefe(l.cvar("outA") == "1" && (flags(0) & WINDOW_MOUSEOVER) == 0 && (flags(1) & WINDOW_MOUSEOVER) == 0, w + "a und b: mouseExit, kein MOUSEOVER");
        pruefe((flags(0) & WINDOW_HASFOCUS) != 0 && (flags(2) & WINDOW_HASFOCUS) == 0,
               w + "Item_MouseLeave nimmt den Fokus nicht, die Dekoration nimmt keinen (a behaelt ihn)");
        l.maus(0, 350, 220);   // ueber dem inaktiven Knopf
        if (dl == Dialekt::SP) {
            pruefe(l.cvar("inAus").empty() && (flags(3) & (WINDOW_MOUSEOVER | WINDOW_HASFOCUS)) == 0, w + "WINDOW_INACTIVE: kein mouseEnter, kein Fokus");
        } else {
            pruefe(l.cvar("inAus") == "1" && (flags(3) & WINDOW_HASFOCUS) != 0, w + "WINDOW_INACTIVE zaehlt im MP nicht: mouseEnter und Fokus");
        }
    }
}

void textRect() {
    std::printf("--- B6-3: Item_SetTextExtents ---\n");
    Schrift f;
    pruefe(ladeFontdat("probe", fontdat(), f), "fontdat");
    Umgebung u;
    u.schrift = [&](int) -> const Schrift* { return &f; };
    std::map<std::string, std::string> cv{{"name", "Padawan"}};
    u.cvar = [&](const std::string& n) {
        const auto it = cv.find(n);
        return it == cv.end() ? std::string() : it->second;
    };
    u.ownerDrawBreite = [](int id, float) { return id == 257 ? 70 : 0; };
    // Eingabefeld zentriert mit cvar: originalWidth = B("Name:") 50 + B("Padawan") 70 = 120, x = 100 - 60
    for (const Dialekt dl : {Dialekt::SP, Dialekt::MP}) {
        const std::string w = dl == Dialekt::SP ? "SP: " : "MP: ";
        u.dialekt = dl;
        const Datei d = lesen("{ menuDef { name m rect 0 0 640 480 visible 1\n"
                              "  itemDef { name e type ITEM_TYPE_EDITFIELD cvar name text \"Name:\" textalign ITEM_ALIGN_CENTER textalignx 100 textscale 1 rect 0 0 300 30 visible 1 }\n"
                              "  itemDef { name o type ITEM_TYPE_OWNERDRAW ownerdraw 257 text \"ab\" textalign ITEM_ALIGN_RIGHT textalignx 100 textscale 1 rect 0 100 300 30 visible 1 }\n"
                              "  itemDef { name o2 type ITEM_TYPE_OWNERDRAW ownerdraw 257 text \"ab\" textalign ITEM_ALIGN_LEFT textalignx 100 textscale 1 rect 0 200 300 30 visible 1 }\n"
                              "} }\n",
                              dl, Spiel::JA);
        pruefe(d.meldungen.empty() && d.menus.size() == 1 && d.menus[0].items.size() == 3 && d.menus[0].items[0].textalignment == ITEM_ALIGN_CENTER,
               w + "Probe ohne Meldung gelesen");
        std::vector<Prim> p;
        zeichneMenu(d, 0, u, p);
        const Prim* e = erstesZeichen(p, 0);
        pruefe(e != nullptr && nah(e->x, 41.0F), w + "Eingabefeld zentriert: Text ab 100 - (50 + 70) / 2 = 40 (+1) (" + (e ? std::to_string(e->x) : "-") + ")");
        const Prim* o = erstesZeichen(p, 1);
        // DC->ownerDrawWidth (hier 70) zaehlt bei RIGHT in beiden Dialekten
        const float soll = 100.0F - 20.0F - 70.0F + 1.0F;
        pruefe(o != nullptr && nah(o->x, soll), w + "ownerdraw rechts: Text ab " + std::to_string(soll) + " (" + (o ? std::to_string(o->x) : "-") + ")");
        const Prim* o2 = erstesZeichen(p, 2);
        pruefe(o2 != nullptr && nah(o2->x, 101.0F), w + "ownerdraw links: ohne Zugabe");
    }
    // UI_OwnerDrawWidth: SP (code/ui/ui_main.cpp:3946) nur UI_KEYBINDSTATUS mit wartender Taste, MP z. B. UI_FORCE_SIDE
    // mit dem Text der Seite (codemp/ui/ui_main.c:2578 ff.): Text_Width(s, scale, 0)
    OdKontext k;
    k.breite = [](const std::string& t, float, int font, const std::string&) { return font == 0 ? static_cast<int>(t.size()) * 10 : -1; };
    k.se = [](const std::string& key) { return key == "MENUS_FORCEDESC_LIGHT" ? std::string("Licht") : key; };
    k.dialekt = Dialekt::SP;
    pruefe(ownerDrawBreite(257, 1.0F, k) == 0 && ownerDrawBreite(250, 1.0F, k) == 0, "SP: UI_OwnerDrawWidth 0");
    k.dialekt = Dialekt::MP;
    pruefe(ownerDrawBreite(257, 1.0F, k) == 50, "MP: UI_FORCE_SIDE - Breite von FORCEDESC_LIGHT (" + std::to_string(ownerDrawBreite(257, 1.0F, k)) + ")");
    pruefe(ownerDrawBreite(212, 1.0F, k) == 80, "MP: UI_BLUETEAM3 ohne Bot - \"3. Human\" (" + std::to_string(ownerDrawBreite(212, 1.0F, k)) + ")");
    pruefe(ownerDrawBreite(208, 1.0F, k) == 0 && ownerDrawBreite(250, 1.0F, k) == 0, "MP: Teamname (teamCount 0) und Tastenstatus ohne Text");
    pruefe(ownerDrawBreite(200, 1.0F, k) == 40, "MP: UI_HANDICAP mit handicap 100 (Vorgabe) - \"None\"");
    u.dialekt = Dialekt::SP;
    // Zentriert mit ungerader Breite: B("abc") bei 0,63 = 19. OpenJK und MD MP: 100 - 19 / 2 = 91, MD SP: 100 - 9,5 = 90,5
    const std::string z = "{ menuDef { name m rect 0 0 640 480 visible 1\n"
                          "  itemDef { name t type ITEM_TYPE_TEXT text \"abc\" textalign ITEM_ALIGN_CENTER textalignx 100 textscale 0.63 rect 0 0 300 30 visible 1 }\n"
                          "} }\n";
    std::vector<Prim> pj;
    std::vector<Prim> pm;
    zeichneMenu(lesen(z, Dialekt::SP, Spiel::JA), 0, u, pj);
    zeichneMenu(lesen(z, Dialekt::SP, Spiel::MD), 0, u, pm);
    const Prim* gj = erstesZeichen(pj, 0);
    const Prim* gm = erstesZeichen(pm, 0);
    pruefe(gj != nullptr && gm != nullptr && nah(gj->x - gm->x, 1.0F),
           "Movie Duels SP: originalWidth / 2.0f, der Text beginnt eine Spalte frueher (" + (gj && gm ? std::to_string(gj->x) + " / " + std::to_string(gm->x) : "-") + ")");
}

void textFarbe() {
    std::printf("--- B6-2: Item_TextColor ---\n");
    Schrift f;
    (void)ladeFontdat("probe", fontdat(), f);
    Umgebung u;
    u.schrift = [&](int) -> const Schrift* { return &f; };
    const std::string t = "{ menuDef { name m rect 0 0 640 480 visible 1 focuscolor 1 1 1 1\n"
                          "  itemDef { name blink type ITEM_TYPE_TEXT text \"ab\" textstyle 1 forecolor 0 1 0 1 rect 0 0 300 30 visible 1 }\n"
                          "  itemDef { name wrap type ITEM_TYPE_TEXT text \"ab\" autowrapped forecolor 0 1 0 1 rect 0 100 300 30 visible 1 }\n"
                          "  itemDef { name knopf type ITEM_TYPE_BUTTON text \"ab\" forecolor 0 1 0 1 rect 0 200 300 30 visible 1 }\n"
                          "} }\n";
    const auto farbe = [&](Dialekt dl, Spiel sp, int zeit, int item, int flags) {
        u.dialekt = dl;
        u.zeitMs = zeit;
        u.zustand.assign(3, ItemZustand{});
        u.zustand[static_cast<std::size_t>(item)].flags = WINDOW_VISIBLE | flags;
        u.zustand[static_cast<std::size_t>(item)].gueltig = true;
        std::vector<Prim> p;
        const Datei d = lesen(t, dl, sp);
        pruefe(d.meldungen.empty(), "Farbprobe ohne Meldung gelesen");
        zeichneMenu(d, 0, u, p);
        const Prim* g = erstesZeichen(p, item);
        return g != nullptr ? g->farbe : Farbe{-1, -1, -1, -1};
    };
    // zeit 0: (0 / BLINK_DIVISOR) & 1 == 0, Puls 0,5 -> 0,9 * forecolor im MP, forecolor im SP
    pruefe(nah(farbe(Dialekt::SP, Spiel::JA, 0, 0, 0).g, 1.0F), "SP (OpenJK): textstyle 1 ohne Wirkung, forecolor");
    pruefe(nah(farbe(Dialekt::SP, Spiel::MD, 0, 0, 0).g, 1.0F), "SP (Movie Duels): textstyle 1 ohne Wirkung, forecolor");
    pruefe(nah(farbe(Dialekt::MP, Spiel::JA, 0, 0, 0).g, 0.9F), "MP: textstyle 1 blinkt (0,9 * forecolor)");
    // autowrapped Text mit Fokus: SP forecolor, MP focusColor
    pruefe(nah(farbe(Dialekt::SP, Spiel::JA, 0, 1, WINDOW_HASFOCUS).r, 0.0F), "SP: autowrapped Text mit Fokus bleibt forecolor");
    pruefe(farbe(Dialekt::MP, Spiel::JA, 0, 1, WINDOW_HASFOCUS).r > 0.5F, "MP: autowrapped Text mit Fokus pulsiert in focusColor");
    // Puls bei realTime 100: ganzzahlig sin(1), Movie Duels SP sin(100 / 75.0f)
    const float ganz = 1.0F - 0.2F * (0.5F + 0.5F * std::sin(1.0F));
    const float flies = 1.0F - 0.2F * (0.5F + 0.5F * std::sin(100.0F / 75.0F));
    const float rj = farbe(Dialekt::SP, Spiel::JA, 100, 2, WINDOW_HASFOCUS).r;
    const float rm = farbe(Dialekt::SP, Spiel::MD, 100, 2, WINDOW_HASFOCUS).r;
    const float rmp = farbe(Dialekt::MP, Spiel::MD, 100, 2, WINDOW_HASFOCUS).r;
    pruefe(nah(rj, ganz), "OpenJK SP: Fokuspuls sin((float)(realTime / 75)) (" + std::to_string(rj) + " soll " + std::to_string(ganz) + ")");
    pruefe(nah(rm, flies), "Movie Duels SP: Fokuspuls sin(realTime / 75.0f) (" + std::to_string(rm) + " soll " + std::to_string(flies) + ")");
    pruefe(nah(rmp, ganz), "Movie Duels MP: Fokuspuls ganzzahlig (" + std::to_string(rmp) + ")");
    // Item_OwnerDraw_Paint blinkt auch im SP, aber mit BLINK_DIVISOR 600 (code/qcommon/q_shared.h), das MP mit 200:
    // bei realTime 300 ist (300 / 600) & 1 = 0 (blinkt), (300 / 200) & 1 = 1 (blinkt nicht)
    Farbe odFarbe{-1, -1, -1, -1};
    u.ownerDraw = [&](const OdAufruf& a) {
        odFarbe = a.farbe;
        return OdErgebnis{};
    };
    const std::string od = "{ menuDef { name m rect 0 0 640 480 visible 1\n"
                           "  itemDef { name od type ITEM_TYPE_OWNERDRAW ownerdraw 250 textstyle 1 forecolor 0 1 0 1 rect 0 0 300 30 visible 1 }\n"
                           "} }\n";
    const auto odBei = [&](Dialekt dl) {
        u.dialekt = dl;
        u.zeitMs = 300;
        u.zustand.clear();
        std::vector<Prim> p;
        zeichneMenu(lesen(od, dl, Spiel::JA), 0, u, p);
        return odFarbe.g;
    };
    const float blinkt = 1.0F - 0.2F * (0.5F + 0.5F * std::sin(4.0F));   // sin((float)(300 / 75))
    pruefe(nah(odBei(Dialekt::SP), blinkt), "SP: ownerdraw mit textstyle 1 blinkt bei 300 ms (BLINK_DIVISOR 600)");
    pruefe(nah(odBei(Dialekt::MP), 1.0F), "MP: ownerdraw bei 300 ms nicht (BLINK_DIVISOR 200)");
}
}  // namespace

int main() {
    maus();
    textRect();
    textFarbe();
    if (g_fehler == 0) { std::printf("menue_f46: alles gruen\n"); }
    return g_fehler == 0 ? 0 : 1;
}
