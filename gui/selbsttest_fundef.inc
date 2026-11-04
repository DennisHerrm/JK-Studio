// selbsttest_fundef.inc - rc609 F: Funde der Codepruefung in der Bedienung (BHED_EDITORTEST=fundef)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor).
//
//   F10  Kartenwechsel auf eine .bsp, deren Entities lesbar sind, deren Geometrie aber nicht (Vertex-Lump
//        hinter dem Dateiende): Gras, Texturen und mapBereit der vorigen Karte (yavin1b, surfaceSprites)
//        duerfen nicht stehen bleiben - wie clearMap.
//   F/8  Eine IBSP (Quake 3, Fassung 46) oeffnen: klare Abweisung (wrong version number wie
//        RE_LoadWorldMap), die geladene Karte bleibt.
namespace fundef {

struct Zustand {
    std::string bsp;   // yavin1b wie gelesen
    bool bereit = false;
    // Gleich nach dem Wechsel gemessen - im naechsten Bild baut prepareScene die Modelle der neuen
    // Entities auf und haengt deren Bilder an textures an (das gehoert zur neuen Karte).
    std::size_t gras = 0;
    std::size_t grasNetz = 0;
    std::size_t texturen = 0;
    bool mapBereit = true;
};

// Ein Lumpeintrag (Versatz) im Kopf: Kennung 4, Fassung 4, dann je Lump Versatz und Laenge.
void setzeInt(std::string& b, std::size_t at, std::int32_t v) {
    for (std::size_t k = 0; k < 4; ++k) { b[at + k] = static_cast<char>((static_cast<std::uint32_t>(v) >> (8 * k)) & 0xFFU); }
}

std::size_t grasZahl() {
    std::size_t n = 0;
    for (const auto& v : g_app->grasStufen) { n += v.size(); }
    return n;
}

}  // namespace fundef

std::vector<Schritt> fundefSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<fundef::Zustand>();
    using namespace fundef;

    add(tu("F10: yavin1b laden (Gras)", [z] {
        g_app->leftMode = 1;
        z->bereit = readFromArchives("maps/yavin1b.bsp", z->bsp) && loadMapFromArchive("maps/yavin1b.bsp");
        if (!z->bereit) { diag::detail("Knopftest: F10 - maps/yavin1b.bsp nicht in den Spielordnern"); }
    }));
    add(warteBis("F10: Karte bereit", [z] { return !z->bereit || (g_app->mapBereit && !g_app->mapDirty); }, 900));
    add(pruefe("F10: vorher Gras, Texturen und mapBereit", [z] {
        diag::detail("Knopftest: F10 vorher - Gras " + std::to_string(grasZahl()) + ", Grasnetz " +
                     std::to_string(g_app->grasNetz.indexes.size()) + ", Texturen " +
                     std::to_string(g_app->textures.byShader.size()) + ", mapBereit " + std::to_string(g_app->mapBereit ? 1 : 0));
        return z->bereit && grasZahl() > 0U && !g_app->textures.byShader.empty() && g_app->mapBereit;
    }));
    add(tu("F10: Kopie mit Vertex-Lump hinter dem Dateiende ueber loadMapFromBytes", [z] {
        std::string b = z->bsp;
        setzeInt(b, 8 + 10 * 8, static_cast<std::int32_t>(b.size()));   // Lump 10 (Ecken): Versatz = Dateiende
        setzeInt(b, 12 + 10 * 8, 80);
        loadMapFromBytes(b, "maps/f10_kaputt.bsp");
        z->gras = grasZahl();
        z->grasNetz = g_app->grasNetz.indexes.size();
        z->texturen = g_app->textures.byShader.size();
        z->mapBereit = g_app->mapBereit;
    }));
    add(pruefe("F10: Entities der neuen Karte, Gras/Grasnetz/Texturen leer, mapBereit aus (wie clearMap)", [z] {
        diag::detail("Knopftest: F10 nachher - Karte " + g_app->map.path + " (" + std::to_string(g_app->map.entities.size()) +
                     " Entities), Geometrie leer " + std::to_string(g_app->geo.empty() ? 1 : 0) + ", Gras " +
                     std::to_string(z->gras) + ", Grasnetz " + std::to_string(z->grasNetz) + ", Texturen " +
                     std::to_string(z->texturen) + ", mapBereit " + std::to_string(z->mapBereit ? 1 : 0) +
                     " (gleich nach dem Wechsel)");
        return g_app->map.path == "maps/f10_kaputt.bsp" && !g_app->map.entities.empty() && g_app->geo.empty() &&
               z->gras == 0U && z->grasNetz == 0U && z->texturen == 0U && !z->mapBereit;
    }));
    add(tu("F/8: wieder yavin1b laden", [z] {
        if (z->bereit) { (void)loadMapFromArchive("maps/yavin1b.bsp"); }
    }));
    add(warteBis("F/8: Karte bereit", [z] { return !z->bereit || (g_app->mapBereit && !g_app->mapDirty); }, 900));
    add(tu("F/8: dieselbe Karte als IBSP 46 (Quake 3) ueber loadMapFromBytes", [z] {
        std::string b = z->bsp;
        b.replace(0, 4, "IBSP");
        setzeInt(b, 4, 46);
        loadMapFromBytes(b, "maps/f8_ibsp.bsp");
    }));
    add(pruefe("F/8: IBSP abgewiesen - die geladene Karte bleibt, keine falsche Geometrie", [z] {
        diag::detail("Knopftest: F/8 - Karte " + g_app->map.path + ", Ecken " + std::to_string(g_app->geo.verts.size()) +
                     ", mapBereit " + std::to_string(g_app->mapBereit ? 1 : 0));
        return z->bereit && g_app->map.path == "maps/yavin1b.bsp" && g_app->mapBereit && !g_app->geo.verts.empty();
    }));
    return s;
}
