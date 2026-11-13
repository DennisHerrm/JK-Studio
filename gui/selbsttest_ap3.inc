// selbsttest_ap3.inc - AP3 (MBENTWURF Teil B, 11.11.): der Rollout-Baustein (gui/ui_rollout.h) und der Rahmen des rechten
// Felds des Model-Tabs (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap3 | ap3foto)
//
//   ap3       erst der Baustein fuer sich in einem Probefenster (Gruppe, Raster, Hauptknopf, Regler, Formularzeilen,
//             Schalter, Segment, Werte, Balken, Tipp, Reiter; beide Stile) - Pruefzeilen "AP3 Baustein ...",
//             dann je Zeile der Abgleichtabelle (plaene/modellblock_stufe0/abgleich_programm_entwurf.md, Abschnitt 3 und 6)
//             eine Pruefung "AP3 F<nr>" bzw. "AP3 R<nr>": Ort, Element, Bedienung - wie 3ds Max und wie Blender.
//   ap3foto   nur Fotos (BHED_FOTOS=<Ordner>): das Probefenster (ap3_baustein_*) und das rechte Feld in beiden Bedienmodi, in
//             jedem Bereich und mit den Reitern von Ansehen/Pruefen (ap3_max_*, ap3_blender_*).
//   BHED_AP3_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe wie der Foto-Selbsttest mbefoto)
//   BHED_AP3_MODELL=<.glm>           Vorgabe models/players/kyle/model.glm (das Modell des Entwurfs)
//   BHED_AP3_TEIL=baustein|rahmen|muster   nur dieser Teil (Vorgabe: alle)
//   BHED_AP3_THEMA=hell|dunkel       das erste helle bzw. dunkle Farbschema (Vorgabe: das eingestellte)
// Eingebunden in selbsttest.cpp (nach selbsttest_ap1.inc).
namespace ap3 {

using mbefoto::inFenster;
using mbefoto::kindFenster;

float em() { return ImGui::GetFontSize(); }

void melde(const std::string& z) { diag::detail("Knopftest: AP3 " + z); }

// --- Der Baustein im Probefenster -------------------------------------------------------------------------------

// je Stil ein eigenes Fenster (der Zustand der Gruppen haengt am Fenster)
const char* probeName(ui::RolloutStil stil) { return stil == ui::RolloutStil::Blender ? "AP3 Baustein (Blender)###ap3probe1" : "AP3 Baustein (3ds Max)###ap3probe0"; }

// Was das Probefenster haelt und was in ihm geschah.
struct Probe {
    float rand = 4.0F;
    float gewicht = 0.65F;
    int glaetten = 2;
    int methode = 1;
    char ziel[32] = "kyle";
    bool schalter[4] = {true, false, true, false};
    int lod = 0;
    int verfahren = 1;
    int reiter = 0;
    int klicks = 0;        // Hauptknopf
    int rasterKlicks = 0;
    int geloest = 0;       // "x" der Listenzeile
    bool regSegment = false;
    bool regSchalter = false;
    // gemessen
    std::vector<ui::RolloutProbe> gruppen;
    ui::FeldProbe feld;
};

const ui::FeldReiter kProbeReiter[4] = {
    {"Create", UiIcon::Plus, "New objects"},
    {"Modify", UiIcon::Einstellungen, "Tools of the area"},
    {"Motion", UiIcon::Abspielen, nullptr},
    {"Display", UiIcon::Sichtbar, nullptr},
};

// Der Inhalt: ein Feld mit vier Reitern, Gruppen in Reiter 0 (Standard), 1 und in beiden; Reiter 2 bleibt leer.
void probeInhalt(Probe& p, ui::RolloutStil stil, bool mitFeld) {
    if (mitFeld) {
        ui::FeldOpt o;
        o.titel = stil == ui::RolloutStil::Blender ? "Properties" : "Command panel";
        o.rechts = "UV";
        o.reiter = kProbeReiter;
        o.reiterZahl = 4;
        o.aktiv = &p.reiter;
        o.standard = 0;
        o.stil = stil;
        o.kontext = stil == ui::RolloutStil::Blender ? 1 : 0;
        o.leerHinweis = "No groups here in this area.";
        ui::feldBeginn("ap3feld", o);
    } else {
        ui::rolloutStilSetzen(stil);
    }
    if (ui::rolloutBeginn("Unwrap###ap3unwrap", {.kurzinfo = "torso - 7 islands", .schritt = 2, .hinweis = "Second step: unwrap the selection."})) {
        if (ui::werkzeugKnopf("Unwrap now", {.icon = UiIcon::Erzeugen, .taste = "U", .hinweis = "Unwraps the selected faces.", .haupt = true})) { ++p.klicks; }
        ui::rasterBeginn(2);
        if (ui::werkzeugKnopf("Project", {.icon = UiIcon::Oben})) { ++p.rasterKlicks; }
        if (ui::werkzeugKnopf("Relax", {.icon = UiIcon::Ruhig, .taste = "Ctrl+R"})) { ++p.rasterKlicks; }
        if (ui::werkzeugKnopf("Reload all the textures of the model", {.icon = UiIcon::Wiederholen, .taste = "Alt+R"})) { ++p.rasterKlicks; }
        if (ui::werkzeugKnopf("Pinned", {.icon = UiIcon::Schloss, .an = true})) { ++p.rasterKlicks; }
        if (ui::werkzeugKnopf("Locked", {.icon = UiIcon::Schloss, .hinweis = "Needs a selection.", .aktiv = false})) { ++p.rasterKlicks; }
        ui::rasterEnde();
        (void)ui::regler("Margin", &p.rand, 0.0F, 16.0F, "%.0f px", "Distance between the islands.");
        (void)ui::regler("Weight", &p.gewicht, 0.0F, 1.0F);
        (void)ui::reglerInt("Smooth", &p.glaetten, 0, 8);
        ui::zeile("Transfer target");
        (void)ImGui::InputText("##ap3ziel", p.ziel, sizeof p.ziel);
        ui::zeile("Method", "How the faces are flattened.");
        static const char* const kMethoden[3] = {"Angle based", "Conformal", "Planar"};
        (void)ImGui::Combo("##ap3methode", &p.methode, kMethoden, 3);
        // eine Auswahlliste meldet ihre Beschriftung sonst nur aufgeklappt
        if (GImGui->TestEngineHookItems) { selbsttestMerkeText(GImGui, ImGui::GetItemID(), "##ap3methode"); }
        ui::rasterBeginn(2);
        p.regSchalter = ui::schalter("Seams", &p.schalter[0], "Shows the seams.") || p.regSchalter;
        (void)ui::schalter("Highlight surface (+ bone references)", &p.schalter[1]);
        (void)ui::schalter("Stretch", &p.schalter[2]);
        (void)ui::schalter("Overlaps", &p.schalter[3]);
        ui::rasterEnde();
        static const char* const kLod[10] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
        p.regSegment = ui::segment("ap3lod", kLod, 10, &p.lod, 0, 3) || p.regSegment;
        static const char* const kVerfahren[3] = {"Transfer from original", "Geodesic (voxel)", "Envelope (fast)"};
        (void)ui::segment("ap3verfahren", kVerfahren, 3, &p.verfahren);
        ui::rolloutEnde();
    }
    if (ui::rolloutBeginn("Check###ap3check", {.kurzinfo = "1 error", .ton = ui::Ton::Fehler, .reiter = ui::kReiterStandard | ui::reiterBit(3)})) {
        ui::wertTabelle({{"Stretch", "low (0.06)", ui::Ton::Gut, "Average stretch of the texture."},
                         {"Overlapping", "3", ui::Ton::Warnung, ""},
                         {"Sequences inside the GLA", "1 BOTH_CIN_16 behind the end of the file", ui::Ton::Fehler, ""}});
        ui::grenzBalken("Vertices per surface (torso)", 812, 1000);
        ui::grenzBalken("Bones per surface", 29, 32, "The game allows 32 bones per surface.");
        ui::grenzBalken("Weights per vertex", 4, 4);
        ui::grenzBalken("Triangles", 5200, 4000);
        ImGui::PushID(0);
        if (ui::eintrag("saber_1 -> *r_hand", "Detach")) { ++p.geloest; }
        ImGui::PopID();
        if (ui::mehrBeginn("More###ap3mehr")) {
            ui::absatz("JKA exports one UV set per vertex. Further sets stay in the project only.");
            ui::mehrEnde();
        }
        ui::tipp("Attach: right-click a tag or a bone in the tree.");
        ui::rolloutEnde();
    }
    if (ui::Rollout r("Create###ap3create", {.zu = true, .reiter = ui::reiterBit(1)}); r) {
        ui::absatz("Only on the second tab.", ui::Ton::Normal);
    }
    if (mitFeld) { ui::feldEnde(); }
}

const ui::RolloutProbe* gruppe(const Probe& p, const char* titel) {
    for (const ui::RolloutProbe& g : p.gruppen) {
        if (g.titel == titel) { return &g; }
    }
    return nullptr;
}

const Element* inProbe(const std::string& label) { return finde(label, "ap3probe", 0, false); }

// Ein Probefenster ueber mehrere Bilder: zeichnet den Inhalt, fuehrt die Klicks des Ablaufs aus (je Klick fuenf Bilder) und
// haelt danach fest, was der Baustein gemeldet hat. klicks: Beschriftungen der Reihe nach; nach dem letzten ruft `danach`.
Schritt probe(const std::string& text, std::shared_ptr<Probe> p, ui::RolloutStil stil, bool mitFeld, float breiteEm, std::vector<std::string> klicks,
              std::function<void()> danach, const std::string& fotoName = "") {
    return {text, [=](int b) {
                const float fs = ImGui::GetFontSize();
                const ImGuiViewport* vp = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + fs * 6.0F, vp->WorkPos.y + fs * 0.5F}, ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2{fs * breiteEm, std::min(fs * 58.0F, vp->WorkSize.y - fs * 1.0F)}, ImGuiCond_Always);
                if (b == 0) { ImGui::SetNextWindowFocus(); }
                if (ui::fensterBegin(probeName(stil), nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse)) { probeInhalt(*p, stil, mitFeld); }
                ImGui::End();
                const int warm = 6;   // Bilder, bis Spalten und Reihenhoehen des letzten Bildes stehen
                const int je = 6;
                ImGuiIO& io = ImGui::GetIO();
                const int n = static_cast<int>(klicks.size());
                if (b >= warm && b < warm + n * je) {
                    const int k = (b - warm) / je;
                    const int phase = (b - warm) % je;
                    if (phase == 0) {
                        const Element* e = inProbe(klicks[static_cast<std::size_t>(k)]);
                        if (e == nullptr) {
                            meldeFehler("AP3 Baustein: \"" + klicks[static_cast<std::size_t>(k)] + "\" nicht im Probefenster gefunden");
                        } else {
                            setzeMaus(e->rect.GetCenter());
                        }
                    }
                    if (phase == 1) { io.AddMouseButtonEvent(0, true); }
                    if (phase == 2) { io.AddMouseButtonEvent(0, false); }
                    return false;
                }
                if (b < warm + n * je + 3) { return false; }
                const int ende = warm + n * je + 3;
                if (b == ende) {
                    p->gruppen.clear();
                    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) { p->gruppen.push_back(ui::rolloutProbe(i)); }
                    p->feld = ui::feldProbe();
                    if (danach) { danach(); }
                    if (!fotoName.empty()) {
                        if (const char* ordner = std::getenv("BHED_FOTOS")) { render::fotoAnfordern(std::string(ordner) + "\\" + fotoName + ".bmp"); }
                    }
                }
                return b >= ende + (fotoName.empty() ? 0 : 6);
            }};
}

// Die Pruefungen des Bausteins: dasselbe Probefenster wie 3ds Max und wie Blender.
void bausteinSchritte(std::vector<Schritt>& s, bool nurFotos) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct Mess {
        ImRect haupt, project, relax, reload, pinned, methode, ziel, margin, weight, seams, stretch, lod1, lod10, kopfElement;
        ImGuiItemStatusFlags kopfFlags = 0;
        bool da = false;
        bool werteDa = false;   // die Zeilen der Wertetabelle und der Balken im Element-Register
    };
    for (int modus = 0; modus < 2; ++modus) {
        const ui::RolloutStil stil = modus == 0 ? ui::RolloutStil::Max : ui::RolloutStil::Blender;
        const std::string wie = modus == 0 ? "wie 3ds Max" : "wie Blender";
        const std::string pf = modus == 0 ? "ap3_baustein_max" : "ap3_baustein_blender";
        auto p = std::make_shared<Probe>();
        auto m = std::make_shared<Mess>();
        const auto messen = [m] {
            const auto r = [](const char* label) {
                const Element* e = inProbe(label);
                return e != nullptr ? e->rect : ImRect{};
            };
            m->haupt = r("Unwrap now");
            m->project = r("Project");
            m->relax = r("Relax");
            m->reload = r("Reload all the textures of the model");
            m->pinned = r("Pinned");
            m->methode = r("##ap3methode");
            m->ziel = r("##ap3ziel");
            m->margin = r("Margin");
            m->weight = r("Weight");
            m->seams = r("Seams");
            m->stretch = r("Stretch");
            m->lod1 = r("1");
            m->lod10 = r("10");
            if (const Element* e = inProbe("Unwrap###ap3unwrap")) {
                m->kopfElement = e->rect;
                m->kopfFlags = e->flags;
            }
            m->werteDa = inProbe("Bones per surface") != nullptr && inProbe("Overlapping") != nullptr && inProbe("Sequences inside the GLA") != nullptr &&
                         inProbe("Attach: right-click a tag or a bone in the tree.") != nullptr;
            m->da = true;
        };
        // 1. ohne Feld: Form der Gruppe und ihrer Bausteine
        add(probe("AP3 Baustein " + wie + ": Probefenster", p, stil, false, 24.0F, {}, messen, nurFotos ? pf : std::string()));
        if (nurFotos) {
            add(probe("AP3 Baustein " + wie + ": Probefenster mit Reitern", p, stil, true, 24.0F, {}, nullptr, pf + "_feld"));
            add(probe("AP3 Baustein " + wie + ": Probefenster schmal", p, stil, true, 12.0F, {}, nullptr, pf + "_schmal"));
            continue;
        }
        const auto name = [wie](const std::string& was) { return "AP3 Baustein " + wie + ": " + was; };
        add(pruefe(name("die Gruppe steht mit ihrem Titel im Element-Register, aufklappbar und offen (wie CollapsingHeader)"), [m] {
            return m->da && m->kopfElement.GetWidth() > 0.0F && (m->kopfFlags & ImGuiItemStatusFlags_Openable) != 0 && (m->kopfFlags & ImGuiItemStatusFlags_Opened) != 0;
        }));
        add(pruefe(name("Ueberschrift mit Schrittnummer 2 und Kurzinfo rechts im Balken (rechtsbuendig, hinter dem Titel)"), [p] {
            const ui::RolloutProbe* g = gruppe(*p, "Unwrap");
            if (g == nullptr || g->schritt != 2 || g->kurzinfo != "torso - 7 islands" || g->info.GetWidth() <= 0.0F) { return false; }
            const bool ok = g->info.Max.x <= g->kopf.Max.x && g->info.Max.x >= g->kopf.Max.x - em() && g->info.Min.x > g->kopf.Min.x + em() * 4.0F && g->kopf.Contains(g->info);
            if (!ok) { melde("Kurzinfo " + mbefoto::rechteck("info", g->info) + " " + mbefoto::rechteck("kopf", g->kopf)); }
            return ok;
        }));
        add(pruefe(name("der Kasten umschliesst Ueberschrift und Inhalt (alle Bausteine der Gruppe liegen darin)"), [p, m] {
            const ui::RolloutProbe* g = gruppe(*p, "Unwrap");
            if (g == nullptr || !g->offen) { return false; }
            return g->kasten.Min.y == g->kopf.Min.y && g->kasten.Max.y > g->kopf.Max.y + em() * 10.0F && g->kasten.Contains(m->haupt) && g->kasten.Contains(m->ziel) && g->kasten.Contains(m->lod10);
        }));
        add(pruefe(name(modus == 0 ? "Stil: flacher Balken, Ecken hoechstens 2 Punkte rund" : "Stil: abgerundeter Kasten (mindestens 0,3 Zeichenhoehen)"), [p, modus] {
            const ui::RolloutProbe* g = gruppe(*p, "Unwrap");
            if (g == nullptr) { return false; }
            melde("Stil " + std::to_string(static_cast<int>(g->stil)) + ", Rundung " + std::to_string(g->rundung) + " bei Schrift " + std::to_string(em()));
            return modus == 0 ? (g->stil == ui::RolloutStil::Max && g->rundung <= 2.0F) : (g->stil == ui::RolloutStil::Blender && g->rundung >= std::floor(em() * 0.3F));
        }));
        add(pruefe(name("der Hauptknopf geht ueber die ganze Breite des Inhalts und ist hoeher als ein Knopf des Rasters; genau einer"), [p, m] {
            const ui::RolloutProbe* g = gruppe(*p, "Unwrap");
            if (g == nullptr || g->hauptKnoepfe != 1 || g->knoepfe != 6) { return false; }
            const float innen = g->kasten.GetWidth() - m->haupt.GetWidth();
            return innen >= 2.0F && innen <= em() * 1.2F && m->haupt.GetHeight() > m->project.GetHeight();
        }));
        add(pruefe(name("Raster: zwei gleich breite Spalten, die Knoepfe einer Reihe gleich hoch (auch wenn einer umbricht)"), [p, m] {
            const ui::RolloutProbe* g = gruppe(*p, "Unwrap");
            if (g == nullptr) { return false; }
            const bool breit = std::fabs(m->project.GetWidth() - m->relax.GetWidth()) < 0.5F && std::fabs(m->reload.GetWidth() - m->pinned.GetWidth()) < 0.5F;
            const bool reihe1 = m->project.Min.y == m->relax.Min.y && m->project.Max.y == m->relax.Max.y && m->relax.Min.x > m->project.Max.x;
            const bool reihe2 = m->reload.Min.y == m->pinned.Min.y && m->reload.Max.y == m->pinned.Max.y && m->reload.Min.y > m->project.Max.y;
            const bool umbruch = m->reload.GetHeight() > m->project.GetHeight() + em() * 0.5F;
            if (!(breit && reihe1 && reihe2 && umbruch)) {
                melde("Raster " + mbefoto::rechteck("project", m->project) + " " + mbefoto::rechteck("relax", m->relax) + " " + mbefoto::rechteck("reload", m->reload) + " " +
                      mbefoto::rechteck("pinned", m->pinned));
            }
            return breit && reihe1 && reihe2 && umbruch;
        }));
        add(pruefe(name("Formularzeilen und Regler: alle Felder und Schieber beginnen an derselben Kante"), [m] {
            const bool ok = m->methode.GetWidth() > 0.0F && std::fabs(m->methode.Min.x - m->ziel.Min.x) < 0.5F && std::fabs(m->margin.Min.x - m->weight.Min.x) < 0.5F &&
                            std::fabs(m->methode.Min.x - m->margin.Min.x) < 0.5F && std::fabs(m->methode.Max.x - m->ziel.Max.x) < 0.5F;
            if (!ok) { melde("Felder " + mbefoto::rechteck("methode", m->methode) + " " + mbefoto::rechteck("ziel", m->ziel) + " " + mbefoto::rechteck("margin", m->margin)); }
            return ok;
        }));
        add(pruefe(name(modus == 0 ? "Beschriftung der Formularzeile linksbuendig am Rand des Inhalts" : "Beschriftung der Formularzeile rechtsbuendig vor dem Feld"), [p, m, modus] {
            const ui::RolloutProbe* g = gruppe(*p, "Unwrap");
            if (g == nullptr) { return false; }
            // die letzte Formularzeile der Gruppe ist "Method" - kuerzer als die Spalte ("Transfer target" bestimmt sie)
            const float abstand = g->feldLinks - g->beschriftungRechts;
            melde("Formularzeile: Beschriftung endet " + std::to_string(abstand) + " Punkte vor dem Feld, Feld ab " + std::to_string(g->feldLinks - g->kasten.Min.x));
            return modus == 0 ? (abstand >= em() * 2.0F) : (abstand >= em() * 0.5F && abstand <= em() * 0.7F);
        }));
        add(pruefe(name("Schalterliste in zwei Spalten, Segment 1-10 in einer Reihe ueber die Breite"), [m] {
            return m->seams.GetWidth() > 0.0F && m->stretch.Min.x == m->seams.Min.x && m->stretch.Min.y > m->seams.Max.y - 1.0F && m->lod1.Min.y == m->lod10.Min.y && m->lod10.Max.x > m->lod1.Max.x + em() * 8.0F;
        }));
        add(pruefe(name("Grenz-Balken, Werte und Tipp: die Zeilen stehen mit ihrem Text im Element-Register"), [m] { return m->werteDa; }));
        // 2. Bedienung: Klicks
        add(probe("AP3 Baustein " + wie + ": Klicks", p, stil, false, 24.0F, {"Unwrap now", "Relax", "Locked", "Seams", "2", "5", "Detach", "Check###ap3check"}, nullptr));
        add(pruefe(name("Klicks kommen an: Hauptknopf 1, Raster 1 (der gesperrte Knopf nicht), Schalter aus, Segment auf 2 (5 ist gesperrt), x der Listenzeile"), [p] {
            const bool ok = p->klicks == 1 && p->rasterKlicks == 1 && !p->schalter[0] && p->regSchalter && p->lod == 1 && p->regSegment && p->geloest == 1;
            if (!ok) {
                melde("Klicks: haupt " + std::to_string(p->klicks) + ", raster " + std::to_string(p->rasterKlicks) + ", schalter " + std::to_string(p->schalter[0] ? 1 : 0) + ", lod " +
                      std::to_string(p->lod) + ", geloest " + std::to_string(p->geloest));
            }
            return ok;
        }));
        add(pruefe(name("ein Klick auf die Ueberschrift klappt die Gruppe zu: der Kasten ist nur noch der Balken"), [p] {
            const ui::RolloutProbe* g = gruppe(*p, "Check");
            return g != nullptr && !g->offen && g->kasten.GetHeight() == g->kopf.GetHeight();
        }));
        // 3. das Feld mit Reitern
        add(probe("AP3 Baustein " + wie + ": Feld mit Reitern", p, stil, true, 24.0F, {}, nullptr));
        add(pruefe(name(modus == 0 ? "Reiter als Reihe mit Text ueber dem Inhalt" : "Reiter als senkrechte Symbolspalte links neben dem Inhalt"), [p, modus] {
            const ui::FeldProbe& f = p->feld;
            melde("Feld: " + mbefoto::rechteck("titel", f.titel) + " " + mbefoto::rechteck("reiter", f.reiter) + " " + mbefoto::rechteck("inhalt", f.inhalt));
            if (f.reiterZahl != 4 || f.reiterAktiv != 0) { return false; }
            if (modus == 0) { return !f.reiterSenkrecht && f.reiter.Min.y >= f.titel.Max.y && f.inhalt.Min.y >= f.reiter.Max.y && f.reiter.GetWidth() > em() * 10.0F; }
            return f.reiterSenkrecht && f.reiter.Max.x <= f.inhalt.Min.x && f.reiter.GetWidth() < em() * 2.6F && f.reiter.GetHeight() > em() * 8.0F;
        }));
        add(pruefe(name("Reiter mit Gruppen: der vorgegebene, \"Modify\" und \"Display\"; \"Motion\" hat keine"), [p] {
            const unsigned b = p->feld.reiterBelegt;
            return (b & 1U) != 0U && (b & 2U) != 0U && (b & 4U) == 0U && (b & 8U) != 0U;
        }));
        add(pruefe(name("im vorgegebenen Reiter stehen die Gruppen ohne eigene Angabe, nicht die des zweiten Reiters"), [p] {
            return gruppe(*p, "Unwrap") != nullptr && gruppe(*p, "Check") != nullptr && gruppe(*p, "Create") == nullptr;
        }));
        add(probe("AP3 Baustein " + wie + ": Klick auf einen leeren und auf den zweiten Reiter", p, stil, true, 24.0F, {"Motion", "Modify"}, nullptr));
        add(pruefe(name("der Reiter ohne Gruppen laesst sich nicht waehlen; der zweite zeigt nur seine Gruppe"), [p] {
            const bool ok = p->reiter == 1 && gruppe(*p, "Create") != nullptr && gruppe(*p, "Unwrap") == nullptr && gruppe(*p, "Check") == nullptr;
            if (!ok) { melde("Reiter " + std::to_string(p->reiter) + ", Gruppen " + std::to_string(p->gruppen.size())); }
            return ok;
        }));
        // 4. schmal: nichts abgeschnitten
        add(probe("AP3 Baustein " + wie + ": schmales Fenster", p, stil, false, 11.0F, {}, messen));
        add(pruefe(name("schmal (11 Zeichenhoehen): das Raster nimmt eine Spalte, kein Baustein ragt ueber den Kasten"), [p, m] {
            const ui::RolloutProbe* g = gruppe(*p, "Unwrap");
            if (g == nullptr) { return false; }
            const bool eine = g->rasterSpalten == 1 && m->relax.Min.y > m->project.Max.y && std::fabs(m->relax.Min.x - m->project.Min.x) < 0.5F;
            bool drin = true;
            for (const ImRect* r : {&m->haupt, &m->project, &m->relax, &m->reload, &m->methode, &m->ziel, &m->margin, &m->lod10}) { drin = drin && r->Max.x <= g->kasten.Max.x + 0.5F && r->Min.x >= g->kasten.Min.x - 0.5F; }
            if (!(eine && drin)) { melde("schmal: Spalten " + std::to_string(g->rasterSpalten) + " " + mbefoto::rechteck("kasten", g->kasten) + " " + mbefoto::rechteck("relax", m->relax) + " " + mbefoto::rechteck("ziel", m->ziel)); }
            return eine && drin;
        }));
    }
}

// --- Die Zeilen der Abgleichtabelle: der Rahmen des rechten Felds und die Kopfzeile des Tabs -------------------------------
// F1, F2 (Reiter), F9, F10 (Stil des Rahmens), F13 (Breite), R2 (Bereichsreihe), R3 (Hinweis), R4 (Schalter Bedienmodus)

bool feldFrisch() { return ui::feldProbe().bild >= ImGui::GetFrameCount() - 2; }

const Element* imFeld(const std::string& label) { return inFenster(label, "mtbefehlsfeld"); }

std::string zeile(const char* nr, int modus, const std::string& was) { return std::string("AP3 ") + nr + (modus == 0 ? " wie 3ds Max: " : " wie Blender: ") + was; }

// Das Feld eines Bereichs in der Kopfzeile des Tabs (auf der Hoehe der Bereichsreihe - das Menue "Animation" heisst gleich).
const Element* kopfFeld(int bereich) {
    static const Str kNamen[kModellBereiche] = {Str::MbBereichAnsehen, Str::MbBereichModellieren, Str::MbBereichUv, Str::MbBereichWeights, Str::MbBereichPainting, Str::MbBereichAnimation};
    const ModellFeld& f = modellFeld();
    if (f.kopfBild < ImGui::GetFrameCount() - 2) { return nullptr; }
    const std::string name = tr(kNamen[bereich]);
    for (const Element& e : g_elemente) {
        if (e.label == name && std::fabs(e.rect.Min.y - f.kopfY) < 2.0F && e.rect.Min.x >= f.kopfVon - 1.0F && e.rect.Max.x <= f.kopfBis + 1.0F) { return &e; }
    }
    return nullptr;
}

ImVec2 trennerOrt() {
    const ModellFeld& f = modellFeld();
    const ImGuiWindow* w = kindFenster("mtbefehlsfeld");
    if (w == nullptr || f.trennerBis <= f.trennerVon) { return ImVec2{-50.0F, -50.0F}; }
    return ImVec2{std::floor((f.trennerVon + f.trennerBis) * 0.5F), w->Pos.y + w->Size.y * 0.5F};
}

float feldBreiteEm() {
    const ImGuiWindow* w = kindFenster("mtbefehlsfeld");
    return w != nullptr ? w->Size.x / em() : 0.0F;
}

// AP4: wie Blender steht das Feld unter dem Outliner und ist niedriger (Entwurf: Outliner 44 % der Spalte) - eine Beschriftung des
// Felds vor dem Klick ganz ins Bild rollen, wie ein Benutzer es taete; steht sie schon im Bild, bleibt die Rolle.
Schritt insFeld(const std::string& label) {
    return tu("AP4: ins Bild des Felds rollen - " + label, [label] {
        const Element* e = finde(label, "mtrollouts", 0, false);
        ImGuiWindow* w = kindFenster("mtrollouts");
        if (e == nullptr || w == nullptr) { return; }
        float d = 0.0F;
        if (e->rect.Max.y > w->InnerRect.Max.y) {
            d = e->rect.Max.y - w->InnerRect.Max.y + ImGui::GetFontSize() * 0.5F;
        } else if (e->rect.Min.y < w->InnerRect.Min.y) {
            d = e->rect.Min.y - w->InnerRect.Min.y - ImGui::GetFontSize() * 0.5F;
        }
        if (d != 0.0F) { ImGui::SetScrollY(w, std::clamp(w->Scroll.y + d, 0.0F, w->ScrollMax.y)); }
    });
}

bool tooltipDa() {
    for (const ImGuiWindow* w : GImGui->Windows) {
        if ((w->Flags & ImGuiWindowFlags_Tooltip) != 0 && w->Active && !w->Hidden) { return true; }
    }
    return false;
}

void rahmenSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    static const Str kMaxReiter[6] = {Str::MfErstellen, Str::MfAendern, Str::MfHierarchie, Str::MfBewegung, Str::MfAnzeige, Str::MfDienste};
    static const Str kBlReiter[7] = {Str::MfWerkzeug, Str::MfObjekt, Str::MfModifier, Str::MfDaten, Str::MfKnochen, Str::MfConstraints, Str::MfMaterial};
    // der Reiter des Entwurfs je Bereich (Ansehen, Modellieren, UV, Weights, Painting, Animation)
    static const int kMaxAktiv[kModellBereiche] = {4, 1, 1, 1, 1, 3};
    static const int kBlAktiv[kModellBereiche] = {1, 2, 3, 3, 0, 4};
    static const char* const kBereich[kModellBereiche] = {"Ansehen/Pruefen", "Modellieren", "UV", "Weights", "Painting", "Animation"};
    auto tipps = std::make_shared<std::array<std::string, 6>>();   // [modus * 3 + (Ansehen, Modellieren, Animation)]
    for (int modus = 0; modus < 2; ++modus) {
        add(tu(std::string("AP3: Bedienmodus ") + (modus == 0 ? "wie 3ds Max" : "wie Blender") + ", Ansehen/Pruefen", [=] {
            modellBedienungSetzen(modus);
            g_app->mtab.bereich = ModellBereich::Ansehen;
            g_app->settings.modellFeldBreite = 0.0F;
        }));
        add(pause(0.6));
        // --- F1 / F2: Titelzeile und Reiter ---
        if (modus == 0) {
            add(pruefe(zeile("F1", modus, "Titelzeile, darunter die Reiter Erstellen, Aendern, Hierarchie, Bewegung, Anzeige, Dienstprogramme als Reihe mit Text, darunter die Gruppen"), [] {
                const ui::FeldProbe& f = ui::feldProbe();
                const ImGuiWindow* w = kindFenster("mtbefehlsfeld");
                if (!feldFrisch() || w == nullptr) { return false; }
                bool alle = true;
                float x = -1.0F;
                float y = -1.0F;
                for (const Str r : kMaxReiter) {
                    const Element* e = imFeld(tr(r));
                    alle = alle && e != nullptr && f.reiter.Contains(e->rect);
                    // von links nach rechts, bei Umbruch in der naechsten Reihe
                    if (e != nullptr) {
                        alle = alle && (e->rect.Min.x > x || e->rect.Min.y > y);
                        x = e->rect.Min.x;
                        y = e->rect.Min.y;
                    }
                }
                melde("F1 Feld: " + mbefoto::rechteck("titel", f.titel) + " " + mbefoto::rechteck("reiter", f.reiter) + " " + mbefoto::rechteck("inhalt", f.inhalt) + " Reiter " + std::to_string(f.reiterZahl));
                return alle && f.stil == ui::RolloutStil::Max && !f.reiterSenkrecht && f.reiterZahl == 6 && w->Rect().Contains(f.titel) && f.reiter.Min.y >= f.titel.Max.y - 1.0F &&
                       f.inhalt.Min.y >= f.reiter.Max.y - 1.0F;
            }));
        } else {
            add(pruefe(zeile("F2", modus, "Titelzeile \"Properties\", die Reiter Werkzeug, Objekt, Modifier, Daten, Knochen, Constraints, Material als senkrechte Symbolspalte links neben den Gruppen"), [] {
                const ui::FeldProbe& f = ui::feldProbe();
                const ImGuiWindow* w = kindFenster("mtbefehlsfeld");
                if (!feldFrisch() || w == nullptr) { return false; }
                bool alle = true;
                float y = -1.0F;
                for (const Str r : kBlReiter) {
                    const Element* e = imFeld(tr(r));
                    alle = alle && e != nullptr && f.reiter.Contains(e->rect) && e->rect.Min.y > y && std::fabs(e->rect.GetWidth() - e->rect.GetHeight()) < 1.0F;
                    if (e != nullptr) { y = e->rect.Min.y; }
                }
                melde("F2 Feld: " + mbefoto::rechteck("titel", f.titel) + " " + mbefoto::rechteck("reiter", f.reiter) + " " + mbefoto::rechteck("inhalt", f.inhalt) + " Reiter " + std::to_string(f.reiterZahl));
                return alle && f.stil == ui::RolloutStil::Blender && f.reiterSenkrecht && f.reiterZahl == 7 && w->Rect().Contains(f.titel) && f.reiter.Min.y >= f.titel.Max.y - 1.0F &&
                       f.reiter.Max.x <= f.inhalt.Min.x + 1.0F && f.reiter.GetWidth() < em() * 2.6F;
            }));
            add(tu("AP3 F2: Maus auf den Reiter Daten", [] {
                const Element* e = imFeld(tr(Str::MfDaten));
                setzeMaus(e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F});
            }));
            add(pause(1.2));
            add(pruefe(zeile("F2", modus, "ein Reiter der Symbolspalte nennt seinen Namen im Tooltip"), [] { return imFeld(tr(Str::MfDaten)) != nullptr && tooltipDa(); }));
            add(tu("AP3: Maus weg", [] { setzeMaus(ImVec2{-50.0F, -50.0F}); }));
        }
        add(pruefe(zeile(modus == 0 ? "F9" : "F10", modus, modus == 0 ? "das Feld zeichnet im Stil wie 3ds Max" : "das Feld zeichnet im Stil wie Blender"), [=] {
            return feldFrisch() && ui::feldProbe().stil == (modus == 0 ? ui::RolloutStil::Max : ui::RolloutStil::Blender) && ui::rolloutStil() == ui::feldProbe().stil;
        }));
        // --- der Bereich waehlt den Reiter des Entwurfs ---
        for (int b = 0; b < kModellBereiche; ++b) {
            add(tu(std::string("AP3: Bereich ") + kBereich[b], [=] { g_app->mtab.bereich = static_cast<ModellBereich>(b); }));
            add(pause(0.35));
            const int soll = modus == 0 ? kMaxAktiv[b] : kBlAktiv[b];
            add(pruefe(zeile(modus == 0 ? "F1" : "F2", modus, std::string("im Bereich ") + kBereich[b] + " ist der Reiter des Entwurfs gewaehlt (" + std::to_string(soll) + ")"), [=] {
                const ui::FeldProbe& f = ui::feldProbe();
                const Element* e = imFeld(tr(modus == 0 ? kMaxReiter[soll] : kBlReiter[soll]));
                return feldFrisch() && f.reiterAktiv == soll && e != nullptr && (e->flags & ImGuiItemStatusFlags_Checked) != 0;
            }));
            if (b == 0 || b == 1 || b == 5) {
                const int platz = modus * 3 + (b == 0 ? 0 : b == 1 ? 1 : 2);
                add(tu("AP3 R3: Hinweis merken", [=] { (*tipps)[static_cast<std::size_t>(platz)] = modellFeld().kopfBild >= ImGui::GetFrameCount() - 2 ? modellFeld().tippText : std::string(); }));
            }
        }
        add(tu("AP3: zurueck nach Ansehen/Pruefen", [] { g_app->mtab.bereich = ModellBereich::Ansehen; }));
        add(pause(0.4));
        add(klick(tr(modus == 0 ? Str::MfErstellen : Str::MfWerkzeug), "mtbefehlsfeld"));
        add(pause(0.3));
        add(pruefe(zeile(modus == 0 ? "F1" : "F2", modus, "ein Reiter ohne Gruppen in diesem Bereich ist nicht waehlbar (der gewaehlte bleibt)"), [=] {
            return feldFrisch() && ui::feldProbe().reiterAktiv == (modus == 0 ? 4 : 1) && (ui::feldProbe().reiterBelegt & 1U) == 0U;
        }));
        // --- R4: der Schalter des Bedienmodus ---
        add(pruefe(zeile("R4", modus, "im rechten Feld steht kein Schalter des Bedienmodus mehr"), [] {
            return feldFrisch() && imFeld(tr(Str::MbWieMax)) == nullptr && imFeld(tr(Str::MbWieBlender)) == nullptr;
        }));
        add(pruefe(zeile("R4", modus, "der Bedienmodus bleibt erreichbar: Statuszeile, Alle Aktionen suchen"), [] {
            bool suche = false;
            for (int i = 0; i < modellZusatzZahl(); ++i) { suche = suche || std::string(modellZusatzText(i)).find(tr(Str::MbWieBlender)) != std::string::npos; }
            return finde("##mzmodus", "", 0, false) != nullptr && suche;
        }));
        // --- F13: Breite des Felds ---
        add(pruefe(zeile("F13", modus, "das Feld ist ab Werk 22 Zeichenhoehen breit"), [] {
            melde("F13 Breite " + std::to_string(feldBreiteEm()) + " Zeichenhoehen");
            return std::fabs(feldBreiteEm() - 22.0F) < 0.15F;
        }));
        if (modus == 0) {
            auto vorher = std::make_shared<float>(0.0F);
            add(tu("AP3 F13: Breite vor dem Ziehen", [=] {
                const ImGuiWindow* w = kindFenster("mtbefehlsfeld");
                *vorher = w != nullptr ? w->Size.x : 0.0F;
            }));
            add(ziehenAn("AP3 F13: die Trennkante 90 Bildpunkte nach links", trennerOrt, -90.0F, 0.0F, 0));
            add(pause(0.3));
            add(pruefe(zeile("F13", modus, "Ziehen an der Trennkante um 90 Bildpunkte nach links macht das Feld 90 Bildpunkte breiter, die Einstellung merkt es"), [=] {
                const ImGuiWindow* w = kindFenster("mtbefehlsfeld");
                const float jetzt = w != nullptr ? w->Size.x : 0.0F;
                melde("F13 Ziehen: " + std::to_string(*vorher) + " -> " + std::to_string(jetzt) + " Bildpunkte, Einstellung " + std::to_string(g_app->settings.modellFeldBreite) +
                      " Zeichenhoehen");
                return *vorher > 0.0F && std::fabs(jetzt - *vorher - 90.0F) < 2.0F && std::fabs(g_app->settings.modellFeldBreite * em() - jetzt) < 2.0F;
            }));
            add(klickAn("AP3 F13: Doppelklick auf die Trennkante", trennerOrt, 0, true));
            add(pause(0.3));
            add(pruefe(zeile("F13", modus, "ein Doppelklick auf die Trennkante stellt die Vorgabe wieder her"), [] {
                return std::fabs(feldBreiteEm() - 22.0F) < 0.15F && g_app->settings.modellFeldBreite == 0.0F;
            }));
        }
        // --- R2: die Bereiche als Reihe ueber die ganze Breite ---
        add(pruefe(zeile("R2", modus, "die Bereiche stehen als eine Reihe ueber Explorer, Ansichten und rechtem Feld"), [] {
            const ModellFeld& f = modellFeld();
            const ImGuiWindow* ex = kindFenster("mtexplorer");
            const ImGuiWindow* an = kindFenster("mtansichten");
            const ImGuiWindow* feld = kindFenster("mtbefehlsfeld");
            const ImGuiWindow* leiste = kindFenster("modebar");
            if (f.kopfBild < ImGui::GetFrameCount() - 2 || ex == nullptr || an == nullptr || feld == nullptr || leiste == nullptr) { return false; }
            const float unten = f.kopfY + ImGui::GetFrameHeight();
            char z[240];
            std::snprintf(z, sizeof z, "R2 Reihe x %.0f..%.0f y %.0f..%.0f, Explorer ab %.0f/%.0f, Ansichten y %.0f, Feld y %.0f, Modusleiste bis x %.0f", static_cast<double>(f.kopfVon),
                          static_cast<double>(f.kopfBis), static_cast<double>(f.kopfY), static_cast<double>(unten), static_cast<double>(ex->Pos.x), static_cast<double>(ex->Pos.y),
                          static_cast<double>(an->Pos.y), static_cast<double>(feld->Pos.y), static_cast<double>(leiste->Pos.x + leiste->Size.x));
            melde(z);
            return unten <= ex->Pos.y + 1.0F && unten <= an->Pos.y + 1.0F && unten <= feld->Pos.y + 1.0F && f.kopfVon >= leiste->Pos.x + leiste->Size.x - 1.0F &&
                   f.kopfVon <= ex->Pos.x + em() * 2.0F;
        }));
        add(pruefe(zeile("R2", modus, "sechs gleich breite Felder (mindestens 7,5 Zeichenhoehen), das gewaehlte ist markiert"), [] {
            float breite = -1.0F;
            int markiert = 0;
            for (int b = 0; b < kModellBereiche; ++b) {
                const Element* e = kopfFeld(b);
                if (e == nullptr) { return false; }
                if (breite >= 0.0F && std::fabs(e->rect.GetWidth() - breite) > 1.0F) { return false; }
                breite = e->rect.GetWidth();
                if ((e->flags & ImGuiItemStatusFlags_Checked) != 0) { markiert += b == static_cast<int>(g_app->mtab.bereich) ? 1 : 10; }
            }
            melde("R2 Feldbreite " + std::to_string(breite / em()) + " Zeichenhoehen");
            return breite >= std::floor(em() * 7.5F) - 1.0F && markiert == 1;
        }));
        add(klickAn("AP3 R2: auf das Feld Modellieren", [] {
            const Element* e = kopfFeld(1);
            return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
        }));
        add(pause(0.3));
        add(pruefe(zeile("R2", modus, "ein Klick auf ein Feld wechselt den Bereich"), [] { return g_app->mtab.bereich == ModellBereich::Modellieren; }));
        add(tu("AP3: zurueck nach Ansehen/Pruefen", [] { g_app->mtab.bereich = ModellBereich::Ansehen; }));
        add(pause(0.3));
        // --- R3: der Hinweis neben den Reitern ---
        add(pruefe(zeile("R3", modus, "neben den Bereichen steht der Hinweis des Bedienmodus: fuer Ansehen/Pruefen, die bearbeitenden Bereiche und Animation je ein eigener"), [=] {
            const std::size_t ab = static_cast<std::size_t>(modus) * 3U;
            const std::string& a = (*tipps)[ab];
            const std::string& b = (*tipps)[ab + 1U];
            const std::string& c = (*tipps)[ab + 2U];
            const ModellFeld& f = modellFeld();
            const std::string nav = tr(modus == 0 ? Str::MnZeileMax : Str::MnZeileBlender);
            melde("R3 Ansehen: " + a);
            melde("R3 Modellieren: " + b);
            melde("R3 Animation: " + c);
            melde("R3 gezeigt: " + f.tippGezeigt);
            return !a.empty() && !b.empty() && !c.empty() && a != b && b != c && a != c && a.find(nav) != std::string::npos && !f.tippGezeigt.empty();
        }));
    }
    add(pruefe("AP3 R3: der Hinweis unterscheidet sich zwischen wie 3ds Max und wie Blender", [=] { return !(*tipps)[0].empty() && (*tipps)[0] != (*tipps)[3] && (*tipps)[1] != (*tipps)[4]; }));
    // --- UIFORM (offener Punkt): Formularzeilen der Gruppe Karten-GLA ---
    add(tu("AP3: wie 3ds Max, Animation, Ziel Karten-GLA", [] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Animation;
        modellGlaZielSetzen(0);
    }));
    add(pause(0.6));
    add(pruefe("AP3 UIFORM: in der Gruppe Karten-GLA beginnen die Felder Ziel und Karte an derselben Kante und enden am selben Rand", [] {
        // der Ausklapper "Ziel" meldet seine Beschriftung nur aufgeklappt: ueber die Kennung
        ImGuiWindow* w = kindFenster("mtrollouts");
        const Element* a = w != nullptr ? findeKennung(w->GetID("##glaziel")) : nullptr;
        // das Feld "Karte" kann ausserhalb des sichtbaren Bereichs liegen (dann meldet es keine Beschriftung): ebenfalls ueber die Kennung
        const Element* b = w != nullptr ? findeKennung(w->GetID("##kgkarte")) : nullptr;
        if (a == nullptr || b == nullptr) {
            melde(std::string("UIFORM Felder: Ziel ") + (a != nullptr ? "da" : "fehlt") + ", Karte " + (b != nullptr ? "da" : "fehlt"));
            return false;
        }
        melde("UIFORM Felder: " + mbefoto::rechteck("ziel", a->rect) + " " + mbefoto::rechteck("karte", b->rect));
        return std::fabs(a->rect.Min.x - b->rect.Min.x) < 0.5F && std::fabs(a->rect.Max.x - b->rect.Max.x) < 0.5F;
    }));
    add(tu("AP3: zurueck nach Ansehen/Pruefen", [] { g_app->mtab.bereich = ModellBereich::Ansehen; }));
    add(pause(0.3));
}

// --- Die Zeilen der Abgleichtabelle: die Form der Gruppen am Muster Ansehen/Pruefen ---------------------------------------
// F3 (Gruppe mit Kurzinfo und Kasten), F5 (Knopfraster), F8 (Werte als Tabelle), F11 (Tipp), F12 (Wahl als Segment),
// F9/F10 (Stil der Gruppen), F1/F2 (jede Gruppe gehoert zu einem Reiter), F16 (Aussehen wie 3ds Max im Ganzen)

const ui::RolloutProbe* feldGruppe(Str titel) {
    const std::string name = tr(titel);
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
        if (ui::rolloutProbe(i).titel == name) { return &ui::rolloutProbe(i); }
    }
    return nullptr;
}

// Die Titel der gezeichneten Gruppen des rechten Felds, mit | getrennt.
std::string feldGruppen() {
    std::string s;
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) { s += (i > 0 ? "|" : "") + ui::rolloutProbe(i).titel; }
    return s;
}

const Element* imRoll(const std::string& label) { return inFenster(label, "mtrollouts"); }

void musterSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        add(tu(std::string("AP3 Muster: Bedienmodus ") + (modus == 0 ? "wie 3ds Max" : "wie Blender") + ", Ansehen/Pruefen, torso gewaehlt", [=] {
            modellBedienungSetzen(modus);
            g_app->mtab.bereich = ModellBereich::Ansehen;
            g_app->settings.modellFeldBreite = 0.0F;
            g_app->mtab.lodWahl = 0;
            g_app->mtab.auswahl = {mb1::teil("torso")};
            if (g_app->mtab.anbauten.empty()) { (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true); }
        }));
        add(pause(0.7));
        // AP5: die Gruppen des Entwurfs (Auswahl, Boden und Kamera, Szene dazu)
        const std::string alle = std::string(tr(Str::MbRModell)) + "|" + tr(Str::PqRAuswahl) + "|" + tr(Str::MbRLod) + "|" + tr(Str::PqRBoden) + "|" + tr(Str::MbRAnbauten) + "|" +
                                 tr(Str::MbRPruefen) + "|" + tr(Str::PqRSzene);
        // --- F3: Gruppe mit Kurzinfo und Kasten ---
        add(pruefe(zeile("F3", modus, "im Reiter des Bereichs stehen die Gruppen untereinander: Modell, Auswahl, Detailstufe und Skin, Boden und Kamera, Anbauten, Pruefung, Szene"), [=] {
            melde("F3 Gruppen: " + feldGruppen());
            return feldFrisch() && feldGruppen() == alle;
        }));
        add(pruefe(zeile("F3", modus, "jede Gruppe ist ein Kasten mit Ueberschrift und Kurzinfo rechts (Dreiecke, LOD 1, Zahl der Anbauten, Befunde)"), [] {
            const ui::RolloutProbe* m = feldGruppe(Str::MbRModell);
            const ui::RolloutProbe* l = feldGruppe(Str::MbRLod);
            const ui::RolloutProbe* a = feldGruppe(Str::MbRAnbauten);
            const ui::RolloutProbe* p = feldGruppe(Str::MbRPruefen);
            if (m == nullptr || l == nullptr || a == nullptr || p == nullptr) { return false; }
            char lod[32];
            std::snprintf(lod, sizeof lod, tr(Str::MfInfoLod), g_app->mtab.lodJetzt + 1);
            melde("F3 Kurzinfos: " + m->kurzinfo + " | " + l->kurzinfo + " | " + a->kurzinfo + " | " + p->kurzinfo);
            bool ok = m->kurzinfo.find_first_of("0123456789") != std::string::npos && l->kurzinfo == lod && a->kurzinfo == "1" && !p->kurzinfo.empty();
            for (const ui::RolloutProbe* g : {m, l, a, p}) {
                ok = ok && g->offen && g->info.GetWidth() > 0.0F && g->info.Max.x <= g->kopf.Max.x && g->info.Max.x >= g->kopf.Max.x - em() && g->kasten.Max.y > g->kopf.Max.y + em() &&
                     g->kasten.Min.y == g->kopf.Min.y;
            }
            // untereinander, ohne Ueberlappung
            return ok && l->kasten.Min.y >= m->kasten.Max.y && a->kasten.Min.y >= l->kasten.Max.y && p->kasten.Min.y >= a->kasten.Max.y;
        }));
        add(pruefe(zeile(modus == 0 ? "F9" : "F10", modus, modus == 0 ? "Gruppen im Stil wie 3ds Max: flacher Balken, eckig" : "Gruppen im Stil wie Blender: abgerundete Kaesten"), [=] {
            const ui::RolloutProbe* m = feldGruppe(Str::MbRModell);
            if (m == nullptr) { return false; }
            return modus == 0 ? (m->stil == ui::RolloutStil::Max && m->rundung <= 2.0F) : (m->stil == ui::RolloutStil::Blender && m->rundung >= std::floor(em() * 0.3F));
        }));
        add(insFeld(tr(Str::MbRLod)));   // AP5: mit den neuen Gruppen (Auswahl) steht die Ueberschrift wie Blender tiefer
        add(pause(0.2));
        add(klick(tr(Str::MbRLod), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("F3", modus, "ein Klick auf die Ueberschrift klappt die Gruppe zu"), [] {
            const ui::RolloutProbe* l = feldGruppe(Str::MbRLod);
            return l != nullptr && !l->offen && l->kasten.GetHeight() == l->kopf.GetHeight();
        }));
        add(insFeld(tr(Str::MbRLod)));   // AP5: mit den neuen Gruppen (Auswahl) steht die Ueberschrift wie Blender tiefer
        add(pause(0.2));
        add(klick(tr(Str::MbRLod), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("F3", modus, "ein zweiter Klick klappt sie wieder auf"), [] {
            const ui::RolloutProbe* l = feldGruppe(Str::MbRLod);
            return l != nullptr && l->offen;
        }));
        // --- F5: Knopfraster ---
        // AP5 P1: in Modell Andere GLA, Texturen neu laden (Alt+R in beiden Bedienmodi wie im Entwurf), Importieren; Foto in "Boden und Kamera"
        // AP11 P1: dazu wie im Entwurf "In neuem Tab oeffnen" (Strg+O) neben Importieren - vier Knoepfe, zwei mit Taste
        add(pruefe(zeile("F5", modus, "Modell: die Knoepfe Andere GLA, Texturen neu laden, Importieren, In neuem Tab oeffnen stehen im Raster mit zwei gleich breiten Spalten, jeder mit Symbol, Texturen neu laden mit der Taste Alt+R, In neuem Tab oeffnen mit Strg+O"), [=] {
            const ui::RolloutProbe* m = feldGruppe(Str::MbRModell);
            const Element* a = imRoll(tr(Str::MbAndereGla));
            const Element* b = imRoll(tr(Str::MbTexNeu));
            const Element* c = imRoll(tr(Str::PqImport));
            const Element* d = imRoll(tr(Str::MxNeuerTab));
            if (m == nullptr || a == nullptr || b == nullptr || c == nullptr || d == nullptr) { return false; }
            melde("F5 Raster: " + mbefoto::rechteck("gla", a->rect) + " " + mbefoto::rechteck("tex", b->rect) + " " + mbefoto::rechteck("foto", c->rect) + " " +
                  mbefoto::rechteck("tab", d->rect) + ", Spalten " + std::to_string(m->rasterSpalten) + ", Knoepfe " + std::to_string(m->knoepfe));
            return m->rasterSpalten == 2 && m->knoepfe == 4 && m->symbole == 4 && m->tasten == 2 && std::fabs(a->rect.GetWidth() - b->rect.GetWidth()) < 0.5F && a->rect.Min.y == b->rect.Min.y && a->rect.Max.y == b->rect.Max.y &&
                   b->rect.Min.x > a->rect.Max.x && c->rect.Min.y > a->rect.Max.y && std::fabs(c->rect.Min.x - a->rect.Min.x) < 0.5F && std::fabs(c->rect.GetWidth() - a->rect.GetWidth()) < 0.5F &&
                   d->rect.Min.y == c->rect.Min.y && std::fabs(d->rect.Min.x - b->rect.Min.x) < 0.5F && std::fabs(d->rect.GetWidth() - b->rect.GetWidth()) < 0.5F &&
                   m->kasten.Contains(a->rect) && m->kasten.Contains(b->rect) && m->kasten.Contains(c->rect) && m->kasten.Contains(d->rect);
        }));
        // --- F8: Werte als Tabelle ---
        add(pruefe(zeile("F8", modus, "Modell: Datei, Animationen (GLA), Flaechen, Dreiecke, Ecken, Knochen als Zeilen einer Tabelle ueber die Breite der Gruppe"), [] {
            const ui::RolloutProbe* m = feldGruppe(Str::MbRModell);
            if (m == nullptr) { return false; }
            float y = m->kopf.Max.y;
            for (const Str z : {Str::MbDatei, Str::MbGla, Str::MbXArtFlaechen, Str::MfDreiecke, Str::MfEcken, Str::MbXArtKnochen}) {
                const Element* e = imRoll(tr(z));
                if (e == nullptr || !m->kasten.Contains(e->rect) || e->rect.Min.y < y - 0.5F || e->rect.GetWidth() < m->kasten.GetWidth() - em() * 1.2F) {
                    melde(std::string("F8 Zeile fehlt oder liegt falsch: ") + tr(z));
                    return false;
                }
                y = e->rect.Max.y;
            }
            return true;
        }));
        add(pruefe(zeile("F8", modus, "Pruefung: die Grenzen des Zielspiels als Zeilen einer Tabelle, davor die Wahl des Zielspiels als Formularzeile"), [] {
            const ui::RolloutProbe* p = feldGruppe(Str::MbRPruefen);
            // der Ausklapper meldet seine Beschriftung nur aufgeklappt: ueber die Kennung
            ImGuiWindow* w = kindFenster("mtrollouts");
            const Element* ziel = w != nullptr ? findeKennung(w->GetID("##zielspiel")) : nullptr;
            if (p == nullptr || ziel == nullptr) { return false; }
            bool ok = p->kasten.Contains(ziel->rect) && ziel->rect.Min.x > p->kasten.Min.x + em() * 2.0F;
            float y = ziel->rect.Max.y;
            for (const Str z : {Str::MfPrEcken, Str::MfPrDreiecke, Str::MfPrKnochen, Str::MfPrGewichte, Str::MfPrInstanzen}) {
                const Element* e = imRoll(tr(z));
                ok = ok && e != nullptr && p->kasten.Contains(e->rect) && e->rect.Min.y >= y - 0.5F;
                if (e != nullptr) { y = e->rect.Max.y; }
            }
            return ok;
        }));
        // --- F11: Tipp und Listenzeile ---
        add(pruefe(zeile("F11", modus, "Anbauten: je Anbau eine Zeile mit x rechts, am Ende der Tipp als Kasten"), [] {
            const ui::RolloutProbe* a = feldGruppe(Str::MbRAnbauten);
            const Element* x = imRoll(tr(Str::MbLoesen));
            const Element* tipp = imRoll(tr(Str::MfAnbauTipp));
            const Element* alle = imRoll(tr(Str::MbAlleLoesen));
            if (a == nullptr || x == nullptr || tipp == nullptr || alle == nullptr) { return false; }
            melde("F11 Anbauten: " + mbefoto::rechteck("x", x->rect) + " " + mbefoto::rechteck("tipp", tipp->rect) + " " + mbefoto::rechteck("kasten", a->kasten));
            return a->kasten.Contains(x->rect) && x->rect.Max.x >= a->kasten.Max.x - em() * 1.5F && tipp->rect.Min.y >= alle->rect.Max.y && a->kasten.Contains(tipp->rect) &&
                   tipp->rect.GetWidth() >= a->kasten.GetWidth() - em() * 1.2F;
        }));
        add(tu("AP3 Muster: die Gruppe Anbauten ins Bild rollen", [] {
            const ui::RolloutProbe* a = feldGruppe(Str::MbRAnbauten);
            ImGuiWindow* w = kindFenster("mtrollouts");
            if (a != nullptr && w != nullptr) { ImGui::SetScrollY(w, std::clamp(w->Scroll.y + a->kasten.Min.y - w->InnerRect.Min.y - em(), 0.0F, w->ScrollMax.y)); }
        }));
        add(pause(0.3));
        add(klick(tr(Str::MbLoesen), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(zeile("F11", modus, "das x loest den Anbau; ohne Anbauten steht nur der Hinweis als Kasten da"), [] {
            const ui::RolloutProbe* a = feldGruppe(Str::MbRAnbauten);
            return g_app->mtab.anbauten.empty() && a != nullptr && a->kurzinfo.empty() && imRoll(tr(Str::MbAnbauKeine)) != nullptr && imRoll(tr(Str::MbLoesen)) == nullptr;
        }));
        add(tu("AP3 Muster: Saebel wieder an *r_hand, nach oben rollen", [] {
            (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true);
            if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
        }));
        add(pause(0.5));
        // --- F12: Wahl als Segment ---
        add(pruefe(zeile("F12", modus, "Detailstufe: Haken Auto (dann ist im Segment keine Stufe gewaehlt), darunter 1-10 als eine Reihe ueber die Breite, Picmip 0-3 als Segment hinter der Beschriftung"), [] {
            const ui::RolloutProbe* l = feldGruppe(Str::MbRLod);
            const Element* automatisch = imRoll(tr(Str::MbLodAuto));
            const Element* eins = imRoll("1");
            const Element* zehn = imRoll("10");
            const Element* p0 = imRoll("0##pm");
            const Element* p3 = imRoll("3##pm");
            if (l == nullptr || automatisch == nullptr || eins == nullptr || zehn == nullptr || p0 == nullptr || p3 == nullptr) { return false; }
            melde("F12 Segmente: " + mbefoto::rechteck("1", eins->rect) + " " + mbefoto::rechteck("10", zehn->rect) + " " + mbefoto::rechteck("p0", p0->rect) + " " + mbefoto::rechteck("p3", p3->rect));
            return (automatisch->flags & ImGuiItemStatusFlags_Checked) != 0 && (eins->flags & ImGuiItemStatusFlags_Checked) == 0 && eins->rect.Min.y == zehn->rect.Min.y &&
                   eins->rect.Min.y >= automatisch->rect.Max.y - 1.0F &&
                   zehn->rect.Max.x - eins->rect.Min.x >= l->kasten.GetWidth() - em() * 1.2F && p0->rect.Min.y == p3->rect.Min.y && p0->rect.Min.x > l->kasten.Min.x + em() * 2.0F &&
                   l->kasten.Contains(zehn->rect) && l->kasten.Contains(p3->rect);
        }));
        add(insFeld("1"));
        add(pause(0.3));
        add(klick("1", "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("F12", modus, "ein Klick auf 1 waehlt die Stufe fest (Auto aus)"), [] {
            const Element* automatisch = imRoll(tr(Str::MbLodAuto));
            const Element* eins = imRoll("1");
            return g_app->mtab.lodWahl == 1 && automatisch != nullptr && (automatisch->flags & ImGuiItemStatusFlags_Checked) == 0 && eins != nullptr &&
                   (eins->flags & ImGuiItemStatusFlags_Checked) != 0;
        }));
        add(insFeld(tr(Str::MbLodAuto)));
        add(pause(0.3));
        add(klick(tr(Str::MbLodAuto), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("F12", modus, "der Haken Auto stellt wieder auf wie im Spiel"), [] { return g_app->mtab.lodWahl == 0; }));
        // --- F1 / F2: jede Gruppe gehoert zu einem Reiter ---
        const Str reiterA = modus == 0 ? Str::MfHierarchie : Str::MfConstraints;
        const Str reiterB = modus == 0 ? Str::MfDienste : Str::MfDaten;
        const Str reiterStd = modus == 0 ? Str::MfAnzeige : Str::MfObjekt;
        const std::string sollA = tr(Str::MbRAnbauten);
        const std::string sollB = modus == 0 ? std::string(tr(Str::MbRPruefen)) + "|" + tr(Str::PqRSzene) : std::string(tr(Str::MbRModell)) + "|" + tr(Str::MbRLod);   // AP5: Szene
        add(klick(tr(reiterA), "mtbefehlsfeld"));
        add(pause(0.4));
        add(pruefe(zeile(modus == 0 ? "F1" : "F2", modus, modus == 0 ? "der Reiter Hierarchie zeigt nur die Anbauten" : "der Reiter Constraints zeigt nur die Anbauten"), [=] {
            melde("Reiter A: " + feldGruppen());
            return feldGruppen() == sollA;
        }));
        add(klick(tr(reiterB), "mtbefehlsfeld"));
        add(pause(0.4));
        add(pruefe(zeile(modus == 0 ? "F1" : "F2", modus, modus == 0 ? "der Reiter Dienstprogramme zeigt die Pruefung fuer das Spiel und die Szene" : "der Reiter Daten zeigt Modell und Detailstufe"), [=] {
            melde("Reiter B: " + feldGruppen());
            return feldGruppen() == sollB;
        }));
        add(klick(tr(reiterStd), "mtbefehlsfeld"));
        add(pause(0.4));
        add(pruefe(zeile(modus == 0 ? "F1" : "F2", modus, "zurueck im Reiter des Bereichs stehen wieder alle Gruppen"), [=] { return feldGruppen() == alle; }));
        if (modus == 0) {
            // --- F16: das Ganze wie 3ds Max ---
            add(pruefe(zeile("F16", modus, "Befehlsfeld mit Reitern, Gruppen mit Kurzinfo im Max-Stil, Knopfraster, Wertetabelle, Tippkasten - alles im selben Feld"), [=] {
                const ui::FeldProbe& f = ui::feldProbe();
                const ui::RolloutProbe* m = feldGruppe(Str::MbRModell);
                return feldFrisch() && f.stil == ui::RolloutStil::Max && f.reiterZahl == 6 && !f.reiterSenkrecht && m != nullptr && m->stil == ui::RolloutStil::Max && !m->kurzinfo.empty() &&
                       m->rasterSpalten == 2 && imRoll(tr(Str::MbDatei)) != nullptr && imRoll(tr(Str::MfAnbauTipp)) != nullptr && feldGruppen() == alle;
            }));
        }
    }
}

// --- Fotos des rechten Felds: beide Bedienmodi, jeder Bereich, die Reiter von Ansehen/Pruefen ------------------------------

void feldFotoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    static const char* const kBereich[kModellBereiche] = {"pruefen", "modell", "uv", "weights", "paint", "anim"};
    for (int modus = 0; modus < 2; ++modus) {
        const std::string p = modus == 0 ? "ap3_max_" : "ap3_blender_";
        for (int b = 0; b < kModellBereiche; ++b) {
            add(tu(std::string("AP3 Foto: ") + p + kBereich[b], [=] {
                modellBedienungSetzen(modus);
                g_app->mtab.bereich = static_cast<ModellBereich>(b);
                g_app->settings.modellFeldBreite = 0.0F;
                g_app->mtab.auswahl = {mb1::teil("torso")};
                if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
            }));
            add(pause(0.8));
            add(foto(p + kBereich[b]));
            if (b == 0) {
                add(tu("AP3 Foto: Reiter Hierarchie / Constraints", [=] { modellFeldReiter(MbReiter::Hierarchie, MbReiterBl::Constraints); }));
                add(pause(0.5));
                add(foto(p + "pruefen_reiter_a"));
                add(tu("AP3 Foto: Reiter Dienstprogramme / Daten", [=] { modellFeldReiter(MbReiter::Dienste, MbReiterBl::Daten); }));
                add(pause(0.5));
                add(foto(p + "pruefen_reiter_b"));
                add(tu("AP3 Foto: zurueck in den Reiter des Bereichs", [=] { modellFeldReiter(MbReiter::Anzeige, MbReiterBl::Objekt); }));
                add(pause(0.4));
                add(tu("AP3 Foto: ganz nach unten gerollt", [] {
                    if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, w->ScrollMax.y); }
                }));
                add(pause(0.5));
                add(foto(p + "pruefen_unten"));
            }
        }
    }
    add(tu("AP3 Foto: zurueck", [] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Ansehen;
    }));
    add(pause(0.3));
}

// Bilder fuer das Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap3): h_ap3_max, h_ap3_blender - dieselbe Figur wie die Seite
// "Model-Tab: Ansehen und Pruefen" (Sturmtruppler, Saebel an *r_hand).
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP3 Handbuch: Ansehen/Pruefen, Saebel an *r_hand, BOTH_STAND1, wie 3ds Max", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = 0.85F;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Ansehen;
        (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true);
        animWaehlen(mb1::animNach("BOTH_STAND1") >= 0 ? mb1::animNach("BOTH_STAND1") : 0, false);
        modellExplorerVorgabe();
        g_app->mtab.auswahl.clear();
        modellAnsichtEinpassenAktiv();
        g_app->mtab.auswahl = {mb1::teil("torso")};
    }));
    add(pause(1.2));
    add(hilfe2::schrittFoto("h_ap3_max"));
    add(tu("AP3 Handbuch: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(0.9));
    add(hilfe2::schrittFoto("h_ap3_blender"));
    add(tu("AP3 Handbuch: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        modellAnbautenLoesen();
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "ap3" && n != "ap3foto") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_AP3_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    const char* tl = std::getenv("BHED_AP3_TEIL");
    const std::string teil = tl != nullptr ? tl : "";
    const char* m = std::getenv("BHED_AP3_MODELL");
    const std::string modell = m != nullptr ? m : "models/players/kyle/model.glm";
    auto skala = std::make_shared<float>(1.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP3: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        if (const char* t = std::getenv("BHED_AP3_THEMA")) {
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
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap3.txt");
    }));
    add(pause(0.8));
    if (teil.empty() || teil == "baustein") { bausteinSchritte(s, n == "ap3foto"); }
    if (teil.empty() || teil != "baustein") {
        for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
        add(tu("AP3: Aufteilung wie neue Einstellungen, Saebel an *r_hand, Sequenz, wie 3ds Max", [=] {
            *teiler = g_app->settings.splitModel;
            *bedienung = g_app->settings.modellBedienung;
            *feldBreite = g_app->settings.modellFeldBreite;
            g_app->settings.splitModel = Settings{}.splitModel;
            g_app->settings.modellFeldBreite = 0.0F;
            modellBedienungSetzen(0);
            g_app->mtab.bereich = ModellBereich::Ansehen;
            g_app->mtab.vierAnsichten = false;
            (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true);
            animWaehlen(mbefoto::sequenz(), false);
            g_app->mtab.auswahl = {mb1::teil("torso")};
            char zeile[300];
            std::snprintf(zeile, sizeof zeile, "%s, %zu Teile, %zu Knochen, Anzeige %.0f x %.0f, Oberflaeche %.2f, Schrift %.0f Punkte hoch", g_app->modelPath.c_str(),
                          g_app->mtab.dok.inhalt().teile.size(), g_app->anim.bones.size(), static_cast<double>(ImGui::GetIO().DisplaySize.x),
                          static_cast<double>(ImGui::GetIO().DisplaySize.y), static_cast<double>(g_app->uiScale), static_cast<double>(ImGui::GetFontSize()));
            melde(zeile);
        }));
        add(pause(1.0));
        add(klickAn("AP3: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
        add(pause(0.4));
        if (n == "ap3" && (teil.empty() || teil == "rahmen")) { rahmenSchritte(s); }
        if (n == "ap3" && (teil.empty() || teil == "muster")) { musterSchritte(s); }
        if (n == "ap3foto" && teil != "baustein") { feldFotoSchritte(s); }
        add(tu("AP3: Modell-Tab zurueck", [=] {
            modellBedienungSetzen(*bedienung);
            g_app->settings.splitModel = *teiler;
            g_app->settings.modellFeldBreite = *feldBreite;
            g_app->mtab.bereich = ModellBereich::Ansehen;
            g_app->mtab.vierAnsichten = false;
            modellAnbautenLoesen();
        }));
    }
    add(tu("AP3: zurueck", [=] {
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

}  // namespace ap3
