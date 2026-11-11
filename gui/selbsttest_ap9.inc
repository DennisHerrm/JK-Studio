// selbsttest_ap9.inc - AP9 (MBENTWURF Teil B, 11.11.): das rechte Feld des Bereichs Painting nach dem freigegebenen Entwurf
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap9 | ap9foto)
//
//   ap9       je Zeile der Abgleichtabelle (plaene/modellblock_stufe0/abgleich_programm_entwurf.md, Abschnitt 11) eine
//             Pruefung "AP9 T<nr>": Ort, Element, Bedienung - wie 3ds Max und wie Blender.
//               T0a/T0b  die Gruppen Ziel, Werkzeuge, Pinsel, Ebenen, Fuer das Spiel im Reiter Aendern (wie 3ds Max) bzw.
//                        Werkzeug (wie Blender), in dieser Reihenfolge, im Stil des Bedienmodus
//               T1       Ziel an erster Stelle: Shader als Formularzeile, Bild und Format als Tabelle, Knoepfe im Raster
//               T2       Werkzeuge im Raster mit Symbol, das gewaehlte ist der Hauptknopf
//               T3       Pinsel: Kurzinfo je Bedienmodus, Regler als Formularzeilen, Erweitert eingeklappt, Tasten [ und ]
//               T4       Ebenen: Zeilen mit Auge und Schloss, Mischart und Deckkraft der gewaehlten Ebene, Raster
//               T5       Fuer das Spiel: Hauptknopf, Ausgabeordner, Zahl der bemalten Bilder; wie 3ds Max auch im Reiter
//                        Dienstprogramme
//               T6       Tipp unter den Ebenen (kein Entwicklerhinweis)
//               T7       das 2D-Bild neben der Ansicht, ein- und ausblendbar
//               T9       wie Blender: Umschalt+F zieht die Staerke
//               T10      die kurze Zeitleiste
//             (T8, Tempo beim Malen: die Laeufe ptlang und ptsprung)
//   ap9foto   nur Fotos (BHED_FOTOS=<Ordner>): der Bereich Painting in beiden Bedienmodi, das rechte Feld in Seiten
//   BHED_AP9_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe wie der Foto-Selbsttest mbefoto)
//   BHED_AP9_MODELL=<.glm>           Vorgabe models/players/kyle/model.glm (das Modell des Entwurfs)
//   BHED_AP9_THEMA=hell|dunkel       das erste helle bzw. dunkle Farbschema (Vorgabe: das eingestellte)
// Eingebunden in selbsttest.cpp (nach selbsttest_ap3.inc); Handbuchbilder ueber BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap9.
namespace ap9 {

namespace mm = modell::texmalen;
using mbefoto::inFenster;
using mbefoto::kindFenster;

float em() { return ImGui::GetFontSize(); }

void melde(const std::string& z) { diag::detail("Knopftest: AP9 " + z); }

ModellMalen& mz() { return g_app->mtab.malen; }
const mm::Leinwand* lw() { return modellMalenLeinwand(); }

std::string titel(const char* nr, int modus, const std::string& was) { return std::string("AP9 ") + nr + (modus == 0 ? " wie 3ds Max: " : " wie Blender: ") + was; }

bool feldFrisch() { return ui::feldProbe().bild >= ImGui::GetFrameCount() - 2; }

// Die Gruppe des rechten Felds mit diesem Titel (nur gezeichnete) und ihr Platz in der Reihenfolge (-1: fehlt).
const ui::RolloutProbe* feldGruppe(Str name) {
    const std::string n = tr(name);
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
        if (ui::rolloutProbe(i).titel == n) { return &ui::rolloutProbe(i); }
    }
    return nullptr;
}

int feldPlatz(Str name) {
    const std::string n = tr(name);
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
        if (ui::rolloutProbe(i).titel == n) { return i; }
    }
    return -1;
}

// Die Titel der gezeichneten Gruppen des rechten Felds, mit | getrennt.
std::string feldGruppen() {
    std::string s;
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) { s += (i > 0 ? "|" : "") + ui::rolloutProbe(i).titel; }
    return s;
}

std::string alleGruppen() {
    return std::string(tr(Str::PtGrZiel)) + "|" + tr(Str::PtGrWerkzeuge) + "|" + tr(Str::PtGrPinsel) + "|" + tr(Str::PtGrEbenen) + "|" + tr(Str::PtGrAusgabe);
}

const Element* imRoll(const std::string& label) { return inFenster(label, "mtrollouts"); }

// Ein Regler des Bausteins: seine Kennung ist "Beschriftung###kennung".
std::string reglerName(Str name, const char* kennung) { return std::string(tr(name)) + "###" + kennung; }
const Element* reglerEl(Str name, const char* kennung) { return imRoll(reglerName(name, kennung)); }

// Ein Element des rollenden Bereichs ueber seine Kennung (Auswahllisten und Textfelder melden ihre Beschriftung nicht immer).
const Element* rollKennung(const char* kennung) {
    ImGuiWindow* w = kindFenster("mtrollouts");
    return w != nullptr ? findeKennung(w->GetID(kennung)) : nullptr;
}

bool gewaehlt(const Element* e) { return e != nullptr && (e->flags & ImGuiItemStatusFlags_Checked) != 0; }

std::string ebeneEins() {
    char b[96];
    std::snprintf(b, sizeof b, tr(Str::PtEbeneName), 1);
    return b;
}

// Die Zeile des Originals in der Ebenenliste: "Original <Datei>" wie im Entwurf.
std::string originalZeile() {
    const mm::Leinwand* l = lw();
    return std::string(tr(Str::PtEbeneOriginal)) + " " + (l != nullptr ? fileName(l->datei) : std::string());
}

std::string bemaltInfo(int n) {
    char b[96];
    std::snprintf(b, sizeof b, trN(Str::PtInfoBemalt, n), n);
    return b;
}

// Eine Gruppe an den oberen Rand des rollenden Bereichs holen (sonst liegen ihre Elemente unter dem Rand und nehmen keinen Klick).
Schritt zeigeGruppe(Str name) {
    return tu(std::string("AP9: Gruppe ins Bild rollen - ") + tr(name), [name] {
        const ui::RolloutProbe* g = feldGruppe(name);
        ImGuiWindow* w = kindFenster("mtrollouts");
        if (g != nullptr && w != nullptr) { ImGui::SetScrollY(w, std::clamp(w->Scroll.y + g->kasten.Min.y - w->InnerRect.Min.y - em() * 0.5F, 0.0F, w->ScrollMax.y)); }
    });
}

// Ein Element des rollenden Bereichs in die Mitte holen.
Schritt zeigeElement(const std::string& was, std::function<const Element*()> ziel) {
    return tu("AP9: ins Bild rollen - " + was, [ziel] {
        const Element* e = ziel();
        ImGuiWindow* w = kindFenster("mtrollouts");
        if (e != nullptr && w != nullptr) {
            ImGui::SetScrollY(w, std::clamp(w->Scroll.y + e->rect.GetCenter().y - w->InnerRect.GetCenter().y, 0.0F, w->ScrollMax.y));
        }
    });
}

Schritt nachOben() {
    return tu("AP9: rechtes Feld nach oben", [] {
        if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
    });
}

// Kein Element des rechten Felds ragt ueber den Rand des rollenden Bereichs (abgeschnittene Knoepfe und Felder).
Schritt randPruefung(const std::string& was) {
    return pruefe("AP9 Breite " + was + ": kein Element des rechten Felds ragt ueber den linken oder rechten Rand", [was] {
        ImGuiWindow* w = kindFenster("mtrollouts");
        if (w == nullptr) { return false; }
        int zahl = 0;
        int raus = 0;
        std::string erste;
        for (const Element& e : g_elemente) {
            if (e.innen.find("mtrollouts") == std::string::npos || e.rect.GetWidth() <= 0.0F) { continue; }
            // das Kindfenster selbst und sein Rollbalken sind kein Inhalt des Felds (sie reichen bis an den Rand des Fensters)
            if (e.label.find("mtrollouts") != std::string::npos || e.id == ImGui::GetWindowScrollbarID(w, ImGuiAxis_Y) || e.id == ImGui::GetWindowScrollbarID(w, ImGuiAxis_X)) {
                continue;
            }
            ++zahl;
            if (e.rect.Max.x > w->InnerClipRect.Max.x + 0.5F || e.rect.Min.x < w->InnerClipRect.Min.x - 0.5F) {
                ++raus;
                if (erste.empty()) { erste = e.label + " " + mbefoto::rechteck("", e.rect); }
            }
        }
        melde("Breite " + was + ": " + std::to_string(zahl) + " Elemente, " + std::to_string(raus) + " ueber dem Rand" + (erste.empty() ? "" : " (" + erste + ")") + ", " +
              mbefoto::rechteck("Bereich", w->InnerClipRect));
        return zahl > 10 && raus == 0;
    });
}

struct Stand {
    float radius = 0.0F;
    float radiusVorSuche = 0.0F;
    float staerke = 0.0F;
    float proj[16]{};
    int gruppe = -1;
    std::string datei;
};

// Die aktive Ansicht zeigt dasselbe wie gemerkt (kein Zoom, kein Schwenk).
bool ansichtWie(const Stand& st) {
    const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
    for (int k = 0; k < 16; ++k) {
        if (std::fabs(a.viewProj[k] - st.proj[k]) > 1e-5F * std::max(1.0F, std::fabs(st.proj[k]))) { return false; }
    }
    return true;
}

// --- Die Zeilen der Abgleichtabelle --------------------------------------------------------------------------------

void zeilenSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<Stand>();
    for (int modus = 0; modus < 2; ++modus) {
        add(tu(std::string("AP9: Bedienmodus ") + (modus == 0 ? "wie 3ds Max" : "wie Blender") + ", Painting, torso gewaehlt, eine Ansicht von vorn", [=] {
            modellBedienungSetzen(modus);
            ModellTab& t = g_app->mtab;
            t.bereich = ModellBereich::Painting;
            t.vierAnsichten = false;
            t.aktiv = 1;
            t.ansichten[1].art = AnsichtArt::Vorne;
            t.lodWahl = 1;
            g_app->animPlaying = false;
            g_app->settings.modellFeldBreite = 0.0F;
            t.auswahl = {mb1::teil("torso")};
            ModellMalen& z = mz();
            z.gruppe = -1;
            z.stufe = 0;
            z.werkzeug = static_cast<int>(mm::Werkzeug::Pinsel);
            z.editorOffen = true;
            z.radiusPx = 18.0F;
            z.pinsel.staerke = 1.0F;
            z.pinsel.haerte = 0.5F;
            z.pinsel.farbe = {1.0F, 0.0F, 0.0F, 1.0F};
            z.farbe2 = {0.0F, 0.0F, 1.0F, 1.0F};
            z.spiegeln = false;
            z.spiegelAchse = 0;
            z.aufnehmen = false;
            for (int i = 0; i < 200 && modellMalenRueckgaengig(); ++i) {}
            g_app->modelDirty = true;
        }));
        add(pause(0.6));
        add(tu("AP9: einpassen", [] { modellAnsichtEinpassenAktiv(); }));
        add(pause(0.8));
        add(nachOben());
        add(pause(0.3));

        // --- T0a / T0b: Aufbau ---
        const char* t0 = modus == 0 ? "T0a" : "T0b";
        add(pruefe(titel(t0, modus, modus == 0 ? "im Befehlsfeld ist der Reiter Aendern gewaehlt; dort stehen Ziel, Werkzeuge, Pinsel, Ebenen, Fuer das Spiel in dieser Reihenfolge"
                                                : "im Eigenschaften-Editor ist der Reiter Werkzeug gewaehlt; dort stehen Ziel, Werkzeuge, Pinsel, Ebenen, Fuer das Spiel in dieser Reihenfolge"),
                   [=] {
                       const ui::FeldProbe& f = ui::feldProbe();
                       const int soll = modus == 0 ? static_cast<int>(MbReiter::Aendern) : static_cast<int>(MbReiterBl::Werkzeug);
                       melde(std::string(t0) + " Gruppen: " + feldGruppen() + ", Reiter " + std::to_string(f.reiterAktiv));
                       return feldFrisch() && feldGruppen() == alleGruppen() && f.reiterAktiv == soll && f.reiterSenkrecht == (modus == 1);
                   }));
        add(pruefe(titel(t0, modus, modus == 0 ? "jede Gruppe ist ein Kasten im Stil wie 3ds Max (flacher Balken, eckig), untereinander ohne Ueberlappung"
                                                : "jede Gruppe ist ein Kasten im Stil wie Blender (abgerundet), untereinander ohne Ueberlappung"),
                   [=] {
                       float y = -1.0e9F;
                       int zahl = 0;
                       for (const Str n : {Str::PtGrZiel, Str::PtGrWerkzeuge, Str::PtGrPinsel, Str::PtGrEbenen, Str::PtGrAusgabe}) {
                           const ui::RolloutProbe* g = feldGruppe(n);
                           if (g == nullptr || !g->offen || g->kasten.Max.y < g->kopf.Max.y + em() || g->kasten.Min.y < y) { return false; }
                           if (modus == 0 ? (g->stil != ui::RolloutStil::Max || g->rundung > 2.0F) : (g->stil != ui::RolloutStil::Blender || g->rundung < std::floor(em() * 0.3F))) { return false; }
                           y = g->kasten.Max.y;
                           ++zahl;
                       }
                       return zahl == 5;
                   }));

        // --- T1: Ziel ---
        add(pruefe(titel("T1", modus, "die Gruppe Ziel steht an erster Stelle; ihre Kurzinfo nennt die Datei des Bilds"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrZiel);
            const mm::Leinwand* l = lw();
            if (g == nullptr || l == nullptr) { return false; }
            melde("T1 Kurzinfo: " + g->kurzinfo + " | Datei " + l->datei);
            return feldPlatz(Str::PtGrZiel) == 0 && g->kurzinfo == fileName(l->datei) && g->info.GetWidth() > 0.0F;
        }));
        add(pruefe(titel("T1", modus, "Shader als Auswahl in einer Formularzeile, darunter Bild (Datei, Groesse, bleibt so) und Format als Zeilen einer Tabelle ueber die Breite der Gruppe"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrZiel);
            const Element* shader = rollKennung("##ptgruppe");
            const Element* bild = imRoll(tr(Str::PtGrBild));
            const Element* format = imRoll(tr(Str::PtZielFormat));
            if (g == nullptr || shader == nullptr || bild == nullptr || format == nullptr) {
                melde(std::string("T1 fehlt: Gruppe ") + (g != nullptr ? "da" : "fehlt") + ", Shader " + (shader != nullptr ? "da" : "fehlt") + ", Bild " + (bild != nullptr ? "da" : "fehlt") +
                      ", Format " + (format != nullptr ? "da" : "fehlt"));
                return false;
            }
            melde("T1 Ziel: " + mbefoto::rechteck("shader", shader->rect) + " " + mbefoto::rechteck("bild", bild->rect) + " " + mbefoto::rechteck("format", format->rect) + " " +
                  mbefoto::rechteck("kasten", g->kasten));
            return g->kasten.Contains(shader->rect) && shader->rect.Min.x > g->kasten.Min.x + em() * 2.0F && g->kasten.Contains(bild->rect) && bild->rect.Min.y >= shader->rect.Max.y - 0.5F &&
                   bild->rect.GetWidth() >= g->kasten.GetWidth() - em() * 1.2F && g->kasten.Contains(format->rect) && format->rect.Min.y >= bild->rect.Max.y - 0.5F;
        }));
        add(pruefe(titel("T1", modus, "Bild zeigen und Auf neue UVs backen stehen im Raster mit zwei gleich breiten Spalten, jeder mit Symbol"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrZiel);
            const Element* zeigen = imRoll(tr(Str::PtEditorZeigen));
            const Element* backen = imRoll(tr(Str::PtUmbacken));
            if (g == nullptr || zeigen == nullptr || backen == nullptr) { return false; }
            return g->rasterSpalten == 2 && g->knoepfe == 2 && g->symbole == 2 && g->kasten.Contains(zeigen->rect) && g->kasten.Contains(backen->rect) &&
                   std::fabs(zeigen->rect.GetWidth() - backen->rect.GetWidth()) < 0.5F && zeigen->rect.Min.y == backen->rect.Min.y && backen->rect.Min.x > zeigen->rect.Max.x;
        }));
        add(tu("AP9 T1: Gruppe und Datei merken", [st] {
            st->gruppe = mz().gruppe;
            st->datei = lw() != nullptr ? lw()->datei : std::string();
        }));
        add(klickAn("AP9 T1: die Auswahl Shader oeffnen", [] {
            const Element* e = rollKennung("##ptgruppe");
            if (e == nullptr) {
                meldeFehler("AP9 T1: Auswahl Shader nicht gefunden");
                return ImVec2{};
            }
            return e->rect.GetCenter();
        }));
        add(pause(0.4));
        add(klickAn("AP9 T1: einen anderen Shader waehlen", [st] {
            // die Eintraege der offenen Liste in ihrer Reihenfolge: der erste, der nicht der gewaehlte ist
            int nr = 0;
            for (const Element& e : g_elemente) {
                if (e.fenster.find("##Combo_") == std::string::npos || e.label.empty() || e.label.rfind("##", 0) == 0) { continue; }
                if (nr != st->gruppe) { return e.rect.GetCenter(); }
                ++nr;
            }
            meldeFehler("AP9 T1: kein anderer Shader in der Liste");
            return ImVec2{};
        }));
        add(pause(0.6));
        add(pruefe(titel("T1", modus, "die Wahl eines anderen Shaders wechselt das Bild; die Kurzinfo nennt die neue Datei"), [st] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrZiel);
            const mm::Leinwand* l = lw();
            melde("T1 Wahl: Gruppe " + std::to_string(st->gruppe) + " -> " + std::to_string(mz().gruppe) + ", Datei " + st->datei + " -> " + (l != nullptr ? l->datei : std::string("-")));
            return g != nullptr && l != nullptr && mz().gruppe != st->gruppe && g->kurzinfo == fileName(l->datei);
        }));
        add(tu("AP9 T1: zurueck zur Gruppe von torso", [] {
            g_app->mtab.auswahl = {mb1::teil("torso")};
            mz().gruppe = -1;
            mz().stufe = 0;
        }));
        add(pause(0.4));

        // --- T7: das 2D-Bild neben der Ansicht ---
        add(pruefe(titel("T7", modus, "links neben der Ansicht steht das 2D-Bild der Textur"), [] {
            const ImGuiWindow* w = kindFenster("mtmaleditor");
            const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
            return w != nullptr && w->Rect().Max.x <= a.lo[0] + 1.0F && mz().leinwand[2] > 100.0F && mz().leinwand[3] > 100.0F;
        }));
        add(klick(tr(Str::PtEditorZeigen), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(titel("T7", modus, "Bild zeigen in der Gruppe Ziel blendet das 2D-Bild aus"), [] { return !mz().editorOffen && kindFenster("mtmaleditor") == nullptr && !gewaehlt(imRoll(tr(Str::PtEditorZeigen))); }));
        add(klick(tr(Str::PtEditorZeigen), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(titel("T7", modus, "ein zweiter Klick blendet es wieder ein (der Knopf ist dann gedrueckt)"), [] {
            return mz().editorOffen && kindFenster("mtmaleditor") != nullptr && gewaehlt(imRoll(tr(Str::PtEditorZeigen)));
        }));

        // --- T2: Werkzeuge ---
        add(zeigeGruppe(Str::PtGrWerkzeuge));
        add(pause(0.3));
        add(pruefe(titel("T2", modus, modus == 0 ? "die Gruppe Werkzeuge steht an zweiter Stelle; ihre Kurzinfo nennt das Vorbild Viewport Canvas"
                                                  : "die Gruppe Werkzeuge steht an zweiter Stelle; ihre Kurzinfo nennt das Vorbild Texture Paint"),
                   [=] {
                       const ui::RolloutProbe* g = feldGruppe(Str::PtGrWerkzeuge);
                       return g != nullptr && feldPlatz(Str::PtGrWerkzeuge) == 1 && g->kurzinfo == tr(modus == 0 ? Str::PtRolloutMax : Str::PtRolloutBlender);
                   }));
        add(pruefe(titel("T2", modus, "sieben Werkzeuge im Raster mit zwei gleich breiten Spalten, jedes mit Symbol; zuerst die fuenf des Entwurfs: Malen, Fuellen, Klonen, Verwischen, Maske"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrWerkzeuge);
            if (g == nullptr) { return false; }
            static const Str kFolge[7] = {Str::PtWzPinsel, Str::PtWzFuellen, Str::PtWzKlonen, Str::PtWzWischen, Str::PtWzMaske, Str::PtWzWeich, Str::PtWzRadierer};
            const Element* e[7];
            for (int i = 0; i < 7; ++i) {
                e[i] = imRoll(tr(kFolge[i]));
                if (e[i] == nullptr || !g->kasten.Contains(e[i]->rect)) {
                    melde(std::string("T2 Werkzeug fehlt oder liegt ausserhalb der Gruppe: ") + tr(kFolge[i]));
                    return false;
                }
            }
            bool ok = g->rasterSpalten == 2 && g->knoepfe == 7 && g->symbole == 7;
            for (int i = 0; i < 7; ++i) {
                ok = ok && std::fabs(e[i]->rect.GetWidth() - e[0]->rect.GetWidth()) < 0.5F && std::fabs(e[i]->rect.Min.x - e[i % 2]->rect.Min.x) < 0.5F;
                if (i % 2 == 1) { ok = ok && e[i]->rect.Min.y == e[i - 1]->rect.Min.y && e[i]->rect.Min.x > e[i - 1]->rect.Max.x; }
                if (i >= 2) { ok = ok && e[i]->rect.Min.y >= e[i - 2]->rect.Max.y; }
            }
            melde("T2 Raster: Spalten " + std::to_string(g->rasterSpalten) + ", Knoepfe " + std::to_string(g->knoepfe) + ", Symbole " + std::to_string(g->symbole) + ", " +
                  mbefoto::rechteck("Malen", e[0]->rect) + " " + mbefoto::rechteck("Fuellen", e[1]->rect) + " " + mbefoto::rechteck("Radieren", e[6]->rect));
            return ok;
        }));
        add(pruefe(titel("T2", modus, "das gewaehlte Werkzeug (Malen) ist der eine Hauptknopf der Gruppe"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrWerkzeuge);
            return g != nullptr && g->hauptKnoepfe == 1 && gewaehlt(imRoll(tr(Str::PtWzPinsel))) && !gewaehlt(imRoll(tr(Str::PtWzFuellen)));
        }));
        add(klick(tr(Str::PtWzFuellen), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(titel("T2", modus, "ein Klick auf Fuellen waehlt das Werkzeug, der Hauptknopf wandert mit"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrWerkzeuge);
            return g != nullptr && mz().werkzeug == static_cast<int>(mm::Werkzeug::Fuellen) && g->hauptKnoepfe == 1 && gewaehlt(imRoll(tr(Str::PtWzFuellen))) &&
                   !gewaehlt(imRoll(tr(Str::PtWzPinsel)));
        }));

        // --- T3: Pinsel ---
        add(pruefe(titel("T3", modus, "mit dem Werkzeug Fuellen steht in der Gruppe Pinsel der Regler Toleranz"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrPinsel);
            const Element* tol = reglerEl(Str::PtToleranz, "pttol");
            return g != nullptr && tol != nullptr && g->kasten.Contains(tol->rect) && imRoll(tr(Str::PtMaskeFrei)) == nullptr;
        }));
        add(klick(tr(Str::PtWzMaske), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(titel("T3", modus, "mit dem Werkzeug Maske steht dort der Haken Freigeben statt der Toleranz"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrPinsel);
            const Element* frei = imRoll(tr(Str::PtMaskeFrei));
            return g != nullptr && frei != nullptr && g->kasten.Contains(frei->rect) && reglerEl(Str::PtToleranz, "pttol") == nullptr;
        }));
        add(klick(tr(Str::PtWzPinsel), "mtrollouts"));
        add(pause(0.4));
        add(zeigeGruppe(Str::PtGrPinsel));
        add(pause(0.3));
        add(pruefe(titel("T3", modus, modus == 0 ? "die Gruppe Pinsel steht an dritter Stelle; ihre Kurzinfo nennt die Tasten [ ] fuer die Groesse"
                                                  : "die Gruppe Pinsel steht an dritter Stelle; ihre Kurzinfo nennt F Radius und Umschalt+F Staerke"),
                   [=] {
                       const ui::RolloutProbe* g = feldGruppe(Str::PtGrPinsel);
                       if (g == nullptr) { return false; }
                       melde("T3 Kurzinfo: " + g->kurzinfo);
                       return feldPlatz(Str::PtGrPinsel) == 2 && g->kurzinfo == tr(modus == 0 ? Str::PtInfoMax : Str::PtInfoBlender) && g->info.GetWidth() > 0.0F &&
                              reglerEl(Str::PtToleranz, "pttol") == nullptr && imRoll(tr(Str::PtMaskeFrei)) == nullptr;
                   }));
        add(pruefe(titel("T3", modus, "Farbe (zwei Farbfelder, Tauschen, Aufnehmen), Radius, Staerke, Abstand als Formularzeilen untereinander; die Felder beginnen an derselben Kante hinter der Beschriftung"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrPinsel);
            const Element* tausch = imRoll(tr(Str::PtFarbenTauschen));
            const Element* nimm = imRoll(tr(Str::PtFarbeAufnehmen));
            const Element* radius = reglerEl(Str::PtRadius, "ptradius");
            const Element* staerke = reglerEl(Str::PtStaerke, "ptstaerke");
            const Element* abstand = reglerEl(Str::PtAbstand, "ptabstand");
            if (g == nullptr || tausch == nullptr || nimm == nullptr || radius == nullptr || staerke == nullptr || abstand == nullptr) {
                melde(std::string("T3 fehlt: Tauschen ") + (tausch != nullptr ? "da" : "fehlt") + ", Aufnehmen " + (nimm != nullptr ? "da" : "fehlt") + ", Radius " +
                      (radius != nullptr ? "da" : "fehlt") + ", Staerke " + (staerke != nullptr ? "da" : "fehlt") + ", Abstand " + (abstand != nullptr ? "da" : "fehlt"));
                return false;
            }
            melde("T3 Zeilen: " + mbefoto::rechteck("tauschen", tausch->rect) + " " + mbefoto::rechteck("radius", radius->rect) + " " + mbefoto::rechteck("staerke", staerke->rect) + " " +
                  mbefoto::rechteck("abstand", abstand->rect) + " " + mbefoto::rechteck("kasten", g->kasten));
            return g->kasten.Contains(tausch->rect) && g->kasten.Contains(nimm->rect) && g->kasten.Contains(radius->rect) && g->kasten.Contains(staerke->rect) && g->kasten.Contains(abstand->rect) &&
                   tausch->rect.Min.y == nimm->rect.Min.y && nimm->rect.Min.x >= tausch->rect.Max.x && tausch->rect.Max.y <= radius->rect.Min.y + 0.5F && radius->rect.Max.y <= staerke->rect.Min.y + 0.5F &&
                   staerke->rect.Max.y <= abstand->rect.Min.y + 0.5F && std::fabs(radius->rect.Min.x - staerke->rect.Min.x) < 0.5F && std::fabs(radius->rect.Min.x - abstand->rect.Min.x) < 0.5F &&
                   std::fabs(radius->rect.GetWidth() - abstand->rect.GetWidth()) < 0.5F && radius->rect.Min.x > g->kasten.Min.x + em() * 2.0F;
        }));
        if (modus == 1) {
            // Der Max-Durchgang hat Erweitert aufgeklappt (der Zustand gehoert dem rollenden Bereich, nicht dem Bedienmodus):
            // zuklappen wie beim Start, damit der Blender-Durchgang dieselben Schritte prueft. Max prueft den Werkszustand.
            add(tu("AP9 T3: Erweitert zuklappen (offen aus dem Max-Durchgang)", [] {
                const Element* mehr = imRoll(reglerName(Str::PtErweitert, "ptmehr"));
                if (ImGuiWindow* w = kindFenster("mtrollouts"); w != nullptr && mehr != nullptr) { w->StateStorage.SetInt(mehr->id, 0); }
            }));
            add(pause(0.3));
        }
        add(pruefe(titel("T3", modus, modus == 0 ? "Erweitert ist ab Werk zugeklappt: Haerte, Spiegeln, Stiftdruck, Rueckseiten und Ausbluten belegen keinen Platz"
                                                 : "Erweitert ist zugeklappt: Haerte, Spiegeln, Stiftdruck, Rueckseiten und Ausbluten belegen keinen Platz"), [] {
            const Element* mehr = imRoll(reglerName(Str::PtErweitert, "ptmehr"));
            return mehr != nullptr && (mehr->flags & ImGuiItemStatusFlags_Openable) != 0 && (mehr->flags & ImGuiItemStatusFlags_Opened) == 0 && reglerEl(Str::PtHaerte, "pthaerte") == nullptr &&
                   imRoll(tr(Str::PtSpiegeln)) == nullptr && imRoll(tr(Str::PtRueckseiten)) == nullptr;
        }));
        add(klick(reglerName(Str::PtErweitert, "ptmehr"), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(titel("T3", modus, "ein Klick auf Erweitert zeigt Haerte, Spiegeln mit Achse X/Y/Z, Stiftdruck fuer Groesse und Staerke, Rueckseiten und Ausbluten - alles im Kasten der Gruppe"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrPinsel);
            if (g == nullptr) { return false; }
            const Element* bluten = rollKennung("##ptausbluten");
            bool ok = bluten != nullptr && g->kasten.Contains(bluten->rect);
            for (const Element* e : {reglerEl(Str::PtHaerte, "pthaerte"), imRoll(tr(Str::PtSpiegeln)), imRoll("X"), imRoll("Y"), imRoll("Z"), imRoll(tr(Str::PtDruckGroesse)), imRoll(tr(Str::PtDruckStaerke)),
                                     imRoll(tr(Str::PtRueckseiten))}) {
                ok = ok && e != nullptr && g->kasten.Contains(e->rect);
            }
            return ok;
        }));
        add(klick(tr(Str::PtSpiegeln), "mtrollouts"));
        add(klick("Y", "mtrollouts"));
        add(pause(0.3));
        add(pruefe(titel("T3", modus, "der Haken Spiegeln und die Wahl der Achse wirken (Spiegeln an, Achse Y)"), [] { return mz().spiegeln && mz().spiegelAchse == 1 && gewaehlt(imRoll(tr(Str::PtSpiegeln))) && gewaehlt(imRoll("Y")); }));
        add(tu("AP9 T3: Spiegeln aus, Achse X", [] {
            mz().spiegeln = false;
            mz().spiegelAchse = 0;
        }));
        add(randPruefung(std::string(modus == 0 ? "wie 3ds Max" : "wie Blender") + ", Erweitert offen"));
        add(klick(tr(Str::PtFarbenTauschen), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(titel("T3", modus, "der Knopf Farben tauschen neben den Farbfeldern tauscht Pinselfarbe und zweite Farbe"), [] {
            return mz().pinsel.farbe[2] > 0.99F && mz().pinsel.farbe[0] < 0.01F && mz().farbe2[0] > 0.99F;
        }));
        add(klick(tr(Str::PtFarbenTauschen), "mtrollouts"));
        add(klick(tr(Str::PtFarbeAufnehmen), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(titel("T3", modus, "der Knopf Farbe aufnehmen stellt die Aufnahme scharf (der naechste Klick nimmt die Farbe)"), [] { return mz().aufnehmen && mz().pinsel.farbe[0] > 0.99F; }));
        add(tu("AP9 T3: Aufnahme aus, Fokus im Model-Tab, Radius 20", [st] {
            mz().aufnehmen = false;
            mz().radiusPx = 20.0F;
            st->radius = 20.0F;
            g_app->mtab.fokus = true;
        }));
        add(klickAn("AP9 T3: Maus in die Ansicht (Mitteltaste, malt nicht)", [] { return pt1c::torsoMitte(0.0F); }, 2));
        add(pause(0.3));
        add(tu("AP9 T3: Ansicht merken", [st] {
            const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
            for (int k = 0; k < 16; ++k) { st->proj[k] = a.viewProj[k]; }
        }));
        add(taste(ImGuiKey_RightBracket, "]"));
        add(pause(0.3));
        add(pruefe(titel("T3", modus, "die Taste ] macht den Pinsel einen Schritt groesser (Radius geteilt durch 0,9); die Ansicht zoomt dabei nicht"), [st] {
            char z[160];
            std::snprintf(z, sizeof z, "T3 ]: Radius %.3f -> %.3f, Ansicht %s", static_cast<double>(st->radius), static_cast<double>(mz().radiusPx), ansichtWie(*st) ? "gleich" : "geaendert");
            melde(z);
            return std::fabs(mz().radiusPx - st->radius / 0.9F) < 0.01F && ansichtWie(*st);
        }));
        add(taste(ImGuiKey_LeftBracket, "["));
        add(pause(0.2));
        add(taste(ImGuiKey_LeftBracket, "["));
        add(pause(0.3));
        add(pruefe(titel("T3", modus, "die Taste [ macht ihn einen Schritt kleiner (zweimal: Radius mal 0,9); die Ansicht zoomt dabei nicht"), [st] {
            char z[160];
            std::snprintf(z, sizeof z, "T3 [ [: Radius %.3f, Ansicht %s", static_cast<double>(mz().radiusPx), ansichtWie(*st) ? "gleich" : "geaendert");
            melde(z);
            return std::fabs(mz().radiusPx - st->radius * 0.9F) < 0.01F && ansichtWie(*st);
        }));
        add(pruefe(titel("T3", modus, "Pinsel kleiner und Pinsel groesser stehen bei den Aktionen des Pinsels mit ihrer Taste [ bzw. ] und einem Tooltip"), [=] {
            int gefunden = 0;
            for (int i = 0; i < modellMalenAktionZahl(); ++i) {
                const std::string text = modellMalenAktionText(i);
                const bool kleiner = text == tr(Str::PtPinselKleiner);
                const bool groesser = text == tr(Str::PtPinselGroesser);
                if (!kleiner && !groesser) { continue; }
                if (modellMalenAktionGruppe(i) == 1 && std::string(modellMalenAktionTaste(i, modus == 1)) == (kleiner ? "[" : "]") && std::string(modellMalenAktionHinweis(i)).size() >= 10U) { ++gefunden; }
            }
            return gefunden == 2;
        }));
        add(tu("AP9 T3: Radius vor der Suche merken", [st] { st->radiusVorSuche = mz().radiusPx; }));
        pt1c::ueberSuche(s, Str::PtPinselGroesser);
        add(pruefe(titel("T3", modus, "Pinsel groesser in Alle Aktionen suchen macht den Pinsel einen Schritt groesser"), [st] {
            return std::fabs(mz().radiusPx - st->radiusVorSuche / 0.9F) < 0.01F;
        }));

        // --- T9: Tasten wie Blender ---
        if (modus == 1) {
            add(tu("AP9 T9: Fokus im Model-Tab, Staerke merken", [st] {
                g_app->mtab.fokus = true;
                st->staerke = mz().pinsel.staerke;
            }));
            add(klickAn("AP9 T9: Maus in die Ansicht (Mitteltaste, malt nicht)", [] { return pt1c::torsoMitte(0.0F); }, 2));
            add(pause(0.3));
            add(tasteKombi(ImGuiMod_Shift | ImGuiKey_F, "Umschalt+F"));
            add(pause(0.3));
            add(pruefe(titel("T9", modus, "Umschalt+F zieht die Staerke des Pinsels (wie die Kurzinfo der Gruppe Pinsel sagt)"), [] { return mz().modalArt == 2; }));
            add(taste(ImGuiKey_Escape, "Esc"));
            add(pause(0.3));
            add(pruefe(titel("T9", modus, "Esc bricht ab, die Staerke bleibt"), [st] { return mz().modalArt == 0 && mz().pinsel.staerke == st->staerke; }));
        }

        // --- T4: Ebenen ---
        add(zeigeGruppe(Str::PtGrEbenen));
        add(pause(0.3));
        add(pruefe(titel("T4", modus, "die Gruppe Ebenen steht an vierter Stelle, Kurzinfo ist die Zahl der Ebenen (1); die Zeile des Originals geht ueber die Breite und traegt rechts das Schloss"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrEbenen);
            const Element* original = imRoll(originalZeile());
            const Element* schloss = imRoll("##ptschloss");
            if (g == nullptr || original == nullptr || schloss == nullptr) {
                melde(std::string("T4 fehlt: Gruppe ") + (g != nullptr ? "da" : "fehlt") + ", Zeile Original " + (original != nullptr ? "da" : "fehlt") + ", Schloss " + (schloss != nullptr ? "da" : "fehlt"));
                return false;
            }
            melde("T4 Original: " + mbefoto::rechteck("zeile", original->rect) + " " + mbefoto::rechteck("schloss", schloss->rect) + " " + mbefoto::rechteck("kasten", g->kasten) + ", Kurzinfo " + g->kurzinfo);
            return feldPlatz(Str::PtGrEbenen) == 3 && g->kurzinfo == "1" && g->kasten.Contains(original->rect) && original->rect.GetWidth() >= g->kasten.GetWidth() - em() * 1.2F &&
                   original->rect.Contains(schloss->rect) && schloss->rect.Max.x >= original->rect.Max.x - em() * 1.6F && gewaehlt(original);
        }));
        add(pruefe(titel("T4", modus, "Neue Ebene und Nach unten vereinen stehen als erste Reihe im Raster mit zwei Spalten, mit Symbol; darunter Ebene hoch, Ebene runter, Ebene loeschen, Maske zeigen, Maske leeren, Maske umkehren"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrEbenen);
            const Element* neu = imRoll(tr(Str::PtEbeneNeu));
            const Element* vereinen = imRoll(tr(Str::PtEbeneVereinen));
            if (g == nullptr || neu == nullptr || vereinen == nullptr) { return false; }
            bool ok = g->rasterSpalten == 2 && g->knoepfe == 8 && g->symbole == 8 && neu->rect.Min.y == vereinen->rect.Min.y && vereinen->rect.Min.x > neu->rect.Max.x &&
                      std::fabs(neu->rect.GetWidth() - vereinen->rect.GetWidth()) < 0.5F && g->kasten.Contains(neu->rect) && g->kasten.Contains(vereinen->rect);
            for (const Str n : {Str::PtEbeneHoch, Str::PtEbeneRunter, Str::PtEbeneLoeschen, Str::PtMaskeAn, Str::PtMaskeLeeren, Str::PtMaskeUmkehren}) {
                const Element* e = imRoll(tr(n));
                ok = ok && e != nullptr && g->kasten.Contains(e->rect) && e->rect.Min.y >= neu->rect.Max.y;
            }
            return ok;
        }));
        add(klick(tr(Str::PtEbeneNeu), "mtrollouts"));
        add(pause(0.5));
        add(pruefe(titel("T4", modus, "Neue Ebene: die neue Zeile steht gewaehlt ueber dem Original, rechts mit dem Auge; die Kurzinfo zaehlt 2"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrEbenen);
            const mm::Leinwand* l = lw();
            const Element* neu = imRoll(ebeneEins());
            const Element* original = imRoll(originalZeile());
            const Element* auge = imRoll(tr(Str::PtEbeneSicht));
            if (g == nullptr || l == nullptr || neu == nullptr || original == nullptr || auge == nullptr) {
                melde(std::string("T4 fehlt: Zeile ") + ebeneEins() + " " + (neu != nullptr ? "da" : "fehlt") + ", Original " + (original != nullptr ? "da" : "fehlt") + ", Auge " + (auge != nullptr ? "da" : "fehlt"));
                return false;
            }
            melde("T4 neu: " + mbefoto::rechteck("zeile", neu->rect) + " " + mbefoto::rechteck("auge", auge->rect) + " " + mbefoto::rechteck("original", original->rect));
            return l->ebenen.size() == 2U && l->aktiv == 1 && g->kurzinfo == "2" && neu->rect.Max.y <= original->rect.Min.y + 0.5F && gewaehlt(neu) && !gewaehlt(original) &&
                   neu->rect.Contains(auge->rect) && auge->rect.Max.x >= neu->rect.Max.x - em() * 1.6F && gewaehlt(auge);
        }));
        add(klick(tr(Str::PtEbeneSicht), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(titel("T4", modus, "ein Klick auf das Auge blendet die Ebene aus (die Zeile bleibt gewaehlt)"), [] {
            const mm::Leinwand* l = lw();
            return l != nullptr && l->ebenen.size() == 2U && !l->ebenen[1].sichtbar && l->aktiv == 1 && !gewaehlt(imRoll(tr(Str::PtEbeneSicht)));
        }));
        add(klick(tr(Str::PtEbeneSicht), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(titel("T4", modus, "ein zweiter Klick blendet sie wieder ein"), [] {
            return lw() != nullptr && lw()->ebenen.size() == 2U && lw()->ebenen[1].sichtbar && gewaehlt(imRoll(tr(Str::PtEbeneSicht)));
        }));
        add(klickAn("AP9 T4: die Zeile des Originals", [] {
            const Element* e = imRoll(originalZeile());
            if (e == nullptr) {
                meldeFehler("AP9 T4: Zeile des Originals nicht gefunden");
                return ImVec2{};
            }
            return e->rect.GetCenter();
        }));
        add(pause(0.3));
        add(pruefe(titel("T4", modus, "ein Klick auf die Zeile des Originals waehlt es zum Malen"), [] { return lw() != nullptr && lw()->aktiv == 0 && gewaehlt(imRoll(originalZeile())); }));
        add(klick(ebeneEins(), "mtrollouts"));
        add(pause(0.3));
        add(pruefe(titel("T4", modus, "ein Klick auf die Zeile der Ebene waehlt sie wieder"), [] { return lw() != nullptr && lw()->aktiv == 1; }));
        add(pruefe(titel("T4", modus, "unter der Liste stehen Mischart und Deckkraft der gewaehlten Ebene als Formularzeilen, ueber dem Raster der Knoepfe"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrEbenen);
            const Element* misch = rollKennung("##ptmisch");
            const Element* deck = reglerEl(Str::PtDeckkraft, "ptdeck");
            const Element* original = imRoll(originalZeile());
            const Element* neu = imRoll(tr(Str::PtEbeneNeu));
            if (g == nullptr || misch == nullptr || deck == nullptr || original == nullptr || neu == nullptr) {
                melde(std::string("T4 fehlt: Mischart ") + (misch != nullptr ? "da" : "fehlt") + ", Deckkraft " + (deck != nullptr ? "da" : "fehlt"));
                return false;
            }
            return g->kasten.Contains(misch->rect) && g->kasten.Contains(deck->rect) && misch->rect.Min.y >= original->rect.Max.y - 0.5F && deck->rect.Min.y >= misch->rect.Max.y - 0.5F &&
                   neu->rect.Min.y >= deck->rect.Max.y - 0.5F && std::fabs(misch->rect.Min.x - deck->rect.Min.x) < 0.5F && misch->rect.Min.x > g->kasten.Min.x + em() * 2.0F;
        }));
        add(klickAn("AP9 T4: die Auswahl Mischart oeffnen", [] {
            const Element* e = rollKennung("##ptmisch");
            if (e == nullptr) {
                meldeFehler("AP9 T4: Auswahl Mischart nicht gefunden");
                return ImVec2{};
            }
            return e->rect.GetCenter();
        }));
        add(pause(0.4));
        add(klick(tr(Str::PtMischMult)));
        add(pause(0.5));
        add(pruefe(titel("T4", modus, "die Mischart Multiplizieren gilt fuer die gewaehlte Ebene; ihre Zeile nennt sie in Klammern"), [] {
            const mm::Leinwand* l = lw();
            const std::string zeilenText = ebeneEins() + " (" + tr(Str::PtMischMult) + ")";
            return l != nullptr && l->ebenen.size() == 2U && l->ebenen[1].misch == mm::Mischart::Multiplizieren && imRoll(zeilenText) != nullptr;
        }));
        add(zeigeElement("Deckkraft", [] { return reglerEl(Str::PtDeckkraft, "ptdeck"); }));
        add(pause(0.3));
        add(klickAn("AP9 T4: Klick auf die Mitte des Reglers Deckkraft", [] {
            const Element* e = reglerEl(Str::PtDeckkraft, "ptdeck");
            if (e == nullptr) {
                meldeFehler("AP9 T4: Regler Deckkraft nicht gefunden");
                return ImVec2{};
            }
            return e->rect.GetCenter();
        }));
        add(pause(0.4));
        add(pruefe(titel("T4", modus, "der Regler Deckkraft setzt die Deckkraft der gewaehlten Ebene (Klick in die Mitte: etwa 50 %)"), [] {
            const float d = lw() != nullptr && lw()->ebenen.size() == 2U ? lw()->ebenen[1].deckkraft : -1.0F;
            melde("T4 Deckkraft nach dem Klick: " + std::to_string(d));
            return d > 0.3F && d < 0.7F;
        }));

        // --- T6: Tipp ---
        add(tu("AP9 T6: ans Ende der Gruppe Ebenen rollen", [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrEbenen);
            ImGuiWindow* w = kindFenster("mtrollouts");
            if (g != nullptr && w != nullptr) { ImGui::SetScrollY(w, std::clamp(w->Scroll.y + g->kasten.Max.y - w->InnerRect.Max.y + em() * 6.0F, 0.0F, w->ScrollMax.y)); }
        }));
        add(pause(0.3));
        add(pruefe(titel("T6", modus, "unter der Gruppe Ebenen steht wie im Entwurf ein Tipp als eigener Kasten ueber die Breite (fuer Nutzer; der Entwicklerhinweis des Entwurfs steht nirgends)"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrEbenen);
            const ui::RolloutProbe* danach = feldGruppe(Str::PtGrAusgabe);
            const Element* tipp = imRoll(tr(Str::PtEbenenTipp));
            if (g == nullptr || danach == nullptr || tipp == nullptr) { return false; }
            for (const Element& e : g_elemente) {
                if (e.label.find("WhiteoutFlakes") != std::string::npos) { return false; }
            }
            melde("T6 Tipp: " + mbefoto::rechteck("tipp", tipp->rect) + " " + mbefoto::rechteck("ebenen", g->kasten) + " " + mbefoto::rechteck("ausgabe", danach->kasten));
            return tipp->rect.Min.y >= g->kasten.Max.y && tipp->rect.Max.y <= danach->kasten.Min.y && tipp->rect.GetWidth() >= g->kasten.GetWidth() - 1.0F;
        }));
        add(randPruefung(std::string(modus == 0 ? "wie 3ds Max" : "wie Blender") + ", zwei Ebenen"));
        add(zeigeElement("Nach unten vereinen", [] { return imRoll(tr(Str::PtEbeneVereinen)); }));
        add(pause(0.3));
        add(klick(tr(Str::PtEbeneVereinen), "mtrollouts"));
        add(pause(0.5));
        add(pruefe(titel("T4", modus, "Nach unten vereinen laesst eine Ebene; die Kurzinfo zaehlt wieder 1"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrEbenen);
            return g != nullptr && lw() != nullptr && lw()->ebenen.size() == 1U && g->kurzinfo == "1";
        }));
        add(tu("AP9 T4: zurueck auf das Original", [] {
            for (int i = 0; i < 200 && modellMalenRueckgaengig(); ++i) {}
        }));
        add(pause(0.4));

        // --- T5: Fuer das Spiel ---
        add(zeigeGruppe(Str::PtGrAusgabe));
        add(pause(0.3));
        add(pruefe(titel("T5", modus, "Fuer das Spiel ist die letzte Gruppe: Ausgabeordner als Formularzeile mit Feld und Knopf ..., der Hauptknopf Fuer das Spiel bauen ueber die Breite, darunter Bemalte Bilder als Tabellenzeile; die Kurzinfo zaehlt die bemalten Bilder (0)"),
                   [] {
                       const ui::RolloutProbe* g = feldGruppe(Str::PtGrAusgabe);
                       const Element* ordner = rollKennung("##ptordner");
                       const Element* wahl = imRoll("...##ptordnerwahl");
                       const Element* bauen = imRoll(tr(Str::PtBauen));
                       const Element* bemalt = imRoll(tr(Str::PtBemalt));
                       if (g == nullptr || ordner == nullptr || wahl == nullptr || bauen == nullptr || bemalt == nullptr) {
                           melde(std::string("T5 fehlt: Gruppe ") + (g != nullptr ? "da" : "fehlt") + ", Ordner " + (ordner != nullptr ? "da" : "fehlt") + ", ... " + (wahl != nullptr ? "da" : "fehlt") +
                                 ", Bauen " + (bauen != nullptr ? "da" : "fehlt") + ", Bemalte Bilder " + (bemalt != nullptr ? "da" : "fehlt"));
                           return false;
                       }
                       melde("T5: " + mbefoto::rechteck("ordner", ordner->rect) + " " + mbefoto::rechteck("wahl", wahl->rect) + " " + mbefoto::rechteck("bauen", bauen->rect) + " " +
                             mbefoto::rechteck("bemalt", bemalt->rect) + ", Kurzinfo " + g->kurzinfo);
                       return feldPlatz(Str::PtGrAusgabe) == ui::rolloutProbeZahl() - 1 && g->hauptKnoepfe == 1 && g->kasten.Contains(ordner->rect) && g->kasten.Contains(wahl->rect) &&
                              wahl->rect.Min.x >= ordner->rect.Max.x && wahl->rect.Min.y < ordner->rect.Max.y && wahl->rect.Max.y > ordner->rect.Min.y && g->kasten.Contains(bauen->rect) &&
                              bauen->rect.GetWidth() >= g->kasten.GetWidth() - em() * 1.2F && bauen->rect.Min.y >= ordner->rect.Max.y && g->kasten.Contains(bemalt->rect) &&
                              bemalt->rect.Min.y >= bauen->rect.Max.y && g->kurzinfo == bemaltInfo(0);
                   }));
        if (modus == 0) {
            add(klick(tr(Str::MfDienste), "mtbefehlsfeld"));
            add(pause(0.5));
            add(pruefe(titel("T5", modus, "der Reiter Dienstprogramme zeigt Fuer das Spiel (und nur diese Gruppe)"), [] {
                melde("T5 Dienstprogramme: " + feldGruppen());
                return feldGruppen() == tr(Str::PtGrAusgabe) && imRoll(tr(Str::PtBauen)) != nullptr;
            }));
            add(klick(tr(Str::MfAendern), "mtbefehlsfeld"));
            add(pause(0.5));
            add(pruefe(titel("T5", modus, "zurueck im Reiter Aendern stehen wieder alle fuenf Gruppen"), [] { return feldGruppen() == alleGruppen(); }));
            add(zeigeGruppe(Str::PtGrAusgabe));
            add(pause(0.3));
        }
        add(tu("AP9 T5: Ausgabe im Laufordner", [] {
            std::snprintf(mz().ordner, sizeof mz().ordner, "%s", mb3::lauf("ap9_mod").c_str());
            std::error_code ec;
            std::filesystem::remove_all(std::filesystem::path(mb3::lauf("ap9_mod")), ec);
        }));
        add(pause(0.2));
        add(klick(tr(Str::PtBauen), "mtrollouts"));
        add(pause(0.4));
        add(pruefe(titel("T5", modus, "der Hauptknopf baut: unbemalt schreibt er nichts und meldet das"), [] {
            std::error_code ec;
            return mz().gebaut == 0 && !std::filesystem::exists(std::filesystem::path(mb3::lauf("ap9_mod")), ec) && mz().meldung == tr(Str::PtNichtsBemalt);
        }));
        add(mb3::ziehenVonNach("AP9 T5: Strich ueber die Brust", [] { return pt1c::torsoMitte(-30.0F); }, [] { return pt1c::torsoMitte(30.0F); }));
        add(pause(0.5));
        add(pruefe(titel("T5", modus, "nach einem Strich zaehlt die Kurzinfo ein bemaltes Bild"), [] {
            const ui::RolloutProbe* g = feldGruppe(Str::PtGrAusgabe);
            if (g == nullptr) { return false; }
            melde("T5 Kurzinfo nach dem Strich: " + g->kurzinfo);
            return g->kurzinfo == bemaltInfo(1);
        }));
        add(klick(tr(Str::PtBauen), "mtrollouts"));
        add(pause(0.5));
        add(pruefe(titel("T5", modus, "der Hauptknopf schreibt das bemalte Bild unter seinem Spielpfad in den Ausgabeordner"), [] {
            const mm::Leinwand* l = lw();
            if (l == nullptr) { return false; }
            std::error_code ec;
            const bool da = std::filesystem::exists(std::filesystem::path(mb3::lauf("ap9_mod")) / l->datei, ec);
            melde("T5 gebaut: " + std::to_string(mz().gebaut) + " Bild(er), Datei " + (da ? "da" : "fehlt") + " | " + mz().meldung);
            return mz().gebaut == 1 && da;
        }));
        add(tu("AP9 T5: Ausgabe leeren, zurueck auf das Original", [] {
            std::error_code ec;
            std::filesystem::remove_all(std::filesystem::path(mb3::lauf("ap9_mod")), ec);
            mz().ordner[0] = '\0';
            for (int i = 0; i < 200 && modellMalenRueckgaengig(); ++i) {}
        }));
        add(pause(0.3));

        // --- T10: kurze Zeitleiste ---
        add(pruefe(titel("T10", modus, "im Bereich Painting steht unten die kurze Zeitleiste"), [] { return modellZeitFassung() == ZeitFassung::Kurz && modellZeit().probe.bild >= ImGui::GetFrameCount() - 2; }));
        add(nachOben());
        add(pause(0.2));
    }
}

// --- Fotos: der Bereich Painting in beiden Bedienmodi, das rechte Feld in Seiten ------------------------------------------

void fotoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const std::string p = modus == 0 ? "ap9_max_" : "ap9_blender_";
        add(tu("AP9 Foto: " + p + "paint", [=] {
            modellBedienungSetzen(modus);
            ModellTab& t = g_app->mtab;
            t.bereich = ModellBereich::Painting;
            t.vierAnsichten = false;
            t.aktiv = 1;
            t.ansichten[1].art = AnsichtArt::Vorne;
            t.lodWahl = 1;
            g_app->animPlaying = false;
            g_app->settings.modellFeldBreite = 0.0F;
            t.auswahl = {mb1::teil("torso")};
            ModellMalen& z = mz();
            z.gruppe = -1;
            z.stufe = 0;
            z.werkzeug = static_cast<int>(mm::Werkzeug::Pinsel);
            z.editorOffen = true;
            z.radiusPx = 14.0F;
            z.pinsel.staerke = 0.8F;
            z.pinsel.farbe = {0.91F, 0.69F, 0.29F, 1.0F};   // die Farbe des Entwurfs (#e8b04a)
            for (int i = 0; i < 200 && modellMalenRueckgaengig(); ++i) {}
            g_app->modelDirty = true;
        }));
        add(pause(0.6));
        add(tu("AP9 Foto: die ganze Figur einpassen (wie das Entwurfsbild), dann torso waehlen", [] {
            g_app->mtab.auswahl.clear();
            modellAnsichtEinpassenAktiv();
            g_app->mtab.auswahl = {mb1::teil("torso")};
            mz().gruppe = -1;
        }));
        add(pause(0.8));
        // wie im Entwurf: eine Ebene ueber dem Original, darauf ein Strich ueber die Brust
        add(pt1c::ausloesen(Str::PtEbeneNeu));
        add(pause(0.3));
        add(mb3::ziehenVonNach("AP9 Foto: Strich ueber die Brust", [] { return pt1c::torsoMitte(-40.0F); }, [] { return pt1c::torsoMitte(40.0F); }));
        add(tu("AP9 Foto: Maus auf die Brust", [] { setzeMaus(pt1c::torsoMitte(30.0F)); }));
        add(nachOben());
        add(pause(0.8));
        add(foto(p + "paint"));
        add(mbefoto::gruppenAuf("mtrollouts"));
        add(pause(0.4));
        for (int k = 0; k < 6; ++k) { add(mbefoto::seitenFoto(p + "paint_feld" + std::to_string(k), "mtrollouts", k)); }
        add(nachOben());
        if (modus == 0) {
            add(tu("AP9 Foto: Reiter Dienstprogramme", [] { modellFeldReiter(MbReiter::Dienste, MbReiterBl::Werkzeug); }));
            add(pause(0.5));
            add(foto(p + "paint_dienste"));
            add(tu("AP9 Foto: zurueck in den Reiter Aendern", [] { modellFeldReiter(MbReiter::Aendern, MbReiterBl::Werkzeug); }));
            add(pause(0.3));
        }
        add(tu("AP9 Foto: zurueck auf das Original", [] {
            for (int i = 0; i < 200 && modellMalenRueckgaengig(); ++i) {}
        }));
        add(pause(0.3));
    }
}

// Bilder fuer das Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=ap9): h_ap9_max, h_ap9_blender - der Sturmtruppler der Seite
// "Model-Tab: Texturen bemalen" mit einem Strich auf dem Original und einem auf einer Ebene, das rechte Feld in beiden Bedienmodi.
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP9 Handbuch: Einstellungen merken", [=] {
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
    }));
    pt1c::figurLaden(s, "max");
    add(tu("AP9 Handbuch: Gruppe torso, rot", [] {
        g_app->mtab.auswahl = {mb1::teil("torso")};
        mz().gruppe = -1;
        mz().pinsel.farbe = {0.85F, 0.1F, 0.1F, 1.0F};
        mz().radiusPx = 16.0F;
    }));
    add(pause(0.5));
    add(mb3::ziehenVonNach("AP9 Handbuch: Strich ueber die Brust", [] { return pt1c::torsoMitte(-50.0F); }, [] { return pt1c::torsoMitte(50.0F); }));
    add(pause(0.2));
    add(pt1c::ausloesen(Str::PtEbeneNeu));
    add(tu("AP9 Handbuch: blau auf der Ebene", [] { mz().pinsel.farbe = {0.1F, 0.3F, 0.95F, 1.0F}; }));
    add(mb3::ziehenVonNach("AP9 Handbuch: Strich im Bild", [] { return pt1c::bildPunkt(0.25F, 0.55F); }, [] { return pt1c::bildPunkt(0.75F, 0.55F); }));
    add(tu("AP9 Handbuch: Maus auf die Brust", [] { setzeMaus(pt1c::torsoMitte(10.0F)); }));
    add(nachOben());
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_ap9_max"));
    add(tu("AP9 Handbuch: wie Blender", [] {
        modellBedienungSetzen(1);
        g_app->mtab.bereich = ModellBereich::Painting;
    }));
    add(pause(0.9));
    add(tu("AP9 Handbuch: Maus auf die Brust", [] { setzeMaus(pt1c::torsoMitte(10.0F)); }));
    add(pause(0.5));
    add(hilfe2::schrittFoto("h_ap9_blender"));
    add(pt1c::zurueckAufOriginal());
    add(tu("AP9 Handbuch: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = true;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "ap9" && n != "ap9foto") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_AP9_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    const char* m = std::getenv("BHED_AP9_MODELL");
    const std::string modell = m != nullptr ? m : "models/players/kyle/model.glm";
    auto skala = std::make_shared<float>(1.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP9: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        if (const char* t = std::getenv("BHED_AP9_THEMA")) {
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
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap9.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    add(tu("AP9: Aufteilung wie neue Einstellungen, wie 3ds Max", [=] {
        *bedienung = g_app->settings.modellBedienung;
        *feldBreite = g_app->settings.modellFeldBreite;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        modellBedienungSetzen(0);
        animWaehlen(mbefoto::sequenz(), false);
        char zeile[300];
        std::snprintf(zeile, sizeof zeile, "%s, %zu Teile, Anzeige %.0f x %.0f, Oberflaeche %.2f, Schrift %.0f Punkte hoch", g_app->modelPath.c_str(), g_app->mtab.dok.inhalt().teile.size(),
                      static_cast<double>(ImGui::GetIO().DisplaySize.x), static_cast<double>(ImGui::GetIO().DisplaySize.y), static_cast<double>(g_app->uiScale),
                      static_cast<double>(ImGui::GetFontSize()));
        melde(zeile);
    }));
    add(pause(1.0));
    if (n == "ap9") { zeilenSchritte(s); }
    if (n == "ap9foto") { fotoSchritte(s); }
    add(tu("AP9: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
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

}  // namespace ap9
