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
#include "bhed/karte/sprungpruefung.h"

#include <atomic>
#include <cstdio>
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

}  // namespace bhed::gui::karteneditor
