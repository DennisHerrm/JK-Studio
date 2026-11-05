// selbsttest_menue_f46.inc - rc610 F46 B6-1: Seitenwechsel aus dem Code (Skript im Bedienen-Modus, Hierarchie,
// Gliederung) leert die Auswahl wie der Reiter (BHED_MENUE_NUR=seitenwechsel, auch im vollen Lauf).
// Eingebunden in namespace menuetest (selbsttest_menue.inc).
// Vorher blieben die Indizes des alten Menues stehen: gizmoTreffer las items[10] eines Menues mit drei Elementen,
// und Strg-Klick in der Hierarchie + Entf loeschte im anderen Menue ein Element, das niemand gewaehlt hatte.

// "gross": zwoelf Knoepfe, g0 oeffnet "klein" (drei Knoepfe)
std::string seitenwechselText() {
    std::string t = "menuDef\n{\n  name \"gross\"\n  rect 0 0 640 480\n  fullScreen 1\n  visible 1\n";
    for (int i = 0; i < 12; ++i) {
        char z[320];
        std::snprintf(z, sizeof(z),
                      "  itemDef\n  {\n    name \"g%d\"\n    type ITEM_TYPE_BUTTON\n    rect %d %d 100 30\n    text \"G%d\"\n    visible 1\n%s  }\n", i,
                      20 + (i % 4) * 150, 20 + (i / 4) * 100, i, i == 0 ? "    action { open klein }\n" : "");
        t += z;
    }
    t += "}\nmenuDef\n{\n  name \"klein\"\n  rect 0 0 640 480\n  fullScreen 1\n  visible 0\n";
    for (int i = 0; i < 3; ++i) {
        char z[240];
        std::snprintf(z, sizeof(z), "  itemDef\n  {\n    name \"k%d\"\n    type ITEM_TYPE_BUTTON\n    rect 20 %d 100 30\n    text \"K%d\"\n    visible 1\n  }\n", i,
                      20 + i * 100, i);
        t += z;
    }
    return t + "}\n";
}

std::vector<Schritt> schritteSeitenwechsel() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto ganzeDateiVorher = std::make_shared<bool>(false);
    const auto oeffnen = [&](const std::string& was, bool ganzeDatei) {
        add(tu("F46 B6-1 " + was + ": Probe oeffnen", [ganzeDatei] {
            me::schliesseAlleOhneSpeichern();
            g_app->leftMode = 4;
            tourSchliessen();
            g_app->settings.menueGanzeDatei = ganzeDatei;
            (void)me::oeffneText("ui/seitenwechsel.menu", seitenwechselText());
            me::setzeInteraktion(false);
            me::setzeSpalten(0.0F, 0.0F);
            me::setzeReiter(2);
            me::leisteVorgabe();
        }));
        add(pause(0.3));
    };
    const auto namenText = [] {
        std::string t;
        for (const me::ItemMerker& x : me::itemMerker()) { t += (t.empty() ? "" : ",") + x.name; }
        return t;
    };

    add(tu("F46 B6-1: Einstellung Ganze Datei merken", [ganzeDateiVorher] { *ganzeDateiVorher = g_app->settings.menueGanzeDatei; }));
    // a) Bedienen: der Knopf oeffnet per Skript ein kleineres Menue, die Seite folgt; zurueck auf Bearbeiten zeichnet
    //    die Flaeche (gizmoTreffer) mit der Auswahl
    oeffnen("Skript", false);
    add(tu("F46 B6-1 Skript: g10 waehlen", [] { me::waehle(me::itemNummer("g10"), false); }));
    add(pruefe("F46 B6-1 Skript: g10 (Nr. 10) auf der Seite gross gewaehlt", [] { return me::stand().seite == 0 && auswahlText() == "10"; }));
    add(tu("F46 B6-1 Skript: Bedienen", [] { me::setzeInteraktion(true); }));
    add(pause(0.3));
    add(tu("F46 B6-1 Skript: Klick auf g0 (open klein)", [] { me::klickVorschau(70.0F, 35.0F); }));
    add(pause(0.3));
    add(pruefe("F46 B6-1 Skript: das Skript oeffnet klein, die Seite folgt, die Auswahl ist leer wie beim Reiter", [] {
        diag::detail("Knopftest: F46 B6-1 Skript - Seite " + std::to_string(me::stand().seite) + ", Auswahl " + auswahlText());
        return me::stand().seite == 1 && me::stand().auswahl.empty();
    }));
    add(tu("F46 B6-1 Skript: zurueck auf Bearbeiten", [] { me::setzeInteraktion(false); }));
    add(pause(0.3));
    add(pruefe("F46 B6-1 Skript: Bearbeiten zeichnet die Flaeche, die Auswahl zeigt auf kein fremdes Element", [] {
        const std::size_t n = me::itemMerker().size();
        const std::vector<int> a = me::stand().auswahl;
        return n == 3U && std::all_of(a.begin(), a.end(), [n](int i) { return i >= 0 && static_cast<std::size_t>(i) < n; });
    }));

    // b) Ganze Datei: g2 im Menue gross, dann Strg-Klick auf k1 in der Hierarchie (Seite klein), Entf
    oeffnen("Hierarchie", true);
    add(tu("F46 B6-1 Hierarchie: g2 waehlen", [] { me::waehle(me::itemNummer("g2"), false); }));
    add(pause(0.2));
    add(halteTaste(ImGuiMod_Ctrl, true));
    add(klickAn("F46 B6-1 Hierarchie: Strg-Klick auf k1 (Menue klein)", [] { return zeileNr("k1", 0); }));
    add(halteTaste(ImGuiMod_Ctrl, false));
    add(pause(0.2));
    add(pruefe("F46 B6-1 Hierarchie: Seite klein, gewaehlt ist nur k1", [] {
        diag::detail("Knopftest: F46 B6-1 Hierarchie - Seite " + std::to_string(me::stand().seite) + ", Auswahl " + auswahlText());
        return me::stand().seite == 1 && auswahlText() == "1";
    }));
    add(taste(ImGuiKey_Delete, "Entf"));
    add(pause(0.3));
    add(pruefe("F46 B6-1 Hierarchie: Entf loescht nur k1, gross behaelt seine zwoelf Knoepfe", [namenText] {
        diag::detail("Knopftest: F46 B6-1 Hierarchie - klein: " + namenText() + ", Items gesamt " + std::to_string(me::stand().items));
        return namenText() == "k0,k2" && me::stand().items == 14;
    }));
    add(tu("F46 B6-1: aufraeumen", [ganzeDateiVorher] {
        g_app->settings.menueGanzeDatei = *ganzeDateiVorher;
        me::setzeInteraktion(false);
        me::schliesseAlleOhneSpeichern();
    }));
    add(pause(0.2));
    return s;
}
