// selbsttest_ap11.inc - AP11 (MBENTWURF Teil B, 13.11.): mehrere Modelle in Reitern, Kopieren/Einfuegen von Flaechen
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap11 | ap11foto | ap11hb)
//
//   ap11      je Zeile der Abgleichtabelle eine Reihe von Pruefungen (jede einmal rot auf dem Stand ohne die Anschluesse):
//               R5  die Reiterzeile ueber den Ansichten (beide Bedienmodi): ein Reiter "kyle/model.glm" mit x, "+", Lage ueber der
//                   aktiven Ansicht und links vom rechten Feld, Tooltips; ein zweites Modell in einem neuen Reiter; Wechsel per Klick;
//                   je Modell bleibt alles (Datei, Animation und Bild, Auswahl, Rueckgaengig, Modifier-Stapel, Control Rig, Einfrieren,
//                   Sperren, Skin-Auge, Kamera und Ansichten, Bemalung) - das Bild der Ansicht ist nach dem Hin- und Rueckwechsel
//                   bitgleich, die Zwischenspeicher passen zum gezeigten Modell; Rueckgaengig je Modell; Schliessen fragt bei
//                   ungespeicherten Aenderungen (Nein behaelt, Ja schliesst), der letzte Reiter wird leer; Beenden fragt je Modell;
//                   ein schon offenes Modell oeffnet nicht doppelt; schnelles Wechseln mit laufender Animation, Schliessen mit offenen
//                   Fenstern (Auto-Skin, Zeitkonfiguration, Dopesheet, Info), Wechsel mitten im Gewichte-Strich
//               X19 Rechtsklick einer Flaeche: Info, Kopieren, "Einfuegen als Kind hiervon", Status, Wurzel loesen (Reihenfolge des
//                   Entwurfs); Kopieren fuellt die Ablage, rechts in der Reiterzeile "Ablage: torso aus kyle/model.glm - Strg+V fuegt
//                   ein"; Einfuegen in ein anderes Modell als Kind (Name, Eltern, Stufen, Shader, Gewichte nach Knochennamen, ein
//                   Rueckgaengig-Schritt, Meldung mit den Grenzen), Strg+V mit gewaehlter Flaeche, Rueckgaengig nur im Ziel, in ein
//                   .md3-Requisit starr; "Alle Aktionen suchen" kennt Kopieren und "In neuem Tab oeffnen"
//               P1  Gruppe Modell: "In neuem Tab oeffnen" mit Strg+O neben "Importieren ..."; Strg+O oeffnet die gewaehlte Datei in
//                   einem neuen Reiter
//   ap11foto  nur Fotos (BHED_FOTOS=<Ordner>): ap11_*
//   ap11hb    die Bilder fuer das Handbuch (h_ap11_*)
//   BHED_AP11_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe wie der Foto-Selbsttest mbefoto)
//   BHED_AP11_THEMA=hell|dunkel
// Eingebunden in selbsttest.cpp (nach selbsttest_ent3a.inc).
namespace ap11 {

void melde(const std::string& z) { diag::detail("Knopftest: AP11 " + z); }

ModellTab& mtab() { return g_app->mtab; }
ModellReiterProbe& rp() { return modellReiterProbe(); }
std::size_t uz(int i) { return static_cast<std::size_t>(i); }
int teil(const char* n) { return mb1::teil(n); }

// Der Name, den die n-te eingefuegte Kopie von `name` im aktiven Modell bekommt (vorhandene Namen: _1, _2 ...)
std::string kopieName(const std::string& name, int n) {
    const modell::Inhalt& in = mtab().dok.inhalt();
    if (in.teilIndex(name) < 0) { return n == 1 ? name : name + "_" + std::to_string(n - 1); }
    return name + "_" + std::to_string(n);
}

// Eine Flaeche des Ziels: die gesuchte, sonst die erste, die kein Tag ist
int flaecheOder(const char* name) {
    const int i = teil(name);
    if (i >= 0) { return i; }
    const modell::Inhalt& in = mtab().dok.inhalt();
    const auto kappe = [](const std::string& n) { return n.size() >= 4U && n.compare(n.size() - 4U, 4U, "_off") == 0; };
    for (int runde = 0; runde < 2; ++runde) {
        for (std::size_t k = 0; k < in.teile.size(); ++k) {
            const modell::Teil& t = in.teile[k];
            if (t.istTag() || kappe(t.name) || t.stufen.empty() || t.stufen[0].ecken.empty()) { continue; }
            if (runde == 1 || t.name.find(name) != std::string::npos) { return static_cast<int>(k); }
        }
    }
    return -1;
}

std::string flaechenName(const char* name) {
    const int i = flaecheOder(name);
    return i >= 0 ? mtab().dok.inhalt().teile[uz(i)].name : std::string(name);
}


const char* const kKyle = "models/players/kyle/model.glm";
const char* const kJedi = "models/players/jedi_hm/model.glm";

std::string fmt(Str s, auto... a) {
    char b[1200];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

const Element* imPopup(const std::string& label) { return finde(label, "##Popup_", 0, false); }

Schritt klickPopup(const std::string& label) {
    return klickAn("\"" + label + "\" (Menue)", [label] {
        const Element* e = imPopup(label);
        if (e == nullptr) { meldeFehler("AP11: \"" + label + "\" steht nicht im Menue"); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    });
}

std::function<ImVec2()> reiterOrt(int i) {
    return [i] {
        const auto& r = rp().reiter;
        if (i < 0 || uz(i) >= r.size()) { return ImVec2{-50.0F, -50.0F}; }
        // auf den Text, nicht auf das x
        return ImVec2{r[uz(i)].second.Min.x + ImGui::GetFontSize() * 1.5F, r[uz(i)].second.GetCenter().y};
    };
}

std::function<ImVec2()> reiterX(int i) {
    return [i] {
        const auto& r = rp().reiter;
        if (i < 0 || uz(i) >= r.size()) { return ImVec2{-50.0F, -50.0F}; }
        const ImRect& x = r[uz(i)].second;
        return ImVec2{x.Max.x - ImGui::GetStyle().FramePadding.x - ImGui::GetFontSize() * 0.5F, x.GetCenter().y};
    };
}

std::uint64_t ansichtHash() {
    std::vector<unsigned char> px;
    int w = 0;
    int h = 0;
    if (!modellAnsichtPixel(mtab().aktiv, px, w, h) || px.empty()) { return 0; }
    std::uint64_t f = 1469598103934665603ULL;
    for (const unsigned char c : px) { f = (f ^ c) * 1099511628211ULL; }
    return f ^ (static_cast<std::uint64_t>(w) << 32U) ^ static_cast<std::uint64_t>(h);
}

// Passen die Zwischenspeicher der Module zum gezeigten Modell? (Zeiger und Verweise auf das alte duerfen den Wechsel nicht ueberleben)
bool stimmig(std::string& warum) {
    const ModellTab& t = mtab();
    const std::size_t teile = t.dok.inhalt().teile.size();
    const std::size_t flaechen = t.anzeige.surfaces.size();
    warum.clear();
    if (g_app->model.surfaces.size() != teile) { warum += "Modell " + std::to_string(g_app->model.surfaces.size()) + " != Teile " + std::to_string(teile) + "; "; }
    if (flaechen != teile) { warum += "Anzeige " + std::to_string(flaechen) + "; "; }
    if (modellHautOrte().size() != flaechen) { warum += "Haut " + std::to_string(modellHautOrte().size()) + "; "; }
    if (modellInhaltGezeigt().teile.size() != teile) { warum += "gezeigt " + std::to_string(modellInhaltGezeigt().teile.size()) + "; "; }
    if (modellStapelZeichnung().surfaces.size() != flaechen) { warum += "Stapel " + std::to_string(modellStapelZeichnung().surfaces.size()) + "; "; }
    if (!modellExplorer().frostTeil.empty() && modellExplorer().frostTeil.size() != teile) { warum += "Frost " + std::to_string(modellExplorer().frostTeil.size()) + "; "; }
    if (t.dokModell != g_app->modelPath) { warum += "Dokument von " + t.dokModell + "; "; }
    for (const ExplorerLage& l : modellExplorer().lage) {
        if (l.art == ExplorerArt::Flaeche && (l.index < 0 || uz(l.index) >= teile || t.dok.inhalt().teile[uz(l.index)].name != l.name)) {
            warum += "Explorer-Zeile " + l.name + "; ";
            break;
        }
    }
    return warum.empty();
}

Schritt stimmigPruefen(const std::string& wann) {
    return pruefe("AP11 R5: " + wann + " - die Zwischenspeicher passen zum gezeigten Modell (Anzeige, Haut, Stapel, Pruefung, Explorer)", [wann] {
        std::string w;
        const bool ok = stimmig(w);
        melde("stimmig " + wann + ": " + (ok ? std::string("ja") : w) + " (" + g_app->modelPath + ")");
        return ok;
    });
}

// Was je Modell bleiben muss
struct Stand {
    std::string pfad;
    std::vector<int> auswahl;
    std::string undo;
    std::size_t stapelTorso = 0;
    bool rig = false;
    float winkel = 0.0F;
    float abstand = 0.0F;
    bool vier = false;
    int aktiv = -1;
    int art0 = -1;
    int anim = -1;
    int bild = -1;
    bool frostKopf = false;
    int sperren = 0;
    bool skinAus = false;
    int bemalt = -1;
    bool malUngesp = false;
    std::size_t teile = 0;
    int bereich = 0;
};

Stand standJetzt() {
    const ModellTab& t = mtab();
    Stand s;
    s.pfad = g_app->modelPath;
    s.auswahl = t.auswahl;
    s.undo = t.dok.rueckgaengigName();
    const int to = teil("torso");
    s.stapelTorso = to >= 0 ? t.dok.inhalt().teile[uz(to)].stapel.size() : 0U;
    s.rig = modellRigDa();
    s.winkel = g_app->modelCam.angles[1];
    s.abstand = g_app->modelOrbit.distance;
    s.vier = t.vierAnsichten;
    s.aktiv = t.aktiv;
    s.art0 = static_cast<int>(t.ansichten[0].art);
    s.anim = g_app->animIndex;
    s.bild = g_app->animFrame;
    s.frostKopf = teil("head") >= 0 && modellTeilEingefroren(teil("head"));
    s.sperren = static_cast<int>(std::count_if(modellGewichte().gesperrt.begin(), modellGewichte().gesperrt.end(), [](char c) { return c != 0; }));
    s.skinAus = modellEnt3aSkinAus();
    s.malUngesp = modellMalenUngespeichert();
    s.teile = t.dok.inhalt().teile.size();
    s.bereich = static_cast<int>(t.bereich);
    return s;
}

std::string text(const Stand& s) {
    char b[600];
    std::snprintf(b, sizeof b, "%s Wahl %zu Undo '%s' Stapel %zu Rig %d Winkel %.1f Abstand %.1f vier %d aktiv %d art0 %d Anim %d Bild %d Frost %d Sperren %d Skin %d Mal %d Teile %zu Bereich %d",
                  s.pfad.c_str(), s.auswahl.size(), s.undo.c_str(), s.stapelTorso, s.rig ? 1 : 0, static_cast<double>(s.winkel), static_cast<double>(s.abstand), s.vier ? 1 : 0,
                  s.aktiv, s.art0, s.anim, s.bild, s.frostKopf ? 1 : 0, s.sperren, s.skinAus ? 1 : 0, s.malUngesp ? 1 : 0, s.teile, s.bereich);
    return b;
}

bool gleich(const Stand& a, const Stand& b) {
    return a.pfad == b.pfad && a.auswahl == b.auswahl && a.undo == b.undo && a.stapelTorso == b.stapelTorso && a.rig == b.rig && a.winkel == b.winkel &&
           a.abstand == b.abstand && a.vier == b.vier && a.aktiv == b.aktiv && a.art0 == b.art0 && a.anim == b.anim && a.bild == b.bild && a.frostKopf == b.frostKopf &&
           a.sperren == b.sperren && a.skinAus == b.skinAus && a.malUngesp == b.malUngesp && a.teile == b.teile && a.bereich == b.bereich;
}

Schritt antwort(Str ja) { return klick(tr(ja), "###frage"); }

void aufraeumenFrage() {
    if (g_app->frageOffen) {
        g_app->frageOffen = false;
        g_app->frageJa = nullptr;
        g_app->frageNein = nullptr;
    }
}

// Ein Modell in einem neuen Reiter (wie "+" > Aus den Spieldateien, ohne Browser)
std::vector<Schritt> neuerReiter(const char* pfad) {
    std::vector<Schritt> s;
    s.push_back(tu(std::string("AP11: in neuem Reiter ") + pfad, [pfad] {
        g_app->leftMode = 2;
        modellInNeuemReiterAusArchiv(pfad);
    }));
    s.push_back(pause(1.2));
    s.push_back(tu("AP11: eine Ansicht, Animation angehalten", [] {
        mtab().vierAnsichten = false;
        mtab().aktiv = 3;
        g_app->animPlaying = false;
    }));
    s.push_back(pause(0.4));
    return s;
}

void modusWaehlen(int modus, ModellBereich b) {
    modellBedienungSetzen(modus);
    mtab().bereich = b;
    g_app->settings.modellFeldBreite = 0.0F;
    g_app->modelDirty = true;
}

// --- R5: die Reiterzeile -----------------------------------------------------------------------------------------------------------
// Platz nur, wenn gebraucht: mit einem Modell und leerer Ablage steht der Model-Tab da wie bisher (Bilder und Selbsttests unveraendert),
// mit einem zweiten Modell (oder etwas in der Ablage) steht die Zeile ueber der Ansicht.
void zeileSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("AP11 R5: ein Modell, Ablage leer, wie 3ds Max, Ansehen/Pruefen, eine Ansicht", [] {
        modusWaehlen(0, ModellBereich::Ansehen);
        mtab().vierAnsichten = false;
    }));
    add(pause(0.6));
    add(pruefe("AP11 R5: mit einem Modell und leerer Ablage keine Reiterzeile - der Model-Tab steht da wie bisher", [] {
        return modellReiterZahl() == 1 && !modellAblageDa() && !modellReiterZeileSichtbar() && rp().bild < ImGui::GetFrameCount() - 3;
    }));
    for (Schritt& x : neuerReiter(kJedi)) { add(std::move(x)); }
    for (int modus = 0; modus < 2; ++modus) {
        const std::string wie = modus == 1 ? " wie Blender: " : " wie 3ds Max: ";
        add(tu("AP11 R5:" + wie + "Ansehen/Pruefen, eine Ansicht", [modus] {
            modusWaehlen(modus, ModellBereich::Ansehen);
            mtab().vierAnsichten = false;
            mtab().aktiv = 3;
        }));
        add(pause(0.6));
        add(pruefe("AP11 R5" + wie + "mit dem zweiten Modell steht ueber der Ansicht die Reiterzeile: \"kyle/model.glm\", \"jedi_hm/model.glm\", \"+\"", [] {
            const ModellReiterProbe& p = rp();
            const bool jetzt = p.bild >= ImGui::GetFrameCount() - 3;
            melde("R5 Zeile: Bild " + std::to_string(p.bild) + "/" + std::to_string(ImGui::GetFrameCount()) + ", Reiter " + std::to_string(p.reiter.size()) +
                  (p.reiter.size() < 2U ? std::string() : " '" + p.reiter[0].first + "' '" + p.reiter[1].first + "'") + ", plus " +
                  std::to_string(static_cast<int>(p.plus.GetWidth())));
            return jetzt && p.reiter.size() == 2U && p.reiter[0].first == "kyle/model.glm" && p.reiter[1].first == "jedi_hm/model.glm" && p.plus.GetWidth() > 0.0F &&
                   modellReiterZahl() == 2;
        }));
        add(pruefe("AP11 R5" + wie + "die Zeile liegt direkt ueber der aktiven Ansicht, so breit wie die Spalte der Ansichten, links vom rechten Feld", [] {
            const ModellReiterProbe& p = rp();
            const ModellAnsicht& a = mtab().ansichten[mtab().aktiv];
            const ImRect feld = modellBl().feld;
            const float abstand = a.lo[1] - p.zeile.Max.y;
            char b[300];
            std::snprintf(b, sizeof b, "R5 Lage: Zeile %.0f..%.0f x %.0f..%.0f, Ansicht x %.0f y %.0f, Feld x %.0f, Abstand %.1f", static_cast<double>(p.zeile.Min.x),
                          static_cast<double>(p.zeile.Max.x), static_cast<double>(p.zeile.Min.y), static_cast<double>(p.zeile.Max.y), static_cast<double>(a.lo[0]),
                          static_cast<double>(a.lo[1]), static_cast<double>(feld.Min.x), static_cast<double>(abstand));
            melde(b);
            return p.zeile.GetHeight() > 0.0F && abstand >= -1.0F && abstand <= ImGui::GetStyle().ItemSpacing.y + 2.0F && std::fabs(p.zeile.Min.x - a.lo[0]) <= 2.0F &&
                   p.zeile.Max.x <= feld.Min.x + 1.0F && p.zeile.GetWidth() >= a.groesse[0] - 2.0F;
        }));
        add(tu("AP11 R5: Maus auf den Reiter kyle", [] { setzeMaus(reiterOrt(0)()); }));
        add(pause(1.2));
        add(pruefe("AP11 R5" + wie + "der Tooltip des Reiters nennt den ganzen Pfad", [] {
            melde("R5 Tooltip Reiter: '" + rp().tooltip + "'");
            return rp().tooltip == kKyle;
        }));
        add(tu("AP11 R5: Maus auf +", [] { setzeMaus(rp().plus.GetCenter()); }));
        add(pause(1.2));
        add(pruefe("AP11 R5" + wie + "der Tooltip von \"+\" nennt Strg+O", [] { return rp().tooltip == tr(Str::MxPlusHint); }));
        add(klickAn("AP11 R5: Klick auf +", [] { return rp().plus.GetCenter(); }));
        add(pause(0.5));
        add(pruefe("AP11 R5" + wie + "\"+\" oeffnet das Menue: Modelldatei ... (Strg+O) und Aus den Spieldateien ...", [] {
            return imPopup(tr(Str::MxNeuerTabDatei)) != nullptr && imPopup(tr(Str::MxNeuerTabPk3)) != nullptr;
        }));
        add(mbefoto::popupsZu());
    }
    add(tu("AP11 R5: wie 3ds Max; den zweiten Reiter schliessen", [] {
        modusWaehlen(0, ModellBereich::Ansehen);
        modellReiterSchliessen(1);
    }));
    add(pause(0.6));
    add(pruefe("AP11 R5: wieder ein Modell - die Zeile geht wieder weg (kein Platz, der nicht gebraucht wird)", [] {
        return rp().geschlossen >= 1 && modellReiterZahl() == 1 && !modellReiterZeileSichtbar() && rp().bild < ImGui::GetFrameCount() - 3;
    }));
}

// --- R5: zweites Modell, je Modell bleibt alles --------------------------------------------------------------------------------------
void jeModellSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto a1 = std::make_shared<Stand>();
    auto hashA = std::make_shared<std::uint64_t>(0);
    auto hashB = std::make_shared<std::uint64_t>(0);
    auto bemalt = std::make_shared<int>(-1);
    for (Schritt& x : neuerReiter(kJedi)) { add(std::move(x)); }
    add(pruefe("AP11 R5: das zweite Modell steht in einem neuen Reiter \"jedi_hm/model.glm\", er ist aktiv, der erste bleibt", [] {
        const ModellReiterInfo i0 = modellReiterInfo(0);
        const ModellReiterInfo i1 = modellReiterInfo(1);
        melde("R5 zwei: " + std::to_string(modellReiterZahl()) + " Reiter, aktiv " + std::to_string(modellReiterAktiv()) + ", '" + i0.titel + "' '" + i1.titel + "', Modell " +
              g_app->modelPath);
        return modellReiterZahl() == 2 && modellReiterAktiv() == 1 && i0.titel == "kyle/model.glm" && i1.titel == "jedi_hm/model.glm" && g_app->modelPath == kJedi &&
               rp().reiter.size() == 2U;
    }));
    add(stimmigPruefen("nach dem Oeffnen im neuen Reiter"));
    add(tu("AP11 R5: Bild des zweiten Modells", [hashB] { *hashB = ansichtHash(); }));
    add(klickAn("AP11 R5: Klick auf den Reiter kyle/model.glm", reiterOrt(0)));
    add(pause(0.6));
    add(pruefe("AP11 R5: der Klick wechselt zum ersten Modell", [] {
        return modellReiterZahl() == 2 && modellReiterAktiv() == 0 && g_app->modelPath == kKyle && mtab().dokModell == kKyle;
    }));
    add(stimmigPruefen("nach dem Wechsel per Klick"));
    // den Stand des ersten Modells veraendern: Auswahl, Stapel (Rueckgaengig), Control Rig, Einfrieren, Sperren, Skin, Kamera, Ansichten, Animation
    add(tu("AP11 R5: kyle - Stapel, Rig, Einfrieren, Sperren, Skin aus, Kamera, vier Ansichten, Animation", [] {
        ModellTab& t = mtab();
        modusWaehlen(0, ModellBereich::Animation);
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animFrame = g_app->animList[uz(g_app->animIndex)].firstFrame + 3;
        if (!modellRigDa()) { (void)modellAnimRigErstellen(false); }
        t.bereich = ModellBereich::Modellieren;
        t.auswahl = {teil("torso")};
        (void)modellStapelHinzufuegen(modell::ModArt::Spiegeln);
        modellTeilEinfrieren(teil("head"), true, false);
        t.bereich = ModellBereich::Weights;
    }));
    add(pause(0.5));
    add(tu("AP11 R5: kyle - Knochen sperren, Skin aus, Kamera und Ansichten", [] {
        ModellTab& t = mtab();
        modellGewichteSperren(1, 0);
        if (!modellEnt3aSkinAus()) { modellEnt3aSkinSchalten(); }
        t.bereich = ModellBereich::Ansehen;
        g_app->modelCam.angles[1] += 37.0F;
        g_app->modelOrbit.distance *= 1.3F;
        t.vierAnsichten = true;
        t.aktiv = 0;
        t.ansichten[0].art = AnsichtArt::Vorne;
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(tu("AP11 R5: kyle - Painting, Pinsel, Gruppe torso", [] {
        ModellTab& t = mtab();
        t.bereich = ModellBereich::Painting;
        t.vierAnsichten = false;
        t.aktiv = 1;
        t.ansichten[1].art = AnsichtArt::Vorne;
        t.malen.editorOffen = true;
        t.malen.radiusPx = 18.0F;
        t.malen.pinsel.staerke = 1.0F;
        t.malen.pinsel.farbe = {1.0F, 0.0F, 0.0F, 1.0F};
        t.auswahl = {teil("torso")};
        t.malen.gruppe = -1;
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(pt1c::werkzeug(Str::PtWzPinsel));
    add(pause(0.3));
    add(tu("AP11 R5: kyle - die Ansicht von vorne einpassen", [] { modellAnsichtEinpassenAktiv(); }));
    add(pause(0.6));
    add(mb3::ziehenVonNach("AP11 R5: kyle - Strich ueber die Brust", [] { return pt1c::torsoMitte(-30.0F); }, [] { return pt1c::torsoMitte(30.0F); }));
    add(pause(0.4));
    add(tu("AP11 R5: kyle - zurueck nach Ansehen/Pruefen, Auswahl torso", [bemalt] {
        ModellTab& t = mtab();
        *bemalt = pt1c::geaendert(pt1c::lw());
        t.bereich = ModellBereich::Ansehen;
        t.auswahl = {teil("torso")};
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(tu("AP11 R5: kyle - vier Ansichten (Alt+W), aktiv oben", [] {
        modellPr().vierJeBereich[0] = 1;
        mtab().vierAnsichten = true;
        mtab().aktiv = 0;
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(tu("AP11 R5: kyle - Stand und Bild merken", [a1, hashA, bemalt] {
        *a1 = standJetzt();
        a1->bemalt = *bemalt;
        *hashA = ansichtHash();
        melde("R5 Stand kyle: " + text(*a1) + ", bemalt " + std::to_string(*bemalt));
    }));
    add(pruefe("AP11 R5: kyle - der Stand ist wirklich veraendert (Rueckgaengig, Stapel, Rig, Frost, Sperren, Skin, Bemalung)", [a1] {
        return !a1->undo.empty() && a1->stapelTorso == 1U && a1->rig && a1->frostKopf && a1->sperren > 0 && a1->skinAus && a1->malUngesp && a1->bemalt > 50 && a1->vier;
    }));
    add(klickAn("AP11 R5: Klick auf den Reiter jedi_hm/model.glm", reiterOrt(1)));
    add(pause(0.8));
    add(pruefe("AP11 R5: im zweiten Reiter nichts vom ersten: keine Auswahl, kein Rueckgaengig, kein Stapel, kein Rig, nichts eingefroren, keine Sperren, Skin an, "
               "eine Ansicht, nicht bemalt",
               [a1] {
                   const Stand b = standJetzt();
                   melde("R5 Stand jedi_hm: " + text(b));
                   return modellReiterAktiv() == 1 && b.pfad == kJedi && b.auswahl.empty() && b.undo.empty() && b.stapelTorso == 0U && !b.rig && !b.frostKopf && b.sperren == 0 &&
                          !b.skinAus && !b.vier && !b.malUngesp && b.winkel != a1->winkel;
               }));
    add(stimmigPruefen("im zweiten Reiter"));
    add(pruefe("AP11 R5: die Ansicht zeigt das zweite Modell - Bild wie vor dem Wechsel, anders als das erste", [hashA, hashB] {
        const std::uint64_t h = ansichtHash();
        melde("R5 Bild jedi_hm " + std::to_string(h) + " (vorher " + std::to_string(*hashB) + ", kyle " + std::to_string(*hashA) + ")");
        return h != 0 && h == *hashB && h != *hashA;
    }));
    add(klickAn("AP11 R5: zurueck auf kyle/model.glm", reiterOrt(0)));
    add(pause(0.8));
    add(pruefe("AP11 R5: zurueck im ersten Reiter ist alles wie vorher (Datei, Animation und Bild, Auswahl, Rueckgaengig, Stapel, Rig, Frost, Sperren, Skin, Kamera, "
               "Ansichten, Bemalung)",
               [a1] {
                   Stand a = standJetzt();
                   mtab().malen.gruppe = -1;
                   a.bemalt = pt1c::geaendert(pt1c::lw());
                   melde("R5 Stand kyle zurueck: " + text(a) + ", bemalt " + std::to_string(a.bemalt));
                   return modellReiterZahl() == 2 && rp().wechsel >= 3 && gleich(a, *a1) && a.bemalt == a1->bemalt;
               }));
    add(stimmigPruefen("zurueck im ersten Reiter"));
    add(pause(0.4));
    add(pruefe("AP11 R5: das Bild der Ansicht ist nach Hin- und Rueckwechsel bitgleich", [hashA] {
        const std::uint64_t h = ansichtHash();
        melde("R5 Bild kyle " + std::to_string(h) + " (vorher " + std::to_string(*hashA) + ")");
        return rp().wechsel >= 3 && h != 0 && h == *hashA;
    }));
    // Rueckgaengig je Modell
    add(klickAn("AP11 R5: zum zweiten Reiter", reiterOrt(1)));
    add(pause(0.6));
    add(tu("AP11 R5: jedi_hm - Glaetten auf eine Flaeche (ein Schritt)", [] {
        mtab().bereich = ModellBereich::Modellieren;
        mtab().auswahl = {flaecheOder("torso")};
        (void)modellStapelHinzufuegen(modell::ModArt::Glaetten);
    }));
    add(pause(0.4));
    add(pruefe("AP11 R5: jedi_hm hat jetzt einen Schritt", [] {
        const int f = flaecheOder("torso");
        return mtab().dok.kannRueckgaengig() && f >= 0 && mtab().dok.inhalt().teile[uz(f)].stapel.size() == 1U;
    }));
    add(klickAn("AP11 R5: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.4));
    add(pruefe("AP11 R5: Strg+Z nimmt nur im zweiten Modell zurueck", [] {
        return !mtab().dok.kannRueckgaengig() && mtab().dok.inhalt().teile[uz(flaecheOder("torso"))].stapel.empty();
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z (nichts mehr)"));
    add(pause(0.3));
    add(klickAn("AP11 R5: zum ersten Reiter", reiterOrt(0)));
    add(pause(0.6));
    add(pruefe("AP11 R5: im ersten Modell steht sein Rueckgaengig-Verlauf unveraendert", [a1] {
        melde("R5 Undo kyle: '" + mtab().dok.rueckgaengigName() + "' (vorher '" + a1->undo + "')");
        return mtab().dok.rueckgaengigName() == a1->undo && mtab().dok.inhalt().teile[uz(teil("torso"))].stapel.size() == 1U;
    }));
    add(tu("AP11 R5: kyle - Skin wieder an, auftauen, entsperren, eine Ansicht", [] {
        if (modellEnt3aSkinAus()) { modellEnt3aSkinSchalten(); }
        modellAllesAuftauen();
        modellGewichteSperren(2, 0);
        modellPr().vierJeBereich[0] = 0;
        mtab().vierAnsichten = false;
        mtab().aktiv = 3;
    }));
    add(pause(0.3));
}

// --- R5: Schliessen, Beenden, schon offen ----------------------------------------------------------------------------------------------
void schliessenSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("AP11 R5: zweiter Reiter aktiv (kyle ist ungespeichert)", [] { modellReiterWechseln(1); }));
    add(pause(0.5));
    add(klickAn("AP11 R5: x am Reiter kyle/model.glm", reiterX(0)));
    add(pause(0.6));
    add(pruefe("AP11 R5: das x fragt nach dem Verwerfen - der Reiter wird dafuer aktiv, der Model-Tab ist vorn", [] {
        melde("R5 Schliessen: Frage " + (g_app->frageOffen ? g_app->frageText : std::string("keine")) + ", aktiv " + std::to_string(modellReiterAktiv()));
        return g_app->frageOffen && g_app->frageText.find("kyle/model.glm") != std::string::npos && modellReiterAktiv() == 0 && g_app->modelPath == kKyle && g_app->leftMode == 2;
    }));
    add(antwort(Str::AnswerNo));
    add(pause(0.5));
    add(pruefe("AP11 R5: Nein - beide Reiter bleiben, kyle mit seinen Aenderungen", [] {
        return !g_app->frageOffen && modellReiterZahl() == 2 && modellUngespeichert() && g_app->modelPath == kKyle;
    }));
    add(klickAn("AP11 R5: noch einmal x am Reiter kyle/model.glm", reiterX(0)));
    add(pause(0.6));
    add(antwort(Str::AnswerYes));
    add(pause(0.8));
    add(pruefe("AP11 R5: Ja - der Reiter ist zu, jedi_hm ist aktiv", [] {
        melde("R5 nach Ja: " + std::to_string(modellReiterZahl()) + " Reiter, " + g_app->modelPath);
        return !g_app->frageOffen && modellReiterZahl() == 1 && g_app->modelPath == kJedi && modellReiterInfo(0).titel == "jedi_hm/model.glm";
    }));
    add(stimmigPruefen("nach dem Schliessen"));
    // ein schon offenes Modell nicht doppelt
    for (Schritt& x : neuerReiter(kKyle)) { add(std::move(x)); }
    add(tu("AP11 R5: noch einmal jedi_hm in neuem Reiter", [] { modellInNeuemReiterAusArchiv(kJedi); }));
    add(pause(0.8));
    add(pruefe("AP11 R5: ein schon offenes Modell oeffnet nicht doppelt - der Reiter wechselt dorthin, die Meldung sagt es", [] {
        melde("R5 schon offen: " + std::to_string(modellReiterZahl()) + " Reiter, aktiv " + std::to_string(modellReiterAktiv()) + ", Meldung '" + mtab().meldung + "'");
        return modellReiterZahl() == 2 && g_app->modelPath == kJedi && mtab().meldung == fmt(Str::MxSchonOffen, "jedi_hm/model.glm");
    }));
    // Beenden fragt je Modell
    add(tu("AP11 R5: beide Modelle ungespeichert (je ein Stapel-Schritt), Beenden ueber das Kreuz", [] {
        mtab().auswahl = {flaecheOder("torso")};
        (void)modellStapelHinzufuegen(modell::ModArt::Spiegeln);
    }));
    add(klickAn("AP11 R5: zum Reiter kyle", reiterOrt(1)));
    add(pause(0.5));
    add(tu("AP11 R5: kyle ein Schritt; Kreuz", [] {
        mtab().auswahl = {teil("torso")};
        (void)modellStapelHinzufuegen(modell::ModArt::Spiegeln);
        g_app->settings.queryOnDiscard = false;
        g_app->quitConfirmed = false;
        g_app->wantQuit = false;
        (void)confirmQuit();
    }));
    add(pause(0.4));
    add(pruefe("AP11 R5: das Kreuz fragt zuerst nach dem aktiven Modell (kyle)", [] {
        return g_app->frageOffen && g_app->frageText.find("kyle/model.glm") != std::string::npos;
    }));
    add(antwort(Str::AnswerNo));
    add(pause(0.4));
    add(tu("AP11 R5: kyle als beantwortet, Kreuz noch einmal", [] {
        mtab().beendenBeantwortet = true;
        (void)confirmQuit();
    }));
    add(pause(0.6));
    add(pruefe("AP11 R5: dann nach dem anderen ungespeicherten Modell (jedi_hm) - sein Reiter wird dafuer aktiv", [] {
        melde("R5 Beenden: Frage " + (g_app->frageOffen ? g_app->frageText : std::string("keine")) + ", aktiv " + g_app->modelPath);
        return g_app->frageOffen && g_app->frageText.find("jedi_hm/model.glm") != std::string::npos && g_app->modelPath == kJedi && !g_app->wantQuit;
    }));
    add(antwort(Str::AnswerNo));
    add(pause(0.4));
    add(pruefe("AP11 R5: Nein - kein Beenden; die Antwort fuer kyle ist vergessen (beim naechsten Kreuz fragt es wieder)", [] {
        const int k = modellReiterAktiv() == 0 ? 1 : 0;
        (void)k;
        return !g_app->frageOffen && !g_app->quitConfirmed && !g_app->wantQuit && !mtab().beendenBeantwortet;
    }));
    // der letzte Reiter wird leer
    add(tu("AP11 R5: beide als gespeichert, alle anderen schliessen", [] {
        aufraeumenFrage();
        mtab().gespeichertStand = mtab().dok.stand();
        modellReiterAndereSchliessen(modellReiterAktiv());
    }));
    add(pause(0.6));
    add(pruefe("AP11 R5: \"Andere Reiter schliessen\" fragt nach dem ungespeicherten anderen Modell", [] { return g_app->frageOffen && g_app->frageText.find("kyle") != std::string::npos; }));
    add(antwort(Str::AnswerYes));
    add(pause(0.8));
    add(pruefe("AP11 R5: ein Reiter bleibt", [] { return modellReiterZahl() == 1 && g_app->modelPath == kJedi; }));
    add(tu("AP11 R5: den letzten Reiter schliessen (gespeichert)", [] {
        mtab().gespeichertStand = mtab().dok.stand();
        modellReiterSchliessen(0);
    }));
    add(pause(0.6));
    add(pruefe("AP11 R5: der letzte Reiter verschwindet nicht - er ist leer (\"(kein Modell)\")", [] {
        melde("R5 leer: " + std::to_string(modellReiterZahl()) + " Reiter, '" + modellReiterInfo(0).titel + "', Modell leer " + (g_app->model.empty() ? "ja" : "nein"));
        return modellReiterZahl() == 1 && g_app->model.empty() && modellReiterInfo(0).titel == tr(Str::MxKeinModell) && !mtab().dok.kannRueckgaengig();
    }));
    for (Schritt& x : neuerReiter(kKyle)) { add(std::move(x)); }
    add(pruefe("AP11 R5: in einen leeren Reiter laedt \"In neuem Tab oeffnen\" ohne zweiten Reiter", [] {
        return rp().geschlossen >= 2 && modellReiterZahl() == 1 && g_app->modelPath == kKyle;
    }));
}

// --- R5: Absturzrichtungen --------------------------------------------------------------------------------------------------------------
void absturzSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : neuerReiter(kJedi)) { add(std::move(x)); }
    add(tu("AP11 R5: beide Modelle spielen eine Animation", [] {
        animWaehlen(mbefoto::sequenz(), true);
        modellReiterWechseln(0);
    }));
    add(pause(0.4));
    add(tu("AP11 R5: kyle spielt", [] { animWaehlen(mbefoto::sequenz(), true); }));
    // jedes Bild ein Wechsel, 80 Bilder lang
    add({"AP11 R5: schnell wechseln (jedes Bild), waehrend beide Animationen laufen", [](int b) {
             modellReiterWechseln(b % 2);
             return b >= 80;
         }});
    add(pause(0.5));
    add(pruefe("AP11 R5: schnelles Wechseln - kein Absturz, zwei Reiter, die laufende Animation im gueltigen Bereich", [] {
        const GlaAnimation& a = g_app->anim;
        melde("R5 schnell: " + std::to_string(rp().wechsel) + " Wechsel, Bild " + std::to_string(g_app->animFrame) + " von " + std::to_string(a.bildZahl()));
        return modellReiterZahl() == 2 && g_app->animPlaying && g_app->animFrame >= 0 && g_app->animFrame < std::max(a.bildZahl(), 1);
    }));
    add(stimmigPruefen("nach schnellem Wechseln"));
    // Schliessen mit offenen Fenstern
    add(tu("AP11 R5: jedi_hm aktiv, Auto-Skin, Zeitkonfiguration, Dopesheet und Info offen", [] { modellReiterWechseln(1); }));
    add(pause(0.4));
    add(tu("AP11 R5: Fenster oeffnen", [] {
        g_app->animPlaying = false;
        modellAutoSkinOeffnen();
        modellZeit().konfigOffen = true;
        mtab().kvOffen = true;
        mtab().bereich = ModellBereich::Animation;
    }));
    add(tu("AP11 R5: torso zeigen", [] { modellExplorerZeigen(ExplorerArt::Flaeche, flaecheOder("torso")); }));
    add(pause(0.6));
    add(klickAn("AP11 R5: Rechtsklick auf eine Flaeche (Info)", [] { return ap2::ort(flaechenName("torso"), 0, static_cast<int>(ExplorerArt::Flaeche))(); }, 1));
    add(pause(0.4));
    add(klickPopup(tr(Str::MbXInfo)));
    add(pause(0.4));
    add(tu("AP11 R5: den Reiter mit offenen Fenstern schliessen", [] {
        mtab().gespeichertStand = mtab().dok.stand();
        modellReiterSchliessen(1);
    }));
    add(pause(1.0));
    add(pruefe("AP11 R5: Schliessen mit offenen Fenstern - kein Absturz, kyle aktiv, die Fenster zeigen kyle", [] {
        std::string w;
        const bool ok = stimmig(w);
        melde("R5 Fenster: " + std::to_string(modellReiterZahl()) + " Reiter, " + g_app->modelPath + ", Auto-Skin " + (modellAutoSkinZustand().fensterSichtbar ? "sichtbar" : "zu") +
              ", " + (ok ? std::string("stimmig") : w));
        return rp().geschlossen >= 3 && modellReiterZahl() == 1 && g_app->modelPath == kKyle && ok;
    }));
    add(tu("AP11 R5: Fenster zu", [] {
        modellAutoSkinZustand().fensterOffen = false;
        modellZeit().konfigOffen = false;
        mtab().kvOffen = false;
        mtab().bereich = ModellBereich::Ansehen;
        g_app->animPlaying = false;
    }));
    add(mbefoto::popupsZu());
    // Wechsel mitten im Gewichte-Strich
    for (Schritt& x : neuerReiter(kJedi)) { add(std::move(x)); }
    add(klickAn("AP11 R5: zu kyle", reiterOrt(0)));
    add(pause(0.5));
    add(tu("AP11 R5: kyle - Weights, Strich beginnt (Taste bleibt gedrueckt), dann Wechsel angefordert", [] {
        mtab().bereich = ModellBereich::Weights;
        mtab().knochenWahl = 1;
        ImVec2 m;
        if (!modellFlaecheAufSchirm(teil("torso"), m)) { meldeFehler("AP11: torso nicht auf dem Schirm"); }
        (void)modellTupfer(mtab().aktiv, m, true);
        modellReiterWechseln(1);
    }));
    add(pause(0.5));
    add(pruefe("AP11 R5: Wechsel mitten im Strich - im neuen Reiter laeuft kein Strich, kein fremder Schritt", [] {
        return modellReiterAktiv() == 1 && !mtab().malStrich && !mtab().dok.kannRueckgaengig();
    }));
    add(klickAn("AP11 R5: zurueck zu kyle", reiterOrt(0)));
    add(pause(0.5));
    add(pruefe("AP11 R5: der Strich ist im alten Modell beendet (sein Schritt liegt dort oben)", [] {
        melde("R5 Strich: Undo '" + mtab().dok.rueckgaengigName() + "', malStrich " + std::to_string(mtab().malStrich ? 1 : 0));
        return modellReiterZahl() == 2 && modellReiterAktiv() == 0 && !mtab().malStrich && mtab().dok.rueckgaengigName() == tr(Str::MbUndoMalen);
    }));
    add(tu("AP11 R5: Strich zuruecknehmen, Ansehen", [] {
        (void)modellRueckgaengig();
        mtab().bereich = ModellBereich::Ansehen;
        mtab().knochenWahl = -1;
    }));
    add(pause(0.3));
}

// --- R5 + Codepruefung seit rc611 R1: Geraeteverlust mit mehreren offenen Modellen (Hinweis Hauptchat 13.11.) --------------------------
// Nach dem Verlust zeigt die aktive Ansicht dasselbe Bild wie vorher - und ebenso das Modell, das waehrenddessen in einem anderen
// Reiter lag (seine Texturen und Puffer kommen beim Wechsel aus seinem Fach neu auf die Grafikkarte). Wie R1: hoechstens 1 % der
// Bildpunkte ueber 8/255 anders, die Schnittstelle laeuft weiter, der Abgeber meldet keinen Fehler.
struct ModellBild {
    std::vector<std::uint8_t> rgba;
    int w = 0;
    int h = 0;
};

bool modellBildLesen(ModellBild& b) {
    gpu::modellZielWaehlen(mtab().aktiv);
    return gpu::leseModellZiel(b.rgba, b.w, b.h);
}

long long modellBildAnders(const ModellBild& a, const ModellBild& b) {
    if (a.rgba.empty() || a.w != b.w || a.h != b.h || a.rgba.size() != b.rgba.size()) { return -1; }
    long long n = 0;
    for (std::size_t i = 0; i + 3U < a.rgba.size(); i += 4U) {
        for (std::size_t k = 0; k < 3U; ++k) {
            if (std::abs(static_cast<int>(a.rgba[i + k]) - static_cast<int>(b.rgba[i + k])) > 8) {
                ++n;
                break;
            }
        }
    }
    return n;
}

void verlustSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto bild = std::make_shared<std::array<ModellBild, 2>>();
    auto aufbauten = std::make_shared<int>(0);
    add(tu("AP11 R1: kyle (Reiter 1) und jedi_hm (Reiter 2) offen, Ansehen/Pruefen, eine Ansicht", [] {
        modellReiterWechseln(1);
        modusWaehlen(0, ModellBereich::Ansehen);
    }));
    add(pause(0.8));
    add(tu("AP11 R1: Bild von jedi_hm", [bild] { (void)modellBildLesen((*bild)[1]); }));
    add(klickAn("AP11 R1: zu kyle", reiterOrt(0)));
    add(pause(0.8));
    add(tu("AP11 R1: Bild von kyle, Geraeteverlust", [bild, aufbauten] {
        (void)modellBildLesen((*bild)[0]);
        *aufbauten = g_app->grafikNeuAufbauten;
        render::geraetVerlorenSpielen();
    }));
    add(warteBis("AP11 R1: Grafik neu aufgebaut", [aufbauten] { return g_app->grafikNeuAufbauten > *aufbauten; }, 300));
    add(pause(1.5));
    add(pruefe("AP11 R1: nach dem Geraeteverlust zeigt die aktive Ansicht (kyle) dasselbe Bild, die Schnittstelle laeuft, kein Fehler des Abgebers", [bild] {
        ModellBild nach;
        const bool gelesen = modellBildLesen(nach);
        const long long anders = gelesen ? modellBildAnders((*bild)[0], nach) : -1;
        std::string text;
        const bool ok = render::grafikBefund(text);
        melde("R1 kyle: " + std::to_string((*bild)[0].w) + " x " + std::to_string((*bild)[0].h) + ", " + std::to_string(anders) + " Bildpunkte anders, " +
              render::backendName(render::current()) + (text.empty() ? std::string() : " | " + text));
        return ok && anders >= 0 && anders * 100 <= static_cast<long long>((*bild)[0].w) * (*bild)[0].h && render::d3dDevice() != nullptr;
    }));
    add(klickAn("AP11 R1: zu jedi_hm (lag waehrend des Verlusts im Fach)", reiterOrt(1)));
    add(pause(1.5));
    add(pruefe("AP11 R1: das Modell aus dem anderen Reiter zeigt nach dem Verlust dasselbe Bild wie vorher", [bild] {
        ModellBild nach;
        const bool gelesen = modellBildLesen(nach);
        const long long anders = gelesen ? modellBildAnders((*bild)[1], nach) : -1;
        std::string text;
        const bool ok = render::grafikBefund(text);
        melde("R1 jedi_hm: " + std::to_string((*bild)[1].w) + " x " + std::to_string((*bild)[1].h) + ", " + std::to_string(anders) + " Bildpunkte anders" +
              (text.empty() ? std::string() : " | " + text));
        return ok && modellReiterAktiv() == 1 && anders >= 0 && anders * 100 <= static_cast<long long>((*bild)[1].w) * (*bild)[1].h;
    }));
    add(stimmigPruefen("nach dem Geraeteverlust"));
    add(klickAn("AP11 R1: zurueck zu kyle", reiterOrt(0)));
    add(pause(0.5));
}

// --- X19: Kopieren und Einfuegen ---------------------------------------------------------------------------------------------------------
struct Quelle {
    std::string name1;
    std::string name2;
    std::string elternName;
    std::string kopfName;
    modell::Teil torso;
    std::vector<std::string> knochen;
    std::string bild;
    std::size_t teileZiel = 0;
    std::size_t teileQuelle = 0;
    std::uint64_t bildVorher = 0;
};

void ablageSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto q = std::make_shared<Quelle>();
    add(tu("AP11 X19: kyle aktiv, wie 3ds Max, Ansehen/Pruefen, torso im Baum", [] {
        modellReiterWechseln(0);
        modusWaehlen(0, ModellBereich::Ansehen);
    }));
    add(pause(0.5));
    add(tu("AP11 X19: torso zeigen", [q] {
        modellExplorerZeigen(ExplorerArt::Flaeche, teil("torso"));
        const modell::Inhalt& in = mtab().dok.inhalt();
        q->torso = in.teile[uz(teil("torso"))];
        q->knochen.clear();
        for (const GlaBone& b : g_app->anim.bones) { q->knochen.push_back(b.name); }
        const GlmSurface& f = g_app->model.surfaces[uz(teil("torso"))];
        q->bild = f.texture.empty() ? f.shader : f.texture;
        q->teileQuelle = in.teile.size();
    }));
    add(pause(0.6));
    add(klickAn("AP11 X19: Rechtsklick auf torso", ap2::ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche)), 1));
    add(pause(0.5));
    add(pruefe("AP11 X19: der Rechtsklick hat Info, Kopieren, Einfuegen als Kind hiervon, Status, Wurzel loesen - in der Reihenfolge des Entwurfs", [] {
        const Element* info = imPopup(tr(Str::MbXInfo));
        const Element* kop = imPopup(tr(Str::MxKopieren));
        const Element* ein = imPopup(tr(Str::MxEinfuegenKind));
        const Element* an = imPopup(tr(Str::MbFlAn));
        const Element* wl = imPopup(tr(Str::MbXWurzelLoesen));
        melde(std::string("X19 Menue: Info ") + (info ? "ja" : "nein") + ", Kopieren " + (kop ? "ja" : "nein") + ", Einfuegen " + (ein ? "ja" : "nein") + ", Wurzel loesen " +
              (wl ? "ja" : "nein"));
        return info && kop && ein && an && wl && info->rect.Min.y < kop->rect.Min.y && kop->rect.Min.y < ein->rect.Min.y && ein->rect.Min.y < an->rect.Min.y &&
               an->rect.Min.y < wl->rect.Min.y;
    }));
    add(klickPopup(tr(Str::MxKopieren)));
    add(pause(0.5));
    add(pruefe("AP11 X19: Kopieren fuellt die Ablage; rechts in der Reiterzeile \"Ablage: torso aus kyle/model.glm - Strg+V fuegt ein\"", [] {
        const ModellReiterProbe& p = rp();
        const std::string soll = fmt(Str::MxAblageHinweis, "torso", "kyle/model.glm");
        melde("X19 Hinweis: '" + p.hinweisText + "' (soll '" + soll + "'), x " + std::to_string(static_cast<int>(p.hinweis.Max.x)) + " / Zeile " +
              std::to_string(static_cast<int>(p.zeile.Max.x)));
        return modellAblageDa() && p.hinweisText == soll && !p.hinweisGekuerzt && p.hinweis.Max.x <= p.zeile.Max.x + 1.0F &&
               p.hinweis.Max.x >= p.zeile.Max.x - ImGui::GetStyle().ItemSpacing.x * 3.0F && p.hinweis.Min.y >= p.zeile.Min.y - 1.0F && p.hinweis.Max.y <= p.zeile.Max.y + 1.0F &&
               (p.reiter.empty() || p.reiter.back().second.Max.x < p.hinweis.Min.x) && mtab().meldung == fmt(Str::MxKopiert, "torso", "kyle/model.glm");
    }));
    // in das andere Modell als Kind von hips
    add(klickAn("AP11 X19: zum Reiter jedi_hm", reiterOrt(1)));
    add(pause(0.6));
    add(tu("AP11 X19: jedi_hm - hips zeigen", [q] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        q->elternName = in.teile[uz(flaecheOder("hips"))].name;
        q->kopfName = in.teile[uz(flaecheOder("head"))].name;
        q->name1 = kopieName("torso", 1);
        q->name2 = kopieName("torso", 2);
        modellExplorerZeigen(ExplorerArt::Flaeche, teil(q->elternName.c_str()));
        q->teileZiel = in.teile.size();
        q->bildVorher = ansichtHash();
        melde("X19 Ziel: Eltern " + q->elternName + ", Kopf " + q->kopfName + ", erwartet " + q->name1 + " / " + q->name2);
    }));
    add(pause(0.6));
    add(klickAn("AP11 X19: Rechtsklick auf den Eltern (hips)", [q] { return ap2::ort(q->elternName, 0, static_cast<int>(ExplorerArt::Flaeche))(); }, 1));
    add(pause(0.5));
    add(klickPopup(tr(Str::MxEinfuegenKind)));
    add(pause(0.8));
    add(pruefe("AP11 X19: eingefuegt als Kind von hips - neuer Name (torso_1, wenn es torso schon gibt), alle Stufen des Ziels, Shader der Quelle, ein Rueckgaengig-Schritt, gewaehlt", [q] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        const int n = in.teilIndex(q->name1);
        melde("X19 eingefuegt: Teile " + std::to_string(in.teile.size()) + " (vorher " + std::to_string(q->teileZiel) + "), torso_1 " + std::to_string(n) + ", Meldung '" +
              mtab().meldung.substr(0, 300) + "'");
        if (n < 0) { return false; }
        const modell::Teil& t = in.teile[uz(n)];
        return in.teile.size() == q->teileZiel + 1U && t.eltern == teil(q->elternName.c_str()) && t.stufen.size() == uz(std::max(in.stufenZahl, 1)) && t.shader == q->bild.substr(0, 63) &&
               mtab().dok.rueckgaengigName() == tr(Str::MxUndoEinfuegen) && mtab().auswahl == std::vector<int>{n} &&
               std::count(in.teile[uz(teil(q->elternName.c_str()))].kinder.begin(), in.teile[uz(teil(q->elternName.c_str()))].kinder.end(), n) == 1;
    }));
    add(pruefe("AP11 X19: die Gewichte gehen an die gleichnamigen Knochen des Ziels (jede Ecke der Stufe 0 nennt dieselben Namen mit denselben Gewichten)", [q] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        const int n = in.teilIndex(q->name1);
        if (n < 0 || q->torso.stufen.empty()) { return false; }
        std::vector<std::string> ziel;
        for (const GlaBone& b : g_app->anim.bones) { ziel.push_back(b.name); }
        const modell::Netz& a = q->torso.stufen[0];
        const modell::Netz& b = in.teile[uz(n)].stufen[0];
        if (a.ecken.size() != b.ecken.size()) { return false; }
        int falsch = 0;
        for (std::size_t e = 0; e < a.ecken.size(); ++e) {
            std::map<std::string, float> x;
            std::map<std::string, float> y;
            for (int k = 0; k < 4; ++k) {
                if (a.ecken[e].gewicht[k] > 0.0F && a.ecken[e].knochen[k] < q->knochen.size()) { x[q->knochen[a.ecken[e].knochen[k]]] += a.ecken[e].gewicht[k]; }
                if (b.ecken[e].gewicht[k] > 0.0F && b.ecken[e].knochen[k] < ziel.size()) { y[ziel[b.ecken[e].knochen[k]]] += b.ecken[e].gewicht[k]; }
            }
            bool g = x.size() == y.size();
            for (const auto& [k, v] : x) { g = g && y.count(k) == 1U && std::fabs(y[k] - v) < 1e-5F; }
            falsch += g ? 0 : 1;
        }
        melde("X19 Gewichte: " + std::to_string(a.ecken.size()) + " Ecken, " + std::to_string(falsch) + " anders");
        return falsch == 0;
    }));
    add(pruefe("AP11 X19: die Meldung nennt Quelle, Eltern und die Grenzen des Zielspiels", [q] {
        const std::string& m = mtab().meldung;
        const std::string anfang = fmt(Str::MxEingefuegt, "torso", "kyle/model.glm", q->elternName.c_str());
        const char* spiel = modell::grenzen(static_cast<modell::Zielspiel>(mtab().zielspiel)).name;
        const std::string ueber = fmt(Str::MxGrenzen, spiel, "");
        melde("X19 Meldung: '" + m.substr(0, 400) + "'");
        return m.rfind(anfang, 0) == 0 && (m.find(fmt(Str::MxGrenzenOk, spiel)) != std::string::npos || m.find(ueber) != std::string::npos);
    }));
    add(pause(0.6));
    add(stimmigPruefen("nach dem Einfuegen"));
    add(pruefe("AP11 X19: die Ansicht zeigt die eingefuegte Flaeche (Bild anders als vorher)", [q] {
        const std::uint64_t h = ansichtHash();
        return h != 0 && h != q->bildVorher;
    }));
    // Strg+V mit gewaehlter Flaeche
    add(tu("AP11 X19: head waehlen", [q] {
        mtab().auswahl = {teil(q->kopfName.c_str())};
        g_app->modelDirty = true;
    }));
    add(klickAn("AP11 X19: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(tu("AP11 X19: head waehlen (der Klick in die Ansicht waehlt ab)", [q] { mtab().auswahl = {teil(q->kopfName.c_str())}; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_V, "Strg+V"));
    add(pause(0.6));
    add(pruefe("AP11 X19: Strg+V fuegt als Kind der gewaehlten Flaeche ein (die zweite Kopie unter head)", [q] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        const int n = in.teilIndex(q->name2);
        return n >= 0 && in.teile[uz(n)].eltern == teil(q->kopfName.c_str());
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.5));
    add(pruefe("AP11 X19: zweimal Strg+Z - das Ziel ist wie vorher", [q] {
        return modellReiterAktiv() == 1 && g_app->modelPath == kJedi && mtab().dok.inhalt().teile.size() == q->teileZiel && mtab().dok.inhalt().teilIndex(q->name1) < 0;
    }));
    add(stimmigPruefen("nach Rueckgaengig"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Y, "Strg+Y"));
    add(pause(0.5));
    add(pruefe("AP11 X19: Strg+Y bringt torso_1 mit dem Shader der Quelle wieder (auch in der Anzeige)", [q] {
        const int n = mtab().dok.inhalt().teilIndex(q->name1);
        const bool anzeige = n >= 0 && uz(n) < g_app->model.surfaces.size() && g_app->model.surfaces[uz(n)].texture == q->bild.substr(0, 63);
        melde("X19 Wiederholen: torso_1 " + std::to_string(n) + ", Bild " + (n >= 0 && uz(n) < g_app->model.surfaces.size() ? g_app->model.surfaces[uz(n)].texture : std::string("-")));
        return n >= 0 && anzeige;
    }));
    add(klickAn("AP11 X19: zum Reiter kyle", reiterOrt(0)));
    add(pause(0.5));
    add(pruefe("AP11 X19: die Quelle ist unveraendert", [q] {
        return modellReiterZahl() >= 2 && g_app->modelPath == kKyle && mtab().dok.inhalt().teile.size() == q->teileQuelle && mtab().dok.rueckgaengigName() != tr(Str::MxUndoEinfuegen);
    }));
    // in ein .md3-Requisit: starr
    add(tu("AP11 X19: neuer Reiter, neues Requisit", [] { modellInNeuemReiter({}, [] { (void)modellNeuesRequisit(); }); }));
    add(pause(1.0));
    add(tu("AP11 X19: einfuegen (oberste Ebene)", [] { (void)modellFlaechenEinfuegen(-1); }));
    add(pause(0.6));
    add(pruefe("AP11 X19: in ein .md3-Requisit eingefuegt - starr an Knochen 0, die Meldung sagt es", [] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        const int n = in.teilIndex("torso");
        bool starr = n >= 0 && in.herkunft == modell::Herkunft::Md3;
        if (starr) {
            for (const modell::Ecke& e : in.teile[uz(n)].stufen[0].ecken) { starr = starr && e.knochen[0] == 0 && e.gewicht[0] == 1.0F; }
        }
        melde("X19 Requisit: " + std::to_string(modellReiterZahl()) + " Reiter, Teile " + std::to_string(in.teile.size()) + ", Meldung '" + mtab().meldung.substr(0, 200) + "'");
        return starr && mtab().meldung.find(tr(Str::MxStarrZiel)) != std::string::npos;
    }));
    add(tu("AP11 X19: Requisit als gespeichert, schliessen", [] {
        mtab().gespeichertStand = mtab().dok.stand();
        modellReiterSchliessen(modellReiterAktiv());
    }));
    add(pause(0.6));
    add(tu("AP11 X19: nur kyle bleibt offen, die Ablage ist gefuellt", [] {
        aufraeumenFrage();
        for (int i = modellReiterZahl() - 1; i >= 0; --i) {
            if (modellReiterInfo(i).pfad != kKyle) { modellReiterSchliessen(i); }
        }
    }));
    add({"AP11 X19: Fragen nach dem Verwerfen mit Ja", [](int b) {
             if (g_app->frageOffen) {
                 const std::function<void()> ja = g_app->frageJa;
                 aufraeumenFrage();
                 if (ja) { ja(); }
             }
             return (modellReiterZahl() == 1 && !g_app->frageOffen) || b > 300;
         }});
    add(pause(0.6));
    add(pruefe("AP11 X19: mit einem Modell und gefuellter Ablage steht die Zeile - ein Reiter und rechts der Hinweis", [] {
        const ModellReiterProbe& p = rp();
        return modellReiterZahl() == 1 && modellReiterZeileSichtbar() && p.bild >= ImGui::GetFrameCount() - 3 && p.reiter.size() == 1U &&
               p.hinweisText == fmt(Str::MxAblageHinweis, "torso", "kyle/model.glm");
    }));
    // Suche
    add(tu("AP11 X19: Alle Aktionen suchen", [] { mtab().aktionenOffen = true; }));
    add(pause(0.5));
    add(tippe(tr(Str::MxKopieren)));
    add(pause(0.5));
    add(pruefe("AP11 X19: \"Alle Aktionen suchen\" findet Kopieren", [] { return mtab().aktionenSichtbar && imPopup(tr(Str::MxKopieren)) != nullptr; }));
    add(mbefoto::popupsZu());
    add(tu("AP11 X19: Alle Aktionen suchen", [] { mtab().aktionenOffen = true; }));
    add(pause(0.5));
    add(tippe(tr(Str::MxNeuerTab)));
    add(pause(0.5));
    add(pruefe("AP11 P1: \"Alle Aktionen suchen\" findet \"In neuem Tab oeffnen\"", [] { return mtab().aktionenSichtbar && imPopup(tr(Str::MxNeuerTab)) != nullptr; }));
    add(mbefoto::popupsZu());
}

// --- P1: Gruppe Modell, Strg+O ---------------------------------------------------------------------------------------------------------
void p1Schritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto datei = std::make_shared<std::string>();
    for (int modus = 0; modus < 2; ++modus) {
        const std::string wie = modus == 1 ? " wie Blender: " : " wie 3ds Max: ";
        add(tu("AP11 P1:" + wie + "kyle, Ansehen/Pruefen, Gruppe Modell oben", [modus] {
            modellReiterWechseln(0);
            modusWaehlen(modus, ModellBereich::Ansehen);
            if (ImGuiWindow* fw = mbefoto::kindFenster("mtrollouts")) { ImGui::SetScrollY(fw, 0.0F); }
        }));
        add(pause(0.8));
        add(pruefe("AP11 P1" + wie + "die Gruppe Modell hat \"In neuem Tab oeffnen\" neben \"Importieren ...\"", [] {
            const Element* neu = finde(tr(Str::MxNeuerTab), "mtrollouts", 0, true);
            const Element* imp = finde(tr(Str::PqImport), "mtrollouts", 0, true);
            melde(std::string("P1: neu ") + (neu ? "ja" : "nein") + ", Importieren " + (imp ? "ja" : "nein"));
            return neu != nullptr && imp != nullptr && std::fabs(neu->rect.GetCenter().y - imp->rect.GetCenter().y) < 2.0F && neu->rect.Min.x > imp->rect.Min.x;
        }));
        add(klickAn("AP11 P1: Klick auf In neuem Tab oeffnen", [] {
            const Element* e = finde(tr(Str::MxNeuerTab), "mtrollouts", 0, true);
            return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
        }));
        add(pause(0.5));
        add(pruefe("AP11 P1" + wie + "der Knopf oeffnet die Wahl Modelldatei / Aus den Spieldateien", [] {
            return imPopup(tr(Str::MxNeuerTabDatei)) != nullptr && imPopup(tr(Str::MxNeuerTabPk3)) != nullptr;
        }));
        add(mbefoto::popupsZu());
    }
    add(tu("AP11 P1: wie 3ds Max; eine Kopie von kyle auf die Platte", [datei] {
        modusWaehlen(0, ModellBereich::Ansehen);
        const std::string ordner = std::filesystem::current_path().string() + "\\ap11";
        std::error_code ec;
        std::filesystem::create_directories(ordner, ec);
        *datei = ordner + "\\kopie.glm";
        std::string bytes;
        if (!modell::glmSchreiben(mtab().dok.inhalt(), bytes)) { meldeFehler("AP11 P1: glmSchreiben"); }
        std::ofstream(*datei, std::ios::binary).write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        platform::dialogAntwortImSelbsttest(*datei);
    }));
    auto zahl = std::make_shared<int>(0);
    add(klickAn("AP11 P1: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(tu("AP11 P1: Zahl der Reiter", [zahl] { *zahl = modellReiterZahl(); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_O, "Strg+O"));
    add(pause(1.5));
    add(pruefe("AP11 P1: Strg+O oeffnet die gewaehlte Datei in einem neuen Reiter (\"ap11/kopie.glm\"), kyle bleibt", [datei, zahl] {
        melde("P1 Strg+O: " + std::to_string(modellReiterZahl()) + " Reiter (vorher " + std::to_string(*zahl) + "), aktiv '" + modellReiterInfo(modellReiterAktiv()).titel +
              "', Modell " + g_app->modelPath);
        return modellReiterZahl() == *zahl + 1 && g_app->modelPath == *datei && modellReiterInfo(modellReiterAktiv()).titel == "ap11/kopie.glm" && modellReiterInfo(0).titel == "kyle/model.glm";
    }));
    auto zu = std::make_shared<int>(0);
    add(tu("AP11 P1: den Reiter schliessen", [zu] {
        *zu = rp().geschlossen;
        modellReiterSchliessen(modellReiterAktiv());
    }));
    add(pause(0.6));
    add(pruefe("AP11 P1: der Reiter ist wieder zu", [zu, zahl] { return rp().geschlossen == *zu + 1 && modellReiterZahl() == *zahl && modellReiterInfo(0).titel == "kyle/model.glm"; }));
}

// --- Fotos -------------------------------------------------------------------------------------------------------------------------------
void fotoSchritte(std::vector<Schritt>& s, bool handbuch) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string p = handbuch ? "h_ap11_" : "ap11_";
    for (Schritt& x : neuerReiter(kJedi)) { add(std::move(x)); }
    add(tu("AP11 Foto: kyle aktiv, torso kopiert", [] {
        modellReiterWechseln(0);
    }));
    add(pause(0.5));
    add(tu("AP11 Foto: torso kopieren", [] {
        mtab().auswahl = {teil("torso")};
        (void)modellFlaechenKopieren();
        mtab().meldung.clear();
    }));
    for (int modus = 0; modus < 2; ++modus) {
        const std::string m = modus == 1 ? "blender" : "max";
        add(tu("AP11 Foto: " + m + ", Ansehen/Pruefen, torso im Baum", [modus] {
            modusWaehlen(modus, ModellBereich::Ansehen);
            mtab().auswahl = {teil("torso")};
            modellExplorerZeigen(ExplorerArt::Flaeche, teil("torso"));
            if (ImGuiWindow* fw = mbefoto::kindFenster("mtrollouts")) { ImGui::SetScrollY(fw, 0.0F); }
            setzeMaus(ImVec2{-50.0F, -50.0F});
        }));
        add(pause(1.0));
        add(foto(p + m + "_reiter"));
        add(klickAn("AP11 Foto: Rechtsklick auf torso", ap2::ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche)), 1));
        add(pause(0.6));
        add(foto(p + m + "_menue"));
        add(mbefoto::popupsZu());
        if (!handbuch) {
            add(klickAn("AP11 Foto: Klick auf +", [] { return rp().plus.GetCenter(); }));
            add(pause(0.6));
            add(foto(p + m + "_plus"));
            add(mbefoto::popupsZu());
        }
    }
    add(tu("AP11 Foto: wie 3ds Max", [] { modusWaehlen(0, ModellBereich::Ansehen); }));
    add(pause(0.3));
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "ap11" && n != "ap11foto" && n != "ap11hb") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    const char* fenster = std::getenv("BHED_AP11_FENSTER");
    if (fenster != nullptr) { (void)std::sscanf(fenster, "%dx%d@%f", &b, &h, &z); }
    const bool fensterSetzen = n != "ap11hb" || fenster != nullptr;
    auto skala = std::make_shared<float>(1.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP11: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        if (fensterSetzen) {
            (void)platform::hauptfensterGroesse(b, h);
            g_app->uiScale = z;
        }
        if (const char* t = std::getenv("BHED_AP11_THEMA")) {
            const bool hell = std::string(t) == "hell";
            for (const theme::Theme& th : theme::themes()) {
                if ((theme::relativeLuminance(th.window) > 0.5F) == hell) {
                    g_app->activeTheme = &th;
                    g_app->themePending = true;
                    break;
                }
            }
        }
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap11.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden(kKyle)) { add(std::move(x)); }
    add(tu("AP11: Aufteilung wie neue Einstellungen, Sequenz, wie 3ds Max", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        animWaehlen(mbefoto::sequenz(), false);
        char text[300];
        std::snprintf(text, sizeof text, "%s, %zu Teile, Anzeige %.0f x %.0f, Oberflaeche %.2f", g_app->modelPath.c_str(), g_app->mtab.dok.inhalt().teile.size(),
                      static_cast<double>(ImGui::GetIO().DisplaySize.x), static_cast<double>(ImGui::GetIO().DisplaySize.y), static_cast<double>(g_app->uiScale));
        melde(text);
    }));
    add(pause(1.0));
    if (n == "ap11") {
        zeileSchritte(s);
        jeModellSchritte(s);
        schliessenSchritte(s);
        absturzSchritte(s);
        verlustSchritte(s);
        ablageSchritte(s);
        p1Schritte(s);
    } else {
        fotoSchritte(s, n == "ap11hb");
    }
    // die anderen Reiter schliessen; jede Frage nach dem Verwerfen mit Ja
    add({"AP11: andere Reiter schliessen", [](int b) {
             if (g_app->frageOffen) {
                 const std::function<void()> ja = g_app->frageJa;
                 aufraeumenFrage();
                 if (ja) { ja(); }
                 return false;
             }
             if (modellReiterZahl() > 1 && b % 4 == 0) { modellReiterSchliessen(modellReiterAktiv() == 0 ? 1 : 0); }
             return (modellReiterZahl() == 1 && !g_app->frageOffen) || b > 400;
         }});
    add(tu("AP11: Modell-Tab zurueck", [=] {
        aufraeumenFrage();
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        g_app->mtab.knochenWahl = -1;
        g_app->mtab.speicherPfad.clear();
        g_app->mtab.gespeichertStand = g_app->mtab.dok.stand();
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

}  // namespace ap11
