// selbsttest_handbuch.inc - aus gui/selbsttest.cpp geschnitten (Stufe 8d Punkt 5, unveraendert): grosse Karte, Stufe 8g und die Bildlaeufe des Handbuchs
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor);
// keine eigene Uebersetzungseinheit - der vorverarbeitete Text ist derselbe wie vor dem Schnitt.
// Abschnitte: Punkt 12: eine grosse Karte (Ladezeit, Bildzeit, Speicher); Bilder fuer das Handbuch (rc599, BHED_EDITORTEST=handbuch); Stufe 3 Punkt 8: Bilder der Seite "Karte bauen"; Punkt 13: Bilder des Karten-Editors fuer das Handbuch; Stufe 4 Punkt 6: Bilder der Seiten "Fertige Karten oeffnen" und "Plugins"; Stufe 5 Punkt 8: Bilder der Seite "Optimierer und Grenzen"; ROFF-Bahnen bauen und aendern (rc599)

// --- Punkt 12: eine grosse Karte (Ladezeit, Bildzeit, Speicher) --------------
//
// BHED_KARTE_GROSS = Pfad einer grossen Karte (duel_mos_espa.map aus den mit
// q3map2 dekompilierten Movie-Duels-Karten: 31671 Pinsel, 6820 Patches).
// Gemessen: karteOeffnen, die Zeit des Editors je Bild (zeichne, davon die
// Ansichten) bei sich drehender Kamera und wanderndem 2D-Bild, der private
// Speicher des Prozesses und der Grafikspeicher.
std::vector<Schritt> karteneditorGrossSchritte() {
    namespace ke = karteneditor;
    std::vector<Schritt> s;
    const char* pfad = std::getenv("BHED_KARTE_GROSS");
    if (pfad == nullptr || pfad[0] == 0) { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct Mess {
        double ladenMs = 0.0;
        long long speicherVor = 0;
        long long grafikVor = 0;
        long long speicherGeladen = 0;   // gleich nach karteOeffnen (vor dem ersten Bild)
        long long speicherGezeichnet = 0;   // nach den Texturen und dem ersten Zeichnen
        long long speicherDraht = 0;        // gezeichnet im Drahtmodell (ohne Texturen)
        long long speicherBRep = 0;         // nach buildBRep aller Pinsel
        long long heapVor = 0;              // HeapSummary: was new/malloc halten
        long long heapBRep = 0;
        long long heapDraht = 0;
        long long heapEnde = 0;
        long long grafikDraht = 0;
        std::vector<double> editor;
        std::vector<double> ansichten;
        std::vector<double> bild;
        std::vector<double> neubau;   // Bilder direkt nach einem Auswahlwechsel
        std::vector<double> neubauKamera;
        std::vector<double> neubauXy;
    };
    auto m = std::make_shared<Mess>();
    const std::string karte = pfad;
    add(tu("Grosse Karte: Speicher vorher", [m] {
        const gpu::SpeicherStand st = gpu::speicherStand();
        m->speicherVor = st.prozessBytes;
        m->grafikVor = st.grafikBytes;
        m->heapVor = platform::heapBelegt();
    }));
    // Erst im Drahtmodell (keine Texturen), dann texturiert: was kosten die Bilder?
    auto modusVorher = std::make_shared<karte::KameraModus>(karte::KameraModus::Textur);
    add(tu("Grosse Karte: Kamera im Drahtmodell", [modusVorher] {
        *modusVorher = ke::editor().kameraE.modus;
        ke::editor().kameraE.modus = karte::KameraModus::Draht;
    }));
    add(tu("Grosse Karte laden: " + karte, [m, karte] {
        const auto t0 = std::chrono::steady_clock::now();
        (void)ke::karteOeffnen(karte);
        m->ladenMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        m->speicherGeladen = gpu::speicherStand().prozessBytes;
        // buildBRep aller Pinsel vor dem ersten Bild (sonst beim Zeichnen)
        const ke::Editor& e = ke::editor();
        for (const auto& ent : e.karte.entitaeten()) {
            for (const auto& k : ent->kinder()) {
                if (k->art() == karte::Primitiv::Art::Pinsel) { (void)static_cast<const karte::Pinsel&>(*k).ecken(e.karte.kontext); }
            }
        }
        m->speicherBRep = gpu::speicherStand().prozessBytes;
        m->heapBRep = platform::heapBelegt();
    }));
    add(pruefe("Grosse Karte offen", [karte] {
        const ke::Editor& e = ke::editor();
        std::size_t pinsel = 0;
        std::size_t patches = 0;
        for (const auto& ent : e.karte.entitaeten()) {
            for (const auto& k : ent->kinder()) {
                if (std::dynamic_pointer_cast<karte::Pinsel>(k)) { ++pinsel; } else { ++patches; }
            }
        }
        diag::detail("Knopftest: Grosse Karte - " + std::to_string(e.karte.entitaeten().size()) + " Entities, " +
                     std::to_string(pinsel) + " Pinsel, " + std::to_string(patches) + " Patches");
        return e.name == karte && pinsel > 30000;
    }));
    add(pause(0.5));
    add(tu("Grosse Karte: Speicher nach dem Drahtmodell, dann texturiert", [m, modusVorher] {
        m->speicherDraht = gpu::speicherStand().prozessBytes;
        m->heapDraht = platform::heapBelegt();
        m->grafikDraht = gpu::speicherStand().grafikBytes;
        ke::editor().kameraE.modus = *modusVorher;
    }));
    add(warteBis("Grosse Karte: Texturen geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    add(pause(0.5));
    add(tu("Grosse Karte: Speicher nach dem ersten Zeichnen", [m] { m->speicherGezeichnet = gpu::speicherStand().prozessBytes; }));
    // 240 Bilder: die Kamera dreht sich, das 2D-Bild wandert (alles neu zeichnen)
    add({"Grosse Karte: Bildzeiten (240 Bilder)", [m](int b) {
             ke::Editor& e = ke::editor();
             if (b > 0) {
                 m->editor.push_back(e.zeichenMs);
                 m->ansichten.push_back(e.ansichtenMs);
                 m->bild.push_back(static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0);
             }
             e.kamera.freiBewegen(6, (b / 60) % 2 == 0 ? 1 : -1, e.kameraE);
             e.xy[0].ansicht.rollen((b / 60) % 2 == 0 ? 8 : -8, 0);
             return b >= 240;
         }});
    // Eine Auswahl aendert den Stand der Szene: Kamera und 2D bauen nur den
    // Rest neu, die Geometrie (Pinsel, Patches) bleibt.
    auto neubauten = std::make_shared<std::array<std::size_t, 2>>();
    add(tu("Grosse Karte: Neubauten merken", [neubauten] {
        (*neubauten)[0] = ke::editor().geoNeubauten;
        (*neubauten)[1] = ke::editor().restNeubauten;
    }));
    add({"Grosse Karte: Auswahl wechseln (5 x), Bildzeit mit Neubau", [m](int b) {
             ke::Editor& e = ke::editor();
             if (b > 0 && b % 2 == 0) {
                 m->neubau.push_back(e.zeichenMs);
                 m->neubauKamera.push_back(e.kameraNeubauMs);
                 m->neubauXy.push_back(e.xyNeubauMs);
             }
             if (b % 2 == 1) {
                 e.karte.auswahl.leeren();
                 const auto& kinder = e.karte.welt()->kinder();
                 if (!kinder.empty()) { e.karte.auswahl.waehlen(e.karte.welt(), kinder[static_cast<std::size_t>(b * 997) % kinder.size()]); }
             }
             return b >= 10;
         }});
    add(pruefe("Grosse Karte: Auswahlwechsel bauen nur den Rest neu, die Geometrie nicht", [neubauten] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Grosse Karte - Neubauten Geometrie " + std::to_string(e.geoNeubauten - (*neubauten)[0]) + ", Rest " +
                     std::to_string(e.restNeubauten - (*neubauten)[1]));
        return e.geoNeubauten == (*neubauten)[0] && e.restNeubauten >= (*neubauten)[1] + 5;
    }));
    add(pruefe("Grosse Karte: Kamera-Szene nach dem Hochladen nicht mehr im Speicher", [] {
        return ke::kameraSzeneMass().find("im Speicher 0 MB") != std::string::npos;
    }));
    add(tu("Grosse Karte: Auswahl leeren", [] { ke::editor().karte.auswahl.leeren(); }));
    add(pruefe("Grosse Karte: Messwerte", [m] {
        const auto stat = [](std::vector<double> v) {
            if (v.empty()) { return std::string("-"); }
            std::sort(v.begin(), v.end());
            double summe = 0.0;
            for (double x : v) { summe += x; }
            char z[160];
            std::snprintf(z, sizeof(z), "Mittel %.2f ms, Median %.2f, 95%% %.2f, max %.2f", summe / static_cast<double>(v.size()),
                          v[v.size() / 2], v[std::min(v.size() - 1, v.size() * 95 / 100)], v.back());
            return std::string(z);
        };
        const gpu::SpeicherStand st = gpu::speicherStand();
        char z[400];
        std::snprintf(z, sizeof(z), "Knopftest: Grosse Karte - Laden %.0f ms; Speicher privat %lld -> %lld MB (+%lld), Grafik %lld -> %lld MB",
                      m->ladenMs, m->speicherVor / (1024 * 1024), st.prozessBytes / (1024 * 1024),
                      (st.prozessBytes - m->speicherVor) / (1024 * 1024), m->grafikVor / (1024 * 1024), st.grafikBytes / (1024 * 1024));
        diag::detail(z);
        std::snprintf(z, sizeof(z),
                      "Knopftest: Grosse Karte - Speicher: nach dem Lesen +%lld MB, nach buildBRep +%lld MB, gezeichnet ohne Texturen +%lld MB (Grafik %lld MB), mit Texturen +%lld MB",
                      (m->speicherGeladen - m->speicherVor) / (1024 * 1024), (m->speicherBRep - m->speicherVor) / (1024 * 1024),
                      (m->speicherDraht - m->speicherVor) / (1024 * 1024),
                      m->grafikDraht / (1024 * 1024), (m->speicherGezeichnet - m->speicherVor) / (1024 * 1024));
        diag::detail(z);
        m->heapEnde = platform::heapBelegt();
        std::snprintf(z, sizeof(z), "Knopftest: Grosse Karte - Heap: nach buildBRep +%lld MB, Drahtmodell +%lld MB, Ende +%lld MB",
                      (m->heapBRep - m->heapVor) / (1024 * 1024), (m->heapDraht - m->heapVor) / (1024 * 1024),
                      (m->heapEnde - m->heapVor) / (1024 * 1024));
        diag::detail(z);
        diag::detail("Knopftest: Grosse Karte - " + ke::kameraSzeneMass());
        diag::detail("Knopftest: Grosse Karte - " + ke::xySzeneMass());
        diag::detail("Knopftest: Grosse Karte - Editor je Bild: " + stat(m->editor));
        diag::detail("Knopftest: Grosse Karte - davon Ansichten: " + stat(m->ansichten));
        diag::detail("Knopftest: Grosse Karte - Bildabstand: " + stat(m->bild));
        diag::detail("Knopftest: Grosse Karte - Bild nach Auswahlwechsel (Neubau): " + stat(m->neubau));
        diag::detail("Knopftest: Grosse Karte - davon Kamera-Szene: " + stat(m->neubauKamera));
        diag::detail("Knopftest: Grosse Karte - davon 2D-Szene: " + stat(m->neubauXy));
        return !m->editor.empty();
    }));
    // Stufe 2, Punkt 12: CSG, Clipper und Komponenten auf der grossen Karte
    struct Stufe2Mess {
        double csgMs = 0, csgUndoMs = 0, csgBild = 0, clipMs = 0, clipBild = 0, eckenMs = 0, eckenBild = 0, ecken1000Ms = 0, ecken1000Bild = 0;
        long long pinselVor = 0, pinselNach = 0;
    };
    auto m2 = std::make_shared<Stufe2Mess>();
    // das laengste der naechsten 6 Bilder (der Neubau nach dem Befehl faellt in eines davon)
    const auto laengstesBild = [](double* ziel) {
        return Schritt{"Grosse Karte: laengstes der naechsten 6 Bilder", [ziel](int b) {
                           if (b == 0) {
                               *ziel = 0.0;
                               return false;
                           }
                           *ziel = std::max(*ziel, ke::editor().zeichenMs);
                           return b >= 6;
                       }};
    };
    const auto pinselZaehlen = [] {
        long long n = 0;
        for (const auto& ent : ke::editor().karte.entitaeten()) {
            for (const auto& k : ent->kinder()) { n += k->art() == karte::Primitiv::Art::Pinsel ? 1 : 0; }
        }
        return n;
    };
    // ein Pinsel aus der Mitte der worldspawn-Liste mit einiger Groesse
    const auto mittlererPinsel = [] {
        const auto& kinder = ke::editor().karte.welt()->kinder();
        for (std::size_t i = kinder.size() / 2; i < kinder.size(); ++i) {
            if (kinder[i]->art() != karte::Primitiv::Art::Pinsel) { continue; }
            const karte::Kasten b = std::static_pointer_cast<karte::Pinsel>(kinder[i])->grenzen(ke::editor().karte.kontext);
            if (b.extents[0] > 32 && b.extents[1] > 32 && b.extents[2] > 32) { return kinder[i]; }
        }
        return kinder.front();
    };
    add(tu("Grosse Karte: CSG Subtract mit einem Pinsel aus der Mitte", [m2, pinselZaehlen, mittlererPinsel] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), mittlererPinsel());
        m2->pinselVor = pinselZaehlen();
        const auto t0 = std::chrono::steady_clock::now();
        (void)ke::ausfuehren("CSGSubtract");
        m2->csgMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        m2->pinselNach = pinselZaehlen();
    }));
    add(laengstesBild(&m2->csgBild));
    add(tu("Grosse Karte: Rueckgaengig", [m2] {
        const auto t0 = std::chrono::steady_clock::now();
        (void)ke::ausfuehren("Undo");
        m2->csgUndoMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    }));
    add(pause(0.1));
    add(tu("Grosse Karte: Clipper an, Ebene durch die Mitte, Clip", [m2, mittlererPinsel] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        const auto p = mittlererPinsel();
        e.karte.auswahl.waehlen(e.karte.welt(), p);
        (void)ke::ausfuehren("ToggleClipper");
        const karte::Kasten b = std::static_pointer_cast<karte::Pinsel>(p)->grenzen(e.karte.kontext);
        const karte::Vektor3d c(b.origin[0], b.origin[1], b.origin[2]);
        e.bearbeitung.clipperPunkte.punkte[0] = c + karte::Vektor3d(0, -64, 0);
        e.bearbeitung.clipperPunkte.punkte[1] = c + karte::Vektor3d(0, 64, 0);
        e.bearbeitung.clipperPunkte.punkte[2] = c + karte::Vektor3d(0, 0, 64);
        e.bearbeitung.clipperPunkte.anzahl = 3;
        const auto t0 = std::chrono::steady_clock::now();
        (void)ke::ausfuehren("ClipperClip");
        m2->clipMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    }));
    add(laengstesBild(&m2->clipBild));
    add(tu("Grosse Karte: Rueckgaengig, Clipper aus", [m2] {
        (void)ke::ausfuehren("Undo");
        (void)ke::ausfuehren("ToggleClipper");
    }));
    add(pause(0.1));
    add(tu("Grosse Karte: Ecken-Modus mit einem Pinsel", [m2, mittlererPinsel] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), mittlererPinsel());
        const auto t0 = std::chrono::steady_clock::now();
        (void)ke::ausfuehren("DragVertices");
        m2->eckenMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    }));
    add(laengstesBild(&m2->eckenBild));
    add(tu("Grosse Karte: Ecken-Modus aus, 1000 Pinsel waehlen, Ecken-Modus", [m2] {
        ke::Editor& e = ke::editor();
        (void)ke::ausfuehren("DragVertices");
        e.karte.auswahl.leeren();
        std::size_t n = 0;
        for (const auto& k : e.karte.welt()->kinder()) {
            if (k->art() != karte::Primitiv::Art::Pinsel) { continue; }
            e.karte.auswahl.waehlen(e.karte.welt(), k);
            if (++n >= 1000) { break; }
        }
        const auto t0 = std::chrono::steady_clock::now();
        (void)ke::ausfuehren("DragVertices");
        m2->ecken1000Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    }));
    add(laengstesBild(&m2->ecken1000Bild));
    add(tu("Grosse Karte: Ecken-Modus aus", [m2] {
        (void)ke::ausfuehren("DragVertices");
        ke::editor().karte.auswahl.leeren();
    }));
    add(pruefe("Grosse Karte: Stufe-2-Zeiten (CSG Subtract, Clip, Ecken-Modus)", [m2] {
        char z[480];
        std::snprintf(z, sizeof(z),
                      "Knopftest: Grosse Karte - CSG Subtract %.1f ms (Pinsel %lld -> %lld), Bild danach %.1f ms, Rueckgaengig %.1f ms; Clip %.1f ms, Bild "
                      "danach %.1f ms; Ecken-Modus 1 Pinsel %.2f ms (Bild %.1f ms), 1000 Pinsel %.1f ms (Bild %.1f ms)",
                      m2->csgMs, m2->pinselVor, m2->pinselNach, m2->csgBild, m2->csgUndoMs, m2->clipMs, m2->clipBild, m2->eckenMs, m2->eckenBild,
                      m2->ecken1000Ms, m2->ecken1000Bild);
        diag::detail(z);
        return m2->pinselNach >= m2->pinselVor && m2->csgMs > 0.0;
    }));
    add(tu("Grosse Karte: Testkarte wieder laden", [] { (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map")); }));
    add(pause(0.3));
    return s;
}

namespace stufe8c {
std::vector<Schritt> schritte();   // selbsttest_karte8c.inc: Tutorial "Erste eigene Karte"
}  // namespace stufe8c
namespace stufe8g {
std::vector<Schritt> schritte();   // selbsttest_karte8g.inc: Build > GPU bake (nach dem Tutorial)
}  // namespace stufe8g
namespace stufe9 {
std::vector<Schritt> schritte();   // selbsttest_karte9.inc: Stufe 9 (nach dem GPU-Backen)
}  // namespace stufe9
namespace optsicher {
std::vector<Schritt> schritte();   // selbsttest_optsicher.inc: Fix all safe aendert nichts Sichtbares
}  // namespace optsicher

std::vector<Schritt> karteneditorSchritte() {
    namespace ke = karteneditor;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // =tempo: nur die Messliste der Stufe 7 (tools/karte/tempo.py editor)
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "tempo") == 0) {
        for (Schritt& x : tempo::schritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    // =split1/2/3: Split-Fenster frei belegen ueber echte Neustarts (Stufe 8b Punkt 2, ein Einstellungsordner)
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strncmp(nur, "split", 5) == 0 && nur[5] >= '1' && nur[5] <= '3') {
        for (Schritt& x : stufe8b::splitNeustart(nur[5] - '0')) { add(std::move(x)); }
        return s;
    }
    // =letzte1/letzte2/letzte3: die letzte Karte beim Start (Stufe 7 Punkt 6f, zwei Laeufe, ein Einstellungsordner)
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strncmp(nur, "letzte", 6) == 0) {
        const std::string n = nur;
        for (Schritt& x : (n == "letzte1" ? stufe7::letzte1() : stufe7::letzte2(n == "letzte2"))) { add(std::move(x)); }
        return s;
    }
    add(tu("Karten-Editor: Spielordner, Modus Events", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
    }));
    add(pause(0.3));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(pause(0.3));
    add(pruefe("Modusleiste: Karten-Editor ist der Arbeitsbereich", [] { return g_app->leftMode == 3 && ke::aktiv(); }));
    add(warteBis("Karten-Editor: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(pruefe("Spiel: Entity-Klassen aus data/karte, worldspawn und info_player_start bekannt", [] {
        const auto& sp = ke::editor().spiel;
        if (!sp) { return false; }
        diag::detail("Knopftest: Karten-Editor - " + sp->name + ", " + std::to_string(sp->klassen.anzahl()) +
                     " Klassen, " + std::to_string(sp->inventar.shader().size()) + " Shader, " +
                     std::to_string(sp->ordner.size()) + " Ordner");
        return sp->klassen.anzahl() > 150 && sp->klassen.finden("worldspawn") != nullptr &&
               sp->klassen.finden("info_player_start") != nullptr;
    }));
    add(pruefe("Standard-Texturskalierung 0.25 (default_scale aus ja.game/jk2.game)",
               [] { return karte::standardTexturSkalierung() == 0.25F; }));
    // NRC legt neue Karten fuer Quake3-Spiele als Brush Primitives an (Brush_Construct(
    // eBrushTypeQuake3BP), Seite Brush "New map Brush Type"); die Schritte der Stufe 1
    // setzen Axial Projection voraus - der Test stellt das ein (Punkt 11 prueft die Vorgabe).
    add(pruefe("Vorgabe \"New map Brush Type\": Brush Primitives (Index 1, wie NRC)", [] { return ke::editor().pinselTyp == 1; }));
    // Stufe 7 Punkt 6b: die Menues zeichnet der Editor jetzt im Stil des Programms
    // (Vorgabe); die Masse und Fotos der Stufen 1-6 sind NRCs - der Test stellt
    // "Menus like NetRadiant-Custom" ein (stufe7::menue prueft den Programmstil).
    add(tu("Testeinstellung: Menus like NetRadiant-Custom (gespeichert)", [] { ke::menueStilSetzen(true); }));
    add(tu("Testeinstellung: New map Brush Type = Axial Projection", [] {
        ke::editor().pinselTyp = 0;
        if (ke::editor().karte.entitaeten().empty()) { ke::editor().karte.kontext.format = karte::PinselFormat::Quake3; }
    }));
    // Eine kleine Karte im NRC-Format: worldspawn mit einem Pinsel, ein Licht.
    add(tu("Testkarte schreiben", [] {
        karte::Karte k;
        k.klassen = &ke::editor().spiel->klassen;
        auto welt = k.weltHolenOderAnlegen();
        welt->setzen("message", "karteneditor");
        auto p = std::make_shared<karte::Pinsel>();
        karte::TexturProjektion proj;
        karte::texdefStandard(karte::TexdefArt::Quake, proj);
        p->quaderBauen(karte::Kasten(karte::Vektor3f(0, 0, 32), karte::Vektor3f(64, 64, 32)), "textures/common/caulk", proj,
                       k.kontext);
        welt->kindEinfuegen(p);
        auto licht = k.neueEntitaet("light", false);
        licht->setzen("origin", "0 0 128");
        licht->setzen("light", "300");
        k.entitaetEinfuegen(licht);
        (void)karte::karteSpeichern(karteTestPfad("karte_quelle.map"), k, false);
    }));
    add(tu("Karte oeffnen (wie nach dem Dateidialog)", [] { (void)ke::karteOeffnen(karteTestPfad("karte_quelle.map")); }));
    add(pause(0.2));
    add(pruefe("Geoeffnet: Name, 2 Entities, 1 Pinsel, nicht geaendert", [] {
        const ke::Editor& e = ke::editor();
        return e.name == karteTestPfad("karte_quelle.map") && e.karte.entitaeten().size() == 2 &&
               e.karte.welt() != nullptr && e.karte.welt()->kinder().size() == 1 && !ke::geaendert();
    }));
    add(tu("Speichern unter (wie nach dem Dateidialog)", [] { (void)ke::karteSpeichernUnter(karteTestPfad("karte_kopie.map")); }));
    add(pruefe("Gespeichert: Datei bytegleich zur Quelle (NRC-Format), oben in der zuletzt-Liste", [] {
        const std::string a = dateiInhalt(karteTestPfad("karte_quelle.map"));
        const std::string b = dateiInhalt(karteTestPfad("karte_kopie.map"));
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Karten-Editor - Quelle " + std::to_string(a.size()) + " Byte, Kopie " +
                     std::to_string(b.size()) + " Byte");
        return !a.empty() && a == b && e.zuletzt.anzahl() >= 1 && e.zuletzt.pfade()[0] == karteTestPfad("karte_kopie.map") &&
               g_app->settings.karteZuletzt.front() == karteTestPfad("karte_kopie.map");
    }));
    // Aendern, dann New Map: die Rueckfrage kommt; Cancel laesst die Karte.
    add(tu("Karte aendern (worldspawn message)", [] {
        ke::Editor& e = ke::editor();
        karte::Befehl b(e.karte.undo, "entitySetKeyValue -key \"message\" -value \"geaendert\"");
        e.karte.welt()->setzen("message", "geaendert");
    }));
    add(pruefe("Geaendert (Map_Modified)", [] { return ke::geaendert(); }));
    add(klick(ke::trK(Str::KeLFile), "###main"));
    add(klick(ke::trK(Str::KeLNewMap)));
    add(pause(0.3));
    add(pruefe("New Map bei geaenderter Karte: Rueckfrage offen", [] { return ke::editor().frageOffen; }));
    add(klick(tr(Str::KeCancel)));
    add(pause(0.3));
    add(pruefe("Cancel: Karte bleibt (Name, Aenderung)", [] {
        const ke::Editor& e = ke::editor();
        return !e.frageOffen && e.name == karteTestPfad("karte_kopie.map") && ke::geaendert() &&
               e.karte.welt()->wert("message") == "geaendert";
    }));
    add(klick(ke::trK(Str::KeLFile), "###main"));
    add(klick(ke::trK(Str::KeLNewMap)));
    add(pause(0.3));
    add(klick(tr(Str::KeDiscard)));
    add(pause(0.3));
    add(pruefe("Discard: neue, leere Karte \"unnamed.map\"", [] {
        const ke::Editor& e = ke::editor();
        return !e.frageOffen && e.name == "unnamed.map" && e.karte.entitaeten().empty() && !ke::geaendert();
    }));
    // Zuletzt geoeffnet: der erste Eintrag oeffnet die gespeicherte Kopie.
    add(klick(ke::trK(Str::KeLFile), "###main"));
    add({"Zuletzt geoeffnet: Eintrag 1 anklicken", [](int b) {
             static Schritt k;
             if (b == 0) { k = klick("1- " + karteTestPfad("karte_kopie.map")); }
             return k.tun(b);
         }});
    add(pause(0.3));
    add(pruefe("Zuletzt geoeffnet: die Kopie ist offen", [] {
        const ke::Editor& e = ke::editor();
        return e.name == karteTestPfad("karte_kopie.map") && e.karte.entitaeten().size() == 2 && !ke::geaendert();
    }));
    add(pruefe("Konsole: Ladezeilen wie NRC (Loading map from, primitives, entities)", [] {
        bool laden = false;
        bool prim = false;
        for (const ke::KonsolenZeile& z : ke::editor().konsole) {
            laden = laden || z.text.rfind("Loading map from ", 0) == 0;
            prim = prim || z.text == "1 primitives";
        }
        return laden && prim;
    }));
    add(foto("karteneditor_start"));
    // BHED_KARTE_NUR=fotos: nur die Fotos zum NRC-Vergleich (schnell beim
    // Nachmessen); =gross: nur die grosse Karte (BHED_KARTE_GROSS).
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "fotos") == 0) {
        for (Schritt& x : karteneditorFotoSchritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "gross") == 0) {
        for (Schritt& x : karteneditorGrossSchritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    // =stufe3: nur die Stufe 3 (Karten bauen) und die Masse des Rahmens
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "stufe3") == 0) {
        for (Schritt& x : karteneditorStufe3Schritte()) { add(std::move(x)); }
        for (Schritt& x : karteneditorMassSchritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    // =stufe4: nur die Stufe 4 (fertige Karten, Originallicht, Plugins)
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "stufe4") == 0) {
        for (Schritt& x : karteneditorStufe4Schritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    // =stufe5: nur die Stufe 5 (der Optimierer)
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "stufe5") == 0) {
        for (Schritt& x : karteneditorStufe5Schritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    // =stufe6: nur die Stufe 6 (die Pruefstufe: jeder Befehl, jede Taste, jeder Menuepunkt und Knopf mit Wirkung)
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "stufe6") == 0) {
        for (Schritt& x : karteneditorStufe6Schritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    // =stufe7: nur die Stufe 7 (Echtzeit-Licht, Schnell-Backen; BHED_STUFE7_NUR=licht|backen|gross)
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "stufe7") == 0) {
        for (Schritt& x : stufe7::schritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    // =stufe8: nur die Stufe 8a (Fortschritt, BSP-Import, Teil-Kompilierungen; BHED_STUFE8_NUR=<Teil>)
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "stufe8") == 0) {
        for (Schritt& x : stufe8::schritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    // =stufe8b: nur die Stufe 8b (neue Editor-Werkzeuge; BHED_STUFE8B_NUR=<Teil>)
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "stufe8b") == 0) {
        for (Schritt& x : stufe8b::schritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    // =stufe8c: nur die Stufe 8c (Tutorial "Erste eigene Karte")
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "stufe8c") == 0) {
        for (Schritt& x : stufe8c::schritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    // =stufe8g: das Tutorial der Stufe 8c (baut die Karte) und Build > GPU bake
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "stufe8g") == 0) {
        for (Schritt& x : stufe8c::schritte()) { add(std::move(x)); }
        for (Schritt& x : stufe8g::schritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    // =optsicher: "Fix all safe" auf den Karten aus BHED_OPTSICHER_KARTEN (Kamera-Vergleich)
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "optsicher") == 0) {
        for (Schritt& x : optsicher::schritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    // =stufe9: das Tutorial der Stufe 8c, Build > GPU bake und die Stufe 9
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "stufe9") == 0) {
        for (Schritt& x : stufe8c::schritte()) { add(std::move(x)); }
        for (Schritt& x : stufe8g::schritte()) { add(std::move(x)); }
        for (Schritt& x : stufe9::schritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    // =stufe2: nur die Wirkungspruefungen der Stufe 2 (Bauwerkzeuge)
    if (const char* nur = std::getenv("BHED_KARTE_NUR"); nur != nullptr && std::strcmp(nur, "stufe2") == 0) {
        for (Schritt& x : karteneditorStufe2Schritte()) { add(std::move(x)); }
        add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
        return s;
    }
    for (Schritt& x : karteneditorRahmenSchritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorTastenSchritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorMassSchritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorXySchritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorKameraSchritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorAuswahlSchritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorTexturSchritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorEntitaetenSchritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorKonsoleSchritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorStufe2Schritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorStufe3Schritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorStufe4Schritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorStufe5Schritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorAlleBefehleSchritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorStufe6Schritte()) { add(std::move(x)); }
    for (Schritt& x : stufe7::schritte()) { add(std::move(x)); }   // Stufe 7: Echtzeit-Licht, Schnell-Backen (gross nur mit BHED_KARTE_GROSS)
    for (Schritt& x : stufe8::schritte()) { add(std::move(x)); }   // Stufe 8a: Fortschritt, BSP-Import, Teil-Kompilierungen
    for (Schritt& x : stufe8b::schritte()) { add(std::move(x)); }   // Stufe 8b: neue Editor-Werkzeuge
    for (Schritt& x : stufe8c::schritte()) { add(std::move(x)); }   // Stufe 8c: Tutorial "Erste eigene Karte"
    for (Schritt& x : stufe8g::schritte()) { add(std::move(x)); }   // Stufe 8g: Build > GPU bake (nach dem Tutorial)
    for (Schritt& x : stufe9::schritte()) { add(std::move(x)); }    // Stufe 9: restliche Karten-Werkzeuge (nach 8g)
    for (Schritt& x : karteneditorFotoSchritte()) { add(std::move(x)); }
    for (Schritt& x : karteneditorGrossSchritte()) { add(std::move(x)); }   // nur mit BHED_KARTE_GROSS
    add(tu("Karten-Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
    return s;
}

// --- Bilder fuer das Handbuch (rc599, BHED_EDITORTEST=handbuch) ----------------
//
// shank: "Dokumentation mit Bildern und Tutorials ... fuer alle Sprachen".
// Nimmt jede Szene des Handbuchs auf - nur das eigene Fenster. Neben jedes
// Bild <name>.bmp schreibt fotoTeil <name>.rect (x0 y0 x1 y1 in Bildpunkten),
// tools/handbuch/bilder.py schneidet danach zu und macht .webp daraus.
//   BHED_FOTOS           Zielordner
//   BHED_SPRACHE         en / de / zh / ja
//   BHED_MISSION_ARCHIV  MD_Missions_Ep3.pk3, BHED_MISSION_NAME md_am_sith
namespace hb {

ImRect fensterRect(const char* teil) {
    for (ImGuiWindow* w : GImGui->Windows) {
        if (w->WasActive && !w->Hidden && std::strstr(w->Name, teil) != nullptr) { return w->Rect(); }
    }
    return ImRect{};
}

ImRect karte() {
    return ImRect{g_app->kartenAnsicht[0], g_app->kartenAnsicht[1], g_app->kartenAnsicht[2], g_app->kartenAnsicht[3]};
}

ImRect um(ImVec2 c, float halbB, float halbH) { return ImRect{c.x - halbB, c.y - halbH, c.x + halbB, c.y + halbH}; }

ImRect vereint(ImRect a, ImRect b) {
    if (a.GetWidth() <= 0.0F) { return b; }
    if (b.GetWidth() <= 0.0F) { return a; }
    a.Add(b);
    return a;
}

// Markierungen fuer ein Foto (rc600): Rahmen in Bildschirmpunkten, bilder.py
// zeichnet sie als gelben Kasten mit Pfeil (wie die Anleitungen von Unreal).
// nr > 0: ein nummerierter Rahmen (die Nummer steht darunter, kein Pfeil) -
// fuer Leisten, deren Teile eine Tabelle im Handbuch erklaert.
struct Marke {
    ImRect r;
    int nr = 0;
    Marke(ImRect rr, int n = 0) : r(rr), nr(n) {}   // NOLINT(google-explicit-constructor) - ein ImRect ist eine Marke
};
using Marken = std::function<std::vector<Marke>()>;

std::map<std::string, Marken>& markenListe() {
    static std::map<std::string, Marken> m;
    return m;
}
// Die Markierungen eines Bildes (oder einer Bildfolge: Name ohne _NN).
void markiere(const std::string& name, Marken m) { markenListe()[name] = std::move(m); }

Schritt fotoTeil(const std::string& name, std::function<ImRect()> ausschnitt, Marken marken = nullptr) {
    return {"Foto " + name, [=](int b) {
                const char* ordner = std::getenv("BHED_FOTOS");
                if (ordner == nullptr) { return true; }
                Marken mk = marken;
                if (!mk) {
                    auto& liste = markenListe();
                    auto it = liste.find(name);
                    if (it == liste.end() && name.size() > 3 && name[name.size() - 3] == '_') { it = liste.find(name.substr(0, name.size() - 3)); }
                    if (it != liste.end()) { mk = it->second; }
                }
                if (b == 1 && mk) {
                    std::ofstream m(::bhed::utf8Pfad(std::string(ordner) + "\\" + name + ".marks"));
                    int nr = 0;
                    for (const Marke& eine : mk()) {
                        const ImRect& k = eine.r;
                        ++nr;
                        if (k.GetWidth() <= 1.0F || k.GetHeight() <= 1.0F) {
                            // Was steht im Ausschnitt? So findet sich die richtige Beschriftung.
                            const ImRect aus = ausschnitt ? ausschnitt() : ImRect{};
                            std::string l;
                            for (const Element& e : g_elemente) {
                                if (!e.label.empty() && aus.Contains(e.rect.GetCenter())) { l += "[" + e.label + "]"; }
                            }
                            diag::detail("Knopftest: Marke leer - " + name + " #" + std::to_string(nr) + " | im Bild: " + l.substr(0, 900));
                            continue;
                        }
                        m << static_cast<int>(k.Min.x) << " " << static_cast<int>(k.Min.y) << " " << static_cast<int>(k.Max.x)
                          << " " << static_cast<int>(k.Max.y);
                        if (eine.nr > 0) { m << " " << eine.nr; }
                        m << "\n";
                    }
                }
                if (b == 1) {
                    ImRect r = ausschnitt ? ausschnitt() : ImRect{};
                    const ImVec2 d = ImGui::GetIO().DisplaySize;
                    r.Min.x = std::max(0.0F, r.Min.x);
                    r.Min.y = std::max(0.0F, r.Min.y);
                    r.Max.x = std::min(d.x, r.Max.x);
                    r.Max.y = std::min(d.y, r.Max.y);
                    std::ofstream f(::bhed::utf8Pfad(std::string(ordner) + "\\" + name + ".rect"));
                    if (r.GetWidth() > 8.0F && r.GetHeight() > 8.0F) {
                        f << static_cast<int>(r.Min.x) << " " << static_cast<int>(r.Min.y) << " " << static_cast<int>(r.Max.x)
                          << " " << static_cast<int>(r.Max.y) << "\n";
                    }
                }
                if (b == 2) { render::fotoAnfordern(std::string(ordner) + "\\" + name + ".bmp"); }
                return b >= 5;
            }};
}

// Das Element mit dieser Beschriftung (wie finde), auf seinen sichtbaren Teil
// beschnitten. `letztes`: das unterste von mehreren (etwa der Open-Knopf in
// der letzten Feldzeile).
ImRect knopf(const std::string& label, const std::string& fenster = "", bool letztes = false) {
    const Element* gefunden = nullptr;
    for (int k = 0;; ++k) {
        const Element* e = finde(label, fenster, k, false);
        if (e == nullptr) { break; }
        if (gefunden == nullptr || (letztes && e->rect.Min.y > gefunden->rect.Min.y)) { gefunden = e; }
        if (!letztes) { break; }
    }
    if (gefunden == nullptr) { return ImRect{}; }
    ImRect r = gefunden->rect;
    r.ClipWithFull(gefunden->clip);
    return r;
}

// Das n-te Element mit dieser Beschriftung (0 = erstes), wahlweise als Praefix.
ImRect knopfN(const std::string& label, const std::string& fenster, int n, bool praefix = false) {
    const Element* e = finde(label, fenster, n, praefix);
    if (e == nullptr) { return ImRect{}; }
    ImRect r = e->rect;
    ImRect c = r;
    c.ClipWithFull(e->clip);
    // In Tabellen ist der gemerkte Clip-Bereich manchmal leer - dann das Element selbst.
    return (c.GetWidth() > 1.0F && c.GetHeight() > 1.0F) ? c : r;
}

// Die n-te Zeile des Skriptbaums mit diesem Befehl - ueber die ganze Breite
// des Baums, damit der Rahmen auch die Werte umfasst.
ImRect zeile(const std::string& befehl, int n = 0) {
    ImRect r = knopfN(befehl, "pane0", n);
    if (r.GetWidth() <= 0.0F) { r = knopfN(befehl, "##flowarea", n); }
    if (r.GetWidth() <= 0.0F) { return r; }
    const ImRect baum = fensterRect("##flowarea");
    r.Max.x = baum.Min.x + baum.GetWidth() * 0.62F;
    return r;
}

// Ein Eintrag der Befehlsliste links ("affect  ( ... )") oder ein Makro.
ImRect eintrag(const std::string& name) {
    ImRect r = knopfN(name + "  ", "", 0, true);
    if (r.GetWidth() <= 0.0F) { r = knopfN(name, "", 0, true); }
    return r;
}

// Mehrere Knoepfe als ein Kasten (eine Gruppe einer Leiste).
ImRect gruppe(const std::vector<std::string>& labels, const std::string& fenster = "") {
    ImRect r{};
    bool da = false;
    for (const std::string& l : labels) {
        const ImRect k = knopf(l, fenster);
        if (k.GetWidth() <= 0.0F) { continue; }
        if (!da) { r = k; da = true; } else { r.Add(k); }
    }
    return r;
}

// Ein Punkt der Karte als kleiner Kasten (Halbmesser in Bildpunkten).
ImRect punkt(const float w[3], float halb) {
    ImVec2 q{0, 0};
    if (!kartenPunktAufSchirm(w, q)) { return ImRect{}; }
    return ImRect{q.x - halb, q.y - halb, q.x + halb, q.y + halb};
}

int entityMitName(const char* klasse, const char* name) {
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        const MapEntity& e = g_app->map.entities[i];
        const std::string* st = e.find("script_targetname");
        if (e.classname == klasse && st != nullptr && *st == name) { return static_cast<int>(i); }
    }
    return -1;
}

ImVec2 entityAufSchirm(int ei);

// Eine Entity (nach targetname oder NPC_targetname) als Kasten.
ImRect entityKasten(const char* name, float halb) {
    for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
        const MapEntity& e = g_app->map.entities[i];
        const std::string* npc = e.find("NPC_targetname");
        const std::string* st = e.find("script_targetname");
        if (e.targetname == name || (npc != nullptr && *npc == name) || (st != nullptr && *st == name)) {
            const ImVec2 q = entityAufSchirm(static_cast<int>(i));
            if (q.x <= 0.0F && q.y <= 0.0F) { return ImRect{}; }
            return ImRect{q.x - halb, q.y - halb, q.x + halb, q.y + halb};
        }
    }
    return ImRect{};
}

// Eine Figur der Szene dort, wo sie zur eingestellten Zeit steht.
ImRect figurKasten(const char* name, float halb) {
    for (const Actor& a : g_app->scene.actors) {
        if (a.name != name) { continue; }
        float o[3];
        const ActorState st = a.at(g_app->playMs);
        for (int k = 0; k < 3; ++k) { o[k] = st.pos[k]; }
        o[2] += 30.0F;
        ImVec2 q{0, 0};
        if (!kartenPunktAufSchirm(o, q)) { return ImRect{}; }
        return ImRect{q.x - halb, q.y - halb * 1.8F, q.x + halb, q.y + halb * 1.4F};
    }
    return ImRect{};
}

ImVec2 entityAufSchirm(int ei) {
    float o[3] = {0, 0, 0};
    if (ei < 0 || !entityOrtAnzeige(g_app->map.entities[static_cast<std::size_t>(ei)], o)) { return ImVec2{0, 0}; }
    ImVec2 s{0, 0};
    (void)kartenPunktAufSchirm(o, s);
    return s;
}

void kameraZurueck(float weg, float hoch) {
    float f[3];
    g_app->cam.forward(f);
    for (int k = 0; k < 3; ++k) { g_app->cam.pos[k] -= f[k] * weg; }
    g_app->cam.pos[2] += hoch;
    g_app->mapDirty = true;
}

// Erster Befehl camera ( MOVE ... ) auf oberster oder zweiter Ebene.
bool sucheMove(const std::vector<Node>& ns, Path& weg) {
    for (std::size_t i = 0; i < ns.size(); ++i) {
        weg.push_back(i);
        const Node& n = ns[i];
        if (n.name == "camera" && !n.args.empty() && n.args[0].text == "MOVE") { return true; }
        if (sucheMove(n.children, weg)) { return true; }
        weg.pop_back();
    }
    return false;
}

Path kameraMove() {
    Path weg;
    return sucheMove(g_app->doc.script().nodes, weg) ? weg : Path{};
}

// Zum Reiter wechseln, dessen Name `teil` enthaelt - und warten, bis er vorn ist.
Schritt zuReiter(const std::string& teil) {
    return {"Reiter " + teil, [teil](int b) {
                const auto vorn = [&] { return mb::reiterName(static_cast<std::size_t>(g_app->activeTab)).find(teil) != std::string::npos; };
                if (b == 0) {
                    for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
                        if (mb::reiterName(i).find(teil) != std::string::npos) { g_app->tabRequest = static_cast<int>(i); }
                    }
                    return false;
                }
                if (vorn() && b > 3) { return true; }
                if (b > 300) {
                    meldeFehler("Reiter " + teil + " kommt nicht nach vorn (vorn: " +
                                mb::reiterName(static_cast<std::size_t>(g_app->activeTab)) + ")");
                    return true;
                }
                return false;
            }};
}

// Nach einer Aenderung am Skript: Baum, Szene und Bild neu.
void skriptNeu() {
    rebuildTree();
    g_app->camTrackValid = false;
    g_app->mapDirty = true;
}

}  // namespace hb

#include "selbsttest_karte8c.inc"
#include "selbsttest_karte8g.inc"
#include "selbsttest_karte9.inc"
#include "selbsttest_optsicher.inc"

// --- Stufe 3 Punkt 8: Bilder der Seite "Karte bauen" ------------------------------
//
// Teil von karteneditorHandbuchSchritte; allein mit BHED_HB_NUR=kartebauen.
// Die Testkarten raum / raum_leck (stufe3::raumKarte) im Laufordner,
// bhed_q3map2 wie im Selbsttest der Stufe 3.
// Fuer die Bilder: die Testkarte mit Texturen des Spiels statt test/* (die
// es nicht gibt - "shader not found"): Boden, Decke, Waende aus einem Ordner
// wie imperial, nur Bilder ohne Durchsicht.
void handbuchRaumTexturen() {
    namespace ke = karteneditor;
    ke::Editor& e = ke::editor();
    if (!e.karteSpiel) { return; }
    const karte::Texturinventar& inv = e.karteSpiel->inventar;
    std::vector<std::string> wahl;
    for (const char* dir : {"imperial", "kejim", "yavin", "doomgiver", "hoth", "vjun"}) {
        for (const karte::ShaderEintrag& s : inv.verzeichnis(dir)) {
            if ((s.flags & (karte::QER_TRANS | karte::QER_SKY | karte::QER_ALPHATEST | karte::QER_FOG)) != 0U) { continue; }
            if (!inv.bildMass(s.name).gefunden) { continue; }
            wahl.push_back(s.name);
        }
        if (wahl.size() >= 3) { break; }
        wahl.clear();
    }
    if (wahl.size() < 3) { return; }
    const auto finde = [&](const char* teil, std::size_t sonst) {
        for (const std::string& n : wahl) {
            if (n.find(teil) != std::string::npos) { return n; }
        }
        return wahl[sonst % wahl.size()];
    };
    const std::string boden = finde("floor", 0);
    const std::string decke = finde("ceil", 1);
    const std::string wand = finde("wall", 2);
    std::size_t i = 0;
    for (const auto& ent : e.karte.entitaeten()) {
        const bool welt = ent->klassenname() == "worldspawn";
        for (const auto& p : ent->kinder()) {
            if (p->art() == karte::Primitiv::Art::Patch) {   // der Bogen: wie der Boden
                static_cast<karte::Patch&>(*p).setzeShader(boden);
                continue;
            }
            const std::string& tex = !welt ? wand : (i == 0 ? boden : (i == 1 ? decke : wand));
            for (const auto& f : std::static_pointer_cast<karte::Pinsel>(p)->flaechen()) { f->setzeShader(tex, e.karte.kontext); }
            if (welt) { ++i; }
        }
    }
    e.karte.undo.start();   // als geaendert: Bauen speichert vorher
    e.karte.undo.finish("textureNameSetSelected");
    ++e.sichtStand;
}

std::vector<Schritt> karteBauenHandbuchBilder() {
    namespace ke = karteneditor;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto editor = [] {
        ImGuiWindow* w = karteFenster();
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        if (w == nullptr) { return ImRect{}; }
        const float oben = haupt != nullptr ? haupt->Pos.y : w->Pos.y;
        const float links = haupt != nullptr ? haupt->Pos.x : w->Pos.x;
        return ImRect{links, oben, w->Rect().Max.x, w->Rect().Max.y};
    };
    const auto fertig = [](const char* was) { return warteBis(was, [] { return !ke::bauLaeuft(); }, 4000); };
    add(tu("Handbuch Karte bauen: bhed_q3map2, Build-Menue wie vorgegeben, Testkarte raum", [] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();
        ke::Editor& e = ke::editor();
        e.bauProjektFuer = -2;
        e.bauGeaendert = false;
        ke::bauMenueSicherstellen();
        e.bau = ke::Editor::BauEinstellungen{};
        stufe3::raumKarteOeffnen("raum", false);
        handbuchRaumTexturen();
        e.kamera.setzeUrsprung(karte::Vektor3f(-460, 200, 150));
        e.kamera.setzeWinkel(karte::Vektor3f(-18, -28, 0));
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        e.xy[0].ansicht.setzeMassstab(0.6F);
        ke::konsoleLeeren();
        ++e.sichtStand;
    }));
    add(warteBis("Handbuch Karte bauen: Texturen geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    add(pause(0.4));
    // Das Build-Menue
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.3));
    add(hb::fotoTeil("h_kartebauen_menue", [] {
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        const ImRect m = obersterPopupRahmen();
        if (haupt == nullptr) { return m; }
        return ImRect{haupt->Pos.x, haupt->Pos.y, m.Max.x + 220.0F, m.Max.y + 20.0F};
    }));
    add(tu("Menue zu, Build > Customize...", [] {
        karteDialogeZu();
        (void)ke::ausfuehren("BuildMenuCustomize");
    }));
    add(pause(0.4));
    add(klickAn("Eintrag [Final] Fast", [] { return ke::bauDialogZeile(1, 15); }));
    add(pause(0.3));
    add(hb::fotoTeil("h_kartebauen_customize", [] { return fensterRahmen("###kartebaumenue"); }));
    add(tu("Dialog zu (Cancel)", [] { karteDialogeZu(); }));
    add(pause(0.3));
    // Bauen: [Final] Fast, danach die Konsole
    add(tu("Build > [Final] Fast", [] {
        for (std::size_t i = 0; i < ke::editor().bauProjekt.eintraege.size(); ++i) {
            if (ke::editor().bauProjekt.eintraege[i].name == "[Final] Fast") { ke::bauStarten(i); }
        }
    }));
    add(pause(0.15));
    add(hb::fotoTeil("h_kartebauen_status", editor));
    add(fertig("Handbuch Karte bauen: gebaut"));
    add(pause(0.3));
    add(hb::fotoTeil("h_kartebauen_konsole", editor));
    // Die Seite Build mit dem Ziel-Ordner
    add(tu("Einstellungen, Seite Build", [] { ke::editor().einstellungenOffen = true; }));
    add(pause(0.3));
    add(klickAn("Einstellungen: Seite Build", [] {
        const Element* el = findeEndung("##seite19", "###karteeinstellungen");
        return el != nullptr ? el->rect.GetCenter() : ImVec2{5, 5};
    }));
    add(pause(0.3));
    add(hb::fotoTeil("h_kartebauen_einstellungen", [] { return fensterRahmen("###karteeinstellungen"); }));
    add(tu("Einstellungen zu", [] { karteDialogeZu(); }));
    add(pause(0.2));
    // Ein Leck: raum_leck, bsp stage, die Linie; Next leak spot
    add(tu("Testkarte raum_leck, Build > Single: bsp stage", [] {
        ke::Editor& e = ke::editor();
        stufe3::raumKarteOeffnen("raum_leck", true);
        handbuchRaumTexturen();
        ke::konsoleLeeren();
        e.bauRueckmeldungen.clear();
        ke::bauStarten(0);
    }));
    add(fertig("Handbuch Karte bauen: Leck"));
    add(tu("2D-Ansicht auf die Linie, Next leak spot", [] {
        ke::Editor& e = ke::editor();
        if (!e.punktdatei.empty()) {
            e.xy[0].ansicht.setzeUrsprung((e.punktdatei.front() + e.punktdatei.back()) * 0.5F);
            e.xy[0].ansicht.setzeMassstab(0.5F);
        }
        ke::leckWeiter();
        // das Q3Map debug window ueber den Texturbrowser (rechts unten)
        if (const ImGuiWindow* k = karteFenster()) {
            ImGui::SetWindowPos("###kartedbgdlg", ImVec2{k->Pos.x + k->Size.x * 0.58F, k->Pos.y + k->Size.y * 0.66F});
        }
        ++e.sichtStand;
    }));
    add(pause(0.5));
    add(hb::fotoTeil("h_kartebauen_leck", editor));
    // Portale: raum, bsp stage, Load .prt (Show 2D an), dann die Einstellungen
    add(tu("Testkarte raum, Build > Single: bsp stage; Leck-Linie aus", [] {
        ke::Editor& e = ke::editor();
        e.bauRueckmeldungOffen = false;
        stufe3::raumKarteOeffnen("raum", false);
        handbuchRaumTexturen();
        ke::konsoleLeeren();
        ke::bauStarten(0);
    }));
    add(fertig("Handbuch Karte bauen: Portale"));
    add(tu("Plugins > prtview > Load .prt file (Show 2D an)", [] {
        ke::Editor& e = ke::editor();
        ke::portalEinstellungenLesen();
        e.portalE.zeige2d = true;
        (void)ke::ausfuehren("prtview::Load.prtFile");
    }));
    add(pause(0.4));
    add(hb::fotoTeil("h_kartebauen_prtladen", [] { return fensterRahmen("###karteprtladen"); }));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##prtok", "###karteprtladen"));
    add(tu("Kamera vor den Durchgang", [] {
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(karte::Vektor3f(330, -40, 110));
        e.kamera.setzeWinkel(karte::Vektor3f(-8, 170, 0));
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        e.xy[0].ansicht.setzeMassstab(0.6F);
        ++e.sichtStand;
    }));
    add(pause(0.5));
    add(hb::fotoTeil("h_kartebauen_portale", editor));
    add(tu("Configure Portal Viewer", [] { (void)ke::ausfuehren("prtview::ConfigurePortalViewer"); }));
    add(pause(0.4));
    add(hb::fotoTeil("h_kartebauen_prtview", [] { return fensterRahmen("###karteprtkonfig"); }));
    add(tu("Handbuch Karte bauen: Ende - Portale weg, Dialoge zu", [] {
        ke::Editor& e = ke::editor();
        karteDialogeZu();
        e.portale.clear();
        e.portalE.zeige2d = false;
        e.bauRueckmeldungOffen = false;
        ke::punktdateiLeeren();
    }));
    add(pause(0.3));
    return s;
}

// --- Punkt 13: Bilder des Karten-Editors fuer das Handbuch -----------------------
//
// BHED_EDITORTEST=handbuch mit BHED_HB_NUR=karteneditor: nur diese Bilder.
// BHED_HB_KARTE = eine .map ohne Benutzerpfad (z. B. Y:\...\mp_duel1.map aus
// q3map2 -convert); ohne sie die kleine Testkarte. Die Beschriftungen in der
// Sprache des Laufs (BHED_SPRACHE), "Begriffe wie NetRadiant" aus.
std::vector<Schritt> karteneditorHandbuchSchritte(bool nurBauen = false) {
    namespace ke = karteneditor;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const char* kp = std::getenv("BHED_HB_KARTE");
    const std::string karte = kp != nullptr ? kp : "";
    add(tu("Handbuch Karten-Editor: Spielordner, Begriffe aus", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
        ke::setzeNrcBegriffe(false);
    }));
    add(pause(0.3));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(pause(0.3));
    add(warteBis("Handbuch Karten-Editor: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("Handbuch Karten-Editor: Karte oeffnen", [karte] {
        if (!karte.empty()) {
            (void)ke::karteOeffnen(karte);
            return;
        }
        karte::Karte k;
        k.klassen = &ke::editor().spiel->klassen;
        auto welt = k.weltHolenOderAnlegen();
        auto pinsel = std::make_shared<karte::Pinsel>();
        karte::TexturProjektion proj;
        karte::texdefStandard(karte::TexdefArt::Quake, proj);
        pinsel->quaderBauen(karte::Kasten(karte::Vektor3f(0, 0, 32), karte::Vektor3f(64, 64, 32)), "textures/common/caulk", proj, k.kontext);
        welt->kindEinfuegen(pinsel);
        auto licht = k.neueEntitaet("light", false);
        licht->setzen("origin", "0 0 128");
        k.entitaetEinfuegen(licht);
        (void)karte::karteSpeichern(karteTestPfad("handbuch.map"), k, false);
        (void)ke::karteOeffnen(karteTestPfad("handbuch.map"));
    }));
    add(warteBis("Handbuch Karten-Editor: Texturen geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    // Die Maus weg von der Modusleiste (sonst steht ihr Tooltip im Bild)
    add(tu("Maus auf die Statuszeile", [] {
        if (ImGuiWindow* w = karteFenster()) { setzeMaus(ImVec2{w->Pos.x + w->Size.x * 0.6F, w->Pos.y + w->Size.y - 4.0F}); }
    }));
    add(pause(0.5));
    if (nurBauen) {   // BHED_HB_NUR=kartebauen: nur die Seite "Karte bauen"
        for (Schritt& x : karteBauenHandbuchBilder()) { s.push_back(std::move(x)); }
        add(tu("Handbuch Karten-Editor: Ende", [] {
            karteDialogeZu();
            g_app->leftMode = 0;
        }));
        add(pause(0.3));
        return s;
    }
    // Seit Stufe 3 Punkt 0b steht die Menueleiste im Programmfenster: das Bild
    // nimmt sie und die Modusleiste mit (vom linken Fensterrand bis zum Editor).
    const auto editor = [] {
        ImGuiWindow* w = karteFenster();
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        if (w == nullptr) { return ImRect{}; }
        const float oben = haupt != nullptr ? haupt->Pos.y : w->Pos.y;
        const float links = haupt != nullptr ? haupt->Pos.x : w->Pos.x;
        return ImRect{links, oben, w->Rect().Max.x, w->Rect().Max.y};
    };
    add(hb::fotoTeil("h_karteneditor", editor));
    // Ein Menue offen (View)
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(hb::fotoTeil("h_karteneditor_menue", [] {
        ImGuiWindow* w = karteFenster();
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        const ImRect m = obersterPopupRahmen();
        if (w == nullptr || haupt == nullptr) { return m; }
        return ImRect{haupt->Pos.x, haupt->Pos.y, std::max(m.Max.x, w->Pos.x + m.GetWidth()) + 40.0F, m.Max.y + 20.0F};
    }));
    add(tu("Menue zu", [] { karteDialogeZu(); }));
    add(pause(0.2));
    // Entity Inspector mit der ersten Punktentitaet, deren Klasse eine
    // Beschreibung hat (sonst der ersten Punktentitaet; die Klassen aus den
    // Movie-Duels-Definitionen haben oft nur eine Leerzeile)
    add(tu("Handbuch Karten-Editor: eine Punktentitaet waehlen, Entity Inspector", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        std::shared_ptr<karte::Entitaet> wahl;
        for (const auto& ent : e.karte.entitaeten()) {
            if (ent->istBehaelter() || ent->klassenname() == "worldspawn") { continue; }
            if (wahl == nullptr) { wahl = ent; }
            if (ent->klasse() != nullptr && !ent->klasse()->unknown && ent->klasse()->kommentar.size() >= 40) {
                wahl = ent;
                break;
            }
        }
        if (wahl != nullptr) { e.karte.auswahl.waehlen(wahl); }
        e.gruppenSeite = 0;
        e.gruppenSeiteWechseln = true;
        e.gruppenDialogOffen = true;
    }));
    add(pause(0.4));
    // Der Gruppendialog (444 x 777 wie NRC) ist bei grosser Skalierung hoeher
    // als der Bildschirm - fuer das Bild so hoch, dass die Reiter unten zu sehen sind.
    add(tu("Handbuch Karten-Editor: Gruppendialog auf den Bildschirm", [] {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const ImRect r = fensterRahmen("###kartegruppe");
        const float h = std::min(r.GetHeight(), vp->WorkSize.y - 80.0F);
        ImGui::SetWindowPos("###kartegruppe", ImVec2{r.Min.x, vp->WorkPos.y + 40.0F});
        ImGui::SetWindowSize("###kartegruppe", ImVec2{r.GetWidth(), h});
    }));
    add(pause(0.4));
    add(hb::fotoTeil("h_karteneditor_entities", [] { return fensterRahmen("###kartegruppe"); }));
    add(pruefe("Gruppendialog: die Reiter zeichnen nur in ihrem Fenster", [] {
        std::string bericht;
        const bool ok = zeichnungImFenster("/##gruppeinnen_", &bericht);
        diag::detail("Knopftest: Gruppendialog - " + bericht);
        return ok;
    }));
    // Der Texturbrowser: in der Anordnung Regular ein Feld des Hauptfensters
    add(tu("Handbuch Karten-Editor: Gruppendialog zu", [] { ke::editor().gruppenDialogOffen = false; }));
    add(warteBis("Handbuch Karten-Editor: Vorschaubilder geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    add(pause(0.5));
    add(hb::fotoTeil("h_karteneditor_texturen", [] {
        for (ImGuiWindow* w : GImGui->Windows) {
            if ((w->Active || w->WasActive) && std::strstr(w->Name, "/##feldTexturen_") != nullptr) { return w->Rect(); }
        }
        return ImRect{};
    }));
    add(tu("Gruppendialog zu, Auswahl leer, Einstellungen auf", [] {
        ke::Editor& e = ke::editor();
        e.gruppenDialogOffen = false;
        e.karte.auswahl.leeren();
        e.einstellungenOffen = true;
    }));
    add(pause(0.3));
    add(klickAn("Einstellungen: Seite Layout", [] {
        const Element* el = findeEndung("##seite5", "###karteeinstellungen");
        return el != nullptr ? el->rect.GetCenter() : ImVec2{5, 5};
    }));
    add(pause(0.3));
    add(hb::fotoTeil("h_karteneditor_einstellungen", [] { return fensterRahmen("###karteeinstellungen"); }));
    add(tu("Einstellungen zu, Shortcuts auf", [] {
        karteDialogeZu();
        (void)ke::ausfuehren("Shortcuts");
    }));
    add(pause(0.4));
    add(hb::fotoTeil("h_karteneditor_tasten", [] { return fensterRahmen("###kartetasten"); }));
    // --- Stufe 2: die Bauwerkzeuge
    // ein Pinsel nahe der Mitte der 2D-Ansicht (32..256 gross)
    const auto pinselInDerMitte = [] {
        ke::Editor& e = ke::editor();
        const karte::Vektor3f o = e.xy[0].ansicht.ursprung;
        std::shared_ptr<karte::Primitiv> bester;
        float abstand = 1e30F;
        for (const auto& k : e.karte.welt()->kinder()) {
            if (k->art() != karte::Primitiv::Art::Pinsel) { continue; }
            const karte::Kasten b = std::static_pointer_cast<karte::Pinsel>(k)->grenzen(e.karte.kontext);
            if (b.extents[0] < 16 || b.extents[1] < 16 || b.extents[0] > 128 || b.extents[1] > 128 || b.extents[2] < 8) { continue; }
            const float d = std::fabs(b.origin[0] - o[0]) + std::fabs(b.origin[1] - o[1]);
            if (d < abstand) {
                abstand = d;
                bester = k;
            }
        }
        e.karte.auswahl.leeren();
        if (bester) { e.karte.auswahl.waehlen(e.karte.welt(), bester); }
        ke::auswahlGeaendert();
        return bester;
    };
    add(tu("Handbuch Karten-Editor: Dialoge zu, ein Pinsel in der Mitte, Ecken-Modus", [pinselInDerMitte] {
        karteDialogeZu();
        (void)pinselInDerMitte();
        (void)ke::ausfuehren("DragVertices");
    }));
    add(pause(0.5));
    add(hb::fotoTeil("h_karteneditor_ecken", editor));
    add(tu("Ecken-Modus aus, Clipper mit drei Punkten durch den Pinsel", [] {
        ke::Editor& e = ke::editor();
        (void)ke::ausfuehren("DragVertices");
        (void)ke::ausfuehren("ToggleClipper");
        karte::Kasten b;
        if (ke::auswahlGrenzen(b)) {
            const karte::Vektor3d c(b.origin[0], b.origin[1], b.origin[2] + b.extents[2]);
            e.bearbeitung.clipperPunkte.punkte[0] = c + karte::Vektor3d(-b.extents[0], -b.extents[1] * 0.5, 0);
            e.bearbeitung.clipperPunkte.punkte[1] = c + karte::Vektor3d(b.extents[0], b.extents[1] * 0.5, 0);
            e.bearbeitung.clipperPunkte.punkte[2] = c + karte::Vektor3d(b.extents[0], b.extents[1] * 0.5, -64);
            e.bearbeitung.clipperPunkte.anzahl = 3;
        }
    }));
    add(pause(0.5));
    add(hb::fotoTeil("h_karteneditor_clipper", editor));
    add(tu("Clipper aus, Brush > CSG > CSG Tool", [] {
        (void)ke::ausfuehren("ToggleClipper");
        (void)ke::ausfuehren("CSGTool");
    }));
    add(pause(0.4));
    add(hb::fotoTeil("h_karteneditor_csg", [] { return fensterRahmen("###kartecsgtool"); }));
    add(tu("CSG Tool zu, Brush > Prism...", [] {
        karteDialogeZu();
        ke::editor().csgWerkzeug.offen = false;
        (void)ke::ausfuehren("BrushPrism");
    }));
    add(pause(0.4));
    add(hb::fotoTeil("h_karteneditor_seiten", [] { return fensterRahmen("###karteseiten"); }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tu("Curve > Cylinder aus dem Pinsel", [pinselInDerMitte] {
        (void)pinselInDerMitte();
        (void)ke::ausfuehren("PatchCylinder");
    }));
    add(pause(0.5));
    add(hb::fotoTeil("h_karteneditor_patch", editor));
    add(tu("Rueckgaengig, den Pinsel waehlen, Surface Inspector", [pinselInDerMitte] {
        (void)ke::ausfuehren("Undo");
        (void)pinselInDerMitte();
        (void)ke::ausfuehren("SurfaceInspector");
    }));
    add(pause(0.5));
    add(hb::fotoTeil("h_karteneditor_surface", [] { return fensterRahmen("###kartesi"); }));
    add(tu("Surface Inspector zu, Misc > Map Info...", [] {
        (void)ke::ausfuehren("SurfaceInspector");
        ke::editor().karte.auswahl.leeren();
        (void)ke::ausfuehren("MapInfo");
    }));
    add(pause(0.4));
    add(hb::fotoTeil("h_karteneditor_karteninfo", [] { return fensterRahmen("###kartekarteninfo"); }));
    add(tu("Map Info zu, View > Layers Browser", [] {
        karteDialogeZu();
        (void)ke::ausfuehren("ToggleLayersBrowser");
    }));
    add(pause(0.4));
    add(tu("Gruppendialog auf den Bildschirm", [] {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const ImRect r = fensterRahmen("###kartegruppe");
        ImGui::SetWindowPos("###kartegruppe", ImVec2{r.Min.x, vp->WorkPos.y + 40.0F});
        ImGui::SetWindowSize("###kartegruppe", ImVec2{r.GetWidth(), std::min(r.GetHeight(), vp->WorkSize.y - 80.0F)});
    }));
    add(pause(0.4));
    add(hb::fotoTeil("h_karteneditor_ebenen", [] { return fensterRahmen("###kartegruppe"); }));
    add(tu("View > Model Browser", [] { (void)ke::ausfuehren("ToggleModelBrowser"); }));
    add(pause(0.5));
    for (const char* p : {"models/", "models/map_objects/"}) {   // die Pfeile aufklappen
        const std::string id = std::string("##mb") + p;
        add(klickAn(std::string("Pfeil vor ") + p, [id]() {
            const Element* el = finde(id, "", 0, true);
            if (el == nullptr) { return ImVec2{0, 0}; }
            const int tiefe = static_cast<int>(std::count(id.begin(), id.end(), '/')) - 1;
            return ImVec2{el->rect.Min.x + (20.0F * static_cast<float>(tiefe) + 8.0F) * ke::massstab(), el->rect.GetCenter().y};
        }));
        add(pause(0.2));
    }
    add(klickAn("Doppelklick auf bespin", stufe2::elementOrt("##mbmodels/map_objects/bespin/"), 0, true));
    add(pause(1.0));
    add(warteBis("Modelle und Texturen geladen", [] { return ke::kameraBilderFertig(); }, 4000));
    add(pause(0.5));
    add(hb::fotoTeil("h_karteneditor_modelle", [] { return fensterRahmen("###kartegruppe"); }));
    add(tu("Gruppendialog zu, Einstellungen auf", [] {
        ke::editor().gruppenDialogOffen = false;
        ke::editor().gruppenSeite = 0;
        ke::editor().einstellungenOffen = true;
    }));
    add(pause(0.3));
    add(klickAn("Einstellungen: Seite Orthographic", [] {
        const Element* el = findeEndung("##seite18", "###karteeinstellungen");
        return el != nullptr ? el->rect.GetCenter() : ImVec2{5, 5};
    }));
    add(pause(0.3));
    add(hb::fotoTeil("h_karteneditor_einstellungen2", [] { return fensterRahmen("###karteeinstellungen"); }));
    add(tu("Einstellungen zu", [] { karteDialogeZu(); }));
    add(pause(0.2));
    add(klick(ke::trK(Str::KeLMisc), "###main"));
    add(pause(0.3));
    add(klickAn("Abreisskante des Menues Misc", [] {
        ImGuiWindow* w = ImGui::FindWindowByName("###Menu_00");
        return w != nullptr ? ImVec2{w->Pos.x + w->Size.x * 0.5F, w->Pos.y + 4.0F * ke::massstab()} : ImVec2{0, 0};
    }));
    add(pause(0.4));
    add(hb::fotoTeil("h_karteneditor_abriss", [] {
        ImGuiWindow* w = karteFenster();
        if (w == nullptr) { return ImRect{}; }
        return ImRect{w->Pos.x, w->Pos.y, w->Pos.x + w->Size.x * 0.75F, w->Pos.y + w->Size.y * 0.55F};
    }));
    add(tu("abgerissenes Fenster zu", [] { ke::editor().abrisse.clear(); }));
    // Stufe 3 Punkt 8: die Seite "Karte bauen"
    for (Schritt& x : karteBauenHandbuchBilder()) { s.push_back(std::move(x)); }
    add(tu("Handbuch Karten-Editor: Ende", [] {
        karteDialogeZu();
        g_app->leftMode = 0;
    }));
    add(pause(0.3));
    return s;
}

// --- Stufe 4 Punkt 6: Bilder der Seiten "Fertige Karten oeffnen" und "Plugins" ---
//
// BHED_EDITORTEST=handbuch mit BHED_HB_NUR=karte4. mp/duel1 aus den .pk3 des
// Spiels (der Arbeitsordner liegt in den Einstellungen - kein Benutzerpfad
// im Bild), danach die Menues und einige Plugin-Dialoge.
std::vector<Schritt> karte4HandbuchSchritte() {
    namespace ke = karteneditor;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto editor = [] {
        ImGuiWindow* w = karteFenster();
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        if (w == nullptr) { return ImRect{}; }
        const float oben = haupt != nullptr ? haupt->Pos.y : w->Pos.y;
        const float links = haupt != nullptr ? haupt->Pos.x : w->Pos.x;
        return ImRect{links, oben, w->Rect().Max.x, w->Rect().Max.y};
    };
    const auto menueBild = [] {
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        ImRect m;
        const ImGuiContext& g = *GImGui;
        for (int i = 0; i < g.OpenPopupStack.Size; ++i) {
            if (const ImGuiWindow* w = g.OpenPopupStack[i].Window) {
                if (i == 0) {
                    m = w->Rect();
                } else {
                    m.Add(w->Rect());
                }
            }
        }
        if (haupt == nullptr) { return m; }
        return ImRect{haupt->Pos.x, haupt->Pos.y, m.Max.x + 40.0F, m.Max.y + 20.0F};
    };
    add(tu("Handbuch Stufe 4: Spielordner, Begriffe aus", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
        ke::setzeNrcBegriffe(false);
    }));
    add(pause(0.3));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(pause(0.3));
    add(warteBis("Handbuch Stufe 4: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("Handbuch Stufe 4: bhed_q3map2, alter Arbeitsordner weg, File > Open .bsp...", [] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(platform::settingsDirectory()) / "karte_bsp" / "duel1", ec);
        ke::bspOeffnenDialog();
    }));
    add(pause(0.6));
    add(klickAn("Filter", stufe2::elementOrt("##bspfilter")));
    add(tippe("duel"));
    add(pause(0.4));
    add(hb::fotoTeil("h_karte4_bspdialog", [] { return fensterRahmen("###kartebspoeffnen"); }));
    add(tu("Handbuch Stufe 4: mp/duel1 oeffnen", [] {
        karteDialogeZu();
        (void)ke::bspOeffnenSpiel("maps/mp/duel1.bsp");
    }));
    add(warteBis("Handbuch Stufe 4: duel1 geladen", [] { return ke::editor().bspHerkunft.aktiv; }, 3000));
    add(stufe8::warteSek("Handbuch Stufe 4: Rueckgewinnung (Stufe 8a) fertig", [] { return ke::bspRueckStand().stand == 0; }, 300));
    add(tu("Handbuch Stufe 4: Ansichten auf die Karte", [] {
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(karte::Vektor3f(830, 180, 110));
        e.kamera.setzeWinkel(karte::Vektor3f(-8, 90, 0));
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        e.xy[0].ansicht.setzeMassstab(0.25F);
        ++e.sichtStand;
    }));
    add(warteBis("Handbuch Stufe 4: Texturen geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    add(tu("Maus auf die Statuszeile", [] {
        if (ImGuiWindow* w = karteFenster()) { setzeMaus(ImVec2{w->Pos.x + w->Size.x * 0.6F, w->Pos.y + w->Size.y - 4.0F}); }
    }));
    add(pause(0.6));
    add(hb::fotoTeil("h_karte4_geladen", editor));
    // Build > Update entities only
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.4));
    add(hb::fotoTeil("h_karte4_nurentities", menueBild));
    add(tu("Menue zu", [] { karteDialogeZu(); }));
    add(pause(0.3));
    // Plugins > bobToolz
    add(klick(ke::trK(Str::KeLPlugins), "###main"));
    add(pause(0.3));
    add(klick("bobToolz", "##Menu_", 0, true));
    add(pause(0.4));
    add(hb::fotoTeil("h_karte4_plugins", menueBild));
    add(tu("Menue zu; Path Plotter", [] {
        karteDialogeZu();
        (void)ke::ausfuehren("bobToolz::PathPlotter");
    }));
    add(pause(0.5));
    add(hb::fotoTeil("h_karte4_pathplotter", [] { return fensterRahmen("###btpathplotter"); }));
    add(tu("Path Plotter zu; MeshTex > Set S/T Scale...", [] {
        ke::editor().bob.dialog = 0;
        karteDialogeZu();
        (void)ke::ausfuehren("meshTex::SetScale");
    }));
    add(pause(0.5));
    add(hb::fotoTeil("h_karte4_meshtex", [] { return fensterRahmen("###mtsetscale"); }));
    add(tu("Set S/T Scale zu; brushexport2 > Export selected as Wavefront Object", [] {
        ke::editor().meshtex.setScaleOffen = false;
        (void)ke::ausfuehren("brushexport2::Export.obj");
    }));
    add(pause(0.5));
    add(hb::fotoTeil("h_karte4_brushexport", [] { return fensterRahmen("###kartebrushexport"); }));
    add(tu("Handbuch Stufe 4: Ende", [] {
        ke::editor().brushExport.offen = false;
        karteDialogeZu();
        g_app->leftMode = 0;
    }));
    add(pause(0.3));
    return s;
}

// --- Stufe 5 Punkt 8: Bilder der Seite "Optimierer und Grenzen" ----------------
//
// BHED_EDITORTEST=handbuch mit BHED_HB_NUR=karte5. mp/duel1 aus den .pk3 des
// Spiels, das Build-Menue mit dem Optimierer, das Fenster mit beiden Reitern,
// die Vorschau einer Behebung und die Einstellungsseite.
std::vector<Schritt> karte5HandbuchSchritte() {
    namespace ke = karteneditor;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto menueBild = [] {
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        ImRect m;
        const ImGuiContext& g = *GImGui;
        for (int i = 0; i < g.OpenPopupStack.Size; ++i) {
            if (const ImGuiWindow* w = g.OpenPopupStack[i].Window) {
                if (i == 0) {
                    m = w->Rect();
                } else {
                    m.Add(w->Rect());
                }
            }
        }
        // der Tooltip des Eintrags (er sagt, dass NetRadiant-Custom keinen Optimierer hat)
        if (const ImGuiWindow* t = ImGui::FindWindowByName("##Tooltip_00"); t != nullptr && t->Active) { m.Add(t->Rect()); }
        if (haupt == nullptr) { return m; }
        return ImRect{haupt->Pos.x, haupt->Pos.y, m.Max.x + 40.0F, m.Max.y + 20.0F};
    };
    const auto fenster = [] { return fensterRahmen("###karteoptimierer"); };
    auto laeufe = std::make_shared<int>(0);
    add(tu("Handbuch Stufe 5: Spielordner, Begriffe aus", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
        ke::setzeNrcBegriffe(false);
    }));
    add(pause(0.3));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(pause(0.3));
    add(warteBis("Handbuch Stufe 5: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("Handbuch Stufe 5: bhed_q3map2, mp/duel1 oeffnen", [] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();
        ke::optimiererSchliessen();
        ke::editor().optimierer = ke::Editor::OptimiererEinstellungen{};
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(platform::settingsDirectory()) / "karte_bsp" / "duel1", ec);
        (void)ke::bspOeffnenSpiel("maps/mp/duel1.bsp");
    }));
    add(warteBis("Handbuch Stufe 5: duel1 geladen", [] { return ke::editor().bspHerkunft.aktiv; }, 3000));
    add(stufe8::warteSek("Handbuch Stufe 5: Rueckgewinnung (Stufe 8a) fertig", [] { return ke::bspRueckStand().stand == 0; }, 300));
    add(warteBis("Handbuch Stufe 5: Texturen geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    // Build > Optimizer...
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.4));
    add(tu("Maus auf Optimizer...", [] {
        if (const Element* el = finde(std::string(ke::trK(Str::KeOptMenue)) + "##bauoptimierer", "", 0, true)) { setzeMaus(el->rect.GetCenter()); }
    }));
    add(pause(0.6));   // bis der Tooltip des Eintrags steht
    add(hb::fotoTeil("h_karte5_menue", menueBild));
    add(tu("Menue zu; Regelwerk Jedi Academy MP (duel1 ist eine MP-Karte), Optimizer oeffnen", [laeufe] {
        karteDialogeZu();
        ke::editor().optimierer.regelwerk = 1;   // karte::GrenzSpiel::JkaMp
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererOeffnen();
    }));
    add(warteBis("Handbuch Stufe 5: Optimierer fertig", [laeufe] { return !ke::optimiererLaeuft() && ke::optimiererBlick().laeufe > *laeufe; }, 3000));
    // Reiter Grenzen, alle Zeilen, die Lightmaps gewaehlt (Quelle und Pruefstelle rechts)
    add(tu("Reiter Grenzen; Nur Probleme aus", [] { ke::optimiererReiter(0); }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeOptNurProbleme)) + "##optnurprobleme", "###karteoptimierer"));
    add(pause(0.3));
    add(tu("Zeile Lightmaps waehlen", [] {
        if (!ke::optimiererZeileWaehlen(0, "renderer.lightmaps@bsp")) { meldeFehler("Zeile renderer.lightmaps@bsp fehlt"); }
    }));
    add(pause(0.6));
    add(hb::fotoTeil("h_karte5_grenzen", fenster));
    // Reiter Pruefungen, verdeckte Flaechen gewaehlt
    add(tu("Reiter Pruefungen", [] { ke::optimiererReiter(1); }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeOptPrVerdeckt)) + "##bfverdeckt", "###karteoptimierer"));
    add(pause(0.6));
    add(hb::fotoTeil("h_karte5_pruefungen", fenster));
    add(klick(std::string(ke::trK(Str::KeOptBeheben)) + "##optbeheben", "###karteoptimierer"));
    add(pause(0.6));
    add(hb::fotoTeil("h_karte5_beheben", fenster));
    // Einstellungen, Seite Optimizer
    add(tu("Optimizer zu; Einstellungen auf", [] {
        ke::optimiererSchliessen();
        ke::editor().einstellungenOffen = true;
    }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KePgOptimizer)) + "##seite23"));
    add(pause(0.6));
    add(hb::fotoTeil("h_karte5_einstellungen", [] { return fensterRahmen("###karteeinstellungen"); }));
    add(tu("Handbuch Stufe 5: Ende", [] {
        karteDialogeZu();
        ke::editor().optimierer = ke::Editor::OptimiererEinstellungen{};
        g_app->leftMode = 0;
    }));
    add(pause(0.3));
    return s;
}

// BHED_EDITORTEST=handbuch mit BHED_HB_NUR=karte7: Stufe 7 - Echtzeit-Licht,
// Schnell-Backen, das Build-Menue mit beiden Eintraegen, die Programm-Einstellungen
// in View, Preferences > Interface und die kleinen Knoepfe.
std::vector<Schritt> karte7HandbuchSchritte() {
    namespace ke = karteneditor;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto menueBild = [] {
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        ImRect m;
        const ImGuiContext& g = *GImGui;
        for (int i = 0; i < g.OpenPopupStack.Size; ++i) {
            if (const ImGuiWindow* w = g.OpenPopupStack[i].Window) {
                if (i == 0) {
                    m = w->Rect();
                } else {
                    m.Add(w->Rect());
                }
            }
        }
        if (const ImGuiWindow* t = ImGui::FindWindowByName("##Tooltip_00"); t != nullptr && t->Active) { m.Add(t->Rect()); }
        if (haupt == nullptr) { return m; }
        return ImRect{haupt->Pos.x, haupt->Pos.y, m.Max.x + 40.0F, m.Max.y + 20.0F};
    };
    const auto kamera = [] { return hb::fensterRect("##feldKamera"); };
    const auto voll = [] { return ImRect{0.0F, 0.0F, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y}; };
    const auto kameraSetzen = [] {
        ke::Editor& e = ke::editor();
        e.kamera.ursprung = karte::Vektor3f(-470.0F, -330.0F, 300.0F);
        e.kamera.winkel = karte::Vektor3f(-28.0F, 38.0F, 0.0F);
    };
    add(tu("Handbuch Stufe 7: Spielordner, Begriffe aus", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
        ke::setzeNrcBegriffe(false);
    }));
    add(pause(0.3));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(pause(0.3));
    add(warteBis("Handbuch Stufe 7: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("Handbuch Stufe 7: Testraum mit Licht und Wand, Echtzeit-Licht an", [kameraSetzen] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();
        stufe7::lichtKarteSchreiben(true);
        (void)ke::karteOeffnen(stufe7::lichtKarte());
        ke::Editor& e = ke::editor();
        e.kameraE.modus = karte::KameraModus::Textur;
        e.echtLicht.an = true;
        e.echtLicht.backen = false;
        kameraSetzen();
    }));
    add(warteBis("Handbuch Stufe 7: Texturen geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    add(pause(0.8));
    add(hb::fotoTeil("h_karte7_licht", kamera));
    // Build-Menue, die Maus auf "Real-time light" (Tooltip)
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.4));
    add(tu("Maus auf Real-time light", [] {
        if (const Element* el = finde(ke::trK(Str::KeLichtMenue), "##Menu_", 0, true)) { setzeMaus(el->rect.GetCenter()); }
    }));
    add(pause(0.8));
    add(hb::fotoTeil("h_karte7_menue", menueBild));
    // Schnell-Backen: die echten Lightmaps
    add(tu("Menue zu, Schnell-Backen an (1 s Pause)", [kameraSetzen] {
        karteDialogeZu();
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = true;
        e.echtLicht.pauseSekunden = 1.0;
        kameraSetzen();
    }));
    add({"Handbuch Stufe 7: Schnell-Backen fertig", [](int b) {
             const ke::Editor& e = ke::editor();
             return e.echtLicht.gebackenZeigen || e.echtLicht.bauStand == 5 || b > 7000;
         }});
    add(pause(0.8));
    add(hb::fotoTeil("h_karte7_gebacken", kamera));
    add(tu("Schnell-Backen und Echtzeit-Licht aus", [] {
        ke::Editor& e = ke::editor();
        e.echtLicht.backen = false;
        e.echtLicht.an = false;
        e.echtLicht.pauseSekunden = 2.0;
    }));
    add(pause(0.3));
    // View mit den Programm-Einstellungen (Stil des Programms)
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.6));
    add(hb::fotoTeil("h_karte7_view", menueBild));
    add(tu("Menue zu; Preferences > Interface", [] {
        karteDialogeZu();
        ke::editor().einstellungenOffen = true;
    }));
    add(pause(0.3));
    add(klick(ke::trK(Str::KePgInterface), "###karteeinstellungen"));
    add(pause(0.6));
    add(hb::fotoTeil("h_karte7_interface", [] { return fensterRahmen("###karteeinstellungen"); }));
    add(tu("Einstellungen zu; Knopfgroesse Klein", [] {
        karteDialogeZu();
        ke::knopfGroesseSetzen(0);
    }));
    add(pause(0.6));
    add(hb::fotoTeil("h_karte7_klein", voll));
    add(tu("Handbuch Stufe 7: Ende, Knopfgroesse Normal", [] {
        ke::knopfGroesseSetzen(1);
        g_app->leftMode = 0;
    }));
    add(pause(0.3));
    return s;
}

// BHED_EDITORTEST=handbuch mit BHED_HB_NUR=karte8: Stufe 8a - das Fortschrittsfenster
// mit Restzeit, das Build-Menue mit den neuen Eintraegen, ein zurueckgewonnenes
// misc_model, geschaetzte Lichter und "Zur fertigen Karte hinzufuegen".
std::vector<Schritt> karte8HandbuchSchritte() {
    namespace ke = karteneditor;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto menueBild = [] {
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        ImRect m;
        const ImGuiContext& g = *GImGui;
        for (int i = 0; i < g.OpenPopupStack.Size; ++i) {
            if (const ImGuiWindow* w = g.OpenPopupStack[i].Window) {
                if (i == 0) {
                    m = w->Rect();
                } else {
                    m.Add(w->Rect());
                }
            }
        }
        if (const ImGuiWindow* t = ImGui::FindWindowByName("##Tooltip_00"); t != nullptr && t->Active) { m.Add(t->Rect()); }
        if (haupt == nullptr) { return m; }
        return ImRect{haupt->Pos.x, haupt->Pos.y, m.Max.x + 40.0F, m.Max.y + 20.0F};
    };
    const auto editor = [] {
        ImGuiWindow* w = karteFenster();
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        if (w == nullptr) { return ImRect{}; }
        const float oben = haupt != nullptr ? haupt->Pos.y : w->Pos.y;
        const float links = haupt != nullptr ? haupt->Pos.x : w->Pos.x;
        return ImRect{links, oben, w->Rect().Max.x, w->Rect().Max.y};
    };
    const auto eintraegeWeg = [] {
        auto& v = ke::editor().bauProjekt.eintraege;
        v.erase(std::remove_if(v.begin(), v.end(), [](const karte::BauEintrag& b) { return b.name.rfind("Handbuch", 0) == 0; }), v.end());
    };
    // Kamera (Ort, Winkel) und die 2D-Ansicht XY um 0 0 im Massstab 0.8
    const auto ansichten = [](karte::Vektor3f ort, karte::Vektor3f winkel) {
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(ort);
        e.kamera.setzeWinkel(winkel);
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        e.xy[0].ansicht.setzeMassstab(0.8F);
        ++e.sichtStand;
    };
    add(tu("Handbuch Stufe 8: Spielordner, Begriffe aus", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
        ke::setzeNrcBegriffe(false);
    }));
    add(pause(0.3));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(pause(0.3));
    add(warteBis("Handbuch Stufe 8: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    // 1. Fortschritt: zweimal bauen, im zweiten Bau die Restzeit aus dem ersten
    add(tu("Handbuch Stufe 8: Testraum, keine Bauzeiten, Eintrag Handbuch", [eintraegeWeg] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();
        ke::bauMenueSicherstellen();
        ke::Editor& e = ke::editor();
        e.bau = ke::Editor::BauEinstellungen{};
        e.bspRueck = ke::Editor::BspRueckEinstellungen{};
        ke::einstellungenSpeichern();
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(stufe8::bauZeitenDatei()), ec);
        stufe3::raumKarteOeffnen("raum8", false);
        ke::konsoleLeeren();
        eintraegeWeg();
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{"Handbuch",
                                                           {"[q3map2] -meta \"[MapFile]\"", "[q3map2] -vis -saveprt \"[MapFile]\"",
                                                            "[q3map2] -light -super 4 -samples 4 -filter -bounce 16 \"[MapFile]\""}});
    }));
    add(stufe3::bauMenuepunkt("Handbuch"));
    add(stufe8::warteSek("Handbuch Stufe 8: erster Bau fertig", [] { return !ke::bauLaeuft(); }, 300));
    add(stufe3::bauMenuepunkt("Handbuch"));
    add(stufe8::warteSek("Handbuch Stufe 8: zweiter Bau im Licht, Bounce 3, Restzeit aus dem ersten", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return st.laeuft && st.schritt == 2 && st.bounce >= 3 && st.schaetzung.ausVerlauf;
    }, 120));
    add(hb::fotoTeil("h_karte8_fortschritt", [] { return fensterRahmen("###kartebaufortschritt"); }));
    add(stufe8::warteSek("Handbuch Stufe 8: zweiter Bau fertig", [] { return !ke::bauLaeuft(); }, 300));
    // 3. misc_model aus der Geometrie: die Testkarte ohne Editor-Daten bauen, als .bsp oeffnen
    add(tu("Handbuch Stufe 8: Testkarte mit misc_model ohne Editor-Daten bauen", [eintraegeWeg] {
        karteDialogeZu();
        ke::Editor& e = ke::editor();
        stufe8::edKarteSchreiben();
        (void)ke::karteOeffnen(stufe8::edKarte());
        std::error_code ec;
        for (const char* x : {".bsp", ".prt", ".srf", ".bhed", ".bhedvis", ".bhedlight"}) {
            std::filesystem::remove(std::filesystem::path(stufe8::edStamm() + x), ec);
        }
        eintraegeWeg();
        e.bauProjekt.eintraege.push_back(
            karte::BauEintrag{"Handbuch Modelle", {"[q3map2] -meta \"[MapFile]\"", "[q3map2] -vis -fast \"[MapFile]\"", "[q3map2] -light -fast \"[MapFile]\""}});
    }));
    // Build-Menue mit den neuen Eintraegen (Editor-Daten behalten und die Teil-Kompilierung an)
    add(tu("Handbuch Stufe 8: Editor-Daten behalten, VIS und Licht wiederverwenden an", [ansichten] {
        ke::Editor& e = ke::editor();
        e.bau.editorDaten = true;
        e.bau.visWieder = true;
        e.bau.lichtWieder = true;
        ansichten(karte::Vektor3f(-330.0F, -300.0F, 230.0F), karte::Vektor3f(-22.0F, 45.0F, 0.0F));
    }));
    add(warteBis("Handbuch Stufe 8: Texturen geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.6));
    add(hb::fotoTeil("h_karte8_menue", menueBild));
    add(tu("Menue zu, die Optionen wieder aus (die Rueckgewinnung braucht eine .bsp ohne Editor-Daten)", [] {
        karteDialogeZu();
        ke::editor().bau = ke::Editor::BauEinstellungen{};
    }));
    add(pause(0.3));
    add(stufe3::bauMenuepunkt("Handbuch Modelle"));
    add(stufe8::warteSek("Handbuch Stufe 8: Bau fertig", [] { return !ke::bauLaeuft(); }, 300));
    add(tu("Handbuch Stufe 8: File > Open .bsp - misc_model zurueckgewinnen", [] {
        ke::konsoleLeeren();
        ke::editor().bspRueckLetzt = ke::Editor::BspRueckLetzt{};
        (void)ke::bspOeffnenDatei(stufe8::edStamm() + ".bsp");
    }));
    add(stufe8::warteSek("Handbuch Stufe 8: Rueckgewinnung fertig",
                         [] { return ke::bspRueckStand().stand == 0 && ke::editor().bspRueckLetzt.sekunden > 0; }, 180));
    add(tu("Handbuch Stufe 8: Ansichten auf die Saeule", [ansichten] { ansichten(karte::Vektor3f(-90.0F, -90.0F, 130.0F), karte::Vektor3f(-8.0F, 45.0F, 0.0F)); }));
    add(warteBis("Handbuch Stufe 8: Texturen geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    add(pause(0.8));
    add(hb::fotoTeil("h_karte8_modelle", editor));
    // 4. Lichter schaetzen
    add(tu("Handbuch Stufe 8: noch einmal oeffnen, nur Lichter schaetzen", [] {
        ke::Editor& e = ke::editor();
        e.bspRueck.modelle = false;
        e.bspRueck.lichter = true;
        e.bspRueckLetzt = ke::Editor::BspRueckLetzt{};
        ke::konsoleLeeren();
        (void)ke::bspOeffnenDatei(stufe8::edStamm() + ".bsp");
    }));
    add(stufe8::warteSek("Handbuch Stufe 8: Schaetzung fertig", [] {
        return ke::bspRueckStand().stand == 0 && stufe3::konsoleText().find(stufe8::vorlageAnfang(Str::KeLsFertig)) != std::string::npos;
    }, 120));
    add(tu("Handbuch Stufe 8: Ansichten", [ansichten] { ansichten(karte::Vektor3f(-330.0F, -300.0F, 230.0F), karte::Vektor3f(-22.0F, 45.0F, 0.0F)); }));
    add(pause(0.8));
    add(hb::fotoTeil("h_karte8_lichter", [] { return hb::vereint(hb::fensterRect("##feldXY"), hb::fensterRect("##feldKonsole")); }));
    // 5. Zur fertigen Karte hinzufuegen: die gebaute Testkarte als Vorlage, eine Saeule in der Ebene "Neu"
    auto ebene = std::make_shared<int>(-1);
    add(tu("Handbuch Stufe 8: Quelle oeffnen, Ebene \"Neu\" mit einer Saeule, Dialog mit der gebauten Karte als Vorlage", [ebene, ansichten] {
        ke::Editor& e = ke::editor();
        e.bspRueck = ke::Editor::BspRueckEinstellungen{};
        ke::einstellungenSpeichern();
        (void)ke::karteOeffnen(stufe8::edKarte());
        karte::Karte& k = e.karte;
        (void)karte::ebeneNeu(k, -1, "Neu", ebene.get());
        k.aktuelleSchicht = static_cast<std::size_t>(*ebene);
        karte::TexturProjektion proj;
        karte::texdefStandard(karte::TexdefArt::Quake, proj);
        auto p = std::make_shared<karte::Pinsel>();
        p->quaderBauen(karte::Kasten(karte::Vektor3f(64, -128, 64), karte::Vektor3f(32, 32, 64)), "textures/yavin/temple_stone2", proj, k.kontext);
        p->schicht = *ebene;
        k.weltHolenOderAnlegen()->kindEinfuegen(p);
        ke::zusammenDialogOeffnen();
        ke::zusammenVorlageWaehlenDatei(stufe8::edStamm() + ".bsp");
        ansichten(karte::Vektor3f(-330.0F, -300.0F, 230.0F), karte::Vektor3f(-22.0F, 45.0F, 0.0F));
    }));
    add(warteBis("Handbuch Stufe 8: Vorlage geladen", [] { return ke::vorlageAktiv() && ke::kameraBilderFertig(); }, 6000));
    add(pause(1.0));
    add(hb::fotoTeil("h_karte8_zusammen", editor));
    add(tu("Handbuch Stufe 8: Bauen und hinzufuegen", [ebene] {
        ke::konsoleLeeren();
        ke::zusammenStarten(*ebene);
    }));
    add(stufe8::warteSek("Handbuch Stufe 8: zusammengefuehrt", [] { return ke::zusammenStand().stand == 2 || ke::zusammenStand().stand == 3; }, 300));
    add(pause(0.5));
    add(hb::fotoTeil("h_karte8_zusammen_fertig", [] { return fensterRahmen("###kartezusammen"); }));
    add(tu("Handbuch Stufe 8: Ende", [eintraegeWeg] {
        ke::vorlageEntfernen();
        karteDialogeZu();
        ke::Editor& e = ke::editor();
        e.bau = ke::Editor::BauEinstellungen{};
        e.bspRueck = ke::Editor::BspRueckEinstellungen{};
        ke::einstellungenSpeichern();
        eintraegeWeg();
        g_app->leftMode = 0;
    }));
    add(pause(0.3));
    return s;
}

// BHED_EDITORTEST=handbuch mit BHED_HB_NUR=karte8b: Stufe 8b - das View-Menue mit den
// neuen Eintraegen, Terrain Generator, Logik-Ansicht, Ansichtsmodi, Spiegel-Modus,
// Undo-Verlauf, Im Editor spielen.
std::vector<Schritt> karte8bHandbuchSchritte() {
    namespace ke = karteneditor;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const auto menueBild = [] {
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        ImRect m;
        const ImGuiContext& g = *GImGui;
        for (int i = 0; i < g.OpenPopupStack.Size; ++i) {
            if (const ImGuiWindow* w = g.OpenPopupStack[i].Window) {
                if (i == 0) {
                    m = w->Rect();
                } else {
                    m.Add(w->Rect());
                }
            }
        }
        if (haupt == nullptr) { return m; }
        return ImRect{haupt->Pos.x, haupt->Pos.y, m.Max.x + 40.0F, m.Max.y + 20.0F};
    };
    const auto editor = [] {
        ImGuiWindow* w = karteFenster();
        const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
        if (w == nullptr) { return ImRect{}; }
        const float oben = haupt != nullptr ? haupt->Pos.y : w->Pos.y;
        const float links = haupt != nullptr ? haupt->Pos.x : w->Pos.x;
        return ImRect{links, oben, w->Rect().Max.x, w->Rect().Max.y};
    };
    const auto kamera = [] { return hb::fensterRect("##feldKamera"); };
    const auto ansichten = [](karte::Vektor3f ort, karte::Vektor3f winkel, float massstab) {
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(ort);
        e.kamera.setzeWinkel(winkel);
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        e.xy[0].ansicht.setzeMassstab(massstab);
        ++e.sichtStand;
    };
    add(tu("Handbuch Stufe 8b: Spielordner, Begriffe aus", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
        ke::setzeNrcBegriffe(false);
    }));
    add(pause(0.3));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(pause(0.3));
    add(warteBis("Handbuch Stufe 8b: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    // Testkarte: ein Hof mit Saeulen, Ausloeser -> Relais -> Tuer, ein NPC_spawner mit Skripten
    add(tu("Handbuch Stufe 8b: Testkarte schreiben und oeffnen", [ansichten] {
        karteDialogeZu();
        using stufe4::kastenText;
        std::string w;
        w += kastenText(-512, -512, -16, 512, 512, 0, "yavin/temple_stone2");
        w += kastenText(-528, -512, 0, -512, 512, 256, "yavin/temple_stone2");
        w += kastenText(512, -512, 0, 528, 512, 256, "yavin/temple_stone2");
        w += kastenText(-512, -528, 0, 512, -512, 256, "yavin/temple_stone2");
        w += kastenText(-512, 512, 0, 512, 528, 256, "yavin/temple_stone2");
        w += kastenText(160, -96, 0, 224, -32, 192, "yavin/temple_stone2");    // Saeule (Spiegel)
        w += kastenText(-480, 0, 0, -32, 448, 16, "yavin/temple_stone2");      // Platte (Terrain)
        w += kastenText(-512, -512, 256, 512, 512, 272, "skies/normallight");
        std::string t = stufe4::weltText(w);
        t += "{\n\"classname\" \"trigger_multiple\"\n\"target\" \"relais\"\n" + kastenText(-64, -400, 0, 64, -300, 96, "system/trigger") + "}\n";
        t += "{\n\"classname\" \"target_relay\"\n\"targetname\" \"relais\"\n\"target\" \"tor\"\n\"target2\" \"wache\"\n\"origin\" \"0 -250 32\"\n}\n";
        t += "{\n\"classname\" \"func_door\"\n\"targetname\" \"tor\"\n\"angle\" \"-1\"\n" + kastenText(-48, 480, 0, 48, 496, 128, "yavin/temple_stone2") + "}\n";
        t += "{\n\"classname\" \"NPC_spawner\"\n\"NPC_type\" \"stormtrooper\"\n\"NPC_targetname\" \"wache\"\n\"spawnscript\" \"handbuch/wache_da\"\n\"deathscript\" \"handbuch/wache_tot\"\n\"origin\" \"0 300 24\"\n\"angle\" \"270\"\n}\n";
        t += "{\n\"classname\" \"light\"\n\"origin\" \"0 0 200\"\n\"light\" \"400\"\n}\n";
        t += "{\n\"classname\" \"info_player_start\"\n\"origin\" \"-300 -300 24\"\n\"angle\" \"45\"\n}\n";
        const std::string pfad = karteTestPfad("hb8b\\hof.map");
        std::filesystem::create_directories(std::filesystem::path(pfad).parent_path());
        {
            std::ofstream f(std::filesystem::path(pfad), std::ios::binary | std::ios::trunc);
            f << t;
        }
        (void)ke::karteOeffnen(pfad);
        ke::Editor& e = ke::editor();
        e.spiegel = karte::SpiegelModus{};
        e.ansichtsmodi = ke::AnsichtsModiZustand{};
        ke::konsoleLeeren();
        ansichten(karte::Vektor3f(-420.0F, -420.0F, 220.0F), karte::Vektor3f(-22.0F, 45.0F, 0.0F), 0.45F);
    }));
    add(warteBis("Handbuch Stufe 8b: Texturen geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    // 1. das View-Menue
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.6));
    add(hb::fotoTeil("h_karte8b_menue", menueBild));
    add(tu("Menue zu", [] { karteDialogeZu(); }));
    add(pause(0.3));
    // 2. Logik-Ansicht
    add(tu("Handbuch Stufe 8b: Logic view", [] { ke::logikZeigen(); }));
    add(pause(0.8));
    add(hb::fotoTeil("h_karte8b_logik", [] { return fensterRahmen("###kartelogik"); }));
    add(tu("Logik zu", [] { ke::editor().logik.offen = false; }));
    // 3. Ansichtsmodi
    add(tu("Handbuch Stufe 8b: Lightmap-Dichte", [ansichten] {
        ke::editor().ansichtsmodi.dichte = true;
        ansichten(karte::Vektor3f(-420.0F, -420.0F, 220.0F), karte::Vektor3f(-22.0F, 45.0F, 0.0F), 0.45F);
    }));
    add(pause(0.8));
    add(hb::fotoTeil("h_karte8b_dichte", kamera));
    add(tu("Handbuch Stufe 8b: Kollision", [] {
        ke::Editor& e = ke::editor();
        e.ansichtsmodi = ke::AnsichtsModiZustand{};
        e.ansichtsmodi.kollision = true;
        ++e.sichtStand;
    }));
    add(pause(0.8));
    add(hb::fotoTeil("h_karte8b_kollision", kamera));
    add(tu("Ansichtsmodi aus", [] {
        ke::editor().ansichtsmodi = ke::AnsichtsModiZustand{};
        ++ke::editor().sichtStand;
    }));
    // 4. Spiegel-Modus: Saeule waehlen, Ebene X durch den Ursprung, um 64 verschieben
    add(tu("Handbuch Stufe 8b: Spiegel-Modus X, Saeule verschieben", [] {
        stufe4::weltWaehlen({5});
        ke::spiegelSetzen(0);
        ke::Editor& e = ke::editor();
        {
            karte::Befehl b(e.karte.undo, "translateTool -xyz 64 0 0");
            e.bearbeitung.verschiebeAuswahl(karte::Vektor3f(64, 0, 0));
        }
        e.kamera.setzeUrsprung(karte::Vektor3f(0.0F, -460.0F, 260.0F));
        e.kamera.setzeWinkel(karte::Vektor3f(-28.0F, 90.0F, 0.0F));
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, -64, 0));
        e.xy[0].ansicht.setzeMassstab(0.9F);
        ++e.sichtStand;
    }));
    add(pause(0.8));
    add(hb::fotoTeil("h_karte8b_spiegel", editor));
    add(tu("Spiegel aus", [] {
        ke::spiegelSetzen(-1);
        ke::editor().karte.auswahl.leeren();
        ke::auswahlGeaendert();
    }));
    // 5. Undo-Verlauf: noch zwei Befehle, dann die Liste
    add(tu("Handbuch Stufe 8b: zwei Befehle, Undo history", [] {
        ke::Editor& e = ke::editor();
        for (const auto& ent : e.karte.entitaeten()) {
            if (ent->klassenname() == "light") {
                karte::Befehl b(e.karte.undo, "entitySetKeyValue -key \"light\" -value \"600\"");
                ent->setzen("light", "600");
            }
        }
        stufe4::weltWaehlen({6});
        {
            karte::Befehl b(e.karte.undo, "translateTool -xyz 0 -32 0");
            e.bearbeitung.verschiebeAuswahl(karte::Vektor3f(0, -32, 0));
        }
        ke::undoListeZeigen();
    }));
    add(pause(0.8));
    add(hb::fotoTeil("h_karte8b_undo", [] { return fensterRahmen("###undoverlauf"); }));
    add(tu("Undo-Liste zu", [] { ke::editor().undoListeOffen = false; }));
    // 6. Terrain Generator: die Platte waehlen, Dialog, Hill erzeugen
    add(tu("Handbuch Stufe 8b: Platte waehlen, Generate Terrain", [] {
        stufe4::weltWaehlen({6});
        ke::Editor& e = ke::editor();
        e.texBro.shader = "textures/yavin/temple_stone2";
        e.terrain.gespeichert = karte::terraingen::Werte{};
        e.terrain.gespeichert.form = 1;            // Hill
        e.terrain.gespeichert.formHoehe = 160;
        e.terrain.gespeichert.quadratIndex = 2;    // 32
        e.terrain.gespeichert.varianz = 16;
        e.terrain.gespeichert.startwertFest = true;
        e.terrain.gespeichert.startwert = 7;
        (void)ke::ausfuehren("terrainGenerator::GenerateTerrain");
    }));
    add(pause(0.8));
    add(hb::fotoTeil("h_karte8b_terrain_dialog", [] { return fensterRahmen("###terraingenerator"); }));
    add(tu("Handbuch Stufe 8b: erzeugen, Dialog zu", [ansichten] {
        ke::terrainErzeugenKnopf();
        ke::terrainSchliessen();
        ke::editor().karte.auswahl.leeren();
        ke::auswahlGeaendert();
        ansichten(karte::Vektor3f(300.0F, -300.0F, 230.0F), karte::Vektor3f(-22.0F, 135.0F, 0.0F), 0.45F);
    }));
    add(pause(1.0));
    add(hb::fotoTeil("h_karte8b_terrain", kamera));
    // 7. Im Editor spielen: an der Kamera starten
    add(tu("Handbuch Stufe 8b: Im Editor spielen", [ansichten] {
        ansichten(karte::Vektor3f(380.0F, -380.0F, 64.0F), karte::Vektor3f(0.0F, 135.0F, 0.0F), 0.45F);
        ke::Editor& e = ke::editor();
        e.spielen.startAnSpawn = false;
        ke::spielenStarten();
    }));
    add(pause(1.2));
    add(hb::fotoTeil("h_karte8b_spielen", kamera));
    add(tu("Handbuch Stufe 8b: Ende", [] {
        ke::spielenBeenden();
        karteDialogeZu();
        g_app->leftMode = 0;
    }));
    add(pause(0.3));
    return s;
}

std::vector<Schritt> handbuchSchritte() {
    // Die Sprache schon JETZT: die Klicks unten tragen ihre Beschriftung
    // (tr) beim Aufbau der Liste - vorher suchten sie auf Deutsch "Cancel".
    if (const char* sp = std::getenv("BHED_SPRACHE")) {
        if (const i18n::LanguageInfo* l = i18n::findLanguage(sp)) { i18n::setLanguage(l->language); }
    }
    if (const char* nur = std::getenv("BHED_HB_NUR"); nur != nullptr && std::strcmp(nur, "karteneditor") == 0) {
        return karteneditorHandbuchSchritte();
    }
    if (const char* nur = std::getenv("BHED_HB_NUR"); nur != nullptr && std::strcmp(nur, "kartebauen") == 0) {
        return karteneditorHandbuchSchritte(true);
    }
    if (const char* nur = std::getenv("BHED_HB_NUR"); nur != nullptr && std::strcmp(nur, "karte4") == 0) {
        return karte4HandbuchSchritte();
    }
    if (const char* nur = std::getenv("BHED_HB_NUR"); nur != nullptr && std::strcmp(nur, "karte5") == 0) {
        return karte5HandbuchSchritte();
    }
    if (const char* nur = std::getenv("BHED_HB_NUR"); nur != nullptr && std::strcmp(nur, "karte7") == 0) {
        return karte7HandbuchSchritte();
    }
    if (const char* nur = std::getenv("BHED_HB_NUR"); nur != nullptr && std::strcmp(nur, "karte8") == 0) {
        return karte8HandbuchSchritte();
    }
    if (const char* nur = std::getenv("BHED_HB_NUR"); nur != nullptr && std::strcmp(nur, "karte8b") == 0) {
        return karte8bHandbuchSchritte();
    }
    // Stufe 8c (Umbenennung): nur das Bild o_hilfe (Menue Hilfe mit "About JK Studio...") -
    // derselbe Ausschnitt und dieselbe Marke wie im grossen Bildlauf (menuR, knopf).
    if (const char* nur = std::getenv("BHED_HB_NUR"); nur != nullptr && std::strcmp(nur, "hilfe") == 0) {
        std::vector<Schritt> t;
        t.push_back(tu("Handbuch Hilfe: Skript-Ansicht", [] { g_app->leftMode = 0; }));
        t.push_back(pause(0.5));
        // Stufe 8d: die Startflaeche (ohne Karte, Ansicht Vorschau) zeigt das Logo von JK Studio ueber dem Titel
        t.push_back(tu("Handbuch Hilfe: Vorschau ohne Karte", [] { g_app->leftMode = 1; }));
        t.push_back(warteBis("Startflaeche: sichtbar, das Logo von JK Studio geladen", [] {
            int b = 0;
            int h = 0;
            return hb::fensterRect("##startflaeche").GetWidth() > 0.0F && jkLogoTextur(b, h) != nullptr && b > 0 && h > 0;
        }, 600));
        t.push_back(hb::fotoTeil("h_start", [] { return hb::fensterRect("##startflaeche"); }));
        t.push_back(tu("Handbuch Hilfe: zurueck zur Skript-Ansicht", [] { g_app->leftMode = 0; }));
        t.push_back(pause(0.5));
        t.push_back(klick(tr(Str::MenuHelp), "###main"));
        t.push_back(pause(0.5));
        t.push_back(hb::fotoTeil("o_hilfe", [] {
            ImRect r{0.0F, 0.0F, 0.0F, 0.0F};
            for (ImGuiWindow* w : GImGui->Windows) {
                if (w->WasActive && !w->Hidden && std::strstr(w->Name, "##Menu") != nullptr) { r = hb::vereint(r, w->Rect()); }
            }
            r.Expand(ImGui::GetFontSize() * 0.6F);
            r.Min.x = std::max(0.0F, r.Min.x - ImGui::GetFontSize() * 2.0F);
            r.Min.y = 0.0F;
            return r;
        }, [] { return std::vector<hb::Marke>{hb::knopf(tr(Str::HelpGuide), "##Menu")}; }));
        t.push_back(klickAn("daneben", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
        t.push_back(pause(0.3));
        // das Ueber-Fenster mit dem Logo von JK Studio, Untertitel, Bereichen und dem Hinweis zur Marke
        t.push_back(tu("Handbuch Hilfe: About", [] { g_app->aboutOpen = true; }));
        t.push_back(pause(0.8));
        t.push_back(pruefe("About: offen, das Logo von JK Studio geladen (Ressource JKLOGO)", [] {
            int b = 0;
            int h = 0;
            return g_app->aboutOpen && jkLogoTextur(b, h) != nullptr && b == 480 && h == 320;
        }));
        t.push_back(hb::fotoTeil("h_ueber", [] { return hb::fensterRect(tr(Str::AboutTitle)); }));
        t.push_back(tu("Handbuch Hilfe: About zu", [] { g_app->aboutOpen = false; }));
        t.push_back(pause(0.3));
        return t;
    }
    if (const char* nur = std::getenv("BHED_HB_NUR"); nur != nullptr && std::strcmp(nur, "tutorial") == 0) {   // Stufe 8c: Erste eigene Karte
        std::vector<Schritt> t;
        t.push_back(tu("Handbuch Tutorial: Karten-Editor", [] { g_app->leftMode = 0; }));
        t.push_back(pause(0.3));
        t.push_back(klick(tr(Str::ViewMapEditor), "###main"));
        t.push_back(pause(0.3));
        for (Schritt& x : stufe8c::tutorial()) { t.push_back(std::move(x)); }
        t.push_back(tu("Handbuch Tutorial: zurueck", [] { g_app->leftMode = 0; }));
        return t;
    }
    if (const char* nur = std::getenv("BHED_HB_NUR"); nur != nullptr && std::strcmp(nur, "gpubacken") == 0) {   // Stufe 8g: GPU-Backen
        std::vector<Schritt> t;
        t.push_back(tu("Handbuch GPU-Backen: Karten-Editor", [] { g_app->leftMode = 0; }));
        t.push_back(pause(0.3));
        t.push_back(klick(tr(Str::ViewMapEditor), "###main"));
        t.push_back(pause(0.3));
        for (Schritt& x : stufe8c::tutorial()) { t.push_back(std::move(x)); }
        for (Schritt& x : stufe8g::schritte()) { t.push_back(std::move(x)); }
        t.push_back(tu("Handbuch GPU-Backen: zurueck", [] { g_app->leftMode = 0; }));
        return t;
    }
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string archiv = std::getenv("BHED_MISSION_ARCHIV") ? std::getenv("BHED_MISSION_ARCHIV") : "";
    const std::string name = std::getenv("BHED_MISSION_NAME") ? std::getenv("BHED_MISSION_NAME") : "md_am_sith";
    const auto voll = [] { return ImRect{0.0F, 0.0F, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y}; };
    const auto kartePlusLeiste = [] {
        ImRect r = hb::karte();
        r.Min.y -= ImGui::GetFontSize() * 2.6F;
        return r;
    };
    const auto zeitleiste = [] { return hb::fensterRect("toolband"); };
    const auto linksUndKarte = [] {
        ImRect r = hb::karte();
        r.Min.x = 0.0F;
        r.Min.y -= ImGui::GetFontSize() * 2.6F;
        return r;
    };
    auto schiff = std::make_shared<int>(-1);
    // Karte und Zeitleiste zusammen - fuer Schritte, die vom Zeitzeiger
    // sprechen (shank: "du erzaehlst von der Timeline, und man sieht die
    // Timeline nicht").
    const auto karteMitZeitleiste = [] {
        const ImRect band = hb::fensterRect("toolband");
        const ImRect k = hb::karte();
        return ImRect{0.0F, k.Min.y - ImGui::GetFontSize() * 2.6F, k.Max.x, band.Max.y};
    };

    // Erkundung einer Karte fuer die Handbuch-Mission (BHED_HB_ERKUNDEN=<datei>):
    // je Zeile "karte maps/x.bsp", "marke name x y z" (ein ref_tag auf dem
    // Boden, seine Hoehe ins Protokoll) oder "name x y z nick gier [oben schnitt]"
    // (ein Foto der Ansicht). Nur Fotos, kein Test.
    if (const char* erk = std::getenv("BHED_HB_ERKUNDEN")) {
        std::ifstream f(::bhed::utf8Pfad(erk));
        std::string zeile;
        add(tu("Erkunden: Ansicht", [] {
            g_app->leftMode = 1;
            g_app->uiScale = 1.0F;
            g_app->zeigeNamen = true;
        }));
        while (std::getline(f, zeile)) {
            std::istringstream is(zeile);
            std::string n;
            if (!(is >> n) || n[0] == '#') { continue; }
            if (n == "karte") {
                std::string k;
                is >> k;
                add(tu("Erkunden: " + k, [k] { (void)loadMapFromArchive(k); }));
                add(warteBis("Erkunden: Karte bereit", [] { return g_app->mapBereit; }, 1500));
                continue;
            }
            if (n == "marke") {
                std::string mn;
                float o[3] = {0, 0, 0};
                is >> mn >> o[0] >> o[1] >> o[2];
                add(tu("Erkunden: Marke " + mn, [mn, o] {
                    const int i = neueEntityAn("ref_tag", o);
                    if (i < 0) { return; }
                    (void)entityKeySetzen(i, "targetname", mn);
                    const bool boden = entityAufBoden(i);
                    float z[3] = {0, 0, 0};
                    (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(i)], z);
                    diag::detail("Knopftest: Marke " + mn + " z " + std::to_string(static_cast<int>(z[2])) + (boden ? "" : " KEIN BODEN"));
                }));
                continue;
            }
            float x = 0, y = 0, z = 0, nick = 0, gier = 0, schnitt = 0;
            int oben = 0;
            is >> x >> y >> z >> nick >> gier >> oben >> schnitt;
            add(tu("Erkunden: " + n, [=] {
                g_app->draufsicht = oben != 0;
                if (oben != 0) { g_app->draufSchnitt = schnitt; }
                g_app->cam.pos[0] = x;
                g_app->cam.pos[1] = y;
                g_app->cam.pos[2] = z;
                g_app->cam.angles[0] = oben != 0 ? 89.0F : nick;
                g_app->cam.angles[1] = gier;
                g_app->cam.angles[2] = 0.0F;
                g_app->mapDirty = true;
            }));
            add(pause(1.0));
            add(hb::fotoTeil(n, [] { return hb::karte(); }));
        }
        return s;
    }

    // ======================================================================
    // Die Handbuch-Mission (rc600): eine frische Mission auf taspir1, der
    // Landeplattform mit der Raven's Claw - shank: "das ist ein Tool fuer die
    // gesamte JKA-Community ... eine eigene frische Mission als Tutorial,
    // wo du alle Dinge zeigst". Alles entsteht mit den Funktionen des
    // Programms; jedes Bild ist ein Schritt auf dem Weg.
    //
    // Szene: Kyle und Rosh gehen ueber die Plattform vor das Schiff, die
    // Kamera faehrt mit, dann hebt die Raven's Claw ab (ROFF), die Kamera
    // wackelt, schwenkt ihr nach und blendet ab. (yavin1 war zu eng: die
    // Felsen der Schlucht verdeckten fast jeden Blick.)
    // ======================================================================
    add(tu("Handbuch: Sprache, Missionsordner im Laufordner", [] {
        if (const char* sp = std::getenv("BHED_SPRACHE")) {
            if (const i18n::LanguageInfo* l = i18n::findLanguage(sp)) { i18n::setLanguage(l->language); }
        }
        // Missionen: ueber BHED_MISSIONEN (der Bildlauf legt sie auf ein
        // Laufwerk X: aus subst - so steht kein Benutzerpfad im Bild).
        if (std::getenv("BHED_MISSIONEN") == nullptr) {
            const std::string wurzel = mb::verbinde(mb::laufOrdner(), "missionen");
            _putenv_s("BHED_MISSIONEN", wurzel.c_str());
        }
        g_app->settings.recentMissions.clear();
        g_app->leftMode = 1;
        g_app->settings.flussAnsicht = 0;
        // Lesbar auf den Bildern: die Oberflaeche groesser - aber nicht so
        // gross, dass die Kartenleiste abgeschnitten wird und die Skriptspalte
        // nur noch Icons zeigt (rc600: 1.6 war zu eng).
        g_app->uiScale = 1.3F;
        // Die Zeitleiste hoeher - sonst sieht man auf den Bildern nur die Kamera.
        g_app->timelineFrac = 0.40F;
    }));
    add(pause(0.8));
    // Alle Icons des Programms auf einem Bogen; bilder.py schneidet sie aus
    // (ikonen.txt: Name und Kasten) - fuer die Knopfseite des Handbuchs.
    add({"Handbuch: Icon-Bogen", [](int b) {
             static const char* const namen[] = {
                 "Neu", "Oeffnen", "Speichern", "Mehr", "Karte", "Paket", "Liste", "Skript", "Entity", "Namen", "Figur",
                 "Effekt", "Leuchten", "Licht", "Nebel", "Himmel", "Kamera", "Einstellungen", "Statistik", "Plus", "Boden",
                 "Hinfliegen", "Duplizieren", "Loeschen", "Rueckgaengig", "Wiederholen", "Explorer", "Suche", "Verschieben",
                 "Drehen", "Mission", "Schliessen", "Haken", "Warnung", "Abspielen", "Pause", "Zurueck", "Vor", "Zielen",
                 "Steuern", "Pfad", "Oben", "Erzeugen", "Raster", "Animieren", "ZumAnfang", "Schleife", "Ton", "Folgen",
                 "Zeilen", "Ruhig", "ZoomPlus", "ZoomMinus", "Einpassen", "Marke", "Blitz", "Tuer", "Wegpunkt", "Bruch",
                 "Sichtbar", "Versteckt", "Bildschirm"};
             static_assert(sizeof(namen) / sizeof(namen[0]) == static_cast<std::size_t>(UiIcon::Bildschirm) + 1,
                           "Icon-Bogen: Namen und UiIcon passen nicht zusammen");
             const int anzahl = static_cast<int>(sizeof(namen) / sizeof(namen[0]));
             const float zelle = 64.0F;
             const int spalten = 16;
             ImGui::SetNextWindowPos(ImVec2{40.0F, 120.0F});
             ImGui::SetNextWindowSize(ImVec2{zelle * spalten + 20.0F, zelle * 4.0F + 20.0F});
             ImGui::Begin("##ikonenbogen", nullptr,
                          ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
                              ImGuiWindowFlags_NoFocusOnAppearing);
             ImDrawList* dl = ImGui::GetWindowDrawList();
             const ImVec2 o = ImGui::GetWindowPos();
             const char* ordner = std::getenv("BHED_FOTOS");
             std::ofstream f;
             if (b == 1 && ordner != nullptr) { f.open(::bhed::utf8Pfad(std::string(ordner) + "\\ikonen.txt")); }
             for (int i = 0; i < anzahl; ++i) {
                 const float x = o.x + 10.0F + static_cast<float>(i % spalten) * zelle;
                 const float y = o.y + 10.0F + static_cast<float>(i / spalten) * zelle;
                 const UiIcon ic = static_cast<UiIcon>(i);
                 uiIcon(dl, ic, ImVec2{x + 12.0F, y + 12.0F}, 40.0F, ImGui::GetColorU32(uiIconFarbe(ic)));
                 if (f) {
                     f << namen[i] << " " << static_cast<int>(x + 4.0F) << " " << static_cast<int>(y + 4.0F) << " "
                       << static_cast<int>(x + 60.0F) << " " << static_cast<int>(y + 60.0F) << "\n";
                 }
             }
             ImGui::End();
             if (b == 2 && ordner != nullptr) { render::fotoAnfordern(std::string(ordner) + "\\ikonen.bmp"); }
             return b >= 6;
         }});
    add(hb::fotoTeil("h_start", [] { return hb::fensterRect("##startflaeche"); }));
    add(tu("Handbuch: Spielordner", [] { g_app->pathsOpen = true; }));
    add(pause(0.5));
    add(hb::fotoTeil("h_spielordner", [] { return hb::fensterRect(tr(Str::PathsTitle)); }));
    // Die Missionen des Grundspiels in assets0.pk3 - nur das Bild der Auswahl.
    add(tu("Handbuch: Spielordner zu, Missionen der pk3", [=] {
        g_app->pathsOpen = false;
        missionenAusArchiv(archiv);
    }));
    add(pause(0.6));
    // rc603: die weiteren Missionen des Spiels liest "Load mission" im
    // Hintergrund - fotografiert wird die fertige Liste.
    add(warteBis("Handbuch: Missionssuche fertig", [] { return !g_app->missionsSuche; }, 3500));
    // rc603 Zusammenfuehrung: taspir1 steht in assets0.pk3 so weit unten, dass
    // nach dem Rollen keine Abschnittsueberschrift mehr im Bild war (Durchsicht
    // t_mission). Mit dem Filter "tasp" steht die Ueberschrift "Missions in
    // assets0.pk3" ueber taspir1 - und das Bild zeigt den Filter gleich mit.
    add(tu("Handbuch: Filter tasp", [] {
        std::snprintf(g_app->missionsFilter, sizeof(g_app->missionsFilter), "%s", "tasp");
    }));
    add(pause(0.3));
    // taspir1 steht weit unten in der Liste - erst dorthin rollen, sonst zeigt
    // die Markierung auf eine Zeile ausserhalb des Bildes.
    // rc603: die Liste rollt in einem eigenen Feld; Zeilen ausserhalb werden
    // nicht angemeldet - also Bild fuer Bild weiterrollen, bis taspir1 mit
    // drei Zeilen Luft darunter im Bild steht.
    add({"Handbuch: taspir1 in der Liste zeigen", [](int b) {
             ImGuiWindow* liste = nullptr;
             for (ImGuiWindow* w : GImGui->Windows) {
                 if (std::strstr(w->Name, "##missionpick") != nullptr && std::strstr(w->Name, "##missionliste") != nullptr) {
                     liste = w;
                 }
             }
             if (liste == nullptr) { return b > 100; }
             const float zeile = ImGui::GetFrameHeightWithSpacing();
             const Element* e = finde("taspir1", "##missionpick", 0, false);
             if (e != nullptr && ganzSichtbar(*e)) {
                 const float unten = liste->InnerRect.Max.y - zeile * 3.0F;
                 if (e->rect.Max.y <= unten + 0.5F) { return true; }
                 // Kurze (gefilterte) Liste: weiter rollen geht nicht.
                 if (liste->Scroll.y >= liste->ScrollMax.y - 0.5F) { return true; }
                 ImGui::SetScrollY(liste, liste->Scroll.y + (e->rect.Max.y - unten));
                 return b > 400;
             }
             ImGui::SetScrollY(liste, liste->Scroll.y + zeile * 2.0F);
             return b > 400;
         }});
    add(pause(0.4));
    add(hb::fotoTeil("h_mission_wahl", [] { return hb::fensterRect("##missionpick"); }));
    add(tu("Handbuch: Auswahl zu", [] {
        g_app->missionPickOpen = false;
        g_app->missionsFilter[0] = 0;
    }));
    add(pause(0.3));

    // --- 1. Die Mission anlegen ------------------------------------------------
    add(tu("Handbuch: Neue Mission (Fenster)", [] { g_app->neueMissionOffen = true; }));
    add(pause(0.6));
    add(hb::fotoTeil("h_mission_neu", [] { return hb::fensterRect("##neuemission"); }));
    add(hb::fotoTeil("t4_1", [] { return hb::fensterRect("##neuemission"); }));
    add(tu("Handbuch: Fenster zu, Mission taspir_tut anlegen", [=] {
        g_app->neueMissionOffen = false;
        std::string fehler;
        (void)neueMissionAnlegen("maps/" + name + ".bsp", "taspir_tut", "taspir_tut", &fehler);
        diag::detail("Knopftest: Handbuch - neue Mission " + fehler);
    }));
    add(warteBis("Handbuch: neue Mission geladen", [] { return !g_app->map.empty() && g_app->mapBereit; }, 1500));
    // taspir1 startet am Spielerstart sein eigenes Intro (taspir1/preIntro) -
    // dieser Runner kommt weg, sonst liefen beide Szenen zugleich.
    // Der Runner des Originals in der Liste (fuer den ersten Projektschritt).
    add(tu("Handbuch: Mission-Tab", [] { g_app->leftMode = 1; }));
    add(pause(0.6));
    add(klick("##entsuche"));
    add(tippe("t271"));
    add(pause(0.5));
    add(hb::fotoTeil("e_t271", linksUndKarte, [] {
        return std::vector<hb::Marke>{hb::Marke(hb::knopf("##entsuche")), hb::Marke(hb::knopfN("t271", "entlayers", 0))};
    }));
    add(klick("##entsuche"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(taste(ImGuiKey_Backspace, "Rueck"));
    add(klickAn("daneben", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
    add(pause(0.3));
    add(tu("Handbuch: nur die Entities des Skripts hervorheben", [] { g_app->skriptFokus = true; }));
    add(tu("Handbuch: den Runner des Originals loeschen", [] {
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const std::string* u = g_app->map.entities[i].find("usescript");
            if (g_app->map.entities[i].classname == "target_scriptrunner" && u != nullptr && u->find("preIntro") != std::string::npos) {
                diag::detail("Knopftest: Handbuch - Runner des Originals geloescht: " + *u);
                (void)entityLoeschen(static_cast<int>(i));
                break;
            }
        }
    }));
    add(pause(1.0));
    add(tu("Handbuch: Blick auf den Spielerstart", [] {
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            if (g_app->map.entities[i].classname == "info_player_start") {
                entityAuswahlSetzen(static_cast<int>(i));
                zurEntityFliegen(i);
                hb::kameraZurueck(220.0F, 140.0F);
                break;
            }
        }
    }));
    add(pause(1.0));
    add(hb::fotoTeil("t4_2", linksUndKarte));

    // --- Markierungen der Bilder (rc600) -----------------------------------------
    // shank: "die Bilder sind zum Teil winzig, und du weisst gar nicht, wo du
    // draufklicken sollst" - mit dem Beispiel der Unreal-Anleitungen: gelbe
    // Kaesten und Pfeile, Leisten mit nummerierten Gruppen. Ein Rahmen je Ziel,
    // Nummern nur, wo eine Tabelle sie erklaert.
    {
        using hb::knopf;
        using hb::gruppe;
        using hb::markiere;
        const auto nur = [](ImRect r) { return std::vector<hb::Marke>{hb::Marke(r)}; };
        markiere("h_start", [=] { return nur(knopf(tr(Str::StartCardNew))); });
        markiere("h_spielordner", [=] { return nur(knopf(tr(Str::Pk3AddPath), tr(Str::PathsTitle))); });
        markiere("h_mission_wahl", [=] { return nur(knopf("taspir1", "##missionpick")); });
        markiere("h_mission_neu", [=] { return nur(knopf(tr(Str::MissionCreate), "##neuemission")); });
        markiere("t4_1", [=] { return nur(knopf(tr(Str::MissionCreate), "##neuemission")); });
        markiere("t4_2", [=] {
            const ImVec2 q = hb::entityAufSchirm(g_app->pickedEntity);
            return nur(ImRect{q.x - 22.0F, q.y - 22.0F, q.x + 22.0F, q.y + 22.0F});
        });
        markiere("k_inspektor", [=] {
            return std::vector<hb::Marke>{hb::Marke(knopf(tr(Str::EntDropFloor), "entinspektor")), hb::Marke(knopf("...", "entinspektor"))};
        });
        const auto vierEntities = [=] {
            std::vector<hb::Marke> v;
            for (const char* n : {"kyle", "rosh", "nav_kyle", "nav_rosh"}) { v.emplace_back(hb::entityKasten(n, 18.0F)); }
            return v;
        };
        markiere("h_entity", vierEntities);
        markiere("t4_3", [=] {
            return std::vector<hb::Marke>{hb::Marke(hb::entityKasten("nav_kyle", 18.0F)), hb::Marke(hb::entityKasten("nav_rosh", 18.0F))};
        });
        markiere("a_k_setzen", [=] {
            const Str t = g_app->platzierKlasse == "NPC_spawner"        ? Str::EntKindNpc
                          : g_app->platzierKlasse == "waypoint_navgoal" ? Str::EntKindNav
                                                                         : Str::AppTitle;
            if (t == Str::AppTitle) { return std::vector<hb::Marke>{}; }
            return nur(knopf(tr(t), "##platzieren"));
        });
        const auto kyle = [=] { return nur(hb::figurKasten("kyle", 22.0F)); };
        markiere("h_figur_bahn", kyle);
        markiere("t2_4", kyle);
        markiere("t2_1", kyle);
        markiere("t2_ergebnis", kyle);
        markiere("t2_6", kyle);
        markiere("t2_2", [=] { return nur(hb::entityKasten("nav_kyle", 18.0F)); });
        markiere("t2_5", [=] { return nur(hb::fensterRect("##Popup")); });
        markiere("k_gangart", [=] { return nur(hb::fensterRect("##Popup")); });
        markiere("h_kamera", [=] { return nur(knopf(tr(Str::CamNewShot))); });
        markiere("t1_3", [=] { return nur(knopf(tr(Str::CamNewShot))); });
        markiere("t1_2", [=] {
            const ImVec2 q = bildZuSchirm(g_app->cameraSx, g_app->cameraSy);
            return nur(ImRect{q.x - 26.0F, q.y - 26.0F, q.x + 26.0F, q.y + 26.0F});
        });
        markiere("h_kamera_verschieben", [=] { return nur(knopf(tr(Str::GizmoMove))); });
        markiere("h_kamera_drehen", [=] { return nur(knopf(tr(Str::GizmoRotate))); });
        markiere("a_k_pilot", [=] { return nur(knopf(tr(Str::CamPilotKey))); });
        markiere("t3_1", [=] { return nur(knopf(tr(Str::EntAnimate))); });
        markiere("t3_2", [=] {
            return std::vector<hb::Marke>{hb::Marke(knopf(tr(Str::AnimModeRoff), "###animation")),
                                          hb::Marke(knopf(tr(Str::AnimAddKey), "###animation"))};
        });
        markiere("t3_4", [=] { return nur(hb::entityKasten("ravensClaw", 46.0F)); });
        markiere("t1_6", [=] { return nur(knopf(tr(Str::Compile))); });
        markiere("k_neu_2", [=] { return nur(knopf(tr(Str::EditorOk), "###editor")); });
        markiere("k_var_if", [=] { return std::vector<hb::Marke>{hb::Marke(hb::knopfN("##v", "###editor", 0)), hb::Marke(hb::knopfN("##v", "###editor", 2))}; });
        markiere("k_suchen", [=] {
            return std::vector<hb::Marke>{hb::Marke(knopf(tr(Str::FindContains), "###find")),
                                          hb::Marke(knopf(tr(Str::FindReplaceAllDocs), "###find"))};
        });
        markiere("k_undo", [=] { return nur(knopf(tr(Str::UndoOnlyThis))); });
        markiere("h_ansicht_einst", [=] { return nur(knopf(tr(Str::MapBrightness), "##Popup")); });
        // Die Combos registrieren sich nicht als Element - der Rahmen um
        // Spielregeln und Schwierigkeit liegt ueber Ok (zwei Zeilen).
        markiere("h_einst_vorschau", [=] {
            const ImRect ok = knopf(tr(Str::EditorOk), tr(Str::PrefsTitle));
            const ImRect w = hb::fensterRect(tr(Str::PrefsTitle));
            if (ok.GetHeight() <= 1.0F) { return std::vector<hb::Marke>{}; }
            const float fs = ImGui::GetFontSize();
            const float zeile = ImGui::GetFrameHeightWithSpacing();
            return nur(ImRect{ok.Min.x - fs * 0.2F, ok.Min.y - zeile * 2.0F - fs * 0.6F, w.Max.x - fs * 0.6F, ok.Min.y - fs * 0.7F});
        });
        markiere("k_marke", [=] { return nur(knopf(tr(Str::EntityCamToTag), "##Popup")); });
        markiere("h_wegpunkte", [=] { return nur(knopf(tr(Str::WegErzeugen))); });
        markiere("k_saber", kyle);
        markiere("k_tuer_menue", [=] { return nur(knopf(tr(Str::EntityUse), "##Popup")); });
        markiere("k_effekt", [=] { return nur(hb::entityKasten("fire1", 30.0F)); });
        markiere("k_tod_insp", [=] { return nur(knopf(tr(Str::EntOpenScript), "", true)); });
        markiere("k_spawnflags", [=] { return nur(knopf(tr(Str::EntFlags))); });
        const auto speichernKnopf = [=] { return nur(knopf(tr(Str::MissionSaveNow), "##missionspeichern")); };
        markiere("k_pruefen", speichernKnopf);
        markiere("h_mission_speichern", speichernKnopf);
        markiere("t4_4", speichernKnopf);
        markiere("k_kartenmenue", [=] { return nur(knopf(tr(Str::OpenEnt), "##Popup")); });
        markiere("k_bahnen", [=] { return nur(knopf(tr(Str::CamPathAll))); });
        markiere("k_waits", [=] { return nur(knopf(tr(Str::TlWaits))); });
        markiere("k_dateimenue", [=] { return nur(knopf(tr(Str::FileBackupFolder), "##Menu")); });
        markiere("k_pk3", [=] {
            return std::vector<hb::Marke>{hb::Marke(knopf(tr(Str::Pk3Filter))), hb::Marke(knopf("scripts/taspir1/intro.ibi"))};
        });
        markiere("h_menue_missionen", [=] { return nur(knopf(tr(Str::RecentMissions), "##Menu")); });
        markiere("t4_6", [=] { return nur(knopf(tr(Str::RecentMissions), "##Menu")); });
        markiere("h_modell", [=] { return nur(knopf(tr(Str::NpcBrowser))); });
        // Leisten mit nummerierten Gruppen - die Tabellen der Knopfseite
        // erklaeren dieselben Nummern.
        markiere("r_leiste_karte", [=] {
            return std::vector<hb::Marke>{
                hb::Marke(gruppe({tr(Str::MissionNew), tr(Str::LoadMission), tr(Str::MissionSave)}), 1),
                hb::Marke(gruppe({tr(Str::MapMore)}), 2),
                hb::Marke(gruppe({tr(Str::MapEntities), tr(Str::MapNames), tr(Str::MapActors), tr(Str::MapEffects), tr(Str::MapGlow),
                                  tr(Str::MapDynLights), tr(Str::MapFog), tr(Str::MapSky)}), 3),
                hb::Marke(gruppe({tr(Str::MapThroughCam), tr(Str::MapTopView), tr(Str::WegTitel)}), 4),
                hb::Marke(gruppe({tr(Str::Grid), tr(Str::MapViewSettings), tr(Str::MapStats)}), 5)};
        });
        markiere("r_transport", [=] {
            return std::vector<hb::Marke>{
                hb::Marke(gruppe({tr(Str::TlRewind), tr(Str::TlPlay), tr(Str::TlStop)}), 1),
                hb::Marke(gruppe({tr(Str::TlLoop), tr(Str::TlAudio), tr(Str::TlFollow)}), 2),
                hb::Marke(gruppe({tr(Str::TlZoomOut), tr(Str::TlZoomIn)}), 3),
                hb::Marke(gruppe({tr(Str::TlTracks), tr(Str::TlWaits)}), 4),
                hb::Marke(gruppe({tr(Str::MapInsertCamera)}), 5)};
        });
        markiere("r_leiste_modell", [=] {
            return std::vector<hb::Marke>{
                hb::Marke(gruppe({tr(Str::OpenGlm), tr(Str::OpenGlmPk3), tr(Str::NpcBrowser)}), 1),
                hb::Marke(gruppe({tr(Str::OpenSkin), tr(Str::ModelLoadGla)}), 2),
                hb::Marke(gruppe({tr(Str::ModelTurntable), tr(Str::ModelBones), tr(Str::ModelCaps), tr(Str::ModelTextures)}), 3),
                hb::Marke(gruppe({tr(Str::ModelResetView)}), 4)};
        });
        // Das Hauptfenster in seinen Teilen (Seite "The window").
        markiere("h_fenster", [=] {
            const ImVec2 d = ImGui::GetIO().DisplaySize;
            const float fs = ImGui::GetFontSize();
            const float zeile = ImGui::GetFrameHeight();
            const ImRect band = hb::fensterRect("toolband");
            const ImRect links = hb::vereint(hb::fensterRect("entlayers"), hb::fensterRect("entinspektor"));
            ImRect karte = hb::karte();
            karte.Min.y -= fs * 2.6F;
            return std::vector<hb::Marke>{
                hb::Marke(ImRect{4.0F, 2.0F, fs * 14.0F, zeile}, 1),
                hb::Marke(ImRect{4.0F, zeile + 2.0F, fs * 22.0F, zeile * 2.2F}, 2),
                hb::Marke(gruppe({tr(Str::ViewEvents), tr(Str::ViewMap), tr(Str::ViewModel)}), 3),
                hb::Marke(links, 4),
                hb::Marke(karte, 5),
                hb::Marke(hb::fensterRect("##flowarea"), 6),
                hb::Marke(band, 7),
                hb::Marke(ImRect{4.0F, d.y - fs * 1.7F, d.x * 0.5F, d.y - 2.0F}, 8)};
        });
        // Fenster mit vielen Bedienelementen: nummerierte Rahmen, die
        // Legende im Handbuch erklaert jede Nummer (shank: "man haette hier
        // eine Erklaerung abgeben koennen, was welcher Button macht").
        const auto nr = [](ImRect r, int n) { return hb::Marke(r, n); };
        markiere("t3_2", [=] {
            const std::string w = "###animation";
            return std::vector<hb::Marke>{
                nr(gruppe({tr(Str::AnimModeMove), tr(Str::AnimModeRoff)}, w), 1),
                nr(hb::knopfN("##roffname", w, 0), 2),
                nr(knopf(tr(Str::AnimRoffSmooth), w), 3),
                nr(knopf(tr(Str::AnimAddKey), w), 4),
                nr(gruppe({tr(Str::AnimDeleteKey), tr(Str::AnimGotoKey)}, w), 5),
                nr(knopf(tr(Str::AnimRemove), w), 6),
                nr(gruppe({"#", tr(Str::AnimTime), tr(Str::CamPosition), tr(Str::CamAngles)}, w), 7)};
        });
        markiere("k_inspektor", [=] {
            const std::string w = "entinspektor";
            return std::vector<hb::Marke>{
                nr(knopf(tr(Str::EntityFlyTo), w), 1),
                nr(knopf(tr(Str::EntDropFloor), w), 2),
                nr(gruppe({tr(Str::EntDuplicate), tr(Str::EntDelete)}, w), 3),
                nr(gruppe({tr(Str::EditUndo), tr(Str::EditRedo)}, w), 4),
                nr(knopf(tr(Str::EntRaw), w), 5),
                nr(knopf("##feldsuche", w), 6),
                nr(knopf("...", w), 7)};
        });
        const auto neueMission = [=] {
            const std::string w = "##neuemission";
            return std::vector<hb::Marke>{
                nr(knopf("##kartensuche", w), 1),
                nr(knopf("##missionsname", w), 2),
                nr(knopf("##skriptordner", w), 3),
                nr(gruppe({tr(Str::MissionCreate), tr(Str::MissionCancel)}, w), 4)};
        };
        markiere("h_mission_neu", neueMission);
        markiere("t4_1", neueMission);
        const auto speichern = [=] {
            const std::string w = "##missionspeichern";
            return std::vector<hb::Marke>{
                nr(gruppe({tr(Str::MissionTarget), tr(Str::MissionChoose)}, w), 1),
                nr(gruppe({tr(Str::MissionSaveNow), tr(Str::MissionCancel)}, w), 2)};
        };
        markiere("h_mission_speichern", speichern);
        markiere("t4_4", speichern);
        markiere("k_suchen", [=] {
            const std::string w = "###find";
            return std::vector<hb::Marke>{
                nr(knopf(tr(Str::FindNamed), w), 1),
                nr(knopf(tr(Str::FindContains), w), 2),
                nr(knopf(tr(Str::FindReplaceWith), w), 3),
                nr(gruppe({tr(Str::FindWhole), tr(Str::FindInSelection)}, w), 4),
                nr(gruppe({tr(Str::FindNextBtn), tr(Str::FindPrevBtn)}, w), 5),
                nr(gruppe({tr(Str::FindReplaceBtn), tr(Str::FindReplaceAll)}, w), 6),
                nr(knopf(tr(Str::FindReplaceAllDocs), w), 7)};
        });
        markiere("h_wegpunkte", [=] {
            return std::vector<hb::Marke>{
                nr(knopf(tr(Str::WegZeigen)), 1),
                nr(gruppe({tr(Str::WegSetzen), tr(Str::WegVerbinden)}), 2),
                nr(gruppe({tr(Str::WegErzeugen), tr(Str::WegEntfernen)}), 3),
                nr(knopf(tr(Str::WegPruefen)), 4)};
        });
    }

    // --- 2. Figuren und Laufziele setzen - als Bildfolge -------------------------
    // Blick von Westen ueber das Nordufer; geklickt wird auf Weltpunkte.
    const auto blick = [](float x, float y, float z, float nick, float gier) {
        g_app->throughCamera = false;
        g_app->draufsicht = false;
        g_app->cam.pos[0] = x;
        g_app->cam.pos[1] = y;
        g_app->cam.pos[2] = z;
        g_app->cam.angles[0] = nick;
        g_app->cam.angles[1] = gier;
        g_app->cam.angles[2] = 0.0F;
        g_app->mapDirty = true;
    };
    const auto schirm = [](float x, float y, float z) {
        const float w[3] = {x, y, z};
        ImVec2 q{0, 0};
        (void)kartenPunktAufSchirm(w, q);
        return q;
    };
    add(tu("Handbuch: Blick ueber das Nordufer", [=] {
        entityAuswahlSetzen(-1);
        blick(-1620.0F, -2120.0F, 830.0F, 26.0F, 90.0F);
    }));
    add(pause(1.0));
    struct Setzen { const char* klasse; float x, y, z; const char* key; const char* name; const char* typ; };
    const Setzen setzen[] = {
        {"NPC_spawner", -1500.0F, -1900.0F, 650.0F, "NPC_targetname", "kyle", "Kyle"},
        {"NPC_spawner", -1720.0F, -1930.0F, 650.0F, "NPC_targetname", "rosh", "rosh_penin"},
        {"waypoint_navgoal", -1560.0F, -1640.0F, 640.0F, "targetname", "nav_kyle", ""},
        {"waypoint_navgoal", -1740.0F, -1660.0F, 640.0F, "targetname", "nav_rosh", ""},
    };
    int bild = 0;
    for (const Setzen& st : setzen) {
        add(tu(std::string("Handbuch: Kachel ") + st.klasse, [st] { g_app->platzierKlasse = st.klasse; }));
        // Die Maus faehrt zum Punkt - Bild fuer Bild, die Vorschau folgt ihr.
        for (int k = 0; k < 6; ++k) {
            add(tu("Handbuch: Maus zum Punkt", [=] {
                const ImVec2 z = schirm(st.x, st.y, st.z);
                const float f = static_cast<float>(k + 1) / 6.0F;
                const ImVec2 von = ansichtMitte();
                setzeMaus(ImVec2{von.x + (z.x - von.x) * f, von.y + (z.y - von.y) * f});
            }));
            add(pause(0.08));
            char n[32];
            std::snprintf(n, sizeof(n), "a_k_setzen_%02d", bild++);
            add(hb::fotoTeil(n, linksUndKarte));
        }
        add(klickAn(std::string("setzen ") + st.name, [=] { return schirm(st.x, st.y, st.z); }));
        add(pause(0.3));
        add(tu(std::string("Handbuch: Name ") + st.name, [st] {
            const int i = g_app->pickedEntity;
            diag::detail("Knopftest: Handbuch - gesetzt " + std::string(st.klasse) + " als " + std::to_string(i));
            if (i < 0) { return; }
            (void)entityKeySetzen(i, st.key, st.name);
            if (st.typ[0] != '\0') { (void)entityKeySetzen(i, "NPC_type", st.typ); }
            (void)entityAufBoden(i);
        }));
        add(pause(0.3));
        for (int k = 0; k < 2; ++k) {
            char n[32];
            std::snprintf(n, sizeof(n), "a_k_setzen_%02d", bild++);
            add(hb::fotoTeil(n, linksUndKarte));
        }
    }
    add(tu("Handbuch: Kyle waehlen (Inspektor)", [] {
        // Die Maus weg von der Karte, sonst steht ein Hover-Hinweis im Bild.
        setzeMaus(ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F});
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const std::string* n = g_app->map.entities[i].find("NPC_targetname");
            if (n != nullptr && *n == "kyle") { entityAuswahlSetzen(static_cast<int>(i)); }
        }
        g_app->mapDirty = true;
    }));
    add(pause(0.6));
    add(hb::fotoTeil("k_inspektor", [] {
        ImRect r = hb::fensterRect("entinspektor");
        r.Expand(ImGui::GetFontSize() * 0.8F);
        return r;
    }));
    add(hb::fotoTeil("h_entity", linksUndKarte));
    add(hb::fotoTeil("t4_3", linksUndKarte));

    // --- 3. Das Skript der Szene ------------------------------------------------------
    const auto skriptSetzen = [](const std::string& text) {
        Script sc;
        std::vector<Diag> d;
        (void)readScript(text, sc, d);
        (void)g_app->doc.ersetzeSkript(std::move(sc), "Handbuch");
        hb::skriptNeu();
    };
    const std::string kopf =
        "//Generated by BehavEd\n\n"
        "rem ( \"taspir_tut - intro. Kyle and Rosh walk to the ship, then it takes off.\" );\n"
        "camera ( /*@CAMERA_COMMANDS*/ ENABLE );\n"
        "camera ( /*@CAMERA_COMMANDS*/ MOVE, < -400.000 -3300.000 1300.000 >, 0 );\n"
        "camera ( /*@CAMERA_COMMANDS*/ PAN, < 10.000 120.000 0.000 >, < 0.000 0.000 0.000 >, 0 );\n";
    const auto lauf = [](const char* wer, const char* ziel, const char* blickZu) {
        return std::string("affect ( \"") + wer + "\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
               "\ttask ( \"walk\" )\n\t{\n"
               "\t\tset ( /*@SET_TYPES*/ \"SET_WALKING\", \"true\" );\n"
               "\t\tset ( /*@SET_TYPES*/ \"SET_NAVGOAL\", \"" + ziel + "\" );\n"
               "\t}\n\n"
               "\tdowait ( \"walk\" );\n"
               "\tset ( /*@SET_TYPES*/ \"SET_LOOK_TARGET\", \"" + blickZu + "\" );\n"
               "}\n\n";
    };
    const std::string laeufe = lauf("kyle", "nav_kyle", "rosh") + lauf("rosh", "nav_rosh", "kyle");
    const std::string kamera =
        "camera ( /*@CAMERA_COMMANDS*/ MOVE, < -1640.000 -2180.000 720.000 >, 4000 );\n"
        "camera ( /*@CAMERA_COMMANDS*/ PAN, < -4.000 88.000 0.000 >, < 0.000 0.000 0.000 >, 4000 );\n"
        "wait ( 5000.000 );\n"
        "camera ( /*@CAMERA_COMMANDS*/ MOVE, < -750.000 -2700.000 1000.000 >, 0 );\n"
        "camera ( /*@CAMERA_COMMANDS*/ PAN, < 4.000 120.000 0.000 >, < 0.000 0.000 0.000 >, 0 );\n"
        "camera ( /*@CAMERA_COMMANDS*/ SHAKE, 4.000, 2000 );\n"
        "wait ( 1000.000 );\n"
        "camera ( /*@CAMERA_COMMANDS*/ PAN, < -15.000 97.000 0.000 >, < 0.000 0.000 0.000 >, 2500 );\n"
        "wait ( 2500.000 );\n"
        "camera ( /*@CAMERA_COMMANDS*/ PAN, < -31.000 40.000 0.000 >, < 0.000 0.000 0.000 >, 2000 );\n"
        "wait ( 2000.000 );\n"
        "camera ( /*@CAMERA_COMMANDS*/ FADE, < 0.000 0.000 0.000 >, 0.000, < 0.000 0.000 0.000 >, 1.000, 1500 );\n"
        "wait ( 1500.000 );\n"
        "camera ( /*@CAMERA_COMMANDS*/ DISABLE );\n";
    add(tu("Handbuch: Skript - die beiden Laeufe", [=] { skriptSetzen(kopf + laeufe + "wait ( 6000.000 );\ncamera ( /*@CAMERA_COMMANDS*/ DISABLE );\n"); }));
    add(warteBis("Handbuch: Szene", [] { return g_app->camTrackValid; }, 900));
    add(pause(0.8));
    // Die Figur mitten im Lauf, ihr Weg.
    add(tu("Handbuch: Kyle mitten im Lauf", [=] {
        // Die Maus weg von der Karte, sonst steht ein Hover-Hinweis im Bild.
        setzeMaus(ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F});
        g_app->playMs = 2500.0;
        g_app->playing = false;
        g_app->cameraSelected = false;
        for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
            if (entityFuerFigur(static_cast<int>(ai)) == g_app->pickedEntity) { g_app->selectedActor = static_cast<int>(ai); }
        }
        blick(-1620.0F, -2120.0F, 830.0F, 26.0F, 90.0F);
    }));
    add(pause(1.0));
    add(hb::fotoTeil("h_figur_bahn", karteMitZeitleiste));
    add(hb::fotoTeil("t2_4", kartePlusLeiste));
    add(hb::fotoTeil("t2_1", linksUndKarte));
    add(hb::fotoTeil("t2_ergebnis", kartePlusLeiste));
    // Rechtsklick auf Kyles Laufziel: gehen, rennen, rueckwaerts, ansehen.
    add(tu("Handbuch: Laufziel waehlen", [] {
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            if (g_app->map.entities[i].targetname == "nav_kyle") { entityAuswahlSetzen(static_cast<int>(i)); }
        }
        g_app->mapDirty = true;
    }));
    add(pause(0.6));
    add(hb::fotoTeil("t2_2", linksUndKarte));
    add(klickAn("Rechtsklick auf das Laufziel", [=] {
        float o[3] = {0, 0, 0};
        if (g_app->pickedEntity >= 0) { (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(g_app->pickedEntity)], o); }
        return schirm(o[0], o[1], o[2]);
    }, 1));
    add(pause(0.5));
    const auto gangartRect = [=] {
        ImRect r = hb::fensterRect("##Popup");
        float o[3] = {0, 0, 0};
        if (g_app->pickedEntity >= 0) { (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(g_app->pickedEntity)], o); }
        r = hb::vereint(r, hb::um(schirm(o[0], o[1], o[2]), ImGui::GetFontSize() * 6.0F, ImGui::GetFontSize() * 4.0F));
        r.Expand(ImGui::GetFontSize() * 2.0F);
        return r;
    };
    add(hb::fotoTeil("t2_5", gangartRect));
    add(hb::fotoTeil("k_gangart", gangartRect));
    add(klickAn("Menue zu", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
    add(pause(0.3));
    // Kyle mitten im Lauf zur Seite ziehen: ein Bahnpunkt (danach zurueck).
    auto tiefe = std::make_shared<std::pair<std::size_t, std::size_t>>();
    add(tu("Handbuch: Kyle waehlen (mitten im Lauf)", [=] {
        tiefe->first = g_app->doc.undoDepth();
        tiefe->second = g_app->entUndo.size();
        const int ei = entityFuerFigur(g_app->selectedActor);
        if (ei >= 0) { entityAuswahlSetzen(ei); }
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(ziehenAn("Handbuch: Kyle zur Seite ziehen", [] { return hb::entityAufSchirm(entityFuerFigur(g_app->selectedActor)); },
                 120.0F, 40.0F, 0));
    add(pause(0.8));
    add(hb::fotoTeil("t2_6", kartePlusLeiste));
    add(tu("Handbuch: Bahnpunkt zuruecknehmen", [=] {
        while (g_app->doc.undoDepth() > tiefe->first && g_app->doc.undo()) {}
        while (g_app->entUndo.size() > tiefe->second && entityRueckgaengig()) {}
        hb::skriptNeu();
        entityAuswahlSetzen(-1);
    }));
    add(pause(0.5));

    // --- 4. Kamera: Einstellungen, Fahrt, Pilot ------------------------------------------
    add(tu("Handbuch: Skript - mit Kamerafahrt", [=] { skriptSetzen(kopf + laeufe + kamera); }));
    add(warteBis("Handbuch: Szene", [] { return g_app->camTrackValid; }, 900));
    add(tu("Handbuch: Kamera waehlen, zweite Einstellung", [=] {
        g_app->cameraSelected = true;
        kameraSchluesselWaehlen(1, true);
        blick(-1200.0F, -2600.0F, 1000.0F, 22.0F, 115.0F);
    }));
    add(pause(1.0));
    add(hb::fotoTeil("h_kamera", linksUndKarte));
    add(hb::fotoTeil("t1_2", linksUndKarte));
    // Die Leisten einzeln - fuer die Knopfseite des Handbuchs.
    add(tu("Handbuch: Oberflaeche 1,0 fuer die ganze Kartenleiste", [] { g_app->uiScale = 1.0F; }));
    add(pause(0.8));
    add(hb::fotoTeil("r_leiste_karte", [] {
        const float fs = ImGui::GetFontSize();
        return ImRect{g_app->kartenAnsicht[0] - fs * 0.4F, g_app->kartenAnsicht[1] - fs * 2.6F, g_app->kartenAnsicht[2] + fs * 0.4F,
                      g_app->kartenAnsicht[1] + 2.0F};
    }));
    add(tu("Handbuch: Oberflaeche wieder 1,3", [] { g_app->uiScale = 1.3F; }));
    add(pause(0.8));
    add(hb::fotoTeil("r_kacheln", [] {
        ImRect r = hb::fensterRect("##platzieren");
        r.Expand(ImGui::GetFontSize() * 0.3F);
        return r;
    }));
    add(hb::fotoTeil("r_transport", [] {
        // Die Transportleiste: der obere Rand des Bandes unter der Tabelle.
        ImRect r = hb::fensterRect("toolband");
        r.Max.y = r.Min.y + ImGui::GetFontSize() * 2.3F;
        return r;
    }));
    add(hb::fotoTeil("h_kamera_verschieben", [] {
        return hb::um(bildZuSchirm(g_app->cameraSx, g_app->cameraSy), ImGui::GetFontSize() * 13.0F, ImGui::GetFontSize() * 8.0F);
    }));
    add(tu("Handbuch: Kamera drehen", [] { g_app->gizmoMode = App::GizmoMode::Rotate; g_app->mapDirty = true; }));
    add(pause(0.6));
    add(hb::fotoTeil("h_kamera_drehen", [] {
        return hb::um(bildZuSchirm(g_app->cameraSx, g_app->cameraSy), ImGui::GetFontSize() * 13.0F, ImGui::GetFontSize() * 8.0F);
    }));
    add(tu("Handbuch: Kamera verschieben, Fahrzeit sichtbar", [] { g_app->gizmoMode = App::GizmoMode::Move; g_app->mapDirty = true; }));
    add(pause(0.4));
    add(hb::fotoTeil("t1_4", [] {
        ImRect r = hb::karte();
        return ImRect{0.0F, r.Min.y - ImGui::GetFontSize() * 2.6F, r.Min.x, r.Max.y};
    }));
    add(tu("Handbuch: neue Einstellung an der Ansicht", [=] {
        tiefe->first = g_app->doc.undoDepth();
        blick(-1250.0F, -2350.0F, 760.0F, 6.0F, 110.0F);
        kameraNeueEinstellung();
    }));
    add(pause(0.8));
    add(hb::fotoTeil("t1_3", linksUndKarte));
    add(tu("Handbuch: die neue Einstellung zuruecknehmen", [=] {
        while (g_app->doc.undoDepth() > tiefe->first && g_app->doc.undo()) {}
        hb::skriptNeu();
    }));
    add(pause(0.6));
    // Pilot: in der zweiten Einstellung sitzen und fliegen - Bildfolge.
    add(tu("Handbuch: Pilot starten", [] {
        g_app->cameraSelected = true;
        kameraSchluesselWaehlen(1, true);
        g_app->playMs = 4000.0;
        kameraPilotStarten();
        setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x * 0.5F, ImGui::GetIO().DisplaySize.y - 4.0F});
    }));
    add(pause(0.8));
    auto pilotStart = std::make_shared<Camera>();
    add(tu("Handbuch: Pilot - Start merken", [=] { *pilotStart = g_app->cam; }));
    for (int k = 0; k < 36; ++k) {
        add(tu("Handbuch: Pilot fliegt", [=] {
            const float f = static_cast<float>(k) / 35.0F;
            g_app->cam = *pilotStart;
            // Ein Stueck zurueck und hoch, dabei langsam nach links drehen.
            float v[3];
            g_app->cam.forward(v);
            for (int c = 0; c < 3; ++c) { g_app->cam.pos[c] -= v[c] * 260.0F * f; }
            g_app->cam.pos[2] += 120.0F * f;
            g_app->cam.angles[1] += 25.0F * f;
            g_app->cam.angles[0] += 8.0F * f;
            g_app->mapDirty = true;
        }));
        add(pause(0.06));
        char n[32];
        std::snprintf(n, sizeof(n), "a_k_pilot_%02d", k);
        add(hb::fotoTeil(n, kartePlusLeiste));
    }
    add(tu("Handbuch: Pilot abbrechen", [] { kameraPilotBeenden(false); }));
    add(pause(0.5));

    // --- 5. Der Baum stuerzt: ROFF im Animationsfenster -----------------------------------
    add(tu("Handbuch: das Schiff animieren", [=] {
        *schiff = hb::entityMitName("misc_model_breakable", "ravensClaw");
        g_app->cameraSelected = false;
        if (*schiff < 0) { return; }
        entityAuswahlSetzen(*schiff);
        animStarten(*schiff);
        g_app->playMs = 5500.0;
    }));
    add(pause(0.5));
    add(klick(tr(Str::AnimModeRoff), "###animation"));
    add(tu("Handbuch: Maus weg, Fenster rechts neben die Karte", [] {
        setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x * 0.5F, ImGui::GetIO().DisplaySize.y - 4.0F});
        const ImGuiWindow* w = ImGui::FindWindowByName("###animation");
        const float breit = w != nullptr ? w->Size.x : ImGui::GetFontSize() * 30.0F;
        ImGui::SetWindowPos("###animation", ImVec2{ImGui::GetIO().DisplaySize.x - breit - 8.0F, g_app->kartenAnsicht[1] + 8.0F});
    }));
    add(pause(0.8));
    add(hb::fotoTeil("t3_1", linksUndKarte));
    add(hb::fotoTeil("t3_2", [] { return hb::fensterRect("###animation"); }));
    add(tu("Handbuch: Schiff - vier Schluessel", [=] {
        if (*schiff < 0) { return; }
        float o[3] = {0, 0, 0};
        (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(*schiff)], o);
        std::vector<AnimSchluessel> k(4);
        const double ms[4] = {5000.0, 6500.0, 8500.0, 10500.0};
        const float weg[4][3] = {{0, 0, 0}, {0, 0, 220}, {900, 0, 720}, {2600, 0, 1600}};
        const float nick[4] = {0.0F, 0.0F, -12.0F, -18.0F};
        for (std::size_t i = 0; i < 4; ++i) {
            k[i].ms = ms[i];
            k[i].ort = {o[0] + weg[i][0], o[1] + weg[i][1], o[2] + weg[i][2]};
            k[i].winkel = {nick[i], 0.0F, 0.0F};
        }
        (void)animSchreiben(k);
        g_app->playMs = 7500.0;
        // Vor dem Schiff, zweiter und dritter Schluessel im Bild.
        blick(o[0] + 450.0F, o[1] - 1000.0F, o[2] + 600.0F, 7.0F, 90.0F);
    }));
    add(pause(1.2));
    add(hb::fotoTeil("h_animation", [=] {
        ImRect r = kartePlusLeiste();
        r.Add(hb::fensterRect("###animation"));
        return r;
    }));
    add(hb::fotoTeil("t3_3", kartePlusLeiste));
    add(hb::fotoTeil("t3_6", [] { return hb::fensterRect("###animation"); }));
    add(tu("Handbuch: Drehring bei 6,5 s", [] { g_app->playMs = 6500.0; g_app->gizmoMode = App::GizmoMode::Rotate; g_app->mapDirty = true; }));
    add(pause(0.8));
    add(hb::fotoTeil("t3_4", karteMitZeitleiste));
    add(hb::fotoTeil("h_gizmo_drehen", [=] {
        return hb::um(hb::entityAufSchirm(*schiff), ImGui::GetFontSize() * 14.0F, ImGui::GetFontSize() * 9.0F);
    }));
    add(tu("Handbuch: Verschiebepfeile", [] { g_app->gizmoMode = App::GizmoMode::Move; g_app->playMs = 5000.0; g_app->mapDirty = true; }));
    add(pause(0.6));
    add(hb::fotoTeil("h_gizmo_verschieben", [=] {
        return hb::um(hb::entityAufSchirm(*schiff), ImGui::GetFontSize() * 14.0F, ImGui::GetFontSize() * 9.0F);
    }));
    add(tu("Handbuch: Fenster zu, Auswahl weg", [] {
        g_app->animFensterOffen = false;
        entityAuswahlSetzen(-1);
        g_app->mapDirty = true;
    }));
    // Der Abflug als Bildfolge (shank: "manche GIFs sind vom Winkel her so
    // schlecht gewaehlt"): ohne Marker und Kacheln, die Kamera steht nah an der
    // Plattform und dreht sich in jedem Bild zum Schiff.
    add(tu("Handbuch: Marker aus fuer den Abflug", [] {
        g_app->showEntities = false;
        entityAuswahlSetzen(-1);
    }));
    add(pause(0.4));
    for (int k = 0; k < 40; ++k) {
        add(tu("Handbuch: Start " + std::to_string(k), [=] {
            g_app->playMs = 4800.0 + 150.0 * k;
            // Wo das Schiff gerade ist: sein Ursprung plus der Weg aus den
            // vier Schluesseln oben (linear dazwischen - fuer den Blick genug).
            float o[3] = {-1808.0F, -1400.0F, 624.0F};
            if (*schiff >= 0) { (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(*schiff)], o); }
            const double ms[4] = {5000.0, 6500.0, 8500.0, 10500.0};
            const float weg[4][3] = {{0, 0, 0}, {0, 0, 220}, {900, 0, 720}, {2600, 0, 1600}};
            const double t = g_app->playMs;
            int i = 0;
            while (i < 2 && t > ms[i + 1]) { ++i; }
            const float f = static_cast<float>(std::clamp((t - ms[i]) / (ms[i + 1] - ms[i]), 0.0, 1.0));
            for (int c = 0; c < 3; ++c) { o[c] += weg[i][c] + (weg[i + 1][c] - weg[i][c]) * f; }
            o[2] += 60.0F;
            const float kx = -1250.0F, ky = -2350.0F, kz = 820.0F;
            const float dx = o[0] - kx, dy = o[1] - ky, dz = o[2] - kz;
            const float gier = std::atan2(dy, dx) * 180.0F / 3.14159265F;
            const float nick = -std::atan2(dz, std::sqrt(dx * dx + dy * dy)) * 180.0F / 3.14159265F;
            blick(kx, ky, kz, nick, gier);
        }));
        add(pause(0.12));
        char n[32];
        std::snprintf(n, sizeof(n), "a_roff_%02d", k);
        add(hb::fotoTeil(n, karteMitZeitleiste));
        if (k == 22) { add(hb::fotoTeil("t3_5", [] { return hb::karte(); })); }
        if (k == 30) { add(hb::fotoTeil("t3_ergebnis", [] { return hb::karte(); })); }
    }
    add(tu("Handbuch: Marker wieder an", [] { g_app->showEntities = true; }));

    // --- 6. Die ganze Szene durch die Kamera ---------------------------------------------
    add(tu("Handbuch: Zeitleiste offen", [] {
        g_app->tlKameraOffen = true;
        g_app->tlOffen.clear();
        g_app->playMs = 3000.0;
        g_app->cameraSelected = true;
        g_app->mapDirty = true;
    }));
    add(pause(0.6));
    add(hb::fotoTeil("h_fenster", voll));
    add(hb::fotoTeil("h_willkommen", voll));
    add(hb::fotoTeil("t1_1", voll));
    add(hb::fotoTeil("h_zeitleiste", zeitleiste));
    add(hb::fotoTeil("h_karte", kartePlusLeiste));
    add(tu("Handbuch: Vorschau an", [] { g_app->kameraBildImBild = true; g_app->mapDirty = true; }));
    add(pause(0.6));
    add(hb::fotoTeil("t1_5", kartePlusLeiste));
    add(tu("Handbuch: durch die Kamera (ohne Auswahl: was sie am Zeiger sieht)", [] {
        g_app->kameraBildImBild = false;
        g_app->throughCamera = true;
        g_app->cameraSelected = false;
        g_app->selectedPath.clear();
        g_app->selection.clear();
        entityAuswahlSetzen(-1);
        g_app->mapDirty = true;
    }));
    for (int k = 0; k < 60; ++k) {
        add(tu("Handbuch: Kamerafahrt " + std::to_string(k), [k] {
            g_app->playMs = 200.0 * k;
            g_app->mapDirty = true;
        }));
        add(pause(0.12));
        char n[32];
        std::snprintf(n, sizeof(n), "a_kamera_%02d", k);
        add(hb::fotoTeil(n, [] { return hb::karte(); }));
        if (k == 14) { add(hb::fotoTeil("t1_ergebnis", [] { return hb::karte(); })); }
    }
    add(tu("Handbuch: Kamera aus", [] {
        g_app->throughCamera = false;
        g_app->kameraBildImBild = false;
        g_app->cameraSelected = false;
        g_app->mapDirty = true;
    }));
    // Zeitleiste: Kyles Laufbalken ziehen - Bildfolge, danach zurueck.
    add(tu("Handbuch: Figurenzeilen, Zeit 2 s", [=] {
        // Die Kameragruppe zu, damit die Laufbalken der Figuren im Bild sind.
        g_app->tlKameraOffen = false;
        g_app->showTracks = true;
        g_app->playMs = 2000.0;
        tiefe->first = g_app->doc.undoDepth();
        blick(-1620.0F, -2120.0F, 830.0F, 26.0F, 90.0F);
    }));
    add(pause(0.6));
    auto balken = std::make_shared<ImVec2>(-1.0F, -1.0F);
    add(tu("Handbuch: einen Laufbalken suchen", [=] {
        *balken = ImVec2{-1.0F, -1.0F};
        const ImRect band = hb::fensterRect("toolband");
        for (const App::TlBalken& b : g_app->tlBalken) {
            const float y = (b.a.y + b.b.y) * 0.5F;
            if (b.b.x - b.a.x > 30.0F && y > band.Min.y && y < band.Max.y) {
                *balken = ImVec2{b.a.x + (b.b.x - b.a.x) * 0.4F, y};
                break;
            }
        }
        diag::detail("Knopftest: Handbuch - Laufbalken bei " + std::to_string(balken->x));
        setzeMaus(*balken);
    }));
    add(pause(0.2));
    add({"Handbuch: Balken anfassen", [](int b) {
             if (b == 1) { ImGui::GetIO().AddMouseButtonEvent(0, true); }
             return b >= 3;
         }});
    for (int k = 0; k < 30; ++k) {
        add(tu("Handbuch: Balken ziehen", [=] {
            const float f = static_cast<float>(k + 1) / 30.0F;
            setzeMaus(ImVec2{balken->x + 140.0F * std::sin(f * 3.14159265F * 0.5F), balken->y});
        }));
        add(pause(0.05));
        char n[32];
        std::snprintf(n, sizeof(n), "a_k_timen_%02d", k);
        // Nur die belegten Zeilen um den gezogenen Balken, nicht die leere Flaeche darunter.
        add(hb::fotoTeil(n, [=] {
            ImRect r = zeitleiste();
            r.Max.y = std::min(r.Max.y, balken->y + ImGui::GetFontSize() * 6.0F);
            return r;
        }));
    }
    add({"Handbuch: Balken loslassen", [](int b) {
             if (b == 1) { ImGui::GetIO().AddMouseButtonEvent(0, false); }
             return b >= 3;
         }});
    add(pause(0.3));
    add(tu("Handbuch: Ziehen zuruecknehmen", [=] {
        while (g_app->doc.undoDepth() > tiefe->first && g_app->doc.undo()) {}
        hb::skriptNeu();
        g_app->showTracks = false;
    }));
    add(pause(0.4));

    // --- 7. Ein zweites Skript ueber den Inspektor: Kyles spawnscript -----------------------
    add(tu("Handbuch: Kyle bekommt ein spawnscript", [] {
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const std::string* n = g_app->map.entities[i].find("NPC_targetname");
            if (n != nullptr && *n == "kyle") {
                entityAuswahlSetzen(static_cast<int>(i));
                (void)entityKeySetzen(static_cast<int>(i), "spawnscript", "taspir_tut/kyle_spawn");
            }
        }
    }));
    add(pause(0.5));
    // Was der Knopf "Open" neben dem Schluessel tut: anlegen und oeffnen.
    add(tu("Handbuch: spawnscript oeffnen (anlegen)", [] { skriptOeffnen("taspir_tut/kyle_spawn", true); }));
    add(pause(0.8));
    add(pruefe("Handbuch: kyle_spawn ist vorn", [] {
        return mb::reiterName(static_cast<std::size_t>(g_app->activeTab)).find("kyle_spawn") != std::string::npos;
    }));
    add(tu("Handbuch: spawnscript mit standOnly-Makro", [=] {
        if (mb::reiterName(static_cast<std::size_t>(g_app->activeTab)).find("kyle_spawn") == std::string::npos) { return; }
        skriptSetzen("//Generated by BehavEd\n\n"
                     "//$\"standOnly\"@3\n"
                     "set ( /*!*/ \"SET_BEHAVIOR_STATE\", /*!*/ \"BS_DEFAULT\" );\n"
                     "set ( /*!*/ \"SET_IGNOREALERTS\", /*!*/ \"true\" );\n"
                     "set ( /*!*/ \"SET_LOOK_FOR_ENEMIES\", /*!*/ \"false\" );\n"
                     "set ( /*@SET_TYPES*/ \"SET_WEAPON\", \"WP_SABER\" );\n");
        g_app->leftMode = 0;
    }));
    add(pause(0.8));
    // Nur die belegten Zeilen - das ganze Feld war fast leer.
    add(hb::fotoTeil("k_makro", [] {
        ImRect r = hb::fensterRect("##flowarea");
        const float fs = ImGui::GetFontSize();
        r.Max.y = std::min(r.Max.y, r.Min.y + fs * 7.0F);
        r.Max.x = std::min(r.Max.x, r.Min.x + fs * 34.0F);
        return r;
    }));
    // Ansicht teilen: spawnscript neben dem Intro.
    add(klick(tr(Str::MenuFile), "###main"));
    add(klick(tr(Str::SplitToggle), "##Menu"));
    add(klick("2", "##Menu"));
    add(pause(0.8));
    add(hb::fotoTeil("k_geteilt", voll));
    add(klick(tr(Str::MenuFile), "###main"));
    add(klick(tr(Str::SplitToggle), "##Menu"));
    add(klick("1", "##Menu"));
    add(pause(0.4));
    add(hb::zuReiter("intro.txt"));
    add(pause(0.8));

    // --- 8. Skriptansichten, Editor, Suchen, Rueckgaengig --------------------------------
    add(tu("Handbuch: Events-Modus", [] { g_app->leftMode = 0; g_app->settings.flussAnsicht = 0; g_app->mapDirty = true; }));
    add(pause(0.8));
    add(hb::fotoTeil("h_skriptbaum", voll));
    add(hb::fotoTeil("t2_3", voll));
    add(hb::fotoTeil("k_neu_3", [] { return hb::fensterRect("##flowarea"); }));
    add(hb::fotoTeil("t1_6", [] {
        ImRect r = hb::fensterRect(tr(Str::GroupActions));
        return r.GetWidth() > 0.0F ? r : ImRect{ImGui::GetIO().DisplaySize.x * 0.82F, 0.0F, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y};
    }));
    add({"Handbuch: Editor fuer camera MOVE", [](int b) {
             if (b == 0) {
                 openEditorForNode(hb::kameraMove());
                 return false;
             }
             return g_app->editorOpen || b > 20;
         }});
    add(pause(0.6));
    add(hb::fotoTeil("h_editor", [] { return hb::fensterRect("###editor"); }));
    add(klick(tr(Str::EditorCancel), "###editor"));
    add(pause(0.3));
    add({"Handbuch: Editor fuer affect", [](int b) {
             if (b == 0) {
                 for (std::size_t i = 0; i < g_app->doc.script().nodes.size(); ++i) {
                     if (g_app->doc.script().nodes[i].name == "affect") { openEditorForNode(Path{i}); break; }
                 }
                 return false;
             }
             return g_app->editorOpen || b > 20;
         }});
    add(pause(0.6));
    add(hb::fotoTeil("k_neu_2", [] { return hb::fensterRect("###editor"); }));
    add(klick(tr(Str::EditorCancel), "###editor"));
    add(pause(0.3));
    // Suchen: jedes SET_NAVGOAL.
    add(tu("Handbuch: Suchen nach SET_NAVGOAL", [] {
        g_app->findVorName = "";
        g_app->findVorText = "SET_NAVGOAL";
        g_app->findVorbelegen = true;
        g_app->findOpen = true;
    }));
    add(pause(0.8));
    add(hb::fotoTeil("k_suchen", [] { return hb::fensterRect("###find"); }));
    add(tu("Handbuch: Suchen zu", [] { g_app->findOpen = false; }));
    add(pause(0.3));
    // Rueckgaengig-Liste: die Schritte bis hierher.
    add(tu("Handbuch: Rueckgaengig-Liste", [] { g_app->undoListeAnfrage = 1; }));
    add(pause(0.8));
    add(hb::fotoTeil("k_undo", [] { return hb::fensterRect(tr(Str::UndoListTitle)); }));
    add(klickAn("Liste zu", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
    add(pause(0.3));
    add(tu("Handbuch: Graph", [] { g_app->settings.flussAnsicht = 1; }));
    add(pause(1.2));
    add(hb::fotoTeil("h_graph", [] { return hb::fensterRect("##flowarea"); }));
    add(tu("Handbuch: Spuren", [] { g_app->settings.flussAnsicht = 2; }));
    add(pause(0.8));
    add(hb::fotoTeil("h_spuren", [] { return hb::fensterRect("##flowarea"); }));
    add(hb::fotoTeil("k_dialog", [] { return hb::fensterRect("##flowarea"); }));
    // Ein neuer, leerer Reiter fuer "Skript von vorn".
    add(tu("Handbuch: neuer Reiter", [] { g_app->settings.flussAnsicht = 0; neuerReiter(); g_app->leftMode = 0; }));
    add(pause(0.8));
    add(hb::fotoTeil("k_neu_1", [] {
        const ImVec2 d = ImGui::GetIO().DisplaySize;
        return ImRect{0.0F, 0.0F, d.x * 0.62F, d.y * 0.55F};
    }));
    add(hb::zuReiter("intro.txt"));
    add(tu("Handbuch: Karte", [] { g_app->leftMode = 1; }));
    add(pause(1.0));

    // --- 9. Ansicht: Draufsicht, Einstellungen, Kameramarke ------------------------------
    add(tu("Handbuch: Draufsicht", [=] {
        blick(-1600.0F, -1450.0F, 3200.0F, 89.0F, 90.0F);
        g_app->draufSchnitt = 1000.0F;
        g_app->draufsicht = true;
        g_app->mapDirty = true;
    }));
    add(pause(1.0));
    add(hb::fotoTeil("k_oben", kartePlusLeiste));
    add(tu("Handbuch: Draufsicht aus, Ansicht einstellen", [=] {
        g_app->draufsicht = false;
        blick(-1560.0F, -2280.0F, 900.0F, 23.0F, 90.0F);
        g_app->ansichtEinstellungenOeffnen = true;
    }));
    add(pause(0.6));
    add(hb::fotoTeil("h_ansicht_einst", [] {
        ImRect r = hb::fensterRect("##Popup");
        r.Expand(ImGui::GetFontSize() * 0.6F);
        return r;
    }));
    add(klickAn("Klappfenster zu", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
    add(pause(0.3));
    // rc601 integ (Handbuch): der Vorschau-Teil der Einstellungen - Lautstaerken
    // (ton), Spielregeln (integ) und Schwierigkeit (bewegung); Seite
    // einstellungen#prefs. Nur der untere Teil des Fensters, ab den Pegeln.
    add(tu("Handbuch: Einstellungen oeffnen", [] { g_app->prefsOpen = true; }));
    add(pause(0.6));
    add(hb::fotoTeil("h_einst_vorschau", [] {
        ImRect r = hb::fensterRect(tr(Str::PrefsTitle));
        const ImRect pegel = hb::knopf(tr(Str::PrefsTonEffekte), tr(Str::PrefsTitle));
        if (pegel.GetHeight() > 1.0F) { r.Min.y = std::max(r.Min.y, pegel.Min.y - ImGui::GetFontSize() * 2.0F); }
        r.Expand(ImGui::GetFontSize() * 0.4F);
        return r;
    }));
    add(klick(tr(Str::EditorCancel), tr(Str::PrefsTitle)));
    add(pause(0.3));
    // Eine Kameramarke und ihr Menue in der Liste.
    add(tu("Handbuch: Kameramarke setzen", [] {
        const float o[3] = {-1150.0F, -2250.0F, 820.0F};
        const int i = neueEntityAn("ref_tag", o);
        if (i >= 0) { (void)entityKeySetzen(i, "targetname", "cam_stream"); }
        g_app->expandedClass = "kat0";
        g_app->hiddenClasses.clear();
    }));
    add(pause(0.6));
    add(klick("##entsuche"));
    add(tippe("cam_stream"));
    add(pause(0.4));
    add(klick("cam_stream", "", 0, false, "", 1));
    add(pause(0.5));
    add(hb::fotoTeil("k_marke", [] {
        ImRect r = hb::fensterRect("##Popup");
        r = hb::vereint(r, ImRect{0.0F, g_app->kartenAnsicht[1], g_app->kartenAnsicht[0], g_app->kartenAnsicht[1] + 10.0F});
        r.Expand(ImGui::GetFontSize() * 0.6F);
        return r;
    }));
    add(klickAn("Menue zu", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
    add(pause(0.3));
    add(klick("##entsuche"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(taste(ImGuiKey_Backspace, "Rueck"));
    // Nicht Esc - das stellt im Eingabefeld den alten Text wieder her.
    add(klickAn("daneben", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
    add(pause(0.3));

    // --- 10. Wegpunkte ---------------------------------------------------------------------
    add(tu("Handbuch: Wegpunkte", [] { g_app->wegFensterOffen = true; g_app->mapDirty = true; }));
    add(pause(0.8));
    add(hb::fotoTeil("h_wegpunkte", [=] {
        ImRect r = kartePlusLeiste();
        r.Add(hb::fensterRect(tr(Str::WegTitel)));
        return r;
    }));
    add(tu("Handbuch: Wegpunkte zu", [] { g_app->wegFensterOffen = false; }));

    // --- 10b. Bilder fuer die zweite Runde der Kurz-Tutorials (rc600) --------------
    // shank: "mach eigene Bilder fuer die". Alles in der Taspir-Mission, nur fuer
    // die Fotos - Skript und Entities kommen danach per Rueckgaengig zurueck.
    auto vorher = std::make_shared<std::pair<std::size_t, std::size_t>>();
    add(tu("Kurz2: Stand merken", [=] {
        vorher->first = g_app->doc.undoDepth();
        vorher->second = g_app->entUndo.size();
        g_app->leftMode = 0;
        g_app->settings.flussAnsicht = 0;
    }));
    // Der belegte Teil des Skriptbaums: so hoch wie seine Zeilen, die linke
    // Haelfte der Spalte - der Rest waere leere Flaeche.
    const auto baumRect = [] {
        ImRect r = hb::fensterRect("##flowarea");
        const float zeile = ImGui::GetFrameHeight();
        r.Max.y = std::min(r.Max.y, r.Min.y + zeile * (static_cast<float>(g_app->rows.size()) + 3.5F));
        r.Max.x = r.Min.x + r.GetWidth() * 0.62F;
        return r;
    };
    const auto baumFoto = [&](const std::string& name, const std::string& text) {
        add(tu("Kurz2: Skript fuer " + name, [=] {
            setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x * 0.5F, ImGui::GetIO().DisplaySize.y - 4.0F});
            skriptSetzen(text);
            g_app->leftMode = 0;
            g_app->expanded.openAll();
            rebuildTree();
        }));
        add(pause(0.6));
        add(hb::fotoTeil(name, baumRect));
    };
    const auto editorFoto = [&](const std::string& name, const std::string& befehl, const std::string& wert = "") {
        add({"Kurz2: Editor " + befehl, [befehl, wert](int b) {
                 if (b == 0) {
                     const std::vector<Node>& ns = g_app->doc.script().nodes;
                     for (std::size_t i = 0; i < ns.size(); ++i) {
                         const bool passt = wert.empty() || (!ns[i].args.empty() && ns[i].args[0].text == wert);
                         if (ns[i].name == befehl && passt) { openEditorForNode(Path{i}); break; }
                     }
                     return false;
                 }
                 return g_app->editorOpen || b > 20;
             }});
        add(pause(0.6));
        add(hb::fotoTeil(name, [] { return hb::fensterRect("###editor"); }));
        add(klick(tr(Str::EditorCancel), "###editor"));
        add(pause(0.3));
    };
    const std::string kopfKurz = "//Generated by BehavEd\n\n";
    // Variablen und if
    baumFoto("k_var", kopfKurz +
                          "declare ( /*@DECLARE_TYPE*/ FLOAT, \"seen\" );\n"
                          "if ( $get( FLOAT, \"seen\")$, $=$, $0$ )\n{\n"
                          "\trem ( \"the scene - only the first time\" );\n"
                          "\tcamera ( /*@CAMERA_COMMANDS*/ ENABLE );\n"
                          "\twait ( 3000.000 );\n"
                          "\tcamera ( /*@CAMERA_COMMANDS*/ DISABLE );\n"
                          "\tset ( \"seen\", \"1\" );\n}\n\n"
                          "else (  )\n{\n"
                          "\tprint ( \"You were here already.\" );\n}\n\n");
    editorFoto("k_var_if", "if");
    // Video und Text
    baumFoto("k_video", kopfKurz +
                            "camera ( /*@CAMERA_COMMANDS*/ ENABLE );\n"
                            "set ( /*@SET_TYPES*/ \"SET_VIDEO_FADE_IN\", \"true\" );\n"
                            "set ( /*@SET_TYPES*/ \"SET_VIDEO_PLAY\", \"jk0101_sw\" );\n"
                            "print ( \"Three days later...\" );\n"
                            "wait ( 3000.000 );\n"
                            "camera ( /*@CAMERA_COMMANDS*/ DISABLE );\n");
    editorFoto("k_video_edit", "set", "SET_VIDEO_PLAY");
    // Kampf nach der Szene
    baumFoto("k_kampf", kopfKurz +
                            "camera ( /*@CAMERA_COMMANDS*/ DISABLE );\n"
                            "affect ( \"rosh\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                            "\tset ( /*@SET_TYPES*/ \"SET_IGNOREENEMIES\", \"false\" );\n"
                            "\tset ( /*@SET_TYPES*/ \"SET_LOOK_FOR_ENEMIES\", \"true\" );\n"
                            "\tset ( /*@SET_TYPES*/ \"SET_ENEMY\", \"player\" );\n}\n\n");
    // Spieler festhalten
    baumFoto("k_sperren", kopfKurz +
                              "affect ( \"player\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                              "\tset ( /*@SET_TYPES*/ \"SET_PLAYER_LOCKED\", \"true\" );\n"
                              "\tset ( /*@SET_TYPES*/ \"SET_LOCK_PLAYER_WEAPONS\", \"true\" );\n}\n\n"
                              "wait ( 5000.000 );\n"
                              "affect ( \"player\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                              "\tset ( /*@SET_TYPES*/ \"SET_PLAYER_LOCKED\", \"false\" );\n"
                              "\tset ( /*@SET_TYPES*/ \"SET_LOCK_PLAYER_WEAPONS\", \"false\" );\n}\n\n");
    // Lichtschwert: Kyle zuendet sein Schwert - in der Szene, nah gesehen.
    add(tu("Kurz2: Lichtschwert", [=] {
        skriptSetzen(kopf +
                     "affect ( \"kyle\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_WEAPON\", \"WP_SABER\" );\n"
                     "\twait ( 1000.000 );\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_SABERACTIVE\", \"true\" );\n}\n\n"
                     "wait ( 6000.000 );\ncamera ( /*@CAMERA_COMMANDS*/ DISABLE );\n");
        g_app->leftMode = 1;
        g_app->cameraSelected = false;
        g_app->throughCamera = false;
        g_app->playMs = 3000.0;
        // Ohne Marker und Auswahl - sonst liegt Kyles Kreis ueber dem Schwert.
        g_app->showEntities = false;
        entityAuswahlSetzen(-1);
        g_app->selectedActor = -1;
        blick(-1370.0F, -1960.0F, 700.0F, 8.0F, 168.0F);
    }));
    add(warteBis("Kurz2: Szene", [] { return g_app->camTrackValid; }, 900));
    add(pause(1.0));
    add(tu("Kurz2: Kyles Schwert ins Protokoll", [] {
        for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
            const Actor& a = g_app->scene.actors[ai];
            if (a.name != "kyle") { continue; }
            const ActorState st = a.at(3000.0);
            diag::detail("Knopftest: Kurz2 - kyle Waffe \"" + st.waffe + "\", an " + std::to_string(st.saberActive) +
                         ", Klingen " + std::to_string(st.saber[0].numBlades) + ", Art " + std::to_string(st.saber[0].art) +
                         ", Laenge " + std::to_string(klingenLaenge(st, 0, 0, 3000.0)));
        }
    }));
    add(hb::fotoTeil("k_saber", karteMitZeitleiste));
    add(tu("Kurz2: Marker wieder an", [] { g_app->showEntities = true; }));
    add(tu("Kurz2: Lichtschwert - Baum", [] { g_app->leftMode = 0; }));
    add(pause(0.6));
    add(tu("Kurz2: Baum aufklappen", [] { g_app->expanded.openAll(); rebuildTree(); }));
    add(pause(0.4));
    add(hb::fotoTeil("k_saber_baum", baumRect));
    // Kamera folgt Kyle
    add(tu("Kurz2: Kamera folgt", [=] {
        skriptSetzen(kopf +
                     "affect ( \"kyle\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_CAMERA_GROUP\", \"hero\" );\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_WALKING\", \"true\" );\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_NAVGOAL\", \"nav_kyle\" );\n}\n\n"
                     "camera ( /*@CAMERA_COMMANDS*/ MOVE, < -1580.000 -2040.000 700.000 >, 0 );\n"
                     "camera ( /*@CAMERA_COMMANDS*/ FOLLOW, \"hero\", 100.000, 1 );\n"
                     "wait ( 6000.000 );\ncamera ( /*@CAMERA_COMMANDS*/ DISABLE );\n");
        g_app->leftMode = 1;
        g_app->tlKameraOffen = true;
        g_app->playMs = 3000.0;
        g_app->cameraSelected = false;
        g_app->selectedPath.clear();
        g_app->selection.clear();
        entityAuswahlSetzen(-1);
        g_app->throughCamera = true;
    }));
    add(warteBis("Kurz2: Szene", [] { return g_app->camTrackValid; }, 900));
    add(pause(1.0));
    add(hb::fotoTeil("k_folgen_blick", [] { return hb::karte(); }));
    add(tu("Kurz2: Kamera aus", [] { g_app->throughCamera = false; }));
    // Patrouille: eine Wache, drei Punkte auf der Plattform, eine Schleife.
    add(tu("Kurz2: Wache und Runde", [=] {
        const float pkt[4][3] = {{-2000.0F, -1700.0F, 650.0F}, {-2050.0F, -1150.0F, 650.0F},
                                 {-1450.0F, -1050.0F, 650.0F}, {-1350.0F, -1650.0F, 650.0F}};
        const char* namen[4] = {"guard", "round1", "round2", "round3"};
        for (int i = 0; i < 4; ++i) {
            const int e = neueEntityAn(i == 0 ? "NPC_spawner" : "waypoint_navgoal", pkt[i]);
            if (e < 0) { continue; }
            (void)entityKeySetzen(e, i == 0 ? "NPC_targetname" : "targetname", namen[i]);
            if (i == 0) { (void)entityKeySetzen(e, "NPC_type", "stormtrooper"); }
            (void)entityAufBoden(e);
        }
        std::string runde;
        for (int i = 1; i <= 3; ++i) {
            const std::string n = "round" + std::to_string(i);
            runde += "\t\ttask ( \"" + n + "\" )\n\t\t{\n"
                     "\t\t\tset ( /*@SET_TYPES*/ \"SET_NAVGOAL\", \"" + n + "\" );\n\t\t}\n\n"
                     "\t\tdowait ( \"" + n + "\" );\n"
                     "\t\twait ( 1000.000 );\n";
        }
        skriptSetzen(kopf +
                     "affect ( \"guard\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                     "//$\"walkOnly\"@5\n"
                     "\tset ( /*!*/ \"SET_BEHAVIOR_STATE\", /*!*/ \"BS_DEFAULT\" );\n"
                     "\tset ( /*!*/ \"SET_WALKING\", /*!*/ \"true\" );\n"
                     "\tset ( /*!*/ \"SET_RUNNING\", /*!*/ \"false\" );\n"
                     "\tset ( /*!*/ \"SET_IGNOREALERTS\", /*!*/ \"true\" );\n"
                     "\tset ( /*!*/ \"SET_LOOK_FOR_ENEMIES\", /*!*/ \"false\" );\n"
                     "\tloop ( -1 )\n\t{\n" + runde + "\t}\n\n}\n\n"
                     "wait ( 12000.000 );\ncamera ( /*@CAMERA_COMMANDS*/ DISABLE );\n");
        g_app->leftMode = 1;
        g_app->playMs = 4000.0;
        for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) { g_app->selectedActor = static_cast<int>(ai); }
        blick(-1700.0F, -2750.0F, 1300.0F, 34.0F, 90.0F);
    }));
    add(warteBis("Kurz2: Szene", [] { return g_app->camTrackValid; }, 900));
    add(tu("Kurz2: die Wache waehlen", [] {
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const std::string* n = g_app->map.entities[i].find("NPC_targetname");
            if (n != nullptr && *n == "guard") { entityAuswahlSetzen(static_cast<int>(i)); }
        }
        const int ei = g_app->pickedEntity;
        for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
            if (entityFuerFigur(static_cast<int>(ai)) == ei) { g_app->selectedActor = static_cast<int>(ai); }
        }
        g_app->mapDirty = true;
    }));
    add(pause(1.0));
    add(hb::fotoTeil("k_runde", kartePlusLeiste));
    add(tu("Kurz2: Runde - Baum", [] { g_app->leftMode = 0; }));
    add(pause(0.6));
    add(tu("Kurz2: Baum aufklappen", [] { g_app->expanded.openAll(); rebuildTree(); }));
    add(pause(0.4));
    add(hb::fotoTeil("k_runde_baum", baumRect));
    // Trigger: ein trigger_once der Karte zeigt auf einen eigenen Runner.
    auto runner = std::make_shared<int>(-1);
    add(tu("Kurz2: Runner setzen, Trigger umlenken", [=] {
        g_app->leftMode = 1;
        const float o[3] = {-1250.0F, -1900.0F, 660.0F};
        *runner = neueEntityAn("target_scriptrunner", o);
        if (*runner >= 0) {
            (void)entityKeySetzen(*runner, "targetname", "start_talk");
            (void)entityKeySetzen(*runner, "usescript", "taspir_tut/talk");
        }
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const MapEntity& e = g_app->map.entities[i];
            const std::string* t = e.find("target");
            if (e.classname == "trigger_once" && t != nullptr && *t == "run_convo") {
                (void)entityKeySetzen(static_cast<int>(i), "target", "start_talk");
                entityAuswahlSetzen(static_cast<int>(i));
                // Die Zeitleiste klein: der Inspektor bekommt die Hoehe, damit
                // "target" im Bild steht.
                g_app->timelineFrac = 0.10F;
                break;
            }
        }
        g_app->mapDirty = true;
    }));
    add(pause(0.6));
    add(klick("##feldsuche"));
    add(tippe("target"));
    add(pause(0.5));
    add(hb::fotoTeil("k_trigger", [] {
        const ImRect band = hb::fensterRect("toolband");
        return ImRect{0.0F, g_app->kartenAnsicht[1] - ImGui::GetFontSize() * 2.6F, g_app->kartenAnsicht[0], band.Min.y - 2.0F};
    }));
    add(klick("##feldsuche"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(taste(ImGuiKey_Backspace, "Rueck"));
    // Nicht Esc - das stellt im Eingabefeld den alten Text wieder her.
    add(klickAn("daneben", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
    add(tu("Kurz2: Zeitleiste wieder gross", [] { g_app->timelineFrac = 0.40F; }));
    // Tuer: Rechtsklick auf die Tuer in der Liste -> "Insert use". Der Name
    // ist eindeutig (bei "building1_1" trafen auch sechs NPCs building1_1b).
    add(klick("##entsuche"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(tippe("bldg2_ext_door"));
    add(pause(0.4));
    add(tu("Kurz2: Tuer-Gruppe aufklappen", [] {
        for (int k = 0; k < entityKategorien(); ++k) {
            if (entityKategorieInfo(k).titel == Str::EntityKatMover) { g_app->expandedClass = "kat" + std::to_string(k); }
        }
    }));
    add(pause(0.4));
    add(klickAn("Rechtsklick auf die Tuer in der Liste", [] {
        const float fs = ImGui::GetFontSize();
        for (const Element& e : g_elemente) {
            if (sichtbarerText(e.label.c_str()) == "bldg2_ext_door" && e.innen.find("entlayers") != std::string::npos &&
                e.rect.GetHeight() < fs * 2.5F) {
                return e.rect.GetCenter();
            }
        }
        return ImVec2{-1.0F, -1.0F};
    }, 1));
    add(pause(0.5));
    add(hb::fotoTeil("k_tuer_menue", [] {
        ImRect r = hb::fensterRect("##Popup");
        r = hb::vereint(r, ImRect{0.0F, g_app->kartenAnsicht[1], g_app->kartenAnsicht[0], g_app->kartenAnsicht[1] + 10.0F});
        r.Expand(ImGui::GetFontSize() * 0.6F);
        return r;
    }));
    add(klickAn("Menue zu", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
    add(pause(0.3));
    add(klick("##entsuche"));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
    add(taste(ImGuiKey_Backspace, "Rueck"));
    // Nicht Esc - das stellt im Eingabefeld den alten Text wieder her.
    add(klickAn("daneben", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
    add(pause(0.3));
    // Levelwechsel: placed, mapname gesetzt, im Inspektor.
    add(tu("Kurz2: Levelwechsel", [] {
        const float o[3] = {-1300.0F, -1100.0F, 660.0F};
        const int i = neueEntityAn("target_level_change", o);
        if (i >= 0) {
            (void)entityKeySetzen(i, "targetname", "next_map");
            (void)entityKeySetzen(i, "mapname", "taspir2");
            entityAuswahlSetzen(i);
        }
        g_app->mapDirty = true;
    }));
    add(pause(0.8));
    add(hb::fotoTeil("k_level", [] {
        const ImRect band = hb::fensterRect("toolband");
        return ImRect{0.0F, g_app->kartenAnsicht[1] - ImGui::GetFontSize() * 2.6F, g_app->kartenAnsicht[0], band.Min.y - 2.0F};
    }));
    // Effekt: ein Feuer auf der Plattform.
    add(tu("Kurz2: Effekt", [=] {
        const float o[3] = {-1450.0F, -1850.0F, 650.0F};
        const int i = neueEntityAn("fx_runner", o);
        if (i >= 0) {
            (void)entityKeySetzen(i, "targetname", "fire1");
            (void)entityKeySetzen(i, "fxFile", "env/fire");
            (void)entityAufBoden(i);
        }
        entityAuswahlSetzen(-1);
        g_app->showEffects = true;
        g_app->showEntities = false;
        blick(-1450.0F, -2080.0F, 730.0F, 14.0F, 90.0F);
    }));
    add(warteBis("Kurz2: Szene mit dem Effekt", [] { return g_app->camTrackValid; }, 900));
    add(pause(1.5));
    add(tu("Kurz2: Effekte ins Protokoll", [] {
        diag::detail("Knopftest: Kurz2 - Effekte der Karte: " + std::to_string(g_app->effectRunners.size()));
    }));
    add(hb::fotoTeil("k_effekt", kartePlusLeiste));
    add(tu("Kurz2: Marker wieder an", [] { g_app->showEntities = true; }));
    // Multiplayer-Regeln: das Meldungsfenster.
    add(tu("Kurz2: Multiplayer-Regeln, Intro mit Kamera", [=] {
        skriptSetzen(kopf + laeufe + kamera);
        g_app->settings.mpZiel = true;
        rebuildTree();
        g_app->leftMode = 0;
        g_app->messagesOpen = true;
    }));
    add(pause(1.0));
    add(hb::fotoTeil("k_mp", [] {
        ImRect r = hb::fensterRect("###messages");
        const ImVec2 d = ImGui::GetIO().DisplaySize;
        r = hb::vereint(r, ImRect{0.0F, d.y - ImGui::GetFontSize() * 1.8F, d.x * 0.5F, d.y});
        r.Expand(ImGui::GetFontSize() * 0.4F);
        return r;
    }));
    add(tu("Kurz2: Einzelspieler", [] {
        g_app->settings.mpZiel = false;
        g_app->messagesOpen = false;
        rebuildTree();
    }));
    // Tastenkuerzel
    add(tu("Kurz2: Tastenfenster", [] { g_app->keysOpen = true; }));
    add(pause(0.8));
    add(hb::fotoTeil("k_tasten", [] { return hb::fensterRect(tr(Str::KeysTitle)); }));
    add(tu("Kurz2: Tastenfenster zu", [] { g_app->keysOpen = false; }));
    // Alles zurueck.
    add(tu("Kurz2: zurueck", [=] {
        while (g_app->doc.undoDepth() > vorher->first && g_app->doc.undo()) {}
        while (g_app->entUndo.size() > vorher->second && entityRueckgaengig()) {}
        hb::skriptNeu();
        entityAuswahlSetzen(-1);
        g_app->leftMode = 1;
        g_app->tlKameraOffen = true;
    }));
    add(pause(0.8));

    // --- 10c. Bilder fuer die dritte Runde der Kurz-Tutorials (rc600) --------------
    // Im Intro geaendert und am Ende per Rueckgaengig zurueck; ein zweiter Reiter
    // taspir_tut/talk bleibt offen (er gehoert dann zur Mission).
    auto vorher3 = std::make_shared<std::pair<std::size_t, std::size_t>>();
    add(tu("Kurz3: Stand merken", [=] {
        vorher3->first = g_app->doc.undoDepth();
        vorher3->second = g_app->entUndo.size();
        g_app->leftMode = 1;
        g_app->settings.flussAnsicht = 0;
    }));
    const auto inspektorRect = [] {
        const ImRect band = hb::fensterRect("toolband");
        return ImRect{0.0F, g_app->kartenAnsicht[1] - ImGui::GetFontSize() * 2.6F, g_app->kartenAnsicht[0], band.Min.y - 2.0F};
    };
    const auto waehleNpc = [](const char* npcName) {
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const std::string* n = g_app->map.entities[i].find("NPC_targetname");
            if (n != nullptr && *n == npcName) {
                entityAuswahlSetzen(static_cast<int>(i));
                return static_cast<int>(i);
            }
        }
        return -1;
    };
    const auto feldFilter = [&](const std::string& was) {
        add(klick("##feldsuche"));
        add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
        if (!was.empty()) {
            add(tippe(was));
        } else {
            add(taste(ImGuiKey_Backspace, "Rueck"));
            add(klickAn("daneben", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
        }
        add(pause(0.4));
    };
    // 1. Deathscript an Rosh, im Inspektor.
    add(tu("Kurz3: Rosh bekommt ein deathscript", [=] {
        const int i = waehleNpc("rosh");
        if (i >= 0) { (void)entityKeySetzen(i, "deathscript", "taspir_tut/rosh_dies"); }
        g_app->timelineFrac = 0.10F;
    }));
    add(pause(0.5));
    feldFilter("script");
    add(hb::fotoTeil("k_tod_insp", inspektorRect));
    feldFilter("");
    // 12. Spawnflags und targetname: Rosh wartet auf seinen Einsatz.
    add(tu("Kurz3: Roshs Spawner bekommt einen targetname", [=] {
        const int i = waehleNpc("rosh");
        if (i >= 0) { (void)entityKeySetzen(i, "targetname", "rosh_spawner"); }
    }));
    add(pause(0.6));
    add(hb::fotoTeil("k_spawnflags", inspektorRect));
    add(tu("Kurz3: Zeitleiste wieder gross", [] { g_app->timelineFrac = 0.40F; }));
    // 5. Save mission mit einem Fehler (S001) und der Warnung der Karte (S005).
    add(tu("Kurz3: Kyle zeigt auf ein fehlendes Skript", [=] {
        const int i = waehleNpc("kyle");
        if (i >= 0) { (void)entityKeySetzen(i, "painscript", "taspir_tut/ouch"); }
        const char* ziel = std::getenv("BHED_HB_ZIEL");
        g_app->mission.zielPk3 = ziel != nullptr ? std::string(ziel) : mb::verbinde(mb::laufOrdner(), "zz_taspir_tut.pk3");
        g_app->missionSpeichernOffen = true;
    }));
    add(pause(0.8));
    add(hb::fotoTeil("k_pruefen", [] { return hb::fensterRect("##missionspeichern"); }));
    add(tu("Kurz3: Fenster zu", [] { g_app->missionSpeichernOffen = false; }));
    // 11. Rahmenauswahl: Kyle, Rosh und die beiden Laufziele.
    add(tu("Kurz3: Blick ueber die Plattform", [=] {
        entityAuswahlSetzen(-1);
        g_app->cameraSelected = false;
        g_app->selectedActor = -1;
        g_app->gizmoMode = App::GizmoMode::Move;
        blick(-1600.0F, -2700.0F, 1250.0F, 32.0F, 90.0F);
    }));
    add(pause(0.8));
    auto ecke = std::make_shared<ImVec2>();
    hb::markiere("k_rahmen", [=] {
        const ImVec2 z = schirm(-1400.0F, -1560.0F, 640.0F);
        ImRect r{*ecke, *ecke};
        r.Add(z);
        return std::vector<hb::Marke>{hb::Marke(r)};
    });
    add(tu("Kurz3: Rahmen berechnen", [=] {
        const float fs = ImGui::GetFontSize();
        *ecke = ImVec2{g_app->kartenAnsicht[0] + fs * 6.0F, g_app->kartenAnsicht[3] - fs * 1.5F};
    }));
    add({"Kurz3: Rahmen bis hinter die Laufziele", [=](int b) {
             ImGuiIO& io = ImGui::GetIO();
             const ImVec2 z = schirm(-1400.0F, -1560.0F, 640.0F);
             if (b == 0) { setzeMaus(*ecke); return false; }
             if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
             if (b >= 3 && b <= 14) {
                 const float f = static_cast<float>(b - 2) / 12.0F;
                 setzeMaus(ImVec2{ecke->x + (z.x - ecke->x) * f, ecke->y + (z.y - ecke->y) * f});
                 return false;
             }
             if (b == 15) { io.AddMouseButtonEvent(0, false); return false; }
             return b >= 20;
         }});
    add(pause(0.8));
    add(tu("Kurz3: Auswahl ins Protokoll", [] {
        int n = 0;
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) { n += entityIstGewaehlt(static_cast<int>(i)) ? 1 : 0; }
        diag::detail("Knopftest: Kurz3 - Rahmen waehlte " + std::to_string(n) + " Entities");
        setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x * 0.5F, ImGui::GetIO().DisplaySize.y - 4.0F});
    }));
    add(pause(0.4));
    add(hb::fotoTeil("k_rahmen", kartePlusLeiste));
    add(tu("Kurz3: Auswahl weg", [] { entityAuswahlSetzen(-1); }));
    // 13. Das Menue "Karten und Dateien".
    add(tu("Kurz3: Karten-und-Dateien-Menue", [] { g_app->kartenMehrOeffnen = true; }));
    add(pause(0.6));
    add(hb::fotoTeil("k_kartenmenue", [] {
        ImRect r = hb::fensterRect("##Popup");
        const float fs = ImGui::GetFontSize();
        r = hb::vereint(r, ImRect{g_app->kartenAnsicht[0], g_app->kartenAnsicht[1] - fs * 2.6F, g_app->kartenAnsicht[0] + fs * 20.0F,
                                  g_app->kartenAnsicht[1]});
        r.Expand(fs * 0.5F);
        return r;
    }));
    add(klickAn("Menue zu", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
    add(pause(0.3));
    // 11b. Alle Kamerabahnen.
    add(tu("Kurz3: alle Kamerabahnen", [=] {
        g_app->cameraSelected = true;
        g_app->kamBahnNurAktiv = false;
        kameraSchluesselWaehlen(1, true);
        blick(-1100.0F, -3300.0F, 1500.0F, 24.0F, 112.0F);
    }));
    add(pause(1.0));
    add(hb::fotoTeil("k_bahnen", linksUndKarte));
    add(tu("Kurz3: Kamera ab", [] { g_app->cameraSelected = false; }));
    // 15. Zeitleiste: Waits als Bloecke, ruhige Spuren.
    add(tu("Kurz3: Waits und ruhige Spuren", [] {
        g_app->tlWaitsZeigen = true;
        g_app->zeigeRuhigeSpuren = true;
        g_app->tlKameraOffen = false;
        g_app->showTracks = true;
        g_app->playMs = 5000.0;
    }));
    add(pause(0.8));
    add(hb::fotoTeil("k_waits", zeitleiste));
    add(tu("Kurz3: Waits aus", [] {
        g_app->tlWaitsZeigen = false;
        g_app->zeigeRuhigeSpuren = false;
        g_app->tlKameraOffen = true;
    }));
    // 7. Der Aenderungsrand im Intro (es ist seit dem Laden geaendert).
    add(tu("Kurz3: Events-Modus, Rand an", [] {
        g_app->leftMode = 0;
        g_app->settings.changeHistory = true;
        g_app->expanded.openAll();
        rebuildTree();
        setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x * 0.5F, ImGui::GetIO().DisplaySize.y - 4.0F});
    }));
    add(pause(0.6));
    add(hb::fotoTeil("k_rand", baumRect));
    // 9. Ein Tippfehler im Feld und das Meldungsfenster.
    add(tu("Kurz3: Tippfehler SET_WALKNG", [=] {
        skriptSetzen(kopf +
                     "affect ( \"kyle\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_WALKNG\", \"true\" );\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_NAVGOAL\", \"nav_kyle\" );\n}\n\n"
                     "wait ( 4000.000 );\ncamera ( /*@CAMERA_COMMANDS*/ DISABLE );\n");
        g_app->messagesOpen = true;
    }));
    add(pause(1.0));
    add(hb::fotoTeil("k_meldung", [] {
        ImRect r = hb::fensterRect("###messages");
        const ImVec2 d = ImGui::GetIO().DisplaySize;
        r = hb::vereint(r, ImRect{0.0F, d.y - ImGui::GetFontSize() * 1.8F, d.x * 0.5F, d.y});
        r.Expand(ImGui::GetFontSize() * 0.4F);
        return r;
    }));
    add(tu("Kurz3: Meldungen zu", [] { g_app->messagesOpen = false; }));
    // 8. Das Dateimenue (Backup-Ordner, Backup, Restore stehen in der Spalte).
    add(klick(tr(Str::MenuFile), "###main"));
    add(pause(0.5));
    add(hb::fotoTeil("k_dateimenue", [] {
        ImRect r{0.0F, 0.0F, 0.0F, 0.0F};
        for (ImGuiWindow* w : GImGui->Windows) {
            if (w->WasActive && !w->Hidden && std::strstr(w->Name, "##Menu") != nullptr) { r = hb::vereint(r, w->Rect()); }
        }
        r.Expand(ImGui::GetFontSize() * 0.5F);
        r.Min.y = 0.0F;
        return r;
    }));
    add(klickAn("Menue zu", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
    add(pause(0.3));
    // Intro und Entities zurueck - BEVOR ein anderer Reiter vorn ist (die
    // Rueckgaengig-Liste gehoert zum Reiter).
    add(tu("Kurz3: Intro und Entities zurueck", [=] {
        while (g_app->doc.undoDepth() > vorher3->first && g_app->doc.undo()) {}
        while (g_app->entUndo.size() > vorher3->second && entityRueckgaengig()) {}
        hb::skriptNeu();
        entityAuswahlSetzen(-1);
    }));
    add(pause(0.5));
    // 1b/2/3. Ein zweites Skript taspir_tut/talk: deathscript-Inhalt,
    // Spawn mit SET_PLAYER_USABLE, dann die Uebergabe per signal.
    add(tu("Kurz3: Skript talk anlegen", [] { skriptOeffnen("taspir_tut/talk", true); }));
    add(pause(0.8));
    add(pruefe("Kurz3: talk ist vorn", [] {
        return mb::reiterName(static_cast<std::size_t>(g_app->activeTab)).find("talk") != std::string::npos;
    }));
    baumFoto("k_tod_baum", kopfKurz +
                               "rem ( \"rosh_dies - runs when Rosh falls\" );\n"
                               "wait ( 1000.000 );\n"
                               "use ( \"bldg2_ext_door\" );\n");
    baumFoto("k_use_baum", kopfKurz +
                               "//$\"standOnly\"@3\n"
                               "set ( /*!*/ \"SET_BEHAVIOR_STATE\", /*!*/ \"BS_DEFAULT\" );\n"
                               "set ( /*!*/ \"SET_IGNOREALERTS\", /*!*/ \"true\" );\n"
                               "set ( /*!*/ \"SET_LOOK_FOR_ENEMIES\", /*!*/ \"false\" );\n"
                               "set ( /*@SET_TYPES*/ \"SET_PLAYER_USABLE\", \"true\" );\n");
    add(tu("Kurz3: talk - die Uebergabe", [=] {
        skriptSetzen(kopfKurz +
                     "rem ( \"talk - started by the intro with run\" );\n"
                     "affect ( \"rosh\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                     "\tset ( /*@SET_TYPES*/ \"SET_LOOK_TARGET\", \"kyle\" );\n}\n\n"
                     "wait ( 2000.000 );\n"
                     "signal ( \"talk_done\" );\n");
        g_app->expanded.openAll();
        rebuildTree();
    }));
    add(hb::zuReiter("intro.txt"));
    add(tu("Kurz3: Intro mit run und waitsignal (nur fuer das Bild)", [=] {
        vorher3->first = g_app->doc.undoDepth();
        skriptSetzen(kopf +
                     "run ( \"taspir_tut/talk\" );\n"
                     "waitsignal ( \"talk_done\" );\n"
                     "camera ( /*@CAMERA_COMMANDS*/ DISABLE );\n");
        g_app->leftMode = 0;
        g_app->expanded.openAll();
        rebuildTree();
    }));
    add(klick(tr(Str::MenuFile), "###main"));
    add(klick(tr(Str::SplitToggle), "##Menu"));
    add(klick("2", "##Menu"));
    add(pause(0.8));
    add(tu("Kurz3: Nebenfeld zeigt talk", [] {
        for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
            if (mb::reiterName(i).find("talk") != std::string::npos) {
                g_app->splitPanes[1].tab = static_cast<int>(i);
                g_app->splitPanes[1].dirty = true;
            }
        }
    }));
    add(pause(0.8));
    add(hb::fotoTeil("k_zusammen", [] {
        ImRect r = hb::fensterRect("##flowarea");
        r.Max.y = r.Min.y + (r.GetHeight() * 0.45F);
        return r;
    }));
    add(klick(tr(Str::MenuFile), "###main"));
    add(klick(tr(Str::SplitToggle), "##Menu"));
    add(klick("1", "##Menu"));
    add(pause(0.4));
    add(tu("Kurz3: Intro zurueck", [=] {
        while (g_app->doc.undoDepth() > vorher3->first && g_app->doc.undo()) {}
        hb::skriptNeu();
    }));
    // 6/5. Der .pk3-Browser und ein Skript des Spiels als Text.
    add(tu("Kurz3: leerer Reiter fuer das Skript aus dem Archiv", [] { neuerReiter(); g_app->leftMode = 0; }));
    add(pause(0.6));
    add(tu("Kurz3: pk3-Browser", [] {
        g_app->pk3Kind = 0;
        std::snprintf(g_app->pk3Filter, sizeof(g_app->pk3Filter), "%s", "taspir1/intro");
        refreshPk3List();
        g_app->pk3Open = true;
    }));
    add(pause(0.8));
    add(hb::fotoTeil("k_pk3", [] { return hb::fensterRect(tr(Str::Pk3Title)); }));
    add(klickAn("Doppelklick auf taspir1/intro.ibi", [] {
        for (const Element& e : g_elemente) {
            if (e.label == "scripts/taspir1/intro.ibi") { return e.rect.GetCenter(); }
        }
        return ImVec2{-1.0F, -1.0F};
    }, 0, true));
    add(pause(1.0));
    add(tu("Kurz3: aufklappen", [] {
        g_app->leftMode = 0;
        g_app->expanded.openAll();
        rebuildTree();
        setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x * 0.5F, ImGui::GetIO().DisplaySize.y - 4.0F});
    }));
    add(pause(0.8));
    add(tu("Kurz3: welcher Reiter", [] {
        diag::detail("Knopftest: Kurz3 - vorn nach dem Doppelklick: " + mb::reiterName(static_cast<std::size_t>(g_app->activeTab)));
    }));
    add(hb::fotoTeil("k_ibi", [] {
        ImRect r = hb::fensterRect("##flowarea");
        r.Max.y = r.Min.y + r.GetHeight() * 0.6F;
        return r;
    }));
    add(hb::zuReiter("intro.txt"));
    add(tu("Kurz3: Karte", [] { g_app->leftMode = 1; g_app->settings.flussAnsicht = 0; }));
    add(pause(0.8));

    // --- 10e. Schrittbilder der Skript-Tutorials (rc600) --------------------------
    // shank: "bei vielen Sachen koennte man viel mehr erklaeren mit mehr Bildern,
    // mach es ausfuehrlicher" - jeder Schritt bekommt ein Bild mit Markierung.
    // Gearbeitet wird in einem Vorfuehrskript im Intro; am Ende wird alles per
    // Rueckgaengig zurueckgenommen.
    {
        using hb::knopf;
        using hb::knopfN;
        using hb::zeile;
        using hb::eintrag;
        using hb::gruppe;
        using M = std::vector<hb::Marke>;
        auto maus = std::make_shared<ImVec2>();
        const auto weg = [] { setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x * 0.5F, ImGui::GetIO().DisplaySize.y - 4.0F}); };
        const auto befehlDb = [](const char* n) -> const Command* {
            for (const Command& c : g_app->db.commands) {
                if (c.name == n) { return &c; }
            }
            return nullptr;
        };
        // Ausschnitte
        const auto oben = [] { return ImGui::GetFrameHeight() * 1.05F; };
        const auto zeilenEnde = [] {
            const ImRect b = hb::fensterRect("##flowarea");
            const float z = ImGui::GetFrameHeight();
            const float rows = static_cast<float>(std::max<std::size_t>(g_app->rows.size(), 8));
            return std::min(b.Max.y, b.Min.y + z * (rows + 3.0F));
        };
        const auto listeBaum = [=] {
            const ImRect b = hb::fensterRect("##flowarea");
            return ImRect{0.0F, oben(), b.Min.x + b.GetWidth() * 0.66F, std::max(zeilenEnde(), b.Min.y + ImGui::GetFrameHeight() * 16.0F)};
        };
        const auto baumAkt = [=] {
            const ImRect b = hb::fensterRect("##flowarea");
            const ImRect comp = knopf(tr(Str::Compile));
            const float unten = comp.GetHeight() > 0.0F ? comp.Max.y + ImGui::GetFontSize() : zeilenEnde();
            return ImRect{b.Min.x, oben(), ImGui::GetIO().DisplaySize.x, std::max(unten, zeilenEnde())};
        };
        const auto nurBaum = [=] {
            const ImRect b = hb::fensterRect("##flowarea");
            return ImRect{b.Min.x, b.Min.y - ImGui::GetFontSize() * 0.3F, b.Min.x + b.GetWidth() * 0.66F, zeilenEnde()};
        };
        const auto editorR = [] {
            ImRect r = hb::fensterRect("###editor");
            r.Expand(ImGui::GetFontSize() * 0.4F);
            return r;
        };
        const auto menuR = [] {
            ImRect r{0.0F, 0.0F, 0.0F, 0.0F};
            for (ImGuiWindow* w : GImGui->Windows) {
                if (w->WasActive && !w->Hidden && std::strstr(w->Name, "##Menu") != nullptr) { r = hb::vereint(r, w->Rect()); }
            }
            r.Expand(ImGui::GetFontSize() * 0.6F);
            r.Min.x = std::max(0.0F, r.Min.x - ImGui::GetFontSize() * 2.0F);
            r.Min.y = 0.0F;
            return r;
        };
        const auto popupR = [=] {
            ImRect r{0.0F, 0.0F, 0.0F, 0.0F};
            for (ImGuiWindow* w : GImGui->Windows) {
                if (w->WasActive && !w->Hidden && (std::strstr(w->Name, "##Popup") != nullptr || std::strstr(w->Name, "##Menu") != nullptr)) {
                    r = hb::vereint(r, w->Rect());
                }
            }
            r = hb::vereint(r, ImRect{(*maus).x - 10.0F, (*maus).y - 10.0F, (*maus).x + 10.0F, (*maus).y + 10.0F});
            r.Expand(ImGui::GetFontSize() * 2.5F);
            return r;
        };
        const auto um = [](std::function<ImRect()> breite, hb::Marken m, float vorZ, float nachZ) {
            return [=] {
                const ImRect b = breite();
                const float z = ImGui::GetFrameHeight();
                float y0 = 1.0e9F;
                float y1 = -1.0e9F;
                for (const hb::Marke& k : m()) {
                    if (k.r.GetWidth() <= 0.0F) { continue; }
                    y0 = std::min(y0, k.r.Min.y);
                    y1 = std::max(y1, k.r.Max.y);
                }
                if (y0 > y1) { return b; }
                ImRect q{b.Min.x, std::max(b.Min.y, y0 - z * vorZ), b.Max.x, std::min(b.Max.y, y1 + z * nachZ)};
                if (q.GetHeight() < z * 11.0F) { q.Max.y = std::min(b.Max.y, q.Min.y + z * 11.0F); }
                return q;
            };
        };
        const auto daneben = [=] { return klickAn("daneben", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }); };
        const auto bildTeil = [&](const std::string& n, std::function<ImRect()> aus, hb::Marken m) { add(hb::fotoTeil(n, std::move(aus), std::move(m))); };
        // Baum- und Aktionsbilder: nur die Gegend um die Rahmen.
        const auto bildBaum = [&](const std::string& n, hb::Marken m) { bildTeil(n, um(nurBaum, m, 4.0F, 4.0F), m); };
        const auto bildAkt = [&](const std::string& n, hb::Marken m) { bildTeil(n, um(baumAkt, m, 9.0F, 3.0F), m); };
        const auto bildListe = [&](const std::string& n, hb::Marken m) { bildTeil(n, um(listeBaum, m, 5.0F, 5.0F), m); };
        // Den Editor oeffnen und warten, bis er wirklich offen ist.
        const auto editor = [&](const std::string& titel, Path weg) {
            add(pause(0.3));
            add({"Skriptbilder: Editor " + titel, [weg](int b) {
                     if (b == 0) {
                         openEditorForNode(weg);
                         return false;
                     }
                     if (b == 3) {
                         std::string l;
                         for (const Element& e : g_elemente) {
                             if (e.fenster.find("###editor") != std::string::npos || e.innen.find("###editor") != std::string::npos) {
                                 l += "[" + e.label + "] ";
                             }
                         }
                         diag::detail("Knopftest: Editor-Elemente: " + l.substr(0, 1500));
                     }
                     return (g_app->editorOpen && b > 4) || b > 40;
                 }});
            add(pause(0.4));
        };
        const auto setzenSchritt = [&](const std::string& titel, std::function<void()> f, double p = 0.6) {
            add(tu("Skriptbilder: " + titel, [=] {
                f();
                g_app->expanded.openAll();
                rebuildTree();
            }));
            add(pause(p));
        };
        const auto auswahl = [](std::vector<Path> wege) {
            g_app->selection = wege;
            g_app->selectedPath = wege.back();
            g_app->auswahlDurchEinfuegen.clear();
            g_app->mapDirty = true;
        };
        const auto rechtsklickZeile = [=](const std::string& befehl, int n) {
            return klickAn("Rechtsklick " + befehl, [=] {
                const ImRect r = zeile(befehl, n);
                const ImVec2 q{r.Min.x + ImGui::GetFontSize() * 2.0F, r.GetCenter().y};
                (*maus) = q;
                return q;
            }, 1);
        };
        const std::string demo = kopf + laeufe + kamera;
        auto tiefeZaehler = std::make_shared<std::size_t>(0);
        add(hb::zuReiter("intro.txt"));
        setzenSchritt("Vorfuehrskript", [=] {
            *tiefeZaehler = g_app->doc.undoDepth();
            skriptSetzen(demo);
            g_app->leftMode = 0;
            g_app->settings.flussAnsicht = 0;
            weg();
        }, 0.8);

        // --- Zeilen ordnen ---
        setzenSchritt("zwei Zeilen gewaehlt", [=] { auswahl({Path{3}, Path{4}}); });
        bildBaum("s_auswahl", [=] { return M{zeile("camera", 1), zeile("camera", 2)}; });
        bildAkt("s_verschieben", [=] { return M{gruppe({tr(Str::ActMoveUp), tr(Str::ActMoveDown)})}; });
        bildAkt("s_clone", [=] { return M{knopf(tr(Str::ActClone))}; });
        bildAkt("s_copy", [=] { return M{knopf(tr(Str::ActCopy)), knopf(tr(Str::ActPaste))}; });
        add(klick(tr(Str::ActRem), "###main"));
        add(pause(0.4));
        add(tu("Skriptbilder: Maus weg", weg));
        add(pause(0.2));
        bildAkt("s_rem", [=] { return M{knopf(tr(Str::ActRem))}; });
        setzenSchritt("REM zurueck", [=] { (void)g_app->doc.undo(); });

        // --- Rueckgaengig ---
        add(klick(tr(Str::MenuEdit), "###main"));
        add(pause(0.4));
        bildTeil("s_undo_menue", menuR, [=] { return M{knopfN(tr(Str::ActUndo), "##Menu", 0, true)}; });
        add(daneben());
        add(tu("Skriptbilder: Rueckgaengig-Liste", [] { g_app->undoListeAnfrage = 1; }));
        add(pause(0.6));
        bildTeil("s_undo_bis", [] { return hb::fensterRect(tr(Str::UndoListTitle)); }, [=] { return M{knopf(tr(Str::UndoUpToHere))}; });
        add(daneben());

        // --- Suchen und ersetzen ---
        add(klick(tr(Str::MenuFile), "###main"));
        add(pause(0.4));
        bildTeil("s_lade_menue", menuR, [=] { return M{knopf(tr(Str::LoadMission), "##Menu")}; });
        add(daneben());
        add(tu("Skriptbilder: Suchen nach kyle", [] {
            g_app->findVorName = "";
            g_app->findVorText = "kyle";
            g_app->findVorbelegen = true;
            g_app->findOpen = true;
        }));
        add(pause(0.6));
        const auto findR = [] {
            ImRect r = hb::fensterRect("###find");
            r.Expand(ImGui::GetFontSize() * 0.4F);
            return r;
        };
        bildTeil("s_find_wert", findR, [=] {
            return M{knopf(tr(Str::FindContains), "###find"), gruppe({tr(Str::FindNextBtn), tr(Str::FindPrevBtn)}, "###find"),
                     knopf(tr(Str::FindWhole), "###find")};
        });
        bildTeil("s_find_name", findR, [=] { return M{knopf(tr(Str::FindNamed), "###find")}; });
        bildTeil("s_find_ersetzen", findR, [=] {
            return M{knopf(tr(Str::FindReplaceWith), "###find"), knopf(tr(Str::FindReplaceAllDocs), "###find")};
        });
        add(tu("Skriptbilder: Suchen zu", [] { g_app->findOpen = false; }));

        // --- Lesezeichen ---
        setzenSchritt("Zeile fuer das Lesezeichen", [=] { auswahl({Path{7}}); weg(); });
        add(klick(tr(Str::MenuEdit), "###main"));
        add(klick(tr(Str::MenuBookmarks), "##Menu"));
        add(pause(0.4));
        bildTeil("s_lese_menue", menuR, [=] { return M{knopf(tr(Str::ActBookmarkToggle), "##Menu")}; });
        bildTeil("s_lese_weiter", menuR, [=] { return M{knopf(tr(Str::ActBookmarkNext), "##Menu"), knopf(tr(Str::ActBookmarkPrev), "##Menu")}; });
        add(klick(tr(Str::ActBookmarkToggle), "##Menu"));
        add(pause(0.4));
        add(tu("Skriptbilder: Maus weg", weg));
        bildBaum("s_lese_gesetzt", [=] { return M{zeile("camera", 3)}; });

        // --- Uebersetzen mit Fehler ---
        setzenSchritt("Skript mit Fehler", [=] {
            skriptSetzen(kopf + "if ( $get( FLOAT, \"seen\")$, $~$, $0$ )\n{\n\twait ( 1000.000 );\n}\n\ncamera ( /*@CAMERA_COMMANDS*/ DISABLE );\n");
            weg();
        });
        auto kompKnopf = std::make_shared<ImRect>();
        add(tu("Skriptbilder: Compile-Knopf merken", [=] { *kompKnopf = knopf(tr(Str::Compile)); }));
        add(klick(tr(Str::Compile), "###main"));
        add(pause(0.25));
        add(tu("Skriptbilder: Maus weg", weg));
        bildTeil("s_comp_fehler", baumAkt, [=] { return M{*kompKnopf, zeile("if", 0)}; });
        add(tu("Skriptbilder: Meldungen", [] { g_app->messagesOpen = true; }));
        add(pause(0.6));
        bildTeil("s_comp_meldung", [] {
            ImRect r = hb::fensterRect("###messages");
            const ImVec2 d = ImGui::GetIO().DisplaySize;
            r = hb::vereint(r, ImRect{0.0F, d.y - ImGui::GetFontSize() * 1.8F, d.x * 0.5F, d.y});
            r.Expand(ImGui::GetFontSize() * 0.4F);
            return r;
        }, [] {
            const ImVec2 d = ImGui::GetIO().DisplaySize;
            return M{ImRect{2.0F, d.y - ImGui::GetFontSize() * 1.6F, ImGui::GetFontSize() * 12.0F, d.y - 2.0F}};
        });
        add(tu("Skriptbilder: Meldungen zu", [] { g_app->messagesOpen = false; }));
        editor("if", Path{5});
        bildTeil("s_comp_editor", editorR, [=] { return M{knopfN("##v", "###editor", 1), knopf(tr(Str::EditorOk), "###editor")}; });
        add(klick(tr(Str::EditorCancel), "###editor"));
        setzenSchritt("zurueck zum Vorfuehrskript", [=] { skriptSetzen(demo); weg(); });

        // --- Warten, bis eine Figur angekommen ist ---
        bildBaum("s_task_affects", [=] { return M{zeile("affect", 0), zeile("affect", 1)}; });
        bildBaum("s_task_task", [=] { return M{zeile("task", 0)}; });
        bildBaum("s_task_dowait", [=] { return M{zeile("dowait", 0)}; });
        setzenSchritt("Skript mit do", [=] {
            skriptSetzen(kopf +
                         "affect ( \"kyle\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                         "\ttask ( \"walk\" )\n\t{\n\t\tset ( /*@SET_TYPES*/ \"SET_NAVGOAL\", \"nav_kyle\" );\n\t}\n\n"
                         "\ttask ( \"look\" )\n\t{\n\t\tset ( /*@SET_TYPES*/ \"SET_LOOK_TARGET\", \"rosh\" );\n\t}\n\n"
                         "\tdo ( \"walk\" );\n\tdo ( \"look\" );\n\twait ( \"walk\" );\n}\n\n");
        });
        bildBaum("s_task_do", [=] { return M{zeile("do", 0), zeile("do", 1), zeile("wait", 0)}; });
        setzenSchritt("Zeitleiste: Kyle aufgeklappt", [=] {
            skriptSetzen(demo);
            g_app->leftMode = 1;
            g_app->tlOffen.insert("kyle");
            g_app->tlKameraOffen = false;
            g_app->playMs = 3000.0;
            blick(-1560.0F, -2280.0F, 900.0F, 23.0F, 90.0F);
        }, 1.0);
        bildTeil("s_task_zeit", karteMitZeitleiste, [=] {
            for (const App::TlBalken& b : g_app->tlBalken) {
                if (b.figur >= 0 && static_cast<std::size_t>(b.figur) < g_app->scene.actors.size() &&
                    g_app->scene.actors[static_cast<std::size_t>(b.figur)].name == "kyle") {
                    return M{ImRect{b.a.x, b.a.y - 4.0F, b.b.x, b.b.y + 4.0F}};
                }
            }
            return M{};
        });
        setzenSchritt("zurueck zu Events", [=] { g_app->leftMode = 0; weg(); });

        // --- Signale: Intro wartet, talk gibt das Signal ---
        auto talkTiefe = std::make_shared<std::size_t>(0);
        add(hb::zuReiter("talk"));
        setzenSchritt("talk mit signal", [=] {
            *talkTiefe = g_app->doc.undoDepth();
            skriptSetzen(kopfKurz + "rem ( \"talk - the other script\" );\nwait ( 2000.000 );\nsignal ( \"door_open\" );\n");
        });
        add(hb::zuReiter("intro.txt"));
        setzenSchritt("Intro mit waitsignal", [=] {
            skriptSetzen(kopf + "waitsignal ( \"door_open\" );\n" + kamera);
            weg();
        });
        add(klick(tr(Str::MenuFile), "###main"));
        add(klick(tr(Str::SplitToggle), "##Menu"));
        add(klick("2", "##Menu"));
        add(pause(0.6));
        add(tu("Skriptbilder: Nebenfeld zeigt talk", [] {
            for (std::size_t i = 0; i < g_app->tabs.size(); ++i) {
                if (mb::reiterName(i).find("talk") != std::string::npos) {
                    g_app->splitPanes[1].tab = static_cast<int>(i);
                    g_app->splitPanes[1].dirty = true;
                }
            }
            g_app->expanded.openAll();
        }));
        add(pause(0.8));
        const auto beideR = [] {
            ImRect r = hb::fensterRect("##flowarea");
            r.Max.y = r.Min.y + r.GetHeight() * 0.42F;
            return r;
        };
        bildTeil("s_sig_wait", beideR, [=] { return M{knopfN("waitsignal", "pane0", 0)}; });
        bildTeil("s_sig_signal", beideR, [=] { return M{knopfN("signal", "pane1", 0)}; });
        bildTeil("s_sig_beide", beideR, [=] { return M{knopfN("waitsignal", "pane0", 0), knopfN("signal", "pane1", 0)}; });
        add(klick(tr(Str::MenuFile), "###main"));
        add(klick(tr(Str::SplitToggle), "##Menu"));
        add(klick("1", "##Menu"));
        add(pause(0.4));
        setzenSchritt("Zeitleiste mit waits", [=] {
            g_app->leftMode = 1;
            g_app->tlWaitsZeigen = true;
            g_app->playMs = 1500.0;
        }, 1.0);
        bildTeil("s_sig_zeit", karteMitZeitleiste, [=] { return M{knopf(tr(Str::TlWaits))}; });
        setzenSchritt("Waits aus, Events", [=] {
            g_app->tlWaitsZeigen = false;
            g_app->leftMode = 0;
            skriptSetzen(demo);
        });

        // --- Makros ---
        add({"Skriptbilder: Befehlsliste nach unten rollen", [](int b) {
                 if (b == 0) {
                     setzeMaus(ImVec2{ImGui::GetFontSize() * 14.0F, ImGui::GetIO().DisplaySize.y * 0.4F});
                     return false;
                 }
                 if (b < 12) { ImGui::GetIO().AddMouseWheelEvent(0.0F, -8.0F); return false; }
                 return b > 20;
             }});
        add(pause(0.3));
        add(tu("Skriptbilder: Maus aus der Liste", weg));
        add(pause(0.3));
        bildTeil("s_makro_liste", listeBaum, [=] { return M{eintrag("standOnly")}; });
        add({"Skriptbilder: Befehlsliste wieder nach oben", [](int b) {
                 if (b == 0) { setzeMaus(ImVec2{ImGui::GetFontSize() * 14.0F, ImGui::GetIO().DisplaySize.y * 0.4F}); return false; }
                 if (b < 12) { ImGui::GetIO().AddMouseWheelEvent(0.0F, 8.0F); return false; }
                 return b > 20;
             }});
        add(tu("Skriptbilder: Maus weg", weg));
        add(pause(0.3));
        setzenSchritt("Skript mit Makro", [=] {
            skriptSetzen(kopfKurz +
                         "//$\"walkOnly\"@5\n"
                         "set ( /*!*/ \"SET_BEHAVIOR_STATE\", /*!*/ \"BS_DEFAULT\" );\n"
                         "set ( /*!*/ \"SET_WALKING\", /*!*/ \"true\" );\n"
                         "set ( /*!*/ \"SET_RUNNING\", /*!*/ \"false\" );\n"
                         "set ( /*!*/ \"SET_IGNOREALERTS\", /*!*/ \"true\" );\n"
                         "set ( /*!*/ \"SET_LOOK_FOR_ENEMIES\", /*!*/ \"false\" );\n");
        });
        bildAkt("s_makro_falten", [=] { return M{knopf(tr(Str::TreeFoldMacros)), zeile("walkOnly", 0)}; });
        setzenSchritt("Vorfuehrskript", [=] { skriptSetzen(demo); weg(); });

        // --- Befehle einfuegen ---
        setzenSchritt("Zeile gewaehlt", [=] { auswahl({Path{3}}); });
        bildListe("s_ein_doppel", [=] { return M{hb::Marke(zeile("camera", 1), 1), hb::Marke(eintrag("wait"), 2)}; });
        setzenSchritt("Befehl in der Liste gewaehlt", [=] {
            for (std::size_t i = 0; i < g_app->db.commands.size(); ++i) {
                if (g_app->db.commands[i].name == "wait") { g_app->selectedCommand = static_cast<int>(i); break; }
            }
        });
        bildListe("s_ein_einfg", [=] { return M{hb::Marke(eintrag("wait"), 1), hb::Marke(zeile("camera", 1), 2)}; });
        bildListe("s_ein_ziehen", [=] { return M{hb::Marke(eintrag("wait"), 1), hb::Marke(zeile("affect", 0), 2)}; });
        add(rechtsklickZeile("camera", 2));
        add(pause(0.4));
        add(klick(tr(Str::GroupEvents), "##Popup"));
        add(pause(0.5));
        bildTeil("s_ein_kontext", popupR, [=] { return M{knopf(tr(Str::GroupEvents), "##Popup")}; });
        add(daneben());
        add(pause(0.3));

        // --- Werte bearbeiten ---
        editor("camera MOVE", Path{3});
        bildTeil("s_wert_felder", editorR, [=] { return M{knopfN("##v", "###editor", 0), knopfN("##v", "###editor", 1)}; });
        bildTeil("s_wert_helper", editorR, [=] { return M{knopfN(tr(Str::EditorHelper), "###editor", 0)}; });
        bildTeil("s_wert_ok", editorR, [=] { return M{knopf(tr(Str::EditorOk), "###editor"), knopf(tr(Str::EditorCancel), "###editor")}; });
        add(klick(tr(Str::EditorCancel), "###editor"));
        editor("set SET_WALKING", Path{5, 0, 0});
        bildTeil("s_wert_liste", editorR, [=] { return M{knopfN("##combo", "###editor", 0), knopfN("##combo", "###editor", 1)}; });
        add(klick(tr(Str::EditorCancel), "###editor"));
        setzenSchritt("Skript mit sound", [=] {
            skriptSetzen(kopfKurz + "sound ( /*@CHANNELS*/ CHAN_VOICE, \"sound/chars/kyle/misc/kyle_ex1.mp3\" );\n");
        });
        editor("sound", Path{1});
        bildTeil("s_wert_datei", editorR, [=] { return M{knopf(tr(Str::EditorBrowse), "###editor"), knopf(tr(Str::EditorPlay), "###editor")}; });
        add(klick(tr(Str::EditorCancel), "###editor"));

        // --- Variablen ---
        const std::string var =
            kopfKurz + "declare ( /*@DECLARE_TYPE*/ FLOAT, \"seen\" );\n"
                       "if ( $get( FLOAT, \"seen\")$, $=$, $0$ )\n{\n"
                       "\trem ( \"the scene - only the first time\" );\n"
                       "\tcamera ( /*@CAMERA_COMMANDS*/ ENABLE );\n"
                       "\twait ( 3000.000 );\n"
                       "\tcamera ( /*@CAMERA_COMMANDS*/ DISABLE );\n"
                       "\tset ( \"seen\", \"1\" );\n}\n\n"
                       "else (  )\n{\n"
                       "\tprint ( \"You were here already.\" );\n}\n\n"
                       "free ( \"seen\" );\n";
        setzenSchritt("Variablen", [=] { skriptSetzen(var); weg(); });
        editor("declare", Path{1});
        bildTeil("s_var_declare", editorR, [=] { return M{knopfN("##combo", "###editor", 0), knopfN("##v", "###editor", 0)}; });
        add(klick(tr(Str::EditorCancel), "###editor"));
        add(pause(0.3));
        bildBaum("s_var_block", [=] { return M{zeile("if", 0), zeile("else", 0)}; });
        bildBaum("s_var_set", [=] { return M{zeile("set", 0)}; });
        bildBaum("s_var_free", [=] { return M{zeile("free", 0)}; });

        // --- Aenderungen sehen, Backup, Meldungen ---
        setzenSchritt("Vorfuehrskript, Rand an", [=] {
            skriptSetzen(demo);
            g_app->settings.changeHistory = true;
            auswahl({Path{7}});
        });
        add(tu("Skriptbilder: Maus auf den Rand", [=] {
            ImVec2 q{0.0F, 0.0F};
            for (std::size_t i = 0; i < g_app->rows.size() && i < g_app->randMitte.size(); ++i) {
                if (g_app->rows[i].path == Path{7}) { q = g_app->randMitte[i]; }
            }
            (*maus) = q;
            setzeMaus(q);
        }));
        add(pause(1.2));
        bildTeil("s_aend_rand", [=] {
            ImRect r = um(nurBaum, [=] { return M{zeile("camera", 3)}; }, 4.0F, 6.0F)();
            r = hb::vereint(r, hb::fensterRect("##Tooltip"));
            return r;
        }, [=] { return M{hb::fensterRect("##Tooltip")}; });
        add(rechtsklickZeile("camera", 3));
        add(pause(0.5));
        bildTeil("s_aend_kontext", popupR, [=] { return M{knopf(tr(Str::ActRevertOriginal), "##Popup")}; });
        add(daneben());
        add(klick(tr(Str::MenuView), "###main"));
        add(pause(0.4));
        bildTeil("s_aend_gleich", menuR, [=] { return M{knopf(tr(Str::ViewHighlightSame), "##Menu")}; });
        add(daneben());
        add(tu("Skriptbilder: Maus weg", weg));
        bildAkt("s_backup", [=] { return M{knopf(tr(Str::FileBackup))}; });
        bildAkt("s_restore", [=] { return M{knopf(tr(Str::FileRestore))}; });
        add(klick(tr(Str::MenuFile), "###main"));
        add(pause(0.4));
        bildTeil("s_ibi_menue", menuR, [=] { return M{knopf(tr(Str::FileOpen), "##Menu")}; });
        add(daneben());
        setzenSchritt("Tippfehler", [=] {
            skriptSetzen(kopf +
                         "affect ( \"kyle\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                         "\tset ( /*@SET_TYPES*/ \"SET_WALKNG\", \"true\" );\n}\n\n"
                         "camera ( /*@CAMERA_COMMANDS*/ DISABLE );\n");
            weg();
        }, 0.8);
        bildTeil("s_meld_zaehler", [] {
            const ImVec2 d = ImGui::GetIO().DisplaySize;
            return ImRect{0.0F, d.y * 0.55F, d.x * 0.55F, d.y};
        }, [] {
            const ImVec2 d = ImGui::GetIO().DisplaySize;
            return M{ImRect{2.0F, d.y - ImGui::GetFontSize() * 1.6F, ImGui::GetFontSize() * 12.0F, d.y - 2.0F}};
        });
        editor("Tippfehler", Path{5, 0});
        bildTeil("s_meld_editor", editorR, [=] { return M{knopfN("##v", "###editor", 0), knopf(tr(Str::EditorOk), "###editor")}; });
        add(klick(tr(Str::EditorCancel), "###editor"));

        // --- .pk3-Browser ---
        add(tu("Skriptbilder: pk3-Browser", [] {
            g_app->pk3Kind = 0;
            std::snprintf(g_app->pk3Filter, sizeof(g_app->pk3Filter), "%s", "taspir1/intro");
            refreshPk3List();
            g_app->pk3Open = true;
        }));
        add(pause(0.8));
        const auto pk3R = [] { return hb::fensterRect(tr(Str::Pk3Title)); };
        bildTeil("s_pk3_art", pk3R, [=] {
            return M{gruppe({tr(Str::Pk3Scripts), tr(Str::Pk3Maps), tr(Str::Pk3Models)})};
        });
        bildTeil("s_pk3_filter", pk3R, [=] { return M{knopf(tr(Str::Pk3Filter))}; });
        bildTeil("s_pk3_eintrag", pk3R, [=] { return M{knopf("scripts/taspir1/intro.ibi")}; });
        add(klick(tr(Str::EditorCancel), tr(Str::Pk3Title)));
        add(pause(0.3));

        // --- Neues Skript in einem leeren Reiter ---
        add(tu("Skriptbilder: Vorfuehrskript zurueck", [=] {
            while (g_app->doc.undoDepth() > *tiefeZaehler && g_app->doc.undo()) {}
            hb::skriptNeu();
        }));
        add(hb::zuReiter("talk"));
        add(tu("Skriptbilder: talk zurueck", [=] {
            while (g_app->doc.undoDepth() > *talkTiefe && g_app->doc.undo()) {}
            hb::skriptNeu();
        }));
        add(tu("Skriptbilder: leerer Reiter", [=] { neuerReiter(); g_app->leftMode = 0; weg(); }));
        add(pause(0.8));
        const auto plusTab = [] {
            ImRect best{};
            for (int k = 0; k < 8; ++k) {
                const ImRect r = hb::knopfN("+", "", k);
                if (r.GetWidth() <= 0.0F) { break; }
                if (r.Min.y < ImGui::GetFrameHeight() * 3.0F && (best.GetWidth() <= 0.0F || r.Min.x > best.Min.x)) { best = r; }
            }
            return best;
        };
        bildTeil("s_neu_plus", listeBaum, [=] { return M{plusTab()}; });
        bildListe("s_neu_affect_liste", [=] { return M{eintrag("affect")}; });
        setzenSchritt("affect eingefuegt", [=] {
            if (const Command* c = befehlDb("affect")) { fuegeBefehlEin(*c); }
        });
        bildListe("s_neu_affect_baum", [=] { return M{zeile("affect", 0)}; });
        editor("affect", Path{0});
        bildTeil("s_neu_editor", editorR, [=] { return M{knopfN("##v", "###editor", 0), knopf(tr(Str::EditorOk), "###editor")}; });
        bildTeil("s_wert_expr", editorR, [=] { return M{knopfN(tr(Str::EditorExpr), "###editor", 0)}; });
        add(klick(tr(Str::EditorCancel), "###editor"));
        setzenSchritt("affect angeklickt", [=] { selectByPath(Path{0}); g_app->auswahlDurchEinfuegen.clear(); weg(); });
        bildListe("s_neu_wait_liste", [=] { return M{hb::Marke(zeile("affect", 0), 1), hb::Marke(eintrag("wait"), 2)}; });
        setzenSchritt("wait eingefuegt", [=] {
            if (const Command* c = befehlDb("wait")) { fuegeBefehlEin(*c); }
            weg();
        });
        bildListe("s_neu_ergebnis", [=] { return M{zeile("wait", 0)}; });
        bildAkt("s_neu_speichern", [=] { return M{knopf(tr(Str::FileSave))}; });
        add(hb::zuReiter("intro.txt"));
        auto tiefeRest = std::make_shared<std::size_t>(0);
        add(tu("Kurz: Stand des Intros merken", [=] { *tiefeRest = g_app->doc.undoDepth(); }));
        // =====================================================================
        // 10f. Schrittbilder fuer Kamera, Zeitleiste, Figuren, Karte, Animation,
        // Modell und Menues (rc600) - im Intro, Mission-Tab.
        // =====================================================================
        const auto mitte = [](ImRect r) { return r.GetCenter(); };
        const auto rolleInspektor = [](float anteil) {
            for (ImGuiWindow* w : GImGui->Windows) {
                if (w != nullptr && std::strstr(w->Name, "entinspektor") != nullptr) { ImGui::SetScrollY(w, w->ScrollMax.y * anteil); }
            }
        };
        const auto vorKlammer = [](const char* t) {
            std::string s = t;
            const std::size_t k = s.find('(');
            if (k != std::string::npos) { s = s.substr(0, k); }
            while (!s.empty() && s.back() == ' ') { s.pop_back(); }
            return s;
        };
        const auto zeitAnzeige = [] {
            ImRect r = knopf(tr(Str::TlUnitSeconds));
            if (r.GetWidth() <= 0.0F) { r = knopf(tr(Str::TlUnitFrames)); }
            return r;
        };
        const auto figurBalken = [](const char* name) {
            for (const App::TlBalken& b : g_app->tlBalken) {
                if (b.figur >= 0 && static_cast<std::size_t>(b.figur) < g_app->scene.actors.size() &&
                    g_app->scene.actors[static_cast<std::size_t>(b.figur)].name == name) {
                    return ImRect{b.a.x, b.a.y - 4.0F, b.b.x, b.b.y + 4.0F};
                }
            }
            return ImRect{};
        };
        const auto zeitR = [] {
            ImRect r = hb::fensterRect("toolband");
            const ImVec2 d = ImGui::GetIO().DisplaySize;
            r.Max.x = std::min(r.Max.x, d.x * 0.72F);
            return r;
        };
        const auto suchen = [&](const std::string& was) {
            add(klick("##entsuche"));
            add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_A, "Strg+A"));
            if (was.empty()) {
                add(taste(ImGuiKey_Backspace, "Rueck"));
                add(daneben());
            } else {
                add(tippe(was));
            }
            add(pause(0.4));
        };
        const auto waehleNamen = [](const char* n) {
            for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
                const MapEntity& e = g_app->map.entities[i];
                const std::string* npc = e.find("NPC_targetname");
                const std::string* st = e.find("script_targetname");
                if (e.targetname == n || (npc != nullptr && *npc == n) || (st != nullptr && *st == n)) {
                    entityAuswahlSetzen(static_cast<int>(i));
                    return static_cast<int>(i);
                }
            }
            return -1;
        };
        const auto rechtsklickAuf = [=](const std::string& was, std::function<ImRect()> ziel) {
            return klickAn("Rechtsklick " + was, [=] {
                const ImVec2 q = ziel().GetCenter();
                *maus = q;
                return q;
            }, 1);
        };
        const std::string kamBefehle = kopf +
            "camera ( /*@CAMERA_COMMANDS*/ ZOOM, 40.000, 2000 );\n"
            "camera ( /*@CAMERA_COMMANDS*/ FADE, < 0.000 0.000 0.000 >, 0.000, < 0.000 0.000 0.000 >, 1.000, 1500 );\n"
            "camera ( /*@CAMERA_COMMANDS*/ SHAKE, 6.000, 1500 );\n"
            "camera ( /*@CAMERA_COMMANDS*/ FOLLOW, \"hero\", 100.000, 1 );\n"
            "camera ( /*@CAMERA_COMMANDS*/ DISTANCE, 200.000, 1 );\n"
            "camera ( /*@CAMERA_COMMANDS*/ DISABLE );\n";

        // --- Editoren der Kamerabefehle ---
        setzenSchritt("Kamerabefehle", [=] { skriptSetzen(kamBefehle); g_app->leftMode = 0; weg(); });
        editor("camera ENABLE", Path{2});
        bildTeil("c_ed_camera", editorR, [=] { return M{knopfN("##combo", "###editor", 0)}; });
        add(klick(tr(Str::EditorCancel), "###editor"));
        editor("ZOOM", Path{5});
        bildTeil("c_ed_zoom", editorR, [=] { return M{hb::Marke(knopfN("##v", "###editor", 0), 1), hb::Marke(knopfN("##v", "###editor", 1), 2)}; });
        add(klick(tr(Str::EditorCancel), "###editor"));
        editor("FADE", Path{6});
        bildTeil("c_ed_fade", editorR, [=] {
            M m;
            for (int i = 0; i < 5; ++i) { m.emplace_back(knopfN("##v", "###editor", i), i + 1); }
            return m;
        });
        add(klick(tr(Str::EditorCancel), "###editor"));
        editor("SHAKE", Path{7});
        bildTeil("c_ed_shake", editorR, [=] { return M{hb::Marke(knopfN("##v", "###editor", 0), 1), hb::Marke(knopfN("##v", "###editor", 1), 2)}; });
        add(klick(tr(Str::EditorCancel), "###editor"));
        editor("FOLLOW", Path{8});
        bildTeil("c_ed_follow", editorR, [=] {
            return M{hb::Marke(knopfN("##v", "###editor", 0), 1), hb::Marke(knopfN("##v", "###editor", 1), 2), hb::Marke(knopfN("##v", "###editor", 2), 3)};
        });
        add(klick(tr(Str::EditorCancel), "###editor"));
        editor("DISTANCE", Path{9});
        bildTeil("c_ed_distance", editorR, [=] { return M{hb::Marke(knopfN("##v", "###editor", 0), 1), hb::Marke(knopfN("##v", "###editor", 1), 2)}; });
        add(klick(tr(Str::EditorCancel), "###editor"));

        // --- Baumbilder fuer Projekt, Figuren und Kamera (Events) ---
        setzenSchritt("Vorfuehrskript fuer Baumbilder", [=] { skriptSetzen(demo); auswahl({Path{6}}); weg(); });
        bildAkt("p3_kopie", [=] { return M{zeile("affect", 1), knopf(tr(Str::ActCopy)), knopf(tr(Str::ActPaste))}; });
        setzenSchritt("Auswahl weg", [=] { g_app->selection.clear(); g_app->selectedPath.clear(); });
        bildBaum("p6_pan", [=] { return M{zeile("camera", 8), zeile("camera", 9)}; });
        bildBaum("p6_schnitt_baum", [=] { return M{zeile("wait", 0), zeile("camera", 5)}; });
        setzenSchritt("Kamera folgt (Baum)", [=] {
            skriptSetzen(kopf +
                         "affect ( \"kyle\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                         "\tset ( /*@SET_TYPES*/ \"SET_CAMERA_GROUP\", \"hero\" );\n"
                         "\tset ( /*@SET_TYPES*/ \"SET_WALKING\", \"true\" );\n"
                         "\tset ( /*@SET_TYPES*/ \"SET_NAVGOAL\", \"nav_kyle\" );\n}\n\n"
                         "camera ( /*@CAMERA_COMMANDS*/ FOLLOW, \"hero\", 100.000, 1 );\n"
                         "camera ( /*@CAMERA_COMMANDS*/ DISTANCE, 200.000, 1 );\n"
                         "wait ( 6000.000 );\ncamera ( /*@CAMERA_COMMANDS*/ DISABLE );\n");
        });
        bildBaum("f_gruppe", [=] { return M{zeile("set", 0)}; });
        setzenSchritt("Video (Baum)", [=] {
            skriptSetzen(kopfKurz + "camera ( /*@CAMERA_COMMANDS*/ ENABLE );\n"
                                    "set ( /*@SET_TYPES*/ \"SET_VIDEO_FADE_IN\", \"true\" );\n"
                                    "set ( /*@SET_TYPES*/ \"SET_VIDEO_FADE_OUT\", \"true\" );\n"
                                    "set ( /*@SET_TYPES*/ \"SET_VIDEO_PLAY\", \"jk0101_sw\" );\n"
                                    "print ( \"Three days later...\" );\n");
        });
        bildBaum("v_fade", [=] { return M{zeile("set", 0), zeile("set", 1)}; });
        setzenSchritt("Kampf (Baum)", [=] {
            skriptSetzen(kopf + "camera ( /*@CAMERA_COMMANDS*/ DISABLE );\n"
                                "affect ( \"rosh\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                                "\tset ( /*@SET_TYPES*/ \"SET_IGNOREENEMIES\", \"false\" );\n"
                                "\tset ( /*@SET_TYPES*/ \"SET_LOOK_FOR_ENEMIES\", \"true\" );\n"
                                "\tset ( /*@SET_TYPES*/ \"SET_ENEMY\", \"player\" );\n}\n\n"
                                "run ( \"taspir_tut/more_enemies\" );\n");
        });
        bildBaum("k_kampf_ende", [=] { return M{zeile("camera", 3)}; });
        bildBaum("k_kampf_an", [=] { return M{zeile("set", 0), zeile("set", 1), zeile("set", 2)}; });
        bildBaum("k_kampf_run", [=] { return M{zeile("run", 0)}; });
        setzenSchritt("Sperren (Baum)", [=] {
            skriptSetzen(kopfKurz + "affect ( \"player\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                                    "\tset ( /*@SET_TYPES*/ \"SET_PLAYER_LOCKED\", \"true\" );\n"
                                    "\tset ( /*@SET_TYPES*/ \"SET_LOCK_PLAYER_WEAPONS\", \"true\" );\n}\n\n"
                                    "wait ( 5000.000 );\n"
                                    "affect ( \"player\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                                    "\tset ( /*@SET_TYPES*/ \"SET_PLAYER_LOCKED\", \"false\" );\n"
                                    "\tset ( /*@SET_TYPES*/ \"SET_LOCK_PLAYER_WEAPONS\", \"false\" );\n}\n\n");
        });
        bildBaum("k_sperren_an", [=] { return M{zeile("set", 0), zeile("set", 1)}; });
        bildBaum("k_sperren_aus", [=] { return M{zeile("set", 2), zeile("set", 3)}; });
        setzenSchritt("Lichtschwert (Baum)", [=] {
            skriptSetzen(kopfKurz + "affect ( \"kyle\", /*@AFFECT_TYPE*/ FLUSH )\n{\n"
                                    "\tset ( /*@SET_TYPES*/ \"SET_WEAPON\", \"WP_SABER\" );\n"
                                    "\twait ( 1000.000 );\n"
                                    "\tset ( /*@SET_TYPES*/ \"SET_SABERACTIVE\", \"true\" );\n"
                                    "\twait ( 3000.000 );\n"
                                    "\tset ( /*@SET_TYPES*/ \"SET_SABERACTIVE\", \"false\" );\n}\n\n");
        });
        bildBaum("l_waffe", [=] { return M{zeile("set", 0)}; });
        bildBaum("l_an", [=] { return M{zeile("set", 1)}; });
        bildBaum("l_aus", [=] { return M{zeile("set", 2)}; });
        setzenSchritt("use-Befehle (Baum)", [=] {
            skriptSetzen(kopfKurz + "use ( \"rosh_spawner\" );\n"
                                    "affect ( \"rosh\", /*@AFFECT_TYPE*/ FLUSH )\n{\n\tset ( /*@SET_TYPES*/ \"SET_LOOK_TARGET\", \"kyle\" );\n}\n\n"
                                    "use ( \"bldg2_ext_door\" );\n"
                                    "set ( /*@SET_TYPES*/ \"SET_MUSIC_STATE\", \"DM_ACTION\" );\n"
                                    "use ( \"fire1\" );\n"
                                    "use ( \"next_map\" );\n");
        });
        bildBaum("e_use_spawner", [=] { return M{zeile("use", 0)}; });
        bildBaum("e_use_tuer", [=] { return M{zeile("use", 1)}; });
        bildBaum("e_musik_baum", [=] { return M{zeile("set", 1)}; });
        bildBaum("e_use_fx", [=] { return M{zeile("use", 2)}; });
        bildBaum("e_use_level", [=] { return M{zeile("use", 3)}; });
        setzenSchritt("Ansprechen (Baum)", [=] {
            skriptSetzen(kopfKurz + "rem ( \"talk - the usescript\" );\n"
                                    "sound ( /*@CHANNELS*/ CHAN_VOICE, \"sound/chars/rosh/misc/rosh_ex1.mp3\" );\n"
                                    "wait ( 2000.000 );\n"
                                    "set ( /*@SET_TYPES*/ \"SET_PLAYER_USABLE\", \"false\" );\n");
        });
        bildBaum("a_einmal", [=] { return M{zeile("set", 0)}; });

        // --- Mission-Tab: das Vorfuehrskript mit Karte und Zeitleiste ---
        setzenSchritt("Mission-Tab mit Vorfuehrskript", [=] {
            skriptSetzen(demo);
            g_app->leftMode = 1;
            g_app->settings.flussAnsicht = 0;
            g_app->timelineFrac = 0.40F;
            g_app->tlKameraOffen = true;
            g_app->tlOffen.clear();
            g_app->showTracks = true;
            entityAuswahlSetzen(-1);
            g_app->cameraSelected = false;
            g_app->playMs = 3000.0;
            blick(-1560.0F, -2280.0F, 900.0F, 23.0F, 90.0F);
            weg();
        }, 1.2);
        add(warteBis("Mission-Tab: Szene", [] { return g_app->camTrackValid; }, 900));
        // Zeitleiste
        bildTeil("c_tl_play", zeitR, [=] { return M{gruppe({tr(Str::TlRewind), tr(Str::TlPlay)}), knopf(tr(Str::TlAudio))}; });
        bildTeil("c_tl_ton", zeitR, [=] { return M{knopf(tr(Str::TlAudio))}; });
        bildTeil("c_tl_folgen", zeitR, [=] { return M{knopf(tr(Str::TlFollow))}; });
        const auto tempoFeld = [=] {
            const ImRect a = knopf(tr(Str::TlLoop));
            const ImRect b = knopf(tr(Str::TlAudio));
            if (a.GetWidth() <= 0.0F || b.GetWidth() <= 0.0F) { return ImRect{}; }
            return ImRect{a.Max.x + 2.0F, a.Min.y, b.Min.x - 2.0F, a.Max.y};
        };
        bildTeil("c_tl_tempo", zeitR, [=] { return M{tempoFeld()}; });
        bildTeil("c_tl_zeit", zeitR, [=] { return M{zeitAnzeige()}; });
        bildTeil("c_tl_ruhig", zeitR, [=] { return M{knopf(tr(Str::TlTracks)), knopf(tr(Str::TlWaits))}; });
        const auto kopfSpalte = [] {
            const ImRect t = hb::fensterRect("tracks");
            if (t.GetWidth() <= 0.0F) { return ImRect{}; }
            return ImRect{t.Min.x + 2.0F, t.Min.y + 2.0F, t.Min.x + ImGui::GetFontSize() * 9.0F, t.Max.y - 2.0F};
        };
        bildTeil("c_zeit_kamera", zeitR, [=] { return M{kopfSpalte()}; });
        bildTeil("c_tl_kante", karteMitZeitleiste, [=] {
            const ImRect b = figurBalken("kyle");
            if (b.GetWidth() <= 0.0F) { return M{}; }
            return M{ImRect{b.Max.x - 8.0F, b.Min.y - 4.0F, b.Max.x + 8.0F, b.Max.y + 4.0F}};
        });
        bildTeil("g_moment", karteMitZeitleiste, [=] { return M{zeitAnzeige()}; });
        // Ein Block anklicken: seine Zeile wird gewaehlt.
        add(klickAn("Laufbalken von Kyle", [=] { return mitte(figurBalken("kyle")); }));
        add(pause(0.5));
        add(tu("Kurz: Maus weg", weg));
        bildTeil("c_tl_klick", voll, [=] { return M{figurBalken("kyle")}; });
        // Kyle aufklappen: Bewegung und Blick
        setzenSchritt("Kyle aufgeklappt", [=] { g_app->tlOffen.insert("kyle"); g_app->tlKameraOffen = false; entityAuswahlSetzen(-1); }, 0.8);
        bildTeil("g_ansehen_zeit", karteMitZeitleiste, [=] { return M{kopfSpalte()}; });
        // Kamera und Einstellungen
        setzenSchritt("Kamera gewaehlt, Einstellung 1", [=] {
            g_app->tlKameraOffen = true;
            g_app->cameraSelected = true;
            kameraSchluesselWaehlen(0, true);
            blick(-1560.0F, -2280.0F, 900.0F, 23.0F, 90.0F);
        }, 1.0);
        bildTeil("c_shots", linksUndKarte, [=] { return M{knopfN("Shot 1", "", 0, true)}; });
        bildTeil("c_aufansicht", linksUndKarte, [=] { return M{knopfN(tr(Str::CamSetToView), "", 0)}; });
        bildTeil("c_hinfliegen", linksUndKarte, [=] { return M{knopf(tr(Str::CamFlyToShot))}; });
        bildTeil("c_pilot_knopf", linksUndKarte, [=] { return M{knopfN(tr(Str::CamPilot), "", 0)}; });
        setzenSchritt("Einstellung 3: der Schnitt", [=] { kameraSchluesselWaehlen(2, true); g_app->timelineFrac = 0.10F; }, 0.8);
        bildTeil("p6_schnitt", linksUndKarte, [=] { return M{knopfN("Shot 3", "", 0, true)}; });
        setzenSchritt("Zeitleiste wieder gross", [=] { g_app->timelineFrac = 0.40F; }, 0.4);
        setzenSchritt("Durch die Kamera", [=] { g_app->throughCamera = true; g_app->playMs = 2000.0; }, 0.8);
        bildTeil("c_durch", kartePlusLeiste, [=] { return M{knopf(tr(Str::MapThroughCam))}; });
        setzenSchritt("Pilot", [=] {
            g_app->throughCamera = false;
            kameraSchluesselWaehlen(1, true);
            g_app->playMs = 4000.0;
            kameraPilotStarten();
        }, 0.8);
        add(tu("Kurz: Maus weg", weg));
        bildTeil("c_pilot_taste", karteMitZeitleiste, [=] { return M{knopf(tr(Str::CamPilotKey))}; });
        bildTeil("c_pilot_fertig", kartePlusLeiste, [=] { return M{knopf(tr(Str::CamPilotApply)), knopf(tr(Str::CamPilotCancel))}; });
        setzenSchritt("Pilot ab", [=] { kameraPilotBeenden(false); g_app->cameraSelected = false; }, 0.6);
        bildTeil("c_frei", kartePlusLeiste, [=] { return M{}; });
        // Blickpunkt merken
        add(tu("Kurz: Maus ueber die Karte", [=] { setzeMaus(mitte(hb::karte())); }));
        add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_1, "Strg+1"));
        add(pause(0.4));
        bildTeil("c_blick_merken", [] {
            const ImVec2 d = ImGui::GetIO().DisplaySize;
            return ImRect{d.x * 0.30F, d.y * 0.62F, d.x, d.y};
        }, [] {
            const ImVec2 d = ImGui::GetIO().DisplaySize;
            return M{ImRect{d.x * 0.60F, d.y - ImGui::GetFontSize() * 1.6F, d.x - 4.0F, d.y - 2.0F}};
        });
        add(tu("Kurz: Maus weg", weg));

        // --- Figuren: Kyle, sein Laufziel, das Menue ---
        setzenSchritt("Kyle mitten im Lauf", [=] {
            g_app->playMs = 2500.0;
            g_app->tlKameraOffen = false;
            const int ei = waehleNamen("kyle");
            for (std::size_t ai = 0; ai < g_app->scene.actors.size(); ++ai) {
                if (entityFuerFigur(static_cast<int>(ai)) == ei) { g_app->selectedActor = static_cast<int>(ai); }
            }
            g_app->gizmoMode = App::GizmoMode::Rotate;
            blick(-1560.0F, -2280.0F, 900.0F, 23.0F, 90.0F);
        }, 1.0);
        bildTeil("g_drehen", kartePlusLeiste, [=] { return M{knopfN(tr(Str::GizmoRotate), "", 0), hb::figurKasten("kyle", 26.0F)}; });
        bildTeil("g_ring_lauf", kartePlusLeiste, [=] { return M{hb::figurKasten("kyle", 34.0F)}; });
        setzenSchritt("Kyle steht (Ende des Laufs)", [=] { g_app->playMs = 7000.0; }, 0.8);
        bildTeil("g_ring", kartePlusLeiste, [=] { return M{hb::figurKasten("kyle", 34.0F)}; });
        setzenSchritt("Laufziel waehlen", [=] { g_app->gizmoMode = App::GizmoMode::Move; g_app->playMs = 2500.0; waehleNamen("nav_kyle"); }, 0.6);
        const auto navKyle = [=] {
            float o[3] = {0, 0, 0};
            const int i = g_app->pickedEntity;
            if (i >= 0) { (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(i)], o); }
            return hb::punkt(o, 12.0F);
        };
        add(rechtsklickAuf("Laufziel", navKyle));
        add(pause(0.5));
        bildTeil("g_ansehen_menue", popupR, [=] { return M{knopf(tr(Str::BahnAnsehen), "##Popup")}; });
        bildTeil("g_ansehen_frei", popupR, [=] { return M{knopf(tr(Str::BahnBlickFrei), "##Popup")}; });
        add(klick(tr(Str::BahnRennen), "##Popup"));
        add(pause(0.6));
        add(tu("Kurz: Maus weg", weg));
        bildTeil("g_gang_zeit", karteMitZeitleiste, [=] { return M{figurBalken("kyle")}; });
        setzenSchritt("Rennen im Baum", [=] { g_app->leftMode = 0; }, 0.6);
        bildBaum("g_gang_baum", [=] { return M{zeile("set", 1)}; });
        setzenSchritt("Rennen zurueck", [=] { (void)g_app->doc.undo(); g_app->leftMode = 1; }, 0.8);

        // --- Karte: Entities ---
        setzenSchritt("Laufziel mit Pfeilen", [=] {
            waehleNamen("nav_kyle");
            g_app->gizmoMode = App::GizmoMode::Move;
            blick(-1560.0F, -2050.0F, 820.0F, 30.0F, 90.0F);
        }, 0.8);
        bildTeil("e_pfeile", kartePlusLeiste, [=] { return M{hb::entityKasten("nav_kyle", 60.0F)}; });
        setzenSchritt("Laufziel mit Ring", [=] { g_app->gizmoMode = App::GizmoMode::Rotate; }, 0.6);
        bildTeil("e_ring", kartePlusLeiste, [=] { return M{hb::entityKasten("nav_kyle", 60.0F)}; });
        setzenSchritt("Verschieben", [=] { g_app->gizmoMode = App::GizmoMode::Move; g_app->timelineFrac = 0.10F; }, 0.6);
        bildTeil("e_dupl", inspektorRect, [=] { return M{knopf(tr(Str::EntDuplicate), "entinspektor")}; });
        bildTeil("e_boden", inspektorRect, [=] { return M{knopf(tr(Str::EntDropFloor), "entinspektor")}; });
        bildTeil("e_klasse", inspektorRect, [=] {
            const ImRect r = hb::fensterRect("entinspektor");
            return M{ImRect{r.Min.x + 4.0F, r.Min.y + 2.0F, r.Max.x - 4.0F, r.Min.y + ImGui::GetFrameHeight() * 1.2F}};
        });
        setzenSchritt("Kyle im Inspektor", [=] { waehleNamen("kyle"); }, 0.6);
        bildTeil("e_npc_namen", inspektorRect, [=] { return M{knopf("##name", "entinspektor"), knopf("...", "entinspektor")}; });
        add(tu("Kurz: Inspektor nach unten", [=] { rolleInspektor(1.0F); }));
        add(pause(0.4));
        bildTeil("e_felder", inspektorRect, [=] { return M{knopfN(tr(Str::EntSecFields), "entinspektor", 0, true), knopf(tr(Str::EntAddKey), "entinspektor")}; });
        add(tu("Kurz: Inspektor nach oben", [=] { rolleInspektor(0.0F); }));
        feldFilter("spawnscript");
        bildTeil("e_kyle_spawn", inspektorRect, [=] { return M{knopf(tr(Str::EntOpenScript), "entinspektor", true)}; });
        feldFilter("");
        setzenSchritt("Rohansicht", [=] { g_app->entRawAnsicht = true; }, 0.6);
        bildTeil("e_roh", inspektorRect, [=] { return M{knopf(tr(Str::EntRaw), "entinspektor")}; });
        setzenSchritt("Rohansicht aus, Rosh", [=] { g_app->entRawAnsicht = false; waehleNamen("rosh"); }, 0.6);
        bildTeil("e_rosh_namen", inspektorRect, [=] { return M{knopf("##name", "entinspektor"), knopf("...", "entinspektor")}; });
        setzenSchritt("Zeitleiste gross", [=] { g_app->timelineFrac = 0.40F; entityAuswahlSetzen(-1); }, 0.6);
        // Kacheln
        bildTeil("e_kachel_npc", linksUndKarte, [=] { return M{knopf(tr(Str::EntKindNpc), "##platzieren")}; });
        bildTeil("e_kachel_tag", linksUndKarte, [=] { return M{knopf(tr(Str::EntKindTag), "##platzieren")}; });
        bildTeil("e_kachel_runner", linksUndKarte, [=] { return M{knopf(tr(Str::EntKindRunner), "##platzieren")}; });
        bildTeil("e_kachel_fx", linksUndKarte, [=] { return M{knopf(tr(Str::EntKindFx), "##platzieren")}; });
        bildTeil("e_kachel_start", linksUndKarte, [=] { return M{knopf(tr(Str::EntKindStart), "##platzieren")}; });
        bildTeil("e_kachel_nav", linksUndKarte, [=] { return M{knopf(tr(Str::EntKindNav), "##platzieren")}; });
        // Leiste
        bildTeil("e_namen_knopf", kartePlusLeiste, [=] { return M{knopf(tr(Str::MapNames))}; });
        bildTeil("e_schalter", kartePlusLeiste, [=] { return M{gruppe({tr(Str::MapEffects), tr(Str::MapGlow), tr(Str::MapDynLights), tr(Str::MapFog), tr(Str::MapSky)})}; });
        bildTeil("e_weg_knopf", kartePlusLeiste, [=] { return M{knopf(tr(Str::WegTitel))}; });
        bildTeil("e_punkt", kartePlusLeiste, [=] {
            const ImRect sv = knopf(tr(Str::MissionSave));
            const ImRect mo = knopf(tr(Str::MapMore));
            if (sv.GetWidth() <= 0.0F || mo.GetWidth() <= 0.0F) { return M{}; }
            return M{ImRect{sv.Max.x + 2.0F, sv.Min.y, mo.Min.x - 2.0F, sv.Max.y}};
        });
        add(tu("Kurz: Namen an", [] { g_app->zeigeNamen = true; g_app->mapDirty = true; }));
        add(pause(0.6));
        bildTeil("e_namen", kartePlusLeiste, [=] { return M{knopf(tr(Str::MapNames))}; });
        add(tu("Kurz: Namen aus", [] { g_app->zeigeNamen = false; }));
        // Ansicht einstellen: Fokus
        add(tu("Kurz: Ansicht einstellen", [] { g_app->ansichtEinstellungenOeffnen = true; }));
        add(pause(0.6));
        bildTeil("e_fokus", [] {
            ImRect r = hb::fensterRect("##Popup");
            r.Expand(ImGui::GetFontSize() * 0.6F);
            return r;
        }, [=] { return M{knopf(tr(Str::MapScriptFocus), "##Popup")}; });
        add(daneben());
        // Suche
        suchen("nav_kyle");
        bildTeil("e_suche", linksUndKarte, [=] { return M{knopf("##entsuche")}; });
        bildTeil("e_zeile", linksUndKarte, [=] { return M{knopfN("nav_kyle", "entlayers", 0)}; });
        suchen("trigger");
        bildTeil("e_suche_trigger", linksUndKarte, [=] { return M{knopf("##entsuche")}; });
        suchen("rosh");
        add(rechtsklickAuf("rosh in der Liste", [=] { return knopfN("rosh", "entlayers", 0); }));
        add(pause(0.5));
        bildTeil("g_ansehen_liste", popupR, [=] { return M{knopf(tr(Str::EntityAsWatch), "##Popup"), knopf(tr(Str::EntityAsLook), "##Popup")}; });
        add(daneben());
        suchen("bldg2_ext");
        add(tu("Kurz: Tuer-Gruppe aufklappen", [] {
            for (int k = 0; k < entityKategorien(); ++k) {
                if (entityKategorieInfo(k).titel == Str::EntityKatMover) { g_app->expandedClass = "kat" + std::to_string(k); }
            }
        }));
        add(pause(0.5));
        bildTeil("e_gruppe_tuer", linksUndKarte, [=] { return M{knopfN(tr(Str::EntityKatMover), "", 0, true)}; });
        suchen("");
        setzenSchritt("Tuer im Inspektor", [=] { g_app->expandedClass.clear(); waehleNamen("bldg2_ext_door"); g_app->timelineFrac = 0.10F; }, 0.6);
        bildTeil("e_tuer_name", inspektorRect, [=] { return M{knopf("##name", "entinspektor")}; });
        bildTeil("e_tuer_anim", inspektorRect, [=] { return M{knopf(tr(Str::EntAnimate), "entinspektor")}; });
        setzenSchritt("Zeitleiste gross", [=] { g_app->timelineFrac = 0.40F; }, 0.4);
        // + Entity
        add(klick(tr(Str::EntNew)));
        add(pause(0.5));
        bildTeil("e_neu_level", popupR, [=] { return M{knopf(tr(Str::EntKindLevel), "##Popup")}; });
        bildTeil("e_neu_musik", popupR, [=] { return M{knopf(tr(Str::EntKindMusic), "##Popup")}; });
        add(daneben());
        // Runner: setzen, Inspektor, count
        auto entTiefe = std::make_shared<std::size_t>(0);
        setzenSchritt("Runner setzen", [=] {
            *entTiefe = g_app->entUndo.size();
            const float o[3] = {-1250.0F, -1900.0F, 660.0F};
            const int i = neueEntityAn("target_scriptrunner", o);
            if (i >= 0) { entityAuswahlSetzen(i); }
            g_app->timelineFrac = 0.10F;
        }, 0.6);
        bildTeil("e_runner_insp", inspektorRect, [=] { return M{knopf(tr(Str::EntOpenScript), "entinspektor", true)}; });
        add(tu("Kurz: Inspektor nach unten", [=] { rolleInspektor(1.0F); }));
        add(pause(0.4));
        bildTeil("e_count", inspektorRect, [=] { return M{knopf(tr(Str::EntAddKey), "entinspektor")}; });
        add(tu("Kurz: Inspektor nach oben", [=] { rolleInspektor(0.0F); }));
        // Gruppe: Kyle, Rosh, beide Laufziele
        setzenSchritt("Gruppe waehlen", [=] {
            g_app->timelineFrac = 0.40F;
            waehleNamen("kyle");
            for (const char* n : {"rosh", "nav_kyle", "nav_rosh"}) {
                for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
                    const MapEntity& e = g_app->map.entities[i];
                    const std::string* npc = e.find("NPC_targetname");
                    if (e.targetname == n || (npc != nullptr && *npc == n)) { entityAuswahlUmschalten(static_cast<int>(i)); }
                }
            }
            g_app->gizmoMode = App::GizmoMode::Move;
            blick(-1610.0F, -2300.0F, 920.0F, 26.0F, 90.0F);
        }, 0.8);
        bildTeil("e_gruppe_pfeile", kartePlusLeiste, [=] { return M{knopfN(tr(Str::GizmoMove), "", 0)}; });
        setzenSchritt("Gruppe drehen", [=] { g_app->gizmoMode = App::GizmoMode::Rotate; }, 0.6);
        bildTeil("e_gruppe_ring", kartePlusLeiste, [=] { return M{knopfN(tr(Str::GizmoRotate), "", 0)}; });
        setzenSchritt("Gruppe: Inspektor", [=] { g_app->gizmoMode = App::GizmoMode::Move; g_app->timelineFrac = 0.10F; }, 0.6);
        bildTeil("e_gruppe_dupl", inspektorRect, [=] { return M{knopf(tr(Str::EntDuplicate), "entinspektor")}; });
        // Draufsicht mit Auswahl und Schnitthoehe
        setzenSchritt("Draufsicht", [=] {
            g_app->timelineFrac = 0.40F;
            waehleNamen("kyle");
            blick(-1600.0F, -1500.0F, 3200.0F, 89.0F, 90.0F);
            g_app->draufSchnitt = 1000.0F;
            g_app->draufsicht = true;
        }, 1.0);
        bildTeil("e_schnitt", kartePlusLeiste, [=] { return M{hb::fensterRect("##draufsicht")}; });
        bildTeil("e_oben_auswahl", kartePlusLeiste, [=] { return M{hb::entityKasten("kyle", 24.0F)}; });
        setzenSchritt("Draufsicht aus", [=] { g_app->draufsicht = false; entityAuswahlSetzen(-1); blick(-1560.0F, -2280.0F, 900.0F, 23.0F, 90.0F); }, 0.6);
        // Wegpunkte
        add(tu("Kurz: Wegpunkte", [] { g_app->wegFensterOffen = true; }));
        add(pause(0.6));
        const auto wegR = [=] { ImRect r = kartePlusLeiste(); r.Add(hb::fensterRect(tr(Str::WegTitel))); return r; };
        bildTeil("e_weg_pruefen", wegR, [=] { return M{knopf(tr(Str::WegPruefen))}; });
        bildTeil("e_weg_setzen", wegR, [=] { return M{knopf(tr(Str::WegSetzen)), knopf(tr(Str::WegVerbinden))}; });
        add(tu("Kurz: Wegpunkte zu", [] { g_app->wegFensterOffen = false; }));
        // Skripte der Karte
        const auto skriptListe = [=] {
            const ImRect m = knopf(tr(Str::MapMore));
            if (m.GetWidth() <= 0.0F) { return ImRect{}; }
            return ImRect{m.Max.x + 4.0F, m.Min.y, m.Max.x + ImGui::GetFontSize() * 7.5F, m.Max.y};
        };
        add(klickAn("Skripte der Karte", [=] { const ImVec2 q = skriptListe().GetCenter(); *maus = q; return q; }));
        add(pause(0.5));
        bildTeil("e_skripte", popupR, [=] { return M{skriptListe()}; });
        add(daneben());
        // Tuer animieren (move): zwei Schluessel
        auto tuerTiefe = std::make_shared<std::size_t>(0);
        setzenSchritt("Tuer animieren", [=] {
            *tuerTiefe = g_app->doc.undoDepth();
            const int t = waehleNamen("bldg2_ext_door");
            if (t >= 0) { animStarten(t); }
            g_app->playMs = 1000.0;
        }, 0.8);
        add(tu("Kurz: Animationsfenster rechts", [=] {
            weg();
            const ImGuiWindow* w = ImGui::FindWindowByName("###animation");
            const float breit = w != nullptr ? w->Size.x : ImGui::GetFontSize() * 30.0F;
            ImGui::SetWindowPos("###animation", ImVec2{ImGui::GetIO().DisplaySize.x - breit - 8.0F, g_app->kartenAnsicht[1] + 8.0F});
        }));
        add(pause(0.6));
        const auto animR = [] { ImRect r = hb::fensterRect("###animation"); r.Expand(ImGui::GetFontSize() * 0.4F); return r; };
        bildTeil("t_modus", animR, [=] { return M{knopf(tr(Str::AnimModeMove), "###animation")}; });
        bildTeil("t_key1", animR, [=] { return M{knopf(tr(Str::AnimAddKey), "###animation")}; });
        add(tu("Kurz: zwei Schluessel fuer die Tuer", [=] {
            const int t = g_app->pickedEntity;
            if (t < 0) { return; }
            float o[3] = {0, 0, 0};
            (void)entityOrt(g_app->map.entities[static_cast<std::size_t>(t)], o);
            std::vector<AnimSchluessel> k(2);
            k[0].ms = 1000.0;
            k[1].ms = 3000.0;
            k[0].ort = {o[0], o[1], o[2]};
            k[1].ort = {o[0], o[1], o[2] + 110.0F};
            (void)animSchreiben(k);
            g_app->playMs = 3000.0;
            zurEntityFliegen(static_cast<std::size_t>(t));
            hb::kameraZurueck(320.0F, 60.0F);
        }));
        add(pause(1.0));
        bildTeil("t_key2", [=] { ImRect r = karteMitZeitleiste(); r.Add(hb::fensterRect("###animation")); return r; }, [=] {
            return M{knopfN("2", "###animation", 0)};
        });
        bildTeil("t_linear", animR, [=] { return M{knopf(tr(Str::AnimLinear), "###animation")}; });
        setzenSchritt("Tuer-Animation zurueck", [=] {
            while (g_app->doc.undoDepth() > *tuerTiefe && g_app->doc.undo()) {}
            g_app->animFensterOffen = false;
            hb::skriptNeu();
            entityAuswahlSetzen(-1);
            blick(-1560.0F, -2280.0F, 900.0F, 23.0F, 90.0F);
        }, 0.6);
        // Mission speichern: die Pakete
        add(tu("Kurz: Mission speichern (Fenster)", [] {
            const char* ziel = std::getenv("BHED_HB_ZIEL");
            g_app->mission.zielPk3 = ziel != nullptr ? std::string(ziel) : mb::verbinde(mb::laufOrdner(), "zz_taspir_tut.pk3");
            g_app->missionSpeichernOffen = true;
        }));
        add(pause(0.8));
        const auto spR = [] { return hb::fensterRect("##missionspeichern"); };
        bildTeil("e_ent_paket", spR, [=] { return M{hb::fensterRect("missionsdateien")}; });
        bildTeil("e_rof_paket", spR, [=] { return M{hb::fensterRect("missionsdateien")}; });
        add(tu("Kurz: Fenster zu", [] { g_app->missionSpeichernOffen = false; }));
        add(tu("Kurz: Entities zurueck", [=] {
            while (g_app->entUndo.size() > *entTiefe && entityRueckgaengig()) {}
            entityAuswahlSetzen(-1);
        }));

        // --- Menues ---
        const auto menue = [&](Str haupt, std::vector<std::pair<std::string, Str>> bilder, Str unter = Str::AppTitle) {
            add(klick(tr(haupt), "###main"));
            if (unter != Str::AppTitle) { add(klick(tr(unter), "##Menu")); }
            add(pause(0.5));
            for (const auto& eintragPaar : bilder) {
                const std::string n = eintragPaar.first;
                const Str st = eintragPaar.second;
                bildTeil(n, menuR, [=] { return M{knopf(tr(st), "##Menu")}; });
            }
            add(daneben());
            add(pause(0.3));
        };
        menue(Str::MenuView, {{"o_theme", Str::ViewTheme}, {"o_sprache", Str::ViewLanguage}, {"o_groesse", Str::ViewDpiScale}});
        menue(Str::MenuView, {{"mp_an", Str::TargetMp}, {"mp_aus", Str::TargetSp}}, Str::ViewTarget);
        menue(Str::MenuDebug, {{"o_layout", Str::DebugReset}});
        menue(Str::MenuHelp, {{"o_hilfe", Str::HelpGuide}});

        // --- Modell ---
        setzenSchritt("Modell mit NPC-Browser", [=] {
            g_app->leftMode = 2;
            (void)npcLaden("Kyle");
            g_app->npcBrowserOffen = true;
        }, 1.5);
        bildTeil("m_npc", voll, [=] { return M{knopf(tr(Str::NpcBrowser))}; });
        add(tu("Kurz: NPC-Browser zu", [] { g_app->npcBrowserOffen = false; }));
        add(pause(0.5));
        const auto modellR = [] {
            const ImVec2 d = ImGui::GetIO().DisplaySize;
            const ImRect band = hb::fensterRect("toolband");
            return ImRect{0.0F, ImGui::GetFrameHeight() * 2.0F, d.x * 0.58F, band.Max.y};
        };
        bildTeil("m_modus", modellR, [=] { return M{knopf(tr(Str::ViewModel))}; });
        bildTeil("m_abspielen", modellR, [=] { return M{knopf(tr(Str::ModelLoop))}; });
        bildTeil("m_insskript", modellR, [=] { return M{knopf(tr(Str::ModelAnimToScript))}; });
        bildTeil("m_details", modellR, [=] { return M{knopf(tr(Str::ModelBones)), knopf(tr(Str::ModelTextures))}; });
        bildTeil("m_caps", modellR, [=] { return M{knopf(tr(Str::ModelCaps))}; });
        bildTeil("m_gla", modellR, [=] { return M{knopf(tr(Str::ModelLoadGla))}; });
        bildTeil("m_flaechen", modellR, [=] {
            const ImRect sk = knopf(tr(Str::ModelSkin));
            if (sk.GetWidth() <= 0.0F) { return M{}; }
            return M{ImRect{sk.Max.x + 2.0F, sk.Min.y, sk.Max.x + ImGui::GetFontSize() * 9.0F, sk.Max.y}};
        });
        setzenSchritt("zurueck zur Mission", [=] { g_app->leftMode = 1; }, 0.6);
        add(tu("Kurz: Intro zurueck", [=] {
            while (g_app->doc.undoDepth() > *tiefeRest && g_app->doc.undo()) {}
            hb::skriptNeu();
            g_app->tlOffen.clear();
            g_app->tlKameraOffen = true;
            entityAuswahlSetzen(-1);
        }));
        // =====================================================================
        // 10g. Eine Seite je Befehl (rc601) - shank: "jedes Event einmal
        // durchgehen und Bilder machen, was jedes Event-Element kann und wie
        // es in den 3 Modi aussieht" und "auch mit Bildern, was jedes Command
        // kann und wo man was eintraegt". Je Befehl ein kleines Skript im
        // Intro, dann vier Bilder: die Zeile in Classic, der Editor mit
        // nummerierten Feldern, die Liste im Graph, der Block in den Spuren.
        // =====================================================================
        {
            struct Bef {
                const char* key;
                const char* befehl;
                int nte;
                std::string text;
            };
            const std::string an = "camera ( /*@CAMERA_COMMANDS*/ ENABLE );\n"
                                   "camera ( /*@CAMERA_COMMANDS*/ MOVE, < -1640.000 -2180.000 720.000 >, 0 );\n"
                                   "camera ( /*@CAMERA_COMMANDS*/ PAN, < -4.000 88.000 0.000 >, < 0.000 0.000 0.000 >, 0 );\n";
            const std::string aus = "wait ( 2000.000 );\ncamera ( /*@CAMERA_COMMANDS*/ DISABLE );\n";
            const auto fig = [](const char* wer, const std::string& rumpf) {
                return std::string("affect ( \"") + wer + "\", /*@AFFECT_TYPE*/ FLUSH )\n{\n" + rumpf + "}\n\n";
            };
            const std::string gehen = "\tset ( /*@SET_TYPES*/ \"SET_WALKING\", \"true\" );\n"
                                      "\tset ( /*@SET_TYPES*/ \"SET_NAVGOAL\", \"nav_kyle\" );\n";
            const std::string aufgabe = "\ttask ( \"walk\" )\n\t{\n\t\tset ( /*@SET_TYPES*/ \"SET_WALKING\", \"true\" );\n"
                                        "\t\tset ( /*@SET_TYPES*/ \"SET_NAVGOAL\", \"nav_kyle\" );\n\t}\n\n";
            const std::string variable = "declare ( /*@DECLARE_TYPE*/ FLOAT, \"seen\" );\n"
                                         "if ( $get( FLOAT, \"seen\")$, $=$, $0$ )\n{\n"
                                         "\tprint ( \"First time here\" );\n\tset ( \"seen\", \"1\" );\n}\n\n"
                                         "else (  )\n{\n\tprint ( \"Welcome back\" );\n}\n\n";
            const std::string k = "camera ( /*@CAMERA_COMMANDS*/ ";
            const std::vector<Bef> befehle = {
                {"flush", "flush", 0, fig("kyle", "\tflush (  );\n" + gehen) + aus},
                {"if", "if", 0, variable},
                {"else", "else", 0, variable},
                {"loop", "loop", 0, an + "loop ( 3 )\n{\n\t" + k + "SHAKE, 2.000, 500 );\n\twait ( 1000.000 );\n}\n\n" + aus},
                {"affect", "affect", 0, an + fig("kyle", gehen) + aus},
                {"run", "run", 0, an + "run ( \"taspir_tut/kyle_spawn\" );\n" + aus},
                {"wait", "wait", 0, an + "wait ( 2000.000 );\n" + k + "DISABLE );\n"},
                {"wait_task", "wait", 0, fig("kyle", aufgabe + "\tdo ( \"walk\" );\n\twait ( \"walk\" );\n\tset ( /*@SET_TYPES*/ \"SET_LOOK_TARGET\", \"rosh\" );\n")},
                {"waitsignal", "waitsignal", 0, an + "waitsignal ( \"door_open\" );\n" + aus},
                {"signal", "signal", 0, "use ( \"bldg2_ext_door\" );\nwait ( 1500.000 );\nsignal ( \"door_open\" );\n"},
                {"sound", "sound", 0, an + fig("kyle", "\tsound ( /*@CHANNELS*/ CHAN_VOICE, \"sound/chars/kyle/01kyk001.mp3\" );\n") + aus},
                {"move", "move", 0, fig("bldg2_ext_door", "\tmove ( < 0.000 0.000 128.000 >, < 0.000 0.000 0.000 >, 2000.000 );\n") + "wait ( 2000.000 );\n"},
                {"rotate", "rotate", 0, fig("bldg2_ext_door", "\trotate ( < 0.000 90.000 0.000 >, 1000.000 );\n") + "wait ( 1000.000 );\n"},
                {"use", "use", 0, an + "use ( \"bldg2_ext_door\" );\n" + aus},
                {"kill", "kill", 0, an + "kill ( \"rosh\" );\n" + aus},
                {"remove", "remove", 0, an + "remove ( \"rosh\" );\n" + aus},
                {"print", "print", 0, an + "print ( \"Three days later...\" );\n" + aus},
                {"rem", "rem", 0, "rem ( \"The ship takes off here\" );\n" + an + aus},
                {"declare", "declare", 0, variable},
                {"free", "free", 0, variable + "free ( \"seen\" );\n"},
                {"set", "set", 0, an + fig("kyle", gehen) + aus},
                {"set_var", "set", 0, variable},
                {"task", "task", 0, fig("kyle", aufgabe + "\tdowait ( \"walk\" );\n")},
                {"do", "do", 0, fig("kyle", aufgabe + "\ttask ( \"look\" )\n\t{\n\t\tset ( /*@SET_TYPES*/ \"SET_LOOK_TARGET\", \"rosh\" );\n\t}\n\n"
                                            "\tdo ( \"walk\" );\n\tdo ( \"look\" );\n\twait ( \"walk\" );\n")},
                {"dowait", "dowait", 0, fig("kyle", aufgabe + "\tdowait ( \"walk\" );\n\tset ( /*@SET_TYPES*/ \"SET_LOOK_TARGET\", \"rosh\" );\n")},
                {"play", "play", 0, fig("ravensClaw", "\tplay ( /*@PLAY_TYPES*/ \"PLAY_ROFF\", \"taspir_tut/roffs/ravensClaw\" );\n") + "wait ( 5000.000 );\n"},
                {"cam_enable", "camera", 0, an + aus},
                {"cam_disable", "camera", 3, an + "wait ( 2000.000 );\n" + k + "DISABLE );\n"},
                {"cam_move", "camera", 1, an + k + "MOVE, < -750.000 -2700.000 1000.000 >, 3000 );\n" + aus},
                {"cam_pan", "camera", 2, an + k + "PAN, < 4.000 120.000 0.000 >, < 0.000 0.000 0.000 >, 3000 );\n" + aus},
                {"cam_zoom", "camera", 3, an + k + "ZOOM, 40.000, 2000 );\n" + aus},
                {"cam_roll", "camera", 3, an + k + "ROLL, 15.000, 1000 );\n" + aus},
                {"cam_track", "camera", 3, an + k + "TRACK, \"cam_track\", 100.000, 1 );\n" + aus},
                {"cam_follow", "camera", 3, an + k + "FOLLOW, \"kyle\", 50.000, 1 );\n" + aus},
                {"cam_distance", "camera", 3, an + k + "DISTANCE, 200.000, 1 );\n" + aus},
                {"cam_fade", "camera", 3, an + k + "FADE, < 0.000 0.000 0.000 >, 0.000, < 0.000 0.000 0.000 >, 1.000, 1500 );\n" + aus},
                {"cam_shake", "camera", 3, an + k + "SHAKE, 4.000, 2000 );\n" + aus},
                {"cam_path", "camera", 3, an + k + "PATH, \"taspir_tut/roffs/campath\" );\n" + aus},
            };
            // Der Weg der n-ten Zeile mit diesem Befehl (Tiefensuche wie der Baum).
            const auto wegVon = [](const std::string& name, int nte) {
                Path gefunden;
                int zaehler = 0;
                Path weg;
                const std::function<bool(const std::vector<Node>&)> suche = [&](const std::vector<Node>& ns) {
                    for (std::size_t i = 0; i < ns.size(); ++i) {
                        weg.push_back(i);
                        if (ns[i].kind == Node::Kind::Command && ns[i].name == name && zaehler++ == nte) {
                            gefunden = weg;
                            return true;
                        }
                        if (suche(ns[i].children)) { return true; }
                        weg.pop_back();
                    }
                    return false;
                };
                (void)suche(g_app->doc.script().nodes);
                return gefunden;
            };
            const auto flussR = [] {
                ImRect r = hb::fensterRect("##flussansicht");
                r.Min.y -= ImGui::GetFrameHeight() * 1.3F;   // mit dem Umschalter Classic / Graph / Tracks
                r.Expand(ImGui::GetFontSize() * 0.3F);
                return r;
            };
            const auto listenZeile = [](const std::string& befehl, int nte) {
                return knopfN(befehl + " (", "befehlsliste", nte, true);
            };
            // Die Felder des Editors, in der Reihenfolge der Werte, nummeriert.
            const auto felder = [] {
                M m;
                const Node* n = nodeAt(g_app->doc.script(), g_app->selectedPath);
                const int anzahl = n != nullptr ? static_cast<int>(n->args.size()) : 0;
                // Textfelder heissen "##v", Klapplisten "##combo" - in der
                // Reihenfolge, in der sie gezeichnet werden, sind die ersten
                // `anzahl` davon die Werte (danach kommen Helfer-Zeilen).
                int nr = 0;
                for (const Element& e : g_elemente) {
                    if (nr >= anzahl) { break; }
                    if (e.fenster.find("###editor") == std::string::npos && e.innen.find("###editor") == std::string::npos) { continue; }
                    if (e.label != "##v" && e.label != "##combo") { continue; }
                    m.push_back(hb::Marke{e.rect, ++nr});
                }
                return m;
            };
            auto tiefeBef = std::make_shared<std::size_t>(0);
            setzenSchritt("Befehlsseiten: Anfang", [=] {
                *tiefeBef = g_app->doc.undoDepth();
                g_app->leftMode = 0;
                g_app->settings.flussAnsicht = 0;
                g_app->spurenListe = true;
                weg();
            }, 0.4);
            for (const Bef& b : befehle) {
                const std::string key = b.key;
                const std::string befehl = b.befehl;
                const int nte = b.nte;
                const std::string text = "//Generated by BehavEd\n\n" + b.text;
                setzenSchritt("Befehl " + key, [=] {
                    skriptSetzen(text);
                    g_app->settings.flussAnsicht = 0;
                    g_app->graph().auswahl.clear();
                    const Path w = wegVon(befehl, nte);
                    if (!w.empty()) { auswahl({w}); }
                    g_app->playMs = 0.0;
                    weg();
                }, 0.7);
                bildBaum("b_" + key + "_c", [=] { return M{zeile(befehl, nte)}; });
                add(tu("Befehl " + key + ": Editor", [=] {
                    const Path w = wegVon(befehl, nte);
                    if (!w.empty()) { auswahl({w}); openEditorForNode(w); }
                }));
                add(warteBis("Befehl " + key + ": Editor offen", [] { return g_app->editorOpen; }, 120));
                add(pause(0.5));
                bildTeil("b_" + key + "_ed", editorR, felder);
                add(tu("Befehl " + key + ": Editor zu, Graph", [] {
                    g_app->editorOpen = false;
                    g_app->settings.flussAnsicht = 1;
                    g_app->graph().auswahl.clear();
                }));
                add(pause(0.8));
                bildTeil("b_" + key + "_g", flussR, [=] { return M{listenZeile(befehl, nte)}; });
                add(tu("Befehl " + key + ": Spuren", [] { g_app->settings.flussAnsicht = 2; }));
                add(pause(0.8));
                bildTeil("b_" + key + "_t", flussR, [=] {
                    const Path w = wegVon(befehl, nte);
                    for (const auto& [q, r] : g_app->spurenBloecke) {
                        if (q == w) {
                            diag::detail("Knopftest: Befehlsseite " + key + ": Block in den Spuren");
                            return M{r};
                        }
                    }
                    diag::detail("Knopftest: Befehlsseite " + key + ": nur in der Liste");
                    return M{listenZeile(befehl, nte)};
                });
                add(tu("Befehl " + key + ": zurueck zu Classic", [] { g_app->settings.flussAnsicht = 0; }));
            }
            setzenSchritt("Befehlsseiten: Ende", [=] {
                while (g_app->doc.undoDepth() > *tiefeBef && g_app->doc.undo()) {}
                g_app->settings.flussAnsicht = 0;
            }, 0.5);
        }
        // =====================================================================
        // 10h. Die Skript-Tutorials in Graph und Tracks (rc601) - shank: "dass
        // jedes Tutorial in jedem Modus erstellt werden kann". Bilder vg_*
        // (Graph) und vt_* (Tracks); Texte in tools/handbuch/inhalt_modi.py.
        // =====================================================================
        {
            const auto ansicht = [](int m) { g_app->settings.flussAnsicht = m; };
            const auto flussR = [] {
                ImRect r = hb::fensterRect("##flussansicht");
                r.Min.y -= ImGui::GetFrameHeight() * 1.3F;
                r.Expand(ImGui::GetFontSize() * 0.3F);
                return r;
            };
            // Ereignisliste und Skriptfeld zusammen - fuer "von links ziehen".
            const auto mitListe = [=] {
                const ImRect f = hb::fensterRect("##flussansicht");
                return ImRect{0.0F, oben(), f.Max.x + 6.0F, f.Max.y + 6.0F};
            };
            const auto listenZeile = [](const std::string& befehl, int nte) { return knopfN(befehl + " (", "befehlsliste", nte, true); };
            const auto blockR = [](const Path& p) {
                for (const auto& [q, r] : g_app->spurenBloecke) {
                    if (q == p) { return r; }
                }
                return ImRect{};
            };
            const auto wegVon = [](const std::string& name, int nte) {
                Path gefunden;
                int zaehler = 0;
                Path weg;
                const std::function<bool(const std::vector<Node>&)> suche = [&](const std::vector<Node>& ns) {
                    for (std::size_t i = 0; i < ns.size(); ++i) {
                        weg.push_back(i);
                        if (ns[i].kind == Node::Kind::Command && ns[i].name == name && zaehler++ == nte) {
                            gefunden = weg;
                            return true;
                        }
                        if (suche(ns[i].children)) { return true; }
                        weg.pop_back();
                    }
                    return false;
                };
                (void)suche(g_app->doc.script().nodes);
                return gefunden;
            };
            const auto kleinS = [](std::string t) {
                for (char& c : t) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
                return t;
            };
            // Der Kasten eines Knotens im Graphen (Schluessel enthaelt `teil`).
            const auto knotenR = [](const std::string& teil) {
                App::GraphZustand& z = g_app->graph();
                const float sk = ImGui::GetFontSize() / 13.0F;
                for (const GraphKnoten& kn : z.g.knoten) {
                    if (kn.schluessel.find(teil) == std::string::npos) { continue; }
                    const ImVec2 a{z.ursprung.x + z.panX + kn.x * z.zoom, z.ursprung.y + z.panY + kn.y * z.zoom};
                    return ImRect{a, ImVec2{a.x + 190.0F * sk * z.zoom, a.y + 62.0F * sk * z.zoom}};
                }
                return ImRect{};
            };
            const auto aktivKnoten = [=] { return knotenR("s:" + kleinS(aktiverSkriptRef())); };
            // Den Knoten in die Mitte der Flaeche schieben - auf taspir1 ist der
            // Graph gross, und das Intro lag ausserhalb des Bildes.
            const auto zentriere = [=](std::function<std::string()> schluessel) {
                return tu("Modi: Knoten in die Mitte", [=] {
                    App::GraphZustand& z = g_app->graph();
                    const std::string k = schluessel();
                    const float sk = ImGui::GetFontSize() / 13.0F;
                    for (const GraphKnoten& kn : z.g.knoten) {
                        if (kn.schluessel != k) { continue; }
                        z.zoom = 1.0F;
                        z.panX = z.groesse.x * 0.40F - (kn.x + 95.0F * sk);
                        z.panY = z.groesse.y * 0.40F - (kn.y + 31.0F * sk);
                    }
                });
            };
            const auto aktivSchluessel = [=] { return "s:" + kleinS(aktiverSkriptRef()); };
            // Eine freie Stelle im Graphen: unten rechts in der Flaeche.
            const auto freiImGraph = [] {
                const App::GraphZustand& z = g_app->graph();
                const ImVec2 m{z.ursprung.x + z.groesse.x * 0.62F, z.ursprung.y + z.groesse.y * 0.78F};
                return ImRect{m.x - 40.0F, m.y - 22.0F, m.x + 40.0F, m.y + 22.0F};
            };
            const auto argSetzen = [](const Path& w, std::vector<std::string> werte) {
                const Node* n = nodeAt(g_app->doc.script(), w);
                if (n == nullptr) { return; }
                Node neuN = *n;
                for (std::size_t i = 0; i < werte.size() && i < neuN.args.size(); ++i) { neuN.args[i].text = werte[i]; }
                (void)g_app->doc.replaceAt(w, neuN);
            };
            const auto befehlRein = [=](const char* befehl) {
                if (const Command* c = befehlDb(befehl)) { fuegeBefehlEin(*c); }
            };
            // Rechtsklick auf eine Stelle, danach ueber "Events" fahren (Untermenue).
            const auto rechtsklick = [=](const std::string& was, std::function<ImRect()> wo) {
                return klickAn("Rechtsklick " + was, [=] {
                    const ImRect r = wo();
                    const ImVec2 q{r.Min.x + std::min(30.0F, r.GetWidth() * 0.5F), r.GetCenter().y};
                    (*maus) = q;
                    return q;
                }, 1);
            };
            const auto zumUntermenue = [=] {
                return tu("Modi: ueber Events fahren", [] {
                    const ImRect r = knopf(tr(Str::GroupEvents), "##Popup");
                    if (r.GetWidth() > 0.0F) { setzeMaus(r.GetCenter()); }
                });
            };
            // Ein echter Zug aus der Ereignisliste.
            const auto ziehAus = [](const std::string& befehl, std::function<ImRect()> ziel) {
                auto start = std::make_shared<ImVec2>();
                return Schritt{"Modi: ziehe " + befehl, [=](int b) {
                                   ImGuiIO& io = ImGui::GetIO();
                                   if (b == 0) {
                                       const ImRect e = hb::eintrag(befehl);
                                       *start = ImVec2{e.Min.x + 20.0F, e.GetCenter().y};
                                       setzeMaus(*start);
                                       return false;
                                   }
                                   if (b == 1) { io.AddMouseButtonEvent(0, true); return false; }
                                   if (b >= 2 && b <= 6) {
                                       setzeMaus(ImVec2{start->x + 5.0F * static_cast<float>(b - 1), start->y + 2.0F * static_cast<float>(b - 1)});
                                       return false;
                                   }
                                   if (b >= 7 && b <= 14) {
                                       const ImVec2 z = ziel().GetCenter();
                                       const float f = std::min(1.0F, static_cast<float>(b - 6) / 5.0F);
                                       const ImVec2 a{start->x + 25.0F, start->y + 10.0F};
                                       setzeMaus(ImVec2{a.x + (z.x - a.x) * f, a.y + (z.y - a.y) * f});
                                       return false;
                                   }
                                   if (b == 15) { io.AddMouseButtonEvent(0, false); return false; }
                                   return b >= 20;
                               }};
            };
            const auto menueMarken = [=] {
                return M{hb::Marke{knopf(tr(Str::ActEdit), "##Popup"), 1}, hb::Marke{knopf(tr(Str::ActClone), "##Popup"), 2},
                         hb::Marke{gruppe({tr(Str::ActCopy), tr(Str::ActCut), tr(Str::ActPaste)}, "##Popup"), 3},
                         hb::Marke{gruppe({tr(Str::MoveUp), tr(Str::MoveDown)}, "##Popup"), 4},
                         hb::Marke{knopf(tr(Str::ActRem), "##Popup"), 5}, hb::Marke{knopf(tr(Str::ActDelete), "##Popup"), 6}};
            };
            const std::string variable = "//Generated by BehavEd\n\n"
                                         "declare ( /*@DECLARE_TYPE*/ FLOAT, \"seen\" );\n"
                                         "if ( $get( FLOAT, \"seen\")$, $=$, $0$ )\n{\n"
                                         "\tprint ( \"First time here\" );\n\tset ( \"seen\", \"1\" );\n}\n\n"
                                         "else (  )\n{\n\tprint ( \"Welcome back\" );\n}\n\n";
            auto tiefeModi = std::make_shared<std::size_t>(0);
            add(hb::zuReiter("intro.txt"));
            setzenSchritt("Modi: Anfang", [=] {
                *tiefeModi = g_app->doc.undoDepth();
                skriptSetzen(demo);
                g_app->leftMode = 0;
                ansicht(1);
                g_app->graph().auswahl.clear();
                g_app->spurenListe = true;
                auswahl({wegVon("wait", 0)});
                weg();
            }, 0.9);

            // --- Graph ---
            add(zentriere(aktivSchluessel));
            add(pause(0.4));
            bildTeil("vg_umschalten", flussR, [=] { return M{knopf(tr(Str::FlowGraph))}; });
            bildTeil("vg_liste", flussR, [=] { return M{hb::Marke{aktivKnoten(), 1}, hb::Marke{hb::fensterRect("##befehlsliste"), 2}}; });
            bildTeil("vg_ein_doppel", mitListe, [=] { return M{hb::Marke{listenZeile("wait", 0), 1}, hb::Marke{hb::eintrag("print"), 2}}; });
            bildTeil("vg_ein_ziehen", mitListe, [=] { return M{hb::Marke{hb::eintrag("sound"), 1}, hb::Marke{listenZeile("camera", 1), 2}}; });
            bildTeil("vg_ein_knoten", mitListe, [=] { return M{hb::Marke{hb::eintrag("print"), 1}, hb::Marke{aktivKnoten(), 2}}; });
            bildTeil("vg_makro", mitListe, [=] { return M{hb::Marke{hb::eintrag("standOnly"), 1}, hb::Marke{listenZeile("camera", 1), 2}}; });
            bildTeil("vg_wert_doppel", flussR, [=] { return M{listenZeile("camera", 1)}; });
            bildTeil("vg_task_liste", flussR, [=] { return M{listenZeile("task", 0)}; });
            bildTeil("vg_task_dowait", flussR, [=] { return M{listenZeile("dowait", 0)}; });
            setzenSchritt("Modi: Graph - Zeile waehlen", [=] { auswahl({wegVon("camera", 1)}); weg(); }, 0.4);
            bildTeil("vg_ord_wahl", flussR, [=] { return M{listenZeile("camera", 1)}; });
            add(rechtsklick("Graph-Zeile", [=] { return listenZeile("camera", 1); }));
            add(pause(0.5));
            bildTeil("vg_ord_menue", popupR, menueMarken);
            add(zumUntermenue());
            add(pause(0.6));
            bildTeil("vg_ein_kontext", popupR, [=] { return M{knopf(tr(Str::GroupEvents), "##Popup")}; });
            add(daneben());
            add(pause(0.3));
            // Das Intro wartet auf ein Signal; ein anderes Skript der Mission.
            setzenSchritt("Modi: Graph - waitsignal im Intro", [=] {
                skriptSetzen(kopf + "waitsignal ( \"door_open\" );\n" + kamera);
                g_app->graph().auswahl.clear();
                weg();
            }, 0.8);
            add(zentriere(aktivSchluessel));
            add(pause(0.4));
            bildTeil("vg_sig_intro", flussR, [=] { return M{hb::Marke{aktivKnoten(), 1}, hb::Marke{listenZeile("waitsignal", 0), 2}}; });
            add(tu("Modi: Graph - ein anderes Skript waehlen", [=] {
                App::GraphZustand& z = g_app->graph();
                const std::string aktiv = "s:" + kleinS(aktiverSkriptRef());
                for (const GraphKnoten& kn : z.g.knoten) {
                    if (!kn.skript.empty() && kn.schluessel != aktiv) { z.auswahl = kn.schluessel; break; }
                }
            }));
            add(zentriere([] { return g_app->graph().auswahl; }));
            add(pause(0.6));
            bildTeil("vg_sig_andere", flussR, [=] {
                return M{hb::Marke{knotenR(g_app->graph().auswahl.empty() ? std::string("~") : g_app->graph().auswahl), 1},
                         hb::Marke{knopf(tr(Str::GraphEditHere)), 2}};
            });
            setzenSchritt("Modi: Graph - Variablen", [=] { skriptSetzen(variable); g_app->graph().auswahl.clear(); weg(); }, 0.8);
            bildTeil("vg_var_liste", flussR, [=] { return M{listenZeile("declare", 0), listenZeile("if", 0), listenZeile("else", 0)}; });
            // Ein neues Skript: affect auf freie Flaeche ziehen (echter Zug).
            setzenSchritt("Modi: Graph - zurueck zum Vorfuehrskript", [=] { skriptSetzen(demo); g_app->graph().auswahl.clear(); weg(); }, 0.8);
            add(zentriere(aktivSchluessel));
            add(pause(0.4));
            bildTeil("vg_neu_ziehen", mitListe, [=] { return M{hb::Marke{hb::eintrag("affect"), 1}, hb::Marke{freiImGraph(), 2}}; });
            add(ziehAus("affect", freiImGraph));
            add(pause(0.8));
            add(tu("Modi: Graph - neues Skript, Maus weg", [=] { weg(); }));
            add(pause(0.4));
            bildTeil("vg_neu_knoten", flussR, [=] { return M{hb::Marke{aktivKnoten(), 1}, hb::Marke{listenZeile("affect", 0), 2}}; });
            setzenSchritt("Modi: Graph - affect kyle, waehlen", [=] {
                const Path w = wegVon("affect", 0);
                argSetzen(w, {"kyle"});
                auswahl({w});
                weg();
            }, 0.5);
            bildTeil("vg_neu_wait", mitListe, [=] { return M{hb::Marke{listenZeile("affect", 0), 1}, hb::Marke{hb::eintrag("wait"), 2}}; });
            setzenSchritt("Modi: Graph - wait in den Block", [=] { befehlRein("wait"); weg(); }, 0.6);
            bildTeil("vg_neu_ergebnis", flussR, [=] { return M{listenZeile("wait", 0)}; });
            add(tu("Modi: Graph - neues Skript verwerfen", [] {
                if (mb::reiterName(static_cast<std::size_t>(g_app->activeTab)) != "intro.txt") {
                    g_app->doc.markSaved();
                    reiterSchliessen(g_app->activeTab);
                }
            }));
            add(hb::zuReiter("intro.txt"));

            // --- Tracks ---
            setzenSchritt("Modi: Tracks - Vorfuehrskript", [=] {
                skriptSetzen(demo);
                ansicht(2);
                auswahl({wegVon("wait", 0)});
                weg();
            }, 0.9);
            bildTeil("vt_liste", flussR, [=] { return M{hb::Marke{hb::fensterRect("##spurenzeit"), 1}, hb::Marke{hb::fensterRect("##spurenliste"), 2}}; });
            bildTeil("vt_ein_doppel", mitListe, [=] { return M{hb::Marke{blockR(wegVon("wait", 0)), 1}, hb::Marke{hb::eintrag("print"), 2}}; });
            bildTeil("vt_ein_spur", mitListe, [=] {
                const ImRect r = blockR(wegVon("set", 1));
                const float x = r.Min.x + r.GetWidth() * 0.6F;
                return M{hb::Marke{hb::eintrag("sound"), 1}, hb::Marke{ImRect{x - 6.0F, r.Min.y - 3.0F, x + 6.0F, r.Max.y + 3.0F}, 2}};
            });
            bildTeil("vt_ein_liste", mitListe, [=] { return M{hb::Marke{hb::eintrag("task"), 1}, hb::Marke{listenZeile("affect", 0), 2}}; });
            bildTeil("vt_makro", mitListe, [=] { return M{hb::Marke{hb::eintrag("standOnly"), 1}, hb::Marke{listenZeile("camera", 1), 2}}; });
            bildTeil("vt_wert_doppel", flussR, [=] { return M{blockR(wegVon("wait", 0))}; });
            bildTeil("vt_wert_kante", flussR, [=] {
                const ImRect r = blockR(wegVon("wait", 0));
                return M{ImRect{r.Max.x - 7.0F, r.Min.y - 4.0F, r.Max.x + 7.0F, r.Max.y + 4.0F}};
            });
            bildTeil("vt_task_liste", flussR, [=] { return M{listenZeile("task", 0)}; });
            bildTeil("vt_task_spur", flussR, [=] { return M{blockR(wegVon("set", 1)), blockR(wegVon("set", 2))}; });
            // Fotolauf rc601: der Lauf war nur ein Strich, und in der Liste stand
            // die oberste Ebene weiter rechts als der Inhalt eines Blocks.
            add(pruefe("Tracks: Kyles Lauf ist ein Block mit Dauer, nicht nur ein Strich", [=] {
                const ImRect r = blockR(wegVon("set", 1));
                diag::detail("Knopftest: Handbuch - Laufblock " + std::to_string(r.GetWidth()) + " px");
                return r.GetWidth() > ImGui::GetFontSize() * 3.0F;
            }));
            add(pruefe("Liste: ein Befehl im Block steht weiter rechts als sein Block", [=] {
                const ImRect a1 = listenZeile("affect", 0);
                const ImRect t1 = listenZeile("task", 0);
                diag::detail("Knopftest: Handbuch - Einzug affect " + std::to_string(a1.Min.x) + ", task " + std::to_string(t1.Min.x));
                return a1.GetWidth() > 0.0F && t1.GetWidth() > 0.0F && t1.Min.x > a1.Min.x + 4.0F;
            }));
            setzenSchritt("Modi: Tracks - do statt dowait", [=] {
                const Path w = wegVon("dowait", 0);
                const Node* n = nodeAt(g_app->doc.script(), w);
                if (n != nullptr) {
                    Node d = *n;
                    d.name = "do";
                    (void)g_app->doc.replaceAt(w, d);
                }
                weg();
            }, 0.8);
            bildTeil("vt_task_do", flussR, [=] { return M{blockR(wegVon("set", 1)), blockR(wegVon("set", 2))}; });
            setzenSchritt("Modi: Tracks - do zurueck", [=] { (void)g_app->doc.undo(); auswahl({wegVon("wait", 1)}); weg(); }, 0.8);
            bildTeil("vt_ord_wahl", flussR, [=] { return M{blockR(wegVon("wait", 1))}; });
            add(rechtsklick("Tracks-Block", [=] { return blockR(wegVon("wait", 1)); }));
            add(pause(0.5));
            bildTeil("vt_ord_menue", popupR, menueMarken);
            add(zumUntermenue());
            add(pause(0.6));
            bildTeil("vt_ein_kontext", popupR, [=] { return M{knopf(tr(Str::GroupEvents), "##Popup")}; });
            add(daneben());
            add(pause(0.3));
            auto verschoben = std::make_shared<Path>();
            setzenSchritt("Modi: Tracks - wait nach oben", [=] {
                Path nachher;
                if (g_app->doc.moveUp(wegVon("wait", 1), &nachher)) { *verschoben = nachher; auswahl({nachher}); }
                weg();
            }, 0.8);
            bildTeil("vt_ord_nachher", flussR, [=] { return M{blockR(*verschoben)}; });
            setzenSchritt("Modi: Tracks - zurueck", [=] { (void)g_app->doc.undo(); weg(); }, 0.5);
            setzenSchritt("Modi: Tracks - waitsignal", [=] { skriptSetzen(kopf + "waitsignal ( \"door_open\" );\n" + kamera); weg(); }, 0.8);
            bildTeil("vt_sig_spur", flussR, [=] {
                const ImRect r = blockR(wegVon("waitsignal", 0));
                return M{r.GetWidth() > 0.0F ? r : listenZeile("waitsignal", 0)};
            });
            setzenSchritt("Modi: Tracks - Variablen", [=] { skriptSetzen(variable); weg(); }, 0.8);
            bildTeil("vt_var_liste", flussR, [=] { return M{listenZeile("declare", 0), listenZeile("if", 0), listenZeile("else", 0)}; });
            // Ein neues, leeres Skript in den Spuren.
            setzenSchritt("Modi: Tracks - neuer Reiter", [=] { neuerReiter(); ansicht(2); weg(); }, 0.6);
            bildTeil("vt_neu_leer", flussR, [=] { return M{hb::Marke{knopf(tr(Str::FlowTracks)), 1}, hb::Marke{hb::fensterRect("##spurenliste"), 2}}; });
            bildTeil("vt_neu_ziehen", mitListe, [=] { return M{hb::Marke{hb::eintrag("affect"), 1}, hb::Marke{hb::fensterRect("##spurenliste"), 2}}; });
            setzenSchritt("Modi: Tracks - affect kyle", [=] {
                befehlRein("affect");
                const Path w = wegVon("affect", 0);
                argSetzen(w, {"kyle"});
                auswahl({w});
                weg();
            }, 0.6);
            bildTeil("vt_neu_wait", mitListe, [=] { return M{hb::Marke{listenZeile("affect", 0), 1}, hb::Marke{hb::eintrag("set"), 2}}; });
            setzenSchritt("Modi: Tracks - Lauf in den Block", [=] {
                befehlRein("set");
                argSetzen(wegVon("set", 0), {"SET_NAVGOAL", "nav_kyle"});
                g_app->camTrackValid = false;
                weg();
            }, 1.0);
            bildTeil("vt_neu_ergebnis", flussR, [=] {
                const ImRect r = blockR(wegVon("set", 0));
                return M{r.GetWidth() > 0.0F ? r : listenZeile("set", 0)};
            });
            add(tu("Modi: Tracks - neuen Reiter verwerfen", [] {
                if (mb::reiterName(static_cast<std::size_t>(g_app->activeTab)) != "intro.txt") {
                    g_app->doc.markSaved();
                    reiterSchliessen(g_app->activeTab);
                }
            }));
            add(hb::zuReiter("intro.txt"));
            setzenSchritt("Modi: Ende", [=] {
                while (g_app->doc.undoDepth() > *tiefeModi && g_app->doc.undo()) {}
                ansicht(0);
            }, 0.5);
        }
        add(hb::zuReiter("intro.txt"));
        add(tu("Skriptbilder: Ende", [] { g_app->leftMode = 1; g_app->selection.clear(); }));
        add(pause(0.6));
    }

    // --- 11. Modell ----------------------------------------------------------------------------
    add(tu("Handbuch: Modell", [] {
        g_app->leftMode = 2;
        (void)npcLaden("Kyle");
    }));
    add(pause(1.5));
    add(hb::fotoTeil("h_modell", [] {
        const ImVec2 d = ImGui::GetIO().DisplaySize;
        const ImRect band = hb::fensterRect("toolband");
        return ImRect{0.0F, ImGui::GetFrameHeight() * 2.0F, d.x * 0.58F, band.Max.y};
    }));
    add(hb::fotoTeil("k_modell_leiste", [] {
        ImRect r = hb::fensterRect("toolband");
        r.Min.y -= ImGui::GetFontSize() * 2.4F;
        r.Max.x = r.Min.x + r.GetWidth() * 0.55F;
        return r;
    }));
    add(hb::fotoTeil("r_leiste_modell", [] {
        const float fs = ImGui::GetFontSize();
        return ImRect{g_app->kartenAnsicht[0] - fs * 0.4F, g_app->kartenAnsicht[1] - fs * 2.6F, g_app->kartenAnsicht[2] + fs * 0.4F,
                      g_app->kartenAnsicht[1] + 2.0F};
    }));
    add(tu("Handbuch: zurueck zur Karte", [] { g_app->leftMode = 1; }));
    add(pause(0.5));

    // --- 11b. Ein Tutorial im Programm (rc601 integ, Paket tour) ---------------------------------
    // Seite tutorials#im-programm: das Tutorial-Fenster und der gelbe Rahmen
    // um das "+" der Reiterleiste (erster Schritt von k-neues-skript).
    add(tu("Handbuch: Tutorial im Programm starten", [] {
        g_app->leftMode = 0;
        (void)tourStarten("k-neues-skript");
    }));
    add(pause(1.2));
    add(hb::fotoTeil("t_tour", voll));
    add(tu("Handbuch: Tutorial schliessen", [] {
        tourSchliessen();
        g_app->leftMode = 1;
    }));
    add(pause(0.5));

    // --- 12. Zuletzt geoeffnet, speichern -------------------------------------------------------
    add(klick(tr(Str::MenuFile), "###main"));
    add(klick(tr(Str::RecentMissions), "##Menu"));
    add(pause(0.5));
    const auto menueRect = [] {
        ImRect r{0.0F, 0.0F, 0.0F, 0.0F};
        for (ImGuiWindow* w : GImGui->Windows) {
            if (w->WasActive && !w->Hidden && std::strstr(w->Name, "##Menu") != nullptr) { r = hb::vereint(r, w->Rect()); }
        }
        r.Expand(ImGui::GetFontSize() * 0.5F);
        r.Min.y = 0.0F;
        return r;
    };
    add(hb::fotoTeil("h_menue_missionen", menueRect));
    add(hb::fotoTeil("t4_6", menueRect));
    add(klickAn("Menue zu", [] { return ImVec2{12.0F, ImGui::GetIO().DisplaySize.y - 6.0F}; }));
    add(pause(0.3));
    add(tu("Handbuch: Blick auf die Szene", [=] { blick(-1620.0F, -2120.0F, 830.0F, 26.0F, 90.0F); g_app->playMs = 4000.0; }));
    add(pause(0.8));
    add(hb::fotoTeil("t4_ergebnis", kartePlusLeiste));
    add(tu("Handbuch: Mission speichern (Fenster), Ziel im Laufordner", [] {
        const char* ziel = std::getenv("BHED_HB_ZIEL");
        g_app->mission.zielPk3 = ziel != nullptr ? std::string(ziel) : mb::verbinde(mb::laufOrdner(), "zz_taspir_tut.pk3");
        g_app->missionSpeichernOffen = true;
    }));
    add(pause(0.8));
    add(hb::fotoTeil("h_mission_speichern", [] { return hb::fensterRect("##missionspeichern"); }));
    add(hb::fotoTeil("t4_4", [] { return hb::fensterRect("##missionspeichern"); }));
    add(tu("Handbuch: speichern", [] {
        g_app->missionSpeichernOffen = false;
        std::string fehler;
        const bool ok = missionSpeichernNach(g_app->mission.zielPk3, &fehler);
        diag::detail("Knopftest: Handbuch - Mission gespeichert " + std::to_string(ok) + " " + fehler);
    }));
    add(pause(0.8));
    add(hb::fotoTeil("t4_5", [] {
        const ImVec2 d = ImGui::GetIO().DisplaySize;
        return ImRect{d.x * 0.58F, d.y * 0.80F, d.x, d.y};
    }));
    return s;
}

// --- ROFF-Bahnen bauen und aendern (rc599) ------------------------------------
//
// shank: "ich will das man auch direkt eine Roff datei bauen und aendern kann".
std::vector<Schritt> roffSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // --- Zwei Kombinationen aus der Handbuch-Pruefung (rc599) ------------------
    // 1. Strg+Rad ueber den Spuren der Zeitleiste zoomt die Zeitleiste - und
    //    vergroesserte bisher zugleich die ganze Oberflaeche.
    auto skala = std::make_shared<float>(1.0F);
    add({"Strg+Rad ueber der Zeitleiste", [=](int b) {
             ImGuiIO& io = ImGui::GetIO();
             if (b == 0) {
                 *skala = g_app->uiScale;
                 setzeMaus(ImVec2{(g_app->kartenAnsicht[0] + g_app->kartenAnsicht[2]) * 0.5F,
                                  g_app->kartenAnsicht[3] + ImGui::GetFontSize() * 7.0F});
                 return false;
             }
             if (b == 2) { io.AddKeyEvent(ImGuiMod_Ctrl, true); return false; }
             if (b == 3) { io.AddMouseWheelEvent(0.0F, 1.0F); return false; }
             if (b == 5) { io.AddKeyEvent(ImGuiMod_Ctrl, false); return false; }
             return b >= 9;
         }});
    add(pruefe("Strg+Rad ueber der Zeitleiste: nur die Zeitleiste zoomt, die Oberflaeche bleibt gleich gross", [=] {
        const float jetzt = g_app->uiScale;
        diag::detail("Knopftest: Kombi - Oberflaeche " + std::to_string(*skala) + " -> " + std::to_string(jetzt));
        g_app->uiScale = *skala;
        return std::fabs(jetzt - *skala) < 0.001F;
    }));
    // 2. Alt+Klick auf das Auge einer Gruppe zeigt nur diese Gruppe - so
    //    verspricht es der Tooltip der Liste.
    add(tu("Kombi: alle Gruppen sichtbar, Alt halten", [] {
        g_app->hiddenClasses.clear();
        g_app->expandedClass.clear();
        g_app->leftMode = 1;
        ImGui::GetIO().AddKeyEvent(ImGuiMod_Alt, true);
    }));
    add(pause(0.2));
    add(klick("O", "", 3));   // das Auge der vierten Gruppe: Figuren (NPC_*)
    add(tu("Kombi: Alt los", [] { ImGui::GetIO().AddKeyEvent(ImGuiMod_Alt, false); }));
    add(pause(0.2));
    add(pruefe("Alt+Klick auf ein Auge: nur diese Gruppe bleibt sichtbar", [] {
        const bool npcSichtbar = g_app->hiddenClasses.count("NPC_spawner") == 0U;
        const bool tagVersteckt = g_app->hiddenClasses.count("ref_tag") != 0U;
        diag::detail("Knopftest: Kombi - versteckt " + std::to_string(g_app->hiddenClasses.size()) + " Klassen, NPC_spawner " +
                     (npcSichtbar ? "sichtbar" : "versteckt") + ", ref_tag " + (tagVersteckt ? "versteckt" : "sichtbar"));
        const bool ok = npcSichtbar && tagVersteckt;
        g_app->hiddenClasses.clear();
        g_app->mapDirty = true;
        return ok;
    }));
    auto idx = std::make_shared<int>(-1);
    const auto schiffName = [] {
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(g_app->animEntity)];
        const std::string* st = e.find("script_targetname");
        return st != nullptr ? *st : e.targetname;
    };
    const auto marke = [] {
        const auto f = findeRoffMarke(g_app->doc.script(), g_app->animRoff);
        return f ? f->anim : RoffAnim{};
    };
    const auto waehle = [idx](const char* text, const char* name) {
        openScriptFromMemory(text, name);
        *idx = -1;
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const MapEntity& e = g_app->map.entities[i];
            const std::string* st = e.find("script_targetname");
            if (e.classname == "func_static" && st != nullptr && *st == "sta2") { *idx = static_cast<int>(i); }
        }
        if (*idx >= 0) {
            entityAuswahlSetzen(*idx);
            zurEntityFliegen(static_cast<std::size_t>(*idx));
        }
        g_app->playMs = 0.0;
        g_app->playing = false;
        prepareScene();
    };
    add(tu("ROFF: ein Skript, das Schiff waehlen", [=] {
        waehle("//Generated by BehavEd\ncamera ( /*@CAMERA_COMMANDS*/ ENABLE );\nwait ( 6000.000 );\n", "rofftest.txt");
        diag::detail("Knopftest: ROFF - Entity " + std::to_string(*idx));
    }));
    add(pause(0.6));
    add(klick(tr(Str::EntAnimate), "###main"));
    add(pause(0.3));
    add(tu("Zeit 1 s", [] { g_app->playMs = 1000.0; g_app->mapDirty = true; }));
    add(klick(tr(Str::AnimModeRoff), "###animation"));
    add(pause(0.4));
    add(pruefe("ROFF: Umschalten legt eine Bahn an - Marke, affect-Block, play ( PLAY_ROFF ) am Skriptanfang", [=] {
        const auto blk = findeRoffBlock(g_app->doc.script(), schiffName());
        const std::string text = writeScript(g_app->doc.script());
        diag::detail("Knopftest: ROFF - Datei \"" + g_app->animRoff + "\", erste Zeile " +
                     (g_app->doc.script().nodes.empty() ? std::string("-") : g_app->doc.script().nodes[0].raw));
        return !g_app->animRoff.empty() && blk.has_value() && *blk == 0U &&
               text.find("\"PLAY_ROFF\", \"" + g_app->animRoff + "\"") != std::string::npos && marke().schluessel.size() == 1U;
    }));
    // Rueckgaengig und Wiederholen: der Modus des Fensters folgt dem Skript
    // (rc599: nach Strg+Z blieb es im ROFF-Modus haengen, ohne Block).
    auto roffName = std::make_shared<std::string>();
    add(tu("ROFF: Name merken, Maus weg (Strg+Z gehoert dann dem Skript)", [=] {
        *roffName = g_app->animRoff;
        setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x - 30.0F, 200.0F});
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.4));
    add(pruefe("ROFF: Strg+Z nimmt den Block zurueck - das Fenster ist wieder bei Skript (move)", [=] {
        diag::detail("Knopftest: ROFF - nach Strg+Z Datei \"" + g_app->animRoff + "\", Block " +
                     std::to_string(findeRoffBlock(g_app->doc.script(), schiffName()).has_value()));
        return g_app->animRoff.empty() && !findeRoffBlock(g_app->doc.script(), schiffName()).has_value();
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Y, "Strg+Y"));
    add(pause(0.4));
    add(pruefe("ROFF: Strg+Y bringt den Block zurueck - das Fenster ist wieder bei der ROFF-Datei", [=] {
        diag::detail("Knopftest: ROFF - nach Strg+Y Datei \"" + g_app->animRoff + "\"");
        return !g_app->animRoff.empty() && g_app->animRoff == *roffName;
    }));
    add(tu("Zeit 4 s", [] { g_app->playMs = 4000.0; g_app->mapDirty = true; }));
    add(klick(tr(Str::AnimAddKey), "###animation"));
    add(pause(0.4));
    add(pruefe("ROFF: zweiter Schluessel 3 s nach dem ersten", [=] {
        const RoffAnim a = marke();
        diag::detail("Knopftest: ROFF - " + std::to_string(a.schluessel.size()) + " Schluessel, Marke " + roffMarke(a));
        return a.schluessel.size() == 2U && std::fabs(a.schluessel[1].ms - 3000.0) < 1.0;
    }));
    // Die Raute des zweiten Schluessels ziehen.
    const auto raute = [] {
        const RoffAnim a = [] {
            const auto f = findeRoffMarke(g_app->doc.script(), g_app->animRoff);
            return f ? f->anim : RoffAnim{};
        }();
        ImVec2 sp{0, 0};
        float o[3] = {0, 0, 0};
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(g_app->animEntity)];
        (void)entityOrt(e, o);
        if (a.schluessel.size() == 2U) {
            const float w[3] = {o[0] + a.schluessel[1].ort[0], o[1] + a.schluessel[1].ort[1], o[2] + a.schluessel[1].ort[2]};
            (void)kartenPunktAufSchirm(w, sp);
        }
        return sp;
    };
    add(ziehenAn("ROFF: die Raute des zweiten Schluessels ziehen", raute, 150.0F, -50.0F, 0));
    add(pause(0.5));
    add(pruefe("ROFF: der zweite Schluessel ist verschoben, der erste bleibt der Start (0 0 0)", [=] {
        const RoffAnim a = marke();
        if (a.schluessel.size() != 2U) { return false; }
        const float d = std::fabs(a.schluessel[1].ort[0]) + std::fabs(a.schluessel[1].ort[1]) + std::fabs(a.schluessel[1].ort[2]);
        const float d0 = std::fabs(a.schluessel[0].ort[0]) + std::fabs(a.schluessel[0].ort[1]) + std::fabs(a.schluessel[0].ort[2]);
        diag::detail("Knopftest: ROFF - zweiter Schluessel " + std::to_string(d) + " vom Start, erster " + std::to_string(d0));
        return d > 10.0F && d0 < 0.01F;
    }));
    add(tu("Szene neu", [] { prepareScene(); }));
    add(pause(0.3));
    add(foto("roff_bahn"));
    add(pruefe("ROFF: die Szene spielt die gebackene Datei - bei 4 s am zweiten Schluessel", [=] {
        const RoffAnim a = marke();
        const std::string name = schiffName();
        for (const Actor& ac : g_app->scene.actors) {
            if (ac.name != name) { continue; }
            const ActorStep* roff = nullptr;
            for (const ActorStep& st : ac.steps) {
                if (st.kind == ActorStep::Kind::Roff) { roff = &st; }
            }
            if (roff == nullptr || a.schluessel.size() != 2U) { return false; }
            const ActorState s4 = ac.at(4000.0);
            float d = 0.0F;
            for (int c = 0; c < 3; ++c) { d += std::fabs(s4.pos[c] - (roff->from[c] + a.schluessel[1].ort[c])); }
            diag::detail("Knopftest: ROFF - play bei " + std::to_string(roff->startMs) + " ms, bei 4 s " +
                         std::to_string(d) + " vom Schluessel");
            return std::fabs(roff->startMs - 950.0) < 1.0 && d < 1.0F;
        }
        return false;
    }));
    auto rtiefe = std::make_shared<std::size_t>(0);
    add(tu("ROFF: Zeiger auf 2,5 s, Drehmodus, das Schiff gewaehlt", [=] {
        *rtiefe = g_app->doc.undoDepth();
        g_app->playMs = 2500.0;
        g_app->playing = false;
        entityAuswahlSetzen(*idx);
        g_app->gizmoMode = App::GizmoMode::Rotate;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(ziehenAn("ROFF: am Gier-Ring ziehen (Gizmo am Zeiger)", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(4, q);
        return q;
    }, 50.0F, -25.0F, 0));
    add(pause(0.5));
    add(pruefe("ROFF: Drehen mit dem Gizmo setzt einen Schluessel bei 2,5 s - der Start bleibt", [=] {
        const RoffAnim a = marke();
        std::string z;
        for (const RoffSchluessel& k : a.schluessel) { z += " " + std::to_string(static_cast<int>(k.ms)) + "/" + std::to_string(k.winkel[1]); }
        diag::detail("Knopftest: ROFF - Gizmo am Zeiger:" + z);
        return a.schluessel.size() == 3U && std::fabs(a.schluessel[1].ms - 1500.0) < 1.0 &&
               std::fabs(a.schluessel[1].winkel[1]) > 2.0F && std::fabs(a.schluessel[0].winkel[1]) < 0.01F;
    }));
    add(tu("ROFF: zuruecknehmen, Verschieben", [=] {
        while (g_app->doc.undoDepth() > *rtiefe && g_app->doc.undo()) {}
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->mapDirty = true;
        g_app->gizmoMode = App::GizmoMode::Move;
    }));
    add(pruefe("ROFF: die Zeitleiste zeigt die Fahrt als Band (Bewegung) am play", [] {
        int roffBalken = 0;
        for (const App::TlBalken& bk : g_app->tlBalken) {
            const Node* n = nodeAt(g_app->doc.script(), bk.pfad);
            if (n != nullptr && n->name == "play") { ++roffBalken; }
        }
        diag::detail("Knopftest: ROFF - Baender am play " + std::to_string(roffBalken) + " von " +
                     std::to_string(g_app->tlBalken.size()));
        return roffBalken > 0;
    }));
    // Kombination: die Entity bekommt einen anderen Namen, nachdem die Bahn
    // angelegt ist (rc599). Der eigene Block muss weiter bearbeitbar sein -
    // und es darf kein zweiter entstehen.
    add(tu("ROFF: Schiff umbenennen (script_targetname sta2 -> sta2b)", [=] {
        (void)entityKeySetzen(g_app->animEntity, "script_targetname", "sta2b");
        g_app->playMs = 5500.0;
    }));
    add(pause(0.3));
    add(klick(tr(Str::AnimAddKey), "###animation"));
    add(pause(0.4));
    add(pruefe("ROFF: nach dem Umbenennen - ein Schluessel mehr, ein einziger Block, affect auf den neuen Namen", [=] {
        const RoffAnim a = marke();
        int bloecke = 0;
        std::string affect;
        for (const RoffMarkeFund& f : alleRoffMarken(g_app->doc.script())) {
            if (f.path.size() == 1U && f.path[0] + 1U < g_app->doc.script().nodes.size()) {
                ++bloecke;
                const Node& n = g_app->doc.script().nodes[f.path[0] + 1U];
                if (!n.args.empty()) { affect = n.args[0].text; }
            }
        }
        diag::detail("Knopftest: ROFF - nach Umbenennen " + std::to_string(a.schluessel.size()) + " Schluessel, " +
                     std::to_string(bloecke) + " Bloecke, affect \"" + affect + "\"");
        return a.schluessel.size() == 3U && bloecke == 1 && affect == "sta2b";
    }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(tu("ROFF: Namen zurueck", [=] {
        (void)entityKeySetzen(g_app->animEntity, "script_targetname", "sta2");
        g_app->camTrackValid = false;
    }));
    add(pause(0.3));
    add(klick(tr(Str::AnimRoffSmooth), "###animation"));
    add(pause(0.3));
    add(pruefe("ROFF: Weich aus - gleichmaessig", [=] { return !marke().weich; }));
    add(tu("Maus weg (Strg+Z gehoert dann dem Skript)", [] { setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x - 30.0F, 200.0F}); }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("ROFF: Strg+Z - wieder weich, Schluessel unveraendert", [=] {
        const RoffAnim a = marke();
        return a.weich && a.schluessel.size() == 2U;
    }));
    // Speichern: die .rof entsteht neben dem Skript.
    auto ordner = std::make_shared<std::string>();
    add(tu("ROFF: Skript in einen scripts-Ordner speichern", [=] {
        std::error_code ec;
        const std::filesystem::path w = std::filesystem::temp_directory_path(ec) / "bhed_rofftest";
        std::filesystem::remove_all(w, ec);
        std::filesystem::create_directories(w / "scripts" / "md_test", ec);
        const auto u = w.u8string();
        *ordner = std::string(u.begin(), u.end());
        (void)speichernUnterPfad(*ordner + "\\scripts\\md_test\\rofftest.txt");
    }));
    add(pause(0.3));
    add(pruefe("ROFF: gespeichert als scripts/<datei>.rof - die Engine liest sie (G_ValidRoff), Bahn wie im Skript", [=] {
        const RoffAnim a = marke();
        const std::string pfad = *ordner + "/scripts/" + a.datei + ".rof";
        std::ifstream f(::bhed::utf8Pfad(pfad), std::ios::binary);
        const std::string daten((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        Roff r;
        const bool ok = leseRoff(daten, r);
        const Roff soll = roffAusAnim(a);
        diag::detail("Knopftest: ROFF - " + pfad + ": " + std::to_string(daten.size()) + " Byte, " +
                     std::to_string(r.bilder.size()) + " Bilder");
        return ok && r.fassung == 2 && r.bildMs == 50 && r.bilder.size() == soll.bilder.size() && daten == schreibeRoff(soll);
    }));
    add(klick(tr(Str::AnimModeMove), "###animation"));
    add(pause(0.4));
    add(pruefe("ROFF: zurueck zu Skript (move) - zwei move-Schluessel, keine Marke mehr", [=] {
        const auto at = findeAnimBlock(g_app->doc.script(), schiffName());
        const auto k = at ? leseAnimBlock(g_app->doc.script(), *at) : std::vector<AnimSchluessel>{};
        return g_app->animRoff.empty() && k.size() == 2U && alleRoffMarken(g_app->doc.script()).empty();
    }));
    add(tu("Fenster zu, gespeichert, Ordner weg", [=] {
        g_app->animFensterOffen = false;
        g_app->doc.markSaved();
        std::error_code ec;
        std::filesystem::remove_all(::bhed::utf8Pfad(*ordner), ec);
    }));
    add(pause(0.2));

    // Eine fertige Datei des Spiels bearbeiten: kor2/roffs/boulder.
    auto vorher = std::make_shared<std::vector<std::array<float, 3>>>();
    add(tu("ROFF: ein Skript mit PLAY_ROFF auf eine Datei des Spiels", [=] {
        waehle("//Generated by BehavEd\naffect ( \"sta2\", /*@AFFECT_TYPE*/ FLUSH )\n{\n\twait ( 500.000 );\n"
               "\tplay ( /*@PLAY_TYPES*/ \"PLAY_ROFF\", \"kor2/roffs/boulder\" );\n}\n",
               "roffedit.txt");
        vorher->clear();
        const std::string name = schiffName();
        for (const Actor& ac : g_app->scene.actors) {
            if (ac.name != name) { continue; }
            for (double t = 0.0; t <= 2500.0; t += 100.0) {
                const ActorState st = ac.at(t);
                vorher->push_back({st.pos[0], st.pos[1], st.pos[2]});
            }
        }
    }));
    add(pause(0.5));
    add(klick(tr(Str::EntAnimate), "###main"));
    add(pause(0.4));
    add(pruefe("ROFF: das Fenster bietet die Bahn der Datei zum Bearbeiten an", [=] {
        diag::detail("Knopftest: ROFF - Staende vorher " + std::to_string(vorher->size()));
        return g_app->animFensterOffen && g_app->animRoff.empty() && vorher->size() == 26U;
    }));
    add(klick(tr(Str::AnimRoffEdit), "###animation"));
    add(pause(0.5));
    add(pruefe("ROFF: Bearbeiten - Kopie boulder_2, Marke vor dem play, das play zeigt auf die Kopie", [=] {
        const RoffAnim a = marke();
        const std::string text = writeScript(g_app->doc.script());
        diag::detail("Knopftest: ROFF - bearbeitet \"" + g_app->animRoff + "\", " + std::to_string(a.schluessel.size()) +
                     " Schluessel, " + (a.weich ? "weich" : "gleichmaessig"));
        return g_app->animRoff == "kor2/roffs/boulder_2" && a.schluessel.size() >= 2U &&
               text.find("\"PLAY_ROFF\", \"kor2/roffs/boulder_2\"") != std::string::npos &&
               text.find("\"kor2/roffs/boulder\"") == std::string::npos;
    }));
    add(tu("Szene neu", [] { prepareScene(); }));
    add(pruefe("ROFF: die Kopie faehrt wie das Original (hoechstens 1 Einheit daneben)", [=] {
        const std::string name = schiffName();
        float weit = 0.0F;
        std::size_t i = 0;
        for (const Actor& ac : g_app->scene.actors) {
            if (ac.name != name) { continue; }
            for (double t = 0.0; t <= 2500.0 && i < vorher->size(); t += 100.0, ++i) {
                const ActorState st = ac.at(t);
                for (int c = 0; c < 3; ++c) { weit = std::max(weit, std::fabs(st.pos[c] - (*vorher)[i][static_cast<std::size_t>(c)])); }
            }
        }
        diag::detail("Knopftest: ROFF - Kopie gegen Original: " + std::to_string(weit));
        return i == vorher->size() && weit <= 1.0F;
    }));
    add(tu("Fenster zu, gespeichert", [] { g_app->animFensterOffen = false; g_app->doc.markSaved(); }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> animSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto idx = std::make_shared<int>(-1);
    add(tu("Animation: ein Skript, das Schiff waehlen", [=] {
        openScriptFromMemory("//Generated by BehavEd\ncamera ( /*@CAMERA_COMMANDS*/ ENABLE );\nwait ( 6000.000 );\n",
                             "animation.txt");
        *idx = -1;
        for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
            const MapEntity& e = g_app->map.entities[i];
            const std::string* st = e.find("script_targetname");
            if (e.classname == "func_static" && st != nullptr && *st == "sta2") { *idx = static_cast<int>(i); }
        }
        if (*idx < 0) {
            for (std::size_t i = 0; i < g_app->map.entities.size(); ++i) {
                const MapEntity& e = g_app->map.entities[i];
                if (e.classname.rfind("func_", 0) == 0 && e.find("origin") != nullptr) { *idx = static_cast<int>(i); break; }
            }
        }
        if (*idx >= 0) {
            entityAuswahlSetzen(*idx);
            zurEntityFliegen(static_cast<std::size_t>(*idx));
        }
        g_app->playMs = 0.0;
        g_app->playing = false;
        prepareScene();
        diag::detail("Knopftest: Animation - Entity " + std::to_string(*idx));
    }));
    add(pause(0.6));
    add(klick(tr(Str::EntAnimate), "###main"));
    add(pause(0.3));
    add(pruefe("Inspektor: Animieren oeffnet das Fenster fuer diese Entity", [=] {
        return *idx >= 0 && g_app->animFensterOffen && g_app->animEntity == *idx;
    }));
    add(tu("Zeit 1 s", [] { g_app->playMs = 1000.0; }));
    add(klick(tr(Str::AnimAddKey), "###animation"));
    add(pause(0.2));
    add(tu("Zeit 4 s", [] { g_app->playMs = 4000.0; g_app->mapDirty = true; }));
    add(klick(tr(Str::AnimAddKey), "###animation"));
    add(pause(0.4));
    const auto schluessel = [] {
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(g_app->animEntity)];
        const std::string* st = e.find("script_targetname");
        const std::string name = st != nullptr ? *st : e.targetname;
        const auto at = findeAnimBlock(g_app->doc.script(), name);
        return at ? leseAnimBlock(g_app->doc.script(), *at) : std::vector<AnimSchluessel>{};
    };
    add(pruefe("Zwei Schluessel bei 1 s und 4 s, am Anfang des Skripts als affect-Block", [=] {
        const auto k = schluessel();
        diag::detail("Knopftest: Animation - " + std::to_string(k.size()) + " Schluessel, erste Zeile " +
                     g_app->doc.script().nodes[0].raw);
        return k.size() == 2U && std::fabs(k[0].ms - 1000.0) < 1.0 && std::fabs(k[1].ms - 4000.0) < 1.0 &&
               g_app->doc.script().nodes.size() >= 4U && g_app->doc.script().nodes[1].name == "affect";
    }));
    auto vor = std::make_shared<std::array<float, 3>>();
    add(tu("Ort des zweiten Schluessels merken", [=] { *vor = schluessel()[1].ort; }));
    add(pause(0.3));
    add(foto("anim_zwei"));
    add(tu("Wo liegt die Raute?", [=] {
        const auto k = schluessel();
        ImVec2 sp{0, 0};
        bool ok = false;
        if (k.size() == 2U) {
            const float w[3] = {k[1].ort[0], k[1].ort[1], k[1].ort[2]};
            ok = kartenPunktAufSchirm(w, sp);
        }
        setzeMaus(sp);
        diag::detail("Knopftest: Animation - Raute bei " + std::to_string(sp.x) + "/" + std::to_string(sp.y) + " (" +
                     std::to_string(ok) + "), Bild " + std::to_string(g_app->kartenAnsicht[0]) + ".." +
                     std::to_string(g_app->kartenAnsicht[2]));
    }));
    add(pause(0.2));
    add(tu("Fenster unter der Maus", [] {
        diag::detail(std::string("Knopftest: Animation - unter der Maus ") +
                     (GImGui->HoveredWindow != nullptr ? GImGui->HoveredWindow->Name : "-") + ", Zeit " +
                     std::to_string(g_app->playMs));
    }));
    add(ziehenAn("die Raute des zweiten Schluessels ziehen", [=] {
            const auto k = schluessel();
            ImVec2 sp{0, 0};
            if (k.size() == 2U) {
                const float w[3] = {k[1].ort[0], k[1].ort[1], k[1].ort[2]};
                kartenPunktAufSchirm(w, sp);
            }
            return sp;
        }, 140.0F, -40.0F, 0));
    add(pause(0.4));
    add(pruefe("Ziehen: der zweite Schluessel hat einen neuen Ort, der erste nicht", [=] {
        diag::detail("Knopftest: Animation - Schluessel gewaehlt " + std::to_string(g_app->animSchluessel) + ", Zug " +
                     std::to_string(g_app->animZug));
        const auto k = schluessel();
        if (k.size() != 2U) { return false; }
        const float d = std::fabs(k[1].ort[0] - (*vor)[0]) + std::fabs(k[1].ort[1] - (*vor)[1]) + std::fabs(k[1].ort[2] - (*vor)[2]);
        diag::detail("Knopftest: Animation - zweiter Schluessel um " + std::to_string(d) + " verschoben");
        return d > 10.0F;
    }));
    add(tu("Szene neu", [] { prepareScene(); }));
    add(pruefe("Die Szene faehrt die Bahn: bei 4 s am zweiten Schluessel, bei 2,5 s dazwischen", [=] {
        const auto k = schluessel();
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(g_app->animEntity)];
        const std::string* st = e.find("script_targetname");
        for (const Actor& a : g_app->scene.actors) {
            if (st == nullptr || a.name != *st) { continue; }
            const ActorState s4 = a.at(4000.0);
            const ActorState s2 = a.at(2500.0);
            diag::detail("Knopftest: Animation - bei 4 s " + std::to_string(s4.pos[0]) + " " + std::to_string(s4.pos[1]) +
                         ", bei 2,5 s " + std::to_string(s2.pos[0]));
            const bool am = std::fabs(s4.pos[0] - k[1].ort[0]) < 1.0F && std::fabs(s4.pos[1] - k[1].ort[1]) < 1.0F;
            const float lo = std::min(k[0].ort[0], k[1].ort[0]) - 0.5F;
            const float hi = std::max(k[0].ort[0], k[1].ort[0]) + 0.5F;
            return am && s2.pos[0] >= lo && s2.pos[0] <= hi;
        }
        return false;
    }));
    // Kombination (rc599): das Gizmo am Zeiger, waehrend das Fenster im
    // move-Modus ist - Drehen setzt einen dritten Schluessel bei 2,5 s, der
    // Ursprung der Entity bleibt.
    auto vorDreh = std::make_shared<std::pair<std::size_t, std::string>>();
    add(tu("Gizmo am Zeiger: 2,5 s, Drehmodus", [=] {
        vorDreh->first = g_app->doc.undoDepth();
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(g_app->animEntity)];
        vorDreh->second = e.find("angles") != nullptr ? *e.find("angles") : (e.find("angle") != nullptr ? *e.find("angle") : "");
        entityAuswahlSetzen(g_app->animEntity);
        g_app->playMs = 2500.0;
        g_app->gizmoMode = App::GizmoMode::Rotate;
        g_app->mapDirty = true;
    }));
    add(pause(0.5));
    add(ziehenAn("Gizmo am Zeiger: am Gier-Ring ziehen", [] {
        ImVec2 q{-1.0F, -1.0F};
        (void)entityGizmoPunkt(4, q);
        return q;
    }, 50.0F, -25.0F, 0));
    add(pause(0.5));
    add(pruefe("Gizmo am Zeiger (move-Modus): ein dritter Schluessel bei 2,5 s mit gedrehter Gier, die Entity selbst unveraendert", [=] {
        const auto k = schluessel();
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(g_app->animEntity)];
        const std::string jetzt = e.find("angles") != nullptr ? *e.find("angles") : (e.find("angle") != nullptr ? *e.find("angle") : "");
        diag::detail("Knopftest: Animation - Gizmo am Zeiger: " + std::to_string(k.size()) + " Schluessel" +
                     (k.size() == 3U ? ", Gier bei 2,5 s " + std::to_string(k[1].winkel[1]) : std::string()));
        return k.size() == 3U && std::fabs(k[1].ms - 2500.0) < 1.0 && std::fabs(k[1].winkel[1]) > 2.0F && jetzt == vorDreh->second;
    }));
    add(tu("Gizmo am Zeiger: zuruecknehmen, Verschieben", [=] {
        while (g_app->doc.undoDepth() > vorDreh->first && g_app->doc.undo()) {}
        rebuildTree();
        g_app->camTrackValid = false;
        g_app->gizmoMode = App::GizmoMode::Move;
    }));
    add(tu("ersten Schluessel waehlen", [] { g_app->animSchluessel = 0; }));
    add(klick(tr(Str::AnimDeleteKey), "###animation"));
    add(pause(0.2));
    add(pruefe("Schluessel loeschen: einer bleibt", [=] { return schluessel().size() == 1U; }));
    add(tu("Maus weg (Strg+Z gehoert dann dem Skript)", [] { setzeMaus(ImVec2{ImGui::GetIO().DisplaySize.x - 30.0F, 200.0F}); }));
    add(tasteMit(ImGuiMod_Ctrl, ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("Strg+Z: wieder zwei", [=] { return schluessel().size() == 2U; }));
    add(klick(tr(Str::AnimRemove), "###animation"));
    add(pause(0.2));
    add(pruefe("Animation entfernen: kein Block mehr", [=] {
        return schluessel().empty() && g_app->doc.script().nodes.size() == 2U;
    }));
    add(tu("Fenster zu, gespeichert", [] { g_app->animFensterOffen = false; g_app->doc.markSaved(); }));
    add(pause(0.2));
    return s;
}

