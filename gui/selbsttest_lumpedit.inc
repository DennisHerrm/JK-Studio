// selbsttest_lumpedit.inc - rc609 J: Shader, Nebel und Flaechen fertiger Karten (BHED_EDITORTEST=lumpedit)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor).
//
// maps/academy1.bsp aus den Spielordnern in einer eigenen .pk3 im Laufordner (<Lauf>\lumpedit\mod\zz_lumps.pk3):
//   A  Fenster "Shader und Flaechen" auf, Klick in der Vorschau auf eine Flaeche mit Lightmap: gewaehlt ist
//      die Flaeche, die der Strahl durch diesen Bildpunkt trifft
//   B  Klicken -> Text: Shader tauschen (passend zum Licht) - der Text hat die neue Zeile, die Vorschau
//      (geo, aus den neuen Bytes gelesen) zeigt den neuen Shader an der Flaeche
//   C  Text -> Klicken: Klick in die Zeile hebt den Shader hervor, Name im Text geaendert, Strg+Enter -
//      Lumps und Vorschau zeigen den neuen Namen
//   D  Rueckgaengig / Wiederholen (je ein Schritt, Vorschau mit)
//   E  Warnung bei unpassendem Shader (Lightmap ohne $lightmap-Stufe; Flaeche ohne Lightmap mit $lightmap)
//   F  Brush-Seiten: die Zahlen im Fenster wie unabhaengig aus dem Brush- und Seiten-Lump gezaehlt; ein
//      Inhalts-Flag aendert den Inhalt der Brushes in der Kollision (CMod_LoadBrushes)
//   G  Speichern Weg 1 (in der .pk3, .pk3.bak) und Weg 2 (lose im Mod-Ordner)
//   H  Karten-Editor: die lose Datei aus G mit File > Open .bsp..., das Fenster dort; eine Flaeche ueber den
//      Strahl der Kamera (lumpEditKarteKlick), Shader tauschen - das Spielbild bekommt die neuen Bytes;
//      Speichern schreibt die lose Datei (mit .bak)
namespace lumpedit {

struct Zustand {
    std::string wurzel;
    std::string mod;
    std::string pk3;
    std::string bsp;           // academy1 wie gelesen
    std::string pk3Vorher;
    bool bereit = false;
    int flaeche = -1;          // Ziel des Klicks (Lightmap >= 0, eben)
    int erwartet = -1;         // was der Strahl durch den Klickpunkt trifft
    ImVec2 punkt{-1.0F, -1.0F};
    std::string passend;       // Shader mit $lightmap-Stufe
    std::string passend2;      // ein zweiter (Text)
    std::string ohneStufe;     // Shader mit Skript ohne $lightmap-Stufe
    int neuNr = -1;            // Nummer des angehaengten Shaders
    std::size_t undo0 = 0;
    std::string eintragNachWeg1;
    std::string neuH;          // H: ein Shader, den die Karte noch nicht hat
    std::string loseVorH;      // H: die lose Datei vor dem Speichern
};

std::string lies(const std::string& p) {
    std::ifstream f(::bhed::utf8Pfad(p), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
std::string eintrag(const std::string& pk3, const std::string& name) {
    Pk3 a;
    std::string t;
    if (!readPk3Directory(pk3, a)) { return t; }
    const Pk3Entry* e = a.find(name);
    if (e != nullptr) { (void)readPk3File(a, *e, t); }
    return t;
}
// Unabhaengig gezaehlt: Brushes und Brush-Seiten mit shader im Roh-Lump (dbrush_t 12, dbrushside_t RBSP 12).
void rohZaehlen(const std::string& bsp, int shader, int& brushes, int& seiten) {
    brushes = seiten = 0;
    karte::BspDatei d;
    if (!karte::bspLesen(bsp, d) || d.lumps.size() < 10) { return; }
    const auto i32 = [](const std::string& s, std::size_t o) {
        std::uint32_t v = 0;
        for (std::size_t k = 0; k < 4; ++k) { v |= static_cast<std::uint32_t>(static_cast<unsigned char>(s[o + k])) << (8 * k); }
        return static_cast<int>(v);
    };
    const std::size_t sg = d.ident == "RBSP" ? 12 : 8;
    for (std::size_t o = 0; o + 12 <= d.lumps[8].size(); o += 12) { brushes += i32(d.lumps[8], o + 8) == shader ? 1 : 0; }
    for (std::size_t o = 0; o + sg <= d.lumps[9].size(); o += sg) { seiten += i32(d.lumps[9], o + 4) == shader ? 1 : 0; }
}
// Kamera vor die Flaeche (entlang ihrer Ebene), die Mitte ihrer Ecken in die Bildmitte.
bool kameraVorFlaeche(int i, float mitte[3]) {
    const BspGeometry& g = g_app->geo;
    if (i < 0 || static_cast<std::size_t>(i) >= g.surfaces.size()) { return false; }
    const BspSurface& s = g.surfaces[static_cast<std::size_t>(i)];
    for (int k = 0; k < 3; ++k) { mitte[k] = (s.mins[k] + s.maxs[k]) * 0.5F; }
    float n[3] = {s.ebenenNormale[0], s.ebenenNormale[1], s.ebenenNormale[2]};
    for (int k = 0; k < 3; ++k) { g_app->cam.pos[k] = mitte[k] + n[k] * 96.0F + (k == 2 ? 8.0F : 0.0F); }
    aimAngles(g_app->cam.pos, mitte, g_app->cam.angles);
    g_app->throughCamera = false;
    g_app->cameraSelected = false;
    g_app->mapDirty = true;
    return true;
}
int shaderNr(const karte::BspLumps& l, const std::string& name) {
    for (std::size_t i = 0; i < l.shader.size(); ++i) {
        if (l.shader[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

}  // namespace lumpedit

std::vector<Schritt> lumpeditSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<lumpedit::Zustand>();
    using namespace lumpedit;

    add(tu("Lumps: academy1 in zz_lumps.pk3 im Laufordner, aus der .pk3 oeffnen", [z] {
        std::error_code ec;
        z->wurzel = (std::filesystem::current_path() / "lumpedit").string();
        std::filesystem::remove_all(::bhed::utf8Pfad(z->wurzel), ec);
        z->mod = z->wurzel + "\\mod";
        std::filesystem::create_directories(::bhed::utf8Pfad(z->mod), ec);
        z->pk3 = z->mod + "\\zz_lumps.pk3";
        g_app->settings.pk3SpeicherWeg = 0;
        if (!readFromArchives("maps/academy1.bsp", z->bsp)) {
            diag::detail("Knopftest: Lumps - maps/academy1.bsp nicht in den Spielordnern");
            return;
        }
        z->bereit = writePk3(z->pk3, {{"maps/lumpedit.bsp", z->bsp}});
        g_app->leftMode = 1;
        if (z->bereit) { (void)karteAusArchivOeffnen(z->pk3, "maps/lumpedit.bsp"); }
    }));
    add(warteBis("Lumps: Karte geladen", [z] { return !z->bereit || (g_app->map.path == "maps/lumpedit.bsp" && !g_app->geo.empty()); }, 900));
    add(pruefe("Lumps: Karte aus der .pk3 geladen", [z] { return z->bereit && g_app->map.path == "maps/lumpedit.bsp" && !g_app->geo.empty(); }));
    add(tu("Lumps A: Fenster > Shader und Flaechen (dock::ausblenden wie der Menuepunkt)", [] {
        dock::ausblenden("mission.lumps", false);
        lumpEditWaehlenSetzen(true);
        lumpEditReiter(0);
    }));
    add(warteBis("Lumps A: Fenster gezeichnet", [] { return lumpEditLumps() != nullptr && !lumpEditPuffer().empty() && lumpEditKnopfOrt("waehlen").x > 0.0F; }, 300));
    add(tu("Lumps A: Fenster rechts an den Rand (die Kartenmitte bleibt frei)", [] {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const std::string name = dock::fensterName("mission.lumps");
        ImGui::SetWindowPos(name.c_str(), ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.70F, vp->WorkPos.y + vp->WorkSize.y * 0.05F});
        ImGui::SetWindowSize(name.c_str(), ImVec2{vp->WorkSize.x * 0.29F, vp->WorkSize.y * 0.88F});
    }));
    add(pause(0.3));
    add(tu("Lumps A: Flaeche mit Lightmap waehlen, Shader fuer die Proben aus den Skripten des Spiels", [z] {
        const karte::BspLumps* l = lumpEditLumps();
        const BspGeometry& g = g_app->geo;
        float best = 0.0F;
        for (std::size_t i = 0; l != nullptr && i < l->flaechen.size() && i < g.surfaces.size(); ++i) {
            const BspSurface& f = g.surfaces[i];
            if (l->flaechen[i].lightmap < 0 || f.type != BspSurface::Type::Planar) { continue; }
            const float dx = f.maxs[0] - f.mins[0];
            const float dy = f.maxs[1] - f.mins[1];
            const float dz = f.maxs[2] - f.mins[2];
            const float a = std::max({dx * dy, dx * dz, dy * dz});
            if (a > best && a < 512.0F * 512.0F) {
                best = a;
                z->flaeche = static_cast<int>(i);
            }
        }
        for (const auto& [name, info] : g_app->shaderMap) {
            if (name.rfind("textures/", 0) != 0 || info.image.empty()) { continue; }
            if (info.lightmapStufe && z->passend.empty()) {
                z->passend = name;
            } else if (info.lightmapStufe && z->passend2.empty()) {
                z->passend2 = name;
            } else if (!info.lightmapStufe && info.imageStages > 0 && z->ohneStufe.empty()) {
                z->ohneStufe = name;
            }
        }
        float mitte[3];
        (void)kameraVorFlaeche(z->flaeche, mitte);
        diag::detail("Knopftest: Lumps A - Flaeche " + std::to_string(z->flaeche) + ", passend " + z->passend + " / " + z->passend2 + ", ohne Stufe " +
                     z->ohneStufe);
    }));
    add(pause(0.6));
    add(tu("Lumps A: Klickpunkt (Mitte der Flaeche auf dem Schirm) und was der Strahl dort trifft", [z] {
        float mitte[3];
        (void)kameraVorFlaeche(z->flaeche, mitte);
        ImVec2 p;
        if (kartenPunktAufSchirm(mitte, p)) {
            z->punkt = p;
            TraceTreffer t;
            if (kartenStrahlUnterMaus(p.x, p.y, t)) { z->erwartet = t.surface; }
        }
        diag::detail("Knopftest: Lumps A - Klickpunkt " + std::to_string(static_cast<int>(z->punkt.x)) + "," + std::to_string(static_cast<int>(z->punkt.y)) +
                     ", Strahl trifft Flaeche " + std::to_string(z->erwartet));
    }));
    add(klickAn("Lumps A: auf die Flaeche in der Vorschau", [z] { return z->punkt; }));
    add(pause(0.3));
    add(pruefe("Lumps A: Klick waehlt die Flaeche unter der Maus (Strahl wie im Spiel), keine Entity", [z] {
        diag::detail("Knopftest: Lumps A - gewaehlt " + std::to_string(lumpEditFlaeche()) + ", erwartet " + std::to_string(z->erwartet) + ", Ziel " +
                     std::to_string(z->flaeche));
        return z->erwartet >= 0 && lumpEditFlaeche() == z->erwartet && g_app->pickedEntity < 0;
    }));
    add(tu("Lumps A: fuer die weiteren Proben die gezielte Flaeche (Lightmap, eben)", [z] { lumpEditFlaecheSetzen(z->flaeche); }));
    add(pause(0.3));
    add(foto("le_flaeche"));

    // --- B: Klicken -> Text und Vorschau -------------------------------------------------------
    add(tu("Lumps B: Shader der Flaeche tauschen (passend zum Licht), nur diese Flaeche", [z] {
        z->undo0 = lumpEditRueckgaengigZahl();
        (void)lumpEditTauschen(z->passend, false);
    }));
    add(warteBis("Lumps B: Vorschau neu gelesen", [z] { return lumpEditLumps() != nullptr && !g_app->geo.empty() && g_app->geo.shaders.size() == lumpEditLumps()->shader.size(); }, 600));
    add(pruefe("Lumps B: Text und Vorschau zeigen den neuen Shader an der Flaeche", [z] {
        const karte::BspLumps* l = lumpEditLumps();
        if (l == nullptr || z->flaeche < 0) { return false; }
        z->neuNr = shaderNr(*l, z->passend);
        const std::string zeile = std::to_string(z->neuNr) + " \"" + z->passend + "\"";
        const bool text = lumpEditText().find(zeile) != std::string::npos && lumpEditPuffer().find(zeile) != std::string::npos;
        const bool lump = l->flaechen[static_cast<std::size_t>(z->flaeche)].shaderNum == z->neuNr;
        const BspGeometry& g = g_app->geo;
        const bool vorschau = static_cast<std::size_t>(z->flaeche) < g.surfaces.size() && g.surfaces[static_cast<std::size_t>(z->flaeche)].shader == z->neuNr &&
                              g.shaders[static_cast<std::size_t>(z->neuNr)].name == z->passend;
        karte::BspLumps w;
        const bool datei = karte::bspLumpsLesen(lumpEditDateiBytes(), w) && w.flaechen[static_cast<std::size_t>(z->flaeche)].shaderNum == z->neuNr;
        diag::detail("Knopftest: Lumps B - Shader " + std::to_string(z->neuNr) + " (" + z->passend + "), Text " + (text ? "ja" : "nein") + ", Lump " +
                     (lump ? "ja" : "nein") + ", Vorschau " + (vorschau ? "ja" : "nein") + ", Datei " + (datei ? "ja" : "nein") + ", Rueckgaengig " +
                     std::to_string(z->undo0) + " -> " + std::to_string(lumpEditRueckgaengigZahl()));
        return z->neuNr >= 0 && text && lump && vorschau && datei && lumpEditRueckgaengigZahl() == z->undo0 + 1 && lumpEditWarnung().empty();
    }));

    // --- C: Text -> Klicken und Vorschau --------------------------------------------------------
    add(tu("Lumps C: Reiter Lumps als Text, keine Flaeche gewaehlt (nichts hervorgehoben)", [] {
        lumpEditFlaecheSetzen(-1);
        lumpEditReiter(1);
    }));
    add(pause(0.4));
    add(klickAn("Lumps C: in die Zeile des neuen Shaders", [z] { return lumpEditZeilenOrt(karte::lumpTextZeile(lumpEditText(), false, z->neuNr) - 1); }));
    add(pause(0.3));
    add(pruefe("Lumps C: Klick in die Zeile hebt den Shader hervor (Flaechen in 3D)", [z] {
        diag::detail("Knopftest: Lumps C - hervorgehoben " + std::to_string(lumpEditHervorShader()) + ", soll " + std::to_string(z->neuNr));
        return lumpEditHervorShader() == z->neuNr && lumpEditFlaeche() < 0;
    }));
    add(foto("le_text"));
    add(tu("Lumps C: im Text den Namen des neuen Shaders ersetzen", [z] {
        std::string t = lumpEditPuffer();
        const std::string alt = std::to_string(z->neuNr) + " \"" + z->passend + "\"";
        const std::size_t p = t.find(alt);
        if (p != std::string::npos) { t.replace(p, alt.size(), std::to_string(z->neuNr) + " \"" + z->passend2 + "\""); }
        lumpEditPufferSetzen(t);
        z->undo0 = lumpEditRueckgaengigZahl();
    }));
    add(klickAn("Lumps C: ins Textfeld (Fokus, die Zeile des Shaders)", [z] { return lumpEditZeilenOrt(karte::lumpTextZeile(lumpEditText(), false, z->neuNr) - 1); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Enter, "Strg+Enter"));
    add(warteBis("Lumps C: uebernommen", [z] { return lumpEditRueckgaengigZahl() == z->undo0 + 1; }, 600));
    add(pause(0.3));
    add(pruefe("Lumps C: Lumps, Fenster und Vorschau zeigen den Namen aus dem Text", [z] {
        const karte::BspLumps* l = lumpEditLumps();
        if (l == nullptr) { return false; }
        const int nr = l->flaechen[static_cast<std::size_t>(z->flaeche)].shaderNum;
        const BspGeometry& g = g_app->geo;
        const bool vorschau = g.shaders.size() > static_cast<std::size_t>(nr) && g.shaders[static_cast<std::size_t>(nr)].name == z->passend2 &&
                              g.surfaces[static_cast<std::size_t>(z->flaeche)].shader == nr;
        diag::detail("Knopftest: Lumps C - Flaeche hat Shader " + std::to_string(nr) + " \"" + l->shader[static_cast<std::size_t>(nr)].name + "\", Vorschau " +
                     (vorschau ? "ja" : "nein") + ", Meldung: " + lumpEditMeldung());
        return nr == z->neuNr && l->shader[static_cast<std::size_t>(nr)].name == z->passend2 && vorschau;
    }));
    add(tu("Lumps C: Fehler im Text - eine Shaderzeile ohne Anfuehrungszeichen", [z] {
        std::string t = lumpEditPuffer();
        const std::string a = std::to_string(z->neuNr) + " \"" + z->passend2 + "\"";
        const std::size_t p = t.find(a);
        if (p != std::string::npos) { t.replace(p, a.size(), std::to_string(z->neuNr) + " " + z->passend2); }
        lumpEditPufferSetzen(t);
        z->undo0 = lumpEditRueckgaengigZahl();
        (void)lumpEditTextUebernehmen();
    }));
    add(pruefe("Lumps C: Fehler mit Zeilennummer, nichts uebernommen", [z] {
        const int soll = karte::lumpTextZeile(lumpEditText(), false, z->neuNr);
        diag::detail("Knopftest: Lumps C - Fehlerzeile " + std::to_string(lumpEditFehlerZeile() + 1) + ", soll " + std::to_string(soll) + ": " + lumpEditMeldung());
        return lumpEditFehlerZeile() + 1 == soll && lumpEditRueckgaengigZahl() == z->undo0;
    }));
    add(tu("Lumps C: Text verwerfen (wieder aus den Lumps)", [] { lumpEditPufferSetzen(latin1NachUtf8(lumpEditText())); (void)lumpEditTextUebernehmen(); }));

    // --- D: Rueckgaengig / Wiederholen -----------------------------------------------------------
    add(tu("Lumps D: Rueckgaengig", [] { (void)lumpEditRueckgaengig(); }));
    add(pause(0.3));
    add(pruefe("Lumps D: Rueckgaengig - Name aus B wieder da, auch in der Vorschau", [z] {
        const karte::BspLumps* l = lumpEditLumps();
        return l != nullptr && l->shader[static_cast<std::size_t>(z->neuNr)].name == z->passend && g_app->geo.shaders[static_cast<std::size_t>(z->neuNr)].name == z->passend;
    }));
    add(tu("Lumps D: noch einmal Rueckgaengig (vor B)", [] { (void)lumpEditRueckgaengig(); }));
    add(pause(0.3));
    add(pruefe("Lumps D: vor B - der Shader-Lump wie in der Datei, Flaeche mit altem Shader", [z] {
        karte::BspLumps o;
        const karte::BspLumps* l = lumpEditLumps();
        return l != nullptr && karte::bspLumpsLesen(z->bsp, o) && l->shader.size() == o.shader.size() &&
               l->flaechen[static_cast<std::size_t>(z->flaeche)].shaderNum == o.flaechen[static_cast<std::size_t>(z->flaeche)].shaderNum &&
               g_app->geo.shaders.size() == o.shader.size() && !lumpEditText().empty() && lumpEditDateiBytes() == z->bsp;
    }));
    add(tu("Lumps D: zweimal Wiederholen", [] {
        (void)lumpEditWiederholen();
        (void)lumpEditWiederholen();
    }));
    add(pause(0.3));
    add(pruefe("Lumps D: Wiederholen - Stand nach C, Vorschau mit", [z] {
        const karte::BspLumps* l = lumpEditLumps();
        return l != nullptr && l->shader[static_cast<std::size_t>(z->neuNr)].name == z->passend2 && g_app->geo.shaders[static_cast<std::size_t>(z->neuNr)].name == z->passend2;
    }));

    // --- E: Warnung bei unpassendem Shader --------------------------------------------------------
    add(tu("Lumps E: Reiter Flaeche, Flaeche mit Lightmap -> Shader ohne $lightmap-Stufe", [z] {
        lumpEditReiter(0);
        lumpEditFlaecheSetzen(z->flaeche);
        (void)lumpEditTauschen(z->ohneStufe, false);
    }));
    add(pause(0.4));
    add(pruefe("Lumps E: Warnung (Lightmap, keine $lightmap-Stufe - tr_shader.cpp)", [z] {
        diag::detail("Knopftest: Lumps E - " + lumpEditWarnung());
        return !z->ohneStufe.empty() && lumpEditLightmapRegel(0, z->ohneStufe) == 1 && lumpEditWarnung().find(z->ohneStufe) != std::string::npos;
    }));
    add(foto("le_warnung"));
    add(pruefe("Lumps E: umgekehrt - Flaeche ohne Lightmap, Shader mit $lightmap-Stufe (weisses Bild)", [z] {
        const karte::BspLumps* l = lumpEditLumps();
        int ohne = -1;
        for (std::size_t i = 0; l != nullptr && i < l->flaechen.size() && ohne < 0; ++i) {
            if (l->flaechen[i].lightmap < 0 && l->flaechen[i].typ != 4) { ohne = static_cast<int>(i); }
        }
        if (ohne < 0) {
            diag::detail("Knopftest: Lumps E - keine Flaeche ohne Lightmap in der Karte, nur die Regel geprueft");
            return lumpEditLightmapRegel(-3, z->passend) == 2;
        }
        lumpEditFlaecheSetzen(ohne);
        const bool ok = lumpEditTauschen(z->passend, false);
        diag::detail("Knopftest: Lumps E - Flaeche " + std::to_string(ohne) + " (Lightmap " + std::to_string(l->flaechen[static_cast<std::size_t>(ohne)].lightmap) +
                     "): " + lumpEditWarnung());
        return ok && lumpEditWarnung().find(z->passend) != std::string::npos && lumpEditLightmapRegel(-3, z->passend) == 2;
    }));

    // --- F: Brush-Seiten und Kollision -------------------------------------------------------------
    add(tu("Lumps F: die Flaeche eines Shaders mit Brushes waehlen (Fenster zeigt die Nutzung)", [z] {
        const karte::BspLumps* l = lumpEditLumps();
        for (std::size_t i = 0; l != nullptr && i < l->flaechen.size(); ++i) {
            const karte::ShaderNutzung u = karte::shaderNutzung(*l, l->flaechen[i].shaderNum);
            if (u.brushes > 0 && u.seiten > 0) {
                lumpEditFlaecheSetzen(static_cast<int>(i));
                z->neuNr = l->flaechen[i].shaderNum;
                break;
            }
        }
    }));
    add(pause(0.4));
    add(pruefe("Lumps F: Brushes und Brush-Seiten im Fenster wie unabhaengig im Roh-Lump gezaehlt", [z] {
        int brushes = 0;
        int seiten = 0;
        rohZaehlen(lumpEditDateiBytes(), z->neuNr, brushes, seiten);
        char soll[160];
        std::snprintf(soll, sizeof(soll), tr(Str::LeKollision), brushes, seiten, 0);
        const std::string t = lumpEditNutzungText();
        const std::string b = std::to_string(brushes);
        const std::string sz = std::to_string(seiten);
        diag::detail("Knopftest: Lumps F - Shader " + std::to_string(z->neuNr) + ": roh " + b + " Brushes, " + sz + " Seiten; Fenster: " + t);
        return brushes > 0 && t.find(b) != std::string::npos && t.find(sz) != std::string::npos && t.rfind(std::string(soll).substr(0, 10), 0) == 0;
    }));
    add(tu("Lumps F: CONTENTS_WATER am Shader dazu", [z] {
        const karte::BspLumps* l = lumpEditLumps();
        if (l != nullptr) {
            const karte::LumpShader& sh = l->shader[static_cast<std::size_t>(z->neuNr)];
            (void)lumpEditFlagsSetzen(z->neuNr, sh.surfaceFlags, sh.contentFlags | 0x00000004U);
        }
    }));
    add(pause(0.4));
    add(pruefe("Lumps F: die Kollision der Karte - jeder Brush mit dem Shader hat jetzt CONTENTS_WATER (CMod_LoadBrushes)", [z] {
        const std::shared_ptr<const KollisionsKarte> k = geladeneKollision(g_app->map.path);
        int mit = 0;
        int alle = 0;
        for (const KollisionsKarte::Pinsel& p : k ? k->pinsel : std::vector<KollisionsKarte::Pinsel>{}) {
            if (p.shader != z->neuNr) { continue; }
            ++alle;
            mit += (p.inhalt & 0x00000004U) != 0U ? 1 : 0;
        }
        diag::detail("Knopftest: Lumps F - " + std::to_string(mit) + " von " + std::to_string(alle) + " Brushes mit CONTENTS_WATER");
        return alle > 0 && mit == alle;
    }));

    // --- G: Speichern Weg 1 und Weg 2 -------------------------------------------------------------
    add(tu("Lumps G: Speichern, Weg 1 (in der .pk3, gemerkt)", [z] {
        z->pk3Vorher = lies(z->pk3);
        g_app->settings.pk3SpeicherWeg = 1;
        lumpEditSpeichern();
    }));
    add(pause(0.4));
    add(pruefe("Lumps G: Weg 1 - der Eintrag in der .pk3 ist die bearbeitete Datei, .pk3.bak = vorher", [z] {
        z->eintragNachWeg1 = eintrag(z->pk3, "maps/lumpedit.bsp");
        const bool bak = lies(z->pk3 + ".bak") == z->pk3Vorher;
        diag::detail("Knopftest: Lumps G - Eintrag " + std::to_string(z->eintragNachWeg1.size()) + " Bytes, Dokument " + std::to_string(lumpEditDateiBytes().size()) +
                     ", .bak " + (bak ? "ja" : "nein") + ", ungespeichert " + (lumpEditIstUngespeichert() ? "ja" : "nein"));
        return !z->eintragNachWeg1.empty() && z->eintragNachWeg1 == lumpEditDateiBytes() && bak && !lumpEditIstUngespeichert();
    }));
    add(tu("Lumps G: noch ein Flag, Speichern Weg 2 (lose im Mod-Ordner, gemerkt)", [z] {
        const karte::BspLumps* l = lumpEditLumps();
        if (l != nullptr) {
            const karte::LumpShader& sh = l->shader[static_cast<std::size_t>(z->neuNr)];
            (void)lumpEditFlagsSetzen(z->neuNr, sh.surfaceFlags | 0x00100000U, sh.contentFlags);   // SURF_NOMARKS
        }
        g_app->settings.pk3SpeicherWeg = 2;
        lumpEditSpeichern();
    }));
    add(pause(0.4));
    add(pruefe("Lumps G: Weg 2 - lose Datei im Mod-Ordner = Dokument, .pk3 unveraendert seit Weg 1", [z] {
        const std::string lose = lies(z->mod + "\\maps\\lumpedit.bsp");
        karte::BspLumps w;
        const bool flag = karte::bspLumpsLesen(lose, w) && (w.shader[static_cast<std::size_t>(z->neuNr)].surfaceFlags & 0x00100000U) != 0U;
        diag::detail("Knopftest: Lumps G - lose " + std::to_string(lose.size()) + " Bytes, SURF_NOMARKS " + (flag ? "ja" : "nein"));
        return lose == lumpEditDateiBytes() && flag && eintrag(z->pk3, "maps/lumpedit.bsp") == z->eintragNachWeg1;
    }));
    add(tu("Lumps: Speicherweg zuruecksetzen", [] { g_app->settings.pk3SpeicherWeg = 0; }));

    // --- H: Karten-Editor -----------------------------------------------------------------------
    add(tu("Lumps H: Karten-Editor, Fenster Shader und Flaechen", [] {
        g_app->leftMode = 3;
        dock::ausblenden("karte.lumps", false);
    }));
    add(pause(1.5));
    add(tu("Lumps H: File > Open .bsp... mit der losen Datei aus G", [z] { (void)karteneditor::bspOeffnenDatei(z->mod + "\\maps\\lumpedit.bsp"); }));
    add(warteBis("Lumps H: die fertige Karte im Fenster", [] { return lumpEditLumps() != nullptr && !lumpEditPuffer().empty(); }, 1800));
    add(tu("Lumps H: Klick in die Kamera auf die Flaeche aus A (Strahl wie Scene_Intersect)", [z] {
        const BspGeometry& g = g_app->geo;
        if (z->flaeche < 0 || static_cast<std::size_t>(z->flaeche) >= g.surfaces.size()) { return; }
        const BspSurface& f = g.surfaces[static_cast<std::size_t>(z->flaeche)];
        float a[3];
        float b[3];
        for (int k = 0; k < 3; ++k) {
            const float m = (f.mins[k] + f.maxs[k]) * 0.5F;
            a[k] = m + f.ebenenNormale[k] * 64.0F;
            b[k] = m - f.ebenenNormale[k] * 64.0F;
        }
        (void)lumpEditKarteKlick(a, b);
    }));
    add(pruefe("Lumps H: die Flaeche ist gewaehlt", [z] {
        diag::detail("Knopftest: Lumps H - gewaehlt " + std::to_string(lumpEditFlaeche()) + ", soll " + std::to_string(z->flaeche));
        return lumpEditFlaeche() == z->flaeche;
    }));
    add(tu("Lumps H: Shader tauschen (ein Shader mit $lightmap-Stufe, den die Karte noch nicht hat)", [z] {
        const karte::BspLumps* l = lumpEditLumps();
        for (const auto& [name, info] : g_app->shaderMap) {
            if (l == nullptr || name.rfind("textures/", 0) != 0 || info.image.empty() || !info.lightmapStufe || shaderNr(*l, name) >= 0) { continue; }
            z->neuH = name;
            break;
        }
        z->undo0 = lumpEditRueckgaengigZahl();
        (void)lumpEditTauschen(z->neuH, false);
    }));
    add(pruefe("Lumps H: Lumps und Spielbild (neue Bytes, neue Nummer) haben den neuen Shader", [z] {
        const karte::BspLumps* l = lumpEditLumps();
        std::shared_ptr<const std::string> daten;
        std::uint64_t nummer = 0;
        const bool bild = lumpEditKarteSpielbild(daten, nummer) && daten && *daten == lumpEditDateiBytes();
        const int nr = l != nullptr ? l->flaechen[static_cast<std::size_t>(z->flaeche)].shaderNum : -1;
        diag::detail("Knopftest: Lumps H - Shader " + std::to_string(nr) + ", Spielbild " + (bild ? "ja" : "nein") + ", Nummer " + std::to_string(nummer & 0xFFFFU));
        return l != nullptr && nr >= 0 && !z->neuH.empty() && l->shader[static_cast<std::size_t>(nr)].name == z->neuH && bild &&
               lumpEditRueckgaengigZahl() == z->undo0 + 1;
    }));
    add(tu("Lumps H: Speichern", [z] {
        z->loseVorH = lies(z->mod + "\\maps\\lumpedit.bsp");
        lumpEditSpeichern();
    }));
    add(pause(0.4));
    add(pruefe("Lumps H: die lose Datei ist die bearbeitete, .bak ist der Stand vorher", [z] {
        const std::string lose = lies(z->mod + "\\maps\\lumpedit.bsp");
        const std::string bak = lies(z->mod + "\\maps\\lumpedit.bsp.bak");
        const bool alt = !bak.empty() && bak == z->loseVorH;
        diag::detail("Knopftest: Lumps H - lose " + std::to_string(lose.size()) + " Bytes, .bak " + std::to_string(bak.size()) + ", vorher " + (alt ? "ja" : "nein"));
        return lose == lumpEditDateiBytes() && alt && !lumpEditIstUngespeichert();
    }));
    add(tu("Lumps H: zurueck in den Mission-Tab", [] {
        dock::ausblenden("karte.lumps", true);
        g_app->leftMode = 1;
    }));
    return s;
}
