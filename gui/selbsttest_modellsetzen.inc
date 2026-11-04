// selbsttest_modellsetzen.inc - rc609 M0: vorhandene Modelle mit einem Klick in die Karte (BHED_EDITORTEST=modellsetzen)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor).
//
// Mission-Tab (Karte BHED_M0_KARTE, Vorgabe md_am_sith, ohne Quelle):
//   1. Browser findet models/map_objects/factory/s_crate_01.md3 (Suche im Fenster "Modelle")
//   2. Eintrag anklicken: das Modell steht als Geist unter der Maus; Klick setzt es als misc_model_static, die
//      Unterkante (R_ModelBounds) auf dem Boden der Spielkollision; die Vorschau zeigt es (mapModels)
//   3. Gizmo: Skalieren gleichmaessig (modelscale), entlang der Hochachse (modelscale_vec), Drehen am Gierring (angle);
//      SP, MD9 und q3map2 lesen dieselbe Lage
//   4. .ent: misc_model_static mit denselben Schluesseln
//   5. Live: neuere Modelldatei (lose, im Laufordner) - neu geladen, Lage bleibt, kein Rueckgaengig-Schritt
//   Je Aktion genau ein Rueckgaengig-Schritt.
// Karten-Editor (Testraum, Boden z 0, mit Quelle):
//   6. Asset-Browser findet das Modell; Ziehen auf die Kamera setzt misc_model (Kollision: spawnflags 2) auf den Boden
//   7. misc_model_static mit Clip-Brush in einem Rueckgaengig-Schritt
//   8. Drehen und Skalieren (rotateSelected/scaleSelected wie der Gizmo): Schluessel und Matrix wie Spiel und q3map2
//   9. Bau (q3map2 -meta): Flaechen des Modells in der .bsp, misc_model_static im Entity-Lump, Kollision fest
//  10. Live: neuere Datei im Import-Ordner - der Editor laedt neu, die Schluessel bleiben
namespace m0 {

namespace ke = karteneditor;

const char* const kModell = "models/map_objects/factory/s_crate_01.md3";
const char* const kLive = "models/m0test/kiste.md3";

std::string kl(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}
std::string v3(const float v[3]) {
    char t[96];
    std::snprintf(t, sizeof(t), "%.3f %.3f %.3f", static_cast<double>(v[0]), static_cast<double>(v[1]), static_cast<double>(v[2]));
    return t;
}
std::string lageText(const modellsetzen::Lage& l) { return "(" + v3(l.winkel) + " | " + v3(l.skala) + ")"; }
bool gleich(const modellsetzen::Lage& a, const modellsetzen::Lage& b) {
    for (int i = 0; i < 3; ++i) {
        const float d = std::fmod(std::fabs(a.winkel[i] - b.winkel[i]), 360.0F);
        if (std::min(d, 360.0F - d) > 1e-3F || std::fabs(a.skala[i] - b.skala[i]) > 1e-3F) { return false; }
    }
    return true;
}
const std::string* wert(const MapEntity& e, const char* k) { return e.find(k); }
int schluesselZahl(const MapEntity& e, const char* a, const char* b) { return (e.find(a) != nullptr ? 1 : 0) + (e.find(b) != nullptr ? 1 : 0); }

// Das md3 des Spiels: Bytes und Grenzen von Bild 0.
struct Datei {
    std::string bytes;
    Md3Model m;
};
Datei& crate() {
    static Datei d;
    if (d.bytes.empty() && readFromArchives(kModell, d.bytes)) { (void)readMd3(d.bytes, d.m); }
    return d;
}
// Dasselbe md3 doppelt so gross: Ecken (short * 64) und Bild-Grenzen mal 2 - eine "neuere Fassung" gleicher Groesse.
std::string verdoppelt(std::string b) {
    const auto i32 = [&b](std::size_t at) {
        std::int32_t v = 0;
        std::memcpy(&v, b.data() + at, 4);
        return v;
    };
    const std::int32_t numFrames = i32(76);
    const std::int32_t numSurfaces = i32(84);
    const std::int32_t ofsFrames = i32(92);
    for (std::int32_t f = 0; f < numFrames; ++f) {
        for (int k = 0; k < 6; ++k) {
            const std::size_t at = static_cast<std::size_t>(ofsFrames) + static_cast<std::size_t>(f) * 56 + static_cast<std::size_t>(k) * 4;
            float v = 0;
            std::memcpy(&v, b.data() + at, 4);
            v *= 2.0F;
            std::memcpy(b.data() + at, &v, 4);
        }
    }
    std::size_t s = static_cast<std::size_t>(i32(100));
    for (std::int32_t n = 0; n < numSurfaces && s + 108 <= b.size(); ++n) {
        const std::int32_t sFrames = i32(s + 72);
        const std::int32_t sVerts = i32(s + 80);
        const std::int32_t ofsXyz = i32(s + 100);
        const std::int32_t ofsEnd = i32(s + 104);
        for (std::int32_t v = 0; v < sFrames * sVerts; ++v) {
            for (int k = 0; k < 3; ++k) {
                const std::size_t at = s + static_cast<std::size_t>(ofsXyz) + static_cast<std::size_t>(v) * 8 + static_cast<std::size_t>(k) * 2;
                std::int16_t q = 0;
                std::memcpy(&q, b.data() + at, 2);
                q = static_cast<std::int16_t>(std::clamp(q * 2, -32768, 32767));
                std::memcpy(b.data() + at, &q, 2);
            }
        }
        s += static_cast<std::size_t>(ofsEnd);
    }
    return b;
}
void schreib(const std::filesystem::path& p, const std::string& inhalt) {
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f.write(inhalt.data(), static_cast<std::streamsize>(inhalt.size()));
}
std::filesystem::path spielOrdner() { return std::filesystem::current_path() / "m0spiel"; }
std::filesystem::path importDatei() { return std::filesystem::path(platform::settingsDirectory()) / "import" / "models" / "m0test" / "kiste.md3"; }

// Boden der Spielkollision unter (x, y) ab z.
bool bodenZ(float x, float y, float z, float& aus) {
    const auto k = geladeneKollision(g_app->map.path);
    if (!k) { return false; }
    const float a[3] = {x, y, z};
    const float b[3] = {x, y, z - 8192.0F};
    const float null[3] = {0, 0, 0};
    const KollisionsSpur s = kollisionsSpur(*k, a, null, null, b, 1U);
    if (s.startsolid || s.fraction >= 1.0F) { return false; }
    aus = modellsetzen::bodenHoehe(s.endpos[2], s.normale[2]);   // die Ebene, nicht der Halt 0,125 davor
    return true;
}

// Maus an einen berechneten Punkt (ohne Klick), danach ein paar Bilder warten.
Schritt mausAn(const std::string& was, std::function<ImVec2()> ort) {
    return {"Maus " + was, [=](int b) {
                if (b == 0) { setzeMaus(ort()); }
                return b >= 6;
            }};
}
// Mit der linken Taste von a nach b ziehen (beide erst zur Laufzeit berechnet).
Schritt ziehe(const std::string& was, std::function<std::pair<ImVec2, ImVec2>()> strecke) {
    auto s = std::make_shared<std::pair<ImVec2, ImVec2>>();
    return {"Ziehe " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    *s = strecke();
                    setzeMaus(s->first);
                    return false;
                }
                if (b == 2) { io.AddMouseButtonEvent(0, true); return false; }
                if (b >= 3 && b <= 14) {
                    const float f = static_cast<float>(b - 2) / 12.0F;
                    setzeMaus(ImVec2{s->first.x + (s->second.x - s->first.x) * f, s->first.y + (s->second.y - s->first.y) * f});
                    return false;
                }
                if (b == 15) { io.AddMouseButtonEvent(0, false); return false; }
                return b >= 20;
            }};
}

struct Stand {
    float ziel[3]{};          // Punkt auf dem Boden, auf den geklickt wird
    int neu = -1;             // gesetzte Entity
    std::size_t undo = 0;     // Rueckgaengig-Stapel vor der Aktion
    std::size_t anzahl = 0;   // Entities vor der Aktion
    modellsetzen::Lage vorLive;
    std::string vorOrt;
    float liveMx = 0.0F;      // Grenze x vor dem Neuladen
};

int gesetzte() {
    const auto& v = g_app->map.entities;
    for (std::size_t i = v.size(); i-- > 0;) {
        if (v[i].classname == "misc_model_static") { return static_cast<int>(i); }
    }
    return -1;
}

std::vector<Schritt> mission() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const char* const kk = std::getenv("BHED_M0_KARTE");
    const std::string karte = (kk != nullptr && *kk != 0) ? kk : "md_am_sith";
    auto st = std::make_shared<Stand>();
    add(tu("M0: Karte " + karte + ", Import-Datei fuer den Karten-Editor", [=] {
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        mission1::ungespeicherteEntitiesVergessen();
        if (!loadMapFromArchive("maps/" + karte + ".bsp")) { meldeFehler("M0: maps/" + karte + ".bsp nicht geladen"); }
        schreib(importDatei(), crate().bytes);
    }));
    add(warteBis("M0: Karte geladen", [=] { return g_app->map.path == "maps/" + karte + ".bsp" && !g_app->geo.empty() && g_app->camTrackValid; }, 6000));
    add(tu("M0: Kamera schraeg auf den Boden vor dem Spielerstart", [=] {
        g_app->throughCamera = false;
        g_app->playing = false;
        g_app->rasterSchritt = 0;
        float o[3] = {0, 0, 0};
        for (const MapEntity& e : g_app->map.entities) {
            if (e.classname == "info_player_start" && entityOrt(e, o)) { break; }
        }
        float z = 0.0F;
        if (!bodenZ(o[0] + 96.0F, o[1], o[2] + 32.0F, z)) { meldeFehler("M0: kein Boden vor dem Spielerstart"); }
        st->ziel[0] = o[0] + 96.0F;
        st->ziel[1] = o[1];
        st->ziel[2] = z;
        g_app->cam.pos[0] = st->ziel[0] - 200.0F;
        g_app->cam.pos[1] = st->ziel[1] - 60.0F;
        g_app->cam.pos[2] = st->ziel[2] + 150.0F;
        aimAngles(g_app->cam.pos, st->ziel, g_app->cam.angles);
        g_app->mapDirty = true;
        diag::detail("Knopftest: M0 - Ziel auf dem Boden " + v3(st->ziel));
    }));
    add(pause(0.5));
    // --- 1. Browser --------------------------------------------------------------------------------------
    add(pruefe("M0 Browser: findet models/map_objects/factory/s_crate_01.md3 (Suche \"factory crate_01\")", [] {
        const auto& l = modellBrowserListe();
        const auto t = modellBrowserTreffer("factory crate_01");
        bool da = false;
        for (const std::size_t i : t) { da = da || kl(l[i].pfad) == kModell; }
        diag::detail("Knopftest: M0 Browser - " + std::to_string(l.size()) + " .md3, " + std::to_string(t.size()) + " Treffer");
        return da && l.size() > 100;
    }));
    add(tu("M0 Browser: Fenster auf, Suche", [] {
        g_app->modellFensterOffen = true;
        std::snprintf(g_app->modellSuche, sizeof(g_app->modellSuche), "%s", "factory s_crate_01.md3");
    }));
    add(pause(0.5));
    add(klickAn("Eintrag s_crate_01.md3", stufe2::elementOrt(std::string(kModell).substr(std::string("models/").size()))));
    add(pruefe("M0 Browser: Klick waehlt das Modell zum Setzen", [] { return g_app->modellPlatzieren == kModell; }));
    const auto zielSchirm = [st] {
        ImVec2 p{0, 0};
        (void)kartenPunktAufSchirm(st->ziel, p);
        return p;
    };
    // --- 2. Geist und Setzen ---------------------------------------------------------------------------------
    add(tu("M0 Geist: Zaehler", [] { g_app->modellGeistGezeichnet = 0; }));
    add(mausAn("auf den Boden", zielSchirm));
    add(pause(0.3));
    add(pruefe("M0 Geist: das Modell steht unter der Maus, Unterkante auf dem Boden", [st] {
        const App::ModellGeist& g = g_app->modellGeist;
        const Datei& d = crate();
        diag::detail("Knopftest: M0 Geist - " + std::string(g.aktiv ? "aktiv" : "aus") + " bei " + v3(g.ort) + ", Grenzen z " +
                     std::to_string(d.m.grenzen[0][2]) + ".." + std::to_string(d.m.grenzen[1][2]) + ", " +
                     std::to_string(g_app->modellGeistGezeichnet) + "x gezeichnet");
        float z = 0.0F;
        return g.aktiv && g.pfad == kModell && g_app->modellGeistGezeichnet > 0 && d.m.kopfGrenzen && bodenZ(g.ort[0], g.ort[1], g.ort[2] + 64.0F, z) &&
               std::fabs(g.ort[2] + d.m.grenzen[0][2] - z) < 0.01F;
    }));
    add(foto("m0_geist"));
    add(tu("M0 Setzen: Stand vorher", [st] {
        st->undo = g_app->entUndo.size();
        st->anzahl = g_app->map.entities.size();
    }));
    add(klickAn("auf den Boden: setzen", zielSchirm));
    add(pruefe("M0 Setzen: misc_model_static, Unterkante (R_ModelBounds) auf dem Boden der Spielkollision, ein Schritt", [st] {
        st->neu = g_app->map.entities.size() == st->anzahl + 1 ? gesetzte() : -1;
        if (st->neu != static_cast<int>(st->anzahl)) {
            st->neu = -1;
            return false;
        }
        if (st->neu < 0) { return false; }
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(st->neu)];
        float o[3] = {0, 0, 0};
        (void)std::sscanf(e.origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
        float z = 0.0F;
        const bool boden = bodenZ(o[0], o[1], o[2] + 64.0F, z);
        const TraceTreffer sicht = traceRay(g_app->geo, std::array<float, 3>{o[0], o[1], o[2] + 64.0F}.data(),
                                            std::array<float, 3>{o[0], o[1], o[2] - 512.0F}.data());
        const float unten = o[2] + crate().m.grenzen[0][2];
        diag::detail("Knopftest: M0 Setzen - origin " + e.origin + ", Unterkante " + std::to_string(unten) + ", Boden (Kollision) " + std::to_string(z) +
                     ", Boden (Bild) " + std::to_string(sicht.point[2]) + ", Schritte " + std::to_string(g_app->entUndo.size() - st->undo));
        return e.find("model") != nullptr && *e.find("model") == kModell && boden && std::fabs(unten - z) < 0.01F && sicht.hit &&
               std::fabs(sicht.point[2] - z) < 1.0F && g_app->entUndo.size() == st->undo + 1 && g_app->pickedEntity == st->neu &&
               std::hypot(o[0] - st->ziel[0], o[1] - st->ziel[1]) < 24.0F;
    }));
    add(warteBis("M0 Setzen: Vorschau neu aufgebaut", [] { return g_app->camTrackValid; }, 600));
    add(pause(0.3));
    add(pruefe("M0 Vorschau: das Modell steht in mapModels an seinem Ursprung", [st] {
        if (st->neu < 0) { return false; }
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(st->neu)];
        float o[3] = {0, 0, 0};
        (void)std::sscanf(e.origin.c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
        for (const App::MapModel& m : g_app->mapModels) {
            if (m.entity == static_cast<std::size_t>(st->neu)) {
                return m.path == kModell && std::fabs(m.origin[0] - o[0]) < 0.01F && std::fabs(m.origin[2] - o[2]) < 0.01F;
            }
        }
        return false;
    }));
    add(foto("m0_gesetzt"));
    // --- 3. Gizmo ------------------------------------------------------------------------------------------------
    add(tu("M0 Gizmo: Stand vorher, Fenster Modelle zu", [st] {
        st->undo = g_app->entUndo.size();
        g_app->modellFensterOffen = false;
    }));
    add(klick(tr(Str::M0GizmoSkalieren)));
    add(pruefe("M0 Gizmo: Skalieren an (nur fuer Kartenmodelle in der Leiste)", [] { return g_app->entGizmoSkalieren; }));
    add(ziehe("Quadrat nach oben (doppelt)", [] {
        ImVec2 a{0, 0};
        (void)entityGizmoGriffOrt(10, a);
        return std::make_pair(a, ImVec2{a.x, a.y - 70.0F * ImGui::GetFontSize() / 16.0F});
    }));
    add(pruefe("M0 Gizmo gleichmaessig: \"modelscale\" \"2\", kein modelscale_vec, SP = MD9 = q3map2, ein Schritt", [st] {
        if (st->neu < 0) { return false; }
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(st->neu)];
        const modellsetzen::Lage sp = modellsetzen::lageWieSpiel(e.keys, false);
        diag::detail("Knopftest: M0 Gizmo - modelscale " + std::string(e.find("modelscale") ? *e.find("modelscale") : "-") + ", Lage " + lageText(sp));
        return wert(e, "modelscale") != nullptr && *wert(e, "modelscale") == "2" && wert(e, "modelscale_vec") == nullptr &&
               gleich(sp, modellsetzen::lageWieQ3map2(e.keys)) && gleich(sp, modellsetzen::lageWieSpiel(e.keys, true)) &&
               g_app->entUndo.size() == st->undo + 1;
    }));
    add(ziehe("Hochachse um die Haelfte", [] {
        ImVec2 a{0, 0};
        ImVec2 o{0, 0};
        (void)entityGizmoGriffOrt(9, a);
        (void)entityGizmoGriffOrt(10, o);
        return std::make_pair(a, ImVec2{a.x + (a.x - o.x) * 0.5F / 0.9F, a.y + (a.y - o.y) * 0.5F / 0.9F});
    }));
    add(pruefe("M0 Gizmo Achse: \"modelscale_vec\" \"2 2 ~3\", kein modelscale, SP = q3map2, ein Schritt", [st] {
        if (st->neu < 0) { return false; }
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(st->neu)];
        const modellsetzen::Lage sp = modellsetzen::lageWieSpiel(e.keys, false);
        diag::detail("Knopftest: M0 Gizmo - modelscale_vec " + std::string(e.find("modelscale_vec") ? *e.find("modelscale_vec") : "-") + ", Lage " + lageText(sp));
        return wert(e, "modelscale_vec") != nullptr && wert(e, "modelscale") == nullptr && std::fabs(sp.skala[0] - 2.0F) < 1e-3F &&
               std::fabs(sp.skala[1] - 2.0F) < 1e-3F && sp.skala[2] > 2.8F && sp.skala[2] < 3.2F && gleich(sp, modellsetzen::lageWieQ3map2(e.keys)) &&
               g_app->entUndo.size() == st->undo + 2;
    }));
    add(klick(tr(Str::GizmoRotate)));
    add(ziehe("Gierring um 90 Grad", [] {
        ImVec2 a{0, 0};
        ImVec2 o{0, 0};
        (void)entityGizmoGriffOrt(4, a);
        (void)entityGizmoGriffOrt(10, o);
        return std::make_pair(a, ImVec2{o.x - (a.y - o.y), o.y + (a.x - o.x)});
    }));
    add(pruefe("M0 Gizmo Drehen: 90 Grad Gier in genau einem Winkelschluessel, SP = MD9 = q3map2, ein Schritt", [st] {
        if (st->neu < 0) { return false; }
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(st->neu)];
        const modellsetzen::Lage sp = modellsetzen::lageWieSpiel(e.keys, false);
        const float d = std::fmod(std::fabs(sp.winkel[1]), 360.0F);
        diag::detail("Knopftest: M0 Gizmo - angle " + std::string(e.find("angle") ? *e.find("angle") : "-") + ", angles " +
                     std::string(e.find("angles") ? *e.find("angles") : "-") + ", Lage " + lageText(sp));
        return schluesselZahl(e, "angle", "angles") == 1 && std::fabs(std::min(d, 360.0F - d) - 90.0F) <= 1.0F &&
               gleich(sp, modellsetzen::lageWieQ3map2(e.keys)) && gleich(sp, modellsetzen::lageWieSpiel(e.keys, true)) &&
               g_app->entUndo.size() == st->undo + 3;
    }));
    add(foto("m0_gizmo"));
    add(klick(tr(Str::GizmoMove)));
    // --- 4. .ent ------------------------------------------------------------------------------------------------
    add(tu("M0 .ent: in den Laufordner speichern (Entities speichern)", [] {
        g_app->entHerkunft = DateiHerkunft::lose((std::filesystem::current_path() / "m0test.ent").string());
        g_app->entHerkunftKarte = g_app->map.path;
        (void)entitiesSpeichern();
    }));
    add(pruefe("M0 .ent: misc_model_static mit model, origin, modelscale_vec und Winkel wie in der Vorschau", [st] {
        if (st->neu < 0) { return false; }
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(st->neu)];
        std::vector<MapEntity> v;
        (void)parseEntities(slurp((std::filesystem::current_path() / "m0test.ent").string()), v);
        for (const MapEntity& f : v) {
            if (f.classname == "misc_model_static" && f.origin == e.origin) {
                return f.find("model") != nullptr && *f.find("model") == kModell && gleich(modellsetzen::lageWieSpiel(f.keys, false), modellsetzen::lageWieSpiel(e.keys, false));
            }
        }
        return false;
    }));
    // --- Spielvergleich (nur mit BHED_M0_KAMERA "x y z nicken gier"): die Vorschau so, wie das Spiel durch dieselbe
    // Kamera sieht (cg_fov 80 auf 4:3: CalcFov 64,3727 Grad senkrecht), ohne Bearbeitungshilfen, Ansicht BHED_M0_ANSICHT.
    if (const char* k = std::getenv("BHED_M0_KAMERA")) {
        float kam[5] = {0, 0, 0, 0, 0};
        (void)std::sscanf(k, "%f %f %f %f %f", &kam[0], &kam[1], &kam[2], &kam[3], &kam[4]);
        if (const char* f = std::getenv("BHED_M0_ANSICHT")) {
            int b = 0;
            int h = 0;
            if (std::sscanf(f, "%dx%d", &b, &h) == 2) {
                for (int runde = 0; runde < 8; ++runde) {   // die Seitenleisten wachsen mit dem Fenster (wie geschuetzVergleich)
                    add(tu("M0 Spielvergleich: Ansicht " + std::to_string(runde), [=] {
                        const std::pair<int, int> jetzt = platform::hauptfensterGroesse(0, 0);
                        (void)platform::hauptfensterGroesse(jetzt.first + b - std::max(1, g_app->mapViewW), jetzt.second + h - std::max(1, g_app->mapViewH));
                    }));
                    add(pause(0.5));
                }
            }
        }
        add(tu("M0 Spielvergleich: Kamera wie im Spiel", [=] {
            entityAuswahlSetzen(-1);
            g_app->modellFensterOffen = false;
            for (int i = 0; i < 3; ++i) { g_app->cam.pos[i] = kam[i]; }
            g_app->cam.angles[0] = kam[3];
            g_app->cam.angles[1] = kam[4];
            g_app->cam.angles[2] = 0.0F;
            g_app->cam.fovY = 64.3727F;
            g_app->throughCamera = false;
            g_app->followCam = false;
            g_app->showEntities = false;   // keine Marken und kein Gizmo im Bild
            g_app->mapDirty = true;
        }));
        add(pause(1.5));
        add(tu("M0 Spielvergleich: Ansicht und Ursprung ins Protokoll", [st] {
            char z[300];
            std::snprintf(z, sizeof(z), "Knopftest: M0 Spielvergleich - Ansicht %d x %d bei %.0f %.0f (Fenster), Modell %s", g_app->mapViewW, g_app->mapViewH,
                          static_cast<double>(g_app->kartenAnsicht[0]), static_cast<double>(g_app->kartenAnsicht[1]),
                          st->neu >= 0 ? g_app->map.entities[static_cast<std::size_t>(st->neu)].origin.c_str() : "-");
            diag::detail(z);
        }));
        add(foto("m0_vorschau_spiel"));
        add(tu("M0 Spielvergleich: zurueck", [] { g_app->showEntities = true; }));
    }
    // --- 5. Live ------------------------------------------------------------------------------------------------
    add(tu("M0 Live: loser Spielordner im Laufordner mit models/m0test/kiste.md3, Modell der Entity darauf", [st] {
        schreib(spielOrdner() / "models" / "m0test" / "kiste.md3", crate().bytes);
        GamePath gp;
        gp.directory = spielOrdner().string();
        g_app->gamePaths.push_back(gp);
        if (st->neu < 0) { return; }
        std::vector<MapEntity> vorher = g_app->map.entities;
        setKey(g_app->map.entities[static_cast<std::size_t>(st->neu)], "model", kLive);
        entitiesGeaendert(std::move(vorher));
    }));
    add(pause(2.5));
    add(tu("M0 Live: Stand vorher, neuere Fassung (doppelt so gross) schreiben", [st] {
        if (st->neu < 0) { return; }
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(st->neu)];
        st->vorLive = modellLageVon(e);
        st->vorOrt = e.origin;
        st->undo = g_app->entUndo.size();
        const auto it = g_app->md3Files.find(kLive);
        st->liveMx = it != g_app->md3Files.end() ? it->second.grenzen[1][0] : 0.0F;
        schreib(spielOrdner() / "models" / "m0test" / "kiste.md3", verdoppelt(crate().bytes));
    }));
    add(pause(2.5));
    add(pruefe("M0 Live: neu geladen (Fassung 1, Grenzen doppelt), Lage und Ursprung gleich, kein Rueckgaengig-Schritt", [st] {
        if (st->neu < 0) { return false; }
        const MapEntity& e = g_app->map.entities[static_cast<std::size_t>(st->neu)];
        const auto it = g_app->md3Files.find(kLive);
        bool vorschau = false;
        for (const App::MapModel& m : g_app->mapModels) {
            if (m.entity == static_cast<std::size_t>(st->neu)) {
                vorschau = m.path == kLive && std::fabs(m.scale[2] - st->vorLive.skala[2]) < 1e-3F && std::fabs(m.angles[1] - st->vorLive.winkel[1]) < 1e-3F;
            }
        }
        const float mx = it != g_app->md3Files.end() ? it->second.grenzen[1][0] : 0.0F;
        diag::detail("Knopftest: M0 Live - Fassung " + std::to_string(modellFassungVon(kLive)) + ", Grenze x " + std::to_string(st->liveMx) + " -> " +
                     std::to_string(mx) + ", origin " + e.origin);
        return modellFassungVon(kLive) == 1 && st->liveMx > 0.0F && std::fabs(mx - 2.0F * st->liveMx) < 0.01F && e.origin == st->vorOrt &&
               gleich(modellLageVon(e), st->vorLive) && g_app->entUndo.size() == st->undo && vorschau;
    }));
    add(tu("M0: Spielordner zurueck, Fenster zu", [] {
        std::erase_if(g_app->gamePaths, [](const GamePath& g) { return g.directory == spielOrdner().string(); });
        g_app->modellFensterOffen = false;
        g_app->modellPlatzieren.clear();
        g_app->entGizmoSkalieren = false;
    }));
    return s;
}

// --- Karten-Editor -----------------------------------------------------------------------------------------------

std::shared_ptr<karte::Entitaet> letzte(const char* klasse) {
    std::shared_ptr<karte::Entitaet> r;
    for (const auto& e : ke::editor().karte.entitaeten()) {
        if (e->klassenname() == klasse) { r = e; }
    }
    return r;
}
float ursprungZ(const karte::Entitaet& e) {
    float o[3] = {0, 0, 0};
    (void)std::sscanf(e.wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
    return o[2];
}
modellsetzen::Schluessel schluessel(const karte::Entitaet& e) {
    modellsetzen::Schluessel k;
    for (const char* n : {"angle", "angles", "modelscale", "modelscale_vec"}) {
        if (!e.wert(n).empty()) { k.emplace_back(n, e.wert(n)); }
    }
    return k;
}
ImVec2 kameraPunkt(float dx) {
    const ke::KameraFenster& f = ke::editor().kamFenster;
    return ImVec2{f.bildX + static_cast<float>(f.breite) * 0.5F + dx, f.bildY + static_cast<float>(f.hoehe) * 0.5F};
}
float modellUnten() {
    const karte::ModellAnsicht a = ke::kartenModellPfad(kModell);
    return a.modell ? a.modell->grenzen.origin[2] - a.modell->grenzen.extents[2] : 0.0F;
}

std::vector<Schritt> editor() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("M0 Editor: Spielordner, Modus Events, Import-Datei (vor dem Laden der Spieldaten)", [] {
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        g_app->leftMode = 0;
        schreib(importDatei(), crate().bytes);
    }));
    add(pause(0.3));
    add(klick(tr(Str::ViewMapEditor), "###main"));
    add(warteBis("M0 Editor: Spieldaten geladen", [] {
        ke::spielPruefen();
        return ke::spielBereit();
    }, 20000));
    add(tu("M0 Editor: Testraum (Boden z 0), Blick von oben auf freien Boden, Asset-Browser mit Suche", [] {
        karteDialogeZu();
        stufe3::q3map2Bereitstellen();
        (void)stufe3::raumKarteOeffnen("m0raum", false);
        ke::Editor& e = ke::editor();
        e.kamera.ursprung = karte::Vektor3f(-120, -150, 200);   // frei: die Mittelwand (x -8..8) hat oben eine Bruecke bei y -64..64
        e.kamera.winkel = karte::Vektor3f(-89.0F, 0.0F, 0.0F);
        if (!ke::assetBrowserOffen()) { ke::assetBrowserUmschalten(); }
        ke::assetSucheSetzen("factory s_crate_01.md3", 0);
        ke::modellSetzKlasse() = 0;
        ke::modellSetzKollision() = true;
    }));
    add(pause(1.0));
    add(pruefe("M0 Editor: Asset-Browser findet das Modell", [] {
        for (const karte::Asset& a : ke::assetListe(0)) {
            if (kl(a.pfad) == kModell) { return true; }
        }
        return false;
    }));
    auto undo0 = std::make_shared<std::size_t>(0);
    add(tu("M0 Editor: Stand vorher", [undo0] { *undo0 = ke::editor().karte.undo.undoAnzahl(); }));
    add(ziehe("Zelle auf die Kamera", [] {
        ImVec2 a{0, 0};
        (void)ke::assetZelleMitte(ke::assetIndex(kModell), a);
        return std::make_pair(a, kameraPunkt(0.0F));
    }));
    add(pause(0.3));
    add(pruefe("M0 Editor: misc_model auf dem Boden (Unterkante z 0), spawnflags 2, ein Schritt", [undo0] {
        const auto e = letzte("misc_model");
        const ke::Editor& ed = ke::editor();
        if (!e) { return false; }
        diag::detail("Knopftest: M0 Editor - misc_model origin " + e->wert("origin") + ", Unterkante " + std::to_string(ursprungZ(*e) + modellUnten()) +
                     ", spawnflags " + e->wert("spawnflags") + ", Schritt " + ed.karte.undo.undoName());
        return e->wert("model") == kModell && e->wert("spawnflags") == "2" && std::fabs(ursprungZ(*e) + modellUnten()) < 0.01F &&
               ed.karte.undo.undoAnzahl() == *undo0 + 1 && ed.karte.undo.undoName() == "entityCreate -class misc_model";
    }));
    auto pinsel0 = std::make_shared<std::size_t>(0);
    add(tu("M0 Editor: misc_model_static mit Clip-Brush (Doppelklick-Weg: Mitte der Kamera + 160)", [undo0, pinsel0] {
        ke::Editor& ed = ke::editor();
        *undo0 = ed.karte.undo.undoAnzahl();
        *pinsel0 = ed.karte.welt() ? ed.karte.welt()->kinder().size() : 0;
        ke::modellSetzKlasse() = 1;
        (void)ke::assetAbgelegt(ke::assetIndex(kModell), kameraPunkt(160.0F));
    }));
    add(pruefe("M0 Editor: misc_model_static auf dem Boden, Clip-Brush (common/clip) um das Modell, ein Schritt; Rueckgaengig nimmt beides", [undo0, pinsel0] {
        ke::Editor& ed = ke::editor();
        const auto e = letzte("misc_model_static");
        const auto w = ed.karte.welt();
        if (!e || !w || w->kinder().size() != *pinsel0 + 1) { return false; }
        const auto& p = w->kinder().back();
        bool clip = p->art() == karte::Primitiv::Art::Pinsel;
        if (clip) {
            for (const auto& f : std::static_pointer_cast<karte::Pinsel>(p)->flaechen()) { clip = clip && kl(f->shader()) == "textures/common/clip"; }
        }
        const bool unten = std::fabs(ursprungZ(*e) + modellUnten()) < 0.01F;
        const bool schritt = ed.karte.undo.undoAnzahl() == *undo0 + 1;
        (void)ed.karte.undo.undo();
        const bool weg = !letzte("misc_model_static") && w->kinder().size() == *pinsel0;
        (void)ed.karte.undo.redo();
        diag::detail("Knopftest: M0 Editor - misc_model_static " + e->wert("origin") + (clip ? ", Clip-Brush" : ", ohne Clip") + (weg ? ", Rueckgaengig nimmt beides" : ""));
        return clip && unten && schritt && weg && letzte("misc_model_static") != nullptr;
    }));
    add(tu("M0 Editor: misc_model_static waehlen, Gier 90 (rotateSelected)", [] {
        ke::Editor& ed = ke::editor();
        ed.karte.auswahl.leeren();
        if (!letzte("misc_model_static")) { return; }
        ed.karte.auswahl.waehlen(letzte("misc_model_static"));
        ke::auswahlGeaendert();
        karte::Befehl undo(ed.karte.undo, "rotateSelected");   // wie der Befehl des Menues
        ed.bearbeitung.dreheAuswahl(karte::Quaternion(0.0F, 0.0F, 0.70710678F, 0.70710678F));
    }));
    add(pruefe("M0 Editor Drehen: \"angle\" \"90\", kein angles", [] {
        const auto e = letzte("misc_model_static");
        if (!e) { return false; }
        diag::detail("Knopftest: M0 Editor - angle " + e->wert("angle") + ", angles " + e->wert("angles"));
        return e->wert("angle") == "90" && e->wert("angles").empty();
    }));
    add(tu("M0 Editor: skalieren 2 (scaleSelected), dann Hochachse 1.5, dann Nicken 30", [] {
        ke::Editor& ed = ke::editor();
        if (ed.karte.auswahl.anzahl() != 1) { return; }
        {
            karte::Befehl undo(ed.karte.undo, "scaleSelected");
            ed.bearbeitung.skaliereAuswahl(karte::Vektor3f(2, 2, 2));
        }
        {
            karte::Befehl undo(ed.karte.undo, "scaleSelected");
            ed.bearbeitung.skaliereAuswahl(karte::Vektor3f(1, 1, 1.5F));
        }
        karte::Befehl undo(ed.karte.undo, "rotateSelected");
        ed.bearbeitung.dreheAuswahl(karte::Quaternion(0.0F, 0.25881905F, 0.0F, 0.96592583F));
    }));
    add(pruefe("M0 Editor Skalieren/Drehen: genau ein Winkel- und ein Groessenschluessel, SP = MD9 = q3map2, Matrix des Editors = AnglesToAxis * Skala", [] {
        const auto e = letzte("misc_model_static");
        if (!e) { return false; }
        const modellsetzen::Schluessel k = schluessel(*e);
        const modellsetzen::Lage sp = modellsetzen::lageWieSpiel(k, false);
        float ax[3][3];
        modellsetzen::achsen(sp, ax);
        const karte::Matrix4 m = karte::miscModellMatrix(*e);
        float abw = 0.0F;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) { abw = std::max(abw, std::fabs(m[static_cast<std::size_t>(i * 4 + j)] - ax[i][j])); }
        }
        int winkel = 0;
        int groesse = 0;
        for (const auto& [a, b] : k) {
            winkel += (a == "angle" || a == "angles") ? 1 : 0;
            groesse += (a == "modelscale" || a == "modelscale_vec") ? 1 : 0;
        }
        diag::detail("Knopftest: M0 Editor - angles " + e->wert("angles") + ", modelscale_vec " + e->wert("modelscale_vec") + ", Lage " + lageText(sp) +
                     ", Matrix-Abweichung " + std::to_string(abw));
        return winkel == 1 && groesse == 1 && gleich(sp, modellsetzen::lageWieQ3map2(k)) && gleich(sp, modellsetzen::lageWieSpiel(k, true)) && abw < 1e-3F;
    }));
    // NRC dreht und skaliert um die Mitte der Auswahl: nach dem Nicken laege der Ursprung unter dem Boden (q3map2: Leck).
    add(tu("M0 Editor: Skalieren und Nicken zurueck (3 Schritte)", [] {
        for (int i = 0; i < 3; ++i) { (void)ke::editor().karte.undo.undo(); }
    }));
    // --- Bau ---------------------------------------------------------------------------------------------
    add(tu("M0 Editor: Bau (q3map2 -meta)", [] {
        ke::Editor& e = ke::editor();
        e.bauProjekt.eintraege.push_back(karte::BauEintrag{"M0 bsp", {"[q3map2] -meta \"[MapFile]\""}});
        ke::bauStarten(e.bauProjekt.eintraege.size() - 1);
    }));
    add(warteBis("M0 Editor: Bau fertig", [] { return !ke::bauLaeuft(); }, 30000));
    add(pruefe("M0 Bau: Flaechen des Modells in der .bsp (eingebacken), misc_model_static im Entity-Lump, misc_model nicht; beide fest", [] {
        const std::string bsp = (std::filesystem::current_path() / "s3bau" / "m0raum.bsp").string();
        const std::string b = slurp(bsp);
        BspGeometry g;
        std::string fehler;
        int flaechen = 0;
        if (readBspGeometry(b, g, &fehler)) {
            std::set<std::string> sh;
            const auto ohneEndung = [](std::string n) {
                n = kl(n);
                const std::size_t p = n.find_last_of('.');
                return p != std::string::npos && n.find('/', p) == std::string::npos ? n.substr(0, p) : n;
            };
            for (const Md3Surface& sf : crate().m.surfaces) { sh.insert(ohneEndung(sf.shader)); }
            for (const BspSurface& f : g.surfaces) {
                if (f.shader >= 0 && static_cast<std::size_t>(f.shader) < g.shaders.size() && sh.count(ohneEndung(g.shaders[static_cast<std::size_t>(f.shader)].name)) > 0) { ++flaechen; }
            }
            diag::detail("Knopftest: M0 Bau - Shader des Modells " + (sh.empty() ? std::string("-") : *sh.begin()) + ", " + std::to_string(g.surfaces.size()) + " Flaechen in der .bsp");
        }
        std::string lump;
        (void)readBspEntityLump(b, lump);
        KollisionsKarte k;
        const bool kollDa = leseKollision(b, k);   // die Bytes der .bsp
        const auto fest = [&k](const karte::Entitaet& e) {
            float o[3] = {0, 0, 0};
            (void)std::sscanf(e.wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
            const float a[3] = {o[0], o[1], 240.0F};
            const float z[3] = {o[0], o[1], -8.0F};
            const float null[3] = {0, 0, 0};
            const KollisionsSpur s = kollisionsSpur(k, a, null, null, z, 1U);
            return s.fraction < 1.0F ? s.endpos[2] : -1.0F;
        };
        const auto em = letzte("misc_model");
        const auto es = letzte("misc_model_static");
        const float hm = em ? fest(*em) : -1.0F;
        const float hs = es ? fest(*es) : -1.0F;
        diag::detail("Knopftest: M0 Bau - " + std::to_string(flaechen) + " Modellflaechen, Lump " + std::to_string(lump.size()) + " Zeichen, fest bei z " + std::to_string(hm) +
                     " (misc_model) / " + std::to_string(hs) + " (Clip-Brush)");
        return flaechen > 0 && lump.find("misc_model_static") != std::string::npos && lump.find("\"misc_model\"") == std::string::npos && kollDa && hm > 1.0F &&
               hs > 1.0F;
    }));
    // --- Live --------------------------------------------------------------------------------------------
    auto vorher = std::make_shared<std::pair<std::string, float>>();
    add(tu("M0 Editor Live: misc_model mit models/m0test/kiste.md3 (Import-Ordner)", [] {
        ke::modellSetzKlasse() = 0;
        ke::modellSetzKollision() = false;
        (void)ke::modellEinsetzen(kLive, kameraPunkt(-160.0F));
    }));
    add(pause(2.5));
    add(tu("M0 Editor Live: neuere Fassung (doppelt so gross) schreiben", [vorher] {
        const karte::ModellAnsicht a = ke::kartenModellPfad(kLive);
        vorher->second = a.modell ? a.modell->grenzen.extents[0] : 0.0F;
        const auto e = letzte("misc_model");
        vorher->first = e ? e->wert("origin") : std::string();
        schreib(importDatei(), verdoppelt(crate().bytes));
    }));
    add(pause(2.5));
    add(pruefe("M0 Editor Live: neu geladen (Grenzen doppelt), Ursprung gleich", [vorher] {
        const karte::ModellAnsicht a = ke::kartenModellPfad(kLive);
        const float x = a.modell ? a.modell->grenzen.extents[0] : 0.0F;
        const auto e = letzte("misc_model");
        diag::detail("Knopftest: M0 Editor Live - Ausdehnung x " + std::to_string(vorher->second) + " -> " + std::to_string(x));
        return vorher->second > 0.0F && std::fabs(x - 2.0F * vorher->second) < 0.05F && e && e->wert("model") == kLive && e->wert("origin") == vorher->first;
    }));
    add(tu("M0 Editor: zurueck zu Events", [] { g_app->leftMode = 0; }));
    return s;
}

}  // namespace m0

// BHED_EDITORTEST=handbuch BHED_HB_NUR=modellsetzen: das Bild h_m0_modelle (Fenster "Modelle" mit Suche, die Kiste als
// Geist unter der Maus auf dem Boden von md_am_sith).
std::vector<Schritt> modellsetzenHandbuchSchritte() {
    std::vector<Schritt> s = m0::mission();
    // bis einschliesslich der Probe des Geists - danach das Bild statt des Testfotos
    std::size_t n = 0;
    while (n < s.size() && s[n].text.find("M0 Geist: das Modell") == std::string::npos) { ++n; }
    s.resize(std::min(s.size(), n + 1));
    // Das Fenster rechts neben die Kartenansicht (ueber die Skriptspalte), damit die Kiste unter der Maus frei steht.
    s.push_back(tu("Handbuch M0: Fenster Modelle rechts neben der Ansicht", [] {
        if (ImGuiWindow* f = ImGui::FindWindowByName("###m0modelle")) {
            ImGui::SetWindowPos(f, ImVec2{std::max(g_app->kartenAnsicht[2] + 8.0F, ImGui::GetMainViewport()->Size.x - f->Size.x - 8.0F),
                                          g_app->kartenAnsicht[1]});
        }
    }));
    s.push_back(pause(0.8));
    s.push_back(foto("h_m0_modelle"));
    return s;
}

std::vector<Schritt> modellsetzenSchritte() {
    std::vector<Schritt> s = m0::mission();
    const char* nur = std::getenv("BHED_M0_NUR");
    if (nur != nullptr && std::strcmp(nur, "mission") == 0) { return s; }
    if (nur != nullptr && std::strcmp(nur, "editor") == 0) { s.clear(); }
    for (Schritt& x : m0::editor()) { s.push_back(std::move(x)); }
    return s;
}
