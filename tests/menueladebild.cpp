// menueladebild.cpp - Menue-Editor M7 Punkt 11: Ladebildschirm (CG_DrawInformation) und
// Verbindungsbild (UI_DrawConnectScreen) mit Beispielwerten.
#include "bhed/menue/ladebild.h"

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
bool nah(float a, float b) { return std::fabs(a - b) < 0.01F; }

// kuenstliche Schrift wie in menuevorschau.cpp: jedes Zeichen 10 breit, pointSize 20
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
const Prim* finde(const std::vector<Prim>& p, const std::string& shader, std::size_t ab = 0) {
    for (std::size_t i = ab; i < p.size(); ++i) {
        if (p[i].shader == shader) { return &p[i]; }
    }
    return nullptr;
}
}  // namespace

int main() {
    Schrift f;
    (void)ladeFontdat("probe", fontdat(), f);
    Umgebung u;
    u.dialekt = Dialekt::SP;
    u.schrift = [&](int) -> const Schrift* { return &f; };
    u.schriftNamens = [&](const std::string&) -> const Schrift* { return &f; };
    // --- Ladebildschirm --------------------------------------------------------------------
    {
        LeseOptionen o;
        o.dialekt = Dialekt::SP;
        const Datei d = lies("ui/loadscreen.menu",
                             "{ menuDef { name loadscreen rect 0 0 640 480 visible 1\n"
                             " itemDef { name background rect 0 0 640 480 background gfx/menus/load forecolor 1 1 1 1 visible 1 decoration }\n"
                             " itemDef { name mappic rect 20 40 200 150 forecolor 1 1 1 0.5 visible 1 decoration }\n"
                             " itemDef { name weaponicons_singlerow rect 100 300 400 60 forecolor 1 1 1 1 visible 1 decoration }\n"
                             " itemDef { name forceicons_singlerow rect 100 370 400 40 forecolor 0.5 0.5 1 1 visible 1 decoration }\n"
                             "} }\n",
                             o);
        LadeKontext k;
        k.se = [](const std::string& key) { return key == "BRIEFINGS_T1_SOUR" ? std::string("Briefing") : std::string(); };
        k.bildDa = [](const std::string&) { return false; };
        k.waffenIcon = {"", "w_saber", "w_pistol", "w_blaster", "", "", "", "", "", "", "w_thermal", "", "", ""};
        LadeWerte w;
        w.nachricht = "Raven's Claw";
        std::vector<Prim> p;
        zeichneLadebild(d, 0, u, w, k, p);
        const Prim* hg = finde(p, "gfx/menus/load");
        const Prim* karte = finde(p, "menu/art/unknownmap");
        pruefe(hg != nullptr && karte != nullptr && nah(karte->x, 20) && nah(karte->w, 200) && nah(karte->farbe.a, 0.5F),
               "loadScreen background und mappic (ohne Levelshot menu/art/unknownmap, Farbe des Items)");
        // 4 Waffen: x = 100 + (400 - (4*60 + 3*12)) / 2 = 162, je 72 weiter
        const Prim* w1 = finde(p, "w_saber");
        const Prim* w4 = finde(p, "w_thermal");
        pruefe(w1 != nullptr && w4 != nullptr && nah(w1->x, 162) && nah(w1->y, 300) && nah(w1->w, 60) && nah(w4->x, 162 + 3 * 72),
               "CG_DrawLoadWeaponsPrintRow: mittig, 60 + 12");
        // Kraefte des Beispiels in showPowers: Tempo, Stoss, Ziehen, Sicht -> 4 * 40 + 3 * 12 = 196, x = 202
        const Prim* k1 = finde(p, "gfx/mp/f_icon_speed");
        const Prim* k4 = finde(p, "gfx/mp/f_icon_sight");
        pruefe(k1 != nullptr && k4 != nullptr && nah(k1->x, 202) && nah(k4->x, 202 + 3 * 52) && nah(k1->farbe.r, 0.5F),
               "CG_DrawLoadForcePrintRow: showPowers-Folge, 40 + 12");
        // CG_LoadBar: Breite 416, links 112, oben 443; Strich 5 * 40 ab 140
        const Prim* bar = finde(p, "gfx/hud/mp_levelload");
        const Prim* tick = finde(p, "gfx/hud/load_tick");
        pruefe(bar != nullptr && nah(bar->x, 112) && nah(bar->y, 443) && nah(bar->w, 416) && nah(bar->h, 32) && tick != nullptr && nah(tick->x, 140) &&
                   nah(tick->w, 200) && nah(tick->y, 455),
               "CG_LoadBar");
        // CS_MESSAGE in Anfuehrungszeichen bei (15, 20): erste Glyphe bei x 15 + horizOffset 1
        bool nachricht = false;
        for (const Prim& x : p) { nachricht = nachricht || (x.shader == "fonts/probe" && nah(x.x, 16) && x.item == -1); }
        pruefe(nachricht, "CS_MESSAGE bei (15, 20)");
        LadeWerte erste;
        erste.ersteKarte = true;
        std::vector<Prim> q;
        zeichneLadebild(d, 0, u, erste, k, q);
        pruefe(!q.empty() && q[0].art == PrimArt::Fuellen && nah(q[0].w, 640) && finde(q, "gfx/menus/load") == nullptr,
               "yavin1 ohne Spielstand: schwarz statt Menue");
    }
    // --- Verbindungsbild -------------------------------------------------------------------
    {
        LeseOptionen o;
        o.dialekt = Dialekt::MP;
        const Datei d = lies("ui/jamp/connect.menu", "{ menuDef { name Connect rect 0 0 640 480 visible 1 } }\n", o);
        Umgebung um = u;
        um.dialekt = Dialekt::MP;
        VerbindungsWerte w;
        std::vector<Prim> p;
        const auto se = [](const std::string& key) {
            if (key == "MENUS_LOADING_MAPNAME") { return std::string("Loading %s"); }
            if (key == "MENUS_CONNECTING_TO") { return std::string("Connecting to %s"); }
            if (key == "MENUS_AWAITING_CHALLENGE") { return std::string("Awaiting challenge...%i"); }
            return std::string();
        };
        zeichneVerbindung(d, 0, um, w, se, p);
        // "Loading mp/ffa1" = 15 Zeichen * 10 = 150 -> x = 320 - 75 = 245 (+ horizOffset 1), y 130; Schatten zuerst
        int zeilen = 0;
        float ersteX = -1;
        for (const Prim& x : p) {
            if (x.shader != "fonts/probe" || x.farbe.r < 0.9F) { continue; }   // ohne die grauen Schatten
            if (ersteX < 0) { ersteX = x.x; }
        }
        for (std::size_t i = 1; i < p.size(); ++i) { zeilen += (p[i].y > p[i - 1].y + 10.0F) ? 1 : 0; }
        pruefe(nah(ersteX, 246) && zeilen >= 2, "Text_PaintCenter: Karte mittig bei 130, weitere Zeilen darunter");
        w.server = "localhost";
        std::vector<Prim> q;
        zeichneVerbindung(d, 0, um, w, se, q);
        pruefe(q.size() < p.size(), "localhost: ohne Zustandszeile");
    }
    std::printf("menueladebild: %s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
