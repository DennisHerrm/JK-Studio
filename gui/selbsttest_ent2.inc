// selbsttest_ent2.inc - ENTSCHEID2 (12.11., verbindliche Entscheidungen von shank zu AP10): Control Rig wie CAT, die acht
// Preset-Kategorien des Entwurfs, Generator-Gaenge in der Preset-Liste (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ent2)
//
//   1  Control Rig erstellen legt eine Animation der Sequenz von FK auf IK auf das Rig (jedes Bild ein Schluessel, Anteil 1),
//      die Bewegung bleibt (Abweichung je Glied ueber die ganze Sequenz gemessen); danach die Griffe wie CAT: Hand-/Fuss-IK-Ziel,
//      Pol, Fussrolle an mehreren Bildern - nur dort aendert sich die Pose; IK/FK je Glied ab dem Bild ohne Sprung.
//   2  die acht Kategorien des Entwurfs in seiner Folge, auch leer (mit Hinweis), dahinter Sonstiges aus presets.txt
//   3  Gehen (erzeugt) / Laufen (erzeugt) als Eintraege der Liste; gewaehlt stehen Tempo und Armschwung unter der Liste, Vorschau
//      und Hauptknopf wirken wie bei Presets; Generator ... zeigt den Rest, Rechtsklick auf den Eintrag die Befehle
//   BHED_ENT2_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe wie ap10)
// Eingebunden in selbsttest.cpp (nach selbsttest_ap10.inc, benutzt dessen Helfer).
namespace ent2 {

using ap10::gruppe;
using ap10::imRoll;
using ap10::imRollIn;
using ap10::inGruppe;
using ap10::zeige;
using mbefoto::inFenster;
using mbefoto::kindFenster;

void melde(const std::string& z) { diag::detail("Knopftest: ENT2 " + z); }

const AnimEntry* seq() { return ap10::sequenzJetzt(); }
int f0() { return seq() != nullptr ? seq()->firstFrame : 0; }
int fEnde() { return seq() != nullptr ? seq()->firstFrame + std::max(seq()->numFrames, 1) - 1 : 0; }

const modell::RigSchicht* sch() { return modellRigSchichtJetzt(); }

const modell::IkSpur* spur(modell::IkArt a) {
    const modell::RigSchicht* s = sch();
    const int i = s != nullptr ? modell::ikSpurIndex(*s, a) : -1;
    return i >= 0 ? &s->ik[static_cast<std::size_t>(i)] : nullptr;
}

std::vector<BoneMatrix> welt(const modell::RigSchicht& s, int bild) {
    std::vector<BoneMatrix> w;
    modell::rigWelt(g_app->anim, s, bild, bild, 0.0F, w);
    return w;
}

float ortAbstand(const BoneMatrix& a, const float b[3]) {
    return std::sqrt((a.m[0][3] - b[0]) * (a.m[0][3] - b[0]) + (a.m[1][3] - b[1]) * (a.m[1][3] - b[1]) + (a.m[2][3] - b[2]) * (a.m[2][3] - b[2]));
}
float ortAbstand(const BoneMatrix& a, const BoneMatrix& b) {
    const float o[3] = {b.m[0][3], b.m[1][3], b.m[2][3]};
    return ortAbstand(a, o);
}

// Eine Aenderung der Schicht der Sequenz als ein Rueckgaengig-Schritt (wie der Zug in der Ansicht: modellIkZiehen ruft dieselben
// Funktionen der Rechnung)
void aendern(const std::function<void(modell::RigSchicht&)>& f) {
    g_app->mtab.dok.aendern("ENT2 Griff", [&](modell::Inhalt& in) { f(in.rig.schichten[seq()->name]); }, false);
    g_app->modelDirty = true;
}

// Der Text eines Eintrags der Kategorie-Wahl in der Gruppe Presets (der erste dieser Beschriftung in der Gruppe - die Wahl steht
// ueber der Liste)
const Element* kategorie(Str t) { return imRollIn(gruppe(Str::PrRollout), tr(t)); }

// --- 1: Control Rig wie CAT -----------------------------------------------------------------------------------------------

void rigSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("ENT2: wie 3ds Max, Bereich Animation, Sequenz, kein Rig, keine Schicht", [] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Animation;
        g_app->settings.modellFeldBreite = 0.0F;
        animWaehlen(mbefoto::sequenz(), false);
        g_app->mtab.rig = modell::Rig{};
        g_app->mtab.rigFuer.clear();
        g_app->mtab.griffWahl = -1;
        g_app->mtab.rt = ModellRetarget{};
        modellRigNurFkSetzen(false);
        modellAnim().generatorOffen = false;
        g_app->mtab.dok.aendern("ENT2 leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
        g_app->animFrame = f0();
        g_app->animPlaying = false;
        if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
    }));
    add(pause(0.8));
    add(zeige(Str::AzRRig));
    add(pause(0.3));
    add(ap3::insFeld(tr(Str::AzRigErstellen)));
    add(pause(0.3));
    add(klick(tr(Str::AzRigErstellen), "mtrollouts"));
    add(pause(1.0));
    add(pruefe("ENT2 1 FK->IK: Control Rig erstellen legt die Animation der Sequenz auf das Rig - vier IK-Spuren, an jedem Bild ein Schluessel, an jedem Bild IK (Anteil 1)", [] {
        const modell::RigSchicht* s = sch();
        if (s == nullptr || seq() == nullptr) {
            melde("1 FK->IK: keine Schicht fuer " + std::string(seq() != nullptr ? seq()->name : "-"));
            return false;
        }
        int spuren = 0;
        int jedesBild = 0;
        float anteilMin = 1.0F;
        for (const modell::IkSpur& sp : s->ik) {
            ++spuren;
            jedesBild += static_cast<int>(sp.ziel.size()) == seq()->numFrames ? 1 : 0;
            for (int b = f0(); b <= fEnde(); ++b) { anteilMin = std::min(anteilMin, modell::ikAnteilAm(sp, static_cast<float>(b))); }
        }
        char z[200];
        std::snprintf(z, sizeof z, "1 FK->IK: %s, %d Bilder, %d Schluessel, %d IK-Spuren, %d mit Schluessel je Bild, kleinster Anteil %.2f", seq()->name.c_str(), seq()->numFrames,
                      s->schluesselZahl(), spuren, jedesBild, static_cast<double>(anteilMin));
        melde(z);
        return modellRigDa() && s->schluesselZahl() > 0 && spuren == 4 && jedesBild == 4 && anteilMin >= 1.0F;
    }));
    add(pruefe("ENT2 1 Abweichung: die Bewegung bleibt - je Glied (Kette mit Hand/Fuss, Fingern, Zehen) ueber die ganze Sequenz gegen die Animation ohne Rig; davon die IK allein gegen dieselben Schluessel als FK", [] {
        const modell::RigSchicht* s = sch();
        if (s == nullptr || s->ik.size() != 4U) { return false; }
        static const modell::RigSchicht kOhne;
        modell::RigSchicht nurFk = *s;
        for (modell::IkSpur& sp : nurFk.ik) {
            for (modell::IkWert& w : sp.wert) { w.ik = 0.0F; }
        }
        const std::vector<float> ges = modell::ikGliedAbweichung(g_app->anim, kOhne, *s, f0(), fEnde());
        const std::vector<float> ik = modell::ikGliedAbweichung(g_app->anim, nurFk, *s, f0(), fEnde());
        std::string z = "1 Abweichung (gesamt / nur IK, Einheiten):";
        float gMax = 0.0F;
        float iMax = 0.0F;
        static const char* kName[4] = {"Arm L", "Arm R", "Bein L", "Bein R"};
        for (std::size_t i = 0; i < s->ik.size(); ++i) {
            char t[96];
            std::snprintf(t, sizeof t, " %s %.4f / %.5f", kName[std::min<std::size_t>(static_cast<std::size_t>(s->ik[i].art), 3U)], static_cast<double>(ges[i]), static_cast<double>(ik[i]));
            z += t;
            gMax = std::max(gMax, ges[i]);
            iMax = std::max(iMax, ik[i]);
        }
        melde(z);
        // Grenzen: gesamt wie "Auf Rig legen" in AP10 (rigReduzieren haelt jedes Knochen-Bild in einer Stufe der gepackten
        // Verschiebung, 1/64 je Achse, und der Drehgrenze; entlang der Kette bis zur Hand bis 8/64) - 1/16 Einheit; gemessen
        // (kyle, BOTH_WALK1, Lauf ent2g1) hoechstens 0,0575 am Arm L - das ist die Reduktion, nicht die IK. Die IK allein (Ziel und
        // Pol aus derselben FK-Pose, geschlossene Zwei-Glied-Loesung) gemessen 0,00002: Rechengenauigkeit - float hat bei Orten von
        // 32 bis 64 Einheiten (Hand, Fuss der Figur) einen Schritt von 2^-18 = 0,0000038, die Kette rechnet drei Drehungen und die
        // Loesung nacheinander; Grenze 0,0001 (rund 26 solche Schritte)
        return gMax < 0.0625F && iMax < 0.0001F;
    }));
    // --- die Griffe wie CAT, an zwei Bildern ---
    auto vorher = std::make_shared<modell::RigSchicht>();
    add(tu("ENT2 1 Griffe: Hand R an Bild +6 und +14 verschieben (IK-Ziel), Schicht vorher merken", [vorher] {
        if (sch() == nullptr) { return; }
        *vorher = *sch();
        for (const auto& [bild, dy, dz] : {std::tuple{f0() + 6, -5.0F, 4.0F}, std::tuple{f0() + 14, 3.0F, 5.0F}}) {
            aendern([&](modell::RigSchicht& x) {
                const int sp = modell::ikSpurIndex(x, modell::IkArt::ArmRechts);
                if (sp < 0) { return; }
                BoneMatrix z = modell::starr(welt(x, bild)[static_cast<std::size_t>(x.ik[static_cast<std::size_t>(sp)].ende.knochen)]);
                z.m[1][3] += dy;
                z.m[2][3] += dz;
                modell::ikZielSetzen(g_app->anim, x, sp, bild, z);
            });
        }
    }));
    add(pause(0.3));
    add(pruefe("ENT2 1 Griffe: die Hand R steht an Bild +6 und +14 auf ihrem verschobenen IK-Ziel (Schluessel, IK), die Nachbarbilder +5, +7, +13, +15 und der linke Arm bleiben", [vorher] {
        const modell::RigSchicht* s = sch();
        const modell::IkSpur* sp = spur(modell::IkArt::ArmRechts);
        const modell::IkSpur* spL = spur(modell::IkArt::ArmLinks);
        if (s == nullptr || sp == nullptr || spL == nullptr || vorher->ik.empty()) { return false; }
        float amZiel = 0.0F;
        float verschoben = 1e9F;
        for (const int b : {f0() + 6, f0() + 14}) {
            const std::vector<BoneMatrix> w = welt(*s, b);
            const modell::IkZiel z = modell::ikZielAm(*sp, static_cast<float>(b), w);
            amZiel = std::max(amZiel, ortAbstand(w[static_cast<std::size_t>(sp->ende.knochen)], z.ziel));
            verschoben = std::min(verschoben, ortAbstand(w[static_cast<std::size_t>(sp->ende.knochen)], welt(*vorher, b)[static_cast<std::size_t>(sp->ende.knochen)]));
        }
        float nachbar = 0.0F;
        for (const int b : {f0() + 5, f0() + 7, f0() + 13, f0() + 15}) {
            const std::vector<BoneMatrix> w = welt(*s, b);
            const std::vector<BoneMatrix> v = welt(*vorher, b);
            for (std::size_t k = 0; k < w.size() && k < v.size(); ++k) { nachbar = std::max(nachbar, ortAbstand(w[k], v[k])); }
        }
        const float links = ortAbstand(welt(*s, f0() + 6)[static_cast<std::size_t>(spL->ende.knochen)], welt(*vorher, f0() + 6)[static_cast<std::size_t>(spL->ende.knochen)]);
        char z[220];
        std::snprintf(z, sizeof z, "1 Griffe Hand: Hand am Ziel %.5f, Hand verschoben mindestens %.2f, Nachbarbilder %.5f, linke Hand %.5f, Anteil +6 %.2f +14 %.2f", static_cast<double>(amZiel),
                      static_cast<double>(verschoben), static_cast<double>(nachbar), static_cast<double>(links), static_cast<double>(modell::ikAnteilAm(*sp, static_cast<float>(f0() + 6))),
                      static_cast<double>(modell::ikAnteilAm(*sp, static_cast<float>(f0() + 14))));
        melde(z);
        return amZiel < 0.01F && verschoben > 4.0F && nachbar < 0.001F && links < 0.001F && modell::ikAnteilAm(*sp, static_cast<float>(f0() + 6)) >= 1.0F &&
               modell::ikAnteilAm(*sp, static_cast<float>(f0() + 14)) >= 1.0F;
    }));
    add(tu("ENT2 1 Griffe: Pol des Ellbogens R an Bild +6 zur Seite, Fuss L (Fussplatte) an Bild +10 hoch, Ferse L an Bild +10 rollen", [vorher] {
        if (sch() == nullptr) { return; }
        *vorher = *sch();
        aendern([](modell::RigSchicht& x) {
            const int sp = modell::ikSpurIndex(x, modell::IkArt::ArmRechts);
            if (sp < 0) { return; }
            const std::vector<BoneMatrix> w = welt(x, f0() + 6);
            const modell::IkZiel z = modell::ikZielAm(x.ik[static_cast<std::size_t>(sp)], static_cast<float>(f0() + 6), w);
            const float p[3] = {z.pol[0] + 8.0F, z.pol[1], z.pol[2] - 4.0F};
            modell::ikPolSetzen(g_app->anim, x, sp, f0() + 6, p);
        });
        aendern([](modell::RigSchicht& x) {
            const int sp = modell::ikSpurIndex(x, modell::IkArt::BeinLinks);
            if (sp < 0) { return; }
            BoneMatrix z = modell::starr(welt(x, f0() + 10)[static_cast<std::size_t>(x.ik[static_cast<std::size_t>(sp)].ende.knochen)]);
            z.m[2][3] += 4.0F;
            modell::ikZielSetzen(g_app->anim, x, sp, f0() + 10, z);
        });
    }));
    add(pause(0.3));
    auto nachFuss = std::make_shared<modell::RigSchicht>();
    add(pruefe("ENT2 1 Griffe: der Pol dreht den Ellbogen R an Bild +6 (die Hand bleibt am Ziel), der Fuss L steht an Bild +10 auf seinem gehobenen Ziel, das Knie folgt; die Nachbarbilder bleiben", [vorher, nachFuss] {
        const modell::RigSchicht* s = sch();
        const modell::IkSpur* arm = spur(modell::IkArt::ArmRechts);
        const modell::IkSpur* bein = spur(modell::IkArt::BeinLinks);
        if (s == nullptr || arm == nullptr || bein == nullptr || vorher->ik.empty()) { return false; }
        const std::vector<BoneMatrix> w6 = welt(*s, f0() + 6);
        const std::vector<BoneMatrix> v6 = welt(*vorher, f0() + 6);
        const float ellbogen = ortAbstand(w6[static_cast<std::size_t>(arm->unten.knochen)], v6[static_cast<std::size_t>(arm->unten.knochen)]);
        const float hand = ortAbstand(w6[static_cast<std::size_t>(arm->ende.knochen)], v6[static_cast<std::size_t>(arm->ende.knochen)]);
        const std::vector<BoneMatrix> w10 = welt(*s, f0() + 10);
        const std::vector<BoneMatrix> v10 = welt(*vorher, f0() + 10);
        const modell::IkZiel zf = modell::ikZielAm(*bein, static_cast<float>(f0() + 10), w10);
        const float fussAmZiel = ortAbstand(w10[static_cast<std::size_t>(bein->ende.knochen)], zf.ziel);
        const float fussHoch = w10[static_cast<std::size_t>(bein->ende.knochen)].m[2][3] - v10[static_cast<std::size_t>(bein->ende.knochen)].m[2][3];
        const float knie = ortAbstand(w10[static_cast<std::size_t>(bein->unten.knochen)], v10[static_cast<std::size_t>(bein->unten.knochen)]);
        float nachbar = 0.0F;
        for (const int b : {f0() + 5, f0() + 7, f0() + 9, f0() + 11}) {
            const std::vector<BoneMatrix> w = welt(*s, b);
            const std::vector<BoneMatrix> v = welt(*vorher, b);
            for (std::size_t k = 0; k < w.size() && k < v.size(); ++k) { nachbar = std::max(nachbar, ortAbstand(w[k], v[k])); }
        }
        char z[240];
        std::snprintf(z, sizeof z, "1 Griffe Pol/Fuss: Ellbogen %.2f, Hand %.5f, Fuss am Ziel %.5f, Fuss hoch %.2f, Knie %.2f, Nachbarbilder %.5f", static_cast<double>(ellbogen), static_cast<double>(hand),
                      static_cast<double>(fussAmZiel), static_cast<double>(fussHoch), static_cast<double>(knie), static_cast<double>(nachbar));
        melde(z);
        *nachFuss = *s;
        return ellbogen > 0.5F && hand < 0.01F && fussAmZiel < 0.01F && fussHoch > 3.9F && knie > 0.5F && nachbar < 0.001F;
    }));
    add(tu("ENT2 1 Griffe: Ferse L an Bild +10 um -20 Grad rollen", [] {
        aendern([](modell::RigSchicht& x) {
            const int sp = modell::ikSpurIndex(x, modell::IkArt::BeinLinks);
            if (sp >= 0) { modell::ikRolleSetzen(g_app->anim, x, sp, f0() + 10, -20.0F, 0.0F); }
        });
    }));
    add(pause(0.3));
    add(pruefe("ENT2 1 Griffe: die Fussrolle an der Ferse dreht den Fuss L an Bild +10 (Rolle -20 im Schluessel), die Nachbarbilder bleiben", [nachFuss] {
        const modell::RigSchicht* s = sch();
        const modell::IkSpur* bein = spur(modell::IkArt::BeinLinks);
        if (s == nullptr || bein == nullptr || nachFuss->ik.empty()) { return false; }
        const std::vector<BoneMatrix> w = welt(*s, f0() + 10);
        const std::vector<BoneMatrix> v = welt(*nachFuss, f0() + 10);
        const modell::IkZiel z = modell::ikZielAm(*bein, static_cast<float>(f0() + 10), w);
        const BoneMatrix& a = w[static_cast<std::size_t>(bein->ende.knochen)];
        const BoneMatrix& b = v[static_cast<std::size_t>(bein->ende.knochen)];
        // Winkel zwischen den Lagen des Fusses (Spur der relativen Drehung)
        float spurR = 0.0F;
        for (int i = 0; i < 3; ++i) {
            for (int k = 0; k < 3; ++k) { spurR += a.m[k][i] * b.m[k][i]; }
        }
        float la = 0.0F;
        float lb = 0.0F;
        for (int i = 0; i < 3; ++i) {
            for (int k = 0; k < 3; ++k) {
                la += a.m[k][i] * a.m[k][i];
                lb += b.m[k][i] * b.m[k][i];
            }
        }
        const float c = std::clamp((spurR / std::sqrt(la * lb) * 3.0F - 1.0F) / 2.0F, -1.0F, 1.0F);
        const float grad = std::acos(c) * 180.0F / 3.14159265F;
        float nachbar = 0.0F;
        for (const int bb : {f0() + 9, f0() + 11}) {
            const std::vector<BoneMatrix> wn = welt(*s, bb);
            const std::vector<BoneMatrix> vn = welt(*nachFuss, bb);
            for (std::size_t k = 0; k < wn.size() && k < vn.size(); ++k) { nachbar = std::max(nachbar, ortAbstand(wn[k], vn[k])); }
        }
        char t[200];
        std::snprintf(t, sizeof t, "1 Griffe Fussrolle: Rolle %.1f, Fuss gedreht %.1f Grad, Nachbarbilder %.5f", static_cast<double>(z.rolle), static_cast<double>(grad), static_cast<double>(nachbar));
        melde(t);
        return std::fabs(z.rolle + 20.0F) < 0.01F && grad > 10.0F && nachbar < 0.001F;
    }));
    add(tu("ENT2 1 Griffe: die vier Schritte zuruecknehmen", [] {
        for (int i = 0; i < 4; ++i) { (void)g_app->mtab.dok.rueckgaengig(); }
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    // --- IK/FK je Glied ab dem Bild, ohne Sprung (Knopf wie 3ds Max) ---
    auto vorUm = std::make_shared<modell::RigSchicht>();
    add(tu("ENT2 1 IK/FK: Bild +12, Schicht vorher merken", [vorUm] {
        g_app->animFrame = f0() + 12;
        g_app->animPlaying = false;
        g_app->modelDirty = true;
        if (sch() != nullptr) { *vorUm = *sch(); }
    }));
    add(zeige(Str::MbIkRollout));
    add(pause(0.5));
    add(klick("IK##azik", "mtrollouts", 1));
    add(pause(0.6));
    add(pruefe("ENT2 1 IK/FK: der Knopf von Arm R schaltet ab Bild +12 bis zum Ende auf FK (Bild +11 bleibt IK), angeglichen - an +11, +12, +13, Mitte und Ende kein Sprung; der Knopf zeigt FK", [vorUm] {
        const modell::RigSchicht* s = sch();
        const modell::IkSpur* sp = spur(modell::IkArt::ArmRechts);
        if (s == nullptr || sp == nullptr || vorUm->ik.empty()) { return false; }
        float fkMax = 0.0F;
        for (int b = f0() + 12; b <= fEnde(); ++b) { fkMax = std::max(fkMax, modell::ikAnteilAm(*sp, static_cast<float>(b))); }
        const float vorIk = modell::ikAnteilAm(*sp, static_cast<float>(f0() + 11));
        float sprung = 0.0F;
        for (const int b : {f0() + 11, f0() + 12, f0() + 13, (f0() + fEnde()) / 2, fEnde()}) { sprung = std::max(sprung, modell::ikSprung(g_app->anim, *vorUm, *s, b)); }
        const Element* knopf = imRoll("FK##azik");
        char z[200];
        std::snprintf(z, sizeof z, "1 IK/FK auf FK: Anteil ab +12 hoechstens %.2f, +11 %.2f, Sprung %.5f, Knopf FK %d, Meldung \"%s\"", static_cast<double>(fkMax), static_cast<double>(vorIk),
                      static_cast<double>(sprung), knopf != nullptr ? 1 : 0, g_app->mtab.meldung.c_str());
        melde(z);
        return fkMax <= 0.0F && vorIk >= 1.0F && sprung < 0.0001F && knopf != nullptr;
    }));
    add(tu("ENT2 1 IK/FK: Bild +20, Schicht vorher merken", [vorUm] {
        g_app->animFrame = f0() + 20;
        g_app->modelDirty = true;
        if (sch() != nullptr) { *vorUm = *sch(); }
    }));
    add(pause(0.4));
    add(klick("FK##azik", "mtrollouts", 0));
    add(pause(0.6));
    add(pruefe("ENT2 1 IK/FK: derselbe Knopf schaltet ab Bild +20 wieder auf IK (+12 bis +19 bleiben FK), angeglichen - an +19, +20, +21 und Ende kein Sprung", [vorUm] {
        const modell::RigSchicht* s = sch();
        const modell::IkSpur* sp = spur(modell::IkArt::ArmRechts);
        if (s == nullptr || sp == nullptr || vorUm->ik.empty()) { return false; }
        float ikMin = 1.0F;
        for (int b = f0() + 20; b <= fEnde(); ++b) { ikMin = std::min(ikMin, modell::ikAnteilAm(*sp, static_cast<float>(b))); }
        float fkMax = 0.0F;
        for (int b = f0() + 12; b < f0() + 20; ++b) { fkMax = std::max(fkMax, modell::ikAnteilAm(*sp, static_cast<float>(b))); }
        float sprung = 0.0F;
        for (const int b : {f0() + 19, f0() + 20, f0() + 21, fEnde()}) { sprung = std::max(sprung, modell::ikSprung(g_app->anim, *vorUm, *s, b)); }
        char z[200];
        std::snprintf(z, sizeof z, "1 IK/FK zurueck auf IK: Anteil ab +20 mindestens %.2f, +12..+19 hoechstens %.2f, Sprung %.5f", static_cast<double>(ikMin), static_cast<double>(fkMax),
                      static_cast<double>(sprung));
        melde(z);
        return ikMin >= 1.0F && fkMax <= 0.0F && sprung < 0.0001F;
    }));
    add(tu("ENT2 1 IK/FK: Bild +12", [] {
        g_app->animFrame = f0() + 12;
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    auto schluesselVor = std::make_shared<int>(0);
    add(tu("ENT2 1: Schluessel vor dem Neu erkennen merken", [schluesselVor] { *schluesselVor = sch() != nullptr ? sch()->schluesselZahl() : -1; }));
    add(zeige(Str::AzRRig));
    add(pause(0.3));
    add(ap3::insFeld(tr(Str::MbRgNeu)));
    add(pause(0.3));
    add(klick(tr(Str::MbRgNeu), "mtrollouts"));
    add(pause(0.6));
    add(pruefe("ENT2 1: Neu erkennen mit vorhandener Schicht legt nichts neu auf - die Schluessel und das umgeschaltete FK an Arm R bleiben", [schluesselVor] {
        const modell::IkSpur* sp = spur(modell::IkArt::ArmRechts);
        const int n = sch() != nullptr ? sch()->schluesselZahl() : -1;
        melde("1 Neu erkennen: Schluessel " + std::to_string(n) + " (vorher " + std::to_string(*schluesselVor) + ")");
        return sp != nullptr && n == *schluesselVor && modell::ikAnteilAm(*sp, static_cast<float>(f0() + 12)) <= 0.0F;
    }));
}

// --- 2 und 3: Presets ----------------------------------------------------------------------------------------------------

void presetSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("ENT2: Presets - keine Wahl, Generator zu", [] {
        g_app->mtab.rt.bibliothekWahl = -1;
        g_app->mtab.rt.gangWahl = -1;
        g_app->mtab.rt.bibliothekKategorie = 0;
        modellAnim().generatorOffen = false;
        (void)modellRtVorschau(false);
    }));
    add(zeige(Str::PrRollout));
    add(pause(0.5));
    static const Str kKat[9] = {Str::AiKatStehen, Str::AzKatGehen, Str::AzKatLaufen, Str::AzKatRueck, Str::AzKatDucken, Str::AzKatSpringen, Str::AiKatKampf, Str::AzKatGesten, Str::AiKatSonst};
    add(pruefe("ENT2 2 Kategorien: die acht des Entwurfs in seiner Folge in zwei Spalten (Stehen Gehen / Laufen Rueckwaerts / Ducken Springen / Kampf Gesten), dahinter Sonstiges", [] {
        std::string z = "2 Kategorien:";
        bool ok = true;
        const Element* vor = nullptr;
        for (int i = 0; i < 9; ++i) {
            const Element* e = kategorie(kKat[i]);
            z += std::string(" ") + tr(kKat[i]) + (e != nullptr ? "" : "(fehlt)");
            if (e == nullptr) {
                ok = false;
                continue;
            }
            if (vor != nullptr) {
                // gerade Stelle: neue Reihe links unter der vorigen; ungerade: rechts daneben in derselben Reihe
                ok = ok && ((i % 2 == 1) ? std::fabs(e->rect.Min.y - vor->rect.Min.y) < 0.5F && e->rect.Min.x > vor->rect.Min.x : e->rect.Min.y >= vor->rect.Max.y - 0.5F);
            }
            vor = e;
        }
        melde(z);
        return ok;
    }));
    add(klick(tr(Str::AzKatRueck), "mtrollouts"));
    add(pause(0.4));
    add(pruefe("ENT2 2 leer: Rueckwaerts ist gewaehlt, statt einer leeren Liste steht der Hinweis; Kurzinfo Rueckwaerts · 0", [] {
        const ui::RolloutProbe* g = gruppe(Str::PrRollout);
        const Element* hinweis = imRollIn(g, tr(Str::EnPresetsLeer));
        melde("2 leer: Kurzinfo " + (g != nullptr ? g->kurzinfo : std::string("-")) + ", Hinweis " + (hinweis != nullptr ? "da" : "fehlt") + ", Liste " +
              (kindFenster("##aibib") != nullptr ? "da" : "weg"));
        return g != nullptr && g->kurzinfo == std::string(tr(Str::AzKatRueck)) + " \xc2\xb7 0" && hinweis != nullptr && kindFenster("##aibib") == nullptr;
    }));
    add(klick(tr(Str::AzKatGesten), "mtrollouts"));
    add(pause(0.4));
    add(pruefe("ENT2 2 leer: Gesten ebenso mit Hinweis; nichts gewaehlt (Vorschau und Hauptknopf ohne Wahl gesperrt)", [] {
        const ui::RolloutProbe* g = gruppe(Str::PrRollout);
        return g != nullptr && g->kurzinfo == std::string(tr(Str::AzKatGesten)) + " \xc2\xb7 0" && imRollIn(g, tr(Str::EnPresetsLeer)) != nullptr && g_app->mtab.rt.bibliothekWahl == -1 &&
               g_app->mtab.rt.gangWahl == -1;
    }));
    add(klick(tr(Str::AiKatSonst), "mtrollouts"));
    add(pause(0.4));
    add(pruefe("ENT2 2 Sonstiges: die Presets der Kategorie other aus presets.txt (Sitting idle, Death)", [] {
        const std::vector<PresetEintrag>& b = modellPresetBibliothek();
        int n = 0;
        bool alle = true;
        for (const PresetEintrag& e : b) {
            if (e.kategorie != "other") { continue; }
            ++n;
            alle = alle && inFenster(e.name, "##aibib") != nullptr;
        }
        const ui::RolloutProbe* g = gruppe(Str::PrRollout);
        melde("2 Sonstiges: " + std::to_string(n) + " Presets, Kurzinfo " + (g != nullptr ? g->kurzinfo : std::string("-")));
        return n > 0 && alle && g != nullptr && g->kurzinfo == std::string(tr(Str::AiKatSonst)) + " \xc2\xb7 " + std::to_string(n);
    }));
    // --- 3: Generator-Gaenge in der Liste ---
    add(klick(tr(Str::AzKatLaufen), "mtrollouts"));
    add(pause(0.4));
    add(pruefe("ENT2 3 Liste: Laufen zeigt Laufen (erzeugt) als Eintrag der Liste; die Kurzinfo zaehlt ihn mit", [] {
        const ui::RolloutProbe* g = gruppe(Str::PrRollout);
        int n = 0;
        for (const PresetEintrag& e : modellPresetBibliothek()) { n += e.kategorie == "run" ? 1 : 0; }
        melde("3 Laufen: " + std::to_string(n) + " Presets, Kurzinfo " + (g != nullptr ? g->kurzinfo : std::string("-")));
        return inFenster(tr(Str::EnGangLaufen), "##aibib") != nullptr && g != nullptr && g->kurzinfo == std::string(tr(Str::AzKatLaufen)) + " \xc2\xb7 " + std::to_string(n + 1);
    }));
    add(klick(tr(Str::AzKatGehen), "mtrollouts"));
    add(pause(0.4));
    add(pruefe("ENT2 3 Liste: Gehen zeigt Gehen (erzeugt); ohne Wahl keine Regler Tempo und Armschwung", [] {
        return inFenster(tr(Str::EnGangGehen), "##aibib") != nullptr && imRoll(tr(Str::PrTempo)) == nullptr && imRoll(tr(Str::PrArm)) == nullptr;
    }));
    add(klickAn("ENT2 3: Gehen (erzeugt) waehlen", [] {
        const Element* e = inFenster(tr(Str::EnGangGehen), "##aibib");
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
    }));
    add(pause(0.5));
    add(pruefe("ENT2 3 Wahl: Gehen (erzeugt) gewaehlt - darunter Tempo und Armschwung (unter der Liste, ueber Vorschau und Generator), sonst keine Regler des Generators", [] {
        const ui::RolloutProbe* g = gruppe(Str::PrRollout);
        const ImGuiWindow* liste = kindFenster("##aibib");
        const Element* tempo = imRollIn(g, tr(Str::PrTempo));
        const Element* arm = imRollIn(g, tr(Str::PrArm));
        const Element* vorschau = imRollIn(g, tr(Str::AzPresetVorschau));
        const bool folge = liste != nullptr && tempo != nullptr && arm != nullptr && vorschau != nullptr && tempo->rect.Min.y >= liste->Pos.y + liste->Size.y - 1.0F &&
                           arm->rect.Min.y >= tempo->rect.Max.y - 0.5F && vorschau->rect.Min.y >= arm->rect.Max.y - 0.5F;
        melde("3 Wahl: gangWahl " + std::to_string(g_app->mtab.rt.gangWahl) + ", Folge " + (folge ? "ok" : "falsch") + ", Schrittlaenge " + (imRoll(tr(Str::PrSchritt)) != nullptr ? "da" : "weg"));
        return g_app->mtab.rt.gangWahl == 0 && g_app->mtab.rt.bibliothekWahl == -1 && folge && imRoll(tr(Str::PrSchritt)) == nullptr && imRoll(tr(Str::PrHuefte)) == nullptr &&
               imRoll(tr(Str::PrHub)) == nullptr;
    }));
    auto tempoVor = std::make_shared<float>(0.0F);
    auto belegtVor = std::make_shared<int>(0);
    add(tu("ENT2 3: Tempo und Plaetze vorher merken", [tempoVor, belegtVor] {
        *tempoVor = modellRtDaten()->gang.tempo;
        *belegtVor = ap10::belegt();
    }));
    add(reglerZiehen(tr(Str::PrTempo), "mtrollouts", 0, 0.2F, 0.45F));
    add(pause(0.4));
    add(pruefe("ENT2 3 Regler: Tempo ziehen aendert das Tempo des Gangs", [tempoVor] {
        melde("3 Tempo: " + std::to_string(*tempoVor) + " -> " + std::to_string(modellRtDaten()->gang.tempo));
        return std::fabs(modellRtDaten()->gang.tempo - *tempoVor) > 1.0F;
    }));
    add(klick(tr(Str::AzPresetVorschau), "mtrollouts"));
    add(pause(1.5));
    add(pruefe("ENT2 3 Vorschau: Vorschau auf Figur erzeugt den Gang auf der Figur und spielt ihn (Gehen, Ziel-Platz BOTH_WALK1); geschrieben ist nichts", [belegtVor] {
        const ModellRetarget& r = g_app->mtab.rt;
        const modell::RetargetDaten& d = *modellRtDaten();
        melde("3 Vorschau: vonPreset " + std::to_string(d.vonPreset ? 1 : 0) + ", Art " + std::to_string(static_cast<int>(d.gang.art)) + ", " + std::to_string(r.welt.size()) + " Bilder, Platz " +
              d.platz + ", belegt " + std::to_string(ap10::belegt()));
        return d.vonPreset && d.gang.art == modell::GangArt::Gehen && r.vorschau && !r.welt.empty() && d.platz == "BOTH_WALK1" && ap10::belegt() == *belegtVor;
    }));
    add(zeige(Str::PrRollout));
    add(pause(0.3));
    add(klick(tr(Str::AzPresetAuflegen), "mtrollouts"));
    add(pause(1.0));
    add(pruefe("ENT2 3 Hauptknopf: Auf Figur legen legt den erzeugten Gang auf einen Platz der Ziel-GLA", [belegtVor] {
        melde("3 Auflegen: belegt " + std::to_string(ap10::belegt()) + " (vorher " + std::to_string(*belegtVor) + ")");
        return ap10::belegt() == *belegtVor + 1;
    }));
    add(zeige(Str::PrRollout));
    add(pause(0.3));
    add(klick(tr(Str::AzGenerator), "mtrollouts"));
    add(pause(0.4));
    add(pruefe("ENT2 3 Generator: Generator ... zeigt den Rest des Generators (Schrittlaenge, Hueftschwung, Hub, Messen) unter dem Hauptknopf, ins Bild gerollt; Tempo und Armschwung stehen nur einmal", [] {
        char messen[160];
        std::snprintf(messen, sizeof messen, tr(Str::PrMessen), "BOTH_WALK1");
        melde(std::string("3 Generator: Schritt ") + (imRoll(tr(Str::PrSchritt)) != nullptr ? "da" : "fehlt") + ", Hub " + (imRoll(tr(Str::PrHub)) != nullptr ? "da" : "fehlt") + ", Messen " +
              (imRoll(messen) != nullptr ? "da" : "fehlt"));
        return modellAnim().generatorOffen && imRoll(tr(Str::PrSchritt)) != nullptr && imRoll(tr(Str::PrHuefte)) != nullptr && imRoll(tr(Str::PrHub)) != nullptr && imRoll(messen) != nullptr &&
               imRoll(tr(Str::PrTempo), 1) == nullptr && imRoll(tr(Str::PrArm), 1) == nullptr;
    }));
    add(zeige(Str::PrRollout));   // die Liste wieder ins Bild (das Aufklappen hat nach unten gerollt)
    add(pause(0.4));
    add(klickAn(
        "ENT2 3: Rechtsklick auf Gehen (erzeugt)",
        [] {
            const Element* e = inFenster(tr(Str::EnGangGehen), "##aibib");
            return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
        },
        1));
    add(pause(0.5));
    add(pruefe("ENT2 3 Rechtsklick: das Menue des Eintrags nennt Vorschau auf Figur, Auf Figur legen, Messen und Generator ...", [] {
        const bool ok = finde(tr(Str::AzPresetVorschau), "##Popup", 0, false) != nullptr && finde(tr(Str::AzPresetAuflegen), "##Popup", 0, false) != nullptr &&
                        finde(tr(Str::PrMessenAktion), "##Popup", 0, false) != nullptr && finde(tr(Str::AzGenerator), "##Popup", 0, false) != nullptr;
        std::string wo;
        for (int n = 0; n < 3; ++n) {
            if (const Element* e = finde(tr(Str::AzPresetVorschau), "", n, false)) { wo += " [" + e->fenster + " | " + e->innen + "]"; }
        }
        melde(std::string("3 Rechtsklick: ") + (ok ? "Menue mit vier Befehlen" : "Menue fehlt oder unvollstaendig") + ", Vorschau auf Figur in" + wo);
        return ok;
    }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pause(0.3));
    add(pruefe("ENT2 3 finden: der Gang des Generators steht in Alle Aktionen suchen (Rig-Aktionen der Gruppe Presets) mit Hinweis", [] {
        int n = 0;
        for (int i = 0; i < modellRigAktionZahl(); ++i) {
            if (std::string(modellRigAktionText(i)) == tr(Str::EnGangWaehlen) && modellRigAktionHinweis(i)[0] != '\0' && modellRigAktionGruppe(i) == 8) { ++n; }
        }
        return n == 1;
    }));
    add(tu("ENT2 3: Vorschau aus, Generator zu", [] {
        (void)modellRtVorschau(false);
        modellAnim().generatorOffen = false;
    }));
    add(pause(0.3));
}

// --- Fotos (BHED_MODELL_NUR=ent2foto): die geaenderten Stellen je Bedienmodus ------------------------------------------------
// ent2_<max|blender>_ikfk     Rig nach dem Erstellen (die Animation von FK auf IK, IK-Ziele zu sehen), Arm R ab Bild +12 FK,
//                             die Gruppe IK/FK oben im Feld
// ent2_<max|blender>_gang     Presets, Kategorie Gehen, Gehen (erzeugt) gewaehlt: Tempo und Armschwung unter der Liste
// ent2_<max|blender>_mehr     dazu Generator ... aufgeklappt (der Rest unter dem Hauptknopf)
// ent2_<max|blender>_leer     Kategorie Rueckwaerts: der Hinweis statt der Liste
void fotoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const std::string p = modus == 0 ? "ent2_max_" : "ent2_blender_";
        add(tu("ENT2 Foto: " + p + "ikfk - Rig erstellen (FK -> IK), Arm R ab Bild +12 FK", [=] {
            modellBedienungSetzen(modus);
            g_app->mtab.bereich = ModellBereich::Animation;
            g_app->settings.modellFeldBreite = 0.0F;
            animWaehlen(mbefoto::sequenz(), false);
            if (modus == 0) {
                g_app->mtab.rig = modell::Rig{};
                g_app->mtab.rigFuer.clear();
                g_app->mtab.dok.aendern("ENT2 leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
                (void)modellAnimRigErstellen(false);
                g_app->animFrame = f0() + 12;
                g_app->animPlaying = false;
                (void)modellIkWechseln(static_cast<int>(modell::IkArt::ArmRechts), false);
            }
            for (std::size_t i = 0; i < g_app->mtab.rig.griffe.size(); ++i) {
                if (g_app->mtab.rig.griffe[i].art == modell::GriffArt::Hand && g_app->mtab.rig.griffe[i].seite == modell::Seite::Rechts) { modellRigGriffWaehlen(static_cast<int>(i)); }
            }
            g_app->modelDirty = true;
            setzeMaus(mbefoto::ansichtEcke());
        }));
        add(pause(1.0));
        add(zeige(Str::MbIkRollout));
        add(pause(0.8));
        add(foto(p + "ikfk"));
        add(tu("ENT2 Foto: " + p + "gang - Presets, Gehen (erzeugt)", [] {
            (void)modellRtVorschau(false);
            modellAnim().generatorOffen = false;
            modellPresetGangWaehlen(static_cast<int>(modell::GangArt::Gehen));
        }));
        add(pause(0.5));
        add(zeige(Str::PrRollout));
        add(pause(0.8));
        add(foto(p + "gang"));
        add(tu("ENT2 Foto: " + p + "mehr - Generator ... auf", [] { modellAnim().generatorOffen = true; }));
        add(pause(0.5));
        add(zeige(Str::PrRollout, 14.0F));
        add(pause(0.8));
        add(foto(p + "mehr"));
        add(tu("ENT2 Foto: " + p + "leer - Kategorie Rueckwaerts", [] {
            modellAnim().generatorOffen = false;
            g_app->mtab.rt.gangWahl = -1;
            g_app->mtab.rt.bibliothekWahl = -1;
            g_app->mtab.rt.bibliothekKategorie = 3;
        }));
        add(pause(0.5));
        add(zeige(Str::PrRollout));
        add(pause(0.8));
        add(foto(p + "leer"));
    }
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "ent2" && n != "ent2foto") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_ENT2_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    auto skala = std::make_shared<float>(1.0F);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    add(tu("ENT2: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        if (const char* t = std::getenv("BHED_ENT2_THEMA")) {
            const bool hell = std::string(t) == "hell";
            for (const theme::Theme& th : theme::themes()) {
                if ((theme::relativeLuminance(th.window) > 0.5F) == hell) {
                    g_app->activeTheme = &th;
                    g_app->themePending = true;
                    break;
                }
            }
        }
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ent2.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    add(tu("ENT2: Aufteilung wie neue Einstellungen, Bereich Animation", [=] {
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
    add(klickAn("ENT2: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (n == "ent2") {
        rigSchritte(s);
        presetSchritte(s);
    } else {
        fotoSchritte(s);
    }
    add(tu("ENT2: zurueck", [=] {
        if (*thema != nullptr && g_app->activeTheme != *thema) {
            g_app->activeTheme = *thema;
            g_app->themePending = true;
        }
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        modellRigNurFkSetzen(false);
        g_app->uiScale = *skala;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.3));
    return s;
}

}  // namespace ent2
