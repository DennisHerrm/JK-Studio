// Modell-Block 1c: Texture Painting - Leinwand, Ebenen, Pinsel, Rueckgaengig in Kacheln, Ausbluten, Umbacken.
// Siehe bhed/modell/malen.h. Neu geschrieben (WhiteoutFlakes hat kein Texture Painting).
#include "bhed/modell/malen.h"

#include "bhed/image.h"
#include "bhed/modell/malen_mess.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <limits>
#include <utility>

namespace bhed::modell::texmalen {

Messung& messung() noexcept {
    static Messung m;
    return m;
}

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

// Ein Rechteck in ungewickelten Texeln (eingeschlossen)
struct TexelBereich {
    int x0 = 0;
    int y0 = 0;
    int x1 = -1;
    int y1 = -1;
};

// Die Texel, deren Mitte im UV-Dreieck liegt (gewickelt), mit Schwerpunktkoordinaten. nur: allein die Texel in diesem
// Rechteck (MBMAL: die ein Tupfer erreichen kann) - jedes mit derselben Rechnung wie ohne.
template <class F>
void texelImDreieck(const Dreieck& d, int b, int h, F&& f, const TexelBereich* nur = nullptr) {
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
    int xa = static_cast<int>(std::floor(x0));
    int xe = static_cast<int>(std::ceil(x1));
    int ya = static_cast<int>(std::floor(y0));
    int ye = static_cast<int>(std::ceil(y1));
    if (nur != nullptr) {
        xa = std::max(xa, nur->x0);
        xe = std::min(xe, nur->x1);
        ya = std::max(ya, nur->y0);
        ye = std::min(ye, nur->y1);
    }
    for (int y = ya; y <= ye; ++y) {
        for (int x = xa; x <= xe; ++x) {
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

// MBMAL: die Texel eines Dreiecks, die ein Tupfer (Kugel um `mitte`) erreichen kann, als Rechteck. Der Ort eines Texels ist
// affin in seiner Lage im Bild (punktIm mit den Schwerpunktkoordinaten aus texelImDreieck), sein Abstand zur Mitte also eine
// quadratische Form: die Texel in Reichweite liegen in einer Ellipse. `out` ist ihr umschliessendes Rechteck mit Spielraum
// (der Radius plus die Rundung der float-Rechnung des Tupfers, dazu zwei Texel) - kein Texel mit Abfall > 0 fehlt, die
// ausgelassenen haetten Abfall 0. false: keines in Reichweite. Entartete und schlecht gestellte Dreiecke: alle Texel.
bool texelInReichweite(const Dreieck& d, int b, int h, const float mitte[3], float radius, TexelBereich& out) {
    out = TexelBereich{std::numeric_limits<int>::min(), std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
    double p[3][2];
    for (int e = 0; e < 3; ++e) {
        p[e][0] = static_cast<double>(d.uv[e][0] * static_cast<float>(b));   // die Ecken wie texelImDreieck (float)
        p[e][1] = static_cast<double>(d.uv[e][1] * static_cast<float>(h));
    }
    const double e1[2] = {p[1][0] - p[0][0], p[1][1] - p[0][1]};
    const double e2[2] = {p[2][0] - p[0][0], p[2][1] - p[0][1]};
    const double fl = e1[0] * e2[1] - e1[1] * e2[0];
    if (!(std::fabs(fl) > 1e-9)) { return true; }
    // Ort = ort[0] + (x - p0x) * a + (y - p0y) * bv; w = ort[0] - mitte
    double a[3];
    double bv[3];
    double w[3];
    double groesse = 0.0;   // groesster Betrag einer Koordinate (Rundung der Orte)
    double kante = 0.0;     // laengste Kante im Raum
    for (int k = 0; k < 3; ++k) {
        const double k1 = static_cast<double>(d.ort[1][k]) - static_cast<double>(d.ort[0][k]);
        const double k2 = static_cast<double>(d.ort[2][k]) - static_cast<double>(d.ort[0][k]);
        a[k] = (e2[1] * k1 - e1[1] * k2) / fl;
        bv[k] = (e1[0] * k2 - e2[0] * k1) / fl;
        w[k] = static_cast<double>(d.ort[0][k]) - static_cast<double>(mitte[k]);
        groesse = std::max({groesse, std::fabs(static_cast<double>(d.ort[0][k])), std::fabs(static_cast<double>(d.ort[1][k])),
                            std::fabs(static_cast<double>(d.ort[2][k])), std::fabs(static_cast<double>(mitte[k]))});
        kante = std::max({kante, std::fabs(k1), std::fabs(k2), std::fabs(k2 - k1)});
    }
    const double aa = a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
    const double ab = a[0] * bv[0] + a[1] * bv[1] + a[2] * bv[2];
    const double bb = bv[0] * bv[0] + bv[1] * bv[1] + bv[2] * bv[2];
    const double aw = a[0] * w[0] + a[1] * w[1] + a[2] * w[2];
    const double bw = bv[0] * w[0] + bv[1] * w[1] + bv[2] * w[2];
    const double ww = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
    const double det = aa * bb - ab * ab;
    if (!(aa > 0.0) || !(bb > 0.0) || !(det > 1e-9 * aa * bb)) { return true; }
    // Rundung der Schwerpunktkoordinaten in float: Produkte von Abstaenden in Texeln, geteilt durch die Flaeche. Ueber einem
    // Tausendstel gilt das Dreieck als schlecht gestellt (seine Texel liegen dann irgendwo im Dreieck: alle pruefen).
    const double bx = std::max({p[0][0], p[1][0], p[2][0]}) - std::min({p[0][0], p[1][0], p[2][0]}) + 1.0;
    const double by = std::max({p[0][1], p[1][1], p[2][1]}) - std::min({p[0][1], p[1][1], p[2][1]}) + 1.0;
    const double rundung = 4e-6 * (bx * bx + by * by) / std::fabs(fl);
    if (!(rundung <= 1e-3)) { return true; }
    const double reichweite = static_cast<double>(radius) * (1.0 + 1e-6) + rundung * 2.0 * 3.0 * kante + 1e-5 * groesse + 1e-6;
    const double xm = (ab * bw - bb * aw) / det;   // die Stelle im Bild, die der Mitte am naechsten liegt (ab p0)
    const double ym = (ab * aw - aa * bw) / det;
    const double rest = reichweite * reichweite - (ww + aw * xm + bw * ym);   // Radius der Ellipse im Quadrat (Ebene des Dreiecks)
    if (!(rest >= 0.0)) { return false; }
    const double hx = std::sqrt(rest * bb / det);
    const double hy = std::sqrt(rest * aa / det);
    const double grenze[4] = {p[0][0] + xm - hx - 2.5, p[0][1] + ym - hy - 2.5, p[0][0] + xm + hx + 1.5, p[0][1] + ym + hy + 1.5};
    for (const double g : grenze) {
        if (!(std::fabs(g) < 1e9)) { return true; }
    }
    out = TexelBereich{static_cast<int>(std::floor(grenze[0])), static_cast<int>(std::floor(grenze[1])), static_cast<int>(std::ceil(grenze[2])),
                       static_cast<int>(std::ceil(grenze[3]))};
    return true;
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

void geaendert(Leinwand& l, const Rechteck& r) {
    schmutzigDazu(l, r);
    ++l.stand;
}

Rechteck ganz(const Leinwand& l) { return Rechteck{0, 0, l.b - 1, l.h - 1}; }

// Ausbluten im Rechteck (der Aufrufer hat es um `ausbluten` erweitert): unbedeckte Texel nehmen den Wert ihres naechsten
// bedeckten - nur, wenn dieser im Tupfer bemalt wurde (tupfMarke), und so weit die Maske des Ziels es zulaesst (bemalen = 0:
// Maske selbst). Jedes Texel darf je Tupfer nur einmal an der Reihe sein (die Maske mischt Ziel und Quelle).
void blutenIn(Leinwand& l, std::vector<std::uint8_t>& px, int bpp, const Rechteck& r, bool maskeAchten) {
    if (l.blutQuelle.empty() || r.leer()) { return; }
    for (int y = r.y0; y <= r.y1; ++y) {
        for (int x = r.x0; x <= r.x1; ++x) {
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

// eine Kachel einer Ebene (-1 Maske) vor der ersten Aenderung im offenen Schritt kopieren
void kachelSichern(Verlauf& v, const Leinwand& l, int ebene, int kx, int ky) {
    const int e = ebene + 1;
    if (!v.laeuft || e < 0 || e >= v.gesichertEbenen) { return; }
    const int kb = (l.b + kKachel - 1) / kKachel;
    const std::size_t m = uz(e) * kachelZahl(l) + uz(ky * kb + kx);
    if (v.gesichert[m] != 0) { return; }
    v.gesichert[m] = 1;
    Kachel k;
    k.ebene = ebene;
    k.kx = kx;
    k.ky = ky;
    kachelKopieren(l, ebene < 0 ? l.maske : l.ebenen[uz(ebene)].rgba, ebene < 0 ? 1 : 4, kx, ky, k.vorher);
    v.offen.kacheln.push_back(std::move(k));
    if (messung().an) { ++messung().kachelnGesichert; }
}

// MBMAL: die Flaeche eines Tupfers in einer Kachel
struct KachelFlaeche {
    int kachel = 0;
    Rechteck r;
};

// der Platz der Kachel in der Liste des laufenden Tupfers (welche: 0 Treffer, 1 geschrieben), neu angelegt beim ersten Mal
std::size_t kachelPlatz(Leinwand& l, int welche, std::vector<KachelFlaeche>& liste, int kachel) {
    std::vector<std::int32_t>& marke = l.kachelMarke[welche];
    std::vector<std::int32_t>& platz = l.kachelPlatz[welche];
    if (marke[uz(kachel)] != l.tupfNr) {
        marke[uz(kachel)] = l.tupfNr;
        platz[uz(kachel)] = static_cast<std::int32_t>(liste.size());
        liste.push_back(KachelFlaeche{kachel, Rechteck{}});
    }
    return uz(platz[uz(kachel)]);
}

int auftragen(Leinwand& l, Verlauf& v, const Pinsel& p, const std::vector<Treffer>& tr, const float richtung[2]) {
    if (tr.empty()) { return 0; }
    // MBMAL: die Flaeche des Tupfers je Kachel - das Rechteck der Treffer in jeder Kachel, um das Ausbluten erweitert und auf
    // die Kacheln verteilt, in die es reicht. Bis hier galt ein Rechteck um alle Treffer: ein Tupfer in 3D ueber eine UV-Naht
    // trifft zwei Inseln, ihr gemeinsames Rechteck reichte bis zum ganzen Bild und wurde je Tupfer gesichert, nach
    // auszublutenden Texeln durchsucht, zusammengesetzt und hochgeladen. Geaendert werden dieselben Texel: Treffer liegen in
    // ihrer Kachel, ein ausgeblutetes Texel hoechstens `ausbluten` neben seiner Quelle; was im alten Rechteck sonst lag, blieb
    // gleich (Ausbluten nur von bemalten Quellen, Zusammensetzen ohne Aenderung ergibt dasselbe).
    const int kb = (l.b + kKachel - 1) / kKachel;
    for (int i = 0; i < 2; ++i) {
        if (l.kachelMarke[i].size() != kachelZahl(l)) {
            l.kachelMarke[i].assign(kachelZahl(l), 0);
            l.kachelPlatz[i].assign(kachelZahl(l), 0);
        }
    }
    std::vector<KachelFlaeche> getroffen;
    {
        int zuletzt = -1;
        std::size_t platz = 0;
        for (const Treffer& t : tr) {
            const int x = t.i % l.b;
            const int y = t.i / l.b;
            l.tupfMarke[uz(t.i)] = l.tupfNr;   // die Quellen des Ausblutens (beim Fuellen hier gesetzt)
            const int kachel = (y / kKachel) * kb + x / kKachel;
            if (kachel != zuletzt) {
                platz = kachelPlatz(l, 0, getroffen, kachel);
                zuletzt = kachel;
            }
            getroffen[platz].r.dazu(x, y);
        }
    }
    std::vector<KachelFlaeche> flaechen;   // je Kachel, in die der Tupfer schreibt (Treffer und Ausbluten), das Rechteck darin
    for (const KachelFlaeche& g : getroffen) {
        const Rechteck e = erweitert(l, g.r);
        for (int ky = e.y0 / kKachel; ky <= e.y1 / kKachel; ++ky) {
            for (int kx = e.x0 / kKachel; kx <= e.x1 / kKachel; ++kx) {
                const Rechteck teil{std::max(e.x0, kx * kKachel), std::max(e.y0, ky * kKachel), std::min(e.x1, kx * kKachel + kKachel - 1),
                                    std::min(e.y1, ky * kKachel + kKachel - 1)};
                flaechen[kachelPlatz(l, 1, flaechen, ky * kb + kx)].r.dazu(teil);
            }
        }
    }
    // Weichzeichnen/Verwischen/Klonen lesen ihre Nachbarn: das Rechteck um den Lesebereich
    const bool maske = p.werkzeug == Werkzeug::Maske;
    const int ebene = maske ? -1 : std::clamp(l.aktiv, 0, static_cast<int>(l.ebenen.size()) - 1);
    Messung& ms = messung();
    if (ms.an) {
        ms.treffer += static_cast<long long>(tr.size());
        for (const KachelFlaeche& f : flaechen) { ms.rechteckTexel += static_cast<long long>(f.r.x1 - f.r.x0 + 1) * static_cast<long long>(f.r.y1 - f.r.y0 + 1); }
    }
    {
        const Stoppuhr uhr(ms.sichernMs);
        for (const KachelFlaeche& f : flaechen) { kachelSichern(v, l, ebene, f.kachel % kb, f.kachel / kb); }
    }
    std::vector<std::uint8_t>& px = ebenenBytes(l, ebene);
    // P7 (Pruefung seit rc610): Weichzeichnen und Verwischen lesen ihre Nachbarn im Stand vor dem Tupfer. Bis hier kopierten
    // sie dafuer je Tupfer die ganze Ebene (2048er Bild: 16 MB, 7 ms je Tupfer); jetzt entstehen die neuen Werte zuerst in
    // `neu` und werden erst nach allen Treffern geschrieben - gelesen wird nur unveraendertes px, dieselben Bytes.
    const bool liestNachbarn = p.werkzeug == Werkzeug::Weichzeichnen || p.werkzeug == Werkzeug::Verwischen;
    const std::vector<std::uint8_t>& vorher = px;
    std::vector<std::uint8_t> neu(liestNachbarn ? tr.size() * 4U : 0U);
    std::vector<std::size_t> neuZiel;   // je Eintrag in `neu` der Ort in px
    const float farbe[3] = {p.farbe[0], p.farbe[1], p.farbe[2]};
    int n = 0;
    Stoppuhr werkzeugUhr(ms.werkzeugMs);
    for (const Treffer& t : tr) {
        const std::size_t i = uz(t.i);
        const float frei = maske ? 1.0F : 1.0F - wertVon(l.maske[i]);
        const float f = std::clamp(t.f * frei, 0.0F, 1.0F);
        if (f <= 0.0F) { continue; }
        const int x = t.i % l.b;
        const int y = t.i / l.b;
        std::uint8_t* q = maske ? &px[i] : &px[i * 4U];
        if (liestNachbarn) {
            q = &neu[neuZiel.size() * 4U];
            neuZiel.push_back(i * 4U);
        }
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
    for (std::size_t k = 0; k < neuZiel.size(); ++k) { std::memcpy(&px[neuZiel[k]], &neu[k * 4U], 4U); }
    werkzeugUhr.halt();
    {
        const Stoppuhr uhr(ms.blutenMs);
        for (const KachelFlaeche& f : flaechen) { blutenIn(l, px, maske ? 1 : 4, f.r, !maske); }
    }
    for (const KachelFlaeche& f : flaechen) { schmutzigDazu(l, f.r); }
    ++l.stand;
    if (maske) { ++l.maskeStand; }
    const Stoppuhr uhr(ms.zusammenMs);
    for (const KachelFlaeche& f : flaechen) { zusammensetzen(l, f.r); }
    return n;
}

float druckFaktor(bool an, float druck) { return an ? std::clamp(druck, 0.0F, 1.0F) : 1.0F; }

void tupfAnfang(Leinwand& l) {
    if (l.tupfMarke.size() != uz(l.b) * uz(l.h)) {
        l.tupfMarke.assign(uz(l.b) * uz(l.h), 0);
        l.tupfWert.assign(uz(l.b) * uz(l.h), 0.0F);
        l.tupfNr = 0;
        l.gitterMarke.clear();   // die Marken zaehlen mit tupfNr
        for (std::vector<std::int32_t>& k : l.kachelMarke) { k.clear(); }
    }
    ++l.tupfNr;
}

// --- MBMAL: das Suchgitter der Dreiecke (Leinwand::Gitter) -------------------------------------------------------------

// der Kasten um eine Kugel bzw. einen Tupfer im Gitter: etwas groesser, damit die float-Rechnung des Tupfers (Abstand der
// Mitten gegen Radius + Umkreis) am Rand kein Dreieck verliert, das sie annimmt
double gitterRand(double radius) { return radius * 1.0001 + 1e-4; }

// die Zelle einer Koordinate (nicht begrenzt; monoton, damit sich ueberlappende Kaesten gemeinsame Zellen haben)
double gitterZelle(const Leinwand::Gitter& g, int achse, double x) { return std::floor((x - static_cast<double>(g.ursprung[achse])) / static_cast<double>(g.zelle)); }

// die Zellen eines Kastens je Achse, auf das Gitter begrenzt (was daneben liegt, zaehlt zur Randzelle - beim Eintragen wie
// beim Suchen, so bleibt die Ueberlappung erhalten)
void gitterBereich(const Leinwand::Gitter& g, const double mitte[3], double rand, int von[3], int bis[3]) {
    for (int k = 0; k < 3; ++k) {
        const double oben = static_cast<double>(g.n[k] - 1);
        von[k] = static_cast<int>(std::clamp(gitterZelle(g, k, mitte[k] - rand), 0.0, oben));
        bis[k] = static_cast<int>(std::clamp(gitterZelle(g, k, mitte[k] + rand), 0.0, oben));
    }
}

void gitterBauen(Leinwand& l) {
    Leinwand::Gitter& g = l.gitter;
    g = Leinwand::Gitter{};
    const std::size_t zahl = l.kugeln.size();
    if (zahl == 0U || zahl != l.dreiecke.size()) { return; }
    double lo[3] = {0.0, 0.0, 0.0};
    double hi[3] = {0.0, 0.0, 0.0};
    double radien = 0.0;
    for (std::size_t i = 0; i < zahl; ++i) {
        const std::array<float, 4>& kg = l.kugeln[i];
        if (!std::isfinite(kg[0]) || !std::isfinite(kg[1]) || !std::isfinite(kg[2]) || !std::isfinite(kg[3])) { return; }   // ohne Gitter: alle pruefen
        const double rand = gitterRand(static_cast<double>(kg[3]));
        for (int k = 0; k < 3; ++k) {
            const double c = static_cast<double>(kg[uz(k)]);
            lo[k] = i == 0U ? c - rand : std::min(lo[k], c - rand);
            hi[k] = i == 0U ? c + rand : std::max(hi[k], c + rand);
        }
        radien += static_cast<double>(kg[3]);
    }
    // Zellen so gross wie ein mittleres Dreieck (sein Umkreis-Durchmesser), hoechstens 64 je Achse
    constexpr int kZellenJeAchse = 64;
    constexpr int kGrossAb = 512;   // ein Dreieck ueber mehr Zellen steht in der Liste `gross` statt in jeder Zelle
    const double zelle = std::max({(hi[0] - lo[0]) / kZellenJeAchse, (hi[1] - lo[1]) / kZellenJeAchse, (hi[2] - lo[2]) / kZellenJeAchse,
                                   2.0 * radien / static_cast<double>(zahl), 1e-3});
    if (!std::isfinite(zelle)) { return; }
    g.zelle = static_cast<float>(zelle);
    for (int k = 0; k < 3; ++k) {
        g.ursprung[k] = static_cast<float>(lo[k]);
        g.n[k] = std::clamp(static_cast<int>(std::floor((hi[k] - lo[k]) / zelle)) + 1, 1, kZellenJeAchse);
    }
    const std::size_t zellen = uz(g.n[0]) * uz(g.n[1]) * uz(g.n[2]);
    // zwei Durchgaenge: zaehlen, dann eintragen (je Zelle aufsteigend nach Dreieck)
    g.anfang.assign(zellen + 1U, 0U);
    for (int durchgang = 0; durchgang < 2; ++durchgang) {
        for (std::size_t i = 0; i < zahl; ++i) {
            const std::array<float, 4>& kg = l.kugeln[i];
            const double mitte[3] = {static_cast<double>(kg[0]), static_cast<double>(kg[1]), static_cast<double>(kg[2])};
            int von[3];
            int bis[3];
            gitterBereich(g, mitte, gitterRand(static_cast<double>(kg[3])), von, bis);
            const bool gross = (bis[0] - von[0] + 1) * (bis[1] - von[1] + 1) * (bis[2] - von[2] + 1) > kGrossAb;
            if (gross) {
                if (durchgang == 0) { g.gross.push_back(static_cast<std::uint32_t>(i)); }
                continue;
            }
            for (int z = von[2]; z <= bis[2]; ++z) {
                for (int y = von[1]; y <= bis[1]; ++y) {
                    for (int x = von[0]; x <= bis[0]; ++x) {
                        const std::size_t c = (uz(z) * uz(g.n[1]) + uz(y)) * uz(g.n[0]) + uz(x);
                        if (durchgang == 0) {
                            ++g.anfang[c + 1U];
                        } else {
                            g.eintraege[g.anfang[c]++] = static_cast<std::uint32_t>(i);
                        }
                    }
                }
            }
        }
        if (durchgang == 0) {
            for (std::size_t c = 0; c < zellen; ++c) { g.anfang[c + 1U] += g.anfang[c]; }
            g.eintraege.assign(g.anfang[zellen], 0U);
        } else {
            // das Eintragen hat jeden Anfang auf sein Ende geschoben: eine Zelle zurueck
            for (std::size_t c = zellen; c > 0U; --c) { g.anfang[c] = g.anfang[c - 1U]; }
            g.anfang[0] = 0U;
        }
    }
    g.dreieckZahl = zahl;
}

// Die Dreiecke, die ein Tupfer um `mitte` erreichen kann, in l.gitterFund - aufsteigend, in der Folge des Durchlaufs ueber
// alle Dreiecke. Eine Obermenge: der Tupfer prueft jedes mit derselben Rechnung wie ohne Gitter.
void gitterSuchen(Leinwand& l, const float mitte[3], float radius) {
    const Leinwand::Gitter& g = l.gitter;
    l.gitterFund.clear();
    if (l.gitterMarke.size() != g.dreieckZahl) { l.gitterMarke.assign(g.dreieckZahl, 0); }
    const auto nimm = [&l](std::uint32_t d) {
        if (l.gitterMarke[d] != l.tupfNr) {
            l.gitterMarke[d] = l.tupfNr;
            l.gitterFund.push_back(d);
        }
    };
    const double m[3] = {static_cast<double>(mitte[0]), static_cast<double>(mitte[1]), static_cast<double>(mitte[2])};
    int von[3];
    int bis[3];
    gitterBereich(g, m, gitterRand(static_cast<double>(radius)), von, bis);
    for (int z = von[2]; z <= bis[2]; ++z) {
        for (int y = von[1]; y <= bis[1]; ++y) {
            const std::size_t zeile = (uz(z) * uz(g.n[1]) + uz(y)) * uz(g.n[0]);
            for (std::uint32_t e = g.anfang[zeile + uz(von[0])]; e < g.anfang[zeile + uz(bis[0]) + 1U]; ++e) { nimm(g.eintraege[e]); }
        }
    }
    for (const std::uint32_t d : g.gross) { nimm(d); }
    std::sort(l.gitterFund.begin(), l.gitterFund.end());
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

std::size_t schrittBytes(const UndoSchritt& s) {
    std::size_t n = s.maskeVorher.size() + s.maskeNachher.size();
    for (const Kachel& k : s.kacheln) { n += k.vorher.size() + k.nachher.size(); }
    for (const Ebene& e : s.stapelVorher) { n += e.rgba.size(); }
    for (const Ebene& e : s.stapelNachher) { n += e.rgba.size(); }
    return n;
}

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
        if (fehler != nullptr) { *fehler = im.error; }   // P13: leer - den Satz in der Sprache der Oberflaeche setzt die GUI
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
    l.ebenen.push_back(Ebene{std::string(), im.rgba, 1.0F, Mischart::Normal, true});   // P13: Name leer = die Grundebene
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
    l.ebenen.push_back(Ebene{std::string(), px, 1.0F, Mischart::Normal, true});
    l.grund = px;
    l.bild = px;
    l.maske.assign(uz(b) * uz(h), 0);
}

namespace {
std::array<float, 4> kugelVon(const Dreieck& d) {
    std::array<float, 4> k{};
    for (int i = 0; i < 3; ++i) { k[uz(i)] = (d.ort[0][i] + d.ort[1][i] + d.ort[2][i]) / 3.0F; }
    for (const auto& e : d.ort) { k[3] = std::max(k[3], std::hypot(e[0] - k[0], e[1] - k[1], e[2] - k[2])); }
    return k;
}
}  // namespace

void dreieckeSetzen(Leinwand& l, const std::vector<Dreieck>& d, bool alsLadestand) {
    l.dreiecke = d;
    l.kugeln.clear();   // P15: einmal je Satz Dreiecke statt je Tupfer
    for (const Dreieck& x : d) { l.kugeln.push_back(kugelVon(x)); }
    gitterBauen(l);
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

void schmutzigDazu(Leinwand& l, const Rechteck& rr) {
    const Rechteck r{std::max(rr.x0, 0), std::max(rr.y0, 0), std::min(rr.x1, l.b - 1), std::min(rr.y1, l.h - 1)};
    if (r.leer()) { return; }
    l.schmutzig.dazu(r);
    const int kb = (l.b + kKachel - 1) / kKachel;
    if (l.schmutzKachel.size() != kachelZahl(l)) {
        l.schmutzKachel.assign(kachelZahl(l), Rechteck{});
        l.schmutzListe.clear();
    }
    for (int ky = r.y0 / kKachel; ky <= r.y1 / kKachel; ++ky) {
        for (int kx = r.x0 / kKachel; kx <= r.x1 / kKachel; ++kx) {
            Rechteck& k = l.schmutzKachel[uz(ky * kb + kx)];
            if (k.leer()) { l.schmutzListe.push_back(ky * kb + kx); }
            k.dazu(Rechteck{std::max(r.x0, kx * kKachel), std::max(r.y0, ky * kKachel), std::min(r.x1, kx * kKachel + kKachel - 1),
                            std::min(r.y1, ky * kKachel + kKachel - 1)});
        }
    }
}

std::vector<Rechteck> schmutzRechtecke(const Leinwand& l) {
    std::vector<Rechteck> out;
    if (l.schmutzListe.empty()) { return out; }
    std::vector<std::int32_t> kacheln = l.schmutzListe;
    std::sort(kacheln.begin(), kacheln.end());
    const int kb = (l.b + kKachel - 1) / kKachel;
    std::vector<std::size_t> oben;    // die Rechtecke der Kachelzeile darueber (Plaetze in out)
    std::vector<std::size_t> zeile;   // die der laufenden Kachelzeile
    int zeileNr = -1;
    for (std::size_t i = 0; i < kacheln.size();) {
        const int ky = kacheln[i] / kb;
        if (ky != zeileNr) {
            oben = ky == zeileNr + 1 ? zeile : std::vector<std::size_t>{};
            zeile.clear();
            zeileNr = ky;
        }
        // ein Lauf nebeneinander liegender Kacheln, deren Rechtecke aneinanderstossen und gleich hoch sind
        Rechteck lauf = l.schmutzKachel[uz(kacheln[i])];
        std::size_t j = i + 1U;
        while (j < kacheln.size() && kacheln[j] == kacheln[j - 1U] + 1 && kacheln[j] / kb == ky) {
            const Rechteck& n = l.schmutzKachel[uz(kacheln[j])];
            if (n.x0 != lauf.x1 + 1 || n.y0 != lauf.y0 || n.y1 != lauf.y1) { break; }
            lauf.x1 = n.x1;
            ++j;
        }
        i = j;
        // an ein Rechteck der Zeile darueber mit denselben Seiten anhaengen, sonst ein neues
        bool angehaengt = false;
        for (const std::size_t o : oben) {
            if (out[o].x0 == lauf.x0 && out[o].x1 == lauf.x1 && out[o].y1 + 1 == lauf.y0) {
                out[o].y1 = lauf.y1;
                zeile.push_back(o);
                angehaengt = true;
                break;
            }
        }
        if (!angehaengt) {
            zeile.push_back(out.size());
            out.push_back(lauf);
        }
    }
    return out;
}

void schmutzigLeeren(Leinwand& l) {
    for (const std::int32_t k : l.schmutzListe) { l.schmutzKachel[uz(k)] = Rechteck{}; }
    l.schmutzListe.clear();
    l.schmutzig = Rechteck{};
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
    for (int ky = std::max(0, r.y0) / kKachel; ky <= std::min(l.h - 1, r.y1) / kKachel; ++ky) {
        for (int kx = std::max(0, r.x0) / kKachel; kx <= std::min(l.b - 1, r.x1) / kKachel; ++kx) { kachelSichern(v, l, ebene, kx, ky); }
    }
}

namespace {
long long g_schrittNummer = 0;   // P4: ueber alle Leinwaende fortlaufend

// ein fertiger Schritt in den Verlauf; danach auf grenzeBytes gekuerzt (P2: fuer jeden Schritt, nicht nur Pinselstriche)
void schrittAblegen(Verlauf& v, UndoSchritt&& s) {
    s.nummer = ++g_schrittNummer;
    v.zurueck.push_back(std::move(s));
    v.vor.clear();
    std::size_t summe = 0;
    for (const UndoSchritt& x : v.zurueck) { summe += schrittBytes(x); }
    while (v.zurueck.size() > 1U && summe > v.grenzeBytes) {
        summe -= schrittBytes(v.zurueck.front());
        v.zurueck.erase(v.zurueck.begin());
    }
}
}  // namespace

void schrittEnde(Verlauf& v, Leinwand& l) {
    if (!v.laeuft) { return; }
    v.laeuft = false;
    if (v.offen.kacheln.empty()) { return; }
    const Stoppuhr uhr(messung().strichEndeMs);
    for (Kachel& k : v.offen.kacheln) {
        kachelKopieren(l, k.ebene < 0 ? l.maske : l.ebenen[uz(k.ebene)].rgba, k.ebene < 0 ? 1 : 4, k.kx, k.ky, k.nachher);
    }
    UndoSchritt s = std::move(v.offen);
    v.offen = UndoSchritt{};
    schrittAblegen(v, std::move(s));
}

void stapelAendern(Verlauf& v, Leinwand& l, const std::string& name, const std::vector<Ebene>& neu, int aktivNeu) {
    schrittEnde(v, l);
    UndoSchritt s;
    s.name = name;
    s.struktur = true;
    s.aktivVorher = l.aktiv;
    s.aktivNachher = aktivNeu;
    // P2: aendert der Schritt keine Bildpunkte (gleiche Ebenen, gleiche rgba), merkt er nur die Eigenschaften. PTFIX3 P26: erst
    // vergleichen, dann kopieren - vorher entstanden beide Stapel samt Bildpunkten und wurden danach geleert (4096er Bild mit
    // vier Ebenen: 512 MB Spitze fuer einen Klick auf "Sichtbar").
    bool gleicheBilder = neu.size() == l.ebenen.size();
    for (std::size_t i = 0; gleicheBilder && i < neu.size(); ++i) { gleicheBilder = neu[i].rgba == l.ebenen[i].rgba; }
    if (gleicheBilder) {
        s.nurEigenschaften = true;
        const auto ohneBild = [](const std::vector<Ebene>& st) {
            std::vector<Ebene> out(st.size());
            for (std::size_t i = 0; i < st.size(); ++i) {
                out[i].name = st[i].name;
                out[i].deckkraft = st[i].deckkraft;
                out[i].misch = st[i].misch;
                out[i].sichtbar = st[i].sichtbar;
            }
            return out;
        };
        s.stapelVorher = ohneBild(l.ebenen);
        s.stapelNachher = ohneBild(neu);
    } else {
        s.stapelVorher = l.ebenen;
        s.stapelNachher = neu;
    }
    l.ebenen = neu;
    l.aktiv = std::clamp(aktivNeu, 0, static_cast<int>(l.ebenen.size()) - 1);
    schrittAblegen(v, std::move(s));
    geaendert(l, ganz(l));
    zusammensetzen(l);
}

namespace {
void anwenden(Leinwand& l, const UndoSchritt& s, bool rueck) {
    if (s.struktur && s.nurEigenschaften) {   // P2: die Bildpunkte bleiben
        const std::vector<Ebene>& st = rueck ? s.stapelVorher : s.stapelNachher;
        for (std::size_t i = 0; i < st.size() && i < l.ebenen.size(); ++i) {
            l.ebenen[i].name = st[i].name;
            l.ebenen[i].deckkraft = st[i].deckkraft;
            l.ebenen[i].misch = st[i].misch;
            l.ebenen[i].sichtbar = st[i].sichtbar;
        }
        l.aktiv = std::clamp(rueck ? s.aktivVorher : s.aktivNachher, 0, static_cast<int>(l.ebenen.size()) - 1);
        geaendert(l, ganz(l));
        zusammensetzen(l);
        return;
    }
    if (s.struktur) {
        l.ebenen = rueck ? s.stapelVorher : s.stapelNachher;
        l.aktiv = std::clamp(rueck ? s.aktivVorher : s.aktivNachher, 0, static_cast<int>(l.ebenen.size()) - 1);
        const std::vector<std::uint8_t>& m = rueck ? s.maskeVorher : s.maskeNachher;
        if (!m.empty()) {
            l.maske = m;
            ++l.maskeStand;
        }
        geaendert(l, ganz(l));
        zusammensetzen(l);
        return;
    }
    // MBMAL: je Kachel statt im Rechteck um alle Kacheln des Schritts (dieselben Bildpunkte: nur sie haben sich geaendert)
    for (const Kachel& k : s.kacheln) {
        if (k.ebene >= static_cast<int>(l.ebenen.size())) { continue; }
        kachelSetzen(l, ebenenBytes(l, k.ebene), k.ebene < 0 ? 1 : 4, k.kx, k.ky, rueck ? k.vorher : k.nachher);
        const Rechteck r{k.kx * kKachel, k.ky * kKachel, std::min(l.b - 1, k.kx * kKachel + kKachel - 1), std::min(l.h - 1, k.ky * kKachel + kKachel - 1)};
        schmutzigDazu(l, r);
        zusammensetzen(l, r);
        if (k.ebene < 0) { ++l.maskeStand; }
    }
    ++l.stand;
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
    Messung& ms = messung();
    Stoppuhr suchUhr(ms.suchenMs);
    long long geprueft = 0;
    long long nah = 0;
    long long texel = 0;
    const auto pruefen = [&](std::size_t di) {
        if (!p.rueckseiten && zugewandt != nullptr && di < zugewandt->size() && (*zugewandt)[di] == 0) { return; }
        const Dreieck& d = l.dreiecke[di];
        // schnell verwerfen: Kugel um das Dreieck (P15: aus dreieckeSetzen; ohne sie wie vorher je Tupfer gerechnet)
        const std::array<float, 4> kg = di < l.kugeln.size() && l.kugeln.size() == l.dreiecke.size() ? l.kugeln[di] : kugelVon(d);
        ++geprueft;
        if (std::hypot(kg[0] - mitte[0], kg[1] - mitte[1], kg[2] - mitte[2]) > radius + kg[3]) { return; }
        ++nah;
        // MBMAL: nur die Texel des Dreiecks, die der Tupfer erreichen kann (vorher alle Texel jedes Dreiecks in Reichweite)
        TexelBereich nur;
        if (!texelInReichweite(d, l.b, l.h, mitte, radius, nur)) { return; }
        texelImDreieck(
            d, l.b, l.h,
            [&](int x, int y, float l0, float l1, float l2) {
                float uv[2];
                float o[3];
                punktIm(d, l0, l1, l2, uv, o);
                ++texel;
                const float f = abfall(std::hypot(o[0] - mitte[0], o[1] - mitte[1], o[2] - mitte[2]), radius, p.haerte) * staerke;
                if (f > 0.0F) { tupfMerken(l, tr, l.texel(x, y), f); }
            },
            &nur);
    };
    // MBMAL: die Dreiecke aus dem Suchgitter statt alle (dieselben in derselben Folge kommen durch die Pruefung der Kugel)
    const bool endlich = std::isfinite(mitte[0]) && std::isfinite(mitte[1]) && std::isfinite(mitte[2]) && std::isfinite(radius);
    if (endlich && !l.dreiecke.empty() && l.gitter.dreieckZahl == l.dreiecke.size() && l.kugeln.size() == l.dreiecke.size()) {
        gitterSuchen(l, mitte, radius);
        for (const std::uint32_t di : l.gitterFund) { pruefen(di); }
    } else {
        for (std::size_t di = 0; di < l.dreiecke.size(); ++di) { pruefen(di); }
    }
    for (Treffer& t : tr) { t.f = l.tupfWert[uz(t.i)]; }
    suchUhr.halt();
    if (ms.an) {
        ++ms.tupfer;
        ms.tupferLeer += tr.empty() ? 1 : 0;
        ms.dreieckeGeprueft += geprueft;
        ms.dreieckeLeinwand += static_cast<long long>(l.dreiecke.size());
        ms.dreieckeNah += nah;
        ms.texelGeprueft += texel;
    }
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
    if (messung().an) { ++messung().tupfer; }
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

void texelJeDreieck(const Dreieck& d, int b, int h, const std::function<void(int x, int y, float l0, float l1, float l2)>& f) {
    texelImDreieck(d, b, h, f);
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
    ++l.maskeStand;
    schrittAblegen(v, std::move(s));
    l.dreieckeBeimLaden = l.dreiecke;
    geaendert(l, ganz(l));
    zusammensetzen(l);
    return true;
}

}  // namespace bhed::modell::texmalen
