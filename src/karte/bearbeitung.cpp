// bearbeitung.cpp - Karten-Editor: Ausgewaehltes verschieben, drehen,
// skalieren, verzerren, in der Groesse aendern und neue Pinsel aufziehen.
// Portiert aus NetRadiant-Custom 20260114 (Quellen in bearbeitung.h).
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
#include "bhed/karte/bearbeitung.h"

#include "bhed/karte/eklasse.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"
#include "bhed/karte/xyansicht.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace bhed::karte {

std::string zahlText(float f) {
    char puffer[16];   // ostream_write( double ): char buf[16], "%g"
    std::snprintf(puffer, sizeof(puffer), "%g", static_cast<double>(f));
    return puffer;
}

namespace {

constexpr float kRadius = 64;   // g_radius (selection_.h:243)

// --- Schnittstellen (selection_.h:82-110) -------------------------------------------

class Verschiebbar {   // Translatable
public:
    virtual ~Verschiebbar() = default;
    virtual void verschieben(const Vektor3f& v) = 0;
};
class Drehbar {   // Rotatable
public:
    virtual ~Drehbar() = default;
    virtual void drehen(const Quaternion& q) = 0;
};
class Skalierbar {   // Scalable
public:
    virtual ~Skalierbar() = default;
    virtual void skalieren(const Vektor3f& s) = 0;
};
class Verzerrbar {   // Skewable
public:
    virtual ~Verzerrbar() = default;
    virtual void verzerren(const Verzerrung& v) = 0;
};
class AllesTransformierbar {   // AllTransformable
public:
    virtual ~AllesTransformierbar() = default;
    virtual void allesTransformieren(const Transformationen& t, const Vektor3f& weltPivot) = 0;
};
class UrsprungVerschiebbar {   // TransformOriginTranslatable
public:
    virtual ~UrsprungVerschiebbar() = default;
    virtual void ursprungVerschieben(const Vektor3f& v, const bool gesetzt[3]) = 0;
};

// testSelect_unselected_scene_point_return_t
struct SzenenPunkt {
    Vektor3d punkt;
    std::optional<Ebene3d> ebene;
};

// Was NRCs Manipulierbare aus Globalen lesen (Manipulatable::m_view,
// m_device_point, m_device_epsilon, g_modifiers, GetGridSize, ...).
class Dienste {
public:
    virtual ~Dienste() = default;
    const Blick* blick = nullptr;
    GeraeteVektor geraetPunkt;   // m_device_point
    GeraeteVektor geraetEps;     // m_device_epsilon
    [[nodiscard]] virtual const Zusatz& zusatzTasten() const = 0;
    [[nodiscard]] virtual float rasterGroesse() const = 0;   // GetGridSize
    [[nodiscard]] virtual float fangGroesse() const = 0;     // GetSnapGridSize
    [[nodiscard]] virtual bool primitivModus() const = 0;    // Mode() == ePrimitive
    [[nodiscard]] virtual float waehlEpsilon() const = 0;    // g_SELECT_EPSILON
    [[nodiscard]] virtual Kasten grenzenMitEntitaeten() const = 0;   // Scene_BoundsSelected_withEntityBounds
    [[nodiscard]] virtual std::optional<SzenenPunkt> ungewaehlterSzenenPunkt(const Blick& v, const GeraeteVektor& p,
                                                                             const GeraeteVektor& e) const = 0;
    virtual void quader(std::shared_ptr<Pinsel>& pinsel, const Kasten& k) = 0;   // Scene_BrushResize_Cuboid
};

// --- Manipulatable (selection_.h:34-48) -----------------------------------------------

class Manipulierbar {
public:
    explicit Manipulierbar(Dienste& d) : m_d(d) {}
    virtual ~Manipulierbar() = default;
    Manipulierbar(const Manipulierbar&) = delete;
    Manipulierbar& operator=(const Manipulierbar&) = delete;
    // Construct( device2manip, device_point, bounds, transform_origin )
    virtual void bauen(const Matrix4& geraetZuManip, const GeraeteVektor& punkt, const Kasten& grenzen, const Vektor3f& ursprung) = 0;
    // Transform( manip2object, device2manip, device_point )
    virtual void transformieren(const Matrix4& manipZuObjekt, const Matrix4& geraetZuManip, const GeraeteVektor& punkt) = 0;

protected:
    [[nodiscard]] const Blick& blick() const { return *m_d.blick; }
    [[nodiscard]] const Zusatz& zusatz() const { return m_d.zusatzTasten(); }
    [[nodiscard]] float fang() const { return m_d.fangGroesse(); }
    Dienste& m_d;
};

// translation_local2object fuer Vektor3d (Rechnung in float wie NRC)
Vektor3f komponentenMal(const Vektor3f& a, const Vektor3f& b) { return Vektor3f(a.x() * b.x(), a.y() * b.y(), a.z() * b.z()); }

// --- selection_mtable_translate.h ------------------------------------------------------

class VerschiebeAchse final : public Manipulierbar {   // TranslateAxis
public:
    VerschiebeAchse(Dienste& d, Verschiebbar& ziel) : Manipulierbar(d), m_ziel(ziel) {}
    void bauen(const Matrix4& g, const GeraeteVektor& p, const Kasten& b, const Vektor3f&) override {
        m_start = punktAufAchse(m_achse, g, p);
        m_grenzen = b;
    }
    void transformieren(const Matrix4& m2o, const Matrix4& g, const GeraeteVektor& p) override {
        Vektor3f aktuell = punktAufAchse(m_achse, g, p);
        aktuell = m_achse * abstandFuerAchse(m_start, aktuell, m_achse);
        aktuell = translationLokalZuObjekt(aktuell, m2o);
        if (zusatz().strg) {
            kastenVerschiebungEinrasten(aktuell, m_grenzen, fang());
        } else {
            einrasten(aktuell, fang());
        }
        m_ziel.verschieben(aktuell);
    }
    void setzeAchse(const Vektor3f& a) { m_achse = a; }

private:
    Vektor3f m_start;
    Vektor3f m_achse;
    Verschiebbar& m_ziel;
    Kasten m_grenzen;
};

class VerschiebeAchse2 final : public Manipulierbar {   // TranslateAxis2
public:
    VerschiebeAchse2(Dienste& d, Verschiebbar& ziel) : Manipulierbar(d), m_ziel(ziel) {}
    void bauen(const Matrix4&, const GeraeteVektor& p, const Kasten& b, const Vektor3f&) override {
        m_achseZ = groessteBetragsKomponente(m_ebeneGewaehlt.normal());
        Vektor3f xy = blick().betrachter() - m_0;
        xy[m_achseZ] = 0;
        normieren(xy);
        m_ebeneZ = Ebene3d(xy, punkt(xy, m_0));
        m_startZ = punktAufEbeneSicht(m_ebeneZ, blick().sicht(), p);
        m_grenzen = b;
    }
    void transformieren(const Matrix4&, const Matrix4&, const GeraeteVektor& p) override {
        Vektor3f aktuell = kAchsen[m_achseZ] * punkt(m_ebeneGewaehlt.normal(), punktAufEbeneSicht(m_ebeneZ, blick().sicht(), p) - m_startZ);
        aktuell = aktuell * (m_ebeneGewaehlt.normal()[m_achseZ] >= 0 ? 1 : -1);
        // INF abfangen (oben am Kasten in 2D wahrscheinlich)
        if (!std::isfinite(aktuell[0]) || !std::isfinite(aktuell[1]) || !std::isfinite(aktuell[2])) { return; }
        if (zusatz().strg) {
            kastenVerschiebungEinrasten(aktuell, m_grenzen, fang());
        } else {
            einrasten(aktuell, fang());
        }
        m_ziel.verschieben(aktuell);
    }
    void setze0(const Vektor3f& start, const Ebene3d& ebene) {
        m_0 = start;
        m_ebeneGewaehlt = ebene;
    }

private:
    Vektor3f m_0;
    Ebene3d m_ebeneGewaehlt;
    std::size_t m_achseZ = 2;
    Ebene3d m_ebeneZ;
    Vektor3f m_startZ;
    Verschiebbar& m_ziel;
    Kasten m_grenzen;
};

class VerschiebeFrei final : public Manipulierbar {   // TranslateFree
public:
    VerschiebeFrei(Dienste& d, Verschiebbar& ziel) : Manipulierbar(d), m_ziel(ziel) {}
    void bauen(const Matrix4& g, const GeraeteVektor& p, const Kasten& b, const Vektor3f&) override {
        m_start = punktAufEbeneGeraet(g, p);
        m_grenzen = b;
    }
    void transformieren(const Matrix4& m2o, const Matrix4& g, const GeraeteVektor& p) override {
        Vektor3f aktuell = punktAufEbeneGeraet(g, p) - m_start;
        if (zusatz().shift) { aktuell = komponentenMal(aktuell, kAchsen[groessteBetragsKomponente(aktuell)]); }   // auf eine Achse
        aktuell = translationLokalZuObjekt(aktuell, m2o);
        if (zusatz().strg) {
            kastenVerschiebungEinrasten(aktuell, m_grenzen, fang());
        } else {
            einrasten(aktuell, fang());
        }
        m_ziel.verschieben(aktuell);
    }

private:
    Vektor3f m_start;
    Verschiebbar& m_ziel;
    Kasten m_grenzen;
};

// AABB_TestPoint (selection_mtable_translate.h:186-197)
std::optional<Vektor3f> kastenTestPunkt(const Blick& v, const GeraeteVektor& p, const GeraeteVektor& e, const Kasten& k) {
    Blick geschert(v);
    auswahlTestBauen(geschert, auswahlKastenPunkt(p, e));
    Treffer bester;
    kastenBesterPunkt(geschert.sicht(), Wegschneiden::CW, k, bester);
    if (bester.gueltig()) {
        return projiziert(transformierterVektor4(vollInvers(geschert.sicht()), Vektor4f(0, 0, bester.tiefe(), 1)));
    }
    return std::nullopt;
}

class KastenFang final : public Manipulierbar {   // SnapBounds
public:
    KastenFang(Dienste& d, Verschiebbar& ziel, AllesTransformierbar& alles) : Manipulierbar(d), m_ziel(ziel), m_alles(alles) {}
    void bauen(const Matrix4&, const GeraeteVektor& p, const Kasten& b, const Vektor3f&) override {
        m_grenzen = m_d.primitivModus() ? m_d.grenzenMitEntitaeten() : b;
        // fuer das Dreh-Einrasten die dem Klick gegenueberliegende Kastenseite
        if (const auto pkt = kastenTestPunkt(blick(), p, m_d.geraetEps, m_grenzen)) { m_0 = *pkt; }
        m_drehAchse = 0;
        m_drehVorzeichen = 1;
        float besterAbstand = FLT_MAX;
        for (const std::size_t achse : {std::size_t{0}, std::size_t{1}, std::size_t{2}}) {
            for (const int vz : {-1, 1}) {
                const float abstand = std::fabs(m_0[achse] - (m_grenzen.origin[achse] + std::copysign(m_grenzen.extents[achse], static_cast<float>(vz))));
                if (abstand < besterAbstand) {
                    besterAbstand = abstand;
                    m_drehAchse = achse;
                    m_drehVorzeichen = vz;
                }
            }
        }
        m_entlang.reset();
    }
    void transformieren(const Matrix4&, const Matrix4&, const GeraeteVektor& p) override {
        Vektor3f aktuell(0, 0, 0);
        if (zusatz().shift) {   // entlang einer Ebene
            if (!m_entlang) {   // zuerst an der Ausgangslage des Zeigers versuchen
                if (const auto t = m_d.ungewaehlterSzenenPunkt(blick(), m_d.geraetPunkt, m_d.geraetEps); t && t->ebene) {
                    m_entlang = t->ebene;
                    m_entlangStart = punktAufEbeneSicht(*m_entlang, blick().sicht(), m_d.geraetPunkt);
                } else if (const auto t2 = m_d.ungewaehlterSzenenPunkt(blick(), p, m_d.geraetEps); t2 && t2->ebene) {
                    m_entlang = t2->ebene;
                    m_entlangStart = punktAufEbeneSicht(*m_entlang, blick().sicht(), p);
                }
            }
            if (m_entlang) {
                aktuell = punktAufEbeneSicht(*m_entlang, blick().sicht(), p) - m_entlangStart;
                const Vektor3d& n = m_entlang->normal();
                const std::size_t maxi = groessteBetragsKomponente(n);
                einrasten(aktuell, fang());
                // die dritte Komponente so, dass normal . verschiebung = 0
                aktuell[maxi] = static_cast<float>(-(n[(maxi + 1) % 3] * aktuell[(maxi + 1) % 3] + n[(maxi + 2) % 3] * aktuell[(maxi + 2) % 3]) / n[maxi]);
                m_ziel.verschieben(aktuell);
                return;
            }
        } else if (const auto t = m_d.ungewaehlterSzenenPunkt(blick(), p, m_d.geraetEps)) {
            const auto ecke = [](const Kasten& b, std::size_t achse, const Vektor3f& nrm, const Vektor3f& strahl) {
                Vektor3f ausdehnung = b.extents;
                ausdehnung[achse] = std::copysign(ausdehnung[achse], nrm[achse]);
                ausdehnung[(achse + 1) % 3] = std::copysign(ausdehnung[(achse + 1) % 3], strahl[(achse + 1) % 3]);
                ausdehnung[(achse + 2) % 3] = std::copysign(ausdehnung[(achse + 2) % 3], strahl[(achse + 2) % 3]);
                return b.origin - ausdehnung;
            };
            const Strahl3 strahl = strahlFuerGeraetePunkt(vollInvers(blick().sicht()), p);
            const Vektor3f nrm = t->ebene ? Vektor3f(t->ebene->normal()) : -strahl.richtung;
            if (zusatz().alt) {   // Dreh-Einrasten
                const Quaternion drehung = quaternionFuerKasten(kAchsen[m_drehAchse] * static_cast<float>(m_drehVorzeichen), nrm);
                const Matrix4 zurueck = drehungAusQuaternion(quaternionInvers(drehung));
                // ohne Ebene wird genau auf den Strahl gedreht ... Strahl leicht
                // verstellen, um eine eindeutige Ecke zu finden
                const Vektor3f zStrahl = transformierteRichtung(
                    zurueck, t->ebene ? strahl.richtung : strahlFuerGeraetePunkt(vollInvers(blick().sicht()), mal(p, 1.1F)).richtung);
                const Vektor3f e = ecke(m_grenzen, m_drehAchse, -zStrahl, zStrahl);
                Transformationen tr;
                tr.drehung = drehung;
                tr.verschiebung = Vektor3f(t->punkt - Vektor3d(e));
                m_alles.allesTransformieren(tr, e);
                return;
            }
            // Verschiebe-Einrasten
            const std::size_t achse = groessteBetragsKomponente(nrm);
            aktuell = Vektor3f(t->punkt - Vektor3d(ecke(m_grenzen, achse, nrm, strahl.richtung)));
            m_ziel.verschieben(aktuell);
            return;
        }
        m_ziel.verschieben(aktuell);   // sonst an die Ausgangslage
    }
    void setze0(const Vektor3f& start) { m_0 = start; }
    static bool bedingung(const Zusatz& z, const Blick& v) { return z.strg && v.fuellen(); }   // useCondition

private:
    Verschiebbar& m_ziel;
    AllesTransformierbar& m_alles;
    Kasten m_grenzen;
    Vektor3f m_0;
    std::size_t m_drehAchse = 0;
    int m_drehVorzeichen = 1;
    std::optional<Ebene3d> m_entlang;
    Vektor3f m_entlangStart;
};

class VerschiebeFreiXyZ final : public Manipulierbar {   // TranslateFreeXY_Z
public:
    static inline int s_blickAbhaengig = 0;   // m_viewdependent
    VerschiebeFreiXyZ(Dienste& d, Verschiebbar& ziel, AllesTransformierbar& alles)
        : Manipulierbar(d), m_ziel(ziel), m_fang(d, ziel, alles) {}
    void bauen(const Matrix4& g, const GeraeteVektor& p, const Kasten& b, const Vektor3f& ursprung) override {
        m_achseZ = (s_blickAbhaengig != 0 || !blick().fuellen()) ? groessteBetragsKomponente(blick().blickrichtung()) : 2;
        // (0,0,0): kein guter Startpunkt (Komponenten irgendwo gezogen)
        if (m_0 == Vektor3f(0, 0, 0)) { m_0 = ursprung; }
        m_ebeneXy = Ebene3d(kAchsen[m_achseZ], m_0[m_achseZ]);
        Vektor3f xy = blick().betrachter() - m_0;
        xy[m_achseZ] = 0;
        normieren(xy);
        m_ebeneZ = Ebene3d(xy, punkt(xy, m_0));
        m_startXy = punktAufEbeneSicht(m_ebeneXy, blick().sicht(), p);
        m_startZ = punktAufEbeneSicht(m_ebeneZ, blick().sicht(), p);
        m_grenzen = b;
        m_fang.bauen(g, p, b, ursprung);
    }
    void transformieren(const Matrix4& m2o, const Matrix4& g, const GeraeteVektor& p) override {
        if (KastenFang::bedingung(zusatz(), blick())) {
            m_fang.transformieren(m2o, g, p);
            return;
        }
        Vektor3f aktuell;
        if (zusatz().alt && blick().fuellen()) {   // nur Z
            aktuell = komponentenMal(punktAufEbeneSicht(m_ebeneZ, blick().sicht(), p) - m_startZ, kAchsen[m_achseZ]);
        } else {
            aktuell = punktAufEbeneSicht(m_ebeneXy, blick().sicht(), p) - m_startXy;
            aktuell[m_achseZ] = 0;
        }
        if (zusatz().shift) { aktuell = komponentenMal(aktuell, kAchsen[groessteBetragsKomponente(aktuell)]); }
        if (zusatz().strg) {
            kastenVerschiebungEinrasten(aktuell, m_grenzen, fang());
        } else {
            einrasten(aktuell, fang());
        }
        m_ziel.verschieben(aktuell);
    }
    void setze0(const Vektor3f& start) {
        m_0 = start;
        m_fang.setze0(start);
    }

private:
    Vektor3f m_0;
    std::size_t m_achseZ = 2;
    Ebene3d m_ebeneXy;
    Ebene3d m_ebeneZ;
    Vektor3f m_startXy;
    Vektor3f m_startZ;
    Verschiebbar& m_ziel;
    Kasten m_grenzen;
    KastenFang m_fang;
};

// --- selection_mtable_rotate.h ---------------------------------------------------------

class DreheFrei final : public Manipulierbar {   // RotateFree
public:
    DreheFrei(Dienste& d, Drehbar& ziel) : Manipulierbar(d), m_ziel(ziel) {}
    void bauen(const Matrix4& g, const GeraeteVektor& p, const Kasten&, const Vektor3f&) override {
        m_start = punktAufKugel(g, p);
        normieren(m_start);
    }
    void transformieren(const Matrix4&, const Matrix4& g, const GeraeteVektor& p) override {
        Vektor3f aktuell = punktAufKugel(g, p);
        normieren(aktuell);
        if (zusatz().shift) {
            for (std::size_t i = 0; i < 3; ++i) {
                if (aktuell[i] == 0) {
                    m_ziel.drehen(quaternionAchseWinkel(kAchsen[i], zahlEingerastet(winkelFuerAchse(m_start, aktuell, kAchsen[i]),
                                                                                     static_cast<float>(kPi / 12.0))));
                    return;
                }
            }
        }
        m_ziel.drehen(quaternionAusEinheitsvektoren(m_start, aktuell));
    }

private:
    Vektor3f m_start;
    Drehbar& m_ziel;
};

class DreheAchse final : public Manipulierbar {   // RotateAxis
public:
    DreheAchse(Dienste& d, Drehbar& ziel) : Manipulierbar(d), m_ziel(ziel) {}
    void bauen(const Matrix4& g, const GeraeteVektor& p, const Kasten&, const Vektor3f& ursprung) override {
        const float skalar = static_cast<float>(
            punkt(m_achse, blick().fuellen() ? normiert(blick().betrachter() - ursprung) : blick().blickrichtung()));
        m_ebenenWeg = std::fabs(skalar) > 0.1F;
        if (m_ebenenWeg) {
            m_ursprung = ursprung;
            m_ebene = Ebene3d(m_achse, punkt(m_achse, m_ursprung));
            m_start = punktAufEbeneSicht(m_ebene, blick().sicht(), p) - m_ursprung;
            normieren(m_start);
        } else {
            m_start = punktAufKugel(g, p, m_radius);
            aufAchseBeschraenken(m_start, m_achse);
        }
    }
    // Wandelt die Lage in einen normierten Vektor senkrecht zur Achse.
    void transformieren(const Matrix4&, const Matrix4& g, const GeraeteVektor& p) override {
        Vektor3f aktuell;
        if (m_ebenenWeg) {
            aktuell = punktAufEbeneSicht(m_ebene, blick().sicht(), p) - m_ursprung;
            normieren(aktuell);
        } else {
            aktuell = punktAufKugel(g, p, m_radius);
            aufAchseBeschraenken(aktuell, m_achse);
        }
        if (zusatz().shift) {
            m_ziel.drehen(quaternionAchseWinkel(m_achse, zahlEingerastet(winkelFuerAchse(m_start, aktuell, m_achse), static_cast<float>(kPi / 12.0))));
        } else {
            m_ziel.drehen(quaternionAchseWinkel(m_achse, winkelFuerAchse(m_start, aktuell, m_achse)));
        }
    }
    void setzeAchse(const Vektor3f& a) { m_achse = a; }
    void setzeRadius(float r) { m_radius = r; }

private:
    Vektor3f m_achse;
    Vektor3f m_start;
    float m_radius = kRadius;
    bool m_ebenenWeg = false;
    Ebene3d m_ebene;
    Vektor3f m_ursprung;
    Drehbar& m_ziel;
};

// --- selection_mtable_scale.h ----------------------------------------------------------

class SkaliereAchse final : public Manipulierbar {   // ScaleAxis
public:
    SkaliereAchse(Dienste& d, Skalierbar& ziel) : Manipulierbar(d), m_ziel(ziel) {}
    void bauen(const Matrix4& g, const GeraeteVektor& p, const Kasten& b, const Vektor3f& u) override {
        m_start = punktAufAchse(m_achse, g, p);
        m_gewaehlteAusdehnung = Vektor3f(std::max(b.origin[0] + b.extents[0] - u[0], -b.origin[0] + b.extents[0] + u[0]),
                                         std::max(b.origin[1] + b.extents[1] - u[1], -b.origin[1] + b.extents[1] + u[1]),
                                         std::max(b.origin[2] + b.extents[2] - u[2], -b.origin[2] + b.extents[2] + u[2]));
        m_grenzen = b;
    }
    void transformieren(const Matrix4& m2o, const Matrix4& g, const GeraeteVektor& p) override {
        const Vektor3f aktuell = punktAufAchse(m_achse, g, p);
        Vektor3f delta = aktuell - m_start;
        delta = translationLokalZuObjekt(delta, m2o);
        einrasten(delta, fang());
        delta = komponentenMal(delta, m_achse);
        Vektor3f start = eingerastet(m_start, fang() != 0 ? fang() : 1e-3F);
        for (std::size_t i = 0; i < 3; ++i) {   // bei grossem Raster nicht auf 0 einrasten
            if (zahlEingerastet(m_start[i], 1e-3F) != 0 && start[i] == 0) { start[i] = fang(); }
        }
        // ohne Kasten
        Vektor3f s(start[0] == 0 ? 1 : 1 + delta[0] / start[0], start[1] == 0 ? 1 : 1 + delta[1] / start[1],
                   start[2] == 0 ? 1 : 1 + delta[2] / start[2]);
        // mit Kasten
        for (std::size_t i = 0; i < 3; ++i) {
            // Epsilon gegen riesige Faktoren bei Modellen mit winziger Ausdehnung
            if (m_gewaehlteAusdehnung[i] > 0.0625F && m_achse[i] != 0) {
                s[i] = (m_gewaehlteAusdehnung[i] + delta[i]) / m_gewaehlteAusdehnung[i];
                if (zusatz().strg) {   // Kastengroesse aufs Raster
                    const float breite = zahlEingerastet(s[i] * m_grenzen.extents[i] * 2.F, fang());
                    s[i] = breite / (m_grenzen.extents[i] * 2.F);
                }
            }
        }
        if (zusatz().shift) {   // alle Achsen gleich
            for (std::size_t i = 0; i < 3; ++i) {
                if (m_achse[i] == 0) { s[i] = static_cast<float>(punkt(s, komponentenMal(m_achse, m_achse))); }
            }
        }
        m_ziel.skalieren(s);
    }
    void setzeAchse(const Vektor3f& a) { m_achse = a; }

private:
    Vektor3f m_start;
    Vektor3f m_achse;
    Skalierbar& m_ziel;
    Vektor3f m_gewaehlteAusdehnung;
    Kasten m_grenzen;
};

class SkaliereFrei final : public Manipulierbar {   // ScaleFree
public:
    SkaliereFrei(Dienste& d, Skalierbar& ziel) : Manipulierbar(d), m_ziel(ziel) {}
    void bauen(const Matrix4& g, const GeraeteVektor& p, const Kasten& b, const Vektor3f& u) override {
        m_start = punktAufEbeneGeraet(g, p);
        m_gewaehlteAusdehnung = Vektor3f(std::max(b.origin[0] + b.extents[0] - u[0], -(b.origin[0] - b.extents[0] - u[0])),
                                         std::max(b.origin[1] + b.extents[1] - u[1], -(b.origin[1] - b.extents[1] - u[1])),
                                         std::max(b.origin[2] + b.extents[2] - u[2], -(b.origin[2] - b.extents[2] - u[2])));
        m_grenzen = b;
    }
    void transformieren(const Matrix4& m2o, const Matrix4& g, const GeraeteVektor& p) override {
        const Vektor3f aktuell = punktAufEbeneGeraet(g, p);
        Vektor3f delta = aktuell - m_start;
        delta = translationLokalZuObjekt(delta, m2o);
        einrasten(delta, fang());
        if (m_achse != Vektor3f(0, 0, 0)) { delta = komponentenMal(delta, m_achse) + komponentenMal(delta, m_achse2); }
        Vektor3f start = eingerastet(m_start, fang() != 0 ? fang() : 1e-3F);
        for (std::size_t i = 0; i < 3; ++i) {
            if (zahlEingerastet(m_start[i], 1e-3F) != 0 && start[i] == 0) { start[i] = fang(); }
        }
        const std::size_t ignorieren = kleinsteBetragsKomponente(m_start);
        if (zusatz().shift) { start[ignorieren] = 0; }
        Vektor3f s(start[0] == 0 ? 1 : 1 + delta[0] / start[0], start[1] == 0 ? 1 : 1 + delta[1] / start[1],
                   start[2] == 0 ? 1 : 1 + delta[2] / start[2]);
        for (std::size_t i = 0; i < 3; ++i) {
            if (m_gewaehlteAusdehnung[i] > 0.0625F && start[i] != 0) {
                s[i] = (m_gewaehlteAusdehnung[i] + delta[i]) / m_gewaehlteAusdehnung[i];
                if (zusatz().strg) {
                    const float breite = zahlEingerastet(s[i] * m_grenzen.extents[i] * 2.F, fang());
                    s[i] = breite / (m_grenzen.extents[i] * 2.F);
                }
            }
        }
        if (zusatz().shift) {   // zwei Achsen gleich
            float beste = ignorieren != 0 ? s[0] : s[1];
            for (std::size_t i = ignorieren != 0 ? 1 : 2; i < 3; ++i) {
                if (ignorieren != i && std::fabs(s[i]) < std::fabs(beste)) { beste = s[i]; }
            }
            for (std::size_t i = 0; i < 3; ++i) {
                if (ignorieren != i) { s[i] = (s[i] < 0) ? -std::fabs(beste) : std::fabs(beste); }
            }
        }
        m_ziel.skalieren(s);
    }
    void setzeAchsen(const Vektor3f& a, const Vektor3f& a2) {
        m_achse = a;
        m_achse2 = a2;
    }

private:
    Vektor3f m_start;
    Vektor3f m_achse;
    Vektor3f m_achse2;
    Skalierbar& m_ziel;
    Vektor3f m_gewaehlteAusdehnung;
    Kasten m_grenzen;
};

// --- selection_mtable_skew.h ------------------------------------------------------------

class VerzerreAchse final : public Manipulierbar {   // SkewAxis
public:
    VerzerreAchse(Dienste& d, Verzerrbar& ziel) : Manipulierbar(d), m_ziel(ziel) {}
    void bauen(const Matrix4&, const GeraeteVektor&, const Kasten& b, const Vektor3f& u) override {
        Vektor3f xy = blick().betrachter() - m_0;
        xy[static_cast<std::size_t>(m_welche)] = 0;
        normieren(xy);
        m_ebeneZ = Ebene3d(xy, punkt(xy, m_0));
        m_grenzen = b;
        const auto um = static_cast<std::size_t>(m_um);
        m_umAusdehnung = b.origin[um] + b.extents[um] * static_cast<float>(m_umVorzeichen) - u[um];
    }
    void transformieren(const Matrix4&, const Matrix4&, const GeraeteVektor& p) override {
        const Vektor3f aktuell = punktAufEbeneSicht(m_ebeneZ, blick().sicht(), p) - m_0;
        Verzerrung v;
        v.index = static_cast<std::size_t>(m_um * 4 + m_welche);
        v.wert = m_umAusdehnung != 0 ? zahlEingerastet(aktuell[static_cast<std::size_t>(m_welche)], fang()) / m_umAusdehnung : 0;
        m_ziel.verzerren(v);
    }
    void setzeAchsen(int welche, int um, int vorzeichen) {
        m_welche = welche;
        m_um = um;
        m_umVorzeichen = vorzeichen;
    }
    void setze0(const Vektor3f& start) { m_0 = start; }

private:
    Vektor3f m_0;
    Ebene3d m_ebeneZ;
    int m_welche = 0;
    int m_um = 1;
    int m_umVorzeichen = 1;
    Verzerrbar& m_ziel;
    float m_umAusdehnung = 0;
    Kasten m_grenzen;
};

// --- selection_mtable_brush.h ------------------------------------------------------------

class NeuerPinsel final : public Manipulierbar {   // DragNewBrush
public:
    explicit NeuerPinsel(Dienste& d) : Manipulierbar(d) {}
    void bauen(const Matrix4&, const GeraeteVektor&, const Kasten&, const Vektor3f&) override {
        m_hoeheZ = m_groesse[0] = m_groesse[1] = m_groesse[2] = m_d.rasterGroesse();
        m_pinsel.reset();
    }
    void transformieren(const Matrix4&, const Matrix4&, const GeraeteVektor& p) override {
        const Vektor3f ecke = Vektor3f(m_groesse.x(), m_groesse.y(), m_hoeheZ) + m_0;
        Vektor3f roh = punktAufEbeneSicht(Ebene3d(kAchseZ, punkt(kAchseZ, ecke)), blick().sicht(), p) - m_0;
        const Vektor3f xy = normiert(Vektor3f(blick().modelview()[2], blick().modelview()[6], 0));
        roh.z() = (punktAufEbeneSicht(Ebene3d(xy, punkt(xy, ecke)), blick().sicht(), p) - m_0).z();
        Vektor3f diff = eingerastet(roh, fang());
        for (std::size_t i = 0; i < 3; ++i) {
            if (diff[i] == 0) { diff[i] = roh[i] < 0 ? -m_d.rasterGroesse() : m_d.rasterGroesse(); }
        }
        if (zusatz().alt) {   // Hoehe einstellen
            diff.x() = m_groesse.x();
            diff.y() = m_groesse.y();
        } else {
            diff.z() = m_groesse.z();
        }
        const float z = projiziert(transformierterVektor4(blick().sicht(), Vektor4f(diff + m_0, 1))).z();
        if (std::isnan(z) || z > 1) { return; }   // NAN und hinter Nah-/Fernebene
        if (zusatz().shift || zusatz().strg) {   // Quadrat oder Wuerfel
            const float seite = std::max(std::fabs(diff.x()), std::fabs(diff.y()));
            diff.x() = std::copysign(seite, diff.x());
            diff.y() = std::copysign(seite, diff.y());
            if (zusatz().strg && !zusatz().alt) { diff.z() = std::copysign(seite, diff.z()); }
        }
        m_groesse = diff;
        if (zusatz().alt) { m_hoeheZ = diff.z(); }
        Vektor3f mn = m_0;
        Vektor3f mx = m_0 + diff;
        for (std::size_t i = 0; i < 3; ++i) {
            if (mn[i] > mx[i]) { std::swap(mn[i], mx[i]); }
        }
        m_d.quader(m_pinsel, kastenAusMinMax(mn, mx));
    }
    void setze0(const Vektor3f& start) { m_0 = start; }

private:
    Vektor3f m_0;
    Vektor3f m_groesse;
    float m_hoeheZ = 0;   // m_setSizeZ
    std::shared_ptr<Pinsel> m_pinsel;
};

// --- TransformOriginTranslate (selection.cpp:806-844) --------------------------------------

class UrsprungVerschieben final : public Manipulierbar {
public:
    UrsprungVerschieben(Dienste& d, UrsprungVerschiebbar& ziel) : Manipulierbar(d), m_ziel(ziel) {}
    void bauen(const Matrix4& g, const GeraeteVektor& p, const Kasten&, const Vektor3f&) override { m_start = punktAufEbeneGeraet(g, p); }
    void transformieren(const Matrix4& m2o, const Matrix4& g, const GeraeteVektor& p) override {
        Vektor3f aktuell = punktAufEbeneGeraet(g, p) - m_start;
        if (zusatz().shift) {   // auf eine Achse
            for (std::size_t i = 0; i < 3; ++i) {
                if (std::fabs(aktuell[i]) >= std::fabs(aktuell[(i + 1) % 3])) {
                    aktuell[(i + 1) % 3] = 0;
                } else {
                    aktuell[i] = 0;
                }
            }
        }
        bool gesetzt[3] = {true, true, true};
        for (std::size_t i = 0; i < 3; ++i) {
            if (std::fabs(aktuell[i]) < 1e-3F) { gesetzt[i] = false; }
        }
        aktuell = translationLokalZuObjekt(aktuell, m2o);
        m_ziel.ursprungVerschieben(aktuell, gesetzt);
    }

private:
    Vektor3f m_start;
    UrsprungVerschiebbar& m_ziel;
};

// --- Manipulator (selection_.h:50-62) ------------------------------------------------------

class Manipulator {
public:
    virtual ~Manipulator() = default;
    virtual Manipulierbar* manipulierbar() = 0;                              // GetManipulatable
    virtual void testen(const Blick& v, const Matrix4& pivotZuWelt) = 0;    // testSelect
    virtual void hervorheben(const Blick& v, const Matrix4& pivotZuWelt) { testen(v, pivotZuWelt); }   // highlight
    virtual void zeichnen(const Blick&, const Matrix4&, ManipZeichnung&) {}  // render
    virtual void setzeGewaehlt(bool an) = 0;
    [[nodiscard]] virtual bool gewaehlt() const = 0;
};

// SelectionPool fuer die SelectableBool der Manipulatoren
class ManipPool {
public:
    void dazu(const Treffer& t, bool* w) {
        if (t.gueltig()) { m_pool.emplace(t, w); }
    }
    [[nodiscard]] bool leer() const { return m_pool.empty(); }
    [[nodiscard]] std::multimap<Treffer, bool*>& menge() { return m_pool; }

private:
    std::multimap<Treffer, bool*> m_pool;
};
// ManipulatorSelectionChangeable::selectionChange( SelectionPool& )
void auswahlWechsel(ManipPool& p) {
    if (!p.leer()) { *p.menge().begin()->second = true; }
}

// Linien in eine Zeichnung (lokal -> Welt)
void linieDazu(std::vector<ManipZeichnung::Linie>& aus, const Matrix4& m, const Vektor3f& a, const Vektor3f& b, ManipFarbe f) {
    aus.push_back(ManipZeichnung::Linie{transformierterPunkt(m, a), transformierterPunkt(m, b), f});
}
void schleifeDazu(std::vector<ManipZeichnung::Linie>& aus, const Matrix4& m, const Vektor3f* p, std::size_t n, ManipFarbe f) {
    for (std::size_t i = 0; i < n; ++i) { linieDazu(aus, m, p[i], p[(i + 1) % n], f); }   // GL_LINE_LOOP
}
void zugDazu(std::vector<ManipZeichnung::Linie>& aus, const Matrix4& m, const Vektor3f* p, std::size_t n, ManipFarbe f) {
    for (std::size_t i = 0; i + 1 < n; ++i) { linieDazu(aus, m, p[i], p[i + 1], f); }   // GL_LINE_STRIP
}
void dreieckeDazu(std::vector<ManipZeichnung::Dreieck>& aus, const Matrix4& m, const std::vector<FlachPunkt>& p, ManipFarbe f) {
    for (std::size_t i = 0; i + 2 < p.size(); i += 3) {
        ManipZeichnung::Dreieck d;
        for (std::size_t k = 0; k < 3; ++k) {
            d.p[k] = transformierterPunkt(m, p[i + k].p);
            d.n[k] = normiert(transformierteRichtung(m, p[i + k].n));
        }
        d.farbe = f;
        aus.push_back(d);
    }
}
std::vector<Vektor3f> nurPunkte(const std::vector<FlachPunkt>& p) {
    std::vector<Vektor3f> r;
    r.reserve(p.size());
    for (const auto& q : p) { r.push_back(q.p); }
    return r;
}

// --- selection_mtor_translate.cpp ------------------------------------------------------------

class VerschiebeManipulator final : public Manipulator {
public:
    VerschiebeManipulator(Dienste& d, Verschiebbar& ziel, std::size_t segmente, float laenge)
        : m_d(d), m_frei(d, ziel), m_achse(d, ziel) {
        for (std::size_t a = 0; a < 3; ++a) {
            pfeilLinie(laenge, m_pfeil[a], a);
            m_kopf[a].resize(3 * 2 * (segmente << 3));
            pfeilKopf(segmente, laenge, m_kopf[a].data(), a);
            m_kopfPunkte[a] = nurPunkte(m_kopf[a]);
        }
        viereckPunkte(16, m_viereck);
    }
    static bool achseZeigen(const PivotZuWelt& p, const Vektor3f& achse) { return std::fabs(punkt(p.bildAchse, achse)) < 0.95; }
    void zeichnen(const Blick& v, const Matrix4& p2w, ManipZeichnung& aus) override {
        m_pivot.aktualisieren(p2w, v.modelview(), v.projektion(), v.bildfeld());
        const ManipFarbe farben[3] = {kFarbeX, kFarbeY, kFarbeZ};
        const Vektor4f* spalten[3] = {&m_pivot.weltRaum.x(), &m_pivot.weltRaum.y(), &m_pivot.weltRaum.z()};
        bool zeigen[3];
        for (std::size_t a = 0; a < 3; ++a) { zeigen[a] = achseZeigen(m_pivot, normiert(spalten[a]->vec3())); }
        // $WIRE_OVERLAY
        for (std::size_t a = 0; a < 3; ++a) {
            if (zeigen[a]) { linieDazu(aus.draht, m_pivot.weltRaum, m_pfeil[a][0], m_pfeil[a][1], farbeGewaehlt(farben[a], m_gewaehlt[a])); }
        }
        schleifeDazu(aus.draht, m_pivot.blickebenenRaum, m_viereck, 4, farbeGewaehlt(kFarbeBild, m_bild));
        // $FLATSHADE_OVERLAY
        for (std::size_t a = 0; a < 3; ++a) {
            if (zeigen[a]) { dreieckeDazu(aus.flach, m_pivot.weltRaum, m_kopf[a], farbeGewaehlt(farben[a], m_gewaehlt[a])); }
        }
    }
    void testen(const Blick& v, const Matrix4& p2w) override {
        if (!m_d.zusatzTasten().keine()) { return; }   // selectionChange( nullptr )
        m_pivot.aktualisieren(p2w, v.modelview(), v.projektion(), v.bildfeld());
        ManipPool pool;
        const Vektor4f* spalten[3] = {&m_pivot.weltRaum.x(), &m_pivot.weltRaum.y(), &m_pivot.weltRaum.z()};
        bool zeigen[3];
        for (std::size_t a = 0; a < 3; ++a) { zeigen[a] = achseZeigen(m_pivot, normiert(spalten[a]->vec3())); }
        {
            const Matrix4 l2v = multipliziert(v.sicht(), m_pivot.blickpunktRaum);
            Treffer t;
            viereckBesterPunkt(l2v, Wegschneiden::CW, m_viereck, t);
            if (t.gueltig()) {
                t = Treffer(0, 0);
                pool.dazu(t, &m_bild);
            }
        }
        {
            const Matrix4 l2v = multipliziert(v.sicht(), m_pivot.weltRaum);
            for (std::size_t a = 0; a < 3; ++a) {
                if (!zeigen[a]) { continue; }
                Treffer t;
                linieBesterPunkt(l2v, m_pfeil[a], t);
                dreieckeBesterPunkt(l2v, Wegschneiden::CW, m_kopfPunkte[a].data(), m_kopfPunkte[a].size(), t);
                pool.dazu(t, &m_gewaehlt[a]);
            }
        }
        auswahlWechsel(pool);
    }
    Manipulierbar* manipulierbar() override {
        for (std::size_t a = 0; a < 3; ++a) {
            if (m_gewaehlt[a]) {
                m_achse.setzeAchse(kAchsen[a]);
                return &m_achse;
            }
        }
        return &m_frei;
    }
    void setzeGewaehlt(bool an) override { m_gewaehlt[0] = m_gewaehlt[1] = m_gewaehlt[2] = m_bild = an; }
    [[nodiscard]] bool gewaehlt() const override { return m_gewaehlt[0] || m_gewaehlt[1] || m_gewaehlt[2] || m_bild; }

private:
    Dienste& m_d;
    VerschiebeFrei m_frei;
    VerschiebeAchse m_achse;
    Vektor3f m_pfeil[3][2];
    std::vector<FlachPunkt> m_kopf[3];
    std::vector<Vektor3f> m_kopfPunkte[3];
    Vektor3f m_viereck[4];
    bool m_gewaehlt[3] = {false, false, false};
    bool m_bild = false;
    PivotZuWelt m_pivot;
};

// --- selection_mtor_rotate.cpp ----------------------------------------------------------------

Vektor3f normiertSicher(const Vektor3f& v) { return v == Vektor3f(0, 0, 0) ? Vektor3f(0, 0, 0) : normiert(v); }   // normalised_safe

class DrehManipulator final : public Manipulator {
public:
    DrehManipulator(Dienste& d, Drehbar& ziel, std::size_t segmente, float radius)
        : m_d(d), m_frei(d, ziel), m_achse(d, ziel), m_kreisX((segmente << 2) + 1), m_kreisY((segmente << 2) + 1),
          m_kreisZ((segmente << 2) + 1), m_kreisBild(segmente << 3), m_kreisKugel(segmente << 3) {
        halbkreisPunkte(segmente, radius, m_kreisX.data(), Umordnung::YZX);
        halbkreisPunkte(segmente, radius, m_kreisY.data(), Umordnung::ZXY);
        halbkreisPunkte(segmente, radius, m_kreisZ.data(), Umordnung::XYZ);
        kreisPunkte(segmente, radius * 1.15F, m_kreisBild.data(), Umordnung::XYZ);
        kreisPunkte(segmente, radius, m_kreisKugel.data(), Umordnung::XYZ);
    }
    void kreiseAktualisieren() {   // updateCircleTransforms
        const Vektor3f lokal = transformierteRichtung(transponiert(m_pivot.weltRaum), m_pivot.blickpunktRaum.z().vec3());
        m_xSichtbar = !gleichEps(kAchseX, lokal, 1e-6F);
        if (m_xSichtbar) {
            m_lzwX = kEinheitsMatrix;
            m_lzwX.y().vec3() = normiertSicher(kreuz(kAchseX, lokal));
            m_lzwX.z().vec3() = normiertSicher(kreuz(m_lzwX.x().vec3(), m_lzwX.y().vec3()));
            vormultipliziere(m_lzwX, m_pivot.weltRaum);
        }
        m_ySichtbar = !gleichEps(kAchseY, lokal, 1e-6F);
        if (m_ySichtbar) {
            m_lzwY = kEinheitsMatrix;
            m_lzwY.z().vec3() = normiertSicher(kreuz(kAchseY, lokal));
            m_lzwY.x().vec3() = normiertSicher(kreuz(m_lzwY.y().vec3(), m_lzwY.z().vec3()));
            vormultipliziere(m_lzwY, m_pivot.weltRaum);
        }
        m_zSichtbar = !gleichEps(kAchseZ, lokal, 1e-6F);
        if (m_zSichtbar) {
            m_lzwZ = kEinheitsMatrix;
            m_lzwZ.x().vec3() = normiertSicher(kreuz(kAchseZ, lokal));
            m_lzwZ.y().vec3() = normiertSicher(kreuz(m_lzwZ.z().vec3(), m_lzwZ.x().vec3()));
            vormultipliziere(m_lzwZ, m_pivot.weltRaum);
        }
    }
    void zeichnen(const Blick& v, const Matrix4& p2w, ManipZeichnung& aus) override {
        m_pivot.aktualisieren(p2w, v.modelview(), v.projektion(), v.bildfeld());
        kreiseAktualisieren();
        // $WIRE_OVERLAY
        schleifeDazu(aus.draht, m_pivot.blickpunktRaum, m_kreisBild.data(), m_kreisBild.size(), farbeGewaehlt(kFarbeBild, m_bild));
        schleifeDazu(aus.draht, m_pivot.blickpunktRaum, m_kreisKugel.data(), m_kreisKugel.size(), farbeGewaehlt(kFarbeKugel, false));
        if (m_xSichtbar) { zugDazu(aus.draht, m_lzwX, m_kreisX.data(), m_kreisX.size(), farbeGewaehlt(kFarbeX, m_x)); }
        if (m_ySichtbar) { zugDazu(aus.draht, m_lzwY, m_kreisY.data(), m_kreisY.size(), farbeGewaehlt(kFarbeY, m_y)); }
        if (m_zSichtbar) { zugDazu(aus.draht, m_lzwZ, m_kreisZ.data(), m_kreisZ.size(), farbeGewaehlt(kFarbeZ, m_z)); }
    }
    void testen(const Blick& v, const Matrix4& p2w) override {
        if (!m_d.zusatzTasten().keine()) { return; }
        m_pivot.aktualisieren(p2w, v.modelview(), v.projektion(), v.bildfeld());
        kreiseAktualisieren();
        ManipPool pool;
        {
            Treffer t;
            linienZugBesterPunkt(multipliziert(v.sicht(), m_lzwX), m_kreisX.data(), m_kreisX.size(), t);
            pool.dazu(t, &m_x);
        }
        {
            Treffer t;
            linienZugBesterPunkt(multipliziert(v.sicht(), m_lzwY), m_kreisY.data(), m_kreisY.size(), t);
            pool.dazu(t, &m_y);
        }
        {
            Treffer t;
            linienZugBesterPunkt(multipliziert(v.sicht(), m_lzwZ), m_kreisZ.data(), m_kreisZ.size(), t);
            pool.dazu(t, &m_z);
        }
        {
            Treffer t;
            linienSchleifeBesterPunkt(multipliziert(v.sicht(), m_pivot.blickpunktRaum), m_kreisBild.data(), m_kreisBild.size(), t);
            pool.dazu(t, &m_bild);
        }
        m_bildAchse = m_pivot.bildAchse;
        if (pool.leer()) { pool.dazu(Treffer(0, 0), &m_kugel); }
        auswahlWechsel(pool);
    }
    Manipulierbar* manipulierbar() override {
        if (m_x) {
            m_achse.setzeAchse(kAchseX);
            return &m_achse;
        }
        if (m_y) {
            m_achse.setzeAchse(kAchseY);
            return &m_achse;
        }
        if (m_z) {
            m_achse.setzeAchse(kAchseZ);
            return &m_achse;
        }
        if (m_bild) {
            m_achse.setzeAchse(m_bildAchse);
            return &m_achse;
        }
        return &m_frei;
    }
    void setzeGewaehlt(bool an) override { m_x = m_y = m_z = m_bild = m_kugel = an; }
    [[nodiscard]] bool gewaehlt() const override { return m_x || m_y || m_z || m_bild || m_kugel; }

private:
    Dienste& m_d;
    DreheFrei m_frei;
    DreheAchse m_achse;
    Vektor3f m_bildAchse;
    std::vector<Vektor3f> m_kreisX, m_kreisY, m_kreisZ, m_kreisBild, m_kreisKugel;
    bool m_x = false, m_y = false, m_z = false, m_bild = false, m_kugel = false;
    PivotZuWelt m_pivot;
    Matrix4 m_lzwX, m_lzwY, m_lzwZ;   // m_local2world_x/y/z
    bool m_xSichtbar = false, m_ySichtbar = false, m_zSichtbar = false;
};

// --- selection_mtor_scale.cpp ------------------------------------------------------------------

class SkalierManipulator final : public Manipulator {
public:
    SkalierManipulator(Dienste& d, Skalierbar& ziel, float laenge) : m_d(d), m_frei(d, ziel), m_achse(d, ziel) {
        for (std::size_t a = 0; a < 3; ++a) { pfeilLinie(laenge, m_pfeil[a], a); }
        viereckPunkte(16, m_viereck);
    }
    void zeichnen(const Blick& v, const Matrix4& p2w, ManipZeichnung& aus) override {
        m_pivot.aktualisieren(p2w, v.modelview(), v.projektion(), v.bildfeld());
        // ohne eigenen Zustand: renderSolid setzt $POINT
        const ManipFarbe farben[3] = {kFarbeX, kFarbeY, kFarbeZ};
        for (std::size_t a = 0; a < 3; ++a) { linieDazu(aus.ohneTiefe, m_pivot.weltRaum, m_pfeil[a][0], m_pfeil[a][1], farbeGewaehlt(farben[a], m_gewaehlt[a])); }
        schleifeDazu(aus.ohneTiefe, m_pivot.blickpunktRaum, m_viereck, 4, farbeGewaehlt(kFarbeBild, m_bild));
    }
    void testen(const Blick& v, const Matrix4& p2w) override {
        if (!m_d.zusatzTasten().keine()) { return; }
        m_pivot.aktualisieren(p2w, v.modelview(), v.projektion(), v.bildfeld());
        ManipPool pool;
        {
            const Matrix4 l2v = multipliziert(v.sicht(), m_pivot.weltRaum);
            for (std::size_t a = 0; a < 3; ++a) {
                Treffer t;
                linieBesterPunkt(l2v, m_pfeil[a], t);
                pool.dazu(t, &m_gewaehlt[a]);
            }
        }
        {
            Treffer t;
            viereckBesterPunkt(multipliziert(v.sicht(), m_pivot.blickpunktRaum), Wegschneiden::CW, m_viereck, t);
            pool.dazu(t, &m_bild);
        }
        auswahlWechsel(pool);
    }
    Manipulierbar* manipulierbar() override {
        for (std::size_t a = 0; a < 3; ++a) {
            if (m_gewaehlt[a]) {
                m_achse.setzeAchse(kAchsen[a]);
                return &m_achse;
            }
        }
        m_frei.setzeAchsen(Vektor3f(0, 0, 0), Vektor3f(0, 0, 0));
        return &m_frei;
    }
    void setzeGewaehlt(bool an) override { m_gewaehlt[0] = m_gewaehlt[1] = m_gewaehlt[2] = m_bild = an; }
    [[nodiscard]] bool gewaehlt() const override { return m_gewaehlt[0] || m_gewaehlt[1] || m_gewaehlt[2] || m_bild; }

private:
    Dienste& m_d;
    SkaliereFrei m_frei;
    SkaliereAchse m_achse;
    Vektor3f m_pfeil[3][2];
    Vektor3f m_viereck[4];
    bool m_gewaehlt[3] = {false, false, false};
    bool m_bild = false;
    PivotZuWelt m_pivot;
};

// --- DragPlanes (libs/dragplanes.h) ------------------------------------------------------------

// aabb_corners_oriented / aabb_planes_oriented mit rotation = Einheit
constexpr std::size_t kKastenIndizes[24] = {
    2, 1, 5, 6,   // +x rechts
    3, 7, 4, 0,   // -x links
    1, 0, 4, 5,   // +y vorn
    3, 2, 6, 7,   // -y hinten
    0, 1, 2, 3,   // +z oben
    7, 6, 5, 4,   // -z unten
};
std::array<Ebene3d, 6> kastenEbenen(const Kasten& k) {
    const double x = k.origin[0];
    const double y = k.origin[1];
    const double z = k.origin[2];
    return {Ebene3d(kAchseX, x + k.extents[0]), Ebene3d(-kAchseX, -(x - k.extents[0])), Ebene3d(kAchseY, y + k.extents[1]),
            Ebene3d(-kAchseY, -(y - k.extents[1])), Ebene3d(kAchseZ, z + k.extents[2]), Ebene3d(-kAchseZ, -(z - k.extents[2]))};
}

// PlaneSelectable::BestPlaneData
struct BesteEbene {
    Ebene3d ebene{0, 0, 0, 0};
    [[nodiscard]] bool gueltig() const { return ebeneGueltig(ebene); }
    Treffer treffer;   // direkt
    [[nodiscard]] bool direkt() const { return treffer.gueltig(); }
    Vektor3f naechsterPunkt;   // indirekt
    float abstand = std::numeric_limits<float>::max();
};

struct ZiehFlaechen {   // DragPlanes
    bool gewaehlt[6] = {false, false, false, false, false, false};
    Kasten grenzen;   // mutable m_bounds

    [[nodiscard]] bool irgendeine() const { return std::any_of(std::begin(gewaehlt), std::end(gewaehlt), [](bool b) { return b; }); }
    void setze(bool an) { std::fill(std::begin(gewaehlt), std::end(gewaehlt), an); }

    // selectPlanes: liefert die Indizes der gewaehlten Seiten
    void ebenenWaehlen(const Kasten& k, AuswahlVolumen& test, std::vector<std::size_t>& gewaehlteSeiten, std::vector<Ebene3d>& gewaehlteEbenen) {
        const std::array<Vektor3f, 8> e = kastenEcken(k);
        const std::array<Ebene3d, 6> eb = kastenEbenen(k);
        const Vektor3f blickRichtung = test.blick().blickrichtung();
        double besterSkalar = 1;
        int wahl[2] = {-1, -1};
        for (std::size_t i = 0; i < 6; ++i) {
            const std::size_t idx = i * 4;
            const Vektor3f mitteF = mitte(e[kKastenIndizes[idx]], e[kKastenIndizes[idx + 2]]);
            const Vektor3f proj = projiziert(transformierterVektor4(test.blick().sicht(), Vektor4f(mitteF, 1)));
            const Vektor3f naechster = projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, proj[2], 1)));
            if (punkt(eb[i].normal(), naechster - e[kKastenIndizes[idx]]) > 0 && punkt(eb[i].normal(), naechster - e[kKastenIndizes[idx + 1]]) > 0 &&
                punkt(eb[i].normal(), naechster - e[kKastenIndizes[idx + 2]]) > 0 && punkt(eb[i].normal(), naechster - e[kKastenIndizes[idx + 3]]) > 0) {
                const double skalar = std::fabs(punkt(eb[i].normal(), blickRichtung));
                const double diff = besterSkalar - skalar;
                if (diff > 0.03) {
                    besterSkalar = skalar;
                    wahl[0] = static_cast<int>(i);
                    wahl[1] = -1;
                } else if (std::fabs(diff) <= 0.03 && !test.blick().fuellen()) {   // in der Kamera nur eine
                    wahl[1] = static_cast<int>(i);
                }
            }
        }
        for (const int i : wahl) {
            if (i >= 0) {
                gewaehlteSeiten.push_back(static_cast<std::size_t>(i));
                gewaehlteEbenen.push_back(eb[static_cast<std::size_t>(i)]);
            }
        }
        grenzen = k;
    }
    // selectReversedPlanes
    void umgedrehteWaehlen(const Kasten& k, const std::vector<Ebene3d>& gewaehlteEbenen, std::vector<std::size_t>& seiten) const;
    // bestPlaneDirect
    void besteEbeneDirekt(const Kasten& k, AuswahlVolumen& test, BesteEbene& daten) {
        Kasten k2 = k;
        for (std::size_t i = 0; i < 3; ++i) {   // flache Patches besser waehlbar
            if (k2.extents[i] < 1) { k2.extents[i] = 4; }
        }
        const std::array<Vektor3f, 8> e = kastenEcken(k2);
        const std::array<Ebene3d, 6> eb = kastenEbenen(k2);
        for (std::size_t i = 0; i < 6; ++i) {
            Treffer t;
            const unsigned int idx[4] = {static_cast<unsigned int>(kKastenIndizes[i * 4]), static_cast<unsigned int>(kKastenIndizes[i * 4 + 1]),
                                         static_cast<unsigned int>(kKastenIndizes[i * 4 + 2]), static_cast<unsigned int>(kKastenIndizes[i * 4 + 3])};
            test.testVierecke(e.data(), idx, 4, t);
            if (t < daten.treffer) {
                daten.treffer = t;
                daten.ebene = eb[i];
            }
        }
        grenzen = k;
    }
    // bestPlaneIndirect
    void besteEbeneIndirekt(const Kasten& k, AuswahlVolumen& test, BesteEbene& daten) {
        const std::array<Vektor3f, 8> e = kastenEcken(k);
        const std::array<Ebene3d, 6> eb = kastenEbenen(k);
        static constexpr std::size_t kKanten[24] = {0, 1, 3, 2, 7, 6, 4, 5, 2, 1, 3, 0, 6, 5, 7, 4, 4, 0, 5, 1, 6, 2, 7, 3};
        static constexpr std::size_t kNachbarn[24] = {4, 2, 4, 3, 5, 3, 5, 2, 4, 0, 4, 1, 5, 0, 5, 1, 1, 2, 2, 0, 0, 3, 3, 1};
        float skalar = 1;
        const bool nullAusdehnung = k.extents[0] == 0 || k.extents[1] == 0 || k.extents[2] == 0;
        for (std::size_t i = 0; i < 24; i += 2) {
            Vektor3f start = e[kKanten[i]];
            Vektor3f ende = e[kKanten[i + 1]];
            if (k.extents[i / 8] != 0 && linieAnNahebeneKlippen(test.blick().sicht(), start, ende) == 2) {
                const Vektor3f neu = naechsterPunktAufLinieF(start, ende);
                const auto abstandNeu = static_cast<float>(laengeQuadrat(neu));
                const auto skalarNeu = static_cast<float>(std::fabs(punkt(normiert(neu), normiert(ende - start))));
                // grosses Epsilon wie NRC (32-Bit-Optimierung rechnet in double)
                if (daten.abstand - abstandNeu > 1e-2F || (std::fabs(abstandNeu - daten.abstand) < 1e-2F && skalarNeu < skalar)) {
                    const Ebene3d& e1 = eb[kNachbarn[i]];
                    const Ebene3d& e2 = eb[kNachbarn[i + 1]];
                    const auto zuweisen = [&](const Ebene3d& neueEbene) {
                        daten.ebene = neueEbene;
                        daten.naechsterPunkt = neu;
                        daten.abstand = abstandNeu;
                        skalar = skalarNeu;
                    };
                    if (test.blick().fuellen()) {
                        if (abstandZuPunkt(e1, test.blick().betrachter()) <= 0) {
                            zuweisen(k.extents[kNachbarn[i] / 2] == 0 ? e2 : e1);   // die andere bei Nullausdehnung
                        } else if (abstandZuPunkt(e2, test.blick().betrachter()) <= 0) {
                            zuweisen(k.extents[kNachbarn[i + 1] / 2] == 0 ? e1 : e2);
                        }
                    } else if (nullAusdehnung || std::fabs(laengeQuadrat(ende - start)) > 1e-3) {
                        if (std::fabs(punkt(e1.normal(), test.blick().blickrichtung())) < std::fabs(punkt(e2.normal(), test.blick().blickrichtung()))) {
                            zuweisen(k.extents[kNachbarn[i] / 2] == 0 ? e2 : e1);
                        } else {
                            zuweisen(k.extents[kNachbarn[i + 1] / 2] == 0 ? e1 : e2);
                        }
                    }
                }
            }
        }
        grenzen = k;
    }
    // selectByPlane
    void perEbeneWaehlen(const Kasten& k, const Ebene3d& e) {
        const std::array<Ebene3d, 6> eb = kastenEbenen(k);
        for (std::size_t i = 0; i < 6; ++i) {
            if (ebenenGleich(e, eb[i]) || ebenenGleich(e, umgedreht(eb[i]))) {
                gewaehlt[i] = true;
                return;
            }
        }
    }
    // gatherPolygonsByPlane
    static void polygoneSammeln(const Kasten& k, const Ebene3d& e, std::vector<std::vector<Vektor3f>>& aus) {
        const std::array<Vektor3f, 8> ec = kastenEcken(k);
        const std::array<Ebene3d, 6> eb = kastenEbenen(k);
        for (std::size_t i = 0; i < 6; ++i) {
            if (ebenenGleich(e, eb[i]) || ebenenGleich(e, umgedreht(eb[i]))) {
                const std::size_t idx = i * 4;
                aus.push_back({ec[kKastenIndizes[idx]], ec[kKastenIndizes[idx + 1]], ec[kKastenIndizes[idx + 2]], ec[kKastenIndizes[idx + 3]]});
                return;
            }
        }
    }
    // evaluateResize / evaluateTransform
    [[nodiscard]] Kasten groesseBerechnen(const Vektor3f& v) const {
        Vektor3f mn = grenzen.origin - grenzen.extents;
        Vektor3f mx = grenzen.origin + grenzen.extents;
        for (std::size_t i = 0; i < 3; ++i) {
            if (grenzen.extents[i] != 0) {
                if (gewaehlt[i * 2]) { mx[i] += v[i]; }
                if (gewaehlt[i * 2 + 1]) { mn[i] += v[i]; }
            }
        }
        return Kasten(mitte(mn, mx), (mx - mn) * 0.5);
    }
    [[nodiscard]] Matrix4 transformBerechnen(const Vektor3f& v) const {
        const Kasten k = groesseBerechnen(v);
        const Vektor3f s(grenzen.extents[0] != 0 ? k.extents[0] / grenzen.extents[0] : 1, grenzen.extents[1] != 0 ? k.extents[1] / grenzen.extents[1] : 1,
                         grenzen.extents[2] != 0 ? k.extents[2] / grenzen.extents[2] : 1);
        Matrix4 m = verschiebungsMatrix(k.origin - grenzen.origin);
        skaliereUmPunkt(m, s, grenzen.origin);
        return m;
    }
    // line_closest_point( line, g_vector3_identity )
    static Vektor3f naechsterPunktAufLinieF(const Vektor3f& start, const Vektor3f& ende) {
        const Vektor3f v = ende - start;
        const Vektor3f w = Vektor3f(0, 0, 0) - start;
        const double c1 = punkt(w, v);
        if (c1 <= 0) { return start; }
        const double c2 = punkt(v, v);
        if (c2 <= c1) { return ende; }
        return start + v * (c1 / c2);
    }
};

void ZiehFlaechen::umgedrehteWaehlen(const Kasten& k, const std::vector<Ebene3d>& gewaehlteEbenen, std::vector<std::size_t>& seiten) const {
    const std::array<Ebene3d, 6> eb = kastenEbenen(k);
    for (std::size_t i = 0; i < 6; ++i) {
        const Ebene3d um = umgedreht(eb[i]);
        // SelectedPlaneSet::contains: std::set mit PlaneLess (genauer Vergleich)
        if (std::any_of(gewaehlteEbenen.begin(), gewaehlteEbenen.end(), [&](const Ebene3d& e) {
                return e.a == um.a && e.b == um.b && e.c == um.c && e.d == um.d;
            })) {
            seiten.push_back(i);
        }
    }
}

// --- selection_mtor_skew.cpp ---------------------------------------------------------------------

class VerzerrManipulator final : public Manipulator {
public:
    VerzerrManipulator(Dienste& d, Verzerrbar& verzerr, Verschiebbar& verschieb, Skalierbar& skalier, Drehbar& dreh,
                       AllesTransformierbar& alles, const Kasten& grenzen, Matrix4& pivotZuWelt, const bool& pivotEigen,
                       std::size_t segmente = 2)
        : m_d(d), m_verzerren(d, verzerr), m_verschieben(d, verschieb, alles), m_skaliereAchse(d, skalier), m_skaliereFrei(d, skalier),
          m_dreheAchse(d, dreh), m_grenzen(grenzen), m_pivotZuWelt(pivotZuWelt), m_pivotEigen(pivotEigen) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 2; ++j) {
                const auto x = static_cast<std::size_t>(i);
                const auto y = static_cast<std::size_t>((i + j + 1) % 3);
                Vektor3f& xy_ = m_linien[i][j][0][0];
                Vektor3f& x_y_ = m_linien[i][j][0][1];
                Vektor3f& xy = m_linien[i][j][1][0];
                Vektor3f& x_y = m_linien[i][j][1][1];
                xy = x_y = xy_ = x_y_ = Vektor3f(0, 0, 0);
                xy[x] = xy_[x] = 1;
                x_y[x] = x_y_[x] = -1;
                xy[y] = x_y[y] = 1;
                xy_[y] = x_y_[y] = -1;
            }
        }
        m_pfeil.resize(3 * 2 * (segmente << 3));
        pfeilKopf(segmente, 0, m_pfeil.data(), 0);
    }

    void modelviewAktualisieren(const Blick& v, const Matrix4& p2w) {   // updateModelview
        m_pivot.aktualisieren(verschiebungsMatrix(m_grenzen.origin), v.modelview(), v.projektion(), v.bildfeld());
        Matrix4& m = m_pivot.weltRaum;   // affin fuer mehr Genauigkeit
        m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = m[11] = 0;
        m[15] = 1;
        m_grenzenZeichnen = kastenFuerGedrehtenKasten(m_grenzen, affinInvers(m_pivot.weltRaum));   // Bildmassstab
        for (std::size_t i = 0; i < 3; ++i) {
            if (m_grenzenZeichnen.extents[i] < 16) {
                m_grenzenZeichnen.extents[i] = 18;
            } else {
                m_grenzenZeichnen.extents[i] += 2.0F;
            }
        }
        m_grenzenZeichnen = kastenFuerGedrehtenKasten(m_grenzenZeichnen, m_pivot.weltRaum);   // Weltmassstab
        m_grenzenZeichnen.origin = m_grenzen.origin;
        m_weltRaum = multipliziert(verschiebungsMatrix(m_grenzenZeichnen.origin), skalierungsMatrix(m_grenzenZeichnen.extents));
        vormultipliziere(m_weltRaum, verschiebungsMatrix(-p2w.t().vec3()));
        vormultipliziere(m_weltRaum, p2w);
    }

    void zeichnen(const Blick& v, const Matrix4& p2w, ManipZeichnung& aus) override {
        modelviewAktualisieren(v, p2w);
        // UpdateColours
        ManipFarbe farbe[3][2][2];
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) { farbe[i][j][k] = farbeGewaehlt(kFarbeBild, m_gewaehlt[i][j][k]); }
            }
        }
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 2; ++j) {
                if (m_skalierFlaechen.gewaehlt[i * 2 + j]) {
                    farbe[(i + 1) % 3][1][j ^ 1] = kFarbeGewaehlt;
                    farbe[(i + 2) % 3][0][j ^ 1] = kFarbeGewaehlt;
                }
            }
        }
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 2; ++j) {
                // die gewaehlte zuletzt, damit sie in 2D oben liegt
                const int erste = m_gewaehlt[i][j][0] ? 1 : 0;
                const int zweite = 1 - erste;
                linieDazu(aus.draht, m_weltRaum, m_linien[i][j][erste][0], m_linien[i][j][erste][1], farbe[i][j][erste]);
                linieDazu(aus.draht, m_weltRaum, m_linien[i][j][zweite][0], m_linien[i][j][zweite][1], farbe[i][j][zweite]);
            }
        }
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) {
                    if (!m_gewaehlt[i][j][k]) { continue; }
                    const Vektor3f ursprung = transformierterPunkt(m_weltRaum, m_linien[i][j][k][0]);
                    const Vektor3f ursprung2 = transformierterPunkt(m_weltRaum, m_linien[i][j][k][1]);
                    Matrix4 mv1;
                    Matrix4 mv2;
                    pivotWeltRaum(mv1, verschiebungsMatrix(ursprung), v.modelview(), v.projektion(), v.bildfeld());
                    pivotWeltRaum(mv2, verschiebungsMatrix(ursprung2), v.modelview(), v.projektion(), v.bildfeld());
                    const Matrix4 dreh = i == 0 ? kEinheitsMatrix : i == 1 ? drehungSinCosZ(1, 0) : drehungSinCosY(-1, 0);
                    multipliziere(mv1, dreh);
                    multipliziere(mv2, dreh);
                    const float x = 0.7F;
                    multipliziere(mv1, skalierungsMatrix(Vektor3f(x, x, x)));
                    multipliziere(mv2, skalierungsMatrix(Vektor3f(-x, x, x)));
                    dreieckeDazu(aus.flach, mv1, m_pfeil, kFarbeGewaehlt);
                    dreieckeDazu(aus.flach, mv2, m_pfeil, kFarbeGewaehlt);
                    return;
                }
            }
        }
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) {
                    if (m_drehGewaehlt[i][j][k]) {
                        const Vektor3f p = transformierterPunkt(m_weltRaum, m_punkt);
                        aus.grossePunkte.push_back(ManipZeichnung::Punkt{p, kFarbeGewaehlt});
                        aus.grossePunkte.push_back(ManipZeichnung::Punkt{p, kFarbeGewaehlt});
                        return;
                    }
                }
            }
        }
    }

    void testen(const Blick& v, const Matrix4& p2w) override {
        modelviewAktualisieren(v, p2w);
        ManipPool pool;
        const Matrix4 l2v = multipliziert(v.sicht(), m_weltRaum);
        const Zusatz& z = m_d.zusatzTasten();
        const bool nurFlaechen = z.genau(false, false, true) && v.fuellen();
        if (!nurFlaechen && !z.keine()) { return; }
        if (!nurFlaechen) {
            // Eckpunkte zum Drehen
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 2; ++j) {
                    for (int k = 0; k < 2; ++k) {
                        m_punkt[static_cast<std::size_t>(i)] = 0;
                        m_punkt[static_cast<std::size_t>((i + 1) % 3)] = j != 0 ? 1.0F : -1.0F;
                        m_punkt[static_cast<std::size_t>((i + 2) % 3)] = k != 0 ? 1.0F : -1.0F;
                        Treffer t;
                        punktBesterPunkt(l2v, m_punkt, t);
                        pool.dazu(t, &m_drehGewaehlt[i][j][k]);
                    }
                }
            }
            if (!pool.leer()) {
                *pool.menge().begin()->second = true;
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 2; ++j) {
                        for (int k = 0; k < 2; ++k) {
                            if (!m_drehGewaehlt[i][j][k]) { continue; }
                            m_punkt[static_cast<std::size_t>(i)] = 0;
                            m_punkt[static_cast<std::size_t>((i + 1) % 3)] = j != 0 ? 1.0F : -1.0F;
                            m_punkt[static_cast<std::size_t>((i + 2) % 3)] = k != 0 ? 1.0F : -1.0F;
                            if (!m_pivotEigen) {
                                const Vektor3f ursprung = m_grenzen.origin + komponentenMal(m_punkt * -1.0F, m_grenzen.extents);
                                m_pivotZuWelt = verschiebungsMatrix(ursprung);
                            }
                            // Radius
                            if (std::fabs(punkt(m_pivot.bildAchse, kAchsen[i])) < 0.2) {
                                Vektor3f ursprung = m_pivotZuWelt.t().vec3();
                                Vektor3f pk = m_grenzenZeichnen.origin + komponentenMal(m_punkt, m_grenzenZeichnen.extents);
                                const Matrix4 inv = affinInvers(m_pivot.weltRaum);
                                transformierePunkt(inv, ursprung);
                                transformierePunkt(inv, pk);
                                pk = pk - ursprung;
                                pk = pk + m_pivot.bildAchse * static_cast<float>(-punkt(pk, m_pivot.bildAchse));   // constrain_to_axis
                                // kleinerer Radius: in 2D nur eine Drehrichtung
                                m_dreheAchse.setzeRadius(static_cast<float>(laenge(pk) - m_d.waehlEpsilon() / 2.0 - 1.0));
                            } else {
                                m_dreheAchse.setzeRadius(kRadius);
                            }
                        }
                    }
                }
            } else {
                // Linien zum Verzerren
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 2; ++j) {
                        for (int k = 0; k < 2; ++k) {
                            Treffer t;
                            linieBesterPunkt(l2v, m_linien[i][j][k], t);
                            pool.dazu(t, &m_gewaehlt[i][j][k]);
                        }
                    }
                }
                if (!pool.leer()) {
                    *pool.menge().begin()->second = true;
                    m_verzerren.setze0(projiziert(transformierterVektor4(vollInvers(v.sicht()), Vektor4f(0, 0, pool.menge().begin()->first.tiefe(), 1))));
                    if (!m_pivotEigen) {
                        for (int i = 0; i < 3; ++i) {
                            for (int j = 0; j < 2; ++j) {
                                for (int k = 0; k < 2; ++k) {
                                    if (m_gewaehlt[i][j][k]) {
                                        const auto um = static_cast<std::size_t>((i + j + 1) % 3);
                                        Vektor3f ursprung = m_grenzen.origin;
                                        ursprung[um] += k != 0 ? -m_grenzen.extents[um] : m_grenzen.extents[um];
                                        m_pivotZuWelt = verschiebungsMatrix(ursprung);
                                    }
                                }
                            }
                        }
                    }
                } else {
                    // Kasten zum Verschieben
                    Treffer t;
                    kastenBesterPunkt(l2v, Wegschneiden::CW, Kasten(Vektor3f(0, 0, 0), Vektor3f(1, 1, 1)), t);
                    pool.dazu(t, &m_verschiebenGewaehlt);
                    if (!pool.leer()) {
                        m_verschieben.setze0(projiziert(transformierterVektor4(vollInvers(v.sicht()), Vektor4f(0, 0, pool.menge().begin()->first.tiefe(), 1))));
                    }
                }
            }
        }
        // testSelectBboxPlanes: Kastenseiten zum Skalieren
        if (pool.leer()) {
            AuswahlVolumen test(v);
            test.netzBeginnen(kEinheitsMatrix, true);
            if (z.genau(false, false, true)) {
                BesteEbene daten;
                m_skalierFlaechen.besteEbeneDirekt(m_grenzenZeichnen, test, daten);
                if (!daten.gueltig()) { m_skalierFlaechen.besteEbeneIndirekt(m_grenzenZeichnen, test, daten); }
                if (daten.gueltig()) { m_skalierFlaechen.perEbeneWaehlen(m_grenzenZeichnen, daten.ebene); }
            } else {
                std::vector<std::size_t> seiten;
                std::vector<Ebene3d> ebenen;
                m_skalierFlaechen.ebenenWaehlen(m_grenzenZeichnen, test, seiten, ebenen);
                for (const std::size_t s : seiten) { m_skalierFlaechen.gewaehlt[s] = true; }
            }
            Vektor3f ursprung = m_grenzen.origin;
            for (std::size_t i = 0; i < 3; ++i) {
                for (std::size_t j = 0; j < 2; ++j) {
                    if (m_skalierFlaechen.gewaehlt[i * 2 + j]) { ursprung[i] += j != 0 ? m_grenzen.extents[i] : -m_grenzen.extents[i]; }
                }
            }
            if (!m_pivotEigen) { m_pivotZuWelt = verschiebungsMatrix(ursprung); }
            return;
        }
        auswahlWechsel(pool);
    }

    Manipulierbar* manipulierbar() override {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) {
                    if (m_gewaehlt[i][j][k]) {
                        m_verzerren.setzeAchsen(i, (i + j + 1) % 3, k != 0 ? 1 : -1);
                        return &m_verzerren;
                    }
                    if (m_drehGewaehlt[i][j][k]) {
                        m_dreheAchse.setzeAchse(kAchsen[i]);
                        return &m_dreheAchse;
                    }
                }
            }
        }
        Vektor3f achsen[2] = {Vektor3f(0, 0, 0), Vektor3f(0, 0, 0)};
        std::size_t anzahl = 0;
        for (std::size_t i = 0; i < 3; ++i) {
            for (std::size_t j = 0; j < 2; ++j) {
                if (m_skalierFlaechen.gewaehlt[i * 2 + j] && anzahl < 2) { achsen[anzahl++][i] = j != 0 ? -1.0F : 1.0F; }
            }
        }
        if (anzahl == 2) {
            m_skaliereFrei.setzeAchsen(achsen[0], achsen[1]);
            return &m_skaliereFrei;
        }
        if (anzahl == 1) {
            m_skaliereAchse.setzeAchse(achsen[0]);
            return &m_skaliereAchse;
        }
        return &m_verschieben;
    }
    void setzeGewaehlt(bool an) override {
        m_verschiebenGewaehlt = an;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) {
                    m_gewaehlt[i][j][k] = an;
                    m_drehGewaehlt[i][j][k] = an;
                }
            }
        }
        m_skalierFlaechen.setze(an);
    }
    [[nodiscard]] bool gewaehlt() const override {
        bool g = false;
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) { g = g || m_gewaehlt[i][j][k] || m_drehGewaehlt[i][j][k]; }
            }
        }
        return g || m_skalierFlaechen.irgendeine() || m_verschiebenGewaehlt;
    }

private:
    Dienste& m_d;
    VerzerreAchse m_verzerren;
    VerschiebeFreiXyZ m_verschieben;
    SkaliereAchse m_skaliereAchse;
    SkaliereFrei m_skaliereFrei;
    DreheAchse m_dreheAchse;
    Kasten m_grenzenZeichnen;
    const Kasten& m_grenzen;
    Matrix4& m_pivotZuWelt;
    const bool& m_pivotEigen;
    Vektor3f m_linien[3][2][2][2];
    bool m_gewaehlt[3][2][2] = {};        // [X][YZ][-+]
    bool m_verschiebenGewaehlt = false;
    ZiehFlaechen m_skalierFlaechen;        // +-X +-Y +-Z
    bool m_drehGewaehlt[3][2][2] = {};    // [X][-+Y][-+Z]
    PivotZuWelt m_pivot;
    Matrix4 m_weltRaum;
    std::vector<FlachPunkt> m_pfeil;
    Vektor3f m_punkt;
};

// --- BuildManipulator (selection.cpp:742-794) -----------------------------------------------------

class BauManipulator final : public Manipulator, public Manipulierbar {
public:
    explicit BauManipulator(Dienste& d) : Manipulierbar(d) {}
    void zeichnen(const Blick&, const Matrix4&, ManipZeichnung& aus) override {
        aus.grossePunkte.push_back(ManipZeichnung::Punkt{Vektor3f(0, 0, 0), kFarbeGewaehlt});
        aus.draht.push_back(ManipZeichnung::Linie{Vektor3f(0, 0, 0), Vektor3f(0, 0, 0), kFarbeGewaehlt});
        aus.draht.push_back(ManipZeichnung::Linie{Vektor3f(0, 0, 0), Vektor3f(0, 0, 0), kFarbeBild});
    }
    void hervorheben(const Blick&, const Matrix4&) override {}
    void testen(const Blick&, const Matrix4&) override { m_gewaehlt = true; }
    void bauen(const Matrix4&, const GeraeteVektor&, const Kasten&, const Vektor3f&) override {}
    void transformieren(const Matrix4&, const Matrix4&, const GeraeteVektor&) override {}
    Manipulierbar* manipulierbar() override {
        m_gewaehlt = false;   // MoveSelected nicht ausfuehren
        return this;
    }
    void setzeGewaehlt(bool an) override { m_gewaehlt = an; }
    [[nodiscard]] bool gewaehlt() const override { return m_gewaehlt; }

private:
    bool m_gewaehlt = false;
};

// Clipper und UV sind noch nicht portiert: greift nie.
class LeerManipulator final : public Manipulator, public Manipulierbar {
public:
    explicit LeerManipulator(Dienste& d) : Manipulierbar(d) {}
    void testen(const Blick&, const Matrix4&) override {}
    void bauen(const Matrix4&, const GeraeteVektor&, const Kasten&, const Vektor3f&) override {}
    void transformieren(const Matrix4&, const Matrix4&, const GeraeteVektor&) override {}
    Manipulierbar* manipulierbar() override { return this; }
    void setzeGewaehlt(bool) override {}
    [[nodiscard]] bool gewaehlt() const override { return false; }
};

// --- TransformOriginManipulator (selection.cpp:846-916) ----------------------------------------------

class UrsprungManipulator final : public Manipulator {
public:
    UrsprungManipulator(Dienste& d, UrsprungVerschiebbar& ziel, const bool& pivotEigen) : m_d(d), m_verschieben(d, ziel), m_pivotEigen(pivotEigen) {}
    void zeichnen(const Blick& v, const Matrix4& p2w, ManipZeichnung& aus) override {
        m_pivot.aktualisieren(p2w, v.modelview(), v.projektion(), v.bildfeld());
        const ManipFarbe f = m_gewaehlt ? (m_pivotEigen ? ManipFarbe{255, 232, 0, 255} : kFarbeGewaehlt)
                                        : (m_pivotEigen ? ManipFarbe{0, 125, 255, 255} : kFarbeBild);
        aus.grossePunkte.push_back(ManipZeichnung::Punkt{transformierterPunkt(m_pivot.weltRaum, Vektor3f(0, 0, 0)), f});
    }
    void testen(const Blick& v, const Matrix4& p2w) override {
        if (!m_d.zusatzTasten().keine()) { return; }
        m_pivot.aktualisieren(p2w, v.modelview(), v.projektion(), v.bildfeld());
        ManipPool pool;
        Treffer t;
        punktBesterPunkt(multipliziert(v.sicht(), m_pivot.weltRaum), Vektor3f(0, 0, 0), t);
        pool.dazu(t, &m_gewaehlt);
        auswahlWechsel(pool);
    }
    Manipulierbar* manipulierbar() override { return &m_verschieben; }
    void setzeGewaehlt(bool an) override { m_gewaehlt = an; }
    [[nodiscard]] bool gewaehlt() const override { return m_gewaehlt; }

private:
    Dienste& m_d;
    UrsprungVerschieben m_verschieben;
    const bool& m_pivotEigen;
    bool m_gewaehlt = false;
    PivotZuWelt m_pivot;
};

// --- Punktentitaeten (plugins/entity) ------------------------------------------------------------

enum class EntArt { Generisch, Modell, MiscModell, Licht };   // entity_for_eclass
EntArt entitaetsArt(const Entitaet& e) {
    const EntitaetsKlasse* k = e.klasse();
    if (k != nullptr && k->miscmodelIs) { return EntArt::MiscModell; }
    if (istLichtEntitaet(e)) { return EntArt::Licht; }
    if (k != nullptr && !k->modell.empty()) { return EntArt::Modell; }
    return EntArt::Generisch;
}

Vektor3f winkelLesen(const Entitaet& e) { return entitaetsWinkel(e.wert("angle"), e.wert("angles"), e.hat("angles"), false); }

// ScaleKey (scale.h): modelscale_vec, sonst modelscale
Vektor3f skalierungLesen(const Entitaet& e) {
    if (e.hat("modelscale_vec")) {   // read_scalevec
        float a = 0;
        float b = 0;
        float c = 0;
        if (std::sscanf(e.wert("modelscale_vec").c_str(), "%f %f %f", &a, &b, &c) != 3 || a == 0 || b == 0 || c == 0) { return Vektor3f(1, 1, 1); }
        return Vektor3f(a, b, c);
    }
    float s = 0;   // read_scale
    char rest = 0;
    const std::string w = e.wert("modelscale");
    if (w.empty() || std::sscanf(w.c_str(), "%f%c", &s, &rest) != 1 || s == 0) { return Vektor3f(1, 1, 1); }
    return Vektor3f(s, s, s);
}

// write_origin
void ursprungSchreiben(Entitaet& e, const Vektor3f& o) { e.setzen("origin", zahlText(o[0]) + ' ' + zahlText(o[1]) + ' ' + zahlText(o[2])); }

// write_angle / write_angles (angle.h, angles.h)
void winkelSchreiben(Entitaet& e, const Vektor3f& w) {
    if (w == Vektor3f(0, 0, 0)) {
        e.setzen("angle", "");
        e.setzen("angles", "");
    } else if (w[0] == 0 && w[1] == 0) {
        const float gier = w[2];
        e.setzen("angles", "");
        e.setzen("angle", gier == 0 ? std::string() : zahlText(gier));
    } else {
        e.setzen("angle", "");
        e.setzen("angles", zahlText(w[1]) + ' ' + zahlText(w[2]) + ' ' + zahlText(w[0]));   // g_stupidQuakeBug aus
    }
}

// write_scale
void skalierungSchreiben(Entitaet& e, const Vektor3f& s) {
    if (s[0] == 1 && s[1] == 1 && s[2] == 1) {
        e.setzen("modelscale", "");
        e.setzen("modelscale_vec", "");
    } else if (s[0] == s[1] && s[0] == s[2]) {
        e.setzen("modelscale_vec", "");
        e.setzen("modelscale", zahlText(s[0]));
    } else {
        e.setzen("modelscale", "");
        e.setzen("modelscale_vec", zahlText(s[0]) + ' ' + zahlText(s[1]) + ' ' + zahlText(s[2]));
    }
}

// localToWorld einer Punktentitaet (updateTransform der Entitaetsarten)
Matrix4 entitaetLokalZuWelt(const Entitaet& e) {
    const Vektor3f o = entitaetUrsprung(e);
    switch (entitaetsArt(e)) {
    case EntArt::Modell: return multipliziert(verschiebungsMatrix(o), drehungEulerXyzGrad(winkelLesen(e)));
    case EntArt::MiscModell: {   // matrix4_transform_by_euler_xyz_degrees
        Matrix4 m = verschiebungsMatrix(o);
        multipliziere(m, drehungEulerXyzGrad(winkelLesen(e)));
        skaliereUm(m, skalierungLesen(e));
        return m;
    }
    case EntArt::Licht:   // localToWorld Einheit, getLocalPivot [Drehung | Ursprung]
    case EntArt::Generisch: break;
    }
    return verschiebungsMatrix(o);
}

// translation_for_pivoted_matrix_transform (selection.cpp:214-257)
Vektor3f gepivotet(const Matrix4& lokal, const Vektor3f& weltPivot, const Matrix4& lokalZuWelt, const Matrix4& lokalZuEltern) {
    const Vektor3f lp = transformierterPunkt(vollInvers(lokalZuWelt), weltPivot);
    const Vektor3f lt = lp - transformierterPunkt(lokal, lp);
    return translationLokalZuObjekt(lt, lokalZuEltern);
}

template<typename T>
bool zeigerLeer(const std::weak_ptr<T>& w) {   // nie belegt (nicht nur abgelaufen)
    const std::weak_ptr<T> leer;
    return !w.owner_before(leer) && !leer.owner_before(w);
}

// line_closest_point (DoubleLine)
Vektor3d naechsterPunktD(const Vektor3d& start, const Vektor3d& ende, const Vektor3d& ziel) {
    const Vektor3d v = ende - start;
    const Vektor3d w = ziel - start;
    const double c1 = punkt(w, v);
    if (c1 <= 0) { return start; }
    const double c2 = punkt(v, v);
    if (c2 <= c1) { return ende; }
    return start + v * (c1 / c2);
}

}  // namespace

// --- RadiantSelectionSystem -------------------------------------------------------------------------

enum class TArt { Verschieben, Drehen, Skalieren, Verzerren, Alles };

struct AuswahlBearbeitung::Impl final : Dienste, Verschiebbar, Drehbar, Skalierbar, Verzerrbar, AllesTransformierbar, UrsprungVerschiebbar {
    Impl(Karte& k, AuswahlBearbeitung& a)
        : karte(k), aussen(a), verschiebeM(std::make_unique<VerschiebeManipulator>(*this, *this, 2, 64.0F)),
          drehM(std::make_unique<DrehManipulator>(*this, *this, 8, 64.0F)), skalierM(std::make_unique<SkalierManipulator>(*this, *this, 64.0F)),
          verzerrM(std::make_unique<VerzerrManipulator>(*this, *this, *this, *this, *this, *this, grenzenWert, pivot, pivotEigen)),
          ziehM(neuerZiehManipulator()), bauM(std::make_unique<BauManipulator>(*this)), leerM(std::make_unique<LeerManipulator>(*this)),
          ursprungM(std::make_unique<UrsprungManipulator>(*this, *this, pivotEigen)) {
        modusSetzen(ManipModus::Verschieben);
    }
    std::unique_ptr<Manipulator> neuerZiehManipulator();

    // --- Dienste ---
    [[nodiscard]] const Zusatz& zusatzTasten() const override { return aussen.zusatz; }
    [[nodiscard]] float rasterGroesse() const override { return aussen.raster; }
    [[nodiscard]] float fangGroesse() const override { return aussen.fang; }
    [[nodiscard]] bool primitivModus() const override { return aussen.einstellung.modus == AuswahlModus::Primitiv; }
    [[nodiscard]] float waehlEpsilon() const override { return aussen.waehlEpsilon; }
    [[nodiscard]] Kasten grenzenMitEntitaeten() const override { return auswahlGrenzenMitEntitaeten(karte); }
    [[nodiscard]] std::optional<SzenenPunkt> ungewaehlterSzenenPunkt(const Blick& v, const GeraeteVektor& p, const GeraeteVektor& e) const override;
    void quader(std::shared_ptr<Pinsel>& pinsel, const Kasten& k) override { aussen.quaderGroesse(pinsel, k); }

    // --- Translatable usw. (selection.cpp:1583-1734) ---
    void verschieben(const Vektor3f& v) override;
    void drehen(const Quaternion& q) override;
    void skalieren(const Vektor3f& s) override;
    void verzerren(const Verzerrung& v) override;
    void allesTransformieren(const Transformationen& t, const Vektor3f& weltPivot) override;
    void ursprungVerschieben(const Vektor3f& v, const bool gesetzt[3]) override;   // transformOriginTranslate

    void modusSetzen(ManipModus m);
    [[nodiscard]] bool etwasGewaehlt() const {   // somethingSelected
        return (aussen.einstellung.modus == AuswahlModus::Komponente && !karte.auswahl.flaechen().empty()) ||
               (aussen.einstellung.modus == AuswahlModus::Primitiv && karte.auswahl.anzahl() != 0);
    }
    [[nodiscard]] bool ursprungVerschiebbar() const {   // transformOrigin_isTranslatable
        return modus == ManipModus::Skalieren || modus == ManipModus::Verzerren || modus == ManipModus::Drehen || modus == ManipModus::Verschieben;
    }
    void signaturPruefen();
    void pivotBauen();   // ConstructPivot
    const Matrix4& pivotZuWelt() {
        pivotBauen();
        return pivot;
    }
    void bewegungBeginnen() { pivotStart = pivotZuWelt(); }   // startMove
    [[nodiscard]] Kasten auswahlKasten();   // getSelectionAABB
    void eigenenUrsprungSetzen(const Vektor3f& u, const bool gesetzt[3]);   // setCustomTransformOrigin
    void alleKomponentenAbwaehlen();   // setSelectedAllComponents( false )
    void festschreiben() { sicherungen.clear(); }   // freezeTransforms

    // Anwenden (revertTransform + evaluateTransform)
    struct Objekt {
        std::shared_ptr<Entitaet> ent;
        std::shared_ptr<Primitiv> prim;   // leer: Punktentitaet
    };
    [[nodiscard]] std::vector<Objekt> gewaehlteObjekte() const;
    void vorAenderung(const std::shared_ptr<Rueckgaengigbar>& o, Pinsel* besitzer);
    void zuruecksetzen();
    void primitiveAnwenden(TArt art, const Transformationen& t, const Vektor3f& weltPivot);
    void komponentenAnwenden(TArt art, const Transformationen& t, const Vektor3f& weltPivot);
    void entitaetTransformieren(Entitaet& e, const Vektor3f& tObj, const Transformationen& t);

    // Ebenenwahl (selection_mtor_drag.cpp)
    bool ebenenWaehlen(AuswahlVolumen& test);   // Scene_forEachPlaneSelectable_selectPlanes
    BesteEbene besteEbene(AuswahlVolumen& test);   // Scene_forEachPlaneSelectable_bestPlane
    bool ebenenWaehlen2(AuswahlVolumen& test, VerschiebeAchse2& achse);   // Scene_forEachPlaneSelectable_selectPlanes2
    void polygoneSammeln(AuswahlVolumen& test, std::vector<std::vector<Vektor3f>>& aus);

    Karte& karte;
    AuswahlBearbeitung& aussen;
    ManipModus modus = ManipModus::Verschieben;
    Kasten grenzenWert;               // m_bounds
    Matrix4 pivot = kEinheitsMatrix;  // m_pivot2world
    Matrix4 pivotStart = kEinheitsMatrix;
    Matrix4 manipZuPivotStart = kEinheitsMatrix;
    bool pivotEigen = false;          // m_pivotIsCustom
    bool pivotWechsel = true;         // m_pivotChanged
    bool pivotBewegt = false;         // m_pivot_moving
    bool rueckgaengigBegonnen = false;   // m_undo_begun
    bool tmpKomponenten = false;      // g_bTmpComponentMode
    Vektor3f verschiebung{0, 0, 0};   // m_translation
    Quaternion drehung = kQuaternionEins;
    Vektor3f skalierung{1, 1, 1};
    Verzerrung verzerrung;
    std::unique_ptr<Manipulator> verschiebeM, drehM, skalierM, verzerrM, ziehM, bauM, leerM, ursprungM;
    Manipulator* manip = nullptr;
    struct PatchEbenen {
        std::shared_ptr<Patch> patch;
        ZiehFlaechen flaechen;
    };
    std::map<const Patch*, PatchEbenen> patchEbenen;   // PatchInstance::m_dragPlanes
    struct Sicherung {
        std::shared_ptr<Rueckgaengigbar> objekt;
        std::unique_ptr<Memento> zustand;
        Pinsel* besitzer = nullptr;
        bool veraendert = false;
    };
    std::map<const Rueckgaengigbar*, Sicherung> sicherungen;
    std::size_t signatur = 0;
    // NewBrushDrag
    bool neuerPinselAktiv = false;
    std::shared_ptr<Pinsel> neuerPinsel;
    Vektor3f neuerPinselStart;
};

// --- DragManipulator (selection_mtor_drag.cpp) ---------------------------------------------------------

namespace {

class ResizeVerschiebbar final : public Verschiebbar {   // ResizeTranslatable
public:
    explicit ResizeVerschiebbar(AuswahlBearbeitung::Impl& u) : m_u(u) {}
    void verschieben(const Vektor3f& v) override {   // Scene_Translate_Component_Selected
        if (m_u.karte.auswahl.anzahl() == 0) { return; }
        Transformationen t;
        t.verschiebung = v;
        m_u.komponentenAnwenden(TArt::Verschieben, t, Vektor3f(0, 0, 0));
    }

private:
    AuswahlBearbeitung::Impl& m_u;
};

class ZiehManipulator final : public Manipulator {
public:
    explicit ZiehManipulator(AuswahlBearbeitung::Impl& u)
        : m_u(u), m_resize(u), m_freiResize(u, m_resize), m_achseResize(u, m_resize), m_freiXyZ(u, u, u), m_neuerPinsel(u) {
        m_kreis.resize(2 << 3);
        kreisPunkte(m_kreis.size() >> 3, 5, m_kreis.data(), Umordnung::XYZ);
        setzeGewaehlt(false);
    }
    Manipulierbar* manipulierbar() override {
        if (m_neu) { return &m_neuerPinsel; }
        if (m_gewaehlt) { return &m_freiResize; }
        if (m_gewaehlt2) { return &m_achseResize; }
        return &m_freiXyZ;
    }
    void testen(const Blick& v, const Matrix4&) override {
        Karte& k = m_u.karte;
        const Zusatz& z = m_u.zusatzTasten();
        const AuswahlEinstellung& e = m_u.aussen.einstellung;
        AuswahlVolumen test(v);
        std::vector<Waehlbar> wahl;   // SelectionPool selector
        if (z.genau(false, true, true) && e.modus == AuswahlModus::Primitiv && (k.auswahl.anzahl() != 0 || !k.auswahl.flaechen().empty())) {
            // Extrudieren (DragExtrudeFaces): noch nicht portiert
            m_extrudieren = false;
        } else if (k.auswahl.anzahl() != 0) {
            if (e.modus == AuswahlModus::Primitiv) {
                if (z.genau(false, false, true)) {
                    if (v.fuellen()) {   // Alt: Groesse entlang der Achse
                        m_gewaehlt2 = m_u.ebenenWaehlen2(test, m_achseResize);
                    }
                    // Alt in 2D: Ecken ziehen (Komponenten) - Stufe 2
                } else if (z.keine()) {
                    JaNeinWaehler jn(k);
                    szeneTestenPrimitive(k, jn, test);
                    if (jn.getroffen()) {   // ein Primitiv getroffen: ziehen
                        m_ziehen = true;
                        test.netzBeginnen(kEinheitsMatrix, true);
                        m_freiXyZ.setze0(projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, jn.bester().tiefe(), 1))));
                    } else {   // nichts getroffen: Flaechen der Ebenen waehlen
                        m_gewaehlt = m_u.ebenenWaehlen(test);
                    }
                }
            } else if (z.keine()) {   // Komponenten
                BesterWaehler bester;
                szeneTestenKomponenten(k, bester, test, e.komponente, true);
                for (const Waehlbar& s : bester.beste()) {
                    if (!istGewaehlt(k, s)) { m_u.alleKomponentenAbwaehlen(); }
                    wahl.push_back(s);
                    m_ziehen = true;
                }
                if (bester.besterTreffer().gueltig()) {
                    test.netzBeginnen(kEinheitsMatrix, true);
                    m_freiXyZ.setze0(projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, bester.besterTreffer().tiefe(), 1))));
                } else if (!k.auswahl.flaechen().empty()) {   // auch ohne Treffer ziehen
                    m_ziehen = true;
                    m_freiXyZ.setze0(Vektor3f(0, 0, 0));
                }
                // Ecken einfuegen (eVertex): Stufe 2
            }
            for (const Waehlbar& w : wahl) { bhed::karte::setzeGewaehlt(k, w, true); }
            m_u.tmpKomponenten = m_gewaehlt || m_gewaehlt2;
        } else if (e.modus == AuswahlModus::Primitiv && m_u.aussen.pinselIn3d && z.keine()) {
            m_neu = true;
            BesterPunktWaehler bp;
            szeneTestenPrimitive(k, bp, test);
            Vektor3f start;
            test.netzBeginnen(kEinheitsMatrix, true);
            if (bp.getroffen()) {
                start = projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, bp.bester().tiefe(), 1)));
            } else {
                const Vektor3f nah = projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, -1, 1)));
                const Vektor3f fern = projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, 1, 1)));
                start = normiert(fern - nah) * static_cast<float>(256.F + m_u.rasterGroesse() * std::sqrt(3.0)) + nah;
            }
            einrasten(start, m_u.fangGroesse());
            m_neuerPinsel.setze0(start);
        }
    }
    void setzeGewaehlt(bool an) override { m_ziehen = m_gewaehlt = m_gewaehlt2 = m_neu = m_extrudieren = an; }
    [[nodiscard]] bool gewaehlt() const override { return m_ziehen || m_gewaehlt || m_gewaehlt2 || m_neu || m_extrudieren; }
    void zeichnen(const Blick& v, const Matrix4&, ManipZeichnung& aus) override {
        if (m_polygone.empty()) { return; }
        if (m_polygone.back().size() == 1) {
            Matrix4 m;
            pivotBlickebenenRaum(m, verschiebungsMatrix(m_polygone.back()[0]), v.modelview(), v.projektion(), v.bildfeld());
            schleifeDazu(aus.ebenenLinien, m, m_kreis.data(), m_kreis.size(), kFarbeGewaehlt);
        } else {
            for (const auto& p : m_polygone) { aus.ebenen.push_back(p); }
        }
    }
    void hervorheben(const Blick& v, const Matrix4&) override {
        Karte& k = m_u.karte;
        const Zusatz& z = m_u.zusatzTasten();
        AuswahlVolumen test(v);
        std::vector<std::vector<Vektor3f>> polygone;
        // Bedingungen wie in testen()
        if (z.genau(false, true, true) && m_u.aussen.einstellung.modus == AuswahlModus::Primitiv &&
            (k.auswahl.anzahl() != 0 || !k.auswahl.flaechen().empty())) {
            // Extrudieren: noch nicht portiert
        } else if (k.auswahl.anzahl() != 0) {
            if (m_u.aussen.einstellung.modus == AuswahlModus::Primitiv) {
                if (z.genau(false, false, true) && v.fuellen()) { m_u.polygoneSammeln(test, polygone); }
            }
            // Komponenten-Hervorhebung (gatherComponentsHighlight): Stufe 2
        }
        m_polygone.swap(polygone);
    }

private:
    AuswahlBearbeitung::Impl& m_u;
    ResizeVerschiebbar m_resize;
    VerschiebeFrei m_freiResize;
    VerschiebeAchse2 m_achseResize;
    VerschiebeFreiXyZ m_freiXyZ;
    NeuerPinsel m_neuerPinsel;
    bool m_ziehen = false;        // m_dragSelected
    bool m_gewaehlt = false;      // m_selected: Komponenten zum Ziehen gewaehlt
    bool m_gewaehlt2 = false;     // m_selected2: Ebenen in der Kamera mit Alt
    bool m_neu = false;           // m_newBrush
    bool m_extrudieren = false;   // m_extrudeFaces
    std::vector<std::vector<Vektor3f>> m_polygone;
    std::vector<Vektor3f> m_kreis;
};

}  // namespace

std::unique_ptr<Manipulator> AuswahlBearbeitung::Impl::neuerZiehManipulator() { return std::make_unique<ZiehManipulator>(*this); }

void AuswahlBearbeitung::Impl::modusSetzen(ManipModus m) {   // SetManipulatorMode
    pivotEigen = false;
    modus = m;
    switch (m) {
    case ManipModus::Verschieben: manip = verschiebeM.get(); break;
    case ManipModus::Drehen: manip = drehM.get(); break;
    case ManipModus::Skalieren: manip = skalierM.get(); break;
    case ManipModus::Verzerren: manip = verzerrM.get(); break;
    case ManipModus::Ziehen: manip = ziehM.get(); break;
    case ManipModus::Bauen: manip = bauM.get(); break;
    case ManipModus::Clipper:
    case ManipModus::UV: manip = leerM.get(); break;
    }
    pivotWechsel = true;
}

// pivotChanged ruft NRC bei jeder Auswahl- und Grenzaenderung und beim
// Rasterwechsel. Hier: eine Kennung aus Auswahl, Aenderungszaehler, Raster und
// Modus.
void AuswahlBearbeitung::Impl::signaturPruefen() {
    std::size_t h = karte.undo.aenderungen();
    const auto mischen = [&h](std::size_t v) { h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2); };
    mischen(karte.auswahl.anzahl());
    for (const auto& el : karte.auswahl.elemente()) {
        mischen(reinterpret_cast<std::uintptr_t>(el.entitaet.lock().get()));
        mischen(reinterpret_cast<std::uintptr_t>(el.primitiv.lock().get()));
    }
    mischen(karte.auswahl.flaechen().size());
    for (const auto& f : karte.auswahl.flaechen()) { mischen(reinterpret_cast<std::uintptr_t>(f.flaeche.lock().get())); }
    for (const auto& [p, pe] : patchEbenen) {
        for (const bool b : pe.flaechen.gewaehlt) { mischen(b ? 1U : 2U); }
    }
    mischen(static_cast<std::size_t>(std::lround(aussen.fang * 1024.0F)));
    mischen(static_cast<std::size_t>(aussen.einstellung.modus));
    mischen(tmpKomponenten ? 1U : 0U);
    if (h != signatur) {
        signatur = h;
        pivotWechsel = true;
    }
}

void AuswahlBearbeitung::Impl::pivotBauen() {   // ConstructPivot
    signaturPruefen();
    if (!pivotWechsel || pivotBewegt) { return; }
    pivotWechsel = false;
    if (etwasGewaehlt()) {
        grenzenWert = auswahlKasten();
        if (!pivotEigen) {
            pivot = verschiebungsMatrix(grenzenWert.origin);
        } else {   // matrix4_assign_rotation( m_pivot2world, g_matrix4_identity )
            pivot[0] = 1;
            pivot[1] = 0;
            pivot[2] = 0;
            pivot[4] = 0;
            pivot[5] = 1;
            pivot[6] = 0;
            pivot[8] = 0;
            pivot[9] = 0;
            pivot[10] = 1;
        }
        // ConstructPivotRotation: mit SELECTIONSYSTEM_AXIAL_PIVOTS nichts
    }
}

// getSelectionAABB (selection.cpp:2268-2282) mit Scene_BoundsSelectedComponent
Kasten AuswahlBearbeitung::Impl::auswahlKasten() {
    Kasten k;
    if (!etwasGewaehlt()) { return k; }
    if (aussen.einstellung.modus == AuswahlModus::Komponente || tmpKomponenten) {
        for (const auto& fw : karte.auswahl.flaechen()) {   // FaceInstance::iterate_selected: Schwerpunkt (nur gewaehlte Pinsel)
            const auto p = fw.pinsel.lock();
            const auto f = fw.flaeche.lock();
            if (!p || !f) { continue; }
            const auto ent = std::find_if(karte.entitaeten().begin(), karte.entitaeten().end(), [&](const auto& e) {
                return std::any_of(e->kinder().begin(), e->kinder().end(), [&](const auto& c) { return c.get() == p.get(); });
            });
            if (ent == karte.entitaeten().end() || !karte.auswahl.gewaehlt(ent->get(), p.get())) { continue; }
            p->bRep(karte.kontext);
            kastenErweiternSicher(k, f->schwerpunkt());   // SelectedFaces_foreach: centroid()
        }
        // PlaneSelectables setzen g_bTmpComponentMode, aber nur Pinsel liefern Komponentengrenzen
        if (!kastenGueltig(k)) { k = auswahlGrenzenPivot(karte); }
    } else {
        k = auswahlGrenzenPivot(karte);
    }
    return k;
}

void AuswahlBearbeitung::Impl::eigenenUrsprungSetzen(const Vektor3f& u, const bool gesetzt[3]) {   // setCustomTransformOrigin
    if (!etwasGewaehlt() || !ursprungVerschiebbar()) { return; }
    const float f = aussen.fang;
    for (std::size_t i = 0; i < 3; ++i) {
        float wert = u[i];
        if (!gesetzt[i]) { continue; }
        float besterAbstand = std::fabs(grenzenWert.origin[i] - wert);
        float bestesZiel = grenzenWert.origin[i];
        float abstand = std::fabs(grenzenWert.origin[i] + grenzenWert.extents[i] - wert);
        if (abstand < besterAbstand) {
            besterAbstand = abstand;
            bestesZiel = grenzenWert.origin[i] + grenzenWert.extents[i];
        }
        abstand = std::fabs(grenzenWert.origin[i] - grenzenWert.extents[i] - wert);
        if (abstand < besterAbstand) {
            besterAbstand = abstand;
            bestesZiel = grenzenWert.origin[i] - grenzenWert.extents[i];
        }
        abstand = std::fabs(zahlEingerastet(wert, f) - wert);
        if (abstand < besterAbstand) { bestesZiel = zahlEingerastet(wert, f); }
        wert = bestesZiel;
        pivot[i + 12] = wert;
    }
    pivotEigen = true;
}

void AuswahlBearbeitung::Impl::alleKomponentenAbwaehlen() {   // setSelectedAllComponents( false )
    karte.auswahl.flaechenLeeren();
    for (auto& [p, pe] : patchEbenen) { pe.flaechen.setze(false); }
    patchEbenen.clear();
    manip->setzeGewaehlt(false);
}

std::vector<AuswahlBearbeitung::Impl::Objekt> AuswahlBearbeitung::Impl::gewaehlteObjekte() const {
    std::vector<Objekt> r;
    for (const auto& el : karte.auswahl.elemente()) {
        auto ent = el.entitaet.lock();
        if (!ent) { continue; }
        if (!zeigerLeer(el.primitiv)) {
            if (auto p = el.primitiv.lock()) { r.push_back(Objekt{ent, p}); }
        } else if (!ent->istBehaelter()) {
            r.push_back(Objekt{ent, nullptr});
        } else {   // Gruppe als Ganzes: ihre Primitive (NRC: origin der Gruppe)
            for (const auto& p : ent->kinder()) { r.push_back(Objekt{ent, p}); }
        }
    }
    return r;
}

void AuswahlBearbeitung::Impl::vorAenderung(const std::shared_ptr<Rueckgaengigbar>& o, Pinsel* besitzer) {
    Sicherung& s = sicherungen[o.get()];
    if (!s.objekt) {
        s.objekt = o;
        s.zustand = o->zustandSichern();
        s.besitzer = besitzer;
    }
    s.veraendert = true;
}

void AuswahlBearbeitung::Impl::zuruecksetzen() {   // revertTransform
    std::set<Pinsel*> pinsel;
    for (auto& [schluessel, s] : sicherungen) {
        (void)schluessel;
        if (!s.veraendert) { continue; }
        s.objekt->zustandLaden(*s.zustand);
        s.veraendert = false;
        if (s.besitzer != nullptr) { pinsel.insert(s.besitzer); }
    }
    for (Pinsel* p : pinsel) { p->geaendert(); }
}

void AuswahlBearbeitung::Impl::entitaetTransformieren(Entitaet& e, const Vektor3f& tObj, const Transformationen& t) {
    Transformationen pruef = t;
    pruef.verschiebung = tObj;
    if (pruef.eins()) { return; }   // TransformModifier::freezeTransform: nur wenn nicht Einheit
    const EntArt art = entitaetsArt(e);
    const Vektor3f ursprung = entitaetUrsprung(e);
    const Vektor3f winkelAlt = winkelLesen(e);
    const Vektor3f skalierungAlt = skalierungLesen(e);
    vorAenderung(std::static_pointer_cast<Rueckgaengigbar>(e.shared_from_this()), nullptr);
    // evaluateTransform: translate, rotate, scale
    const Vektor3f neuUrsprung = eingerastet(ursprung + tObj, 1e-4);   // origin_translated
    Vektor3f winkel = winkelAlt;
    Vektor3f s = skalierungAlt;
    const bool drehen = !(t.drehung[0] == 0 && t.drehung[1] == 0 && t.drehung[2] == 0 && t.drehung[3] == 1);
    if (drehen && art != EntArt::Licht) {   // Licht (Q3): m_rotation wird nicht geschrieben
        winkel = winkelGedreht(winkel, t.drehung);
        if (art == EntArt::Modell && (e.klasse() == nullptr || !e.klasse()->hasAnglesKey)) {   // nur Gierwinkel
            winkel[0] = winkel[1] = 0;
        }
    }
    if (art == EntArt::MiscModell && t.skalierung != Vektor3f(1, 1, 1)) {
        const Matrix4 dreh = drehungEulerXyzGrad(winkelAlt);
        s = skalierungMitVorzeichen(multipliziert(affinInvers(dreh), multipliziert(skalierungsMatrix(t.skalierung), multipliziert(dreh, skalierungsMatrix(s)))));
    }
    // freezeTransform
    ursprungSchreiben(e, neuUrsprung);
    if (art != EntArt::Licht && winkel != winkelAlt) { winkelSchreiben(e, winkel); }
    if (art == EntArt::MiscModell) { skalierungSchreiben(e, s); }
}

void AuswahlBearbeitung::Impl::primitiveAnwenden(TArt art, const Transformationen& t, const Vektor3f& weltPivot) {
    zuruecksetzen();
    for (const Objekt& o : gewaehlteObjekte()) {
        const Matrix4 voll = o.prim ? kEinheitsMatrix : entitaetLokalZuWelt(*o.ent);
        const Matrix4 nurT = verschiebungsMatrix(voll.t().vec3());   // SELECTIONSYSTEM_AXIAL_PIVOTS
        Vektor3f tObj = t.verschiebung;
        switch (art) {
        case TArt::Verschieben: break;
        case TArt::Drehen: tObj = gepivotet(drehungAusQuaternionQuantisiert(t.drehung), weltPivot, nurT, nurT); break;
        case TArt::Skalieren: {
            Matrix4 lokal = multipliziert(vollInvers(voll), multipliziert(skalierungsMatrix(t.skalierung), voll));
            lokal.tx() = lokal.ty() = lokal.tz() = 0;   // Verschiebung weg
            tObj = gepivotet(lokal, weltPivot, voll, voll);
            break;
        }
        case TArt::Verzerren: {
            Matrix4 lokal = kEinheitsMatrix;
            lokal[t.verzerrung.index] = t.verzerrung.wert;
            tObj = gepivotet(lokal, weltPivot, nurT, nurT);
            break;
        }
        case TArt::Alles:
            tObj = gepivotet(transformAusKomponenten(Vektor3f(0, 0, 0), t.drehung, t.skalierung, t.verzerrung), weltPivot, nurT, nurT) + t.verschiebung;
            break;
        }
        if (!o.prim) {
            entitaetTransformieren(*o.ent, tObj, t);
            continue;
        }
        Transformationen pruef = t;
        pruef.verschiebung = tObj;
        if (pruef.eins()) { continue; }
        const Matrix4 mat = transformAusKomponenten(tObj, t.drehung, t.skalierung, t.verzerrung);
        if (o.prim->art() == Primitiv::Art::Pinsel) {
            auto* pinsel = static_cast<Pinsel*>(o.prim.get());
            for (const auto& f : pinsel->flaechen()) { vorAenderung(f, pinsel); }
            pinsel->transformieren(mat, karte.kontext);
        } else {
            vorAenderung(o.prim, nullptr);
            static_cast<Patch*>(o.prim.get())->transformieren(mat);
        }
    }
}

void AuswahlBearbeitung::Impl::komponentenAnwenden(TArt art, const Transformationen& t, const Vektor3f& weltPivot) {
    zuruecksetzen();
    // Pinsel: lokalZuWelt Einheit
    Vektor3f tObj = t.verschiebung;
    switch (art) {
    case TArt::Verschieben: break;
    case TArt::Drehen: tObj = gepivotet(drehungAusQuaternionQuantisiert(t.drehung), weltPivot, kEinheitsMatrix, kEinheitsMatrix); break;
    case TArt::Skalieren: tObj = gepivotet(skalierungsMatrix(t.skalierung), weltPivot, kEinheitsMatrix, kEinheitsMatrix); break;
    case TArt::Verzerren: {
        Matrix4 lokal = kEinheitsMatrix;
        lokal[t.verzerrung.index] = t.verzerrung.wert;
        tObj = gepivotet(lokal, weltPivot, kEinheitsMatrix, kEinheitsMatrix);
        break;
    }
    case TArt::Alles:
        tObj = gepivotet(transformAusKomponenten(Vektor3f(0, 0, 0), t.drehung, t.skalierung, t.verzerrung), weltPivot, kEinheitsMatrix, kEinheitsMatrix) +
               t.verschiebung;
        break;
    }
    Transformationen pruef = t;
    pruef.verschiebung = tObj;
    if (pruef.eins()) { return; }
    const Matrix4 mat = transformAusKomponenten(tObj, t.drehung, t.skalierung, t.verzerrung);
    // keine Texturprojektion beim Groesse-Aendern (BrushInstance::evaluateTransform)
    Kontext k = karte.kontext;
    if (modus == ManipModus::Ziehen && aussen.einstellung.modus == AuswahlModus::Primitiv) { k.texturLock = false; }
    std::map<Pinsel*, std::pair<std::shared_ptr<Pinsel>, std::vector<std::shared_ptr<Flaeche>>>> jePinsel;
    for (const auto& fw : karte.auswahl.flaechen()) {
        auto p = fw.pinsel.lock();
        auto f = fw.flaeche.lock();
        if (!p || !f) { continue; }
        auto& eintrag = jePinsel[p.get()];
        eintrag.first = p;
        eintrag.second.push_back(f);
    }
    for (auto& [roh, eintrag] : jePinsel) {
        // BrushInstance::transformComponents (brush.cpp:525-567)
        const auto anwenden = [&](const Matrix4& m) {
            roh->bRep(k);   // m_centroid_cached
            for (const auto& f : eintrag.second) {
                vorAenderung(f, roh);
                f->transformieren(m, false, k);
            }
            roh->geaendert();
        };
        const auto zurueck = [&]() {   // m_brush.revertTransform()
            for (const auto& f : eintrag.second) {
                Sicherung& s = sicherungen[f.get()];
                if (s.objekt && s.veraendert) {
                    s.objekt->zustandLaden(*s.zustand);
                    s.veraendert = false;
                }
            }
            roh->geaendert();
        };
        anwenden(mat);
        const Vektor3f verschiebungT = mat.t().vec3();
        if (verschiebungT == Vektor3f(0, 0, 0)) { continue; }
        Matrix4 ma = mat;
        ma.tx() = ma.ty() = ma.tz() = 0;
        if (!(ma == kEinheitsMatrix)) { continue; }   // nur Verschiebung
        // Flaechen gewaehlt und der Pinsel ergibt keinen Sinn: die groesste
        // noch sinnvolle Verschiebung im Raster suchen (binaere Suche)
        if (roh->traegtBei()) { continue; }
        for (std::size_t achse = 0; achse < 3; ++achse) {
            const float schritt = verschiebungT[achse] < 0 ? -aussen.raster : aussen.raster;
            int maxI = static_cast<int>(verschiebungT[achse] / schritt + .5F);
            int minI = 0;
            while (maxI > minI) {
                const int jetzt = minI + (maxI - minI + 1) / 2;
                ma[12 + achse] = static_cast<float>(jetzt) * schritt;
                zurueck();
                anwenden(ma);
                if (roh->traegtBei()) {
                    minI = jetzt;
                } else {
                    maxI = jetzt - 1;
                }
            }
            ma[12 + achse] = static_cast<float>(minI) * schritt;
        }
        zurueck();
        anwenden(ma);
    }
    for (auto& [roh, pe] : patchEbenen) {
        (void)roh;
        if (!pe.flaechen.irgendeine()) { continue; }
        vorAenderung(pe.patch, nullptr);
        pe.patch->transformieren(pe.flaechen.transformBerechnen(mat.t().vec3()));   // nur reine Verschiebung
    }
}

void AuswahlBearbeitung::Impl::verschieben(const Vektor3f& v) {
    if (!etwasGewaehlt()) { return; }
    verschiebung = v;
    pivot = pivotStart;
    verschiebeUm(pivot, v);
    Transformationen t;
    t.verschiebung = v;
    if (aussen.einstellung.modus == AuswahlModus::Komponente) {
        if (karte.auswahl.anzahl() != 0) { komponentenAnwenden(TArt::Verschieben, t, Vektor3f(0, 0, 0)); }
    } else if (karte.auswahl.anzahl() != 0) {
        primitiveAnwenden(TArt::Verschieben, t, Vektor3f(0, 0, 0));
    }
}

void AuswahlBearbeitung::Impl::drehen(const Quaternion& q) {
    if (!etwasGewaehlt()) { return; }
    drehung = q;
    Transformationen t;
    t.drehung = q;
    if (aussen.einstellung.modus == AuswahlModus::Komponente) {
        if (!karte.auswahl.flaechen().empty()) { komponentenAnwenden(TArt::Drehen, t, pivot.t().vec3()); }
    } else if (karte.auswahl.anzahl() != 0) {
        primitiveAnwenden(TArt::Drehen, t, pivot.t().vec3());
    }
}

void AuswahlBearbeitung::Impl::skalieren(const Vektor3f& s) {
    if (!etwasGewaehlt()) { return; }
    skalierung = s;
    Transformationen t;
    t.skalierung = s;
    if (aussen.einstellung.modus == AuswahlModus::Komponente) {
        if (!karte.auswahl.flaechen().empty()) { komponentenAnwenden(TArt::Skalieren, t, pivot.t().vec3()); }
    } else if (karte.auswahl.anzahl() != 0) {
        primitiveAnwenden(TArt::Skalieren, t, pivot.t().vec3());
    }
    if (modus == ManipModus::Verzerren) {
        pivot[0] = s[0];
        pivot[5] = s[1];
        pivot[10] = s[2];
    }
}

void AuswahlBearbeitung::Impl::verzerren(const Verzerrung& v) {
    if (!etwasGewaehlt()) { return; }
    verzerrung = v;
    Transformationen t;
    t.verzerrung = v;
    if (aussen.einstellung.modus == AuswahlModus::Komponente) {
        if (!karte.auswahl.flaechen().empty()) { komponentenAnwenden(TArt::Verzerren, t, pivot.t().vec3()); }
    } else if (karte.auswahl.anzahl() != 0) {
        primitiveAnwenden(TArt::Verzerren, t, pivot.t().vec3());
    }
    pivot[v.index] = v.wert;
}

void AuswahlBearbeitung::Impl::allesTransformieren(const Transformationen& t, const Vektor3f& weltPivot) {
    if (!etwasGewaehlt()) { return; }
    if (aussen.einstellung.modus == AuswahlModus::Komponente) {
        komponentenAnwenden(TArt::Alles, t, weltPivot);
    } else {
        primitiveAnwenden(TArt::Alles, t, weltPivot);
    }
}

void AuswahlBearbeitung::Impl::ursprungVerschieben(const Vektor3f& v, const bool gesetzt[3]) {
    pivot = pivotStart;
    eigenenUrsprungSetzen(v + pivotStart.t().vec3(), gesetzt);
}

// testselect_scene_point_unselected + testSelected_scene_snapped_point
std::optional<SzenenPunkt> AuswahlBearbeitung::Impl::ungewaehlterSzenenPunkt(const Blick& v, const GeraeteVektor& p, const GeraeteVektor& e) const {
    Blick geschert(v);
    auswahlTestBauen(geschert, auswahlKastenPunkt(p, e));
    AuswahlVolumen test(geschert);
    Treffer bester;
    const Flaeche* flaeche = nullptr;
    const auto dazu = [&](const Treffer& t, const Flaeche* f) {   // ScenePointSelector::addIntersection
        if (t < bester) {
            bester = t;
            flaeche = f;
        }
    };
    std::unordered_set<const void*> gewaehlt;
    for (const auto& el : karte.auswahl.elemente()) {
        if (zeigerLeer(el.primitiv)) {
            gewaehlt.insert(el.entitaet.lock().get());
        } else {
            gewaehlt.insert(el.primitiv.lock().get());
        }
    }
    for (const auto& ent : karte.entitaeten()) {
        if (ent->versteckt || gewaehlt.count(ent.get()) != 0) { continue; }
        if (!ent->istBehaelter()) {
            Treffer t;
            testeEntitaet(*ent, test, t);
            dazu(t, nullptr);
            continue;
        }
        for (const auto& pr : ent->kinder()) {
            if (pr->versteckt || gewaehlt.count(pr.get()) != 0) { continue; }
            if (pr->art() == Primitiv::Art::Pinsel) {
                const auto& pinsel = static_cast<const Pinsel&>(*pr);
                pinsel.bRep(karte.kontext);
                test.netzBeginnen(kEinheitsMatrix);
                for (const auto& f : pinsel.flaechen()) {
                    Treffer t;
                    testeFlaeche(*f, test, t);
                    dazu(t, f.get());
                }
            } else {
                Treffer t;
                testePatch(static_cast<const Patch&>(*pr), test, t);
                dazu(t, nullptr);
            }
        }
    }
    test.netzBeginnen(kEinheitsMatrix, true);
    if (!bester.gueltig()) { return std::nullopt; }
    const float fangW = aussen.fang;
    Vektor3d punktD(projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, bester.tiefe(), 1))));
    if (flaeche != nullptr) {
        double besterAbstand = FLT_MAX;
        Vektor3d kandidat;
        const Wicklung& w = flaeche->wicklung();
        for (std::size_t i = 0; i < w.numpoints; ++i) {
            const Vektor3d v1 = w[(i + w.numpoints - 1) % w.numpoints].vertex;
            const Vektor3d v2 = w[i].vertex;
            {   // Ecken
                const double abstand = laengeQuadrat(v2 - punktD);
                if (abstand < besterAbstand) {
                    kandidat = v2;
                    besterAbstand = abstand;
                }
            }
            {   // Kanten
                Vektor3d kantenPunkt = naechsterPunktD(v1, v2, punktD);
                if (kantenPunkt != v1 && kantenPunkt != v2) {
                    const Vektor3d richtung = normiert(v2 - v1);
                    const std::size_t maxi = groessteBetragsKomponente(richtung);
                    const double koeff = (zahlEingerastet(punktD[maxi], fangW) - v1[maxi]) / richtung[maxi];
                    kantenPunkt = v1 + richtung * koeff;
                    const double abstand = laengeQuadrat(kantenPunkt - punktD);
                    if (abstand < besterAbstand) {
                        kandidat = kantenPunkt;
                        besterAbstand = abstand;
                    }
                }
            }
        }
        if (bester.abstand() == 0) {   // Ebene, wenn ins Polygon gezeigt
            const Ebene3d& eb = flaeche->ebene();
            const std::size_t maxi = groessteBetragsKomponente(eb.normal());
            Vektor3d ebenenPunkt = eingerastet(punktD, fangW);
            ebenenPunkt[maxi] = (eb.dist() - eb.normal()[(maxi + 1) % 3] * ebenenPunkt[(maxi + 1) % 3] - eb.normal()[(maxi + 2) % 3] * ebenenPunkt[(maxi + 2) % 3]) /
                                eb.normal()[maxi];
            const double abstand = laengeQuadrat(ebenenPunkt - punktD);
            if (abstand < besterAbstand) { kandidat = ebenenPunkt; }
        }
        punktD = kandidat;
    } else {
        einrasten(punktD, fangW);
    }
    return SzenenPunkt{punktD, flaeche != nullptr ? std::optional<Ebene3d>(flaeche->ebene()) : std::nullopt};
}

// Scene_forEachPlaneSelectable_selectPlanes: Flaechen der gewaehlten Pinsel und
// Patch-Kastenseiten, dazu die gegenueberliegenden (gleiche Ebene, umgedreht).
bool AuswahlBearbeitung::Impl::ebenenWaehlen(AuswahlVolumen& test) {
    std::vector<Ebene3d> gewaehlteEbenen;   // SelectedPlaneSet
    std::vector<Waehlbar> flaechen;
    std::vector<std::pair<std::shared_ptr<Patch>, std::vector<std::size_t>>> patchSeiten;
    const std::vector<Objekt> objekte = gewaehlteObjekte();
    for (const Objekt& o : objekte) {
        if (!o.prim || o.prim->versteckt || o.ent->versteckt) { continue; }
        if (o.prim->art() == Primitiv::Art::Pinsel) {   // BrushInstance::selectPlanes
            const auto pinsel = std::static_pointer_cast<Pinsel>(o.prim);
            pinsel->bRep(karte.kontext);
            test.netzBeginnen(kEinheitsMatrix);
            const Vektor3f blickRichtung = test.blick().blickrichtung();
            double besterSkalar = 1;
            std::vector<std::shared_ptr<Flaeche>> beste;
            for (const auto& f : pinsel->flaechen()) {
                // trySelectPlane
                const Vektor3f proj = projiziert(transformierterVektor4(test.blick().sicht(), Vektor4f(f->schwerpunkt(), 1)));
                const Vektor3f naechster = projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, proj[2], 1)));
                const Wicklung& w = f->wicklung();
                bool ok = true;
                for (std::size_t i = 0; i < w.numpoints; ++i) {
                    // Epsilon gegen fast senkrechte Flaechen
                    if (punkt(f->ebene().normal(), Vektor3d(naechster) - w[i].vertex) < 0.005) {
                        ok = false;
                        break;
                    }
                }
                if (!ok) { continue; }
                const double skalar = std::fabs(punkt(f->ebene().normal(), blickRichtung));
                const double diff = besterSkalar - skalar;
                if (diff > 0.03) {
                    besterSkalar = skalar;
                    beste.clear();
                    beste.push_back(f);
                } else if (std::fabs(diff) <= 0.03) {
                    beste.push_back(f);
                }
            }
            for (const auto& f : beste) {
                flaechen.push_back(Waehlbar{Waehlbar::Art::Flaeche, o.ent, o.prim, f});
                gewaehlteEbenen.push_back(f->ebene());
                if (test.blick().fuellen()) { break; }   // in der Kamera nur eine Ebene
            }
        } else {   // PatchInstance::selectPlanes
            const auto patch = std::static_pointer_cast<Patch>(o.prim);
            test.netzBeginnen(kEinheitsMatrix);
            auto& pe = patchEbenen[patch.get()];
            pe.patch = patch;
            std::vector<std::size_t> seiten;
            pe.flaechen.ebenenWaehlen(patch->grenzen(), test, seiten, gewaehlteEbenen);
            patchSeiten.emplace_back(patch, seiten);
        }
    }
    // selectReversedPlanes
    for (const Objekt& o : objekte) {
        if (!o.prim || o.prim->versteckt || o.ent->versteckt) { continue; }
        if (o.prim->art() == Primitiv::Art::Pinsel) {
            const auto pinsel = std::static_pointer_cast<Pinsel>(o.prim);
            for (const auto& f : pinsel->flaechen()) {
                const Ebene3d um = umgedreht(f->ebene());
                if (std::any_of(gewaehlteEbenen.begin(), gewaehlteEbenen.end(),
                                [&](const Ebene3d& e) { return e.a == um.a && e.b == um.b && e.c == um.c && e.d == um.d; })) {
                    flaechen.push_back(Waehlbar{Waehlbar::Art::Flaeche, o.ent, o.prim, f});
                }
            }
        } else {
            const auto patch = std::static_pointer_cast<Patch>(o.prim);
            std::vector<std::size_t> seiten;
            auto& pe = patchEbenen[patch.get()];
            pe.patch = patch;
            pe.flaechen.umgedrehteWaehlen(patch->grenzen(), gewaehlteEbenen, seiten);
            patchSeiten.emplace_back(patch, seiten);
        }
    }
    for (const Waehlbar& w : flaechen) { setzeGewaehlt(karte, w, true); }
    for (const auto& [patch, seiten] : patchSeiten) {
        for (const std::size_t s : seiten) { patchEbenen[patch.get()].flaechen.gewaehlt[s] = true; }
    }
    return !gewaehlteEbenen.empty();
}

// Scene_forEachPlaneSelectable_bestPlane
BesteEbene AuswahlBearbeitung::Impl::besteEbene(AuswahlVolumen& test) {
    BesteEbene daten;
    const std::vector<Objekt> objekte = gewaehlteObjekte();
    const auto direkt = [&](const Objekt& o) {
        if (o.prim->art() == Primitiv::Art::Pinsel) {   // BrushInstance::bestPlaneDirect
            const auto& pinsel = static_cast<const Pinsel&>(*o.prim);
            pinsel.bRep(karte.kontext);
            test.netzBeginnen(kEinheitsMatrix);
            for (const auto& f : pinsel.flaechen()) {
                Treffer t;
                testeFlaeche(*f, test, t);
                if (t < daten.treffer) {
                    daten.treffer = t;
                    daten.ebene = f->ebene();
                }
            }
        } else {
            const auto patch = std::static_pointer_cast<Patch>(o.prim);
            test.netzBeginnen(kEinheitsMatrix);
            patchEbenen[patch.get()].patch = patch;
            patchEbenen[patch.get()].flaechen.besteEbeneDirekt(patch->grenzen(), test, daten);
        }
    };
    const auto indirekt = [&](const Objekt& o) {
        if (o.prim->art() == Primitiv::Art::Pinsel) {   // BrushInstance::bestPlaneIndirect / EdgeInstance::bestPlaneIndirect
            const auto& pinsel = static_cast<const Pinsel&>(*o.prim);
            test.netzBeginnen(kEinheitsMatrix);
            float skalar = 1;
            for (const FlaechenPunkt& kante : pinsel.kanten(karte.kontext)) {
                const Wicklung& w = pinsel[kante.flaeche].wicklung();
                Vektor3f start(w[kante.punkt].vertex);
                Vektor3f ende(w[(kante.punkt + 1) % w.numpoints].vertex);
                if (linieAnNahebeneKlippen(test.blick().sicht(), start, ende) != 2) { continue; }
                const Vektor3f neu = ZiehFlaechen::naechsterPunktAufLinieF(start, ende);
                const auto abstandNeu = static_cast<float>(laengeQuadrat(neu));
                const auto skalarNeu = static_cast<float>(std::fabs(punkt(normiert(neu), normiert(ende - start))));
                if (daten.abstand - abstandNeu > 1e-6F || (std::fabs(abstandNeu - daten.abstand) < 1e-6F && skalarNeu < skalar)) {
                    const Ebene3d& e1 = pinsel[kante.flaeche].ebene();
                    // next_edge: die Nachbarflaeche ueber "adjacent"
                    std::size_t nachbar = w[kante.punkt].adjacent;
                    if (nachbar >= pinsel.anzahl()) {
                        nachbar = kante.flaeche;
                    } else {
                        const Wicklung& wn = pinsel[nachbar].wicklung();
                        bool gefunden = false;
                        for (std::size_t i = 0; i < wn.numpoints; ++i) { gefunden = gefunden || wn[i].adjacent == kante.flaeche; }
                        if (!gefunden) { nachbar = kante.flaeche; }
                    }
                    const Ebene3d& e2 = pinsel[nachbar].ebene();
                    const auto zuweisen = [&](const Ebene3d& neueEbene) {
                        daten.ebene = neueEbene;
                        daten.naechsterPunkt = neu;
                        daten.abstand = abstandNeu;
                        skalar = skalarNeu;
                    };
                    if (test.blick().fuellen()) {
                        if (abstandZuPunkt(e1, test.blick().betrachter()) <= 0) {
                            zuweisen(e1);
                        } else if (abstandZuPunkt(e2, test.blick().betrachter()) <= 0) {
                            zuweisen(e2);
                        }
                    } else if (std::fabs(laengeQuadrat(ende - start)) > 1e-3) {
                        if (std::fabs(punkt(e1.normal(), test.blick().blickrichtung())) < std::fabs(punkt(e2.normal(), test.blick().blickrichtung()))) {
                            zuweisen(e1);
                        } else {
                            zuweisen(e2);
                        }
                    }
                }
            }
        } else {
            const auto patch = std::static_pointer_cast<Patch>(o.prim);
            test.netzBeginnen(kEinheitsMatrix);
            patchEbenen[patch.get()].patch = patch;
            patchEbenen[patch.get()].flaechen.besteEbeneIndirekt(patch->grenzen(), test, daten);
        }
    };
    for (const Objekt& o : objekte) {
        if (o.prim && !o.prim->versteckt && !o.ent->versteckt) { direkt(o); }
    }
    if (!daten.gueltig()) {
        for (const Objekt& o : objekte) {
            if (o.prim && !o.prim->versteckt && !o.ent->versteckt) { indirekt(o); }
        }
    }
    return daten;
}

bool AuswahlBearbeitung::Impl::ebenenWaehlen2(AuswahlVolumen& test, VerschiebeAchse2& achse) {
    const BesteEbene daten = besteEbene(test);
    if (daten.gueltig()) {
        const Ebene3d e = daten.ebene;
        if (daten.direkt()) {
            achse.setze0(punktAufEbeneSicht(e, test.blick().sicht(), GeraeteVektor(0, 0)), e);
        } else {
            test.netzBeginnen(kEinheitsMatrix);
            // kleiner Bildversatz moeglich; Klicks nicht weit weg sind nicht so schlimm
            achse.setze0(projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(daten.naechsterPunkt, 1))), e);
        }
        // selectByPlane
        for (const Objekt& o : gewaehlteObjekte()) {
            if (!o.prim || o.prim->versteckt || o.ent->versteckt) { continue; }
            if (o.prim->art() == Primitiv::Art::Pinsel) {
                for (const auto& f : static_cast<const Pinsel&>(*o.prim).flaechen()) {
                    if (ebenenGleich(e, f->ebene()) || ebenenGleich(e, umgedreht(f->ebene()))) {
                        setzeGewaehlt(karte, Waehlbar{Waehlbar::Art::Flaeche, o.ent, o.prim, f}, true);
                    }
                }
            } else {
                const auto patch = std::static_pointer_cast<Patch>(o.prim);
                auto& pe = patchEbenen[patch.get()];
                pe.patch = patch;
                pe.flaechen.perEbeneWaehlen(patch->grenzen(), e);
            }
        }
    }
    return daten.gueltig();
}

void AuswahlBearbeitung::Impl::polygoneSammeln(AuswahlVolumen& test, std::vector<std::vector<Vektor3f>>& aus) {
    const BesteEbene daten = besteEbene(test);
    if (!daten.gueltig()) { return; }
    for (const Objekt& o : gewaehlteObjekte()) {   // gatherPolygonsByPlane (mit umgedrehter Ebene)
        if (!o.prim || o.prim->versteckt || o.ent->versteckt) { continue; }
        if (o.prim->art() == Primitiv::Art::Pinsel) {
            for (const auto& f : static_cast<const Pinsel&>(*o.prim).flaechen()) {
                if ((ebenenGleich(daten.ebene, f->ebene()) || ebenenGleich(daten.ebene, umgedreht(f->ebene()))) && f->traegtBei()) {
                    const Wicklung& w = f->wicklung();
                    std::vector<Vektor3f> poly(w.numpoints);
                    for (std::size_t i = 0; i < w.numpoints; ++i) { poly[i] = Vektor3f(w[i].vertex); }
                    aus.push_back(std::move(poly));
                }
            }
        } else {
            ZiehFlaechen::polygoneSammeln(static_cast<const Patch&>(*o.prim).grenzen(), daten.ebene, aus);
        }
    }
}

// --- AuswahlBearbeitung -----------------------------------------------------------------------------

AuswahlBearbeitung::AuswahlBearbeitung(Karte& k) : m(std::make_unique<Impl>(k, *this)) {}
AuswahlBearbeitung::~AuswahlBearbeitung() = default;

void AuswahlBearbeitung::setzeModus(ManipModus mo) { m->modusSetzen(mo); }
ManipModus AuswahlBearbeitung::modus() const { return m->modus; }
bool AuswahlBearbeitung::etwasGewaehlt() const { return m->etwasGewaehlt(); }
bool AuswahlBearbeitung::ursprungVerschiebbar() const { return m->ursprungVerschiebbar(); }
void AuswahlBearbeitung::pivotGeaendert() { m->pivotWechsel = true; }
const Matrix4& AuswahlBearbeitung::pivotZuWelt() { return m->pivotZuWelt(); }
const Kasten& AuswahlBearbeitung::grenzen() {
    m->pivotBauen();
    return m->grenzenWert;
}
bool AuswahlBearbeitung::pivotEigen() const { return m->pivotEigen; }
bool AuswahlBearbeitung::bewegtSich() const { return m->pivotBewegt; }
bool AuswahlBearbeitung::rueckgaengigOffen() const { return m->rueckgaengigBegonnen; }
void AuswahlBearbeitung::manipulatorAbwaehlen() { m->manip->setzeGewaehlt(false); }

bool AuswahlBearbeitung::manipulatorWaehlen(const Blick& blick, const GeraeteVektor& p, const GeraeteVektor& eps) {   // SelectManipulator
    VerschiebeFreiXyZ::s_blickAbhaengig = bewegStil3d;
    bool ursprungBewegt = false;
    if (m->etwasGewaehlt() || modus() == ManipModus::Ziehen || modus() == ManipModus::Clipper || modus() == ManipModus::Bauen ||
        modus() == ManipModus::UV) {
        m->blick = &blick;   // Manipulatable::assign_static - vor den Manipulator-Aufrufen
        m->geraetPunkt = p;
        m->geraetEps = eps;
        m->ursprungM->setzeGewaehlt(false);
        m->manip->setzeGewaehlt(false);
        {
            Blick geschert(blick);
            auswahlTestBauen(geschert, auswahlKastenPunkt(p, eps));
            if (m->ursprungVerschiebbar()) {
                m->ursprungM->testen(geschert, m->pivotZuWelt());
                ursprungBewegt = m->ursprungM->gewaehlt();
            }
            if (!ursprungBewegt) { m->manip->testen(geschert, m->pivotZuWelt()); }
        }
        m->bewegungBeginnen();
        m->pivotBewegt = m->manip->gewaehlt();
        if (m->pivotBewegt || ursprungBewegt) {
            PivotZuWelt pz;
            pz.aktualisieren(m->pivotZuWelt(), blick.modelview(), blick.projektion(), blick.bildfeld());
            m->manipZuPivotStart = multipliziert(vollInvers(m->pivotStart), pz.weltRaum);
            const Matrix4 g = geraetZuManip(m->pivotStart, blick.modelview(), blick.projektion(), blick.bildfeld());
            if (m->pivotBewegt) {
                m->manip->manipulierbar()->bauen(g, p, m->grenzenWert, m->pivotZuWelt().t().vec3());
                m->rueckgaengigBegonnen = false;
            } else {
                m->ursprungM->manipulierbar()->bauen(g, p, m->grenzenWert, m->pivotZuWelt().t().vec3());
            }
        }
    }
    return m->pivotBewegt || ursprungBewegt;
}

void AuswahlBearbeitung::manipulatorHervorheben(const Blick& blick, const GeraeteVektor& p, const GeraeteVektor& eps) {   // HighlightManipulator
    m->blick = &blick;
    m->geraetPunkt = p;
    m->geraetEps = eps;
    if ((m->etwasGewaehlt() && m->ursprungVerschiebbar()) || modus() == ManipModus::Ziehen || modus() == ManipModus::Clipper ||
        modus() == ManipModus::Bauen || modus() == ManipModus::UV) {
        m->ursprungM->setzeGewaehlt(false);
        m->manip->setzeGewaehlt(false);
        Blick geschert(blick);
        auswahlTestBauen(geschert, auswahlKastenPunkt(p, eps));
        if (m->ursprungVerschiebbar()) { m->ursprungM->hervorheben(geschert, m->pivotZuWelt()); }
        if (!m->ursprungM->gewaehlt()) { m->manip->hervorheben(geschert, m->pivotZuWelt()); }
    }
}

void AuswahlBearbeitung::auswahlBewegen(const Blick& blick, const GeraeteVektor& p) {   // MoveSelected
    if (m->manip->gewaehlt()) {
        if (!m->rueckgaengigBegonnen) {
            m->rueckgaengigBegonnen = true;
            m->karte.undo.start();
        }
        const Matrix4 g = geraetZuManip(m->pivotStart, blick.modelview(), blick.projektion(), blick.bildfeld());
        m->manip->manipulierbar()->transformieren(m->manipZuPivotStart, g, p);
    } else if (m->ursprungM->gewaehlt()) {
        const Matrix4 g = geraetZuManip(m->pivotStart, blick.modelview(), blick.projektion(), blick.bildfeld());
        m->ursprungM->manipulierbar()->transformieren(m->manipZuPivotStart, g, p);
    }
}

bool AuswahlBearbeitung::bewegenBeenden() {   // endMove
    if (m->ursprungM->gewaehlt()) {
        if (m->pivot == m->pivotStart) {
            m->pivotEigen = !m->pivotEigen;
            m->pivotWechsel = true;
        }
        return true;
    }
    m->festschreiben();
    if (m->tmpKomponenten) {
        m->tmpKomponenten = false;
        m->alleKomponentenAbwaehlen();
    }
    m->pivotBewegt = false;
    m->pivotWechsel = true;
    if (m->rueckgaengigBegonnen) {
        std::string befehl;
        switch (modus()) {
        case ManipModus::Verschieben:
            befehl = "translateTool -xyz " + zahlText(m->verschiebung.x()) + ' ' + zahlText(m->verschiebung.y()) + ' ' + zahlText(m->verschiebung.z());
            break;
        case ManipModus::Drehen:   // NRC gibt die x/y/z-Teile des Quaternions aus
            befehl = "rotateTool -eulerXYZ " + zahlText(m->drehung.x()) + ' ' + zahlText(m->drehung.y()) + ' ' + zahlText(m->drehung.z());
            break;
        case ManipModus::Skalieren:
            befehl = "scaleTool -scale " + zahlText(m->skalierung.x()) + ' ' + zahlText(m->skalierung.y()) + ' ' + zahlText(m->skalierung.z());
            break;
        case ManipModus::Verzerren: befehl = "transformTool"; break;
        case ManipModus::Ziehen: befehl = "dragTool"; break;
        case ManipModus::UV: befehl = "UVTool"; break;
        case ManipModus::Clipper:
        case ManipModus::Bauen: break;
        }
        m->karte.undo.finish(befehl);
    }
    return false;
}

void AuswahlBearbeitung::verschiebeAuswahl(const Vektor3f& v) {   // translateSelected
    m->bewegungBeginnen();
    m->verschieben(v);
    m->festschreiben();
}

void AuswahlBearbeitung::dreheAuswahl(const Quaternion& q, bool ursprungEinrasten) {   // rotateSelected
    if (ursprungEinrasten && !m->pivotEigen) {
        m->pivotBauen();
        Vektor3f t = m->pivot.t().vec3();
        einrasten(t, fang);
        m->pivot.t().vec3() = t;
    }
    m->bewegungBeginnen();
    m->drehen(q);
    m->festschreiben();
}

void AuswahlBearbeitung::skaliereAuswahl(const Vektor3f& s, bool ursprungEinrasten) {   // scaleSelected
    if (ursprungEinrasten && !m->pivotEigen) {
        m->pivotBauen();
        Vektor3f t = m->pivot.t().vec3();
        einrasten(t, fang);
        m->pivot.t().vec3() = t;
    }
    m->bewegungBeginnen();
    m->skalieren(s);
    m->festschreiben();
}

void AuswahlBearbeitung::zeichnen(const Blick& blick, ManipZeichnung& aus) {   // renderSolid
    if (m->etwasGewaehlt() || modus() == ManipModus::Clipper || modus() == ManipModus::Bauen || modus() == ManipModus::UV ||
        modus() == ManipModus::Ziehen) {
        if (m->ursprungVerschiebbar()) { m->ursprungM->zeichnen(blick, m->pivotZuWelt(), aus); }
        m->manip->zeichnen(blick, m->pivotZuWelt(), aus);
    }
}

// --- NewBrushDrag (xywindow.cpp:684-740) ---------------------------------------------------------

void AuswahlBearbeitung::eigenenUrsprungSetzen(const Vektor3f& ursprung, const bool gesetzt[3]) {
    m->pivotBauen();
    m->eigenenUrsprungSetzen(ursprung, gesetzt);
}

void AuswahlBearbeitung::neueKarte() {
    m->sicherungen.clear();
    m->patchEbenen.clear();
    m->tmpKomponenten = false;
    m->pivotBewegt = false;
    m->rueckgaengigBegonnen = false;
    m->pivotEigen = false;
    m->pivotWechsel = true;
    m->neuerPinsel.reset();
    m->neuerPinselAktiv = false;
    m->manip->setzeGewaehlt(false);
    m->ursprungM->setzeGewaehlt(false);
    m->blick = nullptr;
}

bool AuswahlBearbeitung::neuerPinselMoeglich() const { return m->karte.auswahl.anzahl() == 0 && modus() != ManipModus::Clipper; }
bool AuswahlBearbeitung::neuerPinselAktiv() const { return m->neuerPinselAktiv; }

void AuswahlBearbeitung::neuerPinselBeginnen(const XyAnsicht& v, int x, int y) {
    m->neuerPinsel.reset();
    m->neuerPinselStart = v.zuPunkt(x, y, fang);
    m->neuerPinselAktiv = true;
}

void AuswahlBearbeitung::neuerPinselZiehen(const XyAnsicht& v, int x, int y, bool quadrat, bool wuerfel) {
    Vektor3f mn = m->neuerPinselStart;
    Vektor3f mx = v.zuPunkt(x, y, fang);
    const Vektor3f mxEcht = v.zuPunkt(x, y);
    const auto nDim = static_cast<std::size_t>(blickachse(v.typ));
    const auto nDim1 = static_cast<std::size_t>(dim1(v.typ));
    const auto nDim2 = static_cast<std::size_t>(dim2(v.typ));
    // nicht auf Nullgroesse einrasten, wenn der Pinsel schon da ist oder weit genug gezogen
    if (m->neuerPinsel != nullptr || laenge(mxEcht - mn) > fang / std::sqrt(2.0)) {
        for (const std::size_t i : {nDim1, nDim2}) {
            if (mx[i] == mn[i]) { mx[i] = mn[i] + std::copysign(fang, mxEcht[i] - mn[i]); }
        }
    }
    const Vektor3f arbeitMin = arbeitsbereich.origin - arbeitsbereich.extents;
    const Vektor3f arbeitMax = arbeitsbereich.origin + arbeitsbereich.extents;
    mn[nDim] = zahlEingerastet(arbeitMin[nDim], fang);
    mx[nDim] = zahlEingerastet(arbeitMax[nDim], fang);
    if (mx[nDim] <= mn[nDim]) { mx[nDim] = mn[nDim] + raster; }
    if (quadrat || wuerfel) {
        const float seite = std::max(std::fabs(mx[nDim1] - mn[nDim1]), std::fabs(mx[nDim2] - mn[nDim2]));
        for (const std::size_t i : {nDim1, nDim2}) { mx[i] = mn[i] + std::copysign(seite, mx[i] - mn[i]); }
        if (wuerfel) { mx[nDim] = mn[nDim] + seite; }
    }
    for (std::size_t i = 0; i < 3; ++i) {
        if (mn[i] == mx[i]) { return; }   // kein entarteter Pinsel
        if (mn[i] > mx[i]) { std::swap(mn[i], mx[i]); }
    }
    if (m->neuerPinsel == nullptr) { m->karte.undo.start(); }
    quaderGroesse(m->neuerPinsel, kastenAusMinMax(mn, mx));
}

bool AuswahlBearbeitung::neuerPinselBeenden() {
    m->neuerPinselAktiv = false;
    if (m->neuerPinsel != nullptr) {
        m->karte.undo.finish("brushDragNew");
        return true;
    }
    return false;
}

void AuswahlBearbeitung::quaderGroesse(std::shared_ptr<Pinsel>& pinsel, const Kasten& grenzen) {   // Scene_BrushResize_Cuboid
    if (pinsel == nullptr) {
        pinsel = std::make_shared<Pinsel>();
        pinsel->schicht = static_cast<int>(m->karte.aktuelleSchicht);
        const auto welt = m->karte.weltHolenOderAnlegen();
        welt->kindEinfuegen(pinsel);
        m->karte.auswahl.waehlen(welt, pinsel);   // selectPath( brushpath, true )
    }
    TexturProjektion projektion;
    texdefStandard(m->karte.kontext.art(), projektion);   // TextureTransform_getDefault
    pinsel->quaderBauen(grenzen, immerCaulk ? caulkShader : browserShader, projektion, m->karte.kontext);
}

}  // namespace bhed::karte
