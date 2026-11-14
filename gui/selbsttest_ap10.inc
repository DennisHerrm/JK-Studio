// selbsttest_ap10.inc - AP10 (MBENTWURF Teil B, 11.11.): das rechte Feld des Model-Tabs im Bereich Animation nach dem
// freigegebenen Entwurf (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap10 | ap10foto)
//
//   ap10      je Zeile der Abgleichtabelle (plaene/modellblock_stufe0/abgleich_programm_entwurf.md, Abschnitt 12: A0a-A11, A14)
//             eine Pruefung "AP10 A<nr>": Ort (Reihenfolge und Reiter der Gruppe), Element (Tabelle, Raster, Hauptknopf, Tipp),
//             Bedienung (Klick auf den Knopf, Wirkung im Dokument) - wie 3ds Max und wie Blender.
//   ap10foto  nur Fotos (BHED_FOTOS=<Ordner>): das ganze Fenster und das rechte Feld in Seiten, je Bedienmodus (ap10_max_*,
//             ap10_blender_*), mit Rig, Preset-Vorschau und Quelle des Uebertragens.
//   BHED_AP10_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe wie der Foto-Selbsttest mbefoto)
//   BHED_AP10_MODELL=<.glm>           Vorgabe models/players/kyle/model.glm (das Modell des Entwurfs)
//   BHED_AP10_KREATUR=<.glm>          Vorgabe models/players/rancor/model.glm (Rig-Vorlage eines eigenen Skeletts)
//   BHED_AP10_THEMA=hell|dunkel       das erste helle bzw. dunkle Farbschema (Vorgabe: das eingestellte)
// Eingebunden in selbsttest.cpp (nach selbsttest_ap3.inc).
namespace ap10 {

using mbefoto::inFenster;
using mbefoto::kindFenster;

float em() { return ImGui::GetFontSize(); }

void melde(const std::string& z) { diag::detail("Knopftest: AP10 " + z); }

std::string zeile(const char* nr, int modus, const std::string& was) { return std::string("AP10 ") + nr + (modus == 0 ? " wie 3ds Max: " : " wie Blender: ") + was; }

// Die acht Gruppen des Entwurfs in seiner Reihenfolge.
const Str kEntwurf[8] = {Str::AzRAnimation, Str::AzRRig, Str::PrRollout, Str::AzRBearbeiten, Str::AzRUebertragen, Str::MbIkRollout, Str::AzRAnbauten, Str::AzRBauen};

const ui::RolloutProbe* gruppe(const std::string& titel) {
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
        if (ui::rolloutProbe(i).titel == titel) { return &ui::rolloutProbe(i); }
    }
    return nullptr;
}
const ui::RolloutProbe* gruppe(Str titel) { return gruppe(std::string(tr(titel))); }

int gruppeNr(Str titel) {
    const std::string name = tr(titel);
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
        if (ui::rolloutProbe(i).titel == name) { return i; }
    }
    return -1;
}

// Die Titel der gezeichneten Gruppen des rechten Felds, mit | getrennt.
std::string gruppen() {
    std::string s;
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) { s += (i > 0 ? "|" : "") + ui::rolloutProbe(i).titel; }
    return s;
}

std::string folge(std::initializer_list<Str> liste) {
    std::string s;
    for (const Str t : liste) { s += (s.empty() ? "" : "|") + std::string(tr(t)); }
    return s;
}

const Element* imRoll(const std::string& label, int nte = 0) { return finde(label, "mtrollouts", nte, false); }

bool inGruppe(const ui::RolloutProbe* g, const Element* e) { return g != nullptr && e != nullptr && g->kasten.Contains(e->rect); }

// ein Element dieser Gruppe mit der Beschriftung (dieselbe Beschriftung kann in mehreren Gruppen stehen: "Source" in Bearbeiten
// und in Uebertragen)
const Element* imRollIn(const ui::RolloutProbe* g, const std::string& label) {
    for (int n = 0; n < 8; ++n) {
        const Element* e = imRoll(label, n);
        if (e == nullptr) { return nullptr; }
        if (inGruppe(g, e)) { return e; }
    }
    return nullptr;
}

// zwei Knoepfe nebeneinander in einem Raster: gleiche Zeile, gleiche Breite, der zweite rechts vom ersten
bool nebeneinander(const Element* a, const Element* b) {
    return a != nullptr && b != nullptr && std::fabs(a->rect.Min.y - b->rect.Min.y) < 0.5F && std::fabs(a->rect.GetWidth() - b->rect.GetWidth()) < 0.5F && b->rect.Min.x > a->rect.Max.x;
}

// zwei Eintraege einer Wahl (ui::segment) nebeneinander: gleiche Reihe, die Zellen stossen aneinander (keine Luecke wie im
// Raster), gleich breit bis auf einen Bildpunkt (die Zellgrenzen liegen auf ganzen Bildpunkten: floor(breite * spalte / n))
bool segmentNachbarn(const Element* a, const Element* b) {
    return a != nullptr && b != nullptr && std::fabs(a->rect.Min.y - b->rect.Min.y) < 0.5F && std::fabs(a->rect.GetWidth() - b->rect.GetWidth()) <= 1.0F &&
           std::fabs(b->rect.Min.x - a->rect.Max.x) < 0.5F;
}

// ein Hauptknopf: ueber die ganze Breite der Gruppe (bis auf ihr Polster)
bool volleBreite(const ui::RolloutProbe* g, const Element* e) { return inGruppe(g, e) && e->rect.GetWidth() >= g->kasten.GetWidth() - em() * 1.4F; }

// Eine Gruppe oben ins Feld rollen (Elemente ausserhalb des sichtbaren Teils meldet ImGui nicht).
// TEST1411 N: steht die Gruppe nicht im Bild, waehlt der Schritt erst ihren Ort wie der Anwender - Ghosts und Knochen- und Rig-Darstellung
// im Reiter Anzeige (Blender: Daten), sonst das Motion-Feld und, wenn sie dort fehlt, der andere Modus (Setup/Animation wie CAT) - und
// rollt danach.
Schritt zeige(Str t, float tiefer = 0.0F) {
    auto umgeschaltet = std::make_shared<bool>(false);
    return Schritt{std::string("AP10: Gruppe ins Bild - ") + tr(t), [=](int b) {
                       const bool anzeige = t == Str::KdGruppe || t == Str::MbRgGhosts;
                       const bool animation = g_app->mtab.bereich == ModellBereich::Animation;
                       if (b == 0) { *umgeschaltet = false; }
                       if (animation && gruppe(t) == nullptr && (b == 0 || (b == 3 && !anzeige && !*umgeschaltet))) {
                           const int motion = modellBlender() ? static_cast<int>(MbReiterBl::Knochen) : static_cast<int>(MbReiter::Bewegung);
                           if (anzeige) {
                               modellFeldReiter(MbReiter::Anzeige, MbReiterBl::Daten);
                           } else if (b == 0 && modellFeld().reiter != motion) {
                               modellFeldReiter(MbReiter::Bewegung, MbReiterBl::Knochen);
                           } else {
                               modellAnimSetupSetzen(!modellAnimSetupGewaehlt());
                               *umgeschaltet = true;
                           }
                           return false;
                       }
                       if (b > 0 && b < 6 && gruppe(t) == nullptr) { return false; }
                       const ui::RolloutProbe* g = gruppe(t);
                       ImGuiWindow* w = kindFenster("mtrollouts");
                       if (g != nullptr && w != nullptr) { ImGui::SetScrollY(w, std::clamp(w->Scroll.y + g->kasten.Min.y - w->InnerRect.Min.y - em() * 0.4F + tiefer * em(), 0.0F, w->ScrollMax.y)); }
                       return true;
                   }};
}

// TEST1411 N: die Gruppen des Entwurfs, die im Animationsmodus im Motion-Feld stehen (Rig erstellen, Anbauten und Bauen: Setup-Modus)
bool imAnimationsmodus(Str t) { return t != Str::AzRRig && t != Str::AzRAnbauten && t != Str::AzRBauen; }

int ikGriffe() {
    int n = 0;
    for (const modell::Griff& g : g_app->mtab.rig.griffe) { n += g.ik ? 1 : 0; }
    return n;
}

int belegt() {
    int n = 0;
    for (const modell::Platz& p : g_app->mtab.plaetze) { n += p.belegt ? 1 : 0; }
    return n;
}

int schluessel() {
    const modell::RigSchicht* s = modellRigSchichtJetzt();
    return s != nullptr ? s->schluesselZahl() : 0;
}

const AnimEntry* sequenzJetzt() {
    if (g_app->animIndex < 0 || static_cast<std::size_t>(g_app->animIndex) >= g_app->animList.size()) { return nullptr; }
    return &g_app->animList[static_cast<std::size_t>(g_app->animIndex)];
}

bool tooltipDa() {
    for (const ImGuiWindow* w : GImGui->Windows) {
        if ((w->Flags & ImGuiWindowFlags_Tooltip) != 0 && w->Active && !w->Hidden) { return true; }
    }
    return false;
}

// groesster Abstand zweier Posen (Orte aller Knochen)
float poseAbstand(const std::vector<BoneMatrix>& a, const std::vector<BoneMatrix>& b) {
    if (a.size() != b.size() || a.empty()) { return 1e9F; }
    float m = 0.0F;
    for (std::size_t i = 0; i < a.size(); ++i) {
        float d = 0.0F;
        for (int k = 0; k < 3; ++k) { d += (a[i].m[k][3] - b[i].m[k][3]) * (a[i].m[k][3] - b[i].m[k][3]); }
        m = std::max(m, std::sqrt(d));
    }
    return m;
}

// --- Ort und Form: je Bedienmodus --------------------------------------------------------------------------------------

void formSchritte(std::vector<Schritt>& s, int modus) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu(std::string("AP10: Bedienmodus ") + (modus == 0 ? "wie 3ds Max" : "wie Blender") + ", Bereich Animation, Sequenz, Saebel an *r_hand", [=] {
        modellBedienungSetzen(modus);
        g_app->mtab.bereich = ModellBereich::Animation;
        g_app->settings.modellFeldBreite = 0.0F;
        if (g_app->mtab.anbauten.empty()) { (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true); }
        animWaehlen(mbefoto::sequenz(), false);
        // der Zustand eines frisch geladenen Modells: kein Rig, keine Quelle des Uebertragens, Karten-GLA als Ziel
        g_app->mtab.rig = modell::Rig{};
        g_app->mtab.rigFuer.clear();
        g_app->mtab.griffWahl = -1;
        g_app->mtab.rt = ModellRetarget{};
        modellRigNurFkSetzen(false);
        modellAnim().generatorOffen = false;
        std::snprintf(g_app->mtab.kgKarte, sizeof g_app->mtab.kgKarte, "%s", "ap10test");
        if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
    }));
    add(pause(0.8));
    // --- A0a / A0b: Reiter und Reihenfolge ---
    add(pruefe(zeile(modus == 0 ? "A0a" : "A0b", modus,
                     modus == 0 ? "Befehlsfeld, Reiter Bewegung gewaehlt; die Gruppen Animation, Rig erstellen, Presets, Vorhandene Animation bearbeiten, Uebertragen, IK/FK, Anbauten und Vorschau, Fuer das Spiel bauen untereinander in dieser Reihenfolge"
                                : "Eigenschaften, Reiter Knochen gewaehlt; die acht Gruppen des Entwurfs untereinander in seiner Reihenfolge"),
               [=] {
                   const ui::FeldProbe& f = ui::feldProbe();
                   std::string soll;
                   for (const Str t : kEntwurf) {
                       if (imAnimationsmodus(t)) { soll += (soll.empty() ? "" : "|") + std::string(tr(t)); }   // TEST1411 N
                   }
                   melde(std::string(modus == 0 ? "A0a" : "A0b") + " Gruppen: " + gruppen() + " - Reiter " + std::to_string(f.reiterAktiv));
                   return ap3::feldFrisch() && f.reiterAktiv == (modus == 0 ? static_cast<int>(MbReiter::Bewegung) : static_cast<int>(MbReiterBl::Knochen)) &&
                          gruppen().compare(0, soll.size(), soll) == 0;
               }));
    add(pruefe(zeile(modus == 0 ? "A0a" : "A0b", modus, modus == 0 ? "alle acht sind Gruppen des Bausteins im Stil wie 3ds Max (flacher Balken, eckig), ab Werk offen"
                                                                   : "alle acht sind Gruppen des Bausteins im Stil wie Blender (abgerundeter Kasten), ab Werk offen"),
               [=] {
                   bool ok = true;
                   for (const Str t : kEntwurf) {
                       if (!imAnimationsmodus(t)) { continue; }   // TEST1411 N: Rig erstellen, Anbauten und Bauen im Setup-Modus (dort geprueft)
                       const ui::RolloutProbe* g = gruppe(t);
                       ok = ok && g != nullptr && g->offen && g->stil == (modus == 0 ? ui::RolloutStil::Max : ui::RolloutStil::Blender) &&
                            (modus == 0 ? g->rundung <= 2.0F : g->rundung >= std::floor(em() * 0.3F));
                   }
                   return ok;
               }));
    // --- A14: was das Programm mehr kann, steht dahinter ---
    add(pruefe(zeile("A14", modus, "hinter den acht Gruppen des Entwurfs: Pose, Ghosts, Bewegungspfade, Knochen- und Rig-Darstellung, Kurven und Animationswerkzeuge, Posen-Bibliothek, GLA, animation.cfg, Ziel-GLA und Plaetze - alle Gruppen des Bausteins (ohne Rig keine Auto-Physik)"),
               [] {
                   std::string soll;
                   for (const Str t : kEntwurf) {
                       if (imAnimationsmodus(t)) { soll += std::string(tr(t)) + "|"; }
                   }
                   // TEST1411 N: im Animationsmodus; Ghosts und Knochen- und Rig-Darstellung stehen im Reiter Anzeige (Blender: Daten), GLA,
                   // animation.cfg, Ziel-GLA und Plaetze im Setup-Modus und im Reiter Dienstprogramme
                   soll += folge({Str::MbRgGruppePose, Str::MbRgPfade, Str::MbKvRollout, Str::MbKvGruppeBibliothek});
                   // kein Element des Felds ist mehr eine Gruppe ohne den Baustein (ImGui::CollapsingHeader): jede aufklappbare Kopfzeile gehoert zu einer Probe
                   int fremd = 0;
                   for (const Element& e : g_elemente) {
                       if (e.innen.find("mtrollouts") == std::string::npos || (e.flags & ImGuiItemStatusFlags_Openable) == 0) { continue; }
                       bool gefunden = false;
                       for (int i = 0; i < ui::rolloutProbeZahl(); ++i) { gefunden = gefunden || ui::rolloutProbe(i).id == e.id; }
                       // die eingeklappten Teile "Optionen" des Bausteins (mehrBeginn) und Auswahllisten ohne Beschriftung zaehlen nicht
                       if (!gefunden && e.label != tr(Str::AzOptionen) && !e.label.empty() && e.label.compare(0, 2, "##") != 0) { ++fremd; }
                   }
                   melde("A14 Gruppen: " + gruppen() + " - Kopfzeilen ohne Baustein " + std::to_string(fremd));
                   return gruppen() == soll && fremd == 0;
               }));
    // --- A1: Gruppe Animation ---
    add(zeige(Str::AzRAnimation));
    add(pause(0.3));
    add(pruefe(zeile("A1", modus, "Gruppe Animation ganz oben: Name, Ziel-GLA und Plaetze als Zeilen einer Tabelle, die Kurzinfo nennt die Sequenz"), [] {
        const ui::RolloutProbe* g = gruppe(Str::AzRAnimation);
        const AnimEntry* e = sequenzJetzt();
        if (g == nullptr || e == nullptr) { return false; }
        bool ok = gruppeNr(Str::AzRAnimation) == 0 && g->kurzinfo == e->name;
        float y = g->kopf.Max.y;
        for (const Str z : {Str::AzName, Str::AzZielGla, Str::AzPlaetze}) {
            const Element* x = imRoll(tr(z));
            ok = ok && inGruppe(g, x) && x->rect.Min.y >= y - 0.5F && x->rect.GetWidth() >= g->kasten.GetWidth() - em() * 1.4F;
            if (x != nullptr) { y = x->rect.Max.y; }
        }
        melde("A1 Animation: Kurzinfo " + g->kurzinfo + ", " + mbefoto::rechteck("kasten", g->kasten));
        return ok;
    }));
    // --- A2: Gruppe Rig erstellen ---
    add(zeige(Str::AzRRig));
    add(pause(0.3));
    add(pruefe(zeile("A2", modus, "Gruppe Rig erstellen an zweiter Stelle, Kurzinfo = Zahl der Vorlagen (mindestens die 34 Skelette des Basisspiels); Tabelle Skelett, Massstab (fScale), Erkannt als"), [] {
        const ui::RolloutProbe* g = gruppe(Str::AzRRig);
        if (g == nullptr) { return false; }
        const std::vector<modell::RtVorlageInfo> v = modell::rtVorlagen();
        int spiel = 0;
        for (const modell::RtVorlageInfo& x : v) { spiel += !x.datei.empty() && x.name != "_humanoid (JK2)" ? 1 : 0; }
        char info[64];
        std::snprintf(info, sizeof info, trN(Str::AzVorlagenInfo, static_cast<int>(v.size())), static_cast<int>(v.size()));
        bool ok = gruppeNr(Str::AzRRig) == 0 && g->kurzinfo == info && spiel >= 34;
        for (const Str z : {Str::AzSkelett, Str::AzMassstab, Str::AzErkannt}) { ok = ok && inGruppe(g, imRoll(tr(z))); }
        melde("A2 Rig erstellen: Kurzinfo " + g->kurzinfo + ", Vorlagen der Spieldateien " + std::to_string(spiel));
        return ok;
    }));
    add(pruefe(zeile("A2", modus, "Auswahl der Vorlage: die Gruppen (Mensch, Rancor, Wampa, Tauntaun, Howler, ..., Droiden, Fahrzeuge/Schiffe) und Eigenes Skelett als Wahl in zwei Spalten; fuer kyle ist Mensch gewaehlt (erkannt: _humanoid)"), [] {
        const ui::RolloutProbe* g = gruppe(Str::AzRRig);
        const Element* mensch = imRoll(tr(Str::AzVorMensch));
        const Element* rancor = imRoll("Rancor");
        const Element* wampa = imRoll("Wampa");
        const Element* droiden = imRoll(tr(Str::AzVorDroiden));
        const Element* fahrzeuge = imRoll(tr(Str::AzVorFahrzeuge));
        const Element* eigen = imRoll(tr(Str::AzVorEigen));
        if (g == nullptr || mensch == nullptr || rancor == nullptr || wampa == nullptr || droiden == nullptr || fahrzeuge == nullptr || eigen == nullptr) { return false; }
        const int erkannt = modellAnimVorlageErkannt();
        melde("A2 Vorlage: erkannt " + (erkannt >= 0 ? modell::rtVorlagen()[static_cast<std::size_t>(erkannt)].name : std::string("-")) + ", " + mbefoto::rechteck("mensch", mensch->rect) + " " +
              mbefoto::rechteck("rancor", rancor->rect) + " " + mbefoto::rechteck("wampa", wampa->rect));
        return inGruppe(g, mensch) && inGruppe(g, eigen) && segmentNachbarn(mensch, rancor) && wampa->rect.Min.y > mensch->rect.Min.y && std::fabs(wampa->rect.Min.x - mensch->rect.Min.x) < 0.5F &&
               (mensch->flags & ImGuiItemStatusFlags_Checked) != 0 && (eigen->flags & ImGuiItemStatusFlags_Checked) == 0 && erkannt >= 0 &&
               modell::rtVorlagen()[static_cast<std::size_t>(erkannt)].gruppe == "human";
    }));
    add(pruefe(zeile("A2", modus, "ein blauer Hauptknopf Control Rig erstellen ueber die Breite, darunter Animation-Rig (nur FK) und Auf Rig legen im Raster mit Symbol, am Ende der Tipp als Kasten"), [] {
        const ui::RolloutProbe* g = gruppe(Str::AzRRig);
        const Element* haupt = imRoll(tr(modellRigDa() && !modellRigNurFk() ? Str::MbRgNeu : Str::AzRigErstellen));
        const Element* fk = imRoll(tr(Str::AzRigFk));
        const Element* auf = imRoll(tr(Str::AzAufRig));
        const Element* tipp = imRoll(tr(Str::AzRigTipp));
        if (g == nullptr) { return false; }
        melde("A2 Knoepfe: Hauptknoepfe " + std::to_string(g->hauptKnoepfe) + ", Knoepfe " + std::to_string(g->knoepfe) + ", Symbole " + std::to_string(g->symbole) + ", Spalten " +
              std::to_string(g->rasterSpalten));
        return g->hauptKnoepfe == 1 && g->symbole == g->knoepfe && g->rasterSpalten == 2 && volleBreite(g, haupt) && nebeneinander(fk, auf) && inGruppe(g, fk) && fk->rect.Min.y >= haupt->rect.Max.y &&
               inGruppe(g, tipp) && tipp->rect.Min.y >= fk->rect.Max.y;
    }));
    // --- A3: Gruppe Presets ---
    add(zeige(Str::PrRollout));
    add(pause(0.3));
    add(pruefe(zeile("A3", modus, "Gruppe Presets an dritter Stelle, Kurzinfo Kategorie und Zahl; die Kategorien als Wahl in zwei Spalten, darunter die Liste, der Ziel-Platz, Vorschau auf Figur und Generator im Raster, der Hauptknopf Auf Figur legen (Retarget), der Tipp"), [] {
        const ui::RolloutProbe* g = gruppe(Str::PrRollout);
        const ImGuiWindow* liste = kindFenster("##aibib");
        const ImRect platz = modellPresetPlatzKasten();
        if (g == nullptr || liste == nullptr || platz.GetHeight() <= 0.0F) { return false; }
        // "Idle" ist die Kategorie und das erste Preset der Liste: die Kategorie ist der Eintrag ueber der Liste
        const Element* stehen = nullptr;
        for (int n = 0; n < 4 && stehen == nullptr; ++n) {
            const Element* e = imRoll(tr(Str::AiKatStehen), n);
            if (e != nullptr && e->rect.Max.y <= liste->Pos.y) { stehen = e; }
        }
        const Element* gehen = imRoll(tr(Str::AzKatGehen));
        const Element* vorschau = imRoll(tr(Str::AzPresetVorschau));
        const Element* generator = imRoll(tr(Str::AzGenerator));
        const Element* haupt = imRoll(tr(Str::AzPresetAuflegen));
        const Element* tipp = imRoll(tr(Str::AzPresetTipp));
        const std::vector<PresetEintrag>& b = modellPresetBibliothek();
        int inKat = 0;
        std::string erste;
        for (const PresetEintrag& e : b) {
            if (erste.empty()) { erste = e.kategorie; }
            inKat += e.kategorie == erste ? 1 : 0;
        }
        const bool teil[] = {gruppeNr(Str::PrRollout) == 1,
                             g->kurzinfo == std::string(tr(Str::AiKatStehen)) + " \xc2\xb7 " + std::to_string(inKat),
                             segmentNachbarn(stehen, gehen) && inGruppe(g, stehen),
                             stehen != nullptr && liste->Pos.y >= stehen->rect.Max.y,
                             platz.Min.y >= liste->Pos.y + liste->Size.y - 1.0F,
                             nebeneinander(vorschau, generator) && vorschau->rect.Min.y >= platz.Max.y,
                             g->hauptKnoepfe == 1 && volleBreite(g, haupt) && haupt->rect.Min.y >= vorschau->rect.Max.y,
                             inGruppe(g, tipp) && haupt != nullptr && tipp->rect.Min.y >= haupt->rect.Max.y};
        std::string fehlt;
        for (std::size_t i = 0; i < std::size(teil); ++i) { fehlt += teil[i] ? "" : " " + std::to_string(i); }
        melde("A3 Presets: Kurzinfo " + g->kurzinfo + ", " + std::to_string(b.size()) + " Presets, " + mbefoto::rechteck("liste", liste->Rect()) + " " + mbefoto::rechteck("platz", platz) +
              (stehen != nullptr ? " " + mbefoto::rechteck("stehen", stehen->rect) : std::string()) + (gehen != nullptr ? " " + mbefoto::rechteck("gehen", gehen->rect) : std::string()) +
              ", Teile ohne Erfolg:" + (fehlt.empty() ? std::string(" keine") : fehlt));
        return fehlt.empty();
    }));
    add(pruefe(zeile("A3", modus, "die Bibliothek steht oben, der Generator ist zugeklappt (keine Regler Tempo, Armschwung)"), [] {
        return !modellAnim().generatorOffen && imRoll(tr(Str::PrTempo)) == nullptr && imRoll(tr(Str::PrArm)) == nullptr;
    }));
    // --- A4: Vorhandene Animation bearbeiten ---
    add(zeige(Str::AzRBearbeiten));
    add(pause(0.3));
    add(pruefe(zeile("A4", modus, "Gruppe Vorhandene Animation bearbeiten an vierter Stelle: Tabelle Quelle und Bilder, die Knoepfe Frames zu Schluesseln reduzieren und In die GLA backen nebeneinander im Raster, der Tipp"), [] {
        const ui::RolloutProbe* g = gruppe(Str::AzRBearbeiten);
        const Element* red = imRoll(tr(Str::MbKvReduzieren));
        const Element* backen = imRoll(tr(Str::MbRgBacken));
        const Element* tipp = imRoll(tr(Str::AzBearbTipp));
        if (g == nullptr) { return false; }
        melde("A4 Bearbeiten: Knoepfe " + std::to_string(g->knoepfe) + ", Spalten " + std::to_string(g->rasterSpalten));
        return gruppeNr(Str::AzRBearbeiten) == 2 && inGruppe(g, imRoll(tr(Str::AzQuelle))) && inGruppe(g, imRoll(tr(Str::AzBilder))) && nebeneinander(red, backen) && inGruppe(g, red) &&
               g->rasterSpalten == 2 && g->symbole == g->knoepfe && inGruppe(g, tipp) && tipp->rect.Min.y >= red->rect.Max.y;
    }));
    // --- A5: Uebertragen (ohne Quelle) ---
    add(zeige(Str::AzRUebertragen));
    add(pause(0.3));
    add(pruefe(zeile("A5", modus, "Gruppe Uebertragen (Retargeting) an fuenfter Stelle; ohne Quelle: Quelle waehlen und Animation importieren im Raster, der Hinweis als Kasten"), [] {
        const ui::RolloutProbe* g = gruppe(Str::AzRUebertragen);
        if (g == nullptr || !g_app->mtab.rt.quelle.bones.empty()) { return false; }
        return gruppeNr(Str::AzRUebertragen) == 3 && nebeneinander(imRoll(tr(Str::RtQuelleWaehlen)), imRoll(tr(Str::AiImport))) && inGruppe(g, imRoll(tr(Str::RtQuelleKeine))) &&
               g->kurzinfo == tr(Str::AzRtInfoKeine);
    }));
    // --- A6 / A7: IK/FK (ohne Rig: Hinweis) ---
    add(zeige(Str::MbIkRollout));
    add(pause(0.3));
    add(pruefe(zeile(modus == 0 ? "A6" : "A7", modus, "Gruppe IK/FK an sechster Stelle; ohne Rig steht dort der Hinweis als Kasten"), [] {
        const ui::RolloutProbe* g = gruppe(Str::MbIkRollout);
        // AP14: ui::absatz steht jetzt im Element-Register - derselbe Hinweis steht als Absatz auch in der Gruppe Rig weiter oben,
        // imRoll faende diesen zuerst; gesucht ist der in der Gruppe IK/FK
        return g != nullptr && gruppeNr(Str::MbIkRollout) == 4 && (modellRigDa() || imRollIn(g, tr(Str::MbRgNochKeins)) != nullptr);
    }));
    // --- A8: Anbauten und Vorschau ---
    add(zeige(Str::AzRAnbauten));
    add(pause(0.3));
    add(pruefe(zeile("A8", modus, "Gruppe Anbauten und Vorschau an siebter Stelle: Tabelle Anbauten (saber_1 an *r_hand), Primaer, Sekundaer; Kurzinfo = Zahl der Anbauten"), [] {
        const ui::RolloutProbe* g = gruppe(Str::AzRAnbauten);
        if (g == nullptr) { return false; }
        return gruppeNr(Str::AzRAnbauten) == 1 && g->kurzinfo == std::to_string(g_app->mtab.anbauten.size()) && g_app->mtab.anbauten.size() == 1U && inGruppe(g, imRoll(tr(Str::MbRAnbauten))) &&
               inGruppe(g, imRoll(tr(Str::MbPrimaer))) && inGruppe(g, imRoll(tr(Str::AzSekundaer)));
    }));
    // --- A9: Fuer das Spiel bauen ---
    add(zeige(Str::AzRBauen));
    add(pause(0.3));
    add(pruefe(zeile("A9", modus, "Gruppe Fuer das Spiel bauen an achter Stelle: der Wurzelweg als Zeile, ein blauer Hauptknopf ueber die Breite, der Tipp zu SET_ORIGIN"), [] {
        const ui::RolloutProbe* g = gruppe(Str::AzRBauen);
        const Element* haupt = imRoll(tr(Str::MbKgBauen));
        const Element* weg = imRoll(tr(Str::AzWurzelweg));
        const Element* tipp = imRoll(tr(Str::AzBauenTipp));
        if (g == nullptr) { return false; }
        return gruppeNr(Str::AzRBauen) == 2 && g->hauptKnoepfe == 1 && volleBreite(g, haupt) && inGruppe(g, weg) && haupt->rect.Min.y >= weg->rect.Max.y && inGruppe(g, tipp) &&
               tipp->rect.Min.y >= haupt->rect.Max.y;
    }));
    // --- A14: jede Gruppe nennt ihren Reiter ---
    const Str reiterA = modus == 0 ? Str::MfHierarchie : Str::MfConstraints;
    const Str reiterB = modus == 0 ? Str::MfDienste : Str::MfDaten;
    const Str reiterStd = modus == 0 ? Str::MfBewegung : Str::MfKnochen;
    const std::string sollA = modus == 0 ? folge({Str::AzRRig, Str::MbIkRollout, Str::AzRAnbauten}) : folge({Str::MbIkRollout, Str::AzRAnbauten});
    const std::string sollB = modus == 0 ? folge({Str::AzRBauen, Str::MbKvGruppeBibliothek, Str::MbRGla, Str::MbRCfg, Str::MbRKartenGla})
                                         : folge({Str::MbRgGhosts, Str::MbRgPfade, Str::KdGruppe, Str::MbKvGruppeBibliothek, Str::MbRGla, Str::MbRCfg,
                                                  Str::MbRKartenGla});   // AP13K: Knochen- und Rig-Darstellung im Reiter Daten
    add(zeige(Str::AzRAnimation));   // TEST1411 N: zurueck in den Animationsmodus
    add(klick(tr(reiterA), "mtbefehlsfeld"));
    add(pause(0.4));
    add(pruefe(zeile("A14", modus, modus == 0 ? "der Reiter Hierarchie zeigt Rig erstellen, IK/FK, Anbauten und Vorschau" : "der Reiter Constraints zeigt IK/FK, Anbauten und Vorschau"), [=] {
        melde("A14 Reiter A: " + gruppen());
        return gruppen() == sollA;
    }));
    add(klick(tr(reiterB), "mtbefehlsfeld"));
    add(pause(0.4));
    add(pruefe(zeile("A14", modus, modus == 0 ? "der Reiter Dienstprogramme zeigt Fuer das Spiel bauen, Posen-Bibliothek, GLA, animation.cfg, Ziel-GLA und Plaetze"
                                              : "der Reiter Daten zeigt Ghosts, Bewegungspfade, Knochen- und Rig-Darstellung, Posen-Bibliothek, GLA, animation.cfg, Ziel-GLA und Plaetze"),
               [=] {
                   melde("A14 Reiter B: " + gruppen());
                   return gruppen() == sollB;
               }));
    add(klick(tr(reiterStd), "mtbefehlsfeld"));
    add(pause(0.4));
}

// --- Bedienung: Rig erstellen (A2), IK/FK (A6, A7), Ansicht (A10) ------------------------------------------------------------

void rigSchritte(std::vector<Schritt>& s, int modus) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(zeige(Str::AzRRig));
    add(pause(0.3));
    add(ap3::insFeld(tr(Str::AzRigErstellen)));
    add(pause(0.3));
    add(klick(tr(Str::AzRigErstellen), "mtrollouts"));
    add(pause(0.6));
    add(pruefe(zeile("A2", modus, "Klick auf Control Rig erstellen legt das Rig mit IK-Zielen an; der Hauptknopf heisst danach Neu erkennen"), [] {
        melde("A2 Control Rig: " + std::to_string(g_app->mtab.rig.griffe.size()) + " Griffe, davon " + std::to_string(ikGriffe()) + " IK, erkannt " + std::to_string(g_app->mtab.rig.erkennung.erkannt) +
              ", unklar " + std::to_string(g_app->mtab.rig.erkennung.unklar));
        return modellRigDa() && ikGriffe() > 0 && !modellRigNurFk() && imRoll(tr(Str::MbRgNeu)) != nullptr && imRoll(tr(Str::AzRigErstellen)) == nullptr;
    }));
    // --- A10: die Griffe in der Ansicht ---
    add(pruefe(zeile("A10", modus, "die Ansicht zeigt die Griffe des Rigs (FK, IK-Ziele, Pole)"), [] {
        melde("A10 Ansicht: " + std::to_string(g_app->mtab.griffeGezeichnet) + " Griffe gezeichnet");
        return g_app->mtab.rigZeigen && g_app->mtab.griffeGezeichnet > 0;
    }));
    // --- A6 / A7: IK/FK ---
    add(zeige(Str::MbIkRollout));
    add(pause(0.4));
    if (modus == 0) {
        add(pruefe(zeile("A6", modus, "je Glied eine Zeile: Name, Zustand, rechts ein Knopf mit dem Zustand (IK - Control Rig erstellen legt die Animation von FK auf IK, ENT2); darunter der Tipp zu Greifen = IK, Drehen = FK"), [] {
            const ui::RolloutProbe* g = gruppe(Str::MbIkRollout);
            if (g == nullptr) { return false; }
            bool ok = inGruppe(g, imRoll(tr(Str::AzIkTippMax)));
            float y = g->kopf.Max.y;
            static const Str kGlied[4] = {Str::MbIkArmL, Str::MbIkArmR, Str::MbIkBeinL, Str::MbIkBeinR};
            for (int i = 0; i < 4; ++i) {
                const Element* name = imRoll(tr(kGlied[i]));
                const Element* knopf = imRoll("IK##azik", i);
                ok = ok && inGruppe(g, name) && inGruppe(g, knopf) && name->rect.Min.y >= y - 0.5F && knopf->rect.Min.y < name->rect.Max.y && knopf->rect.Max.y > name->rect.Min.y &&
                     knopf->rect.Min.x > name->rect.Max.x && knopf->rect.Max.x >= g->kasten.Max.x - em() * 1.0F;
                if (name != nullptr) { y = name->rect.Max.y; }
            }
            melde("A6 IK/FK: " + std::string(ok ? "vier Zeilen mit Knopf IK rechts" : "Zeilen fehlen oder liegen falsch"));
            return ok;
        }));
        auto vorWechsel = std::make_shared<modell::RigSchicht>();
        add(tu("AP10 A6: Schicht vor dem Wechsel merken", [vorWechsel] {
            if (const modell::RigSchicht* sch = modellRigSchichtJetzt()) { *vorWechsel = *sch; }
        }));
        add(klick("IK##azik", "mtrollouts", 0));
        add(pause(0.5));
        add(pruefe(zeile("A6", modus, "ein Klick auf den Knopf von Arm L wechselt ab diesem Bild auf FK: der Knopf zeigt FK, die anderen drei IK, die Pose springt nicht"), [vorWechsel] {
            const modell::RigSchicht* sch = modellRigSchichtJetzt();
            const int spur = sch != nullptr ? modell::ikSpurIndex(*sch, modell::IkArt::ArmLinks) : -1;
            const float sprung = sch != nullptr && !vorWechsel->ik.empty() ? modell::ikSprung(g_app->anim, *vorWechsel, *sch, g_app->animFrame) : 1e9F;
            melde("A6 Wechsel: Anteil " + std::to_string(modellIkAnteil(0)) + ", Spur " + std::to_string(spur) + ", Sprung " + std::to_string(sprung));
            return modellIkAnteil(0) < 0.5F && spur >= 0 && sprung < 0.01F && imRoll("FK##azik") != nullptr && imRoll("IK##azik", 2) != nullptr && imRoll("IK##azik", 3) == nullptr;
        }));
        add(tu("AP10 A6: Wechsel zuruecknehmen", [] { (void)g_app->mtab.dok.rueckgaengig(); }));
        add(pause(0.3));
    } else {
        add(pruefe(zeile("A7", modus, "je Glied eine Zeile: Name, Regler IK-FK und die Knoepfe IK -> FK und FK -> IK in derselben Zeile (kein Umbruch); darunter der Tipp"), [] {
            const ui::RolloutProbe* g = gruppe(Str::MbIkRollout);
            if (g == nullptr) { return false; }
            bool ok = inGruppe(g, imRoll(tr(Str::AzIkTippBlender)));
            static const Str kGlied[4] = {Str::MbIkArmL, Str::MbIkArmR, Str::MbIkBeinL, Str::MbIkBeinR};
            for (int i = 0; i < 4; ++i) {
                const Element* name = imRoll(tr(kGlied[i]));
                const Element* regler = imRoll("##ikfk", i);
                const Element* a = imRoll(tr(Str::MbIkKnopfIkFk), i);
                const Element* b = imRoll(tr(Str::MbIkKnopfFkIk), i);
                ok = ok && inGruppe(g, name) && inGruppe(g, regler) && inGruppe(g, a) && inGruppe(g, b) && std::fabs(regler->rect.Min.y - a->rect.Min.y) < 0.5F &&
                     std::fabs(a->rect.Min.y - b->rect.Min.y) < 0.5F && regler->rect.Min.x > name->rect.Max.x - 0.5F && a->rect.Min.x > regler->rect.Max.x && b->rect.Min.x > a->rect.Max.x &&
                     regler->rect.GetWidth() >= em() * 3.0F;
                if (i == 0 && regler != nullptr && a != nullptr && b != nullptr) {
                    melde("A7 Zeile Arm L: " + mbefoto::rechteck("regler", regler->rect) + " " + mbefoto::rechteck("ikfk", a->rect) + " " + mbefoto::rechteck("fkik", b->rect));
                }
            }
            return ok;
        }));
        add(reglerZiehen("##ikfk", "mtrollouts", 0, 0.1F, 0.95F));
        add(pause(0.5));
        add(pruefe(zeile("A7", modus, "den Regler von Arm L nach rechts ziehen stellt FK ein (Anteil unter 0,5; vorher IK - die Animation liegt von FK auf IK auf dem Rig)"), [] {
            melde("A7 Regler: Anteil " + std::to_string(modellIkAnteil(0)));
            return modellIkAnteil(0) < 0.5F;
        }));
        add(tu("AP10 A7: Regler zuruecknehmen", [] { (void)g_app->mtab.dok.rueckgaengig(); }));
        add(pause(0.3));
    }
}

// --- Bedienung, einmal (wie 3ds Max): nur FK, Vorlage, Auf Rig legen, Presets, Bearbeiten, Uebertragen, Wurzelweg, Werkzeuge ---

void bedienSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const int modus = 0;
    // --- A2: Animation-Rig (nur FK) ---
    add(zeige(Str::AzRRig));
    add(pause(0.3));
    add(ap3::insFeld(tr(Str::AzRigFk)));
    add(pause(0.3));
    add(klick(tr(Str::AzRigFk), "mtrollouts"));
    add(pause(0.6));
    add(pruefe(zeile("A2", modus, "Klick auf Animation-Rig (nur FK): das Rig hat keine IK-Griffe mehr, Hand und Fuss greifen ohne IK, der Knopf ist gedrueckt, der Hauptknopf heisst wieder Control Rig erstellen"), [] {
        int hand = -1;
        for (std::size_t i = 0; i < g_app->mtab.rig.griffe.size(); ++i) {
            if (g_app->mtab.rig.griffe[i].art == modell::GriffArt::Hand) { hand = static_cast<int>(i); }
        }
        const Element* fk = imRoll(tr(Str::AzRigFk));
        melde("A2 nur FK: " + std::to_string(g_app->mtab.rig.griffe.size()) + " Griffe, davon " + std::to_string(ikGriffe()) + " IK, Hand-Griff " + std::to_string(hand));
        return modellRigDa() && modellRigNurFk() && ikGriffe() == 0 && hand >= 0 && modellIkArtFuerGriff(hand) == -1 && fk != nullptr && (fk->flags & ImGuiItemStatusFlags_Checked) != 0 &&
               imRoll(tr(Str::AzRigErstellen)) != nullptr;
    }));
    add(zeige(Str::MbIkRollout));
    add(pause(0.3));
    add(pruefe(zeile("A2", modus, "mit dem Rig nur mit FK steht in der Gruppe IK/FK der Hinweis statt der Glieder"), [] {
        return inGruppe(gruppe(Str::MbIkRollout), imRoll(tr(Str::AzIkNurFk))) && imRoll(tr(Str::MbIkArmL)) == nullptr;
    }));
    add(zeige(Str::AzRRig));
    add(pause(0.3));
    add(ap3::insFeld(tr(Str::AzRigErstellen)));
    add(pause(0.3));
    add(klick(tr(Str::AzRigErstellen), "mtrollouts"));
    add(pause(0.6));
    add(pruefe(zeile("A2", modus, "Control Rig erstellen stellt die IK-Griffe wieder her"), [] { return modellRigDa() && !modellRigNurFk() && ikGriffe() > 0; }));
    // --- A2: Vorlage waehlen ---
    add(klick(tr(Str::AzVorEigen), "mtrollouts"));
    add(pause(0.6));
    // TEST1411 N: die Wahl steht im Setup-Modus (Rig erstellen); das Ziel des Uebertragens rechnet die Gruppe Uebertragen des Animationsmodus
    auto eigenAn = std::make_shared<bool>(false);
    add(tu("AP10 A2: Eigenes Skelett gewaehlt?", [eigenAn] {
        const Element* eigen = imRoll(tr(Str::AzVorEigen));
        *eigenAn = eigen != nullptr && (eigen->flags & ImGuiItemStatusFlags_Checked) != 0;
    }));
    add(zeige(Str::AzRUebertragen));
    add(pause(0.4));
    add(pruefe(zeile("A2", modus, "Klick auf Eigenes Skelett: keine Vorlage mehr, das Ziel des Uebertragens nimmt vorgeschlagene Ketten"), [eigenAn] {
        const bool eigenGewaehlt = *eigenAn;
        melde("A2 Eigenes Skelett: Wahl " + std::to_string(modellAnimVorlageWahl()) + ", Ziel-Vorlage \"" + g_app->mtab.rt.cz.vorlage + "\", " + std::to_string(g_app->mtab.rt.cz.ketten.size()) + " Ketten");
        return modellAnimVorlageWahl() == -1 && modellAnimVorlage() == -1 && eigenGewaehlt && g_app->mtab.rt.cz.vorlage.empty() && !g_app->mtab.rt.cz.ketten.empty();
    }));
    add(zeige(Str::AzRRig));
    add(klick(tr(Str::AzVorMensch), "mtrollouts"));
    add(pause(0.6));
    add(zeige(Str::AzRUebertragen));
    add(pause(0.4));
    add(pruefe(zeile("A2", modus, "Klick auf Mensch: wieder die erkannte Vorlage _humanoid (JKA), auch fuer das Ziel des Uebertragens"), [] {
        melde("A2 Mensch: Wahl " + std::to_string(modellAnimVorlageWahl()) + ", Ziel-Vorlage \"" + g_app->mtab.rt.cz.vorlage + "\"");
        return modellAnimVorlageWahl() == -2 && modellAnimVorlage() == modellAnimVorlageErkannt() && g_app->mtab.rt.cz.vorlage == "_humanoid (JKA)";
    }));
    // --- A2: Auf Rig legen ---
    auto vorher = std::make_shared<std::vector<BoneMatrix>>();
    auto schritte = std::make_shared<std::string>();
    auto schluesselVor = std::make_shared<int>(0);
    add(tu("AP10 A2: Bild 12 der Sequenz, die Pose merken", [=] {
        if (const AnimEntry* e = sequenzJetzt()) {
            g_app->animFrame = e->firstFrame + std::min(12, std::max(e->numFrames - 1, 0));
            g_app->animTime = 0.0F;
            g_app->animPlaying = false;
            g_app->modelDirty = true;
        }
    }));
    add(pause(0.5));
    add(tu("AP10 A2: Pose merken", [=] {
        *vorher = g_app->modellWelt;
        *schritte = g_app->mtab.dok.rueckgaengigName();
        *schluesselVor = schluessel();
    }));
    add(zeige(Str::AzRRig));   // TEST1411 N: Auf Rig legen steht im Setup-Modus
    add(klick(tr(Str::AzAufRig), "mtrollouts"));
    add(pause(0.8));
    add(zeige(Str::AzRAnimation));   // TEST1411 N: die Pose der Sequenz zeigt der Animationsmodus (Setup zeigt die Setup-Pose)
    add(pause(0.4));
    add(pruefe(zeile("A2", modus, "Klick auf Auf Rig legen: die Bilder der Sequenz sind Schluessel an den Griffen, die vier Glieder sind an jedem Bild IK (Anteil 1, von FK auf IK wie ENT2) und folgen ihren Zielen, die Pose bleibt"), [=] {
        const modell::RigSchicht* sch = modellRigSchichtJetzt();
        const AnimEntry* e = sequenzJetzt();
        if (sch == nullptr || e == nullptr) { return false; }
        int spuren = 0;
        bool jedesBild = true;
        float anteil = 1.0F;
        for (const modell::IkSpur& sp : sch->ik) {
            ++spuren;
            jedesBild = jedesBild && static_cast<int>(sp.ziel.size()) == e->numFrames;
            for (int b = e->firstFrame; b < e->firstFrame + e->numFrames; ++b) { anteil = std::min(anteil, modell::ikAnteilAm(sp, static_cast<float>(b))); }
        }
        const float abstand = poseAbstand(*vorher, g_app->modellWelt);
        char z[240];
        std::snprintf(z, sizeof z, "A2 Auf Rig legen: %d Schluessel, %d IK-Spuren, je Bild %d, Anteil %.2f, Pose bleibt auf %.4f Einheiten, Schritt \"%s\"", sch->schluesselZahl(), spuren, jedesBild ? 1 : 0,
                      static_cast<double>(anteil), static_cast<double>(abstand), g_app->mtab.dok.rueckgaengigName().c_str());
        melde(z);
        // Grenze der Pose: rigReduzieren haelt jedes Knochen-Bild innerhalb einer Stufe der gepackten Verschiebung (compressBone,
        // 1/64 Einheit je Achse) und der Drehgrenze; entlang der Kette bis zur Hand (8 Glieder) summieren sich die Stufen bis
        // 8/64 = 0,125. Gemessen (kyle, BOTH_WALK1, Bild 12; Laeufe ap10_g1 bis ap10_g3 gleich): 0,0479. Grenze 1/16 Einheit -
        // ueber dem Messwert, halb so weit wie die Summe der Stufen.
        return sch->schluesselZahl() > 0 && spuren == 4 && jedesBild && anteil >= 1.0F && abstand < 0.0625F && g_app->mtab.dok.rueckgaengigName() == tr(Str::AzAufRig);
    }));
    add(tu("AP10 A2: Auf Rig legen zuruecknehmen", [] { (void)g_app->mtab.dok.rueckgaengig(); }));
    add(pause(0.4));
    add(pruefe(zeile("A2", modus, "Rueckgaengig nimmt Auf Rig legen in einem Schritt zurueck (die Schluessel wie vorher)"), [=] {
        return schluessel() == *schluesselVor && g_app->mtab.dok.rueckgaengigName() == *schritte;
    }));
    // --- A2: Tooltip ---
    add(tu("AP10 A2: Maus auf Auf Rig legen", [] {
        if (const Element* e = imRoll(tr(Str::AzAufRig))) { setzeMaus(e->rect.GetCenter()); }
    }));
    add(pause(1.2));
    add(pruefe(zeile("A2", modus, "Auf Rig legen zeigt nach der ueblichen Verzoegerung seinen Tooltip"), [] {
        const Element* e = imRoll(tr(Str::AzAufRig));
        return e != nullptr && e->rect.Contains(ImGui::GetIO().MousePos) && tooltipDa();
    }));
    add(tu("AP10: Maus in die Ansicht", [] { setzeMaus(mbefoto::ansichtEcke()); }));
    add(pause(0.3));
    // --- A2 / A14: Menue, Rechtsklick und Suche kennen die neuen Aktionen ---
    add(pruefe(zeile("A2", modus, "Animation-Rig (nur FK) und Auf Rig legen stehen in den Rig-Aktionen (Menue Animation, Rechtsklick, Alle Aktionen suchen) mit Hinweis"), [] {
        int gefunden = 0;
        for (int i = 0; i < modellRigAktionZahl(); ++i) {
            const std::string t = modellRigAktionText(i);
            if ((t == tr(Str::AzRigFk) || t == tr(Str::AzAufRig)) && modellRigAktionHinweis(i)[0] != '\0' && modellRigAktionGruppe(i) == 0) { ++gefunden; }
        }
        return gefunden == 2;
    }));
    // --- A3: Presets ---
    auto belegtVorher = std::make_shared<int>(0);
    add(zeige(Str::PrRollout));
    add(pause(0.3));
    add(klick(tr(Str::AzKatLaufen), "mtrollouts"));
    add(pause(0.4));
    add(pruefe(zeile("A3", modus, "Klick auf die Kategorie Laufen: die Liste zeigt ihre Presets und Laufen (erzeugt), die Kurzinfo nennt Kategorie und Zahl (mit dem erzeugten Gang, ENT2)"), [] {
        const ui::RolloutProbe* g = gruppe(Str::PrRollout);
        const std::vector<PresetEintrag>& b = modellPresetBibliothek();
        int n = 0;
        std::string erstes;
        for (const PresetEintrag& e : b) {
            if (e.kategorie != "run") { continue; }
            if (erstes.empty()) { erstes = e.name; }
            ++n;
        }
        melde("A3 Laufen: " + std::to_string(n) + " Presets, erstes " + erstes + ", Kurzinfo " + (g != nullptr ? g->kurzinfo : std::string("-")));
        return g != nullptr && n > 0 && g->kurzinfo == std::string(tr(Str::AzKatLaufen)) + " \xc2\xb7 " + std::to_string(n + 1) && inFenster(erstes, "##aibib") != nullptr &&
               inFenster(tr(Str::EnGangLaufen), "##aibib") != nullptr;
    }));
    add(tu("AP10 A3: das erste Preset der Kategorie waehlen", [=] {
        const std::vector<PresetEintrag>& b = modellPresetBibliothek();
        for (std::size_t i = 0; i < b.size(); ++i) {
            if (b[i].kategorie == "run") {
                g_app->mtab.rt.bibliothekWahl = static_cast<int>(i);
                break;
            }
        }
        *belegtVorher = belegt();
    }));
    add(pause(0.3));
    add(klick(tr(Str::AzPresetVorschau), "mtrollouts"));
    add(pause(2.0));
    add(pruefe(zeile("A3", modus, "Klick auf Vorschau auf Figur: das Preset ist geladen und uebertragen, die Figur spielt es, der Ziel-Platz steht im Feld; geschrieben ist nichts"), [=] {
        const ModellRetarget& r = g_app->mtab.rt;
        melde("A3 Vorschau: " + std::to_string(r.welt.size()) + " Bilder, Vorschau " + std::to_string(r.vorschau ? 1 : 0) + ", Platz " + modellRtDaten()->platz + ", belegt " + std::to_string(belegt()));
        return r.vorschau && !r.welt.empty() && modellRtDaten()->platz == "BOTH_RUN1" && belegt() == *belegtVorher;
    }));
    add(zeige(Str::PrRollout));
    add(pause(0.3));
    add(klick(tr(Str::AzPresetAuflegen), "mtrollouts"));
    add(pause(1.0));
    add(pruefe(zeile("A3", modus, "Klick auf Auf Figur legen (Retarget): das Preset liegt auf einem Platz der Ziel-GLA"), [=] {
        melde("A3 Auflegen: belegt " + std::to_string(belegt()) + " (vorher " + std::to_string(*belegtVorher) + ")");
        return belegt() == *belegtVorher + 1;
    }));
    add(klick(tr(Str::AzGenerator), "mtrollouts"));
    add(pause(0.4));
    add(pruefe(zeile("A3", modus, "Klick auf Generator ...: der erzeugte Gang der Kategorie (Laufen (erzeugt)) ist gewaehlt, der Rest des Generators (Schrittlaenge, Messen) ist unter dem Hauptknopf aufgeklappt und ins Bild gerollt (ENT2)"), [] {
        char messen[160];
        std::snprintf(messen, sizeof messen, tr(Str::PrMessen), "BOTH_RUN1");
        return modellAnim().generatorOffen && g_app->mtab.rt.gangWahl == 1 && imRoll(tr(Str::PrSchritt)) != nullptr && imRoll(messen) != nullptr;
    }));
    add(zeige(Str::PrRollout));
    add(pause(0.4));
    add(pruefe(zeile("A3", modus, "dabei stehen Tempo und Armschwung unter der Liste ueber dem Hauptknopf (wie der Entwurf, ENT2)"), [] {
        const Element* tempo = imRoll(tr(Str::PrTempo));
        const Element* arm = imRoll(tr(Str::PrArm));
        const Element* haupt = imRoll(tr(Str::AzPresetAuflegen));
        const ImGuiWindow* liste = kindFenster("##aibib");
        return liste != nullptr && tempo != nullptr && arm != nullptr && haupt != nullptr && tempo->rect.Min.y >= liste->Pos.y + liste->Size.y - 1.0F && arm->rect.Max.y <= haupt->rect.Min.y;
    }));
    add(klick(tr(Str::AzGenerator), "mtrollouts"));
    add(tu("AP10 A3: Vorschau aus", [] { (void)modellRtVorschau(false); }));
    add(pause(0.4));
    // --- A4: Vorhandene Animation bearbeiten ---
    auto quellen = std::make_shared<std::size_t>(0);
    auto belegtVorBacken = std::make_shared<int>(0);
    auto schluesselVorA4 = std::make_shared<int>(0);
    add(tu("AP10 A4: Schluessel vor dem Umwandeln merken", [=] { *schluesselVorA4 = schluessel(); }));
    add(zeige(Str::AzRBearbeiten));
    add(pause(0.3));
    add(klick(tr(Str::MbKvReduzieren), "mtrollouts"));
    add(pause(0.8));
    add(pruefe(zeile("A4", modus, "Klick auf Frames zu Schluesseln reduzieren: die Sequenz hat Schluessel (die Zeile Schluessel der Tabelle zaehlt sie)"), [=] {
        *quellen = g_app->mtab.kgQuellen.size();
        *belegtVorBacken = belegt();
        melde("A4 Umwandeln: " + std::to_string(schluessel()) + " Schluessel");
        return schluessel() > 0 && inGruppe(gruppe(Str::AzRBearbeiten), imRoll(tr(Str::AzSchluessel)));
    }));
    add(klick(tr(Str::MbRgBacken), "mtrollouts"));
    add(pause(1.0));
    add(pruefe(zeile("A4", modus, "Klick auf In die GLA backen: die gebackene Sequenz liegt als Quelle auf einem Platz der Ziel-GLA"), [=] {
        melde("A4 Backen: Quellen " + std::to_string(g_app->mtab.kgQuellen.size()) + " (vorher " + std::to_string(*quellen) + "), belegt " + std::to_string(belegt()) + " (vorher " +
              std::to_string(*belegtVorBacken) + ")");
        return g_app->mtab.kgQuellen.size() == *quellen + 1U && belegt() == *belegtVorBacken + 1;
    }));
    // Backen und Reduzieren sind je ein Rueckgaengig-Schritt: beide zuruecknehmen (der Platz des Backens wird wieder frei)
    add(tu("AP10 A4: Backen und Schluessel zuruecknehmen", [] {
        (void)g_app->mtab.dok.rueckgaengig();
        (void)g_app->mtab.dok.rueckgaengig();
    }));
    add(pause(0.3));
    add(pruefe(zeile("A4", modus, "zwei Schritte Rueckgaengig: der Platz des Backens ist wieder frei, die Sequenz hat wieder die Schluessel von vorher"), [=] {
        return belegt() == *belegtVorBacken && schluessel() == *schluesselVorA4;
    }));
    add(pause(0.3));
    // --- A5: Uebertragen mit Quelle ---
    add(tu("AP10 A5: Quelle laden (ein anderes Skelett des Spiels)", [=] {
        bool ok = false;
        for (const char* q : {"models/players/rockettrooper/rockettrooper.gla", "models/players/hazardtrooper/hazardtrooper.gla", "models/players/_humanoid/_humanoid.gla"}) {
            if (!ok) { ok = modellRtQuelleLaden(q); }
        }
        g_app->mtab.rt.oeffnen = false;
        melde("A5 Quelle: " + g_app->mtab.rt.quelleFuer + ", " + std::to_string(g_app->mtab.rt.quelle.bones.size()) + " Knochen");
        *belegtVorher = belegt();
    }));
    add(pause(1.5));
    add(zeige(Str::AzRUebertragen));
    add(pause(0.4));
    add(pruefe(zeile("A5", modus, "mit Quelle: Tabelle Quelle und Ziel, die Teile Charakterisierungs-Pose und Charakter-Definition als Knoepfe im Raster, die Ketten als Tabelle, die Schritte als Schalter"), [] {
        const ui::RolloutProbe* g = gruppe(Str::AzRUebertragen);
        if (g == nullptr || g_app->mtab.rt.quelle.bones.empty()) { return false; }
        const Element* pose = imRoll(tr(Str::RtPoseMax));
        const Element* ketten = imRoll(tr(Str::RtKettenMax));
        const bool teil[] = {g->kurzinfo == fileName(g_app->mtab.rt.quelleFuer), imRollIn(g, tr(Str::AzQuelle)) != nullptr, imRollIn(g, tr(Str::AzZiel)) != nullptr,
                             nebeneinander(pose, ketten) && inGruppe(g, pose), inGruppe(g, imRoll(tr(Str::AzKetteWirbel))), inGruppe(g, imRoll(tr(Str::AzKetteArme))),
                             inGruppe(g, imRoll(tr(Str::AzKetteBeine))), inGruppe(g, imRoll(tr(Str::RtSchrittBecken))), inGruppe(g, imRoll(tr(Str::RtKontakte)))};
        std::string fehlt;
        for (std::size_t i = 0; i < std::size(teil); ++i) { fehlt += teil[i] ? "" : " " + std::to_string(i); }
        melde("A5 Uebertragen: Kurzinfo " + g->kurzinfo + ", Hauptknoepfe " + std::to_string(g->hauptKnoepfe) + ", Teile ohne Erfolg:" + (fehlt.empty() ? std::string(" keine") : fehlt));
        return fehlt.empty();
    }));
    add(klick(tr(Str::RtKettenMax), "mtrollouts"));
    add(pause(0.4));
    add(pruefe(zeile("A5", modus, "Klick auf Charakter-Definition klappt die Zuordnung der Ketten auf (Automatisch, Tabelle Ziel und Quelle)"), [] {
        return inGruppe(gruppe(Str::AzRUebertragen), imRoll(tr(Str::RtAutoLeer))) && imRoll(tr(Str::RtAutoAlle)) != nullptr;
    }));
    add(klick(tr(Str::RtKettenMax), "mtrollouts"));
    add(pause(0.4));
    add(zeige(Str::AzRUebertragen, 12.0F));
    add(pause(0.4));
    add(pruefe(zeile("A5", modus, "der Platz als Formularzeile, darunter ein blauer Hauptknopf (Auf Platz plotten) ueber die Breite, darunter das Fussrutschen als Wert"), [] {
        const ui::RolloutProbe* g = gruppe(Str::AzRUebertragen);
        ImGuiWindow* w = kindFenster("mtrollouts");
        const Element* platz = w != nullptr ? findeKennung(w->GetID("##rtplatz")) : nullptr;
        const Element* haupt = imRoll(tr(Str::RtBackenMax));
        const Element* rutschen = imRoll(tr(Str::AzFussrutschen));
        if (g == nullptr || platz == nullptr) { return false; }
        melde("A5 Hauptknopf: " + std::to_string(g->hauptKnoepfe) + ", Bilder " + std::to_string(g_app->mtab.rt.welt.size()) + ", Rutschen " + std::to_string(g_app->mtab.rt.bericht.rutschenMax));
        return g->hauptKnoepfe == 1 && volleBreite(g, haupt) && haupt->rect.Min.y >= platz->rect.Max.y && inGruppe(g, rutschen) && rutschen->rect.Min.y >= haupt->rect.Max.y;
    }));
    add(klick(tr(Str::RtBackenMax), "mtrollouts"));
    add(pause(1.0));
    add(pruefe(zeile("A5", modus, "Klick auf den Hauptknopf: die uebertragene Animation liegt auf einem Platz der Ziel-GLA"), [=] {
        melde("A5 Plotten: belegt " + std::to_string(belegt()) + " (vorher " + std::to_string(*belegtVorher) + ")");
        return belegt() == *belegtVorher + 1;
    }));
    add(tu("AP10 A5: Vorschau aus", [] { (void)modellRtVorschau(false); }));
    add(pause(0.3));
    // --- A1: die Plaetze zaehlen mit ---
    add(zeige(Str::AzRAnimation));
    add(pause(0.3));
    add(tu("AP10 A1: eine andere Sequenz waehlen", [] { animWaehlen(mb1::animNach("BOTH_STAND1") >= 0 ? mb1::animNach("BOTH_STAND1") : 0, false); }));
    add(pause(0.4));
    add(pruefe(zeile("A1", modus, "die Kurzinfo folgt der gewaehlten Sequenz; die Ziel-GLA hat jetzt belegte Plaetze (Preset, Uebertragung; das Backen aus A4 ist zurueckgenommen)"), [] {
        const ui::RolloutProbe* g = gruppe(Str::AzRAnimation);
        const AnimEntry* e = sequenzJetzt();
        melde("A1 Plaetze: " + std::to_string(belegt()) + " von " + std::to_string(g_app->mtab.plaetze.size()) + ", Kurzinfo " + (g != nullptr ? g->kurzinfo : std::string("-")));
        return g != nullptr && e != nullptr && g->kurzinfo == e->name && belegt() == 2;
    }));
    // --- A9: der Wurzelweg ---
    auto wegSoll = std::make_shared<float>(-1.0F);
    add(tu("AP10 A9: eine Sequenz mit Wurzelbewegung waehlen, den Weg des Beckens unabhaengig rechnen", [=] {
        int becken = -1;
        const auto& ek = g_app->mtab.rig.erkennung.knochen;
        for (std::size_t b = 0; b < ek.size() && becken < 0; ++b) {
            if (ek[b].rolle == modell::Rolle::Becken) { becken = static_cast<int>(b); }
        }
        const modell::RigSchicht leer;
        std::string gewaehlt;
        for (const char* name : {"BOTH_ROLL_F", "BOTH_FLIP_F", "BOTH_JUMP1", "BOTH_RUN1", "BOTH_WALK1"}) {
            const int a = mb1::animNach(name);
            if (a < 0 || becken < 0) { continue; }
            const AnimEntry& e = g_app->animList[static_cast<std::size_t>(a)];
            // alle Bilder der Sequenz (modell::bewegungsPfad), nur das erste und das letzte zaehlen
            const std::vector<std::array<float, 3>> p = modell::bewegungsPfad(g_app->anim, leer, becken, e.firstFrame, e.firstFrame + std::max(e.numFrames, 1) - 1);
            if (p.empty()) { continue; }
            const float weg = std::hypot(p.back()[0] - p.front()[0], p.back()[1] - p.front()[1]);
            *wegSoll = weg;
            gewaehlt = name;
            animWaehlen(a, false);
            if (weg > 1.0F) { break; }
        }
        melde("A9 Wurzelweg: Sequenz " + gewaehlt + ", Becken " + std::to_string(becken) + ", Weg " + std::to_string(*wegSoll));
    }));
    add(pause(0.5));
    add(pruefe(zeile("A9", modus, "der Wurzelweg der Sequenz ist der waagrechte Weg des Beckens vom ersten zum letzten Bild (unabhaengig ueber modell::bewegungsPfad gerechnet)"), [=] {
        float weg = -1.0F;
        const bool da = modellAnimWurzelweg(weg);
        melde("A9 Wurzelweg: Feld " + std::to_string(weg) + ", gerechnet " + std::to_string(*wegSoll));
        // beide Wege rechnen dieselben zwei Weltmatrizen (rigWelt) in float: gleich bis auf die letzte Stelle
        return da && *wegSoll >= 0.0F && std::fabs(weg - *wegSoll) <= 1e-4F * std::max(1.0F, *wegSoll);
    }));
    // --- A11: Werkzeuge fuer die Werkzeugleiste wie Blender ---
    add(tu("AP10 A11: zurueck zur Sequenz des Tests, ein Griff gewaehlt", [] {
        animWaehlen(mbefoto::sequenz(), false);
        for (std::size_t i = 0; i < g_app->mtab.rig.griffe.size(); ++i) {
            if (g_app->mtab.rig.griffe[i].art == modell::GriffArt::Oberarm && g_app->mtab.rig.griffe[i].seite == modell::Seite::Rechts) { modellRigGriffWaehlen(static_cast<int>(i)); }
        }
        g_app->mtab.kopieDa = false;
    }));
    add(pause(0.4));
    add(pruefe(zeile("A11", modus, "die drei Werkzeuge des Entwurfs (Pose kopieren, Spiegeln, Ausduennen) stehen mit Symbol, Text und Hinweis bereit; ohne kopierte Pose ist Spiegeln gesperrt"), [] {
        if (modellAnimWerkzeugZahl() != 3) { return false; }
        const ModellAnimWerkzeug a = modellAnimWerkzeug(0);
        const ModellAnimWerkzeug b = modellAnimWerkzeug(1);
        const ModellAnimWerkzeug c = modellAnimWerkzeug(2);
        return std::string(a.text) == tr(Str::MbRgKopieren) && std::string(b.text) == tr(Str::MbRgGespiegelt) && std::string(c.text) == tr(Str::MbKvReduzieren) && a.hinweis[0] != '\0' &&
               b.hinweis[0] != '\0' && c.hinweis[0] != '\0' && a.aktiv && !b.aktiv && c.aktiv;
    }));
    add(tu("AP10 A11: Pose kopieren, gespiegelt einfuegen, ausduennen", [] {
        const bool k = modellAnimWerkzeugAusloesen(0);
        const bool e = modellAnimWerkzeug(1).aktiv && modellAnimWerkzeugAusloesen(1);
        const int vor = schluessel();
        const bool r = modellAnimWerkzeugAusloesen(2);
        melde("A11 Werkzeuge: kopiert " + std::to_string(k ? 1 : 0) + ", gespiegelt " + std::to_string(e ? 1 : 0) + ", Schluessel " + std::to_string(vor) + " -> " + std::to_string(schluessel()) +
              ", reduziert " + std::to_string(r ? 1 : 0));
    }));
    add(pause(0.5));
    add(pruefe(zeile("A11", modus, "Pose kopieren merkt die Pose, Spiegeln fuegt sie gespiegelt ein, Ausduennen wandelt die Bilder in Schluessel um"), [] {
        return g_app->mtab.kopieDa && schluessel() > 0 && g_app->mtab.dok.rueckgaengigName() == tr(Str::MbKvReduzieren);
    }));
    add(tu("AP10 A11: zuruecknehmen", [] {
        (void)g_app->mtab.dok.rueckgaengig();
        (void)g_app->mtab.dok.rueckgaengig();
    }));
    add(pause(0.3));
    // --- A14: eine der weiteren Gruppen bedienen ---
    add(zeige(Str::MbRgGruppePose));
    add(pause(0.3));
    add(klick(tr(Str::MbRgGruppePose), "mtrollouts"));
    add(pause(0.4));
    add(pruefe(zeile("A14", modus, "ein Klick klappt die Gruppe Pose auf: Pose kopieren, Einfuegen, Gespiegelt einfuegen im Raster mit Symbol"), [] {
        const ui::RolloutProbe* g = gruppe(Str::MbRgGruppePose);
        const Element* kop = imRoll(tr(Str::MbRgKopieren));
        const Element* ein = imRoll(tr(Str::MbRgEinfuegen));
        return g != nullptr && g->offen && nebeneinander(kop, ein) && inGruppe(g, kop) && g->rasterSpalten == 2 && g->symbole == g->knoepfe;
    }));
    add(tu("AP10 A14: Kopie verwerfen", [] { g_app->mtab.kopieDa = false; }));
    add(klick(tr(Str::MbRgKopieren), "mtrollouts"));
    add(pause(0.4));
    add(pruefe(zeile("A14", modus, "Pose kopieren in der Gruppe Pose merkt die Pose"), [] { return g_app->mtab.kopieDa; }));
    add(klick(tr(Str::MbRgGruppePose), "mtrollouts"));
    add(pause(0.3));
    add(pruefe(zeile("A14", modus, "mit Rig steht die Auto-Physik als erste der weiteren Gruppen hinter Fuer das Spiel bauen"), [] {
        return gruppeNr(Str::PhRollout) == 5 && gruppeNr(Str::MbRgGruppePose) == 6;
    }));
}

// --- Rig-Vorlage eines eigenen Skeletts des Spiels (A2) ----------------------------------------------------------------------

void kreaturSchritte(std::vector<Schritt>& s, const std::string& modell) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    auto mit = std::make_shared<std::array<int, 2>>();
    add(tu("AP10 A2 Kreatur: Bereich Animation, Rig mit der erkannten Vorlage", [=] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Animation;
        (void)modellAnimRigErstellen(false);
        *mit = {g_app->mtab.rig.erkennung.erkannt, g_app->mtab.rig.erkennung.unklar};
    }));
    add(pause(0.8));
    add(zeige(Str::AzRRig));
    add(pause(0.3));
    add(pruefe("AP10 A2 Kreatur: das Skelett wird als seine Vorlage erkannt, in der Auswahl ist seine Gruppe gewaehlt (nicht Mensch)", [] {
        const int erkannt = modellAnimVorlageErkannt();
        if (erkannt < 0) {
            melde("A2 Kreatur: keine Vorlage erkannt fuer " + g_app->anim.skeletonName);
            return false;
        }
        const modell::RtVorlageInfo v = modell::rtVorlagen()[static_cast<std::size_t>(erkannt)];
        const Element* mensch = imRoll(tr(Str::AzVorMensch));
        const Element* eigene = imRoll(v.gruppe);
        melde("A2 Kreatur: " + g_app->anim.skeletonName + " erkannt als " + v.name + ", Gruppe " + v.gruppe);
        return v.gruppe != "human" && mensch != nullptr && (mensch->flags & ImGuiItemStatusFlags_Checked) == 0 && eigene != nullptr && (eigene->flags & ImGuiItemStatusFlags_Checked) != 0;
    }));
    add(klick(tr(Str::AzVorEigen), "mtrollouts"));
    add(pause(0.8));
    add(pruefe("AP10 A2 Kreatur: mit der Vorlage bleibt bei der Erkennung nicht mehr offen oder unklar als ohne (Eigenes Skelett)", [=] {
        const int erkanntOhne = g_app->mtab.rig.erkennung.erkannt;
        const int unklarOhne = g_app->mtab.rig.erkennung.unklar;
        melde("A2 Kreatur: mit Vorlage erkannt " + std::to_string((*mit)[0]) + ", unklar " + std::to_string((*mit)[1]) + "; ohne erkannt " + std::to_string(erkanntOhne) + ", unklar " +
              std::to_string(unklarOhne) + " (von " + std::to_string(g_app->anim.bones.size()) + " Knochen)");
        return modellAnimVorlageWahl() == -1 && modellRigDa() && (*mit)[0] >= erkanntOhne && (*mit)[1] <= unklarOhne;
    }));
}

// --- Fotos ----------------------------------------------------------------------------------------------------------------

void fotoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const std::string p = modus == 0 ? "ap10_max_" : "ap10_blender_";
        add(tu("AP10 Foto: " + p + "anim - Rig, Arm R gewaehlt, Preset gewaehlt", [=] {
            modellBedienungSetzen(modus);
            g_app->mtab.bereich = ModellBereich::Animation;
            g_app->settings.modellFeldBreite = 0.0F;
            if (g_app->mtab.anbauten.empty()) { (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true); }
            animWaehlen(mbefoto::sequenz(), false);
            if (!modellRigDa()) { (void)modellAnimRigErstellen(false); }
            for (std::size_t i = 0; i < g_app->mtab.rig.griffe.size(); ++i) {
                if (g_app->mtab.rig.griffe[i].art == modell::GriffArt::Hand && g_app->mtab.rig.griffe[i].seite == modell::Seite::Rechts) { modellRigGriffWaehlen(static_cast<int>(i)); }
            }
            const std::vector<PresetEintrag>& b = modellPresetBibliothek();
            for (std::size_t i = 0, k = 0; i < b.size(); ++i) {
                if (i > 0 && b[i].kategorie != b[i - 1].kategorie) { ++k; }
                if (b[i].kategorie == "walk" && g_app->mtab.rt.bibliothekWahl < 0) {
                    g_app->mtab.rt.bibliothekWahl = static_cast<int>(i);
                    g_app->mtab.rt.bibliothekKategorie = static_cast<int>(k);
                }
            }
            if (g_app->mtab.rt.quelle.bones.empty()) {
                bool ok = false;
                for (const char* q : {"models/players/rockettrooper/rockettrooper.gla", "models/players/hazardtrooper/hazardtrooper.gla", "models/players/_humanoid/_humanoid.gla"}) {
                    if (!ok) { ok = modellRtQuelleLaden(q); }
                }
                g_app->mtab.rt.oeffnen = false;
            }
            if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
            setzeMaus(mbefoto::ansichtEcke());
        }));
        add(pause(1.5));
        add(foto(p + "anim"));
        for (int k = 0; k < 8; ++k) { add(mbefoto::seitenFoto(p + "anim_feld" + std::to_string(k), "mtrollouts", k)); }
        add(tu("AP10 Foto: die weiteren Gruppen aufgeklappt", [] {
            if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
        }));
        add(mbefoto::gruppenAuf("mtrollouts"));
        add(pause(0.5));
        for (int k = 0; k < 14; ++k) { add(mbefoto::seitenFoto(p + "anim_alle" + std::to_string(k), "mtrollouts", k)); }
        add(tu("AP10 Foto: die weiteren Gruppen wieder zu, Reiter Hierarchie / Constraints", [=] {
            ImGuiWindow* w = kindFenster("mtrollouts");
            for (const Str t : {Str::PhRollout, Str::MbRgGruppePose, Str::MbRgGhosts, Str::MbRgPfade, Str::MbKvRollout, Str::MbKvGruppeBibliothek}) {
                if (w != nullptr) { w->StateStorage.SetInt(w->GetID(tr(t)), 0); }
            }
            if (w != nullptr) { ImGui::SetScrollY(w, 0.0F); }
            modellFeldReiter(MbReiter::Hierarchie, MbReiterBl::Constraints);
        }));
        add(pause(0.5));
        add(foto(p + "anim_reiter_a"));
        add(tu("AP10 Foto: Reiter Dienstprogramme / Daten", [] { modellFeldReiter(MbReiter::Dienste, MbReiterBl::Daten); }));
        add(pause(0.5));
        add(foto(p + "anim_reiter_b"));
        add(tu("AP10 Foto: zurueck in den Reiter des Bereichs", [] { modellFeldReiter(MbReiter::Bewegung, MbReiterBl::Knochen); }));
        add(pause(0.4));
    }
}

// Bilder fuer das Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap10): h_ap10_max, h_ap10_blender - Sturmtruppler mit Rig.
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP10 Handbuch: Bereich Animation, BOTH_WALK1, Control Rig, wie 3ds Max", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = 0.85F;
        g_app->settings.modellFeldBreite = 0.0F;
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Animation;
        animWaehlen(mb1::animNach("BOTH_WALK1") >= 0 ? mb1::animNach("BOTH_WALK1") : 0, false);
        (void)modellAnimRigErstellen(false);
        modellExplorerVorgabe();
        modellAnsichtEinpassenAktiv();
    }));
    add(pause(1.2));
    add(hilfe2::schrittFoto("h_ap10_max"));
    add(tu("AP10 Handbuch: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(0.9));
    add(hilfe2::schrittFoto("h_ap10_blender"));
    add(tu("AP10 Handbuch: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "ap10" && n != "ap10foto") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_AP10_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    const char* m = std::getenv("BHED_AP10_MODELL");
    const std::string modell = m != nullptr ? m : "models/players/kyle/model.glm";
    const char* kr = std::getenv("BHED_AP10_KREATUR");
    const std::string kreatur = kr != nullptr ? kr : "models/players/rancor/model.glm";
    auto skala = std::make_shared<float>(1.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP10: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        if (const char* t = std::getenv("BHED_AP10_THEMA")) {
            const bool hell = std::string(t) == "hell";
            for (const theme::Theme& th : theme::themes()) {
                if ((theme::relativeLuminance(th.window) > 0.5F) == hell) {
                    g_app->activeTheme = &th;
                    g_app->themePending = true;
                    melde("Thema " + th.id);
                    break;
                }
            }
        }
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap10.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    add(tu("AP10: Aufteilung wie neue Einstellungen, wie 3ds Max, Bereich Animation", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Animation;
        g_app->mtab.vierAnsichten = false;
        char zeile[300];
        std::snprintf(zeile, sizeof zeile, "%s, %zu Knochen, Anzeige %.0f x %.0f, Oberflaeche %.2f, Schrift %.0f Punkte hoch", g_app->modelPath.c_str(), g_app->anim.bones.size(),
                      static_cast<double>(ImGui::GetIO().DisplaySize.x), static_cast<double>(ImGui::GetIO().DisplaySize.y), static_cast<double>(g_app->uiScale),
                      static_cast<double>(ImGui::GetFontSize()));
        melde(zeile);
    }));
    add(pause(1.0));
    add(klickAn("AP10: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (n == "ap10") {
        formSchritte(s, 0);
        rigSchritte(s, 0);
        bedienSchritte(s);
        formSchritte(s, 1);
        rigSchritte(s, 1);
        add(tu("AP10: Saebel loesen, wie 3ds Max", [] {
            modellAnbautenLoesen();
            modellBedienungSetzen(0);
        }));
        add(pause(0.3));
        kreaturSchritte(s, kreatur);
    } else {
        fotoSchritte(s);
    }
    add(tu("AP10: Modell-Tab zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        modellRigNurFkSetzen(false);
        modellAnbautenLoesen();
    }));
    add(tu("AP10: zurueck", [=] {
        g_app->uiScale = *skala;
        if (*thema != nullptr && g_app->activeTheme != *thema) {
            g_app->activeTheme = *thema;
            g_app->themePending = true;
        }
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.3));
    return s;
}

}  // namespace ap10
