// selbsttest_enttext.inc - rc609 W2b: Entity-Texteditor (BHED_EDITORTEST=enttext)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor).
//
// md_am_sith aus den Spielordnern als lose Kopie im eigenen Laufordner (<Lauf>\enttext\maps\et.bsp):
//   A  Panel ueber das Menue "Fenster" auf, der Text ist der Entity-Lump Byte fuer Byte
//   B  Wahl in 3D springt zum Block, die Hinterlegung reicht von '{' bis '}'; Klick in den Text waehlt in 3D
//   C  Live: eine Ziffer im origin bewegt die Entity sofort, ein Rueckgaengig-Schritt
//   D  Gizmo-Ziehen aendert nur die origin-Zeile; der Inspektor nur seine Zeile
//   E  Uebernehmen (Strg+Enter) mit neuem Schluessel und Kommentar: ein Schritt, Strg+Z nimmt ihn zurueck,
//      der Kommentar bleibt
//   F  Fehler mit Zeile (offenes Anfuehrungszeichen, Euro-Zeichen): nichts uebernommen
//   G  Strg+F hebt hervor, F3 springt weiter, Esc hebt auf; H blendet aus, Alt+H ein (im Panel und ueber der Karte)
//   H  Entities speichern: die .ent ist der Text (Kommentar, Latin-1-Byte)
namespace enttext {

struct Zustand {
    std::string wurzel;
    std::string bsp;
    std::string lump;          // Entity-Lump (bis NUL)
    std::string doc0;          // Text vor einer Aenderung
    std::vector<MapEntity> ents0;
    std::size_t undo0 = 0;
    int ent = -1;              // gewaehlte Entity (mit origin, nicht worldspawn)
    int ent2 = -1;             // eine andere fuer den Klick in den Text
    std::string origin0;
    int zeileOrigin = -1;      // vor dem Kaputtmachen gemerkt (danach liest der Text nur bis zum Fehler)
    bool bereit = false;
};

std::string lies(const std::string& p) {
    std::ifstream f(::bhed::utf8Pfad(p), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
void schreib(const std::string& p, const std::string& t) {
    std::error_code ec;
    std::filesystem::create_directories(::bhed::utf8Pfad(p).parent_path(), ec);
    std::ofstream f(::bhed::utf8Pfad(p), std::ios::binary | std::ios::trunc);
    f.write(t.data(), static_cast<std::streamsize>(t.size()));
}
std::vector<std::string> zeilen(const std::string& t) {
    std::vector<std::string> z;
    std::size_t a = 0;
    for (std::size_t i = 0; i <= t.size(); ++i) {
        if (i == t.size() || t[i] == '\n') {
            z.push_back(t.substr(a, i - a));
            a = i + 1;
        }
    }
    return z;
}
// Welche Zeilen von a nach b anders sind (Anfang und Ende gleich): Anzahl und die erste neue.
int anders(const std::string& a, const std::string& b, std::string* neueZeile = nullptr) {
    const auto za = zeilen(a);
    const auto zb = zeilen(b);
    std::size_t i = 0;
    while (i < za.size() && i < zb.size() && za[i] == zb[i]) { ++i; }
    std::size_t ea = za.size();
    std::size_t eb = zb.size();
    while (ea > i && eb > i && za[ea - 1] == zb[eb - 1]) {
        --ea;
        --eb;
    }
    if (neueZeile != nullptr) { *neueZeile = i < eb ? zb[i] : std::string(); }
    return static_cast<int>(std::max(ea - i, eb - i));
}
// Zeichen eingeben, ohne vorher alles zu markieren (tippe() ersetzt den ganzen Inhalt).
Schritt eingabe(const std::string& text) {
    return {"Eingabe \"" + text + "\"", [=](int b) {
                if (b == 0) {
                    ImGui::GetIO().AddInputCharactersUTF8(text.c_str());
                    return false;
                }
                return b >= 3;
            }};
}
// Zeile (ab 0) des Paars key in Entity i im Puffer.
int paarZeile(int i, const char* key) {
    const int von = entTextBlockZeile(i);
    if (von < 0) { return -1; }
    const auto z = zeilen(entTextPuffer());
    for (std::size_t k = static_cast<std::size_t>(von); k < z.size(); ++k) {
        if (z[k].find(std::string("\"") + key + "\"") != std::string::npos) { return static_cast<int>(k); }
        if (z[k].find('}') != std::string::npos) { break; }
    }
    return -1;
}
// Die Kamera vor die Entity stellen (wie folgeKameraVor, fuer eine Entity).
void kameraVor(int i) {
    float p[3]{};
    if (i < 0 || !entityOrt(g_app->map.entities[static_cast<std::size_t>(i)], p)) { return; }
    const float ziel[3] = {p[0], p[1], p[2] + 16.0F};
    g_app->cam.pos[0] = p[0] - 220.0F;
    g_app->cam.pos[1] = p[1] - 60.0F;
    g_app->cam.pos[2] = p[2] + 120.0F;
    aimAngles(g_app->cam.pos, ziel, g_app->cam.angles);
    g_app->throughCamera = false;
    g_app->cameraSelected = false;
    g_app->mapDirty = true;
}

}  // namespace enttext

std::vector<Schritt> enttextSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<enttext::Zustand>();
    using namespace enttext;

    add(tu("Entity-Text: md_am_sith als lose Karte im Laufordner", [z] {
        std::error_code ec;
        z->wurzel = (std::filesystem::current_path() / "enttext").string();
        std::filesystem::remove_all(::bhed::utf8Pfad(z->wurzel), ec);
        g_app->settings.pk3SpeicherWeg = 0;
        if (!readFromArchives("maps/md_am_sith.bsp", z->bsp)) {
            diag::detail("Knopftest: Entity-Text - maps/md_am_sith.bsp nicht in den Spielordnern");
            return;
        }
        z->bereit = readBspEntityLump(z->bsp, z->lump);
        z->lump = z->lump.substr(0, std::min(z->lump.find('\0'), z->lump.size()));
        schreib(z->wurzel + "\\maps\\et.bsp", z->bsp);
        g_app->leftMode = 1;
        karteDateiLaden(z->wurzel + "\\maps\\et.bsp");
    }));
    add(warteBis("Entity-Text: Karte geladen", [z] { return !z->bereit || g_app->map.path == z->wurzel + "\\maps\\et.bsp"; }, 900));
    add(tu("Entity-Text A: Menue Fenster > Entity-Text (dock::ausblenden wie der Menuepunkt)", [] {
        dock::ausblenden("mission.enttext", false);
        // Rechts oben, damit die Karte daneben frei bleibt (Gizmo, Klicks in 3D).
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const std::string name = dock::fensterName("mission.enttext");
        ImGui::SetWindowPos(name.c_str(), ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.73F, vp->WorkPos.y + vp->WorkSize.y * 0.06F});
        ImGui::SetWindowSize(name.c_str(), ImVec2{vp->WorkSize.x * 0.26F, vp->WorkSize.y * 0.60F});
    }));
    add(warteBis("Entity-Text A: Panel gezeichnet", [] { return !entTextPuffer().empty(); }, 300));
    add(pause(0.3));
    add(pruefe("Entity-Text A: der Text ist der Entity-Lump Byte fuer Byte (als UTF-8 im Feld)", [z] {
        diag::detail("Knopftest: Entity-Text A - Lump " + std::to_string(z->lump.size()) + " Bytes, Text " + std::to_string(entTextDoc().size()) +
                     ", " + std::to_string(g_app->map.entities.size()) + " Entities");
        return z->bereit && entTextDoc() == z->lump && entTextPuffer() == latin1NachUtf8(z->lump);
    }));

    // --- B: Auswahl in beide Richtungen ------------------------------------------------
    add(tu("Entity-Text B: eine Entity mit origin weit hinten in der Liste in 3D waehlen", [z] {
        const int n = static_cast<int>(g_app->map.entities.size());
        for (int i = n * 2 / 3; i < n && z->ent < 0; ++i) {
            const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(i)];
            if (!e.origin.empty() && e.classname != "worldspawn" && e.keys.size() >= 3 && e.find("model") == nullptr) { z->ent = i; }
        }
        for (int i = n / 3; i < n && z->ent2 < 0; ++i) {
            if (i != z->ent && !g_app->map.entities[static_cast<std::size_t>(i)].origin.empty()) { z->ent2 = i; }
        }
        z->origin0 = z->ent >= 0 ? g_app->map.entities[static_cast<std::size_t>(z->ent)].origin : std::string();
        kameraVor(z->ent);
        entityAuswahlSetzen(z->ent);
    }));
    add(pause(0.5));
    add(pruefe("Entity-Text B: der Text sprang zum Block, Hinterlegung von '{' bis '}'", [z] {
        int von = -1;
        int bis = -1;
        entTextHinterlegt(von, bis);
        // unabhaengig gezaehlt: die ent-te '{' am Zeilenanfang und die '}' danach
        const auto zl = zeilen(entTextDoc());
        int nr = -1;
        int sollVon = -1;
        int sollBis = -1;
        for (std::size_t k = 0; k < zl.size(); ++k) {
            if (!zl[k].empty() && zl[k][0] == '{' && ++nr == z->ent) { sollVon = static_cast<int>(k); }
            if (sollVon >= 0 && sollBis < 0 && !zl[k].empty() && zl[k][0] == '}') { sollBis = static_cast<int>(k); }
        }
        const bool sichtbar = entTextZeilenOrt(von, 0).y >= 0.0F && entTextZeilenOrt(bis, 0).y >= 0.0F;
        diag::detail("Knopftest: Entity-Text B - Entity " + std::to_string(z->ent) + ", hinterlegt " + std::to_string(von + 1) + "-" +
                     std::to_string(bis + 1) + ", gezaehlt " + std::to_string(sollVon + 1) + "-" + std::to_string(sollBis + 1) + ", sichtbar " +
                     (sichtbar ? "ja" : "nein"));
        return z->ent >= 0 && von == sollVon && bis == sollBis && sichtbar;
    }));
    add(foto("et_auswahl"));
    add(tu("Entity-Text B: die zweite Entity zum Klicken ins Bild rollen (Wahl in 3D und zurueck)", [z] { entityAuswahlSetzen(z->ent2); }));
    add(pause(0.4));
    add(tu("Entity-Text B: Wahl wieder auf die erste (der Block der zweiten bleibt in der Naehe)", [z] { entityAuswahlSetzen(z->ent); }));
    add(pause(0.4));
    add(klickAn("Entity-Text B: in eine Zeile des Blocks der ersten Entity +1 (Block-Inneres)", [z] {
        return entTextZeilenOrt(entTextBlockZeile(z->ent) + 1, 3);
    }));
    add(pruefe("Entity-Text B: Klick in den Block laesst die Wahl (dieselbe Entity)", [z] { return g_app->pickedEntity == z->ent; }));
    add(tu("Entity-Text B: in 3D abwaehlen", [] { entityAuswahlSetzen(-1); }));
    add(pause(0.3));
    add(klickAn("Entity-Text B: in die Zeile mit '}' derselben Entity", [z] {
        return entTextZeilenOrt(paarZeile(z->ent, "classname"), 1);
    }));
    add(pause(0.2));
    add(pruefe("Entity-Text B: Klick im Text waehlt die Entity in 3D", [z] {
        diag::detail("Knopftest: Entity-Text B - nach Klick gewaehlt " + std::to_string(g_app->pickedEntity));
        return g_app->pickedEntity == z->ent;
    }));

    // --- C: Live-origin -------------------------------------------------------------------
    add(tu("Entity-Text C: Stand merken", [z] {
        z->doc0 = entTextDoc();
        z->ents0 = g_app->map.entities;
        z->undo0 = g_app->entUndo.size();
    }));
    add(klickAn("Entity-Text C: in die origin-Zeile", [z] { return entTextZeilenOrt(paarZeile(z->ent, "origin"), 2); }));
    add(taste(ImGuiKey_End, "Ende"));
    add(taste(ImGuiKey_LeftArrow, "Links (vor das schliessende Anfuehrungszeichen)"));
    add(eingabe("5"));
    add(pause(0.3));
    add(pruefe("Entity-Text C: origin bewegt die Entity sofort, ein Rueckgaengig-Schritt, eine Zeile anders", [z] {
        const std::string& o = g_app->map.entities[static_cast<std::size_t>(z->ent)].origin;
        const int n = anders(z->doc0, entTextDoc());
        diag::detail("Knopftest: Entity-Text C - origin \"" + z->origin0 + "\" -> \"" + o + "\", Rueckgaengig " + std::to_string(z->undo0) + " -> " +
                     std::to_string(g_app->entUndo.size()) + ", Zeilen anders " + std::to_string(n) + ", getippt offen " +
                     (entTextGeaendert() ? "ja" : "nein"));
        return o == z->origin0 + "5" && g_app->entUndo.size() == z->undo0 + 1 && n == 1 && !entTextGeaendert();
    }));
    add(eingabe("0"));
    add(pause(0.3));
    add(pruefe("Entity-Text C: weitertippen bleibt derselbe Rueckgaengig-Schritt", [z] {
        return g_app->map.entities[static_cast<std::size_t>(z->ent)].origin == z->origin0 + "50" && g_app->entUndo.size() == z->undo0 + 1;
    }));
    add(tu("Entity-Text C: Strg+Z (ueber das Rueckgaengig der Entities)", [] { (void)entityRueckgaengig(); }));
    add(pause(0.3));
    add(pruefe("Entity-Text C: ein Strg+Z stellt den alten Stand und den alten Text her", [z] {
        return entListeGleich(g_app->map.entities, z->ents0) && entTextDoc() == z->doc0;
    }));

    // --- D: Gizmo und Inspektor -------------------------------------------------------------
    add(tu("Entity-Text D: Kamera vor die Entity, Verschieben-Gizmo", [z] {
        kameraVor(z->ent);
        entityAuswahlSetzen(z->ent);
        g_app->gizmoMode = App::GizmoMode::Move;
        z->doc0 = entTextDoc();
    }));
    add(gezeichnet("Entity-Text D"));
    add(pause(0.4));
    add(tu("Entity-Text D: liegt das Panel ueber dem Gizmo, an den linken Rand damit", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(1, q);
        const std::string name = dock::fensterName("mission.enttext");
        if (ImGuiWindow* w = ImGui::FindWindowByName(name.c_str()); w != nullptr && q.x + 120.0F >= w->Pos.x && q.x - 40.0F <= w->Pos.x + w->Size.x) {
            ImGui::SetWindowPos(name.c_str(), ImVec2{ImGui::GetMainViewport()->WorkPos.x + 4.0F, w->Pos.y});
        }
    }));
    add(pause(0.3));
    add(ziehenAn("Entity-Text D: an der X-Achse des Gizmos ziehen", [] {
        ImVec2 q{-1.0F, -1.0F};
        const bool da = entityGizmoPunkt(1, q);
        const ImGuiWindow* unter = nullptr;
        for (int i = GImGui->Windows.Size - 1; i >= 0 && unter == nullptr; --i) {
            const ImGuiWindow* w = GImGui->Windows[i];
            if (w->WasActive && !w->Hidden && w->ParentWindow == nullptr && w->Rect().Contains(q)) { unter = w; }
        }
        diag::detail("Knopftest: Entity-Text D - Gizmo X " + std::string(da ? "da" : "fehlt") + " bei " + std::to_string(q.x) + "," + std::to_string(q.y) +
                     ", Fenster darunter " + (unter != nullptr ? unter->Name : "-"));
        return q;
    }, 50.0F, 0.0F, 0));
    add(pause(0.5));
    add(pruefe("Entity-Text D: Gizmo-Ziehen aendert nur die origin-Zeile", [z] {
        std::string neu;
        const int n = anders(z->doc0, entTextDoc(), &neu);
        const std::string& o = g_app->map.entities[static_cast<std::size_t>(z->ent)].origin;
        diag::detail("Knopftest: Entity-Text D - Zeilen anders " + std::to_string(n) + ", neue Zeile \"" + neu + "\", origin \"" + o + "\" (vorher \"" +
                     z->origin0 + "\")");
        return n == 1 && neu.find("\"origin\"") != std::string::npos && neu.find(o) != std::string::npos && o != z->origin0;
    }));
    add(tu("Entity-Text D: Panel wieder rechts", [] {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetWindowPos(dock::fensterName("mission.enttext").c_str(), ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.73F, vp->WorkPos.y + vp->WorkSize.y * 0.06F});
    }));
    add(tu("Entity-Text D: im Inspektor angle setzen", [z] {
        z->doc0 = entTextDoc();
        (void)entityKeySetzen(z->ent, "angle", "135");
    }));
    add(pause(0.3));
    add(pruefe("Entity-Text D: der Inspektor aendert genau eine Zeile (Inspektor und Text gleich)", [z] {
        std::string neu;
        const int n = anders(z->doc0, entTextDoc(), &neu);
        std::vector<MapEntity> t;
        const bool gleich = parseEntitiesGenau(entTextDoc(), t) && entListeGleich(t, g_app->map.entities);
        diag::detail("Knopftest: Entity-Text D - Inspektor: Zeilen anders " + std::to_string(n) + " (\"" + neu + "\"), Text = Entities " +
                     (gleich ? "ja" : "nein"));
        return n == 1 && neu.find("\"angle\" \"135\"") != std::string::npos && gleich;
    }));

    // --- E: Uebernehmen ---------------------------------------------------------------------
    add(tu("Entity-Text E: Stand merken", [z] {
        z->doc0 = entTextDoc();
        z->ents0 = g_app->map.entities;
        z->undo0 = g_app->entUndo.size();
    }));
    add(klickAn("Entity-Text E: in die classname-Zeile", [z] { return entTextZeilenOrt(paarZeile(z->ent, "classname"), 2); }));
    add(taste(ImGuiKey_End, "Ende"));
    add(eingabe("\n// rc609 W2b \xC3\xBC\n\"rc609\" \"w2b T\xC3\xBCr\""));
    add(pause(0.3));
    add(pruefe("Entity-Text E: neuer Schluessel wirkt nicht beim Tippen (nicht live)", [z] {
        return entTextGeaendert() && entListeGleich(g_app->map.entities, z->ents0);
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Enter, "Strg+Enter"));
    add(pause(0.3));
    add(pruefe("Entity-Text E: Uebernehmen - ein Rueckgaengig-Schritt, der Wert als Latin-1-Byte", [z] {
        const std::string* v = g_app->map.entities[static_cast<std::size_t>(z->ent)].find("rc609");
        diag::detail("Knopftest: Entity-Text E - Wert \"" + (v != nullptr ? *v : std::string("-")) + "\", Rueckgaengig " + std::to_string(z->undo0) +
                     " -> " + std::to_string(g_app->entUndo.size()) + ", " + entTextMeldung());
        return v != nullptr && *v == "w2b T\xFCr" && g_app->entUndo.size() == z->undo0 + 1 && !entTextGeaendert() &&
               entTextDoc().find("// rc609 W2b \xFC\n") != std::string::npos;
    }));
    add(tu("Entity-Text E: Strg+Z", [] { (void)entityRueckgaengig(); }));
    add(pause(0.3));
    add(pruefe("Entity-Text E: Strg+Z nimmt die Uebernahme ganz zurueck, der Kommentar bleibt im Text", [z] {
        return entListeGleich(g_app->map.entities, z->ents0) && entTextDoc().find("\"rc609\"") == std::string::npos &&
               entTextDoc().find("// rc609 W2b \xFC\n") != std::string::npos;
    }));
    add(tu("Entity-Text E: Strg+Y", [] { (void)entityWiederholen(); }));
    add(pause(0.3));

    // --- F: Fehler mit Zeile ---------------------------------------------------------------
    add(tu("Entity-Text F: Stand merken", [z] {
        z->ents0 = g_app->map.entities;
        z->undo0 = g_app->entUndo.size();
        z->zeileOrigin = paarZeile(z->ent, "origin");
    }));
    add(klickAn("Entity-Text F: in die origin-Zeile", [z] { return entTextZeilenOrt(z->zeileOrigin, 2); }));
    add(taste(ImGuiKey_End, "Ende"));
    add(eingabe(" \"offen"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Enter, "Strg+Enter"));
    add(pause(0.3));
    add(pruefe("Entity-Text F: offenes Anfuehrungszeichen - Fehler mit der Zeile, nichts uebernommen", [z] {
        const int soll = z->zeileOrigin;
        diag::detail("Knopftest: Entity-Text F - Fehlerzeile " + std::to_string(entTextFehlerZeile() + 1) + " (origin-Zeile " + std::to_string(soll + 1) +
                     "): " + entTextMeldung());
        return entTextFehlerZeile() == soll && !entTextMeldung().empty() && entListeGleich(g_app->map.entities, z->ents0) &&
               g_app->entUndo.size() == z->undo0;
    }));
    add(foto("et_fehler"));
    add(klickAn("Entity-Text F: zurueck in die origin-Zeile", [z] { return entTextZeilenOrt(z->zeileOrigin, 2); }));
    add(taste(ImGuiKey_End, "Ende"));
    for (int i = 0; i < 7; ++i) { add(taste(ImGuiKey_Backspace, "Rueck")); }
    add(eingabe("\xE2\x82\xAC"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Enter, "Strg+Enter"));
    add(pause(0.3));
    add(pruefe("Entity-Text F: Euro-Zeichen (nicht Latin-1) - Meldung mit Zeile, nichts uebernommen, nichts ersetzt", [z] {
        const int soll = z->zeileOrigin;
        diag::detail("Knopftest: Entity-Text F - Euro: Zeile " + std::to_string(entTextFehlerZeile() + 1) + ": " + entTextMeldung());
        return soll >= 0 && entTextFehlerZeile() == soll && !entTextMeldung().empty() && entListeGleich(g_app->map.entities, z->ents0) &&
               g_app->entUndo.size() == z->undo0;
    }));
    add(pruefe("Entity-Text F: nach dem Fehler bleibt das Textfeld aktiv (Weitertippen ohne Klick)", [] { return ImGui::GetIO().WantTextInput; }));
    add(taste(ImGuiKey_Backspace, "Rueck"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Enter, "Strg+Enter"));
    add(pause(0.3));
    add(pruefe("Entity-Text F: berichtigt - uebernommen ohne Aenderung (kein Schritt)", [z] {
        const auto zl = zeilen(entTextPuffer());
        diag::detail("Knopftest: Entity-Text F - berichtigt: Fehlerzeile " + std::to_string(entTextFehlerZeile() + 1) + ", getippt offen " +
                     (entTextGeaendert() ? "ja" : "nein") + ", Entities gleich " + (entListeGleich(g_app->map.entities, z->ents0) ? "ja" : "nein") +
                     ", Rueckgaengig " + std::to_string(z->undo0) + " -> " + std::to_string(g_app->entUndo.size()) + ", origin-Zeile \"" +
                     (z->zeileOrigin >= 0 && static_cast<std::size_t>(z->zeileOrigin) < zl.size() ? zl[static_cast<std::size_t>(z->zeileOrigin)] : std::string()) +
                     "\", " + entTextMeldung());
        return entTextFehlerZeile() < 0 && !entTextGeaendert() && entListeGleich(g_app->map.entities, z->ents0) && g_app->entUndo.size() == z->undo0;
    }));

    // --- G: Suchen, F3, Esc, H, Alt+H ---------------------------------------------------------
    add(klickAn("Entity-Text G: in den Text (Panel im Fokus)", [z] { return entTextZeilenOrt(z->zeileOrigin, 2); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_F, "Strg+F"));
    add(pause(0.2));
    add(tippe("info_player"));
    add(pause(0.3));
    add(pruefe("Entity-Text G: Strg+F - alle Treffer hervorgehoben", [] {
        diag::detail("Knopftest: Entity-Text G - " + std::to_string(entTextTrefferZahl()) + " Treffer, Hervorhebung " + (entTextHervorhebung() ? "an" : "aus"));
        return entTextTrefferZahl() >= 1 && entTextHervorhebung();
    }));
    add(taste(ImGuiKey_F3, "F3"));
    add(pause(0.2));
    add(pruefe("Entity-Text G: F3 springt zum ersten Treffer", [] { return entTextTrefferNr() == 0; }));
    add(taste(ImGuiKey_F3, "F3"));
    add(pause(0.2));
    add(pruefe("Entity-Text G: F3 springt weiter (oder rundum)", [] { return entTextTrefferNr() == (entTextTrefferZahl() > 1 ? 1 : 0); }));
    add(foto("et_suchen"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(pruefe("Entity-Text G: Esc hebt die Hervorhebung auf", [] { return !entTextHervorhebung(); }));
    add(klickAn("Entity-Text G: in die Zeilennummern (Panel im Fokus, Textfeld nicht)", [] { return entTextKopfOrt(); }));
    add(tu("Entity-Text G: die erste Entity waehlen", [z] { entityAuswahlSetzen(z->ent); }));
    add(pause(0.2));
    add(taste(ImGuiKey_H, "H"));
    add(pause(0.2));
    add(pruefe("Entity-Text G: H im Panel blendet die gewaehlte Entity in 3D aus", [z] {
        return g_app->hiddenEntities.count(static_cast<std::size_t>(z->ent)) == 1U;
    }));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_H, "Alt+H"));
    add(pause(0.2));
    add(pruefe("Entity-Text G: Alt+H blendet alle wieder ein", [] { return g_app->hiddenEntities.empty(); }));
    add(tu("Entity-Text G: Maus ueber die Karte (unten in der Kartenansicht, weg vom Panel)", [] {
        const float* k = g_app->kartenAnsicht;
        setzeMaus(ImVec2{k[0] + (k[2] - k[0]) * 0.25F, k[3] - (k[3] - k[1]) * 0.15F});
        ImGui::FocusWindow(nullptr);
    }));
    add(pause(0.3));
    add(taste(ImGuiKey_H, "H"));
    add(pause(0.2));
    add(pruefe("Entity-Text G: H ueber der Karte blendet aus", [z] {
        diag::detail(std::string("Knopftest: Entity-Text G - ueber der Karte: Fokus Bild ") + (g_app->entFokusBild ? "ja" : "nein") + ", ausgeblendet " +
                     std::to_string(g_app->hiddenEntities.size()));
        return g_app->hiddenEntities.count(static_cast<std::size_t>(z->ent)) == 1U;
    }));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_H, "Alt+H"));
    add(pause(0.2));
    add(pruefe("Entity-Text G: Alt+H ueber der Karte blendet ein", [] { return g_app->hiddenEntities.empty(); }));

    // --- H: Speichern ------------------------------------------------------------------------
    add(tu("Entity-Text H: Entities speichern", [] { (void)entitiesSpeichern(); }));
    add(pause(0.3));
    add(pruefe("Entity-Text H: die .ent neben der .bsp ist der Text (Kommentar und Latin-1-Byte erhalten)", [z] {
        const std::string ent = lies(z->wurzel + "\\maps\\et.ent");
        diag::detail("Knopftest: Entity-Text H - .ent " + std::to_string(ent.size()) + " Bytes, Text " + std::to_string(entTextDoc().size()));
        return !ent.empty() && ent == entTextDoc() && ent.find("// rc609 W2b \xFC\n") != std::string::npos &&
               ent.find("\"w2b T\xFCr\"") != std::string::npos;
    }));
    add(tu("Entity-Text: Panel zu, Laufordner aufraeumen", [z] {
        dock::ausblenden("mission.enttext", true);
        g_app->entitiesUngespeichert.clear();
    }));
    return s;
}
