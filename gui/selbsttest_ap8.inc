// selbsttest_ap8.inc - AP8 (MBENTWURF Teil B, 11.11.): der Bereich Weights nach dem freigegebenen Entwurf
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap8 | ap8foto)
//
//   ap8       je Zeile der Abgleichtabelle (plaene/modellblock_stufe0/abgleich_programm_entwurf.md, Abschnitt 10) eine oder
//             mehrere Pruefungen "AP8 W<nr> wie 3ds Max / wie Blender": Ort (Gruppe, Reihenfolge, Reiter), Element (Hauptknopf,
//             Raster, Regler, Tabelle, Tipp) und Bedienung (der Knopf tut am Dokument, was das Vorbild tut; ein Rueckgaengig-
//             Schritt). Dazu: jede neue Funktion steht mit Tooltip in "Alle Aktionen suchen" und im Rechtsklick der Ansicht.
//   ap8foto   nur Fotos (BHED_FOTOS=<Ordner>): der Bereich Weights in beiden Bedienmodi, das rechte Feld oben, in der Mitte
//             und unten, der Spiegelmodus, das Sperren-Menue (ap8_max_*, ap8_blender_*).
//   BHED_AP8_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe wie der Foto-Selbsttest mbefoto)
//   BHED_AP8_MODELL=<.glm>           Vorgabe models/players/kyle/model.glm (das Modell des Entwurfs)
//   BHED_AP8_THEMA=hell|dunkel       das erste helle bzw. dunkle Farbschema (Vorgabe: das eingestellte)
// Eingebunden in selbsttest.cpp (nach selbsttest_ap3.inc).
namespace ap8 {

using ap3::feldFrisch;
using ap3::feldGruppe;
using ap3::feldGruppen;
using ap3::imRoll;
using ap3::tooltipDa;
using mbefoto::kindFenster;

float em() { return ImGui::GetFontSize(); }

void melde(const std::string& z) { diag::detail("Knopftest: AP8 " + z); }

std::string zeile(const char* nr, int modus, const std::string& was) { return std::string("AP8 ") + nr + (modus == 0 ? " wie 3ds Max: " : " wie Blender: ") + was; }

ModellTab& mtab() { return g_app->mtab; }
ModellGewichte& gw() { return modellGewichte(); }

const modell::Netz* netz(int teil) {
    const modell::Inhalt& in = mtab().dok.inhalt();
    if (teil < 0 || static_cast<std::size_t>(teil) >= in.teile.size() || static_cast<std::size_t>(mtab().lodJetzt) >= in.teile[static_cast<std::size_t>(teil)].stufen.size()) {
        return nullptr;
    }
    return &in.teile[static_cast<std::size_t>(teil)].stufen[static_cast<std::size_t>(mtab().lodJetzt)];
}

float gewicht(modell::EckOrt o, int knochen) {
    const modell::Netz* n = netz(o.teil);
    return n != nullptr ? modell::gewichtVon(*n, o.ecke, knochen) : -1.0F;
}

// Die Summe der Malliste einer Ecke (ohne Malliste: -1).
float summe(modell::EckOrt o) {
    const modell::Netz* n = netz(o.teil);
    if (n == nullptr || static_cast<std::size_t>(o.ecke) >= n->mal.size()) { return -1.0F; }
    float s = 0.0F;
    for (const modell::KnochenGewicht& g : n->mal[static_cast<std::size_t>(o.ecke)]) { s += g.gewicht; }
    return s;
}

int knochen(const char* name) { return boneIndex(g_app->anim, name); }

// Eine Gruppe aufklappen und ins Bild rollen.
Schritt zeige(Str titel) {
    return tu(std::string("AP8: Gruppe \"") + tr(titel) + "\" ins Bild", [titel] { ui::rolloutZeigen(tr(titel)); });
}

// Liegt das Element im Kasten der Gruppe?
bool inGruppe(Str titel, const Element* e) {
    const ui::RolloutProbe* g = feldGruppe(titel);
    return g != nullptr && e != nullptr && g->kasten.Contains(e->rect);
}

void alleZurueck() {
    int n = 0;
    while (modellRueckgaengig() && n < 400) { ++n; }
    mtab().eckenWahl.clear();
}

// Ein Paar gespiegelter Punkte des Modells: p links (blau), q rechts - beide in gezeichneten Flaechen und mit Abstand zur Mitte.
bool spiegelPaar(modell::EckOrt& links, modell::EckOrt& rechts) {
    modellGewichteSpiegelStand();
    const ModellGewichte& g = gw();
    const modell::Inhalt& in = mtab().dok.inhalt();
    const int seite = g.links != 0 ? g.links : 1;
    for (int p = 0; p < g.netz.zahl() && static_cast<std::size_t>(p) < g.partner.size(); ++p) {
        const int q = g.partner[static_cast<std::size_t>(p)];
        if (q < 0 || q == p) { continue; }
        const float x = g.netz.ort(in, mtab().lodJetzt, p)[0];
        if ((x > 0.0F ? 1 : -1) != seite || std::fabs(x) < 4.0F) { continue; }
        links = g.netz.ecken[static_cast<std::size_t>(p)].front();
        rechts = g.netz.ecken[static_cast<std::size_t>(q)].front();
        return true;
    }
    return false;
}

// Zwei benachbarte Ecken einer Flaeche, von denen aus Schleife bzw. Ring weiterlaufen: die erste Dreieckskante (von
// hoechstens 400), an der das Netz Vierecke hat. Leer: keine gefunden.
std::vector<modell::EckOrt> zweiNachbarn(int teil, bool schleife) {
    const modell::Netz* n = netz(teil);
    const modell::Inhalt& in = mtab().dok.inhalt();
    for (std::size_t k = 0; n != nullptr && k < n->kanten.size() && k < 400U; ++k) {
        const std::vector<modell::EckOrt> paar = {modell::EckOrt{teil, n->kanten[k].ecke}, modell::EckOrt{teil, n->kanten[static_cast<std::size_t>(n->kanten[k].naechste)].ecke}};
        const std::size_t zahl = (schleife ? modell::eckenSchleife(in, mtab().lodJetzt, paar) : modell::eckenRing(in, mtab().lodJetzt, paar)).size();
        if (zahl > 2U) { return paar; }
    }
    return {};
}

// Die Ecke einer Flaeche, die zur Kamera zeigt und in der aktiven Ansicht der Mitte der Flaeche am naechsten liegt (ein
// Tupfer genau dorthin trifft sie mit voller Staerke).
int vordereEcke(int teil, ImVec2& ort) {
    ImVec2 mitte;
    if (!modellFlaecheAufSchirm(teil, mitte)) { return -1; }
    ModellEckenBild bild;
    modellEckenBild(mtab().ansichten[mtab().aktiv], bild);
    if (teil < 0 || static_cast<std::size_t>(teil) >= bild.ort.size()) { return -1; }
    const auto t = static_cast<std::size_t>(teil);
    int best = -1;
    float bd = 1e30F;
    for (std::size_t k = 0; k < bild.ort[t].size(); ++k) {
        if (bild.imBild[t][k] == 0 || bild.vorn[t][k] == 0) { continue; }
        const float d = std::hypot(bild.ort[t][k][0] - mitte.x, bild.ort[t][k][1] - mitte.y);
        if (d < bd) {
            bd = d;
            best = static_cast<int>(k);
            ort = ImVec2{bild.ort[t][k][0], bild.ort[t][k][1]};
        }
    }
    return best;
}

// Flaechen der gezeichneten Stufe mit Malliste.
int malend() {
    int z = 0;
    for (const modell::Teil& teil : mtab().dok.inhalt().teile) {
        if (static_cast<std::size_t>(mtab().lodJetzt) < teil.stufen.size() && modell::malt(teil.stufen[static_cast<std::size_t>(mtab().lodJetzt)])) { ++z; }
    }
    return z;
}

struct Probe {
    modell::EckOrt a{-1, -1};
    modell::EckOrt b{-1, -1};
    float vorher = 0.0F;
    float vorherB = 0.0F;
    int stand = 0;
    int zahl = 0;
    ImVec2 ort{};
    std::vector<float> gewichte;   // je Ecke einer Flaeche das Gewicht eines Knochens
    int frame = 0;
    bool knochenAn = false;
    std::vector<ImVec2> schirm;    // je Ecke einer Flaeche der Ort im Bild vor einem Zug (leer: nicht im Bild oder abgewandt)
    std::vector<char> schirmDa;
    float laenge = 200.0F;         // die Strecke des Verlaufs (Bildpunkte)
    float startX = 0.0F;           // ihr Anfang: der ganze Bildpunkt rechts der Ecke (ImGui fuehrt die Maus auf ganzen Bildpunkten)
};

// Die Ecken einer Flaeche im Bild der aktiven Ansicht, wie sie jetzt stehen (vor einem Verlauf: von dort rechnet er).
void schirmMerken(Probe& p, int teil) {
    ModellEckenBild bild;
    modellEckenBild(mtab().ansichten[mtab().aktiv], bild);
    p.schirm.clear();
    p.schirmDa.clear();
    if (teil < 0 || static_cast<std::size_t>(teil) >= bild.ort.size()) { return; }
    const auto t = static_cast<std::size_t>(teil);
    for (std::size_t k = 0; k < bild.ort[t].size(); ++k) {
        p.schirm.push_back(ImVec2{bild.ort[t][k][0], bild.ort[t][k][1]});
        p.schirmDa.push_back(bild.imBild[t][k] != 0 && bild.vorn[t][k] != 0 ? 1 : 0);
    }
}

// Wie ziehenAn, die Strecke nach rechts steht aber erst beim Start fest (aus der Lage der Figur, Probe::laenge mal anteil).
Schritt ziehenProbe(const std::string& was, std::shared_ptr<Probe> p, float vonAnteil, float anteil) {
    auto z = std::make_shared<ImVec2>();
    auto dx = std::make_shared<float>(0.0F);
    return {"Ziehe " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    *z = ImVec2{p->startX + p->laenge * vonAnteil, std::floor(p->ort.y)};
                    *dx = p->laenge * anteil;
                    setzeMaus(*z);
                    return false;
                }
                if (b == 2) {
                    io.AddMouseButtonEvent(0, true);
                    return false;
                }
                if (b >= 3 && b <= 14) {
                    setzeMaus(ImVec2{z->x + *dx * static_cast<float>(b - 2) / 12.0F, z->y});
                    return false;
                }
                if (b == 15) {
                    io.AddMouseButtonEvent(0, false);
                    return false;
                }
                return b >= 20;
            }};
}

// Klick auf den LETZTEN Treffer einer Beschriftung im Fenster (die Suche aller Aktionen zeigt z. B. "Smear" aus Painting und
// aus Weights; die Liste des Bereichs Weights steht zuletzt).
Schritt klickLetzter(const std::string& label, const std::string& fenster) {
    return klickAn("\"" + label + "\" (letzter Treffer)", [=] {
        const Element* letzter = nullptr;
        for (int k = 0; k < 50; ++k) {
            const Element* e = finde(label, fenster, k, false);
            if (e == nullptr) { break; }
            letzter = e;
        }
        return letzter != nullptr ? letzter->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    });
}

std::vector<float> gewichteVon(int teil, int k) {
    std::vector<float> w;
    const modell::Netz* n = netz(teil);
    for (std::size_t e = 0; n != nullptr && e < n->ecken.size(); ++e) { w.push_back(modell::gewichtVon(*n, static_cast<int>(e), k)); }
    return w;
}

// Eine Ecke mit sechs Knochen, nicht normiert (fuer Bereinigen und die Grenzen).
void sechsGewichte(modell::EckOrt o) {
    const int stufe = mtab().lodJetzt;
    mtab().dok.aendern("AP8 sechs Gewichte", [&](modell::Inhalt& x) {
        modell::Netz& n = x.teile[static_cast<std::size_t>(o.teil)].stufen[static_cast<std::size_t>(stufe)];
        modell::malAnlegen(n);
        n.mal[static_cast<std::size_t>(o.ecke)] = {{1, 0.5F}, {2, 0.4F}, {3, 0.3F}, {4, 0.2F}, {5, 0.06F}, {6, 0.04F}};
    });
    g_app->modelDirty = true;
}

// --- Die Zeilen W0a bis W13 -----------------------------------------------------------------------------------------------
void zeilenSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const bool bl = modus == 1;
        auto p = std::make_shared<Probe>();
        add(tu(std::string("AP8: Bedienmodus ") + (bl ? "wie Blender" : "wie 3ds Max") + ", Weights, Knochen rhumerus, torso gewaehlt", [=] {
            alleZurueck();
            modellBedienungSetzen(modus);
            mtab().bereich = ModellBereich::Ansehen;
            g_app->modellKnochen = false;
            modellGewichteBereichSchritt();
            ModellGewichte neu;
            neu.gesperrt.assign(g_app->anim.bones.size(), 0);
            gw() = neu;
            mtab().bereich = ModellBereich::Weights;
            mtab().knochenWahl = knochen("rhumerus");
            mtab().auswahl = {mb1::teil("torso")};
            modellAuswahlArtSetzen(AuswahlArt::Objekt);
            mtab().pinselArt = static_cast<int>(modell::PinselArt::Hinzufuegen);
            mtab().pinselGewicht = 1.0F;
            mtab().pinselStaerke = 1.0F;
            mtab().pinselRadius = 40.0F;
            mtab().pinselSpiegeln = false;
            g_app->settings.modellFeldBreite = 0.0F;
            g_app->animPlaying = false;
            if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
        }));
        add(pause(0.8));

        // --- W0a / W0b: Aufbau ---
        const std::string soll = bl ? std::string(tr(Str::MgRPinsel)) + "|" + tr(Str::MgROptionen) + "|" + tr(Str::MgRBereinigen) + "|" + tr(Str::MgRSpiegeln) + "|" + tr(Str::AsGruppe) + "|" +
                                          tr(Str::MgRKnochen) + "|" + tr(Str::MgRGrenzen)
                                    : std::string(tr(Str::MgRStapel)) + "|" + tr(Str::MgRWerkzeug) + "|" + tr(Str::MgRSpiegeln) + "|" + tr(Str::AsGruppe) + "|" + tr(Str::MgRKnochen) + "|" +
                                          tr(Str::MgRGrenzen) + "|" + tr(Str::MgRErweitert);
        add(pruefe(zeile(bl ? "W0b" : "W0a", modus,
                         bl ? "Eigenschaften, Reiter Daten: Pinsel, Optionen, Bereinigen, Spiegeln, Automatisch, Knochen, JKA-Grenzen untereinander"
                            : "Befehlsfeld, Reiter Aendern: Modifier-Stapel, Gewichts-Werkzeug, Spiegeln, Automatisch, Knochen, JKA-Grenzen untereinander (dazu zugeklappt Erweiterte Parameter)"),
                   [=] {
                       const ui::FeldProbe& f = ui::feldProbe();
                       melde(std::string("W0 Gruppen: ") + feldGruppen() + " | Reiter " + std::to_string(f.reiterAktiv));
                       return feldFrisch() && feldGruppen() == soll && f.reiterAktiv == (bl ? 3 : 1) && f.stil == (bl ? ui::RolloutStil::Blender : ui::RolloutStil::Max);
                   }));
        add(pruefe(zeile(bl ? "W0b" : "W0a", modus, "die eine Gruppe \"Weights\" von rc611 gibt es nicht mehr; jede Gruppe ist ein Kasten im Stil des Bedienmodus"), [=] {
            const ui::RolloutProbe* g = feldGruppe(Str::MgRSpiegeln);
            return imRoll("Weights") == nullptr && g != nullptr && g->stil == (bl ? ui::RolloutStil::Blender : ui::RolloutStil::Max) && g->kurzinfo == tr(Str::MgInfoLR);
        }));

        if (!bl) {
            // --- W1: Modifier-Stapel ---
            add(pruefe(zeile("W1", modus, "Modifier-Stapel: Skin (gewaehlt) ueber Edit Poly"), [] {
                const Element* skin = imRoll(tr(Str::MgStapelSkin));
                const Element* poly = imRoll(tr(Str::MgStapelPoly));
                return inGruppe(Str::MgRStapel, skin) && inGruppe(Str::MgRStapel, poly) && (skin->flags & ImGuiItemStatusFlags_Checked) != 0 &&
                       (poly->flags & ImGuiItemStatusFlags_Checked) == 0 && poly->rect.Min.y >= skin->rect.Max.y;
            }));
            add(klick(tr(Str::MgStapelPoly), "mtrollouts"));
            add(pause(0.3));
            add(pruefe(zeile("W1", modus, "ein Klick auf Edit Poly wechselt in den Bereich Modellieren"), [] { return mtab().bereich == ModellBereich::Modellieren; }));
            add(tu("AP8: zurueck nach Weights", [] { mtab().bereich = ModellBereich::Weights; }));
            add(pause(0.5));

            // --- W2: Gewichts-Werkzeug ---
            add(pruefe(zeile("W2", modus, "Gewichts-Werkzeug: Hauptknopf Gewichte malen (betont, an), darunter die sieben Festwerte in einer Reihe"), [] {
                const ui::RolloutProbe* g = feldGruppe(Str::MgRWerkzeug);
                const Element* haupt = imRoll(tr(Str::MgMalenMax));
                const Element* f0 = imRoll("0");
                const Element* f6 = imRoll("1");
                if (g == nullptr || haupt == nullptr || f0 == nullptr || f6 == nullptr) { return false; }
                bool reihe = true;
                for (const char* f : {".1", ".25", ".5", ".75", ".9"}) {
                    const Element* e = imRoll(f);
                    reihe = reihe && e != nullptr && e->rect.Min.y == f0->rect.Min.y;
                }
                melde("W2 Hauptknopf " + mbefoto::rechteck("haupt", haupt->rect) + " Festwerte " + mbefoto::rechteck("0", f0->rect) + mbefoto::rechteck("1", f6->rect));
                return g->hauptKnoepfe == 1 && (haupt->flags & ImGuiItemStatusFlags_Checked) != 0 && haupt->rect.GetWidth() >= g->kasten.GetWidth() - em() * 1.2F && reihe &&
                       f0->rect.Min.y >= haupt->rect.Max.y && f6->rect.Min.y == f0->rect.Min.y && f6->rect.Max.x - f0->rect.Min.x >= g->kasten.GetWidth() - em() * 1.2F;
            }));
            add(pruefe(zeile("W2", modus, "Raster mit zwei gleich breiten Spalten: Setzen, Skalieren / Ring, Schleife / Wachsen, Schrumpfen / Mischen - jeder Knopf mit Symbol"), [] {
                const ui::RolloutProbe* g = feldGruppe(Str::MgRWerkzeug);
                const Element* a = imRoll(tr(Str::MbGwSetzen));
                const Element* b = imRoll(tr(Str::MgSkalieren));
                const Element* c = imRoll(tr(Str::MgRing));
                const Element* d = imRoll(tr(Str::MgSchleife));
                const Element* e = imRoll(tr(Str::MgWachsen));
                const Element* f = imRoll(tr(Str::MgSchrumpfen));
                const Element* m = imRoll(tr(Str::MgMischen));
                if (g == nullptr || a == nullptr || b == nullptr || c == nullptr || d == nullptr || e == nullptr || f == nullptr || m == nullptr) { return false; }
                melde("W2 Raster Spalten " + std::to_string(g->rasterSpalten) + ", Knoepfe " + std::to_string(g->knoepfe) + ", Symbole " + std::to_string(g->symbole));
                return std::fabs(a->rect.GetWidth() - b->rect.GetWidth()) < 0.5F && a->rect.Min.y == b->rect.Min.y && b->rect.Min.x > a->rect.Max.x && c->rect.Min.y > a->rect.Max.y &&
                       c->rect.Min.y == d->rect.Min.y && e->rect.Min.y > c->rect.Max.y && e->rect.Min.y == f->rect.Min.y && m->rect.Min.y > e->rect.Max.y && g->symbole == g->knoepfe &&
                       g->knoepfe >= 8;
            }));
            add(klick(".25", "mtrollouts"));
            add(pause(0.2));
            add(pruefe(zeile("W2", modus, "ein Klick auf den Festwert .25 stellt das Gewicht des Pinsels auf 0,25 (beim Malen aendert er das Modell nicht)"), [] {
                return std::fabs(mtab().pinselGewicht - 0.25F) < 1e-6F && !mtab().dok.kannRueckgaengig();
            }));
            // Malen aus: die linke Taste waehlt
            add(tu("AP8 W2: Ebene Ecke, Ort der mittleren Ecke von torso", [p] {
                modellAuswahlArtSetzen(AuswahlArt::Ecke);
                p->a = modell::EckOrt{mb1::teil("torso"), vordereEcke(mb1::teil("torso"), p->ort)};
                p->zahl = mtab().tupferZahl;
            }));
            add(klick(tr(Str::MgMalenMax), "mtrollouts"));
            add(pause(0.2));
            add(klickAn("AP8 W2: auf die Ecke (Malen aus)", [p] { return p->ort; }));
            add(pause(0.3));
            add(pruefe(zeile("W2", modus, "Hauptknopf aus: der Klick in die Ansicht malt nicht, er waehlt die Ecke"), [p] {
                const Element* haupt = imRoll(tr(Str::MgMalenMax));
                const ui::RolloutProbe* g = feldGruppe(Str::MgRWerkzeug);
                melde("W2 Auswahl " + std::to_string(mtab().eckenWahl.size()) + " Ecken, Tupfer " + std::to_string(mtab().tupferZahl - p->zahl));
                // AP14: der Knopf bleibt aus der Hauptknopf der Gruppe (KnopfOpt::umschalter: nur Rahmen betont), er ist nur nicht mehr gedrueckt
                return !gw().malen && haupt != nullptr && (haupt->flags & ImGuiItemStatusFlags_Checked) == 0 && g != nullptr && g->hauptKnoepfe == 1 && mtab().tupferZahl == p->zahl &&
                       !mtab().eckenWahl.empty() && !mtab().dok.kannRueckgaengig();
            }));
            add(tu("AP8 W2: genau diese Ecke gewaehlt, Gewicht vorher", [p] {
                mtab().eckenWahl = {p->a};
                p->vorher = gewicht(p->a, mtab().knochenWahl);
                p->stand = mtab().dok.stand();
            }));
            add(klick(".5", "mtrollouts"));
            add(pause(0.3));
            add(pruefe(zeile("W2", modus, "Malen aus, Ecke gewaehlt: der Festwert .5 setzt das Gewicht des Knochens sofort auf die Auswahl (wie das Weight Tool), Summe 1, ein Rueckgaengig-Schritt"), [p] {
                melde("W2 Festwert: " + std::to_string(p->vorher) + " -> " + std::to_string(gewicht(p->a, mtab().knochenWahl)) + ", Summe " + std::to_string(summe(p->a)));
                return gewicht(p->a, mtab().knochenWahl) == 0.5F && std::fabs(summe(p->a) - 1.0F) < 1e-5F && mtab().dok.rueckgaengigName() == tr(Str::MbUndoSetzen);
            }));
            add(klick(tr(Str::MgSkalieren), "mtrollouts"));
            add(pause(0.3));
            add(pruefe(zeile("W2", modus, "Skalieren: das Gewicht mal 0,95 (Vorgabe von Scale Weight) = 0,475"), [p] {
                return std::fabs(gewicht(p->a, mtab().knochenWahl) - 0.475F) < 1e-6F && mtab().dok.rueckgaengigName() == tr(Str::MgUndoSkalieren);
            }));
            add(klick(tr(Str::MgWachsen), "mtrollouts"));
            add(pause(0.3));
            add(pruefe(zeile("W2", modus, "Wachsen: die Auswahl nimmt die Nachbarn der Ecke dazu"), [p] {
                p->zahl = static_cast<int>(mtab().eckenWahl.size());
                melde("W2 Wachsen: " + std::to_string(p->zahl) + " Ecken");
                return p->zahl > 3;
            }));
            add(klick(tr(Str::MgMischen), "mtrollouts"));
            add(pause(0.3));
            add(pruefe(zeile("W2", modus, "Mischen: die Gewichte der Auswahl naehern sich ihren Nachbarn (die Mitte faellt unter 0,475), Summe 1"), [p] {
                const float w = gewicht(p->a, mtab().knochenWahl);
                melde("W2 Mischen: Mitte " + std::to_string(w) + ", Summe " + std::to_string(summe(p->a)));
                return w < 0.475F && w > 0.0F && std::fabs(summe(p->a) - 1.0F) < 1e-5F && mtab().dok.rueckgaengigName() == tr(Str::MgUndoMischen);
            }));
            add(klick(tr(Str::MgSchrumpfen), "mtrollouts"));
            add(pause(0.3));
            add(pruefe(zeile("W2", modus, "Schrumpfen nimmt die aeussersten Ecken wieder aus der Auswahl"), [p] { return static_cast<int>(mtab().eckenWahl.size()) < p->zahl && !mtab().eckenWahl.empty(); }));
            add(tu("AP8 W2: zwei benachbarte Ecken von torso gewaehlt (eine Kante an Vierecken)", [p] {
                mtab().eckenWahl = zweiNachbarn(mb1::teil("torso"), true);
                p->zahl = static_cast<int>(mtab().eckenWahl.size());
            }));
            add(klick(tr(Str::MgSchleife), "mtrollouts"));
            add(pause(0.4));
            add(pruefe(zeile("W2", modus, "Schleife: von zwei benachbarten Ecken aus laeuft die Auswahl die Kanten entlang weiter"), [p] {
                const int zahl = static_cast<int>(mtab().eckenWahl.size());
                melde("W2 Schleife: " + std::to_string(p->zahl) + " -> " + std::to_string(zahl) + " Ecken");
                return p->zahl == 2 && zahl > 2;
            }));
            add(tu("AP8 W2: zwei benachbarte Ecken fuer den Ring", [p] {
                mtab().eckenWahl = zweiNachbarn(mb1::teil("torso"), false);
                p->zahl = static_cast<int>(mtab().eckenWahl.size());
            }));
            add(klick(tr(Str::MgRing), "mtrollouts"));
            add(pause(0.4));
            add(pruefe(zeile("W2", modus, "Ring: die Auswahl nimmt die parallelen Kanten dazu"), [p] {
                melde("W2 Ring: " + std::to_string(p->zahl) + " -> " + std::to_string(mtab().eckenWahl.size()) + " Ecken");
                return p->zahl == 2 && mtab().eckenWahl.size() > 2U;
            }));
            add(tu("AP8 W2: ohne Auswahl", [] { mtab().eckenWahl.clear(); }));
            add(klick(tr(Str::MgSkalieren), "mtrollouts"));
            add(pause(0.3));
            add(pruefe(zeile("W2", modus, "ohne Auswahl sagt die Meldung, was fehlt (kein stilles Nichtstun)"), [] { return mtab().meldung == tr(Str::MgKeineAuswahl); }));
            add(klick(tr(Str::MgMalOptionen), "mtrollouts"));
            add(pause(0.3));
            add(pruefe(zeile("W2", modus, "was rc611 konnte, bleibt: unter Pinsel-Optionen die Pinselarten Hinzufuegen/Abziehen/Glaetten/Setzen, Radius, Staerke, Beim Malen spiegeln"), [] {
                bool da = true;
                for (const Str x : {Str::MbGwHinzu, Str::MbGwAbziehen, Str::MbGwGlaetten, Str::MbGwRadius, Str::MbGwStaerke, Str::MbGwSpiegelMalen, Str::MgVerwischen, Str::MgVerlauf}) {
                    da = da && inGruppe(Str::MgRWerkzeug, imRoll(tr(x)));
                }
                return da && inGruppe(Str::MgRWerkzeug, imRoll(tr(Str::MbGwGewicht)));
            }));
            add(klick(tr(Str::MgMalOptionen), "mtrollouts"));
            // --- W11: Tasten ---
            add(pruefe(zeile("W11", modus, "der Hinweis zur Bedienung steht als Tipp in der Gruppe (linke Taste malt, Festwerte, Strg+Z)"), [] {
                return inGruppe(Str::MgRWerkzeug, imRoll(tr(Str::MbGwTastenMax)));
            }));
            add(klick(tr(Str::MgMalenMax), "mtrollouts"));   // Malen wieder an
            add(tu("AP8: zurueck auf die Datei, Objektmodus", [] {
                alleZurueck();
                modellAuswahlArtSetzen(AuswahlArt::Objekt);
                mtab().pinselGewicht = 1.0F;
            }));
            add(pause(0.4));
        } else {
            // --- W3: Pinsel ---
            add(pruefe(zeile("W3", modus, "Pinsel: Hauptknopf Malen (betont, an), darunter die Regler Gewicht, Radius, Staerke"), [] {
                const ui::RolloutProbe* g = feldGruppe(Str::MgRPinsel);
                const Element* haupt = imRoll(tr(Str::MgMalenBl));
                const Element* a = imRoll(tr(Str::MbGwGewicht));
                const Element* b = imRoll(tr(Str::MbGwRadius));
                const Element* c = imRoll(tr(Str::MbGwStaerke));
                if (g == nullptr || haupt == nullptr || a == nullptr || b == nullptr || c == nullptr) { return false; }
                return g->hauptKnoepfe == 1 && (haupt->flags & ImGuiItemStatusFlags_Checked) != 0 && a->rect.Min.y >= haupt->rect.Max.y && b->rect.Min.y >= a->rect.Max.y &&
                       c->rect.Min.y >= b->rect.Max.y && std::fabs(a->rect.Min.x - c->rect.Min.x) < 0.5F && g->beschriftungRechts <= g->feldLinks + 0.5F;
            }));
            add(pruefe(zeile("W3", modus, "Raster: Weichzeichnen, Verlauf / Gewicht holen (mit der Taste Umschalt+X), Verwischen - jeder Knopf mit Symbol"), [] {
                const ui::RolloutProbe* g = feldGruppe(Str::MgRPinsel);
                const Element* a = imRoll(tr(Str::MgWeich));
                const Element* b = imRoll(tr(Str::MgVerlauf));
                const Element* c = imRoll(tr(Str::MrGwAufnehmen));
                const Element* d = imRoll(tr(Str::MgVerwischen));
                if (g == nullptr || a == nullptr || b == nullptr || c == nullptr || d == nullptr) { return false; }
                melde("W3 Raster Spalten " + std::to_string(g->rasterSpalten) + ", Tasten " + std::to_string(g->tasten));
                return g->rasterSpalten == 2 && a->rect.Min.y == b->rect.Min.y && c->rect.Min.y == d->rect.Min.y && c->rect.Min.y > a->rect.Max.y && std::fabs(a->rect.GetWidth() - b->rect.GetWidth()) < 0.5F &&
                       g->tasten >= 1 && g->symbole == g->knoepfe;
            }));
            add(pruefe(zeile("W3", modus, "die Mischung des Pinsels bleibt waehlbar: Hinzufuegen, Abziehen, Setzen als eine Reihe"), [] {
                const Element* a = imRoll(tr(Str::MbGwHinzu));
                const Element* b = imRoll(tr(Str::MbGwAbziehen));
                const Element* c = imRoll(tr(Str::MbGwSetzen));
                return inGruppe(Str::MgRPinsel, a) && inGruppe(Str::MgRPinsel, b) && inGruppe(Str::MgRPinsel, c) && a->rect.Min.y == c->rect.Min.y && (a->flags & ImGuiItemStatusFlags_Checked) != 0;
            }));
            // Verlauf: Setzen 0,8 von der mittleren Ecke nach rechts - so weit, dass rechts davon noch Ecken von torso im Bild sind
            // (60 % des Wegs bis zur rechtesten, hoechstens 200 Bildpunkte)
            add(tu("AP8 W3: Setzen, Gewicht 0,8, Staerke 1, Gewichte und Bildorte von torso vorher", [p] {
                mtab().pinselArt = static_cast<int>(modell::PinselArt::Setzen);
                mtab().pinselGewicht = 0.8F;
                mtab().pinselStaerke = 1.0F;
                p->a = modell::EckOrt{mb1::teil("torso"), vordereEcke(mb1::teil("torso"), p->ort)};
                p->gewichte = gewichteVon(p->a.teil, mtab().knochenWahl);
                p->stand = mtab().dok.stand();
                schirmMerken(*p, p->a.teil);
                float rechts = p->ort.x;
                for (std::size_t e = 0; e < p->schirm.size(); ++e) {
                    if (p->schirmDa[e] != 0) { rechts = std::max(rechts, p->schirm[e].x); }
                }
                p->laenge = std::floor(std::min(200.0F, (rechts - p->ort.x) * 0.6F));
                p->startX = std::floor(p->ort.x) + 1.0F;
                melde("W3 Verlauf: rechteste Ecke " + std::to_string(rechts - p->ort.x) + " Bildpunkte rechts der Ecke, Strecke " + std::to_string(p->laenge));
            }));
            add(klick(tr(Str::MgVerlauf), "mtrollouts"));
            add(pause(0.2));
            add(pruefe(zeile("W3", modus, "Verlauf gewaehlt: der Knopf ist an, darunter die Wahl Linear / Radial"), [] {
                const Element* e = imRoll(tr(Str::MgVerlauf));
                return gw().werkzeug == 2 && e != nullptr && (e->flags & ImGuiItemStatusFlags_Checked) != 0 && inGruppe(Str::MgRPinsel, imRoll(tr(Str::MgVerlaufLinear))) &&
                       inGruppe(Str::MgRPinsel, imRoll(tr(Str::MgVerlaufRadial)));
            }));
            add(ziehenProbe("AP8 W3: Verlauf von der mittleren Ecke nach rechts", p, 0.0F, 1.0F));
            add(pause(0.3));
            add(pruefe(zeile("W3", modus, "Verlauf: am Anfang das ganze Gewicht 0,8, hinter dem Ende bleibt jede Ecke, dazwischen faellt es; ein Rueckgaengig-Schritt"), [p] {
                const modell::Netz* n = netz(p->a.teil);
                if (n == nullptr) { return false; }
                const int k = mtab().knochenWahl;
                int dahinterGleich = 0;
                int dahinterAnders = 0;
                int dazwischen = 0;
                int davorVoll = 0;
                int davorAnders = 0;
                // die Lage jeder Ecke auf der Strecke nach ihrem Bildort beim Beginn des Ziehens (dort rechnet der Verlauf)
                for (std::size_t e = 0; e < n->ecken.size() && e < p->gewichte.size() && e < p->schirm.size(); ++e) {
                    if (p->schirmDa[e] == 0) { continue; }
                    const float w = modell::gewichtVon(*n, static_cast<int>(e), k);
                    const float lage = (p->schirm[e].x - p->startX) / p->laenge;
                    if (lage >= 1.0F) {
                        (w == p->gewichte[e] ? dahinterGleich : dahinterAnders) += 1;
                    } else if (lage <= 0.0F) {
                        (std::fabs(w - 0.8F) < 1e-5F ? davorVoll : davorAnders) += 1;
                    } else if (w > p->gewichte[e] && w < 0.8F + 1e-5F) {
                        ++dazwischen;
                    }
                }
                melde("W3 Verlauf: davor voll " + std::to_string(davorVoll) + " / anders " + std::to_string(davorAnders) + ", dazwischen " + std::to_string(dazwischen) + ", dahinter gleich " +
                      std::to_string(dahinterGleich) + " / anders " + std::to_string(dahinterAnders) + ", Ecke am Anfang " + std::to_string(gewicht(p->a, k)) + ", Verlauf " +
                      std::to_string(gw().verlaufStart[0]) + "/" + std::to_string(gw().verlaufStart[1]) + " -> " + std::to_string(gw().verlaufEnde[0]) + "/" +
                      std::to_string(gw().verlaufEnde[1]) + ", Ecke bei " + std::to_string(p->ort.x) + "/" + std::to_string(p->ort.y));
                return std::fabs(gewicht(p->a, k) - 0.8F) < 1e-5F && davorVoll > 0 && davorAnders == 0 && dazwischen > 0 && dahinterAnders == 0 && dahinterGleich > 0 &&
                       mtab().dok.rueckgaengigName() == tr(Str::MgUndoVerlauf) && !gw().verlaufZieht;
            }));
            add(tu("AP8 W3: Rueckgaengig", [] { (void)modellRueckgaengig(); }));
            add(pause(0.2));
            add(pruefe(zeile("W3", modus, "Rueckgaengig nimmt den ganzen Verlauf zurueck"), [p] { return gewichteVon(p->a.teil, mtab().knochenWahl) == p->gewichte; }));
            // Verwischen: erst ein Verlauf als Gefaelle, dann ein Strich gegen das Gefaelle
            add(ziehenProbe("AP8 W3: noch einmal der Verlauf", p, 0.0F, 1.0F));
            add(klick(tr(Str::MgVerwischen), "mtrollouts"));
            add(pause(0.2));
            add(tu("AP8 W3: Gewichte vor dem Verwischen", [p] { p->gewichte = gewichteVon(p->a.teil, mtab().knochenWahl); }));
            add(ziehenProbe("AP8 W3: Verwischen auf der Strecke des Verlaufs von 30 % bis 90 % nach rechts", p, 0.3F, 0.6F));
            add(pause(0.3));
            add(pruefe(zeile("W3", modus, "Verwischen: der Strich nach rechts zieht das hoehere Gewicht von links mit (Ecken unter dem Pinsel steigen, keine faellt), ein Rueckgaengig-Schritt"), [p] {
                const std::vector<float> jetzt = gewichteVon(p->a.teil, mtab().knochenWahl);
                int hoeher = 0;
                int tiefer = 0;
                for (std::size_t e = 0; e < jetzt.size() && e < p->gewichte.size(); ++e) {
                    hoeher += jetzt[e] > p->gewichte[e] + 1e-6F ? 1 : 0;
                    tiefer += jetzt[e] < p->gewichte[e] - 1e-6F ? 1 : 0;
                }
                melde("W3 Verwischen: " + std::to_string(hoeher) + " Ecken hoeher, " + std::to_string(tiefer) + " tiefer");
                return gw().werkzeug == 1 && hoeher > 0 && tiefer == 0 && mtab().dok.rueckgaengigName() == tr(Str::MbUndoMalen);
            }));
            add(klick(tr(Str::MgWeich), "mtrollouts"));
            add(pause(0.2));
            add(pruefe(zeile("W3", modus, "Weichzeichnen waehlt den Glaetten-Pinsel von rc611"), [] { return gw().werkzeug == 0 && mtab().pinselArt == static_cast<int>(modell::PinselArt::Glaetten); }));
            add(klick(tr(Str::MbGwHinzu), "mtrollouts"));
            add(pause(0.2));

            // --- W4: Optionen und Bereinigen ---
            add(zeige(Str::MgROptionen));
            add(pause(0.4));
            add(pruefe(zeile("W4", modus, "Optionen: Auto-Normalisieren (an), X-Spiegeln (aus), Hoechstens 4 je Ecke (JKA) fest an - als Schalter in zwei Spalten"), [] {
                const Element* a = imRoll(tr(Str::MgAutoNorm));
                const Element* b = imRoll(tr(Str::MgXSpiegeln));
                const Element* c = imRoll(tr(Str::MgVier));
                if (!inGruppe(Str::MgROptionen, a) || !inGruppe(Str::MgROptionen, b) || !inGruppe(Str::MgROptionen, c)) { return false; }
                return (a->flags & ImGuiItemStatusFlags_Checked) != 0 && (b->flags & ImGuiItemStatusFlags_Checked) == 0 && (c->flags & ImGuiItemStatusFlags_Checked) != 0 &&
                       a->rect.Min.y == b->rect.Min.y && b->rect.Min.x > a->rect.Max.x - 1.0F && c->rect.Min.y > a->rect.Min.y;
            }));
            add(tu("AP8 W4: zurueck auf die Datei", [] { alleZurueck(); }));
            add(pause(0.3));
            add(tu("AP8 W4: Setzen 0,5 auf die mittlere Ecke von torso (Auto-Normalisieren an)", [p] {
                mtab().pinselArt = static_cast<int>(modell::PinselArt::Setzen);
                mtab().pinselGewicht = 0.5F;
                mtab().pinselRadius = 3.0F;
                p->a = modell::EckOrt{mb1::teil("torso"), vordereEcke(mb1::teil("torso"), p->ort)};
                (void)modellTupfer(mtab().aktiv, p->ort, true);
                modellStrichEnde();
            }));
            add(pruefe(zeile("W4", modus, "Auto-Normalisieren an: der gemalte Knochen traegt 0,5, die uebrigen teilen sich den Rest - Summe 1"), [p] {
                melde("W4 an: Gewicht " + std::to_string(gewicht(p->a, mtab().knochenWahl)) + ", Summe " + std::to_string(summe(p->a)));
                return std::fabs(gewicht(p->a, mtab().knochenWahl) - 0.5F) < 1e-6F && std::fabs(summe(p->a) - 1.0F) < 1e-5F;
            }));
            add(tu("AP8 W4: Rueckgaengig", [] { (void)modellRueckgaengig(); }));
            add(ap3::insFeld(tr(Str::MgAutoNorm)));
            add(pause(0.3));
            add(klick(tr(Str::MgAutoNorm), "mtrollouts"));
            add(pause(0.2));
            add(tu("AP8 W4: derselbe Tupfer ohne Auto-Normalisieren", [p] {
                const bool ok = modellTupfer(mtab().aktiv, p->ort, true);
                melde("W4 aus: Tupfer " + std::to_string(ok ? 1 : 0) + ", " + std::to_string(mtab().gemalteEcken) + " Ecken, Ecke " + std::to_string(p->a.ecke) + " bei " +
                      std::to_string(p->ort.x) + "/" + std::to_string(p->ort.y) + ", Malliste " + std::to_string(netz(p->a.teil) != nullptr ? netz(p->a.teil)->mal.size() : 0U));
                modellStrichEnde();
            }));
            add(pruefe(zeile("W4", modus, "Auto-Normalisieren aus: nur der gemalte Knochen aendert sich (wie rc611) - Summe 1,5"), [p] {
                melde("W4 aus: Summe " + std::to_string(summe(p->a)));
                return !gw().autoNormalisieren && std::fabs(gewicht(p->a, mtab().knochenWahl) - 0.5F) < 1e-6F && std::fabs(summe(p->a) - 1.5F) < 1e-5F;
            }));
            add(ap3::insFeld(tr(Str::MgAutoNorm)));
            add(pause(0.3));
            add(klick(tr(Str::MgAutoNorm), "mtrollouts"));
            add(klickAn("AP8 W4: auf den gesperrten Schalter Hoechstens 4", [] {
                const Element* e = imRoll(tr(Str::MgVier));
                return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
            }));
            add(pause(0.2));
            add(pruefe(zeile("W4", modus, "Hoechstens 4 je Ecke laesst sich nicht abwaehlen"), [] {
                const Element* c = imRoll(tr(Str::MgVier));
                return c != nullptr && (c->flags & ImGuiItemStatusFlags_Checked) != 0 && gw().autoNormalisieren;
            }));
            add(zeige(Str::MgRBereinigen));
            add(pause(0.4));
            add(pruefe(zeile("W4", modus, "Bereinigen: Alles normalisieren, Saeubern / Glaetten, Begrenzen: 4 im Raster mit zwei Spalten"), [] {
                const ui::RolloutProbe* g = feldGruppe(Str::MgRBereinigen);
                const Element* a = imRoll(tr(Str::MgNormAlle));
                const Element* b = imRoll(tr(Str::MgSaeubern));
                const Element* c = imRoll(tr(Str::MbGwGlaetten));
                const Element* d = imRoll(tr(Str::MgBegrenzen));
                if (g == nullptr || !inGruppe(Str::MgRBereinigen, a) || !inGruppe(Str::MgRBereinigen, b) || !inGruppe(Str::MgRBereinigen, c) || !inGruppe(Str::MgRBereinigen, d)) { return false; }
                return g->rasterSpalten == 2 && a->rect.Min.y == b->rect.Min.y && c->rect.Min.y == d->rect.Min.y && c->rect.Min.y > a->rect.Max.y && g->symbole == 4;
            }));
            add(tu("AP8 W4: eine Ecke mit sechs Knochen (0,5 0,4 0,3 0,2 0,06 0,04), Grenze 0,05", [p] {
                alleZurueck();
                sechsGewichte(p->a);
                gw().saeubernGrenze = 0.05F;
                mtab().knochenWahl = 2;
            }));
            add(pause(0.3));
            add(klick(tr(Str::MgSaeubern), "mtrollouts"));
            add(pause(0.3));
            add(pruefe(zeile("W4", modus, "Saeubern entfernt das Gewicht 0,04 (bis zur Grenze 0,05), 0,06 bleibt; die Summe wird 1"), [p] {
                melde("W4 Saeubern: Knochen 6 " + std::to_string(gewicht(p->a, 6)) + ", Knochen 5 " + std::to_string(gewicht(p->a, 5)) + ", Summe " + std::to_string(summe(p->a)));
                return gewicht(p->a, 6) == 0.0F && gewicht(p->a, 5) > 0.0F && std::fabs(summe(p->a) - 1.0F) < 1e-5F && mtab().dok.rueckgaengigName() == tr(Str::MgUndoSaeubern);
            }));
            add(klick(tr(Str::MgBegrenzen), "mtrollouts"));
            add(pause(0.3));
            add(pruefe(zeile("W4", modus, "Begrenzen: 4 laesst der Ecke ihre vier groessten Knochen"), [p] {
                const modell::Netz* n = netz(p->a.teil);
                return n != nullptr && n->mal[static_cast<std::size_t>(p->a.ecke)].size() == 4U && gewicht(p->a, 5) == 0.0F && gewicht(p->a, 1) > gewicht(p->a, 4) &&
                       mtab().dok.rueckgaengigName() == tr(Str::MgUndoBegrenzen);
            }));
            add(tu("AP8 W4: Rueckgaengig zweimal, Auto-Normalisieren aus", [] {
                (void)modellRueckgaengig();
                (void)modellRueckgaengig();
                gw().autoNormalisieren = false;
            }));
            add(klick(tr(Str::MgNormAlle), "mtrollouts"));
            add(pause(0.3));
            add(pruefe(zeile("W4", modus, "Alles normalisieren: Summe 1, der gewaehlte Knochen behaelt 0,4 (Lock Active)"), [p] {
                melde("W4 Normalisieren: Knochen 2 " + std::to_string(gewicht(p->a, 2)) + ", Summe " + std::to_string(summe(p->a)));
                return gewicht(p->a, 2) == 0.4F && std::fabs(summe(p->a) - 1.0F) < 1e-5F && mtab().dok.rueckgaengigName() == tr(Str::MgUndoNorm);
            }));
            add(tu("AP8 W4: Gewicht vor dem Glaetten", [p] { p->vorher = gewicht(p->a, 1); }));
            add(klick(tr(Str::MbGwGlaetten), "mtrollouts", 0, false, tr(Str::MgBegrenzen)));
            add(pause(0.4));
            add(pruefe(zeile("W4", modus, "Glaetten zieht die Gewichte aller Knochen zu den Nachbarn (Knochen 1 faellt, die Nachbarn haben ihn nicht)"), [p] {
                melde("W4 Glaetten: Knochen 1 " + std::to_string(p->vorher) + " -> " + std::to_string(gewicht(p->a, 1)));
                return gewicht(p->a, 1) < p->vorher && mtab().dok.rueckgaengigName() == tr(Str::MgUndoGlaetten);
            }));
            add(tu("AP8: zurueck auf die Datei", [] {
                alleZurueck();
                gw().autoNormalisieren = true;
                gw().saeubernGrenze = 0.0F;
                mtab().knochenWahl = knochen("rhumerus");
                mtab().pinselRadius = 40.0F;
                mtab().pinselGewicht = 1.0F;
            }));
            add(pause(0.3));
        }

        // --- W5 / W6: Spiegeln ---
        add(zeige(Str::MgRSpiegeln));
        add(pause(0.5));
        add(pruefe(zeile(bl ? "W6" : "W5", modus, "Spiegeln: Regler Toleranz 0,5 und die Tabelle Knochenpaare (ueber Namen), Ecken ohne Gegenstueck, Flaechenuebergreifend; darunter der Tipp zur Grenze von 32 Knochen"), [] {
            const Element* a = imRoll(tr(Str::MgToleranz));
            const Element* b = imRoll(tr(Str::MgPaare));
            const Element* c = imRoll(tr(Str::MgOhne));
            const Element* d = imRoll(tr(Str::MgUeberFlaechen));
            char tipp[200];
            std::snprintf(tipp, sizeof tipp, tr(Str::MgSpiegelTipp), 32);
            const Element* e = imRoll(tipp);
            const modell::SpiegelBefund& bf = gw().befund;
            melde("W5 Befund: Paare " + std::to_string(bf.knochenPaare) + " / " + std::to_string(bf.knochenPaareSoll) + ", Punkte " + std::to_string(bf.punkte) + ", ohne " +
                  std::to_string(bf.ohneGegenstueck) + " (Ecken " + std::to_string(bf.eckenOhne) + "), Mitte " + std::to_string(bf.mitte) + ", Flaechenpaare " + std::to_string(bf.teilPaare.size()) +
                  ", links bei x " + (gw().links > 0 ? ">" : "<") + " 0");
            return inGruppe(Str::MgRSpiegeln, a) && inGruppe(Str::MgRSpiegeln, b) && inGruppe(Str::MgRSpiegeln, c) && inGruppe(Str::MgRSpiegeln, d) && inGruppe(Str::MgRSpiegeln, e) &&
                   gw().toleranz == 0.5F && b->rect.Min.y >= a->rect.Max.y && c->rect.Min.y >= b->rect.Max.y && d->rect.Min.y >= c->rect.Max.y && e->rect.Min.y >= d->rect.Max.y &&
                   bf.knochenPaare >= 10 && bf.knochenPaare == bf.knochenPaareSoll && bf.punkte > 500 && gw().links != 0;
        }));
        add(tu("AP8 W5: ein gespiegeltes Punktepaar; links lhumerus 0,6 (Setzen ueber das Dokument)", [p] {
            if (!spiegelPaar(p->a, p->b)) {
                meldeFehler("AP8 W5: kein gespiegeltes Punktepaar im Modell");
                return;
            }
            const int stufe = mtab().lodJetzt;
            const int lh = knochen("lhumerus");
            const modell::EckenMaske m = modell::eckenMaske(mtab().dok.inhalt(), stufe, {p->a});
            modell::MalRegeln r;
            r.autoNormalisieren = true;
            mtab().dok.aendern("AP8 links 0,6", [&](modell::Inhalt& x) { (void)modell::gewichtSetzen(x, stufe, m, lh, 0.6F, r); });
            p->vorher = gewicht(p->b, knochen("rhumerus"));
            p->vorherB = gewicht(p->b, lh);
            g_app->modelDirty = true;
        }));
        add(pause(0.3));
        if (!bl) {
            add(pruefe(zeile("W5", modus, "Haken Spiegelmodus (links blau, rechts gruen), Raster Blau -> Gruen, Gruen -> Blau / Seiten tauschen, Nur Auswahl"), [] {
                const ui::RolloutProbe* g = feldGruppe(Str::MgRSpiegeln);
                const Element* m = imRoll(tr(Str::MgSpiegelModus));
                const Element* a = imRoll(tr(Str::MgBlauGruen));
                const Element* b = imRoll(tr(Str::MgGruenBlau));
                const Element* c = imRoll(tr(Str::MgTauschen));
                const Element* d = imRoll(tr(Str::MgNurAuswahl));
                if (g == nullptr || m == nullptr || a == nullptr || b == nullptr || c == nullptr || d == nullptr) { return false; }
                return (m->flags & ImGuiItemStatusFlags_Checked) == 0 && a->rect.Min.y >= m->rect.Max.y && a->rect.Min.y == b->rect.Min.y && c->rect.Min.y == d->rect.Min.y &&
                       c->rect.Min.y > a->rect.Max.y && g->rasterSpalten == 2 && g->symbole == 4;
            }));
            add(klick(tr(Str::MgSpiegelModus), "mtrollouts"));
            add(pause(0.5));
            add(pruefe(zeile("W5", modus, "Spiegelmodus an: die Ansicht zeigt die Ecken links blau, rechts gruen und die ohne Gegenstueck gelb"), [p] {
                const int* z = gw().spiegelPunkte;
                p->zahl = z[2];
                melde("W5 Anzeige: blau " + std::to_string(z[0]) + ", gruen " + std::to_string(z[1]) + ", gelb " + std::to_string(z[2]) + ", Mitte " + std::to_string(z[3]));
                return gw().spiegelModus && z[0] > 50 && z[1] > 50 && z[2] > 0;
            }));
            add(tu("AP8 W5: Toleranz 0,01", [] { gw().toleranz = 0.01F; }));
            add(pause(0.5));
            add(pruefe(zeile("W5", modus, "mit Toleranz 0,01 finden mehr Ecken kein Gegenstueck: mehr gelbe Ecken, die Tabelle zaehlt mit"), [p] {
                melde("W5 Toleranz 0,01: gelb " + std::to_string(gw().spiegelPunkte[2]) + " (vorher " + std::to_string(p->zahl) + "), Tabelle " + std::to_string(gw().befund.eckenOhne));
                char wert[64];
                std::snprintf(wert, sizeof wert, tr(Str::MgOhneWert), gw().befund.eckenOhne);
                return gw().spiegelPunkte[2] > p->zahl && gw().befund.eckenOhne > 0;
            }));
            add(tu("AP8 W5: Toleranz 0,5, Spiegelmodus aus", [] {
                gw().toleranz = 0.5F;
                gw().spiegelModus = false;
            }));
            add(pause(0.4));
            add(klick(tr(Str::MgBlauGruen), "mtrollouts"));
            add(pause(0.4));
            add(pruefe(zeile("W5", modus, "Blau -> Gruen: das Gegenstueck rechts traegt rhumerus 0,6 (der Knochen wechselt ueber den Namen die Seite); ein Rueckgaengig-Schritt, die Meldung zaehlt"), [p] {
                melde("W5 Blau -> Gruen: rechts rhumerus " + std::to_string(p->vorher) + " -> " + std::to_string(gewicht(p->b, knochen("rhumerus"))) + " | " + mtab().meldung);
                return gewicht(p->b, knochen("rhumerus")) == gewicht(p->a, knochen("lhumerus")) && gewicht(p->a, knochen("lhumerus")) == 0.6F && mtab().dok.rueckgaengigName() == tr(Str::MbUndoSpiegeln) &&
                       gw().letzteZahl > 300 && !mtab().meldung.empty();
            }));
            add(tu("AP8 W5: Rueckgaengig", [] { (void)modellRueckgaengig(); }));
            add(klick(tr(Str::MgTauschen), "mtrollouts"));
            add(pause(0.4));
            add(pruefe(zeile("W5", modus, "Seiten tauschen: rechts rhumerus 0,6, links jetzt, was rechts stand (als linker Knochen)"), [p] {
                melde("W5 Tauschen: rechts rhumerus " + std::to_string(gewicht(p->b, knochen("rhumerus"))) + ", links lhumerus " + std::to_string(gewicht(p->a, knochen("lhumerus"))) + " (rechts stand " +
                      std::to_string(p->vorher) + ")");
                return gewicht(p->b, knochen("rhumerus")) == 0.6F && gewicht(p->a, knochen("lhumerus")) == p->vorher;
            }));
            add(tu("AP8 W5: Rueckgaengig, Ebene Ecke, eine andere Ecke gewaehlt (nicht das Paar)", [p] {
                (void)modellRueckgaengig();
                modellAuswahlArtSetzen(AuswahlArt::Ecke);
                mtab().eckenWahl = {modell::EckOrt{mb1::teil("torso"), 0}};
            }));
            add(klick(tr(Str::MgNurAuswahl), "mtrollouts"));
            add(klick(tr(Str::MgBlauGruen), "mtrollouts"));
            add(pause(0.4));
            add(pruefe(zeile("W5", modus, "Nur Auswahl: das Paar ist nicht gewaehlt und bleibt, wie es war"), [p] {
                return gw().nurAuswahl && gewicht(p->b, knochen("rhumerus")) == p->vorher && gewicht(p->a, knochen("lhumerus")) == 0.6F;
            }));
            add(tu("AP8 W5: das linke der beiden gewaehlt", [p] { mtab().eckenWahl = {p->a}; }));
            add(klick(tr(Str::MgBlauGruen), "mtrollouts"));
            add(pause(0.4));
            add(pruefe(zeile("W5", modus, "Nur Auswahl mit der linken Ecke gewaehlt: nur ihr Gegenstueck bekommt die Gewichte"), [p] {
                melde("W5 Nur Auswahl: " + std::to_string(gw().letzteZahl) + " Ecken");
                return gewicht(p->b, knochen("rhumerus")) == 0.6F && gw().letzteZahl >= 1 && gw().letzteZahl < 12;
            }));
            add(klick(tr(Str::MgNurAuswahl), "mtrollouts"));
        } else {
            add(pruefe(zeile("W6", modus, "Raster Gruppe spiegeln, Alle Gruppen; Haken Gewichte spiegeln (an), Namen tauschen (an), Topologie-Spiegeln (aus), Beim Malen spiegeln"), [] {
                const Element* a = imRoll(tr(Str::MgGruppeSpiegeln));
                const Element* b = imRoll(tr(Str::MgAlleGruppen));
                const Element* c = imRoll(tr(Str::MgBlGewichte));
                const Element* d = imRoll(tr(Str::MgBlNamen));
                const Element* e = imRoll(tr(Str::MgBlTopologie));
                const Element* f = imRoll(tr(Str::MbGwSpiegelMalen));
                for (const Element* x : {a, b, c, d, e, f}) {
                    if (!inGruppe(Str::MgRSpiegeln, x)) { return false; }
                }
                return a->rect.Min.y == b->rect.Min.y && c->rect.Min.y >= a->rect.Max.y && c->rect.Min.y == d->rect.Min.y && e->rect.Min.y == f->rect.Min.y && e->rect.Min.y > c->rect.Min.y &&
                       (c->flags & ImGuiItemStatusFlags_Checked) != 0 && (d->flags & ImGuiItemStatusFlags_Checked) != 0 && (e->flags & ImGuiItemStatusFlags_Checked) == 0;
            }));
            add(tu("AP8 W6: Knochen lhumerus", [] { mtab().knochenWahl = knochen("lhumerus"); }));
            add(klick(tr(Str::MgGruppeSpiegeln), "mtrollouts"));
            add(pause(0.4));
            add(pruefe(zeile("W6", modus, "Gruppe spiegeln (ohne Auswahl tauschen beide Seiten, mit Namentausch): rechts steht das Gewicht 0,6 jetzt an rhumerus, links an lhumerus, was rechts stand"), [p] {
                melde("W6 Gruppe: rechts rhumerus " + std::to_string(gewicht(p->b, knochen("rhumerus"))) + ", links lhumerus " + std::to_string(gewicht(p->a, knochen("lhumerus"))) + " (rechts lhumerus war " +
                      std::to_string(p->vorherB) + ") | " + mtab().meldung);
                // wie dvert_mirror_op: die Gewichte der aktiven Gruppe lhumerus tauschen (links 0,6 <-> rechts p->vorherB), dann wechseln
                // lhumerus und rhumerus an beiden Punkten den Namen: rechts wird lhumerus 0,6 zu rhumerus 0,6
                return gewicht(p->b, knochen("rhumerus")) == 0.6F && gewicht(p->a, knochen("rhumerus")) == p->vorherB && mtab().dok.rueckgaengigName() == tr(Str::MbUndoSpiegeln) &&
                       gw().letzteZahl > 300;
            }));
            add(tu("AP8 W6: Rueckgaengig", [] { (void)modellRueckgaengig(); }));
            add(klick(tr(Str::MgAlleGruppen), "mtrollouts"));
            add(pause(0.4));
            add(pruefe(zeile("W6", modus, "Alle Gruppen: die ganzen Gewichte der beiden Punkte tauschen und wechseln die Seite"), [p] {
                return gewicht(p->b, knochen("rhumerus")) == 0.6F && gewicht(p->a, knochen("lhumerus")) == p->vorher;
            }));
            add(tu("AP8 W6: Rueckgaengig, Zahl der Punkte ohne Gegenstueck", [p] {
                (void)modellRueckgaengig();
                p->zahl = gw().befund.ohneGegenstueck;
            }));
            add(klick(tr(Str::MgBlTopologie), "mtrollouts"));
            add(pause(0.5));
            add(pruefe(zeile("W6", modus, "Topologie-Spiegeln: die Gegenstuecke kommen aus dem Aufbau des Netzes (die Tabelle zaehlt neu)"), [p] {
                melde("W6 Topologie: ohne Gegenstueck " + std::to_string(gw().befund.ohneGegenstueck) + " (ueber die Lage " + std::to_string(p->zahl) + ")");
                return gw().blTopologie && gw().befund.ohneGegenstueck != p->zahl;
            }));
            add(klick(tr(Str::MgBlTopologie), "mtrollouts"));
            add(tu("AP8 W6: Knochen rhumerus", [] { mtab().knochenWahl = knochen("rhumerus"); }));
        }
        add(tu("AP8: zurueck auf die Datei, Objektmodus", [] {
            alleZurueck();
            modellAuswahlArtSetzen(AuswahlArt::Objekt);
            gw().nurAuswahl = false;
            gw().blTopologie = false;
        }));
        add(pause(0.4));

        // --- W7: der Ort von Auto-Skin ---
        add(zeige(Str::AsGruppe));
        add(pause(0.5));
        add(pruefe(zeile("W7", modus, "Automatisch (Auto-Skin) steht als Gruppe zwischen Spiegeln und Knochen, mit dem Knopf Automatisch gewichten"), [] {
            const ui::RolloutProbe* s = feldGruppe(Str::MgRSpiegeln);
            const ui::RolloutProbe* a = feldGruppe(Str::AsGruppe);
            const ui::RolloutProbe* k = feldGruppe(Str::MgRKnochen);
            return s != nullptr && a != nullptr && k != nullptr && a->offen && a->kasten.Min.y >= s->kasten.Max.y && k->kasten.Min.y >= a->kasten.Max.y &&
                   inGruppe(Str::AsGruppe, imRoll(tr(Str::AsAusfuehren)));
        }));
        add(tu("AP8 W7: Auto-Skin: Einstellungen (Aktion)", [] { modellAutoSkinOeffnen(); }));
        add(pause(0.4));
        add(pruefe(zeile("W7", modus, "das Fenster Auto-Skin bleibt ueber die Aktion erreichbar"), [] { return modellAutoSkinZustand().fensterSichtbar; }));
        add(tu("AP8 W7: Fenster zu", [] { modellAutoSkinZustand().fensterOffen = false; }));

        // --- W8: Knochen ---
        add(zeige(Str::MgRKnochen));
        add(pause(0.5));
        add(pruefe(zeile("W8", modus, "Knochen: Kurzinfo der Knochenname; Zeile Gewaehlt mit der Auswahlliste, Zeile Pose mit Sequenz und Bild, Sperren, Knochen zeigen, der Tipp"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::MgRKnochen);
            ImGuiWindow* w = kindFenster("mtrollouts");
            const Element* liste = w != nullptr ? findeKennung(w->GetID("##gwknochen")) : nullptr;
            const Element* pose = imRoll(tr(Str::MgPose));
            const Element* tipp = imRoll(tr(Str::MgKnochenTipp));
            if (g == nullptr || liste == nullptr || pose == nullptr || tipp == nullptr) { return false; }
            melde("W8 Kurzinfo \"" + g->kurzinfo + "\"");
            return g->kurzinfo == "rhumerus" && g->kasten.Contains(liste->rect) && g->kasten.Contains(pose->rect) && g->kasten.Contains(tipp->rect) && pose->rect.Min.y >= liste->rect.Max.y &&
                   tipp->rect.Min.y > pose->rect.Max.y && inGruppe(Str::MgRKnochen, imRoll(tr(Str::MgSperre))) && inGruppe(Str::MgRKnochen, imRoll(tr(Str::MgKnochenZeigen))) &&
                   inGruppe(Str::MgRKnochen, imRoll(tr(Str::MgSperrMenue)));
        }));
        add(tu("AP8 W8: Knochen lradius", [] { mtab().knochenWahl = knochen("lradius"); }));
        add(pause(0.3));
        add(pruefe(zeile("W8", modus, "ein anderer Knochen: die Kurzinfo folgt"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::MgRKnochen);
            return g != nullptr && g->kurzinfo == "lradius";
        }));
        add(tu("AP8 W8: Knochen rhumerus", [] { mtab().knochenWahl = knochen("rhumerus"); }));

        // --- W12: Sperren ---
        add(ap3::insFeld(tr(Str::MgSperre)));
        add(pause(0.3));
        add(klick(tr(Str::MgSperre), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("W12", modus, "Haken Sperren: der gewaehlte Knochen ist gesperrt und steht als Zeile mit x in der Gruppe"), [] {
            return modellGewichtGesperrt(knochen("rhumerus")) && inGruppe(Str::MgRKnochen, imRoll(tr(Str::MgEntsperren)));
        }));
        add(tu("AP8 W12: Tupfer auf die mittlere Ecke von torso mit dem gesperrten Knochen", [p] {
            p->a = modell::EckOrt{mb1::teil("torso"), vordereEcke(mb1::teil("torso"), p->ort)};
            p->vorher = gewicht(p->a, mtab().knochenWahl);
            p->stand = mtab().dok.stand();
            mtab().meldung.clear();
            (void)modellTupfer(mtab().aktiv, p->ort, true);
            modellStrichEnde();
        }));
        add(pruefe(zeile("W12", modus, "ein gesperrter Knochen laesst sich nicht malen: nichts aendert sich, die Meldung sagt es"), [p] {
            return gewicht(p->a, mtab().knochenWahl) == p->vorher && mtab().dok.stand() == p->stand && mtab().meldung == tr(Str::MgGesperrtAktiv);
        }));
        add(tu("AP8 W12: rhumerus an der Ecke auf 0,3 (ueber das Dokument), dann lhumerus mit Setzen 0,5 darueber malen", [p] {
            const int stufe = mtab().lodJetzt;
            const int rh = knochen("rhumerus");
            const modell::EckenMaske m = modell::eckenMaske(mtab().dok.inhalt(), stufe, {p->a});
            modell::MalRegeln r;
            r.autoNormalisieren = true;
            mtab().dok.aendern("AP8 rhumerus 0,3", [&](modell::Inhalt& x) { (void)modell::gewichtSetzen(x, stufe, m, rh, 0.3F, r); });
            mtab().knochenWahl = knochen("lhumerus");
            mtab().pinselArt = static_cast<int>(modell::PinselArt::Setzen);
            mtab().pinselGewicht = 0.5F;
            mtab().pinselRadius = 3.0F;
            g_app->modelDirty = true;
        }));
        add(pause(0.3));
        add(tu("AP8 W12: Tupfer", [p] {
            (void)vordereEcke(mb1::teil("torso"), p->ort);
            (void)modellTupfer(mtab().aktiv, p->ort, true);
            modellStrichEnde();
        }));
        add(pruefe(zeile("W12", modus, "ein anderer Knochen daneben: der gesperrte behaelt 0,3, der gemalte traegt 0,5, die uebrigen teilen sich 0,2"), [p] {
            melde("W12 gesperrt " + std::to_string(gewicht(p->a, knochen("rhumerus"))) + ", gemalt " + std::to_string(gewicht(p->a, knochen("lhumerus"))) + ", Summe " + std::to_string(summe(p->a)));
            return gewicht(p->a, knochen("rhumerus")) == 0.3F && std::fabs(gewicht(p->a, knochen("lhumerus")) - 0.5F) < 1e-6F && std::fabs(summe(p->a) - 1.0F) < 1e-5F;
        }));
        add(ap3::insFeld(tr(Str::MgEntsperren)));
        add(pause(0.3));
        add(klick(tr(Str::MgEntsperren), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(zeile("W12", modus, "das x der Zeile entsperrt den Knochen"), [] { return !modellGewichtGesperrt(knochen("rhumerus")) && imRoll(tr(Str::MgEntsperren)) == nullptr; }));
        if (bl) {
            add(tu("AP8 W12: Malen aus (der Klick fuer den Fokus soll nichts malen)", [] { gw().malen = false; }));
            add(klickAn("AP8: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
            add(tu("AP8 W12: Malen an, torso gewaehlt, Knochen rhumerus", [] {
                gw().malen = true;
                mtab().auswahl = {mb1::teil("torso")};
                mtab().knochenWahl = knochen("rhumerus");
            }));
            add(taste(ImGuiKey_K, "K"));
            add(pause(0.4));
            add(pruefe(zeile("W12", modus, "K oeffnet das Sperren-Menue mit den acht Eintraegen wie Blender (alle, gewaehlter, nicht gewaehlte, nur ..., umkehren)"), [] {
                bool alle = gw().sperrMenueSichtbar;
                for (const Str x : {Str::MgLockAlle, Str::MgUnlockAlle, Str::MgLockGew, Str::MgUnlockGew, Str::MgLockAndere, Str::MgLockNurGew, Str::MgLockNurAndere, Str::MgLockUmkehren}) {
                    alle = alle && finde(tr(x), "##Popup_", 0, false) != nullptr;
                }
                return alle;
            }));
            add(klick(tr(Str::MgLockAndere), "##Popup_"));
            add(pause(0.3));
            add(pruefe(zeile("W12", modus, "Nicht gewaehlte sperren: alle Knochen ausser dem gewaehlten sind gesperrt"), [] {
                const std::vector<char>& g = gw().gesperrt;
                const int rh = knochen("rhumerus");
                int zahl = 0;
                for (const char c : g) { zahl += c != 0 ? 1 : 0; }
                return !gw().sperrMenueSichtbar && rh >= 0 && g[static_cast<std::size_t>(rh)] == 0 && zahl == static_cast<int>(g.size()) - 1;
            }));
            add(tu("AP8 W12: Sperren umkehren (Aktion), dann alle entsperren", [] {
                modellGewichteSperren(3, 0);
            }));
            add(pruefe(zeile("W12", modus, "Sperren umkehren: nur noch der gewaehlte ist gesperrt"), [] {
                int zahl = 0;
                for (const char c : gw().gesperrt) { zahl += c != 0 ? 1 : 0; }
                return zahl == 1 && modellGewichtGesperrt(knochen("rhumerus"));
            }));
            add(tu("AP8 W12: alle entsperren", [] { modellGewichteSperren(2, 0); }));
        } else {
            add(klick(tr(Str::MgSperrMenue), "mtrollouts"));
            add(pause(0.4));
            add(pruefe(zeile("W12", modus, "der Knopf Sperren... oeffnet dasselbe Menue (wie 3ds Max ohne Taste)"), [] { return gw().sperrMenueSichtbar && finde(tr(Str::MgLockAlle), "##Popup_", 0, false) != nullptr; }));
            add(klick(tr(Str::MgLockAlle), "##Popup_"));
            add(pause(0.3));
            add(pruefe(zeile("W12", modus, "Alle sperren, dann Alles normalisieren: die Meldung sagt, dass alle Knochen gesperrt sind"), [] {
                (void)modellGewichteBereinigen(0);
                const bool ok = mtab().meldung == tr(Str::MgAlleGesperrt);
                modellGewichteSperren(2, 0);
                return ok;
            }));
        }
        add(tu("AP8: zurueck auf die Datei", [] {
            alleZurueck();
            mtab().knochenWahl = knochen("rhumerus");
            mtab().pinselArt = static_cast<int>(modell::PinselArt::Hinzufuegen);
            mtab().pinselGewicht = 1.0F;
            mtab().pinselRadius = 40.0F;
        }));
        add(pause(0.3));

        // --- W9: JKA-Grenzen ---
        add(zeige(Str::MgRGrenzen));
        add(pause(0.5));
        add(pruefe(zeile("W9", modus, "JKA-Grenzen: Balken Ecken je Flaeche (torso), Knochen je Flaeche (torso), Gewichte je Ecke; Zeile Flaechen im Modell; Kurzinfo n / 32 Knochen; Uebernehmen"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::MgRGrenzen);
            char a[120];
            char b[120];
            std::snprintf(a, sizeof a, tr(Str::MgGrEcken), "torso");
            std::snprintf(b, sizeof b, tr(Str::MgGrKnochen), "torso");
            const Element* ea = imRoll(a);
            const Element* eb = imRoll(b);
            const Element* ec = imRoll(tr(Str::MgGrGewichte));
            const Element* ed = imRoll(tr(Str::MgGrFlaechen));
            const Element* ue = imRoll(tr(Str::MbGwUebernehmen));
            if (g == nullptr || ea == nullptr || eb == nullptr || ec == nullptr || ed == nullptr || ue == nullptr) { return false; }
            melde("W9 Kurzinfo \"" + g->kurzinfo + "\"");
            return g->kasten.Contains(ea->rect) && eb->rect.Min.y >= ea->rect.Max.y && ec->rect.Min.y >= eb->rect.Max.y && ed->rect.Min.y >= ec->rect.Max.y && g->kasten.Contains(ue->rect) &&
                   g->kurzinfo.find("/ 32") != std::string::npos && malend() == 0;
        }));
        add(klickAn("AP8 W9: auf den gesperrten Knopf Uebernehmen", [] {
            const Element* e = imRoll(tr(Str::MbGwUebernehmen));
            return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
        }));
        add(pause(0.3));
        add(pruefe(zeile("W9", modus, "ohne Malliste ist Uebernehmen gesperrt: der Klick tut nichts"), [] { return !mtab().dok.kannRueckgaengig(); }));
        add(tu("AP8 W9: eine Ecke von torso mit sechs Knochen", [p] {
            p->a = modell::EckOrt{mb1::teil("torso"), 0};
            sechsGewichte(p->a);
        }));
        add(pause(0.4));
        add(pruefe(zeile("W9", modus, "eine Ecke mit sechs Gewichten: eine Malliste ueber vier, der Balken Gewichte je Ecke steht da"), [] {
            return malend() == 1 && modell::eckenUeberVier(*netz(mb1::teil("torso"))) == 1 && inGruppe(Str::MgRGrenzen, imRoll(tr(Str::MgGrGewichte)));
        }));
        add(ap3::insFeld(tr(Str::MbGwUebernehmen)));
        add(pause(0.3));
        add(klick(tr(Str::MbGwUebernehmen), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(zeile("W9", modus, "Uebernehmen schreibt vier Gewichte wie das Spiel sie liest in die Ecke (kein Knochen ohne Gewicht), die Mallisten sind weg"), [p] {
            const modell::Netz* n = netz(p->a.teil);
            if (n == nullptr || !n->mal.empty()) { return false; }
            const modell::Ecke& e = n->ecken[static_cast<std::size_t>(p->a.ecke)];
            float sum = 0.0F;
            bool positiv = true;
            for (int k = 0; k < 4; ++k) {
                sum += e.gewicht[k];
                positiv = positiv && e.gewicht[k] > 0.0F;
            }
            return positiv && std::fabs(sum - 1.0F) < 1e-6F && e.knochen[0] == 1;
        }));
        add(tu("AP8: zurueck auf die Datei", [] { alleZurueck(); }));
        add(pause(0.3));

        // --- W10: Ansicht ---
        add(pruefe(zeile("W10", modus, "im Bereich Weights sind die Knochen ab Werk zu sehen (der Schalter Bones ist an, die Ansicht zeichnet sie), die Figur traegt die Farben der Gewichte"), [] {
            melde("W10 Knochen-Striche " + std::to_string(g_app->modellKnochenLinien));
            return g_app->modellKnochen && g_app->modellKnochenLinien > 0;
        }));
        add(tu("AP8 W10: nach Ansehen/Pruefen", [] { mtab().bereich = ModellBereich::Ansehen; }));
        add(pause(0.4));
        add(pruefe(zeile("W10", modus, "ausserhalb von Weights steht der Schalter wieder, wie er war (aus)"), [] { return !g_app->modellKnochen && g_app->modellKnochenLinien == 0; }));
        add(tu("AP8 W10: zurueck nach Weights, Knochen dort ausschalten, hinaus und wieder hinein", [] { mtab().bereich = ModellBereich::Weights; }));
        add(pause(0.3));
        add(tu("AP8 W10: Schalter aus", [] { g_app->modellKnochen = false; }));
        add(pause(0.2));
        add(tu("AP8 W10: hinaus", [] { mtab().bereich = ModellBereich::Ansehen; }));
        add(pause(0.3));
        add(tu("AP8 W10: hinein", [] { mtab().bereich = ModellBereich::Weights; }));
        add(pause(0.3));
        add(pruefe(zeile("W10", modus, "der Schalter merkt sich seinen Stand je Bereich: in Weights ausgeschaltet bleibt er dort aus"), [] { return !g_app->modellKnochen; }));
        add(tu("AP8 W10: Knochen in Weights wieder an", [] { g_app->modellKnochen = true; }));
        add(tu("AP8 W10: Maus ueber die Figur", [p] {
            (void)vordereEcke(mb1::teil("torso"), p->ort);
            setzeMaus(p->ort);
        }));
        add(pause(0.3));
        add(pruefe(zeile("W10", modus, "der Pinselkreis steht an der Maus, solange gemalt wird"), [] { return mtab().ueber[11] > 0; }));
        add(tu("AP8 W10: Malen aus", [] { gw().malen = false; }));
        add(pause(0.3));
        add(pruefe(zeile("W10", modus, "ohne Malen kein Pinselkreis"), [] {
            const bool ok = mtab().ueber[11] == 0;
            gw().malen = true;
            return ok;
        }));

        // --- W13: kurze Zeitleiste ---
        add(tu("AP8 W13: Abspielen an", [p] {
            p->frame = g_app->animFrame;
            g_app->animPlaying = true;
        }));
        add(pause(0.7));
        add(pruefe(zeile("W13", modus, "im Bereich Weights steht die kurze Zeitleiste (AP1), und die Pose laeuft weiter, waehrend das Feld offen ist"), [p] {
            const ModellZeitProbe& z = modellZeit().probe;
            melde("W13 Fassung " + std::to_string(z.fassung) + ", Bild " + std::to_string(p->frame) + " -> " + std::to_string(g_app->animFrame));
            const bool ok = modellZeitFassung() == ZeitFassung::Kurz && z.fassung == static_cast<int>(ZeitFassung::Kurz) && g_app->animFrame != p->frame;
            g_app->animPlaying = false;
            return ok;
        }));

        // --- Finden: Suche, Rechtsklick, Tooltip ---
        add(pruefe(zeile("Finden", modus, "jede neue Funktion steht mit einem Hinweis in der Liste fuer Suche und Rechtsklick (24 Aktionen in vier Gruppen)"), [] {
            bool ok = modellGewichteAktionZahl() == 24;
            for (int i = 0; i < modellGewichteAktionZahl(); ++i) {
                ok = ok && modellGewichteAktionText(i)[0] != '\0' && std::strlen(modellGewichteAktionHinweis(i)) > 20U && modellGewichteAktionGruppe(i) >= 0 && modellGewichteAktionGruppe(i) <= 3;
            }
            return ok;
        }));
        add(tu("AP8 Finden: Alle Aktionen suchen", [] { mtab().aktionenOffen = true; }));
        add(pause(0.5));
        add(tippe(tr(Str::MgVerwischen)));
        add(pause(0.4));
        add(pruefe(zeile("Finden", modus, "\"Alle Aktionen suchen\": nach dem Tippen von Verwischen steht die Aktion in der Liste"), [] {
            return mtab().aktionenSichtbar && finde(tr(Str::MgVerwischen), "##Popup_", 0, false) != nullptr;
        }));
        add(klickLetzter(tr(Str::MgVerwischen), "##Popup_"));
        add(pause(0.4));
        add(pruefe(zeile("Finden", modus, "ein Klick auf die Zeile waehlt das Werkzeug Verwischen (und schaltet das Malen an), die Suche schliesst"), [] {
            const bool ok = gw().werkzeug == 1 && gw().malen && mtab().bereich == ModellBereich::Weights && !mtab().aktionenSichtbar;
            modellGewichteWerkzeug(0);
            return ok;
        }));
        add(tu("AP8 Finden: Rechtsklick-Menue der Ansicht", [] { mtab().kontextOeffnen = true; }));
        add(pause(0.5));
        add(pruefe(zeile("Finden", modus, "im Rechtsklick der Ansicht steht das Menue Gewichts-Werkzeuge"), [] { return mtab().kontextSichtbar && finde(tr(Str::MgMenue), "##Popup_", 0, false) != nullptr; }));
        add(klick(tr(Str::MgMenue), "##Popup_"));
        add(pause(0.5));
        add(pruefe(zeile("Finden", modus, "... mit den Untermenues Gewichts-Werkzeug/Pinsel, Bereinigen, Spiegeln, Sperren"), [] {
            int da = 0;
            for (const Str x : {Str::MgRWerkzeug, Str::MgRBereinigen, Str::MgRSpiegeln, Str::MgGrSperren}) {
                for (const Element& e : g_elemente) {
                    if (e.label == tr(x) && e.innen.find("mtrollouts") == std::string::npos) {
                        ++da;
                        break;
                    }
                }
            }
            melde("Finden Rechtsklick: " + std::to_string(da) + " Untermenues");
            return da == 4;
        }));
        add(mbefoto::popupsZu());
        add(pause(0.3));
        // Tooltip an einem Knopf des Felds
        add(zeige(Str::MgRSpiegeln));
        add(pause(0.4));
        add(mbefoto::mausAuf("den Regler Toleranz", [] { return imRoll(tr(Str::MgToleranz)); }));
        add(pruefe(zeile("Finden", modus, "unter der Maus zeigt der Regler Toleranz seinen Tooltip"), [] { return tooltipDa(); }));
        add(tu("AP8: Maus weg", [] { setzeMaus(ImVec2{-50.0F, -50.0F}); }));
        add(pause(0.2));
    }
}

// --- Fotos: beide Bedienmodi, das Feld oben / Mitte / unten, Spiegelmodus, Sperren-Menue -------------------------------------
void fotoSchritte(std::vector<Schritt>& s, const std::string& vor) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const std::string p = vor + (modus == 0 ? "max_" : "blender_");
        add(tu("AP8 Foto: " + p + "weights", [=] {
            alleZurueck();
            modellBedienungSetzen(modus);
            mtab().bereich = ModellBereich::Weights;
            mtab().knochenWahl = knochen("rhumerus");
            mtab().auswahl = {mb1::teil("torso")};
            modellAuswahlArtSetzen(AuswahlArt::Objekt);
            gw().spiegelModus = false;
            gw().werkzeug = 0;
            gw().malen = true;
            g_app->modellKnochen = true;
            g_app->settings.modellFeldBreite = 0.0F;
            if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
        }));
        add(pause(0.9));
        add(foto(p + "oben"));
        add(tu("AP8 Foto: Mitte", [] {
            if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, w->ScrollMax.y * 0.5F); }
        }));
        add(pause(0.5));
        add(foto(p + "mitte"));
        add(tu("AP8 Foto: unten", [] {
            if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, w->ScrollMax.y); }
        }));
        add(pause(0.5));
        add(foto(p + "unten"));
        add(tu("AP8 Foto: Spiegelmodus, ein Knochen gesperrt, Sperren-Menue", [] {
            if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
            gw().spiegelModus = true;
            modellGewichteSperren(1, 1);
            ui::rolloutZeigen(tr(Str::MgRSpiegeln));
        }));
        add(pause(0.7));
        add(foto(p + "spiegelmodus"));
        add(tu("AP8 Foto: Sperren-Menue", [] {
            ui::rolloutZeigen(tr(Str::MgRKnochen));
            const ModellAnsicht& a = mtab().ansichten[mtab().aktiv];
            setzeMaus(ImVec2{a.lo[0] + a.groesse[0] * 0.6F, a.lo[1] + a.groesse[1] * 0.3F});
            gw().sperrMenueOeffnen = true;
        }));
        add(pause(0.7));
        add(foto(p + "sperren"));
        add(mbefoto::popupsZu());
        add(tu("AP8 Foto: zurueck", [] {
            gw().spiegelModus = false;
            modellGewichteSperren(2, 0);
            setzeMaus(ImVec2{-50.0F, -50.0F});
        }));
        add(pause(0.3));
    }
}

// Bilder fuer das Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap8): h_ap8_max, h_ap8_blender, h_ap8_spiegeln - dieselbe
// Figur wie die Seite "Model-Tab: Ansehen und Pruefen" (Sturmtruppler).
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP8 Handbuch: Weights, Knochen rhumerus, BOTH_WALK1, wie 3ds Max", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = 0.85F;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Weights;
        animWaehlen(mb1::animNach("BOTH_WALK1") >= 0 ? mb1::animNach("BOTH_WALK1") : 0, false);
        modellExplorerVorgabe();
        mtab().auswahl.clear();
        modellAnsichtEinpassenAktiv();
        mtab().auswahl = {mb1::teil("torso")};
        mtab().knochenWahl = knochen("rhumerus");
        g_app->animPlaying = false;
    }));
    add(pause(1.2));
    add(hilfe2::schrittFoto("h_ap8_max"));
    add(tu("AP8 Handbuch: Spiegelmodus", [] {
        gw().spiegelModus = true;
        ui::rolloutZeigen(tr(Str::MgRSpiegeln));
    }));
    add(pause(0.9));
    add(hilfe2::schrittFoto("h_ap8_spiegeln"));
    add(tu("AP8 Handbuch: wie Blender", [] {
        gw().spiegelModus = false;
        modellBedienungSetzen(1);
        if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
    }));
    add(pause(0.9));
    add(hilfe2::schrittFoto("h_ap8_blender"));
    add(tu("AP8 Handbuch: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        mtab().bereich = ModellBereich::Ansehen;
        modellGewichteBereichSchritt();
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "ap8" && n != "ap8foto") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_AP8_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    const char* m = std::getenv("BHED_AP8_MODELL");
    const std::string modell = m != nullptr ? m : "models/players/kyle/model.glm";
    auto skala = std::make_shared<float>(1.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP8: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        if (const char* t = std::getenv("BHED_AP8_THEMA")) {
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
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap8.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    add(tu("AP8: Aufteilung wie neue Einstellungen, Sequenz, wie 3ds Max", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(0);
        mtab().vierAnsichten = false;
        animWaehlen(mbefoto::sequenz(), false);
        char zeileText[300];
        std::snprintf(zeileText, sizeof zeileText, "%s, %zu Teile, %zu Knochen, Anzeige %.0f x %.0f, Oberflaeche %.2f, Schrift %.0f Punkte hoch", g_app->modelPath.c_str(),
                      mtab().dok.inhalt().teile.size(), g_app->anim.bones.size(), static_cast<double>(ImGui::GetIO().DisplaySize.x), static_cast<double>(ImGui::GetIO().DisplaySize.y),
                      static_cast<double>(g_app->uiScale), static_cast<double>(ImGui::GetFontSize()));
        melde(zeileText);
    }));
    add(pause(1.0));
    add(klickAn("AP8: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (n == "ap8") { zeilenSchritte(s); }
    if (n == "ap8foto") { fotoSchritte(s, "ap8_"); }
    add(tu("AP8: Modell-Tab zurueck", [=] {
        alleZurueck();
        ModellGewichte neu;
        gw() = neu;
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        mtab().bereich = ModellBereich::Ansehen;
        modellGewichteBereichSchritt();
        modellAuswahlArtSetzen(AuswahlArt::Objekt);
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

}  // namespace ap8
