// selbsttest_mbkopf.inc - MBKOPF (10.11.): Kopfzeile der Ansichten, Anzeige-Schalter dort, ViewCube und Gizmo, Figur von
// vorne (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=kopf; die Bilder des Handbuchs: BHED_EDITORTEST=handbuch
// BHED_HB_NUR=mbkopf mit BHED_FOTOS)
//   kopf   Max: [+] / [Ansicht] / [Schattierung] mit Tooltip, Klick links und rechts, je Ansicht der vier eigene
//          Schattierung, "Knochen und Pruefen", Rechtsklick-Menue, Suche, das rechte Feld ohne Gruppe "Anzeige" mit dem
//          einmaligen Hinweis; 4a ueber den Katalog der Kopfzeile: jeder Anzeige-Schalter an/aus aendert das Bild (Foto
//          mit Ueberlagerung) in Ruhe und Pose, mit und ohne gewaehlte Flaeche, wie 3ds Max und wie Blender, in jedem
//          Bereich; Statistik, Hinweis und Nummern an ihrer Stelle; Vorne/Links der vier Ansichten gegen das Skelett;
//          ViewCube (Flaeche, Kante, Ecke, Home, Ziehen, Orthoansicht), Gizmo (Kugeln, Gegenseite, Ziehen, Knoepfe),
//          Ziffernblock-Schritte, Shift+Z, Strg+`, U.
// Die von shank gemeldeten Fehler stellt selbsttest_mbkopf_rot.inc nach (kopfrot, vor der Behebung rot).
// Eingebunden in selbsttest.cpp (nach selbsttest_mbkopf_rot.inc).
namespace mbkopf {

using mbkopfrot::foto;
using mbkopfrot::poseSetzen;
using mbkopfrot::pruefeMittig;
using mbkopfrot::pruefeVorne;

ModellTab& t() { return g_app->mtab; }
ModellAnsicht& ansicht(int n = -1) { return t().ansichten[n < 0 ? t().aktiv : n]; }

std::function<ImVec2()> feldOrt(int n, int feld) {
    return [=] {
        ImVec2 p{0.0F, 0.0F};
        if (!modellKopfFeldOrt(n, feld, p)) { meldeFehler("MBKOPF: Feld " + std::to_string(feld) + " der Ansicht " + std::to_string(n) + " nicht da"); }
        return p;
    };
}

// Das offene Menue eines Feldes klappt unter dem Feld auf (Viewport Label Menus von 3ds Max, Kopfzeile von Blender) - nicht
// an der Maus ueber dem Feld; ein Feld am rechten Rand der Ansicht richtet das Menue an seiner rechten Kante aus.
Schritt menueUnterFeld(const std::string& name, int n, int feld, bool rechts) {
    return pruefe("MBKOPF 2: das Menue " + name + " klappt unter seinem Feld auf", [=] {
        ImVec2 p{0.0F, 0.0F};
        const ImGuiWindow* w = GImGui->OpenPopupStack.empty() ? nullptr : GImGui->OpenPopupStack.back().Window;
        if (w == nullptr || !modellKopfFeldOrt(n, feld, p)) { return false; }
        const ModellAnsicht& a = ansicht(n);
        const ImRect r = w->Rect();
        char z[200];
        std::snprintf(z, sizeof z, "Knopftest: MBKOPF Menue %s - %.0f,%.0f-%.0f,%.0f, Mitte des Feldes %.0f,%.0f, Kopfzeile unten %.0f", name.c_str(),
                      static_cast<double>(r.Min.x), static_cast<double>(r.Min.y), static_cast<double>(r.Max.x), static_cast<double>(r.Max.y),
                      static_cast<double>(p.x), static_cast<double>(p.y), static_cast<double>(a.kopfUnten));
        diag::detail(z);
        const bool kante = rechts ? (r.Max.x >= p.x && r.Max.x <= a.lo[0] + a.groesse[0] + 1.0F) : (r.Min.x <= p.x && r.Min.x >= a.lo[0] - 1.0F);
        return r.Min.y >= a.kopfUnten - 1.0F && !r.Contains(p) && kante;
    });
}

std::function<ImVec2()> wuerfelOrt(int x, int y, int z) {
    return [=] {
        ImVec2 p{0.0F, 0.0F};
        if (!modellWuerfelOrt(t().aktiv, x, y, z, p)) { meldeFehler("MBKOPF: ViewCube nicht da"); }
        return p;
    };
}

std::function<ImVec2()> kompassOrt(int teil) {
    return [=] {
        ImVec2 p{0.0F, 0.0F};
        if (!modellKompassOrt(t().aktiv, teil, p)) { meldeFehler("MBKOPF: Kompass nicht da"); }
        return p;
    };
}

std::function<ImVec2()> pfeilOrt(int teil) {
    return [=] {
        ImVec2 p{0.0F, 0.0F};
        if (!modellWuerfelPfeilOrt(t().aktiv, teil, p)) { meldeFehler("MBKOPF: Pfeil " + std::to_string(teil) + " des ViewCube nicht da"); }
        return p;
    };
}

std::function<ImVec2()> gizmoOrt(int teil) {
    return [=] {
        ImVec2 p{0.0F, 0.0F};
        if (!modellGizmoOrt(t().aktiv, teil, p)) { meldeFehler("MBKOPF: Gizmo nicht da"); }
        return p;
    };
}

bool tooltipDa() {
    for (const ImGuiWindow* w : GImGui->Windows) {
        if (w->Active && !w->Hidden && (w->Flags & ImGuiWindowFlags_Tooltip) != 0) { return true; }
    }
    return false;
}

// Die Maus auf eine Stelle legen und stehen lassen: nach der Wartezeit von ImGui steht der Tooltip.
void tooltipSchritte(std::vector<Schritt>& s, const std::string& was, std::function<ImVec2()> ort) {
    s.push_back(tu("MBKOPF: Maus auf " + was, [=] { setzeMaus(ort()); }));
    s.push_back(pause(1.5));
    s.push_back(pruefe("MBKOPF 2: " + was + " zeigt unter der Maus seinen Tooltip", [] { return tooltipDa(); }));
}

// Blickrichtung der aktiven Ansicht gegen die erwartete, in Grad.
float winkelZu(float x, float y, float z) {
    float r[3];
    float h[3];
    float v[3];
    modellAnsichtAchsenVon(ansicht().art, r, h, v);
    const float l = std::sqrt(x * x + y * y + z * z);
    const float d = std::clamp((v[0] * x + v[1] * y + v[2] * z) / l, -1.0F, 1.0F);
    const float w = std::acos(d) * 180.0F / 3.14159265F;
    char zeile[160];
    std::snprintf(zeile, sizeof zeile, "Knopftest: MBKOPF Blick %.3f %.3f %.3f, %.2f Grad neben dem Soll, Ansicht %d", static_cast<double>(v[0]),
                  static_cast<double>(v[1]), static_cast<double>(v[2]), static_cast<double>(w), static_cast<int>(ansicht().art));
    diag::detail(zeile);
    return w;
}

Schritt pruefeBlick(const std::string& was, AnsichtArt art, float x, float y, float z) {
    return pruefe("MBKOPF " + was, [=] { return ansicht().art == art && winkelZu(x, y, z) < 0.5F; });
}

// Wie viele Bildpunkte zweier Fotos sich unterscheiden - alle und die ausserhalb des Rechtecks (Anteile des Bilds).
void andersWo(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b, int w, float x0, float y0, float x1, float y1, int& drin, int& draussen) {
    drin = 0;
    draussen = 0;
    if (a.size() != b.size() || w <= 0) { return; }
    const int h = static_cast<int>(a.size() / 4U) / w;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const std::size_t o = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4U;
            if (a[o] == b[o] && a[o + 1] == b[o + 1] && a[o + 2] == b[o + 2]) { continue; }
            const float fx = static_cast<float>(x) / static_cast<float>(w);
            const float fy = static_cast<float>(y) / static_cast<float>(h);
            ++(fx >= x0 && fx <= x1 && fy >= y0 && fy <= y1 ? drin : draussen);
        }
    }
}

// 4a ueber den Katalog der Kopfzeile: jeder Schalter an/aus aendert das Foto mit Ueberlagerung der aktiven Ansicht.
// Ausgegraut (mit Grund im Tooltip) sind nur die Flossen: ohne Knochen und im Blender-Modus (Oktaeder ohne Flossen).
bool alleSchalterWirken(const std::string& wo, bool mitAuswahl) {
    bool ok = true;
    t().auswahl.clear();
    if (mitAuswahl) { t().auswahl = {mb1::teil("torso")}; }
    const bool knochenVorher = g_app->modellKnochen;
    std::string ohne;
    for (int i = 0; i < modellKopfSchalterZahl(); ++i) {
        const std::string name = modellKopfSchalterText(i);
        if (modellKopfSchalterBrauchtAuswahl(i) && !mitAuswahl) { continue; }   // die Auswahlklammer: nur an einer Auswahl (wie 3ds Max)
        // MBRASTER: Schalter des anderen Bedienmodus und solche, die in dieser Ansicht nichts zeigen (Blender: Grid nur in den
        // Achsenansichten, Floor nur in den freien), gehoeren nicht zu dieser Ansicht
        if (!modellKopfSchalterHier(i, t().aktiv)) { continue; }
        // AP5: die Knochengewicht-Schwelle verschiebt bei 5 % (Vorgabe von ModView) Ecken um Bruchteile eines Bildpunkts - ihre Wirkung
        // prueft selbsttest_ap5.inc bei 0.25 gegen eine eigene Zaehlung der Ecken (P3a)
        if (name == tr(Str::PqSchwelle)) { continue; }
        if (name == tr(Str::MkFlossen)) {
            g_app->modellKnochen = false;
            const bool ohneKnochenGrau = !modellKopfSchalterGeht(i);
            g_app->modellKnochen = true;   // Flossen sitzen an den Knochen
            const bool geht = modellKopfSchalterGeht(i);
            if (!ohneKnochenGrau || geht == modellBlender()) {
                diag::detail("Knopftest FEHLER: MBKOPF 4a " + wo + ": " + name + " - ohne Knochen " + (ohneKnochenGrau ? "ausgegraut" : "bedienbar") +
                             ", mit Knochen " + (geht ? "bedienbar" : "ausgegraut"));
                ok = false;
            }
            if (!geht) {
                g_app->modellKnochen = knochenVorher;
                continue;
            }
        } else if (!modellKopfSchalterGeht(i)) {
            diag::detail("Knopftest FEHLER: MBKOPF 4a " + wo + ": " + name + " ausgegraut");
            ok = false;
            continue;
        }
        bool& w = modellKopfSchalterWert(i, t().aktiv);
        const bool vorher = w;
        const Camera kamera = g_app->modelCam;
        if (name == tr(Str::MkZweiseitig)) {   // Rueckseiten sieht man nur von innen
            for (int k = 0; k < 3; ++k) { g_app->modelCam.pos[k] = (g_app->model.mins[k] + g_app->model.maxs[k]) * 0.5F; }
        }
        const std::vector<unsigned char> a = foto();
        w = !vorher;
        const std::vector<unsigned char> b = foto();
        w = vorher;
        g_app->modelCam = kamera;
        g_app->modellKnochen = knochenVorher;
        const int d = mb1::unterschied(a, b);
        if (d <= 20) {
            ohne += std::string(ohne.empty() ? "" : ", ") + name + " (" + std::to_string(d) + ")";
            ok = false;
        }
    }
    t().auswahl.clear();
    g_app->modelDirty = true;
    diag::detail("Knopftest: MBKOPF 4a Katalog " + wo + (mitAuswahl ? ", torso gewaehlt" : ", nichts gewaehlt") + " - " +
                 std::to_string(modellKopfSchalterZahl()) + " Schalter, ohne Wirkung: " + (ohne.empty() ? "keiner" : ohne));
    return ok;
}

// Schaedel und Augenmitte auf dem Schirm einer Ansicht (fuer die Ansicht Links: wohin die Figur im Bild schaut).
bool nasenRichtung(int n, float& dx) {
    const int c = boneIndex(g_app->anim, "cranium");
    const int l = boneIndex(g_app->anim, "leye");
    const int r = boneIndex(g_app->anim, "reye");
    const std::vector<BoneMatrix>& w = g_app->modellWelt;
    if (c < 0 || l < 0 || r < 0 || static_cast<std::size_t>(std::max(c, std::max(l, r))) >= w.size()) { return false; }
    float pc[3];
    float pa[3];
    for (int k = 0; k < 3; ++k) {
        pc[k] = w[static_cast<std::size_t>(c)].m[k][3];
        pa[k] = (w[static_cast<std::size_t>(l)].m[k][3] + w[static_cast<std::size_t>(r)].m[k][3]) * 0.5F;
    }
    ImVec2 sc;
    ImVec2 sa;
    if (!modellAufSchirm(ansicht(n), pc, sc) || !modellAufSchirm(ansicht(n), pa, sa)) { return false; }
    dx = sa.x - sc.x;
    return true;
}

void kopfSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("MBKOPF: Skript fuer den Test, Max-Modus", [] {
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mbk.txt");
        modellBedienungSetzen(0);
        g_app->settings.modellNavigation = true;
        g_app->settings.modellNavigationAlle = false;
        g_app->settings.modellAnzeigeUmzugGesehen = false;
    }));
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    add(pause(0.4));
    // --- das rechte Feld: keine Gruppe "Anzeige" mehr, einmal der Hinweis -----------------------------------------
    // AP3: unter den Gruppen des rechten Felds ("mtrollouts") - "Display" heisst seither auch ein Reiter des Befehlsfelds
    add(pruefe("MBKOPF 5: das rechte Feld hat keine Gruppe Anzeige mehr", [] { return finde(tr(Str::MbRAnzeige), "mtrollouts", 0, false, "") == nullptr; }));
    add(klick(tr(Str::MkVerstanden)));
    add(pause(0.2));
    add(pruefe("MBKOPF 5: der Hinweis auf den neuen Ort ist bestaetigt und weg", [] {
        return g_app->settings.modellAnzeigeUmzugGesehen && finde(tr(Str::MkVerstanden), "", 0, false, "") == nullptr;
    }));
    // --- Max: die drei Felder ----------------------------------------------------------------------------------------
    add(tu("MBKOPF B4: Ansicht zurueckgesetzt", [] { modellAnsichtEinpassen(); }));
    add(pause(0.4));
    add(klickAn("MBKOPF B4: ViewCube Kante Vorne/Rechts als erster Klick nach dem Zuruecksetzen", wuerfelOrt(1, -1, 0)));
    add(pause(0.6));
    add(pruefeBlick("B4: der erste Klick auf die Kante dreht die Perspektive dorthin", AnsichtArt::Perspektive, -1.0F, 1.0F, 0.0F));
    add(tu("MBKOPF B4: Startansicht", [] { modellStartansicht(t().aktiv); }));
    add(pause(0.3));
    tooltipSchritte(s, "[+]", feldOrt(3, 0));
    tooltipSchritte(s, "[Perspektive]", feldOrt(3, 1));
    tooltipSchritte(s, "[Standard]", feldOrt(3, 2));
    tooltipSchritte(s, "ViewCube", wuerfelOrt(0, -1, 0));
    auto zaehler = std::make_shared<int>(0);
    add(tu("MBKOPF 2: Menue-Zaehler merken", [=] { *zaehler = t().kopfMenueZaehler; }));
    add(klickAn("MBKOPF 2: [+]", feldOrt(3, 0)));
    add(pause(0.2));
    add(pruefe("MBKOPF 2: [+] oeffnet sein Menue", [=] { return t().kopfMenueZaehler > *zaehler; }));
    add(menueUnterFeld("[+]", 3, 0, false));
    auto rasterVorher = std::make_shared<bool>(false);
    add(tu("MBKOPF 2: Raster der Ansicht merken (seit MBRASTER ab Werk an)", [=] { *rasterVorher = ansicht(3).sch.raster; }));
    add(klick(tr(Str::MkRaster)));
    add(pause(0.2));
    add(pruefe("MBKOPF 2: [+] > Raster schaltet das Raster der Ansicht", [=] { return ansicht(3).sch.raster != *rasterVorher; }));
    add(klickAn("MBKOPF 2: [+] mit Rechtsklick", feldOrt(3, 0), 1));
    add(pause(0.2));
    add(klick(tr(Str::MkKnochenPruefen)));
    add(pause(0.2));
    add(klick(tr(Str::MbAnKnochen), "##Menu_"));
    add(pause(0.2));
    add(pruefe("MBKOPF 2: [+] > Knochen und Pruefen > Knochen", [] { return g_app->modellKnochen; }));
    add(klickAn("MBKOPF 2: [Schattierung] mit Rechtsklick", feldOrt(3, 2), 1));
    add(pause(0.2));
    add(klick(tr(Str::MkDraht)));
    add(pause(0.2));
    add(pruefe("MBKOPF 2: [Schattierung] > Drahtgitter", [] { return ansicht(3).sch.drahtgitter; }));
    add(klickAn("MBKOPF 2: [Drahtgitter]", feldOrt(3, 2)));
    add(pause(0.2));
    add(klick(tr(Str::MbAnsichtStandard)));
    add(pause(0.2));
    add(pruefe("MBKOPF 2: [Drahtgitter] > Standard", [] { return !ansicht(3).sch.drahtgitter; }));
    add(klickAn("MBKOPF 2: [Perspektive]", feldOrt(3, 1)));
    add(pause(0.2));
    add(klick(tr(Str::MbAnsichtVorne)));
    add(pause(0.2));
    add(pruefeBlick("2: [Ansicht] > Vorne", AnsichtArt::Vorne, 0.0F, 1.0F, 0.0F));
    add(pruefeVorne("[Ansicht] > Vorne"));
    add(pruefeMittig("[Ansicht] > Vorne (wie die Taste F: eingepasst)"));
    add(klickAn("MBKOPF 2: [Vorne]", feldOrt(3, 1)));
    add(pause(0.2));
    add(klick(tr(Str::MbAnsichtPersp)));
    add(pause(0.2));
    add(pruefe("MBKOPF 2: [Ansicht] > Perspektive", [] { return ansicht(3).art == AnsichtArt::Perspektive; }));
    add(tu("MBKOPF 6: vier Ansichten, Raster und Knochen aus", [] {
        t().vierAnsichten = true;
        ansicht(3).sch.raster = false;
        g_app->modellKnochen = false;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    // --- B6: die Orthoansichten stimmen mit der Blickrichtung der Figur ueberein ------------------------------------
    add(pruefeVorne("vier Ansichten, die Ansicht Vorne", 1));
    add(pruefe("MBKOPF B6: die Ansicht Links zeigt die Figur von ihrer rechten Seite, sie schaut im Bild nach rechts (wie Left in 3ds Max)", [] {
        float zur = 0.0F;
        float lr = 0.0F;
        float dx = 0.0F;
        const bool da = mbkopfrot::gesicht(ansicht(2), zur, lr) && nasenRichtung(2, dx);
        diag::detail("Knopftest: MBKOPF Links - Gesicht zur Kamera " + std::to_string(zur) + ", Augen " + std::to_string(dx) + " Punkte rechts vom Schaedel");
        return da && ansicht(2).art == AnsichtArt::Links && std::fabs(zur) < 0.2F && dx > 2.0F;
    }));
    auto bild1 = std::make_shared<mb1::AnsichtBild>();
    auto bild0 = std::make_shared<mb1::AnsichtBild>();
    add(tu("MBKOPF 6: Bilder der Ansichten Oben und Vorne", [=] {
        (void)modellAnsichtPixel(0, bild0->px, bild0->w, bild0->h);
        (void)modellAnsichtPixel(1, bild1->px, bild1->w, bild1->h);
    }));
    add(klickAn("MBKOPF 6: [Standard] der Ansicht Oben", feldOrt(0, 2)));
    add(pause(0.2));
    add(klick(tr(Str::MkDraht)));
    add(pause(0.3));
    add(pruefe("MBKOPF 6: Drahtgitter nur in der Ansicht Oben - ihr Bild anders, Vorne gleich", [=] {
        mb1::AnsichtBild a;
        mb1::AnsichtBild b;
        (void)modellAnsichtPixel(0, a.px, a.w, a.h);
        (void)modellAnsichtPixel(1, b.px, b.w, b.h);
        const int d0 = mb1::unterschied(bild0->px, a.px);
        const int d1 = mb1::unterschied(bild1->px, b.px);
        diag::detail("Knopftest: MBKOPF 6 Oben " + std::to_string(d0) + ", Vorne " + std::to_string(d1));
        return ansicht(0).sch.drahtgitter && !ansicht(1).sch.drahtgitter && !ansicht(3).sch.drahtgitter && d0 > 500 && d1 == 0 && t().aktiv == 0;
    }));
    add(tasteKombi(ImGuiKey_F3, "F3"));
    add(pruefe("MBKOPF 6: F3 schaltet das Drahtgitter der aktiven Ansicht (Oben) wieder aus, die Taste gilt weiter", [] {
        return !ansicht(0).sch.drahtgitter && !ansicht(1).sch.drahtgitter && !ansicht(3).sch.drahtgitter;
    }));
    add(pruefe("MBKOPF B8: der ViewCube nur in der aktiven Ansicht (wie 3ds Max)", [] {
        return modellNavigationSichtbar(0) && !modellNavigationSichtbar(1) && !modellNavigationSichtbar(2) && !modellNavigationSichtbar(3);
    }));
    add(tu("MBKOPF B8: ViewCube in allen Ansichten", [] { g_app->settings.modellNavigationAlle = true; }));
    add(pause(0.2));
    add(pruefe("MBKOPF B8: in allen vier Ansichten gezeichnet", [] {
        ImVec2 p;
        bool ok = true;
        for (int n = 0; n < 4; ++n) { ok = ok && modellNavigationSichtbar(n) && modellWuerfelOrt(n, 9, 9, 9, p); }
        return ok;
    }));
    add(tu("MBKOPF 6: zurueck - eine Ansicht, Perspektive aktiv, ViewCube nur in der aktiven", [] {
        g_app->settings.modellNavigationAlle = false;
        t().vierAnsichten = false;
        t().aktiv = 3;
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    // --- Rechtsklick-Menue der Ansicht -------------------------------------------------------------------------------
    add(klickAn("MBKOPF 3: Rechtsklick in die Ansicht", [] {
        ImVec2 p;
        (void)modellFlaecheAufSchirm(mb1::teil("torso"), p);
        return p;
    }, 1));
    add(pause(0.3));
    add(klick(tr(Str::MkSchattierung)));
    add(pause(0.2));
    add(klick(tr(Str::MkZweiseitig)));
    add(pause(0.2));
    add(pruefe("MBKOPF 3: Rechtsklick > Schattierung > Zweiseitig", [] { return ansicht(3).sch.zweiseitig; }));
    add(tu("MBKOPF 3: zweiseitig aus", [] { ansicht(3).sch.zweiseitig = false; }));
    // --- Suche ------------------------------------------------------------------------------------------------------
    add(tasteKombi(ImGuiKey_X, "X"));
    add(pause(0.3));
    add(tippe(tr(Str::MkStatistik)));
    add(pause(0.2));
    add(klick(std::string("[  ] ") + tr(Str::MkStatistik) + " (7)"));
    add(pause(0.2));
    add(pruefe("MBKOPF 3: Alle Aktionen suchen findet Statistik und schaltet sie in der aktiven Ansicht", [] { return ansicht(3).sch.statistik; }));
    add(pruefe("MBKOPF 3: jeder Anzeige-Schalter hat Text und Tooltip", [] {
        bool ok = modellKopfSchalterZahl() >= 19;
        for (int i = 0; i < modellKopfSchalterZahl(); ++i) { ok = ok && modellKopfSchalterText(i)[0] != '\0' && modellKopfSchalterHinweis(i)[0] != '\0'; }
        return ok;
    }));
    add(tu("MBKOPF 3: Statistik aus", [] { ansicht(3).sch.statistik = false; }));
    add(pause(0.2));
    // --- 4a: jeder Schalter wirkt -----------------------------------------------------------------------------------
    for (const bool blender : {false, true}) {
        for (const bool pose : {false, true}) {
            const std::string wo = std::string(blender ? "Blender" : "Max") + (pose ? ", Pose" : ", Ruhe");
            add(tu("MBKOPF 4a: " + wo, [=] {
                modellBedienungSetzen(blender ? 1 : 0);
                t().bereich = ModellBereich::Ansehen;
                poseSetzen(pose);
                modellStartansicht(t().aktiv);
            }));
            add(pause(0.4));
            add(pruefe("MBKOPF 4a: jeder Schalter aendert das Bild - " + wo, [=] { return alleSchalterWirken(wo, false); }));
            add(pruefe("MBKOPF 4a: jeder Schalter aendert das Bild - " + wo + ", torso gewaehlt", [=] { return alleSchalterWirken(wo, true); }));
        }
    }
    for (int b = 1; b < kModellBereiche; ++b) {
        add(tu("MBKOPF 4a: Bereich " + std::to_string(b), [=] {
            modellBedienungSetzen(0);
            t().bereich = static_cast<ModellBereich>(b);
            poseSetzen(false);
        }));
        add(pause(0.4));
        add(pruefe("MBKOPF 4a: jeder Schalter aendert das Bild - Bereich " + std::to_string(b), [=] { return alleSchalterWirken("Bereich " + std::to_string(b), false); }));
    }
    add(tu("MBKOPF 4a: Ansehen, Pose, nichts gewaehlt", [] {
        t().bereich = ModellBereich::Ansehen;
        poseSetzen(true);
        t().auswahl.clear();
    }));
    add(pause(0.4));
    add(pruefe("MBKOPF 4a: einfarbiger Hintergrund und Sichtfeld aendern das Bild", [] {
        const std::vector<unsigned char> a = foto();
        g_app->settings.modellHintergrund = 0x333333;
        const int d0 = mb1::unterschied(a, foto());
        g_app->settings.modellHintergrund = -1;
        const float fov = g_app->modelCam.fovY;
        g_app->modelCam.fovY = 10.0F;
        const int d1 = mb1::unterschied(a, foto());
        g_app->modelCam.fovY = fov;
        diag::detail("Knopftest: MBKOPF 4a Hintergrund " + std::to_string(d0) + ", Sichtfeld " + std::to_string(d1));
        return d0 > 1000 && d1 > 1000;
    }));
    add(pruefe("MBKOPF 4a: die Statistik steht unten links, der Hinweis der Nummern ohne Auswahl oben unter der Kopfzeile", [] {
        const int w = static_cast<int>(ansicht().groesse[0]);
        const std::vector<unsigned char> a = foto();
        ansicht().sch.statistik = true;
        const std::vector<unsigned char> b = foto();
        ansicht().sch.statistik = false;
        t().an.eckNummern = true;
        const std::vector<unsigned char> c = foto();
        t().an.eckNummern = false;
        int drin = 0;
        int draussen = 0;
        andersWo(a, b, w, 0.0F, 0.9F, 0.6F, 1.0F, drin, draussen);
        const bool statistik = drin > 50 && draussen == 0;
        diag::detail("Knopftest: MBKOPF Statistik unten links " + std::to_string(drin) + ", anderswo " + std::to_string(draussen));
        andersWo(a, c, w, 0.0F, 0.0F, 1.0F, 0.15F, drin, draussen);
        diag::detail("Knopftest: MBKOPF Hinweis oben " + std::to_string(drin) + ", anderswo " + std::to_string(draussen) + ", gezeichnet " +
                     std::to_string(t().ueber[13]));
        return statistik && drin > 50 && draussen == 0;
    }));
    add(tu("MBKOPF 4a: torso gewaehlt", [] { t().auswahl = {mb1::teil("torso")}; }));
    add(pause(0.3));
    add(pruefe("MBKOPF 4a: die Ecken-Nummern stehen in der Pose an den posierten Ecken (Ecke 0 des torso), kein Hinweis mehr", [] {
        t().an.eckNummern = false;
        const std::vector<unsigned char> a = foto();
        t().an.eckNummern = true;
        const std::vector<unsigned char> b = foto();
        t().an.eckNummern = false;
        ImVec2 p;
        if (!modellEckeAufSchirm(mb1::teil("torso"), 0, p) || a.size() != b.size()) { return false; }
        const ModellAnsicht& v = ansicht();
        const int w = static_cast<int>(v.groesse[0]);
        const int px = static_cast<int>(p.x - v.lo[0]);
        const int py = static_cast<int>(p.y - v.lo[1]);
        const int r = static_cast<int>(ImGui::GetFontSize());
        int anders = 0;   // ab dem Punkt nach rechts oben steht " 0"
        for (int y = py - r; y <= py; ++y) {
            for (int x = px; x <= px + r; ++x) {
                const std::size_t o = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4U;
                if (x < 0 || y < 0 || x >= w || o + 3 >= a.size()) { continue; }
                anders += (a[o] != b[o] || a[o + 1] != b[o + 1] || a[o + 2] != b[o + 2]) ? 1 : 0;
            }
        }
        // die Ecke 0 in der Ruhelage liegt woanders: die Nummer folgt der Pose
        const float* ruhe = t().anzeige.surfaces[static_cast<std::size_t>(mb1::teil("torso"))].verts[0].xyz;
        ImVec2 pr;
        const bool ruheDa = modellAufSchirm(v, ruhe, pr);
        diag::detail("Knopftest: MBKOPF Nummer an Ecke 0: " + std::to_string(anders) + " Punkte bei " + std::to_string(px) + "/" + std::to_string(py) +
                     ", Ruhelage " + std::to_string(std::hypot(pr.x - p.x, pr.y - p.y)) + " Punkte daneben");
        return anders > 3 && ruheDa && std::hypot(pr.x - p.x, pr.y - p.y) > 3.0F;
    }));
    add(tu("MBKOPF 4a: Ruhe, nichts gewaehlt", [] {
        poseSetzen(false);
        t().auswahl.clear();
        modellBedienungSetzen(0);
    }));
    add(pause(0.3));
    // --- B6: Startansicht, B4: ViewCube --------------------------------------------------------------------------------
    add(tu("MBKOPF B4: Ansicht verdreht und verschoben", [] {
        g_app->modelOrbit.turn(g_app->modelCam, -110.0F, 35.0F);
        g_app->modelOrbit.pan(g_app->modelCam, -60.0F, 30.0F);
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(klickAn("MBKOPF B4: ViewCube Home", wuerfelOrt(9, 9, 9)));
    add(pause(0.9));
    add(pruefeBlick("B6: Home - Perspektive von vorne (Blick +Y)", AnsichtArt::Perspektive, 0.0F, 1.0F, 0.0F));
    add(pruefeVorne("ViewCube Home"));
    add(pruefeMittig("ViewCube Home"));
    add(klickAn("MBKOPF B4: ViewCube Kante Vorne/Rechts", wuerfelOrt(1, -1, 0)));
    add(pause(0.5));
    add(pruefeBlick("B4: Kante Vorne/Rechts - die Perspektive von vorne rechts", AnsichtArt::Perspektive, -1.0F, 1.0F, 0.0F));
    add(klickAn("MBKOPF B4: ViewCube Flaeche Rechts", wuerfelOrt(1, 0, 0)));
    add(pause(0.5));
    add(pruefeBlick("B4: Flaeche Rechts in der Perspektive - sie dreht dorthin und bleibt Perspektive (wie 3ds Max)", AnsichtArt::Perspektive, -1.0F, 0.0F, 0.0F));
    add(klickAn("MBKOPF B4: ViewCube Ecke Rechts/Hinten/Oben", wuerfelOrt(1, 1, 1)));
    add(pause(0.5));
    add(pruefeBlick("B4: Ecke Rechts/Hinten/Oben - die Perspektive von dort", AnsichtArt::Perspektive, -1.0F, -1.0F, -1.0F));
    add(klickAn("MBKOPF B4: ViewCube Flaeche Oben", wuerfelOrt(0, 0, 1)));
    add(pause(0.5));
    add(pruefeBlick("B4: Flaeche Oben in der Perspektive - senkrecht von oben, Perspektive", AnsichtArt::Perspektive, 0.0F, 0.0F, -1.0F));
    add(klickAn("MBKOPF B4: ViewCube Home", wuerfelOrt(9, 9, 9)));
    add(pause(0.9));
    add(klickAn("MBKOPF B7: in die Ansicht (unten links, nichts)", [] {
        const ModellAnsicht& a = ansicht();
        return ImVec2{a.lo[0] + 20.0F, a.lo[1] + a.groesse[1] - 20.0F};
    }));
    add(tasteKombi(ImGuiKey_T, "T"));
    add(pause(0.2));
    add(pruefeBlick("B7: T - die Orthoansicht Oben", AnsichtArt::Oben, 0.0F, 0.0F, -1.0F));
    add(klickAn("MBKOPF B4: ViewCube Flaeche Oben, Rand zu Vorne (Kante Oben/Vorne)", wuerfelOrt(0, -1, 1)));
    add(pause(0.5));
    add(pruefeBlick("B4: Kante in einer Orthoansicht - Orthographisch von vorne oben (wie 3ds Max)", AnsichtArt::Orthographisch, 0.0F, 1.0F, -1.0F));
    add(klickAn("MBKOPF B4: ViewCube Flaeche Vorne (aus Orthographisch)", wuerfelOrt(0, -1, 0)));
    add(pause(0.5));
    add(pruefeBlick("B4: Flaeche Vorne in einer Orthoansicht - die Ansicht Vorne", AnsichtArt::Vorne, 0.0F, 1.0F, 0.0F));
    add(pruefeVorne("ViewCube Flaeche Vorne (Orthoansicht)"));
    add(klickAn("MBKOPF B4: ViewCube Ecke Rechts/Vorne/Oben (aus der Orthoansicht)", wuerfelOrt(1, -1, 1)));
    add(pause(0.5));
    add(pruefeBlick("B4: Ecke in einer Orthoansicht - Orthographisch von rechts vorn oben (wie 3ds Max)", AnsichtArt::Orthographisch, -1.0F, 1.0F, -1.0F));
    add(klickAn("MBKOPF B4: ViewCube Home", wuerfelOrt(9, 9, 9)));
    add(pause(0.9));
    auto gier = std::make_shared<float>(0.0F);
    add(tu("MBKOPF B4: Gierwinkel merken", [=] { *gier = g_app->modelCam.angles[1]; }));
    add(ziehenAn("MBKOPF B4: am ViewCube ziehen", wuerfelOrt(0, -1, 0), 50.0F, 0.0F, 0));
    add(pause(0.2));
    add(pruefe("MBKOPF B4: Ziehen am ViewCube dreht die Kamera (wie Alt+Mitteltaste, 0,4 Grad je Bildpunkt: 50 Punkte = 20 Grad)", [=] {
        const float d = g_app->modelCam.angles[1] - *gier;
        diag::detail("Knopftest: MBKOPF Ziehen " + std::to_string(d) + " Grad");
        return ansicht().art == AnsichtArt::Perspektive && std::fabs(std::fabs(d) - 20.0F) < 2.5F && t().auswahl.empty();
    }));
    // --- ViewCube nach max_viewcube.h: Durchsichtigkeit, Kompass, Pfeile, Rollpfeile, Home-Flug, Home setzen, Menue ---
    add(tu("MBKOPF VC: Startansicht, Maus weg vom Wuerfel", [] {
        modellStartansicht(t().aktiv);
        const ModellAnsicht& a = ansicht();
        setzeMaus(ImVec2{a.lo[0] + 40.0F, a.lo[1] + a.groesse[1] - 40.0F});
    }));
    add(pause(0.5));
    add(pruefe("MBKOPF VC: ohne Maus darueber ist der ViewCube halb durchsichtig (0,55)", [] {
        diag::detail("Knopftest: MBKOPF VC Deckkraft ohne Maus " + std::to_string(modellWuerfelAlpha(t().aktiv)));
        return std::fabs(modellWuerfelAlpha(t().aktiv) - 0.55F) < 0.001F;
    }));
    add(tu("MBKOPF VC: Maus auf den Wuerfel", [] { setzeMaus(wuerfelOrt(0, -1, 0)()); }));
    add(pause(0.3));
    add(pruefe("MBKOPF VC: mit der Maus darueber deckend", [] { return std::fabs(modellWuerfelAlpha(t().aktiv) - 1.0F) < 0.001F; }));
    add(pruefe("MBKOPF VC: der Kompassring ist gezeichnet (N, O, S, W und der Ring), mit Blick auf Vorne stehen die Pfeile", [] {
        ImVec2 p;
        bool ok = true;
        for (int i = 0; i < 5; ++i) { ok = ok && modellKompassOrt(t().aktiv, i, p); }
        for (int i = 0; i < 6; ++i) { ok = ok && modellWuerfelPfeilOrt(t().aktiv, i, p); }
        return ok;
    }));
    tooltipSchritte(s, "Kompass", kompassOrt(4));
    // Pfeile: mit Blick gerade auf eine Flaeche zu den Nachbarflaechen
    add(klickAn("MBKOPF VC: Pfeil rechts neben der Flaeche Vorne", pfeilOrt(3)));
    add(pause(0.5));
    add(pruefeBlick("VC: Pfeil rechts von Vorne - die Nachbarflaeche Rechts, Perspektive", AnsichtArt::Perspektive, -1.0F, 0.0F, 0.0F));
    add(klickAn("MBKOPF VC: Pfeil oben ueber der Flaeche Rechts", pfeilOrt(0)));
    add(pause(0.5));
    add(pruefeBlick("VC: Pfeil oben von Rechts - die Flaeche Oben", AnsichtArt::Perspektive, 0.0F, 0.0F, -1.0F));
    add(klickAn("MBKOPF VC: ViewCube Kante Oben/Vorne (schraeg)", wuerfelOrt(0, -1, 1)));
    add(pause(0.5));
    add(pruefe("MBKOPF VC: schraeg gesehen gibt es keine Pfeile und Rollpfeile", [] {
        ImVec2 p;
        return !modellWuerfelPfeilOrt(t().aktiv, 0, p) && !modellWuerfelPfeilOrt(t().aktiv, 4, p);
    }));
    // Rollpfeile: 90 Grad um die Blickachse, eine Voreinstellung steht wieder aufrecht
    add(klickAn("MBKOPF VC: ViewCube Flaeche Vorne", wuerfelOrt(0, -1, 0)));
    add(pause(0.5));
    add(klickAn("MBKOPF VC: Rollpfeil rechts oben", pfeilOrt(4)));
    add(pause(0.5));
    add(pruefe("MBKOPF VC: Rollpfeil rechts - 90 Grad im Uhrzeigersinn um die Blickachse (Rollwinkel -90), der Blick bleibt", [] {
        const float* w = g_app->modelCam.angles;
        diag::detail("Knopftest: MBKOPF VC Rollen " + std::to_string(w[0]) + " " + std::to_string(w[1]) + " " + std::to_string(w[2]));
        return std::fabs(w[2] + 90.0F) < 0.01F && std::fabs(w[0]) < 0.01F && std::fabs(w[1] - 90.0F) < 0.01F;
    }));
    add(klickAn("MBKOPF VC: Rollpfeil links oben", pfeilOrt(5)));
    add(pause(0.5));
    add(klickAn("MBKOPF VC: Rollpfeil links oben, noch einmal", pfeilOrt(5)));
    add(pause(0.5));
    add(pruefe("MBKOPF VC: zweimal links - Rollwinkel +90", [] { return std::fabs(g_app->modelCam.angles[2] - 90.0F) < 0.01F; }));
    add(klickAn("MBKOPF VC: ViewCube Flaeche (gerollt)", wuerfelOrt(0, -1, 0)));
    add(pause(0.5));
    add(pruefe("MBKOPF VC: eine Voreinstellung steht aufrecht - der Klick auf die Flaeche nimmt den Rollwinkel zurueck", [] {
        return std::fabs(g_app->modelCam.angles[2]) < 0.01F && std::fabs(g_app->modelCam.angles[1] - 90.0F) < 0.01F;
    }));
    // Kompass: von fast oben liegen alle vier Buchstaben frei
    const float gierSoll[4] = {270.0F, 180.0F, 90.0F, 0.0F};   // von Norden (+Y), Osten (+X), Sueden (-Y), Westen (-X) zur Mitte schauen
    for (int i = 0; i < 4; ++i) {
        add(tu("MBKOPF VC: Blick von schraeg oben (Gier 110 + 90 * " + std::to_string(i) + ", Neigung 75)", [=] {
            const float w[3] = {75.0F, 110.0F + 90.0F * static_cast<float>(i), 0.0F};
            modellBlickSetzen(t().aktiv, AnsichtArt::Perspektive, w, false);
        }));
        add(pause(0.3));
        add(klickAn("MBKOPF VC: Kompass Buchstabe " + std::to_string(i), kompassOrt(i)));
        add(pause(0.5));
        add(pruefe("MBKOPF VC: Klick auf Buchstabe " + std::to_string(i) + " (N O S W) - von dort schauen, die Neigung bleibt", [=] {
            const float* w = g_app->modelCam.angles;
            diag::detail("Knopftest: MBKOPF VC Buchstabe " + std::to_string(i) + ": " + std::to_string(w[0]) + " " + std::to_string(w[1]) + " " + std::to_string(w[2]));
            return ansicht().art == AnsichtArt::Perspektive && std::fabs(std::remainder(w[1] - gierSoll[i], 360.0F)) < 0.01F && std::fabs(w[0] - 75.0F) < 0.01F &&
                   std::fabs(w[2]) < 0.01F;
        }));
    }
    auto winkel = std::make_shared<std::array<float, 3>>();
    add(tu("MBKOPF VC: Winkel merken", [=] { *winkel = {g_app->modelCam.angles[0], g_app->modelCam.angles[1], g_app->modelCam.angles[2]}; }));
    add(ziehenAn("MBKOPF VC: am Kompassring ziehen", kompassOrt(4), 30.0F, 0.0F, 0));
    add(pause(0.2));
    add(pruefe("MBKOPF VC: Ziehen am Ring dreht nur um die Hochachse - Neigung und Rollwinkel bleiben", [=] {
        const float* w = g_app->modelCam.angles;
        diag::detail("Knopftest: MBKOPF VC Ring " + std::to_string(w[1] - (*winkel)[1]) + " Grad Gier, Neigung " + std::to_string(w[0]));
        return std::fabs(std::remainder(w[1] - (*winkel)[1], 360.0F)) > 3.0F && w[0] == (*winkel)[0] && w[2] == (*winkel)[2] && ansicht().art == AnsichtArt::Perspektive;
    }));
    // Norden und die Figur: S steht unter der Flaeche Vorne - wer von Sueden schaut, sieht das Gesicht (am Skelett gemessen)
    add(tu("MBKOPF VC: Blick flach von links vorne (Gier 60, Neigung 10)", [] {
        const float w[3] = {10.0F, 60.0F, 0.0F};
        modellBlickSetzen(t().aktiv, AnsichtArt::Perspektive, w, false);
    }));
    add(pause(0.3));
    add(klickAn("MBKOPF VC: Kompass S (unter der Flaeche Vorne)", kompassOrt(2)));
    add(pause(0.5));
    add(pruefeVorne("Kompass S - wie 3ds Max liegt Sueden unter Vorne, Norden hinter der Figur"));
    add(tu("MBKOPF VC: Norden 90 Grad, Blick von schraeg oben", [] {
        g_app->settings.modellKompassNord = 90;
        const float w[3] = {75.0F, 110.0F, 0.0F};
        modellBlickSetzen(t().aktiv, AnsichtArt::Perspektive, w, false);
    }));
    add(pause(0.3));
    add(klickAn("MBKOPF VC: Kompass N bei Norden 90 Grad", kompassOrt(0)));
    add(pause(0.5));
    add(pruefe("MBKOPF VC: Winkel fuer Norden 90 - N liegt bei +X, der Klick schaut von dort (Gier 180)", [] {
        return std::fabs(std::remainder(g_app->modelCam.angles[1] - 180.0F, 360.0F)) < 0.01F;
    }));
    add(tu("MBKOPF VC: Norden 0", [] { g_app->settings.modellKompassNord = 0; }));
    // Home fliegt
    auto start = std::make_shared<std::array<float, 2>>();
    add(tu("MBKOPF VC: weit weg, verdreht und verschoben", [=] {
        const float w[3] = {40.0F, 200.0F, 0.0F};
        modellBlickSetzen(t().aktiv, AnsichtArt::Perspektive, w, false);
        g_app->modelOrbit.zoom(g_app->modelCam, -12.0F);
        g_app->modelOrbit.pan(g_app->modelCam, 80.0F, -40.0F);
        g_app->modelDirty = true;
        *start = {g_app->modelOrbit.distance, g_app->modelCam.angles[1]};
    }));
    add(pause(0.3));
    add(klickAn("MBKOPF VC: ViewCube Home", wuerfelOrt(9, 9, 9)));
    add(pause(0.12));
    add(pruefe("MBKOPF VC: Home fliegt - kurz nach dem Klick ist die Kamera unterwegs (Abstand und Gier zwischen Anfang und Home)", [=] {
        const float d = g_app->modelOrbit.distance;
        const float y = g_app->modelCam.angles[1];
        diag::detail("Knopftest: MBKOPF VC Home-Flug Abstand " + std::to_string((*start)[0]) + " -> " + std::to_string(d) + ", Gier " + std::to_string(y));
        return ansicht().animStart >= 0.0 && d < (*start)[0] && y < (*start)[1] && y > 90.0F;
    }));
    add(pause(0.8));
    add(pruefe("MBKOPF VC: der Flug endet genau auf der Home-Ansicht (wie die Startansicht: Winkel, Abstand, Ort)", [] {
        const Camera c = g_app->modelCam;
        const float d = g_app->modelOrbit.distance;
        modellStartansicht(t().aktiv);
        float ab = std::fabs(d - g_app->modelOrbit.distance);
        for (int k = 0; k < 3; ++k) { ab = std::max(ab, std::max(std::fabs(c.pos[k] - g_app->modelCam.pos[k]), std::fabs(c.angles[k] - g_app->modelCam.angles[k]))); }
        diag::detail("Knopftest: MBKOPF VC Home-Flug Ende - groesste Abweichung " + std::to_string(ab));
        return ansicht().animStart < 0.0 && ab < 0.001F;
    }));
    add(pruefeVorne("ViewCube Home nach dem Flug"));
    add(pruefeMittig("ViewCube Home nach dem Flug"));
    // Rechtsklick-Menue: Home setzen, orthographisch
    add(tu("MBKOPF VC: Blick von vorne links oben", [] {
        const float w[3] = {20.0F, 135.0F, 0.0F};
        modellBlickSetzen(t().aktiv, AnsichtArt::Perspektive, w, false);
        g_app->modelOrbit.zoom(g_app->modelCam, 3.0F);
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    auto home = std::make_shared<Camera>();
    add(klickAn("MBKOPF VC: Rechtsklick auf den ViewCube", wuerfelOrt(0, -1, 0), 1));
    add(pause(0.3));
    add(klick(tr(Str::MkHomeSetzen)));
    add(pause(0.2));
    add(tu("MBKOPF VC: Home gemerkt, dann Startansicht", [=] {
        *home = g_app->modelCam;
        modellStartansicht(t().aktiv);
    }));
    add(pause(0.3));
    add(klickAn("MBKOPF VC: ViewCube Home (gesetzt)", wuerfelOrt(9, 9, 9)));
    add(pause(0.9));
    add(pruefe("MBKOPF VC: Aktuelle Ansicht als Home setzen - Home fliegt genau dorthin", [=] {
        float ab = 0.0F;
        for (int k = 0; k < 3; ++k) { ab = std::max(ab, std::max(std::fabs(home->pos[k] - g_app->modelCam.pos[k]), std::fabs(home->angles[k] - g_app->modelCam.angles[k]))); }
        diag::detail("Knopftest: MBKOPF VC gesetztes Home - groesste Abweichung " + std::to_string(ab));
        return t().homeDa && ab < 0.001F;
    }));
    add(tu("MBKOPF VC: Ansicht zuruecksetzen (Werkzeugleiste)", [] { modellAnsichtEinpassen(); }));
    add(pause(0.4));
    add(pruefeVorne("Ansicht zuruecksetzen trotz gesetztem Home"));
    add(tu("MBKOPF VC: kein gesetztes Home mehr", [] { modellHomeZuruecksetzen(); }));
    add(klickAn("MBKOPF VC: Rechtsklick auf den ViewCube", wuerfelOrt(0, -1, 0), 1));
    add(pause(0.3));
    add(klick(tr(Str::MkOrtho), "##Popup"));
    add(pause(0.3));
    add(pruefeBlick("VC: Rechtsklick > Orthographisch - die freie Orthoansicht, die Richtung bleibt", AnsichtArt::Orthographisch, 0.0F, 1.0F, 0.0F));
    add(tu("MBKOPF VC: Blick von fast oben (Orthographisch)", [] {
        const float w[3] = {75.0F, 110.0F, 0.0F};
        modellBlickSetzen(t().aktiv, AnsichtArt::Orthographisch, w, false);
    }));
    add(pause(0.3));
    add(klickAn("MBKOPF VC: Kompass S in der Orthoansicht", kompassOrt(2)));
    add(pause(0.5));
    add(pruefeBlick("VC: Buchstabe in einer Orthoansicht - Orthographisch, die Neigung bleibt", AnsichtArt::Orthographisch, 0.0F, std::cos(75.0F * 3.14159265F / 180.0F),
                    -std::sin(75.0F * 3.14159265F / 180.0F)));
    add(klickAn("MBKOPF VC: Rechtsklick auf den ViewCube", wuerfelOrt(0, 0, 1), 1));
    add(pause(0.3));
    add(klick(tr(Str::MkOrtho), "##Popup"));
    add(pause(0.3));
    add(pruefe("MBKOPF VC: Rechtsklick > Orthographisch noch einmal - wieder die Perspektive", [] { return ansicht().art == AnsichtArt::Perspektive; }));
    // die Orthoansicht Oben: der Ring ist ein Kreis, Norden oben
    add(klickAn("MBKOPF VC: in die Ansicht (unten links, nichts)", [] {
        const ModellAnsicht& a = ansicht();
        return ImVec2{a.lo[0] + 20.0F, a.lo[1] + a.groesse[1] - 20.0F};
    }));
    add(tasteKombi(ImGuiKey_T, "T"));
    add(pause(0.3));
    add(pruefe("MBKOPF VC: in der Ansicht Oben ist der Ring ein Kreis - N oben, S unten, O rechts, W links, gleich weit", [] {
        ImVec2 p[4];
        for (int i = 0; i < 4; ++i) {
            if (!modellKompassOrt(t().aktiv, i, p[i])) { return false; }
        }
        const float ns = p[2].y - p[0].y;
        const float ow = p[1].x - p[3].x;
        diag::detail("Knopftest: MBKOPF VC Ring in Oben: N-S " + std::to_string(ns) + ", O-W " + std::to_string(ow));
        return ns > 20.0F && std::fabs(ns - ow) < 1.0F && std::fabs(p[0].x - p[2].x) < 1.0F && std::fabs(p[1].y - p[3].y) < 1.0F;
    }));
    add(tasteKombi(ImGuiKey_P, "P"));
    add(pause(0.2));
    // der Schalter im Menue [+] > ViewCube und in der Suche
    add(klickAn("MBKOPF VC: [+]", feldOrt(3, 0)));
    add(pause(0.2));
    add(klick(tr(Str::MkViewCube)));
    add(pause(0.2));
    add(klick(tr(Str::MkKompass)));
    add(pause(0.3));
    add(pruefe("MBKOPF VC: [+] > ViewCube > Kompass aus - der Ring ist weg, der Wuerfel bleibt", [] {
        ImVec2 p;
        return !g_app->settings.modellKompass && !modellKompassOrt(t().aktiv, 0, p) && modellWuerfelOrt(t().aktiv, 9, 9, 9, p);
    }));
    add(tasteKombi(ImGuiKey_X, "X"));
    add(pause(0.3));
    add(tippe(tr(Str::MkKompass)));
    add(pause(0.2));
    add(klick(std::string("[  ] ") + tr(Str::MkKompass)));
    add(pause(0.3));
    add(pruefe("MBKOPF VC: die Suche findet den Kompass und schaltet ihn wieder ein", [] {
        ImVec2 p;
        return g_app->settings.modellKompass && modellKompassOrt(t().aktiv, 0, p);
    }));
    add(tu("MBKOPF B7: Startansicht", [] { modellStartansicht(t().aktiv); }));
    add(klickAn("MBKOPF B7: in die Ansicht (unten links, nichts)", [] {
        const ModellAnsicht& a = ansicht();
        return ImVec2{a.lo[0] + 20.0F, a.lo[1] + a.groesse[1] - 20.0F};
    }));
    add(tasteKombi(ImGuiKey_U, "U"));
    add(pause(0.2));
    add(pruefeBlick("B7: U - Orthographisch, die Richtung bleibt", AnsichtArt::Orthographisch, 0.0F, 1.0F, 0.0F));
    add(tasteKombi(ImGuiKey_P, "P"));
    add(pause(0.2));
    add(tu("MBKOPF B8: ViewCube aus", [] { g_app->settings.modellNavigation = false; }));
    add(pause(0.2));
    add(pruefe("MBKOPF B8: ViewCube aus - nicht gezeichnet", [] {
        ImVec2 p;
        return !modellWuerfelOrt(t().aktiv, 0, -1, 0, p);
    }));
    add(tu("MBKOPF B8: ViewCube an", [] { g_app->settings.modellNavigation = true; }));
    // --- Blender: Kopfzeile rechts, Gizmo, Ziffernblock --------------------------------------------------------------
    add(tu("MBKOPF Blender: Modus wie Blender, Startansicht", [] {
        modellBedienungSetzen(1);
        modellStartansicht(t().aktiv);
    }));
    add(pause(0.3));
    add(pruefe("MBKOPF Blender: kein Kompass, keine Pfeile (Blender hat das Gizmo)", [] {
        ImVec2 p;
        return !modellKompassOrt(t().aktiv, 0, p) && !modellWuerfelPfeilOrt(t().aktiv, 0, p) && modellGizmoOrt(t().aktiv, 0, p);
    }));
    tooltipSchritte(s, "Blender View", feldOrt(3, 1));
    tooltipSchritte(s, "Blender Overlays", feldOrt(3, 3));
    tooltipSchritte(s, "Blender Shading Solid", feldOrt(3, 11));
    tooltipSchritte(s, "Blender Gizmo", gizmoOrt(6));
    add(klickAn("MBKOPF Blender: Shading Solid", feldOrt(3, 11)));
    add(pause(0.2));
    add(pruefe("MBKOPF Blender: Solid - weiss, kein Drahtgitter", [] { return ansicht(3).sch.weiss && !ansicht(3).sch.drahtgitter; }));
    add(klickAn("MBKOPF Blender: Shading Material Preview", feldOrt(3, 12)));
    add(pause(0.2));
    add(pruefe("MBKOPF Blender: Material Preview - mit Bildern", [] { return !ansicht(3).sch.weiss && !ansicht(3).sch.drahtgitter; }));
    add(klickAn("MBKOPF Blender: Shading-Ausklapper", feldOrt(3, 4)));
    add(pause(0.2));
    add(klick(tr(Str::MkBlRueckseiten)));
    add(pause(0.2));
    add(pruefe("MBKOPF Blender: Viewport Shading > Backface Culling aus - zweiseitig", [] { return ansicht(3).sch.zweiseitig; }));
    add(tu("MBKOPF Blender: einseitig, Menue-Zaehler merken", [=] {
        ansicht(3).sch.zweiseitig = false;
        *zaehler = t().kopfMenueZaehler;
    }));
    add(klickAn("MBKOPF Blender: Overlays", feldOrt(3, 3)));
    add(pause(0.2));
    add(pruefe("MBKOPF Blender: Overlays oeffnet sein Menue", [=] { return t().kopfMenueZaehler > *zaehler; }));
    add(menueUnterFeld("Blender Overlays", 3, 3, true));
    add(klick(tr(Str::MbAnUrsprung)));
    add(pause(0.2));
    add(pruefe("MBKOPF Blender: Overlays > Ursprung", [] { return t().an.ursprung; }));
    add(tu("MBKOPF Blender: Ursprung aus", [] { t().an.ursprung = false; }));
    add(klickAn("MBKOPF Blender: View", feldOrt(3, 1)));
    add(pause(0.2));
    add(klick(tr(Str::MkBlBlickpunkt)));
    add(pause(0.2));
    add(klick(tr(Str::MbAnsichtOben)));
    add(pause(0.2));
    add(pruefeBlick("Blender: View > Blickpunkt > Oben", AnsichtArt::Oben, 0.0F, 0.0F, -1.0F));
    add(klickAn("MBKOPF B5: Gizmo -Y", gizmoOrt(3)));
    add(pause(0.5));
    add(pruefeBlick("B5: Gizmo -Y - Vorne", AnsichtArt::Vorne, 0.0F, 1.0F, 0.0F));
    add(pruefeVorne("Gizmo -Y"));
    add(klickAn("MBKOPF B5: Gizmo -Y erneut", gizmoOrt(3)));
    add(pause(0.5));
    add(pruefeBlick("B5: erneuter Klick - die Gegenseite Hinten", AnsichtArt::Hinten, 0.0F, -1.0F, 0.0F));
    add(klickAn("MBKOPF B5: Gizmo +Z", gizmoOrt(4)));
    add(pause(0.5));
    add(pruefeBlick("B5: Gizmo +Z - Oben", AnsichtArt::Oben, 0.0F, 0.0F, -1.0F));
    add(ziehenAn("MBKOPF B5: am Gizmo ziehen (zwischen den Kugeln)", [] {
        ImVec2 a;
        ImVec2 b;
        (void)modellGizmoOrt(t().aktiv, 0, a);
        (void)modellGizmoOrt(t().aktiv, 1, b);
        const ImVec2 m{(a.x + b.x) * 0.5F, (a.y + b.y) * 0.5F};
        const float l = std::hypot(a.x - m.x, a.y - m.y);
        return ImVec2{m.x + l * 0.55F, m.y + l * 0.55F};
    }, 40.0F, 0.0F, 0));
    add(pause(0.2));
    add(pruefe("MBKOPF B5: Ziehen am Gizmo kreist - aus Oben wird die Perspektive (Auto Perspective)", [] { return ansicht().art == AnsichtArt::Perspektive; }));
    auto abstand = std::make_shared<float>(0.0F);
    auto ort = std::make_shared<std::array<float, 3>>();
    add(tu("MBKOPF B5: Abstand und Ort der Kamera merken", [=] {
        *abstand = g_app->modelOrbit.distance;
        *ort = {g_app->modelCam.pos[0], g_app->modelCam.pos[1], g_app->modelCam.pos[2]};
    }));
    add(ziehenAn("MBKOPF B5: am Knopf Zoom ziehen", gizmoOrt(6), 0.0F, -40.0F, 0));
    add(pause(0.2));
    add(pruefe("MBKOPF B5: Ziehen am Knopf Zoom aendert den Abstand", [=] {
        diag::detail("Knopftest: MBKOPF Zoom " + std::to_string(*abstand) + " -> " + std::to_string(g_app->modelOrbit.distance));
        return std::fabs(g_app->modelOrbit.distance - *abstand) > 1.0F;
    }));
    add(tu("MBKOPF B5: Ort der Kamera merken", [=] { *ort = {g_app->modelCam.pos[0], g_app->modelCam.pos[1], g_app->modelCam.pos[2]}; }));
    add(ziehenAn("MBKOPF B5: am Knopf Verschieben ziehen", gizmoOrt(7), 40.0F, 0.0F, 0));
    add(pause(0.2));
    add(pruefe("MBKOPF B5: Ziehen am Knopf Verschieben schiebt die Kamera seitwaerts, der Abstand bleibt", [=] {
        const float d = std::hypot(g_app->modelCam.pos[0] - (*ort)[0], g_app->modelCam.pos[1] - (*ort)[1]) + std::fabs(g_app->modelCam.pos[2] - (*ort)[2]);
        diag::detail("Knopftest: MBKOPF Verschieben " + std::to_string(d));
        return d > 0.5F;
    }));
    add(klickAn("MBKOPF B5: Knopf Perspektive/Orthographisch", gizmoOrt(8)));
    add(pause(0.3));
    // MBNAV: Blender view3d.view_persportho behaelt die Richtung (bis rc611 sprang der Knopf auf die naechste Achsenansicht)
    add(pruefe("MBKOPF B5: der Knopf schaltet in derselben Richtung auf orthographisch", [] { return ansicht().art == AnsichtArt::Orthographisch; }));
    add(tu("MBKOPF B7: Startansicht", [] { modellStartansicht(t().aktiv); }));
    add(klickAn("MBKOPF B7: in die Ansicht (unten links, nichts)", [] {
        const ModellAnsicht& a = ansicht();
        return ImVec2{a.lo[0] + 20.0F, a.lo[1] + a.groesse[1] - 20.0F};
    }));
    // MBNAV: Orbit Left dreht die Kamera nach links (Gier 90 -> 75; bis rc611 gespiegelt 105), Roll Left dreht die Szene im Bild
    // gegen den Uhrzeigersinn (Roll +15; bis rc611 -15) - view3d_navigate_view_orbit.cc, view3d_navigate_view_roll.cc; weich (200 ms)
    add(tasteKombi(ImGuiKey_Keypad4, "Ziffernblock 4"));
    add(pause(0.4));
    add(pruefe("MBKOPF B7: Ziffernblock 4 - 15 Grad nach links (Gier 90 -> 75)", [] { return std::fabs(g_app->modelCam.angles[1] - 75.0F) < 0.01F; }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_Keypad4, "Umschalt+Ziffernblock 4"));
    add(pause(0.4));
    add(pruefe("MBKOPF B7: Umschalt+Ziffernblock 4 - um 15 Grad gerollt", [] { return std::fabs(g_app->modelCam.angles[2] - 15.0F) < 0.01F; }));
    add(tasteKombi(ImGuiKey_Keypad9, "Ziffernblock 9"));
    add(pause(0.4));
    add(pruefe("MBKOPF B7: Ziffernblock 9 - Gegenseite (Gier + 180)", [] { return std::fabs(std::remainder(g_app->modelCam.angles[1] - 255.0F, 360.0F)) < 0.01F; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Keypad1, "Strg+Ziffernblock 1"));
    add(pause(0.4));
    add(pruefeBlick("B7: Strg+Ziffernblock 1 - Hinten", AnsichtArt::Hinten, 0.0F, -1.0F, 0.0F));
    add(tasteKombi(ImGuiKey_Keypad9, "Ziffernblock 9"));
    add(pruefeBlick("B7: Ziffernblock 9 in Hinten - Vorne", AnsichtArt::Vorne, 0.0F, 1.0F, 0.0F));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_Z, "Umschalt+Z"));
    add(pruefe("MBKOPF Blender: Umschalt+Z - Drahtgitter in der aktiven Ansicht", [] { return ansicht().sch.drahtgitter; }));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_Z, "Umschalt+Z"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_GraveAccent, "Strg+`"));
    add(pruefe("MBKOPF Blender: Strg+` - Gizmo aus", [] { return !g_app->settings.modellNavigation && !ansicht().sch.drahtgitter; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_GraveAccent, "Strg+`"));
    add(tu("MBKOPF: zurueck", [] {
        modellBedienungSetzen(0);
        g_app->settings.modellNavigation = true;
        modellStartansicht(t().aktiv);
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
}

// Die Bilder des Handbuchs (BHED_EDITORTEST=handbuch, BHED_HB_NUR=mbkopf): das Menue [+] mit "Knochen und Pruefen" und
// dem ViewCube (Max), der ViewCube mit Kompass unter der Maus, das Menue Overlays mit Shading-Knoepfen und Gizmo (Blender).
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    auto teiler = std::make_shared<float>(0.0F);
    add(tu("MBKOPF Handbuch: breite Anordnung, Max, BOTH_STAND1, Startansicht", [=] {
        *teiler = g_app->settings.splitModel;
        g_app->settings.splitModel = 0.85F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        g_app->settings.modellNavigation = true;
        modellBedienungSetzen(0);
        animWaehlen(mb1::animNach("BOTH_STAND1") >= 0 ? mb1::animNach("BOTH_STAND1") : 0, false);
        g_app->modellKnochen = true;
    }));
    add(pause(0.5));
    add(tu("MBKOPF Handbuch: Startansicht", [] { modellStartansicht(t().aktiv); }));
    add(pause(0.5));
    add(klickAn("MBKOPF Handbuch: [+]", feldOrt(3, 0)));
    add(pause(0.3));
    add(klick(tr(Str::MkKnochenPruefen)));
    add(pause(0.6));
    add(hilfe2::schrittFoto("h_mbk_max"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tu("MBKOPF Handbuch: ohne Knochen, Blick von vorne links oben", [] {
        g_app->modellKnochen = false;
        const float w[3] = {25.0F, 60.0F, 0.0F};
        modellBlickSetzen(t().aktiv, AnsichtArt::Perspektive, w, false);
    }));
    add(pause(0.4));
    add(tu("MBKOPF Handbuch: Maus auf die Ecke des ViewCube", [] { setzeMaus(wuerfelOrt(-1, -1, 1)()); }));
    add(pause(0.5));
    add(hilfe2::schrittFoto("h_mbk_cube"));
    add(tu("MBKOPF Handbuch: Blender, Startansicht", [] {
        modellBedienungSetzen(1);
        g_app->modellKnochen = false;
        modellStartansicht(t().aktiv);
    }));
    add(pause(0.5));
    add(klickAn("MBKOPF Handbuch: Overlays", feldOrt(3, 3)));
    add(pause(0.6));
    add(hilfe2::schrittFoto("h_mbk_blender"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tu("MBKOPF Handbuch: zurueck", [=] {
        g_app->settings.splitModel = *teiler;
        modellBedienungSetzen(0);
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    std::vector<Schritt> s;
    if (n == "kopfrot") { return mbkopfrot::schritte(); }
    if (n == "kopf") { kopfSchritte(s); }
    return s;
}

}  // namespace mbkopf
