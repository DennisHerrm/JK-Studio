// selbsttest_menue_rc608.inc - Menue-Editor rc608 Teil 2: gezielte Pruefungen (BHED_MENUE_NUR=...).
// Eingebunden in namespace menuetest (selbsttest_menue.inc).

// rc608 Punkt 15 (Tester: "Noticed it doesn't display shaders"): Bilder werden aufgeloest wie im Spiel.
std::vector<Schritt> schritteShader() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("rc608 Shader: Forcemenue aus dem Spiel oeffnen", [] {
        me::schliesseAlleOhneSpeichern();
        g_app->leftMode = 4;
        tourSchliessen();
        (void)me::oeffneAusSpiel("ui/ingameforceselect.menu");
    }));
    add(pause(2.0));
    add(foto("menue_forceselect"));
    add(pruefe("rc608 Shader: keine Bild-Warnung im Forcemenue", [] {
        const auto x = me::stand();
        if (x.menus == 0) { return true; }   // Jedi Outcast hat dieses Menue nicht
        int bilder = 0;
        for (const std::string& d : x.diagnosen) {
            if (d.find("gfx/") != std::string::npos) {
                diag::info("Menue-Editor Diagnose: " + d);
                ++bilder;
            }
        }
        diag::info("Knopftest: Forcemenue Warnungen " + std::to_string(x.diagWarnungen) + ", davon Bilder " + std::to_string(bilder) + ", Bilder geladen " + std::to_string(x.bilderGeladen));
        return bilder == 0 && x.bilderGeladen >= 20;
    }));
    // Tabelle ueber alle Originalmenues der eingestellten Spieldaten. Was fehlt, muss genau das sein, was ein unabhaengiges
    // Skript nach den Regeln der Engine auch nicht findet (SP/rc608/bildtabelle.py, Vergleich der Zeilen ausserhalb)
    add(pruefe("rc608 Shader: Tabelle ueber alle Originalmenues", [] {
        const std::vector<me::BildZeile> tab = me::bildTabelle();
        int da = 0;
        for (const me::BildZeile& b : tab) {
            if (b.da) {
                ++da;
            } else {
                diag::info("Bildtabelle FEHLT " + b.datei + ":" + std::to_string(b.zeile) + " " + b.art + " " + b.name);
            }
        }
        diag::info("Knopftest: Bildtabelle " + std::to_string(tab.size()) + " Eintraege, " + std::to_string(da) + " Treffer, " + std::to_string(tab.size() - static_cast<std::size_t>(da)) + " fehlen");
        return !tab.empty();
    }));
    return s;
}
