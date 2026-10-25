// auswahlsystem.h - Karten-Editor: Auswaehlen in den Ansichten.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/selection.cpp
// (RadiantSelectionSystem: SelectPoint, SelectPoint_InitPaint, SelectArea,
// SelectionPool_Select, Scene_TestSelect, testselect_entity_visible,
// testselect_primitive_visible, testselect_component_visible, Scene_Intersect,
// getBoundsSelected/Instance_getPivotBounds), radiant/selection_selector.h
// (SelectionPool, BooleanSelector, BestSelector, BestPointSelector),
// radiant/brush.h (BrushInstance::testSelect, FaceInstance::testSelect /
// testSelect_centroid, Winding_testSelect), radiant/patch.h
// (Patch::testSelect, PatchInstance::testSelect), libs/entitylib.h
// (aabb_testselect), plugins/entity/light.cpp (light_testselect).
//
//   Copyright (C) 2001-2006, William Joseph.
//   All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_KARTE_AUSWAHLSYSTEM_H
#define BHED_KARTE_AUSWAHLSYSTEM_H

#include "bhed/karte/auswahltest.h"

#include <list>
#include <map>
#include <memory>
#include <vector>

namespace bhed::karte {

class Karte;
class Entitaet;
class Primitiv;
class Pinsel;
class Flaeche;
class Patch;
struct Kontext;

// Ein Waehlbares (Selectable): ein Primitiv (Pinsel, Patch), eine Entitaet
// (Punktentitaet), eine Pinselflaeche (FaceInstance), eine Ecke oder Kante
// eines Pinsels (VertexInstance, EdgeInstance) oder ein Kontrollpunkt eines
// Patches (PatchControlInstance).
struct Waehlbar {
    enum class Art { Primitiv, Entitaet, Flaeche, Ecke, Kante, PatchPunkt };
    Art art = Art::Primitiv;
    std::shared_ptr<Entitaet> entitaet;
    std::shared_ptr<Primitiv> primitiv;    // Primitiv; bei Flaeche/Ecke/Kante der Pinsel, bei PatchPunkt der Patch
    std::shared_ptr<Flaeche> flaeche;
    std::size_t index = 0;                 // Ecke/Kante: Index in Pinsel::ecken()/kanten(); PatchPunkt: Kontrollpunkt
};
[[nodiscard]] bool istGewaehlt(const Karte& k, const Waehlbar& w);   // Selectable::isSelected
void setzeGewaehlt(Karte& k, const Waehlbar& w, bool an);           // Selectable::setSelected

// Selector (selection_.h)
class Waehler {
public:
    virtual ~Waehler() = default;
    virtual void schieben(const Waehlbar& w) = 0;   // pushSelectable
    virtual void holen() = 0;                       // popSelectable
    virtual void trefferDazu(const Treffer& t) = 0; // addIntersection
};

// SelectionPool: Treffer -> Waehlbar, sortiert (multimap, gleiche in
// Einfuegereihenfolge).
class AuswahlPool final : public Waehler {
public:
    explicit AuswahlPool(const Karte& k) : m_karte(k) {}
    void schieben(const Waehlbar& w) override;
    void holen() override;
    void trefferDazu(const Treffer& t) override { naeherZuweisen(m_treffer, t); }
    void dazu(const Treffer& t, const Waehlbar& w);   // addSelectable
    using Menge = std::multimap<Treffer, Waehlbar>;
    [[nodiscard]] Menge& menge() { return m_pool; }
    [[nodiscard]] bool leer() const { return m_pool.empty(); }   // failed

private:
    const Karte& m_karte;
    Menge m_pool;
    Treffer m_treffer;
    Waehlbar m_aktuell;
};

// BooleanSelector: nur ausgewaehlte zaehlen.
class JaNeinWaehler final : public Waehler {
public:
    explicit JaNeinWaehler(const Karte& k) : m_karte(k) {}
    void schieben(const Waehlbar& w) override { m_aktuell = w; }
    void holen() override {}
    void trefferDazu(const Treffer& t) override;
    [[nodiscard]] bool getroffen() const { return m_bester.gueltig(); }   // isSelected
    [[nodiscard]] const Treffer& bester() const { return m_bester; }

private:
    const Karte& m_karte;
    Waehlbar m_aktuell;
    Treffer m_bester;
};

// BestSelector___<DEPTH_EPSILON>: die besten (gleich weit) Waehlbaren.
class BesterWaehler final : public Waehler {
public:
    explicit BesterWaehler(float tiefeEps = 2e-6F) : m_tiefeEps(tiefeEps) {}
    void schieben(const Waehlbar& w) override {
        m_treffer = Treffer();
        m_aktuell = w;
    }
    void holen() override;
    void trefferDazu(const Treffer& t) override { naeherZuweisen(m_treffer, t); }
    [[nodiscard]] const std::list<Waehlbar>& beste() const { return m_beste; }
    [[nodiscard]] const Treffer& besterTreffer() const { return m_bester; }

private:
    float m_tiefeEps;
    Treffer m_treffer;
    Waehlbar m_aktuell;
    Treffer m_bester;
    std::list<Waehlbar> m_beste;
};

// BestPointSelector: nur der naechste Treffer.
class BesterPunktWaehler final : public Waehler {
public:
    void schieben(const Waehlbar&) override {}
    void holen() override {}
    void trefferDazu(const Treffer& t) override { naeherZuweisen(m_bester, t); }
    [[nodiscard]] bool getroffen() const { return m_bester.gueltig(); }
    [[nodiscard]] const Treffer& bester() const { return m_bester; }

private:
    Treffer m_bester;
};

// SelectionSystem::EMode / EComponentMode
enum class AuswahlModus { Entitaet, Primitiv, Komponente };
enum class KomponentenModus { Standard, Ecke, Kante, Flaeche };

// --- Szenentests -----------------------------------------------------------------

// Scene_TestSelect (Entitaet: nur Punktentitaeten; Primitiv: alles;
// Komponente: Komponenten ausgewaehlter Objekte).
void szeneTesten(const Karte& k, Waehler& w, AuswahlVolumen& test, AuswahlModus modus, KomponentenModus komponente);
void szeneTestenPrimitive(const Karte& k, Waehler& w, AuswahlVolumen& test);   // Scene_TestSelect_Primitive
// Scene_TestSelect_Component (alle sichtbaren) / _Selected (nur ausgewaehlte)
void szeneTestenKomponenten(const Karte& k, Waehler& w, AuswahlVolumen& test, KomponentenModus komponente, bool nurGewaehlte);

// --- RadiantSelectionSystem (Auswahl-Teil) ------------------------------------------

enum class AuswahlArt { Manipulator, Ersetzen, Durchlaufen, Auswaehlen, Abwaehlen };   // EModifier

struct AuswahlEinstellung {
    AuswahlModus modus = AuswahlModus::Primitiv;
    KomponentenModus komponente = KomponentenModus::Standard;
    bool punktEntitaetenIn2d = true;   // m_bPreferPointEntsIn2D
};

// SelectPoint: alt = g_modifiers == c_modifierAlt.
void punktWaehlen(Karte& k, const AuswahlEinstellung& e, const Blick& blick, const GeraeteVektor& punkt,
                  const GeraeteVektor& eps, AuswahlArt art, bool flaeche, bool alt);
// SelectPoint_InitPaint
[[nodiscard]] AuswahlArt malenBeginnen(Karte& k, const AuswahlEinstellung& e, const Blick& blick, const GeraeteVektor& punkt,
                                       const GeraeteVektor& eps, bool flaeche, bool alt);
// SelectArea
void bereichWaehlen(Karte& k, const AuswahlEinstellung& e, const Blick& blick, const AuswahlRechteck& rechteck, bool flaeche);
// deselectAll / deselectComponentsOrAll
void alleAbwaehlen(Karte& k, const AuswahlEinstellung& e);
void komponentenOderAlleAbwaehlen(Karte& k, const AuswahlEinstellung& e, bool komponenten);

// Scene_Intersect: Treffer unter dem Punkt (sonst 256 vor der Nahebene).
[[nodiscard]] Vektor3f szeneSchnitt(const Karte& k, const Blick& blick, const GeraeteVektor& punkt, const GeraeteVektor& eps);

// getBoundsSelected (bounds_selected: Punktentitaeten mit ihrem Ursprung)
[[nodiscard]] Kasten auswahlGrenzenPivot(const Karte& k);
// Scene_BoundsSelected_withEntityBounds
[[nodiscard]] Kasten auswahlGrenzenMitEntitaeten(const Karte& k);
// GetMapBounds (sunplug.cpp): die worldAABB der Wurzel - alle Entitaeten samt Kindern
[[nodiscard]] Kasten kartenGrenzen(const Karte& k);
// Die Weltlage einer Punktentitaet (localToWorld = Ursprung).
[[nodiscard]] Vektor3f entitaetUrsprung(const Entitaet& e);
// Der lokale Kasten einer Punktentitaet (read_aabb / m_aabb_light).
[[nodiscard]] Kasten entitaetKastenLokal(const Entitaet& e);
[[nodiscard]] bool istLichtEntitaet(const Entitaet& e);   // light / lightJunior

// Einzelne Treffertests (testSelect der Instanzen), fuer die Manipulatoren.
void testeEntitaet(const Entitaet& e, const Kontext& k, AuswahlVolumen& test, Treffer& bester);   // aabb_testselect / light_testselect / Modell
void testePinsel(const Pinsel& p, const Kontext& k, AuswahlVolumen& test, Treffer& bester);   // BrushInstance::testSelect
void testePatch(const Patch& p, AuswahlVolumen& test, Treffer& bester);                       // PatchInstance::testSelect
void testeFlaeche(const Flaeche& f, AuswahlVolumen& test, Treffer& bester);                   // Face::testSelect (ohne BeginMesh)

}  // namespace bhed::karte

#endif
