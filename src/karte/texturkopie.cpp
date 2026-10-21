// texturkopie.cpp - Karten-Editor: Texturen mit der mittleren Maustaste
// kopieren und auftragen. Portiert aus NetRadiant-Custom 20260114 (Quellen
// in texturkopie.h).
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
#include "bhed/karte/texturkopie.h"

#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/manipulator.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <unordered_set>

namespace bhed::karte {

namespace {

// Instance_isSelected der Primitive
std::unordered_set<const Primitiv*> gewaehltePrimitive(const Karte& k) {
    std::unordered_set<const Primitiv*> s;
    for (const auto& el : k.auswahl.elemente()) {
        if (const auto p = el.primitiv.lock()) { s.insert(p.get()); }
    }
    return s;
}

// line_closest_point (libs/math/line.h)
Vektor3f linieNaechsterPunkt(const Vektor3f& anfang, const Vektor3f& ende, const Vektor3f& p) {
    const Vektor3f v = ende - anfang;
    const Vektor3f w = p - anfang;
    const double c1 = punkt(w, v);
    if (c1 <= 0) { return anfang; }
    const double c2 = punkt(v, v);
    if (c2 <= c1) { return ende; }
    return Vektor3f(anfang + v * static_cast<float>(c1 / c2));
}

// PatchData
struct PatchDaten {
    std::size_t breite = 0;
    std::size_t hoehe = 0;
    const PatchPunkt* punkte = nullptr;
    [[nodiscard]] const PatchPunkt& bei(std::size_t r, std::size_t c) const { return punkte[(r * breite) + c]; }
};

// PatchEdgeIter (surfacedialog.cpp:1180-1265)
class RandSchritt {
public:
    enum class Art { ZeileVor, ZeileZurueck, SpalteVor, SpalteZurueck };   // eRowForward ...
    RandSchritt(const PatchDaten& p, Art art, int zeileOderSpalte)
        : m_p(p), m_art(art),
          m_zeile(art == Art::SpalteVor       ? 0
                  : art == Art::SpalteZurueck ? static_cast<int>(p.hoehe) - 1
                                              : zeileOderSpalte),
          m_spalte(art == Art::ZeileVor       ? 0
                   : art == Art::ZeileZurueck ? static_cast<int>(p.breite) - 1
                                              : zeileOderSpalte) {}
    RandSchritt(const RandSchritt& o, Art art) : m_p(o.m_p), m_art(art), m_zeile(o.m_zeile), m_spalte(o.m_spalte) {}
    [[nodiscard]] const PatchPunkt& operator*() const {
        return m_p.bei(static_cast<std::size_t>(m_zeile), static_cast<std::size_t>(m_spalte));
    }
    explicit operator bool() const {
        return m_zeile >= 0 && m_zeile < static_cast<int>(m_p.hoehe) && m_spalte >= 0 && m_spalte < static_cast<int>(m_p.breite);
    }
    RandSchritt& operator+=(int n) {
        switch (m_art) {
        case Art::ZeileVor: m_spalte += n; break;
        case Art::ZeileZurueck: m_spalte -= n; break;
        case Art::SpalteVor: m_zeile += n; break;
        case Art::SpalteZurueck: m_zeile -= n; break;
        }
        return *this;
    }
    [[nodiscard]] RandSchritt plus(int n) const {
        RandSchritt r(*this);
        r += n;
        return r;
    }
    [[nodiscard]] RandSchritt quer() const {   // getCrossIter
        switch (m_art) {
        case Art::ZeileVor: return RandSchritt(*this, Art::SpalteZurueck);
        case Art::ZeileZurueck: return RandSchritt(*this, Art::SpalteVor);
        case Art::SpalteVor: return RandSchritt(*this, Art::ZeileVor);
        case Art::SpalteZurueck: break;
        }
        return RandSchritt(*this, Art::ZeileZurueck);
    }

private:
    const PatchDaten& m_p;
    Art m_art;
    int m_zeile;
    int m_spalte;
};

// Patch_getClosestTriangle (surfacedialog.cpp:1268-1349): 0 oder 3 Punkte (CW)
std::vector<const PatchPunkt*> naechstesDreieck(const PatchDaten& patch, const Wicklung& w, const Ebene3d& ebene) {
    const auto dreieckOk = [](const PatchPunkt& p0, const PatchPunkt& p1, const PatchPunkt& p2) {
        return laengeQuadrat(kreuz(p1.vertex - p0.vertex, p2.vertex - p0.vertex)) > 1;
    };
    const double eps = .25;
    std::vector<const PatchPunkt*> ergebnis;
    const auto suchen = [&](const auto& pruefen) {
        using A = RandSchritt::Art;
        for (const RandSchritt& start : {RandSchritt(patch, A::ZeileZurueck, 0), RandSchritt(patch, A::ZeileVor, static_cast<int>(patch.hoehe) - 1),
                                         RandSchritt(patch, A::SpalteZurueck, static_cast<int>(patch.breite) - 1),
                                         RandSchritt(patch, A::SpalteVor, 0)}) {
            for (RandSchritt i0 = start; i0; i0 += 2) {
                const PatchPunkt& p0 = *i0;
                if (!pruefen(p0)) { continue; }
                for (RandSchritt i1 = i0.plus(2); i1; i1 += 2) {
                    const PatchPunkt& p1 = *i1;
                    if (!pruefen(p1) || laengeQuadrat(p1.vertex - p0.vertex) <= eps) { continue; }
                    for (RandSchritt i2 = i0.quer().plus(1), i22 = i1.quer().plus(1); i2 && i22; i2 += 1, i22 += 1) {
                        for (const PatchPunkt* p2 : {&*i2, &*i22}) {
                            if (dreieckOk(p0, p1, *p2)) {
                                ergebnis = {&p0, &p1, p2};
                                return;
                            }
                        }
                    }
                }
            }
        }
    };
    // try patchControls-on-edge
    for (std::size_t i = w.numpoints - 1, j = 0; j < w.numpoints && ergebnis.empty(); i = j, ++j) {
        const Vektor3f a(w[i].vertex);
        const Vektor3f b(w[j].vertex);
        suchen([&](const PatchPunkt& p) { return laengeQuadrat(linieNaechsterPunkt(a, b, p.vertex) - p.vertex) < eps; });
    }
    // try patchControls-on-edgeLine
    for (std::size_t i = w.numpoints - 1, j = 0; j < w.numpoints && ergebnis.empty(); i = j, ++j) {
        const Strahld s = strahlAusPunkten(Vektor3d(w[i].vertex), Vektor3d(w[j].vertex));
        suchen([&](const PatchPunkt& p) { return strahlAbstandQuadrat(s, p.vertex) < eps; });
    }
    // try patchControls-on-facePlane
    if (ergebnis.empty()) {
        suchen([&](const PatchPunkt& p) { return std::pow(abstandZuPunkt(ebene, p.vertex), 2) < eps; });
    }
    return ergebnis;
}

// matrix4_pivoted_rotate_by_quaternion
Matrix4 umPunktGedreht(const Quaternion& q, const Vektor3f& punktP) {
    Matrix4 m = kEinheitsMatrix;
    verschiebeUm(m, punktP);
    multipliziere(m, drehungAusQuaternion(q));
    verschiebeUm(m, -punktP);
    return m;
}

void flaecheHolen(const Flaeche& f, const Kontext& k, std::string& shader, FlaechenTextur& ablage) {   // Face_getTexture
    shader = f.shader();
    ablage.projektion = f.texdefHolen(k);
    ablage.flags = f.flags();
    ablage.ebene = f.ebene();
    ablage.wicklung = f.wicklung();
    const auto [w, h] = k.groesse(f.shader());
    ablage.breite = w;
    ablage.hoehe = h;
    ablage.quelle = FlaechenTextur::Quelle::Pinsel;   // m_colour: die Durchschnittsfarbe braucht nur das Licht-Einfuegen nicht
}

void patchHolen(const Patch& p, const Kontext& k, std::string& shader, FlaechenTextur& ablage) {   // Patch_getTexture
    shader = p.shader();
    ablageStandard(ablage, k.art());
    const auto [w, h] = k.groesse(p.shader());
    ablage.breite = w;
    ablage.hoehe = h;
    ablage.patchBreite = p.breite();
    ablage.patchHoehe = p.hoehe();
    ablage.patchPunkte = p.punkte();
    ablage.quelle = FlaechenTextur::Quelle::Patch;
}

void lichtHolen(const Entitaet& e, const std::string& browserShader, std::string& shader, FlaechenTextur& ablage) {   // Light_getTexture
    float c[3] = {ablage.farbe[0], ablage.farbe[1], ablage.farbe[2]};
    if (std::sscanf(e.wert("_color").c_str(), "%f %f %f", &c[0], &c[1], &c[2]) == 3) { ablage.farbe = Vektor3f(c[0], c[1], c[2]); }
    float l = 0;
    if (std::sscanf(e.wert("_light").c_str(), "%f", &l) == 1 || std::sscanf(e.wert("light").c_str(), "%f", &l) == 1) { ablage.licht = l; }
    shader = browserShader;   // preserve shader
}

void flaecheSetzen(Flaeche& f, const Kontext& k, const std::string& shader, FlaechenTextur& ablage, EinfuegeArt art,
                   bool shaderSetzen) {   // Face_setTexture
    if (shaderSetzen) {
        f.setzeShader(shader, k);
        f.setzeFlags(ablage.flags);
    }
    if (art == EinfuegeArt::Werte) {
        f.setzeTexdef(ablage.projektion, k, false);
    } else if (art == EinfuegeArt::Projizieren) {
        f.projiziereTextur(ablage.projektion, Vektor3f(ablage.ebene.normal()), k);
    } else if (art == EinfuegeArt::Nahtlos) {
        if (ablage.quelle == FlaechenTextur::Quelle::Pinsel) {
            const Strahld linie = ebenenSchnitt(ablage.ebene, f.ebene());
            if (laengeQuadrat(linie.direction) <= 1e-10) {
                f.projiziereTextur(ablage.projektion, Vektor3f(ablage.ebene.normal()), k);
                return;
            }
            const Quaternion drehung = quaternionAusEinheitsvektoren(Vektor3f(ablage.ebene.normal()), Vektor3f(f.ebene().normal()));
            const Matrix4 transform = umPunktGedreht(drehung, Vektor3f(linie.origin));
            TexturProjektion proj = ablage.projektion;
            proj.bp.addScale(ablage.breite, ablage.hoehe);
            texdefTransformiertGesperrt(k.art(), proj, ablage.breite, ablage.hoehe, ablage.ebene, transform, Vektor3f(linie.origin));
            proj.bp.removeScale(ablage.breite, ablage.hoehe);
            f.setzeTexdef(proj, k);
            std::string egal;
            flaecheHolen(f, k, egal, ablage);
        } else {
            const PatchDaten pd{ablage.patchBreite, ablage.patchHoehe, ablage.patchPunkte.data()};
            const auto pc = naechstesDreieck(pd, f.wicklung(), f.ebene());
            if (pc.empty()) { return; }
            EbenenPunkte punkte = {Vektor3d(pc[0]->vertex), Vektor3d(pc[1]->vertex), Vektor3d(pc[2]->vertex)};
            const Vektor3d st[3] = {Vektor3d(pc[0]->texcoord[0], pc[0]->texcoord[1], 0), Vektor3d(pc[1]->texcoord[0], pc[1]->texcoord[1], 0),
                                    Vektor3d(pc[2]->texcoord[0], pc[2]->texcoord[1], 0)};
            {   // rotate patch points to face plane
                const Ebene3d ebene = ebeneAusPunkten(punkte[0], punkte[1], punkte[2]);
                const Strahld linie = ebenenSchnitt(f.ebene(), ebene);
                if (laengeQuadrat(linie.direction) > 1e-10) {
                    const Quaternion drehung = quaternionAusEinheitsvektoren(Vektor3f(ebene.normal()), Vektor3f(f.ebene().normal()));
                    const Matrix4 rot = umPunktGedreht(drehung, Vektor3f(linie.origin));
                    for (auto& v : punkte) { v = Vektor3d(transformierterPunkt(rot, Vektor3f(v))); }
                }
            }
            TexturProjektion proj;
            texdefAusSt(k.art(), proj, punkte, st, ablage.breite, ablage.hoehe);
            proj.bp.removeScale(ablage.breite, ablage.hoehe);
            f.setzeTexdef(proj, k);
            std::string egal;
            flaecheHolen(f, k, egal, ablage);
        }
    }
}

void patchSetzen(Patch& p, const Kontext& k, const std::string& shader, FlaechenTextur& ablage, EinfuegeArt art,
                 bool shaderSetzen) {   // Patch_setTexture
    if (shaderSetzen) { p.setzeShader(shader); }
    if (art == EinfuegeArt::Projizieren) {
        p.texturProjizieren(ablage.projektion, Vektor3f(ablage.ebene.normal()), k);
    } else if (art == EinfuegeArt::Nahtlos) {
        const std::vector<PatchPunkt> kopie = p.punkte();
        const PatchDaten pd{p.breite(), p.hoehe(), kopie.data()};
        const auto pc = naechstesDreieck(pd, ablage.wicklung, ablage.ebene);
        if (pc.empty()) { return; }
        EbenenPunkte punkte = {Vektor3d(pc[0]->vertex), Vektor3d(pc[1]->vertex), Vektor3d(pc[2]->vertex)};
        const Vektor3d st[3] = {Vektor3d(pc[0]->texcoord[0], pc[0]->texcoord[1], 0), Vektor3d(pc[1]->texcoord[0], pc[1]->texcoord[1], 0),
                                Vektor3d(pc[2]->texcoord[0], pc[2]->texcoord[1], 0)};
        Matrix4 lokalZuTex0;   // face tex projection
        {
            TexturProjektion proj0 = ablage.projektion;
            proj0.bp.addScale(ablage.breite, ablage.hoehe);
            lokalZuTextur(k.art(), proj0, ablage.breite, ablage.hoehe, Vektor3f(ablage.ebene.normal()), lokalZuTex0);
        }
        {   // rotate patch points to face plane
            const Ebene3d ebene = ebeneAusPunkten(punkte[0], punkte[1], punkte[2]);
            const Strahld linie = ebenenSchnitt(ablage.ebene, ebene);
            if (laengeQuadrat(linie.direction) > 1e-10) {
                const Quaternion drehung = quaternionAusEinheitsvektoren(Vektor3f(ebene.normal()), Vektor3f(ablage.ebene.normal()));
                const Matrix4 rot = umPunktGedreht(drehung, Vektor3f(linie.origin));
                for (auto& v : punkte) { v = Vektor3d(transformierterPunkt(rot, Vektor3f(v))); }
            }
        }
        Matrix4 lokalZuTex;   // patch BP tex projection
        lokalZuTexturAusSt(punkte, st, lokalZuTex);
        Matrix4 texZuLokal = affinInvers(lokalZuTex);
        // adjust t() so that st->world points get to the plane
        const float d = static_cast<float>(ablage.ebene.dist());
        texZuLokal.tx() += texZuLokal.zx() * d;
        texZuLokal.ty() += texZuLokal.zy() * d;
        texZuLokal.tz() += texZuLokal.zz() * d;
        p.texturkoordinatenTransformieren(multipliziert(lokalZuTex0, texZuLokal));   // unproject st->world, project to new st
        // Patch_getTexture (Teil)
        const auto [w, h] = k.groesse(p.shader());
        ablage.breite = w;
        ablage.hoehe = h;
        ablage.patchBreite = p.breite();
        ablage.patchHoehe = p.hoehe();
        ablage.patchPunkte = p.punkte();
        ablage.quelle = FlaechenTextur::Quelle::Patch;
    }
}

void lichtSetzen(Entitaet& e, const FlaechenTextur& ablage, EinfuegeArt art) {   // Light_setTexture
    char wert[64];
    if (art == EinfuegeArt::Nahtlos || art == EinfuegeArt::Projizieren) {
        std::snprintf(wert, sizeof(wert), "%g %g %g", static_cast<double>(ablage.farbe[0]), static_cast<double>(ablage.farbe[1]),
                      static_cast<double>(ablage.farbe[2]));
        e.setzen("_color", wert);
    }
    if (art == EinfuegeArt::Werte || art == EinfuegeArt::Projizieren) {   // write_intensity
        std::snprintf(wert, sizeof(wert), "%g", static_cast<double>(ablage.licht));
        if (e.hat("_light")) {
            e.setzen("_light", wert);
        } else {
            e.setzen("light", wert);
        }
    }
}

}  // namespace

FlaechenTextur::FlaechenTextur() {
    projektion.basisS = Vektor3f(0.7071067811865F, 0.7071067811865F, 0);
    projektion.basisT = Vektor3f(-0.4082482904639F, 0.4082482904639F, -0.4082482904639F * 2.0F);
}

EinfuegeArt einfuegeArtFuer(bool shift, bool ctrl) {
    if (shift) { return ctrl ? EinfuegeArt::Projizieren : EinfuegeArt::Werte; }
    if (ctrl) { return EinfuegeArt::Nahtlos; }
    return EinfuegeArt::Nichts;
}

bool einfuegenMitShader(EinfuegeArt a, bool alt) { return a == EinfuegeArt::Nichts || !alt; }

std::string texturAuftragenName(bool shift, bool ctrl, bool alt) {
    const EinfuegeArt art = einfuegeArtFuer(shift, ctrl);
    const bool shader = einfuegenMitShader(art, alt);
    switch (art) {
    case EinfuegeArt::Werte: return shader ? "paintTexture,Values,LightPower" : "paintTexDefValues";
    case EinfuegeArt::Nahtlos: return shader ? "paintTextureSeamless,LightColor" : "paintTexDefValuesSeamless";
    case EinfuegeArt::Projizieren: return shader ? "projectTexture,LightColor&Power" : "projectTexDefValues";
    case EinfuegeArt::Nichts: break;
    }
    return "paintTexture";
}

void ablageStandard(FlaechenTextur& f, TexdefArt art) {   // FaceTextureClipboard_setDefault
    f.flags = InhaltFlags{0, 0, 0, false};
    f.projektion.texdef = Texdef();
    f.projektion.bp = BpTexdef();
    texdefStandard(art, f.projektion);
}

Texturierbar naechstesTexturierbar(const Karte& k, AuswahlVolumen& test) {
    Texturierbar t;
    Treffer bester;
    for (const auto& ent : k.entitaeten()) {
        if (ent->versteckt) { continue; }   // !path.top().get().visible(): auch die Kinder nicht
        if (!ent->istBehaelter()) {
            // SelectionTestable der Punktentitaet mit OccludeSelector
            Treffer t2;
            testeEntitaet(*ent, test, t2);
            if (t2 < bester) {
                bester = t2;
                const std::string kl = ent->klassenname();
                if (kl.compare(0, 5, "light") == 0) {   // string_equal_n( classname, "light", 5 )
                    t = Texturierbar{};
                    t.art = Texturierbar::Art::Licht;
                    t.licht = ent;
                } else {
                    t = Texturierbar{};
                }
            }
        }
        for (const auto& p : ent->kinder()) {
            if (p->versteckt) { continue; }
            if (p->art() == Primitiv::Art::Pinsel) {
                const auto pinsel = std::static_pointer_cast<Pinsel>(p);
                test.netzBeginnen(kEinheitsMatrix);   // m_test.BeginMesh( brush->localToWorld() )
                pinsel->bRep(k.kontext);
                for (const auto& f : pinsel->flaechen()) {   // Face_getClosest (isFiltered: Stufe 2)
                    Treffer t2;
                    testeFlaeche(*f, test, t2);
                    if (t2.gueltig() && t2 < bester) {
                        bester = t2;
                        t = Texturierbar{};
                        t.art = Texturierbar::Art::Flaeche;
                        t.pinsel = pinsel;
                        t.flaeche = f;
                    }
                }
            } else {
                Treffer t2;
                testePatch(static_cast<const Patch&>(*p), test, t2);
                if (t2 < bester) {   // occluded
                    bester = t2;
                    t = Texturierbar{};
                    t.art = Texturierbar::Art::Patch;
                    t.patch = std::static_pointer_cast<Patch>(p);
                }
            }
        }
    }
    return t;
}

bool naechsteTexturHolen(const Karte& k, AuswahlVolumen& test, const std::string& browserShader, std::string& shader,
                         FlaechenTextur& ablage) {
    const Texturierbar t = naechstesTexturierbar(k, test);
    switch (t.art) {
    case Texturierbar::Art::Flaeche: flaecheHolen(*t.flaeche, k.kontext, shader, ablage); return true;
    case Texturierbar::Art::Patch: patchHolen(*t.patch, k.kontext, shader, ablage); return true;
    case Texturierbar::Art::Licht: lichtHolen(*t.licht, browserShader, shader, ablage); return true;
    case Texturierbar::Art::Nichts: break;
    }
    return false;
}

void naechsteTexturSetzen(Karte& k, AuswahlVolumen& test, const std::string& shader, FlaechenTextur& ablage, EinfuegeArt art,
                          bool shaderSetzen) {
    const Texturierbar t = naechstesTexturierbar(k, test);
    switch (t.art) {
    case Texturierbar::Art::Flaeche: flaecheSetzen(*t.flaeche, k.kontext, shader, ablage, art, shaderSetzen); break;
    case Texturierbar::Art::Patch: patchSetzen(*t.patch, k.kontext, shader, ablage, art, shaderSetzen); break;
    case Texturierbar::Art::Licht: lichtSetzen(*t.licht, ablage, art); break;
    case Texturierbar::Art::Nichts: break;
    }
}

void texdefSetzenAuswahl(Karte& k, const AuswahlEinstellung& e, const TexturProjektion& p, bool basisSetzen, bool basisZuruecksetzen) {
    const auto gewaehlt = gewaehltePrimitive(k);
    if (e.modus != AuswahlModus::Komponente) {   // Scene_BrushSetTexdef_Selected
        for (const auto& ent : k.entitaeten()) {
            for (const auto& pr : ent->kinder()) {
                if (pr->art() != Primitiv::Art::Pinsel || gewaehlt.count(pr.get()) == 0U) { continue; }
                for (const auto& f : static_cast<Pinsel&>(*pr).flaechen()) { f->setzeTexdef(p, k.kontext, basisSetzen, basisZuruecksetzen); }
            }
        }
    }
    for (const auto& fw : k.auswahl.flaechen()) {   // Scene_BrushSetTexdef_Component_Selected
        if (const auto f = fw.flaeche.lock()) { f->setzeTexdef(p, k.kontext, basisSetzen, basisZuruecksetzen); }
    }
}

void texturProjizierenAuswahl(Karte& k, const AuswahlEinstellung& e, const TexturProjektion& p, const Vektor3f& normal) {
    const auto gewaehlt = gewaehltePrimitive(k);
    if (e.modus != AuswahlModus::Komponente) {
        for (const auto& ent : k.entitaeten()) {
            for (const auto& pr : ent->kinder()) {
                if (gewaehlt.count(pr.get()) == 0U) { continue; }
                if (pr->art() == Primitiv::Art::Pinsel) {   // Scene_BrushProjectTexture_Selected
                    for (const auto& f : static_cast<Pinsel&>(*pr).flaechen()) { f->projiziereTextur(p, normal, k.kontext); }
                } else if (!ent->versteckt && !pr->versteckt) {   // Scene_PatchProjectTexture_Selected (sichtbare)
                    static_cast<Patch&>(*pr).texturProjizieren(p, normal, k.kontext);
                }
            }
        }
    }
    for (const auto& fw : k.auswahl.flaechen()) {   // Scene_BrushProjectTexture_Component_Selected
        if (const auto f = fw.flaeche.lock()) { f->projiziereTextur(p, normal, k.kontext); }
    }
}

void patchTexturProjizieren(Patch& p, TexturProjektion proj, const Vektor3f& normal, const Kontext& k) {
    p.texturProjizieren(proj, normal, k);
}

// Scene_applyClosestTexture (surfacedialog.cpp:1683-1702)
void naechsteTexturAuftragen(Karte& k, const AuswahlEinstellung& e, AuswahlVolumen& test, bool shift, bool ctrl, bool alt,
                             bool auswahlTexturieren, const std::string& browserShader, FlaechenTextur& ablage) {
    const EinfuegeArt art = einfuegeArtFuer(shift, ctrl);
    const bool shader = einfuegenMitShader(art, alt);
    if (auswahlTexturieren) {
        if (shader && art != EinfuegeArt::Nahtlos) {   // Select_SetShader
            const auto gewaehlt = gewaehltePrimitive(k);
            if (e.modus != AuswahlModus::Komponente) {
                for (const auto& ent : k.entitaeten()) {
                    for (const auto& pr : ent->kinder()) {
                        if (gewaehlt.count(pr.get()) == 0U) { continue; }
                        if (pr->art() == Primitiv::Art::Pinsel) {
                            for (const auto& f : static_cast<Pinsel&>(*pr).flaechen()) { f->setzeShader(browserShader, k.kontext); }
                        } else if (!ent->versteckt && !pr->versteckt) {
                            static_cast<Patch&>(*pr).setzeShader(browserShader);
                        }
                    }
                }
            }
            for (const auto& fw : k.auswahl.flaechen()) {
                if (const auto f = fw.flaeche.lock()) { f->setzeShader(browserShader, k.kontext); }
            }
        }
        if (art == EinfuegeArt::Werte) {
            texdefSetzenAuswahl(k, e, ablage.projektion, false, false);
        } else if (art == EinfuegeArt::Projizieren) {
            texturProjizierenAuswahl(k, e, ablage.projektion, Vektor3f(ablage.ebene.normal()));
        }
    }
    naechsteTexturSetzen(k, test, browserShader, ablage, art, shader);
}

}  // namespace bhed::karte
