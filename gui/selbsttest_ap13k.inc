// selbsttest_ap13k.inc - AP13K (MBENTWURF Teil B, 12.11.): Knochen-Darstellung (Abgleich R14), Durchsicht, Knochen im Vordergrund,
// Rig-Stile nach drei Vorbildern (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap13k | ap13kfoto; Handbuch BHED_HB_NUR=ap13k)
//
//   K  R14: wie 3ds Max Bone-Kasten (gefuellt wie im Entwurf, Flossen schaltbar), wie Blender "Anzeigen als" Oktaeder, Stick,
//      B-Bone, Huelle, Draht - im Menue der Kopfzeile, je Art eine andere Form im Bild
//   D  Durchsicht: Alt+X wie 3ds Max (die Figur, alle Ansichten), Alt+Z wie Blender (die aktive Ansicht); die Figur liegt mit
//      Deckkraft 0,5 ueber dem Hintergrund (gemessen im Bild); Menue der Schattierung, Suche
//   V  Im Vordergrund: aus verdeckt die Figur die Knochen; mit Durchsicht scheinen sie durch (wie 3ds Max getoent, wie Blender davor)
//   S  Rig-Stile: Gruppe im rechten Feld, Menue, Suche; CAT-Kaesten in den Massen und Farben des Vorbilds, Control Rig mit Ringen
//      nach Seite gefaerbt und IK/FK am Glied, Blender-Oktaeder mit Kreisen und Quadraten; der Stil aendert nur das Aussehen
//   T  Auswahl und Ziehen treffen die sichtbare Form: Ring, Kasten (Kante und Inneres), Kreis des Pols, Oktaeder; die Mitte eines
//      Rings trifft nicht; Knochen waehlen (wie Blender) trifft den Knochen an seiner Form
//   BHED_AP13K_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche; BHED_AP13K_THEMA=hell
// Eingebunden in selbsttest.cpp (nach selbsttest_ap5.inc, benutzt dessen Helfer und die von ap3, ap10, mbefoto).
namespace ap13k {

using mbefoto::kindFenster;

void melde(const std::string& z) { diag::detail("Knopftest: AP13K " + z); }

std::string zeile(const char* nr, int modus, const std::string& was) { return std::string("AP13K ") + nr + (modus == 0 ? " wie 3ds Max: " : " wie Blender: ") + was; }

ModellTab& mtab() { return g_app->mtab; }

struct Bild {
    std::vector<unsigned char> px;
    int w = 0;
    int h = 0;
};

// Das Bild der aktiven Ansicht mit allem, was ueber dem Modell liegt (so gross wie die Ansicht)
Schritt bildHolen(const std::string& was, std::shared_ptr<Bild> b) {
    return tu("AP13K Bild der Ansicht: " + was, [=] {
        b->px.clear();
        const bool ok = modellFotoMitUeberlagerung(0, b->px, b->w, b->h);
        melde("Bild " + was + ": " + std::to_string(ok) + " " + std::to_string(b->w) + "x" + std::to_string(b->h) + ", Formen " + std::to_string(modellKdFormZahl()));
    });
}

// Bildpunkte, die sich in einem Kanal um mehr als `schwelle` unterscheiden; `summe`: die Summe der groessten Kanalabweichung dieser Punkte
int unterschied(const Bild& a, const Bild& b, int schwelle = 8, long long* summe = nullptr) {
    if (a.px.size() != b.px.size() || a.px.empty()) { return -1; }
    int n = 0;
    long long s = 0;
    for (std::size_t i = 0; i + 3 < a.px.size(); i += 4) {
        const int d = std::max({std::abs(a.px[i] - b.px[i]), std::abs(a.px[i + 1] - b.px[i + 1]), std::abs(a.px[i + 2] - b.px[i + 2])});
        if (d > schwelle) {
            ++n;
            s += d;
        }
    }
    if (summe != nullptr) { *summe = s; }
    return n;
}

int knochenNach(const char* name) { return boneIndex(g_app->anim, name); }

int griffNr(modell::GriffArt a, modell::Seite s) {
    const modell::Rig& rig = mtab().rig;
    for (std::size_t i = 0; i < rig.griffe.size(); ++i) {
        if (rig.griffe[i].art == a && rig.griffe[i].seite == s) { return static_cast<int>(i); }
    }
    return -1;
}

// Die erste Form eines Griffs mit dieser Gestalt (-1: irgendeine)
int formVonGriff(int griff, int gestalt = -1) {
    for (int i = 0; i < modellKdFormZahl(); ++i) {
        const KdFormInfo f = modellKdFormInfo(i);
        if (f.griff == griff && griff >= 0 && (gestalt < 0 || f.gestalt == gestalt)) { return i; }
    }
    return -1;
}

int gestaltStil(modell::FormGestalt g) { return static_cast<int>(KdGestalt::Stil) + static_cast<int>(g); }

// Der Punkt auf der sichtbaren Linie einer Form in der aktiven Ansicht (Schirm)
ImVec2 formOrt(int form) {
    ImVec2 p{-50.0F, -50.0F};
    if (form < 0 || !modellKdFormPunkt(form, mtab().aktiv, p)) { meldeFehler("AP13K: kein Punkt auf der Form " + std::to_string(form)); }
    return p;
}

// Die Farbe im Bild an einer Stelle des Schirms: der Bildpunkt der Umgebung (5 x 5), der `soll` am naechsten ist (die Linie ist
// 1-2 Bildpunkte breit, ihr Rand weich)
bool farbeBei(const Bild& b, ImVec2 schirm, const int soll[3], int tol, int ist[3]) {
    const ModellAnsicht& a = mtab().ansichten[mtab().aktiv];
    const int cx = static_cast<int>(schirm.x - a.lo[0]);
    const int cy = static_cast<int>(schirm.y - a.lo[1]);
    int best = 1 << 30;
    for (int y = cy - 2; y <= cy + 2; ++y) {
        for (int x = cx - 2; x <= cx + 2; ++x) {
            if (x < 0 || y < 0 || x >= b.w || y >= b.h) { continue; }
            const unsigned char* p = &b.px[(static_cast<std::size_t>(y) * static_cast<std::size_t>(b.w) + static_cast<std::size_t>(x)) * 4U];
            const int d = std::max({std::abs(p[0] - soll[0]), std::abs(p[1] - soll[1]), std::abs(p[2] - soll[2])});
            if (d < best) {
                best = d;
                ist[0] = p[0];
                ist[1] = p[1];
                ist[2] = p[2];
            }
        }
    }
    return best <= tol;
}

std::uint64_t poseSumme() {
    std::uint64_t h = 1469598103934665603ULL;
    for (const BoneMatrix& m : g_app->modellWelt) {
        const auto* p = reinterpret_cast<const unsigned char*>(m.m);
        for (std::size_t i = 0; i < sizeof m.m; ++i) { h = (h ^ p[i]) * 1099511628211ULL; }
    }
    return h;
}

void grundstellung(int modus, ModellBereich bereich) {
    modellBedienungSetzen(modus);
    ModellTab& t = mtab();
    t.bereich = bereich;
    t.vierAnsichten = false;
    t.aktiv = 3;
    t.knochenWahl = -1;
    t.griffWahl = -1;
    t.auswahl.clear();
    for (ModellAnsicht& a : t.ansichten) {
        a.sch.raster = false;        // fuer die Messungen im Bild: nur Hintergrund, Figur, Knochen
        a.sch.rasterBoden = false;
        a.sch.achsen[0] = a.sch.achsen[1] = a.sch.achsen[2] = false;
    }
    for (bool& d : modellKd().durchsicht) { d = false; }
    g_app->settings.modellKnochenArt = 0;
    g_app->settings.modellRigStil = 0;
    g_app->settings.modellKnochenVorn = true;
    g_app->settings.modellFlossen = false;
    g_app->modellKnochen = true;
    g_app->animPlaying = false;
    g_app->modelDirty = true;
}

Schritt oeffneKnochenMenue(int modus) {
    // wie 3ds Max: [+] > Knochen und Pruefen; wie Blender: Overlays
    return klickAn(std::string("AP13K: Menue der Knochen (") + (modus == 0 ? "[+]" : "Overlays") + ")", ap5::kopfFeld(modus == 0 ? 0 : 3));
}

void knochenMenue(std::vector<Schritt>& s, int modus) {
    s.push_back(oeffneKnochenMenue(modus));
    s.push_back(pause(0.3));
    if (modus == 0) {
        s.push_back(ap5::klickPopup(tr(Str::MkKnochenPruefen)));
        s.push_back(pause(0.3));
    }
}

// --- K: Knochen-Darstellung (R14) -----------------------------------------------------------------------------------------

void knochenSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto ohne = std::make_shared<Bild>();
    auto mit = std::make_shared<Bild>();
    std::array<std::shared_ptr<Bild>, kKnochenArten> art;
    for (auto& b : art) { b = std::make_shared<Bild>(); }
    // --- wie 3ds Max ---
    add(tu("AP13K K: wie 3ds Max, Bereich Animation, Knochen aus", [] {
        grundstellung(0, ModellBereich::Animation);
        g_app->modellKnochen = false;
    }));
    add(pause(0.8));
    add(bildHolen("ohne Knochen", ohne));
    add(tu("AP13K K: Knochen an", [] {
        g_app->modellKnochen = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(bildHolen("Bone-Kasten", mit));
    add(pruefe(zeile("K1", 0, "Knochen sind Bone-Kaesten - je Knochen eine Form mit zwoelf Kanten, gefuellt wie im Entwurf; im Bild der Ansicht gezeichnet"), [ohne, mit] {
        int kaesten = 0;
        int gefuellt = 0;
        for (int i = 0; i < modellKdFormZahl(); ++i) {
            const KdFormInfo f = modellKdFormInfo(i);
            kaesten += f.gestalt == static_cast<int>(KdGestalt::BoneKasten) && f.linien == 12 && f.knochen >= 0 ? 1 : 0;
            gefuellt += f.flaechen > 0 ? 1 : 0;
        }
        const int d = unterschied(*ohne, *mit);
        melde("K1: " + std::to_string(modellKdFormZahl()) + " Formen, " + std::to_string(kaesten) + " Kaesten, " + std::to_string(gefuellt) + " gefuellt, im Bild " +
              std::to_string(d) + " Bildpunkte, Knochenlinien " + std::to_string(g_app->modellKnochenLinien));
        return modellKdFormZahl() > 40 && kaesten == modellKdFormZahl() && gefuellt == kaesten && d > 2000 && g_app->modellKnochenLinien == 12 * kaesten;
    }));
    add(tu("AP13K K: Knochenflossen an", [] {
        g_app->settings.modellFlossen = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(pruefe(zeile("K2", 0, "Knochenflossen: je Kasten vier Flossen (acht Linien mehr)"), [] {
        bool alle = modellKdFormZahl() > 40;
        for (int i = 0; i < modellKdFormZahl(); ++i) { alle = alle && modellKdFormInfo(i).linien == 20; }
        return alle;
    }));
    knochenMenue(s, 0);
    add(pruefe(zeile("K3", 0, "Menue [+] > Knochen und Pruefen: Im Vordergrund, Anzeigen als, Rig-Stil stehen dort"), [] {
        return ap5::imPopup(tr(Str::KdVorn)) != nullptr && ap5::imPopup(tr(Str::KdAnzeigenAls)) != nullptr && ap5::imPopup(tr(Str::KdRigStil)) != nullptr;
    }));
    add(ap5::klickPopup(tr(Str::KdAnzeigenAls)));
    add(pause(0.4));
    add(pruefe(zeile("K3", 0, "Anzeigen als: die fuenf Arten von Blender stehen im Untermenue"), [] {
        return ap5::imPopup(tr(Str::KdArtOktaeder)) != nullptr && ap5::imPopup(tr(Str::KdArtStick)) != nullptr && ap5::imPopup(tr(Str::KdArtBBone)) != nullptr &&
               ap5::imPopup(tr(Str::KdArtHuelle)) != nullptr && ap5::imPopup(tr(Str::KdArtDraht)) != nullptr;
    }));
    add(ap5::klickPopup(tr(Str::KdArtStick)));
    add(pause(0.4));
    add(pruefe(zeile("K3", 0, "wie 3ds Max bleiben die Knochen Bone-Kaesten (Anzeigen als gilt fuer Knochen wie in Blender, der Eintrag ist gesperrt)"), [] {
        return modellKnochenArt() == KnochenArt::Oktaeder && !modellKnochenWieBlender() && modellKdFormZahl() > 40 &&
               modellKdFormInfo(0).gestalt == static_cast<int>(KdGestalt::BoneKasten);
    }));
    add(mbefoto::popupsZu());
    // --- wie Blender ---
    add(tu("AP13K K: wie Blender, Bereich Animation, Knochen an", [] { grundstellung(1, ModellBereich::Animation); }));
    add(pause(0.8));
    add(bildHolen("Oktaeder", art[0]));
    add(pruefe(zeile("K4", 1, "Vorgabe Oktaeder: je Knochen zwoelf Kanten und acht gefuellte Dreiecke, das Gelenk als Kreis"), [] {
        int okta = 0;
        for (int i = 0; i < modellKdFormZahl(); ++i) {
            const KdFormInfo f = modellKdFormInfo(i);
            okta += f.gestalt == static_cast<int>(KdGestalt::Oktaeder) && f.linien == 12 && f.flaechen == 8 && f.punkte >= 1 ? 1 : 0;
        }
        melde("K4: " + std::to_string(okta) + " Oktaeder von " + std::to_string(modellKdFormZahl()) + ", Knochenlinien " + std::to_string(g_app->modellKnochenLinien));
        return modellKnochenArt() == KnochenArt::Oktaeder && modellKnochenWieBlender() && okta > 40 && okta == modellKdFormZahl() && g_app->modellKnochenLinien == 12 * okta;
    }));
    knochenMenue(s, 1);
    add(ap5::klickPopup(tr(Str::KdAnzeigenAls)));
    add(pause(0.4));
    add(ap5::klickPopup(tr(Str::KdArtStick)));
    add(pause(0.5));
    add(mbefoto::popupsZu());
    add(pause(0.3));
    add(bildHolen("Stick", art[1]));
    add(pruefe(zeile("K5", 1, "Overlays > Anzeigen als > Stick: je Knochen eine Linie mit Punkten an den Gelenken, die Einstellung bleibt (modelBoneDisplay)"), [] {
        int stick = 0;
        for (int i = 0; i < modellKdFormZahl(); ++i) {
            const KdFormInfo f = modellKdFormInfo(i);
            stick += f.gestalt == static_cast<int>(KdGestalt::Stick) && f.linien == 1 && f.flaechen == 0 && f.punkte == 2 ? 1 : 0;
        }
        const std::string geschrieben = writeSettings(g_app->settings);
        Settings gelesen;
        const bool ok = readSettings(geschrieben, gelesen);
        melde("K5: " + std::to_string(stick) + " Sticks von " + std::to_string(modellKdFormZahl()));
        return modellKnochenArt() == KnochenArt::Stick && stick > 40 && stick == modellKdFormZahl() && geschrieben.find("modelBoneDisplay=1") != std::string::npos && ok &&
               gelesen.modellKnochenArt == 1;
    }));
    for (int a = 2; a < kKnochenArten; ++a) {
        add(tu("AP13K K: Anzeigen als " + std::to_string(a), [a] { modellKnochenArtSetzen(static_cast<KnochenArt>(a)); }));
        add(pause(0.5));
        add(bildHolen("Art " + std::to_string(a), art[static_cast<std::size_t>(a)]));
    }
    add(pruefe(zeile("K6", 1, "B-Bone (Kasten gleicher Breite), Huelle (Kugeln an den Gelenken, verbunden) und Draht (eine duenne Linie): jede Art ein anderes Bild"),
               [art] {
        std::string z = "K6 Unterschiede:";
        bool alle = true;
        for (int a = 0; a < kKnochenArten; ++a) {
            for (int b = a + 1; b < kKnochenArten; ++b) {
                const int d = unterschied(*art[static_cast<std::size_t>(a)], *art[static_cast<std::size_t>(b)]);
                z += " " + std::to_string(a) + "/" + std::to_string(b) + "=" + std::to_string(d);
                alle = alle && d > 300;
            }
        }
        melde(z);
        const KdFormInfo f = modellKdFormZahl() > 0 ? modellKdFormInfo(0) : KdFormInfo{};
        return alle && modellKnochenArt() == KnochenArt::Draht && f.gestalt == static_cast<int>(KdGestalt::Draht) && f.linien == 1 && f.punkte == 0;
    }));
    add(tu("AP13K K: Huelle", [] { modellKnochenArtSetzen(KnochenArt::Huelle); }));
    add(pause(0.4));
    add(pruefe(zeile("K6", 1, "Huelle: an beiden Gelenken eine Kugel (drei Kreise), dazwischen vier Linien"), [] {
        const KdFormInfo f = modellKdFormZahl() > 0 ? modellKdFormInfo(0) : KdFormInfo{};
        melde("K6 Huelle: Linien " + std::to_string(f.linien));
        return f.gestalt == static_cast<int>(KdGestalt::Huelle) && f.linien > 40;
    }));
    add(tu("AP13K K: B-Bone", [] { modellKnochenArtSetzen(KnochenArt::BBone); }));
    add(pause(0.4));
    add(pruefe(zeile("K6", 1, "B-Bone: zwoelf Kanten, gefuellt"), [] {
        const KdFormInfo f = modellKdFormZahl() > 0 ? modellKdFormInfo(0) : KdFormInfo{};
        return f.gestalt == static_cast<int>(KdGestalt::BBone) && f.linien == 12 && f.flaechen == 12;
    }));
    // --- der gewaehlte Knochen ---
    auto vorWahl = std::make_shared<Bild>();
    auto nachWahl = std::make_shared<Bild>();
    add(tu("AP13K K: Oktaeder, kein Knochen gewaehlt", [] {
        modellKnochenArtSetzen(KnochenArt::Oktaeder);
        mtab().knochenWahl = -1;
    }));
    add(pause(0.4));
    add(bildHolen("ohne Wahl", vorWahl));
    add(tu("AP13K K: ltibia gewaehlt", [] {
        mtab().knochenWahl = knochenNach("ltibia");
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(bildHolen("ltibia gewaehlt", nachWahl));
    add(pruefe(zeile("K7", 1, "der gewaehlte Knochen ist hervorgehoben (hell wie in Blenders Pose), nur seine Form aendert sich im Bild"), [vorWahl, nachWahl] {
        const int form = modellKdFormVonKnochen(knochenNach("ltibia"));
        const int d = unterschied(*vorWahl, *nachWahl);
        melde("K7: Form " + std::to_string(form) + ", " + std::to_string(d) + " Bildpunkte anders");
        return form >= 0 && modellKdFormInfo(form).gewaehlt && d > 50 && d < 20000;
    }));
    add(tu("AP13K K: Auswahl leer", [] {
        mtab().knochenWahl = -1;
        g_app->modelDirty = true;
    }));
}

// --- D: Durchsicht -------------------------------------------------------------------------------------------------------

// Die Deckkraft der Figur in der Durchsicht aus drei Bildern: je Bildpunkt, den die Figur deckt (anders als der Hintergrund),
// (durch - hintergrund) / (voll - hintergrund) im Kanal mit dem groessten Abstand. Der Hintergrund ist der Verlauf der Ansicht:
// die Farbe am linken Rand derselben Zeile (ohne Raster).
bool deckkraft(const Bild& voll, const Bild& durch, float& mittel, int& punkte, float& anteilNah) {
    mittel = 0.0F;
    punkte = 0;
    anteilNah = 0.0F;
    if (voll.px.size() != durch.px.size() || voll.px.empty()) { return false; }
    double summe = 0.0;
    int nah = 0;
    for (int y = 0; y < voll.h; y += 2) {
        const unsigned char* hg = &voll.px[(static_cast<std::size_t>(y) * static_cast<std::size_t>(voll.w) + 2U) * 4U];
        for (int x = 4; x < voll.w - 4; x += 2) {
            const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(voll.w) + static_cast<std::size_t>(x)) * 4U;
            int kanal = 0;
            int abstand = 0;
            for (int k = 0; k < 3; ++k) {
                const int d = std::abs(voll.px[i + static_cast<std::size_t>(k)] - hg[k]);
                if (d > abstand) {
                    abstand = d;
                    kanal = k;
                }
            }
            if (abstand < 60) { continue; }   // deutlich anders als der Hintergrund: sonst ist das Verhaeltnis ungenau (1/255 von 60 = 1,7 %)
            const float a = static_cast<float>(durch.px[i + static_cast<std::size_t>(kanal)] - hg[kanal]) /
                            static_cast<float>(voll.px[i + static_cast<std::size_t>(kanal)] - hg[kanal]);
            summe += a;
            ++punkte;
            nah += std::fabs(a - kDurchsichtAlpha) < 0.04F ? 1 : 0;
        }
    }
    if (punkte == 0) { return false; }
    mittel = static_cast<float>(summe / punkte);
    anteilNah = static_cast<float>(nah) / static_cast<float>(punkte);
    return true;
}

void durchsichtSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        auto voll = std::make_shared<Bild>();
        auto durch = std::make_shared<Bild>();
        auto wieder = std::make_shared<Bild>();
        const Str text = modus == 0 ? Str::KdDurchsicht : Str::KdRoentgen;
        add(tu(std::string("AP13K D: ") + (modus == 0 ? "wie 3ds Max" : "wie Blender") + ", Bereich Ansehen/Pruefen, ohne Knochen, weisse Figur", [modus] {
            grundstellung(modus, ModellBereich::Ansehen);
            g_app->modellKnochen = false;
            for (ModellAnsicht& a : mtab().ansichten) { a.sch.weiss = true; }   // hell gegen den dunklen Hintergrund: die Deckkraft laesst sich messen
        }));
        add(pause(0.8));
        add(klickAn("AP13K D: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
        add(pause(0.4));
        add(tu("AP13K D: Auswahl leer", [] {
            mtab().auswahl.clear();
            g_app->modelDirty = true;
        }));
        add(pause(0.4));
        add(bildHolen("deckend", voll));
        add(tasteKombi(modus == 0 ? (ImGuiMod_Alt | ImGuiKey_X) : (ImGuiMod_Alt | ImGuiKey_Z), modus == 0 ? "Alt+X" : "Alt+Z"));
        add(pause(0.6));
        add(bildHolen("Durchsicht", durch));
        add(foto(std::string("ap13k_") + (modus == 0 ? "max" : "blender") + "_durchsicht"));
        add(pruefe(zeile("D1", modus, modus == 0 ? "Alt+X schaltet die Durchsicht der Figur - in allen vier Ansichten (Objekteigenschaft See-Through)"
                                                : "Alt+Z schaltet Roentgen - nur in der aktiven Ansicht (X-Ray je 3D-Ansicht)"),
                   [modus, text] {
            const ModellKd& k = modellKd();
            const int an = (k.durchsicht[0] ? 1 : 0) + (k.durchsicht[1] ? 1 : 0) + (k.durchsicht[2] ? 1 : 0) + (k.durchsicht[3] ? 1 : 0);
            melde("D1: Durchsicht in " + std::to_string(an) + " Ansichten, aktive " + std::to_string(mtab().aktiv) + ", letzte Taste " + mtab().letzteTaste);
            return modellDurchsicht(mtab().aktiv) && an == (modus == 0 ? 4 : 1) && mtab().letzteTaste == tr(text);
        }));
        add(pruefe(zeile("D2", modus, "im Bild liegt die Figur mit Deckkraft 0,5 ueber dem Hintergrund (Blender xray_alpha 0,5), jeder Bildpunkt einmal"), [voll, durch] {
            float mittel = 0.0F;
            float nah = 0.0F;
            int punkte = 0;
            const bool ok = deckkraft(*voll, *durch, mittel, punkte, nah);
            char z[200];
            std::snprintf(z, sizeof z, "D2: %d Bildpunkte der Figur, Deckkraft im Mittel %.3f, %.1f %% innerhalb 0,5 +- 0,04", punkte, static_cast<double>(mittel),
                          static_cast<double>(nah) * 100.0);
            melde(z);
            // Grenzen: 8 Bit je Kanal - bei einem Abstand ab 60 Stufen rundet jede der drei Farben um hoechstens 0,5 Stufen, das
            // Verhaeltnis um hoechstens 1,5 / 60 = 0,025; dazu Raender der Figur (halb gedeckte Bildpunkte gibt es ohne
            // Mehrfachabtastung nicht). 0,04 je Bildpunkt, 95 % der Punkte, das Mittel auf 0,01.
            return ok && punkte > 5000 && std::fabs(mittel - kDurchsichtAlpha) < 0.01F && nah > 0.95F;
        }));
        // Menue der Schattierung: der Eintrag mit Haken, Klick schaltet aus
        add(klickAn("AP13K D: Menue der Schattierung", ap5::kopfFeld(modus == 0 ? 2 : 4)));
        add(pause(0.4));
        add(pruefe(zeile("D3", modus, "der Eintrag steht im Menue der Schattierung der Kopfzeile, mit Haken"), [text] {
            const Element* e = ap5::imPopup(tr(text));
            return e != nullptr && (e->flags & ImGuiItemStatusFlags_Checked) != 0;
        }));
        add(mbefoto::mausAuf("Durchsicht im Menue", [text] { return ap5::imPopup(tr(text)); }));
        add(pruefe(zeile("D3", modus, "der Eintrag hat einen Tooltip"), [] { return ap3::tooltipDa(); }));
        add(ap5::klickPopup(tr(text)));
        add(pause(0.6));
        add(mbefoto::popupsZu());
        add(pause(0.3));
        add(bildHolen("wieder deckend", wieder));
        add(pruefe(zeile("D3", modus, "Klick schaltet aus: das Bild ist wieder das vom Anfang (kein Bildpunkt anders)"), [voll, wieder] {
            const int d = unterschied(*voll, *wieder, 0);
            melde("D3: " + std::to_string(d) + " Bildpunkte anders");
            return !modellDurchsicht(mtab().aktiv) && d == 0;
        }));
        // Suche
        add(taste(modus == 0 ? ImGuiKey_X : ImGuiKey_F3, modus == 0 ? "X" : "F3"));
        add(pause(0.4));
        add(tippe(tr(text)));
        add(pause(0.4));
        add(pruefe(zeile("D4", modus, "Alle Aktionen suchen findet den Eintrag mit seiner Taste"), [modus, text] {
            const std::string soll = std::string("[  ] ") + tr(text) + (modus == 0 ? " (Alt+X)" : " (Alt+Z)");
            const Element* e = ap5::imPopup(soll);
            if (e == nullptr) { melde("D4: \"" + soll + "\" nicht in der Suche"); }
            return e != nullptr;
        }));
        add(klickAn("AP13K D: Eintrag der Suche", [modus, text] {
            const Element* e = ap5::imPopup(std::string("[  ] ") + tr(text) + (modus == 0 ? " (Alt+X)" : " (Alt+Z)"));
            return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
        }));
        add(pause(0.5));
        add(pruefe(zeile("D4", modus, "... und schaltet die Durchsicht ein"), [] { return modellDurchsicht(mtab().aktiv); }));
        add(tu("AP13K D: Durchsicht aus, Texturen wieder", [] {
            for (bool& d : modellKd().durchsicht) { d = false; }
            for (ModellAnsicht& a : mtab().ansichten) { a.sch.weiss = false; }
            g_app->modelDirty = true;
        }));
        add(pause(0.3));
    }
    add(pruefe("AP13K D5: die Statuszeile des Bereichs Ansehen/Pruefen nennt Alt+X (wie 3ds Max) und Alt+Z (wie Blender) wie der Entwurf", [] {
        return std::string(tr(Str::MzStMaxAnsehen)).find("Alt+X") != std::string::npos && std::string(tr(Str::MzStBlAnsehen)).find("Alt+Z") != std::string::npos;
    }));
}

// --- V: Knochen im Vordergrund -------------------------------------------------------------------------------------------

void vordergrundSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        auto ohne = std::make_shared<Bild>();          // deckend, ohne Knochen
        auto vorn = std::make_shared<Bild>();          // deckend, Knochen im Vordergrund
        auto hinten = std::make_shared<Bild>();        // deckend, Knochen nicht im Vordergrund
        auto durchOhne = std::make_shared<Bild>();     // Durchsicht, ohne Knochen
        auto durchHinten = std::make_shared<Bild>();   // Durchsicht, Knochen nicht im Vordergrund
        const std::string p = std::string("ap13k_") + (modus == 0 ? "max" : "blender");
        add(tu(std::string("AP13K V: ") + (modus == 0 ? "wie 3ds Max" : "wie Blender") + ", Bereich Animation, ohne Knochen", [modus] {
            grundstellung(modus, ModellBereich::Animation);
            g_app->modellKnochen = false;
        }));
        add(pause(0.8));
        add(bildHolen("ohne Knochen", ohne));
        add(tu("AP13K V: Knochen an", [] {
            g_app->modellKnochen = true;
            g_app->modelDirty = true;
        }));
        add(pause(0.5));
        add(bildHolen("Knochen im Vordergrund", vorn));
        add(foto(p + "_knochen_vorn"));
        add(pruefe(zeile("V1", modus, "ab Werk liegen die Knochen im Vordergrund (wie bis rc611 ueber der Figur)"), [ohne, vorn] {
            const int d = unterschied(*ohne, *vorn);
            melde("V1: " + std::to_string(d) + " Bildpunkte Knochen");
            return Settings{}.modellKnochenVorn && g_app->settings.modellKnochenVorn && d > 2000;
        }));
        knochenMenue(s, modus);
        add(mbefoto::mausAuf("Im Vordergrund", [] { return ap5::imPopup(tr(Str::KdVorn)); }));
        add(pruefe(zeile("V2", modus, "Im Vordergrund steht mit Haken und Tooltip im Menue der Knochen"), [] {
            const Element* e = ap5::imPopup(tr(Str::KdVorn));
            return e != nullptr && (e->flags & ImGuiItemStatusFlags_Checked) != 0 && ap3::tooltipDa();
        }));
        add(ap5::klickPopup(tr(Str::KdVorn)));
        add(pause(0.5));
        add(mbefoto::popupsZu());
        add(pause(0.3));
        add(bildHolen("Knochen nicht im Vordergrund", hinten));
        add(foto(p + "_knochen_verdeckt"));
        add(pruefe(zeile("V2", modus, "aus: die Figur verdeckt die Knochen in ihr - weniger als ein Viertel der Bildpunkte bleibt"), [ohne, vorn, hinten] {
            const int dv = unterschied(*ohne, *vorn);
            const int dh = unterschied(*ohne, *hinten);
            melde("V2: im Vordergrund " + std::to_string(dv) + ", verdeckt " + std::to_string(dh) + " Bildpunkte");
            return !g_app->settings.modellKnochenVorn && dh >= 0 && dh < dv / 4;
        }));
        add(tu("AP13K V: Durchsicht an, Knochen aus", [modus] {
            modellDurchsichtUmschalten(mtab().aktiv);
            g_app->modellKnochen = false;
            g_app->modelDirty = true;
            (void)modus;
        }));
        add(pause(0.5));
        add(bildHolen("Durchsicht ohne Knochen", durchOhne));
        add(tu("AP13K V: Knochen an", [] {
            g_app->modellKnochen = true;
            g_app->modelDirty = true;
        }));
        add(pause(0.5));
        add(bildHolen("Durchsicht, Knochen nicht im Vordergrund", durchHinten));
        add(foto(p + "_knochen_durchsicht"));
        add(pruefe(zeile("V3", modus, modus == 0 ? "mit Durchsicht scheinen die Knochen durch die Figur: wieder fast alle Bildpunkte, aber getoent (halb so stark)"
                                                : "mit Roentgen stehen die Knochen vor der Figur (Blender: X-Ray zeigt Knochen davor): so stark wie im Vordergrund"),
                   [modus, ohne, vorn, durchOhne, durchHinten] {
            long long sv = 0;
            long long sd = 0;
            const int dv = unterschied(*ohne, *vorn, 8, &sv);
            const int dd = unterschied(*durchOhne, *durchHinten, 4, &sd);
            const double mv = dv > 0 ? static_cast<double>(sv) / dv : 0.0;
            const double md = dd > 0 ? static_cast<double>(sd) / dd : 0.0;
            char z[220];
            std::snprintf(z, sizeof z, "V3: im Vordergrund %d Bildpunkte (mittlere Abweichung %.1f), mit Durchsicht %d (%.1f)", dv, mv, dd, md);
            melde(z);
            if (dd < dv * 3 / 4) { return false; }
            // wie 3ds Max liegt die halb deckende Figur ueber den Knochen in ihr: ihre Farbe zaehlt halb (Verhaeltnis um 0,5, hier
            // 0,35 bis 0,75: Knochen vor der Figur zaehlen voll, die Schwelle 4 statt 8 nimmt blassere Punkte mit); wie Blender voll
            return modus == 0 ? (md < mv * 0.75 && md > mv * 0.35) : md > mv * 0.85;
        }));
        add(pruefe(zeile("V4", modus, "neben der Figur aendert die Durchsicht an den Knochen nichts: jeder Bildpunkt einer Form einmal gemischt (nicht doppelt so deckend)"),
                   [ohne, hinten, durchOhne, durchHinten] {
            if (ohne->px.size() != durchOhne->px.size() || hinten->px.size() != durchHinten->px.size() || ohne->px.size() != hinten->px.size() || ohne->px.empty()) {
                return false;
            }
            int neben = 0;
            int anders = 0;
            for (std::size_t i = 0; i + 3 < ohne->px.size(); i += 4) {
                // ohne Figur: der Bildpunkt ist mit und ohne Durchsicht (ohne Knochen) gleich
                if (ohne->px[i] != durchOhne->px[i] || ohne->px[i + 1] != durchOhne->px[i + 1] || ohne->px[i + 2] != durchOhne->px[i + 2]) { continue; }
                const int dk = std::max({std::abs(ohne->px[i] - hinten->px[i]), std::abs(ohne->px[i + 1] - hinten->px[i + 1]), std::abs(ohne->px[i + 2] - hinten->px[i + 2])});
                if (dk <= 2) { continue; }   // dort liegt keine Form
                ++neben;
                const int d = std::max({std::abs(hinten->px[i] - durchHinten->px[i]), std::abs(hinten->px[i + 1] - durchHinten->px[i + 1]),
                                        std::abs(hinten->px[i + 2] - durchHinten->px[i + 2])});
                anders += d > 2 ? 1 : 0;
            }
            melde("V4: " + std::to_string(neben) + " Bildpunkte von Formen neben der Figur, davon mit Durchsicht anders " + std::to_string(anders));
            return neben > 500 && anders * 100 <= neben;
        }));
        add(tu("AP13K V: Durchsicht aus, im Vordergrund wieder an", [] {
            for (bool& d : modellKd().durchsicht) { d = false; }
            g_app->settings.modellKnochenVorn = true;
            g_app->modelDirty = true;
        }));
        add(pause(0.3));
    }
}

// --- S: Rig-Stile ----------------------------------------------------------------------------------------------------------

void rigAnlegen() {
    ModellTab& t = mtab();
    t.bereich = ModellBereich::Animation;
    animWaehlen(mbefoto::sequenz(), false);
    t.rig = modell::Rig{};
    t.rigFuer.clear();
    t.griffWahl = -1;
    t.dok.aendern("AP13K leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
    (void)modellAnimRigErstellen(false);
    const AnimEntry* e = ap10::sequenzJetzt();
    g_app->animFrame = e != nullptr ? e->firstFrame : 0;
    g_app->animPlaying = false;
    t.rigZeigen = true;
    g_app->modelDirty = true;
}

void stilSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto pose = std::make_shared<std::array<std::uint64_t, 4>>();
    auto stand = std::make_shared<std::array<std::uint64_t, 4>>();
    auto schluessel = std::make_shared<std::array<int, 4>>();
    auto bild = std::make_shared<Bild>();
    add(tu("AP13K S: wie 3ds Max, Bereich Animation, Control Rig erstellen, Stil Standard, Knochen aus", [] {
        grundstellung(0, ModellBereich::Animation);
        rigAnlegen();
        g_app->modellKnochen = false;
        g_app->settings.modellFeldBreite = 0.0F;
        if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
    }));
    add(pause(1.2));
    add(pruefe(zeile("S0", 0, "Stil Standard: die Griffe als Zeichen an den Gelenken wie im Entwurf (keine Formen im Bild)"), [pose, stand, schluessel] {
        (*pose)[0] = poseSumme();
        (*stand)[0] = static_cast<std::uint64_t>(mtab().dok.stand());
        (*schluessel)[0] = modellRigSchichtJetzt() != nullptr ? modellRigSchichtJetzt()->schluesselZahl() : -1;
        melde("S0: " + std::to_string(mtab().rig.griffe.size()) + " Griffe, gezeichnet " + std::to_string(mtab().griffeGezeichnet) + ", Formen " + std::to_string(modellKdFormZahl()));
        return modellRigDa() && modellRigStil() == modell::RigStil::Standard && !modellKdGriffeAlsFormen() && mtab().griffeGezeichnet > 20 && modellKdFormZahl() == 0;
    }));
    // --- die Gruppe im rechten Feld ---
    add(tu("AP13K S: Gruppe aufklappen", [] { modellKd().oeffnen = true; }));
    add(pause(0.4));
    add(ap10::zeige(Str::KdGruppe));
    add(pause(0.5));
    add(pruefe(zeile("S1", 0, "rechtes Feld, Bereich Animation: die Gruppe Knochen- und Rig-Darstellung mit der Wahl des Stils (vier Eintraege), Knochen, Griffen, "
                              "Im Vordergrund und Durchsicht"),
               [] {
        const ui::RolloutProbe* g = ap10::gruppe(Str::KdGruppe);
        const bool stile = ap10::imRollIn(g, tr(Str::KdStilStandard)) != nullptr && ap10::imRollIn(g, tr(Str::KdStilControl)) != nullptr &&
                           ap10::imRollIn(g, tr(Str::KdStilCat)) != nullptr && ap10::imRollIn(g, tr(Str::KdStilBlender)) != nullptr;
        const bool schalter = ap10::imRollIn(g, tr(Str::MbAnKnochen)) != nullptr && ap10::imRollIn(g, tr(Str::MbRgGriffeZeigen)) != nullptr &&
                              ap10::imRollIn(g, tr(Str::KdVorn)) != nullptr && ap10::imRollIn(g, tr(Str::KdDurchsicht)) != nullptr;
        melde(std::string("S1: Gruppe ") + (g != nullptr ? "da" : "fehlt") + ", Stile " + std::to_string(stile) + ", Schalter " + std::to_string(schalter));
        return g != nullptr && stile && schalter;
    }));
    add(mbefoto::mausAuf("CAT-Kaesten", [] { return ap10::imRollIn(ap10::gruppe(Str::KdGruppe), tr(Str::KdStilCat)); }));
    add(pruefe(zeile("S1", 0, "jeder Stil hat einen Tooltip"), [] { return ap3::tooltipDa(); }));
    add(foto("ap13k_max_gruppe"));
    add(klickAn("AP13K S: CAT-Kaesten in der Gruppe", [] {
        const Element* e = ap10::imRollIn(ap10::gruppe(Str::KdGruppe), tr(Str::KdStilCat));
        if (e == nullptr) { meldeFehler("AP13K: CAT-Kaesten steht nicht in der Gruppe"); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    }));
    add(pause(0.8));
    // --- CAT ---
    add(bildHolen("CAT, Knochen aus", bild));
    add(pruefe(zeile("S2", 0, "CAT-Kaesten gewaehlt (modelRigStyle=2): die FK-Griffe sind Kaesten ihrer Knochen, dazu Fussplatten, Hand-IK-Ziele, Pole als Kreuze, "
                              "die Bodenform"),
               [pose, stand, schluessel] {
        (*pose)[1] = poseSumme();
        (*stand)[1] = static_cast<std::uint64_t>(mtab().dok.stand());
        (*schluessel)[1] = modellRigSchichtJetzt() != nullptr ? modellRigSchichtJetzt()->schluesselZahl() : -1;
        int kaesten = 0;
        int platten = 0;
        int kreuze = 0;
        int boden = 0;
        int ohneGriff = 0;
        for (int i = 0; i < modellKdFormZahl(); ++i) {
            const KdFormInfo f = modellKdFormInfo(i);
            kaesten += f.gestalt == gestaltStil(modell::FormGestalt::Kasten) ? 1 : 0;
            platten += f.gestalt == gestaltStil(modell::FormGestalt::Rechteck) ? 1 : 0;
            // Pole: die Kreuze an den Griffen der Art Pol (auch der Saebelgriff ist ein Kreuz)
            const bool pol = f.griff >= 0 && static_cast<std::size_t>(f.griff) < mtab().rig.griffe.size() &&
                             (mtab().rig.griffe[static_cast<std::size_t>(f.griff)].art == modell::GriffArt::PolEllbogen ||
                              mtab().rig.griffe[static_cast<std::size_t>(f.griff)].art == modell::GriffArt::PolKnie);
            kreuze += f.gestalt == gestaltStil(modell::FormGestalt::Kreuz) && pol ? 1 : 0;
            boden += f.gestalt == gestaltStil(modell::FormGestalt::Bodenform) && f.schmuck ? 1 : 0;
            ohneGriff += f.griff < 0 && !f.schmuck ? 1 : 0;
        }
        melde("S2: Formen " + std::to_string(modellKdFormZahl()) + ", Kaesten " + std::to_string(kaesten) + ", Rechtecke " + std::to_string(platten) + ", Kreuze " +
              std::to_string(kreuze) + ", Bodenform " + std::to_string(boden) + ", ohne Griff " + std::to_string(ohneGriff));
        const std::string geschrieben = writeSettings(g_app->settings);
        return modellRigStil() == modell::RigStil::Cat && modellKdGriffeAlsFormen() && kaesten > 15 && platten == 4 && kreuze == 4 && boden == 1 && ohneGriff == 0 &&
               geschrieben.find("modelRigStyle=2") != std::string::npos;
    }));
    add(pruefe(zeile("S2", 0, "die Masse des Vorbilds: Oberschenkel 8 breit und 5 tief, Unterschenkel 8 x 4, Becken 19 x 14 x 10, Brust 19,35 breit, Kopf 12 - in Einheiten "
                              "des CAT-Rigs (mal 0,64 mal Massstab der Figur); Fussplatte 20 x 10,6"),
               [] {
        const float e = 0.64F * modellKdMassstab();
        const auto kasten = [&](const char* name, float breite, float tiefe, float laenge) {
            const int f = modellKdFormVonKnochen(knochenNach(name));
            if (f < 0) {
                melde(std::string("S2 Masse: keine Form fuer ") + name);
                return false;
            }
            const KdFormInfo i = modellKdFormInfo(f);
            char z[200];
            std::snprintf(z, sizeof z, "S2 Masse %s: Laenge %.3f Breite %.3f Tiefe %.3f (soll Breite %.3f Tiefe %.3f%s)", name, static_cast<double>(i.masse[0]),
                          static_cast<double>(i.masse[1]), static_cast<double>(i.masse[2]), static_cast<double>(breite * e), static_cast<double>(tiefe * e),
                          laenge > 0.0F ? (", Laenge " + std::to_string(laenge * e)).c_str() : "");
            melde(z);
            return std::fabs(i.masse[1] - breite * e) < 0.02F && std::fabs(i.masse[2] - tiefe * e) < 0.02F && (laenge <= 0.0F || std::fabs(i.masse[0] - laenge * e) < 0.02F);
        };
        bool ok = std::fabs(modellKdMassstab() - 1.0F) < 0.05F;   // kyle steht auf dem _humanoid
        ok = kasten("lfemurYZ", 8.0F, 5.0F, 0.0F) && ok;
        ok = kasten("rtibia", 8.0F, 4.0F, 0.0F) && ok;
        ok = kasten("pelvis", 19.0F, 14.0F, 10.0F) && ok;
        ok = kasten("thoracic", 19.35F, 13.9F, 10.0F) && ok;
        ok = kasten("cranium", 12.0F, 12.0F, 12.0F) && ok;
        ok = kasten("lhumerus", 5.544F, 6.16F, 0.0F) && ok;
        ok = kasten("rhumerus", 6.08F, 7.44F, 0.0F) && ok;
        const int platte = formVonGriff(griffNr(modell::GriffArt::Fuss, modell::Seite::Links), gestaltStil(modell::FormGestalt::Rechteck));
        const KdFormInfo pi = platte >= 0 ? modellKdFormInfo(platte) : KdFormInfo{};
        ok = platte >= 0 && std::fabs(pi.masse[0] - 20.0F * e) < 0.02F && std::fabs(pi.masse[1] - 10.6F * e) < 0.02F && ok;
        return ok;
    }));
    add(pruefe(zeile("S2", 0, "die Farben des Vorbilds im Bild: Becken 255/64/0, Brust 255/200/0, Kopf 255/0/0, Arm L 64/255/0, Arm R 0/64/255, Bein L 0/64/0, "
                              "Bein R 0/0/64"),
               [bild] {
        struct Soll {
            const char* knochen;
            int farbe[3];
        };
        static const Soll kSoll[] = {{"pelvis", {255, 64, 0}}, {"thoracic", {255, 200, 0}}, {"cranium", {255, 0, 0}}, {"lhumerus", {64, 255, 0}},
                                     {"rhumerus", {0, 64, 255}}, {"lfemurYZ", {0, 64, 0}}, {"rtibia", {0, 0, 64}}};
        bool ok = true;
        for (const Soll& x : kSoll) {
            const int f = modellKdFormVonKnochen(knochenNach(x.knochen));
            ImVec2 p;
            int ist[3] = {-1, -1, -1};
            const bool da = f >= 0 && modellKdFormPunkt(f, mtab().aktiv, p);
            // die Linie liegt deckend ueber dem Bild (Alpha 1 in ihrer Mitte); 24 Stufen fuer den weichen Rand einer 1,5 Punkte breiten Linie
            const bool passt = da && farbeBei(*bild, p, x.farbe, 24, ist);
            const KdFormInfo i = f >= 0 ? modellKdFormInfo(f) : KdFormInfo{};
            char z[200];
            std::snprintf(z, sizeof z, "S2 Farbe %s: Form %d, im Bild %d/%d/%d, soll %d/%d/%d, Tabelle %.0f/%.0f/%.0f", x.knochen, f, ist[0], ist[1], ist[2], x.farbe[0],
                          x.farbe[1], x.farbe[2], static_cast<double>(i.farbe[0]) * 255.0, static_cast<double>(i.farbe[1]) * 255.0, static_cast<double>(i.farbe[2]) * 255.0);
            melde(z);
            ok = ok && passt;
        }
        return ok;
    }));
    add(foto("ap13k_max_cat"));
    add(tu("AP13K S: Knochen an (alle Kaesten)", [] {
        g_app->modellKnochen = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(pruefe(zeile("S2", 0, "mit Knochen: jeder Knochen mit Rolle hat seinen Kasten (auch Finger und Gesicht)"), [] {
        int kaesten = 0;
        for (int i = 0; i < modellKdFormZahl(); ++i) { kaesten += modellKdFormInfo(i).gestalt == gestaltStil(modell::FormGestalt::Kasten) ? 1 : 0; }
        int mitRolle = 0;
        for (const modell::KnochenErkennung& k : mtab().rig.erkennung.knochen) {
            mitRolle += k.rolle != modell::Rolle::Keine && k.rolle != modell::Rolle::Wurzel && k.rolle != modell::Rolle::Bewegung ? 1 : 0;
        }
        melde("S2: Kaesten " + std::to_string(kaesten) + ", Knochen mit Rolle " + std::to_string(mitRolle));
        return kaesten == mitRolle && modellKdFormVonKnochen(knochenNach("l_d1_j1")) >= 0;
    }));
    add(foto("ap13k_max_cat_knochen"));
    // --- Control Rig ---
    add(tu("AP13K S: Stil Control Rig, Knochen aus", [] {
        g_app->modellKnochen = false;
        modellRigStilSetzen(modell::RigStil::ControlRig);
    }));
    add(pause(0.8));
    add(bildHolen("Control Rig", bild));
    add(pruefe(zeile("S3", 0, "Control Rig (Ringe): jeder Griff hat seine Form; Pole als Kugeln, Fussplatten, ein Bodenkreis; IK oder FK steht an jedem der vier Glieder"),
               [pose, stand, schluessel] {
        (*pose)[2] = poseSumme();
        (*stand)[2] = static_cast<std::uint64_t>(mtab().dok.stand());
        (*schluessel)[2] = modellRigSchichtJetzt() != nullptr ? modellRigSchichtJetzt()->schluesselZahl() : -1;
        const modell::Rig& rig = mtab().rig;
        int ohne = 0;
        for (std::size_t i = 0; i < rig.griffe.size(); ++i) { ohne += modellKdFormVonGriff(static_cast<int>(i)) < 0 ? 1 : 0; }
        int kugeln = 0;
        int bodenkreise = 0;
        for (int i = 0; i < modellKdFormZahl(); ++i) {
            const KdFormInfo f = modellKdFormInfo(i);
            kugeln += f.gestalt == gestaltStil(modell::FormGestalt::Kugel) ? 1 : 0;
            bodenkreise += f.schmuck && f.gestalt == gestaltStil(modell::FormGestalt::Ring) ? 1 : 0;
        }
        melde("S3: Griffe ohne Form " + std::to_string(ohne) + ", Kugeln " + std::to_string(kugeln) + ", Bodenkreise " + std::to_string(bodenkreise) + ", Beschriftungen " +
              std::to_string(modellKd().beschriftungen));
        return modellRigStil() == modell::RigStil::ControlRig && ohne == 0 && kugeln == 4 && bodenkreise == 1 && modellKd().beschriftungen == 4;
    }));
    add(pruefe(zeile("S3", 0, "nach Seite gefaerbt, im Bild gemessen: rechts rot, links blau, Mitte gruen (Kopf) und gelb (Becken)"), [bild] {
        struct Soll {
            modell::GriffArt art;
            modell::Seite seite;
            int was;   // 0 rot, 1 blau, 2 gruen, 3 gelb
        };
        // Griffe, die nie zuruecktreten: Hand und Schluesselbein je Seite, Kopf, Becken
        static const Soll kSoll[] = {{modell::GriffArt::Hand, modell::Seite::Rechts, 0}, {modell::GriffArt::Hand, modell::Seite::Links, 1},
                                     {modell::GriffArt::Schluessel, modell::Seite::Rechts, 0}, {modell::GriffArt::Schluessel, modell::Seite::Links, 1},
                                     {modell::GriffArt::Kopf, modell::Seite::Mitte, 2}, {modell::GriffArt::Becken, modell::Seite::Mitte, 3}};
        bool ok = true;
        for (const Soll& x : kSoll) {
            const int f = formVonGriff(griffNr(x.art, x.seite));
            const KdFormInfo i = f >= 0 ? modellKdFormInfo(f) : KdFormInfo{};
            const int soll[3] = {static_cast<int>(i.farbe[0] * 255.0F), static_cast<int>(i.farbe[1] * 255.0F), static_cast<int>(i.farbe[2] * 255.0F)};
            ImVec2 p;
            int ist[3] = {-1, -1, -1};
            const bool da = f >= 0 && modellKdFormPunkt(f, mtab().aktiv, p) && farbeBei(*bild, p, soll, 40, ist);
            const bool art = x.was == 0 ? ist[0] > ist[2] + 60 && ist[0] > ist[1] + 40
                                        : (x.was == 1 ? ist[2] > ist[0] + 60 : (x.was == 2 ? ist[1] > ist[0] + 60 && ist[1] > ist[2] + 60 : ist[0] > 180 && ist[1] > 160 && ist[2] < 120));
            char z[160];
            std::snprintf(z, sizeof z, "S3 Farbe Griff %d/%d: Form %d, im Bild %d/%d/%d", static_cast<int>(x.art), static_cast<int>(x.seite), f, ist[0], ist[1], ist[2]);
            melde(z);
            ok = ok && da && art;
        }
        return ok;
    }));
    add(pruefe(zeile("S3", 0, "was am Glied gerade nicht wirkt, tritt zurueck: alle vier Glieder laufen nach dem Erstellen unter IK - der FK-Ring des Unterarms "
                              "ist im Bild blasser als seine Farbe (40 % Deckkraft ueber der Figur), bleibt aber rot bzw. blau"),
               [bild] {
        bool ok = true;
        for (const modell::Seite seite : {modell::Seite::Rechts, modell::Seite::Links}) {
            const int f = formVonGriff(griffNr(modell::GriffArt::Unterarm, seite));
            const KdFormInfo i = f >= 0 ? modellKdFormInfo(f) : KdFormInfo{};
            const int voll[3] = {static_cast<int>(i.farbe[0] * 255.0F), static_cast<int>(i.farbe[1] * 255.0F), static_cast<int>(i.farbe[2] * 255.0F)};
            ImVec2 p;
            int ist[3] = {-1, -1, -1};
            const bool da = f >= 0 && modellKdFormPunkt(f, mtab().aktiv, p);
            (void)farbeBei(*bild, p, voll, 255, ist);
            const int haupt = seite == modell::Seite::Rechts ? 0 : 2;
            const int gegen = seite == modell::Seite::Rechts ? 2 : 0;
            char z[160];
            std::snprintf(z, sizeof z, "S3 blass Unterarm %s: im Bild %d/%d/%d, volle Farbe %d/%d/%d", seite == modell::Seite::Rechts ? "R" : "L", ist[0], ist[1], ist[2], voll[0],
                          voll[1], voll[2]);
            melde(z);
            // 40 % der Farbe ueber der Figur: der Hauptkanal liegt deutlich unter dem vollen Wert (hoechstens 80 %), ueberwiegt aber weiter
            ok = ok && da && ist[haupt] < voll[haupt] * 8 / 10 && ist[haupt] > ist[gegen] + 40;
        }
        return ok;
    }));
    add(foto("ap13k_max_controlrig"));
    // --- Blender-Oktaeder (im Bedienmodus wie 3ds Max) ---
    add(tu("AP13K S: Stil Blender-Oktaeder, Knochen an", [] {
        g_app->modellKnochen = true;
        modellRigStilSetzen(modell::RigStil::Blender);
    }));
    add(pause(0.8));
    add(pruefe(zeile("S4", 0, "Blender-Oktaeder: Knochen als Oktaeder auch im Bedienmodus wie 3ds Max; Kreis um das Becken (10,47), Kreise um die Wirbel, Quadrate an "
                              "Hand (2,48) und Fuss (3,56), Kreise als Pole (1,94 und 2,46), der Bodenkreis (26,37)"),
               [pose, stand, schluessel] {
        (*pose)[3] = poseSumme();
        (*stand)[3] = static_cast<std::uint64_t>(mtab().dok.stand());
        (*schluessel)[3] = modellRigSchichtJetzt() != nullptr ? modellRigSchichtJetzt()->schluesselZahl() : -1;
        const float m = modellKdMassstab();
        const int okta = modellKdFormVonKnochen(knochenNach("ltibia"));
        const int becken = formVonGriff(griffNr(modell::GriffArt::Becken, modell::Seite::Mitte), gestaltStil(modell::FormGestalt::Ring));
        const int hand = formVonGriff(griffNr(modell::GriffArt::Hand, modell::Seite::Links), gestaltStil(modell::FormGestalt::Rechteck));
        const int fuss = formVonGriff(griffNr(modell::GriffArt::Fuss, modell::Seite::Rechts), gestaltStil(modell::FormGestalt::Rechteck));
        const int polE = formVonGriff(griffNr(modell::GriffArt::PolEllbogen, modell::Seite::Links), gestaltStil(modell::FormGestalt::Ring));
        const int polK = formVonGriff(griffNr(modell::GriffArt::PolKnie, modell::Seite::Rechts), gestaltStil(modell::FormGestalt::Ring));
        float boden = 0.0F;
        for (int i = 0; i < modellKdFormZahl(); ++i) {
            const KdFormInfo f = modellKdFormInfo(i);
            if (f.schmuck && f.gestalt == gestaltStil(modell::FormGestalt::Ring)) { boden = f.masse[0]; }
        }
        const auto r = [](int f) { return f >= 0 ? modellKdFormInfo(f).masse[0] : -1.0F; };
        char z[240];
        std::snprintf(z, sizeof z, "S4: Oktaeder %d, Becken %.3f, Hand %.3f, Fuss %.3f, Pol Ellbogen %.3f, Pol Knie %.3f, Boden %.3f (Massstab %.3f)", okta,
                      static_cast<double>(r(becken)), static_cast<double>(r(hand)), static_cast<double>(r(fuss)), static_cast<double>(r(polE)), static_cast<double>(r(polK)),
                      static_cast<double>(boden), static_cast<double>(m));
        melde(z);
        return modellKnochenWieBlender() && okta >= 0 && modellKdFormInfo(okta).gestalt == static_cast<int>(KdGestalt::Oktaeder) && std::fabs(r(becken) - 10.468F * m) < 0.03F &&
               std::fabs(r(hand) - 4.9524F * m) < 0.03F && std::fabs(r(fuss) - 7.112F * m) < 0.03F && std::fabs(r(polE) - 1.9379F * m) < 0.02F &&
               std::fabs(r(polK) - 2.4643F * m) < 0.02F && std::fabs(boden - 26.3728F * m) < 0.05F;
    }));
    add(foto("ap13k_max_blender"));
    add(pruefe(zeile("S5", 0, "der Stil aendert nur das Aussehen: Pose, Dokument und Schluessel sind in allen vier Stilen gleich"), [pose, stand, schluessel] {
        const bool gleich = (*pose)[0] == (*pose)[1] && (*pose)[1] == (*pose)[2] && (*pose)[2] == (*pose)[3] && (*stand)[0] == (*stand)[1] && (*stand)[1] == (*stand)[2] &&
                            (*stand)[2] == (*stand)[3] && (*schluessel)[0] == (*schluessel)[1] && (*schluessel)[1] == (*schluessel)[2] && (*schluessel)[2] == (*schluessel)[3];
        melde("S5: Schluessel " + std::to_string((*schluessel)[0]) + ", Stand " + std::to_string((*stand)[0]) + " / " + std::to_string((*stand)[3]));
        return gleich && (*schluessel)[0] > 0 && (*pose)[0] != 0U;
    }));
    // --- Suche und Menue ---
    add(klickAn("AP13K S: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.3));
    add(taste(ImGuiKey_X, "X"));
    add(pause(0.4));
    add(tippe(tr(Str::KdStilControl)));
    add(pause(0.4));
    add(klickAn("AP13K S: Rig-Stil in der Suche", [] {
        const std::string soll = std::string("[  ] ") + tr(Str::KdRigStil) + ": " + tr(Str::KdStilControl);
        const Element* e = ap5::imPopup(soll);
        if (e == nullptr) { meldeFehler("AP13K: \"" + soll + "\" steht nicht in der Suche"); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    }));
    add(pause(0.5));
    add(pruefe(zeile("S6", 0, "Alle Aktionen suchen: Rig-Stil: Control Rig (Ringe) stellt den Stil ein"), [] { return modellRigStil() == modell::RigStil::ControlRig; }));
    knochenMenue(s, 0);
    add(ap5::klickPopup(tr(Str::KdRigStil)));
    add(pause(0.4));
    add(ap5::klickPopup(tr(Str::KdStilStandard)));
    add(pause(0.5));
    add(pruefe(zeile("S7", 0, "Menue [+] > Knochen und Pruefen > Rig-Stil > Standard stellt den Entwurf wieder her"), [] {
        return modellRigStil() == modell::RigStil::Standard && !modellKdGriffeAlsFormen();
    }));
    add(mbefoto::popupsZu());
    // --- Rechtsklick in der Ansicht ---
    add(klickAn("AP13K S: Rechtsklick in die Ansicht", mbefoto::ansichtEcke, 1));
    add(pause(0.5));
    add(ap5::klickPopup(tr(Str::MbRAnzeige)));
    add(pause(0.4));
    add(ap5::klickPopup(tr(Str::MkKnochenPruefen)));
    add(pause(0.4));
    add(pruefe(zeile("S8", 0, "Rechtsklick in die Ansicht > Anzeige > Knochen und Pruefen: Im Vordergrund, Anzeigen als und Rig-Stil stehen auch dort"), [] {
        return ap5::imPopup(tr(Str::KdVorn)) != nullptr && ap5::imPopup(tr(Str::KdAnzeigenAls)) != nullptr && ap5::imPopup(tr(Str::KdRigStil)) != nullptr;
    }));
    add(mbefoto::popupsZu());
    add(pause(0.3));
    // --- wie Blender: die Gruppe und das Menue ---
    add(tu("AP13K S: wie Blender", [] {
        modellBedienungSetzen(1);
        mtab().vierAnsichten = false;
        g_app->modelDirty = true;
        modellKd().oeffnen = true;
    }));
    add(pause(0.8));
    add(ap10::zeige(Str::KdGruppe));
    add(pause(0.5));
    add(pruefe(zeile("S9", 1, "die Gruppe zeigt wie Blender Anzeigen als (fuenf Arten) und Roentgen statt Durchsicht"), [] {
        const ui::RolloutProbe* g = ap10::gruppe(Str::KdGruppe);
        return g != nullptr && ap10::imRollIn(g, tr(Str::KdArtOktaeder)) != nullptr && ap10::imRollIn(g, tr(Str::KdArtStick)) != nullptr &&
               ap10::imRollIn(g, tr(Str::KdArtBBone)) != nullptr && ap10::imRollIn(g, tr(Str::KdArtHuelle)) != nullptr && ap10::imRollIn(g, tr(Str::KdArtDraht)) != nullptr &&
               ap10::imRollIn(g, tr(Str::KdRoentgen)) != nullptr && ap10::imRollIn(g, tr(Str::KdDurchsicht)) == nullptr;
    }));
    add(foto("ap13k_blender_gruppe"));
    knochenMenue(s, 1);
    add(ap5::klickPopup(tr(Str::KdRigStil)));
    add(pause(0.4));
    add(ap5::klickPopup(tr(Str::KdStilBlender)));
    add(pause(0.6));
    add(mbefoto::popupsZu());
    add(pruefe(zeile("S9", 1, "Overlays > Rig-Stil > Blender-Oktaeder stellt den Stil ein"), [] { return modellRigStil() == modell::RigStil::Blender; }));
    add(pause(0.4));
    add(foto("ap13k_blender_blender"));
}

// --- T: Auswahl und Ziehen treffen die sichtbare Form ---------------------------------------------------------------------

Schritt abwaehlen() {
    return tu("AP13K T: nichts gewaehlt", [] {
        modellRigGriffWaehlen(-1);
        mtab().knochenWahl = -1;
        mtab().auswahl.clear();
        g_app->modelDirty = true;
    });
}

void treffSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // --- Control Rig: der Ring trifft, seine Mitte nicht ---
    add(tu("AP13K T: wie 3ds Max, Rig, Stil Control Rig, Knochen aus, Auswaehlen (Q)", [] {
        grundstellung(0, ModellBereich::Animation);
        if (!modellRigDa()) { rigAnlegen(); }
        g_app->modellKnochen = false;
        mtab().gizmo = 0;
        modellRigStilSetzen(modell::RigStil::ControlRig);
    }));
    add(pause(0.8));
    add(abwaehlen());
    add(pause(0.3));
    add(klickAn("AP13K T: auf den Ring des Unterarms R", [] { return formOrt(formVonGriff(griffNr(modell::GriffArt::Unterarm, modell::Seite::Rechts))); }));
    add(pause(0.5));
    add(pruefe(zeile("T1", 0, "Control Rig: ein Klick auf den Ring waehlt den Griff (Unterarm R), seine Form ist hervorgehoben"), [] {
        const int g = griffNr(modell::GriffArt::Unterarm, modell::Seite::Rechts);
        const int f = formVonGriff(g);
        melde("T1: gewaehlt " + std::to_string(mtab().griffWahl) + ", soll " + std::to_string(g));
        return g >= 0 && mtab().griffWahl == g && f >= 0 && modellKdFormInfo(f).gewaehlt;
    }));
    add(abwaehlen());
    add(pause(0.3));
    add(klickAn("AP13K T: in die Mitte des Rings (auf das Gelenk)", [] {
        ImVec2 p{-50.0F, -50.0F};
        (void)modellRigGriffAufSchirm(griffNr(modell::GriffArt::Unterarm, modell::Seite::Rechts), p);
        return p;
    }));
    add(pause(0.5));
    add(pruefe(zeile("T1", 0, "die Mitte des Rings (das Gelenk, wo im Stil Standard das Zeichen sitzt) trifft den Griff nicht: nur die sichtbare Form zaehlt"), [] {
        melde("T1 Mitte: gewaehlt " + std::to_string(mtab().griffWahl));
        return mtab().griffWahl != griffNr(modell::GriffArt::Unterarm, modell::Seite::Rechts);
    }));
    add(abwaehlen());
    add(pause(0.3));
    add(klickAn("AP13K T: auf die Kugel des Pols Knie L", [] { return formOrt(formVonGriff(griffNr(modell::GriffArt::PolKnie, modell::Seite::Links))); }));
    add(pause(0.5));
    add(pruefe(zeile("T1", 0, "ein Klick auf die Kugel waehlt den Pol (Knie L)"), [] { return mtab().griffWahl == griffNr(modell::GriffArt::PolKnie, modell::Seite::Links); }));
    // --- CAT: Kante und Inneres des Kastens ---
    add(tu("AP13K T: Stil CAT-Kaesten", [] { modellRigStilSetzen(modell::RigStil::Cat); }));
    add(pause(0.8));
    add(abwaehlen());
    add(pause(0.3));
    add(klickAn("AP13K T: auf eine Kante des Kastens Unterschenkel L", [] { return formOrt(formVonGriff(griffNr(modell::GriffArt::Unterschenkel, modell::Seite::Links))); }));
    add(pause(0.5));
    add(pruefe(zeile("T2", 0, "CAT: ein Klick auf eine Kante des Kastens waehlt den FK-Griff seines Knochens (Unterschenkel L)"), [] {
        melde("T2 Kante: gewaehlt " + std::to_string(mtab().griffWahl));
        return mtab().griffWahl == griffNr(modell::GriffArt::Unterschenkel, modell::Seite::Links);
    }));
    add(abwaehlen());
    add(pause(0.3));
    add(klickAn("AP13K T: mitten in den Kasten des Kopfs (6 Einheiten des Rigs ueber dem Gelenk, weit von jeder Kante)", [] {
        const std::vector<BoneMatrix>& w = g_app->modellWelt;
        const int a = knochenNach("cranium");
        ImVec2 p{-50.0F, -50.0F};
        if (a < 0) { return p; }
        const float hoch = 6.0F * 0.64F * modellKdMassstab();
        // die Laengsachse des Kopfs in der Pose: die Grundstellung zeigt nach oben (0, 0, 1)
        const float oben[3] = {0.0F, 0.0F, hoch};
        float lokal[3];
        float welt[3];
        const BoneMatrix& bp = g_app->anim.bones[static_cast<std::size_t>(a)].basePose;
        const float ruhe[3] = {bp.m[0][3] + oben[0], bp.m[1][3] + oben[1], bp.m[2][3] + oben[2]};
        g_app->anim.bones[static_cast<std::size_t>(a)].basePoseInv.transform(ruhe, lokal);
        w[static_cast<std::size_t>(a)].transform(lokal, welt);
        (void)modellAufSchirm(mtab().ansichten[mtab().aktiv], welt, p);
        return p;
    }));
    add(pause(0.5));
    add(pruefe(zeile("T2", 0, "... und ein Klick in den Kasten ebenso (Kopf): der Kasten ist ein Koerper"), [] {
        melde("T2 innen: gewaehlt " + std::to_string(mtab().griffWahl));
        return mtab().griffWahl == griffNr(modell::GriffArt::Kopf, modell::Seite::Mitte);
    }));
    add(abwaehlen());
    add(pause(0.3));
    add(klickAn("AP13K T: auf die Fussplatte L", [] {
        return formOrt(formVonGriff(griffNr(modell::GriffArt::Fuss, modell::Seite::Links), gestaltStil(modell::FormGestalt::Rechteck)));
    }));
    add(pause(0.5));
    add(pruefe(zeile("T3", 0, "ein Klick auf die Fussplatte waehlt den Fuss (IK-Ziel des Beins)"), [] {
        melde("T3: gewaehlt " + std::to_string(mtab().griffWahl));
        return mtab().griffWahl == griffNr(modell::GriffArt::Fuss, modell::Seite::Links);
    }));
    // Ziehen: W, der Griff des Gizmos an der X-Achse, 60 Punkte - das IK-Ziel des Beins wandert
    auto zielVorher = std::make_shared<std::array<float, 3>>();
    add(tu("AP13K T: Verschieben (W), Ziel des Beins L merken", [zielVorher] {
        mtab().gizmo = 1;
        const modell::RigSchicht* sch = modellRigSchichtJetzt();
        const int sp = sch != nullptr ? modell::ikSpurIndex(*sch, modell::IkArt::BeinLinks) : -1;
        if (sp >= 0) {
            const modell::IkZiel z = modell::ikZielAm(sch->ik[static_cast<std::size_t>(sp)], static_cast<float>(g_app->animFrame), g_app->modellWelt);
            *zielVorher = {z.ziel.m[0][3], z.ziel.m[1][3], z.ziel.m[2][3]};
        }
    }));
    add(pause(0.5));
    add(ziehenAn("AP13K T: Griff X des Gizmos an der Fussplatte ziehen", [] {
        ImVec2 p{-50.0F, -50.0F};
        if (!modellGizmoGriffOrt(1, p)) { meldeFehler("AP13K: kein Gizmo am gewaehlten Griff"); }
        return p;
    }, 60.0F, 0.0F, 0));
    add(pause(0.6));
    add(pruefe(zeile("T4", 0, "Ziehen am Griff, der ueber seine Form gewaehlt wurde: das IK-Ziel des Beins L wandert; Strg+Z nimmt es zurueck"), [zielVorher] {
        const modell::RigSchicht* sch = modellRigSchichtJetzt();
        const int sp = sch != nullptr ? modell::ikSpurIndex(*sch, modell::IkArt::BeinLinks) : -1;
        if (sp < 0) { return false; }
        const modell::IkZiel z = modell::ikZielAm(sch->ik[static_cast<std::size_t>(sp)], static_cast<float>(g_app->animFrame), g_app->modellWelt);
        const float d = std::sqrt((z.ziel.m[0][3] - (*zielVorher)[0]) * (z.ziel.m[0][3] - (*zielVorher)[0]) + (z.ziel.m[1][3] - (*zielVorher)[1]) * (z.ziel.m[1][3] - (*zielVorher)[1]) +
                                  (z.ziel.m[2][3] - (*zielVorher)[2]) * (z.ziel.m[2][3] - (*zielVorher)[2]));
        char t[120];
        std::snprintf(t, sizeof t, "T4: Ziel um %.3f Einheiten gewandert", static_cast<double>(d));
        melde(t);
        const bool gewandert = d > 0.5F;
        if (gewandert) { (void)modellRueckgaengig(); }   // nur der Zug selbst (ohne Zug nichts zuruecknehmen - sonst ginge das Rig)
        g_app->modelDirty = true;
        return gewandert;
    }));
    add(pause(0.5));
    // --- Blender-Oktaeder: Kreis des Pols, Oktaeder als Griff ---
    add(tu("AP13K T: Stil Blender-Oktaeder, Knochen an, Auswaehlen", [] {
        mtab().gizmo = 0;
        g_app->modellKnochen = true;
        modellRigStilSetzen(modell::RigStil::Blender);
    }));
    add(pause(0.8));
    add(abwaehlen());
    add(pause(0.3));
    add(klickAn("AP13K T: auf den Kreis des Pols Ellbogen R", [] { return formOrt(formVonGriff(griffNr(modell::GriffArt::PolEllbogen, modell::Seite::Rechts))); }));
    add(pause(0.5));
    add(pruefe(zeile("T5", 0, "Blender-Oktaeder: ein Klick auf den Kreis waehlt den Pol (Ellbogen R)"), [] {
        return mtab().griffWahl == griffNr(modell::GriffArt::PolEllbogen, modell::Seite::Rechts);
    }));
    add(abwaehlen());
    add(pause(0.3));
    add(klickAn("AP13K T: auf den Oktaeder des Oberarms L", [] { return formOrt(formVonGriff(griffNr(modell::GriffArt::Oberarm, modell::Seite::Links))); }));
    add(pause(0.5));
    add(pruefe(zeile("T5", 0, "... und ein Klick auf den Oktaeder eines Knochens dessen FK-Griff (Oberarm L)"), [] {
        melde("T5: gewaehlt " + std::to_string(mtab().griffWahl));
        return mtab().griffWahl == griffNr(modell::GriffArt::Oberarm, modell::Seite::Links);
    }));
    // --- Standard: das Zeichen am Gelenk wie bisher ---
    add(tu("AP13K T: Stil Standard, Knochen aus", [] {
        g_app->modellKnochen = false;
        modellRigStilSetzen(modell::RigStil::Standard);
    }));
    add(pause(0.6));
    add(abwaehlen());
    add(pause(0.3));
    add(klickAn("AP13K T: auf das Zeichen der Hand R", [] {
        ImVec2 p{-50.0F, -50.0F};
        (void)modellRigGriffAufSchirm(griffNr(modell::GriffArt::Hand, modell::Seite::Rechts), p);
        return p;
    }));
    add(pause(0.5));
    add(pruefe(zeile("T6", 0, "Stil Standard: das Zeichen am Gelenk waehlt wie bisher (Hand R)"), [] { return mtab().griffWahl == griffNr(modell::GriffArt::Hand, modell::Seite::Rechts); }));
    add(abwaehlen());
    // --- Knochen waehlen (wie Blender, Ansehen/Pruefen): die Form des Knochens ---
    add(tu("AP13K T: wie Blender, Ansehen/Pruefen, Werkzeug Knochen waehlen", [] {
        grundstellung(1, ModellBereich::Ansehen);
        for (int i = 0; i < modellBlWerkzeugZahl(); ++i) {
            if (std::string(modellBlWerkzeugText(i)) == tr(Str::BzKnochenWaehlen)) { (void)modellBlWerkzeugAusloesen(i); }
        }
    }));
    add(pause(0.8));
    add(klickAn("AP13K T: mitten auf den Oktaeder des Unterschenkels L (weit von jedem Gelenk)", [] {
        const std::vector<BoneMatrix>& w = g_app->modellWelt;
        const int a = knochenNach("ltibia");
        const int b = knochenNach("ltalus");
        ImVec2 p{-50.0F, -50.0F};
        if (a < 0 || b < 0) { return p; }
        // 30 % hinter dem Knie: dort ist der Oktaeder breit
        const float m[3] = {w[static_cast<std::size_t>(a)].m[0][3] * 0.7F + w[static_cast<std::size_t>(b)].m[0][3] * 0.3F,
                            w[static_cast<std::size_t>(a)].m[1][3] * 0.7F + w[static_cast<std::size_t>(b)].m[1][3] * 0.3F,
                            w[static_cast<std::size_t>(a)].m[2][3] * 0.7F + w[static_cast<std::size_t>(b)].m[2][3] * 0.3F};
        (void)modellAufSchirm(mtab().ansichten[mtab().aktiv], m, p);
        ImVec2 knie;
        ImVec2 fuss;
        const float ka[3] = {w[static_cast<std::size_t>(a)].m[0][3], w[static_cast<std::size_t>(a)].m[1][3], w[static_cast<std::size_t>(a)].m[2][3]};
        const float kb[3] = {w[static_cast<std::size_t>(b)].m[0][3], w[static_cast<std::size_t>(b)].m[1][3], w[static_cast<std::size_t>(b)].m[2][3]};
        if (modellAufSchirm(mtab().ansichten[mtab().aktiv], ka, knie) && modellAufSchirm(mtab().ansichten[mtab().aktiv], kb, fuss)) {
            char z[160];
            std::snprintf(z, sizeof z, "T7: Klick %.0f Punkte vom Knie, %.0f vom Fuss (Zeichenhoehe %.0f)", static_cast<double>(std::hypot(p.x - knie.x, p.y - knie.y)),
                          static_cast<double>(std::hypot(p.x - fuss.x, p.y - fuss.y)), static_cast<double>(ImGui::GetFontSize()));
            melde(z);
        }
        return p;
    }));
    add(pause(0.5));
    add(pruefe(zeile("T7", 1, "Knochen waehlen trifft den Knochen an seiner Form: ein Klick mitten auf den Oktaeder (weiter als eine Zeichenhoehe von jedem Gelenk) waehlt "
                              "den Knochen, den die Form zeigt (ltalus - der Oktaeder vom Knie zum Fuss)"),
               [] {
        const int k = mtab().knochenWahl;
        melde("T7: gewaehlt " + (k >= 0 ? g_app->anim.bones[static_cast<std::size_t>(k)].name : std::string("-")));
        return k == knochenNach("ltalus");
    }));
    add(tu("AP13K T: Werkzeug Auswaehlen", [] {
        (void)modellBlWerkzeugAusloesen(0);
        mtab().knochenWahl = -1;
    }));
}

// --- Fotos (BHED_MODELL_NUR=ap13kfoto): je Bedienmodus die Stile und Arten fuer den Abgleich mit Entwurf und Vorbildern ---------
//   ap13k_<max|blender>_stil_<standard|control|cat|blender>    Rig mit Knochen in der Ansicht, die Gruppe im Feld offen
//   ap13k_<max|blender>_art_<n>                                Skelett in der Darstellungsart (wie Blender: 0-4; wie 3ds Max: Kasten, Flossen)
//   ap13k_<max|blender>_durch_<stil>                           Durchsicht, Knochen nicht im Vordergrund
//   ap13k_<max|blender>_menue                                  das Menue der Knochen offen
void fotoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    static const char* const kStil[4] = {"standard", "control", "cat", "blender"};
    auto fov = std::make_shared<float>(0.0F);
    for (int modus = 0; modus < 2; ++modus) {
        const std::string p = std::string("ap13k_") + (modus == 0 ? "max" : "blender") + "_";
        // die Kamera wie in den Vorbildern: von schraeg vorn oben, die ganze Figur mit dem Boden
        add(tu("AP13K Foto: Blick von schraeg vorn", [modus, fov] {
            modellBedienungSetzen(modus);
            mtab().vierAnsichten = false;
            mtab().aktiv = 3;
            if (*fov <= 0.0F) { *fov = g_app->modelCam.fovY; }
            g_app->modelCam.fovY = *fov;
            modellStartansicht(3);
            float w[3];
            modellAnsichtWinkel(mtab().ansichten[3], w);
            w[0] += 12.0F;
            w[1] += 24.0F;
            modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
        }));
        add(pause(0.6));
        add(tu("AP13K Foto: " + p + " - Rig, Knochen, Gruppe offen", [modus] {
            grundstellung(modus, ModellBereich::Animation);
            for (ModellAnsicht& a : mtab().ansichten) {
                a.sch.raster = true;
                a.sch.rasterBoden = true;
                a.sch.achsen[0] = a.sch.achsen[1] = true;
            }
            if (!modellRigDa()) { rigAnlegen(); }
            g_app->settings.modellFeldBreite = 0.0F;
            modellKd().oeffnen = true;
            setzeMaus(mbefoto::ansichtEcke());
        }));
        add(pause(1.0));
        add(ap10::zeige(Str::KdGruppe));
        add(pause(0.5));
        for (int st = 0; st < 4; ++st) {
            add(tu("AP13K Foto: Stil " + std::string(kStil[st]), [st] {
                modellRigStilSetzen(static_cast<modell::RigStil>(st));
                g_app->modellKnochen = true;
                for (bool& d : modellKd().durchsicht) { d = false; }
                g_app->settings.modellKnochenVorn = true;
                g_app->modelDirty = true;
            }));
            add(pause(0.8));
            add(foto(p + "stil_" + kStil[st]));
            add(tu("AP13K Foto: Durchsicht, nicht im Vordergrund", [] {
                modellDurchsichtUmschalten(mtab().aktiv);
                g_app->settings.modellKnochenVorn = false;
                g_app->modelDirty = true;
            }));
            add(pause(0.8));
            add(foto(p + "durch_" + kStil[st]));
        }
        add(tu("AP13K Foto: Stil Standard, ohne Rig-Griffe, Knochen im Vordergrund", [] {
            modellRigStilSetzen(modell::RigStil::Standard);
            for (bool& d : modellKd().durchsicht) { d = false; }
            g_app->settings.modellKnochenVorn = true;
            mtab().rigZeigen = false;
            g_app->modelDirty = true;
        }));
        if (modus == 1) {
            for (int a = 0; a < kKnochenArten; ++a) {
                add(tu("AP13K Foto: Anzeigen als " + std::to_string(a), [a] { modellKnochenArtSetzen(static_cast<KnochenArt>(a)); }));
                add(pause(0.6));
                add(foto(p + "art_" + std::to_string(a)));
            }
            add(tu("AP13K Foto: Oktaeder", [] { modellKnochenArtSetzen(KnochenArt::Oktaeder); }));
        } else {
            add(pause(0.6));
            add(foto(p + "art_kasten"));
            add(tu("AP13K Foto: Flossen", [] {
                g_app->settings.modellFlossen = true;
                g_app->modelDirty = true;
            }));
            add(pause(0.6));
            add(foto(p + "art_flossen"));
            add(tu("AP13K Foto: ohne Flossen", [] {
                g_app->settings.modellFlossen = false;
                g_app->modelDirty = true;
            }));
        }
        add(tu("AP13K Foto: Griffe wieder", [] {
            mtab().rigZeigen = true;
            g_app->modelDirty = true;
        }));
        add(pause(0.4));
        add(klickAn("AP13K Foto: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
        add(pause(0.3));
        knochenMenue(s, modus);
        add(ap5::klickPopup(tr(Str::KdRigStil)));
        add(pause(0.5));
        add(foto(p + "menue"));
        add(mbefoto::popupsZu());
        add(pause(0.3));
    }
    add(tu("AP13K Foto: Kamera wie vorher", [fov] {
        if (*fov > 0.0F) { g_app->modelCam.fovY = *fov; }
        modellStartansicht(3);
        g_app->modelDirty = true;
    }));
}

// --- Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap13k): die Bilder der Seite "Model-Tab: Knochen, Rig-Stile und Durchsicht" ---
//   h_ap13k_arten        wie Blender, Knochen als Stick, das Menue Overlays > Anzeigen als offen
//   h_ap13k_durchsicht   wie 3ds Max, Knochen nicht im Vordergrund, Durchsicht
//   h_ap13k_control / _cat / _blender   die drei neuen Rig-Stile mit Control Rig, die Gruppe im rechten Feld offen
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    auto einst = std::make_shared<Settings>();
    auto knochen = std::make_shared<bool>(false);
    add(tu("AP13K Handbuch: wie Blender, Ansehen/Pruefen, Knochen als Stick", [=] {
        *einst = g_app->settings;
        *knochen = g_app->modellKnochen;
        g_app->settings.splitModel = 0.85F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(1);
        ModellTab& t = mtab();
        t.bereich = ModellBereich::Ansehen;
        t.vierAnsichten = false;
        t.aktiv = 3;
        t.auswahl.clear();
        t.knochenWahl = -1;
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
        g_app->modellKnochen = true;
        g_app->settings.modellRigStil = 0;
        g_app->settings.modellKnochenVorn = true;
        g_app->settings.modellKnochenArt = static_cast<int>(KnochenArt::Stick);
        for (bool& d : modellKd().durchsicht) { d = false; }
        g_app->modelDirty = true;
    }));
    add(pause(1.2));
    add(klickAn("AP13K Handbuch: Overlays", ap5::kopfFeld(3)));
    add(pause(0.4));
    add(tu("AP13K Handbuch: Maus auf Anzeigen als", [] {
        const Element* e = ap5::imPopup(tr(Str::KdAnzeigenAls));
        if (e != nullptr) { setzeMaus(e->rect.GetCenter()); }
    }));
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_ap13k_arten"));
    add(mbefoto::popupsZu());
    add(tu("AP13K Handbuch: wie 3ds Max, Knochen nicht im Vordergrund, Durchsicht", [] {
        modellBedienungSetzen(0);
        mtab().vierAnsichten = false;
        g_app->settings.modellKnochenVorn = false;
        for (bool& d : modellKd().durchsicht) { d = true; }
        g_app->modelDirty = true;
        setzeMaus(ImVec2{-50.0F, -50.0F});
    }));
    add(pause(1.2));
    add(hilfe2::schrittFoto("h_ap13k_durchsicht"));
    add(tu("AP13K Handbuch: Bereich Animation, Control Rig, Stil Control Rig, Gruppe offen", [] {
        for (bool& d : modellKd().durchsicht) { d = false; }
        g_app->settings.modellKnochenVorn = true;
        g_app->settings.modellKnochenArt = 0;
        rigAnlegen();
        g_app->modellKnochen = false;
        g_app->settings.modellRigStil = static_cast<int>(modell::RigStil::ControlRig);
        modellKd().oeffnen = true;
        g_app->modelDirty = true;
    }));
    add(pause(1.5));
    add(ap10::zeige(Str::KdGruppe));
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_ap13k_control"));
    add(tu("AP13K Handbuch: Stil CAT-Kaesten, Knochen an", [] {
        g_app->modellKnochen = true;
        g_app->settings.modellRigStil = static_cast<int>(modell::RigStil::Cat);
        g_app->modelDirty = true;
    }));
    add(pause(1.0));
    add(hilfe2::schrittFoto("h_ap13k_cat"));
    add(tu("AP13K Handbuch: Stil Blender-Oktaeder", [] {
        g_app->settings.modellRigStil = static_cast<int>(modell::RigStil::Blender);
        g_app->modelDirty = true;
    }));
    add(pause(1.0));
    add(hilfe2::schrittFoto("h_ap13k_blender"));
    add(tu("AP13K Handbuch: zurueck", [=] {
        modellBedienungSetzen(einst->modellBedienung);
        g_app->settings.modellKnochenArt = einst->modellKnochenArt;
        g_app->settings.modellRigStil = einst->modellRigStil;
        g_app->settings.modellKnochenVorn = einst->modellKnochenVorn;
        g_app->settings.splitModel = einst->splitModel;
        g_app->modellKnochen = *knochen;
        for (bool& d : modellKd().durchsicht) { d = false; }
        mtab().bereich = ModellBereich::Ansehen;
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
    if (n != "ap13k" && n != "ap13kfoto") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_AP13K_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    auto skala = std::make_shared<float>(1.0F);
    auto teiler = std::make_shared<float>(0.0F);
    auto feldBreite = std::make_shared<float>(0.0F);
    auto einst = std::make_shared<Settings>();
    auto knochen = std::make_shared<bool>(false);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    add(tu("AP13K: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        if (const char* t = std::getenv("BHED_AP13K_THEMA")) {
            const bool hell = std::string(t) == "hell";
            for (const theme::Theme& th : theme::themes()) {
                if ((theme::relativeLuminance(th.window) > 0.5F) == hell) {
                    g_app->activeTheme = &th;
                    g_app->themePending = true;
                    break;
                }
            }
        }
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap13k.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    add(tu("AP13K: Aufteilung wie neue Einstellungen, Einstellungen merken", [=] {
        *teiler = g_app->settings.splitModel;
        *feldBreite = g_app->settings.modellFeldBreite;
        *einst = g_app->settings;
        *knochen = g_app->modellKnochen;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        mtab().bereich = ModellBereich::Animation;
        mtab().vierAnsichten = false;
        animWaehlen(mbefoto::sequenz(), false);
        melde(g_app->modelPath + ", " + std::to_string(g_app->anim.bones.size()) + " Knochen, " +
              (render::current() == render::Backend::Direct3D12 ? "Direct3D 12" : "Direct3D 11"));
    }));
    add(pause(1.0));
    add(klickAn("AP13K: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (n == "ap13k") {
        knochenSchritte(s);
        durchsichtSchritte(s);
        vordergrundSchritte(s);
        stilSchritte(s);
        treffSchritte(s);
    } else {
        fotoSchritte(s);
    }
    add(tu("AP13K: zurueck", [=] {
        if (*thema != nullptr && g_app->activeTheme != *thema) {
            g_app->activeTheme = *thema;
            g_app->themePending = true;
        }
        modellBedienungSetzen(einst->modellBedienung);
        g_app->settings.modellKnochenArt = einst->modellKnochenArt;
        g_app->settings.modellRigStil = einst->modellRigStil;
        g_app->settings.modellKnochenVorn = einst->modellKnochenVorn;
        g_app->settings.modellFlossen = einst->modellFlossen;
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->modellKnochen = *knochen;
        for (bool& d : modellKd().durchsicht) { d = false; }
        for (ModellAnsicht& a : mtab().ansichten) { a.sch = ModellSchattierung{}; }
        mtab().bereich = ModellBereich::Ansehen;
        g_app->uiScale = *skala;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.3));
    return s;
}

}  // namespace ap13k
