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
#include "bhed/karte/komponenten.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "bhed/karte/texdef.h"
#include "bhed/karte/xyansicht.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
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

// Ohne Funktion (nur fuer den Grundzustand).
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
          uvM(neuerUvManipulator()), clipM(neuerClipManipulator()),
          ursprungM(std::make_unique<UrsprungManipulator>(*this, *this, pivotEigen)) {
        modusSetzen(ManipModus::Verschieben);
    }
    std::unique_ptr<Manipulator> neuerZiehManipulator();
    std::unique_ptr<Manipulator> neuerUvManipulator();
    std::unique_ptr<Manipulator> neuerClipManipulator();
    void clipperZuruecksetzen(bool ausFlaeche);   // m_clip_manipulator->reset
    void uvFestschreiben();   // m_uv_manipulator->freezeTransform()

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
        return (aussen.einstellung.modus == AuswahlModus::Komponente && komponentenZahl() != 0) ||
               (aussen.einstellung.modus == AuswahlModus::Primitiv && karte.auswahl.anzahl() != 0);
    }
    // countSelectedComponents, zwischengespeichert (die Zahl aendert sich nur mit
    // der Komponentenwahl, der Flaechenwahl oder einer Aenderung der Karte)
    [[nodiscard]] std::size_t komponentenZahl() const {
        const std::size_t schluessel[3] = {komponentenWahlStand(), karte.auswahl.flaechen().size(), karte.undo.aenderungen()};
        if (!komponentenZahlGueltig || schluessel[0] != komponentenZahlSchluessel[0] || schluessel[1] != komponentenZahlSchluessel[1] ||
            schluessel[2] != komponentenZahlSchluessel[2]) {
            komponentenZahlWert = komponentenAnzahl(karte);
            std::copy(schluessel, schluessel + 3, komponentenZahlSchluessel);
            komponentenZahlGueltig = true;
        }
        return komponentenZahlWert;
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
    // freezeTransforms: Face::freezeTransform (m_move_planepts =
    // m_move_planeptsTransformed), BrushInstance::applyTransform (evaluateBRep,
    // vertexModeFree).
    void festschreiben() {
        for (auto& [schluessel, s] : sicherungen) {
            (void)schluessel;
            if (auto* f = dynamic_cast<Flaeche*>(s.objekt.get())) { f->bewegPunkte = f->bewegPunkteT; }
        }
        for (const auto& p : eckenModusPinsel) {
            p->bRep(karte.kontext);
            p->eckenModusEnde();
        }
        eckenModusPinsel.clear();
        sicherungen.clear();
    }

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
    // Scene_forEachPlaneSelectable_bestPlane; nurPinsel: Scene_forEachSelectedBrush_bestPlane
    BesteEbene besteEbene(AuswahlVolumen& test, bool nurPinsel = false);
    // selection_selectVerticesOrFaceVertices (selection_mtor_drag.cpp:270-292)
    bool eckenOderFlaechenEckenWaehlen(AuswahlVolumen& test);
    // Scene_forEachBrush_bestPlane (selection_mtor_drag.cpp:208-232): ohne
    // gewaehlte Flaechen die der gewaehlten Pinsel, sonst aller sichtbaren
    // Pinsel, die gewaehlt sind oder gewaehlte Komponenten haben.
    BesteEbene besteEbeneExtrudieren(AuswahlVolumen& test);
    void besteEbeneKanten(AuswahlVolumen& test, const Objekt& o, BesteEbene& daten);
    // Die sichtbaren Pinsel, die gewaehlt sind oder Komponenten haben (in der
    // Reihenfolge der Szene, Scene_forEachVisibleBrush).
    [[nodiscard]] std::vector<std::pair<std::shared_ptr<Entitaet>, std::shared_ptr<Pinsel>>> extrudierPinsel() const;
    // scene_insert_brush_vertices (selection_mtor_drag.cpp:295-337)
    bool pinselEckenEinfuegen(const Blick& v, AuswahlVolumen& test, VerschiebeFreiXyZ& frei);
    // gatherComponentsHighlight ueber die sichtbaren gewaehlten Pinsel und Patches
    void komponentenHervorheben(AuswahlVolumen& test, KomponentenModus kmodus, std::vector<std::vector<Vektor3f>>& polygone);
    // ScenePointSelector ueber testselect_scene_point (alle) bzw.
    // testselect_scene_point_selected_brushes (nur gewaehlte Pinsel)
    struct SzenenTreffer {
        Treffer treffer;
        const Flaeche* flaeche = nullptr;
    };
    [[nodiscard]] SzenenTreffer szenenPunktTesten(AuswahlVolumen& test, int art) const;   // 0 alle, 1 ungewaehlte, 2 gewaehlte Pinsel
    // testSelected_scene_snapped_point (selection.cpp:668-719)
    [[nodiscard]] Vektor3d szenenPunktEingerastet(AuswahlVolumen& test, const SzenenTreffer& t) const;
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
    std::unique_ptr<Manipulator> verschiebeM, drehM, skalierM, verzerrM, ziehM, bauM, leerM, uvM, clipM, ursprungM;
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
    std::vector<std::shared_ptr<Pinsel>> eckenModusPinsel;   // Pinsel mit m_vertexModeOn in dieser Bewegung
    mutable std::size_t komponentenZahlWert = 0;
    mutable std::size_t komponentenZahlSchluessel[3] = {0, 0, 0};
    mutable bool komponentenZahlGueltig = false;
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

// DragExtrudeFaces (selection_mtable_extrude.h)
class FlaechenExtrudieren final : public Manipulierbar {
public:
    struct Quelle {   // ExtrudeSource
        std::shared_ptr<Entitaet> entitaet;
        std::shared_ptr<Pinsel> pinsel;   // m_brushInstance
        struct FlaecheZuPinsel {          // InFaceOutBrush
            std::shared_ptr<Flaeche> flaeche;
            EbenenPunkte ebenenPunkte{};
            std::shared_ptr<Pinsel> neu;   // m_outBrush
        };
        std::vector<FlaecheZuPinsel> flaechen;
        [[nodiscard]] bool flaecheAusgenommen(const Flaeche* f) const {   // faceExcluded
            return std::none_of(flaechen.begin(), flaechen.end(), [f](const FlaecheZuPinsel& x) { return x.flaeche.get() == f; });
        }
    };
    std::vector<Quelle> quellen;   // m_extrudeSources

    explicit FlaechenExtrudieren(AuswahlBearbeitung::Impl& u);
    void bauen(const Matrix4& g, const GeraeteVektor& p, const Kasten& b, const Vektor3f& ursprung) override;
    void transformieren(const Matrix4& m2o, const Matrix4& g, const GeraeteVektor& p) override;
    void setze0(const Vektor3f& start, const Ebene3d& ebene) {
        m_0 = start;
        m_ebeneGewaehlt = ebene;
    }

private:
    void flaechenVersetzen(const Quelle& q, Pinsel& pinsel, float abstand);   // offsetFaces
    void diagonalExtrudieren(const Pinsel& p0, const Pinsel& p2, Quelle& q);    // brush_extrudeDiag

    AuswahlBearbeitung::Impl& m_u;
    Vektor3f m_0;
    Ebene3d m_ebeneGewaehlt;
    std::size_t m_achseZ = 2;
    Ebene3d m_ebeneZ;
    Vektor3f m_startZ;
    bool m_originalGesichert = false;   // m_originalBrushSaved
    bool m_originalGeaendert = false;   // m_originalBrushChanged
};

class ZiehManipulator final : public Manipulator {
public:
    explicit ZiehManipulator(AuswahlBearbeitung::Impl& u)
        : m_u(u), m_resize(u), m_freiResize(u, m_resize), m_achseResize(u, m_resize), m_freiXyZ(u, u, u), m_neuerPinsel(u), m_extrudieren(u) {
        m_kreis.resize(2 << 3);
        kreisPunkte(m_kreis.size() >> 3, 5, m_kreis.data(), Umordnung::XYZ);
        setzeGewaehlt(false);
    }
    Manipulierbar* manipulierbar() override {
        if (m_neu) { return &m_neuerPinsel; }
        if (m_extrudiert) { return &m_extrudieren; }
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
            m_extrudiert = extrudierenVorbereiten(test);   // Scene_forEachBrush_setupExtrude
        } else if (k.auswahl.anzahl() != 0) {
            if (e.modus == AuswahlModus::Primitiv) {
                if (z.genau(false, false, true)) {
                    if (v.fuellen()) {   // Alt: Groesse entlang der Achse
                        m_gewaehlt2 = m_u.ebenenWaehlen2(test, m_achseResize);
                    } else {             // Alt in 2D: Ecken ziehen
                        m_gewaehlt = m_u.eckenOderFlaechenEckenWaehlen(test);
                    }
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
                } else if (m_u.komponentenZahl() != 0) {   // auch ohne Treffer ziehen, wenn etwas gewaehlt ist
                    m_ziehen = true;
                    m_freiXyZ.setze0(Vektor3f(0, 0, 0));
                } else if (e.komponente == KomponentenModus::Ecke) {   // sonst Ecken einfuegen
                    // hack: zeigt an, dass es kein voruebergehender Modus ist
                    m_ziehen = m_u.tmpKomponenten = m_u.pinselEckenEinfuegen(v, test, m_freiXyZ);
                    return;
                }
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
    void setzeGewaehlt(bool an) override { m_ziehen = m_gewaehlt = m_gewaehlt2 = m_neu = m_extrudiert = an; }
    [[nodiscard]] bool gewaehlt() const override { return m_ziehen || m_gewaehlt || m_gewaehlt2 || m_neu || m_extrudiert; }
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
            const BesteEbene daten = m_u.besteEbeneExtrudieren(test);
            if (daten.gueltig()) {   // gatherPolygonsByPlane( plane, polygons, false )
                for (const auto& [ent, pinsel] : m_u.extrudierPinsel()) {
                    (void)ent;
                    for (const auto& f : pinsel->flaechen()) {
                        if (ebenenGleich(daten.ebene, f->ebene()) && f->traegtBei()) {
                            const Wicklung& w = f->wicklung();
                            std::vector<Vektor3f> poly(w.numpoints);
                            for (std::size_t i = 0; i < w.numpoints; ++i) { poly[i] = Vektor3f(w[i].vertex); }
                            polygone.push_back(std::move(poly));
                        }
                    }
                }
            }
        } else if (k.auswahl.anzahl() != 0) {
            const AuswahlEinstellung& e = m_u.aussen.einstellung;
            if (e.modus == AuswahlModus::Primitiv) {
                if (z.genau(false, false, true)) {
                    if (v.fuellen()) {   // Alt: Groesse
                        m_u.polygoneSammeln(test, polygone);
                    } else {             // Alt: Ecken ziehen
                        m_u.komponentenHervorheben(test, KomponentenModus::Ecke, polygone);
                        if (polygone.empty()) {
                            const BesteEbene daten = m_u.besteEbene(test, true);
                            if (daten.gueltig()) {   // gatherPolygonsByPlane der gewaehlten Pinsel (mit umgedrehter Ebene)
                                for (const auto& g : gewaehltePrimitiveListe(k)) {
                                    if (g.primitiv->art() != Primitiv::Art::Pinsel || g.primitiv->versteckt || g.entitaet->versteckt) { continue; }
                                    for (const auto& f : static_cast<const Pinsel&>(*g.primitiv).flaechen()) {
                                        if ((ebenenGleich(daten.ebene, f->ebene()) || ebenenGleich(daten.ebene, umgedreht(f->ebene()))) && f->traegtBei()) {
                                            const Wicklung& w = f->wicklung();
                                            std::vector<Vektor3f> poly(w.numpoints);
                                            for (std::size_t i = 0; i < w.numpoints; ++i) { poly[i] = Vektor3f(w[i].vertex); }
                                            polygone.push_back(std::move(poly));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (z.keine() || z.genau(true, false, false) || (z.genau(false, true, false) && e.komponente == KomponentenModus::Flaeche)) {
                // Komponenten (wie RadiantSelectionSystem::SelectPoint)
                m_u.komponentenHervorheben(test, e.komponente, polygone);
            }
        }
        m_polygone.swap(polygone);
    }

private:
    // Scene_forEachBrush_setupExtrude (selection_mtor_drag.cpp:234-266)
    bool extrudierenVorbereiten(AuswahlVolumen& test) {
        Karte& k = m_u.karte;
        const BesteEbene daten = m_u.besteEbeneExtrudieren(test);
        if (!daten.gueltig()) { return false; }
        const Ebene3d e = daten.ebene;
        if (daten.direkt()) {
            m_extrudieren.setze0(punktAufEbeneSicht(e, test.blick().sicht(), GeraeteVektor(0, 0)), e);
        } else {
            test.netzBeginnen(kEinheitsMatrix);
            m_extrudieren.setze0(projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(daten.naechsterPunkt, 1))), e);
        }
        m_extrudieren.quellen.clear();
        for (const auto& [ent, pinsel] : m_u.extrudierPinsel()) {
            bool angelegt = false;
            for (const auto& f : pinsel->flaechen()) {
                if (k.auswahl.flaecheGewaehlt(f.get()) || ebenenGleich(e, f->ebene())) {
                    if (!angelegt) {
                        m_extrudieren.quellen.emplace_back();
                        m_extrudieren.quellen.back().entitaet = ent;
                        m_extrudieren.quellen.back().pinsel = pinsel;
                        angelegt = true;
                    }
                    m_extrudieren.quellen.back().flaechen.push_back({f, f->ebenenPunkte(), nullptr});
                }
            }
            for (const auto& f : pinsel->flaechen()) { k.auswahl.flaecheAbwaehlen(f.get()); }   // setSelectedComponents( false, eFace )
            k.auswahl.abwaehlen(ent, pinsel);                                                // setSelected( false )
        }
        return true;
    }

    AuswahlBearbeitung::Impl& m_u;
    ResizeVerschiebbar m_resize;
    VerschiebeFrei m_freiResize;
    VerschiebeAchse2 m_achseResize;
    VerschiebeFreiXyZ m_freiXyZ;
    NeuerPinsel m_neuerPinsel;
    FlaechenExtrudieren m_extrudieren;
    bool m_ziehen = false;        // m_dragSelected
    bool m_gewaehlt = false;      // m_selected: Komponenten zum Ziehen gewaehlt
    bool m_gewaehlt2 = false;     // m_selected2: Ebenen in der Kamera mit Alt
    bool m_neu = false;           // m_newBrush
    bool m_extrudiert = false;    // m_extrudeFaces
    std::vector<std::vector<Vektor3f>> m_polygone;
    std::vector<Vektor3f> m_kreis;
};

}  // namespace

namespace {

FlaechenExtrudieren::FlaechenExtrudieren(AuswahlBearbeitung::Impl& u) : Manipulierbar(u), m_u(u) {}

// DragExtrudeFaces::Construct (selection_mtable_extrude.h:60-110)
void FlaechenExtrudieren::bauen(const Matrix4&, const GeraeteVektor& p, const Kasten&, const Vektor3f&) {
    Karte& karte = m_u.karte;
    const Kontext& k = karte.kontext;
    m_achseZ = groessteBetragsKomponente(m_ebeneGewaehlt.normal());
    Vektor3f xy = blick().betrachter() - m_0;
    xy[m_achseZ] = 0;
    normieren(xy);
    m_ebeneZ = Ebene3d(xy, punkt(xy, m_0));
    m_startZ = punktAufEbeneSicht(m_ebeneZ, blick().sicht(), p);
    m_originalGesichert = false;
    m_originalGeaendert = false;
    Befehl befehl(karte.undo, "ExtrudeBrushFaces");
    for (Quelle& q : quellen) {
        for (auto& fp : q.flaechen) {
            const Flaeche& f = *fp.flaeche;
            auto neu = std::make_shared<Pinsel>();   // GlobalBrushCreator().createBrush()
            neu->schicht = static_cast<int>(karte.aktuelleSchicht);
            q.entitaet->kindEinfuegen(neu);
            karte.auswahl.waehlen(q.entitaet, neu);   // selectPath( path, true )
            fp.neu = neu;
            Flaeche* g = neu->flaecheHinzufuegen(f, k);
            if (g != nullptr) { g->versetzen(m_u.aussen.raster); }
            g = neu->flaecheHinzufuegen(f, k);
            if (g != nullptr) { g->umdrehen(); }
            const Wicklung& w = f.wicklung();
            for (std::size_t i = 0; i < w.numpoints; ++i) {
                const WicklungsPunkt& v = w[i];
                if (v.adjacent == kMaxFlaechen) { continue; }
                g = neu->flaecheHinzufuegen(*q.pinsel->flaechen()[v.adjacent], k);
                if (g == nullptr) { continue; }
                const Vektor3d quer = kreuz(g->ebene().normal(), f.ebene().normal());
                g->setzeEbenenPunkte({v.vertex, v.vertex + quer * 64.0, v.vertex + f.ebene().normal() * 64.0});
            }
        }
    }
}

// DragExtrudeFaces::Transform (selection_mtable_extrude.h:111-216)
void FlaechenExtrudieren::transformieren(const Matrix4&, const Matrix4&, const GeraeteVektor& p) {
    Karte& karte = m_u.karte;
    const Kontext& k = karte.kontext;
    Vektor3f aktuell = kAchsen[m_achseZ] *
                       static_cast<float>(punkt(m_ebeneGewaehlt.normal(), punktAufEbeneSicht(m_ebeneZ, blick().sicht(), p) - m_startZ)) *
                       (m_ebeneGewaehlt.normal()[m_achseZ] >= 0 ? 1.0F : -1.0F);
    // INF abfangen, kommt mit der Oberseite des Kastens in 2D vor
    if (!std::isfinite(aktuell[0]) || !std::isfinite(aktuell[1]) || !std::isfinite(aktuell[2])) { return; }
    einrasten(aktuell, m_u.aussen.fang);
    const float versatz = static_cast<float>(std::fabs(m_ebeneGewaehlt.normal()[m_achseZ]) *
                                             std::copysign(std::max(static_cast<double>(m_u.aussen.raster), laenge(aktuell)),
                                                           punkt(aktuell, m_ebeneGewaehlt.normal())));
    if (versatz >= 0) {   // nach aussen
        if (m_originalGeaendert) {
            m_originalGeaendert = false;
            for (Quelle& q : quellen) {   // den Ausgangspinsel zuruecksetzen
                for (auto& fp : q.flaechen) { fp.flaeche->setzeEbenenPunkte(fp.ebenenPunkte); }
            }
        }
        for (Quelle& q : quellen) {
            Pinsel& p0 = *q.pinsel;
            if (q.flaechen.size() > 1) {
                auto tmp = p0.kopie(k);
                flaechenVersetzen(q, *tmp, versatz);
                diagonalExtrudieren(p0, *tmp, q);
            } else {
                for (auto& fp : q.flaechen) {
                    const Flaeche& f = *fp.flaeche;
                    Pinsel& neu = *fp.neu;
                    neu.leeren();
                    Flaeche* g = neu.flaecheHinzufuegen(f, k);
                    if (g != nullptr) { g->versetzen(versatz); }
                    g = neu.flaecheHinzufuegen(f, k);
                    if (g != nullptr) { g->umdrehen(); }
                    const Wicklung& w = f.wicklung();
                    for (std::size_t i = 0; i < w.numpoints; ++i) {
                        if (w[i].adjacent != kMaxFlaechen) { neu.flaecheHinzufuegen(*p0.flaechen()[w[i].adjacent], k); }
                    }
                }
            }
        }
    } else {   // nach innen
        if (!m_originalGesichert) {
            m_originalGesichert = true;   // undoSave der Ausgangsflaechen (sichern() in den Setzern)
        }
        m_originalGeaendert = true;
        for (Quelle& q : quellen) {
            Pinsel& p0 = *q.pinsel;
            for (auto& fp : q.flaechen) { fp.flaeche->setzeEbenenPunkte(fp.ebenenPunkte); }   // den Ausgangspinsel zuruecksetzen
            if (q.flaechen.size() > 1) {
                auto tmp = p0.kopie(k);
                tmp->bRep(k);
                flaechenVersetzen(q, p0, versatz);
                if (p0.hatBeitragendeFlaechen(k)) { diagonalExtrudieren(p0, *tmp, q); }
            } else {
                for (auto& fp : q.flaechen) {
                    Flaeche& f = *fp.flaeche;
                    Pinsel& neu = *fp.neu;
                    neu.leeren();
                    for (const auto& g0 : p0.flaechen()) { neu.flaecheHinzufuegen(*g0, k); }   // copy( brush0 )
                    Flaeche* g = neu.flaecheHinzufuegen(f, k);
                    if (g != nullptr) {
                        g->versetzen(versatz);
                        g->umdrehen();
                    }
                    neu.leereFlaechenEntfernen(k);
                    f.versetzen(versatz);   // den Ausgangspinsel aendern
                }
            }
        }
    }
}

// offsetFaces (selection_mtable_extrude.h:224-236)
void FlaechenExtrudieren::flaechenVersetzen(const Quelle& q, Pinsel& pinsel, float abstand) {
    const Pinsel& p0 = *q.pinsel;
    for (std::size_t i = 0; i < p0.anzahl(); ++i) {
        if (!q.flaecheAusgenommen(p0.flaechen()[i].get())) { pinsel[i].versetzen(abstand); }
    }
    pinsel.bRep(m_u.karte.kontext);
}

// brush_extrudeDiag (selection_mtable_extrude.h:238-321): p0 und p2 haben
// gleich viele Flaechen in derselben Reihenfolge, p2 ist groesser.
void FlaechenExtrudieren::diagonalExtrudieren(const Pinsel& p0, const Pinsel& p2, Quelle& q) {
    const Kontext& k = m_u.karte.kontext;
    TexturProjektion projektion;
    texdefStandard(k.art(), projektion);   // TexDef_Construct_Default
    p0.bRep(k);
    p2.bRep(k);
    for (std::size_t i = 0; i < p0.anzahl(); ++i) {
        const Flaeche& f0 = p0[i];
        const Flaeche& f2 = p2[i];
        auto it = std::find_if(q.flaechen.begin(), q.flaechen.end(), [&](const auto& x) { return x.flaeche.get() == &f0; });
        if (it == q.flaechen.end() || !(f0.traegtBei() || f2.traegtBei())) { continue; }
        const std::string shader = f0.shader();
        Pinsel& neu = *it->neu;
        neu.leeren();
        if (f0.traegtBei()) {
            if (Flaeche* g = neu.flaecheHinzufuegen(f0, k)) { g->umdrehen(); }
        }
        if (f2.traegtBei()) { neu.flaecheHinzufuegen(f2, k); }
        if (f0.traegtBei() && f2.traegtBei()) {   // zwei gueltige Wicklungen vernaehen
            const auto seiten = [&](const Wicklung& w0, const Wicklung& w2, const Vektor3d& normale, bool tauschen) {
                for (std::size_t a = 0; a < w0.numpoints; ++a) {
                    const std::size_t naechster = wicklungNaechster(w0, a);
                    Vektor3d bester;
                    double besterSkalar = -1;
                    for (std::size_t b = 0; b < w2.numpoints; ++b) {
                        const double skalar = punkt(normiert(kreuz(w0[a].vertex - w0[naechster].vertex, w0[a].vertex - w2[b].vertex)), normale);
                        if (skalar > besterSkalar) {
                            besterSkalar = skalar;
                            bester = w2[b].vertex;
                        }
                    }
                    neu.ebeneHinzufuegen(w0[tauschen ? naechster : a].vertex, w0[tauschen ? a : naechster].vertex, bester, shader, projektion, k);
                }
            };
            // Seitenebenen aus der Sicht beider Wicklungen, weil sich ihre Form beim Aufweiten aendern kann
            seiten(f0.wicklung(), f2.wicklung(), f0.ebene().normal(), false);
            seiten(f2.wicklung(), f0.wicklung(), f0.ebene().normal(), true);
        } else {   // eine gueltige Wicklung
            const auto seiten = [&](const Wicklung& w0, const Pinsel& p, const Ebene3d& ebene, bool tauschen) {
                for (std::size_t a = 0; a < w0.numpoints; ++a) {
                    const std::size_t naechster = wicklungNaechster(w0, a);
                    Vektor3d bester;
                    double besterAbstand = 999999;
                    for (const auto& g : p.flaechen()) {
                        const Wicklung& w2 = g->wicklung();
                        for (std::size_t b = 0; b < w2.numpoints; ++b) {
                            const double abstand = laenge(w0[a].vertex - w2[b].vertex);
                            if (abstand < besterAbstand && abstandZuPunkt(ebene, w2[b].vertex) > .05) {
                                besterAbstand = abstand;
                                bester = w2[b].vertex;
                            }
                        }
                    }
                    neu.ebeneHinzufuegen(w0[tauschen ? naechster : a].vertex, w0[tauschen ? a : naechster].vertex, bester, shader, projektion, k);
                }
            };
            if (f0.traegtBei()) {
                seiten(f0.wicklung(), p2, f0.ebene(), false);
            } else if (f2.traegtBei()) {
                seiten(f2.wicklung(), p0, umgedreht(f2.ebene()), true);
            }
        }
        neu.leereFlaechenEntfernen(k);
    }
}

// --- UVManipulator (selection_mtor_uv.cpp) ------------------------------------------------------------
//
// Das UV-Werkzeug (G) zeigt in der Kamera das Texturgitter der zuletzt
// gewaehlten Flaeche (bzw. des zuletzt gewaehlten Patches) auf ihrer Ebene und
// verschiebt, dreht, skaliert und schert damit die Texturprojektion.

class UvManipulator final : public Manipulator, public Manipulierbar {
public:
    explicit UvManipulator(AuswahlBearbeitung::Impl& u);
    void zeichnen(const Blick& v, const Matrix4& pivotZuWelt, ManipZeichnung& aus) override;   // render
    void testen(const Blick& v, const Matrix4& pivotZuWelt) override;                         // testSelect
    void bauen(const Matrix4& g, const GeraeteVektor& p, const Kasten& b, const Vektor3f& ursprung) override;   // Construct
    void transformieren(const Matrix4& m2o, const Matrix4& g, const GeraeteVektor& p) override;               // Transform
    void festschreiben();   // freezeTransform
    Manipulierbar* manipulierbar() override { return this; }
    void setzeGewaehlt(bool an) override { m_istGewaehlt = an; }
    [[nodiscard]] bool gewaehlt() const override { return m_istGewaehlt; }

private:
    enum class Wahl { Keine, Pivot, GitterU, GitterV, PatchPunkt, PatchZeile, PatchSpalte, Kreis, PivotU, PivotV, U, V, UV, SchiefU, SchiefV, Textur };
    struct Punkt {   // PointVertex
        Vektor3f p;
        ManipFarbe f;
    };
    static constexpr ManipFarbe kWeiss{255, 255, 255, 255};
    static constexpr ManipFarbe kGrau{255, 255, 255, 125};
    static constexpr ManipFarbe kGrauer{100, 100, 100, 150};
    static constexpr ManipFarbe kRot{255, 0, 0, 255};
    static constexpr ManipFarbe kGruen{0, 255, 0, 255};
    static constexpr ManipFarbe kGrue{0, 150, 0, 255};
    static constexpr ManipFarbe kPink{255, 0, 255, 255};
    static constexpr ManipFarbe kPin{150, 0, 150, 255};
    static constexpr ManipFarbe kOrange{255, 125, 0, 255};
    static constexpr ManipFarbe kOrang{255, 125, 0, 125};

    [[nodiscard]] bool patchPunktInnen(std::size_t i) const { return (i % 2) != 0 || ((i / m_patchBreite) % 2) != 0; }
    template<typename F>
    void jedeKante(const F& f) const {   // forEachEdge
        if (m_flaeche) {
            const Wicklung& w = m_flaeche->wicklung();
            for (std::size_t i = 0; i < w.numpoints; ++i) {
                const std::size_t vorher = (i + w.numpoints - 1) % w.numpoints;
                f(Vektor3f(w[vorher].vertex), Vektor3f(w[i].vertex));
            }
        } else if (m_patch) {
            for (std::size_t i = 0; i + 1 < m_patchGitter.size(); i += 2) {
                const Vektor3f p0 = transformierterPunkt(m_flaecheTexZuLokal, m_patchGitter[i].p);
                const Vektor3f p1 = transformierterPunkt(m_flaecheTexZuLokal, m_patchGitter[i + 1].p);
                if (laengeQuadrat(p1 - p0) > 0.1) { f(p0, p1); }
            }
        }
    }
    template<typename F>
    void jederPunkt(const F& f) const {   // forEachPoint
        if (m_flaeche) {
            const Wicklung& w = m_flaeche->wicklung();
            for (std::size_t i = 0; i < w.numpoints; ++i) { f(Vektor3f(w[i].vertex)); }
        } else if (m_patch) {
            for (const auto& v : m_patchKontrolle) { f(transformierterPunkt(m_flaecheTexZuLokal, Vektor3f(v.texcoord.x(), v.texcoord.y(), 0))); }
        }
    }
    template<typename F>
    void jederUvPunkt(const F& f) const {   // forEachUVPoint
        if (m_flaeche) {
            const Wicklung& w = m_flaeche->wicklung();
            for (std::size_t i = 0; i < w.numpoints; ++i) { f(transformierterPunkt(m_flaecheLokalZuTex, Vektor3f(w[i].vertex))); }
        } else if (m_patch) {
            for (const auto& v : m_patchKontrolle) { f(Vektor3f(v.texcoord.x(), v.texcoord.y(), 0)); }
        }
    }
    [[nodiscard]] bool projektionGueltig() const;   // projection_valid
    void flaechenDatenAktualisieren(bool ursprungAktualisieren, bool linienAktualisieren = true);   // UpdateFaceData
    bool datenAktualisieren();   // UpdateData
    void wahlAnwenden(Wahl wahl, int gewaehltU, int gewaehltV, int patchIndex);   // applySelection
    void transformAuftragen(const Matrix4& transform);   // commitTransform

    AuswahlBearbeitung::Impl& m_u;
    Wahl m_wahl = Wahl::Keine;
    int m_gewaehltU = -1;   // m_selectedU (Index der ersten Ecke der Linie in m_uLinien)
    int m_gewaehltV = -1;
    int m_patchIndex = -1;   // m_selectedPatchIndex
    bool m_istGewaehlt = false;
    bool m_ziehen = false;   // zwischen Construct und freezeTransform: NRC aendert nur die "Transformed"-Kopien

    std::shared_ptr<Flaeche> m_flaeche;
    Ebene3d m_ebene;
    std::size_t m_breite = 0;
    std::size_t m_hoehe = 0;
    TexturProjektion m_projektion;
    Matrix4 m_lokalZuTex = kEinheitsMatrix;         // m_local2tex
    Matrix4 m_texZuLokal = kEinheitsMatrix;         // m_tex2local
    Matrix4 m_flaecheLokalZuTex = kEinheitsMatrix;  // m_faceLocal2tex
    Matrix4 m_flaecheTexZuLokal = kEinheitsMatrix;  // m_faceTex2local
    Vektor3f m_ursprung;                            // m_origin

    Matrix4 m_pivotZuWelt0 = kEinheitsMatrix;
    Matrix4 m_pivotZuWelt = kEinheitsMatrix;
    Punkt m_pivotPunkt{Vektor3f(0, 0, 0), kWeiss};
    std::vector<Punkt> m_pivotLinien;
    Matrix4 m_pivotLinienZuWelt = kEinheitsMatrix;
    std::vector<Punkt> m_uLinien;   // im UV-Raum
    std::vector<Punkt> m_vLinien;
    Matrix4 m_linienZuWelt = kEinheitsMatrix;

    unsigned int m_gitterU = 1;   // n - 1 Unterlinien in U, 1-16
    unsigned int m_gitterV = 1;
    Punkt m_gitterPunktU{Vektor3f(0, 0, 0), kWeiss};
    Punkt m_gitterPunktV{Vektor3f(0, 0, 0), kWeiss};
    Vektor2f m_gitterVorzeichen;

    std::vector<Vektor3f> m_kreis;
    ManipFarbe m_kreisFarbe = kGrau;
    Matrix4 m_kreisZuWelt = kEinheitsMatrix;

    std::shared_ptr<Patch> m_patch;
    std::size_t m_patchBreite = 0;
    std::size_t m_patchHoehe = 0;
    std::vector<PatchPunkt> m_patchKontrolle;   // m_patchCtrl
    std::vector<Punkt> m_patchPunkte;
    std::vector<Punkt> m_patchGitter;
    std::vector<unsigned int> m_patchDreiecke;
    std::string m_patchShader;   // m_state_patch_raw

    Vektor3f m_start;
};

UvManipulator::UvManipulator(AuswahlBearbeitung::Impl& u) : Manipulierbar(u), m_u(u) {
    m_kreis.resize(8 << 3);
    kreisPunkte(8, 1, m_kreis.data(), Umordnung::XYZ);
    m_pivotLinien.assign(4, Punkt{Vektor3f(0, 0, 0), kWeiss});
}

bool UvManipulator::projektionGueltig() const {
    return !(!std::isfinite(m_lokalZuTex[0]) || !std::isfinite(m_texZuLokal[0]) ||
             std::fabs(punkt(m_ebene.normal(), m_texZuLokal.z().vec3())) < 1e-6 ||   // entlang der Flaeche projiziert
             laengeQuadrat(m_texZuLokal.x().vec3()) < .01 ||   // stark verkleinert, hoechstens 10 Texturen je Einheit
             laengeQuadrat(m_texZuLokal.y().vec3()) < .01 || laengeQuadrat(m_texZuLokal.x().vec3()) > 1e9 ||
             laengeQuadrat(m_texZuLokal.y().vec3()) > 1e9);
}

void UvManipulator::flaechenDatenAktualisieren(bool ursprungAktualisieren, bool linienAktualisieren) {
    const Kontext& k = m_u.karte.kontext;
    if (m_flaeche) {
        m_ebene = m_flaeche->ebene();
        const auto [w, h] = k.groesse(m_flaeche->shader());
        m_breite = w;
        m_hoehe = h;
        m_projektion = m_flaeche->projektion();
        lokalZuTextur(k.art(), m_projektion, m_breite, m_hoehe, Vektor3f(m_ebene.normal()), m_lokalZuTex);
        m_texZuLokal = affinInvers(m_lokalZuTex);
    } else if (m_patch) {
        const Vektor3f normale = m_patch->durchschnittsNormale();
        m_ebene = Ebene3d(Vektor3d(normale), punkt(normale, m_patch->grenzen().origin));
        m_patchBreite = m_patch->breite();
        m_patchHoehe = m_patch->hoehe();
        m_patchKontrolle = m_patch->punkte();
        m_patchShader = m_patch->shader();
        {
            Vektor3f wRicht;
            Vektor3f hRicht;
            m_patch->durchschnittsAchsen(wRicht, hRicht);
            normieren(wRicht);
            normieren(hRicht);
            // laengste Zeile und Spalte
            float wLaenge = 0;
            float hLaenge = 0;
            std::size_t zeile = 0;
            std::size_t spalte = 0;
            for (std::size_t r = 0; r < m_patchHoehe; ++r) {
                float l = 0;
                for (std::size_t c = 0; c + 1 < m_patchBreite; ++c) { l += static_cast<float>(laenge(m_patch->punkt(r, c + 1).vertex - m_patch->punkt(r, c).vertex)); }
                if (l - wLaenge > .1F || ((r == 0 || r == m_patchHoehe - 1) && gleichEps(l, wLaenge, .1F))) {   // erste und letzte Zeile bevorzugt
                    wLaenge = l;
                    zeile = r;
                }
            }
            for (std::size_t c = 0; c < m_patchBreite; ++c) {
                float l = 0;
                for (std::size_t r = 0; r + 1 < m_patchHoehe; ++r) { l += static_cast<float>(laenge(m_patch->punkt(r + 1, c).vertex - m_patch->punkt(r, c).vertex)); }
                if (l - hLaenge > .1F || ((c == 0 || c == m_patchBreite - 1) && gleichEps(l, hLaenge, .1F))) {
                    hLaenge = l;
                    spalte = c;
                }
            }
            // drei Punkte auf der gefundenen Zeile und Spalte
            const PatchPunkt* p0 = nullptr;
            const PatchPunkt* p1 = nullptr;
            const PatchPunkt* p2 = nullptr;
            Vektor3f v0;
            Vektor3f v1;
            Vektor3f v2;
            {
                float distW0 = 0;
                float distW1 = 0;
                for (std::size_t c = 0; c < spalte; ++c) { distW0 += static_cast<float>(laenge(m_patch->punkt(zeile, c + 1).vertex - m_patch->punkt(zeile, c).vertex)); }
                for (std::size_t c = spalte; c + 1 < m_patchBreite; ++c) { distW1 += static_cast<float>(laenge(m_patch->punkt(zeile, c + 1).vertex - m_patch->punkt(zeile, c).vertex)); }
                float distH0 = 0;
                float distH1 = 0;
                for (std::size_t r = 0; r < zeile; ++r) { distH0 += static_cast<float>(laenge(m_patch->punkt(r + 1, spalte).vertex - m_patch->punkt(r, spalte).vertex)); }
                for (std::size_t r = zeile; r + 1 < m_patchHoehe; ++r) { distH1 += static_cast<float>(laenge(m_patch->punkt(r + 1, spalte).vertex - m_patch->punkt(r, spalte).vertex)); }
                const Kasten& g = m_patch->grenzen();
                const auto betrag = [](const Vektor3f& a) { return Vektor3f(std::fabs(a.x()), std::fabs(a.y()), std::fabs(a.z())); };
                if ((distW0 > distH0 && distW0 > distH1) || (distW1 > distH0 && distW1 > distH1)) {
                    p0 = &m_patch->punkt(0, spalte);
                    p1 = &m_patch->punkt(m_patchHoehe - 1, spalte);
                    p2 = distW0 > distW1 ? &m_patch->punkt(zeile, 0) : &m_patch->punkt(zeile, m_patchBreite - 1);
                    v0 = g.origin + hRicht * static_cast<float>(punkt(g.extents, betrag(hRicht)) * 1.1) + wRicht * (distW0 - wLaenge / 2);
                    v1 = v0 + hRicht * hLaenge;
                    v2 = v0 + hRicht * distH0 + (distW0 > distW1 ? (wRicht * -distW0) : (wRicht * distW1));
                } else {
                    p0 = &m_patch->punkt(zeile, 0);
                    p1 = &m_patch->punkt(zeile, m_patchBreite - 1);
                    p2 = distH0 > distH1 ? &m_patch->punkt(0, spalte) : &m_patch->punkt(m_patchHoehe - 1, spalte);
                    v0 = g.origin + wRicht * static_cast<float>(punkt(g.extents, betrag(wRicht)) * 1.1) + hRicht * (distH0 - hLaenge / 2);
                    v1 = v0 + wRicht * wLaenge;
                    v2 = v0 + wRicht * distW0 + (distH0 > distH1 ? (hRicht * -distH0) : (hRicht * distH1));
                }
                if (punkt(ebeneAusPunkten(v0, v1, v2).normal(), m_ebene.normal()) < 0) {
                    std::swap(p0, p1);
                    std::swap(v0, v1);
                }
            }
            const EbenenPunkte ecken{Vektor3d(v0), Vektor3d(v1), Vektor3d(v2)};
            const Vektor3d sts[3] = {Vektor3d(p0->texcoord.x(), p0->texcoord.y(), 0), Vektor3d(p1->texcoord.x(), p1->texcoord.y(), 0),
                                     Vektor3d(p2->texcoord.x(), p2->texcoord.y(), 0)};
            lokalZuTexturAusSt(ecken, sts, m_lokalZuTex);
            m_texZuLokal = affinInvers(m_lokalZuTex);
        }
    }
    if (!projektionGueltig()) {   // Fehlerpruefung
        m_gewaehltU = m_gewaehltV = -1;
        m_uLinien.clear();
        m_vLinien.clear();
        m_patchIndex = -1;
        return;
    }
    const Ebene3d ebene0(m_ebene.normal(), 0);
    m_flaecheTexZuLokal = m_texZuLokal;
    m_flaecheTexZuLokal.x().vec3() = punktAufEbene(ebene0, m_texZuLokal.x().vec3(), m_texZuLokal.z().vec3());
    m_flaecheTexZuLokal.y().vec3() = punktAufEbene(ebene0, m_texZuLokal.y().vec3(), m_texZuLokal.z().vec3());
    // so verschieben, dass z der UV gleich 0 ist: die Ebene entlang m_tex2local.z() bis dist() = 0
    m_flaecheTexZuLokal = multipliziert(
        verschiebungsMatrix(m_texZuLokal.z().vec3() *
                            static_cast<float>((m_ebene.dist() - punkt(m_ebene.normal(), m_texZuLokal.t().vec3())) / punkt(m_ebene.normal(), m_texZuLokal.z().vec3()))),
        m_flaecheTexZuLokal);
    m_flaecheLokalZuTex = affinInvers(m_flaecheTexZuLokal);

    if (m_patch) {
        m_patchPunkte.clear();
        for (std::size_t i = 0; i < m_patchKontrolle.size(); ++i) {
            m_patchPunkte.push_back(Punkt{Vektor3f(m_patchKontrolle[i].texcoord.x(), m_patchKontrolle[i].texcoord.y(), 0), patchPunktInnen(i) ? kPin : kGrue});
        }
        m_patchGitter.clear();
        for (std::size_t r = 0; r < m_patchHoehe; ++r) {
            for (std::size_t c = 0; c + 1 < m_patchBreite; ++c) {
                const Vektor2f& a = m_patch->punkt(r, c).texcoord;
                const Vektor2f& b = m_patch->punkt(r, c + 1).texcoord;
                m_patchGitter.push_back(Punkt{Vektor3f(a.x(), a.y(), 0), kOrang});
                m_patchGitter.push_back(Punkt{Vektor3f(b.x(), b.y(), 0), kOrang});
            }
        }
        for (std::size_t c = 0; c < m_patchBreite; ++c) {
            for (std::size_t r = 0; r + 1 < m_patchHoehe; ++r) {
                const Vektor2f& a = m_patch->punkt(r, c).texcoord;
                const Vektor2f& b = m_patch->punkt(r + 1, c).texcoord;
                m_patchGitter.push_back(Punkt{Vektor3f(a.x(), a.y(), 0), kOrang});
                m_patchGitter.push_back(Punkt{Vektor3f(b.x(), b.y(), 0), kOrang});
            }
        }
        m_patchDreiecke.clear();
        const auto& pc = m_patch->punkte();
        constexpr double kEntartet = 1e-5;
        for (std::size_t r = 0; r + 1 < m_patchHoehe; ++r) {
            for (std::size_t c = 0; c + 1 < m_patchBreite; ++c) {
                const auto i0 = static_cast<unsigned int>(m_patchBreite * r + c);
                const auto i1 = static_cast<unsigned int>(m_patchBreite * (r + 1) + c);
                const auto i2 = static_cast<unsigned int>(m_patchBreite * (r + 1) + c + 1);
                const auto i3 = static_cast<unsigned int>(m_patchBreite * r + c + 1);
                double quer = kreuz(pc[i2].texcoord - pc[i0].texcoord, pc[i1].texcoord - pc[i0].texcoord);
                if (!gleichEps(quer, 0.0, kEntartet)) {
                    m_patchDreiecke.insert(m_patchDreiecke.end(), {i0, i1, i2});
                    if (quer < 0) { std::swap(*(m_patchDreiecke.end() - 1), *(m_patchDreiecke.end() - 2)); }
                }
                quer = kreuz(pc[i3].texcoord - pc[i0].texcoord, pc[i2].texcoord - pc[i0].texcoord);
                if (!gleichEps(quer, 0.0, kEntartet)) {
                    m_patchDreiecke.insert(m_patchDreiecke.end(), {i0, i2, i3});
                    if (quer < 0) { std::swap(*(m_patchDreiecke.end() - 1), *(m_patchDreiecke.end() - 2)); }
                }
            }
        }
        if (m_patchDreiecke.empty()) {   // wenigstens ein Dreieck versuchen
            const unsigned int i0 = 0;
            for (unsigned int i1 = 1; i1 < pc.size(); ++i1) {
                if (laenge(pc[i1].texcoord - pc[i0].texcoord) > kEntartet) {
                    for (unsigned int i2 = i1 + 1; i2 < pc.size(); ++i2) {
                        const double quer = kreuz(pc[i2].texcoord - pc[i0].texcoord, pc[i1].texcoord - pc[i0].texcoord);
                        if (!gleichEps(quer, 0.0, kEntartet)) {
                            m_patchDreiecke.insert(m_patchDreiecke.end(), {i0, i1, i2});
                            if (quer < 0) { std::swap(*(m_patchDreiecke.end() - 1), *(m_patchDreiecke.end() - 2)); }
                            break;
                        }
                    }
                }
            }
        }
    }

    Vektor2f kleinst(FLT_MAX, FLT_MAX);
    Vektor2f groesst(-FLT_MAX, -FLT_MAX);
    jederUvPunkt([&](const Vektor3f& p) {
        kleinst.x() = std::min(kleinst.x(), p.x());
        groesst.x() = std::max(groesst.x(), p.x());
        kleinst.y() = std::min(kleinst.y(), p.y());
        groesst.y() = std::max(groesst.y(), p.y());
    });
    if (ursprungAktualisieren) { m_ursprung = transformierterPunkt(m_flaecheTexZuLokal, Vektor3f(kleinst.x(), kleinst.y(), 0)); }
    const Vektor3f uvUrsprung = transformierterPunkt(m_flaecheLokalZuTex, m_ursprung);
    {   // Griffe der Gitterdichte, auf der Seite des Polygons
        m_gitterVorzeichen.x() = groesst.y() - uvUrsprung.y() >= uvUrsprung.y() - kleinst.y() ? 1.0F : -1.0F;
        m_gitterVorzeichen.y() = groesst.x() - uvUrsprung.x() >= uvUrsprung.x() - kleinst.x() ? 1.0F : -1.0F;
        m_gitterPunktU.p = Vektor3f(uvUrsprung.x(),
                                    static_cast<float>(zuGanzzahl(uvUrsprung.y() + m_gitterVorzeichen.x() * .25)) +
                                        m_gitterVorzeichen.x() * (1 - 1.0F / std::max(static_cast<float>(m_gitterU), 1.8F)),
                                    0);
        m_gitterPunktV.p = Vektor3f(static_cast<float>(zuGanzzahl(uvUrsprung.x() + m_gitterVorzeichen.y() * .25)) +
                                        m_gitterVorzeichen.y() * (1 - 1.0F / std::max(static_cast<float>(m_gitterV), 1.8F)),
                                    uvUrsprung.y(), 0);
    }
    m_pivotZuWelt = m_texZuLokal;
    normieren(m_pivotZuWelt.x().vec3());
    normieren(m_pivotZuWelt.y().vec3());
    m_pivotZuWelt.t().vec3() = m_ursprung;
    m_pivotZuWelt0 = m_pivotZuWelt;
    {
        float besterAbstand = 0;
        jederPunkt([&](const Vektor3f& p) {
            const auto a = static_cast<float>(laengeQuadrat(p - m_ursprung));
            if (a > besterAbstand) { besterAbstand = a; }
        });
        besterAbstand = std::sqrt(besterAbstand);
        m_kreisZuWelt = kEinheitsMatrix;
        berechneAchsenbasis(m_ebene.normal(), m_kreisZuWelt.x().vec3(), m_kreisZuWelt.y().vec3());
        m_kreisZuWelt.x().vec3() *= besterAbstand;
        m_kreisZuWelt.y().vec3() *= besterAbstand;
        m_kreisZuWelt.z().vec3() = Vektor3f(m_ebene.normal());
        m_kreisZuWelt.t().vec3() = m_ursprung;
    }
    kleinst = Vektor2f(kleinst.x() - 5, kleinst.y() - 5);
    groesst = Vektor2f(groesst.x() + 5, groesst.y() + 5);
    kleinst.x() = static_cast<float>(zuGanzzahl(kleinst.x()));
    kleinst.y() = static_cast<float>(zuGanzzahl(kleinst.y()));
    groesst.x() = static_cast<float>(zuGanzzahl(groesst.x()));
    groesst.y() = static_cast<float>(zuGanzzahl(groesst.y()));
    m_gewaehltU = m_gewaehltV = -1;
    m_patchIndex = -1;
    m_linienZuWelt = m_flaecheTexZuLokal;
    m_pivotLinienZuWelt = m_flaecheTexZuLokal;
    if (linienAktualisieren) {
        const int imax = zuGanzzahl(groesst.y() - kleinst.y()) + 1;
        m_uLinien.clear();
        for (int i = 0; i < imax; ++i) {
            if (i != 0) {
                for (std::size_t j = m_gitterU - 1; j != 0; --j) {   // Unterlinien
                    const float y = kleinst.y() + static_cast<float>(i) - static_cast<float>(j) / static_cast<float>(m_gitterU);
                    m_uLinien.push_back(Punkt{Vektor3f(kleinst.x(), y, 0), kGrauer});
                    m_uLinien.push_back(Punkt{Vektor3f(groesst.x(), y, 0), kGrauer});
                }
            }
            m_uLinien.push_back(Punkt{Vektor3f(kleinst.x(), kleinst.y() + static_cast<float>(i), 0), kGrau});
            m_uLinien.push_back(Punkt{Vektor3f(groesst.x(), kleinst.y() + static_cast<float>(i), 0), kGrau});
        }
    }
    if (linienAktualisieren) {
        const int imax = zuGanzzahl(groesst.x() - kleinst.x()) + 1;
        m_vLinien.clear();
        for (int i = 0; i < imax; ++i) {
            if (i != 0) {
                for (std::size_t j = m_gitterV - 1; j != 0; --j) {
                    const float x = kleinst.x() + static_cast<float>(i) - static_cast<float>(j) / static_cast<float>(m_gitterV);
                    m_vLinien.push_back(Punkt{Vektor3f(x, kleinst.y(), 0), kGrauer});
                    m_vLinien.push_back(Punkt{Vektor3f(x, groesst.y(), 0), kGrauer});
                }
            }
            m_vLinien.push_back(Punkt{Vektor3f(kleinst.x() + static_cast<float>(i), kleinst.y(), 0), kGrau});
            m_vLinien.push_back(Punkt{Vektor3f(kleinst.x() + static_cast<float>(i), groesst.y(), 0), kGrau});
        }
    }
    m_pivotLinien[0].p = Vektor3f(kleinst.x(), uvUrsprung.y(), 0);   // u-Pivotlinie
    m_pivotLinien[1].p = Vektor3f(groesst.x(), uvUrsprung.y(), 0);
    m_pivotLinien[2].p = Vektor3f(uvUrsprung.x(), kleinst.y(), 0);   // v-Pivotlinie
    m_pivotLinien[3].p = Vektor3f(uvUrsprung.x(), groesst.y(), 0);
}

bool UvManipulator::datenAktualisieren() {   // UpdateData
    Karte& karte = m_u.karte;
    if (m_ziehen) { return projektionGueltig(); }
    if (!karte.auswahl.flaechen().empty()) {
        const auto f = karte.auswahl.flaechen().back().flaeche.lock();   // g_SelectedFaceInstances.last()
        if (f) {
            const auto [w, h] = karte.kontext.groesse(f->shader());
            if (m_flaeche != f) {
                m_flaeche = f;
                m_patch.reset();
                flaechenDatenAktualisieren(true);
            } else if (std::memcmp(&m_projektion, &f->projektion(), sizeof(TexturProjektion)) != 0 || m_breite != w || m_hoehe != h) {
                flaechenDatenAktualisieren(!projektionGueltig());   // Ursprung neu, wenn der vorige Stand ungueltig war
            }
            return projektionGueltig();
        }
    } else if (karte.auswahl.anzahl() != 0) {
        const auto p = std::dynamic_pointer_cast<Patch>(karte.auswahl.elemente().back().primitiv.lock());   // ultimateSelected
        if (p) {
            if (m_patch != p) {
                m_patch = p;
                m_flaeche.reset();
                flaechenDatenAktualisieren(true);
            } else if (m_patchBreite != p->breite() || m_patchHoehe != p->hoehe() || m_patchKontrolle.size() != p->punkte().size() ||
                       std::memcmp(m_patchKontrolle.data(), p->punkte().data(), sizeof(PatchPunkt) * m_patchKontrolle.size()) != 0 ||
                       m_patchShader != p->shader()) {
                flaechenDatenAktualisieren(!projektionGueltig());
            }
            return projektionGueltig();
        }
    }
    return false;
}

void UvManipulator::zeichnen(const Blick& v, const Matrix4&, ManipZeichnung& aus) {   // render
    if (!v.fuellen() || !datenAktualisieren()) { return; }
    const auto linien = [&](const std::vector<Punkt>& l, const Matrix4& m) {
        for (std::size_t i = 0; i + 1 < l.size(); i += 2) {
            aus.blendLinien.push_back(ManipZeichnung::Linie{transformierterPunkt(m, l[i].p), transformierterPunkt(m, l[i + 1].p), l[i].f});
        }
    };
    if (m_patch) {   // $uvtool/patchtexture: der Patch mit seiner Textur im UV-Raum
        ManipZeichnung::TexturNetz netz;
        netz.shader = m_patch->shader();
        netz.normale = normiert(transformierteNormale(m_linienZuWelt, Vektor3f(0, 0, 1)));
        for (const unsigned int i : m_patchDreiecke) {
            const Vektor2f& st = m_patch->punkte()[i].texcoord;
            netz.punkte.push_back(transformierterPunkt(m_linienZuWelt, Vektor3f(st.x(), st.y(), 0)));
            netz.st.push_back(st);
        }
        aus.texturNetze.push_back(std::move(netz));
    }
    linien(m_uLinien, m_linienZuWelt);   // $BLENDLINE
    linien(m_vLinien, m_linienZuWelt);
    linien(m_pivotLinien, m_pivotLinienZuWelt);
    if (m_patch) { linien(m_patchGitter, m_flaecheTexZuLokal); }
    {   // RenderablePivot (32) etwas zur Kamera hin, damit er sichtbar ist ($PIVOT, Pivot2World_worldSpace)
        Matrix4 lokalZuWelt;
        pivotWeltRaum(lokalZuWelt, multipliziert(verschiebungsMatrix(normiert(v.betrachter() - m_ursprung)), m_pivotZuWelt), v.modelview(), v.projektion(),
                      v.bildfeld());
        const Vektor3f o = transformierterPunkt(lokalZuWelt, Vektor3f(0, 0, 0));
        aus.pivotLinien.push_back(ManipZeichnung::Linie{o, transformierterPunkt(lokalZuWelt, Vektor3f(32, 0, 0)), kFarbeX});
        aus.pivotLinien.push_back(ManipZeichnung::Linie{o, transformierterPunkt(lokalZuWelt, Vektor3f(0, 32, 0)), kFarbeY});
        aus.pivotLinien.push_back(ManipZeichnung::Linie{o, transformierterPunkt(lokalZuWelt, Vektor3f(0, 0, 32)), kFarbeZ});
    }
    for (std::size_t i = 0; i < m_kreis.size(); ++i) {   // RenderableCircle (GL_LINE_LOOP)
        aus.blendLinien.push_back(ManipZeichnung::Linie{transformierterPunkt(m_kreisZuWelt, m_kreis[i]),
                                                        transformierterPunkt(m_kreisZuWelt, m_kreis[(i + 1) % m_kreis.size()]), m_kreisFarbe});
    }
    // $BIGPOINT
    if (m_patch) {
        for (const Punkt& p : m_patchPunkte) { aus.grossePunkte.push_back(ManipZeichnung::Punkt{transformierterPunkt(m_flaecheTexZuLokal, p.p), p.f}); }
    }
    aus.grossePunkte.push_back(ManipZeichnung::Punkt{transformierterPunkt(m_pivotZuWelt, m_pivotPunkt.p), m_pivotPunkt.f});
    aus.grossePunkte.push_back(ManipZeichnung::Punkt{transformierterPunkt(m_pivotLinienZuWelt, m_gitterPunktU.p), m_gitterPunktU.f});
    aus.grossePunkte.push_back(ManipZeichnung::Punkt{transformierterPunkt(m_pivotLinienZuWelt, m_gitterPunktV.p), m_gitterPunktV.f});
}

void UvManipulator::testen(const Blick& v, const Matrix4&) {   // testSelect
    if (!v.fuellen() || !datenAktualisieren()) {
        m_istGewaehlt = false;
        return;
    }
    Wahl wahl = Wahl::Keine;
    int index = -1;
    Treffer bester;
    const auto dazu = [&](const Treffer& t, Wahl w, int i = -1) {   // UVSelector::addIntersection
        if (t < bester) {
            bester = t;
            wahl = w;
            if (i >= 0) { index = i; }
        }
    };
    const Zusatz& z = zusatz();
    const bool alt = z.genau(false, false, true);
    int gewaehltU = -1;
    int gewaehltV = -1;
    if (!alt) {
        if (!z.keine()) {
            wahlAnwenden(wahl, -1, -1, index);
            return;
        }
        {   // Pivotpunkt
            Treffer t;
            punktBesterPunkt(multipliziert(v.sicht(), m_pivotZuWelt), m_pivotPunkt.p, t);
            dazu(t, Wahl::Pivot);
        }
        if (!bester.gueltig()) {   // Griffe der Gitterdichte
            const Matrix4 lzs = multipliziert(v.sicht(), m_flaecheTexZuLokal);
            Treffer t;
            punktBesterPunkt(lzs, m_gitterPunktU.p, t);
            dazu(t, Wahl::GitterU);
            punktBesterPunkt(lzs, m_gitterPunktV.p, t);
            dazu(t, Wahl::GitterV);
        }
        if (!bester.gueltig() && m_patch) {   // Patch-Punkte
            const Matrix4 lzs = multipliziert(v.sicht(), m_flaecheTexZuLokal);
            Treffer t;
            for (std::size_t i = 0; i < m_patchPunkte.size(); ++i) {
                punktBesterPunkt(lzs, m_patchPunkte[i].p, t);
                dazu(t, Wahl::PatchPunkt, static_cast<int>(i));
            }
        }
        if (!bester.gueltig() && m_patch) {   // Patch-Zeilen und -Spalten
            const Matrix4 lzs = multipliziert(v.sicht(), m_flaecheTexZuLokal);
            Treffer t;
            for (std::size_t r = 0; r < m_patchHoehe; ++r) {
                for (std::size_t c = 0; c + 1 < m_patchBreite; ++c) {
                    const std::size_t i = (r * (m_patchBreite - 1) + c) * 2;
                    const Vektor3f l[2] = {m_patchGitter[i].p, m_patchGitter[i + 1].p};
                    linieBesterPunkt(lzs, l, t);
                    dazu(t, Wahl::PatchZeile, static_cast<int>(r));
                }
            }
            for (std::size_t c = 0; c < m_patchBreite; ++c) {
                for (std::size_t r = 0; r + 1 < m_patchHoehe; ++r) {
                    const std::size_t i = (m_patchBreite - 1) * m_patchHoehe * 2 + (c * (m_patchHoehe - 1) + r) * 2;
                    const Vektor3f l[2] = {m_patchGitter[i].p, m_patchGitter[i + 1].p};
                    linieBesterPunkt(lzs, l, t);
                    dazu(t, Wahl::PatchSpalte, static_cast<int>(c));
                }
            }
        }
        if (!bester.gueltig()) {   // Kreis
            Treffer t;
            linienSchleifeBesterPunkt(multipliziert(v.sicht(), m_kreisZuWelt), m_kreis.data(), m_kreis.size(), t);
            dazu(t, Wahl::Kreis);
        }
        if (!bester.gueltig()) {   // Pivotlinien
            const Matrix4 lzs = multipliziert(v.sicht(), m_flaecheTexZuLokal);
            Treffer t;
            const Vektor3f lu[2] = {m_pivotLinien[0].p, m_pivotLinien[1].p};
            linieBesterPunkt(lzs, lu, t);
            dazu(t, Wahl::PivotU);
            const Vektor3f lv[2] = {m_pivotLinien[2].p, m_pivotLinien[3].p};
            linieBesterPunkt(lzs, lv, t);
            dazu(t, Wahl::PivotV);
        }
    }
    // testSelectUVlines: UV-Linien, Textur
    if (!bester.gueltig()) {
        const Matrix4 bildZuWelt = vollInvers(v.sicht());
        const auto proj = [&](double zz) {   // matrix4_transformed_vector4 mit BasicVector4<double>, vector4_projected
            const Matrix4& m = bildZuWelt;
            const double x = m[8] * zz + m[12];
            const double y = m[9] * zz + m[13];
            const double z = m[10] * zz + m[14];
            const double w = m[11] * zz + m[15];
            return Vektor3d(x / w, y / w, z / w);
        };
        const Strahld strahl = strahlAusPunkten(proj(-1), proj(1));
        const Vektor3d treffer = strahlSchnittEbene(strahl, m_ebene);
        const Vektor3f uvTreffer(transformierterPunkt(m_flaecheLokalZuTex, treffer));
        if (std::fabs(punkt(strahl.direction, m_ebene.normal())) > 1e-6 && !m_uLinien.empty() && !m_vLinien.empty() &&
            transformierterVektor4(v.sicht(), Vektor4f(Vektor3f(treffer), 1)).w() > 0) {
            const int naechsteU = static_cast<int>(std::min(
                m_uLinien.size() - 2,
                static_cast<std::size_t>(zuGanzzahl(std::max(0.F, uvTreffer.y() - m_uLinien.front().p.y()) * static_cast<float>(m_gitterU)) * 2)));
            const int naechsteV = static_cast<int>(std::min(
                m_vLinien.size() - 2,
                static_cast<std::size_t>(zuGanzzahl(std::max(0.F, uvTreffer.x() - m_vLinien.front().p.x()) * static_cast<float>(m_gitterV)) * 2)));
            const Punkt& pu = m_uLinien[static_cast<std::size_t>(naechsteU)];
            const Punkt& pv = m_vLinien[static_cast<std::size_t>(naechsteV)];
            const Vektor2f vorzeichen(uvTreffer.y() > pu.p.y() ? 1.0F : -1.0F, uvTreffer.x() > pv.p.x() ? 1.0F : -1.0F);
            const Vektor3f kreuzung(pv.p.x(), pu.p.y(), 0);
            const Vektor3f uMitte(pv.p.x() + vorzeichen.y() / static_cast<float>(m_gitterV * 2), pu.p.y(), 0);
            const Vektor3f vMitte(pv.p.x(), pu.p.y() + vorzeichen.x() / static_cast<float>(m_gitterU * 2), 0);
            Vektor3f texUMitte[2] = {pu.p, m_uLinien[static_cast<std::size_t>(naechsteU) + 1].p};
            texUMitte[0].y() = texUMitte[1].y() = vMitte.y();
            Vektor3f texVMitte[2] = {pv.p, m_vLinien[static_cast<std::size_t>(naechsteV) + 1].p};
            texVMitte[0].x() = texVMitte[1].x() = uMitte.x();
            Treffer iKreuz, iUMitte, iVMitte, iTexU, iTexV, iU, iV;
            const Treffer iNull;
            const Matrix4 lzs = multipliziert(v.sicht(), m_flaecheTexZuLokal);
            const Vektor3f lu[2] = {pu.p, m_uLinien[static_cast<std::size_t>(naechsteU) + 1].p};
            const Vektor3f lv[2] = {pv.p, m_vLinien[static_cast<std::size_t>(naechsteV) + 1].p};
            linieBesterPunkt(lzs, lu, iU);
            linieBesterPunkt(lzs, lv, iV);
            linieBesterPunkt(lzs, texUMitte, iTexU);
            linieBesterPunkt(lzs, texVMitte, iTexV);
            const bool uGewaehlt = iU < iNull;
            const bool vGewaehlt = iV < iNull;
            if (!uGewaehlt && !vGewaehlt) {   // keine Linie: die Textur
                wahl = Wahl::Textur;
            } else if ((!uGewaehlt || iTexU < iU) && (!vGewaehlt || iTexV < iV)) {   // Linien, aber die Texturmitten liegen naeher
                wahl = Wahl::Textur;
            } else if (uGewaehlt != vGewaehlt) {   // nur eine Linie
                if (uGewaehlt) {
                    wahl = alt ? Wahl::SchiefU : Wahl::U;
                    gewaehltU = naechsteU;
                } else {
                    wahl = alt ? Wahl::SchiefV : Wahl::V;
                    gewaehltV = naechsteV;
                }
            } else if (alt) {   // zwei Linien: zum Scheren nur eine
                if (iU < iV) {
                    wahl = Wahl::SchiefU;
                    gewaehltU = naechsteU;
                } else {
                    wahl = Wahl::SchiefV;
                    gewaehltV = naechsteV;
                }
            } else {
                punktBesterPunkt(lzs, uMitte, iUMitte);
                punktBesterPunkt(lzs, vMitte, iVMitte);
                punktBesterPunkt(lzs, kreuzung, iKreuz);
                const bool um = iUMitte < iNull;
                const bool vm = iVMitte < iNull;
                if ((!um && !vm) || (iKreuz < iUMitte && iKreuz < iVMitte)) {   // zwei Linien
                    wahl = Wahl::UV;
                    gewaehltU = naechsteU;
                    gewaehltV = naechsteV;
                } else if (iUMitte < iVMitte) {   // die naechste Linie
                    wahl = Wahl::U;
                    gewaehltU = naechsteU;
                } else {
                    wahl = Wahl::V;
                    gewaehltV = naechsteV;
                }
            }
        }
    }
    wahlAnwenden(wahl, gewaehltU, gewaehltV, index);
}

void UvManipulator::wahlAnwenden(Wahl wahl, int gewaehltU, int gewaehltV, int patchIndex) {   // applySelection
    if (m_wahl != wahl || m_gewaehltU != gewaehltU || m_gewaehltV != gewaehltV || m_patchIndex != patchIndex) {
        if (m_wahl != wahl) {
            switch (m_wahl) {
            case Wahl::Pivot: m_pivotPunkt.f = kWeiss; break;
            case Wahl::GitterU: m_gitterPunktU.f = kWeiss; break;
            case Wahl::GitterV: m_gitterPunktV.f = kWeiss; break;
            case Wahl::Kreis: m_kreisFarbe = kGrau; break;
            case Wahl::PivotU: m_pivotLinien[0].f = m_pivotLinien[1].f = kWeiss; break;
            case Wahl::PivotV: m_pivotLinien[2].f = m_pivotLinien[3].f = kWeiss; break;
            default: break;
            }
            switch (wahl) {
            case Wahl::Pivot: m_pivotPunkt.f = kRot; break;
            case Wahl::GitterU: m_gitterPunktU.f = kRot; break;
            case Wahl::GitterV: m_gitterPunktV.f = kRot; break;
            case Wahl::Kreis: m_kreisFarbe = kFarbeGewaehlt; break;
            case Wahl::PivotU: m_pivotLinien[0].f = m_pivotLinien[1].f = kRot; break;
            case Wahl::PivotV: m_pivotLinien[2].f = m_pivotLinien[3].f = kRot; break;
            default: break;
            }
        }
        const ManipFarbe gewaehltFarbe = zusatz().genau(false, false, true) ? kGruen : kFarbeGewaehlt;
        if (m_gewaehltU != gewaehltU || m_wahl != wahl) {   // die Linie wechselt oder Skalieren <-> Scheren
            if (m_gewaehltU >= 0 && static_cast<std::size_t>(m_gewaehltU) + 1 < m_uLinien.size()) {
                const ManipFarbe f = ((m_gewaehltU / 2) % static_cast<int>(m_gitterU)) == 0 ? kGrau : kGrauer;
                m_uLinien[static_cast<std::size_t>(m_gewaehltU)].f = m_uLinien[static_cast<std::size_t>(m_gewaehltU) + 1].f = f;
            }
            if (gewaehltU >= 0) { m_uLinien[static_cast<std::size_t>(gewaehltU)].f = m_uLinien[static_cast<std::size_t>(gewaehltU) + 1].f = gewaehltFarbe; }
        }
        if (m_gewaehltV != gewaehltV || m_wahl != wahl) {
            if (m_gewaehltV >= 0 && static_cast<std::size_t>(m_gewaehltV) + 1 < m_vLinien.size()) {
                const ManipFarbe f = ((m_gewaehltV / 2) % static_cast<int>(m_gitterV)) == 0 ? kGrau : kGrauer;
                m_vLinien[static_cast<std::size_t>(m_gewaehltV)].f = m_vLinien[static_cast<std::size_t>(m_gewaehltV) + 1].f = f;
            }
            if (gewaehltV >= 0) { m_vLinien[static_cast<std::size_t>(gewaehltV)].f = m_vLinien[static_cast<std::size_t>(gewaehltV) + 1].f = gewaehltFarbe; }
        }
        if (m_patchIndex != patchIndex || m_wahl != wahl) {
            const auto faerben = [&](Wahl w, int idx, bool an) {
                if (idx < 0) { return; }
                const auto ui = static_cast<std::size_t>(idx);
                const auto punktFarbe = [&](std::size_t i) { return patchPunktInnen(i) ? (an ? kPink : kPin) : (an ? kGruen : kGrue); };
                switch (w) {
                case Wahl::PatchPunkt: m_patchPunkte[ui].f = punktFarbe(ui); break;
                case Wahl::PatchZeile:
                    for (std::size_t c = 0; c + 1 < m_patchBreite; ++c) {
                        const std::size_t i = (ui * (m_patchBreite - 1) + c) * 2;
                        m_patchGitter[i].f = m_patchGitter[i + 1].f = an ? kOrange : kOrang;
                    }
                    for (std::size_t c = 0; c < m_patchBreite; ++c) { m_patchPunkte[ui * m_patchBreite + c].f = punktFarbe(ui * m_patchBreite + c); }
                    break;
                case Wahl::PatchSpalte:
                    for (std::size_t r = 0; r + 1 < m_patchHoehe; ++r) {
                        const std::size_t i = (m_patchBreite - 1) * m_patchHoehe * 2 + (ui * (m_patchHoehe - 1) + r) * 2;
                        m_patchGitter[i].f = m_patchGitter[i + 1].f = an ? kOrange : kOrang;
                    }
                    for (std::size_t r = 0; r < m_patchHoehe; ++r) { m_patchPunkte[r * m_patchBreite + ui].f = punktFarbe(r * m_patchBreite + ui); }
                    break;
                default: break;
                }
            };
            faerben(m_wahl, m_patchIndex, false);
            faerben(wahl, patchIndex, true);
        }
        m_wahl = wahl;
        m_gewaehltU = gewaehltU;
        m_gewaehltV = gewaehltV;
        m_patchIndex = patchIndex;
    }
    m_istGewaehlt = wahl != Wahl::Keine;
}

// transform_local2object: object2local x local x local2object (selection_.h)
Matrix4 transformLokalZuObjekt(const Matrix4& lokal, const Matrix4& lokalZuObjekt, const Matrix4& objektZuLokal) {
    return multipliziert(multipliziert(lokalZuObjekt, lokal), objektZuLokal);
}

void UvManipulator::transformAuftragen(const Matrix4& transform) {   // commitTransform
    const Kontext& k = m_u.karte.kontext;
    if (m_flaeche) {   // Face::transform_texdef: revertTexdef, Texdef_transform, EmitTextureCoordinates
        TexturProjektion p = m_projektion;
        texdefTransformiert(k.art(), p, m_breite, m_hoehe, m_flaeche->ebene(), transform, m_ursprung);
        m_flaeche->projektionSetzen(p);
    } else if (m_patch) {
        const Matrix4 uvTransform = transformLokalZuObjekt(affinInvers(transform), m_flaecheLokalZuTex, m_flaecheTexZuLokal);
        std::vector<PatchPunkt> punkte = m_patch->punkte();
        for (std::size_t i = 0; i < m_patchKontrolle.size() && i < punkte.size(); ++i) {
            const Vektor3f uv = transformierterPunkt(uvTransform, Vektor3f(m_patchKontrolle[i].texcoord.x(), m_patchKontrolle[i].texcoord.y(), 0));
            punkte[i].texcoord = Vektor2f(uv.x(), uv.y());
        }
        m_patch->punkteSetzen(punkte);
    }
}

void UvManipulator::bauen(const Matrix4&, const GeraeteVektor& p, const Kasten&, const Vektor3f&) {   // Construct
    m_start = punktAufEbeneSicht(m_ebene, blick().sicht(), p);
    m_ziehen = true;
}

// Transform (selection_mtor_uv.cpp:952-1650)
void UvManipulator::transformieren(const Matrix4&, const Matrix4&, const GeraeteVektor& p) {
    const Vektor3f aktuell = punktAufEbeneSicht(m_ebene, blick().sicht(), p);
    const bool fangen = zusatz().shift;
    const bool hartFangen = zusatz().strg;
    // Snapper: UV-Achse zu Bildschirm, damit das Einrasten auf dem Schirm gleich weit wirkt
    float sx = 0;
    float sy = 0;
    {
        Vektor3f skala(blick().bildfeld().x().x(), blick().bildfeld().y().y(), 0);
        skala /= std::max(skala.x(), skala.y());
        const Matrix4 proj = multipliziert(skalierungsMatrix(skala), blick().sicht());
        const Vektor3f jetzt = projiziert(transformierterVektor4(proj, Vektor4f(aktuell, 1)));
        const Vektor3f x = projiziert(transformierterVektor4(proj, Vektor4f(aktuell + normiert(m_flaecheTexZuLokal.x().vec3()), 1)));
        const Vektor3f y = projiziert(transformierterVektor4(proj, Vektor4f(aktuell + normiert(m_flaecheTexZuLokal.y().vec3()), 1)));
        sx = static_cast<float>(laenge(x - jetzt) * laenge(m_flaecheTexZuLokal.x().vec3()));
        sy = static_cast<float>(laenge(y - jetzt) * laenge(m_flaecheTexZuLokal.y().vec3()));
    }
    const auto xFaengt = [sx](float uvAbstand, float eps = .01F) { return uvAbstand * sx < eps; };
    const auto yFaengt = [sy](float uvAbstand, float eps = .01F) { return uvAbstand * sy < eps; };

    switch (m_wahl) {
    case Wahl::Pivot:
    case Wahl::PivotU:
    case Wahl::PivotV: {
        const Vektor3f uvStart = transformierterPunkt(m_flaecheLokalZuTex, m_ursprung);
        const Vektor3f uv = transformierterPunkt(m_flaecheLokalZuTex, aktuell);
        float besterU = FLT_MAX;
        float besterV = FLT_MAX;
        float zielU = 0;
        float zielV = 0;
        if (m_wahl != Wahl::PivotV) {
            for (std::size_t i = 0; i < m_uLinien.size(); i += 2) {
                const float a = std::fabs(m_uLinien[i].p.y() - uv.y());
                if (a < besterU) {
                    besterU = a;
                    zielU = m_uLinien[i].p.y();
                }
            }
        }
        if (m_wahl != Wahl::PivotU) {
            for (std::size_t i = 0; i < m_vLinien.size(); i += 2) {
                const float a = std::fabs(m_vLinien[i].p.x() - uv.x());
                if (a < besterV) {
                    besterV = a;
                    zielV = m_vLinien[i].p.x();
                }
            }
        }
        jederUvPunkt([&](const Vektor3f& q) {
            if (m_wahl != Wahl::PivotV) {
                const float a = std::fabs(q.y() - uv.y());
                if (a < besterU) {
                    besterU = a;
                    zielU = q.y();
                }
            }
            if (m_wahl != Wahl::PivotU) {
                const float a = std::fabs(q.x() - uv.x());
                if (a < besterV) {
                    besterV = a;
                    zielV = q.x();
                }
            }
        });
        Vektor3f ergebnis(uvStart);
        if (m_wahl != Wahl::PivotV) { ergebnis.y() = (yFaengt(besterU) || hartFangen) ? zielU : uv.y(); }
        if (m_wahl != Wahl::PivotU) { ergebnis.x() = (xFaengt(besterV) || hartFangen) ? zielV : uv.x(); }
        m_ursprung = transformierterPunkt(m_flaecheTexZuLokal, ergebnis);
        flaechenDatenAktualisieren(false, false);
        break;
    }
    case Wahl::GitterU:
    case Wahl::GitterV: {
        const bool u = m_wahl == Wahl::GitterU;
        const Vektor3f uvUrsprung = transformierterPunkt(m_flaecheLokalZuTex, m_ursprung);
        const Vektor3f uvJetzt = transformierterPunkt(m_flaecheLokalZuTex, aktuell);
        const float vz = u ? m_gitterVorzeichen.x() : m_gitterVorzeichen.y();
        const float o = u ? uvUrsprung.y() : uvUrsprung.x();
        const float j = u ? uvJetzt.y() : uvJetzt.x();
        const float abstand = std::max((static_cast<float>(zuGanzzahl(o + vz * .25)) + vz - j) * vz, .01F);
        unsigned int gitter = static_cast<unsigned int>(std::max(1, std::min(16, static_cast<int>(1 / abstand))));
        if (hartFangen) {   // auf die naechste Zweierpotenz
            gitter--;
            gitter |= gitter >> 1U;
            gitter |= gitter >> 2U;
            gitter |= gitter >> 4U;
            gitter |= gitter >> 8U;
            gitter |= gitter >> 16U;
            gitter++;
        }
        unsigned int& eigen = u ? m_gitterU : m_gitterV;
        unsigned int& anderes = u ? m_gitterV : m_gitterU;
        if (eigen != gitter || (fangen && anderes != gitter)) {
            eigen = gitter;
            if (fangen) { anderes = gitter; }
            flaechenDatenAktualisieren(false);
        }
        break;
    }
    case Wahl::Kreis: {
        const Vektor3f achse = m_texZuLokal.z().vec3();
        Vektor3f von = m_start - m_ursprung;
        aufAchseBeschraenken(von, achse);
        Vektor3f nach = aktuell - m_ursprung;
        aufAchseBeschraenken(nach, achse);
        const auto gedreht = [&](float winkel) {   // matrix4_pivoted_rotate_by_axisangle
            Matrix4 r = verschiebungsMatrix(m_ursprung);
            multipliziere(r, drehungAchseWinkel(achse, winkel));   // matrix4_rotate_by_axisangle (ohne Normieren, wie NRC)
            multipliziere(r, verschiebungsMatrix(-m_ursprung));
            return r;
        };
        Matrix4 dreh = gedreht(fangen ? zahlEingerastet(winkelFuerAchse(von, nach, achse), static_cast<float>(kPi / 12.0)) : winkelFuerAchse(von, nach, achse));
        {   // an Kanten ausrichten
            const Vektor3f uRicht = normiert(transformierteRichtung(dreh, m_texZuLokal.x().vec3()));
            const Vektor3f vRicht = normiert(transformierteRichtung(dreh, m_texZuLokal.y().vec3()));
            float besterSkalar = 0;
            Vektor3f bestesZiel;
            bool vAchse = false;
            jedeKante([&](const Vektor3f& p0, const Vektor3f& p1) {
                Vektor3f kante(p1 - p0);
                aufAchseBeschraenken(kante, achse);
                const auto du = static_cast<float>(std::fabs(punkt(uRicht, kante)));
                if (du > besterSkalar) {
                    besterSkalar = du;
                    bestesZiel = punkt(uRicht, kante) > 0 ? kante : -kante;
                    vAchse = false;
                }
                const auto dv = static_cast<float>(std::fabs(punkt(vRicht, kante)));
                if (dv > besterSkalar) {
                    besterSkalar = dv;
                    bestesZiel = punkt(vRicht, kante) > 0 ? kante : -kante;
                    vAchse = true;
                }
            });
            if (besterSkalar > 0.9994F || hartFangen) {
                const Vektor3f bestesVon = normiert(vAchse ? m_texZuLokal.y().vec3() : m_texZuLokal.x().vec3());
                dreh = gedreht(winkelFuerAchse(bestesVon, bestesZiel, achse));
            }
        }
        Matrix4 flaecheTexZuLokal = multipliziert(dreh, m_texZuLokal);
        const Ebene3d ebene0(m_ebene.normal(), 0);
        flaecheTexZuLokal.x().vec3() = punktAufEbene(ebene0, flaecheTexZuLokal.x().vec3(), m_texZuLokal.z().vec3());
        flaecheTexZuLokal.y().vec3() = punktAufEbene(ebene0, flaecheTexZuLokal.y().vec3(), m_texZuLokal.z().vec3());
        flaecheTexZuLokal = multipliziert(
            verschiebungsMatrix(m_texZuLokal.z().vec3() *
                                static_cast<float>((m_ebene.dist() - punkt(m_ebene.normal(), flaecheTexZuLokal.t().vec3())) / punkt(m_ebene.normal(), m_texZuLokal.z().vec3()))),
            flaecheTexZuLokal);
        m_linienZuWelt = m_pivotLinienZuWelt = flaecheTexZuLokal;
        m_pivotZuWelt = multipliziert(dreh, m_pivotZuWelt0);
        transformAuftragen(dreh);
        break;
    }
    case Wahl::U:
    case Wahl::V:
    case Wahl::UV: {
        const bool mitU = m_wahl != Wahl::V;
        const bool mitV = m_wahl != Wahl::U;
        const Vektor3f uvUrsprung = transformierterPunkt(m_lokalZuTex, m_ursprung);
        const Vektor3f bewegung = transformierterPunkt(m_lokalZuTex, aktuell) - transformierterPunkt(m_lokalZuTex, m_start);
        const Vektor3f uvStart(mitV ? m_vLinien[static_cast<std::size_t>(m_gewaehltV)].p.x() : 0, mitU ? m_uLinien[static_cast<std::size_t>(m_gewaehltU)].p.y() : 0, 0);
        const Vektor3f uvJetzt(mitV ? (m_vLinien[static_cast<std::size_t>(m_gewaehltV)].p + bewegung).x() : 0,
                               mitU ? (m_uLinien[static_cast<std::size_t>(m_gewaehltU)].p + bewegung).y() : 0, 0);
        float besterU = FLT_MAX;
        float zielU = 0;
        float besterV = FLT_MAX;
        float zielV = 0;
        jederUvPunkt([&](const Vektor3f& q) {
            const float au = std::fabs(q.y() - uvJetzt.y());
            if (au < besterU) {
                besterU = au;
                zielU = q.y();
            }
            const float av = std::fabs(q.x() - uvJetzt.x());
            if (av < besterV) {
                besterV = av;
                zielV = q.x();
            }
        });
        Vektor3f ergebnis(mitV ? uvJetzt.x() : 1, mitU ? uvJetzt.y() : 1, 1);
        if (mitU) {
            if (yFaengt(besterU) || hartFangen) { ergebnis.y() = zielU; }
            ergebnis.y() = (ergebnis.y() - uvUrsprung.y()) / (uvStart.y() - uvUrsprung.y());
        }
        if (mitV) {
            if (xFaengt(besterV) || hartFangen) { ergebnis.x() = zielV; }
            ergebnis.x() = (ergebnis.x() - uvUrsprung.x()) / (uvStart.x() - uvUrsprung.x());
        }
        if (fangen) {
            if (m_wahl == Wahl::U) {
                ergebnis.x() = std::fabs(ergebnis.y());
            } else if (m_wahl == Wahl::V) {
                ergebnis.y() = std::fabs(ergebnis.x());
            } else {
                const std::size_t best = std::fabs(ergebnis.x()) > std::fabs(ergebnis.y()) ? 0 : 1;
                ergebnis[(best + 1) % 2] = std::copysign(ergebnis[best], ergebnis[(best + 1) % 2]);
            }
        }
        // nicht auf 0 skalieren, hoechstens 10 Texturen je Welteinheit
        if ((mitV && laengeQuadrat(m_texZuLokal.x().vec3() * ergebnis.x()) < .01) || (mitU && laengeQuadrat(m_texZuLokal.y().vec3() * ergebnis.y()) < .01)) {
            return;
        }
        Matrix4 skala = kEinheitsMatrix;
        skaliereUmPunkt(skala, ergebnis, uvUrsprung);   // matrix4_pivoted_scale_by_vec3
        skala = transformLokalZuObjekt(skala, m_texZuLokal, m_lokalZuTex);
        {
            Matrix4 linienSkala = kEinheitsMatrix;
            skaliereUmPunkt(linienSkala, ergebnis, transformierterPunkt(m_flaecheLokalZuTex, m_ursprung));
            m_linienZuWelt = m_pivotLinienZuWelt = multipliziert(m_flaecheTexZuLokal, linienSkala);
            m_pivotZuWelt = multipliziert(m_pivotZuWelt0, skalierungsMatrix(ergebnis));
        }
        transformAuftragen(skala);
        break;
    }
    case Wahl::SchiefU:
    case Wahl::SchiefV: {
        const bool u = m_wahl == Wahl::SchiefU;
        const Vektor3f uvUrsprung = transformierterPunkt(m_flaecheLokalZuTex, m_ursprung);
        const Vektor3f bewegung = transformierterPunkt(m_flaecheLokalZuTex, aktuell) - transformierterPunkt(m_flaecheLokalZuTex, m_start);
        Matrix4 schief = kEinheitsMatrix;
        const Vektor3f linie = u ? m_uLinien[static_cast<std::size_t>(m_gewaehltU)].p : m_vLinien[static_cast<std::size_t>(m_gewaehltV)].p;
        if (u) {
            schief[4] = bewegung.x() / (linie - uvUrsprung).y();
        } else {
            schief[1] = bewegung.y() / (linie - uvUrsprung).x();
        }
        const Vektor3f geschert = transformierteRichtung(schief, u ? kAchseY : kAchseX);
        const float messAbstand = u ? (linie - uvUrsprung).y() : (linie - uvUrsprung).x();
        float besterAbstand = FLT_MAX;
        Vektor3f bestesZiel;
        const auto anKante = [&](const Vektor3f& kante) {
            if (u) {
                if (std::fabs(kante.y()) > 1e-5F) {   // nicht so einrasten, dass eine Achse gleich der anderen wird
                    const float a = std::fabs(kante.x() * messAbstand / kante.y() - geschert.x() * messAbstand / geschert.y());
                    if (a < besterAbstand) {
                        besterAbstand = a;
                        bestesZiel = kante;
                    }
                }
            } else if (std::fabs(kante.x()) > 1e-5F) {
                const float a = std::fabs(kante.y() * messAbstand / kante.x() - geschert.y() * messAbstand / geschert.x());
                if (a < besterAbstand) {
                    besterAbstand = a;
                    bestesZiel = kante;
                }
            }
        };
        jedeKante([&](const Vektor3f& p0, const Vektor3f& p1) {
            anKante(transformierterPunkt(m_flaecheLokalZuTex, p1) - transformierterPunkt(m_flaecheLokalZuTex, p0));
        });
        jederPunkt([&](const Vektor3f& q) {
            const Vektor3f po = transformierterPunkt(m_flaecheLokalZuTex, q);
            if (u) {
                for (std::size_t i = 0; i < m_vLinien.size(); i += 2) { anKante(po - Vektor3f(m_vLinien[i].p.x(), uvUrsprung.y(), 0)); }
            } else {
                for (std::size_t i = 0; i < m_uLinien.size(); i += 2) { anKante(po - Vektor3f(uvUrsprung.x(), m_uLinien[i].p.y(), 0)); }
            }
            anKante(po - Vektor3f(uvUrsprung.x(), uvUrsprung.y(), 0));
        });
        if (u ? (xFaengt(besterAbstand, .015F) || hartFangen) : (yFaengt(besterAbstand, .015F) || hartFangen)) {
            if (u) {
                schief[4] = bestesZiel.x() / bestesZiel.y();
            } else {
                schief[1] = bestesZiel.y() / bestesZiel.x();
            }
        }
        {
            Matrix4 m = kEinheitsMatrix;
            verschiebeUm(m, uvUrsprung);
            multipliziere(m, schief);
            verschiebeUm(m, -uvUrsprung);
            schief = m;
        }
        m_linienZuWelt = m_pivotLinienZuWelt = multipliziert(m_flaecheTexZuLokal, schief);
        m_pivotZuWelt = transformLokalZuObjekt(schief, m_texZuLokal, m_lokalZuTex);
        multipliziere(m_pivotZuWelt, m_pivotZuWelt0);
        schief = transformLokalZuObjekt(schief, m_flaecheTexZuLokal, m_flaecheLokalZuTex);
        transformAuftragen(schief);
        break;
    }
    case Wahl::Textur: {
        const Vektor3f uvStart = transformierterPunkt(m_flaecheLokalZuTex, m_start);
        const Vektor3f uvJetzt = transformierterPunkt(m_flaecheLokalZuTex, aktuell);
        const Vektor3f bewegung = uvJetzt - uvStart;
        float besterU = FLT_MAX;
        float besterV = FLT_MAX;
        float fangU = 0;
        float fangV = 0;
        const auto pruefen = [&](const Vektor3f& q) {
            for (std::size_t i = 0; i < m_uLinien.size(); i += 2) {
                const float a = q.y() - (m_uLinien[i].p.y() + bewegung.y());
                if (std::fabs(a) < besterU) {
                    besterU = std::fabs(a);
                    fangU = bewegung.y() + a;
                }
            }
            for (std::size_t i = 0; i < m_vLinien.size(); i += 2) {
                const float a = q.x() - (m_vLinien[i].p.x() + bewegung.x());
                if (std::fabs(a) < besterV) {
                    besterV = std::fabs(a);
                    fangV = bewegung.x() + a;
                }
            }
        };
        jederUvPunkt(pruefen);
        pruefen(transformierterPunkt(m_flaecheLokalZuTex, m_ursprung));
        Vektor3f ergebnis(bewegung);
        if (yFaengt(besterU) || hartFangen) { ergebnis.y() = fangU; }
        if (xFaengt(besterV) || hartFangen) { ergebnis.x() = fangV; }
        if (fangen) {
            float& kleiner = std::fabs(bewegung.x() * laenge(m_flaecheTexZuLokal.x().vec3())) < std::fabs(bewegung.y() * laenge(m_flaecheTexZuLokal.y().vec3()))
                                 ? ergebnis.x()
                                 : ergebnis.y();
            kleiner = 0;
        }
        {   // translation_local2object( result, m_faceTex2local, m_faceLocal2tex )
            Matrix4 m = m_flaecheTexZuLokal;
            verschiebeUm(m, ergebnis);
            ergebnis = multipliziert(m, m_flaecheLokalZuTex).t().vec3();
        }
        const Matrix4 verschiebung = verschiebungsMatrix(ergebnis);
        m_linienZuWelt = multipliziert(verschiebung, m_flaecheTexZuLokal);
        transformAuftragen(verschiebung);
        break;
    }
    case Wahl::PatchPunkt:
    case Wahl::PatchZeile:
    case Wahl::PatchSpalte: {
        std::vector<std::size_t> indizes;
        const auto idx = static_cast<std::size_t>(m_patchIndex);
        if (m_wahl == Wahl::PatchPunkt) {
            indizes.push_back(idx);
        } else if (m_wahl == Wahl::PatchZeile) {
            for (std::size_t c = 0; c < m_patchBreite; ++c) { indizes.push_back(idx * m_patchBreite + c); }
        } else {
            for (std::size_t r = 0; r < m_patchHoehe; ++r) { indizes.push_back(r * m_patchBreite + idx); }
        }
        const Vektor3f uvStart = transformierterPunkt(m_flaecheLokalZuTex, m_start);
        const Vektor3f uvJetzt = transformierterPunkt(m_flaecheLokalZuTex, aktuell);
        const Vektor3f bewegung = uvJetzt - uvStart;
        float besterU = FLT_MAX;
        float besterV = FLT_MAX;
        float fangU = 0;
        float fangV = 0;
        for (const std::size_t i : indizes) {
            const Vektor2f& st = m_patchKontrolle[i].texcoord;
            for (std::size_t l = 0; l < m_uLinien.size(); l += 2) {
                const float a = st.y() + bewegung.y() - m_uLinien[l].p.y();
                if (std::fabs(a) < besterU) {
                    besterU = std::fabs(a);
                    fangU = bewegung.y() - a;
                }
            }
            for (std::size_t l = 0; l < m_vLinien.size(); l += 2) {
                const float a = st.x() + bewegung.x() - m_vLinien[l].p.x();
                if (std::fabs(a) < besterV) {
                    besterV = std::fabs(a);
                    fangV = bewegung.x() - a;
                }
            }
            const Vektor3f o = transformierterPunkt(m_flaecheLokalZuTex, m_ursprung);
            {
                const float a = st.y() + bewegung.y() - o.y();
                if (std::fabs(a) < besterU) {
                    besterU = std::fabs(a);
                    fangU = bewegung.y() - a;
                }
            }
            {
                const float a = st.x() + bewegung.x() - o.x();
                if (std::fabs(a) < besterV) {
                    besterV = std::fabs(a);
                    fangV = bewegung.x() - a;
                }
            }
        }
        Vektor3f ergebnis(bewegung);
        if (yFaengt(besterU) || hartFangen) { ergebnis.y() = fangU; }
        if (xFaengt(besterV) || hartFangen) { ergebnis.x() = fangV; }
        if (fangen) {
            float& kleiner = std::fabs(bewegung.x() * laenge(m_flaecheTexZuLokal.x().vec3())) < std::fabs(bewegung.y() * laenge(m_flaecheTexZuLokal.y().vec3()))
                                 ? ergebnis.x()
                                 : ergebnis.y();
            kleiner = 0;
        }
        const Matrix4 verschiebung = verschiebungsMatrix(ergebnis);
        std::vector<PatchPunkt> punkte = m_patch->punkte();
        for (const std::size_t i : indizes) {
            const Vektor3f uv = transformierterPunkt(verschiebung, Vektor3f(m_patchKontrolle[i].texcoord.x(), m_patchKontrolle[i].texcoord.y(), 0));
            punkte[i].texcoord = Vektor2f(uv.x(), uv.y());
            m_patchPunkte[i].p = uv;
        }
        m_patch->punkteSetzen(punkte);
        // das Gitter ganz neu
        for (std::size_t r = 0; r < m_patchHoehe; ++r) {
            for (std::size_t c = 0; c + 1 < m_patchBreite; ++c) {
                const Vektor2f& a = punkte[r * m_patchBreite + c].texcoord;
                const Vektor2f& b = punkte[r * m_patchBreite + c + 1].texcoord;
                m_patchGitter[(r * (m_patchBreite - 1) + c) * 2].p = Vektor3f(a.x(), a.y(), 0);
                m_patchGitter[(r * (m_patchBreite - 1) + c) * 2 + 1].p = Vektor3f(b.x(), b.y(), 0);
            }
        }
        for (std::size_t c = 0; c < m_patchBreite; ++c) {
            for (std::size_t r = 0; r + 1 < m_patchHoehe; ++r) {
                const Vektor2f& a = punkte[r * m_patchBreite + c].texcoord;
                const Vektor2f& b = punkte[(r + 1) * m_patchBreite + c].texcoord;
                const std::size_t i = (m_patchBreite - 1) * m_patchHoehe * 2 + (c * (m_patchHoehe - 1) + r) * 2;
                m_patchGitter[i].p = Vektor3f(a.x(), a.y(), 0);
                m_patchGitter[i + 1].p = Vektor3f(b.x(), b.y(), 0);
            }
        }
        break;
    }
    default: break;
    }
}

void UvManipulator::festschreiben() {   // freezeTransform: die Aenderungen stehen schon (mit Undo) in Flaeche bzw. Patch
    m_ziehen = false;
}

// --- ClipManipulator (selection_mtor_clip.cpp) -------------------------------------------------------
//
// Drei Punkte ('1', '2', '3') legen die Schnittebene fest; in 2D genuegen
// zwei (die dritte Richtung ist die Blickrichtung), in 3D liegen die Punkte
// auf der Szene.

class ClipManipulator final : public Manipulator, public Manipulierbar, public Verschiebbar, public AllesTransformierbar {
public:
    explicit ClipManipulator(AuswahlBearbeitung::Impl& u) : Manipulierbar(u), m_u(u), m_ziehen(u, *this, *this) {
        m_punkte[0].name = '1';
        m_punkte[1].name = '2';
        m_punkte[2].name = '3';
    }
    void zeichnen(const Blick& v, const Matrix4& pivotZuWelt, ManipZeichnung& aus) override;   // render
    void testen(const Blick& v, const Matrix4& pivotZuWelt) override;                         // testSelect
    void hervorheben(const Blick& v, const Matrix4&) override { punkteTesten(v); }           // highlight
    void zuruecksetzen(bool ausFlaeche);                                                       // reset
    // Translatable (in 2D und mit TranslateFreeXY_Z in 3D)
    void verschieben(const Vektor3f& v) override {
        for (Punkt& p : m_punkte) {
            if (p.gewaehlt) {
                p.punkt = p.ohneTransform + Vektor3d(v);
                ebeneAktualisieren();
                break;
            }
        }
    }
    void allesTransformieren(const Transformationen&, const Vektor3f&) override {}   // unerreichbar
    void bauen(const Matrix4& g, const GeraeteVektor& p, const Kasten&, const Vektor3f& ursprung) override {   // Construct
        m_ziehen.setze0(ursprung);
        m_ziehen.bauen(g, p, Kasten(ursprung, Vektor3f(0, 0, 0)), ursprung);
    }
    void transformieren(const Matrix4& m2o, const Matrix4& g, const GeraeteVektor& p) override;   // Transform
    Manipulierbar* manipulierbar() override { return this; }
    void setzeGewaehlt(bool an) override {
        for (Punkt& p : m_punkte) { p.gewaehlt = an; }
    }
    [[nodiscard]] bool gewaehlt() const override { return m_punkte[0].gewaehlt || m_punkte[1].gewaehlt || m_punkte[2].gewaehlt; }
    // quickCondition (selection_mtor_clip.h:31-33): Strg in 2D
    static bool schnellBedingung(const Zusatz& z, const Blick& v) { return z.genau(false, true, false) && !v.fuellen(); }

private:
    struct Punkt {   // ClipperPoint
        bool gesetzt = false;   // m_set
        bool gewaehlt = false;  // SelectableBool
        Vektor3d punkt;         // m_point
        Vektor3d ohneTransform; // m_pointNonTransformed
        char name = '1';
    };
    void blickrichtungSetzen(const Vektor3f& r) {   // viewdir_set
        const std::size_t maxi = groessteBetragsKomponente(r);
        m_blickrichtung = r[maxi] > 0 ? kAchsen[maxi] : -kAchsen[maxi];
    }
    void blickrichtungKorrigieren();                     // viewdir_fixup
    void blickrichtungSchnittwuerdig(const Ebene3d& e);  // viewdir_make_cut_worthy
    void ebeneAktualisieren();                           // updatePlane
    [[nodiscard]] std::size_t neuerIndex(bool fuellen) const;   // newPointIndex
    void neuerPunkt(const Vektor3d& p, const Blick& v);         // newPoint
    bool szeneTesten(const Blick& v, Vektor3d& punktAus) const; // testSelect_scene
    void punkteTesten(const Blick& v);                          // testSelect_points
    void wahlWechsel(Punkt* p) {                                // ManipulatorSelectionChangeable::selectionChange
        if (p != nullptr) { p->gewaehlt = true; }
    }

    AuswahlBearbeitung::Impl& m_u;
    Punkt m_punkte[3];
    VerschiebeFreiXyZ m_ziehen;   // m_dragXY_Z
    Vektor3f m_blickrichtung{0, 0, 1};
};

void ClipManipulator::blickrichtungKorrigieren() {
    const Vektor3d d = m_punkte[1].punkt - m_punkte[0].punkt;
    if (std::fabs(laenge(d)) > 1e-3 && std::fabs(punkt(m_blickrichtung, normiert(d))) > 0.999) {   // zwei Punkte auf der Blickachse
        blickrichtungSetzen(blick().blickrichtung());
        if (std::fabs(punkt(m_blickrichtung, normiert(d))) > 0.999) {
            const Matrix4 bildZuWelt = vollInvers(blick().sicht());
            Vektor3f p[2];
            for (std::size_t i = 0; i < 2; ++i) { p[i] = projiziert(transformierterVektor4(blick().sicht(), Vektor4f(Vektor3f(m_punkte[i].punkt), 1))); }
            const float tiefenRichtung = p[1].z() > p[0].z() ? -1.0F : 1.0F;
            for (std::size_t i = 0; i < 2; ++i) {
                p[i].z() = -1;
                p[i] = projiziert(transformierterVektor4(bildZuWelt, Vektor4f(p[i], 1)));
            }
            blickrichtungSetzen((p[1] - p[0]) * tiefenRichtung);
        }
    }
}

void ClipManipulator::blickrichtungSchnittwuerdig(const Ebene3d& e) {
    const std::size_t maxi = groessteBetragsKomponente(e.normal());
    const Kasten& b = m_u.grenzenWert;
    if (ebeneGueltig(e) && kastenGueltig(b) && std::fabs(e.normal()[maxi]) > 0.999) {   // achsparallele Ebene
        const double anker = e.normal()[maxi] * e.dist();
        if (anker > b.origin[maxi]) {
            if ((anker - (b.origin[maxi] + b.extents[maxi])) > -0.1) { blickrichtungSetzen(-kAchsen[maxi]); }
        } else if ((-anker + (b.origin[maxi] - b.extents[maxi])) > -0.1) {
            blickrichtungSetzen(kAchsen[maxi]);
        }
    }
}

void ClipManipulator::ebeneAktualisieren() {
    std::size_t anzahl = 0;
    while (anzahl < 3 && m_punkte[anzahl].gesetzt) { ++anzahl; }
    ClipperPunkte neu;
    switch (anzahl) {
    case 1: neu = ClipperPunkte{{m_punkte[0].punkt, m_punkte[0].punkt, m_punkte[0].punkt}, anzahl}; break;
    case 2:
        if (blick().fuellen()) {   // 3D
            blickrichtungKorrigieren();
            m_punkte[2].punkt = m_punkte[0].punkt - Vektor3d(m_blickrichtung) * laenge(m_punkte[0].punkt - m_punkte[1].punkt);
            blickrichtungSchnittwuerdig(ebeneAusPunkten(m_punkte[0].punkt, m_punkte[1].punkt, m_punkte[2].punkt));
        }
        m_punkte[2].punkt = m_punkte[0].punkt - Vektor3d(m_blickrichtung) * laenge(m_punkte[0].punkt - m_punkte[1].punkt);
        [[fallthrough]];
    case 3: neu = ClipperPunkte{{m_punkte[0].punkt, m_punkte[1].punkt, m_punkte[2].punkt}, anzahl}; break;
    default: break;
    }
    m_u.aussen.clipperPunkte = neu;   // Clipper_setPlanePoints
}

std::size_t ClipManipulator::neuerIndex(bool fuellen) const {
    const std::size_t maxi = (!fuellen && m_u.aussen.clipper2Punkte2d) ? 2 : 3;
    std::size_t i = 0;
    for (; i < maxi; ++i) {
        if (!m_punkte[i].gesetzt) { break; }
    }
    return i % maxi;
}

void ClipManipulator::neuerPunkt(const Vektor3d& p, const Blick& v) {
    const std::size_t i = neuerIndex(v.fuellen());
    if (i == 0) { m_punkte[1].gesetzt = m_punkte[2].gesetzt = false; }
    m_punkte[i].gesetzt = true;
    m_punkte[i].punkt = p;
    wahlWechsel(&m_punkte[i]);
    if (i == 1) { blickrichtungSetzen(blick().blickrichtung()); }
    ebeneAktualisieren();
}

bool ClipManipulator::szeneTesten(const Blick& v, Vektor3d& punktAus) const {
    AuswahlVolumen test(v);
    const AuswahlBearbeitung::Impl::SzenenTreffer t = m_u.szenenPunktTesten(test, 0);   // Scene_forEachVisible_testselect_scene_point
    test.netzBeginnen(kEinheitsMatrix, true);
    if (!t.treffer.gueltig()) { return false; }
    punktAus = m_u.szenenPunktEingerastet(test, t);
    return true;
}

void ClipManipulator::punkteTesten(const Blick& v) {
    if (!zusatz().keine() && !schnellBedingung(zusatz(), v)) { return; }
    Treffer bester;
    Punkt* wahl = nullptr;
    for (Punkt& p : m_punkte) {   // SelectionPool: der naechste
        if (!p.gesetzt) { continue; }
        Treffer t;
        punktBesterPunkt(v.sicht(), Vektor3f(p.punkt), t);
        if (t.gueltig() && t < bester) {
            bester = t;
            wahl = &p;
        }
    }
    wahlWechsel(wahl);
}

void ClipManipulator::testen(const Blick& v, const Matrix4&) {
    if (!zusatz().keine() && !schnellBedingung(zusatz(), v)) { return; }
    punkteTesten(v);
    if (!gewaehlt()) {
        if (v.fuellen()) {
            Vektor3d p;
            if (szeneTesten(v, p)) { neuerPunkt(p, v); }
        } else {
            const Vektor3f mitte = projiziert(transformierterVektor4(vollInvers(v.sicht()), Vektor4f(0, 0, 0, 1)));
            Vektor3d p(mitte);
            einrasten(p, fang());
            const std::size_t maxi = groessteBetragsKomponente(v.blickrichtung());
            const std::size_t i = neuerIndex(false);
            const Kasten& b = m_u.grenzenWert;
            p[maxi] = b.origin[maxi] + (i == 2 ? -1.0F : 1.0F) * b.extents[maxi];
            neuerPunkt(p, v);
        }
    }
    for (Punkt& p : m_punkte) {
        if (p.gewaehlt) {
            p.ohneTransform = p.punkt;
            m_u.pivot = verschiebungsMatrix(Vektor3f(p.ohneTransform));
            break;
        }
    }
}

void ClipManipulator::transformieren(const Matrix4& m2o, const Matrix4& g, const GeraeteVektor& p) {
    // 2D oder 3D mit Zusatztasten (ausser SnapBounds): frei ziehen
    if (!(zusatz().keine() && blick().fuellen()) && !KastenFang::bedingung(zusatz(), blick())) {
        m_ziehen.transformieren(m2o, g, p);
        return;
    }
    Blick geschert(blick());
    auswahlTestBauen(geschert, auswahlKastenPunkt(p, m_u.geraetEps));
    Vektor3d neu;
    if (szeneTesten(geschert, neu)) {
        for (Punkt& q : m_punkte) {
            if (q.gewaehlt) {
                q.punkt = neu;
                ebeneAktualisieren();
                break;
            }
        }
    }
}

void ClipManipulator::zuruecksetzen(bool ausFlaeche) {
    for (Punkt& p : m_punkte) {
        p.gesetzt = false;
        p.gewaehlt = false;
    }
    Karte& k = m_u.karte;
    if (ausFlaeche && !k.auswahl.flaechen().empty()) {
        const auto f = k.auswahl.flaechen().back().flaeche.lock();
        const auto pinsel = k.auswahl.flaechen().back().pinsel.lock();
        if (f && pinsel) {
            pinsel->bRep(k.kontext);
            if (f->traegtBei()) {
                const Wicklung& w = f->wicklung();
                for (std::size_t i = 0; i < 3; ++i) {
                    m_punkte[i].gesetzt = true;
                    m_punkte[i].punkt = w[i].vertex;
                }
            }
        }
    }
    ebeneAktualisieren();
}

void ClipManipulator::zeichnen(const Blick& v, const Matrix4&, ManipZeichnung& aus) {   // render
    const Matrix4 proj = multipliziert(v.bildfeld(), v.sicht());
    const Matrix4 projInv = vollInvers(proj);
    for (const Punkt& p : m_punkte) {
        if (!p.gesetzt) { continue; }
        const ManipFarbe f = farbeGewaehlt(kFarbeBild, p.gewaehlt);   // UpdateColours
        aus.grossePunkte.push_back(ManipZeichnung::Punkt{Vektor3f(p.punkt), f});
        // der Name 2 Bildpunkte rechts vom Punkt (glRasterPos + drawChar)
        const Vektor3f lage = projiziert(transformierterVektor4(proj, Vektor4f(Vektor3f(p.punkt), 1))) + Vektor3f(2, 0, 0);
        aus.texte.push_back(ManipZeichnung::Text{projiziert(transformierterVektor4(projInv, Vektor4f(lage, 1))), std::string(1, p.name), f});
    }
}
}  // namespace

std::unique_ptr<Manipulator> AuswahlBearbeitung::Impl::neuerZiehManipulator() { return std::make_unique<ZiehManipulator>(*this); }
std::unique_ptr<Manipulator> AuswahlBearbeitung::Impl::neuerUvManipulator() { return std::make_unique<UvManipulator>(*this); }
std::unique_ptr<Manipulator> AuswahlBearbeitung::Impl::neuerClipManipulator() { return std::make_unique<ClipManipulator>(*this); }
void AuswahlBearbeitung::Impl::clipperZuruecksetzen(bool ausFlaeche) { static_cast<ClipManipulator*>(clipM.get())->zuruecksetzen(ausFlaeche); }

// Clipper_ok_plane / Clipper_ok (clippertool.cpp:56-62)
bool AuswahlBearbeitung::clipperEbeneOk() const {
    return modus() == ManipModus::Clipper && clipperPunkte.anzahl > 1 &&
           ebeneGueltig(ebeneAusPunkten(clipperPunkte.punkte[0], clipperPunkte.punkte[1], clipperPunkte.punkte[2]));
}
bool AuswahlBearbeitung::clipperOk() const { return modus() == ManipModus::Clipper && clipperPunkte.anzahl > 0; }

// Clipper_do (clippertool.cpp:121-130)
void AuswahlBearbeitung::clipperAusfuehren(bool teilen, const CsgUmgebung& u, Protokoll& p) {
    pinselTeilen(m->karte, u, clipperPunkte, clipperUmgedreht, clipperCaulk, teilen, p);
    if (clipperPunkteZuruecksetzen) {
        setzeModus(ManipModus::Clipper);   // setzt die Punkte so zurueck
        if (clipperUmdrehenZuruecksetzen) { clipperUmgedreht = false; }
    }
}

// Clipper_doFlip (clippertool.cpp:147-152)
void AuswahlBearbeitung::clipperUmdrehen() {
    if (clipperEbeneOk()) { clipperUmgedreht = !clipperUmgedreht; }
}
void AuswahlBearbeitung::Impl::uvFestschreiben() { static_cast<UvManipulator*>(uvM.get())->festschreiben(); }

std::vector<std::pair<std::shared_ptr<Entitaet>, std::shared_ptr<Pinsel>>> AuswahlBearbeitung::Impl::extrudierPinsel() const {
    std::vector<std::pair<std::shared_ptr<Entitaet>, std::shared_ptr<Pinsel>>> r;
    for (const auto& ent : karte.entitaeten()) {
        if (ent->versteckt) { continue; }
        for (const auto& p : ent->kinder()) {
            if (p->versteckt || p->art() != Primitiv::Art::Pinsel) { continue; }
            const auto pinsel = std::static_pointer_cast<Pinsel>(p);
            if (karte.auswahl.gewaehlt(ent.get(), p.get()) || pinselKomponentenGewaehlt(karte, *pinsel)) { r.emplace_back(ent, pinsel); }
        }
    }
    return r;
}

BesteEbene AuswahlBearbeitung::Impl::besteEbeneExtrudieren(AuswahlVolumen& test) {
    if (karte.auswahl.flaechen().empty()) { return besteEbene(test, true); }   // Scene_forEachSelectedBrush_bestPlane
    BesteEbene daten;
    const auto pinselListe = extrudierPinsel();
    for (const auto& [ent, pinsel] : pinselListe) {   // BrushInstance::bestPlaneDirect
        (void)ent;
        pinsel->bRep(karte.kontext);
        test.netzBeginnen(kEinheitsMatrix);
        for (const auto& f : pinsel->flaechen()) {
            Treffer t;
            testeFlaeche(*f, test, t);
            if (t < daten.treffer) {
                daten.treffer = t;
                daten.ebene = f->ebene();
            }
        }
    }
    if (!daten.gueltig()) {   // bestPlaneIndirect ueber die Kanten
        for (const auto& [ent, pinsel] : pinselListe) {
            Objekt o{ent, pinsel};
            besteEbeneKanten(test, o, daten);
        }
    }
    return daten;
}

void AuswahlBearbeitung::Impl::modusSetzen(ManipModus m) {   // SetManipulatorMode
    if (clipM && (m == ManipModus::Clipper || modus == ManipModus::Clipper)) {
        clipperZuruecksetzen(m == ManipModus::Clipper && modus != ManipModus::Clipper);
        if ((m == ManipModus::Clipper) != (modus == ManipModus::Clipper)) {   // Clipper_modeChanged (clippertool.cpp:101-115)
            if (aussen.clipperUmdrehenZuruecksetzen) { aussen.clipperUmgedreht = false; }
            if (m != ManipModus::Clipper) { aussen.clipperSchnell = false; }
        }
    }
    pivotEigen = false;
    modus = m;
    switch (m) {
    case ManipModus::Verschieben: manip = verschiebeM.get(); break;
    case ManipModus::Drehen: manip = drehM.get(); break;
    case ManipModus::Skalieren: manip = skalierM.get(); break;
    case ManipModus::Verzerren: manip = verzerrM.get(); break;
    case ManipModus::Ziehen: manip = ziehM.get(); break;
    case ManipModus::Bauen: manip = bauM.get(); break;
    case ManipModus::Clipper: manip = clipM.get(); break;   // resetTransforms( eClip ): mit Repeat Transforms
    case ManipModus::UV: manip = uvM.get(); break;
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
    mischen(komponentenWahlStand());
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
        k = komponentenGrenzen(karte);   // Scene_BoundsSelectedComponent
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
    bhed::karte::alleKomponentenAbwaehlen(karte);
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
        if (auto* f = dynamic_cast<Flaeche*>(o.get())) { f->bewegPunkteT = f->bewegPunkte; }   // revertTransform
    }
    s.veraendert = true;
}

void AuswahlBearbeitung::Impl::zuruecksetzen() {   // revertTransform
    std::set<Pinsel*> pinsel;
    for (auto& [schluessel, s] : sicherungen) {
        (void)schluessel;
        if (!s.veraendert) { continue; }
        s.objekt->zustandZuruecksetzen(*s.zustand);
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
    const bool sperre = aussen.texturEckenSperre;   // g_brush_textureVertexlock_enabled
    // foreachSelectedComponent: Pinsel und Patches mit gewaehlten Komponenten
    for (const auto& ent : karte.entitaeten()) {
        for (const auto& prim : ent->kinder()) {
            if (prim->art() == Primitiv::Art::Patch) {   // PatchInstance::transformComponents (patch.h:1590-1606)
                const auto patch = std::static_pointer_cast<Patch>(prim);
                if (!patch->punkteGewaehlt()) { continue; }
                vorAenderung(patch, nullptr);
                std::vector<PatchPunkt> punkte = patch->punkte();
                for (std::size_t i = 0; i < punkte.size(); ++i) {
                    if (patch->punktGewaehlt(i)) { transformierePunkt(mat, punkte[i].vertex); }
                }
                patch->punkteSetzen(punkte);   // UpdateCachedData
                continue;
            }
            const auto pinsel = std::static_pointer_cast<Pinsel>(prim);
            Pinsel* roh = pinsel.get();
            if (!roh->eckenModusAn() && !pinselKomponentenGewaehlt(karte, *roh)) { continue; }
            // BrushInstance::evaluateTransform (brush.h:3989-4040): eine neue
            // Transformation mit gewaehlten Ecken schaltet den Ecken-Modus ein.
            if (!roh->eckenModusAn() &&
                std::any_of(roh->flaechen().begin(), roh->flaechen().end(), [](const auto& f) { return !f->eckenWahl.empty(); })) {
                roh->eckenModusBeginnen();
                roh->eckenModusSammeln(karte.kontext);
                eckenModusPinsel.push_back(pinsel);
            }
            if (roh->eckenModusAn()) {   // vertexModeTransform (brush.cpp:511-516)
                vorAenderung(pinsel, nullptr);
                roh->eckenModusTransformieren(mat, karte.kontext, sperre);
                continue;
            }
            // BrushInstance::transformComponents (brush.cpp:525-567) mit
            // FaceInstance::transformComponents (brush.h:2850-2885)
            const auto anwenden = [&](const Matrix4& m) {
                roh->bRep(k);   // m_centroid_cached
                for (const auto& f : roh->flaechen()) {
                    const bool flaecheGewaehlt = karte.auswahl.flaecheGewaehlt(f.get());
                    if (!flaecheGewaehlt && f->kantenWahl.empty()) { continue; }
                    vorAenderung(f, roh);
                    const TexturProjektion altProjektion = f->projektion();
                    const Vektor3f altNormale(f->ebene().normal());
                    f->bewegPunkteT = f->bewegPunkte;   // revertTransform
                    if (flaecheGewaehlt) { f->transformieren(m, false, k); }
                    // gewaehlte Ecken: hier nie, sie schalten oben den Ecken-Modus ein
                    if (f->kantenWahl.size() == 1) {
                        transformierePunkt(m, f->bewegPunkteT[0]);
                        transformierePunkt(m, f->bewegPunkteT[1]);
                        f->setzeEbenenPunkte(f->bewegPunkteT);   // assign_planepts
                    } else if (f->kantenWahl.size() >= 2) {
                        transformierePunkt(m, f->bewegPunkteT[0]);
                        transformierePunkt(m, f->bewegPunkteT[1]);
                        transformierePunkt(m, f->bewegPunkteT[2]);
                        f->setzeEbenenPunkte(f->bewegPunkteT);
                    }
                    if (sperre && !f->kantenWahl.empty()) { f->texdefAusPunkten(k, altProjektion, altNormale); }
                }
                roh->geaendert();
            };
            const auto zurueck = [&]() {   // m_brush.revertTransform()
                for (const auto& f : roh->flaechen()) {
                    const auto it = sicherungen.find(f.get());
                    if (it == sicherungen.end()) { continue; }
                    Sicherung& s = it->second;
                    if (s.objekt && s.veraendert) {
                        s.objekt->zustandZuruecksetzen(*s.zustand);
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
            if (std::none_of(roh->flaechen().begin(), roh->flaechen().end(), [&](const auto& f) { return karte.auswahl.flaecheGewaehlt(f.get()); })) {
                continue;
            }
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
        if (komponentenZahl() != 0) { komponentenAnwenden(TArt::Drehen, t, pivot.t().vec3()); }
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
        if (komponentenZahl() != 0) { komponentenAnwenden(TArt::Skalieren, t, pivot.t().vec3()); }
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
        if (komponentenZahl() != 0) { komponentenAnwenden(TArt::Verzerren, t, pivot.t().vec3()); }
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

// testselect_scene_point (selection.cpp:608-623), testselect_scene_point_unselected
// (:625-643) und testselect_scene_point_selected_brushes (:645-660) mit
// ScenePointSelector (selection_selector.h:183-217).
AuswahlBearbeitung::Impl::SzenenTreffer AuswahlBearbeitung::Impl::szenenPunktTesten(AuswahlVolumen& test, int art) const {
    SzenenTreffer r;
    const auto dazu = [&](const Treffer& t, const Flaeche* f) {   // ScenePointSelector::addIntersection
        if (t < r.treffer) {
            r.treffer = t;
            r.flaeche = f;
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
        if (ent->versteckt) { continue; }
        if (art == 1 && gewaehlt.count(ent.get()) != 0) { continue; }
        if (!ent->istBehaelter()) {
            if (art == 2) { continue; }
            Treffer t;
            testeEntitaet(*ent, test, t);
            dazu(t, nullptr);
            continue;
        }
        for (const auto& pr : ent->kinder()) {
            if (pr->versteckt) { continue; }
            const bool istGewaehlt = gewaehlt.count(pr.get()) != 0;
            if ((art == 1 && istGewaehlt) || (art == 2 && !istGewaehlt)) { continue; }
            if (pr->art() == Primitiv::Art::Pinsel) {   // testselect_scene_point__brush
                const auto& pinsel = static_cast<const Pinsel&>(*pr);
                pinsel.bRep(karte.kontext);
                test.netzBeginnen(kEinheitsMatrix);
                for (const auto& f : pinsel.flaechen()) {
                    Treffer t;
                    testeFlaeche(*f, test, t);
                    dazu(t, f.get());
                }
            } else if (art != 2) {
                Treffer t;
                testePatch(static_cast<const Patch&>(*pr), test, t);
                dazu(t, nullptr);
            }
        }
    }
    return r;
}

Vektor3d AuswahlBearbeitung::Impl::szenenPunktEingerastet(AuswahlVolumen& test, const SzenenTreffer& t) const {
    const float fangW = aussen.fang;
    Vektor3d punktD(projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, t.treffer.tiefe(), 1))));
    if (t.flaeche != nullptr) {
        double besterAbstand = FLT_MAX;
        Vektor3d kandidat;
        const Wicklung& w = t.flaeche->wicklung();
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
        if (t.treffer.abstand() == 0) {   // Ebene, wenn ins Polygon gezeigt
            const Ebene3d& eb = t.flaeche->ebene();
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
    return punktD;
}

// testSelect_unselected_scene_point (selection.cpp:721-737)
std::optional<SzenenPunkt> AuswahlBearbeitung::Impl::ungewaehlterSzenenPunkt(const Blick& v, const GeraeteVektor& p, const GeraeteVektor& e) const {
    Blick geschert(v);
    auswahlTestBauen(geschert, auswahlKastenPunkt(p, e));
    AuswahlVolumen test(geschert);
    const SzenenTreffer t = szenenPunktTesten(test, 1);
    test.netzBeginnen(kEinheitsMatrix, true);
    if (!t.treffer.gueltig()) { return std::nullopt; }
    return SzenenPunkt{szenenPunktEingerastet(test, t), t.flaeche != nullptr ? std::optional<Ebene3d>(t.flaeche->ebene()) : std::nullopt};
}

// selection_selectVerticesOrFaceVertices (selection_mtor_drag.cpp:270-292)
bool AuswahlBearbeitung::Impl::eckenOderFlaechenEckenWaehlen(AuswahlVolumen& test) {
    {   // zuerst Ecken treffen (DeepBestSelector)
        BesterWaehler tief(2.F);
        szeneTestenKomponenten(karte, tief, test, KomponentenModus::Ecke, true);
        if (!tief.beste().empty()) {
            for (const Waehlbar& w : tief.beste()) { setzeGewaehlt(karte, w, true); }
            return true;
        }
    }
    // sonst die Ecken der Pinselflaechen auf der besten Ebene
    const BesteEbene daten = besteEbene(test, true);
    if (daten.gueltig()) {
        for (const auto& g : gewaehltePrimitiveListe(karte)) {   // BrushInstance::selectVerticesOnPlane (brush.h:3906-3912)
            if (g.primitiv->art() != Primitiv::Art::Pinsel || g.primitiv->versteckt || g.entitaet->versteckt) { continue; }
            auto& pinsel = static_cast<Pinsel&>(*g.primitiv);
            pinsel.bRep(karte.kontext);
            for (std::size_t fi = 0; fi < pinsel.anzahl(); ++fi) {
                const Ebene3d& e = pinsel[fi].ebene();
                if (!(ebenenGleich(daten.ebene, e) || ebenenGleich(daten.ebene, umgedreht(e)))) { continue; }
                const auto& ecken = pinsel.ecken(karte.kontext);
                for (std::size_t i = 0; i < ecken.size(); ++i) {   // VertexInstance::selectVerticesOfFace (brush.h:3264-3275)
                    const FlaechenPunkt start = ecken[i];
                    FlaechenPunkt fp = start;
                    do {
                        if (fp.flaeche == fi) {
                            pinsel.eckeWaehlen(i, true);
                            break;
                        }
                        // next_vertex
                        const std::size_t nf = pinsel[fp.flaeche].wicklung()[fp.punkt].adjacent;
                        const std::size_t np = wicklungFindeNachbar(pinsel[nf].wicklung(), fp.flaeche);
                        fp = np == kMaxFlaechen ? fp : FlaechenPunkt{nf, wicklungNaechster(pinsel[nf].wicklung(), np)};
                    } while (fp.flaeche != start.flaeche);
                }
            }
        }
    }
    return daten.gueltig();
}

// scene_insert_brush_vertices (selection_mtor_drag.cpp:295-337)
bool AuswahlBearbeitung::Impl::pinselEckenEinfuegen(const Blick& v, AuswahlVolumen& test, VerschiebeFreiXyZ& frei) {
    const SzenenTreffer t = szenenPunktTesten(test, v.fuellen() ? 0 : 2);
    test.netzBeginnen(kEinheitsMatrix, true);
    std::vector<Pinsel::EckenModusPunkt> neue;
    if (t.treffer.gueltig()) {
        frei.setze0(projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, t.treffer.tiefe(), 1))));
        Vektor3d punktD = szenenPunktEingerastet(test, t);
        if (!v.fuellen()) { punktD -= Vektor3d(v.blickrichtung()) * static_cast<double>(aussen.raster); }
        Pinsel::EckenModusPunkt e;
        e.ecke = e.neu = punktD;
        e.gewaehlt = true;
        if (t.flaeche != nullptr) {   // die Flaeche als Vorlage (ihr Shader und ihre Projektion)
            for (const auto& ent : karte.entitaeten()) {
                for (const auto& pr : ent->kinder()) {
                    if (pr->art() != Primitiv::Art::Pinsel) { continue; }
                    for (const auto& f : static_cast<const Pinsel&>(*pr).flaechen()) {
                        if (f.get() == t.flaeche) { e.flaechen.push_back(f); }
                    }
                }
            }
        }
        neue.push_back(std::move(e));
    } else if (!v.fuellen()) {   // zwei Punkte durch die ganze Tiefe der Auswahl
        frei.setze0(Vektor3f(0, 0, 0));
        const Kasten b = auswahlGrenzenPivot(karte);   // GlobalSelectionSystem().getBoundsSelected()
        if (!kastenGueltig(b)) { return false; }
        Vektor3d xy(projiziert(transformierterVektor4(test.bildZuWelt(), Vektor4f(0, 0, 0, 1))));
        einrasten(xy, aussen.fang);
        Vektor3d a = xy;
        Vektor3d c = xy;
        const std::size_t maxi = groessteBetragsKomponente(v.blickrichtung());
        a[maxi] = b.origin[maxi] + b.extents[maxi];
        c[maxi] = b.origin[maxi] - b.extents[maxi];
        Pinsel::EckenModusPunkt ea;
        ea.ecke = ea.neu = a;
        ea.gewaehlt = true;
        Pinsel::EckenModusPunkt ec;
        ec.ecke = ec.neu = c;
        ec.gewaehlt = true;
        neue.push_back(std::move(ea));
        neue.push_back(std::move(ec));
    } else {
        return false;
    }
    Befehl befehl(karte.undo, "InsertBrushVertices");
    for (const auto& g : gewaehltePrimitiveListe(karte)) {   // Scene_forEachSelectedBrush
        if (g.primitiv->art() != Primitiv::Art::Pinsel) { continue; }
        const auto pinsel = std::static_pointer_cast<Pinsel>(g.primitiv);
        eckenEinfuegen(*pinsel, neue, karte.kontext, aussen.texturEckenSperre);
        if (pinsel->eckenModusAn()) { eckenModusPinsel.push_back(pinsel); }
    }
    return true;
}

// gatherComponentsHighlight: BrushInstance (brush.h:3723-3787), EdgeInstance
// (:3160-3168), VertexInstance (:3254-3262), PatchInstance (patch.h:1541-1563)
void AuswahlBearbeitung::Impl::komponentenHervorheben(AuswahlVolumen& test, KomponentenModus kmodus, std::vector<std::vector<Vektor3f>>& polygone) {
    Treffer schnitt;
    const auto naeher = [&](const Treffer& t, const Vektor3f& p) {
        if (t < schnitt) {
            schnitt = t;
            polygone.clear();
            polygone.push_back({p});
        }
    };
    for (const auto& g : gewaehltePrimitiveListe(karte)) {   // Scene_forEachVisibleSelectedComponentSelectionTestable
        if (g.primitiv->versteckt || g.entitaet->versteckt) { continue; }
        if (g.primitiv->art() == Primitiv::Art::Patch) {
            if (kmodus != KomponentenModus::Ecke) { continue; }
            const auto& patch = static_cast<const Patch&>(*g.primitiv);
            test.netzBeginnen(kEinheitsMatrix);
            for (const auto& pk : patch.punkte()) {
                Treffer t;
                test.testPunkt(pk.vertex, t);
                naeher(t, pk.vertex);
            }
            continue;
        }
        const auto& pinsel = static_cast<const Pinsel&>(*g.primitiv);
        pinsel.bRep(karte.kontext);
        test.netzBeginnen(kEinheitsMatrix);
        switch (kmodus) {
        case KomponentenModus::Ecke:
            for (const FlaechenPunkt& fp : pinsel.ecken(karte.kontext)) {
                const Vektor3f p(pinsel.eckpunkt(fp));
                Treffer t;
                test.testPunkt(p, t);
                naeher(t, p);
            }
            break;
        case KomponentenModus::Kante:
            for (const FlaechenPunkt& fp : pinsel.kanten(karte.kontext)) {
                const Wicklung& w = pinsel[fp.flaeche].wicklung();
                const Vektor3f p(mitte(w[fp.punkt].vertex, w[wicklungNaechster(w, fp.punkt)].vertex));
                Treffer t;
                test.testPunkt(p, t);
                naeher(t, p);
            }
            break;
        case KomponentenModus::Flaeche:
            if (test.blick().fuellen()) {
                const Ebene3d* ebene = nullptr;
                bool neuerTreffer = false;
                for (const auto& f : pinsel.flaechen()) {
                    Treffer t;
                    testeFlaeche(*f, test, t);
                    if (t.gueltig() && schnitt.gleichEps(t, 0.25F, 2e-6F)) {
                        ebene = &f->ebene();
                    } else if (t < schnitt) {
                        schnitt = t;
                        neuerTreffer = true;
                        ebene = &f->ebene();
                    }
                }
                if (ebene != nullptr) {
                    if (neuerTreffer || (!polygone.empty() && polygone.back().size() >= 3 &&
                                         !ebenenGleich(*ebene, ebeneAusPunkten(polygone.back()[0], polygone.back()[1], polygone.back()[2])))) {
                        polygone.clear();
                    }
                    for (const auto& f : pinsel.flaechen()) {   // gatherPolygonsByPlane( plane, polygons, false )
                        if (ebenenGleich(*ebene, f->ebene()) && f->traegtBei()) {
                            const Wicklung& w = f->wicklung();
                            std::vector<Vektor3f> poly(w.numpoints);
                            for (std::size_t i = 0; i < w.numpoints; ++i) { poly[i] = Vektor3f(w[i].vertex); }
                            polygone.push_back(std::move(poly));
                        }
                    }
                }
            } else {
                for (const auto& f : pinsel.flaechen()) {   // testSelect_centroid
                    Treffer t;
                    test.testPunkt(f->schwerpunkt(), t);
                    naeher(t, f->schwerpunkt());
                }
            }
            break;
        default: break;
        }
    }
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

// BrushInstance::bestPlaneIndirect (brush.h:3863-3870) mit EdgeInstance::bestPlaneIndirect (:3169-3206)
void AuswahlBearbeitung::Impl::besteEbeneKanten(AuswahlVolumen& test, const Objekt& o, BesteEbene& daten) {
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
}

// Scene_forEachPlaneSelectable_bestPlane
BesteEbene AuswahlBearbeitung::Impl::besteEbene(AuswahlVolumen& test, bool nurPinsel) {
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
            besteEbeneKanten(test, o, daten);
        } else {
            const auto patch = std::static_pointer_cast<Patch>(o.prim);
            test.netzBeginnen(kEinheitsMatrix);
            patchEbenen[patch.get()].patch = patch;
            patchEbenen[patch.get()].flaechen.besteEbeneIndirekt(patch->grenzen(), test, daten);
        }
    };
    const auto passt = [nurPinsel](const Objekt& o) {
        return o.prim && !o.prim->versteckt && !o.ent->versteckt && (!nurPinsel || o.prim->art() == Primitiv::Art::Pinsel);
    };
    for (const Objekt& o : objekte) {
        if (passt(o)) { direkt(o); }
    }
    if (!daten.gueltig()) {
        for (const Objekt& o : objekte) {
            if (passt(o)) { indirekt(o); }
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
    if (modus() == ManipModus::UV) {   // m_uv_manipulator->freezeTransform()
        m->uvFestschreiben();
    } else {
        m->festschreiben();   // freezeTransforms
    }
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
