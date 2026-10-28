// parser.cpp - Menue-Editor M1: menuDef/itemDef/assetGlobalDef lesen wie die
// Engine, ins Modell mit Quelltext-Spannen.
//
// Portiert aus OpenJK: codemp/ui/ui_shared.c (PC_Float_Parse, PC_Int_Parse,
// PC_Rect_Parse, PC_String_Parse, PC_Script_Parse, itemParseKeywords,
// menuParseKeywords, Item_Parse, Menu_Parse, Menu_New, Menu_PostParse,
// Menu_UpdatePosition, Item_SetScreenCoords, Item_ValidateTypeData),
// codemp/ui/ui_main.c (UI_ParseMenu, Load_Menu, UI_LoadMenus, Asset_Parse),
// code/ui/ui_shared.cpp und code/ui/ui_main.cpp (dieselben Funktionen der
// SP-Fassung mit COM_ParseExt, den Namenstabellen types[]/styles[]/
// alignment[]/itemFlags[] und PC_ParseWarning) sowie aus Movie Duels
// code/ui/ui_shared.cpp (cvarInt, cvarRotateScale, exec, ownerdrawID).
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
#include "bhed/menue/menue.h"

#include "lexer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>

namespace bhed::menue {

using intern::ComLeser;
using intern::PcQuelle;

namespace {

bool gleichOhneFall(const std::string& a, const char* b) noexcept {
    const std::size_t n = std::strlen(b);
    if (a.size() != n) { return false; }
    for (std::size_t i = 0; i < n; ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// --- Die Argumentarten der Schluesselwoerter --------------------------------
enum class Art {
    Str,          // PC_String_Parse / PC_ParseStringMem
    Tok,          // trap->PC_ReadToken / PC_ParseString (background, focusSound ...)
    Int,
    Float,
    Rect,
    Color,
    Vec3,
    Script,
    Keine,        // decoration, wrapped ...
    Name,         // SP: type/style/textalign/descAlignment als Name
    Flag,         // flag WINDOW_INACTIVE
    CvarFloat,    // cvar def min max
    CvarRotate,   // MD: cvar range
    CvarStrList,
    CvarFloatList,
    Columns,
    ColorRange,
    ImmerFalsch,  // MD ItemParse_ownerdrawID: return qfalse
};

enum Bit : unsigned {
    kSP = 1U,      // in code/ui (JA SP, JK2 SP)
    kMP = 2U,      // in codemp/ui
    kMD = 4U,      // nur Movie Duels (SP)
    kTD = 8U,      // braucht typeData (Item_ValidateTypeData, sonst qfalse)
};

struct Schluessel {
    const char* name;
    Art sp;       // Art in der SP-Fassung
    Art mp;       // Art in der MP-Fassung
    unsigned bits;
};

constexpr unsigned kBeide = kSP | kMP;

// itemParseKeywords - Reihenfolge wie in code/ui/ui_shared.cpp; MP-only und
// MD-only am Ende ihrer Gruppe.
const Schluessel kItem[] = {
    {"accept", Art::Script, Art::Script, kSP},
    {"selectNext", Art::Script, Art::Script, kSP},
    {"selectPrev", Art::Script, Art::Script, kSP},
    {"action", Art::Script, Art::Script, kBeide},
    {"addColorRange", Art::ColorRange, Art::ColorRange, kBeide},
    {"align", Art::Int, Art::Int, kBeide},
    {"appearance_slot", Art::Int, Art::Int, kBeide},
    {"asset_model", Art::Tok, Art::Tok, kBeide | kTD},
    {"asset_shader", Art::Tok, Art::Tok, kBeide},
    {"isCharacter", Art::Int, Art::Int, kBeide},
    {"isSaber", Art::Int, Art::Int, kBeide},
    {"isSaber2", Art::Int, Art::Int, kBeide},
    {"autowrapped", Art::Keine, Art::Keine, kBeide},
    {"backcolor", Art::Color, Art::Color, kBeide},
    {"background", Art::Tok, Art::Tok, kBeide},
    {"border", Art::Int, Art::Int, kBeide},
    {"bordercolor", Art::Color, Art::Color, kBeide},
    {"bordersize", Art::Float, Art::Float, kBeide},
    {"cinematic", Art::Str, Art::Str, kBeide},
    {"columns", Art::Columns, Art::Columns, kBeide | kTD},
    {"cvar", Art::Str, Art::Str, kBeide},
    {"cvarFloat", Art::CvarFloat, Art::CvarFloat, kBeide | kTD},
    {"cvarInt", Art::CvarFloat, Art::CvarFloat, kMD | kTD},
    {"cvarRotateScale", Art::CvarRotate, Art::CvarRotate, kMD | kTD},
    {"cvarFloatList", Art::CvarFloatList, Art::CvarFloatList, kBeide | kTD},
    {"cvarSubString", Art::Keine, Art::Keine, kSP},
    {"cvarStrList", Art::CvarStrList, Art::CvarStrList, kBeide | kTD},
    {"cvarTest", Art::Str, Art::Str, kBeide},
    {"decoration", Art::Keine, Art::Keine, kBeide},
    {"desctext", Art::Str, Art::Str, kBeide},
    {"disableCvar", Art::Script, Art::Script, kBeide},
    {"doubleclick", Art::Script, Art::Script, kBeide | kTD},
    {"elementheight", Art::Float, Art::Float, kBeide | kTD},
    {"elementtype", Art::Int, Art::Int, kBeide | kTD},
    {"elementwidth", Art::Float, Art::Float, kBeide | kTD},
    {"enableCvar", Art::Script, Art::Script, kBeide},
    {"exec", Art::Script, Art::Script, kMD},
    {"feeder", Art::Float, Art::Float, kBeide},
    {"flag", Art::Flag, Art::Flag, kBeide},
    {"focusSound", Art::Tok, Art::Tok, kBeide},
    {"font", Art::Int, Art::Int, kBeide},
    {"forecolor", Art::Color, Art::Color, kBeide},
    {"group", Art::Str, Art::Str, kBeide},
    {"hideCvar", Art::Script, Art::Script, kBeide},
    {"horizontalscroll", Art::Keine, Art::Keine, kBeide},
    {"leaveFocus", Art::Script, Art::Script, kBeide},
    {"maxChars", Art::Int, Art::Int, kBeide | kTD},
    {"maxPaintChars", Art::Int, Art::Int, kBeide | kTD},
    {"model_angle", Art::Int, Art::Int, kBeide | kTD},
    {"model_fovx", Art::Float, Art::Float, kBeide | kTD},
    {"model_fovy", Art::Float, Art::Float, kBeide | kTD},
    {"model_origin", Art::Vec3, Art::Vec3, kBeide | kTD},
    {"model_rotation", Art::Int, Art::Int, kBeide | kTD},
    {"model_g2mins", Art::Vec3, Art::Vec3, kBeide | kTD},
    {"model_g2maxs", Art::Vec3, Art::Vec3, kBeide | kTD},
    {"model_g2scale", Art::Vec3, Art::Vec3, kMP | kTD},
    {"model_g2skin", Art::Tok, Art::Tok, kBeide | kTD},
    {"model_g2anim", Art::Tok, Art::Tok, kBeide | kTD},
    {"mouseEnter", Art::Script, Art::Script, kBeide},
    {"mouseEnterText", Art::Script, Art::Script, kBeide},
    {"mouseExit", Art::Script, Art::Script, kBeide},
    {"mouseExitText", Art::Script, Art::Script, kBeide},
    {"name", Art::Str, Art::Str, kBeide},
    {"notselectable", Art::Keine, Art::Keine, kBeide},
    {"scrollhidden", Art::Keine, Art::Keine, kBeide},
    {"onFocus", Art::Script, Art::Script, kBeide},
    {"outlinecolor", Art::Color, Art::Color, kBeide},
    {"ownerdraw", Art::Int, Art::Int, kBeide},
    {"ownerdrawFlag", Art::Int, Art::Int, kBeide},
    {"ownerdrawID", Art::ImmerFalsch, Art::ImmerFalsch, kMD},
    {"rect", Art::Rect, Art::Rect, kBeide},
    {"rectcvar", Art::Tok, Art::Tok, kMP},
    {"showCvar", Art::Script, Art::Script, kBeide},
    {"special", Art::Float, Art::Float, kBeide},
    {"style", Art::Name, Art::Int, kBeide},
    {"text", Art::Str, Art::Str, kBeide},
    {"text2", Art::Str, Art::Str, kBeide},
    {"text2alignx", Art::Float, Art::Float, kBeide},
    {"text2aligny", Art::Float, Art::Float, kBeide},
    {"textalign", Art::Name, Art::Int, kBeide},
    {"textalignx", Art::Float, Art::Float, kBeide},
    {"textaligny", Art::Float, Art::Float, kBeide},
    {"textscale", Art::Float, Art::Float, kBeide},
    {"textstyle", Art::Int, Art::Int, kBeide},
    {"type", Art::Name, Art::Int, kBeide},
    {"visible", Art::Int, Art::Int, kBeide},
    {"wrapped", Art::Keine, Art::Keine, kBeide},
    {"invertyesno", Art::Int, Art::Int, kBeide},
    {"xoffset", Art::Int, Art::Int, kSP},   // MP: "xoffset\t\t" - passt nie (Tabs im Namen)
    {"lineHeight", Art::Int, Art::Int, kBeide | kTD},
    {"maxLineChars", Art::Int, Art::Int, kMP | kTD},
};

// menuParseKeywords
const Schluessel kMenu[] = {
    {"appearanceIncrement", Art::Float, Art::Float, kBeide},
    {"backcolor", Art::Color, Art::Color, kBeide},
    {"background", Art::Tok, Art::Tok, kBeide},
    {"border", Art::Int, Art::Int, kBeide},
    {"bordercolor", Art::Color, Art::Color, kBeide},
    {"borderSize", Art::Float, Art::Float, kBeide},
    {"cinematic", Art::Str, Art::Str, kBeide},
    {"descAlignment", Art::Name, Art::Int, kBeide},
    {"descTextStyle", Art::Int, Art::Int, kSP},
    {"desccolor", Art::Color, Art::Color, kBeide},
    {"descX", Art::Int, Art::Int, kBeide},
    {"descY", Art::Int, Art::Int, kBeide},
    {"descScale", Art::Float, Art::Float, kBeide},
    {"disablecolor", Art::Color, Art::Color, kBeide},
    {"fadeClamp", Art::Float, Art::Float, kBeide},
    {"fadeCycle", Art::Int, Art::Int, kBeide},
    {"fadeAmount", Art::Float, Art::Float, kBeide},
    {"focuscolor", Art::Color, Art::Color, kBeide},
    {"font", Art::Str, Art::Str, kBeide},
    {"forecolor", Art::Color, Art::Color, kBeide},
    {"fullscreen", Art::Int, Art::Int, kBeide},
    {"itemDef", Art::Keine, Art::Keine, kBeide},
    {"name", Art::Str, Art::Str, kBeide},
    {"onClose", Art::Script, Art::Script, kBeide},
    {"onAccept", Art::Script, Art::Script, kBeide},
    {"onESC", Art::Script, Art::Script, kBeide},
    {"onOpen", Art::Script, Art::Script, kBeide},
    {"outlinecolor", Art::Color, Art::Color, kBeide},
    {"outOfBoundsClick", Art::Keine, Art::Keine, kBeide},
    {"ownerdraw", Art::Int, Art::Int, kBeide},
    {"ownerdrawFlag", Art::Int, Art::Int, kBeide},
    {"ownerdrawID", Art::Int, Art::Int, kMD},
    {"popup", Art::Keine, Art::Keine, kBeide},
    {"rect", Art::Rect, Art::Rect, kBeide},
    {"soundLoop", Art::Str, Art::Str, kBeide},
    {"style", Art::Name, Art::Int, kBeide},
    {"visible", Art::Int, Art::Int, kBeide},
    {"ignoreescape", Art::Int, Art::Int, kSP},
};

bool gilt(const Schluessel& k, Dialekt d, Spiel s) noexcept {
    if (d == Dialekt::MP) { return (k.bits & kMP) != 0; }
    if ((k.bits & kSP) != 0) { return true; }
    return (k.bits & kMD) != 0 && s == Spiel::MD;
}

template <std::size_t N>
const Schluessel* sucheIn(const Schluessel (&tab)[N], const std::string& wort, Dialekt d, Spiel s) {
    for (const Schluessel& k : tab) {
        if (gilt(k, d, s) && gleichOhneFall(wort, k.name)) { return &k; }
    }
    return nullptr;
}

// SP-Namenstabellen (code/ui/ui_shared.cpp)
const char* const kStyles[] = {"WINDOW_STYLE_EMPTY",  "WINDOW_STYLE_FILLED",   "WINDOW_STYLE_GRADIENT",
                               "WINDOW_STYLE_SHADER", "WINDOW_STYLE_TEAMCOLOR", "WINDOW_STYLE_CINEMATIC"};
const char* const kTypes[] = {"ITEM_TYPE_TEXT",     "ITEM_TYPE_BUTTON",   "ITEM_TYPE_RADIOBUTTON",
                              "ITEM_TYPE_CHECKBOX", "ITEM_TYPE_EDITFIELD", "ITEM_TYPE_COMBO",
                              "ITEM_TYPE_LISTBOX",  "ITEM_TYPE_MODEL",    "ITEM_TYPE_OWNERDRAW",
                              "ITEM_TYPE_NUMERICFIELD", "ITEM_TYPE_SLIDER", "ITEM_TYPE_YESNO",
                              "ITEM_TYPE_MULTI",    "ITEM_TYPE_BIND",     "ITEM_TYPE_TEXTSCROLL"};
const char* const kAlign[] = {"ITEM_ALIGN_LEFT", "ITEM_ALIGN_CENTER", "ITEM_ALIGN_RIGHT"};

template <std::size_t N>
int nameIndex(const char* const (&tab)[N], const std::string& w) {
    for (std::size_t i = 0; i < N; ++i) {
        if (gleichOhneFall(w, tab[i])) { return static_cast<int>(i); }
    }
    return -1;
}

// ============================================================================
// Die Grundleser beider Fassungen, mit Aufzeichnung der Woerter.
// ============================================================================
class Grund {
public:
    virtual ~Grund() = default;
    std::vector<Token>* aufz = nullptr;
    // true = gelesen (MP-Sinn; die SP-Fassung dreht ihre Rueckgaben um)
    virtual bool str(std::string& out) = 0;
    virtual bool tok(std::string& out) = 0;
    virtual bool ganz(int& v) = 0;
    virtual bool komma(float& v) = 0;
    virtual bool skript(std::string& out) = 0;
    // Wort fuer die Schluesselwort-Schleife des Items/Menues
    virtual bool schluesselwort(Token& t, bool imItem) = 0;
    virtual void warn(const std::string& text, bool fehler) = 0;
    bool farbe(Farbe& c) {
        float f[4] = {0, 0, 0, 0};
        for (float& x : f) {
            if (!komma(x)) { return false; }
        }
        c = Farbe{f[0], f[1], f[2], f[3]};
        return true;
    }
    bool rechteck(Rechteck& r) {
        return komma(r.x) && komma(r.y) && komma(r.w) && komma(r.h);
    }
    bool vec3(float (&v)[3]) { return komma(v[0]) && komma(v[1]) && komma(v[2]); }

protected:
    void merk(const Token& t) {
        if (aufz != nullptr) { aufz->push_back(t); }
    }
};

class SpGrund final : public Grund {
public:
    SpGrund(const std::string& text, Datei& d) : lex_(text), d_(d) { lex_.printf = &printf_; }
    ComLeser lex_;
    Datei& d_;
    std::vector<std::string> printf_;

    void flush() {
        for (const std::string& p : printf_) {
            d_.meldungen.push_back(Meldung{0, lex_.zeile(), p, false});
        }
        printf_.clear();
    }
    // PC_ParseString / PC_ParseStringMem
    bool str(std::string& out) override {
        Token t;
        const bool err = lex_.parseString(t);
        flush();
        if (err) { return false; }
        merk(t);
        out = t.s;
        return true;
    }
    bool tok(std::string& out) override { return str(out); }
    bool ganz(int& v) override {
        Token t;
        const bool err = lex_.parseInt(v, t);
        flush();
        if (err) { return false; }
        merk(t);
        return true;
    }
    bool komma(float& v) override {
        Token t;
        const bool err = lex_.parseFloat(v, t);
        flush();
        if (err) { return false; }
        merk(t);
        return true;
    }
    // PC_Script_Parse (SP): Woerter mit PC_ParseExt, lange in "", je ein " ".
    bool skript(std::string& out) override {
        std::string s;
        Token t = lex_.parseExt(true);
        if (t.s.empty() && t.typ != TT_STRING) { return false; }
        merk(t);
        if (t.s.empty() || t.s[0] != '{') { return false; }
        while (true) {
            t = lex_.parseExt(true);
            if (t.s.empty() && lex_.amEnde()) { return false; }
            merk(t);
            if (!t.s.empty() && t.s[0] == '}') {
                out = s;
                return true;
            }
            if (t.s.size() > 1) { s += "\"" + t.s + "\""; }
            else { s += t.s; }
            s += " ";
        }
    }
    bool schluesselwort(Token& t, bool imItem) override {
        if (imItem) {
            const bool err = lex_.parseString(t);
            flush();
            return !err;
        }
        t = lex_.parseExt(true);   // Menu_Parse: PC_ParseExt - nie NULL, leer = Ende
        return true;
    }
    void warn(const std::string& text, bool fehler) override {
        d_.meldungen.push_back(Meldung{0, lex_.zeile(), text, fehler});
    }
};

class MpGrund final : public Grund {
public:
    MpGrund(PcQuelle& q, Datei& d) : q_(q), d_(d) {}
    PcQuelle& q_;
    Datei& d_;
    bool lese(Token& t) {
        const bool ok = q_.lies(t);
        if (ok) { merk(t); }
        return ok;
    }
    bool str(std::string& out) override {
        Token t;
        if (!lese(t)) { return false; }
        out = t.s;
        return true;
    }
    bool tok(std::string& out) override { return str(out); }
    bool ganz(int& v) override {   // PC_Int_Parse
        Token t;
        bool neg = false;
        if (!lese(t)) { return false; }
        if (!t.s.empty() && t.s[0] == '-') {
            if (!lese(t)) { return false; }
            neg = true;
        }
        if (t.typ != TT_NUMBER) {
            q_.uiFehler("expected integer but found " + t.s);
            return false;
        }
        v = static_cast<int>(t.ival);
        if (neg) { v = -v; }
        return true;
    }
    bool komma(float& v) override {   // PC_Float_Parse
        Token t;
        bool neg = false;
        if (!lese(t)) { return false; }
        if (!t.s.empty() && t.s[0] == '-') {
            if (!lese(t)) { return false; }
            neg = true;
        }
        if (t.typ != TT_NUMBER) {
            q_.uiFehler("expected float but found " + t.s);
            return false;
        }
        v = neg ? -static_cast<float>(t.fval) : static_cast<float>(t.fval);
        return true;
    }
    bool skript(std::string& out) override {   // PC_Script_Parse (MP, 2048)
        std::string s;
        Token t;
        if (!lese(t)) { return false; }
        if (!gleichOhneFall(t.s, "{")) { return false; }
        while (true) {
            if (!lese(t)) { return false; }
            if (gleichOhneFall(t.s, "}")) {
                out = s;
                return true;
            }
            if (t.s.size() > 1) { s += "\"" + t.s + "\""; }
            else { s += t.s; }
            s += " ";
            if (s.size() > 2047) { s.resize(2047); }
        }
    }
    bool schluesselwort(Token& t, bool) override { return q_.lies(t); }
    void warn(const std::string& text, bool fehler) override {
        q_.uiFehler(text);
        d_.meldungen.back().fehler = fehler;
    }
};

// ============================================================================
// Die Schluesselwoerter anwenden.
// ============================================================================

void typDatenPruefen(Item& it) {   // Item_ValidateTypeData
    if (it.typeData) { return; }
    switch (it.type) {
        case ITEM_TYPE_LISTBOX:
        case ITEM_TYPE_TEXT:
        case ITEM_TYPE_EDITFIELD:
        case ITEM_TYPE_NUMERICFIELD:
        case ITEM_TYPE_YESNO:
        case ITEM_TYPE_BIND:
        case ITEM_TYPE_SLIDER:
        case ITEM_TYPE_MULTI:
        case ITEM_TYPE_MODEL:
        case ITEM_TYPE_TEXTSCROLL:
            it.typeData = true;
            if (it.type == ITEM_TYPE_EDITFIELD || it.type == ITEM_TYPE_NUMERICFIELD) { it.maxPaintChars = 256; }
            break;
        default:
            break;
    }
}

// Liest die Argumente von k fuer ein Item. false = "couldn't parse".
bool itemArg(Grund& g, const Schluessel& k, Dialekt d, Item& it) {
    const std::string n = k.name;
    const Art art = (d == Dialekt::MP) ? k.mp : k.sp;
    if ((k.bits & kTD) != 0 || n == "cvar" || n == "font" || n == "notselectable" || n == "scrollhidden") {
        typDatenPruefen(it);
    }
    if ((k.bits & kTD) != 0 && !it.typeData) { return false; }
    switch (art) {
        case Art::ImmerFalsch:
            return false;
        case Art::Keine:
            if (n == "decoration") { it.window.flags |= WINDOW_DECORATION; }
            else if (n == "wrapped") { it.window.flags |= WINDOW_WRAPPED; }
            else if (n == "autowrapped") { it.window.flags |= WINDOW_AUTOWRAPPED; }
            else if (n == "horizontalscroll") { it.window.flags |= WINDOW_HORIZONTAL; }
            else if (n == "notselectable") { if (it.type == ITEM_TYPE_LISTBOX && it.typeData) { it.notselectable = true; } }
            else if (n == "scrollhidden") { if (it.typeData) { it.scrollhidden = true; } }
            else if (n == "cvarSubString") { it.cvarFlags |= CVAR_SUBSTRING; }
            return true;
        case Art::Str:
        case Art::Tok: {
            std::string s;
            if (!g.str(s)) { return false; }
            if (n == "name") { it.window.name = s; }
            else if (n == "group") { it.window.group = s; }
            else if (n == "text") { it.text = s; }
            else if (n == "text2") { it.text2 = s; }
            else if (n == "desctext") { it.descText = s; }
            else if (n == "cinematic") { it.window.cinematicName = s; }
            else if (n == "cvar") {
                it.cvar = s;
                if (it.typeData) {   // MP ItemParse_cvar: minVal -1, maxVal -1, defVal -1
                    if (d == Dialekt::MP) { it.minVal = it.maxVal = it.defVal = -1; }
                }
            }
            else if (n == "cvarTest") { it.cvarTest = s; }
            else if (n == "background") { it.window.background = s; }
            else if (n == "focusSound") { it.focusSound = s; }
            else if (n == "asset_model") { it.assetModel = s; }
            else if (n == "asset_shader") { it.assetShader = s; }
            else if (n == "model_g2skin") { it.g2skin = s; }
            else if (n == "model_g2anim") { it.g2anim = s; }
            else if (n == "rectcvar") { it.rectcvar = s; }
            return true;
        }
        case Art::Int: {
            int v = 0;
            if (!g.ganz(v)) { return false; }
            if (n == "align") { it.alignment = v; }
            else if (n == "appearance_slot") { it.appearanceSlot = v; }
            else if (n == "isCharacter") { if (v != 0) { it.itemFlags |= ITF_ISCHARACTER; } else { it.itemFlags &= ~ITF_ISCHARACTER; } }
            else if (n == "isSaber") { if (v != 0) { it.itemFlags |= ITF_ISSABER; } else { it.itemFlags &= ~ITF_ISSABER; } }
            else if (n == "isSaber2") { if (v != 0) { it.itemFlags |= ITF_ISSABER2; } else { it.itemFlags &= ~ITF_ISSABER2; } }
            else if (n == "border") { it.window.border = v; }
            else if (n == "elementtype") { it.elementStyle = v; }
            else if (n == "font") { it.font = v; }
            else if (n == "maxChars") { it.maxChars = v; }
            else if (n == "maxPaintChars") { it.maxPaintChars = v; }
            else if (n == "model_angle") { it.angle = v; }
            else if (n == "model_rotation") { it.rotationSpeed = v; }
            else if (n == "ownerdraw") { it.window.ownerDraw = v; it.type = ITEM_TYPE_OWNERDRAW; }
            else if (n == "ownerdrawFlag") { it.window.ownerDrawFlags |= v; }
            else if (n == "style") { it.window.style = v; }
            else if (n == "textalign") { it.textalignment = v; }
            else if (n == "textstyle") { it.textStyle = v; }
            else if (n == "type") { it.type = v; typDatenPruefen(it); }
            else if (n == "visible") { if (v != 0) { it.window.flags |= WINDOW_VISIBLE; } }
            else if (n == "invertyesno") { it.invertYesNo = v; }
            else if (n == "xoffset") { it.xoffset = v; }
            else if (n == "lineHeight") { it.lineHeight = v; }
            else if (n == "maxLineChars") { it.maxLineChars = v; }
            return true;
        }
        case Art::Float: {
            float v = 0;
            if (!g.komma(v)) { return false; }
            if (n == "bordersize") { it.window.borderSize = v; }
            else if (n == "elementheight") { it.elementHeight = v; }
            else if (n == "elementwidth") { it.elementWidth = v; }
            else if (n == "feeder" || n == "special") { it.special = v; }
            else if (n == "model_fovx") { it.fovx = v; }
            else if (n == "model_fovy") { it.fovy = v; }
            else if (n == "text2alignx") { it.text2alignx = v; }
            else if (n == "text2aligny") { it.text2aligny = v; }
            else if (n == "textalignx") { it.textalignx = v; }
            else if (n == "textaligny") { it.textaligny = v; }
            else if (n == "textscale") { it.textscale = v; }
            return true;
        }
        case Art::Rect:
            return g.rechteck(it.window.rectClient);
        case Art::Color: {
            Farbe c;
            if (!g.farbe(c)) { return false; }
            if (n == "backcolor") { it.window.backColor = c; it.window.flags |= WINDOW_BACKCOLORSET; }
            else if (n == "forecolor") { it.window.foreColor = c; it.window.flags |= WINDOW_FORECOLORSET; }
            else if (n == "bordercolor") { it.window.borderColor = c; }
            else if (n == "outlinecolor") { it.window.outlineColor = c; }
            return true;
        }
        case Art::Vec3:
            if (n == "model_origin") { return g.vec3(it.origin); }
            if (n == "model_g2mins") { return g.vec3(it.g2mins); }
            if (n == "model_g2maxs") { return g.vec3(it.g2maxs); }
            return g.vec3(it.g2scale);
        case Art::Script: {
            std::string s;
            if (!g.skript(s)) {
                // MP enable/disable/show/hideCvar: "if (PC_Script_Parse(...)) {...return qtrue} return qfalse"
                return false;
            }
            if (n == "action") { it.action = s; }
            else if (n == "accept") { it.accept = s; }
            else if (n == "selectNext") { it.selectNext = s; }
            else if (n == "selectPrev") { it.selectPrev = s; }
            else if (n == "doubleclick") { it.doubleClick = s; }
            else if (n == "onFocus") { it.onFocus = s; }
            else if (n == "leaveFocus") { it.leaveFocus = s; }
            else if (n == "mouseEnter") { it.mouseEnter = s; }
            else if (n == "mouseExit") { it.mouseExit = s; }
            else if (n == "mouseEnterText") { it.mouseEnterText = s; }
            else if (n == "mouseExitText") { it.mouseExitText = s; }
            else if (n == "enableCvar") { it.enableCvar = s; it.cvarFlags = CVAR_ENABLE; }
            else if (n == "disableCvar") { it.enableCvar = s; it.cvarFlags = CVAR_DISABLE; }
            else if (n == "showCvar") { it.enableCvar = s; it.cvarFlags = CVAR_SHOW; }
            else if (n == "hideCvar") { it.enableCvar = s; it.cvarFlags = CVAR_HIDE; }
            return true;
        }
        case Art::Name: {
            std::string s;
            if (!g.str(s)) { return false; }
            if (n == "type") {
                const int i = nameIndex(kTypes, s);
                if (i < 0) { g.warn("Unknown item type value '" + s + "'", false); }
                else { it.type = i; }
                typDatenPruefen(it);
            } else if (n == "style") {
                const int i = nameIndex(kStyles, s);
                if (i < 0) { g.warn("Unknown item style value '" + s + "'", false); }
                else { it.window.style = i; }
            } else if (n == "textalign") {
                const int i = nameIndex(kAlign, s);
                if (i < 0) { g.warn("Unknown text alignment value '" + s + "'", false); }
                else { it.textalignment = i; }
            }
            return true;
        }
        case Art::Flag: {
            std::string s;
            if (!g.tok(s)) { return false; }
            if (gleichOhneFall(s, "WINDOW_INACTIVE")) { it.window.flags |= WINDOW_INACTIVE; }
            else if (d == Dialekt::MP) { g.warn("Unknown item style value '" + s + "'", false); }
            else { g.warn("Unknown item flag value '" + s + "'", false); }
            return true;
        }
        case Art::CvarFloat: {
            std::string c;
            if (g.str(c) && g.komma(it.defVal) && g.komma(it.minVal) && g.komma(it.maxVal)) {
                it.cvar = c;
                return true;
            }
            return false;
        }
        case Art::CvarRotate: {
            std::string c;
            float range = 0;
            if (g.str(c) && g.komma(range)) {
                it.cvar = c;
                it.maxVal = range;
                return true;
            }
            return false;
        }
        case Art::CvarStrList:
        case Art::CvarFloatList: {
            const bool strList = art == Art::CvarStrList;
            it.cvarList.clear();
            it.cvarStr.clear();
            it.cvarValue.clear();
            it.strDef = strList;
            std::string s;
            if (!g.tok(s)) { return false; }
            if (strList && gleichOhneFall(s, "feeder")) {
                // FEEDER_PLAYER_SPECIES (13) / FEEDER_LANGUAGES (34): Liste kommt aus dem Spiel
                it.multiFeeder = (it.special == 13.0F) ? "FEEDER_PLAYER_SPECIES"
                                 : (it.special == 34.0F) ? "FEEDER_LANGUAGES" : "feeder";
                if (it.special == 13.0F || it.special == 34.0F) { return true; }
            }
            if (s.empty() || s[0] != '{') { return false; }
            int pass = 0;
            while (true) {
                if (!g.str(s)) {
                    g.warn(d == Dialekt::MP ? "end of file inside menu item" : "end of file inside menu item\n", d == Dialekt::MP);
                    return false;
                }
                if (!s.empty() && s[0] == '}') { return true; }
                if (!s.empty() && (s[0] == ',' || s[0] == ';')) { continue; }
                if (strList) {
                    if (pass == 0) {
                        it.cvarList.push_back(s);
                        pass = 1;
                    } else {
                        it.cvarStr.push_back(s);
                        pass = 0;
                        if (it.cvarStr.size() >= 32) { return false; }   // MAX_MULTI_CVARS
                    }
                } else {
                    it.cvarList.push_back(s);
                    float v = 0;
                    if (!g.komma(v)) { return false; }
                    it.cvarValue.push_back(v);
                    if (it.cvarValue.size() >= 32) { return false; }
                }
            }
        }
        case Art::Columns: {
            int num = 0;
            if (!g.ganz(num)) { return false; }
            num = std::min(num, 16);   // MAX_LB_COLUMNS
            it.columns.clear();
            for (int i = 0; i < num; ++i) {
                Spalte sp;
                if (!(g.ganz(sp.pos) && g.ganz(sp.width) && g.ganz(sp.maxChars))) { return false; }
                it.columns.push_back(sp);
            }
            return true;
        }
        case Art::ColorRange: {
            ColorRange c;
            if (g.komma(c.low) && g.komma(c.high) && g.farbe(c.color)) {
                if (it.colorRanges.size() < 10) { it.colorRanges.push_back(c); }   // MAX_COLOR_RANGES
                return true;
            }
            return false;
        }
    }
    return false;
}

bool menuArg(Grund& g, const Schluessel& k, Dialekt d, Menu& m) {
    const std::string n = k.name;
    const Art art = (d == Dialekt::MP) ? k.mp : k.sp;
    switch (art) {
        case Art::Keine:
            if (n == "popup") { m.window.flags |= WINDOW_POPUP; }
            else if (n == "outOfBoundsClick") { m.window.flags |= WINDOW_OOB_CLICK; }
            return true;
        case Art::Str:
        case Art::Tok: {
            std::string s;
            if (!g.str(s)) { return false; }
            if (n == "name") { m.window.name = s; }
            else if (n == "font") { m.font = s; }
            else if (n == "cinematic") { m.window.cinematicName = s; }
            else if (n == "soundLoop") { m.soundName = s; }
            else if (n == "background") { m.window.background = s; }
            return true;
        }
        case Art::Int: {
            int v = 0;
            if (!g.ganz(v)) { return false; }
            if (n == "border") { m.window.border = v; }
            else if (n == "descTextStyle") { m.descTextStyle = v; }
            else if (n == "descX") { m.descX = v; }
            else if (n == "descY") { m.descY = v; }
            else if (n == "fadeCycle") { m.fadeCycle = v; }
            else if (n == "fullscreen") { m.fullScreen = v; }
            else if (n == "ownerdraw") { m.window.ownerDraw = v; }
            else if (n == "ownerdrawFlag") { m.window.ownerDrawFlags |= v; }
            else if (n == "ownerdrawID") { m.ownerdrawID = v; }
            else if (n == "style") { m.window.style = v; }
            else if (n == "descAlignment") { m.descAlignment = v; }
            else if (n == "visible") { if (v != 0) { m.window.flags |= WINDOW_VISIBLE; } }
            return true;
        }
        case Art::Float: {
            float v = 0;
            if (!g.komma(v)) { return false; }
            if (n == "appearanceIncrement") { m.appearanceIncrement = v; }
            else if (n == "borderSize") { m.window.borderSize = v; }
            else if (n == "descScale") { m.descScale = v; }
            else if (n == "fadeClamp") { m.fadeClamp = v; }
            else if (n == "fadeAmount") { m.fadeAmount = v; }
            return true;
        }
        case Art::Rect:
            return g.rechteck(m.window.rect);
        case Art::Color: {
            Farbe c;
            if (!g.farbe(c)) { return false; }
            if (n == "backcolor") { m.window.backColor = c; m.window.flags |= WINDOW_BACKCOLORSET; }
            else if (n == "forecolor") { m.window.foreColor = c; m.window.flags |= WINDOW_FORECOLORSET; }
            else if (n == "bordercolor") { m.window.borderColor = c; }
            else if (n == "focuscolor") { m.focusColor = c; }
            else if (n == "disablecolor") { m.disableColor = c; }
            else if (n == "desccolor") { m.descColor = c; }
            else if (n == "outlinecolor") { m.window.outlineColor = c; }
            return true;
        }
        case Art::Script: {
            std::string s;
            if (!g.skript(s)) { return false; }
            if (n == "onOpen") { m.onOpen = s; }
            else if (n == "onClose") { m.onClose = s; }
            else if (n == "onAccept") { m.onAccept = s; }
            else if (n == "onESC") { m.onESC = s; }
            return true;
        }
        case Art::Name: {
            std::string s;
            if (!g.str(s)) { return false; }
            if (n == "style") {
                const int i = nameIndex(kStyles, s);
                if (i < 0) { g.warn("Unknown menu style value '" + s + "'", false); }
                else { m.window.style = i; }
            } else if (n == "descAlignment") {
                const int i = nameIndex(kAlign, s);
                if (i < 0) { g.warn("Unknown desc alignment value '" + s + "'", false); }
                else { m.descAlignment = i; }
            }
            return true;
        }
        default:
            return false;
    }
}

// ============================================================================
// Item_Parse / Menu_Parse
// ============================================================================

class Leser {
public:
    Leser(Grund& g, Datei& d) : g_(g), d_(d) {}

    // Item_Parse (nach "itemDef")
    bool item(Item& it) {
        Token t;
        if (!g_.schluesselwort(t, true)) { return false; }
        if (t.s.empty() || t.s[0] != '{') { return false; }
        while (true) {
            if (!g_.schluesselwort(t, true)) {
                g_.warn(mp() ? "end of file inside menu item" : "End of file inside menu item", mp());
                return false;
            }
            if (!t.s.empty() && t.s[0] == '}') {
                it.ende = t.ende;
                return true;
            }
            const Schluessel* k = sucheIn(kItem, t.s, d_.dialekt, d_.spiel);
            Eigenschaft e;
            e.schluessel = t.s;
            e.kopf = t;
            if (k == nullptr) {
                g_.warn(mp() ? "unknown menu item keyword " + t.s : "Unknown item keyword '" + t.s + "'", mp());
                e.ok = false;
                it.eigenschaften.push_back(e);
                continue;
            }
            e.kanon = k->name;
            g_.aufz = &e.args;
            const bool ok = itemArg(g_, *k, d_.dialekt, it);
            g_.aufz = nullptr;
            e.ok = ok;
            it.eigenschaften.push_back(e);
            if (!ok) {
                g_.warn(mp() ? "couldn't parse menu item keyword " + t.s : "Couldn't parse item keyword '" + t.s + "'",
                        mp());
                return false;
            }
        }
    }

    // Menu_Parse (nach "menuDef")
    bool menu(Menu& m) {
        Token t;
        if (mp()) {
            if (!g_.schluesselwort(t, false)) { return false; }
            if (t.s.empty() || t.s[0] != '{') { return false; }
        } else {
            g_.schluesselwort(t, false);
            if (t.s.empty() || t.s[0] != '{') {
                g_.warn("Misplaced {", false);
                return false;
            }
        }
        while (true) {
            const bool gelesen = g_.schluesselwort(t, false);
            if (!gelesen || (!mp() && t.s.empty() && t.typ != TT_STRING)) {
                g_.warn(mp() ? "end of file inside menu" : "End of file inside menu.", mp());
                return false;
            }
            if (!t.s.empty() && t.s[0] == '}') {
                m.ende = t.ende;
                return true;
            }
            const Schluessel* k = sucheIn(kMenu, t.s, d_.dialekt, d_.spiel);
            if (k == nullptr) {
                g_.warn(mp() ? "unknown menu keyword " + t.s : "Unknown menu keyword " + t.s, mp());
                Eigenschaft e;
                e.schluessel = t.s;
                e.kopf = t;
                e.ok = false;
                m.eigenschaften.push_back(e);
                continue;
            }
            if (std::strcmp(k->name, "itemDef") == 0) {
                // MenuParse_itemDef
                Item it;
                it.kopf = t;
                it.anfang = t.anfang;
                it.zeile = t.zeile;
                const bool ok = item(it);
                it.ok = ok;
                if (!ok) {
                    if (it.ende < it.anfang) { it.ende = it.anfang; }
                    m.items.push_back(std::move(it));
                    g_.warn(mp() ? "couldn't parse menu keyword " + t.s : "Couldn't parse menu keyword " + t.s + " as " + k->name,
                            mp());
                    return false;
                }
                m.items.push_back(std::move(it));
                continue;
            }
            Eigenschaft e;
            e.schluessel = t.s;
            e.kanon = k->name;
            e.kopf = t;
            g_.aufz = &e.args;
            const bool ok = menuArg(g_, *k, d_.dialekt, m);
            g_.aufz = nullptr;
            e.ok = ok;
            m.eigenschaften.push_back(e);
            if (!ok) {
                g_.warn(mp() ? "couldn't parse menu keyword " + t.s : "Couldn't parse menu keyword " + t.s + " as " + k->name,
                        mp());
                return false;
            }
        }
    }

    // Asset_Parse: die Werte der Schluessel merken (Schriften, Klaenge ...)
    bool assets() {
        Token t;
        if (!g_.schluesselwort(t, false) || t.s != "{") { return false; }
        while (true) {
            if (!g_.schluesselwort(t, false) || (t.s.empty() && t.typ != TT_STRING)) { return false; }
            if (t.s == "}") {
                d_.assets.ende = t.ende;
                return true;
            }
            Eigenschaft e;
            e.schluessel = t.s;
            e.kopf = t;
            g_.aufz = &e.args;
            bool ok = true;
            const char* const schrift[] = {"font", "smallFont", "small2Font", "mediumFont", "bigFont"};
            const char* const zahl[] = {"fadeClamp", "fadeAmount", "shadowX", "shadowY", "fadeCycle"};
            bool bekannt = false;
            for (const char* s : schrift) {
                if (gleichOhneFall(t.s, s)) {
                    std::string n;
                    int p = 0;
                    ok = g_.str(n) && g_.ganz(p);
                    bekannt = true;
                }
            }
            for (const char* s : zahl) {
                if (!bekannt && gleichOhneFall(t.s, s)) {
                    float f = 0;
                    ok = g_.komma(f);
                    bekannt = true;
                }
            }
            if (!bekannt && (gleichOhneFall(t.s, "shadowColor") || gleichOhneFall(t.s, "focuscolor"))) {
                Farbe c;
                ok = g_.farbe(c);
                bekannt = true;
            }
            if (!bekannt) {
                std::string s;
                ok = g_.str(s);
            }
            g_.aufz = nullptr;
            e.ok = ok;
            d_.assets.eigenschaften.push_back(e);
            if (!ok) { return false; }
        }
    }

    // Load_Menu: loadMenu { "a.menu" "b.menu" }
    bool loadMenu() {
        Token t;
        if (!g_.schluesselwort(t, false) || t.s.empty() || t.s[0] != '{') { return false; }
        while (true) {
            if (!g_.schluesselwort(t, false) || t.s.empty()) { return false; }
            if (t.s[0] == '}') { return true; }
            d_.loadMenus.push_back(t.s);
        }
    }

private:
    bool mp() const { return d_.dialekt == Dialekt::MP; }
    Grund& g_;
    Datei& d_;
};

// Menu_PostParse + Menu_UpdatePosition + Item_SetScreenCoords
void nachLesen(Menu& m) {
    if (m.fullScreen != 0) { m.window.rect = Rechteck{0, 0, 640, 480}; }
    float x = m.window.rect.x;
    float y = m.window.rect.y;
    if (m.window.border != 0) {
        x += m.window.borderSize;
        y += m.window.borderSize;
    }
    for (Item& it : m.items) {
        float ix = x;
        float iy = y;
        if (it.window.border != 0) {
            ix += it.window.borderSize;
            iy += it.window.borderSize;
        }
        it.window.rect.x = ix + it.window.rectClient.x;
        it.window.rect.y = iy + it.window.rectClient.y;
        it.window.rect.w = it.window.rectClient.w;
        it.window.rect.h = it.window.rectClient.h;
    }
}

}  // namespace

// ============================================================================
// UI_ParseMenu / UI_LoadMenus
// ============================================================================

Datei lies(const std::string& pfad, const std::string& text, const LeseOptionen& opt) {
    Datei d;
    d.pfad = pfad;
    d.text = text;
    d.dialekt = opt.dialekt;
    d.spiel = opt.spiel;
    d.quellen.push_back(Quelle{pfad, std::string()});   // [0]: Text steht in d.text

    const bool istMenu = pfad.size() >= 5 && gleichOhneFall(pfad.substr(pfad.size() - 5), ".menu");

    auto menuNeu = [&](Leser& l, const Token& kopf) {
        Menu m;
        m.kopf = kopf;
        m.anfang = kopf.anfang;
        m.zeile = kopf.zeile;
        m.ok = l.menu(m);
        if (m.ok) { nachLesen(m); }
        if (m.ende < m.anfang) {
            m.ende = m.items.empty() ? m.anfang : std::max(m.anfang, m.items.back().ende);
        }
        d.menus.push_back(std::move(m));
    };

    if (opt.dialekt == Dialekt::SP) {
        SpGrund g(d.text, d);
        Leser l(g, d);
        while (true) {
            Token t = g.lex_.parseExt(true);
            if (t.s.empty()) { break; }
            if (t.s[0] == '{') { continue; }
            if (t.s[0] == '}') { break; }
            if (gleichOhneFall(t.s, "assetGlobalDef")) {
                d.assets.vorhanden = true;
                d.assets.anfang = t.anfang;
                if (l.assets()) { continue; }
                break;
            }
            if (gleichOhneFall(t.s, "menudef")) {
                menuNeu(l, t);
                continue;
            }
            if (gleichOhneFall(t.s, "loadmenu") && !istMenu) {
                if (l.loadMenu()) { continue; }
                break;
            }
            g.warn("Invalid keyword '" + t.s + "'", false);
        }
        return d;
    }

    // MP: botlib mit den globalen Defines (UI_LoadMenus: PC_LoadGlobalDefines)
    // Die Hauptdatei steht als Quelle 0 mit Text, damit botlib sie lesen kann.
    d.quellen[0].text = text;
    PcQuelle q(opt.lader, d.quellen, d.meldungen);
    if (!opt.globaleDefines.empty()) {
        if (!q.ladeGlobal(opt.globaleDefines)) {
            d.meldungen.push_back(Meldung{0, 0, "global defines " + opt.globaleDefines + " not found", false});
        }
    }
    for (const auto& [name, def] : q.globale) {
        if (def.tokens.size() == 1 && def.parms.empty() && def.tokens[0].typ == TT_NUMBER) {
            d.defines[name] = static_cast<long>(def.tokens[0].ival);
        }
    }
    q.oeffne(0);
    MpGrund g(q, d);
    Leser l(g, d);
    while (true) {
        Token t;
        if (!q.lies(t)) { break; }
        if (!t.s.empty() && t.s[0] == '}') { break; }
        if (gleichOhneFall(t.s, "assetGlobalDef")) {
            d.assets.vorhanden = true;
            d.assets.anfang = t.anfang;
            if (l.assets()) { continue; }
            break;
        }
        if (gleichOhneFall(t.s, "menudef")) {
            menuNeu(l, t);
            continue;
        }
        if (gleichOhneFall(t.s, "loadmenu") && !istMenu) {
            if (l.loadMenu()) { continue; }
            break;
        }
    }
    d.quellen[0].text.clear();   // der Text steht in d.text
    return d;
}

const std::string& schreib(const Datei& d) noexcept { return d.text; }

LeseOptionen optionenFuer(const std::string& pfad, Spiel spiel, Lader lader) {
    LeseOptionen o;
    o.spiel = spiel;
    o.lader = std::move(lader);
    std::string p;
    for (const char c : pfad) { p += (c == '\\') ? '/' : static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    const std::size_t ui = p.rfind("ui/");
    if (ui != std::string::npos) {
        const std::string rest = p.substr(ui + 3);
        const std::size_t slash = rest.find('/');
        if (slash != std::string::npos) {
            const std::string ordner = rest.substr(0, slash);
            if (ordner == "jamp" || ordner == "jk2mp" || ordner == "md_mp") {
                o.dialekt = Dialekt::MP;
                // Gross/Klein wie im Archiv: ui/MD_MP/menudef.h
                const std::string orig = pfad.substr(ui + 3, slash);
                o.globaleDefines = "ui/" + orig + "/menudef.h";
            }
        }
    }
    return o;
}

const char* typName(int typ) noexcept {
    return (typ >= 0 && typ < 15) ? kTypes[typ] : "";
}
const char* styleName(int style) noexcept {
    return (style >= 0 && style < 6) ? kStyles[style] : "";
}

std::vector<std::string> menuSchluessel(Dialekt d, Spiel s) {
    std::vector<std::string> v;
    for (const Schluessel& k : kMenu) {
        if (gilt(k, d, s)) { v.emplace_back(k.name); }
    }
    return v;
}
std::vector<std::string> itemSchluessel(Dialekt d, Spiel s) {
    std::vector<std::string> v;
    for (const Schluessel& k : kItem) {
        if (gilt(k, d, s)) { v.emplace_back(k.name); }
    }
    return v;
}

}  // namespace bhed::menue
