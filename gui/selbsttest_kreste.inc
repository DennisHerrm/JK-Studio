// selbsttest_kreste.inc - Karten-Editor-Reste (Auftrag p_karte, 07.11.): ASEB im echten Bau (BHED_EDITORTEST=karteneditor,
// BHED_KARTE_NUR=kr).
//
// Je Modell drei Karten im selben Raum (Waende textures/imperial/basic, ein Licht), gebaut mit bhed_q3map2 -meta wie der
// Build-Eintrag des Tests von MODOBJ:
//   _modell  das Modell als misc_model - so backt q3map2 es ein (die Vorgabe)
//   _brush   Rechtsklick "Modell in Brushes umwandeln", alle Teile wie vorgeschlagen (Auto), eingefuegt
//   _clip    dasselbe, alle Teile als Modell+Clip (die .obj ohne .mtl im Import-Ordner, usemtl = Shader)
// Geprueft in der .bsp (Lump shaders/surfaces): _brush zeichnet dieselben Shader wie _modell, _clip dieselben Dreiecke je
// Shader wie _modell. Modelle: truhe.ase aus ASEB (wie ein 3ds-Max-Export fuer JKA, ein Bild ohne Ordner: Shader
// models/aseb/griff) und eine echte .ase aus 3ds Max aus den Spieldaten von Movie Duels (MD_Maps_Ep4.pk3:
// models/map_objects/tatooine_outpost/to_chair.ase, *3DSMAX_ASCIIEXPORT 200). Danach der Ladetest der gebauten Karte mit
// dem Stuhl in "Gebaute Karte wie im Spiel" (Schnell-Backen, Spielbild, Foto kr_stuhl_spielbild).
// Eingebunden in gui/selbsttest_handbuch.inc (nach ASEB).
namespace kreste {

namespace ke = karteneditor;
using stufe3::konsoleText;

const char* const kWand = "textures/imperial/basic";
const char* const kStuhl = "models/map_objects/tatooine_outpost/to_chair.ase";

std::string kartePfad(const std::string& name) { return stufe2::laufDatei(("kr_karten\\kr_" + name + ".map").c_str()); }
std::string bspVon(const std::string& map) { return map.substr(0, map.size() - 4) + ".bsp"; }

// Der Raum (innen 1024 x 1024 x 384, Waende kWand), ein Licht, der Spieler vor dem Modell; das Modell bei 0 0 0
std::string raumText(const std::string& modell) {
    std::string t = "// JK Studio self-test KR\n{\n\"classname\" \"worldspawn\"\n";
    const std::string w = std::string(kWand).substr(9);   // im .map ohne "textures/" (q3map2 map.cpp ParseRawBrush :1009)
    const auto quader = [&t, &w](int x0, int y0, int z0, int x1, int y1, int z1) {
        char z[1200];
        std::snprintf(z, sizeof(z),
                      "{\n( %d 0 0 ) ( %d 1 0 ) ( %d 0 1 ) %s 0 0 0 0.5 0.5 0 0 0\n( %d 0 0 ) ( %d 0 1 ) ( %d 1 0 ) %s 0 0 0 0.5 0.5 0 0 0\n"
                      "( 0 %d 0 ) ( 0 %d 1 ) ( 1 %d 0 ) %s 0 0 0 0.5 0.5 0 0 0\n( 0 %d 0 ) ( 1 %d 0 ) ( 0 %d 1 ) %s 0 0 0 0.5 0.5 0 0 0\n"
                      "( 0 0 %d ) ( 1 0 %d ) ( 0 1 %d ) %s 0 0 0 0.5 0.5 0 0 0\n( 0 0 %d ) ( 0 1 %d ) ( 1 0 %d ) %s 0 0 0 0.5 0.5 0 0 0\n}\n",
                      x0, x0, x0, w.c_str(), x1, x1, x1, w.c_str(), y0, y0, y0, w.c_str(), y1, y1, y1, w.c_str(), z0, z0, z0, w.c_str(), z1, z1,
                      z1, w.c_str());
        t += z;
    };
    quader(-528, -528, -16, 528, 528, 0);
    quader(-528, -528, 384, 528, 528, 400);
    quader(-528, -528, 0, -512, 528, 384);
    quader(512, -528, 0, 528, 528, 384);
    quader(-512, -528, 0, 512, -512, 384);
    quader(-512, 512, 0, 512, 528, 384);
    t += "}\n{\n\"classname\" \"info_player_start\"\n\"origin\" \"0 -160 24\"\n\"angle\" \"90\"\n}\n";
    t += "{\n\"classname\" \"light\"\n\"origin\" \"0 -96 256\"\n\"light\" \"600\"\n}\n";
    t += "{\n\"classname\" \"misc_model\"\n\"origin\" \"0 0 0\"\n\"model\" \"" + modell + "\"\n}\n";
    return t;
}

// Dreiecke je Shader der .bsp ohne Raum und caulk (modobj::bspDreiecke: alle Flaechen der Karte)
std::map<std::string, int> modellDreiecke(const std::string& bsp) {
    std::map<std::string, int> m;
    for (const auto& [s, n] : modobj::bspDreiecke(bsp)) {
        std::string k = s;
        for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        if (n > 0 && k != kWand && k != "textures/system/caulk" && k != "noshader") { m[k] += n; }
    }
    return m;
}
// Der Huellkasten der Dreiecks-Flaechen (MST_TRIANGLE_SOUP 3: die Modelle) je Shader, auf 1/10 als Text - Lump 13 surfaces
// (RBSP 148 Bytes: Typ +8, firstVert +12, numVerts +16), Lump 10 drawverts (RBSP 80 Bytes, xyz vorn)
std::map<std::string, std::string> modellKaesten(const std::string& bsp) {
    std::map<std::string, std::array<float, 6>> k;
    karte::BspDatei d;
    if (!karte::bspLesen(dateiInhalt(bsp), d) || d.lumps.size() < 14 || d.ident != "RBSP") { return {}; }
    std::vector<std::string> sh;
    for (std::size_t o = 0; o + 72 <= d.lumps[1].size(); o += 72) { sh.emplace_back(d.lumps[1].c_str() + o); }
    for (std::size_t o = 0; o + 148 <= d.lumps[13].size(); o += 148) {
        std::int32_t s = 0, typ = 0, erste = 0, zahl = 0;
        std::memcpy(&s, d.lumps[13].data() + o, 4);
        std::memcpy(&typ, d.lumps[13].data() + o + 8, 4);
        std::memcpy(&erste, d.lumps[13].data() + o + 12, 4);
        std::memcpy(&zahl, d.lumps[13].data() + o + 16, 4);
        if (typ != 3 || s < 0 || static_cast<std::size_t>(s) >= sh.size()) { continue; }
        auto [it, neu] = k.try_emplace(sh[static_cast<std::size_t>(s)], std::array<float, 6>{1e30F, 1e30F, 1e30F, -1e30F, -1e30F, -1e30F});
        (void)neu;
        for (std::int32_t i = 0; i < zahl; ++i) {
            const std::size_t v = static_cast<std::size_t>(erste + i) * 80;
            if (v + 12 > d.lumps[10].size()) { break; }
            float p[3];
            std::memcpy(p, d.lumps[10].data() + v, 12);
            for (std::size_t a = 0; a < 3; ++a) {
                it->second[a] = std::min(it->second[a], p[a]);
                it->second[a + 3] = std::max(it->second[a + 3], p[a]);
            }
        }
    }
    std::map<std::string, std::string> m;
    for (const auto& [n, b] : k) {
        std::string kl = n;
        for (char& c : kl) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        char t[200];
        std::snprintf(t, sizeof(t), "(%.1f %.1f %.1f)-(%.1f %.1f %.1f)", static_cast<double>(b[0]), static_cast<double>(b[1]), static_cast<double>(b[2]),
                      static_cast<double>(b[3]), static_cast<double>(b[4]), static_cast<double>(b[5]));
        m[kl] = t;
    }
    return m;
}
std::set<std::string> namen(const std::map<std::string, int>& m) {
    std::set<std::string> s;
    for (const auto& [k, n] : m) {
        (void)n;
        s.insert(k);
    }
    return s;
}
std::string liste(const std::map<std::string, int>& m) {
    std::string t;
    for (const auto& [s, n] : m) { t += s + " " + std::to_string(n) + ", "; }
    return t.empty() ? std::string("(keine)") : t;
}

// Ein Kasten mit einem Bild, dessen Name einen Punkt hat (wie "stein.v2.tga"): q3map2 nimmt das Bild ohne Endung, also
// textures/kr/stein.v2 (model.cpp AssModelMesh :216 PathExtensionless) - die Modell+Clip-.obj muss den Namen so schreiben, dass
// ein zweites PathExtensionless ihn nicht kuerzt
std::string punktAse() {
    std::string t = "*3DSMAX_ASCIIEXPORT\t200\n*COMMENT \"AsciiExport Version  2,00 - JK Studio self-test KR\"\n"
                    "*MATERIAL_LIST {\n\t*MATERIAL_COUNT 1\n\t*MATERIAL 0 {\n";
    t += aseb::aseMaterial("Standard", "stein", "C:\\Spiele\\Jedi Academy\\GameData\\base\\textures\\kr\\stein.v2.tga") + "\t}\n}\n";
    t += aseb::aseObjekt("block", aseb::Kasten6{{-24, -24, 0}, {24, 24, 48}}, 0, 0, 0);
    return t;
}

struct Zustand {
    std::map<std::string, std::map<std::string, int>> bsp;   // Kartenname -> Dreiecke je Shader
    std::map<std::string, std::map<std::string, std::string>> kasten;   // Kartenname -> Huellkasten der Modellflaechen je Shader
};

void bauen(std::vector<Schritt>& s, const std::string& name, const std::shared_ptr<Zustand>& z) {
    s.push_back(tu("KR " + name + ": Karte speichern, Build > Test KR", [name] {
        stufe3::q3map2Bereitstellen();
        ke::bauMenueSicherstellen();
        (void)ke::karteSpeichernUnter(kartePfad(name));
        ke::Editor& e = ke::editor();
        if (stufe3::eintragNr("Test KR") < 0) { e.bauProjekt.eintraege.push_back(karte::BauEintrag{"Test KR", {"[q3map2] -meta \"[MapFile]\""}}); }
        std::error_code ec;
        std::filesystem::remove(bspVon(kartePfad(name)), ec);
        ke::konsoleLeeren();
    }));
    s.push_back(stufe3::bauMenuepunkt("Test KR"));
    s.push_back(stufe8::warteSek("KR " + name + ": Bau fertig", [] { return !ke::bauLaeuft(); }, 300));
    s.push_back(pause(0.3));
    s.push_back(tu("KR " + name + ": .bsp lesen", [name, z] {
        z->bsp[name] = modellDreiecke(bspVon(kartePfad(name)));
        z->kasten[name] = modellKaesten(bspVon(kartePfad(name)));
        diag::detail("Knopftest: KR " + name + " - .bsp " + liste(z->bsp[name]));
    }));
}

std::shared_ptr<karte::Entitaet> miscModel() { return aseb::miscModel(); }

// Raum mit dem Modell oeffnen, das misc_model waehlen, umwandeln (art: Auto oder alle als Modell+Clip), einfuegen
void umwandeln(std::vector<Schritt>& s, const std::string& basis, bool clip) {
    s.push_back(tu("KR " + basis + (clip ? ": Modell+Clip" : ": Auto") + " - Raum oeffnen, misc_model waehlen, Modell in Brushes umwandeln", [basis] {
        ke::editor().karte.undo.alsGespeichertMarkieren();
        (void)ke::karteOeffnen(kartePfad(basis + "_modell"));
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        if (const auto en = miscModel()) { e.karte.auswahl.waehlen(en); }
        e.geoImport = ke::GeoImportZustand{};
        ke::modellZuBrushesOeffnen();
        e.geoImport.optimierer = false;
    }));
    s.push_back(pause(0.3));
    if (clip) {
        s.push_back(tu("KR " + basis + ": alle Teile Modell+Clip", [] {
            ke::GeoImportZustand& g = ke::editor().geoImport;
            g.wahl.assign(g.netz.teile.size(), static_cast<int>(karte::TeilArt::ModellClip));
            g.neu = true;
        }));
        // REST Punkt 4: die Vorschau rechnet das Fenster beim Zeichnen neu (vorschauRechnen) - abwarten statt fester Pause
        // (im Volllauf unter Last war sie nach 0,4 s noch die alte: "Art/Brushes 1/1 2/1")
        s.push_back(warteBis("KR " + basis + ": Vorschau mit der neuen Wahl", [] { return !ke::editor().geoImport.neu; }, 600));
    }
    s.push_back(pruefe("KR " + basis + (clip ? ": Modell+Clip" : ": Auto") + " - Fenster offen mit Teilen", [basis, clip] {
        const ke::GeoImportZustand& g = ke::editor().geoImport;
        std::string t;
        for (const auto& v : g.vorschau) { t += std::to_string(static_cast<int>(v.art)) + "/" + std::to_string(v.pinsel.size()) + " "; }
        diag::detail("Knopftest: KR " + basis + " - Teile " + std::to_string(g.netz.teile.size()) + ", Art/Brushes " + t + "Konsole: " + konsoleText().substr(0, 300));
        bool ok = g.offen && !g.netz.teile.empty() && g.vorschau.size() == g.netz.teile.size();
        for (const auto& v : g.vorschau) { ok = ok && (!clip || v.art == karte::TeilArt::ModellClip); }
        return ok;
    }));
    s.push_back(tu("KR " + basis + ": Einfuegen", [] { ke::geoImportEinfuegen(); }));
    s.push_back(pause(0.4));
    s.push_back(pruefe("KR " + basis + ": eingefuegt, das misc_model des Modells ist weg (Modell+Clip: nur das neue im Import-Ordner)", [] {
        bool alt = false;
        for (const auto& en : ke::editor().karte.entitaeten()) {
            alt = alt || (en->klassenname() == "misc_model" && en->wert("model").rfind("models/import/", 0) != 0);
        }
        return !ke::editor().geoImport.offen && !alt;
    }));
}

void modellPruefen(std::vector<Schritt>& s, const std::string& basis, const std::string& modell, const std::shared_ptr<Zustand>& z) {
    s.push_back(tu("KR " + basis + ": Raum mit " + modell + " als misc_model", [basis, modell] {
        stufe2::testDateiSchreiben(kartePfad(basis + "_modell"), raumText(modell));
        ke::editor().karte.undo.alsGespeichertMarkieren();
        (void)ke::karteOeffnen(kartePfad(basis + "_modell"));
    }));
    s.push_back(pause(0.3));
    bauen(s, basis + "_modell", z);
    s.push_back(pruefe("KR " + basis + "_modell: q3map2 backt das Modell ein (Dreiecke je Shader in der .bsp)", [basis, z] {
        return !z->bsp[basis + "_modell"].empty();
    }));
    umwandeln(s, basis, false);
    bauen(s, basis + "_brush", z);
    s.push_back(pruefe("KR " + basis + "_brush: die Brushes zeichnen in der .bsp dieselben Shader wie das misc_model", [basis, z] {
        const auto& soll = z->bsp[basis + "_modell"];
        const auto& ist = z->bsp[basis + "_brush"];
        diag::detail("Knopftest: KR " + basis + "_brush - Shader " + liste(ist) + "| misc_model " + liste(soll));
        return !soll.empty() && namen(ist) == namen(soll);
    }));
    umwandeln(s, basis, true);
    bauen(s, basis + "_clip", z);
    s.push_back(pruefe("KR " + basis + "_clip: Modell+Clip (.obj ohne .mtl) - dieselben Dreiecke je Shader an derselben Stelle (Huellkasten) wie das misc_model", [basis, z] {
        const auto& soll = z->bsp[basis + "_modell"];
        const auto& ist = z->bsp[basis + "_clip"];
        std::string kt;
        for (const auto& [n, b] : z->kasten[basis + "_clip"]) { kt += n + " " + b + " / " + z->kasten[basis + "_modell"][n] + "; "; }
        diag::detail("Knopftest: KR " + basis + "_clip - " + liste(ist) + "| misc_model " + liste(soll) + "| Kaesten " + kt);
        return !soll.empty() && ist == soll && z->kasten[basis + "_clip"] == z->kasten[basis + "_modell"];
    }));
}

// Spiegel-Modus "Ebene an der Auswahl": die Mitte der Auswahl aufs Raster wie NRC vector3_snap (float_snapped: llrint, bei genau
// halbem Raster zur geraden Zahl). Brush x 0..16 (Mitte 8), Raster 16: NRC 0 - std::round gab 16.
std::vector<Schritt> spiegelSchritte() {
    std::vector<Schritt> s;
    auto ebene = std::make_shared<std::array<double, 3>>();
    const auto messen = [ebene](int i, int x0, int x1) {
        return tu("KR Spiegel: Brush x " + std::to_string(x0) + ".." + std::to_string(x1) + ", Raster 16, Ebene X an der Auswahl", [ebene, i, x0, x1] {
            const std::string w = std::string(kWand).substr(9);
            char z[900];
            std::snprintf(z, sizeof(z),
                          "{\n\"classname\" \"worldspawn\"\n{\n( %d 0 0 ) ( %d 1 0 ) ( %d 0 1 ) %s 0 0 0 0.5 0.5 0 0 0\n"
                          "( %d 0 0 ) ( %d 0 1 ) ( %d 1 0 ) %s 0 0 0 0.5 0.5 0 0 0\n( 0 0 0 ) ( 0 0 1 ) ( 1 0 0 ) %s 0 0 0 0.5 0.5 0 0 0\n"
                          "( 0 64 0 ) ( 1 64 0 ) ( 0 64 1 ) %s 0 0 0 0.5 0.5 0 0 0\n( 0 0 0 ) ( 1 0 0 ) ( 0 1 0 ) %s 0 0 0 0.5 0.5 0 0 0\n"
                          "( 0 0 64 ) ( 0 1 64 ) ( 1 0 64 ) %s 0 0 0 0.5 0.5 0 0 0\n}\n}\n",
                          x0, x0, x0, w.c_str(), x1, x1, x1, w.c_str(), w.c_str(), w.c_str(), w.c_str(), w.c_str());
            const std::string pfad = kartePfad("spiegel");
            stufe2::testDateiSchreiben(pfad, z);
            ke::Editor& e = ke::editor();
            e.karte.undo.alsGespeichertMarkieren();
            (void)ke::karteOeffnen(pfad);
            e.raster.setzePotenz(4);
            e.karte.auswahl.leeren();
            const auto p = tutgeo::weltPinsel();
            if (!p.empty()) { e.karte.auswahl.waehlen(e.karte.welt(), p.front()); }
            ke::spiegelSetzen(0);
            e.spiegel.ebene = 999;
            ke::spiegelEbeneAnAuswahl();
            (*ebene)[static_cast<std::size_t>(i)] = e.spiegel.ebene;
            ke::spiegelSetzen(-1);
        });
    };
    s.push_back(messen(0, 0, 16));
    s.push_back(messen(1, -16, 0));
    s.push_back(messen(2, 16, 32));
    s.push_back(pruefe("KR Spiegel: Mitte 8 -> 0, Mitte -8 -> 0, Mitte 24 -> 32 (Raster 16, wie NRC vector3_snap)", [ebene] {
        diag::detail("Knopftest: KR Spiegel - Ebenen " + std::to_string((*ebene)[0]) + " " + std::to_string((*ebene)[1]) + " " + std::to_string((*ebene)[2]));
        return (*ebene)[0] == 0.0 && (*ebene)[1] == 0.0 && (*ebene)[2] == 32.0;
    }));
    return s;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<Zustand>();
    for (Schritt& x : spiegelSchritte()) { add(std::move(x)); }
    add(tu("KR: truhe.ase (ASEB) in den Ordner des Laufs (Extra Resource Path)", [] {
        karteDialogeZu();
        stufe2::testDateiSchreiben(aseb::datei("truhe.ase"), aseb::truheAse());
        stufe2::testDateiSchreiben(aseb::datei("punkt.ase"), punktAse());
        ke::spielNeuRealisieren("Edit Project Settings", [] { ke::editor().zusatzPfade[0] = karte::ordnerBereinigt(aseb::quelle()); });
    }));
    add(warteBis("KR: Spieldaten geladen (truhe.ase und der Stuhl aus Movie Duels)", [] {
        ke::spielPruefen();
        return ke::spielBereit() && ke::editor().karteSpiel && ke::editor().karteSpiel->fs.finden("models/aseb/truhe.ase") != "";
    }, 20000));
    add(pruefe(std::string("KR: ") + kStuhl + " (3ds-Max-Export) in den Spieldaten", [] {
        const std::string p = ke::editor().karteSpiel->fs.finden(kStuhl);
        diag::detail("Knopftest: KR - Stuhl " + (p.empty() ? std::string("fehlt") : p));
        return !p.empty();
    }));
    modellPruefen(s, "truhe", "models/aseb/truhe.ase", z);
    modellPruefen(s, "punkt", "models/aseb/punkt.ase", z);
    modellPruefen(s, "stuhl", kStuhl, z);
    // Datei > Geometrie als Brushes importieren... mit derselben echten .ase als lose Datei (aus der .pk3 in den Ordner des Laufs)
    add(tu("KR stuhl: to_chair.ase aus der .pk3 als Datei, Datei > Geometrie als Brushes importieren (geoImportLaden)", [] {
        std::string b;
        if (!ke::editor().karteSpiel || !ke::editor().karteSpiel->fs.dateiLesen(kStuhl, b)) { meldeFehler("KR stuhl: to_chair.ase nicht lesbar"); }
        stufe2::testDateiSchreiben(aseb::datei("to_chair.ase"), b);
        aseb::neueKarte();
        ke::editor().geoImport = ke::GeoImportZustand{};
        (void)ke::geoImportLaden(aseb::datei("to_chair.ase"));
    }));
    add(pause(0.3));
    add(pruefe("KR stuhl Import: Fenster offen, Dreiecke je Shader wie q3map2 in der .bsp des misc_model (Shader aus dem *BITMAP von 3ds Max)", [z] {
        const ke::GeoImportZustand& g = ke::editor().geoImport;
        std::map<std::string, int> ist;
        for (const auto& [sh, n] : aseb::teilDreiecke(g)) {
            std::string k = sh;
            for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            ist[k] += static_cast<int>(n);
        }
        diag::detail("Knopftest: KR stuhl Import - " + liste(ist) + "| .bsp " + liste(z->bsp["stuhl_modell"]));
        return g.offen && ist == z->bsp["stuhl_modell"];
    }));
    add(tu("KR stuhl Import: Abbrechen", [] { ke::editor().geoImport = ke::GeoImportZustand{}; }));
    // Ladetest: die gebaute Karte mit dem Stuhl aus Brushes in "Gebaute Karte wie im Spiel"
    umwandeln(s, "stuhl", false);
    add(tu("KR stuhl: Schnell-Backen, Spielbild, Kamera vor dem Stuhl", [] {
        ke::Editor& e = ke::editor();
        e.kamera.ursprung = karte::Vektor3f(0.0F, -160.0F, 64.0F);
        e.kamera.winkel = karte::Vektor3f(-10.0F, 90.0F, 0.0F);
        e.filterMaske = 0;
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.backen = true;
        e.echtLicht.spielbild = true;
    }));
    add(stufe9c::langeWarten("KR stuhl: Schnell-Backen fertig", [] {
        const ke::Editor& e = ke::editor();
        if (e.echtLicht.bauStand == 5) {
            meldeFehler("KR stuhl: Schnell-Backen gescheitert");
            return true;
        }
        return e.echtLicht.gebackenZeigen;
    }, 400000));
    add(stufe9c::langeWarten("KR stuhl: Spielbild da", [] { return ke::editor().echtLicht.spielbildGezeigt; }, 20000));
    add(warteBis("KR stuhl: Texturen", [] { return ke::kameraBilderFertig(); }, 20000));
    add(pause(1.0));
    add(foto("kr_stuhl_spielbild"));
    add(pruefe("KR stuhl: die gebaute Karte laedt in \"Gebaute Karte wie im Spiel\" (Spielbild gezeigt, kein Fehler)", [] {
        const ke::Editor& e = ke::editor();
        return e.echtLicht.bauStand != 5 && e.echtLicht.spielbildGezeigt;
    }));
    add(tu("KR: Ende - Echtzeit aus, neue Karte, Events", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.geoImport = ke::GeoImportZustand{};
        aseb::neueKarte();
        g_app->leftMode = 0;
    }));
    return s;
}

}  // namespace kreste
