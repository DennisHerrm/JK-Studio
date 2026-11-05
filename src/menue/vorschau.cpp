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

#include "bhed/menue/profil.h"

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
constexpr int kPulseDivisor = 75;                             // PULSE_DIVISOR (code/ und codemp/qcommon/q_shared.h)
constexpr int kBlinkDivisorSp = 600;                          // BLINK_DIVISOR, code/qcommon/q_shared.h:272
constexpr int kBlinkDivisorMp = 200;                          // BLINK_DIVISOR, codemp/qcommon/q_shared.h:290
constexpr int ITEM_TEXTSTYLE_BLINK = 1;
constexpr int ITEM_TEXTSTYLE_PULSE = 2;
constexpr int ITEM_TEXTSTYLE_SHADOWED = 3;
constexpr int ITEM_TEXTSTYLE_SHADOWEDMORE = 6;

// g_color_table (OpenJK q_shared.c): ^0 .. ^9
const Farbe kFarbtabelle[10] = {{0, 0, 0, 1}, {1, 0, 0, 1}, {0, 1, 0, 1}, {1, 1, 0, 1}, {0, 0, 1, 1},
                                {0, 1, 1, 1}, {1, 0, 1, 1}, {1, 1, 1, 1}, {1, 0.5F, 0, 1}, {0.5F, 0.5F, 0.5F, 1}};

int runden(float v) { return static_cast<int>(std::lround(v)); }
// Round (tr_font.cpp): (int)( value + 0.5f )
int rundeFont(float v) { return static_cast<int>(v + 0.5F); }

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
            // rc608 Punkt 9, im Spiel fotografiert (Movie Duels SP und MP): RE_RegisterShaderNoMip liefert ohne Bild Griff 0.
            const bool bildDa = !m_.window.background.empty() && (!u_.bildDa || u_.bildDa(m_.window.background));
            if (u_.dialekt == Dialekt::SP) {
                // code/ui Menu_Paint: R_SetColor(backColor), ohne Bild whiteShader. backColor ist ab Werk (0,0,0,0): ein leeres Menue
                // zeichnet nichts und ist schwarz, weil _UI_IsFullscreen das Spielbild darunter weglaesst; auch ein Bild ohne backcolor
                // (implizit rgbGen vertex) bleibt unsichtbar.
                if (bildDa) { bild(0, 0, kSchirmBreite, kSchirmHoehe, m_.window.background, m_.window.backColor, -1); }
                // rc608 Teil 3b, im Spiel fotografiert: jk2sp.exe zeichnet backcolor ohne Bild nicht, jasp und Movie Duels fuellen
                else if (m_.window.backColor.a > 0.0F && d_.spiel != Spiel::JK2) { fuellen(0, 0, kSchirmBreite, kSchirmHoehe, m_.window.backColor); }
            } else {
                // codemp/ui Menu_Paint: DC->drawHandlePic mit Griff 0 ("FIXME: make sure we have a default shader"): der graue "<default>"-Kasten
                bild(0, 0, kSchirmBreite, kSchirmHoehe, bildDa ? m_.window.background : std::string(kDefaultShader), Farbe{1, 1, 1, 1}, -1);
            }
        }
        // rc608 Teil 3b, im Spiel fotografiert: Menu_Paint ruft Window_Paint immer. Ein Menue mit style FILLED und backcolor deckt auch den grauen Kasten des MP.
        fenster(m_.window, -1);
        for (std::size_t i = 0; i < m_.items.size(); ++i) {
            item_ = static_cast<int>(i);
            item(m_.items[i]);
        }
        item_ = -1;
    }

    // Text_Width / Text_Height
    int textBreite(const std::string& t, float scale, int font) const {
        const Schrift* f = schrift(font);
        return f != nullptr ? schriftBreite(*f, t, scale, u_.kodierung, u_.asienGlyphen, u_.jk2) : 0;
    }
    int textHoehe(float scale, int font) const {
        const Schrift* f = schrift(font);
        return f != nullptr ? schriftHoehe(*f, scale, u_.jk2) : 0;
    }

    std::string cvar(const std::string& n) const { return u_.cvar ? u_.cvar(n) : std::string(); }
    // M5b: Name aus dem Mod-Profil fuer den Platzhalter (" NAME" oder "")
    std::string profilName(bool ownerdraw, int nr) const {
        if (!d_.profil) { return {}; }
        const auto& tab = ownerdraw ? d_.profil->ownerdraws : d_.profil->feeder;
        const auto f = tab.find(nr);
        return f == tab.end() ? std::string() : " " + f->second;
    }
    float cvarWert(const std::string& n) const { return static_cast<float>(std::atof(cvar(n).c_str())); }
    std::string se(const std::string& t) const { return u_.texte != nullptr ? u_.texte->imSpiel(t) : t; }

    // Item_SetTextExtents (OpenJK code/ui/ui_shared.cpp:6212-6260, codemp/ui/ui_shared.c:4430-4470; Movie Duels SP
    // :6215 ff., MP :5002 ff.). rc610 F46 B6-3: zur Ausrichtung zaehlt bei ownerdraws mit CENTER oder RIGHT
    // DC->ownerDrawWidth dazu und bei zentrierten Eingabefeldern der Wert der cvar (char buff[256]); textRect.w bleibt
    // die Breite des Texts. Movie Duels SP teilt "originalWidth / static_cast<float>(2)", die anderen ganzzahlig.
    Rechteck textRect(const Item& it, const std::string& text) const {
        Rechteck r;
        const int breite = textBreite(text, it.textscale, it.font);
        int originalWidth = breite;
        if (it.type == ITEM_TYPE_OWNERDRAW && (it.textalignment == ITEM_ALIGN_CENTER || it.textalignment == ITEM_ALIGN_RIGHT)) {
            if (u_.ownerDrawBreite) { originalWidth += u_.ownerDrawBreite(it.window.ownerDraw, it.textscale); }
        } else if (it.type == ITEM_TYPE_EDITFIELD && it.textalignment == ITEM_ALIGN_CENTER && !it.cvar.empty()) {
            std::string buff = cvar(it.cvar);
            if (buff.size() > 255) { buff.resize(255); }
            originalWidth += textBreite(buff, it.textscale, it.font);
        }
        r.w = static_cast<float>(breite);
        r.h = static_cast<float>(textHoehe(it.textscale, it.font));
        r.x = it.textalignx;
        r.y = it.textaligny;
        if (it.textalignment == ITEM_ALIGN_RIGHT) {
            r.x = it.textalignx - static_cast<float>(originalWidth);
        } else if (it.textalignment == ITEM_ALIGN_CENTER) {
            const bool mdSp = d_.spiel == Spiel::MD && u_.dialekt == Dialekt::SP;
            r.x = it.textalignx - (mdSp ? static_cast<float>(originalWidth) / 2.0F : static_cast<float>(originalWidth / 2));
        }
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
        value *= sliderMasse(u_.dialekt, d_.spiel).breite;
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
    // rc608 Teil 3b, im Spiel gemessen (Rahmen 1: 4 Bildpunkte bei 1280 x 960): _UI_DrawSides und _UI_DrawTopBottom rechnen size *= xscale bzw. yscale,
    // und das Bild skaliert danach noch einmal. Die Dicke haengt also von der Aufloesung des Spiels ab.
    void seiten(float x, float y, float w, float h, float size, const Farbe& c) {
        const float s = size * u_.rahmenX;
        fuellen(x, y, s, h, c);
        fuellen(x + w - s, y, s, h, c);
    }
    void obenUnten(float x, float y, float w, float h, float size, const Farbe& c) {
        const float s = size * u_.rahmenY;
        fuellen(x, y, w, s, c);
        fuellen(x, y + h - s, w, s, c);
    }
    void rahmen(float x, float y, float w, float h, float size, const Farbe& c) {
        seiten(x, y, w, h, size, c);
        obenUnten(x, y, w, h, size, c);
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
        // M5: ein fehlendes Bild registriert die Engine als Griff 0: FILLED prueft "if (w->background)"
        // und fuellt dann, SHADER zeichnet Griff 0 = tr.shaders[0], den "<default>"-Kasten
        // (R_CreateDefaultImage, OpenJK SP/MP und Movie Duels gleich)
        const bool bildOk = !w.background.empty() && (!u_.bildDa || u_.bildDa(w.background));
        if (w.style == WINDOW_STYLE_FILLED) {
            if (bildOk) {
                bild(f.x, f.y, f.w, f.h, w.background, w.backColor, item);
            } else {
                fuellen(f.x, f.y, f.w, f.h, w.backColor);
            }
        } else if (w.style == WINDOW_STYLE_GRADIENT) {
            bild(f.x, f.y, f.w, f.h, "ui/assets/gradientbar2.tga", w.backColor, item);   // GradientBar_Paint
        } else if (w.style == WINDOW_STYLE_SHADER) {
            const Farbe c = (w.flags & WINDOW_FORECOLORSET) != 0 ? w.foreColor : Farbe{1, 1, 1, 1};
            // "<default>" hat rgbGen CGEN_BAD -> identityLight: die Farbe zaehlt dort nicht
            bild(f.x, f.y, f.w, f.h, bildOk ? w.background : std::string(kDefaultShader), bildOk ? c : Farbe{1, 1, 1, 1}, item);
        } else if (w.style == WINDOW_STYLE_TEAMCOLOR && u_.dialekt == Dialekt::MP) {
            fuellen(f.x, f.y, f.w, f.h, Farbe{1, 0, 0, 1});   // DC->getTeamColor: ohne Spiel rot
        } else if (w.style == WINDOW_STYLE_CINEMATIC && u_.dialekt == Dialekt::MP) {
            // M4: nur MP (codemp ui_shared.c Window_Paint: DC->playCinematic/drawCinematic); die
            // SP-Fassung (OpenJK code/ui, Movie Duels) kennt weder TEAMCOLOR-Flaeche noch CINEMATIC
            platzhalter(f, w.cinematicName, PrimArt::Kino);
        }
        if (w.border == WINDOW_BORDER_FULL) {
            rahmen(w.rect.x, w.rect.y, w.rect.w, w.rect.h, w.borderSize, w.borderColor);
        } else if (w.border == WINDOW_BORDER_HORZ) {   // drawTopBottom
            obenUnten(w.rect.x, w.rect.y, w.rect.w, w.rect.h, w.borderSize, w.borderColor);
        } else if (w.border == WINDOW_BORDER_VERT) {   // drawSides
            seiten(w.rect.x, w.rect.y, w.rect.w, w.rect.h, w.borderSize, w.borderColor);
        }
    }

    static Farbe lerp(const Farbe& a, const Farbe& b, float t) {   // LerpColor
        return Farbe{a.r + t * (b.r - a.r), a.g + t * (b.g - a.g), a.b + t * (b.b - a.b), a.a + t * (b.a - a.a)};
    }
    static Farbe mal(const Farbe& a, float f) { return Farbe{a.r * f, a.g * f, a.b * f, a.a * f}; }

    // rc610 F46 B6-2: 0.5 + 0.5 * sin((float)(DC->realTime / PULSE_DIVISOR)) - ganzzahlig geteilt in OpenJK SP und MP und
    // Movie Duels MP; Movie Duels SP teilt durch static_cast<float>(PULSE_DIVISOR) (Item_TextColor, Item_TextField_Paint,
    // Item_Bind_Paint, Item_OwnerDraw_Paint, Item_Slider_Paint)
    float puls() const {
        const bool mdSp = d_.spiel == Spiel::MD && u_.dialekt == Dialekt::SP;
        const float w = mdSp ? static_cast<float>(u_.zeitMs) / static_cast<float>(kPulseDivisor) : static_cast<float>(u_.zeitMs / kPulseDivisor);
        return 0.5F + 0.5F * std::sin(w);
    }
    // !((DC->realTime / BLINK_DIVISOR) & 1): SP 600, MP 200
    bool blinkAn() const { return ((u_.zeitMs / (u_.dialekt == Dialekt::SP ? kBlinkDivisorSp : kBlinkDivisorMp)) & 1) == 0; }

    // Item_TextColor (OpenJK code/ui/ui_shared.cpp:6276-6327 SP, codemp/ui/ui_shared.c:4483-4516 MP; Movie Duels SP
    // :6255-6297). rc610 F46 B6-2: im SP steht der BLINK-Zweig in /* */ (Movie Duels: fehlt), und der Fokus gilt nicht
    // fuer einen autowrapped Text ("!(item->type == ITEM_TYPE_TEXT && item->window.flags & WINDOW_AUTOWRAPPED)").
    Farbe textFarbe(const Item& it, int flags) const {
        const bool sp = u_.dialekt == Dialekt::SP;
        const bool fokusAus = sp && it.type == ITEM_TYPE_TEXT && ((flags | it.window.flags) & WINDOW_AUTOWRAPPED) != 0;
        Farbe c;
        if ((flags & WINDOW_HASFOCUS) != 0 && !fokusAus) {
            c = lerp(m_.focusColor, mal(m_.focusColor, 0.8F), puls());
        } else if (!sp && it.textStyle == ITEM_TEXTSTYLE_BLINK && blinkAn()) {
            c = lerp(it.window.foreColor, mal(it.window.foreColor, 0.8F), puls());
        } else {
            c = it.window.foreColor;
        }
        if (!it.enableCvar.empty() && !it.cvarTest.empty()) {
            if ((it.cvarFlags & (CVAR_ENABLE | CVAR_DISABLE)) != 0 && !itemZeigtUeberCvar(it, CVAR_ENABLE, u_)) {
                c = m_.disableColor;
            }
        }
        if (u_.dialekt == Dialekt::SP && (flags & WINDOW_INACTIVE) != 0) { c = Farbe{0.5F, 0.5F, 0.5F, 1.0F}; }   // greyColor
        return c;
    }

    // M7: Item_OwnerDraw_Paint (codemp/ui/ui_shared.c, code/ui/ui_shared.cpp): Farbe (colorRanges mit
    // DC->getValue = 0, Fokus pulsiert, Blinken, enableCvar), mit Text erst Item_Text_Paint und dann
    // DC->ownerDrawItem rechts daneben (+8; MP nur bei nicht leerem Text), sonst am rect mit textalignx/y
    void ownerDrawMalen(const Item& it, int flags) {
        const float puls = this->puls();
        Farbe color = it.window.foreColor;
        for (const ColorRange& cr : it.colorRanges) {
            if (0.0F >= cr.low && 0.0F <= cr.high) {
                color = cr.color;
                break;
            }
        }
        if ((flags & WINDOW_HASFOCUS) != 0) {
            color = lerp(m_.focusColor, mal(m_.focusColor, 0.8F), puls);
        } else if (it.textStyle == ITEM_TEXTSTYLE_BLINK && blinkAn()) {
            color = lerp(it.window.foreColor, mal(it.window.foreColor, 0.8F), puls);
        }
        if ((it.cvarFlags & (CVAR_ENABLE | CVAR_DISABLE)) != 0 && !itemZeigtUeberCvar(it, CVAR_ENABLE, u_)) { color = m_.disableColor; }
        OdAufruf a;
        a.id = it.window.ownerDraw;
        a.flags = it.window.ownerDrawFlags;
        a.align = it.alignment;
        a.special = it.special;
        a.scale = it.textscale;
        a.farbe = color;
        a.shader = it.window.background;
        a.stil = it.textStyle;
        a.font = it.font;
        a.y = it.window.rect.y;
        a.w = it.window.rect.w;
        a.h = it.window.rect.h;
        a.textY = it.textaligny;
        if (!it.text.empty()) {
            textMalen(it, flags);
            a.x = tr_.x + tr_.w + ((u_.dialekt == Dialekt::SP || !itemText(it).empty()) ? 8.0F : 0.0F);
        } else {
            a.x = it.window.rect.x;
            a.textX = it.textalignx;
        }
        const OdErgebnis o = u_.ownerDraw ? u_.ownerDraw(a) : OdErgebnis{};
        if (!o.bekannt) {
            platzhalter(it.window.rect, "ownerdraw " + std::to_string(it.window.ownerDraw) + profilName(true, it.window.ownerDraw));
            return;
        }
        teileMalen(o.teile);
        if (o.beispiel) { platzhalter(it.window.rect, "", PrimArt::Beispiel); }
    }

public:
    // M7: Zeichenbefehle von ownerdraws und cgame (Ladebild, Verbindung) mit den Mitteln der Vorschau
    void teileMalen(const std::vector<OdTeil>& teile) {
        for (const OdTeil& t : teile) {
            if (t.art == OdTeil::Art::Bild) {
                bild(t.x, t.y, t.w, t.h, t.inhalt, t.farbe, item_, t.s0, t.t0, t.s1, t.t1);
            } else if (t.art == OdTeil::Art::Fuellen) {
                fuellen(t.x, t.y, t.w, t.h, t.farbe);
            } else if (!t.schrift.empty()) {
                textMit(u_.schriftNamens ? u_.schriftNamens(t.schrift) : nullptr, t.x, t.y, t.scale, t.farbe, t.inhalt, t.maxPixel, t.stil);
            } else {
                text(t.x, t.y, t.scale, t.farbe, t.inhalt, t.maxPixel, t.stil, t.font);
            }
        }
    }

private:
    // Text_Paint -> RE_Font_DrawString
    void text(float x, float y, float scale, const Farbe& c, const std::string& t, int maxPixel, int style, int font) {
        if (u_.textLaeufe != nullptr && item_ == u_.textLaeufeItem) {
            u_.textLaeufe->push_back(TextLauf{static_cast<float>(static_cast<int>(x)), static_cast<float>(static_cast<int>(y)), scale, schrift(font), t});
        }
        // rc608: jk2mp zaehlt hier Zeichen (Q3: Text_Paint limit), JKA Bildpunkte. Sonst blieben von "Bespin Streets"
        // mit "columns 1 2 190 25" nur "Bes" (25 Bildpunkte) uebrig; im Spiel steht der ganze Name.
        if (d_.spiel == Spiel::JK2 && u_.dialekt == Dialekt::MP && maxPixel > 0) {
            textMit(schrift(font), x, y, scale, c, t.substr(0, static_cast<std::size_t>(maxPixel)), 0, style);
            return;
        }
        textMit(schrift(font), x, y, scale, c, t, maxPixel, style);
    }
    // M7: dasselbe mit einer Schrift beim Namen (cgame/Pazaak zeichnen mit eigenem Griff)
    void textMit(const Schrift* f, float x, float y, float scale, const Farbe& c, const std::string& t, int maxPixel, int style) {
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
        std::vector<Prim> glyphen;
        schriftZeichnen(f, ox, oy, t, c, maxPixel, scale, imSchatten, glyphen, u_.kodierung, u_.asienGlyphen, u_.jk2);
        for (const Prim& g : glyphen) { bild(g.x, g.y, g.w, g.h, g.shader, g.farbe, item_, g.s0, g.t0, g.s1, g.t1); }
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
            y += hoehe + (u_.dialekt == Dialekt::SP ? 5 : 2);   // rc608 Teil 3b: code/ui + 5, codemp/ui + 2
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
        // rc608 Teil 3b: code/ui bricht schon bei Breite - textalignx um, rueckt um Hoehe + 4 vor und hoert am Boden des
        // Kastens mit "..." auf (cursorPos bleibt 0, ohne Tastendruck kein Rollen); codemp/ui nichts davon.
        const bool sp = u_.dialekt == Dialekt::SP;
        const float grenze = sp ? it.window.rect.w - it.textalignx : it.window.rect.w;
        const int vorschub = hoehe + (sp ? 4 : 5);
        const float boden = it.window.rect.h - static_cast<float>(hoehe);
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
            const bool voll = sp ? static_cast<float>(textWidth) >= grenze : static_cast<float>(textWidth) > grenze;
            if ((newLine != 0 && voll) || ch == '\n' || ch == '\0') {
                if (!buff.empty()) {
                    float x = it.textalignx;
                    if (it.textalignment == ITEM_ALIGN_RIGHT) { x = it.textalignx - static_cast<float>(newLineWidth); }
                    else if (it.textalignment == ITEM_ALIGN_CENTER) { x = it.textalignx - static_cast<float>(newLineWidth / 2); }
                    float yy = y;
                    zuFenster(x, yy, it.window);
                    std::string zeile = buff.substr(0, newLine);
                    if (sp && ch != '\0' && y + static_cast<float>(vorschub) > boden) { zeile += "..."; }
                    text(x, yy, it.textscale, c, zeile, 0, it.textStyle, it.font);
                }
                if (sp) {
                    y += static_cast<float>(vorschub);
                    if (y > boden) { break; }
                }
                if (ch == '\0') { break; }
                if (!sp) { y += static_cast<float>(vorschub); }
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
            case ITEM_TYPE_OWNERDRAW:   // M7: UI_OwnerDraw je Nummer (ownerdraw.cpp)
                ownerDrawMalen(it, flags);
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
                                           puls())
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
            case ITEM_TYPE_TEXTSCROLL:   // M4: Item_TextScroll_Paint
                textscroll(it);
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
            case ITEM_TYPE_BIND: {   // M7: Item_Bind_Paint mit BindingFromName (tasten.cpp)
                Farbe c;
                if ((flags & WINDOW_HASFOCUS) != 0) {   // ohne wartende Taste (g_bindItem): Fokusfarbe pulsiert
                    c = lerp(m_.focusColor, mal(m_.focusColor, 0.8F), puls());
                } else {
                    c = u_.dialekt == Dialekt::MP ? it.window.foreColor : textFarbe(it, flags);
                }
                if (it.text.empty()) { break; }   // ohne text: "FIXME" (kommt in keinem Menue vor)
                textMalen(it, flags);
                const std::string name = u_.bindung ? u_.bindung(it.cvar) : std::string("???");
                // breiter als der Schirm: Schrift in 0,05-Schritten kleiner (SP misst mit qhMediumFont)
                const int mf = u_.dialekt == Dialekt::MP ? it.font : 0;
                float sc = it.textscale;
                const float x = tr_.x + tr_.w + 8;
                while (sc > 0.0F && x + static_cast<float>(textBreite(name, sc, mf)) >= kSchirmBreite) { sc -= 0.05F; }
                const int yAdj = sc != it.textscale ? textHoehe(it.textscale, mf) - textHoehe(sc, mf) : 0;
                text(x, tr_.y + static_cast<float>(yAdj), sc, c, name, it.maxPaintChars, it.textStyle, it.font);
                break;
            }
            case ITEM_TYPE_SLIDER:
            case ITEM_TYPE_SLIDER_INTEGER: {   // Item_Slider_Paint (Movie Duels auch fuer SLIDER_INTEGER)
                const Farbe c = (flags & WINDOW_HASFOCUS) != 0
                                    ? lerp(m_.focusColor, mal(m_.focusColor, 0.8F),
                                           puls())
                                    : it.window.foreColor;
                const float y = it.window.rect.y;
                float x = it.window.rect.x;
                if (!it.text.empty()) {
                    textMalen(it, flags);
                    x = tr_.x + tr_.w + 8;
                }
                const SliderMasse sm = sliderMasse(u_.dialekt, d_.spiel);
                bild(x, y + sm.leisteY, sm.breite, kSliderHoehe, "menu/new/slider", c, item_);
                const float dx = daumen(it);
                bild(dx - kSliderDaumenBreite / 2, y + sm.daumenY, kSliderDaumenBreite, sm.daumenHoehe, "menu/new/sliderthumb", c, item_);
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
    // M4: Item_TextScroll_Paint (OpenJK SP/MP, Movie Duels): Bildlaufleiste rechts, wenn die
    // Zeilen nicht passen (Movie Duels: nicht fuer "char_desc", NEW_FEEDER_V2), dann ab startPos
    // die Zeilen mit lineHeight, bis der Platz kleiner als eine Zeile ist.
    void textscroll(const Item& it) {
        int startPos = 0;
        if (u_.textLaeufe != nullptr && item_ == u_.textLaeufeItem) { startPos = u_.textLaeufeStart; }
        else if (static_cast<std::size_t>(item_) < u_.zustand.size()) { startPos = u_.zustand[static_cast<std::size_t>(item_)].startPos; }
        const TextScrollZeilen z = textScrollZeilen(it, u_);
        const Rechteck& r = it.window.rect;
        const int count = static_cast<int>(z.zeilen.size());
        float size = r.h - 2;
        std::string name = it.window.name;
        for (char& c : name) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        const bool ohneLeiste = d_.spiel == Spiel::MD && name == "char_desc";   // Q_stricmp
        const Farbe weiss{1, 1, 1, 1};
        if (!ohneLeiste && static_cast<float>(count * it.lineHeight) > size) {
            const float x = r.x + r.w - kScrollbar - 1;
            float y = r.y + 1;
            bild(x, y, kScrollbar, kScrollbar, "gfx/menus/scrollbar_arrow_up_a.tga", weiss, item_);
            y += kScrollbar - 1;
            size = r.h - kScrollbar * 2;
            bild(x, y, kScrollbar, size + 1, "gfx/menus/scrollbar.tga", weiss, item_);
            y += size - 1;
            bild(x, y, kScrollbar, kScrollbar, "gfx/menus/scrollbar_arrow_dwn_a.tga", weiss, item_);
            float thumb = textScrollDaumen(it, count, startPos);
            if (thumb > y - kScrollbar - 1) { thumb = y - kScrollbar - 1; }
            bild(x, thumb, kScrollbar, kScrollbar, "gfx/menus/scrollbar_thumb.tga", weiss, item_);
        }
        size = r.h - 2;
        const float x = r.x + it.textalignx + 1;
        float y = r.y + it.textaligny + 1;
        for (int i = std::max(0, startPos); i < count; ++i) {
            if (!z.da[static_cast<std::size_t>(i)]) { continue; }
            text(x + 4, y, it.textscale, it.window.foreColor, z.zeilen[static_cast<std::size_t>(i)], 0, it.textStyle, it.font);
            size -= static_cast<float>(it.lineHeight);
            if (size < static_cast<float>(it.lineHeight)) { break; }
            y += static_cast<float>(it.lineHeight);
        }
    }

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
                // rc608: jk2mp schreibt den Namen des gewaehlten Bilds unter die Liste ("rebel/default", im Spiel gesehen)
                if (d_.spiel == Spiel::JK2 && mp && cursorPos >= 0 && cursorPos < count) {
                    text(r.x, r.y + r.h, it.textscale, it.window.foreColor, spalte(z[static_cast<std::size_t>(cursorPos)], 0), 0, it.textStyle, it.font);
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
        if (it.elementStyle == 1 && mp && r.w > it.elementWidth * 2 && it.elementWidth > 0 && it.elementHeight > 0) {
            // M5: MP-Item_ListBox_Paint - mehr als doppelt so breit wie ein Element: Zeilen und Spalten
            int start = 0, ende = 0;
            float y = r.y + 1;
            float hoehe = r.h - 2;
            for (int i2 = start; i2 < count; i2++) {
                float x = r.x + 1;
                float breite = r.w - 2;
                for (int i = start; i < count; i++) {
                    const std::string& image = z[static_cast<std::size_t>(i)].bild;
                    if (!image.empty()) {
                        Farbe c = weiss;
                        if ((aktFlags_ & WINDOW_PLAYERCOLOR) != 0) {
                            c = Farbe{cvarWert("ui_char_color_red") / 255.0F, cvarWert("ui_char_color_green") / 255.0F,
                                      cvarWert("ui_char_color_blue") / 255.0F, 1};
                        }
                        bild(x + 1, y + 1, it.elementWidth - 2, it.elementHeight - 2, image, c, item_);
                    }
                    if (i == cursorPos) { rahmen(x, y, it.elementWidth - 1, it.elementHeight - 1, it.window.borderSize, it.window.borderColor); }
                    breite -= it.elementWidth;
                    if (breite < it.elementWidth) { break; }
                    x += it.elementWidth;
                    ende++;
                }
                hoehe -= it.elementHeight;
                if (hoehe < it.elementHeight) { break; }
                ende++;
                start = ende;
                y += it.elementHeight;
            }
            return;
        }
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
        // M7: ohne Darstellung (unbekannte Liste) der beschriftete Kasten; Beispielwerte beschriftet
        const int art = u_.feederArt ? u_.feederArt(it.special) : 1;
        if (count == 0 && (!u_.feeder || art == 0)) { platzhalter(r, "feeder " + zahlText(it.special) + profilName(false, static_cast<int>(it.special))); }
        if (art == 2) { platzhalter(r, "", PrimArt::Beispiel); }
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

void schriftZeichnen(const Schrift& f, int ox, int oy, const std::string& t, Farbe c, int maxPixel, float scale, bool imSchatten,
                     std::vector<Prim>& out, Kodierung k, bool asienGlyphen, Jk2Schrift jk2) {
    const bool jk2Modus = jk2 != Jk2Schrift::Nein;
    const bool rund = jk2 == Jk2Schrift::Sp;   // mbRoundCalcs (siehe schriftHoehe)
    const auto vorschub = [rund](float v) { return rund ? static_cast<float>(rundeFont(v)) : v; };
    // fScaleAsian / fAsianYAdjust (Language_IsAsian)
    const bool asien = asiatisch(k);
    const float scaleAsien = (asien && scale > 0.7F) ? scale * 0.75F : scale;
    const float yAsien = (asien && scale > 0.7F)
                             ? ((static_cast<float>(f.pointSize) * scale) - (static_cast<float>(f.pointSize) * scaleAsien)) / 2.0F
                             : 0.0F;
    const float fox = static_cast<float>(ox);
    float foy = static_cast<float>(oy) + vorschub(static_cast<float>(f.height - (f.descender >> 1)) * scale);
    float fx = fox;
    bool ueberlauf = false;
    const auto* p = reinterpret_cast<const unsigned char*>(t.data());
    for (std::size_t i = 0; i < t.size() && !ueberlauf;) {
        int n = 1;
        const unsigned ch = zeichenLesen(k, p + i, t.size() - i, n);
        i += static_cast<std::size_t>(n);
        if (ch == 10) {
            fx = fox;
            foy += vorschub(static_cast<float>(f.pointSize) * scale);
            if (asien) { foy += 4.0F; }
        } else if (ch == 13) {
        } else if (ch == 32) {
            fx += vorschub(static_cast<float>(f.glyph[' '].horizAdvance) * scale);
            ueberlauf = maxPixel != -1 && (fx - fox) > static_cast<float>(maxPixel);
        } else if (ch == '^' && jk2Modus) {
            // Jedi Outcast (jk2sp.exe und jk2mp.exe im Spiel fotografiert): case '^' nimmt jedes folgende
            // Zeichen als Farbe, ColorIndex(c) = (c - '0') & 7 - "A^Bx" zeichnet A und ein gruenes x
            if (i < t.size()) {
                const int idx = (static_cast<unsigned char>(t[i]) - '0') & 7;
                ++i;
                if (!imSchatten) {
                    const float a = c.a;
                    c = kFarbtabelle[idx];
                    c.a = a;
                }
            }
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
            if (asienGlyphen && asienGlyph(k, ch, f, asienG, shader)) { g = &asienG; }
            if (g->width == 0) {
                g = &f.glyph['.'];
                shader = f.shader;
            }
            const float sc = ch > 255U ? scaleAsien : scale;   // g_iNonScaledCharRange = 255
            const float adv = vorschub(static_cast<float>(g->horizAdvance) * sc);
            ueberlauf = maxPixel != -1 && ((fx + adv) - fox) > static_cast<float>(maxPixel);
            if (!ueberlauf) {
                const float fy = foy - static_cast<float>(g->baseline) * sc;
                Prim q;
                q.art = PrimArt::Bild;
                q.x = fx + static_cast<float>(g->horizOffset) * sc;
                q.y = ch > 255U ? fy - yAsien : fy;
                q.w = static_cast<float>(g->width) * sc;
                q.h = static_cast<float>(g->height) * sc;
                q.s0 = g->s;
                q.t0 = g->t;
                q.s1 = g->s2;
                q.t1 = g->t2;
                q.farbe = c;
                q.shader = shader;
                out.push_back(q);
            }
            fx += adv;
        }
    }
}

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

int schriftBreite(const Schrift& f, const std::string& text, float scale, Kodierung k, bool asienGlyphen, Jk2Schrift jk2) {
    const float scaleAsien = (asiatisch(k) && scale > 0.7F) ? scale * 0.75F : scale;
    float maxW = 0;
    float diese = 0;
    const auto* p = reinterpret_cast<const unsigned char*>(text.data());
    for (std::size_t i = 0; i < text.size();) {
        int n = 1;
        unsigned ch = zeichenLesen(k, p + i, text.size() - i, n);
        i += static_cast<std::size_t>(n);
        // JA/MD: ^0-^9 zaehlen nicht; jk2mp.exe: die Ziffer zaehlt; jk2sp.exe (JK2_MODE): ^ und Ziffer zaehlen
        if (ch == '^' && jk2 != Jk2Schrift::Sp && i < text.size() && text[i] >= '0' && text[i] <= '9') {
            const unsigned ziffer = zeichenLesen(k, p + i, text.size() - i, n);
            i += static_cast<std::size_t>(n);
            if (jk2 == Jk2Schrift::Nein) { continue; }
            ch = ziffer;
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
            const float v = static_cast<float>(adv) * (ch > 255U ? scaleAsien : scale);
            diese += jk2 == Jk2Schrift::Sp ? static_cast<float>(rundeFont(v)) : v;   // mbRoundCalcs
        }
        maxW = std::max(maxW, diese);
    }
    return static_cast<int>(std::ceil(maxW));
}

int schriftHoehe(const Schrift& f, float scale, Jk2Schrift jk2) {
    const float v = static_cast<float>(f.pointSize) * scale;
    return jk2 == Jk2Schrift::Sp ? rundeFont(v) : static_cast<int>(v);
}

// Item_EnableShowViaCvar: OJ SP code/ui/ui_shared.cpp:6120 (MD SP :6109) mit char buff/script[1024],
// OJ MP codemp/ui/ui_shared.c:2211 (MD MP :2334) mit [2048]. cvarSubString: im SP ist buff der erste
// Wert der Liste und durchsucht werden die Woerter der cvar; MD MP vergleicht mit strstr(buff, val)
// (OJ MP kennt cvarSubString nicht).
bool itemZeigtUeberCvar(const Item& it, int flag, const Umgebung& u) {
    if (it.enableCvar.empty() || it.cvarTest.empty()) { return true; }
    const bool mp = u.dialekt == Dialekt::MP;
    const bool teil = (it.cvarFlags & CVAR_SUBSTRING) != 0;
    const std::size_t laenge = mp ? 2047 : 1023;   // Q_strncpyz / getCVarString in den Puffer
    const std::string wert = (u.cvar ? u.cvar(it.cvarTest) : std::string()).substr(0, laenge);
    std::string buff, liste;
    std::size_t p = 0;
    std::string val;
    if (teil && !mp) {
        if (!stringParse(it.enableCvar, p, val)) { return (it.cvarFlags & flag) != 0 ? false : true; }
        buff = val.substr(0, laenge);
        liste = wert;
        p = 0;
    } else {
        buff = wert;
        liste = it.enableCvar.substr(0, laenge);
    }
    while (true) {
        if (!stringParse(liste, p, val)) { return (it.cvarFlags & flag) != 0 ? false : true; }
        if (val == ";") { continue; }
        const bool trifft = (teil && mp && buff.find(val) != std::string::npos) || gleichOhneFall(buff, val);
        if ((it.cvarFlags & flag) != 0) {
            if (trifft) { return true; }
        } else if (trifft) {
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

// --- M4: ITEM_TYPE_TEXTSCROLL (Item_TextScroll_BuildLines, OpenJK SP/MP und Movie Duels gleich) ---

namespace {
// tr_font.cpp: *_IsTrailingPunctuation (die ersten Zeichen der Satzzeichenzeile je Kodierung)
bool asienSatzzeichen(Kodierung k, unsigned code) {
    switch (k) {
        case Kodierung::Taiwanesisch: return code >= ((0xA1U << 8) | 0x40U) && code < (((0xA1U << 8) | 0x40U) + 20);
        case Kodierung::Japanisch: return code >= ((0x81U << 8) | 0x40U) && code < (((0x81U << 8) | 0x40U) + 18);
        case Kodierung::Chinesisch: return code > ((0xA1U << 8) | 0xA0U) && code < (((0xA1U << 8) | 0xA0U) + 14);
        default: return false;
    }
}
}  // namespace

std::string textScrollText(const Item& it, const Umgebung& u) {
    // Item_TextScroll_Paint: mit cvar ersetzt DC->getCVarString den Text; BuildLines: "@" = SE_GetString
    std::string t = it.cvar.empty() ? it.text : (u.cvar ? u.cvar(it.cvar) : std::string());
    if (!t.empty() && t[0] == '@' && u.texte != nullptr) { t = u.texte->imSpiel(t); }
    return t;
}

UmbruchZeile naechsteZeile(const std::string& text, std::size_t& pos, int boxBreite, const std::function<int(const std::string&)>& breite,
                           Kodierung k) {
    UmbruchZeile z;
    const bool asien = asiatisch(k);
    const bool leerzeichen = !(k == Kodierung::Taiwanesisch || k == Kodierung::Japanisch || k == Kodierung::Chinesisch);
    const auto* s = reinterpret_cast<const unsigned char*>(text.c_str());
    const std::size_t n = text.size();
    std::size_t lineStart = pos, best = pos;
    while (pos < n && s[pos] != 0) {
        const std::size_t lastGood = pos;
        int adv = 1;
        unsigned letter = s[pos];
        bool punkt = false;
        if (asien) {   // AnyLanguage_ReadCharFromString
            letter = zeichenLesen(k, s + pos, n - pos, adv);
            punkt = adv == 2 ? asienSatzzeichen(k, letter) : false;
        }
        if (adv != 2) {
            letter = s[pos];
            adv = 1;
            punkt = letter == '!' || letter == '?' || letter == ',' || letter == '.' || letter == ';' || letter == ':';
        }
        pos += static_cast<std::size_t>(adv);
        if (letter == 32 && z.text.empty()) {   // Leerzeichen am Zeilenanfang ueberspringen
            ++lineStart;
            continue;
        }
        if (letter > 255) {
            z.text.push_back(static_cast<char>(letter >> 8));
            z.text.push_back(static_cast<char>(letter & 0xFF));
        } else {
            z.text.push_back(static_cast<char>(letter & 0xFF));
        }
        if (letter == '\n') {   // ausdruecklicher Zeilenumbruch
            z.text.pop_back();
            z.umbruch = true;
            return z;
        }
        if (breite(z.text) >= boxBreite) {
            if (!(letter > 255 && punkt && !leerzeichen)) {
                if (best == lineStart) { best = lastGood; }   // kein Leer- oder Satzzeichen auf der ganzen Zeile
                z.text.resize(best - lineStart);
                pos = best;
                z.umbruch = true;
                return z;
            }
        }
        if (punkt || letter == ' ' || (letter > 255 && !leerzeichen)) { best = pos; }
    }
    return z;
}

TextScrollZeilen textScrollZeilen(const Item& it, const Umgebung& u) {
    TextScrollZeilen z;
    const std::string text = textScrollText(it, u);
    const Schrift* f = u.schrift ? u.schrift(it.font) : nullptr;
    const std::function<int(const std::string&)> breite = [&](const std::string& s) {
        return f != nullptr ? schriftBreite(*f, s, it.textscale, u.kodierung, u.asienGlyphen, u.jk2) : 0;
    };
    const int iBoxWidth = static_cast<int>(it.window.rect.w - kScrollbar - 10);
    std::size_t pos = 0;
    while (pos < text.size() && text[pos] != 0 && z.zeilen.size() < kMaxTextScrollZeilen) {
        const UmbruchZeile zeile = naechsteZeile(text, pos, iBoxWidth, breite, u.kodierung);
        z.zeilen.push_back(zeile.text);
        // letzte Zeile ohne Umbruch: leer = NULL, beim Zeichnen uebersprungen
        z.da.push_back(zeile.umbruch || !zeile.text.empty());
    }
    return z;
}

int textScrollMaxScroll(const Item& it, int zeilen) {   // Item_TextScroll_MaxScroll
    if (it.lineHeight == 0) { return 0; }
    const int max = zeilen - static_cast<int>(it.window.rect.h / static_cast<float>(it.lineHeight)) + 1;
    return max < 0 ? 0 : max;
}

float textScrollDaumen(const Item& it, int zeilen, int startPos) {   // Item_TextScroll_ThumbPosition
    const float max = static_cast<float>(textScrollMaxScroll(it, zeilen));
    const float size = it.window.rect.h - kScrollbar * 2 - 2;
    const float pos = max > 0 ? (size - kScrollbar) / max : 0.0F;
    return static_cast<float>(static_cast<int>(it.window.rect.y + 1 + kScrollbar + pos * static_cast<float>(startPos)));
}

void zeichneMenu(const Datei& d, int menu, const Umgebung& u, std::vector<Prim>& out) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.menus.size()) { return; }
    const Menu& m = d.menus[static_cast<std::size_t>(menu)];
    if ((m.window.flags & WINDOW_VISIBLE) == 0 && !u.erzwingen) { return; }
    // M7: UI_DrawForceSide zeigt/verbirgt beim Zeichnen Gruppen seines Menues (Menu_ShowItemByName:
    // name oder group ohne Gross/Klein) - vom naechsten Bild an gilt das fuer alle Items
    std::vector<OdErgebnis::Zeigen> zeigen;
    if (u.ownerDraw) {
        for (std::size_t i = 0; i < m.items.size(); ++i) {
            const Item& it = m.items[i];
            if (it.type != ITEM_TYPE_OWNERDRAW) { continue; }
            const bool zu = i < u.zustand.size() && u.zustand[i].gueltig;
            if (!itemSichtbar(it, zu ? u.zustand[i].flags : it.window.flags, u)) { continue; }
            OdAufruf a;
            a.id = it.window.ownerDraw;
            for (const OdErgebnis::Zeigen& e : u.ownerDraw(a).zeigen) {
                if (gleichOhneFall(e.menue, m.window.name)) { zeigen.push_back(e); }
            }
        }
    }
    if (zeigen.empty()) {
        Maler(d, m, u, out).menu();
        return;
    }
    Umgebung u2 = u;
    u2.zustand.resize(m.items.size());
    for (std::size_t i = 0; i < m.items.size(); ++i) {
        if (!u2.zustand[i].gueltig) {
            u2.zustand[i].flags = m.items[i].window.flags;
            u2.zustand[i].gueltig = true;
        }
    }
    for (const OdErgebnis::Zeigen& e : zeigen) {
        for (std::size_t i = 0; i < m.items.size(); ++i) {
            const Fenster& w = m.items[i].window;
            if (!gleichOhneFall(w.name, e.item) && (w.group.empty() || !gleichOhneFall(w.group, e.item))) { continue; }
            if (e.zeigen) { u2.zustand[i].flags |= WINDOW_VISIBLE; }
            else { u2.zustand[i].flags &= ~WINDOW_VISIBLE; }
        }
    }
    Maler(d, m, u2, out).menu();
}

void zeichneTeile(const std::vector<OdTeil>& teile, const Umgebung& u, std::vector<Prim>& out) {
    static const Datei leer{};
    static const Menu leerM{};
    Maler(leer, leerM, u, out).teileMalen(teile);
}

TextMass textMasse(const Item& it, const Umgebung& u) {
    TextMass m;
    const Schrift* f = u.schrift ? u.schrift(it.font) : nullptr;
    if (f != nullptr) {
        m.unten = static_cast<float>(f->height - (f->descender >> 1)) * it.textscale;   // foy in RE_Font_DrawString
        m.oben = m.unten - static_cast<float>(f->ascender) * it.textscale;
    }
    return m;
}

Rechteck textRechteck(const Item& it, const std::string& text, const Umgebung& u, Spiel spiel) {
    static const Datei leerJa{};
    static const Datei leerMd = [] {
        Datei d;
        d.spiel = Spiel::MD;
        return d;
    }();
    static const Menu leerM{};
    std::vector<Prim> dummy;
    return Maler(spiel == Spiel::MD ? leerMd : leerJa, leerM, u, dummy).textRect(it, text);
}

SliderMasse sliderMasse(Dialekt d, Spiel s) {
    if (d == Dialekt::MP) { return {}; }
    return {s == Spiel::MD ? 96.0F : 128.0F, 2.0F, 2.0F, 16.0F};
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


namespace {
#include "cvarvorgaben.inc"
#include "cvarvorgaben_jk2.inc"
}  // namespace

std::vector<std::string> spielCvarNamen(Dialekt d, Spiel s) {
    std::vector<std::string> v;
    const auto dazu = [&v](const CvarVorgabe* a, const CvarVorgabe* e) {
        for (; a != e; ++a) { v.emplace_back(a->name); }
    };
    if (s == Spiel::JK2) {
        if (d != Dialekt::SP) { dazu(std::begin(kVorgabenJk2Mp), std::end(kVorgabenJk2Mp)); }
        else { dazu(std::begin(kVorgabenJk2Sp), std::end(kVorgabenJk2Sp)); }
    } else if (d != Dialekt::SP) {
        dazu(std::begin(kEngineMp), std::end(kEngineMp));
        dazu(std::begin(kVorgabenMp), std::end(kVorgabenMp));
    } else if (s == Spiel::MD) {
        dazu(std::begin(kEngineMd), std::end(kEngineMd));
        dazu(std::begin(kVorgabenMd), std::end(kVorgabenMd));
    } else {
        dazu(std::begin(kEngineJa), std::end(kEngineJa));
        dazu(std::begin(kVorgabenJa), std::end(kVorgabenJa));
    }
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    return v;
}

std::string spielCvarVorgabe(Dialekt d, Spiel s, const std::string& name) {
    // rc608: Jedi Outcast hat eigene Werte (sv_hostname noname, ui_netSource 0), aus dem laufenden Spiel
    if (s == Spiel::JK2) {
        const CvarVorgabe* anfang = d != Dialekt::SP ? std::begin(kVorgabenJk2Mp) : std::begin(kVorgabenJk2Sp);
        const CvarVorgabe* ende = d != Dialekt::SP ? std::end(kVorgabenJk2Mp) : std::end(kVorgabenJk2Sp);
        for (const CvarVorgabe* v = anfang; v != ende; ++v) {
            if (gleichOhneFall(name, v->name)) { return v->wert; }
        }
        // Sonst kennt das Spiel ihn hier nicht (ui_about_gametype leer: die Elemente fuer "keine Macht" bleiben versteckt).
        // Nur was der Foto-Lauf selbst setzt, fehlt in der Tabelle: dafuer gilt weiter die Tabelle der Engine.
        static const char* const vomLauf[] = {"in_mouse", "in_joystick", "r_fullscreen", "r_mode", "r_customwidth", "r_customheight",
                                              "s_initsound", "s_volume", "s_musicvolume", "com_maxfps", "sp_language"};
        if (std::none_of(std::begin(vomLauf), std::end(vomLauf), [&name](const char* n) { return gleichOhneFall(name, n); })) { return {}; }
    }
    // M7: was die Engine vor der Oberflaeche anlegt (Com_Init, Fenster, CL_Init, S_Init, R_Register -
    // die Renderer-Cvars der Setup-Menues); das erste Cvar_Get legt den Wert fest
    {
        const CvarVorgabe* anfang = d != Dialekt::SP ? std::begin(kEngineMp) : s == Spiel::MD ? std::begin(kEngineMd) : std::begin(kEngineJa);
        const CvarVorgabe* ende = d != Dialekt::SP ? std::end(kEngineMp) : s == Spiel::MD ? std::end(kEngineMd) : std::end(kEngineJa);
        for (const CvarVorgabe* v = anfang; v != ende; ++v) {
            if (gleichOhneFall(name, v->name)) { return v->wert; }
        }
    }
    if (d != Dialekt::SP) {
        // M5: Mehrspieler wie CL_Init + ui_xcvar.h (OpenJK; ohne das waren ui_char_model und model leer
        // und die Figurenmenues zeigten "models/players//model.glm")
        for (const CvarVorgabe& v : kVorgabenMp) {
            if (gleichOhneFall(name, v.name)) { return v.wert; }
        }
        return {};
    }
    // M3: die Start-Cvars der Oberflaeche (UI_Init, cvarTable) je Spiel - z. B. g_saber_type
    // "single" (das zweite Saebelmodell bleibt versteckt), ui_char_anim, g_char_model
    const CvarVorgabe* const anfang = s == Spiel::MD ? std::begin(kVorgabenMd) : std::begin(kVorgabenJa);
    const CvarVorgabe* const ende = s == Spiel::MD ? std::end(kVorgabenMd) : std::end(kVorgabenJa);
    for (const CvarVorgabe* v = anfang; v != ende; ++v) {
        if (gleichOhneFall(name, v->name)) { return v->wert; }
    }
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
