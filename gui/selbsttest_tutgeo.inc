// selbsttest_tutgeo.inc - rc610 TUT-GEO: die Tour "Ein Modell zu Brushes machen" (k-geometrie)
//
// BHED_EDITORTEST=karteneditor, BHED_KARTE_NUR=tutgeo (auch im Selbsttest hilfe2, BHED_H2_NUR=geo);
// die Bilder des Tutorials (h_tut_geo_1 .. _6) nimmt BHED_EDITORTEST=handbuch BHED_HB_NUR=tutgeo auf.
// Wie die Touren der Stufe H2: echte Aktionen (Klicks, Tippen, Auswahllisten), nach jeder Aktion muss die
// Tour selbst weitergeschaltet haben - geprueft wird die Wirkung am Programmzustand. Der Dateidialog von
// File > Import geometry as brushes... wuerde die Nachrichtenschleife anhalten: dort ruft der Test die
// Funktion dahinter (geoImportLaden) mit dem Beispielmodell auf.
//
// Das Beispielmodell schreibt der Test selbst in den Einstellungsordner des Laufs (keine fremden Dateien),
// wie ein Blender-Export mit Y oben: eine Kiste (48 Einheiten, crate01), eine Saeule mit Sockel (ein
// geschlossenes, nicht konvexes Netz, pillar_base01) und eine offene Pflanzenwand (temple_vines2). Mit dem
// Vorsatz textures/yavin/ sind es Texturen von Jedi Academy (assets1.pk3). Geprueft wird, was das Tutorial
// verspricht: Vorschau je Teil, ein Undo-Schritt, Optimierer, Material und caulk, detail, misc_model mit
// Clip-Brushes, die .obj im Import-Ordner.
namespace tutgeo {

namespace ke = karteneditor;
using stufe3::konsoleText;

std::string beispielPfad() { return platform::settingsDirectory() + "\\beispiel_geo.obj"; }

using Punkt = std::array<double, 3>;

// Eine .obj wie aus Blender (Y oben): Vielecke gegen den Uhrzeigersinn von aussen, in Q3-Koordinaten
// (Z oben) angegeben und als (x, z, -y) geschrieben - der Import rechnet mit "Y oben" zurueck.
struct ObjSchreiber {
    std::string text = "# JK Studio self-test: example model for the tutorial k-geometrie\nmtllib beispiel_geo.mtl\n";
    int punkte = 0;
    void objekt(const std::string& name, const std::string& material) { text += "o " + name + "\nusemtl " + material + "\n"; }
    void flaeche(const std::vector<Punkt>& p, const std::vector<std::array<double, 2>>& st) {
        std::string f = "f";
        for (std::size_t i = 0; i < p.size(); ++i) {
            char z[200];
            std::snprintf(z, sizeof(z), "v %g %g %g\nvt %g %g\n", p[i][0], p[i][2], -p[i][1], st[i][0], st[i][1]);
            text += z;
            ++punkte;
            f += " " + std::to_string(punkte) + "/" + std::to_string(punkte);
        }
        text += f + "\n";
    }
    // UV planar auf die Hauptachse der Flaeche, eine Textur je 64 Einheiten (t nach oben wie in Blender)
    void flaecheProjiziert(const std::vector<Punkt>& p) {
        const Punkt a{p[1][0] - p[0][0], p[1][1] - p[0][1], p[1][2] - p[0][2]};
        const Punkt b{p[2][0] - p[0][0], p[2][1] - p[0][1], p[2][2] - p[0][2]};
        const Punkt n{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
        std::vector<std::array<double, 2>> st;
        for (const Punkt& q : p) {
            if (std::fabs(n[2]) >= std::fabs(n[0]) && std::fabs(n[2]) >= std::fabs(n[1])) {
                st.push_back({q[0] / 64.0, q[1] / 64.0});
            } else if (std::fabs(n[0]) >= std::fabs(n[1])) {
                st.push_back({q[1] / 64.0, q[2] / 64.0});
            } else {
                st.push_back({q[0] / 64.0, q[2] / 64.0});
            }
        }
        flaeche(p, st);
    }
};

void beispielSchreiben() {
    ObjSchreiber o;
    const std::vector<std::array<double, 2>> ganz{{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    // Kiste: 48 Einheiten (kleiner als 64 -> detail), jede Seite einmal die Textur
    {
        const double x0 = -120, x1 = -72, y0 = -24, y1 = 24, z0 = 0, z1 = 48;
        o.objekt("Kiste", "crate01");
        o.flaeche({{x0, y1, z0}, {x1, y1, z0}, {x1, y0, z0}, {x0, y0, z0}}, ganz);   // unten
        o.flaeche({{x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}}, ganz);   // oben
        o.flaeche({{x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1}, {x0, y0, z1}}, ganz);   // vorn (-y)
        o.flaeche({{x1, y1, z0}, {x0, y1, z0}, {x0, y1, z1}, {x1, y1, z1}}, ganz);   // hinten (+y)
        o.flaeche({{x0, y1, z0}, {x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}}, ganz);   // links (-x)
        o.flaeche({{x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}, {x1, y0, z1}}, ganz);   // rechts (+x)
    }
    // Saeule: Sockel 64 x 64 x 16, darauf ein Achteck (Radius 20) bis 176 - ein geschlossenes Netz
    {
        constexpr double kPi = 3.14159265358979323846;
        const double h = 32, sz = 16, oben = 176, r = 20;
        std::array<Punkt, 4> boden{}, ecke{};   // Ecken bei 45, 135, 225, 315 Grad
        std::array<Punkt, 8> ring{}, kopf{};    // Achteck bei 22.5 + 45 k Grad
        for (int j = 0; j < 4; ++j) {
            const double w = (45.0 + 90.0 * j) * kPi / 180.0;
            const double x = std::cos(w) > 0 ? h : -h, y = std::sin(w) > 0 ? h : -h;
            boden[static_cast<std::size_t>(j)] = {x, y, 0};
            ecke[static_cast<std::size_t>(j)] = {x, y, sz};
        }
        for (int k = 0; k < 8; ++k) {
            const double w = (22.5 + 45.0 * k) * kPi / 180.0;
            ring[static_cast<std::size_t>(k)] = {r * std::cos(w), r * std::sin(w), sz};
            kopf[static_cast<std::size_t>(k)] = {r * std::cos(w), r * std::sin(w), oben};
        }
        const auto E = [&](int j) { return ecke[static_cast<std::size_t>((j + 4) % 4)]; };
        const auto B = [&](int j) { return boden[static_cast<std::size_t>((j + 4) % 4)]; };
        const auto R = [&](int k) { return ring[static_cast<std::size_t>((k + 8) % 8)]; };
        const auto K = [&](int k) { return kopf[static_cast<std::size_t>((k + 8) % 8)]; };
        o.objekt("Saeule", "pillar_base01");
        o.flaecheProjiziert({B(3), B(2), B(1), B(0)});                                   // Boden
        for (int j = 0; j < 4; ++j) { o.flaecheProjiziert({B(j - 1), B(j), E(j), E(j - 1)}); }   // Sockelseiten
        for (int j = 0; j < 4; ++j) {
            o.flaecheProjiziert({E(j - 1), E(j), R(2 * j), R(2 * j - 1)});   // Sockel oben: Trapez zur Seite j
            o.flaecheProjiziert({R(2 * j), E(j), R(2 * j + 1)});             // und das Dreieck an der Ecke j
        }
        for (int k = 0; k < 8; ++k) { o.flaecheProjiziert({R(k), R(k + 1), K(k + 1), K(k)}); }   // Schaft
        o.flaecheProjiziert({K(0), K(1), K(2), K(3), K(4), K(5), K(6), K(7)});                   // Deckel
    }
    // Pflanzenwand: eine offene Flaeche (nur die Vorderseite), 64 x 96
    o.objekt("Ranke", "temple_vines2");
    o.flaeche({{72, 0, 0}, {136, 0, 0}, {136, 0, 96}, {72, 0, 96}}, ganz);
    std::ofstream(::bhed::utf8Pfad(beispielPfad()), std::ios::binary) << o.text;
}

// Alle Flaechen eines Brushes mit diesem Shader?
bool ganzMit(const std::shared_ptr<karte::Pinsel>& p, const std::string& shader) {
    for (const auto& f : p->flaechen()) {
        if (f->shader() != shader) { return false; }
    }
    return !p->flaechen().empty();
}

std::vector<std::shared_ptr<karte::Pinsel>> weltPinsel() {
    std::vector<std::shared_ptr<karte::Pinsel>> v;
    if (const auto w = ke::editor().karte.welt()) {
        for (const auto& p : w->kinder()) {
            if (p->art() == karte::Primitiv::Art::Pinsel) { v.push_back(std::static_pointer_cast<karte::Pinsel>(p)); }
        }
    }
    return v;
}

// Die Tour mit echten Aktionen; mit BHED_FOTOS je Schritt ein Bild (h_tut_geo_<n>).
std::vector<Schritt> tour() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    using hilfe2::erledigt;
    using hilfe2::schrittFoto;
    using hilfe2::wartetNoch;
    using hilfe2::zielDa;
    const std::string fenster = "###geoimport";
    add(tu("TUT-GEO: Beispielmodell (.obj wie aus Blender, Y oben) in den Einstellungsordner des Laufs, Karten-Editor", [] {
        beispielSchreiben();
        g_app->leftMode = 3;
    }));
    add(warteBis("TUT-GEO: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(stufe2::grundzustand("TUT-GEO: Grundzustand (Regular, Raster 8, Werkzeug Resize)"));
    add(tu("TUT-GEO: File > New Map, Import wie vorgegeben (Massstab 1, Y oben, Vorsatz textures/import/, Optimierer an)", [] {
        karteDialogeZu();
        (void)ke::ausfuehren("NewMap");
        ke::Editor& e = ke::editor();
        e.geoImport = ke::GeoImportZustand{};
        e.kameraE.modus = karte::KameraModus::Textur;
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.xy[0].ansicht.setzeUrsprung(karte::Vektor3f(8, 0, 0));
        e.xy[0].ansicht.setzeMassstab(1.0F);
        e.kamera.setzeUrsprung(karte::Vektor3f(8, -300, 150));
        e.kamera.setzeWinkel(karte::Vektor3f(-16, 90, 0));
        ++e.sichtStand;
    }));
    add(tu("TUT-GEO: Modus Events, Tour k-geometrie starten", [] {
        g_app->leftMode = 0;
        (void)tourStarten("k-geometrie", 0);
    }));
    add(pause(1.0));   // der Moduswechsel schwingt ein Bild lang ein
    add(pruefe("TUT-GEO: Tour laeuft mit 6 Schritten", [] {
        const TourSicht t = tourSicht();
        return t.aktiv && t.id == "k-geometrie" && t.anzahl == 6;
    }));
    add(zielDa("TUT-GEO: der Knopf Karten-Editor ist hervorgehoben"));
    add(pause(0.6));
    add(schrittFoto("h_tut_geo_1"));
    // 1 -> 2: in den Karten-Editor
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(erledigt("k-geometrie", 1));
    add(zielDa("TUT-GEO: das Menue File des Karten-Editors ist hervorgehoben"));
    add(klick(ke::trK(Str::KeLFile), "###main"));
    add(pause(0.4));
    add(zielDa("TUT-GEO: aufgeklappt zeigt die Tour auf Import geometry as brushes..."));
    add(pause(0.6));
    add(schrittFoto("h_tut_geo_2"));
    add(taste(ImGuiKey_Escape, "Esc (Menue zu)"));
    add(wartetNoch("k-geometrie", 1));
    // Der Eintrag oeffnet einen Dateidialog - der Test ruft die Funktion dahinter mit der Datei auf.
    add(tu("TUT-GEO: File > Import geometry as brushes... mit beispiel_geo.obj (statt des Dateidialogs geoImportLaden)", [] {
        if (!ke::geoImportLaden(beispielPfad())) { meldeFehler("TUT-GEO: beispiel_geo.obj nicht lesbar"); }
    }));
    add(erledigt("k-geometrie", 2));
    add(pruefe("TUT-GEO: Fenster offen, drei Teile Kiste, Saeule, Ranke; Kiste und Saeule geschlossen, die Ranke offen", [] {
        const ke::GeoImportZustand& z = ke::editor().geoImport;
        std::string namen;
        for (const auto& t : z.netz.teile) { namen += t.name + " "; }
        diag::detail("Knopftest: TUT-GEO - Teile " + namen);
        return z.offen && z.netz.teile.size() == 3 && z.vorschau.size() == 3 && z.netz.teile[0].name == "Kiste" &&
               z.netz.teile[1].name == "Saeule" && z.netz.teile[2].name == "Ranke" && z.vorschau[0].geschlossen && z.vorschau[1].geschlossen &&
               !z.vorschau[2].geschlossen;
    }));
    // 3: Shader-Vorsatz tippen, die Ranke einmal als Brushes ansehen, zurueck auf Auto
    add(tu("TUT-GEO: das Fenster (Groesse wie vorgegeben) nach links, neben das Tutorial", [fenster] {
        if (ImGuiWindow* w = ImGui::FindWindowByName(fenster.c_str())) {
            const ImVec2 d = ImGui::GetIO().DisplaySize;
            ImGui::SetWindowPos(w, ImVec2{8.0F, std::max(0.0F, std::min(d.y * 0.18F, d.y - w->Size.y - 8.0F))});
        }
    }));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeGiVorsatz)) + "##givorsatz", fenster));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_A, "Strg+A (Vorsatz waehlen)"));
    add(tippe("textures/yavin/"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.3));
    add(pruefe("TUT-GEO: Vorschau mit textures/yavin/ - Kiste 1 Brush detail (crate01), Saeule geschlossen: 2 strukturelle Brushes (Sockel, Schaft), "
               "Ranke offen: Auto = Modell + Clip (detail)", [] {
        const ke::GeoImportZustand& z = ke::editor().geoImport;
        if (z.vorschau.size() != 3) { return false; }
        const karte::TeilErgebnis& k = z.vorschau[0];
        const karte::TeilErgebnis& p = z.vorschau[1];
        const karte::TeilErgebnis& r = z.vorschau[2];
        bool kisteShader = !k.pinsel.empty();
        for (const auto& b : k.pinsel) {
            for (const auto& sd : b.seiten) { kisteShader = kisteShader && sd.shader == "textures/yavin/crate01"; }
        }
        diag::detail("Knopftest: TUT-GEO - Vorsatz " + std::string(z.vorsatz) + ", Kiste " + std::to_string(k.pinsel.size()) + (k.detail ? " detail" : " strukturell") +
                     ", Saeule " + std::to_string(p.pinsel.size()) + (p.detail ? " detail" : " strukturell") + ", Ranke " + std::to_string(r.pinsel.size()) +
                     (r.art == karte::TeilArt::ModellClip ? " Modell+Clip" : " Brushes"));
        return std::string(z.vorsatz) == "textures/yavin/" && k.art == karte::TeilArt::Pinsel && k.pinsel.size() == 1 && k.detail && kisteShader &&
               p.art == karte::TeilArt::Pinsel && p.pinsel.size() == 2 && !p.detail && r.art == karte::TeilArt::ModellClip && !r.pinsel.empty() && r.detail;
    }));
    add(pruefe("TUT-GEO: die Klappliste der dritten Zeile liegt ganz in der Tabelle (nicht unter ihrem Rand)", [fenster] {
        const ImRect tab = hb::fensterRect("##giteile");
        const ImVec2 o = stufe2::klappOrt("##giart2")();
        diag::detail("Knopftest: TUT-GEO - Tabelle bis y " + std::to_string(static_cast<int>(tab.Max.y)) + ", Klappliste Ranke y " + std::to_string(static_cast<int>(o.y)));
        return tab.GetHeight() > 0.0F && o.y > tab.Min.y && o.y + ImGui::GetFrameHeight() * 0.5F <= tab.Max.y;
    }));
    add(klickAn("TUT-GEO: Klappliste Als der Ranke", stufe2::klappOrt("##giart2")));
    add(pause(0.3));
    add(klick(ke::trK(Str::KeGiPinsel), "##Combo"));
    add(pause(0.3));
    add(pruefe("TUT-GEO: Ranke als Brushes - die offene Flaeche wird ein duenner Detail-Brush mit temple_vines2 vorn", [] {
        const ke::GeoImportZustand& z = ke::editor().geoImport;
        if (z.vorschau.size() != 3 || z.vorschau[2].art != karte::TeilArt::Pinsel || z.vorschau[2].pinsel.size() != 1) { return false; }
        bool vorn = false;
        for (const auto& sd : z.vorschau[2].pinsel[0].seiten) { vorn = vorn || sd.shader == "textures/yavin/temple_vines2"; }
        return z.vorschau[2].detail && z.vorschau[2].pinsel[0].detail && vorn;
    }));
    add(klickAn("TUT-GEO: Klappliste Als der Ranke", stufe2::klappOrt("##giart2")));
    add(pause(0.3));
    add(klick(ke::trK(Str::KeGiAuto), "##Combo"));
    add(pause(0.3));
    add(pruefe("TUT-GEO: Ranke wieder Auto (Modell + Clip)", [] {
        const ke::GeoImportZustand& z = ke::editor().geoImport;
        return z.wahl.size() == 3 && z.wahl[2] == static_cast<int>(karte::TeilArt::Auto) && z.vorschau[2].art == karte::TeilArt::ModellClip;
    }));
    add(pause(0.6));
    add(schrittFoto("h_tut_geo_3"));
    add(wartetNoch("k-geometrie", 2));
    add(klick(tr(Str::TourNext), "###tour"));
    add(erledigt("k-geometrie", 3));
    // 4: Einfuegen
    auto vorher = std::make_shared<std::size_t>(0);
    add(zielDa("TUT-GEO: der Knopf Insert ist hervorgehoben"));
    add(tu("TUT-GEO: Zahl der Brushes vorher", [vorher] { *vorher = weltPinsel().size(); }));
    add(pause(0.6));
    add(schrittFoto("h_tut_geo_4"));
    add(klick(std::string(ke::trK(Str::KeGiEinfuegen)) + "##gieinfuegen", fenster));
    add(erledigt("k-geometrie", 4));
    add(pause(0.4));
    add(pruefe("TUT-GEO: alles eingefuegt - so viele Brushes wie die Vorschau, ein misc_model auf models/import/beispiel_geo.obj im Import-Ordner, "
               "Optimierer offen, Meldung in der Konsole", [vorher] {
        const ke::Editor& ed = ke::editor();
        std::size_t erwartet = 0;
        for (const auto& t : ed.geoImport.vorschau) { erwartet += t.pinsel.size(); }
        int modelle = 0;
        for (const auto& en : ed.karte.entitaeten()) {
            modelle += (en->wert("classname") == "misc_model" && en->wert("model") == "models/import/beispiel_geo.obj") ? 1 : 0;
        }
        std::error_code ec;
        const bool datei = std::filesystem::is_regular_file(
            std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>((platform::settingsDirectory() + "\\import\\models\\import\\beispiel_geo.obj").c_str()))), ec);
        char fertig[1400];   // die Meldung in der Sprache des Programms
        std::snprintf(fertig, sizeof(fertig), ke::trKN(Str::KeGiFertig, erwartet, std::size_t{1}), ed.geoImport.datei.c_str(), static_cast<int>(erwartet), 1);
        diag::detail("Knopftest: TUT-GEO - eingefuegt " + std::to_string(weltPinsel().size() - *vorher) + " Brushes (Vorschau " + std::to_string(erwartet) +
                     "), misc_model " + std::to_string(modelle) + ", Datei " + std::to_string(datei ? 1 : 0));
        return !ed.geoImport.offen && ed.geoImport.letztes.pinsel == erwartet && ed.geoImport.letztes.modelle == 1 && weltPinsel().size() == *vorher + erwartet &&
               modelle == 1 && datei && ke::optimiererOffen() && konsoleText().find(fertig) != std::string::npos;
    }));
    // 5: das Ergebnis ansehen
    add(tu("TUT-GEO: Optimierer zu (er hat die Karte geprueft)", [] { ke::optimiererSchliessen(); }));
    add(warteBis("TUT-GEO: Texturen geladen", [] { return ke::kameraBilderFertig(); }, 6000));
    add(pruefe("TUT-GEO: Kiste ein Detail-Brush ganz crate01; Saeule strukturell mit pillar_base01 und caulk innen; Clip-Brushes der Ranke detail", [] {
        int kiste = 0, saeule = 0, clip = 0, caulk = 0;
        bool saeuleStruktur = true, clipDetail = true;
        for (const auto& p : weltPinsel()) {
            if (ganzMit(p, "textures/yavin/crate01")) { kiste += p->istDetail() ? 1 : 0; }
            if (ganzMit(p, "textures/system/clip")) {
                ++clip;
                clipDetail = clipDetail && p->istDetail();
            }
            bool stein = false;
            for (const auto& f : p->flaechen()) {
                stein = stein || f->shader() == "textures/yavin/pillar_base01";
                caulk += f->shader() == "textures/system/caulk" ? 1 : 0;
            }
            if (stein) {
                ++saeule;
                saeuleStruktur = saeuleStruktur && !p->istDetail();
            }
        }
        diag::detail("Knopftest: TUT-GEO - Kiste " + std::to_string(kiste) + ", Saeule " + std::to_string(saeule) + ", Clip " + std::to_string(clip) +
                     ", caulk-Seiten " + std::to_string(caulk));
        return kiste == 1 && saeule == 2 && saeuleStruktur && clip >= 1 && clipDetail && caulk > 0;
    }));
    add(taste(ImGuiKey_Escape, "Esc (Auswahl weg, die Texturen statt der Auswahlfarbe)"));
    add(warteBis("TUT-GEO: nichts mehr gewaehlt", [] { return ke::editor().karte.auswahl.anzahl() == 0; }, 120));
    add(pause(1.0));
    add(schrittFoto("h_tut_geo_5"));
    add(klick(tr(Str::TourNext), "###tour"));
    add(erledigt("k-geometrie", 5));
    // 6: Bauen und im Spiel ansehen - hier nicht gebaut (das macht hilfe1), nur die Hervorhebung
    add(zielDa("TUT-GEO: das Menue Build ist hervorgehoben"));
    add(pause(0.6));
    add(schrittFoto("h_tut_geo_6"));
    add(pruefe("TUT-GEO: Schritt 6 (bauen) steht, solange nicht gebaut ist", [] {
        const TourSicht t = tourSicht();
        return t.schritt == 5 && !t.erfuellt && !t.fertig;
    }));
    add(klick(tr(Str::TourSkip), "###tour"));
    add(pruefe("TUT-GEO: Tour ganz durch", [] { return tourSicht().fertig; }));
    add(tu("TUT-GEO: Tour zu; ein Undo (Strg+Z)", [] {
        tourSchliessen();
        g_app->leftMode = 3;
    }));
    add(pause(0.3));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(pruefe("TUT-GEO: ein Undo nimmt alle Brushes und das misc_model zurueck", [vorher] {
        bool modell = false;
        for (const auto& en : ke::editor().karte.entitaeten()) { modell = modell || en->wert("classname") == "misc_model"; }
        return weltPinsel().size() == *vorher && !modell;
    }));
    add(tu("TUT-GEO: Ende - Import wie vorgegeben, neue Karte, Events", [] {
        ke::editor().geoImport = ke::GeoImportZustand{};
        (void)ke::ausfuehren("NewMap");
        g_app->leftMode = 0;
    }));
    add(pause(0.3));
    return s;
}

}  // namespace tutgeo
