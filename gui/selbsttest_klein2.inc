// selbsttest_klein2.inc - KLEIN2 (13.11.): zwei kleine Fehler, die AP14b beim Aufnehmen der Handbuchbilder sah, je Fehler erst rot,
// dann gruen (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=klein2; BHED_KLEIN2_PUNKT=esc,unter waehlt die Teile, Vorgabe beide).
//
//   esc    Esc schliesst jedes Rechtsklick-Menue des Model-Tabs wie in 3ds Max und Blender - Szenen-Explorer, Zeitleiste (wie 3ds
//          Max eine Spalte, wie Blender mit Untermenues), Dopesheet, Befehlsfeld (Modifier-Stapel), Werkzeugleiste (wie Blender),
//          die Ansicht mit offenem Untermenue und das Menue Animation der Menueleiste mit offenem Untermenue - und tut sonst nichts
//          (Auswahl, Bereich, Navigationsmodus, Tasten-Zaehler, Bild bleiben). Ohne offenes Menue wirkt Esc wie vorher: es beendet
//          einen Navigationsmodus. Vorher schloss Esc nur das Menue der Ansicht (ENTSCHEID3A C4) und einzelne Menues mit eigener
//          Zeile; das Menue des Explorers blieb offen (Foto Y:\werkzeuge\ap14b\fotos_befund\h_mb1_vier_explorermenue_offen.bmp).
//   unter  Ein Untermenue, das unten nicht mehr ins Fenster passt, geht wie ein Windows-Menue nach oben auf und bleibt im Fenster:
//          Animation > Animation tools im Hauptfenster des Selbsttests (so gross wie der Arbeitsbereich, 2538 x 1472 auf diesem
//          Rechner) ragte unten hinaus und wurde ein eigenes Fenster (Viewport) - im Programm sichtbar, auf dem Foto des
//          Hauptfensters fehlte es (h_mb5_werkzeuge_untermenue_zu.bmp). Gilt fuer alle Untermenues (ui::menue, ui_menue.h).
// Eingebunden in selbsttest.cpp (nach selbsttest_ap11.inc, benutzt die Helfer von mb1, rig3, kv5, mbefoto, ap1, ap2, ap4, ap13s).
namespace klein2 {

void melde(const std::string& z) { diag::detail("Knopftest: KLEIN2 " + z); }

ModellTab& mtab() { return g_app->mtab; }
int torso() { return mb1::teil("torso"); }
float em() { return ImGui::GetFontSize(); }

std::string rechteck(const ImRect& r) {
    char b[120];
    std::snprintf(b, sizeof b, "%.0f,%.0f..%.0f,%.0f", static_cast<double>(r.Min.x), static_cast<double>(r.Min.y), static_cast<double>(r.Max.x),
                  static_cast<double>(r.Max.y));
    return b;
}

// Was Esc ausser dem Schliessen des Menues aendern koennte: Auswahl, Bereich, Navigationsmodus, Werkzeug/Umformung, Tasten-Zaehler
// (jede ausgeloeste Taste zaehlt), Knochen, Bild
std::string stand() {
    const ModellTab& t = mtab();
    std::string s = "Auswahl";
    for (const int a : t.auswahl) { s += " " + std::to_string(a); }
    s += ", Bereich " + std::to_string(static_cast<int>(t.bereich)) + ", Nav " + std::to_string(t.navModus) + ", Tasten " + std::to_string(t.tastenZaehler) +
         ", Knochen " + std::to_string(t.knochenWahl) + ", Bild " + std::to_string(g_app->animFrame) + ", spielt " + std::to_string(g_app->animPlaying ? 1 : 0);
    return s;
}

int offeneMenues() { return GImGui->OpenPopupStack.Size; }

// Ein Fall: Menue oeffnen (Schritte von `oeffnen`), mindestens `tiefe` offene Ebenen pruefen, Esc, alles zu und nichts sonst geaendert
void escFall(std::vector<Schritt>& s, const std::string& name, int tiefe, const std::function<void(std::vector<Schritt>&)>& oeffnen) {
    auto vorher = std::make_shared<std::string>();
    oeffnen(s);
    s.push_back(pruefe("KLEIN2 esc " + name + ": das Menue ist offen (" + std::to_string(tiefe) + (tiefe > 1 ? " Ebenen)" : " Ebene)"), [=] {
        *vorher = stand();
        melde("esc " + name + ": offen " + std::to_string(offeneMenues()) + " Ebenen, " + *vorher);
        return offeneMenues() >= tiefe;
    }));
    s.push_back(tu("KLEIN2 esc " + name + ": Fokus im Model-Tab", [] { mtab().fokus = true; }));
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
    s.push_back(pause(0.4));
    s.push_back(pruefe("KLEIN2 esc " + name + ": Esc schliesst das Menue ganz (wie 3ds Max und Blender), sonst aendert sich nichts", [=] {
        const std::string jetzt = stand();
        melde("esc " + name + ": nach Esc offen " + std::to_string(offeneMenues()) + " Ebenen, vorher " + *vorher + ", nachher " + jetzt);
        return offeneMenues() == 0 && jetzt == *vorher;
    }));
    s.push_back(mbefoto::popupsZu());   // rot: das stehen gebliebene Menue faengt sonst den naechsten Klick ab
    s.push_back(pause(0.2));
}

Schritt rechtsklick(const std::string& was, std::function<ImVec2()> ort) {
    return klickAn("KLEIN2: Rechtsklick " + was, [was, ort] {
        const ImVec2 p = ort();
        if (p.x < 0.0F || p.y < 0.0F) { meldeFehler("KLEIN2: " + was + " nicht gefunden"); }
        return p;
    }, 1);
}

void figur(int modus, ModellBereich bereich) {
    modellBedienungSetzen(modus);
    mtab().bereich = bereich;
    g_app->settings.modellFeldBreite = 0.0F;
    mtab().auswahl = {torso()};
    mtab().knochenWahl = -1;
    mtab().navModus = 0;
    if (ImGuiWindow* fw = mbefoto::kindFenster("mtrollouts")) { ImGui::SetScrollY(fw, 0.0F); }
    g_app->modelDirty = true;
}

// --- esc: Esc schliesst jedes Rechtsklick-Menue des Model-Tabs -------------------------------------------------------------------

void escSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // Szenen-Explorer (wie 3ds Max): Rechtsklick auf die Zeile torso
    escFall(s, "Szenen-Explorer (wie 3ds Max, Zeile torso)", 1, [](std::vector<Schritt>& x) {
        x.push_back(tu("KLEIN2 esc: wie 3ds Max, Ansehen, torso gewaehlt und im Explorer", [] {
            figur(0, ModellBereich::Ansehen);
            modellExplorerZeigen(ExplorerArt::Flaeche, torso());
        }));
        x.push_back(pause(0.6));
        x.push_back(rechtsklick("auf die Zeile torso im Explorer", ap2::ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche))));
        x.push_back(pause(0.5));
    });
    // Szenen-Explorer wie Blender (Outliner)
    escFall(s, "Szenen-Explorer (wie Blender, Zeile torso)", 1, [](std::vector<Schritt>& x) {
        x.push_back(tu("KLEIN2 esc: wie Blender, Ansehen, torso gewaehlt und im Explorer", [] {
            figur(1, ModellBereich::Ansehen);
            modellExplorerZeigen(ExplorerArt::Flaeche, torso());
        }));
        x.push_back(pause(0.6));
        x.push_back(rechtsklick("auf die Zeile torso im Outliner", ap2::ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche))));
        x.push_back(pause(0.5));
    });
    // Zeitleiste wie 3ds Max (eine Spalte): Rechtsklick auf die Skala
    escFall(s, "Zeitleiste (wie 3ds Max, Skala)", 1, [](std::vector<Schritt>& x) {
        x.push_back(tu("KLEIN2 esc: wie 3ds Max, Animation", [] { figur(0, ModellBereich::Animation); }));
        x.push_back(pause(0.6));
        x.push_back(rechtsklick("auf die Skala der Zeitleiste", [] { return ap1::r4(ap1::P().skala).GetCenter(); }));
        x.push_back(pause(0.5));
    });
    // Zeitleiste wie Blender: Rechtsklick, dann das Untermenue Playback - Esc schliesst beide Ebenen (Bereich Weights wie AP1 Z9: im
    // Bereich Animation liegen dort die Spuren des Dopesheets)
    escFall(s, "Zeitleiste (wie Blender, Untermenue Playback offen)", 2, [](std::vector<Schritt>& x) {
        x.push_back(tu("KLEIN2 esc: wie Blender, Weights", [] { figur(1, ModellBereich::Weights); }));
        x.push_back(pause(0.6));
        x.push_back(rechtsklick("auf die Skala der Zeitleiste", [] { return ap1::r4(ap1::P().skala).GetCenter(); }));
        x.push_back(pause(0.5));
        x.push_back(rig3::klickMenue(tr(Str::MzMenuePlayback), false));
        x.push_back(pause(0.5));
    });
    // Dopesheet in den Spuren der Zeitleiste (Bereich Animation, Rig, Testschluessel)
    escFall(s, "Dopesheet (wie 3ds Max)", 1, [](std::vector<Schritt>& x) {
        x.push_back(tu("KLEIN2 esc: wie 3ds Max, Animation, Rig, Testschluessel, Dopesheet", [] {
            figur(0, ModellBereich::Animation);
            g_app->animPlaying = false;
            g_app->animFrame = kv5::f0();
            (void)modellRigAnlegen();
            kv5::testSchluessel();
            mtab().kvOffen = true;
            mtab().kvArt = 0;
            mtab().kvEinpassen = true;
            g_app->modelDirty = true;
        }));
        x.push_back(pause(0.8));
        x.push_back(rechtsklick("in das Feld des Dopesheets", [] {
            ImVec2 p{-1.0F, -1.0F};
            if (!modellKvZeileAufSchirm(0, -1, p) && !modellKvZeileAufSchirm(1, kv5::rhand(), p)) { return ImVec2{-1.0F, -1.0F}; }
            return ImVec2{modellKvBildAufSchirm(static_cast<float>(kv5::f0() + 15)), p.y};
        }));
        x.push_back(pause(0.5));
    });
    // Befehlsfeld: Rechtsklick auf eine Zeile des Modifier-Stapels (TurboSmooth auf torso)
    escFall(s, "Befehlsfeld (wie 3ds Max, Zeile TurboSmooth im Stapel)", 1, [](std::vector<Schritt>& x) {
        x.push_back(tu("KLEIN2 esc: wie 3ds Max, Modellieren, TurboSmooth auf torso", [] {
            figur(0, ModellBereich::Modellieren);
            (void)modellStapelHinzufuegen(modell::ModArt::Glaetten);
            modellStapelZeigen();
        }));
        x.push_back(pause(0.8));
        x.push_back(rechtsklick("auf die Zeile TurboSmooth im Befehlsfeld", [] { return ap13s::zeilenOrt(ap13s::nGlaetten(false)); }));
        x.push_back(pause(0.5));
    });
    add(tu("KLEIN2 esc: Stapel wieder leer", [] { ap13s::stapelLeeren(); }));
    // Werkzeugleiste wie Blender: Rechtsklick unter die Knoepfe
    escFall(s, "Werkzeugleiste (wie Blender)", 1, [](std::vector<Schritt>& x) {
        x.push_back(tu("KLEIN2 esc: wie Blender, Modellieren", [] { figur(1, ModellBereich::Modellieren); }));
        x.push_back(pause(0.6));
        x.push_back(rechtsklick("in die Werkzeugleiste (unter den Knoepfen)", [] {
            const ImRect l = ap4::fenster("mtwerkzeugleiste");
            return ap4::leer(l) ? ImVec2{-1.0F, -1.0F} : ImVec2{l.GetCenter().x, l.Max.y - em() * 0.8F};
        }));
        x.push_back(pause(0.5));
    });
    // Ansicht (C4) mit offenem Untermenue Weights
    escFall(s, "Ansicht (wie 3ds Max, Untermenue Weights offen)", 2, [](std::vector<Schritt>& x) {
        x.push_back(tu("KLEIN2 esc: wie 3ds Max, Ansehen, Rechtsklick-Menue der Ansicht", [] {
            figur(0, ModellBereich::Ansehen);
            mtab().fokus = true;
            mtab().kontextOeffnen = true;
            setzeMaus(kv5::fokusPunkt());
        }));
        x.push_back(pause(0.6));
        x.push_back(rig3::klickMenue(tr(Str::MbBereichWeights), false));
        x.push_back(pause(0.5));
    });
    add(pruefe("KLEIN2 esc: das Menue der Ansicht zaehlt sein Esc weiter (ENTSCHEID3A C4)", [] {
        melde("esc: Ansicht " + std::to_string(modellEnt3a().kontextEscZahl) + " mal");
        return modellEnt3a().kontextEscZahl > 0;
    }));
    // Menueleiste: Animation > Animation tools
    escFall(s, "Menueleiste (Animation > Animation tools offen)", 2, [](std::vector<Schritt>& x) {
        x.push_back(tu("KLEIN2 esc: wie 3ds Max, Animation", [] { figur(0, ModellBereich::Animation); }));
        x.push_back(pause(0.6));
        x.push_back(rig3::klickMenue(tr(Str::MbBereichAnimation), true));
        x.push_back(pause(0.4));
        x.push_back(rig3::klickMenue(tr(Str::MbKvMenue), false));
        x.push_back(pause(0.5));
    });
    // Ohne offenes Menue wirkt Esc wie vorher: mit offenem Menue bleibt der Navigationsmodus, das naechste Esc beendet ihn
    add(tu("KLEIN2 esc: wie 3ds Max, Ansehen, torso im Explorer", [] {
        figur(0, ModellBereich::Ansehen);
        modellExplorerZeigen(ExplorerArt::Flaeche, torso());
    }));
    add(pause(0.6));
    add(rechtsklick("auf die Zeile torso im Explorer", ap2::ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche))));
    add(pause(0.5));
    add(tu("KLEIN2 esc: Navigationsmodus Zoom (wie Alt+Z), Fokus im Model-Tab", [] {
        mtab().navModus = 1;
        mtab().fokus = true;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.4));
    add(pruefe("KLEIN2 esc: das erste Esc schliesst nur das Menue - der Navigationsmodus bleibt", [] {
        melde("esc Nav: offen " + std::to_string(offeneMenues()) + ", Nav " + std::to_string(mtab().navModus));
        return offeneMenues() == 0 && mtab().navModus == 1;
    }));
    add(mbefoto::popupsZu());
    add(tu("KLEIN2 esc: Fokus im Model-Tab", [] { mtab().fokus = true; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.4));
    add(pruefe("KLEIN2 esc: ohne offenes Menue beendet Esc den Navigationsmodus wie vorher", [] {
        melde("esc Nav: zweites Esc, Nav " + std::to_string(mtab().navModus));
        return mtab().navModus == 0;
    }));
}

// --- unter: Untermenues bleiben im Fenster -----------------------------------------------------------------------------------------

// Das Fenster eines offenen Menues der Tiefe `tiefe` (ImGui: "<Titel>###Menu_<tiefe>")
ImGuiWindow* menueFenster(int tiefe) {
    char n[32];
    std::snprintf(n, sizeof n, "###Menu_%02d", tiefe);
    ImGuiWindow* w = ImGui::FindWindowByName(n);
    return w != nullptr && w->Active ? w : nullptr;
}

void unterSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("KLEIN2 unter: wie 3ds Max, Animation, Rig", [] {
        figur(0, ModellBereich::Animation);
        (void)modellRigAnlegen();
    }));
    add(pause(0.6));
    add(rig3::klickMenue(tr(Str::MbBereichAnimation), true));
    add(pause(0.4));
    add(rig3::klickMenue(tr(Str::MbKvMenue), false));
    add(pause(0.6));
    add(pruefe("KLEIN2 unter: Animation > Animation tools steht ganz im Hauptfenster (nach oben aufgegangen), ohne eigenes Fenster", [] {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const ImRect haupt(vp->Pos, ImVec2{vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y});
        const ImGuiWindow* w = menueFenster(1);
        const Element* zeile = rig3::imMenue(tr(Str::MbKvMenue));
        if (w == nullptr || zeile == nullptr) {
            melde(std::string("unter: Untermenue ") + (w != nullptr ? "da" : "fehlt") + ", Zeile " + (zeile != nullptr ? "da" : "fehlt") + ", offen " +
                  std::to_string(offeneMenues()));
            return false;
        }
        // nachgestellt? Unter der Zeile haette das Untermenue unten nicht mehr ins Fenster gepasst
        const float obenNatuerlich = zeile->rect.Min.y - ImGui::GetStyle().WindowPadding.y;
        const bool passtUnten = obenNatuerlich + w->Size.y <= haupt.Max.y;
        const bool eigenes = w->Viewport != vp;
        melde("unter: Fenster " + rechteck(haupt) + ", Zeile Animation tools " + rechteck(zeile->rect) + ", Untermenue " + rechteck(w->Rect()) + " (" +
              std::to_string(static_cast<int>(w->Size.y)) + " hoch), haette unten gepasst " + std::to_string(passtUnten ? 1 : 0) + ", eigenes Fenster " +
              std::to_string(eigenes ? 1 : 0));
        if (passtUnten) { meldeFehler("KLEIN2 unter: nicht nachgestellt - das Untermenue passt in diesem Fenster unten hin"); }
        return !passtUnten && !eigenes && haupt.Contains(w->Rect()) && w->Rect().Max.y < obenNatuerlich + w->Size.y;
    }));
    add(rig3::klickMenue(tr(Str::MbKvGruppeWerkzeuge), false));
    add(pause(0.6));
    add(pruefe("KLEIN2 unter: auch die dritte Ebene (Key tools) steht ganz im Hauptfenster, ohne eigenes Fenster", [] {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const ImRect haupt(vp->Pos, ImVec2{vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y});
        const ImGuiWindow* w = menueFenster(2);
        if (w == nullptr) { return false; }
        melde("unter: Key tools " + rechteck(w->Rect()) + ", eigenes Fenster " + std::to_string(w->Viewport != vp ? 1 : 0));
        return w->Viewport == vp && haupt.Contains(w->Rect());
    }));
    add(mbefoto::popupsZu());
    add(pause(0.2));
    // Gegenprobe oben: ein Untermenue, das passt, geht wie bisher rechts neben der Zeile auf (Datei > ... ist zu kurz; Ansicht der
    // Menueleiste hat "Thema" weit oben)
    add(rig3::klickMenue(tr(Str::MenuView), true));
    add(pause(0.4));
    add(rig3::klickMenue(tr(Str::ViewTheme), false));
    add(pause(0.5));
    add(pruefe("KLEIN2 unter: Gegenprobe - ein Untermenue mit Platz (Ansicht > Thema) geht wie bisher rechts neben seiner Zeile auf", [] {
        const ImGuiWindow* w = menueFenster(1);
        const Element* zeile = rig3::imMenue(tr(Str::ViewTheme));
        if (w == nullptr || zeile == nullptr) { return false; }
        const float obenNatuerlich = zeile->rect.Min.y - ImGui::GetStyle().WindowPadding.y;
        melde("unter: Thema " + rechteck(zeile->rect) + ", Untermenue " + rechteck(w->Rect()));
        return std::fabs(w->Pos.y - obenNatuerlich) <= ImGui::GetStyle().ItemSpacing.y + 1.0F && w->Pos.x >= zeile->rect.Max.x - ImGui::GetStyle().ItemInnerSpacing.x - 1.0F;
    }));
    add(mbefoto::popupsZu());
    add(pause(0.2));
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "klein2") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const char* wahl = std::getenv("BHED_KLEIN2_PUNKT");
    const std::string punkte = std::string(",") + (wahl != nullptr ? wahl : "esc,unter") + ",";
    const auto mit = [&](const char* p) { return punkte.find(std::string(",") + p + ",") != std::string::npos; };
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("KLEIN2: Skript fuer den Test (Fenster und Oberflaeche wie das Handbuch: Arbeitsbereich, Einstellung)", [] {
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "klein2.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    add(tu("KLEIN2: Aufteilung wie neue Einstellungen, BOTH_WALK1, wie 3ds Max", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Ansehen;
        mtab().vierAnsichten = false;
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        g_app->animPlaying = false;
        char text[200];
        std::snprintf(text, sizeof text, "%s, Anzeige %.0f x %.0f, Oberflaeche %.2f x %.2f", g_app->modelPath.c_str(), static_cast<double>(ImGui::GetIO().DisplaySize.x),
                      static_cast<double>(ImGui::GetIO().DisplaySize.y), static_cast<double>(g_app->dpiScale), static_cast<double>(g_app->uiScale));
        melde(text);
    }));
    add(pause(1.0));
    add(klickAn("KLEIN2: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (mit("esc")) { escSchritte(s); }
    if (mit("unter")) { unterSchritte(s); }
    add(tu("KLEIN2: Modell-Tab zurueck", [=] {
        ap13s::stapelLeeren();
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        mtab().bereich = ModellBereich::Ansehen;
        mtab().navModus = 0;
        mtab().knochenWahl = -1;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.3));
    return s;
}

}  // namespace klein2
