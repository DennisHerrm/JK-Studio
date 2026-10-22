// entitaetbefehle.cpp - Karten-Editor: Entitaeten erzeugen, umwandeln,
// verbinden, ihre Schluessel setzen und Entitaeten auswaehlen.
// Portiert aus NetRadiant-Custom 20260114 (die Quellen nennt entitaetbefehle.h).
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
#include "bhed/karte/entitaetbefehle.h"

#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <set>
#include <unordered_set>

namespace bhed::karte {

namespace {

// Instance_isSelected (ganz) und instance.childSelected()
struct Gewaehlt {
    std::unordered_set<const Entitaet*> ganz;
    std::unordered_set<const Primitiv*> prim;
    explicit Gewaehlt(const Karte& k) {
        for (const auto& el : k.auswahl.elemente()) {
            const auto e = el.entitaet.lock();
            const auto p = el.primitiv.lock();
            if (!e) { continue; }
            if (p) {
                prim.insert(p.get());
            } else {
                ganz.insert(e.get());
            }
        }
    }
    [[nodiscard]] bool kind(const Entitaet& e) const {
        return std::any_of(e.kinder().begin(), e.kinder().end(), [&](const auto& p) { return prim.count(p.get()) != 0U; });
    }
    [[nodiscard]] bool betroffen(const Entitaet& e) const { return ganz.count(&e) != 0U || kind(e); }
};

bool gleichOhneFall(const std::string& a, const std::string& b) {   // string_equal_nocase
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}

std::string vektorText(const Vektor3f& v) { return zahlText(v[0]) + ' ' + zahlText(v[1]) + ' ' + zahlText(v[2]); }

// instance.worldAABB() einer Gruppe (Instance::evaluateBounds, scenelib.h):
// die Grenzen der Kinder mit aabb_extend_by_aabb_safe; die Gruppe selbst hat
// keinen gueltigen localAABB.
Kasten kinderGrenzen(const Karte& k, const Entitaet& e) {
    Kasten b;
    for (const auto& p : e.kinder()) {
        if (p->art() == Primitiv::Art::Pinsel) {
            kastenErweiternSicher(b, static_cast<const Pinsel&>(*p).grenzen(k.kontext));
        } else {
            kastenErweiternSicher(b, static_cast<const Patch&>(*p).grenzen());
        }
    }
    return b;
}

// Brush_ConstructPlacehoderCuboid (brushmanip.cpp:844-848): ein Quader mit
// texdef_name_default; der neue BrushNode bekommt die aktuelle Ebene
// (brushnode.h:98, GlobalSceneGraph().currentLayer()).
void platzhalterQuader(Karte& k, const std::shared_ptr<Entitaet>& e, const Kasten& b) {
    auto p = std::make_shared<Pinsel>();
    p->schicht = static_cast<int>(k.aktuelleSchicht);
    e->kindEinfuegen(p);
    TexturProjektion proj;
    texdefStandard(k.kontext.art(), proj);   // TextureTransform_getDefault
    p->quaderBauen(b, kTexturPraefix, proj, k.kontext);
}

// alle Primitive von von nach nach (parentBrushes + Loeschen der Quelle)
void primitiveVerschieben(const std::shared_ptr<Entitaet>& von, const std::shared_ptr<Entitaet>& nach) {
    const Entitaet::Kinder kinder = von->kinder();
    for (const auto& p : kinder) {
        von->kindEntfernen(p.get());
        nach->kindEinfuegen(p);
    }
}

// Scene_EntitySelectByPropertyValues (EntityFindByPropertyValueWalker): sichtbare
// Entitaeten (nicht worldspawn), die passen, samt ihren Primitiven.
template <typename F>
void entitaetenWaehlen(Karte& k, F passt) {
    const auto welt = k.welt();
    for (const auto& ent : k.entitaeten()) {
        if (ent->versteckt || ent == welt || !passt(*ent)) { continue; }
        k.auswahl.waehlen(ent);
        for (const auto& p : ent->kinder()) {
            if (!p->versteckt) { k.auswahl.waehlen(ent, p); }
        }
    }
}

// Scene_EntityGetPropertyValues_nonEmpty
std::vector<std::string> gewaehlteWerte(const Karte& k, const std::string& schluessel) {
    std::vector<std::string> v;
    const Gewaehlt g(k);
    const auto welt = k.welt();
    for (const auto& ent : k.entitaeten()) {
        if (ent == welt || !g.betroffen(*ent)) { continue; }
        const std::string w = ent->wert(schluessel);
        if (!w.empty() && std::find(v.begin(), v.end(), w) == v.end()) { v.push_back(w); }
    }
    return v;
}

}  // namespace

// --- entityinspector.cpp --------------------------------------------------------------------

std::string GewaehlteSchluessel::wert(const std::string& schluessel) const {   // SelectedEntity_getValueForKey
    if (const auto it = werte.find(schluessel); it != werte.end()) { return it->second; }
    if (const auto it = vorgaben.find(schluessel); it != vorgaben.end()) { return it->second; }
    return {};
}

GewaehlteSchluessel gewaehlteSchluessel(const Karte& k) {   // Entity_GetKeyValues_Selected
    GewaehlteSchluessel g;
    std::set<const Entitaet*> besucht;
    for (const auto& el : k.auswahl.elemente()) {
        const auto e = el.entitaet.lock();
        if (!e || !besucht.insert(e.get()).second) { continue; }
        for (const auto& [s, w] : e->schluessel()) { g.werte.emplace(s, w); }   // map::insert: der erste gewinnt
        if (const EntitaetsKlasse* kl = e->klasse()) {
            for (const auto& [name, a] : kl->attribute) { g.vorgaben.emplace(name, a.wert); }
        }
    }
    return g;
}

void schluesselSetzenAuswahl(Karte& k, const std::string& schluessel, const std::string& wert) {   // EntitySetKeyValueSelected
    const Gewaehlt g(k);
    const auto liste = k.entitaeten();
    for (const auto& ent : liste) {
        if (g.betroffen(*ent)) { ent->setzen(schluessel, wert); }
    }
}

void schluesselSetzenRueckgaengig(Karte& k, const std::string& schluessel, const std::string& wert) {
    k.undo.start();
    schluesselSetzenAuswahl(k, schluessel, wert);
    k.undo.finish("entitySetKeyValue -key \"" + schluessel + "\" -value \"" + wert + "\"");
}

void schluesselLoeschenAuswahl(Karte& k, const std::string& schluessel) {   // EntityInspector_clearKeyValue
    if (schluessel == "classname") { return; }
    k.undo.start();
    schluesselSetzenAuswahl(k, schluessel, "");
    k.undo.finish("entityDeleteKey -key " + schluessel);
}

void alleSchluesselLoeschen(Karte& k, const GewaehlteSchluessel& g) {   // EntityInspector_clearAllKeyValues
    k.undo.start();
    for (const auto& [s, w] : g.werte) {
        (void)w;
        if (s != "classname" && s != "origin") { schluesselSetzenAuswahl(k, s, ""); }
    }
    k.undo.finish("entityClear");
}

void spawnflagsSetzen(Karte& k, int flags) {   // EntityInspector_applySpawnflags
    char wert[32] = {};
    if (flags != 0) { std::snprintf(wert, sizeof(wert), "%i", flags); }
    k.undo.start();
    schluesselSetzenAuswahl(k, "spawnflags", wert);
    k.undo.finish("entitySetSpawnflags -flags " + std::to_string(flags));
}

// --- entity.cpp --------------------------------------------------------------------------

std::vector<std::shared_ptr<Primitiv>> gewaehltePrimitiveUmhaengen(Karte& k, const std::shared_ptr<Entitaet>& ziel,
                                                                   const Entitaet* nurAus) {
    std::vector<std::shared_ptr<Primitiv>> umgehaengt;
    const Gewaehlt g(k);
    const auto welt = k.welt();
    const auto liste = k.entitaeten();
    for (const auto& ent : liste) {
        if (ent == ziel) { continue; }   // skip traverse of target node
        if (nurAus != nullptr && ent.get() != nurAus) { continue; }   // traverse_subgraph( ..., start )
        bool leer = false;
        const Entitaet::Kinder kinder = ent->kinder();
        for (const auto& p : kinder) {
            if (g.prim.count(p.get()) == 0U) { continue; }
            k.auswahl.abwaehlen(ent, p);
            ent->kindEntfernen(p.get());
            ziel->kindEinfuegen(p);
            umgehaengt.push_back(p);
            leer = ent->kinder().empty();
        }
        if (leer && ent != welt) {   // delete empty entity left
            k.auswahl.abwaehlen(ent);
            k.entitaetEntfernen(ent.get());
        }
    }
    k.auswahl.aufraeumen();
    return umgehaengt;
}

std::size_t gewaehltePrimitive(const Karte& k) {   // Scene_countSelectedBrushes
    std::size_t n = 0;
    for (const auto& el : k.auswahl.elemente()) {
        if (el.primitiv.lock()) { ++n; }
    }
    return n;
}

std::string gemeinsamerShader(const std::string& name) { return "textures/system/" + name; }   // GetCommonShader

void klasseSetzenAuswahl(Karte& k, const std::string& klassenname) {   // Scene_EntitySetClassname_Selected
    if (k.auswahl.anzahl() == 0) { return; }
    const bool zuWelt = klassenname == "worldspawn";
    const std::string befehl = zuWelt ? std::string("ungroupSelectedEntities") : "entitySetClass -class " + klassenname;
    k.undo.start();
    const auto welt = k.welt();
    const bool m2world = welt && zuWelt;
    const Gewaehlt g(k);
    const auto liste = k.entitaeten();
    for (const auto& ent : liste) {   // EntitySetClassnameSelected::post
        const bool kind = g.kind(*ent);
        if (!(kind || g.ganz.count(ent.get()) != 0U)) { continue; }
        if (ent == welt) {   // do not want to convert whole worldspawn entity
            if (kind && !m2world) {   // create an entity from world brushes instead
                const EntitaetsKlasse* kl = k.klasseFuer(klassenname, true);
                if (kl->fixedsize) { continue; }
                auto neu = k.neueEntitaet(klassenname, true);
                k.entitaetEinfuegen(neu);
                (void)gewaehltePrimitiveUmhaengen(k, neu, welt.get());   // Scene_parentSubgraphSelectedBrushesToEntity( ..., path )
                for (const auto& p : neu->kinder()) { k.auswahl.waehlen(neu, p); }   // Scene_forEachChildSelectable
            }
            continue;
        }
        if (m2world) {   // ungroupSelectedEntities
            if (ent->istBehaelter()) {   // node_is_group
                k.auswahl.abwaehlen(ent);
                for (const auto& p : ent->kinder()) { k.auswahl.abwaehlen(ent, p); }
                primitiveVerschieben(ent, welt);
                k.entitaetEntfernen(ent.get());
            }
            continue;
        }
        const EntitaetsKlasse* kl = k.klasseFuer(klassenname, ent->istBehaelter());
        auto neu = k.neueEntitaet(klassenname, ent->istBehaelter());
        if (ent->istBehaelter() && kl->fixedsize) {   // group entity to point one
            const Kasten b = kinderGrenzen(k, *ent);
            ent->setzen("origin", vektorText(b.origin));
        }
        k.auswahl.abwaehlen(ent);
        for (const auto& p : ent->kinder()) { k.auswahl.abwaehlen(ent, p); }
        if (ent->istBehaelter() && neu->istBehaelter()) { primitiveVerschieben(ent, neu); }   // group entity to group one
        k.entitaetEinfuegen(neu);
        for (const auto& [s, w] : ent->schluessel()) {   // EntityCopyingVisitor
            if (s != "classname") { neu->setzen(s, w); }
        }
        if (!ent->istBehaelter() && !kl->fixedsize) {   // point entity to group one
            Kasten b(Vektor3f(0, 0, 0), Vektor3f(16, 16, 16));
            float o[3] = {0, 0, 0};
            if (std::sscanf(ent->wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]) == 3) { b.origin = Vektor3f(o[0], o[1], o[2]); }
            platzhalterQuader(k, neu, b);
            neu->setzen("origin", "");
        }
        k.entitaetEntfernen(ent.get());
    }
    k.auswahl.aufraeumen();
    k.undo.finish(befehl);
}

ErzeugtErgebnis entitaetErzeugen(Karte& k, const std::string& name, const Vektor3f& ursprung) {
    ErzeugtErgebnis r;
    if (name == "worldspawn") {   // only process if worldspawn is present
        if (const auto welt = k.welt()) {
            k.undo.start();
            (void)gewaehltePrimitiveUmhaengen(k, welt);
            k.undo.finish("ungroupSelectedPrimitives");
            r.entitaet = welt;
            return r;
        }
    }
    r.befehl = "entityCreate -class " + name;
    k.undo.start();
    const EntitaetsKlasse* kl = k.klasseFuer(name, true);
    const bool modell = kl->miscmodelIs;
    const bool pinselGewaehlt = gewaehltePrimitive(k) != 0;
    // is important to have retexturing here; if doing in the end, undo doesn't succeed
    if (name.size() >= 8 && gleichOhneFall(name.substr(0, 8), "trigger_") && pinselGewaehlt && !kl->fixedsize) {
        const std::string trigger = gemeinsamerShader("trigger");
        const Gewaehlt g(k);
        for (const auto& ent : k.entitaeten()) {   // Scene_PatchSetShader_Selected, Scene_BrushSetShader_Selected
            for (const auto& p : ent->kinder()) {
                if (g.prim.count(p.get()) == 0U || p->art() != Primitiv::Art::Patch || ent->versteckt || p->versteckt) { continue; }
                static_cast<Patch&>(*p).setzeShader(trigger);
            }
        }
        for (const auto& ent : k.entitaeten()) {
            for (const auto& p : ent->kinder()) {
                if (g.prim.count(p.get()) == 0U || p->art() != Primitiv::Art::Pinsel) { continue; }
                for (const auto& f : static_cast<Pinsel&>(*p).flaechen()) { f->setzeShader(trigger, k.kontext); }
            }
        }
    }
    if (!(kl->fixedsize || modell) && !pinselGewaehlt) {
        r.fehler = "failed to create a group entity - no brushes are selected";
        k.undo.finish(r.befehl);
        return r;
    }
    auto neu = k.neueEntitaet(name, true);
    k.entitaetEinfuegen(neu);
    if (kl->fixedsize || (modell && !pinselGewaehlt)) {
        neu->setzen("origin", vektorText(ursprung));   // setTranslation + freezeTransform
        k.auswahl.leeren();
        k.auswahl.flaechenLeeren();
        k.auswahl.waehlen(neu);
    } else {
        (void)gewaehltePrimitiveUmhaengen(k, neu);
        for (const auto& p : neu->kinder()) { k.auswahl.waehlen(neu, p); }
    }
    r.entitaet = neu;
    r.lichtFragen = gleichOhneFall(name, "light");   // DoLightIntensityDlg (nicht doom3)
    r.modellFragen = modell && kl->wertFuer(kl->miscmodelKey).empty();   // misc_model_dialog
    if (r.lichtFragen || r.modellFragen) {
        r.offen = true;
    } else {
        k.undo.finish(r.befehl);
    }
    return r;
}

void befehlAbschliessen(Karte& k, ErzeugtErgebnis& e) {
    if (!e.offen) { return; }
    k.undo.finish(e.befehl);
    e.offen = false;
}

ErzeugtErgebnis primitiveZurWelt(Karte& k) { return entitaetErzeugen(k, "worldspawn", Vektor3f(0, 0, 0)); }   // Entity_ungroupSelectedPrimitives

bool primitiveZurEntitaet(Karte& k, bool zurLetzten, std::string* fehler) {   // Entity_moveSelectedPrimitives
    if (k.auswahl.anzahl() < 2) {
        if (fehler != nullptr) { *fehler = "Source and target entity primitives should be selected!"; }
        return false;
    }
    const AuswahlElement& el = zurLetzten ? k.auswahl.elemente().back() : k.auswahl.elemente().front();
    const auto ent = el.entitaet.lock();
    if (!ent || !ent->istBehaelter()) { return false; }   // Node_isEntity && node_is_group
    k.undo.start();
    (void)gewaehltePrimitiveUmhaengen(k, ent);
    k.undo.finish("movePrimitivesToEntity \"" + ent->klassenname() + "\"");
    return true;
}

bool entitaetenVerbinden(Karte& k, int index, std::string* fehler) {   // Entity_connectSelected
    if (k.auswahl.anzahl() != 2) {
        if (fehler != nullptr) {
            *fehler = index == 1 ? "entityKillConnectSelected: exactly two instances must be selected"
                                 : "entityConnectSelected: exactly two instances must be selected";
        }
        return false;
    }
    const auto e1 = k.auswahl.elemente()[0].entitaet.lock();   // penultimateSelected (ScenePath_getEntity)
    const auto e2 = k.auswahl.elemente()[1].entitaet.lock();   // ultimateSelected
    if (!e1 || !e2) {
        if (fehler != nullptr) { *fehler = "entityConnectSelected: both of the selected instances must be an entity"; }
        return false;
    }
    if (e1 == e2) {
        if (fehler != nullptr) { *fehler = "entityConnectSelected: the selected instances must not both be from the same entity"; }
        return false;
    }
    k.undo.start();
    const std::string zielSchluessel = index == 1 ? "killtarget" : "target";   // ConnectEntities
    // prioritize existing target key: intent is to most probably not break existing connections
    std::string wert = e1->wert(zielSchluessel);
    if (wert.empty()) { wert = e2->wert("targetname"); }
    if (wert.empty()) {   // GlobalNamespace().makeUnique( "<classname>1" )
        std::string typ = e2->klassenname();
        if (typ.empty()) { typ = "t"; }
        EindeutigeNamen namen;
        for (const auto& ent : k.entitaeten()) {
            for (const auto& [s, w] : ent->schluessel()) {
                if (schluesselIstName(s) && !w.empty()) { namen.einfuegen(nameLesen(w)); }
            }
        }
        wert = nameSchreiben(namen.eindeutig(nameLesen(typ + "1")));
    }
    e1->setzen(zielSchluessel, wert);
    e2->setzen("targetname", wert);
    k.undo.finish("entityConnectSelected");
    return true;
}

std::shared_ptr<Entitaet> letzteGewaehlteEntitaet(const Karte& k) {
    if (k.auswahl.anzahl() == 0) { return nullptr; }
    return k.auswahl.elemente().back().entitaet.lock();
}

Vektor3f farbeNormiert(Vektor3f c) {   // NormalizeColor
    const float m = std::max({c[0], c[1], c[2]});
    if (m == 0) { return Vektor3f(1, 1, 1); }
    return Vektor3f(c[0] / m, c[1] / m, c[2] / m);
}

void farbeSetzenAuswahl(Karte& k, const Vektor3f& farbe) {   // Entity_setColour (nach color_dialog)
    char puffer[128];
    std::snprintf(puffer, sizeof(puffer), "%g %g %g", static_cast<double>(farbe[0]), static_cast<double>(farbe[1]),
                  static_cast<double>(farbe[2]));
    k.undo.start();
    schluesselSetzenAuswahl(k, "_color", puffer);
    k.undo.finish(std::string("entitySetColour ") + puffer);
}

bool farbeNormierenAuswahl(Karte& k, Vektor3f* neueFarbe) {   // Entity_normalizeColor
    const auto ent = letzteGewaehlteEntitaet(k);
    if (!ent) { return false; }
    const std::string s = ent->wert("_color");
    float c[3] = {0, 0, 0};
    if (s.empty() || std::sscanf(s.c_str(), "%f %f %f", &c[0], &c[1], &c[2]) != 3) { return false; }
    const Vektor3f n = farbeNormiert(Vektor3f(c[0], c[1], c[2]));
    char puffer[128];
    std::snprintf(puffer, sizeof(puffer), "%g %g %g", static_cast<double>(n[0]), static_cast<double>(n[1]), static_cast<double>(n[2]));
    k.undo.start();
    schluesselSetzenAuswahl(k, "_color", puffer);
    k.undo.finish(std::string("entityNormalizeColour ") + puffer);
    if (neueFarbe != nullptr) { *neueFarbe = n; }
    return true;
}

// --- select.cpp --------------------------------------------------------------------------

void waehleNachSchluesselWert(Karte& k, const char* schluessel, const char* wert) {   // Select_EntitiesByKeyValue
    k.auswahl.leeren();
    if (schluessel != nullptr && wert != nullptr) {
        if (*schluessel != 0 && *wert != 0) {
            entitaetenWaehlen(k, [&](const Entitaet& e) { return gleichOhneFall(e.wert(schluessel), wert); });
        }
    } else if (schluessel != nullptr) {
        if (*schluessel != 0) {
            entitaetenWaehlen(k, [&](const Entitaet& e) { return e.hat(schluessel); });
        }
    } else if (wert != nullptr && *wert != 0) {
        entitaetenWaehlen(k, [&](const Entitaet& e) {
            return std::any_of(e.schluessel().begin(), e.schluessel().end(), [&](const auto& kv) { return gleichOhneFall(wert, kv.second); });
        });
    }
}

bool waehleVerbundene(Karte& k, bool zielende, bool ziele, std::string* fehler) {   // Select_ConnectedEntities
    std::vector<std::string> zielWerte;
    std::vector<std::string> namenWerte;
    if (zielende) { namenWerte = gewaehlteWerte(k, "targetname"); }
    if (ziele) { zielWerte = gewaehlteWerte(k, "target"); }
    if (zielWerte.empty() && namenWerte.empty()) {
        if (fehler != nullptr) { *fehler = "SelectConnectedEntities: nothing found"; }
        return false;
    }
    if (!zielende || !ziele) { k.auswahl.leeren(); }
    const auto enthaelt = [](const std::vector<std::string>& v, const std::string& s) { return std::find(v.begin(), v.end(), s) != v.end(); };
    if (zielende && !namenWerte.empty()) {
        entitaetenWaehlen(k, [&](const Entitaet& e) { return enthaelt(namenWerte, e.wert("target")); });
    }
    if (ziele && !zielWerte.empty()) {
        entitaetenWaehlen(k, [&](const Entitaet& e) { return enthaelt(zielWerte, e.wert("targetname")); });
    }
    return true;
}

void auswahlAufEntitaetenErweitern(Karte& k) {   // ExpandSelectionToEntitiesWalker
    const Gewaehlt g(k);
    const auto welt = k.welt();
    for (const auto& ent : k.entitaeten()) {
        if (ent->versteckt || !g.betroffen(*ent)) { continue; }
        if (ent != welt) { k.auswahl.waehlen(ent); }   // avoid selecting world node
        if (!ent->istBehaelter()) { continue; }
        for (const auto& p : ent->kinder()) {
            if (!p->versteckt) { k.auswahl.waehlen(ent, p); }
        }
    }
}

// --- xywindow.cpp ------------------------------------------------------------------------

// EntityClassMenuInserter (xywindow.cpp:806-864)
std::vector<EntitaetMenuePunkt> entitaetMenue(const std::vector<std::string>& klassen) {
    std::vector<EntitaetMenuePunkt> wurzel;
    std::string eltern;          // m_stack.back().second (leer: Wurzel)
    bool imUntermenue = false;   // m_stack.size() == 2
    const auto hinzufuegen = [&](const std::string& name, const std::string& naechste) {   // addItem
        const std::size_t unterstrich = name.find('_');
        if (unterstrich != std::string::npos && unterstrich != 0) {
            const bool naechsteGleich = naechste.compare(0, unterstrich + 1, name, 0, unterstrich + 1) == 0;   // string_equal_n
            const bool kind = !eltern.empty() && eltern.size() == unterstrich && name.compare(0, unterstrich, eltern) == 0;
            if (kind) {   // this is a child
            } else if (naechsteGleich) {
                if (imUntermenue) {   // popMenu
                    imUntermenue = false;
                    eltern.clear();
                }
                wurzel.push_back(EntitaetMenuePunkt{name.substr(0, unterstrich), {}});   // pushMenu
                eltern = name.substr(0, unterstrich);
                imUntermenue = true;
            } else if (imUntermenue) {
                imUntermenue = false;
                eltern.clear();
            }
        } else if (imUntermenue) {
            imUntermenue = false;
            eltern.clear();
        }
        if (imUntermenue) {
            wurzel.back().kinder.push_back(name);
        } else {
            wurzel.push_back(EntitaetMenuePunkt{name, {}});
        }
    };
    for (std::size_t i = 0; i < klassen.size(); ++i) {   // visit: der vorige mit dem naechsten
        hinzufuegen(klassen[i], i + 1 < klassen.size() ? klassen[i + 1] : std::string());
    }
    return wurzel;
}

}  // namespace bhed::karte
