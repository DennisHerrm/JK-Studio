// kartespiegel.cpp - Karten-Editor Stufe 8b Punkt 6: der Spiegel-Modus.
//
// Ein Pinsel wird verschoben: das Gegenstueck entsteht im selben Befehl und
// folgt genau (Text gleich dem Spiegelbild wie Flip), auch live waehrend eines
// offenen Befehls; Rueckgaengig stellt beide byte-gleich her, Wiederholen
// ebenso. Dazu: ein schon vorhandenes Spiegelbild wird als Paar erkannt (keine
// Kopie), die andere Seite eines Paares bearbeiten, Loeschen, Patch,
// Punktentitaet mit angle/angles, Gruppe als Ganzes, ein Pinsel auf der Ebene
// (kein Gegenstueck), Achsen Y und Z.
#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/spiegel.h"

#include <cstdio>
#include <string>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
int g_pruefungen = 0;
void pruefe(bool ok, const std::string& was) {
    ++g_pruefungen;
    if (!ok) { ++g_fehler; }
    std::printf("%s  %s\n", ok ? "OK  " : "FEHL", was.c_str());
}

std::string quader(int x0, int y0, int z0, int x1, int y1, int z1, const std::string& tex) {
    char z[1024];
    std::snprintf(z, sizeof(z),
                  "{\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 3 5 15 0.5 0.25 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "}\n",
                  x0, y0, z1, x0, y1, z1, x1, y0, z1, tex.c_str(),   // oben
                  x0, y0, z0, x1, y0, z0, x0, y1, z0, tex.c_str(),   // unten
                  x0, y0, z0, x0, y0, z1, x1, y0, z0, tex.c_str(),   // vorn (y0)
                  x0, y1, z0, x1, y1, z0, x0, y1, z1, tex.c_str(),   // hinten (y1)
                  x0, y0, z0, x0, y1, z0, x0, y0, z1, tex.c_str(),   // links (x0)
                  x1, y0, z0, x1, y0, z1, x1, y1, z0, tex.c_str());  // rechts (x1)
    return z;
}

std::string patch3() {
    return "{\npatchDef2\n{\ntextures/test/rohr\n( 3 3 0 0 0 )\n(\n"
           "( ( 40 0 0 0 0 ) ( 40 0 32 0 0.5 ) ( 40 0 64 0 1 ) )\n"
           "( ( 56 16 0 0.5 0 ) ( 60 16 32 0.5 0.5 ) ( 56 16 64 0.5 1 ) )\n"
           "( ( 72 0 0 1 0 ) ( 72 0 32 1 0.5 ) ( 72 0 64 1 1 ) )\n"
           ")\n}\n}\n";
}

std::shared_ptr<Primitiv> kind(const Karte& k, std::size_t i) { return k.welt()->kinder()[i]; }

bool istSpiegelbild(const Karte& k, const Primitiv& a, const Primitiv& b, int achse, double ebene) {
    return primitivText(b, k.kontext) == primitivText(*primitivGespiegelt(a, achse, ebene, k.kontext), k.kontext);
}

void schieben(Karte& k, AuswahlBearbeitung& be, const Vektor3f& v, const std::string& name) {
    Befehl b(k.undo, name);
    be.verschiebeAuswahl(v);
}

std::shared_ptr<Entitaet> entitaetMit(const Karte& k, const std::string& schl, const std::string& wert) {
    for (const auto& e : k.entitaeten()) {
        if (e->wert(schl) == wert) { return e; }
    }
    return nullptr;
}

}  // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::string t = "{\n\"classname\" \"worldspawn\"\n";
    t += quader(32, 0, 0, 96, 64, 64, "test/a");          // 0: A
    t += quader(-200, 0, 0, -150, 32, 32, "test/c");      // 1: C
    t += quader(150, 0, 0, 200, 32, 32, "test/c");        // 2: D = Spiegelbild von C (Form)
    t += quader(-32, 100, 0, 32, 140, 16, "test/mitte");  // 3: auf der Ebene
    t += patch3();                                        // 4: Patch
    t += "}\n";
    t += "{\n\"classname\" \"info_player_start\"\n\"origin\" \"100 50 8\"\n\"angle\" \"30\"\n\"targetname\" \"start\"\n}\n";
    t += "{\n\"classname\" \"misc_model\"\n\"origin\" \"64 -64 0\"\n\"angles\" \"10 30 5\"\n\"targetname\" \"modell\"\n}\n";
    t += "{\n\"classname\" \"func_door\"\n\"angle\" \"-1\"\n\"targetname\" \"tuer\"\n" + quader(300, 0, 0, 316, 64, 128, "test/tuer") + "}\n";
    Karte k;
    (void)karteLesen(t, k);
    k.anmelden();
    k.kontext.texturLock = true;
    AuswahlBearbeitung be(k);
    SpiegelModus s;
    s.an = true;
    s.achse = 0;
    s.ebene = 0;
    k.undo.vorAbschluss = [&] { s.abgleichen(k, true); };
    const auto welt = k.welt();
    const std::size_t kinder0 = welt->kinder().size();

    // --- 1. Schluessel spiegeln -------------------------------------------------
    {
        const auto g = schluesselGespiegelt({{"origin", "100 50 8"}, {"angle", "30"}, {"angles", "10 30 5"}}, 0, 0);
        pruefe(g[0].second == "-100 50 8" && g[1].second == "150" && g[2].second == "10 150 355", "Schluessel X: origin, angle 30 -> 150, angles 10 30 5 -> 10 150 355");
        const auto y = schluesselGespiegelt({{"origin", "100 50 8"}, {"angle", "30"}, {"angles", "10 30 5"}}, 1, 16);
        pruefe(y[0].second == "100 -18 8" && y[1].second == "330" && y[2].second == "10 330 355", "Schluessel Y (Ebene 16): 50 -> -18, angle -> 330, angles -> 10 330 355");
        const auto z = schluesselGespiegelt({{"origin", "1 2 3"}, {"angle", "-1"}, {"angles", "10 30 5"}}, 2, 0);
        pruefe(z[0].second == "1 2 -3" && z[1].second == "-2" && z[2].second == "350 30 175", "Schluessel Z: angle -1 (hoch) -> -2, angles -> 350 30 175");
    }

    // --- 2. Pinsel verschieben: Gegenstueck folgt, ein Undo-Schritt -----------------
    const auto a = kind(k, 0);
    k.auswahl.waehlen(welt, a);
    s.abgleichen(k, false);
    pruefe(s.paarAnzahl() == 0 && welt->kinder().size() == kinder0, "gewaehlt, noch unveraendert: kein Gegenstueck");
    const std::string text0 = karteSchreiben(k);
    schieben(k, be, Vektor3f(16, 0, 0), "translate 1");
    pruefe(welt->kinder().size() == kinder0 + 1 && s.paarAnzahl() == 1, "erste Aenderung: Gegenstueck angelegt");
    const auto b = s.gegenstueck(a.get());
    if (!b) {
        std::printf("FEHL  kein Gegenstueck - Abbruch\n");
        return 1;
    }
    pruefe(b != nullptr && istSpiegelbild(k, *a, *b, 0, 0), "Gegenstueck = Spiegelbild (wie Flip, Texture Lock)");
    pruefe(b && b->art() == Primitiv::Art::Pinsel && static_cast<Pinsel&>(*b).grenzen(k.kontext).origin[0] == -80,
           "Gegenstueck liegt bei x -112..-48 (Mitte -80)");
    pruefe(k.undo.undoAnzahl() == 1 && k.undo.undoName() == "translate 1", "ein Rueckgaengig-Schritt");
    const std::string text1 = karteSchreiben(k);
    schieben(k, be, Vektor3f(0, 8, 0), "translate 2");
    pruefe(istSpiegelbild(k, *a, *b, 0, 0) && k.undo.undoAnzahl() == 2, "zweite Verschiebung: Gegenstueck folgt genau");
    const std::string text2 = karteSchreiben(k);
    // live: im offenen Befehl (wie beim Ziehen) gleicht der Frame-Abgleich an
    k.undo.start();
    be.verschiebeAuswahl(Vektor3f(0, 0, 24));
    pruefe(!istSpiegelbild(k, *a, *b, 0, 0), "offener Befehl: vor dem Abgleich noch alt");
    s.abgleichen(k, k.undo.befehlOffen());
    pruefe(istSpiegelbild(k, *a, *b, 0, 0), "offener Befehl: Abgleich im Frame -> live gespiegelt");
    be.verschiebeAuswahl(Vektor3f(0, 0, 8));
    k.undo.finish("translate 3");
    pruefe(istSpiegelbild(k, *a, *b, 0, 0) && k.undo.undoAnzahl() == 3, "Abschluss: gespiegelt, ein Schritt");
    const std::string text3 = karteSchreiben(k);
    pruefe(k.undo.undo() && karteSchreiben(k) == text2, "Rueckgaengig 3: byte-gleich");
    s.abgleichen(k, false);
    pruefe(k.undo.undo() && karteSchreiben(k) == text1, "Rueckgaengig 2: byte-gleich");
    s.abgleichen(k, false);
    pruefe(k.undo.undo() && karteSchreiben(k) == text0, "Rueckgaengig 1: byte-gleich, Gegenstueck weg");
    s.abgleichen(k, false);
    pruefe(k.undo.redo() && k.undo.redo() && k.undo.redo() && karteSchreiben(k) == text3, "Wiederholen x3: byte-gleich");
    s.abgleichen(k, false);
    pruefe(karteSchreiben(k) == text3, "Abgleich ausserhalb eines Befehls aendert nichts");

    // --- 3. die andere Seite bearbeiten ------------------------------------------
    k.auswahl.leeren();
    k.auswahl.waehlen(welt, b);
    s.abgleichen(k, false);
    schieben(k, be, Vektor3f(-8, 0, 0), "translate B");
    pruefe(istSpiegelbild(k, *b, *a, 0, 0) && welt->kinder().size() == kinder0 + 1, "Gegenstueck bearbeitet: das Original folgt");

    // --- 4. vorhandenes Spiegelbild wird erkannt ------------------------------------
    const auto c = kind(k, 1);
    const auto d = kind(k, 2);
    k.auswahl.leeren();
    k.auswahl.waehlen(welt, c);
    s.abgleichen(k, false);
    pruefe(s.gegenstueck(c.get()) == d && s.paarAnzahl() == 2, "vorhandenes Spiegelbild: als Paar erkannt");
    schieben(k, be, Vektor3f(0, 16, 0), "translate C");
    pruefe(istSpiegelbild(k, *c, *d, 0, 0) && welt->kinder().size() == kinder0 + 1, "C verschoben: D folgt, keine Kopie");

    // --- 5. Pinsel auf der Ebene: kein Gegenstueck ---------------------------------
    const auto m = kind(k, 3);
    k.auswahl.leeren();
    k.auswahl.waehlen(welt, m);
    s.abgleichen(k, false);
    schieben(k, be, Vektor3f(0, 0, 8), "translate M");
    pruefe(s.selbstSymmetrisch() == 1 && welt->kinder().size() == kinder0 + 1, "symmetrisch zur Ebene: kein Gegenstueck");

    // --- 6. Loeschen nimmt das Gegenstueck mit ----------------------------------------
    k.auswahl.leeren();
    k.auswahl.waehlen(welt, a);
    s.abgleichen(k, false);
    const std::string vorLoeschen = karteSchreiben(k);
    loescheAuswahl(k, be.einstellung, false);
    pruefe(welt->kinder().size() == kinder0 - 1 && s.paarAnzahl() == 1, "Loeschen: beide weg");
    pruefe(k.undo.undo() && karteSchreiben(k) == vorLoeschen, "Rueckgaengig: beide byte-gleich zurueck");
    s.abgleichen(k, false);

    // --- 7. Patch -----------------------------------------------------------------
    const auto p = kind(k, 4);
    k.auswahl.leeren();
    k.auswahl.waehlen(welt, p);
    s.abgleichen(k, false);
    schieben(k, be, Vektor3f(8, 0, 0), "translate P");
    const auto pg = s.gegenstueck(p.get());
    pruefe(pg != nullptr && pg->art() == Primitiv::Art::Patch && istSpiegelbild(k, *p, *pg, 0, 0), "Patch: Gegenstueck = Spiegelbild");

    // --- 8. Punktentitaet mit angle, Schluessel aendern -------------------------------------
    const auto start = entitaetMit(k, "targetname", "start");
    k.auswahl.leeren();
    k.auswahl.waehlen(start);
    s.abgleichen(k, false);
    schieben(k, be, Vektor3f(0, 0, 8), "translate start");
    const auto start2 = s.gegenstueck(start.get());
    pruefe(start2 && start2->wert("origin") == "-100 50 16" && start2->wert("angle") == "150", "Punktentitaet: origin -100 50 16, angle 150");
    {
        Befehl bf(k.undo, "entitySetKeyValue -key angle -value 90");
        start->setzen("angle", "90");
    }
    pruefe(start2 && start2->wert("angle") == "90", "angle 90 gesetzt: Gegenstueck 90");
    {
        Befehl bf(k.undo, "entitySetKeyValue -key angle -value 45");
        start->setzen("angle", "45");
    }
    pruefe(start2 && start2->wert("angle") == "135", "angle 45: Gegenstueck 135");
    const auto modell = entitaetMit(k, "targetname", "modell");
    k.auswahl.leeren();
    k.auswahl.waehlen(modell);
    s.abgleichen(k, false);
    {
        Befehl bf(k.undo, "entitySetKeyValue -key spawnflags -value 1");
        modell->setzen("spawnflags", "1");
    }
    const auto modell2 = s.gegenstueck(modell.get());
    pruefe(modell2 && modell2->wert("origin") == "-64 -64 0" && modell2->wert("angles") == "10 150 355" && modell2->wert("spawnflags") == "1",
           "misc_model: origin, angles 10 150 355, neuer Schluessel mit");

    // --- 9. Gruppe als Ganzes -------------------------------------------------------
    const auto tuer = entitaetMit(k, "targetname", "tuer");
    k.auswahl.leeren();
    k.auswahl.waehlen(tuer);
    s.abgleichen(k, false);
    schieben(k, be, Vektor3f(0, 32, 0), "translate tuer");
    const auto tuer2 = s.gegenstueck(tuer.get());
    pruefe(tuer2 && tuer2->kinder().size() == 1 && tuer2->wert("angle") == "-1" && istSpiegelbild(k, *tuer->kinder()[0], *tuer2->kinder()[0], 0, 0),
           "func_door als Ganzes: Pinsel gespiegelt, angle -1 bleibt");

    // --- 10. Achse Y und Z ---------------------------------------------------------------
    for (int achse = 1; achse <= 2; ++achse) {
        s.achse = achse;
        s.ebene = achse == 1 ? -16 : 256;
        const auto q = kind(k, 1);   // C
        k.auswahl.leeren();
        k.auswahl.waehlen(welt, q);
        s.abgleichen(k, false);
        const std::size_t vorher = welt->kinder().size();
        schieben(k, be, Vektor3f(8, 0, 0), "translate achse");
        const auto g = s.gegenstueck(q.get());
        pruefe(welt->kinder().size() == vorher + 1 && g && istSpiegelbild(k, *q, *g, achse, s.ebene),
               std::string("Achse ") + (achse == 1 ? "Y (Ebene -16)" : "Z (Ebene 256)") + ": Gegenstueck gespiegelt");
    }

    // --- 11. aus: nichts mehr -------------------------------------------------------
    s.an = false;
    s.abgleichen(k, false);
    const std::size_t vorAus = welt->kinder().size();
    schieben(k, be, Vektor3f(8, 0, 0), "translate aus");
    pruefe(s.paarAnzahl() == 0 && welt->kinder().size() == vorAus, "Modus aus: keine Gegenstuecke");

    std::printf("%d Pruefungen, %d Fehler\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
