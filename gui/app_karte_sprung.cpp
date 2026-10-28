// app_karte_sprung.cpp - Karten-Editor, Stufe 9 Punkt 4: Sprungweite und Platz pruefen
// (eigene Funktion, nicht in NetRadiant-Custom).
//
// View > Check jumps and space > Check: um die Auswahl (sonst um die Kamera, 512 Einheiten)
// prueft karte::sprungPruefen im Hintergrund mit der Kollision aus den Brushes und Patches
// (kollisionAusKarte, wie "Im Editor spielen") und dem Pmove-Port, wo der Spieler steht bzw.
// nur geduckt passt und welche Kanten per Sprung bzw. Machtsprung (Stufe 1-3) erreichbar sind.
// Die 2D-Ansichten und die Kamera zeigen das Ergebnis: gruene Kreuze stehen, gelbe nur
// geduckt, blaue Bahnen normale Spruenge, violette nur mit Machtsprung. "Im Editor spielen"
// springt mit derselben Machtstufe.
#include "karteneditor.h"

#include "app_internal.h"

#include "bhed/kollision.h"
#include "bhed/karte/spielkollision.h"
#include "bhed/karte/npcwege.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/sprungpruefung.h"
#include "bhed/karte/streuen.h"
#include "bhed/karte/grenzen.h"

#include <cmath>
#include <set>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>

namespace bhed::gui::karteneditor {

namespace {

using i18n::Str;

Editor& e() { return editor(); }

struct Lauf {
    std::thread faden;
    std::atomic<bool> abbrechen{false};
    std::atomic<int> fortschritt{0};
    std::atomic<bool> fertig{false};
    std::mutex mutex;
    std::shared_ptr<karte::SprungBericht> ergebnis;
};
Lauf& g_lauf = *new Lauf;

void fadenAbwarten() {
    if (g_lauf.faden.joinable()) {
        g_lauf.abbrechen = true;
        g_lauf.faden.join();
    }
    g_lauf.abbrechen = false;
}

std::vector<std::string> parmsVon(const std::string& shader) {
    const Editor& ed = e();
    const auto& sp = ed.karteSpiel ? ed.karteSpiel : ed.spiel;
    if (!sp) { return {}; }
    const karte::ShaderEintrag* s = sp->inventar.shaderFinden(shader);
    return s != nullptr ? s->surfaceparms : std::vector<std::string>{};
}

}  // namespace

void sprungPruefungStarten() {
    Editor& ed = e();
    fadenAbwarten();
    const auto& sp = ed.karteSpiel ? ed.karteSpiel : ed.spiel;
    const bool jk2 = sp && sp->spiel == kRegelwerkJediOutcast;
    auto k = std::make_shared<KollisionsKarte>();
    (void)karte::kollisionAusKarte(ed.karte, parmsVon, !jk2, *k);
    karte::SprungOptionen o;
    karte::Kasten b;
    if (auswahlGrenzen(b)) {
        for (std::size_t i = 0; i < 3; ++i) {
            o.bereichMin[i] = b.origin[i] - b.extents[i] - 64.0F;
            o.bereichMax[i] = b.origin[i] + b.extents[i] + 64.0F;
        }
    } else {
        for (std::size_t i = 0; i < 3; ++i) {
            const float r = i == 2 ? 384.0F : 512.0F;
            o.bereichMin[i] = ed.kamera.ursprung[i] - r;
            o.bereichMax[i] = ed.kamera.ursprung[i] + r;
        }
    }
    o.machtStufe = ed.sprungMacht;
    o.jk2 = jk2;
    ed.sprungLaeuft = true;
    g_lauf.fertig = false;
    g_lauf.fortschritt = 0;
    char t[256];
    std::snprintf(t, sizeof(t), trK(Str::KeSjStart), static_cast<double>(o.bereichMax[0] - o.bereichMin[0]),
                  static_cast<double>(o.bereichMax[1] - o.bereichMin[1]), ed.sprungMacht);
    ausgabe(t);
    g_lauf.faden = std::thread([k, o] {
        const NavSpurFn spur = [k](const NavVec& a, const NavVec& mi, const NavVec& ma, const NavVec& z, int, std::uint32_t m) {
            const KollisionsSpur s = kollisionsSpur(*k, a.data(), mi.data(), ma.data(), z.data(), m, 0);
            NavSpur r;
            r.allsolid = s.allsolid;
            r.startsolid = s.startsolid;
            r.fraction = s.fraction;
            r.endpos = s.endpos;
            r.normal = s.normale;
            r.contents = s.inhalt;
            r.entity = s.fraction < 1.0F ? kNavWeltEntity : kNavKeineEntity;
            return r;
        };
        auto erg = std::make_shared<karte::SprungBericht>(karte::sprungPruefen(spur, o, &g_lauf.abbrechen, &g_lauf.fortschritt));
        std::lock_guard<std::mutex> l(g_lauf.mutex);
        g_lauf.ergebnis = std::move(erg);
        g_lauf.fertig = true;
    });
}

void sprungPruefungLeeren() {
    fadenAbwarten();
    e().sprungBericht.reset();
    e().sprungLaeuft = false;
    ++e().sichtStand;
}

void sprungPruefungSchritt() {
    Editor& ed = e();
    if (!ed.sprungLaeuft || !g_lauf.fertig.load()) { return; }
    if (g_lauf.faden.joinable()) { g_lauf.faden.join(); }
    {
        std::lock_guard<std::mutex> l(g_lauf.mutex);
        ed.sprungBericht = g_lauf.ergebnis;
    }
    ed.sprungLaeuft = false;
    ++ed.sichtStand;
    if (!ed.sprungBericht) { return; }
    int stehen = 0, ducken = 0, macht = 0;
    for (const auto& p : ed.sprungBericht->punkte) { (p.stehen ? stehen : ducken) += 1; }
    for (const auto& s : ed.sprungBericht->spruenge) { macht += s.macht ? 1 : 0; }
    char t[400];
    std::snprintf(t, sizeof(t), trK(Str::KeSjFertig), stehen, ducken, static_cast<int>(ed.sprungBericht->kanten),
                  static_cast<int>(ed.sprungBericht->spruenge.size()) - macht, macht);
    ausgabe(t);
}

int sprungPruefungFortschritt() { return g_lauf.fortschritt.load(); }

void sprungPruefungBeenden() { fadenAbwarten(); }

void sprungLinien(std::vector<PluginLinien>& v) {
    const Editor& ed = e();
    if (!ed.sprungBericht) { return; }
    PluginLinien punkte;
    punkte.breite = 1.0F;
    const karte::Farbe gruen = karte::farbeAus(0.2F, 1.0F, 0.2F);
    const karte::Farbe gelb = karte::farbeAus(1.0F, 0.85F, 0.1F);
    for (const auto& p : ed.sprungBericht->punkte) {
        const karte::Vektor3f m(p.ort[0], p.ort[1], p.ort[2] - 23.0F);   // auf dem Boden (Ursprung - 24)
        const karte::Farbe f = p.stehen ? gruen : gelb;
        punkte.linien.push_back({m + karte::Vektor3f(-4, -4, 0), m + karte::Vektor3f(4, 4, 0), f});
        punkte.linien.push_back({m + karte::Vektor3f(-4, 4, 0), m + karte::Vektor3f(4, -4, 0), f});
    }
    v.push_back(std::move(punkte));
    PluginLinien bahnen;
    bahnen.breite = 2.0F;
    const karte::Farbe blau = karte::farbeAus(0.25F, 0.55F, 1.0F);
    const karte::Farbe violett = karte::farbeAus(0.85F, 0.3F, 1.0F);
    for (const auto& s : ed.sprungBericht->spruenge) {
        for (std::size_t i = 0; i + 1 < s.bahn.size(); ++i) {
            const karte::Vektor3f a(s.bahn[i][0], s.bahn[i][1], s.bahn[i][2] - 23.0F);
            const karte::Vektor3f b(s.bahn[i + 1][0], s.bahn[i + 1][1], s.bahn[i + 1][2] - 23.0F);
            bahnen.linien.push_back({a, b, s.macht ? violett : blau});
        }
    }
    v.push_back(std::move(bahnen));
}

// --- Stufe 9 Punkt 5: NPC-Wege ------------------------------------------------------------
//
// View > NPC paths: das Wegnetz der Wegpunkte (baueNavNetz wie beim Laden der Karte im Spiel;
// tuerkis, Sprungkanten orange) und ein Testlauf vom ersten zum letzten gewaehlten Entity
// (planeLauf: NAV::GoTo, STEER, Pmove gegen die Kollision der Brushes) - gelb, mit einem
// Kasten, der in Echtzeit die Bahn entlang laeuft.

namespace {

std::shared_ptr<NavWelt> npcWeltHolen() {
    Editor& ed = e();
    if (!ed.npcWelt || ed.npcWeltStand != geometrieStand()) {
        const auto& sp = ed.karteSpiel ? ed.karteSpiel : ed.spiel;
        const bool jk2 = sp && sp->spiel == kRegelwerkJediOutcast;
        auto k = std::make_shared<KollisionsKarte>();
        (void)karte::kollisionAusKarte(ed.karte, parmsVon, !jk2, *k);
        ed.npcWelt = karte::npcWelt(ed.karte, k);
        ed.npcWeltStand = geometrieStand();
    }
    return ed.npcWelt;
}

}  // namespace

void npcNetzUmschalten() {
    Editor& ed = e();
    ed.npcNetzZeigen = !ed.npcNetzZeigen;
    ++ed.sichtStand;
    if (!ed.npcNetzZeigen) { return; }
    const auto w = npcWeltHolen();
    int punkte = 0;
    for (const auto& en : ed.karte.entitaeten()) {
        const std::string k = en->wert("classname");
        punkte += (k == "waypoint" || k == "waypoint_small" || k == "waypoint_navgoal" || k == "point_combat") ? 1 : 0;
    }
    char t[300];
    const int kanten = w && w->netz ? w->netz->zahlKanten() : 0;
    if (punkte <= 3) {
        std::snprintf(t, sizeof(t), trK(Str::KeNwKeinNetz), punkte);
    } else {
        std::snprintf(t, sizeof(t), trK(Str::KeNwNetzInfo), w && w->netz ? static_cast<int>(w->netz->knoten.size()) - 1 : 0, kanten);
    }
    ausgabe(t);
}

bool npcTestlaufStarten() {
    Editor& ed = e();
    std::vector<NavVec> orte;
    for (const auto& a : ed.karte.auswahl.elemente()) {
        const auto ent = a.entitaet.lock();
        if (!ent || !a.primitiv.expired() || ent->wert("classname") == "worldspawn") { continue; }
        const karte::Vektor3f o = karte::entitaetUrsprung(*ent);
        orte.push_back({o[0], o[1], o[2]});
    }
    if (orte.size() < 2) {
        fehler(trK(Str::KeNwZweiWaehlen));
        return false;
    }
    const auto w = npcWeltHolen();
    ed.npcLauf = std::make_shared<LaufPlan>(karte::npcTestlauf(*w, orte.front(), orte.back(), ed.npcRennen));
    ed.npcLaufStart = ImGui::GetTime();
    ++ed.sichtStand;
    char t[300];
    const LaufPlan& p = *ed.npcLauf;
    if (p.ankunftMs > 0.0 && !p.nie) {
        std::snprintf(t, sizeof(t), trK(Str::KeNwAn), p.ankunftMs / 1000.0);
        ausgabe(t);
    } else {
        const NavVec o = p.zustaende.empty() ? orte.front() : p.zustaende.back().ort;
        const double ms = p.zustaende.empty() ? 0.0 : static_cast<double>(p.zustaende.size()) * 50.0;
        std::snprintf(t, sizeof(t), trK(Str::KeNwNichtAn), static_cast<double>(o[0]), static_cast<double>(o[1]), static_cast<double>(o[2]), ms / 1000.0);
        warnung(t);
    }
    return true;
}

void npcTestlaufLeeren() {
    e().npcLauf.reset();
    ++e().sichtStand;
}

void npcLinien(std::vector<PluginLinien>& v) {
    Editor& ed = e();
    const auto vek = [](const NavVec& p, float dz) { return karte::Vektor3f(p[0], p[1], p[2] + dz); };
    if (ed.npcNetzZeigen && ed.npcWelt && ed.npcWelt->netz) {
        const NavNetz& n = *ed.npcWelt->netz;
        PluginLinien netz;
        netz.breite = 2.0F;
        const karte::Farbe tuerkis = karte::farbeAus(0.1F, 0.9F, 0.9F);
        const karte::Farbe orange = karte::farbeAus(1.0F, 0.55F, 0.1F);
        for (std::size_t i = 1; i < n.kanten.size() && i < n.kanteBenutzt.size(); ++i) {
            if (!n.kanteBenutzt[i]) { continue; }
            const NavKante& k = n.kanten[i];
            if (k.a <= 0 || k.b <= 0 || static_cast<std::size_t>(k.a) >= n.knoten.size() || static_cast<std::size_t>(k.b) >= n.knoten.size()) { continue; }
            netz.linien.push_back({vek(n.knoten[static_cast<std::size_t>(k.a)].punkt, 0), vek(n.knoten[static_cast<std::size_t>(k.b)].punkt, 0),
                                   k.hat(NavKante::Springen) ? orange : tuerkis});
        }
        for (std::size_t i = 1; i < n.knoten.size(); ++i) {
            const karte::Vektor3f m = vek(n.knoten[i].punkt, 0);
            netz.linien.push_back({m + karte::Vektor3f(-6, 0, 0), m + karte::Vektor3f(6, 0, 0), tuerkis});
            netz.linien.push_back({m + karte::Vektor3f(0, -6, 0), m + karte::Vektor3f(0, 6, 0), tuerkis});
        }
        v.push_back(std::move(netz));
    }
    if (ed.npcLauf && !ed.npcLauf->zustaende.empty()) {
        const LaufPlan& p = *ed.npcLauf;
        PluginLinien bahn;
        bahn.breite = 2.0F;
        const karte::Farbe gelb = karte::farbeAus(1.0F, 0.95F, 0.2F);
        NavVec vorher = p.anfang.ort;
        for (const LaufZustand& z : p.zustaende) {
            bahn.linien.push_back({vek(vorher, -20), vek(z.ort, -20), gelb});
            vorher = z.ort;
        }
        // der Kasten der Figur, in Echtzeit entlang der Bahn (danach eine Sekunde Pause)
        const double ende = static_cast<double>(p.zustaende.size()) * 50.0 + 1000.0;
        const double t = std::fmod((ImGui::GetTime() - ed.npcLaufStart) * 1000.0, ende);
        const NavVec o = ortBei(p, t);
        const karte::Vektor3f lo(o[0] - 16, o[1] - 16, o[2] - 24), hi(o[0] + 16, o[1] + 16, o[2] + 40);
        const karte::Vektor3f ecken[8] = {{lo[0], lo[1], lo[2]}, {hi[0], lo[1], lo[2]}, {hi[0], hi[1], lo[2]}, {lo[0], hi[1], lo[2]},
                                          {lo[0], lo[1], hi[2]}, {hi[0], lo[1], hi[2]}, {hi[0], hi[1], hi[2]}, {lo[0], hi[1], hi[2]}};
        for (int i = 0; i < 4; ++i) {
            bahn.linien.push_back({ecken[i], ecken[(i + 1) % 4], gelb});
            bahn.linien.push_back({ecken[i + 4], ecken[(i + 1) % 4 + 4], gelb});
            bahn.linien.push_back({ecken[i], ecken[i + 4], gelb});
        }
        v.push_back(std::move(bahn));
    }
}

// --- Stufe 9 Punkt 6: verknuepfte Prefabs --------------------------------------------------

bool verbundKopieBefehl() {
    Editor& ed = e();
    std::set<std::string> namen;
    for (const auto& en : ed.karte.entitaeten()) {
        if (!en->wert(karte::kVerbundName).empty()) { namen.insert(en->wert(karte::kVerbundName)); }
    }
    std::string name;
    for (int i = 1; name.empty() || namen.count(name) != 0; ++i) { name = "link" + std::to_string(i); }
    // eine schon gewaehlte Verbund-Gruppe behaelt ihren Namen (verbundKopieAnlegen nimmt ihn)
    const float schritt = ed.raster.groesse() * 16.0F;
    const auto kopie = karte::verbundKopieAnlegen(ed.karte, name, karte::Vektor3f(schritt, 0, 0), ed.raster.groesse());
    if (!kopie) {
        fehler(trK(Str::KeVbNichts));
        return false;
    }
    ++ed.sichtStand;
    auswahlGeaendert();
    char t[300];
    std::snprintf(t, sizeof(t), trK(Str::KeVbFertig), kopie->wert(karte::kVerbundName).c_str(), static_cast<double>(schritt));
    ausgabe(t);
    return true;
}

bool verbundLoesenBefehl() {
    const bool ok = karte::verbundLoesen(e().karte);
    if (!ok) { fehler(trK(Str::KeVbNichts)); }
    return ok;
}

// --- Stufe 9 Punkt 7: Kurven-Werkzeug ------------------------------------------------------

bool kurveBefehl(karte::KurvenArt art) {
    Editor& ed = e();
    std::vector<karte::Vektor3f> kp;
    for (const auto& a : ed.karte.auswahl.elemente()) {
        const auto ent = a.entitaet.lock();
        if (!ent || !a.primitiv.expired() || ent->wert("classname") == "worldspawn") { continue; }
        kp.push_back(karte::entitaetUrsprung(*ent));
    }
    if (kp.size() < 2) {
        fehler(trK(Str::KeKuZweiWaehlen));
        return false;
    }
    karte::KurvenOptionen o;
    if (!ed.texBro.shader.empty()) { o.shader = ed.texBro.shader; }
    o.schritt = std::max(16.0F, ed.raster.groesse() * 8.0F);
    std::set<std::string> namen;
    for (const auto& en : ed.karte.entitaeten()) { namen.insert(en->wert("targetname")); }
    std::string praefix;
    for (int i = 1; praefix.empty() || namen.count(praefix + "1") != 0; ++i) {
        praefix = (art == karte::KurvenArt::Wegpunkte ? "kurvenwp" : "kurve") + std::to_string(i) + "_";
    }
    const std::size_t n = karte::kurveEinfuegen(ed.karte, kp, art, o, praefix);
    ++ed.sichtStand;
    auswahlGeaendert();
    char t[300];
    std::snprintf(t, sizeof(t), trK(Str::KeKuFertig), static_cast<int>(kp.size()), static_cast<int>(n));
    ausgabe(t);
    return n > 0;
}

// --- Stufe 9 Punkt 8: Modelle verteilen ----------------------------------------------------

bool streuBefehl(int anzahl) {
    Editor& ed = e();
    std::string modell;
    std::string klasse;
    karte::Vektor3f vorlageOrt;
    karte::Vektor3f mn(1e30F, 1e30F, 1e30F), mx(-1e30F, -1e30F, -1e30F);
    bool flaeche = false;
    for (const auto& a : ed.karte.auswahl.elemente()) {
        const auto ent = a.entitaet.lock();
        if (!ent) { continue; }
        const std::string k = ent->wert("classname");
        if (a.primitiv.expired() && (k == "misc_model" || k == "misc_model_static") && !ent->wert("model").empty() && modell.empty()) {
            modell = ent->wert("model");
            klasse = k;
            vorlageOrt = karte::entitaetUrsprung(*ent);
            continue;
        }
        const auto p = a.primitiv.lock();
        if (!p) { continue; }
        const karte::Kasten b = p->art() == karte::Primitiv::Art::Pinsel ? static_cast<const karte::Pinsel&>(*p).grenzen(ed.karte.kontext)
                                                                          : static_cast<const karte::Patch&>(*p).grenzen();
        for (std::size_t i = 0; i < 3; ++i) {
            mn[i] = std::min(mn[i], b.origin[i] - b.extents[i]);
            mx[i] = std::max(mx[i], b.origin[i] + b.extents[i]);
        }
        flaeche = true;
    }
    if (modell.empty()) {
        fehler(trK(Str::KeVtVorlage));
        return false;
    }
    if (!flaeche) {   // ohne gewaehlte Flaeche: 256 um die Vorlage
        mn = vorlageOrt - karte::Vektor3f(256, 256, 256);
        mx = vorlageOrt + karte::Vektor3f(256, 256, 256);
    }
    const auto& sp = ed.karteSpiel ? ed.karteSpiel : ed.spiel;
    const bool jk2 = sp && sp->spiel == kRegelwerkJediOutcast;
    if (klasse == "misc_model_static" && jk2) { klasse = "misc_model"; }   // Jedi Outcast kennt misc_model_static nicht
    auto k = std::make_shared<KollisionsKarte>();
    (void)karte::kollisionAusKarte(ed.karte, parmsVon, !jk2, *k);
    karte::StreuOptionen o;
    o.anzahl = anzahl;
    o.startwert = static_cast<unsigned>(ed.karte.undo.aenderungen() * 2654435761U + 17U);
    int erlaubt = anzahl;
    long long grenze = 0;
    if (klasse == "misc_model_static") {
        const karte::GrenzSpiel gs = karte::grenzSpielFuerRegelwerk(kartenSpielNummer());
        if (const karte::Grenze* g = karte::grenzeSuchen(gs, "laufzeit.miscents")) {
            grenze = g->erlaubt;
            erlaubt = static_cast<int>(std::max<long long>(0, g->erlaubt - karte::miscModelStaticZahl(ed.karte)));
        }
    }
    const karte::StreuErgebnis r = karte::streuOrte(*k, mn, mx, o, erlaubt);
    const std::size_t n = karte::streuEinfuegen(ed.karte, r, klasse, modell);
    ++ed.sichtStand;
    auswahlGeaendert();
    char t[400];
    std::snprintf(t, sizeof(t), trK(Str::KeVtFertig), static_cast<int>(n), klasse.c_str(), modell.c_str());
    ausgabe(t);
    if (r.begrenzt) {
        std::snprintf(t, sizeof(t), trK(Str::KeVtGrenze), static_cast<int>(grenze));
        warnung(t);
    }
    return n > 0;
}

}  // namespace bhed::gui::karteneditor
