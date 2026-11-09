// selbsttest_reste1.inc - Liste reste1 vor rc607 (Karten-Editor).
//
// Punkt 5 (Map-Tab ohne Karte ruht) steht in Modus reiter und 1 (ruheOhneKarte).
//
// Punkt 1: Decals der Effekte in der Echtzeit-Ansicht. CG_ImpactMark schneidet das Quadrat mit
// R_MarkFragments auf die Flaechen der Welt; der Editor hat keine .bsp - seit Pruefung 2 erschien
// dort kein Decal mehr. Jetzt schneidet er gegen die Brushes und Patches (markenWeltAusKarte).
//
// Eingebunden in gui/selbsttest_handbuch.inc (nach Stufe 9d). Allein: BHED_KARTE_NUR=reste1.
namespace reste1 {

namespace ke = karteneditor;

// --- Punkt 1: Decal auf dem Boden ----------------------------------------------------------
std::vector<Schritt> decals() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto da = std::make_shared<bool>(false);
    auto f = std::make_shared<std::array<std::array<double, 3>, 2>>();
    add(tu("Reste1/1: Effekt mit Decal aus dem Spiel (effects/blaster/wall_impact)", [da] {
        const auto& sp = ke::editor().karteSpiel;
        std::string t;
        *da = sp && sp->fs.textLesen("effects/blaster/wall_impact.efx", t) && t.find("Decal") != std::string::npos;
    }));
    add(stufe9d::raumOeffnen("Raum fuer das Decal", false, false));
    add(tu("Reste1/1: Kamera 40 ueber dem Boden, Blick nach unten, Echtzeit-Effekte an", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.effekte = true;
        e.kamera.ursprung = karte::Vektor3f(0, 0, 40);
        e.kamera.winkel = karte::Vektor3f(-89.0F, 0.0F, 0.0F);
    }));
    add(warteBis("Reste1/1: Texturen", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(1.0));
    add(tu("Reste1/1: Bild ohne Decal", [f] { (*f)[0] = stufe9d::mittelFarbe(6, 0, 0); }));
    add(tu("Reste1/1: fx_runner 8 ueber dem Boden (blaster/wall_impact, nach oben)", [] {
        ke::Editor& e = ke::editor();
        e.karte.undo.start();
        auto r = e.karte.neueEntitaet("fx_runner", false);
        r->setzen("origin", "0 0 8");
        r->setzen("fxFile", "blaster/wall_impact");
        e.karte.entitaetEinfuegen(r);
        e.karte.undo.finish("Reste1: fx_runner mit Decal");
        // Der Kasten des fx_runner laege von oben genau ueber dem Decal: Punkt-Entitaeten ausblenden
        // (der Effekt laeuft weiter - die Echtzeit-Effekte fragen den Filter nicht).
        e.filterMaske |= karte::EXCLUDE_POINT_ENT;
        karte::filterAnwenden(e.karte, e.filterMaske);
        ++e.sichtStand;
    }));
    add(pause(2.0));
    add(warteBis("Reste1/1: Decalbilder", [] { return ke::kameraBilderFertig(); }, 3000));
    add(pause(0.5));
    add(tu("Reste1/1: Bild mit Decal", [f] { (*f)[1] = stufe9d::mittelFarbe(6, 0, 0); }));
    add(pruefe("Reste1/1: das Decal liegt auf dem Boden (Brandfleck gegen den Brush geschnitten)", [da, f] {
        const double ohne = (*f)[0][0] + (*f)[0][1] + (*f)[0][2];
        const double mit = (*f)[1][0] + (*f)[1][1] + (*f)[1][2];
        diag::detail("Knopftest: Reste1/1 - Decal: ohne " + stufe9d::farbText((*f)[0]) + ", mit " + stufe9d::farbText((*f)[1]) +
                     ", Ecken " + std::to_string(ke::editor().echtLicht.effektEcken) + "; Welt der Decals (reste2, wie q3map2): " +
                     std::to_string(ke::editor().echtLicht.markenPinsel) + " Pinsel, " + std::to_string(ke::editor().echtLicht.markenFlaechen) +
                     " Flaechen");
        return *da && ohne > 0 && mit < ohne - 30.0 && ke::editor().echtLicht.markenPinsel > 0 && ke::editor().echtLicht.markenFlaechen > 0;
    }));
    add(tu("Reste1/1: Filter zurueck, Echtzeit-Licht aus, neue Karte", [] {
        ke::Editor& e = ke::editor();
        e.filterMaske &= ~static_cast<unsigned int>(karte::EXCLUDE_POINT_ENT);
        karte::filterAnwenden(e.karte, e.filterMaske);
        e.echtLicht.an = false;
        // REST5 P2: der fx_runner hat die Karte geaendert - ohne das fragte neueKarte "New Map" und liess die Frage offen,
        // das Beenden am Ende des Teillaufs wartete dann auf sie (wie selbsttest_editorportal.inc)
        e.karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
    }));
    return s;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    for (Schritt& x : decals()) { s.push_back(std::move(x)); }
    return s;
}

// --- Punkt 5: der Map-Tab ohne geladene Karte ruht (Modus reiter und 1) --------------------
//
// mapDirty blieb ohne Karte stehen (das Netz wird nie "bereit"), darfRuhen lehnte darum immer ab, und
// die Oberflaeche lief ohne Eingabe mit voller Bildrate. Ruhe ist im Selbsttest sonst aus; hier
// ausnahmsweise erlaubt wie im rc596-Abschnitt (selbsttest_vorschau_b.inc).
std::vector<Schritt> ruheOhneKarte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto vorher = std::make_shared<std::pair<int, int>>(0, 0);
    auto ergebnis = std::make_shared<bool>(false);
    add(tu("Reste1/5: Map-Tab ohne Karte, Ruhe im Test erlauben", [vorher] {
        vorher->first = g_app->leftMode;
        g_app->leftMode = 1;
        g_app->playing = false;
        g_app->ruheImTest = true;
        g_app->ruheSperre.fill(0);
    }));
    // REST5 P3: einmal 253 Bilder in 3,5 s unter Volllast (Modus 1 in der Schlussrunde von REST2), in 18 Laeufen unter Last
    // (10 parallel, BHED_TEST_BREMSE 20-200) nicht wieder. Statt einer festen Pause von 2 s erst warten, bis das Programm eine
    // volle Sekunde lang nichts mehr vom Ruhen abhaelt (Nachladen nach dem Wechsel in den Map-Tab braucht unter Last laenger),
    // hoechstens 30 s - danach wird trotzdem gemessen. Gemessen werden weiter 2 s, die Grenze bleibt 8 Bilder je Sekunde.
    {
        auto summe = std::make_shared<long long>(-1);
        auto ruhigSeit = std::make_shared<std::chrono::steady_clock::time_point>();
        auto anfang = std::make_shared<std::chrono::steady_clock::time_point>();
        Schritt w{"Reste1/5: warten, bis das Programm eine Sekunde lang ruhen darf", [summe, ruhigSeit, anfang](int b) {
                      const auto jetzt = std::chrono::steady_clock::now();
                      long long z = 0;
                      for (const int x : g_app->ruheSperre) { z += x; }
                      if (b == 0) {
                          *anfang = jetzt;
                          *summe = -1;
                      }
                      if (z != *summe) {
                          *summe = z;
                          *ruhigSeit = jetzt;
                      }
                      if (std::chrono::duration<double>(jetzt - *ruhigSeit).count() >= 1.0) { return true; }
                      if (std::chrono::duration<double>(jetzt - *anfang).count() > 30.0) {
                          diag::detail("Knopftest: Reste1/5 - nach 30 s noch keine ruhige Sekunde");
                          return true;
                      }
                      return false;
                  }};
        w.eigeneZeitgrenze = true;
        add(std::move(w));
    }
    // Messen: 2 s Bilder zaehlen. Weckten nur Fensternachrichten (von aussen, z. B. unter Last) und kein Grund des Programms,
    // die Messung bis zu dreimal wiederholen - ein Grund aus darfRuhen schlaegt sofort fehl.
    {
        auto bilder = std::make_shared<int>(0);
        auto seit = std::make_shared<std::chrono::steady_clock::time_point>();
        auto versuch = std::make_shared<int>(0);
        Schritt m{"Reste1/5: Bilder zaehlen (2 s)", [bilder, seit, versuch, ergebnis](int b) {
                      const auto jetzt = std::chrono::steady_clock::now();
                      if (b == 0) {
                          *bilder = ImGui::GetFrameCount();
                          *seit = jetzt;
                          g_app->ruheSperre.fill(0);
                          return false;
                      }
                      const double sek = std::chrono::duration<double>(jetzt - *seit).count();
                      if (sek < 2.0) { return false; }
                      const int n = ImGui::GetFrameCount() - *bilder;
                      std::string gruende;
                      bool nurNachricht = true;
                      for (std::size_t i = 0; i < g_app->ruheSperre.size(); ++i) {
                          if (g_app->ruheSperre[i] <= 0) { continue; }
                          gruende += std::string(gruende.empty() ? "" : ", ") + ruheSperreName(i) + " " + std::to_string(g_app->ruheSperre[i]);
                          nurNachricht = nurNachricht && i == 0;
                      }
                      *ergebnis = g_app->geo.surfaces.empty() && static_cast<double>(n) / sek <= 8.0;
                      diag::detail("Knopftest: Reste1/5 - Karte geladen " + std::to_string(!g_app->geo.surfaces.empty()) + ", mapDirty " +
                                   std::to_string(g_app->mapDirty) + ", " + std::to_string(n) + " Bilder in " + std::to_string(sek) +
                                   " s; nicht geruht wegen: " + (gruende.empty() ? "-" : gruende));
                      if (!*ergebnis && nurNachricht && !gruende.empty() && ++*versuch < 3) {
                          diag::detail("Knopftest: Reste1/5 - nur Fensternachrichten, Messung " + std::to_string(*versuch + 1) + " von 3");
                          *bilder = ImGui::GetFrameCount();
                          *seit = jetzt;
                          g_app->ruheSperre.fill(0);
                          return false;
                      }
                      return true;
                  }};
        m.eigeneZeitgrenze = true;
        add(std::move(m));
    }
    add(pruefe("Reste1/5: Map-Tab ohne geladene Karte ruht (hoechstens wenige Bilder je Sekunde)", [ergebnis] {
        g_app->ruheImTest = false;
        return *ergebnis;
    }));
    add(tu("Reste1/5: zurueck", [vorher] {
        g_app->ruheImTest = false;
        g_app->leftMode = vorher->first;
    }));
    add(pause(0.3));
    return s;
}

}  // namespace reste1
