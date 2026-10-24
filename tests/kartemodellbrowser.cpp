// kartemodellbrowser.cpp - Karten-Editor Stufe 2, Punkt 10c: was der
// Modell-Browser rechnet, wie NetRadiant-Custom 20260114
// (radiant/modelwindow.cpp: ModelFS, CellPos, models_set_transforms,
// ModelFolders, ModelBrowser_constructTree).
//
// Sollwerte von Hand: Zellen je Zeile ( breite - 8 ) / ( 2 zelle + 8 ),
// Mitte x = i%n * 2z + z + ( i%n + 1 ) * 8, z = i/n * 2z + z + ( i/n + 1 ) *
// Schrift; die Ordnerliste "*ordner/tiefe*", ohne Eintrag "models/" 99.
#include "bhed/karte/modellbrowser.h"
#include "bhed/karte/texturen.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
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
bool nah(float a, float b) { return std::fabs(a - b) < 1e-3F; }
bool nah3(const Vektor3f& v, float x, float y, float z) { return nah(v[0], x) && nah(v[1], y) && nah(v[2], z); }

void baum() {
    ModellOrdner w;
    w.einfuegen("models/a/x.md3");
    w.einfuegen("models/a/y.md3");
    w.einfuegen("models/b.md3");
    w.einfuegen("wurzel.md3");
    pruefe(w.dateien.size() == 1 && w.ordner.size() == 1 && w.ordner["models"].dateien.count("b.md3") == 1 &&
               w.ordner["models"].ordner["a"].dateien.size() == 2 && w.ordner["models"].ordner["a"].name == "a",
           "ModelFS: Ordner bis zum ersten '/', Dateien darin");
    const auto l = modellOrdnerListe("*models/99*");
    pruefe(l.size() == 1 && l.at("models/") == 99, "*models/99*");
    const auto r = modellOrdnerListe("*/3*");
    pruefe(r.size() == 1 && r.count("") == 1 && r.begin()->second == 3, "*/3*: die Wurzel");
    pruefe(modellOrdnerListe("").count("models/") == 1, "leer: models/ 99");
    const auto m = modellOrdnerListe("*a\\b/2*kaputt*c/x*");
    pruefe(m.size() == 2 && m.count("a/b/") == 1 && m.count("c/") == 1 && m.find("a/b/")->second == 2 && m.find("c/")->second == 0,
           "PathCleaned, ohne '/' verworfen, Tiefe x -> 0");

    namespace fs = std::filesystem;
    const fs::path tmp = fs::temp_directory_path() / "bhed_kartemodellbrowser_test";
    std::error_code ec;
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp / "models" / "a", ec);
    fs::create_directories(tmp / "other", ec);
    for (const char* n : {"models/a/x.md3", "models/b.ASE", "models/c.txt", "other/d.md3", "models/a/e.glm"}) { std::ofstream(tmp / n) << "x"; }
    Dateisystem dfs;
    dfs.ordnerHinzufuegen(tmp.string(), "sp");
    const ModellOrdner t = modellBaum(dfs, "*models/99*", {"md3", "ase", "glm"});
    const auto tm = t.ordner.find("models");
    pruefe(t.ordner.size() == 1 && tm != t.ordner.end() && tm->second.dateien.count("b.ASE") == 1 && tm->second.ordner.count("a") == 1 &&
               tm->second.ordner.at("a").dateien.size() == 2 && tm->second.dateien.size() == 1,
           "constructTree: md3/ASE/glm unter models/, nicht c.txt und other/");
    const ModellOrdner flach = modellBaum(dfs, "*models/1*", {"md3", "ase", "glm"});
    const auto fm = flach.ordner.find("models");
    pruefe(fm != flach.ordner.end() && fm->second.ordner.empty() && fm->second.dateien.size() == 1, "Tiefe 1: nur models/b.ASE");
    fs::remove_all(tmp, ec);
}

void zellen() {
    const ModellZellen z(400, 80, 16, 4);
    pruefe(z.jeZeile() == 2, "zwei Zellen je Zeile bei 400");
    pruefe(nah3(z.ursprung(0), 88, 0, -96) && nah3(z.ursprung(3), 256, 0, -272), "getOrigin");
    pruefe(nah3(z.textOrt(0), 8, 0, -19), "getTextPos (Unterlaenge 4)");
    pruefe(z.gesamtHoehe(100, 3) == 368 && z.gesamtHoehe(1000, 3) == 1000, "totalHeight");
    pruefe(z.treffer(200, 180) == 3 && z.treffer(399, 0) == 1, "testSelect");
    const ModellZellen eng(50, 80, 16, 4);
    pruefe(eng.jeZeile() == 1, "mindestens eine Zelle je Zeile");
    // models_set_transforms
    const Kasten k(Vektor3f(10, 0, 0), Vektor3f(5, 20, 40));
    const Matrix4 m = modellInZelle(k, z, 0);
    pruefe(nah3(transformierterPunkt(m, Vektor3f(10, 0, 40)), 88, 0, -16), "in der Zelle: Skala 80/40, oben bleibt oben");
    pruefe(nah3(transformierterPunkt(m, Vektor3f(15, 0, 0)), 88, -10, -96), "kleinste Ausdehnung x -> Drehung -90 um z");
    const Matrix4 w = modellInZelle(Kasten(Vektor3f(0, 0, 0), Vektor3f(40, 40, 30)), z, 0);
    pruefe(nah3(transformierterPunkt(w, Vektor3f(0, 0, 30)), 88, 0, -36), "z doppelt gewichtet: 40 40 30 dreht um z, nicht um x");
    const Matrix4 d = modellDrehen(m, 80, 0, z, 0);
    pruefe(nah3(transformierterPunkt(d, Vektor3f(10, 0, 0)), 88, 0, -96), "Drehen um den Zellmittelpunkt laesst ihn stehen");
    const Vektor3f q = transformierterPunkt(d, Vektor3f(15, 0, 0));
    pruefe(nah(q[2], -96) && nah(std::sqrt((q[0] - 88) * (q[0] - 88) + q[1] * q[1]), 10), "Drehen (dx 80 -> 45 Grad um z)");
}

}  // namespace

int main() {
    baum();
    zellen();
    std::printf("kartemodellbrowser: %d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
