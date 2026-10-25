// app_karte_xy.cpp - Karten-Editor: die 2D-Ansichten XY / XZ / YZ.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/xywindow.cpp (XYWnd:
// XY_Draw, overlayDraw, XY_MouseDown/Moved/Up, Move_Begin/End, Zoom_Begin,
// wheelEvent, XY_Focus, XY_Centralize, XY_SetViewType, XY_Zoom100, die
// Show-Schalter), radiant/map.cpp (FocusViews, Map_StartPosition),
// plugins/entity (Farben, Kaesten, Pfeile, Namen, Lichter, Verbindungen),
// libs/gtkutil/cursor.h (FreezePointer). Die Rechnung steht in
// bhed/karte/xyansicht.h; hier das Bild (Direct3D 11, karte_gpu.h), die
// Texte (ImGui) und die Maus.
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
#include "karteneditor.h"

#include "app_internal.h"
#include "bhed/karte/kameraszene.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/pinsel.h"
#include "karte_gpu.h"

#include <imgui_internal.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <functional>

namespace bhed::gui::karteneditor {

namespace {

using karte::Farbe;
using karte::Kasten;
using karte::Linie;
using karte::Sicht;
using karte::Vektor3f;

// RAD_* (xywindow.cpp:103-110)
constexpr unsigned kShift = 0x01;
constexpr unsigned kAlt = 0x02;
constexpr unsigned kControl = 0x04;
constexpr unsigned kLinks = 0x10;
constexpr unsigned kMitte = 0x20;
constexpr unsigned kRechts = 0x40;

Editor& e() { return editor(); }

// Was eine Ansicht vom Szenenbild behaelt: die Linien je Blickrichtung
// (sichtbare Kanten haengen nur von der Richtung ab), auf der Grafikkarte und
// fuer den ImGui-Weg.
struct Name {
    Vektor3f anker;
    Kasten box;
    std::string text;
    Farbe farbe = 0;
    bool gewaehlt = false;
};
struct Szene {
    // Punkt 12: die Kanten aller Pinsel und Patches in ihrer Farbe haengen nicht
    // an der Auswahl - nur bei einer Aenderung der Karte neu (geo); das
    // Ausgewaehlte liegt in wahl darueber.
    std::size_t geoStand = static_cast<std::size_t>(-1);
    Sicht geoSicht = Sicht::XY;
    std::vector<Linie> geo;
    kartegpu::Puffer gpuGeo;
    bool geoGpuAktuell = false;
    std::size_t stand = static_cast<std::size_t>(-1);
    Sicht sicht = Sicht::XY;
    std::vector<Linie> welt;    // Breite 1
    std::vector<Linie> wahl;    // Auswahl: Breite 2 ($XY_OVERLAY)
    std::vector<Name> namen;
    kartegpu::Puffer gpuWelt;
    kartegpu::Puffer gpuWahl;
    bool gpuAktuell = false;
};
std::array<Szene, 3> g_szene;

Farbe farbe3(const Farben::F3& c, float a = 1.0F) { return karte::farbeAus(c[0], c[1], c[2], a); }

}  // namespace

std::string xySzeneMass() {
    std::string t;
    for (std::size_t i = 0; i < g_szene.size(); ++i) {
        const Szene& s = g_szene[i];
        char z[160];
        std::snprintf(z, sizeof(z), "%s2D %zu: Geometrie %zu Linien (%zu MB), Rest %zu, Auswahl %zu, %zu Namen", i == 0 ? "" : "; ", i,
                      s.geo.size(), s.geo.capacity() * sizeof(Linie) / (1024 * 1024), s.welt.size(), s.wahl.size(), s.namen.size());
        t += z;
    }
    return t;
}

// Der Stand der Geometrie (ohne Auswahl und Anzeige): Befehle, Rueckgaengig,
// Wiederholen, neue Karte, Verstecken/Zeigen, die Farbe der Pinsel.
std::size_t geometrieStand() {
    const Editor& ed = e();
    std::size_t h = ed.karte.undo.aenderungen() * 1315423911U;
    h ^= ed.karte.undo.undoAnzahl() * 2654435761U + (h << 6) + (h >> 2);
    h ^= ed.karte.undo.redoAnzahl() * 40503U + (h << 6) + (h >> 2);
    h ^= ed.ladeZaehler * 97U + (h << 6) + (h >> 2);
    h ^= ed.sichtStand * 2246822519U + (h << 6) + (h >> 2);
    h ^= static_cast<std::size_t>(farbe3(ed.farben.pinsel)) * 7U + (h << 6) + (h >> 2);
    return h;
}

// Ein Stand der Szene: aendert sich mit jedem Befehl, Rueckgaengig,
// Wiederholen, neuer Karte und neuer Auswahl.
std::size_t szeneStand() {
    const Editor& ed = e();
    std::size_t h = ed.karte.undo.aenderungen() * 1315423911U;
    h ^= ed.karte.undo.undoAnzahl() * 2654435761U + (h << 6) + (h >> 2);
    h ^= ed.karte.undo.redoAnzahl() * 40503U + (h << 6) + (h >> 2);
    h ^= ed.ladeZaehler * 97U + (h << 6) + (h >> 2);
    h ^= ed.karte.auswahl.anzahl() * 31U + (h << 6) + (h >> 2);
    for (const auto& w : ed.karte.auswahl.elemente()) {
        h ^= reinterpret_cast<std::uintptr_t>(w.entitaet.lock().get()) + (h << 6) + (h >> 2);
        h ^= reinterpret_cast<std::uintptr_t>(w.primitiv.lock().get()) + (h << 6) + (h >> 2);
    }
    const Anzeige& a = ed.anzeige;
    h ^= (a.winkel ? 1U : 0U) | (a.verbindungen ? 2U : 0U) | (a.namen ? 4U : 0U) | (ed.ent.zielnamen ? 16U : 0U);
    h ^= ed.karte.undo.aenderungen() * 2654435789U + (h << 6) + (h >> 2);   // jede Aenderung (auch bei vollem Undo-Stapel)
    h ^= static_cast<std::size_t>(farbe3(ed.farben.pinsel)) * 7U + static_cast<std::size_t>(farbe3(ed.farben.auswahl)) * 13U;
    h ^= static_cast<std::size_t>(farbe3(ed.farben.rasterHinten));
    h ^= (a.lichtRadien ? 8U : 0U) + (h << 6) + (h >> 2);
    h ^= ed.sichtStand * 2246822519U + (h << 6) + (h >> 2);   // Verstecken/Zeigen
    h ^= ed.karte.auswahl.flaechen().size() * 3266489917U + (h << 6) + (h >> 2);
    return h;
}

namespace {

// Die Grenzen der Kinder einer Entitaet (Gruppen: Name in der Mitte).
Kasten kinderGrenzen(const karte::Entitaet& ent, const karte::Kontext& k) {
    Kasten b;
    for (const auto& p : ent.kinder()) {
        if (p->art() == karte::Primitiv::Art::Pinsel) {
            const auto& pinsel = static_cast<const karte::Pinsel&>(*p);
            karte::kastenErweiternSicher(b, pinsel.grenzen(k));
        } else {
            karte::kastenErweiternSicher(b, static_cast<const karte::Patch&>(*p).grenzen());
        }
    }
    return b;
}

Vektor3f ursprungVon(const karte::Entitaet& ent) {
    Vektor3f o(0, 0, 0);
    (void)std::sscanf(ent.wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]);
    return o;
}

// Die Farbe einer Entitaet im Drahtgitter (SetWorldspawnColour, Klassenfarbe).
Farbe entitaetsFarbe(const karte::Entitaet& ent) {
    const Editor& ed = e();
    const karte::EntitaetsKlasse* kl = ent.klasse();
    if (ent.klassenname() == "worldspawn") { return farbe3(ed.farben.pinsel); }
    return kl != nullptr ? karte::farbeAus(kl->farbe[0], kl->farbe[1], kl->farbe[2]) : karte::farbeAus(0, 0.5F, 0);
}

// Die Kanten aller sichtbaren Pinsel und Patches in ihrer Farbe (der Teil der
// Szene, der nicht an der Auswahl haengt).
void geoBauen(Szene& s, Sicht sicht) {
    Editor& ed = e();
    s.geo.clear();
    s.geoSicht = sicht;
    const karte::Kontext& k = ed.karte.kontext;
    for (const auto& ent : ed.karte.entitaeten()) {
        if (!ent->sichtbar()) { continue; }
        const Farbe farbe = entitaetsFarbe(*ent);
        for (const auto& p : ent->kinder()) {
            if (!p->sichtbar()) { continue; }
            if (p->art() == karte::Primitiv::Art::Pinsel) {
                karte::pinselKanten(static_cast<const karte::Pinsel&>(*p), k, sicht, farbe, s.geo);
            } else {
                karte::patchGitter(static_cast<const karte::Patch&>(*p), farbe, s.geo);
            }
        }
    }
    s.geoGpuAktuell = false;
}

// Scene_Render mit dem XYRenderer (xywindow.cpp:1647-1704): Drahtgitter in
// der Klassenfarbe, Ausgewaehltes (auch Kinder ausgewaehlter Entities) in der
// Auswahlfarbe und Breite 2 - hier der Rest ohne die Geometrie (geoBauen):
// ausgewaehlte Pinsel und Patches, Punktentitaeten, Gruppen, Namen, Verbindungen.
void szeneBauen(Szene& s, Sicht sicht) {
    Editor& ed = e();
    s.welt.clear();
    s.wahl.clear();
    s.namen.clear();
    s.sicht = sicht;
    const karte::Kontext& k = ed.karte.kontext;
    const Farbe auswahl = farbe3(ed.farben.auswahl);
    // XY_Draw: setLightColorize( vector3_min_component( color_gridback ) < .9f )
    const auto& hg = ed.farben.rasterHinten;
    const bool lichtFarbe = std::min({hg[0], hg[1], hg[2]}) < .9F;
    // Ziele der Verbindungen: targetname -> Mitte
    std::map<std::string, Vektor3f> ziele;
    const auto mitte = [&](const karte::Entitaet& ent) {
        const karte::EntitaetsKlasse* kl = ent.klasse();
        if (kl != nullptr && kl->fixedsize) { return ursprungVon(ent) + (kl->mins + kl->maxs) * 0.5F; }
        return kinderGrenzen(ent, k).origin;
    };
    for (const auto& ent : ed.karte.entitaeten()) {
        const std::string tn = ent->wert("targetname");
        if (!tn.empty()) { ziele.emplace(tn, mitte(*ent)); }
    }
    for (const auto& ent : ed.karte.entitaeten()) {
        if (!ent->sichtbar()) { continue; }   // Node::visible: nicht sichtbar (auch die Kinder)
        const karte::EntitaetsKlasse* kl = ent->klasse();
        const bool welt = ent->klassenname() == "worldspawn";
        // SetWorldspawnColour: worldspawn in color_brushes
        Farbe farbe = welt ? farbe3(ed.farben.pinsel)
                           : (kl != nullptr ? karte::farbeAus(kl->farbe[0], kl->farbe[1], kl->farbe[2]) : karte::farbeAus(0, 0.5F, 0));
        const bool entGewaehlt = ed.karte.auswahl.gewaehlt(ent.get());
        for (const auto& p : ent->kinder()) {
            if (!p->sichtbar()) { continue; }
            const bool gew = entGewaehlt || ed.karte.auswahl.gewaehlt(ent.get(), p.get());
            if (!gew) { continue; }   // in geo (geoBauen)
            if (p->art() == karte::Primitiv::Art::Pinsel) {
                karte::pinselKanten(static_cast<const karte::Pinsel&>(*p), k, sicht, auswahl, s.wahl);
            } else {
                karte::patchGitter(static_cast<const karte::Patch&>(*p), auswahl, s.wahl);
            }
        }
        std::vector<Linie>& ziel = entGewaehlt ? s.wahl : s.welt;
        const Farbe f = entGewaehlt ? auswahl : farbe;
        const bool punkt = kl != nullptr && kl->fixedsize;
        const bool licht = ent->klassenname() == "light" || ent->klassenname() == "lightJunior";
        Kasten box;
        if (!ent->istBehaelter() && k.modelle != nullptr && kl != nullptr && kl->miscmodelIs) {
            // MiscModel: das Modell als Drahtgitter (die Dreiecke), der Name am
            // Ursprung mit aabb_fits_view( AABB( 0, 32 ) ) (miscmodel.cpp:293-303)
            const karte::ModellAnsicht a = k.modelle->ansicht(*ent);
            const karte::Matrix4 m = karte::miscModellMatrix(*ent);
            if (a.modell) {
                for (std::size_t fi = 0; fi < a.modell->flaechen.size(); ++fi) {
                    if (fi < a.shader.size() && a.shader[fi].empty()) { continue; }
                    const karte::ModellFlaeche& fl = a.modell->flaechen[fi];
                    for (std::size_t i = 0; i + 2 < fl.indizes.size(); i += 3) {
                        const Vektor3f p0 = karte::transformierterPunkt(m, fl.punkte[fl.indizes[i]].ort);
                        const Vektor3f p1 = karte::transformierterPunkt(m, fl.punkte[fl.indizes[i + 1]].ort);
                        const Vektor3f p2 = karte::transformierterPunkt(m, fl.punkte[fl.indizes[i + 2]].ort);
                        ziel.push_back(Linie{p0, p1, f});
                        ziel.push_back(Linie{p1, p2, f});
                        ziel.push_back(Linie{p2, p0, f});
                    }
                }
            }
            const Vektor3f o = ursprungVon(*ent);
            const std::string tn = ed.ent.zielnamen ? ent->wert("targetname") : std::string();
            s.namen.push_back({o, Kasten(o, Vektor3f(32, 32, 32)), tn.empty() ? ent->klassenname() : tn, farbe, entGewaehlt});
        } else if (punkt) {
            const Vektor3f o = ursprungVon(*ent);
            box = karte::kastenAusMinMax(o + kl->mins, o + kl->maxs);
            if (licht) {
                // renderSolid: g_lightColorize ? Lichtfarbe : Klassenfarbe
                Farbe lf = farbe;
                if (lichtFarbe) {
                    karte::Vektor3f c(1, 1, 1);
                    if (std::sscanf(ent->wert("_color").c_str(), "%f %f %f", &c[0], &c[1], &c[2]) == 3) {
                        const float m = std::max({c[0], c[1], c[2]});
                        c = m == 0 ? Vektor3f(1, 1, 1) : c * (1.0F / m);
                    } else {
                        c = Vektor3f(1, 1, 1);
                    }
                    lf = karte::farbeAus(c[0], c[1], c[2]);
                }
                karte::lichtOktaeder(box, entGewaehlt ? auswahl : lf, ziel);
                // Light::renderSolid, ausgewaehlt und g_lightRadii ohne "target":
                // zwei Radien als Drahtkugeln (renderWireframe, Highlight
                // ePrimitive aus - in der Farbe des Lichts).
                if (entGewaehlt && ed.anzeige.lichtRadien && !ent->hat("target")) {
                    const karte::LichtRadien r = karte::lichtRadien(ent->wert("_light"), ent->wert("light"), ent->wert("scale"),
                                                                    ent->wert("fade"), ent->wert("spawnflags"));
                    for (const float radius : r.gezeichnet()) { karte::kugelDraht(box.origin, radius, lf, s.welt); }
                }
            } else {
                karte::kastenDraht(box, f, ziel);
                if (ed.anzeige.winkel) {
                    const Vektor3f w = karte::entitaetsWinkel(ent->wert("angle"), ent->wert("angles"), ent->hat("angles"), false);
                    karte::pfeil(box.origin, w, f, ziel);
                }
            }
            if (!welt) {
                // NamedEntity::name(): der Wert von KeyIsName::m_nameKey - "classname",
                // mit "Entity Names = Targetnames" "targetname" (leer: der Klassenname)
                const std::string tn = ed.ent.zielnamen ? ent->wert("targetname") : std::string();
                s.namen.push_back({o, box, tn.empty() ? ent->klassenname() : tn, farbe, entGewaehlt});
            }
        } else if (!welt) {
            box = kinderGrenzen(*ent, k);
            const std::string tn = ed.ent.zielnamen ? ent->wert("targetname") : std::string();
            // Name in der Mitte der Kinder, sonst am Ursprung (group.cpp:198-201)
            const Vektor3f nameMitte = karte::kastenGueltig(box) ? box.origin : ursprungVon(*ent);
            s.namen.push_back({nameMitte, box, tn.empty() ? ent->klassenname() : tn, farbe, entGewaehlt});
            // Pfeil der Gruppe (group.cpp:204-214: m_anglesDraw), ausgewaehlt
            // oder mit ausgewaehltem Kind hervorgehoben.
            const bool winkelZeigen = (kl != nullptr && kl->hasAngles) || ent->hat("angle") || ent->hat("angles");
            if (winkelZeigen && ed.anzeige.winkel) {
                bool kindGewaehlt = false;
                for (const auto& p : ent->kinder()) { kindGewaehlt = kindGewaehlt || ed.karte.auswahl.gewaehlt(ent.get(), p.get()); }
                const Vektor3f w = karte::entitaetsWinkel(ent->wert("angle"), ent->wert("angles"), ent->hat("angles"), true);
                const bool hell = entGewaehlt || kindGewaehlt;
                karte::pfeil(nameMitte, w, hell ? auswahl : farbe, hell ? s.wahl : s.welt);
            }
        }
        // Verbindungen (targetable.h): Linie zur Mitte jedes Ziels, in der
        // Klassenfarbe der Quelle.
        if (ed.anzeige.verbindungen) {
            for (const auto& [schl, wert] : ent->schluessel()) {
                if (schl.rfind("target", 0) != 0 || schl == "targetname") { continue; }
                const std::string rest = schl.substr(6);
                if (!rest.empty() && rest.find_first_not_of("0123456789") != std::string::npos) { continue; }
                const auto it = ziele.find(wert);
                if (it != ziele.end()) { karte::verbindung(mitte(*ent), it->second, farbe, s.welt); }
            }
        }
    }
    s.gpuAktuell = false;
}

// Die Einstellungen fuer XY_DrawGrid aus Editor und Farbschema.
karte::RasterEinstellungen rasterEinstellungen(int index, int schriftHoehe, int unterlaenge) {
    Editor& ed = e();
    karte::RasterEinstellungen r;
    r.raster = ed.raster.groesse();
    r.fang = ed.raster.einrastGroesse();
    r.potenz = ed.raster.potenz();
    r.zeigeRaster = ed.anzeige.raster;
    r.zeigeKoordinaten = ed.anzeige.koordinaten;
    r.zeigeAchsen = ed.anzeige.achsen;
    r.zeigeWorkzone = ed.anzeige.workzone2d;
    r.zeigeBloecke = ed.anzeige.bloecke;
    r.aktiv = ed.aktivXy == index;
    r.workMin = ed.workzone.origin - ed.workzone.extents;   // Select_getWorkZone
    r.workMax = ed.workzone.origin + ed.workzone.extents;
    // g_region_mins/maxs: ohne Region +-GetMaxGridCoord() (Region_defaultMinMax)
    r.regionAktiv = ed.region.aktiv;
    r.regionMin = ed.region.aktiv ? ed.region.min : Vektor3f(-ed.raster.maxKoord(), -ed.raster.maxKoord(), -ed.raster.maxKoord());
    r.regionMax = ed.region.aktiv ? ed.region.max : Vektor3f(ed.raster.maxKoord(), ed.raster.maxKoord(), ed.raster.maxKoord());
    const auto f3 = [](const Farben::F3& c) { return std::array<float, 3>{c[0], c[1], c[2]}; };
    r.hinten = f3(ed.farben.rasterHinten);
    r.klein = f3(ed.farben.rasterKlein);
    r.gross = f3(ed.farben.rasterGross);
    r.block = f3(ed.farben.rasterBlock);
    r.text = f3(ed.farben.rasterText);
    r.name = f3(ed.farben.ansichtName);
    r.achseX = farbe3(ed.farben.achseX);
    r.achseY = farbe3(ed.farben.achseY);
    r.achseZ = farbe3(ed.farben.achseZ);
    r.schriftHoehe = schriftHoehe;
    r.schriftUnterlaenge = unterlaenge;
    if (const auto w = ed.karte.welt()) {
        const std::string bs = w->wert("_blocksize");
        if (!bs.empty()) {
            int b[3] = {1024, 1024, 1024};
            const int n = std::sscanf(bs.c_str(), "%i %i %i", b, b + 1, b + 2);
            if (n == 1 || n == 2) { b[1] = b[2] = b[0]; }   // handle legacy case
            r.blockGroesse = {b[0], b[1], b[2]};
        }
    } else {
        r.weltDa = false;
    }
    return r;
}

}  // namespace

// Die Auswahlgrenzen (getBoundsSelected).
bool auswahlGrenzen(Kasten& b) {
    Editor& ed = e();
    b = Kasten();
    for (const auto& w : ed.karte.auswahl.elemente()) {
        if (const auto p = w.primitiv.lock()) {
            if (p->art() == karte::Primitiv::Art::Pinsel) {
                karte::kastenErweiternSicher(b, static_cast<const karte::Pinsel&>(*p).grenzen(ed.karte.kontext));
            } else {
                karte::kastenErweiternSicher(b, static_cast<const karte::Patch&>(*p).grenzen());
            }
        } else if (const auto ent = w.entitaet.lock()) {
            const karte::EntitaetsKlasse* kl = ent->klasse();
            if (kl != nullptr && kl->fixedsize) {
                const Vektor3f o = ursprungVon(*ent);
                karte::kastenErweiternSicher(b, karte::kastenAusMinMax(o + kl->mins, o + kl->maxs));
            } else {
                karte::kastenErweiternSicher(b, kinderGrenzen(*ent, ed.karte.kontext));
            }
        }
    }
    return karte::kastenGueltig(b);
}

std::vector<int> radEreignisse() {
    // ImGui fasst das Rad je Bild zusammen; Qt liefert ein wheelEvent je
    // Ereignis - die Ereignisse dieses Bilds stehen in InputEventsTrail.
    std::vector<int> v;
    for (const ImGuiInputEvent& ev : ImGui::GetCurrentContext()->InputEventsTrail) {
        if (ev.Type != ImGuiInputEventType_MouseWheel) { continue; }
        const float d = std::fabs(ev.MouseWheel.WheelY) > std::fabs(ev.MouseWheel.WheelX) ? ev.MouseWheel.WheelY : ev.MouseWheel.WheelX;
        if (d > 0) {
            v.push_back(1);
        } else if (d < 0) {
            v.push_back(-1);
        }
    }
    return v;
}

namespace {

// GetCenterBbox (xywindow.cpp:1938-1942): Auswahl, sonst die Kamera +-128.
Kasten mittelKasten() {
    Kasten b;
    if (e().karte.auswahl.anzahl() != 0 && auswahlGrenzen(b)) { return b; }
    return Kasten(e().kamera.ursprung, Vektor3f(128, 128, 128));
}

// Linien ohne Direct3D: mit ImGui (glLineWidth ueber die Dicke).
void imguiLinien(ImDrawList* dl, const karte::XyAnsicht& v, ImVec2 o, const std::vector<Linie>& l, float breite, unsigned muster = 0,
                 int faktor = 1) {
    for (const Linie& x : l) {
        const auto a = v.zuBild(x.a);
        const auto b = v.zuBild(x.b);
        if (muster == 0) {
            dl->AddLine(ImVec2{o.x + a[0], o.y + a[1]}, ImVec2{o.x + b[0], o.y + b[1]}, x.farbe, breite);
            continue;
        }
        // glLineStipple wie im GPU-Weg: Bit (Bildpunkt entlang der Hauptachse / Faktor) mod 16
        const float dx = b[0] - a[0];
        const float dy = b[1] - a[1];
        const int n = static_cast<int>(std::ceil(std::max(std::fabs(dx), std::fabs(dy))));
        for (int c = 0; c < n; ++c) {
            if (((muster >> ((static_cast<unsigned>(c) / static_cast<unsigned>(faktor)) & 15U)) & 1U) == 0U) { continue; }
            const float t0 = static_cast<float>(c) / static_cast<float>(n);
            const float t1 = static_cast<float>(c + 1) / static_cast<float>(n);
            dl->AddLine(ImVec2{o.x + a[0] + dx * t0, o.y + a[1] + dy * t0}, ImVec2{o.x + a[0] + dx * t1, o.y + a[1] + dy * t1}, x.farbe, breite);
        }
    }
}

// XYWnd::XY_Draw (xywindow.cpp:1840-1843): ohne "Solid selection boxes" kommt
// RENDER_LINESTIPPLE in den globalstate - $XY_OVERLAY (die Auswahl) zeichnet dann
// mit glLineStipple( 3, 0xaaaa ) (renderstate.cpp:517-518, 1868-1874).
constexpr unsigned kXyOverlayMuster = 0xAAAAU;
constexpr int kXyOverlayFaktor = 3;

}  // namespace

std::vector<int> xyFenster() {
    // forEachXYWnd: Regular/RegularLeft nur XY, sonst alle drei
    if (e().anordnung == befehle::Anordnung::Regular || e().anordnung == befehle::Anordnung::RegularLeft) { return {0}; }
    return {0, 1, 2};
}

void startPosition() {
    Editor& ed = e();
    // Scene_FindPlayerStart (map.cpp:430-454)
    const char* const klassen[] = {"info_player_start",   "info_player_deathmatch", "team_CTF_redplayer",
                                   "team_CTF_blueplayer", "team_CTF_redspawn",      "team_CTF_bluespawn"};
    std::shared_ptr<karte::Entitaet> start;
    for (const char* name : klassen) {
        for (const auto& ent : ed.karte.entitaeten()) {
            if (ent->klassenname() == name) {
                start = ent;
                break;
            }
        }
        if (start) { break; }
    }
    Vektor3f o(0, 0, 0);
    float winkel = 0;
    if (start && std::sscanf(start->wert("origin").c_str(), "%f %f %f", &o[0], &o[1], &o[2]) == 3) {
        winkel = static_cast<float>(std::atof(start->wert("angle").c_str()));   // string_read_float
    } else {
        o = Vektor3f(0, 0, 0);
    }
    // FocusViews (map.cpp:456-467)
    ed.kamera.setzeUrsprung(o);
    karte::Vektor3f w = ed.kamera.winkel;
    w[0] = 0;        // CAMERA_PITCH
    w[1] = winkel;   // CAMERA_YAW
    ed.kamera.setzeWinkel(w);
    for (auto& x : ed.xy) { x.ansicht.setzeUrsprung(o); }
}

namespace {

// NDIM1NDIM2( viewtype )
void dimensionen(Sicht s, int& d1, int& d2) {
    d1 = s == Sicht::YZ ? 1 : 0;
    d2 = s == Sicht::XY ? 1 : 2;
}

}  // namespace

void xyZeichnen(int index) {
    Editor& ed = e();
    XyFenster& f = ed.xy[static_cast<std::size_t>(index)];
    karte::XyAnsicht& v = f.ansicht;
    static const bool imTest = std::getenv("BHED_EDITORTEST") != nullptr;
    const ImVec2 o = ImGui::GetWindowPos();
    const ImVec2 groesse = ImGui::GetWindowSize();
    f.bildX = o.x;
    f.bildY = o.y;
    v.breite = std::max(1, static_cast<int>(groesse.x));
    v.hoehe = std::max(1, static_cast<int>(groesse.y));
    v.maxRaster = ed.raster.maxKoord();
    v.zumZeiger = ed.anzeige.zumZeiger;
    if (v.massstab < 1e-6F) { v.massstab = 1; }

    // --- Schrift der Ansichten: NRC zeichnet Text mit "Myriad Pro" 8 pt in
    // Geraetepunkten bei 96 dpi (glfont, ohne Hochskalierung) - hier Segoe UI
    // in derselben Groesse (Myriad Pro darf nicht mitgeliefert werden).
    ImFont* ansichtSchrift = glSchrift();   // GlobalOpenGL().m_font (OpenGL Font...)
    const float schriftPx = glSchriftPx();   // 8 pt: Geviert 10,67 -> ImGui-Hoehe
    ImGui::PushFont(ansichtSchrift, schriftPx / std::max(0.01F, g_app->dpiScale * g_app->uiScale));
    const int schriftHoehe = static_cast<int>(std::lround(ImGui::GetFontSize()));
    const int unterlaenge = static_cast<int>(std::lround(ImGui::GetFontSize() * 514.0F / 2724.0F));

    // m_view.Construct( m_projection, m_modelview, m_nWidth, m_nHeight ) / onSizeChanged
    ed.blicke[static_cast<std::size_t>(index)].bauen(v.projektion(), v.modelview(), static_cast<std::size_t>(v.breite),
                                                     static_cast<std::size_t>(v.hoehe));
    beobachterGroesse(index, v.breite, v.hoehe);

    // --- Szene (nur neu bauen, wenn sich etwas geaendert hat)
    Szene& s = g_szene[static_cast<std::size_t>(index)];
    const std::size_t stand = szeneStand();
    const std::size_t geoStand = geometrieStand();
    const auto t0 = std::chrono::steady_clock::now();
    bool neu = false;
    if (s.geoStand != geoStand || s.geoSicht != v.typ) {
        geoBauen(s, v.typ);
        s.geoStand = geoStand;
        ++ed.geoNeubauten;
        neu = true;
    }
    if (s.stand != stand || s.sicht != v.typ) {
        szeneBauen(s, v.typ);
        s.stand = stand;
        ++ed.restNeubauten;
        neu = true;
    }
    if (neu) { ed.xyNeubauMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); }
    const karte::RasterEinstellungen re = rasterEinstellungen(index, schriftHoehe, unterlaenge);
    const karte::RasterZeichnung raster = karte::rasterZeichnen(v, re);
    const karte::Matrix4 mvp = karte::multipliziert(v.projektion(), v.modelview());

    // Overlay-Linien: Groessenanzeige, Kamera-Symbol, Fadenkreuz
    std::vector<Linie> overlay;
    std::vector<karte::Text> overlayTexte;
    if (ed.anzeige.groesse2d && ed.karte.auswahl.anzahl() != 0) {
        Kasten b;
        if (auswahlGrenzen(b)) {
            const auto& c = ed.farben.auswahl;
            const auto g = karte::groessenAnzeige(v, b, karte::farbeAus(c[0] * .65F, c[1] * .65F, c[2] * .65F), schriftHoehe);
            overlay.insert(overlay.end(), g.linien.begin(), g.linien.end());
            overlayTexte = g.texte;
        }
    }
    const std::vector<Linie> kam = karte::kameraSymbol(v, ed.kamera.ursprung, ed.kamera.winkel, farbe3(ed.farben.kamera));
    std::vector<Linie> kreuz;
    if (ed.anzeige.fadenkreuz) { kreuz = karte::fadenkreuz(v, f.maus); }
    // GlobalSelectionSystem().renderSolid mit dem XYRenderer: globalstate
    // RENDER_COLOURARRAY | RENDER_COLOURWRITE - kein Tiefentest, keine
    // Fuellung (Pfeilspitzen als Dreieckskanten), $PLANE_WIRE_OVERLAY gelb und 2 breit.
    const karte::ManipZeichnung mz = manipulatorZeichnung(index);
    std::vector<Linie> manip1;
    std::vector<Linie> manip2;
    std::vector<Linie> manipPunkte;
    {
        const auto farbe = [](const karte::ManipFarbe& c) {
            return karte::farbeAus(static_cast<float>(c.r) / 255.0F, static_cast<float>(c.g) / 255.0F, static_cast<float>(c.b) / 255.0F,
                                   static_cast<float>(c.a) / 255.0F);
        };
        for (const auto& l : mz.ohneTiefe) { manip1.push_back(Linie{l.a, l.b, farbe(l.farbe)}); }   // $POINT (eSortControlFirst)
        for (const auto& l : mz.draht) { manip1.push_back(Linie{l.a, l.b, farbe(l.farbe)}); }       // $WIRE_OVERLAY
        for (const auto& d : mz.flach) {                                                          // $FLATSHADE_OVERLAY
            for (std::size_t i = 0; i < 3; ++i) { manip1.push_back(Linie{d.p[i], d.p[(i + 1) % 3], farbe(d.farbe)}); }
        }
        const Farbe gelb = karte::farbeAus(1, 1, 0);   // $PLANE_WIRE_OVERLAY: m_colour( 1, 1, 0, 1 )
        for (const auto& poly : mz.ebenen) {
            for (std::size_t i = 0; i < poly.size(); ++i) { manip2.push_back(Linie{poly[i], poly[(i + 1) % poly.size()], gelb}); }
        }
        for (const auto& l : mz.ebenenLinien) { manip2.push_back(Linie{l.a, l.b, gelb}); }
        for (const auto& pk : mz.grossePunkte) { manipPunkte.push_back(Linie{pk.p, pk.p, farbe(pk.farbe)}); }   // $BIGPOINT
    }
    // MiscModel::renderSolid: ausgewaehlt der RenderablePivot (16 Bildpunkte,
    // Weltachsen x rot, y gruen, z blau; $PIVOT mit Breite 2)
    if (ed.karte.kontext.modelle != nullptr) {
        const float l = 16.0F / v.massstab;
        for (const auto& w : ed.karte.auswahl.elemente()) {
            const auto ent = w.entitaet.lock();
            if (!ent || !w.primitiv.expired() || ent->istBehaelter() || ent->klasse() == nullptr || !ent->klasse()->miscmodelIs || !ent->sichtbar()) {
                continue;
            }
            const Vektor3f pivot = ursprungVon(*ent);
            manip2.push_back(Linie{pivot, pivot + Vektor3f(l, 0, 0), karte::farbeAus(1, 0, 0)});
            manip2.push_back(Linie{pivot, pivot + Vektor3f(0, l, 0), karte::farbeAus(0, 1, 0)});
            manip2.push_back(Linie{pivot, pivot + Vektor3f(0, 0, l), karte::farbeAus(0, 0, 1)});
        }
    }
    const karte::AuswahlRechteck& rechteck = ed.beobachter[static_cast<std::size_t>(index)].rechteck;
    // BrushInstance::renderComponents / renderComponentsSelected, PatchInstance
    // ebenso: $LATTICE, $POINT (6), $SELPOINT (4, colour_selected)
    const KomponentenZeichnung komp = komponentenZeichnung(ed.blicke[static_cast<std::size_t>(index)]);
    // BrushClipPlane::render ohne RENDER_FILL: Umriss und ein Strich in Richtung
    // der Normalen (Winding_Centroid + Normale * m_maxWorldCoord * 4), $CLIPPER_OVERLAY
    std::vector<Linie> clipLinien;
    {
        const Farbe fc = farbe3(ed.farben.clipper);
        for (const ClipperFlaeche& cf : clipperFlaechen()) {
            for (std::size_t i = 0; i < cf.punkte.size(); ++i) { clipLinien.push_back(Linie{cf.punkte[i], cf.punkte[(i + 1) % cf.punkte.size()], fc}); }
            clipLinien.push_back(Linie{cf.mitte, cf.mitte + cf.normale * static_cast<float>(karte::kMaxWeltKoordinate * 4), fc});
        }
    }

    // Stufe 3 Punkt 5: CPointfile ($POINTFILE: rot, 4 breit; eSortFullbright),
    // CPortalsRender::renderWireframe (width_2d, color_2d; eSortOverlayFirst) und
    // Feedback_draw2D (nach der Groessenanzeige)
    const std::vector<Linie> leckLinien = punktdateiLinien();
    float portalBreite = 1.0F;
    const std::vector<Linie> portalLinien = portalLinien2d(portalBreite);
    const RueckmeldungZeichnung rueck = rueckmeldungZeichnung2d(v.typ);
    const std::vector<PluginLinien> pluginLinien = pluginLinien2d();   // Stufe 4 Punkt 3: bobToolz (renderWireframe)
    const Farbe wicklungFarbe = karte::farbeAus(0.133F, 0.4F, 1.0F, 0.5F);   // glColor4f( 0.133f, 0.4f, 1.0f, 0.5f )

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float hinten[4] = {ed.farben.rasterHinten[0], ed.farben.rasterHinten[1], ed.farben.rasterHinten[2], 1.0F};
    if (kartegpu::verfuegbar() && kartegpu::beginnen(100 + index, v.breite, v.hoehe, ed.anzeige.msaa, hinten)) {
        if (!s.geoGpuAktuell) {
            s.gpuGeo.setzen(s.geo);
            s.geoGpuAktuell = true;
        }
        if (!s.gpuAktuell) {
            s.gpuWelt.setzen(s.welt);
            s.gpuWahl.setzen(s.wahl);
            s.gpuAktuell = true;
        }
        if (f.hintergrund.bild != nullptr && f.hintergrund.sicht == v.typ) {   // m_backgroundImage.render( m_viewType )
            // GL_QUADS mit glColor4f( 1, 1, 1, _alpha ), GL_MODULATE, Mischen, ohne Tiefe
            int d1 = 0;
            int d2 = 0;
            dimensionen(v.typ, d1, d2);
            const XyFenster::Hintergrund& hb = f.hintergrund;
            const auto ecke = [&](float x, float y, float s, float t) {
                kartegpu::Punkt3 p{};
                p.pos[d1] = x;
                p.pos[d2] = y;
                p.normale[2] = 1;
                p.st[0] = s;
                p.st[1] = t;
                p.farbe = 0xFFFFFFFFU;
                return p;
            };
            const kartegpu::Punkt3 a = ecke(hb.xmin, hb.ymin, 0, 1);
            const kartegpu::Punkt3 b2 = ecke(hb.xmax, hb.ymin, 1, 1);
            const kartegpu::Punkt3 c = ecke(hb.xmax, hb.ymax, 1, 0);
            const kartegpu::Punkt3 d = ecke(hb.xmin, hb.ymax, 0, 0);
            kartegpu::Zustand3 z;
            z.matrix = &mvp[0];
            z.bild = hb.bild;
            z.tiefe = kartegpu::TiefenTest::Aus;
            z.tiefeSchreiben = false;
            z.wegschneiden = false;
            z.mischen = kartegpu::Mischen::Alpha;
            kartegpu::dreiecke(std::vector<kartegpu::Punkt3>{a, b2, c, a, c, d}, z);
        }
        kartegpu::zeichnen(raster.linien, &mvp[0], 1.0F);
        kartegpu::zeichnen(raster.linien2, &mvp[0], 2.0F);
        kartegpu::zeichnen(s.gpuGeo, &mvp[0], 1.0F);
        kartegpu::zeichnen(s.gpuWelt, &mvp[0], 1.0F);
        kartegpu::zeichnen(leckLinien, &mvp[0], 4.0F);
        kartegpu::zeichnen(portalLinien, &mvp[0], portalBreite);
        for (const PluginLinien& pl : pluginLinien) { kartegpu::zeichnen(pl.linien, &mvp[0], pl.breite); }
        if (ed.anzeige.keineStrichelung) {
            kartegpu::zeichnen(s.gpuWahl, &mvp[0], 2.0F);
        } else {
            kartegpu::zeichnen(s.gpuWahl, &mvp[0], 2.0F, kXyOverlayMuster, kXyOverlayFaktor);
        }
        kartegpu::zeichnen(komp.gitter, &mvp[0], 1.0F);
        kartegpu::punkte(komp.punkte, &mvp[0], 6.0F);
        kartegpu::punkte(komp.gewaehlt, &mvp[0], 4.0F);
        kartegpu::zeichnen(clipLinien, &mvp[0], 1.0F);
        kartegpu::zeichnen(manip1, &mvp[0], 1.0F);
        kartegpu::zeichnen(manip2, &mvp[0], 2.0F);
        kartegpu::punkte(manipPunkte, &mvp[0], 6.0F);
        kartegpu::zeichnen(overlay, &mvp[0], 1.0F);
        {   // Feedback_draw2D: Punkte (4), Umriss, die Flaeche gemischt
            kartegpu::punkte(rueck.punkte, &mvp[0], 4.0F);
            kartegpu::zeichnen(rueck.linien, &mvp[0], 1.0F);
            std::vector<kartegpu::Punkt3> flaeche;
            for (const auto& poly : rueck.flaechen) {
                for (std::size_t i = 1; i + 1 < poly.size(); ++i) {   // GL_POLYGON
                    for (const Vektor3f& q : {poly[0], poly[i], poly[i + 1]}) {
                        kartegpu::Punkt3 pt{};
                        pt.pos[0] = q[0];
                        pt.pos[1] = q[1];
                        pt.pos[2] = q[2];
                        pt.normale[2] = 1;
                        pt.farbe = wicklungFarbe;
                        flaeche.push_back(pt);
                    }
                }
            }
            if (!flaeche.empty()) {
                kartegpu::Zustand3 z;
                z.matrix = &mvp[0];
                z.tiefe = kartegpu::TiefenTest::Aus;
                z.tiefeSchreiben = false;
                z.wegschneiden = false;
                z.mischen = kartegpu::Mischen::Alpha;
                kartegpu::dreiecke(flaeche, z);
            }
        }
        kartegpu::zeichnen(kam, &mvp[0], 1.0F);
        kartegpu::zeichnen(kreuz, &mvp[0], 1.0F);
        kartegpu::auswahlRechteck(rechteck.min[0], rechteck.min[1], rechteck.max[0], rechteck.max[1], static_cast<int>(rechteck.art));
        kartegpu::beenden();
        dl->AddImage(reinterpret_cast<ImTextureID>(kartegpu::textur(100 + index)), o, ImVec2{o.x + groesse.x, o.y + groesse.y});
    } else {
        dl->AddRectFilled(o, ImVec2{o.x + groesse.x, o.y + groesse.y}, farbe3(ed.farben.rasterHinten));
        imguiLinien(dl, v, o, raster.linien, 1.0F);
        imguiLinien(dl, v, o, raster.linien2, 2.0F);
        imguiLinien(dl, v, o, s.geo, 1.0F);
        imguiLinien(dl, v, o, s.welt, 1.0F);
        imguiLinien(dl, v, o, leckLinien, 4.0F);
        imguiLinien(dl, v, o, portalLinien, portalBreite);
        for (const PluginLinien& pl : pluginLinien) { imguiLinien(dl, v, o, pl.linien, pl.breite); }
        imguiLinien(dl, v, o, s.wahl, 2.0F, ed.anzeige.keineStrichelung ? 0U : kXyOverlayMuster, kXyOverlayFaktor);
        imguiLinien(dl, v, o, clipLinien, 1.0F);
        imguiLinien(dl, v, o, manip1, 1.0F);
        imguiLinien(dl, v, o, manip2, 2.0F);
        imguiLinien(dl, v, o, overlay, 1.0F);
        for (const auto& poly : rueck.flaechen) {   // Feedback_draw2D ohne Direct3D
            std::vector<ImVec2> ecken;
            for (const Vektor3f& q : poly) {
                const auto b = v.zuBild(q);
                ecken.push_back(ImVec2{o.x + b[0], o.y + b[1]});
            }
            dl->AddConvexPolyFilled(ecken.data(), static_cast<int>(ecken.size()), wicklungFarbe);
        }
        imguiLinien(dl, v, o, rueck.linien, 1.0F);
        for (const Linie& pk : rueck.punkte) {
            const auto b = v.zuBild(pk.a);
            dl->AddRectFilled(ImVec2{o.x + b[0] - 2.0F, o.y + b[1] - 2.0F}, ImVec2{o.x + b[0] + 2.0F, o.y + b[1] + 2.0F}, pk.farbe);
        }
        imguiLinien(dl, v, o, kam, 1.0F);
        imguiLinien(dl, v, o, kreuz, 1.0F);
    }
    // --- Texte (glRasterPos: linke Unterkante)
    dl->PushClipRect(o, ImVec2{o.x + groesse.x, o.y + groesse.y}, true);
    const auto text = [&](const karte::Text& t) {
        dl->AddText(ImVec2{o.x + t.x, o.y + t.y - static_cast<float>(schriftHoehe)}, t.farbe, t.text.c_str());
    };
    for (const karte::Text& t : raster.texte) { text(t); }
    for (const karte::Text& t : overlayTexte) { text(t); }
    for (const auto& t : mz.texte) {   // ClipperPoint::render: drawChar am Rasterpunkt
        const auto p = v.zuBild(t.p);
        dl->AddText(ImVec2{o.x + p[0], o.y + p[1] - static_cast<float>(schriftHoehe)}, IM_COL32(t.farbe.r, t.farbe.g, t.farbe.b, t.farbe.a), t.text.c_str());
    }
    // Namen (RenderableNamedEntity): ausgewaehlt immer, sonst wenn ShowNames
    // und aabb_fits_view; normal in der Klassenfarbe, ausgewaehlt gelb, mit
    // schwarzem Schatten.
    for (const Name& n : s.namen) {
        if (!(n.gewaehlt || (ed.anzeige.namen && karte::kastenPasst(n.box, v, ed.anzeige.namenVerhaeltnis)))) { continue; }
        const auto p = v.zuBild(n.anker);
        if (p[0] < -200 || p[1] < 0 || p[0] > groesse.x || p[1] > groesse.y + static_cast<float>(schriftHoehe)) { continue; }
        const ImVec2 at{o.x + p[0], o.y + p[1] - static_cast<float>(schriftHoehe)};
        dl->AddText(ImVec2{at.x + 1, at.y + 1}, IM_COL32(0, 0, 0, 255), n.text.c_str());
        dl->AddText(at, n.gewaehlt ? IM_COL32(255, 255, 0, 255) : n.farbe, n.text.c_str());
    }
    dl->PopClipRect();
    // Fensterumriss der aktiven Ansicht (overlayDraw, 1 Bildpunkt): seit
    // Stufe 3 Punkt 0b der Rahmen des Feldes (app_karte.cpp feldRahmen) in der
    // Akzentfarbe des Farbschemas statt der Farbe je Ansicht.
    ImGui::PopFont();

    // --- Maus (XYGLWidget -> XY_MouseDown / Moved / Up)
    const ImGuiIO& io = ImGui::GetIO();
    const bool ueber = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
                       !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    const int mx = static_cast<int>(std::lround(io.MousePos.x - o.x));
    const int my = static_cast<int>(std::lround(io.MousePos.y - o.y));
    const auto fx = static_cast<float>(mx);
    const auto fy = static_cast<float>(my);
    const unsigned mods = (io.KeyShift ? kShift : 0U) | (io.KeyAlt ? kAlt : 0U) | (io.KeyCtrl ? kControl : 0U);
    // modifiers_for_flags
    karte::Zusatz zusatz;
    zusatz.shift = (mods & kShift) != 0U;
    zusatz.strg = (mods & kControl) != 0U;
    zusatz.alt = (mods & kAlt) != 0U;
    const bool bewegt = io.MouseDelta.x != 0.0F || io.MouseDelta.y != 0.0F;   // mouseMoveEvent
    // Qt liefert die Pan-Deltas in logischen Punkten (cursor.h), Klicks in
    // Geraetepunkten.
    const float qt = std::max(1.0F, std::round(g_app->dpiScale));
    // Mausrad: ein Zoomschritt je Ereignis, macht die Ansicht aktiv.
    if (ueber) {
        for (const int r : radEreignisse()) {
            ed.aktivXy = index;
            if (r > 0) {
                v.hineinMaus(mx, my);
            } else {
                v.hinausMaus(mx, my);
            }
        }
    }
    // XYWnd::SetCustomPivotOrigin: die Blickachse bleibt
    const auto eigenerUrsprung = [&]() {
        bool gesetzt[3] = {true, true, true};
        gesetzt[karte::blickachse(v.typ)] = false;
        ed.bearbeitung.eigenenUrsprungSetzen(v.zuPunkt(mx, my), gesetzt);
    };
    const int knopf[3] = {ImGuiMouseButton_Left, ImGuiMouseButton_Right, ImGuiMouseButton_Middle};
    const unsigned flag[3] = {kLinks, kRechts, kMitte};
    for (int b = 0; b < 3; ++b) {
        if (ueber && ImGui::IsMouseClicked(knopf[b]) && !f.gedrueckt) {
            ed.aktivXy = index;
            f.gedrueckt = true;
            f.tasten = flag[b] | mods;   // buttons_for_event_button
            const unsigned t = f.tasten;
            // Stufe 4 Punkt 3: onMouseDown der Plugins zuerst (XYWindowMouseDown_connect,
            // connectFirst; DTreePlanter::mouseDown gibt SIGNAL_STOP_EMISSION zurueck)
            if (pluginXyDruck(index, b, v.zuPunkt(mx, my))) {
                f.pluginDruck = true;
                continue;
            }
            if (t == kRechts) {   // Move_Begin: FreezePointer
                f.verschieben = true;
                f.ankerX = io.MousePos.x;
                f.ankerY = io.MousePos.y;
                f.letzteX = io.MousePos.x;
                f.letzteY = io.MousePos.y;
                f.entitaetErzeugen = true;   // EntityCreate_MouseDown
                f.erzeugX = mx;
                f.erzeugY = my;
            } else if (t == (kRechts | kAlt)) {   // Zoom_Begin
                f.zoomen = true;
                f.dragZoom = 0;
                f.zoomX = mx;
                f.zoomY = my;
                f.ankerX = io.MousePos.x;
                f.ankerY = io.MousePos.y;
                f.letzteX = io.MousePos.x;
                f.letzteY = io.MousePos.y;
            } else if (t == kLinks && ed.bearbeitung.neuerPinselMoeglich()) {   // NewBrushDrag_Begin
                ed.bearbeitung.neuerPinselBeginnen(v, mx, my);
            } else if (t == (kControl | kMitte)) {   // XYWnd_PositionCamera
                ed.kamera.setzeUrsprung(karte::kameraVersetzen(v, mx, my, ed.raster.einrastGroesse(), ed.kamera.ursprung));
            } else if (t == kMitte) {   // XYWnd_OrientCamera
                ed.kamera.setzeWinkel(karte::kameraDrehen(v, mx, my, ed.kamera.ursprung, ed.kamera.winkel));
            } else if (t == (kMitte | kShift)) {   // SetCustomPivotOrigin
                eigenerUrsprung();
            } else {   // m_window_observer->onMouseDown
                beobachterDruck(index, fx, fy, b, zusatz);
            }
        }
    }
    if (f.gedrueckt) {
        // Bewegung waehrend des Drucks
        if (f.verschieben || f.zoomen) {
            const bool gueltig = ImGui::IsMousePosValid();
            const float dx = gueltig ? (io.MousePos.x - f.letzteX) / qt : 0.0F;
            const float dy = gueltig ? (io.MousePos.y - f.letzteY) / qt : 0.0F;
            const int ix = static_cast<int>(std::lround(dx));
            const int iy = static_cast<int>(std::lround(dy));
            if (ix != 0 || iy != 0) {
                if (f.verschieben) {
                    // EntityCreate_MouseMove( x, y ) mit den Deltas des FreezePointer
                    if (f.entitaetErzeugen && (f.erzeugX != ix || f.erzeugY != iy)) { f.entitaetErzeugen = false; }
                    v.rollen(-ix, iy);   // Scroll( -x, y )
                } else if (iy != 0) {
                    f.dragZoom += iy;
                    const int schwelle = 16;
                    while (std::abs(f.dragZoom) > schwelle) {
                        if (f.dragZoom > 0) {
                            v.hinausMaus(f.zoomX, f.zoomY);
                            f.dragZoom -= schwelle;
                        } else {
                            v.hineinMaus(f.zoomX, f.zoomY);
                            f.dragZoom += schwelle;
                        }
                    }
                }
                f.letzteX += static_cast<float>(ix) * qt;
                f.letzteY += static_cast<float>(iy) * qt;
                if (!imTest) {
                    // Zeiger zurueck an die Ausgangslage (freeze_pointer)
                    ImGui::TeleportMousePos(ImVec2{f.ankerX, f.ankerY});
                    f.letzteX = f.ankerX;
                    f.letzteY = f.ankerY;
                }
            }
            ImGui::SetMouseCursor(ImGuiMouseCursor_None);   // BlankCursor
        } else {
            // XY_MouseMoved( x, y, buttons )
            const auto mausBewegt = [&](int x, int y) {
                const unsigned gehalten = (ImGui::IsMouseDown(ImGuiMouseButton_Left) ? kLinks : 0U) |
                                          (ImGui::IsMouseDown(ImGuiMouseButton_Right) ? kRechts : 0U) |
                                          (ImGui::IsMouseDown(ImGuiMouseButton_Middle) ? kMitte : 0U) | mods;
                if (ed.bearbeitung.neuerPinselAktiv()) {   // NewBrushDrag( x, y, shift, ctrl )
                    ed.bearbeitung.neuerPinselZiehen(v, x, y, (mods & kShift) != 0U, (mods & kControl) != 0U);
                } else if (gehalten == (kControl | kMitte)) {
                    ed.kamera.setzeUrsprung(karte::kameraVersetzen(v, x, y, ed.raster.einrastGroesse(), ed.kamera.ursprung));
                } else if (gehalten == kMitte) {
                    ed.kamera.setzeWinkel(karte::kameraDrehen(v, x, y, ed.kamera.ursprung, ed.kamera.winkel));
                } else if (gehalten == (kMitte | kShift)) {
                    eigenerUrsprung();
                } else {
                    beobachterBewegung(index, static_cast<float>(x), static_cast<float>(y));
                }
            };
            // XYWnd::chaseMouseMotion (xywindow.cpp:302-345): mit M1 (ohne Zusatztaste
            // beim Druck) naeher als 16 Punkte am Rand rollt die Ansicht, die Bewegung
            // selbst geht dann nicht an XY_MouseMoved.
            const auto jagdBewegung = [&](int x, int y) {
                f.jagdDx = 0;
                f.jagdDy = 0;
                if (ed.anzeige.jagen && f.tasten == kLinks) {
                    const int epsilon = 16;
                    if (x < epsilon) {
                        f.jagdDx = std::max(x, 0) - epsilon;
                    } else if ((x - v.breite) > -epsilon) {
                        f.jagdDx = std::min(x - v.breite, 0) + epsilon;
                    }
                    if (y < epsilon) {
                        f.jagdDy = std::max(y, 0) - epsilon;
                    } else if ((y - v.hoehe) > -epsilon) {
                        f.jagdDy = std::min(y - v.hoehe, 0) + epsilon;
                    }
                    if (f.jagdDx != 0 || f.jagdDy != 0) {
                        f.jagdX = x;
                        f.jagdY = y;
                        if (!f.jagen) {   // g_chasemouse_timer.start(); g_chasemouse_caller.start( 4 )
                            f.jagdUhr = ImGui::GetTime();
                            f.jagen = true;
                        }
                        return true;
                    }
                }
                f.jagen = false;   // g_chasemouse_caller.stop()
                return false;
            };
            if (bewegt && !jagdBewegung(mx, my)) {   // m_deferred_motion
                mausBewegt(mx, my);
            } else if (f.jagen && ImGui::GetTime() > f.jagdUhr) {
                // XYWnd::ChaseMouse (xywindow.cpp:293-300): Faktor = Millisekunden seit dem
                // letzten Mal / 10; Scroll( float_to_integer( m * dx ), float_to_integer( m * -dy ) )
                const double jetzt = ImGui::GetTime();
                const float faktor = static_cast<float>((jetzt - f.jagdUhr) * 1000.0) / 10.0F;
                f.jagdUhr = jetzt;
                v.rollen(static_cast<int>(std::lrint(faktor * static_cast<float>(f.jagdDx))),
                         static_cast<int>(std::lrint(faktor * static_cast<float>(-f.jagdDy))));
                mausBewegt(f.jagdX, f.jagdY);
            }
        }
        // Loslassen (jede Taste beendet: m_buttonstate = 0)
        int losKnopf = -1;
        for (int b = 0; b < 3; ++b) {
            if (ImGui::IsMouseReleased(knopf[b])) { losKnopf = b; }
        }
        if (losKnopf >= 0) {
            if (f.pluginDruck) {   // XY_MouseDown lief nicht: auch kein onMouseUp fuer den Beobachter
                f.pluginDruck = false;
            } else if (f.verschieben) {
                f.verschieben = false;
                if (!imTest) { ImGui::TeleportMousePos(ImVec2{f.ankerX, f.ankerY}); }   // unfreeze_pointer( false )
                if (f.entitaetErzeugen) {   // EntityCreate_MouseUp: OnContextMenu
                    f.entitaetErzeugen = false;
                    entitaetenMenueOeffnen(false);
                }
            } else if (f.zoomen) {
                f.zoomen = false;
            } else if (ed.bearbeitung.neuerPinselAktiv()) {   // NewBrushDrag_End
                // M1 ohne neuen Pinsel: Tunnel-Auswahl
                if (!ed.bearbeitung.neuerPinselBeenden()) { beobachterLoslassen(index, fx, fy, losKnopf, zusatz); }
            } else {
                beobachterLoslassen(index, fx, fy, losKnopf, zusatz);
            }
            f.gedrueckt = false;
            f.tasten = 0;
            f.jagen = false;   // mouseReleaseEvent: chaseMouseMotion beendet die Uhr
        }
    } else if (ueber && bewegt) {   // XY_MouseMoved ohne Taste: Manipulator hervorheben
        beobachterBewegung(index, fx, fy);
    }
    // XY_MouseMoved ohne gefangene Bewegung: Position in der Statuszeile
    if (ueber && !f.verschieben && !f.zoomen) {
        f.maus = v.zuPunkt(mx, my, ed.raster.einrastGroesse());
        char z[96];
        std::snprintf(z, sizeof(z), "x: %6.1f  y: %6.1f  z: %6.1f", static_cast<double>(f.maus[0]),
                      static_cast<double>(f.maus[1]), static_cast<double>(f.maus[2]));
        ed.statusPosition = z;
    }
}

bool xyPixel(int index, int x, int y, unsigned char rgba[4]) { return kartegpu::pixel(100 + index, x, y, rgba); }

namespace {

bool hintergrundKasten(Sicht s, Kasten& b) {   // getBoundsSelected, extents in beiden Achsen > 0
    if (!auswahlGrenzen(b)) { return false; }
    int d1 = 0;
    int d2 = 0;
    dimensionen(s, d1, d2);
    return b.extents[static_cast<std::size_t>(d1)] > 0 && b.extents[static_cast<std::size_t>(d2)] > 0;
}

void keineAuswahlMelden() {
    qtMeldung("No selection", "Select some objects to get the bounding box for image.\n", MeldungArt::Fehler, false, nullptr);
}

void hintergrundFrei(XyFenster& f) {   // BackgroundImage::free_tex
    if (f.hintergrund.bild != nullptr) {
        kartegpu::bildFreigeben(f.hintergrund.bild);
        f.hintergrund.bild = nullptr;
    }
    f.hintergrund.datei.clear();
}

}  // namespace

void hintergrundBildFrei(int index) { hintergrundFrei(e().xy[static_cast<std::size_t>(index)]); }

// BackgroundImage::set (xywindow.cpp:1178-1212), der Dateiname schon relativ zum Spiel
void hintergrundBildSetzen(int index, const std::string& relativ) {
    Editor& ed = e();
    XyFenster& f = ed.xy[static_cast<std::size_t>(index)];
    Kasten b;
    if (!hintergrundKasten(f.ansicht.typ, b)) {
        keineAuswahlMelden();
        return;
    }
    hintergrundFrei(f);
    // PathExtensionless( filename ), QERApp_LoadImage
    const std::size_t punkt = relativ.find_last_of('.');
    const std::size_t strich = relativ.find_last_of("/\\");
    const std::string ohne = punkt != std::string::npos && (strich == std::string::npos || punkt > strich) ? relativ.substr(0, punkt) : relativ;
    std::vector<unsigned char> rgba;
    int w = 0;
    int h = 0;
    if (!kartenBildRgba(ohne, rgba, w, h)) {
        fehler("Could not load texture " + ohne);
        return;
    }
    void* bild = kartegpu::bildAnlegen(rgba.data(), w, h, 0);   // LoadTextureRGBA
    if (bild == nullptr) { return; }
    ausgabe("Loaded background texture " + relativ);
    int d1 = 0;
    int d2 = 0;
    dimensionen(f.ansicht.typ, d1, d2);
    f.hintergrund.bild = bild;
    f.hintergrund.sicht = f.ansicht.typ;
    f.hintergrund.xmin = b.origin[static_cast<std::size_t>(d1)] - b.extents[static_cast<std::size_t>(d1)];
    f.hintergrund.ymin = b.origin[static_cast<std::size_t>(d2)] - b.extents[static_cast<std::size_t>(d2)];
    f.hintergrund.xmax = b.origin[static_cast<std::size_t>(d1)] + b.extents[static_cast<std::size_t>(d1)];
    f.hintergrund.ymax = b.origin[static_cast<std::size_t>(d2)] + b.extents[static_cast<std::size_t>(d2)];
    f.hintergrund.datei = relativ;
}

// WXY_SetBackgroundImage -> ActiveXY()->setBackgroundImage(): erst die Auswahl,
// dann der Dateidialog (background_image_dialog: m_userGamePath + "textures/"),
// der Pfad relativ zum Ordner des Spiels (path_make_relative( findRoot )).
void hintergrundBildDialog() {
    Editor& ed = e();
    const int index = ed.aktivXy;
    XyFenster& f = ed.xy[static_cast<std::size_t>(index)];
    Kasten b;
    if (!hintergrundKasten(f.ansicht.typ, b)) {
        keineAuswahlMelden();
        return;
    }
    hintergrundFrei(f);
    std::string start = kartenBenutzerOrdner();
    if (start.size() > 5 && start.compare(start.size() - 5, 5, "maps/") == 0) { start.erase(start.size() - 5); }   // m_userGamePath
    const std::string mitTexturen = start + "textures/";
    std::error_code ec;
    const std::string pfad = kartenDateiDialog(true, "Background Image", "*.*|*.*",
                                                      std::filesystem::is_directory(std::filesystem::path(mitTexturen), ec) ? mitTexturen : start);
    if (pfad.empty()) { return; }
    std::string datei = pfad;
    std::replace(datei.begin(), datei.end(), '\\', '/');
    std::string relativ = datei;
    if (ed.karteSpiel) {
        for (const std::string& o : ed.karteSpiel->ordner) {   // GlobalFileSystem().findRoot
            std::string wurzel = karte::ordnerBereinigt(o);
            if (datei.size() > wurzel.size() && _strnicmp(datei.c_str(), wurzel.c_str(), wurzel.size()) == 0) {
                relativ = datei.substr(wurzel.size());
                break;
            }
        }
    }
    if (relativ == datei) { warnung("WARNING: could not extract the relative path, using full path instead"); }
    hintergrundBildSetzen(index, relativ);
}
int xyAbtastungen(int index) { return kartegpu::abtastungen(100 + index); }

// --- Befehle der 2D-Ansichten (xywindow.cpp:1938-2003, 2096-2177) ------------

void xyBefehl(const std::string& name) {
    Editor& ed = e();
    XyFenster& aktiv = ed.xy[static_cast<std::size_t>(ed.aktivXy)];
    if (name == "NextView" || name == "ViewTop" || name == "ViewFront" || name == "ViewSide") {
        // XY_SetViewType: im 4er-Layout nur zentrieren
        if (ed.anordnung != befehle::Anordnung::Split) {
            const Sicht neu = name == "NextView"  ? karte::naechsteSicht(aktiv.ansicht.typ)
                              : name == "ViewTop" ? Sicht::XY
                              : name == "ViewFront" ? Sicht::XZ
                                                    : Sicht::YZ;
            aktiv.ansicht.typ = neu;
            aktiv.ansicht.setzeUrsprung(mittelKasten().origin);   // XYWnd_Centralize
        } else {
            const Vektor3f p = mittelKasten().origin;
            for (const int i : xyFenster()) { ed.xy[static_cast<std::size_t>(i)].ansicht.setzeUrsprung(p); }
        }
    } else if (name == "ZoomIn") {
        aktiv.ansicht.hinein();
    } else if (name == "ZoomOut") {
        aktiv.ansicht.hinaus();
    } else if (name == "Zoom100") {
        for (const int i : xyFenster()) { ed.xy[static_cast<std::size_t>(i)].ansicht.setzeMassstab(1); }
    } else if (name == "CenterXYView") {
        const Vektor3f p = mittelKasten().origin;
        for (const int i : xyFenster()) { ed.xy[static_cast<std::size_t>(i)].ansicht.setzeUrsprung(p); }
    } else if (name == "XYFocusOnSelected") {
        const Kasten b = mittelKasten();
        for (const int i : xyFenster()) { ed.xy[static_cast<std::size_t>(i)].ansicht.aufGrenzen(b); }
    }
}

}  // namespace bhed::gui::karteneditor
