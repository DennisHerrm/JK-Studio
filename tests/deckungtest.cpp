// deckungtest.cpp - rc609 H2: Kampfpunkt-Vorschlaege hinter Deckung
//
// wegDeckungGilt und wegDeckungVorschlaege (bhed/wegpunkte.h) auf kleiner Testgeometrie aus echten Brushes
// (kollisionsSpur, Welt wie tests/kampfpunkttest.cpp). Die Antworten stehen aus der Quelle fest
// (NPC_FindCombatPoint mit CP_CLEAR|CP_COVER, OpenJK code/game/NPC_combat.cpp:2721-2899; Jedi Outcast
// codeJK2/game/NPC_combat.cpp:2489-2700):
//   niedrige Wand (48)    origin (25) sieht weder Ursprung (24) noch Kopf (60) des Feindes, die Muendung (60) den Kopf
//   im Freien             CP_COVER scheitert
//   hohe Wand (100)       CP_CLEAR scheitert (die Muendung sieht nichts)
//   zu nah, zu weit       avoidDist 200 (ST_Commander), visrange 1024
//   im Festen             der Kasten der Figur steckt in der Wand
//   Jedi Outcast          CP_CLEAR vom origin aus: die Wand dicht vor dem Feind laesst den Kopf frei - dort gilt die
//                         Regel von Jedi Outcast, die von Jedi Academy nicht (der origin sieht den Kopf)
// Die Vorschlaege: jeder erfuellt die Regel gegen seine Feindstelle, Abstand und Hoechstzahl, nur im erlaubten Bereich,
// Abbrechen liefert nichts, vier Faeden rechnen dasselbe wie einer.
#include "bhed/kollision.h"
#include "bhed/navlauf.h"
#include "bhed/wegpunkte.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

using namespace bhed;

namespace {

int gFehler = 0;
void pruefe(bool ok, const std::string& was) {
    std::printf("%s %s\n", ok ? "ok  " : "FEHL", was.c_str());
    if (!ok) { ++gFehler; }
}

constexpr std::uint32_t kOpak = 0x00008000U;   // CONTENTS_OPAQUE

struct Welt {
    KollisionsKarte k;
    Welt() {
        k.knoten.resize(1);
        k.modelle.resize(2);
    }
    int ebene(float nx, float ny, float nz, float d) {
        KollisionsKarte::Ebene e;
        e.normale[0] = nx;
        e.normale[1] = ny;
        e.normale[2] = nz;
        e.abstand = d;
        int achse = -1;
        if (nx == 1.0F && ny == 0.0F && nz == 0.0F) { achse = 0; }
        if (nx == 0.0F && ny == 1.0F && nz == 0.0F) { achse = 1; }
        if (nx == 0.0F && ny == 0.0F && nz == 1.0F) { achse = 2; }
        e.typ = achse >= 0 ? achse : 3;
        e.vorzeichen = (nx < 0.0F ? 1 : 0) | (ny < 0.0F ? 2 : 0) | (nz < 0.0F ? 4 : 0);
        k.ebenen.push_back(e);
        return static_cast<int>(k.ebenen.size()) - 1;
    }
    // Ein fester, undurchsichtiger Brush (wie eine Wand mit gewoehnlichem Shader: CONTENTS_SOLID | CONTENTS_OPAQUE).
    void kasten(const NavVec& mi, const NavVec& ma) {
        KollisionsKarte::Pinsel p;
        p.ersteSeite = static_cast<int>(k.seiten.size());
        p.seiten = 6;
        p.inhalt = kNavSolid | kOpak;
        for (int a = 0; a < 3; ++a) {
            p.mins[a] = mi[static_cast<std::size_t>(a)];
            p.maxs[a] = ma[static_cast<std::size_t>(a)];
        }
        for (const int s : {ebene(1, 0, 0, ma[0]), ebene(-1, 0, 0, -mi[0]), ebene(0, 1, 0, ma[1]), ebene(0, -1, 0, -mi[1]), ebene(0, 0, 1, ma[2]),
                            ebene(0, 0, -1, -mi[2])}) {
            k.seiten.push_back(KollisionsKarte::Seite{s, -1});
        }
        k.pinsel.push_back(p);
        k.modelle[1].erster = 0;
        k.modelle[1].zahl = static_cast<int>(k.pinsel.size());
    }
    [[nodiscard]] WegWelt welt() const {
        WegWelt w;
        const KollisionsKarte* kp = &k;
        w.spur = [kp](const std::array<float, 3>& a, const std::array<float, 3>& mi, const std::array<float, 3>& ma, const std::array<float, 3>& b) {
            const KollisionsSpur s = kollisionsSpur(*kp, a.data(), mi.data(), ma.data(), b.data(), kNavMaskNpcSolid, 1);
            WegSpurTreffer t;
            t.anteil = s.fraction;
            t.imFesten = s.startsolid || s.allsolid;
            t.ende = s.endpos;
            t.normale = s.normale;
            return t;
        };
        w.sicht = [kp](const std::array<float, 3>& a, const std::array<float, 3>& b) {
            const float null[3] = {0.0F, 0.0F, 0.0F};
            return kollisionsSpur(*kp, a.data(), null, null, b.data(), kOpak, 1).fraction >= 1.0F;
        };
        return w;
    }
};

using V = std::array<float, 3>;

// Boden 2048 x 2048 (Oberkante 0), eine Wand bei x 96..112, y -128..128 bis zur Hoehe `hoehe`.
Welt wand(float x0, float hoehe) {
    Welt w;
    w.kasten({-1024.0F, -1024.0F, -16.0F}, {1024.0F, 1024.0F, 0.0F});
    w.kasten({x0, -128.0F, 0.0F}, {x0 + 16.0F, 128.0F, hoehe});
    return w;
}

void regel() {
    const Welt niedrig = wand(96.0F, 48.0F);
    const WegWelt wn = niedrig.welt();
    const WegKastenFn kasten = [&wn](const V& o) { return !wn.spur(o, {-16.0F, -16.0F, -24.0F}, {16.0F, 16.0F, 40.0F}, o).imFesten; };
    const WegDeckungRegeln jka;
    const V ort{0.0F, 0.0F, 25.125F};
    pruefe(wegDeckungGilt(wn.sicht, kasten, ort, {400.0F, 0.0F, 24.0F}, jka), "niedrige Wand: geduckt verdeckt, die Muendung sieht den Kopf - gilt");
    pruefe(!wegDeckungGilt(wn.sicht, kasten, {0.0F, 400.0F, 25.125F}, {400.0F, 400.0F, 24.0F}, jka), "im Freien: CP_COVER scheitert");
    pruefe(!wegDeckungGilt(wn.sicht, kasten, ort, {180.0F, 0.0F, 24.0F}, jka), "180 vom Feind: avoidDist 200 (CP_AVOID_ENEMY)");
    WegDeckungRegeln ohneMeiden = jka;
    ohneMeiden.meiden = 0.0F;
    pruefe(wegDeckungGilt(wn.sicht, kasten, ort, {180.0F, 0.0F, 24.0F}, ohneMeiden), "dasselbe ohne avoidDist: gilt");
    pruefe(!wegDeckungGilt(wn.sicht, kasten, ort, {1030.0F, 0.0F, 24.0F}, jka), "1030 vom Feind: ueber visrange 1024 (CP_CLEAR)");
    pruefe(!wegDeckungGilt(wn.sicht, kasten, {104.0F, 0.0F, 25.125F}, {400.0F, 0.0F, 24.0F}, jka), "der Kasten steckt in der Wand");

    const Welt hoch = wand(96.0F, 100.0F);
    const WegWelt wh = hoch.welt();
    pruefe(!wegDeckungGilt(wh.sicht, kasten, ort, {400.0F, 0.0F, 24.0F}, jka), "hohe Wand: die Muendung sieht nichts (CP_CLEAR)");

    // Die Wand dicht vor dem Feind (x 370..386): der origin sieht den Kopf (57 > 48 an der Wand), nicht den Ursprung.
    const Welt nah = wand(370.0F, 48.0F);
    const WegWelt wz = nah.welt();
    WegDeckungRegeln jk2;
    jk2.jk2 = true;
    jk2.meiden = 0.0F;
    jk2.sammeln = 512.0F;
    pruefe(!wegDeckungGilt(wz.sicht, kasten, ort, {400.0F, 0.0F, 24.0F}, jka), "Wand vor dem Feind, Jedi Academy: der origin sieht den Kopf - CP_COVER scheitert");
    pruefe(wegDeckungGilt(wz.sicht, kasten, ort, {400.0F, 0.0F, 24.0F}, jk2), "dasselbe in Jedi Outcast: CP_COVER nur der Ursprung, CP_CLEAR der Kopf - gilt");
    pruefe(!wegDeckungGilt(wn.sicht, kasten, ort, {400.0F, 0.0F, 24.0F}, jk2), "niedrige Wand bei 96, Jedi Outcast: der origin sieht den Kopf nicht (CP_CLEAR vom origin)");
    pruefe(!wegDeckungGilt(wz.sicht, kasten, {-200.0F, 0.0F, 25.125F}, {400.0F, 0.0F, 24.0F}, jk2), "Jedi Outcast 600 vom Feind: ausserhalb CP_COLLECT_RADIUS 512");
}

std::string text(const WegDeckungErgebnis& e) {
    std::string s;
    for (const WegDeckung& d : e.punkte) {
        char b[160];
        std::snprintf(b, sizeof(b), "(%g %g %g: %d)", static_cast<double>(d.ort[0]), static_cast<double>(d.ort[1]), static_cast<double>(d.ort[2]), d.feinde);
        s += b;
    }
    return s;
}

void vorschlaege() {
    const Welt niedrig = wand(96.0F, 48.0F);
    const WegWelt w = niedrig.welt();
    const WegKastenFn kasten = [&w](const V& o) { return !w.spur(o, {-16.0F, -16.0F, -24.0F}, {16.0F, 16.0F, 40.0F}, o).imFesten; };
    const std::vector<V> starts{{-200.0F, 0.0F, 25.0F}};
    const std::vector<char> istWeg{1};
    WegDeckungWunsch wu;
    wu.reichweite = 2048.0F;
    wu.abstand = 96.0F;
    wu.hoechstens = 8;
    const WegDeckungErgebnis e = wegDeckungVorschlaege(starts, istWeg, w, wu);
    std::printf("     %zu Kandidaten, %zu Feindstellen, %zu mit Deckung, %zu Vorschlaege %s\n", e.kandidaten, e.feindStellen, e.mitDeckung,
                e.punkte.size(), text(e).c_str());
    pruefe(!e.punkte.empty() && e.punkte.size() <= 8U, "Vorschlaege entstehen, hoechstens 8");
    int falsch = 0;
    int eng = 0;
    for (std::size_t i = 0; i < e.punkte.size(); ++i) {
        const WegDeckung& d = e.punkte[i];
        const V o{d.ort[0], d.ort[1], d.ort[2] + 0.125F};
        falsch += wegDeckungGilt(w.sicht, kasten, o, d.feind, wu.regeln) ? 0 : 1;
        falsch += (std::fabs(d.ort[2] - 25.0F) > 0.5F || d.feinde < 1) ? 1 : 0;
        for (std::size_t j = 0; j < i; ++j) {
            const float dd = std::hypot(std::hypot(d.ort[0] - e.punkte[j].ort[0], d.ort[1] - e.punkte[j].ort[1]), d.ort[2] - e.punkte[j].ort[2]);
            eng += dd < wu.abstand ? 1 : 0;
        }
        if (i > 0) { falsch += d.feinde > e.punkte[i - 1].feinde ? 1 : 0; }
    }
    pruefe(falsch == 0, "jeder Vorschlag erfuellt die Regel gegen seine Feindstelle, Boden + 25, die meisten Feindstellen zuerst");
    pruefe(eng == 0, "Vorschlaege mindestens `abstand` auseinander");

    WegDeckungWunsch vier = wu;
    vier.faeden = 4;
    pruefe(text(wegDeckungVorschlaege(starts, istWeg, w, vier)) == text(e), "vier Faeden rechnen dasselbe wie einer");

    WegDeckungWunsch bereich = wu;
    bereich.erlaubt = [](const V& b) { return b[1] > 0.0F; };
    const WegDeckungErgebnis eb = wegDeckungVorschlaege(starts, istWeg, w, bereich);
    int aussen = 0;
    for (const WegDeckung& d : eb.punkte) { aussen += d.boden[1] > 0.0F ? 0 : 1; }
    pruefe(!eb.punkte.empty() && aussen == 0, "nur im erlaubten Bereich (y > 0)");

    WegDeckungWunsch belegt = wu;
    belegt.vorhanden = {e.punkte.front().ort};
    const WegDeckungErgebnis ev = wegDeckungVorschlaege(starts, istWeg, w, belegt);
    int zuNah = 0;
    for (const WegDeckung& d : ev.punkte) {
        zuNah += std::hypot(d.ort[0] - e.punkte.front().ort[0], d.ort[1] - e.punkte.front().ort[1]) < wu.abstand ? 1 : 0;
    }
    pruefe(zuNah == 0, "Abstand auch zu einem vorhandenen Kampfpunkt");

    WegDeckungWunsch ab = wu;
    ab.weiter = [](float) { return false; };
    const WegDeckungErgebnis ea = wegDeckungVorschlaege(starts, istWeg, w, ab);
    pruefe(ea.abgebrochen && ea.punkte.empty(), "Abbrechen: nichts");

    const std::vector<char> keinWeg{0};
    pruefe(wegDeckungVorschlaege(starts, keinWeg, w, wu).punkte.empty(), "ohne Wegpunkt kein Kandidat (nahe dem Netz)");
}

}  // namespace

int main() {
    std::printf("== deckung: Regel ==\n");
    regel();
    std::printf("== deckung: Vorschlaege ==\n");
    vorschlaege();
    std::printf("%s (%d Fehler)\n", gFehler == 0 ? "ALLES OK" : "FEHLER", gFehler);
    return gFehler == 0 ? 0 : 1;
}
