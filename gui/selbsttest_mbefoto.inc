// selbsttest_mbefoto.inc - MBENTWURF Teil A (10.11.): Fotos des Model-Tabs fuer den Abgleich Programm gegen Entwurf
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=mbefoto, BHED_FOTOS=<Ordner>)
//
// Reiner Foto-Lauf: prueft nichts und aendert kein Programmverhalten. Je Bedienmodus (wie 3ds Max, wie Blender) und
// Bereich (Ansehen/Pruefen, Modellieren, UV, Weights, Painting, Animation) das ganze Fenster, das Befehlsfeld in Seiten
// (alle Gruppen aufgeklappt), der Szenen-Explorer nach Hierarchie / nach Ebenen / mit Knochen, die Rechtsklick-Menues,
// vier Ansichten, Dopesheet und Kurveneditor. Dazu die Probe zum fremden Tooltip ueber dem Szenen-Explorer (shank
// 10.11.: "Build scripts ..." ueber "By hierarchy/By layers"): Maus auf dem Knopf der Modusleiste, Maus auf dem Explorer,
// ein waagerechter Schwenk, Strg+Tab und Alt ohne Mausbewegung - mit dem, was ImGui unter der Maus bzw. im
// Tastaturfokus sieht, im Protokoll ("Knopftest: MBE ...").
//   BHED_MBE_FENSTER=3840x2160@1.5   Fenstergroesse und Oberflaeche (Vorgabe), wie BHED_HB_FENSTER. Am Testrechner
//                                    (2560 x 1600 bei 150 %) zeigt das Fenster damit so viel wie ein maximiertes Fenster
//                                    bei Oberflaeche 1 - anderthalbfach vergroessert (Schrift 36 statt 24 Punkte hoch)
//   BHED_MBE_MODELL=<.glm>         Vorgabe models/players/kyle/model.glm (das Modell des Entwurfs)
// Eingebunden in selbsttest.cpp (nach selbsttest_mr.inc).
namespace mbefoto {

// Ein Element mit dieser Beschriftung, dessen Fenster (Kindfenster) den Namensteil traegt.
const Element* inFenster(const std::string& label, const char* innenTeil) {
    for (const Element& e : g_elemente) {
        if (e.label == label && e.innen.find(innenTeil) != std::string::npos) { return &e; }
    }
    return nullptr;
}

// Das Kindfenster, dessen letzter Namensteil mit `teil` beginnt (z. B. "mtrollouts", "mtbaum").
ImGuiWindow* kindFenster(const char* teil) {
    for (ImGuiWindow* w : GImGui->Windows) {
        if (!w->WasActive) { continue; }
        const char* letzter = std::strrchr(w->Name, '/');
        letzter = letzter != nullptr ? letzter + 1 : w->Name;
        if (std::strncmp(letzter, teil, std::strlen(teil)) == 0) { return w; }
    }
    return nullptr;
}

// AP2: die Stelle des Auges (wie 3ds Max an der Zeile: das der ersten Zeile mit Auge).
float augeX() {
    for (const ExplorerLage& l : modellExplorer().lage) {
        if (l.xAuge >= 0.0F) { return l.xAuge; }
    }
    return 0.0F;
}

ImGuiWindow* fensterGenau(const std::string& name) {
    for (ImGuiWindow* w : GImGui->Windows) {
        if (name == w->Name) { return w; }
    }
    return nullptr;
}

// Die sichtbaren Tooltips dieses Bildes mit ihrer Lage.
std::string tooltipLage() {
    std::string s;
    for (ImGuiWindow* w : GImGui->Windows) {
        if ((w->Flags & ImGuiWindowFlags_Tooltip) == 0 || !w->Active || w->Hidden) { continue; }
        char z[160];
        std::snprintf(z, sizeof z, "Tooltip %.0f/%.0f-%.0f/%.0f ", static_cast<double>(w->Pos.x), static_cast<double>(w->Pos.y),
                      static_cast<double>(w->Pos.x + w->Size.x), static_cast<double>(w->Pos.y + w->Size.y));
        s += z;
    }
    return s.empty() ? std::string("kein Tooltip") : s;
}

std::string rechteck(const char* name, const ImRect& r) {
    char z[200];
    std::snprintf(z, sizeof z, "%s %.0f/%.0f-%.0f/%.0f", name, static_cast<double>(r.Min.x), static_cast<double>(r.Min.y), static_cast<double>(r.Max.x),
                  static_cast<double>(r.Max.y));
    return z;
}

// Was der Tastaturfokus von ImGui gerade haelt (ein Element im Tastaturfokus gilt IsItemHovered als "unter der Maus").
std::string navLage() {
    ImGuiContext& g = *GImGui;
    const Element* e = g.NavId != 0 ? findeKennung(g.NavId) : nullptr;
    char z[400];
    std::snprintf(z, sizeof z, "NavCursorVisible %d, NavHighlightItemUnderNav %d, NavId %s in %s, NavWindow %s", g.NavCursorVisible ? 1 : 0,
                  g.NavHighlightItemUnderNav ? 1 : 0, g.NavId == 0 ? "-" : (e != nullptr ? (e->label.empty() ? "(ohne Namen)" : e->label.c_str()) : "?"),
                  e != nullptr ? e->innen.c_str() : "-", g.NavWindow != nullptr ? g.NavWindow->Name : "-");
    return z;
}

// Alle zugeklappten Gruppen und Baumknoten eines Kindfensters aufklappen (ein Durchgang: was erst danach erscheint, bleibt).
Schritt gruppenAuf(const char* fensterTeil) {
    return tu(std::string("MBE: Gruppen in ") + fensterTeil + " aufklappen", [=] {
        int n = 0;
        for (const Element& e : g_elemente) {
            if (e.innen.find(fensterTeil) == std::string::npos) { continue; }
            if ((e.flags & ImGuiItemStatusFlags_Openable) == 0 || (e.flags & ImGuiItemStatusFlags_Opened) != 0) { continue; }
            if (ImGuiWindow* w = fensterGenau(e.innen)) {
                w->StateStorage.SetInt(e.id, 1);
                ++n;
            }
        }
        diag::detail(std::string("Knopftest: MBE ") + fensterTeil + " - " + std::to_string(n) + " Gruppen aufgeklappt");
    });
}

// Ein Kindfenster seitenweise rollen und das ganze Fenster aufnehmen; Seiten hinter dem Ende entfallen.
Schritt seitenFoto(const std::string& name, const char* fensterTeil, int seite) {
    auto aus = std::make_shared<bool>(false);
    return {"MBE Foto " + name, [=](int b) {
                const char* ordner = std::getenv("BHED_FOTOS");
                if (ordner == nullptr) { return true; }
                if (b == 0) {
                    ImGuiWindow* w = kindFenster(fensterTeil);
                    if (w == nullptr) {
                        diag::detail("Knopftest: MBE Foto " + name + " - kein Fenster " + fensterTeil);
                        *aus = true;
                        return true;
                    }
                    const float seiteH = std::max(w->InnerRect.GetHeight() - ImGui::GetFontSize() * 2.0F, 32.0F);
                    if (seite > 0 && seiteH * static_cast<float>(seite - 1) >= w->ScrollMax.y - 1.0F) {
                        *aus = true;
                        return true;
                    }
                    ImGui::SetScrollY(w, std::min(seiteH * static_cast<float>(seite), w->ScrollMax.y));
                    char z[200];
                    std::snprintf(z, sizeof z, "Knopftest: MBE Foto %s - %s Seite %d, Rollen %.0f von %.0f, Hoehe %.0f", name.c_str(), fensterTeil, seite,
                                  static_cast<double>(std::min(seiteH * static_cast<float>(seite), w->ScrollMax.y)), static_cast<double>(w->ScrollMax.y),
                                  static_cast<double>(w->InnerRect.GetHeight()));
                    diag::detail(z);
                    return false;
                }
                if (*aus) { return true; }
                if (b == 4) { render::fotoAnfordern(std::string(ordner) + "\\" + name + ".bmp"); }
                return b >= 8;
            }};
}

// Das Befehlsfeld eines Bereichs: alles aufklappen, dann in Seiten.
void befehlsfeld(std::vector<Schritt>& s, const std::string& name) {
    s.push_back(gruppenAuf("mtrollouts"));
    s.push_back(pause(0.4));
    for (int k = 0; k < 14; ++k) { s.push_back(seitenFoto(name + "_feld" + std::to_string(k), "mtrollouts", k)); }
    s.push_back(tu("MBE: Befehlsfeld nach oben", [] {
        if (ImGuiWindow* w = kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
    }));
    s.push_back(pause(0.2));
}

// Die Maus auf ein Element stellen (ohne Klick) und warten, bis ein Tooltip da sein kann.
Schritt mausAuf(const std::string& was, std::function<const Element*()> ziel) {
    auto bis = std::make_shared<double>(0.0);
    return {"MBE Maus auf " + was, [=](int b) {
                if (b == 0) {
                    const Element* e = ziel();
                    if (e == nullptr) {
                        diag::detail("Knopftest: MBE Maus auf " + was + " - nicht gefunden");
                        return true;
                    }
                    setzeMaus(e->rect.GetCenter());
                    diag::detail("Knopftest: MBE Maus auf " + was + " - " + rechteck("Element", e->rect) + " in " + e->innen);
                    *bis = ImGui::GetTime() + 1.2;   // laenger als jede Tooltip-Verzoegerung
                }
                return ImGui::GetTime() >= *bis;
            }};
}

// Laeuft die Pose in diesem Bereich mit (shank 04.11.: "und in jedem Tab sieht man die animation?"): Abspielen an, nach
// einer halben Sekunde das Bild der Sequenz und die Zahl der neu gezeichneten Ansichten ins Protokoll, dann zurueck.
Schritt abspielProbe(const std::string& bereich) {
    struct Zustand {
        double bis = 0.0;
        int bild = 0;
        int gezeichnet = 0;
    };
    auto z = std::make_shared<Zustand>();
    return {"MBE Abspielen im Bereich " + bereich, [=](int b) {
                if (b == 0) {
                    z->bild = g_app->animFrame;
                    z->gezeichnet = g_app->modellNeuGezeichnet;
                    z->bis = ImGui::GetTime() + 0.6;
                    g_app->animPlaying = true;
                    return false;
                }
                if (ImGui::GetTime() < z->bis) { return false; }
                char zeile[240];
                std::snprintf(zeile, sizeof zeile, "Knopftest: MBE Abspielen im Bereich %s - Bild %d -> %d, Ansichten neu gezeichnet %d, spielt %d", bereich.c_str(), z->bild,
                              g_app->animFrame, g_app->modellNeuGezeichnet - z->gezeichnet, g_app->animPlaying ? 1 : 0);
                diag::detail(zeile);
                g_app->animPlaying = false;
                animWaehlen(g_app->animIndex, false);
                return true;
            }};
}

// Die Lage der Teile des Model-Tabs (Kindfenster) und die Namen der obersten Fenster dieses Bildes.
Schritt aufbauMelden(const std::string& was) {
    return tu("MBE Aufbau " + was, [=] {
        std::string z = "Knopftest: MBE Aufbau " + was + " - ";
        for (const char* n : {"modebar", "mtexplorer", "mtbaum", "animliste", "toolbar", "modelview", "mtansichten", "mtbefehlsfeld", "mtrollouts", "toolband"}) {
            if (const ImGuiWindow* w = kindFenster(n)) { z += rechteck(n, w->Rect()) + "; "; }
        }
        diag::detail(z);
        std::string oben = "Knopftest: MBE Fenster " + was + " - ";
        for (const ImGuiWindow* w : GImGui->Windows) {
            if (w->WasActive && !w->Hidden && (w->Flags & ImGuiWindowFlags_ChildWindow) == 0 && (w->Flags & ImGuiWindowFlags_Tooltip) == 0) { oben += std::string(w->Name) + "; "; }
        }
        diag::detail(oben);
    });
}

// Die Zeilen des Baums: Lage des Auges (Spalte links), Anfang der Zeile, Ende gegen den sichtbaren Rand.
Schritt baumMelden(const std::string& was) {
    return tu("MBE Baum " + was, [=] {
        const ImGuiWindow* w = kindFenster("mtbaum");
        if (w == nullptr) { return; }
        int zeilen = 0;
        int abgeschnitten = 0;
        float tiefste = 0.0F;
        std::string tiefsteName;
        std::string proben;
        for (const Element& e : g_elemente) {
            if (e.innen.find("mtbaum") == std::string::npos || e.label.empty() || e.label.rfind("##", 0) == 0) { continue; }
            ++zeilen;
            if (e.rect.Max.x > w->InnerRect.Max.x + 0.5F) { ++abgeschnitten; }
            if (e.rect.Min.x - augeX() > tiefste) {
                tiefste = e.rect.Min.x - augeX();
                tiefsteName = e.label;
            }
            for (const char* n : {"hips", "torso", "head", "*r_hand", "pelvis", "lower_lumbar", "rhand", "rhang_tag_bone"}) {
                if (e.label == n) {
                    char p[160];
                    std::snprintf(p, sizeof p, "%s: Zeile ab %.0f (Auge + %.0f) bis %.0f; ", n, static_cast<double>(e.rect.Min.x), static_cast<double>(e.rect.Min.x - augeX()),
                                  static_cast<double>(e.rect.Max.x));
                    proben += p;
                }
            }
        }
        char z[400];
        std::snprintf(z, sizeof z,
                      "Knopftest: MBE Baum %s - Fenster innen %.0f..%.0f (Breite %.0f), Inhalt %.0f breit, Rollen waagerecht bis %.0f / senkrecht bis %.0f, Augenspalte x %.0f, "
                      "%d beschriftete Zeilen, davon %d ueber den rechten Rand, tiefster Zeilenanfang Auge + %.0f (%s), Einzug je Ebene %.0f, Zeilenhoehe %.0f",
                      was.c_str(), static_cast<double>(w->InnerRect.Min.x), static_cast<double>(w->InnerRect.Max.x), static_cast<double>(w->InnerRect.GetWidth()),
                      static_cast<double>(w->ContentSize.x), static_cast<double>(w->ScrollMax.x), static_cast<double>(w->ScrollMax.y), static_cast<double>(augeX()), zeilen,
                      abgeschnitten, static_cast<double>(tiefste), tiefsteName.c_str(), static_cast<double>(ImGui::GetStyle().IndentSpacing),
                      static_cast<double>(ImGui::GetTextLineHeightWithSpacing()));
        diag::detail(z);
        diag::detail("Knopftest: MBE Baum " + was + " Zeilen - " + proben);
    });
}

Schritt lageMelden(const std::string& was) {
    return tu("MBE Lage " + was, [=] { diag::detail("Knopftest: MBE " + was + " - " + wasImGuiSieht() + " | " + tooltipLage() + " | " + navLage()); });
}

// Die Maus waagerecht von der Modusleiste in den Explorer, je Bildpunkt drei Bilder; jede Aenderung dessen, was unter der
// Maus liegt, ins Protokoll.
Schritt schwenk(bool aufHoeheHierarchie) {
    struct Zustand {
        float x = 0.0F;
        float x1 = 0.0F;
        float y = 0.0F;
        std::string letzter;
    };
    auto z = std::make_shared<Zustand>();
    return {std::string("MBE Tooltip: Schwenk von der Modusleiste in den Explorer") + (aufHoeheHierarchie ? " (Hoehe By hierarchy)" : " (Hoehe Events)"), [=](int b) {
                if (b == 0) {
                    const Element* e = inFenster(tr(Str::ViewEvents), "modebar");
                    const Element* h = inFenster(tr(Str::MbNachHierarchie), "mtexplorer");
                    if (e == nullptr || h == nullptr) {
                        diag::detail("Knopftest: MBE Tooltip Schwenk - Knopf nicht gefunden");
                        return true;
                    }
                    z->x = e->rect.Max.x - 4.0F;
                    z->x1 = h->rect.Min.x + 8.0F;
                    z->y = aufHoeheHierarchie ? h->rect.GetCenter().y : e->rect.GetCenter().y;
                    setzeMaus(ImVec2{z->x, z->y});
                    return false;
                }
                if (b % 3 != 0) { return false; }
                const std::string jetzt = wasImGuiSieht() + (tooltipLage() == "kein Tooltip" ? " | kein Tooltip" : " | Tooltip");
                if (jetzt != z->letzter) {
                    char k[80];
                    std::snprintf(k, sizeof k, "Knopftest: MBE Tooltip Schwenk x %.0f y %.0f - ", static_cast<double>(z->x), static_cast<double>(z->y));
                    diag::detail(k + jetzt + " | " + tooltipLage());
                    z->letzter = jetzt;
                }
                z->x += 1.0F;
                if (z->x > z->x1) { return true; }
                setzeMaus(ImVec2{z->x, z->y});
                return false;
            }};
}

ImVec2 ansichtEcke() {
    const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
    return ImVec2{a.lo[0] + 24.0F, a.lo[1] + a.groesse[1] - 24.0F};
}

// Das Element der Zeile "Knochen (<gla>, <n>)" des Explorers (AP2: der Kopf steht in modellExplorer().lage).
const Element* knochenKopfElement() {
    for (const ExplorerLage& l : modellExplorer().lage) {
        if (l.art == ExplorerArt::KopfKnochen) { return inFenster(l.name, "mtbaum"); }
    }
    return nullptr;
}

// Den Knoten "Bones" des Baums auf- oder zuklappen (AP2: ueber die Schnittstelle des Explorers).
Schritt knochenKnoten(bool auf) {
    return tu(std::string("MBE: Knoten Bones ") + (auf ? "auf" : "zu"), [=] {
        if (!auf) { modellExplorerKnochenAlleAuf(false); }
        modellExplorerAufklappen(ExplorerArt::KopfKnochen, 0, auf);
    });
}

// Offene Rechtsklick-Menues schliessen: Esc schliesst die Menues des Baums nicht (keine Tastaturnavigation), ein stehen
// gebliebenes Menue verdeckt sonst das naechste Foto und faengt den naechsten Klick ab.
Schritt popupsZu() {
    return tu("MBE: Menues schliessen", [] {
        if (GImGui->OpenPopupStack.Size > 0) { ImGui::ClosePopupToLevel(0, false); }
    });
}

// Alle Knochen mit Kindern aufklappen (AP2: ueber die Schnittstelle des Explorers).
Schritt knochenAlleAuf() {
    return tu("MBE: Knochen mit Kindern aufklappen", [] { modellExplorerKnochenAlleAuf(true); });
}

// Eine Zeile des Baums ins Bild rollen (so viele Zeilen darueber bleiben sichtbar).
Schritt baumRolleZu(const std::string& was, std::function<const Element*()> ziel, float zeilenDarueber) {
    return tu("MBE: " + was + " ins Bild rollen", [=] {
        const Element* e = ziel();
        ImGuiWindow* w = kindFenster("mtbaum");
        if (e == nullptr || w == nullptr) {
            diag::detail("Knopftest: MBE - " + was + " nicht gefunden");
            return;
        }
        ImGui::SetScrollY(w, std::clamp(w->Scroll.y + e->rect.Min.y - w->InnerRect.Min.y - ImGui::GetTextLineHeightWithSpacing() * zeilenDarueber, 0.0F, w->ScrollMax.y));
    });
}

// Die Sequenz der Fotos: BOTH_WALK1 (BOTH_STAND1 ist in Movie Duels nur zwei Bilder lang), sonst BOTH_STAND1, sonst die erste.
int sequenz() {
    for (const char* n : {"BOTH_WALK1", "BOTH_STAND1"}) {
        if (mb1::animNach(n) >= 0) { return mb1::animNach(n); }
    }
    return 0;
}

void tooltipProbe(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("MBE Tooltip: Lage der Leiste und des Explorers", [] {
        const Element* e = inFenster(tr(Str::ViewEvents), "modebar");
        const Element* h = inFenster(tr(Str::MbNachHierarchie), "mtexplorer");
        const Element* l = inFenster(tr(Str::MbNachEbenen), "mtexplorer");
        const ImGuiWindow* leiste = kindFenster("modebar");
        const ImGuiWindow* explorer = kindFenster("mtexplorer");
        std::string z = "Knopftest: MBE Tooltip Lage - ";
        if (e != nullptr) { z += rechteck("Knopf Events", e->rect) + ", "; }
        if (h != nullptr) { z += rechteck("Knopf By hierarchy", h->rect) + ", "; }
        if (l != nullptr) { z += rechteck("Knopf By layers", l->rect) + ", "; }
        if (leiste != nullptr) { z += rechteck("Fenster modebar", leiste->Rect()) + ", "; }
        if (explorer != nullptr) { z += rechteck("Fenster mtexplorer", explorer->Rect()); }
        diag::detail(z);
    }));
    // 1. die Maus steht auf dem Knopf "Events" der Modusleiste: sein Tooltip liegt rechts unter der Maus
    add(mausAuf("Events (Modusleiste)", [] { return inFenster(tr(Str::ViewEvents), "modebar"); }));
    add(lageMelden("Tooltip Maus auf Events"));
    add(foto("p_tooltip_maus_auf_events"));
    // 2. die Maus steht auf "By hierarchy" im Explorer
    add(mausAuf("By hierarchy (Explorer)", [] { return inFenster(tr(Str::MbNachHierarchie), "mtexplorer"); }));
    add(lageMelden("Tooltip Maus auf By hierarchy"));
    add(foto("p_tooltip_maus_auf_hierarchie"));
    add(mausAuf("By layers (Explorer)", [] { return inFenster(tr(Str::MbNachEbenen), "mtexplorer"); }));
    add(lageMelden("Tooltip Maus auf By layers"));
    // 3. Schwenk von der Leiste in den Explorer
    add(schwenk(false));
    add(schwenk(true));
    // 4. Tastaturfokus: Klick in die Ansicht, Maus bleibt dort stehen, Strg+Tab (Fensterwechsel von ImGui im Modus wie 3ds Max)
    add(klickAn("in die Ansicht (Fokus)", ansichtEcke));
    add(pause(0.3));
    add(lageMelden("Tooltip vor Strg+Tab"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Tab, "Strg+Tab"));
    add(pause(1.0));
    add(lageMelden("Tooltip nach Strg+Tab"));
    add(foto("p_tooltip_nach_strgtab"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Tab, "Strg+Tab"));
    add(pause(1.0));
    add(lageMelden("Tooltip nach zweitem Strg+Tab"));
    add(foto("p_tooltip_nach_strgtab2"));
    add(klickAn("in die Ansicht (Fokus)", ansichtEcke));
    add(pause(0.3));
    // 5. Alt allein (Menue-Ebene von ImGui)
    add(Schritt{"MBE Taste Alt allein", [](int b) {
                    ImGuiIO& io = ImGui::GetIO();
                    if (b == 0) { io.AddKeyEvent(ImGuiMod_Alt, true); }
                    if (b == 2) { io.AddKeyEvent(ImGuiMod_Alt, false); }
                    return b >= 6;
                }});
    add(pause(1.0));
    add(lageMelden("Tooltip nach Alt"));
    add(foto("p_tooltip_nach_alt"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(klickAn("in die Ansicht (Fokus)", ansichtEcke));
    add(pause(0.3));
}

// Die Bilder eines Bedienmodus (0 wie 3ds Max, 1 wie Blender).
void modusBilder(std::vector<Schritt>& s, int bedienung) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const std::string p = bedienung == 0 ? "p_max_" : "p_blender_";
    add(tu(std::string("MBE: Bedienmodus ") + (bedienung == 0 ? "wie 3ds Max" : "wie Blender") + ", Ansehen/Pruefen", [=] {
        ModellTab& t = g_app->mtab;
        modellBedienungSetzen(bedienung);
        t.bereich = ModellBereich::Ansehen;
        t.vierAnsichten = false;
        t.aktiv = 3;
        t.kvOffen = false;
        t.auswahl = {mb1::teil("torso")};
        t.knochenWahl = -1;
        t.suche[0] = 0;
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(klickAn("in die Ansicht (Fokus)", ansichtEcke));
    add(pause(0.5));
    // --- Ansehen/Pruefen ---------------------------------------------------------------------------------------------
    add(foto(p + "pruefen"));
    add(aufbauMelden(p + "pruefen"));
    add(baumMelden(p + "pruefen, Knochen zu"));
    add(abspielProbe(p + "pruefen"));
    befehlsfeld(s, p + "pruefen");
    // Szenen-Explorer: nach Ebenen
    add(klick(tr(bedienung == 1 ? Str::MbXSammlungen : Str::MbNachEbenen)));   // AP2: wie Blender "Collections" / "Hierarchy"
    add(pause(0.4));
    add(foto(p + "pruefen_ebenen"));
    add(klick(tr(bedienung == 1 ? Str::MbXHierarchie : Str::MbNachHierarchie)));
    add(pause(0.3));
    // Rechtsklick-Menues: Flaeche, Tag, Sequenz, Modell, Ansicht
    add(baumRolleZu("torso", [] { return inFenster("torso", "mtbaum"); }, 4.0F));
    add(pause(0.3));
    add(klickAn("Rechtsklick torso", [] {
        const Element* e = inFenster("torso", "mtbaum");
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    }, 1));
    add(pause(0.4));
    add(foto(p + "pruefen_menue_flaeche"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(popupsZu());
    add(pause(0.2));
    add(tu("MBE: *r_hand ins Bild", [] {
        const Element* e = inFenster("*r_hand", "mtbaum");
        ImGuiWindow* w = kindFenster("mtbaum");
        if (e != nullptr && w != nullptr) {
            ImGui::SetScrollY(w, std::clamp(w->Scroll.y + e->rect.Min.y - w->InnerRect.Min.y - ImGui::GetFontSize() * 6.0F, 0.0F, w->ScrollMax.y));
        }
    }));
    add(pause(0.3));
    add(klickAn("Rechtsklick *r_hand", [] {
        const Element* e = inFenster("*r_hand", "mtbaum");
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    }, 1));
    add(pause(0.4));
    add(foto(p + "pruefen_menue_tag"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(popupsZu());
    add(pause(0.2));
    // Knochen aufklappen: der tiefe Baum (Einrueckung, waagerechter Rollbalken)
    add(baumRolleZu("Knochen-Kopf", knochenKopfElement, 3.0F));
    add(pause(0.3));
    add(knochenKnoten(true));
    for (int k = 0; k < 14; ++k) {   // je Durchgang klappt die naechste Ebene der Knochen auf
        add(pause(0.1));
        add(knochenAlleAuf());
    }
    add(pause(0.4));
    add(tu("MBE: Knochen ins Bild", [] {
        const Element* e = knochenKopfElement();
        ImGuiWindow* w = kindFenster("mtbaum");
        if (e != nullptr && w != nullptr) {
            ImGui::SetScrollY(w, std::clamp(w->Scroll.y + e->rect.Min.y - w->InnerRect.Min.y - ImGui::GetFontSize() * 1.0F, 0.0F, w->ScrollMax.y));
        }
    }));
    add(pause(0.4));
    add(foto(p + "pruefen_knochen"));
    add(baumMelden(p + "pruefen, Knochen offen"));
    add(baumRolleZu("lower_lumbar", [] { return inFenster("lower_lumbar", "mtbaum"); }, 4.0F));
    add(pause(0.3));
    add(klickAn("Rechtsklick lower_lumbar", [] {   // auf den Namen: die Zeile reicht ueber den rechten Rand des Felds hinaus
        const Element* e = inFenster("lower_lumbar", "mtbaum");
        return e != nullptr ? ImVec2{e->rect.Min.x + ImGui::GetFontSize() * 2.5F, e->rect.GetCenter().y} : ImVec2{};
    }, 1));
    add(pause(0.4));
    add(foto(p + "pruefen_menue_knochen"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(popupsZu());
    add(pause(0.2));
    add(tu("MBE: Baum zum tief liegenden Knochen rhand gerollt (senkrecht und waagerecht)", [] {
        const Element* e = inFenster("rhand", "mtbaum");
        ImGuiWindow* w = kindFenster("mtbaum");
        if (e == nullptr || w == nullptr) {
            diag::detail("Knopftest: MBE - Knochen rhand nicht im Baum");
            return;
        }
        ImGui::SetScrollY(w, std::clamp(w->Scroll.y + e->rect.Min.y - w->InnerRect.Min.y - ImGui::GetTextLineHeightWithSpacing() * 6.0F, 0.0F, w->ScrollMax.y));
        ImGui::SetScrollX(w, std::clamp(w->Scroll.x + e->rect.Min.x - w->InnerRect.Min.x - ImGui::GetFontSize() * 3.0F, 0.0F, w->ScrollMax.x));
    }));
    add(pause(0.4));
    add(foto(p + "pruefen_knochen_tief"));
    add(tu("MBE: Baum zurueck", [] {
        if (ImGuiWindow* w = kindFenster("mtbaum")) {
            ImGui::SetScrollX(w, 0.0F);
            ImGui::SetScrollY(w, 0.0F);
        }
    }));
    add(pause(0.3));
    add(baumRolleZu("Knochen-Kopf", knochenKopfElement, 3.0F));
    add(pause(0.3));
    add(knochenKnoten(false));
    add(pause(0.3));
    add(tu("MBE: Baum nach oben", [] {
        if (ImGuiWindow* w = kindFenster("mtbaum")) { ImGui::SetScrollY(w, 0.0F); }
    }));
    add(tu("MBE: Sequenz ins Bild", [] {
        animWaehlen(sequenz(), false);
        g_app->animScrollen = true;
    }));
    add(pause(0.5));
    add(klickAn("Rechtsklick Sequenz", [] {
        const Element* e = g_app->animIndex >= 0 ? inFenster(g_app->animList[static_cast<std::size_t>(g_app->animIndex)].name, "mtbaum") : nullptr;
        return e != nullptr ? e->rect.GetCenter() : ImVec2{};
    }, 1));
    add(pause(0.4));
    add(foto(p + "pruefen_menue_sequenz"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(popupsZu());
    add(pause(0.2));
    add(klickAn("Rechtsklick in die Ansicht", [] {
        const ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        return ImVec2{a.lo[0] + a.groesse[0] * 0.25F, a.lo[1] + a.groesse[1] * 0.3F};
    }, 1));
    add(pause(0.5));
    add(foto(p + "pruefen_menue_ansicht"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(popupsZu());
    add(pause(0.2));
    // Alle Aktionen suchen (X bzw. F3)
    add(tu("MBE: Alle Aktionen suchen", [] { g_app->mtab.aktionenOffen = true; }));
    add(pause(0.5));
    add(foto(p + "pruefen_aktionen"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pause(0.3));
    // vier Ansichten
    add(tu("MBE: vier Ansichten, Knochen", [] {
        g_app->mtab.vierAnsichten = true;
        g_app->modellKnochen = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(foto(p + "pruefen_vier"));
    add(tu("MBE: eine Ansicht", [] {
        g_app->mtab.vierAnsichten = false;
        g_app->modellKnochen = false;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    // --- Modellieren, UV, Weights, Painting ---------------------------------------------------------------------------
    const struct {
        ModellBereich bereich;
        const char* name;
    } kBereiche[] = {{ModellBereich::Modellieren, "modell"}, {ModellBereich::Uv, "uv"}, {ModellBereich::Weights, "weights"}, {ModellBereich::Painting, "paint"}};
    for (const auto& b : kBereiche) {
        const ModellBereich bereich = b.bereich;
        const std::string name = b.name;
        add(tu("MBE: Bereich " + name, [=] {
            ModellTab& t = g_app->mtab;
            t.bereich = bereich;
            t.auswahl = {mb1::teil("torso")};
            t.knochenWahl = bereich == ModellBereich::Weights ? boneIndex(g_app->anim, "lhumerus") : -1;
            g_app->modelDirty = true;
        }));
        add(pause(1.2));
        add(foto(p + name));
        add(aufbauMelden(p + name));
        add(abspielProbe(p + name));
        befehlsfeld(s, p + name);
    }
    // --- Animation --------------------------------------------------------------------------------------------------------
    add(tu("MBE: Bereich Animation", [] {
        ModellTab& t = g_app->mtab;
        t.bereich = ModellBereich::Animation;
        t.knochenWahl = -1;
        g_app->modelDirty = true;
    }));
    add(pause(1.0));
    add(foto(p + "anim"));
    add(aufbauMelden(p + "anim"));
    add(abspielProbe(p + "anim"));
    add(tu("MBE: Control Rig anlegen, Griff Hand R", [] {
        (void)modellRigAnlegen();
        kv5::testSchluessel();   // drei Schluessel der rechten Hand und des Kopfs bei +0 / +10 / +20
        modellRigGriffWaehlen(rig3::griffNach(modell::GriffArt::Hand, modell::Seite::Rechts));
        g_app->animFrame = g_app->animList[static_cast<std::size_t>(g_app->animIndex)].firstFrame + 12;
        g_app->modelDirty = true;
    }));
    add(pause(1.0));
    add(foto(p + "anim_rig"));
    befehlsfeld(s, p + "anim");
    add(tu("MBE: Dopesheet", [] {
        g_app->mtab.kvOffen = true;
        g_app->mtab.kvArt = 0;
    }));
    add(pause(0.8));
    add(foto(p + "anim_dopesheet"));
    add(tu("MBE: Kurveneditor", [] { g_app->mtab.kvArt = 1; }));
    add(pause(0.8));
    add(foto(p + "anim_kurven"));
    add(tu("MBE: Editor zu, vier Ansichten", [] {
        g_app->mtab.kvOffen = false;
        g_app->mtab.vierAnsichten = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(foto(p + "anim_vier"));
    add(tu("MBE: eine Ansicht, Abspielen", [] {
        g_app->mtab.vierAnsichten = false;
        g_app->animPlaying = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.7));
    add(foto(p + "anim_spielt"));
    add(tu("MBE: Abspielen aus", [] {
        g_app->animPlaying = false;
        animWaehlen(sequenz(), false);
    }));
    add(pause(0.3));
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    if (nur == nullptr || std::string(nur) != "mbefoto") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_MBE_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    const char* m = std::getenv("BHED_MBE_MODELL");
    const std::string modell = m != nullptr ? m : "models/players/kyle/model.glm";
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    auto skala = std::make_shared<float>(1.0F);
    add(tu("MBE: Fenster fuer die Fotos, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mbe.txt");
    }));
    add(pause(0.8));
    for (Schritt& x : mb1::laden(modell)) { add(std::move(x)); }
    add(tu("MBE: Aufteilung wie neue Einstellungen, Saebel an *r_hand, BOTH_WALK1, torso gewaehlt", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        g_app->settings.splitModel = Settings{}.splitModel;
        (void)modellAnbauen("models/weapons2/saber_1/saber_1.glm", "*r_hand", false, true);
        animWaehlen(sequenz(), false);
        g_app->animScrollen = true;
        g_app->mtab.auswahl = {mb1::teil("torso")};
        char zeile[420];
        std::snprintf(zeile, sizeof zeile,
                      "Knopftest: MBE - %s, %zu Teile, %d Stufen, %zu Knochen, %zu Sequenzen, Anzeige %.0f x %.0f, Oberflaeche %.2f, Bildschirm-Massstab %.2f, "
                      "Schrift %.0f Punkte hoch (Fenster %.0f Zeichenhoehen breit)",
                      g_app->modelPath.c_str(), g_app->mtab.dok.inhalt().teile.size(), g_app->mtab.dok.inhalt().stufenZahl, g_app->anim.bones.size(),
                      g_app->animList.size(), static_cast<double>(ImGui::GetIO().DisplaySize.x), static_cast<double>(ImGui::GetIO().DisplaySize.y),
                      static_cast<double>(g_app->uiScale), static_cast<double>(g_app->dpiScale), static_cast<double>(ImGui::GetFontSize()),
                      static_cast<double>(ImGui::GetIO().DisplaySize.x / std::max(ImGui::GetFontSize(), 1.0F)));
        diag::detail(zeile);
    }));
    add(pause(1.0));
    modusBilder(s, 0);
    tooltipProbe(s);
    modusBilder(s, 1);
    add(tu("MBE: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->uiScale = *skala;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->mtab.kvOffen = false;
        g_app->mtab.vierAnsichten = false;
        modellAnbautenLoesen();
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.3));
    return s;
}

}  // namespace mbefoto
