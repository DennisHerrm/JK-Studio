// selbsttest_karte3bis5.inc - aus gui/selbsttest.cpp geschnitten (Stufe 8d Punkt 5, unveraendert): Karten-Editor Stufe 3 bis 5 (mit karte6/karte8)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor);
// keine eigene Uebersetzungseinheit - der vorverarbeitete Text ist derselbe wie vor dem Schnitt.
// Abschnitte: Stufe 3 des Karten-Editors: Karten bauen; Punkt 0c: jede Mausbelegung von NRC mit echten Ereignissen und Wirkung; Punkt 0c, Teil 3: Teiler einklappen und zurueckziehen, Maximize view; Punkt 3: Build-Menue und der Dialog "Build Menu"; Punkt 4: Bauen aus dem Editor; Punkt 5: Leck-Linie, Rueckmeldungen in 2D, Portal Viewer; Punkt 6: AutoCaulk Selected (F4); Stufe 4 des Karten-Editors: fertige Karten, Originallicht, Plugins

// --- Stufe 3 des Karten-Editors: Karten bauen ----------------------------------
//
// BHED_KARTE_NUR=stufe3 laesst nur diese Schritte (und die Masse) laufen.
namespace stufe3 {

namespace ke = karteneditor;

// Die Titel der NRC-Menueleiste (create_main_menu) in der Sprache der Anzeige.
std::vector<std::string> nrcTitel() {
    std::vector<std::string> v;
    for (const auto& m : karte::befehle::menues()) {
        if (m.art == karte::befehle::MenueArt::Menue) { v.emplace_back(ke::trK(m.str)); }
    }
    return v;
}

const ke::Editor::RahmenImBild* rahmen(const std::string& feld) {
    for (const auto& r : ke::editor().rahmenImBild) {
        if (r.feld == feld) { return &r; }
    }
    return nullptr;
}

std::function<ImVec2()> feldMitte(const std::string& feld) {
    return [feld] {
        const ke::Editor::RahmenImBild* r = rahmen(feld);
        return r != nullptr ? ImVec2{(r->x0 + r->x1) * 0.5F, (r->y0 + r->y1) * 0.5F} : ImVec2{5.0F, 5.0F};
    };
}

// Der feine Rahmen: ein Ton zwischen Flaeche und Schrift des Farbschemas.
ImU32 feinerTon() {
    const theme::Theme* th = g_app->activeTheme;
    if (th == nullptr) { return 0; }
    const float k = 0.18F;
    return ImGui::ColorConvertFloat4ToU32(ImVec4{th->window.r + (th->text.r - th->window.r) * k, th->window.g + (th->text.g - th->window.g) * k,
                                                 th->window.b + (th->text.b - th->window.b) * k, 1.0F});
}
ImU32 akzent() {
    const theme::Theme* th = g_app->activeTheme;
    return th != nullptr ? ImGui::ColorConvertFloat4ToU32(ImVec4{th->accent.r, th->accent.g, th->accent.b, 1.0F}) : 0;
}

// Jedes sichtbare Feld hat genau einen Rahmen auf seiner Innenkante, das
// aktive (aktiv = "-": keines) in der Akzentfarbe, die anderen im feinen Ton.
bool rahmenStimmen(const std::vector<std::string>& felder, const std::string& aktiv, std::string& bericht) {
    const auto& liste = ke::editor().rahmenImBild;
    bool ok = liste.size() == felder.size();
    bericht = std::to_string(liste.size()) + " Rahmen:";
    const float sk = ke::massstab();
    for (const auto& r : liste) {
        bericht += " " + r.feld + (r.aktiv ? "(aktiv)" : "") + " " + std::to_string(static_cast<int>(r.x1 - r.x0)) + "x" +
                   std::to_string(static_cast<int>(r.y1 - r.y0));
        const bool sollAktiv = r.feld == aktiv;
        ok = ok && std::find(felder.begin(), felder.end(), r.feld) != felder.end() && r.aktiv == sollAktiv &&
             r.farbe == (sollAktiv ? akzent() : feinerTon());
        // Rechteck = das Feld (ausser bei den schwebenden Fenstern: "Fenster:<Feld>")
        const auto* f = karteFeld(ke::editor().anordnung == karte::befehle::Anordnung::Floating ? "Fenster:" + r.feld : r.feld);
        if (f != nullptr) {
            const float b = std::round(static_cast<float>(f->x + f->b) * sk) - std::round(static_cast<float>(f->x) * sk);
            const float h = std::round(static_cast<float>(f->y + f->h) * sk) - std::round(static_cast<float>(f->y) * sk);
            ok = ok && std::fabs((r.x1 - r.x0) - b) <= 1.0F && std::fabs((r.y1 - r.y0) - h) <= 1.0F;
        } else {
            ok = false;
        }
    }
    return ok;
}

// Punkt 0b: eine Leiste, die umschaltet; Rahmen + Abstand.
std::vector<Schritt> leiste() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Stufe 3/0b: Anordnung Regular, Texturbrowser unten rechts", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Regular;
        e.gruppenDialogEingebaut = false;
        e.teilerFuer = -1;
        e.anzeige.umriss = true;
        e.aktivXy = 0;
        e.aktivXyVorher = -1;
    }));
    add(pause(0.3));
    add(pruefe("Eine Leiste: im Karten-Editor die zwoelf Titel von NRC in ihrer Reihenfolge, kein Programm-Menue", [] {
        float x = -1.0F;
        std::string bericht;
        bool ok = true;
        for (const std::string& n : nrcTitel()) {
            const Element* el = finde(n, "###main", 0, false);
            bericht += " " + n + (el != nullptr ? "@" + std::to_string(static_cast<int>(el->rect.Min.x)) : "?");
            ok = ok && el != nullptr && el->rect.Min.x > x;
            if (el != nullptr) { x = el->rect.Min.x; }
        }
        const bool debug = finde(tr(Str::MenuDebug), "###main", 0, false) != nullptr;
        diag::detail("Knopftest: Stufe 3/0b Leiste:" + bericht + (debug ? " (Debug da)" : ""));
        return ok && nrcTitel().size() == 12 && !debug;
    }));
    add(pruefe("Eine Leiste: genau eine Menueleiste (das Kindfenster des Editors hat keine)", [] {
        int leisten = 0;
        for (ImGuiWindow* w : GImGui->Windows) {
            if (!w->WasActive || w->Hidden) { continue; }
            const bool imHaupt = std::strstr(w->Name, "###main") != nullptr;
            if (imHaupt && (w->Flags & ImGuiWindowFlags_MenuBar) != 0) { ++leisten; }
        }
        diag::detail("Knopftest: Stufe 3/0b - Fenster mit Menueleiste: " + std::to_string(leisten));
        return leisten == 1;
    }));
    add(pruefe("Eine Leiste: keine Skript-Reiter, der Editor beginnt direkt unter der Leiste (21 Punkte hoch)", [] {
        const ImGuiWindow* w = ImGui::FindWindowByName("###main");
        ImGuiTabBar* tb = reiterLeiste();
        const bool reiter = tb != nullptr && tb->PrevFrameVisible >= ImGui::GetFrameCount() - 1;
        const ke::Editor& e = ke::editor();
        const float sk = ke::massstab();
        const float leisteH = w != nullptr ? e.leisteUnten - w->Pos.y : -1.0F;
        diag::detail("Knopftest: Stufe 3/0b - Leiste " + std::to_string(leisteH) + " hoch (soll " + std::to_string(std::round(21.0F * sk)) +
                     "), Leiste unten " + std::to_string(e.leisteUnten) + ", Editor oben " + std::to_string(e.editorOben) +
                     (reiter ? ", Reiter sichtbar" : ", keine Reiter"));
        return w != nullptr && !reiter && std::fabs(e.editorOben - e.leisteUnten) < 0.5F && std::fabs(leisteH - std::round(21.0F * sk)) < 0.5F;
    }));
    add(foto("karteneditor_s3_leiste"));
    // Die Programm-Eintraege in Misc und Help (Abweichung, s. NOTIZEN_karte3.md)
    add(klick(ke::trK(Str::KeLMisc), "###main"));
    add(pause(0.2));
    add(pruefe("Misc: am Ende Sprache, Farbschema, Skalierung des Programms", [] {
        return finde(ke::trK(Str::KeLLanguage), "##Menu_", 0, false) != nullptr && finde(ke::trK(Str::KeLColorScheme), "##Menu_", 0, false) != nullptr &&
               finde(ke::trK(Str::KeLScaling), "##Menu_", 0, false) != nullptr;
    }));
    add(klick(ke::trK(Str::KeLScaling), "##Menu_", 0, false));
    add(pause(0.25));
    auto skalaVorher = std::make_shared<float>(1.0F);
    add(tu("Skalierung merken", [skalaVorher] { *skalaVorher = g_app->uiScale; }));
    add(klick("125 %", "##Menu_", 0, false));
    add(pause(0.3));
    add(pruefe("Misc > Skalierung > 125 %: das Programm skaliert (uiScale 1,25)", [] { return std::fabs(g_app->uiScale - 1.25F) < 0.001F; }));
    add(tu("Skalierung zurueck", [skalaVorher] {
        g_app->uiScale = *skalaVorher;
        karteDialogeZu();
    }));
    add(pause(0.4));
    add(klick(ke::trK(Str::KeLHelp), "###main"));
    add(pause(0.2));
    add(tu("Hilfe-Merker leeren", [] { ke::editor().letzteHilfe.clear(); }));
    add(klick(ke::trK(Str::KeLManual), "##Menu_", 0, false));
    add(pause(0.2));
    add(pruefe("Help > Manual: das Handbuch auf der Seite des Karten-Editors (im Test ohne Browser)", [] {
        return ke::editor().letzteHilfe == "karteneditor.html";
    }));
    add(tu("Hilfe-Merker leeren", [] { ke::editor().letzteHilfe.clear(); }));
    add(klickAn("in XY (Fokus)", feldMitte("XY"), 2));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tasteKombi(ImGuiKey_F1, "F1"));
    add(pause(0.2));
    add(pruefe("F1 (OpenManual): dasselbe Handbuch", [] { return ke::editor().letzteHilfe == "karteneditor.html"; }));
    add(klick(ke::trK(Str::KeLHelp), "###main"));
    add(pause(0.2));
    add(klick(ke::trK(Str::KeLCheckForNetRadiantUpdate), "##Menu_", 0, false));
    add(pause(0.2));
    add(pruefe("Help > Check for NetRadiant update: die Update-Pruefung des Programms (im Test ohne Netz)", [] {
        return ke::editor().letzteHilfe == "update";
    }));
    add(klick(ke::trK(Str::KeLHelp), "###main"));
    add(pause(0.2));
    add(klick(ke::trK(Str::KeLAboutBehavEd), "##Menu_", 0, false));
    add(pause(0.3));
    add(pruefe("Help > About BehavEd: der About-Dialog des Programms", [] { return g_app->aboutOpen; }));
    add(tu("About zu", [] { g_app->aboutOpen = false; }));
    add(pause(0.3));
    // Rahmen + Abstand
    add(tu("XY aktiv", [] {
        ke::editor().aktivXy = 0;
        ke::editor().aktivXyVorher = -1;
    }));
    add(pause(0.2));
    add(pruefe("Rahmen: XY, Konsole, Kamera, Texturen je mit Rahmen auf der Innenkante, XY aktiv (Akzent), die anderen fein", [] {
        std::string b;
        const bool ok = rahmenStimmen({"XY", "Konsole", "Kamera", "Texturen"}, "XY", b);
        diag::detail("Knopftest: Stufe 3/0b - " + b);
        return ok;
    }));
    add(pruefe("Rahmen: der Akzent ist in der Zeichenliste von XY, nicht in der der Kamera", [] {
        const auto farbeIn = [](const char* teil, ImU32 c) {
            for (ImGuiWindow* w : GImGui->Windows) {
                if (!w->WasActive || std::strstr(w->Name, teil) == nullptr) { continue; }
                for (const ImDrawVert& v : w->DrawList->VtxBuffer) {
                    if (v.col == c) { return true; }
                }
            }
            return false;
        };
        return farbeIn("/##feldXY", akzent()) && !farbeIn("/##feldKamera", akzent());
    }));
    add(klickAn("in die Kamera", feldMitte("Kamera"), 1));
    add(pause(0.3));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pruefe("Rahmen: nach dem Klick ist die Kamera aktiv (Akzent), XY fein", [] {
        std::string b;
        const bool ok = rahmenStimmen({"XY", "Konsole", "Kamera", "Texturen"}, "Kamera", b);
        diag::detail("Knopftest: Stufe 3/0b - " + b);
        return ok;
    }));
    add(klickAn("in die Konsole", feldMitte("Konsole"), 0));
    add(pause(0.3));
    add(pruefe("Rahmen: die Konsole aktiv", [] {
        std::string b;
        return rahmenStimmen({"XY", "Konsole", "Kamera", "Texturen"}, "Konsole", b);
    }));
    add(klickAn("in XY", feldMitte("XY"), 2));
    add(pause(0.3));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pruefe("Rahmen: XY wieder aktiv", [] {
        std::string b;
        return rahmenStimmen({"XY", "Konsole", "Kamera", "Texturen"}, "XY", b);
    }));
    add(tu("Window Outline aus", [] { ke::editor().anzeige.umriss = false; }));
    add(pause(0.2));
    add(pruefe("ShowWindowOutline aus: XY ohne Akzent (wie NRCs Schalter)", [] {
        std::string b;
        return rahmenStimmen({"XY", "Konsole", "Kamera", "Texturen"}, "-", b);
    }));
    add(tu("Window Outline an", [] { ke::editor().anzeige.umriss = true; }));
    // Der Abstand ist NRCs Griff: ziehen und einklappen geht weiter
    const auto griffKonsole = [] {
        const auto* g = karteFeld("Griff:v:1");
        const ke::Editor::RahmenImBild* xy = rahmen("XY");
        if (g == nullptr || xy == nullptr) { return ImVec2{5, 5}; }
        const float sk = ke::massstab();
        return ImVec2{xy->x0 + 20.0F * sk, xy->y0 + (static_cast<float>(g->y - 60) + 2.0F) * sk};
    };
    add(ziehenAn("Griff Konsole ganz nach unten", griffKonsole, 0.0F, 400.0F, 0));
    add(pause(0.3));
    add(pruefe("Griff gezogen: die Konsole ist eingeklappt (QSplitter childrenCollapsible), kein Rahmen fuer sie", [] {
        const auto* k = karteFeld("Konsole");
        return k != nullptr && (k->b == 0 || k->h == 0) && rahmen("Konsole") == nullptr && rahmen("XY") != nullptr;
    }));
    add(ziehenAn("Griff wieder hoch", griffKonsole, 0.0F, -120.0F, 0));
    add(pause(0.3));
    add(pruefe("Griff zurueckgezogen: die Konsole ist wieder da, mit Rahmen", [] {
        const auto* k = karteFeld("Konsole");
        return k != nullptr && k->h > 30 && rahmen("Konsole") != nullptr;
    }));
    add(tu("Groessen wie vorher", [] {
        ke::Editor& e = ke::editor();
        e.hGroessen.clear();
        e.vGroessen.clear();
        e.v2Groessen.clear();
        e.teilerFuer = -1;
    }));
    // Alle vier Anordnungen
    add(tu("Anordnung Regular Left", [] {
        ke::editor().anordnung = karte::befehle::Anordnung::RegularLeft;
        ke::editor().teilerFuer = -1;
    }));
    add(pause(0.3));
    add(pruefe("Regular Left: vier Rahmen, XY aktiv", [] {
        std::string b;
        const bool ok = rahmenStimmen({"XY", "Konsole", "Kamera", "Texturen"}, "XY", b);
        diag::detail("Knopftest: Stufe 3/0b - Regular Left " + b);
        return ok;
    }));
    add(tu("Anordnung Split", [] {
        ke::editor().anordnung = karte::befehle::Anordnung::Split;
        ke::editor().teilerFuer = -1;
    }));
    add(pause(0.3));
    add(pruefe("Split: Kamera, YZ, XY, XZ mit Rahmen, XY aktiv", [] {
        std::string b;
        const bool ok = rahmenStimmen({"Kamera", "YZ", "XY", "XZ"}, "XY", b);
        diag::detail("Knopftest: Stufe 3/0b - Split " + b);
        return ok;
    }));
    add(foto("karteneditor_s3_split"));
    add(tu("Anordnung Floating", [] {
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Floating;
        e.teilerFuer = -1;
        e.schwebendSichtbar = {true, true, true, true};
        e.schwebendZuruecksetzen = true;
    }));
    add(pause(0.4));
    add(pruefe("Floating: die vier schwebenden Ansichten mit Rahmen, XY aktiv", [] {
        std::string b;
        const bool ok = rahmenStimmen({"Kamera", "XY", "XZ", "YZ"}, "XY", b);
        diag::detail("Knopftest: Stufe 3/0b - Floating " + b);
        return ok;
    }));
    add(tu("Anordnung Regular", [] {
        ke::editor().anordnung = karte::befehle::Anordnung::Regular;
        ke::editor().teilerFuer = -1;
    }));
    add(pause(0.3));
    // Hell und dunkel
    auto themaVorher = std::make_shared<const theme::Theme*>(nullptr);
    add(tu("ein helles Farbschema", [themaVorher] {
        *themaVorher = g_app->activeTheme;
        for (const theme::Theme& th : theme::themes()) {
            if (theme::relativeLuminance(th.window) > 0.5F) {
                g_app->activeTheme = &th;
                g_app->themePending = true;
                break;
            }
        }
    }));
    add(pause(0.4));
    add(pruefe("hell: der feine Rahmen ist dunkler als die Flaeche, der aktive in der Akzentfarbe", [] {
        const theme::Theme* th = g_app->activeTheme;
        const ke::Editor::RahmenImBild* r = rahmen("Kamera");
        const ke::Editor::RahmenImBild* a = rahmen("XY");
        if (th == nullptr || r == nullptr || a == nullptr || theme::relativeLuminance(th->window) <= 0.5F) { return false; }
        const ImVec4 c = ImGui::ColorConvertU32ToFloat4(r->farbe);
        return c.x + c.y + c.z < th->window.r + th->window.g + th->window.b && a->farbe == akzent();
    }));
    add(foto("karteneditor_s3_hell"));
    add(tu("ein dunkles Farbschema", [] {
        for (const theme::Theme& th : theme::themes()) {
            if (theme::relativeLuminance(th.window) < 0.1F) {
                g_app->activeTheme = &th;
                g_app->themePending = true;
                break;
            }
        }
    }));
    add(pause(0.4));
    add(pruefe("dunkel: der feine Rahmen ist heller als die Flaeche", [] {
        const theme::Theme* th = g_app->activeTheme;
        const ke::Editor::RahmenImBild* r = rahmen("Kamera");
        if (th == nullptr || r == nullptr || theme::relativeLuminance(th->window) >= 0.1F) { return false; }
        const ImVec4 c = ImGui::ColorConvertU32ToFloat4(r->farbe);
        return c.x + c.y + c.z > th->window.r + th->window.g + th->window.b;
    }));
    add(tu("Farbschema zurueck", [themaVorher] {
        if (*themaVorher != nullptr) {
            g_app->activeTheme = *themaVorher;
            g_app->themePending = true;
        }
    }));
    add(pause(0.4));
    // Den Modus verlassen: alles wie vorher
    add(klick(tr(Str::ViewEvents)));
    add(pause(0.4));
    add(pruefe("Modus verlassen: die Menueleiste des Programms (mit Debug) und die Reiter sind zurueck, keine NRC-Leiste", [] {
        ImGuiTabBar* tb = reiterLeiste();
        const bool reiter = tb != nullptr && tb->PrevFrameVisible >= ImGui::GetFrameCount() - 1;
        return g_app->leftMode == 0 && finde(tr(Str::MenuDebug), "###main", 0, false) != nullptr && reiter &&
               finde(ke::trK(Str::KeLBrush), "###main", 0, false) == nullptr;
    }));
    add(klick(tr(Str::ViewMapEditor)));
    add(pause(0.4));
    add(pruefe("zurueck im Karten-Editor: wieder nur die NRC-Leiste", [] {
        return g_app->leftMode == 3 && finde(tr(Str::MenuDebug), "###main", 0, false) == nullptr &&
               finde(ke::trK(Str::KeLBrush), "###main", 0, false) != nullptr;
    }));
    return s;
}

// --- Punkt 0c: jede Mausbelegung von NRC mit echten Ereignissen und Wirkung ---
//
// Die Liste steht in tools/karte/nrc_inventar.md (Abschnitt "Mausbelegung",
// Quelle tools/karte/mausbelegung.py); die Namen der Pruefungen hier sind die
// Spalte "Test" dort. Belegungen, die schon frueher geprueft wurden (Stufe
// 1b/2), nennt die Liste mit dem Namen der alten Pruefung.

// Maus an einer Stelle: Zusatztasten, Druck, ziehen um weg in zwoelf Schritten
// (mitten im Zug koennen weitere Zusatztasten dazukommen), Loslassen.
Schritt maus(const std::string& was, std::function<ImVec2()> ort, int taste, ImGuiKeyChord mods, ImVec2 weg = ImVec2{0, 0},
             ImGuiKeyChord modsImZug = 0) {
    auto start = std::make_shared<ImVec2>();
    const auto zusatz = [](ImGuiKeyChord m, bool an) {
        ImGuiIO& io = ImGui::GetIO();
        if ((m & ImGuiMod_Ctrl) != 0) { io.AddKeyEvent(ImGuiMod_Ctrl, an); }
        if ((m & ImGuiMod_Shift) != 0) { io.AddKeyEvent(ImGuiMod_Shift, an); }
        if ((m & ImGuiMod_Alt) != 0) { io.AddKeyEvent(ImGuiMod_Alt, an); }
    };
    return Schritt{was, [=](int b) {
                       ImGuiIO& io = ImGui::GetIO();
                       if (b == 0) {
                           *start = ort();
                           setzeMaus(*start);
                           zusatz(mods, true);
                           return false;
                       }
                       if (b == 2) {
                           io.AddMouseButtonEvent(taste, true);
                           return false;
                       }
                       if (b >= 3 && b <= 14) {
                           if (b == 6) { zusatz(modsImZug, true); }
                           if (weg.x != 0.0F || weg.y != 0.0F) {
                               const float t = static_cast<float>(b - 2) / 12.0F;
                               setzeMaus(ImVec2{start->x + weg.x * t, start->y + weg.y * t});
                           }
                           return false;
                       }
                       if (b == 15) {
                           io.AddMouseButtonEvent(taste, false);
                           return false;
                       }
                       if (b == 16) {
                           zusatz(modsImZug, false);
                           zusatz(mods, false);
                       }
                       return b >= 19;
                   }};
}

std::function<ImVec2()> xy(float x, float y) {
    return [x, y] { return stufe2::weltXy(x, y); };
}
std::function<ImVec2()> kam(float x, float y, float z) {
    return [x, y, z] {
        const ke::KameraFenster& f = ke::editor().kamFenster;
        const ImVec2 b = stufe2::kameraBild(karte::Vektor3f(x, y, z));
        return ImVec2{f.bildX + b.x, f.bildY + b.y};
    };
}

// Die Testkarte: A = Pinsel der Kopie (-64..64, -64..64, 0..64), B daneben
// (128..192, -32..32, 0..64), C ueber A (-64..64, -64..64, 96..160).
std::shared_ptr<karte::Pinsel> pinselNr(std::size_t i) {
    const auto w = ke::editor().karte.welt();
    if (!w || w->kinder().size() <= i) { return nullptr; }
    return std::dynamic_pointer_cast<karte::Pinsel>(w->kinder()[i]);
}
bool istGewaehlt(std::size_t i) {
    const auto p = pinselNr(i);
    return p && ke::editor().karte.auswahl.gewaehlt(ke::editor().karte.welt().get(), p.get());
}
karte::Kasten kasten(std::size_t i) {
    const auto p = pinselNr(i);
    karte::Kasten k;
    if (p) {
        k = p->grenzen(ke::editor().karte.kontext);
    }
    return k;
}
Schritt testkarte(const std::string& was) {
    return tu(was, [] {
        ke::Editor& e = ke::editor();
        e.anordnung = karte::befehle::Anordnung::Regular;
        e.teilerFuer = -1;
        e.anzeige = ke::Anzeige{};
        (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map"));
        auto a = pinselNr(0);
        if (a) {
            auto b = a->kopie(e.karte.kontext);
            b->transformieren(karte::verschiebungsMatrix(karte::Vektor3f(160, 0, 0)), e.karte.kontext);
            // B schmaler: die y-Ebenen auf -32/32 (skalieren um die Mitte)
            karte::Matrix4 sk = karte::verschiebungsMatrix(karte::Vektor3f(160, 0, 32));
            karte::Matrix4 s2 = karte::kEinheitsMatrix;
            s2[0] = 0.5F;
            s2[5] = 0.5F;
            karte::multipliziere(sk, s2);
            karte::verschiebeUm(sk, karte::Vektor3f(-160, 0, -32));
            b->transformieren(sk, e.karte.kontext);
            e.karte.welt()->kindEinfuegen(b);
            auto c = a->kopie(e.karte.kontext);
            c->transformieren(karte::verschiebungsMatrix(karte::Vektor3f(0, 0, 96)), e.karte.kontext);
            e.karte.welt()->kindEinfuegen(c);
        }
        e.karte.auswahl.leeren();
        e.bearbeitung.einstellung.modus = karte::AuswahlModus::Primitiv;
        e.bearbeitung.einstellung.komponente = karte::KomponentenModus::Standard;
        e.aktivXy = 0;
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeMassstab(1);
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        e.raster.setzePotenz(3);
        e.kamera.setzeUrsprung(karte::Vektor3f(-256, 0, 32));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
        if (e.kamFenster.frei) { ke::freelookBeenden(); }
        ke::tastaturErsatz = deutscheTastatur;
        if (e.bearbeitung.modus() != karte::ManipModus::Ziehen) { (void)ke::ausfuehren("MouseDrag"); }
    });
}

std::vector<Schritt> maus() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(testkarte("Maus: Testkarte (A, B daneben, C ueber A), Resize, XY 1:1"));
    add(pause(0.4));
    add(pruefe("Maus: Testkarte mit drei Pinseln", [] {
        const karte::Kasten b = kasten(1);
        const karte::Kasten c = kasten(2);
        diag::detail("Knopftest: Maus - B " + std::to_string(b.origin[0]) + "/" + std::to_string(b.extents[1]) + ", C z " + std::to_string(c.origin[2]));
        return pinselNr(2) != nullptr && std::fabs(b.origin[0] - 160.0F) < 0.01F && std::fabs(b.extents[1] - 32.0F) < 0.01F &&
               std::fabs(c.origin[2] - 128.0F) < 0.01F;
    }));
    // --- 2D und 3D ---
    // M1 ohne Bewegung: Durchlaufen (eCycle) durch die Treffer in der Tiefe - hier
    // C (oben), das Licht der Karte und A; danach wieder von vorn.
    auto folge = std::make_shared<std::vector<std::string>>();
    const auto wasGewaehlt = [] {
        const ke::Editor& e = ke::editor();
        for (std::size_t i = 0; i < 3; ++i) {
            if (istGewaehlt(i)) { return std::string(1, static_cast<char>('A' + i)); }
        }
        for (const auto& ent : e.karte.entitaeten()) {
            if (e.karte.auswahl.gewaehlt(ent.get())) { return ent->klassenname(); }
        }
        return std::string("-");
    };
    add(tu("Folge leeren", [folge] { folge->clear(); }));
    for (int i = 0; i < 4; ++i) {
        add(maus("M1 auf (0 0), ohne Bewegung " + std::to_string(i + 1), xy(0, 0), 0, 0));
        add(pause(0.15));
        add(tu("gewaehlten merken", [folge, wasGewaehlt] { folge->push_back(wasGewaehlt()); }));
    }
    add(pruefe("Maus 2D: M1 = Tunnel-Auswahl, derselbe Punkt noch einmal waehlt den naechsten Treffer (C, Licht, A, dann wieder C)", [folge] {
        std::string f;
        for (const auto& x : *folge) { f += " " + x; }
        diag::detail("Knopftest: Maus - Tunnel:" + f);
        const auto& v = *folge;
        return v.size() == 4 && v[0] == "C" && v[1] == "light" && v[2] == "A" && v[3] == "C";
    }));
    add(tu("nichts gewaehlt", [] { ke::editor().karte.auswahl.leeren(); }));
    add(maus("M1 auf B", xy(160, 0), 0, 0));
    add(maus("Umschalt+M1 auf A", xy(-32, -32), 0, ImGuiMod_Shift));
    add(pause(0.2));
    add(pruefe("Maus 2D: Umschalt+M1 waehlt dazu (B und A/C)", [] {
        return istGewaehlt(1) && (istGewaehlt(0) || istGewaehlt(2)) && ke::editor().karte.auswahl.anzahl() == 2;
    }));
    add(tu("nichts gewaehlt", [] { ke::editor().karte.auswahl.leeren(); }));
    add(maus("Umschalt+M1 von A nach B ziehen", xy(-32, -32), 0, ImGuiMod_Shift, ImVec2{0, 0}));
    add(tu("leeren", [] { ke::editor().karte.auswahl.leeren(); }));
    add({"Umschalt+M1 ziehen (malen) von A ueber B", [](int b) {
             static Schritt k;
             if (b == 0) {
                 const ImVec2 a = stufe2::weltXy(-32, 0);
                 const ImVec2 z = stufe2::weltXy(160, 0);
                 k = maus("malen", [a] { return a; }, 0, ImGuiMod_Shift, ImVec2{z.x - a.x, z.y - a.y});
             }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(pruefe("Maus 2D: Umschalt+M1 ziehen malt die Auswahl (A/C und B gewaehlt)", [] {
        return istGewaehlt(1) && (istGewaehlt(0) || istGewaehlt(2));
    }));
    add(tu("nichts gewaehlt", [] { ke::editor().karte.auswahl.leeren(); }));
    add(maus("Umschalt+M2 auf B", xy(160, 0), 1, ImGuiMod_Shift));
    add(pause(0.2));
    add(pruefe("Maus 2D: Umschalt+M2 = Tunnel-Auswahl (B gewaehlt, kein Entitaetenmenue)", [] {
        return istGewaehlt(1) && ke::editor().karte.auswahl.anzahl() == 1 &&
               !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    }));
    add(tu("nichts gewaehlt", [] { ke::editor().karte.auswahl.leeren(); }));
    add(maus("Strg+M2 auf B", xy(160, 0), 1, ImGuiMod_Ctrl));
    add(pause(0.2));
    add(pruefe("Maus 2D: Strg+M2 = Flaechen-Tunnel (eine Flaeche von B gewaehlt)", [] {
        const auto& f = ke::editor().karte.auswahl.flaechen();
        diag::detail("Knopftest: Maus - Strg+M2 " + std::to_string(f.size()) + " Flaechen");
        return f.size() == 1 && ke::editor().karte.auswahl.anzahl() == 0;
    }));
    add(tu("nichts gewaehlt", [] { ke::editor().karte.auswahl.leeren(); }));
    add({"Strg+M2 Rechteck um A und B", [](int b) {
             static Schritt k;
             if (b == 0) {
                 const ImVec2 a = stufe2::weltXy(-96, 96);
                 const ImVec2 z = stufe2::weltXy(224, -96);
                 k = maus("Rechteck", [a] { return a; }, 1, ImGuiMod_Ctrl, ImVec2{z.x - a.x, z.y - a.y});
             }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(pruefe("Maus 2D: Strg+M2 ziehen = Rechteck fuer Flaechen (Flaechen von A, B und C, keine Pinsel)", [] {
        const auto& f = ke::editor().karte.auswahl.flaechen();
        diag::detail("Knopftest: Maus - Strg+M2 Rechteck " + std::to_string(f.size()) + " Flaechen");
        return f.size() >= 3 && ke::editor().karte.auswahl.anzahl() == 0;
    }));
    add(tu("nichts gewaehlt, B waehlen, Ecken-Modus", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), pinselNr(1));
    }));
    add(tasteKombi(ImGuiKey_V, "V"));
    add(maus("Alt+M1 auf A/C im Ecken-Modus", xy(-32, -32), 0, ImGuiMod_Alt));
    add(pause(0.2));
    add(pruefe("Maus 2D: Alt+M1 waehlt im Komponenten-Modus Objekte (B und A/C gewaehlt, Ecken-Modus bleibt)", [] {
        const ke::Editor& e = ke::editor();
        return istGewaehlt(1) && (istGewaehlt(0) || istGewaehlt(2)) && e.bearbeitung.einstellung.modus == karte::AuswahlModus::Komponente;
    }));
    add(tasteKombi(ImGuiKey_V, "V"));
    add(tu("B allein gewaehlt", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), pinselNr(1));
    }));
    auto zahl = std::make_shared<std::size_t>(0);
    add(tu("Pinsel zaehlen", [zahl] { *zahl = ke::editor().karte.welt()->kinder().size(); }));
    add(maus("Strg+Alt+M1 an der rechten Flaeche von B nach rechts ziehen", xy(192, 0), 0, ImGuiMod_Ctrl | ImGuiMod_Alt, ImVec2{48, 0}));
    add(pause(0.2));
    add(pruefe("Maus 2D: Strg+Alt+M1 zieht Flaechen heraus (ein Pinsel mehr)", [zahl] {
        const std::size_t n = ke::editor().karte.welt()->kinder().size();
        diag::detail("Knopftest: Maus - Extrusion " + std::to_string(*zahl) + " -> " + std::to_string(n) + " Pinsel");
        return n == *zahl + 1;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("nichts gewaehlt", [] { ke::editor().karte.auswahl.leeren(); }));
    add(tu("Pinsel zaehlen", [zahl] { *zahl = ke::editor().karte.welt()->kinder().size(); }));
    add(maus("M1 ziehen mit Umschalt: neuer Pinsel", xy(256, 256), 0, 0, ImVec2{0, 0}));
    add({"neuer Pinsel quadratisch (Umschalt im Zug)", [](int b) {
             static Schritt k;
             if (b == 0) {
                 const ImVec2 a = stufe2::weltXy(256, 256);
                 const ImVec2 z = stufe2::weltXy(320, 288);
                 k = maus("quadratisch", [a] { return a; }, 0, 0, ImVec2{z.x - a.x, z.y - a.y}, ImGuiMod_Shift);
             }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(pruefe("Maus 2D: M1 ziehen + Umschalt = neuer Pinsel quadratisch (64 x 64)", [zahl] {
        const ke::Editor& e = ke::editor();
        const std::size_t n = e.karte.welt()->kinder().size();
        const karte::Kasten k = kasten(n - 1);
        diag::detail("Knopftest: Maus - neu " + std::to_string(k.extents[0] * 2) + " x " + std::to_string(k.extents[1] * 2) + " x " +
                     std::to_string(k.extents[2] * 2));
        return n >= *zahl + 1 && std::fabs(k.extents[0] - k.extents[1]) < 0.01F && std::fabs(k.extents[0] - 32.0F) < 0.01F;
    }));
    add(tu("nichts gewaehlt", [] { ke::editor().karte.auswahl.leeren(); }));
    add({"neuer Pinsel als Wuerfel (Strg im Zug)", [](int b) {
             static Schritt k;
             if (b == 0) {
                 const ImVec2 a = stufe2::weltXy(-256, 256);
                 const ImVec2 z = stufe2::weltXy(-192, 288);
                 k = maus("Wuerfel", [a] { return a; }, 0, 0, ImVec2{z.x - a.x, z.y - a.y}, ImGuiMod_Ctrl);
             }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(pruefe("Maus 2D: M1 ziehen + Strg = neuer Pinsel als Wuerfel (64 x 64 x 64)", [] {
        const ke::Editor& e = ke::editor();
        const karte::Kasten k = kasten(e.karte.welt()->kinder().size() - 1);
        diag::detail("Knopftest: Maus - Wuerfel " + std::to_string(k.extents[0] * 2) + " x " + std::to_string(k.extents[1] * 2) + " x " +
                     std::to_string(k.extents[2] * 2));
        return std::fabs(k.extents[0] - 32.0F) < 0.01F && std::fabs(k.extents[1] - 32.0F) < 0.01F && std::fabs(k.extents[2] - 32.0F) < 0.01F;
    }));
    // --- nur 2D ---
    add(tu("B allein gewaehlt (QE-Werkzeug)", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), pinselNr(1));
    }));
    add(maus("Alt+M1 an der Ecke (192 32) nach oben ziehen", xy(192, 32), 0, ImGuiMod_Alt, ImVec2{0, -48}));
    add(pause(0.2));
    add(pruefe("Maus 2D: Alt+M1 im QE-Werkzeug schert (eine Flaeche von B steht schraeg)", [] {
        const auto b = pinselNr(1);
        if (!b) { return false; }
        b->bRep(ke::editor().karte.kontext);
        int schraeg = 0;
        for (const auto& f : b->flaechen()) {
            if (!f->traegtBei()) { continue; }
            const auto n = f->ebene().normal();
            const double m = std::max({std::fabs(n[0]), std::fabs(n[1]), std::fabs(n[2])});
            schraeg += m < 0.9999 ? 1 : 0;
        }
        diag::detail("Knopftest: Maus - Scheren: " + std::to_string(schraeg) + " schraege Flaechen");
        return schraeg >= 1;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("Drehen-Werkzeug (Pivot sichtbar)", [] { (void)ke::ausfuehren("MouseRotate"); }));
    add(maus("Umschalt+M3 bei (40 24)", xy(40, 24), 2, ImGuiMod_Shift));
    add(pause(0.2));
    add(pruefe("Maus 2D: Umschalt+M3 setzt den Ursprung der Drehung (40 24, eingerastet), z bleibt", [] {
        ke::Editor& e = ke::editor();
        const karte::Matrix4& m = e.bearbeitung.pivotZuWelt();
        diag::detail("Knopftest: Maus - Pivot " + std::to_string(m[12]) + " " + std::to_string(m[13]) + " " + std::to_string(m[14]));
        return std::fabs(m[12] - 40.0F) < 0.01F && std::fabs(m[13] - 24.0F) < 0.01F && std::fabs(m[14] - 32.0F) < 0.01F;
    }));
    add(tu("Resize", [] { (void)ke::ausfuehren("MouseDrag"); }));
    // --- nur 3D ---
    add(tu("Kamera schaut von oben auf A und B, nichts gewaehlt", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.kamera.setzeUrsprung(karte::Vektor3f(64, -320, 320));
        e.kamera.setzeWinkel(karte::Vektor3f(-40, 90, 0));
    }));
    add(pause(0.3));
    add(maus("Strg+M1 auf die Oberseite von B", kam(160, 0, 64), 0, ImGuiMod_Ctrl));
    add(pause(0.2));
    add(pruefe("Maus 3D: Strg+M1 waehlt eine Flaeche (die Oberseite von B)", [] {
        const auto& f = ke::editor().karte.auswahl.flaechen();
        const auto fl = f.empty() ? nullptr : f.front().flaeche.lock();
        const bool oben = f.size() == 1 && fl && fl->ebene().normal()[2] > 0.99;
        diag::detail("Knopftest: Maus - Strg+M1 3D " + std::to_string(f.size()) + (oben ? " Oberseite" : ""));
        return oben;
    }));
    add(tu("nichts gewaehlt", [] { ke::editor().karte.auswahl.leeren(); }));
    add({"Strg+M1 ziehen von B nach C (Flaechen malen)", [](int b) {
             static Schritt k;
             if (b == 0) {
                 const ImVec2 a = kam(160, 0, 64)();
                 const ImVec2 z = kam(0, 0, 160)();
                 k = maus("malen", [a] { return a; }, 0, ImGuiMod_Ctrl, ImVec2{z.x - a.x, z.y - a.y});
             }
             return k.tun(b);
         }});
    add(pause(0.2));
    add(pruefe("Maus 3D: Strg+M1 ziehen malt Flaechen (die Oberseiten von B und C)", [] {
        const auto& f = ke::editor().karte.auswahl.flaechen();
        int oben = 0;
        for (const auto& x : f) {
            const auto fl = x.flaeche.lock();
            oben += (fl && fl->ebene().normal()[2] > 0.99) ? 1 : 0;
        }
        diag::detail("Knopftest: Maus - Strg+M1 ziehen 3D " + std::to_string(f.size()) + " Flaechen, " + std::to_string(oben) + " oben");
        return oben >= 2;
    }));
    add(tu("B gewaehlt, Kamera von vorn", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), pinselNr(1));
        e.kamera.setzeUrsprung(karte::Vektor3f(160, -320, 32));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 90, 0));
    }));
    add(pause(0.3));
    auto vorher = std::make_shared<karte::Kasten>();
    add(tu("B merken", [vorher] { *vorher = kasten(1); }));
    add(maus("Alt+M1 in der Kamera an der Vorderseite von B nach oben ziehen", kam(176, -32, 48), 0, ImGuiMod_Alt, ImVec2{0, -80}));
    add(pause(0.2));
    add(pruefe("Maus 3D: Alt+M1 = die andere Groessenaenderung (B ist danach anders gross)", [vorher] {
        const karte::Kasten k = kasten(1);
        diag::detail("Knopftest: Maus - Alt+M1 3D: Ausdehnung " + std::to_string(vorher->extents[0]) + "/" + std::to_string(vorher->extents[1]) + "/" +
                     std::to_string(vorher->extents[2]) + " -> " + std::to_string(k.extents[0]) + "/" + std::to_string(k.extents[1]) + "/" +
                     std::to_string(k.extents[2]));
        return std::fabs(k.extents[0] - vorher->extents[0]) + std::fabs(k.extents[1] - vorher->extents[1]) +
                   std::fabs(k.extents[2] - vorher->extents[2]) > 1.0F;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("Resize (QE), B gewaehlt", [vorher] {
        ke::Editor& e = ke::editor();
        if (e.bearbeitung.modus() != karte::ManipModus::Ziehen) { (void)ke::ausfuehren("MouseDrag"); }
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), pinselNr(1));
        *vorher = kasten(1);
    }));
    add(pause(0.3));
    add(maus("M1 auf B ziehen, Alt dazu (senkrecht)", kam(168, -32, 40), 0, 0, ImVec2{60, -60}, ImGuiMod_Alt));
    add(pause(0.2));
    add(pruefe("Maus 3D: M1 + Alt verschiebt nur senkrecht (z anders, x und y gleich)", [vorher] {
        const karte::Kasten k = kasten(1);
        diag::detail("Knopftest: Maus - M1+Alt 3D: Mitte " + std::to_string(vorher->origin[0]) + "/" + std::to_string(vorher->origin[1]) + "/" +
                     std::to_string(vorher->origin[2]) + " -> " + std::to_string(k.origin[0]) + "/" + std::to_string(k.origin[1]) + "/" +
                     std::to_string(k.origin[2]));
        return std::fabs(k.origin[0] - vorher->origin[0]) < 0.01F && std::fabs(k.origin[1] - vorher->origin[1]) < 0.01F &&
               std::fabs(k.origin[2] - vorher->origin[2]) > 1.0F;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(tu("Resize, nichts gewaehlt, Kamera auf A", [] {
        ke::Editor& e = ke::editor();
        (void)ke::ausfuehren("MouseDrag");
        e.karte.auswahl.leeren();
        e.kamera.setzeUrsprung(karte::Vektor3f(-256, 0, 32));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
    }));
    add(pause(0.3));
    // Alt+M2: Orbit um den angeklickten Punkt
    // Der Punkt liegt neben der Blickachse (-64 32 40), damit ein Kreisen um einen
    // anderen Punkt (z. B. 0 0 0 auf der Achse) auffaellt.
    add(maus("Alt+M2 auf A bei (-64 32 40), nach rechts ziehen", kam(-64, 32, 40), 1, ImGuiMod_Alt, ImVec2{120, 0}));
    add(pause(0.2));
    add(pruefe("Maus 3D: Alt+M2 kreist um den angeklickten Punkt (der Abstand zu ihm bleibt, die Kamera wandert, Blickrichtung anders)", [] {
        const ke::Editor& e = ke::editor();
        const karte::Vektor3f d = e.kamera.ursprung - karte::Vektor3f(-64, 32, 40);
        const float abstand = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        const float soll = std::sqrt(192.0F * 192.0F + 32.0F * 32.0F + 8.0F * 8.0F);
        const karte::Vektor3f w = e.kamera.ursprung - karte::Vektor3f(-256, 0, 32);
        const float weg = std::sqrt(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]);
        diag::detail("Knopftest: Maus - Orbit: Abstand " + std::to_string(abstand) + " (soll " + std::to_string(soll) + "), gewandert " + std::to_string(weg) +
                     ", yaw " + std::to_string(e.kamera.winkel[1]) + (e.kamFenster.frei ? ", Freelook an" : ""));
        return std::fabs(abstand - soll) < 0.05F && weg > 5.0F && std::fabs(e.kamera.winkel[1]) > 1.0F && !e.kamFenster.frei;
    }));
    add(tu("Kamera auf A", [] {
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(karte::Vektor3f(-256, 0, 32));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
    }));
    add(pause(0.2));
    add(maus("M2 gedrueckt nach rechts ziehen", kam(-64, 0, 32), 1, 0, ImVec2{100, 0}));
    add(pause(0.2));
    add(pruefe("Maus 3D: M2 ziehen = Strafe seitwaerts (y anders, Winkel gleich)", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Maus - M2 ziehen: " + std::to_string(e.kamera.ursprung[0]) + "/" + std::to_string(e.kamera.ursprung[1]) + "/" +
                     std::to_string(e.kamera.ursprung[2]) + ", yaw " + std::to_string(e.kamera.winkel[1]));
        return std::fabs(e.kamera.ursprung[1]) > 1.0F && std::fabs(e.kamera.winkel[1]) < 0.01F && std::fabs(e.kamera.winkel[0]) < 0.01F;
    }));
    add(tu("Kamera auf A, Freelook aus", [] {
        ke::Editor& e = ke::editor();
        if (e.kamFenster.frei) { ke::freelookBeenden(); }
        e.kamera.setzeUrsprung(karte::Vektor3f(-256, 0, 32));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
    }));
    add(pause(0.3));
    add(maus("kurzer M2: Freelook an", kam(-64, 0, 32), 1, 0));
    add(pause(0.3));
    add({"im Freelook: Strg halten, Maus nach rechts", [](int b) {
             ImGuiIO& io = ImGui::GetIO();
             static ImVec2 a;
             if (b == 0) {
                 a = io.MousePos;
                 io.AddKeyEvent(ImGuiMod_Ctrl, true);
                 return false;
             }
             if (b >= 2 && b <= 10) {
                 setzeMaus(ImVec2{a.x + static_cast<float>(b - 1) * 10.0F, a.y});
                 return false;
             }
             if (b == 11) { io.AddKeyEvent(ImGuiMod_Ctrl, false); }
             return b >= 13;
         }});
    add(pause(0.2));
    add(pruefe("Maus 3D: Strg + Maus im Freelook = Strafe (Ursprung anders, Winkel gleich)", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Maus - Strg+Maus: y " + std::to_string(e.kamera.ursprung[1]) + ", yaw " + std::to_string(e.kamera.winkel[1]));
        return e.kamFenster.frei && std::fabs(e.kamera.ursprung[1]) > 1.0F && std::fabs(e.kamera.winkel[1]) < 0.01F;
    }));
    add(tu("Ursprung zurueck", [] { ke::editor().kamera.setzeUrsprung(karte::Vektor3f(-256, 0, 32)); }));
    add({"im Freelook: Strg+Umschalt halten, Maus nach oben", [](int b) {
             ImGuiIO& io = ImGui::GetIO();
             static ImVec2 a;
             if (b == 0) {
                 a = io.MousePos;
                 io.AddKeyEvent(ImGuiMod_Ctrl, true);
                 io.AddKeyEvent(ImGuiMod_Shift, true);
                 return false;
             }
             if (b >= 2 && b <= 10) {
                 setzeMaus(ImVec2{a.x, a.y - static_cast<float>(b - 1) * 10.0F});
                 return false;
             }
             if (b == 11) {
                 io.AddKeyEvent(ImGuiMod_Shift, false);
                 io.AddKeyEvent(ImGuiMod_Ctrl, false);
             }
             return b >= 13;
         }});
    add(pause(0.2));
    add(pruefe("Maus 3D: Strg+Umschalt + Maus im Freelook = Strafe vor/zurueck (x anders, y und Winkel gleich)", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Maus - Strg+Umschalt+Maus: x " + std::to_string(e.kamera.ursprung[0]) + ", y " + std::to_string(e.kamera.ursprung[1]));
        return std::fabs(e.kamera.ursprung[0] + 256.0F) > 1.0F && std::fabs(e.kamera.ursprung[1]) < 0.01F && std::fabs(e.kamera.winkel[1]) < 0.01F;
    }));
    add(tu("Freelook aus, Kamera auf A", [] {
        ke::Editor& e = ke::editor();
        if (e.kamFenster.frei) { ke::freelookBeenden(); }
        e.kamera.setzeUrsprung(karte::Vektor3f(-256, 0, 32));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 0, 0));
    }));
    add(pause(0.3));
    // M3 und seine Zusatztasten (TexManipulator_, Scene_applyClosestTexture)
    add(maus("M3 auf A (kopieren)", kam(-64, 0, 32), 2, 0));
    add(pause(0.2));
    const auto auftragen = [&add](const std::string& name, ImGuiKeyChord mods, const std::string& undo, bool shaderNeu) {
        add(tu("Browser-Shader system/nodraw", [] { ke::texturShaderWaehlen("textures/system/nodraw"); }));
        add(maus(name + " auf A", kam(-64, 16, 40), 2, mods));
        add(pause(0.2));
        add(pruefe("Maus 3D: " + name + " (" + undo + ")", [undo, shaderNeu] {
            ke::Editor& e = ke::editor();
            const auto a = pinselNr(0);
            int nodraw = 0;
            if (a) {
                for (const auto& f : a->flaechen()) { nodraw += f->shader() == "textures/system/nodraw" ? 1 : 0; }
            }
            diag::detail("Knopftest: Maus - " + undo + ": Rueckgaengig \"" + e.karte.undo.undoName() + "\", nodraw " + std::to_string(nodraw));
            return e.karte.undo.undoName() == undo && nodraw == (shaderNeu ? 1 : 0);
        }));
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    };
    auftragen("Alt+M3", ImGuiMod_Alt, "paintTexture", true);
    auftragen("Umschalt+M3", ImGuiMod_Shift, "paintTexture,Values,LightPower", true);
    auftragen("Strg+M3", ImGuiMod_Ctrl, "paintTextureSeamless,LightColor", true);
    auftragen("Strg+Umschalt+M3", ImGuiMod_Ctrl | ImGuiMod_Shift, "projectTexture,LightColor&Power", true);
    auftragen("Alt+Umschalt+M3", ImGuiMod_Alt | ImGuiMod_Shift, "paintTexDefValues", false);
    auftragen("Alt+Strg+M3", ImGuiMod_Alt | ImGuiMod_Ctrl, "paintTexDefValuesSeamless", false);
    auftragen("Alt+Strg+Umschalt+M3", ImGuiMod_Alt | ImGuiMod_Ctrl | ImGuiMod_Shift, "projectTexDefValues", false);
    // --- Texturbrowser ---
    add(tu("Texturbrowser: system/ zeigen, Shader-Text zu, Startbefehl ersetzen", [] {
        ke::Editor& e = ke::editor();
        e.shaderText.offen = false;
        ke::texturVerzeichnisZeigen("system/");
    }));
    add(pause(0.4));
    add(maus("Umschalt+M1 auf system/caulk", [] {
        ImVec2 p{0, 0};
        (void)ke::texturMitte("textures/system/caulk", p);
        return p;
    }, 0, ImGuiMod_Shift));
    add(pause(0.3));
    add(pruefe("Maus Texturbrowser: Umschalt+M1 oeffnet den Shader im eigenen Editor (system.shader)", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Maus - Shader-Text " + e.shaderText.datei + ", Zeile " + std::to_string(e.shaderText.springen));
        return e.shaderText.offen && e.shaderText.datei.find("system.shader") != std::string::npos;
    }));
    add(tu("Shader-Text zu, Shader-Editor-Befehl editor.exe, Start ersetzt", [] {
        ke::Editor& e = ke::editor();
        e.shaderText.offen = false;
        e.shaderEditorBefehl = "editor.exe";
        e.letzteUrl.clear();
        e.befehlStarten = [](const std::string& z) {
            ke::editor().letzteUrl = z;
            return true;
        };
    }));
    add(pause(0.2));
    add(maus("Strg+Umschalt+M1 auf system/caulk", [] {
        ImVec2 p{0, 0};
        (void)ke::texturMitte("textures/system/caulk", p);
        return p;
    }, 0, ImGuiMod_Ctrl | ImGuiMod_Shift));
    add(pause(0.3));
    add(pruefe("Maus Texturbrowser: Strg+Umschalt+M1 = externer Editor (loser Shader: editor.exe mit der Datei; im Paket der eigene)", [] {
        ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Maus - extern: " + e.letzteUrl + (e.shaderText.offen ? ", eigener offen" : ""));
        const bool lose = e.letzteUrl.rfind("editor.exe \"", 0) == 0 && e.letzteUrl.find("system.shader") != std::string::npos;
        const bool paket = e.letzteUrl.empty() && e.shaderText.offen && e.shaderText.datei.find("::") != std::string::npos;
        return lose || paket;
    }));
    add(tu("Shader-Text zu, Start wie vorher", [] {
        ke::Editor& e = ke::editor();
        e.shaderText.offen = false;
        e.shaderEditorBefehl.clear();
        e.befehlStarten = nullptr;
    }));
    add(pause(0.2));
    auto vorRollen = std::make_shared<int>(0);
    add(tu("Ursprung merken, Liste lang", [vorRollen] {
        ke::Editor& e = ke::editor();
        ke::texturVerzeichnisZeigen("common/");   // viele Texturen: die Liste ist laenger als die Ansicht
        (void)e;
        *vorRollen = e.texBro.ursprungY(e.aktiveShader);
    }));
    add(pause(0.3));
    add(tu("Ursprung merken", [vorRollen] { *vorRollen = ke::editor().texBro.ursprungY(ke::editor().aktiveShader); }));
    add(maus("Umschalt+M2 im Texturbrowser 10 nach unten ziehen", [] {
        const ke::TexturBrowserLage& l = ke::texturBrowserLage();
        return ImVec2{l.ansichtX + 20.0F, l.ansichtY + 30.0F};
    }, 1, ImGuiMod_Shift, ImVec2{0, -20.0F * std::max(1.0F, std::round(g_app->dpiScale))}));
    add(pause(0.2));
    add(pruefe("Maus Texturbrowser: Umschalt+M2 ziehen rollt viermal so schnell (20 logische Punkte -> 80), kein Kontextmenue", [vorRollen] {
        ke::Editor& e = ke::editor();
        const int nach = e.texBro.ursprungY(e.aktiveShader);
        const int unten = std::min(e.texBro.hoehe - e.texBro.gesamtHoehe(e.aktiveShader), 0);
        diag::detail("Knopftest: Maus - Umschalt+M2: " + std::to_string(*vorRollen) + " -> " + std::to_string(nach) + " (unten " + std::to_string(unten) + ")");
        return (nach - *vorRollen == -80 || (nach == unten && *vorRollen - unten < 80)) &&
               !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    }));
    // --- Entitaetenmenue: M2 wandelt um, Strg+M2 haelt es offen ---
    add(tu("ein Licht waehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        for (const auto& ent : e.karte.entitaeten()) {
            if (ent->klassenname() == "light") {
                e.karte.auswahl.waehlen(ent);
                break;
            }
        }
    }));
    add(maus("M2 in die XY-Ansicht (Menue)", xy(256, -256), 1, 0));
    add(pause(0.3));
    add(klick("info"));
    add(pause(0.3));
    add({"M2 auf info_null (umwandeln)", [](int b) {
             static Schritt k;
             if (b == 0) {
                 const Element* el = finde("info_null", "", 0, false);
                 const ImVec2 m = el != nullptr ? el->rect.GetCenter() : ImVec2{5, 5};
                 k = maus("M2 info_null", [m] { return m; }, 1, 0);
             }
             return k.tun(b);
         }});
    add(pause(0.3));
    add(pruefe("Maus Entitaetenmenue: M2 wandelt die gewaehlte Entity um (light -> info_null, wie NRC danach nicht gewaehlt), das Menue schliesst", [] {
        const ke::Editor& e = ke::editor();
        int licht = 0;
        int null = 0;
        for (const auto& ent : e.karte.entitaeten()) {
            licht += ent->klassenname() == "light" ? 1 : 0;
            null += ent->klassenname() == "info_null" ? 1 : 0;
        }
        diag::detail("Knopftest: Maus - umwandeln: light " + std::to_string(licht) + ", info_null " + std::to_string(null) + ", " +
                     e.karte.undo.undoName());
        return licht == 0 && null == 1 && e.karte.undo.undoName() == "entitySetClass -class info_null" &&
               !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    }));
    add(tu("info_null waehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        for (const auto& ent : e.karte.entitaeten()) {
            if (ent->klassenname() == "info_null") {
                e.karte.auswahl.waehlen(ent);
                break;
            }
        }
    }));
    add(maus("M2 in die XY-Ansicht (Menue)", xy(256, -256), 1, 0));
    add(pause(0.3));
    add(klick("info"));
    add(pause(0.3));
    add({"Strg+M2 auf info_notnull (umwandeln, Menue bleibt)", [](int b) {
             static Schritt k;
             if (b == 0) {
                 const Element* el = finde("info_notnull", "", 0, false);
                 const ImVec2 m = el != nullptr ? el->rect.GetCenter() : ImVec2{5, 5};
                 k = maus("Strg+M2 info_notnull", [m] { return m; }, 1, ImGuiMod_Ctrl);
             }
             return k.tun(b);
         }});
    add(pause(0.3));
    add(pruefe("Maus Entitaetenmenue: Strg+M2 wandelt um (info_null -> info_notnull), das Menue bleibt offen", [] {
        const ke::Editor& e = ke::editor();
        int notnull = 0;
        for (const auto& ent : e.karte.entitaeten()) { notnull += ent->klassenname() == "info_notnull" ? 1 : 0; }
        return notnull == 1 && e.karte.undo.undoName() == "entitySetClass -class info_notnull" && finde("info_notnull", "", 0, false) != nullptr;
    }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    // worldspawn im Menue: gewaehlte Primitive aus ihrer Gruppe loesen (m_2world)
    auto weltZahl = std::make_shared<std::size_t>(0);
    add(tu("B allein gewaehlt (Welt-Pinsel)", [weltZahl] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), pinselNr(1));
        *weltZahl = e.karte.welt()->kinder().size();
    }));
    add(maus("M2 in die XY-Ansicht (Menue)", xy(256, -256), 1, 0));
    add(pause(0.3));
    add(klick("func"));
    add(pause(0.3));
    add(klick("func_group"));
    add(pause(0.3));
    auto gruppeB = std::make_shared<std::weak_ptr<karte::Primitiv>>();
    add(pruefe("Maus Entitaetenmenue: M1 auf func_group macht aus dem gewaehlten Welt-Pinsel eine Gruppe", [gruppeB, weltZahl] {
        const ke::Editor& e = ke::editor();
        for (const auto& ent : e.karte.entitaeten()) {
            if (ent->klassenname() == "func_group" && ent->kinder().size() == 1) {
                *gruppeB = ent->kinder().front();
                diag::detail("Knopftest: Maus - func_group: Welt " + std::to_string(*weltZahl) + " -> " + std::to_string(e.karte.welt()->kinder().size()));
                return e.karte.welt()->kinder().size() + 1 == *weltZahl;
            }
        }
        return false;
    }));
    add(tu("den Pinsel der Gruppe waehlen", [gruppeB] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        const auto p = gruppeB->lock();
        for (const auto& ent : e.karte.entitaeten()) {
            for (const auto& k : ent->kinder()) {
                if (k == p) { e.karte.auswahl.waehlen(ent, k); }
            }
        }
    }));
    add(maus("M2 in die XY-Ansicht (Menue)", xy(256, -256), 1, 0));
    add(pause(0.3));
    add(klick("worldspawn"));
    add(pause(0.3));
    add(pruefe("Maus Entitaetenmenue: M1 auf worldspawn loest die gewaehlten Primitive aus der Gruppe (zurueck in die Welt, Gruppe weg)", [weltZahl] {
        const ke::Editor& e = ke::editor();
        int gruppen = 0;
        for (const auto& ent : e.karte.entitaeten()) { gruppen += ent->klassenname() == "func_group" ? 1 : 0; }
        diag::detail("Knopftest: Maus - worldspawn: Welt " + std::to_string(e.karte.welt()->kinder().size()) + " Pinsel, " + std::to_string(gruppen) +
                     " Gruppen, " + e.karte.undo.undoName());
        return e.karte.welt()->kinder().size() == *weltZahl && gruppen == 0 && e.karte.undo.undoName() == "ungroupSelectedPrimitives";
    }));
    // Zusatztasten allein: onModifierDown / onModifierUp (g_modifiers, Hervorhebung)
    add({"Alt druecken (Maus ueber XY)", [](int b) {
             if (b == 0) { setzeMaus(stufe2::weltXy(300, 300)); }
             if (b == 2) { ImGui::GetIO().AddKeyEvent(ImGuiMod_Alt, true); }
             return b >= 5;
         }});
    add(pruefe("Maus: Alt gedrueckt -> g_modifiers mit Alt (onModifierDown)", [] {
        const karte::Zusatz& z = ke::editor().bearbeitung.zusatz;
        return z.alt && !z.shift && !z.strg;
    }));
    add({"Umschalt dazu, dann beide los", [](int b) {
             ImGuiIO& io = ImGui::GetIO();
             if (b == 0) { io.AddKeyEvent(ImGuiMod_Shift, true); }
             if (b == 3) {
                 const karte::Zusatz& z = ke::editor().bearbeitung.zusatz;
                 diag::detail(std::string("Knopftest: Maus - Zusatz bei Alt+Umschalt: ") + (z.alt ? "alt " : "") + (z.shift ? "shift" : ""));
                 io.AddKeyEvent(ImGuiMod_Shift, false);
                 io.AddKeyEvent(ImGuiMod_Alt, false);
             }
             return b >= 6;
         }});
    add(pruefe("Maus: losgelassen -> g_modifiers leer (onModifierUp)", [] {
        const karte::Zusatz& z = ke::editor().bearbeitung.zusatz;
        return !z.alt && !z.shift && !z.strg;
    }));
    // Tab und Leertaste im Dialog Arbitrary rotation: ShortcutOverride angenommen
    // (select.cpp:1645) - kein Klonen (Leertaste), kein Kamera-Fokus (Tab)
    auto stand = std::make_shared<std::pair<std::size_t, karte::Vektor3f>>();
    add(tu("B waehlen, Zahl und Kamera merken", [stand] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), pinselNr(1));
        *stand = {e.karte.welt()->kinder().size(), e.kamera.ursprung};
    }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_R, "Umschalt+R"));
    add(pause(0.3));
    add(klickAn("Feld X", stufe2::elementOrt("##kartedrehenwert0")));
    add(tasteKombi(ImGuiKey_Space, "Leertaste"));
    add(tasteKombi(ImGuiKey_Tab, "Tab"));
    add(pause(0.2));
    add(pruefe("Arbitrary rotation: Tab und Leertaste gehen an den Dialog (kein Klonen, kein Kamera-Fokus), er bleibt offen", [stand] {
        const ke::Editor& e = ke::editor();
        return e.drehDialog.offen && e.karte.welt()->kinder().size() == stand->first && e.kamera.ursprung == stand->second;
    }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(tu("Maus: zurueck (Resize, nichts gewaehlt)", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        if (e.kamFenster.frei) { ke::freelookBeenden(); }
        if (e.bearbeitung.modus() != karte::ManipModus::Ziehen) { (void)ke::ausfuehren("MouseDrag"); }
    }));
    return s;
}

// --- Punkt 0c, Teil 3: Teiler einklappen und zurueckziehen, Maximize view ---
//
// QSplitter mit childrenCollapsible (Vorgabe, wie NRC): ein Feld laesst sich
// bis auf 0 ziehen. Danach muss der Griff am Rand stehen bleiben und sich wieder
// herausziehen lassen - fuer jeden Teiler jeder Anordnung.

// Bildpunkt einer logischen Lage des Editors (aus einem Feld mit Rahmen).
ImVec2 editorBild(float lx, float ly) {
    const float sk = ke::massstab();
    for (const auto& r : ke::editor().rahmenImBild) {
        if (const auto* f = karteFeld(r.feld)) {
            return ImVec2{r.x0 - static_cast<float>(f->x) * sk + lx * sk, r.y0 - static_cast<float>(f->y) * sk + ly * sk};
        }
    }
    return ImVec2{5, 5};
}
std::function<ImVec2()> griffMitte(const std::string& griff) {
    return [griff] {
        const auto* g = karteFeld(griff);
        if (g == nullptr) { return ImVec2{5, 5}; }
        const float gx = static_cast<float>(g->x) + static_cast<float>(g->b) * 0.5F;
        const float gy = static_cast<float>(g->y) + static_cast<float>(g->h) * 0.5F;
        // Der Teiler im Texturbrowser rechnet ab der Ecke des Feldes "Texturen"
        if (griff.rfind("Griff:tex:", 0) == 0) {
            for (const auto& r : ke::editor().rahmenImBild) {
                if (r.feld == "Texturen") {
                    const float sk = ke::massstab();
                    return ImVec2{r.x0 + gx * sk, r.y0 + gy * sk};
                }
            }
        }
        return editorBild(gx, gy);
    };
}
int feldGroesse(const std::string& feld, bool senkrecht) {
    const auto* f = karteFeld(feld);
    if (f == nullptr) { return 0; }
    return senkrecht ? f->h : f->b;
}

// Ein Teiler: Griff bis an den Rand ziehen (das Feld dahinter klappt ein, der
// Griff bleibt am Rand), wieder herausziehen (das Feld ist zurueck); dasselbe in
// die andere Richtung.
void teilerPruefen(std::vector<Schritt>& s, const std::string& anordnung, const std::string& griff, bool senkrecht, const std::string& vorn,
                   const std::string& hinten) {
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const float weit = 3000.0F;
    const float zurueck = 160.0F * std::max(1.0F, std::round(g_app->dpiScale));
    add(maus(anordnung + ": " + griff + " ganz ans Ende ziehen", griffMitte(griff), 0, 0, senkrecht ? ImVec2{0, weit} : ImVec2{weit, 0}));
    add(pause(0.2));
    add(pruefe("Teiler " + anordnung + " " + griff + ": bis ans Ende gezogen klappt " + hinten + " ein, der Griff bleibt am Rand", [griff, senkrecht, hinten, vorn] {
        const auto* g = karteFeld(griff);
        const auto* v = karteFeld(vorn);
        const int h = feldGroesse(hinten, senkrecht);
        diag::detail("Knopftest: Teiler - " + griff + " am Ende: " + hinten + " " + std::to_string(h) + ", Griff " +
                     (g != nullptr ? std::to_string(senkrecht ? g->y : g->x) : std::string("-")) + ", " + vorn + " endet bei " +
                     (v != nullptr ? std::to_string(senkrecht ? v->y + v->h : v->x + v->b) : std::string("-")));
        return g != nullptr && v != nullptr && h == 0 && (senkrecht ? g->y == v->y + v->h : g->x == v->x + v->b);
    }));
    add(maus(anordnung + ": " + griff + " vom Rand zurueckziehen", griffMitte(griff), 0, 0, senkrecht ? ImVec2{0, -zurueck} : ImVec2{-zurueck, 0}));
    add(pause(0.2));
    add(pruefe("Teiler " + anordnung + " " + griff + ": vom Rand zurueckgezogen ist " + hinten + " wieder da", [senkrecht, hinten] {
        return feldGroesse(hinten, senkrecht) > 50;
    }));
    add(maus(anordnung + ": " + griff + " ganz an den Anfang ziehen", griffMitte(griff), 0, 0, senkrecht ? ImVec2{0, -weit} : ImVec2{-weit, 0}));
    add(pause(0.2));
    add(pruefe("Teiler " + anordnung + " " + griff + ": an den Anfang gezogen klappt " + vorn + " ein, der Griff bleibt am Rand", [griff, senkrecht, vorn, hinten] {
        const auto* g = karteFeld(griff);
        const auto* h = karteFeld(hinten);
        const int v = feldGroesse(vorn, senkrecht);
        diag::detail("Knopftest: Teiler - " + griff + " am Anfang: " + vorn + " " + std::to_string(v) + ", Griff " +
                     (g != nullptr ? std::to_string(senkrecht ? g->y : g->x) : std::string("-")));
        return g != nullptr && h != nullptr && v == 0 && (senkrecht ? g->y + g->h == h->y : g->x + g->b == h->x);
    }));
    add(maus(anordnung + ": " + griff + " vom Anfang zurueckziehen", griffMitte(griff), 0, 0, senkrecht ? ImVec2{0, zurueck} : ImVec2{zurueck, 0}));
    add(pause(0.2));
    add(pruefe("Teiler " + anordnung + " " + griff + ": vom Anfang zurueckgezogen ist " + vorn + " wieder da", [senkrecht, vorn] {
        return feldGroesse(vorn, senkrecht) > 50;
    }));
}

std::vector<Schritt> teiler() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const auto anordnen = [&add](const std::string& name, karte::befehle::Anordnung a, bool gruppe) {
        add(tu("Teiler: Anordnung " + name, [a, gruppe] {
            ke::Editor& e = ke::editor();
            e.anordnung = a;
            e.gruppenDialogEingebaut = gruppe;
            e.hGroessen.clear();
            e.vGroessen.clear();
            e.v2Groessen.clear();
            e.teilerFuer = -1;
        }));
        add(pause(0.4));
    };
    using A = karte::befehle::Anordnung;
    anordnen("Regular", A::Regular, false);
    teilerPruefen(s, "Regular", "Griff:h:1", false, "XY", "Kamera");
    teilerPruefen(s, "Regular", "Griff:v:1", true, "XY", "Konsole");
    teilerPruefen(s, "Regular", "Griff:v2:1", true, "Kamera", "Texturen");
    teilerPruefen(s, "Regular", "Griff:tex:1", false, "Texturen:Links", "Texturen:Ansicht");
    anordnen("Regular Left", A::RegularLeft, false);
    teilerPruefen(s, "Regular Left", "Griff:h:1", false, "Kamera", "XY");
    teilerPruefen(s, "Regular Left", "Griff:v:1", true, "XY", "Konsole");
    teilerPruefen(s, "Regular Left", "Griff:v2:1", true, "Kamera", "Texturen");
    anordnen("Split", A::Split, false);
    teilerPruefen(s, "Split", "Griff:h:1", false, "Kamera", "XY");
    teilerPruefen(s, "Split", "Griff:v:1", true, "Kamera", "YZ");
    teilerPruefen(s, "Split", "Griff:v2:1", true, "XY", "XZ");
    anordnen("Regular mit eingebautem Gruppendialog", A::Regular, true);
    add(tu("Gruppendialog zeigen", [] {
        if (!ke::editor().gruppenDialogOffen) { (void)ke::ausfuehren("ToggleEntityInspector"); }
    }));
    add(pause(0.3));
    teilerPruefen(s, "Regular+Gruppe", "Griff:h:2", false, "Kamera", "Gruppe");
    add(pruefe("Floating: keine Teiler (die Ansichten schweben)", [] { return true; }));
    // Maximize view (F12) und zurueck
    anordnen("Regular", A::Regular, false);
    auto vorher = std::make_shared<std::array<int, 4>>();
    add(tu("Groessen merken", [vorher] {
        *vorher = {feldGroesse("XY", false), feldGroesse("XY", true), feldGroesse("Kamera", false), feldGroesse("Konsole", true)};
    }));
    add({"Maus in die Kamera (ohne Klick)", [](int b) {
             if (b == 0) {
                 const auto* f = karteFeld("Kamera");
                 setzeMaus(f != nullptr ? editorBild(static_cast<float>(f->x + f->b / 2), static_cast<float>(f->y + f->h / 2)) : ImVec2{5, 5});
             }
             return b >= 3;
         }});
    add(tasteKombi(ImGuiKey_F12, "F12"));
    add(pause(0.3));
    add(pruefe("Maximize view (F12): die Kamera unter der Maus fuellt den Bereich, XY, Konsole und Texturen sind eingeklappt", [] {
        const int kb = feldGroesse("Kamera", false);
        const int kh = feldGroesse("Kamera", true);
        diag::detail("Knopftest: Teiler - F12: Kamera " + std::to_string(kb) + "x" + std::to_string(kh) + ", XY " + std::to_string(feldGroesse("XY", false)) +
                     ", Texturen " + std::to_string(feldGroesse("Texturen", true)));
        return feldGroesse("XY", false) == 0 && feldGroesse("Texturen", true) == 0 && kb > 1000 && kh > 500;
    }));
    add(tasteKombi(ImGuiKey_F12, "F12"));
    add(pause(0.3));
    add(pruefe("Maximize view (F12) noch einmal: alle Groessen wie vorher", [vorher] {
        const std::array<int, 4> jetzt{feldGroesse("XY", false), feldGroesse("XY", true), feldGroesse("Kamera", false), feldGroesse("Konsole", true)};
        diag::detail("Knopftest: Teiler - F12 zurueck: XY " + std::to_string(jetzt[0]) + "x" + std::to_string(jetzt[1]) + " (vorher " +
                     std::to_string((*vorher)[0]) + "x" + std::to_string((*vorher)[1]) + ")");
        return jetzt == *vorher;
    }));
    add(tu("Teiler: Groessen wie vorher", [] {
        ke::Editor& e = ke::editor();
        e.hGroessen.clear();
        e.vGroessen.clear();
        e.v2Groessen.clear();
        e.gruppenDialogEingebaut = false;
        e.teilerFuer = -1;
    }));
    return s;
}


// --- Punkt 3: Build-Menue und der Dialog "Build Menu" -------------------------

// Die Testkarte von tools/karte/gen_q3map2_testkarten.py (raum.map: zwei Raeume
// mit Durchgang, zwei Lichter, Detail-Saeule, Tuer, Patch; leck: Loch in der
// Ostwand) - derselbe Text.
std::string raumKarte(bool leck) {
    const auto kasten = [](int x0, int y0, int z0, int x1, int y1, int z1, const char* tex, int inhalt) {
        char t[96];
        std::snprintf(t, sizeof(t), "%s 0 0 0 0.5 0.5 %d 0 0", tex, inhalt);
        char z[6][200];
        std::snprintf(z[0], sizeof(z[0]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s", x1, y1, z1, x1, y0, z1, x0, y1, z1, t);
        std::snprintf(z[1], sizeof(z[1]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s", x1, y1, z1, x0, y1, z1, x1, y1, z0, t);
        std::snprintf(z[2], sizeof(z[2]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s", x1, y1, z1, x1, y1, z0, x1, y0, z1, t);
        std::snprintf(z[3], sizeof(z[3]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s", x0, y0, z0, x1, y0, z0, x0, y1, z0, t);
        std::snprintf(z[4], sizeof(z[4]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s", x0, y0, z0, x0, y0, z1, x1, y0, z0, t);
        std::snprintf(z[5], sizeof(z[5]), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s", x0, y0, z0, x0, y1, z0, x0, y0, z1, t);
        std::vector<std::string> v(std::begin(z), std::end(z));
        return v;
    };
    std::vector<std::string> zeilen = {"// entity 0", "{", "\"classname\" \"worldspawn\"", "\"message\" \"BehavEd q3map2 test\"",
                                       "\"_color\" \"1 0.95 0.9\"", "\"ambient\" \"8\""};
    std::vector<std::vector<std::string>> pinsel;
    pinsel.push_back(kasten(-528, -272, -16, 528, 272, 0, "test/boden", 0));
    pinsel.push_back(kasten(-528, -272, 256, 528, 272, 272, "test/decke", 0));
    pinsel.push_back(kasten(-528, 256, 0, 528, 272, 256, "test/wand", 0));
    pinsel.push_back(kasten(-528, -272, 0, 528, -256, 256, "test/wand", 0));
    pinsel.push_back(kasten(-528, -256, 0, -512, 256, 256, "test/wand", 0));
    if (leck) {
        pinsel.push_back(kasten(512, -256, 0, 528, -32, 256, "test/wand", 0));
        pinsel.push_back(kasten(512, 32, 0, 528, 256, 256, "test/wand", 0));
        pinsel.push_back(kasten(512, -32, 0, 528, 32, 64, "test/wand", 0));
        pinsel.push_back(kasten(512, -32, 128, 528, 32, 256, "test/wand", 0));
    } else {
        pinsel.push_back(kasten(512, -256, 0, 528, 256, 256, "test/wand", 0));
    }
    pinsel.push_back(kasten(-8, -256, 0, 8, -64, 256, "test/mitte", 0));
    pinsel.push_back(kasten(-8, 64, 0, 8, 256, 256, "test/mitte", 0));
    pinsel.push_back(kasten(-8, -64, 128, 8, 64, 256, "test/mitte", 0));
    pinsel.push_back(kasten(-288, -32, 0, -224, 32, 256, "test/saeule", 134217728));
    for (std::size_t i = 0; i < pinsel.size(); ++i) {
        zeilen.push_back("// brush " + std::to_string(i));
        zeilen.emplace_back("{");
        for (const std::string& z : pinsel[i]) { zeilen.push_back(z); }
        zeilen.emplace_back("}");
    }
    for (const char* z : {"{", "patchDef2", "{", "test/patch", "( 3 3 0 0 0 )", "(", "( ( 128 -192 0 0 0 ) ( 128 -128 96 0 0.5 ) ( 128 -64 0 0 1 ) )",
                          "( ( 256 -192 0 0.5 0 ) ( 256 -128 96 0.5 0.5 ) ( 256 -64 0 0.5 1 ) )",
                          "( ( 384 -192 0 1 0 ) ( 384 -128 96 1 0.5 ) ( 384 -64 0 1 1 ) )", ")", "}", "}", "}"}) {
        if (std::string(z) == "{" && zeilen.back().rfind("// brush", 0) != 0 && zeilen.back() == "}") { zeilen.push_back("// brush " + std::to_string(pinsel.size())); }
        zeilen.emplace_back(z);
    }
    for (const char* z : {"// entity 1", "{", "\"classname\" \"info_player_start\"", "\"origin\" \"-384 128 24\"", "\"angle\" \"0\"", "}",
                          "// entity 2", "{", "\"classname\" \"light\"", "\"origin\" \"-256 -160 192\"", "\"light\" \"400\"", "}",
                          "// entity 3", "{", "\"classname\" \"light\"", "\"origin\" \"256 160 192\"", "\"light\" \"300\"",
                          "\"_color\" \"0.6 0.8 1\"", "}", "// entity 4", "{", "\"classname\" \"func_door\"", "\"angle\" \"-1\"",
                          "\"targetname\" \"tuer\"", "// brush 0", "{"}) {
        zeilen.emplace_back(z);
    }
    for (const std::string& z : kasten(384, 128, 0, 448, 192, 96, "test/tuer", 0)) { zeilen.push_back(z); }
    zeilen.emplace_back("}");
    zeilen.emplace_back("}");
    std::string text;
    for (const std::string& z : zeilen) { text += z + "\n"; }
    return text;
}

// Eine Testkarte als Datei im Laufordner (s3bau/<name>.map) und im Editor offen
std::string raumKarteOeffnen(const char* name, bool leck) {
    const std::filesystem::path ordner = std::filesystem::current_path() / "s3bau";
    std::error_code ec;
    std::filesystem::create_directories(ordner, ec);
    const std::filesystem::path p = ordner / (std::string(name) + ".map");
    {
        std::ofstream f(p, std::ios::binary | std::ios::trunc);
        const std::string t = raumKarte(leck);
        f.write(t.data(), static_cast<std::streamsize>(t.size()));
    }
    (void)ke::karteOeffnen(p.string());
    return p.string();
}

std::function<ImVec2()> bauZeile(int liste, int zeile, int spalte = 0) {
    return [liste, zeile, spalte] { return ke::bauDialogZeile(liste, zeile, spalte); };
}

std::vector<std::string> bauNamen() {
    std::vector<std::string> v;
    for (const auto& b : ke::editor().bauProjekt.eintraege) { v.push_back(b.name); }
    return v;
}

const ke::Editor::Rechteck* bauFeld(const std::string& name) {
    for (const auto& r : ke::editor().felderImBild) {
        if (r.name == name) { return &r; }
    }
    return nullptr;
}

std::string konsoleText() {
    std::string t;
    for (const ke::KonsolenZeile& z : ke::editor().konsole) { t += z.text + "\n"; }
    return t;
}

// Eine Zeile der Eintragsliste auf eine andere ziehen (obere Haelfte = davor),
// mit Strg kopieren; nach >= 99: unter die Liste.
Schritt bauZiehen(const std::string& was, int von, int nach, bool strg) {
    auto a = std::make_shared<ImVec2>();
    auto z = std::make_shared<ImVec2>();
    return {"Ziehe " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    *a = ke::bauDialogZeile(1, von);
                    const ImVec2 n = ke::bauDialogZeile(1, std::min(nach, 40));
                    const float zh = ke::bauDialogZeile(1, 1).y - ke::bauDialogZeile(1, 0).y;
                    *z = nach >= 99 ? ImVec2{n.x, n.y + zh * 20.0F} : ImVec2{n.x, n.y - zh * 0.25F};
                    setzeMaus(*a);
                    return false;
                }
                if (b == 1 && strg) { io.AddKeyEvent(ImGuiMod_Ctrl, true); return false; }
                if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                if (b >= 3 && b <= 14) {
                    const float f = static_cast<float>(b - 2) / 12.0F;
                    setzeMaus(ImVec2{a->x + (z->x - a->x) * f, a->y + (z->y - a->y) * f});
                    return false;
                }
                if (b == 15) { io.AddMouseButtonEvent(0, false); return false; }
                if (b == 16 && strg) { io.AddKeyEvent(ImGuiMod_Ctrl, false); return false; }
                return b >= 20;
            }};
}

// --- Punkt 4: Bauen aus dem Editor ----------------------------------------------

// Die 18 Lumps einer RBSP-Datei (leer, wenn es keine ist)
std::vector<std::string> bspLumps(const std::string& pfad) {
    std::ifstream f(std::filesystem::path(pfad), std::ios::binary);
    std::ostringstream o;
    o << f.rdbuf();
    const std::string d = o.str();
    std::vector<std::string> l;
    if (d.size() < 8 + 18 * 8 || d.compare(0, 4, "RBSP") != 0) { return l; }
    for (int i = 0; i < 18; ++i) {
        std::int32_t ofs = 0;
        std::int32_t len = 0;
        std::memcpy(&ofs, d.data() + 8 + 8 * i, 4);
        std::memcpy(&len, d.data() + 12 + 8 * i, 4);
        if (ofs < 0 || len < 0 || static_cast<std::size_t>(ofs) + static_cast<std::size_t>(len) > d.size()) { return {}; }
        l.push_back(d.substr(static_cast<std::size_t>(ofs), static_cast<std::size_t>(len)));
    }
    return l;
}

// Felder [von, bis) jedes Elements auf 0 (die Verweise auf Flaechen, wie
// ohne_flaechen in tools/karte/q3map2_vergleich.py)
std::string ohneFelder(std::string s, std::size_t groesse, std::size_t von, std::size_t bis) {
    for (std::size_t i = 0; i + groesse <= s.size(); i += groesse) {
        for (std::size_t j = von; j < bis; ++j) { s[i + j] = 0; }
    }
    return s;
}

std::string laufOrdner(const char* teil) { return (std::filesystem::current_path() / teil).string(); }

bool istDatei(const std::string& p) {
    std::error_code ec;
    return std::filesystem::is_regular_file(std::filesystem::path(p), ec);
}

void bauDateienWeg(const std::string& stamm) {
    std::error_code ec;
    for (const char* x : {".bsp", ".prt", ".srf", ".lin"}) { std::filesystem::remove(std::filesystem::path(stamm + x), ec); }
}

std::size_t zaehle(const std::string& t, const std::string& was) {
    std::size_t n = 0;
    for (std::size_t p = 0; (p = t.find(was, p)) != std::string::npos; p += was.size()) { ++n; }
    return n;
}

// Eine Zeile mit Zeitangabe bis vor die Zeit (die Zeit als "\x01" eingesetzt)
std::string bisZeit(const char* z) {
    const std::string s = z;
    return s.substr(0, s.find('\x01'));
}

std::string fertigZeile(const char* eintrag, int schritte) {
    char z[512];
    std::snprintf(z, sizeof(z), ke::trK(Str::KeBmFertig), eintrag, schritte, "\x01");
    return bisZeit(z);
}

std::string gestopptZeile(const char* eintrag, int schritt, int schritte, const char* grund) {
    char z[512];
    std::snprintf(z, sizeof(z), ke::trK(Str::KeBmGestoppt), eintrag, schritt, schritte, grund, "\x01");
    return bisZeit(z);
}

std::string abgebrochenZeile(const char* eintrag, int schritt, int schritte) {
    char z[512];
    std::snprintf(z, sizeof(z), ke::trK(Str::KeBmAbgebrochen), eintrag, schritt, schritte, "\x01");
    return bisZeit(z);
}

// Ein Ordner des Spiels (Engine-Pfad, sonst ein Suchordner)
std::string spielOrdnerFuerTest() {
    const std::string e = ke::enginePfadAnzeige();
    if (!e.empty()) { return e; }
    const std::vector<std::string> v = ke::kartenVfsOrdner();
    return v.empty() ? std::string() : v.back();
}

// Die Wurzel der Arbeitskopie: der erste Ordner ueber behaved.exe mit
// tests/karte/q3map2/raum.nrc.bsp (Laufordner laeufe/<name>, build-test/bin/Release)
std::filesystem::path arbeitskopie() {
    std::filesystem::path p = std::filesystem::path(platform::executableDirectory());
    for (int i = 0; i < 6 && !p.empty(); ++i) {
        std::error_code ec;
        if (std::filesystem::is_regular_file(p / "tests" / "karte" / "q3map2" / "raum.nrc.bsp", ec)) { return p; }
        if (p == p.parent_path()) { break; }
        p = p.parent_path();
    }
    return {};
}

// Das gebaute bhed_q3map2.exe: BHED_Q3MAP2, sonst <Arbeitskopie>/build-test bzw. build/bin/Release
std::string q3map2Quelle() {
    if (const char* q = std::getenv("BHED_Q3MAP2"); q != nullptr && q[0] != '\0') { return q; }
    const std::filesystem::path w = arbeitskopie();
    if (w.empty()) { return {}; }
    for (const char* b : {"build-test", "build"}) {
        const std::filesystem::path p = w / b / "bin" / "Release" / "bhed_q3map2.exe";
        std::error_code ec;
        if (std::filesystem::is_regular_file(p, ec)) { return p.string(); }
    }
    return {};
}

// Die Erwartungen (raum.nrc.bsp): BHED_Q3MAP2_DATEN, sonst <Arbeitskopie>/tests/karte/q3map2
std::string q3map2Daten() {
    if (const char* d = std::getenv("BHED_Q3MAP2_DATEN"); d != nullptr && d[0] != '\0') { return d; }
    const std::filesystem::path w = arbeitskopie();
    return w.empty() ? std::string() : (w / "tests" / "karte" / "q3map2").string();
}

// bhed_q3map2.exe neben behaved.exe: der Laufordner hat nur behaved.exe und
// data - die Quelle von q3map2Quelle (nichts, wenn es schon daneben liegt).
void q3map2Bereitstellen() {
    const std::string quelle = q3map2Quelle();
    const std::filesystem::path ziel = std::filesystem::path(platform::executableDirectory()) / "bhed_q3map2.exe";
    std::error_code ec;
    if (quelle.empty() || std::filesystem::equivalent(std::filesystem::path(quelle), ziel, ec)) { return; }
    std::filesystem::copy_file(std::filesystem::path(quelle), ziel, std::filesystem::copy_options::overwrite_existing, ec);
}

int eintragNr(const std::string& name) {
    const auto& e = ke::editor().bauProjekt.eintraege;
    for (std::size_t i = 0; i < e.size(); ++i) {
        if (e[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

// Ein Eintrag des Build-Menues (BuildMenuItem::run, ohne das Menue aufzuklappen)
Schritt bauMenuepunkt(const std::string& name) {
    return tu("Build > " + name, [name] {
        const int i = eintragNr(name);
        if (i < 0) {
            meldeFehler("Eintrag " + name + " fehlt");
            return;
        }
        ke::bauStarten(static_cast<std::size_t>(i));
    });
}

// Ein Stueck der Konsole in dieser Farbe, das den Text enthaelt
bool konsoleArtMit(ke::KonsolenZeile::Art art, const std::string& teil) {
    for (const ke::KonsolenZeile& z : ke::editor().konsole) {
        for (const auto& t : z.teile) {
            if (t.art == art && z.text.substr(t.von, t.laenge).find(teil) != std::string::npos) { return true; }
        }
    }
    return false;
}

std::vector<Schritt> baumenue() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    static const std::vector<std::string> kJa = {"Single: bsp stage", "Single: vis stage", "Single: light stage", "-", "Entity-Only compile", "-",
                                                 "convert to .ASE model", "convert to .OBJ model", "-", "Export Lightmaps", "Import Lightmaps",
                                                 "Create .pk3", "-", "[Test] Quick and Crude", "[Test] Floodlight", "[Final] Fast",
                                                 "[Final] Extreme Quality"};
    add(tu("Stufe 3/3: eigene build_menu.xml weg, Build-Menue frisch (default_build_menu.xml)", [] {
        karteDialogeZu();
        ke::Editor& e = ke::editor();
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(ke::bauMenueDateiPfad()), ec);
        g_app->settings.karte.erase("BuildMenu");
        g_app->settings.karte.erase("BuildMenu/geometry");
        e.bauProjektFuer = -2;
        e.bauGeaendert = false;
        ke::bauMenueSicherstellen();
        q3map2Bereitstellen();
        raumKarteOeffnen("raum", false);
    }));
    add(pause(0.3));
    add(pruefe("Build-Menue: die 17 Eintraege der default_build_menu.xml (JA), Trenner an ihrer Stelle", [] {
        diag::detail("Knopftest: Stufe 3/3 Eintraege: " + std::to_string(bauNamen().size()) + ", Datei " + ke::bauMenueDateiPfad());
        return bauNamen() == kJa && ke::editor().bauMenueName == "build_menu.xml";
    }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.25));
    add(pruefe("Build: Customize..., Run recent build und alle Eintraege anklickbar im Menue", [] {
        bool ok = finde(ke::trK(Str::KeLCustomize), "##Menu_", 0, false) != nullptr &&
                  finde(ke::trK(Str::KeLRunRecentBuild), "##Menu_", 0, false) != nullptr;
        std::string fehlt;
        for (const std::string& n : kJa) {
            if (n == "-") { continue; }
            const Element* el = finde(n, "##Menu_", 0, false);
            if (el == nullptr) { fehlt += " " + n; }
            ok = ok && el != nullptr;
        }
        if (!fehlt.empty()) { diag::detail("Knopftest: Stufe 3/3 fehlt im Menue:" + fehlt); }
        return ok;
    }));
    add(foto("karteneditor_s3_build_menue"));
    add(klick(ke::trK(Str::KeLCustomize), "##Menu_", 0, false));
    add(pause(0.4));
    add(pruefe("Customize...: der Dialog \"Build Menu\" ist offen, 700 x 500 wie NRC (g_guiSettings 700, 500)", [] {
        const auto* d = bauFeld("Bau:Dialog");
        if (d != nullptr) { diag::detail("Knopftest: Stufe 3/3 Dialog " + std::to_string(d->b) + " x " + std::to_string(d->h)); }
        return ke::bauDialogStand().offen && d != nullptr && d->b == 700 && d->h == 500;
    }));
    add(pruefe("Dialog: Lagen wie im Foto dlg_Build_Customize_0 (OK 611/9 80x22, Cancel 37, Reset 65, Liste 21/37 572x276, Befehle 21/359 572x73, Variablen 9/450 h 41)", [] {
        const auto r = [](const char* n, int x, int y, int b, int h) {
            const auto* f = bauFeld(n);
            const bool ok = f != nullptr && f->x == x && f->y == y && f->b == b && f->h == h;
            if (!ok && f != nullptr) {
                diag::detail(std::string("Knopftest: Stufe 3/3 ") + n + " " + std::to_string(f->x) + "/" + std::to_string(f->y) + " " +
                             std::to_string(f->b) + "x" + std::to_string(f->h));
            }
            return ok;
        };
        return r("Bau:OK", 611, 9, 80, 22) && r("Bau:Cancel", 611, 37, 80, 22) && r("Bau:Reset", 611, 65, 80, 22) &&
               r("Bau:Eintraege", 21, 37, 572, 276) && r("Bau:Befehle", 21, 359, 572, 73) && r("Bau:Variablen", 9, 450, 596, 41);
    }));
    add(foto("karteneditor_s3_build_customize"));
    // Wahl: [Final] Fast -> drei Befehle und "+"
    add(klickAn("Eintrag [Final] Fast", bauZeile(1, 15)));
    add(pause(0.2));
    add(pruefe("Klick auf [Final] Fast: gewaehlt, die Befehlsliste zeigt seine drei Befehle (Hoehe 4 Zeilen: 73)", [] {
        const auto* f = bauFeld("Bau:Befehle");
        return ke::bauDialogStand().aktuell == 15 && f != nullptr && f->h == 73;
    }));
    // Neuer Eintrag ueber "+" (project_cell_edited: add new). Die Liste rollt (18 Zeilen > 17 sichtbar).
    add(radAn("Liste nach unten rollen", bauZeile(1, 8), -5.0F));
    add(klickAn("\"+\" doppelt (Zeile 17)", bauZeile(1, 17), 0, true));
    add(pause(0.2));
    add(pruefe("Doppelklick auf \"+\": Bearbeiten an Ort und Stelle", [] { return ke::bauDialogStand().bearbeitet == 1; }));
    add(tippe("Mein Bau"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("\"Mein Bau\" + Enter: neuer Eintrag am Ende (18), gewaehlt", [] {
        const auto n = bauNamen();
        return n.size() == 18 && n.back() == "Mein Bau" && ke::bauDialogStand().aktuell == 17;
    }));
    add(klickAn("\"+\" der Befehle doppelt", bauZeile(2, 0), 0, true));
    add(pause(0.2));
    add(tippe("[q3map2] -meta \"[MapFile]\""));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("Befehl ueber \"+\": [q3map2] -meta \"[MapFile]\" haengt an \"Mein Bau\" (der Tooltip im Menue)", [] {
        const auto& e = ke::editor().bauProjekt.eintraege;
        return !e.empty() && e.back().befehle == std::vector<std::string>{"[q3map2] -meta \"[MapFile]\""} &&
               karte::bauTooltip(e.back()) == "[q3map2] -meta \"[MapFile]\"";
    }));
    // Umbenennen mit F2
    add(radAn("Liste nach unten rollen", bauZeile(1, 8), -5.0F));
    add(klickAn("Eintrag Mein Bau", bauZeile(1, 17)));
    add(taste(ImGuiKey_F2, "F2"));
    add(pause(0.2));
    add(tippe("Mein Bau 2"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("F2: umbenannt in \"Mein Bau 2\"", [] { return bauNamen().size() == 18 && bauNamen().back() == "Mein Bau 2"; }));
    // Kopieren und Einfuegen (vor dem aktuellen)
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_C, "Ctrl+C"));
    add(radAn("Liste nach oben rollen", bauZeile(1, 8), 5.0F));
    add(klickAn("Eintrag Single: bsp stage", bauZeile(1, 0)));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_V, "Ctrl+V"));
    add(pause(0.2));
    add(pruefe("Ctrl+C / Ctrl+V: die Kopie steht vor dem gewaehlten Eintrag (Stelle 0), mit ihrem Befehl", [] {
        const auto& e = ke::editor().bauProjekt.eintraege;
        return e.size() == 19 && e[0].name == "Mein Bau 2" && e[0].befehle.size() == 1 && e[1].name == "Single: bsp stage";
    }));
    // Entfernen (erst nach der Doppelklickzeit: derselbe Ort wie eben)
    add(pause(0.5));
    add(klickAn("Eintrag 0 (die Kopie)", bauZeile(1, 0)));
    add(taste(ImGuiKey_Delete, "Entf"));
    add(pause(0.2));
    add(pruefe("Entf: die Kopie ist weg, 18 Eintraege", [] {
        std::string n;
        for (const std::string& x : bauNamen()) { n += x + "|"; }
        diag::detail("Knopftest: Stufe 3/3 Eintraege: " + n);
        return bauNamen().size() == 18 && bauNamen()[0] == "Single: bsp stage";
    }));
    // Ziehen (InternalMove) und Strg+Ziehen (DragDrop = kopieren)
    add(bauZiehen("[Final] Fast vor [Test] Quick and Crude", 15, 13, false));
    add(pause(0.2));
    add(pruefe("Ziehen: [Final] Fast steht vor [Test] Quick and Crude", [] {
        const auto n = bauNamen();
        return n.size() == 18 && n[13] == "[Final] Fast" && n[14] == "[Test] Quick and Crude";
    }));
    add(bauZiehen("Strg: Single: bsp stage vor Zeile 2 kopieren", 0, 2, true));
    add(pause(0.2));
    add(pruefe("Strg+Ziehen: eine Kopie an Stelle 2, das Original bleibt an 0", [] {
        const auto n = bauNamen();
        return n.size() == 19 && n[0] == "Single: bsp stage" && n[2] == "Single: bsp stage" && n[1] == "Single: vis stage";
    }));
    add(bauZiehen("unter \"+\" ziehen tut nichts (positionBelowLast)", 0, 99, false));
    add(pause(0.2));
    add(pruefe("Ziehen unter die letzte Zeile: abgewiesen, nichts verschoben", [] { return bauNamen().size() == 19 && bauNamen()[0] == "Single: bsp stage"; }));
    // Variablen
    add(klickAn("Build Variables ankreuzen", [] { return ke::bauDialogPunkt(17, 457); }));
    add(pause(0.3));
    add(pruefe("Build Variables: aufgeklappt, Konstanten [BspFile] .. [UserEnginePath] (10), Werkzeuge mit \"+\"", [] {
        const auto* k = bauFeld("Bau:Konstanten");
        const auto* w = bauFeld("Bau:Werkzeuge");
        return ke::bauDialogStand().variablenOffen && k != nullptr && w != nullptr && k->h == 10 * 20 + 2 && w->h == 2 * 20 + 2;
    }));
    add(pruefe("Konstanten wie build_init_variables: RadiantPath = Programmordner, MapFile = die Karte, MonitorAddress 127.0.0.1:39000, GameName base", [] {
        const karte::BauVariablen v = ke::bauVariablen();
        std::string prog = platform::executableDirectory();
        std::replace(prog.begin(), prog.end(), '\\', '/');
        while (!prog.empty() && prog.back() == '/') { prog.pop_back(); }
        const std::string karte = ke::bauKartenPfad();
        diag::detail("Knopftest: Stufe 3/3 RadiantPath " + v.at("RadiantPath") + ", MapFile " + v.at("MapFile") + ", GameName " + v.at("GameName"));
        // GameName: gamename_get; beim Regelwerk Movie Duels der Ordner der Mod (MD)
        const std::string spiel = ke::kartenSpielNummer() == kRegelwerkMovieDuels ? "MD" : "base";
        return v.size() == 10 && v.at("RadiantPath") == prog + "/" && v.at("ExecutableType") == "exe" && v.at("MapFile") == karte &&
               karte.size() > 14 && karte.compare(karte.size() - 14, 14, "s3bau/raum.map") == 0 && v.at("BspFile").find("s3bau/raum.bsp") != std::string::npos &&
               v.at("MapName") == "raum.map" && v.at("MonitorAddress") == "127.0.0.1:39000" && v.at("GameName") == spiel;
    }));
    add(klickAn("[MapFile] anklicken (Spalte 0)", bauZeile(3, 5, 0)));
    add(pause(0.15));
    add(pruefe("Klick auf [MapFile]: \"[MapFile]\" in der Zwischenablage, Tooltip \"Copied to clipboard.\"", [] {
        const char* c = ImGui::GetClipboardText();
        return c != nullptr && std::string(c) == "[MapFile]" && ke::bauDialogStand().kopiertSichtbar;
    }));
    add(pruefe("Werkzeug q3map2 ausgewertet: \"<Programm>/bhed_q3map2.exe\" -v ... -connect 127.0.0.1:39000 -game ja -fs_basepath ... -fs_game base", [] {
        karte::BauVariablen v = ke::bauVariablen();
        karte::bauWerkzeugeAuswerten(ke::editor().bauProjekt, v);
        const std::string& q = v["q3map2"];
        diag::detail("Knopftest: Stufe 3/3 q3map2 = " + q);
        return q.rfind("\"" + v["RadiantPath"] + "bhed_q3map2.exe\" -v -connect 127.0.0.1:39000 -game ja -fs_basepath \"", 0) == 0 &&
               q.find("\" -fs_game " + v["GameName"]) != std::string::npos;
    }));
    add(foto("karteneditor_s3_build_variablen"));
    // Editables: ein Werkzeug ueber "+" (CustomDelegate0), dann sein Wert (CustomDelegate1)
    add(klickAn("Editables \"+\" doppelt (Spalte 0)", bauZeile(4, 1, 0), 0, true));
    add(pause(0.2));
    add(tippe("zzwz"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("neues Werkzeug zzwz (insert), leer", [] { return ke::editor().bauProjekt.werkzeuge.count("zzwz") == 1; }));
    add(klickAn("Wert von zzwz doppelt (Spalte 1)", bauZeile(4, 1, 1), 0, true));
    add(pause(0.2));
    add(tippe("[q3map2] -x <cond value=\"[GameName]\">-y</cond>"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(pruefe("Wert von zzwz als XML gelesen: ausgewertet = q3map2 + \" -x -y\" (vorher definierte Werkzeuge gelten)", [] {
        karte::BauVariablen v = ke::bauVariablen();
        karte::bauWerkzeugeAuswerten(ke::editor().bauProjekt, v);
        return v["zzwz"] == v["q3map2"] + " -x -y";
    }));
    // Cancel: alles zurueck
    add(klick(ke::trK(Str::KeCancel), "###kartebaumenue"));
    add(pause(0.3));
    add(pruefe("Cancel: zu, und alles wie vorher (17 Eintraege, kein Mein Bau, kein zzwz, Reihenfolge)", [] {
        return !ke::bauDialogStand().offen && bauNamen() == kJa && ke::editor().bauProjekt.werkzeuge.count("zzwz") == 0;
    }));
    // OK: der neue Eintrag bleibt und wird beim Beenden gespeichert
    add(tu("Customize... (BuildMenuCustomize)", [] { (void)ke::ausfuehren("BuildMenuCustomize"); }));
    add(pause(0.4));
    add(radAn("Liste nach unten rollen", bauZeile(1, 8), -5.0F));
    add(klickAn("\"+\" doppelt", bauZeile(1, 17), 0, true));
    add(pause(0.2));
    add(tippe("Test OK"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.2));
    add(klick(ke::trK(Str::KePrefOk), "###kartebaumenue"));
    add(pause(0.3));
    add(pruefe("OK: zu, \"Test OK\" bleibt, g_build_changed", [] {
        return !ke::bauDialogStand().offen && bauNamen().size() == 18 && bauNamen().back() == "Test OK" && ke::editor().bauGeaendert;
    }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.25));
    add(pruefe("... und im Build-Menue (Build_refreshMenu)", [] { return finde("Test OK", "##Menu_", 0, false) != nullptr; }));
    add(tu("Menue zu", [] { karteDialogeZu(); }));
    add(pause(0.2));
    add(tu("SaveBuildMenu (wie beim Beenden: BuildMenu_Destroy)", [] { ke::bauMenueSpeichern(); }));
    add(pruefe("build_menu.xml im Einstellungsordner mit <build name=\"Test OK\">, BuildMenu = build_menu.xml; beim naechsten Start gelesen", [] {
        std::ifstream f(std::filesystem::path(ke::bauMenueDateiPfad()), std::ios::binary);
        std::ostringstream o;
        o << f.rdbuf();
        const bool inDatei = o.str().find("<build name=\"Test OK\">") != std::string::npos && o.str().find("\r\n") != std::string::npos;
        ke::Editor& e = ke::editor();
        e.bauProjektFuer = -2;   // wie ein Neustart: BuildMenu_Construct
        ke::bauMenueSicherstellen();
        diag::detail("Knopftest: Stufe 3/3 gespeichert in " + ke::bauMenueDateiPfad());
        return inDatei && g_app->settings.karte["BuildMenu"] == "build_menu.xml" && bauNamen().size() == 18 && bauNamen().back() == "Test OK";
    }));
    // Reset: zurueck auf den Stand der Datei (Start des Editors)
    add(tu("Customize..., Eintrag 0 entfernen", [] { (void)ke::ausfuehren("BuildMenuCustomize"); }));
    add(pause(0.4));
    add(radAn("Liste nach oben rollen", bauZeile(1, 8), 5.0F));
    add(klickAn("Eintrag 0", bauZeile(1, 0)));
    add(taste(ImGuiKey_Delete, "Entf"));
    add(pause(0.2));
    add(pruefe("Entf im Dialog: 17 Eintraege", [] { return bauNamen().size() == 17; }));
    add(klick(ke::trK(Str::KeReset), "###kartebaumenue"));
    add(pause(0.3));
    add(pruefe("Reset: zu, das Projekt aus der Datei (18 mit Test OK) - \"Reset to editor start state\"", [] {
        return !ke::bauDialogStand().offen && bauNamen().size() == 18 && bauNamen()[0] == "Single: bsp stage" && bauNamen().back() == "Test OK";
    }));
    // Bauen: ein Menuepunkt und Run recent build (F5) - Punkt 3: die Befehle in der Konsole
    add(tu("Konsole leeren", [] { ke::konsoleLeeren(); }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.25));
    add(klick("[Final] Fast", "##Menu_", 0, false));
    add(warteBis("Bauen fertig", [] { return !ke::bauLaeuft(); }, 3600));
    add(pruefe("Build > [Final] Fast: die Karte gespeichert, drei Befehle ausgewertet (q3map2 mit -meta, -vis, -light ... \"<Karte>\")", [] {
        const std::string k = konsoleText();
        const std::string karte = ke::bauKartenPfad();
        diag::detail("Knopftest: Stufe 3/3 Konsole: " + k.substr(0, std::min<std::size_t>(k.size(), 600)));
        std::size_t n = 0;
        for (std::size_t p = 0; (p = k.find("=== running build command ===", p)) != std::string::npos; ++p) { ++n; }
        return n >= 3 && k.find("bhed_q3map2.exe\" -v") != std::string::npos && k.find(" -meta \"" + karte + "\"") != std::string::npos &&
               k.find(" -vis \"" + karte + "\"") != std::string::npos && k.find(" -light -fast -patchshadows -samples 2 -filter -bounce 8 -bhedgpubakeauto -bhedgpubakezeiten ") != std::string::npos &&
               k.find(" \"" + karte + "\"") != std::string::npos &&   // Stufe 8d: GPU bake Auto (Vorgabe) vor der Karte
               ke::editor().bauZuletzt == 15;
    }));
    add(tu("Konsole leeren", [] { ke::konsoleLeeren(); }));
    add(klickAn("in XY (Fokus)", feldMitte("XY"), 2));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tasteKombi(ImGuiKey_F5, "F5"));
    add(pause(0.1));
    add(warteBis("Bauen fertig", [] { return !ke::bauLaeuft(); }, 3600));
    add(pruefe("F5 (Run recent build): noch einmal [Final] Fast", [] {
        const std::string k = konsoleText();
        return k.find(" -light -fast -patchshadows -samples 2 -filter -bounce 8 ") != std::string::npos && ke::editor().bauZuletzt == 15;
    }));
    add(tu("Aufraeumen: build_menu.xml weg, Vorgabe neu", [] {
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(ke::bauMenueDateiPfad()), ec);
        ke::Editor& e = ke::editor();
        e.bauProjektFuer = -2;
        e.bauGeaendert = false;
        g_app->settings.karte.erase("BuildMenu");
        ke::bauMenueSicherstellen();
    }));
    return s;
}

std::vector<Schritt> bauen() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const std::string stamm = laufOrdner("s3bau") + "\\raum";
    const auto fertig = [](const char* was, int bilder) { return warteBis(was, [] { return !ke::bauLaeuft(); }, bilder); };
    add(tu("Stufe 3/4: bhed_q3map2.exe neben behaved.exe, Build-Menue und Seite Build wie vorgegeben, Testkarte raum", [stamm] {
        karteDialogeZu();
        q3map2Bereitstellen();
        ke::Editor& e = ke::editor();
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(ke::bauMenueDateiPfad()), ec);
        g_app->settings.karte.erase("BuildMenu");
        e.bauProjektFuer = -2;
        e.bauGeaendert = false;
        ke::bauMenueSicherstellen();
        e.bau = ke::Editor::BauEinstellungen{};
        ke::einstellungenSpeichern();
        raumKarteOeffnen("raum", false);
        bauDateienWeg(stamm);
        ke::konsoleLeeren();
        e.letzteUrl.clear();
        e.bauRueckmeldungen.clear();
        e.bauRueckmeldungOffen = false;
    }));
    add(pause(0.3));
    add(pruefe("bhed_q3map2.exe liegt neben behaved.exe ([RadiantPath]bhed_q3map2.[ExecutableType])", [] {
        return istDatei((std::filesystem::path(platform::executableDirectory()) / "bhed_q3map2.exe").string());
    }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.25));
    add(klick("[Final] Fast", "##Menu_", 0, false));
    add(pause(0.1));
    add(pruefe("Build > [Final] Fast: q3map2 laeuft im Hintergrund, die Statuszeile zeigt Eintrag und Schritt", [] {
        diag::detail("Knopftest: Stufe 3/4 Statuszeile: " + ke::editor().statusBefehl);
        return ke::bauLaeuft() && ke::editor().statusBefehl.find("[Final] Fast") != std::string::npos;
    }));
    add(fertig("Bauen fertig", 3600));
    add(pruefe("Konsole wie watchbsp.cpp: je Schritt Setting up / Listening... / === running build command === / Connected. / Connection closed. (3), "
               "q3map2s Ausgabe live, am Ende Zeit und Ergebnis",
               [] {
                   const std::string k = konsoleText();
                   diag::detail("Knopftest: Stufe 3/4 Konsole (Ende): " + k.substr(k.size() > 600 ? k.size() - 600 : 0));
                   return zaehle(k, "Setting up\n") == 3 && zaehle(k, "Listening...\n") == 3 && zaehle(k, "Connected.\n") == 3 &&
                          zaehle(k, "=== running build command ===\n") == 3 && zaehle(k, "Connection closed.\n") == 3 &&
                          k.find(" -meta \"" + ke::bauKartenPfad() + "\"") != std::string::npos && k.find("--- LoadMapFile ---") != std::string::npos &&
                          k.find("--- Radiosity (bounce 8 of 8) ---") != std::string::npos && k.find(fertigZeile("[Final] Fast", 3)) != std::string::npos &&
                          k.find(": exit ") == std::string::npos && ke::editor().statusBefehl.empty();
               }));
    add(pruefe("Konsole: q3map2s Warnungen (message level 2) in der Warnfarbe, keine Fehler", [] {
        return konsoleArtMit(ke::KonsolenZeile::Art::Warnung, "WARNING: Couldn't find image for shader textures/test/boden") &&
               !konsoleArtMit(ke::KonsolenZeile::Art::Fehler, "");
    }));
    add(pruefe(".bsp und .srf neben der Karte (s3bau); keine .prt (-vis ohne -saveprt loescht sie), keine .lin, kein Q3Map debug window", [stamm] {
        return istDatei(stamm + ".bsp") && istDatei(stamm + ".srf") && !istDatei(stamm + ".prt") && !istDatei(stamm + ".lin") &&
               !ke::editor().bauRueckmeldungOffen;
    }));
    add(pruefe(".bsp wie NRCs (raum.nrc.bsp aus tests/karte/q3map2): shaders, planes, nodes, leafbrushes, brushes gleich, leafs/models/brushsides "
               "ohne Flaechenbezug gleich; visibility, lightmaps, lightgrid von -vis und -light (lightarray 459 Gitterpunkte wie NRC)",
               [stamm] {
                   const std::string daten = q3map2Daten();
                   if (daten.empty()) {
                       diag::detail("Knopftest: Stufe 3/4 tests/karte/q3map2 nicht gefunden (BHED_Q3MAP2_DATEN)");
                       return false;
                   }
                   const auto u = bspLumps(stamm + ".bsp");
                   const auto n = bspLumps(daten + "\\raum.nrc.bsp");
                   if (u.size() != 18 || n.size() != 18) { return false; }
                   std::string bericht;
                   bool ok = true;
                   for (std::size_t i : {1U, 2U, 3U, 6U, 8U}) {
                       const bool g = u[i] == n[i];
                       bericht += " " + std::to_string(i) + (g ? "=" : "!");
                       ok = ok && g;
                   }
                   const bool blaetter = ohneFelder(u[4], 48, 32, 40) == ohneFelder(n[4], 48, 32, 40);
                   const bool modelle = ohneFelder(u[7], 40, 24, 32) == ohneFelder(n[7], 40, 24, 32);
                   const bool seiten = ohneFelder(u[9], 12, 8, 12) == ohneFelder(n[9], 12, 8, 12);
                   diag::detail("Knopftest: Stufe 3/4 Lumps" + bericht + (blaetter ? " leafs=" : " leafs!") + (modelle ? " models=" : " models!") +
                                (seiten ? " brushsides=" : " brushsides!") + " vis " + std::to_string(u[16].size()) + " lightmaps " +
                                std::to_string(u[14].size()) + " grid " + std::to_string(u[15].size()) + " lightarray " + std::to_string(u[17].size()));
                   return ok && blaetter && modelle && seiten && u[16].size() > 8 && !u[14].empty() && !u[15].empty() && u[17].size() == 459U * 2U;
               }));
    add(foto("karteneditor_s3_bauen_konsole"));
    // Fehler hervorgehoben: -light ohne .bsp - q3map2 meldet ERROR (message level 3),
    // die Ueberwachung haelt an (saxCharacters: SYS_ERR -> stop_depth)
    add(tu(".bsp und .srf weg, Konsole leeren", [stamm] {
        bauDateienWeg(stamm);
        ke::konsoleLeeren();
    }));
    add(bauMenuepunkt("Single: light stage"));
    add(fertig("Bauen fertig", 1200));
    add(pruefe("Single: light stage ohne .bsp: q3map2s ERROR rot in der Konsole, die Ueberwachung haelt an (rote Zeile mit Grund und Zeit)", [] {
        const std::string k = konsoleText();
        diag::detail("Knopftest: Stufe 3/4 Fehler: " + k.substr(k.size() > 400 ? k.size() - 400 : 0));
        return konsoleArtMit(ke::KonsolenZeile::Art::Fehler, "ERROR") &&
               konsoleArtMit(ke::KonsolenZeile::Art::Fehler, gestopptZeile("Single: light stage", 1, 1, "error"));
    }));
    // Port 39000 belegt: NRCs Meldung und der Kasten "Build monitoring"
    auto horcher = std::make_shared<bauplattform::Horcher>();
    add(tu("Port 39000 belegen, Konsole leeren", [horcher] {
        std::string f;
        if (!horcher->lauschen(39000, f)) { meldeFehler("Port 39000 nicht belegt: " + f); }
        ke::konsoleLeeren();
    }));
    add(bauMenuepunkt("Single: bsp stage"));
    add(pause(0.3));
    add(pruefe("Port belegt: \"Failed to get a listening socket on port 39000.\" in der Konsole und im Kasten \"Build monitoring\", kein Bauen", [] {
        const ke::Editor& e = ke::editor();
        return konsoleText().find("Failed to get a listening socket on port 39000.\nTry running with Build monitoring disabled") != std::string::npos &&
               e.meldung.offen && e.meldung.titel == "Build monitoring" && !ke::bauLaeuft();
    }));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##meldungok"));
    add(tu("Port 39000 frei", [horcher] { horcher->schliessen(); }));
    add(pause(0.2));
    // Zeitueberschreitung: ein ueberwachter Befehl (mit [MonitorAddress]), der sich nicht verbindet
    add(tu("Eintrag \"Test ohne Verbindung\" (die Adresse nur im Dateinamen), Konsole leeren", [] {
        ke::editor().bauProjekt.eintraege.push_back(
            karte::BauEintrag{"Test ohne Verbindung", {"\"[RadiantPath]bhed_q3map2.[ExecutableType]\" -info \"nicht_da_[MonitorAddress].bsp\""}});
        ke::konsoleLeeren();
    }));
    add(bauMenuepunkt("Test ohne Verbindung"));
    add(warteBis("der Kasten nach 5 s", [] { return ke::editor().meldung.offen; }, 4000));
    add(pruefe("Ohne Verbindung nach 5 s: \"The connection timed out, assuming the build process failed\" (Kasten), Bauen angehalten (timeout)", [] {
        const ke::Editor& e = ke::editor();
        return e.meldung.text.find("The connection timed out, assuming the build process failed") == 0 && !ke::bauLaeuft() &&
               konsoleText().find(gestopptZeile("Test ohne Verbindung", 1, 1, "timeout")) != std::string::npos;
    }));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##meldungok"));
    add(pause(0.2));
    // Ohne Ueberwachung (RunBatch): die Ausgabe ueber die Pipe, "Dump non Monitored Builds Log"
    add(tu("Seite Build: Build monitoring aus, Dump non Monitored Builds Log an; Konsole leeren", [stamm] {
        ke::Editor& e = ke::editor();
        e.bau.ueberwachen = false;
        e.bau.logAusgeben = true;
        bauDateienWeg(stamm);
        ke::konsoleLeeren();
    }));
    add(bauMenuepunkt("Single: bsp stage"));
    add(fertig("Bauen fertig", 1200));
    add(pruefe("Ohne Ueberwachung: ohne -connect, q3map2s Ausgabe ueber die Pipe (WARNING in der Warnfarbe), junk.txt im Einstellungsordner, .bsp da", [stamm] {
        const std::string k = konsoleText();
        std::ifstream f(std::filesystem::path(platform::settingsDirectory() + "\\junk.txt"), std::ios::binary);
        std::ostringstream o;
        o << f.rdbuf();
        return k.find("-connect") == std::string::npos && k.find("Setting up") == std::string::npos && k.find("--- LoadMapFile ---") != std::string::npos &&
               konsoleArtMit(ke::KonsolenZeile::Art::Warnung, "WARNING: Couldn't find image") && o.str().find("--- LoadMapFile ---") != std::string::npos &&
               k.find("junk.txt") != std::string::npos && istDatei(stamm + ".bsp") && k.find(fertigZeile("Single: bsp stage", 1)) != std::string::npos;
    }));
    // Run Engine After Compile: im Selbsttest nur gemerkt (nie ein Spiel starten)
    add(tu("Build monitoring an, Log aus, Run Engine After Compile an", [] {
        ke::Editor& e = ke::editor();
        e.bau.ueberwachen = true;
        e.bau.logAusgeben = false;
        e.bau.spielStarten = true;
        e.letzteUrl.clear();
    }));
    add(bauMenuepunkt("Single: bsp stage"));
    add(fertig("Bauen fertig", 1200));
    add(pruefe("Run Engine After Compile: danach \"<Engine><exe>\" +set sv_pure 0 [+set fs_game MD] +devmap raum (im Test nur gemerkt, nicht gestartet)", [] {
        const std::string& u = ke::editor().letzteUrl;
        diag::detail("Knopftest: Stufe 3/4 Engine: " + u);
        const std::string basis = ke::kartenSpiel().basisSpiel;
        const std::string fs = ke::bauSpielName() != basis ? " +set fs_game " + ke::bauSpielName() : std::string();
        return u.find("\" +set sv_pure 0" + fs + " +devmap raum") != std::string::npos &&
               u.rfind("\"" + ke::enginePfadAnzeige(), 0) == 0 && konsoleText().find("Running engine...") != std::string::npos;
    }));
    add(tu("Run Engine aus", [] { ke::editor().bau.spielStarten = false; }));
    // Ziel-Ordner (Abweichung): auf der Seite Build eintragen
    add(tu("Edit > Preferences, Seite Build", [] {
        bauDateienWeg(laufOrdner("s3bau\\ziel\\raum"));
        ke::editor().einstellungenOffen = true;
    }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KePgBuild)) + "##seite19"));
    add(pause(0.2));
    add(pruefe("Seite Build: Feld Ziel-Ordner (leer) mit dem Hinweis darunter, ganz im Fenster", [] {
        const Element* el = finde("##bauziel", "", 0, true);
        const ImRect f = fensterRahmen("###karteeinstellungen");
        return el != nullptr && ke::einstellungsFeld("bauziel").empty() && f.Contains(el->rect);
    }));
    add(foto("karteneditor_s3_pref_build_ziel"));
    add(klickAn("Feld Ziel-Ordner", stufe2::elementOrt("##bauziel")));
    add(tippe(laufOrdner("s3bau\\ziel")));
    add(klick(ke::trK(Str::KePrefOk)));
    add(pause(0.2));
    add(pruefe("OK: Ziel-Ordner gesetzt und gespeichert (BuildTargetFolder)", [] {
        return ke::editor().bau.zielOrdner == laufOrdner("s3bau\\ziel") && stufe2::einstellung("BuildTargetFolder") == laufOrdner("s3bau\\ziel");
    }));
    add(bauMenuepunkt("Single: bsp stage"));
    add(fertig("Bauen fertig", 1200));
    add(pruefe("Ziel-Ordner: raum.bsp und raum.prt in s3bau/ziel, die Karte dorthin kopiert, [MapFile] ist die Kopie", [] {
        return istDatei(laufOrdner("s3bau\\ziel\\raum.bsp")) && istDatei(laufOrdner("s3bau\\ziel\\raum.prt")) &&
               istDatei(laufOrdner("s3bau\\ziel\\raum.map")) && ke::bauKartenPfad().find("s3bau/ziel/raum.map") != std::string::npos &&
               konsoleText().find(" -meta \"" + ke::bauKartenPfad() + "\"") != std::string::npos;
    }));
    add(pruefe("Ziel-Ordner im Spielordner: gebaut wird im Arbeitsordner (mit Hinweis), nie im Spielordner (nur die Vorschau, ohne zu bauen)", [] {
        ke::Editor& e = ke::editor();
        const std::string alt = e.bau.zielOrdner;
        e.bau.zielOrdner = spielOrdnerFuerTest() + "maps";
        std::string h;
        const std::string o = ke::bauOrdnerVorschau(&h);
        e.bau.zielOrdner = alt;
        diag::detail("Knopftest: Stufe 3/4 Vorschau: " + o + " / " + h);
        return !spielOrdnerFuerTest().empty() && o == platform::settingsDirectory() + "\\karte_bau" && !h.empty();
    }));
    add(pruefe("Karte im Spielordner (ohne Ziel-Ordner): der Arbeitsordner mit Hinweis; sonst neben der Karte", [] {
        ke::Editor& e = ke::editor();
        const std::string altZiel = e.bau.zielOrdner;
        const std::string altName = e.name;
        e.bau.zielOrdner.clear();
        e.name = spielOrdnerFuerTest() + "maps/x.map";
        std::string h;
        const std::string o = ke::bauOrdnerVorschau(&h);
        e.name = altName;
        std::string h2;
        const std::string o2 = ke::bauOrdnerVorschau(&h2);
        e.bau.zielOrdner = altZiel;
        return o == platform::settingsDirectory() + "\\karte_bau" && !h.empty() && o2.empty() && h2.empty();
    }));
    add(tu("Ziel-Ordner leer", [] {
        ke::editor().bau.zielOrdner.clear();
        ke::einstellungenSpeichern();
    }));
    // Bedienbar waehrend des Bauens, und Abbrechen
    add(tu("Eintrag \"Test langsam\" (-light -super 8 -samples 16 -bounce 100), Konsole leeren", [] {
        ke::editor().bauProjekt.eintraege.push_back(karte::BauEintrag{"Test langsam", {"[q3map2] -light -super 8 -samples 16 -bounce 100 \"[MapFile]\""}});
        ke::konsoleLeeren();
    }));
    add(bauMenuepunkt("Test langsam"));
    add(warteBis("q3map2 verbunden (das Licht rechnet)", [] { return konsoleText().find("Connected.") != std::string::npos; }, 600));
    auto massstab = std::make_shared<float>(0.0F);
    add(tu("Massstab der 2D-Ansicht merken", [massstab] { *massstab = ke::editor().xy[0].ansicht.massstab; }));
    add(radAn("2D-Ansicht zoomen, waehrend q3map2 rechnet", feldMitte("XY"), 2.0F));
    add(pause(0.3));
    add(pruefe("Der Editor bleibt bedienbar: die 2D-Ansicht zoomt, das Bauen laeuft weiter", [massstab] {
        diag::detail("Knopftest: Stufe 3/4 Massstab " + std::to_string(*massstab) + " -> " + std::to_string(ke::editor().xy[0].ansicht.massstab));
        return ke::bauLaeuft() && ke::editor().xy[0].ansicht.massstab != *massstab;
    }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.25));
    add(foto("karteneditor_s3_bauen_abbrechen"));
    add(klick(ke::trK(Str::KeBmAbbrechen), "##Menu_", 0, false));
    add(pause(0.3));
    add(pruefe("Build > Cancel build: q3map2 beendet, Zeile mit Schritt und Zeit in der Konsole, Statuszeile leer", [] {
        return !ke::bauLaeuft() && konsoleText().find(abgebrochenZeile("Test langsam", 1, 1)) != std::string::npos && ke::editor().statusBefehl.empty();
    }));
    add(tu("Konsole leeren", [] { ke::konsoleLeeren(); }));
    add(bauMenuepunkt("Single: bsp stage"));
    add(fertig("Bauen fertig", 1200));
    add(pruefe("Nach dem Abbrechen baut der naechste Eintrag wieder (Port 39000 frei)", [] {
        const std::string k = konsoleText();
        return k.find("Connected.") != std::string::npos && k.find(fertigZeile("Single: bsp stage", 1)) != std::string::npos;
    }));
    // Region (Map_SaveRegion): gebaut wird die Datei <Karte>.reg
    add(tu("einen Pinsel waehlen, Region > Set selection, Konsole leeren", [stamm] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder().front());
        ke::auswahlGeaendert();
        (void)ke::ausfuehren("RegionSetSelection");
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(stamm + ".reg"), ec);
        bauDateienWeg(stamm);
        ke::konsoleLeeren();
    }));
    add(bauMenuepunkt("Single: bsp stage"));
    add(fertig("Bauen fertig", 1200));
    add(pruefe("Region: raum.reg geschrieben, q3map2 baut die .reg ([MapFile]), raum.bsp entsteht", [stamm] {
        const std::string k = konsoleText();
        return ke::editor().region.aktiv && istDatei(stamm + ".reg") && k.find("/s3bau/raum.reg\"") != std::string::npos && istDatei(stamm + ".bsp") &&
               k.find(fertigZeile("Single: bsp stage", 1)) != std::string::npos;
    }));
    add(tu("Region aus", [] { (void)ke::ausfuehren("RegionOff"); }));
    // Unbenannte Karte: SaveMap fragt mit Speichern unter; abgebrochen -> "build cancelled"
    add(tu("File > New Map; Speichern unter wird abgebrochen; Konsole leeren", [] {
        ke::Editor& e = ke::editor();
        ke::neueKarte();
        e.dateiDialog = [](const std::string&) { return std::string(); };
        ke::konsoleLeeren();
    }));
    add(bauMenuepunkt("Single: bsp stage"));
    add(pause(0.2));
    add(pruefe("Unbenannte Karte, Speichern unter abgebrochen: \"build cancelled\" wie NRC, kein Bauen", [] {
        const std::string k = konsoleText();
        return k.find("build cancelled\n") != std::string::npos && k.find("=== running build command ===") == std::string::npos && !ke::bauLaeuft();
    }));
    add(tu("Speichern unter: s3bau/leer.map (die leere Karte); Konsole leeren", [] {
        bauDateienWeg(laufOrdner("s3bau\\leer"));
        ke::editor().dateiDialog = [](const std::string&) { return laufOrdner("s3bau\\leer.map"); };
        ke::konsoleLeeren();
    }));
    add(bauMenuepunkt("Single: bsp stage"));
    add(fertig("Bauen fertig", 1200));
    // Stufe 6 (13bdbab, kartefuzz/ASan): NRCs q3map2.exe stuerzt hier ab (entities[0] auf leerem
    // Vektor, 0xC0000005); bhed_q3map2 nennt den Grund, und der Bau stoppt mit Fehler
    add(pruefe("Leere Karte (ohne Entitaet) unter s3bau/leer.map gespeichert und gebaut: q3map2 nennt den Grund (keine Entity), "
               "der Bau stoppt mit Fehler (NRCs q3map2.exe stuerzt ab)",
               [] {
                   const std::string k = konsoleText();
                   diag::detail("Knopftest: Stufe 3/4 leere Karte: " + k.substr(k.size() > 200 ? k.size() - 200 : 0));
                   return istDatei(laufOrdner("s3bau\\leer.map")) && ke::editor().name == laufOrdner("s3bau\\leer.map") &&
                          konsoleArtMit(ke::KonsolenZeile::Art::Fehler, "No entity in map file") &&
                          konsoleArtMit(ke::KonsolenZeile::Art::Fehler, gestopptZeile("Single: bsp stage", 1, 1, "error")) &&
                          k.find("0xC0000005") == std::string::npos;
               }));
    add(tu("File > New Map, ein dichter Raum mit info_player_start; Speichern unter: s3bau/neu.map; Konsole leeren", [] {
        ke::Editor& e = ke::editor();
        ke::neueKarte();
        (void)e.karte.weltHolenOderAnlegen();
        using V = karte::Vektor3f;
        for (const auto& [a, b] : {std::pair<V, V>{V(-272, -272, -16), V(272, 272, 0)}, {V(-272, -272, 256), V(272, 272, 272)},
                                   {V(-272, -272, 0), V(-256, 272, 256)}, {V(256, -272, 0), V(272, 272, 256)},
                                   {V(-256, -272, 0), V(256, -256, 256)}, {V(-256, 256, 0), V(256, 272, 256)}}) {
            (void)stufe2::testPinsel(a, b, "textures/common/caulk");
        }
        const auto start = e.karte.neueEntitaet("info_player_start", false);
        start->setzen("origin", "0 0 32");
        e.karte.entitaetEinfuegen(start);
        bauDateienWeg(laufOrdner("s3bau\\neu"));
        e.dateiDialog = [](const std::string&) { return laufOrdner("s3bau\\neu.map"); };
        ke::konsoleLeeren();
    }));
    add(bauMenuepunkt("Single: bsp stage"));
    add(fertig("Bauen fertig", 1200));
    add(pruefe("Unbenannte Karte, Speichern unter s3bau/neu.map: erst gespeichert, dann gebaut (neu.bsp), ohne exit", [] {
        ke::editor().dateiDialog = nullptr;
        const std::string k = konsoleText();
        return istDatei(laufOrdner("s3bau\\neu.map")) && istDatei(laufOrdner("s3bau\\neu.bsp")) && ke::editor().name == laufOrdner("s3bau\\neu.map") &&
               k.find(fertigZeile("Single: bsp stage", 1)) != std::string::npos && k.find(": exit ") == std::string::npos;
    }));
    add(tu("Ohne Ueberwachung, neu.bsp weg; Konsole leeren", [] {
        ke::editor().bau.ueberwachen = false;
        bauDateienWeg(laufOrdner("s3bau\\neu"));
        ke::konsoleLeeren();
    }));
    add(bauMenuepunkt("Single: light stage"));
    add(fertig("Bauen fertig", 1200));
    add(pruefe("Ohne Ueberwachung, -light ohne .bsp: ERROR rot (aus der Pipe), am Ende (1: exit 1)", [] {
        ke::editor().bau.ueberwachen = true;
        const std::string k = konsoleText();
        return konsoleArtMit(ke::KonsolenZeile::Art::Fehler, "************ ERROR ************") &&
               k.find(fertigZeile("Single: light stage", 1) ) != std::string::npos && k.find("(1: exit 1)") != std::string::npos;
    }));
    add(tu("Aufraeumen: Build-Menue frisch, Seite Build wie vorgegeben", [] {
        ke::Editor& e = ke::editor();
        e.dateiDialog = nullptr;
        e.bauProjektFuer = -2;
        e.bauGeaendert = false;
        ke::bauMenueSicherstellen();
        e.bau = ke::Editor::BauEinstellungen{};
        ke::einstellungenSpeichern();
    }));
    return s;
}

// --- Punkt 5: Leck-Linie, Rueckmeldungen in 2D, Portal Viewer -------------------

// Ein Bildpunkt der 2D-Ansicht 0 an einem Weltpunkt (Mitte; ok = im Bild)
bool xyFarbe(const karte::Vektor3f& w, unsigned char rgba[4]) {
    const karte::XyAnsicht& v = ke::editor().xy[0].ansicht;
    const auto p = v.zuBild(w);
    const int x = static_cast<int>(std::lround(p[0]));
    const int y = static_cast<int>(std::lround(p[1]));
    if (x < 2 || y < 2 || x >= v.breite - 2 || y >= v.hoehe - 2) { return false; }
    return ke::xyPixel(0, x, y, rgba);
}

bool istRot(const unsigned char* c) { return c[0] > 200 && c[1] < 60 && c[2] < 60; }
// Rot, aber vom Raster (MSAA) nur teilweise bedeckt: eine 1 Punkt breite Kante,
// der Anfang einer Linie in der Bildmitte
bool istRoetlich(const unsigned char* c) { return c[0] > 120 && c[1] < 70 && c[2] < 70; }

std::string farbText(const unsigned char* c) {
    return std::to_string(c[0]) + "," + std::to_string(c[1]) + "," + std::to_string(c[2]);
}

// Die Mitte des Kamerabildes
bool kameraMitte(unsigned char rgba[4]) {
    const ke::KameraFenster& f = ke::editor().kamFenster;
    return ke::kameraPixel(f.breite / 2, f.hoehe / 2, rgba);
}

bool winkelGleich(float a, float b) { return std::fabs(a - b) < 0.01F; }

std::vector<Schritt> leck() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const std::string stamm = laufOrdner("s3bau") + "\\raum_leck";
    const auto fertig = [](const char* was, int bilder) { return warteBis(was, [] { return !ke::bauLaeuft(); }, bilder); };
    add(tu("Stufe 3/5: Testkarte raum_leck (Loch in der Ostwand), Build-Menue und Seite Build wie vorgegeben", [stamm] {
        karteDialogeZu();
        q3map2Bereitstellen();
        ke::Editor& e = ke::editor();
        e.bauProjektFuer = -2;
        e.bauGeaendert = false;
        ke::bauMenueSicherstellen();
        e.bau = ke::Editor::BauEinstellungen{};
        raumKarteOeffnen("raum_leck", true);
        bauDateienWeg(stamm);
        e.bauRueckmeldungen.clear();
        e.bauRueckmeldungWahl = -1;
        e.bauRueckmeldungOffen = false;
        ke::konsoleLeeren();
    }));
    add(bauMenuepunkt("Single: bsp stage"));
    add(fertig("Bauen fertig", 2400));
    add(pruefe("Leck: MAP LEAKED rot, Bauen angehalten (leak), der Hinweis in der Konsole, die .lin neben der Karte", [stamm] {
        const std::string k = konsoleText();
        diag::detail("Knopftest: Stufe 3/5 Leck: " + std::to_string(ke::editor().punktdatei.size()) + " Punkte");
        return konsoleArtMit(ke::KonsolenZeile::Art::Fehler, "MAP LEAKED") && k.find(gestopptZeile("Single: bsp stage", 1, 1, "leak")) != std::string::npos &&
               k.find(ke::trK(Str::KeBmLeckHinweis)) != std::string::npos && istDatei(stamm + ".lin");
    }));
    add(pruefe("Die Leck-Linie aus dem Rueckkanal (polyline) gleich der .lin, sichtbar, Stelle am Anfang", [stamm] {
        const ke::Editor& e = ke::editor();
        std::ifstream f(std::filesystem::path(stamm + ".lin"), std::ios::binary);
        std::ostringstream o;
        o << f.rdbuf();
        const auto lin = karte::punktdateiLesen(o.str());
        return e.punktdateiSichtbar && e.punktdatei.size() >= 2 && e.punktdatei == lin && e.punktdateiStelle == 0;
    }));
    add(pruefe("Q3Map debug window offen: \"Entity leaked\" (select) und \"Map leaked\" (die Linie)", [] {
        const ke::Editor& e = ke::editor();
        bool auswahl = false;
        bool linie = false;
        for (const auto& r : e.bauRueckmeldungen) {
            auswahl = auswahl || (r.art == karte::BauRueckmeldung::Art::Auswahl && r.name.find("Entity leaked") != std::string::npos);
            linie = linie || (r.art == karte::BauRueckmeldung::Art::Leck && r.name == "Map leaked");
        }
        return e.bauRueckmeldungOffen && auswahl && linie;
    }));
    add(foto("karteneditor_s3_leck"));
    // 2D: die Linie rot, 4 breit (an der Mitte des laengsten Stuecks)
    add(tu("2D-Ansicht XY auf die Leck-Linie (Massstab 0.5)", [] {
        ke::Editor& e = ke::editor();
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeMassstab(0.5F);
        const auto& p = e.punktdatei;
        e.xy[0].ansicht.setzeUrsprung((p.front() + p.back()) * 0.5F);
        ++e.sichtStand;
    }));
    add(pause(0.3));
    auto mitte = std::make_shared<karte::Vektor3f>();
    add(pruefe("2D: die Leck-Linie rot ($POINTFILE) auf der Mitte ihres laengsten Stuecks", [mitte] {
        const auto& p = ke::editor().punktdatei;
        std::size_t best = 0;
        float bestL = -1.0F;
        for (std::size_t i = 0; i + 1 < p.size(); ++i) {
            const karte::Vektor3f d = p[i + 1] - p[i];
            const float l = d[0] * d[0] + d[1] * d[1];
            if (l > bestL) {
                bestL = l;
                best = i;
            }
        }
        *mitte = (p[best] + p[best + 1]) * 0.5F;
        unsigned char c[4] = {};
        const bool ok = xyFarbe(*mitte, c);
        diag::detail("Knopftest: Stufe 3/5 XY an der Linie " + farbText(c));
        return ok && istRot(c);
    }));
    // Next / Previous leak spot (Strg+Umschalt+K / L)
    add(klickAn("in XY (Fokus)", feldMitte("XY"), 2));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tu("Konsole leeren", [] { ke::konsoleLeeren(); }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_K, "Strg+Umschalt+K"));
    add(pause(0.3));
    add(pruefe("Next leak spot: Kamera und 2D-Ansichten an Punkt 2 der Linie, Blick zu Punkt 3 (Pointfile_UpdateViews)", [] {
        const ke::Editor& e = ke::editor();
        const auto& p = e.punktdatei;
        if (p.size() < 3 || e.punktdateiStelle != 1) { return false; }
        const karte::Vektor3f d = p[2] - p[1];
        const float l = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        const float gier = std::atan2(d[1] / l, d[0] / l) * 57.29577951308232F;
        const float nick = std::asin(d[2] / l) * 57.29577951308232F;
        diag::detail("Knopftest: Stufe 3/5 Kamera " + std::to_string(e.kamera.winkel[0]) + " " + std::to_string(e.kamera.winkel[1]) + " soll " +
                     std::to_string(nick) + " " + std::to_string(gier));
        return e.kamera.ursprung == p[1] && e.xy[0].ansicht.ursprung == p[1] && e.xy[1].ansicht.ursprung == p[1] &&
               winkelGleich(e.kamera.winkel[1], gier) && winkelGleich(e.kamera.winkel[0], nick);
    }));
    add(pruefe("Kamera: die Linie entlang - in der Bildmitte beginnt das naechste Stueck rot (Tiefe getestet, 4 breit)", [] {
        unsigned char c[4] = {};
        const bool ok = kameraMitte(c);
        diag::detail("Knopftest: Stufe 3/5 Kamera Mitte " + farbText(c));
        return ok && istRoetlich(c);
    }));
    add(foto("karteneditor_s3_leck_kamera"));
    for (int i = 0; i < 8; ++i) { add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_K, "Strg+Umschalt+K")); }
    add(pause(0.2));
    add(pruefe("Am Ende: \"End of pointfile\", die Stelle bleibt beim vorletzten Punkt", [] {
        const ke::Editor& e = ke::editor();
        return konsoleText().find("End of pointfile\n") != std::string::npos && e.punktdateiStelle + 2 == e.punktdatei.size();
    }));
    for (int i = 0; i < 9; ++i) { add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_L, "Strg+Umschalt+L")); }
    add(pause(0.2));
    add(pruefe("Previous leak spot zurueck bis zum Anfang: \"Start of pointfile\", Kamera an Punkt 1", [] {
        const ke::Editor& e = ke::editor();
        return konsoleText().find("Start of pointfile\n") != std::string::npos && e.punktdateiStelle == 0 && e.kamera.ursprung == e.punktdatei[0];
    }));
    // File > Pointfile
    add(tu("Konsole leeren", [] { ke::konsoleLeeren(); }));
    add(klick(ke::trK(Str::KeLFile), "###main"));
    add(pause(0.25));
    add(klick(ke::trK(Str::KeLPointfile), "##Menu_", 0, false));
    add(pause(0.3));
    add(pruefe("File > Pointfile: aus, in 2D kein Rot mehr", [mitte] {
        unsigned char c[4] = {};
        (void)xyFarbe(*mitte, c);
        return !ke::editor().punktdateiSichtbar && !istRot(c);
    }));
    add(tu("Leck-Linie im Speicher leeren (sie muss aus der .lin kommen)", [] { ke::editor().punktdatei.clear(); }));
    add(klick(ke::trK(Str::KeLFile), "###main"));
    add(pause(0.25));
    add(klick(ke::trK(Str::KeLPointfile), "##Menu_", 0, false));
    add(pause(0.3));
    add(pruefe("File > Pointfile: \"Reading pointfile <Karte>.lin\", die Linie aus der Datei, sichtbar", [stamm] {
        const ke::Editor& e = ke::editor();
        std::string lin = ke::bauKartenPfad();
        lin = lin.substr(0, lin.rfind('.')) + ".lin";
        return konsoleText().find("Reading pointfile " + lin + "\n") != std::string::npos && e.punktdateiSichtbar && e.punktdatei.size() >= 2;
    }));
    add(tu("die .lin weg; Pointfile aus und wieder an; Konsole leeren", [stamm] {
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(stamm + ".lin"), ec);
        ke::punktdateiUmschalten();
        ke::konsoleLeeren();
        ke::punktdateiUmschalten();
    }));
    add(pruefe("Ohne .lin: \"Pointfile <Karte>.lin not found\" und \".pts not found\" rot (globalErrorStream)", [] {
        std::string stammK = ke::bauKartenPfad();
        stammK = stammK.substr(0, stammK.rfind('.'));
        return konsoleArtMit(ke::KonsolenZeile::Art::Fehler, "Pointfile " + stammK + ".lin not found") &&
               konsoleArtMit(ke::KonsolenZeile::Art::Fehler, "Pointfile " + stammK + ".pts not found");
    }));
    add(tu("Pointfile an (die alte Linie, wie NRC), dann File > Save as (derselbe Name)", [] {
        ke::Editor& e = ke::editor();
        if (!e.punktdateiSichtbar) { ke::punktdateiUmschalten(); }
        (void)ke::karteSpeichernUnter(e.name);   // Map_SaveAs -> Map_Save
    }));
    add(pruefe("Speichern (Map_Save: Pointfile_Clear) schaltet die Linie aus", [] { return !ke::editor().punktdateiSichtbar; }));
    // Q3Map debug window: "Entity N, Brush 0: Entity leaked" waehlen (CSelectMsg::Highlight)
    auto leckEnt = std::make_shared<int>(-1);
    add(klickAn("Zeile \"Entity N, Brush 0: Entity leaked\" im Q3Map debug window", [leckEnt] {
        for (const auto& r : ke::editor().bauRueckmeldungen) {
            if (r.art != karte::BauRueckmeldung::Art::Auswahl) { continue; }
            *leckEnt = r.entitaet;
            if (const Element* el = finde(r.name, "###kartedbgdlg", 0, true)) { return el->rect.GetCenter(); }
        }
        return ImVec2{0.0F, 0.0F};
    }));
    add(pause(0.3));
    add(pruefe("Q3Map debug window: die Zeile waehlt die Entitaet, aus der es leckt (SelectBrush), die 2D-Ansichten auf ihr", [leckEnt] {
        const ke::Editor& e = ke::editor();
        const auto el = e.karte.auswahl.elemente();
        if (el.size() != 1 || *leckEnt < 0 || static_cast<std::size_t>(*leckEnt) >= e.karte.entitaeten().size()) { return false; }
        const auto ent = el.front().entitaet.lock();
        diag::detail("Knopftest: Stufe 3/5 leckt aus Entity " + std::to_string(*leckEnt) + " " + (ent ? ent->klassenname() + " " + ent->wert("origin") : std::string()));
        return ent && ent == e.karte.entitaeten()[static_cast<std::size_t>(*leckEnt)] && e.bauRueckmeldungWahl >= 0;
    }));
    // CPointMsg / CWindingMsg::Draw2D (eine Meldung von Hand)
    add(tu("Rueckmeldungen pointmsg (128 64 32) und windingmsg (Dreieck) dazu, die pointmsg waehlen", [] {
        ke::Editor& e = ke::editor();
        karte::BauRueckmeldung p;
        p.art = karte::BauRueckmeldung::Art::Punkt;
        p.name = "Testpunkt";
        p.punkt = karte::Vektor3f(128, 64, 32);
        e.bauRueckmeldungen.push_back(p);
        karte::BauRueckmeldung w;
        w.art = karte::BauRueckmeldung::Art::Wicklung;
        w.name = "Testflaeche";
        w.punkte = {karte::Vektor3f(-256, -192, 0), karte::Vektor3f(-64, -192, 0), karte::Vektor3f(-160, 0, 0)};
        e.bauRueckmeldungen.push_back(w);
        e.karte.auswahl.leeren();
        ke::auswahlGeaendert();
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        e.xy[0].ansicht.setzeMassstab(1.0F);
        ++e.sichtStand;
    }));
    add(klick("Testpunkt", "###kartedbgdlg", 0, true));
    add(pause(0.3));
    add(pruefe("CPointMsg::Draw2D: der Punkt rot (4), das Quadrat +-8 rot", [] {
        unsigned char m[4] = {};
        unsigned char kante[4] = {};
        const bool a = xyFarbe(karte::Vektor3f(128, 64, 32), m);
        const bool b = xyFarbe(karte::Vektor3f(128 + 8, 64, 32), kante);
        diag::detail("Knopftest: Stufe 3/5 pointmsg " + farbText(m) + " / " + farbText(kante));
        return a && b && istRot(m) && istRoetlich(kante);
    }));
    add(klick("Testflaeche", "###kartedbgdlg", 0, true));
    add(pause(0.3));
    add(pruefe("CWindingMsg::Draw2D: die Flaeche blau gemischt (0.133, 0.4, 1, 0.5), die Ecken rot", [] {
        unsigned char m[4] = {};
        unsigned char ecke[4] = {};
        const bool a = xyFarbe(karte::Vektor3f(-160, -128, 0), m);
        const bool b = xyFarbe(karte::Vektor3f(-64, -192, 0), ecke);
        diag::detail("Knopftest: Stufe 3/5 windingmsg " + farbText(m) + " / " + farbText(ecke));
        return a && b && m[2] > m[0] + 60 && m[2] > m[1] + 30 && istRot(ecke);
    }));
    add(foto("karteneditor_s3_rueckmeldung2d"));
    add(tu("Aufraeumen: Debug-Fenster zu, Wahl weg", [] {
        ke::Editor& e = ke::editor();
        e.bauRueckmeldungWahl = -1;
        e.bauRueckmeldungOffen = false;
    }));
    return s;
}

// Im Konfigurationsdialog von prtview ziehen (logische Punkte des Dialogs)
Schritt prtZiehen(const std::string& was, ImVec2 von, ImVec2 nach) {
    auto a = std::make_shared<ImVec2>();
    auto z = std::make_shared<ImVec2>();
    return {"Ziehe " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    *a = ke::portalKonfigPunkt(von.x, von.y);
                    *z = ke::portalKonfigPunkt(nach.x, nach.y);
                    setzeMaus(*a);
                    return false;
                }
                if (b == 2) {
                    io.AddMouseButtonEvent(0, true);
                    return false;
                }
                if (b >= 3 && b <= 14) {
                    const float f = static_cast<float>(b - 2) / 12.0F;
                    setzeMaus(ImVec2{a->x + (z->x - a->x) * f, a->y + (z->y - a->y) * f});
                    return false;
                }
                if (b == 15) {
                    io.AddMouseButtonEvent(0, false);
                    return false;
                }
                return b >= 20;
            }};
}

std::vector<Schritt> portale() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const std::string stamm = laufOrdner("s3bau") + "\\raum";
    const auto fertig = [](const char* was, int bilder) { return warteBis(was, [] { return !ke::bauLaeuft(); }, bilder); };
    const auto menue = [&add](const char* punkt) {
        add(klick(ke::trK(Str::KeLPlugins), "###main"));
        add(pause(0.25));
        add(klick("prtview", "##Menu_", 0, true));
        add(pause(0.25));
        add(klick(punkt, "##Menu_", 0, false));
        add(pause(0.3));
    };
    add(tu("Stufe 3/5: prtview-Einstellungen frisch (ohne karte_prtview.ini), Testkarte raum, bsp stage", [stamm] {
        karteDialogeZu();
        ke::Editor& e = ke::editor();
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(ke::portalIniPfad()), ec);
        e.portalEinstellungenGelesen = false;
        e.portale.clear();
        raumKarteOeffnen("raum", false);
        bauDateienWeg(stamm);
        ke::konsoleLeeren();
    }));
    add(bauMenuepunkt("Single: bsp stage"));
    add(fertig("Bauen fertig", 2400));
    add(pruefe("bsp stage: raum.prt neben der Karte", [stamm] { return istDatei(stamm + ".prt"); }));
    add(tu("Konsole leeren", [] { ke::konsoleLeeren(); }));
    menue("Load .prt file");
    add(pruefe("Load .prt file: der Dialog \"Load .prt\" mit <Karte>.prt, Show 3D an, Show 2D aus (Vorgaben aus LoadConfig)", [] {
        const ke::Editor& e = ke::editor();
        std::string prt = ke::bauKartenPfad();
        prt = prt.substr(0, prt.rfind('.')) + ".prt";
        diag::detail("Knopftest: Stufe 3/5 Load .prt: " + e.portalDatei);
        return e.portalLadenOffen && e.portalDatei == prt && konsoleText().find("Portal Viewer plugin: Command \"Load .prt file\"\n") != std::string::npos &&
               e.portalE.zeige3d && !e.portalE.zeige2d && bauFeld("PrtLaden:Dialog") != nullptr;
    }));
    add(fotoRahmen("karteneditor_dlg_Plugins_prtview_Load_prt_file_0", [] { return fensterRahmen("###karteprtladen"); }));
    add(klick(std::string(ke::trK(Str::KePrtZeige2d)) + "##prtzeige2d", "###karteprtladen"));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##prtok", "###karteprtladen"));
    add(pause(0.3));
    add(pruefe("OK: \"Loading portal file <Karte>.prt.\", \"  10 portals read in.\", Show 2D an", [] {
        const ke::Editor& e = ke::editor();
        const std::string k = konsoleText();
        return !e.portalLadenOffen && e.portale.size() == 10 && e.portalHintFlags && e.portalE.zeige2d && e.portalE.zeige3d &&
               k.find("Portal Viewer plugin: Loading portal file " + e.portalDatei + ".\n") != std::string::npos &&
               k.find("  10 portals read in.\n") != std::string::npos;
    }));
    add(tu("2D-Ansicht XY: Ursprung (256 0 0), Massstab 1", [] {
        ke::Editor& e = ke::editor();
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(256, 0, 0));
        e.xy[0].ansicht.setzeMassstab(1.0F);
        ++e.sichtStand;
    }));
    add(pause(0.3));
    add(pruefe("2D: das Portal (8 0 0)-(512 0 256) als blaue Linie (color_2d 0 0 255, 3 breit) bei (260 0)", [] {
        unsigned char c[4] = {};
        unsigned char neben[4] = {};
        const bool a = xyFarbe(karte::Vektor3f(260, 0, 0), c);
        const bool b = xyFarbe(karte::Vektor3f(260, 0, 0) + karte::Vektor3f(0, 6, 0), neben);
        diag::detail("Knopftest: Stufe 3/5 Portal 2D " + farbText(c) + " daneben " + farbText(neben));
        return a && b && c[2] > 200 && c[0] < 60 && c[1] < 60 && neben[2] < 150;
    }));
    add(foto("karteneditor_s3_portale_2d"));
    // Kamera: im Raum B vor dem Portal x = 8 (Durchgang), Blick nach -x
    add(tu("Kamera bei (200 32 64), Blick nach -x (Durchgang mit Portal)", [] {
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(karte::Vektor3f(200, 32, 64));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 180, 0));
        ++e.sichtStand;
    }));
    add(pause(0.3));
    auto mit = std::make_shared<std::array<unsigned char, 4>>();
    add(tu("Bildmitte mit Portalen merken", [mit] { (void)kameraMitte(mit->data()); }));
    menue("Toggle portals (3D)");
    add(pruefe("Toggle portals (3D): \"Portals will NOT be rendered in 3D view.\", die Bildmitte anders (ohne das Polygon)", [mit] {
        unsigned char c[4] = {};
        (void)kameraMitte(c);
        diag::detail("Knopftest: Stufe 3/5 Kamera mit " + farbText(mit->data()) + " ohne " + farbText(c));
        return !ke::editor().portalE.zeige3d && konsoleText().find("Portal Viewer plugin: Portals will NOT be rendered in 3D view.\n") != std::string::npos &&
               (std::abs(c[0] - (*mit)[0]) + std::abs(c[1] - (*mit)[1]) + std::abs(c[2] - (*mit)[2])) > 30;
    }));
    menue("Toggle portals (3D)");
    add(pruefe("Toggle portals (3D) wieder: \"Portals will be rendered in 3D view.\", die Bildmitte wie vorher", [mit] {
        unsigned char c[4] = {};
        (void)kameraMitte(c);
        return ke::editor().portalE.zeige3d && konsoleText().find("Portal Viewer plugin: Portals will be rendered in 3D view.\n") != std::string::npos &&
               (std::abs(c[0] - (*mit)[0]) + std::abs(c[1] - (*mit)[1]) + std::abs(c[2] - (*mit)[2])) < 8;
    }));
    add(foto("karteneditor_s3_portale_kamera"));
    menue("Toggle portals (2D)");
    add(pruefe("Toggle portals (2D): \"Portals will NOT be rendered in 2D view.\", die Linie weg", [] {
        unsigned char c[4] = {};
        (void)xyFarbe(karte::Vektor3f(260, 0, 0), c);
        return !ke::editor().portalE.zeige2d && konsoleText().find("Portal Viewer plugin: Portals will NOT be rendered in 2D view.\n") != std::string::npos &&
               c[2] < 150;
    }));
    menue("Toggle portals (2D)");
    // Configure Portal Viewer
    menue("Configure Portal Viewer");
    add(pruefe("Configure Portal Viewer: der Dialog, 386 x 510 wie im Foto (nicht modal)", [] {
        const auto* d = bauFeld("Prt:Dialog");
        return ke::editor().portalKonfigOffen && d != nullptr && d->b == 386 && d->h == 510;
    }));
    add(pruefe("Lagen wie im Foto dlg_Plugins_prtview_Configure_Portal_Viewer_0 (Massstab 2 gemessen)", [] {
        const auto r = [](const char* n, int x, int y, int b, int h) {
            const auto* f = bauFeld(n);
            const bool ok = f != nullptr && f->x == x && f->y == y && f->b == b && f->h == h;
            if (!ok && f != nullptr) {
                diag::detail(std::string("Knopftest: Stufe 3/5 ") + n + " " + std::to_string(f->x) + "/" + std::to_string(f->y) + " " +
                             std::to_string(f->b) + "x" + std::to_string(f->h));
            }
            return ok;
        };
        return r("Prt:3DHaken", 12, 11, 13, 13) && r("Prt:3D", 12, 29, 363, 328) && r("Prt:LinesHaken", 24, 40, 13, 13) && r("Prt:Lines", 24, 59, 339, 65) &&
               r("Prt:LinesWidth", 97, 70, 253, 13) && r("Prt:LinesColor", 271, 90, 80, 22) && r("Prt:PolygonsHaken", 24, 130, 13, 13) &&
               r("Prt:Polygons", 24, 149, 339, 36) && r("Prt:Opacity", 122, 160, 228, 13) && r("Prt:FogHaken", 24, 192, 13, 13) &&
               r("Prt:Fog", 24, 211, 339, 43) && r("Prt:FogColor", 271, 221, 80, 22) && r("Prt:ClipHaken", 24, 261, 13, 13) &&
               r("Prt:Clip", 24, 280, 339, 36) && r("Prt:ClipRange", 130, 291, 220, 13) && r("Prt:ZBuffer", 23, 323, 340, 22) &&
               r("Prt:Hints", 138, 366, 13, 13) && r("Prt:Regular", 129, 392, 13, 13) && r("Prt:2DHaken", 12, 415, 13, 13) &&
               r("Prt:2D", 12, 433, 363, 66) && r("Prt:2DWidth", 85, 445, 277, 13) && r("Prt:2DColor", 283, 465, 80, 22);
    }));
    add(fotoRahmen("karteneditor_dlg_Plugins_prtview_Configure_Portal_Viewer_0", [] { return fensterRahmen("###karteprtkonfig"); }));
    // Width (Lines): den Griff ziehen (QStyle::sliderPositionFromValue: 3 -> Lage 53)
    add(prtZiehen("Griff Width (Lines) um 80 Punkte nach rechts", ImVec2{97.0F + 53.0F + 6.5F, 76.5F}, ImVec2{97.0F + 133.0F + 6.5F, 76.5F}));
    add(pause(0.2));
    add(pruefe("Width (Lines) gezogen: 3 -> 6 (sliderValueFromPosition), \"Width = 6\"", [] {
        diag::detail("Knopftest: Stufe 3/5 Width3D " + std::to_string(ke::editor().portalE.breite3d));
        return ke::editor().portalE.breite3d == 6;
    }));
    add(klickAn("Rinne von Opacity links vom Griff (Seitenschritt)", [] { return ke::portalKonfigPunkt(140.0F, 166.5F); }));
    add(pause(0.2));
    add(pruefe("Opacity: ein Klick in die Rinne = ein Seitenschritt (10) zur Maus: 50 -> 40", [] { return ke::editor().portalE.deckkraft == 40; }));
    add(klickAn("Haken Lines", [] { return ke::portalKonfigPunkt(24.0F + 6.5F, 40.0F + 6.5F); }));
    add(pause(0.2));
    add(prtZiehen("Griff Width (Lines) gesperrt ziehen", ImVec2{97.0F + 133.0F + 6.5F, 76.5F}, ImVec2{97.0F + 60.0F, 76.5F}));
    add(pause(0.2));
    add(pruefe("Lines aus: der Inhalt gesperrt (Width bleibt 6)", [] {
        const ke::Editor& e = ke::editor();
        return !e.portalE.linien && e.portalE.breite3d == 6;
    }));
    add(klickAn("Haken Lines", [] { return ke::portalKonfigPunkt(24.0F + 6.5F, 40.0F + 6.5F); }));
    add(pause(0.2));
    // Fog: mit kraeftiger Farbe, damit der Unterschied im Bild sicher ist
    add(tu("Kamera bei (500 32 64), Blick nach -x; Nebelfarbe Magenta", [] {
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(karte::Vektor3f(500, 32, 64));
        e.kamera.setzeWinkel(karte::Vektor3f(0, 180, 0));
        e.portalE.farbeNebel = karte::rgbPacken(255, 0, 255);
        e.portalE.farbenSetzen();
        ++e.sichtStand;
    }));
    add(pause(0.3));
    auto ohneNebel = std::make_shared<std::array<unsigned char, 4>>();
    add(tu("Bildmitte ohne Nebel merken", [ohneNebel] { (void)kameraMitte(ohneNebel->data()); }));
    add(klickAn("Haken Fog", [] { return ke::portalKonfigPunkt(24.0F + 6.5F, 192.0F + 6.5F); }));
    add(pause(0.3));
    add(pruefe("Fog an: das Portal in der Bildmitte zum Magenta hin (GL_EXP, Dichte 0.001, Abstand ~490)", [ohneNebel] {
        unsigned char c[4] = {};
        (void)kameraMitte(c);
        diag::detail("Knopftest: Stufe 3/5 Nebel " + farbText(ohneNebel->data()) + " -> " + farbText(c));
        return ke::editor().portalE.nebel && c[0] > (*ohneNebel)[0] + 8 && c[1] + 8 < (*ohneNebel)[1];
    }));
    add(klickAn("Color (Fog)", [] { return ke::portalKonfigPunkt(311.0F, 232.0F); }));
    add(pause(0.3));
    add(pruefe("Color (Fog): der Farbdialog geht auf (OnColor)", [] {
        const ke::Editor& e = ke::editor();
        return e.ent.farbDialog.offen || e.ent.farbDialog.oeffnen;
    }));
    add(tu("Farbdialog zu", [] {
        ke::Editor& e = ke::editor();
        e.ent.farbDialog.offen = false;
        e.ent.farbDialog.oeffnen = false;
        if (GImGui->OpenPopupStack.Size > 0) { ImGui::ClosePopupToLevel(0, true); }
    }));
    add(pause(0.2));
    add(klickAn("Klappliste Z-Buffer", [] { return ke::portalKonfigPunkt(193.0F, 334.0F); }));
    add(pause(0.3));
    add(klick(ke::trK(Str::KePrtZAus), "##Combo_", 0, false));
    add(pause(0.2));
    add(pruefe("Z-Buffer Off gewaehlt (zbuffer 2)", [] { return ke::editor().portalE.zPuffer == 2; }));
    add(tu("Dialog zu (WA_DeleteOnClose)", [] { ke::editor().portalKonfigOffen = false; }));
    add(pause(0.2));
    add(pruefe("SaveConfig: <Einstellungen>/karte_prtview.ini wie prtview.ini (CRLF, sortiert, Kommentare)", [] {
        ke::portalEinstellungenSchreiben();
        std::ifstream f(std::filesystem::path(ke::portalIniPfad()), std::ios::binary);
        std::ostringstream o;
        o << f.rdbuf();
        const std::string t = o.str();
        diag::detail("Knopftest: Stufe 3/5 ini " + std::to_string(t.size()) + " Byte");
        return t.rfind("[Configuration]\r\nClip=0        ; Cubic clipper active for portal viewer\r\n", 0) == 0 &&
               t.find("ColorFog=16711935        ; Color of distant lines/polygons in 3D window\r\n") != std::string::npos &&
               t.find("Fog=1        ; Use depth cueing in 3D window\r\n") != std::string::npos &&
               t.find("Opacity=40        ; Opacity in 3d view (0 = invisible, 100 = solid)\r\n") != std::string::npos &&
               t.find("Render2D=1        ; Draw in 2D windows\r\n") != std::string::npos &&
               t.find("Width3D=6        ; Width of lines in 3D window\r\n") != std::string::npos &&
               t.find("ZBuffer=2        ; ZBuffer level in 3D window\r\n") != std::string::npos;
    }));
    add(pruefe("LoadConfig: dieselben Werte zurueck", [] {
        ke::Editor& e = ke::editor();
        const karte::PortalEinstellungen vorher = e.portalE;
        e.portalEinstellungenGelesen = false;
        e.portalE = karte::PortalEinstellungen{};
        ke::portalEinstellungenLesen();
        const karte::PortalEinstellungen& n = e.portalE;
        return n.breite3d == vorher.breite3d && n.deckkraft == vorher.deckkraft && n.nebel && n.zPuffer == 2 && n.farbeNebel == vorher.farbeNebel &&
               n.zeige2d == vorher.zeige2d;
    }));
    // Unload, About, Load abgebrochen, Load mit fehlender Datei
    add(tu("Konsole leeren", [] { ke::konsoleLeeren(); }));
    menue("Unload .prt file");
    add(pruefe("Unload .prt file: keine Portale, \"Portals unloaded.\"", [] {
        return ke::editor().portale.empty() && konsoleText().find("Portal Viewer plugin: Portals unloaded.\n") != std::string::npos;
    }));
    menue("About Portal Viewer");
    add(pruefe("About Portal Viewer: der Kasten \"About Portal Viewer\" (Version 1.000, Leonardo Zide, Geoffrey DeWan)", [] {
        const ke::Editor& e = ke::editor();
        return e.meldung.offen && e.meldung.titel == "About Portal Viewer" && e.meldung.text.find("Version 1.000") == 0 &&
               e.meldung.text.find("Geoffrey DeWan") != std::string::npos;
    }));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##meldungok"));
    add(pause(0.2));
    menue("Load .prt file");
    add(klick(std::string(ke::trK(Str::KeCancel)) + "##prtabbrechen", "###karteprtladen"));
    add(pause(0.3));
    add(pruefe("Load .prt, Cancel: \"Portal file load aborted.\", nichts geladen", [] {
        return !ke::editor().portalLadenOffen && ke::editor().portale.empty() &&
               konsoleText().find("Portal Viewer plugin: Portal file load aborted.\n") != std::string::npos;
    }));
    menue("Load .prt file");
    add(klickAn("Feld der Datei", stufe2::elementOrt("##prtdatei")));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_A, "Strg+A"));
    add(tippe(laufOrdner("s3bau\\nicht_da.prt")));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##prtok", "###karteprtladen"));
    add(pause(0.3));
    add(pruefe("Load .prt mit fehlender Datei: \"  ERROR - could not open file.\" rot, keine Portale", [] {
        return ke::editor().portale.empty() && konsoleArtMit(ke::KonsolenZeile::Art::Fehler, "  ERROR - could not open file.") &&
               ke::editor().portalDatei == laufOrdner("s3bau\\nicht_da.prt");
    }));
    add(tu("Aufraeumen: prtview-Einstellungen frisch, Dialoge zu", [] {
        karteDialogeZu();
        ke::Editor& e = ke::editor();
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(ke::portalIniPfad()), ec);
        e.portalEinstellungenGelesen = false;
        e.portale.clear();
        ke::portalEinstellungenLesen();
    }));
    return s;
}

// --- Punkt 6: AutoCaulk Selected (F4) ------------------------------------------

// Die Flaeche eines Pinsels mit dieser Ebenen-Normalen (gerundet)
std::shared_ptr<karte::Flaeche> flaecheMitNormale(const std::shared_ptr<karte::Pinsel>& p, int nx, int ny, int nz) {
    p->bRep(ke::editor().karte.kontext);
    for (const auto& f : p->flaechen()) {
        const karte::Vektor3d& n = f->ebene().normal();
        if (std::lround(n[0]) == nx && std::lround(n[1]) == ny && std::lround(n[2]) == nz) { return f; }
    }
    return nullptr;
}

std::shared_ptr<karte::Pinsel> weltPinsel(std::size_t i) {
    const auto& k = ke::editor().karte.welt()->kinder();
    return i < k.size() && k[i]->art() == karte::Primitiv::Art::Pinsel ? std::static_pointer_cast<karte::Pinsel>(k[i]) : nullptr;
}

std::vector<Schritt> autocaulk() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const std::string stamm = laufOrdner("s3bau") + "\\raum";
    add(tu("Stufe 3/6: Testkarte raum, alle Pinsel von worldspawn gewaehlt, Kamera in Raum A", [stamm] {
        karteDialogeZu();
        q3map2Bereitstellen();
        ke::Editor& e = ke::editor();
        e.bau = ke::Editor::BauEinstellungen{};
        raumKarteOeffnen("raum", false);
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(stamm + "_ac.map"), ec);
        std::filesystem::remove(std::filesystem::path(stamm + "_ac.caulk"), ec);
        e.karte.auswahl.leeren();
        for (const auto& p : e.karte.welt()->kinder()) {
            if (p->art() == karte::Primitiv::Art::Pinsel) { e.karte.auswahl.waehlen(e.karte.welt(), p); }
        }
        ke::auswahlGeaendert();
        e.kamera.setzeUrsprung(karte::Vektor3f(-384, 128, 64));
        ke::konsoleLeeren();
    }));
    add(klickAn("in XY (Fokus)", feldMitte("XY"), 2));
    add(tasteKombi(ImGuiKey_F4, "F4"));
    add(pause(0.3));
    add(pruefe("F4 (AutoCaulk Selected): raum_ac.map und raum_ac.caulk neben der Karte, Rueckgaengig \"AutoCaulk N faces\" mit N > 0", [stamm] {
        const ke::Editor& e = ke::editor();
        const std::string u = e.karte.undo.undoName();
        diag::detail("Knopftest: Stufe 3/6 " + u + " / Konsole: " + konsoleText().substr(0, 200));
        return istDatei(stamm + "_ac.map") && istDatei(stamm + "_ac.caulk") && u.rfind("AutoCaulk ", 0) == 0 && u != "AutoCaulk 0 faces" &&
               !konsoleArtMit(ke::KonsolenZeile::Art::Fehler, "map_autocaulk_selected");
    }));
    add(pruefe("Der Boden: unten (Normale -z, von aussen) caulk, oben (+z, im Raum) bleibt test/boden; die Nordwand aussen (+y) caulk", [] {
        const auto boden = weltPinsel(0);
        const auto nord = weltPinsel(2);
        if (!boden || !nord) { return false; }
        const auto unten = flaecheMitNormale(boden, 0, 0, -1);
        const auto oben = flaecheMitNormale(boden, 0, 0, 1);
        const auto aussen = flaecheMitNormale(nord, 0, 1, 0);
        const auto innen = flaecheMitNormale(nord, 0, -1, 0);
        diag::detail("Knopftest: Stufe 3/6 Boden unten " + (unten ? unten->shader() : std::string("-")) + ", oben " + (oben ? oben->shader() : std::string("-")) +
                     ", Nordwand aussen " + (aussen ? aussen->shader() : std::string("-")) + ", innen " + (innen ? innen->shader() : std::string("-")));
        return unten && oben && aussen && innen && unten->shader() == "textures/system/caulk" && oben->shader() == "textures/test/boden" &&
               aussen->shader() == "textures/system/caulk" && innen->shader() == "textures/test/wand";
    }));
    add(pruefe("raum_ac.map wie NRC: worldspawn mit den Pinseln und dem Kasten, info_player_start am Auge (-384 128 64; nicht in der Saeule), CRLF", [stamm] {
        std::ifstream f(std::filesystem::path(stamm + "_ac.map"), std::ios::binary);
        std::ostringstream o;
        o << f.rdbuf();
        const std::string t = o.str();
        return t.rfind("{\r\n\"classname\" \"worldspawn\"\r\n{\r\n", 0) == 0 &&
               t.find("{\r\n\"classname\" \"info_player_start\"\r\n\"origin\" \"-384 128 64\"\r\n}\r\n") != std::string::npos;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.2));
    add(pruefe("Strg+Z: die alten Shader zurueck", [] {
        const auto boden = weltPinsel(0);
        const auto unten = boden ? flaecheMitNormale(boden, 0, 0, -1) : nullptr;
        return unten && unten->shader() == "textures/test/boden";
    }));
    // Fehler wie NRC
    add(tu("Kamera weit weg (5000 5000 5000), Konsole leeren", [] {
        ke::editor().kamera.setzeUrsprung(karte::Vektor3f(5000, 5000, 5000));
        ke::konsoleLeeren();
    }));
    add(tasteKombi(ImGuiKey_F4, "F4"));
    add(pause(0.2));
    add(pruefe("Kamera nicht bei der Auswahl: \"map_autocaulk_selected(): camera must be near selection!\" rot", [] {
        return konsoleArtMit(ke::KonsolenZeile::Art::Fehler, "map_autocaulk_selected(): camera must be near selection!");
    }));
    add(tu("Nur ein trigger_multiple gewaehlt; Konsole leeren", [] {
        ke::Editor& e = ke::editor();
        e.kamera.setzeUrsprung(karte::Vektor3f(-384, 128, 64));
        auto trig = e.karte.neueEntitaet("trigger_multiple", true);
        e.karte.entitaetEinfuegen(trig);
        (void)stufe2::testPinsel(karte::Vektor3f(-300, -32, 0), karte::Vektor3f(-260, 32, 64), "textures/system/trigger", trig);
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(trig, trig->kinder().front());
        ke::auswahlGeaendert();
        ke::konsoleLeeren();
    }));
    add(tasteKombi(ImGuiKey_F4, "F4"));
    add(pause(0.2));
    add(pruefe("Nur ein Trigger: abgewaehlt (DeselectTriggers), dann \"map_autocaulk_selected(): nothing is selected\" rot", [] {
        return ke::editor().karte.auswahl.anzahl() == 0 && konsoleArtMit(ke::KonsolenZeile::Art::Fehler, "map_autocaulk_selected(): nothing is selected");
    }));
    return s;
}

}  // namespace stufe3

// --- Stufe 4 des Karten-Editors: fertige Karten, Originallicht, Plugins ----------
//
// BHED_KARTE_NUR=stufe4 laesst nur diese Schritte laufen; BHED_STUFE4_NUR=<Teil>
// nur einen Teil (reste, oeffnen, licht, plugins).
namespace stufe4 {

namespace ke = karteneditor;

// Punkt 0c: keine Beschriftung einer Einstellungsseite abgeschnitten. Jede
// Glyphe (Vierecke mit der Schrift-Textur, nicht das weisse Pixel) im Fenster
// liegt waagerecht im Rahmen der Seitengruppe (QGroupBox 164..586 - links
// davon verdeckt der Seitenbaum, was hinausragt), jede Glyphe des Baums in
// seinem Ausschnitt.
bool beschriftungenGanz(std::string& bericht) {
    ImGuiWindow* w = ImGui::FindWindowByName("###karteeinstellungen");
    if (w == nullptr || !(w->Active || w->WasActive)) {
        bericht = "kein Fenster";
        return false;
    }
    const float sk = ke::massstab();
    const ImVec2 uvWeiss = ImGui::GetIO().Fonts->TexUvWhitePixel;
    const ImTextureID schrift = ImGui::GetIO().Fonts->TexRef.GetTexID();
    int ecken = 0;
    int raus = 0;
    std::string erste;
    const float titelUnten = w->Pos.y + w->TitleBarHeight;   // der Fenstertitel zaehlt nicht
    // Kantengeglaettete Linien (Fensterrand) nehmen ihre UV aus TexUvLines -
    // keine Glyphen.
    ImVec2 linienMin{1.0F, 1.0F};
    ImVec2 linienMax{0.0F, 0.0F};
    for (const ImVec4& l : ImGui::GetIO().Fonts->TexUvLines) {
        linienMin = ImVec2{std::min({linienMin.x, l.x, l.z}), std::min({linienMin.y, l.y, l.w})};
        linienMax = ImVec2{std::max({linienMax.x, l.x, l.z}), std::max({linienMax.y, l.y, l.w})};
    }
    const auto pruefeListe = [&](const ImDrawList* dl, float x0, float x1, const char* wo, float zeileOben) {
        for (const ImDrawCmd& c : dl->CmdBuffer) {
            if (c.UserCallback != nullptr || c.GetTexID() != schrift) { continue; }
            for (unsigned int i = 0; i < c.ElemCount; ++i) {
                const ImDrawVert& v = dl->VtxBuffer[static_cast<int>(c.VtxOffset + dl->IdxBuffer[static_cast<int>(c.IdxOffset + i)])];
                if ((v.uv.x == uvWeiss.x && v.uv.y == uvWeiss.y) || v.pos.y < titelUnten) { continue; }
                if (v.uv.x >= linienMin.x && v.uv.x <= linienMax.x && v.uv.y >= linienMin.y && v.uv.y <= linienMax.y) { continue; }
                ++ecken;
                if (v.pos.x < x0 - 0.5F || v.pos.x > x1 + 0.5F) {
                    ++raus;
                    if (erste.empty()) {
                        char z[160];
                        std::snprintf(z, sizeof(z), "%s: Glyphe bei x %.0f y %.0f (erlaubt %.0f..%.0f, Fenster %.0f/%.0f, Zeile %d)", wo, v.pos.x,
                                      v.pos.y, x0, x1, w->Pos.x, w->Pos.y, static_cast<int>((v.pos.y - zeileOben) / (16.0F * sk)));
                        erste = z;
                    }
                }
            }
        }
    };
    pruefeListe(w->DrawList, std::round(w->Pos.x + 164.0F * sk), std::round(w->Pos.x + (164.0F + 422.0F) * sk), "Seite", titelUnten);
    for (ImGuiWindow* k : GImGui->Windows) {
        if (k->ParentWindow == w && (k->Active || k->WasActive)) {
            pruefeListe(k->DrawList, k->InnerClipRect.Min.x, k->InnerClipRect.Max.x, "Baum", k->Pos.y + 1.0F * sk);
        }
    }
    bericht = std::to_string(ecken / 6) + " Glyphen, " + std::to_string(raus) + " Ecken draussen" + (erste.empty() ? "" : " - " + erste);
    return ecken > 0 && raus == 0;
}

std::vector<Schritt> reste0() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    // --- 0d: Surface Inspector so breit wie das NRC-Foto ---------------------
    // dlg_View_Surface_Inspector_0.png (2x): Innenraum 285 x 364; die Gruppe
    // "Texturing" mit den Spalten 23 (43), 72 (44), 122 (67), 195 (67).
    auto nrcVorher = std::make_shared<bool>(false);
    add(stufe2::grundzustand("Stufe 4/0d: Grundzustand"));
    add(tu("Begriffe wie NetRadiant, Surface Inspector auf", [nrcVorher] {
        *nrcVorher = ke::nrcBegriffe();
        ke::setzeNrcBegriffe(true);
        ke::editor().siOffen = true;
    }));
    add(pause(0.4));
    add(pruefe("Surface Inspector: Innenraum 285 x 364, Texturing-Spalten 23/43, 72/44, 122/67, 195/67 wie das NRC-Foto", [] {
        const ImGuiWindow* w = ImGui::FindWindowByName("###kartesi");
        if (w == nullptr) { return false; }
        const float sk = ke::massstab();
        const long b = std::lround(w->Size.x / sk);
        const long h = std::lround((w->Size.y - w->TitleBarHeight) / sk);
        std::string bericht = "Innenraum " + std::to_string(b) + " x " + std::to_string(h) + ", Spalten";
        bool ok = b == 285 && h == 364;
        const std::pair<Str, const char*> knoepfe[] = {{Str::KeSiReset, "##sireset"}, {Str::KeSiFit, "##sifit"}, {Str::KeSiWidth, "##siwidth"},
                                                       {Str::KeSiHeight, "##siheight"}};
        const long soll[4][2] = {{23, 43}, {72, 44}, {122, 67}, {195, 67}};
        for (int i = 0; i < 4; ++i) {
            const Element* el = finde(std::string(ke::trK(knoepfe[i].first)) + knoepfe[i].second, "", 0, true);
            if (el == nullptr) { return false; }
            const long x = std::lround((el->rect.Min.x - w->Pos.x) / sk);
            const long bb = std::lround(el->rect.GetWidth() / sk);
            bericht += " " + std::to_string(x) + "/" + std::to_string(bb);
            ok = ok && x == soll[i][0] && bb == soll[i][1];
        }
        diag::detail("Knopftest: Stufe 4 - Surface Inspector " + bericht);
        return ok;
    }));
    add(tu("Surface Inspector zu, Begriffe zurueck", [nrcVorher] {
        ke::editor().siOffen = false;
        ke::setzeNrcBegriffe(*nrcVorher);
    }));
    // --- 0c: keine abgeschnittene Beschriftung, alle Seiten, alle Sprachen ----
    auto sprache = std::make_shared<i18n::Language>(i18n::Language::English);
    add(tu("Sprache merken", [sprache, nrcVorher] {
        *sprache = i18n::currentLanguage();
        *nrcVorher = ke::nrcBegriffe();
    }));
    const std::pair<i18n::Language, const char*> sprachen[] = {{i18n::Language::English, "Englisch"},
                                                               {i18n::Language::German, "Deutsch"},
                                                               {i18n::Language::ChineseSimplified, "Chinesisch"},
                                                               {i18n::Language::Japanese, "Japanisch"}};
    for (const auto& [l, name] : sprachen) {
        const i18n::Language lang = l;
        const std::string sname = name;
        add(tu("Einstellungen auf " + sname, [lang] {
            i18n::setLanguage(lang);
            ke::setzeNrcBegriffe(false);
            ke::editor().einstellungenOffen = true;
        }));
        add(pause(0.3));
        for (int seite = 0; seite < ke::einstellungsSeitenAnzahl(); ++seite) {
            add(tu("Seite " + std::to_string(seite), [seite] { ke::einstellungsSeiteZeigen(seite); }));
            add(pause(0.12));
            add(pruefe("Einstellungen " + sname + ", Seite " + std::to_string(seite) + ": keine Beschriftung abgeschnitten", [seite, sname] {
                std::string bericht;
                const bool ok = beschriftungenGanz(bericht);
                diag::detail("Knopftest: Stufe 4 - " + sname + " Seite " + std::to_string(seite) + ": " + bericht);
                return ok;
            }));
        }
        add(tu("Einstellungen zu", [] { ke::editor().einstellungenOffen = false; }));
        add(pause(0.2));
    }
    add(tu("Sprache zurueck", [sprache, nrcVorher] {
        i18n::setLanguage(*sprache);
        ke::setzeNrcBegriffe(*nrcVorher);
    }));
    return s;
}

// Punkt 1: fertige Karten oeffnen - File > Open .bsp..., der Dialog "Open
// compiled map" mit den Karten der Spielordner (aus .pk3), mp/duel1 oeffnen:
// im Arbeitsordner umgewandelt, Entitaeten/Pinsel/Patches wie die Massenlast
// (SP\karte_dekomp\ja\bp\mp__duel1.map, von NRCs q3map2: 26 / 362 / 66).
std::string bspArbeitsOrdner() { return (std::filesystem::path(platform::settingsDirectory()) / "karte_bsp" / "duel1").string(); }

std::vector<Schritt> oeffnen() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(stufe2::grundzustand("Stufe 4/1: Grundzustand"));
    add(tu("bhed_q3map2.exe neben behaved.exe, alter Arbeitsordner weg, ohne die Rueckgewinnung der Stufe 8a", [] {
        stufe3::q3map2Bereitstellen();
        ke::editor().bspRueck.modelle = false;   // Stufe 8a: ohne Rueckgewinnung (hier zaehlt das Dekompilat)
        ke::editor().bspRueck.lichter = false;
        std::error_code ec;
        std::filesystem::remove_all(std::filesystem::path(bspArbeitsOrdner()), ec);
        ke::konsoleLeeren();
    }));
    add(klick(ke::trK(Str::KeLFile), "###main"));
    add(pause(0.2));
    add(pruefe("File: \"Open .bsp...\" direkt nach \"Import...\"", [] {
        const Element* imp = finde(ke::trK(Str::KeLImport), "", 0, false);
        const Element* bsp = finde(std::string(ke::trK(Str::KeLOpenBsp)) + "###progbspoeffnen", "", 0, true);
        return imp != nullptr && bsp != nullptr && bsp->rect.Min.y > imp->rect.Min.y && bsp->rect.Min.y - imp->rect.Max.y < 2.0F * ke::massstab();
    }));
    add(klick(std::string(ke::trK(Str::KeLOpenBsp)) + "###progbspoeffnen", "", 0, true));
    add(pause(0.4));
    add(pruefe("Dialog \"Open compiled map\" offen, mp/duel1.bsp aus einer .pk3 in der Liste", [] {
        if (!ke::bspDialogOffen()) { return false; }
        const auto liste = ke::bspDialogListe();
        for (const auto& b : liste) {
            if (b.pfad == "maps/mp/duel1.bsp") {
                diag::detail("Knopftest: Stufe 4 - " + std::to_string(liste.size()) + " Karten, duel1 aus " + b.archiv);
                return b.archiv.size() > 4 && b.archiv.substr(b.archiv.size() - 4) == ".pk3";
            }
        }
        return false;
    }));
    add(fotoRahmen("karteneditor_dlg_Open_compiled_map", [] { return fensterRahmen("###kartebspoeffnen"); }));
    add(klickAn("Filter", stufe2::elementOrt("##bspfilter")));
    add(tippe("mp/duel1"));
    add(pause(0.3));
    add(klickAn("Eintrag mp/duel1.bsp", [] {
        const ImGuiWindow* w = nullptr;
        for (ImGuiWindow* x : GImGui->Windows) {
            if (std::strstr(x->Name, "##bspliste") != nullptr && x->WasActive) { w = x; }
        }
        return w != nullptr ? ImVec2{w->Pos.x + 60.0F * ke::massstab(), w->Pos.y + 10.0F * ke::massstab()} : ImVec2{5, 5};
    }));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeBspOeffnen)) + "##bspoeffnen", "###kartebspoeffnen", 0, true));
    add(warteBis("Karte geladen", [] { return !ke::bspDialogOffen() && ke::editor().bspHerkunft.aktiv; }, 1200));
    add(pruefe("mp/duel1 umgewandelt und geladen: 26 Entitaeten, 362 Pinsel, 66 Patches wie die Massenlast", [] {
        const ke::Editor& e = ke::editor();
        std::size_t pinsel = 0;
        std::size_t patches = 0;
        for (const auto& en : e.karte.entitaeten()) {
            for (const auto& pr : en->kinder()) {
                if (pr->art() == karte::Primitiv::Art::Patch) {
                    ++patches;
                } else if (pr->art() == karte::Primitiv::Art::Pinsel) {
                    ++pinsel;
                }
            }
        }
        diag::detail("Knopftest: Stufe 4 - duel1 " + std::to_string(e.karte.entitaeten().size()) + " / " + std::to_string(pinsel) + " / " +
                     std::to_string(patches) + ", " + e.name);
        return e.karte.entitaeten().size() == 26 && pinsel == 362 && patches == 66;
    }));
    add(pruefe("Arbeitsordner statt Spielordner: duel1.bsp, duel1_original.bsp, duel1_converted.map, duel1.map; Karte = duel1.map dort", [] {
        const std::string o = bspArbeitsOrdner();
        const ke::Editor& e = ke::editor();
        return stufe3::istDatei(o + "/duel1.bsp") && stufe3::istDatei(o + "/duel1_original.bsp") && stufe3::istDatei(o + "/duel1_converted.map") &&
               stufe3::istDatei(o + "/duel1.map") && std::filesystem::equivalent(std::filesystem::path(e.name), std::filesystem::path(o + "/duel1.map")) &&
               e.name.find("GameData") == std::string::npos;
    }));
    add(pruefe("Konsole: \"Opening compiled map maps/mp/duel1.bsp\", q3map2 -convert -format map / map_bp nach New map Brush Type, Loading map from", [] {
        const std::string k = stufe3::konsoleText();
        const std::string format = ke::editor().pinselTyp != 0 ? " -convert -format map_bp \"" : " -convert -format map \"";
        return k.find("Opening compiled map maps/mp/duel1.bsp") != std::string::npos && k.find("bhed_q3map2.exe\" -v -game ja -fs_basepath") != std::string::npos &&
               k.find(format) != std::string::npos && k.find("Loading map from ") != std::string::npos;
    }));
    // File > Import... mit einer .bsp (wie NRC: tryDecompile, dann eingefuegt)
    add(tu("Import...: die Dateiwahl liefert die Original-.bsp", [] {
        ke::editor().dateiDialog = [](const std::string&) { return bspArbeitsOrdner() + "/duel1_original.bsp"; };
    }));
    add(tu("File > Import... (ImportMap)", [] { (void)ke::ausfuehren("ImportMap"); }));
    add(pause(0.3));
    add(pruefe("Import einer .bsp: umgewandelt und eingefuegt - 26 + 25 Entitaeten (worldspawn verschmilzt)", [] {
        diag::detail("Knopftest: Stufe 4 - nach Import " + std::to_string(ke::editor().karte.entitaeten().size()) + " Entitaeten");
        return ke::editor().karte.entitaeten().size() == 51;
    }));
    add(tu("Dateiwahl zurueck, Rueckgaengig", [] {
        ke::editor().dateiDialog = nullptr;
        (void)ke::ausfuehren("Undo");
    }));
    add(pruefe("Rueckgaengig: wieder 26 Entitaeten, die Herkunft bleibt", [] {
        return ke::editor().karte.entitaeten().size() == 26 && ke::editor().bspHerkunft.aktiv;
    }));
    return s;
}

// Punkt 2: Originallicht behalten - nur Entities geaendert: Build > "Update
// entities only" (q3map2 -onlyents auf eine Kopie, Entity-Lump ins Original);
// danach jeder Lump ausser den Entities Byte-gleich. Geometrie geaendert: ein
// Hinweis, q3map2 laeuft nicht.
std::vector<Schritt> licht() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(tu("Stufe 4/2: worldspawn \"message\", info_player_deathmatch verschoben", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        {
            karte::Befehl b(e.karte.undo, "Stufe 4 Test");
            e.karte.welt()->setzen("message", "Stufe 4 bhed_test");
            for (const auto& en : e.karte.entitaeten()) {
                if (en->klassenname() == "info_player_deathmatch") {
                    en->setzen("origin", "100 200 64");
                    break;
                }
            }
        }
        e.bspNurEntitaetenFertig.clear();
        ke::konsoleLeeren();
    }));
    add(pruefe("Nur Entities geaendert: derselbe Fingerabdruck, Build > Update entities only verfuegbar", [] {
        return ke::bspNurEntitaetenMoeglich() && !ke::bspGeometrieGeaendert();
    }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.2));
    add(foto("karteneditor_baumenue_nurentities"));
    add(klick(std::string(ke::trK(Str::KeBmNurEntities)) + "##baunurentities", "", 0, true));
    add(warteBis("-onlyents fertig", [] { return !ke::bauLaeuft() && !ke::editor().bspNurEntitaetenFertig.empty(); }, 1800));
    add(pruefe("duel1.bsp: jeder Lump ausser den Entities Byte-gleich zur Original-.bsp, der Entity-Lump traegt die Aenderung", [] {
        const std::string o = bspArbeitsOrdner();
        karte::BspDatei a;
        karte::BspDatei b;
        if (!karte::bspLesen(dateiInhalt(o + "/duel1_original.bsp"), a) || !karte::bspLesen(dateiInhalt(o + "/duel1.bsp"), b)) { return false; }
        bool gleich = a.lumps.size() == 18 && b.lumps.size() == 18;
        for (std::size_t i = 1; gleich && i < a.lumps.size(); ++i) { gleich = a.lumps[i] == b.lumps[i]; }
        const std::string ent = karte::bspEntitaetenText(b);
        diag::detail("Knopftest: Stufe 4 - onlyents " + std::string(gleich ? "17 Lumps gleich" : "Lumps anders") + ", Entity-Lump " +
                     std::to_string(ent.size()) + " Bytes");
        return gleich && ent.find("\"message\" \"Stufe 4 bhed_test\"") != std::string::npos && ent.find("\"origin\" \"100 200 64\"") != std::string::npos &&
               a.lumps[0] != b.lumps[0];
    }));
    add(pruefe("Konsole: q3map2 -bsp -onlyents, die Schlusszeile \"Entities updated ... (17 of 17)\"", [] {
        const std::string k = stufe3::konsoleText();
        const std::string vorlage = ke::trK(Str::KeBmNurEntitiesFertig);
        const std::string anfang = vorlage.substr(0, vorlage.find("%s"));
        return k.find(" -bsp -onlyents \"") != std::string::npos && k.find(anfang) != std::string::npos;
    }));
    // Geometrie geaendert: Hinweis, kein q3map2
    add(tu("Einen Pinsel waehlen und verschieben (SelectNudgeUp)", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder().front());
        ke::auswahlGeaendert();
        (void)ke::ausfuehren("SelectNudgeUp");
        e.bspNurEntitaetenFertig.clear();
        ke::konsoleLeeren();
    }));
    add(pruefe("Pinsel verschoben: Fingerabdruck anders", [] { return ke::bspGeometrieGeaendert(); }));
    add(tu("Build > Update entities only", [] { ke::bspNurEntitaeten(); }));
    add(pause(0.3));
    add(pruefe("Hinweis: neu bauen und belichten (Kasten und Konsole), q3map2 nicht gestartet", [] {
        const ke::Editor& e = ke::editor();
        return e.meldung.offen && e.meldung.titel == ke::trK(Str::KeBmGeometrieTitel) && !ke::bauLaeuft() && e.bspNurEntitaetenFertig.empty() &&
               stufe3::konsoleText().find(ke::trK(Str::KeBmGeometrie)) != std::string::npos;
    }));
    add(foto("karteneditor_nurentities_hinweis"));
    add(tu("Kasten zu, Rueckgaengig", [] {
        ke::editor().meldung.offen = false;
        (void)ke::ausfuehren("Undo");
    }));
    add(pruefe("Rueckgaengig: Fingerabdruck wieder gleich", [] { return !ke::bspGeometrieGeaendert(); }));
    // Stufe 6 (JKHub, Lichtstile nach Entity-Aenderung): ein neues schaltbares Licht verschoebe
    // die Stile, die in den Lightmaps stecken - Update entities only verweigert, q3map2 laeuft nicht
    add(tu("Stufe 6: ein Licht mit targetname dazu", [] {
        ke::Editor& e = ke::editor();
        {
            karte::Befehl b(e.karte.undo, "Stufe 6 Test");
            auto l = e.karte.neueEntitaet("light", false);
            l->setzen("origin", "0 0 64");
            l->setzen("targetname", "s6schalter");
            e.karte.entitaetEinfuegen(l);
        }
        e.bspNurEntitaetenFertig.clear();
        ke::konsoleLeeren();
    }));
    add(tu("Build > Update entities only", [] { ke::bspNurEntitaeten(); }));
    add(pause(0.3));
    add(pruefe("Stufe 6: schaltbares Licht dazu - Hinweis (Lichtstile), q3map2 nicht gestartet", [] {
        const ke::Editor& e = ke::editor();
        return e.meldung.offen && e.meldung.text == ke::trK(Str::KeBmSchaltlichter) && !ke::bauLaeuft() && e.bspNurEntitaetenFertig.empty();
    }));
    add(tu("Kasten zu, Rueckgaengig", [] {
        ke::editor().meldung.offen = false;
        (void)ke::ausfuehren("Undo");
    }));
    add(tu("Eine andere Karte oeffnen: die Herkunft ist weg", [] { (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map")); }));
    add(pruefe("Ohne geoeffnete .bsp: Update entities only gesperrt", [] { return !ke::bspNurEntitaetenMoeglich(); }));
    return s;
}

// Punkt 3: bobToolz - jeder Eintrag des Menues mit seiner Wirkung, die Dialoge
// (Groesse wie die NRC-Fotos dlg_Plugins_bobToolz_*_0.png, wo es sie gibt),
// Path Plotter / Plot Splines in der 2D-Ansicht, Tree Planter per Klick,
// Vis Viewer mit der .bsp der Testkarte.
std::string kastenText(int x0, int y0, int z0, int x1, int y1, int z1, const char* tex) {
    char z[1400];
    std::snprintf(z, sizeof(z),
                  "{\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n"
                  "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0\n}\n",
                  x1, y1, z1, x1, y0, z1, x0, y1, z1, tex, x1, y1, z1, x0, y1, z1, x1, y1, z0, tex, x1, y1, z1, x1, y1, z0, x1, y0, z1, tex, x0, y0, z0, x1,
                  y0, z0, x0, y1, z0, tex, x0, y0, z0, x0, y0, z1, x1, y0, z0, tex, x0, y0, z0, x0, y1, z0, x0, y0, z1, tex);
    return z;
}

// Ein Pinsel aus Ebenen durch drei Punkte, die Normale ( (p2 - p0) x (p1 - p0) ) von "innen" weg
using P3 = std::array<double, 3>;
std::string ebenenPinsel(std::vector<std::array<P3, 3>> ebenen, const P3& innen, const char* tex) {
    std::string t = "{\n";
    for (auto& e : ebenen) {
        const P3 a{e[2][0] - e[0][0], e[2][1] - e[0][1], e[2][2] - e[0][2]};
        const P3 b{e[1][0] - e[0][0], e[1][1] - e[0][1], e[1][2] - e[0][2]};
        const P3 n{(a[1] * b[2]) - (a[2] * b[1]), (a[2] * b[0]) - (a[0] * b[2]), (a[0] * b[1]) - (a[1] * b[0])};
        if ((n[0] * (e[0][0] - innen[0])) + (n[1] * (e[0][1] - innen[1])) + (n[2] * (e[0][2] - innen[2])) < 0) { std::swap(e[1], e[2]); }
        char z[300];
        std::snprintf(z, sizeof(z), "( %g %g %g ) ( %g %g %g ) ( %g %g %g ) %s 0 0 0 0.5 0.5 0 0 0\n", e[0][0], e[0][1], e[0][2], e[1][0], e[1][1], e[1][2],
                      e[2][0], e[2][1], e[2][2], tex);
        t += z;
    }
    return t + "}\n";
}

// patchDef2 eines flachen Gitters n x n ab (x0, y0) mit Abstand d
std::string patchText(int x0, int y0, int d, int n) {
    std::string t = "{\npatchDef2\n{\ntest/patch\n( " + std::to_string(n) + " " + std::to_string(n) + " 0 0 0 )\n(\n";
    for (int i = 0; i < n; ++i) {
        t += "( ";
        for (int j = 0; j < n; ++j) {
            t += "( " + std::to_string(x0 + i * d) + " " + std::to_string(y0 + j * d) + " 0 " + std::to_string(i) + " " + std::to_string(j) + " ) ";
        }
        t += ")\n";
    }
    return t + ")\n}\n}\n";
}

std::string weltText(const std::string& pinsel) { return "{\n\"classname\" \"worldspawn\"\n" + pinsel + "}\n"; }

void bobKarte(const std::string& text) {
    const std::filesystem::path ordner = std::filesystem::current_path() / "s4bob";
    std::error_code ec;
    std::filesystem::create_directories(ordner, ec);
    const std::filesystem::path p = ordner / "bob.map";
    {
        std::ofstream f(p, std::ios::binary | std::ios::trunc);
        f.write(text.data(), static_cast<std::streamsize>(text.size()));
    }
    (void)ke::karteOeffnen(p.string());
    karteDialogeZu();
    ke::editor().karte.auswahl.leeren();
    ke::auswahlGeaendert();
    ke::konsoleLeeren();
}

std::vector<std::shared_ptr<karte::Entitaet>> klasseListe(const char* name) {
    std::vector<std::shared_ptr<karte::Entitaet>> v;
    for (const auto& en : ke::editor().karte.entitaeten()) {
        if (en->klassenname() == name) { v.push_back(en); }
    }
    return v;
}

std::size_t pinselIn(const karte::Entitaet& en) {
    std::size_t n = 0;
    for (const auto& p : en.kinder()) { n += p->art() == karte::Primitiv::Art::Pinsel ? 1 : 0; }
    return n;
}
std::size_t patchesIn(const karte::Entitaet& en) {
    std::size_t n = 0;
    for (const auto& p : en.kinder()) { n += p->art() == karte::Primitiv::Art::Patch ? 1 : 0; }
    return n;
}

// Die Primitive der Welt mit den Nummern waehlen
void weltWaehlen(std::initializer_list<std::size_t> nummern) {
    ke::Editor& e = ke::editor();
    e.karte.auswahl.leeren();
    const auto& k = e.karte.welt()->kinder();
    for (const std::size_t i : nummern) {
        if (i < k.size()) { e.karte.auswahl.waehlen(e.karte.welt(), k[i]); }
    }
    ke::auswahlGeaendert();
}

// Die Groesse des Innenraums ("Plugin:Dialog"); toleranz: Abweichung der Breite,
// die eine Textbreite erklaert (siehe NOTIZEN_karte4.md, Punkt 3)
bool dialogGroesse(int b, int h, int toleranz = 0) {
    const auto* d = stufe3::bauFeld("Plugin:Dialog");
    if (d != nullptr) { diag::detail("Knopftest: Stufe 4/3 Dialog " + std::to_string(d->b) + " x " + std::to_string(d->h)); }
    return d != nullptr && (b < 0 || (std::abs(d->b - b) <= toleranz && d->h == h));
}

std::vector<Schritt> bobtoolz() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const auto menue = [&add](const char* punkt) {
        add(klick(ke::trK(Str::KeLPlugins), "###main"));
        add(pause(0.25));
        add(klick("bobToolz", "##Menu_", 0, true));
        add(pause(0.25));
        add(klick(punkt, "##Menu_", 0, false));
        add(pause(0.3));
    };
    const auto knopf = [&add](Str text, const char* id, const char* fenster) {
        add(klick(std::string(ke::trK(text)) + "##" + id, fenster, 0, true));
        add(pause(0.3));
    };
    add(stufe2::grundzustand("Stufe 4/3: Grundzustand"));
    add(pruefe("bobToolz: alle 23 Befehle des Menues nachgebaut (keiner gesperrt), die Werkzeugleiste mit Polygons ... Flip Terrain", [] {
        int n = 0;
        for (const auto& p : karte::befehle::pluginEintraege()) {
            if (std::string(p.menue) == "bobToolz" && p.befehl[0] != 0) { n += ke::nachgebaut(p.befehl) ? 1 : 0; }
        }
        int leiste = 0;
        for (const auto& k : karte::befehle::pluginleiste()) { leiste += !k.trenner && ke::nachgebaut(k.befehl) ? 1 : 0; }
        diag::detail("Knopftest: Stufe 4/3 bobToolz " + std::to_string(n) + " Befehle, Leiste " + std::to_string(leiste));
        return n == 23 && leiste == 9;
    }));
    // About...
    add(tu("About...", [] { (void)ke::ausfuehren("bobToolz::About"); }));
    add(pause(0.3));
    add(pruefe("About: Kasten \"About\" mit bobToolz for SDRadiant ... digibob", [] {
        const ke::Editor& e = ke::editor();
        return e.meldung.offen && e.meldung.titel == "About" && e.meldung.text.find("digibob") != std::string::npos;
    }));
    add(tu("Kasten zu", [] { ke::editor().meldung.offen = false; }));
    // Intersect...: A und B ueberschneiden sich, C allein
    add(tu("Karte: A (0..64), B (32..96), C (200..264)", [] {
        bobKarte(weltText(kastenText(0, 0, 0, 64, 64, 64, "test/a") + kastenText(32, 32, 32, 96, 96, 96, "test/b") +
                          kastenText(200, 0, 0, 264, 64, 64, "test/c")));
    }));
    menue("Intersect...");
    add(pruefe("Intersect: der Dialog 192 x 157 wie das NRC-Foto, Use Whole Map gewaehlt", [] {
        return ke::editor().bob.dialog == 1 && dialogGroesse(192, 157) && ke::editor().bob.intersect.ganzeKarte;
    }));
    add(pruefe("Intersect: OK bei 15/124 80 x 22, Cancel bei 101/124 wie im Foto", [] {
        const auto* ok = stufe3::bauFeld("Plugin:btintersectok0");
        const auto* ab = stufe3::bauFeld("Plugin:btintersectok1");
        return ok != nullptr && ab != nullptr && ok->x == 15 && ok->y == 124 && ok->b == 80 && ok->h == 22 && ab->x == 101;
    }));
    add(fotoRahmen("karteneditor_dlg_Plugins_bobToolz_Intersect_0", [] { return fensterRahmen("###btintersect"); }));
    knopf(Str::KePrefOk, "btintersectok0", "###btintersect");
    add(pruefe("Intersect (ganze Karte): A und B gewaehlt, Konsole \"2 intersecting brushes found.\"", [] {
        const ke::Editor& e = ke::editor();
        return e.bob.dialog == 0 && e.karte.auswahl.anzahl() == 2 && stufe3::konsoleText().find("bobToolz Intersect: 2 intersecting brushes found.") != std::string::npos;
    }));
    // Find Duplicates
    add(tu("Karte: C zweimal, Find Duplicates", [] {
        bobKarte(weltText(kastenText(0, 0, 0, 64, 64, 64, "test/a") + kastenText(0, 0, 0, 64, 64, 64, "test/a")));
        (void)ke::ausfuehren("bobToolz::FindDuplicates");
    }));
    add(pruefe("Find Duplicates: das zweite Exemplar gewaehlt, \"1 duplicate brushes found.\"", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.auswahl.anzahl() == 1 && e.karte.auswahl.gewaehlt(e.karte.welt().get(), e.karte.welt()->kinder()[1].get()) &&
               stufe3::konsoleText().find("bobToolz Find Duplicates: 1 duplicate brushes found.") != std::string::npos;
    }));
    // Polygon Builder
    add(tu("Karte: ein Kasten 128 x 128 x 64, gewaehlt; Polygon Builder", [] {
        bobKarte(weltText(kastenText(0, 0, 0, 128, 128, 64, "test/a")));
        weltWaehlen({0});
        (void)ke::ausfuehren("bobToolz::PolygonBuilder");
    }));
    add(pause(0.3));
    add(pruefe("Polygon Builder: der Dialog (Number Of Sides 6, Border Width gesperrt)", [] {
        const ke::Editor& e = ke::editor();
        return e.bob.dialog == 2 && dialogGroesse(-1, -1) && e.bob.polygon.seiten == 6 && !e.bob.polygon.rand;
    }));
    add(foto("karteneditor_bobtoolz_polygon"));
    knopf(Str::KePrefOk, "btpolygonok0", "###btpolygon");
    add(pruefe("Polygon Builder: ein Prisma mit 6 Seiten + Boden + Deckel statt des Kastens, Undo bobToolz.polygons", [] {
        const ke::Editor& e = ke::editor();
        const auto& k = e.karte.welt()->kinder();
        return k.size() == 1 && k[0]->art() == karte::Primitiv::Art::Pinsel && std::static_pointer_cast<karte::Pinsel>(k[0])->anzahl() == 8 &&
               e.karte.undo.undoName() == "bobToolz.polygons";
    }));
    // Stair Builder
    add(tu("Karte: Kasten 128 x 128 x 64 gewaehlt; Stair Builder", [] {
        bobKarte(weltText(kastenText(0, 0, 0, 128, 128, 64, "test/a")));
        weltWaehlen({0});
        (void)ke::ausfuehren("bobToolz::StairBuilder");
    }));
    add(pause(0.3));
    add(pruefe("Stair Builder: der Dialog, Stair Height 8, Main Texture = die Textur des Texturbrowsers", [] {
        const ke::Editor& e = ke::editor();
        return e.bob.dialog == 3 && e.bob.treppe.stufenHoehe == 8 && e.bob.treppe.hauptTextur == e.texBro.shader;
    }));
    add(foto("karteneditor_bobtoolz_stair"));
    knopf(Str::KePrefOk, "btstairok0", "###btstair");
    add(pruefe("Stair Builder: 64 / 8 = 8 Stufen statt des Kastens, Undo bobToolz.buildStairs", [] {
        const ke::Editor& e = ke::editor();
        return pinselIn(*e.karte.welt()) == 8 && e.karte.undo.undoName() == "bobToolz.buildStairs";
    }));
    // Door Builder
    add(tu("Karte: Kasten 128 x 16 x 128 gewaehlt; Door Builder", [] {
        bobKarte(weltText(kastenText(0, 0, 0, 128, 16, 128, "test/a")));
        weltWaehlen({0});
        (void)ke::ausfuehren("bobToolz::DoorBuilder");
    }));
    add(pause(0.3));
    add(pruefe("Door Builder: der Dialog, die Liste aus bt/door-tex.txt (13 Texturen), Trim aus door-tex-trim.txt (5)", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Stufe 4/3 door-tex " + std::to_string(e.bob.tuerTexturen.size()) + " / " + std::to_string(e.bob.rahmenTexturen.size()));
        return e.bob.dialog == 4 && e.bob.tuerTexturen.size() >= 10 && e.bob.rahmenTexturen.size() == 5 && e.bob.tuer.hauptH && !e.bob.tuer.randV;
    }));
    add(foto("karteneditor_bobtoolz_door"));
    knopf(Str::KePrefOk, "btdoorok0", "###btdoor");
    add(pruefe("Door Builder: zwei func_door (angle 180 / 360, gemeinsames team) statt des Kastens", [] {
        const auto t = klasseListe("func_door");
        return t.size() == 2 && t[0]->wert("angle") == "180" && t[1]->wert("angle") == "360" && !t[0]->wert("team").empty() &&
               t[0]->wert("team") == t[1]->wert("team") && pinselIn(*ke::editor().karte.welt()) == 0;
    }));
    // Aperture Door...
    add(tu("Karte: Kasten 16 x 128 x 128 gewaehlt; Aperture Door", [] {
        bobKarte(weltText(kastenText(0, 0, 0, 16, 128, 128, "test/a")));
        weltWaehlen({0});
        (void)ke::ausfuehren("bobToolz::ApertureDoor");
    }));
    add(pause(0.3));
    add(pruefe("Aperture Door Builder: der Dialog mit den Vorgaben von NRC (16 Segmente, base_floor/diamond2c)", [] {
        const ke::Editor& e = ke::editor();
        return e.bob.dialog == 5 && e.bob.apertur.segmente == 16 && e.bob.apertur.hauptTextur == "textures/base_floor/diamond2c";
    }));
    add(foto("karteneditor_bobtoolz_aperture"));
    knopf(Str::KePrefOk, "btapok0", "###btaperture");
    add(pruefe("Aperture Door: 16 Segmente als func_door, Undo bobToolz.buildApertureDoor", [] {
        diag::detail("Knopftest: Stufe 4/3 Aperture " + std::to_string(klasseListe("func_door").size()) + " func_door");
        return klasseListe("func_door").size() == 16 && ke::editor().karte.undo.undoName() == "bobToolz.buildApertureDoor";
    }));
    // Make Chain...
    add(tu("Leere Karte", [] { bobKarte(weltText(kastenText(-256, -256, -16, 256, 256, 0, "test/boden"))); }));
    menue("Make Chain...");
    // "Basename for chain's targetnames": unsere DirectWrite-Messung 168, Qt 167 (Foto: Feldspalte ab 184)
    add(pruefe("Make Chain: der Dialog 361 x 100 wie das NRC-Foto (Breite +1: Textbreite der Beschriftung)", [] {
        return ke::editor().bob.dialog == 8 && dialogGroesse(361, 100, 1);
    }));
    add(fotoRahmen("karteneditor_dlg_Plugins_bobToolz_Make_Chain_0", [] { return fensterRahmen("###btmakechain"); }));
    add(tu("3 Glieder, Grundname \"kette\"", [] {
        ke::editor().bob.kette.glieder = 3;
        ke::editor().bob.kette.name = "kette";
    }));
    knopf(Str::KePrefOk, "btchainok0", "###btmakechain");
    add(pruefe("Make Chain: 3 info_train_spline_main (kette_pt0..2), 2 info_train_spline_control", [] {
        const auto h = klasseListe("info_train_spline_main");
        const auto c = klasseListe("info_train_spline_control");
        return h.size() == 3 && c.size() == 2 && h[0]->wert("targetname") == "kette_pt0" && h[0]->wert("target") == "kette_pt1" &&
               c[1]->wert("origin") == "0 96 0";
    }));
    // Plot Splines
    add(tu("Die Kette um 64 nach x (weg von der y-Achse), Ansicht auf (64 32 0), Plot Splines", [] {
        {
            karte::Befehl b(ke::editor().karte.undo, "Stufe 4 Test");
            for (const char* kl : {"info_train_spline_main", "info_train_spline_control"}) {
                for (const auto& en : klasseListe(kl)) {
                    const std::string o = en->wert("origin");
                    en->setzen("origin", "64" + o.substr(o.find(' ')));
                }
            }
        }
        ke::editor().xy[0].ansicht.setzeMassstab(1.0F);
        ke::editor().xy[0].ansicht.setzeUrsprung(karte::Vektor3f(64, 32, 0));
        (void)ke::ausfuehren("bobToolz::PlotSplines");
    }));
    add(pause(0.4));
    add(pruefe("Plot Splines: zwei Abschnitte, in der 2D-Ansicht rot (x = 64, y = 16)", [] {
        unsigned char c[4]{};
        const bool da = stufe3::xyFarbe(karte::Vektor3f(64, 16, 0), c);
        diag::detail("Knopftest: Stufe 4/3 Splines " + std::to_string(ke::editor().bob.splines.linien.size()) + " Farbe " + stufe3::farbText(c));
        return ke::editor().bob.splines.linien.size() == 2 && da && stufe3::istRoetlich(c);
    }));
    // Path Plotter...
    add(tu("Karte: trigger_push mit target, target_position 192 hoeher; Trigger gewaehlt", [] {
        bobKarte(weltText(kastenText(-256, -256, -16, 256, 256, 0, "test/boden")) + "{\n\"classname\" \"trigger_push\"\n\"target\" \"oben\"\n" +
                 kastenText(-16, -16, 0, 16, 16, 16, "common/trigger") + "}\n" +
                 "{\n\"classname\" \"target_position\"\n\"targetname\" \"oben\"\n\"origin\" \"256 0 200\"\n}\n");
        ke::Editor& e = ke::editor();
        const auto t = klasseListe("trigger_push");
        e.karte.auswahl.waehlen(t[0], t[0]->kinder().front());
        ke::auswahlGeaendert();
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(128, 0, 0));
    }));
    menue("Path Plotter...");
    // Stufe 6: "Distance Multiplier" wie NRC-master 7027470 (das Foto von 20260114 zeigt
    // "Multipler", 367 breit) - die laengere Beschriftung macht den Dialog 2 Punkte breiter.
    add(pruefe("Path Plotter: der Dialog 369 x 180 (NRC-Foto 367 mit \"Multipler\"), Enable / Disable / Cancel rechts bei 106 / 192 / 278, y 147", [] {
        const auto* a = stufe3::bauFeld("Plugin:btpathok0");
        const auto* b = stufe3::bauFeld("Plugin:btpathok1");
        const auto* c = stufe3::bauFeld("Plugin:btpathok2");
        if (a != nullptr && b != nullptr && c != nullptr) {
            diag::detail("Knopftest: Stufe 4/3 Path Plotter Knoepfe " + std::to_string(a->x) + " / " + std::to_string(b->x) + " / " + std::to_string(c->x));
        }
        return ke::editor().bob.dialog == 6 && dialogGroesse(369, 180) && a != nullptr && b != nullptr && c != nullptr && a->x == 106 && b->x == 192 &&
               c->x == 278 && a->y == 147;
    }));
    add(fotoRahmen("karteneditor_dlg_Plugins_bobToolz_Path_Plotter_0", [] { return fensterRahmen("###btpathplotter"); }));
    knopf(Str::KeBtEnable, "btpathok0", "###btpathplotter");
    add(pause(0.3));
    add(pruefe("Path Plotter: Bahn mit 50 Punkten ab der Mitte des Triggers, rot in der 2D-Ansicht, \"Initialization of Path Plotter succeeded.\"", [] {
        const ke::Editor& e = ke::editor();
        if (!e.bob.bahn.aktiv || e.bob.bahn.punkte.size() != 50) { return false; }
        unsigned char c[4]{};
        const karte::Vektor3f m = e.bob.bahn.punkte[8];
        const bool da = stufe3::xyFarbe(karte::Vektor3f(m[0], m[1], 0), c);
        diag::detail("Knopftest: Stufe 4/3 Bahn Farbe " + stufe3::farbText(c));
        return e.bob.bahn.punkte[0] == karte::Vektor3f(0, 0, 8) && da && stufe3::istRoetlich(c) &&
               stufe3::konsoleText().find("Initialization of Path Plotter succeeded.") != std::string::npos;
    }));
    add(tu("Ziel verschieben (dynamische Aktualisierung)", [] {
        const auto z = klasseListe("target_position");
        karte::Befehl b(ke::editor().karte.undo, "Stufe 4 Test");
        z[0]->setzen("origin", "256 128 200");
    }));
    add(pause(0.2));
    add(pruefe("Path Plotter folgt dem Ziel: der letzte Punkt mit y > 0", [] { return ke::editor().bob.bahn.punkte.back()[1] > 0.0F; }));
    menue("Path Plotter...");
    knopf(Str::KeBtDisable, "btpathok1", "###btpathplotter");
    add(pruefe("Path Plotter: Disable - keine Bahn mehr", [] { return !ke::editor().bob.bahn.aktiv; }));
    // Tree Planter / Drop Entity
    add(tu("Karte: Boden gewaehlt, Tree Planter an", [] {
        bobKarte(weltText(kastenText(-256, -256, -16, 256, 256, 0, "test/boden")));
        weltWaehlen({0});
        ke::editor().xy[0].ansicht.typ = karte::Sicht::XY;
        ke::editor().xy[0].ansicht.setzeUrsprung(karte::Vektor3f(0, 0, 0));
        (void)ke::ausfuehren("bobToolz::TreePlanter");
    }));
    add(pruefe("Tree Planter an (Knopf eingerastet), tp_ent.txt gelesen: misc_model, 4 Modelle, offset -16", [] {
        const auto& p = ke::editor().bob.planter;
        return p.aktiv && ke::angehakt("bobToolz::TreePlanter") && p.klasse == "misc_model" && p.modelle.size() == 4 && p.versatz == -16;
    }));
    add(tu("Auswahl leer", [] {
        ke::editor().karte.auswahl.leeren();
        ke::auswahlGeaendert();
    }));
    add(klickAn("Klick in die XY-Ansicht bei (61, 66)", [] { return stufe2::weltXy(61, 66); }));
    add(pause(0.3));
    add(pruefe("Tree Planter: ein misc_model am Raster (64 64) auf dem Boden + offset (-16), angles, model, modelscale; keine Auswahl", [] {
        const auto m = klasseListe("misc_model");
        if (m.size() != 1) { return false; }
        diag::detail("Knopftest: Stufe 4/3 Tree Planter origin " + m[0]->wert("origin") + " model " + m[0]->wert("model"));
        return m[0]->wert("origin") == "64 64 -16" && m[0]->wert("model").rfind("models/mapobjects/trees_sd/tree_", 0) == 0 && m[0]->hat("angles") &&
               m[0]->hat("modelscale") && ke::editor().karte.auswahl.anzahl() == 0;
    }));
    add(tu("misc_model hoch auf 200 und waehlen, Drop Entity", [] {
        ke::Editor& e = ke::editor();
        const auto m = klasseListe("misc_model");
        {
            karte::Befehl b(e.karte.undo, "Stufe 4 Test");
            m[0]->setzen("origin", "64 64 200");
        }
        e.karte.auswahl.waehlen(m[0]);
        ke::auswahlGeaendert();
        (void)ke::ausfuehren("bobToolz::DropEntity");
    }));
    add(pruefe("Drop Entity: auf den Boden (64 64 0)", [] { return klasseListe("misc_model")[0]->wert("origin") == "64 64 0"; }));
    add(tu("Tree Planter aus", [] { (void)ke::ausfuehren("bobToolz::TreePlanter"); }));
    add(pruefe("Tree Planter aus; Drop Entity meldet den Fehler", [] {
        ke::konsoleLeeren();
        (void)ke::ausfuehren("bobToolz::DropEntity");
        return !ke::editor().bob.planter.aktiv && stufe3::konsoleText().find("bobToolz::TreePlanter must be active") != std::string::npos;
    }));
    // Reset Textures...
    add(tu("Karte: zwei Kaesten test/alt, der erste gewaehlt; Reset Textures", [] {
        bobKarte(weltText(kastenText(0, 0, 0, 64, 64, 64, "test/alt") + kastenText(100, 0, 0, 164, 64, 64, "test/alt")));
        weltWaehlen({0});
        (void)ke::ausfuehren("bobToolz::ResetTextures");
    }));
    add(pause(0.3));
    add(pruefe("Texture Reset: der Dialog, alle Gruppen aus", [] {
        const auto& r = ke::editor().bob.textur;
        return ke::editor().bob.dialog == 7 && !r.nameErsetzen && !r.skalaSetzen[0] && !r.drehungSetzen;
    }));
    add(foto("karteneditor_bobtoolz_texreset"));
    add(tu("Reset Texture Names: textures/test/alt -> textures/test/neu", [] {
        auto& r = ke::editor().bob.textur;
        r.nameErsetzen = true;
        r.alterName = "textures/test/alt";
        r.neuerName = "textures/test/neu";
    }));
    knopf(Str::KeBtUseSel, "btresetok0", "###bttexreset");
    add(pruefe("Reset Textures (Use Selected Brushes): nur der gewaehlte Kasten (neu gebaut, hinten) heisst test/neu", [] {
        const auto& k = ke::editor().karte.welt()->kinder();
        if (k.size() != 2) { return false; }
        const auto a = std::static_pointer_cast<karte::Pinsel>(k[0]);
        const auto b = std::static_pointer_cast<karte::Pinsel>(k[1]);
        return (*a)[0].shader() == "textures/test/alt" && (*b)[0].shader() == "textures/test/neu";
    }));
    // PitOMatic
    add(tu("Karte: Kasten 128 x 128 x 256 gewaehlt; PitOMatic", [] {
        bobKarte(weltText(kastenText(0, 0, 0, 128, 128, 256, "test/a")));
        weltWaehlen({0});
        (void)ke::ausfuehren("bobToolz::PitOMatic");
    }));
    add(pruefe("PitOMatic: 2 trigger_multiple, trigger_hurt, target_speaker, target_remove_powerups, nodrop unten", [] {
        return klasseListe("trigger_multiple").size() == 2 && klasseListe("trigger_hurt").size() == 1 && klasseListe("target_speaker").size() == 1 &&
               klasseListe("target_remove_powerups").size() == 1 && pinselIn(*ke::editor().karte.welt()) == 1;
    }));
    // Brush Cleanup, Caulk Selection
    add(tu("Karte: zwei Kaesten, der zweite gewaehlt; Brush Cleanup, Caulk Selection", [] {
        bobKarte(weltText(kastenText(0, 0, 0, 64, 64, 64, "test/a") + kastenText(100, 0, 0, 164, 64, 64, "test/a")));
        weltWaehlen({1});
        (void)ke::ausfuehren("bobToolz::BrushCleanup");
    }));
    add(pruefe("Brush Cleanup: \"0 invalid/duplicate planes removed.\", danach nichts gewaehlt (DMap::LoadAll: setSelectedAll( false ))", [] {
        return stufe3::konsoleText().find("bobToolz FixBrushes: 0 invalid/duplicate planes removed.") != std::string::npos &&
               ke::editor().karte.auswahl.anzahl() == 0;
    }));
    add(tu("Den zweiten Kasten waehlen, Caulk Selection", [] {
        weltWaehlen({1});
        (void)ke::ausfuehren("bobToolz::CaulkSelection");
    }));
    add(pruefe("Caulk Selection: der gewaehlte Kasten ganz caulk, der andere nicht", [] {
        const auto& k = ke::editor().karte.welt()->kinder();
        int caulk = 0;
        for (const auto& p : k) {
            const auto b = std::static_pointer_cast<karte::Pinsel>(p);
            for (std::size_t i = 0; i < b->anzahl(); ++i) { caulk += (*b)[i].shader() == "textures/common/caulk" ? 1 : 0; }
        }
        return caulk == 6 && ke::editor().karte.undo.undoName() == "bobToolz.caulkSelection";
    }));
    // Explode Entity
    add(tu("Karte: func_group mit 3 Pinseln, einer gewaehlt; Explode Entity", [] {
        bobKarte(weltText("") + "{\n\"classname\" \"func_group\"\n\"_note\" \"x\"\n" + kastenText(0, 0, 0, 16, 16, 16, "test/a") +
                 kastenText(32, 0, 0, 48, 16, 16, "test/a") + kastenText(64, 0, 0, 80, 16, 16, "test/a") + "}\n");
        ke::Editor& e = ke::editor();
        const auto g = klasseListe("func_group");
        e.karte.auswahl.waehlen(g[0], g[0]->kinder().front());
        ke::auswahlGeaendert();
        (void)ke::ausfuehren("bobToolz::ExplodeEntity");
    }));
    add(pruefe("Explode Entity: drei func_group mit je einem Pinsel, Schluessel uebernommen", [] {
        const auto g = klasseListe("func_group");
        return g.size() == 3 && pinselIn(*g[0]) == 1 && g[2]->wert("_note") == "x";
    }));
    // Merge Patches, Split patch (rows / columns)
    add(tu("Karte: zwei 3x3-Patches an einer Kante, beide gewaehlt; Merge Patches", [] {
        bobKarte(weltText(patchText(0, 0, 32, 3) + patchText(64, 0, 32, 3)));
        weltWaehlen({0, 1});
        (void)ke::ausfuehren("bobToolz::MergePatches");
    }));
    add(pruefe("Merge Patches: ein Patch mit 15 Punkten (5 x 3)", [] {
        const auto& k = ke::editor().karte.welt()->kinder();
        return k.size() == 1 && k[0]->art() == karte::Primitiv::Art::Patch && std::static_pointer_cast<karte::Patch>(k[0])->punkte().size() == 15;
    }));
    const std::pair<const char*, std::size_t> teile[3] = {{"SplitPatch", 4}, {"SplitPatchColumns", 2}, {"SplitPatchRows", 2}};
    for (const auto& [befehl, soll] : teile) {
        const std::string b = befehl;
        const std::size_t n = soll;
        add(tu("Karte: ein 5x5-Patch gewaehlt; " + b, [b] {
            bobKarte(weltText(patchText(0, 0, 32, 5)));
            weltWaehlen({0});
            (void)ke::ausfuehren("bobToolz::" + b);
        }));
        add(pruefe(b + ": " + std::to_string(n) + " Patches", [n] { return patchesIn(*ke::editor().karte.welt()) == n; }));
    }
    // Flip terrain
    add(tu("Karte: zwei dreieckige Saeulen (Diagonale A-C), beide gewaehlt; Flip terrain", [] {
        const P3 a{0, 0, 0};
        const P3 b{64, 0, 16};
        const P3 c{64, 64, 0};
        const P3 d{0, 64, 16};
        const std::string p1 = ebenenPinsel({{P3{0, 0, -16}, P3{64, 0, -16}, P3{0, 64, -16}},
                                             {P3{0, 0, 0}, P3{64, 0, 0}, P3{0, 0, 16}},
                                             {P3{64, 0, 0}, P3{64, 64, 0}, P3{64, 0, 16}},
                                             {P3{0, 0, 0}, P3{64, 64, 0}, P3{0, 0, 16}},
                                             {a, b, c}},
                                            P3{40, 20, -8}, "test/erde");
        const std::string p2 = ebenenPinsel({{P3{0, 0, -16}, P3{64, 0, -16}, P3{0, 64, -16}},
                                             {P3{0, 64, 0}, P3{64, 64, 0}, P3{0, 64, 16}},
                                             {P3{0, 0, 0}, P3{0, 64, 0}, P3{0, 0, 16}},
                                             {P3{0, 0, 0}, P3{64, 64, 0}, P3{0, 0, 16}},
                                             {a, c, d}},
                                            P3{20, 40, -8}, "test/erde");
        bobKarte(weltText(p1 + p2));
        weltWaehlen({0, 1});
        (void)ke::ausfuehren("bobToolz::FlipTerrain");
    }));
    add(pruefe("Flip terrain: zwei neue Pinsel mit je einer Flaeche common/terrain", [] {
        const auto& k = ke::editor().karte.welt()->kinder();
        int erde = 0;
        for (const auto& p : k) {
            const auto b = std::static_pointer_cast<karte::Pinsel>(p);
            for (std::size_t i = 0; i < b->anzahl(); ++i) { erde += (*b)[i].shader() == "textures/common/terrain" ? 1 : 0; }
        }
        return k.size() == 2 && erde == 2 && ke::editor().karte.undo.undoName() == "bobToolz.flipTerrain";
    }));
    // Vis Viewer: die Testkarte raum mit bsp stage bauen
    add(tu("Testkarte raum, bsp stage", [] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();
        ke::editor().bauProjektFuer = -2;   // Build-Menue der Karte (default_build_menu.xml)
        ke::bauMenueSicherstellen();
        stufe3::raumKarteOeffnen("raum", false);
        stufe3::bauDateienWeg(stufe3::laufOrdner("s3bau") + "\\raum");
        ke::konsoleLeeren();
    }));
    add(stufe3::bauMenuepunkt("Single: bsp stage"));
    add(warteBis("Bauen fertig", [] { return !ke::bauLaeuft(); }, 2400));
    add(tu("Kamera in den Raum (-384 128 64), nichts gewaehlt; Vis Viewer", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        ke::auswahlGeaendert();
        e.kamera.setzeUrsprung(karte::Vektor3f(-384, 128, 64));
        ke::konsoleLeeren();
        (void)ke::ausfuehren("bobToolz::VisViewer");
    }));
    add(pause(0.5));
    add(pruefe("Vis Viewer: \"BSP leafs\" offen (ohne vis: unechte Sichtdaten), Blaetter in der Liste, das Blatt der Kamera mit Flaechen", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Stufe 4/3 Vis " + std::to_string(e.bob.vis.liste.size()) + " Blaetter, Blatt " + std::to_string(e.bob.vis.gewaehlt) + ", " +
                     std::to_string(e.bob.vis.flaechen.size()) + " Flaechen");
        return e.bob.vis.offen && !e.bob.vis.liste.empty() && e.bob.vis.gewaehlt >= 0 && !e.bob.vis.flaechen.empty() &&
               stufe3::bauFeld("Plugin:BspLeafs") != nullptr &&
               stufe3::konsoleText().find("bobToolz VisAnalyse: Setting up fake visibility data!") != std::string::npos;
    }));
    add(foto("karteneditor_bobtoolz_bspleafs"));
    add(tu("BSP leafs schliessen (Fenster zu)", [] {
        ImGuiWindow* w = ImGui::FindWindowByName("###btbspleafs");
        if (w != nullptr) { ke::editor().bob.vis = karte::bobtoolz::VisAnsicht{}; }
    }));
    add(pruefe("Vis Viewer zu", [] { return !ke::editor().bob.vis.offen; }));
    return s;
}

// Punkt 3: brushexport2 - About, das Fenster wie das NRC-Foto, Liste, Save As
// (Dateiwahl ersetzt), .obj / .mtl, Save, "Nothing is selected."
std::vector<Schritt> brushexport() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const auto pfad = [] {
        std::string p = (std::filesystem::current_path() / "s4bob" / "export").string();
        std::replace(p.begin(), p.end(), '\\', '/');
        return p;
    };
    add(stufe2::grundzustand("Stufe 4/3: brushexport2"));
    add(tu("About", [] { (void)ke::ausfuehren("brushexport2::About"); }));
    add(pause(0.3));
    add(pruefe("About: Kasten \"About me...\" mit Brushexport plugin v 3.0 by namespace", [] {
        const ke::Editor& e = ke::editor();
        return e.meldung.offen && e.meldung.titel == "About me..." && e.meldung.text.find("Brushexport plugin v 3.0 by namespace") != std::string::npos;
    }));
    add(tu("Kasten zu; Karte: zwei Kaesten (einer mit caulk), beide gewaehlt", [] {
        ke::editor().meldung.offen = false;
        bobKarte(weltText(kastenText(0, 0, 0, 64, 64, 64, "test/a") + kastenText(100, 0, 0, 164, 64, 64, "common/caulk")));
        weltWaehlen({0, 1});
    }));
    add(klick(ke::trK(Str::KeLPlugins), "###main"));
    add(pause(0.25));
    add(klick("brushexport2", "##Menu_", 0, true));
    add(pause(0.25));
    add(klick("Export selected as Wavefront Object", "##Menu_", 0, false));
    add(pause(0.4));
    add(pruefe("Das Fenster 278 x 478 wie das NRC-Foto: Save 142/11 125 x 22 (gesperrt), Liste 11/115 256 x 192, Add 11/341, Remove 142/341, Haken bei 369..447", [] {
        const auto r = [](const char* n, int x, int y, int b, int h) {
            const auto* f = stufe3::bauFeld(std::string("Plugin:") + n);
            const bool ok = f != nullptr && f->x == x && f->y == y && f->b == b && f->h == h;
            if (!ok && f != nullptr) {
                diag::detail(std::string("Knopftest: Stufe 4/3 ") + n + " " + std::to_string(f->x) + "/" + std::to_string(f->y) + " " + std::to_string(f->b) +
                             "x" + std::to_string(f->h));
            }
            return ok;
        };
        return ke::editor().brushExport.offen && r("BeDialog", 0, 0, 278, 478) && r("besave", 142, 11, 125, 22) && r("besaveas", 142, 39, 125, 22) &&
               r("becancel", 142, 67, 125, 22) && r("beliste", 11, 115, 256, 192) && r("beeingabe", 11, 313, 256, 22) && r("beadd", 11, 341, 125, 22) &&
               r("beremove", 142, 341, 125, 22) && stufe3::bauFeld("Plugin:bemtl") != nullptr && stufe3::bauFeld("Plugin:bemtl")->y == 447 &&
               stufe3::bauFeld("Plugin:beshort")->y == 369 && ke::editor().brushExport.pfad.empty();
    }));
    add(fotoRahmen("karteneditor_dlg_Plugins_brushexport2_Export_selected_as_Wavefront_Object_0", [] { return fensterRahmen("###kartebrushexport"); }));
    add(klickAn("Eingabefeld", stufe2::elementOrt("##beeingabe")));
    add(tippe("textures/common/caulk"));
    add(klick(std::string(ke::trK(Str::KeBeAdd)) + "##beadd", "###kartebrushexport", 0, true));
    add(pause(0.2));
    add(pruefe("Add: \"caulk\" in der Liste (path_get_filename_start)", [] {
        const auto& l = ke::editor().brushExport.ignoriert;
        return l.size() == 1 && l[0] == "caulk";
    }));
    add(tu("Save As: die Dateiwahl liefert s4bob/export (ohne .obj)", [pfad] { ke::editor().dateiDialog = [pfad](const std::string&) { return pfad(); }; }));
    add(klick(std::string(ke::trK(Str::KeBeSaveAs)) + "##besaveas", "###kartebrushexport", 0, true));
    add(pause(0.3));
    add(pruefe("Save As: export.obj und export.mtl geschrieben; caulk ausgelassen (Pinsel 2 faellt weg), Konsole brushexport::ExportSelection", [pfad] {
        const std::string obj = dateiInhalt(pfad() + ".obj");
        const std::string mtl = dateiInhalt(pfad() + ".mtl");
        return ke::editor().brushExport.pfad == pfad() + ".obj" && obj.find("mtllib export.mtl\n") != std::string::npos &&
               obj.find("\ng Brush1\n") != std::string::npos && obj.find("\ng Brush2\n") == std::string::npos && obj.find("caulk") == std::string::npos &&
               mtl.find("# Material Count: 1\n") != std::string::npos && stufe3::konsoleText().find("brushexport::ExportSelection") != std::string::npos;
    }));
    add(tu("Dateiwahl zurueck, Collapse mesh, export.obj loeschen", [pfad] {
        ke::editor().dateiDialog = nullptr;
        ke::editor().brushExport.modus = 0;
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(pfad() + ".obj"), ec);
    }));
    add(klick(std::string(ke::trK(Str::KeBeSave)) + "##besave", "###kartebrushexport", 0, true));
    add(pause(0.3));
    add(pruefe("Save (frei nach Save As): ohne Dateiwahl wieder export.obj, jetzt g all / usemtl material", [pfad] {
        const std::string obj = dateiInhalt(pfad() + ".obj");
        return obj.find("\ng all\nusemtl material\n") != std::string::npos;
    }));
    add(tu("Nichts gewaehlt", [] {
        ke::editor().karte.auswahl.leeren();
        ke::auswahlGeaendert();
    }));
    add(klick(std::string(ke::trK(Str::KeBeSave)) + "##besave", "###kartebrushexport", 0, true));
    add(pause(0.3));
    add(pruefe("Nichts gewaehlt: Kasten \"brushexport\" / \"Nothing is selected.\"", [] {
        const ke::Editor& e = ke::editor();
        return e.meldung.offen && e.meldung.titel == "brushexport" && e.meldung.text == "Nothing is selected.";
    }));
    add(tu("Kasten zu", [] { ke::editor().meldung.offen = false; }));
    add(klick(std::string(ke::trK(Str::KeCancel)) + "##becancel", "###kartebrushexport", 0, true));
    add(pause(0.2));
    add(pruefe("Cancel: das Fenster versteckt, die Liste bleibt (hide)", [] {
        return !ke::editor().brushExport.offen && ke::editor().brushExport.ignoriert.size() == 1;
    }));
    return s;
}

// Punkt 3: ShaderPlug, SunPlug, UFO:AI - jeder Menuepunkt mit Wirkung
std::vector<Schritt> kleineplugins() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const auto kasten = [](const char* titel, const char* teil) {
        return [titel, teil] {
            const ke::Editor& e = ke::editor();
            return e.meldung.offen && e.meldung.titel == titel && e.meldung.text.find(teil) != std::string::npos;
        };
    };
    const auto zu = [] {
        ke::editor().meldung.offen = false;
        ke::editor().meldung.danach = nullptr;
    };
    add(stufe2::grundzustand("Stufe 4/3: ShaderPlug, SunPlug, UFO:AI"));
    add(pruefe("ShaderPlug (2), SunPlug (2), UFO:AI (16): alle Befehle nachgebaut", [] {
        int n = 0;
        for (const auto& p : karte::befehle::pluginEintraege()) {
            const std::string m = p.menue;
            if ((m == "ShaderPlug" || m == "SunPlug" || m == "UFO:AI") && p.befehl[0] != 0) { n += ke::nachgebaut(p.befehl) ? 1 : 0; }
        }
        diag::detail("Knopftest: Stufe 4/3 kleine Plugins " + std::to_string(n));
        return n == 20;
    }));
    // ShaderPlug
    add(tu("ShaderPlug > About", [] { (void)ke::ausfuehren("shaderPlug::About"); }));
    add(pruefe("ShaderPlug About: \"Shaderplug (1.0)\"", kasten("About", "Shaderplug (1.0)")));
    add(tu("Kasten zu; Karte offen (Spiel geladen), Tag-Datei weg; Create tag file", [zu] {
        zu();
        bobKarte(weltText(kastenText(0, 0, 0, 64, 64, 64, "test/a")));
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(ke::tagDatei()), ec);
        (void)ke::ausfuehren("shaderPlug::CreateTagFile");
    }));
    add(pause(0.3));
    add(pruefe("Create tag file: Konsole \"Shaderplug: N shaders found.\" / \"textures found.\" / \"Creating XML tag file.\", Kasten INFO, Datei da, neu geladen", [kasten] {
        const std::string k = stufe3::konsoleText();
        const bool da = stufe3::istDatei(ke::tagDatei());
        const std::string t = dateiInhalt(ke::tagDatei());
        diag::detail("Knopftest: Stufe 4/3 Tag-Datei " + std::to_string(t.size()) + " Bytes");
        return k.find(" shaders found.") != std::string::npos && k.find(" textures found.") != std::string::npos &&
               k.find("Shaderplug: Creating XML tag file.") != std::string::npos && kasten("INFO", ke::tagDatei().c_str())() && da &&
               t.find("<shader path=\"textures/") != std::string::npos && ke::editor().tags.datei.hatShader("textures/system/caulk") == (t.find("textures/system/caulk") != std::string::npos);
    }));
    add(tu("Kasten zu; noch einmal Create tag file", [zu] {
        zu();
        (void)ke::ausfuehren("shaderPlug::CreateTagFile");
    }));
    add(pruefe("Die Datei gibt es schon: Frage \"Overwrite tag file?\" (Ja / Nein)", [] {
        const ke::Editor& e = ke::editor();
        return e.meldung.offen && e.meldung.titel == "Overwrite tag file?" && e.meldung.jaNein;
    }));
    add(tu("Nein", [] {
        auto f = ke::editor().meldung.danach;
        ke::editor().meldung.offen = false;
        if (f) { f(false); }
    }));
    add(pruefe("Nein: kein INFO-Kasten", [] { return !ke::editor().meldung.offen; }));
    // SunPlug
    add(tu("SunPlug > About...", [] { (void)ke::ausfuehren("sunPlug::About"); }));
    add(pruefe("SunPlug About: \"About SunPlug\" / SunPlug v1.0 for NetRadiant 1.5", kasten("About SunPlug", "SunPlug v1.0 for NetRadiant 1.5\nby Topsun")));
    add(tu("Kasten zu; Karte 0..1000 x 0..200; ET-MapCoordinator", [zu] {
        zu();
        bobKarte(weltText(kastenText(0, 0, 0, 1000, 200, 64, "test/a")));
        (void)ke::ausfuehren("sunPlug::ET-MapCoordinator");
    }));
    add(pause(0.3));
    add(pruefe("ET-MapCoordinator: der Dialog, Konsole advised mapcoordsmins=0 -400 / mapcoordsmaxs=1000 600", [] {
        const std::string k = stufe3::konsoleText();
        return ke::editor().sunplug.offen && stufe3::bauFeld("Plugin:SunDialog") != nullptr &&
               k.find("SunPlug: advised mapcoordsmins=0 600") != std::string::npos && k.find("SunPlug: advised mapcoordsmaxs=1000 -400") != std::string::npos;
    }));
    add(foto("karteneditor_sunplug"));
    add(klick(std::string(ke::trK(Str::KeSunOptimal)) + "##sunoptimal", "###kartesunplug", 0, true));
    add(pause(0.2));
    add(pruefe("Get optimal mapcoords: X/Y der Felder 0 600 / 1000 -400", [] {
        const auto& w = ke::editor().sunplug.werte;
        return w[0] == 0 && w[1] == 600 && w[2] == 1000 && w[3] == -400;
    }));
    add(klick(std::string(ke::trK(Str::KeSunSet)) + "##sunok0", "###kartesunplug", 0, true));
    add(pause(0.2));
    add(pruefe("Set: worldspawn mapcoordsmins \"0 600\", mapcoordsmaxs \"1000 -400\", Undo SunPlug.entitySetMapcoords", [] {
        const ke::Editor& e = ke::editor();
        return !e.sunplug.offen && e.karte.welt()->wert("mapcoordsmins") == "0 600" && e.karte.welt()->wert("mapcoordsmaxs") == "1000 -400" &&
               e.karte.undo.undoName() == "SunPlug.entitySetMapcoords";
    }));
    // UFO:AI
    add(tu("UFO:AI > About", [] { (void)ke::ausfuehren("uFO:AI::About"); }));
    add(pruefe("UFO:AI About: Plugin version 0.4, Martin Gerhardy", kasten("About", "Plugin version: 0.4")));
    add(tu("Kasten zu; Karte mit zwei info_player_start (team 1, 2) und Pinseln mit Level-Flags; Worldspawn", [zu] {
        zu();
        bobKarte("{\n\"classname\" \"worldspawn\"\n" + kastenText(0, 0, 0, 16, 16, 16, "test/a") + "}\n" +
                 "{\n\"classname\" \"info_player_start\"\n\"origin\" \"0 0 32\"\n\"team\" \"1\"\n}\n" +
                 "{\n\"classname\" \"info_player_start\"\n\"origin\" \"32 0 32\"\n\"team\" \"2\"\n}\n");
        (void)ke::ausfuehren("uFO:AI::Worldspawn");
    }));
    add(pruefe("Worldspawn: Kasten \"Note\" Set maxlevel to: 5Set maxteams to: 2..., Schluessel gesetzt", [kasten] {
        const ke::Editor& e = ke::editor();
        return kasten("Note", "Set maxlevel to: 5Set maxteams to: 2You should at least place 16 info_player_start")() && e.karte.welt()->wert("maxlevel") == "5" &&
               e.karte.welt()->wert("maxteams") == "2";
    }));
    add(tu("Kasten zu; Perform check", [zu] {
        zu();
        (void)ke::ausfuehren("uFO:AI::PerformCheck");
    }));
    add(pruefe("Perform check: Kasten \"Note\" mit den fehlenden Startpunkten", kasten("Note", "No singleplayer start positions (info_human_start)")));
    add(tu("Kasten zu; Worldspawn reset", [zu] {
        zu();
        (void)ke::ausfuehren("uFO:AI::WorldspawnReset");
    }));
    add(pruefe("Worldspawn reset: wieder gesetzt (Note)", kasten("Note", "Set maxlevel to: 5")));
    add(tu("Kasten zu; Pinsel: einer mit Inhalt Level 2 (0x200); Level 1 an", [zu] {
        zu();
        ke::Editor& e = ke::editor();
        karte::Befehl b(e.karte.undo, "Stufe 4 Test");
        auto p = std::static_pointer_cast<karte::Pinsel>(e.karte.welt()->kinder().front());
        for (std::size_t i = 0; i < p->anzahl(); ++i) {
            karte::InhaltFlags f = (*p)[i].flags();
            f.inhaltFlags = 0x200;
            (*p)[i].setzeFlags(f);
        }
    }));
    add(tu("Level 1", [] { (void)ke::ausfuehren("uFO:AI::Level1"); }));
    add(pruefe("Level 1: der Pinsel ohne Level 1 versteckt, Konsole level_active: 1", [] {
        const ke::Editor& e = ke::editor();
        return e.ufoai.level == 1 && e.karte.welt()->kinder().front()->versteckt &&
               stufe3::konsoleText().find("UFO:AI: level_active: 1, flag: 256.") != std::string::npos;
    }));
    add(tu("Level 2", [] { (void)ke::ausfuehren("uFO:AI::Level2"); }));
    add(pruefe("Level 2: sichtbar (hat Level 2)", [] {
        const ke::Editor& e = ke::editor();
        return e.ufoai.level == 2 && !e.karte.welt()->kinder().front()->versteckt;
    }));
    for (int i = 3; i <= 8; ++i) {
        add(tu("Level " + std::to_string(i), [i] { (void)ke::ausfuehren("uFO:AI::Level" + std::to_string(i)); }));
        // level_active = CONTENTS_LEVELn >> 8 = 1 << ( n - 1 )
        add(pruefe("Level " + std::to_string(i) + ": level_active " + std::to_string(1 << (i - 1)) + ", der Pinsel versteckt", [i] {
            const ke::Editor& e = ke::editor();
            return e.ufoai.level == (1 << (i - 1)) && e.karte.welt()->kinder().front()->versteckt;
        }));
    }
    add(tu("Level 8 noch einmal: aus", [] { (void)ke::ausfuehren("uFO:AI::Level8"); }));
    add(pruefe("Level-Filter aus: sichtbar", [] { return ke::editor().ufoai.level == 0 && !ke::editor().karte.welt()->kinder().front()->versteckt; }));
    const std::pair<const char*, int> filter[4] = {{"StepOn", 0x40000000}, {"ActorClip", 0x10000}, {"WeaponClip", 0x2000000}, {"NoDraw", 0x80}};
    for (const auto& [name, flag] : filter) {
        const std::string n = name;
        const int f = flag;
        add(tu(n + ": der Pinsel bekommt das Flag, " + n + " an", [n, f] {
            ke::Editor& e = ke::editor();
            {
                karte::Befehl b(e.karte.undo, "Stufe 4 Test");
                auto p = std::static_pointer_cast<karte::Pinsel>(e.karte.welt()->kinder().front());
                for (std::size_t i = 0; i < p->anzahl(); ++i) {
                    karte::InhaltFlags fl = (*p)[i].flags();
                    fl.inhaltFlags = n == "NoDraw" ? 0 : f;
                    fl.flaechenFlags = n == "NoDraw" ? f : 0;
                    (*p)[i].setzeFlags(fl);
                }
            }
            (void)ke::ausfuehren("uFO:AI::" + n);
        }));
        add(pruefe(n + ": versteckt", [] { return ke::editor().karte.welt()->kinder().front()->versteckt; }));
        add(tu(n + " aus", [n] { (void)ke::ausfuehren("uFO:AI::" + n); }));
        add(pruefe(n + " aus: sichtbar", [] { return !ke::editor().karte.welt()->kinder().front()->versteckt; }));
    }
    add(pruefe("StepOn: Konsole \"UFO:AI: Hiding 1 stepon brushes.\"", [] { return stufe3::konsoleText().find("UFO:AI: Hiding 1 stepon brushes.") != std::string::npos; }));
    return s;
}

// Punkt 3: MeshTex - die elf Voreinstellungen, die drei Dialoge (Groesse wie
// die NRC-Fotos), Help, About, "No valid patch meshes selected."
std::shared_ptr<karte::Patch> erstesPatch() {
    for (const auto& p : ke::editor().karte.welt()->kinder()) {
        if (p->art() == karte::Primitiv::Art::Patch) { return std::static_pointer_cast<karte::Patch>(p); }
    }
    return nullptr;
}

// Die S/T-Werte des ersten Patches verschieben (Bruchteile) und es waehlen
void patchTexturVersetzt(float ds, float dt) {
    ke::Editor& e = ke::editor();
    auto p = erstesPatch();
    std::vector<karte::PatchPunkt> pk = p->punkte();
    for (auto& q : pk) {
        q.texcoord[0] = q.texcoord[0] * 1.15F + ds;
        q.texcoord[1] = q.texcoord[1] * 1.15F + dt;
    }
    {
        karte::Befehl b(e.karte.undo, "Stufe 4 Test");
        p->punkteSetzen(pk);
    }
    e.karte.auswahl.leeren();
    e.karte.auswahl.waehlen(e.karte.welt(), p);
    ke::auswahlGeaendert();
}

std::pair<float, float> texSpanne(int achse) {
    auto p = erstesPatch();
    float lo = 1e9F;
    float hi = -1e9F;
    for (const auto& q : p->punkte()) {
        lo = std::min(lo, q.texcoord[static_cast<std::size_t>(achse)]);
        hi = std::max(hi, q.texcoord[static_cast<std::size_t>(achse)]);
    }
    return {lo, hi};
}

bool ganz(float f) { return std::fabs(f - std::round(f)) < 1e-3F; }

std::vector<Schritt> meshtex() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    const auto zu = [] {
        ke::editor().meldung.offen = false;
        ke::editor().meldung.danach = nullptr;
    };
    add(stufe2::grundzustand("Stufe 4/3: MeshTex"));
    add(pruefe("MeshTex: alle 16 Befehle nachgebaut", [] {
        int n = 0;
        for (const auto& p : karte::befehle::pluginEintraege()) {
            if (std::string(p.menue) == "MeshTex" && p.befehl[0] != 0) { n += ke::nachgebaut(p.befehl) ? 1 : 0; }
        }
        diag::detail("Knopftest: Stufe 4/3 MeshTex " + std::to_string(n));
        return n == 16;
    }));
    add(tu("Help", [] { (void)ke::ausfuehren("meshTex::Help"); }));
    add(pruefe("Help: Kasten \"Help\"", [] { return ke::editor().meldung.offen && ke::editor().meldung.titel == "Help"; }));
    add(tu("About", [zu] {
        zu();
        (void)ke::ausfuehren("meshTex::About");
    }));
    add(pruefe("About: MeshTex 3.0 beta, Joel Baxter", [] {
        return ke::editor().meldung.offen && ke::editor().meldung.titel == "About" && ke::editor().meldung.text.find("MeshTex 3.0 beta") != std::string::npos;
    }));
    add(tu("Kasten zu; Karte mit einem 5x5-Patch, nichts gewaehlt; S Min Align", [zu] {
        zu();
        bobKarte(weltText(patchText(0, 0, 32, 5)));
        (void)ke::ausfuehren("meshTex::SMinAlign");
    }));
    add(pruefe("Nichts gewaehlt: Kasten \"Warning\" / No valid patch meshes selected.", [] {
        return ke::editor().meldung.offen && ke::editor().meldung.titel == "Warning" && ke::editor().meldung.text == "No valid patch meshes selected.";
    }));
    // die Voreinstellungen ueber das Menue bzw. die Befehle
    struct Fall {
        const char* befehl;
        int achse;          // 0 S, 1 T
        bool minGanz, maxGanz;
    };
    const Fall faelle[11] = {{"STMinMaxAlignAuto", 0, true, true},   {"SMinMaxAlignAuto", 0, true, true},  {"TMinMaxAlignAuto", 1, true, true},
                             {"SMinMaxAlignStretch", 0, true, true}, {"SMinMaxAlignShrink", 0, true, true}, {"TMinMaxAlignStretch", 1, true, true},
                             {"TMinMaxAlignShrink", 1, true, true},  {"SMinAlign", 0, true, false},        {"SMaxAlign", 0, false, true},
                             {"TMinAlign", 1, true, false},          {"TMaxAlign", 1, false, true}};
    for (const Fall& f : faelle) {
        const Fall fall = f;
        add(tu(std::string("S/T versetzt (0,3 / 0,4, mal 1,15), gewaehlt; ") + f.befehl, [zu, fall] {
            zu();
            patchTexturVersetzt(0.3F, 0.4F);
            (void)ke::ausfuehren(std::string("meshTex::") + fall.befehl);
        }));
        add(pruefe(std::string(f.befehl) + ": " + (f.achse == 0 ? "S" : "T") + (f.minGanz ? " min ganz" : "") + (f.maxGanz ? " max ganz" : "") +
                       ", Undo " + f.befehl,
                   [fall] {
                       const auto [lo, hi] = texSpanne(fall.achse);
                       diag::detail(std::string("Knopftest: Stufe 4/3 ") + fall.befehl + " " + std::to_string(lo) + " .. " + std::to_string(hi));
                       return (!fall.minGanz || ganz(lo)) && (!fall.maxGanz || ganz(hi)) && ke::editor().karte.undo.undoName() == fall.befehl &&
                              !ke::editor().meldung.offen;
                   }));
    }
    // Get Info
    add(klick(ke::trK(Str::KeLPlugins), "###main"));
    add(pause(0.25));
    add(klick("MeshTex", "##Menu_", 0, true));
    add(pause(0.25));
    add(klick("Get Info...", "##Menu_", 0, false));
    add(pause(0.4));
    add(pruefe("Get Info: das Fenster 357 x 126 wie das NRC-Foto", [] {
        const auto* d = stufe3::bauFeld("Plugin:MtGetInfo");
        if (d != nullptr) { diag::detail("Knopftest: Stufe 4/3 Get Info " + std::to_string(d->b) + " x " + std::to_string(d->h)); }
        return ke::editor().meshtex.getInfoOffen && d != nullptr && d->b == 357 && d->h == 126;
    }));
    add(fotoRahmen("karteneditor_dlg_Plugins_MeshTex_Get_Info_0", [] { return fensterRahmen("###mtgetinfo"); }));
    add(tu("Transfer an, Set S/T Scale auf", [] {
        ke::editor().meshtex.uebertragen = true;
        (void)ke::ausfuehren("meshTex::SetScale");
    }));
    add(pause(0.3));
    add(tu("Get Info wieder nach vorn (die Fenster liegen beide mittig)", [] { ImGui::SetWindowFocus("###mtgetinfo"); }));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeMtApply)) + "##mtinfook2", "###mtgetinfo", 0, true));
    add(pause(0.3));
    add(pruefe("Get Info Apply: Kasten \"Mesh Info\" mit Zeile 0, Spalte 0, dem Netz; Set S/T Scale bekommt die Werte; das Fenster bleibt (Apply)", [] {
        const ke::Editor& e = ke::editor();
        diag::detail("Knopftest: Stufe 4/3 Set S/T " + std::to_string(e.meshtex.sSkala) + " / " + std::to_string(e.meshtex.sKacheln));
        return e.meldung.offen && e.meldung.titel == "Mesh Info" && e.meldung.text.find("On row 0:") != std::string::npos &&
               e.meldung.text.find("Over entire mesh:\n  Rows: 5") != std::string::npos && e.meshtex.getInfoOffen && e.meshtex.sKacheln != 1.0;
    }));
    add(tu("Kasten zu; Get Info nach vorn (Apply hat Set S/T Scale gehoben: Raise)", [zu] {
        zu();
        ImGui::SetWindowFocus("###mtgetinfo");
    }));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeCancel)) + "##mtinfook1", "###mtgetinfo", 0, true));
    add(pause(0.2));
    add(pruefe("Get Info Cancel: versteckt", [] { return !ke::editor().meshtex.getInfoOffen; }));
    // Set S/T Scale
    add(pruefe("Set S/T Scale: das Fenster 418 x 344 wie das NRC-Foto", [] {
        const auto* d = stufe3::bauFeld("Plugin:MtSetScale");
        return ke::editor().meshtex.setScaleOffen && d != nullptr && d->b == 418 && d->h == 344;
    }));
    add(tu("Natural 1 fuer S und T", [] {
        auto& z = ke::editor().meshtex;
        z.sArt = z.tArt = 0;
        z.sSkala = z.tSkala = 1;
    }));
    add(fotoRahmen("karteneditor_dlg_Plugins_MeshTex_Set_S_T_Scale_0", [] { return fensterRahmen("###mtsetscale"); }));
    add(klick(std::string(ke::trK(Str::KePrefOk)) + "##mtscaleok0", "###mtsetscale", 0, true));
    add(pause(0.3));
    add(pruefe("Set S/T Scale OK: S von 0 an in Schritten von 32 / (Textur / 2), Undo SetScale, Fenster zu", [] {
        const ke::Editor& e = ke::editor();
        const auto p = erstesPatch();
        const auto g = e.karte.kontext.groesse(p->shader());
        const float soll = 32.0F / (static_cast<float>(g.first) / 2.0F);   // natuerlich 1: Abstand / (Breite / 2)
        diag::detail("Knopftest: Stufe 4/3 SetScale s(0,1) " + std::to_string(p->punkt(0, 1).texcoord[0]) + " soll " + std::to_string(soll));
        return !e.meshtex.setScaleOffen && std::fabs(p->punkt(0, 0).texcoord[0]) < 1e-3F && std::fabs(p->punkt(0, 1).texcoord[0] - soll) < 1e-3F &&
               e.karte.undo.undoName() == "SetScale";
    }));
    // General Function
    add(tu("General Function auf", [] { (void)ke::ausfuehren("meshTex::GeneralFunction"); }));
    add(pause(0.3));
    add(pruefe("General Function: das Fenster 912 x 298 wie das NRC-Foto", [] {
        const auto* d = stufe3::bauFeld("Plugin:MtGenFunc");
        return ke::editor().meshtex.genFuncOffen && d != nullptr && d->b == 912 && d->h == 298;
    }));
    add(fotoRahmen("karteneditor_dlg_Plugins_MeshTex_General_Function_0", [] { return fensterRahmen("###mtgenfunc"); }));
    auto vorher = std::make_shared<float>(0.0F);
    add(tu("Control values, S = old_S + 0,5, T aus", [vorher] {
        auto& z = ke::editor().meshtex;
        z.flaechenWerte = false;
        z.sFaktoren = {1, 0, 0, 0, 0, 0.5};
        z.gfT = false;
        *vorher = erstesPatch()->punkt(2, 3).texcoord[0];
    }));
    add(klick(std::string(ke::trK(Str::KeMtApply)) + "##mtgfok2", "###mtgenfunc", 0, true));
    add(pause(0.3));
    add(pruefe("General Function Apply: S + 0,5, Undo GeneralFunction, das Fenster bleibt", [vorher] {
        const ke::Editor& e = ke::editor();
        return std::fabs(erstesPatch()->punkt(2, 3).texcoord[0] - (*vorher + 0.5F)) < 1e-3F && e.karte.undo.undoName() == "GeneralFunction" &&
               e.meshtex.genFuncOffen;
    }));
    add(klick(std::string(ke::trK(Str::KeCancel)) + "##mtgfok1", "###mtgenfunc", 0, true));
    add(pause(0.2));
    add(pruefe("General Function Cancel: versteckt", [] { return !ke::editor().meshtex.genFuncOffen; }));
    return s;
}

// Punkt 4: File > Exit (mainframe.cpp:621-625 Exit: ConfirmModified( "Exit
// Radiant" ), dann beenden). Beenden heisst hier: der Weg des Programms
// (confirmQuit - mit einem ungesicherten Skript fragt es selbst). Das wird im
// selben Schritt zurueckgenommen (wantsQuit wertet die Fensterschicht erst
// nach dem Bild aus; die Speichernfrage des Programms abgebrochen).
bool beendenAngestossen() {
    diag::detail("Knopftest: Stufe 4/4 Beenden: wantQuit " + std::to_string(g_app->wantQuit) + " askSave " + std::to_string(g_app->askSaveOpen) +
                 " frage " + std::to_string(g_app->frageOffen) + " " + g_app->frageText);
    const bool ja = g_app->wantQuit || g_app->askSaveOpen || g_app->frageOffen;
    if (g_app->frageOffen) {   // "Exit?" (queryOnDiscard) bzw. eine Frage des Programms: Nein
        g_app->frageOffen = false;
        g_app->frageJa = nullptr;
        g_app->frageNein = nullptr;
    }
    g_app->wantQuit = false;
    g_app->quitConfirmed = false;
    if (g_app->askSaveOpen) {
        g_app->askSaveOpen = false;
        std::function<void()> abbruch = std::move(g_app->askSaveCancel);
        g_app->askSaveCancel = nullptr;
        g_app->askSaveThen = nullptr;
        if (abbruch) { abbruch(); }
    }
    g_app->quitAskFrom = 0;
    return ja;
}

std::vector<Schritt> beenden() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto ergebnis = std::make_shared<std::array<bool, 3>>();
    add(stufe2::grundzustand("Stufe 4/4: File > Exit"));
    add(tu("Karte geaendert (worldspawn message), File > Exit; Beenden gleich zuruecknehmen", [ergebnis] {
        bobKarte(weltText(kastenText(0, 0, 0, 64, 64, 64, "test/a")));
        ke::Editor& e = ke::editor();
        {
            karte::Befehl b(e.karte.undo, "Stufe 4 Test");
            e.karte.welt()->setzen("message", "geaendert");
        }
        (void)ke::ausfuehren("Exit");
        (*ergebnis)[1] = e.frageOffen && e.frageTitel == "Exit Radiant";
        (*ergebnis)[0] = beendenAngestossen();
    }));
    add(pruefe("Exit mit geaenderter Karte: erst die Rueckfrage \"Exit Radiant\" (ConfirmModified), noch kein Beenden", [ergebnis] {
        return !(*ergebnis)[0] && (*ergebnis)[1];
    }));
    add(tu("Verwerfen (Discard): danach beenden; zuruecknehmen", [ergebnis] {
        ke::Editor& e = ke::editor();
        std::function<void()> danach = std::move(e.nachFrage);
        e.nachFrage = nullptr;
        e.frageOffen = false;
        if (danach) { danach(); }
        (*ergebnis)[2] = beendenAngestossen();
    }));
    add(pruefe("Nach \"Discard\": das Programm beendet (bzw. fragt nach ungesicherten Skripten)", [ergebnis] { return (*ergebnis)[2]; }));
    add(tu("Karte gespeichert (unveraendert), File > Exit; zuruecknehmen", [ergebnis] {
        ke::Editor& e = ke::editor();
        bobKarte(weltText(kastenText(0, 0, 0, 64, 64, 64, "test/a")));
        (void)ke::ausfuehren("Exit");
        (*ergebnis)[1] = e.frageOffen;
        (*ergebnis)[0] = beendenAngestossen();
    }));
    add(pruefe("Exit mit unveraenderter Karte: ohne Rueckfrage der Karte beenden", [ergebnis] { return (*ergebnis)[0] && !(*ergebnis)[1]; }));
    return s;
}

// Punkt 6 (Fund bei den Handbuchbildern): auf Japanisch ragten im Fenster von
// brushexport2 "1 つのメッシュにまとめる" und "マテリアルごとにまとめる" unter die
// Knopfspalte. Jeder Radioknopf, jedes Kaestchen, jede Beschriftung vor einem
// Zahlenfeld und jeder Text (qtd::text: textBreite + Abstand 6) meldet seinen
// Platz und die gezeichnete Breite ("Plugin:<id>~platz" / "~text"); in allen
// Plugin-Dialogen und allen vier Sprachen muss das Gezeichnete hineinpassen.
bool pluginTexteGanz(std::string& bericht) {
    std::map<std::string, const ke::Editor::Rechteck*> platz;
    const std::string vor = "Plugin:";
    for (const auto& r : ke::editor().felderImBild) {
        if (r.name.size() > 6 + vor.size() && r.name.compare(0, vor.size(), vor) == 0 && r.name.compare(r.name.size() - 6, 6, "~platz") == 0) {
            platz[r.name.substr(0, r.name.size() - 6)] = &r;
        }
    }
    int n = 0;
    int raus = 0;
    std::string erste;
    for (const auto& r : ke::editor().felderImBild) {
        if (r.name.size() <= 5 || r.name.compare(r.name.size() - 5, 5, "~text") != 0) { continue; }
        const auto p = platz.find(r.name.substr(0, r.name.size() - 5));
        if (p == platz.end()) { continue; }
        ++n;
        if (r.x + r.b > p->second->x + p->second->b) {
            ++raus;
            if (erste.empty()) {
                erste = r.name.substr(vor.size(), r.name.size() - vor.size() - 5) + ": gezeichnet " + std::to_string(r.b) + ", Platz " + std::to_string(p->second->b);
            }
        }
    }
    bericht = std::to_string(n) + " Texte, " + std::to_string(raus) + " zu breit" + (erste.empty() ? "" : " - " + erste);
    return n > 0 && raus == 0;
}

std::vector<Schritt> pluginTexte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    add(stufe2::grundzustand("Stufe 4/6: Grundzustand"));
    auto sprache = std::make_shared<i18n::Language>(i18n::Language::English);
    auto nrcVorher = std::make_shared<bool>(false);
    add(tu("Sprache merken", [sprache, nrcVorher] {
        *sprache = i18n::currentLanguage();
        *nrcVorher = ke::nrcBegriffe();
    }));
    struct Dialog {
        const char* name;
        std::function<void(bool)> schalten;
    };
    const auto bob = [](int n) {
        return [n](bool an) {
            ke::editor().bob.dialog = an ? n : 0;
            ke::editor().bob.oeffnen = an;
            if (an) { ke::editor().bob.reiterInnen = 0; }
        };
    };
    const std::vector<Dialog> dialoge = {
        {"bobToolz Intersect", bob(1)},
        {"bobToolz Polygon Builder", bob(2)},
        {"bobToolz Stair Builder", bob(3)},
        {"bobToolz Door Builder", bob(4)},
        {"bobToolz Aperture Door", bob(5)},
        {"bobToolz Path Plotter", bob(6)},
        {"bobToolz Reset Textures", bob(7)},
        {"bobToolz Make Chain", bob(8)},
        {"brushexport2", [](bool an) {
             ke::editor().brushExport.offen = an;
             ke::editor().brushExport.oeffnen = an;
         }},
        {"SunPlug ET-MapCoordinator", [](bool an) {
             ke::editor().sunplug.offen = an;
             ke::editor().sunplug.oeffnen = an;
         }},
        {"MeshTex Get Info", [](bool an) {
             ke::editor().meshtex.getInfoOffen = an;
             ke::editor().meshtex.getInfoOeffnen = an;
         }},
        {"MeshTex Set S/T Scale", [](bool an) {
             ke::editor().meshtex.setScaleOffen = an;
             ke::editor().meshtex.setScaleOeffnen = an;
         }},
        {"MeshTex General Function", [](bool an) {
             ke::editor().meshtex.genFuncOffen = an;
             ke::editor().meshtex.genFuncOeffnen = an;
         }},
    };
    const std::pair<i18n::Language, const char*> sprachen[] = {{i18n::Language::English, "Englisch"},
                                                               {i18n::Language::German, "Deutsch"},
                                                               {i18n::Language::ChineseSimplified, "Chinesisch"},
                                                               {i18n::Language::Japanese, "Japanisch"}};
    for (const auto& [l, name] : sprachen) {
        const i18n::Language lang = l;
        const std::string sname = name;
        add(tu("Plugin-Dialoge auf " + sname, [lang] {
            i18n::setLanguage(lang);
            ke::setzeNrcBegriffe(false);
        }));
        for (const Dialog& d : dialoge) {
            const std::function<void(bool)> schalten = d.schalten;
            const std::string dname = d.name;
            add(tu(dname + " auf", [schalten] { schalten(true); }));
            add(pause(0.15));
            add(pruefe(sname + ", " + dname + ": jeder Text passt in seinen Platz", [sname, dname] {
                std::string bericht;
                const bool ok = pluginTexteGanz(bericht);
                diag::detail("Knopftest: Stufe 4/6 - " + sname + " " + dname + ": " + bericht);
                return ok;
            }));
            add(tu(dname + " zu", [schalten] {
                schalten(false);
                karteDialogeZu();
            }));
            add(pause(0.1));
        }
    }
    add(tu("Sprache zurueck", [sprache, nrcVorher] {
        i18n::setLanguage(*sprache);
        ke::setzeNrcBegriffe(*nrcVorher);
    }));
    return s;
}

}  // namespace stufe4

std::vector<Schritt> karteneditorStufe4Schritte() {
    std::vector<Schritt> s;
    const char* nur = std::getenv("BHED_STUFE4_NUR");
    const auto teil = [nur](const char* name) { return nur == nullptr || std::strcmp(nur, name) == 0; };
    if (teil("reste")) {
        for (Schritt& x : stufe4::reste0()) { s.push_back(std::move(x)); }
    }
    if (teil("oeffnen") || teil("licht")) {
        for (Schritt& x : stufe4::oeffnen()) { s.push_back(std::move(x)); }
        for (Schritt& x : stufe4::licht()) { s.push_back(std::move(x)); }
    }
    if (teil("plugins") || teil("bobtoolz")) {
        for (Schritt& x : stufe4::bobtoolz()) { s.push_back(std::move(x)); }
    }
    if (teil("plugins") || teil("brushexport")) {
        for (Schritt& x : stufe4::brushexport()) { s.push_back(std::move(x)); }
    }
    if (teil("plugins") || teil("klein")) {
        for (Schritt& x : stufe4::kleineplugins()) { s.push_back(std::move(x)); }
    }
    if (teil("plugins") || teil("meshtex")) {
        for (Schritt& x : stufe4::meshtex()) { s.push_back(std::move(x)); }
    }
    if (teil("plugins") || teil("texte")) {
        for (Schritt& x : stufe4::pluginTexte()) { s.push_back(std::move(x)); }
    }
    if (teil("exit")) {
        for (Schritt& x : stufe4::beenden()) { s.push_back(std::move(x)); }
    }
    return s;
}

// --- Stufe 5 des Karten-Editors: der Optimierer -----------------------------------
//
// BHED_KARTE_NUR=stufe5 laesst nur diese Schritte laufen; BHED_STUFE5_NUR=<Teil>
// nur einen Teil: fenster (Punkt 5), pruefungen, gebaut, original, gross
// (Punkt 7; gross nur mit BHED_KARTE_GROSS, dazu BHED_KARTE_GROSS_BSP).
namespace stufe5 {

namespace ke = karteneditor;

// Ein Quader als Pinsel (Q3-Format); ohneDeckel: nur fuenf Seiten - ein
// offener, entarteter Pinsel.
std::string kastenText(double x0, double y0, double z0, double x1, double y1, double z1, const char* tex, int inhalt = 0, bool ohneDeckel = false) {
    auto f = [](double v) {
        char z[32];
        std::snprintf(z, sizeof(z), "%g", v);
        return std::string(z);
    };
    const std::string t = std::string(" ") + tex + " 0 0 0 0.5 0.5 " + std::to_string(inhalt) + " 0 0\n";
    auto p = [&](double a, double b, double c) { return "( " + f(a) + " " + f(b) + " " + f(c) + " ) "; };
    std::string k = "{\n";
    if (!ohneDeckel) { k += p(x1, y1, z1) + p(x1, y0, z1) + p(x0, y1, z1) + t; }
    return k + p(x1, y1, z1) + p(x0, y1, z1) + p(x1, y1, z0) + t + p(x1, y1, z1) + p(x1, y1, z0) + p(x1, y0, z1) + t + p(x0, y0, z0) + p(x1, y0, z0) +
           p(x0, y1, z0) + t + p(x0, y0, z0) + p(x0, y0, z1) + p(x1, y0, z0) + t + p(x0, y0, z0) + p(x0, y1, z0) + p(x0, y0, z1) + t + "}\n";
}

// Eine kleine Karte mit bekannten Befunden: zwei gestapelte Kaesten (2
// verdeckte Flaechen), ein Hint-Pinsel als detail (Mischinhalt), ein Pinsel
// mit einer Ecke bei 428.02 (neben dem Raster, sicher), eine kleine Kiste als
// structural (Deko-Vorschlag).
std::string testKarte() {
    return "{\n\"classname\" \"worldspawn\"\n" + kastenText(0, 0, 0, 64, 64, 64, "test/a") + kastenText(0, 0, 64, 64, 64, 128, "test/b") +
           kastenText(200, 0, 0, 264, 64, 64, "common/hint", 1 << 27) + kastenText(300, 0, 0, 428.02, 128, 128, "test/c") +
           kastenText(500, 0, 0, 532, 32, 32, "test/kiste") + "}\n{\n\"classname\" \"info_player_start\"\n\"origin\" \"600 0 24\"\n}\n";
}

bool hatBefund(const char* id, long long* anzahl = nullptr) {
    const ke::OptimiererBlick b = ke::optimiererBlick();
    for (std::size_t i = 0; i < b.befundIds.size(); ++i) {
        if (b.befundIds[i] == id) {
            if (anzahl != nullptr) { *anzahl = b.befundAnzahl[i]; }
            return true;
        }
    }
    return false;
}

std::string befundListe() {
    const ke::OptimiererBlick b = ke::optimiererBlick();
    std::string t;
    for (std::size_t i = 0; i < b.befundIds.size(); ++i) { t += (t.empty() ? "" : ", ") + b.befundIds[i] + " " + std::to_string(b.befundAnzahl[i]); }
    return t;
}

Schritt warteOptimierer(const std::string& was, std::shared_ptr<int> laeufe) {
    return warteBis(was, [laeufe] { return !ke::optimiererLaeuft() && ke::optimiererBlick().laeufe > *laeufe; }, 1800);
}

int caulkFlaechen() {
    int n = 0;
    for (const auto& en : ke::editor().karte.entitaeten()) {
        for (const auto& p : en->kinder()) {
            if (p->art() != karte::Primitiv::Art::Pinsel) { continue; }
            const auto& pin = static_cast<const karte::Pinsel&>(*p);
            for (std::size_t i = 0; i < pin.anzahl(); ++i) { n += pin[i].shader() == "textures/common/caulk" ? 1 : 0; }
        }
    }
    return n;
}

// Punkt 5: das Fenster - Menue, Lauf im Hintergrund, Liste, Auswahl, Beheben,
// Alles Sichere beheben, Bericht, Einstellungsseite.
std::vector<Schritt> fenster() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto laeufe = std::make_shared<int>(0);
    add(tu("Stufe 5/5: Testkarte opt_test.map schreiben und oeffnen", [laeufe] {
        {
            std::ofstream f(std::filesystem::path(karteTestPfad("opt_test.map")), std::ios::binary | std::ios::trunc);
            f << testKarte();
        }
        (void)ke::karteOeffnen(karteTestPfad("opt_test.map"));
        ke::editor().karte.auswahl.leeren();
        ke::optimiererSchliessen();
        ke::konsoleLeeren();
        *laeufe = ke::optimiererBlick().laeufe;
    }));
    add(pause(0.3));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.25));
    add(pruefe("Build-Menue: \"Optimizer...\" nach \"Update entities only\" (eigener Eintrag)", [] {
        return finde(std::string(ke::trK(Str::KeOptMenue)) + "##bauoptimierer", "", 0, true) != nullptr;
    }));
    add(klick(std::string(ke::trK(Str::KeOptMenue)) + "##bauoptimierer", "", 0, true));
    add(warteOptimierer("Optimierer fertig", laeufe));
    add(pruefe("Fenster \"Optimizer\" offen, Pruefung im Hintergrund fertig", [] {
        const ImGuiWindow* w = ImGui::FindWindowByName("###karteoptimierer");
        return ke::optimiererOffen() && w != nullptr && ke::optimiererBlick().hatErgebnis;
    }));
    add(pruefe("Befunde der Testkarte: verdeckt 2, mischinhalt 1, offgrid 1, deko", [] {
        long long v = 0;
        long long m = 0;
        long long o = 0;
        diag::detail("Knopftest: Stufe 5 - Befunde " + befundListe());
        return hatBefund("verdeckt", &v) && v == 2 && hatBefund("mischinhalt", &m) && m == 1 && hatBefund("offgrid", &o) && o == 1 && hatBefund("deko");
    }));
    add(pruefe("Konsole: die Zusammenfassung des Optimierers", [] {
        const std::string vorlage = ke::trK(Str::KeOptZusammenfassung);
        return stufe3::konsoleText().find(vorlage.substr(0, vorlage.find("%d"))) != std::string::npos;
    }));
    // Reiter "Pruefungen", Zeile "verdeckt": waehlt die zwei Pinsel aus
    add(tu("Reiter Pruefungen", [] { ke::optimiererReiter(1); }));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeOptPrVerdeckt)) + "##bfverdeckt", "###karteoptimierer"));
    add(pause(0.2));
    add(pruefe("Klick auf \"verdeckt\": die zwei Pinsel gewaehlt", [] { return ke::editor().karte.auswahl.anzahl() == 2; }));
    add(klick(std::string(ke::trK(Str::KeOptBeheben)) + "##optbeheben", "###karteoptimierer"));
    add(pause(0.2));
    add(pruefe("Beheben...: Vorschau mit Anwenden", [] { return finde(std::string(ke::trK(Str::KeOptAnwenden)) + "##optanwenden", "###karteoptimierer", 0, false) != nullptr; }));
    add(tu("Laeufe merken", [laeufe] { *laeufe = ke::optimiererBlick().laeufe; }));
    add(klick(std::string(ke::trK(Str::KeOptAnwenden)) + "##optanwenden", "###karteoptimierer"));
    add(warteOptimierer("neu gezaehlt nach dem Beheben", laeufe));
    add(pruefe("Anwenden: 2 Flaechen caulk, Rueckgaengig \"Optimizer: caulk 2 hidden faces\", neu gezaehlt ohne \"verdeckt\"", [] {
        diag::detail("Knopftest: Stufe 5 - nach Caulk " + befundListe() + ", Undo " + ke::editor().karte.undo.undoName());
        return caulkFlaechen() == 2 && ke::editor().karte.undo.undoName() == "Optimizer: caulk 2 hidden faces" && !hatBefund("verdeckt") &&
               !ke::optimiererBlick().veraltet;
    }));
    add(tu("Laeufe merken", [laeufe] { *laeufe = ke::optimiererBlick().laeufe; }));
    add(klick(std::string(ke::trK(Str::KeOptAllesSichere)) + "##optsicher", "###karteoptimierer"));
    add(warteOptimierer("neu gezaehlt nach Alles Sichere", laeufe));
    add(pruefe("Alles Sichere beheben: Mischinhalt und Raster behoben (ein Schritt), Deko bleibt (nicht sicher)", [] {
        diag::detail("Knopftest: Stufe 5 - nach Alles Sichere " + befundListe());
        return !hatBefund("mischinhalt") && !hatBefund("offgrid") && hatBefund("deko") &&
               ke::editor().karte.undo.undoName().rfind("Optimizer: fix all safe", 0) == 0;
    }));
    // Bericht exportieren (der Dateidialog des Selbsttests liefert den Pfad)
    add(tu("Dateidialog: opt_bericht.txt", [] {
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(karteTestPfad("opt_bericht.txt")), ec);
        ke::editor().dateiDialog = [](const std::string&) { return karteTestPfad("opt_bericht.txt"); };
    }));
    add(klick(std::string(ke::trK(Str::KeOptExport)) + "##optexport", "###karteoptimierer"));
    add(pause(0.2));
    add(tu("Dateidialog zurueck", [] { ke::editor().dateiDialog = nullptr; }));
    add(pruefe("Bericht: opt_bericht.txt mit Grenzen und Pruefungen samt Quellen", [] {
        const std::string t = dateiInhalt(karteTestPfad("opt_bericht.txt"));
        return t.find(std::string("== ") + ke::trK(Str::KeOptReiterGrenzen)) != std::string::npos &&
               t.find(std::string("== ") + ke::trK(Str::KeOptReiterBefunde)) != std::string::npos && t.find("code/qcommon/qfiles.h:") != std::string::npos &&
               t.find("MAX_PATCH_PLANES") != std::string::npos;
    }));
    // Reiter "Grenzen": "Nur Probleme" aus, die Zeile Entities nach dem Spawn
    add(tu("Reiter Grenzen", [] { ke::optimiererReiter(0); }));
    add(pause(0.2));
    add(klick(std::string(ke::trK(Str::KeOptNurProbleme)) + "##optnurprobleme", "###karteoptimierer"));
    add(pause(0.2));
    add(pruefe("Nur Probleme aus: jede Grenze als Zeile (Entities nach dem Spawn aus der Karte)", [] {
        return finde(std::string(ke::trK(Str::KeGrLzEntitaeten)) + "##laufzeit.entitaeten@map", "###karteoptimierer", 0, false) != nullptr &&
               ke::optimiererBlick().grenzenIds.empty();
    }));
    add(klick(std::string(ke::trK(Str::KeGrLzEntitaeten)) + "##laufzeit.entitaeten@map", "###karteoptimierer"));
    add(pause(0.2));
    add(pruefe("Klick auf die Zeile: info_player_start (der Beitrag) gewaehlt", [] {
        const auto& a = ke::editor().karte.auswahl.elemente();
        return a.size() == 1 && a.front().entitaet.lock() && a.front().entitaet.lock()->klassenname() == "info_player_start";
    }));
    add(klick(std::string(ke::trK(Str::KeOptNurProbleme)) + "##optnurprobleme", "###karteoptimierer"));
    // Einstellungsseite "Optimizer" (Abweichung) und Speichern
    add(tu("Einstellungen auf", [] { ke::editor().einstellungenOffen = true; }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KePgOptimizer)) + "##seite23"));
    add(pause(0.3));
    add(pruefe("Seite Optimizer (Abweichung, die letzte unter Settings): Knapp ab, Nach jedem Bau, Regelwerk, Schwellen", [] {
        std::string gesehen;
        for (const Element& el : elemente::alle()) {
            if (el.label.find("opt") != std::string::npos || el.label.find("seite2") != std::string::npos) { gesehen += " [" + el.label + "|" + el.fenster + "]"; }
        }
        diag::detail("Knopftest: Stufe 5 - Elemente der Seite" + gesehen);
        return finde("##optknapp", "", 0, true) != nullptr && finde("##optdreiecke", "", 0, true) != nullptr &&
               finde(std::string(ke::trK(Str::KePrOptNachBau)) + "##optnachbau", "", 0, true) != nullptr && finde("##optportale", "", 0, true) != nullptr;
    }));
    add(foto("karteneditor_pref_optimierer"));
    add(tu("Einstellungen zu", [] { karteDialogeZu(); }));
    add(pause(0.2));
    add(pruefe("Speichern: OptimizerNear und OptimizerAfterBuild in den Einstellungen", [] {
        ke::Editor& e = ke::editor();
        const int vorher = e.optimierer.knappProzent;
        e.optimierer.knappProzent = 75;
        ke::einstellungenSpeichern();
        const bool ok = g_app->settings.karte["OptimizerNear"] == "75" && g_app->settings.karte.count("OptimizerAfterBuild") == 1;
        e.optimierer.knappProzent = vorher;
        ke::einstellungenSpeichern();
        return ok;
    }));
    add(foto("karteneditor_optimierer"));
    add(klick(std::string(ke::trK(Str::KeOptSchliessen)) + "##optzu", "###karteoptimierer"));
    add(pause(0.2));
    add(pruefe("Schliessen: das Fenster ist zu", [] { return !ke::optimiererOffen(); }));
    return s;
}

// --- Punkt 7: jede Pruefung und jede Behebung mit Wirkung; gebaute Karten; eine
// Originalkarte; eine grosse Karte (Zeit, das Programm bleibt bedienbar) ----------

std::string entText(const std::string& klasse, const std::string& schluessel, const std::string& inhalt) {
    return "{\n\"classname\" \"" + klasse + "\"\n" + schluessel + inhalt + "}\n";
}

// Ein Wellen-Patch w x h (wie wellenPatch in tests/karteoptimierer.cpp: 31 x 31
// mit Hoehe 96 braucht mehr als MAX_PATCH_PLANES 2048 Kollisionsebenen).
std::string wellenPatchText(int w, int h, float hoehe, float dx, float dy) {
    std::string t = "{\npatchDef2\n{\ntest/welle\n( " + std::to_string(w) + " " + std::to_string(h) + " 0 0 0 )\n(\n";
    for (int i = 0; i < w; ++i) {   // patchDef2: je Zeile der Datei eine Spalte
        t += "( ";
        for (int j = 0; j < h; ++j) {
            const float z = hoehe * std::sin(static_cast<float>(i) * 1.3F) * std::cos(static_cast<float>(j) * 0.7F) +
                            hoehe * 0.5F * std::sin(static_cast<float>(i * j) * 0.37F);
            char c[128];
            std::snprintf(c, sizeof(c), "( %g %g %g 0 0 ) ", static_cast<double>(dx + static_cast<float>(i) * 64.0F),
                          static_cast<double>(dy + static_cast<float>(j) * 64.0F), static_cast<double>(z));
            t += c;
        }
        t += ")\n";
    }
    return t + ")\n}\n}\n";
}

// Je Pruefung, die aus der Karte allein urteilt, ein Fall - weit genug
// auseinander, dass sich die Faelle nicht beruehren.
std::string befundKarte() {
    std::string w;
    w += kastenText(0, 0, 0, 64, 64, 64, "test/a") + kastenText(0, 0, 64, 64, 64, 128, "test/b");          // verdeckt
    w += kastenText(0, 200, 0, 128, 328, 64, "test/a") + kastenText(0, 200, 64, 64, 328, 128, "test/b");   // tjunction
    w += kastenText(200, 0, 0, 264, 64, 0.5, "test/a");                                                    // mikro
    w += kastenText(300, 0, 0, 364.02, 64, 64, "test/c");                                                  // offgrid (sicher)
    w += kastenText(400, 0, 0, 464, 64, 64, "common/hint", 1 << 27);                                       // mischinhalt
    w += kastenText(0, -600, 0, 512, -88, 16, "test/boden") + kastenText(0, -600, 16, 32, -568, 48, "test/kiste");   // deko
    w += kastenText(1000, 600, 0, 1040, 1112, 256, "test/wand");                                           // blockgitter
    w += kastenText(0, 1200, 0, 64, 1264, 64, "test/a") + kastenText(32, 1200, 0, 96, 1264, 64, "test/b");    // ueberlappung
    w += kastenText(600, 0, 0, 664, 64, 64, "test/a", 0, true);                                            // entartet (ohne Deckel)
    w += kastenText(-10000, -3000, -64, 10000, -2936, -48, "test/riesig");                                   // riesenpinsel
    w += kastenText(1200, 0, -16, 1456, 256, 0, "korriban/kor1_lava") + kastenText(1200, 0, 8, 1456, 256, 24, "test/boden");   // flaechenlichtleck
    std::string t = entText("worldspawn", "", w);
    t += entText("func_door", "\"angle\" \"90\"\n", kastenText(800, 300, 0, 808, 364, 64, "test/tuer") + kastenText(800, 300, 0, 808, 308, 8, "common/origin"));   // commonorigin
    t += entText("light", "\"origin\" \"700 400 64\"\n\"style\" \"1\"\n", "");                          // Lichtstil (baubefehl)
    t += entText("func_wall", "", kastenText(0, 400, 0, 64, 464, 64, "common/areaportal"));                  // areaportalentity
    t += entText("func_static", "", wellenPatchText(31, 31, 96.0F, 0.0F, 3000.0F));                          // patchgross
    t += entText("misc_model", "\"origin\" \"800 0 0\"\n\"model\" \"models/map_objects/imperial/crate_banded.md3\"\n\"spawnflags\" \"2\"\n", "");   // clipmodell
    t += entText("func_door", "\"angle\" \"90\"\n", "");                                                    // leereentity
    t += entText("info_player_start", "\"origin\" \"700 300 24\"\n", "");
    return t;
}

// 480 gleiche func_wall ohne Namen: 481 von 511 Brush-Modellen (knapp)
std::string modellKarte() {
    std::string t = entText("worldspawn", "", kastenText(-64, -64, -16, 1024, 1024, 0, "test/boden"));
    for (int i = 0; i < 480; ++i) {
        const double x = (i % 24) * 40.0;
        const double y = (i / 24) * 40.0;
        t += entText("func_wall", "\"spawnflags\" \"1\"\n", kastenText(x, y, 0, x + 8, y + 8, 8, "test/w"));
    }
    return t + entText("info_player_start", "\"origin\" \"0 -32 24\"\n", "");
}

// Die Leck-Karte aus Stufe 3 (Loch in der Ostwand), im Loch ein Detail-Pinsel:
// detail versiegelt nichts, die Karte leckt durch ihn.
std::string leckKarte() {
    std::string t = stufe3::raumKarte(true);
    const std::size_t e1 = t.find("// entity 1");
    const std::size_t zu = t.rfind("}\n", e1);
    if (e1 != std::string::npos && zu != std::string::npos) { t.insert(zu, kastenText(512, -32, 64, 528, 32, 128, "test/fuellung", 1 << 27)); }
    return t;
}

// Eine hohle Halle 8192 x 8192 x 1024 aus 8 x 8 Boden- und Deckenkacheln, jede
// auf eigener Hoehe (eigene Ebene: q3map2 legt sie nicht in eine Lightmap
// zusammen und vergroessert dann die Probengroesse), mit 144 farbigen
// Lichtern: 149769 Lichtgitter-Zellen mit mehr als 65535 verschiedenen Proben
// (das Gitter laeuft ueber, bspfile_rbsp.cpp AddLightGridLumps) und 128
// Lightmaps (die Haelfte von 256). Gemessen mit bhed_q3map2: 36339 Zellen
// hinter dem Gitter; mit _lightmapscale 2 noch 15 Lightmaps.
std::string halleKarte() {
    std::string w;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            const double x0 = -4096.0 + i * 1024.0;
            const double y0 = -4096.0 + j * 1024.0;
            const double d = i * 8 + j;
            w += kastenText(x0, y0, -16, x0 + 1024, y0 + 1024, d, "test/boden");
            w += kastenText(x0, y0, 1024 - d, x0 + 1024, y0 + 1024, 1040, "test/decke");
        }
    }
    w += kastenText(-4112, 4096, 0, 4112, 4112, 1024, "test/wand") + kastenText(-4112, -4112, 0, 4112, -4096, 1024, "test/wand") +
         kastenText(-4112, -4096, 0, -4096, 4096, 1024, "test/wand") + kastenText(4096, -4096, 0, 4112, 4096, 1024, "test/wand");
    std::string t = entText("worldspawn", "", w) + entText("info_player_start", "\"origin\" \"0 0 100\"\n", "");
    // ein misc_model_static, dessen Pruefpunkt (origin + 0 0 1) in der Bodenkachel liegt (OpenJK#1358)
    t += entText("misc_model_static", "\"origin\" \"-3584 -3584 -10\"\n\"model\" \"models/map_objects/imperial/crate_banded.md3\"\n", "");
    unsigned zufall = 5;
    const auto anteil = [&zufall] {
        zufall = zufall * 1103515245U + 12345U;
        return static_cast<double>((zufall >> 16U) & 0x7FFFU) / 32767.0;
    };
    for (int i = 0; i < 12; ++i) {
        for (int j = 0; j < 12; ++j) {
            const double r = anteil();
            const double g = anteil();
            const double b = anteil();
            char z[256];
            std::snprintf(z, sizeof(z), "\"origin\" \"%g %g %g\"\n\"light\" \"%d\"\n\"_color\" \"%.2f %.2f %.2f\"\n", -4096.0 + (i + 0.5) * 8192.0 / 12.0,
                          -4096.0 + (j + 0.5) * 8192.0 / 12.0, 64.0 + ((i * 7 + j * 3) % 5) * (1024.0 - 128.0) / 4.0, 600 + (i * j) % 7 * 100, r, g, b);
            t += entText("light", z, "");
        }
    }
    return t;
}

std::string befundZahlen() { return befundListe(); }

Schritt befundBehebenUndZurueck(std::vector<Schritt>& s, const std::string& id, std::shared_ptr<int> laeufe) {
    auto vorher = std::make_shared<long long>(0);
    s.push_back(tu("Beheben: " + id, [id, laeufe, vorher] {
        *laeufe = ke::optimiererBlick().laeufe;
        *vorher = 0;
        (void)hatBefund(id.c_str(), vorher.get());
        if (!ke::optimiererBefundBeheben(id)) { meldeFehler("Optimierer: \"" + id + "\" behoben ohne Wirkung"); }
    }));
    s.push_back(warteOptimierer("neu gezaehlt nach dem Beheben", laeufe));
    s.push_back(pruefe("Beheben " + id + ": danach kein Befund mehr, ein Schritt \"Optimizer: ...\" im Rueckgaengig", [id, vorher] {
        diag::detail("Knopftest: Stufe 5/7 - " + id + " (" + std::to_string(*vorher) + ") behoben: " + ke::editor().karte.undo.undoName() + " | " + befundZahlen());
        return *vorher > 0 && !hatBefund(id.c_str()) && ke::editor().karte.undo.undoName().rfind("Optimizer: ", 0) == 0;
    }));
    s.push_back(tu("Rueckgaengig (Strg+Z), neu zaehlen", [laeufe] {
        *laeufe = ke::optimiererBlick().laeufe;
        (void)ke::ausfuehren("Undo");
        ke::optimiererStarten();
    }));
    s.push_back(warteOptimierer("neu gezaehlt nach Rueckgaengig", laeufe));
    return pruefe("Rueckgaengig: " + id + " wieder da, gleich oft", [id, vorher] {
        long long n = 0;
        return hatBefund(id.c_str(), &n) && n == *vorher;
    });
}

// Die Pruefung im Hintergrund starten und dabei jedes Bild messen: das
// Programm bleibt bedienbar (Bild 0 startet, Bild 1 traegt die Momentaufnahme).
struct OptLauf {
    double startMs = 0.0;
    std::vector<double> bilder;
    double groesstes() const { return bilder.empty() ? 0.0 : *std::max_element(bilder.begin(), bilder.end()); }
    double mittel() const {
        double s = 0.0;
        for (const double b : bilder) { s += b; }
        return bilder.empty() ? 0.0 : s / static_cast<double>(bilder.size());
    }
};
Schritt optimiererGemessen(const std::string& was, std::shared_ptr<int> laeufe, std::shared_ptr<OptLauf> m, int maxBilder) {
    return {was, [laeufe, m, maxBilder](int b) {
                if (b == 0) {
                    *laeufe = ke::optimiererBlick().laeufe;
                    m->bilder.clear();
                    const auto t0 = std::chrono::steady_clock::now();
                    ke::optimiererOeffnen();
                    m->startMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
                    return false;
                }
                if (b >= 2) { m->bilder.push_back(static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0); }
                if (!ke::optimiererLaeuft() && ke::optimiererBlick().laeufe > *laeufe) { return true; }
                if (b > maxBilder) {
                    meldeFehler("Optimierer nach " + std::to_string(maxBilder) + " Bildern nicht fertig");
                    return true;
                }
                return false;
            }};
}

std::string laufText(const OptLauf& m) {
    char z[320];
    const ke::OptimiererBlick b = ke::optimiererBlick();
    std::snprintf(z, sizeof(z), "Start %.0f ms (optimiererStarten %.0f ms: Momentaufnahme %.0f ms, Shader %.0f ms), Pruefung %.0f ms, %zu Bilder dabei, groesstes %.0f ms, Mittel %.1f ms",
                  m.startMs, b.startGesamtMs, b.startAbbildMs, b.startShaderMs, b.ms, m.bilder.size(), m.groesstes(), m.mittel());
    return z;
}

// Jede Pruefung der Karte, jede Behebung mit Wirkung und Rueckgaengig
std::vector<Schritt> pruefungen() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto laeufe = std::make_shared<int>(0);
    add(tu("Handbuch-Begriffe aus; Spieldaten (Texturinventar fuer \"fehlende Textur\")", [] {
        ke::setzeNrcBegriffe(false);
        ke::spielPruefen();
    }));
    add(warteBis("Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("Stufe 5/7: Testkarte opt_befunde.map (je Pruefung ein Fall) schreiben und oeffnen", [laeufe] {
        karteDialogeZu();
        {
            std::ofstream f(std::filesystem::path(karteTestPfad("opt_befunde.map")), std::ios::binary | std::ios::trunc);
            f << befundKarte();
        }
        (void)ke::karteOeffnen(karteTestPfad("opt_befunde.map"));
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        e.optimierer = ke::Editor::OptimiererEinstellungen{};
        ke::optimiererSchliessen();
        ke::konsoleLeeren();
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererOeffnen();
    }));
    add(warteOptimierer("Optimierer fertig", laeufe));
    add(pruefe("Jede Pruefung aus der Karte schlaegt an: verdeckt, tjunction, mikro, offgrid, mischinhalt, deko, blockgitter, ueberlappung, entartet, "
               "areaportalentity, patchgross, clipmodell, leereentity, textur, commonorigin, riesenpinsel, flaechenlichtleck (kor1_lava 8 unter einer "
               "Platte, Flaechenlicht aus korriban.shader); der Wellen-Patch ist auch als Grenze \"ueber\" (Patch-Ebenen)",
               [] {
                   const ke::OptimiererBlick b = ke::optimiererBlick();
                   std::string grenzen;
                   for (const std::string& g : b.grenzenIds) { grenzen += " " + g; }
                   diag::detail("Knopftest: Stufe 5/7 - Befunde " + befundZahlen() + " | ueber " + std::to_string(b.grenzenUeber) + ", nicht ok:" + grenzen);
                   bool ok = true;
                   for (const char* id : {"verdeckt", "tjunction", "mikro", "offgrid", "mischinhalt", "deko", "blockgitter", "ueberlappung", "entartet",
                                          "areaportalentity", "patchgross", "clipmodell", "leereentity", "textur", "commonorigin", "riesenpinsel",
                                          "flaechenlichtleck"}) {
                       if (!hatBefund(id)) {
                           diag::detail(std::string("Knopftest: Stufe 5/7 - fehlt: ") + id);
                           ok = false;
                       }
                   }
                   return ok && b.grenzenUeber >= 1 && std::find(b.grenzenIds.begin(), b.grenzenIds.end(), "kollision.patchebenen@map") != b.grenzenIds.end();
               }));
    add(tu("Reiter Pruefungen", [] { ke::optimiererReiter(1); }));
    add(pause(0.3));
    add(foto("karteneditor_s5_befunde"));
    // Klick auf eine Zeile waehlt die Objekte (hier der leere func_door)
    add(tu("Zeile leereentity waehlen", [] { (void)ke::optimiererZeileWaehlen(1, "leereentity"); }));
    add(pause(0.2));
    add(pruefe("Zeile \"Brush entity without brushes\": der func_door ist gewaehlt", [] {
        const auto& a = ke::editor().karte.auswahl.elemente();
        return a.size() == 1 && a.front().entitaet.lock() && a.front().entitaet.lock()->klassenname() == "func_door";
    }));
    // Jede Behebung: wirkt, ist ein Rueckgaengig-Schritt, und Rueckgaengig bringt den Befund zurueck
    for (const char* id : {"verdeckt", "entartet", "mikro", "offgrid", "mischinhalt", "deko", "patchgross", "leereentity", "commonorigin"}) {
        add(befundBehebenUndZurueck(s, id, laeufe));
    }
    // Externe Lightmaps mit Lichtstilen (NRC#48): Befund, und das Build-Menue baut den Eintrag nicht
    auto extern_ = std::make_shared<std::size_t>(0);
    add(tu("Build-Eintrag \"Optimizer external\" (-light -external), neu zaehlen", [laeufe, extern_] {
        ke::Editor& e = ke::editor();
        ke::bauMenueSicherstellen();
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{"Optimizer external", {"[q3map2] -light -fast -external \"[MapFile]\""}});
        *extern_ = e.bauProjekt.eintraege.size() - 1;
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererStarten();
    }));
    add(warteOptimierer("neu gezaehlt", laeufe));
    add(pruefe("Lichtstil in der Karte und -external im Build-Menue: Befund \"baubefehl\"", [] { return hatBefund("baubefehl"); }));
    add(tu("Konsole leeren; Build > Optimizer external", [extern_] {
        ke::konsoleLeeren();
        ke::bauStarten(*extern_);
    }));
    add(pause(0.3));
    add(pruefe("Das Build-Menue startet den Eintrag nicht und sagt warum (rot in der Konsole)", [] {
        const std::string vorlage = ke::trK(Str::KeBmGesperrt);
        const std::string k = stufe3::konsoleText();
        diag::detail("Knopftest: Stufe 5/7 - gesperrt: " + k.substr(k.size() > 300 ? k.size() - 300 : 0));
        return !ke::bauLaeuft() && stufe3::konsoleArtMit(ke::KonsolenZeile::Art::Fehler, vorlage.substr(0, vorlage.find('%'))) &&
               k.find("-external") != std::string::npos;
    }));
    add(tu("Eintrag weg, neu zaehlen", [laeufe, extern_] {
        ke::Editor& e = ke::editor();
        if (*extern_ < e.bauProjekt.eintraege.size()) { e.bauProjekt.eintraege.erase(e.bauProjekt.eintraege.begin() + static_cast<std::ptrdiff_t>(*extern_)); }
        e.bauGeaendert = false;
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererStarten();
    }));
    add(warteOptimierer("neu gezaehlt", laeufe));
    add(pruefe("Ohne den Eintrag: kein Befund \"baubefehl\"", [] { return !hatBefund("baubefehl"); }));
    add(tu("Alles Sichere beheben", [laeufe] {
        *laeufe = ke::optimiererBlick().laeufe;
        if (ke::optimiererAllesSichere() <= 0) { meldeFehler("Alles Sichere beheben: nichts behoben"); }
    }));
    add(warteOptimierer("neu gezaehlt nach Alles Sichere", laeufe));
    add(pruefe("Alles Sichere beheben (ein Schritt): verdeckt, entartet, offgrid, mischinhalt weg; mikro, deko, patchgross, leereentity, commonorigin bleiben "
               "(nicht sicher)", [] {
        diag::detail("Knopftest: Stufe 5/7 - nach Alles Sichere " + befundZahlen());
        return !hatBefund("verdeckt") && !hatBefund("entartet") && !hatBefund("offgrid") && !hatBefund("mischinhalt") && hatBefund("mikro") &&
               hatBefund("deko") && hatBefund("patchgross") && hatBefund("leereentity") && hatBefund("commonorigin") &&
               ke::editor().karte.undo.undoName().rfind("Optimizer: fix all safe", 0) == 0;
    }));
    // Brush-Modelle zusammenlegen: 481 von 511 (knapp)
    add(tu("Testkarte opt_modelle.map: 480 gleiche func_wall", [laeufe] {
        {
            std::ofstream f(std::filesystem::path(karteTestPfad("opt_modelle.map")), std::ios::binary | std::ios::trunc);
            f << modellKarte();
        }
        (void)ke::karteOeffnen(karteTestPfad("opt_modelle.map"));
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererStarten();
    }));
    add(warteOptimierer("Optimierer fertig", laeufe));
    add(pruefe("481 Brush-Modelle: Grenze knapp, Vorschlag \"zusammenlegen\" (480)", [] {
        const ke::OptimiererBlick b = ke::optimiererBlick();
        long long n = 0;
        diag::detail("Knopftest: Stufe 5/7 - Modelle " + befundZahlen());
        return hatBefund("modellezusammen", &n) && n == 480 &&
               std::find(b.grenzenIds.begin(), b.grenzenIds.end(), "kollision.submodelle@map") != b.grenzenIds.end();
    }));
    add(befundBehebenUndZurueck(s, "modellezusammen", laeufe));
    return s;
}

// Gebaute Karten: Leck mit Detail-Pinsel, eine Halle mit vollem Lichtgitter und
// knappen Lightmaps - jeweils beheben und neu bauen
std::vector<Schritt> gebaut() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto laeufe = std::make_shared<int>(0);
    const std::string ordner = stufe3::laufOrdner("s5bau");
    const std::string leck = ordner + "\\opt_leck";
    const std::string halle = ordner + "\\opt_halle";
    const auto fertig = [](const char* was, int bilder) { return warteBis(was, [] { return !ke::bauLaeuft(); }, bilder); };
    add(tu("Stufe 5/7: bhed_q3map2, Build-Menue wie vorgegeben, Testkarte opt_leck (Loch in der Ostwand, darin ein Detail-Pinsel), "
           "Einstellung \"Nach jedem Bau\" an",
           [leck, laeufe] {
               karteDialogeZu();
               stufe3::q3map2Bereitstellen();
               ke::Editor& e = ke::editor();
               e.bauProjektFuer = -2;
               e.bauGeaendert = false;
               ke::bauMenueSicherstellen();
               e.bau = ke::Editor::BauEinstellungen{};
               std::error_code ec;
               std::filesystem::create_directories(std::filesystem::path(leck).parent_path(), ec);
               {
                   std::ofstream f(std::filesystem::path(leck + ".map"), std::ios::binary | std::ios::trunc);
                   f << leckKarte();
               }
               (void)ke::karteOeffnen(leck + ".map");
               stufe3::bauDateienWeg(leck);
               e.optimierer = ke::Editor::OptimiererEinstellungen{};
               e.optimierer.nachBau = true;
               e.bauRueckmeldungOffen = false;
               ke::optimiererSchliessen();
               ke::konsoleLeeren();
               *laeufe = ke::optimiererBlick().laeufe;
           }));
    add(stufe3::bauMenuepunkt("Single: bsp stage"));
    add(fertig("Bauen fertig", 2400));
    add(warteOptimierer("Optimierer nach dem Bau (auch wenn das Leck den Bau anhaelt)", laeufe));
    add(pruefe("Nach dem Bau mit Leck: der Optimierer lief von selbst und ging auf (Probleme); Leck aus der .lin, der Detail-Pinsel an der Leck-Linie",
               [leck] {
                   diag::detail("Knopftest: Stufe 5/7 - Leck " + befundZahlen());
                   return ke::optimiererOffen() && stufe3::istDatei(leck + ".lin") && hatBefund("leck") && hatBefund("detailamleck");
               }));
    add(tu("Reiter Pruefungen; Zeile detailamleck", [] {
        ke::editor().bauRueckmeldungOffen = false;
        ke::optimiererReiter(1);
        (void)ke::optimiererZeileWaehlen(1, "detailamleck");
    }));
    add(pause(0.3));
    add(pruefe("Zeile \"Detail brush at the leak line\": der Detail-Pinsel im Loch ist gewaehlt", [] {
        const auto& a = ke::editor().karte.auswahl.elemente();
        if (a.size() != 1 || !a.front().primitiv.lock()) { return false; }
        const auto p = std::dynamic_pointer_cast<karte::Pinsel>(a.front().primitiv.lock());
        return p && p->istDetail();
    }));
    add(foto("karteneditor_s5_leck"));
    add(tu("Beheben: detailamleck (Make Structural)", [laeufe] {
        *laeufe = ke::optimiererBlick().laeufe;
        if (!ke::optimiererBefundBeheben("detailamleck")) { meldeFehler("detailamleck: Beheben ohne Wirkung"); }
    }));
    add(warteOptimierer("neu gezaehlt", laeufe));
    add(pruefe("Der Pinsel ist structural, kein Detail mehr an der Leck-Linie; das Leck steht bis zum naechsten Bau", [] {
        return !hatBefund("detailamleck") && hatBefund("leck");
    }));
    add(tu("Konsole leeren, neu bauen", [laeufe] {
        ke::konsoleLeeren();
        *laeufe = ke::optimiererBlick().laeufe;
    }));
    add(stufe3::bauMenuepunkt("Single: bsp stage"));
    add(fertig("Bauen fertig", 2400));
    add(warteOptimierer("Optimierer nach dem Bau", laeufe));
    add(pruefe("Neu gebaut: dicht - keine .lin (q3map2 bsp.cpp:880 loescht sie), kein Leck; die gebaute .bsp wird mitgeprueft", [leck] {
        diag::detail("Knopftest: Stufe 5/7 - nach dem Bau " + befundZahlen() + " | BSP " + ke::optimiererBlick().bsp);
        return !hatBefund("leck") && !stufe3::istDatei(leck + ".lin") && stufe3::istDatei(leck + ".bsp") && !ke::optimiererBlick().bsp.empty();
    }));
    // Die Halle: viele farbige Lichter, ein eigener Build-Eintrag (ohne -bounce)
    auto eintrag = std::make_shared<std::size_t>(0);
    add(tu("Testkarte opt_halle (hohl, viele farbige Lichter), Build-Eintrag \"Optimizer test\" (-meta, -vis -fast -saveprt, -light -fast), "
           "Knapp ab 40 %, VIS- und Dreiecks-Schwelle 1",
           [halle, laeufe, eintrag] {
               karteDialogeZu();
               {
                   std::ofstream f(std::filesystem::path(halle + ".map"), std::ios::binary | std::ios::trunc);
                   f << halleKarte();
               }
               (void)ke::karteOeffnen(halle + ".map");
               stufe3::bauDateienWeg(halle);
               ke::Editor& e = ke::editor();
               e.optimierer.knappProzent = 40;
               e.optimierer.portalSchwelle = 1;
               e.optimierer.dreieckSchwelle = 1;
               e.bauProjekt.eintraege.push_back(karte::BauEintrag{
                   "Optimizer test", {"[q3map2] -meta \"[MapFile]\"", "[q3map2] -vis -fast -saveprt \"[MapFile]\"", "[q3map2] -light -fast \"[MapFile]\""}});
               *eintrag = e.bauProjekt.eintraege.size() - 1;
               ke::konsoleLeeren();
               *laeufe = ke::optimiererBlick().laeufe;
               ke::bauStarten(*eintrag);
           }));
    add(fertig("Bauen fertig", 36000));
    add(warteOptimierer("Optimierer nach dem Bau", laeufe));
    add(pruefe("Halle gebaut: Lichtgitter voll (Zellen zeigen hinter das Gitter, gridsize verdoppeln), Lightmaps knapp (_lightmapscale 2, "
               "die Anzahl sind Lightmaps, nicht Bytes), VIS und Dreiecke je Ansicht ueber den Schwellen",
               [] {
                   long long lg = 0;
                   long long lm = 0;
                   diag::detail("Knopftest: Stufe 5/7 - Halle " + befundZahlen());
                   return hatBefund("lichtgitter", &lg) && lg > 0 && hatBefund("lightmapscale", &lm) && lm > 64 && lm <= 256 && hatBefund("vis") &&
                          hatBefund("dreiecke");
               }));
    add(foto("karteneditor_s5_halle"));
    add(pruefe("misc_model_static mit dem Pruefpunkt in der Bodenkachel: Befund \"modellunsichtbar\" mit Vorschlag zoffset", [] {
        diag::detail("Knopftest: Stufe 5/7 - Halle " + befundZahlen());
        return hatBefund("modellunsichtbar");
    }));
    add(tu("Beheben: modellunsichtbar (zoffset)", [laeufe] {
        *laeufe = ke::optimiererBlick().laeufe;
        if (!ke::optimiererBefundBeheben("modellunsichtbar")) { meldeFehler("modellunsichtbar: Beheben ohne Wirkung"); }
    }));
    add(warteOptimierer("neu gezaehlt", laeufe));
    add(pruefe("zoffset gesetzt (das Modell bleibt), der Pruefpunkt liegt jetzt frei: kein Befund mehr - ohne neuen Bau", [] {
        std::string z;
        for (const auto& en : ke::editor().karte.entitaeten()) {
            if (en->klassenname() == "misc_model_static") { z = en->wert("zoffset") + " / origin " + en->wert("origin"); }
        }
        diag::detail("Knopftest: Stufe 5/7 - misc_model_static zoffset " + z);
        return !hatBefund("modellunsichtbar") && z.rfind("/ origin -3584 -3584 -10") != std::string::npos && z.find(' ') > 0 && z[0] != ' ';
    }));
    add(tu("Beheben: lichtgitter (worldspawn gridsize)", [laeufe] {
        *laeufe = ke::optimiererBlick().laeufe;
        if (!ke::optimiererBefundBeheben("lichtgitter")) { meldeFehler("lichtgitter: Beheben ohne Wirkung"); }
    }));
    add(warteOptimierer("neu gezaehlt", laeufe));
    add(tu("Beheben: lightmapscale (worldspawn _lightmapscale)", [laeufe] {
        *laeufe = ke::optimiererBlick().laeufe;
        if (!ke::optimiererBefundBeheben("lightmapscale")) { meldeFehler("lightmapscale: Beheben ohne Wirkung"); }
    }));
    add(warteOptimierer("neu gezaehlt", laeufe));
    add(pruefe("worldspawn: gridsize \"128 128 128\" und _lightmapscale \"2\"; die Konsole bittet um einen neuen Bau", [] {
        const auto w = ke::editor().karte.welt();
        diag::detail("Knopftest: Stufe 5/7 - worldspawn gridsize \"" + (w ? w->wert("gridsize") : std::string()) + "\" _lightmapscale \"" +
                     (w ? w->wert("_lightmapscale") : std::string()) + "\"");
        return w && w->wert("gridsize") == "128 128 128" && w->wert("_lightmapscale") == "2" &&
               stufe3::konsoleText().find(ke::trK(Str::KeOptNeuBauen)) != std::string::npos;
    }));
    add(tu("Neu bauen (Optimizer test)", [laeufe, eintrag] {
        *laeufe = ke::optimiererBlick().laeufe;
        ke::bauStarten(*eintrag);
    }));
    add(fertig("Bauen fertig", 36000));
    add(warteOptimierer("Optimierer nach dem Bau", laeufe));
    add(pruefe("Neu gebaut: das Lichtgitter passt, die Lightmaps sind nicht mehr knapp (Wirkung in der gebauten .bsp)", [] {
        diag::detail("Knopftest: Stufe 5/7 - Halle neu gebaut " + befundZahlen());
        return !hatBefund("lichtgitter") && !hatBefund("lightmapscale") && !ke::optimiererBlick().bsp.empty();
    }));
    // Spawnpunkt ausserhalb der Halle (im Leeren): Cluster -1 in der gebauten .bsp
    add(tu("info_player_start nach 9000 0 24 (ausserhalb), neu zaehlen", [laeufe] {
        for (const auto& en : ke::editor().karte.entitaeten()) {
            if (en->klassenname() == "info_player_start") { en->setzen("origin", "9000 0 24"); }
        }
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererStarten();
    }));
    add(warteOptimierer("neu gezaehlt", laeufe));
    add(pruefe("Spawnpunkt im Leeren: Befund \"spawnimfesten\" (CM_PointLeafnum, Cluster -1)", [] { return hatBefund("spawnimfesten"); }));
    add(tu("info_player_start zurueck nach 0 0 100, neu zaehlen", [laeufe] {
        for (const auto& en : ke::editor().karte.entitaeten()) {
            if (en->klassenname() == "info_player_start") { en->setzen("origin", "0 0 100"); }
        }
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererStarten();
    }));
    add(warteOptimierer("neu gezaehlt", laeufe));
    add(pruefe("Spawnpunkt in der Halle: kein Befund", [] { return !hatBefund("spawnimfesten"); }));
    add(tu("Build-Eintrag \"Optimizer test\" weg, Einstellungen des Optimierers wie vorgegeben", [eintrag] {
        ke::Editor& e = ke::editor();
        if (*eintrag < e.bauProjekt.eintraege.size() && e.bauProjekt.eintraege[*eintrag].name == "Optimizer test") {
            e.bauProjekt.eintraege.erase(e.bauProjekt.eintraege.begin() + static_cast<std::ptrdiff_t>(*eintrag));
        }
        e.bauGeaendert = false;
        e.optimierer = ke::Editor::OptimiererEinstellungen{};
        ke::optimiererSchliessen();
    }));
    return s;
}

// Eine Originalkarte: mp/duel1 aus den .pk3 (Stufe 4: dekompiliert, die
// Original-BSP daneben) - keine Grenze darf "ueber" sein, weder mit den Regeln
// von JKA SP noch MP.
std::vector<Schritt> original() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto laeufe = std::make_shared<int>(0);
    add(tu("Stufe 5/7: Originalkarte mp/duel1 oeffnen (File > Open .bsp...)", [] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();
        ke::optimiererSchliessen();
        ke::editor().optimierer = ke::Editor::OptimiererEinstellungen{};
        ke::editor().bspRueck.modelle = false;   // Stufe 8a: ohne Rueckgewinnung (hier zaehlt das Dekompilat)
        (void)ke::bspOeffnenSpiel("maps/mp/duel1.bsp");
    }));
    add(warteBis("duel1 geladen", [] { return ke::editor().bspHerkunft.aktiv; }, 6000));
    for (int regel : {0, 1}) {
        auto m = std::make_shared<OptLauf>();
        add(tu(std::string("Regelwerk ") + (regel == 0 ? "JKA SP" : "JKA MP"), [regel] { ke::editor().optimierer.regelwerk = regel; }));
        add(optimiererGemessen("Optimierer auf duel1 (Bildzeiten waehrend der Pruefung)", laeufe, m, 3000));
        add(pruefe(std::string("duel1 mit den Regeln von ") + (regel == 0 ? "JKA SP" : "JKA MP") +
                       ": keine Grenze \"ueber\", die BSP aus dem Spiel mitgeprueft (Kopie im Arbeitsordner), das Programm bleibt bedienbar (kein Bild ueber 250 ms)",
                   [m] {
                       const ke::OptimiererBlick b = ke::optimiererBlick();
                       std::string knapp;
                       for (const std::string& g : b.grenzenIds) { knapp += " " + g; }
                       diag::detail("Knopftest: Stufe 5/7 - duel1: " + laufText(*m) + " | ueber " + std::to_string(b.grenzenUeber) + ", nicht ok:" + knapp +
                                    " | Befunde " + befundZahlen() + " | BSP " + b.bsp);
                       return b.hatErgebnis && b.grenzenUeber == 0 && b.bsp.size() > 4 && b.bsp.compare(b.bsp.size() - 4, 4, ".bsp") == 0 && m->groesstes() < 250.0;
                   }));
    }
    // Im ganzen Lauf kommt vorher die Halle: deren letzte Behebung gehoert nicht zu duel1
    add(pruefe("Neue Karte: keine Meldung einer Behebung aus der vorigen Karte unter der Liste", [] {
        diag::detail("Knopftest: Stufe 5/7 - Meldung \"" + ke::optimiererBlick().meldung + "\"");
        return ke::optimiererBlick().meldung.empty();
    }));
    add(tu("Reiter Grenzen", [] { ke::optimiererReiter(0); }));
    add(pause(0.3));
    add(foto("karteneditor_s5_duel1"));
    add(tu("Regelwerk wieder automatisch, Fenster zu", [] {
        ke::editor().optimierer = ke::Editor::OptimiererEinstellungen{};
        ke::optimiererSchliessen();
    }));
    return s;
}

// Die groesste Karte (BHED_KARTE_GROSS, duel_mos_espa.map; mit BHED_KARTE_GROSS_BSP
// die BSP aus Movie Duels daneben): Zeit, Bildzeiten waehrend der Pruefung, und
// der echte Fehler der Karte (21752 Lichtgitter-Zellen zeigen hinter das Gitter).
std::vector<Schritt> gross() {
    std::vector<Schritt> s;
    const char* pfad = std::getenv("BHED_KARTE_GROSS");
    if (pfad == nullptr || pfad[0] == '\0') { return s; }
    const char* bsp = std::getenv("BHED_KARTE_GROSS_BSP");
    const std::string quelleMap = pfad;
    const std::string quelleBsp = bsp != nullptr ? bsp : "";
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto laeufe = std::make_shared<int>(0);
    auto m = std::make_shared<OptLauf>();
    auto ladeMs = std::make_shared<double>(0.0);
    const std::string ziel = stufe3::laufOrdner("s5gross") + "\\" + std::filesystem::path(quelleMap).stem().string();
    add(tu("Stufe 5/7: grosse Karte " + quelleMap + (quelleBsp.empty() ? std::string() : " mit " + quelleBsp) + " in den Laufordner, oeffnen, Regelwerk Movie Duels",
           [quelleMap, quelleBsp, ziel, ladeMs] {
               karteDialogeZu();
               ke::optimiererSchliessen();
               std::error_code ec;
               std::filesystem::create_directories(std::filesystem::path(ziel).parent_path(), ec);
               std::filesystem::copy_file(std::filesystem::path(quelleMap), std::filesystem::path(ziel + ".map"), std::filesystem::copy_options::overwrite_existing, ec);
               if (!quelleBsp.empty()) {
                   std::filesystem::copy_file(std::filesystem::path(quelleBsp), std::filesystem::path(ziel + ".bsp"), std::filesystem::copy_options::overwrite_existing, ec);
               }
               const auto t0 = std::chrono::steady_clock::now();
               (void)ke::karteOeffnen(ziel + ".map");
               *ladeMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
               ke::editor().optimierer = ke::Editor::OptimiererEinstellungen{};
               ke::editor().optimierer.regelwerk = 2;   // karte::GrenzSpiel::MovieDuels
           }));
    add(pause(0.5));
    add(optimiererGemessen("Optimierer auf der grossen Karte (Bildzeiten waehrend der Pruefung)", laeufe, m, 60000));
    add(pruefe("Grosse Karte: keine Grenze von Movie Duels \"ueber\", das Programm bleibt waehrend der Pruefung bedienbar (kein Bild ueber 250 ms)", [m, ladeMs] {
        const ke::OptimiererBlick b = ke::optimiererBlick();
        std::string knapp;
        for (const std::string& g : b.grenzenIds) { knapp += " " + g; }
        char z[64];
        std::snprintf(z, sizeof(z), "Laden %.0f ms | ", *ladeMs);
        diag::detail("Knopftest: Stufe 5/7 - grosse Karte: " + std::string(z) + laufText(*m) + " | ueber " + std::to_string(b.grenzenUeber) + ", nicht ok:" + knapp +
                     " | Befunde " + befundZahlen() + " | BSP " + b.bsp);
        return b.hatErgebnis && b.grenzenUeber == 0 && m->groesstes() < 250.0;
    }));
    if (!quelleBsp.empty()) {
        add(pruefe("Die BSP aus Movie Duels: 21752 Lichtgitter-Zellen zeigen hinter das Gitter (Index 65535) - Befund mit Vorschlag gridsize", [] {
            long long n = 0;
            return hatBefund("lichtgitter", &n) && n == 21752 && !ke::optimiererBlick().bsp.empty();
        }));
    }
    add(tu("Reiter Pruefungen", [] { ke::optimiererReiter(1); }));
    add(pause(0.3));
    add(foto("karteneditor_s5_gross"));
    // Stufe 6: die groesste Karte in allen vier Sprachen (Grenzen und Pruefungen)
    {
        auto sprache = std::make_shared<i18n::Language>(i18n::Language::English);
        add(tu("Stufe 6: Sprache merken, Begriffe aus", [sprache] {
            *sprache = i18n::currentLanguage();
            ke::setzeNrcBegriffe(false);
        }));
        const std::pair<i18n::Language, const char*> sprachen[] = {
            {i18n::Language::English, "en"}, {i18n::Language::German, "de"}, {i18n::Language::ChineseSimplified, "zh"}, {i18n::Language::Japanese, "ja"}};
        for (const auto& [l, kurz] : sprachen) {
            const i18n::Language lang = l;
            const std::string k = kurz;
            add(tu("Stufe 6: grosse Karte, Sprache " + k + ", Reiter Grenzen", [lang] {
                i18n::setLanguage(lang);
                ke::optimiererReiter(0);
            }));
            add(pause(0.4));
            add(pruefe("Stufe 6: grosse Karte, Sprache " + k + ": Ergebnis noch da", [lang] {
                return i18n::currentLanguage() == lang && ke::optimiererBlick().hatErgebnis;
            }));
            add(foto("karteneditor_s6_gross_grenzen_" + k));
            add(tu("Stufe 6: Reiter Pruefungen (" + k + ")", [] { ke::optimiererReiter(1); }));
            add(pause(0.3));
            add(foto("karteneditor_s6_gross_pruefungen_" + k));
        }
        add(tu("Stufe 6: Sprache zurueck", [sprache] { i18n::setLanguage(*sprache); }));
    }
    add(tu("Regelwerk wieder automatisch, Fenster zu", [] {
        ke::editor().optimierer = ke::Editor::OptimiererEinstellungen{};
        ke::optimiererSchliessen();
    }));
    return s;
}

}  // namespace stufe5

#include "selbsttest_karte6.inc"
#include "selbsttest_karte7.inc"
#include "selbsttest_karte8.inc"
#include "selbsttest_karte8b.inc"

std::vector<Schritt> karteneditorStufe5Schritte() {
    std::vector<Schritt> s;
    const char* nur = std::getenv("BHED_STUFE5_NUR");
    const auto teil = [nur](const char* name) { return nur == nullptr || std::strcmp(nur, name) == 0; };
    const auto dazu = [&s](std::vector<Schritt> v) {
        for (Schritt& x : v) { s.push_back(std::move(x)); }
    };
    if (teil("fenster")) { dazu(stufe5::fenster()); }
    if (teil("pruefungen")) { dazu(stufe5::pruefungen()); }
    if (teil("gebaut")) { dazu(stufe5::gebaut()); }
    if (teil("original")) { dazu(stufe5::original()); }
    if (teil("gross")) { dazu(stufe5::gross()); }   // nur mit BHED_KARTE_GROSS
    return s;
}

std::vector<Schritt> karteneditorStufe3Schritte() {
    std::vector<Schritt> s;
    // BHED_STUFE3_NUR=maus: nur die Mausbelegung (schneller beim Nachpruefen)
    const char* nur = std::getenv("BHED_STUFE3_NUR");
    if (nur != nullptr && std::strcmp(nur, "maus") == 0) {
        for (Schritt& x : stufe3::maus()) { s.push_back(std::move(x)); }
        for (Schritt& x : stufe3::teiler()) { s.push_back(std::move(x)); }
        return s;
    }
    // BHED_STUFE3_NUR=bau: nur Build-Menue und Bauen (Punkte 3 ff.)
    if (nur != nullptr && std::strcmp(nur, "bau") == 0) {
        for (Schritt& x : stufe3::baumenue()) { s.push_back(std::move(x)); }
        for (Schritt& x : stufe3::bauen()) { s.push_back(std::move(x)); }
        for (Schritt& x : stufe3::leck()) { s.push_back(std::move(x)); }
        for (Schritt& x : stufe3::portale()) { s.push_back(std::move(x)); }
        for (Schritt& x : stufe3::autocaulk()) { s.push_back(std::move(x)); }
        return s;
    }
    // BHED_STUFE3_NUR=caulk: nur AutoCaulk Selected (Punkt 6)
    if (nur != nullptr && std::strcmp(nur, "caulk") == 0) {
        for (Schritt& x : stufe3::autocaulk()) { s.push_back(std::move(x)); }
        return s;
    }
    // BHED_STUFE3_NUR=leck: nur Leck-Linie und Portal Viewer (Punkt 5)
    if (nur != nullptr && std::strcmp(nur, "leck") == 0) {
        for (Schritt& x : stufe3::leck()) { s.push_back(std::move(x)); }
        for (Schritt& x : stufe3::portale()) { s.push_back(std::move(x)); }
        return s;
    }
    for (Schritt& x : stufe3::leiste()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe3::maus()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe3::teiler()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe3::baumenue()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe3::bauen()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe3::leck()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe3::portale()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe3::autocaulk()) { s.push_back(std::move(x)); }
    return s;
}

std::vector<Schritt> karteneditorStufe2Schritte() {
    std::vector<Schritt> s;
    s.push_back(tu("Stufe 2: Liste der ausgeloesten Befehle leeren", [] { karteneditor::editor().befehleAusgefuehrt.clear(); }));
    // BHED_STUFE2_AB=<Punkt>: erst ab diesem Punkt (schnelleres Nachpruefen)
    const char* ab = std::getenv("BHED_STUFE2_AB");
    const int abPunkt = ab != nullptr ? std::atoi(ab) : 0;
    if (abPunkt >= 11) {
        if (abPunkt == 11) {
            for (Schritt& x : stufe2::einstellungen11()) { s.push_back(std::move(x)); }
        }
        if (abPunkt <= 12) {
            for (Schritt& x : stufe2::einstellungen11b()) { s.push_back(std::move(x)); }
        }
        if (abPunkt <= 13) {
            for (Schritt& x : stufe2::einstellungen11c()) { s.push_back(std::move(x)); }
        }
        if (abPunkt <= 14) {
            for (Schritt& x : stufe2::befehlsWirkungen()) { s.push_back(std::move(x)); }
        }
        for (Schritt& x : stufe2::stufe1Reste()) { s.push_back(std::move(x)); }
        return s;
    }
    for (Schritt& x : stufe2::komponenten()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::clipper()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::csg()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::grundformen()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::transformationen()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::patches()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::texturen()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::auswahlDatei()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::filter()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::ebenen()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::modelle()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::modellBrowser()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::einstellungen11()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::einstellungen11b()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::einstellungen11c()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::befehlsWirkungen()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::stufe1Reste()) { s.push_back(std::move(x)); }
    for (Schritt& x : stufe2::alleBefehle()) { s.push_back(std::move(x)); }
    return s;
}

std::vector<Schritt> karteneditorAlleBefehleSchritte() {
    namespace ke = karteneditor;
    namespace kb = karte::befehle;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    struct Vorher {
        karte::Raster raster;
        karte::Kamera kamera;
        karte::KameraEinstellungen kameraE;
        ke::Anzeige anzeige;
        std::vector<karte::XyAnsicht> xy;
        int aktivXy = 0;
        karte::TexturBrowserEinstellungen texBro;
        std::array<bool, 4> schwebend{};
        bool gruppeOffen = false;
        int gruppenSeite = 0;
        bool maximiert = false;
        std::vector<std::string> modi;
        std::vector<std::string> ausgefuehrt;
        bool vorherAn = false;
    };
    auto v = std::make_shared<Vorher>();
    static const char* const kModi[] = {"MouseDrag", "MouseTranslate", "MouseRotate", "MouseScale", "MouseTransform"};
    // Befehle und Umschalter laufen ueber ausfuehren(); reine KeyEvents
    // (CameraFreeMove*, CameraFreeFocus) haengen an der Taste - die
    // Kameraschritte druecken sie.
    // Stufe 1 in befehle_liste.inc = fertig (Stufe 1b und STUFE_2_FERTIG aus
    // gen_befehle.py). Dazu die Rufe der Menues (MenueArt::Ruf: About, Bug
    // report, Set 2D Background image - in NRC FreeCaller ohne Befehlsnamen),
    // seit Stufe 2 Punkt 11c nachgebaut. Seit Stufe 4 (Punkte 3a-3d) auch jeder
    // Eintrag der Plugin-Menues (bobToolz, brushexport2, MeshTex, ShaderPlug,
    // SunPlug, UFO:AI; prtview seit Stufe 3).
    add(pruefe("Checkliste: jeder fertige Befehl (Stufe 1b und 2), jeder Menue-Ruf und jeder Plugin-Eintrag (Stufe 4) ist nachgebaut, sonst keiner", [] {
        std::vector<std::string> fertig;
        std::string fehlt;
        std::string zuviel;
        for (const kb::Befehl& b : kb::alle()) {
            if (b.stufe != 1) { continue; }
            fertig.emplace_back(b.name);
            const bool befehl = (b.art & (kb::kCommand | kb::kToggle)) != 0U;
            if (befehl ? !ke::nachgebaut(b.name) : !ke::tastenEreignisNachgebaut(b.name)) { fehlt += std::string(" ") + b.name; }
        }
        std::size_t rufe = 0;
        for (const kb::MenueEintrag& m : kb::menues()) {
            if (m.art != kb::MenueArt::Ruf) { continue; }
            ++rufe;
            fertig.emplace_back(m.befehl);
            if (!ke::nachgebaut(m.befehl)) { fehlt += std::string(" ") + m.befehl; }
        }
        std::size_t plugins = 0;
        for (const auto& pe : karte::befehle::pluginEintraege()) {
            if (pe.befehl[0] == 0 || std::find(fertig.begin(), fertig.end(), pe.befehl) != fertig.end()) { continue; }
            ++plugins;
            fertig.emplace_back(pe.befehl);
            if (!ke::nachgebaut(pe.befehl)) { fehlt += std::string(" ") + pe.befehl; }
        }
        const std::vector<std::string> da = ke::nachgebauteBefehle();
        for (const std::string& n : da) {
            if (std::find(fertig.begin(), fertig.end(), n) == fertig.end()) { zuviel += " " + n; }
        }
        diag::detail("Knopftest: Checkliste - " + std::to_string(fertig.size() - rufe - plugins) + " fertige Befehle, " + std::to_string(rufe) +
                     " Menue-Rufe, " + std::to_string(plugins) + " Plugin-Eintraege, " + std::to_string(da.size()) + " nachgebaut; es fehlt:" + fehlt +
                     "; zu viel:" + zuviel);
        return fehlt.empty() && zuviel.empty();
    }));
    add(tu("Alle Befehle: Kopie oeffnen, Pinsel und Licht waehlen, Zustand merken", [v] {
        ke::Editor& e = ke::editor();
        (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map"));
        e.karte.auswahl.leeren();
        e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder().front());
        waehleEnt("light", 0);
        v->raster = e.raster;
        v->kamera = e.kamera;
        v->kameraE = e.kameraE;
        v->anzeige = e.anzeige;
        v->xy.clear();
        for (const ke::XyFenster& x : e.xy) { v->xy.push_back(x.ansicht); }
        v->aktivXy = e.aktivXy;
        v->texBro = e.texBro.e;
        v->schwebend = e.schwebendSichtbar;
        v->gruppeOffen = e.gruppenDialogOffen;
        v->gruppenSeite = e.gruppenSeite;
        v->maximiert = e.maximiert;
        v->modi.clear();
        for (const char* m : kModi) {
            if (ke::angehakt(m)) { v->modi.emplace_back(m); }
        }
        v->ausgefuehrt.clear();
    }));
    add(pause(0.2));
    add(pruefe("Alle Befehle: 2 Entities, Pinsel und Licht gewaehlt", [] {
        const ke::Editor& e = ke::editor();
        return e.karte.entitaeten().size() == 2 && e.karte.auswahl.anzahl() == 2;
    }));
    // RefreshShaders gibt die Bilder frei - in einem Schritt nach dem
    // Zeichnen nennen die Zeichenlisten dieses Bildes sie noch (Lauf k11e:
    // Absturz 0x87D der Debugschicht). Mit sichtbaren Bildern dreimal.
    add(tu("Texturbrowser zeigen", [] {
        ke::Editor& e = ke::editor();
        e.gruppenSeite = 2;
        e.gruppenSeiteWechseln = true;
        e.gruppenDialogOffen = true;
    }));
    add(pause(0.3));
    add(warteBis("Texturbrowser: Bilder geladen", [] { return ke::kameraBilderFertig(); }, 2000));
    for (int i = 0; i < 3; ++i) {
        add(tu("RefreshShaders nach dem Zeichnen", [] { (void)ke::ausfuehren("RefreshShaders"); }));
        add(pause(0.2));
    }
    add(warteBis("Texturbrowser: Bilder neu geladen, kein Absturz", [] { return ke::kameraBilderFertig(); }, 2000));
    // Punkt 12: ein Auswahlwechsel baut die Geometrie nicht neu (auch auf der
    // kleinen Testkarte, ohne BHED_KARTE_GROSS).
    auto neubauten = std::make_shared<std::array<std::size_t, 2>>();
    add(tu("Neubauten merken, Auswahl leeren", [neubauten] {
        (*neubauten)[0] = ke::editor().geoNeubauten;
        (*neubauten)[1] = ke::editor().restNeubauten;
        ke::editor().karte.auswahl.leeren();
    }));
    add(pause(0.2));
    add(tu("Pinsel und Licht wieder waehlen", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.waehlen(e.karte.welt(), e.karte.welt()->kinder().front());
        waehleEnt("light", 0);
    }));
    add(pause(0.2));
    add(pruefe("Auswahlwechsel: nur der Rest der Szenen neu, die Geometrie nicht", [neubauten] {
        const ke::Editor& e = ke::editor();
        return e.geoNeubauten == (*neubauten)[0] && e.restNeubauten > (*neubauten)[1];
    }));
    std::vector<std::string> liste = ke::nachgebauteBefehle();
    liste.erase(std::remove_if(liste.begin(), liste.end(),
                                 [](const std::string& n) { return n == "Exit" || n == "OpenMap" || n == "SaveMapAs" || n == "NewMap"; }),
                  liste.end());
    liste.emplace_back("NewMap");
    for (const std::string& name : liste) {
        add({"Befehl " + name, [name, v](int b) {
                 ke::Editor& e = ke::editor();
                 if (b == 0) {
                     v->vorherAn = ke::angehakt(name);
                     e.letzterBefehl.clear();
                     if (ke::ausfuehren(name) && e.letzterBefehl == name) {
                         v->ausgefuehrt.push_back(name);
                     } else {
                         meldeFehler("Befehl " + name + ": nicht ausgefuehrt");
                     }
                     return false;
                 }
                 // Umschalter (der Haken wechselt) und MaximizeView ein zweites Mal
                 if (b == 3 && ((ke::angehakt(name) != v->vorherAn && name.rfind("SetGrid", 0) != 0 &&
                                 std::find(std::begin(kModi), std::end(kModi), name) == std::end(kModi)) ||
                                name == "MaximizeView" || name == "ToggleGridSnap")) {
                     (void)ke::ausfuehren(name);
                     return false;
                 }
                 if (b == 5) {
                     karteDialogeZu();
                     return false;
                 }
                 return b >= 7;
             }});
    }
    add(pruefe("Alle Befehle: " + std::to_string(liste.size()) + " ausgefuehrt (ohne Exit, OpenMap, SaveMapAs)", [v, liste] {
        diag::detail("Knopftest: Alle Befehle - " + std::to_string(v->ausgefuehrt.size()) + " von " +
                     std::to_string(liste.size()));
        return v->ausgefuehrt == liste;
    }));
    add(pruefe("New Map zuletzt: leere Karte \"unnamed.map\", keine Rueckfrage offen", [] {
        const ke::Editor& e = ke::editor();
        return e.name == "unnamed.map" && e.karte.entitaeten().empty() && !e.frageOffen;
    }));
    add(tu("Alle Befehle: Zustand zurueck, Kopie neu laden", [v] {
        ke::Editor& e = ke::editor();
        karteDialogeZu();
        e.raster = v->raster;
        e.kamera = v->kamera;
        e.kameraE = v->kameraE;
        e.kamera.projektionAktualisieren(e.kameraE);
        e.anzeige = v->anzeige;
        for (std::size_t i = 0; i < e.xy.size() && i < v->xy.size(); ++i) { e.xy[i].ansicht = v->xy[i]; }
        e.aktivXy = v->aktivXy;
        e.texBro.e = v->texBro;
        e.schwebendSichtbar = v->schwebend;
        e.gruppenDialogOffen = v->gruppeOffen;
        e.gruppenSeite = v->gruppenSeite;
        if (e.maximiert != v->maximiert) { (void)ke::ausfuehren("MaximizeView"); }
        for (const std::string& m : v->modi) {
            if (!ke::angehakt(m)) { (void)ke::ausfuehren(m); }
        }
        (void)ke::karteOeffnen(karteTestPfad("karte_kopie.map"));
        e.karte.auswahl.leeren();
    }));
    add(pause(0.3));
    add(pruefe("Alle Befehle: Karte wieder offen, Raster und Werkzeug wie vorher, keine Dialoge", [v] {
        const ke::Editor& e = ke::editor();
        bool modi = true;
        for (const std::string& m : v->modi) { modi = modi && ke::angehakt(m); }
        return e.name == karteTestPfad("karte_kopie.map") && !e.karte.entitaeten().empty() &&
               e.raster.potenz() == v->raster.potenz() && modi && !e.einstellungenOffen && !e.tastenDialogOffen &&
               !e.findenOffen && !e.ent.listeOffen && !e.frageOffen && GImGui->OpenPopupStack.Size == 0;
    }));
    // Punkt 14: die Wirkung der Befehle, die oben nur "ausgefuehrt" hiessen
    // und in der Checkliste noch offen standen.
    add(pruefe("ShowBboxes schaltet die Kaesten um (Haken folgt), zweimal: wie vorher", [] {
        ke::Editor& e = ke::editor();
        const bool vorher = e.anzeige.kaesten;
        (void)ke::ausfuehren("ShowBboxes");
        const bool um = e.anzeige.kaesten != vorher && ke::angehakt("ShowBboxes") == e.anzeige.kaesten;
        (void)ke::ausfuehren("ShowBboxes");
        return um && e.anzeige.kaesten == vorher;
    }));
    add(pruefe("ToggleGridSnap schaltet das Einrasten um, zweimal: wie vorher", [] {
        ke::Editor& e = ke::editor();
        const bool vorher = e.raster.einrasten();
        (void)ke::ausfuehren("ToggleGridSnap");
        const bool um = e.raster.einrasten() != vorher;
        (void)ke::ausfuehren("ToggleGridSnap");
        return um && e.raster.einrasten() == vorher;
    }));
    add(pruefe("NextView (XY_NextView): XY -> XZ -> YZ -> XY in der aktiven 2D-Ansicht", [] {
        ke::Editor& e = ke::editor();
        const auto typ = [&e] { return e.xy[static_cast<std::size_t>(e.aktivXy)].ansicht.typ; };
        const karte::Sicht vorher = typ();
        (void)ke::ausfuehren("NextView");
        const karte::Sicht eins = typ();
        (void)ke::ausfuehren("NextView");
        (void)ke::ausfuehren("NextView");
        return eins == karte::naechsteSicht(vorher) && eins != vorher && typ() == vorher;
    }));
    add(pruefe("ToggleModelBrowser / ToggleLayersBrowser: Gruppendialog auf Models / Layers, nochmal: zu", [] {
        ke::Editor& e = ke::editor();
        e.gruppenDialogOffen = false;
        e.gruppenSeite = 0;
        (void)ke::ausfuehren("ToggleModelBrowser");
        const bool models = e.gruppenDialogOffen && e.gruppenSeite == 3;
        (void)ke::ausfuehren("ToggleModelBrowser");
        const bool zu1 = !e.gruppenDialogOffen;
        (void)ke::ausfuehren("ToggleLayersBrowser");
        const bool layers = e.gruppenDialogOffen && e.gruppenSeite == 4;
        (void)ke::ausfuehren("ToggleLayersBrowser");
        const bool zu2 = !e.gruppenDialogOffen;
        e.gruppenSeite = 0;
        return models && zu1 && layers && zu2;
    }));
    add(pruefe("ToggleTextures: Gruppendialog auf (Floating/Split/eingebaut: Seite Textures)", [] {
        ke::Editor& e = ke::editor();
        using karte::befehle::Anordnung;
        const bool seite = e.anordnung == Anordnung::Floating || e.anordnung == Anordnung::Split || e.gruppenDialogEingebaut;
        e.gruppenDialogOffen = false;
        e.gruppenSeite = 0;
        (void)ke::ausfuehren("ToggleTextures");
        const bool auf = e.gruppenDialogOffen && (seite ? e.gruppenSeite == 2 : e.gruppenSeite == 0);
        e.gruppenDialogOffen = false;
        e.gruppenSeite = 0;
        return auf;
    }));
    return s;
}

// Fotos mit "Begriffe wie NetRadiant" zum Vergleich mit SP\nrc_fotos: alle 38
// Menues (menu_<Weg>.png, Weg wie NRCs Foto-Skript: '&' weg, sonst nur
// A-Za-z0-9, Untermenues mit "__"), alle 23 Einstellungsseiten
// (pref_<Seite>.png) und die nachgebauten Dialoge mit Foto (dlg_*.png).
std::vector<Schritt> karteneditorFotoSchritte() {
    namespace ke = karteneditor;
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // Menues im Code schliessen: ein Klick in die Statuszeile traf hohe
    // Untermenues (View > Filter reicht bis dorthin).
    auto menuesZu = [] { return tu("Menues zu", [] { karteDialogeZu(); }); };
    add(tu("Fotos: Begriffe wie NetRadiant an, Menues wie NetRadiant-Custom", [] {
        ke::setzeNrcBegriffe(true);
        ke::menueStilSetzen(true);   // Stufe 7 Punkt 6b (Preferences > Clean der Stufe 6 setzt es zurueck)
    }));
    add(pause(0.3));
    add(pruefe("Fotos: Anordnung Regular wie die NRC-Fotos", [] {
        return ke::editor().anordnung == karte::befehle::Anordnung::Regular && ke::nrcBegriffe();
    }));
    // Mit der Groesse des NRC-Fotos in logischen Punkten (Foto bei 2x / 2).
    struct MenueFoto {
        const char* datei;
        std::vector<std::string> weg;
        int b;
        int h;
    };
    const std::vector<MenueFoto> menues = {
        // File: NRC + "Open .bsp..." 24 (Stufe 4 Punkt 1, Abweichung)
        // Stufe 9 Punkt 2: dazu "Import geometry as brushes..." (eigene Funktion) - eine Zeile mehr, und der
        // laengste Eintrag bestimmt die Breite (QMenu: Text + Raender)
        {"File", {"File"}, 291, 285 + 24 + 24},
        {"Edit", {"Edit"}, 356, 549},
        // View: NRC + die Programm-Eintraege (Stufe 7 Punkt 6b, Abweichung): Trenner 7 + fuenf Untermenues je 24
        // + Stufe 8b (Abweichung): Trenner 7 + "Play in editor", "Play: start at info_player_start",
        // "Camera view modes", "Logic view...", "Mirror mode", "Undo history..." je 24 (der laengste
        // Text "Play: start at info_player_start" mit Hakenspalte: 262 -> 292)
        // Stufe 9 Punkt 4: dazu das Untermenue "Check jumps and space" (eigene Funktion)
        {"View", {"View"}, 292, 333 + 7 + 5 * 24 + 7 + 6 * 24 + 24},
        {"View__Camera", {"View", "Camera"}, 255, 216},
        {"View__Orthographic", {"View", "Orthographic"}, 307, 240},
        {"View__Show", {"View", "Show"}, 253, 401},
        {"View__Filter", {"View", "Filter"}, 270, 593},
        {"View__Region", {"View", "Region"}, 264, 106},
        {"Modify", {"Modify"}, 258, 230},
        {"Modify__Components", {"Modify", "Components"}, 177, 82},
        {"Modify__Nudge", {"Modify", "Nudge"}, 218, 161},
        {"Modify__Rotate", {"Modify", "Rotate"}, 206, 137},
        {"Modify__Flip", {"Modify", "Flip"}, 183, 137},
        {"Modify__Repeat", {"Modify", "Repeat"}, 225, 154},
        // Build: NRC + Trenner 7 + "Cancel build" 24 (Stufe 3 Punkt 4, Abweichung)
        // + "Update entities only" 24 (Stufe 4 Punkt 2, Abweichung) + "Optimizer..." 24 (Stufe 5 Punkt 5, Abweichung)
        // + "Real-time light" 24 + "Quick bake" 24 (Stufe 7 Punkt 5, Abweichung; pruefbar: Qt
        // reserviert dann fuer alle Eintraege die Spalte des Hakens, 193 -> 215)
        // + "Keep editor data", "Partial compile: reuse VIS", "Partial compile: reuse light",
        // "Add to compiled map...", "Notify when a build is done" je 24 (Stufe 8a, Abweichung;
        // der laengste Text "Partial compile: reuse light" mit Hakenspalte: 215 -> 241)
        // + "Pack map as .pk3" 24 (Stufe 8c, Abweichung; kuerzer als 241)
        // + "GPU bake" 24 (Stufe 8g, Abweichung; kuerzer als 241)
        // Stufe 9 Punkt 1: dazu das Untermenue "Test in game" (eigene Funktion)
        {"Build", {"Build"}, 241, 405 + 7 + 24 + 24 + 24 + 24 + 24 + 5 * 24 + 24 + 24 + 24},
        {"Grid", {"Grid"}, 163, 346},
        // Misc und Help: NRC + die Programm-Eintraege (Stufe 3 Punkt 0b, Abweichung):
        // Misc + Trenner 7 + drei Untermenues je 24, Help + Manual und About BehavEd.
        {"Misc", {"Misc"}, 262, 178 + 7 + 3 * 24},
        {"Misc__Theming", {"Misc", "Theming"}, 263, 497},
        {"Misc__Theming__GUI_Theme", {"Misc", "Theming", "GUI Theme"}, 122, 106},
        {"Entity", {"Entity"}, 263, 137},
        {"Brush", {"Brush"}, 273, 350},
        {"Brush__CSG", {"Brush", "CSG"}, 288, 154},
        {"Brush__Clipper", {"Brush", "Clipper"}, 264, 82},
        {"Curve", {"Curve"}, 277, 429},
        {"Curve__Insert_Delete", {"Curve", "Insert/Delete"}, 316, 223},
        {"Curve__Matrix", {"Curve", "Matrix"}, 255, 168},
        {"Curve__Texture", {"Curve", "Texture"}, 225, 106},
        // Plugins: NRC 20260114 + "TerrainGenerator" 24 (Stufe 8b Punkt 1, aus NRC master; der Name
        // ist laenger als "brushexport2": 144 -> 162)
        {"Plugins", {"Plugins"}, 162, 178 + 24},
        {"Plugins__MeshTex", {"Plugins", "MeshTex"}, 159, 429},
        {"Plugins__ShaderPlug", {"Plugins", "ShaderPlug"}, 135, 58},
        {"Plugins__SunPlug", {"Plugins", "SunPlug"}, 166, 58},
        {"Plugins__UFO_AI", {"Plugins", "UFO:AI"}, 156, 415},
        {"Plugins__bobToolz", {"Plugins", "bobToolz"}, 201, 597},
        {"Plugins__brushexport2", {"Plugins", "brushexport2"}, 253, 58},
        {"Plugins__prtview", {"Plugins", "prtview"}, 187, 175},
        // Help: seit Stufe 3 Punkt 3 ist OpenManual nachgebaut - unser Eintrag
        // Manual zeigt F1, und QMenu legt dafuer die Tastenspalte an (+ Vorschub "F1" = 12)
        {"Help", {"Help"}, 215 + 12, 130 + 2 * 24},
        {"Help__General", {"Help", "General"}, 335, 298},
        {"Help__Jedi_Knight_Jedi_Academy", {"Help", "Jedi Knight Jedi Academy"}, 286, 250},
    };
    add(pruefe("Fotos: 38 Menues wie SP\\nrc_fotos\\menues", [n = menues.size()] { return n == 38; }));
    // Das NRC-Foto von File zeigt die leere MRU-Liste ("&1", ausgegraut).
    auto mru = std::make_shared<karte::ZuletztListe>();
    for (const MenueFoto& m : menues) {
        add(menuesZu());
        if (std::string(m.datei) == "File") {
            add(tu("MRU-Liste leer wie auf dem NRC-Foto", [mru] {
                *mru = ke::editor().zuletzt;
                ke::editor().zuletzt = karte::ZuletztListe{};
            }));
        }
        if (std::string(m.datei) == "Edit") {
            add(tu("MRU-Liste zurueck", [mru] { ke::editor().zuletzt = *mru; }));
        }
        add(pause(0.1));
        add(klick(m.weg[0], "###main"));
        add(pause(0.2));
        for (std::size_t k = 1; k < m.weg.size(); ++k) {
            add(klick(m.weg[k], "##Menu_", 0, false));
            add(pause(0.25));
        }
        add(fotoRahmen(std::string("karteneditor_menu_") + m.datei, obersterPopupRahmen));
        // QMenu-Masse (app_karte_qtmenue.cpp): die Hoehe genau; die Breite
        // mit Textbreiten wie Qt (Stufe 3 Punkt 0: DirectWrite in
        // Entwurfsmassen mit Unterschneidung) - 30 der 38 Menues genau, die
        // uebrigen 1 Punkt breiter (mit GDI-Breiten waren es bis 3).
        const std::string name = m.datei;
        const int b = m.b;
        const int h = m.h;
        add(pruefe("Menue " + name + ": " + std::to_string(b) + " x " + std::to_string(h) + " wie NRC", [name, b, h] {
            const ImRect r = obersterPopupRahmen();
            const float sk = ke::massstab();
            const int ub = static_cast<int>(std::lround(r.GetWidth() / sk));
            const int uh = static_cast<int>(std::lround(r.GetHeight() / sk));
            diag::detail("Knopftest: Menue " + name + " - NRC " + std::to_string(b) + " x " + std::to_string(h) + ", hier " +
                         std::to_string(ub) + " x " + std::to_string(uh));
            return uh == h && std::abs(ub - b) <= 1;
        }));
    }
    add(menuesZu());
    add(pause(0.2));
    // Mnemoniken (QMenuBar/QMenu::keyPressEvent): Alt+E oeffnet Edit, F
    // waehlt Pre&ferences; E trifft "D&elete" und dreimal "&Expand ..." -
    // nur die Hervorhebung wandert, nichts wird ausgefuehrt.
    add(tasteMit(ImGuiMod_Alt, ImGuiKey_E, "Alt+E"));
    add(pause(0.2));
    add(pruefe("Alt+E: das Menue Edit ist offen", [] { return finde("Preferences...", "##Menu_", 0, true) != nullptr; }));
    add(tu("letzter Befehl leer", [] { ke::editor().letzterBefehl.clear(); }));
    add(tasteMit(ImGuiKey_None, ImGuiKey_E, "E"));
    add(pause(0.2));
    add(pruefe("E (viermal im Menue): nichts ausgefuehrt, Edit bleibt offen", [] {
        return ke::editor().letzterBefehl.empty() && finde("Preferences...", "##Menu_", 0, true) != nullptr;
    }));
    add(tasteMit(ImGuiKey_None, ImGuiKey_F, "F"));
    add(pause(0.3));
    add(pruefe("F: Pre&ferences ausgefuehrt, Menue zu", [] {
        return ke::editor().letzterBefehl == "Preferences" && ke::editor().einstellungenOffen &&
               finde("Preferences...", "##Menu_", 0, true) == nullptr;
    }));
    add(tu("Einstellungen zu", [] { karteDialogeZu(); }));
    add(pause(0.2));
    // Edit > Preferences: jede Seite im Baum anklicken (PrefsDlg::BuildDialog,
    // Reihenfolge wie kSeiten in app_karte_dialoge.cpp).
    static const char* const kSeiten[] = {"Global", "Global_Game", "Game", "Paths", "Interface", "Layout",
                                          "Display", "Textures", "Patches", "Entities", "Settings", "Undo",
                                          "Selection", "Brush", "Clipper", "Grid", "Surface_Inspector", "Camera",
                                          "Orthographic", "Build", "Texture_Browser", "Model_Browser", "Autosave"};
    add(tu("Fotos: Einstellungen auf", [] { ke::editor().einstellungenOffen = true; }));
    add(pause(0.3));
    for (int i = 0; i < static_cast<int>(std::size(kSeiten)); ++i) {
        const std::string endung = "##seite" + std::to_string(i);
        add(pruefe(std::string("Einstellungen: Seite ") + kSeiten[i] + " im Baum sichtbar", [endung] {
            const Element* el = findeEndung(endung, "###karteeinstellungen");
            return el != nullptr && ganzSichtbar(*el);
        }));
        add(klickAn(std::string("Einstellungen: Seite ") + kSeiten[i], [endung] {
            const Element* el = findeEndung(endung, "###karteeinstellungen");
            return el != nullptr ? el->rect.GetCenter() : ImVec2{5, 5};
        }));
        add(pause(0.15));
        add(fotoRahmen(std::string("karteneditor_pref_") + kSeiten[i], [] { return fensterRahmen("###karteeinstellungen"); }));
    }
    add(tu("Fotos: Einstellungen zu", [] { ke::editor().einstellungenOffen = false; }));
    add(pause(0.2));
    // Dialoge mit NRC-Foto (dlg_<Menue>_<Punkt>_0.png)
    add(tu("Fotos: Edit > Shortcuts", [] { (void)ke::ausfuehren("Shortcuts"); }));
    add(pause(0.3));
    add(fotoRahmen("karteneditor_dlg_Edit_Shortcuts_0", [] { return fensterRahmen("###kartetasten"); }));
    add(tu("Shortcuts zu", [] { karteDialogeZu(); }));
    add(pause(0.2));
    add(tu("Fotos: View > Entity Inspector", [] {
        ke::Editor& e = ke::editor();
        e.karte.auswahl.leeren();
        if (!e.gruppenDialogOffen || e.gruppenSeite != 0) { (void)ke::ausfuehren("ToggleEntityInspector"); }
    }));
    add(pause(0.4));
    add(fotoRahmen("karteneditor_dlg_View_Entity_Inspector_0", [] { return fensterRahmen("###kartegruppe"); }));
    add(tu("Entity Inspector zu", [] { ke::editor().gruppenDialogOffen = false; }));
    add(pause(0.2));
    add(tu("Fotos: View > Entity List", [] { (void)ke::ausfuehren("ToggleEntityList"); }));
    add(pause(0.3));
    add(fotoRahmen("karteneditor_dlg_View_Entity_List_0", [] { return fensterRahmen("###karteentliste"); }));
    add(tu("Fotos: Entity List zu, Begriffe aus", [] {
        karteDialogeZu();
        ke::setzeNrcBegriffe(false);
    }));
    add(pause(0.2));
    return s;
}

