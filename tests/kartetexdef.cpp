// kartetexdef.cpp - Karten-Editor Stufe 1a, Punkt 2b: Texturprojektion
// (bhed/karte/texdef.h, Flaechen-Operationen in pinsel.h).
//
// Geprueft werden die Formeln aus radiant/brush_primit.cpp:
//   - Quake3-Texdef (shift/scale/rotate) -> st je Wicklungspunkt, analytisch
//     nachgerechnet (Texdef_toTransform x Normal_GetTransform),
//   - Brush Primitives -> st ueber ComputeAxisBase,
//   - Umrechnung AP <-> BP <-> Valve 220 (st bleibt an jedem Punkt),
//   - Texture Lock beim Verschieben/Drehen (st wandert mit),
//   - Fit, Shift/Scale/Rotate, Standardprojektion.
// Toleranzen: st ist float; NRC rechnet die 4x4-Matrix in float, daher
// relative Abweichungen bis etwa 1e-6 je Rechenschritt.
#include "bhed/karte/pinsel.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace bhed::karte;

namespace {

int g_fehler = 0;
void pruefe(bool ok, const std::string& was) {
    if (!ok) {
        ++g_fehler;
        std::printf("FEHL  %s\n", was.c_str());
    }
}

Vektor3d v(double x, double y, double z) { return Vektor3d(x, y, z); }

std::shared_ptr<Pinsel> quader(TexdefArt art, const TexturProjektion& p) {
    auto b = std::make_shared<Pinsel>();
    const EbenenPunkte ebenen[6] = {
        {v(64, 32, 128), v(64, -32, 128), v(-64, 32, 128)},
        {v(64, 32, 128), v(-64, 32, 128), v(64, 32, 0)},
        {v(64, 32, 128), v(64, 32, 0), v(64, -32, 128)},
        {v(-64, -32, 0), v(64, -32, 0), v(-64, 32, 0)},
        {v(-64, -32, 0), v(-64, -32, 128), v(64, -32, 0)},
        {v(-64, -32, 0), v(-64, 32, 0), v(-64, -32, 128)},
    };
    for (const auto& e : ebenen) {
        Flaeche& f = b->neueFlaecheLaden();
        f.ladeEbenenPunkte(e);
        f.ladeShader("textures/test/wand");
        f.projektionRoh() = p;
        f.skalierungAngewandt = true;
        f.projektionInitialisiert = art == TexdefArt::BrushPrimitives;
    }
    return b;
}

// st eines Weltpunkts p auf Flaeche f (ueber lokalZuTextur, wie
// Texdef_EmitTextureCoordinates).
Vektor2f stVon(const Flaeche& f, const Kontext& k, const Vektor3d& p) {
    Matrix4 m;
    const auto [w, h] = k.groesse(f.shader());
    lokalZuTextur(k.art(), f.projektion(), w, h, f.ebene().normal(), m);
    const Vektor3d st = transformierterPunkt(m, p);
    return Vektor2f(static_cast<float>(st.x()), static_cast<float>(st.y()));
}

bool nahe(double a, double b, double eps) { return std::fabs(a - b) <= eps; }

class Groessen final : public TexturGroessen {
public:
    std::pair<std::size_t, std::size_t> groesse(const std::string& s) const override {
        if (s == "textures/test/breit") { return {256, 64}; }
        if (s == "textures/test/npot") { return {96, 48}; }
        return {128, 128};
    }
};

}  // namespace

int main() {
    Groessen groessen;

    // --- Quake3 (AP): st analytisch ----------------------------------------
    {
        Kontext k;
        k.format = PinselFormat::Quake3;
        k.texturen = &groessen;
        TexturProjektion p;
        p.texdef.scale[0] = 0.5f;
        p.texdef.scale[1] = 0.25f;
        p.texdef.shift[0] = 16;
        p.texdef.shift[1] = 8;
        auto b = quader(k.art(), p);
        b->bRep(k);
        // Oberseite (Normale +z, Achse Z): s = x/(0.5*128) + 16/128, t = -y/(0.25*128) + 8/128
        bool ok = true;
        const Wicklung& w = (*b)[0].wicklung();
        for (std::size_t i = 0; i < w.numpoints; ++i) {
            const double x = w[i].vertex.x();
            const double y = w[i].vertex.y();
            ok = ok && nahe(w[i].texcoord.x(), x / 64.0 + 0.125, 1e-6) && nahe(w[i].texcoord.y(), -y / 32.0 + 0.0625, 1e-6);
        }
        pruefe(ok, "AP Oberseite: s = x/(scale*w) + shift/w, t = -y/(scale*h) + shift/h");
        // +x-Seite (Achse X: Normal_GetTransform s<-y, t<-z): s = y/64 + 1/8, t = -z/32 + 1/16
        ok = true;
        const Wicklung& wx = (*b)[2].wicklung();
        for (std::size_t i = 0; i < wx.numpoints; ++i) {
            const double y = wx[i].vertex.y();
            const double z = wx[i].vertex.z();
            ok = ok && nahe(wx[i].texcoord.x(), y / 64.0 + 0.125, 1e-6) && nahe(wx[i].texcoord.y(), -z / 32.0 + 0.0625, 1e-6);
        }
        pruefe(ok, "AP +x-Seite: Projektion auf y/z");
        // +y-Seite (Achse Y: s<-x, t<-z)
        ok = true;
        const Wicklung& wy = (*b)[1].wicklung();
        for (std::size_t i = 0; i < wy.numpoints; ++i) {
            ok = ok && nahe(wy[i].texcoord.x(), wy[i].vertex.x() / 64.0 + 0.125, 1e-6)
              && nahe(wy[i].texcoord.y(), -wy[i].vertex.z() / 32.0 + 0.0625, 1e-6);
        }
        pruefe(ok, "AP +y-Seite: Projektion auf x/z");
    }
    // --- AP mit Drehung 90 Grad --------------------------------------------
    {
        Kontext k;
        k.texturen = &groessen;
        TexturProjektion p;
        p.texdef.scale[0] = 1;
        p.texdef.scale[1] = 1;
        p.texdef.rotate = 90;
        auto b = quader(k.art(), p);
        b->bRep(k);
        // m0 = cos(-90)/128, m1 = sin(-90)/-128 = 1/128, m4 = 1/128, m5 = cos/-128
        bool ok = true;
        const Wicklung& w = (*b)[0].wicklung();
        for (std::size_t i = 0; i < w.numpoints; ++i) {
            ok = ok && nahe(w[i].texcoord.x(), w[i].vertex.y() / 128.0, 1e-6)
              && nahe(w[i].texcoord.y(), w[i].vertex.x() / 128.0, 1e-6);
        }
        pruefe(ok, "AP rotate 90: s = y/w, t = x/h");
    }

    // --- Brush Primitives: st = coords * (p.texS, p.texT, 1) ---------------
    {
        Kontext k;
        k.format = PinselFormat::Quake3BP;
        k.texturen = &groessen;
        TexturProjektion p;
        p.bp.coords[0][0] = 0.015625f;
        p.bp.coords[0][1] = 0.0078125f;
        p.bp.coords[0][2] = 0.25f;
        p.bp.coords[1][0] = -0.00390625f;
        p.bp.coords[1][1] = 0.015625f;
        p.bp.coords[1][2] = 0.5f;
        auto b = quader(k.art(), p);
        b->bRep(k);
        // Oberseite: ComputeAxisBase((0,0,1)) -> texS (0,1,0), texT (1,0,0)
        bool ok = true;
        const Wicklung& w = (*b)[0].wicklung();
        for (std::size_t i = 0; i < w.numpoints; ++i) {
            const double xs = w[i].vertex.y();
            const double ys = w[i].vertex.x();
            ok = ok && nahe(w[i].texcoord.x(), 0.015625 * xs + 0.0078125 * ys + 0.25, 1e-6)
              && nahe(w[i].texcoord.y(), -0.00390625 * xs + 0.015625 * ys + 0.5, 1e-6);
        }
        pruefe(ok, "BP Oberseite: Achsenbasis (0,1,0)/(1,0,0)");
        // +x-Seite: texS = -normalise(n x up) = -(0,-1,0) = (0,1,0), texT = normalise(n x texS_vor) = (0,0,-1)
        Vektor3f s, t;
        berechneAchsenbasis(Vektor3f(1, 0, 0), s, t);
        pruefe(s == Vektor3f(0, 1, 0) && t == Vektor3f(0, 0, -1), "ComputeAxisBase(+x) = (0,1,0), (0,0,-1)");
        berechneAchsenbasis(Vektor3f(0, 0, -1), s, t);
        pruefe(s == Vektor3f(0, 1, 0) && t == Vektor3f(-1, 0, 0), "ComputeAxisBase(-z) = (0,1,0), (-1,0,0)");
    }

    // --- Umrechnung AP -> BP -> AP und -> Valve: st bleibt -----------------
    {
        Kontext ap;
        ap.format = PinselFormat::Quake3;
        ap.texturen = &groessen;
        Kontext bp = ap;
        bp.format = PinselFormat::Quake3BP;
        Kontext va = ap;
        va.format = PinselFormat::Quake3Valve220;
        TexturProjektion p;
        p.texdef.scale[0] = 0.5f;
        p.texdef.scale[1] = 0.75f;
        p.texdef.shift[0] = 13;
        p.texdef.shift[1] = -7;
        p.texdef.rotate = 30;
        auto b = quader(TexdefArt::Quake, p);
        (*b)[0].ladeShader("textures/test/breit");
        (*b)[2].ladeShader("textures/test/npot");
        b->bRep(ap);
        std::vector<std::vector<Vektor2f>> vorher(b->anzahl());
        for (std::size_t i = 0; i < b->anzahl(); ++i) {
            for (const auto& q : (*b)[i].wicklung()) { vorher[i].push_back(q.texcoord); }
        }
        auto vergleiche = [&](const Kontext& k, const char* was, double eps) {
            b->geaendert();
            b->bRep(k);
            bool ok = true;
            double groesste = 0;
            for (std::size_t i = 0; i < b->anzahl(); ++i) {
                const Wicklung& w = (*b)[i].wicklung();
                for (std::size_t j = 0; j < w.numpoints; ++j) {
                    // st ist bis auf ganze Wiederholungen gleich (Normalisierung der Verschiebung)
                    for (int c = 0; c < 2; ++c) {
                        double d = static_cast<double>(w[j].texcoord[c]) - vorher[i][j][c];
                        d -= std::round(d);
                        groesste = std::max(groesste, std::fabs(d));
                    }
                }
            }
            ok = groesste <= eps;
            if (!ok) { std::printf("      groesste st-Abweichung %.3g\n", groesste); }
            pruefe(ok, was);
        };
        for (std::size_t i = 0; i < b->anzahl(); ++i) { (*b)[i].rechneTexdefUm(TexdefArt::Quake, TexdefArt::BrushPrimitives, bp); }
        vergleiche(bp, "Umrechnung AP -> BP: st bleibt (bis 1e-5)", 1e-5);
        for (std::size_t i = 0; i < b->anzahl(); ++i) { (*b)[i].rechneTexdefUm(TexdefArt::BrushPrimitives, TexdefArt::Quake, ap); }
        vergleiche(ap, "Umrechnung BP -> AP: st bleibt (bis 1e-4)", 1e-4);
        for (std::size_t i = 0; i < b->anzahl(); ++i) { (*b)[i].rechneTexdefUm(TexdefArt::Quake, TexdefArt::Valve, va); }
        vergleiche(va, "Umrechnung AP -> Valve 220: st bleibt (bis 1e-4)", 1e-4);
    }

    // --- Texture Lock: Verschieben und Drehen ------------------------------
    for (const PinselFormat format : {PinselFormat::Quake3, PinselFormat::Quake3BP}) {
        Kontext k;
        k.format = format;
        k.texturen = &groessen;
        k.texturLock = true;
        TexturProjektion p;
        p.texdef.scale[0] = 0.5f;
        p.texdef.scale[1] = 0.5f;
        p.texdef.rotate = 15;
        p.bp.coords[0][0] = 0.01f;
        p.bp.coords[0][1] = 0.003f;
        p.bp.coords[1][0] = -0.002f;
        p.bp.coords[1][1] = 0.012f;
        p.bp.coords[0][2] = 0.3f;
        auto b = quader(k.art(), p);
        b->bRep(k);
        const Vektor3d punkt0 = (*b)[0].wicklung()[0].vertex;
        const Vektor2f st0 = stVon((*b)[0], k, punkt0);
        const Vektor3f weg(40, -24, 8);
        b->transformieren(verschiebungsMatrix(weg), k);
        const Vektor2f st1 = stVon((*b)[0], k, punkt0 + Vektor3d(weg));
        double d0 = static_cast<double>(st1.x()) - st0.x();
        double d1 = static_cast<double>(st1.y()) - st0.y();
        d0 -= std::round(d0);
        d1 -= std::round(d1);
        pruefe(std::fabs(d0) < 1e-4 && std::fabs(d1) < 1e-4,
               std::string("Texture Lock Verschieben (") + (format == PinselFormat::Quake3 ? "AP" : "BP") + "): st wandert mit");
        // Drehung 90 Grad um z um den Ursprung
        const Vektor2f stA = stVon((*b)[0], k, (*b)[0].wicklung()[1].vertex);
        const Vektor3d pa = (*b)[0].wicklung()[1].vertex;
        b->transformieren(drehungZGrad(90), k);
        const Vektor3d pb(-pa.y(), pa.x(), pa.z());
        const Vektor2f stB = stVon((*b)[0], k, pb);
        d0 = static_cast<double>(stB.x()) - stA.x();
        d1 = static_cast<double>(stB.y()) - stA.y();
        d0 -= std::round(d0);
        d1 -= std::round(d1);
        pruefe(std::fabs(d0) < 1e-3 && std::fabs(d1) < 1e-3,
               std::string("Texture Lock Drehen 90 (") + (format == PinselFormat::Quake3 ? "AP" : "BP") + "): st dreht mit");
    }
    // AP: ohne Lock wandert die Textur nicht mit
    {
        Kontext k;
        k.texturen = &groessen;
        TexturProjektion p;
        p.texdef.scale[0] = 0.5f;
        p.texdef.scale[1] = 0.5f;
        auto b = quader(k.art(), p);
        b->transformieren(verschiebungsMatrix(Vektor3f(8, 0, 0)), k);
        pruefe((*b)[0].projektion().texdef.shift[0] == 0, "AP ohne Lock: Verschiebung bleibt 0");
    }

    // --- Fit 1x1: st der Flaeche genau 0..1 --------------------------------
    for (const PinselFormat format : {PinselFormat::Quake3, PinselFormat::Quake3BP}) {
        Kontext k;
        k.format = format;
        k.texturen = &groessen;
        TexturProjektion p;
        p.texdef.scale[0] = 0.3f;
        p.texdef.scale[1] = 0.7f;
        p.texdef.rotate = 0;
        auto b = quader(k.art(), p);
        b->bRep(k);
        (*b)[2].passeTexturEin(1, 1, false, k);
        b->bRep(k);
        float smin = 1e9F, smax = -1e9F, tmin = 1e9F, tmax = -1e9F;
        for (const auto& q : (*b)[2].wicklung()) {
            smin = std::min(smin, q.texcoord.x());
            smax = std::max(smax, q.texcoord.x());
            tmin = std::min(tmin, q.texcoord.y());
            tmax = std::max(tmax, q.texcoord.y());
        }
        const double ds = smax - smin;
        const double dt = tmax - tmin;
        pruefe(nahe(ds, 1, 1e-5) && nahe(dt, 1, 1e-5) && nahe(smin - std::floor(smin + 1e-6), 0, 1e-5),
               std::string("Fit 1x1 (") + (format == PinselFormat::Quake3 ? "AP" : "BP") + "): st-Bereich genau 1x1");
    }

    // --- Shift/Scale/Rotate (AP) und Fake-Texcoords (BP) -------------------
    {
        Kontext k;
        k.texturen = &groessen;
        TexturProjektion p;
        auto b = quader(k.art(), p);
        (*b)[0].verschiebeTextur(8, 4, k);
        (*b)[0].skaliereTextur(0.5f, -1.0f, k);   // scale[1] + t == 0 -> umgedreht
        (*b)[0].dreheTextur(400, k);
        const Texdef& t = (*b)[0].projektion().texdef;
        pruefe(t.shift[0] == 8 && t.shift[1] == 4 && t.scale[0] == 1.5f && t.scale[1] == -1.0f && t.rotate == 40.0f,
               "AP Shift/Scale/Rotate (Texdef_Scale dreht bei 0 das Vorzeichen, rotate mod 360)");
        Texdef td;
        // gleiche Skalierung: TexMatToFakeTexCoords (:656-657) misst die ZEILEN
        // (coords[0][0], coords[0][1]) - bei ungleicher Skalierung ist der
        // Rundweg in NRC selbst nicht exakt.
        td.scale[0] = 0.5f;
        td.scale[1] = 0.5f;
        td.rotate = 30;
        td.shift[0] = 3;
        td.shift[1] = 5;
        BpTexdef bpt;
        schiebSkalDrehZuBp(td, bpt);
        Texdef zurueck;
        bpZuSchiebSkalDreh(bpt, zurueck);
        // Drehsinn: FakeTexCoordsToTexMat setzt coords[0][1] = y * -sin(-rot) > 0,
        // TexMatToFakeTexCoords verneint bei coords[0][1] > 0 -> -30. So steht es in
        // NRC (brush_primit.cpp:665-668 und :679-687); BPTexdef_Rotate (:1094-1103)
        // hat den anderen Drehsinn und passt zu TexMatToFakeTexCoords.
        pruefe(nahe(zurueck.scale[0], 0.5, 1e-6) && nahe(zurueck.scale[1], 0.5, 1e-6) && nahe(zurueck.rotate, -30, 1e-4)
                   && zurueck.shift[0] == 3 && zurueck.shift[1] == 5,
               "BP FakeTexCoordsToTexMat -> TexMatToFakeTexCoords (Drehsinn wie NRC)");
        BpTexdef gedreht;   // Einheitsmatrix 2,0,0 / 0,2,0
        float dreh = 30;
        bpZuweisen(gedreht, nullptr, nullptr, nullptr, nullptr, &dreh);
        Texdef anzeige;
        bpZuSchiebSkalDreh(gedreht, anzeige);
        pruefe(nahe(anzeige.rotate, 30, 1e-4), "BPTexdef_Assign(rotation 30) zeigt 30 (BPTexdef_Rotate)");
    }

    // --- Standardprojektion: default_scale 0.25 (ja.game / jk2.game) -------
    {
        setzeStandardTexturSkalierung(0.25f);
        TexturProjektion p;
        texdefStandard(TexdefArt::BrushPrimitives, p);
        pruefe(p.texdef.scale[0] == 0.25f && p.bp.coords[0][0] == 4.0f && p.bp.coords[1][1] == 4.0f,
               "TexDef_Construct_Default mit 0.25: BP-Matrix 4");
        setzeStandardTexturSkalierung(0.5f);
    }

    // --- Shaderwechsel rechnet die BP-Matrix auf die neue Groesse um -------
    {
        Kontext k;
        k.format = PinselFormat::Quake3BP;
        k.texturen = &groessen;
        TexturProjektion p;   // coords 2,0,0 / 0,2,0 in Texturgroessen-Masstab
        auto b = std::make_shared<Pinsel>();
        b->quaderBauen(Kasten(Vektor3f(0, 0, 0), Vektor3f(16, 16, 16)), "textures/test/wand", p, k);
        const float vorher = (*b)[0].projektion().bp.coords[0][0];
        (*b)[0].setzeShader("textures/test/breit", k);
        pruefe(vorher == 2.0f / 128 && (*b)[0].projektion().bp.coords[0][0] == 2.0f / 256
                   && (*b)[0].projektion().bp.coords[1][1] == 2.0f / 64,
               "SetShader: BP-Matrix von 128x128 auf 256x64 (unrealise/realise)");
    }

    std::printf("kartetexdef: %d Fehlschlaege\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
