// selbsttest_ap6.inc - AP6 (MBENTWURF Teil B, 11.11.): das rechte Feld im Bereich Modellieren nach dem Entwurf
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap6 | ap6foto | ap6hb)
//
//   ap6       je Zeile der Abgleichtabelle (plaene/modellblock_stufe0/abgleich_programm_entwurf.md, Abschnitt 8) eine Pruefung
//             "AP6 M<nr>": Ort, Element, Bedienung - wie 3ds Max und wie Blender.
//               M0a/M0b  die Gruppen Auswahl, Werkzeuge, JKA-Grenzen im Reiter des Bereichs (Aendern bzw. Modifier), im Stil
//                        des Bedienmodus; was in welchem weiteren Reiter steht
//               M3/M4    Gruppe Auswahl: die Ebenen als Segment (1 Ecke ... 5 Element bzw. 1 Ecke, 2 Kante, 3 Flaeche)
//               M5       Gruppe Werkzeuge: die sechs Hauptwerkzeuge im Raster mit Symbol und Taste, der Rest darunter, Tipp;
//                        Strg+Enter uebernimmt und macht weiter
//               M6       Gruppe JKA-Grenzen: Kurzinfo, Balken der gewaehlten Flaeche, die Liste aller Flaechen eingeklappt
//               M7       die Ansicht zeichnet das Netz mit Kanten (unveraendert)
//               M8/M9    Tasten: wie 3ds Max 1-5, Strg+A; wie Blender E Extrudieren, Strg+R, K, Tab
//               M11      Grundkoerper und Requisit im Reiter Erstellen (3ds Max) bzw. Objekt und im Menue Hinzufuegen (Blender)
//             Nicht hier: M1/M2 (Modifier-Stapel, AP13), M10 (kurze Zeitleiste, AP1), vier Ansichten als Vorgabe (R7, AP5).
//   ap6foto   nur Fotos (BHED_FOTOS=<Ordner>): der Bereich Modellieren in beiden Bedienmodi, die weiteren Reiter, die
//             aufgeklappten Teile und ein Requisit mit offenem Werkzeugkaestchen (ap6_max_*, ap6_blender_*).
//   ap6hb     die zwei Bilder fuer das Handbuch (h_ap6_max, h_ap6_blender).
//   BHED_AP6_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe wie der Foto-Selbsttest mbefoto)
//   BHED_AP6_MODELL=<.glm>           Vorgabe models/players/kyle/model.glm (das Modell des Entwurfs)
//   BHED_AP6_THEMA=hell|dunkel       das erste helle bzw. dunkle Farbschema (Vorgabe: das eingestellte)
// Eingebunden in selbsttest.cpp (nach selbsttest_ap3.inc).
namespace ap6 {

namespace w = modell::wof;
using mbefoto::kindFenster;
using md1d::mz;

float em() { return ImGui::GetFontSize(); }

void melde(const std::string& z) { diag::detail("Knopftest: AP6 " + z); }

// Ein Element des rechten Felds ueber seine Beschriftung (auch nur den sichtbaren Teil vor "###").
const Element* imRoll(const std::string& label) { return finde(label, "mtrollouts", 0, false); }

bool feldFrisch() { return ui::feldProbe().bild >= ImGui::GetFrameCount() - 2; }

std::string zeile(const char* nr, int modus, const std::string& was) { return std::string("AP6 ") + nr + (modus == 0 ? " wie 3ds Max: " : " wie Blender: ") + was; }

const ui::RolloutProbe* gruppe(const std::string& titel) {
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
        if (ui::rolloutProbe(i).titel == titel) { return &ui::rolloutProbe(i); }
    }
    return nullptr;
}
const ui::RolloutProbe* gruppe(Str titel) { return gruppe(std::string(tr(titel))); }

// Die Titel der gezeichneten Gruppen des rechten Felds, mit | getrennt.
std::string gruppen() {
    std::string s;
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) { s += (i > 0 ? "|" : "") + ui::rolloutProbe(i).titel; }
    return s;
}

std::string titel(std::initializer_list<Str> liste) {
    std::string s;
    for (const Str x : liste) { s += (s.empty() ? "" : "|") + std::string(tr(x)); }
    return s;
}

// Die Beschriftung einer Ebene im Segment: Taste und Name wie im Entwurf ("1 Ecke", "4 Polygon"; Blender "3 Flaeche").
std::string seg(int taste, Str name) { return std::to_string(taste) + " " + tr(name); }

bool markiert(const Element* e) { return e != nullptr && (e->flags & ImGuiItemStatusFlags_Checked) != 0; }

// Liegt b rechts neben a in derselben Reihe und ist gleich breit?
bool nebeneinander(const Element* a, const Element* b) {
    return a != nullptr && b != nullptr && a->rect.Min.y == b->rect.Min.y && a->rect.Max.y == b->rect.Max.y && b->rect.Min.x > a->rect.Max.x &&
           std::fabs(a->rect.GetWidth() - b->rect.GetWidth()) < 0.5F;
}

// Die ersten `zahl` Vielecke einer Flaeche des Modells im Arbeitsnetz.
std::vector<int> vieleckeVon(const std::string& flaeche, int zahl) {
    modellMeshNetzAktualisieren();
    const int teil = mb1::teil(flaeche);
    std::vector<int> f;
    const w::PolyNetz& n = mz().netz;
    for (std::size_t i = 0; i < n.flaechen.size() && static_cast<int>(f.size()) < zahl; ++i) {
        if (!n.flaechen[i].weg && n.flaechen[i].teil == teil) { f.push_back(static_cast<int>(i)); }
    }
    return f;
}

// Den Deckel des Kastens (z = 64) waehlen - fehlt er, meldet es der Test und waehlt nichts (ein ungueltiges Vieleck in der
// Auswahl gibt es im Programm nicht).
void deckelWaehlen() {
    const int deckel = md1d::vieleckBei(2, 64.0F);
    if (deckel < 0) {
        meldeFehler("AP6: der Kasten hat keinen Deckel bei z = 64 (" + md1d::zahlen() + ")");
        md1d::waehle(w::PAuswahl{}, AuswahlArt::Dreieck);
        return;
    }
    md1d::waehle(w::PAuswahl{{}, {}, {deckel}}, AuswahlArt::Dreieck);
}

void rollenNachOben() {
    if (ImGuiWindow* fw = kindFenster("mtrollouts")) { ImGui::SetScrollY(fw, 0.0F); }
}

// Ein Teil "Weitere" (ui::mehrBeginn) des rechten Felds auf- oder zuklappen.
Schritt teilSetzen(Str name, bool offen) {
    return tu(std::string("AP6: ") + tr(name) + (offen ? " aufklappen" : " zuklappen"), [=] {
        const Element* e = imRoll(tr(name));
        if (e == nullptr) {
            meldeFehler(std::string("AP6: Teil nicht im rechten Feld: ") + tr(name));
            return;
        }
        if (ImGuiWindow* fw = mbefoto::fensterGenau(e->innen)) { fw->StateStorage.SetInt(e->id, offen ? 1 : 0); }
    });
}

bool teilOffen(Str name) {
    const Element* e = imRoll(tr(name));
    return e != nullptr && (e->flags & ImGuiItemStatusFlags_Openable) != 0 && (e->flags & ImGuiItemStatusFlags_Opened) != 0;
}

// --- Die Figur des Entwurfs (kyle, Flaeche torso): Form und Ort der Gruppen ---------------------------------------------

void figurSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const bool bl = modus == 1;
        add(tu(std::string("AP6: Bedienmodus ") + (bl ? "wie Blender" : "wie 3ds Max") + ", Modellieren, Polygone der Flaeche torso gewaehlt", [=] {
            modellBedienungSetzen(modus);
            g_app->mtab.bereich = ModellBereich::Modellieren;
            g_app->settings.modellFeldBreite = 0.0F;
            g_app->mtab.auswahl = {mb1::teil("torso")};
            md1d::waehle(w::PAuswahl{{}, {}, vieleckeVon("torso", 4)}, AuswahlArt::Dreieck);
            rollenNachOben();
        }));
        add(pause(0.8));
        // --- M0a / M0b: Aufbau ---
        const std::string drei = titel({Str::MdGrAuswahl, Str::Ap6Werkzeuge, Str::MdGrGrenzen});
        add(pruefe(zeile(bl ? "M0b" : "M0a", modus, bl ? "im Reiter Modifier stehen die Gruppen Auswahl, Werkzeuge, JKA-Grenzen untereinander"
                                                      : "im Reiter Aendern stehen die Gruppen Auswahl, Werkzeuge, JKA-Grenzen untereinander"), [=] {
            melde("Gruppen: " + gruppen() + ", Reiter " + std::to_string(ui::feldProbe().reiterAktiv));
            if (!feldFrisch() || gruppen() != drei || ui::feldProbe().reiterAktiv != (bl ? 2 : 1)) { return false; }
            const ui::RolloutProbe* a = gruppe(Str::MdGrAuswahl);
            const ui::RolloutProbe* b = gruppe(Str::Ap6Werkzeuge);
            const ui::RolloutProbe* c = gruppe(Str::MdGrGrenzen);
            return a->offen && b->offen && c->offen && b->kasten.Min.y >= a->kasten.Max.y && c->kasten.Min.y >= b->kasten.Max.y;
        }));
        add(pruefe(zeile(bl ? "M0b" : "M0a", modus, bl ? "die Gruppen sind abgerundete Kaesten (Stil wie Blender)" : "die Gruppen haben flache, eckige Ueberschriftsbalken (Stil wie 3ds Max)"), [=] {
            const ui::RolloutProbe* a = gruppe(Str::Ap6Werkzeuge);
            if (a == nullptr) { return false; }
            return bl ? (a->stil == ui::RolloutStil::Blender && a->rundung >= std::floor(em() * 0.3F)) : (a->stil == ui::RolloutStil::Max && a->rundung <= 2.0F);
        }));
        // die weiteren Reiter: was der Bereich dort zeigt
        struct ReiterSoll {
            Str reiter;
            std::string gruppen;
            const char* was;
        };
        const std::string erstellen = titel({Str::Ap6Grundkoerper, Str::MdGrRequisit});
        const std::vector<ReiterSoll> weitere = bl ? std::vector<ReiterSoll>{{Str::MfWerkzeug, titel({Str::MdGrAuswahl, Str::Ap6Werkzeuge}), "der Reiter Werkzeug zeigt Auswahl und Werkzeuge"},
                                                                             {Str::MfObjekt, erstellen, "der Reiter Objekt zeigt Grundkoerper und Requisit"},
                                                                             {Str::MfDaten, titel({Str::MdGrGrenzen}), "der Reiter Daten zeigt die JKA-Grenzen"}}
                                                   : std::vector<ReiterSoll>{{Str::MfErstellen, erstellen, "der Reiter Erstellen zeigt Grundkoerper und Requisit"},
                                                                             {Str::MfDienste, titel({Str::MdGrGrenzen}), "der Reiter Dienstprogramme zeigt die JKA-Grenzen"}};
        for (const ReiterSoll& r : weitere) {
            add(klick(tr(r.reiter), "mtbefehlsfeld"));
            add(pause(0.4));
            const bool erstellenReiter = r.gruppen == erstellen;
            add(pruefe(zeile(erstellenReiter ? "M11" : (bl ? "M0b" : "M0a"), modus, r.was), [=] {
                melde(std::string("Reiter ") + tr(r.reiter) + ": " + gruppen());
                return feldFrisch() && gruppen() == r.gruppen;
            }));
            if (erstellenReiter) {
                add(pruefe(zeile("M11", modus, "dort: die sieben Grundkoerper im Raster mit zwei Spalten und Symbol, darunter Name, Neues Requisit und In die Karte"), [] {
                    const ui::RolloutProbe* g = gruppe(Str::Ap6Grundkoerper);
                    const ui::RolloutProbe* req = gruppe(Str::MdGrRequisit);
                    if (g == nullptr || req == nullptr) { return false; }
                    bool alle = true;
                    for (const Str f : {Str::MdFormKasten, Str::MdFormEbene, Str::MdFormZylinder, Str::MdFormKugel, Str::MdFormKegel, Str::MdFormRohr, Str::MdFormTorus}) {
                        const Element* e = imRoll(tr(f));
                        alle = alle && e != nullptr && g->kasten.Contains(e->rect);
                    }
                    const Element* neu = imRoll(tr(Str::MdNeuesRequisit));
                    const Element* karte = imRoll(tr(Str::MdInDieKarte));
                    ImGuiWindow* fw = kindFenster("mtrollouts");
                    const Element* name = fw != nullptr ? findeKennung(fw->GetID("##mdname")) : nullptr;
                    melde("Grundkoerper: Knoepfe " + std::to_string(g->knoepfe) + ", Symbole " + std::to_string(g->symbole) + ", Spalten " + std::to_string(g->rasterSpalten));
                    return alle && g->knoepfe == 7 && g->symbole == 7 && g->rasterSpalten == 2 && nebeneinander(imRoll(tr(Str::MdFormKasten)), imRoll(tr(Str::MdFormEbene))) &&
                           neu != nullptr && karte != nullptr && name != nullptr && req->kasten.Contains(neu->rect) && req->kasten.Contains(karte->rect) && req->kasten.Contains(name->rect);
                }));
            }
        }
        add(klick(tr(bl ? Str::MfModifier : Str::MfAendern), "mtbefehlsfeld"));
        add(pause(0.4));
        add(pruefe(zeile("M11", modus, "bei einer Figur (.glm) stehen Grundkoerper und Requisit nicht im Reiter des Bereichs (dort nur die Gruppen des Entwurfs)"), [=] {
            return feldFrisch() && gruppen() == drei;
        }));
        // --- M3 / M4: Gruppe Auswahl ---
        const std::vector<std::string> ebenen = bl ? std::vector<std::string>{seg(1, Str::Ap6SegEcke), seg(2, Str::MdEbeneKante), seg(3, Str::Ap6SegFlaeche)}
                                                   : std::vector<std::string>{seg(1, Str::Ap6SegEcke), seg(2, Str::MdEbeneKante), seg(3, Str::MdEbeneRand), seg(4, Str::MdEbeneVieleck),
                                                                              seg(5, Str::MdEbeneElement)};
        const char* nr = bl ? "M4" : "M3";
        add(pruefe(zeile(nr, modus, bl ? "Gruppe Auswahl (Untertitel Edit Mode): das Segment 1 Ecke, 2 Kante, 3 Flaeche in einer Reihe ueber die ganze Breite"
                                       : "Gruppe Auswahl (Untertitel Edit Poly): das Segment 1 Ecke, 2 Kante, 3 Rand, 4 Polygon, 5 Element in einer Reihe ueber die ganze Breite"), [=] {
            const ui::RolloutProbe* g = gruppe(Str::MdGrAuswahl);
            if (g == nullptr) { return false; }
            ImRect alle{FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX};
            std::set<float> reihen;
            for (const std::string& e : ebenen) {
                const Element* x = imRoll(e);
                if (x == nullptr || !g->kasten.Contains(x->rect)) {
                    melde("Segment: fehlt oder ausserhalb der Gruppe: " + e);
                    return false;
                }
                alle.Add(x->rect);
                reihen.insert(x->rect.Min.y);
            }
            melde("Segment: " + mbefoto::rechteck("alle", alle) + " " + mbefoto::rechteck("gruppe", g->kasten) + ", Reihen " + std::to_string(reihen.size()) + ", Kurzinfo " + g->kurzinfo);
            // bei der Vorgabebreite des Felds (22 Zeichenhoehen) stehen alle Ebenen in einer Reihe (gemessen: 693 von 725 Punkten)
            return g->kurzinfo == tr(bl ? Str::MdRolloutBlender : Str::MdRolloutMax) && reihen.size() == 1U && alle.GetWidth() >= g->kasten.GetWidth() - em() * 1.2F &&
                   (bl ? imRoll(seg(3, Str::MdEbeneRand)) == nullptr && imRoll(seg(5, Str::MdEbeneElement)) == nullptr : true);
        }));
        const std::string polygon = bl ? seg(3, Str::Ap6SegFlaeche) : seg(4, Str::MdEbeneVieleck);
        add(pruefe(zeile(nr, modus, "die laufende Ebene (Polygon bzw. Flaeche) ist im Segment gewaehlt, keine andere"), [=] {
            int n = 0;
            for (const std::string& e : ebenen) { n += markiert(imRoll(e)) ? 1 : 0; }
            return markiert(imRoll(polygon)) && n == 1;
        }));
        add(klick(seg(2, Str::MdEbeneKante), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile(nr, modus, "ein Klick auf 2 Kante stellt die Ebene Kante ein"), [=] {
            return g_app->mtab.auswahlArt == AuswahlArt::Kante && markiert(imRoll(seg(2, Str::MdEbeneKante))) && !markiert(imRoll(polygon));
        }));
        add(pruefe(zeile(nr, modus, "darunter Ring, Schleife, Alles, Nichts im Raster mit zwei gleich breiten Spalten, jeder Knopf mit Symbol"), [] {
            const ui::RolloutProbe* g = gruppe(Str::MdGrAuswahl);
            const Element* ring = imRoll(tr(Str::MdRing));
            const Element* loop = imRoll(tr(Str::MdLoop));
            const Element* alles = imRoll(tr(Str::MdAlles));
            const Element* nichts = imRoll(tr(Str::MdNichts));
            if (g == nullptr || ring == nullptr || loop == nullptr || alles == nullptr || nichts == nullptr) { return false; }
            melde("Auswahl: Knoepfe " + std::to_string(g->knoepfe) + ", Symbole " + std::to_string(g->symbole) + ", Spalten " + std::to_string(g->rasterSpalten));
            return g->knoepfe == 4 && g->symbole == 4 && g->rasterSpalten == 2 && nebeneinander(ring, loop) && nebeneinander(alles, nichts) && alles->rect.Min.y > ring->rect.Max.y &&
                   g->kasten.Contains(ring->rect) && g->kasten.Contains(nichts->rect);
        }));
        add(klick(tr(Str::MdAlles), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile(nr, modus, "Alles waehlt alle Kanten; der Zaehler der Ebene steht als Wertzeile darunter (Kanten), ueber die Breite der Gruppe"), [=] {
            const ui::RolloutProbe* g = gruppe(Str::MdGrAuswahl);
            const Element* k = imRoll(tr(Str::Ap6ZKanten));
            const Element* nichts = imRoll(tr(Str::MdNichts));
            if (g == nullptr || k == nullptr || nichts == nullptr) { return false; }
            return static_cast<int>(mz().auswahl.kanten.size()) == mz().netz.kantenZahl() && mz().netz.kantenZahl() > 0 && k->rect.Min.y >= nichts->rect.Max.y && g->kasten.Contains(k->rect) &&
                   k->rect.GetWidth() >= g->kasten.GetWidth() - em() * 1.2F && imRoll(tr(Str::Ap6ZPunkte)) == nullptr && imRoll(tr(bl ? Str::Ap6ZFlaechen : Str::Ap6ZVielecke)) == nullptr;
        }));
        add(klick(tr(Str::MdNichts), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile(nr, modus, "Nichts hebt die Auswahl auf"), [] { return mz().auswahl.kanten.empty() && mz().auswahl.flaechen.empty() && mz().auswahl.punkte.empty(); }));
        // --- M8 / M9: Tasten der Ebenen ---
        add(tu("AP6: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; }));
        if (!bl) {
            static const AuswahlArt kArt[5] = {AuswahlArt::Ecke, AuswahlArt::Kante, AuswahlArt::Rand, AuswahlArt::Dreieck, AuswahlArt::Element};
            for (int k = 0; k < 5; ++k) {
                add(taste(static_cast<ImGuiKey>(ImGuiKey_1 + k), std::to_string(k + 1)));
                add(pause(0.2));
                add(pruefe(zeile("M8", modus, "Taste " + std::to_string(k + 1) + " stellt die Ebene ein, das Segment zeigt sie"), [=] {
                    return g_app->mtab.auswahlArt == kArt[k] && markiert(imRoll(ebenen[static_cast<std::size_t>(k)]));
                }));
            }
            add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_A, "Strg+A"));
            add(pause(0.2));
            add(pruefe(zeile("M8", modus, "Strg+A waehlt alles auf der Ebene (Element: alle Polygone)"), [] {
                return static_cast<int>(mz().auswahl.flaechen.size()) == mz().netz.lebendeFlaechen() && mz().netz.lebendeFlaechen() > 0;
            }));
        } else {
            add(taste(ImGuiKey_1, "1"));
            add(pause(0.2));
            add(pruefe(zeile("M9", modus, "Taste 1 stellt die Ebene Ecke ein, das Segment zeigt sie"), [=] { return g_app->mtab.auswahlArt == AuswahlArt::Ecke && markiert(imRoll(ebenen[0])); }));
            add(taste(ImGuiKey_3, "3"));
            add(pause(0.2));
            add(pruefe(zeile("M9", modus, "Taste 3 stellt die Ebene Flaeche ein"), [=] { return g_app->mtab.auswahlArt == AuswahlArt::Dreieck && markiert(imRoll(ebenen[2])); }));
            add(taste(ImGuiKey_Tab, "Tab"));
            add(pause(0.2));
            add(pruefe(zeile("M9", modus, "Tab wechselt in den Objektmodus: im Segment ist keine Ebene gewaehlt"), [=] {
                int n = 0;
                for (const std::string& e : ebenen) { n += markiert(imRoll(e)) ? 1 : 0; }
                return g_app->mtab.auswahlArt == AuswahlArt::Objekt && n == 0;
            }));
            add(taste(ImGuiKey_Tab, "Tab"));
            add(pause(0.2));
            add(pruefe(zeile("M9", modus, "Tab wechselt zurueck in die Ebene davor (Flaeche)"), [=] { return g_app->mtab.auswahlArt == AuswahlArt::Dreieck && markiert(imRoll(ebenen[2])); }));
        }
        add(tu("AP6: wieder Polygone der Flaeche torso", [] { md1d::waehle(w::PAuswahl{{}, {}, vieleckeVon("torso", 4)}, AuswahlArt::Dreieck); }));
        add(pause(0.4));
        // --- M5: Gruppe Werkzeuge ---
        add(pruefe(zeile("M5", modus, "Gruppe Werkzeuge: Extrudieren, Einfuegen | Fase, Schleifenschnitt | Messer, Verschmelzen im Raster mit zwei gleich breiten Spalten"), [] {
            const ui::RolloutProbe* g = gruppe(Str::Ap6Werkzeuge);
            const Element* a = imRoll(tr(Str::MdExtrude));
            const Element* b = imRoll(tr(Str::MdInset));
            const Element* c = imRoll(tr(Str::MdChamfer));
            const Element* d = imRoll(tr(Str::MdLoopCut));
            const Element* e = imRoll(tr(Str::MdCut));
            const Element* f = imRoll(tr(Str::MdMerge));
            if (g == nullptr || a == nullptr || b == nullptr || c == nullptr || d == nullptr || e == nullptr || f == nullptr) { return false; }
            return nebeneinander(a, b) && nebeneinander(c, d) && nebeneinander(e, f) && c->rect.Min.y > a->rect.Max.y && e->rect.Min.y > c->rect.Max.y &&
                   std::fabs(a->rect.Min.x - c->rect.Min.x) < 0.5F && std::fabs(a->rect.Min.x - e->rect.Min.x) < 0.5F && g->kasten.Contains(a->rect) && g->kasten.Contains(f->rect) &&
                   a->rect.Min.y < g->kopf.Max.y + em() * 1.5F;
        }));
        add(pruefe(zeile("M5", modus, bl ? "jeder Knopf der Gruppe hat ein Symbol; die sechs Hauptwerkzeuge zeigen ihre Taste (E, I, Strg+B, Strg+R, K, M)"
                                         : "jeder Knopf der Gruppe hat ein Symbol; Extrudieren, Fase, Messer und Abschraegen zeigen ihre Taste (Umschalt+E, Strg+Umschalt+C, Alt+C, Strg+Umschalt+B)"), [=] {
            const ui::RolloutProbe* g = gruppe(Str::Ap6Werkzeuge);
            if (g == nullptr) { return false; }
            melde("Werkzeuge: Knoepfe " + std::to_string(g->knoepfe) + ", Symbole " + std::to_string(g->symbole) + ", Tasten " + std::to_string(g->tasten) + ", Spalten " + std::to_string(g->rasterSpalten));
            // Ebene Polygon: sechs Hauptwerkzeuge + Abschraegen, Scharnier, Bruecke. Wie Blender haben genau die sechs eine Taste; wie
            // 3ds Max haben Extrudieren, Fase, Messer und Abschraegen eine (die uebrigen stehen im Menue: ihr Tooltip nennt den Weg).
            return g->knoepfe == 9 && g->symbole == g->knoepfe && g->rasterSpalten == 2 && g->tasten == (bl ? 6 : 4);
        }));
        add(pruefe(zeile("M5", modus, bl ? "der Rest darunter: die weiteren Werkzeuge der Ebene (Untertitel der Gruppe: Flaeche), Geometrie bearbeiten zugeklappt; am Ende der Tipp als Kasten"
                                         : "der Rest darunter: die weiteren Werkzeuge der Ebene (Untertitel der Gruppe: Polygone bearbeiten), Geometrie bearbeiten zugeklappt; am Ende der Tipp als Kasten"), [=] {
            const ui::RolloutProbe* g = gruppe(Str::Ap6Werkzeuge);
            const Element* merge = imRoll(tr(Str::MdMerge));
            const Element* bevel = imRoll(tr(Str::MdBevel));
            const Element* geo = imRoll(tr(Str::MdGrGeometrie));
            const Element* tipp = imRoll(tr(Str::Ap6WerkzeugTipp));
            if (g == nullptr || merge == nullptr || bevel == nullptr || geo == nullptr || tipp == nullptr) { return false; }
            return g->kurzinfo == tr(bl ? Str::Ap6SegFlaeche : Str::MdGrVielecke) && !teilOffen(Str::MdGrGeometrie) && imRoll(tr(Str::MdDetach)) == nullptr && bevel->rect.Min.y >= merge->rect.Max.y &&
                   geo->rect.Min.y >= bevel->rect.Max.y && tipp->rect.Min.y >= geo->rect.Max.y && g->kasten.Contains(tipp->rect) && tipp->rect.GetWidth() >= g->kasten.GetWidth() - em() * 1.2F;
        }));
        add(ap3::insFeld(tr(Str::MdGrGeometrie)));
        add(pause(0.3));
        add(klick(tr(Str::MdGrGeometrie), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(zeile("M5", modus, "ein Klick auf Geometrie bearbeiten klappt den Teil auf: Grundkoerper, Abtrennen, Symmetrisieren, Zu Vierecken, Kollabieren, Eben, Schneiden, Schnellschnitt, Loeschen"), [] {
            const ui::RolloutProbe* g = gruppe(Str::Ap6Werkzeuge);
            if (g == nullptr || !teilOffen(Str::MdGrGeometrie)) { return false; }
            for (const Str x : {Str::MdCreate, Str::MdDetach, Str::MdSymmetrize, Str::MdToQuads, Str::MdCollapse, Str::MdEben, Str::MdSlice, Str::MdQuickSlice, Str::MdDelete}) {
                const Element* e = imRoll(tr(x));
                if (e == nullptr || !g->kasten.Contains(e->rect)) {
                    melde(std::string("Geometrie bearbeiten: fehlt ") + tr(x));
                    return false;
                }
            }
            // "Diese LOD auf alle" nur bei einem Modell mit mehreren Detailstufen
            const bool lods = g_app->mtab.dok.inhalt().stufenZahl > 1;
            return (imRoll(tr(Str::MdLods)) != nullptr) == lods && nebeneinander(imRoll(tr(Str::MdCreate)), imRoll(tr(Str::MdDetach)));
        }));
        // was bis rc611 ein Knopf des Felds war, ist es weiter: je Ebene die Knoepfe ihres Teils
        struct EbeneSoll {
            AuswahlArt art;
            Str teil;      // Untertitel wie 3ds Max
            Str ebene;     // Untertitel wie Blender
            std::vector<Str> knoepfe;
        };
        const std::vector<EbeneSoll> jeEbene = {
            {AuswahlArt::Ecke, Str::MdGrPunkte, Str::Ap6SegEcke, {Str::MdRemove, Str::MdWeld, Str::MdTargetWeld, Str::MdConnect, Str::MdChamfer}},
            {AuswahlArt::Kante, Str::MdGrKanten, Str::MdEbeneKante, {Str::MdRemove, Str::MdRemoveSauber, Str::MdConnect, Str::MdBridge, Str::MdChamfer, Str::MdLoopCut, Str::MdExtrude}},
            {AuswahlArt::Dreieck, Str::MdGrVielecke, Str::Ap6SegFlaeche, {Str::MdBevel, Str::MdHinge, Str::MdBridge, Str::MdExtrude, Str::MdInset}},
        };
        for (const EbeneSoll& e : jeEbene) {
            add(tu("AP6: Ebene wechseln", [=] { modellAuswahlArtSetzen(e.art); }));
            add(pause(0.3));
            add(pruefe(zeile("M5", modus, std::string("Ebene ") + tr(e.teil) + ": der Untertitel der Gruppe heisst so, und sie zeigt die Werkzeuge der Ebene (nichts aus rc611 fehlt)"), [=] {
                const ui::RolloutProbe* g = gruppe(Str::Ap6Werkzeuge);
                if (g == nullptr || g->kurzinfo != tr(bl ? e.ebene : e.teil)) { return false; }
                for (const Str x : e.knoepfe) {
                    if (imRoll(tr(x)) == nullptr) {
                        melde(std::string("Ebene ") + tr(e.teil) + ": fehlt " + tr(x));
                        return false;
                    }
                }
                return true;
            }));
        }
        add(teilSetzen(Str::MdGrGeometrie, false));
        add(tu("AP6: wieder Polygone der Flaeche torso", [] { md1d::waehle(w::PAuswahl{{}, {}, vieleckeVon("torso", 4)}, AuswahlArt::Dreieck); }));
        add(pause(0.4));
        // --- M6: Gruppe JKA-Grenzen ---
        add(pruefe(zeile("M6", modus, "Gruppe JKA-Grenzen: Kurzinfo in der Ueberschrift, Balken Ecken je Flaeche, Knochen je Flaeche, Gewichte je Ecke fuer die gewaehlte Flaeche torso, darunter Flaechen im Modell"), [] {
            const ui::RolloutProbe* g = gruppe(Str::MdGrGrenzen);
            if (g == nullptr) { return false; }
            const Element* ecken = imRoll(std::string(tr(Str::MfPrEcken)) + " (torso)");
            const Element* knochen = imRoll(std::string(tr(Str::MfPrKnochen)) + " (torso)");
            const Element* gewichte = imRoll(std::string(tr(Str::MfPrGewichte)) + " (torso)");
            const Element* flaechen = imRoll(tr(Str::Ap6FlaechenImModell));
            melde("Grenzen: Kurzinfo \"" + g->kurzinfo + "\", Hoehe " + std::to_string(g->kasten.GetHeight() / em()) + " Zeichenhoehen");
            if (ecken == nullptr || knochen == nullptr || gewichte == nullptr || flaechen == nullptr) { return false; }
            return !g->kurzinfo.empty() && g->info.GetWidth() > 0.0F && g->kasten.Contains(ecken->rect) && knochen->rect.Min.y > ecken->rect.Max.y && gewichte->rect.Min.y > knochen->rect.Max.y &&
                   flaechen->rect.Min.y > gewichte->rect.Max.y && g->kasten.Contains(flaechen->rect) && ecken->rect.GetWidth() >= g->kasten.GetWidth() - em() * 1.2F;
        }));
        add(pruefe(zeile("M6", modus, "die Liste aller Flaechen ist eingeklappt: die Gruppe ist keine Bildschirmseite hoch (bis rc611 je Flaeche eine Zeile, bei kyle acht Seiten)"), [] {
            const ui::RolloutProbe* g = gruppe(Str::MdGrGrenzen);
            const modell::Inhalt& in = g_app->mtab.dok.inhalt();
            int flaechen = 0;
            for (const modell::Teil& t : in.teile) { flaechen += t.istTag() ? 0 : 1; }
            char titelAlle[96];
            std::snprintf(titelAlle, sizeof titelAlle, tr(Str::Ap6AlleFlaechen), flaechen);
            const Element* alle = imRoll(titelAlle);
            if (g == nullptr || alle == nullptr) {
                melde(std::string("Grenzen: Teil fehlt: ") + titelAlle);
                return false;
            }
            // Ueberschrift, drei Balken, eine Wertzeile, der zugeklappte Teil, die Zeile "innerhalb der Grenzen": gemessen 11 Zeichenhoehen
            return (alle->flags & ImGuiItemStatusFlags_Openable) != 0 && (alle->flags & ImGuiItemStatusFlags_Opened) == 0 && g->kasten.Contains(alle->rect) && g->kasten.GetHeight() < em() * 14.0F &&
                   imRoll("hips") == nullptr;
        }));
        add(tu("AP6 M6: Alle Flaechen aufklappen", [] {
            for (const Element& e : g_elemente) {
                if (e.innen.find("mtrollouts") == std::string::npos || (e.flags & ImGuiItemStatusFlags_Openable) == 0 || (e.flags & ImGuiItemStatusFlags_Opened) != 0) { continue; }
                if (ImGuiWindow* fw = mbefoto::fensterGenau(e.innen)) { fw->StateStorage.SetInt(e.id, 1); }
            }
        }));
        add(pause(0.4));
        add(pruefe(zeile("M6", modus, "aufgeklappt steht jede Flaeche als Zeile der Tabelle (hips, torso ...)"), [] {
            const ui::RolloutProbe* g = gruppe(Str::MdGrGrenzen);
            const Element* hips = imRoll("hips");
            const Element* torso = imRoll("torso");
            return g != nullptr && hips != nullptr && torso != nullptr && g->kasten.GetHeight() > em() * 30.0F && hips->rect.GetWidth() >= g->kasten.GetWidth() - em() * 1.2F;
        }));
        add(tu("AP6 M6: wieder zuklappen, nichts gewaehlt (Objektebene)", [] {
            for (const Element& e : g_elemente) {
                if (e.innen.find("mtrollouts") == std::string::npos || e.label.rfind(std::string(tr(Str::Ap6AlleFlaechen)).substr(0, 5), 0) != 0) { continue; }
                if (ImGuiWindow* fw = mbefoto::fensterGenau(e.innen)) { fw->StateStorage.SetInt(e.id, 0); }
            }
            for (const Element& e : g_elemente) {
                if (e.innen.find("mtrollouts") != std::string::npos && e.label == tr(Str::MdGrGeometrie)) {
                    if (ImGuiWindow* fw = mbefoto::fensterGenau(e.innen)) { fw->StateStorage.SetInt(e.id, 0); }
                }
            }
            md1d::waehle(w::PAuswahl{}, AuswahlArt::Objekt);
            g_app->mtab.auswahl.clear();
            rollenNachOben();
        }));
        add(pause(0.4));
        add(pruefe(zeile("M6", modus, "ohne Auswahl zeigen die Balken die Flaeche, die einer Grenze am naechsten kommt, und der Hinweis sagt es"), [] {
            const ui::RolloutProbe* g = gruppe(Str::MdGrGrenzen);
            if (g == nullptr) { return false; }
            const std::string anfang = std::string(tr(Str::MfPrEcken)) + " (";
            bool balken = false;
            for (const Element& e : g_elemente) { balken = balken || (e.innen.find("mtrollouts") != std::string::npos && e.label.rfind(anfang, 0) == 0); }
            return balken && imRoll(tr(Str::Ap6VolleFlaeche)) != nullptr;
        }));
        // --- M7: die Ansicht (unveraendert) ---
        add(tu("AP6 M7: Ebene Polygon, nichts gewaehlt", [] { md1d::waehle(w::PAuswahl{}, AuswahlArt::Dreieck); }));
        add(pause(0.4));
        add(klickAn("AP6 M7: Klick in die Mitte der Ansicht (auf die Figur)", [] {
            const ModellAnsicht& a = g_app->mtab.ansichten[static_cast<std::size_t>(g_app->mtab.aktiv)];
            return ImVec2{a.lo[0] + a.groesse[0] * 0.5F, a.lo[1] + a.groesse[1] * 0.5F};
        }));
        add(pause(0.4));
        add(pruefe(zeile("M7", modus, "die Ansicht zeichnet das Netz mit Kanten; ein Klick waehlt ein Polygon, seine Ecken folgen (wie bisher)"), [] {
            melde("Ansicht: " + std::to_string(mz().kantenGezeichnet) + " Kanten der Ueberlagerung, " + std::to_string(mz().auswahl.flaechen.size()) + " Polygone, " +
                  std::to_string(g_app->mtab.eckenWahl.size()) + " Ecken gewaehlt");
            return mz().kantenGezeichnet > 100 && mz().auswahl.flaechen.size() == 1U && !g_app->mtab.eckenWahl.empty();
        }));
        // --- Fund im Lauf ap6k_mb_ap7 (Absturz in transport): eine Aenderung des Dokuments setzt die animList aus der animation.cfg
        // neu (cfgAbgleich); die Zeitleiste zeichnete im selben Bild weiter mit der Sequenz aus der alten, freigegebenen Liste ---
        auto liste = std::make_shared<const AnimEntry*>(nullptr);
        auto veraltet = std::make_shared<int>(0);
        add(tu("AP6 Zeitleiste: Liste merken, Polygone der Flaeche torso extrudieren (aendert das Dokument)", [=] {
            *liste = g_app->animList.data();
            *veraltet = modellZeit().veralteteSequenz;
            md1d::waehle(w::PAuswahl{{}, {}, vieleckeVon("torso", 4)}, AuswahlArt::Dreieck);
            (void)modellMeshWerkzeugStarten(MeshWerkzeug::Extrude);
        }));
        add(pause(0.3));
        add(tu("AP6 Zeitleiste: uebernehmen", [] { modellMeshUebernehmen(false); }));
        add(pause(0.5));
        add(pruefe(zeile("Fund", modus, "nach einer Aenderung des Dokuments zeichnet die Zeitleiste mit der Sequenz der neu gelesenen animation.cfg, nie mit der alten Liste"), [=] {
            const bool cfg = !g_app->mtab.dok.inhalt().cfgPfad.empty();
            const bool neu = g_app->animList.data() != *liste;
            const int n = modellZeit().veralteteSequenz - *veraltet;
            melde("Zeitleiste: animation.cfg " + std::string(cfg ? "da" : "FEHLT") + ", Liste " + (neu ? "neu gelesen" : "NICHT neu gelesen") + ", " + std::to_string(n) +
                  " Bilder mit der alten Liste");
            return cfg && neu && n == 0;
        }));
        add(tu("AP6 Zeitleiste: Rueckgaengig", [] { (void)g_app->mtab.dok.rueckgaengig(); }));
        add(pause(0.3));
    }
}

// --- Ein Requisit (.md3): Grundkoerper, Werkzeuge bedienen, Grenze ueberschreiten, Menue Hinzufuegen --------------------

void requisitSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const bool bl = modus == 1;
        md1d::neuesRequisit(s, "ap6_kiste", "");
        add(tu(std::string("AP6 Requisit: Bedienmodus ") + (bl ? "wie Blender" : "wie 3ds Max"), [=] {
            modellBedienungSetzen(modus);
            g_app->settings.modellFeldBreite = 0.0F;
            mz().grund = w::GrundParam{};
            mz().grund.laenge = mz().grund.breite = mz().grund.hoehe = 64.0F;
            mz().grundNeuerTeil = false;
            rollenNachOben();
        }));
        add(pause(0.6));
        const std::string drei = titel({Str::MdGrAuswahl, Str::Ap6Werkzeuge, Str::MdGrGrenzen});
        add(pruefe(zeile("M11", modus, "ein leeres Requisit: im Reiter des Bereichs steht ganz oben Grundkoerper (mit dem Hinweis, damit zu beginnen) und am Ende Requisit"), [=] {
            melde("leeres Requisit: " + gruppen());
            return feldFrisch() && gruppen() == std::string(tr(Str::Ap6Grundkoerper)) + "|" + drei + "|" + tr(Str::MdGrRequisit) && imRoll(tr(Str::Ap6ErstellenLeer)) != nullptr;
        }));
        auto befehle = std::make_shared<int>(0);
        add(tu("AP6 M11: Zahl der Befehle merken", [=] { *befehle = mz().befehle; }));
        add(klick(tr(Str::MdFormKugel), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(zeile("M11", modus, "ein Klick auf Kugel oeffnet das Werkzeugkaestchen Grundkoerper mit der Form Kugel"), [] {
            return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Create && mz().grund.form == w::Grundform::Kugel;
        }));
        add(klick(tr(Str::MdFormKasten), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(zeile("M11", modus, "ein Klick auf Kasten wechselt im offenen Kaestchen die Form (kein zweiter Schritt), der Knopf ist als gewaehlt gezeichnet"), [=] {
            return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Create && mz().grund.form == w::Grundform::Kasten && mz().befehle == *befehle &&
                   markiert(imRoll(tr(Str::MdFormKasten))) && !markiert(imRoll(tr(Str::MdFormKugel)));
        }));
        add(klick(tr(Str::MdUebernehmen), "###meshkaestchen"));
        add(pause(0.4));
        // der erste Grundkoerper eines leeren Requisits und zurueck (im roten Lauf stuerzte das Programm nach einem Rueckgaengig
        // auf ein leeres Requisit ab - dort nach einer Auswahl mit ungueltigem Vieleck; hier der gueltige Weg)
        add(tu("AP6 M11: Rueckgaengig bis zum leeren Requisit", [] { (void)g_app->mtab.dok.rueckgaengig(); }));
        add(pause(0.5));
        add(pruefe(zeile("M11", modus, "Rueckgaengig nimmt den ersten Grundkoerper zurueck: das Requisit ist wieder leer, Grundkoerper steht wieder oben im Reiter des Bereichs"), [=] {
            melde("zurueck zum leeren Requisit: " + md1d::zahlen() + ", " + gruppen());
            return md1d::zahlen() == "0/0/0" && gruppen().rfind(std::string(tr(Str::Ap6Grundkoerper)) + "|", 0) == 0;
        }));
        add(tu("AP6 M11: Wiederholen", [] { (void)g_app->mtab.dok.wiederholen(); }));
        add(pause(0.5));
        add(pruefe(zeile("M11", modus, "Uebernehmen: der Kasten steht (8/12/6); danach steht Grundkoerper nicht mehr im Reiter des Bereichs, Requisit bleibt mit In die Karte als Hauptknopf"), [=] {
            const ui::RolloutProbe* r = gruppe(Str::MdGrRequisit);
            melde("Requisit mit Kasten: " + gruppen() + ", " + md1d::zahlen());
            return md1d::zahlen() == "8/12/6" && gruppen() == drei + "|" + tr(Str::MdGrRequisit) && r != nullptr && r->hauptKnoepfe == 1 && imRoll(tr(Str::MdInDieKarte)) != nullptr;
        }));
        add(tu("AP6 Requisit: Ansicht einpassen, Deckel waehlen", [] {
            modellAnsichtEinpassenAktiv();
            deckelWaehlen();
        }));
        add(pause(0.4));
        // --- M5: bedienen ---
        add(tu("AP6 M5: Zahl der Befehle merken", [=] { *befehle = mz().befehle; }));
        add(klick(tr(Str::MdExtrude), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(zeile("M5", modus, "ein Klick auf Extrudieren oeffnet das Werkzeugkaestchen mit der Vorschau"), [] { return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Extrude; }));
        add(tu("AP6 M5: Hoehe 16, Fokus im Model-Tab", [] {
            mz().betrag = 16.0F;
            mz().art = 0;
            modellMeshVorschau();
            g_app->mtab.fokus = true;
        }));
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Enter, "Strg+Enter"));
        add(pause(0.4));
        add(pruefe(zeile("M5", modus, "Strg+Enter uebernimmt und oeffnet dasselbe Werkzeug noch einmal (Uebernehmen und weiter)"), [=] {
            melde("Strg+Enter: Befehle " + std::to_string(*befehle) + " -> " + std::to_string(mz().befehle) + ", Kaestchen " + (mz().kaestchenOffen ? "offen" : "zu"));
            return mz().befehle == *befehle + 1 && mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Extrude;
        }));
        add(taste(ImGuiKey_Escape, "Esc"));
        add(pause(0.3));
        add(pruefe(zeile("M5", modus, "Esc bricht die zweite Vorschau ab; der erste Schritt bleibt (12/20/10)"), [] { return !mz().kaestchenOffen && md1d::zahlen() == "12/20/10"; }));
        add(tu("AP6 M5: Rueckgaengig, Ebene Ecke, eine Ecke gewaehlt", [] {
            (void)g_app->mtab.dok.rueckgaengig();
            modellMeshNetzAktualisieren();
            md1d::waehle(w::PAuswahl{{md1d::punktBei(32.0F, 32.0F, 64.0F)}, {}, {}}, AuswahlArt::Ecke);
        }));
        add(pause(0.4));
        add(tu("AP6 M5: Zahl der Befehle merken", [=] { *befehle = mz().befehle; }));
        add(klickAn("AP6 M5: Klick auf den gesperrten Knopf Einfuegen", [] {
            const Element* e = imRoll(tr(Str::MdInset));
            if (e == nullptr) { meldeFehler("AP6 M5: Einfuegen nicht im rechten Feld"); }
            return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
        }));
        add(pause(0.4));
        add(pruefe(zeile("M5", modus, "auf der Ebene Ecke ist Einfuegen gesperrt (es arbeitet auf Polygonen): der Klick oeffnet nichts"), [=] {
            return !mz().kaestchenOffen && mz().befehle == *befehle && md1d::zahlen() == "8/12/6";
        }));
        add(klick(tr(Str::MdChamfer), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(zeile("M5", modus, "Fase arbeitet auf der Ebene Ecke: der Klick oeffnet das Kaestchen"), [] { return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Chamfer; }));
        add(tu("AP6 M5: abbrechen", [] { modellMeshAbbrechen(); }));
        add(pause(0.3));
        if (bl) {
            // --- M9: Tasten wie Blender ---
            add(tu("AP6 M9: Deckel waehlen, Fokus", [] {
                deckelWaehlen();
                g_app->mtab.fokus = true;
            }));
            add(taste(ImGuiKey_E, "E (Blender Extrudieren)"));
            add(pause(0.3));
            add(pruefe(zeile("M9", modus, "E oeffnet Extrudieren (wie in Blender, bis rc611 nur Alt+E)"), [] { return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Extrude; }));
            add(taste(ImGuiKey_Escape, "Esc"));
            add(pause(0.3));
            add(tasteKombi(ImGuiMod_Alt | ImGuiKey_E, "Alt+E"));
            add(pause(0.3));
            add(pruefe(zeile("M9", modus, "Alt+E oeffnet Extrudieren weiterhin (in Blender das Menue Extrudieren; die Arten stehen im Kaestchen)"), [] {
                return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Extrude;
            }));
            add(taste(ImGuiKey_Escape, "Esc"));
            add(pause(0.3));
            add(pruefe(zeile("M9", modus, "die Taste von Extrudieren heisst in der Aktionstabelle E"), [] {
                for (int i = 0; i < modellMeshAktionZahl(); ++i) {
                    if (std::string(modellMeshAktionText(i)) == tr(Str::MdExtrude)) { return std::string(modellMeshAktionTaste(i, true)) == "E" && modellMeshAktionChord(i, true) == ImGuiKey_E; }
                }
                return false;
            }));
            add(tu("AP6 M9: eine Kante waehlen", [] {
                md1d::waehle(w::PAuswahl{{}, {w::kantenPaar(md1d::punktBei(32.0F, -32.0F, 0.0F), md1d::punktBei(32.0F, -32.0F, 64.0F))}, {}}, AuswahlArt::Kante);
                g_app->mtab.fokus = true;
            }));
            add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_R, "Strg+R"));
            add(pause(0.3));
            add(pruefe(zeile("M9", modus, "Strg+R oeffnet den Schleifenschnitt"), [] { return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::LoopCut; }));
            add(taste(ImGuiKey_Escape, "Esc"));
            add(pause(0.3));
            add(taste(ImGuiKey_K, "K"));
            add(pause(0.3));
            add(pruefe(zeile("M9", modus, "K beginnt das Messer; der Knopf Messer ist als gewaehlt gezeichnet"), [] { return mz().cutAn && markiert(imRoll(tr(Str::MdCut))); }));
            add(taste(ImGuiKey_Escape, "Esc"));
            add(pause(0.3));
            add(pruefe(zeile("M9", modus, "Esc beendet das Messer"), [] { return !mz().cutAn; }));
            // --- M11: Menue Hinzufuegen ---
            add(tasteKombi(ImGuiMod_Shift | ImGuiKey_A, "Umschalt+A"));
            add(pause(0.4));
            add(pruefe(zeile("M11", modus, "Umschalt+A oeffnet das Menue Hinzufuegen mit den sieben Grundkoerpern und Neues Requisit"), [] {
                int n = 0;
                for (const Str f : {Str::MdFormKasten, Str::MdFormEbene, Str::MdFormZylinder, Str::MdFormKugel, Str::MdFormKegel, Str::MdFormRohr, Str::MdFormTorus, Str::MdNeuesRequisit}) {
                    for (const Element& e : g_elemente) {
                        if (e.label == tr(f) && e.fenster.find("##Popup_") != std::string::npos) {
                            ++n;
                            break;
                        }
                    }
                }
                melde("Menue Hinzufuegen: " + std::to_string(n) + " von 8 Zeilen, Kaestchen " + (mz().kaestchenOffen ? "offen" : "zu"));
                return n == 8 && !mz().kaestchenOffen;
            }));
            add(klickAn("AP6 M11: Zylinder im Menue Hinzufuegen", [] {
                for (const Element& e : g_elemente) {
                    if (e.label == tr(Str::MdFormZylinder) && e.fenster.find("##Popup_") != std::string::npos) { return e.rect.GetCenter(); }
                }
                meldeFehler("AP6 M11: Zylinder nicht im Menue");
                return ImVec2{-50.0F, -50.0F};
            }));
            add(pause(0.4));
            add(pruefe(zeile("M11", modus, "die Zeile Zylinder oeffnet das Kaestchen Grundkoerper mit der Form Zylinder"), [] {
                return mz().kaestchenOffen && mz().werkzeug == MeshWerkzeug::Create && mz().grund.form == w::Grundform::Zylinder;
            }));
            add(tu("AP6 M11: abbrechen", [] { modellMeshAbbrechen(); }));
            add(pause(0.3));
        }
        // --- M6: eine Grenze ueberschreiten ---
        add(tu("AP6 M6: eine Kugel mit 64 x 64 Segmenten in dieselbe Flaeche (ueber 1000 Ecken)", [] {
            md1d::waehle(w::PAuswahl{}, AuswahlArt::Dreieck);
            mz().grund = w::GrundParam{};
            mz().grund.form = w::Grundform::Kugel;
            mz().grund.laenge = 32.0F;
            mz().grund.segL = 64;
            mz().grund.segH = 64;
            mz().grundNeuerTeil = false;
            (void)modellMeshWerkzeugStarten(MeshWerkzeug::Create);
            modellMeshUebernehmen(false);
        }));
        add(pause(0.5));
        add(pruefe(zeile("M6", modus, "ueber der Grenze: die Kurzinfo nennt einen Befund, der Balken der Ecken steht ueber 1000 (ein Requisit hat nur diesen Balken)"), [] {
            const ui::RolloutProbe* g = gruppe(Str::MdGrGrenzen);
            if (g == nullptr) { return false; }
            char soll[64];
            std::snprintf(soll, sizeof soll, trN(Str::MfInfoFehler, 1), 1);
            const modell::Inhalt& in = g_app->mtab.dok.inhalt();
            const int ecken = in.teile.empty() || in.teile[0].stufen.empty() ? 0 : static_cast<int>(in.teile[0].stufen[0].ecken.size());
            melde("ueber der Grenze: Kurzinfo \"" + g->kurzinfo + "\", " + std::to_string(ecken) + " Ecken");
            // eine .md3 hat kein Skelett: Knochen und Gewichte stehen dort nicht
            return g->kurzinfo == soll && ecken > 1000 && imRoll(std::string(tr(Str::MfPrEcken)) + " (ap6_kiste)") != nullptr &&
                   imRoll(std::string(tr(Str::MfPrKnochen)) + " (ap6_kiste)") == nullptr && imRoll(std::string(tr(Str::MfPrGewichte)) + " (ap6_kiste)") == nullptr;
        }));
        add(tu("AP6 M6: Rueckgaengig", [] { (void)g_app->mtab.dok.rueckgaengig(); }));
        add(pause(0.3));
        add(pruefe(zeile("M6", modus, "nach Rueckgaengig wieder innerhalb der Grenzen: die Kurzinfo meldet keinen Befund"), [] {
            const ui::RolloutProbe* g = gruppe(Str::MdGrGrenzen);
            char keiner[64];
            std::snprintf(keiner, sizeof keiner, trN(Str::MfInfoFehler, 1), 1);
            return g != nullptr && !g->kurzinfo.empty() && g->kurzinfo != keiner && md1d::zahlen() == "8/12/6";
        }));
    }
}

// --- Fotos: der Bereich Modellieren in beiden Bedienmodi ------------------------------------------------------------------

void fotoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const bool bl = modus == 1;
        const std::string p = bl ? "ap6_blender_" : "ap6_max_";
        add(tu("AP6 Foto: " + p + "modell", [=] {
            modellBedienungSetzen(modus);
            g_app->mtab.bereich = ModellBereich::Modellieren;
            g_app->settings.modellFeldBreite = 0.0F;
            g_app->mtab.auswahl = {mb1::teil("torso")};
            md1d::waehle(w::PAuswahl{{}, {}, vieleckeVon("torso", 4)}, AuswahlArt::Dreieck);
            rollenNachOben();
        }));
        add(pause(0.9));
        add(tu("AP6 Foto: Maus in die Ansicht (kein Tooltip im Bild)", [] { setzeMaus(kv5::fokusPunkt()); }));
        add(pause(0.5));
        add(foto(p + "modell"));
        add(tu("AP6 Foto: Reiter Erstellen / Objekt", [] { modellFeldReiter(MbReiter::Erstellen, MbReiterBl::Objekt); }));
        add(pause(0.5));
        add(foto(p + "erstellen"));
        add(tu("AP6 Foto: Reiter Dienstprogramme / Daten", [] { modellFeldReiter(MbReiter::Dienste, MbReiterBl::Daten); }));
        add(pause(0.5));
        add(foto(p + "grenzen"));
        add(tu("AP6 Foto: zurueck in den Reiter des Bereichs, Ebene Kante", [] {
            modellFeldReiter(MbReiter::Aendern, MbReiterBl::Modifier);
            modellAuswahlArtSetzen(AuswahlArt::Kante);
        }));
        add(pause(0.4));
        add(mbefoto::gruppenAuf("mtrollouts"));
        add(pause(0.5));
        add(foto(p + "offen"));
        add(tu("AP6 Foto: ganz nach unten gerollt", [] {
            if (ImGuiWindow* fw = kindFenster("mtrollouts")) { ImGui::SetScrollY(fw, fw->ScrollMax.y); }
        }));
        add(pause(0.5));
        add(foto(p + "offen_unten"));
        add(tu("AP6 Foto: Maus auf Einfuegen (Ebene Kante: gesperrt, der Tooltip nennt die Ebenen)", [] {
            rollenNachOben();
            const Element* e = imRoll(tr(Str::MdInset));
            setzeMaus(e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F});
        }));
        add(pause(1.5));
        add(tu("AP6 Foto: Maus bleibt auf Einfuegen", [] {
            const Element* e = imRoll(tr(Str::MdInset));
            setzeMaus(e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F});
        }));
        add(pause(1.0));
        add(foto(p + "tooltip_gesperrt"));
        add(tu("AP6 Foto: Maus auf Extrudieren (Tooltip mit Taste)", [] {
            const Element* e = imRoll(tr(Str::MdExtrude));
            setzeMaus(e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F});
        }));
        add(pause(1.5));
        add(foto(p + "tooltip"));
        add(tu("AP6 Foto: Maus weg", [] { setzeMaus(kv5::fokusPunkt()); }));
        add(tu("AP6 Foto: Teile wieder zu", [] {
            for (const Element& e : g_elemente) {
                if (e.innen.find("mtrollouts") == std::string::npos || (e.flags & ImGuiItemStatusFlags_Openable) == 0) { continue; }
                const bool gruppeAlle = e.label.rfind(std::string(tr(Str::Ap6AlleFlaechen)).substr(0, 5), 0) == 0;
                if (gruppeAlle || e.label == tr(Str::MdGrGeometrie)) {
                    if (ImGuiWindow* fw = mbefoto::fensterGenau(e.innen)) { fw->StateStorage.SetInt(e.id, 0); }
                }
            }
            rollenNachOben();
        }));
        add(pause(0.3));
    }
    // ein Requisit mit offenem Kaestchen (wie 3ds Max) und das Menue Hinzufuegen (wie Blender)
    md1d::neuesRequisit(s, "ap6_kiste", "");
    add(tu("AP6 Foto: leeres Requisit", [] {
        g_app->settings.modellFeldBreite = 0.0F;
        rollenNachOben();
    }));
    add(pause(0.6));
    add(foto("ap6_max_requisit_leer"));
    md1d::kasten(s, 64.0F);
    add(tu("AP6 Foto: Deckel gewaehlt, Extrudieren im Kaestchen", [] {
        deckelWaehlen();
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Extrude);
        mz().betrag = 16.0F;
        modellMeshVorschau();
        setzeMaus(kv5::fokusPunkt());
    }));
    add(pause(0.8));
    add(foto("ap6_max_requisit_extrude"));
    add(tu("AP6 Foto: abbrechen, wie Blender, Menue Hinzufuegen", [] {
        modellMeshAbbrechen();
        modellBedienungSetzen(1);
        g_app->mtab.fokus = true;
    }));
    add(pause(0.5));
    add(foto("ap6_blender_requisit"));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_A, "Umschalt+A"));
    add(pause(0.6));
    add(foto("ap6_blender_hinzufuegen"));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.3));
}

// Die zwei Bilder fuer das Handbuch (BHED_MODELL_NUR=ap6hb): der Bereich Modellieren wie 3ds Max und wie Blender, Figur mit den
// gewaehlten Polygonen der Flaeche torso.
void handbuchFotos(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        add(tu(std::string("AP6 Handbuch: Modellieren ") + (modus == 0 ? "wie 3ds Max" : "wie Blender"), [=] {
            modellBedienungSetzen(modus);
            g_app->mtab.bereich = ModellBereich::Modellieren;
            g_app->settings.modellFeldBreite = 0.0F;
            g_app->settings.modellAnzeigeUmzugGesehen = true;
            g_app->mtab.auswahl = {mb1::teil("torso")};
            md1d::waehle(w::PAuswahl{{}, {}, vieleckeVon("torso", 4)}, AuswahlArt::Dreieck);
            rollenNachOben();
            setzeMaus(kv5::fokusPunkt());
        }));
        add(pause(1.0));
        add(foto(modus == 0 ? "h_ap6_max" : "h_ap6_blender"));
    }
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "ap6" && n != "ap6foto" && n != "ap6hb") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    const char* fenster = std::getenv("BHED_AP6_FENSTER");
    if (fenster != nullptr) { (void)std::sscanf(fenster, "%dx%d@%f", &b, &h, &z); }
    // die Handbuchbilder in der Groesse der uebrigen (Fenster und Oberflaeche des Laufs), sonst wie der Foto-Selbsttest mbefoto
    const bool fensterSetzen = n != "ap6hb" || fenster != nullptr;
    const char* m = std::getenv("BHED_AP6_MODELL");
    const std::string modell = m != nullptr ? m : "models/players/kyle/model.glm";
    auto skala = std::make_shared<float>(1.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP6: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        if (fensterSetzen) {
            (void)platform::hauptfensterGroesse(b, h);
            g_app->uiScale = z;
        }
        if (const char* t = std::getenv("BHED_AP6_THEMA")) {
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
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap6.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    add(tu("AP6: Aufteilung wie neue Einstellungen, Sequenz, wie 3ds Max", [=] {
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
        std::snprintf(text, sizeof text, "%s, %zu Teile, %d Detailstufen, Anzeige %.0f x %.0f, Oberflaeche %.2f, Schrift %.0f Punkte hoch", g_app->modelPath.c_str(),
                      g_app->mtab.dok.inhalt().teile.size(), g_app->mtab.dok.inhalt().stufenZahl, static_cast<double>(ImGui::GetIO().DisplaySize.x),
                      static_cast<double>(ImGui::GetIO().DisplaySize.y), static_cast<double>(g_app->uiScale), static_cast<double>(ImGui::GetFontSize()));
        melde(text);
    }));
    add(pause(1.0));
    add(klickAn("AP6: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (n == "ap6") {
        figurSchritte(s);
        requisitSchritte(s);
    } else if (n == "ap6foto") {
        fotoSchritte(s);
    } else {
        handbuchFotos(s);
    }
    add(tu("AP6: Modell-Tab zurueck", [=] {
        if (mz().kaestchenOffen) { modellMeshAbbrechen(); }
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        // das Requisit des Tests ist nicht gespeichert: als ungeaendert merken (Beenden fragt sonst nach dem Modell)
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

}  // namespace ap6
