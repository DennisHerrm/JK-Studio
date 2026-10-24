// kartemodell.cpp - Karten-Editor Stufe 2, Punkt 10b: Modelle fuer
// misc_model wie NetRadiant-Custom 20260114 (nullmodel.cpp, md3model,
// assmodel/model.cpp CopyPicoSurface, miscmodel.cpp RemapKeysObserver und
// updateTransform) und q3map2 (pm_ase.c, InsertModel mit .skin).
//
// Sollwerte von Hand: md3 Bild 0 mit 1/64; ASE kehrt die Eckenfolge um und
// spiegelt t (1 - t); der Shader kommt aus dem Bild, ausser der
// Materialname beginnt mit textures/ oder models/; ohne '/' liegt er im
// Ordner des Modells, mit ".." ab "/textures/"; _remap: die laengste Endung
// gewinnt, "*" nur ohne Treffer.
#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/eklasse.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/modell.h"
#include "bhed/karte/region.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
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

void put32(std::string& b, std::size_t at, std::int32_t v) { std::memcpy(b.data() + at, &v, 4); }
void put16(std::string& b, std::size_t at, std::int16_t v) { std::memcpy(b.data() + at, &v, 2); }
void putf(std::string& b, std::size_t at, float v) { std::memcpy(b.data() + at, &v, 4); }

// Eine .md3 mit einer Flaeche, drei Ecken, zwei Bildern (wie tests/md3test.cpp)
std::string md3Datei(const char* shader) {
    constexpr std::size_t kQ = 64;
    constexpr std::size_t kHdr = 108;
    constexpr std::size_t kSurfHdr = 108;
    const int verts = 3;
    const int frames = 2;
    const std::size_t ofsShaders = kSurfHdr;
    const std::size_t ofsTris = ofsShaders + kQ + 4;
    const std::size_t ofsSt = ofsTris + 12;
    const std::size_t ofsXyz = ofsSt + static_cast<std::size_t>(verts) * 8;
    const std::size_t surfEnd = ofsXyz + static_cast<std::size_t>(verts * frames) * 8;
    std::string b(kHdr + surfEnd, '\0');
    std::memcpy(b.data(), "IDP3", 4);
    put32(b, 4, 15);
    put32(b, 8 + kQ + 4, frames);
    put32(b, 8 + kQ + 12, 1);
    put32(b, 8 + kQ + 28, static_cast<std::int32_t>(kHdr));
    put32(b, 8 + kQ + 32, static_cast<std::int32_t>(b.size()));
    const std::size_t s = kHdr;
    std::memcpy(b.data() + s + 4, "flaeche", 7);
    const std::size_t n = s + 4 + kQ;
    put32(b, n + 4, frames);
    put32(b, n + 8, 1);
    put32(b, n + 12, verts);
    put32(b, n + 16, 1);
    put32(b, n + 20, static_cast<std::int32_t>(ofsTris));
    put32(b, n + 24, static_cast<std::int32_t>(ofsShaders));
    put32(b, n + 28, static_cast<std::int32_t>(ofsSt));
    put32(b, n + 32, static_cast<std::int32_t>(ofsXyz));
    put32(b, n + 36, static_cast<std::int32_t>(surfEnd));
    std::memcpy(b.data() + s + ofsShaders, shader, std::strlen(shader));
    for (int k = 0; k < 3; ++k) {
        put32(b, s + ofsTris + static_cast<std::size_t>(k) * 4, k);
        putf(b, s + ofsSt + static_cast<std::size_t>(k) * 8, static_cast<float>(k) * 0.5F);
        putf(b, s + ofsSt + static_cast<std::size_t>(k) * 8 + 4, 0.25F);
    }
    for (int f = 0; f < frames; ++f) {
        for (int k = 0; k < verts; ++k) {
            const std::size_t at = s + ofsXyz + static_cast<std::size_t>(f * verts + k) * 8;
            put16(b, at, static_cast<std::int16_t>(64 * (k + 1) * (f + 1)));
            put16(b, at + 6, static_cast<std::int16_t>(64));
        }
    }
    return b;
}

const char* const kAse =
    "*3DSMAX_ASCIIEXPORT 200\n"
    "*MATERIAL_LIST {\n"
    "\t*MATERIAL_COUNT 3\n"
    "\t*MATERIAL 0 {\n"
    "\t\t*MATERIAL_NAME \"textures/a/b\"\n"
    "\t\t*MAP_DIFFUSE {\n"
    "\t\t\t*BITMAP \"C:\\x\\base\\textures\\c\\d.tga\"\n"
    "\t\t}\n"
    "\t}\n"
    "\t*MATERIAL 1 {\n"
    "\t\t*MATERIAL_NAME \"Holz\"\n"
    "\t\t*SUBMATERIAL 0 {\n"
    "\t\t\t*MATERIAL_NAME \"holz2 extra\"\n"
    "\t\t\t*MAP_DIFFUSE {\n"
    "\t\t\t\t*BITMAP \"..\\..\\textures\\e\\f.jpg\"\n"
    "\t\t\t}\n"
    "\t\t}\n"
    "\t}\n"
    "\t*MATERIAL 2 {\n"
    "\t\t*MATERIAL_NAME \"stein\"\n"
    "\t\t*MAP_DIFFUSE {\n"
    "\t\t\t*BITMAP \"None\"\n"
    "\t\t}\n"
    "\t}\n"
    "}\n"
    "*GEOMOBJECT {\n"
    "\t*NODE_NAME \"teil\"\n"
    "\t*MESH {\n"
    "\t\t*MESH_NUMVERTEX 3\n"
    "\t\t*MESH_NUMFACES 1\n"
    "\t\t*MESH_VERTEX_LIST {\n"
    "\t\t\t*MESH_VERTEX 0 0.0 0.0 0.0\n"
    "\t\t\t*MESH_VERTEX 1 10.0 0.0 0.0\n"
    "\t\t\t*MESH_VERTEX 2 0.0 20.0 5.0\n"
    "\t\t}\n"
    "\t\t*MESH_FACE_LIST {\n"
    "\t\t\t*MESH_FACE 0: A: 0 B: 1 C: 2 AB: 1 BC: 1 CA: 1 *MESH_SMOOTHING 1 *MESH_MTLID 0\n"
    "\t\t}\n"
    "\t\t*MESH_NUMTVERTEX 3\n"
    "\t\t*MESH_TVERTLIST {\n"
    "\t\t\t*MESH_TVERT 0 0.0 0.0 0.0\n"
    "\t\t\t*MESH_TVERT 1 1.0 0.25 0.0\n"
    "\t\t\t*MESH_TVERT 2 0.0 1.0 0.0\n"
    "\t\t}\n"
    "\t\t*MESH_NUMTVFACES 1\n"
    "\t\t*MESH_TFACELIST {\n"
    "\t\t\t*MESH_TFACE 0 0 1 2\n"
    "\t\t}\n"
    "\t}\n"
    "\t*MATERIAL_REF 0\n"
    "}\n"
    "*GEOMOBJECT {\n"
    "\t*NODE_NAME \"zweites\"\n"
    "\t*MESH {\n"
    "\t\t*MESH_NUMVERTEX 3\n"
    "\t\t*MESH_NUMFACES 1\n"
    "\t\t*MESH_VERTEX_LIST {\n"
    "\t\t\t*MESH_VERTEX 0 0.0 0.0 -8.0\n"
    "\t\t\t*MESH_VERTEX 1 1.0 0.0 0.0\n"
    "\t\t\t*MESH_VERTEX 2 0.0 1.0 0.0\n"
    "\t\t}\n"
    "\t\t*MESH_FACE_LIST {\n"
    "\t\t\t*MESH_FACE 0: A: 0 B: 1 C: 2 *MESH_MTLID 7\n"
    "\t\t}\n"
    "\t}\n"
    "\t*MATERIAL_REF 1\n"
    "}\n"
    "*GEOMOBJECT {\n"
    "\t*NODE_NAME \"drittes\"\n"
    "\t*MESH {\n"
    "\t\t*MESH_NUMVERTEX 3\n"
    "\t\t*MESH_NUMFACES 1\n"
    "\t\t*MESH_VERTEX_LIST {\n"
    "\t\t\t*MESH_VERTEX 0 0.0 0.0 0.0\n"
    "\t\t\t*MESH_VERTEX 1 1.0 0.0 0.0\n"
    "\t\t\t*MESH_VERTEX 2 0.0 1.0 0.0\n"
    "\t\t}\n"
    "\t\t*MESH_FACE_LIST {\n"
    "\t\t\t*MESH_FACE 0: A: 0 B: 1 C: 2\n"
    "\t\t}\n"
    "\t}\n"
    "\t*MATERIAL_REF 2\n"
    "}\n";

class TestQuelle final : public ModellQuelle {
public:
    std::shared_ptr<const Modell> modell = std::make_shared<Modell>(nullModell());
    [[nodiscard]] ModellAnsicht ansicht(const Entitaet&) const override { return ModellAnsicht{modell, {"nomodel"}}; }
};

void lader() {
    const Modell n = nullModell();
    pruefe(n.null && n.flaechen.size() == 1 && n.flaechen[0].shader == "nomodel" && n.flaechen[0].indizes.size() == 36 &&
               nah(n.grenzen.extents[0], 8) && nah(n.grenzen.origin[2], 0),
           "NullModel: Kasten 8, nomodel, 12 Dreiecke");
    pruefe(texturName("models\\x\\y.tga") == "models/x/y" && texturName("a/b.c/d") == "a/b.c/d", "parseTextureName");
    Modell m;
    pruefe(md3Lesen(md3Datei("models\\x\\y.tga"), m) && m.flaechen.size() == 1 && m.flaechen[0].shader == "models/x/y", "md3: Shader ohne Endung, '/'");
    pruefe(m.flaechen[0].punkte.size() == 3 && nah(m.flaechen[0].punkte[2].ort[0], 3) && nah(m.grenzen.origin[0], 2) && nah(m.grenzen.extents[0], 1),
           "md3: Bild 0 (1, 2, 3), Grenzen 1..3");
    pruefe(modellArtBekannt("models/a.MD3") && modellArtBekannt("b.ase") && modellArtBekannt("c.glm") && !modellArtBekannt("d.obj"), "Endungen md3/ase/glm");
    // ASE
    Modell a;
    std::string fehler;
    pruefe(aseLesen(kAse, "models/m/tisch.ase", a, &fehler) && a.flaechen.size() == 3, "ASE: drei Flaechen (" + fehler + ")");
    if (a.flaechen.size() == 3) {
        const ModellFlaeche& f = a.flaechen[0];
        pruefe(f.shader == "textures/a/b", "ASE: Materialname mit textures/ gilt (" + f.shader + ")");
        pruefe(nah(f.punkte[0].ort[1], 20) && nah(f.punkte[2].ort[0], 0) && nah(f.punkte[1].ort[0], 10), "ASE: Ecken umgekehrt (C, B, A)");
        pruefe(nah(f.punkte[1].st[1], 0.75F) && nah(f.punkte[0].st[1], 0.0F), "ASE: t = 1 - t");
        pruefe(a.flaechen[1].shader == "textures/e/f", "ASE: Untermaterial 0 (MTLID 7 unbekannt), Bild mit '..' ab /textures/ (" + a.flaechen[1].shader + ")");
        pruefe(a.flaechen[2].shader == "models/m/stein", "ASE: ohne '/' im Ordner des Modells (" + a.flaechen[2].shader + ")");
        pruefe(nah(a.grenzen.origin[2], -1.5F) && nah(a.grenzen.extents[1], 10), "ASE: Grenzen");
    }
    Modell kaputt;
    pruefe(!aseLesen("*MESH_VERTEX 0 1 2 3\n", "x.ase", kaputt, &fehler) && fehler == "Vertex parse error in ASE, line 1.", "ASE: Fehlermeldung wie pico (" + fehler + ")");
}

void skinUndRemap() {
    pruefe(skinDateiName("models/a/b.md3", "1") == "models/a/b.md3_1.skin", "Skin-Nummer: DarkPlaces-Name");
    pruefe(skinDateiName("models/a/b.md3", "rot") == "models/a/b_rot.skin", "Skin-Name: Q3-Name");
    std::vector<std::string> weg;
    const Umbenennungen u = skinLesen("replace a b\r\nc,d\nmuell\n", &weg);
    pruefe(u.size() == 2 && u[0].first == "a" && u[0].second == "b" && u[1].first == "c" && u[1].second == "d" && weg.size() == 1 && weg[0] == "muell",
           ".skin: replace und Komma, ungueltige Zeile verworfen");
    Karte k;
    auto e = k.neueEntitaet("misc_model", false);
    e->setzen("_remap", "*;textures/x");
    e->setzen("_remap2", "b;textures\\y");
    pruefe(remapFuer(*e, "models/a/b") == "textures/y" && remapFuer(*e, "models/c") == "textures/x", "_remap: Endung vor *, PathCleaned");
    e->setzen("_remap3", "a/b;textures/z");
    pruefe(remapFuer(*e, "MODELS/A/B") == "textures/z", "_remap: die laengere Endung gewinnt (ohne Gross/klein)");
    auto f = k.neueEntitaet("misc_model", false);
    f->setzen("_remap", "a/b;textures/lang");
    f->setzen("_remap2", "b;textures/kurz");
    pruefe(remapFuer(*f, "models/a/b") == "textures/lang", "_remap: die laengere Endung gewinnt auch, wenn sie vorn steht");
    e->setzen("_remap4", "kaputt");
    auto leer = k.neueEntitaet("misc_model", false);
    pruefe(remapFuer(*leer, "x") == "", "ohne _remap: nichts");
}

void matrixUndGrenzen() {
    Karte k;
    auto e = k.neueEntitaet("misc_model", false);
    e->setzen("origin", "10 0 0");
    e->setzen("angle", "90");
    e->setzen("modelscale", "2");
    const Matrix4 m = miscModellMatrix(*e);
    const Vektor3f p = transformierterPunkt(m, Vektor3f(1, 0, 0));
    pruefe(nah(p[0], 10) && nah(p[1], 2) && nah(p[2], 0), "misc_model: Ursprung, angle 90, modelscale 2");
    e->setzen("modelscale_vec", "1 3 1");
    const Vektor3f q = transformierterPunkt(miscModellMatrix(*e), Vektor3f(0, 1, 0));
    pruefe(nah(q[0], 7) && nah(q[1], 0), "modelscale_vec vor modelscale");
    e->setzen("modelscale_vec", "");
    TestQuelle quelle;
    k.kontext.modelle = &quelle;
    k.entitaetEinfuegen(e);
    const Kasten b = instanzGrenzen(*e, k.kontext);
    pruefe(nah(b.origin[0], 10) && nah(b.extents[0], 16) && nah(b.extents[2], 16), "worldAABB: NullModel gedreht und skaliert (16)");
    k.auswahl.waehlen(e);
    const Kasten g = auswahlGrenzenPivot(k);
    pruefe(nah(g.origin[0], 10) && nah(g.extents[1], 16), "getBoundsSelected: misc_model mit seinem Modell");
    k.kontext.modelle = nullptr;
    const Kasten ohne = instanzGrenzen(*e, k.kontext);
    pruefe(nah(ohne.extents[0], 8), "ohne Modellquelle: der Kasten der Klasse");
}

}  // namespace

int main() {
    lader();
    skinUndRemap();
    matrixUndGrenzen();
    std::printf("kartemodell: %d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
