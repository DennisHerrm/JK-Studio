// selbsttest_modobj.inc - rc610 MODOBJ: der Modell-Browser zeigt .obj, .fbx usw. wie NetRadiant-Custom, Setzen wandelt
// in .md3 um, die Karte nimmt die Dateien mit (BHED_EDITORTEST=karteneditor, BHED_KARTE_NUR=modobj).
//
// Meldung (Nutzer, 05.11. abends): "NetRadiant can show the obj models here but JKS cant." - shank: "kannst du das so
// adden fuer obj fbx usw." und "convertiert er die obj und fbx nicht direkt in JKA useable format?".
//
// Der Test schreibt seine Modelle selbst in einen Ordner des Laufs (Extra Resource Path, keine fremden Dateien):
//   models/modobj/kiste.obj (+ kiste.mtl, Textur kiste.tga), SAEULE.OBJ (+ SAEULE.MTL: das Material ist ein Shader-Skript
//   des Spiels), platte.fbx (ASCII-FBX), ring.glb, marke.ase, wuerfel.md3; in modobj.pk3 models/modobj_pk3/KEGEL.OBJ.
// Geprueft wird mit echten Klicks im Browser: der Baum und die Dateien wie NRC (alle Modellendungen, gross geschrieben, aus
// der .pk3; nicht .mtl und .tga), das Vorschaubild jedes Modells, Doppelklick = misc_model mit der umgewandelten .md3 im
// Import-Ordner (Dreiecke und Kasten wie das Original, Textur und Shader-Skript dabei), Klick auf ein gewaehltes
// misc_model_static = .md3 mit dem Shader des Spiels, Schalter MD3 aus = der Original-Pfad wie NRC; die Kartenpruefung
// warnt bei Laufzeit-Entities mit einem Format, das das Spiel nicht laedt; q3map2 backt die .md3 mit denselben Dreiecken
// wie die .obj; die .pk3 der Karte hat .md3, Textur und Shader; der Licht-Teil von "In fertige Karte einfuegen" sucht im
// Import-Ordner und im Extra Resource Path.
// Eingebunden in gui/selbsttest_handbuch.inc (nach LICHT1).
namespace modobj {

namespace ke = karteneditor;
using stufe3::konsoleText;

std::string quelle() { return stufe2::laufDatei("modobj_quelle"); }
std::string raumKarte() { return stufe2::laufDatei("modobj_karte\\modobj_raum.map"); }
std::string importDatei(const std::string& rel) { return platform::settingsDirectory() + "\\import\\" + rel; }

struct Zustand {
    std::string spielShader;          // ein Shader mit Skript aus den Spieldaten (Material von SAEULE.OBJ)
    std::string kisteMd3;             // models/import/modobj_kiste_obj/modobj_kiste_obj.md3
    int kisteDreiecke = 0;            // die .obj im Editor
    karte::Kasten kisteKasten;
};

// Ein Quader in Q3-Koordinaten (Z oben) als .obj wie aus Blender (Y oben: x, z, -y), je Seite die ganze Textur
std::string quaderObj(const std::string& mtllib, const std::string& material, float x0, float y0, float z0, float x1, float y1, float z1) {
    const float p[8][3] = {{x0, y0, z0}, {x1, y0, z0}, {x1, y1, z0}, {x0, y1, z0}, {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}};
    std::string t = "# JK Studio self-test MODOBJ\nmtllib " + mtllib + "\no quader\n";
    char z[160];
    for (const auto& q : p) {
        std::snprintf(z, sizeof(z), "v %g %g %g\n", static_cast<double>(q[0]), static_cast<double>(q[2]), static_cast<double>(-q[1]));
        t += z;
    }
    t += "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nusemtl " + material + "\n";
    // gegen den Uhrzeigersinn von aussen (Z oben): unten, oben, vorn, rechts, hinten, links
    const int f[6][4] = {{1, 4, 3, 2}, {5, 6, 7, 8}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 4, 8, 7}, {4, 1, 5, 8}};
    for (const auto& q : f) {
        std::snprintf(z, sizeof(z), "f %d/1 %d/2 %d/3 %d/4\n", q[0], q[1], q[2], q[3]);
        t += z;
    }
    return t;
}

// Eine Platte 64 x 64 als ASCII-FBX 7.4 (Y oben, eine Flaeche aus zwei Dreiecken, ein Material)
std::string platteFbx(const std::string& material) {
    return "; FBX 7.4.0 project file\n"
           "FBXHeaderExtension:  {\n\tFBXHeaderVersion: 1003\n\tFBXVersion: 7400\n}\n"
           "GlobalSettings:  {\n\tVersion: 1000\n\tProperties70:  {\n"
           "\t\tP: \"UpAxis\", \"int\", \"Integer\", \"\",1\n\t\tP: \"UpAxisSign\", \"int\", \"Integer\", \"\",1\n"
           "\t\tP: \"FrontAxis\", \"int\", \"Integer\", \"\",2\n\t\tP: \"FrontAxisSign\", \"int\", \"Integer\", \"\",1\n"
           "\t\tP: \"CoordAxis\", \"int\", \"Integer\", \"\",0\n\t\tP: \"CoordAxisSign\", \"int\", \"Integer\", \"\",1\n"
           "\t\tP: \"UnitScaleFactor\", \"double\", \"Number\", \"\",1\n\t}\n}\n"
           "Objects:  {\n"
           "\tGeometry: 1000, \"Geometry::platte\", \"Mesh\" {\n"
           "\t\tVertices: *12 {\n\t\t\ta: -32,0,32,32,0,32,32,0,-32,-32,0,-32\n\t\t}\n"
           "\t\tPolygonVertexIndex: *4 {\n\t\t\ta: 0,1,2,-4\n\t\t}\n"
           "\t\tGeometryVersion: 124\n"
           "\t\tLayerElementMaterial: 0 {\n\t\t\tVersion: 101\n\t\t\tName: \"\"\n\t\t\tMappingInformationType: \"AllSame\"\n"
           "\t\t\tReferenceInformationType: \"IndexToDirect\"\n\t\t\tMaterials: *1 {\n\t\t\t\ta: 0\n\t\t\t}\n\t\t}\n"
           "\t\tLayer: 0 {\n\t\t\tVersion: 100\n\t\t\tLayerElement:  {\n\t\t\t\tType: \"LayerElementMaterial\"\n\t\t\t\tTypedIndex: 0\n\t\t\t}\n\t\t}\n"
           "\t}\n"
           "\tModel: 2000, \"Model::platte\", \"Mesh\" {\n\t\tVersion: 232\n\t\tProperties70:  {\n\t\t}\n\t\tShading: T\n\t\tCulling: \"CullingOff\"\n\t}\n"
           "\tMaterial: 3000, \"Material::" + material + "\", \"\" {\n\t\tVersion: 102\n\t\tShadingModel: \"phong\"\n\t\tMultiLayer: 0\n"
           "\t\tProperties70:  {\n\t\t}\n\t}\n"
           "}\n"
           "Connections:  {\n\tC: \"OO\",2000,0\n\tC: \"OO\",1000,2000\n\tC: \"OO\",3000,2000\n}\n";
}

// Ein Dreieck als ASE (Z oben, wie 3ds max), Material mit Bild
std::string dreieckAse(const std::string& material) {
    return "*3DSMAX_ASCIIEXPORT\t200\n*MATERIAL_LIST {\n\t*MATERIAL_COUNT 1\n\t*MATERIAL 0 {\n\t\t*MATERIAL_NAME \"" + material +
           "\"\n\t\t*MAP_DIFFUSE {\n\t\t\t*BITMAP \"" + material + ".tga\"\n\t\t}\n\t}\n}\n"
           "*GEOMOBJECT {\n\t*NODE_NAME \"marke\"\n\t*MESH {\n\t\t*TIMEVALUE 0\n\t\t*MESH_NUMVERTEX 3\n\t\t*MESH_NUMFACES 1\n"
           "\t\t*MESH_VERTEX_LIST {\n\t\t\t*MESH_VERTEX 0\t0.0000\t0.0000\t0.0000\n\t\t\t*MESH_VERTEX 1\t32.0000\t0.0000\t0.0000\n"
           "\t\t\t*MESH_VERTEX 2\t0.0000\t0.0000\t32.0000\n\t\t}\n"
           "\t\t*MESH_FACE_LIST {\n\t\t\t*MESH_FACE 0: A: 0 B: 2 C: 1 AB: 1 BC: 1 CA: 1 *MESH_SMOOTHING 1 *MESH_MTLID 0\n\t\t}\n"
           "\t\t*MESH_NUMTVERTEX 3\n\t\t*MESH_TVERTLIST {\n\t\t\t*MESH_TVERT 0\t0.0000\t0.0000\t0.0000\n\t\t\t*MESH_TVERT 1\t1.0000\t0.0000\t0.0000\n"
           "\t\t\t*MESH_TVERT 2\t0.0000\t1.0000\t0.0000\n\t\t}\n\t\t*MESH_NUMTVFACES 1\n\t\t*MESH_TFACELIST {\n\t\t\t*MESH_TFACE 0\t0\t2\t1\n\t\t}\n"
           "\t}\n\t*MATERIAL_REF 0\n}\n";
}

// Ein Teil fuer glbSchreiben / md3Paket: ein Keil (6 Punkte, 8 Dreiecke) in Q3-Koordinaten
karte::ImportNetz keil(const std::string& material) {
    karte::ImportNetz n;
    karte::ImportTeil t;
    t.name = "keil";
    t.material = material;
    t.hatUv = true;
    t.punkte = {{-24, -24, 0}, {24, -24, 0}, {0, 24, 0}, {-24, -24, 40}, {24, -24, 40}, {0, 24, 40}};
    t.uv = {{0, 1}, {1, 1}, {0.5, 0}, {0, 0}, {1, 0}, {0.5, 1}};
    t.dreiecke = {{0, 2, 1}, {3, 4, 5}, {0, 1, 4}, {0, 4, 3}, {1, 2, 5}, {1, 5, 4}, {2, 0, 3}, {2, 3, 5}};
    n.teile.push_back(t);
    return n;
}

// Ein geschlossener Raum 1024 x 1024 x 384 mit Spielerstart
std::string raumText() {
    std::string t = "// JK Studio self-test MODOBJ\n{\n\"classname\" \"worldspawn\"\n";
    const auto quader = [&t](int x0, int y0, int z0, int x1, int y1, int z1) {
        char z[900];
        std::snprintf(z, sizeof(z),
                      "{\n( %d 0 0 ) ( %d 1 0 ) ( %d 0 1 ) textures/system/caulk 0 0 0 0.5 0.5 0 0 0\n"
                      "( %d 0 0 ) ( %d 0 1 ) ( %d 1 0 ) textures/system/caulk 0 0 0 0.5 0.5 0 0 0\n"
                      "( 0 %d 0 ) ( 0 %d 1 ) ( 1 %d 0 ) textures/system/caulk 0 0 0 0.5 0.5 0 0 0\n"
                      "( 0 %d 0 ) ( 1 %d 0 ) ( 0 %d 1 ) textures/system/caulk 0 0 0 0.5 0.5 0 0 0\n"
                      "( 0 0 %d ) ( 1 0 %d ) ( 0 1 %d ) textures/system/caulk 0 0 0 0.5 0.5 0 0 0\n"
                      "( 0 0 %d ) ( 0 1 %d ) ( 1 0 %d ) textures/system/caulk 0 0 0 0.5 0.5 0 0 0\n}\n",
                      x0, x0, x0, x1, x1, x1, y0, y0, y0, y1, y1, y1, z0, z0, z0, z1, z1, z1);
        t += z;
    };
    quader(-528, -528, -16, 528, 528, 0);     // Boden
    quader(-528, -528, 384, 528, 528, 400);   // Decke
    quader(-528, -528, 0, -512, 528, 384);
    quader(512, -528, 0, 528, 528, 384);
    quader(-512, -528, 0, 512, -512, 384);
    quader(-512, 512, 0, 512, 528, 384);
    t += "}\n{\n\"classname\" \"info_player_start\"\n\"origin\" \"0 -300 24\"\n}\n";
    return t;
}

void quelleSchreiben(const Zustand& z) {
    using stufe2::testDateiSchreiben;
    const std::string q = quelle();
    testDateiSchreiben(q + "\\models\\modobj\\kiste.obj", quaderObj("kiste.mtl", "kiste", -16, -16, 0, 16, 16, 32));
    testDateiSchreiben(q + "\\models\\modobj\\kiste.mtl", "newmtl kiste\nKd 1 1 1\nmap_Kd kiste.tga\n");
    testDateiSchreiben(q + "\\models\\modobj\\kiste.tga", stufe2::tgaBild(40, 200, 60));
    // SAEULE.MTL nennt den Shader des Spiels als Material (Name mit '/': NRCs Regel nimmt ihn, wie er ist)
    testDateiSchreiben(q + "\\models\\modobj\\SAEULE.OBJ", quaderObj("SAEULE.MTL", z.spielShader, -8, -8, 0, 8, 8, 96));
    testDateiSchreiben(q + "\\models\\modobj\\SAEULE.MTL", "newmtl " + z.spielShader + "\nKd 1 1 1\n");
    testDateiSchreiben(q + "\\models\\modobj\\platte.fbx", platteFbx("models/modobj/platte"));
    testDateiSchreiben(q + "\\models\\modobj\\ring.glb", karte::glbSchreiben(keil("models/modobj/kiste"), true));
    testDateiSchreiben(q + "\\models\\modobj\\marke.ase", dreieckAse("models/modobj/kiste"));
    const karte::Md3Paket w = karte::md3Paket(keil("wuerfel"), "modobj_wuerfel");
    testDateiSchreiben(q + "\\models\\modobj\\wuerfel.md3", w.dateien.empty() ? std::string() : w.dateien[0].inhalt);
    std::vector<Pk3Datei> pk3(1);
    pk3[0].name = "models/modobj_pk3/KEGEL.OBJ";
    pk3[0].inhalt = quaderObj("kegel.mtl", "models/modobj/kiste", -4, -4, 0, 4, 4, 48);
    std::string err;
    (void)writePk3(q + "\\modobj.pk3", pk3, &err);
    testDateiSchreiben(raumKarte(), raumText());
}

// Die Dreiecke eines Modells im Editor (wie der Browser und die Ansichten es zeichnen)
int dreiecke(const karte::Modell& m) {
    std::size_t n = 0;
    for (const auto& f : m.flaechen) { n += f.indizes.size() / 3; }
    return static_cast<int>(n);
}

int dateiNr(const std::string& name) {
    const auto d = ke::modellBrowserDateien();
    for (std::size_t i = 0; i < d.size(); ++i) {
        if (d[i] == name) { return static_cast<int>(i); }
    }
    return -1;
}

std::function<ImVec2()> zelleVon(const std::string& name) {
    return [name]() {
        const int i = dateiNr(name);
        return i < 0 ? ImVec2{0, 0} : stufe2::modellZelle(i)();
    };
}

std::shared_ptr<karte::Entitaet> letzteMit(const std::string& klasse) {
    std::shared_ptr<karte::Entitaet> r;
    for (const auto& en : ke::editor().karte.entitaeten()) {
        if (en->klassenname() == klasse) { r = en; }
    }
    return r;
}

bool istDatei(const std::string& p) {
    std::error_code ec;
    return std::filesystem::is_regular_file(::bhed::utf8Pfad(p), ec);
}

// Die Dreiecke der .bsp je Shader (Lump 1 shaders: 72 Bytes, Lump 13 surfaces: RBSP 148 Bytes, numIndexes bei +24)
std::map<std::string, int> bspDreiecke(const std::string& bsp) {
    std::map<std::string, int> m;
    karte::BspDatei d;
    if (!karte::bspLesen(dateiInhalt(bsp), d) || d.lumps.size() < 14) { return m; }
    std::vector<std::string> namen;
    for (std::size_t o = 0; o + 72 <= d.lumps[1].size(); o += 72) { namen.emplace_back(d.lumps[1].c_str() + o); }
    const std::size_t g = d.ident == "RBSP" ? 148 : 104;
    for (std::size_t o = 0; o + g <= d.lumps[13].size(); o += g) {
        std::int32_t sh = 0;
        std::int32_t idx = 0;
        std::memcpy(&sh, d.lumps[13].data() + o, 4);
        std::memcpy(&idx, d.lumps[13].data() + o + 24, 4);
        if (sh >= 0 && static_cast<std::size_t>(sh) < namen.size()) { m[namen[static_cast<std::size_t>(sh)]] += idx / 3; }
    }
    return m;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<Zustand>();
    add(tu("MODOBJ: ein Shader-Skript des Spiels waehlen, Modelle in den Ordner des Laufs, als Extra Resource Path", [z] {
        karteDialogeZu();
        const auto& sp = ke::editor().spiel;
        for (const karte::ShaderEintrag* e : sp ? sp->inventar.shader() : std::vector<const karte::ShaderEintrag*>{}) {
            if (z->spielShader.empty() && !e->istStandard() && e->name.rfind("textures/", 0) == 0 && e->name.size() < 40) { z->spielShader = e->name; }
        }
        diag::detail("Knopftest: MODOBJ - Shader des Spiels " + z->spielShader);
        quelleSchreiben(*z);
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0] = karte::ordnerBereinigt(quelle()); });
    }));
    add(warteBis("MODOBJ: Spieldaten mit dem Extra Resource Path geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit() && ke::editor().karteSpiel && !ke::editor().karteSpiel->ordner.empty() &&
               ke::editor().karteSpiel->fs.finden("models/modobj/kiste.obj") != "";
    }, 20000));
    add(tu("MODOBJ: Raum oeffnen, View > Model Browser", [] {
        (void)ke::karteOeffnen(raumKarte());
        ke::editor().karte.auswahl.leeren();
        ke::editor().gruppenSeite = 0;
        (void)ke::ausfuehren("ToggleModelBrowser");
    }));
    add(pause(0.3));
    add(tu("Gruppendialog 444 x 600 rechts, Baum neu", [] {
        const float sc = ke::massstab();
        ImGui::SetWindowPos("###kartegruppe", ImVec2{ImGui::GetMainViewport()->Size.x - 520.0F * sc, 40.0F * sc});
        ImGui::SetWindowSize("###kartegruppe", ImVec2{444.0F * sc, 600.0F * sc});
        ke::modellBrowserBaumBauen();
    }));
    add(pause(0.5));
    add(klickAn("Pfeil vor models/", [] {
        const Element* el = finde("##mbmodels/", "", 0, true);
        return el == nullptr ? ImVec2{0, 0} : ImVec2{el->rect.Min.x + 8.0F * ke::massstab(), el->rect.GetCenter().y};
    }));
    add(pause(0.3));
    add(pruefe("Baum wie NRC: models/modobj/ und models/modobj_pk3/ (nur .obj, gross geschrieben, in der .pk3)", [] {
        const bool a = finde("##mbmodels/modobj/", "", 0, true) != nullptr;
        const bool b = finde("##mbmodels/modobj_pk3/", "", 0, true) != nullptr;
        diag::detail(std::string("Knopftest: MODOBJ - modobj/ ") + (a ? "da" : "fehlt") + ", modobj_pk3/ " + (b ? "da" : "fehlt"));
        return a && b;
    }));
    add(klickAn("Doppelklick auf modobj_pk3", stufe2::elementOrt("##mbmodels/modobj_pk3/"), 0, true));
    add(pause(0.6));
    add(pruefe("modobj_pk3: KEGEL.OBJ aus der .pk3, Vorschau mit Dreiecken", [] {
        const auto d = ke::modellBrowserDateien();
        const karte::ModellAnsicht a = ke::kartenModellPfad("models/modobj_pk3/KEGEL.OBJ");
        diag::detail("Knopftest: MODOBJ - modobj_pk3 " + std::to_string(d.size()) + " Dateien, KEGEL " + std::to_string(a.modell ? dreiecke(*a.modell) : -1) + " Dreiecke");
        return d.size() == 1 && d[0] == "KEGEL.OBJ" && a.modell && !a.modell->null && dreiecke(*a.modell) == 12;
    }));
    add(klickAn("Doppelklick auf modobj", stufe2::elementOrt("##mbmodels/modobj/"), 0, true));
    add(pause(0.6));
    add(pruefe("modobj: alle sechs Modelle (.obj, .OBJ, .fbx, .glb, .ase, .md3), nicht .mtl, .MTL und .tga", [] {
        const auto d = ke::modellBrowserDateien();
        std::string t;
        for (const std::string& x : d) { t += x + " "; }
        diag::detail("Knopftest: MODOBJ - modobj: " + t);
        const std::vector<std::string> soll{"SAEULE.OBJ", "kiste.obj", "marke.ase", "platte.fbx", "ring.glb", "wuerfel.md3"};
        return d == soll;
    }));
    add(pruefe("Vorschau: jedes Modell geladen (kein NullModel), mit seinen Dreiecken und einem Kasten", [] {
        const std::vector<std::pair<std::string, int>> soll{{"SAEULE.OBJ", 12}, {"kiste.obj", 12}, {"marke.ase", 1}, {"platte.fbx", 2}, {"ring.glb", 8}, {"wuerfel.md3", 8}};
        bool ok = true;
        std::string t;
        for (const auto& [name, n] : soll) {
            const karte::ModellAnsicht a = ke::kartenModellPfad("models/modobj/" + name);
            const int ist = a.modell && !a.modell->null ? dreiecke(*a.modell) : -1;
            t += name + " " + std::to_string(ist) + ", ";
            ok = ok && ist == n && karte::kastenGueltig(a.modell->grenzen);
        }
        diag::detail("Knopftest: MODOBJ - Vorschau " + t);
        return ok;
    }));
    add(pause(0.5));
    add(foto("modobj_browser"));
    add(tu("Kiste: Dreiecke und Kasten der .obj merken, Auswahl leer", [z] {
        const karte::ModellAnsicht a = ke::kartenModellPfad("models/modobj/kiste.obj");
        z->kisteDreiecke = a.modell ? dreiecke(*a.modell) : 0;
        if (a.modell) { z->kisteKasten = a.modell->grenzen; }
        ke::editor().karte.auswahl.leeren();
        ke::konsoleLeeren();
    }));
    add(klickAn("Doppelklick auf kiste.obj", zelleVon("kiste.obj"), 0, true));
    add(pause(0.5));
    add(pruefe("Doppelklick (MD3 an): misc_model mit der .md3 im Import-Ordner, Textur und Shader-Skript daneben, Meldung", [z] {
        const auto en = letzteMit("misc_model");
        const std::string m = en ? en->wert("model") : std::string();
        z->kisteMd3 = m;
        const bool md3 = m == "models/import/modobj_kiste_obj/modobj_kiste_obj.md3" && istDatei(importDatei("models\\import\\modobj_kiste_obj\\modobj_kiste_obj.md3"));
        const bool bild = istDatei(importDatei("models\\import\\modobj_kiste_obj\\kiste.tga"));
        const std::string sh = dateiInhalt(importDatei("shaders\\modobj_kiste_obj.shader"));
        const bool skript = sh.find("models/import/modobj_kiste_obj/kiste\n{") != std::string::npos &&
                            sh.find("map models/import/modobj_kiste_obj/kiste.tga") != std::string::npos;
        diag::detail("Knopftest: MODOBJ - misc_model \"" + m + "\", Bild " + (bild ? "ja" : "nein") + ", Skript " + (skript ? "ja" : "nein"));
        return md3 && bild && skript && konsoleText().find("models/import/modobj_kiste_obj/modobj_kiste_obj.md3") != std::string::npos;
    }));
    add(warteBis("MODOBJ: der Import-Ordner im Suchpfad, die .md3 geladen", [z] {
        ke::spielPruefen();
        if (!ke::spielBereit()) { return false; }
        const karte::ModellAnsicht a = ke::kartenModellPfad(z->kisteMd3);
        return a.modell && !a.modell->null;
    }, 3000));
    add(pruefe("die .md3 zeigt die Kiste wie die .obj: gleich viele Dreiecke, Kasten auf 1/64 genau, Shader mit dem Bild", [z] {
        const karte::ModellAnsicht a = ke::kartenModellPfad(z->kisteMd3);
        if (!a.modell || a.modell->null) { return false; }
        const karte::Kasten& k = a.modell->grenzen;
        float d = 0.0F;
        for (int i = 0; i < 3; ++i) {
            d = std::max({d, std::fabs(k.origin[i] - z->kisteKasten.origin[i]), std::fabs(k.extents[i] - z->kisteKasten.extents[i])});
        }
        const std::string sh = a.modell->flaechen.empty() ? std::string() : a.modell->flaechen[0].shader;
        diag::detail("Knopftest: MODOBJ - .md3 " + std::to_string(dreiecke(*a.modell)) + " Dreiecke (obj " + std::to_string(z->kisteDreiecke) +
                     "), Kasten-Abweichung " + std::to_string(d) + ", Shader " + sh);
        return dreiecke(*a.modell) == z->kisteDreiecke && d <= 1.0F / 64.0F + 1e-4F && sh == "models/import/modobj_kiste_obj/kiste";
    }));
    add(tu("misc_model_static setzen und waehlen", [] {
        ke::Editor& e = ke::editor();
        karte::Befehl undo(e.karte.undo, "entityCreate -class misc_model_static");
        auto ent = e.karte.neueEntitaet("misc_model_static", false);
        ent->setzen("origin", "128 0 0");
        e.karte.entitaetEinfuegen(ent);
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(ent);
    }));
    add(klickAn("Klick auf SAEULE.OBJ", zelleVon("SAEULE.OBJ")));
    add(pause(0.5));
    add(pruefe("Klick: misc_model_static bekommt die .md3 (das Spiel laedt nur .md3), Flaeche mit dem Shader des Spiels, kein Skript", [z] {
        const auto en = letzteMit("misc_model_static");
        const std::string m = en ? en->wert("model") : std::string();
        Md3Model md3;
        const bool gelesen = readMd3(dateiInhalt(importDatei("models\\import\\modobj_saeule_obj\\modobj_saeule_obj.md3")), md3);
        const std::string sh = gelesen && !md3.surfaces.empty() ? md3.surfaces[0].shader : std::string();
        diag::detail("Knopftest: MODOBJ - misc_model_static \"" + m + "\", Shader " + sh + " (soll " + z->spielShader + ")");
        return m == "models/import/modobj_saeule_obj/modobj_saeule_obj.md3" && sh == z->spielShader && !z->spielShader.empty() &&
               !istDatei(importDatei("shaders\\modobj_saeule_obj.shader"));
    }));
    // Schalter MD3 aus: der Original-Pfad wie NRC (nur misc_model, q3map2 backt es)
    add(tu("Auswahl leer", [] { ke::editor().karte.auswahl.leeren(); }));
    add(klick(std::string(ke::trK(Str::KeModBroMd3)) + "##modbroumwandeln"));
    add(pause(0.3));
    add(klickAn("Doppelklick auf kiste.obj (MD3 aus)", zelleVon("kiste.obj"), 0, true));
    add(pause(0.5));
    add(pruefe("MD3 aus: misc_model mit models/modobj/kiste.obj (wie NRC), der Schalter gespeichert", [] {
        const auto en = letzteMit("misc_model");
        const std::string m = en ? en->wert("model") : std::string();
        diag::detail("Knopftest: MODOBJ - MD3 aus: \"" + m + "\"");
        const auto it = g_app->settings.karte.find("ModelBrowserConvertMd3");
        return m == "models/modobj/kiste.obj" && it != g_app->settings.karte.end() && it->second == "0";
    }));
    add(klick(std::string(ke::trK(Str::KeModBroMd3)) + "##modbroumwandeln"));
    add(pause(0.3));
    add(tu("Auswahl leer", [] { ke::editor().karte.auswahl.leeren(); }));
    add(klickAn("Doppelklick auf platte.fbx (MD3 wieder an)", zelleVon("platte.fbx"), 0, true));
    add(pause(0.5));
    add(pruefe("platte.fbx: misc_model mit der .md3 (2 Dreiecke), Shader ohne Bild bleibt (wie beim misc_model)", [] {
        const auto en = letzteMit("misc_model");
        const std::string m = en ? en->wert("model") : std::string();
        Md3Model md3;
        const bool gelesen = readMd3(dateiInhalt(importDatei("models\\import\\modobj_platte_fbx\\modobj_platte_fbx.md3")), md3);
        const std::size_t n = gelesen && !md3.surfaces.empty() ? md3.surfaces[0].indexes.size() / 3 : 0;
        diag::detail("Knopftest: MODOBJ - platte \"" + m + "\", " + std::to_string(n) + " Dreiecke, Shader " +
                     (gelesen && !md3.surfaces.empty() ? md3.surfaces[0].shader : std::string()));
        return m == "models/import/modobj_platte_fbx/modobj_platte_fbx.md3" && n == 2 && md3.surfaces[0].shader == "models/modobj/platte";
    }));
    // Kartenpruefung: Laufzeit-Entities mit einem Format, das das Spiel nicht laedt
    auto laeufe = std::make_shared<int>(0);
    add(tu("misc_model_static mit ring.glb und misc_model_breakable mit platte.fbx, Optimierer", [laeufe] {
        ke::Editor& e = ke::editor();
        karte::Befehl undo(e.karte.undo, "entityCreate -class misc_model_static");
        auto a = e.karte.neueEntitaet("misc_model_static", false);
        a->setzen("origin", "-128 128 0");
        a->setzen("model", "models/modobj/ring.glb");
        e.karte.entitaetEinfuegen(a);
        auto b = e.karte.neueEntitaet("misc_model_breakable", false);
        b->setzen("origin", "-128 -128 0");
        b->setzen("model", "models/modobj/platte.fbx");
        e.karte.entitaetEinfuegen(b);
        e.karte.auswahl.leeren();
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererStarten();
    }));
    add(stufe5::warteOptimierer("Optimierer fertig", laeufe));
    add(pruefe("Kartenpruefung: \"modellformat\" fuer die zwei (nicht fuer das misc_model mit der .obj, nicht fuer die .md3)", [] {
        long long n = 0;
        const bool da = stufe5::hatBefund("modellformat", &n);
        diag::detail("Knopftest: MODOBJ - Befunde " + stufe5::befundListe());
        return da && n == 2;
    }));
    add(tu("die zwei wieder weg (Undo), Optimierer zu", [] {
        (void)ke::ausfuehren("Undo");
        ke::optimiererSchliessen();
    }));
    // Bau: q3map2 backt die .md3 mit denselben Dreiecken wie die .obj
    add(tu("Karte speichern, q3map2 -meta (Build-Eintrag des Tests)", [] {
        stufe3::q3map2Bereitstellen();
        ke::bauMenueSicherstellen();
        (void)ke::karteSpeichernUnter(raumKarte());
        ke::Editor& e = ke::editor();
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{"Test MODOBJ", {"[q3map2] -meta \"[MapFile]\""}});
        ke::konsoleLeeren();
    }));
    add(stufe3::bauMenuepunkt("Test MODOBJ"));
    add(stufe8::warteSek("Bau fertig", [] { return !ke::bauLaeuft(); }, 300));
    add(pause(0.5));
    add(pruefe("q3map2: die Kiste als .md3 und als .obj mit gleich vielen Dreiecken in der .bsp, die Platte (.md3) mit 2", [z] {
        const std::string bsp = raumKarte().substr(0, raumKarte().size() - 4) + ".bsp";
        const std::map<std::string, int> d = bspDreiecke(bsp);
        const auto n = [&d](const std::string& sh) {
            const auto it = d.find(sh);
            return it == d.end() ? 0 : it->second;
        };
        diag::detail("Knopftest: MODOBJ - .bsp: md3 " + std::to_string(n("models/import/modobj_kiste_obj/kiste")) + ", obj " + std::to_string(n("models/modobj/kiste")) +
                     ", platte " + std::to_string(n("models/modobj/platte")) + " Dreiecke (Editor " + std::to_string(z->kisteDreiecke) + ")");
        return n("models/import/modobj_kiste_obj/kiste") == z->kisteDreiecke && n("models/modobj/kiste") == z->kisteDreiecke &&
               n("models/modobj/platte") == 2 && konsoleText().find("-fs_pakpath") != std::string::npos;
    }));
    add(pruefe("Pack map as .pk3: .md3, Textur und Shader-Skript aus dem Import-Ordner dabei, die Quellen nicht", [] {
        const std::string p = ke::karteAlsPk3();
        Pk3 a;
        const bool ok = !p.empty() && readPk3Directory(p, a);
        const bool alle = ok && a.find("maps/modobj_raum.bsp") != nullptr && a.find("models/import/modobj_kiste_obj/modobj_kiste_obj.md3") != nullptr &&
                          a.find("models/import/modobj_kiste_obj/kiste.tga") != nullptr && a.find("shaders/modobj_kiste_obj.shader") != nullptr &&
                          a.find("models/import/modobj_saeule_obj/modobj_saeule_obj.md3") != nullptr &&
                          a.find("models/import/modobj_platte_fbx/modobj_platte_fbx.md3") != nullptr;
        const bool ohne = ok && a.find("models/modobj/kiste.obj") == nullptr;
        diag::detail("Knopftest: MODOBJ - Archiv " + p + ": alle " + (alle ? "ja" : "nein") + ", ohne Quellen " + (ohne ? "ja" : "nein"));
        return alle && ohne;
    }));
    add(pruefe("In fertige Karte einfuegen, Licht-Teil: q3map2 sucht im Extra Resource Path und im Import-Ordner (misc_model als Schattenwerfer)", [] {
        const std::vector<std::string> b = karte::teilLichtBefehle(ke::q3map2Aufruf(), "x.map", std::string());
        const std::string zusatz = "-fs_pakpath \"" + karte::ordnerBereinigt(quelle()) + "\"";
        const std::string imp = "-fs_pakpath \"" + ke::importOrdner() + "\"";
        bool ok = b.size() == 2 && !ke::importOrdner().empty();
        for (const std::string& x : b) { ok = ok && x.find(zusatz) != std::string::npos && x.find(imp) != std::string::npos; }
        diag::detail("Knopftest: MODOBJ - Licht-Teil: " + (b.empty() ? std::string() : b[0]));
        return ok;
    }));
    add(tu("Gruppendialog zu", [] {
        ke::editor().gruppenDialogOffen = false;
        ke::editor().gruppenSeite = 0;
    }));
    return s;
}

// Das Bild des Handbuchs (BHED_EDITORTEST=handbuch BHED_HB_NUR=modobj): der Modell-Browser mit dem Ordner models/modobj/ (alle
// sechs Formate), gross neben der Kamera, mit dem Schalter MD3 - das ganze Fenster.
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s = stufe9c::einrichten();
    std::vector<Schritt> t = schritte();
    std::size_t n = 0;
    while (n < t.size() && t[n].text.find("Vorschau: jedes Modell") == std::string::npos) { ++n; }
    t.resize(std::min(t.size(), n + 1));
    for (Schritt& x : t) { s.push_back(std::move(x)); }
    s.push_back(tu("Handbuch MODOBJ: Gruppendialog gross rechts", [] {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetWindowPos("###kartegruppe", ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.42F, vp->WorkPos.y + vp->WorkSize.y * 0.06F});
        ImGui::SetWindowSize("###kartegruppe", ImVec2{vp->WorkSize.x * 0.55F, vp->WorkSize.y * 0.86F});
    }));
    s.push_back(pause(0.8));
    s.push_back(warteBis("Modelle und Texturen geladen", [] { return ke::kameraBilderFertig(); }, 4000));
    s.push_back(pause(0.5));
    s.push_back(hilfe2::schrittFoto("h_modobj_browser"));
    return s;
}

}  // namespace modobj
