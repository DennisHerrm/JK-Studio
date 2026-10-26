// selbsttest_karte8.inc - Stufe 8a des Karten-Editors: Kompilier-Fortschritt,
// BSP-Import ohne Datenverlust und Teil-Kompilierungen.
//
// Eingebunden in gui/selbsttest.cpp (namenloser Namensraum, nach Stufe 7).
// BHED_KARTE_NUR=stufe8 laesst nur diese Schritte laufen; BHED_STUFE8_NUR=<Teil>
// (fortschritt, ...) nur einen Teil.
namespace stufe8 {

namespace ke = karteneditor;
using stufe3::abgebrochenZeile;
using stufe3::bspLumps;
using stufe3::istDatei;
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


// --- Punkt 2: Editor-Daten behalten ----------------------------------------------
//
// Wirkung: dieselbe Karte ohne und mit der Option gebaut - Lumps 1..17 Byte fuer
// Byte gleich, nur der Entity-Lump hat mit der Option alle drei Lichter (sonst nur
// das schaltbare); daneben <karte>.bhed. Open .bsp bringt mit der Begleitdatei die
// Quelle zurueck (Lichter, misc_model mit seinen Schluesseln, func_group mit seinen
// zwei Pinseln, gleiche Pinselzahl, gespeichert Byte fuer Byte wie die Quelle);
// ohne Begleitdatei kommen nur die Lichter zurueck (-keeplights).
std::string edKarte() { return karteTestPfad("s8ed\\ed.map"); }
std::string edStamm() { return karteTestPfad("s8ed\\ed"); }

void edKarteSchreiben() {
    karte::Karte k;
    k.klassen = &ke::editor().spiel->klassen;
    auto welt = k.weltHolenOderAnlegen();
    karte::TexturProjektion proj;
    karte::texdefStandard(karte::TexdefArt::Quake, proj);
    const auto quader = [&](const std::shared_ptr<karte::Entitaet>& en, const karte::Vektor3f& mitte, const karte::Vektor3f& halb) {
        auto p = std::make_shared<karte::Pinsel>();
        p->quaderBauen(karte::Kasten(mitte, halb), "textures/yavin/temple_stone2", proj, k.kontext);
        en->kindEinfuegen(p);
    };
    quader(welt, karte::Vektor3f(0, 0, -8), karte::Vektor3f(512, 512, 8));
    quader(welt, karte::Vektor3f(0, 0, 520), karte::Vektor3f(512, 512, 8));
    quader(welt, karte::Vektor3f(-520, 0, 256), karte::Vektor3f(8, 528, 272));
    quader(welt, karte::Vektor3f(520, 0, 256), karte::Vektor3f(8, 528, 272));
    quader(welt, karte::Vektor3f(0, -520, 256), karte::Vektor3f(512, 8, 272));
    quader(welt, karte::Vektor3f(0, 520, 256), karte::Vektor3f(512, 8, 272));
    const auto punkt = [&](const char* klasse, std::vector<std::pair<const char*, const char*>> kv) {
        auto en = k.neueEntitaet(klasse, false);
        for (const auto& [a, b] : kv) { en->setzen(a, b); }
        k.entitaetEinfuegen(en);
    };
    punkt("light", {{"origin", "0 0 96"}, {"light", "300"}});
    punkt("light", {{"origin", "256 0 96"}, {"light", "200"}, {"_color", "1 0.5 0.25"}});
    punkt("light", {{"origin", "-256 0 96"}, {"light", "250"}, {"targetname", "schalter"}});
    punkt("misc_model", {{"origin", "128 128 0"}, {"model", "models/map_objects/bounty/pillar.md3"}, {"angle", "45"}, {"modelscale", "1.5"}});
    punkt("info_player_start", {{"origin", "-128 -128 24"}});
    auto gruppe = k.neueEntitaet("func_group", true);
    gruppe->setzen("_bhed_notiz", "zwei Kisten");
    quader(gruppe, karte::Vektor3f(-200, 200, 32), karte::Vektor3f(32, 32, 32));
    quader(gruppe, karte::Vektor3f(-120, 200, 32), karte::Vektor3f(32, 32, 32));
    k.entitaetEinfuegen(gruppe);
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(edKarte()).parent_path(), ec);
    (void)karte::karteSpeichern(edKarte(), k, false);
}

struct EdZahl {
    int lichter = 0;
    int schaltbar = 0;
    int modelle = 0;
    int gruppen = 0;
    int gruppenPinsel = 0;
    int pinsel = 0;
    std::string modellSchluessel;
};
EdZahl edZaehlen() {
    EdZahl z;
    for (const auto& en : ke::editor().karte.entitaeten()) {
        const std::string c = en->klassenname();
        if (c == "light") {
            ++z.lichter;
            if (!en->wert("targetname").empty()) { ++z.schaltbar; }
        }
        if (c == "misc_model") {
            ++z.modelle;
            z.modellSchluessel = en->wert("model") + "|" + en->wert("angle") + "|" + en->wert("modelscale") + "|" + en->wert("origin");
        }
        if (c == "func_group") {
            ++z.gruppen;
            z.gruppenPinsel += static_cast<int>(en->kinder().size());
        }
        for (const auto& p : en->kinder()) { z.pinsel += p->art() == karte::Primitiv::Art::Pinsel ? 1 : 0; }
    }
    return z;
}

std::string edZahlText(const EdZahl& z) {
    return "Lichter " + std::to_string(z.lichter) + " (schaltbar " + std::to_string(z.schaltbar) + "), misc_model " + std::to_string(z.modelle) + " [" +
           z.modellSchluessel + "], func_group " + std::to_string(z.gruppen) + " mit " + std::to_string(z.gruppenPinsel) + " Pinseln, Pinsel " +
           std::to_string(z.pinsel);
}

int lichterImLump(const std::string& lump) { return static_cast<int>(zaehle(lump, "\"classname\" \"light\"")); }

std::vector<Schritt> editordaten() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const char* kEintrag = "Test Editordaten";
    auto quelle = std::make_shared<std::string>();
    add(tu("Stufe 8/2: Testkarte mit 3 Lichtern (1 schaltbar), misc_model, func_group (2 Pinsel); Option aus; Eintrag Test Editordaten", [kEintrag, quelle] {
        karteDialogeZu();
        q3map2Bereitstellen();
        ke::bauMenueSicherstellen();   // sonst laedt bauStarten das Menue neu und der Eintrag ist weg
        ke::Editor& e = ke::editor();
        e.bau.editorDaten = false;
        ke::einstellungenSpeichern();
        edKarteSchreiben();
        (void)ke::karteOeffnen(edKarte());
        *quelle = dateiInhalt(edKarte());
        std::error_code ec;
        for (const char* x : {".bsp", ".prt", ".srf", ".bhed", "_ohne.bsp"}) { std::filesystem::remove(std::filesystem::path(edStamm() + x), ec); }
        ke::konsoleLeeren();
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{
            kEintrag, {"[q3map2] -meta \"[MapFile]\"", "[q3map2] -vis -threads 1 \"[MapFile]\"", "[q3map2] -light -fast -samples 2 -bounce 2 \"[MapFile]\""}});
    }));
    add(bauMenuepunkt(kEintrag));
    add(warteSek("Bau ohne Option fertig", [] { return !ke::bauLaeuft(); }, 300));
    add(pruefe("ohne Option: .bsp ohne .bhed, keine -keeplights, im Entity-Lump nur das schaltbare Licht", [] {
        const std::vector<std::string> l = bspLumps(edStamm() + ".bsp");
        std::error_code ec;
        std::filesystem::copy_file(std::filesystem::path(edStamm() + ".bsp"), std::filesystem::path(edStamm() + "_ohne.bsp"),
                                   std::filesystem::copy_options::overwrite_existing, ec);
        const std::string k = konsoleText();
        return l.size() == 18 && lichterImLump(l[0]) == 1 && !istDatei(edStamm() + ".bhed") && k.find("-keeplights") == std::string::npos;
    }));
    // Die Option ueber das Build-Menue
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.25));
    add(klick(ke::trK(Str::KeEdMenue), "##Menu_", 0, false));
    add(pause(0.2));
    add(pruefe("Build > Editor-Daten behalten: an, als BhedKeepEditorData 1 gespeichert", [] {
        return ke::editor().bau.editorDaten && g_app->settings.karte["BhedKeepEditorData"] == "1";
    }));
    add(tu("Konsole leeren, mit Option bauen", [] { ke::konsoleLeeren(); }));
    add(bauMenuepunkt(kEintrag));
    add(warteSek("Bau mit Option fertig", [] { return !ke::bauLaeuft(); }, 300));
    add(pruefe("mit Option: -keeplights nur fuer -meta, <karte>.bhed geschrieben (Kopf + die Quelle Byte fuer Byte)", [quelle] {
        const std::string k = konsoleText();
        char z[256];
        std::snprintf(z, sizeof(z), ke::trK(Str::KeEdLichter), 1);
        const std::string b = dateiInhalt(edStamm() + ".bhed");
        return k.find(z) != std::string::npos && zaehle(k, " -meta -keeplights \"") == 1 && k.find("-vis -threads 1 -keeplights") == std::string::npos &&
               k.find("-bounce 2 -keeplights") == std::string::npos && k.find(vorlageAnfang(Str::KeEdGeschrieben)) != std::string::npos && !quelle->empty() && b.size() > quelle->size() &&
               b.compare(b.size() - quelle->size(), quelle->size(), *quelle) == 0;
    }));
    add(pruefe("Byte-Vergleich: Lumps 1..17 (Geometrie, Lightmaps, Lichtgitter, Sicht) gleich, nur der Entity-Lump anders - mit allen 3 Lichtern", [] {
        const std::vector<std::string> a = bspLumps(edStamm() + "_ohne.bsp");
        const std::vector<std::string> b = bspLumps(edStamm() + ".bsp");
        if (a.size() != 18 || b.size() != 18) { return false; }
        int anders = 0;
        std::string welche;
        for (std::size_t i = 1; i < 18; ++i) {
            if (a[i] != b[i]) {
                ++anders;
                welche += " " + std::to_string(i);
            }
        }
        diag::detail("Knopftest: Stufe 8/2 Entity-Lump ohne " + std::to_string(a[0].size()) + " Byte, mit " + std::to_string(b[0].size()) +
                     " Byte; Lichter " + std::to_string(lichterImLump(a[0])) + " -> " + std::to_string(lichterImLump(b[0])) + "; andere Lumps:" + welche);
        return anders == 0 && a[0] != b[0] && lichterImLump(b[0]) == 3 && b[14].size() > 0;
    }));
    add(tu("File > Open .bsp (mit ed.bhed daneben)", [] {
        ke::konsoleLeeren();
        (void)ke::bspOeffnenDatei(edStamm() + ".bsp");
    }));
    add(pause(0.5));
    add(pruefe("Open .bsp mit Begleitdatei: genaue Quelle - 3 Lichter, misc_model mit Schluesseln, func_group mit 2 Pinseln, 8 Pinsel", [quelle] {
        const EdZahl z = edZaehlen();
        diag::detail("Knopftest: Stufe 8/2 mit .bhed: " + edZahlText(z));
        const std::string k = konsoleText();
        return k.find(vorlageAnfang(Str::KeEdGefunden)) != std::string::npos && z.lichter == 3 && z.schaltbar == 1 && z.modelle == 1 &&
               z.modellSchluessel == "models/map_objects/bounty/pillar.md3|45|1.5|128 128 0" && z.gruppen == 1 && z.gruppenPinsel == 2 && z.pinsel == 8;
    }));
    add(pruefe("die geoeffnete Karte gespeichert: Byte fuer Byte die Quelle", [quelle] {
        const std::string p = karteTestPfad("s8ed\\zurueck.map");
        (void)ke::karteSpeichernUnter(p);
        return dateiInhalt(p) == *quelle;
    }));
    add(tu("Begleitdatei weg, Open .bsp noch einmal", [] {
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(edStamm() + ".bhed"), ec);
        ke::konsoleLeeren();
        (void)ke::bspOeffnenDatei(edStamm() + ".bsp");
    }));
    add(pause(0.5));
    add(pruefe("Open .bsp ohne Begleitdatei: die 3 Lichter kommen aus dem Entity-Lump zurueck (misc_model und func_group nicht - eingebacken)", [] {
        const EdZahl z = edZaehlen();
        diag::detail("Knopftest: Stufe 8/2 ohne .bhed: " + edZahlText(z));
        return konsoleText().find(vorlageAnfang(Str::KeEdGefunden)) == std::string::npos && z.lichter == 3 && z.modelle == 0 && z.gruppen == 0;
    }));
    add(tu("Option aus, Eintrag weg", [] {
        ke::Editor& e = ke::editor();
        e.bau.editorDaten = false;
        ke::einstellungenSpeichern();
        auto& v = e.bauProjekt.eintraege;
        v.erase(std::remove_if(v.begin(), v.end(), [](const karte::BauEintrag& b) { return b.name == "Test Editordaten"; }), v.end());
    }));
    return s;
}


// --- Punkt 3: misc_model aus der Geometrie zurueckgewinnen ------------------------------
//
// Wirkung: die Testkarte aus Punkt 2 (misc_model pillar.md3 bei 128 128 0, angle 45,
// modelscale 1.5) ohne Editor-Daten bauen und als .bsp oeffnen - im Hintergrund kommt
// das misc_model mit denselben Schluesseln zurueck (ein Rueckgaengig-Schritt), "Update
// entities only" bleibt moeglich; ohne die Option kommt keins.
std::vector<Schritt> modelle() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Stufe 8/3: Testkarte (misc_model pillar.md3, angle 45, modelscale 1.5) ohne Editor-Daten bauen", [] {
        karteDialogeZu();
        q3map2Bereitstellen();
        ke::bauMenueSicherstellen();
        ke::Editor& e = ke::editor();
        e.bau.editorDaten = false;
        e.bspRueck = ke::Editor::BspRueckEinstellungen{};
        ke::einstellungenSpeichern();
        edKarteSchreiben();
        (void)ke::karteOeffnen(edKarte());
        std::error_code ec;
        for (const char* x : {".bsp", ".prt", ".srf", ".bhed"}) { std::filesystem::remove(std::filesystem::path(edStamm() + x), ec); }
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{"Test Modelle", {"[q3map2] -meta \"[MapFile]\"", "[q3map2] -vis -fast \"[MapFile]\"", "[q3map2] -light -fast \"[MapFile]\""}});
        ke::konsoleLeeren();
    }));
    add(bauMenuepunkt("Test Modelle"));
    add(warteSek("Bau fertig", [] { return !ke::bauLaeuft(); }, 300));
    add(tu("File > Open .bsp (ohne .bhed): Dekompilat, dann die Rueckgewinnung im Hintergrund", [] {
        ke::konsoleLeeren();
        ke::editor().bspRueckLetzt = ke::Editor::BspRueckLetzt{};
        (void)ke::bspOeffnenDatei(edStamm() + ".bsp");
    }));
    add(warteSek("Rueckgewinnung laeuft (Fenster)", [] { return ke::bspRueckStand().stand == 1 || ke::bspRueckStand().stand == 2 || ke::editor().bspRueckLetzt.sekunden > 0; }, 30));
    add(foto("karteneditor_s8_modelle_laeuft"));
    add(warteSek("Rueckgewinnung fertig", [] { return ke::bspRueckStand().stand == 0 && ke::editor().bspRueckLetzt.sekunden > 0; }, 180));
    add(pruefe("misc_model zurueck: pillar.md3 mit origin 128 128 0, angle 45, modelscale 1.5; Konsole; Update entities only moeglich", [] {
        const EdZahl z = edZaehlen();
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Stufe 8/3 " + edZahlText(z) + ", Treffer " + std::to_string(e.bspRueckLetzt.treffer) + ", Fehler " +
                     std::to_string(e.bspRueckLetzt.maxFehler) + ", " + std::to_string(e.bspRueckLetzt.sekunden) + " s");
        return z.modelle == 1 && z.modellSchluessel == "models/map_objects/bounty/pillar.md3|45|1.5|128 128 0" && e.bspRueckLetzt.treffer == 1 &&
               e.bspRueckLetzt.maxFehler < 0.01 && konsoleText().find(vorlageAnfang(Str::KeRkFertig)) != std::string::npos && ke::bspNurEntitaetenMoeglich() &&
               !ke::bspGeometrieGeaendert();
    }));
    add(foto("karteneditor_s8_modelle"));
    add(tu("Rueckgaengig", [] { (void)ke::editor().karte.undo.undo(); }));
    add(pruefe("Rueckgaengig nimmt das misc_model wieder weg (ein Schritt)", [] { return edZaehlen().modelle == 0; }));
    add(tu("Option aus, noch einmal oeffnen", [] {
        ke::editor().bspRueck.modelle = false;
        ke::editor().bspRueckLetzt = ke::Editor::BspRueckLetzt{};
        (void)ke::bspOeffnenDatei(edStamm() + ".bsp");
    }));
    add(pause(1.0));
    add(pruefe("ohne die Option: keine Rueckgewinnung, kein misc_model", [] {
        return ke::bspRueckStand().stand == 0 && ke::editor().bspRueckLetzt.sekunden == 0 && edZaehlen().modelle == 0;
    }));
    add(tu("Option wieder an, Eintrag weg", [] {
        ke::Editor& e = ke::editor();
        e.bspRueck = ke::Editor::BspRueckEinstellungen{};
        ke::einstellungenSpeichern();
        auto& v = e.bauProjekt.eintraege;
        v.erase(std::remove_if(v.begin(), v.end(), [](const karte::BauEintrag& b) { return b.name == "Test Modelle"; }), v.end());
    }));
    return s;
}


// --- Punkt 4: Lichter schaetzen (experimentell) ------------------------------------------
//
// Wirkung: die Testkarte aus Punkt 2 (Lichter 300 bei 0 0 96, 200 bei 256 0 96, das
// schaltbare 250 bei -256 0 96) ohne Editor-Daten gebaut und mit "Lichter schaetzen"
// geoeffnet: in der Ebene "Geschaetzte Lichter" liegen Lichter mit _bhed_geschaetzt 1
// nahe an den zwei entfernten; das schaltbare kommt aus dem Entity-Lump (keine Doppelung).
std::vector<Schritt> lichter() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Stufe 8/4: Testkarte ohne Editor-Daten bauen", [] {
        karteDialogeZu();
        q3map2Bereitstellen();
        ke::bauMenueSicherstellen();
        ke::Editor& e = ke::editor();
        e.bau.editorDaten = false;
        e.bspRueck = ke::Editor::BspRueckEinstellungen{};
        e.bspRueck.modelle = false;
        e.bspRueck.lichter = true;
        ke::einstellungenSpeichern();
        edKarteSchreiben();
        (void)ke::karteOeffnen(edKarte());
        std::error_code ec;
        for (const char* x : {".bsp", ".prt", ".srf", ".bhed"}) { std::filesystem::remove(std::filesystem::path(edStamm() + x), ec); }
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{"Test Lichter", {"[q3map2] -meta \"[MapFile]\"", "[q3map2] -vis -fast \"[MapFile]\"", "[q3map2] -light -fast \"[MapFile]\""}});
    }));
    add(bauMenuepunkt("Test Lichter"));
    add(warteSek("Bau fertig", [] { return !ke::bauLaeuft(); }, 300));
    add(tu("File > Open .bsp mit \"Lichter schaetzen\"", [] {
        ke::konsoleLeeren();
        ke::editor().bspRueckLetzt = ke::Editor::BspRueckLetzt{};
        (void)ke::bspOeffnenDatei(edStamm() + ".bsp");
    }));
    add(warteSek("Schaetzung fertig", [] { return ke::bspRueckStand().stand == 0 && konsoleText().find(vorlageAnfang(Str::KeLsFertig)) != std::string::npos; }, 120));
    add(pruefe("geschaetzte Lichter: Ebene \"Geschaetzte Lichter\", _bhed_geschaetzt 1, eines nahe an 0 0 96; das schaltbare nicht doppelt", [] {
        const ke::Editor& e = ke::editor();
        int ebene = -1;
        for (std::size_t i = 0; i < e.karte.schichten.size(); ++i) {
            if (e.karte.schichten[i].name == ke::trK(Str::KeLsEbene)) { ebene = static_cast<int>(i); }
        }
        const double ziel[3][3] = {{0, 0, 96}, {256, 0, 96}, {-256, 0, 96}};
        double naechst[3] = {1e9, 1e9, 1e9};
        int geschaetzt = 0;
        int inEbene = 0;
        std::string text;
        for (const auto& en : e.karte.entitaeten()) {
            if (en->klassenname() != "light" || en->wert("_bhed_geschaetzt") != "1") { continue; }
            ++geschaetzt;
            inEbene += en->schicht == ebene ? 1 : 0;
            double p[3] = {0, 0, 0};
            (void)std::sscanf(en->wert("origin").c_str(), "%lf %lf %lf", &p[0], &p[1], &p[2]);
            text += " [" + en->wert("origin") + " light " + en->wert("light") + "]";
            for (int z = 0; z < 3; ++z) {
                naechst[z] = std::min(naechst[z], std::sqrt((p[0] - ziel[z][0]) * (p[0] - ziel[z][0]) + (p[1] - ziel[z][1]) * (p[1] - ziel[z][1]) +
                                                            (p[2] - ziel[z][2]) * (p[2] - ziel[z][2])));
            }
        }
        diag::detail("Knopftest: Stufe 8/4 " + std::to_string(geschaetzt) + " geschaetzt:" + text + "; Abstand " + std::to_string(naechst[0]) + " / " +
                     std::to_string(naechst[1]) + " / schaltbar " + std::to_string(naechst[2]));
        // experimentell: geprueft wird die Wirkung (Ebene, Schluessel, eines nahe am hellsten Licht, keins am behaltenen),
        // die Genauigkeit steht in den Notizen (drei Lichter auf einer Linie: das zweite liegt 84 Einheiten daneben)
        return ebene >= 0 && geschaetzt >= 1 && inEbene == geschaetzt && naechst[0] < 48.0 && naechst[2] > 47.0 &&
               e.bspRueckLetzt.lichter == static_cast<std::size_t>(geschaetzt);
    }));
    add(foto("karteneditor_s8_lichter"));
    add(tu("Optionen zurueck, Eintrag weg", [] {
        ke::Editor& e = ke::editor();
        e.bspRueck = ke::Editor::BspRueckEinstellungen{};
        ke::einstellungenSpeichern();
        auto& v = e.bauProjekt.eintraege;
        v.erase(std::remove_if(v.begin(), v.end(), [](const karte::BauEintrag& b) { return b.name == "Test Lichter"; }), v.end());
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
    if (teil("editordaten")) { dazu(editordaten()); }
    if (teil("modelle")) { dazu(modelle()); }
    if (teil("lichter")) { dazu(lichter()); }
    return s;
}

}  // namespace stufe8
