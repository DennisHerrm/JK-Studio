// kartepatchwerkzeuge.cpp - Karten-Editor Stufe 2, Punkt 6: das Curve-Menue
// wie NetRadiant-Custom 20260114 (radiant/patch.cpp:323-1590, 2870-3290;
// radiant/patchmanip.cpp:49-560).
//
// Grenzen der Vorlagen: (-32 -32 0)..(32 32 64), also vPos[0] = (-32 -32 0),
// vPos[1] = (0 0 32), vPos[2] = (32 32 64); Ansicht XY (Achse 2: x 0, y 1,
// z 2). Texturen 64 x 64, Standardskalierung 0.5: NaturalTexture teilt durch
// 32. Sollwerte von Hand aus den Tabellen pCylIndex/pBevIndex/pEndIndex.
#include "bhed/karte/karte.h"
#include "bhed/karte/patchbefehle.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}
bool nah(double a, double b, double eps = 1e-3) { return std::fabs(a - b) <= eps; }
bool punktNah(const Vektor3f& a, float x, float y, float z, double eps = 1e-3) { return nah(a[0], x, eps) && nah(a[1], y, eps) && nah(a[2], z, eps); }
std::string text(const Vektor3f& v) {
    char t[96];
    std::snprintf(t, sizeof(t), "(%g %g %g)", static_cast<double>(v[0]), static_cast<double>(v[1]), static_cast<double>(v[2]));
    return t;
}

const Kasten kGrenzen(Vektor3f(0, 0, 32), Vektor3f(32, 32, 32));

struct Aufbau {
    Karte k;
    std::shared_ptr<Pinsel> pinsel;
    Aufbau() {
        TexturProjektion proj;
        texdefStandard(TexdefArt::Quake, proj);
        pinsel = std::make_shared<Pinsel>();
        pinsel->quaderBauen(kGrenzen, "textures/a/b", proj, k.kontext);
        k.weltHolenOderAnlegen()->kindEinfuegen(pinsel);
        k.auswahl.waehlen(k.welt(), pinsel);
        k.undo.clear();
    }
    std::shared_ptr<Patch> patch(std::size_t i = 0) const {
        std::size_t n = 0;
        for (const auto& c : k.welt()->kinder()) {
            if (c->art() == Primitiv::Art::Patch) {
                if (n == i) { return std::static_pointer_cast<Patch>(c); }
                ++n;
            }
        }
        return nullptr;
    }
    std::size_t patches() const {
        std::size_t n = 0;
        for (const auto& c : k.welt()->kinder()) { n += c->art() == Primitiv::Art::Patch ? 1U : 0U; }
        return n;
    }
    std::shared_ptr<Patch> vorlage(PatchVorlage art, std::size_t w = 3, std::size_t h = 3) {
        patchVorlageBefehl(k, patchGrenzen(kGrenzen, 8), "textures/a/patch", art, 2, w, h);
        return patch();
    }
};

void vorlagen() {
    {   // Simple Patch Mesh 5 x 3 (constructPlane): oben z 64, Schritt 64/4 bzw. 64/2
        Aufbau a;
        const auto p = a.vorlage(PatchVorlage::Ebene, 5, 3);
        pruefe(p && p->breite() == 5 && p->hoehe() == 3 && punktNah(p->punkt(0, 0).vertex, -32, -32, 64) && punktNah(p->punkt(2, 4).vertex, 32, 32, 64) &&
                   punktNah(p->punkt(1, 1).vertex, -16, 0, 64),
               "Simple Patch Mesh 5x3: Ebene oben");
        pruefe(a.k.welt()->kinder().size() == 1 && a.k.auswahl.anzahl() == 1 && a.k.auswahl.gewaehlt(a.k.welt().get(), p.get()),
               "Simple Patch Mesh: der Pinsel geloescht (Select_Delete), der Patch gewaehlt");
        pruefe(a.k.undo.undoName() == "patchCreatePlane" && p->shader() == "textures/a/patch", "Simple Patch Mesh: Rueckgaengig-Name und Shader");
        a.k.undo.undo();
        pruefe(a.patches() == 0 && a.k.welt()->kinder().size() == 1, "Simple Patch Mesh: Rueckgaengig");
    }
    {   // Vorlage ersetzt einen gewaehlten Patch
        Aufbau a;
        (void)a.vorlage(PatchVorlage::Zylinder);
        const auto p = a.vorlage(PatchVorlage::Ebene, 3, 3);
        pruefe(a.patches() == 1 && p && p->breite() == 3 && a.k.auswahl.anzahl() == 1, "Ebene statt des gewaehlten Zylinders");
        a.k.undo.undo();
        pruefe(a.patches() == 1 && a.patch(0)->breite() == 9, "Ebene statt Zylinder: Rueckgaengig");
    }
    {   // Cylinder (9x3): Punkte 1..8 aus pCylIndex, 0 = 8
        Aufbau a;
        const auto p = a.vorlage(PatchVorlage::Zylinder);
        pruefe(p && p->breite() == 9 && p->hoehe() == 3, "Cylinder 9x3");
        if (p && p->breite() == 9) {
            pruefe(punktNah(p->punkt(0, 1).vertex, -32, -32, 0) && punktNah(p->punkt(0, 2).vertex, 0, -32, 0) && punktNah(p->punkt(1, 0).vertex, -32, 0, 32) &&
                       punktNah(p->punkt(2, 5).vertex, 32, 32, 64) && punktNah(p->punkt(2, 8).vertex, -32, 0, 64),
                   "Cylinder: die Punkte " + text(p->punkt(1, 0).vertex));
            // NaturalTexture: jede Sehne 32 lang -> s = Spalte, t = -Zeile
            pruefe(nah(p->punkt(0, 3).texcoord[0], 3) && nah(p->punkt(2, 0).texcoord[1], -2) && nah(p->punkt(1, 8).texcoord[0], 8),
                   "Cylinder: NaturalTexture s = Spalte, t = -Zeile");
        }
        pruefe(a.k.undo.undoName() == "patchCreateCylinder", "Cylinder: Rueckgaengig-Name");
    }
    {
        Aufbau a;
        const auto p = a.vorlage(PatchVorlage::EckZylinder);
        pruefe(p && punktNah(p->punkt(0, 0).vertex, -32, -32, 0) && punktNah(p->punkt(0, 8).vertex, -32, -32, 0) && punktNah(p->punkt(1, 1).vertex, 0, -32, 32),
               "Square Cylinder: ab Punkt 0, 8 = 0");
    }
    {
        Aufbau a;
        const auto p = a.vorlage(PatchVorlage::Kegel);
        pruefe(p && punktNah(p->punkt(2, 0).vertex, 0, 0, 64) && punktNah(p->punkt(2, 7).vertex, 0, 0, 64) && punktNah(p->punkt(0, 1).vertex, -32, -32, 0) &&
                   a.k.undo.undoName() == "patchCreateCone",
               "Cone: oben die Spitze");
    }
    {
        Aufbau a;
        const auto p = a.vorlage(PatchVorlage::Kugel);
        pruefe(p && p->breite() == 9 && p->hoehe() == 5 && punktNah(p->punkt(0, 3).vertex, 0, 0, 0) && punktNah(p->punkt(4, 6).vertex, 0, 0, 64) &&
                   punktNah(p->punkt(1, 1).vertex, -32, -32, 0) && punktNah(p->punkt(3, 5).vertex, 32, 32, 64) && punktNah(p->punkt(2, 0).vertex, -32, 0, 32),
               "Sphere 9x5: Pole und Ringe");
    }
    {
        Aufbau a;
        const auto p = a.vorlage(PatchVorlage::Bevel);
        pruefe(p && p->breite() == 3 && punktNah(p->punkt(0, 0).vertex, -32, -32, 0) && punktNah(p->punkt(1, 1).vertex, 32, -32, 32) &&
                   punktNah(p->punkt(2, 2).vertex, 32, 32, 64),
               "Bevel: (0 0), (2 0), (2 2)");
    }
    {
        Aufbau a;
        const auto p = a.vorlage(PatchVorlage::EndCap);
        pruefe(p && p->breite() == 5 && punktNah(p->punkt(0, 0).vertex, 32, -32, 0) && punktNah(p->punkt(1, 2).vertex, 0, 32, 32) &&
                   punktNah(p->punkt(2, 4).vertex, -32, -32, 64),
               "End cap: (2 0) (2 2) (1 2) (0 2) (0 0)");
    }
    {   // Exact Cylinder 13x3: n = 6, f = 1 / cos(pi/6): ungerade Punkte auf dem umschriebenen Kreis
        Aufbau a;
        const auto p = a.vorlage(PatchVorlage::ExakterZylinder, 13, 3);
        const double f = 1.0 / std::cos(3.14159265358979323846 / 6.0);
        pruefe(p && p->breite() == 13 && punktNah(p->punkt(0, 0).vertex, 32, 0, 0) &&
                   punktNah(p->punkt(0, 1).vertex, 32, static_cast<float>(32 * 0.5 * f), 0, 1e-3) &&
                   punktNah(p->punkt(2, 2).vertex, 16, static_cast<float>(32 * std::sqrt(0.75)), 64, 1e-3),
               "Exact Cylinder 13x3: Punkte auf dem Kreis " + (p ? text(p->punkt(0, 1).vertex) : std::string()));
        pruefe(a.k.undo.undoName() == "patchCreateXactCylinder", "Exact Cylinder: Rueckgaengig-Name");
        const PatchDichte d = patchDichte(PatchVorlage::ExakteKugel);
        pruefe(d.minZeilen == 5 && d.minSpalten == 7 && d.vorgabeZeilen == 7 && d.vorgabeSpalten == 13, "DoNewPatchDlg: Grenzen der Exact Sphere");
    }
    {   // Exact Sphere 9x5: unten -r, oben +r
        Aufbau a;
        const auto p = a.vorlage(PatchVorlage::ExakteKugel, 9, 5);
        pruefe(p && punktNah(p->punkt(0, 0).vertex, 0, 0, 0) && punktNah(p->punkt(4, 0).vertex, 0, 0, 64) && punktNah(p->punkt(2, 0).vertex, 32, 0, 32),
               "Exact Sphere 9x5: Pole und Aequator");
    }
}

void einfuegenLoeschen() {
    Aufbau a;
    const auto p = a.vorlage(PatchVorlage::Zylinder);
    a.k.auswahl.leeren();
    a.k.auswahl.waehlen(a.k.welt(), p);
    patchBefehl(a.k, PatchBefehl::SpaltenVornEinfuegen);
    pruefe(p->breite() == 11 && a.k.undo.undoName() == "patchInsertFirstColumns", "Insert (2) First Columns: 11 breit");
    if (p->breite() == 11) {
        pruefe(punktNah(p->punkt(0, 1).vertex, -32, -16, 0) && punktNah(p->punkt(0, 2).vertex, -24, -24, 0) && punktNah(p->punkt(0, 3).vertex, -16, -32, 0) &&
                   punktNah(p->punkt(0, 4).vertex, 0, -32, 0),
               "Insert: de Casteljau an pos 2 " + text(p->punkt(0, 2).vertex));
    }
    patchBefehl(a.k, PatchBefehl::SpaltenVornLoeschen);
    pruefe(p->breite() == 9 && punktNah(p->punkt(0, 1).vertex, -32, -32, 0) && punktNah(p->punkt(0, 2).vertex, 0, -32, 0) &&
               a.k.undo.undoName() == "patchDeleteFirstColumns",
           "Del First (2) Columns: wieder der Zylinder");
    patchBefehl(a.k, PatchBefehl::ZeilenHintenEinfuegen);
    pruefe(p->hoehe() == 5 && punktNah(p->punkt(3, 1).vertex, -32, -32, 48) && a.k.undo.undoName() == "patchInsertLastRows", "Insert (2) Last Rows: 5 hoch");
    patchBefehl(a.k, PatchBefehl::ZeilenVornLoeschen);
    pruefe(p->hoehe() == 3 && a.k.undo.undoName() == "patchDeleteFirstRows", "Del First (2) Rows: 3 hoch");
    // nicht unter 3: kein Loeschen bei 3 Spalten
    Aufbau b;
    const auto q = b.vorlage(PatchVorlage::Ebene, 3, 3);
    patchBefehl(b.k, PatchBefehl::SpaltenHintenLoeschen);
    pruefe(q->breite() == 3 && q->hoehe() == 3, "Del Last Columns bei 3x3: nichts (MIN_PATCH_WIDTH)");
}

void matrix() {
    Aufbau a;
    const auto p = a.vorlage(PatchVorlage::EckZylinder);
    patchBefehl(a.k, PatchBefehl::SpaltenGlaetten);   // Smooth( COL ) mit Umlauf (Punkt 0 = 8)
    pruefe(punktNah(p->punkt(0, 2).vertex, 16, -16, 0) && punktNah(p->punkt(0, 4).vertex, 16, 16, 0) && punktNah(p->punkt(0, 6).vertex, -16, 16, 0) &&
               punktNah(p->punkt(0, 0).vertex, -16, -16, 0) && punktNah(p->punkt(0, 8).vertex, -16, -16, 0),
           "Smooth Columns: Mitten und Umlauf " + text(p->punkt(0, 0).vertex));
    pruefe(a.k.undo.undoName() == "patchSmoothColumns", "Smooth Columns: Rueckgaengig-Name");
    a.k.undo.undo();
    patchBefehl(a.k, PatchBefehl::Umkehren);
    pruefe(punktNah(p->punkt(0, 0).vertex, -32, -32, 64) && punktNah(p->punkt(2, 0).vertex, -32, -32, 0) && a.k.undo.undoName() == "patchInvert",
           "Invert: Zeilen vertauscht");
    patchBefehl(a.k, PatchBefehl::Transponieren);
    pruefe(p->breite() == 3 && p->hoehe() == 9 && punktNah(p->punkt(1, 0).vertex, 0, -32, 64) && a.k.undo.undoName() == "patchTranspose",
           "Transpose: 3 x 9");
    // Re-disperse: eine Mitte verschieben, dann wieder in die Mitte
    Aufbau b;
    const auto q = b.vorlage(PatchVorlage::Ebene, 3, 3);
    std::vector<PatchPunkt> pk = q->punkte();
    pk[1].vertex = Vektor3f(10, -20, 64);
    q->punkteSetzen(pk);
    patchBefehl(b.k, PatchBefehl::SpaltenVerteilen);
    pruefe(punktNah(q->punkt(0, 1).vertex, 0, -32, 64) && b.k.undo.undoName() == "patchRedisperseColumns", "Re-disperse Columns: Mitte zwischen 0 und 2");
}

void textur() {
    Aufbau a;
    const auto p = a.vorlage(PatchVorlage::Ebene, 3, 3);
    p->texturWiederholung(2, 1);
    pruefe(nah(p->punkt(0, 2).texcoord[0], 2) && nah(p->punkt(2, 0).texcoord[1], 1) && nah(p->punkt(1, 1).texcoord[0], 1) && nah(p->punkt(1, 1).texcoord[1], 0.5),
           "SetTextureRepeat 2 1");
    patchBefehl(a.k, PatchBefehl::TexturX);
    pruefe(nah(p->punkt(0, 2).texcoord[0], -2) && a.k.undo.undoName() == "patchFlipTextureU", "Invert X: s negiert");
    patchBefehl(a.k, PatchBefehl::TexturY);
    pruefe(nah(p->punkt(2, 0).texcoord[1], -1) && a.k.undo.undoName() == "patchFlipTextureV", "Invert Y: t negiert");
    p->texturVerschieben(32, 16, a.k.kontext);   // s -= 32/64, t += 16/64
    pruefe(nah(p->punkt(0, 0).texcoord[0], -0.5) && nah(p->punkt(0, 0).texcoord[1], 0.25), "TranslateTexture 32 16");
    p->texturSkalieren(2, 4);
    pruefe(nah(p->punkt(0, 0).texcoord[0], -1) && nah(p->punkt(0, 0).texcoord[1], 1), "ScaleTexture 2 4");
    p->texturDrehen(90);   // (x, y) -> (-y, x)
    pruefe(nah(p->punkt(0, 0).texcoord[0], -1) && nah(p->punkt(0, 0).texcoord[1], -1), "RotateTexture 90");
    patchBefehl(a.k, PatchBefehl::TexturNatuerlich);   // Ebene 64 x 64, 2 Schritte je 32: s 0 1 2, t 0 -1 -2
    pruefe(nah(p->punkt(0, 2).texcoord[0], 2) && nah(p->punkt(2, 0).texcoord[1], -2) && a.k.undo.undoName() == "patchNaturalTexture",
           "Naturalize: s 0..2, t 0..-2");
    // Reset Texture (textureReset/Cap): CapTexture
    auto vergleich = p->kopie();
    vergleich->texturKappen(a.k.kontext);
    p->texturWiederholung(3, 3);
    texturZuruecksetzenKappen(a.k, AuswahlEinstellung{});
    bool gleich = true;
    for (std::size_t i = 0; i < p->punkte().size(); ++i) {
        gleich = gleich && nah(p->punkte()[i].texcoord[0], vergleich->punkte()[i].texcoord[0]) && nah(p->punkte()[i].texcoord[1], vergleich->punkte()[i].texcoord[1]);
    }
    pruefe(gleich && a.k.undo.undoName() == "textureReset/Cap", "Reset Texture: wie CapTexture");
}

void kappen() {
    Aufbau a;
    const auto p = a.vorlage(PatchVorlage::Zylinder);
    Protokoll pr;
    patchKappen(a.k, "textures/a/kappe", PatchKappe::EndCap, pr);
    pruefe(pr.size() == 1 && pr[0].text == "cannot create end-cap - patch width != 5\n" && a.patches() == 1, "End-cap an 9 breit: Fehler");
    Protokoll pr2;
    patchKappen(a.k, "textures/a/kappe", PatchKappe::Zylinder, pr2);
    pruefe(pr2.empty() && a.patches() == 3 && a.k.auswahl.anzahl() == 3 && a.k.undo.undoName() == "patchPutCaps", "Cylinder cap: zwei Kappen, alle gewaehlt");
    const auto c = a.patch(1);
    if (c) {   // ConstructSeam Cylinder: 3 x 5, Spalte 0 = p0..p4, Spalte 2 = p8..p4, Mitte redispersed
        pruefe(c->breite() == 3 && c->hoehe() == 5 && punktNah(c->punkt(0, 0).vertex, -32, 0, 0) && punktNah(c->punkt(2, 0).vertex, 0, -32, 0) &&
                   punktNah(c->punkt(2, 2).vertex, 0, 32, 0) && punktNah(c->punkt(2, 1).vertex, 0, 0, 0) && c->shader() == "textures/a/kappe",
               "Cylinder cap: Punkte " + text(c->punkt(2, 1).vertex));
    }
}

void verformenVerdicken() {
    {
        Aufbau a;
        const auto p = a.vorlage(PatchVorlage::Ebene, 3, 3);
        std::srand(1);
        patchVerformen(a.k, 64, 2);
        bool imBereich = true;
        bool einer = false;
        for (const auto& c : p->punkte()) {
            imBereich = imBereich && c.vertex[2] >= 64 && c.vertex[2] <= 128 && c.vertex[2] == std::floor(c.vertex[2]);
            einer = einer || c.vertex[2] > 64;
        }
        pruefe(imBereich && einer && a.k.undo.undoName() == "patchDeform", "Deform 64 Z: ganze Zufallszahlen 0..64");
    }
    {   // Thicken 8 entlang Z mit Waenden: Gegenseite + 4 Waende
        Aufbau a;
        const auto p = a.vorlage(PatchVorlage::Ebene, 3, 3);
        patchVerdicken(a.k, 8, true, 2);
        pruefe(a.patches() == 6 && a.k.auswahl.anzahl() == 6 && a.k.undo.undoName() == "patchThicken", "Thicken Z mit Waenden: 5 neue Patches");
        const auto g = a.patch(1);
        pruefe(g && punktNah(g->punkt(0, 0).vertex, -32, 32, 72) && punktNah(g->punkt(2, 0).vertex, -32, -32, 72), "Thicken: Gegenseite 8 hoeher, umgekehrt");
    }
    {   // entlang der Normalen: kreuz(Zeile, Spalte) = (0 1 0) x (1 0 0) = (0 0 -1)
        Aufbau a;
        (void)a.vorlage(PatchVorlage::Ebene, 3, 3);
        patchVerdicken(a.k, 8, false, 3);
        const auto g = a.patch(1);
        pruefe(a.patches() == 2 && g && nah(g->punkt(1, 1).vertex[2], 56), "Thicken Normal ohne Waende: z 64 - 8");
    }
}

// Nachgestellt aus dem Selbsttest s2p6 (Absturz): nach Rueckgaengig von
// Thicken blieben die geloeschten Patches gewaehlt; ein abgelaufenes Primitiv
// galt in GewaehltMenge als "ganze Entitaet", Select_Delete in der naechsten
// Vorlage loeschte den worldspawn.
void verwaisteAuswahl() {
    Aufbau a;
    (void)a.vorlage(PatchVorlage::Zylinder);
    patchVerdicken(a.k, 8, true, 3);
    const std::size_t nachher = a.k.auswahl.anzahl();
    a.k.undo.undo();
    pruefe(nachher == 4 && a.patches() == 1, "Thicken rueckgaengig: 1 Patch in der Szene");
    a.k.auswahlBereinigen();
    pruefe(a.k.auswahl.anzahl() == 1, "auswahlBereinigen: nur der Zylinder bleibt gewaehlt (" + std::to_string(a.k.auswahl.anzahl()) + ")");
    // ohne Bereinigen: abgelaufene Eintraege duerfen nicht die Welt loeschen
    Aufbau b;
    (void)b.vorlage(PatchVorlage::Zylinder);
    {
        auto weg = std::make_shared<Patch>();
        b.k.welt()->kindEinfuegen(weg);
        b.k.auswahl.waehlen(b.k.welt(), weg);
        b.k.welt()->kindEntfernen(weg.get());
    }   // weg ist abgelaufen, der Eintrag bleibt
    patchVorlageBefehl(b.k, patchGrenzen(kGrenzen, 8), "textures/a/patch", PatchVorlage::Ebene, 2, 3, 3);
    const bool welt = b.k.welt() != nullptr;
    pruefe(welt, "verwaister Eintrag: die Welt bleibt");
    if (welt) { pruefe(b.patches() == 1 && b.patch()->breite() == 3, "verwaister Eintrag: Ebene statt Zylinder"); }
}

}  // namespace

int main() {
    vorlagen();
    einfuegenLoeschen();
    matrix();
    textur();
    kappen();
    verformenVerdicken();
    verwaisteAuswahl();
    std::printf("kartepatchwerkzeuge: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
