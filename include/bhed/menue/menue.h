// menue.h - Menue-Editor, Stufe M1 Punkt 1: Lesen der .menu-Dateien wie die
// Engine und ein Modell, das die Quelltext-Spannen haelt.
//
// Portiert aus OpenJK (codemp/ui/ui_shared.c, codemp/ui/ui_main.c,
// codemp/botlib/l_script.cpp und l_precomp.cpp fuer die MP-Menues;
// code/ui/ui_shared.cpp, code/ui/ui_main.cpp und code/qcommon/q_shared.cpp
// fuer die SP-Menues von Jedi Academy und Jedi Outcast) und aus Movie Duels
// (code/ui/ui_shared.cpp: cvarInt, cvarRotateScale, exec, ownerdrawID).
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
// Zwei Leser wie im Spiel:
//   Dialekt::SP  COM_ParseExt (q_shared.cpp) - jasp.exe, jk2sp.exe, Movie
//                Duels SP. Kein Praeprozessor; type/style/textalign/flag/
//                descAlignment als Namen (ITEM_TYPE_BUTTON ...).
//   Dialekt::MP  botlib-Praeprozessor (l_precomp.cpp) mit #include, #define,
//                Makros, #if... und den globalen Defines aus menudef.h -
//                jamp.exe, jk2mp.exe, Movie Duels MP.
//
// Das Modell haelt den Quelltext unveraendert; jede Eigenschaft kennt die
// Byte-Spannen ihrer Woerter. Speichern ohne Aenderung gibt den Text
// byte-gleich zurueck, eine Aenderung (rect ziehen) ersetzt nur ihre Spanne.
#ifndef BHED_MENUE_MENUE_H
#define BHED_MENUE_MENUE_H

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace bhed::menue {

enum class Dialekt { SP, MP };
// Welches Spiel die Tabellen stellt (Movie Duels kennt vier Woerter mehr).
enum class Spiel { JA, JK2, MD };

// Laedt eine Datei aus dem Spiel (pk3 oder Ordner); false = nicht gefunden.
using Lader = std::function<bool(const std::string& pfad, std::string& inhalt)>;

// botlib token types (l_script.h)
inline constexpr int TT_STRING = 1;
inline constexpr int TT_LITERAL = 2;
inline constexpr int TT_NUMBER = 3;
inline constexpr int TT_NAME = 4;
inline constexpr int TT_PUNCTUATION = 5;

// Ein Wort, wie der Parser des Spiels es geliefert hat, mit Herkunft.
struct Token {
    std::string s;              // wie pc_token_t.string / com_token
    int typ = 0;                // TT_* (SP: TT_NAME oder TT_STRING)
    int subtyp = 0;
    long ival = 0;
    double fval = 0.0;
    int datei = 0;              // 0 = die Hauptdatei, sonst Index in Datei::quellen
    std::uint32_t anfang = 0;   // Byte-Spanne im Text dieser Datei
    std::uint32_t ende = 0;
    int zeile = 0;
    bool ausMakro = false;      // aus einer #define-Erweiterung (Spanne = Aufruf)
};

struct Meldung {
    int datei = 0;
    int zeile = 0;
    std::string text;           // Wortlaut wie die Engine
    bool fehler = false;        // ERROR statt WARNING
};

// Ein Schluesselwort mit seinen Argumenten (menuDef, itemDef oder assetGlobalDef).
struct Eigenschaft {
    std::string schluessel;     // wie im Text
    std::string kanon;          // wie in der Tabelle der Engine (Gross/Klein)
    Token kopf;
    std::vector<Token> args;    // alle gelesenen Woerter (bei Skripten auch { })
    bool ok = true;             // die Engine hat sie angenommen
};

struct Farbe {
    float r = 0, g = 0, b = 0, a = 0;
};
struct Rechteck {
    float x = 0, y = 0, w = 0, h = 0;
};

// windowDef_t (ui_shared.h)
struct Fenster {
    Rechteck rect;              // Bildschirm (nach Menu_UpdatePosition)
    Rechteck rectClient;        // wie im Text
    std::string name, group, background, cinematicName;
    int style = 0;
    int border = 0;
    int ownerDraw = 0;
    int ownerDrawFlags = 0;
    float borderSize = 1.0F;
    int flags = 0;
    Farbe foreColor{1, 1, 1, 1};
    Farbe backColor{0, 0, 0, 0};
    Farbe borderColor{0, 0, 0, 0};
    Farbe outlineColor{0, 0, 0, 0};
};

struct ColorRange {
    float low = 0, high = 0;
    Farbe color;
};
struct Spalte {
    int pos = 0, width = 0, maxChars = 0;
};

// itemDef_t mit den typeData-Teilen (listBoxDef_t, editFieldDef_t,
// multiDef_t, modelDef_t, textScrollDef_t) in einem.
struct Item {
    Fenster window;
    int type = 0;
    bool typeData = false;      // Item_ValidateTypeData hat angelegt
    int alignment = 0;
    int textalignment = 0;
    float textalignx = 0, textaligny = 0;
    float text2alignx = 0, text2aligny = 0;
    float textscale = 0.55F;
    int textStyle = 0;
    std::string text, text2, descText;
    int font = 0;               // SP item->font, MP iMenuFont
    std::string cvar, cvarTest, enableCvar, focusSound;
    int cvarFlags = 0;
    float special = 0;          // feeder
    std::string action, onFocus, leaveFocus, mouseEnter, mouseExit, mouseEnterText, mouseExitText;
    std::string accept, selectNext, selectPrev, doubleClick;
    std::vector<ColorRange> colorRanges;
    int invertYesNo = 0;
    int xoffset = 0;
    int appearanceSlot = 0;
    int itemFlags = 0;          // ITF_*
    // listBoxDef_t
    float elementWidth = 0, elementHeight = 0;
    int elementStyle = 0;
    std::vector<Spalte> columns;
    bool notselectable = false, scrollhidden = false;
    // editFieldDef_t
    float minVal = 0, maxVal = 0, defVal = 0;
    int maxChars = 0, maxPaintChars = 0;
    // multiDef_t
    std::vector<std::string> cvarList, cvarStr;
    std::vector<float> cvarValue;
    bool strDef = false;
    std::string multiFeeder;    // cvarStrList feeder (FEEDER_PLAYER_SPECIES, ...)
    // modelDef_t
    std::string assetModel, assetShader, g2skin, g2anim;
    float origin[3] = {0, 0, 0};
    float g2mins[3] = {0, 0, 0}, g2maxs[3] = {0, 0, 0}, g2scale[3] = {0, 0, 0};
    float fovx = 0, fovy = 0;
    int angle = 0, rotationSpeed = 0;
    // textScrollDef_t
    int lineHeight = 0, maxLineChars = 0;
    std::string rectcvar;

    std::vector<Eigenschaft> eigenschaften;
    Token kopf;                 // "itemDef"
    std::uint32_t anfang = 0;   // "itemDef" im Haupttext
    std::uint32_t ende = 0;     // nach der schliessenden }
    int zeile = 0;
    bool ok = true;
};

// menuDef_t
struct Menu {
    Fenster window;
    std::string font;
    int fullScreen = 0;
    Farbe focusColor{0, 0, 0, 0}, disableColor{0, 0, 0, 0}, descColor{0, 0, 0, 0};
    int descX = 0, descY = 0, descAlignment = 0, descTextStyle = 0;
    float descScale = 0;
    float fadeClamp = 0, fadeAmount = 0;
    int fadeCycle = 0;
    float appearanceIncrement = 0;
    std::string onOpen, onClose, onAccept, onESC, soundName;
    int ownerdrawID = 0;
    std::vector<Item> items;

    std::vector<Eigenschaft> eigenschaften;
    Token kopf;
    std::uint32_t anfang = 0, ende = 0;
    int zeile = 0;
    bool ok = true;
};

// assetGlobalDef (Asset_Parse in ui_main)
struct Assets {
    std::vector<Eigenschaft> eigenschaften;
    std::uint32_t anfang = 0, ende = 0;
    bool vorhanden = false;
};

struct Quelle {
    std::string name;
    std::string text;
};

struct Datei {
    std::string pfad;           // Spielpfad, z. B. ui/jamp/main.menu
    std::string text;           // unveraendert
    Dialekt dialekt = Dialekt::SP;
    Spiel spiel = Spiel::JA;
    std::vector<Menu> menus;
    Assets assets;
    std::vector<Meldung> meldungen;
    std::vector<Quelle> quellen;          // [0] = Hauptdatei (Name), dann #includes
    std::map<std::string, long> defines;  // MP: Zahl-Defines (fuer Vervollstaendigung)
    std::vector<std::string> loadMenus;   // menus.txt: loadMenu { "..." }
};

struct LeseOptionen {
    Dialekt dialekt = Dialekt::SP;
    Spiel spiel = Spiel::JA;
    std::string globaleDefines;           // MP: ui/jamp/menudef.h (leer = keine)
    Lader lader;                          // fuer #include und globale Defines
};

// Liest eine .menu (oder menus.txt) wie UI_ParseMenu / UI_LoadMenus.
[[nodiscard]] Datei lies(const std::string& pfad, const std::string& text, const LeseOptionen& opt);

// Schreibt den Text der Datei (unveraendert = byte-gleich).
[[nodiscard]] const std::string& schreib(const Datei& d) noexcept;

// Dialekt, Spiel und menudef.h aus dem Spielpfad raten:
//   ui/jamp/... ui/jk2mp/... ui/MD_MP/... -> MP mit ui/<ordner>/menudef.h
//   sonst SP.
[[nodiscard]] LeseOptionen optionenFuer(const std::string& pfad, Spiel spiel, Lader lader);

// --- Konstanten aus menudef.h (gleich in SP und MP) -------------------------
inline constexpr int ITEM_TYPE_TEXT = 0;
inline constexpr int ITEM_TYPE_BUTTON = 1;
inline constexpr int ITEM_TYPE_RADIOBUTTON = 2;
inline constexpr int ITEM_TYPE_CHECKBOX = 3;
inline constexpr int ITEM_TYPE_EDITFIELD = 4;
inline constexpr int ITEM_TYPE_COMBO = 5;
inline constexpr int ITEM_TYPE_LISTBOX = 6;
inline constexpr int ITEM_TYPE_MODEL = 7;
inline constexpr int ITEM_TYPE_OWNERDRAW = 8;
inline constexpr int ITEM_TYPE_NUMERICFIELD = 9;
inline constexpr int ITEM_TYPE_SLIDER = 10;
inline constexpr int ITEM_TYPE_YESNO = 11;
inline constexpr int ITEM_TYPE_MULTI = 12;
inline constexpr int ITEM_TYPE_BIND = 13;
inline constexpr int ITEM_TYPE_TEXTSCROLL = 14;

inline constexpr int ITEM_ALIGN_LEFT = 0;
inline constexpr int ITEM_ALIGN_CENTER = 1;
inline constexpr int ITEM_ALIGN_RIGHT = 2;

inline constexpr int WINDOW_STYLE_EMPTY = 0;
inline constexpr int WINDOW_STYLE_FILLED = 1;
inline constexpr int WINDOW_STYLE_GRADIENT = 2;
inline constexpr int WINDOW_STYLE_SHADER = 3;
inline constexpr int WINDOW_STYLE_TEAMCOLOR = 4;
inline constexpr int WINDOW_STYLE_CINEMATIC = 5;

inline constexpr int WINDOW_BORDER_NONE = 0;
inline constexpr int WINDOW_BORDER_FULL = 1;
inline constexpr int WINDOW_BORDER_HORZ = 2;
inline constexpr int WINDOW_BORDER_VERT = 3;
inline constexpr int WINDOW_BORDER_KCGRADIENT = 4;

// windowDef_t.flags (ui_shared.h)
inline constexpr int WINDOW_MOUSEOVER = 0x00000001;
inline constexpr int WINDOW_HASFOCUS = 0x00000002;
inline constexpr int WINDOW_VISIBLE = 0x00000004;
inline constexpr int WINDOW_INACTIVE = 0x00000008;
inline constexpr int WINDOW_DECORATION = 0x00000010;
inline constexpr int WINDOW_FADINGOUT = 0x00000020;
inline constexpr int WINDOW_FADINGIN = 0x00000040;
inline constexpr int WINDOW_MOUSEOVERTEXT = 0x00000080;
inline constexpr int WINDOW_INTRANSITION = 0x00000100;
inline constexpr int WINDOW_FORECOLORSET = 0x00000200;
inline constexpr int WINDOW_HORIZONTAL = 0x00000400;
inline constexpr int WINDOW_LB_LEFTARROW = 0x00000800;
inline constexpr int WINDOW_LB_RIGHTARROW = 0x00001000;
inline constexpr int WINDOW_LB_THUMB = 0x00002000;
inline constexpr int WINDOW_LB_PGUP = 0x00004000;
inline constexpr int WINDOW_LB_PGDN = 0x00008000;
inline constexpr int WINDOW_ORBITING = 0x00010000;
inline constexpr int WINDOW_OOB_CLICK = 0x00020000;
inline constexpr int WINDOW_WRAPPED = 0x00040000;
inline constexpr int WINDOW_AUTOWRAPPED = 0x00080000;
inline constexpr int WINDOW_FORCED = 0x00100000;
inline constexpr int WINDOW_POPUP = 0x00200000;
inline constexpr int WINDOW_BACKCOLORSET = 0x00400000;
inline constexpr int WINDOW_TIMEDVISIBLE = 0x00800000;
inline constexpr int WINDOW_IGNORE_HUDALPHA = 0x01000000;
inline constexpr int WINDOW_DROPSHADOW = 0x02000000;
inline constexpr int WINDOW_INTRANSITIONMODEL = 0x04000000;

// item->cvarFlags
inline constexpr int CVAR_ENABLE = 0x00000001;
inline constexpr int CVAR_DISABLE = 0x00000002;
inline constexpr int CVAR_SHOW = 0x00000004;
inline constexpr int CVAR_HIDE = 0x00000008;
inline constexpr int CVAR_SUBSTRING = 0x00000010;

// item->itemFlags (SP ui_shared.h ITF_*)
inline constexpr int ITF_G2VALID = 0x0001;
inline constexpr int ITF_ISCHARACTER = 0x0002;
inline constexpr int ITF_ISSABER = 0x0004;
inline constexpr int ITF_ISSABER2 = 0x0008;
inline constexpr int ITF_ISANYSABER = ITF_ISSABER | ITF_ISSABER2;

// Name des Item-Typs (ITEM_TYPE_*), "" wenn unbekannt.
[[nodiscard]] const char* typName(int typ) noexcept;
[[nodiscard]] const char* styleName(int style) noexcept;

// Alle Schluesselwoerter der Engine fuer Menue und Item im gewaehlten
// Dialekt/Spiel (Vervollstaendigung, Syntaxfarben).
[[nodiscard]] std::vector<std::string> menuSchluessel(Dialekt d, Spiel s);
[[nodiscard]] std::vector<std::string> itemSchluessel(Dialekt d, Spiel s);

// --- Bearbeiten ueber Spannen ---------------------------------------------

// Zeile (1-basiert) zu einem Byte im Text.
[[nodiscard]] int zeileVon(const std::string& text, std::uint32_t byte) noexcept;

// Eigenschaft im Item suchen (Gross/Klein egal), nullptr wenn nicht da.
[[nodiscard]] const Eigenschaft* finde(const Item& it, const char* schluessel) noexcept;
[[nodiscard]] const Eigenschaft* finde(const Menu& m, const char* schluessel) noexcept;

// Eine Textaenderung: [anfang, ende) durch neu ersetzen.
struct Ersetzung {
    std::uint32_t anfang = 0, ende = 0;
    std::string neu;
};

// Die Ersetzungen, die "rect" eines Items setzen. Vorhandene Zahlen werden
// einzeln ersetzt (Kommentare und Abstaende bleiben); fehlt rect, kommt eine
// neue Zeile mit der Einrueckung des Items dazu. false, wenn rect aus einem
// Makro kommt oder nicht in der Hauptdatei steht (dann bleibt alles).
[[nodiscard]] bool setzeRect(const Datei& d, int menu, int item, const Rechteck& r,
                             std::vector<Ersetzung>& out);
// Dasselbe fuer ein beliebiges Schluesselwort mit fertigen Argumenttexten.
[[nodiscard]] bool setzeEigenschaft(const Datei& d, int menu, int item, const char* schluessel,
                                    const std::vector<std::string>& werte, std::vector<Ersetzung>& out);

// Wendet Ersetzungen (nicht ueberlappend) auf den Text an.
[[nodiscard]] std::string anwenden(const std::string& text, std::vector<Ersetzung> e);

// Meldung im Wortlaut der Engine:
//   MP  "ERROR: ui/jamp/main.menu, line 12: unknown menu item keyword foo"
//   SP  "WARNING: Unknown item keyword 'foo' Line #12 of File 'ui/main.menu'"
[[nodiscard]] std::string meldungText(const Datei& d, const Meldung& m);

// --- @MENUS-Texte (StringEd .str von JA/MD, StripEd .sp von JK2) ------------
struct Texte {
    std::map<std::string, std::string> werte;   // "MENUS_JEDI_HF" (gross) -> Text
    // .str: Dateiname gibt das Paket (strings/English/MENUS.str -> MENUS),
    // REFERENCE x / LANG_ENGLISH "..." (sprache = "ENGLISH", "GERMAN" ...).
    void ladeStr(const std::string& dateiname, const std::string& text, const std::string& sprache = "ENGLISH");
    // .sp: REFERENCE <Paket> am Anfang, dann REFERENCE x / TEXT_LANGUAGE<n> "..."
    void ladeSp(const std::string& text, int sprache = 1);
    // "@MENUS_X" -> Text; ohne @ oder unbekannt: unveraendert (wie SE_GetString
    // liefert die Engine bei unbekannten Schluesseln den Schluessel).
    [[nodiscard]] std::string aufloesen(const std::string& s) const;
    [[nodiscard]] bool kennt(const std::string& s) const;
};

// Zahl so schreiben, wie ein Mensch sie in eine .menu schreibt (12, 0.5, .25 bleibt nicht).
[[nodiscard]] std::string zahlText(float v);

}  // namespace bhed::menue

#endif  // BHED_MENUE_MENUE_H
