// modell_uv.cpp - Modell-Block 1b: UV erstellen (bhed/modell/wof/uv.h, bhed/modell/md3datei.h)
//
// Nach dem Vorbild der Tests von WhiteoutLib (tests/wem_uv_{islands,flatten,layout,seams,checks}_test.cpp) und
// WhiteoutFlakes (tests/uv_tools_test.cpp), auf eigenen Netzen und dem JKA-Modell data/fixtures/model.glm:
//   Inseln     Schnitte der Datei werden Marken; Keile je Ort; geschlossene Insel; Ringe
//   Abflachen  Ebene: LSCM winkel- und flaechentreu; Wuerfel: schliessende Naht, dann ohne Kippen; Pins halten;
//              Begradigen auf die Waagrechte; Entspannen senkt die Dehnung
//   Anordnen   Kasten-Projektion 6 Inseln; Packen ohne Ueberlappung in der Kachel; Stapel ueber den Spiegel
//   Naehte     Faltung der Rollen wie WL foldOf; Ring aufschneiden; Winkel; Form (xatlas)
//   JKA        Kopf und Torso des Modells abgewickelt: Verzerrung (Winkel, Flaeche) und Texel-Dichte gemessen;
//              Rundlauf GLM ohne Aenderung Byte fuer Byte (jede LOD-Stufe); UV -> GLM -> UV gleich; die geteilten Ecken
//              behalten Knochen, Gewichte und Gewichtsfelder; ueber 1000 Ecken wird nichts geschrieben
//   MD3        Rundlauf ohne Aenderung Byte fuer Byte; geteilte Ecken behalten Ort und Normale in jedem Bild
//
// Aufruf: modelluvtest <model.glm>
#include "bhed/glm.h"
#include "bhed/md3.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/glmdatei.h"
#include "bhed/modell/md3datei.h"
#include "bhed/modell/wof/uv.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", what.c_str());
    if (!ok) { ++fails; }
}
std::string z(double v) {
    char b[40];
    std::snprintf(b, sizeof b, "%.4f", v);
    return b;
}
std::string slurp(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}
using namespace bhed;
using namespace bhed::modell;
using namespace bhed::modell::wof;
std::size_t uz(int i) { return static_cast<std::size_t>(i); }

Ecke ecke(float x, float y, float zz, float u, float v, int knochen = 0) {
    Ecke e;
    e.ort[0] = x;
    e.ort[1] = y;
    e.ort[2] = zz;
    e.normale[2] = 1.0F;
    e.uv[0] = u;
    e.uv[1] = v;
    e.knochen[0] = static_cast<std::uint8_t>(knochen);
    e.gewicht[0] = 1.0F;
    return e;
}
Inhalt inhaltAus(std::vector<Ecke> e, const std::vector<std::uint32_t>& idx, const char* name = "flaeche") {
    Inhalt in;
    in.herkunft = Herkunft::Glm;
    in.stufenZahl = 1;
    Teil t;
    t.name = name;
    t.stufen.resize(1);
    for (std::size_t i = 0; i < e.size(); ++i) { e[i].dateiIndex = static_cast<std::int32_t>(i); }
    t.stufen[0].aufbauen(std::move(e), idx);
    in.teile.push_back(std::move(t));
    return in;
}
// Gitter n x n Vierecke in der Ebene z = 0 (Seitenlaenge 1), UV = Ort * 0,05
Inhalt gitter(int n, float versatzX = 0.0F) {
    std::vector<Ecke> e;
    std::vector<std::uint32_t> idx;
    for (int j = 0; j <= n; ++j) {
        for (int i = 0; i <= n; ++i) { e.push_back(ecke(versatzX + static_cast<float>(i), static_cast<float>(j), 0.0F, static_cast<float>(i) * 0.05F, static_cast<float>(j) * 0.05F)); }
    }
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            const auto v = [&](int a, int b) { return static_cast<std::uint32_t>(b * (n + 1) + a); };
            idx.insert(idx.end(), {v(i, j), v(i + 1, j), v(i + 1, j + 1), v(i, j), v(i + 1, j + 1), v(i, j + 1)});
        }
    }
    return inhaltAus(std::move(e), idx);
}
// geschlossener Wuerfel mit 8 Ecken (alle UV 0)
Inhalt wuerfel() {
    std::vector<Ecke> e;
    for (int k = 0; k < 8; ++k) { e.push_back(ecke((k & 1) != 0 ? 1.0F : -1.0F, (k & 2) != 0 ? 1.0F : -1.0F, (k & 4) != 0 ? 1.0F : -1.0F, 0.0F, 0.0F)); }
    const std::vector<std::uint32_t> idx = {0, 2, 3, 0, 3, 1, 4, 5, 7, 4, 7, 6, 0, 1, 5, 0, 5, 4, 2, 6, 7, 2, 7, 3, 0, 4, 6, 0, 6, 2, 1, 3, 7, 1, 7, 5};
    return inhaltAus(std::move(e), idx, "wuerfel");
}
// Rohr: offen an beiden Enden, `rund` Ecken je Ring, `hoch` Ringe; Knochen 1 unten, 2 oben (fuer die Naehte aus dem Skelett)
Inhalt rohr(int rund, int hoch) {
    std::vector<Ecke> e;
    std::vector<std::uint32_t> idx;
    for (int j = 0; j < hoch; ++j) {
        for (int i = 0; i < rund; ++i) {
            const float w = 6.2831853F * static_cast<float>(i) / static_cast<float>(rund);
            Ecke x = ecke(std::cos(w), std::sin(w), static_cast<float>(j) * 0.5F, 0.0F, 0.0F, j * 2 < hoch ? 1 : 2);
            x.normale[0] = std::cos(w);
            x.normale[1] = std::sin(w);
            x.normale[2] = 0.0F;
            e.push_back(x);
        }
    }
    for (int j = 0; j + 1 < hoch; ++j) {
        for (int i = 0; i < rund; ++i) {
            const auto v = [&](int a, int b) { return static_cast<std::uint32_t>(b * rund + (a % rund)); };
            idx.insert(idx.end(), {v(i, j), v(i + 1, j), v(i + 1, j + 1), v(i, j), v(i + 1, j + 1), v(i, j + 1)});
        }
    }
    return inhaltAus(std::move(e), idx, "rohr");
}
UvNetz netzVon(const Inhalt& in) {
    std::vector<int> t;
    for (std::size_t i = 0; i < in.teile.size(); ++i) { t.push_back(static_cast<int>(i)); }
    return netzAus(in, t, 0);
}
UvAuswahl alles(const UvNetz& n, UvEbene e = UvEbene::Flaeche) {
    UvAuswahl a;
    a.ebene = e;
    a.ecke.assign(uz(n.kanten()), 1);
    return a;
}
bool allesInKachel(const UvNetz& n) {
    for (const V2& w : n.uv) {
        if (w.x < -1e-4F || w.y < -1e-4F || w.x > 1.0001F || w.y > 1.0001F) { return false; }
    }
    return true;
}

void testInseln() {
    std::printf("Inseln\n");
    // Gitter, rechte Haelfte der UV verschoben: an der Mitte ein Schnitt der Datei
    Inhalt in = gitter(4);
    Netz& nz = in.teile[0].stufen[0];
    std::vector<Ecke> e = nz.ecken;
    std::vector<std::uint32_t> idx = nz.indizes();
    // die Ecken mit x >= 2 doppeln (rechts eigene Ecken mit verschobener UV)
    std::vector<int> neuVon(e.size(), -1);
    for (std::size_t d = 0; d < idx.size() / 3; ++d) {
        float mx = 0.0F;
        for (int k = 0; k < 3; ++k) { mx += e[idx[d * 3 + uz(k)]].ort[0] / 3.0F; }
        if (mx < 2.0F) { continue; }
        for (int k = 0; k < 3; ++k) {
            std::uint32_t& v = idx[d * 3 + uz(k)];
            if (e[v].ort[0] > 2.0F - 1e-6F && e[v].ort[0] < 2.0F + 1e-6F) {
                if (neuVon[v] < 0) {
                    Ecke c = e[v];
                    c.uv[0] += 0.5F;
                    neuVon[v] = static_cast<int>(e.size());
                    e.push_back(c);
                }
                v = static_cast<std::uint32_t>(neuVon[v]);
            } else if (e[v].ort[0] > 2.0F) {
                e[v].uv[0] += 0.5F;
            }
        }
    }
    Inhalt in2 = inhaltAus(e, idx);
    const UvNetz n = netzVon(in2);
    const UvInseln i = inselnBauen(n);
    expect("Schnitt der Datei: 2 Inseln (" + std::to_string(i.zahl) + ")", i.zahl == 2);
    const int marken = static_cast<int>(std::count(n.naht.begin(), n.naht.end(), u8{1}));
    const int datei = static_cast<int>(std::count(n.dateiNaht.begin(), n.dateiNaht.end(), u8{1}));
    expect("die Schnitte der Datei wurden Marken und Datei-Marken (WL EnsureUvSet): 4 Kanten x 2 Seiten = " + std::to_string(marken), marken == 8 && datei == 8);
    expect("beide Inseln offen, je eine Randschleife", i.geschlossen[0] == 0 && i.geschlossen[1] == 0 && i.schleifen[0] == 1 && i.schleifen[1] == 1);
    expect("Keile: 25 Punkte + 5 an der Naht = " + std::to_string(i.keilZahl), i.keilZahl == 30);
    const UvInseln w = inselnBauen(netzVon(wuerfel()));
    expect("Wuerfel ohne UV: eine geschlossene Insel", w.zahl == 1 && w.geschlossen[0] == 1);
    const UvInseln r = inselnBauen(netzVon(rohr(12, 5)));
    expect("Rohr: ein Ring mit zwei Schleifen", r.zahl == 1 && r.schleifen[0] == 2);
}

void testAbflachen() {
    std::printf("Abflachen\n");
    // Ebene mit verzogener UV (u = x^2): LSCM legt sie winkeltreu und flaechentreu
    Inhalt in = gitter(6);
    for (Ecke& x : in.teile[0].stufen[0].ecken) { x.uv[0] = x.ort[0] * x.ort[0] * 0.02F; }
    UvNetz n = netzVon(in);
    UvInseln i = inselnBauen(n);
    const Verzerrung vor = messen(n, i, 1024);
    const AbflachErgebnis e = lscm(n, i, 0);
    const Verzerrung nach = messen(n, i, 1024);
    expect("Ebene: verzogen Winkel " + z(vor.winkelGrad) + " Grad -> LSCM " + z(nach.winkelGrad) + " (< 0,01), Flaeche " + z(nach.flaeche) + " (< 0,001)",
           e.ok() && nach.winkelGrad < 0.01F && nach.flaeche < 1e-3F && vor.winkelGrad > 1.0F);
    // Pins halten
    n.pin[0] = 1;
    n.pin[uz(n.kanten() - 1)] = 1;
    const V2 a = n.uv[0];
    const V2 b = n.uv[uz(n.kanten() - 1)];
    i = inselnBauen(n);
    for (int h = 0; h < n.kanten(); ++h) {
        if (i.keil(h) == i.keil(0) || i.keil(h) == i.keil(n.kanten() - 1)) { n.pin[uz(h)] = 1; }
    }
    (void)minimaleDehnung(n, i, 0);
    expect("Pins halten ihre UV Bit fuer Bit (minimale Dehnung)", std::memcmp(&n.uv[0], &a, 8) == 0 && std::memcmp(&n.uv[uz(n.kanten() - 1)], &b, 8) == 0);
    // Wuerfel: abwickeln gibt ihm eine schliessende Naht und legt ihn ohne Kippen
    UvNetz w = netzVon(wuerfel());
    UvKontext k;
    const UvAntwort ant = befehlNeuLoesen(w, UvAuswahl{}, k);
    const UvInseln wi = inselnBauen(w);
    const Verzerrung wv = messen(w, wi, 1024);
    expect("Wuerfel: Naht an der verdeckten Seite (Meldung Geschlossen), offen, " + std::to_string(wv.gekippt) + " gekippt, Winkel " + z(wv.winkelGrad),
           ant.meldung == UvMeldung::Geschlossen && wi.geschlossen[0] == 0 && wv.gekippt == 0);
    // Begradigen: die untere Reihe eines leicht verbogenen Gitters auf die Waagrechte
    Inhalt g = gitter(4);
    for (Ecke& x : g.teile[0].stufen[0].ecken) { x.uv[1] += x.ort[0] * 0.004F + (x.ort[1] == 0.0F ? 0.003F * std::sin(x.ort[0] * 2.0F) : 0.0F); }
    UvNetz gn = netzVon(g);
    UvAuswahl unten;
    unten.ebene = UvEbene::Kante;
    unten.ecke.assign(uz(gn.kanten()), 0);
    for (int h = 0; h < gn.kanten(); ++h) { unten.ecke[uz(h)] = gn.ortVon(h).y == 0.0F ? 1 : 0; }
    (void)befehlBegradigen(gn, unten);
    float spanne = 0.0F;
    float v0 = -1.0F;
    for (int h = 0; h < gn.kanten(); ++h) {
        if (gn.ortVon(h).y != 0.0F) { continue; }
        if (v0 < 0.0F) { v0 = gn.uv[uz(h)].y; }
        spanne = std::max(spanne, std::abs(gn.uv[uz(h)].y - v0));
    }
    expect("Begradigen: die Reihe liegt waagrecht (Abweichung " + z(spanne) + ")", spanne < 1e-6F);
    // Entspannen senkt die Dehnung
    Inhalt r = gitter(6);
    for (Ecke& x : r.teile[0].stufen[0].ecken) {
        if (x.ort[0] > 0.0F && x.ort[0] < 6.0F && x.ort[1] > 0.0F && x.ort[1] < 6.0F) { x.uv[0] += 0.012F * std::sin(x.ort[1] * 3.0F); }
    }
    UvNetz rn = netzVon(r);
    const float d0 = messen(rn, inselnBauen(rn), 1024).flaeche;
    (void)befehlEntspannen(rn, alles(rn), 10);
    const float d1 = messen(rn, inselnBauen(rn), 1024).flaeche;
    expect("Entspannen: Flaechenverzerrung " + z(d0) + " -> " + z(d1), d1 < d0);
}

void testAnordnen() {
    std::printf("Anordnen\n");
    UvNetz w = netzVon(wuerfel());
    UvKontext k;
    (void)befehlProjizieren(w, alles(w), Form::Kasten, 3, k);
    UvInseln i = inselnBauen(w);
    expect("Kasten-Projektion: 6 Inseln (" + std::to_string(i.zahl) + ")", i.zahl == 6);
    (void)befehlPacken(w, alles(w), PackUmfang::Alles, k);
    i = inselnBauen(w);
    PruefOptionen po;
    po.kacheln = false;
    const UvBefunde b = pruefen(w, i, po);
    expect("Packen: keine Ueberlappung (" + std::to_string(b.ueberlappt.size()) + "), alles in der Kachel", b.ueberlappt.empty() && b.aussen.empty() && allesInKachel(w));
    // zwei Gitter uebereinander (verschiedene Formen): nach dem Packen getrennt
    Inhalt zwei = gitter(3);
    {
        Inhalt b2 = gitter(5);
        for (Ecke& x : b2.teile[0].stufen[0].ecken) {
            x.ort[2] = 5.0F;
            x.uv[0] *= 0.7F;
        }
        b2.teile[0].name = "zwei";
        zwei.teile.push_back(b2.teile[0]);
    }
    UvNetz zn = netzVon(zwei);
    const UvBefunde vorher = pruefen(zn, inselnBauen(zn), po);
    (void)befehlPacken(zn, UvAuswahl{}, PackUmfang::Alles, k);
    const UvBefunde nachher = pruefen(zn, inselnBauen(zn), po);
    expect("zwei Flaechen einer Textur: vorher " + std::to_string(vorher.ueberlappt.size()) + " ueberlappend, nachher " + std::to_string(nachher.ueberlappt.size()),
           vorher.ueberlappt.size() == 2U && nachher.ueberlappt.empty() && allesInKachel(zn));
    // ein Splitter (ein Dreieck, fast ohne UV-Flaeche, aber lang) quer ueber einem Gitter: nach dem Packen liegt er frei
    // (Selbsttest uv_a2: der Abdruck von WL footprintOf war fuer ihn leer, er blieb liegen)
    {
        Inhalt g = gitter(3);
        Teil splitter;
        splitter.name = "splitter";
        splitter.stufen.resize(1);
        std::vector<Ecke> se = {ecke(0.0F, 0.0F, 9.0F, 0.02F, 0.02F), ecke(3.0F, 0.0F, 9.0F, 0.14F, 0.14F), ecke(3.0F, 0.01F, 9.0F, 0.14F, 0.1401F)};
        for (std::size_t q = 0; q < se.size(); ++q) { se[q].dateiIndex = static_cast<std::int32_t>(q); }
        splitter.stufen[0].aufbauen(std::move(se), {0, 1, 2});
        g.teile.push_back(splitter);
        UvNetz gn = netzVon(g);
        PruefOptionen p2;
        p2.kacheln = false;
        const UvBefunde vor = pruefen(gn, inselnBauen(gn), p2);
        (void)befehlPacken(gn, UvAuswahl{}, PackUmfang::Alles, k);
        const UvBefunde nach = pruefen(gn, inselnBauen(gn), p2);
        expect("Splitter-Dreieck quer ueber einer Insel: vorher " + std::to_string(vor.ueberlappt.size()) + " ueberlappend, nach dem Packen " +
                   std::to_string(nach.ueberlappt.size()),
               vor.ueberlappt.size() == 2U && nach.ueberlappt.empty());
    }
    // Stapel ueber den Spiegel: rechtes Gitter und sein Spiegelbild (x -> -x, Umlauf gedreht), andere UV
    Inhalt sp = gitter(3, 1.0F);
    {
        Teil links = sp.teile[0];
        std::vector<Ecke> e = links.stufen[0].ecken;
        std::vector<std::uint32_t> idx = links.stufen[0].indizes();
        for (Ecke& x : e) {
            x.ort[0] = -x.ort[0];
            x.uv[0] += 0.6F;
        }
        for (std::size_t d = 0; d + 2 < idx.size(); d += 3) { std::swap(idx[d + 1], idx[d + 2]); }
        links.stufen[0].aufbauen(std::move(e), idx);
        links.name = "links";
        sp.teile.push_back(links);
    }
    UvNetz sn = netzVon(sp);
    k.spiegel = punktSpiegel(sn, 0.01F);
    UvAuswahl rechts;
    rechts.ebene = UvEbene::Insel;
    rechts.ecke.assign(uz(sn.kanten()), 0);
    for (int h = 0; h < sn.kanten(); ++h) { rechts.ecke[uz(h)] = sn.teil[uz(h)] == 0 ? 1 : 0; }
    {
        UvNetz direkt = sn;
        const UvInseln di = inselnBauen(direkt);
        const StapelErgebnis se = stapeln(direkt, di, di.inselVonDreieck(0), di.inselVonDreieck(direkt.dreiecke() - 1), k.spiegel);
        expect("Stapel ueber den Punktspiegel gepaart (nicht Seite an Seite)", se.ok() && se.ueberSpiegel);
    }
    const UvAntwort st = befehlStapelnGespiegelt(sn, rechts, k);
    int ungleich = 0;
    for (int h = 0; h < sn.kanten(); ++h) {
        if (sn.teil[uz(h)] != 1) { continue; }
        const int p = k.spiegel[uz(sn.von[uz(h)])];
        for (int g = 0; g < sn.kanten(); ++g) {
            if (sn.von[uz(g)] == p && std::memcmp(&sn.uv[uz(g)], &sn.uv[uz(h)], 8) != 0) { ++ungleich; }
        }
    }
    const UvBefunde sb = pruefen(sn, inselnBauen(sn), po);
    expect("Stapeln gespiegelt: " + std::to_string(st.zahl) + " Stapel, jede Ecke links = ihr Spiegel rechts (" + std::to_string(ungleich) + " ungleich), als Stapel erkannt",
           st.zahl == 1 && ungleich == 0 && sb.stapel == 1 && sb.ueberlappt.empty());
    // Dichte angleichen: zwei Gitter mit verschiedener Dichte -> gleich
    UvNetz dn = netzVon(zwei);
    UvInseln di = inselnBauen(dn);
    (void)befehlDichteAngleichen(dn, UvAuswahl{}, k);
    di = inselnBauen(dn);
    const float a0 = texelDichte(dn, di, 0, 1024);
    const float a1 = texelDichte(dn, di, 1, 1024);
    expect("Dichte angleichen: " + z(a0) + " und " + z(a1) + " Texel je Einheit", std::abs(a0 - a1) < 1e-3F * a0);
}

void testNaehte() {
    std::printf("Naehte\n");
    // Faltung wie WL foldOf
    const std::vector<Rolle> r = {Rolle::Becken, Rolle::Wirbel, Rolle::Wirbel, Rolle::Hals, Rolle::Kopf, Rolle::Schluessel,
                                  Rolle::Oberarm, Rolle::OberarmTwist, Rolle::Unterarm, Rolle::Hand, Rolle::Finger};
    const std::vector<int> e = {-1, 0, 1, 2, 3, 2, 5, 6, 6, 8, 9};
    bool glieder = false;
    const std::vector<int> f = knochenFaltung(r, e, &glieder);
    const std::vector<int> soll = {0, 1, 1, 1, 4, 1, 6, 6, 8, 9, 9};
    expect("Faltung der Knochen (Wirbelkette, Hals, Schluessel, Twist, Finger fallen in den Elternknochen), Glieder erkannt", f == soll && glieder);
    // Rohr: Ring aufschneiden, dann eine Insel ohne Ring
    UvNetz n = netzVon(rohr(16, 6));
    UvKontext k;
    (void)befehlRingAufschneiden(n, UvAuswahl{}, k);
    (void)befehlNeuLoesen(n, UvAuswahl{}, k);
    UvInseln i = inselnBauen(n);
    const Verzerrung v = messen(n, i, 1024);
    expect("Rohr: aufgeschnitten und abgewickelt - 1 Insel, 1 Schleife, Winkel " + z(v.winkelGrad) + ", Flaeche " + z(v.flaeche),
           i.zahl == 1 && i.schleifen[0] == 1 && v.gekippt == 0 && v.winkelGrad < 2.0F);
    // Naehte aus dem Skelett: ein Rohr, unten Oberarm, Mitte Unterarm, oben Hand - Schnitte zwischen den Teilen, das
    // mittlere Teil laengs (WL: zwischen seinen zwei fernsten Grenzkanten); die Enden bleiben Ringe, Figur schneidet sie auf
    UvNetz s = netzVon(rohr(16, 7));
    for (int h = 0; h < s.kanten(); ++h) {
        const float hz = s.ortVon(h).z;
        s.knochen[uz(h)][0] = static_cast<std::uint8_t>(hz < 0.9F ? 1 : (hz < 2.1F ? 2 : 3));
    }
    k.faltung = knochenFaltung({Rolle::Becken, Rolle::Oberarm, Rolle::Unterarm, Rolle::Hand}, {-1, 0, 1, 2}, &k.glieder);
    k.vorn = V3{0.0F, -1.0F, 0.0F};
    UvNetz s2 = s;
    const UvAntwort sa = befehlNaehteAusSkelett(s, UvAuswahl{}, k);
    const UvInseln si = inselnBauen(s);
    int ringe = 0;
    for (int q = 0; q < si.zahl; ++q) { ringe += si.schleifen[uz(q)] >= 2 ? 1 : 0; }
    expect("Naehte aus dem Skelett: 3 Teile, " + std::to_string(sa.zahl) + " Schnittkanten, 3 Inseln, nur die zwei Enden noch Ringe (" + std::to_string(ringe) + ")",
           sa.andere == 3 && si.zahl == 3 && ringe == 2);
    const UvAntwort fa2 = befehlSchnellAbwickeln(s2, UvAuswahl{}, Vorlage::Figur, k);
    const UvInseln fi = inselnBauen(s2);
    int ringe2 = 0;
    for (int q = 0; q < fi.zahl; ++q) { ringe2 += fi.schleifen[uz(q)] >= 2 ? 1 : 0; }
    const Verzerrung fv = messen(s2, fi, 1024);
    expect("Quick unwrap Figur: 3 Inseln ohne Ring, ohne Kippen, Winkel " + z(fv.winkelGrad) + ", in der Kachel", fa2.zahl == 3 && fi.zahl == 3 && ringe2 == 0 &&
                                                                                                                   fv.gekippt == 0 && allesInKachel(s2));
    // nach Winkel: der Wuerfel hat 12 Kanten zu 90 Grad (18 Halbkanten-Paare, 6 Diagonalen flach)
    UvNetz w = netzVon(wuerfel());
    const UvAntwort wa = befehlNaehteNachWinkel(w, UvAuswahl{}, 60.0F);
    expect("Naehte nach Winkel 60: die 12 Kanten des Wuerfels (" + std::to_string(wa.zahl) + ")", wa.zahl == 12);
    // nach Form (xatlas) und Quick unwrap "Requisit"
    UvNetz q = netzVon(wuerfel());
    const UvAntwort qa = befehlSchnellAbwickeln(q, UvAuswahl{}, Vorlage::Requisit, k);
    PruefOptionen po;
    po.kacheln = false;
    const UvBefunde qb = pruefen(q, inselnBauen(q), po);
    expect("Quick unwrap Requisit (xatlas): " + std::to_string(qa.zahl) + " Inseln, ohne Ueberlappung, ohne Kippen, in der Kachel",
           qa.zahl >= 1 && qb.ueberlappt.empty() && qb.gekippt.empty() && allesInKachel(q));
    // Quick unwrap "Figur" ohne Glieder: abgewiesen, nichts geaendert
    UvNetz fz = netzVon(wuerfel());
    UvKontext ohne;
    const std::vector<V2> vor = fz.uv;
    const UvAntwort fa = befehlSchnellAbwickeln(fz, UvAuswahl{}, Vorlage::Figur, ohne);
    expect("Quick unwrap Figur ohne Gliedmassen: abgewiesen (KeineGlieder), UV unveraendert", fa.meldung == UvMeldung::KeineGlieder && fz.uv.size() == vor.size() &&
                                                                                                      std::memcmp(fz.uv.data(), vor.data(), vor.size() * sizeof(V2)) == 0);
}

void testPruefen() {
    std::printf("Pruefen\n");
    Inhalt in = gitter(3);
    std::swap(in.teile[0].stufen[0].ecken[5].uv[0], in.teile[0].stufen[0].ecken[6].uv[0]);   // ein Dreieck umgeklappt
    const UvNetz n = netzVon(in);
    PruefOptionen po;
    po.kacheln = false;
    const UvBefunde b = pruefen(n, inselnBauen(n), po);
    expect("gekippte Dreiecke werden gefunden", b.gekippt.size() == 1U);
    UvNetz a = netzVon(gitter(2));
    for (V2& w : a.uv) { w.x += 1.5F; }
    expect("ausserhalb der Kachel (ohne Wiederholen)", pruefen(a, inselnBauen(a), po).aussen.size() == 1U);
    po.kacheln = true;
    expect("mit Wiederholen (JKA ohne clampMap) kein Befund", pruefen(a, inselnBauen(a), po).aussen.empty());
}

// AP7: Ausnutzung der Textur - der Anteil der Texel der Kachel 0..1, den mindestens eine Insel bedeckt. Die Faelle sind so
// gewaehlt, dass kein Texelmittelpunkt in die Naehe einer Dreieckskante faellt (Kanten bei Vielfachen von 3,2 Texeln,
// Mittelpunkte bei x,5) ausser genau auf der Diagonalen der Quadrate mit u = v - dort rechnet das Raster beide Seiten zu 0
// und zaehlt den Texel einmal. Deshalb gilt die Zahl genau.
void testAusnutzung() {
    std::printf("Ausnutzung\n");
    PruefOptionen po;
    po.aufloesung = 64;
    // Gitter 4 x 4 mit UV = Ort * 0,05: die UV fuellen [0, 0,2] x [0, 0,2] = 12,8 x 12,8 Texel - 13 x 13 Mittelpunkte von 64 x 64
    const UvNetz klein = netzVon(gitter(4));
    const UvBefunde b = pruefen(klein, inselnBauen(klein), po);
    expect("Gitter ueber 12,8 x 12,8 von 64 x 64 Texeln: Ausnutzung 169/4096 (" + z(b.ausnutzung) + ")", b.ausnutzung == 169.0F / 4096.0F);
    // zwei Flaechen mit denselben UV uebereinander zaehlen einmal
    Inhalt zwei = gitter(4);
    zwei.teile.push_back(gitter(4, 20.0F).teile[0]);
    zwei.teile[1].name = "zweite";
    const UvNetz doppelt = netzVon(zwei);
    const UvBefunde bd = pruefen(doppelt, inselnBauen(doppelt), po);
    expect("zwei Inseln uebereinander zaehlen einmal: Ausnutzung 169/4096 (" + z(bd.ausnutzung) + ")", bd.ausnutzung == 169.0F / 4096.0F);
    // ausserhalb der Kachel zaehlt nichts (das Raster der Pruefung ist die Kachel 0..1)
    UvNetz aussen = klein;
    for (V2& w : aussen.uv) { w.x += 2.0F; }
    expect("UV ausserhalb der Kachel: Ausnutzung 0", pruefen(aussen, inselnBauen(aussen), po).ausnutzung == 0.0F);
    // ein Quadrat aus zwei Dreiecken ueber die ganze Kachel
    const UvNetz ganz = netzVon(inhaltAus({ecke(0.0F, 0.0F, 0.0F, 0.0F, 0.0F), ecke(1.0F, 0.0F, 0.0F, 1.0F, 0.0F), ecke(1.0F, 1.0F, 0.0F, 1.0F, 1.0F), ecke(0.0F, 1.0F, 0.0F, 0.0F, 1.0F)}, {0, 1, 2, 0, 2, 3}));
    const UvBefunde bg = pruefen(ganz, inselnBauen(ganz), po);
    expect("die ganze Kachel bedeckt: Ausnutzung 1 (" + z(bg.ausnutzung) + ")", bg.ausnutzung == 1.0F);
}

// --- JKA-Modell --------------------------------------------------------------------------------------------------

std::vector<int> teileOhneTags(const Inhalt& in, int stufe) {
    std::vector<int> t;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if (!in.teile[i].istTag() && uz(stufe) < in.teile[i].stufen.size()) { t.push_back(static_cast<int>(i)); }
    }
    return t;
}

void testJka(const std::string& pfad) {
    std::printf("JKA-Modell %s\n", pfad.c_str());
    const std::string bytes = slurp(pfad);
    Inhalt in;
    std::string f;
    if (!ausGlm(bytes, in, &f)) {
        expect("Modell lesen: " + f, false);
        return;
    }
    // Rundlauf ohne Aenderung: jede Stufe ueber das Arbeitsnetz zurueck, dann Byte fuer Byte gleich
    for (int s = 0; s < in.stufenZahl; ++s) {
        const UvNetz n = netzAus(in, teileOhneTags(in, s), s);
        ZurueckBericht br;
        (void)zurueckschreiben(n, in, 1000, &br);
        expect("LOD " + std::to_string(s) + ": zurueck ohne Aenderung - " + std::to_string(br.geaendert) + " Flaechen geaendert, " + std::to_string(br.neueEcken) + " neue Ecken",
               br.ok && br.geaendert == 0 && br.neueEcken == 0);
    }
    std::string wieder;
    expect("GLM ueber das Arbeitsnetz ohne Aenderung Byte fuer Byte gleich", glmSchreiben(in, wieder) && wieder == bytes);
    // Kopf und Torso: die Datei-UV messen; jede Insel der Datei neu loesen (LSCM, minimale Dehnung; die Naehte der Datei
    // bleiben); Quick unwrap "Requisit" (Naehte nach der Form, xatlas) - Verzerrung (Winkel, Flaeche) und Texel-Dichte
    const int kopf = in.teilIndex("head");
    const int torso = in.teilIndex("torso");
    expect("Kopf und Torso gefunden", kopf >= 0 && torso >= 0);
    if (kopf < 0 || torso < 0) { return; }
    const auto zeile = [](const char* was, const Verzerrung& v) {
        std::printf("    %-34s %3d Inseln  Winkel %7.3f Grad (max %6.1f)  konform %6.4f  Flaeche %6.4f  Dichte %6.1f/%6.1f/%6.1f  gekippt %d\n", was, v.inseln, v.winkelGrad,
                    v.winkelMaxGrad, v.konform, v.flaeche, v.dichteMin, v.dichteMedian, v.dichteMax, v.gekippt);
    };
    PruefOptionen po;
    po.kacheln = false;
    po.aufloesung = 512;
    for (const int t : {kopf, torso}) {
        const char* name = t == kopf ? "Kopf" : "Torso";
        const UvNetz datei = netzAus(in, {t}, 0);
        const Verzerrung vor = messen(datei, inselnBauen(datei), 512);
        zeile((std::string(name) + " Datei").c_str(), vor);
        Verzerrung lscmV;
        for (const UvMethode m : {UvMethode::Winkeltreu, UvMethode::MinimaleDehnung}) {
            UvNetz n = datei;
            UvKontext k;
            k.methode = m;
            k.aufloesung = 512;
            const UvAntwort a = befehlNeuLoesen(n, alles(n, UvEbene::Insel), k);
            const Verzerrung nach = messen(n, inselnBauen(n), 512);
            const bool winkeltreu = m == UvMethode::Winkeltreu;
            zeile((std::string(name) + (winkeltreu ? " neu geloest LSCM" : " neu geloest min. Dehnung")).c_str(), nach);
            if (winkeltreu) {
                lscmV = nach;
                expect(std::string(name) + " LSCM: gleiche Inseln, ohne Kippen, Winkelfehler " + z(nach.winkelGrad) + " <= Datei " + z(vor.winkelGrad),
                       a.meldung == UvMeldung::Keine && nach.inseln == vor.inseln && nach.gekippt == 0 && nach.winkelGrad <= vor.winkelGrad);
            } else {
                expect(std::string(name) + " minimale Dehnung: ohne Kippen, Flaechenverzerrung " + z(nach.flaeche) + " < LSCM " + z(lscmV.flaeche),
                       nach.inseln == vor.inseln && nach.gekippt == 0 && nach.flaeche < lscmV.flaeche);
            }
        }
        Inhalt x = in;
        UvNetz n = netzAus(x, {t}, 0);
        UvKontext k;
        k.aufloesung = 512;
        const UvAntwort a = befehlSchnellAbwickeln(n, UvAuswahl{}, Vorlage::Requisit, k);
        const UvInseln i = inselnBauen(n);
        const Verzerrung nach = messen(n, i, 512);
        zeile((std::string(name) + " Quick unwrap Requisit").c_str(), nach);
        const UvBefunde b = pruefen(n, i, po);
        ZurueckBericht br;
        const bool ok = zurueckschreiben(n, x, 1000, &br);
        {
            UvNetz hn = netzAus(in, {t}, 0);
            const UvAntwort ha = befehlSchnellAbwickeln(hn, UvAuswahl{}, Vorlage::HarteFlaeche, k);
            const UvInseln hi = inselnBauen(hn);
            const Verzerrung hv = messen(hn, hi, 512);
            zeile((std::string(name) + " Quick unwrap Harte Flaeche").c_str(), hv);
            const UvBefunde hb = pruefen(hn, hi, po);
            expect(std::string(name) + " Harte Flaeche: " + std::to_string(ha.zahl) + " Inseln, " + std::to_string(hv.gekippt) + " Dreiecke gekippt, " +
                       std::to_string(hb.ueberlappt.size()) + " ueberlappend, in der Kachel " + std::to_string(allesInKachel(hn) ? 1 : 0),
                   hv.gekippt == 0 && hb.ueberlappt.empty() && allesInKachel(hn));
        }
        expect(std::string(name) + " Requisit: " + std::to_string(a.zahl) + " Inseln, ohne Kippen, ohne Ueberlappung, in der Kachel, " + std::to_string(br.neueEcken) +
                   " neue Ecken unter der Grenze",
               nach.gekippt == 0 && b.ueberlappt.empty() && allesInKachel(n) && ok && br.neueEcken > 0);
        // UV -> GLM -> UV, Gewichte der geteilten Ecken
        std::string neu;
        Inhalt y;
        expect(std::string(name) + " als .glm geschrieben und wieder gelesen", glmSchreiben(x, neu) && ausGlm(neu, y));
        const Netz& a0 = x.teile[uz(t)].stufen[0];
        const Netz& b0 = y.teile[uz(t)].stufen[0];
        bool gleich = a0.kanten.size() == b0.kanten.size();
        for (std::size_t h = 0; gleich && h < a0.kanten.size(); ++h) {
            gleich = std::memcmp(a0.ecken[uz(a0.kanten[h].ecke)].uv, b0.ecken[uz(b0.kanten[h].ecke)].uv, 8) == 0;
        }
        expect("UV -> GLM -> UV: jede Ecke jedes Dreiecks Bit fuer Bit gleich", gleich);
        const Netz& alt = in.teile[uz(t)].stufen[0];
        int falsch = 0;
        for (std::size_t ei = 0; ei < b0.ecken.size() && ei < a0.ecken.size(); ++ei) {
            // die Ecke der neuen Datei gegen die Ecke der alten Datei, von der sie stammt (Dokument: dateiIndex)
            const Ecke& e = b0.ecken[ei];
            const Ecke& q = alt.ecken[uz(std::clamp(a0.ecken[ei].dateiIndex, 0, static_cast<int>(alt.ecken.size()) - 1))];
            if (a0.ecken[ei].dateiIndex < 0 || static_cast<std::size_t>(a0.ecken[ei].dateiIndex) >= alt.ecken.size()) { ++falsch; }
            for (int j = 0; j < 4; ++j) {
                if (e.knochen[j] != q.knochen[j] || e.gewicht[j] != q.gewicht[j] || e.dateiGewicht[j] != q.dateiGewicht[j]) { ++falsch; }
            }
            if (e.dateiPack != q.dateiPack || std::memcmp(e.ort, q.ort, 12) != 0 || std::memcmp(e.normale, q.normale, 12) != 0) { ++falsch; }
        }
        expect("die " + std::to_string(b0.ecken.size() - alt.ecken.size()) + " geteilten Ecken behalten Ort, Normale, Knochen, Gewichte und Gewichtsfelder (" +
                   std::to_string(falsch) + " Abweichungen)",
               falsch == 0 && b0.ecken.size() > alt.ecken.size());
        bool andereGleich = true;
        for (std::size_t q = 0; q < in.teile.size(); ++q) {
            if (static_cast<int>(q) == t || in.teile[q].stufen.empty()) { continue; }
            const Netz& qa = in.teile[q].stufen[0];
            const Netz& qb = y.teile[q].stufen[0];
            andereGleich = andereGleich && qa.ecken.size() == qb.ecken.size() && qa.indizes() == qb.indizes();
            for (std::size_t e = 0; andereGleich && e < qa.ecken.size(); ++e) { andereGleich = std::memcmp(qa.ecken[e].uv, qb.ecken[e].uv, 8) == 0; }
        }
        expect("die anderen Flaechen unveraendert", andereGleich);
    }
    // Grenze: die Teilung haette eine Flaeche ueber der Grenze - nichts wird geschrieben
    {
        Inhalt x = in;
        UvNetz n = netzAus(x, {torso}, 0);
        const int vorher = static_cast<int>(x.teile[uz(torso)].stufen[0].ecken.size());
        UvKontext k;
        (void)befehlSchnellAbwickeln(n, UvAuswahl{}, Vorlage::Requisit, k);
        ZurueckBericht br;
        const bool ok = zurueckschreiben(n, x, vorher + 1, &br);
        expect("Grenze knapp: " + std::to_string(br.ecken) + " Ecken >= " + std::to_string(vorher + 1) + " - abgewiesen, Flaeche unveraendert",
               !ok && br.teil == torso && static_cast<int>(x.teile[uz(torso)].stufen[0].ecken.size()) == vorher);
        const bool ok2 = zurueckschreiben(n, x, br.ecken + 1, &br);
        expect("eine Ecke mehr Grenze: geschrieben", ok2);
    }
}

// --- MD3 -----------------------------------------------------------------------------------------------------------

void schreibe32(std::string& b, std::int32_t v) { b.append(reinterpret_cast<const char*>(&v), 4); }
void schreibeName(std::string& b, const char* n) {
    char x[64] = {};
    std::strncpy(x, n, 63);
    b.append(x, 64);
}
// Ein .md3 mit zwei Bildern, einer Flaeche (Gitter 3x3, Bild 1 = Bild 0 um 2 Einheiten angehoben), einem Tag
std::string testMd3() {
    const int n = 3;
    const int ecken = (n + 1) * (n + 1);
    const int drei = n * n * 2;
    std::string s;   // Flaeche
    s += "IDP3";
    schreibeName(s, "gitter");
    schreibe32(s, 0);
    schreibe32(s, 2);   // Bilder
    schreibe32(s, 1);   // Shader
    schreibe32(s, ecken);
    schreibe32(s, drei);
    const int ofsDrei = 108;
    const int ofsShader = ofsDrei + drei * 12;
    const int ofsSt = ofsShader + 68;
    const int ofsXyz = ofsSt + ecken * 8;
    const int ende = ofsXyz + ecken * 8 * 2;
    for (const int o : {ofsDrei, ofsShader, ofsSt, ofsXyz, ende}) { schreibe32(s, o); }
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            const auto v = [&](int a, int b) { return b * (n + 1) + a; };
            for (const int x : {v(i, j), v(i + 1, j), v(i + 1, j + 1), v(i, j), v(i + 1, j + 1), v(i, j + 1)}) { schreibe32(s, x); }
        }
    }
    schreibeName(s, "textures/test/gitter");
    schreibe32(s, 0);
    for (int j = 0; j <= n; ++j) {
        for (int i = 0; i <= n; ++i) {
            const float st[2] = {static_cast<float>(i) / n, static_cast<float>(j) / n};
            s.append(reinterpret_cast<const char*>(st), 8);
        }
    }
    for (int bild = 0; bild < 2; ++bild) {
        for (int j = 0; j <= n; ++j) {
            for (int i = 0; i <= n; ++i) {
                const std::int16_t xyz[4] = {static_cast<std::int16_t>(i * 64 * 8), static_cast<std::int16_t>(j * 64 * 8), static_cast<std::int16_t>(bild * 2 * 64),
                                             static_cast<std::int16_t>(0)};
                s.append(reinterpret_cast<const char*>(xyz), 8);
            }
        }
    }
    std::string b;
    b += "IDP3";
    schreibe32(b, 15);
    schreibeName(b, "models/test/gitter.md3");
    schreibe32(b, 0);
    schreibe32(b, 2);   // Bilder
    schreibe32(b, 1);   // Tags
    schreibe32(b, 1);   // Flaechen
    schreibe32(b, 0);
    const int ofsBilder = 108;
    const int ofsTags = ofsBilder + 2 * 56;
    const int ofsFl = ofsTags + 2 * 112;
    schreibe32(b, ofsBilder);
    schreibe32(b, ofsTags);
    schreibe32(b, ofsFl);
    schreibe32(b, ofsFl + static_cast<int>(s.size()));
    for (int bild = 0; bild < 2; ++bild) {
        const float fr[10] = {0, 0, static_cast<float>(bild * 2), 24, 24, static_cast<float>(bild * 2), 0, 0, 0, 20};
        b.append(reinterpret_cast<const char*>(fr), 40);
        char nm[16] = "bild";
        b.append(nm, 16);
    }
    for (int bild = 0; bild < 2; ++bild) {
        schreibeName(b, "tag_flash");
        const float t[12] = {1, 2, 3, 1, 0, 0, 0, 1, 0, 0, 0, 1};
        b.append(reinterpret_cast<const char*>(t), 48);
    }
    b += s;
    return b;
}

void testMd3Rundlauf() {
    std::printf("MD3\n");
    const std::string bytes = testMd3();
    Md3Model m;
    std::string f;
    if (!readMd3(bytes, m, &f)) {
        expect("Test-.md3 lesen: " + f, false);
        return;
    }
    Inhalt in;
    ausMd3(m, in, 0);
    in.md3Datei = std::make_shared<const std::string>(bytes);
    std::string aus;
    Md3SchreibBericht br;
    expect("MD3 ohne Aenderung Byte fuer Byte gleich", md3Schreiben(in, aus, 1000, &br) && aus == bytes && br.flaechenRoh == 1);
    // ueber das Arbeitsnetz ohne Aenderung
    {
        Inhalt x = in;
        const UvNetz n = netzAus(x, {0}, 0);
        (void)zurueckschreiben(n, x, 1000);
        std::string a2;
        expect("MD3 ueber das Arbeitsnetz ohne Aenderung Byte fuer Byte gleich", md3Schreiben(x, a2, 1000) && a2 == bytes);
    }
    // eine Naht in der Mitte und abwickeln: mehr Ecken, Bild 1 jeder neuen Ecke wie ihre Vorlage
    UvNetz n = netzAus(in, {0}, 0);
    UvAuswahl a;
    a.ebene = UvEbene::Flaeche;
    a.ecke.assign(uz(n.kanten()), 0);
    for (int d = 0; d < n.dreiecke(); ++d) {
        const bool links = n.ortVon(d * 3).x + n.ortVon(d * 3 + 1).x + n.ortVon(d * 3 + 2).x < 3.0F * 12.0F;
        for (int k = 0; k < 3; ++k) { a.ecke[uz(d * 3 + k)] = links ? 1 : 0; }
    }
    UvKontext k;
    (void)befehlAbwickeln(n, a, k);
    ZurueckBericht zb;
    (void)zurueckschreiben(n, in, 1000, &zb);
    std::string neu;
    const bool ok = md3Schreiben(in, neu, 1000, &br);
    Md3Model m2;
    const bool gelesen = ok && readMd3(neu, m2, &f);
    expect("geteilt (" + std::to_string(zb.neueEcken) + " neue Ecken) geschrieben und gelesen, Flaeche neu aufgebaut", gelesen && br.flaechenNeu == 1 && zb.neueEcken > 0);
    if (!gelesen) { return; }
    const Md3Surface& s = m2.surfaces[0];
    const Md3Vertex* b0 = s.frame(0);
    const Md3Vertex* b1 = s.frame(1);
    int falsch = 0;
    for (int e = 0; e < s.numVerts; ++e) {
        if (std::abs(b1[e].xyz[2] - b0[e].xyz[2] - 2.0F) > 1e-4F) { ++falsch; }
    }
    const Netz& nz = in.teile[0].stufen[0];
    int uvFalsch = 0;
    for (std::size_t h = 0; h < nz.kanten.size(); ++h) {
        if (std::memcmp(nz.ecken[uz(nz.kanten[h].ecke)].uv, s.verts[s.indexes[h]].st, 8) != 0) { ++uvFalsch; }
    }
    expect("jede Ecke in Bild 1 zwei Einheiten ueber Bild 0 (" + std::to_string(falsch) + " falsch), UV je Dreiecksecke wie im Dokument (" + std::to_string(uvFalsch) + ")",
           falsch == 0 && uvFalsch == 0 && m2.numFrames == 2 && m2.tags.size() == 2U);
}

}  // namespace

int main(int argc, char** argv) {
    testInseln();
    testAbflachen();
    testAnordnen();
    testNaehte();
    testPruefen();
    testAusnutzung();
    if (argc > 1) { testJka(argv[1]); }
    testMd3Rundlauf();
    std::printf("%s (%d Fehler)\n", fails == 0 ? "ALLES OK" : "FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
