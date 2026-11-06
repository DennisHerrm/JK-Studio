// kartemd3import.cpp - Stufe 9 Punkt 3: Modell-Import, MD3 samt Shader (ctest karte_md3import)
//
// Der Wuerfel aus einer .obj (mit einer Textur) wird ein MD3-Paket; der Leser des Programms
// (bhed/md3.h, wie R_LoadMD3) liest es zurueck: eine Flaeche, die Punkte am richtigen Ort,
// Umlauf entgegen der Normalen (wie JKA), Normalen nach aussen, Shader = Texturpfad ohne
// Endung, die Textur und das Shader-Skript im Paket. Dazu: Normalen-Kodierung hin und
// zurueck, ein grosses Modell wird verkleinert (modelscale), viele Punkte werden auf mehrere
// Flaechen geteilt (hoechstens 1000 Punkte).
#include <cmath>
#include <cstdio>
#include <set>
#include <string>

#include "bhed/karte/modell.h"
#include "bhed/karte/modellimport.h"
#include "bhed/md3.h"

namespace {
int g_fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++g_fehler; }
}
const char* kWuerfel =
    "o Wuerfel\n"
    "v -32 -32 0\nv 32 -32 0\nv 32 32 0\nv -32 32 0\nv -32 -32 64\nv 32 -32 64\nv 32 32 64\nv -32 32 64\n"
    "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nusemtl Stein\n"
    "f 1/1 4/4 3/3 2/2\nf 5/1 6/2 7/3 8/4\nf 1/1 2/2 6/3 5/4\nf 2/1 3/2 7/3 6/4\nf 3/1 4/2 8/3 7/4\nf 4/1 1/2 5/3 8/4\n";
}  // namespace

int main() {
    using namespace bhed;
    using namespace bhed::karte;
    std::printf("--- Normalen-Kodierung ---\n");
    double mx = 0;
    const double proben[6][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, -1}, {0.6, 0.0, 0.8}, {-0.48, 0.6, -0.64}};
    for (const auto& n : proben) {
        double x = 0, y = 0, z = 0;
        md3NormaleAuspacken(md3Normale(n[0], n[1], n[2]), x, y, z);
        mx = std::max(mx, std::fabs(x - n[0]) + std::fabs(y - n[1]) + std::fabs(z - n[2]));
    }
    expect("hin und zurueck auf 0.06 genau (256 Stufen)", mx < 0.06);

    std::printf("--- Wuerfel -> MD3 ---\n");
    ImportNetz n;
    NetzOptionen o;
    o.yOben = false;
    std::string err;
    expect("obj gelesen", netzLesen(kWuerfel, "obj", o, n, &err) && n.teile.size() == 1);
    if (n.teile.empty()) { return 1; }
    n.teile[0].textur = "Bilder/Stein Wand.PNG";
    n.teile[0].texturDaten = std::string("\x89PNG\r\n\x1a\n-test-", 14);
    n.teile[0].texturEndung = "png";
    const Md3Paket p = md3Paket(n, "Mein Wuerfel");
    expect("kein Fehler, Pfad models/import/mein_wuerfel/mein_wuerfel.md3", p.fehler.empty() && p.modellPfad == "models/import/mein_wuerfel/mein_wuerfel.md3");
    bool bild = false, skript = false;
    for (const Md3Paket::Datei& d : p.dateien) {
        bild = bild || (d.pfad == "models/import/mein_wuerfel/stein_wand.png" && d.inhalt.size() == 14);
        // Stufe 9d: JA, JK2 und MD lesen Shader aus shaders/ (tr_shader.cpp ScanAndLoadShaderFiles( "shaders" ))
        skript = skript || (d.pfad == "shaders/mein_wuerfel.shader" && d.inhalt.find("models/import/mein_wuerfel/stein_wand\n{") != std::string::npos &&
                            d.inhalt.find("map models/import/mein_wuerfel/stein_wand.png") != std::string::npos &&
                            d.inhalt.find("qer_editorimage models/import/mein_wuerfel/stein_wand.png") != std::string::npos);
    }
    expect("Textur im Paket (Name sauber), Shader-Skript unter shaders/ mit qer_editorimage und map", bild && skript);
    // Stufe 9d: ein frueherer Import legte scripts/<stamm>.shader ab - im Archiv fuer das Spiel steht er unter shaders/
    expect("Archivname: scripts/x.shader -> shaders/x.shader, alles andere bleibt",
           importArchivName("scripts/alt.shader") == "shaders/alt.shader" && importArchivName("shaders/neu.shader") == "shaders/neu.shader" &&
               importArchivName("models/import/a/a.md3") == "models/import/a/a.md3" && importArchivName("scripts/notiz.txt") == "scripts/notiz.txt");
    Md3Model m;
    const bool gelesen = !p.dateien.empty() && readMd3(p.dateien[0].inhalt, m, &err);
    if (!gelesen) { std::printf("      %s\n", err.c_str()); }
    expect("readMd3 liest es: 1 Bild, 1 Flaeche, 12 Dreiecke, 24 Punkte (UV-Naehte)",
           gelesen && m.numFrames == 1 && m.surfaces.size() == 1 && m.surfaces[0].indexes.size() == 36 && m.surfaces[0].numVerts == 24);
    if (gelesen && !m.surfaces.empty()) {
        const Md3Surface& s = m.surfaces[0];
        expect("Shader = Texturpfad ohne Endung", s.shader == "models/import/mein_wuerfel/stein_wand");
        bool ecke = false, umlauf = true, aussen = true;
        for (const Md3Vertex& v : s.verts) {
            ecke = ecke || (std::fabs(v.xyz[0] - 32) < 1e-3 && std::fabs(v.xyz[1] - 32) < 1e-3 && std::fabs(v.xyz[2] - 64) < 1e-3);
            const double d = v.normal[0] * v.xyz[0] + v.normal[1] * v.xyz[1] + v.normal[2] * (v.xyz[2] - 32);
            aussen = aussen && d > 0;
        }
        for (std::size_t i = 0; i + 2 < s.indexes.size(); i += 3) {
            const Md3Vertex &a = s.verts[s.indexes[i]], &b = s.verts[s.indexes[i + 1]], &c = s.verts[s.indexes[i + 2]];
            const double ux = b.xyz[0] - a.xyz[0], uy = b.xyz[1] - a.xyz[1], uz = b.xyz[2] - a.xyz[2];
            const double vx = c.xyz[0] - a.xyz[0], vy = c.xyz[1] - a.xyz[1], vz = c.xyz[2] - a.xyz[2];
            const double cx = uy * vz - uz * vy, cy = uz * vx - ux * vz, cz = ux * vy - uy * vx;
            const double mxp = (a.xyz[0] + b.xyz[0] + c.xyz[0]) / 3, myp = (a.xyz[1] + b.xyz[1] + c.xyz[1]) / 3, mzp = (a.xyz[2] + b.xyz[2] + c.xyz[2]) / 3 - 32;
            umlauf = umlauf && (cx * mxp + cy * myp + cz * mzp) < 0;
        }
        expect("Ecke (32,32,64) am Ort (xyz / 64)", ecke);
        expect("Normalen zeigen nach aussen", aussen);
        expect("Umlauf entgegen der Normalen (wie JKA: GL_Cull FRONT)", umlauf);
    }
    expect("klein genug: modelscale 1", p.modelscale == 1.0);

    std::printf("--- gross und viele Punkte ---\n");
    ImportNetz g = n;
    for (ImportPunkt& q : g.teile[0].punkte) {
        q.x *= 40;
        q.y *= 40;
        q.z *= 40;
    }
    const Md3Paket pg = md3Paket(g, "gross");
    Md3Model mg;
    const bool gg = pg.fehler.empty() && readMd3(pg.dateien[0].inhalt, mg, &err);
    float maxXyz = 0;
    for (const Md3Vertex& v : gg ? mg.surfaces[0].verts : std::vector<Md3Vertex>{}) {
        for (float c : v.xyz) { maxXyz = std::max(maxXyz, std::fabs(c)); }
    }
    expect("2560 Einheiten: verkleinert (modelscale ~5), Punkte <= 511", gg && pg.modelscale > 4.9 && pg.modelscale < 5.1 && maxXyz <= 511.0F);
    // ein Gitter mit 41 x 41 Punkten (1681, 3200 Dreiecke) als eine Flaeche
    ImportNetz gitter;
    gitter.teile.resize(1);
    ImportTeil& t = gitter.teile[0];
    t.material = "boden";
    for (int y = 0; y <= 40; ++y) {
        for (int x = 0; x <= 40; ++x) {
            t.punkte.push_back({static_cast<double>(x * 8), static_cast<double>(y * 8), 0});
            t.uv.push_back({x / 40.0, y / 40.0});
        }
    }
    for (std::uint32_t y = 0; y < 40; ++y) {
        for (std::uint32_t x = 0; x < 40; ++x) {
            const std::uint32_t a = y * 41 + x;
            t.dreiecke.push_back({a, a + 1, a + 42});
            t.dreiecke.push_back({a, a + 42, a + 41});
        }
    }
    const Md3Paket pt = md3Paket(gitter, "gitter");
    Md3Model mt;
    bool klein = pt.fehler.empty() && readMd3(pt.dateien[0].inhalt, mt, &err) && mt.surfaces.size() >= 2;
    std::size_t summe = 0;
    for (const Md3Surface& s : klein ? mt.surfaces : std::vector<Md3Surface>{}) {
        klein = klein && s.numVerts <= 1000 && s.indexes.size() <= 6000 && s.shader == "models/import/gitter/boden";
        summe += s.indexes.size() / 3;
    }
    std::printf("      %zu Flaechen\n", mt.surfaces.size());
    expect("3200 Dreiecke auf mehrere Flaechen (je <= 1000 Punkte, <= 2000 Dreiecke), keins verloren", klein && summe == 3200);
    // rc610 MODOBJ: ein Modell, wie der Editor es zeigt (Z oben, Umlauf wie Q3), als Paket
    std::printf("--- md3PaketAusModell ---\n");
    Modell mo;
    ModellFlaeche fa;
    fa.shader = "models/pack/holz";
    fa.punkte = {{Vektor3f(0, 0, 0), Vektor3f(0, 0, 1), Vektor2f(0, 0)}, {Vektor3f(64, 0, 0), Vektor3f(0, 0, 1), Vektor2f(1, 0)},
                 {Vektor3f(0, 64, 0), Vektor3f(0, 0, 1), Vektor2f(0, 1)}};
    fa.indizes = {0, 2, 1};
    ModellFlaeche fb = fa;
    fb.shader = "textures/spiel/skript";
    for (ModellPunkt& q : fb.punkte) { q.ort[2] = 32; }
    mo.flaechen = {fa, fb};
    std::vector<std::string> gefragt;
    const Md3Paket pm = md3PaketAusModell(mo, importStamm("models/Pack/Baum Gross.OBJ"), [&gefragt](const std::string& s, std::string& bild, std::string& b) {
        gefragt.push_back(s);
        if (s != "models/pack/holz") { return false; }
        bild = "models/pack/holz.jpg";
        b = "jpg-bytes";
        return true;
    });
    expect("Stamm aus dem Pfad ohne models/, mit Endung: pack_baum_gross_obj", importStamm("models/Pack/Baum Gross.OBJ") == "pack_baum_gross_obj");
    expect("lange Stamme: 20 Zeichen, verschiedene bleiben verschieden",
           importStamm("models/a/sehr_langer_ordnername/noch_ein_langer_dateiname.obj").size() == 20 &&
               importStamm("models/a/sehr_langer_ordnername/noch_ein_langer_dateiname.obj") != importStamm("models/a/sehr_langer_ordnername/noch_ein_langer_dateiname.fbx"));
    expect("je Flaeche gefragt, kein Fehler", gefragt.size() == 2 && pm.fehler.empty() && pm.modellPfad == "models/import/pack_baum_gross_obj/pack_baum_gross_obj.md3");
    bool jpg = false, skriptHolz = false, skriptSpiel = false;
    for (const Md3Paket::Datei& d : pm.dateien) {
        jpg = jpg || (d.pfad == "models/import/pack_baum_gross_obj/holz.jpg" && d.inhalt == "jpg-bytes");
        if (d.pfad == "shaders/pack_baum_gross_obj.shader") {
            skriptHolz = d.inhalt.find("models/import/pack_baum_gross_obj/holz\n{") != std::string::npos;
            skriptSpiel = d.inhalt.find("textures/spiel/skript") != std::string::npos;
        }
    }
    expect("das Bild im Paket, im Skript nur der neue Shader (der des Spiels bleibt draussen)", jpg && skriptHolz && !skriptSpiel);
    Md3Model mma;
    const bool mok = !pm.dateien.empty() && readMd3(pm.dateien[0].inhalt, mma, &err) && mma.surfaces.size() == 2;
    expect("zwei Flaechen: holz neu benannt, der Shader des Spiels unveraendert",
           mok && mma.surfaces[0].shader == "models/import/pack_baum_gross_obj/holz" && mma.surfaces[1].shader == "textures/spiel/skript");
    if (mok) {
        const Md3Surface& s = mma.surfaces[0];
        bool gleich = s.indexes.size() == 3 && s.verts.size() == 3;
        for (std::size_t k = 0; gleich && k < 3; ++k) {
            const Md3Vertex& v = s.verts[s.indexes[k]];
            const ModellPunkt& q = fa.punkte[fa.indizes[k]];
            gleich = std::fabs(v.xyz[0] - q.ort[0]) < 1e-3 && std::fabs(v.xyz[1] - q.ort[1]) < 1e-3 && std::fabs(v.xyz[2] - q.ort[2]) < 1e-3 &&
                     std::fabs(v.st[0] - q.st[0]) < 1e-6 && std::fabs(v.st[1] - q.st[1]) < 1e-6 && std::fabs(v.normal[2] - 1.0) < 0.01;
        }
        expect("Umlauf, Orte, st und Normalen genau wie im Modell", gleich);
        expect("ohne Bild kein Skript-Eintrag; nur Spiel-Shader: keine .shader-Datei",
               md3PaketAusModell(Modell{{fb}, Kasten(), false}, "nur_spiel", [](const std::string&, std::string&, std::string&) { return false; }).dateien.size() == 1);
    }
    std::printf("--- rc610 R2a: gleichnamige Bilder aus verschiedenen Ordnern ---\n");
    {
        // drei Flaechen: Bild "holz.jpg" aus a/ und aus b/ (verschiedene Bytes), dann a/holz.jpg noch einmal
        ModellFlaeche f1 = fa;
        f1.shader = "models/a/holz";
        ModellFlaeche f2 = fa;
        f2.shader = "models/b/holz";
        for (ModellPunkt& q : f2.punkte) { q.ort[2] = 16; }
        ModellFlaeche f3 = fa;
        f3.shader = "models/a/holz";
        for (ModellPunkt& q : f3.punkte) { q.ort[2] = 48; }
        Modell zwei;
        zwei.flaechen = {f1, f2, f3};
        const Md3Paket pz = md3PaketAusModell(zwei, "zwei_holz", [](const std::string& sh, std::string& bild, std::string& b) {
            bild = sh + ".jpg";
            b = sh == "models/a/holz" ? "bytes-a" : "bytes-b";
            return true;
        });
        Md3Model mz;
        const bool zok = pz.fehler.empty() && !pz.dateien.empty() && readMd3(pz.dateien[0].inhalt, mz, &err) && mz.surfaces.size() == 2;
        std::set<std::string> bilder;
        for (const Md3Paket::Datei& d : pz.dateien) {
            if (d.pfad.size() > 4 && d.pfad.compare(d.pfad.size() - 4, 4, ".jpg") == 0) { bilder.insert(d.pfad + "=" + d.inhalt); }
        }
        expect("zwei verschiedene Bilder gleichen Namens: zwei Shader, zwei Bilddateien mit den eigenen Bytes; dasselbe Bild nochmal: derselbe Shader",
               zok && mz.surfaces[0].shader != mz.surfaces[1].shader && bilder.size() == 2 && bilder.count("models/import/zwei_holz/holz.jpg=bytes-a") == 1);
        if (zok) { std::printf("    Shader: %s, %s\n", mz.surfaces[0].shader.c_str(), mz.surfaces[1].shader.c_str()); }
    }
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
