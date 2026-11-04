// app_modellsetzen.cpp - rc609 M0: vorhandene Modelle mit einem Klick in die Karte (Mission-Tab)
//
// shank (03.11.): "fuer meine map schnellstmoeglich Modelle wie Stuehle erstellen ... mit einem Klick einfuegen ...
// im map tab bewegen". Ohne den Modell-Block: die vorhandenen .md3 der Spielordner.
//
//   Fenster "Modelle"   die .md3 unter models/ (Archive und lose Dateien, die Fassung, die das Spiel liest), Suche
//   Setzen              Modell waehlen, in die Karte klicken oder ziehen; bis dahin steht es als Geist unter der Maus
//                       (3D-Vorschau mit dem Zeichenweg der Kartenmodelle). Es wird misc_model_static - die Karte hat
//                       keine Quelle, misc_model backt nur q3map2 ein. Jedi Outcast kennt misc_model_static nicht
//                       (codeJK2/game/g_spawn.cpp): dann nichts setzen und den Grund nennen.
//   Boden               Wie das Spiel die Lage liest (SP_misc_model_static setzt den Ursprung nicht auf den Boden,
//                       CG_DrawMiscEnts zeichnet das Modell an origin mit AnglesToAxis * modelscale): der Ursprung
//                       so, dass die Unterkante von R_ModelBounds (Bild 0) auf dem Boden der Spielkollision
//                       (CONTENTS_SOLID) unter dem getroffenen Punkt liegt.
//   Live                je Sekunde der Stand (Zeit, Groesse) der Modelldateien der Karte, dort wo das Spiel sie
//                       findet; eine neuere Fassung laedt die Vorschau neu, alle Exemplare behalten ihre Schluessel.
#include "app_internal.h"

#include "bhed/kollision.h"
#include "bhed/modellsetzen.h"
#include "bhed/regelwerk.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace bhed::gui {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::string zahl(float v) {
    char t[48];
    std::snprintf(t, sizeof(t), "%.6f", static_cast<double>(v));   // md3-Ecken sind 1/64 genau (MD3_XYZ_SCALE)
    std::string s = t;
    while (!s.empty() && s.back() == '0') { s.pop_back(); }
    if (!s.empty() && s.back() == '.') { s.pop_back(); }
    return s == "-0" ? std::string("0") : s;
}

// Die Grenzen, die das Spiel fuer das Modell nimmt (R_ModelBounds: Bild 0 aus dem Kopf), sonst der Kasten der Ecken
// von Bild 0. Gemerkt je Pfad und Fassung.
struct Grenzen {
    bool da = false;
    float mn[3]{};
    float mx[3]{};
};
std::map<std::string, Grenzen>& grenzenCache() {
    static std::map<std::string, Grenzen> c;
    return c;
}
const Grenzen& modellGrenzen(const std::string& pfad) {
    const std::string k = klein(pfad) + "#" + std::to_string(modellFassungVon(klein(pfad)));
    auto& c = grenzenCache();
    const auto it = c.find(k);
    if (it != c.end()) { return it->second; }
    Grenzen g;
    std::string data;
    Md3Model m;
    if (readFromArchives(pfad, data) && readMd3(data, m)) {
        if (m.kopfGrenzen) {
            for (int i = 0; i < 3; ++i) {
                g.mn[i] = m.grenzen[0][i];
                g.mx[i] = m.grenzen[1][i];
            }
            g.da = true;
        } else {
            for (const Md3Surface& sf : m.surfaces) {
                const Md3Vertex* v = sf.frame(0);
                for (int n = 0; v != nullptr && n < sf.numVerts; ++n) {
                    for (int i = 0; i < 3; ++i) {
                        g.mn[i] = g.da ? std::min(g.mn[i], v[n].xyz[i]) : v[n].xyz[i];
                        g.mx[i] = g.da ? std::max(g.mx[i], v[n].xyz[i]) : v[n].xyz[i];
                    }
                    g.da = true;
                }
            }
        }
    }
    return c.emplace(k, g).first->second;
}

}  // namespace

int modellSpiel() {
    return regelwerkJediOutcast() ? kRegelwerkJediOutcast : regelwerkMovieDuels() ? kRegelwerkMovieDuels : kRegelwerkOpenJK;
}

int modellFassungVon(const std::string& pfadKlein) {
    const auto it = g_app->modellFassung.find(pfadKlein);
    return it == g_app->modellFassung.end() ? 0 : it->second;
}

const std::vector<modellsetzen::ModellDatei>& modellBrowserListe() {
    if (g_app->modellListeNeu || g_app->modellListeOrdner != g_app->gamePaths.size()) {
        // misc_model_static nimmt nur .md3 (CG_CreateMiscEntFromGent, cg_main.cpp:1977-1982)
        g_app->modellListe = modellsetzen::modellDateien(g_app->gamePaths, {".md3"}, g_app->settings.fsDirBeforePak);
        g_app->modellListeOrdner = g_app->gamePaths.size();
        g_app->modellListeNeu = false;
    }
    return g_app->modellListe;
}

std::vector<std::size_t> modellBrowserTreffer(const std::string& suche) { return modellsetzen::modellSuche(modellBrowserListe(), suche); }

bool istKartenModell(const MapEntity& e) {
    if (e.classname.rfind("misc_model", 0) != 0) { return false; }
    const std::string* m = e.find("model");
    return m != nullptr && !m->empty();
}

modellsetzen::Lage modellLageVon(const MapEntity& e) { return modellsetzen::lageWieSpiel(e.keys, regelwerkMovieDuels()); }

void modellLageSetzen(MapEntity& e, const modellsetzen::Lage& l) {
    for (const auto& [k, v] : modellsetzen::lageSchluessel(l)) { setKey(e, k, v); }
}

bool modellOrtAn(const std::string& pfad, float sx, float sy, const modellsetzen::Lage& lage, float ort[3]) {
    const Grenzen& g = modellGrenzen(pfad);
    TraceTreffer t;
    if (!g.da || !kartenStrahlUnterMaus(sx, sy, t)) { return false; }
    float p[3] = {t.point[0], t.point[1], t.point[2]};
    if (t.normal[2] <= 0.7F) {   // eine Wand (MIN_WALK_NORMAL): ein Stueck davor, dann auf den Boden darunter
        for (int k = 0; k < 3; ++k) { p[k] += t.normal[k] * 16.0F; }
    }
    if (g_app->rasterSchritt > 0) {
        const float r = static_cast<float>(g_app->rasterSchritt);
        p[0] = std::round(p[0] / r) * r;
        p[1] = std::round(p[1] / r) * r;
    }
    // Der Boden der Spielkollision (CONTENTS_SOLID) unter dem Punkt - auch unter einer nicht festen Flaeche.
    float boden = p[2];
    if (const std::shared_ptr<const KollisionsKarte> k = geladeneKollision(g_app->map.path)) {
        const float a[3] = {p[0], p[1], p[2] + 2.0F};
        const float b[3] = {p[0], p[1], p[2] - 8192.0F};
        const float null[3] = {0.0F, 0.0F, 0.0F};
        const KollisionsSpur s = kollisionsSpur(*k, a, null, null, b, 1U);
        if (!s.startsolid && s.fraction < 1.0F) { boden = modellsetzen::bodenHoehe(s.endpos[2], s.normale[2]); }
    } else {
        const float a[3] = {p[0], p[1], p[2] + 2.0F};
        const float b[3] = {p[0], p[1], p[2] - 8192.0F};
        const TraceTreffer u = traceRay(g_app->geo, a, b);
        if (u.hit) { boden = u.point[2]; }
    }
    ort[0] = p[0];
    ort[1] = p[1];
    ort[2] = modellsetzen::ursprungZAufBoden(boden, g.mn, g.mx, lage);
    return true;
}

int modellSetzenAn(const std::string& pfad, float sx, float sy) {
    bool da = false;
    std::string grund;
    for (const modellsetzen::KlassenAngebot& a : modellsetzen::modellKlassen(modellSpiel(), false)) {
        if (a.klasse == "misc_model_static") {
            da = a.da;
            grund = a.grund;
        }
    }
    char t[700];
    if (!da) {
        std::snprintf(t, sizeof(t), tr(Str::M0KlasseFehlt), "misc_model_static", grund.c_str());
        addStatus(t);
        return -1;
    }
    float ort[3];
    if (!modellOrtAn(pfad, sx, sy, modellsetzen::Lage{}, ort)) {
        addStatus(tr(Str::M0KeinOrt));
        return -1;
    }
    std::vector<MapEntity> vorher = g_app->map.entities;
    MapEntity e;
    setKey(e, "classname", "misc_model_static");
    setKey(e, "origin", zahl(ort[0]) + ' ' + zahl(ort[1]) + ' ' + zahl(ort[2]));
    setKey(e, "model", pfad);
    g_app->map.entities.push_back(e);
    entitiesGeaendert(std::move(vorher));
    g_app->pickedEntity = static_cast<int>(g_app->map.entities.size()) - 1;
    g_app->entAuswahl.clear();
    g_app->selectedActor = -1;
    g_app->cameraSelected = false;
    std::snprintf(t, sizeof(t), tr(Str::M0Gesetzt), pfad.c_str(), static_cast<double>(ort[0]), static_cast<double>(ort[1]),
                  static_cast<double>(ort[2]));
    addStatus(t);
    addStatus(tr(Str::M0KeineKollision));
    return g_app->pickedEntity;
}

void modellPlatzierMausInnen();   // Setzen per Klick/Ziehen und der Geist (unten)

namespace {
// Die Vorschau zeichnet nur bei mapDirty neu: ein Geist, der erscheint, verschwindet oder wandert, braucht ein Bild.
void geistGeaendert(const App::ModellGeist& vorher) {
    const App::ModellGeist& g = g_app->modellGeist;
    if (vorher.aktiv != g.aktiv || vorher.pfad != g.pfad || vorher.ort[0] != g.ort[0] || vorher.ort[1] != g.ort[1] || vorher.ort[2] != g.ort[2]) {
        g_app->mapDirty = true;
    }
}
}  // namespace

void modellPlatzierMaus() {
    const App::ModellGeist vorher = g_app->modellGeist;
    g_app->modellGeist.aktiv = false;
    modellPlatzierMausInnen();
    geistGeaendert(vorher);
}

void modellPlatzierMausInnen() {
    if (g_app->modellPlatzieren.empty() || g_app->animZug) { return; }
    const ImGuiIO& io = ImGui::GetIO();
    const bool imBild = io.MousePos.x >= g_app->kartenAnsicht[0] && io.MousePos.x < g_app->kartenAnsicht[2] &&
                        io.MousePos.y >= g_app->kartenAnsicht[1] && io.MousePos.y < g_app->kartenAnsicht[3];
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        g_app->modellPlatzieren.clear();
        g_app->modellZiehen = false;
        return;
    }
    if (imBild) {
        App::ModellGeist& gst = g_app->modellGeist;
        const modellsetzen::Lage l;
        if (modellOrtAn(g_app->modellPlatzieren, io.MousePos.x, io.MousePos.y, l, gst.ort)) {
            gst.aktiv = true;
            gst.pfad = g_app->modellPlatzieren;
            for (int k = 0; k < 3; ++k) {
                gst.winkel[k] = l.winkel[k];
                gst.skala[k] = l.skala[k];
            }
        }
        ImGui::SetTooltip(tr(Str::PlaceClickHint), g_app->modellPlatzieren.c_str());
    }
    if (g_app->modellZiehen) {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (imBild && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
                (void)modellSetzenAn(g_app->modellPlatzieren, io.MousePos.x, io.MousePos.y);
                g_app->wegKlickVerbraucht = true;
            }
            g_app->modellPlatzieren.clear();
            g_app->modellZiehen = false;
        }
        return;
    }
    if (imBild && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        (void)modellSetzenAn(g_app->modellPlatzieren, io.MousePos.x, io.MousePos.y);
        g_app->wegKlickVerbraucht = true;   // dieser Klick waehlt nichts anderes aus
        if (!io.KeyShift) { g_app->modellPlatzieren.clear(); }
    }
}

void drawModellFenster() {
    if (!g_app->modellFensterOffen) { return; }
    const float fs = ImGui::GetFontSize();
    // Unten rechts in der Kartenansicht: links liegen die Kacheln, oben links die Gizmo-Leiste, die Mitte bleibt frei.
    const ImVec2 groesse{fs * 26.0F, fs * 22.0F};
    ImGui::SetNextWindowSize(groesse, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2{std::max(g_app->kartenAnsicht[0], g_app->kartenAnsicht[2] - groesse.x - fs * 0.5F),
                                   std::max(g_app->kartenAnsicht[1], g_app->kartenAnsicht[3] - groesse.y - fs * 0.5F)},
                            ImGuiCond_FirstUseEver);
    bool offen = true;
    const std::string titel = std::string(tr(Str::M0Fenster)) + "###m0modelle";
    if (ImGui::Begin(titel.c_str(), &offen)) {
        for (const modellsetzen::KlassenAngebot& a : modellsetzen::modellKlassen(modellSpiel(), false)) {
            if (a.klasse != "misc_model_static" || a.da) { continue; }
            char t[700];
            std::snprintf(t, sizeof(t), tr(Str::M0KlasseFehlt), a.klasse.c_str(), a.grund.c_str());
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0F, 0.75F, 0.35F, 1.0F});
            ImGui::TextWrapped("%s", t);
            ImGui::PopStyleColor();
        }
        ImGui::SetNextItemWidth(-fs * 7.0F);
        ImGui::InputTextWithHint("##m0suche", tr(Str::M0SucheHint), g_app->modellSuche, sizeof(g_app->modellSuche));
        ImGui::SameLine();
        if (ImGui::Button((std::string(tr(Str::M0Neu)) + "##m0neu").c_str())) { g_app->modellListeNeu = true; }
        const std::vector<std::size_t> treffer = modellBrowserTreffer(g_app->modellSuche);
        const auto& liste = modellBrowserListe();
        ImGui::TextDisabled(trN(Str::M0Anzahl, treffer.size()), static_cast<int>(treffer.size()));
        ImGui::TextWrapped("%s", tr(Str::M0Hinweis));
        if (ImGui::BeginChild("##m0liste", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_Borders)) {
            ImGuiListClipper clip;
            clip.Begin(static_cast<int>(treffer.size()));
            while (clip.Step()) {
                for (int i = clip.DisplayStart; i < clip.DisplayEnd; ++i) {
                    const modellsetzen::ModellDatei& d = liste[treffer[static_cast<std::size_t>(i)]];
                    const bool an = g_app->modellPlatzieren == d.pfad;
                    const std::string name = d.pfad.substr(std::string("models/").size()) + "##m0" + d.pfad;
                    if (ImGui::Selectable(name.c_str(), an)) {
                        g_app->modellPlatzieren = an ? std::string() : d.pfad;
                        g_app->modellZiehen = false;
                        g_app->platzierKlasse.clear();
                    }
                    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 8.0F)) {
                        g_app->modellPlatzieren = d.pfad;
                        g_app->modellZiehen = true;
                    }
                    if (ImGui::IsItemHovered()) {
                        const Grenzen& g = modellGrenzen(d.pfad);
                        ImGui::SetTooltip(tr(Str::M0Info), d.pfad.c_str(), d.herkunft.c_str(), static_cast<double>(g.mx[0] - g.mn[0]),
                                          static_cast<double>(g.mx[1] - g.mn[1]), static_cast<double>(g.mx[2] - g.mn[2]));
                    }
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
    if (!offen) {
        g_app->modellFensterOffen = false;
        g_app->modellPlatzieren.clear();
    }
}

void modellLiveWache() {
    const double jetzt = ImGui::GetTime();
    if (g_app->map.empty() || (g_app->modellWacheZeit >= 0.0 && jetzt - g_app->modellWacheZeit < 1.0)) { return; }
    g_app->modellWacheZeit = jetzt;
    std::set<std::string> pfade;
    for (const MapEntity& e : g_app->map.entities) {
        if (!istKartenModell(e)) { continue; }
        const std::string p = klein(*e.find("model"));
        if (p.size() > 4 && p.compare(p.size() - 4, 4, ".md3") == 0) { pfade.insert(p); }
    }
    std::map<std::string, modellsetzen::DateiStand> pk3Stand;   // je Runde einmal je Archiv
    std::vector<std::string> neu;
    for (const std::string& p : pfade) {
        bool lose = false;
        const std::string ort = modellsetzen::fundort(g_app->gamePaths, p, g_app->settings.fsDirBeforePak, &lose);
        modellsetzen::DateiStand s;
        if (!ort.empty()) {
            if (lose) {
                s = modellsetzen::dateiStand(ort);
            } else {
                auto it = pk3Stand.find(ort);
                if (it == pk3Stand.end()) { it = pk3Stand.emplace(ort, modellsetzen::dateiStand(ort)).first; }
                s = it->second;
            }
        }
        // Neu ist ein anderer Stand oder ein anderer Fundort (etwa eine neue lose Datei vor der .pk3).
        const auto alt = g_app->modellStaende.find(p);
        if (alt != g_app->modellStaende.end() && (alt->second.first != ort || !(alt->second.second == s))) { neu.push_back(p); }
        g_app->modellStaende[p] = {ort, s};
    }
    for (const std::string& p : neu) {
        ++g_app->modellFassung[p];
        g_app->md3Files.erase(p);
        int exemplare = 0;
        for (const MapEntity& e : g_app->map.entities) { exemplare += istKartenModell(e) && klein(*e.find("model")) == p ? 1 : 0; }
        char t[600];
        std::snprintf(t, sizeof(t), trN(Str::M0NeuGeladen, static_cast<std::size_t>(exemplare)), p.c_str(), exemplare);
        addStatus(t);
    }
    if (!neu.empty()) {
        g_app->kartenModelleNeu = true;
        g_app->mapDirty = true;
    }
}

}  // namespace bhed::gui
