// selbsttest_karte8.inc - Stufe 8a des Karten-Editors: Kompilier-Fortschritt,
// BSP-Import ohne Datenverlust und Teil-Kompilierungen.
//
// Eingebunden in gui/selbsttest.cpp (namenloser Namensraum, nach Stufe 7).
// BHED_KARTE_NUR=stufe8 laesst nur diese Schritte laufen; BHED_STUFE8_NUR=<Teil>
// (fortschritt, ...) nur einen Teil.
namespace stufe8 {

namespace ke = karteneditor;
using stufe3::abgebrochenZeile;
using stufe3::bauMenuepunkt;
using stufe3::konsoleText;
using stufe3::q3map2Bereitstellen;
using stufe3::raumKarteOeffnen;
using stufe3::zaehle;

// Warten mit einer Zeitgrenze in Sekunden (ein Bau dauert laenger als 4000 Bilder)
Schritt warteSek(const std::string& was, std::function<bool()> bedingung, double sekunden) {
    auto t0 = std::make_shared<std::chrono::steady_clock::time_point>();
    Schritt x{"Warte: " + was, [=](int b) {
                  if (b == 0) { *t0 = std::chrono::steady_clock::now(); }
                  const double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - *t0).count();
                  if (bedingung()) {
                      char z[64];
                      std::snprintf(z, sizeof(z), " nach %.2f s", t);
                      diag::detail("Knopftest: " + was + z);
                      return true;
                  }
                  if (t > sekunden) {
                      meldeFehler(was + ": nicht nach " + std::to_string(static_cast<int>(sekunden)) + " s");
                      return true;
                  }
                  return false;
              }};
    x.eigeneZeitgrenze = true;
    return x;
}

std::string bauZeitenDatei() { return platform::settingsDirectory() + "\\karte_bauzeiten.json"; }

bool fensterSichtbar(const char* teil) {
    for (ImGuiWindow* w : GImGui->Windows) {
        if (w->WasActive && !w->Hidden && std::strstr(w->Name, teil) != nullptr) { return true; }
    }
    return false;
}

// Der Anfang einer Vorlage bis zum ersten Platzhalter (fuer die Suche in der Konsole)
std::string vorlageAnfang(Str id) {
    const std::string v = ke::trK(id);
    return v.substr(0, v.find('%'));
}

// --- Punkt 1: Fortschritt mit Restzeit ----------------------------------------------
//
// Wirkung: waehrend des Baus zeigt das Fenster "Build progress" Schritt, Stufe,
// Phase von q3map2 mit Pacifier bzw. Bounce, einen Gesamtbalken und die Restzeit
// (erster Bau: geschaetzt aus der Phase; zweiter Bau: aus dem letzten Bau, mit
// Rueckblick der Schaetzung gegen die echte Dauer); am Ende die Zeit je Schritt
// gegen den letzten Bau in der Konsole; die Bauzeiten bleiben je Karte und
// Eintrag gespeichert; Abbrechen im Fenster beendet q3map2; "Melden" ruft im
// Selbsttest nie Windows auf (es wird nur gezaehlt).
std::vector<Schritt> fortschritt() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const char* kEintrag = "Test Fortschritt";
    add(tu("Stufe 8/1: bhed_q3map2, Build-Menue frisch, Testkarte raum8, keine Bauzeiten, Eintraege Test Fortschritt / Test langsam", [kEintrag] {
        karteDialogeZu();
        q3map2Bereitstellen();
        ke::Editor& e = ke::editor();
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(ke::bauMenueDateiPfad()), ec);
        std::filesystem::remove(std::filesystem::path(bauZeitenDatei()), ec);
        g_app->settings.karte.erase("BuildMenu");
        e.bauProjektFuer = -2;
        e.bauGeaendert = false;
        ke::bauMenueSicherstellen();
        e.bau = ke::Editor::BauEinstellungen{};
        ke::einstellungenSpeichern();
        raumKarteOeffnen("raum8", false);
        ke::konsoleLeeren();
        e.bauMeldungenTest = 0;
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{kEintrag,
                                                           {"[q3map2] -meta \"[MapFile]\"", "[q3map2] -vis -saveprt \"[MapFile]\"",
                                                            "[q3map2] -light -super 4 -samples 4 -filter -bounce 16 \"[MapFile]\""}});
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{"Test langsam", {"[q3map2] -light -super 8 -samples 16 -bounce 100 \"[MapFile]\""}});
    }));
    add(bauMenuepunkt(kEintrag));
    add(warteSek("erster Bau: Schritt 3 (LIGHT) mit einer Phase von q3map2", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return st.laeuft && st.schritt == 2 && st.stufe == "LIGHT" && !st.phase.empty();
    }, 60));
    add(pause(0.6));
    add(pruefe("erster Bau: Fenster \"Build progress\" offen, Stufe LIGHT, keine Restzeit aus einem Verlauf", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        diag::detail("Knopftest: Stufe 8/1 erster Bau - Phase " + st.phase + ", Pacifier " + std::to_string(st.pacifier) + ", Bounce " +
                     std::to_string(st.bounce) + "/" + std::to_string(st.bounceVon) + ", gesamt " + std::to_string(st.schaetzung.gesamtAnteil) +
                     ", Text " + ke::editor().bauFortschrittText);
        return fensterSichtbar("###kartebaufortschritt") && st.stufe == "LIGHT" && !st.schaetzung.ausVerlauf && st.schaetzung.gesamtAnteil >= 2.0 / 3.0 - 1e-6;
    }));
    add(foto("karteneditor_s8_fortschritt_erster"));
    add(warteSek("erster Bau fertig", [] { return !ke::bauLaeuft(); }, 180));
    add(pruefe("erster Bau: Zeit je Schritt in der Konsole (BSP, VIS, LIGHT) ohne Vergleich, Bauzeiten gespeichert, gemeldet (nur gezaehlt), Fenster zu", [kEintrag] {
        const std::string k = konsoleText();
        const std::string kopf = ke::trK(Str::KeFsZeitenKopf);
        const std::string vgl = vorlageAnfang(Str::KeFsZeitVergleich);
        const std::string datei = dateiInhalt(bauZeitenDatei());
        diag::detail("Knopftest: Stufe 8/1 Konsole: " + k.substr(k.find(kopf) == std::string::npos ? 0 : k.find(kopf), 200));
        return k.find(kopf) != std::string::npos && k.find(" 1 BSP: ") != std::string::npos && k.find(" 2 VIS: ") != std::string::npos &&
               k.find(" 3 LIGHT: ") != std::string::npos && k.find(vgl) == std::string::npos && datei.find(std::string("\\t") + kEintrag) != std::string::npos &&
               ke::editor().bauMeldungenTest == 1 && !ke::bauFortschrittStand().laeuft && !fensterSichtbar("###kartebaufortschritt");
    }));
    // Zweiter Bau: die Restzeit aus dem ersten
    auto schaetzung = std::make_shared<std::array<double, 3>>();   // geschaetzte Gesamtdauer, Zeitpunkt, echte Dauer
    auto start = std::make_shared<std::chrono::steady_clock::time_point>();
    add(tu("Konsole leeren, zweiter Bau", [start] {
        ke::konsoleLeeren();
        *start = std::chrono::steady_clock::now();
    }));
    add(bauMenuepunkt(kEintrag));
    add(warteSek("zweiter Bau: Schritt 3 seit gut einer Sekunde", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return st.laeuft && st.schritt == 2 && st.phase.find("Radiosity") != std::string::npos && st.bounce >= 2;
    }, 60));
    add(pruefe("zweiter Bau: Restzeit aus dem letzten Bau dieser Karte", [schaetzung] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        (*schaetzung)[0] = st.vergangen + st.schaetzung.rest;
        (*schaetzung)[1] = st.vergangen;
        diag::detail("Knopftest: Stufe 8/1 zweiter Bau - nach " + std::to_string(st.vergangen) + " s Rest " + std::to_string(st.schaetzung.rest) +
                     " s, gesamt " + std::to_string(st.schaetzung.gesamtAnteil) + ", Text " + ke::editor().bauFortschrittText);
        return st.schaetzung.ausVerlauf && st.schaetzung.rest > 0.0 && st.schaetzung.gesamtAnteil > 0.0 && st.schaetzung.gesamtAnteil < 1.0;
    }));
    add(foto("karteneditor_s8_fortschritt_restzeit"));
    add(warteSek("zweiter Bau fertig", [] { return !ke::bauLaeuft(); }, 180));
    add(pruefe("zweiter Bau: Zeit je Schritt gegen den letzten Bau (3 Vergleiche); Schaetzung gegen die echte Dauer", [schaetzung, start] {
        const std::string k = konsoleText();
        const double echt = std::chrono::duration<double>(std::chrono::steady_clock::now() - *start).count();
        (*schaetzung)[2] = echt;
        char z[200];
        std::snprintf(z, sizeof(z), "Knopftest: Stufe 8/1 Schaetzung nach %.1f s: %.1f s, echt %.1f s (%+.0f %%)", (*schaetzung)[1], (*schaetzung)[0], echt,
                      ((*schaetzung)[0] - echt) / echt * 100.0);
        diag::detail(z);
        return zaehle(k, vorlageAnfang(Str::KeFsZeitVergleich)) == 3 && std::fabs((*schaetzung)[0] - echt) / echt < 0.5 && ke::editor().bauMeldungenTest == 2;
    }));
    // Ein langer Lichtlauf: Bounce x von 100, dann Abbrechen im Fenster
    add(tu("Konsole leeren", [] { ke::konsoleLeeren(); }));
    add(bauMenuepunkt("Test langsam"));
    add(warteSek("Test langsam: Bounce 1 von 100 mit Pacifier", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return st.laeuft && st.bounce >= 1 && st.bounceVon == 100 && st.pacifier >= 0.0;
    }, 120));
    add(pause(0.3));
    add(foto("karteneditor_s8_fortschritt_bounce"));
    add(klick(ke::trK(Str::KeBmAbbrechen), "kartebaufortschritt"));
    add(pause(0.3));
    add(pruefe("Abbrechen im Fortschrittsfenster: q3map2 beendet, Zeile in der Konsole, Fenster zu, nichts gemeldet", [] {
        return !ke::bauLaeuft() && konsoleText().find(abgebrochenZeile("Test langsam", 1, 1)) != std::string::npos &&
               !fensterSichtbar("###kartebaufortschritt") && ke::editor().bauMeldungenTest == 2;
    }));
    // Menuepunkt "Melden, wenn der Bau fertig ist": Haken umschalten, gespeichert
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.25));
    add(klick(ke::trK(Str::KeFsMelden), "##Menu_", 0, false));
    add(pause(0.2));
    add(pruefe("Build > Melden: aus, als BhedBuildNotify 0 gespeichert", [] {
        return !ke::editor().bau.melden && g_app->settings.karte["BhedBuildNotify"] == "0";
    }));
    add(tu("Melden wieder an, Testeintraege weg", [] {
        ke::Editor& e = ke::editor();
        e.bau.melden = true;
        ke::einstellungenSpeichern();
        auto& v = e.bauProjekt.eintraege;
        v.erase(std::remove_if(v.begin(), v.end(), [](const karte::BauEintrag& b) { return b.name == "Test Fortschritt" || b.name == "Test langsam"; }),
                v.end());
        ke::konsoleLeeren();
    }));
    return s;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    const char* nur = std::getenv("BHED_STUFE8_NUR");
    const auto teil = [nur](const char* t) { return nur == nullptr || std::strcmp(nur, t) == 0; };
    const auto dazu = [&s](std::vector<Schritt> x) {
        for (Schritt& y : x) { s.push_back(std::move(y)); }
    };
    if (teil("fortschritt")) { dazu(fortschritt()); }
    return s;
}

}  // namespace stufe8
