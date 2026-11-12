// selbsttest_mr.inc - MRESTE (08.11.): Reste des Modell-Blocks vor rc611 (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=blender)
//
//   blender   (zusaetzlich zu selbsttest_mb1a2.inc) der Modus wie Blender in den Stufen Modellieren, UV und Weights so
//             vollstaendig wie der Modus wie 3ds Max - jede Max-Taste hat eine Blender-Taste oder steht in der Liste der
//             Aktionen, fuer die Blender 5.0 keine Taste hat (keymap_data/blender_default.py), und jede neue Blender-Taste
//             wirkt:
//     Modellieren  Strg+L verbundene, Alt+Klick Kantenschleife, Strg+Alt+Klick Kantenring, Alt+Klick auf Vielecke die
//                  Flaechenschleife, J verbindet zwei Punkte, Strg+X loest eine Kante auf (je ein Rueckgaengig zurueck)
//     UV           Strg+E Naht markieren, P Pin, Umschalt+W das Menue Ausrichten (Begradigen, Ausrichten)
//     Weights      Umschalt+X Gewicht aufnehmen, F / Umschalt+F / Strg+F Radius / Staerke / Gewicht ziehen (Esc bricht ab),
//                  Strg+X Gewicht auf die Auswahl, Strg+Strich zieht ab, Umschalt+Strich glaettet
// Eingebunden in selbsttest.cpp (nach selbsttest_pfix.inc).
namespace mr {

namespace w = modell::wof;

// --- Modellieren: ein Kasten mit drei Segmenten je Seite (Punkte mit vier Kanten: Schleifen laufen ganz herum) ----------

int meshAktion(Str s) {
    for (int i = 0; i < modellMeshAktionZahl(); ++i) {
        if (std::string(modellMeshAktionText(i)) == tr(s)) { return i; }
    }
    return -1;
}

int meshAktionBlender(const char* taste) {
    for (int i = 0; i < modellMeshAktionZahl(); ++i) {
        if (std::string(modellMeshAktionTaste(i, true)) == taste) { return i; }
    }
    return -1;
}

int uvAktion(Str s) {
    for (int i = 0; i < modellUvAktionZahl(); ++i) {
        if (std::string(modellUvAktionText(i)) == tr(s)) { return i; }
    }
    return -1;
}

// Die Mitte einer Kante auf dem Schirm, ein Stueck zur Mitte eines ihrer Vielecke hin (der Klick trifft das Vieleck und
// liegt der Kante am naechsten)
ImVec2 kantenKlick(int kante) {
    const w::PolyNetz& n = g_app->mtab.mesh.netz;
    const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
    const w::PKante& k = n.kanten[static_cast<std::size_t>(kante)];
    ImVec2 pa;
    ImVec2 pb;
    if (!modellAufSchirm(a, n.punkte[static_cast<std::size_t>(k.a)].ort, pa) || !modellAufSchirm(a, n.punkte[static_cast<std::size_t>(k.b)].ort, pb) || k.seiten.empty()) {
        return ImVec2{};
    }
    float m[3] = {0.0F, 0.0F, 0.0F};
    const w::PFlaeche& f = n.flaechen[static_cast<std::size_t>(k.seiten[0].first)];
    for (const w::PEcke& e : f.ecken) {
        for (int i = 0; i < 3; ++i) { m[i] += n.punkte[static_cast<std::size_t>(e.punkt)].ort[i] / static_cast<float>(f.ecken.size()); }
    }
    ImVec2 pm;
    if (!modellAufSchirm(a, m, pm)) { return ImVec2{}; }
    const ImVec2 mitte{(pa.x + pb.x) * 0.5F, (pa.y + pb.y) * 0.5F};
    return ImVec2{mitte.x + (pm.x - mitte.x) * 0.2F, mitte.y + (pm.y - mitte.y) * 0.2F};
}

// Eine Kante in der Mitte der Vorderseite (y = -32, alle Punkte), waagerecht (gleiche Hoehe z), nicht am Rand der Seite
int vorderKante() {
    const w::PolyNetz& n = g_app->mtab.mesh.netz;
    for (std::size_t i = 0; i < n.kanten.size(); ++i) {
        const float* a = n.punkte[static_cast<std::size_t>(n.kanten[i].a)].ort;
        const float* b = n.punkte[static_cast<std::size_t>(n.kanten[i].b)].ort;
        if (std::fabs(a[1] + 32.0F) < 1e-3F && std::fabs(b[1] + 32.0F) < 1e-3F && std::fabs(a[2] - b[2]) < 1e-3F && std::fabs(a[2]) > 1.0F &&
            std::fabs(a[2] - 64.0F) > 1.0F && std::fabs(a[0] + b[0]) < 1e-3F) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::string meshZahlen() {
    modellMeshNetzAktualisieren();
    const w::PolyNetz& n = g_app->mtab.mesh.netz;
    return std::to_string(n.lebendePunkte()) + "/" + std::to_string(n.kantenZahl()) + "/" + std::to_string(n.lebendeFlaechen());
}

void meshSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    md1d::neuesRequisit(s, "mr_blender", "");
    add(tu("MR Blender Modellieren: Kasten 3x3x3 Segmente", [] {
        ModellMesh& z = g_app->mtab.mesh;
        z.grund = w::GrundParam{};
        z.grund.form = w::Grundform::Kasten;
        z.grund.laenge = z.grund.breite = z.grund.hoehe = 64.0F;
        z.grund.segL = z.grund.segB = z.grund.segH = 3;
        z.grundNeuerTeil = false;
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Create);
    }));
    add(pause(0.4));
    add(klick(tr(Str::MdUebernehmen), "###meshkaestchen"));
    add(pause(0.4));
    add(pruefe("MR Blender Modellieren: Kasten 56 Punkte, 108 Kanten, 54 Vielecke", [] {
        diag::detail("Knopftest: MR Kasten " + meshZahlen());
        return meshZahlen() == "56/108/54";
    }));
    add(tu("MR Blender Modellieren: wie Blender, Ansicht von vorn, Fokus", [] {
        modellBedienungSetzen(1);
        g_app->mtab.bereich = ModellBereich::Modellieren;
        modellAnsichtSetzen(AnsichtArt::Vorne);
        g_app->mtab.fokus = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    // Die Tabelle: jede Aktion mit einer Taste wie 3ds Max hat eine wie Blender - ausser denen, fuer die Blender 5.0 keine hat
    // (blender_default.py km_edit_mesh: Rand/Element sind keine Auswahlarten, Make Planar, Collapse unter M, Bisect,
    // Target Weld, Clean Remove und Bevel der Vielecke nur im Menue)
    add(pruefe("MR Blender Modellieren: jede Max-Taste hat eine Blender-Taste (oder Blender hat keine)", [] {
        const std::vector<Str> ohne = {Str::MdEbeneRand, Str::MdEbeneElement, Str::MdEben, Str::MdCollapse, Str::MdQuickSlice, Str::MdTargetWeld, Str::MdRemoveSauber, Str::MdBevel};
        bool ok = true;
        for (int i = 0; i < modellMeshAktionZahl(); ++i) {
            if (modellMeshAktionTaste(i, false)[0] == '\0' || modellMeshAktionTaste(i, true)[0] != '\0') { continue; }
            const std::string text = modellMeshAktionText(i);
            if (std::none_of(ohne.begin(), ohne.end(), [&](Str x) { return text == tr(x); })) {
                meldeFehler("MR Blender Modellieren: ohne Blender-Taste: " + text + " (Max " + modellMeshAktionTaste(i, false) + ")");
                ok = false;
            }
        }
        const auto soll = [&](Str x, const char* taste) {
            const int i = meshAktion(x);
            if (i < 0 || std::string(modellMeshAktionTaste(i, true)) != taste) {
                meldeFehler(std::string("MR Blender Modellieren: ") + tr(x) + " soll " + taste + " sein, ist " + (i < 0 ? "-" : modellMeshAktionTaste(i, true)));
                ok = false;
            }
        };
        soll(Str::MdConnect, "J");                 // mesh.vert_connect_path
        soll(Str::MdRemove, "Ctrl+X");             // mesh.dissolve_mode
        soll(Str::MdLoop, "Alt+Click");            // mesh.loop_select
        soll(Str::MdRing, "Ctrl+Alt+Click");       // mesh.edgering_select
        if (meshAktionBlender("Ctrl+L") < 0) {     // mesh.select_linked
            meldeFehler("MR Blender Modellieren: keine Aktion mit Strg+L (Verbundene waehlen)");
            ok = false;
        }
        return ok;
    }));
    // Strg+L: ein Vieleck -> alle 54
    add(tu("MR Blender Modellieren: Vielecke, eines gewaehlt", [] {
        modellAuswahlArtSetzen(AuswahlArt::Dreieck);
        modellMeshNetzAktualisieren();
        w::PAuswahl a;
        a.flaechen = {0};
        md1d::waehle(a, AuswahlArt::Dreieck);
        g_app->mtab.fokus = true;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_L, "Strg+L"));
    add(pause(0.3));
    add(pruefe("MR Blender Strg+L: alle 54 Vielecke des Kastens gewaehlt", [] {
        diag::detail("Knopftest: MR Strg+L - " + std::to_string(g_app->mtab.mesh.auswahl.flaechen.size()) + " Vielecke");
        return g_app->mtab.mesh.auswahl.flaechen.size() == 54U;
    }));
    // Alt+Klick / Strg+Alt+Klick auf eine Kante der Vorderseite
    auto erwartet = std::make_shared<std::size_t>(0);
    const auto altKlick = [&](const char* name, bool strg, AuswahlArt art) {
        add(tu(std::string("MR Blender Modellieren: ") + name + " vorbereiten", [art, strg, erwartet] {
            modellAuswahlArtSetzen(art);
            md1d::waehle(w::PAuswahl{}, art);
            modellMeshNetzAktualisieren();
            const w::PolyNetz& n = g_app->mtab.mesh.netz;
            const int k = vorderKante();
            *erwartet = 0;
            if (k < 0) { return; }
            if (art == AuswahlArt::Kante) {
                *erwartet = (strg ? w::kantenRing(n, k) : w::kantenSchleife(n, k)).size();
            } else {   // Vielecke: die Vielecke am Ring der Kante
                std::set<int> ff;
                for (const int r : w::kantenRing(n, k)) {
                    for (const auto& sf : n.kanten[static_cast<std::size_t>(r)].seiten) { ff.insert(sf.first); }
                }
                *erwartet = ff.size();
            }
            ImGui::GetIO().AddKeyEvent(ImGuiMod_Alt, true);
            if (strg) { ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, true); }
        }));
        add(klickAn(std::string("die Kante (") + name + ")", [] {
            const int k = vorderKante();
            return k < 0 ? ImVec2{} : kantenKlick(k);
        }));
        add(tu(std::string("MR Blender Modellieren: ") + name + " Tasten los", [strg] {
            ImGui::GetIO().AddKeyEvent(ImGuiMod_Alt, false);
            if (strg) { ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, false); }
        }));
        add(pause(0.2));
        add(pruefe(std::string("MR Blender ") + name + ": die Auswahl wie die Rechnung", [art, erwartet, name] {
            const w::PAuswahl& a = g_app->mtab.mesh.auswahl;
            const std::size_t ist = art == AuswahlArt::Kante ? a.kanten.size() : a.flaechen.size();
            const ModellAnsicht& v = g_app->mtab.ansichten[g_app->mtab.aktiv];
            const int k = vorderKante();
            const ImVec2 p = k < 0 ? ImVec2{} : kantenKlick(k);
            char z[200];
            std::snprintf(z, sizeof z, " | Klick %.0f/%.0f in der Ansicht %.0f/%.0f %.0fx%.0f, halbe Hoehe %.1f, Abstand %.1f", static_cast<double>(p.x),
                          static_cast<double>(p.y), static_cast<double>(v.lo[0]), static_cast<double>(v.lo[1]), static_cast<double>(v.groesse[0]),
                          static_cast<double>(v.groesse[1]), static_cast<double>(v.halbHoehe), static_cast<double>(g_app->modelOrbit.distance));
            diag::detail(std::string("Knopftest: MR ") + name + " - " + std::to_string(ist) + ", soll " + std::to_string(*erwartet) + z);
            return *erwartet >= 12U && ist == *erwartet;
        }));
    };
    altKlick("Alt+Klick (Kantenschleife)", false, AuswahlArt::Kante);
    altKlick("Strg+Alt+Klick (Kantenring)", true, AuswahlArt::Kante);
    altKlick("Alt+Klick auf Vielecke (Flaechenschleife)", false, AuswahlArt::Dreieck);
    // J: zwei gegenueberliegende Punkte eines Vielecks verbinden -> eine Kante und ein Vieleck mehr
    auto vorher = std::make_shared<std::string>();
    add(tu("MR Blender Modellieren: zwei gegenueberliegende Punkte eines Vielecks", [vorher] {
        modellAuswahlArtSetzen(AuswahlArt::Ecke);
        modellMeshNetzAktualisieren();
        const w::PFlaeche& f = g_app->mtab.mesh.netz.flaechen[0];
        w::PAuswahl a;
        a.punkte = {f.ecken[0].punkt, f.ecken[2].punkt};
        md1d::waehle(a, AuswahlArt::Ecke);
        *vorher = md1d::md3Jetzt();
        g_app->mtab.fokus = true;
    }));
    add(taste(ImGuiKey_J, "J"));
    add(pause(0.3));
    add(pruefe("MR Blender J: verbunden - 56 Punkte, 109 Kanten, 55 Vielecke", [] {
        diag::detail("Knopftest: MR J - " + meshZahlen() + " | " + g_app->mtab.mesh.meldung);
        return meshZahlen() == "56/109/55";
    }));
    add(tu("MR Blender Modellieren: Rueckgaengig", [] { (void)g_app->mtab.dok.rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("MR Blender J: Rueckgaengig bringt die Datei Byte fuer Byte zurueck", [vorher] { return md1d::md3Jetzt() == *vorher; }));
    // Strg+X: eine Kante zwischen zwei Vielecken aufloesen -> eine Kante und ein Vieleck weniger
    add(tu("MR Blender Modellieren: eine Kante der Vorderseite", [vorher] {
        modellAuswahlArtSetzen(AuswahlArt::Kante);
        modellMeshNetzAktualisieren();
        const w::PolyNetz& n = g_app->mtab.mesh.netz;
        const int k = vorderKante();
        w::PAuswahl a;
        if (k >= 0) { a.kanten = {w::kantenPaar(n.kanten[static_cast<std::size_t>(k)].a, n.kanten[static_cast<std::size_t>(k)].b)}; }
        md1d::waehle(a, AuswahlArt::Kante);
        *vorher = md1d::md3Jetzt();
        g_app->mtab.fokus = true;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_X, "Strg+X"));
    add(pause(0.3));
    add(pruefe("MR Blender Strg+X: aufgeloest - 56 Punkte, 107 Kanten, 53 Vielecke", [] {
        diag::detail("Knopftest: MR Strg+X - " + meshZahlen() + " | " + g_app->mtab.mesh.meldung);
        return meshZahlen() == "56/107/53";
    }));
    add(tu("MR Blender Modellieren: Rueckgaengig", [] { (void)g_app->mtab.dok.rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("MR Blender Strg+X: Rueckgaengig bringt die Datei Byte fuer Byte zurueck", [vorher] { return md1d::md3Jetzt() == *vorher; }));
    add(tu("MR Blender Modellieren: zurueck (wie 3ds Max)", [] { modellBedienungSetzen(0); }));
}

// --- UV ------------------------------------------------------------------------------------------------------------------

void uvSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    uv1b::figurLaden(s, "blender");
    add(pruefe("MR Blender UV: jede Max-Taste hat eine Blender-Taste (oder Blender hat keine)", [] {
        // ohne Blender-Taste (blender_default.py km_uv_editor, Blender 5.0 ohne Legacy): die Tasten von WhiteoutFlakes und
        // 3ds Max fuer Raster, Einpassen, Naht loeschen, Neu loesen, Entspannen, Stapeln, Sperren, das Auswahl-Werkzeug;
        // Abwickeln liegt im Menue U, Begradigen im Menue Umschalt+W. AP7: Dichte (Strg+A) und Packen (Strg+P) haben nach
        // dem freigegebenen Entwurf die Tasten der Blender-Belegung "legacy" und stehen nicht mehr in dieser Liste.
        const std::vector<Str> ohne = {Str::UvWerkzeugAuswahl, Str::UvRaster, Str::UvEinpassen, Str::UvNahtLoeschen, Str::UvAbwickeln, Str::UvNeuLoesen,
                                       Str::UvEntspannen, Str::UvBegradigen, Str::UvStapeln, Str::UvSperren};
        bool ok = true;
        for (int i = 0; i < modellUvAktionZahl(); ++i) {
            if (modellUvAktionTaste(i, false)[0] == '\0' || modellUvAktionTaste(i, true)[0] != '\0') { continue; }
            const std::string text = modellUvAktionText(i);
            if (std::none_of(ohne.begin(), ohne.end(), [&](Str x) { return text == tr(x); })) {
                meldeFehler("MR Blender UV: ohne Blender-Taste: " + text + " (Max " + modellUvAktionTaste(i, false) + ")");
                ok = false;
            }
        }
        return ok;
    }));
    struct Zeile {
        ImGuiKeyChord chord;
        const char* name;
        Str aktion;
    };
    for (const Zeile& z : {Zeile{ImGuiMod_Ctrl | ImGuiKey_E, "Strg+E", Str::UvNahtMarkieren}, Zeile{ImGuiKey_P, "P", Str::UvPin}}) {
        add(tu(std::string("MR Blender UV: Maus ueber dem UV-Editor, vor ") + z.name, [] {
            setzeMaus(uv1b::leinwandMitte());
            g_app->mtab.fokus = true;
            g_app->mtab.letzteTaste.clear();
        }));
        add(pause(0.2));
        add(tasteKombi(z.chord, z.name));
        add(pause(0.2));
        const Str aktion = z.aktion;
        add(pruefe(std::string("MR Blender UV ") + z.name + ": " + tr(aktion), [aktion] {
            const int i = uvAktion(aktion);
            diag::detail("Knopftest: MR UV - letzte Taste \"" + g_app->mtab.letzteTaste + "\", Tabelle " + (i < 0 ? "-" : modellUvAktionTaste(i, true)));
            return g_app->mtab.letzteTaste == tr(aktion);
        }));
    }
    add(tu("MR Blender UV: Maus ueber dem UV-Editor, vor Umschalt+W", [] {
        setzeMaus(uv1b::leinwandMitte());
        g_app->mtab.fokus = true;
    }));
    add(pause(0.2));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_W, "Umschalt+W"));
    add(pause(0.3));
    add(pruefe("MR Blender UV Umschalt+W: das Menue Ausrichten mit Begradigen und Ausrichten", [] {
        bool beg = false;
        bool aus = false;
        for (const Element& e : g_elemente) {
            if (e.fenster.find("Popup") == std::string::npos && e.fenster.find("uvausrichten") == std::string::npos) { continue; }
            beg = beg || e.label == tr(Str::UvBegradigen);
            aus = aus || e.label == tr(Str::UvAusrichten);
        }
        return beg && aus;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(tu("MR Blender UV: zurueck (wie 3ds Max)", [] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Ansehen;
    }));
}

// --- Weights -------------------------------------------------------------------------------------------------------------

int knochenNr(const char* name) {
    for (std::size_t b = 0; b < g_app->anim.bones.size(); ++b) {
        if (g_app->anim.bones[b].name == name) { return static_cast<int>(b); }
    }
    return -1;
}

// Die Gewichte des gewaehlten Knochens je Ecke der gezeichneten Stufe (alle Teile hintereinander)
std::vector<float> gewichtsListe() {
    std::vector<float> out;
    const modell::Inhalt& in = g_app->mtab.dok.inhalt();
    for (const modell::Teil& t : in.teile) {
        if (t.istTag() || static_cast<std::size_t>(g_app->mtab.lodJetzt) >= t.stufen.size()) { continue; }
        const modell::Netz& n = t.stufen[static_cast<std::size_t>(g_app->mtab.lodJetzt)];
        for (std::size_t k = 0; k < n.ecken.size(); ++k) { out.push_back(modell::gewichtVon(n, static_cast<int>(k), g_app->mtab.knochenWahl)); }
    }
    return out;
}

// Summe und groesstes Gewicht des gewaehlten Knochens ueber die Ecken der gezeichneten Stufe
std::pair<double, float> gewichte() {
    const modell::Inhalt& in = g_app->mtab.dok.inhalt();
    double summe = 0.0;
    float hoechst = 0.0F;
    for (const modell::Teil& t : in.teile) {
        if (t.istTag() || static_cast<std::size_t>(g_app->mtab.lodJetzt) >= t.stufen.size()) { continue; }
        const modell::Netz& n = t.stufen[static_cast<std::size_t>(g_app->mtab.lodJetzt)];
        for (std::size_t k = 0; k < n.ecken.size(); ++k) {
            const float g = modell::gewichtVon(n, static_cast<int>(k), g_app->mtab.knochenWahl);
            summe += static_cast<double>(g);
            hoechst = std::max(hoechst, g);
        }
    }
    return {summe, hoechst};
}

ImVec2 ansichtMitte() {
    const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
    return ImVec2{a.lo[0] + a.groesse[0] * 0.5F, a.lo[1] + a.groesse[1] * 0.45F};
}

void gewichtSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("MR Blender Weights: Skript fuer den Test", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mrgw.txt"); }));
    for (Schritt& x : mb1::ladenSeite("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    add(tu("MR Blender Weights: wie Blender, eine Ansicht von vorn, LOD 0, Knochen rhand", [] {
        modellBedienungSetzen(1);
        g_app->mtab.vierAnsichten = false;
        g_app->mtab.bereich = ModellBereich::Weights;
        g_app->mtab.lodWahl = 1;
        g_app->animPlaying = false;
        modellAnsichtSetzen(AnsichtArt::Vorne);
        g_app->mtab.knochenWahl = knochenNr("rhand");
        g_app->mtab.pinselArt = static_cast<int>(modell::PinselArt::Hinzufuegen);
        g_app->mtab.pinselGewicht = 1.0F;
        g_app->mtab.pinselStaerke = 0.5F;
        g_app->mtab.pinselRadius = 40.0F;
        g_app->mtab.pinselSpiegeln = false;
        g_app->mtab.fokus = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    // F / Umschalt+F / Strg+F: 40 Bildpunkte nach rechts, Klick uebernimmt (Radius +40, Staerke und Gewicht +40/200)
    struct Radial {
        ImGuiKeyChord chord;
        const char* name;
        float* wert;
        float soll;
    };
    ModellTab& t = g_app->mtab;
    for (const Radial& r : {Radial{ImGuiKey_F, "F", &t.pinselRadius, 60.0F}, Radial{ImGuiMod_Shift | ImGuiKey_F, "Umschalt+F", &t.pinselStaerke, 0.4F},
                            Radial{ImGuiMod_Ctrl | ImGuiKey_F, "Strg+F", &t.pinselGewicht, 0.4F}}) {
        float* wert = r.wert;
        const float soll = r.soll;
        add(tu(std::string("MR Blender Weights: vor ") + r.name, [wert] {
            if (wert == &g_app->mtab.pinselRadius) { *wert = 20.0F; } else { *wert = 0.2F; }
            setzeMaus(ansichtMitte());
            g_app->mtab.fokus = true;
        }));
        add(pause(0.1));
        add(tasteKombi(r.chord, r.name));
        add(tu(std::string("MR Blender Weights: Maus 40 nach rechts (") + r.name + ")", [] { setzeMaus(ImVec2{ansichtMitte().x + 40.0F, ansichtMitte().y}); }));
        add(pause(0.2));
        add(klickAn(std::string("uebernehmen (") + r.name + ")", [] { return ImVec2{ansichtMitte().x + 40.0F, ansichtMitte().y}; }));
        add(pause(0.2));
        add(pruefe(std::string("MR Blender Weights ") + r.name + ": der Wert folgt der Maus und bleibt nach dem Klick", [wert, soll] {
            diag::detail("Knopftest: MR Weights radial - " + std::to_string(*wert) + ", soll " + std::to_string(soll) + ", letzte Taste \"" + g_app->mtab.letzteTaste + "\" (" +
                         std::to_string(g_app->mtab.tastenZaehler) + ")");
            return std::fabs(*wert - soll) < 0.01F;
        }));
    }
    add(tu("MR Blender Weights: vor F, Esc", [] {
        g_app->mtab.pinselRadius = 40.0F;
        setzeMaus(ansichtMitte());
    }));
    add(tasteKombi(ImGuiKey_F, "F"));
    add(tu("MR Blender Weights: Maus 30 nach rechts", [] { setzeMaus(ImVec2{ansichtMitte().x + 30.0F, ansichtMitte().y}); }));
    add(pause(0.2));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(pruefe("MR Blender Weights F, Esc: der Radius wie vorher (40)", [] { return std::fabs(g_app->mtab.pinselRadius - 40.0F) < 0.01F; }));
    // Striche: erst normal (Hinzufuegen 0,5), dann mit Strg (zieht ab), dann noch einmal normal und mit Umschalt (glaettet)
    auto summe = std::make_shared<std::pair<double, float>>();
    auto anfang = std::make_shared<std::vector<float>>();   // vor dem ersten Strich: das Gemalte sind die Ecken, die davon abweichen
    auto gemaltHoechst = std::make_shared<float>(0.0F);
    const auto hoechstGemalt = [anfang] {
        const std::vector<float> jetzt = gewichtsListe();
        float h = 0.0F;
        for (std::size_t i = 0; i < jetzt.size() && i < anfang->size(); ++i) {
            if (std::fabs(jetzt[i] - (*anfang)[i]) > 1e-6F) { h = std::max(h, jetzt[i]); }
        }
        return h;
    };
    add(tu("MR Blender Weights: Gewichte vor den Strichen merken", [anfang] { *anfang = gewichtsListe(); }));
    const auto strich = [&](const char* name, ImGuiKey mod) {
        add(tu(std::string("MR Blender Weights: vor dem Strich ") + name, [summe] {
            g_app->mtab.pinselGewicht = 1.0F;
            g_app->mtab.pinselStaerke = 0.5F;
            g_app->mtab.pinselRadius = 40.0F;
            *summe = gewichte();
            g_app->mtab.fokus = true;
        }));
        add(ziehenAn(std::string("Strich ") + name, [] { return ansichtMitte(); }, 8.0F, 0.0F, 0, mod));
        add(pause(0.3));
    };
    strich("normal", ImGuiKey_None);
    add(pruefe("MR Blender Weights: ein normaler Strich fuegt rhand hinzu", [summe] {
        const auto g = gewichte();
        diag::detail("Knopftest: MR Weights normal - Summe " + std::to_string(summe->first) + " -> " + std::to_string(g.first) + ", hoechst " + std::to_string(g.second));
        return g.first > summe->first + 0.5;
    }));
    strich("Strg", ImGuiMod_Ctrl);
    add(pruefe("MR Blender Weights Strg+Strich: zieht ab (Summe kleiner)", [summe] {
        const auto g = gewichte();
        diag::detail("Knopftest: MR Weights Strg - Summe " + std::to_string(summe->first) + " -> " + std::to_string(g.first));
        return g.first < summe->first - 0.5;
    }));
    strich("normal (vor Umschalt)", ImGuiKey_None);
    add(tu("MR Blender Weights: das groesste gemalte Gewicht merken", [gemaltHoechst, hoechstGemalt] { *gemaltHoechst = hoechstGemalt(); }));
    strich("Umschalt", ImGuiMod_Shift);
    add(pruefe("MR Blender Weights Umschalt+Strich: glaettet (das groesste gemalte Gewicht sinkt)", [gemaltHoechst, hoechstGemalt] {
        const float h = hoechstGemalt();
        diag::detail("Knopftest: MR Weights Umschalt - hoechst gemalt " + std::to_string(*gemaltHoechst) + " -> " + std::to_string(h));
        return *gemaltHoechst > 0.1F && h < *gemaltHoechst - 0.01F;
    }));
    // Umschalt+X: das Gewicht unter der Maus wird das des Pinsels
    add(tu("MR Blender Weights: Pinselgewicht 0,123, Maus in die Mitte", [] {
        g_app->mtab.pinselGewicht = 0.123F;
        setzeMaus(ansichtMitte());
        g_app->mtab.fokus = true;
    }));
    add(pause(0.2));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_X, "Umschalt+X"));
    add(pause(0.2));
    add(pruefe("MR Blender Weights Umschalt+X: das Gewicht unter der Maus aufgenommen (gemalt, also > 0,123)", [] {
        diag::detail("Knopftest: MR Weights Umschalt+X - " + std::to_string(g_app->mtab.pinselGewicht) + " | " + g_app->mtab.meldung + " | letzte Taste \"" + g_app->mtab.letzteTaste +
                     "\" (" + std::to_string(g_app->mtab.tastenZaehler) + "), Ansicht " + std::to_string(g_app->mtab.aktiv));
        return g_app->mtab.pinselGewicht > 0.13F && g_app->mtab.pinselGewicht <= 1.0F;
    }));
    // Strg+X: Gewicht auf die Auswahl (ein Schritt)
    auto stand = std::make_shared<int>(0);
    add(tu("MR Blender Weights: drei Ecken gewaehlt, Gewicht 1", [stand] {
        g_app->mtab.eckenWahl = {modell::EckOrt{0, 0}, modell::EckOrt{0, 1}, modell::EckOrt{0, 2}};
        g_app->mtab.pinselGewicht = 1.0F;
        *stand = g_app->mtab.dok.stand();
        setzeMaus(ansichtMitte());
        g_app->mtab.fokus = true;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_X, "Strg+X"));
    add(pause(0.2));
    add(pruefe("MR Blender Weights Strg+X: Gewicht auf die Auswahl gesetzt (ein Schritt, Ecke 0 hat 1)", [stand] {
        const modell::Netz& n = g_app->mtab.dok.inhalt().teile[0].stufen[static_cast<std::size_t>(g_app->mtab.lodJetzt)];
        const float g = modell::gewichtVon(n, 0, g_app->mtab.knochenWahl);
        diag::detail("Knopftest: MR Weights Strg+X - Stand " + std::to_string(*stand) + " -> " + std::to_string(g_app->mtab.dok.stand()) + ", Gewicht " + std::to_string(g));
        return g_app->mtab.dok.stand() != *stand && g > 0.99F;
    }));
    add(tu("MR Blender Weights: zurueck (wie 3ds Max), Datei verworfen", [] {
        modellBedienungSetzen(0);
        g_app->mtab.eckenWahl.clear();
        g_app->mtab.bereich = ModellBereich::Ansehen;
        while (g_app->mtab.dok.rueckgaengig()) {}
    }));
}

std::vector<Schritt> schritte(const std::string& n) {
    std::vector<Schritt> s;
    if (n != "blender") { return s; }
    meshSchritte(s);
    uvSchritte(s);
    gewichtSchritte(s);
    return s;
}

}  // namespace mr
