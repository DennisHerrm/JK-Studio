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

#include "bhed/menue/kodierung.h"
#include "bhed/menue/menue.h"
#include "bhed/menue/ownerdraw.h"

#include <functional>
#include <string>
#include <vector>

namespace bhed::menue {

inline constexpr float kSchirmBreite = 640.0F;   // SCREEN_WIDTH
inline constexpr float kSchirmHoehe = 480.0F;    // SCREEN_HEIGHT
inline constexpr float kScrollbar = 16.0F;       // SCROLLBAR_SIZE
inline constexpr const char* kDefaultShader = "*default";   // M5: tr.defaultShader (Griff 0)
inline constexpr int kUiAllmapsSelection = 236;   // code/ui/menudef.h UI_ALLMAPS_SELECTION (Spielstand-Vorschau)
inline constexpr int kUiKeybindStatus = 250;      // UI_KEYBINDSTATUS
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
// M3: Zeichen wie AnyLanguage_ReadCharFromString in der Kodierung der Sprache; asiatische
// Glyphen nur, wenn die Seiten da sind (AsianGlyphsAvailable), Codes > 255 mit 0.75.
[[nodiscard]] int schriftBreite(const Schrift& f, const std::string& text, float scale, Kodierung k = Kodierung::Latin1,
                                bool asienGlyphen = false);
// RE_Font_HeightPixels: mPointSize * scale
[[nodiscard]] int schriftHoehe(const Schrift& f, float scale);

enum class PrimArt {
    Fuellen,      // DC->fillRect
    Bild,         // DC->drawHandlePic / RE_StretchPic (shader, s/t)
    Platzhalter,  // ownerdraw, Feeder ohne Spiel: Rahmen mit Beschriftung
    Modell,       // ITEM_TYPE_MODEL: die 3D-Vorschau zeichnet die Oberflaeche
    Kino,         // WINDOW_STYLE_CINEMATIC (RoQ)
    Beispiel,     // M7: Hinweis "Beispielwerte" ueber einem ownerdraw (nur die Oberflaeche, nicht im Spiel)
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
struct FeederEintrag {
    std::vector<std::string> spalten;
    std::string bild;
    std::vector<std::string> spaltenBild;
};

// Laufzeitstand eines Items (die Skripte aendern ihn: show/hide/setfocus ...).
struct ItemZustand {
    int flags = 0;        // windowDef_t.flags
    bool gueltig = false; // false = die gelesenen Flags nehmen
    int startPos = 0;     // M4: textScrollDef_t.startPos (erste gezeigte Zeile)
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
    // M7: 0 = Liste ohne Darstellung (Platzhalter), 1 = wie im Spiel, 2 = Beispielwerte (FeederDaten::art); ohne: 1
    std::function<int(float feeder)> feederArt;
    // M7: BindingFromName(befehl) fuer ITEM_TYPE_BIND (tasten.h); ohne: "???"
    std::function<std::string(const std::string& befehl)> bindung;
    // M7: DC->ownerDrawItem (ownerdraw.h); ohne: Platzhalter
    std::function<OdErgebnis(const OdAufruf&)> ownerDraw;
    // M7: Schrift beim Namen (cgame zeichnet mit cgs.media.qhFont* statt item->font); ohne: keine
    std::function<const Schrift*(const std::string& name)> schriftNamens;
    // M5: laedt der Renderer dieses Bild/diesen Shader? (RE_RegisterShaderNoMip: fehlt es, kommt
    // Griff 0 = "<default>"-Kasten zurueck; ohne: alles da)
    std::function<bool(const std::string& shader)> bildDa;
    int listCursor = 0;                   // item->cursorPos
    int zeitMs = 0;                       // DC->realTime
    std::vector<ItemZustand> zustand;     // je Item (leer = gelesene Flags)
    bool erzwingen = true;                // forcePaint: auch unsichtbare Menues
    bool alleItems = false;               // Editor: auch unsichtbare Items (gedimmt)
    Dialekt dialekt = Dialekt::MP;
    // M3: se_language der Vorschau (tr_font GetLanguageEnum) und ob die asiatischen
    // Glyphenseiten im Spiel liegen (fonts/<kor|tai|jap|chi>_*_1024_0)
    Kodierung kodierung = Kodierung::Latin1;
    bool asienGlyphen = false;
};

// Menu_Paint fuer ein Menue der Datei.
void zeichneMenu(const Datei& d, int menu, const Umgebung& u, std::vector<Prim>& out);
// M7: Zeichenbefehle (cgame-Text, Bilder) mit Schrift und Farbcodes der Vorschau, Item -1
void zeichneTeile(const std::vector<OdTeil>& teile, const Umgebung& u, std::vector<Prim>& out);

// M4: ITEM_TYPE_TEXTSCROLL. Der Text, wie Item_TextScroll_Paint ihn nimmt (cvar ersetzt text,
// "@" = SE_GetString), umgebrochen wie Item_TextScroll_BuildLines (Breite rect.w - 16 - 10,
// Umbruch nach Leer- und Satzzeichen, AnyLanguage_ReadCharFromString). da[i] = false: NULL-Zeile.
inline constexpr std::size_t kMaxTextScrollZeilen = 256;   // MAX_TEXTSCROLL_LINES
struct TextScrollZeilen {
    std::vector<std::string> zeilen;
    std::vector<bool> da;
};
[[nodiscard]] std::string textScrollText(const Item& it, const Umgebung& u);
[[nodiscard]] TextScrollZeilen textScrollZeilen(const Item& it, const Umgebung& u);
// M7: eine Zeile wie Item_TextScroll_BuildLines bzw. CG_DisplayBoxedText (Zeichen wie
// AnyLanguage_ReadCharFromString, Umbruch hinter dem letzten Leer-/Satzzeichen, sobald die Breite
// boxBreite erreicht); pos wird weitergesetzt. umbruch = false: der Text endete vorher.
struct UmbruchZeile {
    std::string text;
    bool umbruch = false;
};
[[nodiscard]] UmbruchZeile naechsteZeile(const std::string& text, std::size_t& pos, int boxBreite,
                                         const std::function<int(const std::string&)>& breite, Kodierung k);
[[nodiscard]] int textScrollMaxScroll(const Item& it, int zeilen);          // Item_TextScroll_MaxScroll
[[nodiscard]] float textScrollDaumen(const Item& it, int zeilen, int startPos);   // Item_TextScroll_ThumbPosition

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
