// vorschau.h - Menue-Editor M1 Punkt 2: ein Menue zeichnen wie die Engine.
//
// Portiert aus OpenJK: codemp/ui/ui_shared.c (Menu_Paint, Item_Paint,
// Window_Paint, Item_SetTextExtents, ToWindowCoords, Item_TextColor,
// Item_Text_Paint, Item_Text_Wrapped_Paint, Item_Text_AutoWrapped_Paint,
// Item_TextField_Paint, Item_YesNo_Paint, Item_Multi_Paint,
// Item_Multi_Setting, Item_Slider_Paint, Item_Slider_ThumbPosition,
// Item_ListBox_Paint, Item_EnableShowViaCvar), codemp/ui/ui_main.c
// (Text_Paint, Text_Width, Text_Height, MenuFontToHandle) und
// code/rd-common/tr_font.cpp (dfontdat_t, CFontInfo, RE_Font_StrLenPixels,
// RE_Font_HeightPixels, RE_Font_DrawString).
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
//
// Das Ergebnis ist eine Liste von Zeichenbefehlen im virtuellen Bildschirm
// 640x480 (wie RE_StretchPic/RE_SetColor sie bekaemen). Die Oberflaeche
// bildet sie auf ihr Fenster ab (gestreckt wie das Original oder 4:3 mit
// Balken) und laedt die Shader aus den Spieldaten.
#ifndef BHED_MENUE_VORSCHAU_H
#define BHED_MENUE_VORSCHAU_H

#include "bhed/menue/menue.h"

#include <functional>
#include <string>
#include <vector>

namespace bhed::menue {

inline constexpr float kSchirmBreite = 640.0F;   // SCREEN_WIDTH
inline constexpr float kSchirmHoehe = 480.0F;    // SCREEN_HEIGHT
inline constexpr float kScrollbar = 16.0F;       // SCROLLBAR_SIZE
inline constexpr float kSliderBreite = 96.0F;    // SLIDER_WIDTH
inline constexpr float kSliderHoehe = 16.0F;     // SLIDER_HEIGHT
inline constexpr float kSliderDaumenBreite = 12.0F;
inline constexpr float kSliderDaumenHoehe = 20.0F;

// glyphInfo_t / dfontdat_t (tr_font.h)
struct Glyph {
    short width = 0, height = 0, horizAdvance = 0, horizOffset = 0;
    int baseline = 0;
    float s = 0, t = 0, s2 = 0, t2 = 0;
};
struct Schrift {
    std::string name;     // "ergoec"
    std::string shader;   // "fonts/ergoec"
    Glyph glyph[256];
    short pointSize = 0, height = 0, ascender = 0, descender = 0;
    bool ok = false;
};
// CFontInfo::CFontInfo: die Datei muss genau sizeof(dfontdat_t) = 7180 Bytes haben.
[[nodiscard]] bool ladeFontdat(const std::string& name, const std::string& daten, Schrift& out);
// RE_Font_StrLenPixels (JA): ^0-^9 zaehlen nicht, Zeilenumbruch beginnt neu, ceilf.
[[nodiscard]] int schriftBreite(const Schrift& f, const std::string& text, float scale);
// RE_Font_HeightPixels: mPointSize * scale
[[nodiscard]] int schriftHoehe(const Schrift& f, float scale);

enum class PrimArt {
    Fuellen,      // DC->fillRect
    Bild,         // DC->drawHandlePic / RE_StretchPic (shader, s/t)
    Platzhalter,  // ownerdraw, Feeder ohne Spiel: Rahmen mit Beschriftung
    Modell,       // ITEM_TYPE_MODEL: die 3D-Vorschau zeichnet die Oberflaeche
    Kino,         // WINDOW_STYLE_CINEMATIC (RoQ)
};

struct Prim {
    PrimArt art = PrimArt::Fuellen;
    float x = 0, y = 0, w = 0, h = 0;
    float s0 = 0, t0 = 0, s1 = 1, t1 = 1;
    Farbe farbe{1, 1, 1, 1};
    std::string shader;   // Bild/Kino: Shadername; Platzhalter: Beschriftung
    int item = -1;        // Index im Menue (-1 = das Menue selbst)
};

// Eine Zeile einer Listbox: DC->feederItemText je Spalte, DC->feederItemImage
// und die optionalen Bilder je Spalte (SP optionalImage, MP optionalImage1).
// RE_Font_DrawString (ohne die asiatischen Zweige): je Zeichen ein Bild
// (PrimArt::Bild, Shader der Schrift) im virtuellen Bildschirm, ab (ox, oy)
// oben links. ^0-^9 wechseln die Farbe (Deckung bleibt), ausser imSchatten
// (gbInShadow); maxPixel -1 = ohne Grenze. Auch fuer die Untertitel der
// Vorschau (bhed/untertitel.h).
void schriftZeichnen(const Schrift& f, int ox, int oy, const std::string& text, Farbe farbe, int maxPixel, float scale,
                     bool imSchatten, std::vector<Prim>& out);

struct FeederEintrag {
    std::vector<std::string> spalten;
    std::string bild;
    std::vector<std::string> spaltenBild;
};

// Laufzeitstand eines Items (die Skripte aendern ihn: show/hide/setfocus ...).
struct ItemZustand {
    int flags = 0;        // windowDef_t.flags
    bool gueltig = false; // false = die gelesenen Flags nehmen
};

struct Umgebung {
    const Texte* texte = nullptr;
    // DC->getCVarString; "" wenn unbekannt
    std::function<std::string(const std::string&)> cvar;
    // Schrift zu item->font (SP) bzw. iMenuFont (MP), schon nach Dialekt
    // aufgeloest; nullptr = keine Schrift (dann zeichnet die Engine nichts).
    std::function<const Schrift*(int font)> schrift;
    // DC->ownerDrawVisible; ohne: sichtbar
    std::function<bool(int flags)> ownerDrawSichtbar;
    // Feeder: die Zeilen einer Listbox (DC->feederCount/feederItemText/feederItemImage);
    // ohne: beschrifteter Platzhalter
    std::function<std::vector<FeederEintrag>(float feeder)> feeder;
    int listCursor = 0;                   // item->cursorPos
    int zeitMs = 0;                       // DC->realTime
    std::vector<ItemZustand> zustand;     // je Item (leer = gelesene Flags)
    bool erzwingen = true;                // forcePaint: auch unsichtbare Menues
    bool alleItems = false;               // Editor: auch unsichtbare Items (gedimmt)
    Dialekt dialekt = Dialekt::MP;
};

// Menu_Paint fuer ein Menue der Datei.
void zeichneMenu(const Datei& d, int menu, const Umgebung& u, std::vector<Prim>& out);

// Item_EnableShowViaCvar
[[nodiscard]] bool itemZeigtUeberCvar(const Item& it, int flag, const Umgebung& u);
// Ist das Item gerade sichtbar (Item_Paint: ownerDrawFlags, cvarTest, WINDOW_VISIBLE)?
[[nodiscard]] bool itemSichtbar(const Item& it, int flags, const Umgebung& u);

// Item_SetTextExtents: textRect (x, y, w, h) im Bildschirm fuer text.
[[nodiscard]] Rechteck textRechteck(const Item& it, const std::string& text, const Umgebung& u);
// Item_Slider_ThumbPosition
[[nodiscard]] float sliderDaumen(const Item& it, const Umgebung& u);

// Welche Schrift hinter einer Nummer steckt (Namen aus assetGlobalDef und
// den menuDef-Schriften der Datei), Index = item->font bzw. iMenuFont:
//   MP: MenuFontToHandle - 1 smallFont, 2 font, 3 bigFont, 4 small2Font, sonst font.
//   SP: wie UI_RegisterFont/RE_RegisterFont anmelden (fehlende Schrift belegt
//       einen Griff, dann "ergoec"); 0 = qhMediumFont. OpenJK (JA, JK2):
//       Nummer n = registeredFonts[n-1] (UI_MenuFontToReal); Movie Duels:
//       Nummer = Renderer-Griff. "" = zeichnet nichts.
// gibt: gibt es fonts/<name>.fontdat? (leer = alle vorhanden, keine _sharp-Varianten)
using GibtSchrift = std::function<bool(const std::string& name)>;
[[nodiscard]] std::vector<std::string> schriftNamen(const Datei& assets, Dialekt d, Spiel spiel = Spiel::JA,
                                                    const GibtSchrift& gibt = {});
// Dasselbe ueber alle Dateien eines Menuesatzes in Ladereihenfolge (UI_LoadMenus).
[[nodiscard]] std::vector<std::string> schriftNamen(const std::vector<const Datei*>& satz, Dialekt d, Spiel spiel,
                                                    const GibtSchrift& gibt);

// Item_Multi_Setting: der Text einer Mehrfachwahl zum Cvar-Wert (mit cvarStrList feeder
// wie ItemParse_cvarStrList: FEEDER_PLAYER_SPECIES -> "@MENUS_<NAME>").
[[nodiscard]] std::string multiText(const Item& it, const Umgebung& u);

// Was die Engine beim Start selbst an Cvars anlegt, die Menues lesen (SP: ui_char_*
// aus g_char_* von UI_Init) - fuer die Vorschau, solange niemand sie setzt. "" = keine.
[[nodiscard]] std::string spielCvarVorgabe(Dialekt d, Spiel s, const std::string& name);

// Abbildung des virtuellen Bildschirms auf ein Fenster.
enum class Seitenverhaeltnis {
    Strecken,     // wie das Original: 640x480 auf das ganze Fenster
    Balken,       // 4:3 in der Mitte, Balken links/rechts bzw. oben/unten
};
struct Abbildung {
    float sx = 1, sy = 1, ox = 0, oy = 0;
    [[nodiscard]] float x(float v) const { return ox + v * sx; }
    [[nodiscard]] float y(float v) const { return oy + v * sy; }
};
[[nodiscard]] Abbildung abbildung(float breite, float hoehe, Seitenverhaeltnis s);

}  // namespace bhed::menue

#endif  // BHED_MENUE_VORSCHAU_H
