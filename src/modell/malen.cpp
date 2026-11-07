// Modell-Block 1c: Texture Painting - Leinwand, Ebenen, Pinsel, Rueckgaengig in Kacheln, Ausbluten, Umbacken.
// Siehe bhed/modell/malen.h. Neu geschrieben (WhiteoutFlakes hat kein Texture Painting).
#include "bhed/modell/malen.h"

#include "bhed/image.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <utility>

namespace bhed::modell::texmalen {
namespace {

std::size_t uz(int i) { return static_cast<std::size_t>(i); }

int wickel(int x, int n) {
    const int r = x % n;
    return r < 0 ? r + n : r;
}

std::uint8_t byteVon(float x) { return static_cast<std::uint8_t>(std::clamp(x * 255.0F + 0.5F, 0.0F, 255.0F)); }
float wertVon(std::uint8_t x) { return static_cast<float>(x) / 255.0F; }

// "over" mit nicht vormultipliziertem Alpha: c mit Deckung a ueber dem Texel
void ueber(std::uint8_t* t, const float c[3], float a) {
    if (a <= 0.0F) { return; }
    const float ta = wertVon(t[3]);
    const float oa = a + ta * (1.0F - a);
    for (int k = 0; k < 3; ++k) {
        const float tc = wertVon(t[k]);
        t[k] = byteVon(oa > 0.0F ? (c[k] * a + tc * ta * (1.0F - a)) / oa : c[k]);
    }
    t[3] = byteVon(oa);
}

float mische(Mischart m, float r, float l) {
    switch (m) {
        case Mischart::Normal: return l;
        case Mischart::Multiplizieren: return r * l;
        case Mischart::Addieren: return std::min(1.0F, r + l);
        case Mischart::Aufhellen: return 1.0F - (1.0F - r) * (1.0F - l);
        case Mischart::Ineinander: return r < 0.5F ? 2.0F * r * l : 1.0F - 2.0F * (1.0F - r) * (1.0F - l);
    }
    return l;
}

void bilinear(const std::vector<std::uint8_t>& q, int b, int h, float u, float v, float out[4]) {
    // wie GL_REPEAT + GL_LINEAR: Texelmitten bei (i + 0,5) / b
    const float x = u * static_cast<float>(b) - 0.5F;
    const float y = v * static_cast<float>(h) - 0.5F;
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const float fx = x - static_cast<float>(x0);
    const float fy = y - static_cast<float>(y0);
    const auto px = [&](int xx, int yy, int k) {
        return static_cast<float>(q[(uz(wickel(yy, h)) * uz(b) + uz(wickel(xx, b))) * 4U + uz(k)]);
    };
    for (int k = 0; k < 4; ++k) {
        const float a = px(x0, y0, k) * (1.0F - fx) + px(x0 + 1, y0, k) * fx;
        const float c = px(x0, y0 + 1, k) * (1.0F - fx) + px(x0 + 1, y0 + 1, k) * fx;
        out[k] = a * (1.0F - fy) + c * fy;
    }
}

// Die Texel, deren Mitte im UV-Dreieck liegt (gewickelt), mit Schwerpunktkoordinaten
template <class F>
void texelImDreieck(const Dreieck& d, int b, int h, F&& f) {
    float p[3][2];
    for (int e = 0; e < 3; ++e) {
        p[e][0] = d.uv[e][0] * static_cast<float>(b);
        p[e][1] = d.uv[e][1] * static_cast<float>(h);
    }
    const float fl = (p[1][0] - p[0][0]) * (p[2][1] - p[0][1]) - (p[1][1] - p[0][1]) * (p[2][0] - p[0][0]);
    if (std::fabs(fl) < 1e-12F) { return; }
    const float x0 = std::min({p[0][0], p[1][0], p[2][0]});
    const float x1 = std::max({p[0][0], p[1][0], p[2][0]});
    const float y0 = std::min({p[0][1], p[1][1], p[2][1]});
    const float y1 = std::max({p[0][1], p[1][1], p[2][1]});
    // gegen entartete UV (ein Dreieck ueber viele Kacheln): hoechstens viermal die Bildgroesse
    if (x1 - x0 > 4.0F * static_cast<float>(b) || y1 - y0 > 4.0F * static_cast<float>(h)) { return; }
    for (int y = static_cast<int>(std::floor(y0)); y <= static_cast<int>(std::ceil(y1)); ++y) {
        for (int x = static_cast<int>(std::floor(x0)); x <= static_cast<int>(std::ceil(x1)); ++x) {
            const float u = static_cast<float>(x) + 0.5F;
            const float v = static_cast<float>(y) + 0.5F;
            const float l0 = ((p[1][0] - u) * (p[2][1] - v) - (p[1][1] - v) * (p[2][0] - u)) / fl;
            const float l1 = ((p[2][0] - u) * (p[0][1] - v) - (p[2][1] - v) * (p[0][0] - u)) / fl;
            const float l2 = 1.0F - l0 - l1;
            if (l0 < -1e-4F || l1 < -1e-4F || l2 < -1e-4F) { continue; }
            f(wickel(x, b), wickel(y, h), l0, l1, l2);
        }
    }
}

std::vector<std::int32_t> blutKarte(const std::vector<std::uint8_t>& bed, int b, int h, int schritte) {
    std::vector<std::int32_t> q(bed.size(), -1);
    std::vector<int> abst(bed.size(), -1);
    std::deque<std::int32_t> offen;
    for (std::size_t i = 0; i < bed.size(); ++i) {
        if (bed[i] != 0) {
            abst[i] = 0;
            q[i] = static_cast<std::int32_t>(i);
            offen.push_back(static_cast<std::int32_t>(i));
        }
    }
    while (!offen.empty()) {
        const std::int32_t i = offen.front();
        offen.pop_front();
        const int a = abst[uz(i)];
        if (a >= schritte) { continue; }
        const int x = i % b;
        const int y = i / b;
        const int nb[4][2] = {{x + 1, y}, {x - 1, y}, {x, y + 1}, {x, y - 1}};
        for (const auto& n : nb) {
            if (n[0] < 0 || n[1] < 0 || n[0] >= b || n[1] >= h) { continue; }
            const std::size_t j = uz(n[1]) * uz(b) + uz(n[0]);
            if (abst[j] >= 0) { continue; }
            abst[j] = a + 1;
            q[j] = q[uz(i)];
            offen.push_back(static_cast<std::int32_t>(j));
        }
    }
    // bedeckte Texel sind keine Ziele
    for (std::size_t i = 0; i < bed.size(); ++i) {
        if (bed[i] != 0) { q[i] = -1; }
    }
    return q;
}

std::size_t kachelZahl(const Leinwand& l) {
    return uz((l.b + kKachel - 1) / kKachel) * uz((l.h + kKachel - 1) / kKachel);
}

std::vector<std::uint8_t>& ebenenBytes(Leinwand& l, int ebene) { return ebene < 0 ? l.maske : l.ebenen[uz(ebene)].rgba; }

void kachelKopieren(const Leinwand& l, const std::vector<std::uint8_t>& quelle, int bpp, int kx, int ky, std::vector<std::uint8_t>& out) {
    const int x0 = kx * kKachel;
    const int y0 = ky * kKachel;
    const int bw = std::min(kKachel, l.b - x0);
    const int bh = std::min(kKachel, l.h - y0);
    out.resize(uz(bw * bh * bpp));
    for (int y = 0; y < bh; ++y) {
        std::memcpy(&out[uz(y * bw * bpp)], &quelle[(uz(y0 + y) * uz(l.b) + uz(x0)) * uz(bpp)], uz(bw * bpp));
    }
}

void kachelSetzen(const Leinwand& l, std::vector<std::uint8_t>& ziel, int bpp, int kx, int ky, const std::vector<std::uint8_t>& in) {
    const int x0 = kx * kKachel;
    const int y0 = ky * kKachel;
    const int bw = std::min(kKachel, l.b - x0);
    const int bh = std::min(kKachel, l.h - y0);
    for (int y = 0; y < bh; ++y) {
        std::memcpy(&ziel[(uz(y0 + y) * uz(l.b) + uz(x0)) * uz(bpp)], &in[uz(y * bw * bpp)], uz(bw * bpp));
    }
}

std::size_t schrittBytes(const UndoSchritt& s) {
    std::size_t n = s.maskeVorher.size() + s.maskeNachher.size();
    for (const Kachel& k : s.kacheln) { n += k.vorher.size() + k.nachher.size(); }
    for (const Ebene& e : s.stapelVorher) { n += e.rgba.size(); }
    for (const Ebene& e : s.stapelNachher) { n += e.rgba.size(); }
    return n;
}

void geaendert(Leinwand& l, const Rechteck& r) {
    l.schmutzig.dazu(r);
    ++l.stand;
}

Rechteck ganz(const Leinwand& l) { return Rechteck{0, 0, l.b - 1, l.h - 1}; }

// Ausbluten im Rechteck (um `ausbluten` erweitert): unbedeckte Texel nehmen den Wert ihres naechsten bedeckten - nur, wenn
// dieser im Tupfer bemalt wurde (tupfMarke), und so weit die Maske des Ziels es zulaesst (bemalen = 0: Maske selbst)
void blutenIn(Leinwand& l, std::vector<std::uint8_t>& px, int bpp, const Rechteck& r, bool maskeAchten) {
    if (l.blutQuelle.empty() || r.leer()) { return; }
    const int x0 = std::max(0, r.x0 - l.ausbluten);
    const int y0 = std::max(0, r.y0 - l.ausbluten);
    const int x1 = std::min(l.b - 1, r.x1 + l.ausbluten);
    const int y1 = std::min(l.h - 1, r.y1 + l.ausbluten);
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const std::size_t z = l.texel(x, y);
            const std::int32_t q = l.blutQuelle[z];
            if (q < 0 || l.tupfMarke[uz(q)] != l.tupfNr) { continue; }
            const float frei = maskeAchten ? 1.0F - wertVon(l.maske[z]) : 1.0F;
            if (frei <= 0.0F) { continue; }
            for (int k = 0; k < bpp; ++k) {
                std::uint8_t& t = px[z * uz(bpp) + uz(k)];
                t = static_cast<std::uint8_t>(std::lround(static_cast<float>(t) * (1.0F - frei) + static_cast<float>(px[uz(q) * uz(bpp) + uz(k)]) * frei));
            }
        }
    }
}

Rechteck erweitert(const Leinwand& l, const Rechteck& r) {
    if (r.leer()) { return r; }
    return Rechteck{std::max(0, r.x0 - l.ausbluten), std::max(0, r.y0 - l.ausbluten), std::min(l.b - 1, r.x1 + l.ausbluten),
                    std::min(l.h - 1, r.y1 + l.ausbluten)};
}

// Die gesammelten Texel eines Tupfers (je Texel der staerkste Abfall) mit dem Werkzeug auftragen
struct Treffer {
    std::int32_t i;
    float f;
};

int auftragen(Leinwand& l, Verlauf& v, const Pinsel& p, const std::vector<Treffer>& tr, const float richtung[2]) {
    if (tr.empty()) { return 0; }
    Rechteck r;
    for (const Treffer& t : tr) {
        r.dazu(t.i % l.b, t.i / l.b);
        l.tupfMarke[uz(t.i)] = l.tupfNr;   // die Quellen des Ausblutens (beim Fuellen hier gesetzt)
    }
    // Weichzeichnen/Verwischen/Klonen lesen ihre Nachbarn: das Rechteck um den Lesebereich
    const bool maske = p.werkzeug == Werkzeug::Maske;
    const int ebene = maske ? -1 : std::clamp(l.aktiv, 0, static_cast<int>(l.ebenen.size()) - 1);
    const Rechteck geschrieben = erweitert(l, r);
    sichern(v, l, ebene, geschrieben);
    std::vector<std::uint8_t>& px = ebenenBytes(l, ebene);
    const std::vector<std::uint8_t> vorher = (p.werkzeug == Werkzeug::Weichzeichnen || p.werkzeug == Werkzeug::Verwischen) ? px : std::vector<std::uint8_t>{};
    const float farbe[3] = {p.farbe[0], p.farbe[1], p.farbe[2]};
    int n = 0;
    for (const Treffer& t : tr) {
        const std::size_t i = uz(t.i);
        const float frei = maske ? 1.0F : 1.0F - wertVon(l.maske[i]);
        const float f = std::clamp(t.f * frei, 0.0F, 1.0F);
        if (f <= 0.0F) { continue; }
        const int x = t.i % l.b;
        const int y = t.i / l.b;
        std::uint8_t* q = maske ? &px[i] : &px[i * 4U];
        switch (p.werkzeug) {
            case Werkzeug::Pinsel: ueber(q, farbe, f * p.farbe[3]); break;
            case Werkzeug::Radierer:
                if (ebene == 0) {
                    for (int k = 0; k < 4; ++k) {
                        q[k] = byteVon(wertVon(q[k]) * (1.0F - f) + wertVon(l.grund[i * 4U + uz(k)]) * f);
                    }
                } else {
                    q[3] = byteVon(wertVon(q[3]) * (1.0F - f));
                }
                break;
            case Werkzeug::Maske: q[0] = byteVon(wertVon(q[0]) * (1.0F - f) + (p.maskeLoeschen ? 0.0F : 1.0F) * f); break;
            case Werkzeug::Weichzeichnen: {
                float s[4]{};
                int z = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const std::size_t j = l.texel(wickel(x + dx, l.b), wickel(y + dy, l.h)) * 4U;
                        for (int k = 0; k < 4; ++k) { s[k] += static_cast<float>(vorher[j + uz(k)]); }
                        ++z;
                    }
                }
                for (int k = 0; k < 4; ++k) {
                    q[k] = static_cast<std::uint8_t>(std::clamp(static_cast<float>(vorher[i * 4U + uz(k)]) * (1.0F - f) + s[k] / static_cast<float>(z) * f + 0.5F, 0.0F, 255.0F));
                }
                break;
            }
            case Werkzeug::Verwischen: {
                float s[4];
                const float u = (static_cast<float>(x) + 0.5F) / static_cast<float>(l.b) - (richtung != nullptr ? richtung[0] : 0.0F);
                const float w = (static_cast<float>(y) + 0.5F) / static_cast<float>(l.h) - (richtung != nullptr ? richtung[1] : 0.0F);
                bilinear(vorher, l.b, l.h, u, w, s);
                for (int k = 0; k < 4; ++k) {
                    q[k] = static_cast<std::uint8_t>(std::clamp(static_cast<float>(vorher[i * 4U + uz(k)]) * (1.0F - f) + s[k] * f + 0.5F, 0.0F, 255.0F));
                }
                break;
            }
            case Werkzeug::Klonen: {
                float s[4];
                const float u = (static_cast<float>(x) + 0.5F) / static_cast<float>(l.b) + p.klonVersatz[0];
                const float w = (static_cast<float>(y) + 0.5F) / static_cast<float>(l.h) + p.klonVersatz[1];
                bilinear(l.bild, l.b, l.h, u, w, s);
                const float c[3] = {s[0] / 255.0F, s[1] / 255.0F, s[2] / 255.0F};
                ueber(q, c, f * s[3] / 255.0F);
                break;
            }
            case Werkzeug::Fuellen: ueber(q, farbe, f * p.farbe[3]); break;
        }
        ++n;
    }
    blutenIn(l, px, maske ? 1 : 4, r, !maske);
    geaendert(l, geschrieben);
    zusammensetzen(l, geschrieben);
    return n;
}

float druckFaktor(bool an, float druck) { return an ? std::clamp(druck, 0.0F, 1.0F) : 1.0F; }

void tupfAnfang(Leinwand& l) {
    if (l.tupfMarke.size() != uz(l.b) * uz(l.h)) {
        l.tupfMarke.assign(uz(l.b) * uz(l.h), 0);
        l.tupfWert.assign(uz(l.b) * uz(l.h), 0.0F);
        l.tupfNr = 0;
    }
    ++l.tupfNr;
}

void tupfMerken(Leinwand& l, std::vector<Treffer>& tr, std::size_t i, float f) {
    if (l.tupfMarke[i] != l.tupfNr) {
        l.tupfMarke[i] = l.tupfNr;
        l.tupfWert[i] = f;
        tr.push_back(Treffer{static_cast<std::int32_t>(i), 0.0F});
    } else {
        l.tupfWert[i] = std::max(l.tupfWert[i], f);
    }
}

}  // namespace

float abfall(float abstand, float radius, float haerte) noexcept {
    if (radius <= 0.0F || abstand >= radius) { return 0.0F; }
    const float d = abstand / radius;
    const float hh = std::clamp(haerte, 0.0F, 1.0F);
    if (d <= hh) { return 1.0F; }
    const float t = (d - hh) / std::max(1.0F - hh, 1e-6F);
    return 1.0F - t * t * (3.0F - 2.0F * t);
}

void Rechteck::dazu(int x, int y) noexcept {
    if (leer()) {
        x0 = x1 = x;
        y0 = y1 = y;
        return;
    }
    x0 = std::min(x0, x);
    y0 = std::min(y0, y);
    x1 = std::max(x1, x);
    y1 = std::max(y1, y);
}

void Rechteck::dazu(const Rechteck& r) noexcept {
    if (r.leer()) { return; }
    dazu(r.x0, r.y0);
    dazu(r.x1, r.y1);
}

bool leinwandAusDatei(Leinwand& l, const std::string& vfsPfad, const std::string& bytes, std::string* fehler) {
    const auto* d = reinterpret_cast<const unsigned char*>(bytes.data());
    const image::Image im = image::decode(d, bytes.size());
    if (!im.ok || im.width <= 0 || im.height <= 0) {
        if (fehler != nullptr) { *fehler = im.error.empty() ? std::string("unreadable image") : im.error; }
        return false;
    }
    l = Leinwand{};
    l.datei = vfsPfad;
    l.original = bytes;
    l.format = static_cast<int>(image::sniff(d, bytes.size()));
    l.b = im.width;
    l.h = im.height;
    l.alpha = im.hasAlpha;
    if (static_cast<image::Format>(l.format) == image::Format::Tga && bytes.size() >= 18U) {
        l.tgaBits = d[16];
        l.tgaRle = d[2] >= 9;
    }
    l.ebenen.push_back(Ebene{"Original", im.rgba, 1.0F, Mischart::Normal, true});
    l.grund = im.rgba;
    l.bild = im.rgba;
    l.maske.assign(uz(l.b) * uz(l.h), 0);
    return true;
}

void leinwandLeer(Leinwand& l, int b, int h, std::array<std::uint8_t, 4> farbe) {
    l = Leinwand{};
    l.b = b;
    l.h = h;
    l.format = static_cast<int>(image::Format::Tga);
    l.tgaBits = 32;
    l.alpha = true;
    std::vector<std::uint8_t> px(uz(b) * uz(h) * 4U);
    for (std::size_t i = 0; i < px.size(); i += 4U) { std::memcpy(&px[i], farbe.data(), 4); }
    l.ebenen.push_back(Ebene{"Original", px, 1.0F, Mischart::Normal, true});
    l.grund = px;
    l.bild = px;
    l.maske.assign(uz(b) * uz(h), 0);
}

void dreieckeSetzen(Leinwand& l, const std::vector<Dreieck>& d, bool alsLadestand) {
    l.dreiecke = d;
    if (alsLadestand) { l.dreieckeBeimLaden = d; }
    l.bedeckt.assign(uz(l.b) * uz(l.h), 0);
    for (const Dreieck& x : d) {
        // bedeckt = die Texel, deren Mitte im Dreieck liegt (die der Pinsel bemalt); Randtexel ohne Mitte bekommen ihre
        // Farbe durch das Ausbluten
        texelImDreieck(x, l.b, l.h, [&](int tx, int ty, float, float, float) { l.bedeckt[l.texel(tx, ty)] = 1; });
    }
    l.blutQuelle = blutKarte(l.bedeckt, l.b, l.h, l.ausbluten);
}

void zusammensetzen(Leinwand& l, Rechteck r) {
    if (l.ebenen.empty()) { return; }
    if (r.leer()) { r = ganz(l); }
    r.x0 = std::max(r.x0, 0);
    r.y0 = std::max(r.y0, 0);
    r.x1 = std::min(r.x1, l.b - 1);
    r.y1 = std::min(r.y1, l.h - 1);
    l.bild.resize(uz(l.b) * uz(l.h) * 4U);
    for (int y = r.y0; y <= r.y1; ++y) {
        const std::size_t z = l.texel(r.x0, y) * 4U;
        const std::size_t n = uz(r.x1 - r.x0 + 1) * 4U;
        std::memcpy(&l.bild[z], &l.ebenen[0].rgba[z], n);   // die unterste Ebene ist der Grund (Deckkraft 1)
    }
    for (std::size_t e = 1; e < l.ebenen.size(); ++e) {
        const Ebene& eb = l.ebenen[e];
        if (!eb.sichtbar || eb.deckkraft <= 0.0F) { continue; }
        for (int y = r.y0; y <= r.y1; ++y) {
            for (int x = r.x0; x <= r.x1; ++x) {
                const std::size_t i = l.texel(x, y) * 4U;
                const float a = wertVon(eb.rgba[i + 3U]) * eb.deckkraft;
                if (a <= 0.0F) { continue; }
                float c[3];
                for (int k = 0; k < 3; ++k) { c[k] = mische(eb.misch, wertVon(l.bild[i + uz(k)]), wertVon(eb.rgba[i + uz(k)])); }
                ueber(&l.bild[i], c, a);
            }
        }
    }
}

// --- Rueckgaengig ---------------------------------------------------------------------------------------------------

void schrittBeginnen(Verlauf& v, const Leinwand& l, const std::string& name) {
    if (v.laeuft) { return; }
    v.offen = UndoSchritt{};
    v.offen.name = name;
    v.laeuft = true;
    v.gesichertEbenen = static_cast<int>(l.ebenen.size()) + 1;
    v.gesichert.assign(kachelZahl(l) * uz(v.gesichertEbenen), 0);
}

void sichern(Verlauf& v, const Leinwand& l, int ebene, const Rechteck& r) {
    if (!v.laeuft || r.leer()) { return; }
    const int kb = (l.b + kKachel - 1) / kKachel;
    const std::size_t proEbene = kachelZahl(l);
    const int e = ebene + 1;
    if (e < 0 || e >= v.gesichertEbenen) { return; }
    for (int ky = std::max(0, r.y0) / kKachel; ky <= std::min(l.h - 1, r.y1) / kKachel; ++ky) {
        for (int kx = std::max(0, r.x0) / kKachel; kx <= std::min(l.b - 1, r.x1) / kKachel; ++kx) {
            const std::size_t m = uz(e) * proEbene + uz(ky * kb + kx);
            if (v.gesichert[m] != 0) { continue; }
            v.gesichert[m] = 1;
            Kachel k;
            k.ebene = ebene;
            k.kx = kx;
            k.ky = ky;
            kachelKopieren(l, ebene < 0 ? l.maske : l.ebenen[uz(ebene)].rgba, ebene < 0 ? 1 : 4, kx, ky, k.vorher);
            v.offen.kacheln.push_back(std::move(k));
        }
    }
}

void schrittEnde(Verlauf& v, Leinwand& l) {
    if (!v.laeuft) { return; }
    v.laeuft = false;
    if (v.offen.kacheln.empty()) { return; }
    for (Kachel& k : v.offen.kacheln) {
        kachelKopieren(l, k.ebene < 0 ? l.maske : l.ebenen[uz(k.ebene)].rgba, k.ebene < 0 ? 1 : 4, k.kx, k.ky, k.nachher);
    }
    v.zurueck.push_back(std::move(v.offen));
    v.offen = UndoSchritt{};
    v.vor.clear();
    std::size_t summe = 0;
    for (const UndoSchritt& s : v.zurueck) { summe += schrittBytes(s); }
    while (v.zurueck.size() > 1U && summe > v.grenzeBytes) {
        summe -= schrittBytes(v.zurueck.front());
        v.zurueck.erase(v.zurueck.begin());
    }
}

void stapelAendern(Verlauf& v, Leinwand& l, const std::string& name, const std::vector<Ebene>& neu, int aktivNeu) {
    schrittEnde(v, l);
    UndoSchritt s;
    s.name = name;
    s.struktur = true;
    s.stapelVorher = l.ebenen;
    s.aktivVorher = l.aktiv;
    s.stapelNachher = neu;
    s.aktivNachher = aktivNeu;
    l.ebenen = neu;
    l.aktiv = std::clamp(aktivNeu, 0, static_cast<int>(l.ebenen.size()) - 1);
    v.zurueck.push_back(std::move(s));
    v.vor.clear();
    geaendert(l, ganz(l));
    zusammensetzen(l);
}

namespace {
void anwenden(Leinwand& l, const UndoSchritt& s, bool rueck) {
    if (s.struktur) {
        l.ebenen = rueck ? s.stapelVorher : s.stapelNachher;
        l.aktiv = std::clamp(rueck ? s.aktivVorher : s.aktivNachher, 0, static_cast<int>(l.ebenen.size()) - 1);
        const std::vector<std::uint8_t>& m = rueck ? s.maskeVorher : s.maskeNachher;
        if (!m.empty()) { l.maske = m; }
        geaendert(l, ganz(l));
        zusammensetzen(l);
        return;
    }
    Rechteck r;
    for (const Kachel& k : s.kacheln) {
        if (k.ebene >= static_cast<int>(l.ebenen.size())) { continue; }
        kachelSetzen(l, ebenenBytes(l, k.ebene), k.ebene < 0 ? 1 : 4, k.kx, k.ky, rueck ? k.vorher : k.nachher);
        r.dazu(k.kx * kKachel, k.ky * kKachel);
        r.dazu(std::min(l.b - 1, k.kx * kKachel + kKachel - 1), std::min(l.h - 1, k.ky * kKachel + kKachel - 1));
    }
    geaendert(l, r);
    zusammensetzen(l, r);
}
}  // namespace

bool rueckgaengig(Verlauf& v, Leinwand& l) {
    schrittEnde(v, l);
    if (v.zurueck.empty()) { return false; }
    UndoSchritt s = std::move(v.zurueck.back());
    v.zurueck.pop_back();
    anwenden(l, s, true);
    v.vor.push_back(std::move(s));
    return true;
}

bool wiederholen(Verlauf& v, Leinwand& l) {
    schrittEnde(v, l);
    if (v.vor.empty()) { return false; }
    UndoSchritt s = std::move(v.vor.back());
    v.vor.pop_back();
    anwenden(l, s, false);
    v.zurueck.push_back(std::move(s));
    return true;
}

// --- Pinsel ----------------------------------------------------------------------------------------------------------

void punktIm(const Dreieck& d, float l0, float l1, float l2, float uv[2], float ort[3]) noexcept {
    for (int k = 0; k < 2; ++k) { uv[k] = d.uv[0][k] * l0 + d.uv[1][k] * l1 + d.uv[2][k] * l2; }
    for (int k = 0; k < 3; ++k) { ort[k] = d.ort[0][k] * l0 + d.ort[1][k] * l1 + d.ort[2][k] * l2; }
}

void spiegelPunkt(const float p[3], int achse, float out[3]) noexcept {
    for (int k = 0; k < 3; ++k) { out[k] = p[k]; }
    const int a = std::clamp(achse, 0, 2);
    out[a] = -p[a];
}

int tupfer3d(Leinwand& l, Verlauf& v, const Pinsel& p, const float mitte[3], float druck, const std::vector<std::uint8_t>* zugewandt,
             const float richtung[2]) {
    if (l.ebenen.empty() || p.werkzeug == Werkzeug::Fuellen) { return 0; }
    const float radius = p.radius * (p.druckGroesse ? std::max(0.05F, druckFaktor(true, druck)) : 1.0F);
    const float staerke = p.staerke * druckFaktor(p.druckStaerke, druck);
    tupfAnfang(l);
    std::vector<Treffer> tr;
    for (std::size_t di = 0; di < l.dreiecke.size(); ++di) {
        if (!p.rueckseiten && zugewandt != nullptr && di < zugewandt->size() && (*zugewandt)[di] == 0) { continue; }
        const Dreieck& d = l.dreiecke[di];
        // schnell verwerfen: Kugel um das Dreieck
        float m[3];
        for (int k = 0; k < 3; ++k) { m[k] = (d.ort[0][k] + d.ort[1][k] + d.ort[2][k]) / 3.0F; }
        float rr = 0.0F;
        for (const auto& e : d.ort) { rr = std::max(rr, std::hypot(e[0] - m[0], e[1] - m[1], e[2] - m[2])); }
        if (std::hypot(m[0] - mitte[0], m[1] - mitte[1], m[2] - mitte[2]) > radius + rr) { continue; }
        texelImDreieck(d, l.b, l.h, [&](int x, int y, float l0, float l1, float l2) {
            float uv[2];
            float o[3];
            punktIm(d, l0, l1, l2, uv, o);
            const float f = abfall(std::hypot(o[0] - mitte[0], o[1] - mitte[1], o[2] - mitte[2]), radius, p.haerte) * staerke;
            if (f > 0.0F) { tupfMerken(l, tr, l.texel(x, y), f); }
        });
    }
    for (Treffer& t : tr) { t.f = l.tupfWert[uz(t.i)]; }
    return auftragen(l, v, p, tr, richtung);
}

int tupfer2d(Leinwand& l, Verlauf& v, const Pinsel& p, float u, float vv, float druck, const float richtung[2]) {
    if (l.ebenen.empty() || p.werkzeug == Werkzeug::Fuellen) { return 0; }
    const float radius = p.radius * (p.druckGroesse ? std::max(0.05F, druckFaktor(true, druck)) : 1.0F);
    const float staerke = p.staerke * druckFaktor(p.druckStaerke, druck);
    tupfAnfang(l);
    std::vector<Treffer> tr;
    const float cx = u * static_cast<float>(l.b);
    const float cy = vv * static_cast<float>(l.h);
    for (int y = static_cast<int>(std::floor(cy - radius)); y <= static_cast<int>(std::ceil(cy + radius)); ++y) {
        for (int x = static_cast<int>(std::floor(cx - radius)); x <= static_cast<int>(std::ceil(cx + radius)); ++x) {
            const float f = abfall(std::hypot(static_cast<float>(x) + 0.5F - cx, static_cast<float>(y) + 0.5F - cy), radius, p.haerte) * staerke;
            if (f > 0.0F) { tupfMerken(l, tr, l.texel(wickel(x, l.b), wickel(y, l.h)), f); }
        }
    }
    for (Treffer& t : tr) { t.f = l.tupfWert[uz(t.i)]; }
    return auftragen(l, v, p, tr, richtung);
}

int fuellen(Leinwand& l, Verlauf& v, const Pinsel& p, int x, int y) {
    if (l.ebenen.empty() || x < 0 || y < 0 || x >= l.b || y >= l.h) { return 0; }
    const std::size_t start = l.texel(x, y) * 4U;
    float ref[4];
    for (int k = 0; k < 4; ++k) { ref[k] = wertVon(l.bild[start + uz(k)]); }
    const float tol = std::clamp(p.fuellToleranz, 0.0F, 1.0F) * 2.0F;   // Abstand im RGBA-Wuerfel (Diagonale 2)
    std::vector<std::uint8_t> drin(uz(l.b) * uz(l.h), 0);
    std::vector<Treffer> tr;
    std::deque<std::int32_t> offen{static_cast<std::int32_t>(l.texel(x, y))};
    drin[l.texel(x, y)] = 1;
    while (!offen.empty()) {
        const std::int32_t i = offen.front();
        offen.pop_front();
        tr.push_back(Treffer{i, p.staerke});
        const int tx = i % l.b;
        const int ty = i / l.b;
        const int nb[4][2] = {{tx + 1, ty}, {tx - 1, ty}, {tx, ty + 1}, {tx, ty - 1}};
        for (const auto& n : nb) {
            if (n[0] < 0 || n[1] < 0 || n[0] >= l.b || n[1] >= l.h) { continue; }
            const std::size_t j = l.texel(n[0], n[1]);
            if (drin[j] != 0) { continue; }
            float d2 = 0.0F;
            for (int k = 0; k < 4; ++k) {
                const float d = wertVon(l.bild[j * 4U + uz(k)]) - ref[k];
                d2 += d * d;
            }
            if (std::sqrt(d2) > tol) { continue; }
            drin[j] = 1;
            offen.push_back(static_cast<std::int32_t>(j));
        }
    }
    Pinsel q = p;
    q.werkzeug = Werkzeug::Fuellen;
    tupfAnfang(l);
    return auftragen(l, v, q, tr, nullptr);
}

std::vector<std::array<float, 3>> Strich::weiter(const float p[3], float schritt) {
    std::vector<std::array<float, 3>> out;
    if (!laeuft) {
        laeuft = true;
        rest = 0.0F;
        for (int k = 0; k < 3; ++k) { letzter[k] = p[k]; }
        out.push_back({p[0], p[1], p[2]});
        return out;
    }
    const float d = std::hypot(p[0] - letzter[0], p[1] - letzter[1], p[2] - letzter[2]);
    if (schritt <= 0.0F || d <= 0.0F) { return out; }
    float noch = schritt - rest;
    while (noch <= d) {
        const float t = noch / d;
        out.push_back({letzter[0] + (p[0] - letzter[0]) * t, letzter[1] + (p[1] - letzter[1]) * t, letzter[2] + (p[2] - letzter[2]) * t});
        noch += schritt;
    }
    rest = d - (noch - schritt);
    for (int k = 0; k < 3; ++k) { letzter[k] = p[k]; }
    return out;
}

// --- Umbacken --------------------------------------------------------------------------------------------------------

std::vector<std::uint8_t> umbacken(const std::vector<std::uint8_t>& rgba, int b, int h, const std::vector<Dreieck>& alt,
                                   const std::vector<Dreieck>& neu, int ausbluten) {
    std::vector<std::uint8_t> out(uz(b) * uz(h) * 4U, 0);
    std::vector<std::uint8_t> bed(uz(b) * uz(h), 0);
    for (std::size_t d = 0; d < neu.size() && d < alt.size(); ++d) {
        texelImDreieck(neu[d], b, h, [&](int x, int y, float l0, float l1, float l2) {
            const float u = alt[d].uv[0][0] * l0 + alt[d].uv[1][0] * l1 + alt[d].uv[2][0] * l2;
            const float v = alt[d].uv[0][1] * l0 + alt[d].uv[1][1] * l1 + alt[d].uv[2][1] * l2;
            float c[4];
            bilinear(rgba, b, h, u, v, c);
            const std::size_t i = uz(y) * uz(b) + uz(x);
            for (int k = 0; k < 4; ++k) { out[i * 4U + uz(k)] = static_cast<std::uint8_t>(std::clamp(c[k] + 0.5F, 0.0F, 255.0F)); }
            bed[i] = 1;
        });
    }
    const std::vector<std::int32_t> q = blutKarte(bed, b, h, ausbluten);
    for (std::size_t i = 0; i < q.size(); ++i) {
        if (q[i] >= 0) { std::memcpy(&out[i * 4U], &out[uz(q[i]) * 4U], 4); }
    }
    return out;
}

bool leinwandUmbacken(Leinwand& l, Verlauf& v, const std::string& name) {
    if (l.dreieckeBeimLaden.size() != l.dreiecke.size() || l.dreiecke.empty()) { return false; }
    schrittEnde(v, l);
    std::vector<Ebene> neu = l.ebenen;
    for (Ebene& e : neu) { e.rgba = umbacken(e.rgba, l.b, l.h, l.dreieckeBeimLaden, l.dreiecke, l.ausbluten); }
    std::vector<std::uint8_t> m4(uz(l.b) * uz(l.h) * 4U, 0);
    for (std::size_t i = 0; i < l.maske.size(); ++i) { m4[i * 4U] = l.maske[i]; }
    m4 = umbacken(m4, l.b, l.h, l.dreieckeBeimLaden, l.dreiecke, l.ausbluten);
    std::vector<std::uint8_t> maskeNeu(l.maske.size());
    for (std::size_t i = 0; i < maskeNeu.size(); ++i) { maskeNeu[i] = m4[i * 4U]; }
    UndoSchritt s;
    s.name = name;
    s.struktur = true;
    s.stapelVorher = l.ebenen;
    s.aktivVorher = l.aktiv;
    s.stapelNachher = neu;
    s.aktivNachher = l.aktiv;
    s.maskeVorher = l.maske;
    s.maskeNachher = maskeNeu;
    l.ebenen = std::move(neu);
    l.maske = std::move(maskeNeu);
    v.zurueck.push_back(std::move(s));
    v.vor.clear();
    l.dreieckeBeimLaden = l.dreiecke;
    geaendert(l, ganz(l));
    zusammensetzen(l);
    return true;
}

}  // namespace bhed::modell::texmalen
