// selbsttest_ent3a.inc - ENTSCHEID3A (12.11., verbindliche Entscheidungen B7-B13 und C4 aus plaene/entscheidungen_1211.md)
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ent3a | ent3afoto | ent3ahb)
//
//   ent3a     je Entscheidung eine Reihe von Pruefungen (jede einmal rot auf dem Stand vor der Umsetzung):
//               B7   wie 3ds Max (Scene Explorer): ein Klick auf die eingefrorene Zeile waehlt sie, sie bleibt eingefroren; der Tooltip
//                    sagt es; der Rechtsklick der Zeile hat "Auswahl auftauen" (3ds Max: Unfreeze Scene Explorer Selection) und
//                    taut auf. Wie Blender (Outliner, Pfeil "waehlbar" aus) bleibt die Zeile nicht waehlbar
//               B8   wie Blender: die Kante der Werkzeugleiste zieht sie breiter; bis 120 Punkte nur Symbole, darueber mit
//                    Werkzeugnamen; die Breite steht in den Einstellungen; Doppelklick auf die Kante: wieder schmal. Wie 3ds Max
//                    bleibt die linke Spalte, wie sie ist
//               B9   wie Blender zeigt die Seitenleiste fuer einen Knochen lokale Werte: in der Grundpose Ort 0, Quaternion
//                    (1, 0, 0, 0), Massstab 1; in einer Pose die Quaternion der Basis (traeger^-1 * welt); Drehmodus waehlbar
//                    (XYZ Euler: drei Felder, Werte wie bhed/modell/drehmodus.h, gegen Blender belegt im ctest modell_ent3a);
//                    eingetippt: Euler X des Beckens +15 Grad lokal, Y und Z bleiben, ein Rueckgaengig-Schritt; Ort X +2 lokal
//               B10  wie 3ds Max: Strg+Umschalt+Z ist Zoom Extents All (alle Ansichten, ganze Figur; Wiederholen bleibt offen),
//                    Strg+Y wiederholt; wie Blender wiederholt Strg+Umschalt+Z; die Suche nennt die Tasten richtig
//               B11  wie 3ds Max hat Edit Poly ein Auge: aus zeigt die Ansicht die Flaeche wie geladen (Bild gleich dem nach dem
//                    Laden), an wieder das Geaenderte; Skin hat ein Auge: aus steht die Figur in der Bindepose (jede Ecke am Ort
//                    der Datei), die Knochen behalten ihre Pose; beides ohne Rueckgaengig-Schritt, Speichern schreibt dieselben
//                    Bytes; wie Blender gilt es nicht
//               B12  Speichern legt neben die .glm "model.glm.stack"; die .glm ist Byte fuer Byte, was Speichern ohne Begleitdatei
//                    schrieb; Oeffnen bringt den Stapel zurueck (gleicher Stapel, erneutes Speichern gleiche .glm); kaputte
//                    Begleitdatei und ausserhalb geaenderte .glm: das Modell laedt, ohne Stapel, mit Meldung; leerer Stapel:
//                    die alte Begleitdatei verschwindet
//               B13  wie 3ds Max steht im Stapel "Unwrap UVW" zwischen den Eintraegen und Edit Poly; ein Klick wechselt in den
//                    Bereich UV; die Suche findet es; wie Blender gibt es die Zeile nicht
//               C4   Esc schliesst das Rechtsklick-Menue der Ansicht (beide Bedienmodi), sonst geschieht nichts
//   ent3afoto nur Fotos (BHED_FOTOS=<Ordner>): ent3a_*
//   ent3ahb   die Bilder fuer das Handbuch (h_ent3a_*)
//   BHED_ENT3A_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe wie der Foto-Selbsttest mbefoto)
//   BHED_ENT3A_THEMA=hell|dunkel
// Eingebunden in selbsttest.cpp (nach selbsttest_fix612b.inc).
namespace ent3a {

using modell::ModArt;

void melde(const std::string& z) { diag::detail("Knopftest: ENT3A " + z); }

constexpr float kPi = 3.14159265358979323846F;

ModellTab& mtab() { return g_app->mtab; }
ModellEnt3a& e3() { return modellEnt3a(); }
std::size_t uz(int i) { return static_cast<std::size_t>(i); }
int torso() { return mb1::teil("torso"); }

std::string fmt(Str s, auto... a) {
    char b[600];
    std::snprintf(b, sizeof b, tr(s), a...);
    return b;
}

const Element* imPopup(const std::string& label) { return finde(label, "##Popup_", 0, false); }

Schritt klickPopup(const std::string& label) {
    return klickAn("\"" + label + "\" (Menue)", [label] {
        const Element* e = imPopup(label);
        if (e == nullptr) { meldeFehler("ENT3A: \"" + label + "\" steht nicht im Menue"); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    });
}

// Wie viele Elemente mit diesem Text das Menue gerade hat
int imPopupZahl(const std::string& label) {
    int n = 0;
    while (finde(label, "##Popup_", n, false) != nullptr && n < 20) { ++n; }
    return n;
}

void modellWaehlen(int modus, ModellBereich bereich) {
    modellBedienungSetzen(modus);
    mtab().bereich = bereich;
    g_app->settings.modellFeldBreite = 0.0F;
    mtab().auswahl = {torso()};
    mtab().knochenWahl = -1;
    mtab().tagWahl = -1;
    if (ImGuiWindow* fw = mbefoto::kindFenster("mtrollouts")) { ImGui::SetScrollY(fw, 0.0F); }
    g_app->modelDirty = true;
}

// --- B7: eingefrorene Zeile im Szenen-Explorer -------------------------------------------------------------------------------

void b7Schritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const bool bl = modus == 1;
        const std::string wie = bl ? " wie Blender: " : " wie 3ds Max: ";
        add(tu("ENT3A B7: Ansehen, torso eingefroren, nichts gewaehlt", [=] {
            modellWaehlen(modus, ModellBereich::Ansehen);
            modellAllesAuftauen();
            modellTeilEinfrieren(torso(), true, false);
            mtab().auswahl.clear();
            mtab().meldung.clear();
            modellExplorerZeigen(ExplorerArt::Flaeche, torso());
        }));
        add(pause(0.6));
        add(klickAn("ENT3A B7: Klick auf die eingefrorene Zeile torso", ap2::ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche))));
        add(pause(0.4));
        if (!bl) {
            add(pruefe("ENT3A B7" + wie + "der Klick im Explorer waehlt die eingefrorene Flaeche (Scene Explorer), sie bleibt eingefroren", [] {
                const ModellTab& t = mtab();
                melde("B7: gewaehlt " + std::to_string(t.auswahl.size()) + ", eingefroren " + std::to_string(modellTeilEingefroren(torso()) ? 1 : 0) + ", Meldung '" + t.meldung + "'");
                return t.auswahl.size() == 1U && t.auswahl.front() == torso() && modellTeilEingefroren(torso());
            }));
            add(tu("ENT3A B7: Maus auf die Zeile (Tooltip)", [] { setzeMaus(ap2::ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche))()); }));
            add(pause(1.4));
            add(pruefe("ENT3A B7" + wie + "der Tooltip der eingefrorenen Zeile sagt, dass sie in der Liste waehlbar ist und der Klick in der Ansicht hindurchgeht", [] {
                melde("B7: Tooltip '" + ap2::ex().tooltip.substr(0, 160) + "'");
                return ap2::ex().tooltip.find(tr(Str::EaFrostListeHint)) != std::string::npos;
            }));
            add(klickAn("ENT3A B7: Rechtsklick auf die Zeile torso", ap2::ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche)), 1));
            add(pause(0.5));
            add(pruefe("ENT3A B7" + wie + "der Rechtsklick der Zeile hat \"Auswahl auftauen\" (3ds Max: Unfreeze Scene Explorer Selection)", [] {
                return imPopup(tr(Str::EaAuswahlAuftauen)) != nullptr;
            }));
            add(klickPopup(tr(Str::EaAuswahlAuftauen)));
            add(pause(0.4));
            add(pruefe("ENT3A B7" + wie + "\"Auswahl auftauen\" taut torso auf, die Auswahl bleibt, die Meldung nennt es", [] {
                const ModellTab& t = mtab();
                melde("B7: nach Auftauen eingefroren " + std::to_string(modellTeilEingefroren(torso()) ? 1 : 0) + ", Meldung '" + t.meldung + "'");
                return !modellTeilEingefroren(torso()) && t.auswahl.size() == 1U && t.auswahl.front() == torso() && t.meldung == fmt(Str::EaMeldAufgetaut, 1);
            }));
            add(mbefoto::popupsZu());
            // die Suche kennt die Aktion auch
            add(tu("ENT3A B7: torso wieder eingefroren und gewaehlt; Alle Aktionen suchen", [] {
                modellTeilEinfrieren(torso(), true, false);
                mtab().auswahl = {torso()};
                mtab().aktionenOffen = true;
            }));
            add(pause(0.5));
            add(tippe(tr(Str::EaAuswahlAuftauen)));
            add(pause(0.5));
            add(pruefe("ENT3A B7" + wie + "\"Alle Aktionen suchen\" findet \"Auswahl auftauen\"", [] { return mtab().aktionenSichtbar && imPopup(tr(Str::EaAuswahlAuftauen)) != nullptr; }));
            add(ap8::klickLetzter(tr(Str::EaAuswahlAuftauen), "##Popup_"));
            add(pause(0.4));
            add(pruefe("ENT3A B7" + wie + "ueber die Suche aufgetaut", [] { return !modellTeilEingefroren(torso()); }));
            add(mbefoto::popupsZu());
        } else {
            add(pruefe("ENT3A B7" + wie + "im Outliner bleibt die nicht waehlbare Zeile nicht waehlbar (wie bisher), die Meldung nennt es", [] {
                const ModellTab& t = mtab();
                return t.auswahl.empty() && modellTeilEingefroren(torso()) && t.meldung.find("torso") != std::string::npos;
            }));
        }
        add(tu("ENT3A B7: aufraeumen", [] {
            modellAllesAuftauen();
            mtab().auswahl.clear();
        }));
    }
}

// --- B8: Werkzeugleiste breiter ziehen (wie Blender) -------------------------------------------------------------------------

float leistePunkte() { return ap4::fenster("mtwerkzeugleiste").GetWidth() / std::max(g_app->dpiScale * g_app->uiScale, 0.01F); }

// Die Kante der Leiste auf `punkte` Breite ziehen (vom linken Rand der Leiste gemessen)
Schritt kanteZiehen(const std::string& was, float punkte) {
    auto von = std::make_shared<ImVec2>();
    auto ziel = std::make_shared<float>(0.0F);
    return {"ENT3A B8: Kante der Werkzeugleiste ziehen - " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    const ImRect k = e3().leisteKante;
                    const ImRect l = ap4::fenster("mtwerkzeugleiste");
                    if (k.GetWidth() <= 0.0F || l.GetWidth() <= 0.0F) { meldeFehler("ENT3A B8: keine Kante an der Werkzeugleiste"); }
                    *von = k.GetWidth() > 0.0F ? k.GetCenter() : ImVec2{l.Max.x - 1.0F, l.GetCenter().y};
                    *ziel = l.Min.x + punkte * std::max(g_app->dpiScale * g_app->uiScale, 0.01F);
                    setzeMaus(*von);
                    return false;
                }
                if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                if (b >= 3 && b <= 14) {
                    const float f = static_cast<float>(b - 2) / 12.0F;
                    setzeMaus(ImVec2{von->x + (*ziel - von->x) * f, von->y});
                    return false;
                }
                if (b == 15) { io.AddMouseButtonEvent(0, false); return false; }
                return b >= 20;
            }};
}

void b8Schritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto vorher = std::make_shared<std::pair<bool, float>>();
    add(tu("ENT3A B8: wie Blender, Werkzeugleiste an, ohne Namen, schmal", [vorher] {
        *vorher = {g_app->settings.modellBlWerkzeugnamen, g_app->settings.modellBlLeisteBreite};
        modellWaehlen(1, ModellBereich::Modellieren);
        modellBl().werkzeugleiste = true;
        g_app->settings.modellBlWerkzeugnamen = false;
        g_app->settings.modellBlLeisteBreite = 0.0F;
    }));
    add(pause(0.6));
    auto schmal = std::make_shared<float>(0.0F);
    add(pruefe("ENT3A B8 wie Blender: schmale Leiste ohne Namen, an ihrer rechten Kante eine Zieh-Kante", [schmal] {
        *schmal = leistePunkte();
        melde("B8: schmal " + std::to_string(static_cast<int>(*schmal)) + " Punkte, Kante " + std::to_string(static_cast<int>(e3().leisteKante.GetWidth())));
        return !modellBl().namenGezeigt && *schmal > 10.0F && *schmal < 80.0F && e3().leisteKante.GetWidth() > 0.0F &&
               std::fabs(e3().leisteKante.Max.x - ap4::fenster("mtwerkzeugleiste").Max.x) < 2.0F;
    }));
    add(kanteZiehen("auf 100 Punkte", 100.0F));
    add(pause(0.5));
    add(pruefe("ENT3A B8 wie Blender: auf 100 Punkte gezogen - breiter, noch ohne Namen (Blender zeigt sie erst ueber 120)", [schmal] {
        const float p = leistePunkte();
        melde("B8: 100 -> " + std::to_string(p) + " Punkte, Einstellung " + std::to_string(g_app->settings.modellBlLeisteBreite));
        return p > *schmal + 40.0F && std::fabs(p - 100.0F) < 6.0F && !modellBl().namenGezeigt;
    }));
    add(kanteZiehen("auf 160 Punkte", 160.0F));
    add(pause(0.5));
    add(foto("ent3a_bl_leiste_breit"));
    add(pruefe("ENT3A B8 wie Blender: auf 160 Punkte gezogen - die Leiste zeigt die Werkzeugnamen; die Breite steht in den Einstellungen", [] {
        const float p = leistePunkte();
        Settings neu;
        const bool datei = readSettings(writeSettings(g_app->settings), neu) && std::fabs(neu.modellBlLeisteBreite - g_app->settings.modellBlLeisteBreite) < 0.11F;
        melde("B8: 160 -> " + std::to_string(p) + " Punkte, Namen " + std::to_string(modellBl().namenGezeigt ? 1 : 0) + ", Datei " + std::to_string(datei ? 1 : 0));
        return std::fabs(p - 160.0F) < 6.0F && modellBl().namenGezeigt && g_app->settings.modellBlLeisteBreite > 120.0F && datei && !g_app->settings.modellBlWerkzeugnamen;
    }));
    add(Schritt{"ENT3A B8: Doppelklick auf die Kante", [](int b) {
                    ImGuiIO& io = ImGui::GetIO();
                    if (b == 0) {
                        setzeMaus(e3().leisteKante.GetCenter());
                        return false;
                    }
                    if (b == 2 || b == 4) { io.AddMouseButtonEvent(0, true); return false; }
                    if (b == 3 || b == 5) { io.AddMouseButtonEvent(0, false); return false; }
                    return b >= 9;
                }});
    add(pause(0.5));
    add(pruefe("ENT3A B8 wie Blender: Doppelklick auf die Kante - wieder die schmale Leiste ohne Namen", [schmal] {
        return g_app->settings.modellBlLeisteBreite == 0.0F && !modellBl().namenGezeigt && std::fabs(leistePunkte() - *schmal) < 1.0F;
    }));
    add(tu("ENT3A B8: wie 3ds Max", [] {
        g_app->settings.modellBlLeisteBreite = 150.0F;
        modellWaehlen(0, ModellBereich::Modellieren);
    }));
    add(pause(0.5));
    add(pruefe("ENT3A B8 wie 3ds Max: die linke Spalte bleibt der Szenen-Explorer in seiner Breite (die Breite der Blender-Leiste gilt dort nicht)", [] {
        return modellLinksSpalte() == kModellSpalte;
    }));
    add(tu("ENT3A B8: Einstellungen zurueck", [vorher] {
        g_app->settings.modellBlWerkzeugnamen = vorher->first;
        g_app->settings.modellBlLeisteBreite = vorher->second;
    }));
}

// --- B9: Seitenleiste bei Knochen (wie Blender) ------------------------------------------------------------------------------

// Der Wert eines Felds der Seitenleiste (seiteText: "Name=Wert|...")
std::string seitenWert(const std::string& name) {
    const std::string& t = modellBl().seiteText;
    const std::size_t a = t.find(name + "=");
    if (a == std::string::npos) { return {}; }
    const std::size_t e = t.find('|', a);
    return t.substr(a + name.size() + 1, e == std::string::npos ? std::string::npos : e - a - name.size() - 1);
}

// Die Basis des Knochens wie Blender (traeger^-1 * welt), unabhaengig von der Seitenleiste gerechnet (ap4::lokalMatrix): die Drehung
// mit normierten Spalten und der Ort
bool basisVon(int k, float r[3][3], float ort[3]) {
    if (k < 0 || uz(k) >= g_app->anim.bones.size() || uz(k) >= g_app->modellWelt.size()) { return false; }
    const BoneMatrix b = ap4::lokalMatrix(k);
    for (int c = 0; c < 3; ++c) {
        const float l = std::sqrt(b.m[0][c] * b.m[0][c] + b.m[1][c] * b.m[1][c] + b.m[2][c] * b.m[2][c]);
        for (int z = 0; z < 3; ++z) { r[z][c] = l > 1e-6F ? b.m[z][c] / l : 0.0F; }
    }
    for (int z = 0; z < 3; ++z) { ort[z] = b.m[z][3]; }
    return true;
}

// Die Werte der Felder hinter einem Feld der Seitenleiste (seiteText "Name=Wert|..."): `name` und die naechsten `zahl` - 1 Werte
std::string seitenFolge(const std::string& name, int zahl) {
    const std::string& t = modellBl().seiteText;
    std::size_t a = t.find(name + "=");
    if (a == std::string::npos) { return {}; }
    std::string aus;
    for (int i = 0; i < zahl && a != std::string::npos; ++i) {
        const std::size_t gl = t.find('=', a);
        const std::size_t e = t.find('|', gl);
        aus += (i > 0 ? "/" : "") + t.substr(gl + 1, e == std::string::npos ? std::string::npos : e - gl - 1);
        a = e == std::string::npos ? e : e + 1;
    }
    return aus;
}

std::string zahl3(float v) {
    char b[32];
    std::snprintf(b, sizeof b, "%.3f", std::fabs(v) < 0.0005F ? 0.0 : static_cast<double>(v));
    return b;
}

void b9Schritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct Stand {
        float e[3]{};
        float ort[3]{};
        int schritte = 0;
        int knochen = -1;
    };
    auto st = std::make_shared<Stand>();
    add(tu("ENT3A B9: wie Blender, Animation, Rig, Griff Becken, Seitenleiste an", [st] {
        modellWaehlen(1, ModellBereich::Animation);
        mtab().auswahl.clear();
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
        if (!modellRigDa()) { (void)modellAnimRigErstellen(false); }
        modellRigGriffWaehlen(ap4::griffNach(modell::GriffArt::Becken));
        modellBl().seitenleiste = true;
        modellBl().seitenReiter = 0;
        st->knochen = ap4::griffKnochen();
        if (st->knochen >= 0) { e3().drehModus.erase(g_app->anim.bones[uz(st->knochen)].name); }
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(pruefe("ENT3A B9 wie Blender: Ort, Drehung als Quaternion (W, X, Y, Z - Blenders Vorgabe), Drehmodus und Massstab (nur ablesbar)", [] {
        const auto* w = ap4::seitenFeld("dreh3");
        const auto* m = ap4::seitenFeld("drehmodus");
        const auto* sk = ap4::seitenFeld("skal0");
        melde("B9: Seite '" + modellBl().seiteText.substr(0, 300) + "'");
        return w != nullptr && w->eingebbar && m != nullptr && sk != nullptr && !sk->eingebbar && seitenWert(tr(Str::EaDrehModus)) == tr(Str::EaModusQuat);
    }));
    add(pruefe("ENT3A B9 wie Blender: die Werte sind lokal - die Quaternion der Basis (Ruhelage^-1 der Pose), nicht die Drehung in Weltachsen", [st] {
        float r[3][3];
        float ort[3];
        if (!basisVon(st->knochen, r, ort)) { return false; }
        float q[4];
        modell::quatAusMatrix(r, q);
        const std::string soll = zahl3(q[0]) + "/" + zahl3(q[1]) + "/" + zahl3(q[2]) + "/" + zahl3(q[3]);
        const std::string ist = seitenFolge(tr(Str::BzDrehung) + std::string(" W"), 4);
        melde("B9: Quaternion soll " + soll + ", Seitenleiste " + ist);
        return ist == soll;
    }));
    add(tu("ENT3A B9: Grundpose fuer alle Knochen (ein Schritt des Dokuments)", [] { (void)modellBindepose(true); }));
    add(pause(0.6));
    add(pruefe("ENT3A B9 wie Blender: in der Grundpose Ort 0, Quaternion (1, 0, 0, 0), Massstab 1 - wie ein Knochen in seiner Ruhelage in Blender", [] {
        const std::string w = seitenWert(tr(Str::BzDrehung) + std::string(" W"));
        const std::string ort = seitenWert(tr(Str::BzOrt) + std::string(" X"));
        const std::string sk = seitenWert(tr(Str::EaSkalierung) + std::string(" X"));
        melde("B9: Grundpose W " + w + ", Ort X " + ort + ", Massstab X " + sk);
        return w == "1.000" && ort == "0.00" && sk == "1.000";
    }));
    add(tu("ENT3A B9: Grundpose zurueck (Strg+Z)", [] { (void)modellRueckgaengig(); }));
    add(pause(0.5));
    // Drehmodus XYZ Euler ueber die Auswahl
    add(klickAn("ENT3A B9: die Auswahl des Drehmodus", ap4::feldOrt("drehmodus")));
    add(pause(0.5));
    add(klickAn("ENT3A B9: XYZ Euler in der Auswahl", [] {
        const Element* e = finde(tr(Str::EaModusXyz), "##Combo_", 0, false);
        if (e == nullptr) { meldeFehler("ENT3A B9: \"XYZ Euler\" steht nicht in der Auswahl"); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    }));
    add(pause(0.6));
    add(pruefe("ENT3A B9 wie Blender: XYZ Euler - drei Felder X, Y, Z in Grad, Werte wie Blenders Matrix.to_euler('XYZ') der Basis", [st] {
        float r[3][3];
        float ort[3];
        if (!basisVon(st->knochen, r, ort) || ap4::seitenFeld("dreh3") != nullptr || ap4::seitenFeld("dreh2") == nullptr) { return false; }
        float e[3];
        modell::eulerAusMatrix(r, modell::DrehModus::Xyz, e);
        char soll[100];
        std::snprintf(soll, sizeof soll, "%.1f\xc2\xb0", static_cast<double>(e[0] * 180.0F / kPi));
        const std::string ist = seitenWert(tr(Str::BzDrehung) + std::string(" X"));
        melde("B9: Euler X soll " + std::string(soll) + ", Seitenleiste " + ist + ", Modus " + seitenWert(tr(Str::EaDrehModus)));
        return seitenWert(tr(Str::EaDrehModus)) == tr(Str::EaModusXyz) && (ist == soll || (std::string(soll) == "-0.0\xc2\xb0" && ist == "0.0\xc2\xb0"));
    }));
    add(tu("ENT3A B9: Euler und Ort der Basis merken", [st] {
        float r[3][3];
        if (!basisVon(st->knochen, r, st->ort)) { return; }
        modell::eulerAusMatrix(r, modell::DrehModus::Xyz, st->e);
        st->schritte = modellBl().seitenSchritte;
    }));
    add(klickAn("ENT3A B9: Klick auf Rotation X des Beckens", ap4::feldOrt("dreh0")));
    add(pause(0.3));
    add(Schritt{"ENT3A B9: Euler X + 15 Grad tippen", [st](int b) {
                    if (b == 0) { return false; }
                    ImGuiIO& io = ImGui::GetIO();
                    if (b == 1) { io.AddKeyEvent(ImGuiMod_Ctrl, true); io.AddKeyEvent(ImGuiKey_A, true); return false; }
                    if (b == 2) { io.AddKeyEvent(ImGuiKey_A, false); io.AddKeyEvent(ImGuiMod_Ctrl, false); return false; }
                    if (b == 3) {
                        char z[32];
                        std::snprintf(z, sizeof z, "%.1f", static_cast<double>(st->e[0] * 180.0F / kPi + 15.0F));
                        io.AddInputCharactersUTF8(z);
                        return false;
                    }
                    return b >= 6;
                }});
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.6));
    add(pruefe("ENT3A B9 wie Blender: Euler X lokal + 15 Grad eingetippt - die Basis dreht um X, Y und Z bleiben, ein Rueckgaengig-Schritt", [st] {
        float r[3][3];
        float ort[3];
        if (!basisVon(st->knochen, r, ort)) { return false; }
        float e[3];
        modell::eulerAusMatrix(r, modell::DrehModus::Xyz, e);
        const auto grad = [](float a) { return a * 180.0F / kPi; };
        const auto winkel = [](float a, float b) { return std::fabs(std::remainder(a - b, 360.0F)); };
        char z[220];
        std::snprintf(z, sizeof z, "B9: Euler vorher %.2f/%.2f/%.2f, nachher %.2f/%.2f/%.2f, Schritte %d", static_cast<double>(grad(st->e[0])), static_cast<double>(grad(st->e[1])),
                      static_cast<double>(grad(st->e[2])), static_cast<double>(grad(e[0])), static_cast<double>(grad(e[1])), static_cast<double>(grad(e[2])),
                      modellBl().seitenSchritte - st->schritte);
        melde(z);
        return winkel(grad(e[0]), grad(st->e[0]) + 15.0F) < 0.15F && winkel(grad(e[1]), grad(st->e[1])) < 0.15F && winkel(grad(e[2]), grad(st->e[2])) < 0.15F &&
               modellBl().seitenSchritte == st->schritte + 1;
    }));
    add(tu("ENT3A B9: Fokus im Model-Tab", [] { mtab().fokus = true; }));
    add(ap4::strgZ());
    add(pause(0.5));
    add(tu("ENT3A B9: Stand merken", [st] { st->schritte = modellBl().seitenSchritte; }));
    add(klickAn("ENT3A B9: Klick auf Ort X des Beckens", ap4::feldOrt("ort0")));
    add(pause(0.3));
    add(Schritt{"ENT3A B9: Ort X + 2 tippen", [st](int b) {
                    if (b == 0) { return false; }
                    ImGuiIO& io = ImGui::GetIO();
                    if (b == 1) { io.AddKeyEvent(ImGuiMod_Ctrl, true); io.AddKeyEvent(ImGuiKey_A, true); return false; }
                    if (b == 2) { io.AddKeyEvent(ImGuiKey_A, false); io.AddKeyEvent(ImGuiMod_Ctrl, false); return false; }
                    if (b == 3) {
                        char z[32];
                        std::snprintf(z, sizeof z, "%.2f", static_cast<double>(st->ort[0] + 2.0F));
                        io.AddInputCharactersUTF8(z);
                        return false;
                    }
                    return b >= 6;
                }});
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.6));
    add(pruefe("ENT3A B9 wie Blender: Ort X lokal + 2 eingetippt - in den Achsen der Ruhelage, Y und Z bleiben, ein Rueckgaengig-Schritt", [st] {
        float r[3][3];
        float ort[3];
        if (!basisVon(st->knochen, r, ort)) { return false; }
        char z[200];
        std::snprintf(z, sizeof z, "B9: Ort vorher %.3f/%.3f/%.3f, nachher %.3f/%.3f/%.3f", static_cast<double>(st->ort[0]), static_cast<double>(st->ort[1]),
                      static_cast<double>(st->ort[2]), static_cast<double>(ort[0]), static_cast<double>(ort[1]), static_cast<double>(ort[2]));
        melde(z);
        return std::fabs(ort[0] - (st->ort[0] + 2.0F)) < 0.02F && std::fabs(ort[1] - st->ort[1]) < 0.02F && std::fabs(ort[2] - st->ort[2]) < 0.02F &&
               modellBl().seitenSchritte == st->schritte + 1;
    }));
    add(tu("ENT3A B9: Fokus im Model-Tab", [] { mtab().fokus = true; }));
    add(ap4::strgZ());
    add(pause(0.4));
    add(tu("ENT3A B9: wie 3ds Max", [] { modellWaehlen(0, ModellBereich::Animation); }));
    add(pause(0.4));
    add(pruefe("ENT3A B9 wie 3ds Max: keine Seitenleiste (wie bisher)", [] { return modellSeitenleisteBreite(4000.0F) == 0.0F; }));
    add(tu("ENT3A B9: aufraeumen", [st] {
        modellRigGriffWaehlen(-1);
        modellBl().seitenleiste = false;
        if (st->knochen >= 0) { e3().drehModus.erase(g_app->anim.bones[uz(st->knochen)].name); }
    }));
}

// --- B10: Strg+Umschalt+Z ------------------------------------------------------------------------------------------------------

std::string sichten() {
    std::string t;
    char z[200];
    for (const ModellAnsicht& a : mtab().ansichten) {
        std::snprintf(z, sizeof z, "%d:%.2f,%.2f,%.2f,%.2f;", static_cast<int>(a.art), static_cast<double>(a.mitte[0]), static_cast<double>(a.mitte[1]),
                      static_cast<double>(a.mitte[2]), static_cast<double>(a.halbHoehe));
        t += z;
    }
    std::snprintf(z, sizeof z, "cam %.2f,%.2f,%.2f d %.2f", static_cast<double>(g_app->modelCam.pos[0]), static_cast<double>(g_app->modelCam.pos[1]),
                  static_cast<double>(g_app->modelCam.pos[2]), static_cast<double>(g_app->modelOrbit.distance));
    return t + z;
}

void b10Schritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto aus = std::make_shared<std::string>();
    auto nach = std::make_shared<std::string>();
    const auto wiederholbar = [](const char* was) {
        return tu(std::string("ENT3A B10: ein Schritt zum Wiederholen (") + was + ")", [] {
            mtab().dok.aendern("ENT3A Probe", [](modell::Inhalt& x) { x.name += " "; });
            (void)modellRueckgaengig();
            mtab().fokus = true;
        });
    };
    add(tu("ENT3A B10: wie 3ds Max, Ansehen, vier Ansichten, hineingezoomt", [] {
        modellWaehlen(0, ModellBereich::Ansehen);
        mtab().auswahl.clear();
        mtab().vierAnsichten = true;
        for (int i = 0; i < 3; ++i) { modellNavZoomSchritt(2.0F); }
    }));
    add(pause(1.2));
    add(wiederholbar("wie 3ds Max"));
    add(tu("ENT3A B10: Stand der Ansichten merken", [aus] { *aus = sichten(); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, "Strg+Umschalt+Z"));
    add(pause(1.5));
    add(pruefe("ENT3A B10 wie 3ds Max: Strg+Umschalt+Z wiederholt nicht (der Schritt bleibt zum Wiederholen) und aendert die Ansichten", [aus, nach] {
        *nach = sichten();
        melde("B10: vorher " + *aus + " | nachher " + *nach + ", wiederholbar " + std::to_string(mtab().dok.kannWiederholen() ? 1 : 0));
        return mtab().dok.kannWiederholen() && *nach != *aus;
    }));
    add(tu("ENT3A B10: Zoom Extents All direkt (modellNavGrenzen 2)", [] { modellNavGrenzen(2); }));
    add(pause(1.5));
    add(pruefe("ENT3A B10 wie 3ds Max: Strg+Umschalt+Z war Zoom Extents All - noch einmal Zoom Extents All aendert keine Ansicht", [nach] {
        const std::string jetzt = sichten();
        if (jetzt != *nach) { melde("B10: nach Zoom Extents All " + jetzt); }
        return jetzt == *nach;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Y, "Strg+Y"));
    add(pause(0.4));
    add(pruefe("ENT3A B10 wie 3ds Max: Strg+Y wiederholt", [] { return !mtab().dok.kannWiederholen(); }));
    add(pruefe("ENT3A B10 wie 3ds Max: die Tastenliste hat Strg+Umschalt+Z genau einmal - als Zoom Extents All", [] {
        int n = 0;
        bool richtig = false;
        for (int i = 0; i < modellTasteZahl(); ++i) {
            if (modellTasteChord(i) == (ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z)) {
                ++n;
                richtig = std::string(modellTasteText(i)) == tr(Str::MnGrenzenAlle);
            }
        }
        return n == 1 && richtig;
    }));
    add(tu("ENT3A B10: Alle Aktionen suchen", [] {
        mtab().aktionenOffen = true;
        mtab().fokus = true;
    }));
    add(pause(0.5));
    add(tippe("ctrl+shift+z"));
    add(pause(0.5));
    add(pruefe("ENT3A B10 wie 3ds Max: die Suche nach \"Ctrl+Shift+Z\" findet Zoom Extents All einmal, nicht Wiederholen", [] {
        melde("B10: Suche Max - Zoom Extents All " + std::to_string(imPopupZahl(tr(Str::MnGrenzenAlle))) + "x, Wiederholen " + std::to_string(imPopupZahl(tr(Str::EaRedoBl))) + "x");
        return mtab().aktionenSichtbar && imPopupZahl(tr(Str::MnGrenzenAlle)) == 1 && imPopupZahl(tr(Str::EaRedoBl)) == 0;
    }));
    add(mbefoto::popupsZu());
    add(tu("ENT3A B10: vier Ansichten aus", [] { mtab().vierAnsichten = false; }));
    // wie Blender
    add(tu("ENT3A B10: wie Blender", [] { modellWaehlen(1, ModellBereich::Ansehen); }));
    add(pause(0.5));
    add(wiederholbar("wie Blender"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, "Strg+Umschalt+Z"));
    add(pause(0.4));
    add(pruefe("ENT3A B10 wie Blender: Strg+Umschalt+Z wiederholt (wie Blender ed.redo)", [] { return !mtab().dok.kannWiederholen(); }));
    add(tu("ENT3A B10: Alle Aktionen suchen (F3)", [] {
        mtab().aktionenOffen = true;
        mtab().fokus = true;
    }));
    add(pause(0.5));
    add(tippe("ctrl+shift+z"));
    add(pause(0.5));
    add(pruefe("ENT3A B10 wie Blender: die Suche nennt Wiederholen mit Strg+Umschalt+Z (vorher stand dort Strg+Y)", [] {
        return mtab().aktionenSichtbar && imPopupZahl(tr(Str::EaRedoBl)) == 1;
    }));
    add(mbefoto::popupsZu());
}

// --- B11 / B13: Auge fuer Edit Poly und Skin, Zeile Unwrap UVW ---------------------------------------------------------------------

std::string zeilenText() { return ap13s::zeilen(); }

void b11Schritte(std::vector<Schritt>& s, const std::string& modell) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto geladen = std::make_shared<mb1::AnsichtBild>();
    auto geaendert = std::make_shared<mb1::AnsichtBild>();
    auto stand = std::make_shared<int>(0);
    auto bytes = std::make_shared<std::string>();
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    add(tu("ENT3A B11: wie 3ds Max, Modellieren, torso, Stapel leer, Animation Bild 0 angehalten", [] {
        ap13s::stapelLeeren();
        modellWaehlen(0, ModellBereich::Modellieren);
        md1d::waehle(modell::wof::PAuswahl{}, AuswahlArt::Dreieck);
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
        setzeMaus(kv5::fokusPunkt());
    }));
    add(pause(1.0));
    add(tu("ENT3A B11: Bild der Ansicht wie geladen", [geladen] { (void)mb1::ansichtLesen(*geladen); }));
    // B13: die Zeile Unwrap UVW
    add(pruefe("ENT3A B13 wie 3ds Max: von oben Skin, Unwrap UVW, Edit Poly (gewaehlt), GLM-Flaeche torso", [] {
        melde("B13: Zeilen " + zeilenText());
        return zeilenText() == ap13s::liste({tr(Str::MgStapelSkin), tr(Str::EaUnwrap), tr(Str::MgStapelPoly), ap13s::nBasis("torso")});
    }));
    add(pruefe("ENT3A B11 wie 3ds Max: Edit Poly und Skin haben ein Auge (an)", [] {
        const Element* p = ap13s::imRoll(tr(Str::EaPolyAn));
        const Element* k = ap13s::imRoll(tr(Str::EaSkinAn));
        return p != nullptr && k != nullptr && (p->flags & ImGuiItemStatusFlags_Checked) != 0 && (k->flags & ImGuiItemStatusFlags_Checked) != 0;
    }));
    add(tu("ENT3A B11: alle Ecken von torso um 3 nach vorn (ein Schritt des Dokuments, wie ein Werkzeug des Modellierens)", [] {
        mtab().dok.aendern("ENT3A Ecken", [](modell::Inhalt& x) {
            for (modell::Netz& n : x.teile[uz(x.teilIndex("torso"))].stufen) {
                for (modell::Ecke& e : n.ecken) {
                    e.ort[1] += 3.0F;
                    e.ausDatei = false;
                }
                n.ausDatei = false;
            }
        });
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(tu("ENT3A B11: Bild mit dem geaenderten torso; Stand und Rueckgaengig merken", [geaendert, stand, bytes] {
        (void)mb1::ansichtLesen(*geaendert);
        *stand = mtab().dok.stand();
        std::string b;
        (void)modell::glmSchreiben(modellInhaltGezeigt(), b);
        *bytes = b;
    }));
    add(pruefe("ENT3A B11: das Geaenderte ist im Bild", [geladen, geaendert] { return mb1::unterschied(geladen->px, geaendert->px) > 200; }));
    add(ap13s::klickRoll(tr(Str::EaPolyAn)));
    add(pause(0.8));
    add(foto("ent3a_max_polyaus"));
    add(pruefe("ENT3A B11 wie 3ds Max: Edit Poly aus - die Ansicht zeigt torso wie geladen (Bild gleich dem nach dem Laden), das Dokument bleibt", [geladen, stand] {
        mb1::AnsichtBild jetzt;
        (void)mb1::ansichtLesen(jetzt);
        const int anders = mb1::unterschied(geladen->px, jetzt.px);
        const Element* p = ap13s::imRoll(tr(Str::EaPolyAn));
        melde("B11: Edit Poly aus - Bildpunkte anders als geladen " + std::to_string(anders) + ", Stand " + std::to_string(mtab().dok.stand()) + "/" + std::to_string(*stand) +
              ", Meldung '" + mtab().meldung + "'");
        return modellEnt3aPolyAus(torso()) && anders == 0 && mtab().dok.stand() == *stand && mtab().dok.rueckgaengigName() == "ENT3A Ecken" && p != nullptr &&
               (p->flags & ImGuiItemStatusFlags_Checked) == 0;
    }));
    add(pruefe("ENT3A B11 wie 3ds Max: Edit Poly aus aendert nur die Anzeige - Speichern schreibt dieselben Bytes wie mit Auge an", [bytes] {
        std::string b;
        (void)modell::glmSchreiben(modellInhaltGezeigt(), b);
        const std::string datei = ap13s::speichernUndLesen("ent3a_polyaus.glm");
        return !datei.empty() && datei == *bytes && b == *bytes;
    }));
    add(ap13s::klickRoll(tr(Str::EaPolyAn)));
    add(pause(0.8));
    add(pruefe("ENT3A B11 wie 3ds Max: Edit Poly wieder an - wieder das Geaenderte", [geaendert] {
        mb1::AnsichtBild jetzt;
        (void)mb1::ansichtLesen(jetzt);
        return !modellEnt3aPolyAus(torso()) && mb1::unterschied(geaendert->px, jetzt.px) == 0;
    }));
    // Skin: in einer Pose
    auto pose = std::make_shared<mb1::AnsichtBild>();
    auto welt = std::make_shared<std::vector<BoneMatrix>>();
    add(tu("ENT3A B11: Rueckgaengig (Ecken), BOTH_WALK1 Bild 10", [] {
        (void)modellRueckgaengig();
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        g_app->animFrame += 10;
        g_app->animPlaying = false;
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(tu("ENT3A B11: Bild und Knochen der Pose", [pose, welt, stand] {
        (void)mb1::ansichtLesen(*pose);
        *welt = g_app->modellWelt;
        *stand = mtab().dok.stand();
    }));
    // eine Ecke von torso: auf dem Schirm gegen ihren Ort in der Datei (Bindepose)
    const auto eckeAbstand = [] {
        const int t = torso();
        const GlmSurface& g = mtab().anzeige.surfaces[uz(t)];
        if (g.verts.empty()) { return -1.0F; }
        const int k = static_cast<int>(g.verts.size() / 2U);
        ImVec2 gezeigt;
        ImVec2 datei;
        if (!modellEckeAufSchirm(t, k, gezeigt) || !modellAufSchirm(mtab().ansichten[mtab().aktiv], g.verts[uz(k)].xyz, datei)) { return -1.0F; }
        return std::hypot(gezeigt.x - datei.x, gezeigt.y - datei.y);
    };
    add(pruefe("ENT3A B11: in der Pose liegt eine Ecke von torso nicht am Ort der Datei", [eckeAbstand] {
        const float d = eckeAbstand();
        melde("B11: Pose - Ecke " + std::to_string(d) + " Bildpunkte vom Ort der Datei");
        return d > 3.0F;
    }));
    add(ap13s::klickRoll(tr(Str::EaSkinAn)));
    add(pause(0.8));
    add(foto("ent3a_max_skinaus"));
    add(pruefe("ENT3A B11 wie 3ds Max: Skin aus - die Figur steht in der Bindepose (die Ecke am Ort der Datei), die Knochen behalten ihre Pose, das Dokument bleibt",
               [eckeAbstand, pose, welt, stand] {
                   mb1::AnsichtBild jetzt;
                   (void)mb1::ansichtLesen(jetzt);
                   const float d = eckeAbstand();
                   bool gleich = g_app->modellWelt.size() == welt->size();
                   for (std::size_t i = 0; gleich && i < welt->size(); ++i) {
                       for (int z = 0; z < 3; ++z) {
                           for (int c = 0; c < 4; ++c) { gleich = gleich && g_app->modellWelt[i].m[z][c] == (*welt)[i].m[z][c]; }
                       }
                   }
                   const int anders = mb1::unterschied(pose->px, jetzt.px);
                   melde("B11: Skin aus - Ecke " + std::to_string(d) + " Bildpunkte vom Ort der Datei, Knochen gleich " + std::to_string(gleich ? 1 : 0) + ", Bildpunkte anders " +
                         std::to_string(anders) + ", Meldung '" + mtab().meldung + "'");
                   return modellEnt3aSkinAus() && d >= 0.0F && d < 0.6F && gleich && anders > 200 && mtab().dok.stand() == *stand && mtab().meldung == tr(Str::EaMeldSkinAus);
               }));
    add(tu("ENT3A B11: wie Blender (die Augen des Stapels gelten dort nicht)", [] { modellBedienungSetzen(1); }));
    add(pause(0.6));
    add(pruefe("ENT3A B11 wie Blender: Skin aus gilt nicht - die Figur folgt den Knochen, der Stapel hat keine Zeilen Skin, Edit Poly, Unwrap UVW", [eckeAbstand] {
        const float d = eckeAbstand();
        const std::string z = zeilenText();
        return !modellEnt3aSkinAus() && d > 3.0F && z.find(tr(Str::EaUnwrap)) == std::string::npos && z.find(tr(Str::MgStapelSkin)) == std::string::npos;
    }));
    add(tu("ENT3A B11: wie 3ds Max", [] { modellBedienungSetzen(0); }));
    add(pause(0.5));
    add(ap13s::klickRoll(tr(Str::EaSkinAn)));
    add(pause(0.6));
    add(pruefe("ENT3A B11 wie 3ds Max: Skin wieder an - das Bild der Pose", [pose] {
        mb1::AnsichtBild jetzt;
        (void)mb1::ansichtLesen(jetzt);
        return !modellEnt3aSkinAus() && mb1::unterschied(pose->px, jetzt.px) == 0;
    }));
    // B13: Klick auf Unwrap UVW, Suche
    add(ap13s::klickRoll(tr(Str::EaUnwrap)));
    add(pause(0.5));
    add(pruefe("ENT3A B13 wie 3ds Max: ein Klick auf Unwrap UVW wechselt in den Bereich UV", [] { return mtab().bereich == ModellBereich::Uv; }));
    add(tu("ENT3A B13: Modellieren, Alle Aktionen suchen", [] {
        mtab().bereich = ModellBereich::Modellieren;
        mtab().aktionenOffen = true;
        mtab().fokus = true;
    }));
    add(pause(0.5));
    add(tippe("unwrap"));
    add(pause(0.5));
    add(pruefe("ENT3A B13 wie 3ds Max: \"Alle Aktionen suchen\" findet Unwrap UVW", [] { return mtab().aktionenSichtbar && imPopup(tr(Str::EaUnwrap)) != nullptr; }));
    add(ap8::klickLetzter(tr(Str::EaUnwrap), "##Popup_"));
    add(pause(0.4));
    add(pruefe("ENT3A B13 wie 3ds Max: ueber die Suche in den Bereich UV", [] { return mtab().bereich == ModellBereich::Uv; }));
    add(mbefoto::popupsZu());
    add(tu("ENT3A B11: zurueck nach Modellieren", [] { mtab().bereich = ModellBereich::Modellieren; }));
}

// --- B12: Begleitdatei ------------------------------------------------------------------------------------------------------

std::string pfadIm(const std::string& name) { return mb1::fotoOrdner() + "\\" + name; }

void b12Schritte(std::vector<Schritt>& s, const std::string& modell) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto glm = std::make_shared<std::string>();
    auto stapel = std::make_shared<std::string>();
    auto begleit = std::make_shared<std::string>();
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    add(pruefe("ENT3A B12: der Selbsttest hat einen Ordner zum Speichern (BHED_FOTOS)", [] { return !mb1::fotoOrdner().empty(); }));
    add(tu("ENT3A B12: wie 3ds Max, Modellieren, torso: Symmetry (in die Datei) und TurboSmooth (nur Vorschau, 2 Stufen)", [] {
        ap13s::stapelLeeren();
        modellWaehlen(0, ModellBereich::Modellieren);
        (void)modellStapelHinzufuegen(ModArt::Spiegeln);
        (void)modellStapelHinzufuegen(ModArt::Glaetten);
        mtab().dok.aendern("ENT3A Stufen", [](modell::Inhalt& x) { x.teile[uz(x.teilIndex("torso"))].stapel.back().stufen = 2; });
        // E3: der Drehmodus zweier Knochen kommt mit in die Begleitdatei
        e3().drehModus.clear();
        e3().drehModus["pelvis"] = static_cast<int>(modell::DrehModus::Xyz);
        e3().drehModus["cranium"] = static_cast<int>(modell::DrehModus::AchseWinkel);
        std::error_code ec;
        std::filesystem::remove(pfadIm("ent3a_b12.glm.stack"), ec);
    }));
    add(pause(0.6));
    add(pruefe("ENT3A B12: Speichern schreibt die .glm wie ohne Begleitdatei (Byte fuer Byte glmSchreiben des Gezeigten) und daneben model.glm.stack", [glm, stapel, begleit] {
        std::string soll;
        (void)modell::glmSchreiben(modellInhaltGezeigt(), soll);
        *stapel = ap13s::stapelText(torso());
        *glm = ap13s::speichernUndLesen("ent3a_b12.glm");
        *begleit = slurp(pfadIm("ent3a_b12.glm.stack"));
        melde("B12: .glm " + std::to_string(glm->size()) + " Bytes (soll " + std::to_string(soll.size()) + "), Begleitdatei " + std::to_string(begleit->size()) + " Bytes, Stapel " +
              *stapel + ", Meldung '" + mtab().meldung + "'");
        return !glm->empty() && *glm == soll && begleit->rfind("JKSTUDIO-STACK ", 0) == 0 && begleit->find("\nbasis ") != std::string::npos &&
               mtab().meldung.find(fmt(Str::EaBegleitGeschrieben, "ent3a_b12.glm.stack")) != std::string::npos;
    }));
    add(tu("ENT3A B12: ein anderes Modell, dann die gespeicherte .glm von der Platte oeffnen", [] { modellVonPlatteOeffnen(pfadIm("ent3a_b12.glm")); }));
    add(pause(1.2));
    add(pruefe("ENT3A B12: nach dem Oeffnen ist der Stapel wieder da (gleiche Eintraege), ohne Rueckgaengig-Schritt, die Meldung nennt es", [stapel] {
        const std::string jetzt = mtab().dok.inhalt().teilIndex("torso") >= 0 ? ap13s::stapelText(torso()) : std::string("-");
        melde("B12: geoeffnet " + g_app->modelPath + ", Stapel " + jetzt + " (vorher " + *stapel + "), Eintraege " + std::to_string(e3().begleitEintraege) + ", Meldung '" +
              mtab().meldung + "'");
        return jetzt == *stapel && e3().begleitEintraege == 2 && !mtab().dok.kannRueckgaengig() && mtab().meldung == fmt(Str::EaBegleitGelesen, "ent3a_b12.glm.stack", 2);
    }));
    add(pruefe("ENT3A E3: nach dem Oeffnen sind die Drehmodi wieder da (pelvis XYZ Euler, cranium Achsenwinkel), die Begleitdatei nennt sie", [] {
        const auto& d = e3().drehModus;
        const auto modus = [&](const char* k) { return d.count(k) != 0U ? d.at(k) : -1; };
        const std::string datei = slurp(pfadIm("ent3a_b12.glm.stack"));
        melde("E3: Drehmodi pelvis " + std::to_string(modus("pelvis")) + ", cranium " + std::to_string(modus("cranium")) + ", in der Datei " +
              std::to_string(datei.find("\ndreh ") != std::string::npos ? 1 : 0));
        return modus("pelvis") == static_cast<int>(modell::DrehModus::Xyz) && modus("cranium") == static_cast<int>(modell::DrehModus::AchseWinkel) && d.size() == 2U &&
               datei.find("\ndreh ") != std::string::npos;
    }));
    add(tu("ENT3A B12: torso waehlen", [] { modellWaehlen(0, ModellBereich::Modellieren); }));
    add(pause(0.6));
    add(pruefe("ENT3A B12: erneutes Speichern schreibt dieselbe .glm Byte fuer Byte", [glm] {
        const std::string wieder = ap13s::speichernUndLesen("ent3a_b12.glm");
        melde("B12: erneut " + std::to_string(wieder.size()) + " Bytes");
        return wieder == *glm;
    }));
    add(tu("ENT3A B12: die Begleitdatei beschaedigen (abgeschnitten) und die .glm oeffnen", [begleit] {
        std::string fehler;
        (void)sicherSchreiben(pfadIm("ent3a_b12.glm.stack"), begleit->substr(0, begleit->size() / 4), &fehler);
        modellVonPlatteOeffnen(pfadIm("ent3a_b12.glm"));
    }));
    add(pause(1.2));
    add(pruefe("ENT3A B12: beschaedigte Begleitdatei - das Modell laedt ohne Stapel, die Meldung nennt den Grund", [] {
        melde("B12: beschaedigt - Teile " + std::to_string(mtab().dok.inhalt().teile.size()) + ", Meldung '" + mtab().meldung + "'");
        return !mtab().dok.inhalt().teile.empty() && modell::stapelLeer(mtab().dok.inhalt()) &&
               mtab().meldung == fmt(Str::EaBegleitFehler, "ent3a_b12.glm.stack", tr(Str::EaBegleitGrundKaputt));
    }));
    add(tu("ENT3A B12: die Begleitdatei wieder heil, die .glm ausserhalb geaendert (ein Byte), oeffnen", [glm, begleit] {
        std::string fehler;
        (void)sicherSchreiben(pfadIm("ent3a_b12.glm.stack"), *begleit, &fehler);
        std::string anders = *glm;
        anders[anders.size() - 1] = static_cast<char>(anders[anders.size() - 1] ^ 0x01);
        (void)sicherSchreiben(pfadIm("ent3a_b12_anders.glm"), anders, &fehler);
        (void)sicherSchreiben(pfadIm("ent3a_b12_anders.glm.stack"), *begleit, &fehler);
        modellVonPlatteOeffnen(pfadIm("ent3a_b12_anders.glm"));
    }));
    add(pause(1.2));
    add(pruefe("ENT3A B12: .glm ausserhalb geaendert - das Modell laedt ohne Stapel, die Meldung nennt den Grund", [] {
        melde("B12: geaendert - Teile " + std::to_string(mtab().dok.inhalt().teile.size()) + ", Meldung '" + mtab().meldung + "'");
        return !mtab().dok.inhalt().teile.empty() && modell::stapelLeer(mtab().dok.inhalt()) &&
               mtab().meldung == fmt(Str::EaBegleitFehler, "ent3a_b12_anders.glm.stack", tr(Str::EaBegleitGrundAlt));
    }));
    add(tu("ENT3A B12: die heile .glm oeffnen, alle Eintraege entfernen, speichern", [] {
        modellVonPlatteOeffnen(pfadIm("ent3a_b12.glm"));
        mtab().dok.aendern("ENT3A leeren", [](modell::Inhalt& x) {
            for (modell::Teil& t : x.teile) { t.stapel.clear(); }
        });
        e3().drehModus.clear();   // E3: ohne Drehmodus und ohne Stapel braucht es keine Begleitdatei
        mtab().speicherPfad = pfadIm("ent3a_b12.glm");
        modellSpeichern(false);
    }));
    add(pause(0.6));
    add(pruefe("ENT3A B12: leerer Stapel - Speichern entfernt die alte Begleitdatei", [] {
        std::error_code ec;
        return !std::filesystem::exists(pfadIm("ent3a_b12.glm.stack"), ec) && e3().begleitEntfernt > 0;
    }));
    // --- E3: die Begleitdatei auch fuer ein .md3-Requisit ---
    auto md3 = std::make_shared<std::string>();
    auto stapelMd3 = std::make_shared<std::string>();
    for (Schritt& x : mb1::laden("models/map_objects/imperial/crate_banded.md3")) { add(std::move(x)); }
    add(tu("ENT3A E3: .md3, wie 3ds Max, Modellieren, Flaeche 0: TurboSmooth in die Datei", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Modellieren;
        mtab().auswahl = {0};
        ap13s::stapelLeeren();
        (void)modellStapelHinzufuegen(ModArt::Glaetten);
        (void)modellStapelSchalten(ap13s::kennung(0, 0), true);
        std::error_code ec;
        std::filesystem::remove(pfadIm("ent3a_kiste.md3.stack"), ec);
    }));
    add(pause(0.6));
    add(pruefe("ENT3A E3: .md3 speichern - die .md3 Byte fuer Byte wie ohne Begleitdatei (md3Schreiben des Gezeigten), daneben ent3a_kiste.md3.stack mit Basis", [md3, stapelMd3] {
        std::string soll;
        const bool ok = modell::md3Schreiben(modellStapelDateiInhalt(), soll, modell::grenzen(static_cast<modell::Zielspiel>(mtab().zielspiel)).eckenJeFlaeche);
        *stapelMd3 = ap13s::stapelText(0);
        std::error_code ec;
        std::filesystem::remove(pfadIm("ent3a_kiste.md3"), ec);
        mtab().speicherPfad = pfadIm("ent3a_kiste.md3");
        modellSpeichern(false);
        *md3 = slurp(pfadIm("ent3a_kiste.md3"));
        const std::string begleit = slurp(pfadIm("ent3a_kiste.md3.stack"));
        melde("E3 md3: " + std::to_string(md3->size()) + " Bytes (soll " + std::to_string(soll.size()) + "), Begleitdatei " + std::to_string(begleit.size()) + " Bytes, Stapel " +
              *stapelMd3 + ", Meldung '" + mtab().meldung + "'");
        return ok && !md3->empty() && *md3 == soll && begleit.rfind("JKSTUDIO-STACK ", 0) == 0 && begleit.find("\nbasis ") != std::string::npos;
    }));
    add(tu("ENT3A E3: die .md3 von der Platte oeffnen", [] { modellVonPlatteOeffnen(pfadIm("ent3a_kiste.md3")); }));
    add(pause(1.0));
    add(pruefe("ENT3A E3: nach dem Oeffnen der .md3 ist der Stapel wieder da, die Meldung nennt es; Edit Poly hat sein Auge (Flaeche wie geladen)", [stapelMd3] {
        const std::string jetzt = mtab().dok.inhalt().herkunft == modell::Herkunft::Md3 && !mtab().dok.inhalt().teile.empty() ? ap13s::stapelText(0) : std::string("-");
        melde("E3 md3: geoeffnet " + g_app->modelPath + ", Stapel " + jetzt + " (vorher " + *stapelMd3 + "), Meldung '" + mtab().meldung + "'");
        return jetzt == *stapelMd3 && mtab().meldung == fmt(Str::EaBegleitGelesen, "ent3a_kiste.md3.stack", 1) && modellEnt3aGeladen(0) != nullptr;
    }));
    add(pruefe("ENT3A E3: erneutes Speichern schreibt dieselbe .md3 Byte fuer Byte", [md3] {
        mtab().speicherPfad = pfadIm("ent3a_kiste.md3");
        modellSpeichern(false);
        return slurp(pfadIm("ent3a_kiste.md3")) == *md3;
    }));
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
}

// --- C4: Esc schliesst das Rechtsklick-Menue der Ansicht --------------------------------------------------------------------------

void c4Schritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto vorher = std::make_shared<std::string>();
    for (int modus = 0; modus < 2; ++modus) {
        const std::string wie = modus == 0 ? " wie 3ds Max: " : " wie Blender: ";
        add(tu("ENT3A C4:" + wie + "Ansehen, torso gewaehlt, Rechtsklick-Menue der Ansicht", [=] {
            modellWaehlen(modus, ModellBereich::Ansehen);
            mtab().fokus = true;
            mtab().kontextOeffnen = true;
            setzeMaus(kv5::fokusPunkt());
        }));
        add(pause(0.6));
        add(pruefe("ENT3A C4" + wie + "das Menue ist offen", [vorher] {
            *vorher = std::to_string(mtab().auswahl.size()) + "/" + std::to_string(mtab().navModus) + "/" + std::to_string(static_cast<int>(mtab().bereich));
            return mtab().kontextSichtbar;
        }));
        add(taste(ImGuiKey_Escape, "Esc"));
        add(pause(0.5));
        add(pruefe("ENT3A C4" + wie + "Esc schliesst das Menue (wie 3ds Max und Blender), sonst aendert sich nichts", [vorher] {
            const std::string jetzt = std::to_string(mtab().auswahl.size()) + "/" + std::to_string(mtab().navModus) + "/" + std::to_string(static_cast<int>(mtab().bereich));
            melde("C4: offen " + std::to_string(mtab().kontextSichtbar ? 1 : 0) + ", vorher " + *vorher + ", nachher " + jetzt);
            return !mtab().kontextSichtbar && jetzt == *vorher && e3().kontextEscZahl > 0;
        }));
        add(mbefoto::popupsZu());
    }
}

// --- Fotos und Handbuch -----------------------------------------------------------------------------------------------------------

void fotoSchritte(std::vector<Schritt>& s, bool handbuch) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string p = handbuch ? "h_ent3a_" : "ent3a_";
    // B7: wie 3ds Max, eingefrorene Zeile gewaehlt, Rechtsklick mit "Auswahl auftauen"
    add(tu("ENT3A Foto: Explorer, torso eingefroren und gewaehlt", [] {
        modellWaehlen(0, ModellBereich::Ansehen);
        modellAllesAuftauen();
        modellTeilEinfrieren(torso(), true, false);
        mtab().auswahl = {torso()};
        modellExplorerZeigen(ExplorerArt::Flaeche, torso());
    }));
    add(pause(0.8));
    add(klickAn("ENT3A Foto: Rechtsklick auf torso", ap2::ort("torso", 0, static_cast<int>(ExplorerArt::Flaeche)), 1));
    add(pause(0.6));
    add(foto(p + "explorer"));
    add(mbefoto::popupsZu());
    add(tu("ENT3A Foto: auftauen", [] { modellAllesAuftauen(); }));
    // B8: wie Blender, breite Leiste mit Namen
    add(tu("ENT3A Foto: wie Blender, Werkzeugleiste 160 Punkte breit", [] {
        modellWaehlen(1, ModellBereich::Modellieren);
        modellBl().werkzeugleiste = true;
        g_app->settings.modellBlWerkzeugnamen = false;
        g_app->settings.modellBlLeisteBreite = 160.0F;
        setzeMaus(e3().leisteKante.GetCenter());
    }));
    add(pause(1.0));
    add(foto(p + "leiste"));
    add(tu("ENT3A Foto: Leiste schmal", [] { g_app->settings.modellBlLeisteBreite = 0.0F; }));
    // B9: wie Blender, Seitenleiste mit dem Becken
    add(tu("ENT3A Foto: wie Blender, Animation, Griff Becken, Seitenleiste", [] {
        modellWaehlen(1, ModellBereich::Animation);
        mtab().auswahl.clear();
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
        if (!modellRigDa()) { (void)modellAnimRigErstellen(false); }
        modellRigGriffWaehlen(ap4::griffNach(modell::GriffArt::Becken));
        modellBl().seitenleiste = true;
        modellBl().seitenReiter = 0;
        setzeMaus(kv5::fokusPunkt());
    }));
    add(pause(1.0));
    add(foto(p + "seite"));
    if (!handbuch) {
        add(klickAn("ENT3A Foto: Drehmodus aufklappen", ap4::feldOrt("drehmodus")));
        add(pause(0.6));
        add(foto(p + "seite_modi"));
        add(mbefoto::popupsZu());
    }
    add(tu("ENT3A Foto: Seitenleiste aus", [] {
        modellRigGriffWaehlen(-1);
        modellBl().seitenleiste = false;
    }));
    // B11/B13: wie 3ds Max, Stapel mit Augen und Unwrap UVW
    add(tu("ENT3A Foto: wie 3ds Max, Modellieren, torso, TurboSmooth, Skin aus", [] {
        ap13s::stapelLeeren();
        modellWaehlen(0, ModellBereich::Modellieren);
        md1d::waehle(modell::wof::PAuswahl{}, AuswahlArt::Dreieck);
        (void)modellStapelHinzufuegen(ModArt::Glaetten);
        animWaehlen(mb1::animNach("BOTH_WALK1"), false);
        g_app->animFrame += 10;
        g_app->animPlaying = false;
        if (!modellEnt3aSkinAus()) { modellEnt3aSkinSchalten(); }
        setzeMaus(ap13s::imRoll(tr(Str::EaSkinAn)) != nullptr ? ap13s::imRoll(tr(Str::EaSkinAn))->rect.GetCenter() : kv5::fokusPunkt());
    }));
    add(pause(1.4));
    add(foto(p + "stapel"));
    add(tu("ENT3A Foto: Skin an, Stapel leer", [] {
        if (modellEnt3aSkinAus()) { modellEnt3aSkinSchalten(); }
        ap13s::stapelLeeren();
    }));
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "ent3a" && n != "ent3afoto" && n != "ent3ahb") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    const char* fenster = std::getenv("BHED_ENT3A_FENSTER");
    if (fenster != nullptr) { (void)std::sscanf(fenster, "%dx%d@%f", &b, &h, &z); }
    const bool fensterSetzen = n != "ent3ahb" || fenster != nullptr;
    const std::string modell = "models/players/kyle/model.glm";
    auto skala = std::make_shared<float>(1.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("ENT3A: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        if (fensterSetzen) {
            (void)platform::hauptfensterGroesse(b, h);
            g_app->uiScale = z;
        }
        if (const char* t = std::getenv("BHED_ENT3A_THEMA")) {
            const bool hell = std::string(t) == "hell";
            for (const theme::Theme& th : theme::themes()) {
                if ((theme::relativeLuminance(th.window) > 0.5F) == hell) {
                    g_app->activeTheme = &th;
                    g_app->themePending = true;
                    break;
                }
            }
        }
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ent3a.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    add(tu("ENT3A: Aufteilung wie neue Einstellungen, Sequenz, wie 3ds Max", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Modellieren;
        g_app->mtab.vierAnsichten = false;
        animWaehlen(mbefoto::sequenz(), false);
        char text[300];
        std::snprintf(text, sizeof text, "%s, %zu Teile, Anzeige %.0f x %.0f, Oberflaeche %.2f", g_app->modelPath.c_str(), g_app->mtab.dok.inhalt().teile.size(),
                      static_cast<double>(ImGui::GetIO().DisplaySize.x), static_cast<double>(ImGui::GetIO().DisplaySize.y), static_cast<double>(g_app->uiScale));
        melde(text);
    }));
    add(pause(1.0));
    add(klickAn("ENT3A: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (n == "ent3a") {
        b7Schritte(s);
        b8Schritte(s);
        b9Schritte(s);
        b10Schritte(s);
        c4Schritte(s);
        b11Schritte(s, modell);
        b12Schritte(s, modell);
    } else {
        fotoSchritte(s, n == "ent3ahb");
    }
    add(tu("ENT3A: Modell-Tab zurueck", [=] {
        if (md1d::mz().kaestchenOffen) { modellMeshAbbrechen(); }
        modellAllesAuftauen();
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

}  // namespace ent3a
