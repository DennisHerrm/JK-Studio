// vorschau.cpp - Menue-Editor M1 Punkt 2: ein Menue zeichnen wie die Engine.
//
// Portiert aus OpenJK: codemp/ui/ui_shared.c (Menu_Paint, Item_Paint,
// Window_Paint, ToWindowCoords, Item_SetTextExtents, Item_TextColor,
// LerpColor, Item_Text_Paint, Item_Text_Wrapped_Paint,
// Item_Text_AutoWrapped_Paint, Item_TextField_Paint, Item_YesNo_Paint,
// Item_Multi_Setting, Item_Multi_Paint, Item_Slider_ThumbPosition,
// Item_Slider_Paint, Item_ListBox_Paint (senkrecht), Item_EnableShowViaCvar),
// codemp/ui/ui_main.c (Text_Paint, Text_Width, Text_Height,
// MenuFontToHandle), code/ui/ui_main.cpp (Text_Paint der SP-Fassung,
// Asset_Parse-Reihenfolge der Schriften) und code/rd-common/tr_font.cpp
// (CFontInfo, RE_Font_StrLenPixels, RE_Font_HeightPixels,
// RE_Font_DrawString; M3: mit den asiatischen Zweigen, siehe kodierung.h).
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
#include "bhed/menue/vorschau.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>

namespace bhed::menue {

namespace {

constexpr int kGlyphs = 256;
constexpr std::size_t kFontdatGroesse = 256 * 28 + 5 * 2 + 2;   // sizeof(dfontdat_t) = 7180 (auf 4 aufgefuellt)
constexpr float kPulseDivisor = 75.0F;                        // PULSE_DIVISOR
constexpr int kBlinkDivisor = 200;                            // BLINK_DIVISOR
constexpr int ITEM_TEXTSTYLE_BLINK = 1;
constexpr int ITEM_TEXTSTYLE_PULSE = 2;
constexpr int ITEM_TEXTSTYLE_SHADOWED = 3;
constexpr int ITEM_TEXTSTYLE_SHADOWEDMORE = 6;

// g_color_table (OpenJK q_shared.c): ^0 .. ^9
const Farbe kFarbtabelle[10] = {{0, 0, 0, 1}, {1, 0, 0, 1}, {0, 1, 0, 1}, {1, 1, 0, 1}, {0, 0, 1, 1},
                                {0, 1, 1, 1}, {1, 0, 1, 1}, {1, 1, 1, 1}, {1, 0.5F, 0, 1}, {0.5F, 0.5F, 0.5F, 1}};

int runden(float v) { return static_cast<int>(std::lround(v)); }

template <typename T>
T lese(const std::string& d, std::size_t& p) {
    T v{};
    std::memcpy(&v, d.data() + p, sizeof(T));
    p += sizeof(T);
    return v;
}

bool gleichOhneFall(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// String_Parse (COM_ParseExt ohne Zeilenumbrueche) ueber einen Skripttext
bool stringParse(const std::string& s, std::size_t& p, std::string& out) {
    while (p < s.size() && static_cast<unsigned char>(s[p]) <= ' ') {
        if (s[p] == '\n') { return false; }
        ++p;
    }
    if (p >= s.size()) { return false; }
    out.clear();
    if (s[p] == '"') {
        ++p;
        while (p < s.size() && s[p] != '"') { out += s[p++]; }
        if (p < s.size()) { ++p; }
        return true;
    }
    while (p < s.size() && static_cast<signed char>(s[p]) > 32) { out += s[p++]; }
    return true;
}

class Maler {
public:
    Maler(const Datei& d, const Menu& m, const Umgebung& u, std::vector<Prim>& out) : d_(d), m_(m), u_(u), out_(out) {}

    void menu() {
        // Menu_Paint
        if (m_.window.ownerDrawFlags != 0 && u_.ownerDrawSichtbar && !u_.ownerDrawSichtbar(m_.window.ownerDrawFlags)) {
            return;
        }
        if (m_.fullScreen != 0) {
            if (!m_.window.background.empty()) {
                bild(0, 0, kSchirmBreite, kSchirmHoehe, m_.window.background, Farbe{1, 1, 1, 1}, -1);
            }
        } else if (!m_.window.background.empty()) {
            fenster(m_.window, -1);
        }
        for (std::size_t i = 0; i < m_.items.size(); ++i) {
            item_ = static_cast<int>(i);
            item(m_.items[i]);
        }
        item_ = -1;
    }

    // Text_Width / Text_Height
    int textBreite(const std::string& t, float scale, int font) const {
        const Schrift* f = schrift(font);
        return f != nullptr ? schriftBreite(*f, t, scale, u_.kodierung, u_.asienGlyphen) : 0;
    }
    int textHoehe(float scale, int font) const {
        const Schrift* f = schrift(font);
        return f != nullptr ? schriftHoehe(*f, scale) : 0;
    }

    std::string cvar(const std::string& n) const { return u_.cvar ? u_.cvar(n) : std::string(); }
    float cvarWert(const std::string& n) const { return static_cast<float>(std::atof(cvar(n).c_str())); }
    std::string se(const std::string& t) const { return u_.texte != nullptr ? u_.texte->aufloesen(t) : t; }

    // Item_SetTextExtents
    Rechteck textRect(const Item& it, const std::string& text) const {
        Rechteck r;
        const int originalWidth = textBreite(text, it.textscale, it.font);
        r.w = static_cast<float>(originalWidth);
        r.h = static_cast<float>(textHoehe(it.textscale, it.font));
        r.x = it.textalignx;
        r.y = it.textaligny;
        if (it.textalignment == ITEM_ALIGN_RIGHT) { r.x = it.textalignx - static_cast<float>(originalWidth); }
        else if (it.textalignment == ITEM_ALIGN_CENTER) { r.x = it.textalignx - static_cast<float>(originalWidth / 2); }
        zuFenster(r.x, r.y, it.window);
        return r;
    }

    float daumen(const Item& it) const {   // Item_Slider_ThumbPosition
        float x = it.window.rect.x;
        if (!it.text.empty()) {
            const Rechteck tr = textRect(it, se(it.text));
            x = tr.x + tr.w + 8;
        }
        if (!it.typeData || it.cvar.empty()) { return x; }
        float value = cvarWert(it.cvar);
        if (value < it.minVal) { value = it.minVal; }
        else if (value > it.maxVal) { value = it.maxVal; }
        const float range = it.maxVal - it.minVal;
        value -= it.minVal;
        value /= range;
        value *= kSliderBreite;
        return x + value;
    }

private:
    const Schrift* schrift(int font) const { return u_.schrift ? u_.schrift(font) : nullptr; }

    static void zuFenster(float& x, float& y, const Fenster& w) {   // ToWindowCoords
        if (w.border != 0) {
            x += w.borderSize;
            y += w.borderSize;
        }
        x += w.rect.x;
        y += w.rect.y;
    }

    void fuellen(float x, float y, float w, float h, const Farbe& c) {
        Prim p;
        p.art = PrimArt::Fuellen;
        p.x = x; p.y = y; p.w = w; p.h = h;
        p.farbe = gedimmt(c);
        p.item = item_;
        out_.push_back(p);
    }
    void bild(float x, float y, float w, float h, const std::string& shader, const Farbe& c, int item,
              float s0 = 0, float t0 = 0, float s1 = 1, float t1 = 1) {
        Prim p;
        p.art = PrimArt::Bild;
        p.x = x; p.y = y; p.w = w; p.h = h;
        p.s0 = s0; p.t0 = t0; p.s1 = s1; p.t1 = t1;
        p.farbe = gedimmt(c);
        p.shader = shader;
        p.item = item;
        out_.push_back(p);
    }
    void platzhalter(const Rechteck& r, const std::string& name, PrimArt art = PrimArt::Platzhalter) {
        Prim p;
        p.art = art;
        p.x = r.x; p.y = r.y; p.w = r.w; p.h = r.h;
        p.shader = name;
        p.item = item_;
        p.farbe = gedimmt(Farbe{1, 1, 1, 1});
        out_.push_back(p);
    }
    Farbe gedimmt(Farbe c) const {
        if (dimmen_) { c.a *= 0.35F; }
        return c;
    }

    // DC->drawRect (UI_DrawRect: drawSides + drawTopBottom)
    void rahmen(float x, float y, float w, float h, float size, const Farbe& c) {
        fuellen(x, y, size, h, c);
        fuellen(x + w - size, y, size, h, c);
        fuellen(x, y, w, size, c);
        fuellen(x, y + h - size, w, size, c);
    }

    // Window_Paint
    void fenster(const Fenster& w, int item) {
        if (w.style == 0 && w.border == 0) { return; }
        Rechteck f = w.rect;
        if (w.border != 0) {
            f.x += w.borderSize;
            f.y += w.borderSize;
            f.w -= w.borderSize + 1;
            f.h -= w.borderSize + 1;
        }
        if (w.style == WINDOW_STYLE_FILLED) {
            if (!w.background.empty()) {
                bild(f.x, f.y, f.w, f.h, w.background, w.backColor, item);
            } else {
                fuellen(f.x, f.y, f.w, f.h, w.backColor);
            }
        } else if (w.style == WINDOW_STYLE_GRADIENT) {
            bild(f.x, f.y, f.w, f.h, "ui/assets/gradientbar2.tga", w.backColor, item);   // GradientBar_Paint
        } else if (w.style == WINDOW_STYLE_SHADER) {
            const Farbe c = (w.flags & WINDOW_FORECOLORSET) != 0 ? w.foreColor : Farbe{1, 1, 1, 1};
            if (!w.background.empty()) { bild(f.x, f.y, f.w, f.h, w.background, c, item); }
        } else if (w.style == WINDOW_STYLE_TEAMCOLOR) {
            fuellen(f.x, f.y, f.w, f.h, Farbe{1, 0, 0, 1});   // DC->getTeamColor: ohne Spiel rot
        } else if (w.style == WINDOW_STYLE_CINEMATIC) {
            platzhalter(f, w.cinematicName, PrimArt::Kino);
        }
        if (w.border == WINDOW_BORDER_FULL) {
            rahmen(w.rect.x, w.rect.y, w.rect.w, w.rect.h, w.borderSize, w.borderColor);
        } else if (w.border == WINDOW_BORDER_HORZ) {   // drawTopBottom
            fuellen(w.rect.x, w.rect.y, w.rect.w, w.borderSize, w.borderColor);
            fuellen(w.rect.x, w.rect.y + w.rect.h - w.borderSize, w.rect.w, w.borderSize, w.borderColor);
        } else if (w.border == WINDOW_BORDER_VERT) {   // drawSides
            fuellen(w.rect.x, w.rect.y, w.borderSize, w.rect.h, w.borderColor);
            fuellen(w.rect.x + w.rect.w - w.borderSize, w.rect.y, w.borderSize, w.rect.h, w.borderColor);
        }
    }

    static Farbe lerp(const Farbe& a, const Farbe& b, float t) {   // LerpColor
        return Farbe{a.r + t * (b.r - a.r), a.g + t * (b.g - a.g), a.b + t * (b.b - a.b), a.a + t * (b.a - a.a)};
    }
    static Farbe mal(const Farbe& a, float f) { return Farbe{a.r * f, a.g * f, a.b * f, a.a * f}; }

    // Item_TextColor
    Farbe textFarbe(const Item& it, int flags) const {
        const float puls = 0.5F + 0.5F * std::sin(static_cast<float>(u_.zeitMs) / kPulseDivisor);
        Farbe c;
        if ((flags & WINDOW_HASFOCUS) != 0) {
            c = lerp(m_.focusColor, mal(m_.focusColor, 0.8F), puls);
        } else if (it.textStyle == ITEM_TEXTSTYLE_BLINK && ((u_.zeitMs / kBlinkDivisor) & 1) == 0) {
            c = lerp(it.window.foreColor, mal(it.window.foreColor, 0.8F), puls);
        } else {
            c = it.window.foreColor;
        }
        if (!it.enableCvar.empty() && !it.cvarTest.empty()) {
            if ((it.cvarFlags & (CVAR_ENABLE | CVAR_DISABLE)) != 0 && !itemZeigtUeberCvar(it, CVAR_ENABLE, u_)) {
                c = m_.disableColor;
            }
        }
        return c;
    }

    // Text_Paint -> RE_Font_DrawString
    void text(float x, float y, float scale, const Farbe& c, const std::string& t, int maxPixel, int style, int font) {
        const Schrift* f = schrift(font);
        if (f == nullptr || !f->ok) { return; }
        bool blink = false;
        bool schatten = false;
        if (u_.dialekt == Dialekt::MP) {
            blink = style == ITEM_TEXTSTYLE_BLINK || style == ITEM_TEXTSTYLE_PULSE;
        } else {
            blink = style == ITEM_TEXTSTYLE_PULSE;
        }
        schatten = style >= ITEM_TEXTSTYLE_SHADOWED && style <= ITEM_TEXTSTYLE_SHADOWEDMORE;
        if (blink && ((u_.zeitMs >> 7) & 1) != 0) { return; }
        maxPixel = (maxPixel == 0) ? -1 : maxPixel;   // Text_Paint: !limit ? -1 : limit
        // int ox, int oy: die Engine schneidet die Nachkommastellen ab
        const int ox = static_cast<int>(x);
        const int oy = static_cast<int>(y);
        if (schatten) {
            const int off = runden(static_cast<float>(f->pointSize) * scale * 0.075F);
            zeichenkette(*f, ox + off, oy + off, t, Farbe{0.15F, 0.15F, 0.15F, c.a}, maxPixel, scale, true);
        }
        zeichenkette(*f, ox, oy, t, c, maxPixel, scale, false);
    }

    void zeichenkette(const Schrift& f, int ox, int oy, const std::string& t, Farbe c, int maxPixel, float scale,
                      bool imSchatten) {
        // fScaleAsian / fAsianYAdjust (Language_IsAsian)
        const bool asien = asiatisch(u_.kodierung);
        const float scaleAsien = (asien && scale > 0.7F) ? scale * 0.75F : scale;
        const float yAsien = (asien && scale > 0.7F)
                                 ? ((static_cast<float>(f.pointSize) * scale) - (static_cast<float>(f.pointSize) * scaleAsien)) / 2.0F
                                 : 0.0F;
        const float fox = static_cast<float>(ox);
        float foy = static_cast<float>(oy) + static_cast<float>(f.height - (f.descender >> 1)) * scale;
        float fx = fox;
        bool ueberlauf = false;
        const auto* p = reinterpret_cast<const unsigned char*>(t.data());
        for (std::size_t i = 0; i < t.size() && !ueberlauf;) {
            int n = 1;
            const unsigned ch = zeichenLesen(u_.kodierung, p + i, t.size() - i, n);
            i += static_cast<std::size_t>(n);
            if (ch == 10) {
                fx = fox;
                foy += static_cast<float>(f.pointSize) * scale;
                if (asien) { foy += 4.0F; }
            } else if (ch == 13) {
            } else if (ch == 32) {
                fx += static_cast<float>(f.glyph[' '].horizAdvance) * scale;
                ueberlauf = maxPixel != -1 && (fx - fox) > static_cast<float>(maxPixel);
            } else if (ch == '^' && i < t.size() && t[i] >= '0' && t[i] <= '9') {
                const int idx = t[i] - '0';
                ++i;
                if (!imSchatten) {
                    const float a = c.a;
                    c = kFarbtabelle[idx];
                    c.a = a;
                }
            } else {
                // GetLetter(uiLetter, &hShader): asiatische Glyphe oder mGlyphs[uiLetter & 0xff]
                Glyph asienG;
                std::string shader = f.shader;
                const Glyph* g = &f.glyph[ch & 0xFFU];
                if (u_.asienGlyphen && asienGlyph(u_.kodierung, ch, f, asienG, shader)) { g = &asienG; }
                if (g->width == 0) {
                    g = &f.glyph['.'];
                    shader = f.shader;
                }
                const float sc = ch > 255U ? scaleAsien : scale;   // g_iNonScaledCharRange = 255
                const float adv = static_cast<float>(g->horizAdvance) * sc;
                ueberlauf = maxPixel != -1 && ((fx + adv) - fox) > static_cast<float>(maxPixel);
                if (!ueberlauf) {
                    const float fy = foy - static_cast<float>(g->baseline) * sc;
                    bild(fx + static_cast<float>(g->horizOffset) * sc, ch > 255U ? fy - yAsien : fy, static_cast<float>(g->width) * sc,
                         static_cast<float>(g->height) * sc, shader, c, item_, g->s, g->t, g->s2, g->t2);
                }
                fx += adv;
            }
        }
    }

    // Der angezeigte Text eines Items: text, sonst der Wert von cvar.
    std::string itemText(const Item& it) const {
        if (it.text.empty()) { return it.cvar.empty() ? std::string() : cvar(it.cvar); }
        return se(it.text);
    }

    void textMalen(const Item& it, int flags) {   // Item_Text_Paint
        if ((it.window.flags & WINDOW_WRAPPED) != 0) {
            umgebrochen(it, flags);
            return;
        }
        if ((it.window.flags & WINDOW_AUTOWRAPPED) != 0) {
            autoUmbruch(it, flags);
            return;
        }
        if (it.text.empty() && it.cvar.empty()) { return; }
        const std::string t = itemText(it);
        tr_ = textRect(it, t);
        if (t.empty()) { return; }
        const Farbe c = textFarbe(it, flags);
        text(tr_.x, tr_.y, it.textscale, c, t, 0, it.textStyle, it.font);
        if (!it.text2.empty()) {
            text(tr_.x + it.text2alignx, tr_.y + it.text2aligny, it.textscale, c, se(it.text2), 0, it.textStyle, it.font);
        }
    }

    void umgebrochen(const Item& it, int flags) {   // Item_Text_Wrapped_Paint
        if (it.text.empty() && it.cvar.empty()) { return; }
        const std::string t = itemText(it);
        if (t.empty()) { return; }
        const Farbe c = textFarbe(it, flags);
        tr_ = textRect(it, t);
        const float hoehe = tr_.h;
        float y = tr_.y;
        std::size_t start = 0;
        std::size_t p = t.find('\r');
        while (p != std::string::npos) {
            text(tr_.x, y, it.textscale, c, t.substr(start, p - start), 0, it.textStyle, it.font);
            y += hoehe + 2;
            start = p + 1;
            p = t.find('\r', p + 1);
        }
        text(tr_.x, y, it.textscale, c, t.substr(start), 0, it.textStyle, it.font);
    }

    void autoUmbruch(const Item& it, int flags) {   // Item_Text_AutoWrapped_Paint
        if (it.text.empty() && it.cvar.empty()) { return; }
        const std::string t = itemText(it);
        if (t.empty()) { return; }
        const Farbe c = textFarbe(it, flags);
        const int hoehe = textHoehe(it.textscale, it.font);
        float y = it.textaligny;
        std::string buff;
        std::size_t newLine = 0;
        std::size_t newLinePtr = 0;
        int newLineWidth = 0;
        int textWidth = 0;
        std::size_t p = 0;
        while (true) {
            const char ch = p < t.size() ? t[p] : '\0';
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\0') {
                newLine = buff.size();
                newLinePtr = p + 1;
                newLineWidth = textWidth;
            }
            textWidth = textBreite(buff, it.textscale, 0);
            if ((newLine != 0 && static_cast<float>(textWidth) > it.window.rect.w) || ch == '\n' || ch == '\0') {
                if (!buff.empty()) {
                    float x = it.textalignx;
                    if (it.textalignment == ITEM_ALIGN_RIGHT) { x = it.textalignx - static_cast<float>(newLineWidth); }
                    else if (it.textalignment == ITEM_ALIGN_CENTER) { x = it.textalignx - static_cast<float>(newLineWidth / 2); }
                    float yy = y;
                    zuFenster(x, yy, it.window);
                    text(x, yy, it.textscale, c, buff.substr(0, newLine), 0, it.textStyle, it.font);
                }
                if (ch == '\0') { break; }
                y += static_cast<float>(hoehe + 5);
                p = newLinePtr;
                buff.clear();
                newLine = 0;
                newLineWidth = 0;
                continue;
            }
            buff += t[p++];
        }
    }

    void item(const Item& it) {
        int flags = it.window.flags;
        if (static_cast<std::size_t>(item_) < u_.zustand.size() && u_.zustand[static_cast<std::size_t>(item_)].gueltig) {
            flags = u_.zustand[static_cast<std::size_t>(item_)].flags;
        }
        dimmen_ = false;
        if (!itemSichtbar(it, flags, u_)) {
            if (!u_.alleItems) { return; }
            dimmen_ = true;
        }
        tr_ = Rechteck{};
        if ((flags & WINDOW_MOUSEOVER) != 0 && !it.descText.empty()) { beschreibung(it, flags); }
        Fenster w = it.window;
        w.flags = flags;
        fenster(w, item_);
        switch (it.type) {
            case ITEM_TYPE_OWNERDRAW:
                platzhalter(it.window.rect, "ownerdraw " + std::to_string(it.window.ownerDraw));
                break;
            case ITEM_TYPE_TEXT:
            case ITEM_TYPE_BUTTON:
                textMalen(it, flags);
                break;
            case ITEM_TYPE_EDITFIELD:
            case ITEM_TYPE_NUMERICFIELD: {   // Item_TextField_Paint
                textMalen(it, flags);
                std::string buff = it.cvar.empty() ? std::string() : se(cvar(it.cvar));
                const Farbe c = (flags & WINDOW_HASFOCUS) != 0
                                    ? lerp(m_.focusColor, mal(m_.focusColor, 0.8F),
                                           0.5F + 0.5F * std::sin(static_cast<float>(u_.zeitMs) / kPulseDivisor))
                                    : it.window.foreColor;
                const float offset = (!it.text.empty()) ? 8.0F : 0.0F;
                text(tr_.x + tr_.w + offset, tr_.y, it.textscale, c, buff, static_cast<int>(it.window.rect.w), it.textStyle,
                     it.font);
                break;
            }
            case ITEM_TYPE_LISTBOX:
                aktFlags_ = flags;
                listbox(it);
                break;
            case ITEM_TYPE_TEXTSCROLL:
                platzhalter(it.window.rect, "textscroll");
                break;
            case ITEM_TYPE_MODEL:
                platzhalter(it.window.rect, it.assetModel.empty() ? std::string("model") : it.assetModel, PrimArt::Modell);
                break;
            case ITEM_TYPE_YESNO: {   // Item_YesNo_Paint
                const float value = it.cvar.empty() ? 0.0F : cvarWert(it.cvar);
                const std::string ja = se("@MENUS_YES");
                const std::string nein = se("@MENUS_NO");
                const std::string wert = (it.invertYesNo != 0) ? (value == 0.0F ? ja : nein) : (value != 0.0F ? ja : nein);
                const Farbe c = textFarbe(it, flags);
                if (!it.text.empty()) {
                    textMalen(it, flags);
                    text(tr_.x + tr_.w + 8, tr_.y, it.textscale, c, wert, 0, it.textStyle, it.font);
                } else {
                    text(tr_.x, tr_.y, it.textscale, c, wert, 0, it.textStyle, it.font);
                }
                break;
            }
            case ITEM_TYPE_MULTI: {   // Item_Multi_Paint
                std::string t = multiEinstellung(it);
                if (!t.empty() && t[0] == '@') { t = se(t); }
                else if (!t.empty() && t[0] == '*') { t = cvar(t.substr(1)); }
                const Farbe c = textFarbe(it, flags);
                if (!it.text.empty()) {
                    textMalen(it, flags);
                    text(tr_.x + tr_.w + 8, tr_.y, it.textscale, c, t, 0, it.textStyle, it.font);
                } else {
                    text(tr_.x + static_cast<float>(it.xoffset), tr_.y, it.textscale, c, t, 0, it.textStyle, it.font);
                }
                break;
            }
            case ITEM_TYPE_BIND: {   // Item_Bind_Paint: die Taste kommt aus dem Spiel
                textMalen(it, flags);
                const Farbe c = textFarbe(it, flags);
                text(tr_.x + tr_.w + 8, tr_.y, it.textscale, c, "???", 0, it.textStyle, it.font);
                break;
            }
            case ITEM_TYPE_SLIDER:
            case ITEM_TYPE_SLIDER_INTEGER: {   // Item_Slider_Paint (Movie Duels auch fuer SLIDER_INTEGER)
                const Farbe c = (flags & WINDOW_HASFOCUS) != 0
                                    ? lerp(m_.focusColor, mal(m_.focusColor, 0.8F),
                                           0.5F + 0.5F * std::sin(static_cast<float>(u_.zeitMs) / kPulseDivisor))
                                    : it.window.foreColor;
                const float y = it.window.rect.y;
                float x = it.window.rect.x;
                if (!it.text.empty()) {
                    textMalen(it, flags);
                    x = tr_.x + tr_.w + 8;
                }
                bild(x, y, kSliderBreite, kSliderHoehe, "menu/new/slider", c, item_);
                const float dx = daumen(it);
                bild(dx - kSliderDaumenBreite / 2, y - 2, kSliderDaumenBreite, kSliderDaumenHoehe, "menu/new/sliderthumb", c, item_);
                break;
            }
            default:
                break;
        }
    }

    std::string multiEinstellung(const Item& it) const { return multiText(it, u_); }   // Item_Multi_Setting

    void beschreibung(const Item& it, int flags) {
        const std::string t = se(it.descText);
        (void)flags;
        float scale = m_.descScale != 0.0F ? m_.descScale : 1.0F;
        const float scaleKopie = scale;
        float yAdj = 0;
        constexpr int FONT_SMALL2 = 4;
        constexpr int FONT_MEDIUM = 2;
        for (int n = 0; n < 2000; ++n) {
            const int w = textBreite(t, scale, FONT_SMALL2);
            int xPos = m_.descX;
            if (m_.descAlignment == ITEM_ALIGN_RIGHT) { xPos = m_.descX - w; }
            else if (m_.descAlignment == ITEM_ALIGN_CENTER) { xPos = m_.descX - w / 2; }
            if (m_.descAlignment == ITEM_ALIGN_CENTER && static_cast<float>(xPos + w) > kSchirmBreite - 4) {
                scale -= 0.001F;
                continue;
            }
            if (scale != scaleKopie) {
                yAdj = static_cast<float>(textHoehe(scaleKopie, FONT_MEDIUM) - textHoehe(scale, FONT_MEDIUM));
            }
            text(static_cast<float>(xPos), static_cast<float>(m_.descY) + yAdj, scale, m_.descColor, t, 0, it.textStyle, FONT_SMALL2);
            break;
        }
    }

    // Item_ListBox_Paint (SP code/ui/ui_shared.cpp, MP codemp/ui/ui_shared.c)
    // mit startPos 0 und cursorPos aus der Laufzeit (sonst 0).
    void listbox(const Item& it) {
        const Rechteck& r = it.window.rect;
        const std::vector<FeederEintrag> z = u_.feeder ? u_.feeder(it.special) : std::vector<FeederEintrag>{};
        const int count = static_cast<int>(z.size());
        const int cursorPos = std::clamp(u_.listCursor, 0, std::max(0, count - 1));
        const bool mp = u_.dialekt == Dialekt::MP;
        const Farbe weiss{1, 1, 1, 1};
        const bool waagrecht = (aktFlags_ & WINDOW_HORIZONTAL) != 0;
        auto maxScroll = [&]() {   // Item_ListBox_MaxScroll
            const float m = waagrecht ? static_cast<float>(count) - (r.w / it.elementWidth) + 1
                                      : static_cast<float>(count) - (r.h / it.elementHeight) + 1;
            const int mi = static_cast<int>(m);
            return mi < 0 ? 0 : mi;
        };
        auto spalte = [&](const FeederEintrag& e, std::size_t j) -> std::string {
            return j < e.spalten.size() ? e.spalten[j] : std::string();
        };
        auto bildVon = [&](const FeederEintrag& e, std::size_t j) -> std::string {
            return j < e.spaltenBild.size() ? e.spaltenBild[j] : std::string();
        };
        if (waagrecht) {
            if (!it.scrollhidden) {
                if (maxScroll() > 0) {
                    float x = r.x + 1;
                    const float y = r.y + r.h - kScrollbar - 1;
                    bild(x, y, kScrollbar, kScrollbar, "gfx/menus/scrollbar_arrow_left.tga", weiss, item_);
                    x += kScrollbar - 1;
                    const float size = r.w - (kScrollbar * 2);
                    bild(x, y, size + 1, kScrollbar, "gfx/menus/scrollbar.tga", weiss, item_);
                    x += size - 1;
                    bild(x, y, kScrollbar, kScrollbar, "gfx/menus/scrollbar_arrow_right.tga", weiss, item_);
                    float thumb = r.x + 1 + kScrollbar;   // Item_ListBox_ThumbPosition, startPos 0
                    if (thumb > x - kScrollbar - 1) { thumb = x - kScrollbar - 1; }
                    bild(thumb, y, kScrollbar, kScrollbar, "gfx/menus/scrollbar_thumb.tga", weiss, item_);
                }
            }
            float size = r.w - 2;
            if (it.elementStyle == 1) {   // LISTBOX_IMAGE
                float x = r.x + 1;
                const float y = r.y + 1;
                for (int i = 0; i < count; i++) {
                    const std::string& image = z[static_cast<std::size_t>(i)].bild;
                    if (!image.empty()) {
                        Farbe c = weiss;
                        if (!mp && (aktFlags_ & WINDOW_PLAYERCOLOR) != 0) {
                            c = Farbe{cvarWert("ui_char_color_red") / 255.0F, cvarWert("ui_char_color_green") / 255.0F,
                                      cvarWert("ui_char_color_blue") / 255.0F, 1};
                        }
                        bild(x + 1, y + 1, it.elementWidth - 2, it.elementHeight - 2, image, c, item_);
                    }
                    if (i == cursorPos) { rahmen(x, y, it.elementWidth - 1, it.elementHeight - 1, it.window.borderSize, it.window.borderColor); }
                    size -= it.elementWidth;
                    if (size < it.elementWidth || it.elementWidth <= 0) { break; }
                    x += it.elementWidth;
                }
            }
            return;
        }
        if (!it.scrollhidden) {
            const float x = r.x + r.w - kScrollbar - 1;
            float y = r.y + 1;
            bild(x, y, kScrollbar, kScrollbar, "gfx/menus/scrollbar_arrow_up_a.tga", weiss, item_);
            y += kScrollbar - 1;
            const float size = r.h - (kScrollbar * 2);
            bild(x, y, kScrollbar, size + 1, "gfx/menus/scrollbar.tga", weiss, item_);
            y += size - 1;
            bild(x, y, kScrollbar, kScrollbar, "gfx/menus/scrollbar_arrow_dwn_a.tga", weiss, item_);
            float thumb = r.y + 1 + kScrollbar;   // startPos 0
            if (thumb > y - kScrollbar - 1) { thumb = y - kScrollbar - 1; }
            bild(x, thumb, kScrollbar, kScrollbar, "gfx/menus/scrollbar_thumb.tga", weiss, item_);
        }
        float size = r.h - 2;
        if (it.elementStyle == 1) {   // LISTBOX_IMAGE
            const float x = r.x + 1;
            float y = r.y + 1;
            for (int i = 0; i < count; i++) {
                const std::string& image = z[static_cast<std::size_t>(i)].bild;
                if (!image.empty()) { bild(x + 1, y + 1, it.elementWidth - 2, it.elementHeight - 2, image, weiss, item_); }
                if (i == cursorPos) { rahmen(x, y, it.elementWidth - 1, it.elementHeight - 1, it.window.borderSize, it.window.borderColor); }
                size -= it.elementHeight;
                if (size < it.elementHeight || it.elementHeight <= 0) { break; }
                y += it.elementHeight;
            }
            return;
        }
        const float x = r.x + 1;
        float y = r.y + 1 - it.elementHeight;
        for (int i = 0; i < count; i++) {
            const FeederEintrag& e = z[static_cast<std::size_t>(i)];
            if (!it.columns.empty()) {
                for (std::size_t j = 0; j < it.columns.size(); j++) {
                    const Spalte& s = it.columns[j];
                    std::string text = spalte(e, j);
                    if (!text.empty() && text[0] == '@') { text = se(text); }
                    if (!mp) {
                        const std::string opt = bildVon(e, j);
                        if (!opt.empty()) {
                            bild(x + 4 + static_cast<float>(s.pos), y - 1 + it.elementHeight / 2, static_cast<float>(s.width),
                                 static_cast<float>(s.width), opt, weiss, item_);
                        } else {
                            const Farbe& c = i == cursorPos ? m_.focusColor : it.window.foreColor;
                            this->text(x + 4 + static_cast<float>(s.pos), y + it.elementHeight, it.textscale, c, text, s.maxChars, it.textStyle,
                                       it.font);
                        }
                    } else {
                        this->text(x + 4 + static_cast<float>(s.pos), y + it.elementHeight + it.textaligny, it.textscale, it.window.foreColor,
                                   text, s.maxChars, it.textStyle, it.font);
                    }
                }
            } else {
                const std::string text = spalte(e, 0);
                const bool optBild = !bildVon(e, 0).empty();
                if (!optBild && (!text.empty() || !e.spalten.empty())) {
                    if (mp) {
                        this->text(x + 4, y + it.textaligny, it.textscale, it.window.foreColor, text, 0, it.textStyle, it.font);
                    } else {
                        this->text(x + 4, y + it.elementHeight, it.textscale, it.window.foreColor, text, 0, it.textStyle, it.font);
                    }
                }
            }
            if (i == cursorPos) {
                fuellen(x + 2, y + it.elementHeight + 2, r.w - kScrollbar - 4, mp ? it.elementHeight : it.elementHeight + 2, it.window.outlineColor);
            }
            size -= it.elementHeight;
            if (size < it.elementHeight || it.elementHeight <= 0) { break; }
            y += it.elementHeight;
        }
        if (count == 0 && !u_.feeder) { platzhalter(r, "feeder " + zahlText(it.special)); }
    }

    const Datei& d_;
    const Menu& m_;
    const Umgebung& u_;
    std::vector<Prim>& out_;
    int item_ = -1;
    int aktFlags_ = 0;   // windowDef_t.flags des Items, das gerade gemalt wird
    bool dimmen_ = false;
    Rechteck tr_;
};

}  // namespace

bool ladeFontdat(const std::string& name, const std::string& daten, Schrift& out) {
    out = Schrift{};
    out.name = name;
    out.shader = "fonts/" + name;
    if (daten.size() != kFontdatGroesse) { return false; }
    std::size_t p = 0;
    for (int i = 0; i < kGlyphs; ++i) {
        Glyph& g = out.glyph[i];
        g.width = lese<short>(daten, p);
        g.height = lese<short>(daten, p);
        g.horizAdvance = lese<short>(daten, p);
        g.horizOffset = lese<short>(daten, p);
        g.baseline = lese<int>(daten, p);
        g.s = lese<float>(daten, p);
        g.t = lese<float>(daten, p);
        g.s2 = lese<float>(daten, p);
        g.t2 = lese<float>(daten, p);
    }
    out.pointSize = lese<short>(daten, p);
    out.height = lese<short>(daten, p);
    out.ascender = lese<short>(daten, p);
    out.descender = lese<short>(daten, p);
    if (out.height == 0) {   // CFontInfo: "have to completely guess at the baseline"
        out.height = out.pointSize;
        out.ascender = static_cast<short>(out.pointSize - runden(static_cast<float>(out.pointSize) / 10.0F + 2));
        out.descender = static_cast<short>(out.height - out.ascender);
    }
    out.ok = true;
    return true;
}

int schriftBreite(const Schrift& f, const std::string& text, float scale, Kodierung k, bool asienGlyphen) {
    const float scaleAsien = (asiatisch(k) && scale > 0.7F) ? scale * 0.75F : scale;
    float maxW = 0;
    float diese = 0;
    const auto* p = reinterpret_cast<const unsigned char*>(text.data());
    for (std::size_t i = 0; i < text.size();) {
        int n = 1;
        const unsigned ch = zeichenLesen(k, p + i, text.size() - i, n);
        i += static_cast<std::size_t>(n);
        if (ch == '^' && i < text.size() && text[i] >= '0' && text[i] <= '9') {
            (void)zeichenLesen(k, p + i, text.size() - i, n);
            i += static_cast<std::size_t>(n);
            continue;
        }
        if (ch == 0x0A) {
            diese = 0;
        } else {
            // GetLetterHorizAdvance: 0 -> die Breite von '.'
            Glyph asienG;
            std::string shader;
            const Glyph* g = &f.glyph[ch & 0xFFU];
            if (asienGlyphen && asienGlyph(k, ch, f, asienG, shader)) { g = &asienG; }
            const short adv = g->horizAdvance != 0 ? g->horizAdvance : f.glyph['.'].horizAdvance;
            diese += static_cast<float>(adv) * (ch > 255U ? scaleAsien : scale);
        }
        maxW = std::max(maxW, diese);
    }
    return static_cast<int>(std::ceil(maxW));
}

int schriftHoehe(const Schrift& f, float scale) {
    return static_cast<int>(static_cast<float>(f.pointSize) * scale);
}

bool itemZeigtUeberCvar(const Item& it, int flag, const Umgebung& u) {
    if (it.enableCvar.empty() || it.cvarTest.empty()) { return true; }
    const std::string buff = u.cvar ? u.cvar(it.cvarTest) : std::string();
    std::size_t p = 0;
    std::string val;
    while (true) {
        if (!stringParse(it.enableCvar, p, val)) { return (it.cvarFlags & flag) != 0 ? false : true; }
        if (val == ";") { continue; }
        if ((it.cvarFlags & flag) != 0) {
            if (gleichOhneFall(buff, val)) { return true; }
        } else if (gleichOhneFall(buff, val)) {
            return false;
        }
    }
}

bool itemSichtbar(const Item& it, int flags, const Umgebung& u) {
    if (it.window.ownerDrawFlags != 0 && u.ownerDrawSichtbar) {
        if (!u.ownerDrawSichtbar(it.window.ownerDrawFlags)) { flags &= ~WINDOW_VISIBLE; }
        else { flags |= WINDOW_VISIBLE; }
    }
    if ((it.cvarFlags & (CVAR_SHOW | CVAR_HIDE)) != 0 && !itemZeigtUeberCvar(it, CVAR_SHOW, u)) { return false; }
    return (flags & WINDOW_VISIBLE) != 0;
}

void zeichneMenu(const Datei& d, int menu, const Umgebung& u, std::vector<Prim>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    if ((m.window.flags & WINDOW_VISIBLE) == 0 && !u.erzwingen) { return; }
    Maler(d, m, u, out).menu();
}

Rechteck textRechteck(const Item& it, const std::string& text, const Umgebung& u) {
    static const Datei leer{};
    static const Menu leerM{};
    std::vector<Prim> dummy;
    return Maler(leer, leerM, u, dummy).textRect(it, text);
}

float sliderDaumen(const Item& it, const Umgebung& u) {
    static const Datei leer{};
    static const Menu leerM{};
    std::vector<Prim> dummy;
    return Maler(leer, leerM, u, dummy).daumen(it);
}

namespace {
// Der Schriftspeicher des Renderers (code/rd-common/tr_font.cpp): Griff 0 ist
// "fehlt", jede neue Schrift belegt den naechsten Griff - auch eine, deren
// .fontdat fehlt (CFontInfo legt sich trotzdem an, RE_RegisterFont_Real
// liefert dann 0). Der OpenJK-Renderer meldet zu jeder gefundenen Schrift
// <name>_sharp1 .. _sharp8 als eigene Griffe an (MAX_FONT_VARIANTS), Movie
// Duels kennt keine Varianten.
struct SchriftSpeicher {
    std::vector<std::string> griffe{std::string()};   // Griff -> Name ("" = ungueltig)
    std::map<std::string, int> index;                   // g_mapFontIndexes
    std::vector<bool> mitVarianten{false};
    const GibtSchrift* gibt = nullptr;
    bool varianten = true;
    bool da(const std::string& n) const { return gibt == nullptr || !*gibt || (*gibt)(n); }
    int real(const std::string& n) {   // RE_RegisterFont_Real
        const auto it = index.find(n);
        if (it != index.end()) { return it->second; }
        const int h = static_cast<int>(griffe.size());
        const bool ok = da(n);
        griffe.push_back(ok ? n : std::string());
        mitVarianten.push_back(false);
        index[n] = ok ? h : 0;
        return ok ? h : 0;
    }
    int anmelden(const std::string& n) {   // RE_RegisterFont
        const int h = real(n);
        if (h != 0 && varianten && !mitVarianten[static_cast<std::size_t>(h)]) {
            for (int i = 0; i < 8; ++i) {
                const std::string v = n + "_sharp" + std::to_string(i + 1);
                if (gibt == nullptr || !*gibt || !(*gibt)(v)) { break; }
                if (real(v) == 0) { break; }
                mitVarianten[static_cast<std::size_t>(h)] = true;
            }
        }
        return h;
    }
};
}  // namespace

std::vector<std::string> schriftNamen(const Datei& assets, Dialekt d, Spiel spiel, const GibtSchrift& gibt) {
    return schriftNamen(std::vector<const Datei*>{&assets}, d, spiel, gibt);
}

std::vector<std::string> schriftNamen(const std::vector<const Datei*>& satz, Dialekt d, Spiel spiel, const GibtSchrift& gibt) {
    auto schriftName = [](std::string n) {
        if (n.compare(0, 6, "fonts/") == 0) { n = n.substr(6); }
        return n;
    };
    if (d == Dialekt::MP) {
        // MenuFontToHandle: 1 smallFont, 2 font, 3 bigFont, 4 small2Font, sonst qhMediumFont (font)
        std::vector<std::string> v(5);
        for (const Datei* dp : satz) {
            for (const Eigenschaft& e : dp->assets.eigenschaften) {
                if (e.args.empty() || !e.ok) { continue; }
                const std::string n = schriftName(e.args[0].s);
                const std::string& k = e.schluessel;
                if (gleichOhneFall(k, "smallFont")) { v[1] = n; }
                else if (gleichOhneFall(k, "font")) { v[2] = n; }
                else if (gleichOhneFall(k, "bigFont")) { v[3] = n; }
                else if (gleichOhneFall(k, "small2Font")) { v[4] = n; }
            }
        }
        if (gibt) {   // trap->R_RegisterFont: fehlende Schrift = Griff 0, zeichnet nichts
            for (std::string& n : v) {
                if (!n.empty() && !gibt(n)) { n.clear(); }
            }
        }
        v[0] = v[2];
        return v;
    }
    // SP: UI_RegisterFont (ui_atoms.cpp) - Renderer-Griff, bei 0 "ergoec";
    // OpenJK merkt die Griffe ohne Doppelte in registeredFonts.
    SchriftSpeicher r;
    r.gibt = &gibt;
    r.varianten = spiel != Spiel::MD;
    std::vector<int> registriert;
    int medium = 0;
    bool fontRegistered = false;
    auto uiAnmelden = [&](const std::string& n) {
        int h = r.anmelden(n);
        if (h == 0) { h = r.anmelden("ergoec"); }
        if (h != 0 && std::find(registriert.begin(), registriert.end(), h) == registriert.end()) { registriert.push_back(h); }
        return h;
    };
    // UI_LoadMenus: die Dateien des Menuesatzes der Reihe nach - je Datei erst ihr
    // assetGlobalDef, dann die menuDefs (MenuParse_font, nur solange keine mediumFont da ist)
    for (const Datei* dp : satz) {
        for (const Eigenschaft& e : dp->assets.eigenschaften) {
            if (e.args.empty() || !e.ok) { continue; }
            const std::string n = schriftName(e.args[0].s);
            const std::string& k = e.schluessel;
            if (gleichOhneFall(k, "smallFont") || gleichOhneFall(k, "bigFont")) { (void)uiAnmelden(n); }
            else if (gleichOhneFall(k, "mediumFont")) {
                medium = uiAnmelden(n);
                fontRegistered = true;
            }
        }
        for (const Menu& m : dp->menus) {
            if (!m.font.empty() && !fontRegistered) {
                medium = uiAnmelden(schriftName(m.font));
                fontRegistered = true;
            }
        }
    }
    auto name = [&](int h) { return (h > 0 && static_cast<std::size_t>(h) < r.griffe.size()) ? r.griffe[static_cast<std::size_t>(h)] : std::string(); };
    std::vector<std::string> v(1, name(medium));   // Text_Paint: Nummer 0 = qhMediumFont
    if (spiel == Spiel::MD) {
        // Movie Duels: ItemParse_font nimmt die Nummer als Renderer-Griff
        for (std::size_t h = 1; h < r.griffe.size(); ++h) { v.push_back(r.griffe[h]); }
    } else {
        // OpenJK: UI_MenuFontToReal - Nummer n = registeredFonts[n - 1]
        for (int h : registriert) { v.push_back(name(h)); }
    }
    return v;
}

Abbildung abbildung(float breite, float hoehe, Seitenverhaeltnis s) {
    Abbildung a;
    if (s == Seitenverhaeltnis::Strecken) {
        a.sx = breite / kSchirmBreite;
        a.sy = hoehe / kSchirmHoehe;
        return a;
    }
    const float f = std::min(breite / kSchirmBreite, hoehe / kSchirmHoehe);
    a.sx = a.sy = f;
    a.ox = (breite - kSchirmBreite * f) * 0.5F;
    a.oy = (hoehe - kSchirmHoehe * f) * 0.5F;
    return a;
}

std::string multiText(const Item& it, const Umgebung& u) {   // Item_Multi_Setting
    if (!it.typeData) { return "@MENUS_CUSTOM"; }
    const std::string buff = it.cvar.empty() ? std::string() : (u.cvar ? u.cvar(it.cvar) : std::string());
    if (!it.multiFeeder.empty() && it.cvarList.empty()) {
        // ItemParse_cvarStrList "feeder": FEEDER_PLAYER_SPECIES -> "@MENUS_<NAME>" / Name,
        // FEEDER_LANGUAGES -> "@MENUS_MYLANGUAGE" / Sprache
        const bool sprachen = it.multiFeeder == "FEEDER_LANGUAGES";
        for (const FeederEintrag& e : u.feeder ? u.feeder(it.special) : std::vector<FeederEintrag>{}) {
            const std::string name = e.spalten.empty() ? std::string() : e.spalten[0];
            if (gleichOhneFall(buff, name)) {
                if (sprachen) { return "@MENUS_MYLANGUAGE"; }
                std::string k = "@MENUS_" + name;
                for (char& c : k) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
                return k;
            }
        }
        return "@MENUS_CUSTOM";
    }
    const float value = it.cvar.empty() ? 0.0F : static_cast<float>(std::atof((u.cvar ? u.cvar(it.cvar) : std::string()).c_str()));
    for (std::size_t i = 0; i < it.cvarList.size(); ++i) {
        if (it.strDef) {
            if (i < it.cvarStr.size() && gleichOhneFall(buff, it.cvarStr[i])) { return it.cvarList[i]; }
        } else if (i < it.cvarValue.size() && it.cvarValue[i] == value) {
            return it.cvarList[i];
        }
    }
    return "@MENUS_CUSTOM";
}


std::string spielCvarVorgabe(Dialekt d, Spiel s, const std::string& name) {
    if (d != Dialekt::SP) { return {}; }
    // UI_Init (code/ui/ui_atoms.cpp) legt g_char_* an, UI_GetCharacterCvars kopiert sie
    // beim Oeffnen der Figurenmenues nach ui_char_*.
    static const char* const vorgaben[][2] = {
        {"g_char_model", "jedi_tf"},      {"g_char_skin_head", "head_a1"}, {"g_char_skin_torso", "torso_a1"},
        {"g_char_skin_legs", "lower_a1"}, {"g_char_color_red", "255"},     {"g_char_color_green", "255"},
        {"g_char_color_blue", "255"},     {"ui_char_anim", "BOTH_WALK1"},   // ui_main.cpp cvarTable
        // code/game/g_main.cpp (G_InitCvars)
        {"g_saber", "single_1"},          {"g_saber_color", "yellow"},     {"g_saber2_color", "yellow"}};
    for (const auto& v : vorgaben) {
        if (gleichOhneFall(name, v[0])) { return v[1]; }
    }
    if (s == Spiel::MD) {   // Movie Duels code/ui/ui_main.cpp cvarTable: Klingenart der Saeber
        // (M3: die Groessen aller Arten, cg_SFXSabersGlowSize<EP1|EP2|...> bzw. ...CoreSize..., Vorgabe 1.0)
        if (gleichOhneFall(name, "cg_SFXSabers")) { return "4"; }
        if (gleichOhneFall(name, "g_SerenityJediEngineMode")) { return "0"; }
        for (const char* art : {"", "EP1", "EP2", "EP3", "SFX", "OT", "ROTJ", "TFA", "USB", "Rebels", "CW", "Maul"}) {
            if (gleichOhneFall(name, std::string("cg_SFXSabersGlowSize") + art) || gleichOhneFall(name, std::string("cg_SFXSabersCoreSize") + art)) {
                return "1.0";
            }
        }
    }
    return {};
}

}  // namespace bhed::menue
