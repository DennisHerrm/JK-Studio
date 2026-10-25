// patchbefehle.cpp - Karten-Editor: die Patch-Befehle der Szene.
// Portiert aus NetRadiant-Custom 20260114 (Quellen in patchbefehle.h).
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
#include "bhed/karte/patchbefehle.h"

#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/komponenten.h"
#include "bhed/karte/texdef.h"
#include "bhed/karte/texturkopie.h"

#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <vector>

namespace bhed::karte {

namespace {

struct PatchOrt {
    std::shared_ptr<Entitaet> entitaet;
    std::shared_ptr<Patch> patch;
};

// Scene_forEachVisibleSelectedPatchInstance: die sichtbaren gewaehlten Patches
std::vector<PatchOrt> gewaehltePatches(const Karte& k) {
    std::vector<PatchOrt> r;
    for (const GewaehltesPrimitiv& g : gewaehltePrimitiveListe(k)) {
        if (g.primitiv->art() != Primitiv::Art::Patch || !g.primitiv->sichtbar() || !g.entitaet->sichtbar()) { continue; }
        r.push_back({g.entitaet, std::static_pointer_cast<Patch>(g.primitiv)});
    }
    return r;
}

std::shared_ptr<Patch> neuerPatch(Karte& k, const std::shared_ptr<Entitaet>& ent) {
    auto p = std::make_shared<Patch>();
    p->schicht = static_cast<int>(k.aktuelleSchicht);
    ent->kindEinfuegen(p);
    return p;
}

// aabb_small (patchmanip.cpp:173-177)
bool kastenKlein(const Kasten& a) {
    return (a.extents[0] < 0.01 && a.extents[1] < 0.01) || (a.extents[1] < 0.01 && a.extents[2] < 0.01) ||
           (a.extents[0] < 0.01 && a.extents[2] < 0.01);
}

}  // namespace

Kasten patchGrenzen(const Kasten& arbeitsbereich, float raster) {
    Kasten a = arbeitsbereich;
    if (a.extents[0] == 0) { a.extents[0] = raster; }
    if (a.extents[1] == 0) { a.extents[1] = raster; }
    if (a.extents[2] == 0) { a.extents[2] = raster; }
    if (kastenGueltig(a)) { return a; }
    return Kasten(Vektor3f(0, 0, 0), Vektor3f(64, 64, 64));
}

void patchVorlage(Karte& k, const Kasten& grenzen, const std::string& shader, PatchVorlage art, int achse, std::size_t breite, std::size_t hoehe,
                  bool neuVerteilen) {
    std::shared_ptr<Entitaet> ziel;
    if (k.auswahl.anzahl() != 0) {   // den Behaelter aus der Auswahl
        const AuswahlElement& el = k.auswahl.elemente().back();
        const auto prim = el.primitiv.lock();
        const auto ent = el.entitaet.lock();
        if (prim && ent) {
            k.auswahl.abwaehlen(ent);   // "avoid container deletion"
            ziel = ent;
        }
    }
    if (!ziel) { ziel = k.weltHolenOderAnlegen(); }
    auto patch = neuerPatch(k, ziel);
    patch->setzeShader(shader);
    patch->vorlageBauen(grenzen, art, achse, breite, hoehe, k.kontext);
    if (neuVerteilen) {
        patch->neuVerteilen(true);    // Redisperse( COL )
        patch->neuVerteilen(false);   // Redisperse( ROW )
    }
    patch->punkteGeaendert();
    auswahlEntfernen(k);   // "delete last to avoid empty container deletion"
    k.auswahl.leeren();
    k.auswahl.waehlen(ziel, patch);
}

void patchVorlageBefehl(Karte& k, const Kasten& grenzen, const std::string& shader, PatchVorlage art, int achse, std::size_t breite, std::size_t hoehe,
                        bool neuVerteilen) {
    const char* name = "patchCreatePlane";
    switch (art) {
    case PatchVorlage::Ebene: name = "patchCreatePlane"; break;
    case PatchVorlage::Bevel: name = "patchCreateBevel"; break;
    case PatchVorlage::EndCap: name = "patchCreateEndCap"; break;
    case PatchVorlage::Zylinder: name = "patchCreateCylinder"; break;
    case PatchVorlage::DichterZylinder: name = "patchCreateDenseCylinder"; break;
    case PatchVorlage::SehrDichterZylinder: name = "patchCreateVeryDenseCylinder"; break;
    case PatchVorlage::EckZylinder: name = "patchCreateSquareCylinder"; break;
    case PatchVorlage::Kegel: name = "patchCreateCone"; break;
    case PatchVorlage::Kugel: name = "patchCreateSphere"; break;
    case PatchVorlage::ExakterZylinder: name = "patchCreateXactCylinder"; break;
    case PatchVorlage::ExakteKugel: name = "patchCreateXactSphere"; break;
    case PatchVorlage::ExakterKegel: name = "patchCreateXactCone"; break;
    }
    Befehl befehl(k.undo, name);
    patchVorlage(k, grenzen, shader, art, achse, breite, hoehe, neuVerteilen);
}

PatchDichte patchDichte(PatchVorlage art) {
    switch (art) {
    case PatchVorlage::ExakterZylinder:
    case PatchVorlage::ExakterKegel: return {3, 7, 3, 13, 0, 0};
    case PatchVorlage::ExakteKugel: return {5, 7, 7, 13, 0, 0};
    default: return {3, 3, 3, 3, 0, 0};   // Plane
    }
}

namespace {

// NRC#302 (Stufe 6, nach dem Vorschlag aus dem Issue, fix-degenerate-bevel-caps-302):
// Patch_makeCaps kappte immer entlang ROW, also mit der Kurve in der Breite.
// Ist die Kurve in der Hoehe (gebaut oder transponiert), entstanden zwei
// entartete Patches mit allen Punkten auf einer Linie. Jetzt gilt die
// Groessenbedingung fuer Breite und Hoehe; passen beide, nimmt die Kappe die
// Richtung, deren Randschnitt am weitesten von einer Geraden abweicht.
bool kappenGroesseGilt(PatchKappe art, std::size_t n) {   // patch_capSizeValid
    switch (art) {
    case PatchKappe::Bevel:
    case PatchKappe::IBevel: return n == 3 || n == 5;
    case PatchKappe::EndCap:
    case PatchKappe::IEndCap: return n == 5;
    case PatchKappe::Zylinder: return n == 9;
    }
    return false;
}

const char* kappenBedingung(PatchKappe art) {   // patch_capRequirement
    switch (art) {
    case PatchKappe::Bevel:
    case PatchKappe::IBevel: return "bevel-cap - neither patch width, nor height is 3 or 5";
    case PatchKappe::EndCap:
    case PatchKappe::IEndCap: return "end-cap - neither patch width, nor height is 5";
    case PatchKappe::Zylinder: return "cylinder-cap - neither patch width, nor height is 9";
    }
    return "cap - invalid cap type";
}

// points_deviation ueber den Randschnitt (patch_capPoints): spalten = COL
double kappenAbweichung(const Patch& p, bool spalten, bool erste) {
    const std::size_t n = spalten ? p.hoehe() : p.breite();
    const std::size_t index = erste ? 0 : (spalten ? p.breite() : p.hoehe()) - 1;
    std::vector<Vektor3f> pk(n);
    for (std::size_t i = 0; i < n; ++i) { pk[i] = spalten ? p.punkt(i, index).vertex : p.punkt(index, i).vertex; }
    const Vektor3f basis = pk[0];
    std::size_t fern = 0;   // ein geschlossener Schnitt (Zylinder): die Gerade zum entferntesten Punkt
    const auto quadrat = [](const Vektor3f& v) { return static_cast<double>(v[0]) * v[0] + static_cast<double>(v[1]) * v[1] + static_cast<double>(v[2]) * v[2]; };
    for (std::size_t i = 1; i < n; ++i) {
        if (quadrat(pk[i] - basis) > quadrat(pk[fern] - basis)) { fern = i; }
    }
    const double l = std::sqrt(quadrat(pk[fern] - basis));
    if (l == 0) { return 0; }
    const Vektor3f r = pk[fern] - basis;
    double abw = 0;
    for (const Vektor3f& q : pk) {
        const Vektor3f d = q - basis;
        const double t = (static_cast<double>(d[0]) * r[0] + static_cast<double>(d[1]) * r[1] + static_cast<double>(d[2]) * r[2]) / (l * l);
        const double x = d[0] - r[0] * t;
        const double y = d[1] - r[1] * t;
        const double z = d[2] - r[2] * t;
        abw = std::max(abw, std::sqrt(x * x + y * y + z * z));
    }
    return abw;
}

}  // namespace

void patchKappen(Karte& k, const std::string& shader, PatchKappe art, Protokoll& pr) {
    Befehl befehl(k.undo, "patchPutCaps");
    for (const PatchOrt& o : gewaehltePatches(k)) {   // Patch_makeCaps (patchmanip.cpp:85-127)
        const Patch& patch = *o.patch;
        const bool zeilen = kappenGroesseGilt(art, patch.breite());
        const bool spaltenOk = kappenGroesseGilt(art, patch.hoehe());
        if (!zeilen && !spaltenOk) {
            fehler(pr, std::string("cannot create ") + kappenBedingung(art) + "\n");
            continue;
        }
        bool spalten = !zeilen;
        if (zeilen && spaltenOk) {
            const double c = std::max(kappenAbweichung(patch, true, true), kappenAbweichung(patch, true, false));
            const double r = std::max(kappenAbweichung(patch, false, true), kappenAbweichung(patch, false, false));
            spalten = c > r;
        }
        for (const bool erste : {true, false}) {
            auto kappe = neuerPatch(k, o.entitaet);
            patch.kappeMachen(*kappe, art, spalten, erste, k.kontext);   // ROW bzw. COL
            kappe->setzeShader(shader);
            k.auswahl.waehlen(o.entitaet, kappe);   // selectPath( path, true )
        }
    }
}

void patchVerformen(Karte& k, int staerke, int achse) {
    Befehl befehl(k.undo, "patchDeform");
    for (const PatchOrt& o : gewaehltePatches(k)) {   // Patch_deform (patchmanip.cpp:150-159)
        std::vector<PatchPunkt> pkt = o.patch->punkte();
        for (PatchPunkt& c : pkt) {
            const int zufall = static_cast<int>(static_cast<float>(staerke) * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)));
            c.vertex[static_cast<std::size_t>(achse)] += static_cast<float>(zufall);
        }
        o.patch->punkteSetzen(pkt);   // undoSave, controlPointsChanged
    }
}

void patchVerdicken(Karte& k, int dicke, bool waende, int achse) {
    Befehl befehl(k.undo, "patchThicken");
    for (const PatchOrt& o : gewaehltePatches(k)) {   // Patch_thicken (patchmanip.cpp:171-231)
        auto ziel = neuerPatch(k, o.entitaet);
        bool ohne12 = true;
        bool ohne34 = true;
        ziel->verdicktGegenueber(*o.patch, static_cast<float>(dicke), achse, ohne12, ohne34);
        k.auswahl.waehlen(o.entitaet, ziel);
        if (waende && dicke != 0) {
            const int ende = ohne34 ? 2 : 4;
            for (int i = ohne12 ? 2 : 0; i < ende; ++i) {
                auto wand = neuerPatch(k, o.entitaet);
                wand->verdicktWand(*o.patch, *ziel, i, k.kontext);
                if (kastenKlein(wand->grenzen())) {   // "Discarding degenerate patch"
                    o.entitaet->kindEntfernen(wand.get());
                } else {
                    k.auswahl.waehlen(o.entitaet, wand);
                }
            }
        }
        ziel->matrixUmkehren();   // die Gegenseite zeigt nach aussen
        if (kastenKlein(ziel->grenzen())) {
            k.auswahl.abwaehlen(o.entitaet, ziel);
            o.entitaet->kindEntfernen(ziel.get());
        }
    }
}

void patchBefehl(Karte& k, PatchBefehl b) {
    static const char* const namen[] = {"patchInsertFirstColumns", "patchInsertLastColumns", "patchInsertFirstRows", "patchInsertLastRows",
                                        "patchDeleteFirstColumns", "patchDeleteLastColumns", "patchDeleteFirstRows", "patchDeleteLastRows",
                                        "patchInvert",             "patchRedisperseRows",    "patchRedisperseColumns", "patchSmoothRows",
                                        "patchSmoothColumns",      "patchTranspose",         "patchFlipTextureU",    "patchFlipTextureV",
                                        "patchNaturalTexture",     "patchCapTexture"};
    Befehl befehl(k.undo, namen[static_cast<int>(b)]);
    for (const PatchOrt& o : gewaehltePatches(k)) {
        Patch& p = *o.patch;
        switch (b) {
        case PatchBefehl::SpaltenVornEinfuegen: p.einfuegenEntfernen(true, true, true); break;
        case PatchBefehl::SpaltenHintenEinfuegen: p.einfuegenEntfernen(true, true, false); break;
        case PatchBefehl::ZeilenVornEinfuegen: p.einfuegenEntfernen(true, false, true); break;
        case PatchBefehl::ZeilenHintenEinfuegen: p.einfuegenEntfernen(true, false, false); break;
        case PatchBefehl::SpaltenVornLoeschen: p.einfuegenEntfernen(false, true, true); break;
        case PatchBefehl::SpaltenHintenLoeschen: p.einfuegenEntfernen(false, true, false); break;
        case PatchBefehl::ZeilenVornLoeschen: p.einfuegenEntfernen(false, false, true); break;
        case PatchBefehl::ZeilenHintenLoeschen: p.einfuegenEntfernen(false, false, false); break;
        case PatchBefehl::Umkehren: p.matrixUmkehren(); break;
        case PatchBefehl::ZeilenVerteilen: p.neuVerteilen(false); break;
        case PatchBefehl::SpaltenVerteilen: p.neuVerteilen(true); break;
        case PatchBefehl::ZeilenGlaetten: p.glaetten(false); break;
        case PatchBefehl::SpaltenGlaetten: p.glaetten(true); break;
        case PatchBefehl::Transponieren: p.matrixTransponieren(); break;
        case PatchBefehl::TexturX: p.texturSpiegeln(0); break;
        case PatchBefehl::TexturY: p.texturSpiegeln(1); break;
        case PatchBefehl::TexturNatuerlich: p.texturNatuerlich(k.kontext); break;
        case PatchBefehl::TexturKappen: p.texturKappen(k.kontext); break;
        }
    }
}

void texturZuruecksetzenKappen(Karte& k, const AuswahlEinstellung& e) {
    Befehl befehl(k.undo, "textureReset/Cap");
    TexturProjektion projektion;
    texdefStandard(k.kontext.art(), projektion);
    texdefSetzenAuswahl(k, e, projektion, false, true);   // Select_SetTexdef( projection, false, true )
    for (const PatchOrt& o : gewaehltePatches(k)) { o.patch->texturKappen(k.kontext); }   // Scene_PatchCapTexture_Selected
}

}  // namespace bhed::karte
