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
using stufe3::bauMenuepunkt;
using stufe3::q3map2Bereitstellen;

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

// --- Punkt 3: Im Editor spielen ------------------------------------------------------
//
// Wirkung: F6 (echte Taste) startet an der Kamera; eingespielte Eingaben (keine
// echte Maus): Fallen auf den Boden (z 24,125), gegen die Wand (haelt bei
// 512 - 16), Stufe 16 gestiegen, Stufe 24 haelt, playerclip (Shader des Spiels
// mit surfaceparm playerclip) haelt, Sprung 31,62 hoch (wie der ctest
// karte_spielen mit der .bsp), N (echte Taste) Noclip durch die Wand, Esc (echte
// Taste) beendet und stellt die Kamera genau her; View > "Play: start at
// info_player_start" beginnt an origin + 9 mit dem Winkel.

std::string playerclipShader() {
    const ke::Editor& e = ke::editor();
    if (!e.spiel) { return {}; }
    for (const karte::ShaderEintrag* s : e.spiel->inventar.shader()) {
        if (s->name.rfind("textures/system/", 0) != 0) { continue; }
        bool clip = false;
        bool fest = true;
        for (const std::string& p : s->surfaceparms) {
            clip = clip || p == "playerclip";
            fest = fest && p != "monsterclip";
        }
        if (clip && fest) { return s->name; }
    }
    return {};
}

std::string spielKarte(const std::string& clip) {
    using stufe4::kastenText;
    std::string t;
    t += kastenText(-1024, -1024, -16, 1024, 1024, 0, "test/boden");
    t += kastenText(512, -1024, 0, 544, 1024, 256, "test/wand");
    t += kastenText(128, -64, 0, 192, 64, 16, "test/stufe");
    t += kastenText(-192, -64, 0, -128, 64, 24, "test/stufe");
    t += kastenText(-64, -448, 0, 64, -384, 128, clip.substr(clip.find('/') + 1).c_str());
    return stufe4::weltText(t) + "{\n\"classname\" \"info_player_start\"\n\"origin\" \"-320 320 24\"\n\"angle\" \"90\"\n}\n";
}

// n Schritte zu 16 ms mit der Eingabe (vor, seit, hoch) und dem Blick gier
void spielLauf(int bilder, signed char vor, signed char hoch, float gier, float* zMax = nullptr) {
    ke::SpielenZustand& z = ke::editor().spielen;
    z.s.gier = gier;
    for (int i = 0; i < bilder; ++i) {
        z.eingabe = NavSpielerEingabe{vor, 0, i == 0 ? hoch : static_cast<signed char>(0)};
        ke::spielenSchritt(16);
        if (zMax != nullptr) { *zMax = std::max(*zMax, z.s.ort[2]); }
    }
    z.eingabe = NavSpielerEingabe{};
}

std::vector<Schritt> spielen() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(stufe2::grundzustand("Stufe 8b/3: Grundzustand"));
    auto clip = std::make_shared<std::string>();
    auto kamera = std::make_shared<std::pair<karte::Vektor3f, karte::Vektor3f>>();
    add(tu("Karte: Boden, Ostwand, Stufen 16/24, playerclip-Brush mit dem Shader des Spiels, info_player_start; Kamera bei (0 -200 100)", [clip, kamera] {
        *clip = playerclipShader();
        stufe4::bobKarte(spielKarte(clip->empty() ? std::string("textures/system/clip") : *clip));
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(karte::Vektor3f(0, -200, 100));
        e.kamera.setzeWinkel(karte::Vektor3f(-10, 90, 0));
        *kamera = {e.kamera.ursprung, e.kamera.winkel};
        e.spielen.testEingabe = true;
        e.spielen.startAnSpawn = false;
        diag::detail("Knopftest: Stufe 8b/3 playerclip-Shader " + *clip);
    }));
    add(pause(0.3));
    add(tasteKombi(ImGuiKey_F6, "F6"));
    add(pause(0.3));
    add(pruefe("View > Play in editor (F6): aktiv, Kollision aus 5 Brushes, Start an der Kamera (Augen 36 ueber dem Kasten)", [clip] {
        const ke::SpielenZustand& z = ke::editor().spielen;
        diag::detail("Knopftest: Stufe 8b/3 Brushes " + std::to_string(z.bericht.pinsel) + ", Seiten " + std::to_string(z.bericht.seiten) + ", Start " +
                     std::to_string(z.s.ort[2]));
        return z.aktiv && z.bericht.pinsel == 5 && std::fabs(z.s.ort[2] - 64.0F) < 0.01F && !clip->empty() &&
               konsoleText().find(ke::trK(Str::KeSpBeendet)) == std::string::npos;
    }));
    add(tu("eingespielt: 1 s ohne Eingabe (Fallen)", [] { spielLauf(62, 0, 0, 90); }));
    add(pruefe("Spielen > Fallen: steht auf dem Boden (z 24,125), die Kamera auf Augenhoehe 60,125", [] {
        const ke::Editor& e = ke::editor();
        return e.spielen.s.amBoden && std::fabs(e.spielen.s.ort[2] - 24.125F) < 0.01F && std::fabs(e.kamera.ursprung[2] - 60.125F) < 0.01F;
    }));
    add(foto("karteneditor_s8b_spielen"));
    add(tu("eingespielt: von (300 0) 2 s nach +x", [] {
        ke::editor().spielen.s.ort = {300, 0, 24.125F};
        spielLauf(125, 127, 0, 0);
    }));
    add(pruefe("Spielen > Wand: das Laufen gegen die Wand haelt bei x 512 - 16 an", [] {
        const auto& s = ke::editor().spielen.s;
        diag::detail("Knopftest: Stufe 8b/3 Wand x " + std::to_string(s.ort[0]));
        return s.ort[0] > 495.5F && s.ort[0] < 496.0F;
    }));
    auto zMax = std::make_shared<float>(0.0F);
    add(tu("eingespielt: von (0 0) 0,8 s nach +x ueber die Stufe 16", [zMax] {
        ke::editor().spielen.s.ort = {0, 0, 24.125F};
        *zMax = 0;
        spielLauf(50, 127, 0, 0, zMax.get());
    }));
    add(pruefe("Spielen > Treppe: die Stufe 16 wird gestiegen (z 40,125)", [zMax] {
        return std::fabs(*zMax - 40.125F) < 0.01F && ke::editor().spielen.s.ort[0] > 150.0F;
    }));
    add(tu("eingespielt: von (0 0) 1,5 s nach -x gegen die Stufe 24", [] {
        ke::editor().spielen.s.ort = {0, 0, 24.125F};
        spielLauf(94, 127, 0, 180);
    }));
    add(pruefe("Spielen > Stufe 24 haelt den Spieler an", [] {
        const auto& s = ke::editor().spielen.s;
        return s.ort[0] < -110.0F && s.ort[0] > -112.5F && s.ort[2] < 24.2F;
    }));
    add(tu("eingespielt: von (0 -300) 1,5 s nach -y gegen den playerclip-Brush", [] {
        ke::editor().spielen.s.ort = {0, -300, 24.125F};
        spielLauf(94, 127, 0, 270);
    }));
    add(pruefe("Spielen > playerclip haelt den Spieler an (y -384 + 16)", [] {
        const auto& s = ke::editor().spielen.s;
        diag::detail("Knopftest: Stufe 8b/3 playerclip y " + std::to_string(s.ort[1]));
        return s.ort[1] > -368.5F && s.ort[1] < -366.0F;
    }));
    add(tu("eingespielt: Sprung aus dem Stand bei (0 -200)", [zMax] {
        ke::editor().spielen.s.ort = {0, -200, 24.125F};
        ke::editor().spielen.s.tempo = {0, 0, 0};
        spielLauf(5, 0, 0, 0);
        *zMax = 0;
        spielLauf(70, 0, 127, 0, zMax.get());
    }));
    add(pruefe("Spielen > Sprung: 31,62 ueber dem Boden - gleich dem Port mit der gebauten .bsp (ctest karte_spielen)", [zMax] {
        diag::detail("Knopftest: Stufe 8b/3 Sprung " + std::to_string(*zMax - 24.125F));
        return std::fabs((*zMax - 24.125F) - 31.6224F) < 0.001F;
    }));
    add(tasteKombi(ImGuiKey_N, "N"));
    add(tu("Noclip: von (300 0 60) 2 s nach +x", [] {
        ke::editor().spielen.s.ort = {300, 0, 60};
        spielLauf(125, 127, 0, 0);
    }));
    add(pruefe("Spielen > N (echte Taste): Noclip an, durch die Wand", [] {
        const auto& s = ke::editor().spielen.s;
        return s.noclip && s.ort[0] > 600.0F;
    }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(pruefe("Spielen > Esc (echte Taste): beendet, die Kamera genau wie vorher", [kamera] {
        const ke::Editor& e = ke::editor();
        return !e.spielen.aktiv && e.kamera.ursprung == kamera->first && e.kamera.winkel == kamera->second &&
               konsoleText().find(ke::trK(Str::KeSpBeendet)) != std::string::npos;
    }));
    // Start an info_player_start (ueber das Menue)
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeSpSpawn)) + "###spielenspawn", "", 0, true));
    add(pause(0.2));
    add(pruefe("View > Play: start at info_player_start: an, als BhedPlayAtSpawn 1 gespeichert", [] {
        const auto it = g_app->settings.karte.find("BhedPlayAtSpawn");
        return ke::editor().spielen.startAnSpawn && it != g_app->settings.karte.end() && it->second == "1";
    }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeSpMenue)) + "###spielen", "", 0, true));
    add(pause(0.3));
    add(pruefe("View > Play in editor (Menue) mit Spawn: Start bei info_player_start (-320 320 24 + 9), Blick 90", [] {
        const ke::SpielenZustand& z = ke::editor().spielen;
        return z.aktiv && z.amSpawn && z.s.ort[0] == -320.0F && z.s.ort[1] == 320.0F && z.s.ort[2] == 33.0F && z.s.gier == 90.0F;
    }));
    add(tu("beenden, Einstellungen zurueck", [] {
        ke::spielenBeenden();
        ke::Editor& e = ke::editor();
        e.spielen.startAnSpawn = false;
        e.spielen.testEingabe = false;
        ke::einstellungenSpeichern();
    }));
    return s;
}

// --- Punkt 4: Logik-Ansicht ---------------------------------------------------------
//
// Wirkung: View > Logic view... oeffnet das Fenster mit dem Graphen (10 Knoten,
// 8 Kanten, 1 fehlendes Ziel); Klick auf den Kasten "relais" waehlt die Entity
// und zentriert die 2D-Ansicht und die Kamera darauf; Doppelklick auf ein
// fehlendes Skript: Kasten mit dem Pfad; Doppelklick auf ein Skript, das es im
// Spiel gibt: der Skript-Editor des Programms hat es offen. Mit der Testerkarte
// (BHED_KARTE_TESTER, sonst laeufe/s8garden/jeditemple_garden.map, falls da):
// jede Verknuepfung ist eine Kante oder ein fehlendes Ziel, Zeit gemessen.

std::string logikKarte(const std::string& skript) {
    const auto ent = [](const std::string& klasse, const std::string& rest) { return "{\n\"classname\" \"" + klasse + "\"\n" + rest + "}\n"; };
    std::string t = "{\n\"classname\" \"worldspawn\"\n}\n";
    t += ent("trigger_once", "\"targetname\" \"start\"\n\"target\" \"relais\"\n\"target2\" \"gibtsnicht\"\n\"origin\" \"0 0 0\"\n");
    t += ent("target_relay", "\"targetname\" \"relais\"\n\"target\" \"tuer\"\n\"origin\" \"256 128 32\"\n");
    t += ent("func_door", "\"targetname\" \"tuer\"\n\"opentarget\" \"licht\"\n\"origin\" \"128 0 0\"\n");
    t += ent("light", "\"targetname\" \"licht\"\n\"origin\" \"192 0 0\"\n");
    t += ent("NPC_spawner", "\"NPC_targetname\" \"wache\"\n\"deathscript\" \"" + skript + "\"\n\"spawnscript\" \"gibtsnicht/skript\"\n\"origin\" \"0 64 0\"\n");
    t += ent("trigger_multiple", "\"targetname\" \"aufruf\"\n\"target\" \"wache\"\n\"origin\" \"0 128 0\"\n");
    t += ent("target_relay", "\"targetname\" \"a\"\n\"target\" \"b\"\n\"origin\" \"0 192 0\"\n");
    t += ent("target_relay", "\"targetname\" \"b\"\n\"target\" \"a\"\n\"origin\" \"64 192 0\"\n");
    t += ent("target_relay", "\"targetname\" \"fuenf\"\n\"target3\" \"licht\"\n\"origin\" \"0 256 0\"\n");
    t += ent("info_null", "\"origin\" \"0 320 0\"\n");
    return t;
}

int logikKnoten(const std::string& name) {
    const auto& g = ke::editor().logik.graf;
    for (std::size_t i = 0; i < g.knoten.size(); ++i) {
        if (g.knoten[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

ImVec2 logikZeile(int knoten, int zeile) {
    const auto& l = ke::editor().logik.lagen;
    if (knoten < 0 || knoten >= static_cast<int>(l.size())) { return ImVec2{0, 0}; }
    const auto& r = l[static_cast<std::size_t>(knoten)];
    const float zoom = ke::editor().logik.mass;
    return ImVec2{r[0] + 40.0F * zoom, r[1] + (2.0F + 18.0F * static_cast<float>(zeile) + 8.0F) * zoom};
}

std::vector<Schritt> logik() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(stufe2::grundzustand("Stufe 8b/4: Grundzustand"));
    auto skript = std::make_shared<std::string>();
    add(tu("Karte mit Ausloesern, Relais, Tuer, Licht, NPC_spawner (deathscript eines Skripts des Spiels, spawnscript fehlt), Kreis", [skript] {
        ke::Editor& e = ke::editor();
        skript->clear();
        if (e.spiel) {
            for (const char* endung : {"txt", "ibi"}) {   // Endung ohne Punkt, Ergebnis relativ zu scripts/
                const std::vector<std::string> d = e.spiel->fs.dateien("scripts/", endung, 8);
                if (!d.empty()) {
                    std::string p = d.front();
                    if (p.rfind("scripts/", 0) == 0) { p = p.substr(8); }
                    *skript = p.substr(0, p.rfind('.'));
                    break;
                }
            }
        }
        diag::detail("Knopftest: Stufe 8b/4 Skript des Spiels " + *skript);
        stufe4::bobKarte(logikKarte(skript->empty() ? std::string("kein/skript") : *skript));
        e.logik.lagenMerken = true;
        e.logik.nurPruefen = false;
    }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeLgMenue)) + "###logikansicht", "", 0, true));
    add(pause(0.4));
    add(pruefe("View > Logic view...: das Fenster mit 10 Knoten, 8 Kanten, 1 ohne Ziel", [skript] {
        const ke::LogikZustand& z = ke::editor().logik;
        diag::detail("Knopftest: Stufe 8b/4 Zoom " + std::to_string(z.zoom));
        return z.offen && z.graf.knoten.size() == 10 && z.graf.kanten.size() == 8 && z.graf.fehlend == 1 && !skript->empty() &&
               stufe8::fensterSichtbar("###kartelogik");
    }));
    add(foto("karteneditor_s8b_logik"));
    add(klickAn("Kasten relais", [] { return logikZeile(logikKnoten("relais"), 0); }, 0));
    add(pause(0.3));
    add(pruefe("Logic view > Klick: relais gewaehlt, 2D-Ansicht und Kamera darauf zentriert", [] {
        const ke::Editor& e = ke::editor();
        const auto& a = e.karte.auswahl.elemente();
        const auto ent = a.empty() ? nullptr : a.front().entitaet.lock();
        const karte::Vektor3f u = e.xy[static_cast<std::size_t>(e.aktivXy)].ansicht.ursprung;
        diag::detail("Knopftest: Stufe 8b/4 2D-Mitte " + std::to_string(u[0]) + " " + std::to_string(u[1]) + ", Kamera " +
                     std::to_string(e.kamera.ursprung[0]) + " " + std::to_string(e.kamera.ursprung[1]));
        return a.size() == 1 && ent && ent->wert("targetname") == "relais" && std::fabs(u[0] - 256.0F) < 1.0F && std::fabs(u[1] - 128.0F) < 1.0F;
    }));
    add(tu("Fenster nach vorn (die Kamera ist dahinter)", [] { ImGui::SetWindowFocus("###kartelogik"); }));
    add(pause(0.2));
    add(klickAn("Doppelklick spawnscript gibtsnicht/skript", [] { return logikZeile(logikKnoten("wache"), 3); }, 0, true));
    add(pause(0.3));
    add(pruefe("Logic view > Doppelklick auf ein fehlendes Skript: Kasten mit scripts/gibtsnicht/skript", [] {
        const ke::Editor& e = ke::editor();
        return e.meldung.offen && e.meldung.titel == ke::trK(Str::KeLgTitel) && e.meldung.text.find("gibtsnicht/skript") != std::string::npos &&
               e.logik.letztesSkript.empty();
    }));
    add(tu("Kasten zu", [] { ke::editor().meldung.offen = false; }));
    add(pause(0.2));
    add(klickAn("Doppelklick deathscript", [] { return logikZeile(logikKnoten("wache"), 2); }, 0, true));
    add(pause(0.6));
    add(pruefe("Logic view > Doppelklick auf ein Skript des Spiels: im Skript-Editor des Programms offen", [skript] {
        const std::string& p = ke::editor().logik.letztesSkript;
        diag::detail("Knopftest: Stufe 8b/4 geoeffnet " + p + ", Modus " + std::to_string(g_app->leftMode));
        return !p.empty() && g_app->leftMode == 0 && reiterMitSkript(p) >= 0;
    }));
    add(tu("zurueck in den Karten-Editor", [] { g_app->leftMode = 3; }));
    add(pause(0.4));
    // Testerkarte
    add(tu("Testerkarte oeffnen, falls vorhanden", [] {
        const char* p = std::getenv("BHED_KARTE_TESTER");
        const std::string pfad = p != nullptr ? std::string(p) : std::string("Y:\\karte\\laeufe\\s8garden\\jeditemple_garden.map");
        ke::Editor& e = ke::editor();
        e.logik.lagenMerken = false;
        if (std::filesystem::exists(std::filesystem::path(pfad))) {
            (void)ke::karteOeffnen(pfad);
            const auto t0 = std::chrono::steady_clock::now();
            ke::logikZeigen();
            const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
            diag::detail("Knopftest: Stufe 8b/4 Testerkarte " + pfad + ": " + std::to_string(e.logik.graf.knoten.size()) + " Knoten, " +
                         std::to_string(e.logik.graf.kanten.size()) + " Kanten, " + std::to_string(e.logik.graf.fehlend) + " ohne Ziel, " +
                         std::to_string(ms) + " ms");
        } else {
            diag::detail("Knopftest: Stufe 8b/4 Testerkarte nicht da: " + pfad);
        }
    }));
    add(pause(0.5));
    add(pruefe("Logic view (Testerkarte, falls da): jede Verknuepfung ist eine Kante oder ein fehlendes Ziel", [] {
        const ke::Editor& e = ke::editor();
        if (e.name.find("jeditemple_garden") == std::string::npos) { return true; }
        const auto& g = e.logik.graf;
        // Gegenrechnung: je Verknuepfungswert die Zahl der Entities mit dem Namen
        std::map<std::string, int> namen;
        for (const auto& ent : e.karte.entitaeten()) {
            for (const char* n : {"targetname", "NPC_targetname", "script_targetname"}) {
                const std::string w = ent->wert(n);
                if (!w.empty()) {
                    std::string kl = w;
                    for (char& c : kl) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                    ++namen[kl];
                }
            }
        }
        std::size_t kanten = 0;
        int fehlend = 0;
        for (const auto& ent : e.karte.entitaeten()) {
            if (ent->klassenname() == "worldspawn") { continue; }
            for (const auto& [k, w] : ent->schluessel()) {
                std::string kk = k;
                for (char& c : kk) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                if (w.empty() || kk.find("target") == std::string::npos || kk == "targetname" || kk == "npc_targetname" || kk == "script_targetname") {
                    continue;
                }
                std::string kl = w;
                for (char& c : kl) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                const auto it = namen.find(kl);
                if (it == namen.end()) {
                    ++fehlend;
                    ++kanten;
                } else {
                    kanten += static_cast<std::size_t>(it->second);
                }
            }
        }
        diag::detail("Knopftest: Stufe 8b/4 Gegenrechnung " + std::to_string(kanten) + " Kanten, " + std::to_string(fehlend) + " ohne Ziel");
        return g.kanten.size() == kanten && g.fehlend == fehlend && kanten >= 300;
    }));
    add(foto("karteneditor_s8b_logik_tester"));
    add(tu("Logic view zu", [] {
        ke::Editor& e = ke::editor();
        e.logik.offen = false;
        e.logik.lagenMerken = false;
    }));
    return s;
}

// --- Punkt 5: Ansichtsmodi der Kamera ----------------------------------------------
//
// Testkarte: ein geschlossener Raum mit zwei versetzten Zwischenwaenden (die
// westliche Kammer sieht die oestliche nicht), ein playerclip-Brush (Shader des
// Spiels), eine sichtbare nonsolid-Wand (Shader des Spiels), eine func_group mit
// _lightmapscale 2, eine 4096 lange Bodenplatte; gebaut mit -meta und -vis.
// Mess-Stellen: Luxelgroessen wie q3map2 (16, 32, 33 nach der Vergroesserung),
// Kollision (Brushes farbig, nonsolid-Wand rot, Bildpunkt aendert sich), PVS
// gegen die eigene Auswertung der Bits im Vis-Lump der .bsp, Dreieckslast gegen
// die Summe der sichtbaren Flaechen; je Modus ein Foto.

std::string amKarte() { return karteTestPfad("s8bam\\am.map"); }

std::string amShader(const char* parm, bool ohneNodraw) {
    const ke::Editor& e = ke::editor();
    if (!e.spiel) { return {}; }
    for (const karte::ShaderEintrag* s : e.spiel->inventar.shader()) {
        if (s->name.rfind("textures/", 0) != 0) { continue; }
        bool hat = false;
        bool nodraw = false;
        bool anderes = false;
        for (const std::string& p : s->surfaceparms) {
            hat = hat || p == parm;
            nodraw = nodraw || p == "nodraw";
            anderes = anderes || p == "monsterclip" || p == "trigger" || p == "water" || p == "fog" || p == "lava" || p == "slime";
        }
        if (hat && !anderes && (!ohneNodraw || !nodraw)) { return s->name.substr(9); }
    }
    return {};
}

void amKarteSchreiben(const std::string& clip, const std::string& nonsolid) {
    using stufe4::kastenText;
    std::string w;
    w += kastenText(-576, -576, -16, 576, 576, 0, "test/boden");
    w += kastenText(-576, -576, 256, 576, 576, 272, "test/decke");
    w += kastenText(-576, -576, 0, -560, 576, 256, "test/wand");
    w += kastenText(560, -576, 0, 576, 576, 256, "test/wand");
    w += kastenText(-576, -576, 0, 576, -560, 256, "test/wand");
    w += kastenText(-576, 560, 0, 576, 576, 256, "test/wand");
    w += kastenText(-192, -560, 0, -160, 384, 256, "test/wand");    // Zwischenwand 1 (Luecke im Norden)
    w += kastenText(160, -384, 0, 192, 560, 256, "test/wand");      // Zwischenwand 2 (Luecke im Sueden)
    w += kastenText(-2048, 400, 0, 2048, 416, 8, "test/platte");   // 4096 lang (ragt in die Wand, schadet nicht)
    w += kastenText(-96, -96, 0, -32, -32, 128, clip.c_str());
    w += kastenText(32, 32, 0, 96, 96, 128, nonsolid.c_str());
    std::string t = stufe4::weltText(w);
    t += "{\n\"classname\" \"func_group\"\n\"_lightmapscale\" \"2\"\n" + kastenText(-480, -480, 0, -416, -416, 64, "test/dicht") + "}\n";
    t += "{\n\"classname\" \"info_player_start\"\n\"origin\" \"-400 -300 24\"\n}\n";
    std::filesystem::create_directories(std::filesystem::path(amKarte()).parent_path());
    std::ofstream f(std::filesystem::path(amKarte()), std::ios::binary | std::ios::trunc);
    f << t;
}

// Die Bits im Vis-Lump (Lump 16: numClusters, clusterBytes, Tabelle) - unabhaengig von BspGeometry
int pvsBits(const std::string& bsp, int cluster, int* clusterZahl) {
    const std::string d = dateiInhalt(bsp);
    if (d.size() < 8 + 18 * 8) { return -1; }
    std::int32_t ofs = 0;
    std::int32_t len = 0;
    std::memcpy(&ofs, d.data() + 8 + 16 * 8, 4);
    std::memcpy(&len, d.data() + 8 + 16 * 8 + 4, 4);
    if (len < 8 || cluster < 0) { return -1; }
    std::int32_t n = 0;
    std::int32_t bytes = 0;
    std::memcpy(&n, d.data() + ofs, 4);
    std::memcpy(&bytes, d.data() + ofs + 4, 4);
    *clusterZahl = n;
    int sichtbar = 0;
    for (int c = 0; c < n; ++c) {
        const auto b = static_cast<unsigned char>(d[static_cast<std::size_t>(ofs + 8 + cluster * bytes + (c >> 3))]);
        sichtbar += (b & (1U << (c & 7))) != 0U ? 1 : 0;
    }
    return sichtbar;
}

std::array<unsigned char, 4> kameraPixel() {
    const ke::KameraFenster& f = ke::editor().kamFenster;
    std::array<unsigned char, 4> p{};
    (void)kartegpu::pixel(200, f.breite / 2, f.hoehe / 2, p.data());
    return p;
}
int pixelAbstand(const std::array<unsigned char, 4>& a, const std::array<unsigned char, 4>& b) {
    return std::abs(a[0] - b[0]) + std::abs(a[1] - b[1]) + std::abs(a[2] - b[2]);
}

std::vector<Schritt> ansichten() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(stufe2::grundzustand("Stufe 8b/5: Grundzustand"));
    const char* kEintrag = "Test Ansichtsmodi";
    auto namen = std::make_shared<std::pair<std::string, std::string>>();
    add(tu("Testkarte (zwei versetzte Zwischenwaende, playerclip, nonsolid-Wand, func_group _lightmapscale 2, 4096er Platte) schreiben, oeffnen, Eintrag -meta/-vis", [kEintrag, namen] {
        karteDialogeZu();
        q3map2Bereitstellen();
        ke::bauMenueSicherstellen();
        *namen = {amShader("playerclip", false), amShader("nonsolid", true)};
        diag::detail("Knopftest: Stufe 8b/5 Shader playerclip " + namen->first + ", nonsolid " + namen->second);
        amKarteSchreiben(namen->first.empty() ? "system/clip" : namen->first, namen->second.empty() ? "test/nonsolid" : namen->second);
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(karteTestPfad("s8bam\\am.bsp")), ec);
        (void)ke::karteOeffnen(amKarte());
        ke::Editor& e = ke::editor();
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{kEintrag, {"[q3map2] -meta \"[MapFile]\"", "[q3map2] -vis -threads 1 \"[MapFile]\""}});
        e.kamera.setzeUrsprung(karte::Vektor3f(-400, -300, 64));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 45, 0));
        ++e.sichtStand;
    }));
    add(bauMenuepunkt(kEintrag));
    add(stufe8::warteSek("Bau mit VIS fertig", [] { return !ke::bauLaeuft(); }, 300));
    auto ohne = std::make_shared<std::array<unsigned char, 4>>();
    add(tu("Kamera auf die nonsolid-Wand (64 64), Bildpunkt ohne Modus merken", [ohne] {
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(karte::Vektor3f(-60, -60, 64));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 45, 0));
        ++e.sichtStand;
    }));
    add(pause(0.3));
    add(tu("Bildpunkt merken", [ohne] { *ohne = kameraPixel(); }));
    // Kollision ueber das Menue
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeAmMenue)) + "###ansichtsmodi", "", 0, true));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeAmKollision)) + "###amkollision", "", 0, true));
    add(pause(0.4));
    add(pruefe("View > Camera view modes > Collision: playerclip farbig, die sichtbare nonsolid-Wand rot, der Bildpunkt auf ihr aendert sich", [ohne, namen] {
        const ke::AnsichtsModiZustand& m = ke::editor().ansichtsmodi;
        const auto mit = kameraPixel();
        diag::detail("Knopftest: Stufe 8b/5 Kollision farbig " + std::to_string(m.kollisionFarbig) + ", ohne Kollision " + std::to_string(m.ohneKollision) +
                     ", Bildpunkt " + std::to_string(mit[0]) + "/" + std::to_string(mit[1]) + "/" + std::to_string(mit[2]) + " vorher " +
                     std::to_string((*ohne)[0]) + "/" + std::to_string((*ohne)[1]) + "/" + std::to_string((*ohne)[2]));
        return m.kollision && m.kollisionFarbig >= 1 && m.ohneKollision >= 1 && !namen->first.empty() && !namen->second.empty() &&
               pixelAbstand(mit, *ohne) > 30 && mit[0] > mit[1];
    }));
    add(foto("karteneditor_s8b_modus_kollision"));
    add(tu("Kollision aus, Lightmap-Dichte an; Kamera vor die func_group", [] {
        ke::Editor& e = ke::editor();
        e.ansichtsmodi.kollision = false;
        e.ansichtsmodi.dichte = true;
        e.kamera.setzeUrsprung(karte::Vektor3f(-450, -300, 40));
        e.kamera.setzeWinkel(karte::Vektor3f(-20, 270, 0));
        ++e.sichtStand;
    }));
    add(pause(0.4));
    add(pruefe("Camera view modes > Lightmap density: Luxel wie q3map2 - Boden 16, func_group mit _lightmapscale 2 = 32, 4096er Platte vergroessert auf 33", [] {
        const auto& l = ke::editor().ansichtsmodi.luxelJeShader;
        const auto w = [&l](const char* n) {
            const auto it = l.find(std::string("textures/") + n);
            return it != l.end() ? it->second : -1;
        };
        diag::detail("Knopftest: Stufe 8b/5 Luxel boden " + std::to_string(w("test/boden")) + ", dicht " + std::to_string(w("test/dicht")) + ", platte " +
                     std::to_string(w("test/platte")) + ", Flaechen " + std::to_string(ke::editor().ansichtsmodi.dichteFlaechen));
        return w("test/boden") == 16 && w("test/dicht") == 32 && w("test/platte") == 33 && ke::editor().ansichtsmodi.dichteFlaechen > 20;
    }));
    add(foto("karteneditor_s8b_modus_dichte"));
    // Sichtbarkeit
    add(tu("Dichte aus, Sichtbarkeit an; Kamera in der westlichen Kammer", [] {
        ke::Editor& e = ke::editor();
        e.ansichtsmodi.dichte = false;
        e.ansichtsmodi.sichtbarkeit = true;
        e.kamera.setzeUrsprung(karte::Vektor3f(-400, -300, 64));
        e.kamera.setzeWinkel(karte::Vektor3f(-10, 0, 0));
        ++e.sichtStand;
    }));
    add(pause(0.5));
    add(pruefe("Camera view modes > Visibility (PVS): aus der .bsp; die Zahl der sichtbaren Cluster gleich den Bits im Vis-Lump, weniger als alle", [] {
        const ke::AnsichtsModiZustand& m = ke::editor().ansichtsmodi;
        int n = 0;
        const int bits = pvsBits(karteTestPfad("s8bam\\am.bsp"), m.kameraCluster, &n);
        diag::detail("Knopftest: Stufe 8b/5 PVS Cluster " + std::to_string(m.kameraCluster) + " sieht " + std::to_string(m.sichtbareCluster) + " von " +
                     std::to_string(m.clusterZahl) + " (Bits " + std::to_string(bits) + " von " + std::to_string(n) + "), Dreiecke " +
                     std::to_string(m.sichtbareDreiecke));
        return m.bspDa && m.kameraCluster >= 0 && m.sichtbareCluster == bits && n == m.clusterZahl && bits < n && m.sichtbareDreiecke > 0;
    }));
    add(foto("karteneditor_s8b_modus_sicht"));
    // Dreieckslast
    add(tu("Sichtbarkeit aus, Dreieckslast an", [] {
        ke::Editor& e = ke::editor();
        e.ansichtsmodi.sichtbarkeit = false;
        e.ansichtsmodi.last = true;
        ++e.sichtStand;
    }));
    add(pause(0.5));
    add(pruefe("Camera view modes > Triangle load: Last am Ort gleich den Dreiecken der sichtbaren Flaechen, hoechste Last >= diese", [] {
        const ke::AnsichtsModiZustand& m = ke::editor().ansichtsmodi;
        // Gegenrechnung mit der .bsp
        BspGeometry g;
        if (!readBspGeometry(dateiInhalt(karteTestPfad("s8bam\\am.bsp")), g)) { return false; }
        const BspMesh netz = buildModelMesh(g, 0, 6);
        std::uint64_t summe = 0;
        for (const BspMesh::Run& r : netz.runs) {
            if (g.surfaceVisible(r.surface, m.kameraCluster)) { summe += r.numIndexes / 3; }
        }
        diag::detail("Knopftest: Stufe 8b/5 Last " + std::to_string(m.kameraLast) + " (Gegenrechnung " + std::to_string(summe) + "), hoechste " +
                     std::to_string(m.lastMax));
        return m.last && m.kameraLast == summe && m.lastMax >= m.kameraLast && summe > 0;
    }));
    add(foto("karteneditor_s8b_modus_last"));
    add(tu("Modi aus, Eintrag weg", [kEintrag] {
        ke::Editor& e = ke::editor();
        e.ansichtsmodi = ke::AnsichtsModiZustand{};
        auto& v = e.bauProjekt.eintraege;
        v.erase(std::remove_if(v.begin(), v.end(), [kEintrag](const karte::BauEintrag& b) { return b.name == kEintrag; }), v.end());
        ++e.sichtStand;
    }));
    return s;
}

// --- Punkt 6: Spiegel-Modus ----------------------------------------------------------
//
// Wirkung: View > Mirror mode > Mirror plane X schaltet ein (Meldung, magenta
// Linie in 2D); ein Pinsel wird in der XY-Ansicht mit der Maus gezogen - schon
// waehrend des Ziehens steht das Gegenstueck gespiegelt da, nach dem Loslassen
// genau das Spiegelbild (wie Flip); ein Rueckgaengig-Schritt; Strg+Z stellt die
// Karte byte-gleich her (Gegenstueck weg), Strg+Y wieder den Stand danach;
// eine Punktentitaet gezogen: origin und angle gespiegelt; Off schaltet aus.
std::string spKarte() { return karteTestPfad("s8bsp\\spiegel.map"); }

bool spGespiegelt(const std::shared_ptr<karte::Primitiv>& a, const std::shared_ptr<karte::Primitiv>& b) {
    const ke::Editor& e = ke::editor();
    if (!a || !b) { return false; }
    return karte::primitivText(*b, e.karte.kontext) == karte::primitivText(*karte::primitivGespiegelt(*a, 0, 0, e.karte.kontext), e.karte.kontext);
}

// Ziehen in der XY-Ansicht mit M1; im Bild "mitte" laeuft "pruef" (live).
Schritt spZiehen(const std::string& was, ImVec2 von, ImVec2 nach, std::function<void()> pruef) {
    return Schritt{was, [=](int b) {
                       ImGuiIO& io = ImGui::GetIO();
                       const ImVec2 a = stufe2::weltXy(von.x, von.y);
                       const ImVec2 z = stufe2::weltXy(nach.x, nach.y);
                       if (b == 0) {
                           setzeMaus(a);
                           return false;
                       }
                       if (b == 1) {
                           io.AddMouseButtonEvent(0, true);
                           return false;
                       }
                       if (b >= 2 && b <= 11) {
                           const float t = static_cast<float>(b - 1) / 10.0F;
                           setzeMaus(ImVec2{a.x + (z.x - a.x) * t, a.y + (z.y - a.y) * t});
                           return false;
                       }
                       if (b == 12) {
                           pruef();   // Maus noch unten, Befehl offen
                           io.AddMouseButtonEvent(0, false);
                           return false;
                       }
                       return b >= 15;
                   }};
}

std::vector<Schritt> spiegel() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(stufe2::grundzustand("Stufe 8b/6: Grundzustand"));
    add(tu("Testkarte (Pinsel 32..96 x 0..64, info_player_start 100 50 8 angle 30) schreiben und oeffnen, XY-Ansicht auf den Ursprung", [] {
        karteDialogeZu();
        std::string t = weltText(kastenText(32, 0, 0, 96, 64, 64, "test/a"));
        t += "{\n\"classname\" \"info_player_start\"\n\"origin\" \"100 50 8\"\n\"angle\" \"30\"\n}\n";
        std::filesystem::create_directories(std::filesystem::path(spKarte()).parent_path());
        {
            std::ofstream f(std::filesystem::path(spKarte()), std::ios::binary | std::ios::trunc);
            f << t;
        }
        (void)ke::karteOeffnen(spKarte());
        ke::Editor& e = ke::editor();
        e.spiegel = karte::SpiegelModus{};
        e.xy[0].ansicht.setzeMassstab(1);
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        ++e.sichtStand;
    }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeSgMenue)) + "###spiegel", "", 0, true));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeSgX)) + "###spiegelx", "", 0, true));
    add(pause(0.3));
    add(pruefe("View > Mirror mode > Mirror plane X: an, Ebene X = 0, Meldung in der Konsole, magenta Linie in den 2D-Ansichten", [] {
        const ke::Editor& e = ke::editor();
        char z[160];
        std::snprintf(z, sizeof(z), ke::trK(Str::KeSgMeldung), 'X', 0.0);
        bool linie = false;
        for (const ke::PluginLinien& l : ke::pluginLinien2d()) {
            for (const karte::Linie& x : l.linien) { linie = linie || (x.a[0] == 0 && x.b[0] == 0 && x.a[1] != x.b[1]); }
        }
        return e.spiegel.an && e.spiegel.achse == 0 && e.spiegel.ebene == 0 && konsoleText().find(z) != std::string::npos && linie;
    }));
    auto vorher = std::make_shared<std::string>();
    auto nachher = std::make_shared<std::string>();
    auto live = std::make_shared<int>(0);
    auto stufen = std::make_shared<std::size_t>(0);
    add(tu("Pinsel waehlen, Kartentext merken", [vorher, stufen] {
        weltWaehlen({0});
        *vorher = kartenText();
        *stufen = ke::editor().karte.undo.undoAnzahl();
    }));
    add(pause(0.2));
    add(spZiehen("Pinsel in XY von (64 32) nach (128 64) ziehen", ImVec2{64, 32}, ImVec2{128, 64}, [live] {
        const ke::Editor& e = ke::editor();
        const auto& k = e.karte.welt()->kinder();
        const auto b = e.spiegel.gegenstueck(k[0].get());
        *live = !e.karte.undo.befehlOffen() ? -1 : (spGespiegelt(k[0], b) ? 1 : 0);
    }));
    add(pause(0.3));
    add(pruefe("Ziehen: schon waehrend des Ziehens (Befehl offen) ist das Gegenstueck gespiegelt; danach genau das Spiegelbild wie Flip, ein Rueckgaengig-Schritt", [live, stufen, nachher] {
        const ke::Editor& e = ke::editor();
        const auto& k = e.karte.welt()->kinder();
        const auto b = k.size() == 2 ? e.spiegel.gegenstueck(k[0].get()) : nullptr;
        const auto& g = static_cast<const karte::Pinsel&>(*k[0]).grenzen(e.karte.kontext);
        diag::detail("Knopftest: Stufe 8b/6 live " + std::to_string(*live) + ", Pinsel " + std::to_string(k.size()) + ", Mitte " +
                     std::to_string(g.origin[0]) + " " + std::to_string(g.origin[1]) + ", Schritte " +
                     std::to_string(e.karte.undo.undoAnzahl() - *stufen) + " (" + e.karte.undo.undoName() + ")");
        *nachher = kartenText();
        return *live == 1 && b != nullptr && b == k[1] && spGespiegelt(k[0], b) && g.origin[0] > 64 && e.karte.undo.undoAnzahl() == *stufen + 1;
    }));
    add(foto("karteneditor_s8b_spiegel"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("Mirror mode: Strg+Z stellt beide Seiten byte-gleich her (das Gegenstueck ist weg)", [vorher] {
        return kartenText() == *vorher && ke::editor().karte.welt()->kinder().size() == 1;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Y, "Strg+Y"));
    add(pause(0.3));
    add(pruefe("Mirror mode: Strg+Y stellt den Stand nach dem Ziehen byte-gleich her", [nachher] { return kartenText() == *nachher; }));
    add(tu("info_player_start waehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        for (const auto& ent : e.karte.entitaeten()) {
            if (ent->klassenname() == "info_player_start") { e.karte.auswahl.waehlen(ent); }
        }
        ke::auswahlGeaendert();
    }));
    add(pause(0.2));
    add(stufe2::ziehenXy("info_player_start in XY um 0 32 ziehen", 0, 0, ImVec2{100, 50}, ImVec2{100, 82}));
    add(pause(0.3));
    add(pruefe("Mirror mode: Punktentitaet gezogen - Gegenstueck mit gespiegeltem origin (-x) und angle 30 -> 150", [] {
        const ke::Editor& e = ke::editor();
        std::shared_ptr<karte::Entitaet> a;
        for (const auto& ent : e.karte.entitaeten()) {
            if (ent->klassenname() == "info_player_start" && e.karte.auswahl.gewaehlt(ent.get())) { a = ent; }
        }
        const auto b = a ? e.spiegel.gegenstueck(a.get()) : nullptr;
        diag::detail("Knopftest: Stufe 8b/6 Entity " + (a ? a->wert("origin") + " / " + a->wert("angle") : std::string("-")) + " -> " +
                     (b ? b->wert("origin") + " / " + b->wert("angle") : std::string("-")));
        if (!a || !b) { return false; }
        double va[3];
        double vb[3];
        return std::sscanf(a->wert("origin").c_str(), "%lf %lf %lf", &va[0], &va[1], &va[2]) == 3 &&
               std::sscanf(b->wert("origin").c_str(), "%lf %lf %lf", &vb[0], &vb[1], &vb[2]) == 3 && vb[0] == -va[0] && vb[1] == va[1] && vb[2] == va[2] &&
               va[1] > 50 && b->wert("angle") == "150";
    }));
    // Achse Y, Ebene an der Auswahl, Ebene durch den Ursprung, Achse Z
    const auto menuePunkt = [&add](Str text, const char* id) {
        add(klick(ke::trK(Str::KeLView), "###main"));
        add(pause(0.2));
        add(klick(std::string(ke::trK(Str::KeSgMenue)) + "###spiegel", "", 0, true));
        add(pause(0.2));
        add(klick(std::string(ke::trK(text)) + id, "", 0, true));
        add(pause(0.3));
    };
    add(tu("Pinsel waehlen (Mitte y 48 nach dem Ziehen)", [] { weltWaehlen({0}); }));
    menuePunkt(Str::KeSgY, "###spiegely");
    add(pruefe("View > Mirror mode > Mirror plane Y: Achse Y, die Paare der X-Ebene vergessen", [] {
        const ke::Editor& e = ke::editor();
        return e.spiegel.an && e.spiegel.achse == 1 && e.spiegel.paarAnzahl() == 0;
    }));
    menuePunkt(Str::KeSgMitte, "###spiegelmitte");
    add(pruefe("View > Mirror mode > Plane through selection center (on grid): Ebene Y = Mitte der Auswahl auf dem Raster", [] {
        const ke::Editor& e = ke::editor();
        const karte::Kasten b = karte::auswahlGrenzenPivot(e.karte);
        const double r = e.raster.groesse();
        diag::detail("Knopftest: Stufe 8b/6 Ebene " + std::to_string(e.spiegel.ebene) + " (Mitte " + std::to_string(b.origin[1]) + ", Raster " + std::to_string(r) + ")");
        return e.spiegel.ebene == std::round(b.origin[1] / r) * r && e.spiegel.ebene != 0;
    }));
    menuePunkt(Str::KeSgNull, "###spiegelnull");
    add(pruefe("View > Mirror mode > Plane through origin: Ebene = 0", [] { return ke::editor().spiegel.ebene == 0; }));
    menuePunkt(Str::KeSgZ, "###spiegelz");
    add(pruefe("View > Mirror mode > Mirror plane Z: Achse Z", [] { return ke::editor().spiegel.an && ke::editor().spiegel.achse == 2; }));
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeSgMenue)) + "###spiegel", "", 0, true));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeSgAus)) + "###spiegelaus", "", 0, true));
    add(pause(0.3));
    add(pruefe("View > Mirror mode > Off: aus, keine Paare, keine Linie", [] {
        const ke::Editor& e = ke::editor();
        bool linie = false;
        for (const ke::PluginLinien& l : ke::pluginLinien2d()) { linie = linie || !l.linien.empty(); }
        return !e.spiegel.an && e.spiegel.paarAnzahl() == 0 && !linie;
    }));
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
    if (teil("spielen")) { dazu(spielen()); }
    if (teil("logik")) { dazu(logik()); }
    if (teil("ansichten")) { dazu(ansichten()); }
    if (teil("spiegel")) { dazu(spiegel()); }
    return s;
}

}  // namespace stufe8b
