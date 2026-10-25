// auswahlbefehle.cpp - Karten-Editor: die Auswahl-Befehle aus select.cpp.
// Portiert aus NetRadiant-Custom 20260114 (Quellen in auswahlbefehle.h).
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
#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/komponenten.h"

#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/entitaetbefehle.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/kartenformat.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"
#include "bhed/karte/texturen.h"
#include "bhed/karte/wicklung.h"
#include "bhed/karte/xyansicht.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <unordered_set>

namespace bhed::karte {

// --- libs/uniquenames.h ------------------------------------------------------------

NamensZahl::NamensZahl(const std::string& text)
    : wert(text.empty() ? -1 : std::atoi(text.c_str())), mindestBreite(!text.empty() && text[0] == '0' ? static_cast<int>(text.size()) : 0) {}

Name nameLesen(const std::string& name) {
    // die Ziffern am Ende (strrchr( "1234567890", *p ))
    std::size_t ende = name.size();
    while (ende > 0 && std::isdigit(static_cast<unsigned char>(name[ende - 1])) != 0) { --ende; }
    return Name{name.substr(0, ende), NamensZahl(name.substr(ende))};
}

std::string nameSchreiben(const Name& n) {
    std::string s = n.text;
    if (n.zahl.wert != -1) {
        char puffer[32];
        std::snprintf(puffer, sizeof(puffer), "%0*i", n.zahl.mindestBreite, n.zahl.wert);
        s += puffer;
    }
    return s;
}

Name EindeutigeNamen::eindeutig(const Name& n) const {
    const auto it = m_namen.find(n.text);
    if (it == m_namen.end()) { return n; }
    const auto& zahlen = it->second;
    if (zahlen.find(n.zahl) == zahlen.end()) { return n; }   // PostFixes::make_unique
    // find_first_empty: ab 1 die erste Luecke
    NamensZahl z("1");
    for (auto i = zahlen.find(z); i != zahlen.end(); ++i, ++z.wert) {
        if (i->first != z) { break; }
    }
    return Name{n.text, z};
}

void EindeutigeNamen::einfuegen(const Name& n) { ++m_namen[n.text][n.zahl]; }

void EindeutigeNamen::entfernen(const Name& n) {
    const auto it = m_namen.find(n.text);
    if (it == m_namen.end()) { return; }
    const auto z = it->second.find(n.zahl);
    if (z != it->second.end() && --z->second == 0) { it->second.erase(z); }
    if (it->second.empty()) { m_namen.erase(it); }
}

bool schluesselIstName(const std::string& s) {
    if (s == "target" || s == "targetname" || s == "killtarget") { return true; }
    if (s.size() > 6 && s.compare(0, 6, "target") == 0) {   // targetN (Nexuiz): string_is_integer
        const char* rest = s.c_str() + 6;
        char* e = nullptr;
        (void)std::strtol(rest, &e, 10);
        return *e == '\0';
    }
    return false;
}

// BasicNamespace::mergeNames
void namenZusammenfuehren(Karte& k, const std::vector<Entitaet*>& neue) {
    const std::set<const Entitaet*> neuSet(neue.begin(), neue.end());
    EindeutigeNamen namen;   // g_defaultNamespace (NameObserver: nur nicht leere)
    for (const auto& e : k.entitaeten()) {
        if (neuSet.count(e.get()) != 0) { continue; }
        for (const auto& [s, v] : e->schluessel()) {
            if (schluesselIstName(s) && !v.empty()) { namen.einfuegen(nameLesen(v)); }
        }
    }
    std::map<std::string, std::vector<std::pair<Entitaet*, std::string>>> gruppen;
    for (Entitaet* e : neue) {
        for (const auto& [s, v] : e->schluessel()) {
            if (schluesselIstName(s)) { gruppen[v].emplace_back(e, s); }
        }
    }
    for (const auto& [name, schluessel] : gruppen) {
        const Name eindeutig = namen.eindeutig(nameLesen(name));
        namen.einfuegen(eindeutig);
        const std::string text = nameSchreiben(eindeutig);
        for (const auto& [e, s] : schluessel) { e->setzen(s, text); }
    }
}

namespace {

std::shared_ptr<Primitiv> primitivKopie(const Primitiv& p, const Kontext& k) {
    if (p.art() == Primitiv::Art::Pinsel) { return static_cast<const Pinsel&>(p).kopie(k); }
    return static_cast<const Patch&>(p).kopie();
}

// Node_Clone / Node_Clone_Selected einer Entitaet
template<typename F>
std::shared_ptr<Entitaet> entitaetKopie(const Entitaet& e, const Kontext& k, F nimmKind) {
    auto c = std::make_shared<Entitaet>(e.klasse());
    for (const auto& [s, v] : e.schluessel()) { c->setzen(s, v); }
    c->schicht = e.schicht;
    for (const auto& p : e.kinder()) {
        if (nimmKind(*p)) { c->kindEinfuegen(primitivKopie(*p, k)); }
    }
    return c;
}

// Instance_isSelected fuer Entitaeten (ganz) und Primitive
struct GewaehltMenge {
    std::unordered_set<const Entitaet*> ganz;
    std::unordered_set<const Primitiv*> prim;
    explicit GewaehltMenge(const Karte& k) {
        const auto gesetzt = [](const std::weak_ptr<Primitiv>& w) {
            return w.owner_before(std::weak_ptr<Primitiv>{}) || std::weak_ptr<Primitiv>{}.owner_before(w);
        };
        for (const auto& el : k.auswahl.elemente()) {
            const auto e = el.entitaet.lock();
            const auto p = el.primitiv.lock();
            if (!e) { continue; }
            if (p) {
                prim.insert(p.get());
            } else if (!gesetzt(el.primitiv)) {   // ein geloeschtes Primitiv ist nicht die ganze Entitaet
                ganz.insert(e.get());
            }
        }
    }
    [[nodiscard]] bool kindGewaehlt(const Entitaet& e) const {   // instance.childSelected()
        return std::any_of(e.kinder().begin(), e.kinder().end(), [&](const auto& p) { return prim.count(p.get()) != 0; });
    }
};

bool sichtbar(const Entitaet& e) { return e.sichtbar(); }   // Node::visible
bool sichtbar(const Primitiv& p) { return p.sichtbar(); }

bool shaderGleich(const std::string& a, const std::string& b) {   // shader_equal: ohne Gross/klein
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}

}  // namespace

// --- select.cpp ------------------------------------------------------------------------

void auswahlAufheben(Karte& k, AuswahlEinstellung& e) {   // Selection_Deselect (select.cpp:1402-1423)
    if (e.modus == AuswahlModus::Komponente) {
        if (komponentenAnzahl(k) != 0) {
            alleKomponentenAbwaehlen(k);
        } else {   // SelectionSystem_DefaultMode
            e.modus = AuswahlModus::Primitiv;
            e.komponente = KomponentenModus::Standard;
        }
    } else if (komponentenAnzahl(k) != 0) {
        alleKomponentenAbwaehlen(k);
    } else {
        k.auswahl.leeren();
    }
}

void auswahlUmkehren(Karte& k, const AuswahlEinstellung& e) {   // InvertSelectionWalker
    if (e.modus == AuswahlModus::Komponente) {   // InvertSelectionWalker eComponent
        komponentenUmkehren(k, e.komponente);
        return;
    }
    const auto liste = k.entitaeten();
    for (const auto& ent : liste) {
        if (!sichtbar(*ent)) { continue; }
        if (e.modus == AuswahlModus::Entitaet) {
            // die Entitaet wechselt bei ihrem ersten Kind (oder selbst ohne Kinder);
            // ein verstecktes erstes Kind setzt m_selectable = 0
            if (ent->kinder().empty() || sichtbar(*ent->kinder().front())) { k.auswahl.umschalten(ent); }
            continue;
        }
        // ePrimitive: hat die Entitaet Kinder, wechseln nur die sichtbaren Kinder
        if (ent->kinder().empty()) {
            k.auswahl.umschalten(ent);
            continue;
        }
        for (const auto& p : ent->kinder()) {
            if (sichtbar(*p)) { k.auswahl.umschalten(ent, p); }
        }
    }
}

void alleVomTyp(Karte& k, const AuswahlEinstellung& e, const std::string& browserShader) {   // Select_AllOfType
    if (e.modus == AuswahlModus::Komponente) {
        if (e.komponente == KomponentenModus::Flaeche) {
            k.auswahl.flaechenLeeren();
            const GewaehltMenge g(k);   // Scene_BrushSelectByShader_Component: gewaehlte Pinsel
            for (const auto& ent : k.entitaeten()) {
                for (const auto& p : ent->kinder()) {
                    if (p->art() != Primitiv::Art::Pinsel || g.prim.count(p.get()) == 0) { continue; }
                    const auto pinsel = std::static_pointer_cast<Pinsel>(p);
                    for (const auto& f : pinsel->flaechen()) {
                        if (shaderGleich(f->shader(), browserShader)) { k.auswahl.flaecheWaehlen(pinsel, f); }
                    }
                }
            }
        }
        return;
    }
    // EntityGetSelectedPropertyValuesWalker: classname gewaehlter Entitaeten (ohne worldspawn)
    const auto welt = k.welt();
    std::vector<std::string> klassen;
    {
        const GewaehltMenge g(k);
        for (const auto& ent : k.entitaeten()) {
            if (ent == welt) { continue; }
            if (g.ganz.count(ent.get()) != 0 || g.kindGewaehlt(*ent)) {
                const std::string c = ent->wert("classname");
                if (std::find(klassen.begin(), klassen.end(), c) == klassen.end()) { klassen.push_back(c); }
            }
        }
    }
    k.auswahl.leeren();   // setSelectedAll( false )
    if (!klassen.empty()) {   // EntityFindByPropertyValueWalker
        for (const auto& ent : k.entitaeten()) {
            if (!sichtbar(*ent) || ent == welt) { continue; }
            if (std::find(klassen.begin(), klassen.end(), ent->wert("classname")) == klassen.end()) { continue; }
            k.auswahl.waehlen(ent);
            for (const auto& p : ent->kinder()) {
                if (sichtbar(*p)) { k.auswahl.waehlen(ent, p); }
            }
        }
        return;
    }
    waehleNachShader(k, browserShader);   // Scene_BrushPatchSelectByShader
}

bool versteckeGewaehlte(Karte& k, bool versteckt) {   // HideSelected
    const GewaehltMenge g(k);
    bool neu = versteckt;
    for (const auto& ent : k.entitaeten()) {
        if (g.ganz.count(ent.get()) != 0) {
            neu = true;
            ent->versteckt = true;
        }
        for (const auto& p : ent->kinder()) {
            if (g.prim.count(p.get()) != 0) {
                neu = true;
                p->versteckt = true;
            }
        }
        // Behaelter: Name verstecken, wenn der Inhalt ganz versteckt ist
        if (ent->istBehaelter() && std::all_of(ent->kinder().begin(), ent->kinder().end(), [](const auto& p) { return p->versteckt; })) {
            ent->versteckt = true;
        }
    }
    // worldspawn nicht verstecken, damit neue Pinsel sichtbar sind
    if (const auto welt = k.welt()) { welt->versteckt = false; }
    k.auswahl.leeren();   // setSelectedAllComponents( false ), setSelectedAll( false )
    return neu;
}

void zeigeVersteckte(Karte& k) {   // Select_ShowAllHidden
    for (const auto& ent : k.entitaeten()) {
        ent->versteckt = false;
        for (const auto& p : ent->kinder()) { p->versteckt = false; }
    }
}

void loescheAuswahl(Karte& k, const AuswahlEinstellung& e, bool eckenSperre) {   // deleteSelection (select.cpp:1170-1179)
    if (e.modus == AuswahlModus::Komponente && komponentenAnzahl(k) != 0) {
        Befehl b(k.undo, "deleteSelectedComponents");
        eckenEntfernen(k, eckenSperre);   // CSG_DeleteComponents
        return;
    }
    k.undo.start();
    auswahlEntfernen(k);
    k.undo.finish("deleteSelected");
}

void auswahlEntfernen(Karte& k) {   // Select_Delete / Scene_DeleteSelected (ohne eigenen Befehl)
    const GewaehltMenge g(k);
    std::unordered_set<const Primitiv*> geloeschtePrim;
    std::unordered_set<const Entitaet*> geloeschteEnt;
    const auto welt = k.welt();
    const auto liste = k.entitaeten();
    for (const auto& ent : liste) {   // DeleteSelected
        if (g.ganz.count(ent.get()) != 0) {
            k.entitaetEntfernen(ent.get());
            geloeschteEnt.insert(ent.get());
            continue;
        }
        bool kindEntfernt = false;
        const auto kinder = ent->kinder();
        for (const auto& p : kinder) {
            if (g.prim.count(p.get()) != 0) {
                ent->kindEntfernen(p.get());
                geloeschtePrim.insert(p.get());
                kindEntfernt = true;
            }
        }
        // leere Entitaeten (nicht worldspawn) mitloeschen
        if (kindEntfernt && ent != welt && ent->kinder().empty()) {
            k.entitaetEntfernen(ent.get());
            geloeschteEnt.insert(ent.get());
        }
    }
    // die geloeschten Instanzen sind abgewaehlt
    std::vector<AuswahlElement> rest;
    for (const auto& el : k.auswahl.elemente()) {
        const auto ent = el.entitaet.lock();
        const auto p = el.primitiv.lock();
        if (!ent || geloeschteEnt.count(ent.get()) != 0 || (p && geloeschtePrim.count(p.get()) != 0)) { continue; }
        rest.push_back(el);
    }
    std::vector<FlaechenWahl> restFlaechen;
    for (const auto& f : k.auswahl.flaechen()) {
        const auto p = f.pinsel.lock();
        if (p && geloeschtePrim.count(p.get()) == 0) { restFlaechen.push_back(f); }
    }
    k.auswahl.leeren();
    for (const auto& el : rest) { k.auswahl.waehlen(el.entitaet.lock(), el.primitiv.lock()); }
    for (const auto& f : restFlaechen) { k.auswahl.flaecheWaehlen(f.pinsel.lock(), f.flaeche.lock()); }
}

void klonen(Karte& k, const AuswahlEinstellung& e, bool eindeutig, bool befehl) {   // Selection_Clone(_MakeUnique)
    if (e.modus != AuswahlModus::Primitiv) { return; }
    if (befehl) { k.undo.start(); }
    const GewaehltMenge g(k);
    const auto welt = k.welt();
    std::vector<std::shared_ptr<Entitaet>> neueEnt;
    std::vector<std::pair<std::shared_ptr<Entitaet>, std::shared_ptr<Primitiv>>> neuePrim;
    const auto liste = k.entitaeten();
    for (const auto& ent : liste) {   // CloneSelected
        const auto kinderKlonen = [&]() {
            const auto kinder = ent->kinder();
            for (const auto& p : kinder) {
                if (g.prim.count(p.get()) == 0) { continue; }
                auto c = primitivKopie(*p, k.kontext);
                ent->kindEinfuegen(c);
                neuePrim.emplace_back(ent, c);
            }
        };
        if (ent == welt) {   // worldspawn nicht, aber seine Kinder
            kinderKlonen();
            continue;
        }
        if (g.ganz.count(ent.get()) != 0) {
            auto c = entitaetKopie(*ent, k.kontext, [](const Primitiv&) { return true; });
            k.entitaetEinfuegen(c);
            neueEnt.push_back(c);
            continue;
        }
        if (eindeutig && g.kindGewaehlt(*ent)) {   // gewaehlte Primitive einer Gruppe in eine neue Gruppe
            auto c = entitaetKopie(*ent, k.kontext, [&](const Primitiv& p) { return g.prim.count(&p) != 0; });
            k.entitaetEinfuegen(c);
            neueEnt.push_back(c);
            continue;
        }
        kinderKlonen();
    }
    // Map_mergeClonedNames( makeUnique )
    if (eindeutig) {
        std::vector<Entitaet*> roh;
        for (const auto& c : neueEnt) { roh.push_back(c.get()); }
        namenZusammenfuehren(k, roh);
    }
    // die Originale abwaehlen, die Klone waehlen (jede Instanz des Teilbaums)
    k.auswahl.leeren();
    for (const auto& [ent, p] : neuePrim) { k.auswahl.waehlen(ent, p); }
    for (const auto& c : neueEnt) {
        k.auswahl.waehlen(c);
        for (const auto& p : c->kinder()) { k.auswahl.waehlen(c, p); }
    }
    if (befehl) { k.undo.finish(eindeutig ? "cloneSelectedMakeUnique" : "cloneSelected"); }
}

// --- AutoCaulk (map.cpp:2121-2368) ---------------------------------------------------

void triggerAbwaehlen(Karte& k) {   // DeselectTriggers
    const GewaehltMenge g(k);
    for (const auto& e : k.entitaeten()) {
        const std::string& kl = e->klassenname();
        bool trigger = kl.size() >= 8;
        for (std::size_t i = 0; trigger && i < 8; ++i) {   // string_equal_nocase_n( ..., "trigger_", 8 )
            trigger = std::tolower(static_cast<unsigned char>(kl[i])) == "trigger_"[i];
        }
        if (!e->istBehaelter() || !trigger || !(g.ganz.count(e.get()) != 0 || g.kindGewaehlt(*e))) { continue; }
        for (const auto& p : e->kinder()) { k.auswahl.abwaehlen(e, p); }   // post: Instance_setSelected( instance, false )
        k.auswahl.abwaehlen(e);
    }
}

std::vector<std::shared_ptr<Pinsel>> autocaulkPinsel(const Karte& k) {
    const GewaehltMenge g(k);
    std::vector<std::shared_ptr<Pinsel>> v;
    for (const auto& e : k.entitaeten()) {
        const bool ganz = g.ganz.count(e.get()) != 0;
        if (!ganz && !g.kindGewaehlt(*e)) { continue; }   // Node_selectedDescendant || selectedParent
        for (const auto& p : e->kinder()) {
            if (p->art() != Primitiv::Art::Pinsel || !(ganz || g.prim.count(p.get()) != 0)) { continue; }
            v.push_back(std::static_pointer_cast<Pinsel>(p));
        }
    }
    return v;
}

bool autocaulkPunktEntitaet(const Karte& k, const Entitaet& e) {
    return !e.istBehaelter() && k.auswahl.gewaehlt(&e);   // !isContainer() && Node_instanceSelected
}

std::map<std::size_t, std::string> caulkLesen(const std::string& text) {
    // SimpleTokeniser: Leerraum trennt; getToken, getToken, nextLine
    std::map<std::size_t, std::string> m;
    std::size_t i = 0;
    const auto leer = [&](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    const auto token = [&](bool zeileBleibt, std::string& out) {
        while (i < text.size() && leer(text[i])) {
            if (zeileBleibt && text[i] == '\n') { return false; }
            ++i;
        }
        if (i >= text.size()) { return false; }
        const std::size_t a = i;
        while (i < text.size() && !leer(text[i])) { ++i; }
        out = text.substr(a, i - a);
        return true;
    };
    for (;;) {
        std::string num;
        if (!token(false, num)) { break; }
        std::size_t n = 0;
        char* ende = nullptr;
        n = static_cast<std::size_t>(std::strtoul(num.c_str(), &ende, 10));   // string_parse_size
        std::string flaechen;
        if (!token(false, flaechen)) { break; }
        while (i < text.size() && text[i] != '\n') { ++i; }   // tokeniser.nextLine()
        m.emplace(n, flaechen);
    }
    return m;
}

std::size_t caulkAnwenden(Karte& k, const std::map<std::size_t, std::string>& caulk, const std::function<bool(const std::string&)>& vorhanden) {
    const std::vector<std::shared_ptr<Pinsel>> pinsel = autocaulkPinsel(k);
    const std::string caulkS = gemeinsamerShader("caulk");
    const std::string wasser = gemeinsamerShader("watercaulk");
    const std::string lava = gemeinsamerShader("lavacaulk");
    const std::string schleim = gemeinsamerShader("slimecaulk");
    const std::string nodraw = gemeinsamerShader("nodraw");
    const std::string nodrawNichtFest = gemeinsamerShader("nodrawnonsolid");
    std::size_t anzahl = 0;
    k.undo.start();
    for (std::size_t index = 0; index < pinsel.size(); ++index) {   // m_brushIndex
        const auto it = caulk.find(index);
        if (it == caulk.end()) { continue; }
        const std::shared_ptr<Pinsel>& p = pinsel[index];
        p->bRep(k.kontext);
        const char* zeichen = it->second.c_str();
        for (const auto& f : p->flaechen()) {
            if (*zeichen == 0) { break; }
            if (!f->traegtBei()) { continue; }
            const std::string* neu = nullptr;
            switch (*zeichen) {
            case 'c': neu = &caulkS; break;
            case 'w': neu = &wasser; break;
            case 'l': neu = &lava; break;
            case 's': neu = &schleim; break;
            case 'N': neu = &nodraw; break;
            case 'n': neu = &nodrawNichtFest; break;
            default: break;
            }
            if (neu != nullptr && (!vorhanden || vorhanden(*neu))) {   // Stufe 6: kein Shader, den es im Spiel nicht gibt
                f->setzeShader(*neu, k.kontext);
                ++anzahl;
            }
            ++zeichen;
        }
    }
    k.undo.finish("AutoCaulk " + std::to_string(anzahl) + " faces");
    return anzahl;
}

std::string kopieren(const Karte& k) {   // Map_ExportSelected (IncludeSelectedWalker)
    const GewaehltMenge g(k);
    return karteSchreiben(
        k, [&](const Entitaet& e) { return g.ganz.count(&e) != 0 || g.kindGewaehlt(e); },
        [&](const Entitaet& e, const Primitiv& p) { return g.ganz.count(&e) != 0 || g.prim.count(&p) != 0; });
}

void einfuegen(Karte& k, const std::string& text, bool befehl) {   // Paste -> Map_ImportSelected -> MergeMap
    if (befehl) {
        k.undo.start();
        k.auswahl.leeren();   // setSelectedAll( false )
    }
    Karte neu;
    neu.klassen = k.klassen;
    neu.kontext = k.kontext;
    (void)karteLesen(text, neu);
    if (neu.kontext.format != k.kontext.format) {   // Convert_Brushes
        Kontext umrechnen = k.kontext;
        for (const auto& ent : neu.entitaeten()) {
            for (const auto& p : ent->kinder()) {
                if (p->art() != Primitiv::Art::Pinsel) { continue; }
                for (const auto& f : static_cast<Pinsel&>(*p).flaechen()) { f->rechneTexdefUm(neu.kontext.art(), k.kontext.art(), umrechnen); }
            }
        }
    }
    // Map_mergeLayers: gleiche Ebenen behalten, sonst alles auf die aktuelle Ebene
    bool gleicheEbenen = neu.schichten.size() == k.schichten.size();
    for (std::size_t i = 0; gleicheEbenen && i < neu.schichten.size(); ++i) {
        gleicheEbenen = neu.schichten[i].name == k.schichten[i].name && neu.schichten[i].eltern == k.schichten[i].eltern &&
                        neu.schichten[i].farbe == k.schichten[i].farbe;
    }
    if (!gleicheEbenen) {   // LayersAssignWalker( currentLayer )
        const int aktuell = static_cast<int>(k.aktuelleSchicht);
        for (const auto& ent : neu.entitaeten()) {
            if (ent->schicht != kSchichtKeine) { ent->schicht = aktuell; }
            for (const auto& p : ent->kinder()) { p->schicht = aktuell; }
        }
    }
    // Map_gatherNamespaced / Map_mergeClonedNames()
    std::vector<std::shared_ptr<Entitaet>> liste = neu.entitaeten();
    {
        std::vector<Entitaet*> roh;
        for (const auto& e : liste) {
            if (e->klassenname() != "worldspawn") { roh.push_back(e.get()); }
        }
        namenZusammenfuehren(k, roh);
    }
    // MapMergeEntities
    for (const auto& ent : liste) {
        neu.entitaetEntfernen(ent.get());   // aus der Hilfskarte loesen (Undo-Anmeldung)
        if (ent->klassenname() == "worldspawn") {
            if (const auto welt = k.welt()) {   // MapMergeAll: die Pinsel in den vorhandenen worldspawn
                const auto kinder = ent->kinder();
                for (const auto& p : kinder) {
                    ent->kindEntfernen(p.get());
                    welt->kindEinfuegen(p);
                    k.auswahl.waehlen(welt, p);
                }
            } else {
                k.entitaetEinfuegen(ent);
                for (const auto& p : ent->kinder()) { k.auswahl.waehlen(ent, p); }
            }
            continue;
        }
        k.entitaetEinfuegen(ent);
        if (ent->istBehaelter()) {   // node_is_group: SelectChildren
            for (const auto& p : ent->kinder()) { k.auswahl.waehlen(ent, p); }
        } else {
            k.auswahl.waehlen(ent);
        }
    }
    if (befehl) { k.undo.finish("paste"); }
}

Vektor3f schubsRichtung(int richtung, float menge, Sicht s) {   // NudgeSelection, AxisBase_forViewType
    Vektor3f x = kAchseX;
    Vektor3f y = kAchseY;
    if (s == Sicht::XZ) {
        y = kAchseZ;
    } else if (s == Sicht::YZ) {
        x = kAchseY;
        y = kAchseZ;
    }
    Vektor3f a;
    switch (richtung) {   // AxisBase_axisForDirection
    case 0: a = -x; break;   // eNudgeLeft
    case 1: a = y; break;    // eNudgeUp
    case 2: a = x; break;    // eNudgeRight
    default: a = -y; break;  // eNudgeDown
    }
    return a * menge;
}

void amRasterAusrichten(Karte& k, const AuswahlEinstellung& e, float raster, bool eckenSperre) {   // Selection_SnapToGrid
    k.undo.start();
    const GewaehltMenge g(k);
    if (e.modus == AuswahlModus::Komponente && komponentenAnzahl(k) != 0) {
        komponentenEinrasten(k, raster, eckenSperre);   // Scene_SnapToGrid_Component_Selected
    } else {
        for (const auto& ent : k.entitaeten()) {   // SnappableSnapToGridSelected (nur sichtbare)
            if (!ent->sichtbar()) { continue; }
            if (g.ganz.count(ent.get()) != 0 && !ent->istBehaelter()) {   // Punktentitaet: origin_snapped
                const Vektor3f o = eingerastet(entitaetUrsprung(*ent), raster);
                ent->setzen("origin", zahlText(o[0]) + ' ' + zahlText(o[1]) + ' ' + zahlText(o[2]));
            }
            for (const auto& p : ent->kinder()) {
                if (!p->sichtbar() || g.prim.count(p.get()) == 0) { continue; }
                if (p->art() == Primitiv::Art::Pinsel) {
                    static_cast<Pinsel&>(*p).eckenEinrasten(raster, k.kontext, eckenSperre);
                } else {
                    static_cast<Patch&>(*p).einrasten(raster);
                }
            }
        }
    }
    k.undo.finish("snapSelected -grid " + zahlText(raster));
}

// --- Shader (select.cpp, brushmanip.cpp, patchmanip.cpp) -----------------------------

namespace {

// Alle Pinsel der Karte (Scene_forEachBrush, auch versteckte) bzw. nur die
// gewaehlten (Scene_forEachSelectedBrush).
template <typename F>
void jederPinsel(Karte& k, const GewaehltMenge* nurGewaehlt, F f) {
    for (const auto& ent : k.entitaeten()) {
        for (const auto& p : ent->kinder()) {
            if (p->art() != Primitiv::Art::Pinsel) { continue; }
            if (nurGewaehlt != nullptr && nurGewaehlt->prim.count(p.get()) == 0) { continue; }
            f(std::static_pointer_cast<Pinsel>(p));
        }
    }
}

// Scene_forEachVisiblePatch / Scene_forEachVisibleSelectedPatch
template <typename F>
void jederSichtbarePatch(Karte& k, const GewaehltMenge* nurGewaehlt, F f) {
    for (const auto& ent : k.entitaeten()) {
        if (!sichtbar(*ent)) { continue; }
        for (const auto& p : ent->kinder()) {
            if (p->art() != Primitiv::Art::Patch || !sichtbar(*p)) { continue; }
            if (nurGewaehlt != nullptr && nurGewaehlt->prim.count(p.get()) == 0) { continue; }
            f(ent, p);
        }
    }
}

// g_SelectedFaceInstances (Scene_ForEachSelectedBrushFace)
template <typename F>
void jedeGewaehlteFlaeche(Karte& k, F f) {
    const auto liste = k.auswahl.flaechen();
    for (const auto& fw : liste) {
        const auto p = fw.pinsel.lock();
        const auto fl = fw.flaeche.lock();
        if (p && fl) { f(*fl); }
    }
}

// texdef_name_valid (shaderlib.h:68-70): shader_valid && shader_is_texture
bool texdefNameGueltig(const std::string& name) {
    if (!shaderGueltig(name) || name.size() < std::strlen(kTexturPraefix)) { return false; }
    return shaderGleich(name.substr(0, std::strlen(kTexturPraefix)), kTexturPraefix);
}

}  // namespace

bool texturNameGueltig(const std::string& name) { return texdefNameGueltig(name); }
bool shaderNamenGleich(const std::string& a, const std::string& b) { return shaderGleich(a, b); }

// Select_SetShader_Undo (select.cpp:497-502) mit Select_SetShader (:489-495)
bool shaderSetzen(Karte& k, const AuswahlEinstellung& e, const std::string& shader) {
    if (k.auswahl.flaechen().empty() && k.auswahl.anzahl() == 0) { return false; }
    k.undo.start();
    const GewaehltMenge g(k);
    if (e.modus != AuswahlModus::Komponente) {
        // Scene_BrushSetShader_Selected: FaceSetShader an jeder Flaeche
        jederPinsel(k, &g, [&](const std::shared_ptr<Pinsel>& p) {
            for (const auto& f : p->flaechen()) { f->setzeShader(shader, k.kontext); }
        });
        // Scene_PatchSetShader_Selected
        jederSichtbarePatch(k, &g, [&](const std::shared_ptr<Entitaet>&, const std::shared_ptr<Primitiv>& p) {
            static_cast<Patch&>(*p).setzeShader(shader);
        });
    }
    // Scene_BrushSetShader_Component_Selected
    jedeGewaehlteFlaeche(k, [&](Flaeche& f) { f.setzeShader(shader, k.kontext); });
    k.undo.finish("textureNameSetSelected");
    return true;
}

// Scene_BrushPatchSelectByShader (select.cpp:826-829): BrushSelectByShaderWalker
// (nur sichtbare) und Scene_PatchSelectByShader
void waehleNachShader(Karte& k, const std::string& shader) {
    for (const auto& ent : k.entitaeten()) {
        if (!sichtbar(*ent)) { continue; }
        for (const auto& p : ent->kinder()) {
            if (!sichtbar(*p) || p->art() != Primitiv::Art::Pinsel) { continue; }
            const auto& pinsel = static_cast<const Pinsel&>(*p);
            if (std::any_of(pinsel.flaechen().begin(), pinsel.flaechen().end(),
                            [&](const auto& f) { return shaderGleich(f->shader(), shader); })) {   // Brush_hasShader
                k.auswahl.waehlen(ent, p);
            }
        }
    }
    jederSichtbarePatch(k, nullptr, [&](const std::shared_ptr<Entitaet>& ent, const std::shared_ptr<Primitiv>& p) {
        if (shaderGleich(static_cast<const Patch&>(*p).shader(), shader)) { k.auswahl.waehlen(ent, p); }
    });
}

// Select_FacesAndPatchesByShader (select.cpp:889-892): Scene_BrushFacesSelectByShader
// (alle Pinsel) und Scene_PatchSelectByShader
void waehleFlaechenNachShader(Karte& k, const std::string& shader) {
    jederPinsel(k, nullptr, [&](const std::shared_ptr<Pinsel>& p) {
        for (const auto& f : p->flaechen()) {   // FaceSelectByShader
            if (shaderGleich(f->shader(), shader)) { k.auswahl.flaecheWaehlen(p, f); }
        }
    });
    jederSichtbarePatch(k, nullptr, [&](const std::shared_ptr<Entitaet>& ent, const std::shared_ptr<Primitiv>& p) {
        if (shaderGleich(static_cast<const Patch&>(*p).shader(), shader)) { k.auswahl.waehlen(ent, p); }
    });
}

// FindReplaceTextures (select.cpp:679-707)
bool texturenErsetzen(Karte& k, const AuswahlEinstellung& e, const std::string& suchen, const std::string& ersetzen,
                      bool nurAusgewaehlt, std::string* fehler) {
    for (const std::string* n : {&suchen, &ersetzen}) {
        if (!texdefNameGueltig(*n)) {
            if (fehler != nullptr) { *fehler = "FindReplaceTextures: invalid texture name: '" + *n + "', aborted"; }
            return false;
        }
    }
    k.undo.start();
    const bool nurSuchen = shaderGleich(ersetzen, kTexturPraefix);   // pReplace = 0: do search
    const GewaehltMenge g(k);
    // FaceSelectByShader / FaceFindReplaceShader (Face_FindReplaceShader)
    const auto flaeche = [&](const std::shared_ptr<Pinsel>& p, const std::shared_ptr<Flaeche>& f) {
        if (!shaderGleich(f->shader(), suchen)) { return; }
        if (nurSuchen) {
            k.auswahl.flaecheWaehlen(p, f);
        } else {
            f->setzeShader(ersetzen, k.kontext);
        }
    };
    if (nurAusgewaehlt) {
        if (e.modus != AuswahlModus::Komponente) {
            // Scene_BrushFindReplaceShader_Selected
            jederPinsel(k, &g, [&](const std::shared_ptr<Pinsel>& p) {
                for (const auto& f : p->flaechen()) { flaeche(p, f); }
            });
            // Scene_PatchFindReplaceShader_Selected: suchen tut nichts
            if (!nurSuchen) {
                jederSichtbarePatch(k, &g, [&](const std::shared_ptr<Entitaet>&, const std::shared_ptr<Primitiv>& p) {
                    if (shaderGleich(static_cast<const Patch&>(*p).shader(), suchen)) { static_cast<Patch&>(*p).setzeShader(ersetzen); }
                });
            }
        }
        // Scene_BrushFindReplaceShader_Component_Selected
        if (nurSuchen) {
            jederPinsel(k, &g, [&](const std::shared_ptr<Pinsel>& p) {
                for (const auto& f : p->flaechen()) { flaeche(p, f); }
            });
        } else {
            jedeGewaehlteFlaeche(k, [&](Flaeche& f) {
                if (shaderGleich(f.shader(), suchen)) { f.setzeShader(ersetzen, k.kontext); }
            });
        }
    } else {
        // Scene_BrushFindReplaceShader, Scene_PatchFindReplaceShader
        jederPinsel(k, nullptr, [&](const std::shared_ptr<Pinsel>& p) {
            for (const auto& f : p->flaechen()) { flaeche(p, f); }
        });
        jederSichtbarePatch(k, nullptr, [&](const std::shared_ptr<Entitaet>& ent, const std::shared_ptr<Primitiv>& p) {
            auto& patch = static_cast<Patch&>(*p);
            if (!shaderGleich(patch.shader(), suchen)) { return; }
            if (nurSuchen) {
                k.auswahl.waehlen(ent, p);   // PatchSelectByShader
            } else {
                patch.setzeShader(ersetzen);
            }
        });
    }
    k.undo.finish("textureFindReplace -find " + suchen + " -replace " + ersetzen);
    return true;
}

}  // namespace bhed::karte
