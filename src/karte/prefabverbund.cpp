// prefabverbund.cpp - Karten-Editor, Stufe 9 Punkt 6: verknuepfte Prefabs (siehe prefabverbund.h)
//
// Portiert aus NetRadiant-Custom 20260114 (die Bausteine: func_group, Brush::copy,
// Brush::transform, Patch::transform, die Undo-Befehle); der Verbund ist neu.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#include "bhed/karte/prefabverbund.h"

#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/rueckgaengig.h"

#include <cmath>
#include <cstdio>
#include <set>
#include <vector>

namespace bhed::karte {

namespace {

Vektor3f ursprungVon(const Entitaet& e) {
    float v[3] = {0, 0, 0};
    (void)std::sscanf(e.wert(kVerbundUrsprung).c_str(), "%f %f %f", &v[0], &v[1], &v[2]);
    return Vektor3f(v[0], v[1], v[2]);
}

std::string zahl3(const Vektor3f& v) {
    char z[96];
    std::snprintf(z, sizeof(z), "%g %g %g", static_cast<double>(v[0]), static_cast<double>(v[1]), static_cast<double>(v[2]));
    return z;
}

// Kopie eines Primitivs, verschoben, mit Texture Lock (die Textur wandert mit)
std::shared_ptr<Primitiv> verschoben(const Primitiv& p, const Vektor3f& d, const Kontext& k) {
    Kontext kl = k;
    kl.texturLock = true;
    const Matrix4 m = verschiebungsMatrix(d);
    if (p.art() == Primitiv::Art::Pinsel) {
        auto c = static_cast<const Pinsel&>(p).kopie(kl);
        c->schicht = p.schicht;
        c->transformieren(m, kl);
        return c;
    }
    auto c = static_cast<const Patch&>(p).kopie();
    c->schicht = p.schicht;
    c->transformieren(m);
    return c;
}

std::string inhaltRelativ(const Entitaet& e, const Vektor3f& ursprung, const Kontext& k) {
    std::string t;
    const Vektor3f minus(-ursprung[0], -ursprung[1], -ursprung[2]);
    for (const auto& p : e.kinder()) {
        if (!p) { continue; }
        t += primitivText(*verschoben(*p, minus, k), k);
    }
    return t;
}

bool ankerVon(const Entitaet& e, Vektor3f& a) {
    for (const auto& p : e.kinder()) {
        if (!p) { continue; }
        if (p->art() == Primitiv::Art::Pinsel) {
            const auto& b = static_cast<const Pinsel&>(*p);
            if (b.leer()) { continue; }
            const Vektor3d& q = b[0].ebenenPunkte()[0];
            a = Vektor3f(static_cast<float>(q[0]), static_cast<float>(q[1]), static_cast<float>(q[2]));
            return true;
        }
        const auto& pa = static_cast<const Patch&>(*p);
        if (!pa.punkte().empty()) {
            a = pa.punkte().front().vertex;
            return true;
        }
    }
    return false;
}

}  // namespace

std::shared_ptr<Entitaet> verbundKopieAnlegen(Karte& k, const std::string& name, const Vektor3f& versatz, float raster) {
    std::shared_ptr<Entitaet> quelle;
    std::vector<std::pair<std::shared_ptr<Entitaet>, std::shared_ptr<Primitiv>>> lose;
    for (const AuswahlElement& a : k.auswahl.elemente()) {
        const auto ent = a.entitaet.lock();
        if (!ent) { continue; }
        if (ent->wert("classname") == "func_group" && !ent->wert(kVerbundName).empty()) {
            quelle = ent;
            break;
        }
        const auto p = a.primitiv.lock();
        if (p && ent->wert("classname") == "worldspawn") { lose.emplace_back(ent, p); }
    }
    if (!quelle && lose.empty()) { return nullptr; }
    Befehl undo(k.undo, "linkedCopy");
    if (!quelle) {
        // eine neue Gruppe aus den losen Primitiven, Ursprung = Mitte auf dem Raster
        Vektor3f mn(1e30F, 1e30F, 1e30F), mx(-1e30F, -1e30F, -1e30F);
        for (const auto& [ent, p] : lose) {
            (void)ent;
            const Kasten b = p->art() == Primitiv::Art::Pinsel ? static_cast<const Pinsel&>(*p).grenzen(k.kontext) : static_cast<const Patch&>(*p).grenzen();
            for (std::size_t i = 0; i < 3; ++i) {
                mn[i] = std::min(mn[i], b.origin[i] - b.extents[i]);
                mx[i] = std::max(mx[i], b.origin[i] + b.extents[i]);
            }
        }
        Vektor3f u((mn[0] + mx[0]) * 0.5F, (mn[1] + mx[1]) * 0.5F, mn[2]);
        einrasten(u, raster > 0 ? raster : 1.0F);
        quelle = k.neueEntitaet("func_group", true);
        quelle->setzen(kVerbundName, name);
        quelle->setzen(kVerbundUrsprung, zahl3(u));
        k.entitaetEinfuegen(quelle);
        for (const auto& [ent, p] : lose) {
            (void)ent->kindEntfernen(p.get());
            quelle->kindEinfuegen(p);
        }
    }
    auto kopie = k.neueEntitaet("func_group", true);
    for (const auto& [s, w] : quelle->schluessel()) {
        if (s != "classname") { kopie->setzen(s, w); }
    }
    kopie->setzen(kVerbundUrsprung, zahl3(ursprungVon(*quelle) + versatz));
    k.entitaetEinfuegen(kopie);
    for (const auto& p : quelle->kinder()) {
        if (p) { kopie->kindEinfuegen(verschoben(*p, versatz, k.kontext)); }
    }
    k.auswahl.leeren();
    for (const auto& p : kopie->kinder()) { k.auswahl.waehlen(kopie, p); }
    return kopie;
}

bool verbundLoesen(Karte& k) {
    std::set<Entitaet*> gruppen;
    for (const AuswahlElement& a : k.auswahl.elemente()) {
        const auto ent = a.entitaet.lock();
        if (ent && !ent->wert(kVerbundName).empty()) { gruppen.insert(ent.get()); }
    }
    if (gruppen.empty()) { return false; }
    Befehl undo(k.undo, "unlinkCopy");
    for (Entitaet* e : gruppen) {
        e->setzen(kVerbundName, "");
        e->setzen(kVerbundUrsprung, "");
    }
    return true;
}

void PrefabVerbund::abgleichen(Karte& k, bool befehlOffen) {
    if (!befehlOffen && k.undo.aenderungen() == m_aenderungen) { return; }
    // die Exemplare und ihr jetziger Inhalt
    struct Jetzt {
        std::shared_ptr<Entitaet> e;
        std::string name;
        Vektor3f ursprung;
        std::string text;
        Vektor3f anker;
        bool ankerDa = false;
    };
    std::vector<Jetzt> jetzt;
    for (const auto& e : k.entitaeten()) {
        const std::string n = e->wert(kVerbundName);
        if (n.empty() || e->wert("classname") != "func_group") { continue; }
        Jetzt j;
        j.e = e;
        j.name = n;
        j.ursprung = ursprungVon(*e);
        j.text = inhaltRelativ(*e, j.ursprung, k.kontext);
        j.ankerDa = ankerVon(*e, j.anker);
        jetzt.push_back(std::move(j));
    }
    if (befehlOffen) {
        std::set<std::string> fertig;
        for (Jetzt& j : jetzt) {
            const auto it = m_stand.find(j.e.get());
            if (it == m_stand.end() || it->second.text == j.text || fertig.count(j.name) != 0) { continue; }
            // nur verschoben? Dann wandert der Ursprung mit
            if (j.ankerDa) {
                const Vektor3f d = j.anker - it->second.minimum;
                const Vektor3f neu = j.ursprung + d;
                if (inhaltRelativ(*j.e, neu, k.kontext) == it->second.text) {
                    j.e->setzen(kVerbundUrsprung, zahl3(neu));
                    j.ursprung = neu;
                    j.text = it->second.text;
                    continue;
                }
            }
            // Inhalt geaendert: alle anderen Exemplare desselben Verbunds neu schreiben
            fertig.insert(j.name);
            for (Jetzt& o : jetzt) {
                if (o.e == j.e || o.name != j.name) { continue; }
                const auto alt = o.e->kinder();
                for (const auto& p : alt) { (void)o.e->kindEntfernen(p.get()); }
                const Vektor3f d = o.ursprung - j.ursprung;
                for (const auto& p : j.e->kinder()) {
                    if (p) { o.e->kindEinfuegen(verschoben(*p, d, k.kontext)); }
                }
                o.text = j.text;
                o.ankerDa = ankerVon(*o.e, o.anker);
            }
        }
    }
    m_stand.clear();
    for (const Jetzt& j : jetzt) { m_stand[j.e.get()] = Stand{j.e, j.name, j.text, j.ursprung, j.anker}; }
    m_aenderungen = k.undo.aenderungen();
}

}  // namespace bhed::karte
