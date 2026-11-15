// selbsttest_ent3b.inc - ENTSCHEID3B (13.11., plaene/entscheidungen_1211.md, verbindlich): BHED_EDITORTEST=modellblock,
// BHED_MODELL_NUR=ent3b
//
//   B15  Crouch idle und Crouch walk stehen in der Kategorie Ducken (presets.txt), Crouch walk auf seinem Platz des Spiels
//   B16  Tempo des Generators als Faktor (1,0x), die Einheiten je Sekunde im Tooltip; gleicher Wert = dieselbe Bewegung
//   B14  IK/FK blendet zwischen zwei Schluesseln ueber wie CAT: eine Spur je Glied im Dopesheet (sichtbar, verschieben, loeschen),
//        kein Sprung beim Ueberblenden (Grenze 0,0001 wie ENT2), Regler IK/FK wie CATs Limb Animation auch wie 3ds Max
//   B17  der Bodenkreis ist der Griff der Wurzel: verschiebt und dreht die ganze Figur (mit den IK-Zielen) wie CATs Plattform, mit
//        Schluessel und Rueckgaengig; die Kopfmarke bleibt Schmuck
//   C1   Control Rig erstellen nach "nur FK" stellt die Glieder wieder auf IK (FK->IK wie beim ersten Erstellen), Rueckgaengig alles
//   C3   die Rig-Vorlage steht im Dokument: Rueckgaengig und Wiederholen nehmen den Wechsel mit
//   BHED_ENT3B_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche
// Eingebunden in selbsttest.cpp am Ende der Liste (benutzt die Helfer von ap10, ent2, ap13k, mbefoto).
namespace ent3b {

using ap10::gruppe;
using ap10::imRoll;
using ap10::imRollIn;
using ap10::zeige;
using mbefoto::inFenster;
using mbefoto::kindFenster;

void melde(const std::string& z) { diag::detail("Knopftest: ENT3B " + z); }

const AnimEntry* seq() { return ap10::sequenzJetzt(); }
int f0() { return seq() != nullptr ? seq()->firstFrame : 0; }
int fEnde() { return seq() != nullptr ? seq()->firstFrame + std::max(seq()->numFrames, 1) - 1 : 0; }
const modell::RigSchicht* sch() { return modellRigSchichtJetzt(); }
int spurIndex(modell::IkArt a) { return sch() != nullptr ? modell::ikSpurIndex(*sch(), a) : -1; }

std::vector<BoneMatrix> weltBei(const modell::RigSchicht& s, int a, int b, float anteil) {
    std::vector<BoneMatrix> w;
    modell::rigWelt(g_app->anim, s, a, b, anteil, w);
    return w;
}
float ortAbstand(const BoneMatrix& a, const BoneMatrix& b) {
    return std::sqrt((a.m[0][3] - b.m[0][3]) * (a.m[0][3] - b.m[0][3]) + (a.m[1][3] - b.m[1][3]) * (a.m[1][3] - b.m[1][3]) + (a.m[2][3] - b.m[2][3]) * (a.m[2][3] - b.m[2][3]));
}
// der groesste Unterschied zweier Welten (Ort)
float weltAbstand(const std::vector<BoneMatrix>& a, const std::vector<BoneMatrix>& b) {
    float d = 0.0F;
    for (std::size_t k = 0; k < a.size() && k < b.size(); ++k) { d = std::max(d, ortAbstand(a[k], b[k])); }
    return a.size() == b.size() && !a.empty() ? d : 1e9F;
}
// Welt am Bild (Kommabild: zwischen f und f + 1)
std::vector<BoneMatrix> weltAm(const modell::RigSchicht& s, float bild) {
    const int f = static_cast<int>(std::floor(bild));
    const float t = bild - static_cast<float>(f);
    return t > 0.0F ? weltBei(s, f, f + 1, t) : weltBei(s, f, f, 0.0F);
}

void rigFrisch() {
    ModellTab& t = g_app->mtab;
    t.bereich = ModellBereich::Animation;
    animWaehlen(mbefoto::sequenz(), false);
    t.rig = modell::Rig{};
    t.rigFuer.clear();
    t.griffWahl = -1;
    t.knochenWahl = -1;
    t.ikWahl = -1;
    t.kvWahl.clear();
    t.rt = ModellRetarget{};
    modellRigNurFkSetzen(false);
    t.dok.aendern("ENT3B leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
    (void)modellAnimRigErstellen(false);
    g_app->animFrame = f0();
    g_app->animPlaying = false;
    t.rigZeigen = true;
    g_app->modelDirty = true;
}

int griffNr(modell::GriffArt a, modell::Seite s) {
    const modell::Rig& rig = g_app->mtab.rig;
    for (std::size_t i = 0; i < rig.griffe.size(); ++i) {
        if (rig.griffe[i].art == a && rig.griffe[i].seite == s) { return static_cast<int>(i); }
    }
    return -1;
}
int wurzelGriff() { return griffNr(modell::GriffArt::Wurzel, modell::Seite::Mitte); }

// eine Zeile des Dopesheets nach ihrem Code
const Ent3bDsZeile* dsZeile(int code) {
    for (const Ent3bDsZeile& z : ent3bProbe().dsZeilen) {
        if (z.code == code) { return &z; }
    }
    return nullptr;
}

// --- B15 / B16: Presets und Generator -------------------------------------------------------------------------------------

void presetSchritte(std::vector<Schritt>& s, int modus) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string m = modus == 0 ? " wie 3ds Max" : " wie Blender";
    add(tu("ENT3B" + m + ": Bereich Animation, Gruppe Presets, Generator wie neu", [modus] {
        modellBedienungSetzen(modus);
        g_app->mtab.bereich = ModellBereich::Animation;
        g_app->mtab.rt = ModellRetarget{};
        g_app->mtab.dok.aendern("ENT3B Presets leer", [](modell::Inhalt& in) { in.rig.rt = modell::RetargetDaten{}; }, false);
        modellAnim().generatorOffen = false;
    }));
    add(pause(0.6));
    add(zeige(Str::PrRollout));
    add(pause(0.4));
    if (modus == 0) {
        add(pruefe("ENT3B B15: Crouch idle und Crouch walk stehen in presets.txt in der Kategorie crouch (Ducken), nicht mehr in Stehen/Gehen; Crouch walk auf BOTH_CROUCH1WALK", [] {
            int ducken = 0;
            int anders = 0;
            std::string platz;
            for (const PresetEintrag& e : modellPresetBibliothek()) {
                const bool crouch = e.name == "Crouch idle" || e.name == "Crouch walk";
                if (!crouch) { continue; }
                (e.kategorie == "crouch" ? ducken : anders) += 1;
                if (e.name == "Crouch walk") { platz = e.platz; }
            }
            melde("B15 presets.txt: in crouch " + std::to_string(ducken) + ", anderswo " + std::to_string(anders) + ", Platz Crouch walk " + platz);
            return ducken == 2 && anders == 0 && platz == "BOTH_CROUCH1WALK";
        }));
    }
    add(klick(tr(Str::AzKatDucken), "mtrollouts"));
    add(pause(0.5));
    add(pruefe("ENT3B B15" + m + ": die Kategorie Ducken zeigt Crouch idle und Crouch walk (Kurzinfo Ducken · 2), ohne den Hinweis fuer leere Kategorien", [] {
        const ui::RolloutProbe* g = gruppe(Str::PrRollout);
        const bool idle = inFenster("Crouch idle", "##aibib") != nullptr;
        const bool walk = inFenster("Crouch walk", "##aibib") != nullptr;
        melde("B15 Ducken: Kurzinfo " + (g != nullptr ? g->kurzinfo : std::string("-")) + ", Crouch idle " + (idle ? "da" : "fehlt") + ", Crouch walk " + (walk ? "da" : "fehlt"));
        return g != nullptr && g->kurzinfo == std::string(tr(Str::AzKatDucken)) + " \xc2\xb7 2" && idle && walk && imRollIn(g, tr(Str::EnPresetsLeer)) == nullptr;
    }));
    add(klick(tr(Str::AiKatStehen), "mtrollouts"));
    add(pause(0.4));
    add(pruefe("ENT3B B15" + m + ": Stehen zeigt Crouch idle nicht mehr", [] { return inFenster("Idle", "##aibib") != nullptr && inFenster("Crouch idle", "##aibib") == nullptr; }));
    // --- B16 ---
    add(klick(tr(Str::AzKatGehen), "mtrollouts"));
    add(pause(0.4));
    add(klickAn("ENT3B B16" + m + ": Gehen (erzeugt) waehlen", [] {
        const Element* e = inFenster(tr(Str::EnGangGehen), "##aibib");
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
    }));
    add(pause(0.6));
    add(pruefe("ENT3B B16" + m + ": Tempo steht als Faktor (1.00x bei 90 Einheiten je Sekunde, dem walkSpeed des Spiels); der Tooltip nennt die Einheiten je Sekunde", [] {
        const Ent3bProbe& p = ent3bProbe();
        const bool frisch = p.tempoBild >= ImGui::GetFrameCount() - 3;
        melde("B16 Gehen: Tempo " + std::to_string(modellRtDaten()->gang.tempo) + ", Anzeige '" + p.tempoText + "', Tooltip '" + p.tempoHinweis + "'");
        return frisch && modellRtDaten()->gang.tempo == 90.0F && p.tempoText == "1.00\xc3\x97" && p.tempoHinweis.find("90") != std::string::npos;
    }));
    auto vorher = std::make_shared<std::vector<std::vector<BoneMatrix>>>();
    add(ap3::insFeld(tr(Str::AzPresetVorschau)));
    add(pause(0.3));
    add(klick(tr(Str::AzPresetVorschau), "mtrollouts"));
    add(pause(1.5));
    add(tu("ENT3B B16" + m + ": die erzeugte Bewegung bei 1.00x merken (Vorschau auf Figur)", [vorher] {
        *vorher = g_app->mtab.rt.welt;
        melde("B16" + std::string(" Vorschau: ") + std::to_string(vorher->size()) + " Bilder");
    }));
    add(ap3::insFeld(tr(Str::PrTempo)));
    add(pause(0.3));
    add(reglerZiehen(tr(Str::PrTempo), "mtrollouts", 0, 0.2F, 0.45F));
    add(pause(0.5));
    add(pruefe("ENT3B B16" + m + ": Tempo ziehen aendert den Faktor; Tempo des Gangs = Faktor x 90, Anzeige und Tooltip folgen", [] {
        const Ent3bProbe& p = ent3bProbe();
        const float tempo = modellRtDaten()->gang.tempo;
        float faktor = -1.0F;
        (void)std::sscanf(p.tempoText.c_str(), "%f", &faktor);
        char einheiten[32];
        std::snprintf(einheiten, sizeof einheiten, "%.0f", static_cast<double>(tempo));
        melde("B16 gezogen: Tempo " + std::to_string(tempo) + ", Anzeige '" + p.tempoText + "', Tooltip '" + p.tempoHinweis + "'");
        return std::fabs(tempo - 90.0F) > 1.0F && std::fabs(faktor * 90.0F - tempo) < 0.5F && p.tempoHinweis.find(einheiten) != std::string::npos;
    }));
    add(tu("ENT3B B16" + m + ": Tempo zurueck auf 1.00x (Rueckgaengig)", [] { (void)modellRueckgaengig(); }));
    add(pause(1.0));
    add(pruefe("ENT3B B16" + m + ": bei gleichem Wert dieselbe Bewegung wie vorher (Tempo 90, jedes Bild gleich)", [vorher] {
        const auto& w = g_app->mtab.rt.welt;
        float d = w.size() == vorher->size() && !w.empty() ? 0.0F : 1e9F;
        for (std::size_t f = 0; f < w.size() && f < vorher->size(); ++f) { d = std::max(d, weltAbstand(w[f], (*vorher)[f])); }
        melde("B16 gleich: Tempo " + std::to_string(modellRtDaten()->gang.tempo) + ", " + std::to_string(w.size()) + " Bilder, groesster Unterschied " + std::to_string(d));
        return modellRtDaten()->gang.tempo == 90.0F && d == 0.0F;
    }));
    if (modus == 0) {
        add(klick(tr(Str::AzKatLaufen), "mtrollouts"));
        add(pause(0.4));
        add(klickAn("ENT3B B16: Laufen (erzeugt) waehlen", [] {
            const Element* e = inFenster(tr(Str::EnGangLaufen), "##aibib");
            return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
        }));
        add(pause(0.6));
        // der Wechsel auf Laufen misst Tempo und Schritt an BOTH_RUN1 der Figur (modellPresetMessen) - der Faktor gilt gegen runSpeed 300
        add(pruefe("ENT3B B16: Laufen (erzeugt) zeigt den Faktor gegen 300 Einheiten je Sekunde (runSpeed), der Tooltip nennt 300 und das Tempo", [] {
            const Ent3bProbe& p = ent3bProbe();
            const float tempo = modellRtDaten()->gang.tempo;
            char soll[32];
            std::snprintf(soll, sizeof soll, "%.2f\xc3\x97", static_cast<double>(tempo / 300.0F));
            char einheiten[32];
            std::snprintf(einheiten, sizeof einheiten, "%.0f", static_cast<double>(tempo));
            melde("B16 Laufen: Tempo " + std::to_string(tempo) + ", Anzeige '" + p.tempoText + "' (soll '" + soll + "'), Tooltip '" + p.tempoHinweis + "'");
            return modellRtDaten()->gang.art == modell::GangArt::Laufen && p.tempoText == soll && p.tempoHinweis.find("300") != std::string::npos &&
                   p.tempoHinweis.find(einheiten) != std::string::npos;
        }));
    }
}

// --- B14: IK/FK ueberblenden ------------------------------------------------------------------------------------------------

void ikfkSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("ENT3B B14: wie Blender, frisches Control Rig (Animation von FK auf IK)", [] {
        modellBedienungSetzen(1);
        rigFrisch();
    }));
    add(pause(1.0));
    add(pruefe("ENT3B B14 Spur: im Dopesheet steht je Glied eine Zeile IK/FK (vier Zeilen, Code der IK/FK-Spur)", [] {
        const modell::RigSchicht* x = sch();
        int zeilen = 0;
        std::string namen;
        for (const Ent3bDsZeile& z : ent3bProbe().dsZeilen) {
            if (modell::ikAnteilSpur(z.code) >= 0) {
                ++zeilen;
                namen += " '" + z.name + "'";
            }
        }
        melde("B14 Spur: " + std::to_string(zeilen) + " Zeilen IK/FK:" + namen + ", Dopesheet-Bild " + std::to_string(ent3bProbe().dsBild) + " (jetzt " + std::to_string(ImGui::GetFrameCount()) + ")");
        return x != nullptr && x->ik.size() == 4U && zeilen == 4 && namen.find("IK/FK") != std::string::npos;
    }));
    auto vorher = std::make_shared<modell::RigSchicht>();
    const auto a = [] { return f0() + 5; };
    const auto b = [] { return f0() + 15; };
    add(tu("ENT3B B14: Schicht merken; wie Blender am Frame +5 den Regler IK/FK auf IK (Schluessel), am Frame +15 auf FK (Schluessel)", [vorher, a, b] {
        if (sch() != nullptr) { *vorher = *sch(); }
        const int arm = static_cast<int>(modell::IkArt::ArmRechts);
        g_app->animFrame = a();
        (void)modellIkReglerSetzen(arm, 0.0F, false);
        g_app->animFrame = b();
        (void)modellIkReglerSetzen(arm, 1.0F, false);
    }));
    add(pause(0.4));
    add(pruefe("ENT3B B14 Ueberblenden: zwischen den Schluesseln (+5 IK, +15 FK) blendet der Anteil ueber wie CATs Bezier-Spur: +5 = 1, +10 dazwischen, +15 = 0, davor 1, danach 0", [a, b] {
        const int sp = spurIndex(modell::IkArt::ArmRechts);
        if (sp < 0) { return false; }
        const modell::IkSpur& x = sch()->ik[static_cast<std::size_t>(sp)];
        const float ra = modell::ikAnteilAm(x, static_cast<float>(a()));
        const float rm = modell::ikAnteilAm(x, static_cast<float>(f0() + 10));
        const float rb = modell::ikAnteilAm(x, static_cast<float>(b()));
        const float rv = modell::ikAnteilAm(x, static_cast<float>(a() - 2));
        const float rn = modell::ikAnteilAm(x, static_cast<float>(b() + 3));
        char z[200];
        std::snprintf(z, sizeof z, "B14 Anteil: +3 %.3f, +5 %.3f, +10 %.3f, +15 %.3f, +18 %.3f, Schluessel der Spur %d", static_cast<double>(rv), static_cast<double>(ra), static_cast<double>(rm),
                      static_cast<double>(rb), static_cast<double>(rn), static_cast<int>(x.anteil.size()));
        melde(z);
        return ra == 1.0F && rb == 0.0F && rm > 0.05F && rm < 0.95F && rv == 1.0F && rn == 0.0F;
    }));
    // an den Frames sind IK und FK nach Control Rig erstellen gleich (ENT2: 0,00002); zwischen zwei Frames interpolieren FK-Schluessel und
    // IK-Ziel verschieden (gemessen in ent3b_rot bis 0,03) - dort mischt die Ueberblendung beide stetig
    add(pruefe("ENT3B B14 kein Sprung: IK und FK sind angeglichen (Control Rig erstellen) - an jedem Frame von +4 bis +16 die Pose wie vor dem Ueberblenden (< 0,0001, wie ENT2)", [vorher, a, b] {
        const modell::RigSchicht* x = sch();
        if (x == nullptr || vorher->ik.empty()) { return false; }
        float d = 0.0F;
        for (int f = a() - 1; f <= b() + 1; ++f) { d = std::max(d, weltAbstand(weltAm(*x, static_cast<float>(f)), weltAm(*vorher, static_cast<float>(f)))); }
        melde("B14 kein Sprung (angeglichen): groesster Unterschied " + std::to_string(d));
        return d < 0.0001F;
    }));
    // nicht angeglichen: die IK-Ziele des Arms von +5 bis +15 um 6 Einheiten hoch (ohne die FK-Schluessel) - die Ueberblendung fuehrt
    // dann von der IK-Pose (+5) stetig zur FK-Pose (+15)
    auto nurIk = std::make_shared<modell::RigSchicht>();
    auto nurFk = std::make_shared<modell::RigSchicht>();
    add(tu("ENT3B B14: IK-Ziele des Arms R von +5 bis +15 um 6 Einheiten hoch (FK bleibt) - IK und FK nicht mehr gleich", [nurIk, nurFk, a, b] {
        const int sp = spurIndex(modell::IkArt::ArmRechts);
        if (sp < 0) { return; }
        g_app->mtab.dok.aendern("ENT3B Ziele hoch", [&](modell::Inhalt& in) {
            modell::RigSchicht& x = in.rig.schichten[seq()->name];
            modell::IkSpur& spur = x.ik[static_cast<std::size_t>(sp)];
            for (modell::RigSchluessel& k : spur.ziel) {
                if (k.bild >= a() && k.bild <= b()) { k.t[2] += 6.0F; }
            }
            modell::ikHenkelNeu(spur);
        }, false);
        *nurIk = *sch();
        *nurFk = *sch();
        nurIk->ik[static_cast<std::size_t>(sp)].anteil.clear();
        nurFk->ik[static_cast<std::size_t>(sp)].anteil.clear();
        for (modell::IkWert& w : nurIk->ik[static_cast<std::size_t>(sp)].wert) { w.ik = 1.0F; }
        for (modell::IkWert& w : nurFk->ik[static_cast<std::size_t>(sp)].wert) { w.ik = 0.0F; }
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(pruefe("ENT3B B14 kein Sprung beim Ueberblenden: +5 ist die IK-Pose, +15 die FK-Pose (je < 0,0001); dazwischen aendert sich die Pose je Zehntel-Frame hoechstens um ein Viertel des Abstands IK/FK", [nurIk, nurFk, a, b] {
        const modell::RigSchicht* x = sch();
        const int sp = spurIndex(modell::IkArt::ArmRechts);
        if (x == nullptr || sp < 0 || nurIk->ik.empty()) { return false; }
        const int hand = x->ik[static_cast<std::size_t>(sp)].ende.knochen;
        const float anfang = weltAbstand(weltAm(*x, static_cast<float>(a())), weltAm(*nurIk, static_cast<float>(a())));
        const float ende = weltAbstand(weltAm(*x, static_cast<float>(b())), weltAm(*nurFk, static_cast<float>(b())));
        const float abstandIkFk = ortAbstand(weltAm(*nurIk, static_cast<float>(f0() + 10))[static_cast<std::size_t>(hand)], weltAm(*nurFk, static_cast<float>(f0() + 10))[static_cast<std::size_t>(hand)]);
        float schritt = 0.0F;
        std::vector<BoneMatrix> vor = weltAm(*x, static_cast<float>(a()));
        for (int i = 1; i <= (b() - a()) * 10; ++i) {
            const std::vector<BoneMatrix> w = weltAm(*x, static_cast<float>(a()) + static_cast<float>(i) * 0.1F);
            schritt = std::max(schritt, ortAbstand(w[static_cast<std::size_t>(hand)], vor[static_cast<std::size_t>(hand)]));
            vor = w;
        }
        char z[220];
        std::snprintf(z, sizeof z, "B14 Ueberblenden (nicht angeglichen): +5 gegen IK %.6f, +15 gegen FK %.6f, Hand IK/FK %.3f, groesster Schritt je Zehntel-Frame %.4f", static_cast<double>(anfang),
                      static_cast<double>(ende), static_cast<double>(abstandIkFk), static_cast<double>(schritt));
        melde(z);
        return anfang < 0.0001F && ende < 0.0001F && abstandIkFk > 4.0F && schritt < abstandIkFk * 0.25F;
    }));
    add(tu("ENT3B B14: die Ziele wieder herunter (Rueckgaengig)", [] { (void)modellRueckgaengig(); }));
    // --- Dopesheet: Schluessel der Spur IK/FK ziehen und loeschen ---
    add(tu("ENT3B B14: Frame +10, nichts gewaehlt, die Glieder im Dopesheet zugeklappt (die Zeilen IK/FK stehen unter allen Knochen)", [] {
        g_app->animFrame = f0() + 10;
        g_app->mtab.kvWahl.clear();
        for (auto& z : g_app->mtab.kvGliedZu) { z = 1; }
        g_app->mtab.kvScrollY = 0.0F;
    }));
    add(pause(0.5));
    add(tu("ENT3B B14: das Dopesheet so rollen, dass die Zeile IK/FK Arm R zu sehen ist", [] {
        const int sp = spurIndex(modell::IkArt::ArmRechts);
        const Ent3bDsZeile* z = sp >= 0 ? dsZeile(modell::ikAnteilCode(sp)) : nullptr;
        const Ent3bProbe& p = ent3bProbe();
        if (z != nullptr) { g_app->mtab.kvScrollY = std::max(0.0F, g_app->mtab.kvScrollY + z->y - (p.dsOben + (p.dsUnten - p.dsOben) * 0.4F)); }
        melde("B14 Dopesheet: Zeile y " + std::to_string(z != nullptr ? z->y : -1.0F) + ", sichtbar " + std::to_string(p.dsOben) + " bis " + std::to_string(p.dsUnten) +
              ", gerollt " + std::to_string(g_app->mtab.kvScrollY));
    }));
    add(pause(0.4));
    add(klickAn("ENT3B B14: im Dopesheet auf den Schluessel der Zeile IK/FK Arm R an Frame +15", [b] {
        const int sp = spurIndex(modell::IkArt::ArmRechts);
        const Ent3bDsZeile* z = sp >= 0 ? dsZeile(modell::ikAnteilCode(sp)) : nullptr;
        melde("B14 Klick: x " + std::to_string(modellKvBildAufSchirm(static_cast<float>(b()))) + " y " + std::to_string(z != nullptr ? z->y : -1.0F));
        return z != nullptr ? ImVec2{modellKvBildAufSchirm(static_cast<float>(b())), z->y} : ImVec2{-1.0F, -1.0F};
    }));
    add(pause(0.4));
    add(pruefe("ENT3B B14 Dopesheet: ein Klick auf die Raute waehlt den Schluessel der IK/FK-Spur (nur ihn)", [b] {
        const int sp = spurIndex(modell::IkArt::ArmRechts);
        const auto& w = g_app->mtab.kvWahl;
        melde("B14 Wahl: " + std::to_string(w.size()) + " Schluessel" + (w.empty() ? std::string() : ", erster Code " + std::to_string(w[0].knochen) + " Bild " + std::to_string(w[0].bild)));
        return sp >= 0 && w.size() == 1U && w[0].knochen == modell::ikAnteilCode(sp) && w[0].bild == b();
    }));
    add(mb3::ziehenVonNach("ENT3B B14: im Dopesheet die Raute IK/FK Arm R von +15 um 3 Frames nach rechts", [b] {
        const int sp = spurIndex(modell::IkArt::ArmRechts);
        const Ent3bDsZeile* z = sp >= 0 ? dsZeile(modell::ikAnteilCode(sp)) : nullptr;
        return z != nullptr ? ImVec2{modellKvBildAufSchirm(static_cast<float>(b())), z->y} : ImVec2{-1.0F, -1.0F};
    }, [b] {
        const int sp = spurIndex(modell::IkArt::ArmRechts);
        const Ent3bDsZeile* z = sp >= 0 ? dsZeile(modell::ikAnteilCode(sp)) : nullptr;
        return z != nullptr ? ImVec2{modellKvBildAufSchirm(static_cast<float>(b() + 3)), z->y} : ImVec2{-1.0F, -1.0F};
    }));
    add(pause(0.4));
    add(pruefe("ENT3B B14 Dopesheet: der Schluessel FK steht jetzt an +18 - die Ueberblendung reicht von +5 bis +18 (Anteil an +15 zwischen 0 und 1)", [b] {
        const int sp = spurIndex(modell::IkArt::ArmRechts);
        if (sp < 0) { return false; }
        const modell::IkSpur& x = sch()->ik[static_cast<std::size_t>(sp)];
        bool bei18 = false;
        bool bei15 = false;
        for (const modell::RigSchluessel& k : x.anteil) {
            bei18 = bei18 || k.bild == b() + 3;
            bei15 = bei15 || k.bild == b();
        }
        const float r15 = modell::ikAnteilAm(x, static_cast<float>(b()));
        melde("B14 verschoben: Schluessel an +18 " + std::string(bei18 ? "ja" : "nein") + ", an +15 " + (bei15 ? "ja" : "nein") + ", Anteil an +15 " + std::to_string(r15));
        return bei18 && !bei15 && r15 > 0.0F && r15 < 1.0F && modell::ikAnteilAm(x, static_cast<float>(b() + 3)) == 0.0F;
    }));
    add(tu("ENT3B B14: Loeschen (Dopesheet, X) - den gewaehlten Schluessel der Spur", [] {
        const int i = modellKurvenAktionNach(static_cast<int>(Str::MbKvLoeschen));
        if (i >= 0) { modellKurvenAktionAusloesen(i); }
    }));
    add(pause(0.3));
    add(pruefe("ENT3B B14 Dopesheet: geloescht - die Spur hat keinen FK-Schluessel mehr, der Arm bleibt bis zum Ende IK; Rueckgaengig bringt ihn", [b] {
        const int sp = spurIndex(modell::IkArt::ArmRechts);
        if (sp < 0) { return false; }
        const modell::IkSpur& x = sch()->ik[static_cast<std::size_t>(sp)];
        const float rEnde = modell::ikAnteilAm(x, static_cast<float>(fEnde()));
        (void)modellRueckgaengig();
        const float rDanach = modell::ikAnteilAm(sch()->ik[static_cast<std::size_t>(sp)], static_cast<float>(b() + 3));
        melde("B14 geloescht: Anteil am Ende " + std::to_string(rEnde) + ", nach Rueckgaengig an +18 " + std::to_string(rDanach));
        return rEnde == 1.0F && rDanach == 0.0F;
    }));
    add(tu("ENT3B B14: Glieder im Dopesheet wieder auf", [] {
        for (auto& z : g_app->mtab.kvGliedZu) { z = 0; }
    }));
    // --- wie 3ds Max: der Regler IK/FK des gewaehlten Glieds (CAT Limb Animation) ---
    add(tu("ENT3B B14: wie 3ds Max, Glied Arm L waehlen, Frame +20", [] {
        modellBedienungSetzen(0);
        g_app->mtab.ikWahl = static_cast<int>(modell::IkArt::ArmLinks);
        g_app->animFrame = f0() + 20;
    }));
    add(pause(0.5));
    add(zeige(Str::MbIkRollout));
    add(pause(0.4));
    add(pruefe("ENT3B B14 wie 3ds Max: unter dem Glied steht der Regler IK/FK-Verhaeltnis (wie CAT: 0 = IK, 1 = FK) mit Tooltip", [] {
        return imRollIn(gruppe(Str::MbIkRollout), tr(Str::E3IkFkAnteil)) != nullptr;
    }));
    add(reglerZiehen(tr(Str::E3IkFkAnteil), "mtrollouts", 0, 0.0F, 0.6F));
    add(pause(0.4));
    add(pruefe("ENT3B B14 wie 3ds Max: der Regler setzt einen Schluessel der IK/FK-Spur am Frame +20 (Anteil IK unter 1)", [] {
        const int sp = spurIndex(modell::IkArt::ArmLinks);
        if (sp < 0) { return false; }
        const modell::IkSpur& x = sch()->ik[static_cast<std::size_t>(sp)];
        bool schluessel = false;
        for (const modell::RigSchluessel& k : x.anteil) { schluessel = schluessel || k.bild == f0() + 20; }
        const float r = modell::ikAnteilAm(x, static_cast<float>(f0() + 20));
        melde("B14 Max-Regler: Schluessel an +20 " + std::string(schluessel ? "ja" : "nein") + ", Anteil " + std::to_string(r));
        return schluessel && r < 0.9F;
    }));
}

// --- C1: nach nur FK wieder Control Rig ---------------------------------------------------------------------------------------

void c1Schritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto fkSchicht = std::make_shared<modell::RigSchicht>();
    add(tu("ENT3B C1: wie 3ds Max, frisches Control Rig, dann Animation-Rig (nur FK)", [fkSchicht] {
        modellBedienungSetzen(0);
        rigFrisch();
        (void)modellAnimRigErstellen(true);
        if (sch() != nullptr) { *fkSchicht = *sch(); }
    }));
    add(pause(0.6));
    add(zeige(Str::AzRRig));
    add(pause(0.3));
    add(ap3::insFeld(tr(Str::AzRigErstellen)));
    add(klick(tr(Str::AzRigErstellen), "mtrollouts"));
    add(pause(1.0));
    add(pruefe("ENT3B C1: Control Rig erstellen nach nur FK - vier Glieder wieder auf IK, an jedem Bild ein IK-Schluessel, Anteil 1 an jedem Bild (wie beim ersten Erstellen)", [] {
        const modell::RigSchicht* x = sch();
        if (x == nullptr || seq() == nullptr) { return false; }
        int jedes = 0;
        float minAnteil = 1.0F;
        for (const modell::IkSpur& sp : x->ik) {
            jedes += static_cast<int>(sp.ziel.size()) == seq()->numFrames ? 1 : 0;
            for (int f = f0(); f <= fEnde(); ++f) { minAnteil = std::min(minAnteil, modell::ikAnteilAm(sp, static_cast<float>(f))); }
        }
        melde("C1: nurFk " + std::to_string(modellRigNurFk() ? 1 : 0) + ", " + std::to_string(x->ik.size()) + " Spuren, " + std::to_string(jedes) + " mit Schluessel je Bild, kleinster Anteil " +
              std::to_string(minAnteil));
        return !modellRigNurFk() && x->ik.size() == 4U && jedes == 4 && minAnteil == 1.0F;
    }));
    add(pruefe("ENT3B C1: die Bewegung bleibt - je Glied gegen die Schicht nur mit FK (< 1/16 wie ENT2), die IK allein gegen dieselben Schluessel als FK (< 0,0001)", [fkSchicht] {
        const modell::RigSchicht* x = sch();
        if (x == nullptr || x->ik.size() != 4U) { return false; }
        modell::RigSchicht alsFk = *x;
        for (modell::IkSpur& sp : alsFk.ik) {
            sp.anteil.clear();
            for (modell::IkWert& w : sp.wert) { w.ik = 0.0F; }
        }
        const std::vector<float> ges = modell::ikGliedAbweichung(g_app->anim, *fkSchicht, *x, f0(), fEnde());
        const std::vector<float> ik = modell::ikGliedAbweichung(g_app->anim, alsFk, *x, f0(), fEnde());
        float g = 0.0F;
        float i = 0.0F;
        for (std::size_t k = 0; k < ges.size(); ++k) {
            g = std::max(g, ges[k]);
            i = std::max(i, ik[k]);
        }
        melde("C1 Abweichung: gesamt " + std::to_string(g) + ", nur IK " + std::to_string(i));
        return g < 0.0625F && i < 0.0001F;
    }));
    add(tu("ENT3B C1: Strg+Z", [] { (void)modellRueckgaengig(); }));
    add(pause(0.5));
    add(pruefe("ENT3B C1: ein Rueckgaengig nimmt alles zurueck - wieder nur FK, keine IK-Spur, dieselben Schluessel", [fkSchicht] {
        const modell::RigSchicht* x = sch();
        float d = 0.0F;
        if (x != nullptr) {
            for (int f = f0(); f <= fEnde(); ++f) { d = std::max(d, weltAbstand(weltBei(*x, f, f, 0.0F), weltBei(*fkSchicht, f, f, 0.0F))); }
        }
        melde("C1 Rueckgaengig: nurFk " + std::to_string(modellRigNurFk() ? 1 : 0) + ", Spuren " + std::to_string(x != nullptr ? x->ik.size() : 99) + ", Unterschied " + std::to_string(d));
        return x != nullptr && modellRigNurFk() && x->ik.empty() && x->schluesselZahl() == fkSchicht->schluesselZahl() && d == 0.0F;
    }));
    add(tu("ENT3B C1: Strg+Y", [] { (void)modellWiederholen(); }));
    add(pause(0.5));
    add(pruefe("ENT3B C1: Wiederholen - wieder IK an vier Gliedern", [] { return sch() != nullptr && !modellRigNurFk() && sch()->ik.size() == 4U; }));
}

// --- B17: der Bodenkreis ist der Griff der Wurzel -----------------------------------------------------------------------------

void wurzelSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("ENT3B B17: wie 3ds Max, frisches Control Rig, Stil Control Rig, Knochen aus, Verschieben (W)", [] {
        modellBedienungSetzen(0);
        // TEST1411 N: Bewegen und Drehen setzen Schluessel - wie CAT nur im Animationsmodus; seit dem Modusschalter stand der Test nach
        // zeige(AzRRig) oben im Setup-Modus, dort wirkt kein Zug (die Setup-Pose ist statisch)
        modellAnimSetupSetzen(false);
        rigFrisch();
        ModellTab& t = g_app->mtab;
        t.vierAnsichten = false;
        t.aktiv = 3;
        g_app->modellKnochen = false;
        modellRigStilSetzen(modell::RigStil::ControlRig);
        t.gizmo = 0;
    }));
    add(pause(1.0));
    add(pruefe("ENT3B B17: das Rig hat den Griff der Wurzel; der Bodenkreis ist seine Form (wählbar), die Marke ueber dem Kopf bleibt Schmuck", [] {
        const int w = wurzelGriff();
        int boden = 0;
        int kopfSchmuck = 0;
        for (int i = 0; i < modellKdFormZahl(); ++i) {
            const KdFormInfo f = modellKdFormInfo(i);
            if (w >= 0 && f.griff == w && !f.schmuck && f.gestalt == static_cast<int>(KdGestalt::Stil) + static_cast<int>(modell::FormGestalt::Ring)) { ++boden; }
            if (f.schmuck && f.griff < 0) { ++kopfSchmuck; }
        }
        melde("B17: Wurzel-Griff " + std::to_string(w) + ", Bodenkreise als Griff " + std::to_string(boden) + ", Schmuck ohne Griff " + std::to_string(kopfSchmuck));
        return w >= 0 && boden == 1 && kopfSchmuck == 1;
    }));
    add(tu("ENT3B B17: nichts gewaehlt", [] {
        modellRigGriffWaehlen(-1);
        g_app->mtab.knochenWahl = -1;
    }));
    add(pause(0.3));
    add(klickAn("ENT3B B17: auf den Bodenkreis", [] {
        ImVec2 p{-50.0F, -50.0F};
        const int w = wurzelGriff();
        for (int i = 0; w >= 0 && i < modellKdFormZahl(); ++i) {
            if (modellKdFormInfo(i).griff == w && modellKdFormPunkt(i, g_app->mtab.aktiv, p)) { break; }
        }
        return p;
    }));
    add(pause(0.5));
    add(pruefe("ENT3B B17: ein Klick auf den Bodenkreis waehlt den Griff der Wurzel", [] {
        melde("B17 Klick: gewaehlt " + std::to_string(g_app->mtab.griffWahl) + ", soll " + std::to_string(wurzelGriff()));
        return wurzelGriff() >= 0 && g_app->mtab.griffWahl == wurzelGriff();
    }));
    auto vorher = std::make_shared<modell::RigSchicht>();
    add(tu("ENT3B B17: Schicht merken; den Griff der Wurzel um (12, -5, 0) verschieben (wie das Gizmo, ein Zug)", [vorher] {
        if (sch() != nullptr) { *vorher = *sch(); }
        g_app->animFrame = f0() + 4;
        const float weg[3] = {12.0F, -5.0F, 0.0F};
        modellRigZiehen(1, weg);
        modellRigZiehenEnde();
    }));
    add(pause(0.4));
    add(pruefe("ENT3B B17 Verschieben: die ganze Figur (jeder Knochen, auch Haende und Fuesse an ihren IK-Zielen) steht um (12, -5, 0) weiter - am Frame und an jedem anderen; ein Schluessel der Plattform", [vorher] {
        const modell::RigSchicht* x = sch();
        if (x == nullptr || vorher->ik.empty()) { return false; }
        float fehler = 0.0F;
        for (const int f : {f0(), f0() + 4, f0() + 11, fEnde()}) {
            const std::vector<BoneMatrix> a = weltBei(*vorher, f, f, 0.0F);
            const std::vector<BoneMatrix> n = weltBei(*x, f, f, 0.0F);
            for (std::size_t k = 0; k < a.size() && k < n.size(); ++k) {
                fehler = std::max({fehler, std::fabs(n[k].m[0][3] - a[k].m[0][3] - 12.0F), std::fabs(n[k].m[1][3] - a[k].m[1][3] + 5.0F), std::fabs(n[k].m[2][3] - a[k].m[2][3])});
            }
            if (a.size() != n.size() || a.empty()) { fehler = 1e9F; }
        }
        melde("B17 Verschieben: Abweichung vom Weg " + std::to_string(fehler) + ", Schluessel der Plattform " + std::to_string(x->plattform.size()));
        return fehler < 0.001F && x->plattform.size() == 1U;
    }));
    add(tu("ENT3B B17: den Griff der Wurzel um 90 Grad um die Hochachse drehen (Drehen, E)", [] {
        g_app->mtab.gizmo = 2;
        const float z[3] = {0.0F, 0.0F, 1.0F};
        modellRigDrehen(modell::drehungUm(z, 90.0F));
        modellRigZiehenEnde();
    }));
    add(pause(0.4));
    add(pruefe("ENT3B B17 Drehen: die Figur dreht um die Mitte des Bodenkreises (jeder Knochen um 90 Grad um die Hochachse durch die Mitte, auch die IK-Ziele)", [vorher] {
        const modell::RigSchicht* x = sch();
        const int w = wurzelGriff();
        if (x == nullptr || w < 0 || vorher->ik.empty()) { return false; }
        const modell::Griff& g = g_app->mtab.rig.griffe[static_cast<std::size_t>(w)];
        const float mx = g.ort[0] + 12.0F;
        const float my = g.ort[1] - 5.0F;
        float fehler = 0.0F;
        for (const int f : {f0() + 4, f0() + 9}) {
            const std::vector<BoneMatrix> a = weltBei(*vorher, f, f, 0.0F);
            const std::vector<BoneMatrix> n = weltBei(*x, f, f, 0.0F);
            for (std::size_t k = 0; k < a.size() && k < n.size(); ++k) {
                // vorher verschoben um (12, -5), dann um (mx, my) um 90 Grad: (x, y) -> (mx - (y - my), my + (x - mx))
                const float px = a[k].m[0][3] + 12.0F;
                const float py = a[k].m[1][3] - 5.0F;
                const float sx = mx - (py - my);
                const float sy = my + (px - mx);
                fehler = std::max({fehler, std::fabs(n[k].m[0][3] - sx), std::fabs(n[k].m[1][3] - sy), std::fabs(n[k].m[2][3] - a[k].m[2][3])});
            }
        }
        melde("B17 Drehen: Abweichung " + std::to_string(fehler));
        return fehler < 0.001F;
    }));
    add(pruefe("ENT3B B17: die Plattform steht als Zeile im Dopesheet", [] { return dsZeile(modell::kPlattformWahl) != nullptr; }));
    add(tu("ENT3B B17: zweimal Strg+Z", [] {
        (void)modellRueckgaengig();
        (void)modellRueckgaengig();
    }));
    add(pause(0.4));
    add(pruefe("ENT3B B17 Rueckgaengig: die Figur steht wieder wie vorher (jeder Knochen, jedes Bild < 0,0001), keine Plattform", [vorher] {
        const modell::RigSchicht* x = sch();
        float d = 0.0F;
        for (int f = f0(); x != nullptr && f <= fEnde(); ++f) { d = std::max(d, weltAbstand(weltBei(*x, f, f, 0.0F), weltBei(*vorher, f, f, 0.0F))); }
        melde("B17 Rueckgaengig: Unterschied " + std::to_string(d) + ", Plattform " + std::to_string(x != nullptr ? x->plattform.size() : 99));
        return x != nullptr && d < 0.0001F && x->plattform.empty();
    }));
    add(tu("ENT3B B17: Strg+Y", [] { (void)modellWiederholen(); }));
    add(pause(0.3));
    add(pruefe("ENT3B B17 Wiederholen: verschoben wie vorher (ein Schluessel der Plattform)", [] { return sch() != nullptr && sch()->plattform.size() == 1U; }));
    add(tu("ENT3B B17: wie Blender, der Griff der Wurzel gewaehlt", [] {
        modellBedienungSetzen(1);
        modellRigGriffWaehlen(wurzelGriff());
    }));
    add(pause(0.4));
    add(pruefe("ENT3B B17 wie Blender: G greift den Griff der Wurzel (Verschieben), R dreht ihn - beide als Zug, nicht abgewiesen", [] {
        std::string g1;
        std::string g2;
        const bool g = modellRigZugMoeglich(1, &g1);
        const bool r = modellRigZugMoeglich(2, &g2);
        melde("B17 Blender: G " + std::string(g ? "ja" : "nein (" + g1 + ")") + ", R " + (r ? "ja" : "nein (" + g2 + ")"));
        return wurzelGriff() >= 0 && g_app->mtab.griffWahl == wurzelGriff() && g && r;
    }));
}

// --- C3: Rig-Vorlage im Dokument ----------------------------------------------------------------------------------------------

void c3Schritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("ENT3B C3: wie Blender, frisches Control Rig, Vorlage automatisch", [] {
        modellBedienungSetzen(1);
        rigFrisch();
        modellAnimVorlageSetzen(-2);
    }));
    add(pause(0.5));
    add(tu("ENT3B C3: Vorlage Eigenes Skelett", [] { modellAnimVorlageSetzen(-1); }));
    add(pause(0.5));
    add(pruefe("ENT3B C3: die Wahl steht im Dokument (Eigenes Skelett) und das Rig gilt fuer sie", [] {
        melde("C3 gewaehlt: Dokument " + std::to_string(g_app->mtab.dok.inhalt().rig.vorlage) + ", Wahl " + std::to_string(modellAnimVorlageWahl()) + ", Rueckgaengig '" +
              g_app->mtab.dok.rueckgaengigName() + "'");
        return g_app->mtab.dok.inhalt().rig.vorlage == -1 && modellAnimVorlageWahl() == -1 && modellRigDa();
    }));
    add(tu("ENT3B C3: Strg+Z", [] { (void)modellRueckgaengig(); }));
    add(pause(0.6));
    add(pruefe("ENT3B C3 Rueckgaengig: die Vorlage ist wieder automatisch, das Rig wieder angelegt", [] {
        melde("C3 Rueckgaengig: Dokument " + std::to_string(g_app->mtab.dok.inhalt().rig.vorlage) + ", Wahl " + std::to_string(modellAnimVorlageWahl()) + ", Rig " +
              (modellRigDa() ? "da" : "weg"));
        return g_app->mtab.dok.inhalt().rig.vorlage == -2 && modellAnimVorlageWahl() == -2 && modellRigDa();
    }));
    add(tu("ENT3B C3: Strg+Y", [] { (void)modellWiederholen(); }));
    add(pause(0.6));
    add(pruefe("ENT3B C3 Wiederholen: wieder Eigenes Skelett", [] { return g_app->mtab.dok.inhalt().rig.vorlage == -1 && modellAnimVorlageWahl() == -1 && modellRigDa(); }));
    add(tu("ENT3B C3: Vorlage zurueck auf automatisch", [] { modellAnimVorlageSetzen(-2); }));
}

// --- Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ent3b): h_ent3b_ikfk, h_ent3b_wurzel ------------------------------------

std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    auto einst = std::make_shared<Settings>();
    add(tu("ENT3B Handbuch: wie Blender, Bereich Animation, Control Rig, Arm R von IK (+5) nach FK (+15) ueberblendet", [=] {
        *einst = g_app->settings;
        g_app->settings.splitModel = 0.62F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(1);
        ModellTab& t = g_app->mtab;
        t.vierAnsichten = false;
        t.aktiv = 3;
        rigFrisch();
        g_app->settings.modellRigStil = static_cast<int>(modell::RigStil::ControlRig);
        const int arm = static_cast<int>(modell::IkArt::ArmRechts);
        g_app->animFrame = f0() + 5;
        (void)modellIkReglerSetzen(arm, 0.0F, false);
        g_app->animFrame = f0() + 15;
        (void)modellIkReglerSetzen(arm, 1.0F, false);
        g_app->animFrame = f0() + 10;
        t.ikWahl = arm;
        t.kvEinpassen = true;
        // im Dopesheet die Glieder zu: die Zeilen IK/FK stehen unter den Knochen; der Schluessel FK des Arms R gewaehlt
        for (auto& z : t.kvGliedZu) { z = 1; }
        t.kvScrollY = 0.0F;
        const int sp = spurIndex(modell::IkArt::ArmRechts);
        t.kvWahl.clear();
        if (sp >= 0) { t.kvWahl.push_back({modell::ikAnteilCode(sp), f0() + 15}); }
        g_app->modelDirty = true;
    }));
    add(pause(1.5));
    add(ap10::zeige(Str::MbIkRollout));
    add(pause(0.5));
    add(tu("ENT3B Handbuch: das Dopesheet so rollen, dass die Zeilen IK/FK zu sehen sind", [] {
        const int sp = spurIndex(modell::IkArt::ArmLinks);
        const Ent3bDsZeile* z = sp >= 0 ? dsZeile(modell::ikAnteilCode(sp)) : nullptr;
        const Ent3bProbe& p = ent3bProbe();
        if (z != nullptr) { g_app->mtab.kvScrollY = std::max(0.0F, g_app->mtab.kvScrollY + z->y - (p.dsOben + (p.dsUnten - p.dsOben) * 0.15F)); }
    }));
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_ent3b_ikfk"));
    add(tu("ENT3B Handbuch: wie 3ds Max, Stil Control Rig, Knochen aus, der Griff der Wurzel gewaehlt und verschoben (W)", [] {
        modellBedienungSetzen(0);
        ModellTab& t = g_app->mtab;
        t.vierAnsichten = false;
        t.aktiv = 3;
        g_app->modellKnochen = false;
        g_app->settings.modellRigStil = static_cast<int>(modell::RigStil::ControlRig);
        for (auto& z : t.kvGliedZu) { z = 0; }
        t.kvWahl.clear();
        g_app->animFrame = f0() + 4;
        g_app->modelOrbit.turn(g_app->modelCam, 30.0F, 18.0F);   // von schraeg oben: der Bodenkreis ganz im Bild
        g_app->modelOrbit.zoom(g_app->modelCam, -2.0F);
        modellRigGriffWaehlen(wurzelGriff());
        t.gizmo = 1;
        const float weg[3] = {-8.0F, 4.0F, 0.0F};
        modellRigZiehen(1, weg);
        modellRigZiehenEnde();
        g_app->modelDirty = true;
        setzeMaus(ImVec2{-50.0F, -50.0F});
    }));
    add(pause(1.5));
    add(hilfe2::schrittFoto("h_ent3b_wurzel"));
    add(tu("ENT3B Handbuch: zurueck", [=] {
        modellBedienungSetzen(einst->modellBedienung);
        g_app->settings.modellRigStil = einst->modellRigStil;
        g_app->settings.splitModel = einst->splitModel;
        g_app->modellKnochen = true;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "ent3b") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_ENT3B_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    auto skala = std::make_shared<float>(1.0F);
    auto einst = std::make_shared<Settings>();
    add(tu("ENT3B: Fenster, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ent3b.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    add(tu("ENT3B: Aufteilung wie neue Einstellungen, Einstellungen merken", [=] {
        *einst = g_app->settings;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        g_app->mtab.bereich = ModellBereich::Animation;
        g_app->mtab.vierAnsichten = false;
        animWaehlen(mbefoto::sequenz(), false);
        melde(g_app->modelPath + ", " + std::to_string(g_app->anim.bones.size()) + " Knochen, Sequenz " + (seq() != nullptr ? seq()->name : std::string("-")));
    }));
    add(pause(1.0));
    add(klickAn("ENT3B: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    presetSchritte(s, 0);
    presetSchritte(s, 1);
    ikfkSchritte(s);
    c1Schritte(s);
    wurzelSchritte(s);
    c3Schritte(s);
    add(tu("ENT3B: zurueck", [=] {
        modellBedienungSetzen(einst->modellBedienung);
        g_app->settings.modellRigStil = einst->modellRigStil;
        g_app->settings.splitModel = einst->splitModel;
        g_app->settings.modellFeldBreite = einst->modellFeldBreite;
        g_app->modellKnochen = true;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->uiScale = *skala;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.3));
    return s;
}

}  // namespace ent3b
