// selbsttest_aseb.inc - ASEB: .ase, .md3 und die weiteren Modellformate werden Brushes (Datei > Geometrie als Brushes
// importieren...), Rechtsklick auf ein misc_model "Modell in Brushes umwandeln" (BHED_EDITORTEST=karteneditor, BHED_KARTE_NUR=aseb).
//
// Anlass (Tester GP, Discord): "This update now have .ase to brush or not?" - rc610 meldete "unsupported format .ase".
// Der Test schreibt seine Modelle selbst in einen Ordner des Laufs (Extra Resource Path, keine Dateien aus dem Spiel):
//   models/aseb/truhe.ase  wie aus 3ds Max fuer JKA: ein Multimaterial (holz mit absolutem *BITMAP-Pfad bis base\textures,
//                          textures/aseb/metall als Materialname mit eigenem Bild), ein zweites Material griff (Bild ohne Ordner);
//                          zwei geschlossene Kaesten (Truhe 64 x 48 x 32: Seiten holz, oben/unten metall; Griff 16 x 4 x 12)
//   models/aseb/stein.md3  ein Wuerfel aus zwei Flaechen: textures/aseb/stein (fuenf Seiten) und kante (oben, Shader ohne Ordner)
//   models/aseb/figur.glm  nur der Name: .glm bleibt mit klarer Meldung draussen
// Gegenprobe ist q3map2 selbst: bhed_q3map2 -bhedmodels laedt beide Modelle genau wie InsertModel (rc609 KEMD) - Shader je Flaeche,
// Dreiecke, Punkte. Geprueft: die Teile (je Koerper einer, geschlossen), Dreiecke je Shader und die Shader der Brush-Seiten wie
// q3map2, der Huellkasten der Brushes gleich dem des Modells; Rechtsklick auf ein gespiegeltes, gedrehtes misc_model (auch aus dem
// Menue File) ergibt Brushes an derselben Stelle, das misc_model ist weg, ein Undo nimmt alles zurueck; Tooltip und Helfer.
// Eingebunden in gui/selbsttest_handbuch.inc (nach MODOBJ).
namespace aseb {

namespace ke = karteneditor;
using stufe3::konsoleText;

std::string quelle() { return stufe2::laufDatei("aseb_quelle"); }
std::string datei(const std::string& name) { return quelle() + "\\models\\aseb\\" + name; }

// Ein Kasten: 8 Ecken wie die Box von 3ds Max (unten 0..3, oben 4..7; je -x/-y, +x/-y, -x/+y, +x/+y), 12 Dreiecke von aussen
// gegen den Uhrzeiger (Z oben), je zwei eine Seite: unten, oben, vorn (-y), rechts (+x), hinten (+y), links (-x).
struct Kasten6 {
    float a[3];
    float b[3];
};
std::array<std::array<float, 3>, 8> ecken(const Kasten6& k) {
    std::array<std::array<float, 3>, 8> e{};
    for (int i = 0; i < 8; ++i) {
        e[static_cast<std::size_t>(i)] = {(i & 1) != 0 ? k.b[0] : k.a[0], (i & 2) != 0 ? k.b[1] : k.a[1], (i & 4) != 0 ? k.b[2] : k.a[2]};
    }
    return e;
}
constexpr int kFlaechen[12][3] = {{0, 2, 3}, {3, 1, 0}, {4, 5, 7}, {7, 6, 4}, {0, 1, 5}, {5, 4, 0},
                                  {1, 3, 7}, {7, 5, 1}, {3, 2, 6}, {6, 7, 3}, {2, 0, 4}, {4, 6, 2}};

// Ein GEOMOBJECT wie der ASE-Export von 3ds Max (NODE_TM Einheit, Ecken in Weltlage, je Seite die ganze Textur)
std::string aseObjekt(const std::string& name, const Kasten6& k, int materialRef, int mtlUntenOben, int mtlSeiten) {
    char z[256];
    std::string t = "*GEOMOBJECT {\n\t*NODE_NAME \"" + name + "\"\n\t*NODE_TM {\n\t\t*NODE_NAME \"" + name + "\"\n" +
                    "\t\t*INHERIT_POS 0 0 0\n\t\t*INHERIT_ROT 0 0 0\n\t\t*INHERIT_SCL 0 0 0\n\t\t*TM_ROW0 1.0000\t0.0000\t0.0000\n"
                    "\t\t*TM_ROW1 0.0000\t1.0000\t0.0000\n\t\t*TM_ROW2 0.0000\t0.0000\t1.0000\n\t\t*TM_ROW3 0.0000\t0.0000\t0.0000\n"
                    "\t\t*TM_POS 0.0000\t0.0000\t0.0000\n\t\t*TM_ROTAXIS 0.0000\t0.0000\t0.0000\n\t\t*TM_ROTANGLE 0.0000\n"
                    "\t\t*TM_SCALE 1.0000\t1.0000\t1.0000\n\t\t*TM_SCALEAXIS 0.0000\t0.0000\t0.0000\n\t\t*TM_SCALEAXISANG 0.0000\n\t}\n"
                    "\t*MESH {\n\t\t*TIMEVALUE 0\n\t\t*MESH_NUMVERTEX 8\n\t\t*MESH_NUMFACES 12\n\t\t*MESH_VERTEX_LIST {\n";
    const auto e = ecken(k);
    for (int i = 0; i < 8; ++i) {
        const auto& p = e[static_cast<std::size_t>(i)];
        std::snprintf(z, sizeof(z), "\t\t\t*MESH_VERTEX %4d\t%.4f\t%.4f\t%.4f\n", i, static_cast<double>(p[0]), static_cast<double>(p[1]), static_cast<double>(p[2]));
        t += z;
    }
    t += "\t\t}\n\t\t*MESH_FACE_LIST {\n";
    for (int f = 0; f < 12; ++f) {
        std::snprintf(z, sizeof(z), "\t\t\t*MESH_FACE %4d:    A: %4d B: %4d C: %4d AB:    1 BC:    1 CA:    0\t *MESH_SMOOTHING %d \t*MESH_MTLID %d\n", f,
                      kFlaechen[f][0], kFlaechen[f][1], kFlaechen[f][2], f / 2 + 1, f < 4 ? mtlUntenOben : mtlSeiten);
        t += z;
    }
    // je Seite (zwei Dreiecke q0 q1 q2 / q2 q3 q0): die Textur einmal
    t += "\t\t}\n\t\t*MESH_NUMTVERTEX 4\n\t\t*MESH_TVERTLIST {\n\t\t\t*MESH_TVERT 0\t0.0000\t0.0000\t0.0000\n\t\t\t*MESH_TVERT 1\t1.0000\t0.0000\t0.0000\n"
         "\t\t\t*MESH_TVERT 2\t1.0000\t1.0000\t0.0000\n\t\t\t*MESH_TVERT 3\t0.0000\t1.0000\t0.0000\n\t\t}\n\t\t*MESH_NUMTVFACES 12\n\t\t*MESH_TFACELIST {\n";
    for (int f = 0; f < 12; ++f) {
        std::snprintf(z, sizeof(z), "\t\t\t*MESH_TFACE %d\t%d\t%d\t%d\n", f, f % 2 == 0 ? 0 : 2, f % 2 == 0 ? 1 : 3, f % 2 == 0 ? 2 : 0);
        t += z;
    }
    std::snprintf(z, sizeof(z), "\t\t}\n\t}\n\t*PROP_MOTIONBLUR 0\n\t*PROP_CASTSHADOW 1\n\t*PROP_RECVSHADOW 1\n\t*MATERIAL_REF %d\n}\n", materialRef);
    return t + z;
}

std::string aseMaterial(const char* art, const std::string& name, const std::string& bild) {
    return std::string("\t\t*MATERIAL_NAME \"") + name + "\"\n\t\t*MATERIAL_CLASS \"" + art + "\"\n\t\t*MATERIAL_AMBIENT 0.5880\t0.5880\t0.5880\n" +
           "\t\t*MATERIAL_DIFFUSE 0.5880\t0.5880\t0.5880\n\t\t*MATERIAL_SPECULAR 0.9000\t0.9000\t0.9000\n\t\t*MATERIAL_SHINE 0.1000\n" +
           "\t\t*MATERIAL_SHINESTRENGTH 0.0000\n\t\t*MATERIAL_TRANSPARENCY 0.0000\n\t\t*MATERIAL_WIRESIZE 1.0000\n\t\t*MATERIAL_SHADING Blinn\n" +
           (bild.empty() ? std::string()
                         : "\t\t*MAP_DIFFUSE {\n\t\t\t*MAP_NAME \"Map\"\n\t\t\t*MAP_CLASS \"Bitmap\"\n\t\t\t*MAP_SUBNO 1\n\t\t\t*MAP_AMOUNT 1.0000\n"
                           "\t\t\t*BITMAP \"" + bild + "\"\n\t\t\t*MAP_TYPE Screen\n\t\t\t*UVW_U_OFFSET 0.0000\n\t\t\t*UVW_V_OFFSET 0.0000\n"
                           "\t\t\t*UVW_U_TILING 1.0000\n\t\t\t*UVW_V_TILING 1.0000\n\t\t\t*UVW_ANGLE 0.0000\n\t\t\t*BITMAP_FILTER Pyramidal\n\t\t}\n");
}

std::string truheAse() {
    std::string t = "*3DSMAX_ASCIIEXPORT\t200\n*COMMENT \"AsciiExport Version  2,00 - JK Studio self-test ASEB\"\n"
                    "*SCENE {\n\t*SCENE_FILENAME \"truhe.max\"\n\t*SCENE_FIRSTFRAME 0\n\t*SCENE_LASTFRAME 100\n\t*SCENE_FRAMESPEED 30\n"
                    "\t*SCENE_TICKSPERFRAME 160\n\t*SCENE_BACKGROUND_STATIC 0.0000\t0.0000\t0.0000\n\t*SCENE_AMBIENT_STATIC 0.0000\t0.0000\t0.0000\n}\n"
                    "*MATERIAL_LIST {\n\t*MATERIAL_COUNT 2\n\t*MATERIAL 0 {\n";
    t += aseMaterial("Multi/Sub-Object", "truhe_multi", "");
    t += "\t\t*NUMSUBMTLS 2\n\t\t*SUBMATERIAL 0 {\n" + aseMaterial("Standard", "holz", "C:\\Spiele\\Jedi Academy\\GameData\\base\\textures\\aseb\\holz.tga") +
         "\t\t}\n\t\t*SUBMATERIAL 1 {\n" + aseMaterial("Standard", "textures/aseb/metall", "metall_alt.tga") + "\t\t}\n\t}\n";
    t += "\t*MATERIAL 1 {\n" + aseMaterial("Standard", "griff", "griff.tga") + "\t}\n}\n";
    t += aseObjekt("truhe", Kasten6{{-32, -24, 0}, {32, 24, 32}}, 0, 1, 0);
    t += aseObjekt("griff", Kasten6{{-8, -28, 10}, {8, -24, 22}}, 1, 0, 0);
    return t;
}

// Der Wuerfel als .md3 (md3Paket aus zwei Teilen mit festem Shader; ImportTeil laeuft von aussen gegen den Uhrzeiger)
std::string steinMd3() {
    const auto e = ecken(Kasten6{{64, -16, 0}, {96, 16, 32}});
    karte::ImportNetz n;
    for (int teil = 0; teil < 2; ++teil) {
        karte::ImportTeil t;
        t.name = teil == 0 ? "stein" : "kante";
        t.shaderFest = teil == 0 ? "textures/aseb/stein" : "kante";
        t.hatUv = true;
        for (int f = 0; f < 12; ++f) {
            if ((f == 2 || f == 3) != (teil == 1)) { continue; }   // oben (Dreiecke 2, 3) ist die Kante
            const auto b = static_cast<std::uint32_t>(t.punkte.size());
            for (int k = 0; k < 3; ++k) {
                const auto& p = e[static_cast<std::size_t>(kFlaechen[f][k])];
                t.punkte.push_back({p[0], p[1], p[2]});
                static constexpr double kSt[2][3][2] = {{{0, 1}, {1, 1}, {1, 0}}, {{1, 0}, {0, 0}, {0, 1}}};
                t.uv.push_back({kSt[f % 2][k][0], kSt[f % 2][k][1]});
            }
            t.dreiecke.push_back({b, b + 1, b + 2});
        }
        n.teile.push_back(std::move(t));
    }
    const karte::Md3Paket p = karte::md3Paket(n, "aseb_stein");
    return p.dateien.empty() ? std::string() : p.dateien[0].inhalt;
}

// Was q3map2 aus einem Modell macht (-bhedmodels): Dreiecke je Shader, Huellkasten der Punkte
struct Q3Modell {
    bool geladen = false;
    std::map<std::string, std::size_t> dreiecke;
    std::array<float, 3> mn{1e30F, 1e30F, 1e30F};
    std::array<float, 3> mx{-1e30F, -1e30F, -1e30F};
};
struct Zustand {
    std::shared_ptr<bauplattform::Prozess> q3;
    std::string liste, aus;
    std::map<std::string, Q3Modell> modelle;   // VFS-Pfad -> was q3map2 laedt
    std::string vorher;                        // misc_model vor dem Umwandeln: Schluessel
    std::array<float, 3> mn{}, mx{};           // Soll-Huellkasten
};

std::string shaderListe(const std::map<std::string, std::size_t>& m) {
    std::string t;
    for (const auto& [s, n] : m) { t += s + " " + std::to_string(n) + ", "; }
    return t;
}

// Die Dreiecke je Shader der Teile im Fenster (dreieckShader, sonst der Shader des Teils)
std::map<std::string, std::size_t> teilDreiecke(const ke::GeoImportZustand& z) {
    std::map<std::string, std::size_t> m;
    for (const karte::ImportTeil& t : z.netz.teile) {
        for (std::size_t i = 0; i < t.dreiecke.size(); ++i) { ++m[i < t.dreieckShader.size() ? t.dreieckShader[i] : t.shaderFest]; }
    }
    return m;
}

// Die Shader der Brush-Seiten (ohne caulk) einer Liste von Pinseln bzw. der Vorschau
std::set<std::string> seitenShader(const std::vector<karte::TeilErgebnis>& v) {
    std::set<std::string> s;
    for (const auto& t : v) {
        for (const auto& p : t.pinsel) {
            for (const auto& sd : p.seiten) {
                if (sd.shader != "textures/system/caulk") { s.insert(sd.shader); }
            }
        }
    }
    return s;
}
std::set<std::string> weltShader() {
    std::set<std::string> s;
    for (const auto& p : tutgeo::weltPinsel()) {
        for (const auto& f : p->flaechen()) {
            if (f->shader() != "textures/system/caulk") { s.insert(f->shader()); }
        }
    }
    return s;
}
std::set<std::string> schluessel(const std::map<std::string, std::size_t>& m) {
    std::set<std::string> s;
    for (const auto& [k, n] : m) {
        (void)n;
        s.insert(k);
    }
    return s;
}

// Der Huellkasten aller Brushes der Welt
bool weltKasten(std::array<float, 3>& mn, std::array<float, 3>& mx) {
    mn = {1e30F, 1e30F, 1e30F};
    mx = {-1e30F, -1e30F, -1e30F};
    const auto v = tutgeo::weltPinsel();
    for (const auto& p : v) {
        const karte::Kasten& k = p->grenzen(ke::editor().karte.kontext);
        for (int a = 0; a < 3; ++a) {
            mn[static_cast<std::size_t>(a)] = std::min(mn[static_cast<std::size_t>(a)], k.origin[a] - k.extents[a]);
            mx[static_cast<std::size_t>(a)] = std::max(mx[static_cast<std::size_t>(a)], k.origin[a] + k.extents[a]);
        }
    }
    return !v.empty();
}
float abweichung(const std::array<float, 3>& a, const std::array<float, 3>& b, const std::array<float, 3>& c, const std::array<float, 3>& d) {
    float w = 0;
    for (std::size_t i = 0; i < 3; ++i) { w = std::max({w, std::fabs(a[i] - c[i]), std::fabs(b[i] - d[i])}); }
    return w;
}
std::string kastenText(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    char t[160];
    std::snprintf(t, sizeof(t), "(%g %g %g)-(%g %g %g)", static_cast<double>(a[0]), static_cast<double>(a[1]), static_cast<double>(a[2]),
                  static_cast<double>(b[0]), static_cast<double>(b[1]), static_cast<double>(b[2]));
    return t;
}

// Eine neue Karte ohne die Frage nach dem Speichern (die Aenderungen des Tests sind verworfen)
void neueKarte() {
    ke::editor().karte.undo.alsGespeichertMarkieren();
    (void)ke::ausfuehren("NewMap");
}

std::shared_ptr<karte::Entitaet> miscModel() {
    for (const auto& en : ke::editor().karte.entitaeten()) {
        if (en->klassenname() == "misc_model") { return en; }
    }
    return nullptr;
}

// handbuch: die Bilder h_aseb_menue und h_aseb_fenster (BHED_HB_NUR=aseb) - bis zum Fenster, dann Abbrechen
std::vector<Schritt> schritteMit(bool handbuch) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<Zustand>();
    const std::string fenster = "###geoimport";
    add(tu("ASEB: Modelle in den Ordner des Laufs (Extra Resource Path)", [] {
        karteDialogeZu();
        stufe2::testDateiSchreiben(datei("truhe.ase"), truheAse());
        stufe2::testDateiSchreiben(datei("stein.md3"), steinMd3());
        stufe2::testDateiSchreiben(datei("figur.glm"), "2LGM");
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0] = karte::ordnerBereinigt(quelle()); });
    }));
    add(warteBis("ASEB: Spieldaten mit dem Extra Resource Path geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit() && ke::editor().karteSpiel && ke::editor().karteSpiel->fs.finden("models/aseb/truhe.ase") != "";
    }, 20000));
    // Gegenprobe: q3map2 laedt die Modelle wie InsertModel
    add(tu("ASEB: bhed_q3map2 -bhedmodels fuer truhe.ase und stein.md3 starten", [z] {
        stufe3::q3map2Bereitstellen();
        z->liste = stufe2::laufDatei("aseb_liste.txt");
        z->aus = stufe2::laufDatei("aseb_modelle.bhmd");
        stufe2::testDateiSchreiben(z->liste, "models/aseb/truhe.ase\t0\nmodels/aseb/stein.md3\t0\n");
        const std::string befehl = karte::bhedModelleBefehl(ke::q3map2Aufruf(), {karte::ordnerBereinigt(quelle())}, z->liste, z->aus);
        diag::detail("Knopftest: ASEB - " + befehl);
        z->q3 = std::make_shared<bauplattform::Prozess>();
        std::string grund;
        if (!z->q3->starten(befehl, std::string(), false, grund)) { meldeFehler("ASEB: bhed_q3map2 startet nicht: " + grund); }
    }));
    add(warteBis("ASEB: q3map2 fertig", [z] { return !z->q3 || !z->q3->gestartet() || z->q3->warten(0); }, 3000));
    add(pruefe("ASEB: q3map2 laedt beide Modelle (Shader je Flaeche, Dreiecke, Punkte)", [z] {
        std::vector<karte::ModellDatei> m;
        const bool ok = karte::modellDumpLesen(dateiInhalt(z->aus), m);
        std::string t;
        for (const karte::ModellDatei& d : m) {
            Q3Modell& q = z->modelle[d.pfad];
            q.geladen = d.geladen;
            for (const karte::ModellNetz& n : d.netze) {
                q.dreiecke[n.shader] += n.dreiecke.size();
                for (const auto& p : n.punkte) {
                    for (std::size_t a = 0; a < 3; ++a) {
                        q.mn[a] = std::min(q.mn[a], p[a]);
                        q.mx[a] = std::max(q.mx[a], p[a]);
                    }
                }
            }
            t += d.pfad + ": " + shaderListe(q.dreiecke) + kastenText(q.mn, q.mx) + "; ";
        }
        diag::detail("Knopftest: ASEB - q3map2 " + t);
        return ok && z->modelle["models/aseb/truhe.ase"].geladen && z->modelle["models/aseb/stein.md3"].geladen &&
               !z->modelle["models/aseb/truhe.ase"].dreiecke.empty() && !z->modelle["models/aseb/stein.md3"].dreiecke.empty();
    }));
    // 1. Datei > Geometrie als Brushes importieren... mit der .ase (statt des Dateidialogs die Funktion dahinter, wie TUT-GEO)
    add(tu("ASEB: neue Karte, Import wie vorgegeben; truhe.ase laden (geoImportLaden)", [] {
        neueKarte();
        ke::editor().geoImport = ke::GeoImportZustand{};
        ke::konsoleLeeren();
        (void)ke::geoImportLaden(datei("truhe.ase"));
    }));
    add(pause(0.3));
    add(pruefe("ASEB .ase: Fenster offen, je Koerper ein Teil (Truhe, Griff), beide geschlossen, Brushes", [] {
        const ke::GeoImportZustand& g = ke::editor().geoImport;
        std::string t;
        for (std::size_t i = 0; i < g.netz.teile.size() && i < g.vorschau.size(); ++i) {
            t += g.netz.teile[i].name + (g.vorschau[i].geschlossen ? " geschlossen " : " offen ") + std::to_string(g.vorschau[i].pinsel.size()) + " Brushes; ";
        }
        diag::detail("Knopftest: ASEB - .ase offen " + std::to_string(g.offen ? 1 : 0) + ", Teile " + t + "Konsole: " + konsoleText().substr(0, 300));
        bool ok = g.offen && g.netz.teile.size() == 2 && g.vorschau.size() == 2;
        for (std::size_t i = 0; ok && i < 2; ++i) { ok = g.vorschau[i].geschlossen && g.vorschau[i].art == karte::TeilArt::Pinsel && !g.vorschau[i].pinsel.empty(); }
        return ok;
    }));
    add(pruefe("ASEB .ase: Dreiecke je Shader und die Shader der Brush-Seiten (ohne caulk) wie q3map2 fuer dieselbe Datei als misc_model", [z] {
        const ke::GeoImportZustand& g = ke::editor().geoImport;
        const Q3Modell& q = z->modelle["models/aseb/truhe.ase"];
        const auto ist = teilDreiecke(g);
        const auto seiten = seitenShader(g.vorschau);
        std::string st;
        for (const auto& x : seiten) { st += x + " "; }
        diag::detail("Knopftest: ASEB - .ase Teile " + shaderListe(ist) + "| q3map2 " + shaderListe(q.dreiecke) + "| Seiten " + st);
        return !q.dreiecke.empty() && ist == q.dreiecke && seiten == schluessel(q.dreiecke);
    }));
    add(klick(std::string(ke::trK(Str::KeGiEinfuegen)) + "##gieinfuegen", fenster));
    add(pause(0.4));
    add(pruefe("ASEB .ase: eingefuegt - so viele Brushes wie die Vorschau, Huellkasten gleich dem des Modells (q3map2), ein Undo-Schritt", [z] {
        const ke::Editor& ed = ke::editor();
        std::size_t soll = 0;
        for (const auto& t : ed.geoImport.vorschau) { soll += t.pinsel.size(); }
        std::array<float, 3> mn{}, mx{};
        const bool da = weltKasten(mn, mx);
        const Q3Modell& q = z->modelle["models/aseb/truhe.ase"];
        const float w = abweichung(mn, mx, q.mn, q.mx);
        diag::detail("Knopftest: ASEB - .ase " + std::to_string(tutgeo::weltPinsel().size()) + " Brushes (Vorschau " + std::to_string(soll) + "), Kasten " +
                     kastenText(mn, mx) + " gegen " + kastenText(q.mn, q.mx) + ", Abweichung " + std::to_string(w) + ", Undo " + ed.karte.undo.undoName());
        return da && soll > 0 && tutgeo::weltPinsel().size() == soll && w < 0.01F && ed.karte.undo.undoName() == "importGeometry" && !ed.geoImport.offen;
    }));
    // 2. die .md3
    add(tu("ASEB: Optimierer zu; neue Karte; stein.md3 laden", [] {
        ke::optimiererSchliessen();
        neueKarte();
        ke::editor().geoImport = ke::GeoImportZustand{};
        (void)ke::geoImportLaden(datei("stein.md3"));
    }));
    add(pause(0.3));
    add(pruefe("ASEB .md3: ein geschlossener Koerper aus zwei Flaechen; Dreiecke je Shader und Seiten-Shader wie q3map2 (auch der Shader ohne Ordner)", [z] {
        const ke::GeoImportZustand& g = ke::editor().geoImport;
        const Q3Modell& q = z->modelle["models/aseb/stein.md3"];
        const auto ist = teilDreiecke(g);
        const auto seiten = seitenShader(g.vorschau);
        std::string st;
        for (const auto& x : seiten) { st += x + " "; }
        diag::detail("Knopftest: ASEB - .md3 Teile " + std::to_string(g.netz.teile.size()) + ": " + shaderListe(ist) + "| q3map2 " + shaderListe(q.dreiecke) +
                     "| Seiten " + st + "| Konsole: " + konsoleText().substr(0, 300));
        return g.offen && g.netz.teile.size() == 1 && g.vorschau.size() == 1 && g.vorschau[0].geschlossen && g.vorschau[0].art == karte::TeilArt::Pinsel &&
               !q.dreiecke.empty() && ist == q.dreiecke && seiten == schluessel(q.dreiecke);
    }));
    add(klick(std::string(ke::trK(Str::KeGiAbbrechen)) + "##giabbrechen", fenster));
    // 3. .glm: klare Meldung
    add(tu("ASEB: figur.glm laden", [] {
        ke::konsoleLeeren();
        (void)ke::geoImportLaden(datei("figur.glm"));
    }));
    add(pruefe("ASEB .glm: kein Fenster, die Meldung sagt, warum (Ghoul2 ist keine Kartengeometrie)", [] {
        char t[1400];
        std::snprintf(t, sizeof(t), ke::trK(Str::KeGiGlm), datei("figur.glm").c_str());
        diag::detail("Knopftest: ASEB - .glm Konsole: " + konsoleText().substr(0, 300));
        return !ke::editor().geoImport.offen && konsoleText().find(t) != std::string::npos;
    }));
    // 4. Rechtsklick auf ein misc_model: gespiegelt (modelscale_vec -1 1 1) und um 90 Grad gedreht
    add(tu("ASEB: neue Karte, misc_model truhe.ase bei 256 128 0, angles 0 90 0, modelscale_vec -1 1 1, gewaehlt; XY-Ansicht dorthin", [z] {
        neueKarte();
        ke::Editor& e = ke::editor();
        e.geoImport = ke::GeoImportZustand{};
        e.gruppenDialogOffen = false;
        {
            karte::Befehl undo(e.karte.undo, "entityCreate -class misc_model");
            auto ent = e.karte.neueEntitaet("misc_model", false);
            ent->setzen("origin", "256 128 0");
            ent->setzen("model", "models/aseb/truhe.ase");
            ent->setzen("angles", "0 90 0");
            ent->setzen("modelscale_vec", "-1 1 1");
            e.karte.entitaetEinfuegen(ent);
            e.karte.auswahl.leeren();
            e.karte.auswahl.waehlen(ent);
        }
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(256, 128, 0));
        e.xy[0].ansicht.setzeMassstab(1.0F);
        ++e.sichtStand;
        // Soll: der Kasten von q3map2, mit der Lage des misc_model (90 Grad und Spiegelung: die Ecken bleiben Ecken)
        const auto en = miscModel();
        const karte::Matrix4 m = karte::miscModellMatrix(*en);
        const Q3Modell& q = z->modelle["models/aseb/truhe.ase"];
        z->mn = {1e30F, 1e30F, 1e30F};
        z->mx = {-1e30F, -1e30F, -1e30F};
        for (int i = 0; i < 8; ++i) {
            const karte::Vektor3f p = karte::transformierterPunkt(m, karte::Vektor3f((i & 1) != 0 ? q.mx[0] : q.mn[0], (i & 2) != 0 ? q.mx[1] : q.mn[1],
                                                                                     (i & 4) != 0 ? q.mx[2] : q.mn[2]));
            for (std::size_t a = 0; a < 3; ++a) {
                z->mn[a] = std::min(z->mn[a], p[static_cast<int>(a)]);
                z->mx[a] = std::max(z->mx[a], p[static_cast<int>(a)]);
            }
        }
        z->vorher = en->wert("origin") + "|" + en->wert("model") + "|" + en->wert("angles") + "|" + en->wert("modelscale_vec");
    }));
    add(pause(0.3));
    auto xyOrt = [](float x, float y) {
        return [x, y]() {
            const ke::XyFenster& f = ke::editor().xy[0];
            const auto p = f.ansicht.zuBild(karte::Vektor3f(x, y, 0));
            return ImVec2{f.bildX + p[0], f.bildY + p[1]};
        };
    };
    const std::string eintrag = ke::trK(Str::KeMbMenue);
    add(klickAn("ASEB: M2 in die XY-Ansicht auf das misc_model (ohne Bewegung)", xyOrt(256, 128), 1));
    add(pause(0.3));
    add(pruefe("ASEB Rechtsklick: das Entitaetenmenue zeigt oben \"" + eintrag + "\" (gewaehltes misc_model)", [eintrag] {
        const Element* el = finde(eintrag, "", 0, false);
        diag::detail("Knopftest: ASEB - Rechtsklick-Menue: Eintrag " + std::string(el != nullptr ? "da" : "fehlt") + ", info " +
                     (finde("info", "", 0, false) != nullptr ? "da" : "fehlt"));
        return el != nullptr && finde("info", "", 0, false) != nullptr;
    }));
    add(tu("ASEB: die Maus auf den Eintrag (Tooltip)", [eintrag] {
        if (const Element* el = finde(eintrag, "", 0, false)) { setzeMaus(el->rect.GetCenter()); }
    }));
    add(pause(0.8));
    add(pruefe("ASEB Rechtsklick: der Eintrag hat einen Tooltip (was er tut)", [] {
        const ImGuiWindow* t = ImGui::FindWindowByName("##Tooltip_00");
        return t != nullptr && t->Active && std::string(ke::trK(Str::KeMbTip)).size() > 40;
    }));
    if (handbuch) {
        // Das Rechtsklick-Menue ist so lang wie die Liste der Klassen und steht darum in einem eigenen Fenster (ImGui-Viewport),
        // das kein Foto des Programmfensters zeigt - das Bild zeigt denselben Eintrag unter File
        add(taste(ImGuiKey_Escape, "Esc (Rechtsklick-Menue zu)"));
        add(pause(0.3));
        add(klick(ke::trK(Str::KeLFile), "###main"));
        add(pause(0.4));
        add(tu("ASEB: die Maus auf File > " + eintrag + " (Tooltip)", [eintrag] {
            if (const Element* el = finde(eintrag, "", 0, false)) { setzeMaus(el->rect.GetCenter()); }
        }));
        add(pause(0.8));
        add(hilfe2::schrittFoto("h_aseb_menue"));
    } else {
        add(foto("aseb_rechtsklick"));
    }
    add(klick(eintrag));
    add(pause(0.4));
    add(pruefe("ASEB Rechtsklick: Fenster Geometrie zu Brushes mit den Koerpern des Modells (Truhe, Griff), Shader wie q3map2", [z] {
        const ke::GeoImportZustand& g = ke::editor().geoImport;
        const Q3Modell& q = z->modelle["models/aseb/truhe.ase"];
        diag::detail("Knopftest: ASEB - Umwandeln: offen " + std::to_string(g.offen ? 1 : 0) + ", " + std::to_string(g.netz.teile.size()) + " Teile, " +
                     shaderListe(teilDreiecke(g)) + "Datei " + g.datei);
        bool ok = g.offen && g.netz.teile.size() == 2 && g.vorschau.size() == 2 && teilDreiecke(g) == q.dreiecke;
        for (std::size_t i = 0; ok && i < 2; ++i) { ok = g.vorschau[i].geschlossen && g.vorschau[i].art == karte::TeilArt::Pinsel; }
        return ok;
    }));
    if (handbuch) {
        add(pause(0.5));
        add(hilfe2::schrittFoto("h_aseb_fenster"));
        add(klick(std::string(ke::trK(Str::KeGiAbbrechen)) + "##giabbrechen", fenster));
        add(tu("ASEB: Ende - neue Karte", [] { neueKarte(); }));
        return s;
    }
    add(foto("aseb_umwandeln"));
    add(klick(std::string(ke::trK(Str::KeGiEinfuegen)) + "##gieinfuegen", fenster));
    add(pause(0.4));
    add(pruefe("ASEB Rechtsklick: Brushes an derselben Stelle (Huellkasten gleich, gespiegelt und gedreht), das misc_model weg, Seiten-Shader wie q3map2", [z] {
        std::array<float, 3> mn{}, mx{};
        const bool da = weltKasten(mn, mx);
        const float w = abweichung(mn, mx, z->mn, z->mx);
        const Q3Modell& q = z->modelle["models/aseb/truhe.ase"];
        std::string st;
        for (const auto& x : weltShader()) { st += x + " "; }
        diag::detail("Knopftest: ASEB - umgewandelt: " + std::to_string(tutgeo::weltPinsel().size()) + " Brushes, Kasten " + kastenText(mn, mx) + " gegen " +
                     kastenText(z->mn, z->mx) + ", Abweichung " + std::to_string(w) + ", misc_model " + (miscModel() ? "noch da" : "weg") + ", Seiten " + st +
                     ", Undo " + ke::editor().karte.undo.undoName());
        return da && w < 0.01F && !miscModel() && weltShader() == schluessel(q.dreiecke) && ke::editor().karte.undo.undoName() == "importGeometry";
    }));
    add(tu("ASEB: Optimierer zu (er hat die Karte geprueft); in den Karten-Editor (Tastatur)", [] {
        ke::optimiererSchliessen();
        g_app->leftMode = 3;
    }));
    add(pause(0.2));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("ASEB Rechtsklick: ein Undo - keine Brushes, das misc_model wieder da (gleiche Schluessel)", [z] {
        const auto en = miscModel();
        const std::string jetzt = en ? en->wert("origin") + "|" + en->wert("model") + "|" + en->wert("angles") + "|" + en->wert("modelscale_vec") : std::string();
        diag::detail("Knopftest: ASEB - nach Undo " + std::to_string(tutgeo::weltPinsel().size()) + " Brushes, misc_model " + jetzt);
        return tutgeo::weltPinsel().empty() && jetzt == z->vorher;
    }));
    // 5. derselbe Weg ueber das Menue File (Auswahl wie vorher)
    add(tu("ASEB: das misc_model waehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        if (const auto en = miscModel()) { e.karte.auswahl.waehlen(en); }
    }));
    add(klick(ke::trK(Str::KeLFile), "###main"));
    add(pause(0.4));
    add(pruefe("ASEB Menue: File zeigt \"" + eintrag + "\" (aktiv)", [eintrag] { return finde(eintrag, "", 0, false) != nullptr; }));
    add(klick(eintrag));
    add(pause(0.4));
    add(pruefe("ASEB Menue: das Fenster mit beiden Koerpern", [] {
        const ke::GeoImportZustand& g = ke::editor().geoImport;
        return g.offen && g.netz.teile.size() == 2;
    }));
    add(klick(std::string(ke::trK(Str::KeGiAbbrechen)) + "##giabbrechen", fenster));
    add(pause(0.2));
    add(pruefe("ASEB Menue: Abbrechen aendert nichts (misc_model da, keine Brushes)", [] { return miscModel() && tutgeo::weltPinsel().empty() && !ke::editor().geoImport.offen; }));
    // 6. Helfer
    add(pruefe("ASEB Helfer: \"ase to brush\" findet das Tutorial k-geometrie (unter den ersten drei)", [] {
        const auto t = hilfeFrage("ase to brush", 3);
        std::string ids;
        bool da = false;
        for (const auto& x : t) {
            ids += x.id + " ";
            da = da || x.id == "tour:k-geometrie";
        }
        diag::detail("Knopftest: ASEB - Helfer: " + ids);
        return da;
    }));
    add(tu("ASEB: Ende - Import wie vorgegeben, neue Karte, Events", [] {
        ke::editor().geoImport = ke::GeoImportZustand{};
        neueKarte();
        g_app->leftMode = 0;
    }));
    return s;
}

std::vector<Schritt> schritte() { return schritteMit(false); }

std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s = stufe9c::einrichten();
    for (Schritt& x : schritteMit(true)) { s.push_back(std::move(x)); }
    return s;
}

}  // namespace aseb
