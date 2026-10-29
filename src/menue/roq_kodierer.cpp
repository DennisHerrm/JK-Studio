// roq_kodierer.cpp - Menue-Editor M2 Punkt 2: eigene Videos ins RoQ-Format
// schreiben (fuer cinematic in Menues).
//
// Eigener Code. Er schreibt nur die Stuecke, die der Spieler der Engine liest
// (code/client/cl_cin.cpp, portiert in roq.cpp):
//   Kopf      0x1084, 0xFFFFFFFF, fps
//   0x1001    ROQ_QUAD_INFO  xsize ysize maxsize(8) minsize(4)   (einmal)
//   0x1002    ROQ_CODEBOOK   n2 x (y0 y1 y2 y3 u v), n4 x (vier 2x2-Nummern)
//   0x1011    ROQ_QUAD_VQ    2-Bit-Codes in 16-Bit-Woertern, dazwischen die
//                            Argumentbytes in der Lesereihenfolge von
//                            blitVQQuad32fs (Block-Reihenfolge wie recurseQuad)
// Je Bild: Codebuecher per k-Means (2x2 in 6 Dimensionen Y0..Y3 U V, 4x4 als
// vier 2x2 in 24 Dimensionen), dann je 8x8-Block die billigste Wahl nach
// Fehler + lambda * Bits unter ueberspringen (Inhalt des Puffers von vor zwei
// Bildern), Bewegung (8x8 aus dem letzten Bild, -8..7), 8x8-Code (4x4-Eintrag
// verdoppelt) und Teilen in vier 4x4 (je ueberspringen, Bewegung, 4x4-Code
// oder vier 2x2-Codes). Die Fehler rechnen mit genau den Farben, die
// yuv_to_rgb24 der Engine aus dem Codebuch macht; das Rekonstruktionsbild
// folgt den zwei Puffern des Spielers (Bild n in Puffer n % 2, Bild 0 wird in
// Puffer 1 kopiert). Ein Stueck darf hoechstens 65536 Bytes haben (sonst
// bricht die Engine ab) - dann waechst lambda, bis es passt.
#include "bhed/menue/roq.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <random>

namespace bhed::menue {

namespace {

// ROQ_GenYUVTables / yuv_to_rgb24 wie roq.cpp (eigene Kopie: der Kodierer
// soll genau die Farben sehen, die der Spieler zeichnet)
struct Tabellen {
    long yy[256], ub[256], ug[256], vg[256], vr[256];
    Tabellen() {
        constexpr float tUb = 1.77200F / 2.0F * static_cast<float>(1 << 6) + 0.5F;
        constexpr float tVr = 1.40200F / 2.0F * static_cast<float>(1 << 6) + 0.5F;
        constexpr float tUg = 0.34414F / 2.0F * static_cast<float>(1 << 6) + 0.5F;
        constexpr float tVg = 0.71414F / 2.0F * static_cast<float>(1 << 6) + 0.5F;
        for (long i = 0; i < 256; i++) {
            const float x = static_cast<float>(2 * i - 255);
            ub[i] = static_cast<long>(tUb * x + (1 << 5));
            vr[i] = static_cast<long>(tVr * x + (1 << 5));
            ug[i] = static_cast<long>((-tUg * x));
            vg[i] = static_cast<long>(-tVg * x + (1 << 5));
            yy[i] = i << 6 | i >> 2;
        }
    }
    void rgb(int y, int u, int v, int* out) const {
        const long y2 = yy[y];
        out[0] = static_cast<int>(std::clamp((y2 + vr[v]) >> 6, 0L, 255L));
        out[1] = static_cast<int>(std::clamp((y2 + ug[u] + vg[v]) >> 6, 0L, 255L));
        out[2] = static_cast<int>(std::clamp((y2 + ub[u]) >> 6, 0L, 255L));
    }
};
const Tabellen& tab() {
    static const Tabellen t;
    return t;
}

struct Bild3 {   // RGB als int, schneller zu vergleichen
    int b = 0, h = 0;
    std::vector<int> p;   // b*h*3
    int* at(int x, int y) { return &p[(static_cast<std::size_t>(y) * static_cast<std::size_t>(b) + static_cast<std::size_t>(x)) * 3]; }
    const int* at(int x, int y) const { return &p[(static_cast<std::size_t>(y) * static_cast<std::size_t>(b) + static_cast<std::size_t>(x)) * 3]; }
};

// RGB -> YUV (die Umkehrung der Tabellen oben: JPEG-YCbCr, u = Cb, v = Cr)
void zuYuv(const int* rgb, float& y, float& u, float& v) {
    const float r = static_cast<float>(rgb[0]), g = static_cast<float>(rgb[1]), b = static_cast<float>(rgb[2]);
    y = 0.299F * r + 0.587F * g + 0.114F * b;
    u = (b - y) / 1.772F + 127.5F;
    v = (r - y) / 1.402F + 127.5F;
}

using V6 = std::array<float, 6>;
using V24 = std::array<float, 24>;

template <std::size_t N>
float abst(const std::array<float, N>& a, const std::array<float, N>& b) {
    float s = 0;
    for (std::size_t i = 0; i < N; ++i) {
        const float d = a[i] - b[i];
        s += d * d;
    }
    return s;
}

// k-Means (Lloyd) mit Start aus zufaelligen Punkten; leere Gruppen bekommen
// den Punkt mit dem groessten Fehler.
template <std::size_t N>
std::vector<std::array<float, N>> kmeans(const std::vector<std::array<float, N>>& pkt, int k, int runden, std::mt19937& zufall) {
    std::vector<std::array<float, N>> c;
    if (pkt.empty()) { return c; }
    k = std::min<int>(k, static_cast<int>(pkt.size()));
    std::vector<std::size_t> idx(pkt.size());
    for (std::size_t i = 0; i < idx.size(); ++i) { idx[i] = i; }
    std::shuffle(idx.begin(), idx.end(), zufall);
    for (int i = 0; i < k; ++i) { c.push_back(pkt[idx[static_cast<std::size_t>(i)]]); }
    std::vector<int> zu(pkt.size(), 0);
    std::vector<float> fehler(pkt.size(), 0);
    for (int r = 0; r < runden; ++r) {
        for (std::size_t i = 0; i < pkt.size(); ++i) {
            float best = std::numeric_limits<float>::max();
            int bi = 0;
            for (int j = 0; j < k; ++j) {
                const float d = abst(pkt[i], c[static_cast<std::size_t>(j)]);
                if (d < best) {
                    best = d;
                    bi = j;
                }
            }
            zu[i] = bi;
            fehler[i] = best;
        }
        std::vector<std::array<double, N>> summe(static_cast<std::size_t>(k));
        for (auto& s : summe) { s.fill(0.0); }
        std::vector<int> anz(static_cast<std::size_t>(k), 0);
        for (std::size_t i = 0; i < pkt.size(); ++i) {
            auto& s = summe[static_cast<std::size_t>(zu[i])];
            for (std::size_t d = 0; d < N; ++d) { s[d] += pkt[i][d]; }
            ++anz[static_cast<std::size_t>(zu[i])];
        }
        for (int j = 0; j < k; ++j) {
            const auto ju = static_cast<std::size_t>(j);
            if (anz[ju] > 0) {
                for (std::size_t d = 0; d < N; ++d) { c[ju][d] = static_cast<float>(summe[ju][d] / anz[ju]); }
            } else {
                const auto w = static_cast<std::size_t>(std::max_element(fehler.begin(), fehler.end()) - fehler.begin());
                c[ju] = pkt[w];
                fehler[w] = 0;
            }
        }
    }
    return c;
}

std::uint8_t byteVon(float f) { return static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(f)), 0, 255)); }

void put16(std::string& s, unsigned v) {
    s.push_back(static_cast<char>(v & 0xff));
    s.push_back(static_cast<char>((v >> 8) & 0xff));
}
void put32(std::string& s, unsigned v) {
    put16(s, v & 0xffff);
    put16(s, v >> 16);
}
void stueck(std::string& s, unsigned id, const std::string& daten, unsigned arg) {
    put16(s, id);
    put32(s, static_cast<unsigned>(daten.size()));
    put16(s, arg);
    s += daten;
}

// Fehler zwischen Zielblock und einem Block aus einem Bild (Quadratsumme RGB)
long blockFehler(const Bild3& z, int x, int y, const Bild3& q, int qx, int qy, int seite) {
    long s = 0;
    for (int j = 0; j < seite; ++j) {
        const int* a = z.at(x, y + j);
        const int* b = q.at(qx, qy + j);
        for (int i = 0; i < seite * 3; ++i) {
            const long d = a[i] - b[i];
            s += d * d;
        }
    }
    return s;
}

}  // namespace

bool RoqKodierer::beginne(int breite, int hoehe, const RoqOptionen& o) {
    if (breite <= 0 || hoehe <= 0 || breite % 16 != 0 || hoehe % 16 != 0 || breite > 65535 || hoehe > 65535) { return false; }
    breite_ = breite;
    hoehe_ = hoehe;
    opt_ = o;
    opt_.guete = std::clamp(opt_.guete, 1, 100);
    opt_.fps = std::clamp(opt_.fps, 1, 65535);
    bilder_ = 0;
    aus_.clear();
    put16(aus_, kRoqKennung);
    put32(aus_, 0xffffffffU);
    put16(aus_, static_cast<unsigned>(opt_.fps));
    std::string info;
    put16(info, static_cast<unsigned>(breite));
    put16(info, static_cast<unsigned>(hoehe));
    put16(info, 8);   // maxsize
    put16(info, 4);   // minsize
    stueck(aus_, kRoqQuadInfo, info, 0);
    const std::size_t n = static_cast<std::size_t>(breite) * static_cast<std::size_t>(hoehe) * 4;
    vorher_.assign(n, 0);      // Puffer 1
    vorvorher_.assign(n, 0);   // Puffer 0
    return true;
}

void RoqKodierer::bild(const RoqBild& rb) {
    if (breite_ == 0 || rb.breite != breite_ || rb.hoehe != hoehe_ || rb.rgba.size() < static_cast<std::size_t>(breite_ * hoehe_ * 4)) { return; }
    const int b = breite_, h = hoehe_;
    const Tabellen& T = tab();
    std::mt19937 zufall(1234U + static_cast<unsigned>(bilder_));   // gleiche Eingabe = gleiche Datei
    // Ziel als RGB
    Bild3 ziel;
    ziel.b = b;
    ziel.h = h;
    ziel.p.resize(static_cast<std::size_t>(b * h * 3));
    for (int i = 0; i < b * h; ++i) {
        for (int k = 0; k < 3; ++k) { ziel.p[static_cast<std::size_t>(i * 3 + k)] = rb.rgba[static_cast<std::size_t>(i * 4 + k)]; }
    }
    // Die zwei Puffer des Spielers: Bild n schreibt in Puffer n % 2, Bewegung liest den anderen
    std::vector<std::uint8_t>& ziel_puffer = (bilder_ % 2 == 0) ? vorvorher_ : vorher_;   // wird ueberschrieben
    const std::vector<std::uint8_t>& anderer = (bilder_ % 2 == 0) ? vorher_ : vorvorher_;
    auto alsBild3 = [&](const std::vector<std::uint8_t>& p) {
        Bild3 r;
        r.b = b;
        r.h = h;
        r.p.resize(static_cast<std::size_t>(b * h * 3));
        for (int i = 0; i < b * h; ++i) {
            for (int k = 0; k < 3; ++k) { r.p[static_cast<std::size_t>(i * 3 + k)] = p[static_cast<std::size_t>(i * 4 + k)]; }
        }
        return r;
    };
    const Bild3 alt = alsBild3(ziel_puffer);    // "ueberspringen" laesst das stehen
    const Bild3 vor = alsBild3(anderer);        // Bewegung
    const bool nurCodes = bilder_ == 0 || (opt_.schluesselAbstand > 0 && bilder_ % opt_.schluesselAbstand == 0);

    // --- Codebuecher ----------------------------------------------------------
    auto v6 = [&](int x, int y) {
        V6 v{};
        float u = 0, w = 0;
        for (int j = 0; j < 2; ++j) {
            for (int i = 0; i < 2; ++i) {
                float yy = 0, uu = 0, vv = 0;
                zuYuv(ziel.at(x + i, y + j), yy, uu, vv);
                v[static_cast<std::size_t>(j * 2 + i)] = yy;
                u += uu;
                w += vv;
            }
        }
        v[4] = u / 4.0F;
        v[5] = w / 4.0F;
        return v;
    };
    std::vector<V6> alle2;
    std::vector<V24> alle4;
    for (int y = 0; y < h; y += 4) {
        for (int x = 0; x < b; x += 4) {
            V24 q{};
            for (int s = 0; s < 4; ++s) {
                const V6 a = v6(x + (s % 2) * 2, y + (s / 2) * 2);
                alle2.push_back(a);
                std::copy(a.begin(), a.end(), q.begin() + s * 6);
            }
            alle4.push_back(q);
        }
    }
    // Stichprobe fuer das Lernen
    auto stichprobe = [&](auto v, std::size_t n) {
        if (v.size() > n) {
            std::shuffle(v.begin(), v.end(), zufall);
            v.resize(n);
        }
        return v;
    };
    const int runden = 4 + opt_.guete / 15;
    // Groesse der Codebuecher nach der Guete (weniger Eintraege = kleineres Stueck)
    const int eintraege = std::clamp(16 + opt_.guete * 24 / 10, 16, 256);
    std::vector<V6> cb2 = kmeans(stichprobe(alle2, 6000), eintraege, runden, zufall);
    std::vector<V24> cb4f = kmeans(stichprobe(alle4, 3000), eintraege, runden, zufall);
    if (cb2.empty()) { cb2.push_back(V6{}); }
    // Codebuch-Eintraege als Bytes und ihre Farben wie yuv_to_rgb24
    const int n2 = static_cast<int>(cb2.size());
    std::vector<std::array<std::uint8_t, 6>> cb2b(cb2.size());
    std::vector<std::array<int, 12>> cb2rgb(cb2.size());   // 4 Bildpunkte x RGB
    for (std::size_t e = 0; e < cb2.size(); ++e) {
        for (std::size_t k = 0; k < 6; ++k) { cb2b[e][k] = byteVon(cb2[e][k]); }
        for (int p = 0; p < 4; ++p) { T.rgb(cb2b[e][static_cast<std::size_t>(p)], cb2b[e][4], cb2b[e][5], &cb2rgb[e][static_cast<std::size_t>(p * 3)]); }
    }
    auto naechster2 = [&](const V6& v) {
        float best = std::numeric_limits<float>::max();
        int bi = 0;
        for (int e = 0; e < n2; ++e) {
            const float d = abst(v, cb2[static_cast<std::size_t>(e)]);
            if (d < best) {
                best = d;
                bi = e;
            }
        }
        return bi;
    };
    // 4x4-Eintraege: vier 2x2-Nummern; doppelte entfernen; die Engine kann 256
    // 4x4-Eintraege nur zusammen mit 256 2x2-Eintraegen ausdruecken (flags 0)
    std::vector<std::array<std::uint8_t, 4>> cb4;
    for (const V24& q : cb4f) {
        std::array<std::uint8_t, 4> e{};
        for (int s = 0; s < 4; ++s) {
            V6 a{};
            std::copy(q.begin() + s * 6, q.begin() + s * 6 + 6, a.begin());
            e[static_cast<std::size_t>(s)] = static_cast<std::uint8_t>(naechster2(a));
        }
        if (std::find(cb4.begin(), cb4.end(), e) == cb4.end()) { cb4.push_back(e); }
    }
    if (cb4.size() == 256 && n2 < 256) { cb4.pop_back(); }
    const int n4 = static_cast<int>(cb4.size());
    // 4x4-Farben (16 Bildpunkte, Zeilen) aus den 2x2-Eintraegen
    std::vector<std::array<int, 48>> cb4rgb(cb4.size());
    for (std::size_t e = 0; e < cb4.size(); ++e) {
        for (int s = 0; s < 4; ++s) {
            const auto& c = cb2rgb[cb4[e][static_cast<std::size_t>(s)]];
            for (int p = 0; p < 4; ++p) {
                const int px = (s % 2) * 2 + p % 2, py = (s / 2) * 2 + p / 2;
                for (int k = 0; k < 3; ++k) { cb4rgb[e][static_cast<std::size_t>((py * 4 + px) * 3 + k)] = c[static_cast<std::size_t>(p * 3 + k)]; }
            }
        }
    }

    // --- Wahl je Block -------------------------------------------------------
    struct Teil4 {   // ein 4x4 innerhalb eines geteilten 8x8
        long dSkip = 0, dMot = 0, dVq = 0, dCc = 0;
        int mot = 0, vq = 0;
        std::array<int, 4> cc{};
    };
    struct Block8 {
        int x = 0, y = 0;
        long dSkip = 0, dMot = 0, dVq = 0;
        int mot = 0, vq = 0;
        Teil4 t[4];
    };
    std::vector<Block8> bloecke;
    bloecke.reserve(static_cast<std::size_t>(b / 8 * h / 8));
    const long gross = std::numeric_limits<long>::max() / 4;
    // beste Bewegung fuer einen Block: Quelle (px - dx, py - dy), dx, dy in -8..7
    auto bewegung = [&](int x, int y, int seite, int& code) {
        long best = gross;
        code = 0x88;
        for (int dx = -8; dx <= 7; ++dx) {
            const int qx = x - dx;
            if (qx < 0 || qx + seite > b) { continue; }
            for (int dy = -8; dy <= 7; ++dy) {
                const int qy = y - dy;
                if (qy < 0 || qy + seite > h) { continue; }
                const long d = blockFehler(ziel, x, y, vor, qx, qy, seite);
                if (d < best) {
                    best = d;
                    code = (dx + 8) * 16 + (dy + 8);
                }
            }
        }
        return best;
    };
    auto vq4Fehler = [&](int x, int y, int e) {
        long s = 0;
        const auto& c = cb4rgb[static_cast<std::size_t>(e)];
        for (int j = 0; j < 4; ++j) {
            const int* a = ziel.at(x, y + j);
            for (int i = 0; i < 12; ++i) {
                const long d = a[i] - c[static_cast<std::size_t>(j * 12 + i)];
                s += d * d;
            }
        }
        return s;
    };
    auto cc2Fehler = [&](int x, int y, int e) {
        long s = 0;
        const auto& c = cb2rgb[static_cast<std::size_t>(e)];
        for (int p = 0; p < 4; ++p) {
            const int* a = ziel.at(x + p % 2, y + p / 2);
            for (int k = 0; k < 3; ++k) {
                const long d = a[k] - c[static_cast<std::size_t>(p * 3 + k)];
                s += d * d;
            }
        }
        return s;
    };
    for (int my = 0; my < h; my += 16) {
        for (int mx = 0; mx < b; mx += 16) {
            for (int q8 = 0; q8 < 4; ++q8) {
                Block8 bl;
                bl.x = mx + (q8 % 2) * 8;
                bl.y = my + (q8 / 2) * 8;
                bl.dSkip = nurCodes ? gross : blockFehler(ziel, bl.x, bl.y, alt, bl.x, bl.y, 8);
                bl.dMot = nurCodes ? gross : bewegung(bl.x, bl.y, 8, bl.mot);
                // 8x8-Code: der 4x4-Eintrag verdoppelt - Fehler = Streuung in den
                // 2x2-Gruppen + 4 x Abstand der Gruppenmittel (genau)
                {
                    long streu = 0;
                    int mittel[48];
                    for (int j = 0; j < 4; ++j) {
                        for (int i = 0; i < 4; ++i) {
                            for (int k = 0; k < 3; ++k) {
                                int s = 0;
                                int w[4];
                                for (int p = 0; p < 4; ++p) {
                                    w[p] = ziel.at(bl.x + i * 2 + p % 2, bl.y + j * 2 + p / 2)[k];
                                    s += w[p];
                                }
                                const float m = static_cast<float>(s) / 4.0F;
                                for (int p = 0; p < 4; ++p) { streu += static_cast<long>((static_cast<float>(w[p]) - m) * (static_cast<float>(w[p]) - m)); }
                                mittel[(j * 4 + i) * 3 + k] = s;   // 4 x Mittel
                            }
                        }
                    }
                    long best = gross;
                    for (int e = 0; e < n4; ++e) {
                        const auto& c = cb4rgb[static_cast<std::size_t>(e)];
                        long s = 0;
                        for (int i = 0; i < 48 && s < best; ++i) {
                            const long d = mittel[i] - 4L * c[static_cast<std::size_t>(i)];
                            s += d * d;   // (4m - 4c)^2 = 16 (m - c)^2 -> /4 unten
                        }
                        if (s < best) {
                            best = s;
                            bl.vq = e;
                        }
                    }
                    bl.dVq = n4 > 0 ? streu + best / 4 : gross;
                }
                for (int q4 = 0; q4 < 4; ++q4) {
                    Teil4& t = bl.t[q4];
                    const int x = bl.x + (q4 % 2) * 4, y = bl.y + (q4 / 2) * 4;
                    t.dSkip = nurCodes ? gross : blockFehler(ziel, x, y, alt, x, y, 4);
                    t.dMot = nurCodes ? gross : bewegung(x, y, 4, t.mot);
                    long best = gross;
                    for (int e = 0; e < n4; ++e) {
                        const long d = vq4Fehler(x, y, e);
                        if (d < best) {
                            best = d;
                            t.vq = e;
                        }
                    }
                    t.dVq = best;
                    t.dCc = 0;
                    for (int s = 0; s < 4; ++s) {
                        long b2 = gross;
                        for (int e = 0; e < n2; ++e) {
                            const long d = cc2Fehler(x + (s % 2) * 2, y + (s / 2) * 2, e);
                            if (d < b2) {
                                b2 = d;
                                t.cc[static_cast<std::size_t>(s)] = e;
                            }
                        }
                        t.dCc += b2;
                    }
                }
                bloecke.push_back(bl);
            }
        }
    }

    // --- Entscheiden (lambda waechst, bis das Stueck passt) ------------------
    const double q = (100.0 - opt_.guete) / 100.0;
    double lambda = 1.0 + q * q * 6000.0;
    std::vector<int> wahl8, wahl4;   // 0 skip, 1 mot, 2 vq, 3 teilen | 0 skip, 1 mot, 2 vq, 3 cc
    std::string vq;
    for (int versuch = 0; versuch < 40; ++versuch) {
        wahl8.assign(bloecke.size(), 0);
        wahl4.assign(bloecke.size() * 4, 0);
        for (std::size_t i = 0; i < bloecke.size(); ++i) {
            const Block8& bl = bloecke[i];
            double best = static_cast<double>(bl.dSkip) + lambda * 2;
            int w = 0;
            if (const double j = static_cast<double>(bl.dMot) + lambda * 10; j < best) { best = j; w = 1; }
            if (const double j = static_cast<double>(bl.dVq) + lambda * 10; j < best) { best = j; w = 2; }
            double teil = lambda * 2;
            for (int k = 0; k < 4; ++k) {
                const Teil4& t = bl.t[k];
                double tb = static_cast<double>(t.dSkip) + lambda * 2;
                int tw = 0;
                if (const double j = static_cast<double>(t.dMot) + lambda * 10; j < tb) { tb = j; tw = 1; }
                if (const double j = static_cast<double>(t.dVq) + lambda * 10; j < tb) { tb = j; tw = 2; }
                if (const double j = static_cast<double>(t.dCc) + lambda * 34; j < tb) { tb = j; tw = 3; }
                wahl4[i * 4 + static_cast<std::size_t>(k)] = tw;
                teil += tb;
            }
            if (teil < best) { w = 3; }
            wahl8[i] = w;
        }
        // Bitstrom wie blitVQQuad32fs ihn liest
        vq.clear();
        std::size_t wortPos = 0;
        int imWort = 8;
        unsigned wort = 0;
        auto code = [&](unsigned c) {
            if (imWort == 8) {
                if (!vq.empty() || wortPos != 0) {
                    vq[wortPos] = static_cast<char>(wort & 0xff);
                    vq[wortPos + 1] = static_cast<char>((wort >> 8) & 0xff);
                }
                wortPos = vq.size();
                vq.append(2, '\0');
                wort = 0;
                imWort = 0;
            }
            wort |= c << (14 - imWort * 2);
            ++imWort;
        };
        for (std::size_t i = 0; i < bloecke.size(); ++i) {
            const Block8& bl = bloecke[i];
            switch (wahl8[i]) {
                case 0: code(0); break;
                case 1: code(1); vq.push_back(static_cast<char>(bl.mot)); break;
                case 2: code(2); vq.push_back(static_cast<char>(bl.vq)); break;
                default:
                    code(3);
                    for (int k = 0; k < 4; ++k) {
                        const Teil4& t = bl.t[k];
                        switch (wahl4[i * 4 + static_cast<std::size_t>(k)]) {
                            case 0: code(0); break;
                            case 1: code(1); vq.push_back(static_cast<char>(t.mot)); break;
                            case 2: code(2); vq.push_back(static_cast<char>(t.vq)); break;
                            default:
                                code(3);
                                for (int s = 0; s < 4; ++s) { vq.push_back(static_cast<char>(t.cc[static_cast<std::size_t>(s)])); }
                                break;
                        }
                    }
                    break;
            }
        }
        if (!vq.empty()) {
            vq[wortPos] = static_cast<char>(wort & 0xff);
            vq[wortPos + 1] = static_cast<char>((wort >> 8) & 0xff);
        }
        if (vq.size() <= 65000) { break; }
        lambda = lambda * 1.6 + 50.0;
    }

    // --- Stuecke schreiben ------------------------------------------------------
    std::string buch;
    for (int e = 0; e < n2; ++e) {
        for (std::size_t k = 0; k < 6; ++k) { buch.push_back(static_cast<char>(cb2b[static_cast<std::size_t>(e)][k])); }
    }
    for (int e = 0; e < n4; ++e) {
        for (std::size_t k = 0; k < 4; ++k) { buch.push_back(static_cast<char>(cb4[static_cast<std::size_t>(e)][k])); }
    }
    const unsigned flags = (n2 == 256 && n4 == 256) ? 0U : (static_cast<unsigned>(n2 & 0xff) << 8) | static_cast<unsigned>(n4 & 0xff);
    stueck(aus_, kRoqCodebook, buch, flags);
    stueck(aus_, kRoqQuadVq, vq, 0);

    // --- Rekonstruktion wie der Spieler -----------------------------------------
    std::vector<std::uint8_t> neu = ziel_puffer;   // ueberspringen = stehen lassen
    auto setze = [&](int x, int y, const int* rgb) {
        const std::size_t o = (static_cast<std::size_t>(y) * static_cast<std::size_t>(b) + static_cast<std::size_t>(x)) * 4;
        neu[o] = static_cast<std::uint8_t>(rgb[0]);
        neu[o + 1] = static_cast<std::uint8_t>(rgb[1]);
        neu[o + 2] = static_cast<std::uint8_t>(rgb[2]);
        neu[o + 3] = 255;
    };
    auto kopiere = [&](int x, int y, int seite, int mot) {
        const int dx = mot / 16 - 8, dy = mot % 16 - 8;
        for (int j = 0; j < seite; ++j) {
            for (int i = 0; i < seite; ++i) {
                const std::size_t s = (static_cast<std::size_t>(y + j - dy) * static_cast<std::size_t>(b) + static_cast<std::size_t>(x + i - dx)) * 4;
                const std::size_t o = (static_cast<std::size_t>(y + j) * static_cast<std::size_t>(b) + static_cast<std::size_t>(x + i)) * 4;
                std::memcpy(&neu[o], &anderer[s], 4);
            }
        }
    };
    for (std::size_t i = 0; i < bloecke.size(); ++i) {
        const Block8& bl = bloecke[i];
        if (wahl8[i] == 1) { kopiere(bl.x, bl.y, 8, bl.mot); }
        else if (wahl8[i] == 2) {
            const auto& c = cb4rgb[static_cast<std::size_t>(bl.vq)];
            for (int j = 0; j < 8; ++j) {
                for (int k = 0; k < 8; ++k) { setze(bl.x + k, bl.y + j, &c[static_cast<std::size_t>(((j / 2) * 4 + k / 2) * 3)]); }
            }
        } else if (wahl8[i] == 3) {
            for (int k = 0; k < 4; ++k) {
                const Teil4& t = bl.t[k];
                const int x = bl.x + (k % 2) * 4, y = bl.y + (k / 2) * 4;
                const int w = wahl4[i * 4 + static_cast<std::size_t>(k)];
                if (w == 1) { kopiere(x, y, 4, t.mot); }
                else if (w == 2) {
                    const auto& c = cb4rgb[static_cast<std::size_t>(t.vq)];
                    for (int j = 0; j < 4; ++j) {
                        for (int m = 0; m < 4; ++m) { setze(x + m, y + j, &c[static_cast<std::size_t>((j * 4 + m) * 3)]); }
                    }
                } else if (w == 3) {
                    for (int s = 0; s < 4; ++s) {
                        const auto& c = cb2rgb[static_cast<std::size_t>(t.cc[static_cast<std::size_t>(s)])];
                        for (int p = 0; p < 4; ++p) { setze(x + (s % 2) * 2 + p % 2, y + (s / 2) * 2 + p / 2, &c[static_cast<std::size_t>(p * 3)]); }
                    }
                }
            }
        }
    }
    ziel_puffer = neu;
    if (bilder_ == 0) { vorher_ = vorvorher_; }   // erstes Bild: Puffer 0 nach Puffer 1
    ++bilder_;
}

std::string RoqKodierer::fertig() {
    // Die Engine liest das letzte Stueck einer Datei nie (RoQInterrupt prueft
    // RoQPlayed >= ROQSize, bevor sie es auswertet) - ein leeres Tonstueck am
    // Ende, damit das letzte Bild gezeigt wird.
    std::string r = aus_;
    stueck(r, kRoqTonMono, std::string(), 0);
    return r;
}

RoqBild bildAnpassen(const RoqBild& q, int maxSeite) {
    RoqBild r;
    if (q.breite <= 0 || q.hoehe <= 0) { return r; }
    const double f = std::min(1.0, static_cast<double>(maxSeite) / std::max(q.breite, q.hoehe));
    r.breite = std::max(16, static_cast<int>(std::lround(q.breite * f / 16.0)) * 16);
    r.hoehe = std::max(16, static_cast<int>(std::lround(q.hoehe * f / 16.0)) * 16);
    r.rgba.resize(static_cast<std::size_t>(r.breite * r.hoehe * 4));
    for (int y = 0; y < r.hoehe; ++y) {
        const double sy = std::clamp((y + 0.5) * q.hoehe / r.hoehe - 0.5, 0.0, q.hoehe - 1.0);
        const int y0 = static_cast<int>(sy), y1 = std::min(y0 + 1, q.hoehe - 1);
        const double fy = sy - y0;
        for (int x = 0; x < r.breite; ++x) {
            const double sx = std::clamp((x + 0.5) * q.breite / r.breite - 0.5, 0.0, q.breite - 1.0);
            const int x0 = static_cast<int>(sx), x1 = std::min(x0 + 1, q.breite - 1);
            const double fx = sx - x0;
            for (int k = 0; k < 4; ++k) {
                auto p = [&](int xx, int yy) { return static_cast<double>(q.rgba[static_cast<std::size_t>((yy * q.breite + xx) * 4 + k)]); };
                const double v = (p(x0, y0) * (1 - fx) + p(x1, y0) * fx) * (1 - fy) + (p(x0, y1) * (1 - fx) + p(x1, y1) * fx) * fy;
                r.rgba[static_cast<std::size_t>((y * r.breite + x) * 4 + k)] = static_cast<std::uint8_t>(std::clamp(std::lround(v), 0L, 255L));
            }
        }
    }
    return r;
}

}  // namespace bhed::menue
