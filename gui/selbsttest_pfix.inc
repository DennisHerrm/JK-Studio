// selbsttest_pfix.inc - Funde der Code-Pruefung Modell-Block (plaene/codepruefung_modellblock.md) im laufenden Programm
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=pfix | pfixkarte)
//
//   pfix       H1  Werkzeugkaestchen offen, Strg+Z, Esc: der Schritt davor bleibt; ein Befehl bei offenem Kaestchen, Esc:
//                  der Befehl bleibt
//              H2  .glm (Sturmtruppler): eine Aenderung, Detach in einen neuen Teil - das Dokument behaelt seine Schritte,
//                  auch nach Rueckgaengig
//              H3  ungespeichertes Modell: das Kreuz (confirmQuit) und "Neues Requisit" fragen; Nein laesst alles stehen
//   pfixkarte  H4  zwei neue Requisite mit demselben Namen "In die Karte": zwei Dateien, die erste bleibt
// Eingebunden in selbsttest.cpp (nach selbsttest_md.inc).
namespace pfix {

namespace w = modell::wof;
namespace ke = karteneditor;

ModellMesh& mz() { return g_app->mtab.mesh; }

struct Stand {
    std::string datei;
    std::string undo;
    std::size_t teile = 0;
    std::string vfs;
    int antwort = -1;
    float wert = 0.0F;
};

std::vector<Schritt> modellSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<Stand>();
    // --- H1 ---
    md1d::neuesRequisit(s, "pf_kiste", "");
    md1d::kasten(s, 64.0F);
    add(tu("PF H1: Extrude A uebernommen, Extrude B offen", [st] {
        g_app->mtab.fokus = true;
        md1d::waehle(w::PAuswahl{{}, {}, {md1d::vieleckBei(2, 64.0F)}}, AuswahlArt::Dreieck);
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Extrude);
        mz().betrag = 16.0F;
        modellMeshVorschau();
        modellMeshUebernehmen(false);
        st->datei = md1d::md3Jetzt();
        st->undo = g_app->mtab.dok.rueckgaengigName();
        md1d::waehle(w::PAuswahl{{}, {}, {md1d::vieleckBei(2, 80.0F)}}, AuswahlArt::Dreieck);
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Extrude);
        mz().betrag = 8.0F;
        modellMeshVorschau();
    }));
    add(pause(0.3));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.3));
    add(pruefe("PF H1: Kaestchen offen, Strg+Z, Esc - Extrude A bleibt (Datei wie nach A, Rueckgaengig nennt A), Kaestchen zu", [st] {
        const std::string jetzt = md1d::md3Jetzt();
        diag::detail("Knopftest: PF H1 - Rueckgaengig \"" + g_app->mtab.dok.rueckgaengigName() + "\" (A: \"" + st->undo + "\"), Datei " +
                     (jetzt == st->datei ? "wie nach A" : "anders") + ", Kaestchen " + (mz().kaestchenOffen ? "offen" : "zu") + ", " + md1d::zahlen());
        return jetzt == st->datei && g_app->mtab.dok.rueckgaengigName() == st->undo && !mz().kaestchenOffen;
    }));
    add(tu("PF H1: Extrude C offen, dann Delete (ohne Kaestchen)", [] {
        md1d::waehle(w::PAuswahl{{}, {}, {md1d::vieleckBei(2, 80.0F)}}, AuswahlArt::Dreieck);
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Extrude);
        mz().betrag = 8.0F;
        modellMeshVorschau();
    }));
    add(pause(0.3));
    add(md1d::ausloesen(Str::MdDelete));
    add(pause(0.3));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.3));
    add(pruefe("PF H1: Befehl bei offenem Kaestchen, Esc - der Befehl (Delete) bleibt oben, Kaestchen zu", [] {
        diag::detail("Knopftest: PF H1 Befehl - Rueckgaengig \"" + g_app->mtab.dok.rueckgaengigName() + "\", Kaestchen " + (mz().kaestchenOffen ? "offen" : "zu") +
                     ", " + md1d::zahlen());
        return g_app->mtab.dok.rueckgaengigName() == tr(Str::MdDelete) && !mz().kaestchenOffen;
    }));
    // --- M6 ---
    md1d::neuesRequisit(s, "pf_m6", "");
    md1d::kasten(s, 64.0F);
    add(tu("PF M6: obere Vorderkante (zwei Punkte) mit G um 16 in Y (G, Y, 16, Enter)", [] {
        md1d::waehle(w::PAuswahl{{md1d::punktBei(32, -32, 64), md1d::punktBei(-32, -32, 64)}, {}, {}}, AuswahlArt::Ecke);
        // G nimmt die Ecken des Model-Tabs (eckenWahl, sonst beim Klicken aus der Netz-Auswahl abgeglichen): jede Ecke der zwei Punkte
        g_app->mtab.eckenWahl.clear();
        const modell::Netz& n = g_app->mtab.dok.inhalt().teile[0].stufen[0];
        for (std::size_t i = 0; i < n.ecken.size(); ++i) {
            const float* o = n.ecken[i].ort;
            if (std::fabs(std::fabs(o[0]) - 32.0F) + std::fabs(o[1] + 32.0F) + std::fabs(o[2] - 64.0F) < 1e-3F) { g_app->mtab.eckenWahl.push_back(modell::EckOrt{0, static_cast<int>(i)}); }
        }
        diag::detail("Knopftest: PF M6 - " + std::to_string(g_app->mtab.eckenWahl.size()) + " Ecken gewaehlt, G " + (modellModalStarten(1) ? "laeuft" : "nicht gestartet"));
        g_app->mtab.modal.achse = 1;
        g_app->mtab.modal.zahl = "16";
    }));
    add(pause(0.3));
    add(tu("PF M6: G uebernehmen", [] { modellModalEnde(true); }));
    add(pause(0.3));
    add(pruefe("PF M6: die Vorderseite ist geneigt - die Normale ihrer unteren Ecke folgt (0, -0.97, 0.24), wie Max/Blender", [] {
        const modell::Inhalt& in = g_app->mtab.dok.inhalt();
        float n[3] = {0.0F, 0.0F, 0.0F};
        if (!in.teile.empty() && !in.teile[0].stufen.empty()) {
            for (const modell::Ecke& e : in.teile[0].stufen[0].ecken) {
                if (std::fabs(e.ort[0] - 32.0F) + std::fabs(e.ort[1] + 32.0F) + std::fabs(e.ort[2]) < 1e-3F && e.normale[1] < -0.5F) { std::memcpy(n, e.normale, sizeof n); }
            }
        }
        char b[160];
        std::snprintf(b, sizeof b, "Knopftest: PF M6 - Normale %.3f %.3f %.3f", static_cast<double>(n[0]), static_cast<double>(n[1]), static_cast<double>(n[2]));
        diag::detail(b);
        return std::fabs(n[1] + 0.970F) < 0.01F && std::fabs(n[2] - 0.243F) < 0.01F;
    }));
    // --- H2 ---
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    add(tu("PF H2: .glm - eine Aenderung (Name), Bereich Modellieren, ein Polygon von Teil 0, Detach in einen neuen Teil", [st] {
        ModellTab& t = g_app->mtab;
        t.gespeichertStand = -1;
        (void)t.dok.aendern("pfix", [](modell::Inhalt& x) { x.name += "_pf"; });
        t.bereich = ModellBereich::Modellieren;
        st->teile = t.dok.inhalt().teile.size();
        modellMeshNetzAktualisieren();
        int f = -1;
        for (std::size_t i = 0; f < 0 && i < mz().netz.flaechen.size(); ++i) {
            const w::PFlaeche& pf = mz().netz.flaechen[i];
            const auto teil = static_cast<std::size_t>(pf.teil);
            if (!pf.weg && teil < t.dok.inhalt().teile.size() && (t.dok.inhalt().teile[teil].flaggen & GlmSurface::kFlagBolt) == 0U) { f = static_cast<int>(i); }
        }
        md1d::waehle(w::PAuswahl{{}, {}, {f}}, AuswahlArt::Dreieck);
        mz().art = 1;
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Detach);
    }));
    add(pause(0.5));
    add(pruefe("PF H2: Vorschau mit neuem Teil - das Dokument behaelt die Aenderung (Name, Rueckgaengig)", [st] {
        const modell::Dokument& d = g_app->mtab.dok;
        diag::detail("Knopftest: PF H2 Vorschau - Teile " + std::to_string(d.inhalt().teile.size()) + " (vorher " + std::to_string(st->teile) + "), Name " +
                     d.inhalt().name + ", Rueckgaengig \"" + d.rueckgaengigName() + "\", Flaechen des Modells " + std::to_string(g_app->model.surfaces.size()));
        return d.inhalt().teile.size() == st->teile + 1U && d.inhalt().name.find("_pf") != std::string::npos && d.kannRueckgaengig();
    }));
    add(tu("PF H2: Uebernehmen", [] { modellMeshUebernehmen(false); }));
    add(pause(0.3));
    add(tu("PF H2: Rueckgaengig (Detach zurueck)", [] { (void)modellRueckgaengig(); }));
    add(pause(0.5));
    add(pruefe("PF H2: nach Rueckgaengig - Teile wie vorher, die Aenderung davor bleibt, Wiederholen moeglich", [st] {
        const modell::Dokument& d = g_app->mtab.dok;
        diag::detail("Knopftest: PF H2 Rueckgaengig - Teile " + std::to_string(d.inhalt().teile.size()) + ", Name " + d.inhalt().name + ", Rueckgaengig \"" +
                     d.rueckgaengigName() + "\", Wiederholen \"" + d.wiederholenName() + "\"");
        return d.inhalt().teile.size() == st->teile && d.rueckgaengigName() == "pfix" && d.kannWiederholen();
    }));
    // --- M5 ---
    add(tu("PF M5: Quelle A (_humanoid), dann Quelle B (rancor_ice_spider), Strg+Z", [st] {
        (void)modellRtQuelleLaden("models/players/_humanoid/_humanoid.gla");
        st->teile = g_app->mtab.rt.quelle.bones.size();
        (void)modellRtQuelleLaden("models/players/rancor_ice_spider/rancor_ice_spider.gla");
        (void)modellRueckgaengig();
        modellRtAktualisieren();
    }));
    add(pause(0.3));
    add(pruefe("PF M5: nach Rueckgaengig rechnet der Retarget mit der Quelle, die das Dokument nennt (A)", [st] {
        const ModellRetarget& r = g_app->mtab.rt;
        const modell::RetargetDaten* d = modellRtDaten();
        diag::detail("Knopftest: PF M5 - Dokument " + (d != nullptr ? d->quelle : std::string("-")) + ", gerechnet " + r.quelleFuer + " (" +
                     std::to_string(r.quelle.bones.size()) + " Knochen, A " + std::to_string(st->teile) + ")");
        return d != nullptr && d->quelle == r.quelleFuer && r.quelle.bones.size() == st->teile && st->teile > 0U;
    }));
    // --- H6 / H5: IK-Schluessel in der Wahl des Dope Sheets ---
    add(tu("PF H6: Bereich Animation, BOTH_WALK1, Rig, IK am rechten Arm (Schluessel im Bild), Dope Sheet offen, Wahl = IK-Schluessel", [] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Animation;
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        g_app->animPlaying = false;
        g_app->animFrame = kv5::f0();
        g_app->animTime = 0.0F;
        (void)modellRigAnlegen();
        (void)modellIkWechselnRoh(static_cast<int>(modell::IkArt::ArmRechts), true, false);
        g_app->mtab.kvOffen = true;
        g_app->mtab.kvArt = 0;
        const modell::RigSchicht& s = kv5::sch();
        g_app->mtab.kvWahl.clear();
        if (!s.ik.empty() && !s.ik[0].ziel.empty()) { g_app->mtab.kvWahl.push_back({modell::ikWahlCode(0), s.ik[0].ziel[0].bild}); }
        diag::detail("Knopftest: PF H6 - IK-Spuren " + std::to_string(s.ik.size()) + ", Wahl " + std::to_string(g_app->mtab.kvWahl.size()));
    }));
    add(pause(0.5));
    kv5::ueberSuche(s, Str::MbKvIpLinear);
    kv5::ueberSuche(s, Str::MbKvTgVektor);
    kv5::ueberSuche(s, Str::MbKvKopieren);
    kv5::ueberSuche(s, Str::MbKvWtUmkehren);
    add(pruefe("PF H6/H5: Interpolation, Tangenten, Kopieren und Umkehren mit einem IK-Schluessel in der Wahl - kein Absturz, der IK-Schluessel ist linear", [] {
        const modell::RigSchicht& s = kv5::sch();
        const bool linear = !s.ik.empty() && !s.ik[0].ziel.empty();
        diag::detail("Knopftest: PF H6 - IK-Spuren " + std::to_string(s.ik.size()) + ", Schluessel " + std::to_string(linear ? s.ik[0].ziel.size() : 0U) + ", Ablage " +
                     std::to_string(g_app->mtab.kvAblage.size()) + " | " + g_app->mtab.meldung);
        return linear && !g_app->mtab.kvWahl.empty();
    }));
    add(tu("PF H6: Dope Sheet zu, Bereich Modellieren", [] {
        g_app->mtab.kvOffen = false;
        g_app->mtab.kvWahl.clear();
    }));
    // --- M7: Regler IK-FK (Blender-Modus) ueber zehn Bilder gezogen = ein Rueckgaengig-Schritt ---
    add(tu("PF M7: Blender-Modus, Bereich Animation, Griff Hand R (Rollout IK/FK geht auf)", [] {
        modellBedienungSetzen(1);
        g_app->mtab.bereich = ModellBereich::Animation;
        g_app->animPlaying = false;
        modellRigGriffWaehlen(rig3::griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        for (ImGuiWindow* w : GImGui->Windows) {
            if (std::strstr(w->Name, "mtrollouts") != nullptr) { ImGui::SetScrollY(w, 0.0F); }
        }
    }));
    add(pause(0.4));
    add(rig3::rollenZu(tr(Str::MbIkRollout), "mtrollouts"));
    add(pause(0.3));
    add(tu("PF M7: Stand vor dem Ziehen", [st] {
        st->undo = g_app->mtab.dok.rueckgaengigName();
        st->wert = modellIkAnteil(static_cast<int>(modell::IkArt::ArmRechts));
    }));
    add(reglerZiehen("##ikfk", "", static_cast<int>(modell::IkArt::ArmRechts), 0.5F, 0.95F));
    add(pause(0.3));
    add(tu("PF M7: einmal Rueckgaengig", [] { (void)modellRueckgaengig(); }));
    add(pause(0.3));
    add(pruefe("PF M7: ein Rueckgaengig nimmt das ganze Ziehen zurueck (IK-Anteil wie vorher, Schritt davor oben)", [st] {
        const float jetzt = modellIkAnteil(static_cast<int>(modell::IkArt::ArmRechts));
        diag::detail("Knopftest: PF M7 - IK-Anteil " + std::to_string(jetzt) + " (vorher " + std::to_string(st->wert) + "), Rueckgaengig \"" +
                     g_app->mtab.dok.rueckgaengigName() + "\" (vorher \"" + st->undo + "\")");
        return std::fabs(jetzt - st->wert) < 1e-4F && g_app->mtab.dok.rueckgaengigName() == st->undo;
    }));
    add(tu("PF M7: Bedienung wie 3ds Max", [] { modellBedienungSetzen(0); }));
    // --- H3 ---
    add(tu("PF H3: Kreuz des Hauptfensters (confirmQuit) mit ungespeichertem Modell", [st] {
        g_app->settings.queryOnDiscard = false;   // nur die Frage nach dem Modell, nicht "Exit?"
        g_app->quitConfirmed = false;
        g_app->wantQuit = false;
        g_app->leftMode = 0;
        st->antwort = confirmQuit() ? 1 : 0;
    }));
    add(pause(0.3));
    add(pruefe("PF H3: das Kreuz fragt nach dem Modell, der Model-Tab ist vorn", [st] {
        diag::detail("Knopftest: PF H3 Beenden - confirmQuit " + std::to_string(st->antwort) + ", Frage " + (g_app->frageOffen ? g_app->frageText : std::string("keine")) +
                     ", Tab " + std::to_string(g_app->leftMode));
        return st->antwort == 0 && g_app->frageOffen && g_app->leftMode == 2;
    }));
    add(klick(tr(Str::AnswerNo), "###frage"));
    add(pause(0.3));
    add(pruefe("PF H3: Nein - kein Beenden, das Modell bleibt ungespeichert", [] {
        return !g_app->frageOffen && !g_app->quitConfirmed && !g_app->wantQuit && modellUngespeichert();
    }));
    add(md1d::ausloesen(Str::MdNeuesRequisit));
    add(pause(0.3));
    add(pruefe("PF H3: Neues Requisit fragt (das Modell ist ungespeichert)", [st] {
        diag::detail("Knopftest: PF H3 Neues Requisit - Frage " + (g_app->frageOffen ? g_app->frageText : std::string("keine")) + ", Modell " + g_app->modelPath);
        return g_app->frageOffen && g_app->modelPath.find("stormtrooper") != std::string::npos;
    }));
    add(klick(tr(Str::AnswerNo), "###frage"));
    add(pause(0.3));
    add(pruefe("PF H3: Nein - der Sturmtruppler mit seinen Schritten bleibt", [] {
        return !g_app->frageOffen && g_app->modelPath.find("stormtrooper") != std::string::npos && g_app->mtab.dok.kannRueckgaengig() &&
               g_app->mtab.dok.inhalt().name.find("_pf") != std::string::npos;
    }));
    add(md1d::ausloesen(Str::MdNeuesRequisit));
    add(pause(0.3));
    add(klick(tr(Str::AnswerYes), "###frage"));
    add(pause(0.3));
    add(pruefe("PF H3: Ja - ein neues Requisit, nichts mehr ungespeichert", [] {
        return !g_app->frageOffen && g_app->mtab.dok.inhalt().herkunft == modell::Herkunft::Md3 && !modellUngespeichert();
    }));
    add(tu("PF: aufraeumen", [] {
        g_app->settings.queryOnDiscard = g_queryVorher;
        g_app->quitConfirmed = false;
        g_app->wantQuit = false;
    }));
    return s;
}

std::vector<Schritt> karteSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<Stand>();
    add(tu("PF Karte: Spielordner, kein Import-Ordner", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(platform::settingsDirectory()) / "import", ec);
    }));
    add(pause(0.3));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(warteBis("PF Karte: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("PF Karte: Testraum", [] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();
        (void)stufe3::raumKarteOeffnen("m0raum", false);
        ke::modellSetzKlasse() = 0;
        ke::modellSetzKollision() = false;
    }));
    add(pause(1.0));
    md1d::neuesRequisit(s, "pf_prop", "");
    md1d::kasten(s, 32.0F);
    add(tu("PF H4: erstes Requisit In die Karte", [st] {
        (void)modellRequisitInKarte();
        st->vfs = mz().requisitVfs;
        std::string b;
        (void)modell::md3Schreiben(g_app->mtab.dok.inhalt(), b, 0);
        st->datei = b;
    }));
    add(pause(1.0));
    // derselbe Name (die Vorgabe "prop" bleibt beim Anwender ebenso stehen)
    md1d::neuesRequisit(s, "pf_prop", "");
    md1d::kasten(s, 16.0F);
    add(tu("PF H4: zweites Requisit (gleicher Name) In die Karte", [] { (void)modellRequisitInKarte(); }));
    add(pause(1.0));
    add(pruefe("PF H4: zwei Dateien - das zweite Requisit hat einen eigenen Namen, die Datei des ersten ist unveraendert", [st] {
        const std::filesystem::path imp = std::filesystem::path(platform::settingsDirectory()) / "import";
        const std::string erste = slurp((imp / st->vfs).string());
        diag::detail("Knopftest: PF H4 - erstes " + st->vfs + ", zweites " + mz().requisitVfs + ", erste Datei " + (erste == st->datei ? "unveraendert" : "anders") +
                     " | " + mz().meldung);
        return !st->vfs.empty() && mz().requisitVfs != st->vfs && erste == st->datei && std::filesystem::is_regular_file(imp / mz().requisitVfs);
    }));
    add(tu("PF Karte: aufraeumen (Karte als gespeichert, das Beenden fragt nicht)", [] { ke::editor().karte.undo.alsGespeichertMarkieren(); }));
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    if (n == "pfix") { return modellSchritte(); }
    if (n == "pfixkarte") { return karteSchritte(); }
    return {};
}

}  // namespace pfix
