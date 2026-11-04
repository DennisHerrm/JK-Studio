// app_deckung.cpp - rc609 H2: Kampfpunkt-Vorschlaege hinter Deckung im Wegpunkt-Fenster
//
// Mike (03.11.): point_combat "should be sprinkled around randomly (usually behind cover) ... They are used by gunners to
// move to cover or get a better angle for shooting." Die Regel (NPC_FindCombatPoint mit CP_CLEAR|CP_COVER, wie
// ST_Commander ihn ruft) und die Suche stehen in bhed/wegpunkte.h (wegDeckungGilt, wegDeckungVorschlaege). Hier: der
// Auftrag im Hintergrund wie die Abdeckung (W4), die Zuordnung zum Netz wie im Spiel (navKampfPunkte, W4), Annehmen in
// EINEM Rueckgaengig-Schritt, die Geisterpunkte in der Ansicht.
#include "app_internal.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace bhed::gui {

struct WegDeckAuftrag {
    int stand = -1;                          // g_app->entStand beim Start
    std::string karte;
    std::size_t entZahl = 0;
    std::vector<std::array<float, 3>> starts;
    std::vector<char> istWegpunkt;
    WegDeckungWunsch wunsch;
    std::unique_ptr<WegWeltKarte> welt;
    std::shared_ptr<const NavWelt> spiel;     // die Welt zur Laufzeit (navWeltSpawn): Sicht und Kasten wie im Kampf
    WegDeckungErgebnis ergebnis;
    double sekunden = 0.0;
    std::atomic<float> anteil{0.0F};
    std::atomic<bool> abbrechen{false};
    std::atomic<bool> fertig{false};
    std::thread faden;
    WegDeckAuftrag() = default;
    WegDeckAuftrag(const WegDeckAuftrag&) = delete;
    WegDeckAuftrag& operator=(const WegDeckAuftrag&) = delete;
    ~WegDeckAuftrag() {
        abbrechen = true;
        if (faden.joinable()) { faden.join(); }
    }
};

namespace {

// MAX_COMBAT_POINTS (g_local.h:138 OpenJK, Movie Duels gleich; Jedi Outcast ebenso 512): SP_point_combat nimmt nicht mehr.
constexpr int kMaxKampfPunkte = 512;

// Gemessen an den Originalkarten (rc609 H2, Skript h2/cpmess.py, nur gelesen): alle maps/*.bsp der .pk3 des Spiels,
// davon die mit point_combat. Jedi Academy (GameData original/base): 23 Karten, 2998 Kampfpunkte; Movie Duels (Ordner
// MD): 75 Karten, 9097; Jedi Outcast (GameData/base): 23 Karten, 2964. Abstand = Median je Karte des Medians des
// Abstands zum naechsten point_combat; je Wegpunkt = Median je Karte von point_combat / waypoint.
struct Vorgabe {
    float abstand;
    float jeWegpunkt;
};
Vorgabe vorgabe() {
    if (navJk2Regeln()) { return {145.0F, 0.449F}; }
    if (regelwerkMovieDuels()) { return {200.0F, 1.0F}; }
    return {231.0F, 0.519F};
}

bool istKampfpunkt(const MapEntity& e) {
    std::string k = e.classname;
    for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return k == "point_combat";
}
bool istWeg(const MapEntity& e) {
    std::string k = e.classname;
    for (char& c : k) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return k == "waypoint" || k == "waypoint_small";
}
int wegpunktZahl() {
    int n = 0;
    for (const MapEntity& e : g_app->map.entities) { n += istWeg(e) ? 1 : 0; }
    return n;
}
int kampfpunktZahl() {
    int n = 0;
    for (const MapEntity& e : g_app->map.entities) { n += istKampfpunkt(e) ? 1 : 0; }
    return n;
}

// Ein point_combat wie "Kampfpunkt setzen" (W4): origin ganzzahlig, kein target.
MapEntity kampfEntity(const std::array<float, 3>& ort) {
    MapEntity e;
    setKey(e, "classname", "point_combat");
    char o[96];
    std::snprintf(o, sizeof(o), "%d %d %d", static_cast<int>(std::lround(ort[0])), static_cast<int>(std::lround(ort[1])),
                  static_cast<int>(std::lround(ort[2])));
    setKey(e, "origin", o);
    return e;
}

bool deckAktuell() {
    return g_app->wegDeckStand >= 0 && g_app->wegDeckStand == g_app->entStand;
}

// Die Vorschlaege so, wie das Spiel sie nach dem Laden kennt (navKampfPunkte, W4): alle zugleich gesetzt, verworfen, was
// keinen Wegpunkt bekommt ("has no waypoint"). CP_HAS_ROUTE (NAV::InSameRegion mit der Figur) haengt davon ab, wo die Figur
// zur Laufzeit steht; erreichbar sind die Kandidaten schon zu Fuss von einem Wegpunkt aus (das Laufgitter).
std::vector<WegDeckung> zuordnen(const std::vector<WegDeckung>& v, int& verworfen) {
    verworfen = 0;
    if (v.empty()) { return {}; }
    std::vector<MapEntity> ents = g_app->map.entities;
    const std::size_t basis = ents.size();
    for (const WegDeckung& d : v) { ents.push_back(kampfEntity(d.ort)); }
    const std::shared_ptr<const NavWelt> welt = navWeltFuer(ents);
    if (!welt || !welt->netz) { return v; }
    std::vector<WegDeckung> aus;
    for (const NavKampfPunkt& k : navKampfPunkte(ents, *welt)) {
        if (k.entity < static_cast<int>(basis)) { continue; }
        if (k.ohneWegpunkt || k.zuViele) {
            ++verworfen;
        } else {
            aus.push_back(v[static_cast<std::size_t>(k.entity) - basis]);
        }
    }
    return aus;
}

}  // namespace

float wegDeckungAbstandVorgabe() { return vorgabe().abstand; }

int wegDeckungHoechstensVorgabe() {
    const int frei = std::max(0, kMaxKampfPunkte - kampfpunktZahl());
    return std::min(frei, std::max(1, static_cast<int>(std::lround(vorgabe().jeWegpunkt * static_cast<float>(wegpunktZahl())))));
}

bool wegDeckungStarten(bool hintergrund) {
    if (g_app->geo.empty() || g_app->wegDeckAuftrag) { return false; }
    g_app->wegDeck.clear();
    g_app->wegDeckStand = -1;
    g_app->wegDeckVerworfen = 0;
    auto a = std::make_shared<WegDeckAuftrag>();
    a->stand = g_app->entStand;
    a->karte = g_app->map.path;
    a->entZahl = g_app->map.entities.size();
    std::vector<int> startEntity;
    wegStartsVoll(g_app->map.entities, a->starts, startEntity);
    // ohne die Kamera: die Vorschlaege haengen nur an der Karte
    a->starts.pop_back();
    startEntity.pop_back();
    int wege = 0;
    for (const int se : startEntity) {
        a->istWegpunkt.push_back(se >= 0 ? 1 : 0);
        wege += se >= 0 ? 1 : 0;
    }
    if (wege == 0) {
        g_app->wegDeckMeldung = tr(Str::WegDeckKeinNetz);
        return false;
    }
    WegDeckungWunsch& w = a->wunsch;
    if (navJk2Regeln()) {
        w.regeln.jk2 = true;
        w.regeln.meiden = 0.0F;
        w.regeln.sammeln = 512.0F;   // CP_COLLECT_RADIUS (codeJK2/game/NPC_combat.cpp:2489)
    }
    // Nahe dem Netz wie die Abdeckung (W4): die Rasterweite, bzw. der Abstand der vollen Erzeugung, wenn groesser.
    w.reichweite = std::max(g_app->wegAbstand, g_app->wegAbstandVoll);
    w.feindRaster = g_app->wegAbstand;
    w.abstand = g_app->wegDeckAbstand > 0.0F ? g_app->wegDeckAbstand : wegDeckungAbstandVorgabe();
    w.hoechstens = std::min(std::max(0, kMaxKampfPunkte - kampfpunktZahl()),
                            g_app->wegDeckHoechstens > 0 ? g_app->wegDeckHoechstens : wegDeckungHoechstensVorgabe());
    for (const MapEntity& e : g_app->map.entities) {
        float o[3];
        if (istKampfpunkt(e) && entityOrt(e, o)) { w.vorhanden.push_back({o[0], o[1], o[2]}); }
    }
    if (g_app->wegDeckNurBereich) {
        const std::vector<WegBereich> bereiche = g_app->wegBereiche;
        w.erlaubt = [bereiche](const std::array<float, 3>& boden) {
            bool drin = false;
            for (const WegBereich& b : bereiche) {
                if (!imWegBereich(b, boden)) { continue; }
                if (b.art == WegBereich::Art::Sperre) { return false; }
                drin = true;
            }
            return drin;
        };
    }
    w.faeden = std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1);
    a->welt = wegWeltKarte(g_app->map.entities);
    a->spiel = navWeltSpawn();
    if (!a->welt->welt.spur || !a->spiel || !a->spiel->spur) { return false; }
    // Gelaufen wird in der Welt der Erzeugung (Tueren offen, W3a); geprueft wird die Deckung in der Welt, in der gekaempft
    // wird - Brush-Entities an ihrem Platz zur Laufzeit (Tueren zu, START_OFF ohne contents), wie die Figuren der Vorschau.
    // Ein Punkt in einer Tuer ist offen begehbar, geschlossen aber startsolid.
    WegDeckAuftrag* roh = a.get();
    roh->welt->welt.sicht = [roh](const std::array<float, 3>& p, const std::array<float, 3>& q) {
        return gClearLos(roh->spiel->spur, &roh->welt->ents, NavVec{p[0], p[1], p[2]}, NavVec{q[0], q[1], q[2]});
    };
    w.kastenFrei = [roh](const std::array<float, 3>& o) {
        const NavVec p{o[0], o[1], o[2]};
        const NavSpur s = roh->spiel->spur(p, {-16.0F, -16.0F, -24.0F}, {16.0F, 16.0F, 40.0F}, p, kNavKeineEntity, kNavMaskNpcSolid);
        return !s.startsolid && !s.allsolid;
    };
    w.weiter = [roh](float x) {
        roh->anteil = x;
        return !roh->abbrechen.load();
    };
    const auto rechne = [roh] {
        const auto t0 = std::chrono::steady_clock::now();
        roh->ergebnis = wegDeckungVorschlaege(roh->starts, roh->istWegpunkt, roh->welt->welt, roh->wunsch);
        roh->sekunden = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        roh->fertig = true;
    };
    g_app->wegDeckMeldung.clear();
    g_app->wegDeckAuftrag = a;
    if (hintergrund) {
        a->faden = std::thread(rechne);
    } else {
        rechne();
        wegDeckungPruefen();
    }
    return true;
}

void wegDeckungAbbrechen() {
    if (g_app->wegDeckAuftrag) { g_app->wegDeckAuftrag->abbrechen = true; }
}
bool wegDeckungLaeuft() { return g_app->wegDeckAuftrag != nullptr; }
float wegDeckungAnteil() { return g_app->wegDeckAuftrag ? g_app->wegDeckAuftrag->anteil.load() : 0.0F; }

void wegDeckungPruefen() {
    if (!g_app->wegDeckAuftrag || !g_app->wegDeckAuftrag->fertig) { return; }
    const std::shared_ptr<WegDeckAuftrag> a = std::move(g_app->wegDeckAuftrag);
    if (a->faden.joinable()) { a->faden.join(); }
    const WegDeckungErgebnis& e = a->ergebnis;
    if (e.abgebrochen) {
        g_app->wegDeckMeldung = tr(Str::WegDeckAbgebrochen);
        diag::detail("Wegpunkte: Kampfpunkt-Vorschlaege abgebrochen");
        return;
    }
    if (a->stand != g_app->entStand || a->karte != g_app->map.path || a->entZahl != g_app->map.entities.size()) {
        g_app->wegDeckMeldung = tr(Str::WegDeckVeraltet);
        return;
    }
    const auto t0 = std::chrono::steady_clock::now();
    int verworfen = 0;
    g_app->wegDeck = zuordnen(e.punkte, verworfen);
    g_app->wegDeckVerworfen = verworfen;
    g_app->wegDeckStand = g_app->entStand;
    const double msZu = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    char z[200];
    std::snprintf(z, sizeof(z), tr(Str::WegDeckFertig), static_cast<int>(e.kandidaten), static_cast<int>(e.mitDeckung), a->sekunden);
    g_app->wegDeckMeldung = g_app->wegDeck.empty() && verworfen == 0 ? std::string(tr(Str::WegDeckKeine)) + " " + z : std::string(z);
    diag::detail("Wegpunkte: Kampfpunkt-Vorschlaege - " + std::to_string(e.kandidaten) + " Kandidaten, " + std::to_string(e.feindStellen) +
                 " Feindstellen, " + std::to_string(e.mitDeckung) + " mit Deckung, " + std::to_string(e.punkte.size()) + " gewaehlt, " +
                 std::to_string(g_app->wegDeck.size()) + " zugeordnet, " + std::to_string(verworfen) + " verworfen, Abstand " +
                 std::to_string(static_cast<int>(a->wunsch.abstand)) + ", hoechstens " + std::to_string(a->wunsch.hoechstens) + ", " +
                 std::to_string(a->wunsch.faeden) + " Faeden, " + std::to_string(static_cast<int>(a->sekunden * 1000.0)) + " ms (Zuordnung " +
                 std::to_string(static_cast<int>(msZu)) + " ms)");
}

int wegDeckungAnnehmen(int index) {
    if (!deckAktuell() || g_app->wegDeck.empty()) { return 0; }
    std::vector<MapEntity> vorher = g_app->map.entities;
    std::vector<WegDeckung> rest;
    int n = 0;
    for (std::size_t i = 0; i < g_app->wegDeck.size(); ++i) {
        if (index >= 0 && static_cast<int>(i) != index) {
            rest.push_back(g_app->wegDeck[i]);
            continue;
        }
        g_app->map.entities.push_back(kampfEntity(g_app->wegDeck[i].ort));
        ++n;
    }
    if (n == 0) { return 0; }
    entitiesGeaendert(std::move(vorher));
    g_app->hiddenClasses.erase("point_combat");
    g_app->wegDeck = std::move(rest);
    g_app->wegDeckStand = g_app->entStand;   // die uebrigen gelten weiter
    char z[120];
    std::snprintf(z, sizeof(z), trN(Str::WegDeckAngenommen, n), n);
    g_app->wegDeckMeldung = z;
    return n;
}

void wegDeckungVerwerfen() {
    g_app->wegDeck.clear();
    g_app->wegDeckStand = -1;
    g_app->wegDeckVerworfen = 0;
}

void wegDeckungFenster() {
    wegDeckungPruefen();
    if (!deckAktuell() && !g_app->wegDeck.empty()) { wegDeckungVerwerfen(); }   // die Karte hat sich geaendert
    ImGui::PushID("##wegdeckung");
    const float fs = ImGui::GetFontSize();
    const bool laeuft = wegDeckungLaeuft();
    if (iconTextKnopf(tr(Str::WegDeckVorschlagen), UiIcon::Zielen, tr(Str::WegDeckVorschlagenHint), !laeuft && !g_app->geo.empty())) {
        g_app->zeigeWegnetz = true;
        (void)wegDeckungStarten(true);
    }
    ImGui::SameLine();
    if (ImGui::Checkbox(tr(Str::WegDeckNurBereich), &g_app->wegDeckNurBereich)) { wegDeckungVerwerfen(); }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::WegDeckNurBereichHint)); }
    float abstand = g_app->wegDeckAbstand > 0.0F ? g_app->wegDeckAbstand : wegDeckungAbstandVorgabe();
    ImGui::SetNextItemWidth(fs * 7.0F);
    if (ImGui::DragFloat(tr(Str::WegDeckAbstand), &abstand, 4.0F, 32.0F, 2048.0F, "%.0f")) { g_app->wegDeckAbstand = abstand; }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::WegDeckAbstandHint)); }
    ImGui::SameLine();
    int hoechstens = g_app->wegDeckHoechstens > 0 ? g_app->wegDeckHoechstens : wegDeckungHoechstensVorgabe();
    ImGui::SetNextItemWidth(fs * 6.0F);
    if (ImGui::DragInt(tr(Str::WegDeckHoechstens), &hoechstens, 1.0F, 1, kMaxKampfPunkte)) { g_app->wegDeckHoechstens = hoechstens; }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) { ImGui::SetTooltip("%s", tr(Str::WegDeckHoechstensHint)); }
    if (laeuft) {
        ImGui::ProgressBar(wegDeckungAnteil(), ImVec2{fs * 15.0F, 0.0F}, tr(Str::WegDeckRechnet));
        ImGui::SameLine();
        if (iconTextKnopf(tr(Str::WegAbbrechen), UiIcon::Schliessen, tr(Str::WegDeckAbbrechenHint))) { wegDeckungAbbrechen(); }
    }
    if (!g_app->wegDeckMeldung.empty()) {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + fs * 23.0F);
        ImGui::TextDisabled("%s", g_app->wegDeckMeldung.c_str());
        ImGui::PopTextWrapPos();
    }
    // Annehmen und Verwerfen aendern die Liste: erst nach dem Zeichnen (-1 alle annehmen, -2 verwerfen, >= 0 einen).
    int aktion = -3;
    if (deckAktuell() && (!g_app->wegDeck.empty() || g_app->wegDeckVerworfen > 0)) {
        const int n = static_cast<int>(g_app->wegDeck.size());
        char zs[80];
        std::snprintf(zs, sizeof(zs), trN(Str::WegDeckStand, n), n);
        ImGui::TextUnformatted(zs);
        if (g_app->wegDeckVerworfen > 0) { ImGui::TextDisabled(tr(Str::WegDeckVerworfen), g_app->wegDeckVerworfen); }
        if (n > 0) {
            if (iconTextKnopf(tr(Str::WegDeckAlle), UiIcon::Haken, tr(Str::WegDeckAlleHint))) { aktion = -1; }
            ImGui::SameLine();
        }
        if (iconTextKnopf(tr(Str::WegDeckVerwerfen), UiIcon::Loeschen, tr(Str::WegDeckVerwerfenHint))) { aktion = -2; }
        if (n > 0) {
            ImGui::BeginChild("##deckliste", ImVec2{-1.0F, ImGui::GetTextLineHeightWithSpacing() * static_cast<float>(std::min(n, 6) + 1)},
                              ImGuiChildFlags_Borders);
            for (int i = 0; i < n; ++i) {
                const WegDeckung& d = g_app->wegDeck[static_cast<std::size_t>(i)];
                ImGui::PushID(i);
                if (ImGui::SmallButton(tr(Str::WegDeckAnnehmen))) { aktion = i; }
                ImGui::SameLine();
                char z[200];
                std::snprintf(z, sizeof(z), tr(Str::WegDeckGrund), d.feinde, static_cast<double>(d.richtung < 0.0F ? d.richtung + 360.0F : d.richtung));
                if (ImGui::Selectable(z)) {
                    g_app->cam.pos[0] = d.ort[0];
                    g_app->cam.pos[1] = d.ort[1];
                    g_app->cam.pos[2] = d.ort[2] + 64.0F;
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                    ImGui::SetTooltip("%.0f %.0f %.0f -> %.0f %.0f %.0f", static_cast<double>(d.ort[0]), static_cast<double>(d.ort[1]),
                                      static_cast<double>(d.ort[2]), static_cast<double>(d.feindMitte[0]), static_cast<double>(d.feindMitte[1]),
                                      static_cast<double>(d.feindMitte[2]));
                }
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
    }
    if (aktion == -2) {
        wegDeckungVerwerfen();
    } else if (aktion >= -1) {
        (void)wegDeckungAnnehmen(aktion);
    }
    ImGui::PopID();
}

void wegDeckungZeichnen(ImDrawList* dl, float px) {
    if (!deckAktuell()) { return; }
    // Geisterpunkte: der Kasten des point_combat (QUAKED -16 -16 -24, 16 16 32) blass in der Farbe der Kampfpunkte, ein Pfeil
    // in die Richtung der Feindstellen, gegen die er deckt.
    const ImU32 farbe = IM_COL32(200, 40, 200, 120);
    const ImU32 pfeil = IM_COL32(255, 120, 255, 200);
    for (const WegDeckung& d : g_app->wegDeck) {
        if (g_app->draufsicht && d.ort[2] - 24.0F > g_app->draufSchnitt) { continue; }
        static const float kEcke[2][3] = {{-16.0F, -16.0F, -24.0F}, {16.0F, 16.0F, 32.0F}};
        ImVec2 e[8];
        bool alle = true;
        for (int i = 0; i < 8; ++i) {
            const float w[3] = {d.ort[0] + kEcke[i & 1][0], d.ort[1] + kEcke[(i >> 1) & 1][1], d.ort[2] + kEcke[(i >> 2) & 1][2]};
            alle = kartenPunktAufSchirm(w, e[i]) && alle;
        }
        if (alle) {
            static const int kKante[12][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
            for (int st = 0; st < 12; ++st) {   // gestrichelt: noch kein point_combat
                const ImVec2 a = e[kKante[st][0]];
                const ImVec2 b = e[kKante[st][1]];
                for (int t = 0; t < 6; t += 2) {
                    const float t0 = static_cast<float>(t) / 6.0F;
                    const float t1 = static_cast<float>(t + 1) / 6.0F;
                    dl->AddLine(ImVec2{a.x + (b.x - a.x) * t0, a.y + (b.y - a.y) * t0}, ImVec2{a.x + (b.x - a.x) * t1, a.y + (b.y - a.y) * t1}, farbe,
                                1.4F * px);
                }
            }
        }
        const float wm[3] = {d.ort[0], d.ort[1], d.ort[2] - 20.0F};
        const float r = d.richtung * (3.14159265F / 180.0F);
        const float wz[3] = {d.ort[0] + std::cos(r) * 48.0F, d.ort[1] + std::sin(r) * 48.0F, d.ort[2] - 20.0F};
        ImVec2 c;
        ImVec2 z;
        if (kartenPunktAufSchirm(wm, c) && kartenPunktAufSchirm(wz, z)) {
            dl->AddCircleFilled(c, 4.0F * px, pfeil);
            dl->AddLine(c, z, pfeil, 2.0F * px);
            dl->AddCircleFilled(z, 2.5F * px, pfeil);
        }
    }
}

}  // namespace bhed::gui
