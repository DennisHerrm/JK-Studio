// selbsttest_cjkweit.inc - MR4 Punkt 5: CJK-Umbruch ausserhalb des Model-Tabs (karteneditor, BHED_KARTE_NUR=cjkweit;
// Lauf mit SPRACHE=ja bzw. zh-Hans).
//
// ImGui bricht nur an Leerzeichen und nach .,;!?" sowie 、。 um (ImFontCalcWordWrapPositionEx): die erste Zeile eines
// japanischen oder chinesischen Satzes endete am ersten Komma. gui/textumbruch.h bricht zwischen den Zeichen um (JIS X 4051 /
// GB/T 15834). Hier je eine Stelle aus drei Bereichen (Startflaeche des Mission-Tabs: Karten mit AddText, Tutorials-Fenster,
// Helfer "Was willst du zuerst tun?"): jede umbrochene CJK-Zeile (ausser der letzten und vor einem Zeilenwechsel) ist so voll,
// dass die naechste Gruppe nicht mehr hineinpasst (Zeilenbreite + Leerzeichen + Gruppe > Breite; Gruppe = Stueck mit den Zeichen,
// die nach JIS X 4051 nicht am Zeilenanfang stehen und an ihm haengen, bhed::gruppeEnde). Nach der ImGui-Regel gab es dort gar
// keinen Eintrag (keine Stelle ging durch textumbruch).

namespace cjkweit {

// Eintraege seit dem letzten Leeren, deren Fenstername `teil` enthaelt ("" = alle); FEHLER-Text in `fehler`
int auswerten(const std::string& teil, std::string& fehler, int& mehrzeilig) {
    const CjkUmbruchStand& st = cjkUmbruchStand();
    int n = 0;
    mehrzeilig = 0;
    std::set<std::string> gemeldet;   // je Text eine Meldung (er steht in jedem Bild im Protokoll)
    for (const CjkUmbruchStand::Eintrag& e : st.protokoll) {
        if (!teil.empty() && e.fenster.find(teil) == std::string::npos) { continue; }
        ++n;
        if (e.zeilenBreiten.size() >= 2) { ++mehrzeilig; }
        for (std::size_t i = 0; i + 1 < e.zeilenBreiten.size(); ++i) {
            if (e.hart[i]) { continue; }
            if (e.zeilenBreiten[i] + e.naechstes[i] <= e.breite - 0.01F || e.zeilenBreiten[i] > e.breite + 0.01F) {
                char z[700];
                std::snprintf(z, sizeof(z), " [%s Zeile %d: %.0f von %.0f, naechstes %.0f: %s]", e.fenster.c_str(), static_cast<int>(i + 1),
                              static_cast<double>(e.zeilenBreiten[i]), static_cast<double>(e.breite), static_cast<double>(e.naechstes[i]),
                              e.text.substr(0, 400).c_str());
                if (gemeldet.insert(e.text).second) { fehler += z; }
                break;
            }
        }
    }
    return n;
}

Schritt pruefeStelle(const std::string& name, const std::string& teil) {
    return pruefe("CJK weit: " + name + " - umbrochene CJK-Texte fuellen jede Zeile (Umbruch zwischen den Zeichen)", [name, teil] {
        std::string fehler;
        int mehr = 0;
        const int n = auswerten(teil, fehler, mehr);
        std::set<std::string> fenster;
        for (const CjkUmbruchStand::Eintrag& e : cjkUmbruchStand().protokoll) { fenster.insert(e.fenster); }
        std::string f;
        for (const std::string& x : fenster) { f += " " + x; }
        diag::detail("Knopftest: CJK weit - " + name + ": " + std::to_string(n) + " CJK-Texte im Umbruch, " + std::to_string(mehr) +
                     " mehrzeilig, Fenster:" + f + fehler);
        return n > 0 && mehr > 0 && fehler.empty();
    });
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(pruefe("CJK weit: Lauf in Japanisch oder Chinesisch (SPRACHE=ja / zh-Hans)", [] {
        const i18n::Language l = i18n::currentLanguage();
        diag::detail(std::string("Knopftest: CJK weit - Sprache ") + i18n::kurzCode(l));
        return l == i18n::Language::Japanese || l == i18n::Language::ChineseSimplified;
    }));
    // 0. P18 (Codepruefung seit rc610): ohne Messschalter fuellt ein umbrochener CJK-Text keine Messdaten (Bildpfad frei)
    auto p18 = std::make_shared<std::string>();
    add(tu("CJK weit: P18 - ein CJK-Text im Umbruch mit messen = false", [p18] {
        CjkUmbruchStand& st = cjkUmbruchStand();
        const bool vorher = st.messen;
        st.messen = false;
        const int aufrufe = st.aufrufe;
        const std::size_t zeilen = st.zeilenBreiten.size();
        ImGui::SetNextWindowPos(ImVec2{-4000.0F, -4000.0F});
        ImGui::Begin("##cjkweit_p18", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 6.0F);
        cjkTextUnformatted(tr(Str::TourListIntro));
        ImGui::PopTextWrapPos();
        ImGui::End();
        *p18 = "Aufrufe " + std::to_string(aufrufe) + " -> " + std::to_string(st.aufrufe) + ", Zeilen gemessen " + std::to_string(zeilen) + " -> " +
               std::to_string(st.zeilenBreiten.size()) + ", Schalter im Lauf " + (vorher ? "an" : "aus");
        if (st.aufrufe != aufrufe) { *p18 += " GEMESSEN"; }
        st.messen = vorher;
    }));
    add(pruefe("CJK weit: P18 - ohne Messschalter keine Messdaten im Bildpfad, im Selbsttest an (BHED_EDITORTEST)", [p18] {
        diag::detail("Knopftest: CJK weit - P18: " + *p18);
        return p18->find("GEMESSEN") == std::string::npos && p18->find("Schalter im Lauf an") != std::string::npos;
    }));
    // 1. Startflaeche des Mission-Tabs (ohne Karte, Ansicht Vorschau): Karten mit AddText und CalcTextSize
    add(tu("CJK weit: Vorschau ohne Karte, Protokoll an", [] {
        g_app->leftMode = 1;
        cjkUmbruchStand().protokollieren = true;
    }));
    add(warteBis("CJK weit: Startflaeche sichtbar", [] { return hb::fensterRect("##startflaeche").GetWidth() > 0.0F; }, 600));
    add(tu("CJK weit: Protokoll leeren", [] { cjkUmbruchStand().protokoll.clear(); }));
    add(pause(0.5));
    add(foto("cjkw_start"));
    add(pruefeStelle("Startflaeche", "##startflaeche"));
    // 2. Tutorials-Fenster (Liste mit Einleitung und Beschreibungen)
    add(tu("CJK weit: Skript-Ansicht, Tutorials-Fenster auf", [] {
        g_app->leftMode = 0;
        tourFensterOeffnen();
    }));
    add(pause(0.8));
    add(tu("CJK weit: Protokoll leeren", [] { cjkUmbruchStand().protokoll.clear(); }));
    add(pause(0.5));
    add(foto("cjkw_touren"));
    add(pruefeStelle("Tutorials", ""));
    add(tu("CJK weit: Tutorials zu, Helfer mit \"Was willst du zuerst tun?\"", [] {
        tourSchliessen();
        hilfeOeffnen();
        hilfeErsteSchritte(true);
    }));
    add(pause(0.8));
    add(tu("CJK weit: Protokoll leeren", [] { cjkUmbruchStand().protokoll.clear(); }));
    add(pause(0.5));
    add(foto("cjkw_helfer"));
    add(pruefeStelle("Helfer", ""));
    // MR6 P28: ausserhalb des Selbsttests (messen aus) merkt sich der Umbruch sein Ergebnis - der stehende Helfer bricht seine
    // CJK-Texte einmal um, nicht in jedem Bild (vorher je Bild und Text umbruchZeilen mit CalcTextSizeA je Stueck)
    auto p28 = std::make_shared<std::array<long long, 2>>();
    add(tu("CJK weit: P28 - messen aus, Zaehler merken", [p28] {
        CjkUmbruchStand& st = cjkUmbruchStand();
        st.messen = false;
        *p28 = {st.umbruchAufrufe, st.umbruchGerechnet};
    }));
    add(pause(0.6));
    add(pruefe("CJK weit: P28 - der stehende Helfer bricht seine Texte nicht in jedem Bild neu um", [p28] {
        CjkUmbruchStand& st = cjkUmbruchStand();
        const long long aufrufe = st.umbruchAufrufe - (*p28)[0];
        const long long gerechnet = st.umbruchGerechnet - (*p28)[1];
        st.messen = true;
        diag::detail("Knopftest: CJK weit - P28: " + std::to_string(aufrufe) + " Aufrufe in 0,6 s, davon umbrochen " + std::to_string(gerechnet));
        return aufrufe > 0 && gerechnet * 10 <= aufrufe;
    }));
    // MR7 P41: nach einem Neubau der Schriften (buildFonts, z. B. OpenGL Font... im Karten-Editor) sind alle ImFont neu - ein
    // Ergebnis, das der Speicher unter der alten Schrift abgelegt hat, darf nicht mehr gelten, auch wenn die neue Schrift an
    // derselben Adresse liegt. Je Runde: umbrechen, Schriften neu bauen, wieder umbrechen - das zweite Mal muss neu umbrochen werden.
    struct P41 {
        long long gerechnet = 0;
        const void* schrift = nullptr;
        int runden = 0;
        int gleicheAdresse = 0;
        int alt = 0;
        std::size_t ueberlebt = 0;   // gemerkte Ergebnisse des Testtexts unter der alten Schrift, direkt nach dem Neubau
    };
    auto p41 = std::make_shared<P41>();
    const auto p41Umbrechen = [] {
        CjkUmbruchStand& st = cjkUmbruchStand();
        const bool vorher = st.messen;
        st.messen = false;
        ImGui::SetNextWindowPos(ImVec2{-4000.0F, -4000.0F});
        ImGui::Begin("##cjkweit_p41", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing);
        const void* schrift = ImGui::GetFont();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 6.0F);
        cjkTextUnformatted(tr(Str::TourListIntro));
        ImGui::PopTextWrapPos();
        ImGui::End();
        st.messen = vorher;
        return schrift;
    };
    for (int runde = 0; runde < 12; ++runde) {
        add(tu("CJK weit: P41 - Runde " + std::to_string(runde) + ": umbrechen", [p41, p41Umbrechen] {
            p41->schrift = p41Umbrechen();
            p41->gerechnet = cjkUmbruchStand().umbruchGerechnet;
        }));
        add(tu("CJK weit: P41 - Schriften neu bauen", [] { schriftenNeuBauen(); }));
        add(pause(0.2));
        add(tu("CJK weit: P41 - nach dem Neubau umbrechen", [p41, p41Umbrechen] {
            p41->ueberlebt += cjkUmbruchGemerkt(tr(Str::TourListIntro));
            const void* jetzt = p41Umbrechen();
            ++p41->runden;
            p41->gleicheAdresse += jetzt == p41->schrift ? 1 : 0;
            p41->alt += cjkUmbruchStand().umbruchGerechnet == p41->gerechnet ? 1 : 0;
        }));
    }
    add(pruefe("CJK weit: P41 - nach jedem Neubau der Schriften wird neu umbrochen (kein Ergebnis der alten Schrift)", [p41] {
        diag::detail("Knopftest: CJK weit - P41: " + std::to_string(p41->runden) + " Neubauten, Schrift an derselben Adresse " +
                     std::to_string(p41->gleicheAdresse) + ", Ergebnis der alten Schrift genommen " + std::to_string(p41->alt) +
                     ", nach dem Neubau noch gemerkt " + std::to_string(p41->ueberlebt));
        return p41->runden == 12 && p41->alt == 0 && p41->ueberlebt == 0;
    }));
    add(tu("CJK weit: aufraeumen", [] {
        hilfeErsteSchritte(false);
        hilfeSchliessen();
        cjkUmbruchStand().protokollieren = false;
        cjkUmbruchStand().protokoll.clear();
    }));
    add(pause(0.3));
    return s;
}

}  // namespace cjkweit
