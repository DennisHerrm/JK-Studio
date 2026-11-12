// modell_stapel.cpp - AP13S (MBENTWURF Teil B, Abgleich M1, M2, W1): der Modifier-Stapel einer Flaeche (bhed/modell/stapel.h)
//
//   Glaetten      Catmull-Clark am Wuerfel: 26 Punkte, 24 Vierecke, die bekannten Orte (Ecke 5/9, Kantenpunkt 3/4, Flaechenpunkt
//                 auf der Seite); an einer offenen Ebene bleiben die Raender (raenderHalten) bzw. folgen (A + 6P + B) / 8
//   Spiegeln      der Kasten an X: symmetrisch; liegt auf der Quellseite nichts, rechnet der Eintrag nicht (Grund KeineQuelle)
//                 und der Stapel gibt das Netz unveraendert weiter
//   Vorschau      stapelNetz (was die Ansicht zeigt) == das Netz nach stapelAnwenden - ein Eintrag, zwei Eintraege nacheinander
//   Schalter      Auge aus: rechnet nicht; "nur Vorschau": rechnet fuer die Ansicht, nicht fuer die Datei
//   Datei         ohne Eintrag und mit einem Eintrag "nur Vorschau" schreibt glmSchreiben die gelesene Datei Byte fuer Byte;
//                 stapelFuerDatei ohne wirksamen Eintrag gibt denselben Inhalt (dieselben Bytes); mit Spiegeln in der Datei
//                 traegt die geschriebene und wieder gelesene Flaeche das gespiegelte Netz
//   Figur         jede Flaeche von data/fixtures/model.glm geglaettet: gueltige Halbkanten, sechsmal so viele Dreiecke,
//                 Gewichte je Ecke Summe 1 und nur Knochen der Flaeche
//
// Aufruf: modellstapeltest <model.glm>
#include "bhed/glm.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/glmdatei.h"
#include "bhed/modell/stapel.h"
#include "bhed/modell/wof/poly.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("%s %s\n", ok ? "OK    " : "FEHLER", what.c_str());
    if (!ok) { ++fails; }
}
std::string slurp(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}
using namespace bhed;
using namespace bhed::modell;
std::size_t uz(int i) { return static_cast<std::size_t>(i); }

// Ein Inhalt mit einer Flaeche aus einem Grundkoerper (Kasten: x, y in [-l/2, l/2], z in [0, l]; Ebene in z = 0)
Inhalt grundInhalt(wof::Grundform form, float l) {
    Inhalt in;
    in.herkunft = Herkunft::Md3;
    in.stufenZahl = 1;
    Teil t;
    t.name = "form";
    t.stufen.resize(1);
    in.teile.push_back(t);
    wof::PolyNetz n = wof::polyAus(in, 0);
    wof::GrundParam g;
    g.form = form;
    g.laenge = g.breite = g.hoehe = l;
    (void)wof::grundkoerper(n, g, 0);
    wof::normalenNeu(n, 60.0F);
    n.verdichten();
    wof::polyZurueck(n, in);
    return in;
}

Modifikator eintrag(ModArt art, std::uint32_t kennung) {
    Modifikator m;
    m.art = art;
    m.kennung = kennung;
    m.datei = art == ModArt::Spiegeln;
    return m;
}

bool hatEcke(const Netz& n, float x, float y, float z) {
    for (const Ecke& e : n.ecken) {
        if (std::fabs(e.ort[0] - x) + std::fabs(e.ort[1] - y) + std::fabs(e.ort[2] - z) < 1e-4F) { return true; }
    }
    return false;
}

float groesste(const Netz& n, int achse, bool betrag = true) {
    float m = 0.0F;
    for (const Ecke& e : n.ecken) { m = std::max(m, betrag ? std::fabs(e.ort[achse]) : e.ort[achse]); }
    return m;
}

std::string zahlen(const Netz& n) {
    return std::to_string(n.punktZahl) + " Punkte, " + std::to_string(n.ecken.size()) + " Ecken, " + std::to_string(n.dreiecke()) + " Dreiecke";
}

}  // namespace

int main(int argc, char** argv) {
    const StapelUmgebung u;
    // --- Glaetten am Wuerfel ----------------------------------------------------------------------------------------------------
    {
        Inhalt in = grundInhalt(wof::Grundform::Kasten, 2.0F);
        const Netz& basis = in.teile[0].stufen[0];
        expect("Wuerfel: 8 Punkte, 12 Dreiecke (" + zahlen(basis) + ")", basis.punktZahl == 8 && basis.dreiecke() == 12);
        Netz g;
        wof::Ablehnung grund = wof::Ablehnung::Keine;
        expect("Glaetten 1: rechnet", modifikatorAnwenden(in.teile[0], basis, eintrag(ModArt::Glaetten, 1), u, g, &grund));
        std::string fehler;
        expect("Glaetten 1: 26 Punkte, 24 Vierecke = 48 Dreiecke, Halbkanten gueltig (" + zahlen(g) + " " + fehler + ")",
               g.punktZahl == 26 && g.dreiecke() == 48 && g.pruefen(&fehler) && g.randKanten() == 0);
        // Ecke (1, 1, 2) -> (Q + 2R) / 3 = 5/9 von der Mitte; Kantenpunkt 3/4; Flaechenpunkt auf der Seite
        const float e = 5.0F / 9.0F;
        expect("Glaetten 1: Ecke bei 5/9, Kantenpunkt bei 3/4, Flaechenpunkt in der Seitenmitte",
               hatEcke(g, e, e, 1.0F + e) && hatEcke(g, -e, e, 1.0F - e) && hatEcke(g, 0.75F, 0.75F, 1.0F) && hatEcke(g, 1.0F, 0.0F, 1.0F) &&
                   hatEcke(g, 0.0F, 0.0F, 2.0F));
        Modifikator zwei = eintrag(ModArt::Glaetten, 1);
        zwei.stufen = 2;
        Netz g2;
        expect("Glaetten 2: 98 Punkte, 96 Vierecke", modifikatorAnwenden(in.teile[0], basis, zwei, u, g2) && g2.punktZahl == 98 && g2.dreiecke() == 192);
        // 16 Polygone je Seite bleiben Vierecke (Netz::polyVon): das Arbeitsnetz des naechsten Eintrags sieht wieder Vierecke
        Inhalt nach = in;
        nach.teile[0].stufen[0] = g;
        expect("Glaetten 1: das Ergebnis sind Vierecke (24 Vielecke im Arbeitsnetz)", wof::polyAus(nach, 0).lebendeFlaechen() == 24);
    }
    // --- Glaetten an der offenen Ebene: Raender halten oder mitglaetten -----------------------------------------------------------
    {
        Inhalt in = grundInhalt(wof::Grundform::Ebene, 2.0F);
        const Netz& basis = in.teile[0].stufen[0];
        Modifikator m = eintrag(ModArt::Glaetten, 1);
        Netz halten;
        expect("Ebene: Raender halten - 9 Punkte, die Ecken bleiben bei 1", modifikatorAnwenden(in.teile[0], basis, m, u, halten) && halten.punktZahl == 9 &&
                                                                                 hatEcke(halten, 1.0F, 1.0F, 0.0F) && hatEcke(halten, 1.0F, 0.0F, 0.0F));
        m.raenderHalten = false;
        Netz frei;
        expect("Ebene: Raender mitglaetten - die Ecken bei (A + 6P + B) / 8 = 0,75", modifikatorAnwenden(in.teile[0], basis, m, u, frei) && frei.punktZahl == 9 &&
                                                                                           hatEcke(frei, 0.75F, 0.75F, 0.0F) && !hatEcke(frei, 1.0F, 1.0F, 0.0F) &&
                                                                                           hatEcke(frei, 1.0F, 0.0F, 0.0F));
    }
    // --- Spiegeln -----------------------------------------------------------------------------------------------------------------
    {
        Inhalt in = grundInhalt(wof::Grundform::Kasten, 2.0F);
        // eine Ecke der Plus-Seite herausziehen: nach dem Spiegeln steht ihr Gegenstueck auf der Minus-Seite
        for (Ecke& e : in.teile[0].stufen[0].ecken) {
            if (e.ort[0] > 0.5F && e.ort[1] > 0.5F && e.ort[2] > 1.5F) { e.ort[0] = 3.0F; }
        }
        const Netz& basis = in.teile[0].stufen[0];
        Netz s;
        std::string fehler;
        expect("Spiegeln X: rechnet, Halbkanten gueltig, geschlossen", modifikatorAnwenden(in.teile[0], basis, eintrag(ModArt::Spiegeln, 1), u, s) && s.pruefen(&fehler) &&
                                                                           s.randKanten() == 0);
        expect("Spiegeln X: die herausgezogene Ecke (3, 1, 2) hat ihr Gegenstueck (-3, 1, 2)", hatEcke(s, 3.0F, 1.0F, 2.0F) && hatEcke(s, -3.0F, 1.0F, 2.0F));
        Modifikator minus = eintrag(ModArt::Spiegeln, 1);
        minus.vonPlus = false;
        Netz t;
        expect("Spiegeln X von Minus nach Plus: die herausgezogene Ecke ist weg (groesstes x = 1)",
               modifikatorAnwenden(in.teile[0], basis, minus, u, t) && std::fabs(groesste(t, 0) - 1.0F) < 1e-4F);
        // der Kasten ganz auf der Minus-Seite: von Plus nach Minus hat keine Quelle
        Inhalt links = grundInhalt(wof::Grundform::Kasten, 2.0F);
        for (Ecke& e : links.teile[0].stufen[0].ecken) { e.ort[0] -= 5.0F; }
        Netz v;
        wof::Ablehnung grund = wof::Ablehnung::Keine;
        expect("Spiegeln ohne Quellseite: rechnet nicht, Grund KeineQuelle",
               !modifikatorAnwenden(links.teile[0], links.teile[0].stufen[0], eintrag(ModArt::Spiegeln, 1), u, v, &grund) && grund == wof::Ablehnung::KeineQuelle);
        links.teile[0].stapel = {eintrag(ModArt::Spiegeln, 1), eintrag(ModArt::Glaetten, 2)};
        std::vector<wof::Ablehnung> status;
        const Netz weiter = stapelNetz(links.teile[0], 0, StapelZweck::Ansicht, u, &status);
        expect("Stapel: der Eintrag ohne Quellseite wird uebersprungen (Status), der naechste rechnet (26 Punkte)",
               status.size() == 2 && status[0] == wof::Ablehnung::KeineQuelle && status[1] == wof::Ablehnung::Keine && weiter.punktZahl == 26);
        wof::Ablehnung ag = wof::Ablehnung::Keine;
        Inhalt kopie = links;
        expect("Anwenden eines Eintrags, der nicht rechnen kann: nichts geaendert, Grund genannt",
               !stapelAnwenden(kopie, 0, {0}, u, &ag) && ag == wof::Ablehnung::KeineQuelle && kopie.teile[0].stapel.size() == 2 &&
                   netzGleich(kopie.teile[0].stufen[0], links.teile[0].stufen[0]));
    }
    // --- Schalter: Auge, nur Vorschau -----------------------------------------------------------------------------------------------
    {
        Inhalt in = grundInhalt(wof::Grundform::Kasten, 2.0F);
        expect("leerer Stapel: leer, wirkt weder in der Ansicht noch in der Datei",
               stapelLeer(in) && !stapelWirkt(in, StapelZweck::Ansicht) && !stapelWirkt(in, StapelZweck::Datei) && stapelNeueKennung(in) == 1U);
        Modifikator g = eintrag(ModArt::Glaetten, stapelNeueKennung(in));
        in.teile[0].stapel.push_back(g);
        expect("Glaetten ab Werk nur Vorschau: wirkt in der Ansicht, nicht in der Datei",
               !stapelLeer(in) && stapelWirkt(in, StapelZweck::Ansicht) && !stapelWirkt(in, StapelZweck::Datei) && stapelNeueKennung(in) == 2U &&
                   stapelIndex(in.teile[0], 1U) == 0 && stapelIndex(in.teile[0], 7U) == -1);
        expect("nur Vorschau: das Netz fuer die Datei ist das Netz des Dokuments, das der Ansicht ist geglaettet",
               netzGleich(stapelNetz(in.teile[0], 0, StapelZweck::Datei, u), in.teile[0].stufen[0]) &&
                   stapelNetz(in.teile[0], 0, StapelZweck::Ansicht, u).punktZahl == 26);
        in.teile[0].stapel[0].an = false;
        in.teile[0].stapel[0].datei = true;
        expect("Auge aus: wirkt nirgends", !stapelWirkt(in, StapelZweck::Ansicht) && !stapelWirkt(in, StapelZweck::Datei) &&
                                               netzGleich(stapelNetz(in.teile[0], 0, StapelZweck::Ansicht, u), in.teile[0].stufen[0]));
        in.teile[0].stapel[0].an = true;
        expect("Auge an und in die Datei: wirkt in beiden", stapelWirkt(in, StapelZweck::Ansicht) && stapelWirkt(in, StapelZweck::Datei));
        Inhalt tag = in;
        tag.teile[0].flaggen |= GlmSurface::kFlagBolt;
        Netz aus;
        wof::Ablehnung grund = wof::Ablehnung::Keine;
        expect("ein Tag bekommt keinen Eintrag gerechnet", !stapelWirkt(tag, StapelZweck::Ansicht) &&
                                                               !modifikatorAnwenden(tag.teile[0], tag.teile[0].stufen[0], g, u, aus, &grund) && grund == wof::Ablehnung::Tag);
    }
    // --- Vorschau = angewendet --------------------------------------------------------------------------------------------------------
    {
        Inhalt in = grundInhalt(wof::Grundform::Kasten, 2.0F);
        in.teile[0].stapel = {eintrag(ModArt::Glaetten, 1)};
        const Netz vorschau = stapelNetz(in.teile[0], 0, StapelZweck::Ansicht, u);
        const int standVorher = in.netzStand;
        expect("ein Eintrag: anwenden", stapelAnwenden(in, 0, {0}, u));
        expect("ein Eintrag: das Netz des Dokuments ist das der Vorschau (Ecke fuer Ecke, Dreieck fuer Dreieck), der Stapel ist leer",
               netzGleich(in.teile[0].stufen[0], vorschau) && in.teile[0].stapel.empty() && in.netzStand != standVorher);
        // zwei Eintraege: die Vorschau beider == erst den ersten anwenden, dann die Vorschau des zweiten == beide angewendet
        Inhalt zwei = grundInhalt(wof::Grundform::Kasten, 2.0F);
        for (Ecke& e : zwei.teile[0].stufen[0].ecken) {
            if (e.ort[0] > 0.5F && e.ort[1] > 0.5F && e.ort[2] > 1.5F) { e.ort[0] = 3.0F; }
        }
        zwei.teile[0].stapel = {eintrag(ModArt::Spiegeln, 1), eintrag(ModArt::Glaetten, 2)};
        const Netz beide = stapelNetz(zwei.teile[0], 0, StapelZweck::Ansicht, u);
        Inhalt schritt = zwei;
        expect("zwei Eintraege: den ersten anwenden", stapelAnwenden(schritt, 0, {0}, u) && schritt.teile[0].stapel.size() == 1 &&
                                                          schritt.teile[0].stapel[0].kennung == 2U);
        expect("zwei Eintraege: die Vorschau danach ist dieselbe wie davor",
               netzGleich(stapelNetz(schritt.teile[0], 0, StapelZweck::Ansicht, u), beide));
        expect("zwei Eintraege: den zweiten anwenden - das Dokument traegt die Vorschau", stapelAnwenden(schritt, 0, {0}, u) &&
                                                                                           netzGleich(schritt.teile[0].stufen[0], beide));
        Inhalt aufEinmal = zwei;
        expect("zwei Eintraege auf einmal angewendet: dasselbe Netz", stapelAnwenden(aufEinmal, 0, {0, 1}, u) && aufEinmal.teile[0].stapel.empty() &&
                                                                          netzGleich(aufEinmal.teile[0].stufen[0], beide));
        Inhalt falsch = zwei;
        expect("ungueltige Indizes: nichts geaendert", !stapelAnwenden(falsch, 0, {1, 0}, u) && !stapelAnwenden(falsch, 0, {2}, u) &&
                                                           !stapelAnwenden(falsch, 3, {0}, u) && falsch.teile[0].stapel.size() == 2);
    }
    // --- die Figur: Datei und Gewichte ----------------------------------------------------------------------------------------------
    if (argc > 1) {
        const std::string bytes = slurp(argv[1]);
        Inhalt in;
        std::string fehler;
        expect("model.glm gelesen (" + fehler + ")", ausGlm(bytes, in, &fehler));
        std::string roh;
        expect("leerer Stapel: glmSchreiben gibt die gelesene Datei Byte fuer Byte", glmSchreiben(in, roh) && roh == bytes);
        const Inhalt datei = stapelFuerDatei(in, u);
        std::string ueber;
        expect("leerer Stapel: stapelFuerDatei aendert nichts (dieselben Bytes)", glmSchreiben(datei, ueber) && ueber == bytes);
        int flaechen = 0;
        int gueltig = 0;
        int sechsfach = 0;
        int gewichte = 0;
        int knochenPassen = 0;
        int spiegelTeil = -1;
        int spiegelDreiecke = 0;
        for (std::size_t t = 0; t < in.teile.size(); ++t) {
            const Teil& teil = in.teile[t];
            if (teil.istTag() || teil.stufen.empty() || teil.stufen[0].dreiecke() == 0) { continue; }
            ++flaechen;
            Netz g;
            if (!modifikatorAnwenden(teil, teil.stufen[0], eintrag(ModArt::Glaetten, 1), u, g)) { continue; }
            gueltig += g.pruefen() ? 1 : 0;
            sechsfach += g.dreiecke() == teil.stufen[0].dreiecke() * 6 ? 1 : 0;
            std::set<int> erlaubt;
            for (const Ecke& e : teil.stufen[0].ecken) {
                for (int k = 0; k < 4; ++k) {
                    if (e.gewicht[k] > 0.0F) { erlaubt.insert(e.knochen[k]); }
                }
            }
            bool summe = true;
            bool passt = true;
            for (const Ecke& e : g.ecken) {
                float s = 0.0F;
                for (int k = 0; k < 4; ++k) {
                    s += e.gewicht[k];
                    passt = passt && (e.gewicht[k] <= 0.0F || erlaubt.count(e.knochen[k]) != 0U);
                }
                summe = summe && std::fabs(s - 1.0F) < 1e-4F;
            }
            gewichte += summe ? 1 : 0;
            knochenPassen += passt ? 1 : 0;
            // fuer die Pruefung der Datei: die groesste Flaeche, die sich an X spiegeln laesst
            Netz s;
            if (teil.stufen[0].dreiecke() > spiegelDreiecke && modifikatorAnwenden(teil, teil.stufen[0], eintrag(ModArt::Spiegeln, 1), u, s)) {
                spiegelTeil = static_cast<int>(t);
                spiegelDreiecke = teil.stufen[0].dreiecke();
            }
        }
        expect("Figur: " + std::to_string(flaechen) + " Flaechen geglaettet - Halbkanten gueltig " + std::to_string(gueltig) + ", sechsmal so viele Dreiecke " +
                   std::to_string(sechsfach) + ", Gewichte Summe 1 " + std::to_string(gewichte) + ", nur Knochen der Flaeche " + std::to_string(knochenPassen),
               flaechen > 0 && gueltig == flaechen && sechsfach == flaechen && gewichte == flaechen && knochenPassen == flaechen);
        // nur Vorschau: die Datei bleibt Byte fuer Byte
        Inhalt vorschau = in;
        for (Teil& teil : vorschau.teile) {
            if (!teil.istTag()) { teil.stapel.push_back(eintrag(ModArt::Glaetten, stapelNeueKennung(vorschau))); }
        }
        std::string mitVorschau;
        expect("Eintraege nur Vorschau: nichts geht in die Datei, glmSchreiben schreibt die gelesene Datei Byte fuer Byte",
               !stapelWirkt(vorschau, StapelZweck::Datei) && stapelWirkt(vorschau, StapelZweck::Ansicht) && glmSchreiben(vorschau, mitVorschau) && mitVorschau == bytes);
        // Spiegeln in der Datei: geschrieben und wieder gelesen traegt die Flaeche das gespiegelte Netz
        expect("Figur: eine Flaeche laesst sich an X spiegeln", spiegelTeil >= 0);
        if (spiegelTeil >= 0) {
            Inhalt sp = in;
            sp.teile[uz(spiegelTeil)].stapel = {eintrag(ModArt::Spiegeln, 1)};
            const Netz soll = stapelNetz(sp.teile[uz(spiegelTeil)], 0, StapelZweck::Datei, u);
            const Inhalt fuerDatei = stapelFuerDatei(sp, u);
            std::string geschrieben;
            Inhalt gelesen;
            const bool ok = glmSchreiben(fuerDatei, geschrieben) && ausGlm(geschrieben, gelesen, &fehler);
            expect("Spiegeln in der Datei: geschrieben, gelesen (" + fehler + ")", ok && geschrieben != bytes && fuerDatei.teile[uz(spiegelTeil)].stapel.empty());
            if (ok) {
                const Netz& ist = gelesen.teile[uz(spiegelTeil)].stufen[0];
                expect("Spiegeln in der Datei: die Flaeche " + in.teile[uz(spiegelTeil)].name + " traegt das Netz des Stapels (" + zahlen(ist) + " gegen " + zahlen(soll) + ")",
                       ist.ecken.size() == soll.ecken.size() && ist.dreiecke() == soll.dreiecke() && ist.indizes() == soll.indizes());
                // die anderen Flaechen der Stufe 0 bleiben, wie sie waren
                bool andere = true;
                for (std::size_t t = 0; t < in.teile.size() && t < gelesen.teile.size(); ++t) {
                    if (static_cast<int>(t) != spiegelTeil && !in.teile[t].stufen.empty() && !gelesen.teile[t].stufen.empty()) {
                        andere = andere && netzGleich(in.teile[t].stufen[0], gelesen.teile[t].stufen[0]);
                    }
                }
                expect("Spiegeln in der Datei: die uebrigen Flaechen sind unveraendert", andere);
            }
        }
    } else {
        expect("Aufruf mit model.glm", false);
    }
    std::printf("%d Fehler\n", fails);
    return fails == 0 ? 0 : 1;
}
