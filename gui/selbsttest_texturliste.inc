// selbsttest_texturliste.inc - rc609 K: Texturen der Karte (BHED_EDITORTEST=texturliste)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (nach den Karten-Editor-Teilen, mit allen Helfern davor).
//
// Eine Testkarte (Raum mit Brushes, Patches in worldspawn und func_group, Texturen textures/k_alt/a, b, c im
// Import-Ordner des Laufs; textures/k_neu/ hat nur a und b):
//   A  Fenster ueber Misc > Texturen der Karte...; die Zahlen der Liste = unabhaengig aus der .map gezaehlt
//   B  Klick in die Zeile k_alt/a waehlt alle Brush-Seiten und Patches damit
//   C  Ersetzen durch k_neu/a (aus dem Texturbrowser): alle Brush-Seiten und Patches, ein Rueckgaengig-Schritt
//   D  Rueckgaengig (Edit > Undo): die .map Byte fuer Byte wie vorher
//   E  Ordner ersetzen k_alt -> k_neu: die Zuordnung meldet die Luecke k_alt/c, ersetzt a und b, c bleibt
//   F  Build > Single: bsp stage danach sauber (keine Fehlerzeile, kein fehlendes Bild), der Shader-Lump hat
//      die neuen Namen und k_alt/c, aber kein k_alt/a und k_alt/b
namespace texturliste {
namespace ke = karteneditor;

struct Zustand {
    std::string ordner;   // <Lauf>/texturliste
    std::string karte;    // tl.map
    std::map<std::string, std::pair<int, int>> vorher;   // aus der Datei gezaehlt: Shader -> {Seiten, Patches}
    std::size_t schritte = 0;
};

std::string tlKlein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Unkomprimierte 24-Bit-TGA b x h in einer Farbe
std::string tga(int b, int h, unsigned char r, unsigned char g, unsigned char bl) {
    std::string t(18, '\0');
    t[2] = 2;
    t[12] = static_cast<char>(b & 0xFF);
    t[13] = static_cast<char>(b >> 8);
    t[14] = static_cast<char>(h & 0xFF);
    t[15] = static_cast<char>(h >> 8);
    t[16] = 24;
    for (int i = 0; i < b * h; ++i) {
        t.push_back(static_cast<char>(bl));
        t.push_back(static_cast<char>(g));
        t.push_back(static_cast<char>(r));
    }
    return t;
}

void schreibe(const std::filesystem::path& p, const std::string& inhalt) {
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f.write(inhalt.data(), static_cast<std::streamsize>(inhalt.size()));
}

std::string lies(const std::string& p) {
    std::ifstream f(::bhed::utf8Pfad(p), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::string kasten(int x0, int y0, int z0, int x1, int y1, int z1, const char* tex) {
    char z[6][200];
    std::snprintf(z[0], sizeof(z[0]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0", x1, y1, z1, x1, y0, z1, x0, y1, z1, tex);
    std::snprintf(z[1], sizeof(z[1]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0", x1, y1, z1, x0, y1, z1, x1, y1, z0, tex);
    std::snprintf(z[2], sizeof(z[2]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0", x1, y1, z1, x1, y1, z0, x1, y0, z1, tex);
    std::snprintf(z[3], sizeof(z[3]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0", x0, y0, z0, x1, y0, z0, x0, y1, z0, tex);
    std::snprintf(z[4], sizeof(z[4]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0", x0, y0, z0, x0, y0, z1, x1, y0, z0, tex);
    std::snprintf(z[5], sizeof(z[5]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0", x0, y0, z0, x0, y1, z0, x0, y0, z1, tex);
    std::string t = "{\n";
    for (const auto& zeile : z) { t += std::string(zeile) + "\n"; }
    return t + "}\n";
}

std::string patch(const char* tex, int x, int y) {
    char t[700];
    std::snprintf(t, sizeof(t),
                  "{\npatchDef2\n{\n%s\n( 3 3 0 0 0 )\n(\n( ( %d %d 0 0 0 ) ( %d %d 48 0 0.5 ) ( %d %d 0 0 1 ) )\n"
                  "( ( %d %d 0 0.5 0 ) ( %d %d 48 0.5 0.5 ) ( %d %d 0 0.5 1 ) )\n( ( %d %d 0 1 0 ) ( %d %d 48 1 0.5 ) ( %d %d 0 1 1 ) )\n)\n}\n}\n",
                  tex, x, y, x, y + 48, x, y + 96, x + 48, y, x + 48, y + 48, x + 48, y + 96, x + 96, y, x + 96, y + 48, x + 96, y + 96);
    return t;
}

// Ein dichter Raum (Boden und Decke k_alt/a, Waende k_alt/b), ein Patch k_alt/b; func_group mit Saeule k_alt/c,
// Patch k_alt/a und Patch k_alt/c; Spielerstart und Licht
std::string karteText() {
    std::string t = "// entity 0\n{\n\"classname\" \"worldspawn\"\n";
    t += kasten(-272, -272, -16, 272, 272, 0, "k_alt/a");
    t += kasten(-272, -272, 256, 272, 272, 272, "k_alt/a");
    t += kasten(-272, 256, 0, 272, 272, 256, "k_alt/b");
    t += kasten(-272, -272, 0, 272, -256, 256, "k_alt/b");
    t += kasten(-272, -256, 0, -256, 256, 256, "k_alt/b");
    t += kasten(256, -256, 0, 272, 256, 256, "k_alt/b");
    t += patch("k_alt/b", -200, -200);
    t += "}\n// entity 1\n{\n\"classname\" \"func_group\"\n";
    t += kasten(64, 64, 0, 128, 128, 256, "k_alt/c");
    t += patch("k_alt/a", 100, -200);
    t += patch("k_alt/c", -200, 100);
    t += "}\n// entity 2\n{\n\"classname\" \"info_player_start\"\n\"origin\" \"-128 0 24\"\n}\n";
    t += "// entity 3\n{\n\"classname\" \"light\"\n\"origin\" \"0 0 200\"\n\"light\" \"300\"\n}\n";
    return t;
}

// Unabhaengig aus dem Text der .map: Shader (klein, mit textures/) -> {Brush-Seiten, Patches}
std::map<std::string, std::pair<int, int>> zaehlen(const std::string& text) {
    std::map<std::string, std::pair<int, int>> m;
    std::istringstream ein(text);
    std::string z;
    int patch = 0;   // 1: nach patchDef2, 2: nach dessen "{"
    while (std::getline(ein, z)) {
        if (!z.empty() && z.back() == '\r') { z.pop_back(); }
        if (z == "patchDef2") {
            patch = 1;
        } else if (patch == 1 && z == "{") {
            patch = 2;
        } else if (patch == 2) {
            ++m["textures/" + tlKlein(z)].second;
            patch = 0;
        } else if (z.rfind("( ", 0) == 0 && z.rfind("( (", 0) != 0 && z.find(") ( ") != std::string::npos) {
            std::size_t e = 0;
            for (int i = 0; i < 3; ++i) { e = z.find(')', e) + 1; }
            const std::size_t a = z.find_first_not_of(' ', e);
            ++m["textures/" + tlKlein(z.substr(a, z.find(' ', a) - a))].first;
        }
    }
    return m;
}

bool listeWieKarte(const std::map<std::string, std::pair<int, int>>& soll, std::string& text) {
    const auto& liste = ke::texturListeZeilen();
    bool ok = liste.size() == soll.size();
    for (const karte::TexturZeile& t : liste) {
        const auto it = soll.find(tlKlein(t.shader));
        ok = ok && it != soll.end() && it->second.first == t.seiten && it->second.second == t.patches;
        text += " " + t.shader + " " + std::to_string(t.seiten) + "/" + std::to_string(t.patches);
    }
    return ok;
}

int fehlerZeilen() {
    int n = 0;
    for (const ke::KonsolenZeile& z : ke::editor().konsole) {
        for (const auto& t : z.teile) { n += t.art == ke::KonsolenZeile::Art::Fehler ? 1 : 0; }
    }
    return n;
}

// Texturen und Testkarte anlegen, Karten-Editor mit dem Import-Ordner im Suchpfad, Karte oeffnen
void einrichten(std::vector<Schritt>& s, const std::shared_ptr<Zustand>& z) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Texturliste: Texturen k_alt (a, b, c; 64x64) und k_neu (a, b; 128x128) im Import-Ordner des Laufs, Testkarte", [z] {
        const std::filesystem::path imp = ::bhed::utf8Pfad(platform::settingsDirectory() + "\\import\\textures");
        schreibe(imp / "k_alt" / "a.tga", tga(64, 64, 200, 60, 60));
        schreibe(imp / "k_alt" / "b.tga", tga(64, 64, 60, 200, 60));
        schreibe(imp / "k_alt" / "c.tga", tga(64, 64, 60, 60, 200));
        schreibe(imp / "k_neu" / "a.tga", tga(128, 128, 230, 160, 40));
        schreibe(imp / "k_neu" / "b.tga", tga(128, 128, 40, 160, 230));
        z->ordner = (std::filesystem::current_path() / "texturliste").string();
        z->karte = z->ordner + "\\tl.map";
        schreibe(::bhed::utf8Pfad(z->karte), karteText());
        ke::editor().spielKennung.clear();   // der Suchpfad mit dem Import-Ordner (EnginePath_Realise)
        g_app->leftMode = 3;
    }));
    add(warteBis("Texturliste: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit() && !ke::editor().spielKennung.empty();
    }, 20000));
    add(tu("Texturliste: Testkarte oeffnen", [z] {
        karteDialogeZu();
        (void)ke::karteOeffnen(z->karte);
    }));
    add(warteBis("Texturliste: Karte offen", [] { return ke::editor().karte.entitaeten().size() >= 4; }, 3000));
    add(pause(0.5));
}

}  // namespace texturliste

std::vector<Schritt> texturlisteSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<texturliste::Zustand>();
    using namespace texturliste;
    einrichten(s, z);

    // --- A: Fenster, Zahlen wie die .map -------------------------------------------------------
    add(klick(ke::trK(Str::KeLMisc), "###main"));
    add(pause(0.3));
    add(klick(ke::trK(Str::TxlMenue), "##Menu_", 0, false));
    add(pause(0.6));
    add(pruefe("Texturliste A: Misc > Texturen der Karte... oeffnet das Fenster", [] { return g_app->settings.texListeOffen; }));
    add(tu("Texturliste A: Fenster rechts, fast so hoch wie das Programm", [] {
        const ImGuiIO& io = ImGui::GetIO();
        const std::string name = dock::fensterName("karte.texliste");
        ImGui::SetWindowPos(name.c_str(), ImVec2{io.DisplaySize.x * 0.5F, io.DisplaySize.y * 0.04F});
        ImGui::SetWindowSize(name.c_str(), ImVec2{io.DisplaySize.x * 0.48F, io.DisplaySize.y * 0.92F});
    }));
    add(pause(0.3));
    add(pruefe("Texturliste A: die Zahlen der Liste = aus der .map gezaehlt (Brush-Seiten, Patches)", [z] {
        z->vorher = zaehlen(lies(z->karte));
        std::string text;
        const bool ok = listeWieKarte(z->vorher, text);
        diag::detail("Knopftest: Texturliste A -" + text);
        return ok && z->vorher.size() == 3 && z->vorher["textures/k_alt/a"] == std::make_pair(12, 1) &&
               z->vorher["textures/k_alt/b"] == std::make_pair(24, 1) && z->vorher["textures/k_alt/c"] == std::make_pair(6, 1);
    }));

    // --- B: Klick waehlt -----------------------------------------------------------------------
    add(klickAn("Texturliste B: Klick in die Zeile textures/k_alt/a", [] { return ke::texturListeOrt("textures/k_alt/a"); }));
    add(pause(0.3));
    add(pruefe("Texturliste B: gewaehlt sind 12 Brush-Seiten und 1 Patch", [] {
        const auto& a = ke::editor().karte.auswahl;
        diag::detail("Knopftest: Texturliste B - Flaechen " + std::to_string(a.flaechen().size()) + ", Objekte " + std::to_string(a.anzahl()));
        return a.flaechen().size() == 12 && a.anzahl() == 1;
    }));

    // --- C: Ersetzen durch k_neu/a aus dem Texturbrowser -----------------------------------------
    add(tu("Texturliste C: im Texturbrowser textures/k_neu/a waehlen", [z] {
        ke::texturShaderWaehlen("textures/k_neu/a");
        z->schritte = ke::editor().karte.undo.abschluesse();
        (void)karte::karteSpeichern(z->ordner + "\\vorher.map", ke::editor().karte, false);
    }));
    add(pause(0.2));
    add(klickAn("Texturliste C: Aus dem Texturbrowser", [] { return ke::texturListeOrt("browser"); }));
    add(pause(0.2));
    add(klickAn("Texturliste C: Ersetzen", [] { return ke::texturListeOrt("ersetzen"); }));
    add(pause(0.4));
    add(pruefe("Texturliste C: alle 12 Brush-Seiten und der Patch tragen k_neu/a, ein Rueckgaengig-Schritt", [z] {
        const auto nach = zaehlen(karte::karteSchreiben(ke::editor().karte));
        const auto& u = ke::editor().karte.undo;
        const auto it = nach.find("textures/k_neu/a");
        diag::detail("Knopftest: Texturliste C - k_neu/a " + (it != nach.end() ? std::to_string(it->second.first) + "/" + std::to_string(it->second.second) : "-") +
                     ", k_alt/a " + std::to_string(nach.count("textures/k_alt/a")) + ", Schritte +" + std::to_string(u.abschluesse() - z->schritte) +
                     ", Meldung " + ke::texturListeMeldung());
        return it != nach.end() && it->second == std::make_pair(12, 1) && nach.count("textures/k_alt/a") == 0 && u.abschluesse() == z->schritte + 1 &&
               u.undoName().rfind("textureListReplace", 0) == 0;
    }));

    // --- D: Rueckgaengig Byte fuer Byte -----------------------------------------------------------
    add(tu("Texturliste D: Edit > Undo", [] { (void)ke::ausfuehren("Undo"); }));
    add(pause(0.3));
    add(pruefe("Texturliste D: die .map nach Rueckgaengig = vorher, Byte fuer Byte", [z] {
        (void)karte::karteSpeichern(z->ordner + "\\nachher.map", ke::editor().karte, false);
        const std::string a = lies(z->ordner + "\\vorher.map");
        const std::string b = lies(z->ordner + "\\nachher.map");
        diag::detail("Knopftest: Texturliste D - vorher " + std::to_string(a.size()) + " Bytes, nachher " + std::to_string(b.size()));
        return !a.empty() && a == b;
    }));

    // --- E: Ordner ersetzen mit Luecke ------------------------------------------------------------
    add(klickAn("Texturliste E: Klick in die Zeile textures/k_alt/b (alter Ordner textures/k_alt/)", [] { return ke::texturListeOrt("textures/k_alt/b"); }));
    add(tu("Texturliste E: Texturbrowser zeigt k_neu/", [] { ke::texturVerzeichnisZeigen("k_neu/"); }));
    add(pause(0.3));
    add(klickAn("Texturliste: Ordner des Texturbrowsers", [] { return ke::texturListeOrt("ordnerbrowser"); }));
    add(pause(0.2));
    add(klickAn("Texturliste E: Zuordnung zeigen", [] { return ke::texturListeOrt("zuordnung"); }));
    add(pause(0.3));
    add(pruefe("Texturliste E: Zuordnung a, b -> k_neu; die Luecke k_alt/c ist gemeldet", [] {
        const auto& zu = ke::texturListeZuordnung();
        int luecken = 0;
        bool ok = zu.size() == 3;
        for (const karte::OrdnerZuordnung& o : zu) {
            luecken += o.fehlt ? 1 : 0;
            ok = ok && (tlKlein(o.alt) == "textures/k_alt/c") == o.fehlt && tlKlein(o.neu) == "textures/k_neu/" + tlKlein(o.alt).substr(15);
            diag::detail("Knopftest: Texturliste E - " + o.alt + " -> " + o.neu + (o.fehlt ? " (Luecke)" : ""));
        }
        return ok && luecken == 1;
    }));
    add(tu("Texturliste E: Schritte merken", [z] { z->schritte = ke::editor().karte.undo.abschluesse(); }));
    add(klickAn("Texturliste E: 2 Texturen ersetzen", [] { return ke::texturListeOrt("ordnerersetzen"); }));
    add(pause(0.4));
    add(pruefe("Texturliste E: a und b ersetzt (Brushes und Patches), k_alt/c bleibt, ein Rueckgaengig-Schritt", [z] {
        const auto nach = zaehlen(karte::karteSchreiben(ke::editor().karte));
        std::string text;
        for (const auto& [n, w] : nach) { text += " " + n + " " + std::to_string(w.first) + "/" + std::to_string(w.second); }
        diag::detail("Knopftest: Texturliste E nachher -" + text);
        return nach.size() == 3 && nach.count("textures/k_neu/a") == 1 && nach.at("textures/k_neu/a") == std::make_pair(12, 1) &&
               nach.count("textures/k_neu/b") == 1 && nach.at("textures/k_neu/b") == std::make_pair(24, 1) && nach.count("textures/k_alt/c") == 1 &&
               nach.at("textures/k_alt/c") == std::make_pair(6, 1) && ke::editor().karte.undo.abschluesse() == z->schritte + 1;
    }));

    // --- F: Bau danach sauber -----------------------------------------------------------------------
    add(tu("Texturliste F: Konsole leeren, Build-Menue des Spiels", [] {
        ke::konsoleLeeren();
        ke::bauMenueSicherstellen();
        stufe3::q3map2Bereitstellen();   // BHED_Q3MAP2 bzw. das gebaute bhed_q3map2.exe neben das Programm
    }));
    add(stufe3::bauMenuepunkt("Single: bsp stage"));
    add(warteBis("Texturliste F: Bauen fertig", [] { return !ke::bauLaeuft(); }, 3600));
    add(pruefe("Texturliste F: Bau sauber - keine Fehlerzeile, kein fehlendes Bild; der Shader-Lump hat k_neu/a, k_neu/b, k_alt/c", [z] {
        const std::string k = stufe3::konsoleText();
        karte::BspLumps l;
        const bool gelesen = karte::bspLumpsLesen(lies(z->ordner + "\\tl.bsp"), l);
        std::set<std::string> namen;
        for (const auto& sh : l.shader) { namen.insert(tlKlein(sh.name)); }
        std::string text;
        for (const std::string& n : namen) { text += " " + n; }
        diag::detail("Knopftest: Texturliste F - bsp " + std::string(gelesen ? "ja" : "nein") + ", Fehlerzeilen " + std::to_string(fehlerZeilen()) + ", Shader" + text);
        return gelesen && fehlerZeilen() == 0 && k.find("Couldn't find image") == std::string::npos && k.find("leaked") == std::string::npos &&
               namen.count("textures/k_neu/a") == 1 && namen.count("textures/k_neu/b") == 1 && namen.count("textures/k_alt/c") == 1 &&
               namen.count("textures/k_alt/a") == 0 && namen.count("textures/k_alt/b") == 0;
    }));
    add(tu("Texturliste: Fenster zu, Mission-Tab", [] {
        dock::ausblenden("karte.texliste", true);
        g_app->leftMode = 1;
    }));
    return s;
}

// BHED_EDITORTEST=handbuch mit BHED_HB_NUR=texturliste: das Bild h_texturliste - das Fenster rechts ueber der
// Kamera, die Zeile k_alt/a angeklickt (ihre Flaechen in der Kamera gewaehlt), Ersetzen durch k_neu/a aus dem
// Texturbrowser, die Ordner-Zuordnung k_alt -> k_neu mit der Luecke k_alt/c.
std::vector<Schritt> texturlisteHandbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<texturliste::Zustand>();
    using namespace texturliste;
    einrichten(s, z);
    add(tu("Handbuch Texturliste: Kamera texturiert in den Raum, Fenster auf", [] {
        ke::setzeNrcBegriffe(false);
        ke::Editor& e = ke::editor();
        e.kameraE.modus = karte::KameraModus::Textur;
        e.kamera.ursprung = karte::Vektor3f(-230.0F, -230.0F, 220.0F);
        e.kamera.winkel = karte::Vektor3f(-28.0F, 45.0F, 0.0F);
        ke::texturListeOeffnen();
    }));
    add(pause(0.6));   // das Fenster ist einmal gezeichnet, bevor es verschoben wird
    add(tu("Handbuch Texturliste: Fenster rechts (erst wenn es gezeichnet ist)", [] {
        const ImGuiIO& io = ImGui::GetIO();
        const std::string name = dock::fensterName("karte.texliste");
        ImGui::SetWindowPos(name.c_str(), ImVec2{io.DisplaySize.x * 0.55F, io.DisplaySize.y * 0.06F});
        ImGui::SetWindowSize(name.c_str(), ImVec2{io.DisplaySize.x * 0.43F, io.DisplaySize.y * 0.88F});
    }));
    add(pause(0.6));
    add(klickAn("Handbuch Texturliste: Zeile k_alt/a", [] { return ke::texturListeOrt("textures/k_alt/a"); }));
    add(tu("Handbuch Texturliste: Texturbrowser k_neu/a, Ordner k_neu/", [] {
        ke::texturShaderWaehlen("textures/k_neu/a");
        ke::texturVerzeichnisZeigen("k_neu/");
    }));
    add(pause(0.3));
    add(klickAn("Handbuch Texturliste: Aus dem Texturbrowser", [] { return ke::texturListeOrt("browser"); }));
    add(klickAn("Texturliste: Ordner des Texturbrowsers", [] { return ke::texturListeOrt("ordnerbrowser"); }));
    add(pause(0.2));
    add(klickAn("Handbuch Texturliste: Zuordnung zeigen", [] { return ke::texturListeOrt("zuordnung"); }));
    add(tu("Handbuch Texturliste: Maus aus dem Bild", [] {
        const ImGuiIO& io = ImGui::GetIO();
        setzeMaus(ImVec2{io.DisplaySize.x * 0.3F, io.DisplaySize.y - 4.0F});
    }));
    add(warteBis("Handbuch Texturliste: Bilder geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    add(pause(1.0));
    hb::markiere("h_texturliste", [] {
        std::vector<hb::Marke> m;
        const float fs = ImGui::GetFontSize();
        const ImRect f = hb::fensterRect("###dock.karte.texliste");
        const ImVec2 zeile = ke::texturListeOrt("textures/k_alt/a");
        if (zeile.x > 0.0F) {
            // genau die Zeile (Bild 2 * Schrift + Zellabstand), sonst deckt der Rahmen die Spaltenkoepfe
            const float h = std::round(fs * 2.0F) * 0.5F + ImGui::GetStyle().CellPadding.y;
            m.emplace_back(ImRect{f.Min.x + 4.0F, zeile.y - h, f.Max.x - 4.0F, zeile.y + h}, 1);
        }
        const ImVec2 b = ke::texturListeOrt("browser");
        if (b.x > 0.0F) {
            // so breit wie der Knopf (Beschriftung + Rahmenabstand), nicht ueber den Fensterrand
            const float halb = ImGui::CalcTextSize(ke::trK(Str::TxlAusBrowser)).x * 0.5F + ImGui::GetStyle().FramePadding.x + fs * 0.3F;
            ImRect r = hb::um(b, halb, fs * 0.9F);
            r.Max.x = std::min(r.Max.x, f.Max.x - 4.0F);
            m.emplace_back(r, 2);
        }
        const ImVec2 o = ke::texturListeOrt("ordnerersetzen");
        if (o.x > 0.0F) { m.emplace_back(ImRect{f.Min.x + 4.0F, o.y - fs * 5.25F, f.Max.x - 4.0F, o.y - fs * 0.8F}, 3); }
        return m;
    });
    add(hb::fotoTeil("h_texturliste", [] { return ImRect{0.0F, 0.0F, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y}; }));
    add(tu("Handbuch Texturliste: Fenster zu", [] {
        dock::ausblenden("karte.texliste", true);
        g_app->leftMode = 0;
    }));
    return s;
}
