// selbsttest_ph.inc - Modell-Block Stufe 6: Auto-Physik (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ph | phspiel | phzeit)
//
//   Erreichbarkeit  jede Aktion der Gruppe Auto-Physik im Menue Animation (Untermenue) mit Tooltip, im Rechtsklick der Ansicht
//                   (Rig > Auto-Physik), in der Suche (X wie 3ds Max, F3 wie Blender); das Rollout geht auf; Fenster "Figur
//                   animieren": Menue Auto-Physik, Rollout
//   Sprung          Becken als Dreieck hoch und vorn, ein Arm schwingt: rot ohne Physik (Schwerpunkt weicht von der Parabel ab,
//                   Drehimpuls schwankt), gruen mit Physik (Parabel mit g 800, Drehimpuls konstant), Schluessel unangetastet,
//                   Staerke 0,5 dazwischen, ein Rueckgaengig-Schritt, aus -> gebacken Byte fuer Byte wie ohne
//   Kontakte        Becken wandert bei gebeugten Knien: rot die Fuesse rutschen mit, gruen sie stehen
//   Stuetzflaeche   Oberkoerper 90 Grad vorgebeugt: Warnung ab dem Bild, vorher keine; die Ansicht zeichnet Schwerpunkt, Weg, Flaeche
//   Federkette      Ketten vorschlagen (ltail, rtail), Ruck des Beckens: Nachlauf, danach ruhig
//   wie Blender     F3 > Auto-Physik einstellen, Menue Animation > Auto-Physik
//   Mission-Tab     mb7_a springt (Wurzelweg): rot die gebackene Karten-GLA weicht von der Parabel ab, gruen nicht; Zeile
//                   "physik" im Skript, ein Rueckgaengig-Schritt
//   phspiel         derselbe Sprung gebaut nach BHED_FIGUR_BAU, Kamera von der Seite, Vorschau je 50 ms (Fotos und Werte) fuer
//                   den Vergleich im Spiel (ph/spiel.py, ph/vergleich.py)
//   phzeit          Bildzeit allein (BHED_D3D_DEBUG=0): Physik an, drei Figuren, Kurveneditor offen
// Eingebunden in selbsttest.cpp (nach selbsttest_mb7.inc).
namespace ph6 {

using kv5::f0;
using kv5::sch;
using kv5::seq;
using rig3::imMenue;
using rig3::klickMenue;
using rig3::woIst;

int knochen(const char* n) { return boneIndex(g_app->anim, n); }

std::string zahl(double v) {
    char b[40];
    std::snprintf(b, sizeof b, "%.4f", v);
    return b;
}

struct Zustand {
    int erstes = 0;
    int anzahl = 0;
    int a = -1;                       // Flug (Absprung, Landung)
    int l = -1;
    float parabelOhne = 0.0F;
    float drehOhne = 0.0F;
    float rutschenOhne = 0.0F;
    std::size_t undo = 0;
    modell::RigSchicht vorher;        // die Schicht vor dem Einschalten (unangetastet?)
    modell::MassenModell mm;
    float bauParabel = 0.0F;
    // Mission
    float missionOhne = 0.0F;
    std::size_t skriptTiefe = 0;
};

double bildSek() { return animBildMs(seq()) / 1000.0; }

modell::Vec3 com(const modell::MassenModell& mm, const modell::RigSchicht& s, int f) {
    std::vector<BoneMatrix> w;
    modell::rigWelt(g_app->anim, s, f, f, 0.0F, w);
    return modell::schwerpunkt(g_app->anim, mm, w);
}

// Groesste Abweichung des Schwerpunkts von der Parabel durch c(a), c(l) mit g
float parabel(const modell::MassenModell& mm, const modell::RigSchicht& s, int a, int l, double g) {
    if (a < 0 || l <= a) { return -1.0F; }
    const double dt = bildSek();
    const modell::Vec3 ca = com(mm, s, a);
    const modell::Vec3 cl = com(mm, s, l);
    const double tt = (l - a) * dt;
    float abw = 0.0F;
    for (int f = a; f <= l; ++f) {
        const double t = (f - a) * dt;
        const modell::Vec3 c = com(mm, s, f);
        double d2 = 0.0;
        for (int k = 0; k < 3; ++k) {
            const double gk = k == 2 ? -g : 0.0;
            const double v0 = (cl[static_cast<std::size_t>(k)] - ca[static_cast<std::size_t>(k)] - 0.5 * gk * tt * tt) / tt;
            const double p = ca[static_cast<std::size_t>(k)] + v0 * t + 0.5 * gk * t * t;
            d2 += (c[static_cast<std::size_t>(k)] - p) * (c[static_cast<std::size_t>(k)] - p);
        }
        abw = std::max(abw, static_cast<float>(std::sqrt(d2)));
    }
    return abw;
}

float drehSchwankung(const modell::MassenModell& mm, const modell::RigSchicht& s, int a, int l) {
    std::vector<modell::Vec3> ls;
    std::vector<BoneMatrix> w0;
    std::vector<BoneMatrix> w1;
    for (int f = a; f < l; ++f) {
        modell::rigWelt(g_app->anim, s, f, f, 0.0F, w0);
        modell::rigWelt(g_app->anim, s, f + 1, f + 1, 0.0F, w1);
        ls.push_back(modell::drehimpuls(g_app->anim, mm, w0, w1, bildSek()));
    }
    modell::Vec3 m{};
    float gross = 1e-9F;
    for (const modell::Vec3& x : ls) {
        for (int k = 0; k < 3; ++k) { m[static_cast<std::size_t>(k)] += x[static_cast<std::size_t>(k)] / static_cast<float>(ls.size()); }
        gross = std::max(gross, std::hypot(x[0], x[1], x[2]));
    }
    float abw = 0.0F;
    for (const modell::Vec3& x : ls) { abw = std::max(abw, std::hypot(x[0] - m[0], x[1] - m[1], x[2] - m[2]) / gross); }
    return abw;
}

// Ein Knochen in Weltachsen verschoben (Schluessel am Bild) bzw. gedreht, ein Rueckgaengig-Schritt je Aufruf
void verschieben(const char* name, int bild, float x, float y, float z) {
    const int k = knochen(name);
    const std::string sq = seq().name;
    g_app->mtab.dok.aendern("PH Test", [&](modell::Inhalt& in) {
        modell::RigSchicht& s = in.rig.schichten[sq];
        s.neuInterp = modell::Interp::Linear;
        std::vector<BoneMatrix> w;
        modell::rigWelt(g_app->anim, s, bild, bild, 0.0F, w);
        const float weg[3] = {x, y, z};
        modell::rigKnochenVerschieben(g_app->anim, s, k, bild, w, weg);
    }, false);
    g_app->modelDirty = true;
}

void drehen(const char* name, int bild, const float achse[3], float grad) {
    const int k = knochen(name);
    const std::string sq = seq().name;
    g_app->mtab.dok.aendern("PH Test", [&](modell::Inhalt& in) {
        modell::RigSchicht& s = in.rig.schichten[sq];
        s.neuInterp = modell::Interp::Linear;
        modell::rigSchluesselSetzen(s, k, bild, modell::drehungUm(achse, grad));
    }, false);
    g_app->modelDirty = true;
}

void schichtLeer(const char* was) {
    g_app->mtab.dok.aendern(was, [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
    g_app->modelDirty = true;
}

Schritt rechnen() {
    return warteBis("PH: Physik gerechnet", [] {
        modellPhysikAktualisieren();
        return g_app->mtab.physikStand == g_app->mtab.dok.stand();
    }, 300);
}

// Die Aktion der Gruppe Auto-Physik ueber die Suche (X / F3)
void ueberSuche(std::vector<Schritt>& s, Str text) {
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
    s.push_back(tu("PH: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; }));
    s.push_back(ik6::tasteSuche());
    s.push_back(pause(0.2));
    s.push_back(pruefe("PH Suche: Alle Aktionen suchen ist offen (X / F3)", [] { return g_app->mtab.aktionenSichtbar; }));
    const std::string zeile = tr(text);
    s.push_back(tippe(zeile));
    s.push_back(pause(0.2));
    s.push_back(klickAn("\"" + zeile + "\" in der Suche", [zeile] {
        for (const Element& e : g_elemente) {
            if ((e.label == zeile || e.label.rfind(zeile + " (", 0) == 0) && e.fenster.find("##Popup_") != std::string::npos) { return e.rect.GetCenter(); }
        }
        meldeFehler("PH: " + zeile + " nicht in der Suche: " + woIst(zeile));
        return ImVec2{};
    }));
    s.push_back(pause(0.3));
}

Schritt menueVoll(const std::string& wo) {
    return pruefe("PH " + wo + ": jede Aktion der Gruppe Auto-Physik steht da, mit Tooltip (keine Taste: weder 3ds Max noch Blender haben eine)", [wo] {
        int n = 0;
        for (int i = 0; i < modellRigAktionZahl(); ++i) {
            if (modellRigAktionGruppe(i) != 6) { continue; }
            const std::string text = modellRigAktionText(i);
            if (imMenue(text) == nullptr || std::string(modellRigAktionHinweis(i)).size() < 10U || modellRigAktionTaste(i, false)[0] != '\0' ||
                modellRigAktionTaste(i, true)[0] != '\0') {
                meldeFehler("PH: fehlt im Menue oder ohne Tooltip: " + text + " " + woIst(text));
                return false;
            }
            ++n;
        }
        diag::detail("Knopftest: PH " + wo + " - " + std::to_string(n) + " Aktionen mit Tooltip");
        return n >= 8;
    });
}

// --- Model-Tab -------------------------------------------------------------------------------------------------------

std::vector<Schritt> modellSchritte(bool handbuch) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<Zustand>();
    const std::string pfad = "models/players/stormtrooper/model.glm";
    add(tu("PH: Skript fuer den Test", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ph.txt"); }));
    for (Schritt& x : mb1::laden(pfad)) { add(std::move(x)); }
    add(tu("PH: Bereich Animation, eine ruhige BOTH_STAND* angehalten, wie 3ds Max, Rig, Schicht leer", [z] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Animation;
        // eine ruhige Sequenz mit genug Bildern (BOTH_STAND1 hat beim Sturmtruppler 2): die BOTH_STAND* mit mindestens 40 Bildern,
        // deren Becken und Fuesse sich am wenigsten bewegen
        int wahl = -1;
        float ruhe = 1e30F;
        const int lf = knochen("ltalus");
        const int rf = knochen("rtalus");
        const int pv = knochen("pelvis");
        for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
            const AnimEntry& e = g_app->animList[i];
            if (e.name.rfind("BOTH_STAND", 0) != 0 || e.numFrames < 40 || lf < 0 || rf < 0 || pv < 0) { continue; }
            std::vector<BoneMatrix> w0;
            std::vector<BoneMatrix> w;
            g_app->anim.worldMatrices(e.firstFrame, w0);
            float weg = 0.0F;
            for (int f = e.firstFrame; f < e.firstFrame + e.numFrames; f += 2) {
                g_app->anim.worldMatrices(f, w);
                for (const int k : {lf, rf, pv}) {
                    const auto uk = static_cast<std::size_t>(k);
                    weg = std::max(weg, std::hypot(w[uk].m[0][3] - w0[uk].m[0][3], w[uk].m[1][3] - w0[uk].m[1][3], w[uk].m[2][3] - w0[uk].m[2][3]));
                }
            }
            // und im Stand ohne Warnung (manche Posen stehen mit dem Schwerpunkt vor den Zehen)
            modell::PhysikUmgebung u;
            u.bodenZ = g_app->mtab.an.bodenZ;
            modell::PhysikBericht b;
            (void)modell::physikAnalyse(g_app->anim, modell::rigAnlegen(g_app->anim, g_app->mtab.dok.inhalt(), 0, {}), modell::RigSchicht{}, e.firstFrame, e.numFrames, u, b);
            if (weg < ruhe && b.warnungen == 0) {
                ruhe = weg;
                wahl = static_cast<int>(i);
            }
        }
        animWaehlen(wahl, false);
        g_app->animPlaying = false;
        g_app->animFrame = f0();
        g_app->animTime = 0.0F;
        (void)modellRigAnlegen();
        schichtLeer("PH leer");
        g_app->mtab.pfade.clear();
        z->erstes = seq().firstFrame;
        z->anzahl = seq().numFrames;
        z->mm = modell::massenModell(g_app->anim, g_app->mtab.rig);
        diag::detail("Knopftest: PH - " + seq().name + " " + std::to_string(z->anzahl) + " Bilder ab " + std::to_string(z->erstes) + ", " +
                     std::to_string(animBildMs(seq())) + " ms je Bild, Massenmodell " + (z->mm.gueltig() ? "da" : z->mm.fehler) + ", Boden " +
                     std::to_string(g_app->mtab.an.bodenZ));
    }));
    add(pause(0.5));
    add(pruefe("PH: Massenmodell des Sturmtrupplers (14 Segmente nach Winter), Sequenz mit mindestens 40 Bildern", [z] {
        // Messgrund der Kontakthoehe: die Fuesse der Standanimation des Spiels ueber dem Boden des Model-Tabs (ModView -24)
        for (const char* n : {"BOTH_STAND1", "BOTH_STAND2", "BOTH_WALK1", "BOTH_RUN1"}) {
            const int i = mb1::animNach(n);
            if (i < 0) { continue; }
            const AnimEntry& e = g_app->animList[static_cast<std::size_t>(i)];
            modell::PhysikUmgebung u;
            u.bodenZ = g_app->mtab.an.bodenZ;
            modell::PhysikBericht b;
            (void)modell::physikAnalyse(g_app->anim, g_app->mtab.rig, modell::RigSchicht{}, e.firstFrame, e.numFrames, u, b);
            float lo[2] = {1e9F, 1e9F};
            float hi[2] = {-1e9F, -1e9F};
            for (const modell::PhysikBild& pb : b.bilder) {
                for (int k = 0; k < 2; ++k) {
                    lo[k] = std::min(lo[k], pb.fussHoehe[static_cast<std::size_t>(k)]);
                    hi[k] = std::max(hi[k], pb.fussHoehe[static_cast<std::size_t>(k)]);
                }
            }
            diag::detail(std::string("Knopftest: PH Fusshoehe ") + n + " - links " + zahl(lo[0]) + " bis " + zahl(hi[0]) + ", rechts " + zahl(lo[1]) + " bis " + zahl(hi[1]));
        }
        return z->mm.gueltig() && z->mm.segmente.size() == 14U && z->anzahl >= 40;
    }));
    // --- Erreichbarkeit: Menue Animation > Auto-Physik, Rechtsklick, Suche ---
    add(taste(ImGuiKey_Escape, "Esc"));
    add(klickMenue(tr(Str::MbBereichAnimation), true));
    add(pause(0.3));
    add(klickMenue(tr(Str::PhRollout), false));
    add(pause(0.4));
    add(menueVoll("Menue Animation > Auto-Physik"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(klickAn("Rechtsklick in die Ansicht", [] { return kv5::fokusPunkt(); }, 1));
    add(pause(0.3));
    add(klickMenue(tr(Str::MbRgGruppeRig), false));
    add(pause(0.3));
    add(klickMenue(tr(Str::PhRollout), false));
    add(pause(0.4));
    add(menueVoll("Rechtsklick > Rig > Auto-Physik"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    // --- Sprung: Becken als Dreieck, ein Arm schwingt (Rot ohne Physik) ---
    add(tu("PH Sprung: Becken Bild 10 -> 20 (30 hoch, 30 vor) -> 30 (60 vor), Arm L 0 -> 120 Grad (12 -> 28)", [z] {
        const int b = z->erstes;
        // rigKnochenVerschieben verschiebt vom jetzigen Stand am Bild aus (nach dem letzten Schluessel gilt dessen Wert)
        verschieben("pelvis", b + 10, 0.0F, 0.0F, 0.0F);
        verschieben("pelvis", b + 20, 0.0F, -30.0F, 30.0F);
        verschieben("pelvis", b + 30, 0.0F, -30.0F, -30.0F);
        verschieben("pelvis", b + 39, 0.0F, 0.0F, 0.0F);
        const float x[3] = {1.0F, 0.0F, 0.0F};
        drehen("lhumerus", b + 12, x, 0.0F);
        drehen("lhumerus", b + 28, x, 120.0F);
        g_app->animFrame = b + 20;
    }));
    add(rechnen());
    add(pruefe("PH Rot: ohne Physik weicht der Schwerpunkt im Flug von der Parabel ab, der Drehimpuls schwankt", [z] {
        const modell::PhysikBericht& b = g_app->mtab.physik;
        if (b.fluege.empty()) {
            meldeFehler("PH: kein Flug erkannt");
            return false;
        }
        z->a = b.fluege.front().first;
        z->l = b.fluege.front().second;
        z->parabelOhne = parabel(z->mm, sch(), z->a, z->l, 800.0);
        z->drehOhne = drehSchwankung(z->mm, sch(), z->a, z->l);
        diag::detail("Knopftest: PH Rot - Flug " + std::to_string(z->a - z->erstes) + " bis " + std::to_string(z->l - z->erstes) + ", Parabel " +
                     zahl(z->parabelOhne) + ", Drehimpuls " + zahl(z->drehOhne * 100.0F) + " %");
        return z->parabelOhne > 5.0F && z->drehOhne > 0.2F;
    }));
    // Einschalten ueber die Suche (ein Rueckgaengig-Schritt), das Rollout geht auf
    add(tu("PH: Rueckgaengig-Tiefe und Schicht merken", [z] {
        z->undo = g_app->mtab.dok.kannRueckgaengig() ? 1U : 0U;
        z->vorher = sch();
    }));
    ueberSuche(s, Str::PhAn);
    add(rechnen());
    add(pause(0.4));
    add(pruefe("PH: Auto-Physik an ueber die Suche, ein Rueckgaengig-Schritt \"Auto-Physik an/aus\", das Rollout ist offen", [] {
        const modell::PhysikEinstellung* e = modellPhysikEinstellung();
        diag::detail("Knopftest: PH an - " + std::to_string(e != nullptr && e->an) + ", Rueckgaengig \"" + g_app->mtab.dok.rueckgaengigName() + "\", Rollout im Bild " +
                     std::to_string(g_app->mtab.physikRolloutBild) + " (jetzt " + std::to_string(ImGui::GetFrameCount()) + ")");
        return e != nullptr && e->an && g_app->mtab.dok.rueckgaengigName() == tr(Str::PhUndoAn) && g_app->mtab.physikRolloutBild >= ImGui::GetFrameCount() - 3;
    }));
    add(pruefe("PH Gruen: Schwerpunkt auf der Parabel (g 800), Drehimpuls konstant, Schluessel unangetastet, Physik in eigener Spur", [z] {
        const modell::RigSchicht& s = sch();
        const float p = parabel(z->mm, s, z->a, z->l, 800.0);
        const float d = drehSchwankung(z->mm, s, z->a, z->l);
        bool gleich = s.knochen.size() == z->vorher.knochen.size();
        for (std::size_t k = 0; gleich && k < s.knochen.size(); ++k) {
            gleich = s.knochen[k].size() == z->vorher.knochen[k].size() &&
                     (s.knochen[k].empty() || std::memcmp(s.knochen[k].data(), z->vorher.knochen[k].data(), s.knochen[k].size() * sizeof(modell::RigSchluessel)) == 0);
        }
        int spur = 0;
        for (const auto& v : s.physik.knochen) { spur += v.empty() ? 0 : 1; }
        diag::detail("Knopftest: PH Gruen - Parabel " + zahl(p) + " (vorher " + zahl(z->parabelOhne) + "), Drehimpuls " + zahl(d * 100.0F) + " % (vorher " +
                     zahl(z->drehOhne * 100.0F) + " %), Schluessel gleich " + std::to_string(gleich) + ", Spur " + std::to_string(spur) + " Knochen, Rechnung " +
                     zahl(g_app->mtab.physikMs) + " ms");
        return p < 0.05F && d < 0.01F && gleich && spur > 0;
    }));
    add(tu("PH: Bild mitten im Flug", [z] { g_app->animFrame = (z->a + z->l) / 2; }));
    add(pause(0.5));
    add(pruefe("PH Ansicht: Schwerpunkt, Weg und Stuetzflaeche gezeichnet", [] {
        diag::detail("Knopftest: PH Ansicht - " + std::to_string(g_app->mtab.physikGezeichnet) + " Teile");
        return g_app->mtab.physikGezeichnet >= 2;
    }));
    if (handbuch) {
        add(tu("PH Handbuch: Ansicht von links (orthographisch), Figur und Sprung im Bild", [] {
            ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
            a.art = AnsichtArt::Links;
            modellAnsichtEinpassenAktiv();
            a.mitte[1] -= 30.0F;   // der Sprung geht 60 nach vorn (-Y)
            a.mitte[2] += 20.0F;
            a.halbHoehe = 80.0F;
            g_app->modelDirty = true;
        }));
        add(pause(0.4));
        add(rig3::rollenZu(tr(Str::PhRollout), "mtrollouts"));
        add(tu("PH Handbuch: Maus in die Ansicht (kein Tooltip im Bild)", [] { setzeMaus(kv5::fokusPunkt()); }));
        add(pause(0.8));
        add(hilfe2::schrittFoto("h_ph_sprung"));
        return s;
    }
    // Staerke 0,5
    add(tu("PH: Staerke 0,5", [] { (void)modellPhysikEinstellen("PH Staerke", [](modell::PhysikEinstellung& e) { e.staerke = 0.5F; }); }));
    add(rechnen());
    add(pruefe("PH: Staerke 0,5 liegt zwischen ohne und mit Physik", [z] {
        const float p = parabel(z->mm, sch(), z->a, z->l, 800.0);
        diag::detail("Knopftest: PH Staerke 0,5 - Parabel " + zahl(p));
        return p > 0.05F && p < z->parabelOhne;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(rechnen());
    // Filter aus -> gebacken Byte fuer Byte wie ohne Physik
    add(pruefe("PH Filter: aus -> gebacken Byte fuer Byte wie ohne Physik, an -> anders und die GLA traegt die Parabel", [z] {
        modell::RigSchicht aus = sch();
        aus.physik.e.an = false;
        modell::RigSchicht ohne = sch();
        ohne.physik = modell::PhysikSchicht{};
        GlaAnimation g0;
        GlaAnimation g1;
        GlaAnimation g2;
        (void)modell::rigBacken(g_app->anim, ohne, z->erstes, z->anzahl, false, "x", g0);
        (void)modell::rigBacken(g_app->anim, aus, z->erstes, z->anzahl, false, "x", g1);
        (void)modell::rigBacken(g_app->anim, sch(), z->erstes, z->anzahl, false, "x", g2);
        // die gebackene GLA ohne Schicht: dieselben Bilder ab 0
        std::vector<BoneMatrix> w;
        float p = 0.0F;
        {
            const double dt = bildSek();
            const int a = z->a - z->erstes;
            const int l = z->l - z->erstes;
            g2.worldMatrices(a, w);
            const modell::Vec3 ca = modell::schwerpunkt(g2, z->mm, w);
            g2.worldMatrices(l, w);
            const modell::Vec3 cl = modell::schwerpunkt(g2, z->mm, w);
            const double tt = (l - a) * dt;
            for (int f = a; f <= l; ++f) {
                const double t = (f - a) * dt;
                g2.worldMatrices(f, w);
                const modell::Vec3 c = modell::schwerpunkt(g2, z->mm, w);
                double d2 = 0.0;
                for (int k = 0; k < 3; ++k) {
                    const double gk = k == 2 ? -800.0 : 0.0;
                    const double v0 = (cl[static_cast<std::size_t>(k)] - ca[static_cast<std::size_t>(k)] - 0.5 * gk * tt * tt) / tt;
                    const double q = ca[static_cast<std::size_t>(k)] + v0 * t + 0.5 * gk * t * t;
                    d2 += (c[static_cast<std::size_t>(k)] - q) * (c[static_cast<std::size_t>(k)] - q);
                }
                p = std::max(p, static_cast<float>(std::sqrt(d2)));
            }
        }
        z->bauParabel = p;
        const bool aus0 = g0.bonePool == g1.bonePool && g0.indexes == g1.indexes;
        diag::detail("Knopftest: PH Filter - aus gleich " + std::to_string(aus0) + ", an anders " + std::to_string(g0.bonePool != g2.bonePool) +
                     ", gebackene Parabel " + zahl(p));
        return aus0 && g0.bonePool != g2.bonePool && p < 0.1F;
    }));
    // Ein Rueckgaengig-Schritt: Strg+Z schaltet aus (Spur leer), Strg+Y wieder an
    add(tu("PH: Maus in die Ansicht", [] { setzeMaus(kv5::fokusPunkt()); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(rechnen());
    add(pruefe("PH: ein Strg+Z schaltet die Physik aus, die Spur ist leer, die Animation wie vorher", [z] {
        const modell::PhysikEinstellung* e = modellPhysikEinstellung();
        const float p = parabel(z->mm, sch(), z->a, z->l, 800.0);
        diag::detail("Knopftest: PH Strg+Z - an " + std::to_string(e != nullptr && e->an) + ", Spur " + std::to_string(sch().physik.knochen.size()) + ", Parabel " + zahl(p));
        return e != nullptr && !e->an && sch().physik.knochen.empty() && std::fabs(p - z->parabelOhne) < 1e-3F;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Y, "Strg+Y"));
    add(rechnen());
    add(pruefe("PH: Strg+Y schaltet sie wieder an (Parabel)", [z] {
        const modell::PhysikEinstellung* e = modellPhysikEinstellung();
        return e != nullptr && e->an && parabel(z->mm, sch(), z->a, z->l, 800.0) < 0.05F;
    }));
    // --- Kontakte: Becken wandert bei gebeugten Knien ---
    add(tu("PH Kontakte: Schicht leer, Knie gebeugt (Becken 6 tiefer), das Becken wandert 4 Einheiten vor", [z] {
        schichtLeer("PH Kontakte");
        const int b = z->erstes;
        verschieben("pelvis", b, 0.0F, 0.0F, -6.0F);
        verschieben("pelvis", b + 30, 0.0F, -4.0F, 0.0F);   // vom jetzigen Stand (-6) aus
        g_app->animFrame = b + 30;
    }));
    add(rechnen());
    const auto rutschen = [z] {
        // das Becken senkt sich mit den Fuessen (Kinder): die Fuesse stehen 6 tiefer, der Boden folgt fuer die Probe mit
        const modell::RigSchicht& s = sch();
        std::vector<BoneMatrix> w;
        modell::rigWelt(g_app->anim, s, z->erstes, z->erstes, 0.0F, w);
        const modell::Vec3 p0 = modell::punktWelt(g_app->anim, z->mm.fuss[0].front(), w);
        float weg = 0.0F;
        for (int f = z->erstes; f <= z->erstes + 30; ++f) {
            modell::rigWelt(g_app->anim, s, f, f, 0.0F, w);
            const modell::Vec3 p = modell::punktWelt(g_app->anim, z->mm.fuss[0].front(), w);
            weg = std::max(weg, std::hypot(p[0] - p0[0], p[1] - p0[1]));
        }
        return weg;
    };
    add(tu("PH Kontakte: Boden 6 tiefer (die Fuesse gehen mit dem Becken), Physik aus", [] {
        g_app->mtab.an.bodenZ -= 6.0F;
        (void)modellPhysikEinstellen("PH aus", [](modell::PhysikEinstellung& e) { e.an = false; });
    }));
    add(rechnen());
    add(pruefe("PH Rot: ohne Physik rutscht der linke Fuss mit dem Becken", [z, rutschen] {
        z->rutschenOhne = rutschen();
        diag::detail("Knopftest: PH Kontakte rot - Fuss rutscht " + zahl(z->rutschenOhne) + ", Kontakt Bild 30 " +
                     std::to_string(g_app->mtab.physik.bilder.size() > 30U && g_app->mtab.physik.bilder[30].kontakt[0]));
        return z->rutschenOhne > 3.0F;
    }));
    add(tu("PH Kontakte: Physik an", [] { (void)modellPhysikAn(true); }));
    add(rechnen());
    add(pruefe("PH Gruen: mit Physik bleibt der Fuss stehen (Bein-IK)", [z, rutschen] {
        const float w = rutschen();
        diag::detail("Knopftest: PH Kontakte gruen - Fuss rutscht " + zahl(w) + " (vorher " + zahl(z->rutschenOhne) + ")");
        return w < 0.05F;
    }));
    add(tu("PH: Boden zurueck", [] { g_app->mtab.an.bodenZ += 6.0F; }));
    // --- Stuetzflaeche: vorgebeugt ---
    add(tu("PH Stuetzflaeche: Schicht leer, Physik an", [] {
        schichtLeer("PH Stand");
        (void)modellPhysikAn(true);
    }));
    add(rechnen());
    add(pruefe("PH: im Stand liegt der Schwerpunkt innen (keine Warnung)", [] {
        diag::detail("Knopftest: PH Stand - Warnungen " + std::to_string(g_app->mtab.physik.warnungen) + ", Stand Bild 0 " +
                     std::to_string(!g_app->mtab.physik.bilder.empty() && g_app->mtab.physik.bilder[0].imStand) + ", Ecken " +
                     std::to_string(g_app->mtab.physik.bilder.empty() ? 0 : g_app->mtab.physik.bilder[0].stuetz.size()));
        return g_app->mtab.physik.warnungen == 0 && !g_app->mtab.physik.bilder.empty() && g_app->mtab.physik.bilder[0].imStand;
    }));
    add(tu("PH Stuetzflaeche: ab Bild 20 der Oberkoerper 90 Grad vorgebeugt (um die Seitenachse X der Welt)", [z] {
        const int k = knochen("lower_lumbar");
        const std::string sq = seq().name;
        const float x[3] = {1.0F, 0.0F, 0.0F};
        // vorn ist -Y: +90 um +X kippt oben (+Z) nach -Y; fehlt die Warnung, die Gegenrichtung (die Probe soll sie finden)
        for (const float grad : {90.0F, -90.0F}) {
            g_app->mtab.dok.aendern("PH Stand", [&](modell::Inhalt& in) {
                modell::RigSchicht& s = in.rig.schichten[sq];
                s.neuInterp = modell::Interp::Linear;
                std::vector<BoneMatrix> w;
                modell::rigWelt(g_app->anim, s, z->erstes + 20, z->erstes + 20, 0.0F, w);
                const BoneMatrix& xw = w[static_cast<std::size_t>(k)];
                BoneMatrix r = modell::drehungUm(x, grad);
                for (int a = 0; a < 3; ++a) {
                    r.m[a][3] = xw.m[a][3] - (r.m[a][0] * xw.m[0][3] + r.m[a][1] * xw.m[1][3] + r.m[a][2] * xw.m[2][3]);
                }
                modell::rigSchluesselSetzen(s, k, z->erstes + 19, BoneMatrix::identity());
                modell::rigSchluesselSetzen(s, k, z->erstes + 20, modell::starrInvers(xw) * r * xw);
            }, false);
            modell::PhysikUmgebung u;
            u.bodenZ = g_app->mtab.an.bodenZ;
            modell::PhysikBericht b;
            (void)modell::physikAnalyse(g_app->anim, g_app->mtab.rig, sch(), z->erstes, z->anzahl, u, b);
            diag::detail("Knopftest: PH Stand " + std::to_string(grad) + " Grad - Warnungen " + std::to_string(b.warnungen));
            if (b.warnungen > 0) { break; }
        }
        g_app->modelDirty = true;
        g_app->animFrame = z->erstes + 25;
    }));
    add(rechnen());
    add(pause(0.4));
    add(pruefe("PH: Warnung ab Bild 20 (Schwerpunkt ausserhalb der Stuetzflaeche), die Ansicht zeigt sie", [z] {
        const modell::PhysikBericht& b = g_app->mtab.physik;
        diag::detail("Knopftest: PH Warnung - ab Bild " + std::to_string(b.warnBild - z->erstes) + ", " + std::to_string(b.warnungen) + " Bilder, Ansicht " +
                     std::to_string(g_app->mtab.physikGezeichnet));
        return b.warnBild == z->erstes + 20 && g_app->mtab.physikGezeichnet >= 2;
    }));
    // --- Federkette ---
    add(tu("PH Federkette: Schicht leer, Ketten vorschlagen (Menue-Aktion), Ruck der Huefte bei Bild 5 -> 7", [z] {
        schichtLeer("PH Feder");
        (void)modellPhysikAn(true);
        // nur die Feder: die Huefte dreht den Fuss, die Kontakte hielten ihn (der Oberschenkel und mit ihm ltail wichen ab)
        (void)modellPhysikEinstellen("PH Feder", [](modell::PhysikEinstellung& e) { e.kontakte = false; });
        for (int i = 0; i < modellRigAktionZahl(); ++i) {
            if (std::string(modellRigAktionText(i)) == tr(Str::PhKettenVorschlag)) { modellRigAktionAusloesen(i); }
        }
        const float zachse[3] = {0.0F, 0.0F, 1.0F};
        drehen("lfemurYZ", z->erstes + 5, zachse, 0.0F);
        drehen("lfemurYZ", z->erstes + 7, zachse, 40.0F);
    }));
    add(rechnen());
    add(pruefe("PH Federkette: ltail und rtail als Ketten; ltail schwingt nach und kommt zur Ruhe", [z] {
        const modell::PhysikEinstellung* e = modellPhysikEinstellung();
        const int lt = knochen("ltail");
        bool hat = false;
        for (const modell::PhysikKette& k : e != nullptr ? e->ketten : std::vector<modell::PhysikKette>{}) { hat = hat || k.knochen == lt; }
        modell::RigSchicht ohne = sch();
        ohne.physik.e.an = false;
        std::vector<float> fehler;
        std::vector<BoneMatrix> w0;
        std::vector<BoneMatrix> w1;
        for (int f = z->erstes; f < z->erstes + z->anzahl; ++f) {
            modell::rigWelt(g_app->anim, ohne, f, f, 0.0F, w0);
            modell::rigWelt(g_app->anim, sch(), f, f, 0.0F, w1);
            const BoneMatrix a = modell::starr(w0[static_cast<std::size_t>(lt)]);
            const BoneMatrix b = modell::starr(w1[static_cast<std::size_t>(lt)]);
            double tr = 0.0;
            for (int i = 0; i < 3; ++i) {
                for (int k = 0; k < 3; ++k) { tr += static_cast<double>(a.m[k][i]) * b.m[k][i]; }
            }
            fehler.push_back(static_cast<float>(std::acos(std::clamp((tr - 1.0) * 0.5, -1.0, 1.0)) * 57.29578));
        }
        const float frueh = fehler.size() > 20U ? *std::max_element(fehler.begin() + 6, fehler.begin() + 20) : 0.0F;
        const float spaet = fehler.empty() ? 0.0F : fehler.back();
        diag::detail("Knopftest: PH Federkette - " + std::to_string(e != nullptr ? e->ketten.size() : 0) + " Ketten, ltail " + std::to_string(hat) + ", Nachlauf " +
                     zahl(frueh) + " Grad, am Ende " + zahl(spaet) + " Grad, " + std::to_string(g_app->mtab.physik.kettenKnochen) + " Kettenknochen");
        return hat && frueh > 3.0F && spaet < frueh * 0.1F;
    }));
    // --- wie Blender: F3 > Auto-Physik einstellen, Menue ---
    add(tu("PH: Rollout zu, wie Blender", [] {
        modellBedienungSetzen(1);
        g_app->mtab.physikRolloutBild = -1;
    }));
    add(pause(0.3));
    ueberSuche(s, Str::PhRolloutOeffnen);
    add(pause(0.3));
    add(pruefe("PH wie Blender: F3 > Auto-Physik einstellen oeffnet das Rollout", [] {
        diag::detail("Knopftest: PH Blender - Rollout im Bild " + std::to_string(g_app->mtab.physikRolloutBild) + " (jetzt " + std::to_string(ImGui::GetFrameCount()) + ")");
        return g_app->mtab.physikRolloutBild >= ImGui::GetFrameCount() - 3;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(klickMenue(tr(Str::MbBereichAnimation), true));
    add(pause(0.3));
    add(klickMenue(tr(Str::PhRollout), false));
    add(pause(0.4));
    add(menueVoll("wie Blender: Menue Animation > Auto-Physik"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(tu("PH: wieder wie 3ds Max, Schicht leer", [] {
        modellBedienungSetzen(0);
        schichtLeer("PH Ende");
    }));
    return s;
}

// --- Mission-Tab: eine Figur springt --------------------------------------------------------------------------------------

// Der Schwerpunkt der Figur in der Karte, wie die Vorschau sie zeichnet (gebackene Karten-GLA)
bool comKarte(const std::string& fig, double ms, std::array<float, 3>& c, std::array<float, 3>* becken = nullptr) {
    std::vector<BoneMatrix> w;
    float fw[16];
    const GlaAnimation* g = nullptr;
    if (!fa7::gezeichnet(fig, ms, w, fw, &g) || g == nullptr) { return false; }
    static std::map<const GlaAnimation*, modell::MassenModell> mms;
    auto it = mms.find(g);
    if (it == mms.end()) { it = mms.emplace(g, modell::massenModell(*g, modell::rigAnlegen(*g, modell::Inhalt{}, 0, {}))).first; }
    if (!it->second.gueltig()) { return false; }
    const modell::Vec3 p = modell::schwerpunkt(*g, it->second, w);
    for (int r = 0; r < 3; ++r) { c[static_cast<std::size_t>(r)] = fw[r * 4] * p[0] + fw[r * 4 + 1] * p[1] + fw[r * 4 + 2] * p[2] + fw[r * 4 + 3]; }
    if (becken != nullptr) { *becken = fa7::knochenInKarte(w, fw, *g, "pelvis"); }
    return true;
}

// Die Parabel des Schwerpunkts in der Karte (Vorschau aus der gebackenen GLA), Flug aus dem Bericht der Figur
float missionParabel(const std::string& fig, const modell::PhysikBericht& b, int a, int l) {
    const double start = figurAnimStartMs(fig);
    const double dt = 0.05;   // 20 fps (frameLerp 50 ms)
    std::array<float, 3> ca{};
    std::array<float, 3> cl{};
    if (!comKarte(fig, start + a * 50.0 + 1.0, ca) || !comKarte(fig, start + l * 50.0 + 1.0, cl)) { return -1.0F; }
    (void)b;
    const double g = g_app->scene.schwerkraft;
    const double tt = (l - a) * dt;
    float abw = 0.0F;
    for (int f = a; f <= l; ++f) {
        std::array<float, 3> c{};
        if (!comKarte(fig, start + f * 50.0 + 1.0, c)) { return -1.0F; }
        const double t = (f - a) * dt;
        double d2 = 0.0;
        for (int k = 0; k < 3; ++k) {
            const double gk = k == 2 ? -g : 0.0;
            const double v0 = (cl[static_cast<std::size_t>(k)] - ca[static_cast<std::size_t>(k)] - 0.5 * gk * tt * tt) / tt;
            const double q = ca[static_cast<std::size_t>(k)] + v0 * t + 0.5 * gk * t * t;
            d2 += (c[static_cast<std::size_t>(k)] - q) * (c[static_cast<std::size_t>(k)] - q);
        }
        abw = std::max(abw, static_cast<float>(std::sqrt(d2)));
    }
    return abw;
}

// mb7_a springt: Becken Bild 10 -> 20 (40 hoch, 40 vor) -> 30 (80 vor), Animation 50 Bilder ab 1 s
void sprungAufbauen(std::vector<Schritt>& s) {
    const auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("PH Mission: mb7_a animieren (50 Bilder ab 1 s)", [] {
        g_app->settings.spielSeiten = 1920.0F / 1440.0F;
        g_app->playMs = 1000.0;
        (void)figurAnimStarten("mb7_a");
        (void)figurAnimEinstellen([](modell::FigurAnim& a) { a.bilder = 50; });
    }));
    add(warteBis("PH Mission: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    add(tu("PH Mission: Sprung ueber den Wurzelweg (+X der Karte)", [] {
        const float null[3] = {0.0F, 0.0F, 0.0F};
        const float oben[3] = {40.0F, 0.0F, 40.0F};
        const float vor[3] = {40.0F, 0.0F, -40.0F};
        // wie ein gezogener Punkt: vom jetzigen Stand am Bild aus (nach dem letzten Schluessel gilt dessen Wert)
        (void)figurAnimBeckenWeg("mb7_a", 0, null);
        (void)figurAnimBeckenWeg("mb7_a", 10, null);
        (void)figurAnimBeckenWeg("mb7_a", 20, oben);
        (void)figurAnimBeckenWeg("mb7_a", 30, vor);
        (void)figurAnimBeckenWeg("mb7_a", 49, null);
    }));
    add(warteBis("PH Mission: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
}

std::vector<Schritt> missionSchritte(bool handbuch) {
    std::vector<Schritt> s;
    const auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto fz = std::make_shared<fa7::Zustand>();
    auto z = std::make_shared<Zustand>();
    fa7::aufbau(s, fz);
    sprungAufbauen(s);
    add(pruefe("PH Mission Rot: ohne Physik weicht der Schwerpunkt der gebackenen Karten-GLA im Flug von der Parabel ab", [z] {
        // Flug: der Bericht kommt erst mit Physik; ohne nimmt die Probe die Bilder 10..30 (die Wurzel-Schluessel)
        z->missionOhne = missionParabel("mb7_a", {}, 10, 30);
        diag::detail("Knopftest: PH Mission rot - Parabel " + zahl(z->missionOhne) + ", Schwerkraft der Karte " + std::to_string(g_app->scene.schwerkraft));
        return z->missionOhne > 5.0F;
    }));
    // Erreichbarkeit im Fenster: Menue Auto-Physik
    add(klickAn("Menue Auto-Physik im Fenster Figur animieren", [] {
        const Element* e = finde(tr(Str::PhRollout), "", 0, false);
        for (const Element& x : g_elemente) {
            if (x.label == tr(Str::PhRollout) && x.fenster.find("###figuranim") != std::string::npos) { return x.rect.GetCenter(); }
        }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    }));
    add(pause(0.4));
    add(pruefe("PH Mission: Menue Auto-Physik des Fensters mit allen Aktionen und Tooltips", [] {
        int n = 0;
        for (const Str t : {Str::PhAn, Str::PhZeigen, Str::PhAbsprungHier, Str::PhLandungHier, Str::PhFlugAutomatisch, Str::PhKettenVorschlag}) {
            n += imMenue(tr(t)) != nullptr ? 1 : 0;
        }
        diag::detail("Knopftest: PH Mission Menue - " + std::to_string(n) + " von 6; " + tr(Str::PhAn) + " " + woIst(tr(Str::PhAn)));
        return n == 6;
    }));
    add(klickAn(std::string("\"") + tr(Str::PhAn) + "\" im Menue", [] {
        const Element* e = imMenue(tr(Str::PhAn));
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    }));
    add(pause(0.3));
    add(warteBis("PH Mission: Szene mit Physik", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    add(pause(0.5));
    add(pruefe("PH Mission Gruen: Physik an (Zeile physik im Skript), Flug erkannt, Schwerpunkt der gebackenen Karten-GLA auf der Parabel", [z] {
        const modell::PhysikBericht b = figurAnimPhysikBericht("mb7_a");
        const std::string text = writeScript(g_app->doc.script());
        const bool zeile = text.find("figur+ \"mb7_a\" physik 1") != std::string::npos;
        if (b.fluege.empty()) {
            std::string h;
            for (std::size_t i = 0; i < b.bilder.size(); i += 5) {
                h += " " + std::to_string(i) + ":" + zahl(b.bilder[i].fussHoehe[0]) + "/" + zahl(b.bilder[i].bodenZ);
            }
            meldeFehler("PH Mission: kein Flug (" + b.fehler + "), " + std::to_string(b.bilder.size()) + " Bilder, Fusshoehe/Boden:" + h);
            return false;
        }
        const float p = missionParabel("mb7_a", b, b.fluege.front().first, b.fluege.front().second);
        diag::detail("Knopftest: PH Mission gruen - Zeile " + std::to_string(zeile) + ", Flug " + std::to_string(b.fluege.front().first) + " bis " +
                     std::to_string(b.fluege.front().second) + ", Parabel " + zahl(p) + " (vorher " + zahl(z->missionOhne) + "), im Bericht " + zahl(b.parabelAbw) +
                     ", Drehimpuls " + zahl(b.drehimpulsNachher * 100.0F) + " %");
        // die Karten-GLA ist gepackt (1/64 Einheit, Quaternion 1/16383): halbe Einheit Spielraum
        return zeile && p >= 0.0F && p < 0.5F;
    }));
    add(tu("PH Mission: Zeit mitten im Flug, Fenster offen, Griffe an", [] {
        g_app->figAnim.offen = true;
        g_app->figAnim.posieren = true;
        g_app->playing = false;
        g_app->playMs = figurAnimStartMs("mb7_a") + 20.0 * 50.0;
        g_app->mapDirty = true;
    }));
    add(pause(0.6));
    add(pruefe("PH Mission: Schwerpunkt, Weg und Stuetzflaeche in der Karte gezeichnet, Rollout Auto-Physik offen", [] {
        diag::detail("Knopftest: PH Mission Ansicht - " + std::to_string(g_app->figAnim.physikGezeichnet) + " Teile, Rollout im Bild " +
                     std::to_string(g_app->figAnim.physikRolloutBild));
        return g_app->figAnim.physikGezeichnet >= 2 && g_app->figAnim.physikRolloutBild >= ImGui::GetFrameCount() - 3;
    }));
    if (handbuch) {
        add(fa7::fensterImBild());
        add(pause(0.3));
        add(tu("PH Handbuch: Kamera von der Seite auf den Sprung, Fenster links", [fz] {
            const int ea = entityFuerFigur(fa7::figur("mb7_a"));
            float oa[3]{};
            if (ea >= 0) { (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(ea)], oa); }
            g_app->throughCamera = false;
            g_app->cam.pos[0] = oa[0] + 40.0F;
            g_app->cam.pos[1] = oa[1] - 230.0F;
            g_app->cam.pos[2] = fz->boden + 70.0F;
            const float ziel[3] = {oa[0] + 40.0F, oa[1], fz->boden + 60.0F};
            aimAngles(g_app->cam.pos, ziel, g_app->cam.angles);
            g_app->mapDirty = true;
            if (ImGuiWindow* w = ImGui::FindWindowByName("###figuranim")) {
                const ImGuiViewport* vp = ImGui::GetMainViewport();
                const float fs = ImGui::GetFontSize();
                ImGui::SetWindowPos(w, ImVec2{vp->WorkPos.x + fs * 0.5F, vp->WorkPos.y + fs * 4.0F});
                ImGui::SetWindowSize(w, ImVec2{fs * 34.0F, vp->WorkSize.y - fs * 6.0F});   // die Breite der Vorgabe (Menueleiste in allen Sprachen)
            }
        }));
        add(pause(0.4));
        add(tu("PH Handbuch: Rollout Auto-Physik im Fenster ins Bild rollen", [] {
            ImGuiWindow* w = ImGui::FindWindowByName("###figuranim");
            const Element* kopf = nullptr;
            for (const Element& e : g_elemente) {
                if (e.label == tr(Str::PhRollout) && e.fenster.find("###figuranim") != std::string::npos && (kopf == nullptr || e.rect.Min.y > kopf->rect.Min.y)) { kopf = &e; }
            }
            if (w != nullptr && kopf != nullptr) {
                ImGui::SetScrollY(w, std::clamp(w->Scroll.y + kopf->rect.Min.y - w->InnerRect.Min.y - ImGui::GetFontSize() * 2.0F, 0.0F, w->ScrollMax.y));
            }
            // die Maus in die Ecke der Karte (ueber der Mitte stand ein Spawner: sein Name im Bild)
            setzeMaus(ImVec2{g_app->kartenAnsicht[0] + 12.0F, g_app->kartenAnsicht[3] - 12.0F});
        }));
        add(pause(0.8));
        add(hilfe2::schrittFoto("h_ph_figur"));
        return s;
    }
    add(pruefe("PH Mission: das Fenster Figur animieren liegt ganz im Hauptfenster (auch bei grosser Schrift)", [] {
        const ImGuiWindow* w = ImGui::FindWindowByName("###figuranim");
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        if (w == nullptr) { return false; }
        diag::detail("Knopftest: PH Fenster - " + std::to_string(w->Pos.y) + " + " + std::to_string(w->Size.y) + " in " + std::to_string(vp->WorkPos.y) + " + " +
                     std::to_string(vp->WorkSize.y) + ", eigenes Fenster " + std::to_string(w->Viewport != vp));
        return w->Viewport == vp && w->Pos.y + w->Size.y <= vp->WorkPos.y + vp->WorkSize.y + 0.5F;
    }));
    add(tu("PH Mission: Maus ueber die Karte", [] { setzeMaus(ansichtMitte()); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.5));
    add(pruefe("PH Mission: ein Strg+Z nimmt die Physik zurueck (Zeile physik weg)", [] {
        const std::string text = writeScript(g_app->doc.script());
        return text.find("figur+ \"mb7_a\" physik 1") == std::string::npos;
    }));
    // pfix M8: Regler und Zugfelder des Fensters mit der Maus - der beim Loslassen eingestellte Wert kommt ins Dokument
    auto vorher = std::make_shared<modell::FigurAnim>();
    add(tu("PF M8: Physik an", [vorher] {
        (void)figurAnimPhysikAn(true);
        *vorher = figurAnimKopie("mb7_a");
    }));
    add(pause(0.4));
    const auto insBild = [](const std::string& label) {
        return tu("PF M8: \"" + label + "\" ins Bild rollen", [label] {
            ImGuiWindow* w = ImGui::FindWindowByName("###figuranim");
            const Element* e = finde(label, "###figuranim", 0, false);
            if (w != nullptr && e != nullptr) {
                ImGui::SetScrollY(w, std::clamp(w->Scroll.y + e->rect.Min.y - w->InnerRect.Min.y - ImGui::GetFontSize() * 4.0F, 0.0F, w->ScrollMax.y));
            }
        });
    };
    add(insBild(tr(Str::PhStaerke)));
    add(pause(0.4));
    add(reglerZiehen(tr(Str::PhStaerke), "###figuranim", 0, 0.9F, 0.3F));
    add(pause(0.4));
    add(pruefe("PF M8: Staerke nach dem Ziehen wie eingestellt (um 0,3), nicht der alte Wert", [vorher] {
        const modell::FigurAnim a = figurAnimKopie("mb7_a");
        diag::detail("Knopftest: PF M8 Staerke - " + std::to_string(a.schicht.physik.e.staerke) + " (vorher " + std::to_string(vorher->schicht.physik.e.staerke) + ")");
        return a.schicht.physik.e.staerke < 0.5F && a.schicht.physik.e.staerke > 0.1F;
    }));
    add(tu("PF M8: Fenster nach oben rollen (das Feld Bilder steht oben)", [] {
        if (ImGuiWindow* w = ImGui::FindWindowByName("###figuranim")) { ImGui::SetScrollY(w, 0.0F); }
    }));
    add(pause(0.3));
    add(insBild("##fabilder"));
    add(pause(0.4));
    add(reglerZiehen("##fabilder", "###figuranim", 0, 0.2F, 0.9F));
    add(pause(0.4));
    add(pruefe("PF M8: Bilder nach dem Ziehen nach rechts mehr als vorher", [vorher] {
        const modell::FigurAnim a = figurAnimKopie("mb7_a");
        diag::detail("Knopftest: PF M8 Bilder - " + std::to_string(a.bilder) + " (vorher " + std::to_string(vorher->bilder) + ")");
        return a.bilder > vorher->bilder;
    }));
    return s;
}

// --- Spiel: derselbe Sprung von der Seite --------------------------------------------------------------------------------

std::vector<Schritt> spielSchritte() {
    std::vector<Schritt> s;
    const auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto fz = std::make_shared<fa7::Zustand>();
    fa7::aufbau(s, fz);
    add(tu("PH Spiel: a frei in die linke Haelfte (der Pfeiler der Testkarte stand neben ihr), b und c hinter die Mittelwand; Kamera von +Y",
           [fz] {
        const int ea = entityFuerFigur(fa7::figur("mb7_a"));
        const int eb = entityFuerFigur(fa7::figur("mb7_b"));
        const int ec = entityFuerFigur(fa7::figur("mb7_c"));
        if (ea < 0 || eb < 0 || ec < 0) {
            meldeFehler("PH Spiel: Spawner");
            return;
        }
        const float sp[3] = {fz->spieler[0], fz->spieler[1], fz->boden + 24.0F};
        const float oa[3] = {sp[0] - 76.0F, sp[1] - 228.0F, sp[2]};
        const float ob[3] = {sp[0] + 584.0F, sp[1] + 22.0F, sp[2]};
        const float oc[3] = {sp[0] + 644.0F, sp[1] + 22.0F, sp[2]};
        (void)entityKeySetzen(ea, "origin", fa7::v3(oa));
        (void)entityKeySetzen(eb, "origin", fa7::v3(ob));
        (void)entityKeySetzen(ec, "origin", fa7::v3(oc));
        // von +Y mit Blick -Y: der Sprung (+X der Karte) laeuft im Bild von rechts nach links
        const float kam[3] = {oa[0] + 40.0F, oa[1] + 340.0F, fz->boden + 70.0F};
        const TraceTreffer t = traceRay(g_app->geo, oa, kam);
        const std::string text = std::string("//Generated by BehavEd\n\ncamera ( ENABLE );\ncamera ( ZOOM, 90.000, 0.000 );\ncamera ( MOVE, < ") + fa7::v3(kam) +
                                 " >, 0.000 );\ncamera ( PAN, < 0.000 -90.000 0.000 >, < 0.000 0.000 0.000 >, 0.000 );\nwait ( 16000.000 );\ncamera ( DISABLE );\n";
        Script neu;
        std::vector<Diag> d;
        if (!readScript(text, neu, d)) {
            meldeFehler("PH Spiel: Skript");
            return;
        }
        g_app->doc.ersetzeSkript(std::move(neu), "mb7szene");
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
        diag::detail("Knopftest: PH Spiel - Kamera " + fa7::v3(kam) + ", a " + fa7::v3(oa) + ", freie Sicht " + std::to_string(!t.hit));
    }));
    add(warteBis("PH Spiel: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen() && fa7::figur("mb7_a") >= 0; }, 2000));
    sprungAufbauen(s);
    add(tu("PH Spiel: Physik an", [] { (void)figurAnimPhysikAn(true); }));
    add(warteBis("PH Spiel: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    add(tu("PH Spiel: bauen und Vorschau aufzeichnen", [fz] {
        prepareScene();
        const std::string o = mb::verbinde(fz->ordner, "ph");
        g_app->figAnim.ziel = 0;
        if (!figurAnimBauen(o, true)) { meldeFehler("PH Spiel: Bauen: " + g_app->figAnim.meldung); }
        std::string f;
        if (!missionSpeichernNach(mb::verbinde(o, "zz_mb7szene.pk3"), &f)) { meldeFehler("PH Spiel: Mission: " + f); }
        std::string text = "# t_ms schwerpunkt_x y z becken_x y z\n";
        for (int t = 0; t <= 5000; t += 50) {
            std::array<float, 3> c{};
            std::array<float, 3> b{};
            if (!comKarte("mb7_a", static_cast<double>(t), c, &b)) { continue; }
            char z2[200];
            std::snprintf(z2, sizeof z2, "%d %.3f %.3f %.3f %.3f %.3f %.3f\n", t, static_cast<double>(c[0]), static_cast<double>(c[1]), static_cast<double>(c[2]),
                          static_cast<double>(b[0]), static_cast<double>(b[1]), static_cast<double>(b[2]));
            text += z2;
        }
        const modell::PhysikBericht br = figurAnimPhysikBericht("mb7_a");
        if (!br.fluege.empty()) {
            text += "# flug " + std::to_string(br.fluege.front().first) + " " + std::to_string(br.fluege.front().second) + " start " +
                    std::to_string(figurAnimStartMs("mb7_a")) + " schwerkraft " + std::to_string(g_app->scene.schwerkraft) + "\n";
        }
        std::string fe;
        if (!sicherSchreiben(mb::verbinde(o, "ph_vorschau.txt"), text, &fe)) { meldeFehler("PH Spiel: " + fe); }
        diag::detail("Knopftest: PH Spiel - gebaut nach " + o + ", " + g_app->figAnim.meldung);
    }));
    for (int ms = 0; ms <= 4000; ms += 50) {
        add(tu("PH Spiel: Vorschau durch die Kamera bei " + std::to_string(ms) + " ms", [=] {
            g_app->figAnim.offen = false;
            g_app->throughCamera = true;
            g_app->playing = false;
            g_app->playMs = static_cast<double>(ms);
            g_app->mapDirty = true;
        }));
        add(pause(0.3));
        add(mission1::probeFoto("ph_vorschau_" + std::to_string(ms)));
    }
    add(tu("PH Spiel: Kamera wieder frei", [] { g_app->throughCamera = false; }));
    add(pruefe("PH Spiel: gebaut (GLA, Mission, Vorschau)", [fz] {
        const std::string o = mb::verbinde(fz->ordner, "ph");
        return !slurp(mb::verbinde(o, "models/players/_humanoid_mb7szene/_humanoid_mb7szene.gla")).empty() && !slurp(mb::verbinde(o, "zz_mb7szene.pk3")).empty() &&
               !slurp(mb::verbinde(o, "ph_vorschau.txt")).empty();
    }));
    return s;
}

// --- Bildzeit (allein, BHED_D3D_DEBUG=0): Physik an, drei Figuren, Kurveneditor offen -------------------------------------

std::vector<Schritt> zeitSchritte() {
    std::vector<Schritt> s;
    const auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto fz = std::make_shared<fa7::Zustand>();
    fa7::aufbau(s, fz);
    sprungAufbauen(s);
    for (const char* f : {"mb7_b", "mb7_c"}) {
        const std::string fig = f;
        add(tu("PH Zeit: " + fig + " springt ebenso", [fig] {
            g_app->playMs = 1000.0;
            (void)figurAnimStarten(fig);
            (void)figurAnimEinstellen([](modell::FigurAnim& a) { a.bilder = 50; });
            const float null[3] = {0.0F, 0.0F, 0.0F};
            const float oben[3] = {0.0F, -30.0F, 30.0F};
            const float vor[3] = {0.0F, -30.0F, -30.0F};
            (void)figurAnimBeckenWeg(fig, 0, null);
            (void)figurAnimBeckenWeg(fig, 10, null);
            (void)figurAnimBeckenWeg(fig, 20, oben);
            (void)figurAnimBeckenWeg(fig, 30, vor);
            (void)figurAnimBeckenWeg(fig, 49, null);
        }));
        add(warteBis("PH Zeit: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    }
    for (const bool physik : {false, true}) {
        add(tu(std::string("PH Zeit: Physik ") + (physik ? "an" : "aus") + " (drei Figuren)", [physik] {
            for (const char* f : {"mb7_a", "mb7_b", "mb7_c"}) {
                g_app->figAnim.figur = f;
                (void)figurAnimPhysikAn(physik);
            }
        }));
        add(warteBis("PH Zeit: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
        add(m1Messblock("Mission", std::string("drei Figuren springen, Physik ") + (physik ? "an" : "aus") + ", Kurven offen, Abspielen", [] {
            g_app->figAnim.figur = "mb7_a";
            g_app->figAnim.offen = true;
            g_app->figAnim.posieren = true;
            g_app->figAnim.kurvenZeigen = true;
            g_app->playMs = 1000.0;
            g_app->playing = true;
        }));
    }
    add(tu("PH Zeit: anhalten", [] { g_app->playing = false; }));
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    if (n == "ph") {
        std::vector<Schritt> s = modellSchritte(false);
        for (Schritt& x : missionSchritte(false)) { s.push_back(std::move(x)); }
        return s;
    }
    if (n == "phspiel") { return spielSchritte(); }
    if (n == "phzeit") { return zeitSchritte(); }
    return {};
}

// Die Bilder der Handbuchseite (BHED_HB_NUR=ph): Model-Tab mit dem Sprung, Fenster "Figur animieren" mit Physik
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s = modellSchritte(true);
    for (Schritt& x : missionSchritte(true)) { s.push_back(std::move(x)); }
    return s;
}

}  // namespace ph6
