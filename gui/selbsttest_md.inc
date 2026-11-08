// selbsttest_md.inc - Modell-Block 1d: Modellieren und Requisite in die Karte (BHED_EDITORTEST=modellblock,
// BHED_MODELL_NUR=md | mdkarte | mdhb)
//
//   md        ein neues Requisit (.md3) im Model-Tab, wie 3ds Max:
//     Erreichbarkeit  jede Aktion im Menue "Modellieren" (Untermenue je Gruppe) mit Tooltip, im Rechtsklick der Ansicht,
//                     in der Suche (X / F3); Knoepfe im Befehlsfeld; Werkzeugkaestchen mit Uebernehmen / + / Abbrechen
//     Grundkoerper    Kasten ueber das Kaestchen (Klick auf Uebernehmen): 8/12/6
//     Auswahl         Klick in der Ansicht waehlt ein Polygon (Ebene 4), Strg dazu
//     je Werkzeug     Extrude, Inset, Bevel, Chamfer (Kante, Punkt), Connect, Loop Cut, Cut, Weld, Target Weld, Merge,
//                     Bridge, Remove, Delete, Detach, Symmetrize, To Quads: Zahl der Punkte/Kanten/Polygone wie erwartet,
//                     ein Rueckgaengig-Schritt bringt die Datei Byte fuer Byte zurueck
//     Tasten          Max Umschalt+E oeffnet Extrude, Esc bricht ab; Blender I oeffnet Inset, Enter uebernimmt
//     Speichern       als .md3 im Laufordner, gelesen: eine Flaeche, Ecken wie das Dokument, unter 1000
//   mdkarte   Karten-Editor mit dem Testraum: das Requisit mit einem Klick in die Karte (misc_model, models/map_objects/
//             <karte>/), der Editor zeigt es; Rueckweg ueber die Entity, Aenderung + Strg+S -> der Editor laedt es neu;
//             Bau mit q3map2: die Flaechen des Requisits stehen in der .bsp
// Eingebunden in selbsttest.cpp (nach selbsttest_uv.inc).
namespace md1d {

namespace w = modell::wof;
namespace ke = karteneditor;
using rig3::imMenue;
using rig3::klickMenue;
using rig3::woIst;

ModellMesh& mz() { return g_app->mtab.mesh; }

std::string md3Jetzt() {
    std::string b;
    (void)modell::md3Schreiben(g_app->mtab.dok.inhalt(), b, 0);
    return b;
}

std::string zahlen() {
    modellMeshNetzAktualisieren();
    const w::PolyNetz& n = mz().netz;
    return std::to_string(n.lebendePunkte()) + "/" + std::to_string(n.kantenZahl()) + "/" + std::to_string(n.lebendeFlaechen());
}

int aktion(Str s) {
    for (int i = 0; i < modellMeshAktionZahl(); ++i) {
        if (std::string(modellMeshAktionText(i)) == tr(s)) { return i; }
    }
    return -1;
}

Schritt ausloesen(Str s) {
    return tu(std::string("MD: ") + tr(s), [s] {
        const int i = aktion(s);
        if (i < 0) {
            meldeFehler(std::string("MD: Aktion fehlt: ") + tr(s));
            return;
        }
        modellMeshAktionAusloesen(i);
    });
}

// Das Vieleck, dessen Punkte alle auf einer Achsenebene liegen (Kasten: die Seiten)
int vieleckBei(int achse, float wert) {
    modellMeshNetzAktualisieren();
    const w::PolyNetz& n = mz().netz;
    for (std::size_t f = 0; f < n.flaechen.size(); ++f) {
        bool alle = true;
        for (const w::PEcke& e : n.flaechen[f].ecken) { alle = alle && std::fabs(n.punkte[static_cast<std::size_t>(e.punkt)].ort[achse] - wert) < 1e-3F; }
        if (alle) { return static_cast<int>(f); }
    }
    return -1;
}
int punktBei(float x, float y, float z) {
    const w::PolyNetz& n = mz().netz;
    for (std::size_t i = 0; i < n.punkte.size(); ++i) {
        const float* o = n.punkte[i].ort;
        if (std::fabs(o[0] - x) + std::fabs(o[1] - y) + std::fabs(o[2] - z) < 1e-3F) { return static_cast<int>(i); }
    }
    return -1;
}
void waehle(const w::PAuswahl& a, AuswahlArt art) {
    modellAuswahlArtSetzen(art);
    mz().auswahl = a;
    mz().auswahl.sortieren();
    mz().punktFolge = a.punkte;
}

struct Stand {
    std::string vorher;
    std::size_t undo = 0;
};

// Ein Werkzeug: Auswahl setzen, ausloesen, (Kaestchen: Betrag, uebernehmen), Zahlen pruefen, Rueckgaengig -> Byte fuer Byte
void werkzeug(std::vector<Schritt>& s, const std::shared_ptr<Stand>& st, Str name, const std::function<void()>& waehlen, const std::function<void()>& werte,
              const std::string& erwartet) {
    s.push_back(tu(std::string("MD: vor ") + tr(name), [st, waehlen] {
        modellMeshNetzAktualisieren();
        waehlen();
        st->vorher = md3Jetzt();
    }));
    s.push_back(ausloesen(name));
    s.push_back(pause(0.2));
    s.push_back(tu(std::string("MD: Werte und Uebernehmen ") + tr(name), [werte] {
        if (mz().kaestchenOffen) {
            if (werte) { werte(); }
            mz().neuPlanen = true;
            modellMeshVorschau();
            modellMeshUebernehmen(false);
        }
    }));
    s.push_back(pause(0.2));
    s.push_back(pruefe(std::string("MD ") + tr(name) + ": " + erwartet + " Punkte/Kanten/Polygone, gueltig", [erwartet, name] {
        modellMeshNetzAktualisieren();
        std::string g;
        const bool ok = mz().netz.pruefen(&g);
        diag::detail(std::string("Knopftest: MD ") + tr(name) + " - " + zahlen() + (ok ? "" : " ungueltig: " + g) + " | " + mz().meldung);
        return ok && zahlen() == erwartet;
    }));
    s.push_back(tu(std::string("MD: Rueckgaengig nach ") + tr(name), [] { (void)g_app->mtab.dok.rueckgaengig(); }));
    s.push_back(pause(0.2));
    s.push_back(pruefe(std::string("MD ") + tr(name) + ": ein Rueckgaengig-Schritt bringt die Datei Byte fuer Byte zurueck", [st] { return md3Jetzt() == st->vorher; }));
}

const Str kGruppen[7] = {Str::MdGrAuswahl, Str::MdGrGeometrie, Str::MdGrPunkte, Str::MdGrKanten, Str::MdGrRaender, Str::MdGrVielecke, Str::MdGrErstellen};

void menueAlle(std::vector<Schritt>& s, bool rechtsklick) {
    for (int g = 0; g < 7; ++g) {
        for (int k = 0; k < 3; ++k) { s.push_back(taste(ImGuiKey_Escape, "Esc")); }
        s.push_back(pause(0.3));
        if (rechtsklick) {
            s.push_back(klickAn("Rechtsklick in die Ansicht", [] { return kv5::fokusPunkt(); }, 1));
            s.push_back(pause(0.3));
            s.push_back(klickMenue(tr(Str::MdMenue), false));
        } else {
            s.push_back(klickAn("\"Modellieren\" (Menueleiste, falls zu)", [] {
                if (imMenue(tr(kGruppen[0])) != nullptr) { return imMenue(tr(kGruppen[0]))->rect.GetCenter(); }
                const Element* e = rig3::menueLeiste(tr(Str::MdMenue));
                if (e == nullptr) {
                    meldeFehler("MD: Menue Modellieren nicht in der Leiste");
                    return ImVec2{};
                }
                return e->rect.GetCenter();
            }));
        }
        s.push_back(pause(0.3));
        s.push_back(klickMenue(tr(kGruppen[g]), false));
        s.push_back(pause(0.3));
        s.push_back(pruefe("MD " + std::string(rechtsklick ? "Rechtsklick > " : "Menue ") + tr(Str::MdMenue) + ": jede Aktion der Gruppe " + std::to_string(g) + " mit Tooltip", [g] {
            int n = 0;
            for (int i = 0; i < modellMeshAktionZahl(); ++i) {
                if (modellMeshAktionGruppe(i) != g) { continue; }
                const std::string text = modellMeshAktionText(i);
                if (imMenue(text) == nullptr || std::string(modellMeshAktionHinweis(i)).size() < 10U) {
                    meldeFehler("MD: fehlt im Menue oder ohne Tooltip: " + text + " " + woIst(text));
                    return false;
                }
                ++n;
            }
            return n >= 1;
        }));
    }
    for (int k = 0; k < 3; ++k) { s.push_back(taste(ImGuiKey_Escape, "Esc")); }
}

void neuesRequisit(std::vector<Schritt>& s, const char* name, const std::string& shader) {
    s.push_back(tu(std::string("MD: Model-Tab, neues Requisit ") + name, [name, shader] {
        g_app->leftMode = 2;
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        modellBedienungSetzen(0);
        g_app->mtab.vierAnsichten = false;
        g_app->mtab.aktiv = 3;
        g_app->animPlaying = false;
        std::snprintf(mz().requisitName, sizeof mz().requisitName, "%s", name);
        (void)modellNeuesRequisit();
        if (!shader.empty()) {
            g_app->mtab.dok.aendern("shader", [shader](modell::Inhalt& x) { x.teile[0].shader = shader; });
            modellMeshNetzAktualisieren();   // das gezeichnete Modell bekommt den Shader und seine Bilder
        }
    }));
    s.push_back(pause(0.4));
}

// Einen Kasten ueber das Kaestchen erstellen (Max Create > Box), mit einem echten Klick auf "Uebernehmen"
void kasten(std::vector<Schritt>& s, float l) {
    s.push_back(tu("MD: Grundkoerper Kasten", [l] {
        mz().grund = w::GrundParam{};
        mz().grund.form = w::Grundform::Kasten;
        mz().grund.laenge = mz().grund.breite = mz().grund.hoehe = l;
        mz().grundNeuerTeil = false;
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Create);
    }));
    s.push_back(pause(0.4));
    s.push_back(pruefe("MD Kaestchen: offen in der Ansicht, mit Uebernehmen / + / Abbrechen und Tooltips", [] {
        bool ok = mz().kaestchenOffen;
        for (const char* l : {tr(Str::MdUebernehmen), "+", tr(Str::MdAbbrechen)}) {
            bool da = false;
            for (const Element& e : g_elemente) { da = da || (e.label == l && e.fenster.find("meshkaestchen") != std::string::npos); }
            if (!da) { meldeFehler(std::string("MD: Knopf fehlt im Kaestchen: ") + l + " " + woIst(l)); }
            ok = ok && da;
        }
        return ok && mz().kaestchenLage[2] > 50.0F;
    }));
    s.push_back(klick(tr(Str::MdUebernehmen), "###meshkaestchen"));
    s.push_back(pause(0.4));
    s.push_back(pruefe("MD Kasten: 8/12/6, Kaestchen zu, ein Teil", [] {
        diag::detail("Knopftest: MD Kasten - " + zahlen());
        return !mz().kaestchenOffen && zahlen() == "8/12/6";
    }));
    s.push_back(tu("MD: Ansicht einpassen", [] { modellAnsichtEinpassenAktiv(); }));
    s.push_back(pause(0.4));
}

std::vector<Schritt> modellSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<Stand>();
    neuesRequisit(s, "md_kiste", "");
    add(pruefe("MD Neues Requisit: .md3 ohne Datei, ein Teil, Bereich Modellieren, Befehlsfeld mit den Rollouts", [] {
        const modell::Inhalt& in = g_app->mtab.dok.inhalt();
        bool roll = false;
        for (const Element& e : g_elemente) { roll = roll || e.label == tr(Str::MdRolloutMax); }
        return roll && in.herkunft == modell::Herkunft::Md3 && !in.md3Datei && in.teile.size() == 1U && g_app->mtab.bereich == ModellBereich::Modellieren;
    }));
    kasten(s, 64.0F);
    add(pruefe("MD Befehlsfeld: Rollout Edit Poly, Geometrie, Polygone, JKA-Grenzen und Requisit; Knoepfe mit Tooltip", [] {
        int da = 0;
        for (const Str x : {Str::MdRolloutMax, Str::MdGrGeometrie, Str::MdGrVielecke, Str::MdGrGrenzen, Str::MdGrRequisit, Str::MdExtrude, Str::MdInset, Str::MdInDieKarte}) {
            bool d = false;
            for (const Element& e : g_elemente) { d = d || e.label == tr(x); }
            if (!d) { meldeFehler(std::string("MD: fehlt im Befehlsfeld: ") + tr(x)); }
            da += d ? 1 : 0;
        }
        return da == 8;
    }));
    // --- Erreichbarkeit ---
    menueAlle(s, false);
    menueAlle(s, true);
    add(tu("MD: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; }));
    add(ik6::tasteSuche());
    add(pause(0.2));
    add(tippe(tr(Str::MdToQuads)));
    add(pause(0.2));
    add(klickAn("To Quads in der Suche", [] {
        const std::string z = tr(Str::MdToQuads);
        for (const Element& e : g_elemente) {
            if ((e.label == z || e.label.rfind(z + " (", 0) == 0) && e.fenster.find("##Popup_") != std::string::npos) { return e.rect.GetCenter(); }
        }
        meldeFehler("MD: To Quads nicht in der Suche: " + woIst(z));
        return ImVec2{};
    }));
    add(pause(0.3));
    add(pruefe("MD Suche: To Quads ausgeloest (Kaestchen offen)", [] { return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::ToQuads; }));
    add(tu("MD: Abbrechen", [] { modellMeshAbbrechen(); }));
    // --- Auswahl mit der Maus ---
    add(tu("MD: Ebene Polygon (4)", [] {
        modellAuswahlArtSetzen(AuswahlArt::Dreieck);
        mz().auswahl = w::PAuswahl{};
    }));
    add(klickAn("Mitte der Ansicht (auf den Kasten)", [] {
        const ModellAnsicht& a = g_app->mtab.ansichten[static_cast<std::size_t>(g_app->mtab.aktiv)];
        return ImVec2{a.lo[0] + a.groesse[0] * 0.5F, a.lo[1] + a.groesse[1] * 0.5F};
    }));
    add(pause(0.3));
    add(pruefe("MD Auswahl: ein Klick waehlt ein Polygon, alle Ecken seiner Punkte folgen (Gizmo/G R S)", [] {
        diag::detail("Knopftest: MD Klick - " + std::to_string(mz().auswahl.flaechen.size()) + " Polygone, " + std::to_string(g_app->mtab.eckenWahl.size()) + " Ecken");
        return mz().auswahl.flaechen.size() == 1U && g_app->mtab.eckenWahl.size() == 12U;   // vier Punkte, je drei Ecken (harte Kanten)
    }));
    // --- Werkzeuge (Kasten 64, Mitte unten bei 0) ---
    const auto oben = [] { waehle(w::PAuswahl{{}, {}, {vieleckBei(2, 64.0F)}}, AuswahlArt::Dreieck); };
    const auto obenVorn = [] { waehle(w::PAuswahl{{}, {}, {vieleckBei(2, 64.0F), vieleckBei(1, -32.0F)}}, AuswahlArt::Dreieck); };
    const auto kante = [](float ax, float ay, float az, float bx, float by, float bz) {
        return [=] { waehle(w::PAuswahl{{}, {w::kantenPaar(punktBei(ax, ay, az), punktBei(bx, by, bz))}, {}}, AuswahlArt::Kante); };
    };
    werkzeug(s, st, Str::MdExtrude, oben, [] { mz().betrag = 16.0F; mz().art = 0; }, "12/20/10");
    werkzeug(s, st, Str::MdExtrude, obenVorn, [] { mz().betrag = 8.0F; mz().art = 2; }, "16/28/14");
    werkzeug(s, st, Str::MdInset, oben, [] { mz().betrag = 8.0F; mz().art = 0; }, "12/20/10");
    werkzeug(s, st, Str::MdBevel, oben, [] { mz().hoehe = 8.0F; mz().betrag = -8.0F; mz().art = 0; }, "12/20/10");
    werkzeug(s, st, Str::MdChamfer, kante(32, 32, 0, 32, 32, 64), [] { mz().betrag = 4.0F; }, "10/15/7");
    // MRESTE Punkt 3: Chamfer mit 3 Segmenten (Max Segments, Blender Segments) - zwei Bogenpunkte je Ende, drei Vierecke
    werkzeug(s, st, Str::MdChamfer, kante(32, 32, 0, 32, 32, 64), [] {
        mz().betrag = 4.0F;
        mz().segmente = 3;
        mz().profil = 0.5F;
    }, "14/21/9");
    s.push_back(tu("MD: Segmente wieder 1", [] { mz().segmente = 1; }));
    werkzeug(s, st, Str::MdChamferPunkt, [] { waehle(w::PAuswahl{{punktBei(32, 32, 64)}, {}, {}}, AuswahlArt::Ecke); }, [] { mz().betrag = 8.0F; }, "10/15/7");
    werkzeug(s, st, Str::MdConnect,
             [] {
                 waehle(w::PAuswahl{{}, {w::kantenPaar(punktBei(-32, -32, 64), punktBei(32, -32, 64)), w::kantenPaar(punktBei(-32, 32, 64), punktBei(32, 32, 64))}, {}}, AuswahlArt::Kante);
             },
             [] { mz().segmente = 2; mz().betrag = 0.0F; }, "12/18/8");
    werkzeug(s, st, Str::MdLoopCut, kante(32, -32, 0, 32, -32, 64), [] { mz().segmente = 1; mz().betrag = 0.25F; }, "12/20/10");
    werkzeug(s, st, Str::MdWeld,
             [] { waehle(w::PAuswahl{{punktBei(32, 32, 64), punktBei(-32, 32, 64)}, {}, {}}, AuswahlArt::Ecke); }, [] { mz().schwelle = 100.0F; }, "7/11/6");
    werkzeug(s, st, Str::MdMerge, [] { waehle(w::PAuswahl{{punktBei(32, 32, 64), punktBei(-32, 32, 64)}, {}, {}}, AuswahlArt::Ecke); }, [] { mz().art = 0; }, "7/11/6");
    werkzeug(s, st, Str::MdBridge, [] { waehle(w::PAuswahl{{}, {}, {vieleckBei(2, 64.0F), vieleckBei(2, 0.0F)}}, AuswahlArt::Dreieck); },
             [] { mz().segmente = 2; mz().verdrehen = 0; }, "12/24/12");
    werkzeug(s, st, Str::MdDetach, oben, [] { mz().art = 0; }, "12/16/6");
    werkzeug(s, st, Str::MdSymmetrize, [] { waehle(w::PAuswahl{}, AuswahlArt::Dreieck); }, [] { mz().achse = 0; mz().vonPlus = true; }, "12/20/10");
    // ohne Kaestchen: Target Weld (zwei gewaehlt: der erste auf den zweiten), Remove, Delete, Cut
    werkzeug(s, st, Str::MdTargetWeld, [] { waehle(w::PAuswahl{{punktBei(32, 32, 64), punktBei(-32, 32, 64)}, {}, {}}, AuswahlArt::Ecke); }, {}, "7/11/6");
    werkzeug(s, st, Str::MdRemove, kante(-32, -32, 64, 32, -32, 64), {}, "8/11/5");
    werkzeug(s, st, Str::MdDelete, oben, {}, "8/12/5");
    // Cut ueber die Maus: zwei Klicks auf zwei Kanten des Deckels (von oben gesehen)
    add(tu("MD Cut: Ansicht Oben, eingepasst", [st] {
        modellAnsichtSetzen(AnsichtArt::Oben);
        modellAnsichtEinpassenAktiv();
        st->vorher = md3Jetzt();
    }));
    add(pause(0.4));
    add(ausloesen(Str::MdCut));
    const auto kantenMitte = [](float ax, float ay, float bx, float by) {
        return [=] {
            ImVec2 p{0, 0};
            const float o[3] = {(ax + bx) * 0.5F, (ay + by) * 0.5F, 64.0F};
            (void)modellAufSchirm(g_app->mtab.ansichten[static_cast<std::size_t>(g_app->mtab.aktiv)], o, p);
            return p;
        };
    };
    add(klickAn("Cut: Mitte der vorderen Deckelkante (knapp innen)", kantenMitte(-32, -31, 32, -31)));
    add(pause(0.2));
    add(klickAn("Cut: Mitte der hinteren Deckelkante (knapp innen)", kantenMitte(-32, 31, 32, 31)));
    add(pause(0.3));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pruefe("MD Cut: zwei Klicks schneiden den Deckel: 10/15/7, Esc beendet", [] {
        diag::detail("Knopftest: MD Cut - " + zahlen() + " | " + mz().meldung);
        return zahlen() == "10/15/7" && !mz().cutAn;
    }));
    add(tu("MD Cut: Rueckgaengig (ein Schritt je Schnitt)", [st] {
        while (md3Jetzt() != st->vorher && g_app->mtab.dok.kannRueckgaengig()) { (void)g_app->mtab.dok.rueckgaengig(); }
    }));
    add(pruefe("MD Cut: Rueckgaengig bringt den Kasten zurueck", [st] { return md3Jetzt() == st->vorher; }));
    add(tu("MD: Ansicht Perspektive", [] {
        modellAnsichtSetzen(AnsichtArt::Perspektive);
        modellAnsichtEinpassenAktiv();
    }));
    // To Quads: das Requisit als .md3 gelesen hat Dreiecke
    add(tu("MD To Quads: Kasten als .md3 schreiben und wieder oeffnen (12 Dreiecke)", [] {
        const std::string b = md3Jetzt();
        std::string f;
        (void)modellMd3Oeffnen(b, "md_kiste.md3", &f);
        modellAuswahlArtSetzen(AuswahlArt::Dreieck);
    }));
    add(pause(0.3));
    add(pruefe("MD To Quads: gelesen 8/18/12", [] { return zahlen() == "8/18/12"; }));
    werkzeug(s, st, Str::MdToQuads, [] { waehle(w::PAuswahl{}, AuswahlArt::Dreieck); }, {}, "8/12/6");
    // p_jka Slice: nichts gewaehlt = das ganze Modell, Ebene in der Mitte der Hoehe (Vorgabe Z); 8 Kanten kreuzen (4 senkrechte,
    // 4 Diagonalen der Seiten), 8 Seitendreiecke geteilt: 16/34/20
    werkzeug(s, st, Str::MdSlice, [] { waehle(w::PAuswahl{}, AuswahlArt::Dreieck); mz().achse = 2; }, {}, "16/34/20");
    // p_jka Hinge: ein Dreieck des Deckels um eine seiner Kanten (30 Grad): eine Kopie, zwei Dreieckswaende, keine Wand am
    // Scharnier - 9/21/14
    werkzeug(s, st, Str::MdHinge, oben, {}, "9/21/14");
    // p_jka QuickSlice: zwei Klicks in der Ansicht von vorn, waagrecht durch die halbe Hoehe - dieselbe Ebene wie Slice oben
    // (vier Ansichten wie 3ds Max, damit die Ansicht von vorn im Bild ist - sichtbar gilt ab dem naechsten Bild)
    add(tu("MD QuickSlice: vier Ansichten", [] { g_app->mtab.vierAnsichten = true; }));
    add(pause(0.3));
    add(tu("MD QuickSlice: zwei Klicks in der Ansicht von vorn, waagrecht in halber Hoehe", [st] {
        modellMeshNetzAktualisieren();
        waehle(w::PAuswahl{}, AuswahlArt::Dreieck);
        st->vorher = md3Jetzt();
        const ModellAnsicht* vorn = nullptr;
        for (const ModellAnsicht& a : g_app->mtab.ansichten) {
            if (a.art == AnsichtArt::Vorne && a.sichtbar) { vorn = &a; }
        }
        if (vorn == nullptr) {
            meldeFehler("MD QuickSlice: keine Ansicht von vorn sichtbar");
            return;
        }
        float lo[3] = {1e30F, 1e30F, 1e30F}, hi[3] = {-1e30F, -1e30F, -1e30F};
        for (const w::PPunkt& q : mz().netz.punkte) {
            for (int c = 0; c < 3; ++c) {
                lo[c] = std::min(lo[c], q.ort[c]);
                hi[c] = std::max(hi[c], q.ort[c]);
            }
        }
        const float z = 0.5F * (lo[2] + hi[2]), y = 0.5F * (lo[1] + hi[1]);
        const float p0[3] = {lo[0] - 8.0F, y, z}, p1[3] = {hi[0] + 8.0F, y, z};
        ImVec2 s0, s1;
        if (!modellAufSchirm(*vorn, p0, s0) || !modellAufSchirm(*vorn, p1, s1)) {
            meldeFehler("MD QuickSlice: Punkte nicht auf dem Schirm");
            return;
        }
        (void)modellMeshQuickSlice(*vorn, s0, s1);
    }));
    add(pause(0.2));
    add(pruefe("MD QuickSlice: 16/34/20 wie Slice in halber Hoehe, gueltig", [] {
        modellMeshNetzAktualisieren();
        std::string g;
        const bool ok = mz().netz.pruefen(&g);
        diag::detail("Knopftest: MD QuickSlice - " + zahlen() + (ok ? "" : " ungueltig: " + g) + " | " + mz().meldung);
        return ok && zahlen() == "16/34/20";
    }));
    add(tu("MD: Rueckgaengig nach QuickSlice", [] { (void)g_app->mtab.dok.rueckgaengig(); }));
    add(pause(0.2));
    add(pruefe("MD QuickSlice: ein Rueckgaengig-Schritt bringt die Datei Byte fuer Byte zurueck", [st] { return md3Jetzt() == st->vorher; }));
    add(tu("MD QuickSlice: eine Ansicht", [] { g_app->mtab.vierAnsichten = false; }));
    // --- Tasten ---
    add(tu("MD Tasten: Deckel waehlen, Fokus", [oben, st] {
        oben();
        g_app->mtab.fokus = true;
        st->vorher = md3Jetzt();
    }));
    add(tasteMit(ImGuiMod_Shift, ImGuiKey_E, "Umschalt+E (Max Extrude)"));
    add(pause(0.3));
    add(pruefe("MD Taste Max Umschalt+E: Extrude im Kaestchen", [] { return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Extrude; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(pruefe("MD Esc: abgebrochen, die Datei wie vorher", [st] { return !mz().kaestchenOffen && md3Jetzt() == st->vorher; }));
    add(tu("MD Tasten: wie Blender", [oben] {
        modellBedienungSetzen(1);
        oben();
        g_app->mtab.fokus = true;
    }));
    add(taste(ImGuiKey_I, "I (Blender Inset)"));
    add(pause(0.3));
    add(pruefe("MD Taste Blender I: Inset im Kaestchen", [] { return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Inset; }));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.3));
    // der Kasten ist hier der gelesene aus To Quads (Rueckgaengig: 12 Dreiecke) - ein Dreieck eingefuegt: 11/24/15
    add(pruefe("MD Enter: Inset uebernommen (ein Dreieck des Deckels), 11/24/15", [] { return !mz().kaestchenOffen && zahlen() == "11/24/15"; }));
    add(tu("MD: wieder wie 3ds Max", [] { modellBedienungSetzen(0); }));
    // p_jka: die Max-Tasten gegen die Autodesk-Hilfe 3ds Max 2024 "Keyboard Shortcuts" (GUID-A73E1B09, Tabellen Edit Poly,
    // Editable Poly, Main UI/Editable Polygon Object) und "Editable Poly (Vertex/Edge)" (Remove, Clean Remove); jede
    // Aktion ohne Max-Taste nennt im Tooltip den Weg ueber das Menue
    {
        struct Soll {
            Str text;
            ImGuiKeyChord chord;
            const char* taste;
        };
        static const Soll kAutodesk[] = {
            {Str::MdExtrude, ImGuiMod_Shift | ImGuiKey_E, "Shift+E"},
            {Str::MdBevel, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_B, "Ctrl+Shift+B"},
            {Str::MdChamfer, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_C, "Ctrl+Shift+C"},
            {Str::MdConnect, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_E, "Ctrl+Shift+E"},
            {Str::MdCut, ImGuiMod_Alt | ImGuiKey_C, "Alt+C"},
            {Str::MdTargetWeld, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_W, "Ctrl+Shift+W"},
            {Str::MdRing, ImGuiMod_Alt | ImGuiKey_R, "Alt+R"},
            {Str::MdLoop, ImGuiMod_Alt | ImGuiKey_L, "Alt+L"},
            {Str::MdRemove, ImGuiKey_Backspace, "Backspace"},
            {Str::MdRemoveSauber, ImGuiMod_Ctrl | ImGuiKey_Backspace, "Ctrl+Backspace"},
            {Str::MdEben, ImGuiMod_Alt | ImGuiKey_M, "Alt+M"},
            {Str::MdCollapse, ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_C, "Ctrl+Alt+C"},
            {Str::MdQuickSlice, ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Q, "Ctrl+Shift+Q"},
        };
        for (const Soll& k : kAutodesk) {
            add(pruefe(std::string("MD Max-Taste wie Autodesk: ") + tr(k.text) + " = " + k.taste, [k] {
                for (int i = 0; i < modellMeshAktionZahl(); ++i) {
                    if (std::string(modellMeshAktionText(i)) != tr(k.text)) { continue; }
                    return modellMeshAktionChord(i, false) == k.chord && std::string(modellMeshAktionTaste(i, false)) == k.taste;
                }
                return false;
            }));
        }
        add(pruefe("MD Max: jede Aktion ohne Taste nennt im Tooltip den Weg ueber das Menue", [] {
            int ohne = 0;
            for (int i = 0; i < modellMeshAktionZahl(); ++i) {
                if (modellMeshAktionTaste(i, false)[0] != '\0') { continue; }
                ++ohne;
                const std::string t = modellMeshAktionTooltip(i);
                if (t.find(std::string(tr(Str::MdMenue)) + " > ") == std::string::npos || t.find(modellMeshAktionText(i)) == std::string::npos) {
                    meldeFehler(std::string("MD: kein Weg im Tooltip von ") + modellMeshAktionText(i));
                    return false;
                }
            }
            return ohne > 0;
        }));
    }
    add(tu("MD Tasten: Deckel waehlen (Max), Fokus", [oben] {
        oben();
        g_app->mtab.fokus = true;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_B, "Strg+Umschalt+B (Max Bevel)"));
    add(pause(0.3));
    add(pruefe("MD Taste Max Strg+Umschalt+B: Bevel im Kaestchen", [] { return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Bevel; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(pruefe("MD Esc: Bevel abgebrochen", [] { return !mz().kaestchenOffen; }));
    // --- Speichern ---
    add(tu("MD Speichern: als .md3 in den Laufordner", [] {
        g_app->mtab.speicherPfad = (std::filesystem::current_path() / "md_kiste.md3").string();
        modellSpeichern(false);
    }));
    add(pause(0.3));
    add(pruefe("MD Speichern: .md3 gelesen, eine Flaeche, Ecken und Dreiecke wie das Dokument, unter 1000", [] {
        const std::string b = slurp((std::filesystem::current_path() / "md_kiste.md3").string());
        Md3Model m;
        std::string f;
        if (!readMd3(b, m, &f) || m.surfaces.size() != 1U) {
            meldeFehler("MD: .md3 nicht lesbar: " + f);
            return false;
        }
        const modell::Netz& nz = g_app->mtab.dok.inhalt().teile[0].stufen[0];
        diag::detail("Knopftest: MD Speichern - " + std::to_string(b.size()) + " Bytes, " + std::to_string(m.surfaces[0].numVerts) + " Ecken, " +
                     std::to_string(m.surfaces[0].indexes.size() / 3U) + " Dreiecke");
        return m.surfaces[0].numVerts == static_cast<int>(nz.ecken.size()) && m.surfaces[0].indexes.size() == nz.indizes().size() && m.surfaces[0].numVerts < 1000;
    }));
    return s;
}

// --- Requisit in die Karte ---------------------------------------------------------------------------------------

std::string vfs() { return "models/map_objects/m0raum/md_kiste.md3"; }

int requisitEntitaeten() {
    int n = 0;
    for (const auto& e : ke::editor().karte.entitaeten()) { n += e->wert("model") == vfs() ? 1 : 0; }
    return n;
}

std::vector<Schritt> karteSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<std::pair<float, std::size_t>>(0.0F, 0);
    add(tu("MD Karte: Spielordner, Modus Events (kein Import-Ordner: er entsteht mit dem Requisit)", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(platform::settingsDirectory()) / "import", ec);
    }));
    add(pause(0.3));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(warteBis("MD Karte: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("MD Karte: Testraum (Boden z 0), Blick von oben", [] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();
        (void)stufe3::raumKarteOeffnen("m0raum", false);
        ke::Editor& e = ke::editor();
        e.kamera.ursprung = karte::Vektor3f(-120, -150, 200);
        e.kamera.winkel = karte::Vektor3f(-89.0F, 0.0F, 0.0F);
        ke::modellSetzKlasse() = 0;
        ke::modellSetzKollision() = false;
    }));
    add(pause(1.0));
    // das Requisit: Kasten 32 mit der Textur der Spielkiste, Deckel 16 hoch extrudiert
    neuesRequisit(s, "md_kiste", m0::crate().m.surfaces.empty() ? std::string() : m0::crate().m.surfaces[0].shader);
    kasten(s, 32.0F);
    add(tu("MD Karte: Deckel extrudieren", [] {
        waehle(w::PAuswahl{{}, {}, {vieleckBei(2, 32.0F)}}, AuswahlArt::Dreieck);
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Extrude);
        mz().betrag = 16.0F;
        modellMeshVorschau();
        modellMeshUebernehmen(false);
    }));
    add(pause(0.3));
    add(klick(tr(Str::MdInDieKarte)));
    add(pause(0.3));
    add(pruefe("MD Karte: ein Klick - .md3 im Import-Ordner, Map-Tab vorn", [] {
        const std::filesystem::path p = std::filesystem::path(platform::settingsDirectory()) / "import" / "models" / "map_objects" / "m0raum" / "md_kiste.md3";
        diag::detail("Knopftest: MD Karte - " + mz().meldung);
        return std::filesystem::is_regular_file(p) && g_app->leftMode == 3 && mz().requisitVfs == vfs();
    }));
    add(warteBis("MD Karte: misc_model gesetzt (das Dateisystem findet den neuen Import-Ordner)", [] {
        ke::spielPruefen();
        return !ke::requisitWartet() && requisitEntitaeten() == 1;
    }, 3000));
    add(pause(1.0));
    add(pruefe("MD Karte: misc_model mit models/map_objects/m0raum/md_kiste.md3 auf dem Boden, der Editor zeigt das Modell (Kasten 32 x 32 x 48)", [] {
        const karte::ModellAnsicht a = ke::kartenModellPfad(vfs());
        std::shared_ptr<karte::Entitaet> ent;
        for (const auto& e : ke::editor().karte.entitaeten()) {
            if (e->wert("model") == vfs()) { ent = e; }
        }
        if (!ent || !a.modell || a.modell->null) {
            meldeFehler("MD Karte: Entity oder Modell fehlt");
            return false;
        }
        const karte::Kasten& g = a.modell->grenzen;
        diag::detail("Knopftest: MD Karte - " + ent->klassenname() + " origin " + ent->wert("origin") + ", Grenzen " + std::to_string(g.extents[0] * 2) + " x " +
                     std::to_string(g.extents[1] * 2) + " x " + std::to_string(g.extents[2] * 2));
        return ent->klassenname() == "misc_model" && std::fabs(g.extents[0] - 16.0F) < 0.1F && std::fabs(g.extents[2] - 24.0F) < 0.1F;
    }));
    // Rueckweg: ueber die Entity in den Model-Tab, Deckel noch einmal hoeher, Strg+S -> der Editor laedt jedes Exemplar neu
    add(tu("MD Karte: Rueckweg ueber die Entity (Im Model-Tab bearbeiten)", [] { (void)modellRequisitBearbeiten(vfs()); }));
    add(pause(0.5));
    add(pruefe("MD Rueckweg: Model-Tab, Bereich Modellieren, das Requisit mit 12 Punkten (als .md3 gelesen: 12/30/20)", [] {
        diag::detail("Knopftest: MD Rueckweg - " + zahlen() + " | " + mz().meldung);
        return g_app->leftMode == 2 && g_app->mtab.bereich == ModellBereich::Modellieren && mz().netz.lebendePunkte() == 12;
    }));
    add(tu("MD Rueckweg: Deckel um 16 hoeher, Strg+S", [] {
        modellMeshNetzAktualisieren();
        waehle(w::PAuswahl{{}, {}, {}}, AuswahlArt::Ecke);
        w::PAuswahl a;
        for (std::size_t i = 0; i < mz().netz.punkte.size(); ++i) {
            if (mz().netz.punkte[i].ort[2] > 40.0F) { a.punkte.push_back(static_cast<int>(i)); }
        }
        mz().auswahl = a;
        g_app->mtab.dok.aendern("hoeher", [a](modell::Inhalt& x) {
            w::PolyNetz n = w::polyAus(x, 0);
            for (const int p : a.punkte) { n.punkte[static_cast<std::size_t>(p)].ort[2] += 16.0F; }
            n.beruehren(0);
            w::polyZurueck(n, x);
        });
        modellSpeichern(false);
    }));
    add(warteBis("MD Live: der Editor laedt das Requisit neu (64 hoch)", [] {
        ke::modellLiveWacheKarte();
        const karte::ModellAnsicht a = ke::kartenModellPfad(vfs());
        return a.modell && std::fabs(a.modell->grenzen.extents[2] - 32.0F) < 0.1F;
    }, 600));
    add(pruefe("MD Live: das Exemplar behaelt Lage und Schluessel", [] { return requisitEntitaeten() == 1; }));
    // Bau: die Flaechen des Requisits stehen in der .bsp (misc_model backt q3map2 ein)
    add(tu("MD Karte: Bau (q3map2 -meta)", [] {
        g_app->leftMode = 3;
        ke::Editor& e = ke::editor();
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{"MD bsp", {"[q3map2] -meta \"[MapFile]\""}});
        ke::bauStarten(e.bauProjekt.eintraege.size() - 1);
    }));
    add(warteBis("MD Karte: Bau fertig", [] { return !ke::bauLaeuft(); }, 30000));
    add(pruefe("MD Bau: die Flaechen des Requisits (Textur der Kiste) sind in die .bsp eingebacken", [] {
        const std::string b = slurp((std::filesystem::current_path() / "s3bau" / "m0raum.bsp").string());
        BspGeometry g;
        std::string fehler;
        if (!readBspGeometry(b, g, &fehler)) {
            meldeFehler("MD Bau: .bsp nicht lesbar: " + fehler);
            return false;
        }
        std::string sh = m0::crate().m.surfaces.empty() ? std::string() : m0::kl(m0::crate().m.surfaces[0].shader);
        if (const std::size_t p = sh.find_last_of('.'); p != std::string::npos && sh.find('/', p) == std::string::npos) { sh = sh.substr(0, p); }
        int z = 0;
        for (const BspSurface& f : g.surfaces) {
            if (f.shader < 0 || static_cast<std::size_t>(f.shader) >= g.shaders.size()) { continue; }
            std::string n = m0::kl(g.shaders[static_cast<std::size_t>(f.shader)].name);
            if (const std::size_t p = n.find_last_of('.'); p != std::string::npos && n.find('/', p) == std::string::npos) { n = n.substr(0, p); }
            z += n == sh ? 1 : 0;
        }
        diag::detail("Knopftest: MD Bau - " + std::to_string(z) + " Flaechen mit " + sh + " von " + std::to_string(g.surfaces.size()));
        return z > 0;
    }));
    return s;
}

// --- Handbuch (BHED_HB_NUR=md): h_md_modell (Model-Tab, Kaestchen offen), h_md_karte (Requisit im Karten-Editor) ----------
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    neuesRequisit(s, "md_kiste", m0::crate().m.surfaces.empty() ? std::string() : m0::crate().m.surfaces[0].shader);
    kasten(s, 32.0F);
    add(tu("MD Handbuch: Deckel extrudiert und eingefuegt, Extrude im Kaestchen offen", [] {
        g_app->mtab.vierAnsichten = true;
        waehle(w::PAuswahl{}, AuswahlArt::Dreieck);
        for (int i = 0; i < 4; ++i) {   // eingepasst auf das ganze Modell (ohne Auswahl), dann waehlen
            g_app->mtab.aktiv = i;
            modellAnsichtEinpassenAktiv();
        }
        waehle(w::PAuswahl{{}, {}, {vieleckBei(2, 32.0F)}}, AuswahlArt::Dreieck);
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Inset);
        mz().betrag = 6.0F;
        modellMeshVorschau();
        modellMeshUebernehmen(false);
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Extrude);
        mz().betrag = 12.0F;
        modellMeshVorschau();
        g_app->mtab.aktiv = 3;
    }));
    add(pause(0.8));
    add(tu("MD Handbuch: Maus in die Ansicht (kein Tooltip im Bild)", [] { setzeMaus(kv5::fokusPunkt()); }));
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_md_modell"));
    add(tu("MD Handbuch: uebernehmen, eine Ansicht", [] {
        modellMeshUebernehmen(false);
        g_app->mtab.vierAnsichten = false;
    }));
    // in den Karten-Editor (Testraum) und mit einem Klick hinein
    add(tu("MD Handbuch: Karten-Editor", [] { g_app->leftMode = 0; }));
    add(pause(0.3));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(warteBis("MD Handbuch: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    auto undo0 = std::make_shared<std::size_t>(0);
    add(tu("MD Handbuch: Testraum, Kamera schraeg", [undo0] {
        karteDialogeZu();
        (void)stufe3::raumKarteOeffnen("m0raum", false);
        *undo0 = ke::editor().karte.undo.undoAnzahl();
        // der Testraum hat Test-Shader: fuer das Bild eine Textur des Spiels auf alle Brushes (am Ende zurueckgenommen)
        ke::Editor& ed = ke::editor();
        karte::auswahlAufheben(ed.karte, ed.bearbeitung.einstellung);
        karte::auswahlUmkehren(ed.karte, ed.bearbeitung.einstellung);
        (void)karte::shaderSetzen(ed.karte, ed.bearbeitung.einstellung, "textures/yavin/temple_stone2");
        karte::auswahlAufheben(ed.karte, ed.bearbeitung.einstellung);
        ke::Editor& e = ke::editor();
        e.kamera.ursprung = karte::Vektor3f(-120, -150, 200);
        e.kamera.winkel = karte::Vektor3f(-89.0F, 0.0F, 0.0F);
        ke::modellSetzKlasse() = 0;
    }));
    add(pause(1.0));
    add(tu("MD Handbuch: In die Karte", [] {
        g_app->leftMode = 2;
        (void)modellRequisitInKarte();
    }));
    add(warteBis("MD Handbuch: misc_model gesetzt", [] {
        ke::spielPruefen();
        return !ke::requisitWartet() && requisitEntitaeten() >= 1;
    }, 3000));
    add(tu("MD Handbuch: Kamera schraeg auf das Requisit", [] {
        ke::Editor& e = ke::editor();
        for (const auto& en : e.karte.entitaeten()) {
            if (en->wert("model") != vfs()) { continue; }
            float o[3] = {0, 0, 0};
            (void)std::sscanf(en->wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
            e.kamera.ursprung = karte::Vektor3f(o[0] - 70.0F, o[1] - 90.0F, o[2] + 70.0F);
            e.kamera.winkel = karte::Vektor3f(-30.0F, 52.0F, 0.0F);
        }
    }));
    add(tu("MD Handbuch: Maus in die Kamera (kein Tooltip im Bild)", [] {
        const ke::KameraFenster& f = ke::editor().kamFenster;
        setzeMaus(ImVec2{f.bildX + static_cast<float>(f.breite) * 0.9F, f.bildY + static_cast<float>(f.hoehe) * 0.9F});
    }));
    add(pause(1.5));
    add(hilfe2::schrittFoto("h_md_karte"));
    add(tu("MD Handbuch: die Karte wieder wie geladen (Beenden fragt sonst nach der Karte)", [undo0] {
        while (ke::editor().karte.undo.undoAnzahl() > *undo0) { (void)ke::editor().karte.undo.undo(); }
    }));
    return s;
}

// --- Bildzeit (BHED_MODELL_NUR=mdzeit, allein, BHED_D3D_DEBUG=0): Sturmtruppler, Ansehen gegen Modellieren --------
std::vector<Schritt> zeitSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { s.push_back(std::move(x)); }
    add(tu("MD Zeit: LOD 0 fest", [] {
        modellBedienungSetzen(0);
        g_app->mtab.lodWahl = 1;
    }));
    const auto messung = [&](const std::string& was, ModellBereich b, bool alles) {
        add(m1Messblock("Sturmtruppler", "MD Zeit: " + was, [b, alles] {
            g_app->mtab.bereich = b;
            if (b == ModellBereich::Modellieren) {
                modellAuswahlArtSetzen(AuswahlArt::Dreieck);
                modellMeshNetzAktualisieren();
                w::PAuswahl a;
                if (alles) {
                    for (std::size_t i = 0; i < mz().netz.flaechen.size(); ++i) { a.flaechen.push_back(static_cast<int>(i)); }
                }
                mz().auswahl = a;
            }
            g_app->animPlaying = true;   // die Figur bewegt sich: jedes Bild neu
        }));
        add(tu("MD Zeit: Kanten " + was, [] { diag::detail("Knopftest: MD Zeit - " + std::to_string(mz().kantenGezeichnet) + " Kanten der Ueberlagerung, " + zahlen()); }));
    };
    messung("Bereich Ansehen", ModellBereich::Ansehen, false);
    messung("Bereich Modellieren (Polygone, nichts gewaehlt)", ModellBereich::Modellieren, false);
    messung("Bereich Modellieren (alle Polygone gewaehlt)", ModellBereich::Modellieren, true);
    return s;
}

// --- Im Spiel (BHED_MODELL_NUR=mdspiel): Requisit in den Testraum, Schnell-Backen (-meta, -vis -fast, -light -fast) mit und
// ohne Requisit, je ein Foto der Kamera im Spielbild ("Built map as in the game") 1920 x 1440 von einem festen Augpunkt vor
// dem Requisit (Blick +X, Neigung 0, Sichtfeld 80 waagrecht wie cg_fov 80 im 4:3-Fenster: CG_CalcFov rechnet fov_y aus der
// Fensterhoehe). Beide .bsp kommen in den Laufordner (s3bau_mit, s3bau_ohne) fuer den Vergleich im Spiel; das Protokoll nennt
// den Befehl setviewpos fuer denselben Augpunkt.
std::vector<Schritt> spielSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto undo0 = std::make_shared<std::size_t>(0);   // Rueckgaengig-Schritte der Karte vor dem Requisit
    add(tu("MD Spiel: Spielordner, Modus Events", [] {
        // MR4 Punkt 7: BHED_MB_SPIEL=jka - Daten und Regeln des Original-JKA (Vergleich mit dem Spielfoto des Originals)
        if (const char* sp = std::getenv("BHED_MB_SPIEL"); sp != nullptr && std::string(sp) == "jka") { mb1::spielSetzen("jka"); }
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
    }));
    add(pause(0.3));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(warteBis("MD Spiel: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("MD Spiel: Testraum, Blick von oben", [undo0] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();
        (void)stufe3::raumKarteOeffnen("m0raum", false);
        ke::Editor& e = ke::editor();
        *undo0 = e.karte.undo.undoAnzahl();
        e.kamera.ursprung = karte::Vektor3f(-120, -150, 200);
        e.kamera.winkel = karte::Vektor3f(-89.0F, 0.0F, 0.0F);
        ke::modellSetzKlasse() = 0;
        ke::modellSetzKollision() = false;
    }));
    add(pause(1.0));
    neuesRequisit(s, "md_kiste", m0::crate().m.surfaces.empty() ? std::string() : m0::crate().m.surfaces[0].shader);
    kasten(s, 32.0F);
    add(tu("MD Spiel: Deckel eingefuegt und extrudiert, In die Karte", [] {
        waehle(w::PAuswahl{{}, {}, {vieleckBei(2, 32.0F)}}, AuswahlArt::Dreieck);
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Inset);
        mz().betrag = 6.0F;
        modellMeshVorschau();
        modellMeshUebernehmen(false);
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Extrude);
        mz().betrag = 16.0F;
        modellMeshVorschau();
        modellMeshUebernehmen(false);
        (void)modellRequisitInKarte();
    }));
    add(warteBis("MD Spiel: misc_model gesetzt", [] {
        ke::spielPruefen();
        return !ke::requisitWartet() && requisitEntitaeten() >= 1;
    }, 3000));
    add(tu("MD Spiel: Augpunkt 96 vor dem Requisit, 40 hoch, Blick +X, Sichtfeld 80, Bild 1920 x 1440; Lage ins Protokoll", [] {
        ke::Editor& e = ke::editor();
        for (const auto& en : e.karte.entitaeten()) {
            if (en->wert("model") != vfs()) { continue; }
            float o[3] = {0, 0, 0};
            (void)std::sscanf(en->wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
            e.kamera.ursprung = karte::Vektor3f(o[0] - 96.0F, o[1], o[2] + 40.0F);
            e.kamera.winkel = karte::Vektor3f(0.0F, 0.0F, 0.0F);
            // Cmd_SetViewpos_f (g_cmds.cpp): origin[2] -= 25, der Blick liegt dann viewheight hoeher - stehend
            // DEFAULT_MAXS_2 40 + STANDARD_VIEWHEIGHT_OFFSET -4 = 36 (bg_pmove.cpp PM_CheckDuck); im Spiel mit viewpos geprueft
            char b[240];
            std::snprintf(b, sizeof b, "Knopftest: MD Spiel - Augpunkt %g %g %g, setviewpos %g %g %g 0 (Requisit bei %g %g %g)", static_cast<double>(o[0] - 96.0F),
                          static_cast<double>(o[1]), static_cast<double>(o[2] + 40.0F), static_cast<double>(o[0] - 96.0F), static_cast<double>(o[1]),
                          static_cast<double>(o[2] + 40.0F + 25.0F - 36.0F), static_cast<double>(o[0]), static_cast<double>(o[1]), static_cast<double>(o[2]));
            diag::detail(b);
        }
        karte::auswahlAufheben(e.karte, e.bearbeitung.einstellung);   // ohne Auswahlfarbe, Griffe und Arbeitsbereich wie im Spiel
        e.kameraE.sichtfeld = 80.0F;
        e.kameraE.modus = karte::KameraModus::Textur;
        e.kamFenster.messBreite = 1920;
        e.kamFenster.messHoehe = 1440;
        e.filterMaske = karte::EXCLUDE_LIGHTS | karte::EXCLUDE_POINT_ENT;
        e.echtLicht.pauseSekunden = 0.2;
        e.echtLicht.an = true;
        e.echtLicht.backen = true;
        e.echtLicht.spielbild = true;
        ke::spielbildZeitFest(0.0);
        const ke::KameraFenster& f = e.kamFenster;
        setzeMaus(ImVec2{f.bildX - 40.0F, f.bildY + 40.0F});
    }));
    // je Fassung: Schnell-Backen abwarten, Spielbild, Foto und die .bsp (schnell.bsp) in den Laufordner
    const auto fassung = [&](const std::string& name) {
        add(stufe9c::langeWarten("MD Spiel " + name + ": Schnell-Backen fertig", [] {
            const ke::Editor& e = ke::editor();
            if (e.echtLicht.bauStand == 5) {
                meldeFehler("MD Spiel: Schnell-Backen gescheitert");
                return true;
            }
            return e.echtLicht.bauStand == 3 && e.echtLicht.gebackenZeigen;
        }, 400000));
        add(stufe9c::langeWarten("MD Spiel " + name + ": Spielbild da", [] { return ke::editor().echtLicht.spielbildGezeigt && ke::kameraBilderFertig(); }, 20000));
        add(pause(1.0));
        add(tu("MD Spiel " + name + ": Foto und .bsp", [name] {
            stufe9c::Bild b;
            if (!stufe9c::bildHolen(b)) {
                meldeFehler("MD Spiel: kein Kamerabild");
                return;
            }
            const char* o = std::getenv("BHED_FOTOS");
            stufe9c::bmpSchreiben(((o != nullptr && o[0] != '\0') ? std::string(o) : std::filesystem::current_path().string()) + "\\md_spiel_vorschau_" + name + ".bmp", b);
            diag::detail("Knopftest: MD Spiel " + name + " - Vorschau " + std::to_string(b.breite) + " x " + std::to_string(b.hoehe));
            std::error_code ec;
            const std::filesystem::path z = std::filesystem::current_path() / ("s3bau_" + name);
            std::filesystem::create_directories(z, ec);
            std::filesystem::copy_file(std::filesystem::path(ke::editor().echtLicht.bauOrdner) / "maps" / "schnell.bsp", z / "m0raum.bsp",
                                       std::filesystem::copy_options::overwrite_existing, ec);
        }));
        add(pruefe("MD Spiel " + name + ": Foto 1920 x 1440 und .bsp im Laufordner", [name] {
            return ke::editor().kamFenster.breite == 1920 && ke::editor().kamFenster.hoehe == 1440 &&
                   std::filesystem::is_regular_file(std::filesystem::current_path() / ("s3bau_" + name) / "m0raum.bsp");
        }));
    };
    fassung("mit");
    add(tu("MD Spiel: das Requisit wieder aus der Karte (Rueckgaengig bis vor das Einsetzen)", [undo0] {
        while (ke::editor().karte.undo.undoAnzahl() > *undo0) { (void)ke::editor().karte.undo.undo(); }
    }));
    add(pruefe("MD Spiel: kein misc_model des Requisits mehr", [] { return requisitEntitaeten() == 0; }));
    fassung("ohne");
    add(tu("MD Spiel: aufraeumen (Messgroesse, Spielbild-Zeit, Schnell-Backen aus)", [] {
        ke::Editor& e = ke::editor();
        e.kamFenster.messBreite = 0;
        e.kamFenster.messHoehe = 0;
        e.echtLicht.backen = false;
        ke::spielbildZeitFest(-1.0);
    }));
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    if (n == "md") { return modellSchritte(); }
    if (n == "mdspiel") { return spielSchritte(); }
    if (n == "mdzeit") { return zeitSchritte(); }
    if (n == "mdkarte") { return karteSchritte(); }
    return {};
}

}  // namespace md1d
