// app_karte_auswahl.cpp - Karten-Editor: Auswaehlen und Bearbeiten mit der Maus
// und die Befehle der Auswahl.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/selection.cpp
// (RadiantWindowObserver: onSizeChanged, onMouseDown, onMouseMotion,
// onMouseUp, onModifierDown/Up, mouse_moved_epsilon, incMouseMove; Selector_:
// m1selecting, m2selecting, modifier_for_mouseMoved, modifier_for_state,
// getDeviceArea, m2testSelect, mouseDown/Moved/Up, testSelect_simpleM1;
// Manipulator_: getEpsilon, mouseDown/Moved/Up, highlight), radiant/tools.cpp
// (DragMode, TranslateMode, RotateMode, ScaleMode, SkewMode,
// ToggleRotateScaleModes, ToggleDragSkewModes, die Statustexte,
// ComponentMode_SelectionChanged, SelectionSystem_DefaultMode) und
// radiant/select.cpp (Select_registerCommands: Undo, Redo, Copy, Paste,
// PasteToCamera, MoveToCamera, CloneSelection(AndMakeUnique),
// DeleteSelection(2/3), UnSelectSelection(2), InvertSelection,
// SelectAllOfType, SnapToGrid, Selection_Nudge*, Selection_MoveUp/Down,
// HideSelected, ShowHidden).
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
#include "karteneditor.h"

#include "app_internal.h"
#include "bhed/karte/auswahlbefehle.h"
#include "bhed/karte/auswahlsystem.h"
#include "bhed/karte/komponenten.h"
#include "bhed/karte/patch.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace bhed::gui::karteneditor {

namespace {

using karte::AuswahlArt;
using karte::GeraeteVektor;
using karte::ManipModus;

Editor& e() { return editor(); }

constexpr float kBewegEps = .01F;   // RadiantWindowObserver::m_moveEpsilon

FensterBeobachter& beob(int f) { return e().beobachter[static_cast<std::size_t>(f)]; }
const karte::Blick& blick(int f) { return e().blicke[static_cast<std::size_t>(f)]; }

// window_to_normalised_device
GeraeteVektor geraet(const FensterBeobachter& b, float x, float y) { return karte::fensterZuGeraet(x, y, b.breite, b.hoehe); }

// updateEpsilon: g_SELECT_EPSILON in Geraetekoordinaten
GeraeteVektor epsilon(const FensterBeobachter& b) {
    const float s = e().bearbeitung.waehlEpsilon;
    return GeraeteVektor(s / static_cast<float>(b.breite), s / static_cast<float>(b.hoehe));
}

// Manipulator_::getEpsilon
GeraeteVektor manipEpsilon(const FensterBeobachter& b) {
    const GeraeteVektor eps = epsilon(b);
    const float s = e().bearbeitung.waehlEpsilon;
    switch (e().bearbeitung.modus()) {
    case ManipModus::Clipper: return GeraeteVektor(eps.x() / s * (s + 4), eps.y() / s * (s + 4));
    case ManipModus::Ziehen:
    case ManipModus::UV: return eps;
    default: return GeraeteVektor(eps.x() / s * 8, eps.y() / s * 8);   // transformOrigin_isTranslatable
    }
}

// mouse_moved_epsilon
bool bewegtEps(const FensterBeobachter& b, float x, float y, const GeraeteVektor& start, float& weg) {
    if (weg > kBewegEps) { return true; }
    const GeraeteVektor g = geraet(b, x, y);
    const float jetzt = std::max(std::fabs(g.x() - start.x()), std::fabs(g.y() - start.y()));
    weg = std::max(weg, jetzt);
    return weg > kBewegEps;
}

const karte::Zusatz& zusatz() { return e().bearbeitung.zusatz; }   // g_modifiers
bool istModus(karte::AuswahlModus m) { return e().bearbeitung.einstellung.modus == m; }

// --- Selector_ ---
bool m1Waehlen(const FensterBeobachter& b) {
    const karte::Zusatz& z = zusatz();
    return !b.maus2 && (z.genau(true, false, false) || z.genau(false, true, false) ||
                        (z.genau(false, false, true) && istModus(karte::AuswahlModus::Komponente)));   // Primitive im Komponentenmodus
}
bool m2Waehlen(const FensterBeobachter& b) {
    const karte::Zusatz& z = zusatz();
    return b.maus2 && (z.genau(true, false, false) || z.genau(false, true, false));
}
bool flaecheZusatz() { return zusatz().genau(false, true, false); }   // g_modifiers == c_modifier_face
bool altZusatz() { return zusatz().genau(false, false, true); }       // g_modifiers == c_modifierAlt
AuswahlArt artFuerBewegt(const FensterBeobachter& b) { return b.mausBewegt ? AuswahlArt::Ersetzen : AuswahlArt::Durchlaufen; }
AuswahlArt artFuerZustand(const FensterBeobachter& b) { return m2Waehlen(b) ? artFuerBewegt(b) : AuswahlArt::Manipulator; }

karte::AuswahlRechteck bereich(const FensterBeobachter& b) {   // getDeviceArea
    const GeraeteVektor d = b.aktuell - b.start;
    if (b.mausBewegtGedrueckt && m2Waehlen(b) && d.x() != 0 && d.y() != 0) { return karte::auswahlKastenFlaeche(b.start, d); }
    return karte::AuswahlRechteck();
}

void punktWaehlen(int f, const GeraeteVektor& p, AuswahlArt art, bool flaeche) {   // SelectPoint
    Editor& ed = e();
    karte::punktWaehlen(ed.karte, ed.bearbeitung.einstellung, blick(f), p, epsilon(beob(f)), art, flaeche, altZusatz());
    auswahlGeaendert();
}

void m2Testen(int f, const GeraeteVektor& pos) {   // m2testSelect
    FensterBeobachter& b = beob(f);
    const AuswahlArt art = artFuerZustand(b);
    if (art != AuswahlArt::Manipulator) {
        const GeraeteVektor d = pos - b.start;
        if (b.mausBewegtGedrueckt) {
            if (d.x() != 0 && d.y() != 0) {
                Editor& ed = e();
                karte::bereichWaehlen(ed.karte, ed.bearbeitung.einstellung, blick(f), karte::auswahlKastenFlaeche(b.start, d), flaecheZusatz());
                auswahlGeaendert();
            }
        } else {
            punktWaehlen(f, pos, art, flaecheZusatz());
        }
    }
    b.start = b.aktuell = GeraeteVektor(0, 0);
    b.rechteck = bereich(b);   // draw_area
}

void auswahlDruck(int f, const GeraeteVektor& pos) {   // Selector_::mouseDown
    FensterBeobachter& b = beob(f);
    b.start = b.aktuell = karte::geraetBegrenzt(pos);
    b.malArt = AuswahlArt::Auswaehlen;
    if (m1Waehlen(b)) {
        Editor& ed = e();
        b.malArt = karte::malenBeginnen(ed.karte, ed.bearbeitung.einstellung, blick(f), pos, epsilon(b), flaecheZusatz(), altZusatz());
        auswahlGeaendert();
    }
}

void auswahlBewegung(int f, const GeraeteVektor& pos) {   // Selector_::mouseMoved
    FensterBeobachter& b = beob(f);
    b.aktuell = karte::geraetBegrenzt(pos);
    if (b.maus2) {
        b.rechteck = bereich(b);
    } else if (m1Waehlen(b)) {
        punktWaehlen(f, b.aktuell, b.malArt, flaecheZusatz());
    }
}

void auswahlLoslassen(int f, const GeraeteVektor& pos) {   // Selector_::mouseUp
    FensterBeobachter& b = beob(f);
    if (b.maus2) {
        m2Testen(f, karte::geraetBegrenzt(pos));
    } else {
        b.start = b.aktuell = GeraeteVektor(0, 0);
    }
}

// --- TexManipulator_ (selection.cpp:2434-2479) ---
karte::Blick texturBlick(int f, const GeraeteVektor& pos) {   // View scissored( *m_view ) + ConstructSelectionTest
    karte::Blick geschoren = blick(f);
    karte::auswahlTestBauen(geschoren, karte::auswahlKastenPunkt(pos, epsilon(beob(f))));
    return geschoren;
}

void texturDruck(int f, const GeraeteVektor& pos) {   // TexManipulator_::mouseDown
    Editor& ed = e();
    FensterBeobachter& b = beob(f);
    const karte::Blick geschoren = texturBlick(f, pos);
    karte::AuswahlVolumen test(geschoren);
    if (zusatz().keine()) {   // c_modifier_copy_texture: Scene_copyClosestTexture
        std::string shader;
        if (karte::naechsteTexturHolen(ed.karte, test, ed.texBro.shader, shader, ed.texturAblage)) { texturShaderWaehlen(shader); }
    } else {
        b.texRueckgaengig = true;
        ed.karte.undo.start();
        karte::naechsteTexturAuftragen(ed.karte, ed.bearbeitung.einstellung, test, zusatz().shift, zusatz().strg, zusatz().alt, true,
                                       ed.texBro.shader, ed.texturAblage);
    }
}

void texturBewegung(int f, const GeraeteVektor& pos) {   // TexManipulator_::mouseMoved
    Editor& ed = e();
    if (!beob(f).texRueckgaengig) { return; }
    const karte::Blick geschoren = texturBlick(f, karte::geraetBegrenzt(pos));   // device_constrained
    karte::AuswahlVolumen test(geschoren);
    karte::naechsteTexturAuftragen(ed.karte, ed.bearbeitung.einstellung, test, zusatz().shift, zusatz().strg, zusatz().alt, false,
                                   ed.texBro.shader, ed.texturAblage);
}

void texturLoslassen(int f) {   // TexManipulator_::mouseUp
    Editor& ed = e();
    FensterBeobachter& b = beob(f);
    if (b.texRueckgaengig) {
        ed.karte.undo.finish(karte::texturAuftragenName(zusatz().shift, zusatz().strg, zusatz().alt));
        b.texRueckgaengig = false;
    }
}

}  // namespace

// --- RadiantWindowObserver -----------------------------------------------------------

void beobachterGroesse(int fenster, int breite, int hoehe) {
    FensterBeobachter& b = beob(fenster);
    b.breite = std::max(1, breite);
    b.hoehe = std::max(1, hoehe);
}

void beobachterDruck(int fenster, float x, float y, int knopf, const karte::Zusatz& z) {   // onMouseDown
    FensterBeobachter& b = beob(fenster);
    if (b.mausUnten) { return; }   // keine gleichzeitigen Druecke
    Editor& ed = e();
    const GeraeteVektor pos = geraet(b, x, y);
    if (knopf == 0 || (knopf == 1 && !z.keine())) {
        b.mausUnten = true;
        // ClipManipulator::quickCondition (Strg+M1 in 2D: Clipper) - der
        // Clipper kommt mit Stufe 2; bis dahin geschieht hier nichts.
        const bool clipper2d = knopf == 0 && z.genau(false, true, false) && !blick(fenster).fuellen();
        if (clipper2d && ed.bearbeitung.modus() != ManipModus::Clipper) {
            b.ziel = FensterBeobachter::Ziel::Keins;
        } else if (knopf == 0 && ed.bearbeitung.manipulatorWaehlen(blick(fenster), pos, manipEpsilon(b))) {   // Manipulator_::mouseDown
            b.ziel = FensterBeobachter::Ziel::Manipulator;
        } else {
            b.maus2 = knopf == 1;
            auswahlDruck(fenster, pos);
            b.ziel = FensterBeobachter::Ziel::Auswahl;
        }
    } else if (knopf == 2) {   // c_button_texture: TexManipulator_
        b.mausUnten = true;
        texturDruck(fenster, pos);
        b.ziel = FensterBeobachter::Ziel::Textur;
    }
    b.bewegStart = pos;
    b.bewegtGedrueckt = 0;
}

void beobachterBewegung(int fenster, float x, float y) {   // onMouseMotion
    FensterBeobachter& b = beob(fenster);
    Editor& ed = e();
    b.mausBewegt = bewegtEps(b, x, y, b.bewegEnde, b.bewegt);
    const GeraeteVektor pos = geraet(b, x, y);
    if (b.mausUnten && b.ziel != FensterBeobachter::Ziel::Keins) {
        b.manipBewegtGedrueckt = b.mausBewegtGedrueckt = bewegtEps(b, x, y, b.bewegStart, b.bewegtGedrueckt);
        if (b.ziel == FensterBeobachter::Ziel::Manipulator) {
            if (b.manipBewegtGedrueckt) { ed.bearbeitung.auswahlBewegen(blick(fenster), pos); }   // Manipulator_::mouseMoved
        } else if (b.ziel == FensterBeobachter::Ziel::Textur) {
            texturBewegung(fenster, pos);
        } else {
            auswahlBewegung(fenster, pos);
        }
    } else {
        ed.bearbeitung.manipulatorHervorheben(blick(fenster), pos, manipEpsilon(b));   // Manipulator_::highlight
    }
    ed.letzterBeobachter = fenster;
    ed.letzteX = x;
    ed.letzteY = y;
}

void beobachterLoslassen(int fenster, float x, float y, int knopf, const karte::Zusatz& z) {   // onMouseUp
    FensterBeobachter& b = beob(fenster);
    Editor& ed = e();
    const GeraeteVektor pos = geraet(b, x, y);
    if (knopf >= 0 && b.ziel != FensterBeobachter::Ziel::Keins) {
        if (b.ziel == FensterBeobachter::Ziel::Manipulator) {
            b.ursprungBewegt = ed.bearbeitung.bewegenBeenden();   // Manipulator_::mouseUp
        } else if (b.ziel == FensterBeobachter::Ziel::Textur) {
            texturLoslassen(fenster);
        } else {
            auswahlLoslassen(fenster, pos);
        }
        b.ziel = FensterBeobachter::Ziel::Keins;
    }
    // M1 ohne Bewegung: Tunnel-Auswahl
    if (knopf == 0 && z.keine() && !b.mausBewegtGedrueckt && !b.ursprungBewegt &&
        !(istModus(karte::AuswahlModus::Komponente) && ed.bearbeitung.modus() == ManipModus::Ziehen) &&
        ed.bearbeitung.modus() != ManipModus::Clipper && ed.bearbeitung.modus() != ManipModus::Bauen) {
        punktWaehlen(fenster, karte::geraetBegrenzt(pos), artFuerBewegt(b), false);   // testSelect_simpleM1
    }
    b.mausUnten = false;   // die Sperre immer loesen
    b.ursprungBewegt = false;
    b.mausBewegt = false;
    b.mausBewegtGedrueckt = false;
    b.manipBewegtGedrueckt = false;
    b.bewegEnde = pos;
    b.bewegt = 0;
}

void zusatztastenPruefen() {   // onModifierDown / onModifierUp
    Editor& ed = e();
    const ImGuiIO& io = ImGui::GetIO();
    karte::Zusatz z;
    z.shift = io.KeyShift;
    z.strg = io.KeyCtrl;
    z.alt = io.KeyAlt;
    if (z.genau(ed.bearbeitung.zusatz.shift, ed.bearbeitung.zusatz.strg, ed.bearbeitung.zusatz.alt)) { return; }
    ed.bearbeitung.zusatz = z;
    if (ed.letzterBeobachter >= 0) { beobachterBewegung(ed.letzterBeobachter, ed.letzteX, ed.letzteY); }
}

karte::ManipZeichnung manipulatorZeichnung(int fenster) {
    karte::ManipZeichnung z;
    e().bearbeitung.zeichnen(blick(fenster), z);
    return z;
}

KomponentenZeichnung komponentenZeichnung(const karte::Blick& blick) {
    const Editor& ed = e();
    const karte::Karte& k = ed.karte;
    KomponentenZeichnung z;
    const karte::Farbe gruen = karte::farbeAus(0, 1, 0);                                // colour_vertex
    const karte::Farbe blau = karte::farbeAus(0, 0, 1);                                 // colour_selected
    const karte::Farbe magenta = karte::farbeAus(1, 0, 1);                              // colour_inside
    const karte::Farbe orange = karte::farbeAus(1, 0.5F, 0);                            // $LATTICE
    const auto punkt = [](std::vector<karte::Linie>& v, const karte::Vektor3f& p, karte::Farbe f) { v.push_back(karte::Linie{p, p, f}); };
    const karte::AuswahlEinstellung& einst = ed.bearbeitung.einstellung;
    const bool komponentenModus = einst.modus == karte::AuswahlModus::Komponente;
    const auto pinselZeichnen = [&](const karte::Pinsel& pinsel, bool gewaehltesPrimitiv) {
        pinsel.bRep(k.kontext);
        // renderComponentsSelected: FaceInstance::iterate_selected der beitragenden Flaechen
        for (const auto& f : pinsel.flaechen()) {
            if (!f->traegtBei()) { continue; }
            const karte::Wicklung& w = f->wicklung();
            for (const std::size_t nachbar : f->eckenWahl) {
                const std::size_t i = karte::wicklungFindeNachbar(w, nachbar);
                if (i != karte::kMaxFlaechen) { punkt(z.gewaehlt, karte::Vektor3f(w[i].vertex), blau); }
            }
            for (const std::size_t nachbar : f->kantenWahl) {
                const std::size_t i = karte::wicklungFindeNachbar(w, nachbar);
                if (i != karte::kMaxFlaechen) { punkt(z.gewaehlt, karte::Vektor3f(karte::mitte(w[i].vertex, w[karte::wicklungNaechster(w, i)].vertex)), blau); }
            }
            if (k.auswahl.flaecheGewaehlt(f.get())) { punkt(z.gewaehlt, f->schwerpunkt(), blau); }
        }
        if (!komponentenModus || !gewaehltesPrimitiv) { return; }   // renderCommon: componentMode && isSelected()
        if (blick.fuellen() && einst.komponente == karte::KomponentenModus::Flaeche) {   // m_render_faces_wireframe
            for (const auto& f : pinsel.flaechen()) {
                if (f->traegtBei() && karte::abstandZuPunkt(f->ebene(), blick.betrachter()) > 0) { punkt(z.punkte, f->schwerpunkt(), gruen); }
            }
            return;
        }
        switch (einst.komponente) {
        case karte::KomponentenModus::Ecke:
            for (const karte::FlaechenPunkt& fp : pinsel.ecken(k.kontext)) {
                punkt(blick.fuellen() ? z.tiefPunkte : z.punkte, karte::Vektor3f(pinsel.eckpunkt(fp)), gruen);
            }
            break;
        case karte::KomponentenModus::Kante:
            for (const karte::FlaechenPunkt& fp : pinsel.kanten(k.kontext)) {
                const karte::Wicklung& w = pinsel[fp.flaeche].wicklung();
                punkt(z.punkte, karte::Vektor3f(karte::mitte(w[fp.punkt].vertex, w[karte::wicklungNaechster(w, fp.punkt)].vertex)), gruen);
            }
            break;
        case karte::KomponentenModus::Flaeche:   // m_render_faces (m_faceCentroidPoints)
            for (const auto& f : pinsel.flaechen()) { punkt(z.punkte, f->schwerpunkt(), gruen); }
            break;
        default: break;
        }
    };
    const auto patchZeichnen = [&](const karte::Patch& patch, bool gewaehltesPrimitiv) {
        const auto& pk = patch.punkte();
        for (std::size_t i = 0; i < pk.size(); ++i) {   // update_selected
            if (patch.punktGewaehlt(i)) { punkt(z.gewaehlt, pk[i].vertex, blau); }
        }
        if (!komponentenModus || !gewaehltesPrimitiv || einst.komponente != karte::KomponentenModus::Ecke) { return; }
        const std::size_t b = patch.breite();
        for (std::size_t i = 0; i < pk.size(); ++i) {
            // colour_for_index: Ecken der Teilfelder gruen, innere Punkte magenta
            punkt(z.punkte, pk[i].vertex, ((i % 2) != 0 || ((i / b) % 2) != 0) ? magenta : gruen);
            if ((i % b) != 0) { z.gitter.push_back(karte::Linie{pk[i - 1].vertex, pk[i].vertex, orange}); }
            if (i >= b) { z.gitter.push_back(karte::Linie{pk[i - b].vertex, pk[i].vertex, orange}); }
        }
    };
    std::set<const karte::Primitiv*> gezeichnet;
    for (const auto& g : karte::gewaehltePrimitiveListe(k)) {
        if (g.primitiv->versteckt || g.entitaet->versteckt) { continue; }
        gezeichnet.insert(g.primitiv.get());
        if (g.primitiv->art() == karte::Primitiv::Art::Pinsel) {
            pinselZeichnen(static_cast<const karte::Pinsel&>(*g.primitiv), true);
        } else {
            patchZeichnen(static_cast<const karte::Patch&>(*g.primitiv), true);
        }
    }
    // gewaehlte Flaechen ungewaehlter Pinsel (Strg+Umschalt+M1)
    for (const auto& fw : k.auswahl.flaechen()) {
        const auto p = fw.pinsel.lock();
        const auto f = fw.flaeche.lock();
        if (!p || !f || p->versteckt || gezeichnet.count(p.get()) != 0) { continue; }
        p->bRep(k.kontext);
        if (f->traegtBei()) { punkt(z.gewaehlt, f->schwerpunkt(), blau); }
    }
    return z;
}

void auswahlGeaendert() {
    Editor& ed = e();
    ed.bearbeitung.manipulatorAbwaehlen();   // setSelectedAll: m_manipulator->setSelected( false )
    // ComponentMode_SelectionChanged
    if (istModus(karte::AuswahlModus::Komponente) && ed.karte.auswahl.anzahl() == 0) {
        ed.bearbeitung.einstellung.modus = karte::AuswahlModus::Primitiv;   // SelectionSystem_DefaultMode
        ed.bearbeitung.einstellung.komponente = karte::KomponentenModus::Standard;
    }
}

// --- tools.cpp ----------------------------------------------------------------------------

namespace {

const char* statusText(ManipModus m) {
    switch (m) {
    case ManipModus::Ziehen: return "QE4 Drag Tool: move and resize objects";            // c_ResizeMode_status
    case ManipModus::Verschieben: return "Translate Tool: translate objects and components";
    case ManipModus::Drehen: return "Rotate Tool: rotate objects and components";
    case ManipModus::Skalieren: return "Scale Tool: scale objects and components";
    case ManipModus::Verzerren: return "Transform Tool: transform objects and components";
    case ManipModus::Clipper: return "Clipper Tool: apply clip planes to brushes";
    case ManipModus::Bauen: return "Build Tool: extrude, build chains, clone";
    case ManipModus::UV: return "UV Tool: edit texture alignment";
    }
    return "";
}

void standardModus() {   // SelectionSystem_DefaultMode (tools.cpp:38-42)
    Editor& ed = e();
    ed.bearbeitung.einstellung.modus = karte::AuswahlModus::Primitiv;
    ed.bearbeitung.einstellung.komponente = karte::KomponentenModus::Standard;
}

// g_currentToolModeSupportsComponentEditing (tools.cpp:250-395)
bool werkzeugMitKomponenten(ManipModus m) { return m != ManipModus::Clipper && m != ManipModus::Bauen && m != ManipModus::UV; }

// DragMode, TranslateMode, ...: dasselbe Werkzeug noch einmal = das Standardwerkzeug
void werkzeug(ManipModus m) {
    Editor& ed = e();
    constexpr ManipModus kStandard = ManipModus::Ziehen;   // g_defaultToolMode
    if (ed.werkzeug == m && m != kStandard) {
        werkzeug(kStandard);
        return;
    }
    ed.werkzeug = m;
    if (!werkzeugMitKomponenten(m)) { standardModus(); }   // ClipperMode, BuildMode, UVMode
    ed.statusBefehl = statusText(m);   // Sys_Status
    ed.bearbeitung.setzeModus(m);
}

// SelectVertexMode / SelectEdgeMode / SelectFaceMode (tools.cpp:106-162)
void komponentenModus(karte::KomponentenModus m) {
    Editor& ed = e();
    karte::AuswahlEinstellung& einst = ed.bearbeitung.einstellung;
    if (einst.modus == karte::AuswahlModus::Komponente && einst.komponente == m) {
        standardModus();
    } else if (ed.karte.auswahl.anzahl() != 0) {
        if (!werkzeugMitKomponenten(ed.werkzeug)) { werkzeug(ManipModus::Ziehen); }   // g_defaultToolMode()
        einst.modus = karte::AuswahlModus::Komponente;
        einst.komponente = m;
    }
    ed.bearbeitung.pivotGeaendert();   // SetMode: pivotChanged
}

// GlobalXYWnd_getCurrentViewType
karte::Sicht aktuelleSicht() { return e().xy[static_cast<std::size_t>(e().aktivXy)].ansicht.typ; }

void schubsen(int richtung, const char* name) {   // Selection_Nudge*
    Editor& ed = e();
    ed.karte.undo.start();
    ed.bearbeitung.verschiebeAuswahl(karte::schubsRichtung(richtung, ed.raster.groesse(), aktuelleSicht()));   // NudgeManipulator
    ed.karte.undo.finish(name);
}

// TranslateToCamera
void zurKamera() {
    Editor& ed = e();
    const karte::Kasten b = karte::auswahlGrenzenPivot(ed.karte);
    ed.bearbeitung.verschiebeAuswahl(karte::eingerastet(ed.kamera.ursprung - b.origin, ed.raster.einrastGroesse()));
}

}  // namespace

bool auswahlBefehlAn(const std::string& name) {
    const Editor& ed = e();
    if (name == "MouseDrag") { return ed.bearbeitung.modus() == ManipModus::Ziehen; }
    if (name == "MouseTranslate") { return ed.bearbeitung.modus() == ManipModus::Verschieben; }
    if (name == "MouseRotate") { return ed.bearbeitung.modus() == ManipModus::Drehen; }
    if (name == "MouseScale") { return ed.bearbeitung.modus() == ManipModus::Skalieren; }
    if (name == "MouseTransform") { return ed.bearbeitung.modus() == ManipModus::Verzerren; }
    if (name == "MouseBuild") { return ed.bearbeitung.modus() == ManipModus::Bauen; }   // BuildToolExport
    if (name == "MouseUV") { return ed.bearbeitung.modus() == ManipModus::UV; }         // UVToolExport
    if (name == "HideSelected") { return ed.knotenVersteckt; }   // g_hidden_item
    const karte::AuswahlEinstellung& einst = ed.bearbeitung.einstellung;
    const bool komp = einst.modus == karte::AuswahlModus::Komponente;
    if (name == "DragVertices") { return komp && einst.komponente == karte::KomponentenModus::Ecke; }   // VertexMode
    if (name == "DragEdges") { return komp && einst.komponente == karte::KomponentenModus::Kante; }     // EdgeMode
    if (name == "DragFaces") { return komp && einst.komponente == karte::KomponentenModus::Flaeche; }   // FaceMode
    if (name == "TogTexLock") { return ed.karte.kontext.texturLock; }                 // g_brush_texturelock_enabled
    if (name == "TogTexVertexLock") { return ed.bearbeitung.texturEckenSperre; }      // g_brush_textureVertexlock_enabled
    return false;
}

void auswahlBefehl(const std::string& name) {
    Editor& ed = e();
    karte::Karte& k = ed.karte;
    karte::AuswahlEinstellung& einst = ed.bearbeitung.einstellung;
    if (name == "MouseDrag") {
        werkzeug(ManipModus::Ziehen);
    } else if (name == "MouseTranslate") {
        werkzeug(ManipModus::Verschieben);
    } else if (name == "MouseRotate") {
        werkzeug(ManipModus::Drehen);
    } else if (name == "MouseScale") {
        werkzeug(ManipModus::Skalieren);
    } else if (name == "MouseTransform") {
        werkzeug(ManipModus::Verzerren);
    } else if (name == "MouseBuild") {   // BuildMode (tools.cpp:372-388)
        werkzeug(ManipModus::Bauen);
    } else if (name == "MouseUV") {   // UVMode (tools.cpp:390-406)
        werkzeug(ManipModus::UV);
    } else if (name == "DragVertices") {
        komponentenModus(karte::KomponentenModus::Ecke);
    } else if (name == "DragEdges") {
        komponentenModus(karte::KomponentenModus::Kante);
    } else if (name == "DragFaces") {
        komponentenModus(karte::KomponentenModus::Flaeche);
    } else if (name == "TogTexLock") {   // Texdef_ToggleMoveLock (brushmanip.cpp:1460-1464)
        k.kontext.texturLock = !k.kontext.texturLock;
    } else if (name == "TogTexVertexLock") {   // Texdef_ToggleMoveVLock (brushmanip.cpp:1467-1470)
        ed.bearbeitung.texturEckenSperre = !ed.bearbeitung.texturEckenSperre;
    } else if (name == "MouseRotateOrScale") {   // ToggleRotateScaleModes
        werkzeug(ed.werkzeug == ManipModus::Drehen ? ManipModus::Skalieren : ManipModus::Drehen);
    } else if (name == "MouseDragOrTransform") {   // ToggleDragSkewModes
        werkzeug(ed.werkzeug == ManipModus::Ziehen ? ManipModus::Verzerren : ManipModus::Ziehen);
    } else if (name == "Undo") {
        k.undo.undo();
        ed.bearbeitung.pivotGeaendert();
    } else if (name == "Redo" || name == "Redo2") {
        k.undo.redo();
        ed.bearbeitung.pivotGeaendert();
    } else if (name == "Copy") {
        ed.zwischenablage = karte::kopieren(k);
    } else if (name == "Paste") {
        karte::einfuegen(k, ed.zwischenablage);
        auswahlGeaendert();
    } else if (name == "PasteToCamera") {   // setSelectedAll( false ), "pasteToCamera", Selection_Paste, TranslateToCamera
        k.auswahl.leeren();
        auswahlGeaendert();
        k.undo.start();
        karte::einfuegen(k, ed.zwischenablage, false);
        zurKamera();
        k.undo.finish("pasteToCamera");
    } else if (name == "MoveToCamera") {
        k.undo.start();
        zurKamera();
        k.undo.finish("moveToCamera");
    } else if (name == "CloneSelection" || name == "CloneSelectionAndMakeUnique") {
        const bool eindeutig = name == "CloneSelectionAndMakeUnique";
        if (einst.modus == karte::AuswahlModus::Primitiv) {
            k.undo.start();
            karte::klonen(k, einst, eindeutig, false);
            if (ed.nachKlonenSchubsen) {   // g_bNudgeAfterClone
                ed.bearbeitung.verschiebeAuswahl(karte::schubsRichtung(2, ed.raster.groesse(), aktuelleSicht()));
                ed.bearbeitung.verschiebeAuswahl(karte::schubsRichtung(3, ed.raster.groesse(), aktuelleSicht()));
            }
            k.undo.finish(eindeutig ? "cloneSelectedMakeUnique" : "cloneSelected");
            auswahlGeaendert();
        }
    } else if (name == "DeleteSelection" || name == "DeleteSelection2" || name == "DeleteSelection3") {
        karte::loescheAuswahl(k, einst, ed.bearbeitung.texturEckenSperre);
        auswahlGeaendert();
    } else if (name == "UnSelectSelection" || name == "UnSelectSelection2") {
        karte::auswahlAufheben(k, einst);
        auswahlGeaendert();
    } else if (name == "InvertSelection") {
        karte::auswahlUmkehren(k, einst);
        auswahlGeaendert();
    } else if (name == "SelectAllOfType") {
        karte::alleVomTyp(k, einst, ed.bearbeitung.browserShader);
        auswahlGeaendert();
    } else if (name == "SnapToGrid") {
        karte::amRasterAusrichten(k, einst, ed.raster.groesse(), ed.bearbeitung.texturEckenSperre);
    } else if (name == "SelectNudgeLeft") {
        schubsen(0, "nudgeSelectedLeft");
    } else if (name == "SelectNudgeUp") {
        schubsen(1, "nudgeSelectedUp");
    } else if (name == "SelectNudgeRight") {
        schubsen(2, "nudgeSelectedRight");
    } else if (name == "SelectNudgeDown") {
        schubsen(3, "nudgeSelectedDown");
    } else if (name == "MoveSelectionUP" || name == "MoveSelectionDOWN") {   // Selection_NudgeZ
        const float menge = name == "MoveSelectionUP" ? ed.raster.groesse() : -ed.raster.groesse();
        k.undo.start();
        ed.bearbeitung.verschiebeAuswahl(karte::Vektor3f(0, 0, menge));
        k.undo.finish("nudgeSelected -axis z -amount " + karte::zahlText(menge));
    } else if (name == "HideSelected") {
        ed.knotenVersteckt = karte::versteckeGewaehlte(k, ed.knotenVersteckt);
        ++ed.sichtStand;
        auswahlGeaendert();
    } else if (name == "ShowHidden") {
        karte::zeigeVersteckte(k);
        ed.knotenVersteckt = false;
        ++ed.sichtStand;
    }
}

}  // namespace bhed::gui::karteneditor
