// portale.h - Karten-Editor: der Portal Viewer (prtview, Stufe 3 Punkt 5).
//
// q3map2 schreibt neben die .bsp eine Portaldatei (.prt, "PRT1"): die Zahl der
// Cluster, der Portale und der Flaechen, dann je Portal "<Punkte> <Cluster>
// <Cluster> <Hint> (x y z) ...". Das Plugin prtview liest sie und zeigt die
// Portale in den 2D-Ansichten (Linien) und in der Kamera (Polygone in
// zufaelliger Farbe, Umrisse, Nebel, Wuerfel-Ausschnitt). Hier der Teil ohne
// Oberflaeche: das Lesen (CPortals::Load, CBspPortal::Build), die
// Einstellungen in prtview.ini (LoadConfig/SaveConfig mit IniFile aus
// libs/profile/profile2.h) und FixColors.
//
// Portiert aus NetRadiant-Custom 20260114: contrib/prtview/portals.cpp,
// portals.h, prtview.cpp; libs/profile/profile2.h.
//
//   PrtView plugin for GtkRadiant
//   Copyright (C) 2001 Geoffrey Dewan, Loki software and qeradiant.com
//
//   Urspruenglich unter der GNU Lesser General Public License 2.1 (oder
//   spaeter); nach deren Abschnitt 3 hier unter der GNU General Public
//   License, Version 2:
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the
//   Free Software Foundation; version 2 of the License. This program is
//   distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
//   without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//   PARTICULAR PURPOSE. See the GNU General Public License for more details
//   (LICENSE).
//
//   IniFile: Copyright (c) 2001, Loki software, inc. All rights reserved
//   (BSD-Lizenz, siehe libs/profile/profile2.h in NetRadiant-Custom).
#ifndef BHED_KARTE_PORTALE_H
#define BHED_KARTE_PORTALE_H

#include "bhed/karte/mathe.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace bhed::karte {

// IniFile (profile2.h): Abschnitte und Schluessel sortiert (std::map),
// Kommentare mit ';' (ganze Zeile oder nach einem Leerraum), Leerraum vorn
// und hinten weg.
class IniDatei {
public:
    void lesen(const std::string& text);
    [[nodiscard]] std::string schreiben() const;   // "[Abschnitt]\n" und "Schluessel=Wert\n"
    void setzen(const std::string& abschnitt, const std::string& schluessel, const std::string& wert);
    [[nodiscard]] std::optional<std::string> wert(const std::string& abschnitt, const std::string& schluessel) const;

private:
    std::map<std::string, std::map<std::string, std::string>> m_abschnitte;
};

// RGB_PACK / RGB_UNPACK_* (portals.h)
using GepackteFarbe = std::uint32_t;
[[nodiscard]] constexpr GepackteFarbe rgbPacken(int r, int g, int b) {
    return static_cast<std::uint32_t>(static_cast<std::uint8_t>(r)) | (static_cast<std::uint32_t>(static_cast<std::uint8_t>(g)) << 8) |
           (static_cast<std::uint32_t>(static_cast<std::uint8_t>(b)) << 16);
}

// Die Einstellungen von CPortals (mit den Vorgaben aus LoadConfig).
struct PortalEinstellungen {
    bool zeige2d = false;                         // show_2d ("Render2D")
    int breite2d = 3;                             // width_2d ("Width2D", 1..10)
    GepackteFarbe farbe2d = rgbPacken(0, 0, 255);   // color_2d ("Color2D")
    bool hints = true;                            // draw_hints ("DrawHints")
    bool nichtHints = true;                       // draw_nonhints ("DrawNonHints")
    bool zeige3d = true;                          // show_3d ("Render3D")
    int zPuffer = 1;                              // zbuffer ("ZBuffer": 0 Test und Schreiben, 1 nur Test, 2 aus)
    bool nebel = false;                           // fog ("Fog")
    bool polygone = true;                         // polygons ("Polygons")
    bool linien = true;                           // lines ("Lines")
    int breite3d = 3;                             // width_3d ("Width3D", 1..10)
    GepackteFarbe farbe3d = rgbPacken(255, 255, 0);         // color_3d ("Color3D")
    GepackteFarbe farbeNebel = rgbPacken(127, 127, 127);    // color_fog ("ColorFog")
    int deckkraft = 50;                           // opacity_3d ("Opacity", 0..100)
    bool ausschnitt = false;                      // clip ("Clip")
    int ausschnittWeite = 1024;                   // clip_range ("ClipRange", 64..8192)
    // FixColors: die Farben als float (A = 1)
    float fp2d[4]{0, 0, 1, 1};
    float fp3d[4]{1, 1, 0, 1};
    float fpNebel[4]{127.0F / 255.0F, 127.0F / 255.0F, 127.0F / 255.0F, 1};
    void farbenSetzen();   // CPortals::FixColors
};

// LoadConfig (prtview.cpp): Werte aus der Ini (Abschnitt "Configuration"),
// sonst die Vorgaben; Breiten, Deckkraft und Weite begrenzt, ZBuffer 0..2.
[[nodiscard]] PortalEinstellungen portalEinstellungenAus(const IniDatei& ini);
// SaveConfig: je Wert "<Zahl>        ; <Kommentar>".
[[nodiscard]] IniDatei portalEinstellungenIni(const PortalEinstellungen& e);

// rand() der C-Laufzeit von Windows (msvcrt/ucrt: x = x * 214013 + 2531011,
// (x >> 16) & 0x7fff, Anfang 1) - eigene Folge, damit die Farben der Portale
// nicht von anderen rand()-Aufrufen des Programms abhaengen.
class PortalZufall {
public:
    int naechste();

private:
    std::uint32_t m_zustand = 1;
};

// CBspPortal
struct BspPortal {
    Vektor3f mitte{0, 0, 0};           // center
    std::vector<Vektor3f> punkte;      // point
    std::vector<Vektor3f> innen;       // inner_point (1 % zur Mitte)
    float farbe[4]{0, 0, 0, 1};        // fp_color_random
    Vektor3f min{0, 0, 0};
    Vektor3f max{0, 0, 0};
    float abstand = 0.0F;              // dist (Sortieren nach der Kamera)
    bool hint = false;
};

// Eine Meldung von CPortals::Load: art 0 globalOutputStream, 2 globalErrorStream.
struct PortalMeldung {
    int art = 0;
    std::string text;   // mit '\n'
};

// CPortals::Load: dateiName nur fuer die Meldung, inhalt = der Dateiinhalt
// (nullptr: "could not open file"). portale ist danach leer, wenn etwas nicht
// stimmte. hintFlags wie CPortals::hint_flags.
[[nodiscard]] std::vector<PortalMeldung> portaleLaden(const std::string& dateiName, const std::string* inhalt, std::vector<BspPortal>& portale,
                                                      bool& hintFlags, PortalZufall& zufall);

inline constexpr const char* kPortalMeldungVorn = "Portal Viewer plugin: ";   // MSG_PREFIX

}  // namespace bhed::karte

#endif
