// texturbefehle.cpp - Karten-Editor: die Textur-Befehle der Auswahl.
// Portiert aus NetRadiant-Custom 20260114 (Quellen in texturbefehle.h).
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
#include "bhed/karte/texturbefehle.h"

#include "bhed/karte/bearbeitung.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/komponenten.h"

#include <cmath>
#include <memory>
#include <vector>

namespace bhed::karte {

namespace {

// Scene_forEachSelectedBrush (foreachSelected): die gewaehlten Pinsel
std::vector<std::shared_ptr<Pinsel>> gewaehltePinsel(const Karte& k) {
    std::vector<std::shared_ptr<Pinsel>> r;
    for (const GewaehltesPrimitiv& g : gewaehltePrimitiveListe(k)) {
        if (g.primitiv->art() == Primitiv::Art::Pinsel) { r.push_back(std::static_pointer_cast<Pinsel>(g.primitiv)); }
    }
    return r;
}

// Scene_forEachVisibleSelectedPatch
std::vector<std::shared_ptr<Patch>> gewaehltePatches(const Karte& k) {
    std::vector<std::shared_ptr<Patch>> r;
    for (const GewaehltesPrimitiv& g : gewaehltePrimitiveListe(k)) {
        if (g.primitiv->art() == Primitiv::Art::Patch && g.primitiv->sichtbar() && g.entitaet->sichtbar()) {
            r.push_back(std::static_pointer_cast<Patch>(g.primitiv));
        }
    }
    return r;
}

// Scene_ForEachSelectedBrush_ForEachFace bzw. Scene_ForEachSelectedBrushFace
template <typename F>
void jedeFlaeche(const Karte& k, const AuswahlEinstellung& e, F f) {
    if (e.modus != AuswahlModus::Komponente) {
        for (const auto& p : gewaehltePinsel(k)) {
            p->bRep(k.kontext);
            for (const auto& fl : p->flaechen()) { f(*fl); }
        }
    }
    for (const auto& w : k.auswahl.flaechen()) {
        if (const auto fl = w.flaeche.lock()) {
            if (const auto p = w.pinsel.lock()) { p->bRep(k.kontext); }
            f(*fl);
        }
    }
}

// Patch_convertScale (patchdialog.cpp:199-207)
float patchSkalierung(float s) {
    if (s > 0) { return s; }
    if (s < 0) { return -1 / s; }
    return 1;
}

// Patch::ProjectTexture( texdef, direction ) (patch.cpp:1230-1243)
void patchProjizieren(Patch& p, const Texdef& td, const Vektor3f* richtung, const Kontext& k) {
    const auto [w, h] = k.groesse(p.shader());
    Matrix4 m;
    lokalZuTexturFuerProjektion(td, w, h, p.durchschnittsNormale(), richtung, m);
    std::vector<PatchPunkt> pkt = p.punkte();
    for (auto& c : pkt) {
        const Vektor3f st = transformierterPunkt(m, c.vertex);
        c.texcoord = Vektor2f(st[0], st[1]);
    }
    p.punkteSetzen(pkt);
}

// Patch_SetTexdef (patchmanip.cpp:1150-1188)
void patchTexdefSetzen(Karte& k, const float* hShift, const float* vShift, const float* hScale, const float* vScale, const float* drehung) {
    for (const auto& patch : gewaehltePatches(k)) {
        PatchTexdef c = patchTexdef(*patch, k.kontext);
        if (c.gueltig()) {
            bpZuweisen(c.bp, hShift, vShift, hScale, vScale, drehung);
            const auto [w, h] = k.kontext.groesse(patch->shader());
            c.bp.addScale(w, h);
            Matrix4 lokalZuTex;
            bpLokalZuTextur(c.bp, c.ebene, lokalZuTex);
            multipliziere(lokalZuTex, c.texZuLokal);
            std::vector<PatchPunkt> pkt = patch->punkte();
            for (auto& p : pkt) {
                const Vektor3f st = transformierterPunkt(lokalZuTex, Vektor3f(p.texcoord[0], p.texcoord[1], 0));
                p.texcoord = Vektor2f(st[0], st[1]);
            }
            patch->punkteSetzen(pkt);
        } else {   // "fallback": wie NRC fuer alle gewaehlten Patches
            for (const auto& q : gewaehltePatches(k)) {
                if (hShift != nullptr) { q->texturVerschieben(*hShift > 0 ? 8.F : -8.F, 0.F, k.kontext); }
                if (vShift != nullptr) { q->texturVerschieben(0.F, *vShift > 0 ? 8.F : -8.F, k.kontext); }
                if (hScale != nullptr) { q->texturSkalieren(patchSkalierung(*hScale > 0 ? .5F : -.5F), patchSkalierung(0)); }
                if (vScale != nullptr) { q->texturSkalieren(patchSkalierung(0), patchSkalierung(*vScale > 0 ? .5F : -.5F)); }
                if (drehung != nullptr) { q->texturDrehen(*drehung > 0 ? 15.F : -15.F); }
            }
        }
    }
}

}  // namespace

void schritteAmRaster(SiWerte& w, float raster, float hSkalierung, float vSkalierung) {
    w.verschiebung[0] = static_cast<float>(zuGanzzahl(raster / hSkalierung));
    w.verschiebung[1] = static_cast<float>(zuGanzzahl(raster / vSkalierung));
}

void texdefSchieben(Karte& k, const AuswahlEinstellung& e, float x, float y) {
    Befehl befehl(k.undo, "brushShiftTexture -x " + zahlText(x) + " -y " + zahlText(y));
    jedeFlaeche(k, e, [&](Flaeche& f) { f.verschiebeTextur(x, y, k.kontext); });   // Select_ShiftTexture
    if (e.modus != AuswahlModus::Komponente) {
        for (const auto& p : gewaehltePatches(k)) { p->texturVerschieben(x, y, k.kontext); }
    }
}

void texdefSkalieren(Karte& k, const AuswahlEinstellung& e, float x, float y) {
    Befehl befehl(k.undo, "brushScaleTexture -x " + zahlText(x) + " -y " + zahlText(y));
    jedeFlaeche(k, e, [&](Flaeche& f) { f.skaliereTextur(x, y, k.kontext); });
    if (e.modus != AuswahlModus::Komponente) {
        for (const auto& p : gewaehltePatches(k)) { p->texturSkalieren(patchSkalierung(x), patchSkalierung(y)); }
    }
}

void texdefDrehen(Karte& k, const AuswahlEinstellung& e, float winkel) {
    Befehl befehl(k.undo, "brushRotateTexture -angle " + zahlText(winkel));
    jedeFlaeche(k, e, [&](Flaeche& f) { f.dreheTextur(winkel, k.kontext); });
    if (e.modus != AuswahlModus::Komponente) {
        for (const auto& p : gewaehltePatches(k)) { p->texturDrehen(winkel); }
    }
}

void texturEinpassenAuswahl(Karte& k, const AuswahlEinstellung& e, float h, float v, bool nurAbmessung) {
    if (e.modus != AuswahlModus::Komponente) {
        for (const auto& p : gewaehltePinsel(k)) {
            p->bRep(k.kontext);
            for (const auto& f : p->flaechen()) { f->passeTexturEin(h, v, nurAbmessung, k.kontext); }
        }
        for (const auto& p : gewaehltePatches(k)) { p->texturWiederholung(h, v); }   // Scene_PatchTileTexture_Selected
    }
    for (const auto& w : k.auswahl.flaechen()) {
        if (const auto f = w.flaeche.lock()) {
            if (const auto p = w.pinsel.lock()) { p->bRep(k.kontext); }
            f->passeTexturEin(h, v, nurAbmessung, k.kontext);
        }
    }
}

void texturEinpassenBefehl(Karte& k, const AuswahlEinstellung& e, float h, float v, int art) {
    static const char* const namen[] = {"textureAutoFit", "textureAutoFitWidth", "textureAutoFitHeight", "textureAutoFitWidthOnly",
                                        "textureAutoFitHeightOnly"};
    Befehl befehl(k.undo, namen[art]);
    switch (art) {
    case 0: texturEinpassenAuswahl(k, e, h, v, false); break;
    case 1: texturEinpassenAuswahl(k, e, h, 0, false); break;
    case 2: texturEinpassenAuswahl(k, e, 0, v, false); break;
    case 3: texturEinpassenAuswahl(k, e, h, 0, true); break;
    default: texturEinpassenAuswahl(k, e, 0, v, true); break;
    }
}

void texturProjizierenBefehl(Karte& k, const AuswahlEinstellung& e, const Texdef& td, int art, const Vektor3f& richtung, Protokoll& pr) {
    if (k.kontext.art() == TexdefArt::Quake) { warnung(pr, "function doesn't work for *brushes*, having Axial Projection type\n"); }
    Befehl befehl(k.undo, std::string("textureProject") + (art == 0 ? "Axial" : art == 1 ? "Ortho" : "Cam"));
    const Vektor3f* r = art == 0 ? nullptr : &richtung;
    // Select_ProjectTexture( texdef, direction ) (select.cpp:905-913)
    jedeFlaeche(k, e, [&](Flaeche& f) { f.projiziereTextur(td, r, k.kontext); });
    if (e.modus != AuswahlModus::Komponente) {
        for (const auto& p : gewaehltePatches(k)) { patchProjizieren(*p, td, r, k.kontext); }
    }
}

void texdefWerteSetzen(Karte& k, const AuswahlEinstellung& e, const float* hShift, const float* vShift, const float* hScale, const float* vScale,
                       const float* drehung) {
    jedeFlaeche(k, e, [&](Flaeche& f) { f.setzeTexdef(hShift, vShift, hScale, vScale, drehung, k.kontext); });   // Select_SetTexdef
    patchTexdefSetzen(k, hShift, vShift, hScale, vScale, drehung);
}

void texdefWertBefehl(Karte& k, const AuswahlEinstellung& e, int welches, float wert) {
    static const char* const namen[] = {"-hShift ", "-vShift ", "-hScale ", "-vScale ", "-rotation "};
    const float anzeige = welches == 4 ? static_cast<float>(zuGanzzahl(wert * 100.F)) / 100.F : wert;
    Befehl befehl(k.undo, std::string("textureProjectionSetSelected ") + namen[welches] + zahlText(anzeige));
    texdefWerteSetzen(k, e, welches == 0 ? &wert : nullptr, welches == 1 ? &wert : nullptr, welches == 2 ? &wert : nullptr,
                      welches == 3 ? &wert : nullptr, welches == 4 ? &wert : nullptr);
}

void texturUmkehrenBefehl(Karte& k, const AuswahlEinstellung& e, bool waagerecht, const SiWerte& /*schritte*/, float verschiebungFeld,
                          float skalierungFeld) {
    Befehl befehl(k.undo, waagerecht ? "textureInvertHorizontally" : "textureInvertVertically");
    const float s = -verschiebungFeld;
    const float c = -skalierungFeld;
    if (waagerecht) {   // Select_SetTexdef( &shift, 0, &scale, 0, 0 ), Scene_PatchFlipTexture_Selected( 0 )
        jedeFlaeche(k, e, [&](Flaeche& f) { f.setzeTexdef(&s, nullptr, &c, nullptr, nullptr, k.kontext); });
    } else {
        jedeFlaeche(k, e, [&](Flaeche& f) { f.setzeTexdef(nullptr, &s, nullptr, &c, nullptr, k.kontext); });
    }
    for (const auto& p : gewaehltePatches(k)) { p->texturSpiegeln(waagerecht ? 0 : 1); }
}

bool PatchTexdef::gueltig() const {
    return !(!std::isfinite(lokalZuTex[0]) || !std::isfinite(texZuLokal[0]) || std::fabs(punkt(Vektor3f(ebene.normal()), texZuLokal.z().vec3())) < 1e-6 ||
             laengeQuadrat(texZuLokal.x().vec3()) < .01 || laengeQuadrat(texZuLokal.y().vec3()) < .01 || laengeQuadrat(texZuLokal.x().vec3()) > 1e9 ||
             laengeQuadrat(texZuLokal.y().vec3()) > 1e9);
}

PatchTexdef patchTexdef(const Patch& patch, const Kontext& k) {
    PatchTexdef c;
    c.ebene.normal() = Vektor3d(patch.durchschnittsNormale());
    c.ebene.dist() = punkt(c.ebene.normal(), Vektor3d(patch.grenzen().origin));
    const std::size_t b = patch.breite();
    const std::size_t h = patch.hoehe();
    Vektor3f wRicht;
    Vektor3f hRicht;
    patch.durchschnittsAchsen(wRicht, hRicht);
    normieren(wRicht);
    normieren(hRicht);
    // die laengste Zeile und Spalte
    float wLaenge = 0;
    float hLaenge = 0;
    std::size_t zeile = 0;
    std::size_t spalte = 0;
    for (std::size_t r = 0; r < h; ++r) {
        float l = 0;
        for (std::size_t cc = 0; cc < b - 1; ++cc) { l += static_cast<float>(laenge(patch.punkt(r, cc + 1).vertex - patch.punkt(r, cc).vertex)); }
        if (l - wLaenge > .1F || ((r == 0 || r == h - 1) && gleichEps(l, wLaenge, .1F))) {   // erste und letzte Zeile bevorzugt
            wLaenge = l;
            zeile = r;
        }
    }
    for (std::size_t cc = 0; cc < b; ++cc) {
        float l = 0;
        for (std::size_t r = 0; r < h - 1; ++r) { l += static_cast<float>(laenge(patch.punkt(r + 1, cc).vertex - patch.punkt(r, cc).vertex)); }
        if (l - hLaenge > .1F || ((cc == 0 || cc == b - 1) && gleichEps(l, hLaenge, .1F))) {
            hLaenge = l;
            spalte = cc;
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
        for (std::size_t cc = 0; cc < spalte; ++cc) { distW0 += static_cast<float>(laenge(patch.punkt(zeile, cc + 1).vertex - patch.punkt(zeile, cc).vertex)); }
        for (std::size_t cc = spalte; cc < b - 1; ++cc) { distW1 += static_cast<float>(laenge(patch.punkt(zeile, cc + 1).vertex - patch.punkt(zeile, cc).vertex)); }
        float distH0 = 0;
        float distH1 = 0;
        for (std::size_t r = 0; r < zeile; ++r) { distH0 += static_cast<float>(laenge(patch.punkt(r + 1, spalte).vertex - patch.punkt(r, spalte).vertex)); }
        for (std::size_t r = zeile; r < h - 1; ++r) { distH1 += static_cast<float>(laenge(patch.punkt(r + 1, spalte).vertex - patch.punkt(r, spalte).vertex)); }
        if ((distW0 > distH0 && distW0 > distH1) || (distW1 > distH0 && distW1 > distH1)) {
            p0 = &patch.punkt(0, spalte);
            p1 = &patch.punkt(h - 1, spalte);
            p2 = distW0 > distW1 ? &patch.punkt(zeile, 0) : &patch.punkt(zeile, b - 1);
            v0 = p0->vertex;
            v1 = v0 + hRicht * hLaenge;
            v2 = v0 + hRicht * distH0 + (distW0 > distW1 ? (wRicht * -distW0) : (wRicht * distW1));
        } else {
            p0 = &patch.punkt(zeile, 0);
            p1 = &patch.punkt(zeile, b - 1);
            p2 = distH0 > distH1 ? &patch.punkt(0, spalte) : &patch.punkt(h - 1, spalte);
            v0 = p0->vertex;
            v1 = v0 + wRicht * wLaenge;
            v2 = v0 + wRicht * distW0 + (distH0 > distH1 ? (hRicht * -distH0) : (hRicht * distH1));
        }
        if (punkt(ebeneAusPunkten(v0, v1, v2).normal(), c.ebene.normal()) < 0) {
            std::swap(p0, p1);
            std::swap(v0, v1);
        }
    }
    const EbenenPunkte ecken{Vektor3d(v0), Vektor3d(v1), Vektor3d(v2)};
    const Vektor3d sts[3]{Vektor3d(p0->texcoord[0], p0->texcoord[1], 0), Vektor3d(p1->texcoord[0], p1->texcoord[1], 0),
                          Vektor3d(p2->texcoord[0], p2->texcoord[1], 0)};
    lokalZuTexturAusSt(ecken, sts, c.lokalZuTex);
    c.texZuLokal = affinInvers(c.lokalZuTex);
    bpAusSt(c.bp, ecken, sts, ebeneAusPunkten(ecken).normal());
    const auto [tw, th] = k.groesse(patch.shader());
    c.bp.removeScale(tw, th);
    return c;
}

SiAnzeige siAusAuswahl(const Karte& k) {
    SiAnzeige a;
    if (!k.auswahl.flaechen().empty()) {   // g_SelectedFaceInstances: die zuletzt gewaehlte Flaeche
        const auto f = k.auswahl.flaechen().back().flaeche.lock();
        if (f) {
            a.projektion = f->texdefHolen(k.kontext);
            a.shader = f->shader();
            a.gefunden = true;
        }
        return a;
    }
    // Scene_BrushGetShaderTexdef_Selected: der zuletzt gewaehlte Pinsel, sonst der erste
    std::shared_ptr<Pinsel> pinsel;
    if (k.auswahl.anzahl() != 0) { pinsel = std::dynamic_pointer_cast<Pinsel>(k.auswahl.elemente().back().primitiv.lock()); }
    if (!pinsel) {
        const auto liste = gewaehltePinsel(k);
        if (!liste.empty()) { pinsel = liste.front(); }
    }
    if (pinsel && !pinsel->flaechen().empty()) {
        const Flaeche& f = *pinsel->flaechen().front();
        a.shader = f.shader();
        a.projektion = f.texdefHolen(k.kontext);
        a.gefunden = true;
        return a;
    }
    // Scene_PatchGetShaderTexdef_Selected: der zuletzt gewaehlte sichtbare Patch, sonst der erste
    std::shared_ptr<Patch> patch;
    if (k.auswahl.anzahl() != 0) {
        patch = std::dynamic_pointer_cast<Patch>(k.auswahl.elemente().back().primitiv.lock());
        if (patch && !patch->sichtbar()) { patch = nullptr; }
    }
    if (!patch) {
        for (const GewaehltesPrimitiv& g : gewaehltePrimitiveListe(k)) {
            if (g.primitiv->art() == Primitiv::Art::Patch) {
                patch = std::static_pointer_cast<Patch>(g.primitiv);
                break;
            }
        }
    }
    if (patch) {
        a.shader = patch->shader();
        a.patchModus = true;
        a.gefunden = true;
        const PatchTexdef c = patchTexdef(*patch, k.kontext);
        if (c.gueltig()) { a.projektion.bp = c.bp; }
    }
    return a;
}

}  // namespace bhed::karte
