// spiegel.cpp - Karten-Editor, Stufe 8b Punkt 6: der Spiegel-Modus (siehe
// spiegel.h; nicht in NetRadiant-Custom). Portiert aus NetRadiant-Custom ist
// das Spiegeln wie "Flip" (scaleSelected mit -1 -> Brush::transform mit
// Texture Lock, Patch::transform mit PatchControlArray_invert); Paare und
// Abgleich sind eigener Code.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version (LICENSE).
#include "bhed/karte/spiegel.h"

#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <unordered_set>

namespace bhed::karte {

namespace {

template <typename T>
bool nieBelegt(const std::weak_ptr<T>& w) {
    return !w.owner_before(std::weak_ptr<T>{}) && !std::weak_ptr<T>{}.owner_before(w);
}

std::string objektText(const Entitaet& e, const Primitiv* p, const Kontext& k) {
    return p != nullptr ? primitivText(*p, k) : entitaetText(e, k);
}

// Ecken eines Pinsels bzw. Kontrollpunkte eines Patches, auf 1/64 gerundet und
// sortiert: gleiche Form ohne Ruecksicht auf Reihenfolge und Texturen.
using Punkte = std::vector<std::array<long long, 3>>;
void punkteSammeln(const Primitiv& p, const Kontext& k, Punkte& aus) {
    const auto dazu = [&](double x, double y, double z) {
        aus.push_back({std::llround(x * 64.0), std::llround(y * 64.0), std::llround(z * 64.0)});
    };
    if (p.art() == Primitiv::Art::Pinsel) {
        const auto& b = static_cast<const Pinsel&>(p);
        for (const FlaechenPunkt& fp : b.ecken(k)) {
            const Vektor3d v = b.eckpunkt(fp);
            dazu(v[0], v[1], v[2]);
        }
    } else {
        for (const PatchPunkt& c : static_cast<const Patch&>(p).punkte()) { dazu(c.vertex[0], c.vertex[1], c.vertex[2]); }
    }
}
Punkte formVon(const Primitiv& p, const Kontext& k) {
    Punkte r;
    punkteSammeln(p, k, r);
    std::sort(r.begin(), r.end());
    r.erase(std::unique(r.begin(), r.end()), r.end());
    return r;
}
bool nah(const std::array<long long, 3>& a, const std::array<long long, 3>& b) {
    for (std::size_t i = 0; i < 3; ++i) {
        if (std::llabs(a[i] - b[i]) > 2) { return false; }   // 1/32 Einheit
    }
    return true;
}
bool formGleich(const Punkte& a, const Punkte& b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (!nah(a[i], b[i])) { return false; }
    }
    return true;
}

double winkelNorm(double w) {
    w = std::fmod(w, 360.0);
    if (w < 0) { w += 360.0; }
    if (w == 0) { w = 0; }   // -0 -> 0
    return w;
}
std::string zahl(double d) { return zahlText(static_cast<float>(d)); }

// Euler (pitch yaw roll) eines um die Achse gespiegelten Objekts: die
// Vorwaertsrichtung wird gespiegelt, "oben" bleibt oben (bei Z: unten).
//   X: (p, 180-y, -r)   Y: (p, -y, -r)   Z: (-p, y, 180-r)
std::array<double, 3> eulerGespiegelt(const std::array<double, 3>& w, int achse) {
    switch (achse) {
    case 0: return {w[0], 180.0 - w[1], -w[2]};
    case 1: return {w[0], -w[1], -w[2]};
    default: return {-w[0], w[1], 180.0 - w[2]};
    }
}

bool ursprungLesen(const std::string& s, double v[3]) { return std::sscanf(s.c_str(), "%lf %lf %lf", &v[0], &v[1], &v[2]) == 3; }

// Gespiegelte Entitaet (nicht eingefuegt, nicht angemeldet).
std::shared_ptr<Entitaet> entitaetGespiegelt(Karte& k, const Entitaet& e, int achse, double ebene) {
    auto n = k.neueEntitaet(e.klassenname(), e.istBehaelter());
    for (const auto& [s, w] : schluesselGespiegelt(e.schluessel(), achse, ebene)) { n->setzen(s, w); }
    n->schicht = e.schicht;
    for (const auto& c : e.kinder()) { n->kindEinfuegen(primitivGespiegelt(*c, achse, ebene, k.kontext)); }
    return n;
}

bool lebt(const std::shared_ptr<Entitaet>& e, const std::shared_ptr<Primitiv>& p, bool mitPrimitiv) {
    if (!e || e->undoSystem() == nullptr) { return false; }
    if (!mitPrimitiv) { return true; }
    return p && p->undoSystem() != nullptr;
}

// Pinsel/Patch "ziel" auf das Spiegelbild von "quelle" setzen (mit Undo).
void primitivSetzen(Primitiv& ziel, const Primitiv& quelle, int achse, double ebene, const Kontext& k) {
    const auto m = primitivGespiegelt(quelle, achse, ebene, k);
    if (ziel.art() != quelle.art()) { return; }
    if (ziel.art() == Primitiv::Art::Pinsel) {
        auto& z = static_cast<Pinsel&>(ziel);
        const auto& g = static_cast<const Pinsel&>(*m);
        if (z.anzahl() == g.anzahl()) {
            for (std::size_t i = 0; i < z.anzahl(); ++i) { z[i].zustandLaden(*g[i].zustandSichern()); }
            z.geaendert();
        } else {
            z.leeren();
            for (const auto& f : g.flaechen()) { z.flaecheHinzufuegen(*f, k); }
        }
    } else {
        ziel.zustandLaden(*m->zustandSichern());
    }
}

void entitaetSetzen(Karte& k, Entitaet& ziel, const Entitaet& quelle, int achse, double ebene) {
    const auto soll = schluesselGespiegelt(quelle.schluessel(), achse, ebene);
    std::vector<std::string> weg;
    for (const auto& [s, w] : ziel.schluessel()) {
        (void)w;
        if (std::none_of(soll.begin(), soll.end(), [&](const auto& x) { return x.first == s; })) { weg.push_back(s); }
    }
    for (const std::string& s : weg) { ziel.setzen(s, ""); }
    for (const auto& [s, w] : soll) {
        if (s == "classname") { continue; }
        if (!ziel.hat(s) || ziel.wert(s) != w) { ziel.setzen(s, w); }
    }
    const auto& qk = quelle.kinder();
    const auto& zk = ziel.kinder();
    bool gleich = qk.size() == zk.size();
    for (std::size_t i = 0; gleich && i < qk.size(); ++i) { gleich = qk[i]->art() == zk[i]->art(); }
    if (gleich) {
        for (std::size_t i = 0; i < qk.size(); ++i) { primitivSetzen(*zk[i], *qk[i], achse, ebene, k.kontext); }
    } else {
        const Entitaet::Kinder alt = zk;
        for (const auto& c : alt) { ziel.kindEntfernen(c.get()); }
        for (const auto& c : qk) {
            auto m = primitivGespiegelt(*c, achse, ebene, k.kontext);
            ziel.kindEinfuegen(std::move(m));
        }
    }
}

struct Einheit {
    std::shared_ptr<Entitaet> e;
    std::shared_ptr<Primitiv> p;
    [[nodiscard]] const void* schluessel() const { return p ? static_cast<const void*>(p.get()) : static_cast<const void*>(e.get()); }
};

std::vector<Einheit> gewaehlteEinheiten(const Karte& k, std::size_t hoechstens) {
    std::vector<Einheit> r;
    std::unordered_set<const void*> schon;
    for (const AuswahlElement& el : k.auswahl.elemente()) {
        auto e = el.entitaet.lock();
        if (!e) { continue; }
        Einheit u;
        u.e = e;
        if (!nieBelegt(el.primitiv)) {
            u.p = el.primitiv.lock();
            if (!u.p) { continue; }
        } else if (e->klassenname() == "worldspawn") {
            continue;
        }
        if (!schon.insert(u.schluessel()).second) { continue; }
        r.push_back(std::move(u));
        if (r.size() > hoechstens) { return {}; }
    }
    return r;
}

}  // namespace

Matrix4 spiegelMatrix(int achse, double ebene) {
    Vektor3f s(1, 1, 1);
    s[static_cast<std::size_t>(achse)] = -1;
    Matrix4 m = skalierungsMatrix(s);
    const float t = static_cast<float>(2.0 * ebene);
    if (achse == 0) {
        m.tx() = t;
    } else if (achse == 1) {
        m.ty() = t;
    } else {
        m.tz() = t;
    }
    return m;
}

std::shared_ptr<Primitiv> primitivGespiegelt(const Primitiv& p, int achse, double ebene, const Kontext& k) {
    const Matrix4 m = spiegelMatrix(achse, ebene);
    if (p.art() == Primitiv::Art::Pinsel) {
        auto c = static_cast<const Pinsel&>(p).kopie(k);
        c->transformieren(m, k);
        return c;
    }
    auto c = static_cast<const Patch&>(p).kopie();
    c->schicht = p.schicht;
    c->transformieren(m);
    return c;
}

std::vector<std::pair<std::string, std::string>> schluesselGespiegelt(const std::vector<std::pair<std::string, std::string>>& s, int achse,
                                                                      double ebene) {
    std::vector<std::pair<std::string, std::string>> r = s;
    for (auto& [k, w] : r) {
        if (k == "origin") {
            double v[3];
            if (ursprungLesen(w, v)) {
                v[achse] = (2.0 * ebene) - v[achse];
                w = zahl(v[0]) + ' ' + zahl(v[1]) + ' ' + zahl(v[2]);
            }
        } else if (k == "angle") {
            double a = 0;
            if (std::sscanf(w.c_str(), "%lf", &a) == 1) {
                if (a == -1 || a == -2) {   // hoch / runter (G_SetMovedir)
                    if (achse == 2) { w = a == -1 ? "-2" : "-1"; }
                } else if (achse != 2) {
                    w = zahl(winkelNorm(achse == 0 ? 180.0 - a : -a));
                }
            }
        } else if (k == "angles") {
            std::array<double, 3> e{};
            if (std::sscanf(w.c_str(), "%lf %lf %lf", &e[0], &e[1], &e[2]) == 3) {
                const auto g = eulerGespiegelt(e, achse);
                w = zahl(winkelNorm(g[0])) + ' ' + zahl(winkelNorm(g[1])) + ' ' + zahl(winkelNorm(g[2]));
            }
        }
    }
    return r;
}

void SpiegelModus::vergessen() {
    m_paare.clear();
    m_einzeln.clear();
    m_aenderungen = static_cast<std::size_t>(-1);
}

std::shared_ptr<Primitiv> SpiegelModus::gegenstueck(const Primitiv* p) const {
    for (const Paar& q : m_paare) {
        auto a = q.a.p.lock();
        auto b = q.b.p.lock();
        if (a && a.get() == p) { return b; }
        if (b && b.get() == p) { return a; }
    }
    return nullptr;
}

std::shared_ptr<Entitaet> SpiegelModus::gegenstueck(const Entitaet* e) const {
    for (const Paar& q : m_paare) {
        if (!nieBelegt(q.a.p)) { continue; }
        auto a = q.a.e.lock();
        auto b = q.b.e.lock();
        if (a && a.get() == e) { return b; }
        if (b && b.get() == e) { return a; }
    }
    return nullptr;
}

std::size_t SpiegelModus::selbstSymmetrisch() const {
    return static_cast<std::size_t>(std::count_if(m_einzeln.begin(), m_einzeln.end(), [](const auto& x) { return x.second.selbst; }));
}

void SpiegelModus::paarBilden(Karte& k, const Seite& a, const Seite& b) {
    Paar q{a, b};
    auto ae = a.e.lock();
    auto be = b.e.lock();
    q.a.text = objektText(*ae, a.p.lock().get(), k.kontext);
    q.b.text = objektText(*be, b.p.lock().get(), k.kontext);
    m_paare.push_back(std::move(q));
}

void SpiegelModus::abgleichen(Karte& k, bool befehlOffen) {
    if (!an) {
        if (!m_paare.empty() || !m_einzeln.empty()) { vergessen(); }
        return;
    }
    if (m_laeuft) { return; }
    m_laeuft = true;
    if (achse != m_achseAlt || ebene != m_ebeneAlt) {
        vergessen();
        m_achseAlt = achse;
        m_ebeneAlt = ebene;
    }
    const Kontext& kx = k.kontext;
    const bool geaendert = k.undo.aenderungen() != m_aenderungen;
    bool entfernt = false;

    // 1. Die Paare: eine Seite weg -> die andere auch (im Befehl); eine Seite
    // geaendert -> die andere neu (im Befehl); sonst den Stand merken.
    for (std::size_t i = m_paare.size(); i-- > 0;) {
        Paar& q = m_paare[i];
        const bool mitP = !nieBelegt(q.a.p);
        auto ae = q.a.e.lock();
        auto ap = q.a.p.lock();
        auto be = q.b.e.lock();
        auto bp = q.b.p.lock();
        const bool daA = lebt(ae, ap, mitP);
        const bool daB = lebt(be, bp, mitP);
        if (!daA || !daB) {
            if (befehlOffen && (daA || daB)) {
                const auto& e = daA ? ae : be;
                const auto& p = daA ? ap : bp;
                if (mitP) {
                    e->kindEntfernen(p.get());
                } else {
                    k.entitaetEntfernen(e.get());
                }
                entfernt = true;
            }
            m_paare.erase(m_paare.begin() + static_cast<std::ptrdiff_t>(i));
            continue;
        }
        if (!geaendert) { continue; }
        std::string tA = objektText(*ae, ap.get(), kx);
        std::string tB = objektText(*be, bp.get(), kx);
        const bool aNeu = tA != q.a.text;
        const bool bNeu = tB != q.b.text;
        if (befehlOffen && (aNeu || bNeu)) {
            bool vonA = aNeu && !bNeu;
            if (aNeu && bNeu) {   // beide: die gewaehlte Seite gilt (A bei Gleichstand)
                vonA = k.auswahl.gewaehlt(ae.get(), ap.get()) || !k.auswahl.gewaehlt(be.get(), bp.get());
            }
            if (mitP) {
                if (vonA) {
                    primitivSetzen(*bp, *ap, achse, ebene, kx);
                } else {
                    primitivSetzen(*ap, *bp, achse, ebene, kx);
                }
            } else if (vonA) {
                entitaetSetzen(k, *be, *ae, achse, ebene);
            } else {
                entitaetSetzen(k, *ae, *be, achse, ebene);
            }
            ++m_uebertragungen;
            (vonA ? tB : tA) = objektText(vonA ? *be : *ae, vonA ? bp.get() : ap.get(), kx);
        }
        q.a.text = std::move(tA);
        q.b.text = std::move(tB);
    }

    // 2. Die gewaehlten Objekte ohne Paar.
    std::unordered_set<const void*> gepaart;
    std::unordered_map<const Entitaet*, std::shared_ptr<Entitaet>> partnerEntitaet;
    for (const Paar& q : m_paare) {
        if (!nieBelegt(q.a.p)) {
            gepaart.insert(q.a.p.lock().get());
            gepaart.insert(q.b.p.lock().get());
        } else {
            auto ae = q.a.e.lock();
            auto be = q.b.e.lock();
            gepaart.insert(ae.get());
            gepaart.insert(be.get());
            partnerEntitaet[ae.get()] = be;
            partnerEntitaet[be.get()] = ae;
        }
    }
    for (auto& [s, ein] : m_einzeln) {
        (void)s;
        ein.gesehen = false;
    }
    const auto zwillingSuchen = [&](const Einheit& u, bool& selbst) -> Seite {
        selbst = false;
        if (u.p) {
            const Punkte soll = formVon(*primitivGespiegelt(*u.p, achse, ebene, kx), kx);
            if (formGleich(soll, formVon(*u.p, kx))) {
                selbst = true;
                return {};
            }
            std::vector<std::shared_ptr<Entitaet>> orte{u.e};
            if (auto it = partnerEntitaet.find(u.e.get()); it != partnerEntitaet.end()) { orte.push_back(it->second); }
            for (const auto& ort : orte) {
                for (const auto& c : ort->kinder()) {
                    if (c == u.p || c->art() != u.p->art() || gepaart.count(c.get()) != 0) { continue; }
                    if (formGleich(soll, formVon(*c, kx))) { return Seite{ort, c, {}}; }
                }
            }
            return {};
        }
        const auto g = entitaetGespiegelt(k, *u.e, achse, ebene);
        const auto formAlle = [&](const Entitaet& e) {
            Punkte r;
            for (const auto& c : e.kinder()) { punkteSammeln(*c, kx, r); }
            std::sort(r.begin(), r.end());
            r.erase(std::unique(r.begin(), r.end()), r.end());
            return r;
        };
        const Punkte soll = formAlle(*g);
        double so[3] = {0, 0, 0};
        const bool mitUrsprung = ursprungLesen(g->wert("origin"), so);
        const auto passt = [&](const Entitaet& e) {
            if (e.klassenname() != g->klassenname() || e.kinder().size() != g->kinder().size()) { return false; }
            if (mitUrsprung) {
                double v[3];
                if (!ursprungLesen(e.wert("origin"), v)) { return false; }
                for (int i = 0; i < 3; ++i) {
                    if (std::fabs(v[i] - so[i]) > 0.05) { return false; }
                }
            }
            return formGleich(soll, formAlle(e));
        };
        if (passt(*u.e) && entitaetText(*g, kx) == entitaetText(*u.e, kx)) {
            selbst = true;
            return {};
        }
        for (const auto& e : k.entitaeten()) {
            if (e == u.e || gepaart.count(e.get()) != 0 || e->klassenname() == "worldspawn") { continue; }
            if (passt(*e)) { return Seite{e, {}, {}}; }
        }
        return {};
    };
    const auto anlegen = [&](const Einheit& u) {
        Seite b;
        if (u.p) {
            std::shared_ptr<Entitaet> ort = u.e;
            if (auto it = partnerEntitaet.find(u.e.get()); it != partnerEntitaet.end()) { ort = it->second; }
            auto m = primitivGespiegelt(*u.p, achse, ebene, kx);
            m->schicht = u.p->schicht;
            ort->kindEinfuegen(m);
            b = Seite{ort, m, {}};
        } else {
            auto g = entitaetGespiegelt(k, *u.e, achse, ebene);
            k.entitaetEinfuegen(g);
            b = Seite{g, {}, {}};
        }
        ++m_uebertragungen;
        paarBilden(k, Seite{u.e, u.p, {}}, b);
        gepaart.insert(u.schluessel());
        gepaart.insert(u.p ? static_cast<const void*>(b.p.lock().get()) : static_cast<const void*>(b.e.lock().get()));
    };
    for (const Einheit& u : gewaehlteEinheiten(k, hoechstens)) {
        if (gepaart.count(u.schluessel()) != 0) { continue; }
        if (!lebt(u.e, u.p, static_cast<bool>(u.p))) { continue; }
        auto it = m_einzeln.find(u.schluessel());
        if (it == m_einzeln.end()) {
            bool selbst = false;
            const Seite z = zwillingSuchen(u, selbst);
            if (!nieBelegt(z.e)) {
                paarBilden(k, Seite{u.e, u.p, {}}, z);
                gepaart.insert(u.schluessel());
                gepaart.insert(u.p ? static_cast<const void*>(z.p.lock().get()) : static_cast<const void*>(z.e.lock().get()));
                continue;
            }
            if (befehlOffen && !selbst) {   // im Befehl neu (aufgezogen, eingefuegt, geklont): gleich spiegeln
                anlegen(u);
                continue;
            }
            Einzel ein;
            ein.e = u.e;
            ein.p = u.p;
            ein.text = objektText(*u.e, u.p.get(), kx);
            ein.selbst = selbst;
            ein.gesehen = true;
            m_einzeln.emplace(u.schluessel(), std::move(ein));
            continue;
        }
        Einzel& ein = it->second;
        ein.gesehen = true;
        if (!geaendert) { continue; }
        std::string t = objektText(*u.e, u.p.get(), kx);
        if (t == ein.text) { continue; }
        if (befehlOffen && !ein.selbst) {
            m_einzeln.erase(it);
            anlegen(u);
            continue;
        }
        ein.text = std::move(t);
    }
    for (auto it = m_einzeln.begin(); it != m_einzeln.end();) {
        it = it->second.gesehen ? std::next(it) : m_einzeln.erase(it);
    }
    if (entfernt) { k.auswahlBereinigen(); }
    m_aenderungen = k.undo.aenderungen();
    m_laeuft = false;
}

}  // namespace bhed::karte
