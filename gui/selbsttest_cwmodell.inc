// selbsttest_cwmodell.inc - CW-MODELL (15.11.2026, Clone-Wars-Kette Teil 3) im Model-Tab
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=cwmodell | cwmodellhb)
//
//   cwmodell    Detailstufen: Gruppe ProOptimizer / Decimate im Bereich Modellieren (3ds Max / Blender), Speichern ohne Erzeugen
//               dieselben Bytes wie die Datei des Spiels, Erzeugen (ein Rueckgaengig-Schritt, die Ansicht zeigt die letzte Stufe, Ecken
//               je Stufe fallen), Speichern mit drei Stufen in der .glm; .md3 mit name_1.md3 / name_2.md3 beim Speichern (alle Bilder);
//               die Dateien name_1.md3 neben einer .md3 des Spiels in "Detailstufe und Skin"; Clone-Wars-Richtwert nur mit dem Profil.
//               Bolts: *driver anlegen, Hinweise nur als Fahrzeug, *muzzle1, Speichern, Rueckgaengig. Eigenes Skelett an einer Waffe:
//               Neues Skelett, Knochen, Flaechen starr, Sequenzen, GLA/GLM/animation.cfg schreiben und lesen, Rueckgaengig; unbekannte
//               Namen der animation.cfg als Befund. Finden: Aktionssuche, Menue des Stapels.
//   cwmodellhb  die Bilder fuer das Handbuch (h_cwmodell_*; BHED_FOTOS)
//   BHED_CWMODELL_FENSTER=2538x1472@1.0   Fenster und Oberflaeche (Vorgabe wie die Handbuchbilder von CW-IMPORT)
// Eingebunden in selbsttest.cpp (nach selbsttest_test1411.inc).
namespace cwmodell {

void melde(const std::string& z) { diag::detail("Knopftest: CW-MODELL " + z); }
ModellTab& mtab() { return g_app->mtab; }

std::string laufOrdner() {
    const char* e = std::getenv("BHED_EINSTELLUNGEN");
    std::string d = e != nullptr ? std::string(e) + "/../cwmodell" : std::string("cwmodell");
    std::error_code ec;
    std::filesystem::create_directories(::bhed::utf8Pfad(d), ec);
    return d;
}

std::string dateiLesen(const std::string& p) {
    std::ifstream f(::bhed::utf8Pfad(p), std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

void speichernNach(const std::string& p) {
    mtab().speicherPfad = p;
    mtab().frageArt = 0;
    modellSpeichern(false);
}

int stufenEcken(int s) { return modell::stufenZahlen(mtab().dok.inhalt(), s).ecken; }

void cloneWars(bool an) { g_app->settings.karte["OptimizerCloneWars"] = an ? "1" : "0"; }

std::vector<Schritt> lodSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto original = std::make_shared<std::string>();
    auto stufenVorher = std::make_shared<int>(0);
    auto ecken = std::make_shared<std::array<int, 3>>();
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    add(tu("CW-MODELL LOD: wie 3ds Max, Bereich Modellieren, Profil aus", [=] {
        cloneWars(false);
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Modellieren;
        mtab().lodWahl = 0;
        modellLodZeigen();   // ab Werk zugeklappt
        (void)readFromArchives("models/players/kyle/model.glm", *original);
        *stufenVorher = mtab().dok.inhalt().stufenZahl;
        melde("kyle: " + std::to_string(*stufenVorher) + " Stufen, " + std::to_string(stufenEcken(0)) + " Ecken");
    }));
    add(pause(0.8));
    add(pruefe("CW-MODELL LOD: Gruppe ProOptimizer im Bereich Modellieren gezeichnet (Tabelle mit den Stufen der Datei)", [=] {
        const ModellLod& l = modellLod();
        melde("Gruppe Bild " + std::to_string(l.probeBild) + ", Zeilen " + std::to_string(l.probeZeilen.size()));
        return l.probeBild > 0 && static_cast<int>(l.probeZeilen.size()) == *stufenVorher;
    }));
    add(pruefe("CW-MODELL LOD: Speichern ohne Erzeugen schreibt die Datei des Spiels Byte fuer Byte", [=] {
        const std::string p = laufOrdner() + "/kyle_ohne.glm";
        speichernNach(p);
        const std::string b = dateiLesen(p);
        melde("ohne Erzeugen " + std::to_string(b.size()) + " Bytes, Datei " + std::to_string(original->size()));
        return !b.empty() && b == *original;
    }));
    add(pruefe("CW-MODELL LOD: Erzeugen - drei Stufen, die Ansicht zeigt die letzte, Ecken fallen", [=] {
        const bool ok = modellLodErzeugen();
        for (int i = 0; i < 3; ++i) { (*ecken)[static_cast<std::size_t>(i)] = stufenEcken(i); }
        melde("Erzeugen " + std::string(ok ? "ja" : "nein") + ", Ecken " + std::to_string((*ecken)[0]) + "/" + std::to_string((*ecken)[1]) + "/" +
              std::to_string((*ecken)[2]) + ", Wahl " + std::to_string(mtab().lodWahl) + ", Meldung " + mtab().meldung);
        return ok && mtab().dok.inhalt().stufenZahl == 3 && mtab().lodWahl == 3 && (*ecken)[1] < (*ecken)[0] && (*ecken)[2] < (*ecken)[1];
    }));
    add(pause(0.6));
    add(pruefe("CW-MODELL LOD: gezeichnet wird Stufe 3, die Tabelle zeigt drei Zeilen mit vorher -> nachher", [] {
        const ModellLod& l = modellLod();
        std::string z;
        for (const std::string& x : l.probeZeilen) { z += x + "; "; }
        melde("gezeichnet " + std::to_string(mtab().lodJetzt + 1) + ", Tabelle " + z);
        return mtab().lodJetzt == 2 && l.probeZeilen.size() == 3U;
    }));
    add(pruefe("CW-MODELL LOD: Rueckgaengig bringt die Stufen der Datei, Wiederholen die neuen", [=] {
        const bool z = mtab().dok.rueckgaengig();
        const int vorher = mtab().dok.inhalt().stufenZahl;
        const bool w = mtab().dok.wiederholen();
        return z && w && vorher == *stufenVorher && mtab().dok.inhalt().stufenZahl == 3 && stufenEcken(2) == (*ecken)[2];
    }));
    add(pruefe("CW-MODELL LOD: gespeichert liest die .glm drei Stufen mit den Ecken des Dokuments", [=] {
        const std::string p = laufOrdner() + "/kyle_lod.glm";
        speichernNach(p);
        const std::string b = dateiLesen(p);
        bool ok = !b.empty();
        for (int st = 0; st < 3 && ok; ++st) {
            GlmModel m;
            ok = readGlm(b, m, nullptr, st) && m.numLods == 3;
            int e = 0;
            for (const GlmSurface& f : m.surfaces) {
                if (!f.isTag() && !f.isOffByName()) { e += static_cast<int>(f.verts.size()); }
            }
            ok = ok && e == (*ecken)[static_cast<std::size_t>(st)];
        }
        melde("mit Stufen " + std::to_string(b.size()) + " Bytes");
        return ok;
    }));
    add(pruefe("CW-MODELL LOD: ohne Pruefprofil kein Richtwert", [] {
        return std::none_of(modellLodBefunde().begin(), modellLodBefunde().end(), [](const modell::StufenBefund& b) { return b.art == modell::StufenBefundArt::Richtwert; });
    }));
    add(pruefe("CW-MODELL LOD: mit Pruefprofil Clone-Wars-Kette senkt der Richtwert die letzte Stufe, darueber ein Befund", [=] {
        cloneWars(true);
        const bool ok = modellLodErzeugen();
        const int letzte = stufenEcken(2);
        const auto b = modellLodBefunde();
        const bool befund = std::any_of(b.begin(), b.end(), [](const modell::StufenBefund& x) { return x.art == modell::StufenBefundArt::Richtwert; });
        melde("Richtwert: letzte Stufe " + std::to_string(letzte) + " (ohne " + std::to_string((*ecken)[2]) + "), Befund " + (befund ? "ja" : "nein"));
        cloneWars(false);
        return ok && letzte < (*ecken)[2] && befund == (letzte > 500);
    }));
    add(tu("CW-MODELL LOD: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(0.6));
    add(pruefe("CW-MODELL LOD: wie Blender im Reiter Modifier gezeichnet", [] { return modellLod().probeBild > ImGui::GetFrameCount() - 5; }));
    add(pruefe("CW-MODELL LOD: Aktionssuche und Menue nennen die Detailstufen", [] {
        bool gefunden = false;
        ImGui::Begin("##cwsuche", nullptr, ImGuiWindowFlags_NoSavedSettings);
        gefunden = !modellLodAktionenSuche("decimate");   // nichts ausgeloest
        ImGui::End();
        return gefunden;
    }));
    add(tu("CW-MODELL LOD: wie 3ds Max", [] { modellBedienungSetzen(0); }));
    return s;
}

std::vector<Schritt> md3Schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto bilder = std::make_shared<int>(0);
    for (Schritt& x : mb1::laden("models/map_objects/hoth/bacta_tank.md3")) { add(std::move(x)); }
    add(tu("CW-MODELL MD3: Ansehen/Pruefen, bacta_tank mit bacta_tank_1.md3 des Spiels", [] {
        mtab().bereich = ModellBereich::Ansehen;
        mtab().lodWahl = 0;
    }));
    add(pause(0.8));
    add(pruefe("CW-MODELL MD3: \"Detailstufe und Skin\" nennt die Datei bacta_tank_1.md3 daneben", [] {
        std::string z;
        for (const std::string& x : modellLod().probeZeilen) { z += x + "; "; }
        melde("bacta_tank: " + z);
        return modellLod().probeZeilen.size() == 2U && z.find("bacta_tank_1.md3") != std::string::npos;
    }));
    for (Schritt& x : mb1::laden("models/map_objects/danger/ship_part03.md3")) { add(std::move(x)); }
    add(pruefe("CW-MODELL MD3: Erzeugen und Speichern schreibt ship_part03_1.md3 und _2.md3 mit allen Bildern", [=] {
        std::string bytes;
        Md3Model alt;
        (void)readFromArchives("models/map_objects/danger/ship_part03.md3", bytes);
        (void)readMd3(bytes, alt);
        *bilder = alt.numFrames;
        mtab().bereich = ModellBereich::Modellieren;
        const bool ok = modellLodErzeugen();
        const std::string p = laufOrdner() + "/ship_part03.md3";
        std::error_code ec;
        std::filesystem::remove(::bhed::utf8Pfad(modell::md3StufenPfad(p, 1)), ec);
        std::filesystem::remove(::bhed::utf8Pfad(modell::md3StufenPfad(p, 2)), ec);
        speichernNach(p);
        bool gut = ok;
        for (int st = 1; st < 3; ++st) {
            Md3Model m;
            const std::string d = dateiLesen(modell::md3StufenPfad(p, st));
            gut = gut && readMd3(d, m) && m.numFrames == alt.numFrames && modell::stufenZahlen(m).ecken == stufenEcken(st) &&
                  modell::stufenZahlen(m).ecken < modell::stufenZahlen(alt).ecken;
            melde("Stufe " + std::to_string(st + 1) + ": " + std::to_string(d.size()) + " Bytes, " + std::to_string(modell::stufenZahlen(m).ecken) + " Ecken");
        }
        Md3Model null;
        gut = gut && readMd3(dateiLesen(p), null) && modell::stufenZahlen(null).ecken == modell::stufenZahlen(alt).ecken;
        melde("Meldung: " + mtab().meldung);
        return gut;
    }));
    // Rueckfrage Hauptchat 15.11.: ohne erzeugte Stufen schreibt Speichern keine _1/_2, vorhandene (Ravens) bleiben unberuehrt; sind Stufen
    // erzeugt und liegen _1/_2 schon daneben, fragt das Programm vor dem Ueberschreiben (im Programm, kein Windows-Dialog)
    auto fremd1 = std::make_shared<std::string>();
    auto fremd2 = std::make_shared<std::string>("CW-MODELL fremde Datei _2");
    for (Schritt& x : mb1::laden("models/map_objects/danger/ship_part03.md3")) { add(std::move(x)); }
    add(pruefe("CW-MODELL MD3: ohne erzeugte Stufen schreibt Speichern keine name_1.md3 / name_2.md3", [] {
        const std::string d = laufOrdner() + "/ohne";
        std::error_code ec;
        std::filesystem::remove_all(::bhed::utf8Pfad(d), ec);
        std::filesystem::create_directories(::bhed::utf8Pfad(d), ec);
        speichernNach(d + "/ship_part03.md3");
        const bool da1 = std::filesystem::exists(::bhed::utf8Pfad(d + "/ship_part03_1.md3"), ec);
        const bool da2 = std::filesystem::exists(::bhed::utf8Pfad(d + "/ship_part03_2.md3"), ec);
        melde(std::string("ohne Stufen: _1 ") + (da1 ? "da" : "nicht da") + ", _2 " + (da2 ? "da" : "nicht da") + ", Stufen " +
              std::to_string(mtab().dok.inhalt().stufenZahl));
        return mtab().dok.inhalt().stufenZahl == 1 && std::filesystem::exists(::bhed::utf8Pfad(d + "/ship_part03.md3"), ec) && !da1 && !da2;
    }));
    add(pruefe("CW-MODELL MD3: vorhandene name_1.md3 / name_2.md3 (Ravens) bleiben ohne erzeugte Stufen Byte fuer Byte", [=] {
        const std::string d = laufOrdner() + "/raven";
        std::error_code ec;
        std::filesystem::remove_all(::bhed::utf8Pfad(d), ec);
        std::filesystem::create_directories(::bhed::utf8Pfad(d), ec);
        (void)readFromArchives("models/map_objects/hoth/bacta_tank_1.md3", *fremd1);
        std::ofstream(::bhed::utf8Pfad(d + "/ship_part03_1.md3"), std::ios::binary) << *fremd1;
        std::ofstream(::bhed::utf8Pfad(d + "/ship_part03_2.md3"), std::ios::binary) << *fremd2;
        speichernNach(d + "/ship_part03.md3");
        return !fremd1->empty() && dateiLesen(d + "/ship_part03_1.md3") == *fremd1 && dateiLesen(d + "/ship_part03_2.md3") == *fremd2;
    }));
    add(pruefe("CW-MODELL MD3: Stufen erzeugt, _1/_2 liegen schon daneben - vor dem Ueberschreiben fragt das Programm, die Dateien bleiben bis dahin", [=] {
        const std::string d = laufOrdner() + "/raven";
        const bool ok = modellLodErzeugen();
        speichernNach(d + "/ship_part03.md3");
        const bool unberuehrt = dateiLesen(d + "/ship_part03_1.md3") == *fremd1 && dateiLesen(d + "/ship_part03_2.md3") == *fremd2;
        melde("Frage " + std::to_string(mtab().frageArt) + ", Zeilen " + std::to_string(mtab().frageZeilen.size()) + (unberuehrt ? ", Dateien unberuehrt" : ", Dateien GEAENDERT"));
        return ok && mtab().frageArt != 0 && unberuehrt;
    }));
    add(warteBis("CW-MODELL MD3: die Frage im Programm ist offen", [] { return g_app->mtab.frageSichtbar; }, 300));
    add(klick(tr(Str::MbSpTrotzdem), "###mbfrage"));
    add(pause(0.4));
    add(pruefe("CW-MODELL MD3: nach \"Trotzdem\" stehen die erzeugten Stufen in _1/_2 (alle Bilder)", [=] {
        const std::string d = laufOrdner() + "/raven";
        Md3Model m1;
        Md3Model m2;
        const bool ok = readMd3(dateiLesen(d + "/ship_part03_1.md3"), m1) && readMd3(dateiLesen(d + "/ship_part03_2.md3"), m2);
        return ok && modell::stufenZahlen(m1).ecken == stufenEcken(1) && modell::stufenZahlen(m2).ecken == stufenEcken(2) && !g_app->mtab.frageSichtbar;
    }));
    return s;
}

std::vector<Schritt> boltSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    add(tu("CW-MODELL Bolts: Bereich Modellieren, wie 3ds Max", [] {
        cloneWars(false);
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Modellieren;
        modellCw().fahrzeug = false;
        modellCwZeigen(0);   // ab Werk zugeklappt
    }));
    add(pause(0.6));
    add(pruefe("CW-MODELL Bolts: *driver an pelvis angelegt, ohne Markierung und Profil kein Hinweis", [] {
        ModellCw& c = modellCw();
        std::snprintf(c.boltName, sizeof c.boltName, "*driver");
        c.boltKnochen = boneIndex(g_app->anim, "pelvis");
        for (int k = 0; k < 3; ++k) {
            c.boltOrt[k] = c.boltKnochen >= 0 ? g_app->anim.bones[static_cast<std::size_t>(c.boltKnochen)].basePose.m[k][3] : 0.0F;
            c.boltWinkel[k] = 0.0F;
        }
        const bool ok = modellBoltAnlegen();
        const int t = mtab().dok.inhalt().teilIndex("*driver");
        melde("Bolt " + std::string(ok ? "ja" : "nein") + ", Teil " + std::to_string(t) + ", Hinweise " + std::to_string(modellBoltHinweise().size()));
        return ok && t >= 0 && mtab().dok.inhalt().teile[static_cast<std::size_t>(t)].istTag() && modellBoltHinweise().empty();
    }));
    add(pruefe("CW-MODELL Bolts: als Fahrzeug markiert fehlt die Muendung, mit *muzzle1 kein Hinweis", [] {
        modellCw().fahrzeug = true;
        const std::size_t vorher = modellBoltHinweise().size();
        std::snprintf(modellCw().boltName, sizeof modellCw().boltName, "*muzzle1");
        modellCw().boltOrt[0] += 20.0F;
        const bool ok = modellBoltAnlegen();
        const std::size_t nachher = modellBoltHinweise().size();
        modellCw().fahrzeug = false;
        return vorher == 1U && ok && nachher == 0U;
    }));
    add(pause(0.5));
    add(pruefe("CW-MODELL Bolts: die Liste der Gruppe zeigt beide, gespeichert stehen sie in der .glm", [] {
        std::string z;
        for (const std::string& x : modellCw().probeBolts) { z += x + "; "; }
        melde("Bolts: " + z);
        const std::string p = laufOrdner() + "/kyle_bolts.glm";
        speichernNach(p);
        GlmModel m;
        const bool ok = readGlm(dateiLesen(p), m) && surfaceIndex(m, "*driver") >= 0 && surfaceIndex(m, "*muzzle1") >= 0;
        return ok && z.find("*driver") != std::string::npos && z.find("*muzzle1") != std::string::npos;
    }));
    add(pruefe("CW-MODELL Bolts: Rueckgaengig nimmt *muzzle1 und *driver zurueck", [] {
        const bool a = mtab().dok.rueckgaengig();
        const bool b = mtab().dok.rueckgaengig();
        return a && b && mtab().dok.inhalt().teilIndex("*driver") < 0;
    }));
    return s;
}

std::vector<Schritt> skelettSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto glaVorher = std::make_shared<std::string>();
    for (Schritt& x : mb1::laden("models/weapons2/blaster_r/blaster_w.glm")) { add(std::move(x)); }
    add(tu("CW-MODELL Skelett: Bereich Animation, wie 3ds Max", [=] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Animation;
        *glaVorher = g_app->anim.skeletonName;
        melde("blaster_w: GLA " + g_app->anim.skeletonName + ", " + std::to_string(g_app->anim.bones.size()) + " Knochen");
    }));
    add(pause(0.6));
    add(pruefe("CW-MODELL Skelett: ein Skelett aus einer Datei bleibt (keine Knochen anlegbar)", [] { return !modellCwEigenesSkelett() && !modellKnochenAnlegen(); }));
    add(pruefe("CW-MODELL Skelett: Neues Skelett cw_test - model_root, Kopf models/players/cw_test/cw_test", [] {
        std::snprintf(modellCw().skelettName, sizeof modellCw().skelettName, "cw_test");
        const bool ok = modellSkelettNeu();
        return ok && modellCwEigenesSkelett() && mtab().dok.inhalt().glaPfad == "models/players/cw_test/cw_test";
    }));
    add(pause(0.3));
    add(pruefe("CW-MODELL Skelett: die Anzeige nimmt das eigene Skelett (1 Knochen, 1 Bild)", [] {
        return g_app->anim.bones.size() == 1U && g_app->anim.numFrames == 1 && g_app->anim.skeletonName == "models/players/cw_test/cw_test";
    }));
    add(pruefe("CW-MODELL Skelett: Knochen body und barrel, Flaeche 0 starr an barrel", [] {
        ModellCw& c = modellCw();
        std::snprintf(c.knochenName, sizeof c.knochenName, "body");
        c.knochenEltern = 0;
        c.knochenOrt[0] = c.knochenOrt[1] = c.knochenOrt[2] = 0.0F;
        const bool a = modellKnochenAnlegen();
        std::snprintf(c.knochenName, sizeof c.knochenName, "barrel");
        c.knochenEltern = 1;
        c.knochenOrt[0] = 10.0F;
        const bool b = modellKnochenAnlegen();
        int erste = -1;
        for (std::size_t i = 0; i < mtab().dok.inhalt().teile.size() && erste < 0; ++i) {
            if (!mtab().dok.inhalt().teile[i].istTag()) { erste = static_cast<int>(i); }
        }
        mtab().auswahl = {erste};
        c.knochenGewaehlt = 2;
        const bool f = modellFlaechenAnKnochen();
        const modell::Inhalt& in = mtab().dok.inhalt();
        melde("Flaeche " + std::to_string(erste) + ": " + mtab().meldung);
        return a && b && f && erste >= 0 && in.knochenZahl == 3 && in.teile[static_cast<std::size_t>(erste)].stufen[0].ecken[0].knochen[0] == 2;
    }));
    add(pruefe("CW-MODELL Skelett: Sequenz BOTH_WALK1 (10 Bilder) - Liste und Bilder", [] {
        ModellCw& c = modellCw();
        std::snprintf(c.sequenzName, sizeof c.sequenzName, "BOTH_WALK1");
        c.sequenzBilder = 10;
        c.sequenzFps = 15;
        const bool ok = modellSequenzAnlegen();
        const modell::Inhalt& in = mtab().dok.inhalt();
        return ok && in.skelett && in.skelett->numFrames == 11 && in.cfg.zeileVon("BOTH_WALK1") >= 0;
    }));
    add(pause(0.3));
    add(pruefe("CW-MODELL Skelett: die Zeitleiste kennt BOTH_WALK1", [] {
        return std::any_of(g_app->animList.begin(), g_app->animList.end(), [](const AnimEntry& e) { return e.name == "BOTH_WALK1" && e.numFrames == 10; });
    }));
    add(pruefe("CW-MODELL Skelett: ein Name, den das Spiel nicht kennt, ist ein Befund der animation.cfg (kein stilles Verwerfen)", [] {
        std::snprintf(modellCw().sequenzName, sizeof modellCw().sequenzName, "ATRT_HOP");
        modellCw().sequenzBilder = 4;
        (void)modellSequenzAnlegen();
        const auto b = modellCfgBefunde();
        const bool befund = std::any_of(b.begin(), b.end(), [](const modell::CfgBefund& x) { return x.art == modell::CfgBefundArt::Unbekannt && x.name == "ATRT_HOP"; });
        const bool still = std::none_of(b.begin(), b.end(), [](const modell::CfgBefund& x) { return x.art == modell::CfgBefundArt::Unbekannt && x.name == "BOTH_WALK1"; });
        melde(std::string("Befund ATRT_HOP ") + (befund ? "ja" : "nein") + ", " + std::to_string(b.size()) + " Befunde");
        return befund && still;
    }));
    add(pruefe("CW-MODELL Skelett: GLA, GLM und animation.cfg geschrieben und gelesen", [] {
        const std::string d = laufOrdner();
        const bool g = modellGlaSpeichern(d + "/cw_test.gla");
        const bool c = modellCfgSpeichern(d + "/animation.cfg");
        speichernNach(d + "/cw_test_model.glm");
        GlaAnimation a;
        GlmModel m;
        const modell::AnimCfg cfg = modell::animCfgLesen(dateiLesen(d + "/animation.cfg"));
        const bool ok = g && c && readGla(dateiLesen(d + "/cw_test.gla"), a) && a.bones.size() == 3U && a.numFrames == 15 &&
                        readGlm(dateiLesen(d + "/cw_test_model.glm"), m) && m.animFile == "models/players/cw_test/cw_test" && m.numBones == 3 &&
                        cfg.zeileVon("BOTH_WALK1") >= 0;
        melde("GLA " + std::to_string(a.bones.size()) + " Knochen, " + std::to_string(a.numFrames) + " Bilder; GLM " + m.animFile);
        return ok;
    }));
    add(pruefe("CW-MODELL Skelett: Rueckgaengig bis vor Neues Skelett - wieder die GLA der Datei", [=] {
        for (int i = 0; i < 6; ++i) { (void)mtab().dok.rueckgaengig(); }
        return !modellCwEigenesSkelett();
    }));
    add(pause(0.3));
    add(pruefe("CW-MODELL Skelett: die Anzeige hat wieder das Skelett der Datei", [=] { return g_app->anim.skeletonName == *glaVorher; }));
    return s;
}

// Die Bilder fuer das Handbuch (BHED_MODELL_NUR=cwmodellhb, BHED_FOTOS)
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // die Oberflaeche spricht die Sprache des Laufs - sonst landen deutsche Bilder unter zh (der erste Bildlauf setzte language=zh,
    // das Programm kennt nur zh-Hans und nahm die Sprache des Systems)
    add(pruefe("CW-MODELL Handbuch: die Oberflaeche spricht die Sprache des Laufs (BHED_SPRACHE)", [] {
        const char* sp = std::getenv("BHED_SPRACHE");
        if (sp == nullptr || *sp == '\0') { return true; }
        const i18n::LanguageInfo* l = i18n::findLanguage(sp);
        melde(std::string("Sprache des Laufs ") + sp + ", Oberflaeche " + i18n::kurzCode(i18n::currentLanguage()));
        return l != nullptr && l->language == i18n::currentLanguage();
    }));
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    for (int modus = 0; modus < 2; ++modus) {
        add(tu(std::string("CW-MODELL Handbuch: Detailstufen ") + (modus == 0 ? "wie 3ds Max" : "wie Blender"), [=] {
            cloneWars(false);
            modellBedienungSetzen(modus);
            mtab().bereich = ModellBereich::Modellieren;
            if (mtab().dok.inhalt().stufenZahl != 3 || modellLod().berichtFuer.empty()) { (void)modellLodErzeugen(); }
            modellLodZeigen();
        }));
        add(pause(1.0));
        // die Gruppe steht nach den JKA-Grenzen am Ende des Felds: ganz hinunter rollen, damit Erzeugen und die Tabelle im Bild sind
        // (das erste Bild wie Blender zeigte nur die Kopfzeile und die Regler)
        add(tu("CW-MODELL Handbuch: Feld ans Ende rollen", [] {
            if (ImGuiWindow* fw = mbefoto::kindFenster("mtrollouts")) { ImGui::SetScrollY(fw, fw->ScrollMax.y); }
        }));
        add(pause(0.6));
        add(foto(modus == 0 ? "h_cwmodell_lod_max" : "h_cwmodell_lod_blender"));
        add(hb::keinLaufpfad(modus == 0 ? "h_cwmodell_lod_max" : "h_cwmodell_lod_blender", [] { return g_app->modelPath + " | " + mtab().meldung; }));
    }
    add(tu("CW-MODELL Handbuch: Detailstufe und Skin (Ansehen)", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Ansehen;
        mtab().lodWahl = 2;
        // das Feld behielt die Rollung des Bereichs Modellieren (ans Ende gerollt) - die Gruppe "Detailstufe und Skin" ganz ins Bild
        if (ImGuiWindow* fw = mbefoto::kindFenster("mtrollouts")) { ImGui::SetScrollY(fw, 0.0F); }
        ui::rolloutZeigen(tr(Str::MbRLod));
    }));
    add(pause(1.0));
    add(foto("h_cwmodell_stufen"));
    add(hb::keinLaufpfad("h_cwmodell_stufen", [] { return g_app->modelPath + " | " + mtab().meldung; }));
    add(tu("CW-MODELL Handbuch: Bolts", [] {
        mtab().bereich = ModellBereich::Modellieren;
        mtab().lodWahl = 0;
        ModellCw& c = modellCw();
        std::snprintf(c.boltName, sizeof c.boltName, "*driver");
        c.boltKnochen = boneIndex(g_app->anim, "pelvis");
        for (int k = 0; k < 3; ++k) { c.boltOrt[k] = c.boltKnochen >= 0 ? g_app->anim.bones[static_cast<std::size_t>(c.boltKnochen)].basePose.m[k][3] : 0.0F; }
        (void)modellBoltAnlegen();
        c.fahrzeug = true;
        modellCwZeigen(0);
    }));
    add(pause(1.2));
    add(foto("h_cwmodell_bolts"));
    add(hb::keinLaufpfad("h_cwmodell_bolts", [] { return g_app->modelPath + " | " + mtab().meldung; }));
    for (Schritt& x : mb1::laden("models/weapons2/blaster_r/blaster_w.glm")) { add(std::move(x)); }
    add(tu("CW-MODELL Handbuch: eigenes Skelett", [] {
        modellCw().fahrzeug = false;
        mtab().bereich = ModellBereich::Animation;
        std::snprintf(modellCw().skelettName, sizeof modellCw().skelettName, "cw_atrt");
        (void)modellSkelettNeu();
        ModellCw& c = modellCw();
        std::snprintf(c.knochenName, sizeof c.knochenName, "body");
        c.knochenEltern = 0;
        (void)modellKnochenAnlegen();
        std::snprintf(c.knochenName, sizeof c.knochenName, "leg_l");
        c.knochenOrt[1] = 8.0F;
        (void)modellKnochenAnlegen();
        std::snprintf(c.sequenzName, sizeof c.sequenzName, "BOTH_WALK1");
        (void)modellSequenzAnlegen();
        modellCwZeigen(1);
    }));
    add(pause(1.2));
    add(foto("h_cwmodell_skelett"));
    add(hb::keinLaufpfad("h_cwmodell_skelett", [] { return g_app->modelPath + " | " + mtab().meldung; }));
    return s;
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "cwmodell" && n != "cwmodellhb") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 2538;
    int h = 1472;
    float z = 1.0F;
    if (const char* f = std::getenv("BHED_CWMODELL_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    add(tu("CW-MODELL: Fenster, Skript fuer den Test", [=] {
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "cwmodell.txt");
    }));
    add(pause(0.8));
    add(tu("CW-MODELL: Aufteilung wie neue Einstellungen", [] {
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        g_app->mtab.vierAnsichten = false;
    }));
    if (n == "cwmodellhb") {
        for (Schritt& x : handbuchSchritte()) { add(std::move(x)); }
        return s;
    }
    for (Schritt& x : lodSchritte()) { add(std::move(x)); }
    for (Schritt& x : md3Schritte()) { add(std::move(x)); }
    for (Schritt& x : boltSchritte()) { add(std::move(x)); }
    for (Schritt& x : skelettSchritte()) { add(std::move(x)); }
    add(tu("CW-MODELL: Profil aus", [] { cloneWars(false); }));
    return s;
}

}  // namespace cwmodell
