// Modell-Block 1c: das bemalte Bild im Format des Originals schreiben (TGA, JPEG, PNG). Siehe bhed/modell/malen.h.
//
// JPEG: Baseline nach ITU-T T.81 (sequenziell, Huffman), Farbraum JFIF (YCbCr nach CCIR 601 wie libjpeg jccolor.c),
// 4:4:4 ohne Unterabtastung (keine Farbaufloesung verschenkt), Huffman-Tabellen aus Anhang K.3, Quantisierung mit den
// Tabellen des Originals (gleiche Qualitaetsstufe wie die Datei, die das Spiel bisher lud). Gelesen wird das Ergebnis
// von jedem Baseline-Leser, auch von der Engine (OpenJK tr_image_jpg.cpp ueber libjpeg).
#include "bhed/modell/malen.h"

#include "bhed/image.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace bhed::modell::texmalen {
namespace {

std::size_t uz(int i) { return static_cast<std::size_t>(i); }

void le16(std::string& s, int v) {
    s.push_back(static_cast<char>(v & 0xFF));
    s.push_back(static_cast<char>((v >> 8) & 0xFF));
}
void be16(std::string& s, int v) {
    s.push_back(static_cast<char>((v >> 8) & 0xFF));
    s.push_back(static_cast<char>(v & 0xFF));
}
void be32(std::string& s, std::uint32_t v) {
    for (int k = 3; k >= 0; --k) { s.push_back(static_cast<char>((v >> (k * 8)) & 0xFFU)); }
}

// --- JPEG ------------------------------------------------------------------------------------------------------------

constexpr int kZickzack[64] = {0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,  12, 19, 26, 33, 40, 48,
                               41, 34, 27, 20, 13, 6,  7,  14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23,
                               30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

// T.81 Anhang K.1 (natuerliche Folge)
constexpr int kLumaK1[64] = {16, 11, 10, 16, 24,  40,  51,  61,  12, 12, 14, 19, 26,  58,  60,  55,  14, 13, 16, 24, 40, 57,
                             69, 56, 14, 17, 22,  29,  51,  87,  80, 62, 18, 22, 37,  56,  68,  109, 103, 77, 24, 35, 55, 64,
                             81, 104, 113, 92, 49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99};
constexpr int kChromaK1[64] = {17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99, 99, 99, 99, 24, 26, 56, 99, 99, 99,
                               99, 99, 47, 66, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
                               99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99};

// T.81 Anhang K.3: Laengen je Bitzahl (1..16) und Werte
constexpr std::uint8_t kDcLumaBits[16] = {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
constexpr std::uint8_t kDcChromaBits[16] = {0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
constexpr std::uint8_t kDcWerte[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
constexpr std::uint8_t kAcLumaBits[16] = {0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d};
constexpr std::uint8_t kAcLumaWerte[162] = {
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81,
    0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18,
    0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75,
    0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
    0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5,
    0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};
constexpr std::uint8_t kAcChromaBits[16] = {0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77};
constexpr std::uint8_t kAcChromaWerte[162] = {
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08,
    0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25,
    0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba,
    0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe2, 0xe3, 0xe4,
    0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};

constexpr int bitSumme(const std::uint8_t (&b)[16]) {
    int s = 0;
    for (const std::uint8_t x : b) { s += x; }
    return s;
}
static_assert(bitSumme(kDcLumaBits) == 12 && bitSumme(kDcChromaBits) == 12, "T.81 K.3 DC");
static_assert(bitSumme(kAcLumaBits) == 162 && bitSumme(kAcChromaBits) == 162, "T.81 K.3 AC");

struct Huffman {
    std::array<std::uint16_t, 256> code{};
    std::array<std::uint8_t, 256> laenge{};
};

// T.81 Anhang C: kanonische Codes aus den Laengen
Huffman huffman(const std::uint8_t (&bits)[16], const std::uint8_t* werte) {
    Huffman h;
    int c = 0;
    int k = 0;
    for (int l = 1; l <= 16; ++l) {
        for (int i = 0; i < bits[l - 1]; ++i) {
            h.code[werte[k]] = static_cast<std::uint16_t>(c);
            h.laenge[werte[k]] = static_cast<std::uint8_t>(l);
            ++c;
            ++k;
        }
        c <<= 1;
    }
    return h;
}

struct BitSchreiber {
    std::string& out;
    std::uint32_t puffer = 0;
    int zahl = 0;
    void schreib(std::uint32_t bits, int n) {
        for (int i = n - 1; i >= 0; --i) {
            puffer = (puffer << 1U) | ((bits >> static_cast<std::uint32_t>(i)) & 1U);
            if (++zahl == 8) {
                const auto b = static_cast<char>(puffer & 0xFFU);
                out.push_back(b);
                if ((puffer & 0xFFU) == 0xFFU) { out.push_back('\0'); }   // Byte-Stopfen (T.81 F.1.2.3)
                puffer = 0;
                zahl = 0;
            }
        }
    }
    void ende() {
        while (zahl != 0) { schreib(1U, 1); }
    }
};

int kategorie(int v) {
    int a = v < 0 ? -v : v;
    int n = 0;
    while (a != 0) {
        a >>= 1;
        ++n;
    }
    return n;
}

void block(BitSchreiber& bs, const float (&px)[64], const std::array<float, 64>& qNat, int& dcVor, const Huffman& dc, const Huffman& ac) {
    // DCT-II, getrennt (T.81 A.3.3)
    static const auto kCos = [] {
        std::array<float, 64> c{};
        for (int x = 0; x < 8; ++x) {
            for (int u = 0; u < 8; ++u) {
                const double cu = u == 0 ? 1.0 / std::sqrt(2.0) : 1.0;
                c[uz(u * 8 + x)] = static_cast<float>(cu * 0.5 * std::cos((2.0 * x + 1.0) * u * 3.14159265358979323846 / 16.0));
            }
        }
        return c;
    }();
    float zw[64];
    for (int y = 0; y < 8; ++y) {
        for (int u = 0; u < 8; ++u) {
            float s = 0.0F;
            for (int x = 0; x < 8; ++x) { s += kCos[uz(u * 8 + x)] * px[y * 8 + x]; }
            zw[y * 8 + u] = s;
        }
    }
    int q[64];
    for (int v = 0; v < 8; ++v) {
        for (int u = 0; u < 8; ++u) {
            float s = 0.0F;
            for (int y = 0; y < 8; ++y) { s += kCos[uz(v * 8 + y)] * zw[y * 8 + u]; }
            q[v * 8 + u] = static_cast<int>(std::lround(s / qNat[uz(v * 8 + u)]));
        }
    }
    const int d = q[0] - dcVor;
    dcVor = q[0];
    const int kd = kategorie(d);
    bs.schreib(dc.code[uz(kd)], dc.laenge[uz(kd)]);
    if (kd != 0) { bs.schreib(static_cast<std::uint32_t>(d < 0 ? d + (1 << kd) - 1 : d), kd); }
    int lauf = 0;
    for (int i = 1; i < 64; ++i) {
        const int v = q[kZickzack[i]];
        if (v == 0) {
            ++lauf;
            continue;
        }
        while (lauf > 15) {
            bs.schreib(ac.code[0xF0], ac.laenge[0xF0]);
            lauf -= 16;
        }
        const int k = kategorie(v);
        const int sym = (lauf << 4) | k;
        bs.schreib(ac.code[uz(sym)], ac.laenge[uz(sym)]);
        bs.schreib(static_cast<std::uint32_t>(v < 0 ? v + (1 << k) - 1 : v), k);
        lauf = 0;
    }
    if (lauf > 0) { bs.schreib(ac.code[0], ac.laenge[0]); }
}

void dht(std::string& s, int klasse, const std::uint8_t (&bits)[16], const std::uint8_t* werte, int n) {
    s += "\xFF\xC4";
    be16(s, 2 + 1 + 16 + n);
    s.push_back(static_cast<char>(klasse));
    for (const std::uint8_t b : bits) { s.push_back(static_cast<char>(b)); }
    for (int i = 0; i < n; ++i) { s.push_back(static_cast<char>(werte[i])); }
}

// --- PNG -------------------------------------------------------------------------------------------------------------

std::uint32_t crc32(const std::string& s, std::size_t von, std::size_t n) {
    static const auto kTab = [] {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t i = 0; i < 256U; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k) { c = (c & 1U) != 0U ? 0xEDB88320U ^ (c >> 1U) : c >> 1U; }
            t[i] = c;
        }
        return t;
    }();
    std::uint32_t c = 0xFFFFFFFFU;
    for (std::size_t i = von; i < von + n; ++i) { c = kTab[(c ^ static_cast<std::uint8_t>(s[i])) & 0xFFU] ^ (c >> 8U); }
    return c ^ 0xFFFFFFFFU;
}

void pngTeil(std::string& out, const char* art, const std::string& daten) {
    be32(out, static_cast<std::uint32_t>(daten.size()));
    const std::size_t von = out.size();
    out += art;
    out += daten;
    be32(out, crc32(out, von, out.size() - von));
}

}  // namespace

std::vector<std::uint16_t> jpegQuant(const std::string& bytes) {
    std::array<std::vector<std::uint16_t>, 4> t;
    const auto* d = reinterpret_cast<const unsigned char*>(bytes.data());
    std::size_t i = 2;
    while (i + 4 <= bytes.size()) {
        if (d[i] != 0xFF) { break; }
        const unsigned m = d[i + 1];
        if (m == 0xD8 || (m >= 0xD0 && m <= 0xD7)) {
            i += 2;
            continue;
        }
        if (m == 0xDA || m == 0xD9) { break; }
        const std::size_t len = (static_cast<std::size_t>(d[i + 2]) << 8U) | d[i + 3];
        if (m == 0xDB) {
            std::size_t p = i + 4;
            while (p < i + 2 + len && p < bytes.size()) {
                const unsigned pq = d[p] >> 4U;
                const unsigned tq = d[p] & 3U;
                ++p;
                std::vector<std::uint16_t> q(64);
                for (int k = 0; k < 64; ++k) {
                    if (pq != 0U) {
                        if (p + 1 >= bytes.size()) { return {}; }
                        q[uz(k)] = static_cast<std::uint16_t>((d[p] << 8U) | d[p + 1]);
                        p += 2;
                    } else {
                        if (p >= bytes.size()) { return {}; }
                        q[uz(k)] = d[p];
                        ++p;
                    }
                }
                t[tq] = std::move(q);
            }
        }
        i += 2 + len;
    }
    if (t[0].empty()) { return {}; }
    std::vector<std::uint16_t> out = t[0];
    const std::vector<std::uint16_t>& c = t[1].empty() ? t[0] : t[1];
    out.insert(out.end(), c.begin(), c.end());
    return out;
}

std::string jpegSchreiben(const std::uint8_t* rgba, int b, int h, const std::vector<std::uint16_t>& quant) {
    std::vector<std::uint16_t> qz = quant;
    if (qz.size() != 128U) {
        // Anhang K mit Qualitaet 90 (Skala wie libjpeg jpeg_quality_scaling: 200 - 2q)
        qz.assign(128, 1);
        for (int i = 0; i < 64; ++i) {
            qz[uz(i)] = static_cast<std::uint16_t>(std::clamp((kLumaK1[kZickzack[i]] * 20 + 50) / 100, 1, 255));
            qz[uz(64 + i)] = static_cast<std::uint16_t>(std::clamp((kChromaK1[kZickzack[i]] * 20 + 50) / 100, 1, 255));
        }
    }
    std::array<std::array<float, 64>, 2> qNat{};
    bool breit = false;
    for (int t = 0; t < 2; ++t) {
        for (int i = 0; i < 64; ++i) {
            const std::uint16_t v = std::max<std::uint16_t>(qz[uz(t * 64 + i)], 1);
            qNat[uz(t)][uz(kZickzack[i])] = static_cast<float>(v);
            breit = breit || v > 255;
        }
    }
    std::string s = "\xFF\xD8";
    // APP0 JFIF 1.01, Seitenverhaeltnis 1:1
    s += "\xFF\xE0";
    be16(s, 16);
    s += std::string("JFIF\0", 5);
    s.push_back('\x01');
    s.push_back('\x01');
    s.push_back('\0');
    be16(s, 1);
    be16(s, 1);
    s.push_back('\0');
    s.push_back('\0');
    for (int t = 0; t < 2; ++t) {
        s += "\xFF\xDB";
        be16(s, 2 + 1 + 64 * (breit ? 2 : 1));
        s.push_back(static_cast<char>((breit ? 0x10 : 0) | t));
        for (int i = 0; i < 64; ++i) {
            const std::uint16_t v = std::max<std::uint16_t>(qz[uz(t * 64 + i)], 1);
            if (breit) { s.push_back(static_cast<char>(v >> 8U)); }
            s.push_back(static_cast<char>(v & 0xFFU));
        }
    }
    // SOF0: 8 Bit, drei Komponenten ohne Unterabtastung
    s += "\xFF\xC0";
    be16(s, 17);
    s.push_back('\x08');
    be16(s, h);
    be16(s, b);
    s.push_back('\x03');
    for (int c = 1; c <= 3; ++c) {
        s.push_back(static_cast<char>(c));
        s.push_back('\x11');
        s.push_back(static_cast<char>(c == 1 ? 0 : 1));
    }
    dht(s, 0x00, kDcLumaBits, kDcWerte, 12);
    dht(s, 0x10, kAcLumaBits, kAcLumaWerte, 162);
    dht(s, 0x01, kDcChromaBits, kDcWerte, 12);
    dht(s, 0x11, kAcChromaBits, kAcChromaWerte, 162);
    s += "\xFF\xDA";
    be16(s, 12);
    s.push_back('\x03');
    for (int c = 1; c <= 3; ++c) {
        s.push_back(static_cast<char>(c));
        s.push_back(static_cast<char>(c == 1 ? 0x00 : 0x11));
    }
    s.push_back('\0');
    s.push_back('\x3F');
    s.push_back('\0');
    const Huffman dcL = huffman(kDcLumaBits, kDcWerte);
    const Huffman acL = huffman(kAcLumaBits, kAcLumaWerte);
    const Huffman dcC = huffman(kDcChromaBits, kDcWerte);
    const Huffman acC = huffman(kAcChromaBits, kAcChromaWerte);
    BitSchreiber bs{s};
    int dc[3] = {0, 0, 0};
    for (int by = 0; by < h; by += 8) {
        for (int bx = 0; bx < b; bx += 8) {
            float y8[64];
            float cb8[64];
            float cr8[64];
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 8; ++x) {
                    // am Rand den letzten Bildpunkt wiederholen (wie libjpeg expand_right_edge / die letzte Zeile)
                    const int px = std::min(bx + x, b - 1);
                    const int py = std::min(by + y, h - 1);
                    const std::uint8_t* p = &rgba[(uz(py) * uz(b) + uz(px)) * 4U];
                    const float r = p[0];
                    const float g = p[1];
                    const float bl = p[2];
                    y8[y * 8 + x] = 0.299F * r + 0.587F * g + 0.114F * bl - 128.0F;
                    cb8[y * 8 + x] = -0.168736F * r - 0.331264F * g + 0.5F * bl;
                    cr8[y * 8 + x] = 0.5F * r - 0.418688F * g - 0.081312F * bl;
                }
            }
            block(bs, y8, qNat[0], dc[0], dcL, acL);
            block(bs, cb8, qNat[1], dc[1], dcC, acC);
            block(bs, cr8, qNat[1], dc[2], dcC, acC);
        }
    }
    bs.ende();
    s += "\xFF\xD9";
    return s;
}

std::string tgaSchreiben(const std::uint8_t* rgba, int b, int h, int bits, bool rle) {
    const int bpp = bits == 32 ? 4 : 3;
    std::string s;
    s.push_back('\0');
    s.push_back('\0');
    s.push_back(static_cast<char>(rle ? 10 : 2));
    s.append(5, '\0');
    le16(s, 0);
    le16(s, 0);
    le16(s, b);
    le16(s, h);
    s.push_back(static_cast<char>(bpp * 8));
    // Alpha-Bits, Ursprung UNTEN links (Zeilen von unten nach oben): LoadTGA der Engine weist RLE-Bilder von oben nach
    // unten ab ("RLE-RGB Images (type 10) must be in bottom-to-top format", tr_image_tga.cpp; ctest modell_malen rot) -
    // darum alle so, wie die Bilder des Spiels
    s.push_back(static_cast<char>(bpp == 4 ? 8 : 0));
    const auto px = [&](int x, int y, char* o) {
        const std::uint8_t* p = &rgba[(uz(y) * uz(b) + uz(x)) * 4U];
        o[0] = static_cast<char>(p[2]);
        o[1] = static_cast<char>(p[1]);
        o[2] = static_cast<char>(p[0]);
        o[3] = static_cast<char>(p[3]);
    };
    for (int zeile = 0; zeile < h; ++zeile) {
        const int y = h - 1 - zeile;
        if (!rle) {
            for (int x = 0; x < b; ++x) {
                char o[4];
                px(x, y, o);
                s.append(o, uz(bpp));
            }
            continue;
        }
        // Lauflaengen je Zeile (Pakete nicht ueber die Zeile, wie die TGA-Vorgabe 2.0 empfiehlt)
        int x = 0;
        while (x < b) {
            char a[4];
            px(x, y, a);
            int lauf = 1;
            while (x + lauf < b && lauf < 128) {
                char c[4];
                px(x + lauf, y, c);
                if (std::memcmp(a, c, uz(bpp)) != 0) { break; }
                ++lauf;
            }
            if (lauf >= 2) {
                s.push_back(static_cast<char>(0x80 | (lauf - 1)));
                s.append(a, uz(bpp));
                x += lauf;
                continue;
            }
            // rohes Paket bis zum naechsten Lauf
            int n = 1;
            while (x + n < b && n < 128) {
                char c[4];
                char d[4];
                px(x + n, y, c);
                if (x + n + 1 < b) {
                    px(x + n + 1, y, d);
                    if (std::memcmp(c, d, uz(bpp)) == 0) { break; }
                }
                ++n;
            }
            s.push_back(static_cast<char>(n - 1));
            for (int i = 0; i < n; ++i) {
                char c[4];
                px(x + i, y, c);
                s.append(c, uz(bpp));
            }
            x += n;
        }
    }
    return s;
}

std::string pngSchreiben(const std::uint8_t* rgba, int b, int h, bool alpha) {
    const int bpp = alpha ? 4 : 3;
    std::string roh;
    roh.reserve(uz(h) * (uz(b) * uz(bpp) + 1U));
    for (int y = 0; y < h; ++y) {
        roh.push_back('\0');   // Filter "None"
        for (int x = 0; x < b; ++x) { roh.append(reinterpret_cast<const char*>(&rgba[(uz(y) * uz(b) + uz(x)) * 4U]), uz(bpp)); }
    }
    // zlib (RFC 1950) mit ungepackten Bloecken (RFC 1951 3.2.4), Adler-32
    std::string z = "\x78\x01";
    std::size_t p = 0;
    do {
        const std::size_t n = std::min<std::size_t>(65535U, roh.size() - p);
        const bool letzter = p + n >= roh.size();
        z.push_back(letzter ? '\x01' : '\0');
        le16(z, static_cast<int>(n));
        le16(z, static_cast<int>(~n & 0xFFFFU));
        z.append(roh, p, n);
        p += n;
    } while (p < roh.size());
    std::uint32_t a = 1;
    std::uint32_t bb = 0;
    for (const char c : roh) {
        a = (a + static_cast<std::uint8_t>(c)) % 65521U;
        bb = (bb + a) % 65521U;
    }
    be32(z, (bb << 16U) | a);
    std::string s = "\x89PNG\r\n\x1a\n";
    std::string ihdr;
    be32(ihdr, static_cast<std::uint32_t>(b));
    be32(ihdr, static_cast<std::uint32_t>(h));
    ihdr.push_back('\x08');
    ihdr.push_back(static_cast<char>(alpha ? 6 : 2));
    ihdr.append(3, '\0');
    pngTeil(s, "IHDR", ihdr);
    pngTeil(s, "IDAT", z);
    pngTeil(s, "IEND", std::string());
    return s;
}

bool leinwandBytes(const Leinwand& l, std::string& out, std::string* fehler) {
    if (l.bild.size() != uz(l.b) * uz(l.h) * 4U) {
        if (fehler != nullptr) { *fehler = "no image"; }
        return false;
    }
    // unbemalt (das Ergebnis gleich dem Original): die Bytes der Datei, bytegleich
    if (!l.original.empty() && l.bild == l.grund) {
        out = l.original;
        return true;
    }
    switch (static_cast<image::Format>(l.format)) {
        case image::Format::Jpeg: out = jpegSchreiben(l.bild.data(), l.b, l.h, jpegQuant(l.original)); return true;
        case image::Format::Png: out = pngSchreiben(l.bild.data(), l.b, l.h, l.alpha); return true;
        case image::Format::Tga:
        case image::Format::Unknown: break;
    }
    out = tgaSchreiben(l.bild.data(), l.b, l.h, l.tgaBits == 24 ? 24 : (l.tgaBits == 32 || l.alpha ? 32 : 24), l.tgaRle);
    return true;
}

}  // namespace bhed::modell::texmalen
