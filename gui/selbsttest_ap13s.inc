// selbsttest_ap13s.inc - AP13S (MBENTWURF Teil B, 12.11.): Modifier-Stapel und Glaetten-Vorschau im Model-Tab
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap13s | ap13sfoto | ap13shb)
//
//   ap13s     je Zeile der Abgleichtabelle (plaene/modellblock_stufe0/abgleich_programm_entwurf.md) eine Reihe von Pruefungen:
//               M1       wie 3ds Max, Bereich Modellieren: Gruppe "Modifier-Stapel" zuoberst im Reiter Aendern, Kurzinfo die
//                        Flaeche; Zeilen von unten nach oben Flaeche der Datei -> Edit Poly (gewaehlt) -> Eintraege (-> Skin);
//                        "Modifier-Liste", Auge, Einstellungen, Umordnen (Knopf, Rechtsklick, Ziehen), "Collapse To" (Reduzieren bis),
//                        Entfernen - jede Aktion ein Rueckgaengig-Schritt, Wiederholen bringt sie zurueck
//               M2       wie Blender: Gruppe "Modifier" zuoberst im Reiter Modifier; Zeilen von oben nach unten in der
//                        Reihenfolge des Rechnens, je mit Auge und Datei-Schalter; "Modifier hinzufuegen", Anwenden wie
//                        Blender (nur dieser Eintrag, Meldung wenn er nicht der erste war)
//               W1       wie 3ds Max, Bereich Weights: derselbe Stapel, Skin gewaehlt; Edit Poly / Skin wechseln den Bereich.
//                        Die Glaetten-Vorschau stimmt mit dem angewendeten Ergebnis ueberein: das Netz der Vorschau ist Ecke
//                        fuer Ecke das Netz nach dem Anwenden, das gezeichnete Modell ist dasselbe, das Bild ist dasselbe
//               Datei    solange der Stapel leer ist (und solange jeder Eintrag "nur Vorschau" ist), schreibt Speichern
//                        dieselben Bytes wie ohne Stapel: die geladene Figur Byte fuer Byte die Datei des Spiels, eine geaenderte
//                        die Bytes von glmSchreiben(Dokument); die Pruefsummen stehen im Protokoll (Vergleich mit dem Programm
//                        vor AP13S). Ein Eintrag, der in die Datei geht, steht nach dem Speichern in der Datei.
//               Requisit ein .md3 ohne Skelett: Stapel ohne Skin, die einzige Flaeche ohne Auswahl, Export wie die Ansicht
//               Finden   jede Aktion in "Alle Aktionen suchen", im Rechtsklick der Ansicht und der Zeile, mit Tooltip
//               Fund     Symmetrisieren legt die Gewichte der gespiegelten Seite auf die Knochen der anderen Seite
//   ap13sfoto nur Fotos (BHED_FOTOS=<Ordner>): der Stapel in beiden Bedienmodi (ap13s_max_*, ap13s_blender_*)
//   ap13shb   die zwei Bilder fuer das Handbuch (h_ap13s_max, h_ap13s_blender)
//   BHED_AP13S_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe wie der Foto-Selbsttest mbefoto)
//   BHED_AP13S_MODELL=<.glm>           Vorgabe models/players/kyle/model.glm (das Modell des Entwurfs)
//   BHED_AP13S_THEMA=hell|dunkel       das erste helle bzw. dunkle Farbschema (Vorgabe: das eingestellte)
// Eingebunden in selbsttest.cpp (nach selbsttest_ap5.inc).
namespace ap13s {

namespace w = modell::wof;
using mbefoto::kindFenster;
using md1d::mz;
using modell::ModArt;

void melde(const std::string& z) { diag::detail("Knopftest: AP13S " + z); }

ModellTab& mtab() { return g_app->mtab; }
ModellStapel& st() { return modellStapel(); }
std::size_t uz(int i) { return static_cast<std::size_t>(i); }

const Element* imRoll(const std::string& label, int nte = 0) { return finde(label, "mtrollouts", nte, false); }
const Element* imPopup(const std::string& label) { return finde(label, "##Popup_", 0, false); }

bool feldFrisch() { return ui::feldProbe().bild >= ImGui::GetFrameCount() - 2; }
bool stapelFrisch() { return st().probeBild >= ImGui::GetFrameCount() - 2; }

std::string zeile(const char* nr, int modus, const std::string& was) { return std::string("AP13S ") + nr + (modus == 0 ? " wie 3ds Max: " : " wie Blender: ") + was; }

const ui::RolloutProbe* gruppe(const std::string& titel) {
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
        if (ui::rolloutProbe(i).titel == titel) { return &ui::rolloutProbe(i); }
    }
    return nullptr;
}

std::string gruppen() {
    std::string s;
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) { s += (i > 0 ? "|" : "") + ui::rolloutProbe(i).titel; }
    return s;
}

const char* gruppenTitel(bool bl) { return tr(bl ? Str::StpGruppeBl : Str::MgRStapel); }

// Die Namen der Eintraege, wie der Entwurf sie zeigt: "Symmetry (X)" / "Mirror (X)", "TurboSmooth (preview only)" / "Subdivision (preview only)"
std::string fmt1(Str s, const std::string& a) {
    char b[300];
    std::snprintf(b, sizeof b, tr(s), a.c_str());
    return b;
}
std::string nSpiegeln(bool bl, const char* achse = "X", bool datei = true) {
    char b[200];
    std::snprintf(b, sizeof b, tr(Str::StpMitAchse), tr(bl ? Str::StpSpiegelnBl : Str::StpSpiegelnMax), achse);
    return datei ? std::string(b) : fmt1(Str::StpNurVorschau, b);
}
std::string nGlaetten(bool bl, bool datei = false) {
    const std::string n = tr(bl ? Str::StpGlaettenBl : Str::StpGlaettenMax);
    return datei ? n : fmt1(Str::StpNurVorschau, n);
}
std::string nBasis(const std::string& flaeche, bool md3 = false) { return fmt1(md3 ? Str::StpBasisMd3 : Str::StpBasisGlm, flaeche); }

// Die Zeilen der Liste von oben nach unten, mit | getrennt (leer: die Gruppe wurde nicht gezeichnet)
std::string zeilen() {
    if (!stapelFrisch()) { return "(nicht gezeichnet)"; }
    std::string s;
    for (std::size_t i = 0; i < st().probeZeilen.size(); ++i) { s += (i > 0 ? "|" : "") + st().probeZeilen[i]; }
    return s;
}
std::string liste(std::initializer_list<std::string> l) {
    std::string s;
    for (const std::string& x : l) { s += (s.empty() ? "" : "|") + x; }
    return s;
}
std::string gewaehlteZeile() {
    return stapelFrisch() && st().probeGewaehlt >= 0 && uz(st().probeGewaehlt) < st().probeZeilen.size() ? st().probeZeilen[uz(st().probeGewaehlt)] : std::string();
}

int torso() { return mb1::teil("torso"); }
const modell::Teil& teilVon(int t) { return mtab().dok.inhalt().teile[uz(t)]; }
const modell::Netz& netzVon(int t) { return teilVon(t).stufen[uz(mtab().lodJetzt)]; }

// FNV-1a ueber Bytes (Pruefsumme fuer das Protokoll)
std::uint64_t fnv(const void* daten, std::size_t zahl, std::uint64_t h = 1469598103934665603ULL) {
    const unsigned char* p = static_cast<const unsigned char*>(daten);
    for (std::size_t i = 0; i < zahl; ++i) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}
std::string hex(std::uint64_t h) {
    char b[32];
    std::snprintf(b, sizeof b, "%016llx", static_cast<unsigned long long>(h));
    return b;
}
std::uint64_t netzSumme(const modell::Netz& n) {
    std::uint64_t h = fnv("netz", 4);
    for (const modell::Ecke& e : n.ecken) {
        h = fnv(e.ort, sizeof e.ort, h);
        h = fnv(e.normale, sizeof e.normale, h);
        h = fnv(e.uv, sizeof e.uv, h);
        h = fnv(e.knochen, sizeof e.knochen, h);
        h = fnv(e.gewicht, sizeof e.gewicht, h);
    }
    const std::vector<std::uint32_t> idx = n.indizes();
    return idx.empty() ? h : fnv(idx.data(), idx.size() * sizeof(std::uint32_t), h);
}
std::uint64_t flaecheSumme(const GlmSurface& s) {
    std::uint64_t h = fnv("glm", 3);
    for (const GlmVertex& v : s.verts) {
        h = fnv(v.xyz, sizeof v.xyz, h);
        h = fnv(v.normal, sizeof v.normal, h);
        h = fnv(v.st, sizeof v.st, h);
        h = fnv(v.bones, sizeof v.bones, h);
        h = fnv(v.weights, sizeof v.weights, h);
    }
    return s.indexes.empty() ? h : fnv(s.indexes.data(), s.indexes.size() * sizeof(std::uint32_t), h);
}

// Der Stapel einer Flaeche als Text und der Zustand des Dokuments fuer Rueckgaengig/Wiederholen (Stapel + Netz der Flaeche)
std::string stapelText(int t) {
    std::string s;
    for (const modell::Modifikator& m : teilVon(t).stapel) {
        char b[96];
        std::snprintf(b, sizeof b, "%s%u:%c%c:%d%s:%s:%s ", m.art == ModArt::Spiegeln ? "S" : "G", m.kennung, "XYZ"[std::min<int>(m.achse, 2)], m.vonPlus ? '+' : '-',
                      static_cast<int>(m.stufen), m.raenderHalten ? "r" : "f", m.an ? "an" : "aus", m.datei ? "datei" : "vorschau");
        s += b;
    }
    return s.empty() ? "(leer)" : s;
}
std::string zustand(int t) { return stapelText(t) + "#" + std::to_string(netzVon(t).ecken.size()) + "/" + std::to_string(netzVon(t).dreiecke()) + "/" + hex(netzSumme(netzVon(t))); }

int dreieckeGezeichnet(int t) {
    const GlmModel& m = modellStapelZeichnung();
    return uz(t) < m.surfaces.size() ? static_cast<int>(m.surfaces[uz(t)].indexes.size() / 3U) : -1;
}
std::uint32_t kennung(int t, int index) { return uz(index) < teilVon(t).stapel.size() ? teilVon(t).stapel[uz(index)].kennung : 0U; }
// Hat torso den Eintrag `index`? Und der Eintrag selbst - fehlt er, ein leerer (jede Pruefung fragt vorher hat()).
bool hat(int index) { return index >= 0 && uz(index) < teilVon(torso()).stapel.size(); }
const modell::Modifikator& eintrag(int index) {
    static const modell::Modifikator keiner;
    return hat(index) ? teilVon(torso()).stapel[uz(index)] : keiner;
}

void rollenNachOben() {
    if (ImGuiWindow* fw = kindFenster("mtrollouts")) { ImGui::SetScrollY(fw, 0.0F); }
}

// Nach einer Aktion: `vorher` ist der Zustand davor. Rueckgaengig stellt ihn her, Wiederholen den Zustand danach - und der
// Schritt traegt den erwarteten Namen. `flaeche` < 0: die Flaeche torso (erst beim Lauf bekannt - die Schrittliste entsteht vor
// dem Laden des Modells).
Schritt undoRedo(const std::string& text, int flaeche, std::shared_ptr<std::string> vorher, std::function<std::string()> name = {}) {
    return pruefe(text + ": ein Rueckgaengig-Schritt - Strg+Z stellt den Stand davor her, Strg+Y den danach", [=] {
        const int t = flaeche < 0 ? torso() : flaeche;
        const std::string nach = zustand(t);
        const std::string schritt = mtab().dok.rueckgaengigName();
        bool ok = nach != *vorher;
        if (name) { ok = ok && schritt == name(); }
        ok = ok && modellRueckgaengig();
        const std::string zurueck = zustand(t);
        ok = ok && zurueck == *vorher;
        ok = ok && modellWiederholen();
        const std::string wieder = zustand(t);
        ok = ok && wieder == nach;
        if (!ok) { melde(text + " - Schritt \"" + schritt + "\", davor " + *vorher + ", danach " + nach + ", nach Strg+Z " + zurueck + ", nach Strg+Y " + wieder); }
        return ok;
    });
}
Schritt merke(int flaeche, std::shared_ptr<std::string> vorher) {
    return tu("AP13S: Stand merken", [=] { *vorher = zustand(flaeche < 0 ? torso() : flaeche); });
}

Schritt klickRoll(const std::string& label, int nte = 0, int maustaste = 0) {
    return klickAn("\"" + label + "\" im rechten Feld", [=] {
        const Element* e = imRoll(label, nte);
        if (e == nullptr) { meldeFehler("AP13S: \"" + label + "\" steht nicht im rechten Feld (" + zeilen() + ")"); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    }, maustaste);
}
Schritt klickPopup(const std::string& label) {
    return klickAn("\"" + label + "\" (Menue)", [label] {
        const Element* e = imPopup(label);
        if (e == nullptr) { meldeFehler("AP13S: \"" + label + "\" steht nicht im Menue"); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F};
    });
}
// Die Maus auf eine Zeile der Liste (linker Teil: neben dem Namen, nicht auf den Knoepfen rechts)
ImVec2 zeilenOrt(const std::string& label) {
    const Element* e = imRoll(label);
    return e != nullptr ? ImVec2{e->rect.Min.x + e->rect.GetWidth() * 0.3F, e->rect.GetCenter().y} : ImVec2{-50.0F, -50.0F};
}
// Eine Zeile mit gedrueckter Maustaste auf eine andere ziehen (Umordnen wie in 3ds Max und Blender)
Schritt zieheZeile(const std::string& von, const std::string& nach) {
    auto a = std::make_shared<ImVec2>();
    auto z = std::make_shared<ImVec2>();
    return {"Ziehe die Zeile \"" + von + "\" auf \"" + nach + "\"", [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    *a = zeilenOrt(von);
                    *z = zeilenOrt(nach);
                    if (a->x < 0.0F || z->x < 0.0F) { meldeFehler("AP13S Ziehen: Zeile nicht gefunden (" + zeilen() + ")"); }
                    setzeMaus(*a);
                    return false;
                }
                if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                if (b >= 4 && b <= 12) {
                    const float f = static_cast<float>(b - 3) / 9.0F;
                    setzeMaus(ImVec2{a->x, a->y + (z->y - a->y) * f});
                    return false;
                }
                if (b == 15) { io.AddMouseButtonEvent(0, false); return false; }
                return b >= 18;
            }};
}

void figurWaehlen(int modus, ModellBereich bereich) {
    modellBedienungSetzen(modus);
    mtab().bereich = bereich;
    g_app->settings.modellFeldBreite = 0.0F;
    mtab().auswahl = {torso()};
    rollenNachOben();
}

// Alle Eintraege der Flaeche ueber das Dokument entfernen (Aufraeumen zwischen den Abschnitten; kein Befehl des Programms)
void stapelLeeren() {
    if (modell::stapelLeer(mtab().dok.inhalt())) { return; }
    mtab().dok.aendern("AP13S aufraeumen", [](modell::Inhalt& x) {
        for (modell::Teil& t : x.teile) { t.stapel.clear(); }
    });
    st().gewaehlt = 0;
    g_app->modelDirty = true;
}

// --- M1 / M2: der Stapel im Bereich Modellieren -------------------------------------------------------------------------

void stapelSchritte(std::vector<Schritt>& s, int modus) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const bool bl = modus == 1;
    const char* nr = bl ? "M2" : "M1";
    auto vorher = std::make_shared<std::string>();
    auto bildOhne = std::make_shared<mb1::AnsichtBild>();
    auto bildMit = std::make_shared<mb1::AnsichtBild>();
    add(tu(std::string("AP13S: Bedienmodus ") + (bl ? "wie Blender" : "wie 3ds Max") + ", Modellieren, Flaeche torso gewaehlt, Stapel leer", [=] {
        stapelLeeren();
        figurWaehlen(modus, ModellBereich::Modellieren);
        md1d::waehle(w::PAuswahl{}, AuswahlArt::Dreieck);
    }));
    add(pause(0.8));
    add(tu("AP13S: Bild der Ansicht ohne Eintrag", [=] { (void)mb1::ansichtLesen(*bildOhne); }));
    // --- Ort und Form der Gruppe ---
    add(pruefe(zeile(nr, modus, bl ? "die Gruppe Modifier steht zuoberst im Reiter Modifier, Kurzinfo torso, im Stil wie Blender"
                                   : "die Gruppe Modifier-Stapel steht zuoberst im Reiter Aendern, Kurzinfo torso, im Stil wie 3ds Max"), [=] {
        const ui::RolloutProbe* g = gruppe(gruppenTitel(bl));
        melde("Gruppen: " + gruppen() + ", Reiter " + std::to_string(ui::feldProbe().reiterAktiv) + ", Zeilen " + zeilen());
        return feldFrisch() && g != nullptr && ui::rolloutProbeZahl() > 0 && ui::rolloutProbe(0).titel == gruppenTitel(bl) && g->kurzinfo == "torso" &&
               ui::feldProbe().reiterAktiv == (bl ? 2 : 1) && g->stil == (bl ? ui::RolloutStil::Blender : ui::RolloutStil::Max) && g->offen;
    }));
    if (bl) {
        add(pruefe(zeile(nr, modus, "ohne Eintrag: keine Zeile, der Hinweis nennt den Knopf Modifier hinzufuegen"), [] {
            return zeilen().empty() && stapelFrisch() && imRoll(tr(Str::StpHinzuBl)) != nullptr;
        }));
    } else {
        add(pruefe(zeile(nr, modus, "ohne Eintrag von oben: Skin, Edit Poly (gewaehlt), GLM-Flaeche torso - der Stapel liest sich von unten nach oben"), [] {
            return zeilen() == liste({tr(Str::MgStapelSkin), tr(Str::EaUnwrap), tr(Str::MgStapelPoly), nBasis("torso")}) && gewaehlteZeile() == tr(Str::MgStapelPoly);
        }));
    }
    add(pruefe(zeile(nr, modus, bl ? "der Knopf Modifier hinzufuegen steht ueber der Liste" : "der Knopf Modifier-Liste steht ueber dem Stapel"), [=] {
        const Element* k = imRoll(tr(bl ? Str::StpHinzuBl : Str::StpHinzuMax));
        const ui::RolloutProbe* g = gruppe(gruppenTitel(bl));
        return k != nullptr && g != nullptr && g->kasten.Contains(k->rect) && (st().probeRechtecke.empty() || k->rect.Max.y <= st().probeRechtecke.front().Min.y);
    }));
    // --- Hinzufuegen: Glaetten ---
    add(merke(-1, vorher));
    add(klickRoll(tr(bl ? Str::StpHinzuBl : Str::StpHinzuMax)));
    add(pause(0.5));
    add(pruefe(zeile(nr, modus, bl ? "das Menue nennt Mirror und Subdivision" : "das Menue nennt Symmetry und TurboSmooth"), [=] {
        return imPopup(tr(bl ? Str::StpSpiegelnBl : Str::StpSpiegelnMax)) != nullptr && imPopup(tr(bl ? Str::StpGlaettenBl : Str::StpGlaettenMax)) != nullptr;
    }));
    add(klickPopup(tr(bl ? Str::StpGlaettenBl : Str::StpGlaettenMax)));
    add(pause(0.6));
    add(pruefe(zeile(nr, modus, bl ? "Subdivision (nur Vorschau) steht in der Liste und ist gewaehlt, mit Auge (an) und Datei-Schalter (aus)"
                                   : "TurboSmooth (nur Vorschau) steht ueber Edit Poly und ist gewaehlt, mit Auge (an)"), [=] {
        const std::string soll = bl ? nGlaetten(true) : liste({tr(Str::MgStapelSkin), nGlaetten(false), tr(Str::EaUnwrap), tr(Str::MgStapelPoly), nBasis("torso")});
        const Element* auge = imRoll(tr(Str::StpAn));
        const Element* datei = imRoll(tr(Str::StpDatei));
        const bool knoepfe = auge != nullptr && (auge->flags & ImGuiItemStatusFlags_Checked) != 0 &&
                             (bl ? datei != nullptr && (datei->flags & ImGuiItemStatusFlags_Checked) == 0 && datei->rect.Min.x > auge->rect.Min.x : true);
        melde("Zeilen " + zeilen() + ", gewaehlt " + gewaehlteZeile() + ", Stapel " + stapelText(torso()));
        return zeilen() == soll && gewaehlteZeile() == nGlaetten(bl) && knoepfe && teilVon(torso()).stapel.size() == 1 && !eintrag(0).datei;
    }));
    add(undoRedo(zeile(nr, modus, "Hinzufuegen"), -1, vorher, [=] { return fmt1(Str::StpAktHinzu, tr(bl ? Str::StpGlaettenBl : Str::StpGlaettenMax)); }));
    add(pause(0.5));
    add(pruefe(zeile(nr, modus, "die Ansicht zeichnet die geglaettete Flaeche (sechsmal so viele Dreiecke), das Netz des Dokuments bleibt"), [=] {
        const int dok = netzVon(torso()).dreiecke();
        const int gezeichnet = dreieckeGezeichnet(torso());
        const int kaefig = static_cast<int>(mtab().anzeige.surfaces[uz(torso())].indexes.size() / 3U);
        (void)mb1::ansichtLesen(*bildMit);
        const int anders = mb1::unterschied(bildOhne->px, bildMit->px);
        melde("Dreiecke Dokument " + std::to_string(dok) + ", gezeichnet " + std::to_string(gezeichnet) + ", Kaefig " + std::to_string(kaefig) + ", Bildpunkte anders " +
              std::to_string(anders));
        return modellStapelErsetzt(torso()) && gezeichnet == dok * 6 && kaefig == dok && anders > 50;
    }));
    add(pruefe(zeile(nr, modus, "unter der Liste stehen die Einstellungen des gewaehlten Eintrags: Unterteilungen, Raender halten, In die Datei schreiben, Ecken und Dreiecke davor und danach"), [=] {
        const Element* stufen = imRoll(tr(bl ? Str::StpStufenBl : Str::StpStufenMax));
        const Element* raender = imRoll(tr(Str::StpRaender));
        const Element* ecken = imRoll(tr(Str::StpEcken));
        return stufen != nullptr && raender != nullptr && ecken != nullptr && !st().probeRechtecke.empty() && stufen->rect.Min.y >= st().probeRechtecke.back().Max.y &&
               imRoll(tr(Str::StpHoch)) != nullptr && imRoll(tr(Str::StpRunter)) != nullptr && imRoll(tr(bl ? Str::StpAnwendenBl : Str::StpAnwendenMax)) != nullptr &&
               imRoll(tr(Str::StpEntfernen)) != nullptr;
    }));
    // --- Auge ---
    add(merke(-1, vorher));
    add(klickRoll(tr(Str::StpAn)));
    add(pause(0.6));
    add(pruefe(zeile(nr, modus, "Auge aus: der Eintrag rechnet nicht mehr, die Ansicht zeigt wieder genau das Bild ohne ihn"), [=] {
        mb1::AnsichtBild jetzt;
        (void)mb1::ansichtLesen(jetzt);
        const int anders = mb1::unterschied(bildOhne->px, jetzt.px);
        const Element* auge = imRoll(tr(Str::StpAn));
        melde("Auge aus: Bildpunkte anders als ohne Eintrag " + std::to_string(anders));
        return hat(0) && !eintrag(0).an && !modellStapelErsetzt(torso()) && anders == 0 && auge != nullptr && (auge->flags & ImGuiItemStatusFlags_Checked) == 0;
    }));
    add(undoRedo(zeile(nr, modus, "Auge"), -1, vorher, [] { return std::string(tr(Str::StpAktAnAus)); }));
    add(klickRoll(tr(Str::StpAn)));
    add(pause(0.5));
    add(pruefe(zeile(nr, modus, "Auge wieder an: die Ansicht zeigt wieder das Bild mit dem Eintrag"), [=] {
        mb1::AnsichtBild jetzt;
        (void)mb1::ansichtLesen(jetzt);
        return hat(0) && eintrag(0).an && modellStapelErsetzt(torso()) && mb1::unterschied(bildMit->px, jetzt.px) == 0;
    }));
    // --- Einstellung: Unterteilungen (Klick an das rechte Ende des Reglers: 3) ---
    add(merke(-1, vorher));
    add(klickAn("rechtes Ende des Reglers Unterteilungen", [=] {
        const Element* e = imRoll(tr(bl ? Str::StpStufenBl : Str::StpStufenMax));
        return e != nullptr ? ImVec2{e->rect.Max.x - 3.0F, e->rect.GetCenter().y} : ImVec2{-50.0F, -50.0F};
    }));
    add(pause(0.8));
    add(pruefe(zeile(nr, modus, "Regler auf 3: die Ansicht zeichnet 96-mal so viele Dreiecke"), [] {
        return hat(0) && eintrag(0).stufen == 3 && dreieckeGezeichnet(torso()) == netzVon(torso()).dreiecke() * 96;
    }));
    add(undoRedo(zeile(nr, modus, "Regler Unterteilungen"), -1, vorher, [] { return std::string(tr(Str::StpUndoEinstellung)); }));
    add(tu("AP13S: zurueck auf 1 Unterteilung (Strg+Z)", [] { (void)modellRueckgaengig(); }));
    add(pause(0.5));
    add(merke(-1, vorher));
    add(klickRoll(tr(Str::StpRaender)));
    add(pause(0.5));
    add(pruefe(zeile(nr, modus, "Raender halten aus: der Rand der Flaeche glaettet mit (andere Ecken, gleiche Zahl)"), [] {
        return hat(0) && !eintrag(0).raenderHalten && dreieckeGezeichnet(torso()) == netzVon(torso()).dreiecke() * 6;
    }));
    add(undoRedo(zeile(nr, modus, "Raender halten"), -1, vorher));
    add(tu("AP13S: Raender wieder halten (Strg+Z)", [] { (void)modellRueckgaengig(); }));
    add(pause(0.4));
    // --- zweiter Eintrag ueber "Alle Aktionen suchen": Spiegeln ---
    add(merke(-1, vorher));
    add(tu("AP13S: Alle Aktionen suchen", [] { mtab().aktionenOffen = true; }));
    add(pause(0.5));
    add(tippe(fmt1(Str::StpAktHinzu, tr(bl ? Str::StpSpiegelnBl : Str::StpSpiegelnMax))));
    add(pause(0.5));
    add(pruefe(zeile(nr, modus, "\"Alle Aktionen suchen\" findet die Aktion Modifier hinzufuegen: Spiegeln"), [=] {
        return mtab().aktionenSichtbar && imPopup(fmt1(Str::StpAktHinzu, tr(bl ? Str::StpSpiegelnBl : Str::StpSpiegelnMax))) != nullptr;
    }));
    add(ap8::klickLetzter(fmt1(Str::StpAktHinzu, tr(bl ? Str::StpSpiegelnBl : Str::StpSpiegelnMax)), "##Popup_"));
    add(pause(0.7));
    add(pruefe(zeile(nr, modus, bl ? "Mirror (X) steht unter Subdivision (er rechnet danach), ist gewaehlt, Auge und Datei-Schalter an"
                                   : "Symmetry (X) steht ueber TurboSmooth (er rechnet danach) und ist gewaehlt"), [=] {
        const std::string soll = bl ? liste({nGlaetten(true), nSpiegeln(true)}) : liste({tr(Str::MgStapelSkin), nSpiegeln(false), nGlaetten(false), tr(Str::EaUnwrap), tr(Str::MgStapelPoly), nBasis("torso")});
        const auto& stapel = teilVon(torso()).stapel;
        melde("Zeilen " + zeilen() + ", Stapel " + stapelText(torso()));
        return zeilen() == soll && gewaehlteZeile() == nSpiegeln(bl) && stapel.size() == 2 && stapel[1].art == ModArt::Spiegeln && stapel[1].datei && stapel[1].an;
    }));
    add(undoRedo(zeile(nr, modus, "Hinzufuegen ueber die Suche"), -1, vorher));
    add(pause(0.5));
    add(pruefe(zeile(nr, modus, "die gezeichnete Flaeche ist an X gespiegelt: jede Ecke hat ihr Gegenstueck auf der anderen Seite"), [] {
        const GlmSurface& g = modellStapelZeichnung().surfaces[uz(torso())];
        int ohne = 0;
        for (const GlmVertex& v : g.verts) {
            bool da = std::fabs(v.xyz[0]) < 1e-3F;
            for (std::size_t k = 0; !da && k < g.verts.size(); ++k) {
                da = std::fabs(g.verts[k].xyz[0] + v.xyz[0]) < 1e-3F && std::fabs(g.verts[k].xyz[1] - v.xyz[1]) < 1e-3F && std::fabs(g.verts[k].xyz[2] - v.xyz[2]) < 1e-3F;
            }
            ohne += da ? 0 : 1;
        }
        melde("gespiegelt: " + std::to_string(g.verts.size()) + " Ecken, ohne Gegenstueck " + std::to_string(ohne));
        return modellStapelErsetzt(torso()) && !g.verts.empty() && ohne == 0;
    }));
    add(pruefe(zeile(nr, modus, "Einstellungen des Spiegelns: Achse X/Y/Z und Richtung als Wahl, In die Datei schreiben (an)"), [] {
        return imRoll("X") != nullptr && imRoll("Y") != nullptr && imRoll("Z") != nullptr && imRoll(tr(Str::StpPlusMinus)) != nullptr && imRoll(tr(Str::StpMinusPlus)) != nullptr;
    }));
    add(merke(-1, vorher));
    add(klickRoll(tr(Str::StpMinusPlus)));
    add(pause(0.5));
    add(pruefe(zeile(nr, modus, "Richtung umgekehrt: die Minus-Seite ist die Quelle"), [] { return hat(1) && !eintrag(1).vonPlus; }));
    add(undoRedo(zeile(nr, modus, "Richtung"), -1, vorher));
    add(tu("AP13S: Richtung zurueck (Strg+Z)", [] { (void)modellRueckgaengig(); }));
    add(pause(0.4));
    add(merke(-1, vorher));
    add(klickRoll("Y"));
    add(pause(0.5));
    add(pruefe(zeile(nr, modus, "Achse Y: der Name der Zeile nennt sie"), [=] { return hat(1) && eintrag(1).achse == 1 && gewaehlteZeile() == nSpiegeln(bl, "Y"); }));
    add(undoRedo(zeile(nr, modus, "Achse"), -1, vorher));
    add(tu("AP13S: Achse zurueck (Strg+Z)", [] { (void)modellRueckgaengig(); }));
    add(pause(0.4));
    // --- Umordnen: Knopf, Rechtsklick, Ziehen ---
    add(merke(-1, vorher));
    add(ap3::insFeld(tr(bl ? Str::StpHoch : Str::StpRunter)));
    add(pause(0.3));
    add(klickRoll(tr(bl ? Str::StpHoch : Str::StpRunter)));
    add(pause(0.6));
    add(pruefe(zeile(nr, modus, bl ? "Nach oben: Mirror steht jetzt ueber Subdivision und rechnet zuerst" : "Nach unten: Symmetry steht jetzt unter TurboSmooth und rechnet zuerst"), [=] {
        const std::string soll = bl ? liste({nSpiegeln(true), nGlaetten(true)}) : liste({tr(Str::MgStapelSkin), nGlaetten(false), nSpiegeln(false), tr(Str::EaUnwrap), tr(Str::MgStapelPoly), nBasis("torso")});
        melde("Zeilen " + zeilen() + ", Stapel " + stapelText(torso()));
        return zeilen() == soll && eintrag(0).art == ModArt::Spiegeln && gewaehlteZeile() == nSpiegeln(bl);
    }));
    add(undoRedo(zeile(nr, modus, "Umordnen mit dem Knopf"), -1, vorher, [=] { return std::string(tr(bl ? Str::StpAktHoch : Str::StpAktRunter)); }));
    add(pruefe(zeile(nr, modus, "am Ende der Liste ist der Knopf in diese Richtung gesperrt (ein Klick aendert nichts)"), [=] {
        const std::string davor = stapelText(torso());
        return hat(1) && !modellStapelBewegen(kennung(torso(), 0), bl ? -1 : 1) && stapelText(torso()) == davor;
    }));
    add(tu("AP13S: nach oben rollen", [] { rollenNachOben(); }));
    add(pause(0.3));
    add(merke(-1, vorher));
    add(klickAn("Rechtsklick auf die Zeile Glaetten", [=] { return zeilenOrt(nGlaetten(bl)); }, 1));
    add(pause(0.6));
    add(pruefe(zeile(nr, modus, "Rechtsklick auf eine Zeile: An, In die Datei schreiben, Nach oben, Nach unten, Anwenden, Alle anwenden, Entfernen"), [=] {
        int da = 0;
        for (const Str x : {Str::StpAn, Str::StpDatei, Str::StpHoch, Str::StpRunter, bl ? Str::StpAnwendenBl : Str::StpAnwendenMax, bl ? Str::StpAlleAnwendenBl : Str::StpAlleAnwendenMax,
                            Str::StpEntfernen}) {
            da += imPopup(tr(x)) != nullptr ? 1 : 0;
        }
        return da == 7 && gewaehlteZeile() == nGlaetten(bl);
    }));
    add(klickPopup(tr(Str::StpDatei)));
    add(pause(0.6));
    add(pruefe(zeile(nr, modus, "In die Datei schreiben (Rechtsklick): der Zusatz nur Vorschau faellt aus dem Namen"), [=] {
        return hat(1) && eintrag(1).datei && gewaehlteZeile() == nGlaetten(bl, true);
    }));
    add(undoRedo(zeile(nr, modus, "In die Datei schreiben"), -1, vorher, [] { return std::string(tr(Str::StpAktDatei)); }));
    add(tu("AP13S: wieder nur Vorschau (Strg+Z)", [] { (void)modellRueckgaengig(); }));
    add(pause(0.5));
    add(merke(-1, vorher));
    add(zieheZeile(nGlaetten(bl), nSpiegeln(bl)));
    add(pause(0.6));
    add(pruefe(zeile(nr, modus, "Ziehen einer Zeile auf die andere tauscht die Reihenfolge"), [=] {
        melde("nach dem Ziehen: " + zeilen() + ", Stapel " + stapelText(torso()));
        return hat(1) && eintrag(0).art == ModArt::Glaetten && eintrag(1).art == ModArt::Spiegeln;
    }));
    add(undoRedo(zeile(nr, modus, "Umordnen durch Ziehen"), -1, vorher));
    add(tu("AP13S: Reihenfolge zurueck: erst Spiegeln, dann Glaetten (Strg+Z)", [] { (void)modellRueckgaengig(); }));
    add(pause(0.5));
    // --- Tooltip ---
    add(tu("AP13S: Maus auf die Zeile Spiegeln", [=] { setzeMaus(zeilenOrt(nSpiegeln(bl))); }));
    add(pause(1.6));
    add(pruefe(zeile(nr, modus, "die Zeile hat einen Tooltip"), [] { return ap3::tooltipDa(); }));
    add(tu("AP13S: Maus auf das Auge", [] {
        const Element* e = imRoll(tr(Str::StpAn));
        setzeMaus(e != nullptr ? e->rect.GetCenter() : ImVec2{-50.0F, -50.0F});
    }));
    add(pause(1.6));
    add(pruefe(zeile(nr, modus, "das Auge hat einen Tooltip"), [] { return ap3::tooltipDa(); }));
    add(tu("AP13S: Maus weg", [] { setzeMaus(kv5::fokusPunkt()); }));
    add(pause(0.4));
    // --- Rechtsklick der Ansicht ---
    add(tu("AP13S: Rechtsklick-Menue der Ansicht", [] { mtab().kontextOeffnen = true; }));
    add(pause(0.5));
    add(pruefe(zeile(nr, modus, "im Rechtsklick der Ansicht steht das Menue des Stapels"), [=] { return mtab().kontextSichtbar && imPopup(gruppenTitel(bl)) != nullptr; }));
    add(klickPopup(gruppenTitel(bl)));
    add(pause(0.5));
    add(pruefe(zeile(nr, modus, "... mit Hinzufuegen, An, In die Datei, Nach oben/unten, Anwenden, Alle anwenden, Entfernen, Stapel zeigen"), [=] {
        int da = 0;
        for (const Str x : {bl ? Str::StpHinzuBl : Str::StpHinzuMax, Str::StpAn, Str::StpDatei, Str::StpHoch, Str::StpRunter, bl ? Str::StpAnwendenBl : Str::StpAnwendenMax,
                            bl ? Str::StpAlleAnwendenBl : Str::StpAlleAnwendenMax, Str::StpEntfernen, Str::StpAktZeigen}) {
            for (const Element& e : g_elemente) {
                if (e.label == tr(x) && e.innen.find("mtrollouts") == std::string::npos) {
                    ++da;
                    break;
                }
            }
        }
        return da == 9;
    }));
    add(mbefoto::popupsZu());
    add(pause(0.4));
    // --- Vorschau = angewendet (W1) ---
    auto vorschau = std::make_shared<modell::Netz>();
    auto vorschauFlaeche = std::make_shared<std::uint64_t>(0);
    auto vorschauBild = std::make_shared<mb1::AnsichtBild>();
    auto vorAnwenden = std::make_shared<std::size_t>(0);   // Eintraege im Stapel vor dem Anwenden (zwei)
    auto angewendet = std::make_shared<bool>(false);
    if (bl) {
        // wie Blender: Anwenden nimmt nur den gewaehlten Eintrag; zuerst der erste (Mirror), dann der naechste (Subdivision)
        add(tu("AP13S: Mirror waehlen, Vorschau merken (nur Mirror: Subdivision aus)", [=] {
            st().gewaehlt = kennung(torso(), 0);
            (void)modellStapelSchalten(kennung(torso(), 1), false);
        }));
        add(pause(0.6));
    } else {
        add(tu("AP13S: TurboSmooth waehlen (oberster Eintrag)", [] { st().gewaehlt = kennung(torso(), 1); }));
        add(pause(0.4));
    }
    add(tu("AP13S: Vorschau merken (Netz, gezeichnete Flaeche, Bild)", [=] {
        md1d::waehle(w::PAuswahl{}, AuswahlArt::Dreieck);
        *vorschau = modellStapelVorschauNetz(torso());
        *vorschauFlaeche = flaecheSumme(modellStapelZeichnung().surfaces[uz(torso())]);
        *vorAnwenden = teilVon(torso()).stapel.size();
        (void)mb1::ansichtLesen(*vorschauBild);
    }));
    add(merke(-1, vorher));
    add(ap3::insFeld(tr(bl ? Str::StpAnwendenBl : Str::StpAnwendenMax)));
    add(pause(0.3));
    add(klickRoll(tr(bl ? Str::StpAnwendenBl : Str::StpAnwendenMax)));
    add(pause(0.8));
    add(pruefe(zeile("W1", modus, bl ? "Anwenden (Mirror): das Netz des Dokuments ist Ecke fuer Ecke das der Vorschau, der Eintrag ist weg, Subdivision bleibt"
                                     : "Collapse To (Reduzieren bis) TurboSmooth: das Netz des Dokuments ist Ecke fuer Ecke das der Vorschau, beide Eintraege sind weg"), [=] {
        const bool gleich = modell::netzGleich(netzVon(torso()), *vorschau);
        const std::size_t rest = teilVon(torso()).stapel.size();
        melde("angewendet: Netz gleich " + std::to_string(gleich ? 1 : 0) + ", " + std::to_string(netzVon(torso()).ecken.size()) + " Ecken, " + std::to_string(netzVon(torso()).dreiecke()) +
              " Dreiecke, Vorschau " + std::to_string(vorschau->ecken.size()) + " / " + std::to_string(vorschau->dreiecke()) + ", Rest im Stapel " + std::to_string(rest));
        *angewendet = *vorAnwenden == 2U && rest == (bl ? 1U : 0U) && modellStapel().befehle > 0;
        return gleich && !vorschau->ecken.empty() && *angewendet;
    }));
    add(pruefe(zeile("W1", modus, "das gezeichnete Modell ist dasselbe wie in der Vorschau (Ecken und Dreiecke bitgleich) und das Bild der Ansicht auch"), [=] {
        mb1::AnsichtBild jetzt;
        (void)mb1::ansichtLesen(jetzt);
        const int anders = mb1::unterschied(vorschauBild->px, jetzt.px);
        const std::uint64_t summe = flaecheSumme(modellStapelZeichnung().surfaces[uz(torso())]);
        melde("Vorschau " + hex(*vorschauFlaeche) + ", angewendet " + hex(summe) + ", Bildpunkte anders " + std::to_string(anders));
        return *angewendet && summe == *vorschauFlaeche && anders == 0 && !modellStapelErsetzt(torso());
    }));
    add(undoRedo(zeile("W1", modus, "Anwenden"), -1, vorher, [=] { return std::string(tr(bl ? Str::StpAktAnwendenBl : Str::StpAktAnwendenMax)); }));
    if (bl) {
        add(tu("AP13S: Anwenden zuruecknehmen, Subdivision an; jetzt Subdivision anwenden, obwohl Mirror davor steht", [=] {
            (void)modellRueckgaengig();
            (void)modellStapelSchalten(kennung(torso(), 1), false);
            st().gewaehlt = kennung(torso(), 1);
        }));
        add(pause(0.6));
        add(pruefe(zeile(nr, modus, "Anwenden eines Eintrags, der nicht der erste ist: nur er wird angewendet, Mirror bleibt, die Meldung sagt es wie Blender"), [=] {
            const int vor = netzVon(torso()).dreiecke();
            const bool ok = modellStapelAnwenden(kennung(torso(), 1));
            const auto& stapel = teilVon(torso()).stapel;
            melde("nicht der erste: " + mtab().meldung + " | " + stapelText(torso()));
            return ok && stapel.size() == 1 && stapel[0].art == ModArt::Spiegeln && netzVon(torso()).dreiecke() == vor * 6 && mtab().meldung == tr(Str::StpMeldNichtErster);
        }));
        add(tu("AP13S: zuruecknehmen", [] { (void)modellRueckgaengig(); }));
        add(pause(0.5));
    } else {
        add(tu("AP13S: Anwenden zuruecknehmen", [] { (void)modellRueckgaengig(); }));
        add(pause(0.5));
    }
    // --- Alle anwenden, ein ausgeschalteter Eintrag ---
    add(merke(-1, vorher));
    add(pruefe(zeile(nr, modus, "ein ausgeschalteter Eintrag laesst sich nicht anwenden (Meldung), der Stapel bleibt"), [=] {
        if (!hat(1)) { return false; }
        const std::uint32_t k = kennung(torso(), 1);
        (void)modellStapelSchalten(k, false);
        const std::string davor = stapelText(torso());
        const bool abgelehnt = !modellStapelAnwenden(k) && stapelText(torso()) == davor;
        (void)modellRueckgaengig();
        return abgelehnt;
    }));
    add(pause(0.4));
    add(pruefe(zeile(nr, modus, "Alle anwenden: beide Eintraege in der Reihenfolge des Stapels, das Netz ist das der Vorschau beider"), [=] {
        const modell::Netz beide = modellStapelVorschauNetz(torso());
        const bool ok = modellStapelAllesAnwenden();
        return ok && teilVon(torso()).stapel.empty() && modell::netzGleich(netzVon(torso()), beide);
    }));
    add(undoRedo(zeile(nr, modus, "Alle anwenden"), -1, vorher, [=] { return std::string(tr(bl ? Str::StpAktAlleBl : Str::StpAktAlleMax)); }));
    add(tu("AP13S: zuruecknehmen", [] { (void)modellRueckgaengig(); }));
    add(pause(0.5));
    // --- Entfernen ---
    add(tu("AP13S: Glaetten waehlen", [] { st().gewaehlt = kennung(torso(), 1); }));
    add(pause(0.4));
    add(merke(-1, vorher));
    add(ap3::insFeld(tr(Str::StpEntfernen)));
    add(pause(0.3));
    add(klickRoll(tr(Str::StpEntfernen)));
    add(pause(0.6));
    add(pruefe(zeile(nr, modus, "Entfernen: der Eintrag ist weg, die Flaeche bleibt, wie sie ohne ihn ist"), [=] {
        const auto& stapel = teilVon(torso()).stapel;
        return stapel.size() == 1 && stapel[0].art == ModArt::Spiegeln && st().gewaehlt == 0U;
    }));
    add(undoRedo(zeile(nr, modus, "Entfernen"), -1, vorher, [] { return std::string(tr(Str::StpAktEntfernen)); }));
    add(tu("AP13S: aufraeumen", [] {
        stapelLeeren();
        rollenNachOben();
    }));
    add(pause(0.5));
}

// --- W1: derselbe Stapel im Bereich Weights -----------------------------------------------------------------------------

void weightsSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("AP13S W1: wie 3ds Max, Bereich Weights, Flaeche torso, Knochen lhumerus", [] {
        stapelLeeren();
        figurWaehlen(0, ModellBereich::Weights);
        mtab().knochenWahl = boneIndex(g_app->anim, "lhumerus");
    }));
    add(pause(0.8));
    auto bildOhne = std::make_shared<mb1::AnsichtBild>();
    add(tu("AP13S W1: Bild der Gewichtsansicht ohne Eintrag", [=] { (void)mb1::ansichtLesen(*bildOhne); }));
    add(pruefe(zeile("W1", 0, "im Bereich Weights steht die Gruppe Modifier-Stapel zuoberst im Reiter Aendern: Skin (gewaehlt) ueber Edit Poly ueber der Flaeche"), [] {
        melde("Weights: Gruppen " + gruppen() + ", Zeilen " + zeilen());
        return feldFrisch() && ui::rolloutProbeZahl() > 0 && ui::rolloutProbe(0).titel == tr(Str::MgRStapel) &&
               zeilen() == liste({tr(Str::MgStapelSkin), tr(Str::EaUnwrap), tr(Str::MgStapelPoly), nBasis("torso")}) && gewaehlteZeile() == tr(Str::MgStapelSkin);
    }));
    add(tu("AP13S W1: TurboSmooth hinzufuegen", [] { (void)modellStapelHinzufuegen(ModArt::Glaetten); }));
    add(pause(0.8));
    add(pruefe(zeile("W1", 0, "ein Eintrag steht zwischen Edit Poly und Skin, die Gewichtsansicht zeichnet die geglaettete Flaeche (das Bild aendert sich)"), [=] {
        mb1::AnsichtBild jetzt;
        (void)mb1::ansichtLesen(jetzt);
        const int anders = mb1::unterschied(bildOhne->px, jetzt.px);
        melde("Gewichtsansicht: Bildpunkte anders " + std::to_string(anders));
        return zeilen() == liste({tr(Str::MgStapelSkin), nGlaetten(false), tr(Str::EaUnwrap), tr(Str::MgStapelPoly), nBasis("torso")}) && modellStapelErsetzt(torso()) &&
               dreieckeGezeichnet(torso()) == netzVon(torso()).dreiecke() * 6 && anders > 50;
    }));
    add(pruefe(zeile("W1", 0, "jede Ecke der Vorschau traegt hoechstens vier Knochen mit Summe 1 (wie die Datei)"), [] {
        const GlmSurface& g = modellStapelZeichnung().surfaces[uz(torso())];
        int schlecht = 0;
        for (const GlmVertex& v : g.verts) {
            const float summe = v.weights[0] + v.weights[1] + v.weights[2] + v.weights[3];
            schlecht += std::fabs(summe - 1.0F) > 1e-4F ? 1 : 0;
        }
        return modellStapelErsetzt(torso()) && !g.verts.empty() && schlecht == 0;
    }));
    add(klickRoll(tr(Str::MgStapelPoly)));
    add(pause(0.5));
    add(pruefe(zeile("W1", 0, "ein Klick auf Edit Poly wechselt in den Bereich Modellieren - dort ist Edit Poly gewaehlt und der Eintrag steht weiter da"), [] {
        return mtab().bereich == ModellBereich::Modellieren && gewaehlteZeile() == tr(Str::MgStapelPoly) &&
               zeilen() == liste({tr(Str::MgStapelSkin), nGlaetten(false), tr(Str::EaUnwrap), tr(Str::MgStapelPoly), nBasis("torso")});
    }));
    add(klickRoll(tr(Str::MgStapelSkin)));
    add(pause(0.5));
    add(pruefe(zeile("W1", 0, "ein Klick auf Skin wechselt in den Bereich Weights - dort ist Skin gewaehlt"), [] {
        return mtab().bereich == ModellBereich::Weights && gewaehlteZeile() == tr(Str::MgStapelSkin);
    }));
    // wie Blender zeigt der Entwurf im Bereich Weights (Reiter Daten) keinen Stapel; er steht im Reiter Modifier
    add(tu("AP13S W1: wie Blender, Bereich Weights", [] { figurWaehlen(1, ModellBereich::Weights); }));
    add(pause(0.8));
    add(pruefe(zeile("W1", 1, "im Reiter Daten (Vorgabe des Bereichs) steht kein Stapel, der Reiter Modifier ist waehlbar"), [] {
        return feldFrisch() && ui::feldProbe().reiterAktiv == 3 && gruppe(tr(Str::StpGruppeBl)) == nullptr && (ui::feldProbe().reiterBelegt & ui::reiterBit(2)) != 0U;
    }));
    add(tu("AP13S W1: Reiter Modifier", [] { modellFeldReiter(MbReiter::Aendern, MbReiterBl::Modifier); }));
    add(pause(0.6));
    add(pruefe(zeile("W1", 1, "im Reiter Modifier steht die Gruppe Modifier mit dem Eintrag Subdivision (nur Vorschau)"), [] {
        return gruppe(tr(Str::StpGruppeBl)) != nullptr && zeilen() == nGlaetten(true);
    }));
    add(tu("AP13S W1: aufraeumen", [] {
        stapelLeeren();
        mtab().knochenWahl = -1;
        figurWaehlen(0, ModellBereich::Modellieren);
    }));
    add(pause(0.5));
}

// --- Datei: leerer Stapel = dieselben Bytes ---------------------------------------------------------------------------------

std::string speichernUndLesen(const std::string& name) {
    const std::string ordner = mb1::fotoOrdner();
    if (ordner.empty()) { return {}; }
    const std::string pfad = ordner + "\\" + name;
    std::error_code ec;
    std::filesystem::remove(pfad, ec);
    mtab().speicherPfad = pfad;
    modellSpeichern(false);
    return slurp(pfad);
}

void dateiSchritte(std::vector<Schritt>& s, const std::string& modell) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto original = std::make_shared<std::string>();
    auto geaendert = std::make_shared<std::string>();
    auto gerechnet = std::make_shared<int>(0);   // Stand des Zaehlers "der Stapel rechnete fuer die Datei" zu Beginn
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    add(tu("AP13S Datei: wie 3ds Max, Modellieren, torso", [=] {
        figurWaehlen(0, ModellBereich::Modellieren);
        *gerechnet = st().dateiAuswertungen;
    }));
    add(pause(0.6));
    add(pruefe("AP13S Datei: der Selbsttest hat einen Ordner zum Speichern (BHED_FOTOS)", [] { return !mb1::fotoOrdner().empty(); }));
    add(pruefe("AP13S Datei: leerer Stapel - Speichern schreibt die Datei des Spiels Byte fuer Byte", [=] {
        std::string spiel;
        (void)readFromArchives(modell, spiel);
        *original = speichernUndLesen("ap13s_leer.glm");
        const bool selbst = &modellStapelDateiInhalt() == &mtab().dok.inhalt();
        melde("Datei leer: " + std::to_string(original->size()) + " Bytes, Pruefsumme " + hex(fnv(original->data(), original->size())) + ", Spiel " +
              std::to_string(spiel.size()) + " Bytes " + hex(fnv(spiel.data(), spiel.size())) + ", Speichern schreibt das Dokument selbst " + std::to_string(selbst ? 1 : 0));
        return !spiel.empty() && *original == spiel && selbst && st().dateiAuswertungen == *gerechnet;
    }));
    add(tu("AP13S Datei: eine Ecke von torso um 2 nach vorn (ein Schritt des Dokuments)", [] {
        mtab().dok.aendern("AP13S Ecke", [](modell::Inhalt& x) {
            for (modell::Netz& n : x.teile[uz(x.teilIndex("torso"))].stufen) {
                // alle Ecken des Punkts (an UV-Naehten fuehrt die Datei mehrere am selben Ort)
                for (std::size_t e = 0; e < n.ecken.size(); ++e) {
                    if (n.punktVon[e] != n.punktVon[0]) { continue; }
                    n.ecken[e].ort[1] += 2.0F;
                    n.ecken[e].ausDatei = false;
                    n.ausDatei = false;
                }
            }
        });
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(pruefe("AP13S Datei: geaendertes Modell, leerer Stapel - Speichern schreibt die Bytes von glmSchreiben(Dokument)", [=] {
        *geaendert = speichernUndLesen("ap13s_geaendert.glm");
        std::string direkt;
        const bool ok = modell::glmSchreiben(mtab().dok.inhalt(), direkt);
        melde("Datei geaendert: " + std::to_string(geaendert->size()) + " Bytes, Pruefsumme " + hex(fnv(geaendert->data(), geaendert->size())));
        return ok && !geaendert->empty() && *geaendert == direkt && *geaendert != *original && st().dateiAuswertungen == *gerechnet;
    }));
    add(tu("AP13S Datei: TurboSmooth (nur Vorschau) auf torso", [] { (void)modellStapelHinzufuegen(ModArt::Glaetten); }));
    add(pause(0.6));
    add(pruefe("AP13S Datei: ein Eintrag nur Vorschau - die Ansicht zeigt ihn, Speichern schreibt dieselben Bytes wie ohne ihn", [=] {
        const std::string mit = speichernUndLesen("ap13s_vorschau.glm");
        melde("Datei mit Vorschau-Eintrag: " + std::to_string(mit.size()) + " Bytes, Pruefsumme " + hex(fnv(mit.data(), mit.size())));
        return modellStapelErsetzt(torso()) && mit == *geaendert && st().dateiAuswertungen == *gerechnet;
    }));
    add(tu("AP13S Datei: Symmetry (X) auf torso - geht in die Datei", [] { (void)modellStapelHinzufuegen(ModArt::Spiegeln); }));
    add(pause(0.6));
    add(pruefe("AP13S Datei: ein Eintrag, der in die Datei geht - die gespeicherte Flaeche torso traegt das gespiegelte Netz, die Vorschau des Glaettens nicht", [=] {
        const modell::Teil& t = teilVon(torso());
        const modell::Netz soll = modell::stapelNetz(t, 0, modell::StapelZweck::Datei, modellStapelUmgebung());
        const std::string mit = speichernUndLesen("ap13s_spiegeln.glm");
        modell::Inhalt gelesen;
        std::string fehler;
        const bool ok = modell::ausGlm(mit, gelesen, &fehler);
        const int ti = ok ? gelesen.teilIndex("torso") : -1;
        const bool gleich = ti >= 0 && gelesen.teile[uz(ti)].stufen[0].ecken.size() == soll.ecken.size() && gelesen.teile[uz(ti)].stufen[0].indizes() == soll.indizes();
        melde("Datei mit Spiegeln: " + std::to_string(mit.size()) + " Bytes, torso " + (ti >= 0 ? std::to_string(gelesen.teile[uz(ti)].stufen[0].ecken.size()) : std::string("-")) +
              " Ecken, Soll " + std::to_string(soll.ecken.size()) + ", Dokument " + std::to_string(t.stufen[0].ecken.size()) + " " + fehler);
        return ok && gleich && mit != *geaendert && st().dateiAuswertungen > *gerechnet && soll.ecken.size() != t.stufen[0].ecken.size() && t.stapel.size() == 2;
    }));
    add(pruefe("AP13S Datei: beide Eintraege zurueckgenommen (Strg+Z, Strg+Z) - der Stapel ist leer und Speichern schreibt wieder die Bytes von vorher", [=] {
        (void)modellRueckgaengig();
        (void)modellRueckgaengig();
        const int davor = st().dateiAuswertungen;
        const std::string wieder = speichernUndLesen("ap13s_wieder.glm");
        return modell::stapelLeer(mtab().dok.inhalt()) && wieder == *geaendert && st().dateiAuswertungen == davor;
    }));
    add(pause(0.4));
}

// --- Fund: Symmetrisieren und die Knochen der anderen Seite -----------------------------------------------------------------

void fundSchritte(std::vector<Schritt>& s, const std::string& modell) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    // Ecken auf einer Seite der Flaeche, die an einem Knochen der ANDEREN Seite haengen. Wo links ist, misst die Figur des Spiels
    // (modell::linkeSeite: Schwerpunkt der Gewichte linker gegen rechter Knochen) - einmal vor dem Werkzeug.
    auto linksBei = std::make_shared<int>(0);
    const auto falscheSeite = [linksBei](int t, std::string* text) {
        std::vector<std::string> namen;
        for (const auto& b : g_app->anim.bones) { namen.push_back(b.name); }
        if (*linksBei == 0) { *linksBei = modell::linkeSeite(mtab().dok.inhalt(), 0, namen); }
        const int links = *linksBei;
        int falsch = 0;
        int seitig = 0;
        for (const modell::Ecke& e : netzVon(t).ecken) {
            if (std::fabs(e.ort[0]) < 0.5F) { continue; }
            const int seite = (e.ort[0] > 0.0F ? 1 : -1) * links;   // +1: die Ecke liegt links
            for (int k = 0; k < 4; ++k) {
                if (e.gewicht[k] < 0.25F) { continue; }
                const int ks = modell::knochenSeite(namen, e.knochen[k]);
                if (ks == 0) { continue; }
                ++seitig;
                falsch += ks != seite ? 1 : 0;
            }
        }
        if (text != nullptr) { *text = std::to_string(falsch) + " von " + std::to_string(seitig) + " Gewichten an Knochen einer Seite haengen an der falschen (links bei x " + (links > 0 ? ">" : "<") + " 0)"; }
        return falsch;
    };
    add(tu("AP13S Fund: wie 3ds Max, Modellieren, torso", [] { figurWaehlen(0, ModellBereich::Modellieren); }));
    add(pause(0.6));
    add(pruefe("AP13S Fund: die Figur des Spiels hat (fast) keine Gewichte an Knochen der falschen Seite", [=] {
        std::string text;
        const int falsch = falscheSeite(torso(), &text);
        melde("Fund vorher: " + text);
        return falsch < 20;
    }));
    add(tu("AP13S Fund: Symmetrisieren (Werkzeug des Modellierens), X, Plus nach Minus, uebernehmen", [] {
        // Polygone von torso gewaehlt: das Werkzeug spiegelt die Flaechen der Auswahl (ohne Auswahl alle)
        md1d::waehle(w::PAuswahl{{}, {}, ap6::vieleckeVon("torso", 4)}, AuswahlArt::Dreieck);
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Symmetrize);
        mz().achse = 0;
        mz().vonPlus = true;
        modellMeshVorschau();
        modellMeshUebernehmen(false);
    }));
    add(pause(0.6));
    add(pruefe("AP13S Fund: nach Symmetrisieren haengt die gespiegelte Seite an den Knochen ihrer Seite (lhumerus -> rhumerus)", [=] {
        std::string text;
        const int falsch = falscheSeite(torso(), &text);
        melde("Fund nach Symmetrisieren: " + text);
        return falsch < 20;
    }));
    add(tu("AP13S Fund: zuruecknehmen", [] { (void)modellRueckgaengig(); }));
    add(pause(0.4));
}

// --- Requisit: ein .md3 ohne Skelett -------------------------------------------------------------------------------------------

void requisitSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto vorher = std::make_shared<std::string>();
    md1d::neuesRequisit(s, "ap13s_kiste", "");
    md1d::kasten(s, 64.0F);
    add(tu("AP13S Requisit: nichts gewaehlt", [] {
        mtab().auswahl.clear();
        md1d::waehle(w::PAuswahl{}, AuswahlArt::Dreieck);
        g_app->settings.modellFeldBreite = 0.0F;
        rollenNachOben();
    }));
    add(pause(0.6));
    add(pruefe("AP13S Requisit: ohne Auswahl zeigt der Stapel die einzige Flaeche - Edit Poly (gewaehlt) ueber der MD3-Flaeche, kein Skin", [] {
        const int t = modellStapelTeil();
        melde("Requisit: Flaeche " + std::to_string(t) + ", Zeilen " + zeilen());
        return t == 0 && zeilen() == liste({tr(Str::EaUnwrap), tr(Str::MgStapelPoly), nBasis(teilVon(0).name, true)}) && gewaehlteZeile() == tr(Str::MgStapelPoly);
    }));
    add(merke(0, vorher));
    add(tu("AP13S Requisit: TurboSmooth", [] { (void)modellStapelHinzufuegen(ModArt::Glaetten); }));
    add(pause(0.6));
    add(pruefe("AP13S Requisit: der Kasten (12 Dreiecke) ist in der Ansicht geglaettet (48 Dreiecke), Rueckgaengig nimmt den Eintrag", [] {
        return netzVon(0).dreiecke() == 12 && dreieckeGezeichnet(0) == 48;
    }));
    add(undoRedo("AP13S Requisit: Hinzufuegen", 0, vorher));
    add(pruefe("AP13S Requisit: nur Vorschau - der Export (.md3) schreibt dieselben Bytes wie ohne Stapel; mit In die Datei schreiben das geglaettete Netz", [] {
        const int unbegrenzt = 0;
        std::string ohne;
        std::string mit;
        const modell::Inhalt& datei = modellStapelDateiInhalt();
        const bool selbst = &datei == &mtab().dok.inhalt();
        bool ok = modell::md3Schreiben(mtab().dok.inhalt(), ohne, unbegrenzt) && modell::md3Schreiben(datei, mit, unbegrenzt) && ohne == mit && selbst;
        (void)modellStapelSchalten(kennung(0, 0), true);
        const modell::Inhalt& datei2 = modellStapelDateiInhalt();
        std::string glatt;
        ok = ok && &datei2 != &mtab().dok.inhalt() && datei2.teile[0].stufen[0].dreiecke() == 48 && modell::md3Schreiben(datei2, glatt, unbegrenzt) && glatt != ohne;
        Md3Model gelesen;
        ok = ok && readMd3(glatt, gelesen) && !gelesen.surfaces.empty() && gelesen.surfaces[0].indexes.size() == 48U * 3U;
        (void)modellRueckgaengig();
        return ok;
    }));
    add(merke(0, vorher));
    add(pruefe("AP13S Requisit: Anwenden - das Netz des Dokuments ist das der Vorschau (26 Punkte, 48 Dreiecke), Edit Poly ist wieder gewaehlt", [] {
        const modell::Netz vorschau = modellStapelVorschauNetz(0);
        const bool ok = modellStapelAnwenden(kennung(0, 0));
        return ok && modell::netzGleich(netzVon(0), vorschau) && netzVon(0).punktZahl == 26 && netzVon(0).dreiecke() == 48 && teilVon(0).stapel.empty();
    }));
    add(undoRedo("AP13S Requisit: Anwenden", 0, vorher));
    add(pause(0.5));
    add(pruefe("AP13S Requisit: nach dem Anwenden arbeitet das Modellieren auf dem neuen Netz (24 Vielecke im Arbeitsnetz)", [] {
        modellMeshNetzAktualisieren();
        return mz().netz.lebendeFlaechen() == 24 && gewaehlteZeile() == tr(Str::MgStapelPoly);
    }));
}

// --- Finden: Suche und Menue ----------------------------------------------------------------------------------------------------

void findenSchritte(std::vector<Schritt>& s, int modus) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const bool bl = modus == 1;
    add(tu(std::string("AP13S Finden: ") + (bl ? "wie Blender" : "wie 3ds Max") + ", Bereich Ansehen/Pruefen, Flaeche torso", [=] {
        stapelLeeren();
        figurWaehlen(modus, ModellBereich::Ansehen);
    }));
    add(pause(0.6));
    add(tu("AP13S Finden: Alle Aktionen suchen", [] { mtab().aktionenOffen = true; }));
    add(pause(0.5));
    add(tippe("modifier"));
    add(pause(0.5));
    add(pruefe(zeile("Finden", modus, "\"Alle Aktionen suchen\" nennt die zehn Aktionen des Stapels, jede mit Tooltip"), [=] {
        int da = 0;
        const std::vector<std::string> soll = {
            fmt1(Str::StpAktHinzu, tr(bl ? Str::StpSpiegelnBl : Str::StpSpiegelnMax)), fmt1(Str::StpAktHinzu, tr(bl ? Str::StpGlaettenBl : Str::StpGlaettenMax)),
            tr(Str::StpAktAnAus), tr(Str::StpAktDatei), tr(Str::StpAktHoch), tr(Str::StpAktRunter), tr(bl ? Str::StpAktAnwendenBl : Str::StpAktAnwendenMax),
            tr(bl ? Str::StpAktAlleBl : Str::StpAktAlleMax), tr(Str::StpAktEntfernen), tr(Str::StpAktZeigen)};
        for (const std::string& x : soll) { da += imPopup(x) != nullptr ? 1 : 0; }
        melde("Suche \"modifier\": " + std::to_string(da) + " von " + std::to_string(soll.size()));
        return mtab().aktionenSichtbar && da == static_cast<int>(soll.size());
    }));
    add(tippe(tr(Str::StpAktZeigen)));
    add(pause(0.5));
    add(ap8::klickLetzter(tr(Str::StpAktZeigen), "##Popup_"));
    add(pause(0.8));
    add(pruefe(zeile("Finden", modus, "Modifier-Stapel zeigen: der Bereich Modellieren ist offen, der Reiter gewaehlt, die Gruppe im Bild"), [=] {
        const ui::RolloutProbe* g = gruppe(gruppenTitel(bl));
        const ImGuiWindow* fw = kindFenster("mtrollouts");
        return mtab().bereich == ModellBereich::Modellieren && ui::feldProbe().reiterAktiv == (bl ? 2 : 1) && g != nullptr && g->offen && fw != nullptr &&
               g->kopf.Min.y >= fw->InnerRect.Min.y - 1.0F && g->kopf.Max.y <= fw->InnerRect.Max.y + 1.0F;
    }));
    add(pruefe(zeile("Finden", modus, "ohne gewaehlten Eintrag sagen An/aus, Anwenden und Entfernen, dass keiner gewaehlt ist (kein Schritt)"), [] {
        const std::string davor = zustand(torso());
        const bool nichts = !modellStapelSchalten(0, false) && !modellStapelAnwenden(0) && !modellStapelEntfernen(0) && !modellStapelBewegen(0, 1);
        return nichts && zustand(torso()) == davor && mtab().meldung == tr(Str::StpMeldKeiner);
    }));
    add(pruefe(zeile("Finden", modus, "ohne Flaeche (ein Tag gewaehlt, kein Eintrag im Modell) gibt es keinen neuen Eintrag, die Gruppe sagt, was zu tun ist"), [] {
        const std::vector<int> auswahl = mtab().auswahl;
        mtab().auswahl.clear();
        md1d::waehle(w::PAuswahl{}, AuswahlArt::Dreieck);
        const bool keine = modellStapelTeil() < 0 && !modellStapelHinzufuegen(ModArt::Glaetten) && mtab().meldung == tr(Str::StpMeldKeineFlaeche);
        mtab().auswahl = auswahl;
        return keine;
    }));
    add(pause(0.3));
}

// --- Fotos -----------------------------------------------------------------------------------------------------------------------

void fotoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        const bool bl = modus == 1;
        const std::string p = bl ? "ap13s_blender_" : "ap13s_max_";
        add(tu(std::string("AP13S Foto: ") + (bl ? "wie Blender" : "wie 3ds Max") + ", Modellieren, torso, leerer Stapel", [=] {
            stapelLeeren();
            figurWaehlen(modus, ModellBereich::Modellieren);
            md1d::waehle(w::PAuswahl{}, AuswahlArt::Dreieck);
            setzeMaus(kv5::fokusPunkt());
        }));
        add(pause(0.9));
        add(foto(p + "leer"));
        add(tu("AP13S Foto: Spiegeln und Glaetten, Glaetten gewaehlt", [] {
            (void)modellStapelHinzufuegen(ModArt::Spiegeln);
            (void)modellStapelHinzufuegen(ModArt::Glaetten);
        }));
        add(pause(0.9));
        add(foto(p + "modell"));
        add(tu("AP13S Foto: Spiegeln gewaehlt", [] { st().gewaehlt = kennung(torso(), 0); }));
        add(pause(0.6));
        add(foto(p + "spiegeln"));
        add(klickRoll(tr(bl ? Str::StpHinzuBl : Str::StpHinzuMax)));
        add(pause(0.6));
        add(foto(p + "hinzufuegen"));
        add(mbefoto::popupsZu());
        add(pause(0.3));
        add(klickAn("Rechtsklick auf die Zeile Glaetten", [=] { return zeilenOrt(nGlaetten(bl)); }, 1));
        add(pause(0.6));
        add(foto(p + "rechtsklick"));
        add(mbefoto::popupsZu());
        add(pause(0.3));
        add(tu("AP13S Foto: Maus auf die Zeile Spiegeln (Tooltip)", [=] { setzeMaus(zeilenOrt(nSpiegeln(bl))); }));
        add(pause(1.6));
        add(tu("AP13S Foto: Maus bleibt", [=] { setzeMaus(zeilenOrt(nSpiegeln(bl))); }));
        add(pause(0.8));
        add(foto(p + "tooltip"));
        add(tu("AP13S Foto: Glaetten mit 2 Unterteilungen und in die Datei (ueber der Grenze des Spiels)", [] {
            setzeMaus(kv5::fokusPunkt());
            st().gewaehlt = kennung(torso(), 1);
            mtab().dok.aendern("AP13S Foto", [](modell::Inhalt& x) {
                auto& stapel = x.teile[uz(x.teilIndex("torso"))].stapel;
                if (stapel.size() < 2U) { return; }
                stapel[1].stufen = 2;
                stapel[1].datei = true;
            });
            g_app->modelDirty = true;
        }));
        add(pause(0.9));
        add(foto(p + "grenze"));
        add(tu("AP13S Foto: Auge des Glaettens aus", [] {
            (void)modellRueckgaengig();
            (void)modellStapelSchalten(kennung(torso(), 1), false);
        }));
        add(pause(0.8));
        add(foto(p + "aus"));
        add(tu("AP13S Foto: Bereich Weights, Knochen lhumerus", [=] {
            (void)modellStapelSchalten(kennung(torso(), 1), false);
            st().gewaehlt = 0;
            figurWaehlen(modus, ModellBereich::Weights);
            mtab().knochenWahl = boneIndex(g_app->anim, "lhumerus");
            if (bl) { modellFeldReiter(MbReiter::Aendern, MbReiterBl::Modifier); }
        }));
        add(pause(0.9));
        if (bl) {
            add(tu("AP13S Foto: wie Blender der Reiter Modifier", [] { modellFeldReiter(MbReiter::Aendern, MbReiterBl::Modifier); }));
            add(pause(0.6));
        }
        add(foto(p + "weights"));
        add(tu("AP13S Foto: zurueck", [=] {
            mtab().knochenWahl = -1;
            stapelLeeren();
            figurWaehlen(modus, ModellBereich::Modellieren);
        }));
        add(pause(0.4));
    }
    // ein Requisit: der Kasten, daneben geglaettet
    md1d::neuesRequisit(s, "ap13s_kiste", "");
    md1d::kasten(s, 64.0F);
    add(tu("AP13S Foto: Requisit, leerer Stapel", [] {
        g_app->settings.modellFeldBreite = 0.0F;
        mtab().auswahl.clear();
        md1d::waehle(w::PAuswahl{}, AuswahlArt::Dreieck);
        rollenNachOben();
        setzeMaus(kv5::fokusPunkt());
    }));
    add(pause(0.6));
    add(foto("ap13s_max_requisit"));
    add(tu("AP13S Foto: Requisit mit TurboSmooth, 2 Unterteilungen", [] {
        (void)modellStapelHinzufuegen(ModArt::Glaetten);
        mtab().dok.aendern("AP13S Foto", [](modell::Inhalt& x) {
            if (!x.teile[0].stapel.empty()) { x.teile[0].stapel[0].stufen = 2; }
        });
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(foto("ap13s_max_requisit_glatt"));
    add(tu("AP13S Foto: angewendet", [] { (void)modellStapelAnwenden(kennung(0, 0)); }));
    add(pause(0.8));
    add(foto("ap13s_max_requisit_angewendet"));
}

// Die zwei Bilder fuer das Handbuch (BHED_MODELL_NUR=ap13shb): der Stapel wie 3ds Max und wie Blender, Flaeche torso mit Spiegeln
// und Glaetten, Glaetten gewaehlt.
void handbuchFotos(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        add(tu(std::string("AP13S Handbuch: Stapel ") + (modus == 0 ? "wie 3ds Max" : "wie Blender"), [=] {
            stapelLeeren();
            g_app->settings.modellAnzeigeUmzugGesehen = true;
            figurWaehlen(modus, ModellBereich::Modellieren);
            md1d::waehle(w::PAuswahl{}, AuswahlArt::Dreieck);
            (void)modellStapelHinzufuegen(ModArt::Spiegeln);
            (void)modellStapelHinzufuegen(ModArt::Glaetten);
            setzeMaus(kv5::fokusPunkt());
        }));
        add(pause(1.0));
        add(foto(modus == 0 ? "h_ap13s_max" : "h_ap13s_blender"));
    }
    add(tu("AP13S Handbuch: aufraeumen", [] { stapelLeeren(); }));
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "ap13s" && n != "ap13sfoto" && n != "ap13shb") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    const char* fenster = std::getenv("BHED_AP13S_FENSTER");
    if (fenster != nullptr) { (void)std::sscanf(fenster, "%dx%d@%f", &b, &h, &z); }
    // die Handbuchbilder in der Groesse der uebrigen (Fenster und Oberflaeche des Laufs), sonst wie der Foto-Selbsttest mbefoto
    const bool fensterSetzen = n != "ap13shb" || fenster != nullptr;
    const char* m = std::getenv("BHED_AP13S_MODELL");
    const std::string modell = m != nullptr ? m : "models/players/kyle/model.glm";
    auto skala = std::make_shared<float>(1.0F);
    auto thema = std::make_shared<const theme::Theme*>(nullptr);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto feldBreite = std::make_shared<float>(0.0F);
    add(tu("AP13S: Fenster, Thema, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        *thema = g_app->activeTheme;
        if (fensterSetzen) {
            (void)platform::hauptfensterGroesse(b, h);
            g_app->uiScale = z;
        }
        if (const char* t = std::getenv("BHED_AP13S_THEMA")) {
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
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap13s.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    add(tu("AP13S: Aufteilung wie neue Einstellungen, Sequenz, wie 3ds Max", [=] {
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
    add(klickAn("AP13S: in die Ansicht (Fokus)", mbefoto::ansichtEcke));
    add(pause(0.4));
    if (n == "ap13s") {
        stapelSchritte(s, 0);
        stapelSchritte(s, 1);
        weightsSchritte(s);
        findenSchritte(s, 0);
        findenSchritte(s, 1);
        dateiSchritte(s, modell);
        fundSchritte(s, modell);
        add(tu("AP13S: wie 3ds Max fuer das Requisit", [] { modellBedienungSetzen(0); }));
        requisitSchritte(s);
    } else if (n == "ap13sfoto") {
        fotoSchritte(s);
    } else {
        handbuchFotos(s);
    }
    add(tu("AP13S: Modell-Tab zurueck", [=] {
        if (mz().kaestchenOffen) { modellMeshAbbrechen(); }
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->settings.modellFeldBreite = *feldBreite;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = false;
        g_app->mtab.knochenWahl = -1;
        g_app->mtab.speicherPfad.clear();
        // das Modell des Tests ist nicht gespeichert: als ungeaendert merken (Beenden fragt sonst nach dem Modell)
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

}  // namespace ap13s
