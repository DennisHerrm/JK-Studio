// selbsttest_fix612c.inc - FIX612C (12.11.): Funde P3-5 bis P3-10 und die Vermutungen PV4/PV5 der Codepruefung seit rc611
// (JKStudio_Uebergabe/plaene/codepruefung_seit_rc611_p3.md), je Fund erst rot im laufenden Programm, dann gruen
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=fix612c)
//
//   P3-5  "Animation-Rig (nur FK)" nach "Control Rig erstellen": keine toten IK-Spuren, Pose ohne Sprung, Griffe drehen, die Wahl
//         springt nicht auf einen anderen Griff, Rueckgaengig/Wiederholen stellen Modus, Spuren und Griffe her
//   P3-6  "Knochen waehlen" wie 3ds Max: Anzeige in der Ansicht, Haken im Rechtsklick, Q beendet es, Wechsel von wie Blender
//   P3-7  Szenen-Explorer: "Loesen" im Rechtsklick eines Anbaus liest keine Zeile hinter dem Listenende (Probe-Zaehler)
//   P3-8  Gizmo-Zug am Griff, bei gehaltener Maus Q: der Zug endet, Strg+Z / Strg+Y wirken normal
//   P3-9  [ und ] wie 3ds Max: im Painting der Pinsel (Entscheidung shank), sonst Zoom; die Suche nennt [ dort nicht fuer das Zoomen
//   P3-10 Gang-Regler der Presets: Strg+Z waehrend des Ziehens, weiterziehen - der Rest ist ein eigener Schritt
//   PV4   Reiter des rechten Felds bei sehr kleinem Fenster (Probe: wird das Kindfenster uebersprungen, faellt der Reiter zurueck?)
//   PV5   Wechsel der Rig-Vorlage mit geladener Retarget-Quelle (Probe: bleibt eine Zuordnung von Hand?)
// Eingebunden in selbsttest.cpp (nach den anderen Modell-Block-Gruppen, benutzt die Helfer von ap10, ap2, ap3, rig3, mb3, pt1c).
namespace fix612c {

using ap10::gruppe;
using ap10::ikGriffe;
using ap10::imRoll;
using ap10::zeige;
using mbefoto::kindFenster;

void melde(const std::string& z) { diag::detail("Knopftest: FIX612C " + z); }

const AnimEntry* seq() { return ap10::sequenzJetzt(); }
int f0() { return seq() != nullptr ? seq()->firstFrame : 0; }
int fEnde() { return seq() != nullptr ? seq()->firstFrame + std::max(seq()->numFrames, 1) - 1 : 0; }

// IK-Spuren mit Schluesseln in allen Schichten des Dokuments (tote Spuren, wenn das Rig nur FK hat)
int ikSpurenMitSchluesseln() {
    int n = 0;
    for (const auto& [name, s] : g_app->mtab.dok.inhalt().rig.schichten) {
        for (const modell::IkSpur& sp : s.ik) { n += (!sp.ziel.empty() || !sp.wert.empty()) ? 1 : 0; }
    }
    return n;
}

std::vector<BoneMatrix> welt(int bild) {
    static const modell::RigSchicht kLeer;
    const modell::RigSchicht* s = modellRigSchichtJetzt();
    std::vector<BoneMatrix> w;
    modell::rigWelt(g_app->anim, s != nullptr ? *s : kLeer, bild, bild, 0.0F, w);
    return w;
}

float ort(const std::vector<BoneMatrix>& w, int k, int achse) {
    return k >= 0 && static_cast<std::size_t>(k) < w.size() ? w[static_cast<std::size_t>(k)].m[achse][3] : 0.0F;
}
float ortAbstand(const std::vector<BoneMatrix>& a, const std::vector<BoneMatrix>& b, int k) {
    float d = 0.0F;
    for (int i = 0; i < 3; ++i) { d += (ort(a, k, i) - ort(b, k, i)) * (ort(a, k, i) - ort(b, k, i)); }
    return std::sqrt(d);
}

// eine andere Sequenz der GLA ohne Schicht (Gegenprobe P3-5)
int andereSequenz() {
    const auto& sch = g_app->mtab.dok.inhalt().rig.schichten;
    for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
        const AnimEntry& e = g_app->animList[i];
        if (static_cast<int>(i) != mbefoto::sequenz() && e.numFrames >= 2 && sch.find(e.name) == sch.end()) { return static_cast<int>(i); }
    }
    return -1;
}

int aktion(Str text) {
    for (int i = 0; i < modellRigAktionZahl(); ++i) {
        if (std::string(modellRigAktionText(i)) == tr(text)) { return i; }
    }
    return -1;
}

Schritt zuruecksetzen(const char* was) {
    return tu(std::string("FIX612C: ") + was + " - wie 3ds Max, Bereich Animation, Sequenz, kein Rig, keine Schicht", [] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Animation;
        g_app->settings.modellFeldBreite = 0.0F;
        animWaehlen(mbefoto::sequenz(), false);
        g_app->mtab.rig = modell::Rig{};
        g_app->mtab.rigFuer.clear();
        g_app->mtab.griffWahl = -1;
        g_app->mtab.ikWahl = -1;
        g_app->mtab.gizmo = 0;
        g_app->mtab.rt = ModellRetarget{};
        modellRigNurFkSetzen(false);
        modellAnim().generatorOffen = false;
        g_app->mtab.dok.aendern("FIX612C leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
        g_app->animFrame = f0();
        g_app->animPlaying = false;
        g_app->modelDirty = true;
        if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
    });
}

// --- P3-5 -----------------------------------------------------------------------------------------------------------------

void nurFkSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(zuruecksetzen("P3-5"));
    add(pause(0.5));
    add(tu("FIX612C P3-5: Control Rig erstellen (die Sequenz geht von FK auf IK auf das Rig)", [] { (void)modellAnimRigErstellen(false); }));
    add(pause(0.6));
    auto posen = std::make_shared<std::vector<std::vector<BoneMatrix>>>();
    auto spurenVor = std::make_shared<int>(0);
    add(tu("FIX612C P3-5: Pose jedes Bilds merken, Pol-Ziel Ellbogen R waehlen", [=] {
        posen->clear();
        for (int b = f0(); b <= fEnde(); ++b) { posen->push_back(welt(b)); }
        *spurenVor = ikSpurenMitSchluesseln();
        modellRigGriffWaehlen(rig3::griffNach(modell::GriffArt::PolEllbogen, modell::Seite::Rechts));
        melde("P3-5 vorher: " + std::string(seq() != nullptr ? seq()->name : "-") + ", " + std::to_string(posen->size()) + " Bilder, " + std::to_string(*spurenVor) +
              " IK-Spuren mit Schluesseln, " + std::to_string(ikGriffe()) + " IK-Griffe, Wahl " + std::to_string(g_app->mtab.griffWahl));
    }));
    add(pruefe("FIX612C P3-5 Vorbedingung: Control Rig mit vier IK-Spuren, IK-Griffen und gewaehltem Pol-Ziel", [=] {
        return modellRigDa() && *spurenVor == 4 && ikGriffe() > 0 && g_app->mtab.griffWahl >= 0 && !modellRigNurFk();
    }));
    add(tu("FIX612C P3-5: Animation-Rig (nur FK)", [] { (void)modellAnimRigErstellen(true); }));
    add(pause(0.6));
    add(pruefe("FIX612C P3-5a: nach Animation-Rig (nur FK) traegt keine Schicht mehr IK-Spuren mit Schluesseln (keine toten Spuren)", [] {
        melde("P3-5a: " + std::to_string(ikSpurenMitSchluesseln()) + " IK-Spuren mit Schluesseln, nur FK " + (modellRigNurFk() ? "ja" : "nein") + ", " + std::to_string(ikGriffe()) +
              " IK-Griffe");
        return modellRigNurFk() && ikGriffe() == 0 && ikSpurenMitSchluesseln() == 0;
    }));
    add(pruefe("FIX612C P3-5b: die Pose jedes Bilds der Sequenz bleibt beim Wechsel (kein Sprung, Grenze 0,0001 wie ENT2)", [=] {
        float m = 0.0F;
        int b = f0();
        for (const std::vector<BoneMatrix>& vor : *posen) { m = std::max(m, ap10::poseAbstand(vor, welt(b++))); }
        char z[120];
        std::snprintf(z, sizeof z, "P3-5b: groesster Abstand %.6f ueber %d Bilder", static_cast<double>(m), static_cast<int>(posen->size()));
        melde(z);
        return !posen->empty() && m < 0.0001F;
    }));
    add(pruefe("FIX612C P3-5d: die Wahl springt nicht auf einen anderen Griff (das Pol-Ziel gibt es nur mit IK - keiner gewaehlt)", [] {
        const int w = g_app->mtab.griffWahl;
        melde("P3-5d: Wahl " + std::to_string(w) + (w >= 0 && static_cast<std::size_t>(w) < g_app->mtab.rig.griffe.size()
                                                         ? " Art " + std::to_string(static_cast<int>(g_app->mtab.rig.griffe[static_cast<std::size_t>(w)].art))
                                                         : std::string()));
        return w == -1;
    }));
    auto handVor = std::make_shared<std::vector<BoneMatrix>>();
    add(tu("FIX612C P3-5c: Griff Oberarm R um Welt-Y 30 Grad drehen", [=] {
        g_app->animFrame = f0() + std::min(5, std::max(seq() != nullptr ? seq()->numFrames - 1 : 0, 0));
        *handVor = welt(g_app->animFrame);
        modellRigGriffWaehlen(rig3::griffNach(modell::GriffArt::Oberarm, modell::Seite::Rechts));
        g_app->mtab.gizmo = 2;
        const float w[3] = {0.0F, 30.0F, 0.0F};
        modellRigZiehen(2, w);
        modellRigZiehenEnde();
    }));
    add(pause(0.3));
    add(pruefe("FIX612C P3-5c: im Rig nur mit FK dreht der Oberarm die Hand mit (vorher hielt die tote IK sie fest)", [=] {
        const float d = ortAbstand(*handVor, welt(g_app->animFrame), boneIndex(g_app->anim, "rhand"));
        char z[120];
        std::snprintf(z, sizeof z, "P3-5c: rhand bewegt um %.3f Einheiten", static_cast<double>(d));
        melde(z);
        return d > 1.0F;
    }));
    add(tu("FIX612C P3-5c: Drehung zuruecknehmen", [] { (void)modellRueckgaengig(); }));
    add(pause(0.3));
    auto nameVor = std::make_shared<std::string>();
    add(tu("FIX612C P3-5e: Glied Arm R gewaehlt, Aktion Auf IK", [=] {
        *nameVor = g_app->mtab.dok.rueckgaengigName();
        g_app->mtab.ikWahl = static_cast<int>(modell::IkArt::ArmRechts);
        const int i = aktion(Str::MbIkAufIk);
        if (i >= 0) { modellRigAktionAusloesen(i); }
    }));
    add(pause(0.3));
    add(pruefe("FIX612C P3-5e: Auf IK legt im Rig nur mit FK keine IK-Spur und keinen Schritt an", [=] {
        melde("P3-5e: " + std::to_string(ikSpurenMitSchluesseln()) + " IK-Spuren, oberster Schritt \"" + g_app->mtab.dok.rueckgaengigName() + "\" (vorher \"" + *nameVor + "\")");
        return ikSpurenMitSchluesseln() == 0 && g_app->mtab.dok.rueckgaengigName() == *nameVor;
    }));
    add(tu("FIX612C P3-5g: Auf Rig legen im Rig nur mit FK", [] {
        g_app->mtab.ikWahl = -1;
        (void)modellAnimAufRigLegen();
    }));
    add(pause(0.5));
    add(pruefe("FIX612C P3-5g: Auf Rig legen im Rig nur mit FK legt keine IK-Spur an", [] {
        melde("P3-5g: " + std::to_string(ikSpurenMitSchluesseln()) + " IK-Spuren mit Schluesseln");
        return ikSpurenMitSchluesseln() == 0 && modellRigNurFk();
    }));
    add(tu("FIX612C P3-5f: Strg+Z zweimal (Auf Rig legen, Animation-Rig nur FK)", [] {
        (void)modellRueckgaengig();
        (void)modellRueckgaengig();
    }));
    add(pause(0.5));
    add(pruefe("FIX612C P3-5f: Rueckgaengig stellt das Control Rig wieder her - Modus, vier IK-Spuren, IK-Griffe, Gruppe IK/FK", [=] {
        melde("P3-5f: nur FK " + std::string(modellRigNurFk() ? "ja" : "nein") + ", " + std::to_string(ikSpurenMitSchluesseln()) + " IK-Spuren, " + std::to_string(ikGriffe()) +
              " IK-Griffe, oberster Schritt \"" + g_app->mtab.dok.rueckgaengigName() + "\"");
        return !modellRigNurFk() && ikSpurenMitSchluesseln() == 4 && ikGriffe() > 0;
    }));
    add(zeige(Str::MbIkRollout));
    add(pause(0.3));
    add(pruefe("FIX612C P3-5f: die Gruppe IK/FK zeigt wieder die Glieder", [] { return imRoll(tr(Str::MbIkArmR)) != nullptr && imRoll(tr(Str::AzIkNurFk)) == nullptr; }));
    add(tu("FIX612C P3-5f: Strg+Y", [] { (void)modellWiederholen(); }));
    add(pause(0.5));
    add(pruefe("FIX612C P3-5f: Wiederholen stellt das Rig nur mit FK wieder her (keine Spuren, keine IK-Griffe), die Pose bleibt", [=] {
        float m = 0.0F;
        int b = f0();
        for (const std::vector<BoneMatrix>& vor : *posen) { m = std::max(m, ap10::poseAbstand(vor, welt(b++))); }
        melde("P3-5f Wiederholen: nur FK " + std::string(modellRigNurFk() ? "ja" : "nein") + ", " + std::to_string(ikSpurenMitSchluesseln()) + " IK-Spuren, " + std::to_string(ikGriffe()) +
              " IK-Griffe, Abstand " + std::to_string(m));
        return modellRigNurFk() && ikSpurenMitSchluesseln() == 0 && ikGriffe() == 0 && m < 0.0001F;
    }));
    auto schichtVor = std::make_shared<int>(0);
    add(tu("FIX612C P3-5i: Control Rig erstellen (Schicht schon da)", [=] {
        *schichtVor = ap10::schluessel();
        (void)modellAnimRigErstellen(false);
    }));
    add(pause(0.5));
    // ENT3B (C1, Entscheidung 12.11.): Control Rig erstellen nach nur FK stellt die Glieder wieder auf IK (FK -> IK wie beim ersten
    // Erstellen) - bis dahin blieb die Schicht unberuehrt (FIX612C-Vorgabe)
    add(pruefe("FIX612C P3-5i: Control Rig erstellen nach nur FK: IK-Griffe wieder da, die Glieder wieder auf IK (ENT3B C1), ein Strg+Z fuehrt zurueck auf nur FK mit derselben Schicht", [=] {
        const bool voll = !modellRigNurFk() && ikGriffe() > 0 && ikSpurenMitSchluesseln() == 4;
        (void)modellRueckgaengig();
        const bool zurueck = modellRigNurFk() && ap10::schluessel() == *schichtVor && ikSpurenMitSchluesseln() == 0;
        melde(std::string("P3-5i: voll ") + (voll ? "ja" : "nein") + ", zurueck " + (zurueck ? "ja" : "nein"));
        return voll && zurueck;
    }));
    add(pause(0.3));
    // Gegenprobe: eine Sequenz ohne Schicht (dort drehte der Griff schon vor der Behebung)
    auto andere = std::make_shared<int>(-1);
    auto handAndere = std::make_shared<std::vector<BoneMatrix>>();
    add(tu("FIX612C P3-5h: andere Sequenz ohne Schicht, Oberarm R drehen", [=] {
        *andere = andereSequenz();
        if (*andere < 0) { return; }
        animWaehlen(*andere, false);
        g_app->animFrame = f0();
        *handAndere = welt(g_app->animFrame);
        modellRigGriffWaehlen(rig3::griffNach(modell::GriffArt::Oberarm, modell::Seite::Rechts));
        const float w[3] = {0.0F, 30.0F, 0.0F};
        modellRigZiehen(2, w);
        modellRigZiehenEnde();
    }));
    add(pause(0.3));
    add(pruefe("FIX612C P3-5h Gegenprobe: im Rig nur mit FK dreht der Oberarm auf einer Sequenz ohne Schicht die Hand mit", [=] {
        const float d = ortAbstand(*handAndere, welt(g_app->animFrame), boneIndex(g_app->anim, "rhand"));
        melde("P3-5h: " + std::string(seq() != nullptr ? seq()->name : "-") + ", rhand bewegt um " + std::to_string(d));
        return *andere >= 0 && d > 1.0F && modellRigNurFk();
    }));
    add(tu("FIX612C P3-5j: Drehung zuruecknehmen, Control Rig erstellen auf der Sequenz ohne Schicht", [] {
        (void)modellRueckgaengig();
        (void)modellAnimRigErstellen(false);
    }));
    add(pause(0.6));
    add(pruefe("FIX612C P3-5j: Control Rig erstellen auf einer Sequenz ohne Schicht legt sie auf (IK an allen vier Gliedern); ein Strg+Z nimmt Auflegen und Modus zusammen zurueck", [] {
        const modell::RigSchicht* s = modellRigSchichtJetzt();
        const int ik = s != nullptr ? static_cast<int>(s->ik.size()) : 0;
        const bool voll = !modellRigNurFk() && ik == 4;
        (void)modellRueckgaengig();
        const modell::RigSchicht* s2 = modellRigSchichtJetzt();
        const bool zurueck = modellRigNurFk() && (s2 == nullptr || (s2->schluesselZahl() == 0 && s2->ik.empty()));
        melde("P3-5j: " + std::to_string(ik) + " IK-Spuren, voll " + (voll ? "ja" : "nein") + ", ein Strg+Z: nur FK " + (modellRigNurFk() ? "ja" : "nein") + ", Schicht leer " +
              (zurueck ? "ja" : "nein"));
        return voll && zurueck;
    }));
    add(tu("FIX612C P3-5: zurueck auf die Sequenz", [] {
        animWaehlen(mbefoto::sequenz(), false);
        modellRigNurFkSetzen(false);
    }));
    add(pause(0.3));
}

// --- P3-6 -----------------------------------------------------------------------------------------------------------------

ImVec2 torso() {
    ImVec2 m{-1.0F, -1.0F};
    if (!modellFlaecheAufSchirm(mb1::teil("torso"), m)) { meldeFehler("FIX612C: torso nicht auf dem Schirm"); }
    return m;
}

void knochenSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("FIX612C P3-6: wie 3ds Max, Bereich Ansehen, Werkzeug aus, keine Auswahl", [] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Ansehen;
        modellBl().werkzeug = 0;
        g_app->mtab.auswahl.clear();
        g_app->mtab.gizmo = 0;
    }));
    add(pause(0.6));
    rig3::ueberSuche(s, Str::BzKnochenWaehlen);
    add(pause(0.4));
    add(pruefe("FIX612C P3-6a: Knochen waehlen ueber die Suche - an, mit Meldung und Anzeige in der Ansicht", [] {
        melde("P3-6a: Werkzeug " + std::to_string(modellBl().werkzeug) + ", Meldung \"" + g_app->mtab.meldung + "\", Anzeige im Bild " +
              std::to_string(fix612cProbe().knochenHinweisBild) + " (jetzt " + std::to_string(ImGui::GetFrameCount()) + ")");
        return modellBl().werkzeug == 2 && g_app->mtab.meldung == tr(Str::BzKnochenAn) && fix612cProbe().knochenHinweisBild >= ImGui::GetFrameCount() - 3;
    }));
    add(klickAn("FIX612C P3-6b: Rechtsklick in die Ansicht", mbefoto::ansichtEcke, 1));
    add(pause(0.4));
    add(pruefe("FIX612C P3-6b: im Rechtsklick steht Knochen waehlen mit Haken", [] {
        const Element* e = rig3::imMenue(tr(Str::BzKnochenWaehlen));
        melde("P3-6b: Eintrag " + std::string(e != nullptr ? "da" : "fehlt") + (e != nullptr && (e->flags & ImGuiItemStatusFlags_Checked) != 0 ? ", mit Haken" : ""));
        return e != nullptr && (e->flags & ImGuiItemStatusFlags_Checked) != 0;
    }));
    add(taste(ImGuiKey_Escape, "Esc (Menue zu)"));
    add(pause(0.3));
    // das Menue der Ansicht schliesst Esc nicht (es hat keinen Tastaturfokus); der erste Klick daneben schliesst es und nimmt dem Model-Tab
    // den Fokus, der zweite gibt ihn zurueck (wie rig3::ueberSuche)
    for (int k = 0; k < 2; ++k) {
        add(klickAn("FIX612C P3-6c: in eine leere Ecke der Ansicht", mbefoto::ansichtEcke));
        add(pause(0.3));
    }
    add(taste(ImGuiKey_Q, "Q"));
    add(pause(0.3));
    add(klickAn("FIX612C P3-6c: auf den Rumpf", torso));
    add(pause(0.4));
    add(pruefe("FIX612C P3-6c: Q beendet Knochen waehlen - ein Klick auf den Rumpf waehlt wieder die Flaeche", [] {
        melde("P3-6c: Werkzeug " + std::to_string(modellBl().werkzeug) + ", " + std::to_string(g_app->mtab.auswahl.size()) + " gewaehlt");
        return modellBl().werkzeug == 0 && std::find(g_app->mtab.auswahl.begin(), g_app->mtab.auswahl.end(), mb1::teil("torso")) != g_app->mtab.auswahl.end();
    }));
    add(tu("FIX612C P3-6d: wie Blender Knochen waehlen, zurueck auf wie 3ds Max", [] {
        modellBedienungSetzen(1);
        modellBl().werkzeug = 2;
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.auswahl.clear();
    }));
    add(pause(0.5));
    add(klickAn("FIX612C P3-6d: auf den Rumpf", torso));
    add(pause(0.4));
    add(pruefe("FIX612C P3-6d: das Werkzeug Knochen waehlen der Werkzeugleiste wie Blender kommt nicht unsichtbar mit nach wie 3ds Max", [] {
        melde("P3-6d: Werkzeug " + std::to_string(modellBl().werkzeug) + ", " + std::to_string(g_app->mtab.auswahl.size()) + " gewaehlt");
        return modellBl().werkzeug == 0 && !g_app->mtab.auswahl.empty();
    }));
    add(klickAn("FIX612C P3-6e: Rechtsklick in die Ansicht", mbefoto::ansichtEcke, 1));
    add(pause(0.4));
    add(rig3::klickMenue(tr(Str::BzKnochenWaehlen), false));
    add(pause(0.4));
    add(pruefe("FIX612C P3-6e: Rechtsklick > Knochen waehlen schaltet es an", [] { return modellBl().werkzeug == 2; }));
    add(klickAn("FIX612C P3-6e: Rechtsklick in die Ansicht", mbefoto::ansichtEcke, 1));
    add(pause(0.4));
    add(rig3::klickMenue(tr(Str::BzKnochenWaehlen), false));
    add(pause(0.4));
    add(pruefe("FIX612C P3-6e: noch einmal schaltet es aus", [] { return modellBl().werkzeug == 0; }));
}

// --- P3-7 -----------------------------------------------------------------------------------------------------------------

std::function<ImVec2()> zeilenOrt(ExplorerArt art, int index) {
    return [=] {
        const ExplorerLage* l = ap2::zeileArt(art, index);
        if (l == nullptr || l->xName < 0.0F) { return ImVec2{-1.0F, -1.0F}; }
        return ImVec2{l->xName + ImGui::GetFontSize(), (l->y0 + l->y1) * 0.5F};
    };
}

void explorerSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("FIX612C P3-7: Bereich Ansehen, zwei Anbauten (Saebel an *r_hand und *l_hand), Explorer nach Ebenen, Ebene Anbauten offen", [] {
        g_app->mtab.bereich = ModellBereich::Ansehen;
        modellBl().werkzeug = 0;
        g_app->mtab.anbauten.clear();
        (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true);
        (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*l_hand", false, true);
        modellExplorer().art = 1;
        modellExplorerAufklappen(ExplorerArt::Ebene, static_cast<int>(ExplorerEbene::Anbauten), true);
        modellExplorerZeigen(ExplorerArt::Ebene, static_cast<int>(ExplorerEbene::Anbauten));
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(pruefe("FIX612C P3-7 Vorbedingung: zwei Anbau-Zeilen im Explorer", [] {
        melde("P3-7: " + std::to_string(g_app->mtab.anbauten.size()) + " Anbauten, Zeile 0 " + (ap2::zeileArt(ExplorerArt::Anbau, 0) != nullptr ? "da" : "fehlt") + ", Zeile 1 " +
              (ap2::zeileArt(ExplorerArt::Anbau, 1) != nullptr ? "da" : "fehlt"));
        return g_app->mtab.anbauten.size() == 2U && ap2::zeileArt(ExplorerArt::Anbau, 0) != nullptr && ap2::zeileArt(ExplorerArt::Anbau, 1) != nullptr;
    }));
    add(klickAn("FIX612C P3-7: Rechtsklick auf den ersten Anbau", zeilenOrt(ExplorerArt::Anbau, 0), 1));
    add(pause(0.4));
    add(tu("FIX612C P3-7: Probe-Zaehler auf 0", [] { fix612cProbe().explorerHinterEnde = 0; }));
    add(rig3::klickMenue(tr(Str::MbLoesen), false));
    add(pause(0.5));
    add(pruefe("FIX612C P3-7a: Loesen im Rechtsklick des ersten Anbaus - keine Zeile liest hinter das Ende der Anbauten, ein Anbau bleibt", [] {
        melde("P3-7a: " + std::to_string(fix612cProbe().explorerHinterEnde) + " Zeilen hinter dem Listenende, " + std::to_string(g_app->mtab.anbauten.size()) + " Anbauten");
        return fix612cProbe().explorerHinterEnde == 0 && g_app->mtab.anbauten.size() == 1U && ap2::zeileArt(ExplorerArt::Anbau, 0) != nullptr &&
               ap2::zeileArt(ExplorerArt::Anbau, 1) == nullptr;
    }));
    // zweiter Weg: Kopfzeile "Control Rig" > Animation-Rig (nur FK) - die IK-Griffe fallen weg
    add(zuruecksetzen("P3-7b"));
    add(tu("FIX612C P3-7b: Control Rig, Explorer nach Hierarchie, Griffe offen", [] {
        (void)modellAnimRigErstellen(false);
        modellExplorer().art = 0;
        modellExplorerAufklappen(ExplorerArt::KopfRig, 0, true);
        modellExplorerZeigen(ExplorerArt::KopfRig, 0);
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(tu("FIX612C P3-7b: Griff-Zeilen melden (der Selbsttest baut alle Zeilen ausser den Sequenzen)", [] {
        int letzte = -1;
        for (const ExplorerLage& l : modellExplorer().lage) {
            if (l.art == ExplorerArt::Griff) { letzte = std::max(letzte, l.index); }
        }
        melde("P3-7b: " + std::to_string(g_app->mtab.rig.griffe.size()) + " Griffe, letzte Zeile " + std::to_string(letzte) + ", Kopfzeile " +
              (ap2::zeileArt(ExplorerArt::KopfRig) != nullptr ? "da" : "fehlt"));
    }));
    add(pause(0.4));
    add(klickAn("FIX612C P3-7b: Rechtsklick auf die Kopfzeile Control Rig", zeilenOrt(ExplorerArt::KopfRig, -1), 1));
    add(pause(0.4));
    add(tu("FIX612C P3-7b: Probe-Zaehler auf 0", [] { fix612cProbe().explorerHinterEnde = 0; }));
    add(rig3::klickMenue(tr(Str::AzRigFk), false));
    add(pause(0.5));
    add(pruefe("FIX612C P3-7b: Animation-Rig (nur FK) im Rechtsklick der Kopfzeile - keine Griff-Zeile liest hinter das Ende", [] {
        melde("P3-7b: " + std::to_string(fix612cProbe().explorerHinterEnde) + " Zeilen hinter dem Listenende, nur FK " + (modellRigNurFk() ? "ja" : "nein"));
        return fix612cProbe().explorerHinterEnde == 0 && modellRigNurFk();
    }));
    add(tu("FIX612C P3-7: zurueck (keine Anbauten, nach Ebenen aus)", [] {
        g_app->mtab.anbauten.clear();
        modellExplorer().art = 0;
        modellRigNurFkSetzen(false);
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
}

// --- P3-8 -----------------------------------------------------------------------------------------------------------------

Schritt ringZugMitQ(std::shared_ptr<bool> zog) {
    auto z = std::make_shared<ImVec2>();
    return {"FIX612C P3-8: am Ring Z ziehen, bei gehaltener Maus Q, loslassen", [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    ImVec2 p{-1.0F, -1.0F};
                    if (!modellGizmoRingOrt(2, 45.0F, p)) { meldeFehler("FIX612C P3-8: kein Ring"); }
                    *z = p;
                    setzeMaus(p);
                    return false;
                }
                if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                if (b >= 3 && b <= 10) {
                    setzeMaus(ImVec2{z->x + 6.0F * static_cast<float>(b - 2), z->y});
                    return false;
                }
                if (b == 11) {
                    *zog = g_app->mtab.rigZieht;
                    io.AddKeyEvent(ImGuiKey_Q, true);
                    return false;
                }
                if (b == 12) { io.AddKeyEvent(ImGuiKey_Q, false); return false; }
                if (b == 13) { setzeMaus(ImVec2{z->x + 54.0F, z->y}); return false; }
                if (b == 15) { io.AddMouseButtonEvent(0, false); return false; }
                return b >= 20;
            }};
}

float drehAbstand(const BoneMatrix& a, const BoneMatrix& b) {
    float m = 0.0F;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { m = std::max(m, std::fabs(a.m[i][j] - b.m[i][j])); }
    }
    return m;
}

void gizmoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(zuruecksetzen("P3-8"));
    add(tu("FIX612C P3-8: Control Rig, Griff Kopf", [] {
        (void)modellAnimRigErstellen(false);
        modellRigGriffWaehlen(rig3::griffNach(modell::GriffArt::Kopf, modell::Seite::Mitte));
    }));
    add(pause(0.5));
    add(klickAn("FIX612C P3-8: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.2));
    add(tu("FIX612C P3-8: Griff Kopf wieder waehlen", [] { modellRigGriffWaehlen(rig3::griffNach(modell::GriffArt::Kopf, modell::Seite::Mitte)); }));
    add(taste(ImGuiKey_E, "E"));
    add(pause(0.3));
    auto vorher = std::make_shared<BoneMatrix>();
    add(tu("FIX612C P3-8: Kopf vorher", [=] { *vorher = g_app->modellWelt[static_cast<std::size_t>(boneIndex(g_app->anim, "cranium"))]; }));
    auto zog = std::make_shared<bool>(false);
    add(ringZugMitQ(zog));
    add(pause(0.3));
    auto gedreht = std::make_shared<BoneMatrix>();
    add(pruefe("FIX612C P3-8a: nach Q bei gehaltener Maus und Loslassen ist der Zug beendet", [=] {
        *gedreht = g_app->modellWelt[static_cast<std::size_t>(boneIndex(g_app->anim, "cranium"))];
        melde("P3-8a: Zug lief " + std::string(*zog ? "ja" : "nein") + ", Zug offen " + (g_app->mtab.rigZieht ? "ja" : "nein") + ", Gizmo " + std::to_string(g_app->mtab.gizmo) +
              ", Kopf gedreht " + std::to_string(drehAbstand(*vorher, *gedreht)));
        return *zog && !g_app->mtab.rigZieht && g_app->mtab.gizmo == 0 && drehAbstand(*vorher, *gedreht) > 0.01F;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("FIX612C P3-8b: Strg+Z nimmt die Drehung zurueck", [=] {
        return drehAbstand(*vorher, g_app->modellWelt[static_cast<std::size_t>(boneIndex(g_app->anim, "cranium"))]) < 1e-4F;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Y, "Strg+Y"));
    add(pause(0.3));
    add(pruefe("FIX612C P3-8c: Strg+Y stellt die Drehung wieder her", [=] {
        const float d = drehAbstand(*gedreht, g_app->modellWelt[static_cast<std::size_t>(boneIndex(g_app->anim, "cranium"))]);
        melde("P3-8c: Abstand zur gedrehten Pose " + std::to_string(d));
        return d < 1e-4F;
    }));
}

// --- P3-10 ----------------------------------------------------------------------------------------------------------------

Schritt reglerMitStrgZ(const std::string& label) {
    auto r = std::make_shared<ImRect>();
    return {"FIX612C P3-10: Regler " + label + " ziehen, mittendrin Strg+Z, weiterziehen", [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                const auto bei = [&](float a) { setzeMaus(ImVec2{r->Min.x + r->GetWidth() * a, r->GetCenter().y}); };
                if (b == 0) {
                    const Element* e = finde(label, "mtrollouts", 0, false);
                    if (e == nullptr) {
                        meldeFehler("FIX612C P3-10: " + label + " nicht gefunden");
                        return true;
                    }
                    *r = e->rect;
                    bei(0.2F);
                    return false;
                }
                if (b == 1) { io.AddMouseButtonEvent(0, true); return false; }
                if (b >= 2 && b <= 6) { bei(0.2F + 0.02F * static_cast<float>(b - 1)); return false; }
                if (b == 7) {
                    io.AddKeyEvent(ImGuiMod_Ctrl, true);
                    io.AddKeyEvent(ImGuiKey_Z, true);
                    return false;
                }
                if (b == 8) {
                    io.AddKeyEvent(ImGuiKey_Z, false);
                    io.AddKeyEvent(ImGuiMod_Ctrl, false);
                    return false;
                }
                if (b >= 9 && b <= 14) { bei(0.3F + 0.025F * static_cast<float>(b - 8)); return false; }
                if (b == 15) { io.AddMouseButtonEvent(0, false); return false; }
                return b >= 18;
            }};
}

void reglerSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(zuruecksetzen("P3-10"));
    add(pause(0.4));
    add(zeige(Str::PrRollout));
    add(pause(0.4));
    add(klick(tr(Str::AzKatGehen), "mtrollouts"));
    add(pause(0.4));
    add(klickAn("FIX612C P3-10: Gehen (erzeugt) waehlen", [] {
        const Element* e = mbefoto::inFenster(tr(Str::EnGangGehen), "##aibib");
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
    }));
    add(pause(0.5));
    auto tempoVor = std::make_shared<float>(0.0F);
    auto nameVor = std::make_shared<std::string>();
    add(tu("FIX612C P3-10: Tempo und obersten Schritt merken", [=] {
        *tempoVor = modellRtDaten()->gang.tempo;
        *nameVor = g_app->mtab.dok.rueckgaengigName();
    }));
    add(reglerMitStrgZ(tr(Str::PrTempo)));
    add(pause(0.4));
    auto tempoNach = std::make_shared<float>(0.0F);
    add(pruefe("FIX612C P3-10a: nach Strg+Z mitten im Ziehen ist der Rest des Zugs ein eigener Schritt", [=] {
        *tempoNach = modellRtDaten()->gang.tempo;
        melde("P3-10a: Tempo " + std::to_string(*tempoVor) + " -> " + std::to_string(*tempoNach) + ", oberster Schritt \"" + g_app->mtab.dok.rueckgaengigName() + "\" (vorher \"" +
              *nameVor + "\")");
        return std::fabs(*tempoNach - *tempoVor) > 1.0F && g_app->mtab.dok.rueckgaengigName() == tr(Str::PrUndo);
    }));
    add(tu("FIX612C P3-10b: Strg+Z", [] { (void)modellRueckgaengig(); }));
    add(pause(0.3));
    add(pruefe("FIX612C P3-10b: ein Strg+Z nimmt nur diesen Schritt zurueck (Tempo wie vorher, darunter der Schritt von vorher)", [=] {
        melde("P3-10b: Tempo " + std::to_string(modellRtDaten()->gang.tempo) + ", oberster Schritt \"" + g_app->mtab.dok.rueckgaengigName() + "\"");
        return std::fabs(modellRtDaten()->gang.tempo - *tempoVor) < 1e-4F && g_app->mtab.dok.rueckgaengigName() == *nameVor;
    }));
}

// --- P3-9 -----------------------------------------------------------------------------------------------------------------

// Die Suche (X) mit dem gemeinsamen Anfang der beiden Zoom-Texte oeffnen ("Zoom viewport in")
void zoomSuche(std::vector<Schritt>& s) {
    s.push_back(taste(ImGuiKey_X, "X (Alle Aktionen suchen)"));
    s.push_back(pause(0.3));
    const std::string t = tr(Str::MnZoomHeran2);
    s.push_back(tippe(t.substr(0, t.rfind(" ("))));
    s.push_back(pause(0.3));
}

void klammerSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto radius = std::make_shared<float>(0.0F);
    auto abstand = std::make_shared<float>(0.0F);
    const auto merken = [=] {
        *radius = g_app->mtab.malen.radiusPx;
        *abstand = g_app->modelOrbit.distance;
    };
    add(tu("FIX612C P3-9: wie 3ds Max, Bereich Painting, Maus in die Ansicht", [=] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Painting;
    }));
    add(pause(0.8));
    add(tu("FIX612C P3-9: Maus in die Ansicht, Pinsel und Abstand merken", [=] {
        setzeMaus(mbefoto::ansichtEcke());
        merken();
    }));
    add(taste(ImGuiKey_LeftBracket, "["));
    add(pause(0.3));
    add(pruefe("FIX612C P3-9a: im Painting stellt [ wie 3ds Max den Pinsel kleiner, die Ansicht zoomt nicht (Entscheidung shank)", [=] {
        melde("P3-9a: Radius " + std::to_string(*radius) + " -> " + std::to_string(g_app->mtab.malen.radiusPx) + ", Abstand " + std::to_string(*abstand) + " -> " +
              std::to_string(g_app->modelOrbit.distance));
        return g_app->mtab.malen.radiusPx < *radius - 0.01F && std::fabs(g_app->modelOrbit.distance - *abstand) < 1e-4F;
    }));
    zoomSuche(s);
    add(pruefe("FIX612C P3-9b: im Painting nennt die Suche das Zoomen nicht mit [ (dort stellt die Taste den Pinsel), aber mit Strg+=", [] {
        const bool klammer = rig3::imMenue(tr(Str::MnZoomHeran)) != nullptr;
        const bool strg = rig3::imMenue(tr(Str::MnZoomHeran2)) != nullptr;
        melde(std::string("P3-9b: Suche mit [ ") + (klammer ? "da" : "fehlt") + ", mit Strg+= " + (strg ? "da" : "fehlt"));
        return !klammer && strg;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.3));
    add(tu("FIX612C P3-9: Bereich Ansehen", [=] {
        g_app->mtab.bereich = ModellBereich::Ansehen;
        modellBl().werkzeug = 0;
    }));
    add(pause(0.5));
    add(tu("FIX612C P3-9: Maus in die Ansicht, merken", [=] {
        setzeMaus(mbefoto::ansichtEcke());
        merken();
    }));
    add(taste(ImGuiKey_LeftBracket, "["));
    add(pause(0.5));
    add(pruefe("FIX612C P3-9c: ausserhalb des Painting zoomt [ die Ansicht, der Pinsel bleibt", [=] {
        melde("P3-9c: Radius " + std::to_string(*radius) + " -> " + std::to_string(g_app->mtab.malen.radiusPx) + ", Abstand " + std::to_string(*abstand) + " -> " +
              std::to_string(g_app->modelOrbit.distance));
        return std::fabs(g_app->mtab.malen.radiusPx - *radius) < 1e-4F && std::fabs(g_app->modelOrbit.distance - *abstand) > 1e-3F;
    }));
    zoomSuche(s);
    add(pruefe("FIX612C P3-9d: ausserhalb des Painting nennt die Suche beide Tasten", [] {
        return rig3::imMenue(tr(Str::MnZoomHeran)) != nullptr && rig3::imMenue(tr(Str::MnZoomHeran2)) != nullptr;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.3));
}

// --- PV4 / PV5 (Proben) ---------------------------------------------------------------------------------------------------

std::string kettenNamen() {
    std::string n;
    for (std::size_t i = 0; i < g_app->mtab.rt.cz.ketten.size() && i < 6; ++i) { n += (i > 0 ? "," : "") + g_app->mtab.rt.cz.ketten[i].schluessel(); }
    return n;
}

void pv5Schritte(std::vector<Schritt>& s, const std::string& figur) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(zuruecksetzen("PV5"));
    add(tu("FIX612C PV5 " + figur + ": Quelle laden", [figur] {
        bool ok = false;
        for (const char* q : {"models/players/rockettrooper/rockettrooper.gla", "models/players/hazardtrooper/hazardtrooper.gla", "models/players/_humanoid/_humanoid.gla"}) {
            if (!ok) { ok = modellRtQuelleLaden(q); }
        }
        g_app->mtab.rt.oeffnen = false;
        melde("PV5 " + figur + " Quelle: " + g_app->mtab.rt.quelleFuer);
    }));
    add(pause(1.2));
    auto zuVor = std::make_shared<modell::RtZuordnung>();
    add(tu("FIX612C PV5 " + figur + ": Kette 0 von Hand der letzten Kette der Quelle zuordnen", [=] {
        const modell::RetargetDaten* d = modellRtDaten();
        const int ziel = d != nullptr && !d->zu.quelle.empty() ? static_cast<int>(g_app->mtab.rt.cq.ketten.size()) - 1 : -1;
        (void)modellRtEinstellen("FIX612C von Hand", [ziel](modell::RetargetDaten& x) {
            if (x.zu.quelle.empty()) { return; }
            x.zu.quelle[0] = ziel;
            x.zu.vonHand.resize(x.zu.quelle.size(), 0);
            x.zu.vonHand[0] = 1;
        });
        *zuVor = modellRtDaten()->zu;
        melde("PV5 " + figur + ": Vorlage " + std::to_string(modellAnimVorlageWahl()) + ", " + std::to_string(g_app->mtab.rt.cz.ketten.size()) + " Ketten im Ziel (" + kettenNamen() + "), Kette 0 -> " + std::to_string(ziel));
    }));
    add(pause(0.5));
    add(tu("FIX612C PV5 " + figur + ": Vorlage Eigenes Skelett", [] { modellAnimVorlageSetzen(-1); }));
    add(pause(0.8));
    add(tu("FIX612C PV5 " + figur + ": Stand melden, Vorlage zurueck auf Suchen", [figur] {
        const modell::RetargetDaten* d = modellRtDaten();
        melde("PV5 " + figur + " eigenes Skelett: " + std::to_string(g_app->mtab.rt.cz.ketten.size()) + " Ketten (" + kettenNamen() + "), Zuordnung " + std::to_string(d->zu.quelle.size()) + ", Kette 0 -> " +
              std::to_string(d->zu.quelle.empty() ? -9 : d->zu.quelle[0]));
        modellAnimVorlageSetzen(-2);
    }));
    add(pause(0.8));
    add(pruefe("FIX612C PV5 " + figur + ": zurueck auf die alte Vorlage ist die Zuordnung von Hand wieder da", [=] {
        const modell::RetargetDaten* d = modellRtDaten();
        melde("PV5 " + figur + " zurueck: " + std::to_string(g_app->mtab.rt.cz.ketten.size()) + " Ketten, Kette 0 -> " + std::to_string(d->zu.quelle.empty() ? -9 : d->zu.quelle[0]) + " (vorher " +
              std::to_string(zuVor->quelle.empty() ? -9 : zuVor->quelle[0]) + ")");
        return d->zu.quelle == zuVor->quelle && d->zu.vonHand == zuVor->vonHand;
    }));
}

void probenSchritte(std::vector<Schritt>& s, int breite, int hoehe) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(zuruecksetzen("PV4"));
    add(pause(0.4));
    add(klick(tr(Str::MfHierarchie), "mtbefehlsfeld"));
    add(pause(0.4));
    auto reiter = std::make_shared<int>(-99);
    auto uebersprungen = std::make_shared<int>(0);
    add(tu("FIX612C PV4: Reiter merken", [=] {
        *reiter = modellFeld().reiter;
        *uebersprungen = 0;
        melde("PV4: Reiter " + std::to_string(*reiter));
    }));
    for (const int h : {600, 420, 320, 240, 160}) {
        add(Schritt{"FIX612C PV4: Fenster " + std::to_string(breite) + "x" + std::to_string(h), [=](int b) {
                        if (b == 0) {
                            const auto g = platform::hauptfensterGroesse(breite, h);
                            melde("PV4: Fenster verlangt " + std::to_string(h) + ", bekommen " + std::to_string(g.first) + "x" + std::to_string(g.second));
                            return false;
                        }
                        if (const ImGuiWindow* w = kindFenster("mtrollouts")) { *uebersprungen += w->SkipItems ? 1 : 0; }
                        if (b == 30) {
                            const ImGuiWindow* w = kindFenster("mtrollouts");
                            melde("PV4: Hoehe " + std::to_string(h) + ": Feld " + (w != nullptr ? std::to_string(static_cast<int>(w->Size.y)) : std::string("fehlt")) + ", Reiter " +
                                  std::to_string(modellFeld().reiter) + ", Bilder mit SkipItems bisher " + std::to_string(*uebersprungen));
                        }
                        return b >= 30;
                    }});
    }
    add(tu("FIX612C PV4: Fenster zurueck", [=] { (void)platform::hauptfensterGroesse(breite, hoehe); }));
    add(pause(1.0));
    add(pruefe("FIX612C PV4: nach sehr kleinem Fenster ist der gewaehlte Reiter (Hierarchie) noch gewaehlt", [=] {
        melde("PV4: Reiter vorher " + std::to_string(*reiter) + ", nachher " + std::to_string(modellFeld().reiter) + ", Bilder mit SkipItems " + std::to_string(*uebersprungen));
        return modellFeld().reiter == *reiter;
    }));
    pv5Schritte(s, "kyle");
    for (Schritt& x : mb1::laden("models/players/rancor/model.glm")) { add(std::move(x)); }
    pv5Schritte(s, "rancor");
    add(tu("FIX612C PV5: zurueck", [] { g_app->mtab.rt = ModellRetarget{}; }));
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "fix612c") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_FIX612C_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    auto skala = std::make_shared<float>(1.0F);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("FIX612C: Fenster, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "fix612c.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    add(tu("FIX612C: Aufteilung wie neue Einstellungen, Bereich Animation", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        g_app->mtab.bereich = ModellBereich::Animation;
        g_app->mtab.vierAnsichten = false;
        melde(g_app->modelPath + ", " + std::to_string(g_app->anim.bones.size()) + " Knochen");
    }));
    add(pause(1.0));
    add(klickAn("FIX612C: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    nurFkSchritte(s);
    knochenSchritte(s);
    explorerSchritte(s);
    gizmoSchritte(s);
    reglerSchritte(s);
    klammerSchritte(s);
    probenSchritte(s, b, h);
    add(tu("FIX612C: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        modellRigNurFkSetzen(false);
        modellBl().werkzeug = 0;
        g_app->uiScale = *skala;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.3));
    return s;
}

}  // namespace fix612c
