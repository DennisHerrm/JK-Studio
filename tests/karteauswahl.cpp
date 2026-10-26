// karteauswahl.cpp - Karten-Editor Stufe 1b, Punkt 7: Auswaehlen wie
// NetRadiant-Custom (bhed/karte/auswahltest.h, bhed/karte/auswahlsystem.h).
//
// Sollwerte aus radiant/selection.cpp (SelectPoint: Ersetzen, Durchlaufen,
// Punktentitaeten zuerst in 2D; SelectPoint_InitPaint; SelectArea mit
// SelectionBoxForArea), radiant/selection_volume.h (BestPoint: Tiefe,
// Abstand, Wegschneiden), radiant/brush.h (Flaechen: Polygon in 3D,
// Schwerpunkt in 2D) und Scene_Intersect.
#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/kamera.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"
#include "bhed/karte/xyansicht.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
bool nah(float a, float b, float eps = 1e-3F) { return std::fabs(a - b) <= eps; }

std::shared_ptr<Pinsel> wuerfel(Karte& k, const Vektor3f& mitte, float halb, const std::string& shader = "textures/a/b") {
    TexturProjektion proj;
    texdefStandard(TexdefArt::Quake, proj);
    auto p = std::make_shared<Pinsel>();
    p->quaderBauen(Kasten(mitte, Vektor3f(halb, halb, halb)), shader, proj, k.kontext);
    k.weltHolenOderAnlegen()->kindEinfuegen(p);
    return p;
}

Blick blick2d(Sicht s, float massstab, const Vektor3f& ursprung) {
    XyAnsicht v;
    v.typ = s;
    v.breite = 800;
    v.hoehe = 600;
    v.massstab = massstab;
    v.ursprung = ursprung;
    Blick b(false);
    b.bauen(v.projektion(), v.modelview(), 800, 600);
    return b;
}

Blick blick3d(const Vektor3f& o, const Vektor3f& w) {
    Kamera k;
    k.breite = 800;
    k.hoehe = 600;
    k.ursprung = o;
    k.winkel = w;
    k.projektionAktualisieren(KameraEinstellungen());
    k.modelviewAktualisieren();
    Blick b(true);
    b.bauen(k.projektion, k.modelview, 800, 600);
    return b;
}

const GeraeteVektor kEps(12.0F / 800.0F, 12.0F / 600.0F);   // g_SELECT_EPSILON / Fenster

void grundlagen() {
    Treffer a(0.5F, 0);
    Treffer b(0.3F, 0.1F);
    pruefe(a < b, "direkter Treffer (Abstand 0) vor indirektem");
    Treffer c(0.2F, 0);
    pruefe(c < a && !(a < c), "direkt: kleinere Tiefe zuerst");
    pruefe(!Treffer().gueltig() && a.gueltig(), "Tiefe 1 = kein Treffer");
    const AuswahlRechteck r = auswahlKastenFlaeche(GeraeteVektor(0, 0), GeraeteVektor(-0.2F, -0.3F));
    pruefe(r.art == AuswahlRechteck::Art::Abwaehlen && nah(r.min[0], -0.2F) && nah(r.max[1], 0), "Rechteck nach links unten: abwaehlen");
    pruefe(auswahlKastenFlaeche(GeraeteVektor(0, 0), GeraeteVektor(0.2F, -0.3F)).art == AuswahlRechteck::Art::Umschalten,
           "Rechteck gemischt: umschalten");
    pruefe(auswahlKastenFlaeche(GeraeteVektor(0, 0), GeraeteVektor(0.2F, 0.3F)).art == AuswahlRechteck::Art::Auswaehlen,
           "Rechteck nach rechts oben: auswaehlen");
    const GeraeteVektor g = fensterZuGeraet(400, 299, 800, 600);
    pruefe(nah(g[0], 0) && nah(g[1], 0), "window_to_normalised_device");
}

void tunnel3d() {
    // zwei Wuerfel hintereinander auf der Blickachse (+x)
    Karte k;
    auto nahW = wuerfel(k, Vektor3f(200, 0, 0), 32);
    auto fernW = wuerfel(k, Vektor3f(500, 0, 0), 32);
    const Blick b = blick3d(Vektor3f(0, 0, 0), Vektor3f(0, 0, 0));
    AuswahlEinstellung e;
    const auto welt = k.welt();
    punktWaehlen(k, e, b, GeraeteVektor(0, 0), kEps, AuswahlArt::Ersetzen, false, false);
    pruefe(k.auswahl.gewaehlt(welt.get(), nahW.get()) && k.auswahl.anzahl() == 1, "M1 3D: der vordere Wuerfel");
    punktWaehlen(k, e, b, GeraeteVektor(0, 0), kEps, AuswahlArt::Durchlaufen, false, false);
    pruefe(k.auswahl.gewaehlt(welt.get(), fernW.get()) && k.auswahl.anzahl() == 1, "Tunnel: der naechste dahinter");
    punktWaehlen(k, e, b, GeraeteVektor(0, 0), kEps, AuswahlArt::Durchlaufen, false, false);
    pruefe(k.auswahl.gewaehlt(welt.get(), nahW.get()) && k.auswahl.anzahl() == 1, "Tunnel: am Ende wieder der erste");
    punktWaehlen(k, e, b, GeraeteVektor(0.9F, 0.9F), kEps, AuswahlArt::Durchlaufen, false, false);
    pruefe(k.auswahl.anzahl() == 0, "Tunnel ins Leere: alles abgewaehlt");
    // Malen: Shift+M1 schaltet um und gibt die Art fuer das Ziehen zurueck
    const AuswahlArt a1 = malenBeginnen(k, e, b, GeraeteVektor(0, 0), kEps, false, false);
    pruefe(a1 == AuswahlArt::Auswaehlen && k.auswahl.gewaehlt(welt.get(), nahW.get()), "InitPaint: waehlt den vorderen, malt auswaehlend");
    const AuswahlArt a2 = malenBeginnen(k, e, b, GeraeteVektor(0, 0), kEps, false, false);
    pruefe(a2 == AuswahlArt::Abwaehlen && k.auswahl.anzahl() == 0, "InitPaint auf Ausgewaehltem: abwaehlend");
    // versteckt: nicht waehlbar
    nahW->versteckt = true;
    punktWaehlen(k, e, b, GeraeteVektor(0, 0), kEps, AuswahlArt::Ersetzen, false, false);
    pruefe(k.auswahl.gewaehlt(welt.get(), fernW.get()), "versteckter Wuerfel wird uebergangen");
    nahW->versteckt = false;
    // Flaechen (Strg): die Vorderseite des vorderen Wuerfels
    k.auswahl.leeren();
    punktWaehlen(k, e, b, GeraeteVektor(0, 0), kEps, AuswahlArt::Ersetzen, true, false);
    bool vorn = k.auswahl.flaechen().size() == 1 && k.auswahl.anzahl() == 0;
    if (vorn) {
        const auto f = k.auswahl.flaechen()[0].flaeche.lock();
        vorn = f && std::fabs(f->ebene().normal()[0] + 1.0) < 1e-6 && k.auswahl.flaechen()[0].pinsel.lock() == nahW;
    }
    pruefe(vorn, "Strg+M1 3D: die Flaeche -x des vorderen Wuerfels");
    // Scene_Intersect: Treffer auf der Vorderseite
    const Vektor3f s = szeneSchnitt(k, b, GeraeteVektor(0, 0), kEps);
    pruefe(nah(s[0], 168, 0.05F) && nah(s[1], 0, 0.05F), "Scene_Intersect: Punkt auf x = 168 (ist " + std::to_string(s[0]) + ")");
    const Vektor3f s2 = szeneSchnitt(k, b, GeraeteVektor(0.9F, 0.9F), kEps);
    pruefe(nah(static_cast<float>(laenge(s2)), 257, 1.0F), "Scene_Intersect ohne Treffer: 256 hinter der Nahebene");
}

void tunnel2d() {
    // zwei Wuerfel uebereinander (z) - in XY decken sie sich; ein Licht darauf
    Karte k;
    auto unten = wuerfel(k, Vektor3f(0, 0, 0), 32);
    auto oben = wuerfel(k, Vektor3f(0, 0, 128), 32);
    auto licht = k.neueEntitaet("light", false);
    licht->setzen("origin", "0 0 300");
    k.entitaetEinfuegen(licht);
    const Blick b = blick2d(Sicht::XY, 1, Vektor3f(0, 0, 0));
    AuswahlEinstellung e;
    const auto welt = k.welt();
    punktWaehlen(k, e, b, GeraeteVektor(0, 0), kEps, AuswahlArt::Ersetzen, false, false);
    pruefe(k.auswahl.gewaehlt(licht.get()) && k.auswahl.anzahl() == 1, "2D: Punktentitaeten zuerst (m_bPreferPointEntsIn2D)");
    // Durchlaufen: alle Primitive nach Tiefe - von oben: Licht, oberer, unterer Wuerfel
    k.auswahl.leeren();
    punktWaehlen(k, e, b, GeraeteVektor(0, 0), kEps, AuswahlArt::Durchlaufen, false, false);
    const bool erst = k.auswahl.gewaehlt(licht.get());
    punktWaehlen(k, e, b, GeraeteVektor(0, 0), kEps, AuswahlArt::Durchlaufen, false, false);
    const bool zweit = k.auswahl.gewaehlt(welt.get(), oben.get());
    punktWaehlen(k, e, b, GeraeteVektor(0, 0), kEps, AuswahlArt::Durchlaufen, false, false);
    const bool dritt = k.auswahl.gewaehlt(welt.get(), unten.get());
    pruefe(erst && zweit && dritt, "2D-Tunnel: Licht, oberer, unterer Wuerfel");
    // Rechteck: nur der rechte von zwei Wuerfeln nebeneinander
    Karte k2;
    auto links = wuerfel(k2, Vektor3f(-100, 0, 0), 16);
    auto rechts = wuerfel(k2, Vektor3f(100, 0, 0), 16);
    const auto welt2 = k2.welt();
    // Geraetepunkte: x = Welt / 400 (Massstab 1, halbe Breite 400)
    bereichWaehlen(k2, e, b, auswahlKastenFlaeche(GeraeteVektor(0.1F, -0.2F), GeraeteVektor(0.4F, 0.4F)), false);
    pruefe(k2.auswahl.gewaehlt(welt2.get(), rechts.get()) && !k2.auswahl.gewaehlt(welt2.get(), links.get()),
           "Rechteck: nur was im Rechteck liegt");
    bereichWaehlen(k2, e, b, auswahlKastenFlaeche(GeraeteVektor(-0.5F, -0.2F), GeraeteVektor(1.0F, 0.4F)), false);
    pruefe(k2.auswahl.anzahl() == 2, "Rechteck ueber beide: beide ausgewaehlt");
    bereichWaehlen(k2, e, b, auswahlKastenFlaeche(GeraeteVektor(0.5F, 0.2F), GeraeteVektor(-0.4F, -0.4F)), false);
    pruefe(k2.auswahl.anzahl() == 1 && k2.auswahl.gewaehlt(welt2.get(), links.get()), "Rechteck nach links unten: rechter abgewaehlt");
    // Flaechen in 2D: Schwerpunkt im Fangbereich (Oberseite des unteren? von oben: alle Seiten)
    Karte k3;
    auto w3 = wuerfel(k3, Vektor3f(0, 0, 0), 64);
    punktWaehlen(k3, e, b, GeraeteVektor(0, 0), kEps, AuswahlArt::Ersetzen, true, false);
    pruefe(!k3.auswahl.flaechen().empty() && k3.auswahl.anzahl() == 0, "Strg+M1 2D: Flaechen ueber ihren Schwerpunkt");
    // Patch: waehlbar
    Karte k4;
    auto pa = std::make_shared<Patch>();
    pa->setzeGroesse(3, 3);
    for (std::size_t r = 0; r < 3; ++r) {
        for (std::size_t c = 0; c < 3; ++c) {
            pa->punkt(r, c).vertex = Vektor3f(static_cast<float>(c) * 32 - 32, static_cast<float>(r) * 32 - 32, 0);
        }
    }
    pa->punkteGeaendert();
    k4.weltHolenOderAnlegen()->kindEinfuegen(pa);
    punktWaehlen(k4, e, b, GeraeteVektor(0, 0), kEps, AuswahlArt::Ersetzen, false, false);
    pruefe(k4.auswahl.gewaehlt(k4.welt().get(), pa.get()), "Patch: waehlbar (Viereckstreifen, zweiseitig)");
    const Kasten g = auswahlGrenzenPivot(k4);
    pruefe(nah(g.extents[0], 32) && nah(g.origin[2], 0), "getBoundsSelected des Patches");
}

}  // namespace

// Stufe 7: Auswahl::umschaltenAlle (Invert Selection) = umschalten() je Paar nacheinander,
// auch mit Eintraegen, deren Objekt schon geloescht ist (weak_ptr abgelaufen).
void umschaltenAlleGleich() {
    unsigned zufall = 12345U;
    const auto naechste = [&zufall] {
        zufall = zufall * 1103515245U + 12345U;
        return (zufall >> 16U) & 0x7FFFU;
    };
    int runden = 0;
    for (int r = 0; r < 40; ++r) {
        Karte k;
        std::vector<std::shared_ptr<Pinsel>> pinsel;
        for (int i = 0; i < 30; ++i) { pinsel.push_back(wuerfel(k, Vektor3f(float(i) * 40.0F, 0, 0), 8)); }
        auto ent = k.neueEntitaet("light", false);
        k.entitaetEinfuegen(ent);
        const auto welt = k.welt();
        // Anfangsauswahl: zufaellig, dazu Pinsel, die danach geloescht werden
        std::vector<std::shared_ptr<Pinsel>> geloescht;
        for (int i = 0; i < 8; ++i) {
            auto p = std::make_shared<Pinsel>();
            k.auswahl.waehlen(welt, p);
            geloescht.push_back(p);
        }
        for (const auto& p : pinsel) {
            if (naechste() % 3 == 0) { k.auswahl.waehlen(welt, p); }
        }
        if (naechste() % 2 == 0) { k.auswahl.waehlen(ent); }
        geloescht.erase(geloescht.begin(), geloescht.begin() + 4);   // 4 laufen ab
        std::vector<std::pair<std::shared_ptr<Entitaet>, std::shared_ptr<Primitiv>>> paare;
        for (const auto& p : pinsel) {
            if (naechste() % 4 != 0) { paare.emplace_back(welt, p); }
        }
        paare.emplace_back(ent, nullptr);
        Auswahl a = k.auswahl;
        Auswahl b = k.auswahl;
        for (const auto& [e, p] : paare) { a.umschalten(e, p); }
        b.umschaltenAlle(paare);
        bool gleich = a.anzahl() == b.anzahl();
        for (std::size_t i = 0; gleich && i < a.anzahl(); ++i) {
            gleich = a.elemente()[i].entitaet.lock() == b.elemente()[i].entitaet.lock() &&
                     a.elemente()[i].primitiv.lock() == b.elemente()[i].primitiv.lock() &&
                     a.elemente()[i].primitiv.expired() == b.elemente()[i].primitiv.expired();
        }
        runden += gleich ? 1 : 0;
    }
    pruefe(runden == 40, "umschaltenAlle = umschalten je Paar (" + std::to_string(runden) + " von 40 Runden gleich)");
}

// Stufe 7: gewaehlt() mit Zwischenspeicher = die Suche durch alle Elemente (lock()),
// auch nachdem gewaehlte Objekte zerstoert wurden (ohne Aenderung der Auswahl).
void gewaehltGleichSuche() {
    Karte k;
    std::vector<std::shared_ptr<Pinsel>> pinsel;
    for (int i = 0; i < 20; ++i) { pinsel.push_back(wuerfel(k, Vektor3f(float(i) * 40.0F, 0, 0), 8)); }
    const auto welt = k.welt();
    const auto suche = [&k](const Entitaet* e, const Primitiv* p) {
        for (const AuswahlElement& a : k.auswahl.elemente()) {
            if (a.entitaet.lock().get() == e && a.primitiv.lock().get() == p) { return true; }
        }
        return false;
    };
    int fehler = 0;
    const auto vergleiche = [&](const char* wann) {
        for (const auto& p : pinsel) {
            if (k.auswahl.gewaehlt(welt.get(), p.get()) != suche(welt.get(), p.get())) { ++fehler; }
        }
        if (k.auswahl.gewaehlt(welt.get()) != suche(welt.get(), nullptr)) {
            ++fehler;
            std::printf("  gewaehlt(welt) anders: %s\n", wann);
        }
    };
    for (int i = 0; i < 20; i += 2) { k.auswahl.waehlen(welt, pinsel[static_cast<std::size_t>(i)]); }
    vergleiche("nach waehlen");
    auto weg = std::make_shared<Pinsel>();
    k.auswahl.waehlen(welt, weg);
    vergleiche("mit einem freien Pinsel");
    weg.reset();   // zerstoert: lock() leer -> das Element zaehlt als (welt, nullptr)
    vergleiche("nach dem Zerstoeren");
    k.auswahl.abwaehlen(welt, pinsel[2]);
    k.auswahl.umschalten(welt, pinsel[3]);
    vergleiche("nach abwaehlen/umschalten");
    k.auswahl.aufraeumen();
    vergleiche("nach aufraeumen");
    k.auswahl.leeren();
    vergleiche("nach leeren");
    pruefe(fehler == 0, "gewaehlt mit Zwischenspeicher = Suche (" + std::to_string(fehler) + " Abweichungen)");
}

int main() {
    grundlagen();
    tunnel3d();
    tunnel2d();
    umschaltenAlleGleich();
    gewaehltGleichSuche();
    std::printf("karteauswahl: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
