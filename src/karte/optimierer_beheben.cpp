// optimierer_beheben.cpp - Karten-Editor, Stufe 5 Punkt 4: der Optimierer
// behebt, was er gefunden hat - je Vorschlag ein Rueckgaengig-Schritt, "Alles
// Sichere beheben" als einer (siehe optimierer.h).
//
// Portiert aus NetRadiant-Custom 20260114 (die Befehle, die eine Behebung
// benutzt): brush.h Face::SetShader / Brush::setDetail (MakeDetail,
// MakeStructural), brush.cpp vertex snapping (Snap to Grid, Brush::snapto),
// contrib/bobtoolz DPatch::Split (Split patch), radiant/select.cpp
// Select_Delete; dazu OpenJK/q3map2 fuer die worldspawn-Schluessel
// (gridsize: tr_bsp.cpp R_LoadEntities, light.cpp; _lightmapscale:
// lightmaps_ydnar.cpp).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   Copyright (C) 2001-2006, William Joseph. All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/optimierer.h"

#include "bhed/karte/bobtoolz.h"
#include "bhed/karte/karte.h"
#include "bobtoolz_d.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>

namespace bhed::karte {

namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

constexpr const char* kCaulk = "textures/common/caulk";

struct Aufgeloest {
    std::shared_ptr<Entitaet> ent;
    std::shared_ptr<Primitiv> prim;
    int flaeche = -1;
    std::string wert;   // ObjektRef::wert
};

// Die Objekte eines Befunds als Zeiger in die Karte (vor jeder Aenderung).
std::vector<Aufgeloest> aufloesen(const Karte& k, const Befund& b) {
    std::vector<Aufgeloest> r;
    const auto& ents = k.entitaeten();
    for (const ObjektRef& o : b.objekte) {
        if (o.entitaet < 0 || o.entitaet >= static_cast<int>(ents.size())) { continue; }
        Aufgeloest a;
        a.ent = ents[static_cast<std::size_t>(o.entitaet)];
        if (o.primitiv >= 0) {
            if (o.primitiv >= static_cast<int>(a.ent->kinder().size())) { continue; }
            a.prim = a.ent->kinder()[static_cast<std::size_t>(o.primitiv)];
        }
        a.flaeche = o.flaeche;
        a.wert = o.wert;
        r.push_back(std::move(a));
    }
    return r;
}

std::string mehrzahl(long long n, const char* eins, const char* viele) { return std::to_string(n) + " " + (n == 1 ? eins : viele); }

// Die Behebung ohne eigenen Rueckgaengig-Schritt.
BehebungsErgebnis anwenden(Karte& k, const Befund& b, const std::vector<Aufgeloest>& objekte) {
    BehebungsErgebnis e;
    e.ok = true;
    switch (b.behebung) {
    case Behebung::Keine:
        e.ok = false;
        e.meldung = "nothing to fix automatically";
        break;
    case Behebung::Caulk:
        for (const Aufgeloest& a : objekte) {
            if (!a.prim || a.prim->art() != Primitiv::Art::Pinsel || a.flaeche < 0) { continue; }
            auto& p = static_cast<Pinsel&>(*a.prim);
            if (a.flaeche >= static_cast<int>(p.anzahl())) { continue; }
            Flaeche& f = p[static_cast<std::size_t>(a.flaeche)];
            if (klein(f.shader()) == kCaulk) { continue; }
            f.setzeShader(kCaulk, k.kontext);   // Face::SetShader
            ++e.geaendert;
        }
        e.undoName = "Optimizer: caulk " + mehrzahl(e.geaendert, "hidden face", "hidden faces");
        break;
    case Behebung::Detail:
    case Behebung::Struktur: {
        const bool detail = b.behebung == Behebung::Detail;
        for (const Aufgeloest& a : objekte) {
            if (!a.prim || a.prim->art() != Primitiv::Art::Pinsel) { continue; }
            auto& p = static_cast<Pinsel&>(*a.prim);
            if (p.istDetail() == detail) { continue; }
            p.setzeDetail(detail);   // MakeDetail / MakeStructural
            ++e.geaendert;
        }
        e.undoName = std::string("Optimizer: make ") + mehrzahl(e.geaendert, "brush", "brushes") + (detail ? " detail" : " structural");
        break;
    }
    case Behebung::Einrasten: {
        long long kaputt = 0;
        for (const Aufgeloest& a : objekte) {
            if (!a.prim || a.prim->art() != Primitiv::Art::Pinsel) { continue; }
            auto& p = static_cast<Pinsel&>(*a.prim);
            p.eckenEinrasten(1.0F, k.kontext);   // Snap to Grid (Raster 1)
            ++e.geaendert;
            if (p.entartet(k.kontext)) { ++kaputt; }
        }
        e.undoName = "Optimizer: snap " + mehrzahl(e.geaendert, "brush", "brushes") + " to the grid";
        if (kaputt > 0) { e.meldung = std::to_string(kaputt) + " brushes became degenerate - undo and fix by hand"; }
        break;
    }
    case Behebung::PinselEntfernen:
        for (const Aufgeloest& a : objekte) {
            if (!a.prim || !a.ent) { continue; }
            k.auswahl.abwaehlen(a.ent, a.prim);
            if (a.ent->kindEntfernen(a.prim.get())) { ++e.geaendert; }
        }
        e.undoName = "Optimizer: delete " + mehrzahl(e.geaendert, "brush", "brushes");
        break;
    case Behebung::PatchTeilen: {
        bobtoolz::Meldungen m;
        for (const Aufgeloest& a : objekte) {
            if (!a.prim || !a.ent || a.prim->art() != Primitiv::Art::Patch) { continue; }
            bobtoolz::d::DPatch patch;
            patch.LoadFromPatch(a.ent, std::static_pointer_cast<Patch>(a.prim));
            std::list<bobtoolz::d::DPatch> teile = patch.Split(m);   // DoSplitPatch
            if (teile.size() < 2) { continue; }
            for (auto&& t : teile) { (void)t.BuildInRadiant(k, a.ent); }
            bobtoolz::d::primitivLoeschen(k, a.ent, a.prim);
            ++e.geaendert;
        }
        e.undoName = "Optimizer: split " + mehrzahl(e.geaendert, "patch", "patches");
        break;
    }
    case Behebung::ModelleZusammenlegen: {
        // gleiche Klasse und gleiche Schluessel (ohne "model"): die Primitive in
        // die erste Entitaet der Gruppe, die anderen loeschen
        std::map<std::string, std::vector<std::shared_ptr<Entitaet>>> gruppen;
        std::set<const Entitaet*> gesehen;
        for (const Aufgeloest& a : objekte) {
            if (!a.ent || !gesehen.insert(a.ent.get()).second) { continue; }
            std::vector<std::pair<std::string, std::string>> s;
            for (const auto& kv : a.ent->schluessel()) {
                const std::string kk = klein(kv.first);
                if (kk == "model") { continue; }
                s.emplace_back(kk, kv.second);
            }
            std::sort(s.begin(), s.end());
            std::string schl;
            for (const auto& kv : s) { schl += kv.first + "=" + kv.second + ";"; }
            gruppen[schl].push_back(a.ent);
        }
        for (auto& [schl, liste] : gruppen) {
            if (liste.size() < 2) { continue; }
            const std::shared_ptr<Entitaet>& ziel = liste.front();
            for (std::size_t i = 1; i < liste.size(); ++i) {
                const std::vector<std::shared_ptr<Primitiv>> kinder = liste[i]->kinder();
                for (const auto& p : kinder) {
                    k.auswahl.abwaehlen(liste[i], p);
                    (void)liste[i]->kindEntfernen(p.get());
                    ziel->kindEinfuegen(p);
                }
                k.auswahl.abwaehlen(liste[i]);
                if (k.entitaetEntfernen(liste[i].get())) { ++e.geaendert; }
            }
        }
        e.undoName = "Optimizer: merge " + mehrzahl(e.geaendert, "brush entity", "brush entities");
        break;
    }
    case Behebung::WeltSchluessel: {
        auto welt = k.weltHolenOderAnlegen();
        if (welt->wert(b.schluessel) != b.wert) {
            welt->setzen(b.schluessel, b.wert);
            e.geaendert = 1;
        }
        e.undoName = "Optimizer: worldspawn " + b.schluessel + " " + b.wert;
        break;
    }
    case Behebung::EntitaetEntfernen:
        for (const Aufgeloest& a : objekte) {
            if (!a.ent || a.prim) { continue; }
            k.auswahl.abwaehlen(a.ent);
            if (k.entitaetEntfernen(a.ent.get())) { ++e.geaendert; }
        }
        e.undoName = "Optimizer: delete " + mehrzahl(e.geaendert, "entity", "entities");
        break;
    case Behebung::TexturErsetzen:
        for (const Aufgeloest& a : objekte) {
            if (!a.prim || a.prim->art() != Primitiv::Art::Pinsel || a.flaeche < 0) { continue; }
            auto& p = static_cast<Pinsel&>(*a.prim);
            if (a.flaeche >= static_cast<int>(p.anzahl())) { continue; }
            Flaeche& f = p[static_cast<std::size_t>(a.flaeche)];
            if (klein(f.shader()) == klein(b.wert)) { continue; }
            f.setzeShader(b.wert, k.kontext);   // Face::SetShader
            ++e.geaendert;
        }
        e.undoName = "Optimizer: " + b.wert + " on " + mehrzahl(e.geaendert, "face", "faces");
        break;
    case Behebung::EntitaetSchluessel: {
        long long ohne = 0;
        for (const Aufgeloest& a : objekte) {
            if (!a.ent || a.prim) { continue; }
            if (a.wert.empty()) {
                ++ohne;
                continue;
            }
            if (a.ent->wert(b.schluessel) == a.wert) { continue; }
            a.ent->setzen(b.schluessel, a.wert);   // SetKeyValue
            ++e.geaendert;
        }
        e.undoName = "Optimizer: " + b.schluessel + " on " + mehrzahl(e.geaendert, "entity", "entities");
        if (ohne > 0) { e.meldung = std::to_string(ohne) + " entities without a suggested value - fix them by hand"; }
        break;
    }
    }
    if (e.ok && e.geaendert == 0 && e.meldung.empty()) { e.meldung = "already fixed"; }
    return e;
}

}  // namespace

bool abbildPasst(const Karte& k, const KartenAbbild& a) {
    return k.undo.stand() == a.stand && k.entitaeten().size() == a.ents.size();
}

std::string behebungsVorschau(const Befund& b) {
    const std::string n = std::to_string(b.anzahl);
    switch (b.behebung) {
    case Behebung::Keine: return "show only - fix by hand";
    case Behebung::Caulk: return n + " faces get " + std::string(kCaulk);
    case Behebung::Detail: return n + " brushes become detail";
    case Behebung::Struktur: return n + " brushes become structural";
    case Behebung::Einrasten: return n + " brushes are snapped to the 1-unit grid";
    case Behebung::PinselEntfernen: return n + " brushes are deleted";
    case Behebung::PatchTeilen: return n + " patches are split into 3x3 pieces (bobToolz Split patch)";
    case Behebung::ModelleZusammenlegen: return n + " brush entities are merged into one per group";
    case Behebung::WeltSchluessel: return "worldspawn: \"" + b.schluessel + "\" \"" + b.wert + "\" (rebuild and relight afterwards)";
    case Behebung::EntitaetEntfernen: return n + " entities are deleted";
    case Behebung::TexturErsetzen: return n + " faces get " + b.wert;
    case Behebung::EntitaetSchluessel: return n + " entities get \"" + b.schluessel + "\" (only the cull point moves, the model stays)";
    }
    return std::string();
}

BehebungsErgebnis beheben(Karte& k, const KartenAbbild& a, const Befund& b) {
    BehebungsErgebnis e;
    if (!abbildPasst(k, a)) {
        e.meldung = "the map changed since the check - check again";
        return e;
    }
    if (b.behebung == Behebung::Keine) {
        e.meldung = "nothing to fix automatically";
        return e;
    }
    const std::vector<Aufgeloest> objekte = aufloesen(k, b);
    k.undo.start();
    e = anwenden(k, b, objekte);
    k.undo.finish(e.undoName);
    k.auswahlBereinigen();
    return e;
}

std::vector<std::pair<std::string, BehebungsErgebnis>> allesSichereBeheben(Karte& k, const KartenAbbild& a, const std::vector<Befund>& befunde) {
    std::vector<std::pair<std::string, BehebungsErgebnis>> r;
    if (!abbildPasst(k, a)) { return r; }
    // erst alle Objekte aufloesen, dann aendern (Loeschen verschiebt die Indizes)
    std::vector<std::pair<const Befund*, std::vector<Aufgeloest>>> plan;
    for (const Befund& b : befunde) {
        if (!b.sicher || b.behebung == Behebung::Keine) { continue; }
        plan.emplace_back(&b, aufloesen(k, b));
    }
    if (plan.empty()) { return r; }
    k.undo.start();
    long long summe = 0;
    for (const auto& [b, objekte] : plan) {
        BehebungsErgebnis e = anwenden(k, *b, objekte);
        summe += e.geaendert;
        r.emplace_back(b->pruefung, std::move(e));
    }
    k.undo.finish("Optimizer: fix all safe (" + std::to_string(summe) + ")");
    k.auswahlBereinigen();
    return r;
}

}  // namespace bhed::karte
