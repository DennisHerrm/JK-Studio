// selbsttest_ap4.inc - AP4 (MBENTWURF Teil B, 12.11.): der Model-Tab im Bedienmodus wie Blender nach dem freigegebenen Entwurf
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap4 | ap4foto)
//
//   ap4       je Zeile der Abgleichtabelle (plaene/modellblock_stufe0/abgleich_programm_entwurf.md: R16-R18, X2, X15, X28, F2, F17,
//             P0b, P8, M0b, U0b, W0b, T0b, A0b, A11) eine Pruefung "AP4 <Zeile>": Ort (Werkzeugleiste links, Outliner oben rechts,
//             Eigenschaften darunter, Seitenleiste rechts in der Ansicht), Element (Knoepfe der Leiste je Bereich, Modus im Kopf,
//             Hinweis N/T) und Bedienung (Klick auf ein Werkzeug und seine Wirkung, T, N, Messen, Knochen waehlen, Foto, Umschalten
//             ohne Verlust, Anordnung je Bedienmodus) - dazu, dass der Modus wie 3ds Max unveraendert bleibt.
//   ap4foto   nur Fotos (BHED_FOTOS=<Ordner>): je Bedienmodus jeder Bereich, dazu Werkzeugleiste aus und Seitenleiste an.
//   BHED_AP4_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe wie der Foto-Selbsttest mbefoto)
//   BHED_AP4_MODELL=<.glm>           Vorgabe models/players/kyle/model.glm (das Modell des Entwurfs)
//   BHED_AP4_THEMA=hell|dunkel       das erste helle bzw. dunkle Farbschema (Vorgabe: das eingestellte)
// Eingebunden in selbsttest.cpp (nach selbsttest_ap10.inc).
namespace ap4 {

using ap3::tooltipDa;
using mbefoto::inFenster;
using mbefoto::kindFenster;

float em() { return ImGui::GetFontSize(); }

void melde(const std::string& z) { diag::detail("Knopftest: AP4 " + z); }

std::string zeile(const char* nr, int modus, const std::string& was) { return std::string("AP4 ") + nr + (modus == 0 ? " wie 3ds Max: " : " wie Blender: ") + was; }

ModellTab& mtab() { return g_app->mtab; }

// Das Rechteck eines Kindfensters (leer: in diesem Bild nicht gezeichnet).
ImRect fenster(const char* teil) {
    const ImGuiWindow* w = kindFenster(teil);
    if (w == nullptr || !w->Active) { return ImRect{}; }
    return ImRect{w->Pos, ImVec2{w->Pos.x + w->Size.x, w->Pos.y + w->Size.y}};
}

bool leer(const ImRect& r) { return r.GetWidth() <= 0.0F || r.GetHeight() <= 0.0F; }

std::string rechteck(const char* name, const ImRect& r) { return mbefoto::rechteck(name, r); }

const Element* inLeiste(const std::string& label) { return inFenster(label, "mtwerkzeugleiste"); }

// Ein Knopf der Werkzeugleiste (Mitte; weit draussen, wenn es ihn nicht gibt - der Klick geht dann ins Leere)
std::function<ImVec2()> knopfOrt(const std::string& label) {
    return [label] {
        const Element* e = inLeiste(label);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    };
}

Schritt klickLeiste(const std::string& label) { return klickAn("AP4 Werkzeugleiste: " + label, knopfOrt(label)); }

int flaecheNach(const char* name) {
    for (std::size_t i = 0; i < g_app->model.surfaces.size(); ++i) {
        if (g_app->model.surfaces[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

int knochenNach(const char* name) {
    for (std::size_t i = 0; i < g_app->anim.bones.size(); ++i) {
        if (g_app->anim.bones[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

const ModellAnsicht& aktiv() { return mtab().ansichten[static_cast<std::size_t>(std::clamp(mtab().aktiv, 0, 3))]; }

bool gelenkAufSchirm(int b, ImVec2& out) {
    if (b < 0 || static_cast<std::size_t>(b) >= g_app->modellWelt.size()) { return false; }
    const BoneMatrix& m = g_app->modellWelt[static_cast<std::size_t>(b)];
    const float p[3] = {m.m[0][3], m.m[1][3], m.m[2][3]};
    return modellAufSchirm(aktiv(), p, out);
}

// Die Texte der Knoepfe der Leiste in ihrer Reihenfolge, mit | getrennt (aus der Probe des letzten Bilds).
std::string leistenTexte() {
    std::string s;
    for (const auto& k : modellBl().knoepfe) { s += (s.empty() ? "" : "|") + k.first; }
    return s;
}

std::string folge(std::initializer_list<std::string> liste) {
    std::string s;
    for (const std::string& t : liste) { s += (s.empty() ? "" : "|") + t; }
    return s;
}

// Die Knoepfe stehen senkrecht untereinander, gleich gross, in der Leiste, und die Leiste links neben den Ansichten.
bool leisteSenkrecht() {
    const ModellBlender& b = modellBl();
    const ImRect l = fenster("mtwerkzeugleiste");
    const ImRect v = fenster("mtansichten");
    if (leer(l) || leer(v) || b.knoepfe.size() < 4) { return false; }
    bool ok = l.Max.x <= v.Min.x + 0.5F && l.GetWidth() <= em() * 4.0F;
    for (std::size_t i = 0; i < b.knoepfe.size(); ++i) {
        const ImRect& r = b.knoepfe[i].second;
        ok = ok && l.Contains(r) && std::fabs(r.GetWidth() - b.knoepfe[0].second.GetWidth()) < 0.5F && std::fabs(r.GetWidth() - r.GetHeight()) < 0.5F;
        if (i > 0) { ok = ok && r.Min.y >= b.knoepfe[i - 1].second.Max.y - 0.5F && std::fabs(r.Min.x - b.knoepfe[0].second.Min.x) < 0.5F; }
    }
    return ok && b.trennstrich == 4;
}

// --- Rahmen: Max unveraendert, Blender mit Leiste links, Outliner oben rechts, Eigenschaften darunter -----------------------

void rahmenSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("AP4: wie 3ds Max, Bereich Ansehen/Pruefen", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Ansehen;
    }));
    add(pause(0.8));
    add(pruefe(zeile("R16", 0, "unveraendert: links der Szenen-Explorer, keine Werkzeugleiste, rechts nur das Befehlsfeld (oben buendig mit den Ansichten), kein Modus im Kopf, kein Hinweis N/T"), [] {
        const ImRect x = fenster("mtexplorer");
        const ImRect v = fenster("mtansichten");
        const ImRect f = fenster("mtbefehlsfeld");
        const ModellBlender& b = modellBl();
        melde("R16 Max: " + rechteck("explorer", x) + " " + rechteck("ansichten", v) + " " + rechteck("feld", f));
        return !leer(x) && !leer(v) && !leer(f) && x.Max.x <= v.Min.x && kindFenster("mtwerkzeugleiste") == nullptr && std::fabs(f.Min.y - v.Min.y) <= 2.0F &&
               b.modusText.empty() && leer(b.hinweis) && std::string(modellLinksTitel()) == tr(Str::MbExplorer);
    }));
    add(tu("AP4: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(1.0));
    add(pruefe(zeile("R16", 1, "Aufbau wie Blender: links die Werkzeugleiste (schmal, links neben den Ansichten), rechts oben der Outliner, darunter der Eigenschaften-Editor in derselben Spalte"), [] {
        const ImRect l = fenster("mtwerkzeugleiste");
        const ImRect x = fenster("mtexplorer");
        const ImRect v = fenster("mtansichten");
        const ImRect f = fenster("mtbefehlsfeld");
        melde("R16 Blender: " + rechteck("leiste", l) + " " + rechteck("explorer", x) + " " + rechteck("ansichten", v) + " " + rechteck("feld", f));
        return !leer(l) && !leer(x) && !leer(v) && !leer(f) && l.Max.x <= v.Min.x + 0.5F && l.GetWidth() <= em() * 4.0F && x.Min.x >= v.Max.x &&
               std::fabs(x.Min.x - f.Min.x) <= 2.0F && x.Max.y <= f.Min.y + 0.5F && std::fabs(x.GetWidth() - f.GetWidth()) <= 2.0F;
    }));
    add(pruefe(zeile("X2", 1, "Outliner oben rechts mit dem Titel \"Outliner\", gut 40 % der Spaltenhoehe (Entwurf 44 %), oben buendig mit den Ansichten"), [] {
        const ModellBlender& b = modellBl();
        const ImRect v = fenster("mtansichten");
        const float hoehe = v.GetHeight();
        const float anteil = hoehe > 0.0F ? b.outliner.GetHeight() / hoehe : 0.0F;
        char z[96];
        std::snprintf(z, sizeof z, "X2 Outliner: Anteil %.2f, Titel %s", static_cast<double>(anteil), modellExplorerTitel());
        melde(std::string(z) + " " + rechteck("outliner", b.outliner));
        return std::string(modellExplorerTitel()) == tr(Str::MbXOutliner) && anteil > 0.38F && anteil < 0.52F && std::fabs(b.outliner.Min.y - v.Min.y) <= 2.0F;
    }));
    add(pruefe(zeile("X15", 1, "Outliner: die Umschalter (Auge) stehen rechts in einer Spalte, der Baum links (Baustein von AP2, hier am Ort oben rechts)"), [] {
        const ImRect x = fenster("mtexplorer");
        const float auge = mbefoto::augeX();
        melde("X15 Auge bei x " + std::to_string(static_cast<int>(auge)) + " " + rechteck("outliner", x));
        return !leer(x) && auge > x.Min.x + x.GetWidth() * 0.6F && auge < x.Max.x;
    }));
    add(pruefe(zeile("X28", 1, "der Outliner ist derselbe Baum wie der Szenen-Explorer (Hierarchie / Sammlungen), nur am Ort wie Blender"), [] {
        return kindFenster("mtbaum") != nullptr && fenster("mtexplorer").Min.x > fenster("mtansichten").Max.x;
    }));
    add(pruefe(zeile("F2", 1, "Eigenschaften-Editor: Titel \"Properties\", senkrechte Reiterspalte, Stil wie Blender (Baustein von AP3)"), [] {
        const ui::FeldProbe& f = ui::feldProbe();
        return ap3::feldFrisch() && f.stil == ui::RolloutStil::Blender && f.reiterSenkrecht && f.reiterZahl == static_cast<int>(MbReiterBl::Zahl);
    }));
    add(pruefe(zeile("F17", 1, "rechtes Feld wie Blender: der Outliner darueber, die Eigenschaften darunter; die Trennkante liegt dazwischen"), [] {
        const ModellBlender& b = modellBl();
        const ImRect f = fenster("mtbefehlsfeld");
        return !leer(b.outliner) && !leer(f) && b.outliner.Max.y <= f.Min.y && f.Min.y - b.outliner.Max.y <= em() * 1.2F && ui::feldProbe().titel.Min.y >= f.Min.y;
    }));
    // Trennkante ziehen
    add(tu("AP4: Anteil des Outliners merken", [] { modellBl().outlinerAnteil = 0.44F; }));
    add(pause(0.3));
    add(ziehenAn("AP4: Trennkante Outliner/Eigenschaften nach unten", [] {
                     const ModellBlender& b = modellBl();
                     return ImVec2{b.outliner.GetCenter().x, b.outliner.Max.y + em() * 0.25F};
                 },
                 0.0F, 120.0F, 0));
    add(pause(0.4));
    add(pruefe(zeile("F17", 1, "die Trennkante zwischen Outliner und Eigenschaften laesst sich ziehen"), [] {
        const float a = modellBl().outlinerAnteil;
        melde("F17 Anteil nach dem Ziehen " + std::to_string(a));
        modellBl().outlinerAnteil = 0.44F;
        return a > 0.46F;
    }));
    add(pause(0.3));
}

// --- Werkzeugleiste und Kopf je Bereich ----------------------------------------------------------------------------------

struct BereichSoll {
    ModellBereich b;
    const char* zeile;   // Abgleichtabelle
    std::vector<Str> werkzeuge;
    Str modus;
};

std::vector<BereichSoll> soll() {
    return {
        {ModellBereich::Ansehen, "P0b", {Str::BzMessen, Str::BzFlaecheWaehlen, Str::BzKnochenWaehlen, Str::MbFoto}, Str::MbBlObjektModus},
        // ENTSCHEID1: die Namen wie Blender 5.0 (space_toolsystem_toolbar.py label=...), Extrudieren und Messer zeigen das
        // gewaehlte Werkzeug ihrer Gruppe (ab Werk das erste: Extrude Region, Knife)
        {ModellBereich::Modellieren, "M0b", {Str::BeExtrude, Str::BeInset, Str::BzFase, Str::BeLoopCut, Str::BzMesser, Str::MdMerge}, Str::MbBlEditModus},
        {ModellBereich::Uv, "U0b", {Str::BeNaht, Str::UvAbwickeln, Str::BeEntspannen, Str::BePacken}, Str::MbBlEditModus},
        {ModellBereich::Weights, "W0b", {Str::MgRPinsel, Str::MgWeich, Str::BeVerlauf, Str::BeGewichtHolen}, Str::BzModusWeight},
        {ModellBereich::Painting, "T0b", {Str::MgRPinsel, Str::BeFuellen, Str::PtWzKlonen, Str::BeWischen, Str::BeMaske}, Str::BzModusTexture},
        {ModellBereich::Animation, "A0b", {Str::BeKopieren, Str::BeGespiegelt, Str::MbKvReduzieren}, Str::BzModusPose},
    };
}

void bereichSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (const BereichSoll& b : soll()) {
        const ModellBereich bereich = b.b;
        add(tu(std::string("AP4: wie Blender, Bereich ") + b.zeile, [bereich] {
            modellBedienungSetzen(1);
            mtab().bereich = bereich;
            modellBl().werkzeugleiste = true;
            modellBl().seitenleiste = false;
        }));
        add(pause(0.6));
        std::string werkzeuge;
        for (const Str w : b.werkzeuge) { werkzeuge += (werkzeuge.empty() ? "" : ", ") + std::string(tr(w)); }
        const std::vector<Str> liste = b.werkzeuge;
        add(pruefe(zeile(b.zeile, 1, "Werkzeugleiste links: Auswaehlen, Bewegen, Drehen, Skalieren, Trennstrich, " + werkzeuge + " - senkrecht, gleich gross, eigene Symbole"), [liste] {
            std::string soll = folge({tr(Str::BzAuswaehlen), tr(Str::BzBewegen), tr(Str::BzDrehen), tr(Str::BzSkalieren)});
            for (const Str w : liste) { soll += std::string("|") + tr(w); }
            melde("Leiste: " + leistenTexte() + " - soll " + soll);
            return leistenTexte() == soll && leisteSenkrecht();
        }));
        const Str modus = b.modus;
        const bool fov = bereich == ModellBereich::Ansehen;   // Entwurf P0b: "Benutzer-Perspektive - Objektmodus - FOV 80"
        add(pruefe(zeile(b.zeile, 1, std::string("Kopf der Ansicht \"") + tr(Str::MkBlUserPersp) + " \xc2\xb7 " + tr(b.modus) + (fov ? " \xc2\xb7 FOV" : "") +
                                         "\" mit dem Modus als Ausklapper"),
                   [modus, fov] {
            const ModellBlender& bl = modellBl();
            char sichtfeld[48];
            std::snprintf(sichtfeld, sizeof sichtfeld, " \xc2\xb7 FOV %.0f\xc2\xb0", static_cast<double>(g_app->modelCam.fovY));
            const std::string soll = std::string(tr(Str::MkBlUserPersp)) + " \xc2\xb7 " + tr(modus) + (fov ? sichtfeld : "");
            melde("Kopf: \"" + bl.modusText + "\" " + rechteck("modus", bl.modusFeld));
            return bl.modusText == soll && !leer(bl.modusFeld) && aktiv().lo[1] <= bl.modusFeld.Min.y && bl.modusFeld.Max.y <= aktiv().lo[1] + em() * 2.0F;
        }));
        const bool mitHinweis = bereich != ModellBereich::Ansehen;
        add(pruefe(zeile(b.zeile, 1, mitHinweis ? "unten rechts in der Ansicht der Hinweis \"N Seitenleiste \xc2\xb7 T Werkzeuge\"" : "kein Hinweis N/T (dort steht laut Entwurf die Detailstufe)"),
                   [mitHinweis] {
                       const ImRect h = modellBl().hinweis;
                       const ModellAnsicht& a = aktiv();
                       if (!mitHinweis) { return leer(h); }
                       return !leer(h) && h.Max.x <= a.lo[0] + a.groesse[0] && h.Max.x >= a.lo[0] + a.groesse[0] - em() * 1.5F && h.Max.y >= a.lo[1] + a.groesse[1] - em() * 1.5F;
                   }));
        add(pruefe(zeile(b.zeile, 1, "rechts Outliner ueber den Eigenschaften (Reiter des Bereichs wie im Entwurf)"), [bereich] {
            const ImRect x = fenster("mtexplorer");
            const ImRect f = fenster("mtbefehlsfeld");
            return !leer(x) && !leer(f) && x.Max.y <= f.Min.y + 0.5F && ui::feldProbe().reiterAktiv == static_cast<int>(modellReiterStandardBl(bereich));
        }));
    }
    // Moduswahl im Kopf: Klick auf den Modus, Eintrag Weight Paint -> Bereich Weights
    add(tu("AP4: Bereich Ansehen/Pruefen", [] { mtab().bereich = ModellBereich::Ansehen; }));
    add(pause(0.4));
    add(klickAn("AP4: Modus im Kopf der Ansicht", [] { return modellBl().modusFeld.GetCenter(); }));
    add(pause(0.4));
    add(pruefe(zeile("R16", 1, "der Modus im Kopf oeffnet die Moduswahl (Objektmodus, Bearbeitungsmodus, Weight Paint, Texture Paint, Pose-Modus)"), [] {
        int da = 0;
        for (const Str x : {Str::MbBlObjektModus, Str::MbBlEditModus, Str::BzModusWeight, Str::BzModusTexture, Str::BzModusPose}) { da += finde(tr(x), "##Popup_", 0, false) != nullptr ? 1 : 0; }
        return da == 5;
    }));
    add(klick(tr(Str::BzModusWeight), "##Popup_"));
    add(pause(0.4));
    add(pruefe(zeile("R16", 1, "Weight Paint in der Moduswahl wechselt in den Bereich Weights"), [] { return mtab().bereich == ModellBereich::Weights; }));
    add(tu("AP4: zurueck nach Ansehen/Pruefen", [] { mtab().bereich = ModellBereich::Ansehen; }));
    add(pause(0.3));
}

// --- Bedienung der Werkzeuge -------------------------------------------------------------------------------------------

void werkzeugSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // R17: die vier Werkzeuge jedes Bereichs waehlen das Gizmo; Tooltip mit Taste
    add(tu("AP4: Ansehen/Pruefen, Gizmo Auswahl", [] {
        mtab().bereich = ModellBereich::Ansehen;
        mtab().gizmo = 0;
        modellBl().werkzeug = 0;
    }));
    add(pause(0.3));
    add(klickLeiste(tr(Str::BzBewegen)));
    add(pause(0.3));
    add(pruefe(zeile("R17", 1, "Klick auf Bewegen waehlt das Gizmo Verschieben, der Knopf ist hervorgehoben"), [] { return mtab().gizmo == 1 && modellBlWerkzeugAn(1) && !modellBlWerkzeugAn(0); }));
    add(klickLeiste(tr(Str::BzSkalieren)));
    add(pause(0.3));
    add(pruefe(zeile("R17", 1, "Klick auf Skalieren waehlt das Gizmo Skalieren"), [] { return mtab().gizmo == 3; }));
    add(mbefoto::mausAuf("den Knopf Drehen", [] { return inLeiste(tr(Str::BzDrehen)); }));
    add(pruefe(zeile("R17", 1, "unter der Maus zeigt ein Werkzeug seinen Tooltip (Name, Taste R, was es tut)"), [] { return tooltipDa(); }));
    add(klickLeiste(tr(Str::BzAuswaehlen)));
    add(pause(0.3));
    // T blendet aus und ein
    add(klickAn("AP4: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.3));
    add(tu("AP4: Breite der Ansichten merken", [] { melde("R17 vor T: " + rechteck("ansichten", fenster("mtansichten"))); }));
    auto breiteVor = std::make_shared<float>(0.0F);
    add(tu("AP4: Breite merken", [breiteVor] { *breiteVor = fenster("mtansichten").GetWidth(); }));
    add(taste(ImGuiKey_T, "T"));
    add(pause(0.5));
    add(pruefe(zeile("R17", 1, "T blendet die Werkzeugleiste aus: ein schmaler Streifen mit Pfeil bleibt, die Ansichten werden breiter"), [breiteVor] {
        const ModellBlender& b = modellBl();
        const float jetzt = fenster("mtansichten").GetWidth();
        melde("R17 nach T: " + rechteck("streifen", b.leiste) + ", Ansichten " + std::to_string(static_cast<int>(*breiteVor)) + " -> " + std::to_string(static_cast<int>(jetzt)));
        return !b.werkzeugleiste && kindFenster("mtwerkzeugleiste") == nullptr && !leer(b.leiste) && b.leiste.GetWidth() <= em() * 1.6F && jetzt > *breiteVor + em();
    }));
    add(klickAn("AP4: der Pfeil im Streifen", [] { return ImVec2{modellBl().leiste.GetCenter().x, modellBl().leiste.Min.y + em() * 0.7F}; }));
    add(pause(0.5));
    add(pruefe(zeile("R17", 1, "der Pfeil im Streifen zeigt die Werkzeugleiste wieder"), [] { return modellBl().werkzeugleiste && kindFenster("mtwerkzeugleiste") != nullptr; }));
    add(klickAn("AP4: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(taste(ImGuiKey_T, "T"));
    add(pause(0.3));
    add(taste(ImGuiKey_T, "T"));
    add(pause(0.4));
    add(pruefe(zeile("R17", 1, "T zweimal: aus und wieder an"), [] { return modellBl().werkzeugleiste && kindFenster("mtwerkzeugleiste") != nullptr; }));

    // R18: N zeigt die Seitenleiste mit den Werten der Auswahl
    add(tu("AP4: Flaeche torso waehlen", [] {
        const int f = flaecheNach("torso");
        mtab().auswahl = f >= 0 ? std::vector<int>{f} : std::vector<int>{};
        mtab().knochenWahl = -1;
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(taste(ImGuiKey_N, "N"));
    add(pause(0.5));
    add(pruefe(zeile("R18", 1, "N zeigt die Seitenleiste rechts in der Ansicht (zwischen den Ansichten und den Eigenschaften), Reiter Objekt: gewaehlt torso, Position, Abmessungen"), [] {
        const ImRect sl = fenster("mtseitenleiste");
        const ImRect v = fenster("mtansichten");
        const ImRect f = fenster("mtbefehlsfeld");
        const std::string t = modellBl().seiteText;
        melde("R18 " + rechteck("seite", sl) + " Inhalt: " + t);
        return modellBl().seitenleiste && !leer(sl) && sl.Min.x >= v.Max.x && sl.Max.x <= f.Min.x && t.find(std::string(tr(Str::BzGewaehlt)) + "=torso") != std::string::npos &&
               t.find(std::string(tr(Str::BzMasse)) + " X=") != std::string::npos;
    }));
    add(klickAn("AP4: Reiter Ansicht der Seitenleiste", [] {
        const Element* e = inFenster(tr(Str::BzReiterAnsicht), "mtseitenleiste");
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    }));
    add(pause(0.4));
    add(pruefe(zeile("R18", 1, "Reiter Ansicht: Sichtfeld und Projektion"), [] {
        const std::string t = modellBl().seiteText;
        melde("R18 Ansicht: " + t);
        return modellBl().seitenReiter == 1 && t.find(std::string(tr(Str::BzSichtfeld)) + "=") != std::string::npos;
    }));
    add(klickAn("AP4: Reiter Objekt", [] {
        const Element* e = inFenster(tr(Str::BzReiterObjekt), "mtseitenleiste");
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    }));
    add(klickAn("AP4: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(taste(ImGuiKey_N, "N"));
    add(pause(0.4));
    add(pruefe(zeile("R18", 1, "N noch einmal blendet sie aus"), [] { return !modellBl().seitenleiste && kindFenster("mtseitenleiste") == nullptr; }));

    // P8: Messen (zwei Klicks und Ziehen), Knochen waehlen, Flaeche waehlen, Foto
    auto klick1 = std::make_shared<ImVec2>();
    auto klick2 = std::make_shared<ImVec2>();
    add(klickLeiste(tr(Str::BzMessen)));
    add(pause(0.3));
    add(pruefe(zeile("P8", 1, "Klick auf Messen waehlt das Werkzeug (hervorgehoben, Bewegen usw. nicht)"), [] { return modellBl().werkzeug == 1 && modellBlWerkzeugAn(4) && !modellBlWerkzeugAn(0); }));
    add(klickAn("AP4: Messen - erster Punkt auf torso", [klick1] {
        ImVec2 p{-50.0F, -50.0F};
        (void)modellFlaecheAufSchirm(flaecheNach("torso"), p);
        *klick1 = p;
        return p;
    }));
    add(pause(0.3));
    add(klickAn("AP4: Messen - zweiter Punkt auf head", [klick2] {
        ImVec2 p{-50.0F, -50.0F};
        (void)modellFlaecheAufSchirm(flaecheNach("head"), p);
        *klick2 = p;
        return p;
    }));
    add(pause(0.4));
    add(pruefe(zeile("P8", 1, "zwei Klicks messen: beide Punkte liegen auf dem Modell genau unter den Klicks (zurueck auf den Schirm gerechnet, unter 1 Bildpunkt), der Abstand ist ihre Strecke"),
               [klick1, klick2] {
                   const ModellBlender& b = modellBl();
                   float d = 0.0F;
                   ImVec2 a;
                   ImVec2 c;
                   const bool ok = modellBlAbstand(d) && modellAufSchirm(aktiv(), b.messA, a) && modellAufSchirm(aktiv(), b.messB, c);
                   const float fa = std::hypot(a.x - klick1->x, a.y - klick1->y);
                   const float fc = std::hypot(c.x - klick2->x, c.y - klick2->y);
                   const float strecke = std::sqrt((b.messA[0] - b.messB[0]) * (b.messA[0] - b.messB[0]) + (b.messA[1] - b.messB[1]) * (b.messA[1] - b.messB[1]) +
                                                   (b.messA[2] - b.messB[2]) * (b.messA[2] - b.messB[2]));
                   char z[200];
                   std::snprintf(z, sizeof z, "P8 Messen: Abstand %.3f, Strecke %.3f, Abweichung auf dem Schirm %.3f / %.3f px", static_cast<double>(d), static_cast<double>(strecke),
                                 static_cast<double>(fa), static_cast<double>(fc));
                   melde(z);
                   return ok && b.messPunkte == 2 && fa < 1.0F && fc < 1.0F && std::fabs(d - strecke) < 1e-4F && d > 1.0F;
               }));
    add(pruefe(zeile("P8", 1, "die Meldung nennt den Abstand in Einheiten"), [] {
        float d = 0.0F;
        if (!modellBlAbstand(d)) { return false; }
        char z[64];
        std::snprintf(z, sizeof z, tr(Str::BzEinheiten), static_cast<double>(d));
        return mtab().meldung.find(z) != std::string::npos;
    }));
    add(ziehenAn("AP4: Messen - von head nach torso ziehen", [] {
                     ImVec2 p{-50.0F, -50.0F};
                     (void)modellFlaecheAufSchirm(flaecheNach("head"), p);
                     return p;
                 },
                 0.0F, 0.0F, 0));
    add(pause(0.3));
    add(pruefe(zeile("P8", 1, "Ziehen auf derselben Stelle zaehlt als Klick: ein neuer erster Punkt"), [] { return modellBl().messPunkte == 1; }));
    add(klickAn("AP4: Messen daneben (Hintergrund)", mbefoto::ansichtEcke));
    add(pause(0.3));
    add(pruefe(zeile("P8", 1, "ein Klick neben das Modell sagt es in der Meldung und laesst die Messung"), [] { return modellBl().messPunkte == 1 && mtab().meldung == tr(Str::BzDaneben); }));
    // Knochen waehlen
    auto knochen = std::make_shared<int>(-1);
    add(klickLeiste(tr(Str::BzKnochenWaehlen)));
    add(pause(0.4));
    add(pruefe(zeile("P8", 1, "Knochen waehlen zeigt die Knochen"), [] { return modellBl().werkzeug == 2 && g_app->modellKnochen; }));
    add(klickAn("AP4: auf das Gelenk eines Knochens", [knochen] {
        for (const char* n : {"lhumerus", "rhumerus", "cranium", "thoracic"}) {
            ImVec2 p;
            if (gelenkAufSchirm(knochenNach(n), p)) {
                *knochen = knochenNach(n);
                return p;
            }
        }
        return ImVec2{-50.0F, -50.0F};
    }));
    add(pause(0.4));
    add(pruefe(zeile("P8", 1, "ein Klick auf ein Gelenk waehlt einen Knochen, dessen Gelenk genau dort liegt"), [knochen] {
        ImVec2 a;
        ImVec2 b;
        const bool ok = *knochen >= 0 && mtab().knochenWahl >= 0 && gelenkAufSchirm(*knochen, a) && gelenkAufSchirm(mtab().knochenWahl, b);
        melde("P8 Knochen: geklickt " + std::to_string(*knochen) + ", gewaehlt " + std::to_string(mtab().knochenWahl));
        return ok && std::hypot(a.x - b.x, a.y - b.y) < 1.0F;
    }));
    // Flaeche waehlen
    add(klickLeiste(tr(Str::BzFlaecheWaehlen)));
    add(pause(0.3));
    add(klickAn("AP4: Flaeche head anklicken", [] {
        ImVec2 p{-50.0F, -50.0F};
        (void)modellFlaecheAufSchirm(flaecheNach("head"), p);
        return p;
    }));
    add(pause(0.4));
    add(pruefe(zeile("P8", 1, "Flaeche waehlen: ein Klick waehlt die Flaeche head"), [] {
        return modellBl().werkzeug == 3 && mtab().auswahlArt == AuswahlArt::Objekt && mtab().auswahl.size() == 1 && mtab().auswahl.front() == flaecheNach("head");
    }));
    // Foto
    auto foto = std::make_shared<std::string>();
    add(tu("AP4: Antwort des Dateidialogs fuer das Foto", [foto] {
        const char* f = std::getenv("BHED_FOTOS");
        *foto = std::string(f != nullptr ? f : ".") + "\\ap4_werkzeugfoto.bmp";
        std::remove(foto->c_str());
        platform::dialogAntwortImSelbsttest(*foto);
    }));
    add(klickLeiste(tr(Str::MbFoto)));
    add(pause(0.8));
    add(pruefe(zeile("P8", 1, "Foto schreibt das Bild der Ansicht (1920 breit) in die gewaehlte Datei"), [foto] {
        std::FILE* f = std::fopen(foto->c_str(), "rb");
        long groesse = 0;
        if (f != nullptr) {
            std::fseek(f, 0, SEEK_END);
            groesse = std::ftell(f);
            std::fclose(f);
        }
        melde("P8 Foto " + *foto + ": " + std::to_string(groesse) + " Bytes");
        return modellBl().fotos >= 1 && groesse > 1920L * 100L * 3L;
    }));

    // M0b: Messer startet das Schneiden (Blender K)
    add(tu("AP4: Bereich Modellieren", [] {
        mtab().bereich = ModellBereich::Modellieren;
        mtab().mesh.cutAn = false;
    }));
    add(pause(0.5));
    add(klickLeiste(tr(Str::BzMesser)));
    add(pause(0.3));
    add(pruefe(zeile("M0b", 1, "Klick auf Messer startet das Schneiden (wie K), der Knopf ist hervorgehoben"), [] {
        const int i = 4 + 4;
        return mtab().mesh.cutAn && modellBlWerkzeugAn(i);
    }));
    add(tu("AP4: Schneiden beenden", [] { mtab().mesh.cutAn = false; }));
    // U0b: Packen
    add(tu("AP4: Bereich UV, torso", [] {
        mtab().bereich = ModellBereich::Uv;
        const int f = flaecheNach("torso");
        mtab().auswahl = f >= 0 ? std::vector<int>{f} : std::vector<int>{};
    }));
    add(pause(0.6));
    auto befehleVor = std::make_shared<int>(0);
    add(tu("AP4: UV-Befehle zaehlen", [befehleVor] { *befehleVor = mtab().uv.befehle; }));
    add(klickLeiste(tr(Str::BePacken)));
    add(pause(0.5));
    add(pruefe(zeile("U0b", 1, "Klick auf Packen fuehrt den UV-Befehl Packen aus (wie Strg+P)"), [befehleVor] { return mtab().uv.befehle == *befehleVor + 1 && mtab().uv.letzterBefehl == tr(Str::UvPacken); }));
    add(tu("AP4: Packen zuruecknehmen", [] { (void)modellRueckgaengig(); }));
    // W0b: Verlauf, Gewicht holen, Pinsel
    add(tu("AP4: Bereich Weights", [] { mtab().bereich = ModellBereich::Weights; }));
    add(pause(0.5));
    add(klickLeiste(tr(Str::BeVerlauf)));
    add(pause(0.3));
    add(pruefe(zeile("W0b", 1, "Klick auf Verlauf waehlt das Werkzeug Verlauf"), [] { return modellGewichte().werkzeug == 2 && modellBlWerkzeugAn(6); }));
    add(klickLeiste(tr(Str::BeGewichtHolen)));
    add(pause(0.3));
    add(pruefe(zeile("W0b", 1, "Klick auf Gewicht holen: der naechste Klick in die Ansicht nimmt auf"), [] { return mtab().gwAufnehmenKlick && modellBlWerkzeugAn(7); }));
    add(klickLeiste(tr(Str::MgRPinsel)));
    add(pause(0.3));
    add(pruefe(zeile("W0b", 1, "Klick auf Pinsel malt Gewichte (Malen an, Werkzeug Pinsel)"), [] { return modellGewichte().malen && modellGewichte().werkzeug == 0 && !mtab().gwAufnehmenKlick && modellBlWerkzeugAn(4); }));
    add(tu("AP4: Malen aus", [] { modellGewichteMalenSetzen(false); }));
    // T0b: Fuellen
    add(tu("AP4: Bereich Painting", [] { mtab().bereich = ModellBereich::Painting; }));
    add(pause(0.6));
    add(klickLeiste(tr(Str::BeFuellen)));
    add(pause(0.3));
    add(pruefe(zeile("T0b", 1, "Klick auf Fuellen waehlt das Werkzeug Fuellen (hervorgehoben)"), [] {
        return mtab().malen.werkzeug == static_cast<int>(modell::texmalen::Werkzeug::Fuellen) && modellBlWerkzeugAn(5);
    }));
    add(klickLeiste(tr(Str::MgRPinsel)));
    add(pause(0.3));
    add(pruefe(zeile("T0b", 1, "Klick auf Pinsel waehlt wieder den Pinsel"), [] { return mtab().malen.werkzeug == static_cast<int>(modell::texmalen::Werkzeug::Pinsel); }));
    // A0b / A11: Pose kopieren
    add(tu("AP4: Bereich Animation, Sequenz", [] {
        mtab().bereich = ModellBereich::Animation;
        mtab().kopieDa = false;
        animWaehlen(mbefoto::sequenz(), false);
    }));
    add(pause(0.6));
    add(pruefe(zeile("A11", 1, "Gespiegelt einfuegen ist gesperrt, solange keine Pose kopiert ist"), [] {
        const Element* e = inLeiste(tr(Str::BeGespiegelt));
        return e != nullptr && !modellAnimWerkzeug(1).aktiv;
    }));
    add(klickLeiste(tr(Str::BeKopieren)));
    add(pause(0.4));
    add(pruefe(zeile("A11", 1, "Klick auf Pose kopieren kopiert die Pose (Schnittstelle von AP10)"), [] { return mtab().kopieDa && modellAnimWerkzeug(1).aktiv; }));
    add(tu("AP4: zurueck nach Ansehen/Pruefen", [] {
        mtab().bereich = ModellBereich::Ansehen;
        modellBl().werkzeug = 0;
        mtab().gizmo = 0;
        g_app->modellKnochen = false;
    }));
    add(pause(0.3));
}

// --- ENTSCHEID1 Punkt 3: Seitenleiste (N) eingebbar wie Blender ---------------------------------------------------------
// shank 12.11. "ja Bitte wie in Blender": Position/Drehung (und was Blender dort zeigt) als Zahlenfelder - ziehen aendert, ein Klick
// tippt ein (Enter uebernimmt, Esc bricht ab), Strg+Z nimmt zurueck, eine Rueckgaengig-Stufe je Eingabe. Blender: Item-Panel der
// Seitenleiste (Object: Location/Rotation/Scale/Dimensions; Pose Mode: der aktive Knochen; Edit Mode: Median) - hier die Flaeche
// (Mitte und Abmessungen ihres Kastens in der Pose) und der Griff des Rigs im Posenmodus (Ort und Drehung des Knochens).

const ModellBlender::SeitenFeld* seitenFeld(const std::string& id) {
    for (const auto& f : modellBl().seitenFelder) {
        if (f.id == id) { return &f; }
    }
    return nullptr;
}

std::function<ImVec2()> feldOrt(const std::string& id) {
    return [id] {
        const ModellBlender::SeitenFeld* f = seitenFeld(id);
        return f != nullptr ? f->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    };
}

bool kasten(float m[3], float d[3]) {
    float mn[3];
    float mx[3];
    if (!modellGrenzen(true, mn, mx)) { return false; }
    for (int i = 0; i < 3; ++i) {
        m[i] = (mn[i] + mx[i]) * 0.5F;
        d[i] = mx[i] - mn[i];
    }
    return true;
}

// Euler XYZ (Blenders Vorgabe, R = Rz * Ry * Rx) aus der Drehung einer Weltmatrix (Spalten normiert) - eigene Rechnung des Tests
void euler(const BoneMatrix& b, float out[3]) {
    float r[3][3];
    for (int c = 0; c < 3; ++c) {
        const float l = std::sqrt(b.m[0][c] * b.m[0][c] + b.m[1][c] * b.m[1][c] + b.m[2][c] * b.m[2][c]);
        for (int z = 0; z < 3; ++z) { r[z][c] = l > 1e-6F ? b.m[z][c] / l : 0.0F; }
    }
    const float grad = 180.0F / 3.14159265358979323846F;
    out[0] = std::atan2(r[2][1], r[2][2]) * grad;
    out[1] = std::asin(std::clamp(-r[2][0], -1.0F, 1.0F)) * grad;
    out[2] = std::atan2(r[1][0], r[0][0]) * grad;
}

int griffNach(modell::GriffArt art) {
    const auto& g = mtab().rig.griffe;
    for (std::size_t i = 0; i < g.size(); ++i) {
        if (g[i].art == art) { return static_cast<int>(i); }
    }
    return -1;
}

int griffKnochen() {
    const ModellTab& t = mtab();
    return t.griffWahl >= 0 && static_cast<std::size_t>(t.griffWahl) < t.rig.griffe.size() ? t.rig.griffe[static_cast<std::size_t>(t.griffWahl)].knochen : -1;
}

std::string zahlText(float v) {
    char z[32];
    std::snprintf(z, sizeof z, "%.2f", static_cast<double>(v));
    return z;
}

Schritt strgZ() { return tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"); }

void seitenEingabeSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct Stand {
        float m[3]{};
        float d[3]{};
        float e[3]{};
        float o[3]{};
        int schritte = 0;
        float ziel = 0.0F;
    };
    auto st = std::make_shared<Stand>();
    // --- Flaeche torso in Ansehen/Pruefen (Objektmodus) ---
    add(tu("AP4 ENTSCHEID1: wie Blender, Ansehen/Pruefen, torso, Seitenleiste an (Reiter Gegenstand)", [] {
        modellBedienungSetzen(1);
        mtab().bereich = ModellBereich::Ansehen;
        mtab().gizmo = 0;
        modellBl().werkzeug = 0;
        modellBl().seitenleiste = true;
        modellBl().seitenReiter = 0;
        const int f = flaecheNach("torso");
        mtab().auswahl = f >= 0 ? std::vector<int>{f} : std::vector<int>{};
        mtab().knochenWahl = -1;
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: Position X/Y/Z und Dimensionen X/Y/Z der Flaeche sind Zahlenfelder in der Seitenleiste (wie Blender), mit Tooltip"), [] {
        int n = 0;
        std::string ids;
        for (const auto& f : modellBl().seitenFelder) {
            ids += f.id + (f.eingebbar ? "+ " : "- ");
            n += (f.id.rfind("ort", 0) == 0 || f.id.rfind("masse", 0) == 0) && f.eingebbar && !leer(f.rect) && fenster("mtseitenleiste").Contains(f.rect) ? 1 : 0;
        }
        melde("Seitenfelder: " + ids);
        return n == 6;
    }));
    add(tu("AP4 ENTSCHEID1: Stand merken", [st] {
        (void)kasten(st->m, st->d);
        st->schritte = modellBl().seitenSchritte;
        st->ziel = std::round(st->m[0] * 10.0F + 37.0F) / 10.0F;
    }));
    add(klickAn("AP4 ENTSCHEID1: Klick auf Position X (zum Eintippen)", feldOrt("ort0")));
    add(pause(0.3));
    add(Schritt{"AP4 ENTSCHEID1: Wert tippen", [st](int b) {
                    if (b == 0) { return false; }
                    ImGuiIO& io = ImGui::GetIO();
                    if (b == 1) { io.AddKeyEvent(ImGuiMod_Ctrl, true); io.AddKeyEvent(ImGuiKey_A, true); return false; }
                    if (b == 2) { io.AddKeyEvent(ImGuiKey_A, false); io.AddKeyEvent(ImGuiMod_Ctrl, false); return false; }
                    if (b == 3) { io.AddInputCharactersUTF8(zahlText(st->ziel).c_str()); return false; }
                    return b >= 6;
                }});
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.4));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: Eintippen und Enter setzt die Mitte der Flaeche auf den Wert (die Flaeche wandert), ein Rueckgaengig-Schritt"), [st] {
        float m[3];
        float d[3];
        const bool ok = kasten(m, d);
        char z[200];
        std::snprintf(z, sizeof z, "Position X: soll %.2f, ist %.3f (vorher %.3f), Y %.3f -> %.3f, Schritte %d", static_cast<double>(st->ziel), static_cast<double>(m[0]),
                      static_cast<double>(st->m[0]), static_cast<double>(st->m[1]), static_cast<double>(m[1]), modellBl().seitenSchritte - st->schritte);
        melde(z);
        return ok && std::fabs(m[0] - st->ziel) < 0.01F && std::fabs(m[1] - st->m[1]) < 0.01F && std::fabs(d[0] - st->d[0]) < 0.01F && modellBl().seitenSchritte == st->schritte + 1;
    }));
    add(tu("AP4 ENTSCHEID1: Fokus im Model-Tab", [] { mtab().fokus = true; }));
    add(strgZ());
    add(pause(0.4));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: Strg+Z nimmt die ganze Eingabe in einem Schritt zurueck"), [st] {
        float m[3];
        float d[3];
        return kasten(m, d) && std::fabs(m[0] - st->m[0]) < 1e-3F && std::fabs(m[1] - st->m[1]) < 1e-3F && std::fabs(m[2] - st->m[2]) < 1e-3F;
    }));
    // Esc bricht ab
    add(tu("AP4 ENTSCHEID1: Stand merken", [st] { st->schritte = modellBl().seitenSchritte; }));
    add(klickAn("AP4 ENTSCHEID1: Klick auf Position Z", feldOrt("ort2")));
    add(pause(0.3));
    add(tippe("999"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.4));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: Esc bricht die Eingabe ab - die Flaeche steht wie vorher, kein Rueckgaengig-Schritt"), [st] {
        float m[3];
        float d[3];
        return kasten(m, d) && std::fabs(m[2] - st->m[2]) < 1e-3F && modellBl().seitenSchritte == st->schritte;
    }));
    // Ziehen: Dimensionen Z
    add(ziehenAn("AP4 ENTSCHEID1: Dimensionen Z nach rechts ziehen", feldOrt("masse2"), 60.0F, 0.0F, 0));
    add(pause(0.4));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: Ziehen nach rechts vergroessert die Dimension Z (die Flaeche wird hoeher), ein Rueckgaengig-Schritt"), [st] {
        float m[3];
        float d[3];
        const bool ok = kasten(m, d);
        char z[160];
        std::snprintf(z, sizeof z, "Dimension Z %.3f -> %.3f, X %.3f -> %.3f, Schritte %d", static_cast<double>(st->d[2]), static_cast<double>(d[2]), static_cast<double>(st->d[0]),
                      static_cast<double>(d[0]), modellBl().seitenSchritte - st->schritte);
        melde(z);
        return ok && d[2] > st->d[2] + 0.5F && std::fabs(d[0] - st->d[0]) < 0.01F && modellBl().seitenSchritte == st->schritte + 1;
    }));
    add(tu("AP4 ENTSCHEID1: Fokus im Model-Tab", [] { mtab().fokus = true; }));
    add(strgZ());
    add(pause(0.4));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: Strg+Z nimmt das ganze Ziehen in einem Schritt zurueck"), [st] {
        float m[3];
        float d[3];
        return kasten(m, d) && std::fabs(d[2] - st->d[2]) < 1e-3F && std::fabs(m[2] - st->m[2]) < 1e-3F;
    }));
    add(mbefoto::mausAuf("das Feld Position Y", [] {
        static Element e;
        const ModellBlender::SeitenFeld* f = seitenFeld("ort1");
        e.rect = f != nullptr ? f->rect : ImRect{};
        return f != nullptr ? &e : nullptr;
    }));
    add(pause(0.6));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: unter der Maus erklaert der Tooltip das Feld (ziehen, tippen, Strg+Z)"), [] { return tooltipDa(); }));
    // --- Knochen in Ansehen/Pruefen: nur ablesbar (Blender stellt Knochen im Posenmodus) ---
    add(tu("AP4 ENTSCHEID1: Knochen cranium gewaehlt, keine Flaeche", [] {
        mtab().auswahl.clear();
        mtab().knochenWahl = knochenNach("cranium");
        setzeMaus(ImVec2{-50.0F, -50.0F});
    }));
    add(pause(0.4));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: ein Knochen im Objektmodus steht nur zum Ablesen da (wie Blender: Knochen stellt man im Posenmodus)"), [] {
        const ModellBlender::SeitenFeld* o = seitenFeld("ort0");
        const ModellBlender::SeitenFeld* d = seitenFeld("dreh0");
        return o != nullptr && d != nullptr && !o->eingebbar && !d->eingebbar;
    }));
    // --- Griffe des Rigs im Posenmodus (Bereich Animation) ---
    add(tu("AP4 ENTSCHEID1: Bereich Animation, Rig, Griff Becken", [] {
        mtab().bereich = ModellBereich::Animation;
        mtab().knochenWahl = -1;
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
        if (!modellRigDa()) { (void)modellAnimRigErstellen(false); }
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Becken));
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: Griff Becken im Posenmodus - Position und Rotation sind eingebbar"), [] {
        int n = 0;
        for (const char* id : {"ort0", "ort1", "ort2", "dreh0", "dreh1", "dreh2"}) {
            const ModellBlender::SeitenFeld* f = seitenFeld(id);
            n += f != nullptr && f->eingebbar ? 1 : 0;
        }
        return mtab().griffWahl >= 0 && n == 6;
    }));
    add(tu("AP4 ENTSCHEID1: Stand des Beckens merken", [st] {
        const int k = griffKnochen();
        if (k < 0 || static_cast<std::size_t>(k) >= g_app->modellWelt.size()) { return; }
        const BoneMatrix& b = g_app->modellWelt[static_cast<std::size_t>(k)];
        for (int i = 0; i < 3; ++i) { st->o[i] = b.m[i][3]; }
        st->schritte = modellBl().seitenSchritte;
        st->ziel = std::round(st->o[0] * 10.0F + 53.0F) / 10.0F;
    }));
    add(klickAn("AP4 ENTSCHEID1: Klick auf Position X des Beckens", feldOrt("ort0")));
    add(pause(0.3));
    add(Schritt{"AP4 ENTSCHEID1: Wert tippen", [st](int b) {
                    if (b == 0) { return false; }
                    ImGuiIO& io = ImGui::GetIO();
                    if (b == 1) { io.AddKeyEvent(ImGuiMod_Ctrl, true); io.AddKeyEvent(ImGuiKey_A, true); return false; }
                    if (b == 2) { io.AddKeyEvent(ImGuiKey_A, false); io.AddKeyEvent(ImGuiMod_Ctrl, false); return false; }
                    if (b == 3) { io.AddInputCharactersUTF8(zahlText(st->ziel).c_str()); return false; }
                    return b >= 6;
                }});
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.5));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: Position X des Beckens eingetippt - der Knochen steht dort (in der Pose), ein Rueckgaengig-Schritt"), [st] {
        const int k = griffKnochen();
        if (k < 0 || static_cast<std::size_t>(k) >= g_app->modellWelt.size()) { return false; }
        const BoneMatrix& b = g_app->modellWelt[static_cast<std::size_t>(k)];
        char z[200];
        std::snprintf(z, sizeof z, "Becken X: soll %.2f, ist %.3f (vorher %.3f), Y %.3f -> %.3f, Schritte %d", static_cast<double>(st->ziel), static_cast<double>(b.m[0][3]),
                      static_cast<double>(st->o[0]), static_cast<double>(st->o[1]), static_cast<double>(b.m[1][3]), modellBl().seitenSchritte - st->schritte);
        melde(z);
        return std::fabs(b.m[0][3] - st->ziel) < 0.02F && std::fabs(b.m[1][3] - st->o[1]) < 0.02F && modellBl().seitenSchritte == st->schritte + 1;
    }));
    add(tu("AP4 ENTSCHEID1: Fokus im Model-Tab", [] { mtab().fokus = true; }));
    add(strgZ());
    add(pause(0.5));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: Strg+Z stellt das Becken zurueck"), [st] {
        const int k = griffKnochen();
        if (k < 0 || static_cast<std::size_t>(k) >= g_app->modellWelt.size()) { return false; }
        const BoneMatrix& b = g_app->modellWelt[static_cast<std::size_t>(k)];
        return std::fabs(b.m[0][3] - st->o[0]) < 1e-3F && std::fabs(b.m[1][3] - st->o[1]) < 1e-3F;
    }));
    // Kopf: nur drehen (wie im Gizmo/R); Rotation Z eintippen
    add(tu("AP4 ENTSCHEID1: Griff Kopf", [st] {
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Kopf));
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: Griff Kopf - Rotation eingebbar, Position nur ablesbar (der Kopf dreht nur, wie mit dem Gizmo)"), [] {
        const ModellBlender::SeitenFeld* o = seitenFeld("ort0");
        const ModellBlender::SeitenFeld* d = seitenFeld("dreh2");
        return o != nullptr && d != nullptr && !o->eingebbar && d->eingebbar;
    }));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: die nur ablesbaren Felder zeigen ihren Wert grau wie Blenders inaktive Felder, die eingebbaren nicht (Fotofund)"), [] {
        int grau = 0;
        int falsch = 0;
        for (const auto& f : modellBl().seitenFelder) {
            grau += !f.eingebbar && f.grau ? 1 : 0;
            falsch += (f.eingebbar && f.grau) || (!f.eingebbar && !f.grau) ? 1 : 0;
        }
        melde("Felder grau " + std::to_string(grau) + ", falsch " + std::to_string(falsch));
        return grau == 3 && falsch == 0;
    }));
    add(tu("AP4 ENTSCHEID1: Drehung des Kopfs merken", [st] {
        const int k = griffKnochen();
        if (k < 0 || static_cast<std::size_t>(k) >= g_app->modellWelt.size()) { return; }
        euler(g_app->modellWelt[static_cast<std::size_t>(k)], st->e);
        st->schritte = modellBl().seitenSchritte;
        st->ziel = std::round(st->e[2] * 10.0F + 200.0F) / 10.0F;
    }));
    add(klickAn("AP4 ENTSCHEID1: Klick auf Rotation Z des Kopfs", feldOrt("dreh2")));
    add(pause(0.3));
    add(Schritt{"AP4 ENTSCHEID1: Winkel tippen", [st](int b) {
                    if (b == 0) { return false; }
                    ImGuiIO& io = ImGui::GetIO();
                    if (b == 1) { io.AddKeyEvent(ImGuiMod_Ctrl, true); io.AddKeyEvent(ImGuiKey_A, true); return false; }
                    if (b == 2) { io.AddKeyEvent(ImGuiKey_A, false); io.AddKeyEvent(ImGuiMod_Ctrl, false); return false; }
                    if (b == 3) { io.AddInputCharactersUTF8(zahlText(st->ziel).c_str()); return false; }
                    return b >= 6;
                }});
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.5));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: Rotation Z des Kopfs eingetippt - der Knochen hat diese Rotation (Euler XYZ wie Blender), X und Y bleiben"), [st] {
        const int k = griffKnochen();
        if (k < 0 || static_cast<std::size_t>(k) >= g_app->modellWelt.size()) { return false; }
        float e[3];
        euler(g_app->modellWelt[static_cast<std::size_t>(k)], e);
        char z[200];
        std::snprintf(z, sizeof z, "Kopf Rotation: soll Z %.1f, ist %.2f / %.2f / %.2f (vorher %.2f / %.2f / %.2f)", static_cast<double>(st->ziel), static_cast<double>(e[0]),
                      static_cast<double>(e[1]), static_cast<double>(e[2]), static_cast<double>(st->e[0]), static_cast<double>(st->e[1]), static_cast<double>(st->e[2]));
        melde(z);
        const auto winkel = [](float a, float b) { return std::fabs(std::remainder(a - b, 360.0F)); };
        return winkel(e[2], st->ziel) < 0.1F && winkel(e[0], st->e[0]) < 0.1F && winkel(e[1], st->e[1]) < 0.1F && modellBl().seitenSchritte == st->schritte + 1;
    }));
    add(tu("AP4 ENTSCHEID1: Fokus im Model-Tab", [] { mtab().fokus = true; }));
    add(strgZ());
    add(pause(0.5));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1: Strg+Z stellt die Drehung des Kopfs zurueck"), [st] {
        const int k = griffKnochen();
        if (k < 0 || static_cast<std::size_t>(k) >= g_app->modellWelt.size()) { return false; }
        float e[3];
        euler(g_app->modellWelt[static_cast<std::size_t>(k)], e);
        return std::fabs(std::remainder(e[2] - st->e[2], 360.0F)) < 0.01F;
    }));
    add(tu("AP4 ENTSCHEID1: Seitenleiste aus, zurueck nach Ansehen/Pruefen", [] {
        modellRigGriffWaehlen(-1);
        modellBl().seitenleiste = false;
        mtab().bereich = ModellBereich::Ansehen;
    }));
    add(pause(0.3));
}

// --- ENTSCHEID1 Punkt 5: Werkzeuggruppen wie Blender, Werkzeugnamen zeigen ----------------------------------------------------
// Blender 5.0: space_toolsystem_common.py draw_cls - eine Gruppe ist ein Knopf mit operator_menu_hold (Menue WM_MT_toolsystem_submenu,
// das zuletzt gewaehlte Werkzeug der Gruppe steht im Knopf); interface_handlers.cc: halten oeffnet nach BUTTON_AUTO_OPEN_THRESH
// (0,2 s), Ziehen ueber die Ziehschwelle sofort, Loslassen ausserhalb des Menues schliesst es (BLOCK_POPUP_HOLD); kleines Dreieck
// in der Ecke (Commit 7c02008e "button triangle for hold action").

struct Halten {
    int los = -1;          // Bild, in dem die Maus zum Ziel geht
    double t0 = 0.0;
    bool offen = false;    // war die Gruppe offen, als die Zeit um war?
    float nach = -1.0F;
    bool gezogen = false;
};

// Druecken auf `ort`, waehrend des Haltens um dx ziehen, nach `sek` Sekunden die Maus auf `ziel` und loslassen.
Schritt haltenUndLoslassen(const std::string& was, std::function<ImVec2()> ort, double sek, float dx, std::function<ImVec2()> ziel, std::shared_ptr<Halten> h) {
    auto z = std::make_shared<ImVec2>();
    return {"Halte " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    *h = Halten{};
                    *z = ort();
                    setzeMaus(*z);
                    return false;
                }
                if (b == 2) {
                    io.AddMouseButtonEvent(0, true);
                    h->t0 = ImGui::GetTime();
                    return false;
                }
                if (b > 2 && h->los < 0) {
                    if (dx != 0.0F && b <= 5) { setzeMaus(ImVec2{z->x + dx * static_cast<float>(b - 2) / 3.0F, z->y}); }
                    if (ImGui::GetTime() - h->t0 < sek && b < 3000) { return false; }
                    h->offen = !modellBl().gruppeOffen.empty();
                    h->nach = modellBl().gruppeOffenNach;
                    h->gezogen = modellBl().gruppeGezogen;
                    h->los = b;
                    setzeMaus(ziel());
                    return false;
                }
                if (b == h->los + 3) { io.AddMouseButtonEvent(0, false); return false; }
                return b >= h->los + 8;
            }};
}

std::function<ImVec2()> eintragOrt(Str text) {
    return [text] {
        for (const auto& e : modellBl().gruppenEintraege) {
            if (e.first == tr(text)) { return e.second.GetCenter(); }
        }
        return ImVec2{-50.0F, -50.0F};
    };
}

int knopfNr(const std::string& label) {
    const auto& k = modellBl().knoepfe;
    for (std::size_t i = 0; i < k.size(); ++i) {
        if (k[i].first == label) { return static_cast<int>(i); }
    }
    return -1;
}

void gruppenSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("AP4 ENTSCHEID1: wie Blender, Modellieren, torso, Werkzeug ohne Kaestchen", [] {
        modellBedienungSetzen(1);
        mtab().bereich = ModellBereich::Modellieren;
        modellBl().werkzeugleiste = true;
        const int f = flaecheNach("torso");
        mtab().auswahl = f >= 0 ? std::vector<int>{f} : std::vector<int>{};
        mtab().mesh.cutAn = false;
        mtab().mesh.quickAn = false;
        setzeMaus(ImVec2{-50.0F, -50.0F});
    }));
    add(pause(0.6));
    add(pruefe(zeile("M0b", 1, "ENTSCHEID1: Extrude Region und Knife sind Gruppen wie in Blender - ein kleines Dreieck unten rechts im Knopf"), [] {
        const ModellBlender& b = modellBl();
        bool ok = b.gruppen.size() == 2 && b.dreiecke.size() == 2;
        std::string z;
        for (std::size_t i = 0; ok && i < b.gruppen.size(); ++i) {
            const int k = knopfNr(b.gruppen[i]);
            z += b.gruppen[i] + " ";
            ok = k >= 0;
            if (!ok) { break; }
            const ImRect r = b.knoepfe[static_cast<std::size_t>(k)].second;
            const ImRect d = b.dreiecke[i];
            ok = r.Contains(d) && d.Min.x >= r.GetCenter().x && d.Min.y >= r.GetCenter().y && d.GetWidth() <= r.GetWidth() * 0.4F;
        }
        melde("Gruppen: " + z + "- " + std::to_string(b.gruppen.size()) + " Gruppen, " + std::to_string(b.dreiecke.size()) + " Dreiecke");
        return ok && std::find(b.gruppen.begin(), b.gruppen.end(), std::string(tr(Str::BeExtrude))) != b.gruppen.end() &&
               std::find(b.gruppen.begin(), b.gruppen.end(), std::string(tr(Str::BzMesser))) != b.gruppen.end();
    }));
    add(klickLeiste(tr(Str::BeExtrude)));
    add(pause(0.4));
    add(pruefe(zeile("M0b", 1, "ENTSCHEID1: ein kurzer Klick auf die Gruppe nimmt das Werkzeug im Knopf (Extrude Region = Extrudieren als Gruppe), die Gruppe bleibt zu"), [] {
        const ModellMesh& m = mtab().mesh;
        return m.kaestchenOffen && m.werkzeug == MeshWerkzeug::Extrude && m.art == 0 && modellBl().gruppeOffen.empty();
    }));
    add(tu("AP4 ENTSCHEID1: Kaestchen schliessen", [] { modellMeshUebernehmen(false); }));
    add(pause(0.3));
    auto h = std::make_shared<Halten>();
    add(haltenUndLoslassen("Extrude Region 0,4 s, dann auf Extrude Individual loslassen", knopfOrt(tr(Str::BeExtrude)), 0.4, 0.0F, eintragOrt(Str::BeExtrudeEinzeln), h));
    add(pause(0.4));
    add(pruefe(zeile("M0b", 1, "ENTSCHEID1: Gedrueckt halten oeffnet nach 0,2 s die Gruppe (Blender BUTTON_AUTO_OPEN_THRESH), Loslassen auf einem Eintrag waehlt ihn"), [h] {
        const ModellMesh& m = mtab().mesh;
        char z[200];
        std::snprintf(z, sizeof z, "Halten: offen %d nach %.3f s, gezogen %d; Werkzeug %d, Art %d, Leiste %s", h->offen ? 1 : 0, static_cast<double>(h->nach), h->gezogen ? 1 : 0,
                      static_cast<int>(m.werkzeug), m.art, leistenTexte().c_str());
        melde(z);
        return h->offen && !h->gezogen && h->nach >= 0.18F && h->nach < 0.4F && m.kaestchenOffen && m.werkzeug == MeshWerkzeug::Extrude && m.art == 2 &&
               modellBl().gruppeOffen.empty();
    }));
    add(pruefe(zeile("M0b", 1, "ENTSCHEID1: der Knopf zeigt jetzt das gewaehlte Werkzeug der Gruppe (Extrude Individual), wie Blender"), [] {
        return knopfNr(tr(Str::BeExtrudeEinzeln)) == 4 && knopfNr(tr(Str::BeExtrude)) < 0;
    }));
    add(tu("AP4 ENTSCHEID1: Kaestchen schliessen", [] { modellMeshUebernehmen(false); }));
    add(pause(0.3));
    auto z2 = std::make_shared<Halten>();
    add(haltenUndLoslassen("Knife und gleich ziehen, dann auf dem Knopf loslassen", knopfOrt(tr(Str::BzMesser)), 0.1, 14.0F, knopfOrt(tr(Str::BzMesser)), z2));
    add(pause(0.4));
    add(pruefe(zeile("M0b", 1, "ENTSCHEID1: Ziehen oeffnet die Gruppe sofort (vor 0,2 s); Loslassen ausserhalb des Menues schliesst es ohne Wahl"), [z2] {
        const ModellMesh& m = mtab().mesh;
        char z[160];
        std::snprintf(z, sizeof z, "Ziehen: offen %d nach %.3f s, gezogen %d; Messer %d, Halbieren %d", z2->offen ? 1 : 0, static_cast<double>(z2->nach), z2->gezogen ? 1 : 0,
                      m.cutAn ? 1 : 0, m.quickAn ? 1 : 0);
        melde(z);
        return z2->offen && z2->gezogen && z2->nach >= 0.0F && z2->nach < 0.18F && !m.cutAn && !m.quickAn && modellBl().gruppeOffen.empty();
    }));
    add(tu("AP4 ENTSCHEID1: Extrude-Gruppe zurueck auf Extrude Region", [] { modellBlGruppenVorgabe(); }));
    add(pause(0.3));
    // Rechtsklick "Werkzeugnamen zeigen"
    auto schmal = std::make_shared<float>(0.0F);
    add(tu("AP4 ENTSCHEID1: Breite der Leiste merken", [schmal] { *schmal = fenster("mtwerkzeugleiste").GetWidth(); }));
    add(klickAn("AP4 ENTSCHEID1: Rechtsklick in die Werkzeugleiste (unter den Knoepfen)", [] {
                    const ImRect l = fenster("mtwerkzeugleiste");
                    return ImVec2{l.GetCenter().x, l.Max.y - em() * 0.8F};
                }, 1));
    add(pause(0.4));
    add(pruefe(zeile("R17", 1, "ENTSCHEID1: der Rechtsklick auf die Werkzeugleiste bietet \"Werkzeugnamen zeigen\""), [] { return finde(tr(Str::BeWerkzeugnamen), "##Popup_", 0, false) != nullptr; }));
    add(klick(tr(Str::BeWerkzeugnamen), "##Popup_"));
    add(pause(0.6));
    add(pruefe(zeile("R17", 1, "ENTSCHEID1: mit Namen ist die Leiste breiter, jeder Knopf zeigt seinen Namen neben dem Symbol, die Einstellung merkt es sich"), [schmal] {
        const ModellBlender& b = modellBl();
        const ImRect l = fenster("mtwerkzeugleiste");
        float laengster = 0.0F;
        bool ok = b.knoepfe.size() >= 7;
        for (const auto& k : b.knoepfe) {
            laengster = std::max(laengster, ImGui::CalcTextSize(k.first.c_str()).x);
            ok = ok && l.Contains(k.second) && k.second.GetWidth() > k.second.GetHeight() * 1.5F;
        }
        Settings neu;
        const bool datei = readSettings(writeSettings(g_app->settings), neu) && neu.modellBlWerkzeugnamen;
        melde("Namen: Leiste " + std::to_string(static_cast<int>(*schmal)) + " -> " + std::to_string(static_cast<int>(l.GetWidth())) + ", laengster Name " +
              std::to_string(static_cast<int>(laengster)) + ", Einstellung " + std::to_string(g_app->settings.modellBlWerkzeugnamen) + ", Datei " + std::to_string(datei));
        return ok && b.namenGezeigt && l.GetWidth() > *schmal + laengster && g_app->settings.modellBlWerkzeugnamen && datei;
    }));
    add(klickAn("AP4 ENTSCHEID1: Rechtsklick in die Werkzeugleiste", [] {
                    const ImRect l = fenster("mtwerkzeugleiste");
                    return ImVec2{l.GetCenter().x, l.Max.y - em() * 0.8F};
                }, 1));
    add(pause(0.4));
    add(klick(tr(Str::BeWerkzeugnamen), "##Popup_"));
    add(pause(0.6));
    add(pruefe(zeile("R17", 1, "ENTSCHEID1: noch einmal - wieder die schmale Leiste nur mit Symbolen"), [schmal] {
        return !modellBl().namenGezeigt && std::fabs(fenster("mtwerkzeugleiste").GetWidth() - *schmal) < 1.0F && !g_app->settings.modellBlWerkzeugnamen && leisteSenkrecht();
    }));
    add(tu("AP4 ENTSCHEID1: zurueck nach Ansehen/Pruefen", [] {
        mtab().bereich = ModellBereich::Ansehen;
        mtab().auswahl.clear();
    }));
    add(pause(0.3));
}

// --- Umschalten ohne Verlust, Anordnung je Bedienmodus, Finden -----------------------------------------------------------

void umschaltSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct Stand {
        std::vector<int> auswahl;
        int knochen = -1;
        int bild = 0;
        int sequenz = -1;
    };
    auto vorher = std::make_shared<Stand>();
    add(tu("AP4: Auswahl, Knochen, Bild setzen; Seitenleiste an, Werkzeugleiste aus", [vorher] {
        modellBedienungSetzen(1);
        mtab().bereich = ModellBereich::Weights;
        const int f = flaecheNach("torso");
        mtab().auswahl = f >= 0 ? std::vector<int>{f} : std::vector<int>{};
        mtab().knochenWahl = knochenNach("lhumerus");
        g_app->animPlaying = false;
        modellBl().seitenleiste = true;
        modellBl().werkzeugleiste = false;
        vorher->auswahl = mtab().auswahl;
        vorher->knochen = mtab().knochenWahl;
        vorher->sequenz = g_app->animIndex;
        vorher->bild = g_app->animFrame;
    }));
    add(pause(0.5));
    add(tu("AP4: wie 3ds Max", [] { modellBedienungSetzen(0); }));
    add(pause(0.8));
    add(pruefe(zeile("R16", 0, "nach dem Umschalten wie 3ds Max: Explorer links, keine Werkzeugleiste, keine Seitenleiste, Auswahl, Knochen, Sequenz und Bild unveraendert"), [vorher] {
        const ImRect x = fenster("mtexplorer");
        const ImRect v = fenster("mtansichten");
        return !leer(x) && x.Max.x <= v.Min.x && kindFenster("mtwerkzeugleiste") == nullptr && kindFenster("mtseitenleiste") == nullptr && mtab().auswahl == vorher->auswahl &&
               mtab().knochenWahl == vorher->knochen && g_app->animIndex == vorher->sequenz && g_app->animFrame == vorher->bild && mtab().bereich == ModellBereich::Weights;
    }));
    add(tu("AP4: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(0.8));
    add(pruefe(zeile("R16", 1, "zurueck wie Blender: Auswahl, Knochen, Bild unveraendert, Werkzeugleiste weiter aus, Seitenleiste weiter an"), [vorher] {
        return mtab().auswahl == vorher->auswahl && mtab().knochenWahl == vorher->knochen && g_app->animFrame == vorher->bild && !modellBl().werkzeugleiste && modellBl().seitenleiste &&
               kindFenster("mtseitenleiste") != nullptr && kindFenster("mtwerkzeugleiste") == nullptr;
    }));
    add(tu("AP4: Leisten zurueck", [] {
        modellBl().seitenleiste = false;
        modellBl().werkzeugleiste = true;
    }));
    // Anordnung je Bedienmodus: wie Blender die Zeitleiste ausblenden, wie 3ds Max ist sie da, wie Blender wieder aus
    add(tu("AP4: wie Blender das Band (Zeitleiste) ausblenden", [] { dock::ausblenden("modell.leiste", true); }));
    add(pause(0.6));
    add(tu("AP4: wie 3ds Max", [] { modellBedienungSetzen(0); }));
    add(pause(0.8));
    add(pruefe(zeile("R16", 0, "jeder Bedienmodus hat seine Anordnung: wie 3ds Max ist das Band da"), [] { return !dock::versteckt("modell.leiste"); }));
    add(tu("AP4: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(0.8));
    add(pruefe(zeile("R16", 1, "... und wie Blender wieder ausgeblendet (die Anordnung von Blender kam zurueck)"), [] { return dock::versteckt("modell.leiste"); }));
    add(tu("AP4: Band wieder einblenden (wie Blender), dann wie 3ds Max und zurueck", [] { dock::ausblenden("modell.leiste", false); }));
    add(pause(0.5));
    add(tu("AP4: wie 3ds Max", [] { modellBedienungSetzen(0); }));
    add(pause(0.6));
    add(tu("AP4: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(0.6));
    add(pruefe(zeile("R16", 1, "nach dem Einblenden bleibt das Band in beiden Modi da"), [] { return !dock::versteckt("modell.leiste"); }));

    // Finden: Alle Aktionen suchen und Rechtsklick
    for (int modus = 1; modus >= 0; --modus) {
        add(tu(std::string("AP4 Finden: ") + (modus == 0 ? "wie 3ds Max" : "wie Blender") + ", Alle Aktionen suchen", [modus] {
            modellBedienungSetzen(modus);
            mtab().bereich = ModellBereich::Ansehen;
        }));
        add(pause(0.5));
        add(klickAn("AP4: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
        // je Aktion die Suche oeffnen und ihren Namen tippen (die lange Liste rollt; was ausserhalb liegt, meldet ImGui ohne Namen)
        struct Such {
            Str text;
            const char* taste;
            bool nurBlender;
        };
        for (const Such& x : {Such{Str::BzWerkzeugleiste, "T", true}, Such{Str::BzSeitenleiste, "N", true}, Such{Str::BzKnochenWaehlen, "", false}, Such{Str::BzMessenMax, "", false}}) {
            const std::string zeileText = std::string(tr(x.text)) + (x.taste[0] != '\0' ? std::string(" (") + x.taste + ")" : std::string());
            const bool soll = modus == 1 || !x.nurBlender;
            add(tu("AP4 Finden: Suche oeffnen", [] { mtab().aktionenOffen = true; }));
            add(pause(0.5));
            add(tippe(tr(x.text)));
            add(pause(0.4));
            add(pruefe(zeile(x.nurBlender ? "R17" : "P8", modus, std::string("\"Alle Aktionen suchen\" ") + (soll ? "nennt " : "nennt nicht (wie 3ds Max) ") + zeileText), [zeileText, soll] {
                return mtab().aktionenSichtbar && (finde(zeileText, "##Popup_", 0, false) != nullptr) == soll;
            }));
            if (x.text != Str::BzMessenMax) { add(mbefoto::popupsZu()); }
        }
        add(klick(tr(Str::BzMessenMax), "##Popup_"));
        add(pause(0.4));
        add(pruefe(zeile("P8", modus, "die Zeile Abstand messen waehlt das Werkzeug Messen"), [] { return modellBl().werkzeug == 1 && !mtab().aktionenSichtbar; }));
        add(tu("AP4 Finden: Rechtsklick-Menue der Ansicht", [] { mtab().kontextOeffnen = true; }));
        add(pause(0.5));
        add(pruefe(zeile("R18", modus, modus == 1 ? "im Rechtsklick der Ansicht: Werkzeugleiste (T), Seitenleiste (N), Abstand messen" : "im Rechtsklick der Ansicht: Abstand messen"), [modus] {
            const bool leisten = finde(tr(Str::BzWerkzeugleiste), "##Popup_", 0, false) != nullptr && finde(tr(Str::BzSeitenleiste), "##Popup_", 0, false) != nullptr;
            return mtab().kontextSichtbar && finde(tr(Str::BzMessenMax), "##Popup_", 0, false) != nullptr && leisten == (modus == 1);
        }));
        if (modus == 1) {
            add(klick(tr(Str::BzSeitenleiste), "##Popup_"));
            add(pause(0.4));
            add(pruefe(zeile("R18", 1, "Seitenleiste im Rechtsklick blendet sie ein"), [] { return modellBl().seitenleiste; }));
            add(tu("AP4: Seitenleiste aus", [] { modellBl().seitenleiste = false; }));
        }
        add(mbefoto::popupsZu());
        add(tu("AP4: Werkzeug zurueck", [] { modellBl().werkzeug = 0; }));
        add(pause(0.3));
    }
    add(pruefe(zeile("R16", 0, "die Taste T bleibt wie 3ds Max die Ansicht Oben (keine Werkzeugleiste)"), [] { return kindFenster("mtwerkzeugleiste") == nullptr; }));
    add(klickAn("AP4: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(taste(ImGuiKey_T, "T"));
    add(pause(0.4));
    add(pruefe(zeile("R16", 0, "T wie 3ds Max: Ansicht Oben, die Werkzeugleiste von Blender bleibt an (unberuehrt)"), [] {
        const bool ok = aktiv().art == AnsichtArt::Oben && modellBl().werkzeugleiste && kindFenster("mtwerkzeugleiste") == nullptr;
        mtab().ansichten[static_cast<std::size_t>(std::clamp(mtab().aktiv, 0, 3))].art = AnsichtArt::Perspektive;
        return ok;
    }));
}

// --- Fotos ----------------------------------------------------------------------------------------------------------------

// ENTSCHEID1: die Gruppe eines Knopfs gedrueckt halten, bis sie offen ist, dann die Maus auf den Eintrag `eintrag` - die Taste bleibt
// gedrueckt (fuer ein Foto des offenen Menues); gruppeLoslassen() laesst ausserhalb los und schliesst es ohne Wahl.
Schritt gruppeHalten(const std::string& knopf, Str eintrag) {
    auto t0 = std::make_shared<double>(0.0);
    return {"Halte die Gruppe " + knopf, [=](int b) {
                if (b == 0) {
                    setzeMaus(knopfOrt(knopf)());
                    return false;
                }
                if (b == 2) {
                    ImGui::GetIO().AddMouseButtonEvent(0, true);
                    *t0 = ImGui::GetTime();
                    return false;
                }
                if (b < 2 || (ImGui::GetTime() - *t0 < 0.4 && b < 3000)) { return false; }
                setzeMaus(eintragOrt(eintrag)());
                return b > 2 && ImGui::GetTime() - *t0 >= 0.5;
            }};
}

Schritt gruppeLoslassen() {
    return {"Gruppe loslassen (ausserhalb)", [](int b) {
                if (b == 0) {
                    setzeMaus(ImVec2{-50.0F, -50.0F});
                    return false;
                }
                if (b == 2) {
                    ImGui::GetIO().AddMouseButtonEvent(0, false);
                    return false;
                }
                return b >= 6;
            }};
}


void fotoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    static const char* const kName[kModellBereiche] = {"pruefen", "modell", "uv", "weights", "paint", "anim"};
    for (int modus = 0; modus < 2; ++modus) {
        const std::string p = modus == 0 ? "ap4_max_" : "ap4_blender_";
        for (int b = 0; b < kModellBereiche; ++b) {
            add(tu("AP4 Foto: " + p + kName[b], [modus, b] {
                modellBedienungSetzen(modus);
                mtab().bereich = static_cast<ModellBereich>(b);
                modellBl().werkzeugleiste = true;
                modellBl().seitenleiste = false;
                modellBl().werkzeug = 0;
                const int f = flaecheNach("torso");
                mtab().auswahl = f >= 0 ? std::vector<int>{f} : std::vector<int>{};
                if (b == static_cast<int>(ModellBereich::Animation) && !modellRigDa()) { (void)modellAnimRigErstellen(false); }
                if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
                setzeMaus(ImVec2{-50.0F, -50.0F});
            }));
            add(pause(1.2));
            add(foto(p + kName[b]));
            if (modus == 1) {
                add(pruefe(zeile("R17", 1, std::string("Foto ") + kName[b] + ": alle Knoepfe der Werkzeugleiste stehen ganz in ihrer Spalte, untereinander (auch im kleinen Fenster)"), [] {
                    const ImRect l = fenster("mtwerkzeugleiste");
                    bool ok = !leer(l) && modellBl().knoepfe.size() >= 7;
                    for (const auto& k : modellBl().knoepfe) { ok = ok && l.Contains(k.second); }
                    melde("Foto Leiste: " + rechteck("leiste", l) + ", " + std::to_string(modellBl().knoepfe.size()) + " Knoepfe" +
                          (modellBl().knoepfe.empty() ? std::string() : ", " + rechteck("letzter", modellBl().knoepfe.back().second)));
                    return ok && leisteSenkrecht();
                }));
            }
        }
    }
    // wie Blender: Seitenleiste an (Ansehen/Pruefen mit Messung), Werkzeugleiste aus, Moduswahl offen
    add(tu("AP4 Foto: wie Blender, Ansehen/Pruefen, Seitenleiste an, Messung torso - head", [] {
        modellBedienungSetzen(1);
        mtab().bereich = ModellBereich::Ansehen;
        modellBl().seitenleiste = true;
        modellBl().werkzeug = 1;
        modellBl().messPunkte = 0;
    }));
    add(pause(0.8));
    add(klickAn("AP4 Foto: erster Punkt", [] {
        ImVec2 p{-50.0F, -50.0F};
        (void)modellFlaecheAufSchirm(flaecheNach("torso"), p);
        return p;
    }));
    add(klickAn("AP4 Foto: zweiter Punkt", [] {
        ImVec2 p{-50.0F, -50.0F};
        (void)modellFlaecheAufSchirm(flaecheNach("head"), p);
        return p;
    }));
    add(tu("AP4 Foto: Maus weg", [] { setzeMaus(ImVec2{-50.0F, -50.0F}); }));
    add(pause(1.0));
    add(foto("ap4_blender_seitenleiste"));
    add(tu("AP4 Foto: Werkzeugleiste aus, Seitenleiste aus", [] {
        modellBl().werkzeugleiste = false;
        modellBl().seitenleiste = false;
        modellBl().werkzeug = 0;
    }));
    add(pause(0.8));
    add(foto("ap4_blender_leisteaus"));
    add(tu("AP4 Foto: Werkzeugleiste an, Bereich Animation", [] {
        modellBl().werkzeugleiste = true;
        mtab().bereich = ModellBereich::Animation;
    }));
    add(pause(0.6));
    add(klickAn("AP4 Foto: Moduswahl", [] { return modellBl().modusFeld.GetCenter(); }));
    add(pause(0.6));
    add(foto("ap4_blender_moduswahl"));
    add(mbefoto::popupsZu());
    add(mbefoto::mausAuf("den Knopf Pose kopieren", [] { return inLeiste(tr(Str::BeKopieren)); }));
    add(pause(0.3));
    add(foto("ap4_blender_tooltip"));
    add(tu("AP4 Foto: Maus weg", [] { setzeMaus(ImVec2{-50.0F, -50.0F}); }));
    add(pause(0.3));
    // ENTSCHEID1: Werkzeugnamen zeigen, die offene Gruppe Extrude, die Seitenleiste mit Zahlenfeldern (Griff Kopf im Posenmodus)
    add(tu("AP4 Foto: Modellieren, Werkzeugnamen zeigen", [] {
        mtab().bereich = ModellBereich::Modellieren;
        g_app->settings.modellBlWerkzeugnamen = true;
        modellBlGruppenVorgabe();
    }));
    add(pause(0.8));
    add(foto("ap4_blender_namen"));
    add(gruppeHalten(tr(Str::BeExtrude), Str::BeExtrudeEinzeln));
    add(foto("ap4_blender_gruppe"));
    add(gruppeLoslassen());
    add(tu("AP4 Foto: schmale Leiste; Animation, Griff Kopf, Seitenleiste", [] {
        g_app->settings.modellBlWerkzeugnamen = false;
        mtab().bereich = ModellBereich::Animation;
        if (!modellRigDa()) { (void)modellAnimRigErstellen(false); }
        modellRigGriffWaehlen(griffNach(modell::GriffArt::Kopf));
        modellBl().seitenleiste = true;
        modellBl().seitenReiter = 0;
    }));
    add(pause(0.8));
    add(mbefoto::mausAuf("das Feld Rotation Z", [] {
        static Element e;
        const ModellBlender::SeitenFeld* f = seitenFeld("dreh2");
        e.rect = f != nullptr ? f->rect : ImRect{};
        return f != nullptr ? &e : nullptr;
    }));
    add(pause(0.8));
    add(foto("ap4_blender_seite_felder"));
    add(pruefe(zeile("R18", 1, "ENTSCHEID1 Foto: jede Beschriftung der Seitenleiste passt in ihre Spalte oder endet mit ... (Blender kuerzt so; Fotofund im kleinen Fenster)"), [] {
        bool ok = !modellBl().seitenFelder.empty();
        std::string z;
        for (const auto& f : modellBl().seitenFelder) {
            if (f.beschriftungBreite > f.platz + 0.5F) {
                ok = false;
                z += " " + f.id + " '" + f.beschriftung + "' " + std::to_string(static_cast<int>(f.beschriftungBreite)) + ">" + std::to_string(static_cast<int>(f.platz));
            }
        }
        melde("Beschriftungen:" + (z.empty() ? std::string(" alle passen") : z));
        return ok;
    }));
    add(tu("AP4 Foto: Seitenleiste aus, Maus weg", [] {
        modellRigGriffWaehlen(-1);
        modellBl().seitenleiste = false;
        setzeMaus(ImVec2{-50.0F, -50.0F});
    }));
    add(pause(0.3));
}

// Bilder fuer das Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap4): h_ap4_max, h_ap4_blender (Modellieren), h_ap4_seitenleiste
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP4 Handbuch: Bereich Modellieren, torso, wie 3ds Max", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = 0.85F;
        g_app->settings.modellFeldBreite = 0.0F;
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Modellieren;
        const int f = flaecheNach("torso");
        g_app->mtab.auswahl = f >= 0 ? std::vector<int>{f} : std::vector<int>{};
        modellBl().werkzeugleiste = true;
        modellBl().seitenleiste = false;
        modellExplorerVorgabe();
        // die ganze Figur im Bild (eingepasst beim Laden; Einpassen auf die Auswahl zeigte nur den Rumpf), BOTH_WALK1 angehalten
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
        setzeMaus(ImVec2{-50.0F, -50.0F});
    }));
    add(pause(1.2));
    add(hilfe2::schrittFoto("h_ap4_max"));
    add(tu("AP4 Handbuch: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(1.0));
    add(hilfe2::schrittFoto("h_ap4_blender"));
    add(tu("AP4 Handbuch: Werkzeugnamen zeigen (ENTSCHEID1)", [] {
        g_app->settings.modellBlWerkzeugnamen = true;
        modellBlGruppenVorgabe();
    }));
    add(pause(0.8));
    add(gruppeHalten(tr(Str::BeExtrude), Str::BeExtrudeEinzeln));
    add(hilfe2::schrittFoto("h_ap4_gruppen"));
    add(gruppeLoslassen());
    add(tu("AP4 Handbuch: schmale Leiste", [] { g_app->settings.modellBlWerkzeugnamen = false; }));
    add(pause(0.5));
    add(tu("AP4 Handbuch: wie Blender, Ansehen/Pruefen, Seitenleiste, Messen", [] {
        g_app->mtab.bereich = ModellBereich::Ansehen;
        modellBl().seitenleiste = true;
        modellBl().werkzeug = 1;
        modellBl().messPunkte = 0;
    }));
    add(pause(0.8));
    add(klickAn("AP4 Handbuch: erster Punkt", [] {
        ImVec2 p{-50.0F, -50.0F};
        (void)modellFlaecheAufSchirm(flaecheNach("torso"), p);
        return p;
    }));
    add(klickAn("AP4 Handbuch: zweiter Punkt", [] {
        ImVec2 p{-50.0F, -50.0F};
        (void)modellFlaecheAufSchirm(flaecheNach("head"), p);
        return p;
    }));
    add(tu("AP4 Handbuch: Maus weg", [] { setzeMaus(ImVec2{-50.0F, -50.0F}); }));
    add(pause(1.0));
    add(hilfe2::schrittFoto("h_ap4_seitenleiste"));
    add(tu("AP4 Handbuch: zurueck", [=] {
        modellBl().seitenleiste = false;
        modellBl().werkzeug = 0;
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
    if (n != "ap4" && n != "ap4foto") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_AP4_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    const char* m = std::getenv("BHED_AP4_MODELL");
    const std::string modell = m != nullptr ? m : "models/players/kyle/model.glm";
    auto skala = std::make_shared<float>(1.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP4: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        if (const char* t = std::getenv("BHED_AP4_THEMA")) {
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
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap4.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    add(tu("AP4: Aufteilung wie neue Einstellungen, wie 3ds Max, Bereich Ansehen/Pruefen", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        modellBl() = ModellBlender{};
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
        char zeile[300];
        std::snprintf(zeile, sizeof zeile, "%s, %zu Knochen, Anzeige %.0f x %.0f, Oberflaeche %.2f, Schrift %.0f Punkte hoch", g_app->modelPath.c_str(), g_app->anim.bones.size(),
                      static_cast<double>(ImGui::GetIO().DisplaySize.x), static_cast<double>(ImGui::GetIO().DisplaySize.y), static_cast<double>(g_app->uiScale),
                      static_cast<double>(ImGui::GetFontSize()));
        melde(zeile);
    }));
    add(pause(1.0));
    add(klickAn("AP4: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (n == "ap4") {
        rahmenSchritte(s);
        bereichSchritte(s);
        werkzeugSchritte(s);
        seitenEingabeSchritte(s);
        gruppenSchritte(s);
        umschaltSchritte(s);
    } else {
        fotoSchritte(s);
    }
    add(tu("AP4: Modell-Tab zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        modellBl() = ModellBlender{};
        g_app->modellKnochen = false;
    }));
    add(tu("AP4: zurueck", [=] {
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

}  // namespace ap4
