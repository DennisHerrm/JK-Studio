// kartecwimport.cpp - Karten-Editor, CW-IMPORT (src/karte/cwimport.cpp): die Testfigur (Kasten der Engine, Marken), Massstab und
// Achsen beim Import, Massstab je Raum, Treppe zu Clip-Rampe, glatte Clip-Wand, Ebene Collision.
#include "bhed/karte/cwimport.h"
#include "bhed/karte/cwwerkzeuge.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/optimierer.h"
#include "bhed/karte/pinsel.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
bool nah(double a, double b, double e = 1e-4) { return std::fabs(a - b) <= e; }
bool nahV(const Vektor3f& a, double x, double y, double z) { return nah(a.x(), x) && nah(a.y(), y) && nah(a.z(), z); }

bool hatText(const FigurZeichnung& z, const std::string& t) {
    for (const FigurText& x : z.texte) {
        if (x.text == t) { return true; }
    }
    return false;
}
// Die groesste Hoehe der Linien einer Art (relativ zum Ursprung) und die groesste waagerechte Ausdehnung in y bzw. x
void ausdehnung(const FigurZeichnung& z, FigurArt art, const Vektor3f& o, double& hoch, double& y, double& x) {
    hoch = -1e9;
    y = 0;
    x = 0;
    for (const FigurLinie& l : z.linien) {
        if (l.art != art) { continue; }
        for (const Vektor3f& p : {l.a, l.b}) {
            hoch = std::max(hoch, static_cast<double>(p.z() - o.z()));
            y = std::max(y, std::fabs(static_cast<double>(p.y() - o.y())));
            x = std::max(x, std::fabs(static_cast<double>(p.x() - o.x())));
        }
    }
}

void testeTestfigur() {
    const CwMasse m;   // Vorgaben aus CW-PRUEF: Tuer 110 x 64, Gang 128 x 96
    FigurLage l;
    l.ursprung = Vektor3f(100, 200, 24);
    {
        const FigurZeichnung z = testfigurZeichnung(l, spielfigur(GrenzSpiel::MovieDuels), m);
        pruefe(nahV(z.mins, 84, 184, 0) && nahV(z.maxs, 116, 216, 64), "Movie Duels: Kasten -16..16 x -24..40 um den Ursprung (g_client.cpp:94-96, bg_public.h:76-77)");
        pruefe(hatText(z, "64"), "Hoehe 64 beschriftet");
        double hoch = 0, y = 0, x = 0;
        ausdehnung(z, FigurArt::Hoehe, l.ursprung, hoch, y, x);
        pruefe(nah(hoch, 16) && hatText(z, "40"), "stehend: Marke der Oberkante geduckt 16 (CROUCH_MAXS_2), 40 ueber den Fuessen");
        ausdehnung(z, FigurArt::Stufe, l.ursprung, hoch, y, x);
        pruefe(nah(hoch, -24 + 18) && hatText(z, "STEPSIZE 18"), "STEPSIZE 18 ueber den Fuessen (bg_public.h:93)");
        ausdehnung(z, FigurArt::Tuer, l.ursprung, hoch, y, x);
        pruefe(nah(hoch, -24 + 110) && nah(y, 32) && nah(x, 0) && hatText(z, "110 x 64"), "Tuerrahmen 110 x 64 um die Figur, Ebene x = Ursprung");
        ausdehnung(z, FigurArt::Gang, l.ursprung, hoch, y, x);
        pruefe(nah(hoch, -24 + 128) && nah(y, 48) && hatText(z, "128 x 96"), "Gangrahmen 128 x 96");
    }
    {
        FigurLage g = l;
        g.geduckt = true;
        g.quer = true;
        const FigurZeichnung z = testfigurZeichnung(g, spielfigur(GrenzSpiel::JkaSp), m);
        pruefe(nahV(z.maxs, 116, 216, 24 + 16) && hatText(z, "40"), "geduckt: Oberkante 16 (bg_public.h:53), 40 hoch");
        double hoch = 0, y = 0, x = 0;
        ausdehnung(z, FigurArt::Hoehe, g.ursprung, hoch, y, x);
        pruefe(nah(hoch, 40), "geduckt: Marke der Oberkante stehend 40 (Platz zum Aufstehen)");
        ausdehnung(z, FigurArt::Tuer, g.ursprung, hoch, y, x);
        pruefe(nah(x, 32) && nah(y, 0), "quer: Tuerrahmen in der Ebene y = Ursprung");
    }
    {
        const FigurZeichnung z = testfigurZeichnung(l, spielfigur(GrenzSpiel::JkaMp), m);
        pruefe(nah(z.maxs.x() - z.mins.x(), 30), "Mehrspieler: 30 breit (codemp/game/g_client.c:31)");
    }
    {
        FigurLage o = l;
        o.marken = false;
        const FigurZeichnung z = testfigurZeichnung(o, spielfigur(GrenzSpiel::JkaSp), m);
        bool nurKasten = true;
        for (const FigurLinie& x : z.linien) { nurKasten = nurKasten && (x.art == FigurArt::Kasten || x.art == FigurArt::Ursprung); }
        pruefe(nurKasten && z.linien.size() == 12 + 3, "ohne Marken: 12 Kanten und das Ursprungskreuz");
    }
    {
        CwMasse eigen;
        eigen.tuerHoehe = 96;
        eigen.tuerBreite = 48;
        const FigurZeichnung z = testfigurZeichnung(l, spielfigur(GrenzSpiel::JkaSp), eigen);
        pruefe(hatText(z, "96 x 48"), "Tuermass aus den Einstellungen des Optimierers");
    }
    pruefe(nahV(testfigurAufBoden(Vektor3f(1, 2, 3), spielfigur(GrenzSpiel::JkaSp)), 1, 2, 27), "auf dem Boden: Ursprung 24 ueber den Fuessen");
    pruefe(std::string(testfigurSequenz(false)) == "BOTH_STAND1" && std::string(testfigurSequenz(true)) == "BOTH_CROUCH1",
           "Pose wie PM_Footsteps ohne Bewegung: BOTH_STAND1, geduckt BOTH_CROUCH1");
}


// --- Karten fuer die Werkzeuge ---------------------------------------------------------------------------------------------------------
std::string kastenText(double x0, double y0, double z0, double x1, double y1, double z1, const std::string& sh, const std::string& unten = "") {
    char b[2048];
    const std::string u = unten.empty() ? sh : unten;
    std::snprintf(b, sizeof(b),
                  "{\n( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n}\n",
                  x0, y0, z0, x0, y1, z0, x0, y0, z1, sh.c_str(),   // -x
                  x1, y0, z0, x1, y0, z1, x1, y1, z0, sh.c_str(),   // +x
                  x0, y0, z0, x0, y0, z1, x1, y0, z0, sh.c_str(),   // -y
                  x0, y1, z0, x1, y1, z0, x0, y1, z1, sh.c_str(),   // +y
                  x0, y0, z0, x1, y0, z0, x0, y1, z0, u.c_str(),    // -z
                  x0, y0, z1, x0, y1, z1, x1, y0, z1, sh.c_str());  // +z
    return b;
}

// Boden 512 x 512 (Oberkante 0), eine Treppe mit 4 Stufen (Hoehe 16, Tiefe 24) in +x ab x 0, Breite y -64 ... 64; stufenShader fuer
// die Stufen (Q3-Format ohne "textures/"), danach oben ein Podest x 96 ... 192 in 64
std::string treppenKarte(const std::string& stufenShader, const std::string& stufenUnten = "") {
    std::string t = "{\n\"classname\" \"worldspawn\"\n" + kastenText(-256, -256, -16, 256, 256, 0, "test/fest");
    for (int i = 1; i <= 4; ++i) { t += kastenText(24.0 * (i - 1), -64, 0, 24.0 * i, 64, 16.0 * i, stufenShader, stufenUnten); }
    t += kastenText(96, -64, 0, 192, 64, 64, "test/fest");
    return t + "}\n{\n\"classname\" \"info_player_start\"\n\"origin\" \"-200 0 24\"\n}\n";
}

// die Stufen (Pinsel 1 bis 4 des worldspawn) waehlen
void stufenWaehlen(Karte& k) {
    k.auswahl.leeren();
    const auto welt = k.welt();
    for (std::size_t i = 1; i <= 4 && i < welt->kinder().size(); ++i) { k.auswahl.waehlen(welt, welt->kinder()[i]); }
}

const std::vector<std::string> kSichtKollision = {"laufohnekollision", "kollisionsabstand", "unsichtbarekollision"};

long long sichtKollision(const Karte& k, std::string* text = nullptr) {
    PruefOptionen o;
    o.spiel = GrenzSpiel::JkaSp;
    o.clonewars = true;
    ShaderArt luft;   // nicht fest, gezeichnet
    luft.cSolid = false;
    luft.inhalt = 0x8000;
    o.shader["textures/test/luft"] = luft;
    long long n = 0;
    for (const Befund& b : pruefeKarte(kartenAbbild(k), o)) {
        if (std::find(kSichtKollision.begin(), kSichtKollision.end(), b.pruefung) == kSichtKollision.end()) { continue; }
        n += b.anzahl;
        if (text != nullptr) { *text += b.pruefung + " " + std::to_string(b.anzahl) + "; "; }
    }
    return n;
}

void testeRampe() {
    {   // die Rechnung allein
        std::vector<TreppenFlaeche> f;
        for (int i = 1; i <= 4; ++i) {
            const double x0 = 24.0 * (i - 1), x1 = 24.0 * i, z = 16.0 * i;
            f.push_back(TreppenFlaeche{{Vektor3d(x0, -64, z), Vektor3d(x1, -64, z), Vektor3d(x1, 64, z), Vektor3d(x0, 64, z)}});
        }
        const TreppenRampe r = rampeRechnen(f, 0.0, 0.7);
        pruefe(r.ok && r.tritte == 4 && nah(r.steigung, 16.0 / 24.0) && nah(r.oben, 64) && nah(r.unten, 0) && nah(r.richtung.x(), 1) &&
                   r.ebenen.size() == 6,
               "Rampe: 4 Tritte, Steigung 16/24 durch die Vorderkanten, unten 0, oben 64, hinauf +x, 6 Ebenen");
        std::vector<TreppenFlaeche> eben = {f[0], f[0]};
        pruefe(!rampeRechnen(eben, 0.0, 0.7).ok, "keine Treppe (nur eine Hoehe): Fehler statt Rampe");
    }
    {   // ROT: eine Treppe aus sichtbaren, nicht festen Stufen (wie ein misc_model ohne Clip) - die Pruefung Sicht gegen Kollision meldet
        Karte k;
        (void)karteLesen(treppenKarte("test/luft"), k);
        k.anmelden();
        std::string vorher;
        const long long rot = sichtKollision(k, &vorher);
        pruefe(rot > 0, "rot: nicht feste sichtbare Stufen ohne Clip - Sicht gegen Kollision meldet (" + vorher + ")");
        stufenWaehlen(k);
        const TreppeErgebnis e = treppeZuRampe(k, TreppeOptionen{});
        std::string nachher;
        const long long gruen = sichtKollision(k, &nachher);
        pruefe(e.ok && e.rampe && gruen == 0, "gruen: nach Treppe -> Clip-Rampe meldet Sicht gegen Kollision nichts (" + nachher + e.fehler + ")");
    }
    {   // feste Stufen: danach nicht fest (eine verdeckte Seite system/nodraw), die Rampe in der Ebene Collision, eine Undo-Stufe
        Karte k;
        (void)karteLesen(treppenKarte("test/fest", "system/caulk"), k);
        k.anmelden();
        stufenWaehlen(k);
        const TreppeErgebnis e = treppeZuRampe(k, TreppeOptionen{});
        const int ebene = kollisionsEbeneFinden(k);
        bool rampeImKasten = false;
        if (e.rampe) {
            const Kasten& g = e.rampe->grenzen(k.kontext);
            rampeImKasten = nah(g.origin.x() - g.extents.x(), -24, 0.01) && nah(g.origin.x() + g.extents.x(), 96, 0.01) &&
                            nah(g.origin.z() - g.extents.z(), 0, 0.01) && nah(g.origin.z() + g.extents.z(), 64, 0.01) && nah(g.extents.y(), 64, 0.01);
        }
        pruefe(e.ok && e.pinselNichtFest == 4 && e.sichtbareSeiten == 0 && e.begehbar,
               "4 Stufen nicht fest ueber ihre caulk-Seite (keine sichtbare Seite geaendert), Rampe begehbar (ok " + std::to_string(e.ok) + ", nicht fest " +
                   std::to_string(e.pinselNichtFest) + ", sichtbar " + std::to_string(e.sichtbareSeiten) + ", " + e.fehler + ")");
        std::string kasten;
        if (e.rampe) {
            const Kasten& g = e.rampe->grenzen(k.kontext);
            kasten = std::to_string(g.origin.x() - g.extents.x()) + ".." + std::to_string(g.origin.x() + g.extents.x()) + " z " +
                     std::to_string(g.origin.z() - g.extents.z()) + ".." + std::to_string(g.origin.z() + g.extents.z()) + " y " + std::to_string(g.extents.y());
        }
        pruefe(rampeImKasten, "Rampe x -24 ... 96 (Fuss eine Stufentiefe vor der ersten Stufe, hinten am obersten Tritt), z 0 ... 64, so breit wie die Treppe (" + kasten + ")");
        pruefe(ebene >= 0 && e.rampe && e.rampe->schicht == ebene && k.schichten[static_cast<std::size_t>(ebene)].name == "Collision",
               "die Rampe liegt in der Ebene Collision");
        pruefe(sichtKollision(k) == 0, "feste Stufen nach dem Werkzeug: Sicht gegen Kollision meldet nichts");
        {   // zur Auskunft: alle Clone-Wars-Befunde nach dem Werkzeug
            PruefOptionen o;
            o.spiel = GrenzSpiel::JkaSp;
            o.clonewars = true;
            for (const Befund& b : pruefeKarte(kartenAbbild(k), o)) {
                std::printf("      nach dem Werkzeug: %s %lld %s\n", b.pruefung.c_str(), b.anzahl, b.objekte.empty() ? "" : b.objekte[0].text.c_str());
            }
        }
        (void)k.undo.undo();
        const std::string zurueck = karteSchreiben(k);
        pruefe(zurueck.find("system/nodraw") == std::string::npos && zurueck.find("system/clip") == std::string::npos,
               "eine Undo-Stufe nimmt Rampe und nodraw-Seiten zurueck");
    }
    {   // nur reine Kollisionspinsel gewaehlt: keine sichtbaren Stufen
        Karte k;
        (void)karteLesen(treppenKarte("system/clip"), k);
        k.anmelden();
        stufenWaehlen(k);
        const TreppeErgebnis e = treppeZuRampe(k, TreppeOptionen{});
        pruefe(!e.ok, "nur Clip-Pinsel gewaehlt: keine sichtbaren Stufen, keine Rampe (" + e.fehler + ")");
    }
}

void testeWand() {
    // eine unregelmaessige Wand: drei Pinsel verschieden tief (Vorderseite bei y -8, -20, -4), die Figur steht bei y -200
    Karte k;
    const std::string t = "{\n\"classname\" \"worldspawn\"\n" + kastenText(0, -8, 0, 64, 16, 128, "test/fest") + kastenText(64, -20, 0, 128, 16, 128, "test/fest") +
                          kastenText(128, -4, 0, 192, 16, 128, "test/fest") + "}\n";
    (void)karteLesen(t, k);
    k.anmelden();
    k.auswahl.leeren();
    for (const auto& p : k.welt()->kinder()) { k.auswahl.waehlen(k.welt(), p); }
    const WandErgebnis e = glatteClipWand(k, WandArt::Ebene, Vektor3d(96, -200, 64), kClipSpieler);
    bool vorn = false;
    if (e.pinsel) {
        const Kasten& g = e.pinsel->grenzen(k.kontext);
        vorn = nah(g.origin.y() - g.extents.y(), -20, 0.01) && nah(g.origin.x() - g.extents.x(), 0, 0.01) && nah(g.origin.x() + g.extents.x(), 192, 0.01);
    }
    std::string wk;
    if (e.pinsel) {
        const Kasten& g = e.pinsel->grenzen(k.kontext);
        wk = "x " + std::to_string(g.origin.x() - g.extents.x()) + ".." + std::to_string(g.origin.x() + g.extents.x()) + " y " + std::to_string(g.origin.y() - g.extents.y()) +
             ".." + std::to_string(g.origin.y() + g.extents.y());
    }
    pruefe(e.ok && vorn && nah(e.normale.y(), -1, 1e-6), "Clip-Wand (Ebene): vorn am weitesten vorstehenden Punkt y -20, so lang wie die Wand, Normale zur Figur (" + wk +
                                                          ", n " + std::to_string(e.normale.x()) + " " + std::to_string(e.normale.y()) + ", " + e.fehler + ")");
    pruefe(e.pinsel && e.pinsel->schicht == kollisionsEbeneFinden(k), "Clip-Wand in der Ebene Collision");
    const std::vector<Ebene3d> h = wandEbenen({Vektor3d(0, 0, 0), Vektor3d(64, 0, 0), Vektor3d(0, 64, 0), Vektor3d(0, 0, 64), Vektor3d(64, 64, 64)},
                                             WandArt::Huelle, Vektor3d(0, -100, 0));
    pruefe(h.size() >= 4, "Clip-Wand (Huelle): Ebenen der konvexen Huelle");
}

void testeRaum() {
    Karte k;
    const std::string t = "{\n\"classname\" \"worldspawn\"\n" + kastenText(0, 0, 0, 100, 100, 10, "test/fest") + "}\n"
                          "{\n\"classname\" \"light\"\n\"origin\" \"50 50 60\"\n}\n";
    (void)karteLesen(t, k);
    k.anmelden();
    const std::string vorher = karteSchreiben(k);
    k.auswahl.leeren();
    k.auswahl.waehlen(k.welt(), k.welt()->kinder()[0]);
    k.auswahl.waehlen(k.entitaeten()[1]);
    RaumOptionen o;
    o.faktor = 1.25;
    o.pivot = Vektor3d(0, 0, 0);
    o.name = "hangar";
    const RaumErgebnis r = raumSkalieren(k, o);
    const Kasten& g = static_cast<const Pinsel&>(*k.welt()->kinder()[0]).grenzen(k.kontext);
    pruefe(r.primitive == 1 && r.entitaeten == 1 && nah(g.origin.x() + g.extents.x(), 125, 0.01) && nah(g.origin.z() + g.extents.z(), 12.5, 0.01),
           "Raum x 1,25 um 0 0 0: Pinsel 125 lang, 12,5 hoch; das Licht wandert");
    pruefe(k.entitaeten()[1]->wert("origin") == "62.5 62.5 75", "Ursprung des Lichts skaliert");
    pruefe(r.marker && r.marker->klassenname() == "info_notnull" && r.marker->wert("cw_scale") == "1.25" && r.marker->wert("cw_room") == "hangar",
           "Marker info_notnull mit cw_scale 1.25 und cw_room");
    const std::string map = karteSchreiben(k);
    pruefe(map.find("// cw_scale 1.25 cw_room \"hangar\" pivot 0 0 0") != std::string::npos, "Kommentar in der .map: // cw_scale 1.25 ...");
    Karte k2;
    (void)karteLesen(map, k2);
    k2.anmelden();
    pruefe(karteSchreiben(k2) == map, "die .map mit Kommentar liest sich wieder und schreibt sich gleich");
    (void)k.undo.undo();
    pruefe(karteSchreiben(k) == vorher, "eine Undo-Stufe stellt den Raum wieder her");
}

void testeVorgaben() {
    // ohne Werkzeug: keine Ebene Collision (nichts ist Standard)
    Karte k;
    (void)karteLesen(treppenKarte("test/fest"), k);
    k.anmelden();
    pruefe(kollisionsEbeneFinden(k) < 0, "ohne Werkzeug gibt es keine Ebene Collision");
}

}  // namespace

int main() {
    testeTestfigur();
    testeRampe();
    testeWand();
    testeRaum();
    testeVorgaben();
    std::printf("kartecwimport: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
