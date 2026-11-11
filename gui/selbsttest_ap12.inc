// selbsttest_ap12.inc - AP12: Auto-Skin, Kern (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=ap12 | ap12foto)
//
//   Erreichbarkeit  "Automatisch gewichten" und "Auto-Skin: Einstellungen" in "Alle Aktionen suchen" (X wie 3ds Max, F3 wie
//                   Blender) und im Rechtsklick der Ansicht (Weights), mit Tooltip; das Fenster traegt jedes Bedienelement
//                   der Gruppe aus dem Entwurf (W7) mit Tooltip
//   Lauf            kyle mit Ravens Gewichten: rot vorher (kein Ergebnis, die Gewichte der Datei), gruen nachher - jede Ecke
//                   aller Stufen neu, hoechstens 4 Gewichte, hoechstens 32 Knochen je Flaeche, jede Ecke wie das Spiel sie
//                   liest; EIN Rueckgaengig-Schritt (zurueck = bitgleich die Datei, wiederholen = bitgleich das Ergebnis);
//                   die Anzeige zeichnet die neuen Gewichte; das Ergebnis nennt den Abstand zu den Gewichten davor
//   Verfahren       Huelle (kein Voxelgitter); Uebertragen vom selben Modell = bitgleich die Datei; Original fehlt:
//                   Meldung, kein Rueckgaengig-Schritt; nur die gewaehlte Flaeche
//   ap12foto        das Fenster vor und nach dem Lauf, in beiden Bedienmodi, beim Uebertragen und mit Meldung (BHED_FOTOS)
//   BHED_AP12_MODELL=<.glm>   Vorgabe models/players/kyle/model.glm
// Eingebunden in selbsttest.cpp (nach selbsttest_mbefoto.inc).
namespace ap12 {

using rig3::woIst;

std::string modellPfad() {
    const char* p = std::getenv("BHED_AP12_MODELL");
    return p != nullptr && *p != '\0' ? p : "models/players/kyle/model.glm";
}

bool imFenster(const Element& e) { return e.fenster.find("mtautoskin") != std::string::npos || e.innen.find("mtautoskin") != std::string::npos; }

const Element* imFensterMit(const std::string& label) {
    for (const Element& e : g_elemente) {
        if (imFenster(e) && (e.label == label || e.label.rfind(label + "##", 0) == 0)) { return &e; }
    }
    return nullptr;
}

std::string nurAuswahlText() {
    const ModellTab& t = g_app->mtab;
    int n = 0;
    for (const int i : t.auswahl) {
        if (i >= 0 && static_cast<std::size_t>(i) < t.dok.inhalt().teile.size() && !t.dok.inhalt().teile[static_cast<std::size_t>(i)].istTag()) { ++n; }
    }
    return std::string(tr(Str::AsNurAuswahl)) + " (" + std::to_string(n) + ")";
}

// Die Bedienelemente der Gruppe (Entwurf W7: drei Verfahren, Aufloesung, Steifigkeit, Glaetten, Naehte schliessen, Drehknochen
// nutzen, Danach spiegeln, Hauptknopf; dazu Gesicht/Anhaenge und nur die Auswahl). Die Regler tragen ihre Beschriftung
// links daneben und heissen selbst nur ##as...
std::vector<std::string> bedienelemente() {
    return {tr(Str::AsVerfUebertragen), tr(Str::AsVerfVoxel), tr(Str::AsVerfHuelle), "##asaufloesung", "##assteifigkeit", "##asglaetten",
            tr(Str::AsNaehte),          tr(Str::AsDreh),      tr(Str::AsDetail),     tr(Str::AsSpiegeln), nurAuswahlText(),  tr(Str::AsAusfuehren)};
}

// Die Gewichte aller Ecken aller Flaechen und Stufen als Bytes (Knochen und Gewichte, wie sie in der Ecke stehen).
std::string gewichtBytes(const modell::Inhalt& in, int nurTeil = -1) {
    std::string s;
    for (std::size_t t = 0; t < in.teile.size(); ++t) {
        if (in.teile[t].istTag() || (nurTeil >= 0 && static_cast<int>(t) != nurTeil)) { continue; }
        for (const modell::Netz& n : in.teile[t].stufen) {
            for (const modell::Ecke& e : n.ecken) {
                s.append(reinterpret_cast<const char*>(e.knochen), sizeof e.knochen);
                s.append(reinterpret_cast<const char*>(e.gewicht), sizeof e.gewicht);
            }
        }
    }
    return s;
}

// Was das Spiel aus den Gewichten einer Ecke macht: je Knochen sein Anteil.
std::map<int, double> anteile(const modell::Ecke& e) {
    std::map<int, double> m;
    for (int k = 0; k < 4; ++k) {
        if (e.gewicht[k] > 0.0F) { m[e.knochen[k]] += static_cast<double>(e.gewicht[k]); }
    }
    return m;
}

// Gleich bis auf die float-Rundung des letzten Gewichts (1 - Summe): ein Millionstel
bool gleicheAnteile(const std::map<int, double>& a, const std::map<int, double>& b) {
    if (a.size() != b.size()) { return false; }
    for (const auto& [k, w] : a) {
        const auto it = b.find(k);
        if (it == b.end() || std::abs(it->second - w) > 1e-6) { return false; }
    }
    return true;
}

// Uebertragen vom Modell selbst: Ecken, deren Gewichte danach anders sind als in der Datei - und wie viele davon an
// einem Ort liegen, an dem die Datei selbst verschiedene Gewichte traegt (dort wird aus mehreren Ecken ein Punkt).
struct SelbstAbgleich {
    int ecken = 0;
    int anders = 0;
    int andersAnNaht = 0;
    int nahtEcken = 0;
};
SelbstAbgleich selbstAbgleich(const modell::Inhalt& datei, const modell::Inhalt& neu) {
    SelbstAbgleich r;
    for (int stufe = 0; stufe < datei.stufenZahl; ++stufe) {
        // je Ort die Anteile der ersten Ecke; uneinheitlich, wenn eine weitere Ecke am selben Ort andere traegt
        std::map<std::array<float, 3>, std::pair<std::map<int, double>, bool>> orte;
        for (const modell::Teil& t : datei.teile) {
            if (t.istTag() || static_cast<std::size_t>(stufe) >= t.stufen.size()) { continue; }
            for (const modell::Ecke& e : t.stufen[static_cast<std::size_t>(stufe)].ecken) {
                const std::array<float, 3> ort = {e.ort[0] + 0.0F, e.ort[1] + 0.0F, e.ort[2] + 0.0F};
                const auto [it, erster] = orte.try_emplace(ort, anteile(e), false);
                if (!erster && !gleicheAnteile(it->second.first, anteile(e))) { it->second.second = true; }
            }
        }
        for (std::size_t t = 0; t < datei.teile.size() && t < neu.teile.size(); ++t) {
            if (datei.teile[t].istTag() || static_cast<std::size_t>(stufe) >= datei.teile[t].stufen.size()) { continue; }
            const modell::Netz& a = datei.teile[t].stufen[static_cast<std::size_t>(stufe)];
            const modell::Netz& b = neu.teile[t].stufen[static_cast<std::size_t>(stufe)];
            for (std::size_t e = 0; e < a.ecken.size() && e < b.ecken.size(); ++e) {
                const std::array<float, 3> ort = {a.ecken[e].ort[0] + 0.0F, a.ecken[e].ort[1] + 0.0F, a.ecken[e].ort[2] + 0.0F};
                const bool naht = orte[ort].second;
                const bool gleich = gleicheAnteile(anteile(a.ecken[e]), anteile(b.ecken[e]));
                ++r.ecken;
                r.nahtEcken += naht ? 1 : 0;
                r.anders += gleich ? 0 : 1;
                r.andersAnNaht += !gleich && naht ? 1 : 0;
            }
        }
    }
    return r;
}

int eckenZahl(const modell::Inhalt& in) {
    int n = 0;
    for (const modell::Teil& t : in.teile) {
        if (t.istTag()) { continue; }
        for (const modell::Netz& netz : t.stufen) { n += static_cast<int>(netz.ecken.size()); }
    }
    return n;
}

// Steht in jeder Ecke, was eine .glm tragen kann und das Spiel liest (G2_GetVertBoneWeight: die ersten n - 1 Gewichte
// 10-Bit-Werte, absteigend, das letzte 1 - Summe in float)?
bool wieImSpiel(const modell::Inhalt& in, int& gewichteMax, int& knochenMax) {
    gewichteMax = 0;
    knochenMax = 0;
    for (const modell::Teil& t : in.teile) {
        if (t.istTag()) { continue; }
        for (const modell::Netz& n : t.stufen) {
            const modell::FlaechenMass m = modell::mass(n);
            gewichteMax = std::max(gewichteMax, m.gewichteMax);
            knochenMax = std::max(knochenMax, m.knochen);
            for (const modell::Ecke& e : n.ecken) {
                int zahl = 0;
                while (zahl < 4 && e.gewicht[zahl] > 0.0F) { ++zahl; }
                if (zahl == 0) { return false; }
                float summe = 0.0F;
                for (int k = 0; k < 4; ++k) {
                    if (k >= zahl) {
                        if (e.gewicht[k] != 0.0F) { return false; }
                        continue;
                    }
                    if (k + 1 == zahl) {
                        const float rest = 1.0F - summe;
                        if (std::memcmp(&rest, &e.gewicht[k], sizeof rest) != 0) { return false; }
                        continue;
                    }
                    const int bits = static_cast<int>(std::lround(static_cast<double>(e.gewicht[k]) * 1023.0));
                    const float soll = modell::g2Gewicht(bits);
                    if (bits < 1 || bits > 1023 || std::memcmp(&soll, &e.gewicht[k], sizeof soll) != 0) { return false; }
                    if (k > 0 && e.gewicht[k] > e.gewicht[k - 1]) { return false; }
                    summe += e.gewicht[k];
                }
            }
        }
    }
    return true;
}

struct Zustand {
    std::string datei;      // die Gewichte, wie die Datei sie brachte
    std::string ergebnis;   // nach dem ersten Lauf
    std::string undoVorher;
    int stand = 0;
    modell::Inhalt dateiInhalt;   // das Dokument, wie die Datei es brachte (fuer den Abgleich beim Uebertragen)
};

// Eine Aktion ueber "Alle Aktionen suchen" (X / F3) ausloesen.
void ueberSuche(std::vector<Schritt>& s, Str text) {
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
    s.push_back(tu("AP12: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; }));
    s.push_back(ik6::tasteSuche());
    s.push_back(pause(0.2));
    s.push_back(pruefe("AP12 Suche: Alle Aktionen suchen ist offen (X / F3)", [] { return g_app->mtab.aktionenSichtbar; }));
    const std::string zeile = tr(text);
    s.push_back(tippe(zeile));
    s.push_back(pause(0.2));
    s.push_back(klickAn("\"" + zeile + "\" in der Suche", [zeile] {
        for (const Element& e : g_elemente) {
            if (e.label == zeile && e.fenster.find("##Popup_") != std::string::npos) { return e.rect.GetCenter(); }
        }
        meldeFehler("AP12: " + zeile + " nicht in der Suche: " + woIst(zeile));
        return ImVec2{};
    }));
    s.push_back(pause(0.3));
}

Schritt gerechnet(const std::string& was) {
    return pruefe("AP12 " + was + ": der Lauf ist durch, mit Ergebnis fuer dieses Modell", [] {
        const ModellAutoSkin& z = modellAutoSkinZustand();
        return z.gelaufen && z.fehler.empty() && z.bericht.ok && z.ladeStand == g_app->mtab.dok.ladeStand();
    });
}

std::vector<Schritt> grundSchritte(std::shared_ptr<Zustand> z, bool blender) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("AP12: Skript fuer den Test", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "ap12.txt"); }));
    for (Schritt& x : mb1::laden(modellPfad())) { add(std::move(x)); }
    add(tu(std::string("AP12: Bereich Weights, ") + (blender ? "wie Blender" : "wie 3ds Max") + ", Einstellungen ab Werk", [z, blender] {
        modellBedienungSetzen(blender ? 1 : 0);
        g_app->mtab.bereich = ModellBereich::Weights;
        g_app->mtab.auswahl.clear();
        ModellAutoSkin& a = modellAutoSkinZustand();
        a = ModellAutoSkin{};
        z->datei = gewichtBytes(g_app->mtab.dok.inhalt());
        z->undoVorher = g_app->mtab.dok.rueckgaengigName();
    }));
    add(pause(0.3));
    return s;
}

std::vector<Schritt> schritteAp12() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<Zustand>();
    for (Schritt& x : grundSchritte(z, false)) { add(std::move(x)); }
    add(pruefe("AP12 Modell: eine .glm mit Skelett, dessen Knochenzahl passt", [] {
        const modell::Inhalt& in = g_app->mtab.dok.inhalt();
        diag::detail("Knopftest: AP12 Modell " + in.name + ", " + std::to_string(eckenZahl(in)) + " Ecken in " + std::to_string(in.stufenZahl) +
                     " Stufen, " + std::to_string(in.knochenZahl) + " Knochen, GLA " + std::to_string(g_app->anim.bones.size()));
        return in.herkunft == modell::Herkunft::Glm && in.knochenZahl > 1 && in.knochenZahl == static_cast<int>(g_app->anim.bones.size());
    }));
    // --- Erreichbarkeit ---
    add(pruefe("AP12 Aktionen: Automatisch gewichten und Auto-Skin: Einstellungen stehen bei den Aktionen ohne Taste (Suche und Rechtsklick > "
               "Weights), mit Tooltip",
               [] {
                   int da = 0;
                   for (const Str x : {Str::AsAusfuehren, Str::AsOeffnen}) {
                       for (int i = 0; i < modellZusatzZahl(); ++i) {
                           if (std::string(modellZusatzText(i)) == tr(x) && std::string(modellZusatzHinweis(i)).size() > 20U) { ++da; }
                       }
                   }
                   return da == 2;
               }));
    add(pruefe("AP12 vorher: das Fenster ist zu, es gibt kein Ergebnis", [] {
        const ModellAutoSkin& a = modellAutoSkinZustand();
        return !a.fensterSichtbar && !a.gelaufen;
    }));
    ueberSuche(s, Str::AsOeffnen);
    add(pruefe("AP12 Suche: Auto-Skin: Einstellungen oeffnet das Fenster", [] { return modellAutoSkinZustand().fensterSichtbar; }));
    add(pruefe("AP12 Fenster: jedes Bedienelement der Gruppe ist da", [] {
        bool ok = true;
        for (const std::string& l : bedienelemente()) {
            if (imFensterMit(l) == nullptr) {
                meldeFehler("AP12: \"" + l + "\" fehlt im Fenster: " + woIst(l));
                ok = false;
            }
        }
        return ok;
    }));
    for (const std::string& l : bedienelemente()) {
        add(mbefoto::mausAuf("AP12 " + l, [l] { return imFensterMit(l); }));
        add(pruefe("AP12 Tooltip: \"" + l + "\"", [] { return mbefoto::tooltipLage() != "kein Tooltip"; }));
    }
    add(tu("AP12: Maus weg", [] { setzeMaus(ImVec2{4.0F, 4.0F}); }));
    add(pause(0.2));
    // --- der Lauf ueber den Hauptknopf ---
    add(klickAn("Automatisch gewichten (Hauptknopf im Fenster)", [] {
        const Element* e = imFensterMit(tr(Str::AsAusfuehren));
        if (e == nullptr) {
            meldeFehler("AP12: Hauptknopf nicht gefunden");
            return ImVec2{};
        }
        return e->rect.GetCenter();
    }));
    add(pause(0.5));
    add(gerechnet("Hauptknopf"));
    add(pruefe("AP12 Lauf: jede Ecke aller Stufen neu gewichtet, hoechstens 4 Gewichte je Ecke, hoechstens 32 Knochen je Flaeche, jede Ecke wie das Spiel sie liest",
               [z] {
                   const modell::Inhalt& in = g_app->mtab.dok.inhalt();
                   const ModellAutoSkin& a = modellAutoSkinZustand();
                   const modell::Grenzen& g = modell::grenzen(static_cast<modell::Zielspiel>(g_app->mtab.zielspiel));
                   int gewichte = 0;
                   int knochen = 0;
                   const bool spiel = wieImSpiel(in, gewichte, knochen);
                   z->ergebnis = gewichtBytes(in);
                   char t[400];
                   std::snprintf(t, sizeof t,
                                 "Knopftest: AP12 Lauf - %d Ecken (Modell %d), Gewichte je Ecke %d, Knochen je Flaeche %d (%s), benutzt %d von %d, "
                                 "Voxel %d x %d x %d, fest %d, Knochen aussen %d, ohne Weg %d, %.0f ms",
                                 a.bericht.ecken, eckenZahl(in), gewichte, knochen, a.bericht.knochenJeFlaecheName.c_str(), a.bericht.knochenBenutzt,
                                 a.bericht.knochenTragend, a.bericht.voxel[0], a.bericht.voxel[1], a.bericht.voxel[2], a.bericht.voxelFest,
                                 a.bericht.knochenAussen, a.bericht.punkteUnerreicht, a.ms);
                   diag::detail(t);
                   return spiel && a.bericht.ecken == eckenZahl(in) && gewichte <= g.gewichteJeEcke && gewichte == a.bericht.gewichteMax &&
                          knochen <= g.knochenJeFlaeche && knochen == a.bericht.knochenJeFlaecheMax && z->ergebnis != z->datei;
               }));
    add(pruefe("AP12 Ergebnis: Abstand zu den Gewichten davor ueber 200 Bilder der GLA, in Einheiten", [] {
        const ModellAutoSkin& a = modellAutoSkinZustand();
        char t[300];
        std::snprintf(t, sizeof t, "Knopftest: AP12 Ergebnis - staerkster Knochen gleich an %d von %d Ecken, Gewichtsabweichung %.3f, Verformung Mittel %.3f, 95 %% %.3f, groesste %.2f Einheiten ueber %d Bilder",
                      a.vergleich.hauptGleich, a.vergleich.ecken, a.vergleich.abweichungMittel, a.verformung.mittel, a.verformung.p95, a.verformung.max,
                      a.verformung.bilder);
        diag::detail(t);
        return a.verformung.bilder == 200 && a.verformung.mittel > 0.0 && a.verformung.max >= a.verformung.p95 && a.vergleich.ecken > 0 &&
               a.vergleich.hauptGleich > 0 && a.vergleich.hauptGleich < a.vergleich.ecken;
    }));
    add(pruefe("AP12 Anzeige: das gezeichnete Modell traegt die neuen Gewichte", [] {
        const ModellTab& t = g_app->mtab;
        const modell::Inhalt& in = t.dok.inhalt();
        int verglichen = 0;
        for (std::size_t i = 0; i < in.teile.size() && i < t.anzeige.surfaces.size(); ++i) {
            if (in.teile[i].istTag() || static_cast<std::size_t>(t.lodJetzt) >= in.teile[i].stufen.size()) { continue; }
            const modell::Netz& n = in.teile[i].stufen[static_cast<std::size_t>(t.lodJetzt)];
            const GlmSurface& f = t.anzeige.surfaces[i];
            if (f.verts.size() != n.ecken.size()) { continue; }
            for (std::size_t k = 0; k < n.ecken.size(); ++k) {
                if (std::memcmp(f.verts[k].weights, n.ecken[k].gewicht, sizeof n.ecken[k].gewicht) != 0 ||
                    std::memcmp(f.verts[k].bones, n.ecken[k].knochen, sizeof n.ecken[k].knochen) != 0) {
                    return false;
                }
                ++verglichen;
            }
        }
        return verglichen > 100;
    }));
    add(pruefe("AP12 Rueckgaengig: genau ein Schritt mit dem Namen Auto-Skin", [z] {
        return g_app->mtab.dok.rueckgaengigName() == tr(Str::AsUndo) && z->undoVorher != tr(Str::AsUndo);
    }));
    add(tu("AP12: Rueckgaengig", [] { (void)modellRueckgaengig(); }));
    add(pruefe("AP12 Rueckgaengig: bitgleich die Gewichte der Datei, davor steht kein Auto-Skin-Schritt", [z] {
        return gewichtBytes(g_app->mtab.dok.inhalt()) == z->datei && g_app->mtab.dok.rueckgaengigName() == z->undoVorher;
    }));
    add(tu("AP12: Wiederholen", [] { (void)modellWiederholen(); }));
    add(pruefe("AP12 Wiederholen: bitgleich das Ergebnis", [z] { return gewichtBytes(g_app->mtab.dok.inhalt()) == z->ergebnis; }));
    add(tu("AP12: zurueck auf die Datei", [] { (void)modellRueckgaengig(); }));
    // --- die Aktion ueber die Suche (rechnet ohne das Fenster) ---
    add(tu("AP12: Ergebnis vergessen", [] {
        modellAutoSkinZustand().gelaufen = false;
        modellAutoSkinZustand().fensterOffen = false;
    }));
    ueberSuche(s, Str::AsAusfuehren);
    add(pause(0.5));
    add(gerechnet("Suche"));
    add(pruefe("AP12 Suche: Automatisch gewichten rechnet dasselbe wie der Hauptknopf (bitgleich), die Statuszeile meldet es", [z] {
        return gewichtBytes(g_app->mtab.dok.inhalt()) == z->ergebnis && g_app->mtab.meldung.find(tr(Str::AsUndo)) != std::string::npos;
    }));
    add(tu("AP12: zurueck auf die Datei", [] { (void)modellRueckgaengig(); }));
    // --- Verfahren ---
    add(tu("AP12: Verfahren Huelle", [] {
        modellAutoSkinZustand().o.verfahren = modell::AutoSkinVerfahren::Huelle;
        (void)modellAutoSkin();
    }));
    add(pruefe("AP12 Huelle: laeuft ohne Voxelgitter, ein anderes Ergebnis als geodaetisch, Grenzen eingehalten", [z] {
        const ModellAutoSkin& a = modellAutoSkinZustand();
        int gewichte = 0;
        int knochen = 0;
        const bool spiel = wieImSpiel(g_app->mtab.dok.inhalt(), gewichte, knochen);
        return a.gelaufen && a.bericht.voxelFest == 0 && spiel && gewichte <= 4 && knochen <= 32 && gewichtBytes(g_app->mtab.dok.inhalt()) != z->ergebnis;
    }));
    add(tu("AP12: zurueck auf die Datei, Verfahren Uebertragen vom Modell selbst", [z] {
        (void)modellRueckgaengig();
        z->dateiInhalt = g_app->mtab.dok.inhalt();
        ModellAutoSkin& a = modellAutoSkinZustand();
        a.o.verfahren = modell::AutoSkinVerfahren::Uebertragen;
        std::snprintf(a.vorlage, sizeof a.vorlage, "%s", modellPfad().c_str());
        (void)modellAutoSkin();
    }));
    // Bitgleich mit der Datei kann das nicht sein: Ecken gleichen Orts werden ein Punkt (in der Datei tragen sie teils
    // verschiedene Gewichte), und nicht absteigend abgelegte Gewichte werden umsortiert. Geprueft wird, was das Spiel aus
    // den Gewichten macht: jede Ecke an einem Ort mit einheitlichen Gewichten traegt dieselben Anteile wie in der Datei.
    add(pruefe("AP12 Uebertragen: vom selben Modell kommen die Gewichte der Datei (anders nur an Orten, an denen die Datei selbst verschiedene "
               "Gewichte traegt), kein Punkt ist fern",
               [z] {
                   const ModellAutoSkin& a = modellAutoSkinZustand();
                   const SelbstAbgleich r = selbstAbgleich(z->dateiInhalt, g_app->mtab.dok.inhalt());
                   char t[400];
                   std::snprintf(t, sizeof t,
                                 "Knopftest: AP12 Uebertragen - %d Punkte vom Original, %d fern; %d Ecken, davon %d an Orten mit verschiedenen Gewichten in "
                                 "der Datei; anders %d, davon %d an solchen Orten; staerkster Knochen gleich an %d, groesster Verformungsfehler %.6f",
                                 a.bericht.uebertragen, a.bericht.fern, r.ecken, r.nahtEcken, r.anders, r.andersAnNaht, a.vergleich.hauptGleich,
                                 a.verformung.max);
                   diag::detail(t);
                   return a.gelaufen && a.bericht.fern == 0 && a.bericht.uebertragen > 0 && r.ecken == eckenZahl(g_app->mtab.dok.inhalt()) &&
                          r.anders == r.andersAnNaht;
               }));
    add(tu("AP12: Original, das es nicht gibt", [z] {
        ModellAutoSkin& a = modellAutoSkinZustand();
        std::snprintf(a.vorlage, sizeof a.vorlage, "models/players/ap12_gibt_es_nicht/model.glm");
        z->stand = static_cast<int>(g_app->mtab.dok.rueckgaengigName() == tr(Str::AsUndo));
        z->undoVorher = gewichtBytes(g_app->mtab.dok.inhalt());
        (void)modellAutoSkin();
    }));
    add(pruefe("AP12 Uebertragen ohne Original: Meldung mit dem Pfad, kein Ergebnis, das Modell bleibt", [z] {
        const ModellAutoSkin& a = modellAutoSkinZustand();
        return !a.gelaufen && a.fehler.find("ap12_gibt_es_nicht") != std::string::npos && g_app->mtab.meldung == a.fehler &&
               gewichtBytes(g_app->mtab.dok.inhalt()) == z->undoVorher;
    }));
    // --- nur die gewaehlte Flaeche ---
    add(tu("AP12: zurueck auf die Datei, nur die Flaeche torso, geodaetisch", [z] {
        (void)modellRueckgaengig();
        ModellTab& t = g_app->mtab;
        ModellAutoSkin& a = modellAutoSkinZustand();
        a.o.verfahren = modell::AutoSkinVerfahren::Voxel;
        a.nurAuswahl = true;
        const int torso = t.dok.inhalt().teilIndex("torso");
        z->stand = torso;
        t.auswahl = {torso};
        (void)modellAutoSkin();
    }));
    add(pruefe("AP12 nur Auswahl: die Flaeche torso ist neu gewichtet, alle anderen tragen bitgleich die Gewichte der Datei", [z] {
        const modell::Inhalt& in = g_app->mtab.dok.inhalt();
        if (z->stand < 0) { return false; }
        // die Gewichte der Datei ohne torso gegen das Dokument ohne torso
        modell::Inhalt ohne = in;
        (void)modellRueckgaengig();
        const modell::Inhalt& datei = g_app->mtab.dok.inhalt();
        bool andere = true;
        for (std::size_t t = 0; t < in.teile.size(); ++t) {
            if (static_cast<int>(t) == z->stand) { continue; }
            andere = andere && gewichtBytes(ohne, static_cast<int>(t)) == gewichtBytes(datei, static_cast<int>(t));
        }
        const bool neu = gewichtBytes(ohne, z->stand) != gewichtBytes(datei, z->stand);
        return andere && neu && modellAutoSkinZustand().bericht.ecken < eckenZahl(in);
    }));
    add(tu("AP12: Fenster zu, Einstellungen ab Werk", [] {
        modellAutoSkinZustand() = ModellAutoSkin{};
        g_app->mtab.auswahl.clear();
    }));
    // --- wie Blender: F3 ---
    add(tu("AP12: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(0.3));
    ueberSuche(s, Str::AsOeffnen);
    add(pruefe("AP12 wie Blender: F3 > Auto-Skin: Einstellungen oeffnet das Fenster", [] { return modellAutoSkinZustand().fensterSichtbar; }));
    add(tu("AP12: aufraeumen", [] {
        modellAutoSkinZustand() = ModellAutoSkin{};
        modellBedienungSetzen(0);
    }));
    return s;
}

// Das Fenster vor und nach dem Lauf (BHED_FOTOS), 2560 x 1440 bei Oberflaeche 1,5 (BHED_AP12_FENSTER=<b>x<h>@<z>):
// ap12_vorher, ap12_nachher (wie 3ds Max), ap12_blender (wie Blender, nach dem Lauf), ap12_uebertragen, ap12_fehler
std::vector<Schritt> fotoSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<Zustand>();
    auto skala = std::make_shared<float>(1.0F);
    int b = 2560;
    int h = 1440;
    float o = 1.5F;
    if (const char* f = std::getenv("BHED_AP12_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &o); }
    add(tu("AP12 Foto: Fenster fuer die Fotos", [=] {
        *skala = g_app->uiScale;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = o;
    }));
    add(pause(0.8));
    for (Schritt& x : grundSchritte(z, false)) { add(std::move(x)); }
    add(tu("AP12 Foto: Fenster auf", [] { modellAutoSkinOeffnen(); }));
    add(pause(0.5));
    add(foto("ap12_vorher"));
    add(tu("AP12 Foto: rechnen", [] { (void)modellAutoSkin(); }));
    add(pause(0.5));
    add(foto("ap12_nachher"));
    add(tu("AP12 Foto: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(0.5));
    add(foto("ap12_blender"));
    add(tu("AP12 Foto: wie 3ds Max, Uebertragen vom Modell selbst", [] {
        modellBedienungSetzen(0);
        (void)modellRueckgaengig();
        ModellAutoSkin& a = modellAutoSkinZustand();
        a.o.verfahren = modell::AutoSkinVerfahren::Uebertragen;
        std::snprintf(a.vorlage, sizeof a.vorlage, "%s", modellPfad().c_str());
        (void)modellAutoSkin();
    }));
    add(pause(0.5));
    add(foto("ap12_uebertragen"));
    add(tu("AP12 Foto: Uebertragen mit einem Original, das fehlt", [] {
        ModellAutoSkin& a = modellAutoSkinZustand();
        std::snprintf(a.vorlage, sizeof a.vorlage, "models/players/ap12_gibt_es_nicht/model.glm");
        (void)modellAutoSkin();
    }));
    add(pause(0.5));
    add(foto("ap12_fehler"));
    add(tu("AP12 Foto: aufraeumen", [=] {
        (void)modellRueckgaengig();
        modellAutoSkinZustand() = ModellAutoSkin{};
        g_app->uiScale = *skala;
    }));
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    if (n == "ap12") { return schritteAp12(); }
    if (n == "ap12foto") { return fotoSchritte(); }
    return {};
}

}  // namespace ap12
