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

// --- Im Spiel (BHED_MODELL_NUR=mdspiel): Requisit in den Testraum, Bau (-meta), Vorschau-Foto der Kamera von einem festen
// Punkt vor dem Requisit (Blick +X, Neigung 0, Sichtfeld 80 waagrecht wie cg_fov 80); die Lage steht im Protokoll fuer
// setviewpos im Spiel (g_cmds.cpp Cmd_SetViewpos_f: der Blick liegt auf dem angegebenen Punkt)
std::vector<Schritt> spielSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("MD Spiel: Spielordner, Modus Events", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
    }));
    add(pause(0.3));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(warteBis("MD Spiel: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("MD Spiel: Testraum, Blick von oben", [] {
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
    add(tu("MD Spiel: Kamera 96 vor dem Requisit, 40 hoch, Blick +X, Sichtfeld 80; Lage ins Protokoll", [] {
        ke::Editor& e = ke::editor();
        for (const auto& en : e.karte.entitaeten()) {
            if (en->wert("model") != vfs()) { continue; }
            float o[3] = {0, 0, 0};
            (void)std::sscanf(en->wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
            e.kamera.ursprung = karte::Vektor3f(o[0] - 96.0F, o[1], o[2] + 40.0F);
            e.kamera.winkel = karte::Vektor3f(0.0F, 0.0F, 0.0F);
            e.kameraE.sichtfeld = 80.0F;
            e.kamera.projektionAktualisieren(e.kameraE);
            char b[200];
            std::snprintf(b, sizeof b, "Knopftest: MD Spiel - setviewpos %g %g %g 0 (Requisit bei %g %g %g)", static_cast<double>(o[0] - 96.0F), static_cast<double>(o[1]),
                          static_cast<double>(o[2] + 40.0F), static_cast<double>(o[0]), static_cast<double>(o[1]), static_cast<double>(o[2]));
            diag::detail(b);
        }
        const ke::KameraFenster& f = e.kamFenster;
        setzeMaus(ImVec2{f.bildX - 40.0F, f.bildY + 40.0F});
    }));
    add(pause(1.5));
    add(hb::fotoTeil("md_spiel_vorschau", [] {
        const ke::KameraFenster& f = ke::editor().kamFenster;
        return ImRect{f.bildX, f.bildY, f.bildX + static_cast<float>(f.breite), f.bildY + static_cast<float>(f.hoehe)};
    }));
    add(tu("MD Spiel: Bau (q3map2 -meta)", [] {
        ke::Editor& e = ke::editor();
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{"MD bsp", {"[q3map2] -meta \"[MapFile]\""}});
        ke::bauStarten(e.bauProjekt.eintraege.size() - 1);
    }));
    add(warteBis("MD Spiel: Bau fertig", [] { return !ke::bauLaeuft(); }, 30000));
    add(pruefe("MD Spiel: .bsp gebaut", [] { return std::filesystem::is_regular_file(std::filesystem::current_path() / "s3bau" / "m0raum.bsp"); }));
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
