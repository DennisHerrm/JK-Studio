// selbsttest_rt.inc - Modell-Block Stufe 5: Retargeting (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=rt | rtjka | rtspiel | rtzeit)
//
//   rt          Movie Duels, Sturmtruppler als Ziel; Quellen aus BHED_RT_GLA (die GLAs der Spiele als lose Dateien,
//               <dir>/<jka|jk2|md>/models/...):
//     Erreichbarkeit  jede Aktion der Gruppe Retargeting im Menue Animation (Untermenue) mit Tooltip, im Rechtsklick der
//                     Ansicht (Rig > Retargeting), in der Suche (X wie 3ds Max, F3 wie Blender), das Rollout geht auf
//     Vorlagen        jedes der 34 eigenen Skelette des Basisspiels (JKA) erkennt seine Vorlage (Datei + Knochen), JK2 und
//                     die MD-Figur ebenso; ein MD-Skelett ohne Vorlage bekommt vorgeschlagene Ketten
//     JK2 -> JKA      BOTH_WALK1 aus JK2 auf den Sturmtruppler: Ketten zugeordnet, Fussrutschen in Einheiten (alles an /
//                     nur FK / ohne Becken), k = 1 (gleiche Beine), Saebel-Tag an der Hand, Spruenge (ohne Kontakte keine),
//                     Vorschau = gepackte GLA, Quelle daneben
//                     gezeichnet, Rueckgaengig, Backen auf einen Platz mit den fps der animation.cfg
//     Rundlauf        Retargeting aus -> die eigene Sequenz gebacken Byte fuer Byte wie vorher
//     JKA -> MD       BOTH_RUN1 aus JKA auf die MD-Figur (Gegenprobe)
//   rtjka       Original-JKA: Wampa als Ziel (Kreatur, Vorlage): rot ohne Laengenausgleich rutschen die Fuesse, gruen nicht
//   rtspiel     Mission mb7raum: mb7_a spielt den uebertragenen JK2-Gang als Karten-GLA (SET_ANIM_BOTH BOTH_CIN_n),
//               gebaut nach BHED_FIGUR_BAU, Vorschau-Fotos durch die Skriptkamera fuer den Vergleich im Spiel
//   rtzeit      Bildzeit allein (BHED_D3D_DEBUG=0): Vorschau mit Quelle daneben abspielend gegen die eigene Animation
// Eingebunden in selbsttest.cpp (nach selbsttest_ph.inc).
namespace rt5 {

using kv5::f0;
using kv5::seq;
using rig3::imMenue;
using rig3::klickMenue;
using rig3::woIst;

std::string zahl(double v) {
    char b[40];
    std::snprintf(b, sizeof b, "%.4f", v);
    return b;
}

std::string glaOrdner() {
    const char* d = std::getenv("BHED_RT_GLA");
    return d != nullptr ? d : "";
}
std::string glaDatei(const std::string& spiel, const std::string& pfad) { return mb::verbinde(mb::verbinde(glaOrdner(), spiel), pfad); }
// Ohne BHED_RT_GLA (oder mit einem Ordner, den es nicht gibt) laufen die Gruppen nicht los: die Schritte danach greifen auf
// die Quelle und ihre Ergebnisse zu (r.welt[0], Ketten-Indizes) - ohne Quelle stuerzte der Selbsttest mit 0xC0000005 ab statt
// FEHLER-Zeilen zu schreiben (Altbestand Stufe 5, Zusatz p_animimport). Eine Zeile mit Meldung, sonst nichts.
bool glaOrdnerDa() {
    std::error_code ec;
    return !glaOrdner().empty() && std::filesystem::is_directory(glaOrdner(), ec);
}
std::vector<Schritt> ohneGlaOrdner(const std::string& n) {
    std::vector<Schritt> s;
    s.push_back(pruefe("RT " + n + ": BHED_RT_GLA zeigt auf die GLAs der Spiele (<dir>/<jka|jk2|md>/models/...)", [n] {
        meldeFehler("RT " + n + ": BHED_RT_GLA fehlt oder ist kein Ordner (\"" + glaOrdner() + "\") - Gruppe nicht gelaufen");
        return false;
    }));
    return s;
}

struct Zustand {
    std::string vorher;           // gebackene eigene Sequenz vor dem Retargeting (Rundlauf)
    std::size_t undo = 0;
    float rutschenVoll = -1.0F;
    float rutschenFk = -1.0F;
    float rutschenOhneBecken = -1.0F;
};

// Die eigene Sequenz der Figur gebacken (ohne Schicht) als Bytes
std::string eigeneGebacken() {
    GlaAnimation g;
    modell::BackBericht br;
    const modell::RigSchicht leer;
    if (!modell::rigBacken(g_app->anim, leer, seq().firstFrame, seq().numFrames, false, "x", g, &br)) { return {}; }
    std::string b;
    (void)schreibGla(g, b);
    return b;
}

// Retargeting rechnen mit Aenderungen an der Einstellung (ohne Rueckgaengig-Schritt), Bericht zurueck
modell::RtBericht rechne(const std::function<void(modell::RtEinstellung&)>& f) {
    const ModellRetarget& r = g_app->mtab.rt;
    modell::RtEinstellung e = g_app->mtab.dok.inhalt().rig.rt.e;
    f(e);
    const AnimEntry* q = nullptr;
    for (const AnimEntry& a : r.quelleCfg) {
        if (a.name == g_app->mtab.dok.inhalt().rig.rt.sequenz) { q = &a; }
    }
    std::vector<std::vector<BoneMatrix>> w;
    modell::RtBericht b;
    (void)modell::rtUebertragen(r.quelle, r.cq, g_app->anim, r.cz, g_app->mtab.dok.inhalt().rig.rt.zu, e, q != nullptr ? q->firstFrame : 0,
                                q != nullptr ? q->numFrames : r.quelle.bildZahl(), w, &b);
    return b;
}

// Die Spruenge mit Bild und Knochen
std::string sprungText(const modell::RtBericht& b) {
    std::string t;
    for (std::size_t i = 0; i < b.spruenge.size() && i < 8; ++i) {
        const int k = i < b.sprungKnochen.size() ? b.sprungKnochen[i] : -1;
        t += " " + std::to_string(b.spruenge[i]) + ":" + (k >= 0 && static_cast<std::size_t>(k) < g_app->anim.bones.size() ? g_app->anim.bones[static_cast<std::size_t>(k)].name : "?");
    }
    return t;
}

Schritt menueVoll(const std::string& wo) {
    return pruefe("RT " + wo + ": jede Aktion der Gruppe Retargeting steht da, mit Tooltip (keine Taste: weder 3ds Max noch Blender haben eine)", [wo] {
        int n = 0;
        for (int i = 0; i < modellRigAktionZahl(); ++i) {
            if (modellRigAktionGruppe(i) != 7) { continue; }
            const std::string text = modellRigAktionText(i);
            if (imMenue(text) == nullptr || std::string(modellRigAktionHinweis(i)).size() < 10U || modellRigAktionTaste(i, false)[0] != '\0' ||
                modellRigAktionTaste(i, true)[0] != '\0') {
                meldeFehler("RT: fehlt im Menue oder ohne Tooltip: " + text + " " + woIst(text));
                return false;
            }
            ++n;
        }
        diag::detail("Knopftest: RT " + wo + " - " + std::to_string(n) + " Aktionen mit Tooltip");
        return n >= 10;
    });
}

void ueberSuche(std::vector<Schritt>& s, Str text) {
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
    s.push_back(tu("RT: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; }));
    s.push_back(ik6::tasteSuche());
    s.push_back(pause(0.2));
    s.push_back(pruefe("RT Suche: Alle Aktionen suchen ist offen (X / F3)", [] { return g_app->mtab.aktionenSichtbar; }));
    const std::string zeile = tr(text);
    s.push_back(tippe(zeile));
    s.push_back(pause(0.2));
    s.push_back(klickAn("\"" + zeile + "\" in der Suche", [zeile] {
        for (const Element& e : g_elemente) {
            if ((e.label == zeile || e.label.rfind(zeile + " (", 0) == 0) && e.fenster.find("##Popup_") != std::string::npos) { return e.rect.GetCenter(); }
        }
        meldeFehler("RT: " + zeile + " nicht in der Suche: " + woIst(zeile));
        return ImVec2{};
    }));
    s.push_back(pause(0.3));
}

// Die Figur laden, Bereich Animation, eine Sequenz der Figur, wie 3ds Max
void figurLaden(std::vector<Schritt>& s, const std::string& pfad, const char* sequenz) {
    s.push_back(tu("RT: Skript fuer den Test", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "rt.txt"); }));
    for (Schritt& x : mb1::laden(pfad)) { s.push_back(std::move(x)); }
    const std::string sq = sequenz;
    s.push_back(tu("RT: Bereich Animation, " + sq + ", wie 3ds Max", [sq] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Animation;
        animWaehlen(std::max(mb1::animNach(sq), 0), false);
        g_app->animPlaying = false;
        g_app->animFrame = f0();
        g_app->animTime = 0.0F;
        g_app->mtab.rt.vorschau = false;
    }));
    s.push_back(pause(0.5));
}

Schritt quelle(const std::string& spiel, const std::string& pfad, const std::string& sequenz) {
    return tu("RT: Quelle " + spiel + " " + pfad + " " + sequenz, [=] {
        const std::string d = glaDatei(spiel, pfad);
        if (!modellRtQuelleLaden(d)) {
            meldeFehler("RT: Quelle nicht geladen: " + d + " (BHED_RT_GLA)");
            return;
        }
        (void)modellRtEinstellen(tr(Str::RtUndoQuelle), [&](modell::RetargetDaten& x) { x.sequenz = sequenz; });
        g_app->mtab.rt.stand = -1;
        modellRtAktualisieren();
    });
}

std::vector<Schritt> modellSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<Zustand>();
    figurLaden(s, "models/players/stormtrooper/model.glm", "BOTH_STAND1");
    add(tu("RT: eigene Sequenz gebacken (vor dem Retargeting)", [z] { z->vorher = eigeneGebacken(); }));
    // --- Erreichbarkeit ---
    add(taste(ImGuiKey_Escape, "Esc"));
    add(klickMenue(tr(Str::MbBereichAnimation), true));
    add(pause(0.3));
    add(klickMenue(tr(Str::RtRollout), false));
    add(pause(0.4));
    add(menueVoll("Menue Animation > Retargeting"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(klickAn("Rechtsklick in die Ansicht", [] { return kv5::fokusPunkt(); }, 1));
    add(pause(0.3));
    add(klickMenue(tr(Str::MbRgGruppeRig), false));
    add(pause(0.3));
    add(klickMenue(tr(Str::RtRollout), false));
    add(pause(0.4));
    add(menueVoll("Rechtsklick > Rig > Retargeting"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(tu("RT: Rollout zu", [] { g_app->mtab.rt.rolloutBild = -1; }));
    ueberSuche(s, Str::RtRolloutOeffnen);
    add(pause(0.3));
    add(pruefe("RT: X > Retargeting oeffnen klappt das Rollout auf", [] { return g_app->mtab.rt.rolloutBild >= ImGui::GetFrameCount() - 3; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    // --- Vorlagen der Spiele ---
    add(pruefe("RT Vorlagen: jedes eigene Skelett des Basisspiels (34) erkennt seine Vorlage, Ketten und fScale", [] {
        const std::vector<modell::RtVorlageInfo> v = modell::rtVorlagen();
        int gefunden = 0;
        int geprueft = 0;
        std::string fehlt;
        std::error_code ec;
        const std::filesystem::path w = std::filesystem::path(glaDatei("jka", ""));
        for (auto it = std::filesystem::recursive_directory_iterator(w, ec); it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            if (ec || !it->is_regular_file() || it->path().extension() != ".gla") { continue; }
            const std::string rel = std::filesystem::relative(it->path(), w, ec).generic_string();
            if (rel.find("players/_humanoid_") != std::string::npos) { continue; }   // die Karten-GLAs: Skelett _humanoid
            GlaAnimation g;
            if (!readGla(slurp(it->path().string()), g)) { continue; }
            ++geprueft;
            const int i = modell::rtVorlageFuer(g, rel);
            const bool ok = i >= 0 && v[static_cast<std::size_t>(i)].datei == rel && std::fabs(v[static_cast<std::size_t>(i)].massstab - g.scale) < 1e-4F;
            if (ok) {
                ++gefunden;
            } else {
                fehlt += " " + rel;
            }
        }
        diag::detail("Knopftest: RT Vorlagen - " + std::to_string(gefunden) + " von " + std::to_string(geprueft) + " erkannt" + fehlt + "; " +
                     std::to_string(v.size()) + " Vorlagen");
        return geprueft == 34 && gefunden == 34;
    }));
    add(pruefe("RT Vorlagen: JK2 und MD _humanoid; ein MD-Skelett ohne Vorlage (rancor_ice_spider) bekommt vorgeschlagene Ketten", [] {
        GlaAnimation j2;
        GlaAnimation md;
        GlaAnimation sp;
        const bool a = readGla(slurp(glaDatei("jk2", "models/players/_humanoid/_humanoid.gla")), j2);
        const bool b = readGla(slurp(glaDatei("md", "models/players/_humanoid/_humanoid.gla")), md);
        const bool c = readGla(slurp(glaDatei("md", "models/players/rancor_ice_spider/rancor_ice_spider.gla")), sp);
        const modell::RtCharakter cj = modell::rtCharakter(j2, "models/players/_humanoid/_humanoid.gla");
        const modell::RtCharakter cm = modell::rtCharakter(md, "models/players/_humanoid/_humanoid.gla");
        const modell::RtCharakter cs = modell::rtCharakter(sp, "models/players/rancor_ice_spider/rancor_ice_spider.gla");
        int glieder = 0;
        for (const modell::RtKette& k : cs.ketten) { glieder += (k.art == modell::KettenArt::Bein || k.art == modell::KettenArt::Arm) ? 1 : 0; }
        diag::detail("Knopftest: RT Vorlagen - JK2 " + cj.vorlage + ", MD " + cm.vorlage + ", rancor_ice_spider " +
                     (cs.vorlage.empty() ? "vorgeschlagen" : cs.vorlage) + " " + std::to_string(cs.ketten.size()) + " Ketten, " + std::to_string(glieder) +
                     " Glieder, Becken " + (cs.becken >= 0 ? sp.bones[static_cast<std::size_t>(cs.becken)].name : std::string("-")));
        return a && b && c && cj.vorlage == "_humanoid (JK2)" && cm.vorlage == "_humanoid (JKA)" && cs.vorlage.empty() && glieder >= 2;
    }));
    // --- JK2 -> JKA (MD-Figur) ---
    add(quelle("jk2", "models/players/_humanoid/_humanoid.gla", "BOTH_WALK1"));
    add(pruefe("RT JK2 -> MD: Vorlagen JK2/JKA, alle Ketten ausser den Rockschoessen zugeordnet", [] {
        const ModellRetarget& r = g_app->mtab.rt;
        const modell::RtZuordnung& zu = g_app->mtab.dok.inhalt().rig.rt.zu;
        int voll = 0;
        std::string fehlt;
        for (std::size_t i = 0; i < zu.quelle.size(); ++i) {
            voll += zu.quelle[i] >= 0 ? 1 : 0;
            if (zu.quelle[i] < 0) { fehlt += " " + r.cz.ketten[i].schluessel(); }
        }
        diag::detail("Knopftest: RT JK2 - Quelle " + r.cq.vorlage + ", Ziel " + r.cz.vorlage + ", " + std::to_string(voll) + " von " +
                     std::to_string(r.cz.ketten.size()) + " Ketten (ohne:" + fehlt + "), " + std::to_string(r.welt.size()) + " Bilder, " + zahl(r.ms) + " ms");
        return r.cq.vorlage == "_humanoid (JK2)" && r.cz.vorlage == "_humanoid (JKA)" && voll == static_cast<int>(r.cz.ketten.size()) - 2 && !r.welt.empty();
    }));
    add(tu("RT JK2: Uebertragen ueber das Menue (Vorschau an), Rollout auf", [] {
        for (int i = 0; i < modellRigAktionZahl(); ++i) {
            if (std::string(modellRigAktionText(i)) == tr(Str::RtUebertragen)) { modellRigAktionAusloesen(i); }
        }
    }));
    add(pause(0.6));
    add(pruefe("RT JK2: Fussrutschen in Einheiten (alles an / nur FK / ohne Becken), Spruenge, Bein-Verhaeltnis", [z] {
        const modell::RtBericht voll = g_app->mtab.rt.bericht;
        const modell::RtBericht fk = rechne([](modell::RtEinstellung& e) { e.ik = false; });
        const modell::RtBericht ohne = rechne([](modell::RtEinstellung& e) {
            e.ik = false;
            e.becken = false;
        });
        const modell::RtBericht ohneKontakt = rechne([](modell::RtEinstellung& e) { e.kontakte = false; });
        diag::detail("Knopftest: RT JK2 -> MD Spruenge - ohne Kontakte " + std::to_string(ohneKontakt.spruenge.size()) + sprungText(ohneKontakt) + ", nur FK " +
                     std::to_string(fk.spruenge.size()) + sprungText(fk) + ", Fussrutschen ohne Kontakte " + zahl(ohneKontakt.rutschenMax));
        z->rutschenVoll = voll.rutschenMax;
        z->rutschenFk = fk.rutschenMax;
        z->rutschenOhneBecken = ohne.rutschenMax;
        diag::detail("Knopftest: RT JK2 -> MD Fussrutschen - alles an " + zahl(voll.rutschenMax) + ", nur FK " + zahl(fk.rutschenMax) + ", ohne Becken " +
                     zahl(ohne.rutschenMax) + ", Quelle skaliert " + zahl(voll.rutschenQuelle) + "; k " + zahl(voll.k) + " (Beine " + zahl(voll.beinZiel) +
                     " / " + zahl(voll.beinQuelle) + "); Spruenge " + std::to_string(voll.spruenge.size()) + sprungText(voll) + "; Posen " +
                     zahl(voll.poseAbweichung) + " Grad");
        // Spruenge: ohne Kontakte und nur mit FK keine; mit Kontakten zieht die Fuss-IK den Fuss bis zur Abweichung der Quelle von
        // ihrer gleichmaessigen Bewegung (1,45) zurueck - am fast gestreckten Knie dreht das Hueft/Knie in einem Bild, die Pruefung
        // markiert diese Bilder (gewollt: der Anwender sieht sie)
        return !voll.bilder.empty() && voll.rutschenMax <= voll.rutschenQuelle + 0.05F && ohneKontakt.spruenge.empty() && fk.spruenge.empty() &&
               std::fabs(voll.k - 1.0F) < 1e-3F;
    }));
    add(pruefe("RT JK2: Saebel-Tag rhang_tag_bone bleibt an rhand (Abstand je Bild wie in der Pose)", [] {
        const ModellRetarget& r = g_app->mtab.rt;
        const int h = boneIndex(g_app->anim, "rhand");
        const int t = boneIndex(g_app->anim, "rhang_tag_bone");
        if (h < 0 || t < 0 || r.welt.empty()) { return false; }
        float abw = 0.0F;
        const BoneMatrix rel0 = modell::starrInvers(r.welt[0][static_cast<std::size_t>(h)]) * r.welt[0][static_cast<std::size_t>(t)];
        for (const auto& w : r.welt) {
            const BoneMatrix rel = modell::starrInvers(w[static_cast<std::size_t>(h)]) * w[static_cast<std::size_t>(t)];
            abw = std::max(abw, std::hypot(rel.m[0][3] - rel0.m[0][3], rel.m[1][3] - rel0.m[1][3], rel.m[2][3] - rel0.m[2][3]));
        }
        diag::detail("Knopftest: RT JK2 Saebel-Tag - Abweichung " + zahl(abw));
        return abw < 0.01F;
    }));
    add(pruefe("RT JK2: die Figur zeigt die gepackte GLA (Vorschau), die Quelle daneben ist gezeichnet", [] {
        const ModellRetarget& r = g_app->mtab.rt;
        if (!r.vorschau || r.welt.empty() || g_app->modellWelt.size() != r.welt[0].size()) { return false; }
        const auto b = static_cast<std::size_t>(std::clamp(static_cast<int>(r.bild), 0, static_cast<int>(r.welt.size()) - 1));
        float d = 0.0F;
        for (std::size_t k = 0; k < r.welt[b].size(); ++k) {
            d = std::max(d, std::hypot(g_app->modellWelt[k].m[0][3] - r.welt[b][k].m[0][3], g_app->modellWelt[k].m[1][3] - r.welt[b][k].m[1][3],
                                       g_app->modellWelt[k].m[2][3] - r.welt[b][k].m[2][3]));
        }
        diag::detail("Knopftest: RT Vorschau - Bild " + std::to_string(b) + ", gepackt gegen gerechnet " + zahl(d) + ", Quelle " + std::to_string(r.gezeichnet) +
                     " Knochen gezeichnet");
        // gepackt: Ort 1/64 je Achse und Glied (compressBone)
        return d < 0.3F && d > 0.0F && r.gezeichnet >= 40;
    }));
    add(tu("RT JK2: Abspielen", [] { g_app->mtab.rt.spielt = true; }));
    add(pause(1.0));
    add(pruefe("RT JK2: das Abspielen laeuft (Bild > 0)", [] {
        diag::detail("Knopftest: RT Abspielen - Bild " + zahl(g_app->mtab.rt.bild));
        return g_app->mtab.rt.bild > 1.0F;
    }));
    add(tu("RT JK2: anhalten; Kette arm_l von Hand auf arm_r", [z] {
        g_app->mtab.rt.spielt = false;
        z->undo = g_app->mtab.dok.stand();
        const ModellRetarget& r = g_app->mtab.rt;
        int zi = -1;
        int qi = -1;
        for (std::size_t i = 0; i < r.cz.ketten.size(); ++i) {
            if (r.cz.ketten[i].schluessel() == "arm_l_0") { zi = static_cast<int>(i); }
        }
        for (std::size_t i = 0; i < r.cq.ketten.size(); ++i) {
            if (r.cq.ketten[i].schluessel() == "arm_r_0") { qi = static_cast<int>(i); }
        }
        (void)modellRtEinstellen(tr(Str::RtUndoZuordnung), [&](modell::RetargetDaten& x) {
            x.zu.quelle[static_cast<std::size_t>(zi)] = qi;
            x.zu.vonHand[static_cast<std::size_t>(zi)] = 1;
        });
    }));
    add(pause(0.3));
    add(pruefe("RT JK2: von Hand zugeordnet, Leere zuordnen laesst sie stehen", [] {
        (void)modellRtZuordnen(false);
        const modell::RtZuordnung& zu = g_app->mtab.dok.inhalt().rig.rt.zu;
        int hand = 0;
        for (const std::uint8_t v : zu.vonHand) { hand += v; }
        return hand == 1;
    }));
    add(tu("RT: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pruefe("RT JK2: Strg+Z nimmt Zuordnen und die Kette von Hand zurueck", [] {
        const modell::RtZuordnung& zu = g_app->mtab.dok.inhalt().rig.rt.zu;
        int hand = 0;
        for (const std::uint8_t v : zu.vonHand) { hand += v; }
        return hand == 0;
    }));
    add(tu("RT JK2: Plotten auf einen Platz (Karten-GLA)", [] {
        modellGlaZielSetzen(0);
        (void)modellRtBacken();
    }));
    add(pruefe("RT JK2: auf dem Platz liegen die Bilder der Quelle mit den fps ihrer animation.cfg, Skelett der Figur", [] {
        const ModellTab& t = g_app->mtab;
        const ModellRetarget& r = t.rt;
        int fps = -1;
        int bilder = -1;
        for (const AnimEntry& e : r.quelleCfg) {
            if (e.name == "BOTH_WALK1") {
                fps = e.fps;
                bilder = e.numFrames;
            }
        }
        for (const modell::Platz& p : t.plaetze) {
            if (!p.belegt || p.quelle.gla == nullptr || p.quelle.glaName != "BOTH_WALK1") { continue; }
            diag::detail("Knopftest: RT Platz " + p.name + " - " + std::to_string(p.quelle.gla->numFrames) + " Bilder, " + std::to_string(p.quelle.sequenz.fps) +
                         " fps (Quelle " + std::to_string(bilder) + " / " + std::to_string(fps) + "), " + std::to_string(p.quelle.gla->bones.size()) + " Knochen");
            return p.quelle.gla->numFrames == bilder && p.quelle.sequenz.fps == fps && p.quelle.gla->bones.size() == g_app->anim.bones.size();
        }
        return false;
    }));
    // --- Gegenprobe JKA -> MD ---
    add(quelle("jka", "models/players/_humanoid/_humanoid.gla", "BOTH_RUN1"));
    add(pruefe("RT JKA -> MD (BOTH_RUN1, Gegenprobe): Fussrutschen und Spruenge", [] {
        const modell::RtBericht& b = g_app->mtab.rt.bericht;
        diag::detail("Knopftest: RT JKA -> MD - Fussrutschen " + zahl(b.rutschenMax) + " (Quelle skaliert " + zahl(b.rutschenQuelle) + "), k " + zahl(b.k) +
                     ", Spruenge " + std::to_string(b.spruenge.size()) + ", " + std::to_string(b.kettenZugeordnet) + " Ketten");
        return b.fehler.empty() && b.rutschenMax <= b.rutschenQuelle + 0.05F && b.spruenge.empty();
    }));
    add(tu("RT: Vorschau aus", [] { (void)modellRtVorschau(false); }));
    add(pause(0.3));
    add(pruefe("RT Rundlauf: Retargeting aus -> die eigene Sequenz gebacken Byte fuer Byte wie vorher", [z] {
        const std::string jetzt = eigeneGebacken();
        diag::detail("Knopftest: RT Rundlauf - " + std::to_string(z->vorher.size()) + " / " + std::to_string(jetzt.size()) + " Bytes");
        return !jetzt.empty() && jetzt == z->vorher;
    }));
    // --- wie Blender ---
    add(tu("RT: wie Blender, Rollout zu", [] {
        modellBedienungSetzen(1);
        g_app->mtab.rt.rolloutBild = -1;
    }));
    add(pause(0.3));
    ueberSuche(s, Str::RtRolloutOeffnen);
    add(pause(0.3));
    add(pruefe("RT wie Blender: F3 > Retargeting oeffnen klappt das Rollout auf", [] { return g_app->mtab.rt.rolloutBild >= ImGui::GetFrameCount() - 3; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(klickMenue(tr(Str::MbBereichAnimation), true));
    add(pause(0.3));
    add(klickMenue(tr(Str::RtRollout), false));
    add(pause(0.4));
    add(menueVoll("wie Blender: Menue Animation > Retargeting"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(tu("RT: wieder wie 3ds Max", [] { modellBedienungSetzen(0); }));
    return s;
}

// Original-JKA: eine Kreatur mit Vorlage als Ziel (Wampa), Quelle der JKA-Gang
std::vector<Schritt> jkaSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("RT JKA: Spiel jka", [] {
        mb1::spielSetzen("jka");
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
    }));
    add(pause(0.5));
    figurLaden(s, "models/players/wampa/model.glm", "BOTH_STAND1");
    add(quelle("jka", "models/players/_humanoid/_humanoid.gla", "BOTH_WALK1"));
    add(pruefe("RT JKA -> Wampa: Vorlage, Ketten; rot ohne Laengenausgleich rutschen die Fuesse, gruen nicht", [] {
        const ModellRetarget& r = g_app->mtab.rt;
        const modell::RtBericht voll = r.bericht;
        const modell::RtBericht ohne = rechne([](modell::RtEinstellung& e) {
            e.becken = false;
            e.ik = false;
        });
        const modell::RtBericht fk = rechne([](modell::RtEinstellung& e) { e.ik = false; });
        diag::detail("Knopftest: RT JKA -> Wampa - Ziel " + r.cz.vorlage + ", " + std::to_string(voll.kettenZugeordnet) + " Ketten, k " + zahl(voll.k) + " (Beine " +
                     zahl(voll.beinZiel) + " / " + zahl(voll.beinQuelle) + "); Fussrutschen ohne Becken " + zahl(ohne.rutschenMax) + ", nur FK " + zahl(fk.rutschenMax) +
                     ", alles an " + zahl(voll.rutschenMax) + ", Quelle skaliert " + zahl(voll.rutschenQuelle) + ", Spruenge " + std::to_string(voll.spruenge.size()) + sprungText(voll));
        return r.cz.vorlage == "wampa" && ohne.rutschenMax > voll.rutschenMax + 1.0F && voll.rutschenMax <= voll.rutschenQuelle + 0.05F;
    }));
    add(tu("RT JKA: Vorschau, Spiel md", [] {
        (void)modellRtVorschau(true);
        mb1::spielSetzen("md");
    }));
    return s;
}

std::vector<Schritt> zeitSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    figurLaden(s, "models/players/stormtrooper/model.glm", "BOTH_WALK1");
    add(m1Messblock("Modell", "eigene Animation abspielend (ohne Retargeting)", [] {
        g_app->mtab.rt.vorschau = false;
        g_app->animPlaying = true;
    }));
    add(quelle("jk2", "models/players/_humanoid/_humanoid.gla", "BOTH_WALK1"));
    add(m1Messblock("Modell", "Retargeting: Vorschau abspielend, Quelle daneben, Rollout offen", [] {
        g_app->animPlaying = false;
        (void)modellRtVorschau(true);
        g_app->mtab.rt.quelleZeigen = true;
        g_app->mtab.rt.oeffnen = true;
        g_app->mtab.rt.spielt = true;
    }));
    add(tu("RT Zeit: eine Rechnung (Einstellung geaendert)", [] {
        g_app->mtab.rt.stand = -1;
        modellRtAktualisieren();
        diag::detail("Knopftest: RT Zeit - eine Rechnung " + zahl(g_app->mtab.rt.ms) + " ms fuer " + std::to_string(g_app->mtab.rt.welt.size()) + " Bilder");
    }));
    add(tu("RT Zeit: anhalten", [] {
        g_app->mtab.rt.spielt = false;
        (void)modellRtVorschau(false);
    }));
    return s;
}

std::vector<Schritt> spielSchritte();   // unten

// Stufe 5b: Presets (BHED_MODELL_NUR=pr) - Erreichbarkeit, Messen an BOTH_WALK1/BOTH_RUN1, jede Haltung auf der Figur (Fuss am
// Boden, gleichmaessig, nahtlos), Rueckgaengig, Backen auf den Platz BOTH_WALK1 der eigenen GLA
std::vector<Schritt> presetSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    figurLaden(s, "models/players/stormtrooper/model.glm", "BOTH_STAND1");
    add(taste(ImGuiKey_Escape, "Esc"));
    add(klickMenue(tr(Str::MbBereichAnimation), true));
    add(pause(0.3));
    add(klickMenue(tr(Str::PrRollout), false));
    add(pause(0.4));
    add(pruefe("PR Menue Animation > Presets: jede Aktion mit Tooltip", [] {
        int n = 0;
        for (int i = 0; i < modellRigAktionZahl(); ++i) {
            if (modellRigAktionGruppe(i) != 8) { continue; }
            const std::string text = modellRigAktionText(i);
            if (imMenue(text) == nullptr || std::string(modellRigAktionHinweis(i)).size() < 10U) {
                meldeFehler("PR: fehlt im Menue oder ohne Tooltip: " + text + " " + woIst(text));
                return false;
            }
            ++n;
        }
        return n == 4;   // Presets oeffnen, Auf Figur legen, Messen; dazu Preset-Bibliothek oeffnen (Animations-Import)
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(tu("PR: Rollout zu", [] { g_app->mtab.rt.presetRolloutBild = -1; }));
    ueberSuche(s, Str::PrRolloutOeffnen);
    add(pause(0.3));
    add(pruefe("PR: X > Presets oeffnen klappt das Rollout auf", [] { return g_app->mtab.rt.presetRolloutBild >= ImGui::GetFrameCount() - 3; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    for (const modell::GangArt art : {modell::GangArt::Gehen, modell::GangArt::Laufen}) {
        const std::string an = art == modell::GangArt::Laufen ? "Laufen" : "Gehen";
        add(tu("PR " + an + ": Kategorie, an der Figur gemessen", [art] {
            (void)modellRtEinstellen(tr(Str::PrUndo), [art](modell::RetargetDaten& x) { x.gang.art = art; });
            (void)modellPresetMessen();
            diag::detail("Knopftest: PR Messgrund - " + std::string(art == modell::GangArt::Laufen ? "BOTH_RUN1" : "BOTH_WALK1") + " der Figur: Tempo " +
                         zahl(g_app->mtab.dok.inhalt().rig.rt.gang.tempo) + ", Schritt " + zahl(g_app->mtab.dok.inhalt().rig.rt.gang.schritt));
        }));
        for (int h = 0; h < modell::kGangHaltungen; ++h) {
            add(tu("PR " + an + ": Haltung " + std::to_string(h) + " auf die Figur", [h] {
                (void)modellRtEinstellen(tr(Str::PrUndo), [h](modell::RetargetDaten& x) { x.gang.haltung = static_cast<modell::GangHaltung>(h); });
                (void)modellPresetAuflegen();
            }));
            add(pause(0.2));
            add(pruefe("PR " + an + " Haltung " + std::to_string(h) + ": Fuss im Stand am Boden, gleichmaessig, Zyklus nahtlos, die Figur zeigt ihn", [h] {
                const ModellRetarget& r = g_app->mtab.rt;
                diag::detail("Knopftest: PR Haltung " + std::to_string(h) + " - " + std::to_string(r.welt.size()) + " Bilder, Boden " + zahl(r.gang.bodenAbw) +
                             ", Gleiten " + zahl(r.gang.gleitAbw) + ", Naht " + zahl(r.gang.naht) + ", Standbilder " + std::to_string(r.gang.standBilder) + ", " +
                             zahl(r.ms) + " ms");
                return !r.welt.empty() && r.vorschau && r.gang.bodenAbw < 0.01F && r.gang.gleitAbw < 0.01F && r.gang.naht < 1e-3F && r.gang.standBilder > 0;
            }));
        }
    }
    add(tu("PR: Fokus, Haltung schwer", [] {
        g_app->mtab.fokus = true;
        (void)modellRtEinstellen(tr(Str::PrUndo), [](modell::RetargetDaten& x) { x.gang.haltung = modell::GangHaltung::Schwer; });
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pruefe("PR: Strg+Z nimmt die Haltung zurueck (Saebel)", [] { return g_app->mtab.dok.inhalt().rig.rt.gang.haltung == modell::GangHaltung::Saebel; }));
    add(tu("PR: Backen auf BOTH_RUN1 der eigenen GLA", [] {
        modellGlaZielSetzen(2);
        (void)modellRtEinstellen(tr(Str::PrUndo), [](modell::RetargetDaten& x) { x.platz.clear(); });
        (void)modellRtBacken();
    }));
    add(pruefe("PR: auf dem Platz BOTH_RUN1 liegt der Zyklus (Bilder, fps der Zeile, Schleife ab 0)", [] {
        for (const modell::Platz& p : g_app->mtab.plaetze) {
            if (!p.belegt || p.name != "BOTH_RUN1" || p.quelle.gla == nullptr) { continue; }
            diag::detail("Knopftest: PR Platz - " + std::to_string(p.quelle.gla->numFrames) + " Bilder, " + std::to_string(p.quelle.sequenz.fps) + " fps, Schleife " +
                         std::to_string(p.quelle.sequenz.loopFrame));
            return p.quelle.gla->numFrames == static_cast<int>(g_app->mtab.rt.welt.size()) && p.quelle.sequenz.loopFrame == 0;
        }
        return false;
    }));
    add(tu("PR: Vorschau aus, Karten-GLA", [] {
        (void)modellRtVorschau(false);
        modellGlaZielSetzen(0);
    }));
    return s;
}

// Die Bilder der Handbuchseite (BHED_HB_NUR=rt): der JK2-Gang auf dem Sturmtruppler mit der Quelle daneben; Ketten, Pose, Schritte
std::vector<Schritt> handbuchSchritte() {
    if (!glaOrdnerDa()) { return ohneGlaOrdner("Handbuch"); }
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    figurLaden(s, "models/players/stormtrooper/model.glm", "BOTH_STAND1");
    add(quelle("jk2", "models/players/_humanoid/_humanoid.gla", "BOTH_WALK1"));
    add(tu("RT Handbuch: uebertragen, Bild 8, Ansicht von vorn-seitlich mit der Quelle daneben", [] {
        (void)modellRtUebertragen();
        g_app->mtab.rt.bild = 8.0F;
        g_app->mtab.rt.spielt = false;
        g_app->mtab.rt.quelleZeigen = true;
        ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        a.art = AnsichtArt::Links;   // von der Seite: die Beine im Schritt
        modellAnsichtEinpassenAktiv();
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(tu("RT Handbuch: Figur und Quelle zusammen ins Bild", [] {
        ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        for (int k = 0; k < 3; ++k) { a.mitte[k] += g_app->mtab.rt.versatz[k] * 0.5F; }
        a.halbHoehe *= 1.35F;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(rig3::rollenZu(tr(Str::AzRUebertragen), "mtrollouts"));   // AP10: die Gruppe heisst wie im Entwurf (bis rc611 "Retargeting")
    add(tu("RT Handbuch: Maus in die Ansicht (kein Tooltip im Bild)", [] { setzeMaus(kv5::fokusPunkt()); }));
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_rt_uebertragen"));
    add(tu("RT Handbuch: Ketten, Pose und Schritte aufgeklappt", [] { g_app->mtab.rt.baeumeOeffnen = true; }));
    add(pause(0.5));
    add(rig3::rollenZu(tr(Str::RtKettenMax), "mtrollouts"));
    add(tu("RT Handbuch: Maus in die Ansicht", [] { setzeMaus(kv5::fokusPunkt()); }));
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_rt_ketten"));
    // Stufe 5b: ein Gang aus dem Generator (Haltung Saebel), Rollout Presets offen
    add(tu("RT Handbuch: Presets - Gehen, Saebel gezogen, auf die Figur, Bild 6", [] {
        (void)modellRtEinstellen(tr(Str::PrUndo), [](modell::RetargetDaten& x) {
            x.gang.art = modell::GangArt::Gehen;
            x.gang.haltung = modell::GangHaltung::Saebel;
        });
        (void)modellPresetMessen();
        (void)modellPresetAuflegen();
        g_app->mtab.rt.spielt = false;
        g_app->mtab.rt.bild = 6.0F;
        g_app->mtab.rt.oeffnen = false;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(tu("RT Handbuch: ohne Quelle daneben - die Figur allein eingepasst", [] {
        modellAnsichtEinpassenAktiv();
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(rig3::rollenZu(tr(Str::PrRollout), "mtrollouts"));
    add(tu("RT Handbuch: Maus in die Ansicht", [] { setzeMaus(kv5::fokusPunkt()); }));
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_pr_gang"));
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    if ((n == "rt" || n == "rtjka" || n == "rtspiel" || n == "rtzeit") && !glaOrdnerDa()) { return ohneGlaOrdner(n); }
    if (n == "rt") { return modellSchritte(); }
    if (n == "rtjka") { return jkaSchritte(); }
    if (n == "rtspiel") { return spielSchritte(); }
    if (n == "rtzeit") { return zeitSchritte(); }
    if (n == "pr") { return presetSchritte(); }
    return {};
}

// --- Im Spiel: mb7_a spielt den uebertragenen JK2-Gang (Karten-GLA) ------------------------------------------------------
std::vector<Schritt> spielSchritte() {
    std::vector<Schritt> s;
    const auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto fz = std::make_shared<fa7::Zustand>();
    fa7::aufbau(s, fz);
    add(tu("RT Spiel: a frei, Kamera von der Seite; Spielfenster 1920 x 1440", [fz] {
        g_app->settings.spielSeiten = 1920.0F / 1440.0F;
        const int ea = entityFuerFigur(fa7::figur("mb7_a"));
        const int eb = entityFuerFigur(fa7::figur("mb7_b"));
        const int ec = entityFuerFigur(fa7::figur("mb7_c"));
        if (ea < 0 || eb < 0 || ec < 0) {
            meldeFehler("RT Spiel: Spawner");
            return;
        }
        const float sp[3] = {fz->spieler[0], fz->spieler[1], fz->boden + 24.0F};
        const float oa[3] = {sp[0] - 76.0F, sp[1] - 228.0F, sp[2]};
        const float ob[3] = {sp[0] + 584.0F, sp[1] + 22.0F, sp[2]};
        const float oc[3] = {sp[0] + 644.0F, sp[1] + 22.0F, sp[2]};
        (void)entityKeySetzen(ea, "origin", fa7::v3(oa));
        (void)entityKeySetzen(eb, "origin", fa7::v3(ob));
        (void)entityKeySetzen(ec, "origin", fa7::v3(oc));
        // von +Y mit Blick -Y, nah genug fuer eine grosse Figur
        const float kam[3] = {oa[0], oa[1] + 150.0F, fz->boden + 50.0F};
        const std::string text = std::string("//Generated by BehavEd\n\ncamera ( ENABLE );\ncamera ( ZOOM, 80.000, 0.000 );\ncamera ( MOVE, < ") + fa7::v3(kam) +
                                 " >, 0.000 );\ncamera ( PAN, < 0.000 -90.000 0.000 >, < 0.000 0.000 0.000 >, 0.000 );\nwait ( 16000.000 );\ncamera ( DISABLE );\n";
        Script neu;
        std::vector<Diag> d;
        if (!readScript(text, neu, d)) {
            meldeFehler("RT Spiel: Skript");
            return;
        }
        g_app->doc.ersetzeSkript(std::move(neu), "mb7szene");
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
        diag::detail("Knopftest: RT Spiel - Kamera " + fa7::v3(kam) + ", a " + fa7::v3(oa));
    }));
    add(warteBis("RT Spiel: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen() && fa7::figur("mb7_a") >= 0; }, 2000));
    add(tu("RT Spiel: mb7_a bekommt den JK2-Gang, uebertragen auf ihr Skelett, als Schluessel (Karten-GLA beim Bauen)", [] {
        GlaAnimation q;
        if (!readGla(slurp(glaDatei("jk2", "models/players/_humanoid/_humanoid.gla")), q)) {
            meldeFehler("RT Spiel: JK2-Quelle (BHED_RT_GLA)");
            return;
        }
        const std::vector<AnimEntry> cfg = parseAnimationCfg(slurp(glaDatei("jk2", "models/players/_humanoid/animation.cfg")));
        const AnimEntry* walk = nullptr;
        for (const AnimEntry& e : cfg) {
            if (e.name == "BOTH_WALK1") { walk = &e; }
        }
        const int ai = fa7::figur("mb7_a");
        std::vector<BoneMatrix> w;
        float fw[16];
        const GlaAnimation* g = nullptr;
        if (walk == nullptr || !fa7::gezeichnet("mb7_a", 0.0, w, fw, &g) || g == nullptr || ai < 0) {
            meldeFehler("RT Spiel: BOTH_WALK1 / Skelett von mb7_a");
            return;
        }
        const modell::RtCharakter cq = modell::rtCharakter(q, "models/players/_humanoid/_humanoid.gla");
        const modell::RtCharakter cz = modell::rtCharakter(*g, g->skeletonName + ".gla");
        const modell::RtZuordnung zu = modell::rtZuordnen(cq, cz);
        std::vector<std::vector<BoneMatrix>> welt;
        modell::RtBericht br;
        // zwei Durchgaenge des Gangs
        if (!modell::rtUebertragen(q, cq, *g, cz, zu, modell::RtEinstellung{}, walk->firstFrame, walk->numFrames, welt, &br)) {
            meldeFehler("RT Spiel: Uebertragen " + br.fehler);
            return;
        }
        std::vector<std::vector<BoneMatrix>> zwei = welt;
        zwei.insert(zwei.end(), welt.begin(), welt.end());
        const modell::RigSchicht schicht = modell::rtAlsSchicht(*g, zwei);
        (void)figurAnimStarten("mb7_a");
        const int fps = walk->fps;
        const int n = static_cast<int>(zwei.size());
        (void)figurAnimEinstellen([&](modell::FigurAnim& a) {
            a.bilder = n;
            a.fps = fps;
            a.setOrigin = false;
            a.schicht = schicht;
        });
        diag::detail("Knopftest: RT Spiel - " + std::to_string(n) + " Bilder mit " + std::to_string(fps) + " fps, k " + zahl(br.k) + ", Fussrutschen " +
                     zahl(br.rutschenMax) + ", Ziel " + cz.vorlage + " (" + g->skeletonName + ")");
    }));
    add(warteBis("RT Spiel: Szene", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1200));
    add(tu("RT Spiel: bauen und Vorschau aufzeichnen", [fz] {
        prepareScene();
        const std::string o = mb::verbinde(fz->ordner, "rt");
        g_app->figAnim.ziel = 0;
        if (!figurAnimBauen(o, true)) { meldeFehler("RT Spiel: Bauen: " + g_app->figAnim.meldung); }
        std::string f;
        if (!missionSpeichernNach(mb::verbinde(o, "zz_mb7szene.pk3"), &f)) { meldeFehler("RT Spiel: Mission: " + f); }
        diag::detail("Knopftest: RT Spiel - gebaut nach " + o + ", " + g_app->figAnim.meldung);
    }));
    for (int ms = 0; ms <= 4000; ms += 250) {
        add(tu("RT Spiel: Vorschau durch die Kamera bei " + std::to_string(ms) + " ms", [=] {
            g_app->figAnim.offen = false;
            g_app->throughCamera = true;
            g_app->playing = false;
            g_app->playMs = static_cast<double>(ms);
            g_app->mapDirty = true;
        }));
        add(pause(0.5));
        add(mission1::probeFoto("rt_vorschau_" + std::to_string(ms)));
    }
    add(tu("RT Spiel: Kamera wieder frei", [] { g_app->throughCamera = false; }));
    return s;
}

}  // namespace rt5
