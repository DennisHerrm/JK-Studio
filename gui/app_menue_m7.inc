// app_menue_m7.inc - Menue-Editor M7: Datapad und Beispielwerte (Punkt 14/11), ui_load (Punkt 13),
// Komponenten (Punkt 3) und Modell-Gizmo (Punkt 12).
// Wird von app_menue.cpp eingebunden (nach app_menue_m6.inc).

namespace {

// --- Punkt 14 und 11: Datapad und Beispielwerte -------------------------------------------------

// M7 Punkt 14: ui.Draw_DataPad(DP_HUD) und DP_* (cgame) mit Beispielwerten - Jedi Academy mit den Strichen
// aus ui/hud.menu, Jedi Outcast und Movie Duels mit ihrem eigenen HUD (zeichneDatapadHudJk2).
// ui/hud.menu und ext_data/weapons.dat einmal je Spieldatenstand (Datapad, Ladebildschirm); das Beispiel
// passend zum Spiel
void datapadDatenLaden() {
    Zustand& z = Z();
    if (z.datapadFuer != z.spiel) {
        z.datapadFuer = z.spiel;
        z.datapadWerte = datapadBeispiel(z.spiel);
    }
    if (!z.datapadGelesen) {
        z.datapadGelesen = true;
        std::string t;
        if (z.daten.lies("ui/hud.menu", t)) {
            LeseOptionen o = leseOptionen("ui/hud.menu");
            o.dialekt = Dialekt::SP;
            z.datapadHud = std::make_unique<Datei>(lies("ui/hud.menu", t, o));
        }
        if (z.daten.lies("ext_data/weapons.dat", t)) { z.waffenIcon = waffenIcons(t, z.spiel); }
    }
}

bool datapadZeichnen(int art, std::vector<OdTeil>& out, const DatapadUmgebung& du) {
    Zustand& z = Z();
    datapadDatenLaden();
    std::vector<Prim> hud;
    if (z.spiel != Spiel::JA) {
        zeichneDatapadHudJk2(z.spiel, z.hudWerte, hud);
    } else if (z.datapadHud) {
        zeichneDatapadHud(*z.datapadHud, z.hudWerte, hud);
    }
    for (const Prim& p : hud) {   // die HUD-Teile sind Bilder (Striche, Ziffern, Rahmen)
        OdTeil t;
        t.art = p.art == PrimArt::Fuellen ? OdTeil::Art::Fuellen : OdTeil::Art::Bild;
        t.x = p.x;
        t.y = p.y;
        t.w = p.w;
        t.h = p.h;
        t.s0 = p.s0;
        t.t0 = p.t0;
        t.s1 = p.s1;
        t.t1 = p.t1;
        t.farbe = p.farbe;
        t.inhalt = p.shader;
        out.push_back(std::move(t));
    }
    DatapadKontext k;
    k.spiel = z.spiel;
    k.se = [texte = du.texte](const std::string& key) { return texte != nullptr ? texte->imSpiel("@" + key) : key; };
    k.breite = [du](const std::string& t, float sc, const std::string& name) {
        const Schrift* f = schriftNamens(name);
        return f != nullptr ? schriftBreite(*f, t, sc, du.kodierung, du.asienGlyphen, du.jk2) : 0;
    };
    k.hoehe = [](float sc, const std::string& name) {
        const Schrift* f = schriftNamens(name);
        return f != nullptr ? schriftHoehe(*f, sc) : 0;
    };
    k.kodierung = du.kodierung;
    k.waffenIcon = z.waffenIcon;
    if (z.spiel == Spiel::MD) {   // CG_DisplayBoxedText(..., 4, ...): Griff 4 der Movie-Duels-Schriften
        const Schrift* f = du.schriftNr ? du.schriftNr(4) : nullptr;
        k.schrift4 = f != nullptr ? f->name : std::string();
        k.bildDa = du.bildDa;
    }
    k.werte = z.datapadWerte;
    zeichneDatapad(art, k, out);
    return true;
}

// M7 Punkt 14: Beispielwerte fuer ownerdraws, die im Spiel Werte des laufenden Spiels zeigen
void zeichneBeispielWerte(const Dokument& d) {
    Zustand& z = Z();
    if (z.beispielPrims == 0 || !ImGui::CollapsingHeader(tr(Str::MeOdBeispiel), ImGuiTreeNodeFlags_DefaultOpen)) { return; }
    ImGui::TextWrapped("%s", tr(Str::MeOdBeispielTip));
    const float breit = ImGui::GetFontSize() * 9;
    if (istLadebild(d.datei) || istVerbindung(d.datei)) { ImGui::Checkbox(tr(Str::MeHudWie), &z.hudWie); }
    if (istLadebild(d.datei)) {   // M7 Punkt 11: CG_DrawInformation
        LadeWerte& l = z.ladeWerte;
        ImGui::SetNextItemWidth(breit);
        eingabe(tr(Str::MeLadeKarte), &l.karte);
        ImGui::SetNextItemWidth(breit);
        eingabe(tr(Str::MeLadeNachricht), &l.nachricht);
        ImGui::SetNextItemWidth(breit);
        ImGui::SliderInt(tr(Str::MeLadeStufe), &l.stufe, 0, 9);
        ImGui::Checkbox(tr(Str::MeLadeErsteKarte), &l.ersteKarte);
    }
    if (istVerbindung(d.datei)) {   // M7 Punkt 11: UI_DrawConnectScreen
        VerbindungsWerte& v = z.verbindungsWerte;
        ImGui::SetNextItemWidth(breit);
        eingabe(tr(Str::MeVerbServer), &v.server);
        ImGui::SetNextItemWidth(breit);
        eingabe(tr(Str::MeLadeKarte), &v.karte);
        static const char* const zustaende[] = {"CA_CONNECTING", "CA_CHALLENGING", "CA_CONNECTED"};
        int zi = std::clamp(v.zustand, 1, 3) - 1;
        ImGui::SetNextItemWidth(breit);
        if (ImGui::Combo(tr(Str::MeVerbZustand), &zi, zustaende, 3)) { v.zustand = zi + 1; }
        ImGui::SetNextItemWidth(breit);
        ImGui::SliderInt(tr(Str::MeVerbPakete), &v.pakete, 0, 20);
        ImGui::SetNextItemWidth(breit);
        eingabe(tr(Str::MeVerbMeldung), &v.meldung);
        ImGui::SetNextItemWidth(breit);
        eingabe("motd", &v.motd);
    }
    if (d.datei.dialekt == Dialekt::SP) {
        datapadDatenLaden();
        DatapadWerte& w = z.datapadWerte;
        const std::vector<std::string> waffen = datapadWaffenNamen(z.spiel);
        int wi = std::clamp(w.waffe, 1, 13) - 1;
        ImGui::SetNextItemWidth(breit);
        if (ImGui::Combo(tr(Str::MeOdWaffe), &wi, [](void* p, int i) { return (*static_cast<const std::vector<std::string>*>(p))[static_cast<std::size_t>(i)].c_str(); },
                         const_cast<std::vector<std::string>*>(&waffen), 13)) {
            w.waffe = wi + 1;
            w.waffen |= 1U << w.waffe;
        }
        const std::vector<std::string> kraefte = datapadKraftNamen(z.spiel);
        const std::vector<int> fp = datapadKraefte(z.spiel);   // showDataPadPowers
        const int n = static_cast<int>(fp.size());
        auto stufeVon = [&]() -> int& { return w.kraft[static_cast<std::size_t>(fp[static_cast<std::size_t>(std::clamp(w.kraftWahl, 0, n - 1))])]; };
        ImGui::SetNextItemWidth(breit);
        if (ImGui::Combo(tr(Str::MeOdKraft), &w.kraftWahl, [](void* p, int i) { return (*static_cast<const std::vector<std::string>*>(p))[static_cast<std::size_t>(i)].c_str(); },
                         const_cast<std::vector<std::string>*>(&kraefte), n)) {
            if (stufeVon() == 0) { stufeVon() = 1; }
        }
        int& stufe = stufeVon();
        ImGui::SetNextItemWidth(breit);
        ImGui::SliderInt(tr(Str::MeOdKraftStufe), &stufe, 1, 3);
        for (std::size_t i = 0; i < w.ziele.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            ImGui::Checkbox(w.ziele[i].name.c_str(), &w.ziele[i].erfuellt);
            ImGui::PopID();
        }
        hudRegler(z.hudWerte);
    } else {
        Spielbeispiel& b = z.spielBeispiel;
        ImGui::Checkbox("g_jediVmerc", &b.jediVmerc);
        ImGui::Checkbox("g_forceBasedTeams", &b.forceBasedTeams);
        ImGui::SetNextItemWidth(breit);
        ImGui::SliderInt(tr(Str::MeOdMaxRang), &b.maxRank, 0, 7);
        ImGui::Checkbox(tr(Str::MeOdServerSuche), &b.serverRefresh);
        if (b.serverRefresh) {
            ImGui::SetNextItemWidth(breit);
            ImGui::SliderInt("LAN_GetServerCount", &b.serverAnzahl, 0, 500);
        }
        if (z.spiel == Spiel::JK2) { eingabe("MOTD", &b.motd); }
    }
}

// --- Punkt 13: lose Dateien und ui_load im laufenden Spiel ---------------------------------------

// M7 Punkt 13: alle Dateien des Pakets lose in den Spielordner des eigenen Testordners (nie ins Spiel)
bool loseDateienSchreiben(const std::string& ordner) {
    std::error_code ec;
    for (const Pk3Datei& f : paketDateien(true)) {
        const fs::path ziel = utf8Pfad(ordner) / utf8Pfad(f.name);
        fs::create_directories(ziel.parent_path(), ec);
        if (!schreibeDatei(u8(ziel), f.inhalt)) { return false; }
    }
    return true;
}

// Nach dem Speichern: laeuft das vom Editor gestartete Spiel noch, die Dateien neu ablegen und ui_load
// in seine Konsole geben (UI_Load: Menus_CloseAll, UI_LoadMenus, das zuletzt offene Menue wieder auf)
void imLaufendenSpielNeuLaden() {
    Zustand& z = Z();
    if (z.testPid == 0 || !z.testUiLoad || z.testOrdnerLose.empty()) { return; }
    if (!platform::prozessLaeuft(z.testPid)) {
        z.testPid = 0;
        return;
    }
    if (loseDateienSchreiben(z.testOrdnerLose) && platform::spielKonsolenBefehl(z.testPid, "ui_load")) {
        ++z.testNeuGeladen;
        z.meldung = tr(Str::MeTestNeuGeladen);
        addStatus(z.meldung);
    }
}

// --- Punkt 3: Komponenten -------------------------------------------------------------------------

// Vor dem Anwenden einer Aenderung: gibt es Kopien, die aus dem neuen Text abgeglichenen Kopien gleich mit
// hineinnehmen - alles als eine Ersetzung, also ein Rueckgaengig-Schritt
std::vector<Ersetzung> mitKopienAbgleich(const Dokument& d, std::vector<Ersetzung> e) {
    const std::string text = d.puffer.text();
    if (text.find(kInstanzKennung) == std::string::npos) { return e; }
    const std::string neu = anwenden(text, e);
    const Datei p = lies(d.name, neu, leseOptionen(d.name));
    const std::vector<Ersetzung> s = kopienAbgleichen(p, neu);
    if (s.empty()) { return e; }
    return alsEineErsetzung(text, anwenden(neu, s));
}

void zeichneKomponente(Dokument& d) {
    Zustand& z = Z();
    const int mi = menuDerSeite(d);
    if (mi < 0 || d.auswahl.size() != 1 || !ImGui::CollapsingHeader(tr(Str::MeKomp), ImGuiTreeNodeFlags_DefaultOpen)) { return; }
    const int item = d.auswahl[0];
    const std::string text = d.puffer.text();
    const Item& it = d.datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(item)];
    std::string name;
    const char rolle = komponentenRolle(d.datei, text, mi, item, name);
    char buf[300];
    if (rolle == 'k') {
        const std::vector<Komponente> alle = komponenten(d.datei, text);
        const Komponente* k = nullptr;
        for (const Komponente& x : alle) {
            if (x.menu == mi && x.original == item) { k = &x; }
        }
        const int kopien = k != nullptr ? static_cast<int>(k->instanzen.size()) : 0;
        std::snprintf(buf, sizeof(buf), trN(Str::MeKompOriginal, kopien), name.c_str(), kopien);
        ImGui::TextWrapped("%s", buf);
        if (k != nullptr && ImGui::Button(tr(Str::MeKompKopie))) {
            // neben das letzte Stueck der Komponente, Name <original>_<n>
            const Rechteck r = it.window.rect;
            const int n = static_cast<int>(k->instanzen.size()) + 2;
            const float versatz = 20.0F * static_cast<float>(n - 1);
            std::vector<Ersetzung> e;
            const std::string kopieName = (it.window.name.empty() ? std::string("kopie") : it.window.name) + "_" + std::to_string(n);
            if (kopieEinfuegen(d.datei, text, *k, r.x + versatz, r.y + versatz, kopieName, e)) { aendere(std::move(e)); }
        }
        tipp(Str::MeKompKopieTip);
    } else if (rolle == 'i') {
        std::snprintf(buf, sizeof(buf), tr(Str::MeKompKopieVon), name.c_str());
        ImGui::TextWrapped("%s", buf);
        ImGui::TextDisabled("%s", tr(Str::MeKompKopieHinweis));
        for (const Komponente& k : komponenten(d.datei, text)) {
            if (k.menu == mi && k.name == name && ImGui::Button(tr(Str::MeKompZumOriginal))) { d.auswahl.assign(1, k.original); }
        }
    } else {
        // je Element neu vorbelegen - auch beim Wechsel der Datei mit gleicher Nummer
        const std::string fuer = d.name + "|" + std::to_string(mi) + "|" + std::to_string(item) + "|" + it.window.name;
        if (z.kompFuer != fuer) {
            z.kompFuer = fuer;
            z.kompName = it.window.name.empty() ? std::string("komponente") : it.window.name;
        }
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10);
        eingabe("##kompname", &z.kompName);
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeKompAnlegen))) {
            std::vector<Ersetzung> e;
            if (alsKomponente(d.datei, text, mi, item, z.kompName, e)) { aendere(std::move(e)); }
            else { z.meldung = tr(Str::MeKompFehler); }
        }
        tipp(Str::MeKompTip);
    }
}

// --- Punkt 12: Modelle per Gizmo ausrichten -------------------------------------------------------
//
// Item_Model_Paint (code/ui/ui_shared.cpp) nimmt die Lage des Modells nicht aus model_origin, sondern aus
// model_g2mins/maxs: Abstand origin[0] = (maxs.z - mins.z) / 2 / 0.268, Hoehe origin[2] = -(mins.z + maxs.z)
// / 2, seitlich origin[1] = (mins.y + maxs.y) / 2; ohne Angabe -16 -16 -24 / 16 16 32. Drehung model_angle,
// Blickwinkel model_fovx/fovy (0 = aus der Breite). Die Griffe aendern genau diese Schluessel; beim
// Loslassen eine Ersetzung (ein Rueckgaengig-Schritt).

GizmoWerte gizmoAus(const Item& it) {
    GizmoWerte g;
    g.angle = it.angle;
    for (int k = 0; k < 3; ++k) {
        g.mins[k] = it.g2mins[k];
        g.maxs[k] = it.g2maxs[k];
    }
    if (g.mins[0] == 0 && g.mins[1] == 0 && g.mins[2] == 0 && g.maxs[0] == 0 && g.maxs[1] == 0 && g.maxs[2] == 0) {
        const float a[3] = {-16, -16, -24}, b[3] = {16, 16, 32};   // die Vorgabe von Item_Model_Paint
        for (int k = 0; k < 3; ++k) {
            g.mins[k] = a[k];
            g.maxs[k] = b[k];
        }
    }
    g.fovx = it.fovx;
    g.fovy = it.fovy;
    return g;
}

// Waehrend des Ziehens: die Werte im gezeichneten Menue (eine Kopie der Datei)
bool gizmoVorschau(const Dokument& d, int mi, Datei& kopie) {
    Zustand& z = Z();
    if (z.gizmoArt == 0 || z.gizmoItem < 0 || mi < 0) { return false; }
    Item& it = kopie.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(z.gizmoItem)];
    it.angle = z.gizmoJetzt.angle;
    for (int k = 0; k < 3; ++k) {
        it.g2mins[k] = z.gizmoJetzt.mins[k];
        it.g2maxs[k] = z.gizmoJetzt.maxs[k];
    }
    it.fovx = z.gizmoJetzt.fovx;
    it.fovy = z.gizmoJetzt.fovy;
    (void)d;
    return true;
}

// Die Griffe am gewaehlten Modell: 1 Drehen (waagrecht ziehen), 2 Abstand (senkrecht), 3 Hoehe (senkrecht),
// 4 Blickwinkel (waagrecht). Rueckgabe true, wenn ein Klick einen Griff traf.
std::array<ImVec2, 4> gizmoGriffe(const Rechteck& r, const Abbildung& ab, ImVec2 o) {
    const ImVec2 a{o.x + ab.x(r.x), o.y + ab.y(r.y)}, b{o.x + ab.x(r.x + r.w), o.y + ab.y(r.y + r.h)};
    const float m = (a.x + b.x) * 0.5F;
    return {ImVec2{m, a.y + 12}, ImVec2{b.x - 12, (a.y + b.y) * 0.5F}, ImVec2{a.x + 12, (a.y + b.y) * 0.5F}, ImVec2{m, b.y - 12}};
}

int gizmoTreffer(const Dokument& d, int mi, const Abbildung& ab, ImVec2 o, ImVec2 maus) {
    if (Z().interaktion || mi < 0 || d.auswahl.size() != 1) { return 0; }
    const Item& it = d.datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(d.auswahl[0])];
    if (it.type != ITEM_TYPE_MODEL) { return 0; }
    const std::array<ImVec2, 4> g = gizmoGriffe(it.window.rect, ab, o);
    for (int i = 0; i < 4; ++i) {
        if ((maus.x - g[static_cast<std::size_t>(i)].x) * (maus.x - g[static_cast<std::size_t>(i)].x) +
                (maus.y - g[static_cast<std::size_t>(i)].y) * (maus.y - g[static_cast<std::size_t>(i)].y) <=
            100.0F) {
            return i + 1;
        }
    }
    return 0;
}

bool gizmoKlick(Dokument& d, int mi, const Abbildung& ab, ImVec2 o, ImVec2 maus) {
    Zustand& z = Z();
    const int art = gizmoTreffer(d, mi, ab, o, maus);
    if (art == 0) { return false; }
    const Item& it = d.datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(d.auswahl[0])];
    z.gizmoArt = art;
    z.gizmoItem = d.auswahl[0];
    z.gizmoStart = gizmoAus(it);
    if (art == 4 && z.gizmoStart.fovx == 0) {   // aus der Breite: refdef.fov_x = (int)(w / 640 * 90), hier bei 640x480
        z.gizmoStart.fovx = static_cast<float>(static_cast<int>(it.window.rect.w / 640.0F * 90.0F));
    }
    z.gizmoJetzt = z.gizmoStart;
    z.gizmoMaus = maus;
    return true;
}

// Zeichnen und Ziehen (jedes Bild); beim Loslassen schreiben
void zeichneModellGizmo(ImDrawList* dl, Dokument& d, int mi, const Abbildung& ab, ImVec2 o) {
    Zustand& z = Z();
    if (z.gizmoArt != 0) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        const float dx = m.x - z.gizmoMaus.x, dy = m.y - z.gizmoMaus.y;
        GizmoWerte g = z.gizmoStart;
        if (z.gizmoArt == 1) {
            g.angle = z.gizmoStart.angle + static_cast<int>(std::lround(dx * 0.5F));
        } else if (z.gizmoArt == 2) {   // Abstand: halbe z-Spanne um die Mitte strecken
            const float f = std::exp(dy * 0.01F);
            const float mitte = 0.5F * (g.mins[2] + g.maxs[2]), halb = 0.5F * (g.maxs[2] - g.mins[2]) * f;
            g.mins[2] = std::round((mitte - halb) * 10.0F) / 10.0F;
            g.maxs[2] = std::round((mitte + halb) * 10.0F) / 10.0F;
        } else if (z.gizmoArt == 3) {   // Hoehe: die z-Spanne verschieben
            const float v = std::round(dy * 0.25F * 10.0F) / 10.0F;
            g.mins[2] += v;
            g.maxs[2] += v;
        } else {   // Blickwinkel
            g.fovx = std::clamp(std::round(z.gizmoStart.fovx + dx * 0.25F), 1.0F, 179.0F);
            if (z.gizmoStart.fovy != 0) { g.fovy = std::clamp(std::round(z.gizmoStart.fovy * g.fovx / z.gizmoStart.fovx), 1.0F, 179.0F); }
        }
        z.gizmoJetzt = g;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const GizmoWerte s = z.gizmoStart;
            std::vector<Ersetzung> e;
            auto zahl = [](float v) { return zahlText(v); };
            const int item = z.gizmoItem;
            bool ok = true;
            if (g.angle != s.angle) { ok = setzeEigenschaft(d.datei, mi, item, "model_angle", {std::to_string(g.angle)}, e) && ok; }
            if (g.mins[2] != s.mins[2] || g.maxs[2] != s.maxs[2]) {
                ok = setzeEigenschaft(d.datei, mi, item, "model_g2mins", {zahl(g.mins[0]), zahl(g.mins[1]), zahl(g.mins[2])}, e) && ok;
                ok = setzeEigenschaft(d.datei, mi, item, "model_g2maxs", {zahl(g.maxs[0]), zahl(g.maxs[1]), zahl(g.maxs[2])}, e) && ok;
            }
            if (g.fovx != s.fovx) { ok = setzeEigenschaft(d.datei, mi, item, "model_fovx", {zahl(g.fovx)}, e) && ok; }
            if (g.fovy != s.fovy) { ok = setzeEigenschaft(d.datei, mi, item, "model_fovy", {zahl(g.fovy)}, e) && ok; }
            if (!ok) { ausMakroMelden(); }
            else if (!e.empty()) { aendere(std::move(e)); }
            z.gizmoArt = 0;
            z.gizmoItem = -1;
        }
    }
    if (z.interaktion || mi < 0 || d.auswahl.size() != 1) { return; }
    const Item& it = d.datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(d.auswahl[0])];
    if (it.type != ITEM_TYPE_MODEL) { return; }
    const std::array<ImVec2, 4> griffe = gizmoGriffe(it.window.rect, ab, o);
    static const char* const zeichen[4] = {"<>", "+-", "^v", "fov"};
    static const Str hilfe[4] = {Str::MeGizmoDrehen, Str::MeGizmoAbstand, Str::MeGizmoHoehe, Str::MeGizmoFov};
    const int ueber = gizmoTreffer(d, mi, ab, o, ImGui::GetIO().MousePos);
    for (int i = 0; i < 4; ++i) {
        const bool aktiv = z.gizmoArt == i + 1 || ueber == i + 1;
        const ImVec2 p = griffe[static_cast<std::size_t>(i)];
        dl->AddCircleFilled(p, 10.0F, aktiv ? IM_COL32(255, 190, 40, 230) : IM_COL32(40, 120, 220, 210));
        const ImVec2 t = ImGui::CalcTextSize(zeichen[i]);
        dl->AddText(ImVec2{p.x - t.x * 0.5F, p.y - t.y * 0.5F}, IM_COL32(255, 255, 255, 255), zeichen[i]);
    }
    if (ueber != 0 || z.gizmoArt != 0) {
        const GizmoWerte g = z.gizmoArt != 0 ? z.gizmoJetzt : gizmoAus(it);
        char b[200];
        std::snprintf(b, sizeof(b), "%s\nmodel_angle %d  model_g2mins z %g  model_g2maxs z %g  model_fovx %g", tr(hilfe[(z.gizmoArt != 0 ? z.gizmoArt : ueber) - 1]),
                      g.angle, static_cast<double>(g.mins[2]), static_cast<double>(g.maxs[2]), static_cast<double>(g.fovx));
        ImGui::SetTooltip("%s", b);
    }
}

}  // namespace

// --- Punkt 14: Platzhalter-Bestand fuer den Selbsttest ----------------------------------------------

// M7 Punkt 14: jedes menuDef aller Original-Menues (ui/**/*.menu) zeichnen wie die Vorschau und die
// ownerdraws/Feeder sammeln, die noch einen beschrifteten Platzhalter statt der Darstellung bekommen
std::vector<std::string> platzhalterImSpiel() {
    datenPruefen();
    Zustand& z = Z();
    std::set<std::string> offen;
    for (const std::string& pfad : z.daten.liste("ui/", {".menu"})) {
        std::string t;
        if (!z.daten.lies(pfad, t)) { continue; }
        Dokument d;
        d.name = pfad;
        d.datei = lies(pfad, t, leseOptionen(pfad));
        const Umgebung u = umgebung(d);
        for (std::size_t m = 0; m < d.datei.menus.size(); ++m) {
            std::vector<Prim> p;
            zeichneMenu(d.datei, static_cast<int>(m), u, p);
            for (const Prim& x : p) {
                if (x.art != PrimArt::Platzhalter) { continue; }
                if (x.shader.compare(0, 10, "ownerdraw ") != 0 && x.shader.compare(0, 7, "feeder ") != 0) { continue; }
                offen.insert(std::string(d.datei.dialekt == Dialekt::MP ? "MP " : "SP ") + x.shader + " (" + pfad + ")");
            }
        }
    }
    return {offen.begin(), offen.end()};
}
