// kodierung.cpp - Menue-Editor M3 Punkt 7: Texte in den asiatischen Kodierungen
// der Engine. Portiert aus OpenJK code/rd-common/tr_font.cpp (Einzelheiten in
// include/bhed/menue/kodierung.h).
//
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
#include "bhed/menue/kodierung.h"

#include "bhed/menue/vorschau.h"

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <cctype>

namespace bhed::menue {

namespace {

// tr_font.cpp: Grenzen der Doppelbyte-Codes
constexpr unsigned kKscHiStart = 0xB0, kKscHiStop = 0xC8, kKscLoLo = 0xA0, kKscLoHi = 0xFF, kKscProZeile = 96;
constexpr unsigned kBig5HiStart0 = 0xA1, kBig5HiStop0 = 0xC6, kBig5HiStart1 = 0xC9, kBig5HiStop1 = 0xF9;
constexpr unsigned kBig5LoLo0 = 0x40, kBig5LoHi0 = 0x7E, kBig5LoLo1 = 0xA1, kBig5LoHi1 = 0xFE, kBig5ProZeile = 160;
constexpr unsigned kSjisHiStart0 = 0x81, kSjisHiStop0 = 0x9F, kSjisHiStart1 = 0xE0, kSjisHiStop1 = 0xEF;
constexpr unsigned kSjisLoStart0 = 0x40, kSjisLoStop0 = 0x7E, kSjisLoStart1 = 0x80, kSjisLoStop1 = 0xFC;
constexpr unsigned kSjisProZeile = ((kSjisLoStop0 - kSjisLoStart0) + 1) + ((kSjisLoStop1 - kSjisLoStart1) + 1);
constexpr unsigned kGbHiStart = 0xA1, kGbHiStop = 0xF7, kGbLoLo = 0xA0, kGbLoHi = 0xFF, kGbProZeile = 95;

bool koreanGueltig(unsigned hi, unsigned lo) { return hi >= kKscHiStart && hi <= kKscHiStop && lo > kKscLoLo && lo < kKscLoHi; }
bool big5Gueltig(unsigned code) {
    const unsigned hi = (code >> 8) & 0xFF;
    if ((hi >= kBig5HiStart0 && hi <= kBig5HiStop0) || (hi >= kBig5HiStart1 && hi <= kBig5HiStop1)) {
        const unsigned lo = code & 0xFF;
        return (lo >= kBig5LoLo0 && lo <= kBig5LoHi0) || (lo >= kBig5LoLo1 && lo <= kBig5LoHi1);
    }
    return false;
}
bool sjisGueltig(unsigned hi, unsigned lo) {
    if ((hi >= kSjisHiStart0 && hi <= kSjisHiStop0) || (hi >= kSjisHiStart1 && hi <= kSjisHiStop1)) {
        return (lo >= kSjisLoStart0 && lo <= kSjisLoStop0) || (lo >= kSjisLoStart1 && lo <= kSjisLoStop1);
    }
    return false;
}
bool gbGueltig(unsigned hi, unsigned lo) { return hi >= kGbHiStart && hi <= kGbHiStop && lo > kGbLoLo && lo < kGbLoHi; }

UINT codepage(Kodierung k) {
    switch (k) {
        case Kodierung::Koreanisch: return 949;
        case Kodierung::Taiwanesisch: return 950;
        case Kodierung::Japanisch: return 932;
        case Kodierung::Chinesisch: return 936;
        case Kodierung::Latin1: break;
    }
    return 28591;
}

void anhaengenUtf8(std::string& o, unsigned cp) {
    if (cp < 0x80) {
        o += static_cast<char>(cp);
    } else if (cp < 0x800) {
        o += static_cast<char>(0xC0 | (cp >> 6));
        o += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        o += static_cast<char>(0xE0 | (cp >> 12));
        o += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        o += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        o += static_cast<char>(0xF0 | (cp >> 18));
        o += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        o += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        o += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

// naechster Codepunkt aus UTF-8 (ungueltige Bytes: U+FFFD, ein Byte weiter)
unsigned naechsterCodepunkt(const std::string& s, std::size_t& i) {
    const auto c = static_cast<unsigned char>(s[i]);
    const int n = c < 0x80 ? 0 : (c & 0xE0) == 0xC0 ? 1 : (c & 0xF0) == 0xE0 ? 2 : (c & 0xF8) == 0xF0 ? 3 : -1;
    if (n == 0) {
        ++i;
        return c;
    }
    if (n < 0 || i + static_cast<std::size_t>(n) >= s.size()) {
        ++i;
        return 0xFFFD;
    }
    unsigned cp = n == 1 ? (c & 0x1FU) : n == 2 ? (c & 0x0FU) : (c & 0x07U);
    for (int j = 1; j <= n; ++j) {
        const auto f = static_cast<unsigned char>(s[i + static_cast<std::size_t>(j)]);
        if ((f & 0xC0) != 0x80) {
            ++i;
            return 0xFFFD;
        }
        cp = (cp << 6) | (f & 0x3FU);
    }
    i += static_cast<std::size_t>(n) + 1;
    return cp;
}

}  // namespace

Kodierung kodierungDerSprache(const std::string& seLanguage) {
    std::string s = seLanguage;
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    if (s == "korean") { return Kodierung::Koreanisch; }
    if (s == "taiwanese") { return Kodierung::Taiwanesisch; }
    if (s == "japanese") { return Kodierung::Japanisch; }
    if (s == "chinese") { return Kodierung::Chinesisch; }
    return Kodierung::Latin1;
}

bool asiatisch(Kodierung k) { return k != Kodierung::Latin1; }

unsigned zeichenLesen(Kodierung k, const unsigned char* p, std::size_t rest, int& laenge) {
    laenge = 1;
    if (rest == 0) { return 0; }
    const unsigned hi = p[0];
    const unsigned lo = rest > 1 ? p[1] : 0U;   // die Engine liest dort die abschliessende 0
    bool zwei = false;
    switch (k) {
        case Kodierung::Koreanisch: zwei = koreanGueltig(hi, lo); break;
        case Kodierung::Taiwanesisch: zwei = big5Gueltig(hi * 256 + lo); break;
        case Kodierung::Japanisch: zwei = sjisGueltig(hi, lo); break;
        case Kodierung::Chinesisch: zwei = gbGueltig(hi, lo); break;
        case Kodierung::Latin1: break;
    }
    if (!zwei) { return hi; }
    laenge = 2;
    return hi * 256 + lo;
}

int asienIndex(Kodierung k, unsigned code) {
    const unsigned hi = (code >> 8) & 0xFF;
    const unsigned lo = code & 0xFF;
    switch (k) {
        case Kodierung::Koreanisch:
            if (!koreanGueltig(hi, lo)) { return 0; }
            code -= (kKscHiStart * 256) + kKscLoLo;
            return static_cast<int>(((code >> 8) * kKscProZeile) + (code & 0xFF));
        case Kodierung::Taiwanesisch:
            if (!big5Gueltig(code)) { return 0; }
            code -= (kBig5HiStart0 * 256) + kBig5LoLo0;
            if ((code & 0xFF) >= (kBig5LoLo1 - 1) - kBig5LoLo0) { code -= ((kBig5LoLo1 - 1) - (kBig5LoHi0 + 1)) - 1; }
            return static_cast<int>(((code >> 8) * kBig5ProZeile) + (code & 0xFF));
        case Kodierung::Japanisch:
            if (!sjisGueltig(hi, lo)) { return 0; }
            code -= (kSjisHiStart0 << 8) | kSjisLoStart0;
            if ((code & 0xFF) >= kSjisLoStart1 - kSjisLoStart0) { code -= (kSjisLoStart1 - kSjisLoStop0) - 1; }
            if (((code >> 8) & 0xFF) >= kSjisHiStart1 - kSjisHiStart0) { code -= ((kSjisHiStart1 - kSjisHiStop0) - 1) << 8; }
            return static_cast<int>(((code >> 8) * kSjisProZeile) + (code & 0xFF));
        case Kodierung::Chinesisch:
            if (!gbGueltig(hi, lo)) { return 0; }
            code -= (kGbHiStart * 256) + kGbLoLo;
            return static_cast<int>(((code >> 8) * kGbProZeile) + (code & 0xFF));
        case Kodierung::Latin1: break;
    }
    return 0;
}

int asienSeiten(Kodierung k) {
    // GLYPH_MAX_KOREAN/TAIWANESE/JAPANESE/CHINESE_SHADERS
    return k == Kodierung::Taiwanesisch ? 4 : asiatisch(k) ? 3 : 0;
}

namespace {
// *_InitFields: m_iAsianGlyphsAcross
int quer(Kodierung k) { return k == Kodierung::Koreanisch ? 32 : 64; }
}  // namespace

std::string asienSeite(Kodierung k, int seite) {
    const char* lang = k == Kodierung::Koreanisch ? "kor" : k == Kodierung::Taiwanesisch ? "tai" : k == Kodierung::Japanisch ? "jap" : "chi";
    return std::string("fonts/") + lang + "_" + std::to_string(1024 / quer(k)) + "_1024_" + std::to_string(seite);
}

bool asienGlyph(Kodierung k, unsigned code, const Schrift& w, Glyph& g, std::string& shader) {
    // UpdateAsianIfNeeded: nur mit westlicher Schrift (mHeight) und nicht fuer "aurabesh"
    if (!asiatisch(k) || w.height == 0 || w.name == "aurabesh") { return false; }
    int idx = asienIndex(k, code);
    if (idx == 0) { return false; }
    const int q = quer(k);
    const int seiten = asienSeiten(k);
    int seite = idx / (q * q);
    if (seite > seiten) { seite = 0; }
    idx -= seite * (q * q);
    const int spalte = idx % q;
    const int zeile = idx / q;
    const bool halb = seite == seiten - 1;   // m_bAsianLastPageHalfHeight
    const int runter = halb ? q / 2 : q;
    const auto fq = static_cast<float>(q);
    const auto fr = static_cast<float>(runter);
    switch (k) {
        case Kodierung::Taiwanesisch:
            g.s = static_cast<float>(((1024 / q) * spalte) + 1) / 1024.0F;
            g.t = static_cast<float>(((1024 / runter) * zeile) + 1) / 1024.0F;
            g.s2 = static_cast<float>((1024 / q) * (spalte + 1)) / 1024.0F;
            g.t2 = static_cast<float>((1024 / runter) * (zeile + 1)) / 1024.0F;
            break;
        case Kodierung::Japanisch:
        case Kodierung::Chinesisch:
            g.s = static_cast<float>((1024 / q) * spalte) / 1024.0F;
            g.t = static_cast<float>((1024 / runter) * zeile) / 1024.0F;
            g.s2 = static_cast<float>(((1024 / q) * (spalte + 1)) - 1) / 1024.0F;
            g.t2 = static_cast<float>(((1024 / runter) * (zeile + 1)) - 1) / 1024.0F;
            break;
        default:
            g.s = static_cast<float>(spalte) / fq;
            g.t = static_cast<float>(zeile) / fr;
            g.s2 = static_cast<float>(spalte + 1) / fq;
            g.t2 = static_cast<float>(zeile + 1) / fr;
            break;
    }
    // UpdateAsianIfNeeded(bForceReEval): so gross wie die westliche Schrift
    const int hoch = w.height < 16 ? 16 : w.height;
    g.width = static_cast<short>(hoch);
    g.height = static_cast<short>(hoch);
    g.horizAdvance = static_cast<short>(k == Kodierung::Koreanisch ? hoch - 1 : hoch + 3);
    g.horizOffset = 0;
    g.baseline = w.ascender + ((hoch - w.height) >> 1);
    shader = asienSeite(k, seite);
    return true;
}

std::string ausUtf8(const std::string& utf8, Kodierung k, std::string* nichtDarstellbar) {
    std::string o;
    auto fehlt = [&](unsigned cp) {
        o += '?';
        if (nichtDarstellbar != nullptr) { anhaengenUtf8(*nichtDarstellbar, cp); }
    };
    for (std::size_t i = 0; i < utf8.size();) {
        const unsigned cp = naechsterCodepunkt(utf8, i);
        if (cp < 0x80) {
            o += static_cast<char>(cp);
            continue;
        }
        if (k == Kodierung::Latin1) {
            if (cp < 0x100) { o += static_cast<char>(cp); }
            else { fehlt(cp); }
            continue;
        }
        if (cp > 0xFFFF) {   // kein Zeichen der Doppelbyte-Codes
            fehlt(cp);
            continue;
        }
        const auto wc = static_cast<wchar_t>(cp);
        char buf[8] = {};
        BOOL ersatz = FALSE;
        const int n = WideCharToMultiByte(codepage(k), WC_NO_BEST_FIT_CHARS, &wc, 1, buf, sizeof(buf), nullptr, &ersatz);
        int laenge = 0;
        // nur, was die Schrift der Engine als ein Zeichen liest (AnyLanguage_ReadCharFromString)
        if (n == 2 && ersatz == FALSE && zeichenLesen(k, reinterpret_cast<const unsigned char*>(buf), 2, laenge) > 255 && laenge == 2) {
            o.append(buf, 2);
        } else {
            fehlt(cp);
        }
    }
    return o;
}

std::string zuUtf8(const std::string& bytes, Kodierung k) {
    std::string o;
    const auto* p = reinterpret_cast<const unsigned char*>(bytes.data());
    for (std::size_t i = 0; i < bytes.size();) {
        int laenge = 1;
        const unsigned c = zeichenLesen(k, p + i, bytes.size() - i, laenge);
        if (laenge == 2) {
            wchar_t wc[2] = {};
            const int n = MultiByteToWideChar(codepage(k), MB_ERR_INVALID_CHARS, bytes.data() + i, 2, wc, 2);
            anhaengenUtf8(o, n == 1 ? static_cast<unsigned>(wc[0]) : 0xFFFDU);
        } else {
            anhaengenUtf8(o, c);   // ein Byte: die Engine nimmt die westliche Glyphe (Latin-1)
        }
        i += static_cast<std::size_t>(laenge);
    }
    return o;
}

}  // namespace bhed::menue
