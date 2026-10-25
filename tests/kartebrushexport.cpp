// kartebrushexport.cpp - Karten-Editor Stufe 4 Punkt 3: das Plugin
// brushexport2 ohne Oberflaeche (bhed/karte/brushexport.h).
//
// Geprueft gegen NRC 20260114 contrib/brushexport/export.cpp:
//   - Kopf, "mtllib <name>.mtl", "g Brush<n>" je Pinsel (COLLAPSE_NONE),
//     "g all" + "usemtl material" (COLLAPSE_ALL), je Material eine Gruppe mit
//     dem Kurznamen (COLLAPSE_BY_MATERIAL), "o" statt "g";
//   - Ecken "v x z -y" in "%1.6lf", die Wicklung rueckwaerts, Verschweissen
//     (Edge_isDegenerate: Abstand^2 < ON_EPSILON^2) innerhalb der Gruppe;
//   - "vt s -t", "f v/t ..." je Flaeche, nach Material sortiert (multimap),
//     "usemtl <Shader>", kurze Namen (letzte 20 Zeichen);
//   - ausgelassene Materialien (Kurzname, ohne Gross/klein), Pinsel ohne
//     Flaeche fallen weg;
//   - .mtl: Anzahl, "newmtl", "Kd" mit "%g", "map_Kd".
//
// Aufruf: kartebrushexport
#include "bhed/karte/brushexport.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"

#include <cstdio>
#include <string>

using namespace bhed::karte;
namespace be = bhed::karte::brushexport;

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

std::string kasten(int x0, int y0, int z0, int x1, int y1, int z1, const char* oben, const char* rest) {
    char z[1600];
    std::snprintf(z, sizeof(z),
                  "{\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n}\n",
                  x1, y1, z1, x1, y0, z1, x0, y1, z1, oben, x1, y1, z1, x0, y1, z1, x1, y1, z0, rest, x1, y1, z1, x1, y1, z0, x1, y0, z1, rest, x0, y0, z0,
                  x1, y0, z0, x0, y1, z0, rest, x0, y0, z0, x0, y0, z1, x1, y0, z0, rest, x0, y0, z0, x0, y1, z0, x0, y0, z1, rest);
    return z;
}

std::size_t zaehle(const std::string& t, const std::string& was) {
    std::size_t n = 0;
    for (std::size_t p = 0; (p = t.find(was, p)) != std::string::npos; p += was.size()) { ++n; }
    return n;
}

void allesWaehlen(Karte& k) {
    k.auswahl.leeren();
    for (const auto& p : k.welt()->kinder()) { k.auswahl.waehlen(k.welt(), p); }
}

}  // namespace

int main() {
    Karte k;
    (void)karteLesen("{\n\"classname\" \"worldspawn\"\n" + kasten(0, 0, 0, 64, 64, 64, "test/oben", "test/wand") +
                         kasten(100, 0, 0, 164, 64, 64, "test/oben", "common/caulk") + "}\n",
                     k);
    k.anmelden();
    const be::TexturFarbe farbe = [](const std::string& s) {
        return s == "textures/test/oben" ? std::array<float, 3>{0.25F, 0.5F, 1.0F} : std::array<float, 3>{0.125F, 0.125F, 0.125F};
    };
    // ohne Auswahl: keine Gruppe
    be::Optionen o;
    be::Ergebnis r = be::exportieren(k, o, "Y:/aus/test", farbe);
    pruefe(r.objPfad == "Y:/aus/test.obj" && r.mtlPfad == "Y:/aus/test.mtl", "Pfade: \".obj\" angehaengt, .mtl daneben");
    pruefe(r.obj.find("# Wavefront Objectfile exported with radiants brushexport plugin 3.0 by Thomas 'namespace' Nitschke, spam@codecreator.net\n\n"
                      "mtllib test.mtl\n") == 0,
           "Kopf und mtllib (nur der Dateiname)");
    pruefe(zaehle(r.obj, "\ng ") == 0, "nichts gewaehlt: keine Gruppe");
    // Don't collapse, verschweissen
    allesWaehlen(k);
    r = be::exportieren(k, o, "Y:/aus/test.obj", farbe);
    pruefe(zaehle(r.obj, "\ng Brush1\n") == 1 && zaehle(r.obj, "\ng Brush2\n") == 1, "Don't collapse: g Brush1, g Brush2");
    pruefe(zaehle(r.obj, "\nv ") == 16, "Verschweisst: 8 Ecken je Pinsel (" + std::to_string(zaehle(r.obj, "\nv ")) + ")");
    pruefe(zaehle(r.obj, "\nvt ") == 48 && zaehle(r.obj, "\nf ") == 12, "je Flaeche 4 vt, 12 Flaechen");
    pruefe(r.obj.find("v 64.000000 64.000000 -64.000000\n") != std::string::npos && r.obj.find("v 164.000000 0.000000 -0.000000\n") != std::string::npos,
           "v x z -y in %1.6lf (auch -0.000000)");
    pruefe(zaehle(r.obj, "\nusemtl textures/test/oben") == 2 && zaehle(r.obj, "\nusemtl textures/common/caulk") == 1 &&
               zaehle(r.obj, "\nusemtl textures/test/wand") == 1,
           "usemtl je Gruppe und Material (nach Namen sortiert)");
    pruefe(r.obj.find("usemtl textures/test/oben\nf 1/1 2/2 3/3 4/4") != std::string::npos, "die Oberseite als erste Flaeche, 1/1 2/2 3/3 4/4");
    {   // brushMaterials: multimap - in Brush2 caulk vor test/oben (nach dem Namen), nicht in Flaechenfolge
        const std::size_t b2 = r.obj.find("\ng Brush2\n");
        const std::size_t c = r.obj.find("\nusemtl textures/common/caulk", b2);
        const std::size_t ob = r.obj.find("\nusemtl textures/test/oben", b2);
        pruefe(b2 != std::string::npos && c != std::string::npos && ob != std::string::npos && c < ob, "usemtl nach Material sortiert (multimap)");
    }
    pruefe(r.mtl == "# Wavefront material file exported with NetRadiants brushexport plugin.\n# Material Count: 3\n\n"
                    "newmtl textures/common/caulk\nKd 0.125 0.125 0.125\nmap_Kd textures/common/caulk\n"
                    "newmtl textures/test/oben\nKd 0.25 0.5 1\nmap_Kd textures/test/oben\n"
                    "newmtl textures/test/wand\nKd 0.125 0.125 0.125\nmap_Kd textures/test/wand\n",
           ".mtl: drei Materialien sortiert, Kd mit %g");
    // ohne Verschweissen
    o.verschweissen = false;
    r = be::exportieren(k, o, "Y:/aus/test.obj", farbe);
    pruefe(zaehle(r.obj, "\nv ") == 48, "ohne Verschweissen: 4 Ecken je Flaeche");
    o.verschweissen = true;
    // Collapse mesh
    o.modus = be::Zusammen::alle;
    r = be::exportieren(k, o, "Y:/aus/test.obj", farbe);
    pruefe(zaehle(r.obj, "\ng all\nusemtl material\n\n") == 1 && zaehle(r.obj, "\ng ") == 1 && zaehle(r.obj, "\nv ") == 16,
           "Collapse mesh: g all, usemtl material, 16 Ecken");
    pruefe(r.mtl.find("# Material Count: 1\n\nnewmtl material\nKd 0.5 0.5 0.5\nmap_Kd material\n") != std::string::npos, "Collapse mesh: ein Material 0.5");
    // Collapse by material, ausgelassen: caulk
    o.modus = be::Zusammen::nachMaterial;
    o.ignoriert = {"CAULK"};
    r = be::exportieren(k, o, "Y:/aus/test.obj", farbe);
    pruefe(zaehle(r.obj, "\ng oben\n") == 1 && zaehle(r.obj, "\ng wand\n") == 1 && zaehle(r.obj, "\ng ") == 2 && r.obj.find("caulk") == std::string::npos,
           "Collapse by material: g oben, g wand (Kurzname), caulk ausgelassen (ohne Gross/klein)");
    // objects, kurze Namen
    o.modus = be::Zusammen::keine;
    o.ignoriert = {"wand", "oben"};
    o.objekte = true;
    o.kurzeNamen = true;
    r = be::exportieren(k, o, "Y:/aus/test.obj", farbe);
    pruefe(zaehle(r.obj, "\no Brush1\n") == 1 && zaehle(r.obj, "\no ") == 1, "Pinsel 1 ganz ausgelassen: seine Gruppe faellt weg, o statt g");
    pruefe(zaehle(r.obj, "\nusemtl extures/common/caulk") == 1 && r.mtl.find("newmtl extures/common/caulk\n") != std::string::npos &&
               r.mtl.find("map_Kd textures/common/caulk\n") != std::string::npos,
           "kurze Namen: die letzten 20 Zeichen (map_Kd voll)");
    o.materialien = false;
    r = be::exportieren(k, o, "Y:/aus/test.obj", farbe);
    pruefe(r.obj.find("mtllib") == std::string::npos && r.mtl.empty(), "ohne .mtl: kein mtllib");
    pruefe(be::materialName("textures/common/caulk") == "caulk" && be::materialName("nodraw") == "nodraw", "OnAddMaterial: path_get_filename_start");
    std::printf("kartebrushexport: %d Pruefungen, %d Fehlschlaege\n", g_pruefungen, g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
