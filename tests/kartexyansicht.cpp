// kartexyansicht.cpp - Karten-Editor Stufe 1b, Punkt 5: die 2D-Ansichten
// rechnen wie NetRadiant-Custom (bhed/karte/xyansicht.h).
//
// Sollwerte aus radiant/xywindow.cpp (XY_DrawGrid-Beispiele der Portier-
// Spezifikation), XY_ToPoint, SetScale, ZoomCompensateOrigin,
// FocusOnBounds, update_wireframe, arrow_draw, angles.h,
// XYWnd_OrientCamera und TargetLinesPushBack.
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"
#include "bhed/karte/xyansicht.h"

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
bool nah(float a, float b, float eps = 1e-3F) { return std::fabs(a - b) <= eps; }

void raster() {
    XyAnsicht v;
    v.breite = 800;
    v.hoehe = 600;
    RasterEinstellungen e;
    e.raster = 16;
    e.fang = 16;
    e.potenz = 4;
    v.massstab = 1;
    RasterZeichnung r = rasterZeichnen(v, e);
    pruefe(r.kleinSchritt == 16 && r.grossSchritt == 64 && r.maske == 3 && r.textSchrittX == 64,
           "Raster 16, Massstab 1: klein 16, gross 64, Maske 3, Beschriftung 64");
    v.massstab = 0.25F;
    r = rasterZeichnen(v, e);
    pruefe(r.kleinSchritt == 32 && r.grossSchritt == 512 && r.maske == 15 && r.textSchrittX == 256,
           "Raster 16, Massstab 0.25: klein 32, gross 512, Maske 15, Beschriftung 256");
    e.raster = 8;
    e.fang = 8;
    e.potenz = 3;
    v.massstab = 5;
    r = rasterZeichnen(v, e);
    pruefe(r.kleinSchritt == 8 && r.grossSchritt == 8 && r.maske == 0, "Raster 8, Massstab 5: nur grosse Linien alle 8");
    e.fang = 0;
    r = rasterZeichnen(v, e);
    pruefe(nah(r.alpha, 0.3F), "Fang aus: Raster blass (Alpha 0.3)");
    // Linien: bei Raster 16 / Massstab 1 und 800 x 600 um (0,20): grosse
    // Linien x = -448 .. 448 (xb = 64 * floor(-400/64) = -448, xe = 448).
    e.raster = 16;
    e.fang = 16;
    e.potenz = 4;
    v.massstab = 1;
    v.ursprung = Vektor3f(0, 0, 0);
    r = rasterZeichnen(v, e);
    std::size_t gross = 0;
    for (const Linie& l : r.linien) {
        if ((l.farbe & 0xFFFFFFU) == (farbeAus(e.gross[0], e.gross[1], e.gross[2]) & 0xFFFFFFU)) { ++gross; }
    }
    // senkrecht -448..448 in 64er-Schritten = 15, waagerecht -320..320 = 11
    pruefe(gross == 26, "grosse Linien: 15 senkrecht + 11 waagerecht (ist " + std::to_string(gross) + ")");
    // Achsenkreuz: 4 breite Linien, 4 Buchstaben
    std::size_t buchstaben = 0;
    for (const Text& t : r.texte) {
        if (t.text.size() == 1 && (t.text == "X" || t.text == "Y")) { ++buchstaben; }
    }
    pruefe(r.linien2.size() == 4 && buchstaben == 4, "Achsenkreuz: 4 Linien Breite 2, X/Y zweimal");
}

void zustand() {
    XyAnsicht v;
    v.breite = 200;
    v.hoehe = 200;
    const Vektor3f p = v.zuPunkt(100, 100);
    pruefe(nah(p[0], 0) && nah(p[1], 20) && p[2] == 0, "XY_ToPoint Mitte: Ursprung (0, 20), Blickachse 0");
    const Vektor3f q = v.zuPunkt(0, 0);
    pruefe(nah(q[0], -100) && nah(q[1], 120), "XY_ToPoint links oben: (-100, 120)");
    const Vektor3f g = v.zuPunkt(7, 93, 16);
    pruefe(g[0] == -96 && g[1] == 32, "XY_ToPoint eingerastet (Raster 16)");
    v.breite = 500;
    v.hoehe = 500;
    v.setzeMassstab(0.0001F);
    pruefe(nah(v.massstab, 500.0F / (2.2F * 65536.0F), 1e-7F), "SetScale: kleinster Massstab = min(W,H) / (2.2 * 65536)");
    v.setzeMassstab(1000);
    pruefe(v.massstab == 64, "SetScale: groesster Massstab 64");
    v.setzeMassstab(1);
    v.ursprung = Vektor3f(0, 0, 0);
    const Vektor3f vorher = v.zuPunkt(400, 100);
    v.hineinMaus(400, 100);
    const Vektor3f nachher = v.zuPunkt(400, 100);
    pruefe(nah(v.massstab, 1.25F) && nah(vorher[0], nachher[0], 0.01F) && nah(vorher[1], nachher[1], 0.01F),
           "ZoomInWithMouse: x 5/4, der Punkt unter dem Zeiger bleibt");
    v.aufGrenzen(Kasten(Vektor3f(10, 20, 30), Vektor3f(64, 300, 8)));
    pruefe(nah(v.massstab, std::min(500.0F / 384.0F, 500.0F / 900.0F)) && v.ursprung == Vektor3f(10, 20, 30),
           "FocusOnBounds: Mitte, Massstab = min(W / 3 max(128, e1), H / 3 max(128, e2))");
    pruefe(naechsteSicht(Sicht::XY) == Sicht::XZ && naechsteSicht(Sicht::XZ) == Sicht::YZ && naechsteSicht(Sicht::YZ) == Sicht::XY,
           "NextView: XY -> XZ -> YZ -> XY");
    // Kamera aus 2D: rechts vom Kameraursprung = yaw 0, darueber = 90
    XyAnsicht w;
    w.breite = 200;
    w.hoehe = 200;
    w.ursprung = Vektor3f(0, 0, 0);
    Vektor3f winkel = kameraDrehen(w, 150, 100, Vektor3f(0, 0, 0), Vektor3f(0, 0, 0));
    pruefe(nah(winkel[1], 0), "OrientCamera: Zeiger rechts -> yaw 0");
    winkel = kameraDrehen(w, 100, 50, Vektor3f(0, 0, 0), Vektor3f(0, 0, 0));
    pruefe(nah(winkel[1], 90), "OrientCamera: Zeiger oben -> yaw 90");
    const Vektor3f o = kameraVersetzen(w, 150, 100, 16, Vektor3f(1, 2, 77));
    pruefe(o == Vektor3f(48, 0, 77), "PositionCamera: eingerastet, Hoehe bleibt (ist " + std::to_string(o[0]) + ")");
}

void szene() {
    // Wuerfel 0..64: in XY nur die Oberseite (4 Kanten), in XZ die Vorderseite.
    Kontext k;
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    Pinsel p;
    p.quaderBauen(Kasten(Vektor3f(32, 32, 32), Vektor3f(32, 32, 32)), "textures/common/caulk", proj, k);
    std::vector<Linie> l;
    pinselKanten(p, k, Sicht::XY, 0xFF000000U, l);
    bool oben = l.size() == 4;
    for (const Linie& x : l) { oben = oben && x.a[2] == 64 && x.b[2] == 64; }
    pruefe(oben, "Pinselkanten XY: nur die 4 Kanten der Oberseite (ist " + std::to_string(l.size()) + ")");
    l.clear();
    pinselKanten(p, k, Sicht::XZ, 0xFF000000U, l);
    bool vorn = l.size() == 4;
    for (const Linie& x : l) { vorn = vorn && x.a[1] == 0 && x.b[1] == 0; }
    pruefe(vorn, "Pinselkanten XZ: die 4 Kanten der Vorderseite (y = 0)");
    l.clear();
    kastenDraht(Kasten(Vektor3f(0, 0, 0), Vektor3f(8, 8, 8)), 0, l);
    pruefe(l.size() == 13, "aabb_draw_wire: 12 Kanten + Diagonale");
    l.clear();
    pfeil(Vektor3f(0, 0, 0), Vektor3f(0, 0, 90), 0, l);
    pruefe(l.size() == 9 && nah(l[0].b[0], 0) && nah(l[0].b[1], 32), "arrow_draw: 9 Linien, angle 90 zeigt nach +y, 32 lang");
    pruefe(entitaetsWinkel("90", "", false, false) == Vektor3f(0, 0, 90), "angle 90 -> (0, 0, 90)");
    pruefe(entitaetsWinkel("90", "10 20 30", true, false) == Vektor3f(30, 10, 20), "angles \"10 20 30\" -> (30, 10, 20), angles vor angle");
    pruefe(entitaetsWinkel("-1", "", false, true) == Vektor3f(0, -90, 0), "Gruppe angle -1 -> hoch (0, -90, 0)");
    pruefe(entitaetsWinkel("-90", "", false, false) == Vektor3f(0, 0, 270), "angle -90 -> 270 (normalise_angles)");
    l.clear();
    verbindung(Vektor3f(0, 0, 0), Vektor3f(100, 0, 0), 0, l);
    const std::size_t kurz = l.size();
    l.clear();
    verbindung(Vektor3f(0, 0, 0), Vektor3f(600, 0, 0), 0, l);
    const std::size_t mittel = l.size();
    l.clear();
    verbindung(Vektor3f(0, 0, 0), Vektor3f(1000, 0, 0), 0, l);
    pruefe(kurz == 3 && mittel == 5 && l.size() == 7, "Verbindung: Pfeil in der Mitte, ab 512 zusaetzlich bei 256 vom Ende");
    l.clear();
    lichtOktaeder(Kasten(Vektor3f(0, 0, 0), Vektor3f(8, 8, 8)), 0, l);
    pruefe(l.size() == 24, "Licht: Oktaeder aus 8 Dreiecken");
    XyAnsicht v;
    v.breite = 800;
    v.hoehe = 600;
    v.massstab = 1;
    pruefe(!kastenPasst(Kasten(Vektor3f(0, 0, 0), Vektor3f(1, 1, 1)), v, 64) == false, "aabb_fits_view: (400+300)/(8+8) = 43.75 < 64");
    v.massstab = 0.25F;
    pruefe(!kastenPasst(Kasten(Vektor3f(0, 0, 0), Vektor3f(1, 1, 1)), v, 64), "aabb_fits_view bei 0.25: 175 > 64 - kein Name");
    // Patch 3x3 flach: ungekruemmt wird nicht unterteilt (Blattkurven), die
    // Tesselierung hat nur die 2 x 2 Ecken - das Gitter ist der Umriss.
    Patch pa;
    pa.setzeGroesse(3, 3);
    for (std::size_t r = 0; r < 3; ++r) {
        for (std::size_t c = 0; c < 3; ++c) { pa.punkt(r, c).vertex = Vektor3f(static_cast<float>(c) * 32, static_cast<float>(r) * 32, 0); }
    }
    pa.punkteGeaendert();
    l.clear();
    patchGitter(pa, 0, l);
    pruefe(l.size() == 4, "Patch-Drahtgitter 3x3 flach: Umriss, 4 Strecken (ist " + std::to_string(l.size()) + ")");
    // gekruemmt (Mitte angehoben): Kurven werden geteilt, mehr Linien
    pa.punkt(1, 1).vertex = Vektor3f(32, 32, 64);
    pa.punkteGeaendert();
    l.clear();
    patchGitter(pa, 0, l);
    pruefe(l.size() > 4, "Patch-Drahtgitter gekruemmt: Zusatzlinien der Kurvenbaeume (ist " + std::to_string(l.size()) + ")");
    const auto k2 = kameraSymbol(v, Vektor3f(0, 0, 0), Vektor3f(0, 0, 0), 0);
    pruefe(k2.size() == 7, "Kamera-Symbol: Raute mit Mittellinie (5) und Blickwinkel (2)");
}

}  // namespace

int main() {
    raster();
    zustand();
    szene();
    std::printf("kartexyansicht: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
