// selbsttest_mb6.inc - Modell-Block Stufe 4: IK/FK, Fussrolle, Einrasten, Space Switching, Pfade an Haenden, Fuessen und
// Saebelspitze, Reduzieren mit Auswahl, Posen-Bibliothek als Datei (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=mb6)
//
//   Erreichbarkeit  jede IK/FK-Aktion im Menue Animation (Gruppe IK/FK) mit Tooltip, in der Suche, im Rechtsklick des Griffs;
//                   das Rollout IK/FK geht auf, wenn ein Glied gewaehlt wird (wie Max: Bewegung > IK-Solver)
//   wie 3ds Max     W an der Hand ist IK (Ziel erreicht, Gelenk in der Pol-Ebene, Laengen gleich, FK-Schluessel am Bild, ein
//                   Rueckgaengig-Schritt); "Glied auf IK/FK" ohne Sprung (Knochen und Bildpunkte); rot: ohne Einrasten ist
//                   der Sprung messbar; E am Oberarm unter IK wechselt auf FK; Pol (W), Fussrolle (E an Ferse/Zehe)
//   Raum            "Saebel an die andere Hand": kein Sprung, danach folgt die rechte Hand der linken starr
//   Pfade           an Hand, Fuss und Saebelspitze ziehen: der Punkt steht unter der Maus, IK-Schluessel, Strg+Z
//   wie Blender     G in FK: Meldung; IK -> FK, Regler IK-FK auf 0 ohne Sprung; G bewegt das Ziel; rot: Regler ohne Einrasten
//   Reduzieren      nur gewaehlte Knochen in einer Spanne: die anderen Schluessel bleiben, gebacken innerhalb der Grenze
//   Posen-Datei     speichern (BHED_POSEN oder Laufordner\mb6_posen), Bibliothek leeren, ueber das Menue laden
//   ikrundlauf      BHED_MODELL_NUR=ikrundlauf, BHED_MB_SPIEL=jka|jk2|md: IK <-> FK an jedem Frame jeder Sequenz (siehe unten)
// Eingebunden in selbsttest.cpp (nach selbsttest_mb5.inc).
namespace ik6 {

using rig3::griffNach;
using rig3::imMenue;
using rig3::klickMenue;
using rig3::woIst;
using kv5::f0;
using kv5::sch;
using kv5::schl;
using kv5::seq;
using kv5::ueberSuche;

constexpr int kArmR = static_cast<int>(modell::IkArt::ArmRechts);
constexpr int kBeinR = static_cast<int>(modell::IkArt::BeinRechts);

int knochen(const char* n) { return boneIndex(g_app->anim, n); }

const modell::IkSpur* spur(int art) {
    const int i = modell::ikSpurIndex(sch(), static_cast<modell::IkArt>(art));
    return i >= 0 ? &sch().ik[static_cast<std::size_t>(i)] : nullptr;
}

bool ikSchluessel(int art, int bild) {
    const modell::IkSpur* s = spur(art);
    if (s == nullptr) { return false; }
    return std::any_of(s->ziel.begin(), s->ziel.end(), [bild](const modell::RigSchluessel& k) { return k.bild == bild; });
}

void ort(const BoneMatrix& m, float p[3]) {
    p[0] = m.m[0][3];
    p[1] = m.m[1][3];
    p[2] = m.m[2][3];
}

float abstand(const float a[3], const float b[3]) { return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2])); }

float weltDiff(const std::vector<BoneMatrix>& a, const std::vector<BoneMatrix>& b) {
    float d = a.size() == b.size() ? 0.0F : 1e9F;
    for (std::size_t k = 0; k < a.size() && k < b.size(); ++k) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 4; ++j) { d = std::max(d, std::fabs(a[k].m[i][j] - b[k].m[i][j])); }
        }
    }
    return d;
}

// Stand vor einer Aktion: Welt, Schicht und das Bild der aktiven Ansicht
struct Stand {
    std::vector<BoneMatrix> welt;
    modell::RigSchicht schicht;
    std::vector<unsigned char> pixel;
    int w = 0;
    int h = 0;
    int bild = 0;
};

Schritt merke(const std::shared_ptr<Stand>& st, const std::string& was) {
    return tu("MB6: Stand merken (" + was + ")", [st] {
        st->welt = g_app->modellWelt;
        st->schicht = sch();
        st->bild = g_app->animFrame;
        (void)modellAnsichtPixel(g_app->mtab.aktiv, st->pixel, st->w, st->h);
    });
}

// Wie viele Bildpunkte weichen um mehr als 8 Stufen ab?
int pixelAnders(const Stand& st) {
    std::vector<unsigned char> p;
    int w = 0;
    int h = 0;
    if (!modellAnsichtPixel(g_app->mtab.aktiv, p, w, h) || w != st.w || h != st.h || p.size() != st.pixel.size()) { return -1; }
    int n = 0;
    for (std::size_t i = 0; i + 3 < p.size(); i += 4) {
        for (int c = 0; c < 3; ++c) {
            if (std::abs(static_cast<int>(p[i + static_cast<std::size_t>(c)]) - static_cast<int>(st.pixel[i + static_cast<std::size_t>(c)])) > 8) {
                ++n;
                break;
            }
        }
    }
    return n;
}

// "ohne Sprung": Knochen (Welt) unter 0,001 Einheiten und kein Bildpunkt anders
Schritt ohneSprung(const std::shared_ptr<Stand>& st, const std::string& was) {
    return pruefe("MB6 " + was + ": ohne Sprung (Weltmatrizen < 0,001, hoechstens 8 Bildpunkte gekippt)", [st, was] {
        const float d = weltDiff(st->welt, g_app->modellWelt);
        const int px = pixelAnders(*st);
        diag::detail("Knopftest: MB6 " + was + " - Weltmatrizen " + std::to_string(d) + ", Bildpunkte anders " + std::to_string(px) + " von " +
                     std::to_string(st->w * st->h));
        // Bildpunkte: hoechstens 8 duerfen kippen (Kanten bei Aenderungen unter 1/10000 Einheit, mb6_ui7: 1 Punkt bei 0,000024);
        // ein Sprung aendert hunderte (rot: 129 und 270)
        return d < 1e-3F && px >= 0 && px <= 8;
    });
}

// Die Kette des rechten Arms pruefen: Hand am Ziel, Ellbogen in der Ebene Schulter/Ziel/Pol, Laengen wie vorher
bool kettePasst(int art, const std::vector<BoneMatrix>& vorher, std::string& bericht) {
    const modell::IkSpur* s = spur(art);
    const auto& w = g_app->modellWelt;
    if (s == nullptr || w.size() != vorher.size()) { return false; }
    const modell::IkZiel z = modell::ikZielAm(*s, static_cast<float>(g_app->animFrame), w);
    float a0[3], b0[3], c0[3], a[3], b[3], c[3], t[3];
    ort(vorher[static_cast<std::size_t>(s->oben.knochen)], a0);
    ort(vorher[static_cast<std::size_t>(s->unten.knochen)], b0);
    ort(vorher[static_cast<std::size_t>(s->ende.knochen)], c0);
    ort(w[static_cast<std::size_t>(s->oben.knochen)], a);
    ort(w[static_cast<std::size_t>(s->unten.knochen)], b);
    ort(w[static_cast<std::size_t>(s->ende.knochen)], c);
    ort(z.ziel, t);
    const float u[3] = {t[0] - a[0], t[1] - a[1], t[2] - a[2]};
    const float p[3] = {z.pol[0] - a[0], z.pol[1] - a[1], z.pol[2] - a[2]};
    float n[3] = {u[1] * p[2] - u[2] * p[1], u[2] * p[0] - u[0] * p[2], u[0] * p[1] - u[1] * p[0]};
    const float ln = std::max(std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]), 1e-9F);
    const float ebene = std::fabs((b[0] - a[0]) * n[0] + (b[1] - a[1]) * n[1] + (b[2] - a[2]) * n[2]) / ln;
    const float laenge = std::max(std::fabs(abstand(a, b) - abstand(a0, b0)), std::fabs(abstand(b, c) - abstand(b0, c0)));
    char z2[240];
    std::snprintf(z2, sizeof z2, "Hand %.4f vom Ziel, Ellbogen %.4f aus der Pol-Ebene, Laengen %.4f, Hand bewegt %.2f", static_cast<double>(abstand(c, t)),
                  static_cast<double>(ebene), static_cast<double>(laenge), static_cast<double>(abstand(c, c0)));
    bericht = z2;
    return abstand(c, t) < 0.01F && ebene < 0.01F && laenge < 0.01F;
}

// Eine Aktion des Menues IK/FK ueber den Rechtsklick in die Ansicht (Untermenue Rig)
std::vector<Schritt> ueberRechtsklick(Str text) {
    std::vector<Schritt> s;
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
    s.push_back(klickAn("Rechtsklick in die Ansicht", [] { return kv5::fokusPunkt(); }, 1));
    s.push_back(pause(0.3));
    s.push_back(klickMenue(tr(Str::MbRgGruppeRig), false));
    s.push_back(pause(0.3));
    s.push_back(klickMenue(tr(Str::MbIkGruppe), false));
    s.push_back(pause(0.3));
    s.push_back(klickMenue(tr(text), false));
    s.push_back(pause(0.3));
    return s;
}

// Die Suche "Alle Aktionen" mit der Taste des Modus beim Ausfuehren (X wie 3ds Max, F3 wie Blender) - kv5::ueberSuche
// waehlt die Taste schon beim Aufbau der Schritte
Schritt tasteSuche() {
    auto k = std::make_shared<ImGuiKey>(ImGuiKey_X);
    return {"Taste Alle Aktionen suchen (X / F3)", [k](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    *k = modellBlender() ? ImGuiKey_F3 : ImGuiKey_X;
                    io.AddKeyEvent(*k, true);
                    return false;
                }
                if (b == 1) { io.AddKeyEvent(*k, false); return false; }
                return b >= 4;
            }};
}

void sucheIk(std::vector<Schritt>& s, Str text) {
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
    s.push_back(tu("MB6: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; }));
    s.push_back(tasteSuche());
    s.push_back(pause(0.2));
    s.push_back(pruefe("MB6 Suche: Alle Aktionen suchen ist offen (X / F3)", [] { return g_app->mtab.aktionenSichtbar; }));
    const std::string zeile = tr(text);
    s.push_back(tippe(zeile));
    s.push_back(pause(0.2));
    s.push_back(klickAn("\"" + zeile + "\" in der Suche", [zeile] {
        for (const Element& e : g_elemente) {
            if ((e.label == zeile || e.label.rfind(zeile + " (", 0) == 0) && e.fenster.find("##Popup_") != std::string::npos) { return e.rect.GetCenter(); }
        }
        meldeFehler("MB6: " + zeile + " nicht in der Suche: " + woIst(zeile));
        return ImVec2{};
    }));
    s.push_back(pause(0.3));
}

std::vector<Schritt> uiSchritte(bool handbuch) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string pfad = "models/players/stormtrooper/model.glm";
    add(tu("MB6: Skript fuer den Test, Posen-Ordner im Laufordner", [] {
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mb6.txt");
        if (std::getenv("BHED_POSEN") == nullptr) {
            const std::u8string o = (std::filesystem::current_path() / "mb6_posen").u8string();
            (void)_putenv_s("BHED_POSEN", std::string(o.begin(), o.end()).c_str());
        }
    }));
    for (Schritt& x : mb1::laden(pfad)) { add(std::move(x)); }
    add(tu("MB6: Bereich Animation, BOTH_WALK1 angehalten, wie 3ds Max, Rig, Schicht leer", [] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Animation;
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        g_app->animPlaying = false;
        g_app->animFrame = f0() + 10;
        g_app->animTime = 0.0F;
        (void)modellRigAnlegen();
        g_app->mtab.dok.aendern("MB6 leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
        g_app->mtab.pfade.clear();
        g_app->mtab.ikAutoEinrasten = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    // --- Erreichbarkeit: Menue Animation, Gruppe IK/FK -------------------------------------------------------------
    add(taste(ImGuiKey_Escape, "Esc"));
    add(klickMenue(tr(Str::MbBereichAnimation), true));
    add(pause(0.3));
    const auto menueGruppen = [](bool ik) {
        return [ik] {
            int n = 0;
            for (int i = 0; i < modellRigAktionZahl(); ++i) {
                if (modellRigAktionGruppe(i) >= 6) { continue; }   // Stufe 6/5: eigene Untermenues Auto-Physik, Retargeting (selbsttest_ph/rt.inc)
                if ((modellRigAktionGruppe(i) == 5) != ik) { continue; }
                const std::string text = modellRigAktionText(i);
                if (imMenue(text) == nullptr || std::string(modellRigAktionHinweis(i)).size() < 10U) {
                    meldeFehler("MB6: fehlt im Menue oder ohne Tooltip: " + text + " " + woIst(text));
                    return false;
                }
                ++n;
            }
            diag::detail("Knopftest: MB6 Menue - " + std::to_string(n) + (ik ? " IK/FK-Aktionen" : " Rig-Aktionen") + " mit Tooltip");
            return n > 0;
        };
    };
    add(pruefe("MB6 Menue Animation: jede Rig-Aktion steht da, mit Tooltip", menueGruppen(false)));
    add(klickMenue(tr(Str::MbIkGruppe), false));
    add(pause(0.4));
    add(pruefe("MB6 Menue Animation > IK/FK: jede IK/FK-Aktion steht da, mit Tooltip", menueGruppen(true)));
    add(taste(ImGuiKey_Escape, "Esc"));
    // das Rollout geht auf, wenn ein Glied gewaehlt wird (wie Max)
    add(tu("MB6: Griff Hand R waehlen, das Befehlsfeld nach oben (dort folgt IK/FK auf das Control Rig)", [] {
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        for (ImGuiWindow* w : GImGui->Windows) {
            if (std::strstr(w->Name, "mtrollouts") != nullptr) { ImGui::SetScrollY(w, 0.0F); }
        }
    }));
    add(pause(0.4));
    add(rig3::rollenZu(tr(Str::MbIkRollout), "mtrollouts"));
    add(pause(0.4));
    add(pruefe("MB6 Rollout IK/FK: geht mit dem Glied auf (wie das IK-Solver-Rollout von 3ds Max), mit IK aktiviert / IK/FK einrasten", [] {
        diag::detail("Knopftest: MB6 Rollout - offen im Bild " + std::to_string(g_app->mtab.ikRolloutBild) + " (jetzt " + std::to_string(ImGui::GetFrameCount()) + "), " +
                     tr(Str::MbIkAn) + " " + woIst(tr(Str::MbIkAn)));
        return g_app->mtab.ikRolloutBild >= ImGui::GetFrameCount() - 2 && finde(tr(Str::MbIkAn), "", 0, false) != nullptr;
    }));
    // Rechtsklick am Griff im Explorer: die IK/FK-Aktionen
    add(rig3::rollenZu(std::string(tr(Str::MbRgHand)) + " " + tr(Str::MbRgR), "mtbaum"));
    add(pause(0.3));
    add(klickAn("Rechtsklick auf Hand R im Explorer", [] {
        const Element* e = finde(std::string(tr(Str::MbRgHand)) + " " + tr(Str::MbRgR), "", 0, false);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    }, 1));
    add(pause(0.3));
    add(klickMenue(tr(Str::MbIkGruppe), false));
    add(pause(0.3));
    add(pruefe("MB6 Rechtsklick am Griff Hand R: die IK/FK-Aktionen (Glied auf IK, IK auf FK einrasten, Raum)", [] {
        return imMenue(tr(Str::MbIkAufIk)) != nullptr && imMenue(tr(Str::MbIkZielAufFk)) != nullptr && imMenue(tr(Str::MbIkRaumHand)) != nullptr;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    // --- 3ds Max: W an der Hand ist IK -----------------------------------------------------------------------------
    auto st = std::make_shared<Stand>();
    for (int k = 0; k < 2; ++k) {
        add(klickAn("in die Ansicht (Fokus, ein offenes Menue nimmt den ersten Klick)", [] { return kv5::fokusPunkt(); }));
        add(pause(0.1));
    }
    add(tu("MB6 Max W: Griff Hand R, W, Frame +10", [] {
        g_app->animFrame = f0() + 10;
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        g_app->mtab.gizmo = 1;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(merke(st, "vor W"));
    add(mb3::ziehenVonNach("den Griff Y des Gizmos an der Hand 20 Punkte nach rechts", [] {
        ImVec2 p;
        (void)modellGizmoGriffOrt(2, p);
        return p;
    }, [] {
        ImVec2 p;
        (void)modellGizmoGriffOrt(2, p);
        return ImVec2{p.x + 20.0F, p.y};
    }));
    add(pause(0.4));
    add(pruefe("MB6 Max W an der Hand: IK - Hand am Ziel, Ellbogen in der Pol-Ebene, Laengen gleich, IK-Schluessel und FK-Schluessel am Frame, ein Rueckgaengig-Schritt", [st] {
        std::string b;
        const bool k = kettePasst(kArmR, st->welt, b);
        diag::detail("Knopftest: MB6 Max W - " + b + ", " + g_app->mtab.dok.rueckgaengigName());
        const int bild = f0() + 10;
        return k && ikSchluessel(kArmR, bild) && modellIkAnteil(kArmR) >= 1.0F && schl(knochen("rhumerus"), bild) != nullptr && schl(knochen("rradius"), bild) != nullptr &&
               schl(knochen("rhand"), bild) != nullptr && g_app->mtab.ikZugArt == 1 && g_app->mtab.dok.rueckgaengigName() == tr(Str::MbRgUndoVerschieben);
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("MB6 Max W: Strg+Z nimmt den ganzen Zug zurueck (keine Schluessel, Pose wie vorher)", [st] {
        return sch().schluesselZahl() == 0 && spur(kArmR) == nullptr && weltDiff(st->welt, g_app->modellWelt) < 1e-4F;
    }));
    // --- Glied auf IK / FK ohne Sprung (Suche), rot: ohne Einrasten ------------------------------------------------
    add(tu("MB6: Frame +16, Griff Hand R, Gizmo Auswahl", [] {
        g_app->animFrame = f0() + 16;
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        g_app->mtab.gizmo = 0;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(merke(st, "vor Glied auf IK"));
    ueberSuche(s, Str::MbIkAufIk);
    add(pruefe("MB6 Glied auf IK (Suche): IK ab Frame +16, ein Schluessel", [] { return modellIkAnteil(kArmR) >= 1.0F && ikSchluessel(kArmR, f0() + 16); }));
    add(ohneSprung(st, "Glied auf IK"));
    add(tu("MB6: Frame +24 (das Ziel haelt, die FK-Pose geht weiter)", [] {
        g_app->animFrame = f0() + 24;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(merke(st, "vor dem Wechsel auf FK"));
    add(tu("MB6 rot: Wechsel auf FK OHNE Einrasten", [] { (void)modellIkWechselnRoh(kArmR, false, false); }));
    add(pause(0.4));
    add(pruefe("MB6 rot: ohne Einrasten erkennt die Pruefung den Sprung (Weltmatrizen > 0,5 oder Bildpunkte anders)", [st] {
        const float d = weltDiff(st->welt, g_app->modellWelt);
        const float sp = modell::ikSprung(g_app->anim, st->schicht, sch(), f0() + 24);
        const int px = pixelAnders(*st);
        diag::detail("Knopftest: MB6 rot - Sprung " + std::to_string(sp) + ", Weltmatrizen " + std::to_string(d) + ", Bildpunkte " + std::to_string(px));
        return sp > 0.5F && d > 0.5F && px > 0;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(merke(st, "vor Glied auf FK"));
    for (Schritt& x : ueberRechtsklick(Str::MbIkAufFk)) { add(std::move(x)); }
    add(pruefe("MB6 Glied auf FK (Rechtsklick der Ansicht): FK ab Frame +24", [] { return modellIkAnteil(kArmR) <= 0.0F && ikSchluessel(kArmR, f0() + 24); }));
    add(ohneSprung(st, "Glied auf FK"));
    // --- 3ds Max: E am Oberarm unter IK wechselt auf FK ------------------------------------------------------------
    add(tu("MB6 Max E: Frame +20 (IK), Griff Oberarm R, E", [] {
        g_app->animFrame = f0() + 20;
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Oberarm, modell::Seite::Rechts));
        g_app->mtab.gizmo = 2;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(merke(st, "vor E am Oberarm"));
    add(mb3::ziehenVonNach("am Ring Z des Oberarms 40 Punkte nach rechts", [] {
        ImVec2 p;
        (void)modellGizmoRingOrt(2, 45.0F, p);
        return p;
    }, [] {
        ImVec2 p;
        (void)modellGizmoRingOrt(2, 45.0F, p);
        return ImVec2{p.x + 40.0F, p.y};
    }));
    add(pause(0.4));
    add(pruefe("MB6 Max E am Oberarm unter IK: das Glied wechselt an diesem Frame auf FK (Schluessel), der Oberarm dreht", [st] {
        float h0[3];
        float h1[3];
        ort(st->welt[static_cast<std::size_t>(knochen("rhand"))], h0);
        ort(g_app->modellWelt[static_cast<std::size_t>(knochen("rhand"))], h1);
        diag::detail("Knopftest: MB6 Max E - Hand bewegt " + std::to_string(abstand(h0, h1)) + ", Anteil " + std::to_string(modellIkAnteil(kArmR)));
        return g_app->mtab.ikZugArt == 4 && modellIkAnteil(kArmR) <= 0.0F && ikSchluessel(kArmR, f0() + 20) && abstand(h0, h1) > 0.5F &&
               schl(knochen("rhumerus"), f0() + 20) != nullptr;
    }));
    // --- Pol (W) ---------------------------------------------------------------------------------------------------
    add(tu("MB6 Pol: Frame +16 (IK), Griff Pol Ellbogen R, W", [] {
        g_app->animFrame = f0() + 16;
        modellRigGriffWaehlen(griffNach(modell::GriffArt::PolEllbogen, modell::Seite::Rechts));
        g_app->mtab.gizmo = 1;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(merke(st, "vor dem Pol"));
    add(mb3::ziehenVonNach("den Griff Z des Gizmos am Pol 40 Punkte nach oben", [] {
        ImVec2 p;
        (void)modellGizmoGriffOrt(3, p);
        return p;
    }, [] {
        ImVec2 p;
        (void)modellGizmoGriffOrt(3, p);
        return ImVec2{p.x, p.y - 40.0F};
    }));
    add(pause(0.4));
    add(pruefe("MB6 Pol gezogen: der Ellbogen folgt (in der neuen Pol-Ebene), die Hand bleibt am Ziel", [st] {
        std::string b;
        const bool k = kettePasst(kArmR, st->welt, b);
        float e0[3];
        float e1[3];
        ort(st->welt[static_cast<std::size_t>(knochen("rradius"))], e0);
        ort(g_app->modellWelt[static_cast<std::size_t>(knochen("rradius"))], e1);
        diag::detail("Knopftest: MB6 Pol - " + b + ", Ellbogen bewegt " + std::to_string(abstand(e0, e1)));
        return k && g_app->mtab.ikZugArt == 2 && abstand(e0, e1) > 0.05F;
    }));
    // --- Fussrolle: E an der Ferse, an der Zehenspitze ---------------------------------------------------------------
    auto pivot = std::make_shared<std::array<float, 3>>();
    auto knieBild = std::make_shared<int>(0);
    add(tu("MB6 Fussrolle: das Bild der Sequenz mit dem staerker gebeugten rechten Knie", [knieBild] {
        float best = -2.0F;
        std::vector<BoneMatrix> w;
        for (int f = f0(); f < f0() + seq().numFrames; ++f) {
            g_app->anim.worldMatrices(f, w);
            float a[3], b[3], c[3];
            ort(w[static_cast<std::size_t>(knochen("rfemurYZ"))], a);
            ort(w[static_cast<std::size_t>(knochen("rtibia"))], b);
            ort(w[static_cast<std::size_t>(knochen("rtalus"))], c);
            const float u[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
            const float v[3] = {c[0] - b[0], c[1] - b[1], c[2] - b[2]};
            const float cosw = (u[0] * v[0] + u[1] * v[1] + u[2] * v[2]) / std::max(abstand(a, b) * abstand(c, b), 1e-6F);
            if (cosw > best) {   // groesster Kosinus am Knie = staerkste Beugung
                best = cosw;
                *knieBild = f;
            }
        }
        diag::detail("Knopftest: MB6 Fussrolle - Bild " + std::to_string(*knieBild - f0()) + ", Kniewinkel " + std::to_string(std::acos(std::clamp(best, -1.0F, 1.0F)) * 57.29578F));
    }));
    for (const bool zehe : {false, true}) {
        add(tu(std::string("MB6 Fussrolle: Griff ") + (zehe ? "Zehe" : "Ferse") + " R, E", [pivot, zehe, knieBild] {
            g_app->animFrame = *knieBild;
            const int g = griffNach(zehe ? modell::GriffArt::Zehe : modell::GriffArt::Ferse, modell::Seite::Rechts);
            modellRigGriffWaehlen(g);
            g_app->mtab.gizmo = 2;
            g_app->modelDirty = true;
        }));
        add(pause(0.4));
        add(tu("MB6 Fussrolle: Drehpunkt vorher (Welt des neuen Bilds)", [pivot, zehe] {
            const int g = griffNach(zehe ? modell::GriffArt::Zehe : modell::GriffArt::Ferse, modell::Seite::Rechts);
            float p[3] = {0.0F, 0.0F, 0.0F};
            (void)modellRigGriffWelt(g, g_app->modellWelt, p);
            *pivot = {p[0], p[1], p[2]};
        }));
        add(merke(st, "vor der Fussrolle"));
        add(mb3::ziehenVonNach("am Ring X 40 Punkte nach rechts", [] {
            ImVec2 p;
            (void)modellGizmoRingOrt(0, 45.0F, p);
            return p;
        }, [] {
            ImVec2 p;
            (void)modellGizmoRingOrt(0, 45.0F, p);
            return ImVec2{p.x + 40.0F, p.y};
        }));
        add(pause(0.4));
        add(pruefe(std::string("MB6 Fussrolle um die ") + (zehe ? "Zehenspitze" : "Ferse") + ": der Punkt bleibt liegen, der Fuss dreht, Schluessel am Frame", [st, pivot, zehe] {
            const int g = griffNach(zehe ? modell::GriffArt::Zehe : modell::GriffArt::Ferse, modell::Seite::Rechts);
            float p[3];
            (void)modellRigGriffWelt(g, g_app->modellWelt, p);
            float k0[3];
            float k1[3];
            ort(st->welt[static_cast<std::size_t>(knochen("rtalus"))], k0);
            ort(g_app->modellWelt[static_cast<std::size_t>(knochen("rtalus"))], k1);
            const modell::IkSpur* sp = spur(kBeinR);
            const modell::IkZiel z = sp != nullptr ? modell::ikZielAm(*sp, static_cast<float>(g_app->animFrame), g_app->modellWelt) : modell::IkZiel{};
            diag::detail("Knopftest: MB6 Fussrolle - Drehpunkt " + std::to_string(abstand(p, pivot->data())) + ", Knoechel " + std::to_string(abstand(k0, k1)) + ", Rolle " +
                         std::to_string(z.rolle) + ", Spitze " + std::to_string(z.spitze));
            return g_app->mtab.ikZugArt == 3 && abstand(p, pivot->data()) < 0.05F && abstand(k0, k1) > 0.3F && z.da && (zehe ? z.rolle > 5.0F && z.spitze > 0.99F : z.rolle < -5.0F);
        }));
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
        add(pause(0.3));
    }
    if (handbuch) {
        add(tu("MB6 Handbuch: IK am rechten Arm (Ziel bewegt), Pfad der Hand, Ansicht eingepasst", [] {
            g_app->animFrame = f0() + 16;
            modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
            (void)modellIkWechseln(kArmR, true);
            g_app->mtab.dok.aendern(tr(Str::MbRgUndoVerschieben), [](modell::Inhalt& in) {
                modell::RigSchicht& sch2 = in.rig.schichten[seq().name];
                const int sp = modell::ikSpurIndex(sch2, modell::IkArt::ArmRechts);
                if (sp < 0) { return; }
                std::vector<BoneMatrix> w;
                modell::rigWelt(g_app->anim, sch2, f0() + 16, f0() + 16, 0.0F, w);
                BoneMatrix z = modell::starr(w[static_cast<std::size_t>(sch2.ik[static_cast<std::size_t>(sp)].ende.knochen)]);
                z.m[1][3] -= 6.0F;
                z.m[2][3] += 6.0F;
                modell::ikZielSetzen(g_app->anim, sch2, sp, f0() + 16, z);
            }, false);
            g_app->mtab.knochenWahl = knochen("rhand");
            (void)modellPfadHinzu();
            modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
            g_app->mtab.gizmo = 1;
            modellAnsichtEinpassenAktiv();
            g_app->modelDirty = true;
        }));
        add(pause(0.6));
        add(rig3::rollenZu(tr(Str::MbIkRollout), "mtrollouts"));
        add(tu("MB6 Handbuch: Maus in die Ansicht (kein Tooltip im Bild)", [] { setzeMaus(kv5::fokusPunkt()); }));
        add(pause(0.4));
        add(hilfe2::schrittFoto("h_mb6_ik"));
        add(tu("MB6 Handbuch: Fussrolle an der Ferse, Ansicht eingepasst", [] {
            g_app->mtab.pfade.clear();
            g_app->animFrame = f0() + 10;
            modellRigGriffWaehlen(griffNach(modell::GriffArt::Ferse, modell::Seite::Rechts));
            g_app->mtab.gizmo = 2;
            (void)modellIkRolle(kBeinR, -22.0F, 0.0F);
            modellRigGriffWaehlen(griffNach(modell::GriffArt::Ferse, modell::Seite::Rechts));
            modellAnsichtEinpassenAktiv();
            g_app->modelDirty = true;
        }));
        add(pause(0.6));
        add(rig3::rollenZu(tr(Str::MbIkRollout), "mtrollouts"));
        add(tu("MB6 Handbuch: Maus in die Ansicht", [] { setzeMaus(kv5::fokusPunkt()); }));
        add(pause(0.4));
        add(hilfe2::schrittFoto("h_mb6_fussrolle"));
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    }
    // --- Raum: Saebel an die andere Hand -----------------------------------------------------------------------------
    add(tu("MB6 Raum: Schicht leer, Frame +4, Hand R auf IK", [] {
        g_app->mtab.dok.aendern("MB6 leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
        g_app->animFrame = f0() + 4;
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        g_app->mtab.gizmo = 0;
        (void)modellIkWechseln(kArmR, true);
        g_app->animFrame = f0() + 12;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(merke(st, "vor dem Raumwechsel"));
    ueberSuche(s, Str::MbIkSaebelHand);
    auto rel = std::make_shared<BoneMatrix>();
    add(pruefe("MB6 Saebel an die andere Hand (Suche): Raum linke Hand ab Frame +12, Schluessel an +11 und +12", [rel] {
        const modell::IkSpur* sp = spur(kArmR);
        const modell::IkZiel z = sp != nullptr ? modell::ikZielAm(*sp, static_cast<float>(f0() + 12), g_app->modellWelt) : modell::IkZiel{};
        const auto& w = g_app->modellWelt;
        *rel = modell::starrInvers(modell::starr(w[static_cast<std::size_t>(knochen("lhand"))])) * z.ziel;
        return z.da && z.raum == knochen("lhand") && ikSchluessel(kArmR, f0() + 11) && ikSchluessel(kArmR, f0() + 12);
    }));
    add(ohneSprung(st, "Raumwechsel"));
    add(tu("MB6 Raum: Frame +22", [] {
        g_app->animFrame = f0() + 22;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(pruefe("MB6 Raum: an Frame +22 folgt das Ziel der rechten Hand der linken starr, die Hand steht am Ziel (oder der Arm ist gestreckt)", [rel] {
        const auto& w = g_app->modellWelt;
        const modell::IkSpur* sp = spur(kArmR);
        const modell::IkZiel z = sp != nullptr ? modell::ikZielAm(*sp, static_cast<float>(f0() + 22), w) : modell::IkZiel{};
        const BoneMatrix r = modell::starrInvers(modell::starr(w[static_cast<std::size_t>(knochen("lhand"))])) * z.ziel;
        float d = 0.0F;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 4; ++j) { d = std::max(d, std::fabs(r.m[i][j] - rel->m[i][j])); }
        }
        float a[3], b[3], c[3], t[3];
        ort(w[static_cast<std::size_t>(sp->oben.knochen)], a);
        ort(w[static_cast<std::size_t>(sp->unten.knochen)], b);
        ort(w[static_cast<std::size_t>(sp->ende.knochen)], c);
        ort(z.ziel, t);
        const float gestreckt = abstand(a, b) + abstand(b, c) - abstand(a, c);
        diag::detail("Knopftest: MB6 Raum - Ziel zur linken Hand " + std::to_string(d) + ", Hand vom Ziel " + std::to_string(abstand(c, t)) + ", Streckung " + std::to_string(gestreckt));
        return d < 1e-3F && (abstand(c, t) < 0.01F || gestreckt < 0.01F);
    }));
    if (handbuch) {
        add(tu("MB6 Handbuch: Raum, Saebelgriff gewaehlt, Ansicht eingepasst", [] {
            modellRigGriffWaehlen(griffNach(modell::GriffArt::Saebelgriff, modell::Seite::Rechts));
            modellAnsichtEinpassenAktiv();
            g_app->modelDirty = true;
        }));
        add(pause(0.6));
        add(rig3::rollenZu(tr(Str::MbIkRollout), "mtrollouts"));
        add(tu("MB6 Handbuch: Maus in die Ansicht", [] { setzeMaus(kv5::fokusPunkt()); }));
        add(pause(0.4));
        add(hilfe2::schrittFoto("h_mb6_raum"));
    }
    // --- Pfade an Hand, Fuss und Saebelspitze ziehen ----------------------------------------------------------------
    add(tu("MB6 Pfade: Schicht leer, Saebel an *r_hand, Pfade rhand, rtalus, Saebelspitze, Frame +8, Ansicht Vorne", [] {
        g_app->mtab.dok.aendern("MB6 leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
        (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true);
        g_app->mtab.pfade.clear();
        for (const char* n : {"rhand", "rtalus"}) {
            modellRigGriffWaehlen(-1);
            g_app->mtab.knochenWahl = knochen(n);
            (void)modellPfadHinzu();
        }
        (void)modellPfadSpitzeHinzu();
        g_app->animFrame = f0() + 8;
        g_app->mtab.gizmo = 0;
        modellRigGriffWaehlen(-1);
        modellAnsichtEinpassenAktiv();
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(pruefe("MB6 Pfade: drei Pfade (Hand, Fuss, Saebelspitze), die Spitze liegt eine Klinge weit vor der Hand", [] {
        const ModellTab& t = g_app->mtab;
        if (t.pfade.size() != 3U || !t.pfade[2].spitze) { return false; }
        float sp[3];
        float h[3];
        if (!modellSaebelSpitze(t.pfade[2], g_app->modellWelt, sp)) { return false; }
        ort(g_app->modellWelt[static_cast<std::size_t>(knochen("rhand"))], h);
        diag::detail("Knopftest: MB6 Pfade - Spitze " + std::to_string(abstand(sp, h)) + " von der Hand");
        return abstand(sp, h) > 20.0F && abstand(sp, h) < 50.0F;
    }));
    static const char* const kPfadName[3] = {"Hand", "Fuss", "Saebelspitze"};
    for (int p = 0; p < 3; ++p) {
        auto ziel = std::make_shared<ImVec2>();
        add(mb3::ziehenVonNach(std::string("den Punkt des Pfads ") + kPfadName[p] + " bei +8 um (30, -25)", [p, ziel] {
            ImVec2 q;
            (void)modellPfadPunktAufSchirm(p, f0() + 8, q);
            *ziel = ImVec2{q.x + 30.0F, q.y - 25.0F};
            return q;
        }, [ziel] { return *ziel; }));
        add(pause(0.4));
        add(pruefe(std::string("MB6 Pfad ") + kPfadName[p] + " gezogen: der Punkt steht bei +8 unter der Maus (2 Punkte), IK-Schluessel des Glieds, ein Rueckgaengig-Schritt", [p, ziel] {
            ImVec2 q;
            const bool da = modellPfadPunktAufSchirm(p, f0() + 8, q);
            const int art = p == 1 ? kBeinR : kArmR;
            diag::detail("Knopftest: MB6 Pfad " + std::string(kPfadName[p]) + " - Punkt (" + std::to_string(q.x) + ", " + std::to_string(q.y) + "), Maus (" + std::to_string(ziel->x) +
                         ", " + std::to_string(ziel->y) + "), " + g_app->mtab.rigMeldung);
            return da && std::hypot(q.x - ziel->x, q.y - ziel->y) < 2.0F && ikSchluessel(art, f0() + 8) && modellIkAnteil(art) >= 1.0F;
        }));
        if (handbuch && p == 2) { add(hilfe2::schrittFoto("h_mb6_pfade")); }
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
        add(pause(0.3));
        add(pruefe(std::string("MB6 Pfad ") + kPfadName[p] + ": Strg+Z nimmt den Zug zurueck", [] { return sch().schluesselZahl() == 0 && sch().ik.empty(); }));
    }
    add(tu("MB6: Pfade leer, Saebel ab", [] {
        g_app->mtab.pfade.clear();
        modellAnbautenLoesen();
    }));
    // --- wie Blender: Regler IK-FK, IK -> FK, FK -> IK, G ------------------------------------------------------------
    add(tu("MB6 Blender: Bedienung wie Blender, Schicht leer, Frame +10, Griff Hand R", [] {
        modellBedienungSetzen(1);
        g_app->mtab.dok.aendern("MB6 leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
        g_app->animFrame = f0() + 10;
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        setzeMaus(ImVec2{a.lo[0] + a.groesse[0] * 0.5F, a.lo[1] + a.groesse[1] * 0.5F});
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(taste(ImGuiKey_G, "G"));
    add(pause(0.2));
    add(pruefe("MB6 Blender G an der Hand in FK: klare Meldung, nichts geaendert", [] {
        char erwartet[400];
        std::snprintf(erwartet, sizeof erwartet, tr(Str::MbIkNurIk), tr(Str::MbIkArmR));
        return g_app->mtab.rigMeldung == erwartet && sch().ik.empty() && g_app->mtab.rigModal == 0;
    }));
    sucheIk(s, Str::MbIkZielAufFk);
    add(pruefe("MB6 Blender IK -> FK: Ziel und Pol auf der FK-Pose, Regler bleibt FK", [] { return ikSchluessel(kArmR, f0() + 10) && modellIkAnteil(kArmR) <= 0.0F; }));
    add(merke(st, "vor dem Regler"));
    add(tu("MB6 Blender: Regler IK-FK auf 0 (IK)", [] { (void)modellIkReglerSetzen(kArmR, 0.0F); }));
    add(pause(0.4));
    add(pruefe("MB6 Blender Regler 0: IK", [] { return modellIkAnteil(kArmR) >= 1.0F; }));
    add(ohneSprung(st, "Regler IK-FK nach IK -> FK"));
    // G bewegt jetzt das Ziel
    add(tu("MB6 Blender G: Maus in die Mitte", [st] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        setzeMaus(ImVec2{a.lo[0] + a.groesse[0] * 0.5F, a.lo[1] + a.groesse[1] * 0.5F});
        st->welt = g_app->modellWelt;
    }));
    add(pause(0.1));
    add(taste(ImGuiKey_G, "G"));
    add(tu("MB6 Blender G: Maus 30 Punkte nach rechts", [] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        setzeMaus(ImVec2{a.lo[0] + a.groesse[0] * 0.5F + 30.0F, a.lo[1] + a.groesse[1] * 0.5F});
    }));
    add(pause(0.2));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.3));
    add(pruefe("MB6 Blender G an der Hand in IK: das Ziel bewegt sich, die Kette folgt (Hand am Ziel, Pol-Ebene, Laengen)", [st] {
        std::string b;
        const bool k = kettePasst(kArmR, st->welt, b);
        float h0[3];
        float h1[3];
        ort(st->welt[static_cast<std::size_t>(knochen("rhand"))], h0);
        ort(g_app->modellWelt[static_cast<std::size_t>(knochen("rhand"))], h1);
        diag::detail("Knopftest: MB6 Blender G - " + b);
        return k && abstand(h0, h1) > 0.5F && g_app->mtab.rigModal == 0;
    }));
    // rot: Regler auf FK ohne Einrasten an einem Frame zwischen den Schluesseln (das Ziel haelt, die FK-Pose laeuft)
    add(tu("MB6 Blender: Frame +26", [] {
        g_app->animFrame = f0() + 26;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(merke(st, "vor dem Regler auf FK"));
    add(tu("MB6 Blender rot: Regler IK-FK auf 1 (FK) ohne FK -> IK", [] { (void)modellIkReglerSetzen(kArmR, 1.0F); }));
    add(pause(0.4));
    add(pruefe("MB6 Blender rot: ohne Einrasten erkennt die Pruefung den Sprung", [st] {
        const float d = weltDiff(st->welt, g_app->modellWelt);
        diag::detail("Knopftest: MB6 Blender rot - Weltmatrizen " + std::to_string(d) + ", Bildpunkte " + std::to_string(pixelAnders(*st)));
        return d > 0.5F && pixelAnders(*st) > 0;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    sucheIk(s, Str::MbIkFkAufIk);
    add(tu("MB6 Blender: Regler IK-FK auf 1 (FK) nach FK -> IK", [] { (void)modellIkReglerSetzen(kArmR, 1.0F); }));
    add(pause(0.4));
    add(ohneSprung(st, "Regler IK-FK nach FK -> IK"));
    add(tu("MB6: wie 3ds Max", [] { modellBedienungSetzen(0); }));
    // --- Reduzieren nur gewaehlte Knochen in einer Spanne ------------------------------------------------------------
    auto vorRed = std::make_shared<modell::RigSchicht>();
    add(tu("MB6 Reduzieren: BOTH_WALK1 ganz reduziert, die Hand R mit je Frame einem Schluessel (dieselbe Kurve)", [vorRed] {
        const int erstes = f0();
        const int anzahl = seq().numFrames;
        modell::RigSchicht r;
        (void)modell::rigReduzieren(g_app->anim, modell::RigSchicht{}, erstes, anzahl, r, nullptr);
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        // die Knochen des Griffs dicht: ein Schluessel je Frame mit dem Wert der reduzierten Kurve (das Reduzieren muss sie
        // in der Spanne wieder ausduennen)
        const modell::Griff& gr = g_app->mtab.rig.griffe[static_cast<std::size_t>(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts))];
        std::vector<int> kn{gr.knochen};
        kn.insert(kn.end(), gr.mit.begin(), gr.mit.end());
        modell::RigSchicht dicht = r;
        for (const int b : kn) {
            for (int f = erstes; f < erstes + anzahl; ++f) {
                BoneMatrix m;
                if (modell::rigWert(r, b, static_cast<float>(f), m)) { modell::rigSchluesselSetzen(dicht, b, f, m); }
            }
        }
        g_app->mtab.dok.aendern("MB6 reduziert", [&](modell::Inhalt& in) { in.rig.schichten[seq().name] = dicht; }, false);
        g_app->animFrame = f0() + 10;
        *vorRed = sch();
        g_app->mtab.kvWahl.clear();
        g_app->mtab.kvRedNurGewaehlte = true;
        g_app->mtab.kvRedSpanne = true;
        g_app->mtab.kvRedVon = f0() + 5;
        g_app->mtab.kvRedBis = f0() + 15;
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    ueberSuche(s, Str::MbKvReduzieren);
    add(pruefe("MB6 Reduzieren nur gewaehlte Knochen (Hand R) in +5..+15: weniger Schluessel dort, alle anderen Schluessel Bit fuer Bit, gebacken innerhalb der Grenze", [vorRed] {
        const modell::RigSchicht& n = sch();
        const modell::ReduzierBericht& b = g_app->mtab.kvBericht;
        const int hand = knochen("rhand");
        // gewaehlt sind die Knochen des Griffs Hand R: Hand, mitgedrehte Finger, Drehknochen
        const modell::Griff& gr = g_app->mtab.rig.griffe[static_cast<std::size_t>(griffNach(modell::GriffArt::Hand, modell::Seite::Rechts))];
        std::vector<int> gewaehlt{gr.knochen};
        gewaehlt.insert(gewaehlt.end(), gr.mit.begin(), gr.mit.end());
        if (gr.twist >= 0) { gewaehlt.push_back(gr.twist); }
        const auto istGewaehlt = [&](int k) { return std::find(gewaehlt.begin(), gewaehlt.end(), k) != gewaehlt.end(); };
        int vorher = 0;
        int nachher = 0;
        int andereAnders = 0;
        for (std::size_t k = 0; k < vorRed->knochen.size(); ++k) {
            const auto& a = vorRed->knochen[k];
            const auto& c = k < n.knochen.size() ? n.knochen[k] : std::vector<modell::RigSchluessel>{};
            for (const modell::RigSchluessel& x : a) {
                const bool drin = x.bild >= f0() + 5 && x.bild <= f0() + 15;
                if (static_cast<int>(k) == hand && drin) { ++vorher; }
                if (!istGewaehlt(static_cast<int>(k))) {
                    const bool gleich = std::any_of(c.begin(), c.end(), [&](const modell::RigSchluessel& y) {
                        return y.bild == x.bild && std::memcmp(x.q, y.q, sizeof x.q) == 0 && std::memcmp(x.t, y.t, sizeof x.t) == 0;
                    });
                    andereAnders += gleich ? 0 : 1;
                }
            }
            if (static_cast<int>(k) == hand) {
                for (const modell::RigSchluessel& y : c) { nachher += (y.bild >= f0() + 5 && y.bild <= f0() + 15) ? 1 : 0; }
            }
        }
        diag::detail("Knopftest: MB6 Reduzieren - rhand in der Spanne " + std::to_string(vorher) + " -> " + std::to_string(nachher) + ", andere anders " +
                     std::to_string(andereAnders) + ", " + g_app->mtab.rigMeldung);
        return b.gewaehlt == static_cast<int>(gewaehlt.size()) && nachher < vorher && andereAnders == 0 && b.ueberGrenze == 0 && b.maxAnteil <= 1.0;
    }));
    add(tu("MB6 Reduzieren: Auswahl aus", [] {
        g_app->mtab.kvRedNurGewaehlte = false;
        g_app->mtab.kvRedSpanne = false;
    }));
    // --- Posen-Bibliothek als Datei ----------------------------------------------------------------------------------
    auto posen = std::make_shared<std::vector<modell::BibliothekPose>>();
    add(tu("MB6 Posen: zwei Posen in die Bibliothek, Dateiname mb6", [posen] {
        g_app->mtab.dok.aendern("MB6 Posen leer", [](modell::Inhalt& in) { in.rig.posen.clear(); }, false);
        for (const int b : {f0() + 2, f0() + 9}) {
            g_app->animFrame = b;
            std::vector<BoneMatrix> w;
            g_app->anim.worldMatrices(b, w);
            const modell::BibliothekPose p = modell::bibliothekPose("mb6 " + std::to_string(b), modell::poseAus(g_app->anim, w));
            g_app->mtab.dok.aendern("MB6 Pose", [&](modell::Inhalt& in) { in.rig.posen.push_back(p); }, false);
        }
        *posen = g_app->mtab.dok.inhalt().rig.posen;
        std::snprintf(g_app->mtab.posenDatei, sizeof g_app->mtab.posenDatei, "%s", "mb6");
    }));
    ueberSuche(s, Str::MbIkPosenSpeichern);
    add(pruefe("MB6 Posen: als Datei gespeichert (Posen-Ordner, nicht im Spielordner)", [] {
        std::error_code ec;
        const std::filesystem::path p = utf8Pfad(modellPosenOrdner()) / "mb6.jkposen";
        diag::detail("Knopftest: MB6 Posen - " + g_app->mtab.rigMeldung);
        return std::filesystem::is_regular_file(p, ec) && !modellImSpielordner(modellPosenOrdner());
    }));
    add(tu("MB6 Posen: Bibliothek leeren", [] { g_app->mtab.dok.aendern("MB6 Posen leer", [](modell::Inhalt& in) { in.rig.posen.clear(); }, false); }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(klickMenue(tr(Str::MbBereichAnimation), true));
    add(pause(0.5));
    add(klickMenue(tr(Str::MbKvMenue), false));
    add(pause(0.5));
    add(klickMenue(tr(Str::MbKvGruppeBibliothek), false));
    add(pause(0.5));
    add(klickMenue(tr(Str::MbIkPosenLaden), false));
    add(pause(0.5));
    add(klickMenue("mb6", false));
    add(pause(0.3));
    add(pruefe("MB6 Posen: ueber das Menue geladen - dieselben Posen Bit fuer Bit, ein Rueckgaengig-Schritt", [posen] {
        const auto& l = g_app->mtab.dok.inhalt().rig.posen;
        bool gleich = l.size() == posen->size();
        for (std::size_t i = 0; gleich && i < l.size(); ++i) {
            gleich = l[i].name == (*posen)[i].name && l[i].namen == (*posen)[i].namen && l[i].welt.size() == (*posen)[i].welt.size();
            for (std::size_t k = 0; gleich && k < l[i].welt.size(); ++k) { gleich = std::memcmp(l[i].welt[k].data(), (*posen)[i].welt[k].data(), sizeof(float) * 12U) == 0; }
        }
        return gleich && g_app->mtab.dok.rueckgaengigName() == tr(Str::MbIkUndoPosen);
    }));
    for (int k = 0; k < 3; ++k) { add(taste(ImGuiKey_Escape, "Esc")); }
    return s;
}

// --- Rundlauf IK <-> FK an jeder Sequenz von _humanoid (BHED_MODELL_NUR=ikrundlauf, BHED_MB_SPIEL=jka|jk2|md) -----------
// Je Sequenz und Glied: IK -> FK an jedem Frame (Ziel und Pol aus der FK-Pose), gebacken gegen die GLA; dann die Ziele an drei
// Frames bewegt, FK -> IK an jedem Frame, ohne IK gebacken gegen mit IK. Jedes Knochen-Frame innerhalb der Grenze aus
// compressBone (Stufe 3: Drehung, Verschiebung hoechstens eine Stufe). Bericht mb6_ikrundlauf_<spiel>.txt.
struct RundVergleich {
    long long alle = 0;
    long long gleich = 0;
    int ueber = 0;
    double anteil = 0.0;
    std::string schlimmster;
};

void vergleichen(const GlaAnimation& g, const modell::RigSchicht& a, const modell::RigSchicht& b, const AnimEntry& e, RundVergleich& v) {
    GlaAnimation ga;
    GlaAnimation gb;
    modell::BackBericht x;
    if (!modell::rigBacken(g, a, e.firstFrame, e.numFrames, false, "a", ga, &x) || !modell::rigBacken(g, b, e.firstFrame, e.numFrames, false, "b", gb, &x)) {
        ++v.ueber;
        return;
    }
    const std::size_t nb = g.bones.size();
    for (std::size_t at = 0; at < ga.indexes.size(); ++at) {
        const std::uint8_t* p = ga.bonePool.data() + static_cast<std::size_t>(ga.indexes[at]) * 14U;
        const std::uint8_t* q = gb.bonePool.data() + static_cast<std::size_t>(gb.indexes[at]) * 14U;
        const double an = modell::drehAbstand(p, q) / modell::kompressionsGrenze(p);
        if (an > v.anteil) {
            v.anteil = an;
            v.schlimmster = e.name + " " + g.bones[at % nb].name + " Frame " + std::to_string(e.firstFrame + static_cast<int>(at / nb));
        }
        v.ueber += (an > 1.0 || modell::verschiebungsStufen(p, q) > 1) ? 1 : 0;
        v.gleich += std::memcmp(p, q, 14U) == 0 ? 1 : 0;
        ++v.alle;
    }
}

Schritt rundlauf(const std::string& spiel) {
    struct Zustand {
        GlaAnimation g;
        modell::Rig rig;
        std::vector<AnimEntry> seqs;
        std::size_t i = 0;
        RundVergleich ikFk;
        RundVergleich fkIk;
        int glieder = 0;
        int sequenzen = 0;
        std::string bericht;
        std::chrono::steady_clock::time_point anfang{};
    };
    auto z = std::make_shared<Zustand>();
    Schritt s{"MB6 IK-Rundlauf (" + spiel + "): jede Sequenz von _humanoid", [=](int b) {
                  if (b == 0) {
                      std::string bytes;
                      std::string cfg;
                      if (!readFromArchives("models/players/_humanoid/_humanoid.gla", bytes) || !readGla(bytes, z->g) ||
                          !readFromArchives("models/players/_humanoid/animation.cfg", cfg)) {
                          meldeFehler("MB6 IK-Rundlauf " + spiel + ": _humanoid.gla oder animation.cfg fehlt");
                          return true;
                      }
                      std::set<std::pair<int, int>> schon;
                      for (const AnimEntry& e : parseAnimationCfg(cfg)) {
                          if (e.numFrames <= 0 || e.firstFrame < 0 || e.firstFrame + e.numFrames > z->g.numFrames) { continue; }
                          if (!schon.insert({e.firstFrame, e.numFrames}).second) { continue; }
                          z->seqs.push_back(e);
                      }
                      z->rig = modell::rigAnlegen(z->g, modell::Inhalt{}, 0, {});
                      z->anfang = std::chrono::steady_clock::now();
                      diag::detail("Knopftest: MB6 IK-Rundlauf " + spiel + " - " + std::to_string(z->seqs.size()) + " Sequenzen, " + std::to_string(z->g.numFrames) + " Frames, " +
                                   std::to_string(z->g.bones.size()) + " Knochen");
                      return false;
                  }
                  if (z->i < z->seqs.size()) {
                      const AnimEntry& e = z->seqs[z->i++];
                      const int von = e.firstFrame;
                      const int bis = e.firstFrame + e.numFrames - 1;
                      modell::RigSchicht leer;
                      modell::RigSchicht ik;
                      for (int a = 0; a < modell::kIkArten; ++a) {
                          modell::IkSpur sp;
                          if (!modell::ikSpurAnlegen(z->g, z->rig, static_cast<modell::IkArt>(a), sp)) { continue; }
                          ik.ik.push_back(sp);
                          modell::ikAufFkBereich(z->g, ik, static_cast<int>(ik.ik.size()) - 1, von, bis, 1.0F);
                      }
                      if (z->sequenzen == 0) { z->glieder = static_cast<int>(ik.ik.size()); }
                      vergleichen(z->g, leer, ik, e, z->ikFk);
                      // die Ziele an Anfang, Mitte und Ende bewegt (eine halbe Einheit hoch, zur Wurzel hin), dann FK -> IK
                      modell::RigSchicht bewegt = ik;
                      for (std::size_t sp = 0; sp < bewegt.ik.size(); ++sp) {
                          for (const int f : {von, von + (bis - von) / 2, bis}) {
                              std::vector<BoneMatrix> w;
                              modell::rigWelt(z->g, bewegt, f, f, 0.0F, w);
                              BoneMatrix ziel = modell::starr(w[static_cast<std::size_t>(bewegt.ik[sp].ende.knochen)]);
                              const BoneMatrix& o = w[static_cast<std::size_t>(bewegt.ik[sp].oben.knochen)];
                              for (int k = 0; k < 3; ++k) { ziel.m[k][3] += (o.m[k][3] - ziel.m[k][3]) * 0.05F; }
                              ziel.m[2][3] += 0.5F;
                              modell::ikZielSetzen(z->g, bewegt, static_cast<int>(sp), f, ziel);
                          }
                      }
                      modell::RigSchicht fk = bewegt;
                      for (std::size_t sp = 0; sp < fk.ik.size(); ++sp) { (void)modell::fkAufIkBereich(z->g, fk, static_cast<int>(sp), von, bis); }
                      fk.ik.clear();
                      vergleichen(z->g, bewegt, fk, e, z->fkIk);
                      ++z->sequenzen;
                      return false;
                  }
                  const double sek = std::chrono::duration<double>(std::chrono::steady_clock::now() - z->anfang).count();
                  char zeile[900];
                  std::snprintf(zeile, sizeof zeile,
                                "MB6 IK-Rundlauf %s: %d Sequenzen, %d Glieder; IK -> FK an jedem Frame gegen die GLA: %lld Knochen-Frames, %lld Byte fuer Byte, groesster "
                                "Anteil %.4f (%s), ueber %d; Ziele bewegt, FK -> IK an jedem Frame, ohne IK gegen mit IK: %lld Knochen-Frames, %lld Byte fuer Byte, groesster "
                                "Anteil %.4f (%s), ueber %d; %.0f s",
                                spiel.c_str(), z->sequenzen, z->glieder, z->ikFk.alle, z->ikFk.gleich, z->ikFk.anteil, z->ikFk.schlimmster.c_str(), z->ikFk.ueber,
                                z->fkIk.alle, z->fkIk.gleich, z->fkIk.anteil, z->fkIk.schlimmster.c_str(), z->fkIk.ueber, sek);
                  diag::detail(std::string("Knopftest: ") + zeile);
                  {
                      std::ofstream f("mb6_ikrundlauf_" + spiel + ".txt", std::ios::binary);
                      f << zeile << "\n";
                  }
                  pruefeWahr(z->sequenzen > 0 && z->glieder == 4 && z->ikFk.ueber == 0,
                             "MB6 IK-Rundlauf " + spiel + ": IK -> FK an jedem Frame jeder Sequenz zeigt die GLA (gebacken innerhalb der Grenze)");
                  // FK -> IK: die Schluessel sind Drehungen (Stufe 3); dreht die IK einen Knochen, dessen Weltmatrix nicht orthonormal ist
                  // (Raven-Daten von JKA/JK2: MC_UnCompressQuat normiert nicht, das summiert sich die Kette hinab), ist M = X^-1 U X keine
                  // reine Drehung und der Schluessel haelt nur die naechste Drehung - gemessen (mb6_irl3): JKA 2207 von 1189744
                  // Knochen-Frames (Fingerglieder, groesster Anteil 2,6), JK2 189 von 1314288 (Drehknochen des Unterarms, 1,8), Movie
                  // Duels 0 von 3281283. Darum streng nur fuer Movie Duels, fuer JKA/JK2 die Zahlen im Bericht.
                  if (spiel == "md") {
                      pruefeWahr(z->sequenzen > 0 && z->fkIk.ueber == 0,
                                 "MB6 IK-Rundlauf " + spiel + ": FK -> IK an jedem Frame, ohne IK dieselbe Animation (gebacken innerhalb der Grenze)");
                  }
                  return true;
              }};
    s.eigeneZeitgrenze = true;
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    if (n == "ikrundlauf") {
        const char* sp = std::getenv("BHED_MB_SPIEL");
        const std::string spiel = sp != nullptr ? sp : "md";
        std::vector<Schritt> s;
        s.push_back(tu("MB6 IK-Rundlauf: Spiel " + spiel, [=] {
            if (spiel != "md") { mb1::spielSetzen(spiel); }
            if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        }));
        s.push_back(pause(0.3));
        s.push_back(rundlauf(spiel));
        return s;
    }
    if (n == "mb6") { return uiSchritte(false); }
    return {};
}

std::vector<Schritt> handbuchSchritte() { return uiSchritte(true); }

}  // namespace ik6
