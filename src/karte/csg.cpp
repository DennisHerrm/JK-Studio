// csg.cpp - Karten-Editor: CSG und das Teilen durch eine Ebene.
// Portiert aus NetRadiant-Custom 20260114 (Quellen in csg.h; Zeilen: csg.cpp).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/csg.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/komponenten.h"
#include "bhed/karte/texdef.h"

#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif
#include "../../third_party/quickhull/QuickHull.hpp"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <algorithm>
#include <cctype>
#include <cmath>

namespace bhed::karte {

namespace {

using PinselListe = std::vector<std::shared_ptr<Pinsel>>;

struct PinselOrt {
    std::shared_ptr<Entitaet> entitaet;
    std::shared_ptr<Pinsel> pinsel;
};

bool gleicherShader(const std::string& a, const std::string& b) {   // shader_equal
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}

// aabb_intersects_aabb (libs/math/aabb.h:149-157)
bool kastenSchneiden(const Kasten& a, const Kasten& b) {
    for (std::size_t i = 0; i < 3; ++i) {
        if (!(std::fabs(b.origin[i] - a.origin[i]) < (a.extents[i] + b.extents[i]))) { return false; }
    }
    return true;
}

// BrushGatherSelected (csg.cpp:374-387): sichtbare gewaehlte Pinsel in der Reihenfolge der Szene
std::vector<PinselOrt> gewaehltePinsel(const Karte& k) {
    std::vector<PinselOrt> r;
    for (const auto& ent : k.entitaeten()) {
        if (ent->versteckt) { continue; }
        for (const auto& p : ent->kinder()) {
            if (p->versteckt || p->art() != Primitiv::Art::Pinsel || !k.auswahl.gewaehlt(ent.get(), p.get())) { continue; }
            r.push_back({ent, std::static_pointer_cast<Pinsel>(p)});
        }
    }
    return r;
}

// Path_deleteTop fuer einen Pinsel: aus der Entitaet nehmen, abwaehlen.
void pinselEntfernen(Karte& k, const std::shared_ptr<Entitaet>& ent, const std::shared_ptr<Pinsel>& p) {
    k.auswahl.abwaehlen(ent, p);
    for (const auto& f : p->flaechen()) { k.auswahl.flaecheAbwaehlen(f.get()); }
    ent->kindEntfernen(p.get());
}

// BrushDeleteSelected (csg.cpp:409-437): sichtbare gewaehlte Pinsel ausser
// "behalten" loeschen; leer gewordene Entitaeten (nicht worldspawn, nicht
// "behaltenEnt") mit.
void gewaehltePinselLoeschen(Karte& k, const Primitiv* behalten = nullptr, const Entitaet* behaltenEnt = nullptr) {
    const auto welt = k.welt();
    const auto liste = k.entitaeten();
    for (const auto& ent : liste) {
        bool elternLoeschen = false;
        const auto kinder = ent->kinder();
        for (const auto& p : kinder) {
            if (p->art() != Primitiv::Art::Pinsel) { continue; }
            if (!ent->versteckt && !p->versteckt && k.auswahl.gewaehlt(ent.get(), p.get()) && p.get() != behalten) {
                pinselEntfernen(k, ent, std::static_pointer_cast<Pinsel>(p));
                elternLoeschen = ent->kinder().empty();
            }
        }
        if (elternLoeschen && ent != welt && ent.get() != behaltenEnt) {
            k.auswahl.abwaehlen(ent);
            k.entitaetEntfernen(ent.get());
        }
    }
}

// Neuer Pinsel (GlobalBrushCreator().createBrush()) als Kopie, in die Entitaet.
std::shared_ptr<Pinsel> pinselEinfuegen(Karte& k, const std::shared_ptr<Entitaet>& ent, const Pinsel& vorlage) {
    auto neu = vorlage.kopie(k.kontext);   // Node_getBrush( node )->copy( *brush )
    neu->schicht = static_cast<int>(k.aktuelleSchicht);
    ent->kindEinfuegen(neu);
    return neu;
}

// ultimate_group_path (csg.cpp:919-935)
std::shared_ptr<Entitaet> letzteGruppe(Karte& k) {
    if (k.auswahl.anzahl() != 0) {
        const AuswahlElement& el = k.auswahl.elemente().back();
        auto ent = el.entitaet.lock();
        if (el.primitiv.lock()) { return ent; }   // Primitiv: die Entitaet darueber
        if (ent && ent->istBehaelter()) { return ent; }   // Gruppenentitaet
    }
    return k.weltHolenOderAnlegen();
}

// HollowSettings::faceExcluded / excludeFaces (csg.cpp:86-120)
struct Ausschluss {
    const HohlEinstellung& e;
    double minSkalar = 0;
    double maxSkalar = 0;
    std::vector<Vektor3d> normalen;   // m_exclude_vec
    [[nodiscard]] bool ausgenommen(const Flaeche& f) const {
        if (e.nachAchse) {
            const double s = punkt(f.ebene().normal(), e.achse);
            return s < minSkalar + 0.001 || s > maxSkalar - 0.001;
        }
        // genauer Vergleich wie NRC (kann mit veraenderten Flaechen scheitern)
        return std::find(normalen.begin(), normalen.end(), f.ebene().normal()) != normalen.end();
    }
    void sammeln(const Karte& k, const Pinsel& p) {
        if (e.nachAchse) {
            minSkalar = maxSkalar = 0;
            for (const auto& f : p.flaechen()) {
                const double s = punkt(f->ebene().normal(), e.achse);
                if (s < minSkalar) {
                    minSkalar = s;
                } else if (s > maxSkalar) {
                    maxSkalar = s;
                }
            }
        } else {
            normalen.clear();
            for (const auto& f : p.flaechen()) {
                if (e.gewaehlteAusschliessen == k.auswahl.flaecheGewaehlt(f.get())) { normalen.push_back(f->ebene().normal()); }
            }
        }
    }
};

// CaulkFace (csg.cpp:123-134)
void caulkFlaechen(Pinsel& p, const Ausschluss& a, const CsgUmgebung& u, const Kontext& k) {
    for (const auto& f : p.flaechen()) {
        if (!a.ausgenommen(*f)) { f->setzeShader(u.caulkShader, k); }
    }
}

// brush_extrudeDiag (csg.cpp:219-299)
void diagonalExtrudieren(const Pinsel& p0, const Pinsel& p2, PinselListe& aus, const Ausschluss& a, const CsgUmgebung& u, const Kontext& k) {
    TexturProjektion projektion;
    texdefStandard(k.art(), projektion);
    const std::string shader = a.e.caulk ? u.caulkShader : u.browserShader;
    p0.bRep(k);
    p2.bRep(k);
    for (std::size_t i = 0; i < p0.anzahl(); ++i) {
        const Flaeche& f0 = p0[i];
        const Flaeche& f2 = p2[i];
        if (a.ausgenommen(f0) || !f0.traegtBei()) { continue; }
        auto neu = p0.kopie(k);
        neu->leeren();
        aus.push_back(neu);
        if (Flaeche* g = neu->flaecheHinzufuegen(f0, k)) {
            if (!a.e.innenEntfernen && a.e.caulk) { g->setzeShader(shader, k); }
            g->umdrehen();
        }
        if (f2.traegtBei()) {   // zwei gueltige Wicklungen vernaehen
            neu->flaecheHinzufuegen(f2, k);
            const auto seiten = [&](const Wicklung& w0, const Wicklung& w2, const Vektor3d& normale, bool tauschen) {
                for (std::size_t x = 0; x < w0.numpoints; ++x) {
                    const std::size_t naechster = wicklungNaechster(w0, x);
                    Vektor3d bester;
                    double besterSkalar = -1;
                    for (std::size_t y = 0; y < w2.numpoints; ++y) {
                        const double s = punkt(normiert(kreuz(w0[x].vertex - w0[naechster].vertex, w0[x].vertex - w2[y].vertex)), normale);
                        if (s > besterSkalar) {
                            besterSkalar = s;
                            bester = w2[y].vertex;
                        }
                    }
                    neu->ebeneHinzufuegen(w0[tauschen ? naechster : x].vertex, w0[tauschen ? x : naechster].vertex, bester, shader, projektion, k);
                }
            };
            seiten(f0.wicklung(), f2.wicklung(), f0.ebene().normal(), false);
            seiten(f2.wicklung(), f0.wicklung(), f0.ebene().normal(), true);
        } else {   // eine gueltige Wicklung
            const Wicklung& w0 = f0.wicklung();
            for (std::size_t x = 0; x < w0.numpoints; ++x) {
                const std::size_t naechster = wicklungNaechster(w0, x);
                Vektor3d bester;
                double besterAbstand = 999999;
                for (const auto& g : p2.flaechen()) {
                    const Wicklung& w2 = g->wicklung();
                    for (std::size_t y = 0; y < w2.numpoints; ++y) {
                        const double d = laenge(w0[x].vertex - w2[y].vertex);
                        if (d < besterAbstand && abstandZuPunkt(f0.ebene(), w2[y].vertex) > .05) {
                            besterAbstand = d;
                            bester = w2[y].vertex;
                        }
                    }
                }
                neu->ebeneHinzufuegen(w0[x].vertex, w0[naechster].vertex, bester, shader, projektion, k);
            }
        }
    }
}

// FaceMakeBrush (csg.cpp:136-217) ueber alle Flaechen von "pinsel"
void flaechenZuPinseln(Pinsel& pinsel, PinselListe& aus, const Ausschluss& a, const CsgUmgebung& u, const Kontext& k) {
    const HohlEinstellung& e = a.e;
    for (std::size_t i = 0; i < pinsel.anzahl(); ++i) {
        Flaeche& f = pinsel[i];
        if (a.ausgenommen(f)) { continue; }
        if (e.art == HohlArt::Ziehen) {
            if (!f.traegtBei()) { continue; }
            f.versetzen(e.versatz);
            aus.push_back(pinsel.kopie(k));
            f.versetzen(-e.versatz);
            if (e.caulk) { caulkFlaechen(*aus.back(), a, u, k); }
            if (Flaeche* g = aus.back()->flaecheHinzufuegen(f, k)) { g->umdrehen(); }
        } else if (e.art == HohlArt::Huelle) {
            if (!f.traegtBei()) { continue; }
            aus.push_back(pinsel.kopie(k));
            if (!e.innenEntfernen && e.caulk) { f.setzeShader(u.caulkShader, k); }
            Flaeche* g = aus.back()->flaecheHinzufuegen(f, k);
            f.versetzen(-e.versatz);
            if (e.caulk) { f.setzeShader(u.caulkShader, k); }
            if (g != nullptr) {
                g->umdrehen();
                g->versetzen(e.versatz);
            }
        } else if (e.art == HohlArt::Extrudieren) {
            if (!f.traegtBei()) { continue; }
            auto neu = pinsel.kopie(k);
            neu->leeren();
            aus.push_back(neu);
            if (Flaeche* g = neu->flaecheHinzufuegen(f, k)) { g->versetzen(e.versatz); }
            if (!e.innenEntfernen && e.caulk) { f.setzeShader(u.caulkShader, k); }
            if (Flaeche* g = neu->flaecheHinzufuegen(f, k)) { g->umdrehen(); }
            const Wicklung& w = f.wicklung();
            TexturProjektion projektion;
            texdefStandard(k.art(), projektion);
            for (std::size_t x = 0; x < w.numpoints; ++x) {
                const std::size_t naechster = wicklungNaechster(w, x);
                neu->ebeneHinzufuegen(w[x].vertex, w[naechster].vertex, w[naechster].vertex + f.ebene().normal() * static_cast<double>(e.versatz),
                                      u.browserShader, projektion, k);
            }
        }
    }
}

// BrushHollowSelectedWalker (csg.cpp:317-372)
void aushoehlenAlle(Karte& k, const CsgUmgebung& u, const HohlEinstellung& e) {
    const Kontext& kt = k.kontext;
    Ausschluss a{e};
    const auto liste = k.entitaeten();
    for (const auto& ent : liste) {
        if (ent->versteckt) { continue; }
        const auto kinder = ent->kinder();
        for (const auto& prim : kinder) {
            if (prim->versteckt || prim->art() != Primitiv::Art::Pinsel) { continue; }
            const auto pinsel = std::static_pointer_cast<Pinsel>(prim);
            if (!k.auswahl.gewaehlt(ent.get(), pinsel.get()) && !pinselKomponentenGewaehlt(k, *pinsel)) { continue; }
            pinsel->bRep(kt);
            a.sammeln(k, *pinsel);
            PinselListe aus;
            if (e.art == HohlArt::Ziehen) {
                if (!e.innenEntfernen && e.caulk) { caulkFlaechen(*pinsel, a, u, kt); }
                auto tmp = pinsel->kopie(kt);
                tmp->leereFlaechenEntfernen(kt);
                tmp->bRep(kt);
                flaechenZuPinseln(*tmp, aus, a, u, kt);
            } else if (e.art == HohlArt::Diagonal) {
                auto tmp = pinsel->kopie(kt);
                for (const auto& f : tmp->flaechen()) {   // FaceOffset
                    if (!a.ausgenommen(*f)) { f->versetzen(e.versatz); }
                }
                tmp->bRep(kt);
                diagonalExtrudieren(*pinsel, *tmp, aus, a, u, kt);
                if (!e.innenEntfernen && e.caulk) { caulkFlaechen(*pinsel, a, u, kt); }
            } else {
                flaechenZuPinseln(*pinsel, aus, a, u, kt);
            }
            for (const auto& b : aus) {
                b->leereFlaechenEntfernen(kt);
                if (b->hatBeitragendeFlaechen(kt)) { (void)pinselEinfuegen(k, ent, *b); }
            }
        }
    }
}

// Brush_subtract (csg.cpp:545-572)
bool pinselSubtrahieren(const Pinsel& p, const Pinsel& anderer, const std::vector<const Flaeche*>& andereFlaechen, PinselListe& teile, const Kontext& k) {
    if (!kastenSchneiden(p.grenzen(k), anderer.grenzen(k))) { return false; }
    PinselListe fragmente;
    auto hinten = p.kopie(k);
    for (const Flaeche* f : andereFlaechen) {
        const PinselTeilung t = pinselEinordnen(*hinten, k, f->ebene());
        if (t.counts[kVorne] != 0 && t.counts[kHinten] != 0) {
            fragmente.push_back(hinten->kopie(k));
            if (Flaeche* g = fragmente.back()->flaecheHinzufuegen(*f, k)) { g->umdrehen(); }
            hinten->flaecheHinzufuegen(*f, k);
        } else if (t.counts[kHinten] == 0) {
            return false;
        }
    }
    teile.insert(teile.end(), fragmente.begin(), fragmente.end());
    return true;
}

// MergeVertices (csg.cpp:983-1017)
struct EckenMenge {
    std::vector<Vektor3d> ecken;
    void dazu(const Vektor3d& v) {
        if (!enthaelt(v)) { ecken.push_back(v); }
    }
    [[nodiscard]] bool enthaelt(const Vektor3d& v) const {
        return std::any_of(ecken.begin(), ecken.end(), [&](const Vektor3d& w) { return kanteEntartet(v, w); });
    }
    [[nodiscard]] PinselTeilung ebeneEinordnen(const Ebene3d& e) const {
        PinselTeilung t;
        for (const Vektor3d& v : ecken) {
            punktEinordnen(v, e, t);
            if (t.counts[kVorne] != 0 && t.counts[kHinten] != 0) { break; }   // die Ebene liegt innen
        }
        return t;
    }
};

// MergePlane(s) (csg.cpp:1038-1069)
struct HuellenEbene {
    Ebene3d ebene;
    const Flaeche* flaeche = nullptr;
    EbenenPunkte punkte{};
};
void ebeneDazu(std::vector<HuellenEbene>& ebenen, const HuellenEbene& e) {
    if (std::none_of(ebenen.begin(), ebenen.end(), [&](const HuellenEbene& x) { return ebenenGleich(e.ebene, x.ebene); })) { ebenen.push_back(e); }
}

// CSG_build_hull (csg.cpp:1071-1113), der QuickHull-Zweig
void huelleBauen(const EckenMenge& ecken, std::vector<HuellenEbene>& ebenen) {
    quickhull::QuickHull<double> huelle;
    std::vector<quickhull::Vector3<double>> wolke;
    wolke.reserve(ecken.ecken.size());
    for (const Vektor3d& v : ecken.ecken) { wolke.emplace_back(v.x(), v.y(), v.z()); }
    auto ergebnis = huelle.getConvexHull(wolke, true, true);
    const auto& indizes = ergebnis.getIndexBuffer();
    const std::size_t dreiecke = indizes.size() / 3;
    for (std::size_t i = 0; i < dreiecke; ++i) {
        EbenenPunkte punkte{};
        for (std::size_t j = 0; j < 3; ++j) { punkte[j] = ecken.ecken[indizes[i * 3 + j]]; }
        const Ebene3d e = ebeneAusPunkten(punkte);
        if (ebeneGueltig(e)) { ebeneDazu(ebenen, HuellenEbene{e, nullptr, punkte}); }
    }
}

}  // namespace

PinselTeilung pinselEinordnen(const Pinsel& p, const Kontext& k, const Ebene3d& e) {
    p.bRep(k);
    PinselTeilung t;
    for (const auto& f : p.flaechen()) {
        if (f->traegtBei()) { t += wicklungEinordnen(f->wicklung(), e); }
    }
    return t;
}

void csgSubtrahieren(Karte& k, Protokoll& p) {
    const Kontext& kt = k.kontext;
    const std::vector<PinselOrt> gewaehlt = gewaehltePinsel(k);
    if (gewaehlt.empty()) {
        warnung(p, "CSG Subtract: No brushes selected.\n");
        return;
    }
    ausgabe(p, "CSG Subtract: Subtracting " + std::to_string(gewaehlt.size()) + " brushes.\n");
    Befehl befehl(k.undo, "brushSubtract");
    // SubtractBrushesFromUnselected (csg.cpp:574-708): die Flaechen jedes
    // gewaehlten Pinsels in einer guenstigen Reihenfolge
    std::vector<std::vector<const Flaeche*>> pinselFlaechen;
    pinselFlaechen.reserve(gewaehlt.size());
    for (const PinselOrt& g : gewaehlt) {
        g.pinsel->bRep(kt);
        std::vector<Projektionsachse> erledigt;
        auto& flaechen = pinselFlaechen.emplace_back();
        for (const auto& f : g.pinsel->flaechen()) {
            if (f->traegtBei()) { flaechen.push_back(f.get()); }
        }
        const auto achse = [](const Flaeche* f) { return projektionsachseFuerNormale(Vektor3f(f->ebene().normal())); };
        std::sort(flaechen.begin(), flaechen.end(), [&](const Flaeche* a, const Flaeche* b) {
            const Vektor3d& n1 = a->ebene().normal();
            const Vektor3d& n2 = b->ebene().normal();
            const Projektionsachse p1 = achse(a);
            const Projektionsachse p2 = achse(b);
            const double b1 = std::fabs(n1[static_cast<std::size_t>(p1)]);
            const double b2 = std::fabs(n2[static_cast<std::size_t>(p2)]);
            return gleichEps(b1, b2, kEbeneNormaleEps) ? p1 > p2   // Z > Y > X
                                                       : b1 > b2;  // oder die achsnaehere
        });
        auto it = flaechen.begin();
        while (true) {   // die Projektionen durchgehen und eine guenstigere Teilungsfolge bauen
            const auto gefunden = std::find_if(flaechen.begin(), flaechen.end(), [&](const Flaeche* f) {
                return std::find(erledigt.begin(), erledigt.end(), achse(f)) == erledigt.end();
            });
            if (gefunden == flaechen.end()) { break; }
            const auto vor = [&](std::vector<const Flaeche*>::iterator wer) {   // eine Flaeche nach vorn ziehen
                const Flaeche* f = *wer;
                const auto idx = static_cast<std::ptrdiff_t>(it - flaechen.begin());
                flaechen.erase(wer);
                it = flaechen.begin() + idx;
                it = flaechen.insert(it, f) + 1;
            };
            vor(gefunden);
            const Vektor3d& n1 = (*(it - 1))->ebene().normal();
            const Projektionsachse p1 = achse(*(it - 1));
            erledigt.push_back(p1);
            auto mehr = flaechen.end();   // die gegenueberliegende Flaeche
            double besterBetrag = 0;
            double besterSkalar = 1;
            for (auto f = it; f != flaechen.end(); ++f) {
                const Vektor3d& n2 = (*f)->ebene().normal();
                const Projektionsachse p2 = achse(*f);
                const auto i1 = static_cast<std::size_t>(p1);
                const auto i2 = static_cast<std::size_t>(p2);
                if (p1 == p2 && n1[i1] * n2[i2] < 0 &&
                    (std::fabs(n2[i2]) > besterBetrag + kEbeneNormaleEps ||
                     (std::fabs(n2[i2]) > besterBetrag - kEbeneNormaleEps && punkt(n1, n2) < besterSkalar))) {
                    besterBetrag = std::fabs(n2[i2]);
                    besterSkalar = punkt(n1, n2);
                    mehr = f;
                }
            }
            if (mehr != flaechen.end()) { vor(mehr); }
        }
    }
    std::size_t vorher = 0;
    std::size_t nachher = 0;
    const auto welt = k.welt();
    const auto liste = k.entitaeten();
    for (const auto& ent : liste) {
        if (ent->versteckt) { continue; }
        bool elternLoeschen = false;
        const auto kinder = ent->kinder();
        for (const auto& prim : kinder) {
            if (prim->art() != Primitiv::Art::Pinsel || prim->versteckt || k.auswahl.gewaehlt(ent.get(), prim.get())) { continue; }
            const auto pinsel = std::static_pointer_cast<Pinsel>(prim);
            if (std::none_of(gewaehlt.begin(), gewaehlt.end(), [&](const PinselOrt& g) { return kastenSchneiden(pinsel->grenzen(kt), g.pinsel->grenzen(kt)); })) {
                continue;
            }
            PinselListe puffer[2];
            bool tausch = false;
            auto original = pinsel->kopie(kt);
            puffer[tausch].push_back(original);
            for (std::size_t i = 0; i < gewaehlt.size(); ++i) {
                for (const auto& b : puffer[tausch]) {
                    if (!pinselSubtrahieren(*b, *gewaehlt[i].pinsel, pinselFlaechen[i], puffer[!tausch], kt)) { puffer[!tausch].push_back(b); }
                }
                puffer[tausch].clear();
                tausch = !tausch;
            }
            PinselListe& aus = puffer[tausch];
            if (aus.size() == 1 && aus.back() == original) { continue; }
            ++vorher;
            for (const auto& b : aus) {
                ++nachher;
                b->leereFlaechenEntfernen(kt);
                if (!b->leer()) { (void)pinselEinfuegen(k, ent, *b); }
            }
            pinselEntfernen(k, ent, pinsel);
            elternLoeschen = ent->kinder().empty();
        }
        if (elternLoeschen && ent != welt) { k.entitaetEntfernen(ent.get()); }
    }
    ausgabe(p, "CSG Subtract: Result: " + std::to_string(nachher) + (nachher == 1 ? " fragment" : " fragments") + " from " + std::to_string(vorher) +
                   (vorher == 1 ? " brush.\n" : " brushes.\n"));
}

bool pinselVereinigen(Pinsel& ziel, const std::vector<std::shared_ptr<Pinsel>>& ein, bool nurForm, const Kontext& k) {
    std::vector<const Flaeche*> flaechen;   // moegliche Aussenflaechen
    for (std::size_t i = 0; i < ein.size(); ++i) {
        ein[i]->bRep(k);
        for (const auto& f1p : ein[i]->flaechen()) {
            if (!f1p->traegtBei()) { continue; }
            const Flaeche& f1 = *f1p;
            bool weg = false;
            for (std::size_t j = 0; !weg && j < ein.size(); ++j) {   // die Flaechen der anderen Pinsel
                if (j == i) { continue; }
                for (const auto& f2 : ein[j]->flaechen()) {
                    if (ebenenGegenueber(f1.ebene(), f2->ebene())) {   // gegenueberliegende Ebenen fallen weg
                        weg = true;
                        break;
                    }
                }
            }
            for (std::size_t m = 0; !weg && m < flaechen.size(); ++m) {   // schon gesammelte
                const Flaeche& f2 = *flaechen[m];
                if (ebenenGleich(f1.ebene(), f2.ebene())) {
                    if (!nurForm && !gleicherShader(f1.shader(), f2.shader())) { return false; }
                    weg = true;   // doppelte Ebene
                    break;
                }
                if (wicklungenKonkav(f1.wicklung(), f2.wicklung(), f1.ebene(), f2.ebene())) { return false; }   // nicht konvex
            }
            if (!weg) { flaechen.push_back(&f1); }
        }
    }
    for (const Flaeche* f : flaechen) {
        if (ziel.flaecheHinzufuegen(*f, k) == nullptr) { return false; }   // zu viele Seiten
    }
    ziel.leereFlaechenEntfernen(k);
    return true;
}

void csgVereinigen(Karte& k, Protokoll& p) {
    const std::vector<PinselOrt> gewaehlt = gewaehltePinsel(k);
    if (gewaehlt.empty()) {
        warnung(p, "CSG Merge: No brushes selected.\n");
        return;
    }
    if (gewaehlt.size() < 2) {
        warnung(p, "CSG Merge: At least two brushes have to be selected.\n");
        return;
    }
    ausgabe(p, "CSG Merge: Merging " + std::to_string(gewaehlt.size()) + " brushes.\n");
    Befehl befehl(k.undo, "brushMerge");
    auto neu = std::make_shared<Pinsel>();
    PinselListe ein;
    for (const PinselOrt& g : gewaehlt) { ein.push_back(g.pinsel); }
    if (!pinselVereinigen(*neu, ein, true, k.kontext)) {
        warnung(p, "CSG Merge: Failed - result would not be convex.\n");
        return;
    }
    const auto gruppe = letzteGruppe(k);
    gewaehltePinselLoeschen(k, nullptr, gruppe.get());   // die Ausgangspinsel weg
    neu->schicht = static_cast<int>(k.aktuelleSchicht);
    gruppe->kindEinfuegen(neu);
    k.auswahl.waehlen(gruppe, neu);   // selectPath( path, true )
    ausgabe(p, "CSG Merge: Succeeded.\n");
}

void csgHuelleBauen(Karte& k, const CsgUmgebung& u, const ClipperPunkte& punkte, Protokoll& p) {
    const Kontext& kt = k.kontext;
    if (k.auswahl.anzahl() == 0 && komponentenAnzahl(k) == 0) {
        warnung(p, "CSG Wrap Merge: No brushes or components selected.\n");
        return;
    }
    const bool primit = u.primitivModus;
    std::vector<PinselOrt> gewaehlt;
    if (primit) { gewaehlt = gewaehltePinsel(k); }
    EckenMenge ecken;
    for (const PinselOrt& g : gewaehlt) {   // eindeutige Ecken
        g.pinsel->bRep(kt);
        for (const auto& f : g.pinsel->flaechen()) {
            if (!f->traegtBei()) { continue; }
            const Wicklung& w = f->wicklung();
            for (std::size_t i = 0; i < w.numpoints; ++i) { ecken.dazu(w[i].vertex); }
        }
    }
    for (const auto& ent : k.entitaeten()) {   // Scene_gatherSelectedComponents (sichtbare)
        if (ent->versteckt) { continue; }
        for (const auto& prim : ent->kinder()) {
            if (prim->versteckt) { continue; }
            if (prim->art() == Primitiv::Art::Pinsel) {
                komponentenSammeln(k, static_cast<const Pinsel&>(*prim), [&](const Vektor3d& v) { ecken.dazu(v); });
            } else {
                const auto& patch = static_cast<const Patch&>(*prim);
                for (std::size_t i = 0; i < patch.punkte().size(); ++i) {
                    if (patch.punktGewaehlt(i)) { ecken.dazu(Vektor3d(patch.punkte()[i].vertex)); }
                }
            }
        }
    }
    for (std::size_t i = 0; i < punkte.anzahl; ++i) { ecken.dazu(punkte.punkte[i]); }
    if (ecken.ecken.size() < 4) {
        warnung(p, "CSG Wrap Merge: Too few vertices: " + std::to_string(ecken.ecken.size()) + ".\n");
        return;
    }
    std::vector<HuellenEbene> ebenen;   // eindeutige und brauchbare Ebenen
    for (const PinselOrt& g : gewaehlt) {
        for (const auto& f : g.pinsel->flaechen()) {
            if (!f->traegtBei()) { continue; }
            const PinselTeilung t = ecken.ebeneEinordnen(f->ebene());
            if ((t.counts[kVorne] == 0) != (t.counts[kHinten] == 0)) { ebeneDazu(ebenen, HuellenEbene{f->ebene(), f.get(), {}}); }
        }
    }
    huelleBauen(ecken, ebenen);
    if (ebenen.size() < 4) {
        warnung(p, "CSG Wrap Merge: Too few planes: " + std::to_string(ebenen.size()) + ".\n");
        return;
    }
    std::shared_ptr<Pinsel> pinsel;
    std::shared_ptr<Entitaet> pinselEnt;
    if (k.auswahl.anzahl() > 0) {   // der zuletzt gewaehlte Pinsel (ultimateSelected)
        const AuswahlElement& el = k.auswahl.elemente().back();
        pinsel = std::dynamic_pointer_cast<Pinsel>(el.primitiv.lock());
        pinselEnt = el.entitaet.lock();
    }
    const bool alterPinsel = pinsel && primit;
    auto neu = std::make_shared<Pinsel>();
    if (alterPinsel) {
        pinsel->leeren();
    } else {
        pinsel = neu;
    }
    {
        TexturProjektion projektion;
        texdefStandard(kt.art(), projektion);
        for (const HuellenEbene& e : ebenen) {
            if (e.flaeche != nullptr) {
                pinsel->flaecheHinzufuegen(*e.flaeche, kt);
            } else {
                pinsel->ebeneHinzufuegen(e.punkte[0], e.punkte[1], e.punkte[2], u.browserShader, projektion, kt);
            }
        }
        pinsel->leereFlaechenEntfernen(kt);
    }
    if (!pinsel->hatBeitragendeFlaechen(kt)) {
        warnung(p, "CSG Wrap Merge: Failed - result would not be convex.\n");
        return;
    }
    if (alterPinsel) {
        gewaehltePinselLoeschen(k, pinsel.get(), nullptr);
    } else {
        const auto gruppe = letzteGruppe(k);
        if (primit) { gewaehltePinselLoeschen(k, nullptr, gruppe.get()); }   // die Ausgangspinsel weg
        neu->schicht = static_cast<int>(k.aktuelleSchicht);
        gruppe->kindEinfuegen(neu);
        k.auswahl.waehlen(gruppe, neu);
    }
}

void csgHuellenVereinigen(Karte& k, const CsgUmgebung& u, const ClipperPunkte& punkte, Protokoll& p) {
    Befehl befehl(k.undo, "brushWrapMerge");
    csgHuelleBauen(k, u, punkte, p);
}

void csgSchneiden(Karte& k, Protokoll& p) {
    const Kontext& kt = k.kontext;
    // Scene_forEachVisibleSelectedBrush: in Auswahlreihenfolge, der letzte bleibt
    std::vector<PinselOrt> gewaehlt;
    for (const GewaehltesPrimitiv& g : gewaehltePrimitiveListe(k)) {
        if (g.primitiv->art() == Primitiv::Art::Pinsel && !g.primitiv->versteckt && !g.entitaet->versteckt) {
            gewaehlt.push_back({g.entitaet, std::static_pointer_cast<Pinsel>(g.primitiv)});
        }
    }
    if (gewaehlt.empty()) {
        warnung(p, "CSG Intersect: No brushes selected.\n");
        return;
    }
    if (gewaehlt.size() < 2) {
        warnung(p, "CSG Intersect: At least two brushes have to be selected.\n");
        return;
    }
    Befehl befehl(k.undo, "brushIntersect");
    const PinselOrt letzter = gewaehlt.back();
    gewaehlt.pop_back();
    for (const PinselOrt& g : gewaehlt) {
        g.pinsel->bRep(kt);
        for (const auto& f : g.pinsel->flaechen()) {
            if (!f->traegtBei()) { continue; }
            const PinselTeilung t = pinselEinordnen(*letzter.pinsel, kt, f->ebene());
            if (t.counts[kVorne] != 0) {
                letzter.pinsel->flaecheHinzufuegen(*f, kt);
                letzter.pinsel->leereFlaechenEntfernen(kt);
            }
        }
    }
    if (!letzter.pinsel->hatBeitragendeFlaechen(kt)) {
        // so in NRC 20260114 ("Intesect"); master schreibt "Intersect" - die Fehlerbehebung uebernommen
        warnung(p, "CSG Intersect: Failed - result would not be convex.\n");
        gewaehltePinselLoeschen(k);
    } else {
        gewaehltePinselLoeschen(k, letzter.pinsel.get(), nullptr);
    }
}

void csgRaumBauen(Karte& k, const CsgUmgebung& u) {
    HohlEinstellung e;
    e.art = HohlArt::Ziehen;
    e.versatz = u.raster;
    e.nachAchse = false;
    e.gewaehlteAusschliessen = true;
    e.caulk = true;
    e.innenEntfernen = true;
    Befehl befehl(k.undo, "makeRoom");
    aushoehlenAlle(k, u, e);
    gewaehltePinselLoeschen(k);
}

void csgAushoehlen(Karte& k, const CsgUmgebung& u, const HohlEinstellung& e, const std::string& rueckgaengig) {
    Befehl befehl(k.undo, rueckgaengig);
    aushoehlenAlle(k, u, e);
    if (e.innenEntfernen) { gewaehltePinselLoeschen(k); }
}

void csgPinselVersetzen(Karte& k, const HohlEinstellung& e, bool schrumpfen) {
    HohlEinstellung s = e;
    if (schrumpfen) { s.versatz *= -1; }
    Befehl befehl(k.undo, schrumpfen ? "Shrink brush" : "Expand brush");
    Ausschluss a{s};
    for (const auto& ent : k.entitaeten()) {   // Scene_forEachVisibleBrush mit BrushFaceOffset
        if (ent->versteckt) { continue; }
        for (const auto& prim : ent->kinder()) {
            if (prim->versteckt || prim->art() != Primitiv::Art::Pinsel) { continue; }
            const auto pinsel = std::static_pointer_cast<Pinsel>(prim);
            if (!k.auswahl.gewaehlt(ent.get(), pinsel.get()) && !pinselKomponentenGewaehlt(k, *pinsel)) { continue; }
            a.sammeln(k, *pinsel);
            for (const auto& f : pinsel->flaechen()) {
                if (!a.ausgenommen(*f)) { f->versetzen(s.versatz); }
            }
        }
    }
}

Ebene3d clipperEbene(const ClipperPunkte& punkte, bool umgedreht) {
    if (punkte.anzahl < 2) { return Ebene3d(0, 0, 0, 0); }
    return umgedreht ? ebeneAusPunkten(punkte.punkte[0], punkte.punkte[2], punkte.punkte[1])
                     : ebeneAusPunkten(punkte.punkte[0], punkte.punkte[1], punkte.punkte[2]);
}

void clipperWicklung(const Pinsel& pinsel, const Kontext& k, const Ebene3d& ebene, Wicklung& w) {
    if (ebeneGueltig(ebene)) {
        pinsel.bRep(k);
        pinsel.wicklungFuerEbene(w, ebene);
    } else {
        w.resize(0);
    }
}

void pinselTeilen(Karte& k, const CsgUmgebung& u, const ClipperPunkte& punkteEin, bool umgedreht, bool caulk, bool teilen, Protokoll& p) {
    const Kontext& kt = k.kontext;
    const std::string shader = caulk ? u.caulkShader : u.browserShader;
    TexturProjektion projektion;
    texdefStandard(kt.art(), projektion);
    // BrushSplitByPlaneSelected (csg.cpp:736-791)
    ClipperPunkte punkte = punkteEin;
    if (umgedreht) { std::swap(punkte.punkte[1], punkte.punkte[2]); }
    const Ebene3d ebene = ebeneAusPunkten(punkte.punkte[0], punkte.punkte[1], punkte.punkte[2]);
    bool getan = false;   // m_gj
    if (punkteEin.anzahl > 1 && ebeneGueltig(ebeneAusPunkten(punkteEin.punkte[0], punkteEin.punkte[1], punkteEin.punkte[2]))) {
        const auto liste = k.entitaeten();
        for (const auto& ent : liste) {
            const auto kinder = ent->kinder();
            for (const auto& prim : kinder) {
                if (ent->versteckt || prim->versteckt || prim->art() != Primitiv::Art::Pinsel || !k.auswahl.gewaehlt(ent.get(), prim.get())) { continue; }
                const auto pinsel = std::static_pointer_cast<Pinsel>(prim);
                const PinselTeilung t = pinselEinordnen(*pinsel, kt, ebene);
                if (t.counts[kHinten] != 0 && t.counts[kVorne] != 0) {   // die Ebene schneidet den Pinsel
                    getan = true;
                    if (teilen) {
                        auto teil = pinsel->kopie(kt);
                        teil->ebeneHinzufuegen(punkte.punkte[0], punkte.punkte[2], punkte.punkte[1], shader, projektion, kt);
                        teil->leereFlaechenEntfernen(kt);
                        teil->schicht = static_cast<int>(k.aktuelleSchicht);
                        ent->kindEinfuegen(teil);
                        k.auswahl.waehlen(ent, teil);   // selectPath( fragmentPath, true )
                    }
                    pinsel->ebeneHinzufuegen(punkte.punkte[0], punkte.punkte[1], punkte.punkte[2], shader, projektion, kt);
                    pinsel->leereFlaechenEntfernen(kt);
                } else if (!teilen && t.counts[kVorne] != 0) {   // ganz vor der Ebene: weg
                    getan = true;
                    pinselEntfernen(k, ent, pinsel);
                }
            }
        }
    }
    if (!getan) { csgHuelleBauen(k, u, punkteEin, p); }
}

}  // namespace bhed::karte
