// app_karte_spiegel.cpp - Karten-Editor, Stufe 8b Punkt 6: der Spiegel-Modus
// (eigene Funktion, nicht in NetRadiant-Custom; der Kern steht in
// src/karte/spiegel.cpp). View > "Mirror mode": aus / Ebene X / Y / Z, die
// Ebene durch die Mitte der Auswahl (aufs Raster) oder durch den Ursprung.
// Die Ebene erscheint in den 2D-Ansichten als magentafarbene Linie.
// Vorgabe aus - so verhaelt sich der Editor wie NRC.
#include "karteneditor.h"

#include "app_internal.h"
#include "bhed/karte/auswahlsystem.h"

#include <cmath>
#include <cstdio>

namespace bhed::gui::karteneditor {

namespace {

using i18n::Str;

Editor& e() { return editor(); }

void melden() {
    const karte::SpiegelModus& s = e().spiegel;
    if (!s.an) {
        ausgabe(trK(Str::KeSgAusMeldung));
        return;
    }
    char z[160];
    std::snprintf(z, sizeof(z), trK(Str::KeSgMeldung), "XYZ"[s.achse], s.ebene);
    ausgabe(z);
}

}  // namespace

void spiegelTick() {
    Editor& ed = e();
    ed.spiegel.abgleichen(ed.karte, ed.karte.undo.befehlOffen());
    // Stufe 9 Punkt 6: ausserhalb eines Befehls (Rueckgaengig, Laden) nur den Stand merken -
    // waehrend eines Befehls (Ziehen) nicht: dann bliebe der Stand davor nicht erhalten
    if (!ed.karte.undo.befehlOffen()) { ed.verbund.abgleichen(ed.karte, false); }
}

void spiegelSetzen(int achse) {
    karte::SpiegelModus& s = e().spiegel;
    if (achse < 0) {
        s.an = false;
    } else {
        s.an = true;
        s.achse = achse;
    }
    spiegelTick();
    melden();
}

void spiegelEbeneAnAuswahl() {
    Editor& ed = e();
    karte::SpiegelModus& s = ed.spiegel;
    if (ed.karte.auswahl.anzahl() == 0) { return; }
    const karte::Kasten b = karte::auswahlGrenzenPivot(ed.karte);
    const double r = ed.raster.groesse();
    s.ebene = std::round(static_cast<double>(b.origin[static_cast<std::size_t>(s.achse)]) / r) * r;
    if (s.ebene == 0) { s.ebene = 0; }   // -0
    spiegelTick();
    melden();
}

void spiegelEbeneNull() {
    e().spiegel.ebene = 0;
    spiegelTick();
    melden();
}

void spiegelMenue() {
    karte::SpiegelModus& s = e().spiegel;
    if (!qtMenueAnfang((std::string(trK(Str::KeSgMenue)) + "###spiegel").c_str())) { return; }
    const struct {
        Str text;
        int achse;
        const char* id;
    } punkte[4] = {{Str::KeSgAus, -1, "###spiegelaus"}, {Str::KeSgX, 0, "###spiegelx"}, {Str::KeSgY, 1, "###spiegely"}, {Str::KeSgZ, 2, "###spiegelz"}};
    for (const auto& pk : punkte) {
        const std::string id = std::string(trK(pk.text)) + pk.id;
        QtPunkt p;
        p.id = id.c_str();
        p.art = QtArt::Exklusiv;
        p.haken = pk.achse < 0 ? !s.an : (s.an && s.achse == pk.achse);
        p.tooltip = trK(Str::KeSgTip);
        if (qtPunkt(p)) { spiegelSetzen(pk.achse); }
    }
    qtTrenner();
    {
        const std::string id = std::string(trK(Str::KeSgMitte)) + "###spiegelmitte";
        QtPunkt p;
        p.id = id.c_str();
        p.aktiv = e().karte.auswahl.anzahl() != 0;
        p.tooltip = trK(Str::KeSgMitteTip);
        if (qtPunkt(p)) { spiegelEbeneAnAuswahl(); }
    }
    {
        const std::string id = std::string(trK(Str::KeSgNull)) + "###spiegelnull";
        QtPunkt p;
        p.id = id.c_str();
        p.tooltip = trK(Str::KeSgNullTip);
        if (qtPunkt(p)) { spiegelEbeneNull(); }
    }
    qtMenueEnde();
}

void spiegelLinien(std::vector<PluginLinien>& v) {
    const karte::SpiegelModus& s = e().spiegel;
    if (!s.an) { return; }
    const float m = e().raster.maxKoord();
    const karte::Farbe f = karte::farbeAus(1, 0, 1);
    PluginLinien l;
    l.breite = 2.0F;
    const auto punkt = [&](float u, float w, int andere1, int andere2) {
        karte::Vektor3f p(0, 0, 0);
        p[static_cast<std::size_t>(s.achse)] = static_cast<float>(s.ebene);
        p[static_cast<std::size_t>(andere1)] = u;
        p[static_cast<std::size_t>(andere2)] = w;
        return p;
    };
    const int a1 = (s.achse + 1) % 3;
    const int a2 = (s.achse + 2) % 3;
    l.linien.push_back(karte::Linie{punkt(-m, 0, a1, a2), punkt(m, 0, a1, a2), f});
    l.linien.push_back(karte::Linie{punkt(0, -m, a1, a2), punkt(0, m, a1, a2), f});
    v.push_back(std::move(l));
}

}  // namespace bhed::gui::karteneditor
