// menueskript.cpp - Menue-Editor M1 Punkt 5/6: Seiten, gefundene Cvars und
// die Bedienung wie im Spiel (Item_RunScript, open/close/show/hide/setfocus/
// setcvar, Menuestapel, onESC, Ja/Nein, Mehrfachwahl, Schieber).
#include "bhed/menue/skript.h"

#include <algorithm>
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
    const std::string t =
        "{\n"
        "menuDef { name main rect 0 0 640 480 visible 0\n"
        "  onOpen { setcvar ui_tab 1 ; show grpA ; hide grpB ; play \"sound/x.wav\" }\n"
        "  itemDef { name b1 type ITEM_TYPE_BUTTON rect 10 10 100 20 visible 1 action { open sub ; setcvar x 5 } }\n"
        "  itemDef { name a1 group grpA rect 10 40 100 20 visible 0 }\n"
        "  itemDef { name b2 group grpB rect 10 70 100 20 visible 1 }\n"
        "  itemDef { name t1 cvarTest ui_tab showCvar { \"1\" } rect 200 10 50 20 visible 1 }\n"
        "  itemDef { name t2 cvarTest ui_tab showCvar { \"2\" } rect 200 10 50 20 visible 1 }\n"
        "  itemDef { name yn type ITEM_TYPE_YESNO cvar r_yn rect 300 10 50 20 visible 1 }\n"
        "  itemDef { name mu type ITEM_TYPE_MULTI cvar r_mu cvarFloatList { \"a\" 0 \"b\" 1 \"c\" 2 } rect 300 40 50 20 visible 1 }\n"
        "  itemDef { name sl type ITEM_TYPE_SLIDER cvarFloat r_sl 0 0 10 rect 300 70 120 20 visible 1 }\n"
        "}\n"
        "menuDef { name sub rect 100 100 200 200 visible 0\n"
        "  onOpen { setfocus s1 }\n"
        "  onESC { close sub }\n"
        "  itemDef { name s1 type ITEM_TYPE_BUTTON rect 0 0 50 20 visible 1 onFocus { setcvar fokus ja } }\n"
        "}\n"
        "}\n";
    LeseOptionen o;
    o.dialekt = Dialekt::SP;
    const Datei d = lies("ui/s.menu", t, o);
    pruefe(d.menus.size() == 2 && d.menus[0].ok && d.menus[1].ok, "Datei gelesen");

    const auto seiten = seitenImMenue(d);
    pruefe(seiten.size() == 4 && seiten[1].cvar == "ui_tab" && seiten[1].wert == "1" && seiten[2].wert == "2",
           "Seiten: zwei menuDefs und zwei ui_tab-Unterseiten");
    const auto cv = cvarsImMenue(d);
    auto fund = [&](const std::string& n) {
        return std::find_if(cv.begin(), cv.end(), [&](const CvarFund& f) { return f.name == n; });
    };
    pruefe(fund("ui_tab") != cv.end() && fund("ui_tab")->sichtbarkeit && fund("ui_tab")->werte.size() == 2,
           "Cvars: ui_tab steuert die Sichtbarkeit, Werte 1 und 2");
    pruefe(fund("x") != cv.end() && fund("r_mu") != cv.end() && fund("r_mu")->werte.size() == 3, "Cvars: setcvar und cvarFloatList");

    Laufzeit l;
    l.start(d);
    l.oeffne("main");
    pruefe((l.menuFlags[0] & WINDOW_VISIBLE) != 0 && (l.menuFlags[0] & WINDOW_HASFOCUS) != 0, "open main");
    pruefe(l.cvar("ui_tab") == "1", "onOpen: setcvar");
    pruefe((l.items[0][1].flags & WINDOW_VISIBLE) != 0 && (l.items[0][2].flags & WINDOW_VISIBLE) == 0, "onOpen: show grpA, hide grpB");
    pruefe(!l.protokoll.empty() && l.protokoll.back() == "play sound/x.wav", "play wird gemeldet");
    Umgebung u;
    l.fuelle(0, u);
    pruefe(itemSichtbar(d.menus[0].items[3], l.items[0][3].flags, u) && !itemSichtbar(d.menus[0].items[4], l.items[0][4].flags, u),
           "ui_tab 1: t1 sichtbar, t2 nicht");

    l.klick(0, 20, 15);
    pruefe((l.menuFlags[1] & WINDOW_VISIBLE) != 0 && l.stapel.size() == 1 && l.stapel[0] == 0, "Klick auf b1: open sub, main auf dem Stapel");
    pruefe(l.cvar("x") == "5", "action: setcvar x 5");
    pruefe((l.items[1][0].flags & WINDOW_HASFOCUS) != 0 && l.cvar("fokus") == "ja", "sub onOpen: setfocus s1 mit onFocus");
    l.escape(1);
    pruefe((l.menuFlags[1] & WINDOW_VISIBLE) == 0 && (l.menuFlags[0] & WINDOW_HASFOCUS) != 0 && l.stapel.empty(),
           "onESC: close sub, main bekommt den Fokus zurueck");

    l.klick(0, 310, 15);
    pruefe(l.cvar("r_yn") == "1", "Ja/Nein schaltet auf 1");
    l.klick(0, 310, 15);
    pruefe(l.cvar("r_yn") == "0", "Ja/Nein schaltet auf 0");
    l.klick(0, 310, 45);
    pruefe(l.cvar("r_mu") == "1", "Mehrfachwahl: naechster Wert");
    l.klick(0, 300 + 48, 75);
    pruefe(l.cvar("r_sl") == "5", "Schieber: Mitte = 5");
    l.maus(0, 20, 15);
    pruefe((l.items[0][0].flags & WINDOW_MOUSEOVER) != 0, "Maus ueber b1: WINDOW_MOUSEOVER");
    l.maus(0, 600, 400);
    pruefe((l.items[0][0].flags & WINDOW_MOUSEOVER) == 0, "Maus weg: kein MOUSEOVER");
    l.schliesse("all");
    pruefe(l.menuFlags[0] == (d.menus[0].window.flags & ~(WINDOW_VISIBLE | WINDOW_HASFOCUS)), "close all");

    // --- M2: transition, orbit, fadeout im Takt der Engine (Item_Paint, Fade) ----------
    {
        LeseOptionen so;
        so.dialekt = Dialekt::SP;
        const Datei a = lies("ui/anim.menu",
                             "{ menuDef { name anim rect 0 0 640 480 visible 1 fadeAmount 0.25 fadeCycle 10 fadeClamp 1\n"
                             "  onOpen { transition t 0 0 10 10 100 0 10 10 20 10 ; orbit o 110 100 100 100 5 ; fadeout f }\n"
                             "  itemDef { name t rect 0 0 10 10 visible 1 }\n"
                             "  itemDef { name o rect 110 100 10 10 visible 1 }\n"
                             "  itemDef { name f text \"x\" rect 0 0 10 10 forecolor 1 1 1 1 visible 1 } } }\n",
                             so);
        Laufzeit la;
        la.start(a);
        la.spielFps = 100;   // 10 ms je Bild
        la.oeffne("anim");
        pruefe((la.items[0][0].flags & WINDOW_INTRANSITION) != 0, "transition: WINDOW_INTRANSITION gesetzt");
        pruefe(la.rects[0][0].x == 0.0F, "transition: beginnt bei rectFrom");
        (void)la.takt(1000);   // erstes Bild: rectEffects2.x = 100/10 = 10 je Schritt
        const float x1 = la.rects[0][0].x;
        (void)la.takt(1000 + 4 * 10);   // 4 weitere Bilder: Schritt erst bei realTime > nextTime (1030), also alle 30 ms
        const float x2 = la.rects[0][0].x;
        pruefe(x1 == 10.0F && x2 == 20.0F, "transition: 10 Punkte je Schritt, offsetTime 20 bei 10-ms-Bildern = alle 30 ms (" + std::to_string(x1) + ", " + std::to_string(x2) + ")");
        (void)la.takt(1000 + 300);
        pruefe(la.rects[0][0].x == 100.0F && (la.items[0][0].flags & WINDOW_INTRANSITION) == 0, "transition: am Ziel, Flag weg");
        pruefe((la.items[0][1].flags & WINDOW_ORBITING) != 0 && la.rects[0][1].y != 100.0F, "orbit: dreht um (100, 100)");
        pruefe((la.items[0][2].flags & WINDOW_VISIBLE) == 0 && (la.items[0][2].flags & WINDOW_FADINGOUT) == 0,
               "fadeout: nach 4 x 0.25 unsichtbar (Fade)");
        pruefe(la.vorderGesetzt[0][2] && la.vorder[0][2].a <= 0.0F, "fadeout: foreColor[3] auf 0");
    }

    std::printf("menueskript: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
