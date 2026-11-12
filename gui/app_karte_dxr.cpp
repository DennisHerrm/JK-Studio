// app_karte_dxr.cpp - Karten-Editor: die Schattenart "Raytracing-Schatten (DXR)" des Echtzeit-Lichts in der Oberflaeche.
//
// Der Menuepunkt steht in Build bei den Schatten-Einstellungen des Echtzeit-Lichts (nach "Patch shadows"). Vorgabe aus:
// Schattenkarten wie bisher. Geht es nicht (Direct3D 11, Grafikkarte ohne DXR 1.1, Bau ohne dxc, Fehler), ist er ausgegraut
// und der Tooltip nennt den Grund; die Einstellung bleibt gemerkt und gilt wieder, sobald es geht. Das Rechnen selbst:
// gui/karte_dxr.h (Strukturen), gui/karte_dxr.hlsl (Strahlen), app_karte_echtlicht.cpp (Wahl je Bild).
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#include "karte_dxr.h"
#include "karte_echtlicht.h"
#include "karteneditor.h"

#include "bhed/i18n.h"

#include <cstdlib>
#include <string>

namespace bhed::gui::karteneditor {

using i18n::Str;

bool strahlenGewaehlt() {
    static const int lauf = [] {
        const char* w = std::getenv("BHED_DXR");
        return w == nullptr || w[0] == '\0' ? -1 : (w[0] == '0' ? 0 : 1);
    }();
    return lauf >= 0 ? lauf == 1 : editor().echtLicht.schattenStrahlen;
}

std::string strahlenGrundText() {
    switch (kartegpu::dxr::grund()) {
        case kartegpu::dxr::Grund::Keiner: break;
        case kartegpu::dxr::Grund::NichtD3d12: return trK(Str::KeStrahlenGrundD3d12);
        case kartegpu::dxr::Grund::KeinDxr: return trK(Str::KeStrahlenGrundGeraet);
        case kartegpu::dxr::Grund::KeinShader: return trK(Str::KeStrahlenGrundShader);
        case kartegpu::dxr::Grund::Fehler: return trK(Str::KeStrahlenGrundFehler);
    }
    return std::string();
}

StrahlenStand strahlenStand() {
    const kartegpu::dxr::Stand k = kartegpu::dxr::stand();
    StrahlenStand s;
    s.grund = static_cast<int>(kartegpu::dxr::grund());
    s.teile = k.teile;
    s.dreiecke = k.dreiecke;
    s.filterTeile = k.filterTeile;
    s.gebaut = k.gebaut;
    s.schnellGebaut = k.schnellGebaut;
    s.nachgezogen = k.nachgezogen;
    s.szenen = k.szenen;
    s.bytes = k.bytes;
    s.strukturBytes = k.strukturBytes;
    s.gebunden = k.gebunden;
    return s;
}

void strahlenMenuePunkt() {
    Editor& ed = editor();
    const std::string id = std::string(trK(Str::KeStrahlenMenue)) + "##bauschattenstrahlen";
    const std::string grund = strahlenGrundText();
    QtPunkt p;
    p.id = id.c_str();
    p.art = QtArt::Pruefbar;
    p.aktiv = grund.empty();
    p.haken = strahlenGewaehlt() && grund.empty();
    p.tooltip = trK(Str::KeStrahlenTip);
    if (!grund.empty()) { p.tooltip += "\n\n" + grund; }
    if (qtPunkt(p)) {
        ed.echtLicht.schattenStrahlen = !ed.echtLicht.schattenStrahlen;
        einstellungenSpeichern();
        neuZeichnen();
    }
}

}  // namespace bhed::gui::karteneditor
