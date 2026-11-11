// selbsttest_pt.inc - Modell-Block 1c: Texture Painting (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=pt | ptspiel | pthb)
//
//   pt        Sturmtruppler (Spieldaten), Bereich Painting, wie 3ds Max:
//     Bild            die Leinwand der Flaeche torso hat Groesse und Format der Datei, die die Engine laedt; das 2D-Bild
//                     steht links neben der Ansicht
//     Erreichbarkeit  jede Aktion im Menue "Painting" (Untermenue je Gruppe) mit Tooltip, im Rechtsklick der Ansicht, in
//                     der Suche (X); die Werkzeugknoepfe im Befehlsfeld
//     je Werkzeug     Malen in 3D (ein Strich = ein Rueckgaengig-Schritt, Strg+Z bringt das Bild Byte fuer Byte zurueck,
//                     die Vorschau-Textur der Flaeche aendert sich sofort), Spiegeln (mehr als 1,5 mal so viele Texel),
//                     in 2D Malen, Fuellen, Weichzeichnen, Verwischen, Klonen (Strg+Klick Quelle), Maske (geschuetzt),
//                     Radieren auf einer Ebene, Ebene neu/loeschen (das Original bleibt unberuehrt)
//     Tasten          wie Blender F (Radius ziehen, Enter), X (Farben tauschen), 1 (Maske zeigen)
//     Animation       ein Strich, waehrend die Animation laeuft (Pose aus dem GPU-Skinning, Tupfer in der Ruhelage)
//     Bauen           "Fuer das Spiel bauen" schreibt nur bemalte Bilder, unter ihrem Spielpfad, in Groesse und Format des
//                     Originals (JPEG mit dessen Tabellen); unbemalt schreibt es nichts
//   ptzeit    Bildzeit (allein, BHED_D3D_DEBUG=0): Ansehen, Painting, Painting mit einem Tupfer je Bild (Pinsel und
//             Weichzeichnen), Painting mit voller Maske gezeigt
//   ptfix     Pruefung seit rc610 P1, P3, P4;  ptfix2 (mit SPRACHE=ja) P8, P13, P19
//   ptfix3    PTFIX3: P23 Verwerfen, P24 Umbacken je Stufe, P25 Wiederholen, P29 ein Bild je Name ohne Endung, P30 Flaechen
//             der Leinwand, P31 neu geladene Stufen (Movie-Duels-Daten: Yoda, Yoda-Schatten)
//   ptstufen  alle Shader-Stufen wie im Spiel: Aayla (Movie Duels), torso.tga zeichnen die Shader torso und torso_cull -
//             nach einem Strich zeigt jede Stufe jeder Flaeche mit diesem Bild das bemalte, eine Leinwand je Datei, ein
//             gebautes Bild mit beiden Strichen
//   ptspiel   der Sturmtruppler mit einem roten Streifen ueber Brust und Ruecken als Mod in BHED_PAINT_MOD; Foto der Vorschau
//             von vorn fuer den Vergleich im Spiel (Movie Duels, eigener fs_homepath)
//   pthb      Handbuchbilder (BHED_HB_NUR=pt)
//   ptlang    MBMAL: viele Striche nacheinander (kyle body, Animation laeuft, allein und mit BHED_D3D_DEBUG=0) - Bildzeit beim
//             Malen ueber die Dauer eines Strichs und ueber die Striche, Speicher, Verlauf, wohin die Zeit geht;
//             BHED_PTLANG_FEST=1: feste Tupfer, Pruefsummen von Bild und gebauter Datei
//   ptsprung  MBMAL Punkt 5: ein Sprung zwischen zwei Koerperteilen malt nur am gezogenen Weg am Schirm (wie Substance
//             Painter und Blender), Strich auf einer Flaeche ohne Sprung nach ptsprung_flaeche.bin
// Eingebunden in selbsttest.cpp (nach selbsttest_md.inc).
namespace pt1c {

namespace mm = modell::texmalen;
using rig3::imMenue;
using rig3::klickMenue;
using rig3::woIst;

ModellMalen& mz() { return g_app->mtab.malen; }
const mm::Leinwand* lw() { return modellMalenLeinwand(); }

int teilVon(const std::string& name) { return g_app->mtab.dok.inhalt().teilIndex(name); }

int aktion(Str s) {
    for (int i = 0; i < modellMalenAktionZahl(); ++i) {
        if (std::string(modellMalenAktionText(i)) == tr(s)) { return i; }
    }
    return -1;
}

Schritt ausloesen(Str s) {
    return tu(std::string("PT: ") + tr(s), [s] {
        const int i = aktion(s);
        if (i < 0) {
            meldeFehler(std::string("PT: Aktion fehlt: ") + tr(s));
            return;
        }
        modellMalenAktionAusloesen(i);
    });
}

// geaenderte Texel des Ergebnisses gegen das Original
int geaendert(const mm::Leinwand* l) {
    if (l == nullptr) { return -1; }
    int n = 0;
    for (std::size_t i = 0; i + 3 < l->bild.size() && i + 3 < l->grund.size(); i += 4U) {
        n += (l->bild[i] != l->grund[i] || l->bild[i + 1] != l->grund[i + 1] || l->bild[i + 2] != l->grund[i + 2]) ? 1 : 0;
    }
    return n;
}

int unterschied(const std::vector<std::uint8_t>& a, const std::vector<std::uint8_t>& b) {
    int n = 0;
    for (std::size_t i = 0; i + 3 < a.size() && i + 3 < b.size(); i += 4U) {
        n += (a[i] != b[i] || a[i + 1] != b[i + 1] || a[i + 2] != b[i + 2]) ? 1 : 0;
    }
    return n;
}

ImVec2 torsoMitte(float dx) {
    ImVec2 m;
    if (!modellFlaecheAufSchirm(teilVon("torso"), m)) {
        meldeFehler("PT: torso nicht auf dem Schirm");
        return ImVec2{};
    }
    return ImVec2{m.x + dx, m.y};
}

ImVec2 bildPunkt(float fx, float fy) {
    const ModellMalen& z = g_app->mtab.malen;
    return ImVec2{z.leinwand[0] + z.leinwand[2] * fx, z.leinwand[1] + z.leinwand[3] * fy};
}

// alles zurueck auf das Original (zwischen den Werkzeugen)
Schritt zurueckAufOriginal() {
    return tu("PT: zurueck auf das Original (Rueckgaengig bis zum Anfang)", [] {
        for (int i = 0; i < 200 && modellMalenRueckgaengig(); ++i) {}
    });
}

Schritt werkzeug(Str s) {
    return ausloesen(s);
}

void figurLaden(std::vector<Schritt>& s, const char* modus, const char* modell = "models/players/stormtrooper/model.glm") {
    s.push_back(tu("PT: Skript fuer den Test", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "pt.txt"); }));
    for (Schritt& x : mb1::laden(modell)) { s.push_back(std::move(x)); }
    const std::string m = modus;
    s.push_back(tu("PT: Bereich Painting, " + m + ", eine Ansicht von vorn, LOD 0", [m] {
        modellBedienungSetzen(m == "blender" ? 1 : 0);
        ModellTab& t = g_app->mtab;
        t.bereich = ModellBereich::Painting;
        t.vierAnsichten = false;
        t.aktiv = 1;
        t.ansichten[1].art = AnsichtArt::Vorne;
        t.lodWahl = 1;
        g_app->animPlaying = false;
        mz().editorOffen = true;
        mz().radiusPx = 18.0F;
        mz().pinsel.staerke = 1.0F;
        mz().pinsel.haerte = 0.5F;
        mz().pinsel.farbe = {1.0F, 0.0F, 0.0F, 1.0F};
        g_app->modelDirty = true;
    }));
    s.push_back(pause(0.5));
    s.push_back(tu("PT: einpassen", [] { modellAnsichtEinpassenAktiv(); }));
    s.push_back(pause(0.8));
}

Schritt gruppeVoll(int gruppe, const std::string& wo) {
    return pruefe("PT " + wo + ": jede Aktion der Gruppe " + std::to_string(gruppe) + " steht da, mit Tooltip", [gruppe, wo] {
        int n = 0;
        for (int i = 0; i < modellMalenAktionZahl(); ++i) {
            if (modellMalenAktionGruppe(i) != gruppe) { continue; }
            const std::string text = modellMalenAktionText(i);
            if (imMenue(text) == nullptr || std::string(modellMalenAktionHinweis(i)).size() < 10U) {
                meldeFehler("PT: fehlt im Menue oder ohne Tooltip: " + text + " " + woIst(text));
                return false;
            }
            ++n;
        }
        diag::detail("Knopftest: PT " + wo + " Gruppe " + std::to_string(gruppe) + " - " + std::to_string(n) + " Aktionen mit Tooltip");
        return n >= 3;
    });
}

const Str kGruppen[5] = {Str::PtGrWerkzeuge, Str::PtGrPinsel, Str::PtGrEbenen, Str::PtGrMaske, Str::PtGrBild};

void menueAlle(std::vector<Schritt>& s, bool rechtsklick) {
    for (int g = 0; g < 5; ++g) {
        s.push_back(taste(ImGuiKey_Escape, "Esc"));
        s.push_back(taste(ImGuiKey_Escape, "Esc"));
        s.push_back(pause(0.3));
        if (rechtsklick) {
            s.push_back(klickAn("Rechtsklick in die Ansicht", [] { return kv5::fokusPunkt(); }, 1));
            s.push_back(pause(0.3));
            s.push_back(klickMenue(tr(Str::PtMenue), false));
        } else {
            s.push_back(klickAn("\"Painting\" (Menueleiste, falls zu)", [] {
                if (imMenue(tr(kGruppen[0])) != nullptr) { return imMenue(tr(kGruppen[0]))->rect.GetCenter(); }
                const Element* e = rig3::menueLeiste(tr(Str::PtMenue));
                if (e == nullptr) {
                    meldeFehler("PT: Menue Painting nicht in der Leiste");
                    return ImVec2{};
                }
                return e->rect.GetCenter();
            }));
        }
        s.push_back(pause(0.3));
        s.push_back(klickMenue(tr(kGruppen[g]), false));
        s.push_back(pause(0.3));
        s.push_back(gruppeVoll(g, rechtsklick ? "Rechtsklick > Painting" : "Menue Painting"));
    }
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
}

void ueberSuche(std::vector<Schritt>& s, Str text) {
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
    s.push_back(tu("PT: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; }));
    s.push_back(ik6::tasteSuche());
    s.push_back(pause(0.2));
    s.push_back(pruefe("PT Suche: Alle Aktionen suchen ist offen (X / F3)", [] { return g_app->mtab.aktionenSichtbar; }));
    const std::string zeile = tr(text);
    s.push_back(tippe(zeile));
    s.push_back(pause(0.2));
    s.push_back(klickAn("\"" + zeile + "\" in der Suche", [zeile] {
        for (const Element& e : g_elemente) {
            if ((e.label == zeile || e.label.rfind(zeile + " (", 0) == 0) && e.fenster.find("##Popup_") != std::string::npos) { return e.rect.GetCenter(); }
        }
        meldeFehler("PT: " + zeile + " nicht in der Suche: " + woIst(zeile));
        return ImVec2{};
    }));
    s.push_back(pause(0.3));
}

struct Stand {
    std::vector<std::uint8_t> bild;
    std::vector<std::uint8_t> vorschau;
    std::vector<std::uint8_t> ebene0;
    std::vector<std::uint8_t> ansicht;   // REST Punkt 1: das Bild der aktiven 3D-Ansicht (Renderziel)
    int n = 0;
    int n1 = 0;
    float radius = 0.0F;
    std::size_t schritte = 0;
};

std::vector<std::uint8_t> vorschauTorso() {
    const int t = teilVon("torso");
    const auto& by = g_app->modelTextures.bySurface;
    return t >= 0 && static_cast<std::size_t>(t) < by.size() ? by[static_cast<std::size_t>(t)].rgba : std::vector<std::uint8_t>{};
}

// das Bild der aktiven 3D-Ansicht, wie es auf dem Schirm steht (Renderziel der Ansicht, gpu::leseModellZiel)
std::vector<std::uint8_t> ansichtBild() {
    std::vector<std::uint8_t> px;
    int w = 0;
    int h = 0;
    gpu::modellZielWaehlen(g_app->mtab.aktiv);
    if (!gpu::leseModellZiel(px, w, h)) { px.clear(); }
    return px;
}

// REST Punkt 1: die Textur der Flaeche torso auf der Grafikkarte (alle Stufen, zurueckgelesen) gegen das ganze Bild samt
// Pyramide (Tex::buildMips, wie beim ganzen Hochladen) und die Bildpunkte der Vorschau gegen das Ergebnis der Leinwand
Schritt grafikGleich(const std::string& wann, const std::string& flaeche = "torso") {
    return pruefe("PT Grafikkarte " + wann + ": Texturen " + flaeche + " (Flaeche und Shader-Stufen, alle Pyramidenstufen) gleich dem ganz hochgeladenen Bild, Vorschau = Ergebnis", [wann, flaeche] {
        const int t = teilVon(flaeche);
        const auto& by = g_app->modelTextures.bySurface;
        const mm::Leinwand* l = lw();
        if (t < 0 || static_cast<std::size_t>(t) >= by.size() || l == nullptr) {
            meldeFehler("PT Grafikkarte " + wann + ": kein Bild");
            return false;
        }
        // gleich: jede hochgeladene Textur Stufe fuer Stufe gleich Tex::buildMips ihrer Bildpunkte
        int texturen = 0;
        int gleich = 0;
        const auto vergleiche = [&](const TextureSet::Tex& x) {
            TextureSet::Tex ganz;
            ganz.width = x.width;
            ganz.height = x.height;
            ganz.rgba = x.rgba;
            ganz.buildMips();
            const int stufen = 1 + static_cast<int>(ganz.mips.size());
            int ok = 0;
            for (int st = 0; st < stufen; ++st) {
                std::vector<std::uint8_t> px;
                int w = 0;
                int h = 0;
                int sw = 0;
                int sh = 0;
                const std::uint8_t* soll = ganz.levelData(st, sw, sh);
                if (!gpu::leseFigurBild(&x, st, px, w, h)) { return; }   // nicht hochgeladen (nicht gezeichnet)
                ok += w == sw && h == sh && std::memcmp(px.data(), soll, px.size()) == 0 ? 1 : 0;
            }
            ++texturen;
            gleich += ok == stufen ? 1 : 0;
        };
        vergleiche(by[static_cast<std::size_t>(t)]);
        int stufenMitBild = 0;   // Shader-Stufen, die das Ergebnis der Leinwand zeigen
        const auto& sf = g_app->modelTextures.stufen;
        if (static_cast<std::size_t>(t) < sf.size()) {
            for (const auto& fs : sf[static_cast<std::size_t>(t)]) {
                if (fs.pixel.size() != 1U || fs.pixel[0] == nullptr) { continue; }
                vergleiche(*fs.pixel[0]);
                stufenMitBild += fs.pixel[0]->rgba == l->bild ? 1 : 0;
            }
        }
        const bool stufenDa = static_cast<std::size_t>(t) < sf.size() && !sf[static_cast<std::size_t>(t)].empty();
        const bool vorschau = by[static_cast<std::size_t>(t)].width != l->b || by[static_cast<std::size_t>(t)].rgba == l->bild;
        diag::detail("Knopftest: PT Grafikkarte " + wann + " - " + std::to_string(gleich) + " von " + std::to_string(texturen) + " Texturen gleich, Stufen mit dem Bild " +
                     std::to_string(stufenMitBild) + (stufenDa ? "" : " (keine Shader-Stufen)") + ", Vorschau = Ergebnis " + std::to_string(vorschau ? 1 : 0));
        return texturen > 0 && gleich == texturen && vorschau && (!stufenDa || stufenMitBild > 0);
    });
}

std::vector<Schritt> modellSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<Stand>();
    figurLaden(s, "max");
    // --- Bild ---
    add(tu("PT: Gruppe der Flaeche torso", [] {
        const int t = teilVon("torso");
        g_app->mtab.auswahl = {t};
        mz().gruppe = -1;
    }));
    add(pause(0.5));
    add(pruefe("PT Bild: Leinwand der Flaeche torso in Groesse und Format der Datei des Spiels, 2D-Bild neben der Ansicht", [] {
        const mm::Leinwand* l = lw();
        if (l == nullptr) {
            meldeFehler("PT: keine Leinwand fuer torso - " + mz().meldung);
            return false;
        }
        std::string bytes;
        const bool da = readFromArchives(l->datei, bytes);
        const auto* d = reinterpret_cast<const unsigned char*>(bytes.data());
        const image::Image im = image::decode(d, bytes.size());
        const bool gleich = da && im.ok && im.width == l->b && im.height == l->h && bytes == l->original &&
                            static_cast<int>(image::sniff(d, bytes.size())) == l->format;
        diag::detail("Knopftest: PT Bild - " + l->datei + " " + std::to_string(l->b) + "x" + std::to_string(l->h) + " " +
                     image::formatName(static_cast<image::Format>(l->format)) + ", " + std::to_string(l->dreiecke.size()) + " Dreiecke, 2D " +
                     std::to_string(static_cast<int>(mz().leinwand[2])) + "x" + std::to_string(static_cast<int>(mz().leinwand[3])));
        return gleich && l->dreiecke.size() > 100U && mz().leinwand[2] > 100.0F && mz().leinwand[3] > 100.0F;
    }));
    // --- Erreichbarkeit ---
    menueAlle(s, false);
    menueAlle(s, true);
    for (const Str t : {Str::PtWzKlonen, Str::PtEbeneNeu}) {
        ueberSuche(s, t);
        if (t == Str::PtWzKlonen) {
            add(pruefe("PT Suche: Klonen ist das Werkzeug", [] { return mz().werkzeug == static_cast<int>(mm::Werkzeug::Klonen); }));
        } else {
            add(pruefe("PT Suche: Neue Ebene legt eine Ebene ueber dem Original an", [] { return lw() != nullptr && lw()->ebenen.size() == 2U && lw()->aktiv == 1; }));
        }
    }
    add(zurueckAufOriginal());
    // AP9: "Viewport Canvas" ist seit dem Umbau nach dem Entwurf kein Gruppentitel mehr - die Werkzeuge stehen in der Gruppe
    // Werkzeuge, deren Kurzinfo das Vorbild nennt
    add(pruefe("PT Befehlsfeld: Gruppe Werkzeuge (Kurzinfo Viewport Canvas) mit allen Werkzeugknoepfen, Fuer das Spiel bauen", [] {
        int n = 0;
        for (const Str w : {Str::PtWzPinsel, Str::PtWzKlonen, Str::PtWzWischen, Str::PtWzWeich, Str::PtWzFuellen, Str::PtWzMaske, Str::PtWzRadierer}) {
            for (const Element& e : g_elemente) {
                if (e.label == tr(w) && e.innen.find("mtrollouts") != std::string::npos) {
                    ++n;
                    break;
                }
            }
        }
        bool roll = false;
        for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
            roll = roll || (ui::rolloutProbe(i).titel == tr(Str::PtGrWerkzeuge) && ui::rolloutProbe(i).kurzinfo == tr(Str::PtRolloutMax));
        }
        bool bau = false;
        for (const Element& e : g_elemente) { bau = bau || e.label == tr(Str::PtBauen); }
        diag::detail("Knopftest: PT Befehlsfeld - " + std::to_string(n) + "/7 Werkzeuge, Rollout " + std::to_string(roll ? 1 : 0) + ", Bauen " + std::to_string(bau ? 1 : 0));
        return n == 7 && roll;
    }));
    // "Fuer das Spiel bauen" steht unten im Befehlsfeld (je nach Sprache und Breite ausserhalb des Sichtbaren): hinrollen
    const auto rollen = [](bool ansEnde) {
        return tu(ansEnde ? "PT Befehlsfeld: ans Ende rollen" : "PT Befehlsfeld: zurueck nach oben", [ansEnde] {
            for (ImGuiWindow* w : GImGui->Windows) {
                if (std::strstr(w->Name, "mtrollouts") != nullptr) { ImGui::SetScrollY(w, ansEnde ? w->ScrollMax.y : 0.0F); }
            }
        });
    };
    add(rollen(true));
    add(pause(0.3));
    add(pruefe("PT Befehlsfeld: Fuer das Spiel bauen", [] {
        for (const Element& e : g_elemente) {
            if (e.label == tr(Str::PtBauen) && e.innen.find("mtrollouts") != std::string::npos) { return true; }
        }
        return false;
    }));
    add(rollen(false));
    add(pause(0.2));
    // --- Malen in 3D ---
    add(werkzeug(Str::PtWzPinsel));
    add(tu("PT 3D: vorher", [st] {
        st->vorschau = vorschauTorso();
        st->ansicht = ansichtBild();
        st->schritte = 0;
    }));
    add(mb3::ziehenVonNach("PT 3D: Strich ueber die Brust", [] { return torsoMitte(-30.0F); }, [] { return torsoMitte(30.0F); }));
    add(pause(0.3));
    add(pruefe("PT 3D Malen: Texel rot, Vorschau-Textur der Flaeche sofort neu, ein Strich", [st] {
        const int n = geaendert(lw());
        const int v = unterschied(st->vorschau, vorschauTorso());
        st->n = n;
        diag::detail("Knopftest: PT 3D Malen - " + std::to_string(n) + " Texel geaendert, Vorschau " + std::to_string(v) + " Texel, " + std::to_string(mz().tupfer) +
                     " Tupfer | " + mz().meldung);
        return n > 50 && v > 0 && !mz().strichLaeuft;
    }));
    // REST Punkt 1: die 3D-Ansicht zeigt den Strich sofort (das Bild der Ansicht ist neu gezeichnet, nicht nur die Textur)
    add(pruefe("PT 3D Malen: die Ansicht zeigt den Strich (Bild der Ansicht neu gezeichnet)", [st] {
        const std::vector<std::uint8_t> jetzt = ansichtBild();
        const int d = unterschied(st->ansicht, jetzt);
        diag::detail("Knopftest: PT 3D Ansicht - " + std::to_string(d) + " Bildpunkte der Ansicht anders, " + std::to_string(jetzt.size() / 4U) + " Bildpunkte");
        return !jetzt.empty() && jetzt.size() == st->ansicht.size() && d > 20;
    }));
    add(grafikGleich("nach dem 3D-Strich"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("PT Rueckgaengig: Strg+Z bringt das Bild Byte fuer Byte zurueck", [] {
        diag::detail("Knopftest: PT Strg+Z - " + std::to_string(geaendert(lw())) + " Texel anders als das Original | " + mz().meldung);
        return lw() != nullptr && lw()->bild == lw()->grund;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Y, "Strg+Y"));
    add(pause(0.3));
    add(pruefe("PT Wiederholen: Strg+Y malt den Strich wieder", [st] { return geaendert(lw()) == st->n; }));
    add(zurueckAufOriginal());
    // --- Spiegeln ---
    add(klickAn("PT Spiegeln: ein Tupfer ohne Spiegeln", [] { return torsoMitte(-25.0F); }));
    add(pause(0.3));
    add(tu("PT Spiegeln: zaehlen", [st] { st->n1 = geaendert(lw()); }));
    add(zurueckAufOriginal());
    add(ausloesen(Str::PtSpiegeln));
    add(klickAn("PT Spiegeln: derselbe Tupfer mit Spiegeln", [] { return torsoMitte(-25.0F); }));
    add(pause(0.3));
    add(pruefe("PT Spiegeln: der gespiegelte Tupfer malt die andere Seite mit (mehr als 1,5 mal so viele Texel)", [st] {
        const int n2 = geaendert(lw());
        diag::detail("Knopftest: PT Spiegeln - ohne " + std::to_string(st->n1) + ", mit " + std::to_string(n2) + " Texel");
        return st->n1 > 0 && n2 > st->n1 * 3 / 2;
    }));
    add(ausloesen(Str::PtSpiegeln));
    add(zurueckAufOriginal());
    // --- 2D ---
    add(werkzeug(Str::PtWzPinsel));
    add(pause(0.2));
    add(tu("PT 2D: Ansicht vorher", [st] { st->ansicht = ansichtBild(); }));
    add(mb3::ziehenVonNach("PT 2D: Strich im Bild", [] { return bildPunkt(0.3F, 0.5F); }, [] { return bildPunkt(0.7F, 0.5F); }));
    add(pause(0.3));
    add(pruefe("PT 2D Malen: Texel geaendert", [st] {
        st->n = geaendert(lw());
        diag::detail("Knopftest: PT 2D Malen - " + std::to_string(st->n) + " Texel");
        return st->n > 50;
    }));
    // REST Punkt 1: ein Strich im 2D-Bild erscheint auch in der 3D-Ansicht
    add(pruefe("PT 2D Malen: die 3D-Ansicht zeigt den Strich", [st] {
        const std::vector<std::uint8_t> jetzt = ansichtBild();
        const int d = unterschied(st->ansicht, jetzt);
        diag::detail("Knopftest: PT 2D Ansicht - " + std::to_string(d) + " Bildpunkte der 3D-Ansicht anders");
        return !jetzt.empty() && jetzt.size() == st->ansicht.size() && d > 20;
    }));
    add(werkzeug(Str::PtWzWeich));
    add(tu("PT 2D: vorher", [st] { st->bild = lw() != nullptr ? lw()->bild : std::vector<std::uint8_t>{}; }));
    add(mb3::ziehenVonNach("PT 2D Weichzeichnen ueber den Strich", [] { return bildPunkt(0.5F, 0.45F); }, [] { return bildPunkt(0.5F, 0.55F); }));
    add(pause(0.3));
    add(pruefe("PT 2D Weichzeichnen: Texel geaendert", [st] { return lw() != nullptr && unterschied(st->bild, lw()->bild) > 10; }));
    add(werkzeug(Str::PtWzWischen));
    add(tu("PT 2D: vorher", [st] { st->bild = lw() != nullptr ? lw()->bild : std::vector<std::uint8_t>{}; }));
    add(mb3::ziehenVonNach("PT 2D Verwischen quer zum Strich", [] { return bildPunkt(0.4F, 0.5F); }, [] { return bildPunkt(0.4F, 0.62F); }));
    add(pause(0.3));
    add(pruefe("PT 2D Verwischen: Texel geaendert", [st] { return lw() != nullptr && unterschied(st->bild, lw()->bild) > 10; }));
    add(werkzeug(Str::PtWzKlonen));
    add(tu("PT 2D: Strg halten", [] { ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, true); }));
    add(klickAn("PT 2D Klonen: Strg+Klick setzt die Quelle auf den Strich", [] { return bildPunkt(0.5F, 0.5F); }));
    add(tu("PT 2D: Strg los", [] { ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, false); }));
    add(pause(0.2));
    add(tu("PT 2D: vorher", [st] { st->bild = lw() != nullptr ? lw()->bild : std::vector<std::uint8_t>{}; }));
    add(mb3::ziehenVonNach("PT 2D Klonen: Strich weiter unten", [] { return bildPunkt(0.3F, 0.8F); }, [] { return bildPunkt(0.5F, 0.8F); }));
    add(pause(0.3));
    add(pruefe("PT 2D Klonen: Quelle gesetzt, Texel kopiert", [st] {
        const int n = lw() != nullptr ? unterschied(st->bild, lw()->bild) : 0;
        diag::detail("Knopftest: PT 2D Klonen - Quelle " + std::to_string(mz().klonQuelleDa ? 1 : 0) + ", " + std::to_string(n) + " Texel | " + mz().meldung);
        return mz().klonQuelleDa && n > 10;
    }));
    add(werkzeug(Str::PtWzFuellen));
    add(tu("PT 2D: vorher, Fuellfarbe gruen", [st] {
        st->bild = lw() != nullptr ? lw()->bild : std::vector<std::uint8_t>{};
        mz().pinsel.farbe = {0.0F, 1.0F, 0.0F, 1.0F};
    }));
    add(klickAn("PT 2D Fuellen: Klick", [] { return bildPunkt(0.5F, 0.5F); }));
    add(pause(0.3));
    add(pruefe("PT 2D Fuellen: die Flaeche gleicher Farbe gefuellt", [st] {
        const int n = lw() != nullptr ? unterschied(st->bild, lw()->bild) : 0;
        diag::detail("Knopftest: PT 2D Fuellen - " + std::to_string(n) + " Texel, gemeldet " + std::to_string(mz().texelGemalt));
        return n > 0 && mz().texelGemalt > 0;
    }));
    add(grafikGleich("nach den 2D-Werkzeugen"));
    add(zurueckAufOriginal());
    add(pause(0.2));
    add(grafikGleich("nach Rueckgaengig"));
    add(tu("PT: Pinselfarbe rot", [] { mz().pinsel.farbe = {1.0F, 0.0F, 0.0F, 1.0F}; }));
    // --- Maske ---
    add(werkzeug(Str::PtWzMaske));
    add(mb3::ziehenVonNach("PT Maske: Streifen schuetzen", [] { return bildPunkt(0.3F, 0.3F); }, [] { return bildPunkt(0.7F, 0.3F); }));
    add(pause(0.2));
    add(werkzeug(Str::PtWzPinsel));
    add(mb3::ziehenVonNach("PT Maske: Malen ueber den Streifen", [] { return bildPunkt(0.3F, 0.3F); }, [] { return bildPunkt(0.7F, 0.3F); }));
    add(pause(0.3));
    add(pruefe("PT Maske: unter der Maske bleibt das Original (Mitte des Streifens)", [] {
        const mm::Leinwand* l = lw();
        if (l == nullptr) { return false; }
        int geschuetzt = 0;
        int veraendert = 0;
        for (std::size_t i = 0; i < l->maske.size(); ++i) {
            if (l->maske[i] != 255) { continue; }
            ++geschuetzt;
            veraendert += std::memcmp(&l->bild[i * 4U], &l->grund[i * 4U], 3) != 0 ? 1 : 0;
        }
        diag::detail("Knopftest: PT Maske - " + std::to_string(geschuetzt) + " voll geschuetzte Texel, davon " + std::to_string(veraendert) + " bemalt");
        return geschuetzt > 20 && veraendert == 0;
    }));
    add(zurueckAufOriginal());
    // --- Ebenen und Radieren ---
    add(ausloesen(Str::PtEbeneNeu));
    add(werkzeug(Str::PtWzPinsel));
    add(mb3::ziehenVonNach("PT Ebene: Strich auf der neuen Ebene", [] { return bildPunkt(0.3F, 0.6F); }, [] { return bildPunkt(0.7F, 0.6F); }));
    add(pause(0.3));
    add(pruefe("PT Ebene: das Ergebnis ist bemalt, das Original (Ebene 0) unberuehrt", [st] {
        const mm::Leinwand* l = lw();
        st->n = geaendert(l);
        return l != nullptr && l->ebenen.size() == 2U && st->n > 50 && l->ebenen[0].rgba == l->grund;
    }));
    add(werkzeug(Str::PtWzRadierer));
    add(tu("PT Radieren: harte Kante, groesser", [] {
        mz().pinsel.haerte = 1.0F;
        mz().radiusPx = 30.0F;
    }));
    add(mb3::ziehenVonNach("PT Radieren auf der Ebene", [] { return bildPunkt(0.3F, 0.6F); }, [] { return bildPunkt(0.7F, 0.6F); }));
    add(pause(0.3));
    add(pruefe("PT Radieren: die Ebene wird durchsichtig, weniger Texel bemalt", [st] {
        const int n = geaendert(lw());
        diag::detail("Knopftest: PT Radieren - vorher " + std::to_string(st->n) + ", nachher " + std::to_string(n) + " Texel");
        return n < st->n / 2;
    }));
    add(ausloesen(Str::PtEbeneLoeschen));
    add(pause(0.2));
    add(pruefe("PT Ebene loeschen: nur das Original bleibt, Ergebnis = Original", [] { return lw() != nullptr && lw()->ebenen.size() == 1U && lw()->bild == lw()->grund; }));
    add(tu("PT: Pinsel zurueck", [] {
        mz().pinsel.haerte = 0.5F;
        mz().radiusPx = 18.0F;
    }));
    add(zurueckAufOriginal());
    // --- Tasten wie Blender ---
    add(tu("PT Tasten: wie Blender, Fokus", [st] {
        modellBedienungSetzen(1);
        g_app->mtab.fokus = true;
        st->radius = mz().radiusPx;
    }));
    add(klickAn("PT Tasten: Maus in die Ansicht", [] { return torsoMitte(0.0F); }, 2));
    add(taste(ImGuiKey_F, "F"));
    add(pause(0.2));
    add(tu("PT Tasten: Maus 40 nach rechts", [] {
        const ImVec2 m = ImGui::GetIO().MousePos;
        setzeMaus(ImVec2{m.x + 40.0F, m.y});
    }));
    add(pause(0.2));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("PT Tasten Blender F: Radius ziehen, Enter uebernimmt (+40 px)", [st] {
        diag::detail("Knopftest: PT F - Radius " + std::to_string(st->radius) + " -> " + std::to_string(mz().radiusPx));
        return mz().modalArt == 0 && std::fabs(mz().radiusPx - st->radius - 40.0F) < 2.0F;
    }));
    add(tu("PT Tasten: Farben merken", [st] {
        st->bild.assign(4, 0);
        for (int k = 0; k < 4; ++k) { st->bild[static_cast<std::size_t>(k)] = static_cast<std::uint8_t>(mz().farbe2[static_cast<std::size_t>(k)] * 255.0F); }
    }));
    add(taste(ImGuiKey_X, "X"));
    add(pause(0.2));
    add(pruefe("PT Tasten Blender X: Pinselfarbe und zweite Farbe getauscht", [st] {
        return static_cast<std::uint8_t>(mz().pinsel.farbe[0] * 255.0F) == st->bild[0] && mz().farbe2[0] > 0.99F;
    }));
    add(taste(ImGuiKey_X, "X"));
    add(tu("PT Tasten: Maske gezeigt?", [st] { st->n = mz().maskeZeigen ? 1 : 0; }));
    add(taste(ImGuiKey_1, "1"));
    add(pause(0.2));
    add(pruefe("PT Tasten Blender 1: Maske zeigen umgeschaltet", [st] { return (mz().maskeZeigen ? 1 : 0) != st->n; }));
    add(taste(ImGuiKey_1, "1"));
    add(tu("PT Tasten: zurueck wie 3ds Max", [st] {
        modellBedienungSetzen(0);
        mz().radiusPx = st->radius;
    }));
    // --- waehrend die Animation laeuft ---
    // REST: mit dem Pinsel - zuletzt war der Radierer gewaehlt (PT Radieren); auf der untersten Ebene stellt er das Original her.
    // Bis M9 lief der Strich ueber den Ruecken und aenderte dort 885 Texel, seit die Ansicht Vorne die Brust zeigt keine.
    add(werkzeug(Str::PtWzPinsel));
    add(tu("PT Animation: abspielen", [st] {
        st->n1 = mz().tupfer;
        g_app->animPlaying = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(mb3::ziehenVonNach("PT Animation: Strich waehrend die Figur sich bewegt", [] { return torsoMitte(-30.0F); }, [] { return torsoMitte(30.0F); }));
    add(pause(0.3));
    add(pruefe("PT Animation: der Strich malt in der Pose (Texel geaendert)", [st] {
        const int n = geaendert(lw());
        diag::detail("Knopftest: PT Animation - " + std::to_string(n) + " Texel, Bild " + std::to_string(g_app->animFrame) + ", " + std::to_string(mz().tupfer - st->n1) + " Tupfer");
        return n > 50;
    }));
    add(tu("PT Animation: anhalten", [] { g_app->animPlaying = false; }));
    // --- Bauen ---
    add(tu("PT Bauen: Ausgabe im Laufordner", [] {
        std::snprintf(mz().ordner, sizeof mz().ordner, "%s", mb3::lauf("pt_mod").c_str());
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(mb3::lauf("pt_mod")), ec);
    }));
    add(ausloesen(Str::PtBauen));
    add(pause(0.3));
    add(pruefe("PT Bauen: ein Bild unter seinem Spielpfad, Groesse und Format des Originals, gelesen gleich dem Ergebnis", [] {
        const mm::Leinwand* l = lw();
        if (l == nullptr || mz().gebaut != 1) {
            meldeFehler("PT Bauen: " + std::to_string(mz().gebaut) + " Bilder | " + mz().meldung);
            return false;
        }
        const std::string pfad = (std::filesystem::path(mb3::lauf("pt_mod")) / l->datei).string();
        const std::string bytes = mb3::datei(pfad);
        const auto* d = reinterpret_cast<const unsigned char*>(bytes.data());
        const image::Image im = image::decode(d, bytes.size());
        const bool format = static_cast<int>(image::sniff(d, bytes.size())) == l->format;
        bool tab = true;
        double abw = 0.0;
        if (static_cast<image::Format>(l->format) == image::Format::Jpeg) {
            tab = mm::jpegQuant(bytes) == mm::jpegQuant(l->original);
            for (std::size_t i = 0; im.ok && i < im.rgba.size() && i < l->bild.size(); ++i) {
                if (i % 4U != 3U) { abw += std::fabs(static_cast<double>(im.rgba[i]) - static_cast<double>(l->bild[i])); }
            }
            abw /= static_cast<double>(std::max<std::size_t>(1U, l->bild.size() / 4U * 3U));
        } else if (im.ok) {
            tab = im.rgba == l->bild || !l->alpha;
        }
        int dateien = 0;
        std::error_code ec;
        for (const auto& e : std::filesystem::recursive_directory_iterator(std::filesystem::path(mb3::lauf("pt_mod")), ec)) { dateien += e.is_regular_file() ? 1 : 0; }
        diag::detail("Knopftest: PT Bauen - " + pfad + " " + std::to_string(bytes.size()) + " Bytes, " + std::to_string(im.width) + "x" + std::to_string(im.height) +
                     ", Format " + std::to_string(format ? 1 : 0) + ", Tabellen/Bildpunkte " + std::to_string(tab ? 1 : 0) + ", Abweichung " + std::to_string(abw) + ", " +
                     std::to_string(dateien) + " Datei(en)");
        return im.ok && im.width == l->b && im.height == l->h && format && tab && abw < 4.0 && dateien == 1;
    }));
    add(zurueckAufOriginal());
    add(tu("PT Bauen: Ausgabe leeren", [] {
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(mb3::lauf("pt_mod")), ec);
    }));
    add(ausloesen(Str::PtBauen));
    add(pause(0.3));
    add(pruefe("PT Bauen unbemalt: nichts geschrieben", [] {
        std::error_code ec;
        return mz().gebaut == 0 && !std::filesystem::exists(std::filesystem::path(mb3::lauf("pt_mod")), ec);
    }));
    add(tu("PT: Ende", [] {
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = true;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.3));
    return s;
}

// --- Im Spiel (BHED_MODELL_NUR=ptspiel) ------------------------------------------------------------------------------
// Ein roter Streifen quer ueber Brust und Ruecken (3D, mit Spiegeln aus), als Mod nach BHED_PAINT_MOD; Fotos der Vorschau
// vorher und nachher von vorn.
std::vector<Schritt> spielSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    figurLaden(s, "max");
    add(tu("PT Spiel: Gruppe torso, harter roter Pinsel", [] {
        g_app->mtab.auswahl = {teilVon("torso")};
        mz().gruppe = -1;
        mz().pinsel.haerte = 0.9F;
        mz().radiusPx = 14.0F;
        mz().pinsel.rueckseiten = true;
        g_app->mtab.bereich = ModellBereich::Ansehen;
    }));
    add(pause(0.8));
    add(mission1::probeFoto("pt_vorschau_vorher"));
    add(tu("PT Spiel: Bereich Painting", [] { g_app->mtab.bereich = ModellBereich::Painting; }));
    add(pause(0.5));
    add(mb3::ziehenVonNach("PT Spiel: Streifen ueber die Brust", [] { return torsoMitte(-60.0F); }, [] { return torsoMitte(60.0F); }));
    add(pause(0.3));
    add(tu("PT Spiel: Fuer das Spiel bauen", [] {
        const char* d = std::getenv("BHED_PAINT_MOD");
        std::snprintf(mz().ordner, sizeof mz().ordner, "%s", d != nullptr ? d : mb3::lauf("pt_mod").c_str());
        const int n = modellMalenBauen();
        diag::detail("Knopftest: PT Spiel - " + std::to_string(n) + " Bild(er) nach " + mz().gebautOrdner + " | " + mz().meldung);
        if (n != 1) { meldeFehler("PT Spiel: " + std::to_string(n) + " Bilder gebaut"); }
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.auswahl.clear();   // REST Punkt 3: das Foto ohne den Auswahlrahmen ueber dem Rumpf (Paarbild mit dem Spiel)
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(mission1::probeFoto("pt_vorschau_nachher"));
    add(zurueckAufOriginal());
    return s;
}

// --- Bildzeit (BHED_MODELL_NUR=ptzeit, allein und mit BHED_D3D_DEBUG=0): Ansehen gegen Painting mit offenem 2D-Bild,
// die Figur in Bewegung; dazu je Bild ein Tupfer auf die Brust (Hochladen der Vorschau-Textur jedes Bild)
std::vector<Schritt> zeitSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    figurLaden(s, "max");
    add(tu("PT Zeit: Gruppe torso", [] {
        g_app->mtab.auswahl = {teilVon("torso")};
        mz().gruppe = -1;
        (void)lw();
    }));
    const auto messung = [&](const std::string& was, ModellBereich b, bool tupfen, mm::Werkzeug w = mm::Werkzeug::Pinsel, bool maske = false) {
        add(m1Messblock("Sturmtruppler", "PT Zeit: " + was, [b, tupfen, w, maske] {
            g_app->mtab.bereich = b;
            mz().testTupfen = tupfen;
            mz().werkzeug = static_cast<int>(w);
            mz().maskeZeigen = true;
            if (const mm::Leinwand* l = lw(); l != nullptr && (l->maske.empty() || (l->maske[0] >= 128) != maske)) {
                modellMalenAktionAusloesen(aktion(Str::PtMaskeUmkehren));   // P8: volle bzw. wieder leere Maske
            }
            g_app->animPlaying = true;   // die Figur bewegt sich: jedes Bild neu
        }));
        add(tu("PT Zeit: Ende " + was, [] {
            diag::detail("Knopftest: PT Zeit - " + std::to_string(mz().hochgeladen) + " mal hochgeladen, zuletzt " + std::to_string(mz().hochladenMs) + " ms");
            mz().testTupfen = false;
        }));
    };
    messung("Bereich Ansehen", ModellBereich::Ansehen, false);
    messung("Bereich Painting (2D-Bild offen)", ModellBereich::Painting, false);
    messung("Bereich Painting, je Bild ein Tupfer", ModellBereich::Painting, true);
    messung("Bereich Painting, je Bild ein Tupfer Weichzeichnen", ModellBereich::Painting, true, mm::Werkzeug::Weichzeichnen);   // P7
    messung("Bereich Painting, volle Maske gezeigt", ModellBereich::Painting, false, mm::Werkzeug::Pinsel, true);               // P8
    add(zurueckAufOriginal());
    return s;
}

std::vector<Schritt> handbuchSchritte();   // unten

// --- Pruefung seit rc610 (BHED_MODELL_NUR=ptfix): P1 Leinwaende bei LOD-Wechsel und Frage, P4 Rueckgaengig ueber alle Bilder,
// P3 Deckkraft-Regler ------------------------------------------------------------------------------------------------------------
ImVec2 teilMitte(const char* teil, float dx) {
    ImVec2 m;
    if (!modellFlaecheAufSchirm(teilVon(teil), m)) {
        meldeFehler(std::string("PT: ") + teil + " nicht auf dem Schirm");
        return ImVec2{};
    }
    return ImVec2{m.x + dx, m.y};
}

Schritt gruppeVon(const char* teil) {
    return tu(std::string("PT fix: Gruppe der Flaeche ") + teil, [teil] {
        g_app->mtab.auswahl = {teilVon(teil)};
        mz().gruppe = -1;
    });
}

std::vector<Schritt> fixSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto dokStand = std::make_shared<long long>(0);
    figurLaden(s, "max");
    add(gruppeVon("torso"));
    add(pause(0.3));
    add(werkzeug(Str::PtWzPinsel));
    add(mb3::ziehenVonNach("PT fix: Strich ueber die Brust", [] { return torsoMitte(-30.0F); }, [] { return torsoMitte(30.0F); }));
    add(pause(0.3));
    add(pruefe("PT fix P1: nach einem Strich gilt das Modell als ungespeichert (Beenden und anderes Modell fragen)", [] {
        diag::detail("Knopftest: PT fix P1 - " + std::to_string(geaendert(lw())) + " Texel, ungespeichert " + (modellUngespeichert() ? "ja" : "nein"));
        return geaendert(lw()) > 50 && modellUngespeichert();
    }));
    add(gruppeVon("head"));
    add(pause(0.3));
    add(mb3::ziehenVonNach("PT fix: Strich ueber den Helm", [] { return teilMitte("head", -6.0F); }, [] { return teilMitte("head", 6.0F); }));
    add(pause(0.3));
    add(tu("PT fix P4: Stand des Modelldokuments", [dokStand] { *dokStand = static_cast<long long>(g_app->mtab.dok.stand()); }));
    for (int i = 0; i < 6; ++i) {
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
        add(pause(0.15));
    }
    add(pruefe("PT fix P4: Strg+Z nimmt die Striche beider Bilder zurueck (Helm, dann Rumpf), das Modelldokument bleibt", [dokStand] {
        const int helm = geaendert(lw());
        g_app->mtab.auswahl = {teilVon("torso")};
        mz().gruppe = -1;
        const int rumpf = geaendert(lw());
        const bool dok = static_cast<long long>(g_app->mtab.dok.stand()) == *dokStand;
        diag::detail("Knopftest: PT fix P4 - Helm " + std::to_string(helm) + ", Rumpf " + std::to_string(rumpf) + " Texel anders, Dokument " + (dok ? "gleich" : "geaendert"));
        return helm == 0 && rumpf == 0 && dok;
    }));
    add(gruppeVon("torso"));
    add(pause(0.3));
    add(mb3::ziehenVonNach("PT fix: noch ein Strich ueber die Brust", [] { return torsoMitte(-30.0F); }, [] { return torsoMitte(30.0F); }));
    add(pause(0.3));
    add(tu("PT fix P1: andere Detailstufe (wie ein Sprung der automatischen Stufe beim Zoomen)", [] {
        g_app->mtab.lodWahl = 2;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(pruefe("PT fix P1: die Leinwand torso behaelt den Strich ueber den Wechsel der Detailstufe", [] {
        const int n = geaendert(lw());
        diag::detail("Knopftest: PT fix P1 - nach LOD 2: " + std::to_string(n) + " Texel bemalt, LOD jetzt " + std::to_string(g_app->mtab.lodJetzt));
        return n > 50;
    }));
    add(tu("PT fix: zurueck auf LOD 0", [] {
        g_app->mtab.lodWahl = 1;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    // P3: auf einer neuen Ebene die Mitte des Deckkraft-Reglers anklicken, Strg+Z stellt 100 % her
    add(ausloesen(Str::PtEbeneNeu));
    add(pause(0.3));
    // AP9: der Regler steht unter der Ebenenliste (Deckkraft der gewaehlten Ebene) - erst ins Bild rollen
    const std::string deckRegler = std::string(tr(Str::PtDeckkraft)) + "###ptdeck";
    add(tu("PT fix P3: den Regler Deckkraft ins Bild rollen", [deckRegler] {
        const Element* e = finde(deckRegler, "mtrollouts", 0, false);
        for (ImGuiWindow* w : GImGui->Windows) {
            if (e != nullptr && std::strstr(w->Name, "mtrollouts") != nullptr) {
                ImGui::SetScrollY(w, std::clamp(w->Scroll.y + e->rect.GetCenter().y - w->InnerRect.GetCenter().y, 0.0F, w->ScrollMax.y));
            }
        }
    }));
    add(pause(0.3));
    add(klickAn("PT fix P3: Klick auf die Mitte des Deckkraft-Reglers", [deckRegler] {
        const Element* e = finde(deckRegler, "mtrollouts", 0, false);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{0.0F, 0.0F};
    }));
    add(pause(0.3));
    add(pruefe("PT fix P3: der Klick setzt die Deckkraft (etwa 50 %)", [] {
        const float d = lw() != nullptr && lw()->aktiv >= 0 ? lw()->ebenen[static_cast<std::size_t>(lw()->aktiv)].deckkraft : -1.0F;
        diag::detail("Knopftest: PT fix P3 - Deckkraft nach dem Klick " + std::to_string(d));
        return d > 0.2F && d < 0.8F;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("PT fix P3: Strg+Z stellt die Deckkraft vor dem Klick her (100 %)", [] {
        const float d = lw() != nullptr && lw()->aktiv >= 0 ? lw()->ebenen[static_cast<std::size_t>(lw()->aktiv)].deckkraft : -1.0F;
        diag::detail("Knopftest: PT fix P3 - Deckkraft nach Strg+Z " + std::to_string(d));
        return std::fabs(d - 1.0F) < 1e-4F;
    }));
    add(zurueckAufOriginal());
    return s;
}

// --- Pruefung seit rc610, Rest (BHED_MODELL_NUR=ptfix2, mit SPRACHE=ja): P13 Name der Grundebene in der Sprache der
// Oberflaeche, P8 "Maske zeigen" liest nicht jedes Bild alle Texel, P19 neu geladene Texturen in einem anderen Bereich ------
std::vector<Schritt> fix2Schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    figurLaden(s, "max");
    add(gruppeVon("torso"));
    add(pause(0.3));
    add(pruefe("PT fix2 P13: die Grundebene heisst in der Sprache der Oberflaeche, nicht englisch \"Original\"", [] {
        const bool liste = finde(tr(Str::PtGrEbenen), "", 0, false) != nullptr;
        const bool englisch = finde("Original", "", 0, false) != nullptr;
        diag::detail(std::string("Knopftest: PT fix2 P13 - Ebenenliste ") + (liste ? "da" : "fehlt") + ", \"Original\" " + (englisch ? "angezeigt" : "nicht angezeigt"));
        return liste && !englisch;
    }));
    // P8: volle Maske (Umkehren der leeren), dann wieder leer, dann Strg+Z (wieder voll, ueber den Verlauf)
    const auto maskeStand = [](const std::string& was, int rechteckeHoechstens) {
        return pruefe("PT fix2 P8: " + was + " - im ruhigen Bild keine Texel gelesen, hoechstens " + std::to_string(rechteckeHoechstens) + " Rechtecke", [was, rechteckeHoechstens] {
            const mm::Leinwand* l = lw();
            int maskiert = 0;
            if (l != nullptr) {
                for (const std::uint8_t x : l->maske) { maskiert += x >= 128 ? 1 : 0; }
            }
            diag::detail("Knopftest: PT fix2 P8 " + was + " - " + std::to_string(maskiert) + " Texel maskiert, im letzten Bild " + std::to_string(mz().maskeTexel) +
                         " Texel gelesen, " + std::to_string(mz().maskeRechtecke) + " Rechtecke");
            return l != nullptr && mz().maskeZeigen && mz().maskeTexel == 0 && mz().maskeRechtecke <= rechteckeHoechstens &&
                   (rechteckeHoechstens == 0 ? maskiert == 0 : maskiert == l->b * l->h);
        });
    };
    add(ausloesen(Str::PtMaskeUmkehren));
    add(pause(0.3));
    add(maskeStand("volle Maske", 1));
    add(ausloesen(Str::PtMaskeLeeren));
    add(pause(0.3));
    add(maskeStand("leere Maske", 0));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(maskeStand("Strg+Z: wieder volle Maske", 1));
    add(ausloesen(Str::PtMaskeLeeren));
    add(pause(0.3));
    // P19: bemalen, im Bereich Modellieren die Texturen neu laden (wie Detach/Grundkoerper in einen neuen Teil,
    // modellFlaechenAbgleichen) - die Vorschau zeigt weiter das bemalte Bild, ohne zurueck nach Painting
    add(werkzeug(Str::PtWzPinsel));
    add(mb3::ziehenVonNach("PT fix2: Strich ueber die Brust", [] { return torsoMitte(-30.0F); }, [] { return torsoMitte(30.0F); }));
    add(pause(0.3));
    add(tu("PT fix2 P19: Bereich Modellieren, Texturen der Vorschau neu geladen", [] {
        g_app->mtab.bereich = ModellBereich::Modellieren;
        loadModelTextures();
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(pruefe("PT fix2 P19: nach neu geladenen Texturen im Bereich Modellieren zeigen Flaeche und Shader-Stufen torso das bemalte Bild", [] {
        const int t = teilVon("torso");
        const auto& by = g_app->modelTextures.bySurface;
        const mm::Leinwand* l = lw();
        if (t < 0 || static_cast<std::size_t>(t) >= by.size() || l == nullptr) {
            meldeFehler("PT fix2 P19: kein Bild");
            return false;
        }
        const TextureSet::Tex& x = by[static_cast<std::size_t>(t)];
        const bool flaeche = x.width != l->b || x.rgba == l->bild;
        int stufen = 0;
        int mitBild = 0;
        const auto& sf = g_app->modelTextures.stufen;
        if (static_cast<std::size_t>(t) < sf.size()) {
            for (const auto& fs : sf[static_cast<std::size_t>(t)]) {
                if (fs.pixel.size() != 1U || fs.pixel[0] == nullptr || fs.pixel[0]->width != l->b) { continue; }
                ++stufen;
                mitBild += fs.pixel[0]->rgba == l->bild ? 1 : 0;
            }
        }
        diag::detail("Knopftest: PT fix2 P19 - " + std::to_string(geaendert(l)) + " Texel bemalt, Flaeche " + (flaeche ? "bemalt" : "Original") + ", Stufen mit dem Bild " +
                     std::to_string(mitBild) + " von " + std::to_string(stufen) + " in Bildgroesse");
        return geaendert(l) > 50 && flaeche && (stufen == 0 || mitBild > 0);
    }));
    add(tu("PT fix2: zurueck nach Painting", [] { g_app->mtab.bereich = ModellBereich::Painting; }));
    add(pause(0.3));
    add(zurueckAufOriginal());
    return s;
}

// --- Punkt 2 (BHED_MODELL_NUR=ptstufen): alle Shader-Stufen in 3D wie im Spiel. Das Spiel laedt jedes Bild einmal
// (R_FindImageFile nach dem Namen, tr_image.cpp), jede Stufe jedes Shaders, die es nennt, zeigt die bemalte Datei. Aayla
// (Movie Duels, model_default.skin): torso.tga zeichnen die Shader torso (Rumpf, Arme, Beine) und torso_cull (hips_cloth),
// beide mit torso_specular darueber.
// Gezaehlt ueber alle Flaechen: Stufen (und Flaechenbilder) in der Groesse des Bilds, die noch das Original zeigen
// (Bildpunkte gleich dem Grund der Leinwand), muessen 0 sein; die mit dem bemalten Bild mindestens so viele wie Flaechen.
Schritt alleStufenBemalt(const std::string& wann) {
    return pruefe("PT Stufen " + wann + ": jede Stufe jeder Flaeche mit dem bemalten Bild zeigt es (keine zeigt das Original)", [wann] {
        const mm::Leinwand* l = lw();
        if (l == nullptr || geaendert(l) <= 50) {
            meldeFehler("PT Stufen " + wann + ": nichts bemalt");
            return false;
        }
        int original = 0;
        int bemalt = 0;
        int flaechen = 0;
        std::string welche;
        const auto& sf = g_app->modelTextures.stufen;
        const auto& by = g_app->modelTextures.bySurface;
        const auto zaehle = [&](const TextureSet::Tex& x, std::size_t t) {
            if (x.width != l->b || x.height != l->h) { return; }
            if (x.rgba == l->grund) {
                ++original;
                if (t < g_app->model.surfaces.size() && welche.find(g_app->model.surfaces[t].name) == std::string::npos) { welche += " " + g_app->model.surfaces[t].name; }
            }
            bemalt += x.rgba == l->bild ? 1 : 0;
        };
        for (std::size_t t = 0; t < sf.size(); ++t) {
            bool da = false;
            for (const auto& fs : sf[t]) {
                if (fs.pixel.size() != 1U || fs.pixel[0] == nullptr) { continue; }
                da = da || fs.pixel[0]->rgba == l->bild;
                zaehle(*fs.pixel[0], t);
            }
            flaechen += da ? 1 : 0;
        }
        for (std::size_t t = 0; t < by.size(); ++t) {
            if (t < sf.size() && !sf[t].empty()) { continue; }   // Flaechen mit Shader-Stufen zeichnen die Stufen
            zaehle(by[t], t);
        }
        diag::detail("Knopftest: PT Stufen " + wann + " - Stufen mit dem bemalten Bild " + std::to_string(bemalt) + " auf " + std::to_string(flaechen) +
                     " Flaechen, mit dem Original " + std::to_string(original) + (welche.empty() ? "" : " (" + welche + " )"));
        return original == 0 && bemalt > 0 && flaechen >= 2;
    });
}

std::vector<Schritt> stufenSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto texel = std::make_shared<int>(0);
    figurLaden(s, "max", "models/players/aayla/model.glm");
    add(gruppeVon("torso"));
    add(pause(0.3));
    add(werkzeug(Str::PtWzPinsel));
    add(mb3::ziehenVonNach("PT Stufen: Strich ueber den Rumpf", [] { return teilMitte("torso", -20.0F); }, [] { return teilMitte("torso", 20.0F); }));
    add(pause(0.3));
    add(alleStufenBemalt("nach dem Strich"));
    add(tu("PT Stufen: bemalte Texel merken", [texel] { *texel = geaendert(lw()); }));
    add(gruppeVon("hips_cloth"));
    add(pause(0.3));
    add(pruefe("PT Stufen: hips_cloth (Shader torso_cull) malt auf dieselbe Leinwand torso.tga - der Strich ist da", [texel] {
        const int n = geaendert(lw());
        diag::detail("Knopftest: PT Stufen - Leinwand der Gruppe hips_cloth: " + std::to_string(n) + " Texel bemalt (Rumpf " + std::to_string(*texel) + ")");
        return n == *texel && n > 50;
    }));
    add(mb3::ziehenVonNach("PT Stufen: Strich ueber hips_cloth", [] { return teilMitte("hips_cloth", -8.0F); }, [] { return teilMitte("hips_cloth", 8.0F); }));
    add(pause(0.3));
    add(alleStufenBemalt("nach dem Strich ueber hips_cloth"));
    add(tu("PT Stufen: Fuer das Spiel bauen", [] {
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(mb3::lauf("pt_stufen_mod")), ec);
        std::snprintf(mz().ordner, sizeof mz().ordner, "%s", mb3::lauf("pt_stufen_mod").c_str());
        (void)modellMalenBauen();
    }));
    add(pause(0.3));
    add(pruefe("PT Stufen: Bauen schreibt torso.tga einmal, mit beiden Strichen", [texel] {
        const int n = geaendert(lw());
        diag::detail("Knopftest: PT Stufen - gebaut " + std::to_string(mz().gebaut) + " Bild(er), Leinwand " + std::to_string(n) + " Texel bemalt (erster Strich " +
                     std::to_string(*texel) + ") | " + mz().meldung);
        return mz().gebaut == 1 && n > *texel;
    }));
    add(tu("PT Stufen: Ausgabe leeren", [] {
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(mb3::lauf("pt_stufen_mod")), ec);
        mz().ordner[0] = '\0';
    }));
    add(zurueckAufOriginal());
    return s;
}

// --- PTFIX3 (BHED_MODELL_NUR=ptfix3): Funde der Pruefung seit rc610 P23, P24, P25, P29, P30, P31 (Movie-Duels-Daten fuer P29/P30b)
//   P25  Strich auf dem Helm, Strg+Z, Strich auf der Brust, Strg+Y: der verworfene Helm-Strich bleibt weg (eine lineare Kette)
//   P31  die Stufen der Vorschau neu geladen, bis ihre Liste wieder an derselben Adresse liegt: sie zeigen das bemalte Bild
//   P23  dasselbe Modell mit "verwerfen" neu laden: kein bemaltes Bild mehr, nichts ungespeichert, keine zweite Frage
//   P24  Detailstufe 2 (Ladestand dieser Stufe), UV des Rumpfs dort verschoben, "Auf neue UV umbacken": gebacken innerhalb der Stufe
//   P30a Detach eines Rumpf-Polygons in einen neuen Teil: die Leinwand behaelt alle Dreiecke (der neue Teil gehoert dazu);
//        Umbacken danach meldet die geaenderten Dreiecke statt "UV nicht geaendert"
//   P29  Yoda (MD): head_hair zeigt "hair.tga", das Skript "hair" laedt hair.png (R_LoadImage: png vor tga) - EIN Bild "hair"
//        (GenerateImageMappingName ohne Endung), die Leinwand traegt hair.png
//   P30b Yoda-Schatten (MD): "smoke" nennen body/head/caps nur in Stufen mit tcMod - der Pinsel malt nur auf den Rauchflaechen
Schritt helmUndRumpf(const std::string& was, std::shared_ptr<int> helm, std::shared_ptr<int> rumpf) {
    return tu("PT fix3: bemalte Texel " + was, [helm, rumpf] {
        g_app->mtab.auswahl = {teilVon("head")};
        mz().gruppe = -1;
        *helm = geaendert(lw());
        g_app->mtab.auswahl = {teilVon("torso")};
        mz().gruppe = -1;
        *rumpf = geaendert(lw());
    });
}

std::string kleinPt(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Dreiecke der Flaechen, deren Shader (Haut vor Shader) auf `endung` endet, in der aktuellen Stufe
int dreieckeMitShader(const std::string& endung) {
    const modell::Inhalt& in = g_app->mtab.dok.inhalt();
    const auto st = static_cast<std::size_t>(g_app->mtab.lodJetzt);
    int n = 0;
    for (std::size_t i = 0; i < in.teile.size() && i < g_app->model.surfaces.size(); ++i) {
        const GlmSurface& f = g_app->model.surfaces[i];
        const std::string sh = kleinPt(!f.texture.empty() ? f.texture : f.shader);
        if (in.teile[i].istTag() || st >= in.teile[i].stufen.size() || sh.size() < endung.size() || sh.compare(sh.size() - endung.size(), endung.size(), endung) != 0) {
            continue;
        }
        n += in.teile[i].stufen[st].dreiecke();
    }
    return n;
}

std::vector<Schritt> fix3Schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto helm = std::make_shared<int>(0);
    auto rumpf = std::make_shared<int>(0);
    auto zahl = std::make_shared<int>(0);
    figurLaden(s, "max");
    // --- P25 ---
    add(gruppeVon("head"));
    add(pause(0.3));
    add(werkzeug(Str::PtWzPinsel));
    add(mb3::ziehenVonNach("PT fix3 P25: Strich ueber den Helm", [] { return teilMitte("head", -6.0F); }, [] { return teilMitte("head", 6.0F); }));
    add(pause(0.3));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(gruppeVon("torso"));
    add(pause(0.3));
    add(mb3::ziehenVonNach("PT fix3 P25: Strich ueber die Brust", [] { return torsoMitte(-30.0F); }, [] { return torsoMitte(30.0F); }));
    add(pause(0.3));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Y, "Strg+Y"));
    add(pause(0.2));
    add(helmUndRumpf("nach Strg+Y", helm, rumpf));
    add(pruefe("PT fix3 P25: Strg+Y nach einem neuen Strich bringt den verworfenen Helm-Strich nicht zurueck", [helm, rumpf] {
        diag::detail("Knopftest: PT fix3 P25 - Helm " + std::to_string(*helm) + ", Rumpf " + std::to_string(*rumpf) + " Texel bemalt");
        return *helm == 0 && *rumpf > 50;
    }));
    // --- P31 ---
    add(gruppeVon("torso"));
    add(pause(0.3));
    add(tu("PT fix3 P31: Stufen der Vorschau neu laden, bis ihre Liste wieder an derselben Adresse liegt", [zahl] {
        const void* vorher = g_app->modelTextures.stufen.data();
        *zahl = -1;
        for (int i = 1; i <= 400; ++i) {
            loadModelTextures();
            if (g_app->modelTextures.stufen.data() == vorher) {
                *zahl = i;
                break;
            }
        }
        diag::detail("Knopftest: PT fix3 P31 - Adresse der Stufenliste " +
                     (*zahl > 0 ? "nach " + std::to_string(*zahl) + " Ladevorgaengen wieder gleich" : std::string("in 400 Ladevorgaengen nie gleich")));
    }));
    add(pause(0.5));
    add(pruefe("PT fix3 P31: nach dem Neuladen zeigen die Stufen torso das bemalte Bild", [zahl] {
        const int t = teilVon("torso");
        const mm::Leinwand* l = lw();
        const auto& sf = g_app->modelTextures.stufen;
        if (t < 0 || static_cast<std::size_t>(t) >= sf.size() || l == nullptr) {
            meldeFehler("PT fix3 P31: kein Bild");
            return false;
        }
        int stufen = 0;
        int mitBild = 0;
        for (const auto& fs : sf[static_cast<std::size_t>(t)]) {
            if (fs.pixel.size() != 1U || fs.pixel[0] == nullptr || fs.pixel[0]->width != l->b) { continue; }
            ++stufen;
            mitBild += fs.pixel[0]->rgba == l->bild ? 1 : 0;
        }
        diag::detail("Knopftest: PT fix3 P31 - " + std::to_string(geaendert(l)) + " Texel bemalt, Stufen mit dem Bild " + std::to_string(mitBild) + " von " +
                     std::to_string(stufen) + " (Adresse gleich nach " + std::to_string(*zahl) + ")");
        return geaendert(l) > 50 && stufen > 0 && mitBild > 0;
    }));
    // --- P23 ---
    add(pruefe("PT fix3 P23: bemalt gilt das Modell als ungespeichert", [] { return modellUngespeichert(); }));
    add(tu("PT fix3 P23: dasselbe Modell oeffnen (fragt nach dem Verwerfen)", [] {
        modellVerwerfenDann([] {
            std::string fehler;
            if (!oeffneModellAusArchiv("models/players/stormtrooper/model.glm", &fehler)) { meldeFehler("PT fix3 P23: " + fehler); }
        });
    }));
    add(pause(0.3));
    add(pruefe("PT fix3 P23: die Frage nach dem Verwerfen ist offen", [] { return g_app->frageOffen; }));
    add(klick(tr(Str::AnswerYes), "###frage"));
    add(pause(0.8));
    add(tu("PT fix3 P23: Bereich Painting, Gruppe torso", [] {
        g_app->mtab.bereich = ModellBereich::Painting;
        g_app->mtab.auswahl = {teilVon("torso")};
        mz().gruppe = -1;
    }));
    add(pause(0.3));
    add(pruefe("PT fix3 P23: nach \"verwerfen\" ist kein Bild bemalt und nichts ungespeichert", [] {
        const int n = geaendert(lw());
        diag::detail("Knopftest: PT fix3 P23 - Rumpf " + std::to_string(n) + " Texel bemalt, ungespeichert " + (modellUngespeichert() ? "ja" : "nein"));
        return n == 0 && !modellUngespeichert();
    }));
    add(tu("PT fix3 P23: noch einmal dasselbe Modell oeffnen", [] {
        modellVerwerfenDann([] {
            std::string fehler;
            (void)oeffneModellAusArchiv("models/players/stormtrooper/model.glm", &fehler);
        });
    }));
    add(pause(0.3));
    add(pruefe("PT fix3 P23: keine zweite Frage (das Verwerfen hat gewirkt)", [] {
        const bool offen = g_app->frageOffen;
        if (offen) {   // aufraeumen wie die Startfrage des Selbsttests
            g_app->frageOffen = false;
            g_app->frageJa = nullptr;
            g_app->frageNein = nullptr;
        }
        diag::detail(std::string("Knopftest: PT fix3 P23 - zweite Frage ") + (offen ? "offen" : "keine"));
        return !offen;
    }));
    add(pause(0.8));
    // --- P24 ---
    add(tu("PT fix3 P24: Bereich Painting, Gruppe torso, Detailstufe 2", [] {
        g_app->mtab.bereich = ModellBereich::Painting;
        g_app->mtab.auswahl = {teilVon("torso")};
        mz().gruppe = -1;
        g_app->mtab.lodWahl = 2;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(pruefe("PT fix3 P24: Detailstufe 2 angezeigt, Leinwand torso da", [] {
        diag::detail("Knopftest: PT fix3 P24 - LOD jetzt " + std::to_string(g_app->mtab.lodJetzt) + ", Leinwand " + (lw() != nullptr ? "da" : "fehlt"));
        return g_app->mtab.lodJetzt == 1 && lw() != nullptr;
    }));
    add(tu("PT fix3 P24: UV des Rumpfs in Stufe 2 um 1/8 nach rechts (wie im UV-Editor)", [] {
        const int t = teilVon("torso");
        (void)g_app->mtab.dok.aendern("PT fix3 UV", [t](modell::Inhalt& in) {
            for (modell::Ecke& e : in.teile[static_cast<std::size_t>(t)].stufen[1].ecken) { e.uv[0] += 0.125F; }
        });
    }));
    add(pause(0.3));
    add(ausloesen(Str::PtUmbacken));
    add(pause(0.3));
    add(pruefe("PT fix3 P24: \"Auf neue UV umbacken\" backt innerhalb der Stufe 2 (das Bild folgt der UV)", [] {
        const int n = geaendert(lw());
        diag::detail("Knopftest: PT fix3 P24 - " + std::to_string(n) + " Texel anders als das Original | " + mz().meldung);
        return n > 1000 && mz().meldung == tr(Str::PtUmbackenFertig);
    }));
    add(zurueckAufOriginal());
    add(tu("PT fix3 P24: UV-Schritt zurueck, Detailstufe 1", [] {
        g_app->mtab.bereich = ModellBereich::Modellieren;
        (void)modellRueckgaengig();
        g_app->mtab.bereich = ModellBereich::Painting;
        g_app->mtab.lodWahl = 1;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    // --- P30a ---
    add(tu("PT fix3 P30: Dreiecke der Leinwand torso merken", [zahl] {
        g_app->mtab.auswahl = {teilVon("torso")};
        mz().gruppe = -1;
        *zahl = lw() != nullptr ? static_cast<int>(lw()->dreiecke.size()) : -1;
    }));
    add(tu("PT fix3 P30: Bereich Modellieren, ein Polygon des Rumpfs, Detach in einen neuen Teil", [] {
        ModellTab& t = g_app->mtab;
        t.bereich = ModellBereich::Modellieren;
        modellMeshNetzAktualisieren();
        const int rumpfTeil = teilVon("torso");
        int f = -1;
        for (std::size_t i = 0; f < 0 && i < t.mesh.netz.flaechen.size(); ++i) {
            const modell::wof::PFlaeche& pf = t.mesh.netz.flaechen[i];
            if (!pf.weg && pf.teil == rumpfTeil) { f = static_cast<int>(i); }
        }
        md1d::waehle(modell::wof::PAuswahl{{}, {}, {f}}, AuswahlArt::Dreieck);
        t.mesh.art = 1;
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Detach);
    }));
    add(pause(0.5));
    add(tu("PT fix3 P30: Uebernehmen, zurueck nach Painting", [] {
        modellMeshUebernehmen(false);
        g_app->mtab.bereich = ModellBereich::Painting;
        g_app->mtab.auswahl = {teilVon("torso")};
        mz().gruppe = -1;
    }));
    add(pause(0.5));
    add(pruefe("PT fix3 P30: nach dem Detach traegt die Leinwand torso alle Dreiecke (auch die des neuen Teils)", [zahl] {
        const int n = lw() != nullptr ? static_cast<int>(lw()->dreiecke.size()) : -1;
        diag::detail("Knopftest: PT fix3 P30 - Dreiecke der Leinwand " + std::to_string(n) + " (vorher " + std::to_string(*zahl) + "), Teile " +
                     std::to_string(g_app->mtab.dok.inhalt().teile.size()));
        return n == *zahl && n > 0;
    }));
    add(ausloesen(Str::PtUmbacken));
    add(pause(0.3));
    add(pruefe("PT fix3 P24: Umbacken nach dem Detach meldet die geaenderten Dreiecke", [] {
        diag::detail("Knopftest: PT fix3 P24 Meldung - " + mz().meldung);
        return mz().meldung == tr(Str::PtUmbackenAnders);
    }));
    // --- P29 ---
    for (Schritt& x : mb1::laden("models/players/yoda/model.glm")) { add(std::move(x)); }
    add(tu("PT fix3 P29: Bereich Painting, Gruppe head_hair", [] {
        g_app->mtab.bereich = ModellBereich::Painting;
        g_app->mtab.auswahl = {teilVon("head_hair")};
        mz().gruppe = -1;
        mz().stufe = 0;
    }));
    add(pause(0.5));
    add(pruefe("PT fix3 P29: hair.tga (Haut) und das Skript hair sind EIN Bild, die Leinwand traegt hair.png wie im Spiel", [] {
        const mm::Leinwand* a = lw();
        mz().stufe = 1;
        const mm::Leinwand* b = lw();
        mz().stufe = 0;
        const std::string da = a != nullptr ? a->datei : std::string("-");
        const std::string db = b != nullptr ? b->datei : std::string("-");
        diag::detail("Knopftest: PT fix3 P29 - Stufe 1 " + da + ", Stufe 2 " + db + (a == b ? " (dieselbe Leinwand)" : " (zwei Leinwaende)"));
        return a != nullptr && a == b && kleinPt(da) == "models/players/yoda/hair.png";
    }));
    // --- P30b ---
    for (Schritt& x : mb1::laden("models/players/yoda_shadow/model.glm")) { add(std::move(x)); }
    add(tu("PT fix3 P30: Bereich Painting, Gruppe torso_smoke", [] {
        g_app->mtab.bereich = ModellBereich::Painting;
        g_app->mtab.auswahl = {teilVon("torso_smoke")};
        mz().gruppe = -1;
        mz().stufe = 0;
    }));
    add(pause(0.5));
    add(pruefe("PT fix3 P30: der Pinsel der Leinwand smoke liegt nur auf den Rauchflaechen (body/head/caps nennen smoke nur mit tcMod)", [] {
        const mm::Leinwand* l = lw();
        const int n = l != nullptr ? static_cast<int>(l->dreiecke.size()) : -1;
        // smoke_top.tga gibt es nicht als Datei: head_smoke zeigt ueber das Skript smoke_top das Bild smoke
        const int rauch = dreieckeMitShader("yoda_shadow/smoke.tga") + dreieckeMitShader("yoda_shadow/smoke_top.tga");
        diag::detail("Knopftest: PT fix3 P30 - Leinwand " + (l != nullptr ? l->datei : std::string("-")) + ", " + std::to_string(n) + " Dreiecke, Rauchflaechen " +
                     std::to_string(rauch));
        return l != nullptr && n == rauch && n > 0;
    }));
    return s;
}

// --- MBMAL (BHED_MODELL_NUR=ptlang, allein, BHED_D3D_DEBUG=0): shank 10.11. "wenn ich lange painte, dann wird es laggy und die
// fps geht runter" - beim Malen auf dem Modell in der 3D-Ansicht. Wie sein Bild: kyle, body (2048 x 2048), Painting, 2D-Bild mit
// "Show UVs", eine 3D-Ansicht, Animation BOTH_A2_TR_BL laeuft, Pinsel Radius 24, Strength 1, Spacing 10 %, Pressure: strength.
// Viele Striche nacheinander ueber die echte Maus; sie bewegt sich nach der Uhr wie eine Hand (ein Strich dauert kMalSek, bei
// jeder Bildrate gleich lang), reihum ueber alle Flaechen mit dem Bild body. Schalter: BHED_PTLANG_STRICHE (Vorgabe 2000),
// BHED_PTLANG_WO=3d|2d, BHED_PTLANG_DRUCK=1 (simulierter Stiftdruck), BHED_PTLANG_ANIM=0 (die Animation steht - Gegenprobe).
// Je 100 Striche eine Zeile und am Ende ptlang_<wo>.csv im Laufordner: Bildzeit beim Malen und in Ruhe, Bildzeit am Anfang und
// am Ende eines Strichs, Bilder je Sekunde, Grafikkarte, Speicher, Verlauf, Tupfer und wohin die Zeit geht (malen_mess.h).
// Rot: die Bildzeit steigt ueber die Dauer eines Strichs oder ueber die Striche, das Malen laeuft unter 60 Bildern je Sekunde,
// oder der Speicher waechst ueber den Verlauf hinaus.
// BHED_PTLANG_FEST=1: die Animation steht, die Maus geht je Bild einen festen Schritt - jeder Lauf setzt dieselben Tupfer. Am
// Ende die Pruefsummen des Bilds und der gebauten Datei (vor und nach einer Aenderung am Malen gleich), die Texturen der
// Grafikkarte gegen das Bild, Rueckgaengig bis zum Original und Wiederholen bis zum selben Bild.
std::uint64_t fnvBytes(const std::uint8_t* p, std::size_t n) {
    std::uint64_t h = 1469598103934665603ULL;
    for (std::size_t i = 0; i < n; ++i) { h = (h ^ p[i]) * 1099511628211ULL; }
    return h;
}

std::string hexVon(std::uint64_t h) {
    char z[24];
    std::snprintf(z, sizeof z, "%016llx", static_cast<unsigned long long>(h));
    return z;
}

std::vector<Schritt> langSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto schalter = [](const char* name, const char* wert) {
        const char* e = std::getenv(name);
        return e != nullptr && std::string(e) == wert;
    };
    const bool fest = schalter("BHED_PTLANG_FEST", "1");
    const char* eStriche = std::getenv("BHED_PTLANG_STRICHE");
    const int striche = std::max(20, eStriche != nullptr ? std::atoi(eStriche) : (fest ? 200 : 2000));
    const bool zweiD = schalter("BHED_PTLANG_WO", "2d");
    const bool druck = schalter("BHED_PTLANG_DRUCK", "1");
    const bool anim = !fest && !schalter("BHED_PTLANG_ANIM", "0");
    const std::string wo = std::string(zweiD ? "2d" : "3d") + (druck ? "_druck" : "") + (fest ? "_fest" : anim ? "" : "_steht");
    auto teile = std::make_shared<std::vector<int>>();   // die Flaechen mit dem Bild body
    figurLaden(s, "max", "models/players/kyle/model.glm");
    add(tu("PT lang: Gruppe body (2048 x 2048), Pinsel wie shanks Bild, Animation BOTH_A2_TR_BL", [teile, anim] {
        const modell::Inhalt& in = g_app->mtab.dok.inhalt();
        std::string namen;
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (in.teile[i].istTag() || !modellTeilGezeichnet(static_cast<int>(i))) { continue; }
            g_app->mtab.auswahl = {static_cast<int>(i)};
            mz().gruppe = -1;
            mz().stufe = 0;
            const mm::Leinwand* l = lw();
            if (l != nullptr && kleinPt(l->datei).find("/body.") != std::string::npos) {
                teile->push_back(static_cast<int>(i));
                namen += (namen.empty() ? "" : ", ") + in.teile[i].name;
            }
        }
        if (!teile->empty()) {
            g_app->mtab.auswahl = {teile->front()};
            mz().gruppe = -1;
        }
        const mm::Leinwand* l = lw();
        // die Ausdehnung der Dreiecke in der Ruhelage (der Tupfer misst seinen Weg dort)
        float lo[3] = {0.0F, 0.0F, 0.0F};
        float hi[3] = {0.0F, 0.0F, 0.0F};
        bool erste = true;
        for (const mm::Dreieck& d : l != nullptr ? l->dreiecke : std::vector<mm::Dreieck>{}) {
            for (const auto& e : d.ort) {
                for (int k = 0; k < 3; ++k) {
                    lo[k] = erste ? e[k] : std::min(lo[k], e[k]);
                    hi[k] = erste ? e[k] : std::max(hi[k], e[k]);
                }
                erste = false;
            }
        }
        char kasten[160];
        std::snprintf(kasten, sizeof kasten, ", Ruhelage %.1f %.1f %.1f bis %.1f %.1f %.1f", static_cast<double>(lo[0]), static_cast<double>(lo[1]),
                      static_cast<double>(lo[2]), static_cast<double>(hi[0]), static_cast<double>(hi[1]), static_cast<double>(hi[2]));
        diag::detail("Knopftest: PT lang - Leinwand " + (l != nullptr ? l->datei + " " + std::to_string(l->b) + " x " + std::to_string(l->h) + ", " +
                                                                               std::to_string(l->dreiecke.size()) + " Dreiecke"
                                                                       : std::string("-")) +
                     kasten + ", Flaechen: " + namen);
        if (teile->empty() || l == nullptr || l->b != 2048 || l->h != 2048) { meldeFehler("PT lang: kyle body 2048 x 2048 nicht gefunden"); }
        g_app->mtab.ansichten[1].art = AnsichtArt::Perspektive;
        mz().radiusPx = 24.0F;
        mz().pinsel.staerke = 1.0F;
        mz().pinsel.abstand = 0.1F;
        mz().druckStaerke = true;
        mz().druckGroesse = false;
        mz().spiegeln = false;
        mz().uvZeigen = true;
        mz().editorOffen = true;
        g_app->loopMode = 1;
        g_app->animSpeed = 1.0F;
        for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
            if (g_app->animList[i].name == "BOTH_A2_TR_BL") { animWaehlen(static_cast<int>(i), false); }
        }
        g_app->animPlaying = anim;
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    // die ganze Figur wie in shanks Bild: einpassen ohne Auswahl (mit der Auswahl passte es nur die gewaehlte Flaeche ein - die
    // Mitten der anderen lagen ausserhalb der Ansicht, ihre Klicks trafen das 2D-Bild oder Knoepfe des Befehlsfelds)
    add(tu("PT lang: einpassen (ganze Figur)", [] {
        g_app->mtab.auswahl.clear();
        modellAnsichtEinpassenAktiv();
    }));
    add(pause(1.0));
    constexpr double kMalSek = 0.6;     // ein Strich: so lange ist die Taste unten
    constexpr double kRuheSek = 0.15;   // zwischen zwei Strichen
    constexpr int kFestSchritte = 14;   // fest: Mausschritte je Strich
    constexpr int kBlock = 100;         // Striche je Zeile
    constexpr int kEimer = 6;           // Bildzeit nach der Zeit seit dem Druecken, je kMalSek / kEimer
    struct Lauf {
        int strich = 0;
        int phase = 0;     // 0 die Maus steht am Anfang des Strichs, 1 Taste unten, 2 Ruhe
        int imPhase = 0;   // Bilder in der Phase
        std::chrono::steady_clock::time_point phaseAb{};
        std::chrono::steady_clock::time_point vorher{};
        std::chrono::steady_clock::time_point laufAb{};
        ImVec2 a{}, b{};
        int neuVorher = 0;
        int tupferVorher = 0;
        int hochVorher = 0;
        int figurHochVorher = 0;
        // je Block
        std::vector<double> mal, ruhe, gpu;
        std::array<double, kEimer> eimer{};
        std::array<int, kEimer> eimerZahl{};
        double malSek = 0.0;
        int leerBlock = 0;
        // ueber den Lauf
        std::array<double, kEimer> alle{};
        std::array<int, kEimer> alleZahl{};
        double ersteMal = -1.0, letzteMal = -1.0, summeMal = 0.0;
        long long bilderMal = 0;
        long long privatMitte = -1, privatEnde = -1, privatAnfang = -1;
        double verlaufMitte = 0.0, verlaufEnde = 0.0;
        int leer = 0;   // Fehlversuche: der Klick traf die Figur nicht (abgebrochen, zaehlt nicht als Strich)
        bool fehlversuch = false;
        double seitVorher = 0.0;   // Zeit seit dem Druecken, als das laufende Bild begann
        // Zaehler ueber den Lauf (der schlechteste Block bzw. die Summe)
        double dreieckeAnteil = 0.0;   // gepruefte Dreiecke je Tupfer, als Anteil der Dreiecke der Leinwand
        double flaecheJeTreffer = 0.0; // bearbeitete Flaeche je getroffenem Texel
        long long ereignisse3d = 0;    // Mausereignisse, die in 3D gemalt haben
        long long maskeGelesen = 0;    // wie oft die 2D-Ansicht die Maske neu gelesen hat
        long long maskeBisher = 0;     // Stand des Zaehlers im Block beim vorigen Bild (Meldung je Lesen)
        int maskeMeldungen = 0;
        std::set<std::string> maskeLeinwaende;   // Leinwaende, deren Maske die 2D-Ansicht schon gelesen hat
        long long maskeWieder = 0;     // Lesungen einer Leinwand, die schon gelesen war
        std::string csv;
    };
    auto st = std::make_shared<Lauf>();
    Schritt x{"PT lang: " + std::to_string(striche) + " Striche (" + wo + ")", [=](int bild) {
                  ImGuiIO& io = ImGui::GetIO();
                  const auto jetzt = std::chrono::steady_clock::now();
                  const auto sek = [](std::chrono::steady_clock::duration d) { return std::chrono::duration<double>(d).count(); };
                  mm::Messung& ms = mm::messung();
                  if (bild == 0) {
                      g_m1Messen = true;
                      st->laufAb = jetzt;
                      st->hochVorher = mz().hochgeladen;
                      int geteilt = 0;
                      gpu::figurBilderZahlen(st->figurHochVorher, geteilt);
                      ms = mm::Messung{};
                      ms.an = true;
                      st->csv = "striche;sek;mal_ms;mal_p99_ms;ruhe_ms;mal_bilder_je_s;mal_anfang_ms;mal_ende_ms;gpu_median_ms;privat_mb;arbeit_mb;grafik_mb;verlauf_mb;"
                                "schritte;kacheln;tupfer_je_strich;tupfer_leer_anteil;dreiecke_je_tupfer;texel_je_tupfer;treffer_je_tupfer;rechteck_je_tupfer;"
                                "kacheln_je_strich;weg_je_strich;sprung_max;ereignisse_je_strich;hochtexel_je_ereignis;ms_treffer;ms_suchen;ms_sichern;ms_werkzeug;"
                                "ms_bluten;ms_zusammen;ms_hochladen;ms_strichende;ms_maske;ms_vergleich;ms_ansicht;ms_editor;ms_bereich;hochgeladen;"
                                "figurbilder_ganz;leer\n";
                  } else {
                      // das Bild, das gerade gelaufen ist: mit der Taste unten (Phase 1) ein Malbild, in der Ruhe ein Ruhebild
                      const double cpu = std::chrono::duration<double, std::milli>(jetzt - g_m1BildAnfang).count();
                      if (st->phase == 1 && st->imPhase > 0) {
                          st->mal.push_back(cpu);
                          const int e = std::clamp(static_cast<int>(st->seitVorher / kMalSek * kEimer), 0, kEimer - 1);
                          st->eimer[static_cast<std::size_t>(e)] += cpu;
                          ++st->eimerZahl[static_cast<std::size_t>(e)];
                          st->malSek += sek(jetzt - st->vorher);
                      } else if (st->phase == 2 && st->imPhase > 2) {
                          st->ruhe.push_back(cpu);
                      }
                      if (g_app->modellNeuGezeichnet != st->neuVorher) {
                          const float g = gpu::modellGpuMs();
                          if (g >= 0.0F) { st->gpu.push_back(static_cast<double>(g)); }
                      }
                  }
                  if (ms.maskeGelesen > st->maskeBisher) {
                      // wann und welche Leinwand: ein Strich mit dem Pinsel aendert die Maske nicht - jede Leinwand liest sie einmal
                      // (ein Strich auf einer Flaeche mit einem anderen Bild zeigt dessen Leinwand), danach nicht wieder
                      const mm::Leinwand* lm = lw();
                      const std::string name = lm != nullptr ? lm->datei : std::string("-");
                      if (!st->maskeLeinwaende.insert(name).second) { ++st->maskeWieder; }
                      if (++st->maskeMeldungen <= 20) {
                          diag::detail("Knopftest: PT lang " + wo + " - Maske gelesen bei Strich " + std::to_string(st->strich) + ", Phase " + std::to_string(st->phase) +
                                       ", Bild " + std::to_string(st->imPhase) + ", Leinwand " + name + (lm != nullptr ? " Stand " + std::to_string(lm->stand) : std::string()) +
                                       ", Gruppe " + std::to_string(mz().gruppe) + ", letzter Befehl " + mz().letzterBefehl);
                      }
                  }
                  st->maskeBisher = ms.maskeGelesen;
                  st->vorher = jetzt;
                  st->neuVorher = g_app->modellNeuGezeichnet;
                  ++st->imPhase;
                  if (st->phase == 0) {
                      if (st->imPhase == 1) {
                          // Farbe je Strich anders (sonst malte ein Strich ueber gleiche Farbe), Lage je Strich versetzt, Richtung wechselnd
                          const int n = st->strich;
                          const float hue = static_cast<float>(n % 12) / 12.0F;
                          mz().pinsel.farbe = {0.5F + 0.5F * std::cos(6.2831853F * hue), 0.5F + 0.5F * std::cos(6.2831853F * (hue - 0.333F)),
                                               0.5F + 0.5F * std::cos(6.2831853F * (hue - 0.667F)), 1.0F};
                          const float versatz = static_cast<float>((n * 37) % 61 - 30);
                          const bool rueck = (n % 2) == 1;
                          if (zweiD) {
                              const float fy = 0.05F + 0.9F * static_cast<float>((n * 37) % 101) / 100.0F;
                              st->a = bildPunkt(rueck ? 0.9F : 0.1F, fy);
                              st->b = bildPunkt(rueck ? 0.1F : 0.9F, fy);
                          } else {
                              // der Strich beginnt auf der Flaeche (ihre Mitte am Schirm) und geht 160 Punkte zur Seite
                              ImVec2 m{};
                              if (teile->empty() || !modellFlaecheAufSchirm((*teile)[static_cast<std::size_t>(n + st->leer) % teile->size()], m)) { m = ImVec2{}; }
                              st->a = ImVec2{m.x, m.y + versatz * 0.2F};
                              st->b = ImVec2{m.x + (rueck ? -160.0F : 160.0F), m.y + versatz};
                              // nur in der Ansicht: der Anfang darin (sonst ein Fehlversuch ohne Klick), das Ende auf ihren Rand begrenzt
                              const ModellAnsicht& av = g_app->mtab.ansichten[g_app->mtab.aktiv];
                              const float x0 = av.lo[0] + 4.0F;
                              const float y0 = av.lo[1] + 4.0F;
                              const float x1 = av.lo[0] + av.groesse[0] - 4.0F;
                              const float y1 = av.lo[1] + av.groesse[1] - 4.0F;
                              st->b = ImVec2{std::clamp(st->b.x, x0, std::max(x0, x1)), std::clamp(st->b.y, y0, std::max(y0, y1))};
                              if (st->a.x < x0 || st->a.x > x1 || st->a.y < y0 || st->a.y > y1) {
                                  st->phase = 2;
                                  st->imPhase = 0;
                                  st->phaseAb = jetzt;
                                  st->fehlversuch = true;
                                  return false;
                              }
                          }
                          st->tupferVorher = mz().tupfer;
                          setzeMaus(st->a);
                      } else if (st->imPhase == 3) {
                          io.AddMouseButtonEvent(0, true);
                          st->phase = 1;
                          st->imPhase = 0;
                          st->phaseAb = jetzt;
                      }
                  } else if (st->phase == 1) {
                      const double t = fest ? static_cast<double>(st->imPhase) / static_cast<double>(kFestSchritte) : sek(jetzt - st->phaseAb) / kMalSek;
                      st->seitVorher = t * kMalSek;
                      if (!zweiD && st->imPhase == 2 && !mz().strichLaeuft) {
                          // der Klick hat die Figur nicht getroffen (die Mitte der Flaeche liegt hinter einer anderen): abbrechen
                          io.AddMouseButtonEvent(0, false);
                          st->phase = 2;
                          st->imPhase = 0;
                          st->phaseAb = jetzt;
                          st->fehlversuch = true;
                      } else if (t <= 1.0) {
                          const auto tf = static_cast<float>(t);
                          if (druck) { modellStiftDruck(0.35F + 0.65F * std::fabs(std::sin(3.14159265F * tf + static_cast<float>(st->strich)))); }
                          setzeMaus(ImVec2{st->a.x + (st->b.x - st->a.x) * tf, st->a.y + (st->b.y - st->a.y) * tf});
                      } else {
                          io.AddMouseButtonEvent(0, false);
                          st->phase = 2;
                          st->imPhase = 0;
                          st->phaseAb = jetzt;
                      }
                  } else if (st->fehlversuch) {
                      if (st->imPhase >= 2) {
                          st->fehlversuch = false;
                          ++st->leer;
                          ++st->leerBlock;
                          st->phase = 0;
                          st->imPhase = 0;
                          if (st->leer > striche + 50) {
                              g_m1Messen = false;
                              ms = mm::Messung{};
                              meldeFehler("PT lang " + wo + ": die Striche treffen die Figur nicht (" + std::to_string(st->leer) + " Fehlversuche)");
                              return true;
                          }
                      }
                  } else if (fest ? st->imPhase >= 4 : sek(jetzt - st->phaseAb) >= kRuheSek) {
                      ++st->strich;
                      st->phase = 0;
                      st->imPhase = 0;
                      if (st->strich % kBlock == 0 || st->strich == striche) {
                          const int imBlock = st->strich % kBlock == 0 ? kBlock : st->strich % kBlock;
                          const gpu::SpeicherStand sp = gpu::speicherStand();
                          const MalVerlaufStand vl = modellMalenVerlaufStand();
                          int hoch = 0;
                          int geteilt = 0;
                          gpu::figurBilderZahlen(hoch, geteilt);
                          const double malM = m1Mittel(st->mal);
                          const double verlaufMb = static_cast<double>(vl.bytes) / 1048576.0;
                          if (st->ersteMal < 0.0) {
                              st->ersteMal = malM;
                              st->privatAnfang = sp.prozessBytes;
                          }
                          st->letzteMal = malM;
                          for (const double v : st->mal) { st->summeMal += v; }
                          st->bilderMal += static_cast<long long>(st->mal.size());
                          if (st->privatMitte < 0 && st->strich * 2 >= striche) {
                              st->privatMitte = sp.prozessBytes;
                              st->verlaufMitte = verlaufMb;
                          }
                          st->privatEnde = sp.prozessBytes;
                          st->verlaufEnde = verlaufMb;
                          for (int e = 0; e < kEimer; ++e) {
                              st->alle[static_cast<std::size_t>(e)] += st->eimer[static_cast<std::size_t>(e)];
                              st->alleZahl[static_cast<std::size_t>(e)] += st->eimerZahl[static_cast<std::size_t>(e)];
                          }
                          const auto eimerMittel = [&](int e) {
                              return st->eimerZahl[static_cast<std::size_t>(e)] > 0 ? st->eimer[static_cast<std::size_t>(e)] / st->eimerZahl[static_cast<std::size_t>(e)] : 0.0;
                          };
                          const double jeStrich = 1.0 / static_cast<double>(imBlock);
                          const double jeTupfer = ms.tupfer > 0 ? 1.0 / static_cast<double>(ms.tupfer) : 0.0;
                          char z[1400];
                          std::snprintf(z, sizeof z,
                                        "%d;%.0f;%.3f;%.3f;%.3f;%.1f;%.3f;%.3f;%.3f;%lld;%lld;%lld;%.1f;%d;%d;%.1f;%.3f;%.0f;%.0f;%.0f;%.0f;%.1f;%.1f;%.1f;%.1f;%.0f;"
                                        "%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%d;%d;%d\n",
                                        st->strich, sek(jetzt - st->laufAb), malM, m1Quantil(st->mal, 0.99), m1Mittel(st->ruhe),
                                        st->malSek > 0.0 ? static_cast<double>(st->mal.size()) / st->malSek : 0.0, eimerMittel(0), eimerMittel(kEimer - 1),
                                        m1Quantil(st->gpu, 0.5), sp.prozessBytes >> 20, sp.arbeitsBytes >> 20, sp.grafikBytes >> 20, verlaufMb, vl.schritte, vl.kacheln,
                                        static_cast<double>(ms.tupfer) * jeStrich, static_cast<double>(ms.tupferLeer) * jeTupfer,
                                        static_cast<double>(ms.dreieckeGeprueft) * jeTupfer, static_cast<double>(ms.texelGeprueft) * jeTupfer,
                                        static_cast<double>(ms.treffer) * jeTupfer, static_cast<double>(ms.rechteckTexel) * jeTupfer,
                                        static_cast<double>(ms.kachelnGesichert) * jeStrich, ms.wegRuhe * jeStrich, ms.wegRuheMax,
                                        static_cast<double>(ms.ereignisse) * jeStrich,
                                        ms.ereignisse > 0 ? static_cast<double>(ms.hochTexel) / static_cast<double>(ms.ereignisse) : 0.0, ms.trefferMs * jeStrich,
                                        ms.suchenMs * jeStrich, ms.sichernMs * jeStrich, ms.werkzeugMs * jeStrich, ms.blutenMs * jeStrich, ms.zusammenMs * jeStrich,
                                        ms.hochladenMs * jeStrich, ms.strichEndeMs * jeStrich, ms.maskeMs * jeStrich, ms.vergleichMs * jeStrich, ms.ansichtMs * jeStrich,
                                        ms.editorMs * jeStrich, ms.bereichMs * jeStrich, mz().hochgeladen - st->hochVorher, hoch - st->figurHochVorher, st->leerBlock);
                          st->csv += z;
                          diag::detail(std::string("Knopftest: PT lang ") + wo + " Block " + z);
                          const modell::Inhalt& in = g_app->mtab.dok.inhalt();
                          const auto nameVon = [&in](int t) { return t >= 0 && static_cast<std::size_t>(t) < in.teile.size() ? in.teile[static_cast<std::size_t>(t)].name : std::string("-"); };
                          std::snprintf(z, sizeof z,
                                        "Knopftest: PT lang %s Spruenge bis Strich %d - %lld von %lld Ereignissen ueber zehn Radien; der groesste %.1f: von %s (%.1f %.1f %.1f) "
                                        "nach %s (%.1f %.1f %.1f)",
                                        wo.c_str(), st->strich, ms.ereignisseWeit, ms.ereignisse, ms.wegRuheMax, nameVon(ms.sprungFlaechen[0]).c_str(), ms.sprungVon[0],
                                        ms.sprungVon[1], ms.sprungVon[2], nameVon(ms.sprungFlaechen[1]).c_str(), ms.sprungNach[0], ms.sprungNach[1], ms.sprungNach[2]);
                          diag::detail(z);
                          st->mal.clear();
                          st->ruhe.clear();
                          st->gpu.clear();
                          st->eimer = {};
                          st->eimerZahl = {};
                          st->malSek = 0.0;
                          st->leerBlock = 0;
                          // gegen die Leinwand jedes Tupfers (die Striche treffen verschiedene Bilder; die am Ende gewaehlte Leinwand ist
                          // nicht die, auf der der Block gemalt hat)
                          if (ms.dreieckeLeinwand > 0) {
                              st->dreieckeAnteil = std::max(st->dreieckeAnteil, static_cast<double>(ms.dreieckeGeprueft) / static_cast<double>(ms.dreieckeLeinwand));
                          }
                          if (ms.treffer > 0) { st->flaecheJeTreffer = std::max(st->flaecheJeTreffer, static_cast<double>(ms.rechteckTexel) / static_cast<double>(ms.treffer)); }
                          st->ereignisse3d += ms.ereignisse;
                          st->maskeGelesen += ms.maskeGelesen;
                          ms = mm::Messung{};
                          ms.an = true;
                          st->maskeBisher = 0;
                      }
                      if (st->strich >= striche) {
                          g_m1Messen = false;
                          ms = mm::Messung{};
                          const auto alleMittel = [&](int e) {
                              return st->alleZahl[static_cast<std::size_t>(e)] > 0 ? st->alle[static_cast<std::size_t>(e)] / st->alleZahl[static_cast<std::size_t>(e)] : 0.0;
                          };
                          const double malMittel = st->bilderMal > 0 ? st->summeMal / static_cast<double>(st->bilderMal) : 0.0;
                          std::string verlauf;
                          for (int e = 0; e < kEimer; ++e) {
                              char w[40];
                              std::snprintf(w, sizeof w, "%s%.2f", e > 0 ? " / " : "", alleMittel(e));
                              verlauf += w;
                          }
                          const double fremd = static_cast<double>((st->privatEnde - st->privatMitte) >> 20) - (st->verlaufEnde - st->verlaufMitte);
                          char z[900];
                          std::snprintf(z, sizeof z,
                                        "Knopftest: PT lang %s - Bildzeit beim Malen %.2f ms (erste %d Striche %.2f ms, letzte %.2f ms); ueber die Dauer eines Strichs "
                                        "(je %.0f ms): %s ms; privat Anfang %lld MB, Mitte %lld MB, Ende %lld MB, Verlauf Mitte %.0f MB, Ende %.0f MB; %d Fehlversuche",
                                        wo.c_str(), malMittel, kBlock, st->ersteMal, st->letzteMal, kMalSek / kEimer * 1000.0, verlauf.c_str(), st->privatAnfang >> 20,
                                        st->privatMitte >> 20, st->privatEnde >> 20, st->verlaufMitte, st->verlaufEnde, st->leer);
                          diag::detail(z);
                          std::ofstream(mb3::lauf("ptlang_" + wo + ".csv"), std::ios::binary) << st->csv;
                          if (!fest) {
                              // Rot, wenn die Bildzeit ueber die Striche steigt (letzte 100 gegen die ersten 100), das Malen nicht fluessig ist
                              // (60 Bilder je Sekunde: 16,7 ms je Bild) oder der Speicher in der zweiten Haelfte ueber den Verlauf hinaus waechst
                              // (mehr als 64 MB: vier Bilder 2048 x 2048)
                              pruefeWahr(st->letzteMal <= st->ersteMal * 1.25 + 0.5, "PT lang " + wo + ": Bildzeit nach " + std::to_string(striche) + " Strichen wie am Anfang");
                              pruefeWahr(malMittel <= 1000.0 / 60.0, "PT lang " + wo + ": Malen mit mindestens 60 Bildern je Sekunde");
                              pruefeWahr(fremd <= 64.0, "PT lang " + wo + ": Speicher begrenzt (zweite Haelfte hoechstens 64 MB ueber dem Verlauf)");
                              // die Ursachen als Zaehler (gelten auch unter Last): ein Tupfer prueft nur die Dreiecke in seiner Naehe (hoechstens ein
                              // Zehntel der Leinwand), bearbeitet nur die Flaeche um seine Treffer (das Rechteck um einen Kreis samt Ausbluten ist
                              // das 1,3- bis 2-fache seiner Flaeche; bis zum Vierfachen fuer Tupfer ueber zwei Inseln), der Pinsel liest die Maske
                              // nicht neu, und ein Strich im 2D-Bild malt nicht nebenbei in 3D
                              std::snprintf(z, sizeof z,
                                            "Knopftest: PT lang %s Zaehler - gepruefte Dreiecke je Tupfer %.1f %% der Leinwand, bearbeitete Flaeche je Treffer %.2f, "
                                            "Maske %lld mal gelesen (%zu Leinwaende, %lld mal wieder), %lld Mausereignisse in 3D",
                                            wo.c_str(), st->dreieckeAnteil * 100.0, st->flaecheJeTreffer, st->maskeGelesen, st->maskeLeinwaende.size(), st->maskeWieder,
                                            st->ereignisse3d);
                              diag::detail(z);
                              if (!zweiD) {
                                  pruefeWahr(st->dreieckeAnteil <= 0.1, "PT lang " + wo + ": ein Tupfer prueft nur die Dreiecke in seiner Naehe");
                                  pruefeWahr(st->flaecheJeTreffer <= 4.0, "PT lang " + wo + ": ein Tupfer bearbeitet nur die Flaeche um seine Treffer");
                              } else {
                                  pruefeWahr(st->ereignisse3d == 0, "PT lang " + wo + ": ein Strich im 2D-Bild malt nicht in der 3D-Ansicht");
                              }
                              pruefeWahr(st->maskeWieder == 0, "PT lang " + wo + ": der Pinsel liest die Maske nicht neu");
                          }
                          pruefeWahr(st->leer <= striche, "PT lang " + wo + ": die Striche treffen (" + std::to_string(st->leer) + " Fehlversuche)");
                          return true;
                      }
                  }
                  return false;
              }};
    x.eigeneZeitgrenze = true;
    add(std::move(x));
    add(tu("PT lang: Animation aus", [] { g_app->animPlaying = false; }));
    if (fest) {
        // dieselben Tupfer in jedem Lauf: Bild, gebaute Datei, Grafikkarte, Rueckgaengig und Wiederholen
        auto bildSumme = std::make_shared<std::uint64_t>(0);
        add(pause(0.3));
        add(tu("PT lang fest: Pruefsummen des Bilds und der gebauten Datei", [bildSumme, teile] {
            if (!teile->empty()) {
                g_app->mtab.auswahl = {teile->front()};
                mz().gruppe = -1;
            }
            const mm::Leinwand* l = lw();
            if (l == nullptr) {
                meldeFehler("PT lang fest: keine Leinwand");
                return;
            }
            *bildSumme = fnvBytes(l->bild.data(), l->bild.size());
            std::string ebenen;
            for (const mm::Ebene& e : l->ebenen) { ebenen += (ebenen.empty() ? "" : ",") + hexVon(fnvBytes(e.rgba.data(), e.rgba.size())); }
            std::snprintf(mz().ordner, sizeof mz().ordner, "%s", mb3::lauf("ptlang_mod").c_str());
            const int n = modellMalenBauen();
            std::string datei;
            std::string bytes;
            const std::string pfad = mb3::lauf("ptlang_mod") + "/" + l->datei;
            std::ifstream in(pfad, std::ios::binary);
            if (in) { bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()); }
            const MalVerlaufStand vl = modellMalenVerlaufStand();
            diag::detail("Knopftest: PT lang fest - Bild " + hexVon(*bildSumme) + ", Ebenen " + ebenen + ", Maske " + hexVon(fnvBytes(l->maske.data(), l->maske.size())) +
                         ", gebaut " + std::to_string(n) + ", Datei " + hexVon(fnvBytes(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size())) + " (" +
                         std::to_string(bytes.size()) + " Bytes), " + std::to_string(geaendert(l)) + " Texel anders, " + std::to_string(mz().tupfer) + " Tupfer, Verlauf " +
                         std::to_string(vl.schritte) + " Schritte, " + std::to_string(vl.kacheln) + " Kacheln, " + std::to_string(vl.bytes) + " Bytes");
            // AP9: n >= 1 - je nach Groesse der Ansicht treffen die festen Striche auch den Kopf (head.jpg), dann sind es zwei
            // gebaute Bilder (auf dem Stand mit dem Rahmen des rechten Felds von AP3 schon vor AP9: "gebaut 2")
            pruefeWahr(n >= 1 && !bytes.empty() && geaendert(l) > 1000, "PT lang fest: das bemalte Bild ist gebaut");
            mz().ordner[0] = '\0';
        }));
        add(pause(0.3));
        add(grafikGleich("nach den festen Strichen", "torso"));
        add(tu("PT lang fest: Rueckgaengig bis zum Original, Wiederholen bis zum selben Bild", [bildSumme, striche] {
            int zurueck = 0;
            while (zurueck < striche + 5 && modellMalenRueckgaengig()) { ++zurueck; }
            const mm::Leinwand* l = lw();
            const bool original = l != nullptr && l->bild == l->grund && l->ebenen[0].rgba == l->grund;
            int vor = 0;
            while (vor < striche + 5 && modellMalenWiederholen()) { ++vor; }
            l = lw();
            const std::uint64_t wieder = l != nullptr ? fnvBytes(l->bild.data(), l->bild.size()) : 0U;
            diag::detail("Knopftest: PT lang fest - " + std::to_string(zurueck) + " Schritte zurueck: Original " + (original ? "ja" : "nein") + "; " + std::to_string(vor) +
                         " Schritte vor: Bild " + hexVon(wieder));
            pruefeWahr(zurueck > 0 && original, "PT lang fest: Rueckgaengig bringt das Original Byte fuer Byte zurueck");
            pruefeWahr(vor == zurueck && wieder == *bildSumme, "PT lang fest: Wiederholen bringt dasselbe Bild zurueck");
        }));
        add(pause(0.3));
        add(grafikGleich("nach Rueckgaengig und Wiederholen", "torso"));
    }
    return s;
}

// --- MBMAL Punkt 5 (BHED_MODELL_NUR=ptsprung): ein Strich in der 3D-Ansicht verteilt seine Tupfer am Schirm wie Substance
// Painter und Blender (shank 10.11. "okay ja mach das"). Springt der Pinsel zwischen zwei Koerperteilen (kyle in einer Pose,
// r_hand -> l_hand in einem Mausereignis wie eine schnelle Hand), verband das Programm die Treffer in der Ruhelage und legte
// dort eine Tupferlinie: Streifen, wo der Pinsel nie war. Je Texel, den der Strich aendert, die Stelle in der Pose am Schirm
// (modellMalenPfadPruefen): keiner weiter vom gezogenen Weg als die Grenze. Dazu ein Strich auf einer zusammenhaengenden
// Flaeche (torso, ohne Sprung): seine Texel in ptsprung_flaeche.bin im Laufordner (Vergleich vorher/nachher ausserhalb, siehe
// Y:\werkzeuge\mbm\flaeche_vergleich.py), und was der Sprung kostet (Zeit, Tupfer, Strahlen).
std::vector<Schritt> sprungSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct Stand {
        std::vector<std::uint8_t> vorher;
        ImVec2 von{}, nach{};
        mm::Messung ms0{};
        int tupfer0 = 0;
        mm::Messung jetzt{};
        int tupfer = 0;
        bool begonnen = false;
    };
    auto st = std::make_shared<Stand>();
    figurLaden(s, "max", "models/players/kyle/model.glm");
    add(tu("PT Sprung: kyle body, Pinsel wie shanks Bild, Pose BOTH_A2_TR_BL (Mitte, steht)", [] {
        g_app->mtab.auswahl = {teilVon("torso")};
        mz().gruppe = -1;
        mz().stufe = 0;
        g_app->mtab.ansichten[1].art = AnsichtArt::Perspektive;
        mz().werkzeug = static_cast<int>(mm::Werkzeug::Pinsel);
        mz().radiusPx = 24.0F;
        mz().pinsel.staerke = 1.0F;
        mz().pinsel.abstand = 0.1F;
        mz().pinsel.farbe = {0.9F, 0.1F, 0.1F, 1.0F};
        mz().druckStaerke = true;
        mz().druckGroesse = false;
        mz().spiegeln = false;
        mz().uvZeigen = true;
        mz().editorOffen = true;
        for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
            if (g_app->animList[i].name == "BOTH_A2_TR_BL") {
                animWaehlen(static_cast<int>(i), false);
                g_app->animFrame = g_app->animList[i].firstFrame + g_app->animList[i].numFrames / 2;
            }
        }
        g_app->animPlaying = false;
        g_app->modelDirty = true;
        (void)lw();   // die Leinwand der Auswahl (torso: body) bleibt gewaehlt, auch ohne Auswahl
    }));
    add(pause(0.5));
    add(tu("PT Sprung: von der Seite (Gier 180, Nick 0), einpassen (ganze Figur)", [] {
        g_app->mtab.auswahl.clear();
        // Die Blickrichtung fest, nicht die Startansicht: geschrieben und gemessen wurde der Test in der Startansicht bis rc611
        // (Gier 180, von der Seite; groesster Schritt zweier Tupfer des Strichs ueber den torso in der Ruhelage 0,23). Seit
        // MBKOPF beginnt die Perspektive vor dem Gesicht (Gier 90); von dort lagen zwei Tupfer desselben Strichs 1,36
        // auseinander, ueber der Grenze 1,0 fuer "ohne Sprung" (zusammengefuehrter Stand, Lauf z2_ptsprung: 2 OK, 1 FEHLER).
        g_app->modelOrbit.turn(g_app->modelCam, 180.0F - g_app->modelCam.angles[1], -g_app->modelCam.angles[0]);
        modellAnsichtEinpassenAktiv();
    }));
    add(pause(1.0));
    add(pruefe("PT Sprung: Leinwand body, beide Haende am Schirm", [st] {
        const mm::Leinwand* l = lw();
        const bool ha = modellFlaecheAufSchirm(teilVon("r_hand"), st->von);
        const bool hb = modellFlaecheAufSchirm(teilVon("l_hand"), st->nach);
        char z[200];
        std::snprintf(z, sizeof z, "Knopftest: PT Sprung - r_hand %.0f %.0f, l_hand %.0f %.0f, Abstand %.0f Punkte", static_cast<double>(st->von.x),
                      static_cast<double>(st->von.y), static_cast<double>(st->nach.x), static_cast<double>(st->nach.y),
                      static_cast<double>(std::hypot(st->nach.x - st->von.x, st->nach.y - st->von.y)));
        diag::detail(z);
        const ModellAnsicht& av = g_app->mtab.ansichten[g_app->mtab.aktiv];
        const auto drin = [&av](ImVec2 p) { return p.x > av.lo[0] && p.y > av.lo[1] && p.x < av.lo[0] + av.groesse[0] && p.y < av.lo[1] + av.groesse[1]; };
        return l != nullptr && kleinPt(l->datei).find("/body.") != std::string::npos && ha && hb && drin(st->von) && drin(st->nach);
    }));
    // der Sprung: Taste auf r_hand, die Maus springt in einem Ereignis auf l_hand, Taste los
    add(Schritt{"PT Sprung: Strich r_hand -> l_hand in einem Mausereignis", [st](int bild) {
                    ImGuiIO& io = ImGui::GetIO();
                    mm::Messung& ms = mm::messung();
                    if (bild == 0) {
                        st->vorher = lw() != nullptr ? lw()->bild : std::vector<std::uint8_t>{};
                        ms = mm::Messung{};
                        ms.an = true;
                        setzeMaus(st->von);
                    } else if (bild == 2) {
                        io.AddMouseButtonEvent(0, true);
                    } else if (bild == 4) {
                        st->begonnen = mz().strichLaeuft;
                        st->ms0 = ms;
                        st->tupfer0 = mz().tupfer;
                        setzeMaus(st->nach);
                    } else if (bild == 6) {
                        st->jetzt = ms;
                        st->tupfer = mz().tupfer;
                        io.AddMouseButtonEvent(0, false);
                    } else if (bild >= 9) {
                        ms.an = false;
                        return true;
                    }
                    return false;
                }});
    add(pruefe("PT Sprung: kein Texel ausserhalb des gezogenen Wegs am Schirm", [st] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        const float grenze = 1.5F * mz().radiusPx;
        const MalPfadPruefung p = modellMalenPfadPruefen(a, st->vorher, {st->von, st->nach}, grenze);
        const mm::Messung& v = st->ms0;
        const mm::Messung& n = st->jetzt;
        char z[420];
        std::snprintf(z, sizeof z,
                      "Knopftest: PT Sprung - %d Texel geaendert, %d weiter als %.0f Punkte vom Weg (weitester %.0f), %d ausserhalb der Dreiecke; "
                      "das Sprung-Ereignis: %.2f ms, %d Tupfer (%lld ohne Texel), %lld Strahlen, %lld Stellen ohne Flaeche, groesster Sprung in der Ruhelage %.1f",
                      p.geaendert, p.ausserhalb, static_cast<double>(grenze), static_cast<double>(p.weitestePx), p.ohneDreieck, n.ansichtMs - v.ansichtMs,
                      st->tupfer - st->tupfer0, n.tupferLeer - v.tupferLeer, n.strahlen - v.strahlen, n.tupferDaneben - v.tupferDaneben, n.wegRuheMax);
        diag::detail(z);
        std::snprintf(z, sizeof z,
                      "Knopftest: PT Sprung - das Sprung-Ereignis in ms: Strahl %.2f, Suchen %.2f, Sichern %.2f, Werkzeug %.2f, Ausbluten %.2f, Zusammensetzen %.2f, "
                      "Hochladen %.2f; %lld Texel getroffen, %lld bearbeitet",
                      n.trefferMs - v.trefferMs, n.suchenMs - v.suchenMs, n.sichernMs - v.sichernMs, n.werkzeugMs - v.werkzeugMs, n.blutenMs - v.blutenMs,
                      n.zusammenMs - v.zusammenMs, n.hochladenMs - v.hochladenMs, n.treffer - v.treffer, n.rechteckTexel - v.rechteckTexel);
        diag::detail(z);
        if (!p.flaechenAusserhalb.empty()) { diag::detail("Knopftest: PT Sprung - ausserhalb des Wegs: " + p.flaechenAusserhalb); }
        return st->begonnen && p.geaendert > 50 && p.ausserhalb == 0;
    }));
    add(tu("PT Sprung: Rueckgaengig", [] { (void)modellMalenRueckgaengig(); }));
    add(pause(0.3));
    // ein Strich auf einer zusammenhaengenden Flaeche: Mitte des torso 25 Punkte nach links bis 25 nach rechts
    add(tu("PT Sprung: Flaeche - vorher", [st] {
        st->vorher = lw() != nullptr ? lw()->bild : std::vector<std::uint8_t>{};
        mm::messung() = mm::Messung{};
        mm::messung().an = true;
        st->tupfer0 = mz().tupfer;
    }));
    add(mb3::ziehenVonNach("PT Sprung: Strich ueber den torso", [] { return torsoMitte(-25.0F); }, [] { return torsoMitte(25.0F); }));
    add(pause(0.3));
    // ohne Wegpruefung: der Tupfer ist eine Kugel in der Ruhelage und erreicht dort auch Flaechen, die in der Pose weiter weg
    // liegen (Bestand seit Modell-Block 1c, unabhaengig vom Abstand der Tupfer; die Namen stehen in der Zeile)
    add(pruefe("PT Sprung: Flaeche - Strich ohne Sprung, Texel nach ptsprung_flaeche.bin", [st] {
        const mm::Leinwand* l = lw();
        mm::Messung& ms = mm::messung();
        ms.an = false;
        if (l == nullptr || st->vorher.size() != l->bild.size()) { return false; }
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        const float grenze = 1.5F * mz().radiusPx;
        const MalPfadPruefung p = modellMalenPfadPruefen(a, st->vorher, {torsoMitte(-25.0F), torsoMitte(25.0F)}, grenze);
        // je geaenderter Texel: Nummer (4 Bytes) und RGBA
        std::string bin;
        int n = 0;
        for (std::size_t i = 0; i < l->bild.size() / 4U; ++i) {
            if (std::memcmp(&l->bild[i * 4U], &st->vorher[i * 4U], 4U) == 0) { continue; }
            const auto nr = static_cast<std::uint32_t>(i);
            bin.append(reinterpret_cast<const char*>(&nr), 4U);
            bin.append(reinterpret_cast<const char*>(&l->bild[i * 4U]), 4U);
            ++n;
        }
        std::ofstream(mb3::lauf("ptsprung_flaeche.bin"), std::ios::binary) << bin;
        char z[360];
        std::snprintf(z, sizeof z,
                      "Knopftest: PT Sprung Flaeche - %d Texel geaendert (Summe %s), %d weiter als %.0f Punkte vom Weg (weitester %.0f), %d Tupfer, %lld Strahlen, "
                      "groesster Sprung in der Ruhelage %.2f",
                      n, hexVon(fnvBytes(reinterpret_cast<const std::uint8_t*>(bin.data()), bin.size())).c_str(), p.ausserhalb, static_cast<double>(grenze),
                      static_cast<double>(p.weitestePx), mz().tupfer - st->tupfer0, ms.strahlen, ms.wegRuheMax);
        diag::detail(z);
        {   // wo der groesste Schritt liegt: Flaechen und Orte in der Ruhelage (zeigt, ob er zwischen zwei Koerperteilen liegt)
            const modell::Inhalt& in = g_app->mtab.dok.inhalt();
            const auto nameVon = [&in](int t) { return t >= 0 && static_cast<std::size_t>(t) < in.teile.size() ? in.teile[static_cast<std::size_t>(t)].name : std::string("-"); };
            std::snprintf(z, sizeof z, "Knopftest: PT Sprung Flaeche - der groesste Schritt: von %s (%.2f %.2f %.2f) nach %s (%.2f %.2f %.2f)",
                          nameVon(ms.sprungFlaechen[0]).c_str(), ms.sprungVon[0], ms.sprungVon[1], ms.sprungVon[2], nameVon(ms.sprungFlaechen[1]).c_str(),
                          ms.sprungNach[0], ms.sprungNach[1], ms.sprungNach[2]);
            diag::detail(z);
        }
        if (!p.flaechenAusserhalb.empty()) { diag::detail("Knopftest: PT Sprung Flaeche - ausserhalb des Wegs: " + p.flaechenAusserhalb); }
        return n > 50 && ms.wegRuheMax < 1.0;
    }));
    add(zurueckAufOriginal());
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    if (n == "ptlang") { return langSchritte(); }
    if (n == "ptsprung") { return sprungSchritte(); }
    if (n == "ptfix3") { return fix3Schritte(); }
    if (n == "ptstufen") { return stufenSchritte(); }
    if (n == "ptfix") { return fixSchritte(); }
    if (n == "ptfix2") { return fix2Schritte(); }
    if (n == "pt") { return modellSchritte(); }
    if (n == "ptspiel") { return spielSchritte(); }
    if (n == "ptzeit") { return zeitSchritte(); }
    return {};
}

// --- Handbuch (BHED_HB_NUR=pt): h_pt_malen (3D-Strich, 2D-Bild, Ebenen), h_pt_ebenen (Maske, zwei Ebenen) -------------
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    figurLaden(s, "max");
    add(tu("PT Handbuch: Gruppe torso", [] {
        g_app->mtab.auswahl = {teilVon("torso")};
        mz().gruppe = -1;
        mz().pinsel.farbe = {0.85F, 0.1F, 0.1F, 1.0F};
        mz().radiusPx = 16.0F;
    }));
    add(pause(0.5));
    add(mb3::ziehenVonNach("PT Handbuch: Strich ueber die Brust", [] { return torsoMitte(-50.0F); }, [] { return torsoMitte(50.0F); }));
    add(pause(0.2));
    add(ausloesen(Str::PtEbeneNeu));
    add(tu("PT Handbuch: blau auf der Ebene", [] { mz().pinsel.farbe = {0.1F, 0.3F, 0.95F, 1.0F}; }));
    add(mb3::ziehenVonNach("PT Handbuch: Strich im Bild", [] { return bildPunkt(0.25F, 0.55F); }, [] { return bildPunkt(0.75F, 0.55F); }));
    add(tu("PT Handbuch: Maus auf die Brust", [] { setzeMaus(torsoMitte(10.0F)); }));
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_pt_malen"));
    add(zurueckAufOriginal());
    add(tu("PT Handbuch: Ende", [] {
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.vierAnsichten = true;
    }));
    return s;
}

}  // namespace pt1c
