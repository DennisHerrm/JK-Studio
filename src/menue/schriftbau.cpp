// schriftbau.cpp - Menue-Editor M2 Punkt 9 (siehe schriftbau.h).
// Eigener Code.
#include "bhed/menue/schriftbau.h"

#include <algorithm>
#include <cstring>

namespace bhed::menue {

namespace {
template <typename T>
void schreib(std::string& s, T v) {
    char b[sizeof(T)];
    std::memcpy(b, &v, sizeof(T));
    s.append(b, sizeof(T));
}
}  // namespace

int cp1252ZuUnicode(int b) {
    static const int k80[32] = {0x20AC, 0,      0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
                                0x2039, 0x0152, 0,      0x017D, 0,      0,      0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
                                0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0,      0x017E, 0x0178};
    if (b < 32 || b > 255) { return 0; }
    if (b >= 128 && b < 160) { return k80[b - 128]; }
    return b;
}

SchriftDateien baueSchrift(const std::vector<GlyphBild>& zeichen, const SchriftMasse& m) {
    SchriftDateien out;
    // Lage in der Grafik: Zeilen von links nach rechts, 1 Bildpunkt Abstand
    struct Lage {
        int x = 0, y = 0;
    };
    std::vector<Lage> lage(zeichen.size());
    int breite = 256;
    for (int versuch = 0; versuch < 2; ++versuch) {
        int x = 1, y = 1, zeilenHoehe = 0;
        bool passt = true;
        for (std::size_t i = 0; i < zeichen.size(); ++i) {
            const GlyphBild& g = zeichen[i];
            if (g.breite + 2 > breite) { passt = false; }
            if (x + g.breite + 1 > breite) {
                x = 1;
                y += zeilenHoehe + 1;
                zeilenHoehe = 0;
            }
            lage[i] = Lage{x, y};
            x += g.breite + 1;
            zeilenHoehe = std::max(zeilenHoehe, g.hoehe);
        }
        const int noetig = y + zeilenHoehe + 1;
        int h = 1;
        while (h < noetig) { h *= 2; }
        if (passt && (h <= breite * 2 || versuch == 1)) {
            out.texBreite = breite;
            out.texHoehe = h;
            break;
        }
        breite = 512;
    }
    const int tw = out.texBreite, th = out.texHoehe;
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(tw) * static_cast<std::size_t>(th) * 4U, 0);
    for (std::size_t p = 0; p < rgba.size(); p += 4) { rgba[p] = rgba[p + 1] = rgba[p + 2] = 255; }   // weiss, Deckung im Alpha
    struct Info {
        short w = 0, h = 0, adv = 0, off = 0;
        int base = 0;
        float s = 0, t = 0, s2 = 0, t2 = 0;
    };
    Info info[256];
    for (std::size_t i = 0; i < zeichen.size(); ++i) {
        const GlyphBild& g = zeichen[i];
        if (g.code < 0 || g.code > 255) { continue; }
        for (int yy = 0; yy < g.hoehe; ++yy) {
            for (int xx = 0; xx < g.breite; ++xx) {
                const std::size_t q = static_cast<std::size_t>(yy) * static_cast<std::size_t>(g.breite) + static_cast<std::size_t>(xx);
                if (q >= g.deckung.size()) { continue; }
                const std::size_t z = (static_cast<std::size_t>(lage[i].y + yy) * static_cast<std::size_t>(tw) + static_cast<std::size_t>(lage[i].x + xx)) * 4U;
                if (z + 3 < rgba.size()) { rgba[z + 3] = g.deckung[q]; }
            }
        }
        Info& f = info[g.code];
        f.w = static_cast<short>(g.breite);
        f.h = static_cast<short>(g.hoehe);
        f.adv = static_cast<short>(g.vorschub);
        f.off = static_cast<short>(g.x0);
        f.base = -g.y0 - 1;
        f.s = static_cast<float>(lage[i].x) / static_cast<float>(tw);
        f.t = static_cast<float>(lage[i].y) / static_cast<float>(th);
        f.s2 = static_cast<float>(lage[i].x + g.breite) / static_cast<float>(tw);
        f.t2 = static_cast<float>(lage[i].y + g.hoehe) / static_cast<float>(th);
    }
    // dfontdat_t
    for (const Info& f : info) {
        schreib(out.fontdat, f.w);
        schreib(out.fontdat, f.h);
        schreib(out.fontdat, f.adv);
        schreib(out.fontdat, f.off);
        schreib(out.fontdat, f.base);
        schreib(out.fontdat, f.s);
        schreib(out.fontdat, f.t);
        schreib(out.fontdat, f.s2);
        schreib(out.fontdat, f.t2);
    }
    schreib(out.fontdat, static_cast<short>(m.punkt));
    schreib(out.fontdat, static_cast<short>(m.aufstieg + m.abstieg));   // mHeight
    schreib(out.fontdat, static_cast<short>(m.aufstieg));
    schreib(out.fontdat, static_cast<short>(m.abstieg));
    schreib(out.fontdat, static_cast<short>(0));   // mKoreanHack
    out.fontdat.append(2, '\0');                    // auf 4 aufgefuellt: 7180
    // TGA: unkomprimiert, 32 Bit, Ursprung oben links (Bit 5), 8 Alpha-Bits
    std::string& t = out.tga;
    const unsigned char kopf[18] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, static_cast<unsigned char>(tw & 255), static_cast<unsigned char>(tw >> 8),
                                    static_cast<unsigned char>(th & 255), static_cast<unsigned char>(th >> 8), 32, 0x28};
    t.assign(reinterpret_cast<const char*>(kopf), 18);
    t.reserve(18 + rgba.size());
    for (std::size_t p = 0; p < rgba.size(); p += 4) {   // BGRA
        t += static_cast<char>(rgba[p + 2]);
        t += static_cast<char>(rgba[p + 1]);
        t += static_cast<char>(rgba[p]);
        t += static_cast<char>(rgba[p + 3]);
    }
    return out;
}

}  // namespace bhed::menue
