// selbsttest_karte8c.inc - Stufe 8c des Karten-Editors: das Tutorial "Erste eigene
// Karte", Schritt fuer Schritt wie ein Mensch in der Oberflaeche (Maus, Tasten,
// Menues), mit Wirkungspruefungen nach jedem Schritt und den Bildern fuer das
// Handbuch (tools/handbuch/inhalt_karte8c.py).
//
// Eingebunden in gui/selbsttest.cpp nach dem Namensraum hb (fotoTeil). Laeuft im
// Modus karteneditor (BHED_KARTE_NUR=stufe8c nur dieser Teil) und als Bildlauf
// BHED_EDITORTEST=handbuch mit BHED_HB_NUR=tutorial (dann mit BHED_FOTOS).
// Am Ende: die gebaute Karte hat kein Leck, der Spieler steht am
// info_player_start und laeuft, Build > Pack map as .pk3 legt maps/<Karte>.bsp
// in ein Archiv neben der .bsp - nie in einen Spielordner.
namespace stufe8c {

namespace ke = karteneditor;
using stufe3::istDatei;
using stufe3::konsoleText;

// Die Stelle eines Weltpunkts in der aktiven 2D-Ansicht (jede Sicht: XY, XZ, YZ)
ImVec2 bild2d(karte::Vektor3f p) {
    const ke::XyFenster& f = ke::editor().xy[0];
    const auto b = f.ansicht.zuBild(p);
    return ImVec2{f.bildX + b[0], f.bildY + b[1]};
}

void mods(ImGuiKeyChord m, bool an) {
    ImGuiIO& io = ImGui::GetIO();
    if ((m & ImGuiMod_Ctrl) != 0) { io.AddKeyEvent(ImGuiMod_Ctrl, an); }
    if ((m & ImGuiMod_Shift) != 0) { io.AddKeyEvent(ImGuiMod_Shift, an); }
    if ((m & ImGuiMod_Alt) != 0) { io.AddKeyEvent(ImGuiMod_Alt, an); }
}

// Maus ziehen in der 2D-Ansicht: Zusatztasten, Druck, zehn Schritte, Loslassen.
Schritt ziehen2d(const std::string& was, int taste, ImGuiKeyChord m, karte::Vektor3f von, karte::Vektor3f nach) {
    return Schritt{was, [=](int b) {
                       ImGuiIO& io = ImGui::GetIO();
                       const ImVec2 a = bild2d(von);
                       const ImVec2 z = bild2d(nach);
                       if (b == 0) {
                           setzeMaus(a);
                           mods(m, true);
                           return false;
                       }
                       if (b == 1) {
                           io.AddMouseButtonEvent(taste, true);
                           return false;
                       }
                       if (b >= 2 && b <= 11) {
                           const float t = static_cast<float>(b - 1) / 10.0F;
                           setzeMaus(ImVec2{a.x + (z.x - a.x) * t, a.y + (z.y - a.y) * t});
                           return false;
                       }
                       if (b == 12) {
                           io.AddMouseButtonEvent(taste, false);
                           return false;
                       }
                       if (b == 13) { mods(m, false); }
                       return b >= 15;
                   }};
}

// Ein Klick in der 2D-Ansicht (mit Zusatztasten)
Schritt klick2d(const std::string& was, int taste, ImGuiKeyChord m, karte::Vektor3f an) {
    return Schritt{was, [=](int b) {
                       ImGuiIO& io = ImGui::GetIO();
                       if (b == 0) {
                           setzeMaus(bild2d(an));
                           mods(m, true);
                           return false;
                       }
                       if (b == 2) {
                           io.AddMouseButtonEvent(taste, true);
                           return false;
                       }
                       if (b == 3) {
                           io.AddMouseButtonEvent(taste, false);
                           return false;
                       }
                       if (b == 4) { mods(m, false); }
                       return b >= 6;
                   }};
}

std::size_t pinselZahl() {
    const auto w = ke::editor().karte.welt();
    return w ? w->kinder().size() : 0;
}

karte::Kasten gesamtGrenzen() {
    karte::Vektor3f lo{1e9F, 1e9F, 1e9F};
    karte::Vektor3f hi{-1e9F, -1e9F, -1e9F};
    const auto w = ke::editor().karte.welt();
    if (w) {
        for (const auto& p : w->kinder()) {
            if (p->art() != karte::Primitiv::Art::Pinsel) { continue; }
            const karte::Kasten& b = std::static_pointer_cast<karte::Pinsel>(p)->grenzen(ke::editor().karte.kontext);
            for (std::size_t i = 0; i < 3; ++i) {
                lo[i] = std::min(lo[i], b.origin[i] - b.extents[i]);
                hi[i] = std::max(hi[i], b.origin[i] + b.extents[i]);
            }
        }
    }
    karte::Kasten g;
    g.origin = (lo + hi) * 0.5F;
    g.extents = (hi - lo) * 0.5F;
    return g;
}

bool kastenIst(const karte::Kasten& b, karte::Vektor3f lo, karte::Vektor3f hi) {
    for (std::size_t i = 0; i < 3; ++i) {
        if (std::fabs((b.origin[i] - b.extents[i]) - lo[i]) > 0.01F || std::fabs((b.origin[i] + b.extents[i]) - hi[i]) > 0.01F) { return false; }
    }
    return true;
}

// Flaechen aller Weltpinsel mit diesem Shader
int flaechenMit(const std::string& shader) {
    int n = 0;
    const auto w = ke::editor().karte.welt();
    if (!w) { return 0; }
    for (const auto& p : w->kinder()) {
        if (p->art() != karte::Primitiv::Art::Pinsel) { continue; }
        for (const auto& f : std::static_pointer_cast<karte::Pinsel>(p)->flaechen()) { n += f->shader() == shader ? 1 : 0; }
    }
    return n;
}

// Was gewaehlt ist (fuer das Protokoll): Klasse bzw. Pinsel mit Grenzen
std::string auswahlText() {
    const ke::Editor& e = ke::editor();
    std::string t;
    for (const karte::AuswahlElement& el : e.karte.auswahl.elemente()) {
        const auto ent = el.entitaet.lock();
        const auto p = el.primitiv.lock();
        if (p && p->art() == karte::Primitiv::Art::Pinsel) {
            const karte::Kasten& b = std::static_pointer_cast<karte::Pinsel>(p)->grenzen(e.karte.kontext);
            char z[160];
            std::snprintf(z, sizeof(z), " [Pinsel %g %g %g .. %g %g %g]", static_cast<double>(b.origin[0] - b.extents[0]), static_cast<double>(b.origin[1] - b.extents[1]),
                          static_cast<double>(b.origin[2] - b.extents[2]), static_cast<double>(b.origin[0] + b.extents[0]),
                          static_cast<double>(b.origin[1] + b.extents[1]), static_cast<double>(b.origin[2] + b.extents[2]));
            t += z;
        } else if (ent) {
            t += " [" + ent->klassenname() + "]";
        }
    }
    return t;
}

std::shared_ptr<karte::Entitaet> entitaet(const char* klasse) {
    for (const auto& ent : ke::editor().karte.entitaeten()) {
        if (ent->klassenname() == klasse) { return ent; }
    }
    return nullptr;
}

// Zwei Texturen des Spiels fuer Wand und Boden (ohne Durchsicht, mit Bild), aus
// dem ersten passenden Ordner; der Ordner steht danach im Texturbrowser.
struct Wahl {
    std::string ordner;
    std::string wand;
    std::string boden;
};

bool texturenWaehlen(Wahl& w) {
    ke::Editor& e = ke::editor();
    if (!e.karteSpiel) { return false; }
    const karte::Texturinventar& inv = e.karteSpiel->inventar;
    for (const char* dir : {"imperial", "kejim", "yavin", "doomgiver", "hoth", "vjun", "factory"}) {
        std::vector<std::string> v;
        for (const karte::ShaderEintrag& s : inv.verzeichnis(dir)) {
            if ((s.flags & (karte::QER_TRANS | karte::QER_SKY | karte::QER_ALPHATEST | karte::QER_FOG)) != 0U) { continue; }
            if (!inv.bildMass(s.name).gefunden) { continue; }
            v.push_back(s.name);
            if (v.size() >= 6) { break; }   // die ersten: oben im Browser, ohne Rollen sichtbar
        }
        if (v.size() < 2) { continue; }
        w.ordner = std::string(dir) + "/";
        w.wand = v[0];
        w.boden = v[1];
        for (const std::string& n : v) {
            if (n.find("floor") != std::string::npos && n != w.wand) {
                w.boden = n;
                break;
            }
        }
        return true;
    }
    return false;
}

std::string kartenPfad() { return stufe3::laufOrdner("tutorial") + "\\maps\\meinekarte.map"; }
std::string stamm() { return stufe3::laufOrdner("tutorial") + "\\maps\\meinekarte"; }

// Ausschnitte fuer die Bilder
ImRect editorRahmen() {
    ImGuiWindow* w = karteFenster();
    const ImGuiWindow* haupt = ImGui::FindWindowByName("###main");
    if (w == nullptr) { return ImRect{}; }
    const float oben = haupt != nullptr ? haupt->Pos.y : w->Pos.y;
    const float links = haupt != nullptr ? haupt->Pos.x : w->Pos.x;
    return ImRect{links, oben, w->Rect().Max.x, w->Rect().Max.y};
}
ImRect menueRahmen() {
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
}
// die 2D-Ansicht samt offenem Kontextmenue
ImRect xyMitMenue() {
    const ke::Editor::RahmenImBild* x = stufe3::rahmen("XY");
    if (x == nullptr) { return ImRect{}; }
    ImRect r{x->x0, x->y0, x->x1, x->y1};
    const ImGuiContext& g = *GImGui;
    for (int i = 0; i < g.OpenPopupStack.Size; ++i) {
        if (const ImGuiWindow* w = g.OpenPopupStack[i].Window) { r.Add(w->Rect()); }
    }
    for (const char* n : {"###kartelicht"}) {
        const ImRect d = fensterRahmen(n);
        if (d.GetWidth() > 0.0F) { r.Add(d); }
    }
    return r;
}
ImRect kameraRahmen() { return hb::fensterRect("##feldKamera"); }

std::vector<Schritt> tutorial() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto wahl = std::make_shared<Wahl>();
    add(tu("Tutorial: Spielordner, Karten-Editor", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        ke::setzeNrcBegriffe(false);
    }));
    add(pause(0.2));
    add(tu("Tutorial: Karten-Editor als Arbeitsbereich", [] {
        if (g_app->leftMode != 3) { g_app->leftMode = 3; }
    }));
    add(pause(0.3));
    add(warteBis("Tutorial: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(stufe2::grundzustand("Tutorial: Grundzustand (Regular, Raster 8, Werkzeug Resize)"));
    add(tu("Tutorial: bhed_q3map2 bereit, Build-Menue wie vorgegeben, alte Tutorial-Dateien weg", [] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();
        ke::Editor& e = ke::editor();
        e.bau = ke::Editor::BauEinstellungen{};
        e.bauProjektFuer = -2;
        e.bauGeaendert = false;
        ke::bauMenueSicherstellen();
        e.spielen.startAnSpawn = false;
        e.echtLicht.an = false;
        // wie nach einem frischen Start: die Arbeitszone (Hoehe eines neu gezogenen Brushes,
        // Select_getWorkZone) wie beim Programmstart - im vollen Lauf lag sie vom vorigen Teil auf 0..64
        e.workzone = karte::Kasten{karte::Vektor3f(0, 0, 0), karte::Vektor3f(64, 64, 64)};
        // ebenso die Frage nach der Lichtstaerke (ein frueherer Teil hakt "Don't show ... until restart" an)
        e.ent.lichtNichtZeigen = false;
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(stufe3::laufOrdner("tutorial")), ec);
        std::filesystem::create_directories(std::filesystem::path(stamm()).parent_path(), ec);
    }));
    // 1. Spiel und Ordner: Edit > Preferences, Seiten Game und Paths
    add(tu("Tutorial 1: Edit > Preferences", [] { ke::editor().einstellungenOffen = true; }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KePgGame)) + "##seite1", "###karteeinstellungen"));
    add(pause(0.3));
    add(hb::fotoTeil("h_tut_karte_spiel", [] { return fensterRahmen("###karteeinstellungen"); }));
    add(klick(std::string(ke::trK(Str::KePgPaths)) + "##seite3", "###karteeinstellungen"));
    add(pause(0.3));
    add(pruefe("Tutorial 1: Preferences offen, Seite Paths; das Spiel ist geladen (Shader im Inventar)", [] {
        const ke::Editor& e = ke::editor();
        return e.einstellungenOffen && e.karteSpiel && !e.karteSpiel->inventar.shader().empty();
    }));
    add(hb::fotoTeil("h_tut_karte_pfade", [] { return fensterRahmen("###karteeinstellungen"); }));
    add(tu("Tutorial 1: Preferences zu", [] { karteDialogeZu(); }));
    add(pause(0.2));
    // 2. File > New Map
    add(klick(ke::trK(Str::KeLFile), "###main"));
    add(pause(0.3));
    add(hb::fotoTeil("h_tut_karte_neu", menueRahmen));
    add(tu("Tutorial 2: File > New Map", [] {
        karteDialogeZu();
        (void)ke::ausfuehren("NewMap");
    }));
    add(pause(0.3));
    add(pruefe("Tutorial 2: neue Karte - unnamed.map, hoechstens worldspawn, keine Pinsel", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Tutorial - neue Karte " + e.name + ", Entities " + std::to_string(e.karte.entitaeten().size()) + ", Pinsel " +
                     std::to_string(pinselZahl()));
        return e.name == "unnamed.map" && e.karte.entitaeten().size() <= 1 && pinselZahl() == 0;
    }));
    add(tu("Tutorial 2: 2D-Ansicht herauszoomen (Mausrad), Mitte 0 0", [] {
        ke::Editor& e = ke::editor();
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        e.xy[0].ansicht.setzeMassstab(0.6F);
        e.kamera.setzeUrsprung(karte::Vektor3f(-560, -560, 330));
        e.kamera.setzeWinkel(karte::Vektor3f(-25, 45, 0));
        ++e.sichtStand;
    }));
    add(pause(0.3));
    // 3. Ein Pinsel: in der Draufsicht ziehen
    add(klickAn("Tutorial 3: in die 2D-Ansicht (Fokus)", stufe3::feldMitte("XY"), 2));
    add(ziehen2d("Tutorial 3: M1 ziehen von -256 -256 nach 256 256", ImGuiMouseButton_Left, 0, karte::Vektor3f(-256, -256, 0),
                 karte::Vektor3f(256, 256, 0)));
    add(pause(0.2));
    add(pruefe("Tutorial 3: ein Pinsel 512 x 512, hoch wie die Arbeitszone (-64..64), gewaehlt", [] {
        const ke::Editor& e = ke::editor();
        const karte::Kasten b = gesamtGrenzen();
        diag::detail("Knopftest: Tutorial - Pinsel " + std::to_string(pinselZahl()) + ", z " + std::to_string(b.origin[2] - b.extents[2]) + ".." +
                     std::to_string(b.origin[2] + b.extents[2]));
        return pinselZahl() == 1 && kastenIst(b, karte::Vektor3f(-256, -256, -64), karte::Vektor3f(256, 256, 64)) && e.karte.auswahl.anzahl() == 1;
    }));
    add(hb::fotoTeil("h_tut_karte_brush", editorRahmen));
    // 4. Hoehe: Vorderansicht (Num 1), die Oberkante nach oben ziehen
    add(tasteKombi(ImGuiKey_Keypad1, "Num 1 (ViewFront)"));
    add(pause(0.2));
    add(pruefe("Tutorial 4: Num 1 - die 2D-Ansicht zeigt XZ", [] { return ke::editor().xy[0].ansicht.typ == karte::Sicht::XZ; }));
    add(tu("Tutorial 4: Mitte auf den Raum", [] {
        ke::Editor& e = ke::editor();
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 64));
        ++e.sichtStand;
    }));
    add(pause(0.2));
    add(ziehen2d("Tutorial 4: ueber der Oberkante M1 ziehen von z 80 nach z 208 (Resize)", ImGuiMouseButton_Left, 0, karte::Vektor3f(0, 0, 80),
                 karte::Vektor3f(0, 0, 208)));
    add(pause(0.2));
    add(pruefe("Tutorial 4: der Pinsel ist jetzt 256 hoch (-64..192)", [] {
        const karte::Kasten b = gesamtGrenzen();
        diag::detail("Knopftest: Tutorial - Hoehe z " + std::to_string(b.origin[2] - b.extents[2]) + ".." + std::to_string(b.origin[2] + b.extents[2]));
        return pinselZahl() == 1 && kastenIst(b, karte::Vektor3f(-256, -256, -64), karte::Vektor3f(256, 256, 192));
    }));
    add(hb::fotoTeil("h_tut_karte_hoehe", editorRahmen));
    // 5. Textur: den Ordner im Texturbrowser, Klick auf die Wandtextur
    add(tu("Tutorial 5: Texturordner zeigen (Doppelklick im Baum)", [wahl] {
        if (!texturenWaehlen(*wahl)) {
            meldeFehler("Tutorial: keine passenden Texturen im Spiel");
            return;
        }
        diag::detail("Knopftest: Tutorial - Ordner " + wahl->ordner + ", Wand " + wahl->wand + ", Boden " + wahl->boden);
        ke::texturVerzeichnisZeigen(wahl->ordner);
    }));
    add(warteBis("Tutorial 5: Texturen geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    add(pause(0.3));
    add(klickAn("Tutorial 5: M1 auf die Wandtextur im Texturbrowser", [wahl] {
        ImVec2 p{0, 0};
        (void)ke::texturMitte(wahl->wand, p);
        return p;
    }));
    add(pause(0.2));
    add(pruefe("Tutorial 5: alle 6 Seiten des Pinsels tragen die Wandtextur", [wahl] { return !wahl->wand.empty() && flaechenMit(wahl->wand) == 6; }));
    add(hb::fotoTeil("h_tut_karte_textur", editorRahmen));
    // 6. Hohl machen: CSG Tool, Caulk aus, Hollow "pull"
    add(klickAn("Tutorial 6: Knopf CSG Tool", stufe2::elementOrt("##knopfCSGTool")));
    add(pause(0.3));
    add(tu("Tutorial 6: Werte wie vorgegeben (16, -faces, Innen weg)", [] {
        ke::Editor::CsgWerkzeug& w = ke::editor().csgWerkzeug;
        w.dicke = 16.0;
        w.ausschluss = 0;
        w.innenWeg = true;
        w.caulk = true;
    }));
    add(klickAn("Tutorial 6: Caulk aus (das macht gleich AutoCaulk)", stufe2::elementOrt("##csgcaulk")));
    add(pause(0.2));
    add(hb::fotoTeil("h_tut_karte_csg", [] { return hb::vereint(fensterRahmen("###kartecsgtool"), editorRahmen()); }));
    add(klickAn("Tutorial 6: Hollow::pull faces", stufe2::elementOrt("##csgpull")));
    add(pause(0.2));
    add(pruefe("Tutorial 6: sechs Platten 16 dick aussen um den Innenraum (-272..272, -80..208), alle mit der Wandtextur", [wahl] {
        const karte::Kasten b = gesamtGrenzen();
        diag::detail("Knopftest: Tutorial - Hollow " + std::to_string(pinselZahl()) + " Pinsel, Wand-Seiten " + std::to_string(flaechenMit(wahl->wand)));
        return pinselZahl() == 6 && kastenIst(b, karte::Vektor3f(-272, -272, -80), karte::Vektor3f(272, 272, 208)) && flaechenMit(wahl->wand) == 36 &&
               !ke::editor().csgWerkzeug.caulk;
    }));
    add(tu("Tutorial 6: CSG Tool zu, Caulk wieder an (Vorgabe)", [] {
        ke::Editor::CsgWerkzeug& w = ke::editor().csgWerkzeug;
        w.offen = false;
        w.caulk = true;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(hb::fotoTeil("h_tut_karte_hohl", editorRahmen));
    // 7. Boden: in der Vorderansicht mit Umschalt+M1 waehlen, Bodentextur
    add(klick2d("Tutorial 7: Umschalt+M1 auf den Boden (z -72)", ImGuiMouseButton_Left, ImGuiMod_Shift, karte::Vektor3f(0, 0, -72)));
    add(pause(0.2));
    add(pruefe("Tutorial 7: nur der Boden ist gewaehlt (-256..256, z -80..-64)", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Tutorial - Boden gewaehlt:" + auswahlText());
        if (e.karte.auswahl.anzahl() != 1) { return false; }
        const karte::Kasten b = karte::auswahlGrenzenPivot(e.karte);
        return kastenIst(b, karte::Vektor3f(-256, -256, -80), karte::Vektor3f(256, 256, -64));
    }));
    add(klickAn("Tutorial 7: M1 auf die Bodentextur", [wahl] {
        ImVec2 p{0, 0};
        (void)ke::texturMitte(wahl->boden, p);
        return p;
    }));
    add(pause(0.2));
    add(pruefe("Tutorial 7: der Boden traegt die Bodentextur (6 Seiten)", [wahl] { return wahl->boden != wahl->wand && flaechenMit(wahl->boden) == 6; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    // 8. Licht: M2 in der Vorderansicht, light, Staerke 300
    add(klickAn("Tutorial 8: M2 in die Vorderansicht bei x 0, z 96", [] { return bild2d(karte::Vektor3f(0, 0, 96)); }, 1));
    add(pause(0.3));
    add(hb::fotoTeil("h_tut_karte_entities", xyMitMenue));
    add(klick("light"));
    add(pause(0.3));
    add(pruefe("Tutorial 8: Set Light intensity offen", [] { return ke::editor().ent.lichtOffen; }));
    add(hb::fotoTeil("h_tut_karte_licht", xyMitMenue));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##lichtok"));
    add(pause(0.2));
    add(pruefe("Tutorial 8: ein light im Raum (0 y 96), mit Staerke", [] {
        const auto l = entitaet("light");
        if (!l) { return false; }
        karte::Vektor3f o{0, 0, 0};
        std::sscanf(l->wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
        diag::detail("Knopftest: Tutorial - light bei " + l->wert("origin") + ", " + l->wert("light"));
        return std::fabs(o[0]) < 0.5F && std::fabs(o[2] - 96.0F) < 0.5F && std::fabs(o[1]) < 256.0F && !l->wert("light").empty();
    }));
    // 9. info_player_start: M2, info > info_player_start
    add(taste(ImGuiKey_Escape, "Esc"));
    add(klickAn("Tutorial 9: M2 in die Vorderansicht bei x -160, z -40", [] { return bild2d(karte::Vektor3f(-160, 0, -40)); }, 1));
    add(pause(0.3));
    add(klick("info"));
    add(pause(0.3));
    add(hb::fotoTeil("h_tut_karte_spawn", xyMitMenue));
    add(klick("info_player_start"));
    add(pause(0.3));
    add(pruefe("Tutorial 9: info_player_start im Raum auf dem Boden (-160 y -40)", [] {
        const auto p = entitaet("info_player_start");
        if (!p) { return false; }
        karte::Vektor3f o{0, 0, 0};
        std::sscanf(p->wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
        diag::detail("Knopftest: Tutorial - info_player_start bei " + p->wert("origin"));
        return std::fabs(o[0] + 160.0F) < 0.5F && std::fabs(o[2] + 40.0F) < 0.5F && std::fabs(o[1]) < 240.0F;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    // 10. Speichern (AutoCaulk braucht eine gespeicherte Karte) (File > Save as: der Windows-Dateidialog - im Test die Funktion dahinter)
    add(tu("Tutorial 10: File > Save as... meinekarte.map", [] { (void)ke::karteSpeichernUnter(kartenPfad()); }));
    add(pruefe("Tutorial 10: gespeichert", [] { return istDatei(kartenPfad()) && ke::editor().name == kartenPfad(); }));
    add(pause(0.2));
    add(hb::fotoTeil("h_tut_karte_gespeichert", editorRahmen));
    // 11. Zurueck in die Draufsicht, die Kamera in den Raum, alles waehlen, AutoCaulk (F4)
    add(tasteKombi(ImGuiKey_Keypad7, "Num 7 (ViewTop)"));
    add(pause(0.2));
    add(pruefe("Tutorial 11: Num 7 - wieder XY", [] { return ke::editor().xy[0].ansicht.typ == karte::Sicht::XY; }));
    add(tu("Tutorial 11: Kamera in den Raum fliegen", [] {
        ke::Editor& e = ke::editor();
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        e.xy[0].ansicht.setzeMassstab(0.4F);
        e.kamera.setzeUrsprung(karte::Vektor3f(-200, -200, 120));
        e.kamera.setzeWinkel(karte::Vektor3f(-20, 45, 0));
        ++e.sichtStand;
    }));
    add(pause(0.2));
    add(ziehen2d("Tutorial 11: Umschalt+M2 ein Rechteck um den ganzen Raum ziehen", ImGuiMouseButton_Right, ImGuiMod_Shift, karte::Vektor3f(-320, -320, 0),
                 karte::Vektor3f(320, 320, 0)));
    add(pause(0.2));
    add(pruefe("Tutorial 11: die sechs Pinsel und die zwei Entities gewaehlt", [] {
        diag::detail("Knopftest: Tutorial - gewaehlt " + std::to_string(ke::editor().karte.auswahl.anzahl()) + ":" + auswahlText());
        return ke::editor().karte.auswahl.anzahl() == 8;
    }));
    add(tu("Tutorial 11: Konsole leeren", [] { ke::konsoleLeeren(); }));
    add(klickAn("Tutorial 11: in die 2D-Ansicht (Fokus, M3)", stufe3::feldMitte("XY"), 2));
    add(tasteKombi(ImGuiKey_F4, "F4 (AutoCaulk Selected)"));
    add(warteBis("Tutorial 11: AutoCaulk fertig", [] { return ke::editor().karte.undo.undoName().rfind("AutoCaulk ", 0) == 0; }, 3000));
    add(pruefe("Tutorial 11: AutoCaulk - die Aussenseiten caulk (6 x 5 = 30), innen bleiben 5 Wandseiten und 1 Bodenseite", [wahl] {
        const int caulk = flaechenMit("textures/system/caulk") + flaechenMit("textures/common/caulk");
        diag::detail("Knopftest: Tutorial - Konsole: " + konsoleText().substr(0, 600));
        diag::detail("Knopftest: Tutorial - " + ke::editor().karte.undo.undoName() + ", caulk " + std::to_string(caulk) + ", Wand " +
                     std::to_string(flaechenMit(wahl->wand)) + ", Boden " + std::to_string(flaechenMit(wahl->boden)));
        return caulk == 30 && flaechenMit(wahl->wand) == 5 && flaechenMit(wahl->boden) == 1;
    }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(pause(0.3));
    add(hb::fotoTeil("h_tut_karte_caulk", editorRahmen));
    // 12. Optimizer
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeOptMenue)) + "##bauoptimierer"));
    add(warteBis("Tutorial 12: Optimierer fertig", [] { return ke::optimiererOffen() && !ke::optimiererLaeuft() && ke::optimiererBlick().hatErgebnis; }, 6000));
    add(pause(0.3));
    add(pruefe("Tutorial 12: Optimizer - keine Grenze ueberschritten", [] {
        const ke::OptimiererBlick b = ke::optimiererBlick();
        diag::detail("Knopftest: Tutorial - Optimizer ueber " + std::to_string(b.grenzenUeber) + ", knapp " + std::to_string(b.grenzenKnapp) + ", Befunde " +
                     std::to_string(b.befunde));
        return b.hatErgebnis && b.grenzenUeber == 0;
    }));
    add(hb::fotoTeil("h_tut_karte_optimierer", [] { return fensterRahmen("###karteoptimierer"); }));
    add(tu("Tutorial 12: Optimizer zu", [] { ke::optimiererSchliessen(); }));
    // 13. Bauen: Build > [Final] Fast, das Fortschrittsfenster
    add(tu("Tutorial 13: Konsole leeren", [] { ke::konsoleLeeren(); }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.3));
    add(hb::fotoTeil("h_tut_karte_buildmenue", menueRahmen));
    add(tu("Tutorial 13: Build > [Final] Fast", [] {
        karteDialogeZu();
        const auto& v = ke::editor().bauProjekt.eintraege;
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (v[i].name == "[Final] Fast") {
                ke::bauStarten(i);
                return;
            }
        }
        meldeFehler("Tutorial: Eintrag [Final] Fast fehlt");
    }));
    add(warteBis("Tutorial 13: Fortschrittsfenster", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return !st.laeuft || st.schritt >= 1;
    }, 3000));
    add(hb::fotoTeil("h_tut_karte_fortschritt", [] { return fensterRahmen("###kartebaufortschritt"); }));
    add(stufe8::warteSek("Tutorial 13: Bau fertig", [] { return !ke::bauLaeuft(); }, 300));
    add(pause(0.3));
    add(pruefe("Tutorial 13: gebaut ohne Leck - meinekarte.bsp da, kein LEAKED, kein Fehler in der Konsole", [] {
        const std::string k = konsoleText();
        diag::detail("Knopftest: Tutorial - Bau, Konsole " + std::to_string(k.size()) + " Zeichen, Leck " +
                     std::to_string(k.find("LEAKED") != std::string::npos ? 1 : 0));
        return istDatei(stamm() + ".bsp") && k.find("LEAKED") == std::string::npos && k.find("leaked") == std::string::npos &&
               !stufe3::konsoleArtMit(ke::KonsolenZeile::Art::Fehler, "");
    }));
    add(hb::fotoTeil("h_tut_karte_gebaut", editorRahmen));
    // 14. Im Editor spielen: an info_player_start, vorwaerts laufen
    add(klick(ke::trK(Str::KeLView), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeSpSpawn)) + "###spielenspawn"));
    add(pause(0.2));
    add(pruefe("Tutorial 14: View > Play: start at info_player_start ist an", [] { return ke::editor().spielen.startAnSpawn; }));
    add(klickAn("Tutorial 14: in die Kamera (Fokus)", stufe3::feldMitte("Kamera"), 2));
    add(tasteKombi(ImGuiKey_F6, "F6 (Play in editor)"));
    add(pause(0.3));
    auto start = std::make_shared<karte::Vektor3f>();
    add(pruefe("Tutorial 14: F6 - Spielen aktiv, am info_player_start", [start] {
        const ke::SpielenZustand& z = ke::editor().spielen;
        *start = karte::Vektor3f(z.s.ort[0], z.s.ort[1], z.s.ort[2]);
        diag::detail("Knopftest: Tutorial - Spielen ab " + std::to_string(z.s.ort[0]) + " " + std::to_string(z.s.ort[1]) + " " + std::to_string(z.s.ort[2]));
        return z.aktiv && z.amSpawn && std::fabs(z.s.ort[0] + 160.0F) < 1.0F;
    }));
    add(tu("Tutorial 14: eingespielt - 1 s fallen (auf den Boden)", [start] {
        ke::SpielenZustand& z = ke::editor().spielen;
        z.testEingabe = true;
        stufe8b::spielLauf(62, 0, 0, 0);
        *start = karte::Vektor3f(z.s.ort[0], z.s.ort[1], z.s.ort[2]);
    }));
    add(pause(0.4));
    add(hb::fotoTeil("h_tut_karte_spielen", kameraRahmen));
    add(tu("Tutorial 14: eingespielt - 1,5 s vorwaerts nach +x", [] { stufe8b::spielLauf(94, 127, 0, 0); }));
    add(pause(0.3));
    add(pruefe("Tutorial 14: der Spieler steht auf dem Boden (z -64 + 24, auf 0,5) und ist ueber 200 Einheiten gelaufen, im Raum", [start] {
        const ke::SpielenZustand& z = ke::editor().spielen;
        const float weg = z.s.ort[0] - (*start)[0];
        diag::detail("Knopftest: Tutorial - gelaufen " + std::to_string(weg) + " bis " + std::to_string(z.s.ort[0]) + " " + std::to_string(z.s.ort[1]) +
                     " " + std::to_string(z.s.ort[2]) + ", am Boden " + std::to_string(z.s.amBoden ? 1 : 0));
        return z.aktiv && z.s.amBoden && std::fabs((*start)[2] - (-64.0F + 24.0F)) < 0.5F && std::fabs(z.s.ort[2] - (-64.0F + 24.0F)) < 0.5F && weg > 200.0F &&
               z.s.ort[0] < 256.0F;
    }));
    add(tasteKombi(ImGuiKey_Escape, "Esc (Spielen beenden)"));
    add(pause(0.2));
    add(pruefe("Tutorial 14: Esc beendet das Spielen", [] { return !ke::editor().spielen.aktiv; }));
    add(tu("Tutorial 14: Start an info_player_start wieder aus (Vorgabe), eingespielte Eingabe aus", [] {
        ke::editor().spielen.startAnSpawn = false;
        ke::editor().spielen.testEingabe = false;
    }));
    // 15. Build > Pack map as .pk3 (eigene Funktion): das Archiv neben der .bsp
    add(tu("Tutorial 15: Konsole leeren", [] { ke::konsoleLeeren(); }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.3));
    add(hb::fotoTeil("h_tut_karte_pk3menue", menueRahmen));
    add(klick(std::string(ke::trK(Str::KePkMenue)) + "##baupk3"));
    add(pause(0.3));
    add(pruefe("Tutorial 15: meinekarte.pk3 neben der .bsp, darin maps/meinekarte.bsp gleich der .bsp, Meldung mit devmap", [] {
        Pk3 a;
        if (!readPk3Directory(stamm() + ".pk3", a)) { return false; }
        const Pk3Entry* e = a.find("maps/meinekarte.bsp");
        std::string inhalt;
        if (e == nullptr || !readPk3File(a, *e, inhalt)) { return false; }
        const std::string bsp = dateiInhalt(stamm() + ".bsp");
        diag::detail("Knopftest: Tutorial - pk3 " + std::to_string(a.entries.size()) + " Eintraege, bsp " + std::to_string(bsp.size()) + " Bytes");
        return !bsp.empty() && inhalt == bsp && konsoleText().find("devmap meinekarte") != std::string::npos;
    }));
    add(hb::fotoTeil("h_tut_karte_pk3", editorRahmen));
    add(tu("Tutorial: Ende - Dialoge zu", [] {
        karteDialogeZu();
        ke::konsoleLeeren();
    }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> schritte() { return tutorial(); }

}  // namespace stufe8c
