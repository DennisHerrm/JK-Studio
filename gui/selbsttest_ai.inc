// selbsttest_ai.inc - Animations-Import (FBX/glTF/BVH/COLLADA) und Preset-Bibliothek (BHED_EDITORTEST=modellblock,
// BHED_MODELL_NUR=ai)
//
//   ai   Movie Duels, Sturmtruppler als Ziel; Testdateien tests/animimport (eigene Blender-Szene mit HumanIK/Mixamo-Namen,
//        im Quellbaum ueber dem Laufordner gesucht):
//     Erreichbarkeit  "Animation importieren" im Menue Animation > Retargeting und im Rechtsklick (Rig > Retargeting) mit
//                     Tooltip, als Knopf im Rollout Retargeting; "Preset-Bibliothek oeffnen" in der Suche (X) klappt das
//                     Rollout Presets auf, die Gruppe Bibliothek steht darin
//     Mixamo-FBX      ein Klick: Vorlage HumanIK/Mixamo erkannt, Ketten zugeordnet, Vorschau spielt auf der Figur,
//                     Fussrutschen klein; Backen auf einen Platz mit den fps der Datei
//     je Format       glb, BVH, COLLADA: gleiche Knochenzahl und Bilder wie das FBX
//     Bibliothek      jeder Eintrag: Datei da, Quelle und Lizenz genannt, auf die Figur gelegt (Vorschau), Zyklus nahtlos
//                     (Naht nicht groesser als ein Schritt), Platz aus der Bibliothek, gebacken mit Schleife ab 0
// Eingebunden in selbsttest.cpp (nach selbsttest_rt.inc).
namespace ai {

using rig3::imMenue;
using rig3::klickMenue;
using rig3::woIst;

// tests/animimport im Quellbaum: vom Laufordner (Y:/<worktree>/laeufe/<name>) aufwaerts
std::string testOrdner() {
    std::error_code ec;
    std::filesystem::path p = std::filesystem::current_path(ec);   // lauf.py: cwd = Laufordner
    for (int i = 0; i < 6 && !p.empty(); ++i) {
        if (std::filesystem::is_regular_file(p / "tests" / "animimport" / "testgang.fbx", ec)) { return (p / "tests" / "animimport").string(); }
        p = p.parent_path();
    }
    return {};
}

Schritt imMenueMitTooltip(const std::string& wo, Str text) {
    return pruefe("AI " + wo + ": " + std::string(tr(text)) + " steht da, mit Tooltip", [wo, text] {
        for (int i = 0; i < modellRigAktionZahl(); ++i) {
            if (std::string(modellRigAktionText(i)) != tr(text)) { continue; }
            if (imMenue(tr(text)) == nullptr || std::string(modellRigAktionHinweis(i)).size() < 10U) {
                meldeFehler("AI: fehlt im Menue oder ohne Tooltip: " + std::string(tr(text)) + " " + woIst(tr(text)));
                return false;
            }
            return true;
        }
        meldeFehler("AI: keine Aktion " + std::string(tr(text)));
        return false;
    });
}

std::vector<Schritt> schritte(const std::string& n) {
    if (n != "ai") { return {}; }
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string d = testOrdner();
    if (d.empty()) {
        add(pruefe("AI: tests/animimport im Quellbaum ueber dem Laufordner", [] {
            meldeFehler("AI: tests/animimport nicht gefunden (vom Laufordner aufwaerts)");
            return false;
        }));
        return s;
    }
    rt5::figurLaden(s, "models/players/stormtrooper/model.glm", "BOTH_STAND1");
    // --- Erreichbarkeit ---
    add(taste(ImGuiKey_Escape, "Esc"));
    add(klickMenue(tr(Str::MbBereichAnimation), true));
    add(pause(0.3));
    add(klickMenue(tr(Str::RtRollout), false));
    add(pause(0.4));
    add(imMenueMitTooltip("Menue Animation > Retargeting", Str::AiImport));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(klickAn("Rechtsklick in die Ansicht", [] { return kv5::fokusPunkt(); }, 1));
    add(pause(0.3));
    add(klickMenue(tr(Str::MbRgGruppeRig), false));
    add(pause(0.3));
    add(klickMenue(tr(Str::RtRollout), false));
    add(pause(0.4));
    add(imMenueMitTooltip("Rechtsklick > Rig > Retargeting", Str::AiImport));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(tu("AI: Presets zu", [] { g_app->mtab.rt.presetRolloutBild = -1; }));
    rt5::ueberSuche(s, Str::AiBibliothekOeffnen);
    add(pause(0.4));
    add(pruefe("AI: X > Preset-Bibliothek oeffnen klappt das Rollout Presets auf, die Gruppe Bibliothek steht darin", [] {
        if (g_app->mtab.rt.presetRolloutBild < ImGui::GetFrameCount() - 3) { return false; }
        for (const Element& e : g_elemente) {
            if (e.label == tr(Str::AzPresetVorschau)) { return true; }   // AP10: bis rc611 "Put on figure" (AiAuflegen)
        }
        meldeFehler("AI: Knopf " + std::string(tr(Str::AzPresetVorschau)) + " fehlt: " + woIst(tr(Str::AzPresetVorschau)));
        return false;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(tu("AI: Rollout Retargeting auf", [] { g_app->mtab.rt.oeffnen = true; }));
    add(pause(0.4));
    add(pruefe("AI: Knopf Animation importieren im Rollout Retargeting", [] {
        for (const Element& e : g_elemente) {
            if (e.label == tr(Str::AiImport)) { return true; }
        }
        meldeFehler("AI: Knopf fehlt: " + woIst(tr(Str::AiImport)));
        return false;
    }));
    // --- Mixamo-FBX auf einen Klick ---
    add(tu("AI: testgang.fbx importieren (wie Datei waehlen)", [d] { (void)modellRtImportLaden(d + "/testgang.fbx"); }));
    add(pause(0.5));
    add(pruefe("AI FBX: Vorlage HumanIK/Mixamo, Ketten zugeordnet, Vorschau spielt, Fussrutschen klein", [] {
        const ModellRetarget& r = g_app->mtab.rt;
        diag::detail("Knopftest: AI FBX - " + std::to_string(r.quelle.bones.size()) + " Knochen, Vorlage " + r.cq.vorlage + ", " +
                     std::to_string(r.bericht.kettenZugeordnet) + " Ketten, k " + rt5::zahl(r.bericht.k) + ", Rutschen " + rt5::zahl(r.bericht.rutschenMax) +
                     " (Quelle " + rt5::zahl(r.bericht.rutschenQuelle) + "), " + std::to_string(r.welt.size()) + " Bilder; " + r.importInfo);
        return r.cq.vorlage.find("Mixamo") != std::string::npos && r.bericht.kettenZugeordnet >= 9 && r.vorschau && r.spielt && r.welt.size() >= 30 &&
               r.bericht.rutschenMax < 0.5F;
    }));
    add(tu("AI FBX: auf den Platz BOTH_CIN_1 backen", [] {
        (void)modellRtEinstellen(tr(Str::PrUndo), [](modell::RetargetDaten& x) { x.platz = "BOTH_CIN_1"; });
        (void)modellRtBacken();
    }));
    add(pruefe("AI FBX: auf dem Platz BOTH_CIN_1 liegen die Bilder mit 30 fps", [] {
        for (const modell::Platz& p : g_app->mtab.plaetze) {
            if (!p.belegt || p.name != "BOTH_CIN_1" || p.quelle.gla == nullptr) { continue; }
            diag::detail("Knopftest: AI Platz - " + std::to_string(p.quelle.gla->numFrames) + " Bilder, " + std::to_string(p.quelle.sequenz.fps) + " fps");
            return p.quelle.gla->numFrames == static_cast<int>(g_app->mtab.rt.welt.size()) && p.quelle.sequenz.fps == 30;
        }
        return false;
    }));
    for (const char* f : {"testgang.glb", "testgang.bvh", "testgang.dae"}) {
        const std::string datei = f;
        add(tu("AI: " + datei + " importieren", [d, datei] { (void)modellRtImportLaden(d + "/" + datei); }));
        add(pause(0.3));
        add(pruefe("AI " + datei + ": 22 Knochen (dazu Armature als Wurzel, wenn der Exporter sie schreibt), Vorlage Mixamo, Vorschau", [datei] {
            const ModellRetarget& r = g_app->mtab.rt;
            diag::detail("Knopftest: AI " + datei + " - " + std::to_string(r.quelle.bones.size()) + " Knochen, " + std::to_string(r.welt.size()) +
                         " Bilder, Rutschen " + rt5::zahl(r.bericht.rutschenMax) + "; " + r.importInfo);
            const std::size_t soll = 22U + (boneIndex(r.quelle, "Armature") >= 0 ? 1U : 0U);
            return r.quelle.bones.size() == soll && r.cq.vorlage.find("Mixamo") != std::string::npos && r.vorschau && r.welt.size() >= 30;
        }));
    }
    // --- FIX2 P14: Meldungen des Imports in der Sprache der Oberflaeche (vorher deutsch in jeder Sprache) ---
    add(tu("AI P14: testgang.bvh importieren (vorn aus dem Skelett, Hinweis Grundstellung)", [d] { (void)modellRtImportLaden(d + "/testgang.bvh"); }));
    add(pruefe("AI P14: Import-Hinweise ohne deutsche Saetze ausserhalb von de", [] {
        const std::string& t = g_app->mtab.rt.importInfo;
        diag::detail("Knopftest: AI P14 Hinweise (" + std::string(i18n::currentLanguage() == i18n::Language::German ? "de" : "nicht de") + "): " + t);
        if (i18n::currentLanguage() == i18n::Language::German) { return true; }
        return t.find("aus dem Skelett") == std::string::npos && t.find("Grundstellung") == std::string::npos && t.find("Skalierung") == std::string::npos;
    }));
    add(tu("AI P14: glTF ohne Animation importieren", [] {
        std::error_code ec;
        const std::filesystem::path p = std::filesystem::current_path(ec) / "ai_p14_ohne_anim.gltf";
        {
            std::ofstream o(p, std::ios::binary);
            o << R"({"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0]}],"nodes":[{"name":"a","children":[1]},{"name":"b"}]})";
        }
        (void)modellRtImportLaden(p.string());
        std::filesystem::remove(p, ec);
    }));
    add(pruefe("AI P14: Fehler \"keine Animation\" in der Sprache der Oberflaeche", [] {
        const std::string& t = g_app->mtab.rt.quelleFehler;
        diag::detail("Knopftest: AI P14 Fehler: " + t);
        if (t.empty()) { return false; }
        if (i18n::currentLanguage() == i18n::Language::German) { return true; }
        return t.find("keine Animation") == std::string::npos;
    }));
    // --- Preset-Bibliothek ---
    add(pruefe("AI Bibliothek: Eintraege da, je Eintrag Datei, Quelle, Lizenz, Platz", [] {
        const std::vector<PresetEintrag>& b = modellPresetBibliothek();
        std::error_code ec;
        for (const PresetEintrag& e : b) {
            if (!std::filesystem::is_regular_file(e.datei, ec) || e.quelle.empty() || e.lizenz.empty() || e.platz.empty()) {
                meldeFehler("AI Bibliothek: unvollstaendig: " + e.name + " (" + e.datei + ")");
                return false;
            }
        }
        diag::detail("Knopftest: AI Bibliothek - " + std::to_string(b.size()) + " Eintraege in " + g_app->mtab.rt.bibliothekOrdner);
        return b.size() >= 4;
    }));
    add(tu("AI Bibliothek: jeder Eintrag auf die Figur, Naht und Rutschen", [] {
        const std::vector<PresetEintrag>& b = modellPresetBibliothek();
        for (std::size_t i = 0; i < b.size(); ++i) {
            if (!modellPresetBibliothekAuflegen(static_cast<int>(i))) {
                meldeFehler("AI Bibliothek: nicht aufgelegt: " + b[i].name + " - " + g_app->mtab.meldung);
                continue;
            }
            const ModellRetarget& r = g_app->mtab.rt;
            const bool ok = r.vorschau && r.welt.size() >= 5 && (!b[i].schleife || (r.weltNaht >= 0.0F && r.weltNaht <= 1.25F));
            const std::string t = "AI Bibliothek: " + b[i].kategorie + "/" + b[i].name + " - " + std::to_string(r.welt.size()) + " Bilder, Naht " + rt5::zahl(r.weltNaht) +
                                  ", Rutschen " + rt5::zahl(r.bericht.rutschenMax) + " (Quelle " + rt5::zahl(r.bericht.rutschenQuelle) + "), Vorlage " + r.cq.vorlage +
                                  ", " + std::to_string(r.bericht.kettenZugeordnet) + " Ketten, Platz " + g_app->mtab.dok.inhalt().rig.rt.platz;
            if (ok) {
                meldeOk(t);
            } else {
                meldeFehler(t);
            }
        }
    }));
    add(tu("AI Bibliothek: der erste Eintrag gebacken, Ziel _humanoid.gla (dort gibt es den Platz der Bibliothek)", [] {
        modellGlaZielSetzen(1);
        (void)modellPresetBibliothekAuflegen(0);   // die Liste beginnt mit einer Schleife (Stehen)
        (void)modellRtBacken();
    }));
    add(pruefe("AI Bibliothek: auf dem Platz liegt der Zyklus mit Schleife ab 0", [] {
        const std::string platz = g_app->mtab.dok.inhalt().rig.rt.platz;
        for (const modell::Platz& p : g_app->mtab.plaetze) {
            if (!p.belegt || p.name != platz || p.quelle.gla == nullptr) { continue; }
            return p.quelle.gla->numFrames == static_cast<int>(g_app->mtab.rt.welt.size()) && p.quelle.sequenz.loopFrame == 0;
        }
        meldeFehler("AI Bibliothek: Platz " + platz + " leer");
        return false;
    }));
    add(tu("AI: Vorschau aus, Karten-GLA", [] {
        (void)modellRtVorschau(false);
        modellGlaZielSetzen(0);
    }));
    return s;
}

// Das Bild der Handbuchseite (BHED_HB_NUR=ai): ein Gang der Bibliothek (100STYLE, Motion-Capture) auf dem Sturmtruppler,
// Rollout Presets mit der Gruppe Bibliothek offen
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    rt5::figurLaden(s, "models/players/stormtrooper/model.glm", "BOTH_STAND1");
    add(tu("AI Handbuch: Gang aus der Bibliothek auf die Figur, Bild 10, von der Seite", [] {
        const std::vector<PresetEintrag>& b = modellPresetBibliothek();
        int wahl = 0;
        for (std::size_t i = 0; i < b.size(); ++i) {
            if (b[i].kategorie == "walk" && b[i].datei.find("100style") != std::string::npos) {
                wahl = static_cast<int>(i);
                break;
            }
        }
        for (std::size_t i = 0, k = 0; i < b.size(); ++i) {
            if (i > 0 && b[i].kategorie != b[i - 1].kategorie) { ++k; }
            if (static_cast<int>(i) == wahl) { g_app->mtab.rt.bibliothekKategorie = static_cast<int>(k); }
        }
        (void)modellPresetBibliothekAuflegen(wahl);
        g_app->mtab.rt.spielt = false;
        g_app->mtab.rt.bild = 10.0F;
        g_app->mtab.rt.oeffnen = false;
        g_app->mtab.rt.quelleZeigen = false;
        ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        a.art = AnsichtArt::Links;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(tu("AI Handbuch: die Figur im uebertragenen Gang eingepasst", [] {
        modellAnsichtEinpassenAktiv();
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    // die Gruppe Bibliothek steht am Ende des Rollouts Presets (zugeschnittene Knoepfe meldet ImGui nicht): ihre Lage im Fenster
    // merkt sich das Rollout (bibliothekY)
    add(tu("AI Handbuch: Befehlsfeld bis zur Gruppe Bibliothek", [] {
        for (ImGuiWindow* w : GImGui->Windows) {
            if (std::strstr(w->Name, "mtrollouts") != nullptr && g_app->mtab.rt.bibliothekY > 0.0F) {
                ImGui::SetScrollY(w, std::clamp(g_app->mtab.rt.bibliothekY - ImGui::GetFontSize() * 1.5F, 0.0F, w->ScrollMax.y));
            }
        }
    }));
    add(tu("AI Handbuch: Maus in die Ansicht", [] { setzeMaus(kv5::fokusPunkt()); }));
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_ai_bibliothek"));
    return s;
}

}  // namespace ai
