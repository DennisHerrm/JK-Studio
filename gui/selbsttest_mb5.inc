// selbsttest_mb5.inc - Modell-Block Stufe 3b: Dopesheet, Kurveneditor, Animationswerkzeuge, Aufraeumen, Pfade ziehen
// (BHED_EDITORTEST=modellblock)
//
//   BHED_MODELL_NUR=mb5             Model-Tab mit dem Sturmtruppler, BOTH_WALK1: Dopesheet ueber das Menue, Schluessel mit
//                                   K/I, Raute anklicken, ziehen (Zeit), Rahmen, Kopieren/Einfuegen/Loeschen (Rechtsklick,
//                                   Suche, X wie Blender), Kurveneditor: Interpolation, Tangenten, Schluessel und Henkel
//                                   ziehen; jedes Werkzeug ueber die Suche gegen die Rechnung von bhed/modell/animkurven.h
//                                   (ctest modell_kurven), Animation spiegeln je Knochenpaar und Frame, Posen-Bibliothek,
//                                   Marker, Reduzieren mit Backen auf einen Platz, Pfad des Beckens ziehen; Erreichbarkeit
//                                   jeder Aktion (Menue, Rechtsklick, Suche, Tooltip) und die Tasten gegen die Tabellen
//                                   (BHED_MB_BELEGE = Ordner mit max_hotkeys_de.txt und blender_keys.txt)
//   BHED_MODELL_NUR=kurvenrundlauf  jede Sequenz der animation.cfg von _humanoid (BHED_MB_SPIEL=jka|jk2|md): reduziert,
//                                   gebacken jedes Knochen-Bild innerhalb der Grenze; Schluessel vorher/nachher; Bericht
//                                   mb5_reduktion_<spiel>.txt
// Eingebunden in selbsttest.cpp (nach selbsttest_mb3.inc).
namespace kv5 {

using rig3::griffNach;
using rig3::imMenue;
using rig3::klickMenue;
using rig3::menueLeiste;
using rig3::woIst;

const modell::RigSchicht& sch() {
    static const modell::RigSchicht kLeer;
    const modell::RigSchicht* s = modellRigSchichtJetzt();
    return s != nullptr ? *s : kLeer;
}

// Ein freier Punkt der aktiven Ansicht fuer den Fokus: oben links unter der Kopfzeile (unten liegt der Editor)
ImVec2 fokusPunkt() {
    const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
    return ImVec2{a.lo[0] + 12.0F, a.lo[1] + ImGui::GetFontSize() * 3.5F};
}

const AnimEntry& seq() { return g_app->animList[static_cast<std::size_t>(g_app->animIndex)]; }
int f0() { return seq().firstFrame; }
int rhand() { return boneIndex(g_app->anim, "rhand"); }

int aktion(Str text) { return modellKurvenAktionNach(static_cast<int>(text)); }

// Ein Element in einem Popup (Regler, Suche) anklicken
Schritt klickImPopup(const std::string& label) {
    return klickAn("\"" + label + "\" (Popup)", [label] {
        for (const Element& e : g_elemente) {
            if (e.label == label && e.fenster.find("##Popup_") != std::string::npos) { return e.rect.GetCenter(); }
        }
        meldeFehler("MB5: " + label + " nicht im Popup: " + woIst(label));
        return ImVec2{};
    });
}

const modell::RigSchluessel* schl(int k, int bild) {
    const modell::RigSchicht& s = sch();
    if (k < 0 || static_cast<std::size_t>(k) >= s.knochen.size()) { return nullptr; }
    for (const modell::RigSchluessel& x : s.knochen[static_cast<std::size_t>(k)]) {
        if (x.bild == bild) { return &x; }
    }
    return nullptr;
}

std::size_t zahl(int k) { return k >= 0 && static_cast<std::size_t>(k) < sch().knochen.size() ? sch().knochen[static_cast<std::size_t>(k)].size() : 0U; }

// Fuer Ortsangaben beim Ziehen: nie nullptr (fehlt der Schluessel, meldet die Pruefung danach den Fehler)
const modell::RigSchluessel* schlSicher(int k, int bild) {
    static const modell::RigSchluessel kLeer;
    const modell::RigSchluessel* x = schl(k, bild);
    return x != nullptr ? x : &kLeer;
}

bool gleicheSchicht(const modell::RigSchicht& a, const modell::RigSchicht& b) {
    const std::size_t n = std::max(a.knochen.size(), b.knochen.size());
    for (std::size_t k = 0; k < n; ++k) {
        static const std::vector<modell::RigSchluessel> kLeer;
        const auto& x = k < a.knochen.size() ? a.knochen[k] : kLeer;
        const auto& y = k < b.knochen.size() ? b.knochen[k] : kLeer;
        if (x.size() != y.size()) { return false; }
        for (std::size_t i = 0; i < x.size(); ++i) {
            if (x[i].bild != y[i].bild || x[i].interp != y[i].interp) { return false; }
            for (int c = 0; c < modell::kKanaele; ++c) {
                if (std::fabs(modell::kanalWert(x[i], c) - modell::kanalWert(y[i], c)) > 1e-5F) { return false; }
            }
        }
    }
    return true;
}

// Die Aktion ueber "Alle Aktionen suchen" (X/F3): tippen, die Zeile anklicken (Beschriftung beginnt mit dem Text).
void ueberSuche(std::vector<Schritt>& s, Str text) {
    // der Fokus ohne Klick (ein Klick in die Ansicht waehlte den Griff ab, ein offenes Menue schliesst Esc)
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
    s.push_back(tu("MB5: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; }));
    s.push_back(taste(modellBlender() ? ImGuiKey_F3 : ImGuiKey_X, "Alle Aktionen suchen"));
    s.push_back(pause(0.2));
    // die Zeile der Suche: Aktionen der Stufe 3b mit Gruppe und Taste, sonst der Text
    const int i = aktion(text);
    const std::string zeile = i >= 0 ? modellKurvenAktionSuchText(i) : std::string(tr(text));
    s.push_back(tippe(zeile));
    s.push_back(pause(0.2));
    s.push_back(klickAn("\"" + zeile + "\" in der Suche", [zeile] {
        for (const Element& e : g_elemente) {
            if ((e.label == zeile || e.label.rfind(zeile + " (", 0) == 0) && e.fenster.find("##Popup_") != std::string::npos) { return e.rect.GetCenter(); }
        }
        meldeFehler("MB5: " + zeile + " nicht in der Suche: " + woIst(zeile));
        return ImVec2{};
    }));
    s.push_back(pause(0.3));
}

// Den Regler einer Aktion auf einen Wert stellen und uebernehmen (Knopf "Uebernehmen")
void reglerAuf(std::vector<Schritt>& s, float wert) {
    s.push_back(tu("MB5 Regler auf " + std::to_string(wert), [wert] {
        g_app->mtab.kvReglerWert = wert;
    }));
    s.push_back(pause(0.1));
    s.push_back(klickImPopup(tr(Str::MbKvAnwenden)));
    s.push_back(pause(0.3));
}

int bildJetzt() { return std::clamp(g_app->animFrame, 0, std::max(g_app->anim.bildZahl() - 1, 0)); }

// Eine Pruefung eines Werkzeugs: vorher den Stand merken, dann die Aktion, dann gegen die Rechnung auf dem gemerkten Stand
struct Merk {
    modell::RigSchicht vorher;
    modell::Wahl wahl;
    int bild = 0;
};

Schritt merken(const std::shared_ptr<Merk>& m) {
    return tu("MB5: Stand merken", [m] {
        m->vorher = sch();
        m->wahl = g_app->mtab.kvWahl;
        m->bild = bildJetzt();
    });
}

// Drei Schluessel der rechten Hand (und die mitgedrehten) bei +0/+10/+20, dazu der Kopf
void testSchluessel() {
    ModellTab& t = g_app->mtab;
    const std::string name = seq().name;
    const int f = f0();
    t.dok.aendern("MB5 Testschluessel", [&](modell::Inhalt& in) {
        modell::RigSchicht& s = in.rig.schichten[name];
        s = modell::RigSchicht{};
        s.neuInterp = modell::Interp::Bezier;
        const float ax[3] = {0.0F, 0.3F, 1.0F};
        for (const auto& [b, w] : std::vector<std::pair<int, float>>{{0, 0.0F}, {10, 35.0F}, {20, 10.0F}}) {
            modell::rigSchluesselSetzen(s, rhand(), f + b, modell::drehungUm(ax, w));
            modell::rigSchluesselSetzen(s, boneIndex(g_app->anim, "cranium"), f + b, modell::drehungUm(ax, -w * 0.5F));
        }
    }, false);
    t.kvWahl.clear();
    g_app->modelDirty = true;
}

std::vector<Schritt> uiSchritte(bool handbuch) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string pfad = "models/players/stormtrooper/model.glm";
    add(tu("MB5: Skript fuer den Test", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mb5.txt"); }));
    for (Schritt& x : mb1::laden(pfad)) { add(std::move(x)); }
    add(tu("MB5: Bereich Animation, BOTH_WALK1 angehalten, wie 3ds Max, Rig", [] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Animation;
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        g_app->animPlaying = false;
        g_app->animFrame = f0();
        g_app->animTime = 0.0F;
        (void)modellRigAnlegen();
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(tu("MB5: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; }));
    // --- Dopesheet ueber das Menue Animation ------------------------------------------------------------------------
    add(klickMenue(tr(Str::MbBereichAnimation), true));
    add(pause(0.3));
    add(klickMenue(tr(Str::MbKvMenue), false));
    add(pause(0.3));
    add(klickMenue(tr(Str::MbKvGruppeEditor), false));
    add(pause(0.3));
    add(klickMenue(tr(Str::MbKvDsOeffnen), false));
    add(pause(0.5));
    add(pruefe("MB5 Dopesheet: ueber Animation > Animationswerkzeuge > Editoren geoeffnet, Fenster \"Track View - Dope Sheet\" (wie 3ds Max)", [] {
        const ModellTab& t = g_app->mtab;
        return t.kvOffen && t.kvArt == 0 && t.kvGezeichnet >= ImGui::GetFrameCount() - 2 && t.kvTitel == tr(Str::MbKvDsMax);
    }));
    // --- K setzt einen Schluessel (Max: Keys einstellen) --------------------------------------------------------------
    add(tu("MB5: Griff Hand R, Frame +5, Schicht leer", [] {
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        g_app->animFrame = f0() + 5;
        g_app->mtab.dok.aendern("MB5 leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
        g_app->modelDirty = true;
    }));
    add(tu("MB5: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; }));
    add(taste(ImGuiKey_K, "K (Keys einstellen, wie 3ds Max)"));
    add(pause(0.3));
    add(pruefe("MB5 K: Schluessel fuer rhand (und die Finger) am Frame +5, im Dopesheet als Raute", [] {
        return schl(rhand(), f0() + 5) != nullptr && sch().schluesselZahl() > 1 && g_app->mtab.kvRauten > 0;
    }));
    // --- Rauten waehlen und ziehen --------------------------------------------------------------------------------------
    add(tu("MB5: Testschluessel bei +0/+10/+20 (rhand, cranium), Nur animierte", [] {
        testSchluessel();
        g_app->mtab.kvNurAnimierte = true;
        g_app->mtab.kvNurGewaehlte = false;
        g_app->mtab.kvEinpassen = true;
    }));
    add(pause(0.4));
    add(klickAn("auf die Raute rhand +10", [] {
        ImVec2 p;
        (void)modellDsRauteAufSchirm(rhand(), f0() + 10, p);
        return p;
    }));
    add(pause(0.2));
    add(pruefe("MB5 Dopesheet: Klick auf die Raute waehlt genau diesen Schluessel", [] {
        const auto& w = g_app->mtab.kvWahl;
        return w.size() == 1U && w[0].knochen == rhand() && w[0].bild == f0() + 10;
    }));
    add(mb3::ziehenVonNach("die Raute rhand +10 um 3 Frames nach rechts", [] {
        ImVec2 p;
        (void)modellDsRauteAufSchirm(rhand(), f0() + 10, p);
        return p;
    }, [] {
        ImVec2 p;
        (void)modellDsRauteAufSchirm(rhand(), f0() + 10, p);
        return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 13)), p.y};
    }));
    add(pause(0.3));
    add(pruefe("MB5 Dopesheet: gezogen liegt der Schluessel bei +13 (ganze Frames), +10 ist frei, die Auswahl folgt", [] {
        const auto& w = g_app->mtab.kvWahl;
        return schl(rhand(), f0() + 13) != nullptr && schl(rhand(), f0() + 10) == nullptr && w.size() == 1U && w[0].bild == f0() + 13;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("MB5 Dopesheet: Strg+Z nimmt das Ziehen in einem Schritt zurueck (+10 wieder da)", [] { return schl(rhand(), f0() + 10) != nullptr && schl(rhand(), f0() + 13) == nullptr; }));
    // Rahmen ueber die Zusammenfassung von -1 bis +11
    add(mb3::ziehenVonNach("einen Rahmen ueber die Zusammenfassung (Frames -1 bis +11)", [] {
        ImVec2 p;
        (void)modellKvZeileAufSchirm(0, -1, p);
        return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0()) - 0.6F), p.y - 4.0F};
    }, [] {
        ImVec2 p;
        (void)modellKvZeileAufSchirm(0, -1, p);
        return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 11)), p.y + 4.0F};
    }));
    add(pause(0.3));
    add(pruefe("MB5 Dopesheet: der Rahmen ueber der Zusammenfassung waehlt alle Schluessel bei +0 und +10 (rhand, cranium)", [] {
        const auto& w = g_app->mtab.kvWahl;
        int n = 0;
        for (const auto& x : w) { n += (x.bild == f0() || x.bild == f0() + 10) ? 1 : 0; }
        return n == 4 && w.size() == 4U;
    }));
    // --- Kopieren (Rechtsklick), Einfuegen (Suche), Loeschen ------------------------------------------------------------
    add(klickAn("Rechtsklick in das Feld des Dopesheets", [] {
        ImVec2 p;
        (void)modellKvZeileAufSchirm(0, -1, p);
        return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 15)), p.y};
    }, 1));
    add(pause(0.3));
    add(klickMenue(tr(Str::MbKvGruppeSchluessel), false));
    add(pause(0.3));
    add(klickMenue(tr(Str::MbKvKopieren), false));
    add(pause(0.3));
    add(pruefe("MB5 Rechtsklick > Schluessel > Kopieren: 4 Schluessel in der Ablage", [] { return g_app->mtab.kvAblage.size() == 4U; }));
    add(tu("MB5: Frame +3", [] {
        g_app->animFrame = f0() + 3;
        g_app->modelDirty = true;
    }));
    ueberSuche(s, Str::MbKvEinfuegen);
    add(pruefe("MB5 Suche > Schluessel einfuegen: bei +3 und +13 dieselben Werte wie bei +0 und +10", [] {
        const modell::RigSchluessel* a = schl(rhand(), f0() + 10);
        const modell::RigSchluessel* b = schl(rhand(), f0() + 13);
        const modell::RigSchluessel* c = schl(rhand(), f0() + 3);
        if (a == nullptr || b == nullptr || c == nullptr) { return false; }
        bool gleich = true;
        for (int k = 0; k < modell::kKanaele; ++k) { gleich = gleich && std::fabs(modell::kanalWert(*a, k) - modell::kanalWert(*b, k)) < 1e-6F; }
        return gleich && g_app->mtab.kvWahl.size() == 4U;
    }));
    // Loeschen wie Blender: X mit der Maus ueber dem Editor
    add(tu("MB5: wie Blender, Maus ueber das Dopesheet", [] {
        modellBedienungSetzen(1);
        ImVec2 p;
        (void)modellKvZeileAufSchirm(0, -1, p);
        setzeMaus(ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 22)), p.y});
    }));
    add(pause(0.3));
    add(pruefe("MB5 wie Blender: der Editor heisst \"Dope Sheet\"", [] {
        return g_app->mtab.kvTitel == tr(Str::MbKvDsBl) && g_app->mtab.kvGezeichnet >= ImGui::GetFrameCount() - 2;
    }));
    add(taste(ImGuiKey_X, "X (Dopesheet: Loeschen, wie Blender)"));
    add(pause(0.3));
    add(pruefe("MB5 X ueber dem Dopesheet: die 4 eingefuegten Schluessel sind weg, die anderen bleiben", [] {
        return schl(rhand(), f0() + 3) == nullptr && schl(rhand(), f0() + 13) == nullptr && schl(rhand(), f0() + 10) != nullptr && g_app->mtab.kvWahl.empty();
    }));
    // G wie Blender: Schluessel mit der Maus verschieben
    add(tu("MB5: rhand +20 waehlen", [] { g_app->mtab.kvWahl = {{rhand(), f0() + 20}}; }));
    add(taste(ImGuiKey_G, "G (Schluessel verschieben, wie Blender)"));
    add(pause(0.2));
    add(tu("MB5: Maus 4 Frames nach links", [] {
        ImVec2 p;
        (void)modellKvZeileAufSchirm(0, -1, p);
        setzeMaus(ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 18)), p.y});
    }));
    add(pause(0.3));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.3));
    add(pruefe("MB5 G: der Schluessel liegt 4 Frames frueher (+16)", [] { return schl(rhand(), f0() + 16) != nullptr && schl(rhand(), f0() + 20) == nullptr; }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(tu("MB5: wieder wie 3ds Max", [] { modellBedienungSetzen(0); }));
    add(pause(0.3));
    // --- Kurveneditor ------------------------------------------------------------------------------------------------
    add(tu("MB5: Kurveneditor (Menue des Editors), rhand aufgeklappt, nur Z-Drehung von rhand sichtbar", [] {
        ModellTab& t = g_app->mtab;
        t.kvArt = 1;
        t.kvEinpassen = true;
        t.kvKnochenAuf = {rhand()};
        t.kvKanalAus.clear();
        t.kvKanalAus[rhand()] = 0x7FU & ~(1U << 3U);
        t.kvKanalAus[boneIndex(g_app->anim, "cranium")] = 0x7FU;
        t.kvNurAnimierte = true;
        t.kvWahl = {{rhand(), f0() + 10}};
    }));
    add(pause(0.5));
    add(pruefe("MB5 Kurveneditor: \"Track View - Curve Editor\", Kurven gezeichnet", [] {
        return g_app->mtab.kvTitel == tr(Str::MbKvKeMax) && g_app->mtab.kvKurvenPunkte > 0;
    }));
    ueberSuche(s, Str::MbKvIpLinear);
    add(pruefe("MB5 Interpolation Linear (Suche): der Schluessel +10 ist linear, zwischen +10 und +20 dreht slerp", [] {
        const modell::RigSchluessel* k = schl(rhand(), f0() + 10);
        BoneMatrix m;
        (void)modell::rigWert(sch(), rhand(), static_cast<float>(f0() + 15), m);
        const float ax[3] = {0.0F, 0.3F, 1.0F};
        return k != nullptr && k->interp == modell::Interp::Linear && rig3::drehDiff(m, modell::drehungUm(ax, 22.5F)) < 1e-4F;
    }));
    ueberSuche(s, Str::MbKvIpBezier);
    ueberSuche(s, Str::MbKvTgVektor);   // wie 3ds Max heisst Vektor "Linear" (Zeile "Tangenten: Linear")
    // Vektor-Henkel neben Auto-Punkten loest Continuous Acceleration mit (bezier_output_handle_inner nimmt HD_VECT): die
    // Zeit bleibt ein Drittel, der Wert ist der der Rechnung von Blender
    add(pruefe("MB5 Tangente \"Linear\" (Max) = Vektor: Henkel ein Drittel des Abstands in der Zeit, Werte wie BKE_fcurve_handles_recalc", [] {
        if (sch().knochen.size() <= static_cast<std::size_t>(rhand())) { return false; }
        const auto& v = sch().knochen[static_cast<std::size_t>(rhand())];
        std::vector<modell::RigSchluessel> neu = v;
        modell::henkelNeu(neu);
        for (std::size_t i = 0; i + 1 < v.size(); ++i) {
            if (v[i].bild != f0() + 10) { continue; }
            char z[300];
            std::snprintf(z, sizeof z, "Knopftest: MB5 Vektor - Arten %d/%d, links (%.5f, %.6f), rechts (%.5f, %.6f), neu gerechnet rechts %.6f, Interp %d", static_cast<int>(v[i].henkelL[3]),
                          static_cast<int>(v[i].henkelR[3]), static_cast<double>(v[i].hl[3][0]), static_cast<double>(v[i].hl[3][1]), static_cast<double>(v[i].hr[3][0]),
                          static_cast<double>(v[i].hr[3][1]), static_cast<double>(neu[i].hr[3][1]), static_cast<int>(v[i].interp));
            diag::detail(z);
            return v[i].henkelR[3] == modell::Henkel::Vektor && v[i].henkelL[3] == modell::Henkel::Vektor && std::fabs(v[i].hr[3][0] - 10.0F / 3.0F) < 1e-4F &&
                   std::fabs(v[i].hl[3][0] + 10.0F / 3.0F) < 1e-4F && std::fabs(v[i].hr[3][1] - neu[i].hr[3][1]) < 1e-6F;
        }
        return false;
    }));
    // den Schluessel +10 im Kanal X ziehen: 40 Bildpunkte nach oben
    auto wert0 = std::make_shared<float>(0.0F);
    auto proPx = std::make_shared<float>(0.0F);
    add(tu("MB5: Wert vorher, Werte je Bildpunkt", [=] {
        *wert0 = modell::kanalWert(*schlSicher(rhand(), f0() + 10), 3);
        ImVec2 a;
        ImVec2 b;
        (void)modellKePunktAufSchirm(rhand(), 3, f0() + 10, 0.0F, a);
        (void)modellKePunktAufSchirm(rhand(), 3, f0() + 10, 1.0F, b);
        *proPx = 1.0F / std::max(a.y - b.y, 1e-3F);
    }));
    add(mb3::ziehenVonNach("den Schluessel rhand +10 (Z-Drehung) 40 Punkte nach oben", [=] {
        ImVec2 p;
        (void)modellKePunktAufSchirm(rhand(), 3, f0() + 10, *wert0, p);
        return p;
    }, [=] {
        ImVec2 p;
        (void)modellKePunktAufSchirm(rhand(), 3, f0() + 10, *wert0, p);
        return ImVec2{p.x, p.y - 40.0F};
    }));
    add(pause(0.3));
    add(pruefe("MB5 Kurveneditor: der Wert stieg um 40 Bildpunkte (Werte je Punkt aus dem Feld), die Zeit blieb", [=] {
        const modell::RigSchluessel* k = schl(rhand(), f0() + 10);
        const float soll = *wert0 + 40.0F * *proPx;
        char z[160];
        std::snprintf(z, sizeof z, "Knopftest: MB5 Kurve - %.5f -> %.5f (soll %.5f)", static_cast<double>(*wert0), k != nullptr ? static_cast<double>(modell::kanalWert(*k, 3)) : -9.0,
                      static_cast<double>(soll));
        diag::detail(z);
        return k != nullptr && std::fabs(modell::kanalWert(*k, 3) - soll) < *proPx * 1.5F;
    }));
    // rechten Henkel ziehen: wird Ausgerichtet (Auto) bzw. Frei (Vektor)
    add(mb3::ziehenVonNach("den rechten Henkel rhand +10 nach oben", [] {
        const modell::RigSchluessel* k = schlSicher(rhand(), f0() + 10);
        ImVec2 p;
        (void)modellKePunktAufSchirm(rhand(), 3, f0() + 10, modell::kanalWert(*k, 3) + k->hr[3][1], p);
        return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 10) + k->hr[3][0]), p.y};
    }, [] {
        const modell::RigSchluessel* k = schlSicher(rhand(), f0() + 10);
        ImVec2 p;
        (void)modellKePunktAufSchirm(rhand(), 3, f0() + 10, modell::kanalWert(*k, 3) + k->hr[3][1], p);
        return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 10) + k->hr[3][0]), p.y - 30.0F};
    }));
    add(pause(0.3));
    add(pruefe("MB5 Kurveneditor: der gezogene Vektor-Henkel ist Frei (wie Blender), der linke bleibt Vektor", [] {
        const modell::RigSchluessel* k = schl(rhand(), f0() + 10);
        return k != nullptr && k->henkelR[3] == modell::Henkel::Frei && k->henkelL[3] == modell::Henkel::Vektor;
    }));
    ueberSuche(s, Str::MbKvTgAutoGeklemmt);   // Max "Auto" = Auto Clamped
    ueberSuche(s, Str::MbKvAllesZeigen);      // der gezogene Schluessel lag am oberen Rand (mb5_ui7: der Druck traf das Lineal)
    add(pruefe("MB5 Alles zeigen (Suche): der Schluessel +10 liegt wieder im Feld", [] {
        const modell::RigSchluessel* k = schlSicher(rhand(), f0() + 10);
        ImVec2 p;
        (void)modellKePunktAufSchirm(rhand(), 3, f0() + 10, modell::kanalWert(*k, 3), p);
        return modellKvFeldEnthaelt(p);
    }));
    add(mb3::ziehenVonNach("den rechten Henkel (Auto) nach unten", [] {
        const modell::RigSchluessel* k = schlSicher(rhand(), f0() + 10);
        ImVec2 p;
        (void)modellKePunktAufSchirm(rhand(), 3, f0() + 10, modell::kanalWert(*k, 3) + k->hr[3][1], p);
        return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 10) + k->hr[3][0]), p.y};
    }, [] {
        const modell::RigSchluessel* k = schlSicher(rhand(), f0() + 10);
        ImVec2 p;
        (void)modellKePunktAufSchirm(rhand(), 3, f0() + 10, modell::kanalWert(*k, 3) + k->hr[3][1], p);
        return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 10) + k->hr[3][0]), p.y + 25.0F};
    }));
    add(pause(0.3));
    add(pruefe("MB5 Kurveneditor: ein gezogener Auto-Henkel wird Ausgerichtet, der linke liegt gegenueber (Blender)", [] {
        const modell::RigSchluessel* k = schl(rhand(), f0() + 10);
        if (k == nullptr) { return false; }
        const float kreuz = k->hl[3][0] * k->hr[3][1] - k->hl[3][1] * k->hr[3][0];
        char z[300];
        std::snprintf(z, sizeof z, "Knopftest: MB5 Ausgerichtet - Arten %d/%d, links (%.5f, %.6f), rechts (%.5f, %.6f), Kreuz %.7f", static_cast<int>(k->henkelL[3]),
                      static_cast<int>(k->henkelR[3]), static_cast<double>(k->hl[3][0]), static_cast<double>(k->hl[3][1]), static_cast<double>(k->hr[3][0]),
                      static_cast<double>(k->hr[3][1]), static_cast<double>(kreuz));
        diag::detail(z);
        return k->henkelR[3] == modell::Henkel::Ausgerichtet && k->henkelL[3] == modell::Henkel::Ausgerichtet && std::fabs(kreuz) < 1e-4F && k->hl[3][0] < 0.0F;
    }));
    // --- Werkzeuge auf Schluesseln: Suche, Regler, gegen die Rechnung ----------------------------------------------------
    struct WzFall {
        Str text;
        modell::KurvenWerkzeug art;
        float wert;
    };
    for (const WzFall& w : std::vector<WzFall>{{Str::MbKvWzBreakdown, modell::KurvenWerkzeug::Breakdown, 0.3F},
                                               {Str::MbKvWzNachbar, modell::KurvenWerkzeug::BlendNachbar, -0.5F},
                                               {Str::MbKvWzVorgabe, modell::KurvenWerkzeug::BlendVorgabe, 0.5F},
                                               {Str::MbKvWzPushPull, modell::KurvenWerkzeug::PushPull, 1.6F},
                                               {Str::MbKvWzEase, modell::KurvenWerkzeug::Ease, 0.5F},
                                               {Str::MbKvWzGlaetten, modell::KurvenWerkzeug::Glaetten, 0.8F}}) {
        auto m = std::make_shared<Merk>();
        add(tu(std::string("MB5 ") + tr(w.text) + ": Testschluessel, +10 gewaehlt", [] {
            testSchluessel();
            g_app->mtab.kvWahl = {{rhand(), f0() + 10}, {boneIndex(g_app->anim, "cranium"), f0() + 10}};
        }));
        add(pause(0.2));
        add(merken(m));
        ueberSuche(s, w.text);
        add(pruefe(std::string("MB5 ") + tr(w.text) + ": Regler offen", [] { return g_app->mtab.kvRegler != 0 && g_app->mtab.kvReglerSichtbar >= ImGui::GetFrameCount() - 2; }));
        reglerAuf(s, w.wert);
        add(pruefe(std::string("MB5 ") + tr(w.text) + ": Ergebnis = Rechnung (Faktor " + std::to_string(w.wert) + "), ein Rueckgaengig-Schritt", [m, w] {
            modell::RigSchicht soll = m->vorher;
            (void)modell::kurvenWerkzeug(soll, m->wahl, w.art, w.wert);
            return gleicheSchicht(sch(), soll) && !gleicheSchicht(sch(), m->vorher) && g_app->mtab.kvRegler == 0;
        }));
        add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
        add(pause(0.2));
        add(pruefe(std::string("MB5 ") + tr(w.text) + ": Strg+Z stellt die Schluessel wieder her", [m] { return gleicheSchicht(sch(), m->vorher); }));
    }
    // --- Posen-Schieber am Zeitcursor ----------------------------------------------------------------------------------
    struct PsFall {
        Str text;
        modell::PosenSchieber art;
        float wert;
    };
    for (const PsFall& w : std::vector<PsFall>{{Str::MbKvPsBreakdown, modell::PosenSchieber::Breakdown, 0.25F},
                                               {Str::MbKvPsPush, modell::PosenSchieber::Push, 0.5F},
                                               {Str::MbKvPsRelax, modell::PosenSchieber::Relax, 0.5F},
                                               {Str::MbKvPsBlend, modell::PosenSchieber::Blend, 0.8F},
                                               {Str::MbKvPsRuhe, modell::PosenSchieber::BlendRuhe, 0.5F}}) {
        auto m = std::make_shared<Merk>();
        add(tu(std::string("MB5 ") + tr(w.text) + ": Testschluessel, Frame +6, Griff Hand R", [] {
            testSchluessel();
            g_app->animFrame = f0() + 6;
            modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        }));
        add(pause(0.2));
        add(merken(m));
        ueberSuche(s, w.text);
        reglerAuf(s, w.wert);
        add(pruefe(std::string("MB5 ") + tr(w.text) + ": Pose am Frame +6 = pose_slide-Rechnung auf den Knochen des Griffs", [m, w] {
            const modell::Griff& g = g_app->mtab.rig.griffe[static_cast<std::size_t>(g_app->mtab.griffWahl)];
            std::vector<int> k{g.knochen};
            k.insert(k.end(), g.mit.begin(), g.mit.end());
            if (g.twist >= 0) { k.push_back(g.twist); }
            modell::RigSchicht soll = m->vorher;
            soll.neuInterp = modell::Interp::Bezier;
            (void)modell::poseSchieben(soll, k, m->bild, w.art, w.wert);
            return gleicheSchicht(sch(), soll) && schl(rhand(), f0() + 6) != nullptr;
        }));
    }
    // --- Timing ------------------------------------------------------------------------------------------------------
    {
        auto m = std::make_shared<Merk>();
        add(tu("MB5 Timing: Testschluessel, +10 und +20 gewaehlt, Frame +14", [] {
            testSchluessel();
            g_app->mtab.kvWahl = {{rhand(), f0() + 10}, {rhand(), f0() + 20}};
            g_app->animFrame = f0() + 14;
        }));
        add(merken(m));
        ueberSuche(s, Str::MbKvErsterCursor);
        add(pruefe("MB5 Erster auf aktuellen Frame: +10 -> +14, +20 -> +24", [] { return schl(rhand(), f0() + 14) != nullptr && schl(rhand(), f0() + 24) != nullptr && schl(rhand(), f0() + 10) == nullptr; }));
        ueberSuche(s, Str::MbKvLetzterCursor);
        add(pruefe("MB5 Letzter auf aktuellen Frame: +14 -> +4, +24 -> +14", [] { return schl(rhand(), f0() + 4) != nullptr && schl(rhand(), f0() + 14) != nullptr && schl(rhand(), f0() + 24) == nullptr; }));
        ueberSuche(s, Str::MbKvEinrastenCursor);
        add(pruefe("MB5 Auf aktuellen Frame einrasten: beide auf +14 (einer bleibt)", [] { return schl(rhand(), f0() + 14) != nullptr && schl(rhand(), f0() + 4) == nullptr && zahl(rhand()) == 2U; }));
        add(tu("MB5 Timing: Testschluessel, Frame +5", [] {
            testSchluessel();
            g_app->animFrame = f0() + 5;
        }));
        ueberSuche(s, Str::MbKvZeitEinfuegen);
        reglerAuf(s, 3.0F);
        add(pruefe("MB5 Zwischenframes einfuegen (3 ab +5): +10 -> +13, +20 -> +23, +0 bleibt (rhand und cranium)", [] {
            const int c = boneIndex(g_app->anim, "cranium");
            return schl(rhand(), f0()) != nullptr && schl(rhand(), f0() + 13) != nullptr && schl(rhand(), f0() + 23) != nullptr && schl(c, f0() + 13) != nullptr;
        }));
        ueberSuche(s, Str::MbKvZeitEntfernen);
        reglerAuf(s, 3.0F);
        add(pruefe("MB5 Zwischenframes entfernen (3 ab +5): wieder +10 und +20", [] { return schl(rhand(), f0() + 10) != nullptr && schl(rhand(), f0() + 20) != nullptr && schl(rhand(), f0() + 13) == nullptr; }));
        auto m2 = std::make_shared<Merk>();
        add(tu("MB5 Zeitversatz: +10 gewaehlt", [] { g_app->mtab.kvWahl = {{rhand(), f0() + 10}}; }));
        add(merken(m2));
        ueberSuche(s, Str::MbKvZeitVersatz);
        reglerAuf(s, 4.0F);
        add(pruefe("MB5 Zeitversatz 4: Werte der Kurve 4 Frames spaeter (time_offset)", [m2] {
            modell::RigSchicht soll = m2->vorher;
            (void)modell::zeitVersatz(soll, m2->wahl, 4.0F);
            return gleicheSchicht(sch(), soll) && !gleicheSchicht(sch(), m2->vorher);
        }));
        add(tu("MB5 Zeiten teilen: rhand +0/+10/+20, cranium nur +0 und +7, beide gewaehlt (cranium zuletzt)", [] {
            testSchluessel();
            const int c = boneIndex(g_app->anim, "cranium");
            g_app->mtab.dok.vorschau([&](modell::Inhalt& in) {
                auto& v = in.rig.schichten[seq().name].knochen[static_cast<std::size_t>(c)];
                v.erase(v.begin() + 1, v.end());
                modell::RigSchluessel k = v[0];
                k.bild = f0() + 7;
                v.push_back(k);
                modell::henkelNeu(v);
            }, false);
            g_app->mtab.kvWahl = {{rhand(), f0()}, {c, f0()}};
            g_app->mtab.kvLetzterKnochen = c;
        }));
        ueberSuche(s, Str::MbKvZeitenTeilen);
        add(pruefe("MB5 Schluesselzeiten teilen: rhand und cranium haben 0, 7, 10, 20", [] {
            const int c = boneIndex(g_app->anim, "cranium");
            return schl(rhand(), f0() + 7) != nullptr && schl(c, f0() + 10) != nullptr && schl(c, f0() + 20) != nullptr;
        }));
        add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
        add(pause(0.2));
        ueberSuche(s, Str::MbKvZeitenVomLetzten);
        add(pruefe("MB5 Zeiten vom zuletzt gewaehlten (cranium): rhand genau 0 und 7", [] {
            if (zahl(rhand()) != 2U) { return false; }
            const auto& v = sch().knochen[static_cast<std::size_t>(rhand())];
            return v[0].bild == f0() && v[1].bild == f0() + 7;
        }));
    }
    // --- Werte -------------------------------------------------------------------------------------------------------
    struct WtFall {
        Str text;
        modell::WertArt art;
    };
    for (const WtFall& w : std::vector<WtFall>{{Str::MbKvWtNull, modell::WertArt::Null}, {Str::MbKvWtAktuell, modell::WertArt::AktuellerWert},
                                               {Str::MbKvWtNegieren, modell::WertArt::Negieren}, {Str::MbKvWtUmkehren, modell::WertArt::Umkehren},
                                               {Str::MbKvWtSpiegeln, modell::WertArt::SpiegelnCursor}}) {
        auto m = std::make_shared<Merk>();
        add(tu(std::string("MB5 ") + tr(w.text) + ": Testschluessel, +10 und +20 gewaehlt, Frame +4", [] {
            testSchluessel();
            g_app->mtab.kvWahl = {{rhand(), f0() + 10}, {rhand(), f0() + 20}};
            g_app->animFrame = f0() + 4;
        }));
        add(merken(m));
        ueberSuche(s, w.text);
        add(pruefe(std::string("MB5 ") + tr(w.text) + ": Ergebnis = Rechnung", [m, w] {
            modell::RigSchicht soll = m->vorher;
            (void)modell::werteAendern(soll, m->wahl, w.art, m->bild);
            return gleicheSchicht(sch(), soll) && !gleicheSchicht(sch(), m->vorher);
        }));
    }
    // --- Anim Offset: ein gedrehter Griff dreht die anderen Schluessel mit ---------------------------------------------
    {
        auto m = std::make_shared<Merk>();
        add(tu("MB5 Anim Offset: an, Testschluessel, Frame +10, Griff Hand R", [] {
            testSchluessel();
            g_app->mtab.kvAnimOffset = false;
            g_app->animFrame = f0() + 10;
            g_app->mtab.kvWahl.clear();
            modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        }));
        ueberSuche(s, Str::MbKvAnimOffset);
        add(pause(0.3));
        add(merken(m));
        add(tu("MB5 Anim Offset: den Griff um 20 Grad (Welt-Z) drehen wie am Gizmo", [] {
            const float w[3] = {0.0F, 0.0F, 20.0F};
            modellRigZiehen(2, w);
            modellRigZiehenEnde();
        }));
        add(pause(0.3));
        add(pruefe("MB5 Anim Offset: auch die Schluessel bei +0 und +20 haben die Aenderung von +10 (q_k * q_v^-1 q_n)", [m] {
            const modell::RigSchluessel* v0 = nullptr;
            for (const auto& k : m->vorher.knochen[static_cast<std::size_t>(rhand())]) { v0 = k.bild == f0() ? &k : v0; }
            const modell::RigSchluessel* n0 = schl(rhand(), f0());
            const modell::RigSchluessel* n10 = schl(rhand(), f0() + 10);
            if (v0 == nullptr || n0 == nullptr || n10 == nullptr || !g_app->mtab.kvAnimOffset) { return false; }
            modell::RigSchicht soll = m->vorher;
            const modell::RigSchluessel alt = [&] {
                for (const auto& k : m->vorher.knochen[static_cast<std::size_t>(rhand())]) {
                    if (k.bild == f0() + 10) { return k; }
                }
                return modell::RigSchluessel{};
            }();
            (void)modell::animOffset(soll, rhand(), alt, *n10, {});
            const modell::RigSchluessel* s0 = nullptr;
            for (const auto& k : soll.knochen[static_cast<std::size_t>(rhand())]) { s0 = k.bild == f0() ? &k : s0; }
            if (s0 == nullptr) { return false; }
            float d = 0.0F;
            for (int c = 0; c < 4; ++c) { d = std::max(d, std::fabs(std::fabs(s0->q[c]) - std::fabs(n0->q[c]))); }
            return d < 1e-4F && std::fabs(v0->q[0] - n0->q[0]) > 1e-3F;
        }));
        add(tu("MB5 Anim Offset aus", [] { g_app->mtab.kvAnimOffset = false; }));
    }
    // --- Animation spiegeln: je Bild und Knochenpaar ---------------------------------------------------------------------
    {
        auto vorher = std::make_shared<std::vector<std::vector<BoneMatrix>>>();
        add(tu("MB5 Spiegeln: Schicht leer, Welt jedes Frames von BOTH_WALK1 merken", [vorher] {
            g_app->mtab.dok.aendern("MB5 leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
            vorher->clear();
            std::vector<BoneMatrix> w;
            for (int f = 0; f < seq().numFrames; ++f) {
                g_app->anim.worldMatrices(f0() + f, w);
                vorher->push_back(w);
            }
        }));
        ueberSuche(s, Str::MbKvAnimSpiegeln);
        add(pruefe("MB5 Animation spiegeln: jedes Frame, jeder Knochen = S W(Partner) B(Partner)^-1 S B (Seitenachse aus den Daten)", [vorher] {
            const modell::Rig& r = g_app->mtab.rig;
            BoneMatrix S = BoneMatrix::identity();
            S.m[r.erkennung.seitenAchse][r.erkennung.seitenAchse] = -1.0F;
            float schlimm = 0.0F;
            int paare = 0;
            std::vector<BoneMatrix> w;
            for (int f = 0; f < static_cast<int>(vorher->size()); ++f) {
                modell::rigWelt(g_app->anim, sch(), f0() + f, f0() + f, 0.0F, w);
                for (std::size_t b = 0; b < w.size(); ++b) {
                    const int p = r.partner[b];
                    if (p < 0) { continue; }
                    const BoneMatrix soll = S * (*vorher)[static_cast<std::size_t>(f)][static_cast<std::size_t>(p)] * g_app->anim.bones[static_cast<std::size_t>(p)].basePoseInv * S *
                                            g_app->anim.bones[b].basePose;
                    for (int i = 0; i < 3; ++i) {
                        for (int j = 0; j < 4; ++j) { schlimm = std::max(schlimm, std::fabs(w[b].m[i][j] - soll.m[i][j]) / (j == 3 ? 10.0F : 1.0F)); }
                    }
                    ++paare;
                }
            }
            diag::detail("Knopftest: MB5 Spiegeln - " + std::to_string(paare) + " Knochen-Frames, groesster Abstand " + std::to_string(schlimm));
            int mitPartner = 0;
            for (const int p : r.partner) { mitPartner += p >= 0 ? 1 : 0; }
            return paare == static_cast<int>(vorher->size()) * mitPartner && paare > 0 && schlimm < 2e-3F;
        }));
    }
    // --- Posen-Bibliothek --------------------------------------------------------------------------------------------
    {
        auto welt = std::make_shared<std::vector<BoneMatrix>>();
        add(tu("MB5 Bibliothek: Schicht leer, Frame +8, Name \"schritt\"", [welt] {
            g_app->mtab.dok.aendern("MB5 leer", [](modell::Inhalt& in) {
                in.rig.schichten.clear();
                in.rig.posen.clear();
            }, false);
            g_app->animFrame = f0() + 8;
            std::snprintf(g_app->mtab.kvPoseName, sizeof g_app->mtab.kvPoseName, "%s", "schritt");
            modellRigGriffWaehlen(-1);
            g_app->mtab.knochenWahl = -1;
        }));
        add(pause(0.3));
        add(tu("MB5 Bibliothek: Welt am Frame +8", [welt] { g_app->anim.worldMatrices(f0() + 8, *welt); }));
        ueberSuche(s, Str::MbKvPoseSpeichern);
        add(pruefe("MB5 Pose in die Bibliothek: \"schritt\" im Modelldokument", [] {
            const auto& p = g_app->mtab.dok.inhalt().rig.posen;
            return p.size() == 1U && p[0].name == "schritt" && p[0].welt.size() == g_app->anim.bones.size();
        }));
        add(tu("MB5 Bibliothek: Frame +20", [] { g_app->animFrame = f0() + 20; }));
        ueberSuche(s, Str::MbKvPoseAnwenden);
        add(pause(0.3));
        add(pruefe("MB5 Pose anwenden: Frame +20 steht wie +8 (jeder Knochen)", [welt] {
            std::vector<BoneMatrix> w;
            modell::rigWelt(g_app->anim, sch(), f0() + 20, f0() + 20, 0.0F, w);
            float d = 0.0F;
            for (std::size_t b = 0; b < w.size(); ++b) {
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 4; ++j) { d = std::max(d, std::fabs(w[b].m[i][j] - (*welt)[b].m[i][j])); }
                }
            }
            return d < 2e-3F;
        }));
        ueberSuche(s, Str::MbKvPoseGespiegelt);
        add(pause(0.3));
        add(pruefe("MB5 Pose gespiegelt anwenden: rhumerus steht wie lhumerus gespiegelt", [welt] {
            const modell::Rig& r = g_app->mtab.rig;
            BoneMatrix S = BoneMatrix::identity();
            S.m[r.erkennung.seitenAchse][r.erkennung.seitenAchse] = -1.0F;
            std::vector<BoneMatrix> w;
            modell::rigWelt(g_app->anim, sch(), f0() + 20, f0() + 20, 0.0F, w);
            const int rh = boneIndex(g_app->anim, "rhumerus");
            const int lh = boneIndex(g_app->anim, "lhumerus");
            const BoneMatrix soll = S * (*welt)[static_cast<std::size_t>(lh)] * g_app->anim.bones[static_cast<std::size_t>(lh)].basePoseInv * S * g_app->anim.bones[static_cast<std::size_t>(rh)].basePose;
            float d = 0.0F;
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) { d = std::max(d, std::fabs(w[static_cast<std::size_t>(rh)].m[i][j] - soll.m[i][j])); }
            }
            return d < 2e-3F;
        }));
    }
    // --- Marker der animation.cfg ----------------------------------------------------------------------------------------
    add(tu("MB5 Marker: Testschluessel", [] { testSchluessel(); }));
    ueberSuche(s, Str::MbKvMkGrenzen);
    add(pruefe("MB5 Schluessel an Anfang und Ende: rhand und cranium haben einen am letzten Frame der Sequenz", [] {
        const int letzt = f0() + seq().numFrames - 1;
        return schl(rhand(), letzt) != nullptr && schl(boneIndex(g_app->anim, "cranium"), letzt) != nullptr && schl(rhand(), f0()) != nullptr;
    }));
    ueberSuche(s, Str::MbKvMkSchleife);
    add(pruefe("MB5 Schleife schliessen: das letzte Frame hat die Pose des Schleifen-Frames", [] {
        const int letzt = f0() + seq().numFrames - 1;
        BoneMatrix a;
        BoneMatrix b;
        (void)modell::rigWert(sch(), rhand(), static_cast<float>(f0() + std::max(seq().loopFrame, 0)), a);
        (void)modell::rigWert(sch(), rhand(), static_cast<float>(letzt), b);
        return rig3::drehDiff(a, b) < 1e-5F;
    }));
    ueberSuche(s, Str::MbKvMkWaehlen);
    add(pruefe("MB5 Schluessel der Sequenz waehlen: alle Schluessel der gezeigten Knochen", [] {
        int n = 0;
        for (const auto& v : sch().knochen) { n += static_cast<int>(v.size()); }
        return static_cast<int>(g_app->mtab.kvWahl.size()) == n && n > 0;
    }));
    // --- Aufraeumen ----------------------------------------------------------------------------------------------------
    add(tu("MB5 Aufraeumen: rhand mit zwei gleichen Schluesseln, cranium statisch", [] {
        testSchluessel();
        const int c = boneIndex(g_app->anim, "cranium");
        g_app->mtab.dok.vorschau([&](modell::Inhalt& in) {
            auto& s2 = in.rig.schichten[seq().name];
            auto& v = s2.knochen[static_cast<std::size_t>(rhand())];
            modell::RigSchluessel k = v[1];
            k.bild = f0() + 14;
            v.insert(v.begin() + 2, k);
            k.bild = f0() + 12;
            v.insert(v.begin() + 2, k);
            const modell::RigSchluessel zwei = s2.knochen[static_cast<std::size_t>(c)][1];
            for (auto& x : s2.knochen[static_cast<std::size_t>(c)]) { std::memcpy(x.q, zwei.q, sizeof x.q); }
            v[2].q[0] = -v[2].q[0];
            v[2].q[1] = -v[2].q[1];
            v[2].q[2] = -v[2].q[2];
            v[2].q[3] = -v[2].q[3];
        }, false);
        modellRigGriffWaehlen(-1);
        g_app->mtab.knochenWahl = -1;
    }));
    ueberSuche(s, Str::MbKvQuatFilter);
    add(pruefe("MB5 Quaternion-Filter: das gewendete q (+12) liegt wieder in der Halbkugel des Nachbarn", [] {
        const modell::RigSchluessel* a = schl(rhand(), f0() + 10);
        const modell::RigSchluessel* b = schl(rhand(), f0() + 12);
        return a != nullptr && b != nullptr && a->q[0] * b->q[0] + a->q[1] * b->q[1] + a->q[2] * b->q[2] + a->q[3] * b->q[3] > 0.0F;
    }));
    ueberSuche(s, Str::MbKvUeberfluessig);
    add(pruefe("MB5 Ueberfluessige entfernen (clean_fcurve): +12 (gleich wie +10 und +14) faellt, +14 haelt den Wert bis vor +20 und bleibt",
               [] { return schl(rhand(), f0() + 12) == nullptr && schl(rhand(), f0() + 10) != nullptr && schl(rhand(), f0() + 14) != nullptr; }));
    ueberSuche(s, Str::MbKvStatisch);
    add(pruefe("MB5 Statische Kanaele: cranium (gleich, nicht die Einheit) behaelt einen Schluessel", [] {
        return zahl(boneIndex(g_app->anim, "cranium")) == 1U && zahl(rhand()) == 4U;
    }));
    // --- Reduzieren und Backen auf einen Platz ---------------------------------------------------------------------------
    add(tu("MB5 Reduzieren: Schicht leer, Karte mb5test, Ziel Karten-GLA", [] {
        g_app->mtab.dok.aendern("MB5 leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
        std::snprintf(g_app->mtab.kgKarte, sizeof g_app->mtab.kgKarte, "%s", "mb5test");
        modellGlaZielSetzen(0);
    }));
    add(pause(0.3));
    ueberSuche(s, Str::MbKvReduzieren);
    add(pruefe("MB5 Reduzieren BOTH_WALK1: weniger Schluessel als Frames, gebacken jedes Knochen-Frame innerhalb der Grenze, Bindebasis", [] {
        const modell::ReduzierBericht& b = g_app->mtab.kvBericht;
        diag::detail("Knopftest: MB5 Reduzieren - " + g_app->mtab.rigMeldung);
        return sch().bindeBasis && b.nachher > 0 && b.nachher < b.vorher && b.ueberGrenze == 0 && b.maxAnteil <= 1.0;
    }));
    ueberSuche(s, Str::MbRgBacken);
    add(pruefe("MB5 Backen ueber das Ziel-Feld: der Platz traegt BOTH_WALK1_rig, jedes Knochen-Frame gegen die GLA innerhalb der Grenze", [] {
        const ModellTab& t = g_app->mtab;
        if (t.plaetze.empty() || !t.plaetze[0].belegt || t.plaetze[0].quelle.gla == nullptr) { return false; }
        const GlaAnimation& g = *t.plaetze[0].quelle.gla;
        const int nb = static_cast<int>(g.bones.size());
        int ueber = 0;
        double schlimm = 0.0;
        for (int f = 0; f < seq().numFrames; ++f) {
            for (int b = 0; b < nb; ++b) {
                const std::size_t a = static_cast<std::size_t>(f) * static_cast<std::size_t>(nb) + static_cast<std::size_t>(b);
                const std::size_t q = static_cast<std::size_t>(f0() + f) * static_cast<std::size_t>(nb) + static_cast<std::size_t>(b);
                const std::uint8_t* x = g_app->anim.bonePool.data() + static_cast<std::size_t>(g_app->anim.indexes[q]) * 14U;
                const std::uint8_t* y = g.bonePool.data() + static_cast<std::size_t>(g.indexes[a]) * 14U;
                const double an = modell::drehAbstand(x, y) / modell::kompressionsGrenze(x);
                schlimm = std::max(schlimm, an);
                ueber += (an > 1.0 || modell::verschiebungsStufen(x, y) > 1) ? 1 : 0;
            }
        }
        diag::detail("Knopftest: MB5 Backen - groesster Anteil " + std::to_string(schlimm) + ", ueber " + std::to_string(ueber));
        return g.numFrames == seq().numFrames && ueber == 0;
    }));
    // --- Bewegungspfad des Beckens ziehen ----------------------------------------------------------------------------
    auto pfadVorher = std::make_shared<std::array<float, 3>>();
    add(tu("MB5 Pfad: Schicht leer, Pfad pelvis und rhand, Frame +8, Ansicht Vorne", [pfadVorher] {
        g_app->mtab.dok.aendern("MB5 leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
        g_app->mtab.pfade.clear();
        for (const char* n : {"pelvis", "rhand"}) {
            modellRigGriffWaehlen(-1);
            g_app->mtab.knochenWahl = boneIndex(g_app->anim, n);
            (void)modellPfadHinzu();
        }
        g_app->animFrame = f0() + 8;
        g_app->mtab.gizmo = 0;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(tu("MB5 Pfad: Ort vorher", [pfadVorher] {
        std::vector<BoneMatrix> w;
        g_app->anim.worldMatrices(f0() + 8, w);
        const auto& x = w[static_cast<std::size_t>(boneIndex(g_app->anim, "pelvis"))];
        *pfadVorher = {x.m[0][3], x.m[1][3], x.m[2][3]};
    }));
    auto ziel = std::make_shared<ImVec2>();
    add(mb3::ziehenVonNach("den Punkt des Beckenpfads bei +8 um (40, -30)", [ziel] {
        ImVec2 p;
        (void)modellPfadPunktAufSchirm(0, f0() + 8, p);
        *ziel = ImVec2{p.x + 40.0F, p.y - 30.0F};
        return p;
    }, [ziel] { return *ziel; }));
    add(pause(0.3));
    add(pruefe("MB5 Pfad ziehen: das Becken steht bei +8 unter der Maus (1,5 Punkte), in der Bildebene, ein Schluessel", [pfadVorher, ziel] {
        ImVec2 p;
        if (!modellPfadPunktAufSchirm(0, f0() + 8, p) || schl(boneIndex(g_app->anim, "pelvis"), f0() + 8) == nullptr) { return false; }
        // Weg senkrecht zur Blickrichtung: Bildebene der Ansicht
        const float* vp = g_app->mtab.ansichten[g_app->mtab.aktiv].viewProj;
        float r[3] = {vp[0], vp[1], vp[2]};
        float o[3] = {vp[4], vp[5], vp[6]};
        float n[3] = {r[1] * o[2] - r[2] * o[1], r[2] * o[0] - r[0] * o[2], r[0] * o[1] - r[1] * o[0]};
        const float ln = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        const float* w = g_app->mtab.kvPfadWeg;
        const float aufN = (w[0] * n[0] + w[1] * n[1] + w[2] * n[2]) / std::max(ln, 1e-6F);
        char z[200];
        std::snprintf(z, sizeof z, "Knopftest: MB5 Pfad - Punkt (%.1f, %.1f), Maus (%.1f, %.1f), Weg (%.3f %.3f %.3f), senkrecht %.5f", static_cast<double>(p.x), static_cast<double>(p.y),
                      static_cast<double>(ziel->x), static_cast<double>(ziel->y), static_cast<double>(w[0]), static_cast<double>(w[1]), static_cast<double>(w[2]),
                      static_cast<double>(aufN));
        diag::detail(z);
        (void)pfadVorher;
        return std::hypot(p.x - ziel->x, p.y - ziel->y) < 1.5F && std::fabs(aufN) < 1e-3F;
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("MB5 Pfad: Strg+Z nimmt den Zug zurueck (kein Schluessel am Becken)", [] { return schl(boneIndex(g_app->anim, "pelvis"), f0() + 8) == nullptr; }));
    add(klickAn("auf einen Punkt des Handpfads", [] {
        ImVec2 p;
        (void)modellPfadPunktAufSchirm(1, f0() + 8, p);
        return p;
    }));
    add(pause(0.3));
    add(pruefe("MB5 Pfad der Hand: nur Anzeige bis IK (Meldung), kein Schluessel", [] {
        return g_app->mtab.rigMeldung == tr(Str::MbKvPfadNurAnzeige) && sch().schluesselZahl() == 0;
    }));
    add(tu("MB5: Pfade leer", [] { g_app->mtab.pfade.clear(); }));
    // --- Erreichbarkeit: Menue, Rechtsklick, Suche, Tooltip ---------------------------------------------------------------
    // das Menue einmal oeffnen, dann jede Gruppe der Reihe nach (das naechste Untermenue schliesst das vorige)
    add(taste(ImGuiKey_Escape, "Esc"));
    add(klickMenue(tr(Str::MbBereichAnimation), true));
    add(pause(0.2));
    add(klickMenue(tr(Str::MbKvMenue), false));
    add(pause(0.2));
    for (int g = 0; g < 11; ++g) {
        add(klickAn(std::string("Untermenue ") + std::to_string(g), [g] {
            const Element* e = imMenue(modellKurvenGruppeText(g));
            if (e == nullptr) { meldeFehler(std::string("MB5: Untermenue fehlt: ") + modellKurvenGruppeText(g)); }
            return e != nullptr ? e->rect.GetCenter() : ImVec2{};
        }));
        add(pause(0.3));
        add(pruefe(std::string("MB5 Menue Animation > Animationswerkzeuge > Gruppe ") + std::to_string(g) + ": jede Aktion der Gruppe mit Tooltip", [g] {
            for (int i = 0; i < modellKurvenAktionZahl(); ++i) {
                if (modellKurvenAktionGruppe(i) != g) { continue; }
                if (imMenue(modellKurvenAktionText(i)) == nullptr || std::strlen(modellKurvenAktionHinweis(i)) < 10U) {
                    meldeFehler(std::string("MB5: fehlt im Menue: ") + modellKurvenAktionText(i));
                    return false;
                }
            }
            return true;
        }));
    }
    for (int k = 0; k < 4; ++k) {
        add(taste(ImGuiKey_Escape, "Esc"));
        add(pause(0.1));
    }
    add(tu("MB5: Dopesheet", [] {
        g_app->mtab.kvArt = 0;
        testSchluessel();
    }));
    add(pause(0.3));
    for (int g = 0; g < 11; ++g) {
        add(klickAn("Rechtsklick ins Dopesheet", [] {
            ImVec2 p;
            (void)modellKvZeileAufSchirm(0, -1, p);
            return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 15)), p.y};
        }, 1));
        add(pause(0.2));
        add(klickAn(std::string("Untermenue ") + std::to_string(g), [g] {
            const Element* e = imMenue(modellKurvenGruppeText(g));
            return e != nullptr ? e->rect.GetCenter() : ImVec2{};
        }));
        add(pause(0.3));
        add(pruefe(std::string("MB5 Rechtsklick im Dopesheet > Gruppe ") + std::to_string(g) + ": jede Aktion", [g] {
            for (int i = 0; i < modellKurvenAktionZahl(); ++i) {
                if (modellKurvenAktionGruppe(i) == g && imMenue(modellKurvenAktionText(i)) == nullptr) {
                    meldeFehler(std::string("MB5: fehlt im Rechtsklick: ") + modellKurvenAktionText(i));
                    return false;
                }
            }
            return true;
        }));
        add(taste(ImGuiKey_Escape, "Esc"));
        add(taste(ImGuiKey_Escape, "Esc"));
        add(pause(0.1));
    }
    add(klickAn("Rechtsklick in die Ansicht", [] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        return ImVec2{a.lo[0] + a.groesse[0] * 0.85F, a.lo[1] + a.groesse[1] * 0.2F};
    }, 1));
    add(pause(0.3));
    add(pruefe("MB5 Rechtsklick der Ansicht: Untermenue Animationswerkzeuge", [] { return imMenue(tr(Str::MbKvMenue)) != nullptr; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    // die Suche findet jede Aktion (mit dem Text der Aktion)
    for (int i = 0; i < 64; ++i) {
        add({"MB5 Suche Aktion " + std::to_string(i), [i](int b) {
                 if (i >= modellKurvenAktionZahl()) { return true; }
                 ImGuiIO& io = ImGui::GetIO();
                 if (b == 0) { g_app->mtab.aktionenOffen = true; }
                 if (b == 3) { io.AddInputCharactersUTF8(modellKurvenAktionSuchText(i).c_str()); }
                 if (b == 6) {
                     bool da = false;
                     for (const Element& e : g_elemente) { da = da || (e.label == modellKurvenAktionSuchText(i) && e.fenster.find("##Popup_") != std::string::npos); }
                     pruefeWahr(da, "MB5 Suche findet \"" + modellKurvenAktionSuchText(i) + "\"");
                     io.AddKeyEvent(ImGuiKey_Escape, true);
                 }
                 if (b == 7) { io.AddKeyEvent(ImGuiKey_Escape, false); }
                 return b >= 9;
             }});
    }
    // --- Tasten gegen die Tabellen -------------------------------------------------------------------------------------
    add(pruefe("MB5 Tasten: jede Taste wie 3ds Max und wie Blender steht so in der Tabelle (BHED_MB_BELEGE: max_hotkeys_de.txt, blender_keys.txt)", [] {
        const char* ordner = std::getenv("BHED_MB_BELEGE");
        if (ordner == nullptr) {
            meldeFehler("MB5 Tasten: BHED_MB_BELEGE nicht gesetzt");
            return false;
        }
        const auto lies = [](const std::string& p) {
            std::ifstream f(p, std::ios::binary);
            std::ostringstream o;
            o << f.rdbuf();
            return o.str();
        };
        const std::string max = lies(std::string(ordner) + "/max_hotkeys_de.txt");
        const std::string bl = lies(std::string(ordner) + "/blender_keys.txt");
        const auto trim = [](std::string x) {
            while (!x.empty() && x.front() == ' ') { x.erase(x.begin()); }
            while (!x.empty() && x.back() == ' ') { x.pop_back(); }
            return x;
        };
        int zeilen = 0;
        for (int i = 0; i < modellKurvenAktionZahl(); ++i) {
            const std::string q = modellKurvenAktionQuelle(i);
            std::vector<std::string> teile;
            std::stringstream qs(q);
            std::string stueck;
            while (std::getline(qs, stueck, '|')) { teile.push_back(trim(stueck)); }
            for (const bool blender : {false, true}) {
                const std::string taste = modellKurvenAktionTaste(i, blender);
                if (taste.empty()) { continue; }
                bool belegt = false;
                if (!blender) {
                    // "max_hotkeys_de.txt: <Text> <Taste>" - der Text steht in der Karte von Autodesk, die Taste dahinter
                    for (const std::string& t2 : teile) {
                        if (t2.rfind("max_hotkeys_de.txt: ", 0) != 0) { continue; }
                        const std::string z = t2.substr(20);
                        belegt = max.find(z) != std::string::npos && z.size() > taste.size() && z.compare(z.size() - taste.size(), taste.size(), taste) == 0;
                    }
                } else if (teile.size() >= 3U) {
                    // die letzten drei Teile: "<Bereich> | <Operator> | <Taste> PRESS ..." = eine Zeile "Blender | ..." der Tabelle
                    const std::string z = "Blender | " + teile[teile.size() - 3] + " | " + teile[teile.size() - 2] + " | " + teile.back();
                    std::string tb = taste;
                    const std::size_t plus = tb.find("Shift+");
                    if (plus != std::string::npos) { tb.replace(plus, 6, "Umschalt+"); }
                    belegt = bl.find(z) != std::string::npos && teile.back().rfind(tb + " PRESS", 0) == 0;
                }
                if (!belegt) {
                    meldeFehler(std::string("MB5 Taste ohne Beleg: ") + modellKurvenAktionText(i) + " " + taste + " (" + q + ")");
                    return false;
                }
                ++zeilen;
            }
        }
        diag::detail("Knopftest: MB5 Tasten - " + std::to_string(zeilen) + " Zeilen gegen die Tabellen");
        int erwartet = 0;
        for (int i = 0; i < modellKurvenAktionZahl(); ++i) {
            erwartet += modellKurvenAktionTaste(i, false)[0] != '\0' ? 1 : 0;
            erwartet += modellKurvenAktionTaste(i, true)[0] != '\0' ? 1 : 0;
        }
        return zeilen == erwartet && zeilen > 0;
    }));
    // Blender: I setzt einen Schluessel, Alt+S oeffnet Glaetten (ueber dem Editor)
    add(tu("MB5 Blender: Schicht leer, Griff Hand R, Frame +3", [] {
        modellBedienungSetzen(1);
        g_app->mtab.dok.aendern("MB5 leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        g_app->animFrame = f0() + 3;
    }));
    add(tu("MB5: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; }));
    add(taste(ImGuiKey_I, "I (Schluessel einfuegen, wie Blender)"));
    add(pause(0.3));
    add(pruefe("MB5 Blender I: Schluessel fuer rhand am Frame +3", [] { return schl(rhand(), f0() + 3) != nullptr; }));
    add(tu("MB5 Blender: Testschluessel, +10 gewaehlt, Maus ueber den Editor", [] {
        testSchluessel();
        g_app->mtab.kvWahl = {{rhand(), f0() + 10}};
        ImVec2 p;
        (void)modellKvZeileAufSchirm(0, -1, p);
        setzeMaus(ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 15)), p.y});
    }));
    add(pause(0.3));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_S, "Alt+S (Glaetten, Graph Editor)"));
    add(pause(0.3));
    add(pruefe("MB5 Blender Alt+S ueber dem Editor: der Regler Glaetten ist offen", [] { return g_app->mtab.kvRegler != 0 && g_app->mtab.kvReglerText == static_cast<int>(Str::MbKvWzGlaetten); }));
    add(klickImPopup(tr(Str::MbKvAbbrechen)));
    add(pause(0.2));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_D, "Alt+D (Mischen, Graph Editor)"));
    add(pause(0.3));
    add(pruefe("MB5 Blender Alt+D: der Regler Zum Nachbarn mischen ist offen", [] { return g_app->mtab.kvRegler != 0 && g_app->mtab.kvReglerText == static_cast<int>(Str::MbKvWzNachbar); }));
    add(klickImPopup(tr(Str::MbKvAbbrechen)));
    add(pause(0.2));
    add(pruefe("MB5 Blender: Tangenten heissen wie Blender (Auto Clamped), wie Max anders (Auto)", [] {
        return std::string(modellKurvenAktionText(aktion(Str::MbKvTgAutoGeklemmt))) == tr(Str::MbKvTgAutoGeklemmt);
    }));
    // Klicks in die Ansicht bei offenem Kurveneditor nach Posen-Schiebern (mb5_ui2 stuerzte hier ab, damals mit dem Editor
    // als eigenem Fenster; die Klicks wie in Stufe 3 unten links in der Ansicht)
    add(tu("MB5 Klicks: Testschluessel, Kurveneditor, Griff Hand R, Frame +6", [] {
        modellBedienungSetzen(0);
        testSchluessel();
        g_app->mtab.kvArt = 1;
        g_app->animFrame = f0() + 6;
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
    }));
    ueberSuche(s, Str::MbKvPsBlend);
    reglerAuf(s, 0.8F);
    for (int k = 0; k < 4; ++k) {
        add(klickAn("in die Ansicht unten links", [] {
            const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
            return ImVec2{a.lo[0] + 12.0F, a.lo[1] + a.groesse[1] - 12.0F};
        }));
        add(pause(0.2));
    }
    add(pruefe("MB5 Klicks in die Ansicht nach den Schiebern: kein Absturz, die Figur gezeichnet", [] { return g_app->modellAufrufe > 0; }));
    if (handbuch) {
        add(tu("MB5 Handbuch: wie 3ds Max, Testschluessel, Dopesheet, Pfad Becken und Hand", [] {
            modellBedienungSetzen(0);
            testSchluessel();
            g_app->mtab.kvArt = 0;
            g_app->mtab.kvEinpassen = true;
            g_app->mtab.kvWahl = {{rhand(), f0() + 10}};
            g_app->animFrame = f0() + 6;
            modellAnsichtEinpassenAktiv();
        }));
        add(pause(0.6));
        add(tu("MB5 Handbuch: Maus in die Ansicht (kein Tooltip im Bild)", [] { setzeMaus(fokusPunkt()); }));
        add(pause(0.3));
        add(hilfe2::schrittFoto("h_mb5_dopesheet"));
        add(tu("MB5 Handbuch: Kurveneditor", [] {
            g_app->mtab.kvArt = 1;
            g_app->mtab.kvEinpassen = true;
            g_app->mtab.kvKnochenAuf = {rhand()};
            g_app->mtab.kvKanalAus.clear();
        }));
        add(pause(0.6));
        add(tu("MB5 Handbuch: Maus in die Ansicht (kein Tooltip im Bild)", [] { setzeMaus(fokusPunkt()); }));
        add(pause(0.3));
        add(hilfe2::schrittFoto("h_mb5_kurven"));
        add(klickMenue(tr(Str::MbBereichAnimation), true));
        add(pause(0.2));
        add(klickMenue(tr(Str::MbKvMenue), false));
        add(pause(0.2));
        add(klickMenue(tr(Str::MbKvGruppeWerkzeuge), false));
        add(pause(0.4));
        add(hilfe2::schrittFoto("h_mb5_werkzeuge"));
        add(taste(ImGuiKey_Escape, "Esc"));
        add(taste(ImGuiKey_Escape, "Esc"));
        add(tu("MB5 Handbuch: Reduzieren", [] { g_app->mtab.dok.aendern("MB5 leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false); }));
        ueberSuche(s, Str::MbKvReduzieren);
        add(tu("MB5 Handbuch: Kurveneditor auf die reduzierte Sequenz, nur das Becken (Nur ausgewaehlte)", [] {
            g_app->mtab.kvNurGewaehlte = true;
            g_app->mtab.kvEinpassen = true;
            g_app->mtab.kvKnochenAuf = {boneIndex(g_app->anim, "pelvis")};
            modellRigGriffWaehlen(griffNach(modell::GriffArt::Becken, modell::Seite::Mitte));
        }));
        add(pause(0.6));
        add(tu("MB5 Handbuch: Maus in die Ansicht (kein Tooltip im Bild)", [] { setzeMaus(fokusPunkt()); }));
        add(pause(0.3));
        add(hilfe2::schrittFoto("h_mb5_aufraeumen"));
    }
    add(tu("MB5: zurueck", [] {
        while (g_app->mtab.dok.kannRueckgaengig()) { (void)modellRueckgaengig(); }
        g_app->mtab.kvOffen = false;
        g_app->mtab.kvWahl.clear();
        g_app->mtab.pfade.clear();
        g_app->mtab.bereich = ModellBereich::Ansehen;
        modellBedienungSetzen(0);
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

// --- Reduzieren ueber alle Sequenzen von _humanoid ---------------------------------------------------------------------
Schritt rundlauf(const std::string& spiel) {
    struct Zustand {
        GlaAnimation g;
        std::vector<AnimEntry> seqs;
        std::size_t i = 0;
        long long vorher = 0, nachher = 0, eineStufe = 0;
        int linear = 0, bezier = 0, statisch = 0, ueber = 0, sequenzen = 0;
        double maxAnteil = 0.0;
        std::string schlimmster;
        std::string bericht;
        std::string pfad;
        std::chrono::steady_clock::time_point anfang{};
    };
    auto z = std::make_shared<Zustand>();
    Schritt s{"MB5 Reduzieren (" + spiel + "): jede Sequenz von _humanoid", [=](int b) {
                  if (b == 0) {
                      z->pfad = "models/players/_humanoid/_humanoid.gla";
                      std::string bytes;
                      std::string cfg;
                      if (!readFromArchives(z->pfad, bytes) || !readGla(bytes, z->g) || !readFromArchives("models/players/_humanoid/animation.cfg", cfg)) {
                          meldeFehler("MB5 Reduzieren " + spiel + ": _humanoid.gla oder animation.cfg fehlt");
                          return true;
                      }
                      std::set<std::pair<int, int>> schon;
                      for (const AnimEntry& e : parseAnimationCfg(cfg)) {
                          if (e.numFrames <= 0 || e.firstFrame < 0 || e.firstFrame + e.numFrames > z->g.numFrames) { continue; }
                          if (!schon.insert({e.firstFrame, e.numFrames}).second) { continue; }
                          z->seqs.push_back(e);
                      }
                      z->anfang = std::chrono::steady_clock::now();
                      diag::detail("Knopftest: MB5 Reduzieren " + spiel + " - " + std::to_string(z->seqs.size()) + " Sequenzen, " + std::to_string(z->g.numFrames) + " Frames");
                      return false;
                  }
                  if (z->i < z->seqs.size()) {
                      const AnimEntry& e = z->seqs[z->i++];
                      modell::RigSchicht red;
                      modell::ReduzierBericht br;
                      if (!modell::rigReduzieren(z->g, modell::RigSchicht{}, e.firstFrame, e.numFrames, red, &br)) {
                          z->bericht += e.name + ": " + br.fehler + "\n";
                          ++z->ueber;
                          return false;
                      }
                      ++z->sequenzen;
                      z->vorher += br.vorher;
                      z->nachher += br.nachher;
                      z->eineStufe += br.eineStufe;
                      z->linear += br.linear;
                      z->bezier += br.bezier;
                      z->statisch += br.statisch;
                      z->ueber += br.ueberGrenze;
                      if (br.maxAnteil > z->maxAnteil) {
                          z->maxAnteil = br.maxAnteil;
                          z->schlimmster = e.name + " " + br.schlimmster;
                      }
                      char zeile[300];
                      std::snprintf(zeile, sizeof zeile, "%s: %d Frames, %lld -> %lld Schluessel (%.1f %%), linear %d, Bezier %d, statisch %d, Anteil %.3f, eine Stufe %lld, ueber %d\n",
                                    e.name.c_str(), e.numFrames, br.vorher, br.nachher, 100.0 * static_cast<double>(br.nachher) / static_cast<double>(std::max(br.vorher, 1LL)),
                                    br.linear, br.bezier, br.statisch, br.maxAnteil, br.eineStufe, br.ueberGrenze);
                      z->bericht += zeile;
                      return false;
                  }
                  const double sek = std::chrono::duration<double>(std::chrono::steady_clock::now() - z->anfang).count();
                  char zeile[600];
                  std::snprintf(zeile, sizeof zeile,
                                "MB5 Reduzieren %s: %d Sequenzen von %s, Schluessel %lld -> %lld (%.1f %%), Knochen linear %d / Bezier %d / statisch %d; groesster Anteil an der "
                                "Grenze %.4f (%s), Knochen-Frames eine Stufe Verschiebung %lld, ueber der Grenze %d; %.0f s",
                                spiel.c_str(), z->sequenzen, z->pfad.c_str(), z->vorher, z->nachher, 100.0 * static_cast<double>(z->nachher) / static_cast<double>(std::max(z->vorher, 1LL)),
                                z->linear, z->bezier, z->statisch, z->maxAnteil, z->schlimmster.c_str(), z->eineStufe, z->ueber, sek);
                  diag::detail(std::string("Knopftest: ") + zeile);
                  {
                      std::ofstream f("mb5_reduktion_" + spiel + ".txt", std::ios::binary);
                      f << zeile << "\n\nJe Sequenz:\n" << z->bericht;
                  }
                  pruefeWahr(z->sequenzen > 0 && z->ueber == 0 && z->maxAnteil <= 1.0,
                             "MB5 Reduzieren " + spiel + ": jede Sequenz reduziert, gebacken jedes Knochen-Frame innerhalb der Grenze");
                  pruefeWahr(z->nachher < z->vorher, "MB5 Reduzieren " + spiel + ": weniger Schluessel als Frames");
                  return true;
              }};
    s.eigeneZeitgrenze = true;
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    std::vector<Schritt> s;
    if (n == "kurvenrundlauf") {
        const char* sp = std::getenv("BHED_MB_SPIEL");
        const std::string spiel = sp != nullptr ? sp : "md";
        s.push_back(tu("MB5 Reduzieren: Spiel " + spiel, [=] {
            if (spiel != "md") { mb1::spielSetzen(spiel); }
            if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        }));
        s.push_back(pause(0.3));
        s.push_back(rundlauf(spiel));
        return s;
    }
    if (n == "mb5") { return uiSchritte(false); }
    return s;
}

std::vector<Schritt> handbuchSchritte() { return uiSchritte(true); }

}  // namespace kv5
