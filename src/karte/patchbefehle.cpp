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
        if (g.primitiv->art() != Primitiv::Art::Patch || g.primitiv->versteckt || g.entitaet->versteckt) { continue; }
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

void patchKappen(Karte& k, const std::string& shader, PatchKappe art, Protokoll& pr) {
    Befehl befehl(k.undo, "patchPutCaps");
    for (const PatchOrt& o : gewaehltePatches(k)) {   // Patch_makeCaps (patchmanip.cpp:85-127)
        const Patch& patch = *o.patch;
        if ((art == PatchKappe::EndCap || art == PatchKappe::IEndCap) && patch.breite() != 5) {
            fehler(pr, "cannot create end-cap - patch width != 5\n");
            continue;
        }
        if ((art == PatchKappe::Bevel || art == PatchKappe::IBevel) && patch.breite() != 3 && patch.breite() != 5) {
            fehler(pr, "cannot create bevel-cap - patch width != 3\n");
            continue;
        }
        if (art == PatchKappe::Zylinder && patch.breite() != 9) {
            fehler(pr, "cannot create cylinder-cap - patch width != 9\n");
            continue;
        }
        for (const bool erste : {true, false}) {
            auto kappe = neuerPatch(k, o.entitaet);
            patch.kappeMachen(*kappe, art, false, erste, k.kontext);   // ROW
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
