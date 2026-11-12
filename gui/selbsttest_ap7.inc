// selbsttest_ap7.inc - AP7 (MBENTWURF Teil B, 11.11.): das rechte Feld des Bereichs UV nach dem freigegebenen Entwurf
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap7 | ap7foto)
//
//   ap7       je Zeile der Abgleichtabelle (plaene/modellblock_stufe0/abgleich_programm_entwurf.md, Abschnitt 9: U0a-U10)
//             Pruefungen "AP7 U<nr> wie 3ds Max" bzw. "wie Blender": Ort, Element, Bedienung - die Gruppen Auswahl,
//             1 Naehte festlegen, 2 Abwickeln, 3 Aufraeumen, Pruefen ueber den Baustein gui/ui_rollout.h, der Hauptknopf,
//             die Wertetabelle, die Tipps, die Tasten wie Blender (L, A, Strg+A, Strg+P); dazu: nichts ging verloren (jeder
//             Befehl von rc611 steht im Feld).
//   ap7foto   nur Fotos (BHED_FOTOS=<Ordner>): der Bereich UV in beiden Bedienmodi (ap7_max_uv, ap7_blender_uv) und das
//             rechte Feld seitenweise, erst wie ab Werk ("Weitere" zu: ..._feld<n>), dann alles aufgeklappt (..._auf<n>).
//   BHED_AP7_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe wie der Foto-Selbsttest mbefoto)
//   BHED_AP7_MODELL=<.glm>           Vorgabe models/players/kyle/model.glm (das Modell des Entwurfs)
//   BHED_AP7_THEMA=hell|dunkel       das erste helle bzw. dunkle Farbschema (Vorgabe: das eingestellte)
// Eingebunden in selbsttest.cpp (nach selbsttest_ap3.inc); Handbuchbilder BHED_HB_NUR=ap7 (h_ap7_max, h_ap7_blender).
namespace ap7 {

using mbefoto::inFenster;
using mbefoto::kindFenster;

namespace w = modell::wof;

float em() { return ImGui::GetFontSize(); }

void melde(const std::string& z) { diag::detail("Knopftest: AP7 " + z); }

std::string zeile(const char* nr, int modus, const std::string& was) { return std::string("AP7 ") + nr + (modus == 0 ? " wie 3ds Max: " : " wie Blender: ") + was; }

const Element* imRoll(const std::string& label) { return inFenster(label, "mtrollouts"); }

bool feldFrisch() { return ui::feldProbe().bild >= ImGui::GetFrameCount() - 2; }

// Die Gruppe des rechten Felds mit diesem sichtbaren Titel (im letzten Bild gezeichnet).
const ui::RolloutProbe* gruppe(Str titel) {
    const std::string name = tr(titel);
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
        if (ui::rolloutProbe(i).titel == name) { return &ui::rolloutProbe(i); }
    }
    return nullptr;
}

std::string gruppenTitel() {
    std::string s;
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) { s += (i > 0 ? "|" : "") + ui::rolloutProbe(i).titel; }
    return s;
}

// Die Beschriftung einer Gruppe bzw. eines Unterteils im Element-Register: Titel und Kennung, wie gui/wof/app_modelltab_uv.cpp
// sie setzt (uvrwahl, uvrnaht, uvrabw, uvrauf; uvmehrnaht, uvmehrabw, uvmehrauf, uvmehrpruef).
std::string kopf(Str titel, const char* kennung) { return std::string(tr(titel)) + "###" + kennung; }

// Die Beschriftung einer Auswahlebene im Segment: die Taste des Bedienmodus und der Name ("4 Face").
std::string ebene(Str s, bool blender) {
    const int i = uv1b::aktion(s);
    return i < 0 ? std::string() : std::string(modellUvAktionTaste(i, blender)) + " " + modellUvAktionText(i);
}

Schritt rolle(Str titel) {
    return tu(std::string("AP7: Gruppe \"") + tr(titel) + "\" ins Bild rollen", [titel] {
        const ui::RolloutProbe* g = gruppe(titel);
        ImGuiWindow* f = kindFenster("mtrollouts");
        if (g != nullptr && f != nullptr) { ImGui::SetScrollY(f, std::clamp(f->Scroll.y + g->kopf.Min.y - f->InnerRect.Min.y - em() * 0.5F, 0.0F, f->ScrollMax.y)); }
    });
}

using EckMenge = std::set<std::pair<int, int>>;

EckMenge wahlJetzt() {
    EckMenge s;
    for (const modell::EckOrt& o : g_app->mtab.eckenWahl) { s.insert({o.teil, o.ecke}); }
    return s;
}

EckMenge eckenVonInsel(int insel) {
    const ModellUv& u = g_app->mtab.uv;
    EckMenge s;
    if (insel < 0 || insel >= u.inseln.zahl) { return s; }
    for (const int d : u.inseln.dreieckeVon(insel)) {
        for (int k = 0; k < 3; ++k) {
            const std::size_t h = static_cast<std::size_t>(d * 3 + k);
            s.insert({u.netz.teile[static_cast<std::size_t>(u.netz.teil[h])], u.netz.ecke[h]});
        }
    }
    return s;
}

EckMenge alleEcken() {
    const ModellUv& u = g_app->mtab.uv;
    EckMenge s;
    for (int h = 0; h < u.netz.kanten(); ++h) {
        s.insert({u.netz.teile[static_cast<std::size_t>(u.netz.teil[static_cast<std::size_t>(h)])], u.netz.ecke[static_cast<std::size_t>(h)]});
    }
    return s;
}

bool imDreieckUv(w::V2 p, w::V2 a, w::V2 b, w::V2 c) {
    const auto f = [](w::V2 p1, w::V2 p2, w::V2 p3) { return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y); };
    const float d1 = f(p, a, b);
    const float d2 = f(p, b, c);
    const float d3 = f(p, c, a);
    return !((d1 < 0 || d2 < 0 || d3 < 0) && (d1 > 0 || d2 > 0 || d3 > 0));
}

// Ein Dreieck, dessen Mitte im UV-Editor sichtbar ist und nur von Dreiecken seiner eigenen Insel bedeckt wird: die Stelle am
// Schirm (wie drawModellUvEditor rechnet) und die Insel. false: keins gefunden.
bool freieInselStelle(ImVec2& ort, int& insel) {
    const ModellUv& u = g_app->mtab.uv;
    const w::UvNetz& n = u.netz;
    const float m = std::min(u.leinwand[2], u.leinwand[3]) * u.zoom;
    const auto uvVon = [&](int h) { return n.uv[static_cast<std::size_t>(h)]; };
    for (int d = 0; d < n.dreiecke(); ++d) {
        const w::V2 p{(uvVon(d * 3).x + uvVon(d * 3 + 1).x + uvVon(d * 3 + 2).x) / 3.0F, (uvVon(d * 3).y + uvVon(d * 3 + 1).y + uvVon(d * 3 + 2).y) / 3.0F};
        const ImVec2 s{u.leinwand[0] + u.leinwand[2] * 0.5F + (p.x - u.mitte[0]) * m, u.leinwand[1] + u.leinwand[3] * 0.5F + (p.y - u.mitte[1]) * m};
        if (s.x < u.leinwand[0] + em() || s.y < u.leinwand[1] + em() || s.x > u.leinwand[0] + u.leinwand[2] - em() || s.y > u.leinwand[1] + u.leinwand[3] - em() * 3.0F) { continue; }
        const int i = u.inseln.inselVonDreieck(d);
        bool allein = true;
        for (int e = 0; e < n.dreiecke() && allein; ++e) {
            if (u.inseln.inselVonDreieck(e) != i && imDreieckUv(p, uvVon(e * 3), uvVon(e * 3 + 1), uvVon(e * 3 + 2))) { allein = false; }
        }
        if (allein) {
            ort = s;
            insel = i;
            return true;
        }
    }
    return false;
}

// Aenderungen am Modell zuruecknehmen, bis es wieder die Bytes von `soll` hat (hoechstens acht Schritte).
Schritt zurueckAuf(std::shared_ptr<std::string> soll) {
    return tu("AP7: Rueckgaengig bis zum Ausgangsmodell", [soll] {
        for (int k = 0; k < 8 && uv1b::glmJetzt() != *soll; ++k) {
            if (!g_app->mtab.dok.rueckgaengig()) { break; }
        }
        if (uv1b::glmJetzt() != *soll) { meldeFehler("AP7: das Modell ist nach Rueckgaengig nicht wieder das Ausgangsmodell"); }
        g_app->modelDirty = true;
    });
}

// Maus ueber den UV-Editor (dort gelten die Tasten des Bereichs), Fokus im Model-Tab.
Schritt ueberEditor() {
    return tu("AP7: Maus ueber den UV-Editor", [] {
        setzeMaus(uv1b::leinwandMitte());
        g_app->mtab.fokus = true;
        g_app->mtab.letzteTaste.clear();
    });
}

void uvSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto anfang = std::make_shared<std::string>();
    for (int modus = 0; modus < 2; ++modus) {
        const bool bl = modus == 1;
        add(tu(std::string("AP7: Bedienmodus ") + (bl ? "wie Blender" : "wie 3ds Max") + ", Bereich UV, torso gewaehlt, alles wie ab Werk", [=] {
            modellBedienungSetzen(modus);
            ModellTab& t = g_app->mtab;
            t.bereich = ModellBereich::Uv;
            g_app->settings.modellFeldBreite = 0.0F;
            t.lodWahl = 1;   // LOD 1 fest (die Ansicht waehlt sonst nach dem Abstand)
            g_app->animPlaying = false;
            t.eckenWahl.clear();
            t.auswahl = {mb1::teil("torso")};
            ModellUv& u = t.uv;
            u.offen = true;
            u.gruppe = -1;
            u.hintergrund = 0;
            u.dehnungZeigen = false;
            u.ueberlappZeigen = true;
            u.drehen = true;
            u.abstand = 4;
            u.packUmfang = 0;
            u.projAchse = 3;
            modellUvFeld() = ModellUvFeld{};
            const int flaeche = uv1b::aktion(Str::UvEbeneFlaeche);
            if (flaeche >= 0) { modellUvAktionAusloesen(flaeche); }
            if (ImGuiWindow* f = kindFenster("mtrollouts")) {
                ImGui::SetScrollY(f, 0.0F);
                for (const char* k : {"uvmehrnaht", "uvmehrabw", "uvmehrauf", "uvmehrpruef"}) { f->StateStorage.SetInt(f->GetID(kopf(Str::A7Mehr, k).c_str()), 0); }
            }
            g_app->modelDirty = true;
        }));
        add(pause(0.9));
        add(tu("AP7: Ausgangsmodell merken", [anfang] { *anfang = uv1b::glmJetzt(); }));

        // --- U0a / U0b: Aufbau des Felds ---
        const std::string soll = std::string(tr(Str::A7RAuswahl)) + "|" + tr(Str::A7RNaehte) + "|" + tr(Str::UvGrAbwickeln) + "|" + tr(Str::A7RAufraeumen) + "|" + tr(Str::UvGrPruefen);
        const char* u0 = bl ? "U0b" : "U0a";
        add(pruefe(zeile(u0, modus, bl ? "Eigenschaften, Reiter Daten: die fuenf Gruppen des Entwurfs untereinander - Auswahl, Naehte festlegen, Abwickeln, Aufraeumen, Pruefen"
                                      : "Befehlsfeld, Reiter Aendern: die fuenf Gruppen des Entwurfs untereinander - Auswahl, Naehte festlegen, Abwickeln, Aufraeumen, Pruefen"),
                   [=] {
                       melde(std::string(u0) + " Gruppen: " + gruppenTitel());
                       const ui::FeldProbe& f = ui::feldProbe();
                       const int reiter = bl ? static_cast<int>(MbReiterBl::Daten) : static_cast<int>(MbReiter::Aendern);
                       if (!feldFrisch() || gruppenTitel() != soll || f.reiterAktiv != reiter) { return false; }
                       float y = -1.0F;
                       for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
                           const ui::RolloutProbe& g = ui::rolloutProbe(i);
                           if (!g.offen || g.kasten.Min.y < y) { return false; }
                           y = g.kasten.Max.y;
                       }
                       return true;
                   }));
        add(pruefe(zeile(u0, modus, "die Arbeitsschritte tragen die Nummern 1, 2, 3 (Naehte festlegen, Abwickeln, Aufraeumen); Auswahl und Pruefen haben keine"), [] {
            const ui::RolloutProbe* a = gruppe(Str::A7RAuswahl);
            const ui::RolloutProbe* n = gruppe(Str::A7RNaehte);
            const ui::RolloutProbe* ab = gruppe(Str::UvGrAbwickeln);
            const ui::RolloutProbe* au = gruppe(Str::A7RAufraeumen);
            const ui::RolloutProbe* p = gruppe(Str::UvGrPruefen);
            return a != nullptr && n != nullptr && ab != nullptr && au != nullptr && p != nullptr && a->schritt == 0 && n->schritt == 1 && ab->schritt == 2 && au->schritt == 3 && p->schritt == 0;
        }));
        add(pruefe(zeile(u0, modus, bl ? "die Gruppen im Stil wie Blender: abgerundete Kaesten" : "die Gruppen im Stil wie 3ds Max: flache Balken, eckig"), [=] {
            const ui::RolloutProbe* n = gruppe(Str::A7RNaehte);
            if (n == nullptr) { return false; }
            return bl ? (n->stil == ui::RolloutStil::Blender && n->rundung >= std::floor(em() * 0.3F)) : (n->stil == ui::RolloutStil::Max && n->rundung <= 2.0F);
        }));

        // --- U1: UV-Fenster und Ansicht (gleich geblieben) ---
        add(pruefe(zeile("U1", modus, "der UV-Editor steht mit der Textur neben der Ansicht, die Naehte liegen auch auf dem Modell"), [] {
            const ModellUv& u = g_app->mtab.uv;
            return u.leinwand[2] > 200.0F && u.texturDa && u.dreieckeGezeichnet > 100 && u.nahtStriche3d > 0 && u.inseln.zahl > 0;
        }));

        // --- U2: Gruppe Auswahl ---
        add(pruefe(zeile("U2", modus, "Auswahl: die Kurzinfo nennt rechts in der Ueberschrift Textur bzw. Flaeche und die Zahl der Inseln"), [] {
            const ui::RolloutProbe* a = gruppe(Str::A7RAuswahl);
            if (a == nullptr) { return false; }
            melde("U2 Kurzinfo: " + a->kurzinfo);
            const std::size_t punkt = a->kurzinfo.find(" \xc2\xb7 ");
            const int inseln = g_app->mtab.uv.inseln.zahl;
            return punkt != std::string::npos && punkt > 0 && a->kurzinfo.find(std::to_string(inseln), punkt) != std::string::npos && a->info.GetWidth() > 0.0F && a->kopf.Contains(a->info);
        }));
        add(pruefe(zeile("U2", modus, "Auswahl: die Wahl der Textur als Formularzeile in der Gruppe"), [] {
            const ui::RolloutProbe* a = gruppe(Str::A7RAuswahl);
            ImGuiWindow* f = kindFenster("mtrollouts");
            const Element* wahl = f != nullptr ? findeKennung(f->GetID("##uvgruppe")) : nullptr;
            return a != nullptr && wahl != nullptr && a->kasten.Contains(wahl->rect) && wahl->rect.Min.x > a->kasten.Min.x + em() * 2.0F;
        }));
        add(pruefe(zeile("U2", modus, bl ? "Auswahl: die Ebenen als ein Segment mit der Taste des Bedienmodus - 1 Ecke, 2 Kante, 3 Flaeche, 4 Insel; Flaeche ist gewaehlt"
                                        : "Auswahl: die Ebenen als ein Segment mit der Taste des Bedienmodus - 1 Ecke, 2 Kante, 4 Flaeche, 5 Insel; Flaeche ist gewaehlt"),
                   [=] {
                       const ui::RolloutProbe* a = gruppe(Str::A7RAuswahl);
                       const Element* e[4] = {imRoll(ebene(Str::UvEbeneEcke, bl)), imRoll(ebene(Str::UvEbeneKante, bl)), imRoll(ebene(Str::UvEbeneFlaeche, bl)), imRoll(ebene(Str::UvEbeneInsel, bl))};
                       if (a == nullptr || e[0] == nullptr || e[1] == nullptr || e[2] == nullptr || e[3] == nullptr) { return false; }
                       melde("U2 Ebenen: \"" + e[0]->label + "\" " + mbefoto::rechteck("", e[0]->rect) + " \"" + e[2]->label + "\" " + mbefoto::rechteck("", e[2]->rect) + " \"" + e[3]->label + "\" " +
                             mbefoto::rechteck("", e[3]->rect));
                       bool ok = (e[2]->flags & ImGuiItemStatusFlags_Checked) != 0 && (e[0]->flags & ImGuiItemStatusFlags_Checked) == 0;
                       for (int i = 0; i < 4; ++i) { ok = ok && a->kasten.Contains(e[i]->rect) && (e[i]->flags & ImGuiItemStatusFlags_Checkable) != 0; }
                       // eine Reihe ueber die Breite - oder gleichmaessig umbrochen (zwei mal zwei), wenn die Texte nicht passen
                       const bool eineReihe = e[0]->rect.Min.y == e[3]->rect.Min.y && e[3]->rect.Max.x - e[0]->rect.Min.x >= a->kasten.GetWidth() - em() * 1.2F;
                       const bool zweiReihen = e[0]->rect.Min.y == e[1]->rect.Min.y && e[2]->rect.Min.y == e[3]->rect.Min.y && e[2]->rect.Min.y >= e[0]->rect.Max.y - 1.0F;
                       return ok && (eineReihe || zweiReihen);
                   }));
        add(klick(ebene(Str::UvEbeneKante, bl), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("U2", modus, "Auswahl: ein Klick auf Kante stellt die Ebene um - im UV-Editor und in der Ansicht"), [=] {
            const Element* k = imRoll(ebene(Str::UvEbeneKante, bl));
            return g_app->mtab.uv.ebene == 1 && g_app->mtab.auswahlArt == AuswahlArt::Kante && k != nullptr && (k->flags & ImGuiItemStatusFlags_Checked) != 0;
        }));
        add(klick(ebene(Str::UvEbeneFlaeche, bl), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("U2", modus, bl ? "Auswahl: Insel waehlen (L) und Alles waehlen (A) im Raster mit zwei gleich breiten Spalten, jeder mit Symbol und Taste"
                                        : "Auswahl: Insel waehlen und Alles waehlen (Strg+A) im Raster mit zwei gleich breiten Spalten, jeder mit Symbol"),
                   [=] {
                       const ui::RolloutProbe* a = gruppe(Str::A7RAuswahl);
                       const Element* insel = imRoll(tr(Str::A7InselWaehlen));
                       const Element* alles = imRoll(tr(Str::A7AllesWaehlen));
                       if (a == nullptr || insel == nullptr || alles == nullptr) { return false; }
                       melde("U2 Raster: " + mbefoto::rechteck("insel", insel->rect) + " " + mbefoto::rechteck("alles", alles->rect) + ", Spalten " + std::to_string(a->rasterSpalten) + ", Knoepfe " +
                             std::to_string(a->knoepfe) + ", Symbole " + std::to_string(a->symbole) + ", Tasten " + std::to_string(a->tasten));
                       return a->rasterSpalten == 2 && a->knoepfe == 2 && a->symbole == 2 && a->tasten == (bl ? 2 : 1) && std::fabs(insel->rect.GetWidth() - alles->rect.GetWidth()) < 0.5F &&
                              insel->rect.Min.y == alles->rect.Min.y && alles->rect.Min.x > insel->rect.Max.x && a->kasten.Contains(insel->rect) && a->kasten.Contains(alles->rect);
                   }));
        add(klick(tr(Str::A7AllesWaehlen), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("U2", modus, "Auswahl: Alles waehlen waehlt jede UV-Ecke der Flaechen dieser Textur"), [] {
            const EckMenge alle = alleEcken();
            melde("U2 Alles: " + std::to_string(wahlJetzt().size()) + " von " + std::to_string(alle.size()) + " Ecken gewaehlt");
            return !alle.empty() && wahlJetzt() == alle;
        }));
        auto inselNr = std::make_shared<int>(-1);
        add(tu("AP7: nur ein Dreieck gewaehlt (das erste des Netzes)", [inselNr] {
            ModellTab& t = g_app->mtab;
            const ModellUv& u = t.uv;
            t.eckenWahl.clear();
            t.auswahl.clear();
            if (u.netz.dreiecke() == 0) { return; }
            *inselNr = u.inseln.inselVonDreieck(0);
            for (std::size_t h = 0; h < 3; ++h) { t.eckenWahl.push_back(modell::EckOrt{u.netz.teile[static_cast<std::size_t>(u.netz.teil[h])], u.netz.ecke[h]}); }
            g_app->modelDirty = true;
        }));
        add(pause(0.3));
        add(klick(tr(Str::A7InselWaehlen), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("U2", modus, "Auswahl: Insel waehlen erweitert die Auswahl auf die ganze Insel des gewaehlten Dreiecks"), [inselNr] {
            const EckMenge solche = eckenVonInsel(*inselNr);
            melde("U2 Insel " + std::to_string(*inselNr) + ": " + std::to_string(wahlJetzt().size()) + " Ecken gewaehlt, die Insel hat " + std::to_string(solche.size()));
            return solche.size() > 3U && wahlJetzt() == solche;
        }));
        add(pruefe(zeile("U2", modus, "Auswahl: der Schalter UV-Editor steht in der Gruppe und ist an"), [] {
            const ui::RolloutProbe* a = gruppe(Str::A7RAuswahl);
            const Element* e = imRoll(tr(Str::UvEditorZeigen));
            return a != nullptr && e != nullptr && a->kasten.Contains(e->rect) && (e->flags & ImGuiItemStatusFlags_Checked) != 0;
        }));
        // wie bisher: auf der Ebene Insel waehlt ein Klick im UV-Editor die ganze Insel (derselbe Treffer wie die Taste L)
        add(klick(ebene(Str::UvEbeneInsel, bl), "mtrollouts"));
        add(pause(0.3));
        auto klickInsel = std::make_shared<int>(-1);
        add(klickAn("AP7: auf eine Insel im UV-Editor", [klickInsel] {
            g_app->mtab.eckenWahl.clear();
            g_app->mtab.auswahl.clear();
            ImVec2 ort{};
            if (!freieInselStelle(ort, *klickInsel)) { meldeFehler("AP7 U1: keine freie Stelle einer Insel im UV-Editor gefunden"); }
            return ort;
        }));
        add(pause(0.4));
        add(pruefe(zeile("U1", modus, "Ebene Insel: ein Klick im UV-Editor waehlt die ganze Insel unter der Maus"), [klickInsel] {
            const EckMenge solche = eckenVonInsel(*klickInsel);
            melde("U1 Klick: Insel " + std::to_string(*klickInsel) + ", " + std::to_string(wahlJetzt().size()) + " Ecken gewaehlt, die Insel hat " + std::to_string(solche.size()));
            return g_app->mtab.uv.ebene == 3 && !solche.empty() && wahlJetzt() == solche;
        }));
        add(klick(ebene(Str::UvEbeneFlaeche, bl), "mtrollouts"));
        add(pause(0.3));

        // --- U3: 1 Naehte festlegen ---
        add(pruefe(zeile("U3", modus, bl ? "Naehte festlegen: Naht markieren (Strg+E) und Naht loeschen im Raster, darunter Naehte aus dem Skelett ueber die volle Breite, jeder mit Symbol"
                                        : "Naehte festlegen: Naht markieren (C) und Naht loeschen (Umschalt+C) im Raster, darunter Naehte aus dem Skelett ueber die volle Breite, jeder mit Symbol"),
                   [=] {
                       const ui::RolloutProbe* n = gruppe(Str::A7RNaehte);
                       const Element* an = imRoll(tr(Str::UvNahtMarkieren));
                       const Element* aus = imRoll(tr(Str::UvNahtLoeschen));
                       const Element* skelett = imRoll(tr(Str::UvNaehteSkelett));
                       if (n == nullptr || an == nullptr || aus == nullptr || skelett == nullptr) { return false; }
                       melde("U3 Knoepfe: " + mbefoto::rechteck("markieren", an->rect) + " " + mbefoto::rechteck("loeschen", aus->rect) + " " + mbefoto::rechteck("skelett", skelett->rect) + ", Knoepfe " +
                             std::to_string(n->knoepfe) + ", Symbole " + std::to_string(n->symbole) + ", Tasten " + std::to_string(n->tasten) + ", Hauptknoepfe " + std::to_string(n->hauptKnoepfe));
                       return n->knoepfe == 3 && n->symbole == 3 && n->tasten == (bl ? 1 : 2) && n->hauptKnoepfe == 0 && std::fabs(an->rect.GetWidth() - aus->rect.GetWidth()) < 0.5F &&
                              an->rect.Min.y == aus->rect.Min.y && aus->rect.Min.x > an->rect.Max.x && skelett->rect.Min.y >= an->rect.Max.y &&
                              skelett->rect.GetWidth() >= n->kasten.GetWidth() - em() * 1.2F && n->kasten.Contains(an->rect) && n->kasten.Contains(aus->rect) && n->kasten.Contains(skelett->rect);
                   }));
        add(pruefe(zeile("U3", modus, "Naehte festlegen: die uebrigen Befehle stehen eingeklappt unter Weitere, am Ende der Gruppe der Tipp als Kasten"), [] {
            const ui::RolloutProbe* n = gruppe(Str::A7RNaehte);
            const Element* mehr = imRoll(kopf(Str::A7Mehr, "uvmehrnaht"));
            const Element* tipp = imRoll(tr(Str::A7TippNaehte));
            const Element* skelett = imRoll(tr(Str::UvNaehteSkelett));
            if (n == nullptr || mehr == nullptr || tipp == nullptr || skelett == nullptr) { return false; }
            return (mehr->flags & ImGuiItemStatusFlags_Openable) != 0 && (mehr->flags & ImGuiItemStatusFlags_Opened) == 0 && imRoll(tr(Str::UvNaehteForm)) == nullptr &&
                   mehr->rect.Min.y >= skelett->rect.Max.y && tipp->rect.Min.y >= mehr->rect.Max.y - 1.0F && n->kasten.Contains(tipp->rect) &&
                   tipp->rect.GetWidth() >= n->kasten.GetWidth() - em() * 1.2F && n->kasten.Max.y - tipp->rect.Max.y <= em();
        }));
        add(klick(kopf(Str::A7Mehr, "uvmehrnaht"), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("U3", modus, "Naehte festlegen: ein Klick auf Weitere zeigt Naehte nach Form, Naehte nach Winkel mit dem Winkelregler, Ring aufschneiden, Naehte spiegeln"), [] {
            const ui::RolloutProbe* n = gruppe(Str::A7RNaehte);
            if (n == nullptr) { return false; }
            for (const Str x : {Str::UvNaehteForm, Str::UvNaehteWinkel, Str::UvRingSchneiden, Str::UvSpiegeln, Str::MdWinkel}) {
                const Element* e = imRoll(tr(x));
                if (e == nullptr || !n->kasten.Contains(e->rect)) {
                    melde(std::string("U3 Weitere: fehlt oder liegt ausserhalb der Gruppe: ") + tr(x));
                    return false;
                }
            }
            return true;
        }));
        add(klick(kopf(Str::A7Mehr, "uvmehrnaht"), "mtrollouts"));
        add(pause(0.2));
        auto marken = std::make_shared<int>(0);
        add(tu("AP7: halber Torso gewaehlt (Flaechen)", [marken] {
            uv1b::halbeFlaecheWaehlen("torso");
            *marken = uv1b::markenZahl();
        }));
        add(pause(0.3));
        add(klick(tr(Str::UvNahtMarkieren), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(zeile("U3", modus, "Naehte festlegen: der Knopf Naht markieren macht den Rand der Auswahl zur Naht (mehr Marken)"), [marken] {
            const int m = uv1b::markenZahl();
            melde("U3 Marken: " + std::to_string(*marken) + " -> " + std::to_string(m));
            return m > *marken;
        }));
        // die Marken stehen nicht in der .glm: hier genau den einen Schritt zurueck (zurueckAuf vergleicht die Bytes der Datei)
        add(tu("AP7: Rueckgaengig (Naht markieren)", [marken] {
            if (uv1b::markenZahl() > *marken) { (void)g_app->mtab.dok.rueckgaengig(); }
            g_app->modelDirty = true;
        }));
        add(pause(0.3));
        add(pruefe(zeile("U3", modus, "Naehte festlegen: Rueckgaengig nimmt die Naht wieder weg"), [marken] { return uv1b::markenZahl() == *marken; }));

        // --- U4: 2 Abwickeln ---
        add(rolle(Str::UvGrAbwickeln));
        add(pause(0.3));
        add(pruefe(zeile("U4", modus, "Abwickeln: der Hauptknopf Abwickeln ueber die volle Breite, hervorgehoben und hoeher als die Knoepfe des Rasters, mit Symbol und der Taste U; genau einer"), [] {
            const ui::RolloutProbe* g = gruppe(Str::UvGrAbwickeln);
            const Element* haupt = imRoll(tr(Str::UvAbwickeln));
            const Element* schnell = imRoll(tr(Str::UvGrSchnell));
            if (g == nullptr || haupt == nullptr || schnell == nullptr) { return false; }
            melde("U4 Hauptknopf: " + mbefoto::rechteck("abwickeln", haupt->rect) + " " + mbefoto::rechteck("schnell", schnell->rect) + ", Knoepfe " + std::to_string(g->knoepfe) + ", Hauptknoepfe " +
                  std::to_string(g->hauptKnoepfe) + ", Symbole " + std::to_string(g->symbole) + ", Tasten " + std::to_string(g->tasten));
            return g->hauptKnoepfe == 1 && g->knoepfe == 3 && g->symbole == 3 && g->tasten == 1 && haupt->rect.GetWidth() >= g->kasten.GetWidth() - em() * 1.2F &&
                   haupt->rect.GetHeight() > schnell->rect.GetHeight() && haupt->rect.Max.y <= schnell->rect.Min.y && g->kasten.Contains(haupt->rect);
        }));
        add(pruefe(zeile("U4", modus, "Abwickeln: darunter Schnell abwickeln und Projizieren im Raster, die Vorlage (Figur, Requisit, harte Flaeche) und die Projektion (eben, Zylinder, Kugel, Kasten) als Segmente"), [] {
            const ui::RolloutProbe* g = gruppe(Str::UvGrAbwickeln);
            const Element* schnell = imRoll(tr(Str::UvGrSchnell));
            const Element* proj = imRoll(tr(Str::A7Projizieren));
            if (g == nullptr || schnell == nullptr || proj == nullptr) { return false; }
            bool ok = g->rasterSpalten == 2 && std::fabs(schnell->rect.GetWidth() - proj->rect.GetWidth()) < 0.5F && schnell->rect.Min.y == proj->rect.Min.y && proj->rect.Min.x > schnell->rect.Max.x;
            float y = proj->rect.Max.y;
            float x = -1.0F;
            for (const Str e : {Str::UvSchnellFigur, Str::UvSchnellRequisit, Str::UvSchnellHart}) {
                const Element* z = imRoll(tr(e));
                ok = ok && z != nullptr && g->kasten.Contains(z->rect) && (z->flags & ImGuiItemStatusFlags_Checkable) != 0 && z->rect.Min.y >= y - 1.0F;
                if (z != nullptr) { x = std::max(x, z->rect.Max.y); }
            }
            y = x;
            // die Reihenfolge des Entwurfs: eben, Zylinder, Kugel, Kasten
            const Element* p[4] = {imRoll(tr(Str::UvProjEben)), imRoll(tr(Str::UvProjZylinder)), imRoll(tr(Str::UvProjKugel)), imRoll(tr(Str::UvProjKasten))};
            for (int i = 0; i < 4; ++i) {
                ok = ok && p[i] != nullptr && g->kasten.Contains(p[i]->rect) && (p[i]->flags & ImGuiItemStatusFlags_Checkable) != 0 && p[i]->rect.Min.y >= y - 1.0F;
                if (i > 0 && ok) { ok = p[i]->rect.Min.y > p[i - 1]->rect.Min.y || p[i]->rect.Min.x > p[i - 1]->rect.Min.x; }
            }
            return ok && p[0] != nullptr && (p[0]->flags & ImGuiItemStatusFlags_Checked) != 0;
        }));
        add(pruefe(zeile("U4", modus, "Abwickeln: Entspannen steht nicht mehr hier, sondern im Schritt Aufraeumen"), [] {
            const ui::RolloutProbe* ab = gruppe(Str::UvGrAbwickeln);
            const ui::RolloutProbe* au = gruppe(Str::A7RAufraeumen);
            const Element* e = imRoll(tr(Str::UvEntspannen));
            return ab != nullptr && au != nullptr && e != nullptr && au->kasten.Contains(e->rect) && !ab->kasten.Contains(e->rect);
        }));
        add(tu("AP7: halber Torso gewaehlt (Flaechen)", [] { uv1b::halbeFlaecheWaehlen("torso"); }));
        add(pause(0.3));
        // Die Gruppe heisst wie der Knopf ("Unwrap###uvrabw"); klick() faende auch ihre Ueberschrift - imRoll vergleicht die ganze
        // Beschriftung und trifft nur den Hauptknopf.
        add(klickAn("AP7: den Hauptknopf Abwickeln", [] {
            const Element* e = imRoll(tr(Str::UvAbwickeln));
            return e != nullptr ? e->rect.GetCenter() : ImVec2{-1.0F, -1.0F};
        }));
        add(pause(0.6));
        add(pruefe(zeile("U4", modus, "Abwickeln: ein Klick auf den Hauptknopf wickelt die gewaehlten Flaechen ab (das Modell aendert sich)"), [anfang] {
            return g_app->mtab.uv.letzterBefehl == tr(Str::UvAbwickeln) && uv1b::glmJetzt() != *anfang;
        }));
        add(zurueckAuf(anfang));
        add(pause(0.3));
        add(klick(tr(Str::UvProjZylinder), "mtrollouts"));
        add(pause(0.3));
        add(klick(tr(Str::A7Projizieren), "mtrollouts"));
        add(pause(0.6));
        add(pruefe(zeile("U4", modus, "Abwickeln: Zylinder im Segment gewaehlt - Projizieren projiziert auf den Zylinder"), [] {
            const Element* z = imRoll(tr(Str::UvProjZylinder));
            return modellUvFeld().projektion == 1 && z != nullptr && (z->flags & ImGuiItemStatusFlags_Checked) != 0 && g_app->mtab.uv.letzterBefehl == tr(Str::UvProjZylinder);
        }));
        add(zurueckAuf(anfang));
        add(pause(0.3));
        add(klick(tr(Str::UvSchnellRequisit), "mtrollouts"));
        add(pause(0.3));
        add(klick(tr(Str::UvGrSchnell), "mtrollouts"));
        add(pause(1.0));
        add(pruefe(zeile("U4", modus, "Abwickeln: Requisit im Segment gewaehlt - Schnell abwickeln nimmt die Vorlage Requisit"), [] {
            return modellUvFeld().schnell == 1 && modellUvSchnellVorlage() == 1 && g_app->mtab.uv.letzterBefehl == tr(Str::UvSchnellRequisit);
        }));
        add(zurueckAuf(anfang));
        add(pause(0.3));

        // --- U5: 3 Aufraeumen ---
        add(rolle(Str::A7RAufraeumen));
        add(pause(0.3));
        add(pruefe(zeile("U5", modus, bl ? "Aufraeumen: Entspannen und Dichte angleichen (Strg+A) im Raster, darunter Packen (Strg+P) ueber die volle Breite, jeder mit Symbol"
                                        : "Aufraeumen: Entspannen (Umschalt+U) und Dichte angleichen (D) im Raster, darunter Packen (Umschalt+P) ueber die volle Breite, jeder mit Symbol"),
                   [=] {
                       const ui::RolloutProbe* g = gruppe(Str::A7RAufraeumen);
                       const Element* entspannen = imRoll(tr(Str::UvEntspannen));
                       const Element* dichte = imRoll(tr(Str::UvDichte));
                       const Element* packen = imRoll(tr(Str::UvPacken));
                       if (g == nullptr || entspannen == nullptr || dichte == nullptr || packen == nullptr) { return false; }
                       melde("U5 Knoepfe: " + mbefoto::rechteck("entspannen", entspannen->rect) + " " + mbefoto::rechteck("dichte", dichte->rect) + " " + mbefoto::rechteck("packen", packen->rect) + ", Knoepfe " +
                             std::to_string(g->knoepfe) + ", Symbole " + std::to_string(g->symbole) + ", Tasten " + std::to_string(g->tasten));
                       return g->knoepfe == 3 && g->symbole == 3 && g->tasten == (bl ? 2 : 3) && g->hauptKnoepfe == 0 && std::fabs(entspannen->rect.GetWidth() - dichte->rect.GetWidth()) < 0.5F &&
                              entspannen->rect.Min.y == dichte->rect.Min.y && dichte->rect.Min.x > entspannen->rect.Max.x && packen->rect.Min.y >= entspannen->rect.Max.y &&
                              packen->rect.GetWidth() >= g->kasten.GetWidth() - em() * 1.2F && g->kasten.Contains(packen->rect);
                   }));
        add(pruefe(zeile("U5", modus, "Aufraeumen: der Abstand als Regler (Beschriftung, Schieber, Wert), darunter Drehen mit dem Haken erlaubt"), [] {
            const ui::RolloutProbe* g = gruppe(Str::A7RAufraeumen);
            const Element* abstand = imRoll(tr(Str::A7Abstand));
            const Element* erlaubt = imRoll(tr(Str::A7Erlaubt));
            const Element* packen = imRoll(tr(Str::UvPacken));
            if (g == nullptr || abstand == nullptr || erlaubt == nullptr || packen == nullptr) { return false; }
            melde("U5 Regler: " + mbefoto::rechteck("abstand", abstand->rect) + " " + mbefoto::rechteck("erlaubt", erlaubt->rect) + ", Beschriftung bis " + std::to_string(g->beschriftungRechts) + ", Feld ab " +
                  std::to_string(g->feldLinks));
            return g->kasten.Contains(abstand->rect) && g->kasten.Contains(erlaubt->rect) && abstand->rect.Min.y >= packen->rect.Max.y && erlaubt->rect.Min.y >= abstand->rect.Max.y - 1.0F &&
                   erlaubt->rect.Min.x > g->kasten.Min.x + em() * 2.0F && (erlaubt->flags & ImGuiItemStatusFlags_Checked) != 0;
        }));
        add(klick(tr(Str::A7Erlaubt), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("U5", modus, "Aufraeumen: der Haken erlaubt schaltet das Drehen beim Packen aus"), [] { return !g_app->mtab.uv.drehen; }));
        add(klick(tr(Str::A7Erlaubt), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("U5", modus, "Aufraeumen: ein zweiter Klick schaltet es wieder ein"), [] { return g_app->mtab.uv.drehen; }));
        add(klick(kopf(Str::A7Mehr, "uvmehrauf"), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("U5", modus, "Aufraeumen: unter Weitere stehen Begradigen, Rechteck, Gespiegelt stapeln, Ausrichten, Sperren und der Umfang des Packens"), [] {
            const ui::RolloutProbe* g = gruppe(Str::A7RAufraeumen);
            if (g == nullptr) { return false; }
            for (const Str x : {Str::UvBegradigen, Str::UvRechteck, Str::UvStapeln, Str::UvAusrichten, Str::UvSperren, Str::UvPackAuto, Str::UvPackAuswahl, Str::UvPackAlles}) {
                const Element* e = imRoll(tr(x));
                if (e == nullptr || !g->kasten.Contains(e->rect)) {
                    melde(std::string("U5 Weitere: fehlt oder liegt ausserhalb der Gruppe: ") + tr(x));
                    return false;
                }
            }
            return true;
        }));
        add(klick(kopf(Str::A7Mehr, "uvmehrauf"), "mtrollouts"));
        add(pause(0.2));

        // --- U6: Pruefen ---
        add(rolle(Str::UvGrPruefen));
        add(pause(0.3));
        add(pruefe(zeile("U6", modus, "Pruefen: die Kurzinfo sagt gruen \"alles gut\" oder gelb die Zahl der Befunde (ueberlappt, umgeklappt, gedehnt)"), [] {
            const ui::RolloutProbe* g = gruppe(Str::UvGrPruefen);
            if (g == nullptr) { return false; }
            const w::UvBefunde& b = g_app->mtab.uv.befunde;
            const int n = static_cast<int>(b.ueberlappt.size() + b.gekippt.size() + b.gedehnt.size());
            char erwartet[96];
            if (n == 0) {
                std::snprintf(erwartet, sizeof erwartet, "%s", tr(Str::MfInfoGut));
            } else {
                std::snprintf(erwartet, sizeof erwartet, trN(Str::MfInfoFehler, n), n);
            }
            melde("U6 Kurzinfo: \"" + g->kurzinfo + "\", Befunde " + std::to_string(n));
            return g->kurzinfo == erwartet && g->info.GetWidth() > 0.0F;
        }));
        add(pruefe(zeile("U6", modus, "Pruefen: die Schalter Schachbrett, Dehnung zeigen, Ueberlappungen zeigen, Naehte dick als Liste in zwei Spalten"), [] {
            const ui::RolloutProbe* g = gruppe(Str::UvGrPruefen);
            const Element* e[4] = {imRoll(tr(Str::UvSchachbrett)), imRoll(tr(Str::UvDehnungZeigen)), imRoll(tr(Str::UvUeberlappZeigen)), imRoll(tr(Str::A7NahtDick))};
            if (g == nullptr || e[0] == nullptr || e[1] == nullptr || e[2] == nullptr || e[3] == nullptr) { return false; }
            melde("U6 Schalter: " + mbefoto::rechteck("schachbrett", e[0]->rect) + " " + mbefoto::rechteck("dehnung", e[1]->rect) + " " + mbefoto::rechteck("ueberlappt", e[2]->rect) + " " +
                  mbefoto::rechteck("naehte", e[3]->rect));
            bool ok = true;
            for (int i = 0; i < 4; ++i) { ok = ok && g->kasten.Contains(e[i]->rect) && (e[i]->flags & ImGuiItemStatusFlags_Checkable) != 0; }
            return ok && e[0]->rect.Min.y == e[1]->rect.Min.y && e[1]->rect.Min.x > e[0]->rect.Max.x - 1.0F && e[2]->rect.Min.y >= e[0]->rect.Max.y - 1.0F && e[2]->rect.Min.y == e[3]->rect.Min.y &&
                   std::fabs(e[2]->rect.Min.x - e[0]->rect.Min.x) < 0.5F && (e[0]->flags & ImGuiItemStatusFlags_Checked) == 0 && (e[2]->flags & ImGuiItemStatusFlags_Checked) != 0;
        }));
        add(klick(tr(Str::A7NahtDick), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("U6", modus, "Pruefen: Naehte dick zeichnet die Naehte im UV-Editor mit mindestens doppelter Strichstaerke"), [] {
            melde("U6 Naehte dick: " + std::to_string(modellUvNahtDicke()) + " Bildpunkte bei Schrift " + std::to_string(em()));
            return modellUvFeld().nahtDick && modellUvNahtDicke() >= 4.0F;
        }));
        add(klick(tr(Str::A7NahtDick), "mtrollouts"));
        add(pause(0.2));
        add(klick(tr(Str::UvSchachbrett), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("U6", modus, "Pruefen: der Schalter Schachbrett legt das Schachbrett unter die UV (wie der Knopf im UV-Editor)"), [] {
            const Element* e = imRoll(tr(Str::UvSchachbrett));
            return !modellUvFeld().nahtDick && modellUvNahtDicke() == 2.0F && g_app->mtab.uv.hintergrund == 1 && e != nullptr && (e->flags & ImGuiItemStatusFlags_Checked) != 0;
        }));
        add(klick(tr(Str::UvSchachbrett), "mtrollouts"));
        add(pause(0.2));
        add(pruefe(zeile("U6", modus, "Pruefen: die Werte als Tabelle - Dehnung, Ueberlappungen, Ausnutzung der Textur, Texel-Dichte - je eine Zeile ueber die Breite der Gruppe"), [] {
            const ui::RolloutProbe* g = gruppe(Str::UvGrPruefen);
            const Element* naehte = imRoll(tr(Str::A7NahtDick));
            if (g == nullptr || naehte == nullptr) { return false; }
            float y = naehte->rect.Max.y;
            for (const Str z : {Str::A7Dehnung, Str::A7Ueberlappungen, Str::A7Ausnutzung, Str::A7Dichte}) {
                const Element* e = imRoll(tr(z));
                if (e == nullptr || !g->kasten.Contains(e->rect) || e->rect.Min.y < y - 1.0F || e->rect.GetWidth() < g->kasten.GetWidth() - em() * 1.2F) {
                    melde(std::string("U6 Zeile fehlt oder liegt falsch: ") + tr(z));
                    return false;
                }
                y = e->rect.Max.y;
            }
            return true;
        }));
        add(pruefe(zeile("U6", modus, "Pruefen: die Ausnutzung der Textur ist gemessen (Anteil der bedeckten Texel der Kachel, ueber 0 und hoechstens 100 %)"), [] {
            const float a = g_app->mtab.uv.befunde.ausnutzung;
            melde("U6 Ausnutzung: " + std::to_string(a * 100.0F) + " %");
            return a > 0.0F && a <= 1.0F;
        }));

        // --- U7: Tipp am Ende ---
        add(pruefe(zeile("U7", modus, "am Ende des Felds steht der Tipp zum einen UV-Satz von JKA als Kasten ueber die Breite, unter der Gruppe Pruefen"), [] {
            const ui::RolloutProbe* g = gruppe(Str::UvGrPruefen);
            const Element* tipp = imRoll(tr(Str::A7TippSatz));
            if (g == nullptr || tipp == nullptr) { return false; }
            return tipp->rect.Min.y >= g->kasten.Max.y && tipp->rect.GetWidth() >= g->kasten.GetWidth() - em() * 1.2F;
        }));

        // --- U8 / U9: Tasten ---
        if (!bl) {
            add(pruefe(zeile("U8", modus, "Tasten wie die Karte von 3ds Max: 4 = Flaeche (3 ist dort der Rand), 5 = Insel - so steht es im Segment; Alles waehlen Strg+A"), [] {
                const int alles = uv1b::aktion(Str::A7AllesWaehlen);
                return imRoll(std::string("4 ") + tr(Str::UvEbeneFlaeche)) != nullptr && imRoll(std::string("5 ") + tr(Str::UvEbeneInsel)) != nullptr && alles >= 0 &&
                       std::string(modellUvAktionTaste(alles, false)) == "Ctrl+A";
            }));
        } else {
            add(pruefe(zeile("U9", modus, "Tasten wie Blender in der Aktionstabelle: L Insel waehlen, A Alles waehlen, Strg+A Dichte angleichen, Strg+P Packen"), [] {
                struct Soll {
                    Str s;
                    const char* taste;
                };
                for (const Soll& x : {Soll{Str::A7InselWaehlen, "L"}, Soll{Str::A7AllesWaehlen, "A"}, Soll{Str::UvDichte, "Ctrl+A"}, Soll{Str::UvPacken, "Ctrl+P"}}) {
                    const int i = uv1b::aktion(x.s);
                    if (i < 0 || std::string(modellUvAktionTaste(i, true)) != x.taste) {
                        melde(std::string("U9 Taste fehlt: ") + x.taste + " " + tr(x.s));
                        return false;
                    }
                }
                return true;
            }));
        }
        add(tu("AP7: nichts gewaehlt", [] {
            g_app->mtab.eckenWahl.clear();
            g_app->mtab.auswahl.clear();
            g_app->modelDirty = true;
        }));
        add(ueberEditor());
        add(pause(0.3));
        add(tasteKombi(bl ? ImGuiKeyChord{ImGuiKey_A} : ImGuiKeyChord{ImGuiMod_Ctrl | ImGuiKey_A}, bl ? "A" : "Strg+A"));
        add(pause(0.3));
        add(pruefe(zeile(bl ? "U9" : "U8", modus, bl ? "A ueber dem UV-Editor waehlt alles" : "Strg+A ueber dem UV-Editor waehlt alles"), [] {
            const EckMenge alle = alleEcken();
            return !alle.empty() && wahlJetzt() == alle && g_app->mtab.letzteTaste == tr(Str::A7AllesWaehlen);
        }));
        if (bl) {
            auto ziel = std::make_shared<int>(-1);
            add(tu("AP7: nichts gewaehlt, Maus auf eine Insel im UV-Editor", [ziel] {
                g_app->mtab.eckenWahl.clear();
                g_app->mtab.auswahl.clear();
                g_app->mtab.fokus = true;
                ImVec2 ort;
                if (!freieInselStelle(ort, *ziel)) {
                    meldeFehler("AP7 U9: keine freie Stelle einer Insel im UV-Editor gefunden");
                    return;
                }
                setzeMaus(ort);
            }));
            add(pause(0.3));
            add(taste(ImGuiKey_L, "L"));
            add(pause(0.3));
            add(pruefe(zeile("U9", modus, "L ueber einer Insel im UV-Editor waehlt diese Insel (Blender: Select Linked Pick)"), [ziel] {
                const EckMenge solche = eckenVonInsel(*ziel);
                melde("U9 L: Insel " + std::to_string(*ziel) + ", " + std::to_string(wahlJetzt().size()) + " Ecken gewaehlt, die Insel hat " + std::to_string(solche.size()));
                return !solche.empty() && wahlJetzt() == solche;
            }));
            add(ueberEditor());
            add(pause(0.2));
            add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_A, "Strg+A"));
            add(pause(0.6));
            add(pruefe(zeile("U9", modus, "Strg+A ueber dem UV-Editor gleicht die Dichte an"), [] { return g_app->mtab.letzteTaste == tr(Str::UvDichte) && g_app->mtab.uv.letzterBefehl == tr(Str::UvDichte); }));
            add(zurueckAuf(anfang));
            add(ueberEditor());
            add(pause(0.2));
            add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_P, "Strg+P"));
            add(pause(0.8));
            add(pruefe(zeile("U9", modus, "Strg+P ueber dem UV-Editor packt"), [] { return g_app->mtab.letzteTaste == tr(Str::UvPacken) && g_app->mtab.uv.letzterBefehl == tr(Str::UvPacken); }));
            add(zurueckAuf(anfang));
        }

        // --- U10: kurze Zeitleiste (AP1) ---
        add(pruefe(zeile("U10", modus, "im Bereich UV steht die kurze Zeitleiste"), [] { return modellZeitFassung() == ZeitFassung::Kurz; }));

        // --- Nichts ging verloren: jeder Befehl von rc611 steht im Feld (alles aufgeklappt) ---
        add(mbefoto::gruppenAuf("mtrollouts"));
        add(pause(0.5));
        add(pruefe(zeile("alle", modus, "nichts ging verloren: jeder Befehl des Felds von rc611 steht (mit Weitere aufgeklappt) im Feld"), [] {
            bool ok = true;
            for (const Str x : {Str::UvEditorZeigen,   Str::UvNahtMarkieren, Str::UvNahtLoeschen,   Str::UvNaehteSkelett, Str::UvNaehteForm,     Str::UvNaehteWinkel,   Str::UvRingSchneiden,
                                Str::UvSpiegeln,       Str::UvAbwickeln,     Str::UvNeuLoesen,      Str::UvMethodeLscm,   Str::UvMethodeDehnung, Str::UvProjEben,       Str::UvProjKasten,
                                Str::UvProjZylinder,   Str::UvProjKugel,     Str::UvProjPassend,    Str::UvEntspannen,    Str::UvBegradigen,     Str::UvRechteck,       Str::UvPin,
                                Str::UvPinsLoesen,     Str::UvStapeln,       Str::UvDichte,         Str::UvAusrichten,    Str::UvSperren,        Str::UvPacken,         Str::UvPackAuto,
                                Str::UvPackAuswahl,    Str::UvPackAlles,     Str::UvSchnellFigur,   Str::UvSchnellRequisit, Str::UvSchnellHart,  Str::UvDehnungZeigen,  Str::UvUeberlappZeigen}) {
                if (imRoll(tr(x)) == nullptr) {
                    melde(std::string("fehlt im Feld: ") + tr(x));
                    ok = false;
                }
            }
            for (const char* achse : {"X", "Y", "Z"}) { ok = ok && imRoll(achse) != nullptr; }
            ImGuiWindow* f = kindFenster("mtrollouts");
            return ok && f != nullptr && findeKennung(f->GetID("##uvgruppe")) != nullptr && imRoll(tr(Str::A7Abstand)) != nullptr && imRoll(tr(Str::MdWinkel)) != nullptr;
        }));
        add(pruefe(zeile("alle", modus, "jede UV-Aktion hat einen Tooltip-Text; die neuen stehen in der Aktionstabelle (Menue UV, Rechtsklick, Alle Aktionen suchen)"), [] {
            for (const Str x : {Str::A7InselWaehlen, Str::A7AllesWaehlen, Str::A7NahtDick}) {
                const int i = uv1b::aktion(x);
                if (i < 0 || std::string(modellUvAktionHinweis(i)).size() < 10U) { return false; }
            }
            for (int i = 0; i < modellUvAktionZahl(); ++i) {
                if (std::string(modellUvAktionHinweis(i)).size() < 10U) { return false; }
            }
            return true;
        }));
        add(tu("AP7: Feld nach oben", [] {
            if (ImGuiWindow* f = kindFenster("mtrollouts")) { ImGui::SetScrollY(f, 0.0F); }
        }));
        add(pause(0.3));
        add(rolle(Str::UvGrAbwickeln));
        add(pause(0.3));
        add(mbefoto::mausAuf("den Hauptknopf Abwickeln", [] { return imRoll(tr(Str::UvAbwickeln)); }));
        add(pruefe(zeile("U4", modus, "der Hauptknopf zeigt nach der ueblichen Verzoegerung seinen Tooltip"), [] {
            for (const ImGuiWindow* f : GImGui->Windows) {
                if ((f->Flags & ImGuiWindowFlags_Tooltip) != 0 && f->Active && !f->Hidden) { return imRoll(tr(Str::UvAbwickeln)) != nullptr; }
            }
            return false;
        }));
        add(tu("AP7: Maus in die Ansicht, Feld nach oben, Weitere zu", [] {
            setzeMaus(mbefoto::ansichtEcke());
            if (ImGuiWindow* f = kindFenster("mtrollouts")) {
                ImGui::SetScrollY(f, 0.0F);
                for (const char* k : {"uvmehrnaht", "uvmehrabw", "uvmehrauf", "uvmehrpruef"}) { f->StateStorage.SetInt(f->GetID(kopf(Str::A7Mehr, k).c_str()), 0); }
            }
        }));
        add(pause(0.3));
    }
}

// --- Fotos: der Bereich UV in beiden Bedienmodi, das rechte Feld seitenweise ----------------------------------------

void fotoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const std::string p = modus == 0 ? "ap7_max_uv" : "ap7_blender_uv";
        add(tu("AP7 Foto: " + p, [=] {
            modellBedienungSetzen(modus);
            ModellTab& t = g_app->mtab;
            t.bereich = ModellBereich::Uv;
            g_app->settings.modellFeldBreite = 0.0F;
            t.lodWahl = 1;
            t.eckenWahl.clear();
            t.auswahl = {mb1::teil("torso")};
            t.uv.offen = true;
            t.uv.gruppe = -1;
            modellUvFeld() = ModellUvFeld{};
            const int flaeche = uv1b::aktion(Str::UvEbeneFlaeche);
            if (flaeche >= 0) { modellUvAktionAusloesen(flaeche); }
            if (ImGuiWindow* f = kindFenster("mtrollouts")) {
                ImGui::SetScrollY(f, 0.0F);
                for (const char* k : {"uvmehrnaht", "uvmehrabw", "uvmehrauf", "uvmehrpruef"}) { f->StateStorage.SetInt(f->GetID(kopf(Str::A7Mehr, k).c_str()), 0); }
            }
            g_app->modelDirty = true;
        }));
        add(pause(1.0));
        add(tu("AP7 Foto: Maus in die Ansicht", [] { setzeMaus(mbefoto::ansichtEcke()); }));
        add(pause(0.5));
        add(foto(p));
        for (int k = 0; k < 6; ++k) { add(mbefoto::seitenFoto(p + "_feld" + std::to_string(k), "mtrollouts", k)); }
        add(mbefoto::gruppenAuf("mtrollouts"));
        add(pause(0.5));
        for (int k = 0; k < 8; ++k) { add(mbefoto::seitenFoto(p + "_auf" + std::to_string(k), "mtrollouts", k)); }
        add(tu("AP7 Foto: Feld nach oben, Weitere zu", [] {
            if (ImGuiWindow* f = kindFenster("mtrollouts")) {
                ImGui::SetScrollY(f, 0.0F);
                for (const char* k : {"uvmehrnaht", "uvmehrabw", "uvmehrauf", "uvmehrpruef"}) { f->StateStorage.SetInt(f->GetID(kopf(Str::A7Mehr, k).c_str()), 0); }
            }
        }));
        add(pause(0.3));
    }
}

// Bilder fuer das Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap7): h_ap7_max, h_ap7_blender - der Sturmtruppler der Seite
// "Model-Tab: UV erstellen", der halbe Torso gewaehlt, das Feld mit den Schritten 1 bis 3.
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP7 Handbuch: Bedienmodus merken", [=] {
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
    }));
    uv1b::figurLaden(s, "max");
    for (int modus = 0; modus < 2; ++modus) {
        add(tu(std::string("AP7 Handbuch: ") + (modus == 0 ? "wie 3ds Max" : "wie Blender") + ", Ansicht von vorn, der halbe Torso gewaehlt", [=] {
            modellBedienungSetzen(modus);
            ModellTab& t = g_app->mtab;
            t.bereich = ModellBereich::Uv;
            modellUvFeld() = ModellUvFeld{};
            ModellAnsicht& a = t.ansichten[t.aktiv];
            a.art = AnsichtArt::Vorne;
            modellAnsichtEinpassenAktiv();
            uv1b::halbeFlaecheWaehlen("torso");
            const int flaeche = uv1b::aktion(Str::UvEbeneFlaeche);
            if (flaeche >= 0) { modellUvAktionAusloesen(flaeche); }
            if (ImGuiWindow* f = kindFenster("mtrollouts")) { ImGui::SetScrollY(f, 0.0F); }
            g_app->modelDirty = true;
        }));
        add(pause(0.8));
        add(tu("AP7 Handbuch: Maus in die Ansicht (kein Tooltip im Bild)", [] { setzeMaus(kv5::fokusPunkt()); }));
        add(pause(0.8));
        add(hilfe2::schrittFoto(modus == 0 ? "h_ap7_max" : "h_ap7_blender"));
    }
    add(tu("AP7 Handbuch: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
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
    if (n != "ap7" && n != "ap7foto") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_AP7_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    const char* m = std::getenv("BHED_AP7_MODELL");
    const std::string modell = m != nullptr ? m : "models/players/kyle/model.glm";
    auto skala = std::make_shared<float>(1.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP7: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        if (const char* t = std::getenv("BHED_AP7_THEMA")) {
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
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap7.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    add(tu("AP7: Aufteilung wie neue Einstellungen, Sequenz, eine Ansicht", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        g_app->mtab.vierAnsichten = false;
        animWaehlen(mbefoto::sequenz(), false);
        char zeile[300];
        std::snprintf(zeile, sizeof zeile, "%s, %zu Teile, Anzeige %.0f x %.0f, Oberflaeche %.2f, Schrift %.0f Punkte hoch", g_app->modelPath.c_str(), g_app->mtab.dok.inhalt().teile.size(),
                      static_cast<double>(ImGui::GetIO().DisplaySize.x), static_cast<double>(ImGui::GetIO().DisplaySize.y), static_cast<double>(g_app->uiScale),
                      static_cast<double>(ImGui::GetFontSize()));
        melde(zeile);
    }));
    add(pause(0.8));
    add(klickAn("AP7: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (n == "ap7") { uvSchritte(s); }
    if (n == "ap7foto") { fotoSchritte(s); }
    add(tu("AP7: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        g_app->mtab.eckenWahl.clear();
        g_app->mtab.auswahl.clear();
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

}  // namespace ap7
