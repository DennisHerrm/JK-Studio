// selbsttest_ap5.inc - AP5 (MBENTWURF Teil B, 12.11.): der Bereich Ansehen/Pruefen des Model-Tabs vollstaendig nach dem Entwurf
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap5 | ap5foto; Handbuch BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap5)
//
//   ap5       je Zeile der Abgleichtabelle (plaene/modellblock_stufe0/abgleich_programm_entwurf.md: P1-P11, X18, X19, R7, R10, Z20)
//             eine Pruefung "AP5 <Zeile>" in beiden Bedienmodi: Ort (Gruppe im rechten Feld, Menue der Kopfzeile, Ansicht), Element
//             (Knoepfe, Zeilen, Werte gegen eine eigene Rechnung) und Bedienung (Klick, Taste, Suche, Rechtsklick) mit ihrer Wirkung.
//   ap5foto   nur Fotos (BHED_FOTOS=<Ordner>): je Bedienmodus Ansehen/Pruefen ganz und das rechte Feld in Seiten, die Pruefanzeigen,
//             vier Ansichten wie 3ds Max, die Schattierungs-Auswahl wie Blender.
//   BHED_AP5_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe wie der Foto-Selbsttest mbefoto)
//   BHED_AP5_THEMA=hell|dunkel       das erste helle bzw. dunkle Farbschema
// Eingebunden in selbsttest.cpp (nach selbsttest_ap4.inc).
namespace ap5 {

using ap3::feldGruppe;
using ap3::feldGruppen;
using ap3::imRoll;
using ap3::insFeld;
using mbefoto::inFenster;
using mbefoto::kindFenster;

float em() { return ImGui::GetFontSize(); }

void melde(const std::string& z) { diag::detail("Knopftest: AP5 " + z); }

std::string zeile(const char* nr, int modus, const std::string& was) { return std::string("AP5 ") + nr + (modus == 0 ? " wie 3ds Max: " : " wie Blender: ") + was; }

ModellTab& mtab() { return g_app->mtab; }

const ModellAnsicht& aktiv() { return mtab().ansichten[static_cast<std::size_t>(std::clamp(mtab().aktiv, 0, 3))]; }

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

const Element* imPopup(const std::string& label) { return ap1::imPopup(label); }

Schritt klickRoll(const std::string& label) {
    return klickAn("AP5: \"" + label + "\" im rechten Feld", [label] {
        const Element* e = imRoll(label);
        if (e == nullptr) { meldeFehler("AP5: \"" + label + "\" steht nicht im rechten Feld"); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    });
}

// Eine Beschriftung des rechten Felds erst ganz ins Bild rollen (wie ein Benutzer), dann anklicken
void klickFeld(std::vector<Schritt>& s, const std::string& label) {
    s.push_back(insFeld(label));
    s.push_back(pause(0.2));
    s.push_back(klickRoll(label));
}

Schritt klickPopup(const std::string& label) {
    return klickAn("AP5: \"" + label + "\" im Menue", [label] {
        const Element* e = imPopup(label);
        if (e == nullptr) { meldeFehler("AP5: \"" + label + "\" steht nicht im Menue"); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    });
}

std::function<ImVec2()> kopfFeld(int feld) {
    return [feld] {
        ImVec2 p{-50.0F, -50.0F};
        if (!modellKopfFeldOrt(std::clamp(mtab().aktiv, 0, 3), feld, p)) { meldeFehler("AP5: Feld " + std::to_string(feld) + " der Kopfzeile nicht da"); }
        return p;
    };
}

int teil(const char* name) { return mb1::teil(name); }

int knochenNach(const char* name) {
    for (std::size_t i = 0; i < g_app->anim.bones.size(); ++i) {
        if (g_app->anim.bones[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

int sequenzNach(const char* name) {
    for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
        if (g_app->animList[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

// Die Netze der gezeichneten Stufe aus dem Dokument (eigene Rechnung gegen die Anzeige des Programms)
const modell::Netz* netz(int i) {
    const modell::Inhalt& in = mtab().dok.inhalt();
    const std::size_t s = static_cast<std::size_t>(std::max(mtab().lodJetzt, 0));
    if (i < 0 || static_cast<std::size_t>(i) >= in.teile.size() || s >= in.teile[static_cast<std::size_t>(i)].stufen.size()) { return nullptr; }
    return &in.teile[static_cast<std::size_t>(i)].stufen[s];
}

// Ein Wert aus der Probe der Pruefung ("Name=Wert|...")
// Ein Element mit dieser Beschriftung innerhalb eines Rechtecks (gleich lautende Zeilen anderer Gruppen ausgenommen)
const Element* imKasten(const std::string& label, const ImRect& kasten) {
    for (const Element& e : g_elemente) {
        if (e.label == label && e.innen.find("mtrollouts") != std::string::npos && kasten.Contains(e.rect)) { return &e; }
    }
    return nullptr;
}

std::string pruefWert(Str name) {
    const std::string& p = modellPr().pruefWerte;
    const std::string k = std::string(tr(name)) + "=";
    const std::size_t a = p.find(k);
    if (a == std::string::npos) { return "?"; }
    const std::size_t e = p.find('|', a);
    return p.substr(a + k.size(), e == std::string::npos ? std::string::npos : e - a - k.size());
}

// Wie weit sich zwei Bilder der aktiven Ansicht unterscheiden (Zahl der Bildpunkte mit einer Abweichung ueber 8 in einem Kanal)
int bildUnterschied(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
    if (a.size() != b.size() || a.empty()) { return -1; }
    int n = 0;
    for (std::size_t i = 0; i + 3 < a.size(); i += 4) {
        if (std::abs(a[i] - b[i]) > 8 || std::abs(a[i + 1] - b[i + 1]) > 8 || std::abs(a[i + 2] - b[i + 2]) > 8) { ++n; }
    }
    return n;
}

Schritt bildMerken(std::shared_ptr<std::vector<unsigned char>> ziel, std::shared_ptr<int> neu) {
    return tu("AP5: Foto der aktiven Ansicht merken", [ziel, neu] {
        int w = 0;
        int h = 0;
        ziel->clear();
        const bool ok = modellFotoMitUeberlagerung(0, *ziel, w, h);
        *neu = g_app->modellNeuGezeichnet;
        melde("Foto der Ansicht " + std::to_string(mtab().aktiv) + ": " + std::to_string(ok) + " " + std::to_string(w) + "x" + std::to_string(h) + ", Stand " +
              modellPrStand(mtab().aktiv) + ", neu gezeichnet " + std::to_string(*neu));
    });
}

// --- Gruppen von Ansehen/Pruefen in beiden Bedienmodi --------------------------------------------------------------------------

void gruppenSchritte(std::vector<Schritt>& s, int modus) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu(std::string("AP5: ") + (modus == 0 ? "wie 3ds Max" : "wie Blender") + ", Ansehen/Pruefen, torso gewaehlt, saber_1 an *r_hand", [modus] {
        modellBedienungSetzen(modus);
        mtab().bereich = ModellBereich::Ansehen;
        mtab().lodWahl = 0;
        mtab().zielspiel = static_cast<int>(modell::Zielspiel::JkaSp);
        mtab().auswahl = {teil("torso")};
        mtab().knochenWahl = -1;
        mtab().tagWahl = -1;
        if (mtab().anbauten.empty()) { (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true); }
        if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
    }));
    add(pause(0.8));
    // Reihenfolge des Entwurfs
    add(pruefe(zeile("P0", modus, "im Reiter des Bereichs die Gruppen des Entwurfs: Modell, Auswahl, Detailstufe und Skin, Boden und Kamera, Anbauten, Pruefung fuer das Spiel, Szene"), [] {
        const std::string soll = std::string(tr(Str::MbRModell)) + "|" + tr(Str::PqRAuswahl) + "|" + tr(Str::MbRLod) + "|" + tr(Str::PqRBoden) + "|" + tr(Str::MbRAnbauten) + "|" +
                                 tr(Str::MbRPruefen) + "|" + tr(Str::PqRSzene);
        melde("Gruppen: " + feldGruppen() + " - soll " + soll);
        return feldGruppen() == soll;
    }));
    // --- P1 Modell ---
    add(pruefe(zeile("P1", modus, "Modell: Raster mit Andere GLA, Texturen neu laden (Alt+R auch wie Blender), Importieren - zwei Spalten, jeder Knopf mit Symbol"), [] {
        const ui::RolloutProbe* m = feldGruppe(Str::MbRModell);
        const Element* a = imRoll(tr(Str::MbAndereGla));
        const Element* b = imRoll(tr(Str::MbTexNeu));
        const Element* c = imRoll(tr(Str::PqImport));
        if (m == nullptr || a == nullptr || b == nullptr || c == nullptr) { return false; }
        melde("P1: Knoepfe " + std::to_string(m->knoepfe) + ", Symbole " + std::to_string(m->symbole) + ", Tasten " + std::to_string(m->tasten) + ", Spalten " +
              std::to_string(m->rasterSpalten));
        return m->knoepfe == 3 && m->symbole == 3 && m->tasten == 1 && m->rasterSpalten == 2 && std::fabs(a->rect.Min.y - b->rect.Min.y) < 0.5F &&
               c->rect.Min.y > a->rect.Max.y - 0.5F && m->kasten.Contains(c->rect);
    }));
    add(pruefe(zeile("P1", modus, "Alt+R laedt die Texturen neu - in der Tastenliste des Bedienmodus"), [modus] {
        bool da = false;
        if (modus == 0) {
            for (int i = 0; i < modellTasteZahl(); ++i) { da = da || modellTasteChord(i) == (ImGuiMod_Alt | ImGuiKey_R); }
        } else {
            for (int i = 0; i < modellBlenderTasteZahl(); ++i) { da = da || modellBlenderTasteChord(i) == (ImGuiMod_Alt | ImGuiKey_R); }
        }
        return da;
    }));
    add(insFeld(tr(Str::PqImport)));
    add(pause(0.2));
    klickFeld(s, tr(Str::PqImport));
    add(pause(0.3));
    add(pruefe(zeile("P1", modus, "Importieren oeffnet die Wahl: von der Platte (.glm, .md3) oder aus dem Spiel (.pk3)"), [] {
        return imPopup(tr(Str::PqImportDatei)) != nullptr && imPopup(tr(Str::PqImportPk3)) != nullptr;
    }));
    add(klickPopup(tr(Str::PqImportPk3)));
    add(pause(0.3));
    add(pruefe(zeile("P1", modus, "\"Aus dem Spiel (.pk3)\" oeffnet den Archiv-Browser fuer Modelle"), [] {
        const bool offen = g_app->pk3Open && g_app->pk3Kind == 2;
        g_app->pk3Open = false;
        return offen;
    }));
    add(pause(0.3));
    // --- X18 Auswahl ---
    add(pruefe(zeile("X18", modus, "Gruppe Auswahl: Kurzinfo torso, Ecken, Dreiecke, Knochenbezuege, Shader, Eltern, Status als Tabelle - Werte wie im Dokument"), [] {
        const ui::RolloutProbe* g = feldGruppe(Str::PqRAuswahl);
        if (g == nullptr || g->kurzinfo != "torso") { return false; }
        bool ok = true;
        std::string fehlt;
        for (const Str z : {Str::MbXInfoEcken, Str::MbXInfoDreiecke, Str::MbXInfoKnochenbez, Str::MbXInfoShader, Str::MbXInfoEltern, Str::MbXInfoStatus}) {
            const Element* e = imKasten(tr(z), g->kasten);
            if (e == nullptr) { fehlt += std::string(tr(z)) + "; "; }
            ok = ok && e != nullptr;
        }
        melde("X18 Zeilen: " + (fehlt.empty() ? std::string("alle da") : fehlt) + " " + mbefoto::rechteck("kasten", g->kasten));
        const std::string info = modellExplorerInfo(ExplorerArt::Flaeche, teil("torso"));
        const modell::Netz* n = netz(teil("torso"));
        const bool zahl = n != nullptr && info.find(std::string(tr(Str::MbXInfoEcken)) + ": " + std::to_string(n->ecken.size()) + "\n") != std::string::npos &&
                          info.find(std::string(tr(Str::MbXInfoDreiecke)) + ": " + std::to_string(n->dreiecke()) + "\n") != std::string::npos;
        melde("X18 Info torso: " + info);
        return ok && zahl;
    }));
    add(tu("AP5: Knochen lower_lumbar gewaehlt, keine Flaeche", [] {
        mtab().auswahl.clear();
        mtab().knochenWahl = knochenNach("lower_lumbar");
    }));
    add(pause(0.4));
    add(pruefe(zeile("X18", modus, "Gruppe Auswahl zeigt den gewaehlten Knochen (Nummer, Eltern)"), [] {
        const ui::RolloutProbe* g = feldGruppe(Str::PqRAuswahl);
        return g != nullptr && g->kurzinfo == "lower_lumbar" && imKasten(tr(Str::MbXInfoNummer), g->kasten) != nullptr && imKasten(tr(Str::MbXInfoEltern), g->kasten) != nullptr;
    }));
    add(tu("AP5: wieder torso", [] {
        mtab().knochenWahl = -1;
        mtab().auswahl = {teil("torso")};
    }));
    add(pause(0.3));
    // --- P2 Detailstufe und Skin ---
    add(insFeld("10"));
    add(pause(0.2));
    klickFeld(s, "10");
    add(pause(0.4));
    add(pruefe(zeile("P2", modus, "LOD 10 ist waehlbar (ueber der Zahl der Stufen) und zeigt wie im Spiel die letzte Stufe; der Satz nennt es"), [] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        char soll[200];
        std::snprintf(soll, sizeof soll, tr(Str::PqLodWieSpiel), 10, in.stufenZahl);
        melde("P2: Wahl " + std::to_string(mtab().lodWahl) + ", gezeichnet " + std::to_string(mtab().lodJetzt + 1) + " von " + std::to_string(in.stufenZahl) + ", Satz \"" +
              modellPr().lodText + "\"");
        return mtab().lodWahl == 10 && mtab().lodJetzt == in.stufenZahl - 1 && modellPr().lodText.rfind(soll, 0) == 0;
    }));
    add(pruefe(zeile("P2", modus, "der Satz \"Das Modell hat n LODs. n Dreiecke.\" mit den Zahlen des Dokuments"), [] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        int d = 0;
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (const modell::Netz* n = in.teile[i].istTag() ? nullptr : netz(static_cast<int>(i))) { d += n->dreiecke(); }
        }
        char soll[200];
        std::snprintf(soll, sizeof soll, trN(Str::PqLodZahl, in.stufenZahl, d), in.stufenZahl, d);
        return modellPr().lodText.find(soll) != std::string::npos;
    }));
    add(tu("AP5: LOD wieder automatisch", [] { mtab().lodWahl = 0; }));
    add(pause(0.3));
    add(pruefe(zeile("Z20", modus, "Skins als zweispaltige Auswahl in \"Detailstufe und Skin\" (nicht in der Zeitleiste), mit dem Flaechenzaehler"), [] {
        const auto& h = g_app->modelSkins;
        const ui::RolloutProbe* l = feldGruppe(Str::MbRLod);
        if (h.size() < 2 || l == nullptr) { return false; }
        const Element* a = imRoll(h[0]);
        const Element* b = imRoll(h[1]);
        bool ok = a != nullptr && b != nullptr && std::fabs(a->rect.Min.y - b->rect.Min.y) < 0.5F && b->rect.Min.x > a->rect.Max.x - 0.5F &&
                  std::fabs(a->rect.GetWidth() - b->rect.GetWidth()) < 1.5F && l->kasten.Contains(a->rect) && l->kasten.Contains(b->rect);
        if (h.size() >= 3) {
            const Element* c = imRoll(h[2]);
            ok = ok && c != nullptr && c->rect.Min.y >= a->rect.Max.y - 0.5F && std::fabs(c->rect.Min.x - a->rect.Min.x) < 0.5F;
        }
        melde("Z20: " + std::to_string(h.size()) + " Skins" + (a != nullptr ? " " + mbefoto::rechteck("a", a->rect) : std::string()) +
              (b != nullptr ? " " + mbefoto::rechteck("b", b->rect) : std::string()));
        return ok && imRoll(tr(Str::ModelSkin)) != nullptr && ap1::imBand(tr(Str::ModelSkin)) == nullptr;
    }));
    auto haut = std::make_shared<int>(-1);
    add(tu("AP5: die Haut merken", [haut] { *haut = g_app->modelSkinIndex; }));
    add(tu("AP5: die Skins ins Bild rollen", [] {
        const Element* e = g_app->modelSkins.size() >= 2 ? imRoll(g_app->modelSkins[1]) : nullptr;
        ImGuiWindow* w = kindFenster("mtrollouts");
        if (e == nullptr || w == nullptr) { return; }
        if (e->rect.Max.y > w->InnerRect.Max.y || e->rect.Min.y < w->InnerRect.Min.y) {
            ImGui::SetScrollY(w, std::clamp(w->Scroll.y + e->rect.Min.y - w->InnerRect.GetCenter().y, 0.0F, w->ScrollMax.y));
        }
    }));
    add(pause(0.3));
    add(klickAn("AP5: eine andere Haut", [haut] {
        const auto& h = g_app->modelSkins;
        for (std::size_t i = 0; i < h.size(); ++i) {
            if (static_cast<int>(i) == *haut) { continue; }
            if (const Element* e = imRoll(h[i])) { return e->rect.GetCenter(); }
        }
        return ImVec2{-50.0F, -50.0F};
    }));
    add(pause(0.4));
    add(pruefe(zeile("Z20", modus, "ein Klick auf eine Haut legt sie an"), [haut] { return g_app->modelSkinIndex >= 0 && g_app->modelSkinIndex != *haut; }));
    add(tu("AP5: Haut zurueck", [haut] {
        int h = *haut;
        for (std::size_t i = 0; h < 0 && i < g_app->modelSkins.size(); ++i) {
            if (klein(g_app->modelSkins[i]) == "model_default.skin") { h = static_cast<int>(i); }   // ohne gewaehlte Haut: die des Ladens
        }
        if (h >= 0) { applySkinByIndex(h); }
    }));
    // --- P4 Boden und Kamera ---
    add(insFeld(tr(Str::PqRBoden)));
    add(tu("AP5: Boden aus", [] { mtab().an.boden = false; }));
    add(pause(0.3));
    add(insFeld("-24"));
    klickFeld(s, "-24");
    add(pause(0.3));
    add(pruefe(zeile("P4", modus, "Boden -24 schaltet den grauen Boden auf -24"), [] { return mtab().an.boden && mtab().an.bodenZ == -24.0F; }));
    klickFeld(s, tr(Str::MrTiefster));
    add(pause(0.3));
    add(pruefe(zeile("P4", modus, "Boden \"tiefster Punkt\" legt ihn unter den tiefsten Punkt der Figur in der Pose (wie [+] > Tiefster Punkt)"), [] {
        const float z = mtab().an.bodenZ;
        modellBodenAufTiefstenPunkt();
        melde("P4 Boden: Klick " + std::to_string(z) + ", Rechnung " + std::to_string(mtab().an.bodenZ));
        return mtab().an.boden && z != -24.0F && z == mtab().an.bodenZ;
    }));
    for (const float fov : {10.0F, 90.0F, 80.0F}) {
        char t[16];
        std::snprintf(t, sizeof t, "%.0f\xc2\xb0", static_cast<double>(fov));
        klickFeld(s, t);
        add(pause(0.2));
        add(pruefe(zeile("P4", modus, std::string("FOV ") + t + " setzt das Sichtfeld der Perspektive"), [fov] { return std::fabs(g_app->modelCam.fovY - fov) < 0.01F; }));
    }
    add(tu("AP5: Hintergrund Verlauf", [] { g_app->settings.modellHintergrund = -1; }));
    add(pause(0.2));
    klickFeld(s, "##pqhgan");
    add(pause(0.2));
    add(pruefe(zeile("P4", modus, "Hintergrund: der Haken schaltet die einfarbige Hintergrundfarbe ein (ModView Clear colour)"), [] { return g_app->settings.modellHintergrund >= 0; }));
    klickFeld(s, "##pqhgan");
    add(pause(0.2));
    add(pruefe(zeile("P4", modus, "ohne Haken wieder der Verlauf"), [] { return g_app->settings.modellHintergrund < 0; }));
    add(pruefe(zeile("P4", modus, "Gewicht-Schwelle als Regler, Ansicht zuruecksetzen und Foto (1920+) als Knoepfe der Gruppe"), [] {
        const ui::RolloutProbe* g = feldGruppe(Str::PqRBoden);
        return g != nullptr && imRoll(tr(Str::PqSchwelleWert)) != nullptr && imRoll(tr(Str::PqAnsichtZurueck)) != nullptr && imRoll(tr(Str::PqFoto)) != nullptr && g->knoepfe == 2 &&
               g->kasten.Contains(imRoll(tr(Str::PqFoto))->rect);
    }));
    auto kamera = std::make_shared<Camera>();
    add(tu("AP5: Kamera einpassen", [] { modellAnsichtEinpassen(); }));
    add(pause(0.4));
    add(tu("AP5: Kamera eingepasst merken, dann verdrehen", [kamera] {
        *kamera = g_app->modelCam;
        g_app->modelCam.angles[1] += 40.0F;
        g_app->modelCam.pos[2] += 30.0F;
        g_app->modelDirty = true;
    }));
    add(insFeld(tr(Str::PqAnsichtZurueck)));
    klickFeld(s, tr(Str::PqAnsichtZurueck));
    add(pause(0.3));
    add(pruefe(zeile("P4", modus, "Ansicht zuruecksetzen stellt die Kamera wie beim Laden (modellAnsichtEinpassen)"), [kamera] {
        const Camera c = g_app->modelCam;
        const Camera d = *kamera;
        char z[240];
        std::snprintf(z, sizeof z, "P4 Kamera %.2f %.2f %.2f / %.2f %.2f, soll %.2f %.2f %.2f / %.2f %.2f", static_cast<double>(c.pos[0]), static_cast<double>(c.pos[1]),
                      static_cast<double>(c.pos[2]), static_cast<double>(c.angles[0]), static_cast<double>(c.angles[1]), static_cast<double>(d.pos[0]),
                      static_cast<double>(d.pos[1]), static_cast<double>(d.pos[2]), static_cast<double>(d.angles[0]), static_cast<double>(d.angles[1]));
        melde(z);
        bool gleich = true;
        for (int k = 0; k < 3; ++k) { gleich = gleich && std::fabs(c.pos[k] - d.pos[k]) < 1e-3F && std::fabs(c.angles[k] - d.angles[k]) < 1e-3F; }
        return gleich;
    }));
    // --- P5 Anbauten ---
    add(insFeld(tr(Str::MfAnbauTipp)));
    add(pause(0.2));
    add(pruefe(zeile("P5", modus, "Anbauten: Kurzinfo 1, eine Zeile mit x am rechten Rand der Gruppe, darunter der Tipp als Kasten"), [] {
        const ui::RolloutProbe* g = feldGruppe(Str::MbRAnbauten);
        const Element* x = g != nullptr ? imKasten(tr(Str::MbLoesen), g->kasten) : nullptr;
        const Element* t = g != nullptr ? imKasten(tr(Str::MfAnbauTipp), g->kasten) : nullptr;
        melde("P5: Kurzinfo " + (g != nullptr ? g->kurzinfo : std::string("-")) + (x != nullptr ? " " + mbefoto::rechteck("x", x->rect) : std::string(" kein x")) +
              (t != nullptr ? " " + mbefoto::rechteck("tipp", t->rect) : std::string(" kein Tipp")) + (g != nullptr ? " " + mbefoto::rechteck("gruppe", g->kasten) : std::string()));
        return g != nullptr && g->kurzinfo == "1" && x != nullptr && t != nullptr && x->rect.Max.x >= g->kasten.Max.x - em() * 1.5F && t->rect.Min.y > x->rect.Max.y - 0.5F &&
               mtab().anbauten.size() == 1 && mtab().anbauten[0].ziel == "*r_hand";
    }));
    klickFeld(s, tr(Str::MbLoesen));
    add(pause(0.3));
    add(pruefe(zeile("P5", modus, "das x loest den Anbau"), [] { return mtab().anbauten.empty(); }));
    add(tu("AP5: saber_1 wieder an *r_hand", [] { (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true); }));
    // --- P6 Pruefung fuer das Spiel ---
    add(insFeld(tr(Str::PqPrSchatten)));
    add(pause(0.3));
    add(pruefe(zeile("P6", modus, "Pruefung: Balken Ecken, Knochen, Gewichte der gewaehlten Flaeche (torso) gegen die Grenzen des Zielspiels"), [] {
        const ui::RolloutProbe* g = feldGruppe(Str::MbRPruefen);
        bool ok = g != nullptr;
        for (const Str z : {Str::MfPrEcken, Str::MfPrKnochen, Str::MfPrGewichte}) {
            const Element* e = imRoll(std::string(tr(z)) + " (torso)");
            ok = ok && e != nullptr && g->kasten.Contains(e->rect);
        }
        return ok;
    }));
    add(pruefe(zeile("P6", modus, "Pruefung: Flaechen, Sequenzen innerhalb der GLA, gesuchte Tags, Shader ohne Bild, LODs, ohne eigenen Schatten - Werte gegen eigene Rechnung"), [] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        const auto spiel = static_cast<modell::Zielspiel>(mtab().zielspiel);
        int flaechen = 0;
        int schatten = 0;
        std::vector<std::string> tags;
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (in.teile[i].istTag()) {
                tags.push_back(klein(in.teile[i].name));
                continue;
            }
            ++flaechen;
            const modell::Netz* n = netz(static_cast<int>(i));
            if (n != nullptr && static_cast<int>(n->ecken.size()) >= modell::grenzen(spiel).eckenJeFlaeche / 2) { ++schatten; }
        }
        int innen = 0;
        for (const AnimEntry& e : g_app->animList) { innen += e.firstFrame + e.numFrames <= g_app->anim.bildZahl() ? 1 : 0; }
        // die Tags des Spielermodells (OpenJK g_client.cpp:1171-1192) - JKA SP, _humanoid
        static const char* const kSp[] = {"*head_eyes", "*l_arm_elbow", "*r_arm_elbow", "*l_hand", "*r_hand", "*hips_l_knee", "*hips_r_knee", "*l_leg_foot", "*r_leg_foot"};
        int da = 0;
        for (const char* k : kSp) { da += std::find(tags.begin(), tags.end(), k) != tags.end() ? 1 : 0; }
        char lods[80];
        std::snprintf(lods, sizeof lods, tr(Str::PqPrLodsWert), in.stufenZahl);
        melde("P6 Werte: " + modellPr().pruefWerte + " - Rechnung Flaechen " + std::to_string(flaechen) + ", Sequenzen " + std::to_string(innen) + "/" +
              std::to_string(g_app->animList.size()) + ", Tags " + std::to_string(da) + "/9, Schatten " + std::to_string(schatten));
        bool ok = true;
        for (const Str z : {Str::PqPrFlaechen, Str::PqPrSequenzen, Str::PqPrTags, Str::PqPrShader, Str::PqPrLods, Str::PqPrSchatten}) { ok = ok && imRoll(tr(z)) != nullptr; }
        return ok && spiel == modell::Zielspiel::JkaSp && pruefWert(Str::PqPrFlaechen) == std::to_string(flaechen) &&
               pruefWert(Str::PqPrSequenzen) == std::to_string(innen) + " / " + std::to_string(g_app->animList.size()) &&
               pruefWert(Str::PqPrTags) == std::to_string(da) + " / 9" && pruefWert(Str::PqPrLods) == lods && pruefWert(Str::PqPrSchatten) == std::to_string(schatten);
    }));
    // --- P7 Szene ---
    add(insFeld(tr(Str::PqNpc)));
    add(pause(0.2));
    add(pruefe(zeile("P7", modus, "Gruppe Szene: Szene speichern, ModView-Skript lesen, NPC-Browser (Raster, jeder mit Symbol)"), [] {
        const ui::RolloutProbe* g = feldGruppe(Str::PqRSzene);
        return g != nullptr && g->knoepfe == 3 && g->symbole == 3 && imRoll(tr(Str::PqSzeneSpeichern)) != nullptr && imRoll(tr(Str::PqMvsLesen)) != nullptr &&
               imRoll(tr(Str::PqNpc)) != nullptr;
    }));
    auto npc = std::make_shared<bool>(false);
    add(tu("AP5: NPC-Browser merken", [npc] { *npc = g_app->npcBrowserOffen; }));
    klickFeld(s, tr(Str::PqNpc));
    add(pause(0.3));
    add(pruefe(zeile("P7", modus, "NPC-Browser schaltet den Browser"), [npc] {
        const bool ok = g_app->npcBrowserOffen != *npc;
        g_app->npcBrowserOffen = *npc;
        return ok;
    }));
    add(pause(0.2));
}

// --- P7: Szene speichern und lesen ------------------------------------------------------------------------------------------

std::string ordner() {
    const char* f = std::getenv("BHED_FOTOS");
    return f != nullptr ? std::string(f) : std::string(".");
}

void szeneSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct Stand {
        int haut = -1;
        int primaer = -1;
        int sekundaer = -1;
        int lod = 0;
        float fov = 0.0F;
        float bodenZ = 0.0F;
        std::string anbauten;
        std::string flaechen;
    };
    auto vorher = std::make_shared<Stand>();
    const auto stand = [] {
        Stand x;
        x.haut = g_app->modelSkinIndex;
        x.primaer = g_app->animIndex;
        x.sekundaer = mtab().sekundaer;
        x.lod = mtab().lodWahl;
        x.fov = g_app->modelCam.fovY;
        x.bodenZ = mtab().an.boden ? mtab().an.bodenZ : -999.0F;
        for (const ModellAnbau& a : mtab().anbauten) { x.anbauten += a.pfad + ">" + a.ziel + (a.knochen ? "(k)" : "") + ";"; }
        for (const modell::FlaechenSchalter& f : mtab().g2.slist) { x.flaechen += std::to_string(f.flaeche) + ":" + std::to_string(f.offFlags) + ";"; }
        return x;
    };
    add(tu("AP5 P7: Szene einrichten - Haut, Primaer BOTH_STAND1, Sekundaer TORSO_WEAPONREADY3, head aus, saber_1 an *r_hand, LOD 2, FOV 10, Boden -24", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Ansehen;
        if (g_app->modelSkins.size() >= 2) { applySkinByIndex(1); }
        animWaehlen(sequenzNach("BOTH_STAND1"), false);
        mtab().sekundaer = sequenzNach("TORSO_WEAPONREADY3");
        modellAnbautenLoesen();
        (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true);
        mtab().lodWahl = 2;
        g_app->modelCam.fovY = 10.0F;
        mtab().an.boden = true;
        mtab().an.bodenZ = -24.0F;
    }));
    add(pause(0.5));
    add(tu("AP5 P7: head aus (nach der Haut)", [] { modellFlaecheSchalten(teil("head"), modell::kG2Aus); }));
    add(pause(0.4));
    add(tu("AP5 P7: Stand merken, Szene speichern", [vorher, stand] {
        *vorher = stand();
        melde("P7 Stand vorher: Haut " + std::to_string(vorher->haut) + ", Primaer " + std::to_string(vorher->primaer) + ", Sekundaer " + std::to_string(vorher->sekundaer) +
              ", Anbauten " + vorher->anbauten + ", Flaechen " + vorher->flaechen);
        (void)modellSzeneSpeichern(ordner() + "\\ap5_szene.mvs");
    }));
    add(pause(0.3));
    add(pruefe("AP5 P7: Szene speichern schreibt ein ModView-Skript, das ModView lesen kann (loadmodel, modelfile, boltmodel, surfaces_off, locksequence)", [] {
        std::ifstream f(ordner() + "\\ap5_szene.mvs", std::ios::binary);
        std::stringstream ss;
        ss << f.rdbuf();
        modell::Szene sz;
        std::string fehler;
        const bool ok = modell::szeneLesen(ss.str(), sz, &fehler);
        melde("P7 Skript:\n" + ss.str());
        return ok && sz.modell == "models/players/kyle/model.glm" && sz.primaer == "BOTH_STAND1" && sz.sekundaer == "TORSO_WEAPONREADY3" && sz.sekundaerStart == "lower_lumbar" &&
               sz.flaechenAus.size() == 1 && sz.flaechenAus[0] == "head" && sz.anbauten.size() == 1 && sz.anbauten[0].ziel == "*r_hand" && !sz.anbauten[0].knochen &&
               sz.lod == 2 && sz.sichtfeld == 10.0F && sz.boden == 1;
    }));
    add(tu("AP5 P7: alles anders - Kyle neu, ohne Anbau, LOD automatisch, FOV 80, Boden aus", [] {
        std::string fehler;
        (void)oeffneModellAusArchiv("models/players/kyle/model.glm", &fehler);
        modellAnbautenLoesen();
        mtab().lodWahl = 0;
        g_app->modelCam.fovY = 80.0F;
        mtab().an.boden = false;
    }));
    add(pause(1.0));
    add(tu("AP5 P7: Szene lesen", [] { (void)modellSzeneLesen(ordner() + "\\ap5_szene.mvs"); }));
    add(pause(1.0));
    add(pruefe("AP5 P7: Szene lesen stellt alles wieder her (Haut, Sequenzen, Flaechen, Anbau, LOD, FOV, Boden)", [vorher, stand] {
        const Stand n = stand();
        melde("P7 Stand nachher: Haut " + std::to_string(n.haut) + ", Primaer " + std::to_string(n.primaer) + ", Sekundaer " + std::to_string(n.sekundaer) + ", Anbauten " +
              n.anbauten + ", Flaechen " + n.flaechen + ", Meldung " + mtab().meldung);
        return n.haut == vorher->haut && n.primaer == vorher->primaer && n.sekundaer == vorher->sekundaer && n.lod == vorher->lod && n.fov == vorher->fov &&
               n.bodenZ == vorher->bodenZ && n.anbauten == vorher->anbauten && n.flaechen == vorher->flaechen;
    }));
    // ein ModView-Skript von Hand (wie ModView es schreibt): Anbau an einen Knochen, Wurzel, aus ohne Kinder, ein Schluessel, den JK Studio nicht abbildet
    add(tu("AP5 P7: ModView-Skript von Hand lesen", [] {
        std::ofstream f(ordner() + "\\ap5_modview.mvs", std::ios::binary);
        f << "loadmodel\n{\n\tname\t\t\"model\"\n\tmodelfile\t\"models/players/kyle/model.glm\"\n\tsurfacename_rootoverride\t\"hips\"\n"
             "\tmultiseq_primary_lock\t\"<any non-blank string>\"\n\tsurfaces_offnochildren\n\t{\n\t\tname0\t\"l_leg\"\n\t}\n\n"
             "\tboltmodel\n\t{\n\t\tname\t\"saber_1\"\n\t\tmodelfile\t\"models/weapons2/saber_1/saber_1.glm\"\n\t\tparent\t\"model\"\n\t\tbolttobone\t\"lhand\"\n\t}\n\n}\n";
        f.close();
        (void)modellSzeneLesen(ordner() + "\\ap5_modview.mvs");
    }));
    add(pause(1.0));
    add(pruefe("AP5 P7: das ModView-Skript - Wurzel hips, l_leg aus ohne Kinder, saber_1 am Knochen lhand; multiseq als nicht uebernommen gemeldet", [] {
        const ModellTab& t = mtab();
        bool kinder = false;
        for (const modell::FlaechenSchalter& f : t.g2.slist) { kinder = kinder || (f.flaeche == teil("l_leg") && (f.offFlags & modell::kG2OhneKinder) != 0U); }
        melde("P7 ModView: Wurzel " + std::to_string(t.g2.wurzel) + ", Anbauten " + std::to_string(t.anbauten.size()) + ", Meldung " + t.meldung);
        return t.g2.wurzel == teil("hips") && kinder && t.anbauten.size() == 1 && t.anbauten[0].knochen && t.anbauten[0].ziel == "lhand" &&
               t.meldung.find("multiseq_primary_lock") != std::string::npos;
    }));
    add(tu("AP5 P7: Kyle frisch", [] {
        std::string fehler;
        (void)oeffneModellAusArchiv("models/players/kyle/model.glm", &fehler);
        modellAnbautenLoesen();
        mtab().lodWahl = 0;
        g_app->modelCam.fovY = 80.0F;
        mtab().an.boden = false;
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
    }));
    add(pause(1.0));
}

// --- P3/P3a: die Anzeige-Schalter in den Menues der Kopfzeile, ihre Wirkung -----------------------------------------------------

void anzeigeSchritte(std::vector<Schritt>& s, int modus) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu(std::string("AP5 P3a: ") + (modus == 0 ? "wie 3ds Max" : "wie Blender") + ", torso, die fuenf Schalter auf Werk", [modus] {
        modellBedienungSetzen(modus);
        mtab().bereich = ModellBereich::Ansehen;
        mtab().auswahl = {teil("torso")};
        modellPr().alpha = true;
        modellPr().filter = true;
        modellPr().schwelleAn = false;
        modellPr().hervorheben = false;
        modellPr().schatten = false;
        ModellSchattierung& sch = mtab().ansichten[static_cast<std::size_t>(std::clamp(mtab().aktiv, 0, 3))].sch;
        melde("P3a vorher: Texturen " + std::to_string(g_app->showModelTextures) + ", gefunden " + std::to_string(g_app->modelTextures.found) + ", weiss " +
              std::to_string(sch.weiss) + ", Drahtgitter " + std::to_string(sch.drahtgitter));
        g_app->showModelTextures = true;   // Filter und Alpha wirken nur auf Bilder
        sch.weiss = false;
        sch.drahtgitter = false;
        setzeMaus(ImVec2{-50.0F, -50.0F});
    }));
    add(pause(0.6));
    // Alpha und Filter: Max im Menue [Standard] (Viewport Shading), Blender im Ausklapper Viewport Shading
    add(klickAn("AP5 P3a: Menue der Schattierung", kopfFeld(modus == 0 ? 2 : 4)));
    add(pause(0.3));
    add(pruefe(zeile("P3a", modus, "Alpha-Blending und Texturfilter im Menue der Schattierung"), [] { return imPopup(tr(Str::PqAlpha)) != nullptr && imPopup(tr(Str::PqFilter)) != nullptr; }));
    auto bildA = std::make_shared<std::vector<unsigned char>>();
    auto bildB = std::make_shared<std::vector<unsigned char>>();
    auto neuA = std::make_shared<int>(0);
    auto neuB = std::make_shared<int>(0);
    add(tu("AP5 P3a: Kamera nah an den Kopf (Texel gross)", [] {
        g_app->modelCam.fovY = 10.0F;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(bildMerken(bildA, neuA));
    add(klickPopup(tr(Str::PqFilter)));
    add(pause(0.6));
    add(bildMerken(bildB, neuB));
    add(foto(std::string("ap5_") + (modus == 0 ? "max" : "blender") + "_filteraus"));   // D3D12-2: punktFilter je Schnittstelle
    add(pruefe(zeile("P3a", modus, "Texturfilter aus: der Schalter ist aus, die Ansicht zeichnet neu, das Bild zeigt die Bildpunkte der Textur (anderes Bild)"),
               [bildA, bildB, neuA, neuB] {
        const int d = bildUnterschied(*bildA, *bildB);
        melde("P3a Filter: Unterschied " + std::to_string(d) + " Bildpunkte");
        return !modellPr().filter && d > 0 && *neuB > *neuA;
    }));
    add(tu("AP5 P3a: Filter wieder an, Alpha aus", [] {
        modellPr().filter = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(bildMerken(bildA, neuA));
    add(klickAn("AP5 P3a: Menue der Schattierung", kopfFeld(modus == 0 ? 2 : 4)));
    add(pause(0.3));
    add(klickPopup(tr(Str::PqAlpha)));
    add(pause(0.6));
    add(bildMerken(bildB, neuB));
    add(foto(std::string("ap5_") + (modus == 0 ? "max" : "blender") + "_alphaaus"));   // D3D12-2: opak je Schnittstelle
    add(pruefe(zeile("P3a", modus, "Alpha-Blending aus: der Schalter ist aus, die Ansicht zeichnet neu, alles deckend (anderes Bild)"), [bildA, bildB, neuA, neuB] {
        const int d = bildUnterschied(*bildA, *bildB);
        melde("P3a Alpha: Unterschied " + std::to_string(d) + " Bildpunkte");
        return !modellPr().alpha && d > 0 && *neuB > *neuA;
    }));
    add(tu("AP5 P3a: Alpha wieder an, FOV 80", [] {
        modellPr().alpha = true;
        g_app->modelCam.fovY = 80.0F;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    // Schwelle, Hervorheben, Schatten: Max im Menue [+] > Knochen und Pruefen, Blender im Ausklapper Overlays
    if (modus == 0) {
        add(klickAn("AP5 P3a: Menue [+]", kopfFeld(0)));
        add(pause(0.3));
        add(klickPopup(tr(Str::MkKnochenPruefen)));
    } else {
        add(klickAn("AP5 P3a: Overlays", kopfFeld(3)));
    }
    add(pause(0.4));
    add(pruefe(zeile("P3a", modus, "Knochengewicht-Schwelle (mit Zahl), Flaeche hervorheben, nicht schattenwerfende Flaechen im Menue"), [] {
        return imPopup(tr(Str::PqSchwelle)) != nullptr && imPopup(tr(Str::PqSchwelleWert)) != nullptr && imPopup(tr(Str::PqHervor)) != nullptr && imPopup(tr(Str::PqSchatten)) != nullptr;
    }));
    add(mbefoto::popupsZu());
    add(tu("AP5 P3a: tusken (eine Flaeche mit mehr als 500 Ecken), Zielspiel JKA SP", [] {
        std::string fehler;
        (void)oeffneModellAusArchiv("models/players/tusken/model.glm", &fehler);
        mtab().zielspiel = static_cast<int>(modell::Zielspiel::JkaSp);
    }));
    add(pause(1.2));
    if (modus == 0) {
        add(klickAn("AP5 P3a: Menue [+]", kopfFeld(0)));
        add(pause(0.3));
        add(klickPopup(tr(Str::MkKnochenPruefen)));
    } else {
        add(klickAn("AP5 P3a: Overlays", kopfFeld(3)));
    }
    add(pause(0.4));
    add(klickPopup(tr(Str::PqSchatten)));
    add(pause(0.5));
    add(pruefe(zeile("P3a", modus, "nicht schattenwerfende Flaechen: rote Striche auf jeder gezeichneten Flaeche mit mindestens SHADER_MAX_VERTEXES / 2 Ecken"), [] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        const int grenze = modell::grenzen(static_cast<modell::Zielspiel>(mtab().zielspiel)).eckenJeFlaeche / 2;
        int soll = 0;
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            const modell::Netz* n = in.teile[i].istTag() ? nullptr : netz(static_cast<int>(i));
            if (n != nullptr && static_cast<int>(n->ecken.size()) >= grenze && modellTeilGezeichnet(static_cast<int>(i))) { ++soll; }
        }
        melde("P3a Schatten: " + std::to_string(modellPr().schattenFlaechen) + " Flaechen gestrichelt, Rechnung " + std::to_string(soll) + " (Grenze " + std::to_string(grenze) + ")");
        return modellPr().schatten && soll > 0 && modellPr().schattenFlaechen == soll;
    }));
    add(tu("AP5 P3a: Zielspiel Movie Duels SP (SHADER_MAX_VERTEXES 4000)", [] { mtab().zielspiel = static_cast<int>(modell::Zielspiel::MdSp); }));
    add(pause(0.4));
    add(pruefe(zeile("P3a", modus, "dieselbe Figur gegen Movie Duels SP: die Grenze ist 2000 Ecken - keine Flaeche gestrichelt"), [] {
        melde("P3a Schatten MD: " + std::to_string(modellPr().schattenFlaechen));
        return modellPr().schattenFlaechen == 0;
    }));
    add(tu("AP5 P3a: Kyle wieder, Zielspiel JKA SP, torso", [] {
        modellPr().schatten = false;
        std::string fehler;
        (void)oeffneModellAusArchiv("models/players/kyle/model.glm", &fehler);
        mtab().zielspiel = static_cast<int>(modell::Zielspiel::JkaSp);
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
    }));
    add(pause(1.2));
    add(tu("AP5 P3a: torso", [] { mtab().auswahl = {teil("torso")}; }));
    add(pause(0.3));
    if (modus == 0) {
        add(klickAn("AP5 P3a: Menue [+]", kopfFeld(0)));
        add(pause(0.3));
        add(klickPopup(tr(Str::MkKnochenPruefen)));
    } else {
        add(klickAn("AP5 P3a: Overlays", kopfFeld(3)));
    }
    add(pause(0.4));
    add(klickPopup(tr(Str::PqHervor)));
    add(pause(0.5));
    add(pruefe(zeile("P3a", modus, "Flaeche hervorheben: gelb umrandet, jeder Knochen, auf den torso sich bezieht, mit Ursprung, Name und Nummer"), [] {
        std::vector<int> b;
        if (const modell::Netz* n = netz(teil("torso"))) {
            for (const modell::Ecke& e : n->ecken) {
                for (int k = 0; k < 4; ++k) {
                    if (e.gewicht[k] > 0.0F && std::find(b.begin(), b.end(), static_cast<int>(e.knochen[k])) == b.end()) { b.push_back(static_cast<int>(e.knochen[k])); }
                }
            }
        }
        melde("P3a Hervorheben: " + std::to_string(modellPr().bezugKnochen) + " Knochen beschriftet, Rechnung " + std::to_string(b.size()));
        return modellPr().hervorheben && !b.empty() && modellPr().bezugKnochen == static_cast<int>(b.size());
    }));
    if (modus == 0) {
        add(klickAn("AP5 P3a: Menue [+]", kopfFeld(0)));
        add(pause(0.3));
        add(klickPopup(tr(Str::MkKnochenPruefen)));
    } else {
        add(klickAn("AP5 P3a: Overlays", kopfFeld(3)));
    }
    add(pause(0.4));
    add(tu("AP5 P3a: Schwelle 0.25", [] { modellPr().schwelle = 0.25F; }));
    add(klickPopup(tr(Str::PqSchwelle)));
    add(pause(0.6));
    add(pruefe(zeile("P3a", modus, "Knochengewicht-Schwelle 0.25: jede Ecke mit einem Gewicht bis 0.25 verliert es in der Anzeige (ModView r_glm.cpp)"), [] {
        int soll = 0;
        const modell::Inhalt& in = mtab().dok.inhalt();
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            const modell::Netz* n = netz(static_cast<int>(i));
            if (n == nullptr) { continue; }
            for (const modell::Ecke& e : n->ecken) {
                bool weg = false;
                for (int k = 0; k < 4; ++k) { weg = weg || (e.gewicht[k] > 0.0F && !(e.gewicht[k] * 100.0F > 25.0F)); }
                soll += weg ? 1 : 0;
            }
        }
        melde("P3a Schwelle: " + std::to_string(modellPr().schwelleEcken) + " Ecken, Rechnung " + std::to_string(soll));
        return modellPr().schwelleAn && soll > 0 && modellPr().schwelleEcken == soll;
    }));
    add(tu("AP5 P3a: Schalter auf Werk", [] {
        modellPr().schwelleAn = false;
        modellPr().schwelle = 0.05F;
        modellPr().hervorheben = false;
        modellPr().schatten = false;
        g_app->modelDirty = true;
    }));
    add(mbefoto::popupsZu());
    add(pause(0.3));
}

// --- R10, P8-P11, X19, R7 ------------------------------------------------------------------------------------------------------

void restSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        add(tu(std::string("AP5 R10: ") + (modus == 0 ? "wie 3ds Max" : "wie Blender") + ", Ansehen/Pruefen", [modus] {
            modellBedienungSetzen(modus);
            mtab().bereich = ModellBereich::Ansehen;
            mtab().vierAnsichten = false;
            mtab().aktiv = 3;
        }));
        add(pause(0.6));
        add(pruefe(zeile("R10", modus, "unten rechts in der Ansicht \"LOD n - n Dreiecke - wie im Spiel gezeichnet (Ghoul2 aus dem Mission-Tab)\" mit der Zahl der gezeichneten Dreiecke"), [] {
            const modell::Inhalt& in = mtab().dok.inhalt();
            int d = 0;
            for (std::size_t i = 0; i < in.teile.size(); ++i) {
                const modell::Netz* n = in.teile[i].istTag() ? nullptr : netz(static_cast<int>(i));
                if (n != nullptr && modellTeilGezeichnet(static_cast<int>(i))) { d += n->dreiecke(); }
            }
            char soll[300];
            std::snprintf(soll, sizeof soll, trN(Str::PqLodHinweis, mtab().lodJetzt + 1, d), mtab().lodJetzt + 1, d);
            const ImRect& r = modellPr().lodHinweis;
            const ModellAnsicht& a = aktiv();
            melde("R10: \"" + modellPr().lodHinweisText + "\" " + mbefoto::rechteck("hinweis", r));
            return modellPr().lodHinweisText == soll && r.GetWidth() > 0.0F && r.Max.x <= a.lo[0] + a.groesse[0] && r.Max.x >= a.lo[0] + a.groesse[0] - em() &&
                   r.Max.y <= a.lo[1] + a.groesse[1] && r.Max.y >= a.lo[1] + a.groesse[1] - em();
        }));
    }
    add(tu("AP5 R10: Bereich Modellieren", [] { mtab().bereich = ModellBereich::Modellieren; }));
    add(pause(0.4));
    add(pruefe("AP5 R10: in den anderen Bereichen steht der Hinweis nicht", [] { return modellPr().lodHinweisText.empty(); }));
    // P8: die Werkzeugleiste wie Blender (AP4) hat Messen, Flaeche waehlen, Knochen waehlen, Foto
    add(tu("AP5 P8: wie Blender, Ansehen/Pruefen", [] {
        modellBedienungSetzen(1);
        mtab().bereich = ModellBereich::Ansehen;
    }));
    add(pause(0.5));
    add(pruefe("AP5 P8 wie Blender: Werkzeugleiste des Bereichs - Messen, Flaeche waehlen, Knochen waehlen, Foto", [] {
        std::string t;
        for (int i = 0; i < modellBlWerkzeugZahl(); ++i) { t += std::string(modellBlWerkzeugText(i)) + "|"; }
        return t.find(std::string(tr(Str::BzMessen)) + "|" + tr(Str::BzFlaecheWaehlen) + "|" + tr(Str::BzKnochenWaehlen) + "|" + tr(Str::MbFoto) + "|") != std::string::npos;
    }));
    // P10: Z und Umschalt+Alt+Z wie Blender
    add(klickAn("AP5 P10: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.3));
    add(tasteKombi(ImGuiKey_Z, "Z"));
    add(pause(0.4));
    add(pruefe("AP5 P10 wie Blender: Z oeffnet die Schattierungs-Auswahl (Drahtgitter, Solid, Material)", [] {
        return modellPr().schattierungSichtbar && imPopup(tr(Str::MkBlDraht)) != nullptr && imPopup(tr(Str::MkBlSolid)) != nullptr && imPopup(tr(Str::MkBlMaterial)) != nullptr;
    }));
    add(klickPopup(tr(Str::MkBlDraht)));
    add(pause(0.3));
    add(pruefe("AP5 P10 wie Blender: Drahtgitter in der Auswahl schaltet die aktive Ansicht auf Drahtgitter", [] {
        const bool d = aktiv().sch.drahtgitter;
        mtab().ansichten[static_cast<std::size_t>(std::clamp(mtab().aktiv, 0, 3))].sch.drahtgitter = false;
        return d;
    }));
    add(klickAn("AP5 P10: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.2));
    add(tasteKombi(ImGuiMod_Shift | ImGuiMod_Alt | ImGuiKey_Z, "Umschalt+Alt+Z"));
    add(pause(0.5));
    add(pruefe("AP5 P10 wie Blender: Umschalt+Alt+Z blendet die Overlays der Ansicht aus (Raster, Ueberlagerung, Hinweis)", [] {
        const int n = std::clamp(mtab().aktiv, 0, 3);
        melde("P10 Stand " + modellPrStand(n));
        return modellPr().overlaysAus[n] && modellPr().lodHinweis.GetWidth() == 0.0F && modellPrStand(n).back() == '1';
    }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiMod_Alt | ImGuiKey_Z, "Umschalt+Alt+Z"));
    add(pause(0.5));
    add(pruefe("AP5 P10 wie Blender: noch einmal - die Overlays sind wieder da", [] {
        return !modellPr().overlaysAus[std::clamp(mtab().aktiv, 0, 3)] && modellPr().lodHinweis.GetWidth() > 0.0F;
    }));
    // P9: die Tasten wie 3ds Max des Bereichs (Alt+X Durchsicht gehoert zu R14/AP13)
    add(pruefe("AP5 P9 wie 3ds Max: Q Auswaehlen, F3 Drahtgitter, Alt+W Ansicht, / Abspielen, , und . Bild", [] {
        int da = 0;
        for (const ImGuiKeyChord k : {static_cast<ImGuiKeyChord>(ImGuiKey_Q), static_cast<ImGuiKeyChord>(ImGuiKey_F3), static_cast<ImGuiKeyChord>(ImGuiMod_Alt | ImGuiKey_W),
                                      static_cast<ImGuiKeyChord>(ImGuiKey_Slash), static_cast<ImGuiKeyChord>(ImGuiKey_Comma), static_cast<ImGuiKeyChord>(ImGuiKey_Period)}) {
            for (int i = 0; i < modellTasteZahl(); ++i) {
                if (modellTasteChord(i) == k) {
                    ++da;
                    break;
                }
            }
        }
        return da == 6;
    }));
    // P11: Startbereich beim Oeffnen einer .glm
    add(tu("AP5 P11: wie 3ds Max, Bereich Weights, Kyle neu oeffnen", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Weights;
    }));
    add(pause(0.4));
    add(tu("AP5 P11: oeffnen", [] {
        std::string fehler;
        (void)oeffneModellAusArchiv("models/players/kyle/model.glm", &fehler);
    }));
    add(pause(1.0));
    add(pruefe("AP5 P11: beim Oeffnen einer .glm steht der Model-Tab in Ansehen/Pruefen", [] { return mtab().bereich == ModellBereich::Ansehen; }));
    add(tu("AP5: Sequenz wie der Entwurf", [] {
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
        mtab().auswahl = {teil("torso")};
    }));
    add(pause(0.6));
    // X19: Rechtsklick auf eine Flaeche im Szenen-Explorer
    for (int modus = 0; modus < 2; ++modus) {
        add(tu(std::string("AP5 X19: ") + (modus == 0 ? "wie 3ds Max" : "wie Blender"), [modus] { modellBedienungSetzen(modus); }));
        add(pause(0.6));
        add(mbefoto::baumRolleZu("torso", [] { return finde("torso", "mtbaum", 0, false); }, 2.0F));
        add(pause(0.3));
        add(klick("torso", "mtbaum", 0, false, "", 1));
        add(pause(0.4));
        add(pruefe(zeile("X19", modus, "Rechtsklick auf eine Flaeche: Info, Status an / aus / aus + ohne Kinder, Als Wurzel setzen, Wurzel loesen, Alle _off aus / an, Ursprungszustand"), [] {
            int da = 0;
            for (const Str z : {Str::MbXInfo, Str::MbFlAn, Str::MbFlAus, Str::MbFlAusOhneKinder, Str::MbFlWurzel, Str::MbXWurzelLoesen, Str::MbFlKappenAus, Str::MbFlKappenAn,
                                Str::MbFlUrsprung}) {
                da += imPopup(tr(z)) != nullptr ? 1 : 0;
            }
            return da == 9;
        }));
        add(klickPopup(tr(Str::MbFlWurzel)));
        add(pause(0.4));
        add(pruefe(zeile("X19", modus, "Als Wurzel setzen"), [] { return mtab().g2.wurzel == teil("torso"); }));
        add(mbefoto::baumRolleZu("torso", [] { return finde("torso", "mtbaum", 0, false); }, 2.0F));
        add(klick("torso", "mtbaum", 0, false, "", 1));
        add(pause(0.4));
        add(klickPopup(tr(Str::MbXWurzelLoesen)));
        add(pause(0.4));
        add(pruefe(zeile("X19", modus, "Wurzel loesen"), [] { return mtab().g2.wurzel == 0; }));
    }
    // R7: vier Ansichten je Bereich wie 3ds Max
    add(tu("AP5 R7: wie 3ds Max, Ansehen/Pruefen, eine Ansicht, die Wahl je Bereich auf Werk", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Ansehen;
        mtab().vierAnsichten = false;
        for (int& v : modellPr().vierJeBereich) { v = -1; }
    }));
    add(pause(0.5));
    static const char* const kName[kModellBereiche] = {"Ansehen/Pruefen", "Modellieren", "UV", "Weights", "Painting", "Animation"};
    static const bool kVier[kModellBereiche] = {false, true, false, true, true, true};
    for (int b : {1, 2, 3, 4, 5, 0}) {
        add(klickAn(std::string("AP5 R7: Bereich ") + kName[b], [b] {
            const Element* e = ap3::kopfFeld(b);
            return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
        }));
        add(pause(0.5));
        add(pruefe(std::string("AP5 R7 wie 3ds Max: ") + kName[b] + (kVier[b] ? " zeigt ab Werk vier Ansichten (Oben, Vorne, Links, Perspektive)" : " zeigt eine Ansicht"), [b] {
            return static_cast<int>(mtab().bereich) == b && mtab().vierAnsichten == kVier[b] && (!kVier[b] || (mtab().ansichten[0].art == AnsichtArt::Oben && mtab().ansichten[1].art == AnsichtArt::Vorne &&
                                                                                                                 mtab().ansichten[2].art == AnsichtArt::Links && mtab().ansichten[3].art == AnsichtArt::Perspektive));
        }));
    }
    add(klickAn("AP5 R7: Bereich Modellieren", [] {
        const Element* e = ap3::kopfFeld(1);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    }));
    add(pause(0.4));
    add(klickAn("AP5 R7: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.2));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_W, "Alt+W"));
    add(pause(0.3));
    add(pruefe("AP5 R7 wie 3ds Max: Alt+W maximiert die aktive Ansicht in Modellieren", [] { return !mtab().vierAnsichten; }));
    for (int b : {3, 1}) {
        add(klickAn(std::string("AP5 R7: Bereich ") + kName[b], [b] {
            const Element* e = ap3::kopfFeld(b);
            return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
        }));
        add(pause(0.4));
    }
    add(pruefe("AP5 R7 wie 3ds Max: zurueck in Modellieren bleibt die Wahl (eine Ansicht), Weights hatte vier", [] { return mtab().bereich == ModellBereich::Modellieren && !mtab().vierAnsichten; }));
    add(tu("AP5 R7: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(0.4));
    add(klickAn("AP5 R7: Bereich Weights", [] {
        const Element* e = ap3::kopfFeld(3);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    }));
    add(pause(0.4));
    add(pruefe("AP5 R7 wie Blender: eine grosse Ansicht, auch in Weights", [] { return mtab().bereich == ModellBereich::Weights && !mtab().vierAnsichten; }));
    add(tu("AP5 R7: zurueck", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Ansehen;
        mtab().vierAnsichten = false;
        modellPr() = ModellPruefen{};
    }));
    add(pause(0.4));
}

// --- Auffindbar: Alle Aktionen suchen -------------------------------------------------------------------------------------------

void sucheSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        std::vector<Str> eintraege = {Str::PqFlaechenliste, Str::PqRBoden, Str::PqRAuswahl, Str::PqRSzene, Str::PqImport, Str::PqAlpha, Str::PqFilter, Str::PqSchwelle,
                                      Str::PqHervor, Str::PqSchatten};
        if (modus == 1) {
            eintraege.push_back(Str::PqOverlays);
            eintraege.push_back(Str::PqSchattierungMenue);
        }
        for (const Str z : eintraege) {
            add(tu(std::string("AP5 Suche: ") + (modus == 0 ? "wie 3ds Max" : "wie Blender") + ", " + tr(z), [modus] {
                modellBedienungSetzen(modus);
                mtab().bereich = ModellBereich::Ansehen;
                mtab().aktionenOffen = true;
            }));
            add(pause(0.4));
            add(tippe(tr(z)));
            add(pause(0.3));
            add(pruefe(zeile("Z20", modus, std::string("Alle Aktionen suchen findet \"") + tr(z) + "\" (Flaechenliste, Gruppen von Ansehen/Pruefen, die Schalter von ModView, die Tasten wie Blender)"),
                       [z] {
                           std::string alle;
                           for (const Element& e : g_elemente) {
                               if (e.fenster.find("##Popup_") == std::string::npos) { continue; }
                               if (e.label.find(tr(z)) != std::string::npos && e.label.rfind("##", 0) != 0) { return true; }
                               alle += e.label + " | ";
                           }
                           melde("Suche: im Menue nur " + alle);
                           return false;
                       }));
            add(mbefoto::popupsZu());
            add(pause(0.2));
        }
    }
    auto listen = std::make_shared<int>(0);
    add(tu("AP5 Suche: Flaechenliste ueber die Suche", [listen] {
        *listen = g_app->modellFlaechenListen;
        mtab().aktionenOffen = true;
    }));
    add(pause(0.4));
    add(tippe(tr(Str::PqFlaechenliste)));
    add(pause(0.3));
    add(klickAn("AP5 Suche: Flaechenliste", [] {
        for (const Element& e : g_elemente) {
            if (e.fenster.find("##Popup_") != std::string::npos && e.label.rfind(tr(Str::PqFlaechenliste), 0) == 0) { return e.rect.GetCenter(); }
        }
        return ImVec2{-50.0F, -50.0F};
    }));
    add(pause(0.3));
    add(pruefe("AP5 Z20: die Flaechenliste aus der Suche schreibt die Flaechen ins Detailprotokoll", [listen] { return g_app->modellFlaechenListen == *listen + 1; }));
}

// --- Fotos ----------------------------------------------------------------------------------------------------------------

void fotoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const std::string p = modus == 0 ? "ap5_max_" : "ap5_blender_";
        add(tu("AP5 Foto: " + p + "pruefen", [modus] {
            modellBedienungSetzen(modus);
            mtab().bereich = ModellBereich::Ansehen;
            mtab().vierAnsichten = false;
            mtab().auswahl = {teil("torso")};
            if (mtab().anbauten.empty()) { (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true); }
            if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
            setzeMaus(ImVec2{-50.0F, -50.0F});
        }));
        add(pause(1.2));
        add(foto(p + "pruefen"));
        mbefoto::befehlsfeld(s, p + "pruefen");
        add(tu("AP5 Foto: Pruefanzeigen an (Schatten, Hervorheben)", [] {
            modellPr().schatten = true;
            modellPr().hervorheben = true;
            g_app->modelDirty = true;
        }));
        add(pause(0.8));
        add(foto(p + "pruefanzeigen"));
        add(tu("AP5 Foto: Pruefanzeigen aus, Modellieren", [] {
            modellPr().schatten = false;
            modellPr().hervorheben = false;
            mtab().bereich = ModellBereich::Modellieren;
        }));
        add(pause(1.2));
        add(foto(p + "modell"));
    }
    add(tu("AP5 Foto: wie Blender, Ansehen/Pruefen, Z", [] {
        modellBedienungSetzen(1);
        mtab().bereich = ModellBereich::Ansehen;
        modellPr().schattierungOeffnen = true;
    }));
    add(pause(0.8));
    add(foto("ap5_blender_schattierung"));
    add(mbefoto::popupsZu());
    add(pause(0.3));
}

// Bilder fuer das Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap5): h_ap5_max, h_ap5_blender (Ansehen/Pruefen), h_ap5_pruefen
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    add(tu("AP5 Handbuch: Ansehen/Pruefen, torso, saber_1, wie 3ds Max", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        g_app->settings.splitModel = 0.85F;
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        g_app->mtab.auswahl = {teil("torso")};
        if (g_app->mtab.anbauten.empty()) { (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true); }
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
        setzeMaus(ImVec2{-50.0F, -50.0F});
    }));
    add(pause(1.2));
    add(hilfe2::schrittFoto("h_ap5_max"));
    add(tu("AP5 Handbuch: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(1.0));
    add(hilfe2::schrittFoto("h_ap5_blender"));
    add(tu("AP5 Handbuch: wie 3ds Max, Pruefanzeigen, Feld bis zur Pruefung gerollt", [] {
        modellBedienungSetzen(0);
        modellPr().schatten = true;
        modellPr().hervorheben = true;
        g_app->modelDirty = true;
    }));
    add(insFeld(tr(Str::PqPrSchatten)));
    add(pause(1.0));
    add(hilfe2::schrittFoto("h_ap5_pruefen"));
    add(tu("AP5 Handbuch: zurueck", [=] {
        modellPr() = ModellPruefen{};
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->mtab.bereich = ModellBereich::Ansehen;
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
    if (n != "ap5" && n != "ap5foto") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_AP5_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    auto skala = std::make_shared<float>(1.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP5: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        if (const char* t = std::getenv("BHED_AP5_THEMA")) {
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
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap5.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    add(tu("AP5: Aufteilung wie neue Einstellungen, wie 3ds Max, Bereich Ansehen/Pruefen", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(0);
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        modellPr() = ModellPruefen{};
        modellExplorerVorgabe();
        animWaehlen(mbefoto::sequenz(), false);
        g_app->animPlaying = false;
        char zeile[300];
        std::snprintf(zeile, sizeof zeile, "%s, %zu Knochen, %d Haeute, Anzeige %.0f x %.0f, Oberflaeche %.2f, Schrift %.0f Punkte hoch", g_app->modelPath.c_str(),
                      g_app->anim.bones.size(), static_cast<int>(g_app->modelSkins.size()), static_cast<double>(ImGui::GetIO().DisplaySize.x),
                      static_cast<double>(ImGui::GetIO().DisplaySize.y), static_cast<double>(g_app->uiScale), static_cast<double>(ImGui::GetFontSize()));
        melde(zeile);
    }));
    add(pause(1.0));
    add(klickAn("AP5: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (n == "ap5") {
        gruppenSchritte(s, 0);
        gruppenSchritte(s, 1);
        anzeigeSchritte(s, 0);
        anzeigeSchritte(s, 1);
        szeneSchritte(s);
        restSchritte(s);
        sucheSchritte(s);
    } else {
        fotoSchritte(s);
    }
    add(tu("AP5: Modell-Tab zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        modellPr() = ModellPruefen{};
        modellAnbautenLoesen();
    }));
    add(tu("AP5: zurueck", [=] {
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

}  // namespace ap5
