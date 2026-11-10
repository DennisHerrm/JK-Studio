// selbsttest_mbkopf_rot.inc - MBKOPF (10.11.): die Fehler von shank nachgestellt (BHED_EDITORTEST=modellblock,
// BHED_MODELL_NUR=kopfrot). Nur mit dem, was es schon in rc611 gab - derselbe Test war vor der Behebung rot:
//   - "ich kann das hier nicht bedienen?": Klick auf [+], [Ansicht] und [Standard] der Kopfzeile oeffnet ein Menue
//   - "das das model immer von vorne angezeigt wird und nicht von der Seite": nach dem Laden, in den vier Ansichten
//     (Alt+W) und nach dem Zuruecksetzen schaut die Perspektive der Figur ins Gesicht - gemessen am Skelett (die
//     Augenknochen leye/reye liegen vorn am Schaedel cranium), nicht an einer Achse abgelesen - und die Figur steht
//     mittig und ganz im Bild (Rahmen um alle gezeichneten Ecken in der Pose)
//   - "Vertex numbers ... angehakt, im Bild nichts zu sehen" / "die sachen scheinen auch keine funktion zu haben": jeder
//     Anzeige-Schalter aendert das Bild, in Ruhe und Pose, ohne und mit gewaehlter Flaeche (4a)
//   - "dann fehlt mir ein viewcube": ein Klick oben rechts in der Ansicht (die Flaeche Vorne des Wuerfels) dreht sie
// Eingebunden in selbsttest.cpp (nach selbsttest_mr.inc).
namespace mbkopfrot {

ModellAnsicht& ansicht(int n = -1) { return g_app->mtab.ansichten[n < 0 ? g_app->mtab.aktiv : n]; }

bool popupOffen() { return GImGui->OpenPopupStack.Size > 0; }

// Drahtgitter, Kanten, zweiseitig, Raster und weisse Texturen gehoeren seit MBKOPF der einen Ansicht
// (ModellAnsicht::sch), bis rc611 galten sie fuer alle (ModellTab::an, Texturen: App::showModelTextures) - so laeuft
// derselbe Test auf beiden Staenden.
template <typename T>
bool& schDraht(T& t) {
    if constexpr (requires { t.an.drahtgitter; }) { return t.an.drahtgitter; } else { return t.ansichten[t.aktiv].sch.drahtgitter; }
}
template <typename T>
bool& schKanten(T& t) {
    if constexpr (requires { t.an.kanten; }) { return t.an.kanten; } else { return t.ansichten[t.aktiv].sch.kanten; }
}
template <typename T>
bool& schZweiseitig(T& t) {
    if constexpr (requires { t.an.zweiseitig; }) { return t.an.zweiseitig; } else { return t.ansichten[t.aktiv].sch.zweiseitig; }
}
template <typename T>
bool& schRaster(T& t) {
    if constexpr (requires { t.an.raster; }) { return t.an.raster; } else { return t.ansichten[t.aktiv].sch.raster; }
}
template <typename T>
bool& schWeiss(T& t) {
    if constexpr (requires { t.ansichten[0].sch.weiss; }) {
        return t.ansichten[t.aktiv].sch.weiss;
    } else {
        (void)t;
        return g_app->showModelTextures;
    }
}

// Das Gesicht aus dem Skelett der Pose: die Augenknochen liegen vorn am Schaedel. `zurKamera` ist der Kosinus zwischen
// der Strecke Schaedel -> Augenmitte (waagerecht) und der Richtung zur Kamera (1 = die Figur schaut genau in die
// Kamera, 0 = von der Seite, -1 = Hinterkopf); `linksRechts` ist der Schirmabstand linkes minus rechtes Auge (> 0: ihr
// linkes Auge steht im Bild rechts - so sieht man ein Gegenueber, nicht sein Spiegelbild).
bool gesicht(const ModellAnsicht& a, float& zurKamera, float& linksRechts) {
    const int c = boneIndex(g_app->anim, "cranium");
    const int l = boneIndex(g_app->anim, "leye");
    const int r = boneIndex(g_app->anim, "reye");
    const std::vector<BoneMatrix>& w = g_app->modellWelt;
    if (c < 0 || l < 0 || r < 0 || static_cast<std::size_t>(std::max(c, std::max(l, r))) >= w.size()) { return false; }
    float p[3][3];
    const int knochen[3] = {c, l, r};
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) { p[i][k] = w[static_cast<std::size_t>(knochen[i])].m[k][3]; }
    }
    const float g[2] = {(p[1][0] + p[2][0]) * 0.5F - p[0][0], (p[1][1] + p[2][1]) * 0.5F - p[0][1]};
    const float len = std::hypot(g[0], g[1]);
    float rechts[3];
    float hoch[3];
    float blick[3];
    modellAnsichtAchsenVon(a.art, rechts, hoch, blick);
    ImVec2 sl;
    ImVec2 sr;
    if (len < 1e-3F || !modellAufSchirm(a, p[1], sl) || !modellAufSchirm(a, p[2], sr)) { return false; }
    zurKamera = -(g[0] * blick[0] + g[1] * blick[1]) / len;
    linksRechts = sl.x - sr.x;
    return true;
}

// Der Rahmen um alle gezeichneten Ecken der Figur in der Pose, in Anteilen der Ansicht (0 links/oben, 1 rechts/unten).
bool figurRahmen(const ModellAnsicht& a, float r[4]) {
    const auto& h = modellHaut();
    r[0] = r[1] = 1e9F;
    r[2] = r[3] = -1e9F;
    bool da = false;
    for (std::size_t i = 0; i < h.size(); ++i) {
        if (!modellTeilGezeichnet(static_cast<int>(i)) || g_app->mtab.anzeige.surfaces[i].isTag()) { continue; }
        for (const auto& p : h[i]) {
            ImVec2 s;
            if (!modellAufSchirm(a, p.data(), s)) { return false; }
            const float x = (s.x - a.lo[0]) / a.groesse[0];
            const float y = (s.y - a.lo[1]) / a.groesse[1];
            r[0] = std::min(r[0], x);
            r[1] = std::min(r[1], y);
            r[2] = std::max(r[2], x);
            r[3] = std::max(r[3], y);
            da = true;
        }
    }
    return da;
}

Schritt pruefeVorne(const std::string& wo, int n = -1) {
    return pruefe("MBKOPF vorne: " + wo + " - die Kamera schaut der Figur ins Gesicht (Augen vor dem Schaedel zur Kamera, ihr linkes Auge rechts im Bild)",
                  [=] {
                      float zur = 0.0F;
                      float lr = 0.0F;
                      const bool da = gesicht(ansicht(n), zur, lr);
                      char z[200];
                      std::snprintf(z, sizeof z, "Knopftest: MBKOPF vorne %s: Gesicht zur Kamera %.3f, linkes Auge %.1f Punkte rechts vom rechten, Ansicht %d",
                                    wo.c_str(), static_cast<double>(zur), static_cast<double>(lr), static_cast<int>(ansicht(n).art));
                      diag::detail(z);
                      return da && zur > 0.95F && lr > 2.0F;
                  });
}

Schritt pruefeMittig(const std::string& wo, int n = -1) {
    return pruefe("MBKOPF mittig: " + wo + " - die Figur steht mittig und ganz im Bild (Rahmen um alle Ecken: Mitte auf 5 %, groesste Seite ueber die Haelfte)",
                  [=] {
                      float r[4];
                      const bool da = figurRahmen(ansicht(n), r);
                      char z[200];
                      std::snprintf(z, sizeof z, "Knopftest: MBKOPF mittig %s: Rahmen x %.3f..%.3f, y %.3f..%.3f", wo.c_str(), static_cast<double>(r[0]),
                                    static_cast<double>(r[2]), static_cast<double>(r[1]), static_cast<double>(r[3]));
                      diag::detail(z);
                      return da && std::fabs((r[0] + r[2]) * 0.5F - 0.5F) < 0.05F && std::fabs((r[1] + r[3]) * 0.5F - 0.5F) < 0.05F && r[0] >= 0.0F &&
                             r[1] >= 0.0F && r[2] <= 1.0F && r[3] <= 1.0F && std::max(r[2] - r[0], r[3] - r[1]) > 0.5F;
                  });
}

std::vector<unsigned char> foto() {
    std::vector<unsigned char> px;
    int w = 0;
    int h = 0;
    (void)modellFotoMitUeberlagerung(0, px, w, h);
    return px;
}

// Das Foto der aktiven Ansicht (mit Ueberlagerung, 1920 breit) in den Ordner BHED_FOTOS - zum Ansehen.
Schritt fotoAblegen(const std::string& name) {
    return tu("MBKOPF: Foto " + name, [=] {
        if (mb1::fotoOrdner().empty()) { return; }
        std::vector<unsigned char> px;
        int w = 0;
        int h = 0;
        if (modellFotoMitUeberlagerung(1920, px, w, h)) { (void)modellBmp(mb1::fotoOrdner() + "\\" + name + ".bmp", w, h, px); }
    });
}

void poseSetzen(bool pose) {
    if (pose) {
        int a = mb1::animNach("BOTH_ATTACK1");
        if (a < 0) { a = std::min(1, static_cast<int>(g_app->animList.size()) - 1); }
        g_app->animIndex = a;
        const AnimEntry& e = g_app->animList[static_cast<std::size_t>(std::max(a, 0))];
        g_app->animFrame = e.firstFrame + e.numFrames / 2;
    } else {
        g_app->animIndex = -1;
        g_app->animFrame = 0;
    }
    g_app->animPlaying = false;
    g_app->animTime = 0.0F;
    g_app->modelDirty = true;
}

// 4a: jeder Anzeige-Schalter an/aus aendert das Foto der aktiven Ansicht (mit Ueberlagerung). Die Statistik (7) stand bis
// rc611 nur auf dem Schirm, nicht im Foto - sie prueft `kopf` ueber den Katalog der Kopfzeile.
struct Schalter {
    const char* name;
    std::function<bool&()> wert;
    int art;   // 0 immer, 1 nur an einer Auswahl (die Klammer umschliesst sie), 2 von innen (Rueckseiten), 3 an den Knochen
};

const std::vector<Schalter>& schalter() {
    static const std::vector<Schalter> kListe = {
        {"Drahtgitter", [] () -> bool& { return schDraht(g_app->mtab); }, 0},
        {"Kanten ueber Flaechen", [] () -> bool& { return schKanten(g_app->mtab); }, 0},
        {"Zweiseitig", [] () -> bool& { return schZweiseitig(g_app->mtab); }, 2},
        {"Weisse Texturen", [] () -> bool& { return schWeiss(g_app->mtab); }, 0},
        {"Raster", [] () -> bool& { return schRaster(g_app->mtab); }, 0},
        {"Gewichte", [] () -> bool& { return g_app->mtab.an.gewichte; }, 0},
        {"Silhouette", [] () -> bool& { return g_app->mtab.silhouette; }, 0},
        {"Boden", [] () -> bool& { return g_app->mtab.an.boden; }, 0},
        {"Ursprung", [] () -> bool& { return g_app->mtab.an.ursprung; }, 0},
        {"Huellkasten", [] () -> bool& { return g_app->mtab.an.huelle; }, 0},
        {"Auswahlklammer", [] () -> bool& { return g_app->mtab.an.klammer; }, 1},
        {"Hoehenlineal", [] () -> bool& { return g_app->mtab.an.lineal; }, 0},
        {"Knochen", [] () -> bool& { return g_app->modellKnochen; }, 0},
        {"Knochenflossen", [] () -> bool& { return g_app->settings.modellFlossen; }, 3},
        {"Normalen", [] () -> bool& { return g_app->mtab.an.normalen; }, 0},
        {"Tag-Flaechen", [] () -> bool& { return g_app->mtab.an.tags; }, 0},
        {"Riss-Finder", [] () -> bool& { return g_app->mtab.an.risse; }, 0},
        {"Ecken-Nummern", [] () -> bool& { return g_app->mtab.an.eckNummern; }, 0},
        {"Dreiecks-Nummern", [] () -> bool& { return g_app->mtab.an.dreieckNummern; }, 0},
    };
    return kListe;
}

bool schalterWirken(const std::string& wo, bool mitAuswahl) {
    ModellTab& t = g_app->mtab;
    bool ok = true;
    t.auswahl.clear();
    if (mitAuswahl) { t.auswahl = {mb1::teil("torso")}; }
    std::string ohne;
    for (const Schalter& s : schalter()) {
        if (s.art == 1 && !mitAuswahl) { continue; }
        const bool knochenVorher = g_app->modellKnochen;
        if (s.art == 3) { g_app->modellKnochen = true; }
        const Camera kamera = g_app->modelCam;
        if (s.art == 2) {   // Rueckseiten sieht man nur von innen
            for (int k = 0; k < 3; ++k) { g_app->modelCam.pos[k] = (g_app->model.mins[k] + g_app->model.maxs[k]) * 0.5F; }
        }
        bool& w = s.wert();
        const bool vorher = w;
        const std::vector<unsigned char> a = foto();
        w = !vorher;
        const std::vector<unsigned char> b = foto();
        w = vorher;
        g_app->modelCam = kamera;
        g_app->modellKnochen = knochenVorher;
        const int d = mb1::unterschied(a, b);
        if (d <= 20) {
            ohne += std::string(ohne.empty() ? "" : ", ") + s.name + " (" + std::to_string(d) + ")";
            ok = false;
        }
    }
    t.auswahl.clear();
    g_app->modelDirty = true;
    diag::detail("Knopftest: MBKOPF 4a " + wo + (mitAuswahl ? ", torso gewaehlt" : ", nichts gewaehlt") + " - ohne Wirkung: " + (ohne.empty() ? "keiner" : ohne));
    return ok;
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("MBKOPF rot: Skript fuer den Test, Bedienung wie 3ds Max", [] {
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mbk.txt");
        modellBedienungSetzen(0);
    }));
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    add(pause(0.5));
    // --- B6: von vorne und mittig - beim Laden, in den vier Ansichten, nach dem Zuruecksetzen ----------------------
    add(pruefe("MBKOPF rot: nach dem Laden ist die aktive Ansicht die Perspektive", [] { return ansicht().art == AnsichtArt::Perspektive; }));
    add(pruefeVorne("nach dem Laden (Sturmtruppler)"));
    add(pruefeMittig("nach dem Laden (Sturmtruppler)"));
    add(fotoAblegen("mbk_start_stormtrooper"));
    add(tu("MBKOPF rot: vier Ansichten (Alt+W)", [] {
        g_app->mtab.vierAnsichten = true;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(pruefeVorne("vier Ansichten (Alt+W), die Perspektive", 3));
    add(pruefeMittig("vier Ansichten (Alt+W), die Perspektive", 3));
    add(tu("MBKOPF rot: wieder eine Ansicht", [] {
        g_app->mtab.vierAnsichten = false;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(pruefeVorne("zurueck aus den vier Ansichten"));
    add(pruefeMittig("zurueck aus den vier Ansichten"));
    add(tu("MBKOPF rot: Ansicht verdreht und verschoben, dann zurueckgesetzt", [] {
        g_app->modelOrbit.turn(g_app->modelCam, 70.0F, 20.0F);
        g_app->modelOrbit.pan(g_app->modelCam, 40.0F, 25.0F);
        modellAnsichtEinpassen();
    }));
    add(pause(0.4));
    add(pruefeVorne("nach dem Zuruecksetzen"));
    add(pruefeMittig("nach dem Zuruecksetzen"));
    // --- die Kopfzeile: [+] links oben, [Ansicht] und [Standard] daneben -------------------------------------------
    add(klickAn("MBKOPF rot: auf [+] der Kopfzeile", [] {
        const ModellAnsicht& a = ansicht();
        return ImVec2{a.lo[0] + 6.0F + ImGui::CalcTextSize("[+]").x * 0.5F, a.lo[1] + 4.0F + ImGui::GetFontSize() * 0.5F};
    }));
    add(pause(0.2));
    add(pruefe("MBKOPF rot: [+] oeffnet ein Menue", [] { return popupOffen(); }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(klickAn("MBKOPF rot: auf [Perspektive] der Kopfzeile", [] {
        const ModellAnsicht& a = ansicht();
        return ImVec2{a.lo[0] + 25.0F + ImGui::CalcTextSize("[+]").x, a.lo[1] + 4.0F + ImGui::GetFontSize() * 0.5F};
    }));
    add(pause(0.2));
    add(pruefe("MBKOPF rot: [Perspektive] oeffnet ein Menue", [] { return popupOffen(); }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(klickAn("MBKOPF rot: auf [Standard] der Kopfzeile", [] {
        const ModellAnsicht& a = ansicht();
        const float w0 = ImGui::CalcTextSize("[+]").x;
        const float w1 = ImGui::CalcTextSize((std::string("[") + tr(Str::MbAnsichtPersp) + "]").c_str()).x;
        return ImVec2{a.lo[0] + 23.0F + w0 + w1 + 12.0F, a.lo[1] + 4.0F + ImGui::GetFontSize() * 0.5F};
    }, 1));
    add(pause(0.2));
    // bis rc611 oeffnete der Rechtsklick dort das Rechtsklick-Menue der Ansicht (die Kopfzeile war nur Text)
    add(pruefe("MBKOPF rot: [Standard] oeffnet mit Rechtsklick sein eigenes Menue wie in 3ds Max, nicht das Rechtsklick-Menue der Ansicht",
               [] { return popupOffen() && !g_app->mtab.kontextSichtbar; }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(pruefe("MBKOPF rot: Esc schliesst das Menue wieder", [] {
        diag::detail("Knopftest: MBKOPF rot nach Esc - offene Popups " + std::to_string(GImGui->OpenPopupStack.Size));
        return !popupOffen();
    }));
    // --- 4a: jeder Schalter wirkt ----------------------------------------------------------------------------------
    for (const bool pose : {false, true}) {
        const std::string wo = pose ? "Pose (BOTH_ATTACK1)" : "Ruhe";
        add(tu("MBKOPF rot 4a: " + wo, [=] {
            g_app->mtab.bereich = ModellBereich::Ansehen;
            g_app->mtab.auswahl.clear();
            poseSetzen(pose);
        }));
        add(pause(0.4));
        add(pruefe("MBKOPF rot 4a: jeder Anzeige-Schalter aendert das Bild - " + wo + ", nichts gewaehlt", [=] { return schalterWirken(wo, false); }));
        add(pruefe("MBKOPF rot 4a: jeder Anzeige-Schalter aendert das Bild - " + wo + ", torso gewaehlt", [=] { return schalterWirken(wo, true); }));
    }
    add(tu("MBKOPF rot 4a: Ruhe, nichts gewaehlt", [] {
        poseSetzen(false);
        g_app->mtab.auswahl.clear();
    }));
    add(pause(0.3));
    // --- der ViewCube oben rechts (Masse wie max_viewcube.h: Kasten 130 breit, 6 vom Rand, Wuerfel 78 nach 26 Rand, bei
    // 16er Schrift): in der Startansicht zeigt er seine Flaeche Vorne, ihr rechter Randstreifen ist die Kante
    // Vorne/Rechts (die Mitte des Wuerfels plus 0,39 Kantenlaengen) -------------------------------------------------
    add(tu("MBKOPF rot: zurueckgesetzt (von vorne)", [] {
        modellAnsichtEinpassen();
        const ImGuiContext& g = *GImGui;
        diag::detail("Knopftest: MBKOPF rot vor dem ViewCube - offene Popups " + std::to_string(g.OpenPopupStack.Size) + ", aktives Element " +
                     std::to_string(g.ActiveId) + ", Fokus " + (g.NavWindow != nullptr ? g.NavWindow->Name : "-"));
    }));
    add(pause(0.4));
    add(klickAn("MBKOPF rot: oben rechts in der Ansicht (ViewCube, Kante Vorne/Rechts)", [] {
        const ModellAnsicht& a = ansicht();
        const float m = std::min(ImGui::GetFontSize() / 16.0F, std::min(a.groesse[0], a.groesse[1]) * 0.5F / 140.0F);
        const ImVec2 mitte{a.lo[0] + a.groesse[0] - 6.0F * m - 65.0F * m, a.lo[1] + 6.0F * m + 26.0F * m + 39.0F * m};
        const ImVec2 p{mitte.x + 0.39F * 39.0F * m / 0.92F, mitte.y};
        char z[160];
        std::snprintf(z, sizeof z, "Knopftest: MBKOPF rot ViewCube Klick bei %.1f/%.1f, Ansicht %.0f/%.0f %.0fx%.0f, Schrift %.1f", static_cast<double>(p.x),
                      static_cast<double>(p.y), static_cast<double>(a.lo[0]), static_cast<double>(a.lo[1]), static_cast<double>(a.groesse[0]),
                      static_cast<double>(a.groesse[1]), static_cast<double>(ImGui::GetFontSize()));
        diag::detail(z);
        return p;
    }));
    add(pause(0.12));
    add(tu("MBKOPF rot: kurz nach dem Klick", [] {
        diag::detail("Knopftest: MBKOPF rot ViewCube kurz nach dem Klick Gier " + std::to_string(g_app->modelCam.angles[1]));
    }));
    add(pause(0.6));
    add(pruefe("MBKOPF rot: der Klick auf die Kante des ViewCube dreht die Perspektive dorthin (Blick von vorne rechts: -X +Y)", [] {
        float v[3];
        g_app->modelCam.forward(v);
        char z[160];
        std::snprintf(z, sizeof z, "Knopftest: MBKOPF rot ViewCube Blick %.3f %.3f %.3f, Ansicht %d", static_cast<double>(v[0]), static_cast<double>(v[1]),
                      static_cast<double>(v[2]), static_cast<int>(ansicht().art));
        diag::detail(z);
        return ansicht().art == AnsichtArt::Perspektive && (-v[0] + v[1]) * 0.70710678F > 0.999F;
    }));
    // --- Perspektive <-> Orthographisch (Blender Ziffernblock 5, der Knopf des Gizmos): von vorne bleibt vorne ------
    add(tu("MBKOPF rot: Perspektive mit Blick auf das Gesicht (Gier 90), dann Perspektive/Orthographisch umgeschaltet", [] {
        g_app->mtab.auswahl.clear();
        g_app->modelOrbit.turn(g_app->modelCam, 90.0F - g_app->modelCam.angles[1], -g_app->modelCam.angles[0]);
        modellPerspektiveUmschalten();
    }));
    add(pause(0.3));
    add(pruefe("MBKOPF rot: Perspektive/Orthographisch von vorne gibt die Achsenansicht Vorne", [] { return ansicht().art == AnsichtArt::Vorne; }));
    add(pruefeVorne("Perspektive/Orthographisch umgeschaltet"));
    add(tu("MBKOPF rot: zurueck zur Perspektive", [] { modellPerspektiveUmschalten(); }));
    add(pause(0.2));
    // --- B6 gilt fuer jede Figur: Kyle (shanks Bild) ------------------------------------------------------------------
    for (Schritt& x : mb1::laden("models/players/kyle/model.glm")) { add(std::move(x)); }
    add(pause(0.5));
    add(pruefeVorne("nach dem Laden (Kyle)"));
    add(pruefeMittig("nach dem Laden (Kyle)"));
    add(fotoAblegen("mbk_start_kyle"));
    add(tu("MBKOPF rot: zurueck", [] {
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

}  // namespace mbkopfrot
