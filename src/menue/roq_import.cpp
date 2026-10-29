// roq_import.cpp - Menue-Editor M2 Punkt 2 / M4 Punkt 5: Bildfolgen (animiertes GIF,
// Videos ueber die Oberflaeche) in ein RoQ verwandeln: Zielgroesse mit Einpassen und
// Flaechenmittel, Ton auf 22050 Hz, Codec-Gruppen fuer die Meldung fehlender Decoder.
//
// Eigener Code. Das GIF liest stb_image (public domain, Sean Barrett,
// third_party/assimp/contrib/stb/stb_image.h - nur der GIF-Teil, mit
// STB_IMAGE_STATIC in dieser Datei, getrennt von der Kopie in assimp).
#include "bhed/menue/roq.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

#if defined(_MSC_VER)
#pragma warning(push, 0)
#pragma warning(disable : 4100 4127 4244 4245 4456 4457 4505 4701 4702 6001 6011 6246 6262 6385 6386 26451)
#endif
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_GIF
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "../../third_party/assimp/contrib/stb/stb_image.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace bhed::menue {

bool gifLesen(const std::string& daten, Bildfolge& out) {
    out = Bildfolge{};
    if (daten.size() < 6 || daten.compare(0, 3, "GIF") != 0) { return false; }
    int* dauer = nullptr;
    int w = 0, h = 0, n = 0, comp = 0;
    stbi_uc* p = stbi_load_gif_from_memory(reinterpret_cast<const stbi_uc*>(daten.data()), static_cast<int>(daten.size()), &dauer, &w, &h, &n,
                                           &comp, 4);
    if (p == nullptr || w <= 0 || h <= 0 || n <= 0) {
        if (p != nullptr) { stbi_image_free(p); }
        if (dauer != nullptr) { STBI_FREE(dauer); }
        return false;
    }
    const std::size_t groesse = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4;
    for (int i = 0; i < n; ++i) {
        RoqBild b;
        b.breite = w;
        b.hoehe = h;
        b.rgba.assign(p + groesse * static_cast<std::size_t>(i), p + groesse * static_cast<std::size_t>(i + 1));
        // Durchsichtiges wie die Engine es zeichnet: auf Schwarz
        for (std::size_t k = 0; k < groesse; k += 4) {
            const unsigned a = b.rgba[k + 3];
            for (std::size_t c = 0; c < 3; ++c) { b.rgba[k + c] = static_cast<std::uint8_t>(b.rgba[k + c] * a / 255U); }
            b.rgba[k + 3] = 255;
        }
        out.bilder.push_back(std::move(b));
        // GIF: Verzoegerung in 1/100 s; 0 oder 1 spielen Browser mit 100 ms
        const int d = dauer != nullptr ? dauer[i] : 100;
        out.dauerMs.push_back(d <= 10 ? 100 : d);
    }
    stbi_image_free(p);
    if (dauer != nullptr) { STBI_FREE(dauer); }
    return true;
}

std::vector<std::size_t> aufFps(const Bildfolge& f, int fps) {
    std::vector<std::size_t> r;
    if (f.bilder.empty() || fps <= 0) { return r; }
    long long gesamt = 0;
    for (std::size_t i = 0; i < f.bilder.size(); ++i) { gesamt += i < f.dauerMs.size() ? std::max(1, f.dauerMs[i]) : 1000 / fps; }
    const long long n = std::max<long long>(1, gesamt * fps / 1000);
    std::size_t bild = 0;
    long long bisMs = f.dauerMs.empty() ? 1000 / fps : std::max(1, f.dauerMs[0]);
    for (long long k = 0; k < n; ++k) {
        const long long t = k * 1000 / fps;
        while (t >= bisMs && bild + 1 < f.bilder.size()) {
            ++bild;
            bisMs += bild < f.dauerMs.size() ? std::max(1, f.dauerMs[bild]) : 1000 / fps;
        }
        r.push_back(bild);
    }
    return r;
}

RoqGroesse roqGroessePruefen(int breite, int hoehe) {
    const auto zweierpotenz = [](int v) { return v >= 16 && (v & (v - 1)) == 0; };   // >= 16: Bloecke des Kodierers
    if (!zweierpotenz(breite) || !zweierpotenz(hoehe)) { return RoqGroesse::KeineZweierpotenz; }
    if (static_cast<long long>(breite) * hoehe > 512LL * 512LL) { return RoqGroesse::ZuGross; }
    return RoqGroesse::Ok;
}

namespace {

// Gewichte einer Achse: Zielpunkt j bekommt den Quellbereich [q0 + j * s, q0 + (j + 1) * s)
// mit s = qn / zn. Verkleinern (s >= 1): jeder ueberdeckte Quellpunkt mit seinem Anteil
// (Flaechenmittel); vergroessern: bilinear um die Mitte des Bereichs.
struct Achse {
    std::vector<int> erster;              // je Zielpunkt der erste Quellpunkt
    std::vector<std::vector<float>> w;    // Gewichte ab erster (Summe 1)
};
Achse achse(double q0, double qn, int quellMax, int zn) {
    Achse a;
    a.erster.resize(static_cast<std::size_t>(zn));
    a.w.resize(static_cast<std::size_t>(zn));
    const double s = qn / zn;
    for (int j = 0; j < zn; ++j) {
        std::vector<float>& w = a.w[static_cast<std::size_t>(j)];
        if (s >= 1.0) {
            const double von = q0 + j * s, bis = q0 + (j + 1) * s;
            const int i0 = std::clamp(static_cast<int>(std::floor(von)), 0, quellMax - 1);
            const int i1 = std::clamp(static_cast<int>(std::ceil(bis)) - 1, i0, quellMax - 1);
            a.erster[static_cast<std::size_t>(j)] = i0;
            double summe = 0;
            for (int i = i0; i <= i1; ++i) {
                const double anteil = std::max(0.0, std::min(bis, i + 1.0) - std::max(von, static_cast<double>(i)));
                w.push_back(static_cast<float>(anteil));
                summe += anteil;
            }
            if (summe <= 0) {
                w.assign(1, 1.0F);
                continue;
            }
            for (float& g : w) { g = static_cast<float>(g / summe); }
        } else {
            const double mitte = std::clamp(q0 + (j + 0.5) * s - 0.5, 0.0, quellMax - 1.0);
            const int i0 = static_cast<int>(mitte);
            const int i1 = std::min(i0 + 1, quellMax - 1);
            const auto f = static_cast<float>(mitte - i0);
            a.erster[static_cast<std::size_t>(j)] = i0;
            if (i1 == i0) {
                w.assign(1, 1.0F);
            } else {
                w = {1.0F - f, f};
            }
        }
    }
    return a;
}

// Quellbereich (qx, qy, qb, qh) auf zb x zh, getrennt erst waagerecht, dann senkrecht
std::vector<std::uint8_t> skaliereBereich(const RoqBild& q, double qx, double qy, double qb, double qh, int zb, int zh) {
    const Achse ax = achse(qx, qb, q.breite, zb);
    const Achse ay = achse(qy, qh, q.hoehe, zh);
    std::vector<float> zw(static_cast<std::size_t>(zb) * static_cast<std::size_t>(q.hoehe) * 3);
    for (int y = 0; y < q.hoehe; ++y) {
        const std::uint8_t* zeile = &q.rgba[static_cast<std::size_t>(y) * static_cast<std::size_t>(q.breite) * 4];
        for (int x = 0; x < zb; ++x) {
            float c[3] = {0, 0, 0};
            const int e = ax.erster[static_cast<std::size_t>(x)];
            const std::vector<float>& w = ax.w[static_cast<std::size_t>(x)];
            for (std::size_t k = 0; k < w.size(); ++k) {
                const std::uint8_t* px = zeile + (static_cast<std::size_t>(e) + k) * 4;
                for (int c0 = 0; c0 < 3; ++c0) { c[c0] += w[k] * px[c0]; }
            }
            float* o = &zw[(static_cast<std::size_t>(y) * static_cast<std::size_t>(zb) + static_cast<std::size_t>(x)) * 3];
            o[0] = c[0];
            o[1] = c[1];
            o[2] = c[2];
        }
    }
    std::vector<std::uint8_t> r(static_cast<std::size_t>(zb) * static_cast<std::size_t>(zh) * 4);
    for (int y = 0; y < zh; ++y) {
        const int e = ay.erster[static_cast<std::size_t>(y)];
        const std::vector<float>& w = ay.w[static_cast<std::size_t>(y)];
        for (int x = 0; x < zb; ++x) {
            float c[3] = {0, 0, 0};
            for (std::size_t k = 0; k < w.size(); ++k) {
                const float* px = &zw[((static_cast<std::size_t>(e) + k) * static_cast<std::size_t>(zb) + static_cast<std::size_t>(x)) * 3];
                for (int c0 = 0; c0 < 3; ++c0) { c[c0] += w[k] * px[c0]; }
            }
            std::uint8_t* o = &r[(static_cast<std::size_t>(y) * static_cast<std::size_t>(zb) + static_cast<std::size_t>(x)) * 4];
            for (int c0 = 0; c0 < 3; ++c0) { o[c0] = static_cast<std::uint8_t>(std::clamp(std::lround(c[c0]), 0L, 255L)); }
            o[3] = 255;
        }
    }
    return r;
}

}  // namespace

RoqBild bildSkalieren(const RoqBild& q, const VideoZiel& z) {
    RoqBild r;
    if (q.breite <= 0 || q.hoehe <= 0 || z.breite <= 0 || z.hoehe <= 0 ||
        q.rgba.size() < static_cast<std::size_t>(q.breite) * static_cast<std::size_t>(q.hoehe) * 4) {
        return r;
    }
    r.breite = z.breite;
    r.hoehe = z.hoehe;
    const double sb = static_cast<double>(z.breite) / q.breite, sh = static_cast<double>(z.hoehe) / q.hoehe;
    if (z.einpassen == Einpassen::Strecken) {
        r.rgba = skaliereBereich(q, 0, 0, q.breite, q.hoehe, z.breite, z.hoehe);
    } else if (z.einpassen == Einpassen::Zuschneiden) {
        const double s = std::max(sb, sh);
        const double qb = z.breite / s, qh = z.hoehe / s;
        r.rgba = skaliereBereich(q, (q.breite - qb) * 0.5, (q.hoehe - qh) * 0.5, qb, qh, z.breite, z.hoehe);
    } else {
        const double s = std::min(sb, sh);
        const int ib = std::clamp(static_cast<int>(std::lround(q.breite * s)), 1, z.breite);
        const int ih = std::clamp(static_cast<int>(std::lround(q.hoehe * s)), 1, z.hoehe);
        const int ox = (z.breite - ib) / 2, oy = (z.hoehe - ih) / 2;
        const std::vector<std::uint8_t> innen = skaliereBereich(q, 0, 0, q.breite, q.hoehe, ib, ih);
        r.rgba.assign(static_cast<std::size_t>(z.breite) * static_cast<std::size_t>(z.hoehe) * 4, 0);
        for (std::size_t k = 3; k < r.rgba.size(); k += 4) { r.rgba[k] = 255; }
        for (int y = 0; y < ih; ++y) {
            std::memcpy(&r.rgba[(static_cast<std::size_t>(y + oy) * static_cast<std::size_t>(z.breite) + static_cast<std::size_t>(ox)) * 4],
                        &innen[static_cast<std::size_t>(y) * static_cast<std::size_t>(ib) * 4], static_cast<std::size_t>(ib) * 4);
        }
    }
    return r;
}

RoqTon tonAuf22050(const std::vector<std::int16_t>& pcm, int rate, int kanaeleEin, int kanaeleAus) {
    RoqTon r;
    if (rate <= 0 || kanaeleEin <= 0 || (kanaeleAus != 1 && kanaeleAus != 2) || pcm.size() < static_cast<std::size_t>(kanaeleEin)) { return r; }
    r.kanaele = kanaeleAus;
    const std::size_t n = pcm.size() / static_cast<std::size_t>(kanaeleEin);
    const std::size_t ke = static_cast<std::size_t>(kanaeleEin);
    // Kanaele: mono = Mittel aller, stereo = die ersten zwei (mono verdoppelt)
    std::vector<std::vector<float>> kanal(static_cast<std::size_t>(kanaeleAus), std::vector<float>(n));
    for (std::size_t i = 0; i < n; ++i) {
        const std::int16_t* p = &pcm[i * ke];
        if (kanaeleAus == 1) {
            float s = 0;
            for (std::size_t c = 0; c < ke; ++c) { s += p[c]; }
            kanal[0][i] = s / static_cast<float>(ke);
        } else {
            kanal[0][i] = p[0];
            kanal[1][i] = ke >= 2 ? p[1] : p[0];
        }
    }
    const double verh = static_cast<double>(kRoqTonRate) / rate;   // Ausgabe je Eingabeprobe
    const std::size_t aus = rate == kRoqTonRate ? n : static_cast<std::size_t>(std::floor(static_cast<double>(n) * verh));
    r.pcm.resize(aus * static_cast<std::size_t>(kanaeleAus));
    const double grenze = std::min(1.0, verh);   // relativ zur Nyquist-Frequenz der Eingabe
    const int halb = static_cast<int>(std::ceil(16.0 / grenze));
    constexpr double kPi = 3.14159265358979323846;
    for (std::size_t m = 0; m < aus; ++m) {
        const double mitte = static_cast<double>(m) / verh;
        const long long i0 = static_cast<long long>(std::floor(mitte));
        for (int c = 0; c < kanaeleAus; ++c) {
            double v = 0;
            if (rate == kRoqTonRate) {
                v = kanal[static_cast<std::size_t>(c)][m];
            } else {
                double summe = 0, gew = 0;
                for (long long i = i0 - halb + 1; i <= i0 + halb; ++i) {
                    if (i < 0 || i >= static_cast<long long>(n)) { continue; }
                    const double x = static_cast<double>(i) - mitte;
                    const double t = grenze * x;
                    const double sinc = std::abs(t) < 1e-9 ? 1.0 : std::sin(kPi * t) / (kPi * t);
                    const double fenster = 0.5 * (1.0 + std::cos(kPi * x / halb));   // Hann
                    const double w = grenze * sinc * fenster;
                    summe += w * kanal[static_cast<std::size_t>(c)][static_cast<std::size_t>(i)];
                    gew += w;
                }
                v = gew != 0 ? summe / gew : 0.0;
            }
            r.pcm[m * static_cast<std::size_t>(kanaeleAus) + static_cast<std::size_t>(c)] =
                static_cast<std::int16_t>(std::clamp(std::lround(v), -32768L, 32767L));
        }
    }
    return r;
}

long long RoqImport::takteFuer(long long dauerMs, int fps) { return std::max<long long>(1, dauerMs * std::max(1, fps) / 1000); }

bool RoqImport::beginne(const VideoZiel& z, const RoqOptionen& o, const RoqTon* ton) {
    ziel_ = z;
    fps_ = std::max(1, o.fps);
    takt_ = 0;
    erste_ = -1;
    quelle_ = RoqBild{};
    aktuell_ = RoqBild{};
    skaliert_ = false;
    return z.breite % 16 == 0 && z.hoehe % 16 == 0 && kod_.beginne(z.breite, z.hoehe, o, ton);
}

void RoqImport::zeige() {
    if (!skaliert_) {
        aktuell_ = bildSkalieren(quelle_, ziel_);
        skaliert_ = true;
    }
    kod_.bild(aktuell_);
    ++takt_;
}

void RoqImport::bisZeit(long long zeitMs) {
    // Takt k zeigt das Bild, das bei k * 1000 / fps gilt
    while (!quelle_.rgba.empty() && static_cast<long long>(takt_) * 1000 / fps_ < zeitMs) { zeige(); }
}

void RoqImport::bild(const RoqBild& quelle, long long zeitMs) {
    if (erste_ < 0) { erste_ = zeitMs; }
    bisZeit(zeitMs - erste_);
    quelle_ = quelle;
    skaliert_ = false;
}

std::string RoqImport::fertig(long long dauerMs) {
    if (quelle_.rgba.empty()) { return {}; }
    const long long n = takteFuer(dauerMs, fps_);
    while (takt_ < n) { zeige(); }
    return kod_.fertig();
}

std::string kodiereRoq(const Bildfolge& f, const RoqOptionen& o, const VideoZiel& z, const RoqTon* ton,
                       const std::function<bool(int, int)>& fortschritt) {
    if (f.bilder.empty()) { return {}; }
    const std::vector<std::size_t> folge = aufFps(f, o.fps);
    RoqKodierer k;
    if (!k.beginne(z.breite, z.hoehe, o, ton)) { return {}; }
    std::size_t zuletzt = static_cast<std::size_t>(-1);
    RoqBild angepasst;
    for (std::size_t i = 0; i < folge.size(); ++i) {
        if (folge[i] != zuletzt) {
            angepasst = bildSkalieren(f.bilder[folge[i]], z);
            zuletzt = folge[i];
        }
        k.bild(angepasst);
        if (fortschritt && !fortschritt(static_cast<int>(i + 1), static_cast<int>(folge.size()))) { return {}; }
    }
    return k.fertig();
}

CodecGruppe codecGruppe(std::uint32_t fourcc) {
    // FourCC klein geschrieben vergleichen (Media Foundation: MFVideoFormat_HEVC = HEVC,
    // MFVideoFormat_AV1 = AV01, MFVideoFormat_VP80/VP90, AVI: DIVX, DX50, XVID ...)
    std::string t = fourccText(fourcc);
    for (char& c : t) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    if (t == "hevc" || t == "h265" || t == "hvc1" || t == "hev1" || t == "hevs") { return CodecGruppe::Hevc; }
    if (t == "av01" || t == "av1 ") { return CodecGruppe::Av1; }
    if (t == "vp80") { return CodecGruppe::Vp8; }
    if (t == "vp90" || t == "vp09") { return CodecGruppe::Vp9; }
    if (t == "divx" || t == "dx50" || t == "div3" || t == "div4" || t == "div5" || t == "xvid" || t == "fmp4" || t == "dvx3") { return CodecGruppe::DivxXvid; }
    return CodecGruppe::Unbekannt;
}

std::string fourccText(std::uint32_t fourcc) {
    std::string t;
    for (int i = 0; i < 4; ++i) {
        const auto c = static_cast<char>((fourcc >> (8 * i)) & 0xff);
        t.push_back(std::isprint(static_cast<unsigned char>(c)) != 0 ? c : '?');
    }
    return t;
}

}  // namespace bhed::menue
