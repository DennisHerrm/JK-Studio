// selbsttest_karte8b.inc - Stufe 8b des Karten-Editors: neue Editor-Werkzeuge
// (Terrain Generator, frei belegbare Split-Fenster, Im Editor spielen,
// Logik-Ansicht, Ansichtsmodi, Spiegel-Modus, Undo-Verlauf, misc_model-Index).
//
// Eingebunden in gui/selbsttest.cpp (namenloser Namensraum, nach Stufe 8a).
// BHED_KARTE_NUR=stufe8b laesst nur diese Schritte laufen; BHED_STUFE8B_NUR=<Teil>
// (terrain, ...) nur einen Teil.
namespace stufe8b {

namespace ke = karteneditor;
namespace tg = karte::terraingen;
using stufe3::konsoleText;
using stufe4::bobKarte;
using stufe4::kastenText;
using stufe4::weltText;
using stufe4::weltWaehlen;

// Klick auf ein Element des Dialogs "###terraingenerator" (Text##id)
Schritt tgKlick(const std::string& text, const char* id) { return klick(text + "##" + id, "###terraingenerator", 0, true); }

std::string kartenText() { return karte::karteSchreiben(ke::editor().karte); }

std::shared_ptr<karte::Entitaet> ersteGruppe(const char* klasse = "func_group") {
    for (const auto& ent : ke::editor().karte.entitaeten()) {
        if (ent->klassenname() == klasse) { return ent; }
    }
    return nullptr;
}
int gruppenAnzahl() {
    int n = 0;
    for (const auto& ent : ke::editor().karte.entitaeten()) { n += ent->klassenname() == "func_group" ? 1 : 0; }
    return n;
}

// --- Punkt 1: Terrain Generator -------------------------------------------------------
//
// Wirkung: Plugins > TerrainGenerator > Generate Terrain oeffnet den nicht modalen
// Dialog (die Auswahl live 512/256/64, die Textur aus dem Texturbrowser); Base
// Shape per Klick, fester Startwert (eigene Zeile), Generate: der Auswahl-Pinsel
// weg, eine func_group mit 64 detail-Pinseln und _terraingen_size, die erste
// Dreiecksflaeche genau wie generate_height_map mit demselben Startwert; ein
// Rueckgaengig stellt die Karte Byte fuer Byte her, Wiederholen das Terrain;
// gleicher Startwert = gleiche Karte, ohne festen Startwert laeuft die Folge
// weiter (wie NRC); Kasten "No Selection"; Manual Size 1024 x 1024 um den
// Ursprung; Tunnel = 4 func_group; Irregular Grid verschiebt innere Ecken; Reset
// to Default; der Texturbrowser fuellt das Feld; Pick zeigt die Texturseite im
// Gruppendialog (Split); Close behaelt die Werte bis zum naechsten Oeffnen;
// About.
std::vector<Schritt> terrain() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(stufe2::grundzustand("Stufe 8b/1: Grundzustand"));
    add(pruefe("Terrain Generator: Menue Plugins > TerrainGenerator mit Generate Terrain und About (nachgebaut), zwischen SunPlug und UFO:AI", [] {
        std::vector<std::string> menues;
        int n = 0;
        for (const auto& p : karte::befehle::pluginEintraege()) {
            if (menues.empty() || menues.back() != p.menue) { menues.emplace_back(p.menue); }
            if (std::string(p.menue) == "TerrainGenerator" && p.befehl[0] != 0) { n += ke::nachgebaut(p.befehl) ? 1 : 0; }
        }
        const auto i = std::find(menues.begin(), menues.end(), "TerrainGenerator");
        return n == 2 && i != menues.end() && i != menues.begin() && *(i - 1) == "SunPlug" && (i + 1) != menues.end() && *(i + 1) == "UFO:AI";
    }));
    add(tu("About", [] { (void)ke::ausfuehren("terrainGenerator::About"); }));
    add(pause(0.3));
    add(pruefe("Terrain Generator > About: Kasten mit \"Developed by vallz and vld\"", [] {
        const ke::Editor& e = ke::editor();
        return e.meldung.offen && e.meldung.text.find("Developed by vallz and vld") != std::string::npos;
    }));
    add(tu("Kasten zu; Karte: ein Pinsel 0..512 x 0..256 x 0..64, gewaehlt; Texturbrowser auf textures/test/gras; Generate Terrain", [] {
        ke::editor().meldung.offen = false;
        bobKarte(weltText(kastenText(0, 0, 0, 512, 256, 64, "test/a")));
        weltWaehlen({0});
        ke::Editor& e = ke::editor();
        e.texBro.shader = "textures/test/gras";
        e.terrain.gespeichert = tg::Werte{};
        e.terrain.erzeugt = 0;
        (void)ke::ausfuehren("terrainGenerator::GenerateTerrain");
    }));
    add(pause(0.4));
    add(pruefe("Generate Terrain: der Dialog offen, Use Selection 512 / 256 / 64, Textur aus dem Texturbrowser, Vorgaben wie NRC (64, Flat, Perlin, 32, 0.005)", [] {
        const ke::Editor& e = ke::editor();
        const tg::Werte& w = e.terrain.w;
        return e.terrain.offen && stufe3::bauFeld("Plugin:TgDialog") != nullptr && stufe3::bauFeld("Plugin:tgselw") != nullptr && w.ziel == 0 &&
               w.textur == "textures/test/gras" && tg::auswahlGrenzen(e.karte).valid && w.quadratIndex == 3 && w.form == 0 && w.rauschen == 0 &&
               w.varianz == 32.0 && w.frequenz == 0.005;
    }));
    add(foto("karteneditor_s8b_terrain_dialog"));
    // Base Shape per Klick: Hill
    add(klick("##tgform", "###terraingenerator", 0, false));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeTgShHill)) + "##tgform1", "", 0, true));
    add(pause(0.2));
    add(pruefe("Terrain Generator > Base Shape: Hill gewaehlt, die Zeile \"Peak Height:\" erscheint", [] {
        return ke::editor().terrain.w.form == 1 && stufe3::bauFeld("Plugin:tgformhoehe") != nullptr;
    }));
    add(tgKlick(ke::trK(Str::KeTgSeedFix), "tgseedfest"));
    add(pause(0.2));
    add(tu("Startwert 1234, Peak Height 128", [] {
        ke::editor().terrain.w.startwert = 1234;
        ke::editor().terrain.w.formHoehe = 128;
    }));
    auto vorher = std::make_shared<std::string>();
    auto nachher = std::make_shared<std::string>();
    add(tu("Karte merken", [vorher] { *vorher = kartenText(); }));
    add(tgKlick(ke::trK(Str::KeTgGenerate), "tggenerate"));
    add(pause(0.3));
    add(pruefe("Terrain Generator > Generate: Auswahl-Pinsel weg, 1 func_group mit 64 detail-Pinseln, _terraingen_size \"512 256 64\", gewaehlt, Konsole", [nachher] {
        const ke::Editor& e = ke::editor();
        const auto g = ersteGruppe();
        bool detail = g != nullptr;
        if (g) {
            for (const auto& p : g->kinder()) { detail = detail && p->art() == karte::Primitiv::Art::Pinsel && static_cast<const karte::Pinsel&>(*p).istDetail(); }
        }
        *nachher = kartenText();
        diag::detail("Knopftest: Stufe 8b/1 Gruppen " + std::to_string(gruppenAnzahl()) + ", Pinsel " + std::to_string(g ? g->kinder().size() : 0) +
                     ", Welt " + std::to_string(e.karte.welt()->kinder().size()) + ", gewaehlt " + std::to_string(e.karte.auswahl.anzahl()));
        return e.terrain.erzeugt == 1 && gruppenAnzahl() == 1 && g && g->kinder().size() == 64 && detail && g->wert("_terraingen_size") == "512 256 64" &&
               e.karte.welt()->kinder().empty() && e.karte.auswahl.anzahl() == 64 &&
               konsoleText().find("TerrainGenerator: generating terrain - bounds (512 x 256 x 64), step (64 x 64), texture: textures/test/gras") !=
                   std::string::npos &&
               konsoleText().find("TerrainGenerator: generation complete") != std::string::npos;
    }));
    add(pruefe("Generate: die erste Dreiecksflaeche genau wie generate_height_map mit Startwert 1234 (Hill 128, Perlin 32/0.005)", [] {
        const auto g = ersteGruppe();
        if (!g || g->kinder().empty()) { return false; }
        tg::BrushData ziel;
        ziel.min_x = 0;
        ziel.max_x = 512;
        ziel.min_y = 0;
        ziel.max_y = 256;
        ziel.min_z = 0;
        ziel.max_z = 64;
        ziel.width_x = 512;
        ziel.length_y = 256;
        ziel.height_z = 64;
        std::srand(1234);
        const tg::TerrainMap m = tg::generate_height_map(ziel, 64, 64, tg::ShapeType::Hill, 128, 32, 0.005, tg::NoiseType::Perlin, 0, tg::Axis::X, false, 1,
                                                         [] { return std::rand(); });
        const auto& p = static_cast<const karte::Pinsel&>(*g->kinder().front());
        const karte::EbenenPunkte& e = p[0].ebenenPunkte();
        const tg::GridPoint bl = m.at({0.0, 0.0});
        const tg::GridPoint tl = m.at({0.0, 64.0});
        const tg::GridPoint br = m.at({64.0, 0.0});
        diag::detail("Knopftest: Stufe 8b/1 erste Flaeche (" + std::to_string(e[0][2]) + " " + std::to_string(e[1][2]) + " " + std::to_string(e[2][2]) +
                     ") erwartet (" + std::to_string(bl.z) + " " + std::to_string(tl.z) + " " + std::to_string(br.z) + ")");
        return e[0][0] == bl.x && e[0][1] == bl.y && e[0][2] == bl.z && e[1][2] == tl.z && e[2][2] == br.z && p[0].shader() == "textures/test/gras";
    }));
    add(foto("karteneditor_s8b_terrain_huegel"));
    add(tu("Undo", [] { (void)ke::ausfuehren("Undo"); }));
    add(pruefe("Terrain Generator > Generate, Rueckgaengig: ein Schritt stellt die Karte Byte fuer Byte her (der Pinsel ist zurueck)", [vorher] {
        return kartenText() == *vorher && gruppenAnzahl() == 0;
    }));
    add(tu("Redo", [] { (void)ke::ausfuehren("Redo"); }));
    add(pruefe("Wiederholen: das Terrain Byte fuer Byte", [nachher] { return kartenText() == *nachher; }));
    add(tu("Undo; Pinsel waehlen; noch einmal Generate mit Startwert 1234", [] {
        (void)ke::ausfuehren("Undo");
        weltWaehlen({0});
        ke::terrainErzeugenKnopf();
    }));
    add(pruefe("Terrain Generator > Seed: gleicher Startwert = dieselbe Karte Byte fuer Byte", [nachher] { return kartenText() == *nachher; }));
    add(tu("Undo; Pinsel waehlen; Startwert aus (NRC: die Folge laeuft weiter); zweimal erzeugen", [] {
        (void)ke::ausfuehren("Undo");
        weltWaehlen({0});
        ke::editor().terrain.w.startwertFest = false;
        ke::terrainErzeugenKnopf();
    }));
    auto ohne1 = std::make_shared<std::string>();
    add(tu("merken, Undo, Pinsel waehlen, erzeugen", [ohne1] {
        *ohne1 = kartenText();
        (void)ke::ausfuehren("Undo");
        weltWaehlen({0});
        ke::terrainErzeugenKnopf();
    }));
    add(pruefe("ohne festen Startwert: zwei Laeufe verschieden (die Zufallsfolge der C-Laufzeit laeuft weiter wie in NRC)", [ohne1, nachher] {
        return kartenText() != *ohne1 && *ohne1 != *nachher && gruppenAnzahl() == 1;
    }));
    // Kasten: keine Auswahl
    add(tu("Auswahl leeren", [] {
        ke::editor().karte.auswahl.leeren();
        ke::auswahlGeaendert();
    }));
    add(tgKlick(ke::trK(Str::KeTgGenerate), "tggenerate"));
    add(pause(0.3));
    add(pruefe("Generate ohne Auswahl: Kasten \"Terrain Generator - No Selection\", Karte unveraendert", [] {
        const ke::Editor& e = ke::editor();
        return e.meldung.offen && e.meldung.titel == ke::trK(Str::KeTgNoSelTitel) && gruppenAnzahl() == 1;
    }));
    add(tu("Kasten zu; Target per Klick auf Manual Size", [] { ke::editor().meldung.offen = false; }));
    add(klick("##tgziel", "###terraingenerator", 0, false));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeTgManual)) + "##tgziel1", "", 0, true));
    add(pause(0.2));
    add(pruefe("Terrain Generator > Target: Manual Size, die SpinBox-Zeilen 1024 / 1024 / 64", [] {
        const tg::Werte& w = ke::editor().terrain.w;
        return w.ziel == 1 && stufe3::bauFeld("Plugin:tgmanw") != nullptr && w.manuellB == 1024 && w.manuellH == 64;
    }));
    add(tgKlick(ke::trK(Str::KeTgGenerate), "tggenerate"));
    add(pause(0.3));
    add(pruefe("Generate mit Manual Size: eine zweite func_group 1024 x 1024 um den Ursprung (256 Pinsel), ohne _terraingen_size", [] {
        std::shared_ptr<karte::Entitaet> neu;
        for (const auto& ent : ke::editor().karte.entitaeten()) {
            if (ent->klassenname() == "func_group" && ent->wert("_terraingen_size").empty()) { neu = ent; }
        }
        if (!neu) { return false; }
        const karte::Kasten k = karte::auswahlGrenzenPivot(ke::editor().karte);
        return gruppenAnzahl() == 2 && neu->kinder().size() == 16U * 16U * 2U && k.origin[0] == 0 && k.origin[1] == 0 && k.extents[0] == 512;
    }));
    // Tunnel (Manual Size), Achse Y
    add(tu("Base Shape Tunnel, Axis Y, Variance 0", [] {
        tg::Werte& w = ke::editor().terrain.w;
        w.form = static_cast<int>(tg::ShapeType::Tunnel);
        w.achse = 1;
        w.varianz = 0;
    }));
    add(pause(0.2));
    add(pruefe("Tunnel: Axis-Zeile da, Irregular Grid und Terrace ausgeblendet", [] {
        return stufe3::bauFeld("Plugin:tgachse") != nullptr && ke::editor().terrain.w.form == 7;
    }));
    add(tgKlick(ke::trK(Str::KeTgGenerate), "tggenerate"));
    add(pause(0.3));
    add(pruefe("Terrain Generator > Generate (Tunnel): vier neue func_group (Boden, Decke, zwei Waende)", [] { return gruppenAnzahl() == 6; }));
    // Irregular Grid
    add(tu("Flat, Variance 0, Irregular Grid per Klick, Raster 8", [] {
        tg::Werte& w = ke::editor().terrain.w;
        w.form = 0;
        w.rasterIndex = 3;
        w.unregelmaessig = false;
    }));
    add(pause(0.2));
    add(tgKlick(ke::trK(Str::KeTgIrregular), "tgjitter"));
    add(pause(0.2));
    add(tgKlick(ke::trK(Str::KeTgGenerate), "tggenerate"));
    add(pause(0.3));
    add(pruefe("Terrain Generator > Irregular Grid: an, innere Ecken nicht mehr auf dem 64er-Raster (Vielfache von 8)", [] {
        const ke::Editor& e = ke::editor();
        int verschoben = 0;
        int ecken = 0;
        for (const auto& el : e.karte.auswahl.elemente()) {
            const auto p = el.primitiv.lock();
            if (!p || p->art() != karte::Primitiv::Art::Pinsel) { continue; }
            const karte::EbenenPunkte& ep = static_cast<const karte::Pinsel&>(*p)[0].ebenenPunkte();
            for (int i = 0; i < 3; ++i) {
                ++ecken;
                const double x = ep[static_cast<std::size_t>(i)][0];
                verschoben += (std::fmod(std::fabs(x), 64.0) != 0.0 && std::fmod(std::fabs(x), 8.0) == 0.0) ? 1 : 0;
            }
        }
        diag::detail("Knopftest: Stufe 8b/1 unregelmaessig: " + std::to_string(verschoben) + " von " + std::to_string(ecken) + " Ecken verschoben");
        return e.terrain.w.unregelmaessig && verschoben > 0;
    }));
    // Texturbrowser fuellt das Feld; Pick
    add(tu("Texturbrowser: textures/test/fels", [] { ke::editor().texBro.shader = "textures/test/fels"; }));
    add(pause(0.2));
    add(pruefe("Terrain Generator > Texture: der Texturbrowser fuellt das Feld (pick_timer)", [] { return ke::editor().terrain.w.textur == "textures/test/fels"; }));
    add(tu("Anordnung Split, Gruppendialog zu, Seite Entities", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Split;
        e.gruppenDialogOffen = false;
        e.gruppenSeite = 0;
    }));
    add(pause(0.3));
    add(tgKlick(ke::trK(Str::KeTgPick), "tgpick"));
    add(pause(0.2));
    add(pruefe("Terrain Generator > Pick: im Split-Layout zeigt es die Seite Textures des Gruppendialogs", [] {
        const ke::Editor& e = ke::editor();
        return e.gruppenDialogOffen && e.gruppenSeite == 2;
    }));
    add(tu("Anordnung zurueck auf Regular", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Regular;
        e.gruppenDialogOffen = false;
    }));
    add(pause(0.3));
    // Reset to Default
    add(tgKlick(ke::trK(Str::KeTgReset), "tgreset"));
    add(pause(0.2));
    add(pruefe("Terrain Generator > Reset to Default: alle Werte auf die Vorgabe, Target bleibt Manual Size, Textur \"textures/\"", [] {
        const tg::Werte& w = ke::editor().terrain.w;
        return w.ziel == 1 && w.form == 0 && w.achse == 0 && w.varianz == 32.0 && w.frequenz == 0.005 && w.rasterIndex == 0 && !w.unregelmaessig &&
               w.textur == "textures/" && !w.startwertFest;
    }));
    add(tu("Form Valley, Manual Width 2048", [] {
        tg::Werte& w = ke::editor().terrain.w;
        w.form = static_cast<int>(tg::ShapeType::Valley);
        w.manuellB = 2048;
    }));
    add(tgKlick(ke::trK(Str::KeTgClose), "tgclose"));
    add(pause(0.2));
    add(pruefe("Terrain Generator > Close: der Dialog zu, die Werte gemerkt (static)", [] {
        const ke::TerrainZustand& z = ke::editor().terrain;
        return !z.offen && z.gespeichert.form == 6 && z.gespeichert.manuellB == 2048 && z.gespeichert.ziel == 1;
    }));
    add(tu("wieder oeffnen, zweimal (s_active_dialog)", [] {
        (void)ke::ausfuehren("terrainGenerator::GenerateTerrain");
        (void)ke::ausfuehren("terrainGenerator::GenerateTerrain");
    }));
    add(pause(0.3));
    add(pruefe("wieder geoeffnet: Valley und Width 2048 sind noch da, Valley Depth statt Peak Height, nur ein Dialog", [] {
        const ke::TerrainZustand& z = ke::editor().terrain;
        int dialoge = 0;
        for (ImGuiWindow* w : GImGui->Windows) {
            if (w->WasActive && std::strstr(w->Name, "###terraingenerator") != nullptr && std::strstr(w->Name, "/") == nullptr) { ++dialoge; }
        }
        return z.offen && z.w.form == 6 && z.w.manuellB == 2048 && dialoge == 1;
    }));
    add(tu("Dialog zu (Werte zurueck auf die Vorgabe)", [] {
        ke::terrainSchliessen();
        ke::editor().terrain.gespeichert = tg::Werte{};
    }));
    return s;
}

// --- Punkt 2: Split-Fenster frei belegen --------------------------------------------
//
// Wirkung: ohne Schalter wie NRC (Kamera oben links, YZ unten links, XY oben
// rechts, XZ unten rechts, kein Platzmenue, Strg+Tab aendert im Split nichts am
// Typ, gleich geteilt); Preferences > Layout > "Free split windows" an; das
// kleine Menue: Kamera nach unten rechts (tauscht mit XZ), XY im Platz oben
// links (zwei XY-Fenster); Strg+Tab im aktiven Fenster wechselt den Typ; den
// Teiler ziehen merkt die Groessen; ein simulierter Neustart (Speichern, Zustand
// verwerfen, Laden) stellt Belegung, Typen, Groessen und den Regular-Typ her;
// Schalter aus = sofort wieder NRC. Der echte Neustart: BHED_KARTE_NUR=split1/2/3.

const ke::Editor::Rechteck* feldVon(const char* name) { return stufe3::bauFeld(name); }

int platzMenues() {
    int n = 0;
    for (ImGuiWindow* w : GImGui->Windows) {
        if (w->WasActive && !w->Hidden && std::strstr(w->Name, "##platzknopf") != nullptr) { ++n; }
    }
    return n;
}

// Bildschirmlage eines Punktes in logischen Punkten des Arbeitsbereichs
ImVec2 bildLage(float x, float y) {
    const ImVec2 vp = ImGui::GetMainViewport()->Pos;
    const float s = ke::massstab();
    return ImVec2{vp.x + x * s, vp.y + y * s};
}

// Die Feldnamen je Platz (0 oben links, 1 unten links, 2 oben rechts, 3 unten rechts)
std::string platzFeld(int platz) {
    static const char* const namen[4] = {"XY", "XZ", "YZ", "Kamera"};
    return namen[ke::editor().splitBelegung[static_cast<std::size_t>(platz)]];
}

std::string typenText() {
    const ke::Editor& e = ke::editor();
    return std::to_string(static_cast<int>(e.xy[0].ansicht.typ)) + std::to_string(static_cast<int>(e.xy[1].ansicht.typ)) +
           std::to_string(static_cast<int>(e.xy[2].ansicht.typ)) + " aktiv " + std::to_string(e.aktivXy);
}

bool lageWieNrc() {
    const auto* k = feldVon("Kamera");
    const auto* xy = feldVon("XY");
    const auto* xz = feldVon("XZ");
    const auto* yz = feldVon("YZ");
    return k && xy && xz && yz && k->x < xy->x && k->y < yz->y && k->x == yz->x && xy->x == xz->x && xy->y < xz->y;
}

void splitZurueck() {
    ke::Editor& e = ke::editor();
    e.splitFrei = false;
    e.splitBelegung = {3, 2, 0, 1};
    e.splitHGroessen.clear();
    e.splitVGroessen.clear();
    e.splitV2Groessen.clear();
    e.xy[0].ansicht.typ = karte::Sicht::XY;
    e.xy[1].ansicht.typ = karte::Sicht::XZ;
    e.xy[2].ansicht.typ = karte::Sicht::YZ;
    e.anordnung = karte::befehle::Anordnung::Regular;
    e.teilerFuer = -1;
    ke::einstellungenSpeichern();
}

// Mitte des kleinen Menueknopfs eines Platzes (merkeFeld "Platzmenue<n>")
ImVec2 platzKnopf(int platz) {
    const auto* f = feldVon(("Platzmenue" + std::to_string(platz)).c_str());
    return f ? bildLage(static_cast<float>(f->x) + 8.0F, static_cast<float>(f->y) + 8.0F) : ImVec2{0, 0};
}

// Mitte des Griffs zwischen den Spalten (hSplit, Griff vor Kind 1)
ImVec2 spaltenGriff() {
    const Element* el = finde("##griffh1", "", 0, true);
    return el != nullptr ? el->rect.GetCenter() : ImVec2{0, 0};
}

std::vector<Schritt> split() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(stufe2::grundzustand("Stufe 8b/2: Grundzustand"));
    add(tu("Split-Layout, Schalter aus (NRC)", [] {
        splitZurueck();
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Split;
        e.teilerFuer = -1;
        e.aktivXy = 0;
    }));
    add(pause(0.4));
    add(pruefe("Split ohne Schalter wie NRC: Kamera oben links, YZ unten links, XY oben rechts, XZ unten rechts, kein Platzmenue, gleich geteilt", [] {
        const auto* k = feldVon("Kamera");
        const auto* xy = feldVon("XY");
        return lageWieNrc() && platzMenues() == 0 && k && xy && std::abs(k->b - xy->b) <= 1;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Tab, "Strg+Tab"));
    add(pause(0.2));
    add(pruefe("Strg+Tab im Split ohne Schalter: XY bleibt XY (XY_SetViewType zentriert nur, wie NRC)", [] {
        diag::detail("Knopftest: Stufe 8b/2 nach Strg+Tab Typen " + typenText());
        return ke::editor().xy[0].ansicht.typ == karte::Sicht::XY && lageWieNrc();
    }));
    // Schalter ueber Preferences > Layout
    add(tu("Preferences, Seite Layout", [] {
        ke::editor().einstellungenOffen = true;
        ke::einstellungsSeiteZeigen(5);
    }));
    add(pause(0.4));
    add(klick(std::string(ke::trK(Str::KeSfSchalter)) + "##splitfrei", "###karteeinstellungen", 0, true));
    add(pause(0.2));
    add(klick(ke::trK(Str::KePrefOk), "###karteeinstellungen"));
    add(pause(0.4));
    add(pruefe("Preferences > Layout > Free split windows: an, als BhedSplitFree 1 gespeichert, vier Platzmenues", [] {
        const auto it = g_app->settings.karte.find("BhedSplitFree");
        diag::detail("Knopftest: Stufe 8b/2 nach OK Typen " + typenText());
        return ke::editor().splitFrei && it != g_app->settings.karte.end() && it->second == "1" && platzMenues() == 4;
    }));
    add(foto("karteneditor_s8b_split_menue"));
    // Platzmenue unten rechts: Kamera
    add(klickAn("Platzmenue unten rechts", [] { return platzKnopf(3); }, 0));
    add(pause(0.3));
    add(foto("karteneditor_s8b_split_popup"));
    add(klick(std::string(ke::trK(Str::KeViewCamera)) + "##platzwahl0", "", 0, true));
    add(pause(0.3));
    add(pruefe("Split > Platzmenue > Camera unten rechts: die Kamera ist dort, XZ ging nach oben links", [] {
        const auto* k = feldVon("Kamera");
        const auto* xz = feldVon("XZ");
        const auto* xy = feldVon("XY");
        diag::detail("Knopftest: Stufe 8b/2 nach Camera Typen " + typenText());
        return platzFeld(3) == "Kamera" && platzFeld(0) == "XZ" && k && xz && xy && k->x == xy->x && k->y > xy->y && xz->x < xy->x && xz->y == xy->y;
    }));
    add(klickAn("Platzmenue oben links", [] { return platzKnopf(0); }, 0));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeViewXY)) + "##platzwahl1", "", 0, true));
    add(pause(0.3));
    add(pruefe("Split > Platzmenue > XY oben links: das Fenster dort zeigt XY - zwei XY-Fenster", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Stufe 8b/2 Belegung " + std::to_string(e.splitBelegung[0]) + std::to_string(e.splitBelegung[1]) +
                     std::to_string(e.splitBelegung[2]) + std::to_string(e.splitBelegung[3]) + ", Typen " +
                     std::to_string(static_cast<int>(e.xy[0].ansicht.typ)) + std::to_string(static_cast<int>(e.xy[1].ansicht.typ)) +
                     std::to_string(static_cast<int>(e.xy[2].ansicht.typ)) + ", aktiv " + std::to_string(e.aktivXy));
        return e.xy[1].ansicht.typ == karte::Sicht::XY && e.xy[0].ansicht.typ == karte::Sicht::XY && e.aktivXy == 1;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Tab, "Strg+Tab"));
    add(pause(0.2));
    add(pruefe("Split > Strg+Tab mit Schalter: das aktive Fenster wechselt den Typ (XY -> XZ wie XY_NextView), gespeichert", [] {
        const auto it = g_app->settings.karte.find("BhedViewTypes");
        diag::detail("Knopftest: Stufe 8b/2 nach Strg+Tab mit Schalter " + typenText() + ", gespeichert " +
                     (it != g_app->settings.karte.end() ? it->second : std::string("-")));
        return ke::editor().xy[1].ansicht.typ == karte::Sicht::XZ && it != g_app->settings.karte.end() && it->second == "2,1,0";
    }));
    // Teiler zwischen den Spalten 100 Punkte nach links ziehen
    auto vorherB = std::make_shared<int>(0);
    add(tu("Breite der linken Spalte merken", [vorherB] {
        const auto* f = feldVon(platzFeld(0).c_str());
        *vorherB = f ? f->b : 0;
    }));
    add(ziehenAn("Teiler der Spalten 100 nach links", [] { return spaltenGriff(); }, -100.0F * ke::massstab(), 0.0F, 0));
    add(pause(0.3));
    add(pruefe("Split > Teiler ziehen: die linke Spalte ist schmaler, die Groessen gespeichert (BhedSplitH)", [vorherB] {
        const auto* f = feldVon(platzFeld(0).c_str());
        const auto it = g_app->settings.karte.find("BhedSplitH");
        diag::detail("Knopftest: Stufe 8b/2 linke Spalte " + std::to_string(*vorherB) + " -> " + std::to_string(f ? f->b : -1));
        return f && f->b < *vorherB - 50 && it != g_app->settings.karte.end() && !ke::editor().splitHGroessen.empty();
    }));
    // simulierter Neustart
    auto stand = std::make_shared<std::string>();
    add(tu("simulierter Neustart: speichern, Zustand verwerfen, laden", [stand] {
        ke::Editor& e = ke::editor();
        ke::einstellungenSpeichern();
        *stand = std::to_string(e.splitBelegung[0]) + std::to_string(e.splitBelegung[3]) + "/" + std::to_string(static_cast<int>(e.xy[1].ansicht.typ)) +
                 "/" + std::to_string(e.splitHGroessen.empty() ? -1 : e.splitHGroessen[0]);
        e.splitBelegung = {3, 2, 0, 1};
        e.xy[1].ansicht.typ = karte::Sicht::XZ;
        e.splitHGroessen.clear();
        ke::einstellungenLaden();
        e.teilerFuer = -1;
    }));
    add(pause(0.4));
    add(pruefe("Split > nach dem simulierten Neustart: Belegung, 2D-Typen und Split-Groesse wie vorher", [stand] {
        const ke::Editor& e = ke::editor();
        const std::string jetzt = std::to_string(e.splitBelegung[0]) + std::to_string(e.splitBelegung[3]) + "/" +
                                  std::to_string(static_cast<int>(e.xy[1].ansicht.typ)) + "/" +
                                  std::to_string(e.splitHGroessen.empty() ? -1 : e.splitHGroessen[0]);
        const auto g = e.hSplit.groessen();
        diag::detail("Knopftest: Stufe 8b/2 vorher " + *stand + ", nachher " + jetzt);
        return jetzt == *stand && !g.empty() && !e.splitHGroessen.empty() && g[0] == e.splitHGroessen[0];
    }));
    // Regular: der 2D-Typ bleibt mit dem Schalter
    add(tu("Regular-Layout, NextView (XY -> XZ), simulierter Neustart", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Regular;
        e.teilerFuer = -1;
        e.aktivXy = 0;
        (void)ke::ausfuehren("NextView");   // XY -> XZ
        e.xy[0].ansicht.typ = karte::Sicht::XY;   // verwerfen ...
        ke::einstellungenLaden();                 // ... und laden
    }));
    add(pause(0.3));
    add(pruefe("Regular > 2D-Ansichtstyp nach dem simulierten Neustart: XZ (mit Schalter)", [] {
        return ke::editor().xy[0].ansicht.typ == karte::Sicht::XZ;
    }));
    // Schalter aus: sofort NRC
    add(tu("Split; Preferences > Layout", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Split;
        e.teilerFuer = -1;
        e.einstellungenOffen = true;
        ke::einstellungsSeiteZeigen(5);
    }));
    add(pause(0.4));
    add(klick(std::string(ke::trK(Str::KeSfSchalter)) + "##splitfrei", "###karteeinstellungen", 0, true));
    add(pause(0.2));
    add(klick(ke::trK(Str::KePrefOk), "###karteeinstellungen"));
    add(pause(0.4));
    add(pruefe("Free split windows aus: sofort wieder NRC (Lage, Typen XY/XZ/YZ, kein Platzmenue), nichts mehr gemerkt", [] {
        const ke::Editor& e = ke::editor();
        const auto& k = g_app->settings.karte;
        return !e.splitFrei && lageWieNrc() && platzMenues() == 0 && e.xy[0].ansicht.typ == karte::Sicht::XY &&
               e.xy[1].ansicht.typ == karte::Sicht::XZ && e.xy[2].ansicht.typ == karte::Sicht::YZ && k.find("BhedSplitSlots") == k.end();
    }));
    add(tu("Regular, alles zurueck", [] { splitZurueck(); }));
    return s;
}

// Der echte Neustart (drei Laeufe mit einem Einstellungsordner, SP/s8b/split_neustart.sh)
std::vector<Schritt> splitEintritt() {
    std::vector<Schritt> s;
    s.push_back(tu("Stufe 8b/2: Spielordner, Modus Events", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
    }));
    s.push_back(pause(0.3));
    s.push_back(klick(tr(Str::ViewMapEditor), "###main"));
    s.push_back(pause(0.6));
    return s;
}

std::vector<Schritt> splitNeustart(int lauf) {
    std::vector<Schritt> s = splitEintritt();
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    if (lauf == 1) {
        add(tu("Lauf 1: Split, Schalter an, Kamera unten rechts, XZ-Fenster oben links als YZ, Teiler", [] {
            ke::Editor& e = ke::editor();
            e.anordnung = karte::befehle::Anordnung::Split;
            e.splitFrei = true;
            e.teilerFuer = -1;
            ke::einstellungenSpeichern();
            ke::splitPlatzWaehlen(3, 0);
            ke::splitPlatzWaehlen(0, 3);
        }));
        add(pause(0.4));
        add(ziehenAn("Lauf 1: Teiler der Spalten 120 nach links", [] { return spaltenGriff(); }, -120.0F * ke::massstab(), 0.0F, 0));
        add(pause(0.3));
        add(pruefe("Split > Neustart Lauf 1: Belegung XZ(YZ)/YZ/XY/Kamera, Groessen gespeichert", [] {
            const ke::Editor& e = ke::editor();
            const auto& k = g_app->settings.karte;
            const auto it = k.find("BhedSplitH");
            diag::detail("Knopftest: Stufe 8b/2 Lauf 1 " + (it != k.end() ? it->second : std::string("-")) + ", Slots " +
                         (k.count("BhedSplitSlots") ? k.at("BhedSplitSlots") : std::string("-")));
            return e.splitBelegung[3] == 3 && e.splitBelegung[0] == 1 && e.xy[1].ansicht.typ == karte::Sicht::YZ && it != k.end();
        }));
        return s;
    }
    if (lauf == 2) {
        add(pruefe("Split > Neustart Lauf 2: nach dem echten Neustart Belegung, Typ und Groessen wie in Lauf 1", [] {
            const ke::Editor& e = ke::editor();
            const auto& k = g_app->settings.karte;
            const auto g = e.hSplit.groessen();
            const auto* kam = feldVon("Kamera");
            const auto* xz = feldVon("XZ");
            const auto* xy = feldVon("XY");
            diag::detail("Knopftest: Stufe 8b/2 Lauf 2 Slots " + (k.count("BhedSplitSlots") ? k.at("BhedSplitSlots") : std::string("-")) + ", h " +
                         (g.empty() ? std::string("-") : std::to_string(g[0]) + "," + std::to_string(g.size() > 1 ? g[1] : -1)));
            return e.anordnung == karte::befehle::Anordnung::Split && e.splitFrei && e.splitBelegung[3] == 3 && e.splitBelegung[0] == 1 &&
                   e.xy[1].ansicht.typ == karte::Sicht::YZ && !e.splitHGroessen.empty() && !g.empty() && g[0] == e.splitHGroessen[0] && kam &&
                   xz && xy && kam->y > xy->y && xz->x < xy->x;
        }));
        add(tu("Lauf 2: Schalter aus (wie Preferences OK)", [] {
            ke::Editor& e = ke::editor();
            e.splitFrei = false;
            ke::einstellungenSpeichern();
        }));
        return s;
    }
    add(pruefe("Split > Neustart Lauf 3 (Schalter aus): wie NRC - Kamera oben links, Typen XY/XZ/YZ, gleich geteilt", [] {
        const ke::Editor& e = ke::editor();
        const auto* k = feldVon("Kamera");
        const auto* xy = feldVon("XY");
        return e.anordnung == karte::befehle::Anordnung::Split && !e.splitFrei && lageWieNrc() && e.xy[1].ansicht.typ == karte::Sicht::XZ && k && xy &&
               std::abs(k->b - xy->b) <= 1;
    }));
    add(tu("Lauf 3: Regular", [] { splitZurueck(); }));
    return s;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    const char* nur = std::getenv("BHED_STUFE8B_NUR");
    const auto teil = [nur](const char* t) { return nur == nullptr || std::strcmp(nur, t) == 0; };
    const auto dazu = [&s](std::vector<Schritt> x) {
        for (Schritt& y : x) { s.push_back(std::move(y)); }
    };
    if (teil("terrain")) { dazu(terrain()); }
    if (teil("split")) { dazu(split()); }
    return s;
}

}  // namespace stufe8b
