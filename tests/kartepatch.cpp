// kartepatch.cpp - Karten-Editor Stufe 1a, Punkt 2c: Patches (bhed/karte/patch.h).
//
// Geprueft wird die Anzeige-Tesselierung aus radiant/patch.cpp:
//   - patchDef2 adaptiv: eine Viertelkreis-Kurve (Radius 128) wird geteilt,
//     solange Sehnenlaenge * Bogenwinkel/pi > 4 (g_PatchSubdivideThreshold);
//     bei 90 Grad ergibt das 90.5 / 4^n -> drei Stufen, 9 Spalten. Gerade
//     Richtungen werden nicht geteilt (2 Zeilen).
//   - jeder erzeugte Punkt liegt auf der analytischen quadratischen
//     Bezierflaeche (de Casteljau),
//   - patchDef3 fest: (sx+1) x (sy+1) Punkte bei u = i/sx, v = j/sy,
//   - Normalen sind Einheitsvektoren,
//   - setDims mit NRCs Fehler bei gerader Hoehe, Begrenzung auf 31,
//   - Umkehren, Transponieren, Spiegeln, Rueckgaengig.
#include "bhed/karte/patch.h"

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

// 3x3-Patch: Spalten gekruemmt (Viertelkreis in x/z), Zeilen gerade in y.
std::shared_ptr<Patch> viertelZylinder(bool def3, std::size_t sx, std::size_t sy) {
    auto p = std::make_shared<Patch>();
    p->patchDef3 = def3;
    p->unterteilungX = sx;
    p->unterteilungY = sy;
    p->setzeGroesse(3, 3);
    const float xs[3] = {128, 128, 0};
    const float zs[3] = {0, 128, 128};
    for (std::size_t r = 0; r < 3; ++r) {
        for (std::size_t c = 0; c < 3; ++c) {
            p->punkt(r, c).vertex = Vektor3f(xs[c], 64.0F * static_cast<float>(r), zs[c]);
            p->punkt(r, c).texcoord = Vektor2f(0.5F * static_cast<float>(c), 0.25F * static_cast<float>(r));
        }
    }
    p->punkteGeaendert();
    return p;
}

double bezier(double a, double b, double c, double t) { return (1 - t) * (1 - t) * a + 2 * t * (1 - t) * b + t * t * c; }

}  // namespace

int main() {
    // --- patchDef2: adaptiv ------------------------------------------------
    {
        auto p = viertelZylinder(false, 0, 0);
        const PatchTesselierung& t = p->tesselierung();
        pruefe(t.nArrayWidth == 9, "patchDef2 Viertelkreis: 9 Spalten (3 Teilungsstufen), ist " + std::to_string(t.nArrayWidth));
        pruefe(t.nArrayHeight == 2, "patchDef2 gerade Richtung: 2 Zeilen, ist " + std::to_string(t.nArrayHeight));
        pruefe(t.vertices.size() == 18 && t.indices.size() == 9 * 2 * 1, "patchDef2: 18 Punkte, 18 Indizes");
        pruefe(t.numStrips == 1 && t.lenStrips == 18, "patchDef2: ein Streifen der Laenge 18 (bWidthStrips)");
        bool aufKurve = true;
        bool einheit = true;
        for (const auto& v : t.vertices) {
            // x = 128 (1 - u^2), z = 128 (2u - u^2)
            const double u = std::sqrt(std::max(0.0, 1.0 - v.vertex.x() / 128.0));
            aufKurve = aufKurve && std::fabs(v.vertex.z() - 128.0 * (2 * u - u * u)) < 1e-3;
            aufKurve = aufKurve && std::fabs(v.texcoord.x() - u) < 1e-5;   // s = 0, 0.5, 1 linear in u
            einheit = einheit && std::fabs(laenge(v.normal) - 1.0) < 1e-5 && std::fabs(v.normal.y()) < 1e-5;
        }
        pruefe(aufKurve, "patchDef2: Punkte liegen auf der Bezierkurve, s passt zu u");
        pruefe(einheit, "patchDef2: Normalen sind Einheitsvektoren senkrecht zur Achse");
        // die Spalten liegen bei u = k/8 (dyadisch)
        bool dyadisch = true;
        for (std::size_t i = 0; i < 9; ++i) {
            const auto& v = t.vertices[i];
            const double u = static_cast<double>(i) / 8.0;
            dyadisch = dyadisch && std::fabs(v.vertex.x() - bezier(128, 128, 0, u)) < 1e-3;
        }
        pruefe(dyadisch, "patchDef2: Spalte i bei u = i/8");
        pruefe(p->grenzen().origin[0] == 64 && p->grenzen().extents[1] == 64, "patchDef2: Grenzen aus den Kontrollpunkten");
        // Schwellwert 64: 90.5 > 64 -> nur eine Stufe (5.66 und 1.41 darunter): 3 Spalten
        p->punkteGeaendert(64);
        pruefe(p->tesselierung().nArrayWidth == 3, "Schwellwert 64: eine Teilung, 3 Spalten");
    }

    // --- flacher Patch: keine Teilung --------------------------------------
    {
        auto p = std::make_shared<Patch>();
        p->setzeGroesse(3, 3);
        for (std::size_t r = 0; r < 3; ++r) {
            for (std::size_t c = 0; c < 3; ++c) {
                p->punkt(r, c).vertex = Vektor3f(32.0F * c, 32.0F * r, 0);
            }
        }
        p->punkteGeaendert();
        pruefe(p->tesselierung().vertices.size() == 4, "flacher Patch: nur die 4 Ecken");
        bool z = true;
        for (const auto& v : p->tesselierung().vertices) {
            z = z && std::fabs(std::fabs(v.normal.z()) - 1) < 1e-6;
        }
        pruefe(z, "flacher Patch: Normalen +-z");
    }

    // --- patchDef3: feste Unterteilung -------------------------------------
    {
        auto p = viertelZylinder(true, 4, 2);
        const PatchTesselierung& t = p->tesselierung();
        pruefe(t.nArrayWidth == 5 && t.nArrayHeight == 3, "patchDef3 4x2: 5 x 3 Punkte");
        bool genau = true;
        for (std::size_t j = 0; j < 3; ++j) {
            for (std::size_t i = 0; i < 5; ++i) {
                const auto& v = t.vertices[j * 5 + i];
                const double u = static_cast<double>(i) / 4.0;
                const double w = static_cast<double>(j) / 2.0;
                genau = genau && std::fabs(v.vertex.x() - bezier(128, 128, 0, u)) < 1e-4
                     && std::fabs(v.vertex.z() - bezier(0, 128, 128, u)) < 1e-4 && std::fabs(v.vertex.y() - 128 * w) < 1e-4
                     && std::fabs(v.texcoord.x() - u) < 1e-6 && std::fabs(v.texcoord.y() - 0.5 * w) < 1e-6;
            }
        }
        pruefe(genau, "patchDef3: Punkt (i, j) = B(i/4, j/2), st linear");
    }

    // --- 5x3-Patch: zwei Teilfelder nebeneinander --------------------------
    {
        auto p = std::make_shared<Patch>();
        p->setzeGroesse(5, 3);
        for (std::size_t r = 0; r < 3; ++r) {
            for (std::size_t c = 0; c < 5; ++c) {
                const float x = (c % 2 == 0) ? 0.0F : 64.0F;   // Zickzack in x -> beide Teilfelder gekruemmt
                p->punkt(r, c).vertex = Vektor3f(x, 64.0F * static_cast<float>(c), 32.0F * static_cast<float>(r));
            }
        }
        p->punkteGeaendert();
        const auto& t = p->tesselierung();
        pruefe(t.arrayWidth.size() == 2 && t.nArrayWidth == 1 + t.arrayWidth[0] + t.arrayWidth[1],
               "5x3: zwei Teilfelder, nArrayWidth = 1 + Summe");
        pruefe(t.vertices.size() == t.nArrayWidth * t.nArrayHeight, "5x3: Punkte = Breite x Hoehe");
    }

    // --- setDims (patch.cpp:156-186) ---------------------------------------
    {
        Patch p;
        p.setzeGroesse(4, 4);
        pruefe(p.breite() == 3 && p.hoehe() == 4 && p.punkte().size() == 12,
               "setDims(4,4): Breite 3, Hoehe bleibt 4 (NRC verringert m_height statt h)");
        p.setzeGroesse(33, 1);
        pruefe(p.breite() == 31 && p.hoehe() == 3, "setDims(33,1): 31 x 3 (Grenzen)");
    }

    // --- Umkehren, Transponieren, Spiegeln, Undo ---------------------------
    {
        Rueckgaengig undo;
        auto p = viertelZylinder(false, 0, 0);
        p->anmelden(&undo);
        const Vektor3f ecke = p->punkt(0, 0).vertex;
        {
            Befehl b(undo, "patchUmkehren");
            p->matrixUmkehren();
        }
        pruefe(p->punkt(2, 0).vertex == ecke && p->punkt(0, 0).vertex.y() == 128, "InvertMatrix: Zeilen vertauscht");
        {
            Befehl b(undo, "patchTransponieren");
            p->matrixTransponieren();
        }
        pruefe(p->tesselierung().nArrayWidth == 2 && p->tesselierung().nArrayHeight == 9,
               "TransposeMatrix: gekruemmte Richtung jetzt in den Zeilen");
        {
            Befehl b(undo, "patchSpiegeln");
            p->transformieren(skalierungsMatrix(Vektor3f(1, -1, 1)));
        }
        pruefe(p->punkt(0, 0).vertex.y() == 0 || p->punkt(0, 0).vertex.y() == -128, "Spiegeln: Punkte gespiegelt");
        pruefe(undo.undoAnzahl() == 3 && undo.undo() && undo.undo() && undo.undo(), "drei Schritte rueckgaengig");
        pruefe(p->punkt(0, 0).vertex == ecke && p->tesselierung().nArrayWidth == 9, "Undo: Ausgangslage wieder da");
        p->setzeShader("TEXTURES/");   // gleich ohne Gross/klein: keine Aenderung
        pruefe(p->shader() == "textures/", "SetShader: gleicher Name ohne Gross/klein aendert nichts");
        p->abmelden();
    }

    std::printf("kartepatch: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
