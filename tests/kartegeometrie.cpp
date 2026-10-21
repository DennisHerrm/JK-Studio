// kartegeometrie.cpp - Karten-Editor Stufe 1a, Punkt 2a: Mathematik und
// Pinselgeometrie (bhed/karte/mathe.h, wicklung.h, pinsel.h, rueckgaengig.h).
//
// Geprueft wird, was NRC aus drei Ebenenpunkten macht: Ebene (double),
// Wicklungen durch Zuschneiden eines "unendlichen" Polygons, Aufraeumen
// entarteter Kanten/Flaechen, Vorrang doppelter Ebenen, Kanten- und
// Eckenlisten, Grenzen, Inhaltsflags/Detail und Rueckgaengig. Die Erwartungen
// sind analytisch (Schnittpunkte von Ebenen) oder folgen direkt aus dem
// NRC-Quelltext (Zeilen stehen dabei). Der Abgleich mit dem echten q3map2
// folgt in kartevergleich (Punkt 3).
#include "bhed/karte/pinsel.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

Vektor3d v(double x, double y, double z) { return Vektor3d(x, y, z); }

// Pinsel aus Ebenenpunkten wie in einer .map (Quake3-Reihenfolge).
std::shared_ptr<Pinsel> pinselAus(std::initializer_list<EbenenPunkte> ebenen) {
    auto p = std::make_shared<Pinsel>();
    for (const auto& e : ebenen) {
        Flaeche& f = p->neueFlaecheLaden();
        f.ladeEbenenPunkte(e);
        f.ladeShader("textures/system/caulk");
        f.skalierungAngewandt = true;
    }
    return p;
}

// Der Quader -64..64 x -32..32 x 0..128, so wie NRC ihn schreibt
// (Brush_ConstructCuboid, Ebenenpunkte von aussen gesehen 0,1,2).
std::shared_ptr<Pinsel> quader() {
    return pinselAus({
        {v(64, 32, 128), v(64, -32, 128), v(-64, 32, 128)},   // oben (Normale +z)
        {v(64, 32, 128), v(-64, 32, 128), v(64, 32, 0)},      // +y
        {v(64, 32, 128), v(64, 32, 0), v(64, -32, 128)},      // +x
        {v(-64, -32, 0), v(64, -32, 0), v(-64, 32, 0)},       // unten (-z)
        {v(-64, -32, 0), v(-64, -32, 128), v(64, -32, 0)},    // -y
        {v(-64, -32, 0), v(-64, 32, 0), v(-64, -32, 128)},    // -x
    });
}

bool nahe(double a, double b, double eps = 1e-9) { return std::fabs(a - b) <= eps; }

}  // namespace

int main() {
    Kontext k;

    // --- Ebene aus drei Punkten (plane.h:141-147) ---------------------------
    {
        const Ebene3d e = ebeneAusPunkten(v(0, 0, 0), v(64, 0, 0), v(0, 64, 0));
        // cross(p2-p0, p1-p0) = (0,64,0)x(64,0,0) = (0,0,-4096) -> (0,0,-1)
        pruefe(e.a == 0 && e.b == 0 && e.c == -1 && e.d == 0, "Ebene (0,0,0)(64,0,0)(0,64,0) = (0,0,-1),0");
        const Ebene3d s = ebeneAusPunkten(v(0, 0, 16), v(0, 64, 16), v(64, 0, 48));
        // n = normalise( (64,0,32) x (0,64,0) ) = normalise( (-2048, 0, 4096) )
        pruefe(nahe(s.a, -2048.0 / std::sqrt(2048.0 * 2048.0 + 4096.0 * 4096.0)) && nahe(s.b, 0)
                   && nahe(s.c, 4096.0 / std::sqrt(2048.0 * 2048.0 + 4096.0 * 4096.0)),
               "schraege Ebene: Normale normalise((64,0,32)x(0,64,0))");
        pruefe(nahe(s.d, 16 * s.c), "schraege Ebene: Abstand = dot(p0, n)");
        // plane3_valid: |n|^2 == 1 auf 0.01
        pruefe(ebeneGueltig(s) && !ebeneGueltig(ebeneAusPunkten(v(0, 0, 0), v(1, 0, 0), v(2, 0, 0))),
               "kollineare Punkte geben keine gueltige Ebene (NaN)");
    }

    // --- Projektionsachse: Vorrang Z > X > Y (winding.h:46-54) -------------
    {
        pruefe(projektionsachseFuerNormale(Vektor3f(0.6F, 0.6F, 0.52F)) == Projektionsachse::X,
               "Achse: x und y gleich -> X (Y ist nicht 'besser')");
        pruefe(projektionsachseFuerNormale(Vektor3f(0.57735F, 0.57735F, 0.57735F)) == Projektionsachse::Z,
               "Achse: alle gleich -> Z");
        pruefe(projektionsachseFuerNormale(Vektor3f(0.1F, 0.9F, 0.4F)) == Projektionsachse::Y, "Achse: y gross -> Y");
    }

    // --- Quader: Wicklungen, Grenzen, Kanten, Ecken ------------------------
    {
        auto p = quader();
        p->bRep(k);
        pruefe(!p->entartet(k), "Quader ist nicht entartet");
        bool vier = true;
        for (std::size_t i = 0; i < p->anzahl(); ++i) {
            vier = vier && (*p)[i].wicklung().numpoints == 4;
        }
        pruefe(vier, "Quader: jede Flaeche hat 4 Punkte");
        const Kasten& g = p->grenzen(k);
        pruefe(g.origin[0] == 0 && g.origin[1] == 0 && g.origin[2] == 64 && g.extents[0] == 64 && g.extents[1] == 32
                   && g.extents[2] == 64,
               "Quader: Grenzen origin (0,0,64) extents (64,32,64)");
        pruefe(p->ecken(k).size() == 8, "Quader: 8 eindeutige Ecken (buildBRep)");
        pruefe(p->kanten(k).size() == 12, "Quader: 12 eindeutige Kanten (buildBRep)");
        pruefe(!p->eulerFehler(k), "Quader: Euler V+F-E = 2");
        // jede Ecke liegt genau auf einer Quaderecke
        bool ecken = true;
        for (const auto& e : p->ecken(k)) {
            const Vektor3d q = p->eckpunkt(e);
            ecken = ecken && std::fabs(std::fabs(q.x()) - 64) < 1e-9 && std::fabs(std::fabs(q.y()) - 32) < 1e-9
                 && (std::fabs(q.z()) < 1e-9 || std::fabs(q.z() - 128) < 1e-9);
        }
        pruefe(ecken, "Quader: Eckpunkte auf (+-64, +-32, 0/128) bis 1e-9");
        // Nachbarschaft: jede Kante der Oberseite grenzt an eine Seitenflaeche
        const Wicklung& oben = (*p)[0].wicklung();
        bool nachbarn = true;
        for (std::size_t i = 0; i < oben.numpoints; ++i) {
            nachbarn = nachbarn && oben[i].adjacent != 0 && oben[i].adjacent != 3 && oben[i].adjacent < 6;
        }
        pruefe(nachbarn, "Quader: Nachbarn der Oberseite sind Seitenflaechen");
        // Schwerpunkt der Oberseite (Winding_Centroid)
        const Vektor3f sp = (*p)[0].schwerpunkt();
        pruefe(sp[0] == 0 && sp[1] == 0 && sp[2] == 128, "Quader: Schwerpunkt Oberseite (0,0,128)");
        pruefe(p->traegtBei() && p->hatBeitragendeFlaechen(k), "Quader traegt bei");
    }

    // --- schraeger Keil: Schnittpunkte analytisch ---------------------------
    {
        // Quader 0..64 in x/y, z von 0 bis Ebene z = 16 + x/2 (Normale (-1,0,2)/sqrt5)
        auto p = pinselAus({
            {v(0, 0, 16), v(0, 64, 16), v(64, 0, 48)},   // schraeg oben
            {v(64, 64, 64), v(0, 64, 64), v(64, 64, 0)}, // +y
            {v(64, 64, 64), v(64, 64, 0), v(64, 0, 64)}, // +x
            {v(0, 0, 0), v(64, 0, 0), v(0, 64, 0)},      // unten
            {v(0, 0, 0), v(0, 0, 64), v(64, 0, 0)},      // -y
            {v(0, 0, 0), v(0, 64, 0), v(0, 0, 64)},      // -x
        });
        p->bRep(k);
        const Wicklung& w = (*p)[0].wicklung();
        pruefe(w.numpoints == 4, "Keil: schraege Flaeche hat 4 Punkte");
        bool aufEbene = true;
        double hoechstes = 0;
        for (std::size_t i = 0; i < w.numpoints; ++i) {
            const auto& q = w[i].vertex;
            aufEbene = aufEbene && std::fabs(q.z() - (16 + q.x() / 2)) < 1e-9;
            hoechstes = std::max(hoechstes, q.z());
        }
        pruefe(aufEbene, "Keil: Punkte liegen auf z = 16 + x/2 (1e-9)");
        pruefe(nahe(hoechstes, 48), "Keil: hoechster Punkt z = 48");
        pruefe(p->ecken(k).size() == 8 && p->kanten(k).size() == 12, "Keil: 8 Ecken, 12 Kanten");
    }

    // --- doppelte Ebene: die mit kleinerem Index gewinnt (brush.h:1445-1454)
    {
        auto p = pinselAus({
            {v(64, 32, 128), v(64, -32, 128), v(-64, 32, 128)},
            {v(64, 32, 128), v(-64, 32, 128), v(64, 32, 0)},
            {v(64, 32, 128), v(64, 32, 0), v(64, -32, 128)},
            {v(-64, -32, 0), v(64, -32, 0), v(-64, 32, 0)},
            {v(-64, -32, 0), v(-64, -32, 128), v(64, -32, 0)},
            {v(-64, -32, 0), v(-64, 32, 0), v(-64, -32, 128)},
            {v(0, 32, 128), v(0, -32, 128), v(-64, 32, 128)},   // dieselbe Ebene wie 0
        });
        p->bRep(k);
        pruefe((*p)[0].traegtBei() && !(*p)[6].traegtBei(), "doppelte Ebene: Flaeche 0 bleibt, Flaeche 6 nicht");
        pruefe(!p->entartet(k) && p->ecken(k).size() == 8, "doppelte Ebene: Pinsel bleibt ein Quader");
        // removeEmptyFaces entfernt die ueberzaehlige Flaeche
        p->leereFlaechenEntfernen(k);
        pruefe(p->anzahl() == 6, "removeEmptyFaces: 6 Flaechen bleiben");
    }

    // --- ueberzaehlige Ebene ausserhalb: traegt nicht bei -------------------
    {
        auto p = quader();
        Flaeche& f = p->neueFlaecheLaden();
        f.ladeEbenenPunkte({v(0, 32, 200), v(0, -32, 200), v(-64, 32, 200)});   // z = 200, ausserhalb
        p->bRep(k);
        pruefe(!(*p)[6].traegtBei() && (*p)[0].traegtBei(), "Ebene ausserhalb des Quaders traegt nicht bei");
    }

    // --- offener Pinsel: entartet (isBounded) ------------------------------
    {
        auto p = pinselAus({
            {v(64, 32, 128), v(64, -32, 128), v(-64, 32, 128)},
            {v(64, 32, 128), v(-64, 32, 128), v(64, 32, 0)},
            {v(64, 32, 128), v(64, 32, 0), v(64, -32, 128)},
            {v(-64, -32, 0), v(64, -32, 0), v(-64, 32, 0)},
            {v(-64, -32, 0), v(-64, -32, 128), v(64, -32, 0)},
        });
        p->bRep(k);
        bool leer = true;
        for (std::size_t i = 0; i < p->anzahl(); ++i) {
            leer = leer && (*p)[i].wicklung().numpoints == 0;
        }
        pruefe(p->entartet(k) && leer, "offener Pinsel (5 Ebenen): entartet, alle Wicklungen leer");
        // Brush::contributes (brush.h:2233-2298) zaehlt nur Flaechen mit mehr als zwei
        // Punkten nach dem Zuschneiden - die offenen Seiten reichen bis 64*1024*2.22.
        pruefe(p->traegtBei(), "offener Pinsel 'traegt bei' (contributes prueft keine Geschlossenheit)");
    }

    // --- Flaeche, die nur eine Kante beruehrt: entartete Flaeche -----------
    {
        auto p = quader();
        // Ebene durch die Kante x=64, z=128 unter 45 Grad: beruehrt nur die Kante
        Flaeche& f = p->neueFlaecheLaden();
        f.ladeEbenenPunkte({v(64, 32, 128), v(64, -32, 128), v(0, 32, 192)});
        p->bRep(k);
        pruefe((*p)[6].wicklung().numpoints == 0, "Ebene an der Kante: Wicklung entfernt (removeDegenerateFaces)");
        pruefe(!p->eulerFehler(k) && p->ecken(k).size() == 8 && p->kanten(k).size() == 12,
               "Ebene an der Kante: Quader-Topologie bleibt (8/12, Euler)");
    }

    // --- Quader aus Grenzen (Brush_ConstructCuboid) ------------------------
    {
        auto p = std::make_shared<Pinsel>();
        p->quaderBauen(Kasten(Vektor3f(0, 0, 32), Vektor3f(32, 16, 32)), "textures/base/wall", TexturProjektion(), k);
        pruefe(p->anzahl() == 6, "quaderBauen: 6 Flaechen");
        const auto& e0 = (*p)[0].ebenenPunkte();
        pruefe(e0[0] == v(32, 16, 64) && e0[1] == v(32, -16, 64) && e0[2] == v(-32, 16, 64),
               "quaderBauen: erste Ebene (maxs, maxs mit y=min, maxs mit x=min) wie brushmanip.cpp:51-59");
        pruefe((*p)[0].ebene().c == 1 && (*p)[0].ebene().d == 64, "quaderBauen: erste Flaeche ist +z bei 64");
        pruefe(p->ecken(k).size() == 8 && !p->entartet(k), "quaderBauen: gueltiger Quader");
        pruefe((*p)[0].projektionInitialisiert && (*p)[0].skalierungAngewandt,
               "quaderBauen: Projektion initialisiert und skaliert (FaceTexdef::realiseShader)");
        // BP: addScale mit 64x64 (notex) -> 2/64
        pruefe((*p)[0].projektion().bp.coords[0][0] == 2.0f / 64.0f, "quaderBauen: BP-Matrix /64 (addScale)");
    }

    // --- Detail (brush.h:1332-1351) und Flags (ContentsFlagsValue) ---------
    {
        auto p = quader();
        p->setzeDetail(true);
        pruefe(p->istDetail() && (*p)[3].flags().inhaltFlags == kDetailMaske && !(*p)[3].flags().angegeben,
               "Detail: nicht angegebene Flags -> (0, DETAIL, 0, nicht angegeben)");
        InhaltFlags f;
        f.inhaltFlags = 0x10;
        f.flaechenFlags = 0x200;
        f.angegeben = true;
        (*p)[1].setzeFlags(f);
        pruefe((*p)[1].flags().inhaltFlags == (0x10 | kDetailMaske), "SetFlags behaelt das Detail-Bit");
        p->setzeDetail(false);
        pruefe(!p->istDetail() && (*p)[1].flags().inhaltFlags == 0x10 && (*p)[3].flags().inhaltFlags == 0,
               "Structural: Detail-Bit weg, angegebene Flags bleiben");
    }

    // --- Rueckgaengig: Flaechenaenderung und Pinselliste --------------------
    {
        Rueckgaengig undo;
        auto p = quader();
        p->anmeldenMitFlaechen(&undo);
        const EbenenPunkte alt = (*p)[0].ebenenPunkte();
        {
            Befehl b(undo, "ebeneVerschieben");
            (*p)[0].setzeEbenenPunkte({v(64, 32, 160), v(64, -32, 160), v(-64, 32, 160)});
            (*p)[0].setzeEbenenPunkte({v(64, 32, 170), v(64, -32, 170), v(-64, 32, 170)});
        }
        pruefe(undo.undoAnzahl() == 1 && undo.undoName() == "ebeneVerschieben", "Undo: ein Schritt je Befehl");
        pruefe(p->grenzen(k).extents[2] == 85, "Undo: Aenderung wirkt (Hoehe 170)");
        pruefe(undo.undo(), "Undo ausgefuehrt");
        pruefe((*p)[0].ebenenPunkte() == alt && p->grenzen(k).extents[2] == 64, "Undo: alter Stand (Hoehe 128)");
        pruefe(undo.redoAnzahl() == 1 && undo.redo(), "Redo ausgefuehrt");
        pruefe((*p)[0].ebenenPunkte()[0] == v(64, 32, 170), "Redo: Stand nach dem Befehl");
        {
            Befehl b(undo, "nichts");
        }
        pruefe(undo.undoAnzahl() == 1, "Befehl ohne Aenderung legt keinen Schritt an");
        {
            Befehl b(undo, "flaecheWeg");
            p->flaecheEntfernen(5);
        }
        pruefe(p->anzahl() == 5 && p->entartet(k), "Flaeche entfernt -> offen");
        pruefe(undo.undo() && p->anzahl() == 6 && !p->entartet(k) && p->ecken(k).size() == 8,
               "Undo stellt die Flaechenliste wieder her");
        p->abmeldenMitFlaechen();
    }

    // --- Transformation mit Spiegelung dreht die Punktreihenfolge ----------
    {
        auto p = quader();
        Matrix4 m = skalierungsMatrix(Vektor3f(-1, 1, 1));
        p->transformieren(m, k);
        pruefe(!p->entartet(k) && p->ecken(k).size() == 8, "Spiegelung: Quader bleibt gueltig (Punkte umgedreht)");
        pruefe((*p)[2].ebene().a == -1, "Spiegelung: +x-Flaeche zeigt nach -x");
    }

    // --- Rasten: float_snapped rechnet im Typ des Rasters ------------------
    {
        const double d = 10.3;
        pruefe(zahlEingerastet(d, 0.25f) == 10.25 && zahlEingerastet(d, 0.0) == d, "float_snapped");
    }

    std::printf("kartegeometrie: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
