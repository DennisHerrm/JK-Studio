// selbsttest_ap1.inc - AP1 (MBENTWURF Teil B, 11.11.): die Zeitleiste des Model-Tabs nach dem freigegebenen Entwurf
// (BHED_EDITORTEST=modellblock)
//
//   BHED_MODELL_NUR=ap1       je Zeile der Abgleichtabelle (Z1-Z23, R8, A12, A13) Ort, Element und Bedienung, in beiden
//                             Bedienmodi (wie 3ds Max, wie Blender): Fassung je Bereich, Griff, Knopfreihe, Skala mit Strichen
//                             je Frame bzw. Rauten, Schluessel in Skala und Spuren, Bild 0 buendig, Spuren in der Leiste,
//                             Auto Key / Set Key mit der Pose ohne Schluessel, Zeitkonfiguration, Spruenge, Tasten,
//                             Statuszeile; dazu was es vorher gab (Tempo, Schleife, Interpolation, Marker, Haut).
//   BHED_MODELL_NUR=ap1foto   Fotos beider Bedienmodi in allen sechs Bereichen (BHED_FOTOS=<Ordner>) und die Pruefung, dass
//                             in der Sprache und Fenstergroesse des Laufs nichts ueber den Rand der Leiste hinausreicht
//                             (BHED_AP1_FENSTER=3840x2160@1.5, SPRACHE und Thema ueber die Einstellungen des Laufs).
// Eingebunden in selbsttest.cpp (nach selbsttest_mbefoto.inc).
namespace ap1 {

using kv5::f0;
using kv5::rhand;
using kv5::schl;
using kv5::seq;

const ModellZeitProbe& P() { return modellZeit().probe; }
bool leisteDa() { return P().bild >= ImGui::GetFrameCount() - 2; }
ImRect r4(const float r[4]) { return ImRect{r[0], r[1], r[0] + r[2], r[1] + r[3]}; }

const Element* imBand(const std::string& label) {
    for (const Element& e : g_elemente) {
        if (e.label == label && e.innen.find("toolband") != std::string::npos) { return &e; }
    }
    return nullptr;
}

const Element* imPopup(const std::string& label, bool praefix = false) {
    for (const Element& e : g_elemente) {
        const bool passt = praefix ? e.label.rfind(label, 0) == 0 : e.label == label;
        if (passt && (e.fenster.find("##Popup_") != std::string::npos || e.fenster.find("##Menu_") != std::string::npos)) { return &e; }
    }
    return nullptr;
}

bool tooltipDa() {
    for (ImGuiWindow* w : GImGui->Windows) {
        if ((w->Flags & ImGuiWindowFlags_Tooltip) != 0 && w->Active && !w->Hidden) { return true; }
    }
    return false;
}

void melde(const std::string& z) { diag::detail("Knopftest: AP1 " + z); }

// Der Hinweis der Leiste zur Pose ohne Schluessel (Auto Key aus), mit der Taste des Modus
std::string offenText() {
    char b[400];
    std::snprintf(b, sizeof b, tr(Str::MzOffen), modellBlender() ? "I" : "K");
    return b;
}

std::string zahl(float v) {
    char z[32];
    std::snprintf(z, sizeof z, "%.1f", static_cast<double>(v));
    return z;
}

Schritt bereich(ModellBereich b, const char* name) {
    return tu(std::string("AP1: Bereich ") + name, [=] {
        g_app->mtab.bereich = b;
        g_app->modelDirty = true;
    });
}

Schritt fokus() {
    return tu("AP1: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; });
}

Schritt klickBand(const std::string& label) {
    return klickAn("\"" + label + "\" in der Zeitleiste", [label] {
        const Element* e = imBand(label);
        if (e == nullptr) { meldeFehler("AP1: \"" + label + "\" steht nicht in der Zeitleiste"); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    });
}

Schritt klickPopup(const std::string& label) {
    return klickAn("\"" + label + "\" (Menue)", [label] {
        const Element* e = imPopup(label);
        if (e == nullptr) { meldeFehler("AP1: \"" + label + "\" steht nicht im Menue"); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    });
}

// Drei Drehschluessel (rhand, cranium bei +0/+10/+20), ein Bewegungsschluessel (pelvis +5) und einer mit beidem (pelvis +15)
void schluessel() {
    kv5::testSchluessel();
    ModellTab& t = g_app->mtab;
    const std::string name = seq().name;
    const int becken = boneIndex(g_app->anim, "pelvis");
    t.dok.aendern("AP1 Schluessel", [&](modell::Inhalt& in) {
        modell::RigSchicht& s = in.rig.schichten[name];
        BoneMatrix m = BoneMatrix::identity();
        m.m[2][3] = 4.0F;
        modell::rigSchluesselSetzen(s, becken, f0() + 5, m);
        const float ax[3] = {0.0F, 0.0F, 1.0F};
        BoneMatrix d = modell::drehungUm(ax, 20.0F);
        d.m[2][3] = 3.0F;
        modell::rigSchluesselSetzen(s, becken, f0() + 15, d);
    }, false);
    t.kvNurAnimierte = true;
    t.kvNurGewaehlte = false;
    t.kvEinpassen = true;
    g_app->modelDirty = true;
}

void handDrehen() {
    const float w[3] = {0.0F, 0.0F, 30.0F};
    modellRigZiehen(2, w);   // wie das Gizmo: ein Zug, ein Rueckgaengig-Schritt
    modellRigZiehenEnde();
}

void schichtLeer() {
    g_app->mtab.dok.aendern("AP1 leer", [](modell::Inhalt& in) { in.rig.schichten.clear(); }, false);
    g_app->mtab.kvWahl.clear();
    g_app->modelDirty = true;
}

// Reicht ein Element der Leiste ueber ihren Rand (abgeschnitten)? Die Zeilen des Spurbereichs rollen in ihrem eigenen Feld.
bool allesImBand(std::string& was) {
    const ImRect b = r4(P().band);
    for (const Element& e : g_elemente) {
        if (e.innen.find("toolband") == std::string::npos || e.innen.find("zeilen") != std::string::npos || e.rect.GetWidth() <= 0.0F) { continue; }
        if (e.rect.Min.x < b.Min.x - 1.0F || e.rect.Max.x > b.Max.x + 1.0F || e.rect.Min.y < b.Min.y - 1.0F || e.rect.Max.y > b.Max.y + 1.0F) {
            was = e.label + " " + zahl(e.rect.Min.x) + "/" + zahl(e.rect.Min.y) + "-" + zahl(e.rect.Max.x) + "/" + zahl(e.rect.Max.y);
            return false;
        }
    }
    return true;
}

// ENTSCHEID1 (shank 11.11.: "Aus man kann es an machen"): Auto Key ab Werk aus wie in 3ds Max (Auto Key Mode aus) und Blender
// (Auto-Keying aus); die Einstellung modelTimelineAutoKey merkt sich die Wahl des Anwenders. Laeuft vor allen anderen Schritten
// mit den Einstellungen des Laufs (behaved.cfg.sicher kennt den Schluessel nicht: das ist die Vorgabe ab Werk).
void werkVorgabe(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto name = [](const std::string& text) { return "AP1 Z6 ENTSCHEID1 (wie 3ds Max): " + text; };
    const auto ausDatei = [](const Settings& quelle) {
        Settings neu;
        neu.modellZeitAutoKey = !quelle.modellZeitAutoKey;   // so faellt auf, wenn das Lesen den Wert nicht setzt
        const bool ok = readSettings(writeSettings(quelle), neu);
        return ok && neu.modellZeitAutoKey == quelle.modellZeitAutoKey;
    };
    add(tu("AP1 ENTSCHEID1: wie 3ds Max, Ansehen/Pruefen", [] {
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(pruefe(name("Auto Key ist ab Werk aus wie in 3ds Max und Blender (neue Einstellungen, Einstellungsdatei des Laufs ohne den Schluessel, Zeitleiste)"), [] {
        melde("Werk: Settings{} " + std::to_string(Settings{}.modellZeitAutoKey) + ", Lauf " + std::to_string(g_app->settings.modellZeitAutoKey) + ", Zeitleiste " +
              std::to_string(modellZeit().autoKey) + ", Leiste da " + std::to_string(leisteDa()));
        return leisteDa() && !Settings{}.modellZeitAutoKey && !g_app->settings.modellZeitAutoKey && !modellZeit().autoKey;
    }));
    add(klickBand(tr(Str::MzAutoKey)));
    add(pause(0.3));
    add(pruefe(name("ein Klick auf Auto Key schaltet an, die Einstellung merkt es sich und steht so in der Datei"), [ausDatei] {
        melde("an: Zeitleiste " + std::to_string(modellZeit().autoKey) + ", Einstellung " + std::to_string(g_app->settings.modellZeitAutoKey));
        return modellZeit().autoKey && g_app->settings.modellZeitAutoKey && ausDatei(g_app->settings);
    }));
    add(tu("AP1 ENTSCHEID1: wie ein neuer Start - die Zeitleiste liest die Einstellung neu", [] {
        modellZeit().autoKey = false;
        modellZeit().einstellungenGelesen = false;
    }));
    add(pause(0.4));
    add(pruefe(name("nach einem neuen Start steht Auto Key wieder an (die Wahl des Anwenders)"), [] { return modellZeit().autoKey; }));
    add(klickBand(tr(Str::MzAutoKey)));
    add(pause(0.3));
    add(pruefe(name("ein zweiter Klick schaltet aus, auch in der Einstellung und in der Datei"), [ausDatei] {
        return !modellZeit().autoKey && !g_app->settings.modellZeitAutoKey && ausDatei(g_app->settings);
    }));
    add(tu("AP1 ENTSCHEID1: wie ein neuer Start", [] {
        modellZeit().autoKey = true;
        modellZeit().einstellungenGelesen = false;
    }));
    add(pause(0.4));
    add(pruefe(name("nach einem neuen Start bleibt Auto Key aus"), [] { return !modellZeit().autoKey; }));
}

void pruefungen(std::vector<Schritt>& s, int modus) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const bool bl = modus == 1;
    const std::string M = bl ? "wie Blender" : "wie 3ds Max";
    const auto name = [=](const std::string& zeile, const std::string& text) { return "AP1 " + zeile + " (" + M + "): " + text; };
    add(tu("AP1: Bedienmodus " + M + ", Ansehen/Pruefen, BOTH_WALK1 Bild 12, Rig, Schluessel", [=] {
        ModellTab& t = g_app->mtab;
        modellBedienungSetzen(modus);
        t.bereich = ModellBereich::Ansehen;
        t.vierAnsichten = false;
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        g_app->animPlaying = false;
        g_app->animTime = 0.0F;
        (void)modellRigAnlegen();
        schluessel();
        modellRigGriffWaehlen(rig3::griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        g_app->animFrame = f0() + 12;
        modellZeitAutoKey(true);
        modellZeit().keyModus = false;
        modellZeit().sequenzAuf = true;
        g_app->settings.modellZeitAnzeige = 0;
        g_app->animSpeed = 1.0F;
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    // --- Z1, Z2: in jedem Bereich, drei Fassungen ---------------------------------------------------------------------------
    struct Soll {
        ModellBereich b;
        const char* name;
        int fassung;
    };
    static const Soll kSoll[kModellBereiche] = {{ModellBereich::Ansehen, "Ansehen/Pruefen", 1}, {ModellBereich::Modellieren, "Modellieren", 0},
                                                {ModellBereich::Uv, "UV", 0},                   {ModellBereich::Weights, "Weights", 0},
                                                {ModellBereich::Painting, "Painting", 0},       {ModellBereich::Animation, "Animation", 2}};
    auto hoehen = std::make_shared<std::array<float, kModellBereiche>>();
    for (int i = 0; i < kModellBereiche; ++i) {
        const Soll soll = kSoll[i];
        add(bereich(soll.b, soll.name));
        add(pause(0.4));
        add(pruefe(name("Z1", std::string("die Leiste steht im Bereich ") + soll.name + " ueber die volle Breite unter den Ansichten"), [=] {
            const ImRect b = r4(P().band);
            const ImGuiWindow* an = mbefoto::kindFenster("mtansichten");
            melde(std::string("Z1 ") + soll.name + ": Leiste gezeichnet " + std::to_string(leisteDa()) + ", Band " + zahl(b.Min.x) + "/" + zahl(b.Min.y) + " " + zahl(b.GetWidth()) + " x " +
                  zahl(b.GetHeight()) + ", Fenster " + zahl(ImGui::GetIO().DisplaySize.x) + ", Ansichten bis " + (an != nullptr ? zahl(an->Pos.y + an->Size.y) : std::string("-")));
            (*hoehen)[static_cast<std::size_t>(i)] = b.GetHeight();
            return leisteDa() && b.GetWidth() >= ImGui::GetIO().DisplaySize.x * 0.85F && an != nullptr && b.Min.y >= an->Pos.y + an->Size.y - 1.0F;
        }));
        add(pruefe(name("Z2", std::string("Fassung im Bereich ") + soll.name + (soll.fassung == 2 ? ": voll" : soll.fassung == 1 ? ": Sequenz" : ": kurz")), [=] {
            const bool spuren = P().transport.find("spuren") != std::string::npos;
            const bool tempo = P().transport.find("tempo") != std::string::npos;
            melde(std::string("Z2 ") + soll.name + ": Fassung " + std::to_string(P().fassung) + ", Knopfreihe " + P().transport + ", Hinweis \"" + P().hinweisText + "\"");
            return leisteDa() && P().fassung == soll.fassung && spuren == (soll.fassung != 0) && tempo == (soll.fassung == 1);
        }));
        add(pruefe(name("R8", std::string("Statuszeile im Bereich ") + soll.name + ": Bedienmodus, Tasten des Bereichs, Undo und Suche - unterste Zeile der Leiste"), [=] {
            static const Str kMax[kModellBereiche] = {Str::MzStMaxAnsehen, Str::MzStMaxModell, Str::MzStMaxUv, Str::MzStMaxWeights, Str::MzStMaxPaint, Str::MzStMaxAnim};
            static const Str kBl[kModellBereiche] = {Str::MzStBlAnsehen, Str::MzStBlModell, Str::MzStBlUv, Str::MzStBlWeights, Str::MzStBlPaint, Str::MzStBlAnim};
            const std::string soll2 = std::string(tr(bl ? Str::MbWieBlender : Str::MbWieMax)) + " | " + tr(bl ? kBl[i] : kMax[i]) + " | " + tr(bl ? Str::MzStatusUndoBl : Str::MzStatusUndoMax);
            const ImRect st = r4(P().status);
            const ImRect b = r4(P().band);
            melde(std::string("R8 ") + soll.name + ": \"" + P().statusText + "\", Zeile " + zahl(st.Min.y) + "-" + zahl(st.Max.y) + ", Band bis " + zahl(b.Max.y));
            return leisteDa() && P().statusText == soll2 && st.GetHeight() > 0.0F && std::fabs(st.Max.y - b.Max.y) <= 1.0F && st.Max.y <= ImGui::GetIO().DisplaySize.y;
        }));
    }
    add(pruefe(name("Z2", "die volle Fassung (Animation, Spuren offen) ist hoeher als die Sequenz, die kurze am niedrigsten"), [=] {
        const auto& h = *hoehen;
        melde("Z2 Hoehen: Ansehen " + zahl(h[0]) + ", Modellieren " + zahl(h[1]) + ", UV " + zahl(h[2]) + ", Weights " + zahl(h[3]) + ", Painting " + zahl(h[4]) + ", Animation " + zahl(h[5]));
        return h[1] > 0.0F && h[1] == h[2] && h[2] == h[3] && h[3] == h[4] && h[0] > h[1] && h[5] > h[0];
    }));
    // --- Z15, Z14, Z12, Z13: Spuren in der Leiste, ein Lineal, Striche je Frame, Schluessel ---------------------------------
    add(pruefe(name("Z15", "im Bereich Animation stehen die Spuren ab Werk offen in der Leiste (kein eigener Kasten \"animeditor\" mehr)"), [=] {
        const ImRect sp = r4(P().spuren);
        const ImRect b = r4(P().band);
        melde("Z15: kvOffen " + std::to_string(g_app->mtab.kvOffen) + ", Spurbereich " + zahl(sp.Min.y) + "-" + zahl(sp.Max.y) + " im Band " + zahl(b.Min.y) + "-" + zahl(b.Max.y) + ", Spuren " +
              std::to_string(P().spurZeilen) + ", Kasten " + std::to_string(mbefoto::kindFenster("animeditor") != nullptr));
        return g_app->mtab.kvOffen && sp.GetHeight() > ImGui::GetFontSize() * 2.0F && sp.Min.y > b.Min.y && sp.Max.y < b.Max.y && P().spurZeilen >= 4 &&
               mbefoto::kindFenster("animeditor") == nullptr && g_app->mtab.kvGezeichnet >= ImGui::GetFrameCount() - 2;
    }));
    add(pruefe(name("Z14", std::string("links die Namensspalte \"") + tr(bl ? Str::MbKvZusammenfassung : Str::MzAuswahlSpur) + "\", Bild 0 in Skala und Spuren an derselben Stelle, Linie des Bilds durch alle Spuren"), [=] {
        const ImRect n = r4(P().namen);
        const ImRect sk = r4(P().skala);
        const float x0 = modellKvBildAufSchirm(static_cast<float>(f0()));
        ImVec2 raute;
        const bool da = modellDsRauteAufSchirm(rhand(), f0(), raute);
        melde("Z14: Namen \"" + P().namenText + "\" " + zahl(n.Min.x) + "-" + zahl(n.Max.x) + ", Skala ab " + zahl(sk.Min.x) + ", Bild 0 Skala " + zahl(P().bildX0) + " / Spuren " + zahl(x0) + " / Raute " +
              (da ? zahl(raute.x) : std::string("-")) + ", Linien " + std::to_string(P().kopfLinien));
        return P().namenText == tr(bl ? Str::MbKvZusammenfassung : Str::MzAuswahlSpur) && std::fabs(n.Max.x - sk.Min.x) <= 1.0F && da && std::fabs(P().bildX0 - x0) <= 0.01F &&
               std::fabs(raute.x - x0) <= 0.01F && P().kopfLinien == 2;
    }));
    add(pruefe(name("Z12", bl ? "keine Striche je Frame (Blender)" : "in der Skala und in jeder Spur ein feiner Strich je Frame"), [=] {
        const int n = seq().numFrames;
        melde("Z12: Frames " + std::to_string(n) + ", Striche Skala " + std::to_string(P().frameStriche) + ", Striche Spuren " + std::to_string(P().spurStriche) + " in " + std::to_string(P().spurZeilen) + " Spuren");
        return bl ? (P().frameStriche == 0 && P().spurStriche == 0) : (P().frameStriche == n && P().spurZeilen > 0 && P().spurStriche == n * P().spurZeilen);
    }));
    add(pruefe(name("Z13", bl ? "Schluessel als Rauten in Skala und Spuren" : "Schluessel als Striche: Gruen = Drehung, Rot = Bewegung, zweifarbig fuer beides"), [=] {
        melde("Z13: Striche " + std::to_string(P().striche) + " (rot " + std::to_string(P().stricheRot) + ", gruen " + std::to_string(P().stricheGruen) + ", zweifarbig " + std::to_string(P().stricheZwei) +
              "), Rauten " + std::to_string(P().rauten));
        // Skala: +0/+10/+20 gruen, +5 rot, +15 zweifarbig; Spuren: Koerper (5), cranium (3), pelvis (2), Arm R (3), rhand (3) = 5 + 16
        return bl ? (P().rauten == 21 && P().striche == 0) : (P().striche == 21 && P().stricheRot == 3 && P().stricheZwei == 3 && P().stricheGruen == 15 && P().rauten == 0);
    }));
    add(pruefe(name("Z17", "Hinweis unter der Leiste: Legende der Schluessel und die Zaehlung"), [=] {
        const std::string& h = P().hinweisText;
        melde("Z17: \"" + h + "\"");
        return h.find(tr(bl ? Str::MzLegendeBlender : Str::MzLegendeMax)) != std::string::npos && h.find(tr(bl ? Str::MbZeitHinweisBlender : Str::MbZeitHinweis)) != std::string::npos;
    }));
    // --- Z11: Skala mit Teilstrichen, Zahlen und dem Kaestchen des Abspielkopfs; Klick und Ziehen setzen das Bild ------------
    add(pruefe(name("Z11", "Skala: Teilstriche mit Zahlen, Abspielkopf mit der Bildnummer im Kaestchen"), [=] {
        const ImRect sk = r4(P().skala);
        const ImRect k = r4(P().kopf);
        const float x = modellKvBildAufSchirm(static_cast<float>(g_app->animFrame));
        melde("Z11: Skala " + zahl(sk.GetWidth()) + " x " + zahl(sk.GetHeight()) + ", Teilstriche " + std::to_string(P().teilstriche) + ", Kaestchen \"" + P().kopfText + "\" " + zahl(k.Min.x) + "-" + zahl(k.Max.x) +
              ", Bild bei " + zahl(x));
        return std::fabs(sk.GetHeight() - modellZeitSkalaHoehe()) <= 1.0F && P().teilstriche >= 4 && P().kopfText == std::to_string(12 + (bl ? 1 : 0)) && k.Min.x <= x && k.Max.x >= x &&
               k.Min.y >= sk.Min.y - 2.0F && k.Max.y < sk.GetCenter().y;
    }));
    add(klickAn("in die Skala (Zahlen) bei Bild 20", [] { return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 20)), r4(P().skala).Max.y - 4.0F}; }));
    add(pause(0.2));
    add(pruefe(name("Z11", "ein Klick in die Skala springt auf das Bild"), [] {
        melde("Z11 Klick: Bild " + std::to_string(g_app->animFrame - f0()));
        return g_app->animFrame == f0() + 20;
    }));
    add(mb3::ziehenVonNach("in der Skala von Bild 20 nach Bild 3", [] { return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 20)), r4(P().skala).Max.y - 4.0F}; },
                           [] { return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 3)), r4(P().skala).Max.y - 4.0F}; }));
    add(pause(0.2));
    add(pruefe(name("Z11", "Ziehen in der Skala setzt das Bild (scrubben)"), [] {
        melde("Z11 Ziehen: Bild " + std::to_string(g_app->animFrame - f0()));
        return g_app->animFrame == f0() + 3;
    }));
    // --- Z3: der Griff ---------------------------------------------------------------------------------------------------------
    auto spurVorher = std::make_shared<float>(0.0F);
    add(pruefe(name("Z3", "mittig ueber der Leiste der Griff"), [=] {
        const ImRect g = r4(P().griff);
        const ImRect b = r4(P().band);
        *spurVorher = P().spuren[3];
        melde("Z3: Griff " + zahl(g.Min.x) + "-" + zahl(g.Max.x) + " bei " + zahl(g.Min.y) + ", Band Mitte " + zahl(b.GetCenter().x) + ", Spurbereich " + zahl(*spurVorher));
        return g.GetWidth() > ImGui::GetFontSize() * 3.0F && std::fabs(g.GetCenter().x - b.GetCenter().x) <= 1.0F && std::fabs(g.Min.y - b.Min.y) <= 1.0F;
    }));
    add(mb3::ziehenVonNach("den Griff 4 Schrifthoehen nach oben", [] { return r4(P().griff).GetCenter(); },
                           [] { return ImVec2{r4(P().griff).GetCenter().x, r4(P().griff).GetCenter().y - ImGui::GetFontSize() * 4.0F}; }));
    add(pause(0.3));
    add(pruefe(name("Z3", "den Griff hochziehen macht den Spurbereich um den Weg hoeher"), [=] {
        melde("Z3 hoch: Spurbereich " + zahl(*spurVorher) + " -> " + zahl(P().spuren[3]) + ", Weg " + zahl(ImGui::GetFontSize() * 4.0F));
        return std::fabs(P().spuren[3] - (*spurVorher + ImGui::GetFontSize() * 4.0F)) <= 2.0F;
    }));
    add(mb3::ziehenVonNach("den Griff ganz nach unten", [] { return r4(P().griff).GetCenter(); },
                           [] { return ImVec2{r4(P().griff).GetCenter().x, ImGui::GetIO().DisplaySize.y - 4.0F}; }));
    add(pause(0.3));
    add(pruefe(name("Z3", "den Griff ganz herunterziehen zeigt nur die Zeitleiste (Spuren zu), die Skala mit den Schluesseln bleibt"), [=] {
        melde("Z3 unten: kvOffen " + std::to_string(g_app->mtab.kvOffen) + ", Spurbereich " + zahl(P().spuren[3]) + ", Schluessel in der Skala " + std::to_string(bl ? P().rauten : P().striche));
        return !g_app->mtab.kvOffen && P().spuren[3] <= 0.0F && (bl ? P().rauten : P().striche) == 5;
    }));
    // --- Z10: Knopf Spuren -------------------------------------------------------------------------------------------------------
    add(klickBand(tr(Str::MzSpuren)));
    add(pause(0.3));
    add(pruefe(name("Z10", "der Knopf \"Spuren\" klappt die Spuren wieder auf"), [=] {
        melde("Z10: kvOffen " + std::to_string(g_app->mtab.kvOffen) + ", Spurbereich " + zahl(P().spuren[3]));
        return g_app->mtab.kvOffen && P().spuren[3] > ImGui::GetFontSize() * 2.0F;
    }));
    add(tu("AP1: Spurbereich wieder auf die Vorgabe", [] {
        g_app->settings.modellZeitSpurHoeheMax = Settings{}.modellZeitSpurHoeheMax;
        g_app->settings.modellZeitSpurHoeheBlender = Settings{}.modellZeitSpurHoeheBlender;
    }));
    // --- Z4 / Z18, Z5, Z21: die Knopfreihe ---------------------------------------------------------------------------------------
    add(pruefe(name(bl ? "Z18" : "Z4", bl ? "Knopfreihe: Anfang, Schluessel zurueck, Abspielen, Schluessel vor, Ende, Aufnahme, Start/Ende/Bild, Bildrate"
                                         : "Knopfreihe: Anfang, Bild zurueck, Abspielen, Bild vor, Ende, Bild, Auto Key, Set Key, Zeitkonfiguration"), [=] {
        const std::string soll = bl ? "anfang|keyzurueck|abspielen|keyvor|ende|aufnahme|start|ende|bild|zeitkonfig|" : "anfang|bildzurueck|abspielen|bildvor|ende|bildanzeige|autokey|setkey|zeitkonfig|";
        const char* knoepfe[5] = {tr(Str::TlRewind), tr(bl ? Str::MzKeyZurueck : Str::ModelPrevFrame), tr(Str::ModelPlay), tr(bl ? Str::MzKeyVor : Str::ModelNextFrame), tr(Str::MzEnde)};
        bool alle = true;
        float x = -1.0F;
        for (const char* k : knoepfe) {
            const Element* e = imBand(k);
            alle = alle && e != nullptr && e->rect.Min.x > x;
            x = e != nullptr ? e->rect.Min.x : x;
        }
        melde(std::string(bl ? "Z18" : "Z4") + ": Knopfreihe " + P().transport + ", die fuenf Knoepfe der Reihe nach " + std::to_string(alle) + ", Zeilen " + std::to_string(P().transportZeilen));
        return alle && (P().transport + "|").rfind(soll, 0) == 0 && P().transportZeilen == 1;
    }));
    add(klickBand(tr(Str::MzEnde)));
    add(pause(0.2));
    add(pruefe(name("Z4", "der Knopf \"Ans Ende\" springt auf das letzte Bild"), [] {
        melde("Z4 Ende: Bild " + std::to_string(g_app->animFrame - f0()) + " von " + std::to_string(seq().numFrames - 1));
        return g_app->animFrame == f0() + seq().numFrames - 1;
    }));
    add(mbefoto::mausAuf("\"Ans Ende\"", [] { return imBand(tr(Str::MzEnde)); }));
    add(pruefe(name("Z4", "der Tooltip des Knopfs nennt die Taste des Modus"), [=] {
        const std::string tip = tr(bl ? Str::MzTipEndeBl : Str::MzTipEndeMax);
        melde("Z4 Tooltip: da " + std::to_string(tooltipDa()) + ", Text \"" + tip + "\"");
        return tooltipDa() && tip.find('(') != std::string::npos;
    }));
    add(klickBand(tr(Str::TlRewind)));
    add(pause(0.2));
    add(pruefe(name("Z5", bl ? "Zaehlung ab 1 (1 / Anzahl)" : "Bildanzeige ab 0 (0 / Anzahl - 1)"), [=] {
        const std::string soll = bl ? "1 / " + std::to_string(seq().numFrames) : "0 / " + std::to_string(seq().numFrames - 1);
        melde("Z5: \"" + g_app->mtab.zaehlungText + "\", soll Anfang \"" + soll + "\"");
        return g_app->animFrame == f0() && g_app->mtab.zaehlungText.rfind(soll, 0) == 0 && (bl ? findeEndung("##mzbild", "") != nullptr : P().transport.find("bildanzeige") != std::string::npos);
    }));
    add(pruefe(name("Z21", "der Name der Sequenz steht in der Knopfreihe"), [] {
        melde("Z21: Knopfreihe " + P().transport);
        return P().transport.find("|name") != std::string::npos;
    }));
    // --- Z18 / Z19 / A13: Spruenge zum Schluessel ---------------------------------------------------------------------------------
    if (bl) {
        add(klickBand(tr(Str::MzKeyVor)));
        add(pause(0.2));
        add(pruefe(name("Z18", "\"Schluessel vor\" springt von Bild 1 auf den naechsten Schluessel (+5)"), [] {
            melde("Z18 Schluessel vor: Bild " + std::to_string(g_app->animFrame - f0()));
            return g_app->animFrame == f0() + 5;
        }));
        add(fokus());
        add(taste(ImGuiKey_UpArrow, "Pfeil hoch (naechster Schluessel, wie Blender)"));
        add(pause(0.2));
        add(taste(ImGuiKey_UpArrow, "Pfeil hoch"));
        add(pause(0.2));
        add(pruefe(name("A13", "Pfeil hoch springt zweimal weiter (+10, +15)"), [] {
            melde("A13 hoch: Bild " + std::to_string(g_app->animFrame - f0()));
            return g_app->animFrame == f0() + 15;
        }));
        add(taste(ImGuiKey_DownArrow, "Pfeil runter (voriger Schluessel)"));
        add(pause(0.2));
        add(pruefe(name("A13", "Pfeil runter springt zurueck (+10)"), [] {
            melde("A13 runter: Bild " + std::to_string(g_app->animFrame - f0()));
            return g_app->animFrame == f0() + 10;
        }));
        add(klickBand(tr(Str::MzKeyZurueck)));
        add(pause(0.2));
        add(pruefe(name("Z18", "\"Schluessel zurueck\" springt auf +5"), [] { return g_app->animFrame == f0() + 5; }));
        add(tu("AP1: Bild +7, Hand R gewaehlt", [] {
            g_app->animFrame = f0() + 7;
            g_app->modelDirty = true;
        }));
        add(fokus());
        add(taste(ImGuiKey_K, "K (Keying-Menue, wie Blender)"));
        add(pause(0.4));
        add(pruefe(name("A13", "K oeffnet das Keying-Menue mit \"Schluessel setzen\""), [] {
            melde("A13 K: im Menue " + std::to_string(imPopup(tr(Str::MbKvSetzen)) != nullptr) + ", " + std::to_string(imPopup(tr(Str::MbRgSchluesselLoeschen)) != nullptr));
            return imPopup(tr(Str::MbKvSetzen)) != nullptr && imPopup(tr(Str::MbRgSchluesselLoeschen)) != nullptr;
        }));
        add(klickPopup(tr(Str::MbKvSetzen)));
        add(pause(0.3));
        add(pruefe(name("A13", "\"Schluessel setzen\" im Keying-Menue setzt den Schluessel der Hand am Bild +7"), [] { return schl(rhand(), f0() + 7) != nullptr; }));
    } else {
        add(tu("AP1: Key-Modus an, Bild 0", [] {
            modellZeit().keyModus = true;
            g_app->animFrame = f0();
        }));
        add(klickBand(tr(Str::ModelNextFrame)));
        add(pause(0.2));
        add(pruefe(name("Z18", "Key-Modus: \"Bild vor\" springt auf den naechsten Schluessel (+5)"), [] {
            melde("Z18 Key-Modus: Bild " + std::to_string(g_app->animFrame - f0()));
            return g_app->animFrame == f0() + 5;
        }));
        add(fokus());
        add(taste(ImGuiKey_Period, ". (Key-Modus)"));
        add(pause(0.2));
        add(pruefe(name("Z19", "Key-Modus: die Taste . springt weiter (+10)"), [] { return g_app->animFrame == f0() + 10; }));
        add(taste(ImGuiKey_Comma, ", (Key-Modus)"));
        add(pause(0.2));
        add(pruefe(name("Z19", "Key-Modus: die Taste , springt zurueck (+5)"), [] { return g_app->animFrame == f0() + 5; }));
        add(tu("AP1: Key-Modus aus", [] { modellZeit().keyModus = false; }));
        add(taste(ImGuiKey_Period, ". (ein Bild)"));
        add(pause(0.2));
        add(pruefe(name("Z19", "ohne Key-Modus geht . ein Bild weiter (+6)"), [] { return g_app->animFrame == f0() + 6; }));
    }
    // --- Z6, Z7, A12: Auto Key und Set Key ---------------------------------------------------------------------------------------
    const std::string autoLabel = tr(bl ? Str::MzAutoKeying : Str::MzAutoKey);
    add(tu("AP1: Schicht leer, Bild +7, Hand R", [] {
        schichtLeer();
        modellRigGriffWaehlen(rig3::griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        g_app->animFrame = f0() + 7;
    }));
    add(pause(0.2));
    add(pruefe(name("Z6", "der Knopf " + autoLabel + " steht in der Knopfreihe, fuer die folgenden Zeilen an (ab Werk aus: ENTSCHEID1)"), [=] {
        melde("Z6: Knopf " + std::to_string(imBand(autoLabel) != nullptr) + ", an " + std::to_string(modellZeit().autoKey));
        return imBand(autoLabel) != nullptr && modellZeit().autoKey;
    }));
    add(tu("AP1: Hand R drehen (Auto Key an)", [] { handDrehen(); }));
    add(pause(0.2));
    add(tu("AP1: ein Bild weiter", [] {
        g_app->animFrame = f0() + 8;
        g_app->modelDirty = true;
    }));
    add(pause(0.2));
    add(pruefe(name("Z6", "Auto Key an: der Zug setzt einen Schluessel, er bleibt beim Bildwechsel (wie bis rc611)"), [] {
        melde("Z6 an: Schluessel +7 " + std::to_string(schl(rhand(), f0() + 7) != nullptr) + ", offen " + std::to_string(modellZeit().offen.size()));
        return schl(rhand(), f0() + 7) != nullptr && modellZeit().offen.empty();
    }));
    if (bl) {
        add(klickBand(autoLabel));
    } else {
        add(fokus());
        add(taste(ImGuiKey_N, "N (Auto Key, wie 3ds Max)"));
    }
    add(pause(0.2));
    add(pruefe(name(bl ? "Z18" : "A12", bl ? "der Aufnahme-Knopf schaltet Auto-Keying aus" : "die Taste N schaltet Auto Key aus"), [] { return !modellZeit().autoKey; }));
    auto verworfen = std::make_shared<int>(0);
    add(tu("AP1: Schicht leer, Bild +7, Hand R drehen (Auto Key aus)", [=] {
        schichtLeer();
        g_app->animFrame = f0() + 7;
        *verworfen = modellZeit().verworfen;
    }));
    add(pause(0.2));
    add(tu("AP1: Hand R drehen", [] { handDrehen(); }));
    add(pause(0.3));
    add(pruefe(name("Z6", "Auto Key aus: die Pose steht da, gilt aber als Pose ohne Schluessel (Hinweis in der Leiste)"), [] {
        const std::string h = P().hinweisText;
        melde("Z6 aus: Schluessel +7 " + std::to_string(schl(rhand(), f0() + 7) != nullptr) + ", offen " + std::to_string(modellZeit().offen.size()) + ", Hinweis \"" + h + "\"");
        return schl(rhand(), f0() + 7) != nullptr && modellZeit().offen.size() == 1U && h == offenText();
    }));
    add(klickAn("in die Skala (Zahlen) bei Bild 9", [] { return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 9)), r4(P().skala).Max.y - 4.0F}; }));
    add(pause(0.3));
    add(pruefe(name("Z6", "Auto Key aus: ein Bildwechsel verwirft die Pose ohne Schluessel"), [=] {
        melde("Z6 verworfen: Schluessel +7 " + std::to_string(schl(rhand(), f0() + 7) != nullptr) + ", verworfen " + std::to_string(modellZeit().verworfen - *verworfen) + ", Bild " +
              std::to_string(g_app->animFrame - f0()));
        return schl(rhand(), f0() + 7) == nullptr && modellZeit().verworfen == *verworfen + 1 && modellZeit().offen.empty() && g_app->animFrame == f0() + 9;
    }));
    add(tu("AP1: Bild +7, Hand R drehen (Auto Key aus)", [] {
        g_app->animFrame = f0() + 7;
        g_app->modelDirty = true;
    }));
    add(pause(0.2));
    add(tu("AP1: Hand R drehen", [] { handDrehen(); }));
    add(pause(0.2));
    if (bl) {
        add(fokus());
        add(taste(ImGuiKey_I, "I (Schluessel setzen, wie Blender)"));
    } else {
        add(klickBand(tr(Str::MzSetKey)));
    }
    add(pause(0.3));
    add(klickAn("in die Skala (Zahlen) bei Bild 9", [] { return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 9)), r4(P().skala).Max.y - 4.0F}; }));
    add(pause(0.3));
    add(pruefe(name("Z7", bl ? "I behaelt die Pose: der Schluessel bleibt nach dem Bildwechsel" : "der Knopf Set Key behaelt die Pose: der Schluessel bleibt nach dem Bildwechsel"), [] {
        melde("Z7: Schluessel +7 " + std::to_string(schl(rhand(), f0() + 7) != nullptr) + ", behalten " + std::to_string(modellZeit().uebernommen) + ", offen " + std::to_string(modellZeit().offen.size()));
        return schl(rhand(), f0() + 7) != nullptr && modellZeit().offen.empty() && g_app->animFrame == f0() + 9;
    }));
    if (!bl) {
        add(tu("AP1: Bild +3", [] { g_app->animFrame = f0() + 3; }));
        add(fokus());
        add(taste(ImGuiKey_K, "K (Set Key, wie 3ds Max)"));
        add(pause(0.2));
        add(pruefe(name("A12", "die Taste K setzt einen Schluessel am Bild +3"), [] { return schl(rhand(), f0() + 3) != nullptr; }));
        add(taste(ImGuiKey_N, "N (Auto Key an)"));
        add(pause(0.2));
        add(pruefe(name("A12", "die Taste N schaltet Auto Key wieder an (die Einstellung folgt)"), [] { return modellZeit().autoKey && g_app->settings.modellZeitAutoKey; }));
    }
    add(tu("AP1: Auto Key an, Testschluessel", [] {
        modellZeitAutoKey(true);
        schluessel();
        g_app->animFrame = f0() + 12;
    }));
    add(pause(0.3));
    // --- Schluessel in der Skala waehlen und ziehen (Spurleiste) ------------------------------------------------------------------
    add(klickAn("auf den Schluessel +10 in der Skala", [] {
        ImVec2 p;
        (void)modellKvZeileAufSchirm(0, -1, p);
        return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 10)), p.y};
    }));
    add(pause(0.2));
    add(pruefe(name("Z13", "ein Klick auf einen Schluessel in der Skala waehlt die Schluessel dieses Bilds (rhand, cranium), das Bild bleibt"), [] {
        const auto& w = g_app->mtab.kvWahl;
        melde("Z13 Wahl: " + std::to_string(w.size()) + " Schluessel, Bild " + std::to_string(g_app->animFrame - f0()));
        return w.size() == 2U && w[0].bild == f0() + 10 && g_app->animFrame == f0() + 12;
    }));
    add(mb3::ziehenVonNach("den Schluessel +10 in der Skala um 3 Frames nach rechts", [] {
        ImVec2 p;
        (void)modellKvZeileAufSchirm(0, -1, p);
        return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 10)), p.y};
    }, [] {
        ImVec2 p;
        (void)modellKvZeileAufSchirm(0, -1, p);
        return ImVec2{modellKvBildAufSchirm(static_cast<float>(f0() + 13)), p.y};
    }));
    add(pause(0.3));
    add(pruefe(name("Z13", "in der Skala gezogen liegen die Schluessel bei +13 (ein Rueckgaengig-Schritt)"), [] {
        return schl(rhand(), f0() + 13) != nullptr && schl(rhand(), f0() + 10) == nullptr;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe(name("Z13", "Strg+Z stellt +10 wieder her"), [] { return schl(rhand(), f0() + 10) != nullptr && schl(rhand(), f0() + 13) == nullptr; }));
    // --- Z8: Zeitkonfiguration -----------------------------------------------------------------------------------------------------
    add(klickBand(tr(Str::MzZeitKonfig)));
    add(pause(0.4));
    add(pruefe(name("Z8", "der Knopf mit der Bildrate oeffnet die Zeitkonfiguration mit den Werten der animation.cfg (Anfang, Anzahl, Schleife, fps, Bereichsbalken)"), [] {
        const bool felder = findeEndung("##mkanfang", "mzkonfig") != nullptr && findeEndung("##mkanzahl", "mzkonfig") != nullptr && findeEndung("##mkschleife", "mzkonfig") != nullptr &&
                            findeEndung("##mkfps", "mzkonfig") != nullptr && findeEndung("##mkbalken", "mzkonfig") != nullptr;
        melde("Z8: offen " + std::to_string(modellZeit().konfigOffen) + ", Felder " + std::to_string(felder) + ", Zeitanzeige " + std::to_string(finde(tr(Str::MzSekunden), "mzkonfig", 0, false) != nullptr));
        return modellZeit().konfigOffen && felder && finde(tr(Str::MzSekunden), "mzkonfig", 0, false) != nullptr;
    }));
    add(klick(tr(Str::MzSekunden), "mzkonfig"));
    add(pause(0.3));
    add(pruefe(name("Z8", "Zeitanzeige Sekunden: das Kaestchen des Abspielkopfs zeigt Sekunden"), [] {
        char soll[32];
        std::snprintf(soll, sizeof soll, "%.2f", 12.0 / static_cast<double>(std::max(std::abs(seq().fps), 1)));
        melde("Z8 Sekunden: Kaestchen \"" + P().kopfText + "\", soll \"" + soll + "\"");
        return g_app->settings.modellZeitAnzeige == 1 && P().kopfText == soll;
    }));
    add(klick(tr(Str::MzFrames), "mzkonfig"));
    add(pause(0.2));
    add(tu("AP1: Zeitkonfiguration zu", [] { modellZeit().konfigOffen = false; }));
    add(pause(0.2));
    // --- Ansehen/Pruefen: Z9, Z16, Z20 -----------------------------------------------------------------------------------------------
    add(bereich(ModellBereich::Ansehen, "Ansehen/Pruefen"));
    add(tu("AP1: Sekundaer TORSO_WEAPONREADY3 (oder die naechste TORSO_-Sequenz)", [] {
        int a = mb1::animNach("TORSO_WEAPONREADY3");
        for (std::size_t i = 0; a < 0 && i < g_app->animList.size(); ++i) {
            if (g_app->animList[i].name.rfind("TORSO_", 0) == 0) { a = static_cast<int>(i); }
        }
        g_app->mtab.sekundaer = a;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(pruefe(name("Z9", "Ansehen/Pruefen: Tempo (- 1.00x +), Interpolation und Schleife erzwingen in der Knopfreihe"), [] {
        const bool da = imBand(tr(Str::MzLangsamer)) != nullptr && imBand(tr(Str::MzSchneller)) != nullptr && imBand(tr(Str::MbInterpolation)) != nullptr && imBand(tr(Str::MzSchleife)) != nullptr;
        melde("Z9: Knopfreihe " + P().transport + ", Knoepfe " + std::to_string(da));
        return da && P().transport.find("langsamer|tempo|schneller|interpolation|schleife") != std::string::npos;
    }));
    add(klickBand(tr(Str::MzSchneller)));
    add(pause(0.2));
    add(pruefe(name("Z9", "\"+\" stellt das Tempo auf 1.5"), [] { return std::fabs(g_app->animSpeed - 1.5F) < 0.001F; }));
    add(klickBand(tr(Str::MzLangsamer)));
    add(klickBand(tr(Str::MzLangsamer)));
    add(pause(0.2));
    add(pruefe(name("Z9", "zweimal \"-\" stellt es auf 0.5"), [] { return std::fabs(g_app->animSpeed - 0.5F) < 0.001F; }));
    add(klickBand(tr(Str::MzSchneller)));
    add(klickBand(tr(Str::MzSchleife)));
    add(pause(0.2));
    add(pruefe(name("Z9", "\"Schleife erzwingen\" schaltet die Schleife"), [] { return g_app->loopMode == 1 && std::fabs(g_app->animSpeed - 1.0F) < 0.001F; }));
    add(klickBand(tr(Str::MzSchleife)));
    add(klickBand(tr(Str::MbInterpolation)));
    add(pause(0.2));
    add(pruefe(name("Z9", "\"Interpolation\" schaltet die Interpolation aus"), [] { return g_app->loopMode == 0 && !g_app->mtab.interpolieren; }));
    add(klickBand(tr(Str::MbInterpolation)));
    add(pause(0.2));
    add(pruefe(name("Z16", "Ansehen/Pruefen: zwei Zeilen Primaer / Sekundaer unter der Skala, \"Sekundaer aus\" daneben"), [] {
        const ImRect sp = r4(P().spuren);
        const ImRect sk = r4(P().skala);
        melde("Z16: Zeilen " + std::to_string(P().spurZeilen) + ", Bereich " + zahl(sp.Min.y) + "-" + zahl(sp.Max.y) + " unter der Skala bis " + zahl(sk.Max.y) + ", Knopf " +
              std::to_string(imBand(tr(Str::MbSekundaerAus)) != nullptr));
        return P().spurZeilen == 2 && sp.Min.y >= sk.Max.y && sp.GetHeight() > 0.0F && imBand(tr(Str::MbSekundaerAus)) != nullptr;
    }));
    add(klickBand(tr(Str::MbSekundaerAus)));
    add(pause(0.2));
    add(pruefe(name("Z16", "\"Sekundaer aus\" nimmt die Sekundaersequenz weg"), [] { return g_app->mtab.sekundaer < 0; }));
    add(klickBand(tr(Str::MzSpuren)));
    add(pause(0.3));
    add(pruefe(name("Z10", "Ansehen/Pruefen: der Knopf \"Spuren\" klappt die Zeilen zu"), [] { return !modellZeit().sequenzAuf && P().spuren[3] <= 0.0F && P().spurZeilen == 0; }));
    add(klickBand(tr(Str::MzSpuren)));
    add(pause(0.2));
    add(pruefe(name("Z20", "Haut und Flaechenzaehler stehen nicht mehr in der Zeitleiste, sondern in der Gruppe \"Detailstufe und Skin\""), [] {
        const Element* rechts = mbefoto::inFenster(tr(Str::ModelSkin), "mtrollouts");
        melde("Z20: in der Leiste " + std::to_string(imBand(tr(Str::ModelSkin)) != nullptr) + ", im Befehlsfeld " + std::to_string(rechts != nullptr) + ", Haeute " + std::to_string(g_app->modelSkins.size()));
        return imBand(tr(Str::ModelSkin)) == nullptr && (rechts != nullptr || g_app->modelSkins.empty());
    }));
    // --- kurze Fassung: was die Knopfreihe nicht zeigt, steht im Menue der Zeitleiste; Rechtsklick; Suche ---------------------------
    add(bereich(ModellBereich::Weights, "Weights"));
    add(pause(0.4));
    add(pruefe(name("Z9", "kurze Fassung: kein Tempo/Interpolation/Schleife in der Knopfreihe"), [] {
        return imBand(tr(Str::MzSchleife)) == nullptr && imBand(tr(Str::MbInterpolation)) == nullptr && imBand(tr(Str::MzSpuren)) == nullptr;
    }));
    add(klickAn("Rechtsklick in die Skala", [] { return r4(P().skala).GetCenter(); }, 1));
    add(pause(0.4));
    add(pruefe(name("Z9", "Rechtsklick auf die Skala oeffnet das Menue der Zeitleiste mit Schleife, Interpolation, Tempo, Auto Key und Zeitkonfiguration"), [=] {
        const bool wiedergabe = bl ? imPopup(tr(Str::MzMenuePlayback)) != nullptr : (imPopup(tr(Str::MzSchleife)) != nullptr && imPopup(tr(Str::MbInterpolation)) != nullptr && imPopup(tr(Str::ModelSpeed)) != nullptr);
        const bool rest = bl ? (imPopup(tr(Str::MzKeying)) != nullptr && imPopup(tr(Str::MzMenueView)) != nullptr && imPopup(tr(Str::MzMenueMarker)) != nullptr)
                             : (imPopup(tr(Str::MzAutoKey)) != nullptr && imPopup(tr(Str::MzZeitKonfig)) != nullptr && imPopup(tr(Str::MzSpuren)) != nullptr);
        melde("Z9 Menue: Wiedergabe " + std::to_string(wiedergabe) + ", Rest " + std::to_string(rest) + ", Animationswerkzeuge " + std::to_string(imPopup(tr(Str::MbKvMenue)) != nullptr));
        return wiedergabe && rest && imPopup(tr(Str::MbKvMenue)) != nullptr;
    }));
    add(mbefoto::popupsZu());
    add(pause(0.2));
    add(klickBand(tr(Str::MzMenue)));
    add(pause(0.4));
    add(pruefe(name("Z9", "der Menue-Knopf am Ende der Knopfreihe oeffnet dasselbe Menue"), [=] { return imPopup(tr(bl ? Str::MzMenuePlayback : Str::MzSchleife)) != nullptr; }));
    add(mbefoto::popupsZu());
    add(pause(0.2));
    // die Suche oeffnen und auf die Gruppe der Zeitleiste eingrenzen (Zeilen ausserhalb des sichtbaren Teils der Liste
    // tragen im Elementverzeichnis keine Beschriftung)
    add(fokus());
    add(taste(bl ? ImGuiKey_F3 : ImGuiKey_X, "Alle Aktionen suchen"));
    add(pause(0.3));
    add(tippe(std::string(tr(Str::MzGruppe)) + ":"));
    add(pause(0.4));
    add(pruefe(name("Z6", "\"Alle Aktionen suchen\" kennt die Aktionen der Zeitleiste (Auto Key, Zeitkonfiguration, Spuren, Schluessel-Spruenge)"), [=] {
        const std::string vor = std::string(tr(Str::MzGruppe)) + ": ";
        const bool da = imPopup(vor + tr(bl ? Str::MzAutoKeying : Str::MzAutoKey), true) != nullptr && imPopup(vor + tr(Str::MzZeitKonfig), true) != nullptr && imPopup(vor + tr(Str::MzSpuren), true) != nullptr &&
                        imPopup(vor + tr(Str::MzKeyVor), true) != nullptr && imPopup(vor + tr(Str::MzFlaechenListe), true) != nullptr;
        melde("Suche: Aktionen der Zeitleiste " + std::to_string(da));
        return da;
    }));
    add(mbefoto::popupsZu());
    add(pause(0.2));
    // --- R8: Klick auf den Bedienmodus in der Statuszeile ------------------------------------------------------------------------------
    add(klickAn("auf den Bedienmodus in der Statuszeile", [] {
        const ImRect st = r4(P().status);
        return ImVec2{st.Min.x + ImGui::GetFontSize() * 1.5F, st.GetCenter().y};
    }));
    add(pause(0.4));
    add(pruefe(name("R8", "ein Klick auf den Bedienmodus in der Statuszeile bietet beide Modi an"), [] { return imPopup(tr(Str::MbWieMax)) != nullptr && imPopup(tr(Str::MbWieBlender)) != nullptr; }));
    add(klickPopup(tr(bl ? Str::MbWieMax : Str::MbWieBlender)));
    add(pause(0.4));
    add(pruefe(name("R8", "die Wahl schaltet den Bedienmodus um, die Leiste baut sich um"), [=] {
        melde(std::string("R8 Umschalten: Blender ") + std::to_string(modellBlender()) + ", Leiste " + std::to_string(P().blender) + ", Knopfreihe " + P().transport);
        return modellBlender() == !bl && P().blender == !bl && (P().transport.find("aufnahme") != std::string::npos) == !bl;
    }));
    add(tu("AP1: Bedienmodus zurueck", [=] { modellBedienungSetzen(modus); }));
    add(pause(0.3));
}

void fotos(std::vector<Schritt>& s, int modus) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string p = modus == 0 ? "ap1_max_" : "ap1_blender_";
    add(tu(std::string("AP1 Foto: Bedienmodus ") + (modus == 0 ? "wie 3ds Max" : "wie Blender"), [=] {
        ModellTab& t = g_app->mtab;
        modellBedienungSetzen(modus);
        t.bereich = ModellBereich::Ansehen;
        t.vierAnsichten = false;
        t.kvOffen = true;
        t.kvArt = 0;
        t.auswahl = {mb1::teil("torso")};
        t.sekundaer = mb1::animNach("TORSO_WEAPONREADY3");
        modellZeit().sequenzAuf = true;
        modellZeit().konfigOffen = false;
        g_app->animFrame = f0() + 12;
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    static const struct {
        ModellBereich b;
        const char* name;
    } kBereiche[kModellBereiche] = {{ModellBereich::Ansehen, "pruefen"}, {ModellBereich::Modellieren, "modell"}, {ModellBereich::Uv, "uv"},
                                    {ModellBereich::Weights, "weights"}, {ModellBereich::Painting, "paint"},     {ModellBereich::Animation, "anim"}};
    for (const auto& b : kBereiche) {
        const ModellBereich ber = b.b;
        const std::string n = b.name;
        add(tu("AP1 Foto: Bereich " + n, [=] {
            g_app->mtab.bereich = ber;
            if (ber == ModellBereich::Animation) {
                (void)modellRigAnlegen();
                schluessel();
                modellRigGriffWaehlen(rig3::griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
                g_app->animFrame = f0() + 12;
            }
            g_app->modelDirty = true;
        }));
        add(pause(1.0));
        add(pruefe("AP1 Foto " + p + n + ": nichts reicht ueber den Rand der Leiste, die Statuszeile ist im Fenster", [=] {
            std::string was;
            const bool ok = allesImBand(was);
            const ImRect band = r4(P().band);
            melde("Foto " + p + n + ": Fenster " + zahl(ImGui::GetIO().DisplaySize.x) + " x " + zahl(ImGui::GetIO().DisplaySize.y) + ", Band " + zahl(band.GetWidth()) + " x " + zahl(band.GetHeight()) +
                  ", Zeilen der Knopfreihe " + std::to_string(P().transportZeilen) + ", Knopfreihe " + P().transport + (ok ? std::string() : ", ueber dem Rand: " + was));
            return leisteDa() && ok && band.Max.y <= ImGui::GetIO().DisplaySize.y + 0.5F;
        }));
        add(foto(p + n));
    }
    // Animation: Spuren zu, Kurveneditor, Zeitkonfiguration, Menue der Zeitleiste
    add(tu("AP1 Foto: Spuren zu", [] { g_app->mtab.kvOffen = false; }));
    add(pause(0.6));
    add(foto(p + "anim_zu"));
    add(tu("AP1 Foto: Kurveneditor", [] {
        g_app->mtab.kvOffen = true;
        g_app->mtab.kvArt = 1;
        g_app->mtab.kvEinpassen = true;
    }));
    add(pause(0.8));
    add(foto(p + "anim_kurven"));
    add(tu("AP1 Foto: Dopesheet, Zeitkonfiguration", [] {
        g_app->mtab.kvArt = 0;
        g_app->mtab.kvEinpassen = true;
        modellZeit().konfigOffen = true;
    }));
    add(pause(0.8));
    add(foto(p + "anim_zeitkonfig"));
    add(tu("AP1 Foto: Zeitkonfiguration zu, Menue der Zeitleiste", [] {
        modellZeit().konfigOffen = false;
        modellZeit().menueOeffnen = true;
    }));
    add(pause(0.6));
    add(foto(p + "anim_menue"));
    add(mbefoto::popupsZu());
    add(tu("AP1 Foto: Auto Key aus, Pose ohne Schluessel", [] {
        modellZeitAutoKey(false);
        g_app->animFrame = f0() + 7;
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(tu("AP1 Foto: Hand R drehen", [] { handDrehen(); }));
    add(pause(0.6));
    add(foto(p + "anim_pose_offen"));
    add(tu("AP1 Foto: Auto Key wieder aus (ab Werk, ENTSCHEID1)", [] { modellZeitAutoKey(false); }));
    add(pause(0.3));
}

// Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap1): die Bilder h_ap1_max, h_ap1_blender, h_ap1_pruefen der Seite
// "Model-Tab: die Zeitleiste" - Kyle mit dem Saebel, BOTH_WALK1 Bild 12, Rig mit Schluesseln, Hand R gewaehlt.
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto bedienung = std::make_shared<int>(0);
    add(tu("AP1 Handbuch: Skript fuer den Test", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap1.txt"); }));
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    const auto stellen = [](int modus, ModellBereich b) {
        ModellTab& t = g_app->mtab;
        modellBedienungSetzen(modus);
        t.bereich = b;
        t.vierAnsichten = false;
        t.kvOffen = true;
        t.kvArt = 0;
        t.kvEinpassen = true;
        modellZeit().sequenzAuf = true;
        modellZeit().konfigOffen = false;
        modellZeitAutoKey(false);   // ENTSCHEID1: wie ab Werk (aus, wie 3ds Max und Blender)
        g_app->animPlaying = false;
        g_app->animFrame = f0() + 12;
        g_app->modelDirty = true;
    };
    add(tu("AP1 Handbuch: wie 3ds Max, Animation, BOTH_WALK1, Saebel, Rig mit Schluesseln, Hand R", [=] {
        *bedienung = g_app->settings.modellBedienung;
        (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true);
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        (void)modellRigAnlegen();
        schluessel();
        modellRigGriffWaehlen(rig3::griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        g_app->mtab.sekundaer = mb1::animNach("TORSO_WEAPONREADY3");
        stellen(0, ModellBereich::Animation);
    }));
    add(pause(1.0));
    add(tu("AP1 Handbuch: Maus in die Ansicht (kein Tooltip im Bild)", [] { setzeMaus(kv5::fokusPunkt()); }));
    add(pause(0.3));
    add(hilfe2::schrittFoto("h_ap1_max"));
    add(tu("AP1 Handbuch: wie Blender", [=] { stellen(1, ModellBereich::Animation); }));
    add(pause(1.0));
    add(hilfe2::schrittFoto("h_ap1_blender"));
    add(tu("AP1 Handbuch: wie 3ds Max, Ansehen/Pruefen", [=] { stellen(0, ModellBereich::Ansehen); }));
    add(pause(1.0));
    add(hilfe2::schrittFoto("h_ap1_pruefen"));
    add(tu("AP1 Handbuch: zurueck", [=] {
        while (g_app->mtab.dok.kannRueckgaengig()) { (void)modellRueckgaengig(); }
        modellBedienungSetzen(*bedienung);
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.sekundaer = -1;
        modellAnbautenLoesen();
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    return s;
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "ap1" && n != "ap1foto") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto bedienung = std::make_shared<int>(0);
    auto skala = std::make_shared<float>(1.0F);
    auto teiler = std::make_shared<float>(0.0F);
    int b = n == "ap1foto" ? 3840 : 2560;
    int h = n == "ap1foto" ? 2160 : 1440;
    float z = n == "ap1foto" ? 1.5F : 1.0F;
    if (const char* f = std::getenv("BHED_AP1_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    add(tu("AP1: Fenster, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap1.txt");
    }));
    add(pause(0.8));
    const char* m = std::getenv("BHED_AP1_MODELL");
    for (Schritt& x : mb1::laden(m != nullptr ? m : (n == "ap1foto" ? "models/players/kyle/model.glm" : "models/players/stormtrooper/model.glm"))) { add(std::move(x)); }
    add(tu("AP1: BOTH_WALK1 angehalten", [=] {
        *bedienung = g_app->settings.modellBedienung;
        *teiler = g_app->settings.splitModel;
        g_app->settings.splitModel = Settings{}.splitModel;
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        g_app->animPlaying = false;
        g_app->animFrame = f0();
        g_app->animTime = 0.0F;
        g_app->mtab.fokus = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    auto autoKey = std::make_shared<bool>(false);
    add(tu("AP1: Auto Key merken", [autoKey] { *autoKey = modellZeit().autoKey; }));
    if (n == "ap1") {
        werkVorgabe(s);
        pruefungen(s, 0);
        pruefungen(s, 1);
    } else {
        add(tu("AP1 Foto: Saebel an *r_hand", [] { (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true); }));
        add(pause(0.5));
        fotos(s, 0);
        fotos(s, 1);
    }
    add(tu("AP1: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->uiScale = *skala;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.sekundaer = -1;
        modellZeitAutoKey(*autoKey);
        modellAnbautenLoesen();
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.3));
    return s;
}

}  // namespace ap1
