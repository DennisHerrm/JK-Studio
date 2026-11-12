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
        {ModellBereich::Modellieren, "M0b", {Str::MdExtrude, Str::MdInset, Str::BzFase, Str::MdLoopCut, Str::BzMesser, Str::MdMerge}, Str::MbBlEditModus},
        {ModellBereich::Uv, "U0b", {Str::UvNahtMarkieren, Str::UvAbwickeln, Str::UvEntspannen, Str::UvPacken}, Str::MbBlEditModus},
        {ModellBereich::Weights, "W0b", {Str::MgRPinsel, Str::MgWeich, Str::MgVerlauf, Str::MrGwAufnehmen}, Str::BzModusWeight},
        {ModellBereich::Painting, "T0b", {Str::PtWzPinsel, Str::PtWzFuellen, Str::PtWzKlonen, Str::PtWzWischen, Str::PtWzMaske}, Str::BzModusTexture},
        {ModellBereich::Animation, "A0b", {Str::MbRgKopieren, Str::MbRgGespiegelt, Str::MbKvReduzieren}, Str::BzModusPose},
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
    add(klickLeiste(tr(Str::UvPacken)));
    add(pause(0.5));
    add(pruefe(zeile("U0b", 1, "Klick auf Packen fuehrt den UV-Befehl Packen aus (wie Strg+P)"), [befehleVor] { return mtab().uv.befehle == *befehleVor + 1 && mtab().uv.letzterBefehl == tr(Str::UvPacken); }));
    add(tu("AP4: Packen zuruecknehmen", [] { (void)modellRueckgaengig(); }));
    // W0b: Verlauf, Gewicht holen, Pinsel
    add(tu("AP4: Bereich Weights", [] { mtab().bereich = ModellBereich::Weights; }));
    add(pause(0.5));
    add(klickLeiste(tr(Str::MgVerlauf)));
    add(pause(0.3));
    add(pruefe(zeile("W0b", 1, "Klick auf Verlauf waehlt das Werkzeug Verlauf"), [] { return modellGewichte().werkzeug == 2 && modellBlWerkzeugAn(6); }));
    add(klickLeiste(tr(Str::MrGwAufnehmen)));
    add(pause(0.3));
    add(pruefe(zeile("W0b", 1, "Klick auf Gewicht holen: der naechste Klick in die Ansicht nimmt auf"), [] { return mtab().gwAufnehmenKlick && modellBlWerkzeugAn(7); }));
    add(klickLeiste(tr(Str::MgRPinsel)));
    add(pause(0.3));
    add(pruefe(zeile("W0b", 1, "Klick auf Pinsel malt Gewichte (Malen an, Werkzeug Pinsel)"), [] { return modellGewichte().malen && modellGewichte().werkzeug == 0 && !mtab().gwAufnehmenKlick && modellBlWerkzeugAn(4); }));
    add(tu("AP4: Malen aus", [] { modellGewichteMalenSetzen(false); }));
    // T0b: Fuellen
    add(tu("AP4: Bereich Painting", [] { mtab().bereich = ModellBereich::Painting; }));
    add(pause(0.6));
    add(klickLeiste(tr(Str::PtWzFuellen)));
    add(pause(0.3));
    add(pruefe(zeile("T0b", 1, "Klick auf Fuellen waehlt das Werkzeug Fuellen (hervorgehoben)"), [] {
        return mtab().malen.werkzeug == static_cast<int>(modell::texmalen::Werkzeug::Fuellen) && modellBlWerkzeugAn(5);
    }));
    add(klickLeiste(tr(Str::PtWzPinsel)));
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
        const Element* e = inLeiste(tr(Str::MbRgGespiegelt));
        return e != nullptr && !modellAnimWerkzeug(1).aktiv;
    }));
    add(klickLeiste(tr(Str::MbRgKopieren)));
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
    add(mbefoto::mausAuf("den Knopf Pose kopieren", [] { return inLeiste(tr(Str::MbRgKopieren)); }));
    add(pause(0.3));
    add(foto("ap4_blender_tooltip"));
    add(tu("AP4 Foto: Maus weg", [] { setzeMaus(ImVec2{-50.0F, -50.0F}); }));
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
