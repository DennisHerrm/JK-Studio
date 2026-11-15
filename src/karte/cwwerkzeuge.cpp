// cwwerkzeuge.cpp - Karten-Editor, CW-IMPORT Punkte 3 bis 6: Massstab je Raum, Treppe -> Clip-Rampe, glatte Clip-Wand, Ebene
// Collision (siehe cwwerkzeuge.h). Eigene Werkzeuge, nicht in NetRadiant-Custom; was q3map2 daraus baut, nach q3map2 (NRC 20260114:
// map.cpp SetBrushContents, model.cpp clipflags_doClip) und den Shadern von Jedi Academy (shaders/system.shader).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114) ist nur, was q3map2 aus den Werkzeugen baut: die Clip-Spawnflags eines
// misc_model (model.cpp:357-371 clipflags_doClip, kClipFlags) und die Regel, dass eine nicht feste Seite den ganzen Pinsel
// durchlaessig macht (map.cpp:455-503 SetBrushContents). Die konvexe Huelle rechnet QuickHull (third_party/quickhull, Public
// Domain, wie in NetRadiant-Custom); die Werkzeuge selbst sind eigener Code.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version (LICENSE).
#include "bhed/karte/cwwerkzeuge.h"

#include "bhed/karte/ebenen.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/modell.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/rueckgaengig.h"
#include "bhed/karte/texdef.h"

#include "../../third_party/quickhull/QuickHull.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>

namespace bhed::karte {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::string zahl(double v) {
    char t[32];
    std::snprintf(t, sizeof(t), "%g", v);
    return t;
}

Vektor3d vd(const Vektor3f& v) { return Vektor3d(v.x(), v.y(), v.z()); }

// Nicht gezeichnete Werkzeug-Shader (caulk, clip, nodraw ...): nach dem Namen wie der Optimierer ohne Inventar (shaderArtNachName)
bool unsichtbar(const std::string& shader) {
    const std::string s = klein(shader);
    const std::size_t sl = s.find_last_of('/');
    const std::string n = sl == std::string::npos ? s : s.substr(sl + 1);
    const bool system = s.rfind("textures/system/", 0) == 0 || s.rfind("textures/common/", 0) == 0;
    return system && (n == "caulk" || n.find("clip") != std::string::npos || n.find("nodraw") != std::string::npos || n == "trigger" ||
                      n == "hint" || n == "skip" || n == "origin" || n == "areaportal");
}
bool caulk(const std::string& shader) {
    const std::string s = klein(shader);
    return s == "textures/system/caulk" || s == "textures/common/caulk";
}

// clipflags_doClip (q3map2 model.cpp:357-371): eClipFlags = eClipModel | eExtrudeFaceNormals | eExtrudeTerrain | eExtrudeVertexNormals
// | ePyramidalClip | eExtrudeDownwards | eExtrudeUpwards | eMaxExtrude | eAxialBackplane
constexpr int kClipFlags = (1 << 1) | (1 << 3) | (1 << 4) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11) | (1 << 12);

// Ein Pinsel in die Welt (wie importEinfuegen), in eine Ebene, gewaehlt
void inWelt(Karte& k, const std::shared_ptr<Pinsel>& p, int ebene) {
    std::shared_ptr<Entitaet> welt = k.weltHolenOderAnlegen();
    p->schicht = ebene >= 0 ? ebene : static_cast<int>(k.aktuelleSchicht);
    welt->kindEinfuegen(p);
    k.auswahl.waehlen(welt, p);
}

}  // namespace

// --- Punkt 6 ---------------------------------------------------------------------------------------------------------------------------

int kollisionsEbeneFinden(const Karte& k) {
    for (std::size_t i = 0; i < k.schichten.size(); ++i) {
        if (k.schichten[i].name == kKollisionsEbene) { return static_cast<int>(i); }
    }
    return -1;
}

int kollisionsEbeneHolen(Karte& k) {
    const int da = kollisionsEbeneFinden(k);
    if (da >= 0) { return da; }
    // ebeneNeu macht die neue Ebene zur aktuellen (layer_new) - die bisherige bleibt die aktuelle; ihre Nummer kann sich verschieben
    const std::string aktuell = k.aktuelleSchicht < k.schichten.size() ? k.schichten[k.aktuelleSchicht].name : std::string();
    int neu = -1;
    const EbenenNummern nummern = ebeneNeu(k, -1, kKollisionsEbene, &neu);
    for (std::size_t i = 0; i < k.schichten.size(); ++i) {
        if (k.schichten[i].name == aktuell && static_cast<int>(i) != neu) {
            k.aktuelleSchicht = i;
            break;
        }
    }
    (void)nummern;
    ebeneFarbe(k, neu, {170, 120, 255});   // violett wie die Clip-Shader im Editor
    return neu;
}

std::shared_ptr<Pinsel> pinselAusEbenen(const std::vector<Ebene3d>& ebenen, const std::string& shader, const Kontext& k) {
    auto p = std::make_shared<Pinsel>();
    for (const Ebene3d& e : ebenen) {
        const Vektor3d n = normiert(e.normal());
        const Vektor3d q = n * e.dist();
        // ebeneAusPunkten: Normale = (p2 - p0) x (p1 - p0); mit p1 = q + t, p2 = q + t x n ist das n
        const Vektor3d hilfs = std::fabs(n.z()) < 0.9 ? Vektor3d(0, 0, 1) : Vektor3d(1, 0, 0);
        const Vektor3d t = normiert(kreuz(hilfs, n)) * 64.0;
        TexturProjektion proj;
        texdefStandard(k.art(), proj);
        (void)p->ebeneHinzufuegen(q, q + t, q + kreuz(t, n), shader, proj, k);
    }
    if (p->anzahl() < 4 || p->entartet(k)) { return nullptr; }
    return p;
}

// --- Punkt 3 ---------------------------------------------------------------------------------------------------------------------------

RaumErgebnis raumSkalieren(Karte& k, const RaumOptionen& o) {
    RaumErgebnis r;
    if (!(o.faktor > 0.01 && o.faktor < 100.0)) {
        r.fehler = "scale factor out of range";
        return r;
    }
    if (k.auswahl.anzahl() == 0) {
        r.fehler = "nothing selected";
        return r;
    }
    Befehl undo(k.undo, "cwScaleRoom");
    Matrix4 m = kEinheitsMatrix;
    const auto f = static_cast<float>(o.faktor);
    const Vektor3f pivot(static_cast<float>(o.pivot.x()), static_cast<float>(o.pivot.y()), static_cast<float>(o.pivot.z()));
    skaliereUmPunkt(m, Vektor3f(f, f, f), pivot);
    std::vector<std::pair<std::shared_ptr<Entitaet>, std::shared_ptr<Primitiv>>> gewaehlt;
    std::set<const Entitaet*> punktEntitaeten;
    for (const AuswahlElement& a : k.auswahl.elemente()) {
        auto en = a.entitaet.lock();
        if (!en) { continue; }
        auto pr = a.primitiv.lock();
        gewaehlt.emplace_back(en, pr);
        if (pr) {
            if (pr->art() == Primitiv::Art::Pinsel) {
                static_cast<Pinsel&>(*pr).transformieren(m, k.kontext);
            } else {
                static_cast<Patch&>(*pr).transformieren(m);
            }
            ++r.primitive;
        } else if (en->istBehaelter()) {   // ganze Brush-Entitaet: ihre Kinder
            for (const auto& kind : en->kinder()) {
                if (kind->art() == Primitiv::Art::Pinsel) {
                    static_cast<Pinsel&>(*kind).transformieren(m, k.kontext);
                } else {
                    static_cast<Patch&>(*kind).transformieren(m);
                }
                ++r.primitive;
            }
        } else if (punktEntitaeten.insert(en.get()).second && en->hat("origin")) {
            // Punkt-Entitaet: nur der Ursprung wandert mit (Modelle und Lichter behalten ihre Groesse)
            double x = 0, y = 0, z = 0;
            if (std::sscanf(en->wert("origin").c_str(), "%lf %lf %lf", &x, &y, &z) == 3) {
                const Vektor3d neu = o.pivot + (Vektor3d(x, y, z) - o.pivot) * o.faktor;
                en->setzen("origin", zahl(neu.x()) + " " + zahl(neu.y()) + " " + zahl(neu.z()));
                ++r.entitaeten;
            }
        }
    }
    if (o.marker) {
        auto ent = k.neueEntitaet("info_notnull", false);
        ent->setzen("origin", zahl(o.pivot.x()) + " " + zahl(o.pivot.y()) + " " + zahl(o.pivot.z()));
        ent->setzen("cw_scale", zahl(o.faktor));
        if (!o.name.empty()) { ent->setzen("cw_room", o.name); }
        k.entitaetEinfuegen(ent);
        k.auswahl.waehlen(ent);
        r.marker = ent;
    }
    return r;
}

std::string raumKommentar(const Entitaet& e) {
    if (!e.hat("cw_scale")) { return std::string(); }
    std::string t = "cw_scale " + e.wert("cw_scale");
    if (e.hat("cw_room")) { t += " cw_room \"" + e.wert("cw_room") + "\""; }
    if (e.hat("origin")) { t += " pivot " + e.wert("origin"); }
    return t + " (JK Studio: room scale, data only)";
}

// --- Punkt 4 ---------------------------------------------------------------------------------------------------------------------------

TreppenRampe rampeRechnen(const std::vector<TreppenFlaeche>& flaechen, double unten, double gehen) {
    TreppenRampe r;
    // Tritte: die Flaechen nach Hoehe (Mittel der Ecken), Flaechen innerhalb 1 Einheit gehoeren zusammen
    struct Tritt {
        double z = 0;
        std::vector<Vektor3d> ecken;
        Vektor3d mitte{0, 0, 0};
    };
    std::vector<Tritt> tritte;
    for (const TreppenFlaeche& f : flaechen) {
        if (f.ecken.size() < 3) { continue; }
        double z = 0;
        for (const Vektor3d& p : f.ecken) { z += p.z(); }
        z /= static_cast<double>(f.ecken.size());
        auto it = std::find_if(tritte.begin(), tritte.end(), [z](const Tritt& t) { return std::fabs(t.z - z) <= 1.0; });
        if (it == tritte.end()) {
            tritte.push_back(Tritt{z, {}, {}});
            it = tritte.end() - 1;
        }
        it->ecken.insert(it->ecken.end(), f.ecken.begin(), f.ecken.end());
    }
    std::sort(tritte.begin(), tritte.end(), [](const Tritt& a, const Tritt& b) { return a.z < b.z; });
    // der unterste "Tritt" auf der Unterkante ist der Boden, keine Stufe
    while (!tritte.empty() && tritte.front().z <= unten + 1.0) { tritte.erase(tritte.begin()); }
    r.tritte = tritte.size();
    if (tritte.size() < 2) {
        r.fehler = "no stairs: fewer than two walkable step faces above the bottom";
        return r;
    }
    for (Tritt& t : tritte) {
        Vektor3d s{0, 0, 0};
        for (const Vektor3d& p : t.ecken) { s = s + p; }
        t.mitte = s * (1.0 / static_cast<double>(t.ecken.size()));
    }
    // die Richtung hinauf: vom untersten zum obersten Tritt, waagerecht
    Vektor3d u = tritte.back().mitte - tritte.front().mitte;
    u = Vektor3d(u.x(), u.y(), 0);
    if (laenge(u) < 1.0) {
        r.fehler = "no stairs: the steps lie above each other";
        return r;
    }
    u = normiert(u);
    const Vektor3d v = kreuz(Vektor3d(0, 0, 1), u);
    // je Tritt die Vorderkante (kleinstes u) und Hinterkante; quer die Breite aller Tritte
    std::vector<double> vorn;
    double vMin = 1e30, vMax = -1e30;
    for (const Tritt& t : tritte) {
        double a = 1e30;
        for (const Vektor3d& p : t.ecken) {
            a = std::min(a, punkt(p, u));
            vMin = std::min(vMin, punkt(p, v));
            vMax = std::max(vMax, punkt(p, v));
        }
        vorn.push_back(a);
    }
    double hinten = -1e30;
    for (const Vektor3d& p : tritte.back().ecken) { hinten = std::max(hinten, punkt(p, u)); }
    const std::size_t n = tritte.size() - 1;
    const double zN = tritte.back().z;
    const double uN = vorn[n];
    // die Rampe durch die Vorderkante des obersten Tritts, so flach, dass keine Vorderkante darueber liegt
    double s = 1e30;
    for (std::size_t i = 0; i < n; ++i) {
        const double du = uN - vorn[i];
        if (du <= 0.5) {
            r.fehler = "no stairs: a lower step reaches past the top step";
            return r;
        }
        s = std::min(s, (zN - tritte[i].z) / du);
    }
    if (!(s > 0.0) || hinten <= uN + 0.5 || vMax - vMin < 1.0) {
        r.fehler = "no stairs: the top step has no depth or the stairs no width";
        return r;
    }
    const double uStart = uN - (zN - unten) / s;
    r.unten = unten;
    r.oben = zN;
    r.steigung = s;
    r.richtung = u;
    // Ebenen (Normale nach aussen): Boden, Rampe, oben, hinten, zwei Seiten
    const auto ebene = [](const Vektor3d& nn, const Vektor3d& p) {
        const Vektor3d e = normiert(nn);
        return Ebene3d(e, punkt(e, p));
    };
    const Vektor3d z(0, 0, 1);
    const Vektor3d a = u * uStart + z * unten;      // Fuss der Rampe (auf v = 0)
    r.ebenen.push_back(ebene(z * -1.0, a));
    r.ebenen.push_back(ebene(z - u * s, a));        // Normale (-s, 1) in (u, z)
    r.ebenen.push_back(ebene(z, u * uN + z * zN));
    r.ebenen.push_back(ebene(u, u * hinten + z * zN));
    r.ebenen.push_back(ebene(v * -1.0, v * vMin));
    r.ebenen.push_back(ebene(v, v * vMax));
    r.ok = true;
    (void)gehen;
    return r;
}

TreppeErgebnis treppeZuRampe(Karte& k, const TreppeOptionen& o) {
    TreppeErgebnis r;
    struct StufenPinsel {
        std::shared_ptr<Entitaet> en;
        std::shared_ptr<Pinsel> p;
    };
    std::vector<StufenPinsel> stufen;
    std::vector<StufenPinsel> kollision;     // reine Kollisionspinsel (alle Seiten unsichtbar)
    std::vector<std::shared_ptr<Entitaet>> modelle;
    std::vector<TreppenFlaeche> flaechen;
    double unten = 1e30;
    for (const AuswahlElement& a : k.auswahl.elemente()) {
        auto en = a.entitaet.lock();
        if (!en) { continue; }
        auto pr = a.primitiv.lock();
        if (pr && pr->art() == Primitiv::Art::Pinsel) {
            auto p = std::static_pointer_cast<Pinsel>(pr);
            p->bRep(k.kontext);   // die Wicklungen entstehen erst bei Bedarf (evaluateBRep)
            bool allesUnsichtbar = true;
            for (const auto& f : p->flaechen()) {
                allesUnsichtbar = allesUnsichtbar && unsichtbar(f->shader());
                const Wicklung& w = f->wicklung();
                for (std::size_t i = 0; i < w.numpoints; ++i) { unten = std::min(unten, w.points[i].vertex.z()); }
            }
            if (allesUnsichtbar) {
                kollision.push_back({en, p});
                continue;
            }
            stufen.push_back({en, p});
            for (const auto& f : p->flaechen()) {
                if (f->ebene().normal().z() < o.gehen || unsichtbar(f->shader())) { continue; }
                TreppenFlaeche tf;
                const Wicklung& w = f->wicklung();
                for (std::size_t i = 0; i < w.numpoints; ++i) { tf.ecken.push_back(w.points[i].vertex); }
                flaechen.push_back(std::move(tf));
            }
        } else if (!pr && !en->istBehaelter() && klein(en->klassenname()).rfind("misc_model", 0) == 0 && k.kontext.modelle != nullptr) {
            const ModellAnsicht an = k.kontext.modelle->ansicht(*en);
            if (!an.modell || an.modell->null) { continue; }
            modelle.push_back(en);
            const Matrix4 lage = miscModellMatrix(*en);
            for (std::size_t fi = 0; fi < an.modell->flaechen.size(); ++fi) {
                const ModellFlaeche& f = an.modell->flaechen[fi];
                if (fi < an.shader.size() && an.shader[fi].empty()) { continue; }   // die Skin bildet die Flaeche nicht ab
                for (std::size_t i = 0; i + 2 < f.indizes.size(); i += 3) {
                    Vektor3d p[3];
                    for (int j = 0; j < 3; ++j) {
                        p[j] = vd(transformierterPunkt(lage, f.punkte[f.indizes[i + static_cast<std::size_t>(j)]].ort));
                        unten = std::min(unten, p[j].z());
                    }
                    // Q3-Dreiecke im Uhrzeigersinn (von aussen): Normale = (p2 - p0) x (p1 - p0)
                    const Vektor3d nn = normiert(kreuz(p[2] - p[0], p[1] - p[0]));
                    if (nn.z() >= o.gehen) { flaechen.push_back(TreppenFlaeche{{p[0], p[1], p[2]}}); }
                }
            }
        }
    }
    if (stufen.empty() && modelle.empty()) {
        r.fehler = "select the stairs: brushes or a misc_model";
        return r;
    }
    const TreppenRampe rr = rampeRechnen(flaechen, unten, o.gehen);
    r.tritte = rr.tritte;
    if (!rr.ok) {
        r.fehler = rr.fehler;
        return r;
    }
    r.unten = rr.unten;
    r.oben = rr.oben;
    r.steigung = rr.steigung;
    r.richtung = rr.richtung;
    r.begehbar = 1.0 / std::sqrt(1.0 + rr.steigung * rr.steigung) >= o.gehen;
    auto rampe = pinselAusEbenen(rr.ebenen, o.clipShader, k.kontext);
    if (!rampe) {
        r.fehler = "the ramp brush is empty";
        return r;
    }
    Befehl undo(k.undo, "cwStairsToRamp");
    k.auswahl.leeren();
    // die Stufen: eine verdeckte Seite nicht fest (caulk zuerst, sonst die nach unten zeigende)
    for (const StufenPinsel& sp : stufen) {
        Flaeche* wahl = nullptr;
        for (const auto& f : sp.p->flaechen()) {
            if (caulk(f->shader())) {
                wahl = f.get();
                break;
            }
        }
        if (wahl == nullptr) {
            double tiefste = 1.0;
            for (const auto& f : sp.p->flaechen()) {
                if (f->ebene().normal().z() < tiefste) {
                    tiefste = f->ebene().normal().z();
                    wahl = f.get();
                }
            }
        }
        if (wahl == nullptr) { continue; }
        if (!unsichtbar(wahl->shader())) { ++r.sichtbareSeiten; }
        wahl->setzeShader(o.nichtFest, k.kontext);
        ++r.pinselNichtFest;
    }
    // misc_model: ohne Clip-Spawnflags (clipflags_doClip)
    for (const auto& en : modelle) {
        const int sf = std::atoi(en->wert("spawnflags").c_str());
        if ((sf & kClipFlags) == 0) { continue; }
        const int neu = sf & ~kClipFlags;
        en->setzen("spawnflags", neu == 0 ? std::string() : std::to_string(neu));
        ++r.modelleOhneClip;
    }
    // reine Kollisionspinsel der Auswahl (nodraw-/clip-/caulk-Kollision der Treppe) entfernen
    for (const StufenPinsel& kp : kollision) {
        if (kp.en->kindEntfernen(kp.p.get())) { ++r.kollisionEntfernt; }
    }
    inWelt(k, rampe, kollisionsEbeneHolen(k));
    r.rampe = rampe;
    r.ok = true;
    return r;
}

// --- Punkt 5 ---------------------------------------------------------------------------------------------------------------------------

std::vector<Ebene3d> wandEbenen(const std::vector<Vektor3d>& punkte, WandArt art, const Vektor3d& blickVon, Vektor3d* normale,
                                const Vektor3d* flaechenNormale) {
    std::vector<Ebene3d> ebenen;
    if (punkte.size() < 4) { return ebenen; }
    Vektor3d mitte{0, 0, 0};
    for (const Vektor3d& p : punkte) { mitte = mitte + p; }
    mitte = mitte * (1.0 / static_cast<double>(punkte.size()));
    if (art == WandArt::Huelle) {
        quickhull::QuickHull<double> huelle;
        std::vector<quickhull::Vector3<double>> wolke;
        for (const Vektor3d& p : punkte) { wolke.emplace_back(p.x(), p.y(), p.z()); }
        auto erg = huelle.getConvexHull(wolke, true, true);
        const auto& ix = erg.getIndexBuffer();
        const auto& vb = erg.getVertexBuffer();
        for (std::size_t i = 0; i + 2 < ix.size(); i += 3) {
            const auto P = [&](std::size_t j) { return Vektor3d(vb[ix[j]].x, vb[ix[j]].y, vb[ix[j]].z); };
            Ebene3d e = ebeneAusPunkten(P(i), P(i + 1), P(i + 2));
            if (!ebeneGueltig(e)) { continue; }
            if (punkt(e.normal(), mitte) > e.dist()) { e = umgedreht(e); }   // nach aussen
            if (std::none_of(ebenen.begin(), ebenen.end(), [&](const Ebene3d& x) { return ebenenGleich(x, e); })) { ebenen.push_back(e); }
        }
        return ebenen;
    }
    // Ebene: die Richtung der Wand waagerecht (Hauptachse der Punkte in x/y), die Normale zur Spielfigur hin
    double sxx = 0, sxy = 0, syy = 0;
    double zMin = 1e30, zMax = -1e30;
    for (const Vektor3d& p : punkte) {
        const double dx = p.x() - mitte.x(), dy = p.y() - mitte.y();
        sxx += dx * dx;
        sxy += dx * dy;
        syy += dy * dy;
        zMin = std::min(zMin, p.z());
        zMax = std::max(zMax, p.z());
    }
    const double winkel = 0.5 * std::atan2(2.0 * sxy, sxx - syy);
    Vektor3d w(std::cos(winkel), std::sin(winkel), 0);
    Vektor3d n(-w.y(), w.x(), 0);
    if (flaechenNormale != nullptr) {   // die Seiten zur Figur hin bestimmen die Richtung (Vorspruenge verdrehen sie nicht)
        const Vektor3d f(flaechenNormale->x(), flaechenNormale->y(), 0);
        if (laenge(f) > 1e-6) {
            n = normiert(f);
            w = Vektor3d(n.y(), -n.x(), 0);
        }
    }
    const Vektor3d zurFigur(blickVon.x() - mitte.x(), blickVon.y() - mitte.y(), 0);
    if (punkt(n, zurFigur) < 0) { n = n * -1.0; }
    double wMin = 1e30, wMax = -1e30, nMin = 1e30, nMax = -1e30;
    for (const Vektor3d& p : punkte) {
        wMin = std::min(wMin, punkt(p, w));
        wMax = std::max(wMax, punkt(p, w));
        nMin = std::min(nMin, punkt(p, n));
        nMax = std::max(nMax, punkt(p, n));
    }
    if (nMax - nMin < 8.0) { nMin = nMax - 8.0; }   // eine flache Wand: 8 dick nach hinten (in die Wand)
    if (zMax - zMin < 1.0 || wMax - wMin < 1.0) { return ebenen; }
    const Vektor3d z(0, 0, 1);
    ebenen.push_back(Ebene3d(n, nMax));             // vorn: am weitesten vorstehenden Punkt
    ebenen.push_back(Ebene3d(n * -1.0, -nMin));
    ebenen.push_back(Ebene3d(w, wMax));
    ebenen.push_back(Ebene3d(w * -1.0, -wMin));
    ebenen.push_back(Ebene3d(z, zMax));
    ebenen.push_back(Ebene3d(z * -1.0, -zMin));
    if (normale != nullptr) { *normale = n; }
    return ebenen;
}

WandErgebnis glatteClipWand(Karte& k, WandArt art, const Vektor3d& blickVon, const std::string& clipShader) {
    WandErgebnis r;
    std::vector<Vektor3d> punkte;
    std::vector<std::array<Vektor3d, 3>> dreiecke;   // fuer die Richtung: Seiten zur Figur hin
    for (const AuswahlElement& a : k.auswahl.elemente()) {
        auto en = a.entitaet.lock();
        if (!en) { continue; }
        auto pr = a.primitiv.lock();
        if (pr && pr->art() == Primitiv::Art::Pinsel) {
            static_cast<const Pinsel&>(*pr).bRep(k.kontext);   // die Wicklungen entstehen erst bei Bedarf (evaluateBRep)
            for (const auto& f : static_cast<const Pinsel&>(*pr).flaechen()) {
                const Wicklung& w = f->wicklung();
                for (std::size_t i = 0; i < w.numpoints; ++i) { punkte.push_back(w.points[i].vertex); }
                for (std::size_t i = 1; i + 1 < w.numpoints; ++i) {   // Faecher; die Normale der Seite zeigt nach aussen
                    const Vektor3d n = f->ebene().normal();
                    const Vektor3d a0 = w.points[0].vertex, a1 = w.points[i].vertex, a2 = w.points[i + 1].vertex;
                    dreiecke.push_back(punkt(kreuz(a1 - a0, a2 - a0), n) >= 0 ? std::array<Vektor3d, 3>{a0, a1, a2} : std::array<Vektor3d, 3>{a0, a2, a1});
                }
            }
        } else if (pr) {
            for (const PatchPunkt& p : static_cast<const Patch&>(*pr).punkte()) { punkte.push_back(vd(p.vertex)); }
        } else if (!en->istBehaelter() && k.kontext.modelle != nullptr) {
            const ModellAnsicht an = k.kontext.modelle->ansicht(*en);
            if (!an.modell || an.modell->null) { continue; }
            const Matrix4 lage = miscModellMatrix(*en);
            for (const ModellFlaeche& f : an.modell->flaechen) {
                for (const ModellPunkt& q : f.punkte) { punkte.push_back(vd(transformierterPunkt(lage, q.ort))); }
                for (std::size_t i = 0; i + 2 < f.indizes.size(); i += 3) {   // Q3-Umlauf (im Uhrzeigersinn): Normale = (p2 - p0) x (p1 - p0)
                    const Vektor3d a0 = vd(transformierterPunkt(lage, f.punkte[f.indizes[i]].ort));
                    const Vektor3d a1 = vd(transformierterPunkt(lage, f.punkte[f.indizes[i + 1]].ort));
                    const Vektor3d a2 = vd(transformierterPunkt(lage, f.punkte[f.indizes[i + 2]].ort));
                    dreiecke.push_back({a0, a2, a1});
                }
            }
        }
    }
    r.punkte = punkte.size();
    // die Richtung: Summe der Normalen mal Flaeche der Seiten, die zur Figur (Kamera) zeigen
    Vektor3d mitte{0, 0, 0};
    for (const Vektor3d& p : punkte) { mitte = mitte + p; }
    if (!punkte.empty()) { mitte = mitte * (1.0 / static_cast<double>(punkte.size())); }
    const Vektor3d zurFigur(blickVon.x() - mitte.x(), blickVon.y() - mitte.y(), 0);
    Vektor3d summe{0, 0, 0};
    for (const auto& d : dreiecke) {
        const Vektor3d n2 = kreuz(d[1] - d[0], d[2] - d[0]);   // Laenge = doppelte Flaeche
        if (punkt(Vektor3d(n2.x(), n2.y(), 0), zurFigur) > 0) { summe = summe + Vektor3d(n2.x(), n2.y(), 0); }
    }
    const std::vector<Ebene3d> ebenen = wandEbenen(punkte, art, blickVon, &r.normale, dreiecke.empty() ? nullptr : &summe);
    if (ebenen.size() < 4) {
        r.fehler = "select the wall: brushes, patches or a misc_model with some size";
        return r;
    }
    auto p = pinselAusEbenen(ebenen, clipShader, k.kontext);
    if (!p) {
        r.fehler = "the clip brush is empty";
        return r;
    }
    Befehl undo(k.undo, "cwClipWall");
    k.auswahl.leeren();
    inWelt(k, p, kollisionsEbeneHolen(k));
    r.pinsel = p;
    r.ok = true;
    return r;
}

}  // namespace bhed::karte
