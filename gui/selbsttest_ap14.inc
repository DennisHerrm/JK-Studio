// selbsttest_ap14.inc - AP14 (rc612): Zusammenfuehrung des Model-Tabs - Reste am Rollout-Baustein (AP3), die Zeitleiste bei
// Mindestgroesse, gleiche Namen in der Suche "Alle Aktionen"
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap14 | ap14foto; Handbuch BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap14)
//
//   ap14      W   wertTabelle bricht kein Wort mitten im Wort um - je Bereich und Bedienmodus, alle Gruppen offen
//             K   dasselbe fuer Knoepfe und Schalter des Felds
//             Z   Zeitleiste bei Mindestgroesse: mit offenen Spuren behaelt sie hoechstens den Anteil, den sie bei der Mindesthoehe
//                 des Entwurfs (700 px bei 14 px Schrift = 50 Zeichenhoehen) hat; das rechte Feld zeigt mehr als den Kopf seiner ersten Gruppe
//             R   Entscheidung B6: bei Mindestgroesse stehen die Reiter des Befehlsfelds (wie 3ds Max) nur als Symbole in EINER Reihe,
//                 der Name steht im Tooltip; ueber der Mindesthoehe des Entwurfs bleibt der Text
//             S   "Alle Aktionen suchen": keine zwei Zeilen mit demselben Text ohne unterscheidenden Zusatz ("Smear")
//             H   der Helfer kennt "Hoechstens 4 je Ecke" als Ort des rechten Felds, "Zeig es mir" fuehrt aus dem Tab Script ueber
//                 Model-Tab, Bereich Weights und die zugeklappte Gruppe zum Schalter (Orte aus tools/gen_hilfe_orte.py)
//             U1  Hauptknopf als Umschalter ("Gewichte malen"): Hauptknopf in beiden Zustaenden, gleiche Hoehe
//             U2  gesperrte Elemente tragen im Element-Register die Fahne gesperrt
//             U3  ui::absatz steht mit seinem Text im Element-Register
//             U4  ein gesperrter Schalter zeigt seinen Tooltip
//             U6  ui::segment ohne gewaehlten Eintrag (Festwerte des Weight Tools) betont keinen
//   ap14foto  nur Fotos (BHED_FOTOS=<Ordner>): je Bedienmodus Animation mit offenen Spuren, Weights (Spiegeln), die Suche
//   BHED_AP14_FENSTER=1350x900@1.0   Fenstergroesse und Oberflaeche; Vorgabe ist die Mindestgroesse (900 x 600 bei 150 %)
//   BHED_AP14_THEMA=hell|dunkel      das erste helle bzw. dunkle Farbschema
// Eingebunden in selbsttest.cpp (nach selbsttest_ap5.inc).
namespace ap14 {

using ap3::feldGruppe;
using ap3::imRoll;
using ap3::insFeld;
using mbefoto::kindFenster;

void melde(const std::string& z) { diag::detail("Knopftest: AP14 " + z); }

std::string zeile(const char* nr, int modus, const std::string& was) {
    return std::string("AP14 ") + nr + (modus == 0 ? " wie 3ds Max: " : modus == 1 ? " wie Blender: " : ": ") + was;
}

ModellTab& mtab() { return g_app->mtab; }

const char* bereichKurz(int b) {
    static const char* const k[kModellBereiche] = {"Ansehen/Pruefen", "Modellieren", "UV", "Weights", "Painting", "Animation"};
    return k[std::clamp(b, 0, kModellBereiche - 1)];
}

// Alle Gruppen und eingeklappten Teile des rechten Felds aufklappen (zweimal: ein eingeklappter Teil erscheint erst mit seiner Gruppe)
void alleAuf(std::vector<Schritt>& s) {
    for (int i = 0; i < 3; ++i) {
        s.push_back(mbefoto::gruppenAuf("mtrollouts"));
        s.push_back(pause(0.25));
    }
}

Schritt bereichSetzen(int modus, int bereich) {
    return tu(std::string("AP14: ") + (modus == 0 ? "wie 3ds Max" : "wie Blender") + ", Bereich " + bereichKurz(bereich), [=] {
        ModellTab& t = mtab();
        modellBedienungSetzen(modus);
        t.bereich = static_cast<ModellBereich>(bereich);
        t.auswahl = {mb1::teil("torso")};
        t.knochenWahl = bereich == static_cast<int>(ModellBereich::Weights) ? boneIndex(g_app->anim, "lhumerus") : -1;
        t.kvOffen = false;
        g_app->animPlaying = false;
        g_app->modelDirty = true;
        if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
        setzeMaus(ImVec2{-50.0F, -50.0F});
    });
}

// --- W / K: kein Wort bricht mitten im Wort um ------------------------------------------------------------------------------

void wortSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        for (int b = 0; b < kModellBereiche; ++b) {
            add(bereichSetzen(modus, b));
            add(pause(0.8));
            alleAuf(s);
            add(pause(0.4));
            const std::string wo = std::string(bereichKurz(b));
            add(pruefe(zeile("W", modus, wo + " - wertTabelle bricht kein Wort mitten im Wort um"), [wo] {
                const ui::WortbruchProbe& p = ui::wortbruchProbe();
                char z[200];
                std::snprintf(z, sizeof z, "W %s: Feld %.0f breit, Werte %d, Knoepfe %d, Schalter %d", wo.c_str(),
                              static_cast<double>(kindFenster("mtrollouts") != nullptr ? kindFenster("mtrollouts")->InnerRect.GetWidth() : 0.0F), p.werte, p.knoepfe, p.schalter);
                melde(std::string(z) + (p.erster.empty() ? "" : " - zuerst \"" + p.erster + "\""));
                return p.werte == 0;
            }));
            add(pruefe(zeile("K", modus, wo + " - Knoepfe und Schalter brechen kein Wort mitten im Wort um"), [] {
                const ui::WortbruchProbe& p = ui::wortbruchProbe();
                return p.knoepfe == 0 && p.schalter == 0;
            }));
        }
    }
}

// --- Z: die Zeitleiste bei Mindestgroesse ---------------------------------------------------------------------------------

// Die Mindesthoehe des Entwurfs in Zeichenhoehen: .app { min-height: 700px } bei body { font: 14px } (entwurf_modelltab.html)
constexpr float kEntwurfHoehe = 700.0F / 14.0F;

// --- R: Reiter des Befehlsfelds bei Mindestgroesse (Entscheidung B6) -------------------------------------------------------

void reiterSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(pruefe(zeile("R", 0, "bei Mindestgroesse stehen die Reiter des Befehlsfelds als Symbole in einer Reihe"), [] {
        const ui::FeldProbe& f = ui::feldProbe();
        const float fs = ImGui::GetFontSize();
        char z[200];
        std::snprintf(z, sizeof z, "R: Anzeige %.1f Zeichenhoehen, %d Reiter, Reihe %.0f hoch (%.1f Zeichenhoehen), Symbole %d", static_cast<double>(ImGui::GetIO().DisplaySize.y / fs),
                      f.reiterZahl, static_cast<double>(f.reiter.GetHeight()), static_cast<double>(f.reiter.GetHeight() / fs), f.reiterSymbole ? 1 : 0);
        melde(z);
        return ImGui::GetIO().DisplaySize.y < kEntwurfHoehe * fs && f.reiterZahl == 6 && !f.reiterSenkrecht && f.reiter.GetHeight() <= 2.0F * fs;
    }));
    // der Name steht im Tooltip: die Maus auf den dritten Reiter (Hierarchy)
    add(mbefoto::mausAuf("AP14 R: Reiter " + std::string(tr(Str::MfHierarchie)), [] { return elemente::finde(tr(Str::MfHierarchie), "mtbefehlsfeld", 0, false); }));
    add(pruefe(zeile("R", 0, std::string("der Reiter \"") + tr(Str::MfHierarchie) + "\" zeigt seinen Namen im Tooltip"), [] {
        const std::string t = ui::feldProbe().reiterTooltip;
        melde("R: Tooltip des Reiters \"" + t + "\", " + mbefoto::tooltipLage());
        return t == tr(Str::MfHierarchie);
    }));
    add(tu("AP14 R: Maus weg", [] { setzeMaus(ImVec2{-50.0F, -50.0F}); }));
    add(pause(0.2));
}

void zeitSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        add(bereichSetzen(modus, static_cast<int>(ModellBereich::Animation)));
        add(pause(0.6));
        auto zu = std::make_shared<float>(0.0F);
        add(tu("AP14 Z: Hoehe der Zeitleiste ohne Spuren merken, dann Spuren (Dope Sheet) auf", [zu] {
            *zu = modellZeitleisteHoehe();
            mtab().kvOffen = true;
            mtab().kvArt = 0;
            if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
        }));
        add(pause(0.8));
        add(pruefe(zeile("Z", modus, "Animation mit offenen Spuren - die Zeitleiste nimmt hoechstens den Anteil der Hoehe, den sie bei der Mindesthoehe des Entwurfs hat"),
                   [zu, modus] {
                       const float fs = ImGui::GetFontSize();
                       const float hoehe = ImGui::GetIO().DisplaySize.y;
                       const float gesamt = modellZeitleisteHoehe();
                       const float fest = *zu;                                    // alles ausser dem Spurbereich
                       const float gesetzt = std::clamp(modus == 1 ? g_app->settings.modellZeitSpurHoeheBlender : g_app->settings.modellZeitSpurHoeheMax, 2.5F, 26.0F) * fs;
                       const float luft = gesamt - fest - modellZeitSpurHoeheJetzt();   // der Abstand, der mit den Spuren dazukommt
                       // Anteil bei der Mindesthoehe des Entwurfs; unter ihr bleibt der Anteil, mindestens 2,5 Zeichenhoehen Spuren
                       const float bezug = kEntwurfHoehe * fs;
                       const float anteilEntwurf = (fest + luft + gesetzt) / bezug;
                       const float erlaubt = hoehe >= bezug ? fest + luft + gesetzt : std::max(anteilEntwurf * hoehe, fest + luft + 2.5F * fs);
                       char z[260];
                       std::snprintf(z, sizeof z, "Z: Anzeige %.0f hoch (%.1f Zeichenhoehen), Zeitleiste %.0f = %.1f %% (ohne Spuren %.0f), Spuren %.0f (gesetzt %.0f), erlaubt %.0f = %.1f %%",
                                     static_cast<double>(hoehe), static_cast<double>(hoehe / fs), static_cast<double>(gesamt), static_cast<double>(100.0F * gesamt / hoehe),
                                     static_cast<double>(fest), static_cast<double>(modellZeitSpurHoeheJetzt()), static_cast<double>(gesetzt), static_cast<double>(erlaubt),
                                     static_cast<double>(100.0F * erlaubt / hoehe));
                       melde(z);
                       return gesamt <= erlaubt + 1.5F;
                   }));
        // "Felder zeigen nur ihren Kopf" (AP10, ENT2): das rechte Feld muss unter seinen Reitern mehr zeigen als die Ueberschrift
        // seiner ersten Gruppe - verlangt sind ihr Kopf und drei Textzeilen darunter (vor der Behebung: 0,4 bis 1,4 Zeilen). Wie
        // Blender ist das Feld von vornherein niedriger (der Outliner steht darueber, AP4): dort wird nur gemessen. Deutsch fehlte
        // dafuer 1 Punkt, solange die Reiter als Text in drei Reihen standen - seit Entscheidung B6 stehen sie bei Mindestgroesse
        // als Symbole in einer Reihe (Pruefung R).
        add(pruefe(zeile("Z", modus, "Animation mit offenen Spuren - das rechte Feld zeigt den Kopf seiner ersten Gruppe und mindestens drei Zeilen darunter"), [modus] {
            const ui::RolloutProbe* g = feldGruppe(Str::AzRAnimation);
            const ImGuiWindow* w = kindFenster("mtrollouts");
            if (g == nullptr || w == nullptr) { return false; }
            const float sichtbar = w->InnerClipRect.Max.y - g->kopf.Min.y;
            const float verlangt = g->kopf.GetHeight() + 3.0F * ImGui::GetTextLineHeightWithSpacing();
            char z[220];
            std::snprintf(z, sizeof z, "Z: Gruppe %.0f..%.0f, Feld sichtbar %.0f..%.0f (%.1f Zeichenhoehen), ab dem Kopf der Gruppe %.0f, verlangt %.0f", static_cast<double>(g->kasten.Min.y),
                          static_cast<double>(g->kasten.Max.y), static_cast<double>(w->InnerClipRect.Min.y), static_cast<double>(w->InnerClipRect.Max.y),
                          static_cast<double>(w->InnerClipRect.GetHeight() / ImGui::GetFontSize()), static_cast<double>(sichtbar), static_cast<double>(verlangt));
            melde(z);
            return modus == 1 || sichtbar >= verlangt;
        }));
        add(tu("AP14 Z: Spuren zu", [] { mtab().kvOffen = false; }));
        if (modus == 0) { reiterSchritte(s); }
        add(pause(0.3));
    }
}

// --- S: gleiche Namen in "Alle Aktionen suchen" -----------------------------------------------------------------------------

// Texte, die in der offenen Suche mehrfach stehen, ohne dass ein gezeigter Zusatz die Zeilen unterscheidet ("" = keine)
std::string ununterscheidbar(int* zeilen) {
    std::map<std::string, std::vector<ImGuiID>> nach;
    int n = 0;
    for (const Element& e : g_elemente) {
        if (e.label.empty() || e.fenster.rfind("##Popup_", 0) != 0 || e.label.rfind("##", 0) == 0) { continue; }
        nach[elemente::sichtbarerText(e.label.c_str())].push_back(e.id);
        ++n;
    }
    if (zeilen != nullptr) { *zeilen = n; }
    std::string aus;
    for (const auto& [text, ids] : nach) {
        if (ids.size() < 2U) { continue; }
        std::set<std::string> zusaetze;
        bool gleich = false;
        for (const ImGuiID id : ids) {
            std::string zusatz;
            for (const ModellAktionProbe& p : modellAktionProben()) {
                if (p.id == id) { zusatz = p.zusatz; }
            }
            gleich = gleich || !zusaetze.insert(zusatz).second;
        }
        if (gleich) { aus += (aus.empty() ? "" : ", ") + text + " x" + std::to_string(ids.size()); }
    }
    return aus;
}

void sucheSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        add(bereichSetzen(modus, static_cast<int>(ModellBereich::Weights)));
        add(pause(0.5));
        add(klickAn("AP14 S: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
        add(pause(0.3));
        add(tu("AP14 S: Alle Aktionen suchen", [] { mtab().aktionenOffen = true; }));
        add(pause(0.6));
        add(pruefe(zeile("S", modus, "die ganze Liste - keine zwei Zeilen mit demselben Text ohne unterscheidenden Zusatz"), [] {
            int n = 0;
            const std::string doppelt = ununterscheidbar(&n);
            melde("S: " + std::to_string(n) + " Zeilen" + (doppelt.empty() ? "" : ", gleich: " + doppelt));
            return mtab().aktionenSichtbar && n > 20 && doppelt.empty();
        }));
        add(tippe(tr(Str::MgVerwischen)));
        add(pause(0.5));
        add(pruefe(zeile("S", modus, std::string("nach \"") + tr(Str::MgVerwischen) + "\" - die Zeilen von Weights und Painting nennen ihren Bereich"), [] {
            int n = 0;
            const std::string doppelt = ununterscheidbar(&n);
            int mitZusatz = 0;
            std::string zusaetze;
            for (const ModellAktionProbe& p : modellAktionProben()) {
                if (p.text == tr(Str::MgVerwischen) && !p.zusatz.empty()) {
                    ++mitZusatz;
                    zusaetze += " [" + p.zusatz + "]";
                }
            }
            melde("S: " + std::to_string(n) + " Zeilen nach dem Tippen, Zusaetze" + zusaetze + (doppelt.empty() ? "" : ", gleich: " + doppelt));
            return mtab().aktionenSichtbar && n >= 2 && doppelt.empty() && mitZusatz >= 2;
        }));
        add(tasteKombi(ImGuiKey_Escape, "Esc"));
        add(mbefoto::popupsZu());
        add(pause(0.3));
    }
}

// --- H: der Helfer fuehrt zu einem Element des rechten Felds ---------------------------------------------------------------

// Folgt der Tour, die "Zeig es mir" des Helfers gestartet hat: solange sie einen Oeffner zeigt (Modusleiste, Feld der
// Bereichsreihe, Ueberschrift einer zugeklappten Gruppe), wird dessen Mitte geklickt - wie ein Mensch, der der Hervorhebung
// folgt. Fertig, wenn die Tour das Ziel selbst zeigt oder nach 8 Klicks. `klicks` zaehlt die Klicks.
Schritt tourFolgen(const std::string& was, std::shared_ptr<int> klicks) {
    auto phase = std::make_shared<int>(0);
    auto ab = std::make_shared<int>(0);
    return {"AP14 H: " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    *phase = 0;
                    *ab = 0;
                    *klicks = 0;
                }
                if (*phase == 0) {   // warten, bis die Hervorhebung steht
                    const TourSicht t = tourSicht();
                    if (t.ziele.empty() || !t.overlay || t.hinweisKommt) { return b - *ab > 600; }
                    if (!t.zeigtOeffner || *klicks >= 8) { return true; }
                    {
                        // was gezeigt wird und was dort liegt (Protokoll je Klick)
                        const ImVec2 m = t.ziele[0].GetCenter();
                        std::string unter = "-";
                        for (const Element& e : elemente::alle()) {
                            if (e.rect.Contains(m) && !e.label.empty()) { unter = e.label + " in " + e.innen; }
                        }
                        const Element* v = elemente::finde(tr(Str::MgVier), "mtrollouts", 0, false);
                        std::string offen;
                        for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
                            offen += " " + ui::rolloutProbe(i).titel + (ui::rolloutProbe(i).offen ? "+" : "-");
                        }
                        char z[200];
                        std::snprintf(z, sizeof z, "H: Klick %d auf %.0f/%.0f (Stufe %d, %zu Ziele), Schalter im Register %s", *klicks + 1, static_cast<double>(m.x),
                                      static_cast<double>(m.y), t.stufe, t.ziele.size(), v != nullptr ? "ja" : "nein");
                        melde(std::string(z) + ", darunter: " + unter + ", Gruppen:" + offen);
                    }
                    setzeMaus(t.ziele[0].GetCenter());
                    *phase = 1;
                    *ab = b;
                    return false;
                }
                const int d = b - *ab;
                if (d == 2) { io.AddMouseButtonEvent(0, true); }
                if (d == 3) { io.AddMouseButtonEvent(0, false); }
                if (d == 4) { setzeMaus(ImVec2{-50.0F, -50.0F}); }
                if (d >= 40) {   // die Tour schaltet mit einer kurzen Animation weiter
                    ++*klicks;
                    *phase = 0;
                    *ab = b;
                }
                return false;
            }};
}

void hilfeSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string id = "ort:knopf:MgVier@mtrollouts";
    for (int modus = 0; modus < 2; ++modus) {
        // die Gruppe mit dem Schalter zu, dann in den Tab Script: die Tour muss Tab, Bereich und Gruppe selbst oeffnen
        add(bereichSetzen(modus, static_cast<int>(ModellBereich::Weights)));
        add(pause(0.6));
        add(tu("AP14 H: die Gruppen mit dem Schalter zu (Erweitert wie 3ds Max, Optionen wie Blender), Tab Script, Bereich Ansehen/Pruefen", [] {
            if (ImGuiWindow* w = kindFenster("mtrollouts")) {
                for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
                    const std::string& tt = ui::rolloutProbe(i).titel;
                    if (tt == tr(Str::MgRErweitert) || tt == tr(Str::MgROptionen)) { w->StateStorage.SetInt(ui::rolloutProbe(i).id, 0); }
                }
                ImGui::SetScrollY(w, 0.0F);
            }
            mtab().bereich = ModellBereich::Ansehen;
            g_app->leftMode = 1;
            tourSchliessen();
        }));
        add(pause(0.6));
        add(pruefe(zeile("H", modus, std::string("der Helfer findet \"") + tr(Str::MgVier) + "\" als Ort im rechten Feld"), [id] {
            std::string ids;
            bool da = false;
            for (const HilfeTreffer& t : hilfeFrage(tr(Str::MgVier), 12)) {
                da = da || t.id == id;
                ids += " " + t.id;
            }
            melde("H: Treffer" + ids);
            return da;
        }));
        add(tu("AP14 H: Zeig es mir", [id] { (void)hilfeZeige(id); }));
        auto klicks = std::make_shared<int>(0);
        add(tourFolgen("der Hervorhebung folgen", klicks));
        add(pause(0.3));
        add(pruefe(zeile("H", modus, "die Tour fuehrt ueber Model-Tab, Bereich Weights und die Gruppe zum Schalter und hebt ihn hervor"), [klicks] {
            const TourSicht t = tourSicht();
            const Element* e = imRoll(tr(Str::MgVier));
            char z[240];
            std::snprintf(z, sizeof z, "H: %d Klicks, Tab %d, Bereich %d, Ziel %s, Oeffner %d, Schalter %s", *klicks, g_app->leftMode, static_cast<int>(mtab().bereich),
                          t.ziele.empty() ? "-" : "da", t.zeigtOeffner ? 1 : 0, e != nullptr ? "im Feld" : "nicht im Feld");
            melde(z);
            return *klicks >= 3 && g_app->leftMode == 2 && mtab().bereich == ModellBereich::Weights && !t.zeigtOeffner && !t.ziele.empty() && e != nullptr &&
                   t.ziele[0].Contains(e->rect.GetCenter());
        }));
        // zurueck in den Model-Tab - auch wenn die Tour dort nicht hinfuehrte, laufen die Pruefungen danach im richtigen Tab
        add(tu("AP14 H: Tour zu, Model-Tab", [] {
            tourSchliessen();
            g_app->leftMode = 2;
        }));
        add(pause(0.3));
    }
}

// --- U: die Wuensche an den Baustein ---------------------------------------------------------------------------------------

void wunschSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // U1 Hauptknopf als Umschalter - wie 3ds Max "Gewichte malen" in der Gruppe Gewichts-Werkzeug
    add(bereichSetzen(0, static_cast<int>(ModellBereich::Weights)));
    add(pause(0.6));
    auto hoeheAus = std::make_shared<float>(0.0F);
    auto hauptAus = std::make_shared<int>(-1);
    add(tu("AP14 U1: Malen aus", [] { modellGewichteMalenSetzen(false); }));
    add(pause(0.4));
    add(tu("AP14 U1: Knopf \"Gewichte malen\" aus messen", [=] {
        const Element* e = imRoll(tr(Str::MgMalenMax));
        const ui::RolloutProbe* g = feldGruppe(Str::MgRWerkzeug);
        *hoeheAus = e != nullptr ? e->rect.GetHeight() : 0.0F;
        *hauptAus = g != nullptr ? g->hauptKnoepfe : -1;
    }));
    add(tu("AP14 U1: Malen an", [] { modellGewichteMalenSetzen(true); }));
    add(pause(0.4));
    add(pruefe(zeile("U1", 0, "\"Gewichte malen\" ist an und aus der Hauptknopf der Gruppe und gleich hoch (der Inhalt darunter springt nicht)"), [=] {
        const Element* e = imRoll(tr(Str::MgMalenMax));
        const ui::RolloutProbe* g = feldGruppe(Str::MgRWerkzeug);
        if (e == nullptr || g == nullptr) { return false; }
        char z[160];
        std::snprintf(z, sizeof z, "U1: aus %.0f hoch (Hauptknoepfe %d), an %.0f hoch (Hauptknoepfe %d)", static_cast<double>(*hoeheAus), *hauptAus,
                      static_cast<double>(e->rect.GetHeight()), g->hauptKnoepfe);
        melde(z);
        return *hauptAus == 1 && g->hauptKnoepfe == 1 && *hoeheAus > 0.0F && std::fabs(*hoeheAus - e->rect.GetHeight()) < 0.5F;
    }));
    // U6 Wahl ohne gewaehlten Eintrag: die Festwerte des Weight Tools, wenn das Gewicht keiner von ihnen ist
    add(tu("AP14 U6: Gewicht 0,333 (kein Festwert)", [] { mtab().pinselGewicht = 0.333F; }));
    add(pause(0.4));
    add(pruefe(zeile("U6", 0, "die Reihe der Festwerte 0 .. 1 betont keinen Eintrag, wenn das Gewicht kein Festwert ist"), [] {
        int da = 0;
        int betont = 0;
        for (const char* t : {"0", ".1", ".25", ".5", ".75", ".9", "1"}) {
            if (const Element* e = imRoll(t)) {
                ++da;
                betont += (e->flags & ImGuiItemStatusFlags_Checked) != 0 ? 1 : 0;
            }
        }
        melde("U6: " + std::to_string(da) + " Festwerte, betont " + std::to_string(betont));
        return da == 7 && betont == 0;
    }));
    // U3 ui::absatz im Element-Register: ohne gewaehlten Knochen steht die Warnzeile ueber dem Hauptknopf
    add(tu("AP14 U3: kein Knochen gewaehlt", [] {
        mtab().pinselGewicht = 1.0F;
        mtab().knochenWahl = -1;
    }));
    add(pause(0.4));
    add(pruefe(zeile("U3", 0, std::string("die Warnzeile \"") + tr(Str::MbGwKnochenWaehlen) + "\" (ui::absatz) steht mit ihrem Text im Element-Register"), [] {
        const Element* e = imRoll(tr(Str::MbGwKnochenWaehlen));
        const ui::RolloutProbe* g = feldGruppe(Str::MgRWerkzeug);
        if (e != nullptr && g != nullptr) {
            char z[160];
            std::snprintf(z, sizeof z, "U3: Zeile %.0f/%.0f - %.0f/%.0f", static_cast<double>(e->rect.Min.x), static_cast<double>(e->rect.Min.y), static_cast<double>(e->rect.Max.x),
                          static_cast<double>(e->rect.Max.y));
            melde(z);
        }
        return e != nullptr && g != nullptr && e->rect.GetHeight() >= ImGui::GetFontSize() && g->kasten.Contains(e->rect);
    }));
    // U2 / U4 gesperrt: wie Blender der fest angehakte Schalter "Hoechstens 4 je Ecke (JKA)" in den Optionen
    add(bereichSetzen(1, static_cast<int>(ModellBereich::Weights)));
    add(pause(0.6));
    alleAuf(s);
    add(insFeld(tr(Str::MgVier)));
    add(pause(0.4));
    add(pruefe(zeile("U2", 1, "der feste Schalter \"Hoechstens 4 je Ecke\" traegt im Element-Register die Fahne gesperrt, der Schalter Auto-Normalisieren nicht"), [] {
        const Element* vier = imRoll(tr(Str::MgVier));
        const Element* norm = imRoll(tr(Str::MgAutoNorm));
        melde(std::string("U2: Hoechstens 4 ") + (vier == nullptr ? "fehlt" : vier->gesperrt ? "gesperrt" : "frei") + ", Auto-Normalisieren " +
              (norm == nullptr ? "fehlt" : norm->gesperrt ? "gesperrt" : "frei"));
        return vier != nullptr && norm != nullptr && vier->gesperrt && !norm->gesperrt;
    }));
    add(mbefoto::mausAuf("AP14 U4 Hoechstens 4 je Ecke", [] { return imRoll(tr(Str::MgVier)); }));
    add(pruefe(zeile("U4", 1, "der gesperrte Schalter zeigt seinen Tooltip"), [] {
        const std::string lage = mbefoto::tooltipLage();
        melde("U4: " + lage);
        return lage != "kein Tooltip";
    }));
    add(tu("AP14 U: Maus weg", [] { setzeMaus(ImVec2{-50.0F, -50.0F}); }));
    add(pause(0.3));
}

// --- Fotos ----------------------------------------------------------------------------------------------------------------

void fotoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const std::string p = modus == 0 ? "ap14_max_" : "ap14_blender_";
        add(bereichSetzen(modus, static_cast<int>(ModellBereich::Animation)));
        add(pause(0.8));
        add(foto(p + "anim_spuren_zu"));
        add(tu("AP14 Foto: Spuren auf", [] {
            mtab().kvOffen = true;
            mtab().kvArt = 0;
        }));
        add(pause(1.0));
        add(foto(p + "anim_spuren_auf"));
        add(tu("AP14 Foto: Spuren zu", [] { mtab().kvOffen = false; }));
        add(bereichSetzen(modus, static_cast<int>(ModellBereich::Ansehen)));
        add(pause(0.8));
        add(foto(p + "pruefen"));
        add(bereichSetzen(modus, static_cast<int>(ModellBereich::Weights)));
        add(pause(0.6));
        alleAuf(s);
        add(insFeld(tr(Str::MgToleranz)));
        add(tu("AP14 Foto: die Tabelle der Gruppe Spiegeln ins Bild", [] {
            if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, std::min(w->Scroll.y + ImGui::GetFontSize() * 6.0F, w->ScrollMax.y)); }
        }));
        add(pause(0.6));
        add(foto(p + "weights_spiegeln"));
        add(tu("AP14 Foto: Malen aus (Hauptknopf als Umschalter)", [] {
            modellGewichteMalenSetzen(false);
            if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
        }));
        add(pause(0.6));
        add(foto(p + "weights_malen_aus"));
        add(tu("AP14 Foto: Malen an", [] { modellGewichteMalenSetzen(true); }));
        add(pause(0.6));
        add(foto(p + "weights_malen_an"));
        add(insFeld(tr(Str::MgVier)));
        add(pause(0.3));
        add(mbefoto::mausAuf("AP14 Foto Hoechstens 4 je Ecke", [] { return imRoll(tr(Str::MgVier)); }));
        add(foto(p + "weights_gesperrt_tooltip"));
        add(tu("AP14 Foto: Maus weg", [] { setzeMaus(ImVec2{-50.0F, -50.0F}); }));
        add(klickAn("AP14 Foto: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
        add(pause(0.3));
        add(tu("AP14 Foto: Alle Aktionen suchen", [] { mtab().aktionenOffen = true; }));
        add(pause(0.6));
        add(tippe(tr(Str::MgVerwischen)));
        add(pause(0.6));
        add(foto(p + "suche"));
        add(tasteKombi(ImGuiKey_Escape, "Esc"));
        add(mbefoto::popupsZu());
        add(pause(0.3));
    }
}

// Bilder fuer das Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap14): h_ap12_max, h_ap12_blender - die Gruppe "Automatisch
// (Auto-Skin)" im rechten Feld des Bereichs Weights nach einem Lauf an kyle (Seite modell-autoskin, tools/handbuch/inhalt_ap12.py)
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto gelaufen = std::make_shared<bool>(false);
    add(tu("AP14 Handbuch: Weights wie 3ds Max, Knochen lhumerus", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        g_app->settings.splitModel = 0.85F;
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Weights;
        mtab().vierAnsichten = false;
        mtab().knochenWahl = boneIndex(g_app->anim, "lhumerus");
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
        modellAutoSkinZustand() = ModellAutoSkin{};
        setzeMaus(ImVec2{-50.0F, -50.0F});
    }));
    add(pause(1.0));
    add(tu("AP14 Handbuch: rechnen (Vorgabe der Gruppe)", [=] { *gelaufen = modellAutoSkin(); }));
    add(pause(1.0));
    // die uebrigen Gruppen zu, damit die Gruppe mit Einstellungen, Hauptknopf und Ergebnis ins Feld passt
    const auto nurAutoSkin = [] {
        return tu("AP14 Handbuch: die uebrigen Gruppen des Felds zuklappen", [] {
            ImGuiWindow* w = kindFenster("mtrollouts");
            if (w == nullptr) { return; }
            const std::string bleibt = tr(Str::AsGruppe);
            for (const Element& e : g_elemente) {
                if (e.innen.find("mtrollouts") == std::string::npos || (e.flags & ImGuiItemStatusFlags_Openable) == 0) { continue; }
                // nur die Ueberschriften der Gruppen (ui::rolloutProbe), nicht die eingeklappten Teile in der Gruppe Auto-Skin
                bool gruppe = false;
                for (int i = 0; i < ui::rolloutProbeZahl(); ++i) { gruppe = gruppe || ui::rolloutProbe(i).id == e.id; }
                if (gruppe) { w->StateStorage.SetInt(e.id, elemente::sichtbarerText(e.label.c_str()) == bleibt ? 1 : 0); }
            }
            ImGui::SetScrollY(w, 0.0F);
        });
    };
    add(nurAutoSkin());
    add(pause(0.8));
    add(pruefe("AP14 Handbuch: der Lauf ist gelungen und die Gruppe zeigt ihr Ergebnis",
               [=] { return *gelaufen && modellAutoSkinZustand().gelaufen && imRoll(tr(Str::AsAusfuehren)) != nullptr; }));
    add(hilfe2::schrittFoto("h_ap12_max"));
    add(tu("AP14 Handbuch: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(1.0));
    add(nurAutoSkin());
    add(pause(0.8));
    // wie Blender ist das Feld niedriger (Outliner darueber): die Gruppe an den oberen Rand rollen, damit Einstellungen,
    // Hauptknopf und Ergebnis im Bild stehen statt der zugeklappten Gruppen darueber
    add(tu("AP14 Handbuch: die Gruppe Auto-Skin an den oberen Rand des Felds rollen", [] {
        ImGuiWindow* w = kindFenster("mtrollouts");
        if (w == nullptr) { return; }
        const std::string titel = tr(Str::AsGruppe);
        for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
            const ui::RolloutProbe& p = ui::rolloutProbe(i);
            if (p.titel == titel) { ImGui::SetScrollY(w, w->Scroll.y + p.kopf.Min.y - w->InnerClipRect.Min.y); }
        }
    }));
    add(pause(0.8));
    add(pruefe("AP14 Handbuch: wie Blender steht der Hauptknopf der Gruppe Auto-Skin ganz im Feld", [] {
        const Element* e = imRoll(tr(Str::AsAusfuehren));
        const ImGuiWindow* w = kindFenster("mtrollouts");
        return e != nullptr && w != nullptr && e->rect.Min.y >= w->InnerClipRect.Min.y && e->rect.Max.y <= w->InnerClipRect.Max.y;
    }));
    add(hilfe2::schrittFoto("h_ap12_blender"));
    add(tu("AP14 Handbuch: zurueck", [=] {
        (void)modellRueckgaengig();
        modellAutoSkinZustand() = ModellAutoSkin{};
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        mtab().bereich = ModellBereich::Ansehen;
        mtab().knochenWahl = -1;
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
    if (n != "ap14" && n != "ap14foto" && n != "ap14h") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 1350;
    int h = 900;
    float z = 1.0F;
    if (const char* f = std::getenv("BHED_AP14_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    auto skala = std::make_shared<float>(1.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    auto spurMax = std::make_shared<float>(0.0F);
    auto spurBlender = std::make_shared<float>(0.0F);
    add(tu("AP14: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        if (const char* t = std::getenv("BHED_AP14_THEMA")) {
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
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap14.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    add(tu("AP14: Aufteilung und Zeitleiste wie neue Einstellungen, wie 3ds Max, Bereich Ansehen/Pruefen", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        *spurMax = g_app->settings.modellZeitSpurHoeheMax;
        *spurBlender = g_app->settings.modellZeitSpurHoeheBlender;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellZeitSpurHoeheMax = Settings{}.modellZeitSpurHoeheMax;
        g_app->settings.modellZeitSpurHoeheBlender = Settings{}.modellZeitSpurHoeheBlender;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        modellExplorerVorgabe();
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
        char zl[300];
        std::snprintf(zl, sizeof zl, "%s, %zu Knochen, Anzeige %.0f x %.0f, Oberflaeche %.2f, Schrift %.0f Punkte hoch, Sprache %s", g_app->modelPath.c_str(), g_app->anim.bones.size(),
                      static_cast<double>(ImGui::GetIO().DisplaySize.x), static_cast<double>(ImGui::GetIO().DisplaySize.y), static_cast<double>(g_app->uiScale),
                      static_cast<double>(ImGui::GetFontSize()), i18n::kurzCode(i18n::currentLanguage()));
        melde(zl);
    }));
    add(pause(1.0));
    add(klickAn("AP14: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (n == "ap14h") {   // nur der Helfer (H) - zum gezielten Wiederholen
        hilfeSchritte(s);
    } else if (n == "ap14") {
        wortSchritte(s);
        zeitSchritte(s);
        sucheSchritte(s);
        hilfeSchritte(s);
        wunschSchritte(s);
    } else {
        fotoSchritte(s);
    }
    add(tu("AP14: Modell-Tab zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->settings.modellZeitSpurHoeheMax = *spurMax;
        g_app->settings.modellZeitSpurHoeheBlender = *spurBlender;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        g_app->mtab.kvOffen = false;
        g_app->mtab.knochenWahl = -1;
        modellGewichteMalenSetzen(true);
    }));
    add(tu("AP14: zurueck", [=] {
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

}  // namespace ap14
