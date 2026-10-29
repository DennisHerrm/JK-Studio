// schrift_ttf.cpp - Menue-Editor M2 Punkt 9: die Zeichen einer TTF rastern
// (stb_truetype aus Dear ImGui, imstb_truetype.h, public domain / MIT), daraus
// baut bhed::menue::baueSchrift die .fontdat und die .tga fuer das Spiel.
#include "schrift_ttf.h"

#include <algorithm>
#include <cmath>

// Fremdcode: nicht unter unsere Warnungsregeln (wie das imgui-Ziel)
#ifdef _MSC_VER
#pragma warning(disable : 4505)   // nicht benutzte statische Funktionen
#pragma warning(push, 0)
#endif
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "imstb_truetype.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace bhed::gui {

bool rastereTtf(const std::string& ttf, int pixel, std::vector<menue::GlyphBild>& aus, menue::SchriftMasse& m) {
    aus.clear();
    if (ttf.size() < 12 || pixel < 4 || pixel > 128) { return false; }
    const auto* daten = reinterpret_cast<const unsigned char*>(ttf.data());
    const int versatz = stbtt_GetFontOffsetForIndex(daten, 0);
    if (versatz < 0) { return false; }
    stbtt_fontinfo f;
    if (stbtt_InitFont(&f, daten, versatz) == 0) { return false; }
    const float sc = stbtt_ScaleForPixelHeight(&f, static_cast<float>(pixel));
    int auf = 0, ab = 0, luecke = 0;
    stbtt_GetFontVMetrics(&f, &auf, &ab, &luecke);
    m.punkt = pixel;
    m.aufstieg = static_cast<int>(std::lround(static_cast<float>(auf) * sc));
    m.abstieg = static_cast<int>(std::lround(static_cast<float>(-ab) * sc));
    for (int b = 32; b < 256; ++b) {
        const int cp = menue::cp1252ZuUnicode(b);
        if (cp == 0) { continue; }
        const int gi = stbtt_FindGlyphIndex(&f, cp);
        if (gi == 0 && cp != ' ') { continue; }
        int vor = 0, lsb = 0;
        stbtt_GetGlyphHMetrics(&f, gi, &vor, &lsb);
        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        stbtt_GetGlyphBitmapBox(&f, gi, sc, sc, &x0, &y0, &x1, &y1);
        menue::GlyphBild g;
        g.code = b;
        g.breite = std::max(0, x1 - x0);
        g.hoehe = std::max(0, y1 - y0);
        g.x0 = x0;
        g.y0 = y0;
        g.vorschub = static_cast<int>(std::lround(static_cast<float>(vor) * sc));
        g.deckung.assign(static_cast<std::size_t>(g.breite) * static_cast<std::size_t>(g.hoehe), 0);
        if (g.breite > 0 && g.hoehe > 0) { stbtt_MakeGlyphBitmap(&f, g.deckung.data(), g.breite, g.hoehe, g.breite, sc, sc, gi); }
        aus.push_back(std::move(g));
    }
    return !aus.empty();
}

}  // namespace bhed::gui
