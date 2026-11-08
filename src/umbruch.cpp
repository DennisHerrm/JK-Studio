// umbruch.cpp - siehe bhed/umbruch.h

#include "bhed/umbruch.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace bhed {

namespace {

// UTF-8: Zeichen ab i, Laenge in n (kaputte Folgen als einzelnes Byte)
std::uint32_t zeichen(std::string_view t, std::size_t i, std::size_t& n) {
    const auto b = static_cast<unsigned char>(t[i]);
    std::size_t laenge = 1;
    std::uint32_t c = b;
    if (b >= 0xF0U) {
        laenge = 4;
        c = b & 0x07U;
    } else if (b >= 0xE0U) {
        laenge = 3;
        c = b & 0x0FU;
    } else if (b >= 0xC0U) {
        laenge = 2;
        c = b & 0x1FU;
    }
    if (laenge > 1) {
        if (i + laenge > t.size()) {
            n = 1;
            return b;
        }
        for (std::size_t k = 1; k < laenge; ++k) {
            const auto f = static_cast<unsigned char>(t[i + k]);
            if ((f & 0xC0U) != 0x80U) {
                n = 1;
                return b;
            }
            c = (c << 6U) | (f & 0x3FU);
        }
    }
    n = laenge;
    return c;
}

bool cjk(std::uint32_t c) {
    return (c >= 0x2E80U && c <= 0x9FFFU) ||   // Radikale, CJK-Satzzeichen, Kana, Bopomofo, Ideogramme
           (c >= 0xAC00U && c <= 0xD7AFU) ||   // Hangul
           (c >= 0xF900U && c <= 0xFAFFU) ||   // Kompatibilitaetsideogramme
           (c >= 0xFE30U && c <= 0xFE4FU) ||   // CJK-Kompatibilitaetsformen
           (c >= 0xFF00U && c <= 0xFFEFU) ||   // Voll- und Halbbreite
           (c >= 0x20000U && c <= 0x3FFFFU);   // Ideogramme Ebene 2 und 3
}

bool leer(std::uint32_t c) { return c == ' ' || c == '\t' || c == 0x3000U; }

// JIS X 4051 (gyoto kinsoku) / GB/T 15834: nicht am Zeilenanfang
bool nichtAmAnfang(std::uint32_t c) {
    static constexpr std::u32string_view kListe =
        U"\u3001\u3002\uFF0C\uFF0E\u30FB\uFF1A\uFF1B\uFF1F\uFF01\u30FC\u2010\uFF5E\u2026\u2025\uFF09\u3015\uFF3D\uFF5D\u3009\u300B\u300D\u300F\u3011\u3019\u3017\u301F\u2019\u201D\uFF60\u00BB\u309D\u309E\u30FD\u30FE\u3005\u303B\u3041\u3043\u3045\u3047\u3049\u3063\u3083\u3085\u3087\u308E\u3095\u3096\u30A1\u30A3\u30A5\u30A7\u30A9\u30C3\u30E3\u30E5\u30E7\u30EE\u30F5\u30F6\u31F0\u31F1\u31F2\u31F3\u31F4\u31F5\u31F6\u31F7\u31F8\u31F9\u31FA\u31FB\u31FC\u31FD\u31FE\u31FF"
        U"\u309B\u309C%),.:;?!]}";
    return kListe.find(static_cast<char32_t>(c)) != std::u32string_view::npos;
}

// gyomatsu kinsoku: nicht am Zeilenende
bool nichtAmEnde(std::uint32_t c) {
    static constexpr std::u32string_view kListe = U"\uFF08\u3014\uFF3B\uFF5B\u3008\u300A\u300C\u300E\u3010\u3018\u3016\u301D\u2018\u201C\uFF5F\u00AB([{";
    return kListe.find(static_cast<char32_t>(c)) != std::u32string_view::npos;
}

struct Stueck {
    std::size_t a = 0;
    std::size_t e = 0;
    bool leer = false;
    bool zeile = false;   // '\n'
    std::uint32_t erstes = 0;
    std::uint32_t letztes = 0;
};

}  // namespace

bool hatCjk(std::string_view text) {
    for (std::size_t i = 0; i < text.size();) {
        std::size_t n = 1;
        if (cjk(zeichen(text, i, n))) { return true; }
        i += n;
    }
    return false;
}

namespace {
// Schritt 1 und 2 von umbruchZeilen: die Stuecke mit den Sperren zu Gruppen zusammengefasst
std::vector<Stueck> gruppenBilden(std::string_view text) {
    // 1. Stuecke: Leerraum, Zeilenwechsel, je CJK-Zeichen eins, sonst Woerter bis zum naechsten Leerraum oder CJK-Zeichen
    std::vector<Stueck> st;
    for (std::size_t i = 0; i < text.size();) {
        std::size_t n = 1;
        const std::uint32_t c = zeichen(text, i, n);
        Stueck s;
        s.a = i;
        s.erstes = c;
        s.letztes = c;
        if (c == '\n') {
            s.zeile = true;
            s.e = i + n;
        } else if (leer(c)) {
            s.leer = true;
            std::size_t j = i + n;
            while (j < text.size()) {
                std::size_t m = 1;
                if (!leer(zeichen(text, j, m))) { break; }
                j += m;
            }
            s.e = j;
        } else if (cjk(c)) {
            s.e = i + n;
        } else {
            std::size_t j = i + n;
            while (j < text.size()) {
                std::size_t m = 1;
                const std::uint32_t d = zeichen(text, j, m);
                if (d == '\n' || leer(d) || cjk(d)) { break; }
                s.letztes = d;
                j += m;
            }
            s.e = j;
        }
        st.push_back(s);
        i = s.e;
    }
    // 2. Sperren: was nicht am Zeilenanfang stehen darf, haengt am Stueck davor; eine oeffnende Klammer am Stueck danach
    std::vector<Stueck> g;
    for (const Stueck& s : st) {
        const bool anhaengen = !g.empty() && !g.back().leer && !g.back().zeile && !s.leer && !s.zeile &&
                               (nichtAmAnfang(s.erstes) || nichtAmEnde(g.back().letztes));
        if (anhaengen) {
            g.back().e = s.e;
            g.back().letztes = s.letztes;
        } else {
            g.push_back(s);
        }
    }
    return g;
}
}  // namespace

std::size_t gruppeEnde(std::string_view text, std::size_t ab) {
    for (const Stueck& s : gruppenBilden(text)) {
        if (s.a >= ab && !s.leer && !s.zeile) { return s.e; }
    }
    return text.size();
}

std::vector<std::pair<std::size_t, std::size_t>> umbruchZeilen(std::string_view text, float breite,
                                                               const std::function<float(std::string_view)>& messen) {
    const std::vector<Stueck> g = gruppenBilden(text);
    // 3. gierig fuellen
    std::vector<std::pair<std::size_t, std::size_t>> zeilen;
    std::size_t za = 0;
    std::size_t ze = 0;
    bool offen = false;
    float w = 0.0F;
    float leerBreite = 0.0F;
    const auto schliessen = [&] {
        zeilen.emplace_back(za, offen ? ze : za);
        offen = false;
        w = 0.0F;
        leerBreite = 0.0F;
    };
    for (const Stueck& s : g) {
        if (s.zeile) {
            if (offen) {
                schliessen();
            } else {
                zeilen.emplace_back(s.a, s.a);
            }
            continue;
        }
        if (s.leer) {
            if (offen) { leerBreite += messen(text.substr(s.a, s.e - s.a)); }
            continue;   // am Zeilenanfang entfallen sie (wie ImGui)
        }
        const float sw = messen(text.substr(s.a, s.e - s.a));
        if (offen && w + leerBreite + sw > breite) { schliessen(); }
        if (!offen && sw > breite) {
            // zu lang fuer eine Zeile: zeichenweise teilen (wie ImGui "Cut words that cannot possibly fit")
            std::size_t i = s.a;
            while (i < s.e) {
                std::size_t j = i;
                float ww = 0.0F;
                while (j < s.e) {
                    std::size_t n = 1;
                    (void)zeichen(text, j, n);
                    const float cw = messen(text.substr(j, n));
                    if (j > i && ww + cw > breite) { break; }
                    ww += cw;
                    j += n;
                }
                za = i;
                ze = j;
                offen = true;
                w = ww;
                if (j < s.e) { schliessen(); }
                i = j;
            }
            continue;
        }
        if (!offen) {
            za = s.a;
            offen = true;
            w = 0.0F;
            leerBreite = 0.0F;
        }
        w += leerBreite + sw;
        leerBreite = 0.0F;
        ze = s.e;
    }
    if (offen) { schliessen(); }
    return zeilen;
}

}  // namespace bhed
