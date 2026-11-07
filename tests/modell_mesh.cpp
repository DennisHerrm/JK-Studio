// modell_mesh.cpp - Modell-Block 1d: Modellieren (bhed/modell/wof/poly.h, bhed/modell/md3datei.h)
//
// Nach dem Vorbild der Tests von WhiteoutLib (tests/wem_modelling_*_test.cpp: jedes Werkzeug auf einem kleinen Netz,
// erwartete Zahl der Punkte, Kanten und Flaechen, mannigfaltig, Betragsgrenze) und WhiteoutFlakes (tests/mesh_tools_
// test.cpp), auf eigenen Netzen und dem JKA-Modell data/fixtures/model.glm:
//   Grundkoerper  Kasten, Ebene, Zylinder, Kugel, Kegel, Rohr, Torus: Punkte/Kanten/Flaechen, Euler, geschlossen
//   Werkzeuge     Extrude (Gruppe, je Vieleck, Rand), Inset (Grenze 0,5 am Einheitswuerfel), Bevel, Chamfer (eine Kante,
//                 alle zwoelf: Grenze 0,5 wie WL; Punkt), Connect (Kanten, Punkte), Loop Cut, Cut, Weld, Target Weld,
//                 Merge, Bridge (Raender, Vielecke), Remove (Kante, Punkt), Delete, Detach (Element, neuer Teil),
//                 Symmetrize, To Quads
//   Erhalten      UV der eingerueckten Ecken in der Quellflaeche; Gewichte beim Teilen gemischt (vier, Summe 1)
//   JKA           Grenzen (1000 Ecken je Flaeche, Namen 64, Tag ein Dreieck); Umlauf der Datei (Normale aussen);
//                 Rundlauf GLM ohne Aenderung Byte fuer Byte, ueber das Arbeitsnetz dieselben Dreiecke; MD3 neu
//                 geschrieben und gelesen, Rundlauf Byte fuer Byte, bewegter Punkt kommt an
//
// Aufruf: modellmeshtest <model.glm>
#include "bhed/glm.h"
#include "bhed/md3.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/glmdatei.h"
#include "bhed/modell/md3datei.h"
#include "bhed/modell/wof/poly.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", what.c_str());
    if (!ok) { ++fails; }
}
std::string slurp(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}
using namespace bhed;
using namespace bhed::modell;
using namespace bhed::modell::wof;
std::size_t uz(int i) { return static_cast<std::size_t>(i); }

int randKanten(const PolyNetz& n) {
    int z = 0;
    for (const PKante& k : n.kanten) { z += k.seiten.size() == 1U ? 1 : 0; }
    return z;
}
std::string zahlen(const PolyNetz& n) {
    return std::to_string(n.lebendePunkte()) + "/" + std::to_string(n.kantenZahl()) + "/" + std::to_string(n.lebendeFlaechen());
}
// Punkte/Kanten/Flaechen nach dem Verdichten, gueltig
bool netzIst(PolyNetz& n, int p, int k, int f, int rand = -1) {
    n.verdichten();
    std::string g;
    const bool ok = n.pruefen(&g);
    if (!ok) { std::printf("        ungueltig: %s\n", g.c_str()); }
    const bool z = n.lebendePunkte() == p && n.kantenZahl() == k && n.lebendeFlaechen() == f && (rand < 0 || randKanten(n) == rand);
    if (!z) { std::printf("        ist %s Rand %d, erwartet %d/%d/%d Rand %d\n", zahlen(n).c_str(), randKanten(n), p, k, f, rand); }
    return ok && z;
}

PolyNetz leer(int teile = 1) {
    PolyNetz n;
    n.teilBeruehrt.assign(uz(teile), 0);
    n.teilPunkte.assign(uz(teile), 0);
    return n;
}
PolyNetz kasten(float l = 1.0F, int seg = 1) {
    PolyNetz n = leer();
    GrundParam g;
    g.form = Grundform::Kasten;
    g.laenge = g.breite = g.hoehe = l;
    g.segL = g.segB = g.segH = seg;
    (void)grundkoerper(n, g, 0);
    n.verdichten();
    return n;
}
int vieleckBei(const PolyNetz& n, int achse, float wert) {   // das Vieleck, dessen Punkte alle auf der Ebene liegen
    for (std::size_t f = 0; f < n.flaechen.size(); ++f) {
        if (n.flaechen[f].weg) { continue; }
        bool alle = true;
        for (const PEcke& e : n.flaechen[f].ecken) { alle = alle && std::abs(n.punkte[uz(e.punkt)].ort[achse] - wert) < 1e-4F; }
        if (alle) { return static_cast<int>(f); }
    }
    return -1;
}
std::pair<int, int> kanteZwischen(const PolyNetz& n, float ax, float ay, float az, float bx, float by, float bz) {
    int a = -1, b = -1;
    for (std::size_t i = 0; i < n.punkte.size(); ++i) {
        const float* o = n.punkte[i].ort;
        if (std::abs(o[0] - ax) + std::abs(o[1] - ay) + std::abs(o[2] - az) < 1e-4F) { a = static_cast<int>(i); }
        if (std::abs(o[0] - bx) + std::abs(o[1] - by) + std::abs(o[2] - bz) < 1e-4F) { b = static_cast<int>(i); }
    }
    return {a, b};
}
int punktBei(const PolyNetz& n, float x, float y, float z) { return kanteZwischen(n, x, y, z, x, y, z).first; }

Inhalt inhaltMit(const PolyNetz& n, const std::string& name = "kiste") {
    Inhalt in;
    in.herkunft = Herkunft::Md3;
    in.name = name;
    in.stufenZahl = 1;
    Teil t;
    t.name = name;
    t.shader = "textures/test/kiste";
    t.stufen.resize(1);
    in.teile.push_back(t);
    PolyNetz k = n;
    k.teilBeruehrt.assign(1, 1);
    polyZurueck(k, in);
    return in;
}

void grundkoerperTest() {
    std::printf("Grundkoerper\n");
    struct F {
        Grundform f;
        float l, b, h;
        int s1, s2, s3;
        int p, k, fl;
        const char* name;
    };
    const F fs[] = {
        {Grundform::Kasten, 1, 1, 1, 1, 1, 1, 8, 12, 6, "Kasten 1x1x1"},
        {Grundform::Kasten, 2, 2, 2, 2, 2, 2, 26, 48, 24, "Kasten 2x2x2 Segmente"},
        {Grundform::Ebene, 2, 2, 0, 2, 2, 1, 9, 12, 4, "Ebene 2x2"},
        {Grundform::Zylinder, 1, 1, 2, 8, 1, 1, 16, 24, 10, "Zylinder 8 Seiten"},
        {Grundform::Zylinder, 1, 1, 2, 8, 1, 3, 32, 56, 26, "Zylinder 8 Seiten 3 Hoehen"},
        {Grundform::Kugel, 1, 1, 1, 8, 1, 4, 26, 56, 32, "Kugel 8x4"},
        {Grundform::Kegel, 1, 0, 2, 8, 1, 1, 9, 16, 9, "Kegel mit Spitze"},
        {Grundform::Kegel, 2, 1, 2, 8, 1, 1, 16, 24, 10, "Kegelstumpf"},
        {Grundform::Rohr, 2, 1, 2, 8, 1, 1, 32, 64, 32, "Rohr 8 Seiten"},
        {Grundform::Torus, 2, 0.5F, 0, 8, 1, 4, 32, 64, 32, "Torus 8x4"},
    };
    for (const F& f : fs) {
        PolyNetz n = leer();
        GrundParam g;
        g.form = f.f;
        g.laenge = f.l;
        g.breite = f.b;
        g.hoehe = f.h;
        g.segL = f.s1;
        g.segB = f.s2;
        g.segH = f.s3;
        const Plan p = grundkoerper(n, g, 0);
        expect(std::string(f.name) + ": " + std::to_string(f.p) + "/" + std::to_string(f.k) + "/" + std::to_string(f.fl) + ", geschlossen",
               !p.abgelehnt() && netzIst(n, f.p, f.k, f.fl, f.f == Grundform::Ebene ? 8 : 0));
        // Normalen zeigen nach aussen (weg von der Mitte des Koerpers)
        if (f.f != Grundform::Ebene && f.f != Grundform::Rohr && f.f != Grundform::Torus) {
            float mx[3] = {0, 0, 0};
            for (const PPunkt& q : n.punkte) {
                for (int k = 0; k < 3; ++k) { mx[k] += q.ort[k] / static_cast<float>(n.punkte.size()); }
            }
            int falsch = 0;
            for (const PFlaeche& fl : n.flaechen) {
                for (const PEcke& e : fl.ecken) {
                    const float* o = n.punkte[uz(e.punkt)].ort;
                    const float d = (o[0] - mx[0]) * e.e.normale[0] + (o[1] - mx[1]) * e.e.normale[1] + (o[2] - mx[2]) * e.e.normale[2];
                    falsch += d <= 0.0F ? 1 : 0;
                }
            }
            expect(std::string(f.name) + ": jede Normale nach aussen", falsch == 0);
        }
    }
    // Kasten: harte Kanten (je Seite eigene Normale), UV je Seite 0..1
    PolyNetz k = kasten();
    const int oben = vieleckBei(k, 2, 1.0F);
    bool uvOk = oben >= 0;
    if (uvOk) {
        for (const PEcke& e : k.flaechen[uz(oben)].ecken) {
            uvOk = uvOk && (e.e.uv[0] == 0.0F || e.e.uv[0] == 1.0F) && (e.e.uv[1] == 0.0F || e.e.uv[1] == 1.0F) && std::abs(e.e.normale[2] - 1.0F) < 1e-5F;
        }
    }
    expect("Kasten: Deckel mit UV 0..1 und Normale +Z (harte Kante)", uvOk);
}

void extrudeTest() {
    std::printf("Extrude, Inset, Bevel\n");
    {
        PolyNetz n = kasten();
        const int oben = vieleckBei(n, 2, 1.0F);
        Plan p = extrudeFlaechen(n, {oben}, ExtrudeArt::Gruppe);
        betragAnwenden(n, p, 10.0F);
        const bool hoch = !p.abgelehnt() && std::abs(n.punkte[uz(n.flaechen[uz(p.auswahl.flaechen[0])].ecken[0].punkt)].ort[2] - 11.0F) < 1e-4F;
        expect("Extrude Gruppe: 12/20/10, Deckel auf 11", hoch && netzIst(n, 12, 20, 10, 0));
    }
    {
        PolyNetz n = kasten();
        const int oben = vieleckBei(n, 2, 1.0F), vorn = vieleckBei(n, 1, 0.0F - 0.5F);
        Plan p = extrudeFlaechen(n, {oben, vorn}, ExtrudeArt::JeVieleck);
        betragAnwenden(n, p, 1.0F);
        expect("Extrude je Vieleck (zwei Nachbarn): 16/28/14", !p.abgelehnt() && netzIst(n, 16, 28, 14, 0));
    }
    {
        PolyNetz n = kasten();
        const int oben = vieleckBei(n, 2, 1.0F), vorn = vieleckBei(n, 1, -0.5F);
        Plan p = extrudeFlaechen(n, {oben, vorn}, ExtrudeArt::LokaleNormale);
        betragAnwenden(n, p, 1.0F);
        expect("Extrude lokale Normale (zwei Nachbarn): 14/24/12", !p.abgelehnt() && netzIst(n, 14, 24, 12, 0));
    }
    {
        PolyNetz n = leer();
        GrundParam g;
        g.form = Grundform::Ebene;
        g.laenge = g.breite = 2.0F;
        (void)grundkoerper(n, g, 0);
        n.verdichten();
        std::vector<std::pair<int, int>> r;
        for (const PKante& k : n.kanten) { r.emplace_back(k.a, k.b); }
        Plan p = extrudeRand(n, r);
        betragAnwenden(n, p, 1.0F);
        // die Ebene waechst nach aussen: der neue Rand liegt 1 weiter
        float maxX = 0.0F;
        for (const PPunkt& q : n.punkte) { maxX = std::max(maxX, q.ort[0]); }
        expect("Extrude Rand (Ebene, vier Kanten): 8/12/5, waechst in der Ebene (Ecke auf der Winkelhalbierenden, 1 + 1/sqrt 2)", !p.abgelehnt() && std::abs(maxX - (1.0F + std::sqrt(0.5F))) < 1e-4F && netzIst(n, 8, 12, 5, 4));
    }
    {
        PolyNetz n = kasten();
        const int oben = vieleckBei(n, 2, 1.0F);
        Plan p = inset(n, {oben}, InsetArt::Gruppe);
        betragAnwenden(n, p, 0.25F);
        // die inneren Ecken bei 0,25 / 0,75; ihre UV in der Quellflaeche mitgewandert (0,25 / 0,75)
        const PFlaeche& in = n.flaechen[uz(p.auswahl.flaechen[0])];
        bool lage = true, uv = true;
        for (const PEcke& e : in.ecken) {
            const float* o = n.punkte[uz(e.punkt)].ort;
            lage = lage && std::abs(std::abs(o[0]) - 0.25F) < 1e-4F && std::abs(std::abs(o[1]) - 0.25F) < 1e-4F;
            uv = uv && (std::abs(e.e.uv[0] - 0.25F) < 1e-4F || std::abs(e.e.uv[0] - 0.75F) < 1e-4F) && (std::abs(e.e.uv[1] - 0.25F) < 1e-4F || std::abs(e.e.uv[1] - 0.75F) < 1e-4F);
        }
        expect("Inset 0,25: innen 0,5 breit, Grenze 0,5 (WL AmountRange)", lage && std::abs(p.betragMax - 0.5F) < 1e-4F);
        expect("Inset: UV der inneren Ecken in der Quellflaeche (0,25 / 0,75)", uv);
        expect("Inset: 12/20/10", netzIst(n, 12, 20, 10, 0));
    }
    {
        PolyNetz n = kasten();
        const int oben = vieleckBei(n, 2, 1.0F);
        Plan p = bevel(n, {oben}, ExtrudeArt::Gruppe, 1.0F);
        betragAnwenden(n, p, -0.25F);
        float maxX = 0.0F, maxZ = 0.0F;
        for (const PPunkt& q : n.punkte) {
            if (q.ort[2] > 1.5F) { maxX = std::max(maxX, q.ort[0]); }
            maxZ = std::max(maxZ, q.ort[2]);
        }
        expect("Bevel Hoehe 1, Umriss -0,25: Deckel auf 2, halbe Breite 0,25", !p.abgelehnt() && std::abs(maxZ - 2.0F) < 1e-4F && std::abs(maxX - 0.25F) < 1e-4F && netzIst(n, 12, 20, 10, 0));
    }
}

void chamferTest() {
    std::printf("Chamfer, Connect, Loop Cut, Cut\n");
    {
        PolyNetz n = kasten();
        Plan p = chamferKanten(n, {kanteZwischen(n, 0.5F, 0.5F, 0.0F, 0.5F, 0.5F, 1.0F)});
        betragAnwenden(n, p, 0.1F);
        expect("Chamfer eine Kante: 10/15/7", !p.abgelehnt() && p.auswahl.flaechen.size() == 1U && netzIst(n, 10, 15, 7, 0));
    }
    {
        PolyNetz n = kasten();
        std::vector<std::pair<int, int>> alle;
        for (const PKante& k : n.kanten) { alle.emplace_back(k.a, k.b); }
        Plan p = chamferKanten(n, alle);
        betragAnwenden(n, p, 0.1F);
        expect("Chamfer alle zwoelf Kanten: 24/48/26, Grenze 0,5 (WL: der Wuerfel stoppt bei 0,5)",
               !p.abgelehnt() && std::abs(p.betragMax - 0.5F) < 1e-4F && netzIst(n, 24, 48, 26, 0));
    }
    {
        PolyNetz n = kasten();
        Plan p = chamferPunkte(n, {punktBei(n, 0.5F, 0.5F, 1.0F)});
        betragAnwenden(n, p, 0.2F);
        expect("Chamfer Punkt: 10/15/7, Grenze 1 (Kantenlaenge)", !p.abgelehnt() && std::abs(p.betragMax - 1.0F) < 1e-4F && netzIst(n, 10, 15, 7, 0));
    }
    {
        PolyNetz n = kasten();
        Plan p = chamferKanten(n, {kanteZwischen(n, 0.5F, 0.5F, 0.0F, 0.5F, 0.5F, 1.0F)});
        PolyNetz r = kasten();
        const int oben = vieleckBei(r, 2, 1.0F);
        (void)loeschen(r, PAuswahl{{}, {}, {oben}}, PEbene::Vieleck);
        r.verdichten();
        Plan pr = chamferKanten(r, {kanteZwischen(r, 0.5F, 0.5F, 1.0F, -0.5F, 0.5F, 1.0F)});
        expect("Chamfer an einer Randkante abgelehnt (WL BorderEdgeChamfer)", !p.abgelehnt() && pr.grund == Ablehnung::Rand);
    }
    {
        PolyNetz n = kasten();
        Plan p = connectKanten(n, {kanteZwischen(n, -0.5F, -0.5F, 1.0F, 0.5F, -0.5F, 1.0F), kanteZwischen(n, -0.5F, 0.5F, 1.0F, 0.5F, 0.5F, 1.0F)}, 1);
        betragAnwenden(n, p, 0.0F);
        expect("Connect zwei Kanten des Deckels: 10/15/7, eine neue Kante", !p.abgelehnt() && p.auswahl.kanten.size() == 1U && netzIst(n, 10, 15, 7, 0));
    }
    {
        PolyNetz n = kasten();
        Plan p = connectKanten(n, {kanteZwischen(n, -0.5F, -0.5F, 1.0F, 0.5F, -0.5F, 1.0F), kanteZwischen(n, -0.5F, 0.5F, 1.0F, 0.5F, 0.5F, 1.0F)}, 3);
        betragAnwenden(n, p, 0.5F);
        expect("Connect 3 Segmente: 14/21/9, Slide in [-1, 1]", !p.abgelehnt() && p.betragMin == -1.0F && p.betragMax == 1.0F && netzIst(n, 14, 21, 9, 0));
    }
    {
        PolyNetz n = kasten();
        Plan p = connectPunkte(n, {punktBei(n, -0.5F, -0.5F, 1.0F), punktBei(n, 0.5F, 0.5F, 1.0F)});
        expect("Connect Punkte (Diagonale des Deckels): 8/13/7", !p.abgelehnt() && netzIst(n, 8, 13, 7, 0));
    }
    {
        PolyNetz n = kasten();
        Plan p = loopCut(n, kanteZwischen(n, 0.5F, -0.5F, 0.0F, 0.5F, -0.5F, 1.0F), 1);
        betragAnwenden(n, p, 0.0F);
        bool mitte = true;
        for (const PPunkt& q : n.punkte) { mitte = mitte && (q.ort[2] == 0.0F || q.ort[2] == 1.0F || std::abs(q.ort[2] - 0.5F) < 1e-5F); }
        expect("Loop Cut um den Kasten: 12/20/10, Schnitt auf halber Hoehe", !p.abgelehnt() && mitte && netzIst(n, 12, 20, 10, 0));
    }
    {
        PolyNetz n = kasten();
        Plan p = loopCut(n, kanteZwischen(n, 0.5F, -0.5F, 0.0F, 0.5F, -0.5F, 1.0F), 2);
        expect("Loop Cut 2 Schnitte: 16/28/14", !p.abgelehnt() && netzIst(n, 16, 28, 14, 0));
    }
    {
        PolyNetz n = kasten();
        SchnittPunkt a;
        a.kante = kanteZwischen(n, -0.5F, -0.5F, 1.0F, 0.5F, -0.5F, 1.0F);
        a.t = 0.5F;
        SchnittPunkt b;
        b.kante = kanteZwischen(n, -0.5F, 0.5F, 1.0F, 0.5F, 0.5F, 1.0F);
        b.t = 0.25F;
        Plan p = schnitt(n, a, b);
        expect("Cut ueber den Deckel (Kante zu Kante): 10/15/7", !p.abgelehnt() && netzIst(n, 10, 15, 7, 0));
        SchnittPunkt c;
        c.punkt = p.auswahl.punkte.empty() ? -1 : p.auswahl.punkte[0];
        SchnittPunkt d;
        d.punkt = punktBei(n, -0.5F, -0.5F, 0.0F);
        const Plan q = schnitt(n, c, d);
        expect("Cut weiter vom letzten Punkt: ohne gemeinsames Vieleck abgelehnt", q.grund == Ablehnung::NichtEinVieleck);
    }
}

void zusammenTest() {
    std::printf("Weld, Target Weld, Merge, Bridge\n");
    {
        PolyNetz n = leer();
        GrundParam g;
        g.form = Grundform::Ebene;
        g.laenge = 2.0F;
        g.breite = 1.0F;
        g.segL = 2;
        (void)grundkoerper(n, g, 0);
        n.verdichten();
        Plan d = detach(n, {0}, -1);
        n.verdichten();
        const bool getrennt = !d.abgelehnt() && n.lebendePunkte() == 8;
        std::vector<int> alle;
        for (std::size_t i = 0; i < n.punkte.size(); ++i) { alle.push_back(static_cast<int>(i)); }
        Plan w = weld(n, alle, weldSchwelleVorgabe(n));
        expect("Detach als Element 8 Punkte, Weld mit Vorgabe-Schwelle zurueck: 6/7/2", getrennt && !w.abgelehnt() && netzIst(n, 6, 7, 2, 6));
    }
    {
        PolyNetz n = kasten();
        Plan p = targetWeld(n, punktBei(n, 0.5F, 0.5F, 1.0F), punktBei(n, -0.5F, 0.5F, 1.0F));
        const int z = punktBei(n, -0.5F, 0.5F, 1.0F);
        expect("Target Weld: 7/11/6, der Zielpunkt bleibt", !p.abgelehnt() && z >= 0 && netzIst(n, 7, 11, 6, 0));
    }
    {
        PolyNetz n = kasten();
        Plan p = mergen(n, {punktBei(n, 0.5F, 0.5F, 1.0F), punktBei(n, -0.5F, 0.5F, 1.0F)}, MergeArt::Mitte);
        expect("Merge Mitte (Kante des Deckels): 7/11/6, Punkt bei x 0", !p.abgelehnt() && punktBei(n, 0.0F, 0.5F, 1.0F) >= 0 && netzIst(n, 7, 11, 6, 0));
    }
    {
        PolyNetz n = kasten();
        GrundParam g;
        g.mitte[0] = 3.0F;
        g.laenge = g.breite = g.hoehe = 1.0F;
        (void)grundkoerper(n, g, 0);
        n.verdichten();
        Plan p = bridgeFlaechen(n, vieleckBei(n, 0, 0.5F), vieleckBei(n, 0, 2.5F), 1, 0);
        expect("Bridge zweier Kaesten (Seite an Seite): 16/28/14, geschlossen", !p.abgelehnt() && netzIst(n, 16, 28, 14, 0));
        PolyNetz m = kasten();
        const Plan q = bridgeFlaechen(m, vieleckBei(m, 2, 1.0F), vieleckBei(m, 2, 0.0F), 1, 0);
        expect("Bridge Deckel und Boden eines Kastens abgelehnt: die Verbindungen sind schon Kanten (WL WouldPinch)", q.grund == Ablehnung::WuerdeKneifen && m.lebendeFlaechen() == 6);
    }
    {
        PolyNetz n = kasten();
        const int oben = vieleckBei(n, 2, 1.0F), unten = vieleckBei(n, 2, 0.0F);
        (void)loeschen(n, PAuswahl{{}, {}, {oben, unten}}, PEbene::Vieleck);
        n.verdichten();
        Plan p = bridgeRaender(n, kanteZwischen(n, -0.5F, -0.5F, 1.0F, 0.5F, -0.5F, 1.0F), kanteZwischen(n, -0.5F, -0.5F, 0.0F, 0.5F, -0.5F, 0.0F), 2, 0);
        expect("Bridge Raender mit 2 Segmenten: 12/24/12", !p.abgelehnt() && netzIst(n, 12, 24, 12, 0));
        PolyNetz m = kasten();
        (void)loeschen(m, PAuswahl{{}, {}, {vieleckBei(m, 2, 1.0F)}}, PEbene::Vieleck);
        m.verdichten();
        const Plan q = bridgeRaender(m, kanteZwischen(m, -0.5F, -0.5F, 1.0F, 0.5F, -0.5F, 1.0F), kanteZwischen(m, 0.5F, 0.5F, 1.0F, -0.5F, 0.5F, 1.0F), 1, 0);
        expect("Bridge derselben Schleife abgelehnt (WL SameLoop)", q.grund == Ablehnung::GleicheSchleife);
    }
}

void entfernenTest() {
    std::printf("Remove, Delete, Detach, Symmetrize, To Quads\n");
    {
        PolyNetz n = kasten();
        PAuswahl a;
        a.kanten = {kanteZwischen(n, -0.5F, -0.5F, 1.0F, 0.5F, -0.5F, 1.0F)};
        Plan p = entfernen(n, a, PEbene::Kante, false);
        expect("Remove Kante: 8/11/5", !p.abgelehnt() && netzIst(n, 8, 11, 5, 0));
    }
    {
        PolyNetz n = kasten();
        PAuswahl a;
        a.punkte = {punktBei(n, 0.5F, 0.5F, 1.0F)};
        Plan p = entfernen(n, a, PEbene::Punkt, false);
        expect("Remove Punkt (drei Vielecke werden eines): 7/9/4", !p.abgelehnt() && netzIst(n, 7, 9, 4, 0));
    }
    {
        PolyNetz n = kasten(2.0F, 2);
        PAuswahl a;
        a.kanten = {kanteZwischen(n, 0.0F, -1.0F, 2.0F, 0.0F, 0.0F, 2.0F), kanteZwischen(n, 0.0F, 0.0F, 2.0F, 0.0F, 1.0F, 2.0F)};
        PolyNetz ohne = n;
        Plan q = entfernen(ohne, a, PEbene::Kante, false);
        Plan p = entfernen(n, a, PEbene::Kante, true);
        expect("Remove zwei Kanten ohne Aufraeumen: 26/46/22", !q.abgelehnt() && netzIst(ohne, 26, 46, 22, 0));
        expect("Remove mit Aufraeumen (Strg+Backspace): der Punkt mit zwei Kanten geht mit, 25/45/22", !p.abgelehnt() && netzIst(n, 25, 45, 22, 0));
    }
    {
        PolyNetz n = kasten();
        const int oben = vieleckBei(n, 2, 1.0F);
        Plan p = loeschen(n, PAuswahl{{}, {}, {oben}}, PEbene::Vieleck);
        expect("Delete Deckel: 8/12/5, vier Randkanten", !p.abgelehnt() && netzIst(n, 8, 12, 5, 4));
        PolyNetz m = kasten();
        std::vector<int> alle;
        for (int i = 0; i < 6; ++i) { alle.push_back(i); }
        expect("Delete aller Vielecke abgelehnt (WL AllFaces)", loeschen(m, PAuswahl{{}, {}, alle}, PEbene::Vieleck).grund == Ablehnung::AlleFlaechen);
    }
    {
        PolyNetz n = kasten();
        Plan p = detach(n, {vieleckBei(n, 2, 1.0F)}, -1);
        expect("Detach als Element: 12/16/6, acht Randkanten", !p.abgelehnt() && netzIst(n, 12, 16, 6, 8));
        PolyNetz m = kasten();
        Plan q = detach(m, {vieleckBei(m, 2, 1.0F)}, 1);
        m.verdichten();
        int teil1 = 0;
        for (const PFlaeche& f : m.flaechen) { teil1 += f.teil == 1 ? 1 : 0; }
        expect("Detach in einen neuen Teil: ein Vieleck im Teil 1, 12 Punkte", !q.abgelehnt() && teil1 == 1 && m.lebendePunkte() == 12 && m.teilBeruehrt.size() == 2U);
    }
    {
        PolyNetz n = kasten(2.0F, 2);
        // die rechte Seite verschoben: nach Symmetrize von + nach - ist die linke ihr Spiegel
        const int p0 = punktBei(n, 1.0F, 1.0F, 2.0F);
        n.punkte[uz(p0)].ort[0] = 1.5F;
        SymmetrieParam sp;
        sp.achse = 0;
        Plan p = symmetrisieren(n, sp, {});
        n.verdichten();
        const bool spiegel = punktBei(n, -1.5F, 1.0F, 2.0F) >= 0 && punktBei(n, -1.0F, 1.0F, 2.0F) < 0;
        if (!spiegel || p.abgelehnt()) { std::printf("        sym: grund %d spiegel %d %s\n", static_cast<int>(p.grund), spiegel ? 1 : 0, zahlen(n).c_str()); }
        expect("Symmetrize X: Spiegel der verschobenen Ecke, 26/48/24 geschlossen", !p.abgelehnt() && spiegel && netzIst(n, 26, 48, 24, 0));
        PolyNetz m = kasten(2.0F, 1);
        for (PPunkt& q : m.punkte) { q.ort[0] -= 5.0F; }
        expect("Symmetrize ohne Quellseite abgelehnt (WL NoSourceSide)", symmetrisieren(m, sp, {}).grund == Ablehnung::KeineQuelle);
    }
    {
        // Kasten -> .md3 -> gelesen: zwoelf Dreiecke; To Quads macht wieder sechs Vierecke mit denselben Dreiecken
        PolyNetz n = kasten();
        Inhalt in = inhaltMit(n);
        std::string bytes;
        Md3SchreibBericht br;
        const bool ok = md3Schreiben(in, bytes, 1000, &br);
        Md3Model m;
        std::string err;
        const bool gelesen = ok && readMd3(bytes, m, &err);
        Inhalt aus;
        if (gelesen) {
            ausMd3(m, aus, 0);
            aus.md3Datei = std::make_shared<const std::string>(bytes);
        }
        PolyNetz d = polyAus(aus, 0);
        const int dreiecke = d.lebendeFlaechen();
        const std::vector<std::uint32_t> vorher = aus.teile.empty() ? std::vector<std::uint32_t>{} : aus.teile[0].stufen[0].indizes();
        Plan q = zuVierecken(d, {}, QuadParam{});
        if (!gelesen || dreiecke != 12) { std::printf("        md3: ok %d gelesen %d Flaechen %zu Dreiecke %d Ecken %d %s %s\n", ok ? 1 : 0, gelesen ? 1 : 0, m.surfaces.size(), dreiecke, br.ecken, err.c_str(), br.fehler.c_str()); }
        expect("MD3 neu geschrieben: 1 Flaeche, 24 Ecken, 12 Dreiecke", gelesen && m.surfaces.size() == 1U && dreiecke == 12 && br.ecken == 24);
        expect("To Quads: 12 Dreiecke -> 6 Vierecke", !q.abgelehnt() && netzIst(d, 8, 12, 6, 0));
        polyZurueck(d, aus);
        std::vector<std::uint32_t> nachher = aus.teile[0].stufen[0].indizes();
        // dieselben Dreiecke (als Mengen von Orten)
        const auto orte = [&](const std::vector<std::uint32_t>& idx) {
            std::set<std::vector<int>> s;
            for (std::size_t t = 0; t + 2 < idx.size(); t += 3) {
                std::vector<int> tri;
                for (int k = 0; k < 3; ++k) {
                    const float* o = aus.teile[0].stufen[0].ecken[idx[t + uz(k)]].ort;
                    tri.push_back(static_cast<int>(std::lround(o[0] * 100)) * 1000000 + static_cast<int>(std::lround(o[1] * 100)) * 1000 + static_cast<int>(std::lround(o[2] * 100)));
                }
                std::sort(tri.begin(), tri.end());
                s.insert(tri);
            }
            return s;
        };
        expect("To Quads behaelt die Diagonalen (dieselben 12 Dreiecke)", vorher.size() == nachher.size() && orte(nachher).size() == 12U && aus.teile[0].stufen[0].polyVon.size() == 12U);
        // Rundlauf der gelesenen .md3 ohne Aenderung: Byte fuer Byte
        std::string wieder;
        Inhalt roh;
        ausMd3(m, roh, 0);
        roh.md3Datei = std::make_shared<const std::string>(bytes);
        expect("MD3 Rundlauf ohne Aenderung Byte fuer Byte", md3Schreiben(roh, wieder, 1000) && wieder == bytes);
        // ein Punkt bewegt: kommt an, Kasten der Bilder neu
        PolyNetz e = polyAus(roh, 0);
        const int pt = punktBei(e, 0.5F, 0.5F, 1.0F);
        e.punkte[uz(pt)].ort[2] = 3.0F;
        e.beruehren(0);
        polyZurueck(e, roh);
        std::string bewegt;
        Md3Model m2;
        const bool b2 = md3Schreiben(roh, bewegt, 1000) && readMd3(bewegt, m2, &err);
        float maxZ = 0.0F;
        if (b2) {
            for (int v = 0; v < m2.surfaces[0].numVerts; ++v) { maxZ = std::max(maxZ, m2.surfaces[0].verts[uz(v)].xyz[2]); }
        }
        expect("MD3: bewegter Punkt geschrieben (z 3)", b2 && std::abs(maxZ - 3.0F) < 1e-3F);
    }
}

void grenzenTest(const std::string& glmPfad) {
    std::printf("JKA-Grenzen, Rundlauf, Umlauf, Gewichte\n");
    {
        PolyNetz n = leer();
        GrundParam g;
        g.form = Grundform::Ebene;
        g.laenge = g.breite = 64.0F;
        g.segL = g.segB = 32;
        (void)grundkoerper(n, g, 0);
        n.verdichten();
        Inhalt in = inhaltMit(n, std::string(70, 'x'));
        const std::vector<TeilGrenze> gr = grenzenPruefen(in, 0);
        const Grenzen& j = grenzen(Zielspiel::JkaSp);
        std::string bytes;
        Md3SchreibBericht br;
        const bool geschrieben = md3Schreiben(in, bytes, j.eckenJeFlaeche, &br);
        expect("Grenze: 1089 Ecken >= 1000 gemeldet (SHADER_MAX_VERTEXES), .md3 nicht geschrieben",
               gr.size() == 1U && gr[0].mass.ecken == 1089 && !pruefeMass(gr[0].mass, j).empty() && !geschrieben);
        expect("Grenze: Name 70 Zeichen zu lang (MAX_QPATH 64)", gr.size() == 1U && gr[0].nameZuLang);
        expect("Ecken vor dem Zurueckschreiben gezaehlt (1089)", eckenNachZurueck(n, 0) == 1089);
    }
    {
        // Gewichte beim Teilen: Knochen 1 und 2 gemischt, Summe 1 (WL BlendInfluences, vier je Ecke)
        PolyNetz n = leer();
        GrundParam g;
        g.form = Grundform::Ebene;
        (void)grundkoerper(n, g, 0);
        n.verdichten();
        for (PFlaeche& f : n.flaechen) {
            for (PEcke& e : f.ecken) {
                const bool links = n.punkte[uz(e.punkt)].ort[0] < 0.0F;
                e.e.knochen[0] = static_cast<std::uint8_t>(links ? 1 : 2);
                e.e.gewicht[0] = 1.0F;
            }
        }
        Plan p = connectKanten(n, {kanteZwischen(n, -32.0F, -32.0F, 0.0F, 32.0F, -32.0F, 0.0F), kanteZwischen(n, -32.0F, 32.0F, 0.0F, 32.0F, 32.0F, 0.0F)}, 1);
        bool ok = !p.abgelehnt();
        for (const PFlaeche& f : n.flaechen) {
            for (const PEcke& e : f.ecken) {
                if (std::abs(n.punkte[uz(e.punkt)].ort[0]) > 1e-4F) { continue; }
                float s = 0.0F;
                for (int k = 0; k < 4; ++k) { s += e.e.gewicht[k]; }
                ok = ok && std::abs(s - 1.0F) < 1e-5F && std::abs(e.e.gewicht[0] - 0.5F) < 1e-5F && std::abs(e.e.gewicht[1] - 0.5F) < 1e-5F;
            }
        }
        expect("Gewichte der neuen Ecke: 0,5 / 0,5 auf Knochen 1 und 2", ok);
    }
    const std::string bytes = slurp(glmPfad);
    Inhalt in;
    if (!ausGlm(bytes, in)) {
        expect("model.glm gelesen", false);
        return;
    }
    {
        PolyNetz n = polyAus(in, 0);
        // Umlauf: die Normale nach aussen (gegen Newell, im Uhrzeigersinn) zeigt wie die Normalen der Ecken
        int gut = 0, alle = 0;
        for (const PFlaeche& f : n.flaechen) {
            double nx = 0, ny = 0, nz = 0;
            const std::size_t k = f.ecken.size();
            for (std::size_t i = 0; i < k; ++i) {
                const float* a = n.punkte[uz(f.ecken[i].punkt)].ort;
                const float* b = n.punkte[uz(f.ecken[(i + 1) % k].punkt)].ort;
                nx += (a[1] - b[1]) * (a[2] + b[2]);
                ny += (a[2] - b[2]) * (a[0] + b[0]);
                nz += (a[0] - b[0]) * (a[1] + b[1]);
            }
            double sx = 0, sy = 0, sz = 0;
            for (const PEcke& e : f.ecken) {
                sx += e.e.normale[0];
                sy += e.e.normale[1];
                sz += e.e.normale[2];
            }
            ++alle;
            gut += -(nx * sx + ny * sy + nz * sz) > 0.0 ? 1 : 0;
        }
        std::printf("        Umlauf: %d von %d Vielecken mit der Normale der Ecken\n", gut, alle);
        expect("JKA-Figur: Umlauf im Uhrzeigersinn von aussen (ueber 95 % der Vielecke)", alle > 0 && gut * 100 > alle * 95);
        // ohne Aenderung: nichts beruehrt, Byte fuer Byte
        Inhalt x = in;
        polyZurueck(n, x);
        std::string aus;
        expect("GLM Rundlauf ueber das Arbeitsnetz ohne Aenderung Byte fuer Byte", glmSchreiben(x, aus) && aus == bytes);
        // alle Teile neu geschrieben: dieselben Dreiecke und Ecken je Teil
        Inhalt y = in;
        PolyNetz m = polyAus(y, 0);
        std::fill(m.teilBeruehrt.begin(), m.teilBeruehrt.end(), 1);
        polyZurueck(m, y);
        bool gleich = true;
        for (std::size_t t = 0; t < in.teile.size(); ++t) {
            if (in.teile[t].istTag() || in.teile[t].stufen.empty()) { continue; }
            gleich = gleich && in.teile[t].stufen[0].dreiecke() == y.teile[t].stufen[0].dreiecke() && in.teile[t].stufen[0].ecken.size() >= y.teile[t].stufen[0].ecken.size();
        }
        std::string aus2;
        Inhalt z;
        const bool wieder = glmSchreiben(y, aus2) && ausGlm(aus2, z);
        expect("GLM ueber das Arbeitsnetz neu geschrieben: gleiche Dreiecke je Flaeche, wieder lesbar", gleich && wieder);
        // ein Inset auf der Figur: Gewichte bleiben vier je Ecke, Summe 1; Tags unberuehrt
        PolyNetz w = polyAus(in, 0);
        Plan p = inset(w, {0, 1, 2}, InsetArt::Gruppe);
        betragAnwenden(w, p, 0.2F);
        normalenNeu(w);
        Inhalt v = in;
        polyZurueck(w, v);
        bool gew = true, tags = true;
        for (std::size_t t = 0; t < v.teile.size(); ++t) {
            if (v.teile[t].istTag()) {
                tags = tags && v.teile[t].stufen[0].dreiecke() == 1;
                continue;
            }
            for (const Ecke& e : v.teile[t].stufen[0].ecken) {
                float s = 0.0F;
                for (int k = 0; k < 4; ++k) { s += e.gewicht[k]; }
                gew = gew && std::abs(s - 1.0F) < 1e-3F;
            }
        }
        std::string aus3;
        GlmSchreibBericht gb;
        expect("Inset auf der JKA-Figur: Gewichte Summe 1, Tags ein Dreieck, .glm geschrieben", !p.abgelehnt() && gew && tags && glmSchreiben(v, aus3, &gb));
        // Grenzen der Figur: kein Tag mit mehr als einem Dreieck
        bool tagOk = true;
        for (const TeilGrenze& g : grenzenPruefen(in, 0)) { tagOk = tagOk && !g.tagKeinDreieck; }
        expect("JKA-Figur: Tags sind je ein Dreieck", tagOk);
    }
}

}  // namespace

// p_jka: Max Make Planar - die Punkte in einer Ebene (Normale gemittelt, durch ihre Mitte), UV unveraendert
// Abstand des vierten Punkts von der Ebene der ersten drei (-1: die drei liegen auf einer Linie)
double abstandVierter(const PolyNetz& n, const std::vector<int>& pk) {
    const auto o = [&](std::size_t i) { return n.punkte[uz(pk[i])].ort; };
    const double ux = o(1)[0] - o(0)[0], uy = o(1)[1] - o(0)[1], uzz = o(1)[2] - o(0)[2];
    const double vx = o(2)[0] - o(0)[0], vy = o(2)[1] - o(0)[1], vz = o(2)[2] - o(0)[2];
    const double nx = uy * vz - uzz * vy, ny = uzz * vx - ux * vz, nz = ux * vy - uy * vx;
    const double l = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (!(l > 0.0)) { return -1.0; }
    return std::abs((o(3)[0] - o(0)[0]) * nx + (o(3)[1] - o(0)[1]) * ny + (o(3)[2] - o(0)[2]) * nz) / l;
}

void ebenTest() {
    std::printf("Make Planar\n");
    {
        PolyNetz n = kasten();
        const int oben = vieleckBei(n, 2, 1.0F);
        n.punkte[uz(punktBei(n, 0.5F, 0.5F, 1.0F))].ort[2] = 1.4F;   // eine Ecke des Deckels angehoben
        std::vector<int> pk;
        std::vector<std::array<float, 2>> uvVorher;
        for (const PEcke& e : n.flaechen[uz(oben)].ecken) {
            pk.push_back(e.punkt);
            uvVorher.push_back({e.e.uv[0], e.e.uv[1]});
        }
        const double vorher = abstandVierter(n, pk);
        const Plan p = ebenMachen(n, PAuswahl{{}, {}, {oben}}, PEbene::Vieleck);
        const double nachher = abstandVierter(n, pk);
        double mz = 0.0;
        bool uvGleich = true;
        for (std::size_t i = 0; i < pk.size(); ++i) {
            mz += static_cast<double>(n.punkte[uz(pk[i])].ort[2]) / 4.0;
            uvGleich = uvGleich && n.flaechen[uz(oben)].ecken[i].e.uv[0] == uvVorher[i][0] && n.flaechen[uz(oben)].ecken[i].e.uv[1] == uvVorher[i][1];
        }
        std::printf("        Abstand vorher %.4f nachher %.6f, Mitte z %.4f\n", vorher, nachher, mz);
        expect("Make Planar am Deckel (eine Ecke 0,4 hoch): vier Ecken in einer Ebene, Mitte bleibt 1,1, UV gleich",
               !p.abgelehnt() && p.geaendert == 4 && vorher > 0.1 && nachher >= 0.0 && nachher < 1e-4 && std::abs(mz - 1.1) < 1e-4 && uvGleich &&
                   netzIst(n, 8, 12, 6, 0));
    }
    {
        PolyNetz n = kasten();
        // vier Punkte der Seite y -0,5, einer 0,3 heraus: die gemittelte Normale der Vielecke an den Punkten
        const std::vector<int> pk = {punktBei(n, -0.5F, -0.5F, 0.0F), punktBei(n, 0.5F, -0.5F, 0.0F), punktBei(n, 0.5F, -0.5F, 1.0F), punktBei(n, -0.5F, -0.5F, 1.0F)};
        n.punkte[uz(pk[2])].ort[1] = -0.8F;
        const Plan p = ebenMachen(n, PAuswahl{pk, {}, {}}, PEbene::Punkt);
        const double d = abstandVierter(n, pk);
        std::printf("        Abstand des vierten Punkts %.6f\n", d);
        expect("Make Planar an vier Punkten der Seite (einer 0,3 heraus): eine Ebene", !p.abgelehnt() && d >= 0.0 && d < 1e-4);
        const Plan q = ebenMachen(n, PAuswahl{{pk[0], pk[1]}, {}, {}}, PEbene::Punkt);
        expect("Make Planar mit zwei Punkten abgelehnt (leere Auswahl)", q.grund == Ablehnung::LeereAuswahl);
    }
}

// p_jka: Max Slice - das ganze Modell oder die gewaehlten Vielecke an einer Ebene quer zur Achse geteilt
void schnittTest() {
    std::printf("Slice\n");
    {
        PolyNetz n = kasten();
        const Plan p = schneiden(n, {}, 2, 0.25F);
        // vier Seiten je zweigeteilt: 8 + 4 Punkte, 12 + 4 + 4 Kanten, 6 + 4 Vielecke, jeder neue Punkt auf z 0,25
        bool aufEbene = true;
        int neu = 0;
        for (const PPunkt& q : n.punkte) {
            if (q.weg || std::abs(q.ort[2]) < 1e-6F || std::abs(q.ort[2] - 1.0F) < 1e-6F) { continue; }
            ++neu;
            aufEbene = aufEbene && q.ort[2] == 0.25F;
        }
        expect("Slice des Kastens bei z 0,25: 12/20/10, vier neue Punkte genau auf der Ebene, 4 Teilungen",
               !p.abgelehnt() && p.geaendert == 4 && neu == 4 && aufEbene && netzIst(n, 12, 20, 10, 0));
    }
    {
        PolyNetz n = kasten();
        const int vorn = vieleckBei(n, 1, -0.5F);
        const Plan p = schneiden(n, {vorn}, 0, 0.0F);
        // nur die Vorderseite geteilt; die neuen Punkte auf Ober- und Unterkante sitzen auch im Deckel und Boden (keine
        // T-Stelle): 10 Punkte, 15 Kanten, 7 Vielecke
        expect("Slice nur der gewaehlten Vorderseite bei x 0: 10/15/7, eine Teilung", !p.abgelehnt() && p.geaendert == 1 && netzIst(n, 10, 15, 7, 0));
    }
    {
        PolyNetz n = kasten();
        const Plan p = schneiden(n, {}, 2, 3.0F);
        expect("Slice ausserhalb des Modells: abgelehnt (WL NothingToSlice), 8/12/6", p.grund == Ablehnung::NichtsGeschnitten && netzIst(n, 8, 12, 6, 0));
    }
    {
        // QuickSlice-Ebene schraeg: durch (0, 0, 0,4), Normale (0, 1, 1) - jeder neue Punkt liegt auf ihr
        PolyNetz n = kasten();
        const float o[3] = {0.0F, 0.0F, 0.4F}, nn[3] = {0.0F, 1.0F, 1.0F};
        const std::size_t vorher = n.punkte.size();
        const Plan p = schneidenEbene(n, {}, o, nn);
        double groesste = 0.0;
        for (std::size_t i = vorher; i < n.punkte.size(); ++i) {
            const float* q = n.punkte[i].ort;
            groesste = std::max(groesste, std::abs((static_cast<double>(q[1]) + static_cast<double>(q[2]) - 0.4) / std::sqrt(2.0)));
        }
        std::printf("        schraeg: %zu neue Punkte, groesster Abstand %.2e\n", n.punkte.size() - vorher, groesste);
        expect("Slice schraeg (Normale 0 1 1): neue Punkte auf der Ebene, Netz gueltig, geteilt",
               !p.abgelehnt() && p.geaendert > 0 && n.punkte.size() > vorher && groesste < 1e-5 && netzIst(n, n.lebendePunkte(), n.kantenZahl(), n.lebendeFlaechen(), 0));
    }
}

// p_jka: Max Hinge From Edge (WL PlanHingeFaces) - der Deckel um seine vordere Kante
void hingeTest() {
    std::printf("Hinge\n");
    {
        PolyNetz n = kasten();
        const int oben = vieleckBei(n, 2, 1.0F);
        const auto kanten = hingeKanten(n, {oben});
        const std::pair<int, int> k = kantenPaar(punktBei(n, -0.5F, -0.5F, 1.0F), punktBei(n, 0.5F, -0.5F, 1.0F));
        const Plan p = hinge(n, {oben}, k, 90.0F);
        // zwei Kopien, an der Scharnierkante keine Wand, daneben zwei Dreiecke, gegenueber ein Viereck: 10/17/9; um 90 Grad
        // steht der Deckel senkrecht, seine freie Kante bei z 2 ueber dem Scharnier
        expect("Hinge des Deckels um die vordere Kante 90 Grad: 4 moegliche Scharniere, 10/17/9, freie Kante bei y -0,5 z 2",
               kanten.size() == 4U && !p.abgelehnt() && punktBei(n, -0.5F, -0.5F, 2.0F) >= 0 && punktBei(n, 0.5F, -0.5F, 2.0F) >= 0 &&
                   netzIst(n, 10, 17, 9, 0));
    }
    {
        PolyNetz n = kasten();
        const int oben = vieleckBei(n, 2, 1.0F);
        const Plan p = hinge(n, {oben}, kantenPaar(punktBei(n, -0.5F, -0.5F, 0.0F), punktBei(n, 0.5F, -0.5F, 0.0F)), 45.0F);
        expect("Hinge um eine Kante, die nicht am Rand der Auswahl liegt: abgelehnt (WL HingeNotOnBoundary), 8/12/6",
               p.grund == Ablehnung::HingeNichtAmRand && netzIst(n, 8, 12, 6, 0));
    }
}

// p_jka: Max Collapse (Ctrl+Alt+C) = mergen(Mitte) ueber die Punkte der Auswahl
void collapseTest() {
    std::printf("Collapse\n");
    PolyNetz n = kasten();
    const int oben = vieleckBei(n, 2, 1.0F);
    const Plan p = mergen(n, auswahlPunkte(n, PAuswahl{{}, {}, {oben}}), MergeArt::Mitte);
    expect("Collapse des Deckels: eine Spitze bei (0, 0, 1), 5/8/5", !p.abgelehnt() && punktBei(n, 0.0F, 0.0F, 1.0F) >= 0 && netzIst(n, 5, 8, 5, 0));
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Aufruf: modellmeshtest <model.glm>\n");
        return 2;
    }
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    grundkoerperTest();
    extrudeTest();
    chamferTest();
    zusammenTest();
    entfernenTest();
    ebenTest();
    schnittTest();
    hingeTest();
    collapseTest();
    grenzenTest(argv[1]);
    std::printf("%s (%d Fehler)\n", fails == 0 ? "OK" : "FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
