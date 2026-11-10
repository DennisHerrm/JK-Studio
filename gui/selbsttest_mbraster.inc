// selbsttest_mbraster.inc - MBRASTER (10.11.): das Boden-Raster des Model-Tabs wie 3ds Max (Home Grid) bzw. Blender
// (Overlays: Floor, Grid, Axes X Y Z, Scale, Subdivisions), ab Werk an (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=raster).
// shank 10.11.: "ich habe gesehen es gibt im model Tab keinen boden wie in 3ds max oder blender. Kann man den einfuegen
// einen grid". Geprueft wird am Bild der Ansicht (Foto mit Ueberlagerung) gegen die Lage, die die Vorgaben verlangen:
// Linien alle 8 Einheiten (Rasterabstand wie das Raster des Karten-Editors) in Bodenhoehe (an.bodenZ, ab Werk -24 wie ModView
// und DEFAULT_MINS_2), duenn (hoechstens zwei Bildpunkte neben der gerechneten Linie, auch bei schraegem Blick), Achsen in
// ihren Farben, die Orthoansicht bildfuellend, der Abstand passt sich dem Zoom an, die Figur verdeckt das Raster, der Schalter
// wirkt je Ansicht, beide Bedienmodi, beide Farbschemen, heller Hintergrund.
// Dazu (BHED_MODELL_NUR=kopf, am Ende der Gruppe): zwei Bildfehler aus der MBKOPF-Abnahme am Fensterbild - die vordere
// Gegenkugel -Y des Gizmos deckt die Kugel Y dahinter (Blender: von hinten nach vorn, die vordere deckend), und jeder
// Buchstabe des Kompasses ist zu lesen, auch wenn der ViewCube davor steht.
// Eingebunden in selbsttest.cpp nach selbsttest_dock4.inc.
namespace mbraster {

ModellTab& t() { return g_app->mtab; }
ModellAnsicht& ansicht(int n = -1) { return t().ansichten[n < 0 ? t().aktiv : n]; }

constexpr float kAbstand = 8.0F;   // die Vorgabe des Auftrags (Raster des Karten-Editors), im Test fest

// Das Raster der aktiven Ansicht ganz an oder aus - im Blender-Modus auch der Boden (Floor) und die Achsen X/Y, die
// es dort neben dem Raster gibt (vor MBRASTER nur der Schalter G).
template <typename S>
void rasterSchalten(S& sch, bool an) {
    sch.raster = an;
    if constexpr (requires { sch.rasterBoden; }) { sch.rasterBoden = an; }
    if constexpr (requires { sch.achsen[0]; }) {
        sch.achsen[0] = an;
        sch.achsen[1] = an;
    }
}

template <typename S>
bool rasterAbWerk(const S& sch) {
    bool an = sch.raster;
    if constexpr (requires { sch.rasterBoden; }) { an = an && sch.rasterBoden; }
    return an;
}

template <typename S>
void zAchseSetzen(S& sch, bool an) {
    if constexpr (requires { sch.achsen[2]; }) { sch.achsen[2] = an; }
    (void)sch;
    (void)an;
}

// Ein Foto der aktiven Ansicht (3D und Ueberlagerung, so gross wie die Ansicht) und die Matrix, mit der es entstand.
struct Foto {
    std::vector<unsigned char> px;
    int w = 0;
    int h = 0;
    float vp[16]{};
    [[nodiscard]] bool da() const { return w > 0 && h > 0 && px.size() == static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U; }
    [[nodiscard]] const unsigned char* p(int x, int y) const {
        x = std::clamp(x, 0, w - 1);
        y = std::clamp(y, 0, h - 1);
        return &px[(static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4U];
    }
};

Foto foto() {
    Foto f;
    (void)modellFotoMitUeberlagerung(0, f.px, f.w, f.h);
    // modellFotoMitUeberlagerung rechnet die Matrix fuer Breite und Hoehe des Fotos neu (baueSicht) - so gross wie die Ansicht
    float* m = f.vp;
    const ModellAnsicht& a = ansicht();
    std::memcpy(m, a.viewProj, sizeof f.vp);
    return f;
}

int hell(const unsigned char* c) { return (c[0] * 30 + c[1] * 59 + c[2] * 11) / 100; }
int abst(const unsigned char* a, const unsigned char* b) { return std::max({std::abs(a[0] - b[0]), std::abs(a[1] - b[1]), std::abs(a[2] - b[2])}); }

// Groesster Unterschied zweier Fotos im 3x3-Feld um (x, y) - eine Linie mit Kantenglaettung trifft den Punkt nicht immer mittig.
int unterschiedUm(const Foto& a, const Foto& b, float x, float y) {
    int m = 0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            m = std::max(m, abst(a.p(static_cast<int>(x) + dx, static_cast<int>(y) + dy), b.p(static_cast<int>(x) + dx, static_cast<int>(y) + dy)));
        }
    }
    return m;
}

bool aufFoto(const Foto& f, float wx, float wy, float wz, float& x, float& y) {
    const float* m = f.vp;
    const float cx = m[0] * wx + m[1] * wy + m[2] * wz + m[3];
    const float cy = m[4] * wx + m[5] * wy + m[6] * wz + m[7];
    const float cw = m[12] * wx + m[13] * wy + m[14] * wz + m[15];
    if (cw <= 1e-3F) { return false; }
    x = (cx / cw * 0.5F + 0.5F) * static_cast<float>(f.w);
    y = (0.5F - cy / cw * 0.5F) * static_cast<float>(f.h);
    return x >= 1.0F && y >= 1.0F && x < static_cast<float>(f.w) - 1.0F && y < static_cast<float>(f.h) - 1.0F;
}

// 4x4 umkehren (zeilenweise), fuer den Strahl durch einen Bildpunkt.
bool umkehren(const float m[16], double inv[16]) {
    double a[4][8];
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            a[r][c] = m[r * 4 + c];
            a[r][c + 4] = r == c ? 1.0 : 0.0;
        }
    }
    for (int c = 0; c < 4; ++c) {
        int best = c;
        for (int r = c + 1; r < 4; ++r) {
            if (std::fabs(a[r][c]) > std::fabs(a[best][c])) { best = r; }
        }
        if (std::fabs(a[best][c]) < 1e-12) { return false; }
        for (int k = 0; k < 8; ++k) { std::swap(a[c][k], a[best][k]); }
        const double d = a[c][c];
        for (int k = 0; k < 8; ++k) { a[c][k] /= d; }
        for (int r = 0; r < 4; ++r) {
            if (r == c) { continue; }
            const double f = a[r][c];
            for (int k = 0; k < 8; ++k) { a[r][k] -= f * a[c][k]; }
        }
    }
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) { inv[r * 4 + c] = a[r][c + 4]; }
    }
    return true;
}

// Der Punkt der Ebene z = zEbene unter dem Bildpunkt (Mitte des Bildpunkts x + 0.5).
bool aufBoden(const double inv[16], int w, int h, double x, double y, double zEbene, double& wx, double& wy) {
    const double nx = (x + 0.5) / w * 2.0 - 1.0;
    const double ny = 1.0 - (y + 0.5) / h * 2.0;
    double p[2][3];
    for (int k = 0; k < 2; ++k) {
        const double nz = k == 0 ? 0.0 : 1.0;
        double q[4];
        for (int r = 0; r < 4; ++r) { q[r] = inv[r * 4 + 0] * nx + inv[r * 4 + 1] * ny + inv[r * 4 + 2] * nz + inv[r * 4 + 3]; }
        if (std::fabs(q[3]) < 1e-12) { return false; }
        for (int r = 0; r < 3; ++r) { p[k][r] = q[r] / q[3]; }
    }
    const double dz = p[1][2] - p[0][2];
    if (std::fabs(dz) < 1e-9) { return false; }
    const double s = (zEbene - p[0][2]) / dz;
    if (s < 0.0) { return false; }
    wx = p[0][0] + (p[1][0] - p[0][0]) * s;
    wy = p[0][1] + (p[1][1] - p[0][1]) * s;
    return true;
}

// Wie viele Bildpunkte neben der naechsten Linie des Rasters (Abstand kAbstand, Ursprung 0/0) liegt der Bildpunkt - gemessen
// wie fwidth: der Abstand in Welteinheiten durch die Aenderung je Bildpunkt.
bool linienAbstand(const double inv[16], int w, int h, int x, int y, double zEbene, double& px, double& wx, double& wy, double& fw) {
    double x1 = 0.0;
    double y1 = 0.0;
    double x2 = 0.0;
    double y2 = 0.0;
    if (!aufBoden(inv, w, h, x, y, zEbene, wx, wy) || !aufBoden(inv, w, h, x + 1, y, zEbene, x1, y1) || !aufBoden(inv, w, h, x, y + 1, zEbene, x2, y2)) {
        return false;
    }
    const double fx = std::fabs(x1 - wx) + std::fabs(x2 - wx);
    const double fy = std::fabs(y1 - wy) + std::fabs(y2 - wy);
    const double dx = std::fabs(wx - kAbstand * std::round(wx / kAbstand));
    const double dy = std::fabs(wy - kAbstand * std::round(wy / kAbstand));
    px = std::min(dx / std::max(fx, 1e-9), dy / std::max(fy, 1e-9));
    fw = std::max(fx, fy);
    return true;
}

// Der Grauverlauf des Hintergrunds in Zeile y (modellHintergrundPixelShaderHlsl: 34 + 22 * (1 - v)).
int verlauf(int y, int h) { return static_cast<int>(34.0F + 22.0F * (1.0F - (static_cast<float>(y) + 0.5F) / static_cast<float>(h))); }

// Entlang einer Zeile des Fotos: die Mitten der Stellen, an denen das Raster etwas zeichnet (Unterschied zu `aus`).
std::vector<float> linienInZeile(const Foto& an, const Foto& aus, int y, int schwelle) {
    std::vector<float> mitten;
    int start = -1;
    for (int x = 0; x <= an.w; ++x) {
        const bool linie = x < an.w && abst(an.p(x, y), aus.p(x, y)) > schwelle;
        if (linie && start < 0) { start = x; }
        if (!linie && start >= 0) {
            mitten.push_back((static_cast<float>(start) + static_cast<float>(x - 1)) * 0.5F);
            start = -1;
        }
    }
    return mitten;
}

float medianAbstand(const std::vector<float>& m) {
    if (m.size() < 2) { return 0.0F; }
    std::vector<float> d;
    for (std::size_t i = 1; i < m.size(); ++i) { d.push_back(m[i] - m[i - 1]); }
    std::nth_element(d.begin(), d.begin() + static_cast<std::ptrdiff_t>(d.size() / 2), d.end());
    return d[d.size() / 2];
}

void einAnsicht(AnsichtArt art) {
    t().vierAnsichten = false;
    t().aktiv = 3;
    if (art == AnsichtArt::Perspektive) {
        modellStartansicht(3);
    } else {
        ansicht(3).animStart = -1.0;
        ansicht(3).art = art;
        modellAnsichtEinpassenIn(3);
    }
    g_app->modelDirty = true;
}

// --- die Pruefungen ------------------------------------------------------------------------------------------------

// Raster an und aus: das Foto mit und ohne Raster. Die Figur, der Hintergrund und alles andere sind in beiden gleich.
struct Paar {
    Foto an;
    Foto aus;
};

Paar paar() {
    Paar p;
    ModellSchattierung& sch = ansicht().sch;
    const ModellSchattierung vorher = sch;
    rasterSchalten(sch, true);
    p.an = foto();
    rasterSchalten(sch, false);
    p.aus = foto();
    sch = vorher;
    return p;
}

// Ist der Bildpunkt freier Hintergrund (der Grauverlauf, keine Figur)? Nur ohne grauen Boden und einfarbigen Grund.
bool frei(const Foto& f, int x, int y) {
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const unsigned char* c = f.p(x + dx, y + dy);
            const int v = verlauf(std::clamp(y + dy, 0, f.h - 1), f.h);
            if (std::abs(c[0] - v) > 3 || std::abs(c[1] - v) > 3 || std::abs(c[2] - v) > 3) { return false; }
        }
    }
    return true;
}

// Liegt der Bildpunkt im Innern der Figur (im 5 x 5-Feld kein Bildpunkt des Hintergrunds)? Am Rand der Figur zeigt sich das
// Raster daneben zu Recht.
bool innen(const Foto& f, int x, int y) {
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            const unsigned char* c = f.p(x + dx, y + dy);
            const int v = verlauf(std::clamp(y + dy, 0, f.h - 1), f.h);
            if (std::abs(c[0] - v) <= 3 && std::abs(c[1] - v) <= 3 && std::abs(c[2] - v) <= 3) { return false; }
        }
    }
    return true;
}

// Das Rechteck der Figur auf dem Foto (alle gezeichneten Ecken in der Pose), 6 Bildpunkte Rand.
struct Rechteck {
    float x0 = 1e9F;
    float y0 = 1e9F;
    float x1 = -1e9F;
    float y1 = -1e9F;
    [[nodiscard]] bool drin(float x, float y) const { return x >= x0 && x <= x1 && y >= y0 && y <= y1; }
};

Rechteck figurRechteck(const Foto& f) {
    Rechteck r;
    const auto& haut = modellHaut();
    const float* m = f.vp;
    for (std::size_t i = 0; i < haut.size(); ++i) {
        if (!modellTeilGezeichnet(static_cast<int>(i))) { continue; }
        for (const auto& q : haut[i]) {
            const float cw = m[12] * q[0] + m[13] * q[1] + m[14] * q[2] + m[15];
            if (cw <= 1e-3F) { continue; }
            const float x = ((m[0] * q[0] + m[1] * q[1] + m[2] * q[2] + m[3]) / cw * 0.5F + 0.5F) * static_cast<float>(f.w);
            const float y = (0.5F - (m[4] * q[0] + m[5] * q[1] + m[6] * q[2] + m[7]) / cw * 0.5F) * static_cast<float>(f.h);
            r.x0 = std::min(r.x0, x - 6.0F);
            r.y0 = std::min(r.y0, y - 6.0F);
            r.x1 = std::max(r.x1, x + 6.0F);
            r.y1 = std::max(r.y1, y + 6.0F);
        }
    }
    return r;
}

float figurOben() {
    float z = -1e9F;
    const auto& haut = modellHaut();
    for (std::size_t i = 0; i < haut.size(); ++i) {
        if (!modellTeilGezeichnet(static_cast<int>(i))) { continue; }
        for (const auto& q : haut[i]) { z = std::max(z, q[2]); }
    }
    return z;
}

// Linien in Bodenhoehe alle 8 Einheiten: Punkte auf den Linien (ohne die Achsen) zeigen das Raster, Mitten der Zellen nicht.
// Genommen wird, was im Foto liegt und nicht von der Figur bedeckt sein kann (ausserhalb ihres Rechtecks), innerhalb von
// 48 Einheiten um den Ursprung (3ds Max zeichnet in der Perspektive 7 Linien weit).
bool linienAmOrt(const std::string& wo, const Paar& p, bool zellenLeer) {
    const float z = t().an.bodenZ;
    const Rechteck fig = figurRechteck(p.an);
    int auf = 0;
    int aufDa = 0;
    int zelle = 0;
    int zelleLeer = 0;
    std::string fehlt;
    const auto punkt = [&](float wx, float wy, float& sx, float& sy) { return aufFoto(p.an, wx, wy, z, sx, sy) && !fig.drin(sx, sy); };
    for (int i = -6; i <= 6; ++i) {
        for (int j = -6; j <= 5; ++j) {
            const float a = kAbstand * static_cast<float>(i);
            const float b = kAbstand * static_cast<float>(j) + kAbstand * 0.5F;
            float sx = 0.0F;
            float sy = 0.0F;
            for (int k = 0; k < 2; ++k) {   // auf der Linie x = a (y = b) bzw. y = a (x = b)
                if (i == 0) { continue; }
                const float wx = k == 0 ? a : b;
                const float wy = k == 0 ? b : a;
                if (!punkt(wx, wy, sx, sy)) { continue; }
                ++auf;
                if (unterschiedUm(p.an, p.aus, sx, sy) > 8) {
                    ++aufDa;
                } else if (fehlt.size() < 120) {
                    fehlt += " (" + std::to_string(static_cast<int>(wx)) + "|" + std::to_string(static_cast<int>(wy)) + ")";
                }
            }
            // die Mitte der Zelle (a+4, b): nur, wenn ihre vier Raender mindestens 5 Bildpunkte entfernt liegen
            const float cx = a + kAbstand * 0.5F;
            if (i < 6 && punkt(cx, b, sx, sy)) {
                bool gross = true;
                const float rand[4][2] = {{a, b}, {a + kAbstand, b}, {cx, b - kAbstand * 0.5F}, {cx, b + kAbstand * 0.5F}};
                for (const auto& q : rand) {
                    float rx = 0.0F;
                    float ry = 0.0F;
                    gross = gross && aufFoto(p.an, q[0], q[1], z, rx, ry) && std::hypot(rx - sx, ry - sy) >= 5.0F;
                }
                if (gross) {
                    ++zelle;
                    if (abst(p.an.p(static_cast<int>(sx), static_cast<int>(sy)), p.aus.p(static_cast<int>(sx), static_cast<int>(sy))) <= 3) { ++zelleLeer; }
                }
            }
        }
    }
    char z2[420];
    std::snprintf(z2, sizeof z2, "Knopftest: MBRASTER %s - Punkte auf den Linien %d von %d mit Raster, Mitten der Zellen %d von %d leer%s%s", wo.c_str(), aufDa,
                  auf, zelleLeer, zelle, fehlt.empty() ? "" : "; ohne Raster:", fehlt.c_str());
    diag::detail(z2);
    return auf >= 8 && aufDa == auf && (!zellenLeer || (zelle >= 4 && zelleLeer == zelle));
}

// Der erste Punkt einer Linie des Bodens (ab (x0, y0) in Schritten (dx, dy)), der im Foto auf freiem Hintergrund liegt.
bool freierPunkt(const Paar& p, float x0, float y0, float dx, float dy, float& sx, float& sy, float& wx, float& wy) {
    for (int k = 0; k < 40; ++k) {
        wx = x0 + dx * static_cast<float>(k);
        wy = y0 + dy * static_cast<float>(k);
        if (aufFoto(p.an, wx, wy, t().an.bodenZ, sx, sy) && frei(p.aus, static_cast<int>(sx), static_cast<int>(sy))) { return true; }
    }
    return false;
}

// Die Figur verdeckt das Raster: Punkte der Linien hinter den Fuessen (y ab 12), die im Foto auf der Figur liegen, zeigen
// kein Raster; ein Punkt der Achse x = 0 vor der Figur zeigt es.
bool verdeckt(const std::string& wo, const Paar& p) {
    const float z = t().an.bodenZ;
    int aufFigur = 0;
    int sauber = 0;
    int schlimmst = 0;
    for (int i = -4; i <= 4; ++i) {
        for (float y = 12.0F; y <= 48.0F; y += 2.0F) {
            float sx = 0.0F;
            float sy = 0.0F;
            if (!aufFoto(p.an, kAbstand * static_cast<float>(i), y, z, sx, sy) || !innen(p.aus, static_cast<int>(sx), static_cast<int>(sy))) { continue; }
            ++aufFigur;
            const int d = unterschiedUm(p.an, p.aus, sx, sy);
            schlimmst = std::max(schlimmst, d);
            sauber += d <= 6 ? 1 : 0;
        }
    }
    float vx = 0.0F;
    float vy = 0.0F;
    float wx = 0.0F;
    float wy = 0.0F;
    const bool vorDa = freierPunkt(p, 0.0F, -6.0F, 0.0F, -2.0F, vx, vy, wx, wy);
    const int vorne = vorDa ? unterschiedUm(p.an, p.aus, vx, vy) : -1;
    char z2[260];
    std::snprintf(z2, sizeof z2, "Knopftest: MBRASTER %s - %d Punkte der Linien hinter den Fuessen liegen auf der Figur, %d davon ohne Raster (groesster "
                  "Unterschied %d); Achse vor der Figur bei y %.0f: %d", wo.c_str(), aufFigur, sauber, schlimmst, static_cast<double>(wy), vorne);
    diag::detail(z2);
    return aufFigur >= 10 && sauber == aufFigur && vorne > 12;
}

// Duenn und an der gerechneten Stelle (Perspektive, schraeger Blick): jeder Bildpunkt, an dem das Raster zeichnet, liegt
// hoechstens 2 Bildpunkte neben einer Linie x = 8k / y = 8k in Bodenhoehe; im Max-Modus fehlt auf den Linien innerhalb der
// Ausdehnung (7 Linien wie Max' "Perspective View Grid Extent") nichts.
bool duennUndDa(const std::string& wo, const Paar& p, bool fehlendePruefen) {
    double inv[16];
    if (!p.an.da() || !p.aus.da() || !umkehren(p.an.vp, inv)) { return false; }
    const double z = t().an.bodenZ;
    int linie = 0;
    int weit = 0;
    int soll = 0;
    int fehlt = 0;
    double schlimmst = 0.0;
    for (int y = 0; y < p.an.h; ++y) {
        for (int x = 0; x < p.an.w; ++x) {
            double d = 0.0;
            double wx = 0.0;
            double wy = 0.0;
            double fw = 0.0;
            if (!linienAbstand(inv, p.an.w, p.an.h, x, y, z, d, wx, wy, fw)) { continue; }
            if (std::fabs(wx) > 54.0 || std::fabs(wy) > 54.0) { continue; }
            const bool zeichnet = abst(p.an.p(x, y), p.aus.p(x, y)) > 8;
            if (zeichnet) {
                ++linie;
                if (d > 2.0) {
                    ++weit;
                    schlimmst = std::max(schlimmst, d);
                }
            }
            // auf einer Linie, Zellen mindestens 4 Bildpunkte gross, dort nur Hintergrund (nicht die Figur)
            if (fehlendePruefen && d < 0.25 && fw < kAbstand / 4.0 && std::abs(p.aus.p(x, y)[0] - verlauf(y, p.an.h)) <= 2) {
                ++soll;
                if (!zeichnet) { ++fehlt; }
            }
        }
    }
    char z2[260];
    std::snprintf(z2, sizeof z2,
                  "Knopftest: MBRASTER %s - %d Bildpunkte des Rasters, davon %d mehr als 2 Punkte neben einer Linie (weitester %.1f); auf den Linien "
                  "%d von %d gezeichnet",
                  wo.c_str(), linie, weit, schlimmst, soll - fehlt, soll);
    diag::detail(z2);
    return linie >= 500 && weit * 200 <= linie && (!fehlendePruefen || (soll >= 200 && fehlt * 33 <= soll));
}

// Die Farbe des Bildpunkts mit Raster an einem Ort des Bodens.
bool farbeAm(const Foto& f, float wx, float wy, float wz, int rgb[3]) {
    float sx = 0.0F;
    float sy = 0.0F;
    if (!aufFoto(f, wx, wy, wz, sx, sy)) { return false; }
    // die kraeftigste Farbe im 3x3-Feld (Kantenglaettung)
    int best = -1;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const unsigned char* c = f.p(static_cast<int>(sx) + dx, static_cast<int>(sy) + dy);
            const int s = std::max({c[0], c[1], c[2]}) - std::min({c[0], c[1], c[2]});
            if (s > best) {
                best = s;
                for (int k = 0; k < 3; ++k) { rgb[k] = c[k]; }
            }
        }
    }
    return true;
}

// Ortho bildfuellend und der Abstand passt sich dem Zoom an: entlang einer Zeile ueber dem Kopf (dort kreuzen nur die
// senkrechten Linien) reichen die Linien von Rand zu Rand, und sie liegen weder dichter als 4 Bildpunkte noch so weit, dass
// keine zwei mehr in der Ansicht stehen.
bool orthoZeile(const std::string& wo, const Paar& p, int y, bool randPruefen, float& abstand) {
    const std::vector<float> m = linienInZeile(p.an, p.aus, y, 6);
    abstand = medianAbstand(m);
    const float w = static_cast<float>(p.an.w);
    const bool rand = !m.empty() && m.front() < w * 0.1F && m.back() > w * 0.9F;
    char z2[220];
    std::snprintf(z2, sizeof z2, "Knopftest: MBRASTER %s - Zeile %d: %d Linien, erste %.0f, letzte %.0f von %d, Abstand %.1f Bildpunkte", wo.c_str(), y,
                  static_cast<int>(m.size()), m.empty() ? -1.0 : static_cast<double>(m.front()), m.empty() ? -1.0 : static_cast<double>(m.back()), p.an.w,
                  static_cast<double>(abstand));
    diag::detail(z2);
    return (!randPruefen || rand) && abstand >= 4.0F && abstand <= w / 2.0F;
}

// Die Zeile des Fotos zwischen zwei waagrechten Linien ueber dem Kopf der Figur (Vorne: Welt-z, Linien alle 8 ab der Bodenhoehe).
// `zelle`: die feinste Stufe, die dort zu sehen sein kann (eingepasst: Blender zeigt Linien alle 1 Einheit, 3ds Max alle 8).
int zeileZwischen(const Foto& f, float zNah, float zelle) {
    const float b = t().an.bodenZ;
    const float z = b + zelle * (std::floor((zNah - b) / zelle) + 0.5F);
    float sx = 0.0F;
    float sy = 0.0F;
    if (!aufFoto(f, 0.0F, 0.0F, z, sx, sy)) { return f.h / 8; }
    return std::clamp(static_cast<int>(sy), 2, f.h - 3);
}

std::vector<Schritt> rasterSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // Kamera, Zoom, Modus oder Hintergrund aendern, dann ein Bild warten: die Pruefung rechnet mit der Matrix der Ansicht
    auto stelle = [&](const std::string& was, std::function<void()> tun) {
        add(tu("MBRASTER: " + was, std::move(tun)));
        add(pause(0.3));
    };
    auto blick = [](float neigung, float gier) {
        const float w[3] = {neigung, gier, 0.0F};
        modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
        g_app->modelDirty = true;
    };
    stelle("Skript fuer den Test, Max-Modus, eine Ansicht", [] {
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mbr.txt");
        modellBedienungSetzen(0);
        g_app->settings.modellNavigation = true;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        g_app->settings.modellHintergrund = -1;
        g_app->modellKnochen = false;
    });
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    add(pause(0.4));
    // --- ab Werk an, in jeder Ansicht -----------------------------------------------------------------------------
    add(pruefe("MBRASTER 1: das Raster ist ab Werk in allen vier Ansichten an (wie 3ds Max und Blender)", [] {
        bool an = true;
        for (int i = 0; i < 4; ++i) { an = an && rasterAbWerk(ansicht(i).sch); }
        return an;
    }));
    stelle("Perspektive von vorne", [] { einAnsicht(AnsichtArt::Perspektive); });
    add(pruefe("MBRASTER 1: ab Werk sichtbar - im Bild der Startansicht liegen Linien alle 8 Einheiten in Bodenhoehe (Max)", [] {
        const Foto abWerk = foto();
        Paar p = paar();
        p.an = abWerk;   // so, wie die Ansicht nach dem Laden aussieht
        return linienAmOrt("ab Werk, Max, Perspektive", p, true);
    }));
    add(pruefe("MBRASTER 6: Max - die Figur verdeckt das Raster (Linien hinter den Fuessen), vor der Figur ist es zu sehen", [] {
        return verdeckt("Max, verdeckt", paar());
    }));
    add(pruefe("MBRASTER 3: Max - die Weltachsen sind dunkler als der Hintergrund hervorgehoben (Home Grid: Achsen schwarz)", [] {
        const Paar p = paar();
        float sx = 0.0F;
        float sy = 0.0F;
        float wx = 0.0F;
        float wy = 0.0F;
        bool ok = true;
        std::string zeile;
        for (int k = 0; k < 2; ++k) {   // die X-Achse (y = 0) neben der Figur, die Y-Achse (x = 0) vor ihr
            const bool da = k == 0 ? freierPunkt(p, 20.0F, 0.0F, 2.0F, 0.0F, sx, sy, wx, wy) : freierPunkt(p, 0.0F, -6.0F, 0.0F, -2.0F, sx, sy, wx, wy);
            int dunkel = 255;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) { dunkel = std::min(dunkel, hell(p.an.p(static_cast<int>(sx) + dx, static_cast<int>(sy) + dy))); }
            }
            const int grund = hell(p.aus.p(static_cast<int>(sx), static_cast<int>(sy)));
            zeile += std::string(k == 0 ? " X" : " Y") + " bei " + std::to_string(static_cast<int>(wx)) + "|" + std::to_string(static_cast<int>(wy)) + ": " +
                     std::to_string(dunkel) + " gegen Hintergrund " + std::to_string(grund) + ";";
            ok = ok && da && dunkel + 15 < grund;
        }
        diag::detail("Knopftest: MBRASTER Max-Achsen -" + zeile);
        return ok;
    }));
    stelle("Max, schraeg von vorne links, flach ueber dem Boden", [=] { blick(14.0F, 62.0F); });
    add(pruefe("MBRASTER 2: duenn und an der Stelle - schraeg von vorne links, flach ueber dem Boden (Max)", [] {
        return duennUndDa("Max, schraeg", paar(), true);
    }));
    // --- Orthoansicht: bildfuellend, der Abstand passt sich dem Zoom an --------------------------------------------
    for (const int bl : {0, 1}) {
        const std::string modus = bl != 0 ? "Blender" : "Max";
        stelle("Ansicht Vorne (" + modus + ")", [bl] {
            modellBedienungSetzen(bl);
            einAnsicht(AnsichtArt::Vorne);
        });
        auto abstand1 = std::make_shared<float>(0.0F);
        add(pruefe("MBRASTER 4: " + modus + " Vorne - das Raster fuellt die Ansicht bis in die vier Ecken", [] {
            const Paar p = paar();
            int ecken = 0;
            std::string zahl;
            for (int e = 0; e < 4; ++e) {
                const int x0 = (e % 2) == 0 ? 0 : p.an.w * 9 / 10;
                const int y0 = (e / 2) == 0 ? 0 : p.an.h * 9 / 10;
                int n = 0;
                for (int y = y0; y < y0 + p.an.h / 10; ++y) {
                    for (int x = x0; x < x0 + p.an.w / 10; ++x) { n += abst(p.an.p(x, y), p.aus.p(x, y)) > 6 ? 1 : 0; }
                }
                ecken += n >= 20 ? 1 : 0;
                zahl += " " + std::to_string(n);
            }
            diag::detail("Knopftest: MBRASTER Ortho bildfuellend - Rasterpunkte in den Ecken" + zahl);
            return ecken == 4;
        }));
        add(pruefe("MBRASTER 6: " + modus + " Vorne - die Figur verdeckt das Raster (es liegt hinter ihr)", [] {
            const Paar p = paar();
            float sx = 0.0F;
            float sy = 0.0F;
            const float mitteZ = (g_app->model.mins[2] + g_app->model.maxs[2]) * 0.5F;
            return aufFoto(p.an, 0.0F, 0.0F, mitteZ, sx, sy) && unterschiedUm(p.an, p.aus, sx, sy) <= 3;
        }));
        add(pruefe("MBRASTER 5: " + modus + " Vorne eingepasst - Linien ueber dem Kopf von Rand zu Rand, 4 Bildpunkte bis halbe Breite auseinander", [=] {
            const Paar p = paar();
            return orthoZeile(modus + " Vorne eingepasst", p, zeileZwischen(p.an, figurOben() + 4.0F, 1.0F), true, *abstand1);
        }));
        auto hoehe = std::make_shared<float>(0.0F);
        stelle(modus + " Vorne 64-mal weiter weg", [=] {
            *hoehe = ansicht().halbHoehe;
            ansicht().halbHoehe = *hoehe * 64.0F;
            g_app->modelDirty = true;
        });
        add(pruefe("MBRASTER 5: " + modus + " Vorne 64-mal weiter weg - der Abstand waechst mit (Linien nie dichter als 4 Bildpunkte, bildfuellend)", [=] {
            float d = 0.0F;
            const Paar p = paar();
            return orthoZeile(modus + " Vorne x64", p, zeileZwischen(p.an, ansicht().mitte[2], 64.0F), true, d);
        }));
        stelle(modus + " Vorne wieder eingepasst", [=] {
            ansicht().halbHoehe = *hoehe;
            g_app->modelDirty = true;
        });
        if (bl != 0) {
            auto mitte = std::make_shared<float>(0.0F);
            stelle("Blender Vorne 16-mal naeher, neben der Figur", [=] {
                *hoehe = ansicht().halbHoehe;
                *mitte = ansicht().mitte[0];
                ansicht().halbHoehe = *hoehe / 16.0F;
                ansicht().mitte[0] = *mitte + 60.0F;   // neben der Figur (sie verdeckt das Raster)
                g_app->modelDirty = true;
            });
            add(pruefe("MBRASTER 5: Blender Vorne 16-mal naeher - Unterteilungen erscheinen (Blender: 3 Stufen feiner als der Massstab)", [=] {
                float d = 0.0F;
                const Paar p = paar();
                return orthoZeile("Blender Vorne /16", p, zeileZwischen(p.an, ansicht().mitte[2], 0.125F), true, d);
            }));
            stelle("Blender Vorne wieder eingepasst", [=] {
                ansicht().halbHoehe = *hoehe;
                ansicht().mitte[0] = *mitte;
                g_app->modelDirty = true;
            });
        }
    }
    // --- Blender: Achsen in ihren Farben, Boden blendet zum Horizont aus -------------------------------------------
    stelle("Blender, Perspektive von vorne", [] {
        modellBedienungSetzen(1);
        einAnsicht(AnsichtArt::Perspektive);
    });
    add(pruefe("MBRASTER 1: Blender - ab Werk Linien alle 8 Einheiten in Bodenhoehe", [] { return linienAmOrt("Blender, Perspektive", paar(), true); }));
    add(pruefe("MBRASTER 6: Blender - die Figur verdeckt das Raster (Linien hinter den Fuessen), vor der Figur ist es zu sehen", [] {
        return verdeckt("Blender, verdeckt", paar());
    }));
    add(pruefe("MBRASTER 3: Blender - die X-Achse rot, die Y-Achse gruen, ohne Z-Achse (wie Blender ab Werk)", [] {
        const Paar p = paar();
        float sx = 0.0F;
        float sy = 0.0F;
        float wx = 0.0F;
        float wy = 0.0F;
        int x[3] = {};
        int y[3] = {};
        const bool ok = freierPunkt(p, 20.0F, 0.0F, 2.0F, 0.0F, sx, sy, wx, wy) && farbeAm(p.an, wx, wy, t().an.bodenZ, x) &&
                        freierPunkt(p, 0.0F, -6.0F, 0.0F, -2.0F, sx, sy, wx, wy) && farbeAm(p.an, wx, wy, t().an.bodenZ, y);
        float kx = 0.0F;
        float ky = 0.0F;
        const bool kopf = aufFoto(p.an, 0.0F, 0.0F, figurOben() + 2.0F, kx, ky);
        const int ueberKopf = kopf ? unterschiedUm(p.an, p.aus, kx, ky) : -1;
        char z2[200];
        std::snprintf(z2, sizeof z2, "Knopftest: MBRASTER Blender-Achsen - X %d %d %d, Y %d %d %d, ueber dem Kopf (Z-Achse aus) %d", x[0], x[1], x[2], y[0],
                      y[1], y[2], ueberKopf);
        diag::detail(z2);
        return ok && kopf && x[0] > x[1] + 50 && x[0] > x[2] + 30 && y[1] > y[0] + 30 && y[1] > y[2] + 60 && ueberKopf <= 6;
    }));
    add(pruefe("MBRASTER 3: Blender - Achse Z eingeschaltet: blau senkrecht durch den Ursprung (ueber dem Kopf)", [] {
        ModellSchattierung& sch = ansicht().sch;
        zAchseSetzen(sch, true);
        const Foto an = foto();
        zAchseSetzen(sch, false);
        const Foto aus = foto();
        float sx = 0.0F;
        float sy = 0.0F;
        int c[3] = {};
        const float z = figurOben() + 2.0F;
        const bool ok = aufFoto(an, 0.0F, 0.0F, z, sx, sy) && farbeAm(an, 0.0F, 0.0F, z, c);
        diag::detail("Knopftest: MBRASTER Blender Z-Achse - " + std::to_string(c[0]) + " " + std::to_string(c[1]) + " " + std::to_string(c[2]) +
                     ", Unterschied " + std::to_string(ok ? unterschiedUm(an, aus, sx, sy) : -1));
        return ok && unterschiedUm(an, aus, sx, sy) > 25 && c[2] > c[0] + 40;
    }));
    stelle("Blender, schraeg und flach", [=] { blick(14.0F, 62.0F); });
    add(pruefe("MBRASTER 2: Blender - duenn und an der Stelle, schraeg und flach", [] { return duennUndDa("Blender, schraeg", paar(), false); }));
    stelle("Blender, flach von vorne", [=] { blick(8.0F, 90.0F); });
    add(pruefe("MBRASTER 3: Blender - der Boden blendet zum Horizont aus (weit hinten schwaecher als nah)", [] {
        const Paar p = paar();
        int nah = 0;
        int fern = 0;
        float sx = 0.0F;
        float sy = 0.0F;
        float wx = 0.0F;
        float wy = 0.0F;
        // nah: die Linie x = 24 vor der Figur; fern: dieselbe Linie 2000 bis 6000 Einheiten weiter hinten
        if (freierPunkt(p, 24.0F, -4.0F, 0.0F, -2.0F, sx, sy, wx, wy)) { nah = unterschiedUm(p.an, p.aus, sx, sy); }
        for (int k = 1; k <= 3; ++k) {
            if (aufFoto(p.an, 24.0F, 2000.0F * static_cast<float>(k), t().an.bodenZ, sx, sy)) { fern = std::max(fern, unterschiedUm(p.an, p.aus, sx, sy)); }
        }
        diag::detail("Knopftest: MBRASTER Blender Horizont - nah " + std::to_string(nah) + " (y " + std::to_string(static_cast<int>(wy)) + "), fern " +
                     std::to_string(fern));
        return nah > 12 && fern * 2 < nah;
    }));
    // --- je Ansicht, beide Farbschemen, heller Hintergrund, grauer Boden ---------------------------------------------
    stelle("Max, vier Ansichten, Oben aktiv", [] {
        modellBedienungSetzen(0);
        t().vierAnsichten = true;
        t().aktiv = 0;
        g_app->modelDirty = true;
    });
    add(pause(0.2));
    add(pruefe("MBRASTER 7: G gilt je Ansicht - Raster aus in Oben aendert Oben, nicht die Perspektive", [] {
        const Foto oben = foto();
        t().aktiv = 3;
        const Foto persp = foto();
        t().aktiv = 0;
        ansicht(0).sch.raster = !ansicht(0).sch.raster;
        const Foto oben2 = foto();
        t().aktiv = 3;
        const Foto persp2 = foto();
        t().aktiv = 0;
        ansicht(0).sch.raster = !ansicht(0).sch.raster;
        const int d0 = mb1::unterschied(oben.px, oben2.px);
        const int d3 = mb1::unterschied(persp.px, persp2.px);
        diag::detail("Knopftest: MBRASTER je Ansicht - Oben " + std::to_string(d0) + " Punkte anders, Perspektive " + std::to_string(d3));
        return d0 > 200 && d3 == 0;
    }));
    stelle("eine Ansicht, Perspektive", [] { einAnsicht(AnsichtArt::Perspektive); });
    for (const char* id : {"light", "dark"}) {
        const std::string tid = id;
        for (const int bl : {0, 1}) {
            stelle(std::string("Farbschema ") + id + (bl != 0 ? ", Blender" : ", Max"), [tid, bl] {
                if (const theme::Theme* th = theme::findTheme(tid)) {
                    g_app->activeTheme = th;
                    g_app->themePending = true;
                }
                modellBedienungSetzen(bl);
            });
            add(pruefe(std::string("MBRASTER 9: Farbschema ") + id + (bl != 0 ? ", Blender" : ", Max") + " - das Raster ist zu sehen", [tid, bl] {
                return linienAmOrt(std::string("Farbschema ") + tid + (bl != 0 ? ", Blender" : ", Max"), paar(), false);
            }));
        }
    }
    for (const int bl : {0, 1}) {
        stelle(std::string("heller einfarbiger Hintergrund B4B4B4, ") + (bl != 0 ? "Blender" : "Max"), [bl] {
            g_app->settings.modellHintergrund = 0xB4B4B4;
            modellBedienungSetzen(bl);
            g_app->modelDirty = true;
        });
        add(pruefe(std::string("MBRASTER 9: heller Hintergrund (wie Max hell) - ") + (bl != 0 ? "Blender" : "Max") + ": Linien dunkler als der Grund", [bl] {
            const Paar p = paar();
            bool ok = linienAmOrt(std::string("heller Grund") + (bl != 0 ? ", Blender" : ", Max"), p, false);
            float sx = 0.0F;
            float sy = 0.0F;
            const Rechteck fig = figurRechteck(p.an);
            bool da = false;
            // auf einer Linie x = 24 ... 48 (auch links), in der Mitte zwischen zwei Querlinien, neben der Figur
            for (int i = 3; i <= 6 && !da; ++i) {
                for (const float seite : {1.0F, -1.0F}) {
                    for (float y = -44.0F; y <= 44.0F && !da; y += 8.0F) {
                        da = aufFoto(p.an, seite * kAbstand * static_cast<float>(i), y, t().an.bodenZ, sx, sy) && !fig.drin(sx, sy);
                    }
                }
            }
            int dunkelst = 255;
            const int b = da ? hell(p.aus.p(static_cast<int>(sx), static_cast<int>(sy))) : 0;
            for (int dy = -1; da && dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) { dunkelst = std::min(dunkelst, hell(p.an.p(static_cast<int>(sx) + dx, static_cast<int>(sy) + dy))); }
            }
            diag::detail("Knopftest: MBRASTER heller Grund - Linie " + std::to_string(dunkelst) + ", Grund " + std::to_string(b));
            ok = ok && da && dunkelst + 15 < b;
            return ok;
        }));
    }
    stelle("Max, Verlauf, grauer Boden", [] {
        modellBedienungSetzen(0);
        g_app->settings.modellHintergrund = -1;
        t().an.boden = true;
        g_app->modelDirty = true;
    });
    for (const float gier : {89.9F, 90.0F, 90.1F, 90.37F}) {
        stelle("grauer Boden, Gier " + std::to_string(gier), [=] { blick(20.0F, gier); });
        add(pruefe("MBRASTER 8: grauer Boden und Raster zugleich - die Linien liegen auf dem Boden (Gier " + std::to_string(gier) + ", ohne Flimmern)", [gier] {
            return linienAmOrt("grauer Boden, Gier " + std::to_string(gier), paar(), false);
        }));
    }
    stelle("Ansicht Oben mit grauem Boden", [] { einAnsicht(AnsichtArt::Oben); });
    add(pruefe("MBRASTER 10: Ansicht Oben mit grauem Boden - das Raster liegt ueber dem Boden, hinter der Figur", [] {
        return linienAmOrt("Oben mit grauem Boden", paar(), false);
    }));
    // --- Bedienung: Menue [+] > Rastereinstellungen, Rechtsklick, Overlays > Boden, Suche, Abstand, Tiefster Punkt -----------
    stelle("Bedienung: Max, eine Ansicht, Perspektive, ohne grauen Boden", [] {
        t().an.boden = false;
        modellBedienungSetzen(0);
        einAnsicht(AnsichtArt::Perspektive);
    });
    add(klickAn("MBRASTER Bedienung: [+]", mbkopf::feldOrt(3, 0)));
    add(pause(0.3));
    add(klick(tr(Str::MrRasterEinst)));
    add(pause(0.3));
    add(pruefe("MBRASTER Bedienung: [+] > Rastereinstellungen zeigt Abstand, Hauptlinien, Groesse in der Perspektive, Bodenhoehe und Tiefster Punkt", [] {
        return finde(tr(Str::MrAbstand), "", 0, false) != nullptr && finde(tr(Str::MrHaupt), "", 0, false) != nullptr &&
               finde(tr(Str::MrAusdehnung), "", 0, false) != nullptr && finde(tr(Str::MbAnBodenZ), "", 0, false) != nullptr &&
               finde(tr(Str::MrTiefster), "", 0, false) != nullptr;
    }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    auto vorher = std::make_shared<bool>(false);
    add(tu("MBRASTER Bedienung: Raster merken", [=] { *vorher = ansicht(3).sch.raster; }));
    add(klickAn("MBRASTER Bedienung: Rechtsklick in die Ansicht", [] {
        const ModellAnsicht& a = ansicht(3);
        return ImVec2{a.lo[0] + a.groesse[0] * 0.15F, a.lo[1] + a.groesse[1] * 0.5F};
    }, 1));
    add(pause(0.3));
    add(klick(tr(Str::MbRAnzeige)));
    add(pause(0.3));
    add(klick(tr(Str::MkRaster)));
    add(pause(0.2));
    add(pruefe("MBRASTER Bedienung: Rechtsklick > Anzeige > Raster schaltet das Raster der Ansicht", [=] { return ansicht(3).sch.raster != *vorher; }));
    add(tu("MBRASTER Bedienung: Raster wieder an", [] { ansicht(3).sch.raster = true; }));
    add(tasteKombi(ImGuiKey_G, "G"));
    add(pause(0.2));
    add(pruefe("MBRASTER Bedienung: G schaltet das Raster der aktiven Ansicht (Max)", [] { return !ansicht(3).sch.raster; }));
    add(tasteKombi(ImGuiKey_G, "G"));
    add(pause(0.2));
    stelle("Bedienung: Rasterabstand 16", [] {
        g_app->settings.modellRasterAbstand = 16.0F;
        g_app->modelDirty = true;
    });
    add(pruefe("MBRASTER Bedienung: Rasterabstand 16 - Linien bei x = 16 und 32, keine mehr bei x = 8 und 24", [] {
        const Paar p = paar();
        const Rechteck fig = figurRechteck(p.an);
        int auf = 0;
        int aufDa = 0;
        int zwischen = 0;
        int zwischenLeer = 0;
        for (int i = -6; i <= 6; ++i) {
            if (i == 0) { continue; }
            for (float y = -44.0F; y <= 44.0F; y += 16.0F) {   // Mitten zwischen den Querlinien (alle 16)
                float sx = 0.0F;
                float sy = 0.0F;
                if (!aufFoto(p.an, 8.0F * static_cast<float>(i), y, t().an.bodenZ, sx, sy) || fig.drin(sx, sy)) { continue; }
                const int d = unterschiedUm(p.an, p.aus, sx, sy);
                if (i % 2 == 0) {
                    ++auf;
                    aufDa += d > 8 ? 1 : 0;
                } else {
                    ++zwischen;
                    zwischenLeer += abst(p.an.p(static_cast<int>(sx), static_cast<int>(sy)), p.aus.p(static_cast<int>(sx), static_cast<int>(sy))) <= 3 ? 1 : 0;
                }
            }
        }
        diag::detail("Knopftest: MBRASTER Abstand 16 - auf den Linien " + std::to_string(aufDa) + " von " + std::to_string(auf) + ", dazwischen leer " +
                     std::to_string(zwischenLeer) + " von " + std::to_string(zwischen));
        return auf >= 4 && aufDa == auf && zwischen >= 2 && zwischenLeer == zwischen;
    }));
    stelle("Bedienung: Rasterabstand wieder 8", [] {
        g_app->settings.modellRasterAbstand = 8.0F;
        g_app->modelDirty = true;
    });
    add(pruefe("MBRASTER Bedienung: Tiefster Punkt legt den Boden unter den tiefsten Punkt der Figur (auf 1/8 abgerundet)", [] {
        const float vorherZ = t().an.bodenZ;
        float tief = 1e9F;
        const auto& haut = modellHaut();
        for (std::size_t i = 0; i < haut.size(); ++i) {
            if (!modellTeilGezeichnet(static_cast<int>(i))) { continue; }
            for (const auto& q : haut[i]) { tief = std::min(tief, q[2]); }
        }
        modellBodenAufTiefstenPunkt();
        const float z = t().an.bodenZ;
        t().an.bodenZ = vorherZ;
        g_app->modelDirty = true;
        diag::detail("Knopftest: MBRASTER Tiefster Punkt - tiefste Ecke " + std::to_string(tief) + ", Boden " + std::to_string(z));
        return z <= tief && tief - z < 0.125F;
    }));
    stelle("Bedienung: Blender", [] { modellBedienungSetzen(1); });
    add(klickAn("MBRASTER Bedienung: Overlays", mbkopf::feldOrt(3, 3)));
    add(pause(0.3));
    add(pruefe("MBRASTER Bedienung: Overlays > Hilfslinien mit Raster, Boden, Massstab und Unterteilungen", [] {
        return finde(tr(Str::MkRaster), "", 0, false) != nullptr && finde(tr(Str::MrBoden), "", 0, false) != nullptr &&
               finde(tr(Str::MrMassstab), "", 0, false) != nullptr && finde(tr(Str::MrUnterteil), "", 0, false) != nullptr;
    }));
    add(klick(tr(Str::MrBoden)));
    add(pause(0.2));
    add(pruefe("MBRASTER Bedienung: Overlays > Boden schaltet den Boden der Ansicht (Blender Floor)", [] { return !ansicht(3).sch.rasterBoden; }));
    add(tu("MBRASTER Bedienung: Boden wieder an", [] { ansicht(3).sch.rasterBoden = true; }));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(tasteKombi(ImGuiKey_F3, "F3"));
    add(pause(0.3));
    add(tippe(tr(Str::MrAchseZ)));
    add(pause(0.2));
    add(klick(std::string("[  ] ") + tr(Str::MrAchseZ)));
    add(pause(0.2));
    add(pruefe("MBRASTER Bedienung: F3-Suche findet Achse Z und schaltet sie in der aktiven Ansicht", [] { return ansicht(3).sch.achsen[2]; }));
    add(tu("MBRASTER Bedienung: Achse Z aus", [] { ansicht(3).sch.achsen[2] = false; }));
    add(pause(0.2));
    stelle("zurueck", [] {
        t().an.boden = false;
        modellBedienungSetzen(0);
        t().vierAnsichten = false;
        einAnsicht(AnsichtArt::Perspektive);
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    });
    return s;
}

// --- MBKOPF-Abnahme: Gizmo und Kompass am Fensterbild ---------------------------------------------------------------

struct Fenster {
    int w = 0;
    int h = 0;
    std::vector<unsigned char> bgra;
    [[nodiscard]] bool da() const { return w > 0 && h > 0; }
    [[nodiscard]] const unsigned char* p(int x, int y) const {
        x = std::clamp(x, 0, w - 1);
        y = std::clamp(y, 0, h - 1);
        return &bgra[(static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4U];
    }
};

// Das Hauptfenster als Bild (render::fotoAnfordern: 32 Bit, von oben) in den Laufordner, dann gelesen.
Schritt fensterFoto(const std::string& name, std::shared_ptr<Fenster> ziel) {
    return {"MBRASTER: Fensterbild " + name, [=](int b) {
                const std::string pfad = platform::executableDirectory() + "\\mbr_" + name + ".bmp";
                if (b == 2) { render::fotoAnfordern(pfad); }
                if (b < 8) { return false; }
                std::ifstream f(::bhed::utf8Pfad(pfad), std::ios::binary);
                unsigned char kopf[54] = {};
                *ziel = Fenster{};
                if (f.read(reinterpret_cast<char*>(kopf), sizeof kopf)) {
                    const auto i32 = [&](int at) {
                        return static_cast<std::int32_t>(static_cast<std::uint32_t>(kopf[at]) | (static_cast<std::uint32_t>(kopf[at + 1]) << 8U) |
                                                         (static_cast<std::uint32_t>(kopf[at + 2]) << 16U) | (static_cast<std::uint32_t>(kopf[at + 3]) << 24U));
                    };
                    const int w = i32(18);
                    const int h = -i32(22);
                    if (w > 0 && h > 0 && kopf[28] == 32) {
                        ziel->bgra.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
                        if (f.read(reinterpret_cast<char*>(ziel->bgra.data()), static_cast<std::streamsize>(ziel->bgra.size()))) {
                            ziel->w = w;
                            ziel->h = h;
                        }
                    }
                }
                return true;
            }};
}

int hellBgra(const unsigned char* c) { return (c[2] * 30 + c[1] * 59 + c[0] * 11) / 100; }

std::vector<Schritt> kopfZusatzSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // (a) Blender-Gizmo: von vorne, 20 Grad von oben - die Kugel Y (hinten) und die Gegenkugel -Y (vorne) ueberlappen sich
    add(tu("MBKOPF A: Model-Tab, Blender, eine Ansicht, von vorne 10 Grad von oben (Y und -Y ueberdecken sich), Maus weg vom Gizmo", [] {
        g_app->leftMode = 2;   // die Gruppe kopf endet im Mission-Tab
        modellBedienungSetzen(1);
        t().vierAnsichten = false;
        t().aktiv = 3;
        g_app->settings.modellNavigation = true;
        const float w[3] = {10.0F, 90.0F, 0.0F};
        modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
        const ModellAnsicht& a = ansicht(3);
        setzeMaus(ImVec2{a.lo[0] + 40.0F, a.lo[1] + a.groesse[1] - 40.0F});
    }));
    add(pause(0.5));
    auto gizmoBild = std::make_shared<Fenster>();
    add(fensterFoto("gizmo", gizmoBild));
    add(pruefe("MBKOPF A: Gizmo - wo sich Y (hinten) und -Y (vorne) ueberdecken, liegt die vordere Gegenkugel deckend oben (Blender)", [=] {
        ImVec2 y;
        ImVec2 ny;
        ImVec2 k0;
        ImVec2 k1;
        if (!gizmoBild->da() || !modellGizmoOrt(3, 2, y) || !modellGizmoOrt(3, 3, ny) || !modellGizmoOrt(3, 6, k0) || !modellGizmoOrt(3, 7, k1)) {
            return false;
        }
        // Radius der Kugeln aus dem Abstand der Knoepfe (gizmo(): Kugel 0,22 des Radius, Knoepfe 2,4 * 0,30 des Radius auseinander)
        const float r = std::hypot(k1.x - k0.x, k1.y - k0.y) * 0.22F / 0.72F;
        const float d = std::hypot(ny.x - y.x, ny.y - y.y);
        if (d < 1.0F) { return false; }
        // die reine Farbe der vorderen Kugel: auf -Y, von Y abgewandt (ausserhalb der Kugel Y)
        const ImVec2 weg{(ny.x - y.x) / d, (ny.y - y.y) / d};
        const unsigned char* f = gizmoBild->p(static_cast<int>(ny.x + weg.x * r * 0.5F), static_cast<int>(ny.y + weg.y * r * 0.5F));
        // die Linse, in der beide Kugeln liegen (innen, ohne Rand und Kantenglaettung): dort muss die vordere Kugel zu sehen sein
        int linse = 0;
        int anders = 0;
        for (int py = static_cast<int>(std::min(y.y, ny.y) - r); py <= static_cast<int>(std::max(y.y, ny.y) + r); ++py) {
            for (int px = static_cast<int>(std::min(y.x, ny.x) - r); px <= static_cast<int>(std::max(y.x, ny.x) + r); ++px) {
                const float fx = static_cast<float>(px) + 0.5F;
                const float fy = static_cast<float>(py) + 0.5F;
                if (std::hypot(fx - y.x, fy - y.y) > r * 0.8F || std::hypot(fx - ny.x, fy - ny.y) > r * 0.8F) { continue; }
                ++linse;
                const unsigned char* c = gizmoBild->p(px, py);
                anders += std::max({std::abs(c[0] - f[0]), std::abs(c[1] - f[1]), std::abs(c[2] - f[2])}) > 12 ? 1 : 0;
            }
        }
        char z[260];
        std::snprintf(z, sizeof z, "Knopftest: MBKOPF A Gizmo - Radius %.1f, Abstand Y/-Y %.1f, vordere Kugel -Y %d %d %d; Ueberdeckung %d Bildpunkte, davon %d "
                      "nicht in der Farbe von -Y", static_cast<double>(r), static_cast<double>(d), f[2], f[1], f[0], linse, anders);
        diag::detail(z);
        return linse >= 20 && anders <= 2;
    }));
    // (b) ViewCube mit Kompass, Maus auf dem Wuerfel (deckend): jeder Buchstabe ist zu sehen
    add(tu("MBKOPF B: Max, Kompass an, von vorne 25 Grad von oben (N liegt hinter dem Wuerfel)", [] {
        modellBedienungSetzen(0);
        g_app->settings.modellKompass = true;
        g_app->settings.modellKompassNord = 0;
        const float w[3] = {25.0F, 90.0F, 0.0F};
        modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
    }));
    add(pause(0.5));
    add(tu("MBKOPF B: Maus auf die Flaeche Vorne des ViewCube (deckend)", [] {
        ImVec2 p;
        if (modellWuerfelOrt(3, 0, -1, 0, p)) { setzeMaus(p); }
    }));
    add(pause(0.5));
    auto orte = std::make_shared<std::array<ImVec2, 4>>();
    add(tu("MBKOPF B: Orte der Buchstaben merken", [=] {
        for (int i = 0; i < 4; ++i) { (void)modellKompassOrt(3, i, (*orte)[static_cast<std::size_t>(i)]); }
    }));
    auto mit = std::make_shared<Fenster>();
    auto ohne = std::make_shared<Fenster>();
    add(fensterFoto("kompass_an", mit));
    add(tu("MBKOPF B: Kompass aus", [] { g_app->settings.modellKompass = false; }));
    add(pause(0.3));
    add(fensterFoto("kompass_aus", ohne));
    add(tu("MBKOPF B: Kompass wieder an", [] { g_app->settings.modellKompass = true; }));
    add(pruefe("MBKOPF B: alle vier Buchstaben N, O, S, W sind zu lesen - auch der, vor dem der ViewCube steht", [=] {
        if (!mit->da() || !ohne->da() || mit->w != ohne->w) { return false; }
        const Str namen[4] = {Str::MkKompassN, Str::MkKompassO, Str::MkKompassS, Str::MkKompassW};
        bool ok = true;
        std::string zeile;
        for (int i = 0; i < 4; ++i) {
            const ImVec2 c = (*orte)[static_cast<std::size_t>(i)];
            const ImVec2 ts = ImGui::CalcTextSize(tr(namen[i]));
            int anders = 0;
            int lo = 255;
            int hi = 0;
            for (int y = static_cast<int>(c.y - ts.y * 0.5F); y <= static_cast<int>(c.y + ts.y * 0.5F); ++y) {
                for (int x = static_cast<int>(c.x - ts.x * 0.5F); x <= static_cast<int>(c.x + ts.x * 0.5F); ++x) {
                    const unsigned char* a = mit->p(x, y);
                    const unsigned char* b = ohne->p(x, y);
                    anders += std::abs(a[0] - b[0]) + std::abs(a[1] - b[1]) + std::abs(a[2] - b[2]) > 60 ? 1 : 0;
                    lo = std::min(lo, hellBgra(a));
                    hi = std::max(hi, hellBgra(a));
                }
            }
            zeile += std::string(" ") + tr(namen[i]) + ": " + std::to_string(anders) + " Punkte, Kontrast " + std::to_string(hi - lo) + ";";
            ok = ok && anders >= 4 && hi - lo >= 80;
        }
        diag::detail("Knopftest: MBKOPF B Kompass -" + zeile);
        return ok;
    }));
    return s;
}

// Fotos der Zustaende, die rasterSchritte prueft (BHED_MODELL_NUR=rasterfotos, mit BHED_FOTOS) - zum Ansehen.
std::vector<Schritt> fotoSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    add(tu("MBRASTER Fotos: Skript, Max", [] {
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mbrf.txt");
        modellBedienungSetzen(0);
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        g_app->settings.modellHintergrund = -1;
        g_app->modellKnochen = false;
    }));
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    add(pause(0.4));
    struct Fall {
        const char* name;
        int blender;
        AnsichtArt art;
        float neigung;
        float gier;
        bool boden;
        int hintergrund;
        float zoom;
    };
    const Fall faelle[] = {
        {"mbr_max_start", 0, AnsichtArt::Perspektive, -1.0F, 0.0F, false, -1, 1.0F},
        {"mbr_max_schraeg", 0, AnsichtArt::Perspektive, 14.0F, 62.0F, false, -1, 1.0F},
        {"mbr_max_vorne", 0, AnsichtArt::Vorne, 0.0F, 0.0F, false, -1, 1.0F},
        {"mbr_max_vorne64", 0, AnsichtArt::Vorne, 0.0F, 0.0F, false, -1, 64.0F},
        {"mbr_max_oben_boden", 0, AnsichtArt::Oben, 0.0F, 0.0F, true, -1, 1.0F},
        {"mbr_max_boden", 0, AnsichtArt::Perspektive, 20.0F, 90.0F, true, -1, 1.0F},
        {"mbr_max_hell", 0, AnsichtArt::Perspektive, -1.0F, 0.0F, false, 0xB4B4B4, 1.0F},
        {"mbr_bl_start", 1, AnsichtArt::Perspektive, -1.0F, 0.0F, false, -1, 1.0F},
        {"mbr_bl_schraeg", 1, AnsichtArt::Perspektive, 14.0F, 62.0F, false, -1, 1.0F},
        {"mbr_bl_flach", 1, AnsichtArt::Perspektive, 8.0F, 90.0F, false, -1, 1.0F},
        {"mbr_bl_vorne", 1, AnsichtArt::Vorne, 0.0F, 0.0F, false, -1, 1.0F},
        {"mbr_bl_vorne16", 1, AnsichtArt::Vorne, 0.0F, 0.0F, false, -1, 1.0F / 16.0F},
        {"mbr_bl_ortho", 1, AnsichtArt::Orthographisch, 0.0F, 0.0F, false, -1, 1.0F},
    };
    for (const Fall& f : faelle) {
        add(tu(std::string("MBRASTER Fotos: ") + f.name, [f] {
            modellBedienungSetzen(f.blender);
            g_app->settings.modellHintergrund = f.hintergrund;
            t().an.boden = f.boden;
            einAnsicht(f.art);
            if (f.art == AnsichtArt::Perspektive && f.neigung >= 0.0F) {
                const float w[3] = {f.neigung, f.gier, 0.0F};
                modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
            }
            if (f.art == AnsichtArt::Orthographisch) {
                const float w[3] = {30.0F, 60.0F, 0.0F};
                modellBlickSetzen(3, AnsichtArt::Orthographisch, w, false);
            }
            ansicht(3).halbHoehe *= f.zoom;
            g_app->modelDirty = true;
        }));
        add(pause(0.3));
        add(mbkopfrot::fotoAblegen(f.name));
    }
    add(tu("MBRASTER Fotos: zurueck", [] {
        modellBedienungSetzen(0);
        g_app->settings.modellHintergrund = -1;
        t().an.boden = false;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    return s;
}

// Die Bilder des Handbuchs (BHED_EDITORTEST=handbuch, BHED_HB_NUR=mbraster): das Home Grid mit dem Menue [+] >
// Rastereinstellungen (Max), die vier Ansichten (Max), der Boden mit den farbigen Achsen und dem Menue Overlays (Blender).
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    auto teiler = std::make_shared<float>(0.0F);
    add(tu("MBRASTER Handbuch: breite Anordnung, Max, BOTH_STAND1, ohne Knochen", [=] {
        *teiler = g_app->settings.splitModel;
        g_app->settings.splitModel = 0.85F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        g_app->settings.modellNavigation = true;
        g_app->settings.modellHintergrund = -1;
        modellBedienungSetzen(0);
        animWaehlen(mb1::animNach("BOTH_STAND1") >= 0 ? mb1::animNach("BOTH_STAND1") : 0, false);
        g_app->modellKnochen = false;
        t().vierAnsichten = false;
        t().aktiv = 3;
    }));
    add(pause(0.5));
    add(tu("MBRASTER Handbuch: von vorne links oben", [] {
        modellStartansicht(3);
        const float w[3] = {22.0F, 66.0F, 0.0F};
        modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
        // etwas zurueck, damit das Raster ganz zu sehen ist
        g_app->modelOrbit.zoom(g_app->modelCam, -2.0F);
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(klickAn("MBRASTER Handbuch: [+]", mbkopf::feldOrt(3, 0)));
    add(pause(0.3));
    add(klick(tr(Str::MrRasterEinst)));
    add(pause(0.3));
    // sonst deckt der Tooltip von "Rastereinstellungen" die Zeilen des Untermenues zu: die Maus auf den freien Rand des
    // Untermenues rechts neben "Rasterabstand" (keine Beschriftung, kein Tooltip, das Untermenue bleibt offen)
    add(tu("MBRASTER Handbuch: Maus ins Untermenue", [] {
        const Element* e = finde(tr(Str::MrAbstand), "", 0, false, "");
        if (e == nullptr) {
            meldeFehler("MBRASTER Handbuch: Rasterabstand im Untermenue nicht gefunden");
            return;
        }
        setzeMaus(ImVec2{e->clip.Max.x - 4.0F, e->rect.GetCenter().y});
    }));
    add(pause(0.6));
    add(hilfe2::schrittFoto("h_mbr_max"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tu("MBRASTER Handbuch: vier Ansichten, der Rumpf gewaehlt (Auswahlklammer)", [] {
        t().vierAnsichten = true;
        t().aktiv = 3;
        t().auswahl = {mb1::teil("torso")};
        // die ganze Figur einpassen, nicht die Auswahl (sonst fuellt der Rumpf die Orthoansichten und verdeckt das Raster)
        for (int i = 0; i < 3; ++i) { modellAllesEinpassenIn(i); }
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(tu("MBRASTER Handbuch: Maus weg", [] {
        const ModellAnsicht& a = ansicht(3);
        setzeMaus(ImVec2{a.lo[0] + 20.0F, a.lo[1] + a.groesse[1] - 20.0F});
    }));
    add(pause(0.4));
    add(hilfe2::schrittFoto("h_mbr_vier"));
    add(tu("MBRASTER Handbuch: Blender, flach von vorne links", [] {
        t().auswahl.clear();
        modellBedienungSetzen(1);
        t().vierAnsichten = false;
        t().aktiv = 3;
        modellStartansicht(3);
        const float w[3] = {12.0F, 64.0F, 0.0F};
        modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
        g_app->modelOrbit.zoom(g_app->modelCam, -2.0F);
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(klickAn("MBRASTER Handbuch: Overlays", mbkopf::feldOrt(3, 3)));
    add(pause(0.6));
    add(hilfe2::schrittFoto("h_mbr_blender"));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(tu("MBRASTER Handbuch: zurueck", [=] {
        g_app->settings.splitModel = *teiler;
        modellBedienungSetzen(0);
        modellStartansicht(3);
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    if (n == "raster") { return rasterSchritte(); }
    if (n == "rasterfotos") { return fotoSchritte(); }
    if (n == "kopf") { return kopfZusatzSchritte(); }
    if (n == "kopfab") {   // nur A und B (mit eigener Figur) - fuer den Rot-Lauf und zum Nachpruefen
        std::vector<Schritt> s;
        s.push_back(tu("MBKOPF AB: Skript, Max-Modus", [] {
            openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mbkab.txt");
            modellBedienungSetzen(0);
            g_app->settings.modellAnzeigeUmzugGesehen = true;
        }));
        for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { s.push_back(std::move(x)); }
        s.push_back(pause(0.4));
        for (Schritt& x : kopfZusatzSchritte()) { s.push_back(std::move(x)); }
        return s;
    }
    return {};
}

}  // namespace mbraster
