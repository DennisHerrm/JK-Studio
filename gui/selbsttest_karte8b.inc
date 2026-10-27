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

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    const char* nur = std::getenv("BHED_STUFE8B_NUR");
    const auto teil = [nur](const char* t) { return nur == nullptr || std::strcmp(nur, t) == 0; };
    const auto dazu = [&s](std::vector<Schritt> x) {
        for (Schritt& y : x) { s.push_back(std::move(y)); }
    };
    if (teil("terrain")) { dazu(terrain()); }
    return s;
}

}  // namespace stufe8b
