// kartegeoimport.cpp - Stufe 9 Punkt 2: Geometrie zu Brushes (ctest karte_geoimport)
//
// Liest kleine Netze als obj und gltf (eingebettet) und prueft die Umwandlung: ein Wuerfel
// wird ein Brush mit sechs texturierten Seiten (UV als drei st-Punkte), ein L-Koerper wird
// konvex zerlegt (jeder Innenpunkt in einem Brush, die Kerbe in keinem, verdeckte Seiten
// caulk), eine offene Flaeche wird ein Prisma (zwei Dreiecke zu einer Seite zusammengefasst,
// detail), Auto waehlt Modell+Clip fuer offene Netze, Y-oben wird Z-oben, kaputte Dateien
// werden mit Grund abgewiesen.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "bhed/karte/bspoeffnen.h"
#include "bhed/karte/geometrieimport.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/pinsel.h"

namespace {
int g_fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++g_fehler; }
}
using namespace bhed::karte;

// Liegt q in einem Brush? (hinter allen Seiten-Ebenen, Normale (p2-p0) x (p1-p0))
bool imPinsel(const ImportPinsel& b, double x, double y, double z) {
    for (const ImportSeite& s : b.seiten) {
        const double ax = s.p[2].x - s.p[0].x, ay = s.p[2].y - s.p[0].y, az = s.p[2].z - s.p[0].z;
        const double bx = s.p[1].x - s.p[0].x, by = s.p[1].y - s.p[0].y, bz = s.p[1].z - s.p[0].z;
        double nx = ay * bz - az * by, ny = az * bx - ax * bz, nz = ax * by - ay * bx;
        const double l = std::sqrt(nx * nx + ny * ny + nz * nz);
        nx /= l;
        ny /= l;
        nz /= l;
        if ((x - s.p[0].x) * nx + (y - s.p[0].y) * ny + (z - s.p[0].z) * nz > -0.01) { return false; }
    }
    return true;
}
int zaehle(const std::vector<ImportPinsel>& v, double x, double y, double z) {
    int n = 0;
    for (const ImportPinsel& b : v) { n += imPinsel(b, x, y, z) ? 1 : 0; }
    return n;
}

const char* kWuerfel =
    "o Wuerfel\n"
    "v 0 0 0\nv 64 0 0\nv 64 64 0\nv 0 64 0\nv 0 0 64\nv 64 0 64\nv 64 64 64\nv 0 64 64\n"
    "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nusemtl Stein.TGA\n"
    // Umlauf gegen den Uhrzeiger von aussen
    "f 1/1 4/4 3/3 2/2\nf 5/1 6/2 7/3 8/4\nf 1/1 2/2 6/3 5/4\nf 2/1 3/2 7/3 6/4\nf 3/1 4/2 8/3 7/4\nf 4/1 1/2 5/3 8/4\n";

std::string base64(const std::vector<unsigned char>& d) {
    static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string r;
    for (std::size_t i = 0; i < d.size(); i += 3) {
        const unsigned v = (static_cast<unsigned>(d[i]) << 16) | ((i + 1 < d.size() ? d[i + 1] : 0U) << 8) | (i + 2 < d.size() ? d[i + 2] : 0U);
        r += t[(v >> 18) & 63];
        r += t[(v >> 12) & 63];
        r += i + 1 < d.size() ? t[(v >> 6) & 63] : '=';
        r += i + 2 < d.size() ? t[v & 63] : '=';
    }
    return r;
}
}  // namespace

int main() {
    ZerlegOptionen opt;
    opt.shaderVorsatz = "textures/import/";
    NetzOptionen ohneDrehung;
    ohneDrehung.yOben = false;
    std::string err;

    std::printf("--- Wuerfel (obj) ---\n");
    ImportNetz n;
    expect("obj gelesen", netzLesen(kWuerfel, "obj", ohneDrehung, n, &err) && n.teile.size() == 1);
    if (!n.teile.empty()) {
        const ImportTeil& t = n.teile[0];
        expect("geschlossen", teilGeschlossen(t));
        const TeilErgebnis r = teilUmwandeln(t, TeilArt::Auto, opt);
        expect("Auto: Brushes, 1 Brush mit 6 Seiten, 6 Ebenen aus 12 Dreiecken",
               r.art == TeilArt::Pinsel && r.pinsel.size() == 1 && r.pinsel[0].seiten.size() == 6 && r.ebenen == 6 && r.dreiecke == 12);
        const std::vector<ImportSeite> seiten = r.pinsel.empty() ? std::vector<ImportSeite>{} : r.pinsel[0].seiten;
        bool alle = !seiten.empty();
        for (const ImportSeite& s : seiten) { alle = alle && s.shader == "textures/import/stein" && s.uvGesetzt; }
        expect("jede Seite: Material als Shader (textures/import/stein), UV gesetzt", alle);
        expect("Mitte im Brush, aussen nicht", zaehle(r.pinsel, 32, 32, 32) == 1 && zaehle(r.pinsel, 70, 32, 32) == 0);
        // UV der Unterseite: vt = (x/64, y/64) -> st = (x/64, 1 - y/64)
        bool st = false;
        for (const ImportSeite& s : seiten) {
            if (std::fabs(s.p[0].z) + std::fabs(s.p[1].z) + std::fabs(s.p[2].z) > 1e-6) { continue; }
            st = true;
            for (std::size_t k = 0; k < 3; ++k) {
                st = st && std::fabs(s.st[k].x - s.p[k].x / 64) < 1e-6 && std::fabs(s.st[k].y - (1 - s.p[k].y / 64)) < 1e-6;
            }
        }
        expect("UV der Unterseite: st = (x/64, 1 - y/64) an allen drei Ebenenpunkten (t nach unten)", st);
        expect("64er Wuerfel ist nicht detail (Grenze 64)", !r.detail);
    }

    std::printf("--- L-Koerper (konvexe Zerlegung) ---\n");
    // L-Grundriss (0,0)-(128,0)-(128,64)-(64,64)-(64,128)-(0,128), Hoehe 32
    std::string l = "o L\n";
    const int gx[6] = {0, 128, 128, 64, 64, 0}, gy[6] = {0, 0, 64, 64, 128, 128};
    for (int z = 0; z < 2; ++z) {
        for (int i = 0; i < 6; ++i) { l += "v " + std::to_string(gx[i]) + " " + std::to_string(gy[i]) + " " + std::to_string(z * 32) + "\n"; }
    }
    l += "f 1 6 5 4 3 2\nf 7 8 9 10 11 12\n";   // unten (nach -z), oben (nach +z)
    for (int i = 0; i < 6; ++i) {
        const int a = i + 1, b = (i + 1) % 6 + 1;
        l += "f " + std::to_string(a) + " " + std::to_string(b) + " " + std::to_string(b + 6) + " " + std::to_string(a + 6) + "\n";
    }
    ImportNetz nl;
    expect("L gelesen", netzLesen(l, "obj", ohneDrehung, nl, &err) && nl.teile.size() == 1);
    if (!nl.teile.empty()) {
        const TeilErgebnis r = teilUmwandeln(nl.teile[0], TeilArt::Pinsel, opt);
        expect("geschlossen, 2 bis 3 konvexe Brushes", r.geschlossen && r.pinsel.size() >= 2 && r.pinsel.size() <= 3);
        expect("Innenpunkte in genau einem Brush",
               zaehle(r.pinsel, 100, 30, 16) == 1 && zaehle(r.pinsel, 30, 100, 16) == 1 && zaehle(r.pinsel, 30, 30, 16) == 1);
        expect("die Kerbe (100,100) in keinem Brush", zaehle(r.pinsel, 100, 100, 16) == 0);
        int caulk = 0, mat = 0;
        for (const ImportPinsel& b : r.pinsel) {
            for (const ImportSeite& s : b.seiten) { (s.shader == opt.caulk ? caulk : mat) += 1; }
        }
        std::printf("      %zu Brushes, %d Seiten aussen, %d caulk\n", r.pinsel.size(), mat, caulk);
        expect("die ganz verdeckte Schnittseite caulk, die Aussenseiten nicht", caulk >= 1 && mat >= 8);
    }

    std::printf("--- offene Flaeche ---\n");
    const char* platte = "o Platte\nv 0 0 0\nv 128 0 0\nv 128 128 0\nv 0 128 0\nusemtl holz\nf 1 2 3\nf 1 3 4\n";
    ImportNetz np;
    expect("Platte gelesen", netzLesen(platte, "obj", ohneDrehung, np, &err) && np.teile.size() == 1);
    if (!np.teile.empty()) {
        expect("offen", !teilGeschlossen(np.teile[0]));
        const TeilErgebnis a = teilUmwandeln(np.teile[0], TeilArt::Auto, opt);
        expect("Auto: Modell+Clip, Clip-Shader", a.art == TeilArt::ModellClip && !a.pinsel.empty() && a.pinsel[0].seiten[0].shader == opt.clip);
        const TeilErgebnis b = teilUmwandeln(np.teile[0], TeilArt::Pinsel, opt);
        expect("Brushes: zwei Dreiecke zu einer Flaeche -> ein Prisma mit 6 Seiten, detail",
               b.pinsel.size() == 1 && b.pinsel[0].seiten.size() == 6 && b.pinsel[0].detail);
        expect("Prisma: 4 Einheiten unter der Oberseite drin (Dicke 8), 9 darunter nicht",
               zaehle(b.pinsel, 64, 64, -4) == 1 && zaehle(b.pinsel, 64, 64, -9) == 0);
        int holz = 0;
        for (const ImportSeite& s : b.pinsel.empty() ? std::vector<ImportSeite>{} : b.pinsel[0].seiten) { holz += s.shader == "textures/import/holz" ? 1 : 0; }
        expect("nur die Oberseite traegt das Material, der Rest caulk", holz == 1);
    }

    std::printf("--- glTF (eingebettet), Y oben, Skala 2 ---\n");
    {
        const float pos[9] = {0, 0, 0, 64, 0, 0, 0, 64, 0};   // Dreieck in glTF-Koordinaten (Y oben)
        const unsigned short idx[3] = {0, 1, 2};
        std::vector<unsigned char> buf(sizeof(pos) + sizeof(idx) + 2);
        std::memcpy(buf.data(), pos, sizeof(pos));
        std::memcpy(buf.data() + sizeof(pos), idx, sizeof(idx));
        const std::string js =
            std::string("{\"asset\":{\"version\":\"2.0\"},\"scene\":0,\"scenes\":[{\"nodes\":[0]}],\"nodes\":[{\"mesh\":0,\"name\":\"Dreieck\"}],") +
            "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1}]}]," +
            "\"buffers\":[{\"byteLength\":44,\"uri\":\"data:application/octet-stream;base64," + base64(buf) + "\"}]," +
            "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},{\"buffer\":0,\"byteOffset\":36,\"byteLength\":6}]," +
            "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\",\"min\":[0,0,0],\"max\":[64,64,0]}," +
            "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}]}";
        ImportNetz ng;
        NetzOptionen yo;
        yo.skala = 2.0;
        const bool ok = netzLesen(js, "gltf", yo, ng, &err);
        if (!ok) { std::printf("      %s\n", err.c_str()); }
        expect("gltf gelesen, ein Teil", ok && ng.teile.size() == 1);
        bool hoch = false;
        for (const ImportPunkt& p : ng.teile.empty() ? std::vector<ImportPunkt>{} : ng.teile[0].punkte) {
            hoch = hoch || (std::fabs(p.z - 128) < 1e-4 && std::fabs(p.y) < 1e-4 && std::fabs(p.x) < 1e-4);
        }
        expect("glTF (0,64,0) wird mit Skala 2 zu Q3 (0,0,128)", hoch);
    }

    std::printf("--- Einfuegen in eine Karte (Undo) ---\n");
    if (!nl.teile.empty() && !np.teile.empty()) {
        Karte k;
        (void)karteLesen("{\n\"classname\" \"worldspawn\"\n}\n", k);
        k.anmelden();
        const std::string vorher = karteSchreiben(k);
        std::vector<TeilErgebnis> teile = {teilUmwandeln(nl.teile[0], TeilArt::Pinsel, opt), teilUmwandeln(np.teile[0], TeilArt::ModellClip, opt)};
        const EinfuegeErgebnis e = importEinfuegen(k, teile, "models/import/platte.obj");
        const std::string nach = karteSchreiben(k);
        expect("2 L-Brushes + 1 Clip-Brush, ein misc_model, alles gewaehlt", e.pinsel == 3 && e.modelle == 1 && k.auswahl.anzahl() == 4);
        expect("misc_model mit dem Pfad, Clip und caulk in der Karte",
               nach.find("\"model\" \"models/import/platte.obj\"") != std::string::npos && nach.find("system/clip") != std::string::npos &&
                   nach.find("system/caulk") != std::string::npos);
        (void)k.undo.undo();
        expect("eine Undo-Stufe stellt die Karte wieder her", karteSchreiben(k) == vorher);
        std::vector<const ImportTeil*> mt = {&np.teile[0]};
        const std::string obj = modellObj(mt, opt);
        ImportNetz zurueck;
        expect("das Modell als .obj: wieder lesbar, gleiches Material (usemtl = Shader)",
               netzLesen(obj, "obj", ohneDrehung, zurueck, &err) && zurueck.teile.size() == 1 && zurueck.teile[0].dreiecke.size() == 2 &&
                   obj.find("usemtl textures/import/holz") != std::string::npos);
    }

    std::printf("--- BSPs anderer id-Tech-3-Spiele ---\n");
    {
        const auto kopf = [](const char* id, int v) { return std::string(id) + std::string(1, static_cast<char>(v)) + std::string(3, '\0') + "rest"; };
        expect("RBSP -> eigener Typ, IBSP 46 -> quake3, IBSP 47 -> wolf, FBSP -> qfusion, kurz -> eigener",
               q3map2TypFuerBsp(kopf("RBSP", 1), "ja") == "ja" && q3map2TypFuerBsp(kopf("IBSP", 46), "ja") == "quake3" &&
                   q3map2TypFuerBsp(kopf("IBSP", 47), "jk2") == "wolf" && q3map2TypFuerBsp(kopf("FBSP", 1), "ja") == "qfusion" &&
                   q3map2TypFuerBsp("IB", "ja") == "ja");
    }

    std::printf("--- Blender hin und zurueck (Punkt 10) ---\n");
    if (!n.teile.empty()) {
        const auto gleich = [](const ImportTeil& a, const ImportTeil& b) {
            if (a.punkte.size() != b.punkte.size() || a.dreiecke.size() != b.dreiecke.size()) { return false; }
            for (std::size_t i = 0; i < a.punkte.size(); ++i) {
                if (std::fabs(a.punkte[i].x - b.punkte[i].x) + std::fabs(a.punkte[i].y - b.punkte[i].y) + std::fabs(a.punkte[i].z - b.punkte[i].z) > 1e-3) {
                    return false;
                }
            }
            return true;
        };
        const std::string glb = glbSchreiben(n, true);
        ImportNetz zurueck;
        const bool ok = netzLesen(glb, "glb", NetzOptionen{}, zurueck, &err);   // Y oben, Skala 1 (Vorgabe des Dialogs)
        if (!ok) { std::printf("      %s\n", err.c_str()); }
        expect(".glb (Y oben) wieder eingelesen: dieselben Punkte und Dreiecke, Material, UV",
               ok && zurueck.teile.size() == 1 && gleich(n.teile[0], zurueck.teile[0]) && zurueck.teile[0].material == n.teile[0].material &&
                   zurueck.teile[0].hatUv && std::fabs(zurueck.teile[0].uv[0][1] - n.teile[0].uv[0][1]) < 1e-5);
    }

    std::printf("--- Abweisen ---\n");
    ImportNetz nx;
    expect("kaputte .fbx: false mit Grund", !netzLesen("Kaydara FBX Binary  \x00\x1a\x00 kaputt", "fbx", NetzOptionen{}, nx, &err) && !err.empty());
    expect("unbekannte Endung: false", !netzLesen("x", "3ds", NetzOptionen{}, nx, &err));
    expect("materialShader: Pfad und Endung weg, klein, Sonderzeichen _",
           materialShader("C:\\tex\\Mauer Alt.PNG", opt) == "textures/import/mauer_alt" && materialShader("", opt) == opt.ohneMaterial);

    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
