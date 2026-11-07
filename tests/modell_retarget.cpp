// modell_retarget.cpp - Modell-Block Stufe 5: Retargeting (bhed/modell/retarget.h)
//
//   Vorlagen       JKA/JK2 _humanoid (Hand-Tabelle der Knochen-Erkennung), HumanIK/Mixamo, Biped und CAT (Namen nach Quelle,
//                  Lage aus dem JKA-Skelett) werden erkannt; Ketten der Vorlage; Kreaturen (Bantha, Droideka) bekommen
//                  vorgeschlagene Ketten
//   Zuordnung      Ketten ueber ihren Schluessel, von Hand bleibt, nur leere gefuellt; Knochen JK2 -> JKA (Finger ueber die
//                  Ziffer, Drehknochen ueber den Namen)
//   Gleiche Figur  Quelle = Ziel: die Welt jedes Bilds wie die Quelle (Abweichung unter fester Grenze), gebacken unter der
//                  Grenze der Kompression
//   Bindepose      Quelle mit T-Bindepose, Ziel mit der A-Bindepose des Spiels: ohne Ausrichten verdrehte Arme (rot, Grad),
//                  ausgerichtet gleich
//   Laenge         Ziel mit 1,3-fachen Beinen, Gang der Quelle ohne Rutschen: ohne Laengenausgleich rutscht der Standfuss
//                  (rot, Einheiten), mit Becken-Skalierung nicht; laengere Unterschenkel allein: FK rutscht, IK + Kontakte nicht
//   Reach/Reihen-  Reach 0 = FK, 0,5 dazwischen; IK vor dem Becken laesst die Fuesse vom Ziel weg (umstellbar wirkt)
//   folge, Griff   der Tag rhang_tag_bone bleibt an der Hand (JK2 -> JKA)
//   Spruenge       ein umgedrehtes Bild im Ziel wird gefunden, gleiche Folge nicht
//
// Aufruf: modellretargettest  |  modellretargettest --vorschlag <ausgabe.txt> <datei.gla> <name> (Vorlagen erzeugen)
#include "bhed/gla.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/ik.h"
#include "bhed/modell/retarget.h"
#include "bhed/modell/rig.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>
#include <vector>

#include "rig_handtabelle.inc"

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", what.c_str());
    if (!ok) { ++fails; }
}
std::string z(double v) {
    char b[40];
    std::snprintf(b, sizeof b, "%.4f", v);
    return b;
}

using namespace bhed;
using namespace bhed::modell;
using V3 = std::array<float, 3>;
std::size_t uz(int i) { return static_cast<std::size_t>(i); }

const RigHandSkelett& tabelle(const char* name) {
    for (const RigHandSkelett& s : kRigHandSkelette) {
        if (std::strcmp(s.name, name) == 0) { return s; }
    }
    return kRigHandSkelette[0];
}

// Ein Skelett aus Namen, Eltern und Orten (Grundstellung ohne Drehung), ohne Bilder
GlaAnimation skelett(const std::vector<std::string>& namen, const std::vector<int>& eltern, const std::vector<V3>& orte, const char* datei) {
    GlaAnimation g;
    g.name = datei;
    // wie im Kopf der GLA: der Pfad ohne Endung
    g.skeletonName = std::string(datei).substr(0, std::string(datei).size() > 4 && std::string(datei).rfind(".gla") == std::string(datei).size() - 4
                                                      ? std::string(datei).size() - 4
                                                      : std::string(datei).size());
    g.scale = 0.64F;
    for (std::size_t i = 0; i < namen.size(); ++i) {
        GlaBone b;
        b.name = namen[i];
        b.parent = eltern[i];
        b.basePose = BoneMatrix::identity();
        b.basePoseInv = BoneMatrix::identity();
        for (int k = 0; k < 3; ++k) {
            b.basePose.m[k][3] = orte[i][uz(k)];
            b.basePoseInv.m[k][3] = -orte[i][uz(k)];
        }
        g.bones.push_back(b);
    }
    for (std::size_t i = 0; i < g.bones.size(); ++i) {
        if (g.bones[i].parent >= 0) { g.bones[uz(g.bones[i].parent)].kinder.push_back(static_cast<int>(i)); }
    }
    return g;
}

GlaAnimation ausTabelle(const char* name, const std::function<V3(const std::string&, V3)>& ortAendern = {}) {
    const RigHandSkelett& h = tabelle(name);
    std::vector<std::string> n;
    std::vector<int> e;
    std::vector<V3> o;
    for (int i = 0; i < h.zahl; ++i) {
        n.emplace_back(h.knochen[i].name);
        e.push_back(h.knochen[i].eltern);
        V3 p{h.knochen[i].x, h.knochen[i].y, h.knochen[i].z};
        if (ortAendern) { p = ortAendern(n.back(), p); }
        o.push_back(p);
    }
    return skelett(n, e, o, h.gla);
}

V3 ort(const BoneMatrix& m) { return {m.m[0][3], m.m[1][3], m.m[2][3]}; }
V3 sub(const V3& a, const V3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
float laenge(const V3& a) { return std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]); }
float winkel(const V3& a, const V3& b) {
    const float c = (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) / std::max(laenge(a) * laenge(b), 1e-9F);
    return std::acos(std::clamp(c, -1.0F, 1.0F)) * 57.29578F;
}

std::vector<BoneMatrix> bindeWelt(const GlaAnimation& g) {
    std::vector<BoneMatrix> w;
    for (const GlaBone& b : g.bones) { w.push_back(b.basePose); }
    return w;
}

// Den Teilbaum (nach Eltern) von b um die Weltachse drehen (um den Ursprung von b)
void drehTeilbaum(const GlaAnimation& g, std::vector<BoneMatrix>& w, int b, const float achse[3], float grad) {
    const BoneMatrix r = drehungUm(achse, grad);
    const V3 o = ort(w[uz(b)]);
    std::vector<int> v{b};
    for (std::size_t i = 0; i < v.size(); ++i) {
        for (const int k : g.bones[uz(v[i])].kinder) { v.push_back(k); }
    }
    for (const int d : v) {
        BoneMatrix x = w[uz(d)];
        const V3 p = sub(ort(x), o);
        BoneMatrix rr = r;
        rr.m[0][3] = rr.m[1][3] = rr.m[2][3] = 0.0F;
        BoneMatrix xr = x;
        xr.m[0][3] = xr.m[1][3] = xr.m[2][3] = 0.0F;
        x = rr * xr;
        x.m[0][3] = o[0] + rr.m[0][0] * p[0] + rr.m[0][1] * p[1] + rr.m[0][2] * p[2];
        x.m[1][3] = o[1] + rr.m[1][0] * p[0] + rr.m[1][1] * p[1] + rr.m[1][2] * p[2];
        x.m[2][3] = o[2] + rr.m[2][0] * p[0] + rr.m[2][1] * p[1] + rr.m[2][2] * p[2];
        w[uz(d)] = x;
    }
}
void schiebeTeilbaum(const GlaAnimation& g, std::vector<BoneMatrix>& w, int b, const V3& d) {
    std::vector<int> v{b};
    for (std::size_t i = 0; i < v.size(); ++i) {
        for (const int k : g.bones[uz(v[i])].kinder) { v.push_back(k); }
    }
    for (const int x : v) {
        for (int k = 0; k < 3; ++k) { w[uz(x)].m[k][3] += d[uz(k)]; }
    }
}

// Eine GLA mit diesen Welten als Bildern
GlaAnimation mitBildern(const GlaAnimation& g, const std::vector<std::vector<BoneMatrix>>& welten) {
    GlaAnimation a;
    std::string f;
    const bool ok = rtBacken(g, welten, g.skeletonName, a, &f);
    if (!ok) { std::printf("  backen: %s\n", f.c_str()); }
    return a;
}

int kn(const GlaAnimation& g, const char* n) { return boneIndex(g, n); }

int g_schlimmster = -1;   // Knochen der groessten Abweichung (Bericht)
float groessteAbweichung(const std::vector<std::vector<BoneMatrix>>& a, const std::vector<std::vector<BoneMatrix>>& b, float* grad = nullptr) {
    float d = 0.0F;
    float w = 0.0F;
    g_schlimmster = -1;
    for (std::size_t f = 0; f < a.size() && f < b.size(); ++f) {
        for (std::size_t k = 0; k < a[f].size() && k < b[f].size(); ++k) {
            const float dk = laenge(sub(ort(a[f][k]), ort(b[f][k])));
            float wk = 0.0F;
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) { wk = std::max(wk, std::fabs(a[f][k].m[i][j] - b[f][k].m[i][j])); }
            }
            if (dk > d || wk > w) { g_schlimmster = static_cast<int>(k); }
            d = std::max(d, dk);
            w = std::max(w, wk);
        }
    }
    if (grad != nullptr) { *grad = std::asin(std::min(w, 1.0F)) * 57.29578F; }
    return d;
}

std::vector<std::vector<BoneMatrix>> welten(const GlaAnimation& g) {
    std::vector<std::vector<BoneMatrix>> v(uz(g.numFrames));
    for (int f = 0; f < g.numFrames; ++f) { g.worldMatrices(f, v[uz(f)]); }
    return v;
}

// Ein Gang ohne Rutschen (Kompassgang): das Standbein gestreckt, das Becken auf dem Bogen ueber dem festen Fuss; das
// Schwungbein gegengleich mit gebeugtem Knie. Vorn ist -Y wie im Skelett der Spiele. Bilder je Schritt, Schritte.
std::vector<std::vector<BoneMatrix>> gang(const GlaAnimation& g, int jeSchritt, int schritte, float alphaGrad, std::vector<int>& standL) {
    const int becken = kn(g, "pelvis");
    const int lf = kn(g, "lfemurYZ");
    const int rf = kn(g, "rfemurYZ");
    const int lt = kn(g, "ltibia");
    const int rt = kn(g, "rtibia");
    const int la = kn(g, "ltalus");
    const int ra = kn(g, "rtalus");
    const std::vector<BoneMatrix> b = bindeWelt(g);
    const float achse[3] = {1.0F, 0.0F, 0.0F};   // um die Seitenachse; +Winkel dreht den Fuss nach +Y
    // das Bein der Grundstellung steht schraeg (Knoechel 2,04 hinter der Huefte): die Winkel gelten um das senkrechte Bein, damit
    // beide Fuesse im Doppelstand gleich hoch sind
    const V3 bh = sub(ort(b[uz(la)]), ort(b[uz(lf)]));
    const float neigung = std::atan2(bh[1], -bh[2]) * 57.29578F;
    // Die Fusslage am Ende eines Schritts (u = 1): Standbein -alpha, Schwungbein +alpha, beide gestreckt; das Becken so, dass der
    // Standfuss am Ort `stand` liegt
    const auto ende = [&](bool linksSteht, const V3& stand, std::vector<BoneMatrix>& e) {
        e = b;
        drehTeilbaum(g, e, linksSteht ? lf : rf, achse, -alphaGrad - neigung);
        drehTeilbaum(g, e, linksSteht ? rf : lf, achse, alphaGrad - neigung);
        schiebeTeilbaum(g, e, becken, sub(stand, ort(e[uz(linksSteht ? la : ra)])));
    };
    std::vector<std::vector<BoneMatrix>> aus;
    standL.clear();
    V3 standOrt = ort(b[uz(la)]);   // der Standfuss des ersten Schritts (links)
    V3 schwungAb{};                 // wo der Schwungfuss abhebt
    {
        // Bild 0: Doppelstand (Standbein +alpha, Schwungbein -alpha)
        std::vector<BoneMatrix> w = b;
        drehTeilbaum(g, w, lf, achse, alphaGrad - neigung);
        drehTeilbaum(g, w, rf, achse, -alphaGrad - neigung);
        schiebeTeilbaum(g, w, becken, sub(standOrt, ort(w[uz(la)])));
        schwungAb = ort(w[uz(ra)]);
    }
    for (int s = 0; s < schritte; ++s) {
        const bool linksSteht = s % 2 == 0;
        const int sf = linksSteht ? lf : rf;
        const int wf = linksSteht ? rf : lf;
        const int wt = linksSteht ? rt : lt;
        const int sa = linksSteht ? la : ra;
        const int wa = linksSteht ? ra : la;
        std::vector<BoneMatrix> e;
        ende(linksSteht, standOrt, e);
        const V3 landen = ort(e[uz(wa)]);
        for (int i = 0; i < jeSchritt; ++i) {
            const float u = static_cast<float>(i) / static_cast<float>(jeSchritt);
            const float th = alphaGrad * (1.0F - 2.0F * u);   // Standbein von +alpha nach -alpha
            std::vector<BoneMatrix> w = b;
            drehTeilbaum(g, w, sf, achse, th - neigung);
            drehTeilbaum(g, w, linksSteht ? lt : rt, achse, -20.0F * std::sin(3.1415927F * u));   // Standknie gebeugt (an den Enden gestreckt)
            drehTeilbaum(g, w, wf, achse, -th - neigung);
            // Becken so, dass der Standfuss an seinem Ort bleibt
            schiebeTeilbaum(g, w, becken, sub(standOrt, ort(w[uz(sa)])));
            // Schwungfuss: von der Abhebe- zur Landestelle, 4 Einheiten Hub (Zwei-Glied-IK aus Stufe 4, Knie nach vorn -Y)
            const V3 ziel{schwungAb[0] + (landen[0] - schwungAb[0]) * u, schwungAb[1] + (landen[1] - schwungAb[1]) * u,
                          schwungAb[2] + (landen[2] - schwungAb[2]) * u + 4.0F * std::sin(3.1415927F * u)};
            const V3 pa = ort(w[uz(wf)]);
            const V3 pb = ort(w[uz(wt)]);
            const V3 pc = ort(w[uz(wa)]);
            const float fa[3] = {pa[0], pa[1], pa[2]};
            const float fb[3] = {pb[0], pb[1], pb[2]};
            const float fc[3] = {pc[0], pc[1], pc[2]};
            const float fz[3] = {ziel[0], ziel[1], ziel[2]};
            const float fp[3] = {pb[0], pb[1] - 30.0F, pb[2]};
            float bn[3];
            float cn[3];
            (void)zweiGliedIk(fa, fb, fc, fz, fp, bn, cn);
            // Oberschenkel um die Huefte auf das neue Knie, Unterschenkel um das Knie auf den neuen Fuss (Drehachse senkrecht zu beiden)
            const auto dreh = [&](int knochen, const V3& um, const V3& von, const V3& nach) {
                const V3 x = sub(von, um);
                const V3 y = sub(nach, um);
                float ax[3] = {x[1] * y[2] - x[2] * y[1], x[2] * y[0] - x[0] * y[2], x[0] * y[1] - x[1] * y[0]};
                const float l = std::sqrt(ax[0] * ax[0] + ax[1] * ax[1] + ax[2] * ax[2]);
                if (l < 1e-6F) { return; }
                for (float& v : ax) { v /= l; }
                drehTeilbaum(g, w, knochen, ax, winkel(x, y));
            };
            dreh(wf, pa, pb, V3{bn[0], bn[1], bn[2]});
            dreh(wt, ort(w[uz(wt)]), ort(w[uz(wa)]), V3{cn[0], cn[1], cn[2]});
            standL.push_back(linksSteht ? 1 : 0);
            aus.push_back(std::move(w));
        }
        // naechster Schritt: der Schwungfuss steht, der alte Standfuss hebt ab
        schwungAb = standOrt;
        standOrt = landen;
    }
    return aus;
}

// Waagrechter Weg eines Fusses ueber die Bilder, in denen er steht (bekannt aus dem Gang)
float standRutschen(const std::vector<std::vector<BoneMatrix>>& w, int fuss, const std::vector<int>& steht, int sollWert) {
    float d = 0.0F;
    V3 start{};
    bool drin = false;
    for (std::size_t f = 0; f < w.size(); ++f) {
        const bool st = steht[f] == sollWert;
        const V3 p = ort(w[f][uz(fuss)]);
        if (st && !drin) { start = p; }
        drin = st;
        if (st) { d = std::max(d, std::hypot(p[0] - start[0], p[1] - start[1])); }
    }
    return d;
}

void vorschlagAusgeben(int argc, char** argv) {
    // modellretargettest --vorschlag <aus.txt> <datei.gla> <name> <spielpfad>
    std::ifstream in(argv[3], std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    GlaAnimation g;
    std::string fehler;
    if (!readGla(bytes, g, &fehler)) {
        std::printf("lesen: %s\n", fehler.c_str());
        return;
    }
    const RtCharakter c = rtVorschlag(g);
    std::ofstream o(argv[2], std::ios::binary);
    o << (argc > 5 ? argv[5] : argv[3]) << "\t" << g.bones.size() << "\t" << g.scale << "\t" << argv[4] << "\n";
    const auto zeile = [&](int b, char art, Seite s, int nr, int st) {
        if (b < 0) { return; }
        o << g.bones[uz(b)].name << "\t" << art << "\t" << (s == Seite::Links ? 'L' : (s == Seite::Rechts ? 'R' : 'M')) << "\t" << nr << "\t" << st << "\n";
    };
    zeile(c.wurzel, 'W', Seite::Mitte, 0, 0);
    zeile(c.becken, 'P', Seite::Mitte, 0, 0);
    zeile(c.bewegung, 'M', Seite::Mitte, 0, 0);
    const char arten[] = "SHKCABFT";
    for (const RtKette& k : c.ketten) {
        for (std::size_t i = 0; i < k.knochen.size(); ++i) { zeile(k.knochen[i], arten[static_cast<int>(k.art)], k.seite, k.nummer, static_cast<int>(i)); }
        for (const int t : k.twist) { zeile(t, 't', k.seite, 0, 0); }
    }
    std::printf("%s: %zu Ketten\n", argv[4], c.ketten.size());
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 5 && std::strcmp(argv[1], "--vorschlag") == 0) {
        vorschlagAusgeben(argc, argv);
        return 0;
    }
    const GlaAnimation jka = ausTabelle("jka");
    const GlaAnimation jk2 = ausTabelle("jk2");
    const std::vector<RtVorlageInfo> vorlagen = rtVorlagen();
    const auto vorlageName = [&](int i) { return i >= 0 && uz(i) < vorlagen.size() ? vorlagen[uz(i)].name : std::string("-"); };

    std::printf("Vorlagen\n");
    {
        expect("JKA erkannt: " + vorlageName(rtVorlageFuer(jka)), vorlageName(rtVorlageFuer(jka)) == "_humanoid (JKA)");
        expect("JK2 erkannt: " + vorlageName(rtVorlageFuer(jk2)), vorlageName(rtVorlageFuer(jk2)) == "_humanoid (JK2)");
        const RtCharakter cj = rtCharakter(jka);
        int arme = 0;
        int beine = 0;
        int finger = 0;
        for (const RtKette& k : cj.ketten) {
            arme += k.art == KettenArt::Arm ? 1 : 0;
            beine += k.art == KettenArt::Bein ? 1 : 0;
            finger += k.art == KettenArt::Finger ? 1 : 0;
        }
        expect("JKA: 2 Arme, 2 Beine, 6 Finger (" + std::to_string(arme) + "/" + std::to_string(beine) + "/" + std::to_string(finger) + ")",
               arme == 2 && beine == 2 && finger == 6);
        expect("JKA: Becken, Wurzel, Motion", cj.becken == kn(jka, "pelvis") && cj.wurzel == kn(jka, "model_root") && cj.bewegung == kn(jka, "Motion"));
        expect("JKA: lradiusX auf lradius -> lhand, Anteil " + z(cj.twistAnteil[uz(kn(jka, "lradiusX"))]),
               cj.twistAnfang[uz(kn(jka, "lradiusX"))] == kn(jka, "lradius") && cj.twistEnde[uz(kn(jka, "lradiusX"))] == kn(jka, "lhand") &&
                   cj.twistAnteil[uz(kn(jka, "lradiusX"))] > 0.3F && cj.twistAnteil[uz(kn(jka, "lradiusX"))] < 0.7F);
        expect("JKA: rhang_tag_bone und r_d1_j1 folgen rhand", cj.folgt[uz(kn(jka, "rhang_tag_bone"))] == kn(jka, "rhand") &&
                                                                    cj.folgt[uz(kn(jka, "r_d1_j1"))] == kn(jka, "rhand"));
        // Vorschlag aus der Erkennung gegen die Vorlage: dieselben Ketten
        const RtCharakter vj = rtVorschlag(jka);
        int gleich = 0;
        for (const RtKette& k : cj.ketten) {
            for (const RtKette& v : vj.ketten) { gleich += (k.schluessel() == v.schluessel() && k.knochen == v.knochen) ? 1 : 0; }
        }
        expect("JKA: Vorschlag = Vorlage in " + std::to_string(gleich) + " von " + std::to_string(cj.ketten.size()) + " Ketten",
               gleich == static_cast<int>(cj.ketten.size()));
        // fremde Namen mit der Lage des JKA-Skeletts
        const auto umbenannt = [&](const std::vector<std::pair<const char*, const char*>>& namen) {
            std::vector<std::string> n;
            std::vector<int> e;
            std::vector<V3> o;
            std::vector<int> alt;
            for (const auto& [a, neu] : namen) {
                const int b = kn(jka, a);
                alt.push_back(b);
                n.emplace_back(neu);
                o.push_back(ort(jka.bones[uz(b)].basePose));
            }
            for (std::size_t i = 0; i < namen.size(); ++i) {
                int p = jka.bones[uz(alt[i])].parent;
                int ne = -1;
                while (p >= 0 && ne < 0) {
                    for (std::size_t j = 0; j < alt.size(); ++j) {
                        if (alt[j] == p) { ne = static_cast<int>(j); }
                    }
                    p = jka.bones[uz(p)].parent;
                }
                e.push_back(ne);
            }
            return skelett(n, e, o, "fremd");
        };
        const GlaAnimation mix = umbenannt({{"pelvis", "mixamorig:Hips"}, {"lower_lumbar", "mixamorig:Spine"}, {"upper_lumbar", "mixamorig:Spine1"},
                                            {"thoracic", "mixamorig:Spine2"}, {"cervical", "mixamorig:Neck"}, {"cranium", "mixamorig:Head"},
                                            {"lclavical", "mixamorig:LeftShoulder"}, {"lhumerus", "mixamorig:LeftArm"}, {"lradius", "mixamorig:LeftForeArm"},
                                            {"lhand", "mixamorig:LeftHand"}, {"rclavical", "mixamorig:RightShoulder"}, {"rhumerus", "mixamorig:RightArm"},
                                            {"rradius", "mixamorig:RightForeArm"}, {"rhand", "mixamorig:RightHand"}, {"lfemurYZ", "mixamorig:LeftUpLeg"},
                                            {"ltibia", "mixamorig:LeftLeg"}, {"ltalus", "mixamorig:LeftFoot"}, {"rfemurYZ", "mixamorig:RightUpLeg"},
                                            {"rtibia", "mixamorig:RightLeg"}, {"rtalus", "mixamorig:RightFoot"}, {"l_d2_j1", "mixamorig:LeftHandIndex1"},
                                            {"l_d2_j2", "mixamorig:LeftHandIndex2"}});
        expect("Mixamo erkannt: " + vorlageName(rtVorlageFuer(mix)), vorlageName(rtVorlageFuer(mix)) == "HumanIK / Mixamo");
        const GlaAnimation bip = umbenannt({{"model_root", "Bip001"}, {"pelvis", "Bip001 Pelvis"}, {"lower_lumbar", "Bip001 Spine"}, {"upper_lumbar", "Bip001 Spine1"},
                                            {"cervical", "Bip001 Neck"}, {"cranium", "Bip001 Head"}, {"lclavical", "Bip001 L Clavicle"},
                                            {"lhumerus", "Bip001 L UpperArm"}, {"lradius", "Bip001 L Forearm"}, {"lhand", "Bip001 L Hand"},
                                            {"rclavical", "Bip001 R Clavicle"}, {"rhumerus", "Bip001 R UpperArm"}, {"rradius", "Bip001 R Forearm"},
                                            {"rhand", "Bip001 R Hand"}, {"lfemurYZ", "Bip001 L Thigh"}, {"ltibia", "Bip001 L Calf"}, {"ltalus", "Bip001 L Foot"},
                                            {"rfemurYZ", "Bip001 R Thigh"}, {"rtibia", "Bip001 R Calf"}, {"rtalus", "Bip001 R Foot"}});
        expect("Biped erkannt: " + vorlageName(rtVorlageFuer(bip)), vorlageName(rtVorlageFuer(bip)) == "3ds Max Biped");
        const GlaAnimation cat = umbenannt({{"pelvis", "BaseHumanPelvis"}, {"lower_lumbar", "BaseHumanSpine1"}, {"upper_lumbar", "BaseHumanSpine2"},
                                            {"thoracic", "BaseHumanRibcage"}, {"cervical", "BaseHumanNeck1"}, {"cranium", "BaseHumanHead"},
                                            {"lclavical", "BaseHumanLCollarbone"}, {"lhumerus", "BaseHumanLUpperarm"}, {"lradius", "BaseHumanLForearm"},
                                            {"lhand", "BaseHumanLPalm"}, {"rclavical", "BaseHumanRCollarbone"}, {"rhumerus", "BaseHumanRUpperarm"},
                                            {"rradius", "BaseHumanRForearm"}, {"rhand", "BaseHumanRPalm"}, {"lfemurYZ", "BaseHumanLThigh"}, {"ltibia", "BaseHumanLCalf"},
                                            {"ltalus", "BaseHumanLFoot"}, {"rfemurYZ", "BaseHumanRThigh"}, {"rtibia", "BaseHumanRCalf"}, {"rtalus", "BaseHumanRFoot"}});
        expect("CAT erkannt: " + vorlageName(rtVorlageFuer(cat)), vorlageName(rtVorlageFuer(cat)) == "3ds Max CAT (Base Human)");
        // Mixamo -> JKA: Arme, Beine, Wirbel, Hals, Kopf, Schluesselbeine und ein Finger zugeordnet
        const RtCharakter cm = rtCharakter(mix);
        const RtZuordnung zm = rtZuordnen(cm, cj);
        const std::vector<int> km = rtKnochenZuordnung(mix, cm, jka, cj, zm);
        expect("Mixamo -> JKA: lradius <- LeftForeArm, l_d2_j2 <- LeftHandIndex2, cranium <- Head",
               km[uz(kn(jka, "lradius"))] == kn(mix, "mixamorig:LeftForeArm") && km[uz(kn(jka, "l_d2_j2"))] == kn(mix, "mixamorig:LeftHandIndex2") &&
                   km[uz(kn(jka, "cranium"))] == kn(mix, "mixamorig:Head"));
        // Drehknochen: Mixamo hat keinen; die Hand dreht 90 Grad um den Unterarm -> lradiusX (Anteil 0,5) nimmt 45 Grad
        {
            std::vector<BoneMatrix> w = bindeWelt(mix);
            const V3 a = ort(w[uz(kn(mix, "mixamorig:LeftForeArm"))]);
            const V3 h = ort(w[uz(kn(mix, "mixamorig:LeftHand"))]);
            V3 ach = sub(h, a);
            const float l = laenge(ach);
            const float ax[3] = {ach[0] / l, ach[1] / l, ach[2] / l};
            drehTeilbaum(mix, w, kn(mix, "mixamorig:LeftHand"), ax, 90.0F);
            const GlaAnimation q = mitBildern(mix, {w});
            RtEinstellung e;
            e.poseQuelle.haltung = RtHaltung::Bindepose;
            e.poseZiel.haltung = RtHaltung::Bindepose;
            std::vector<std::vector<BoneMatrix>> zw;
            (void)rtUebertragen(q, cm, jka, cj, zm, e, 0, 1, zw, nullptr);
            const BoneMatrix rel = starrInvers(starr(zw[0][uz(kn(jka, "lradius"))])) * starr(zw[0][uz(kn(jka, "lradiusX"))]);
            const float tw = std::acos(std::clamp((rel.m[0][0] + rel.m[1][1] + rel.m[2][2] - 1.0F) * 0.5F, -1.0F, 1.0F)) * 57.29578F;
            expect("Drehknochen: Hand 90 Grad um den Unterarm -> lradiusX " + z(tw) + " Grad (soll 45)", std::fabs(tw - 45.0F) < 1.0F);
        }
        // Kreaturen: vorgeschlagene Ketten
        for (const char* k : {"bantha", "droideka"}) {
            const GlaAnimation g = ausTabelle(k);
            const RtCharakter c = rtCharakter(g);
            int b = 0;
            for (const RtKette& ke : c.ketten) { b += (ke.art == KettenArt::Bein || ke.art == KettenArt::Arm) ? 1 : 0; }
            expect(std::string(k) + ": vorgeschlagen, " + std::to_string(c.ketten.size()) + " Ketten, Glieder " + std::to_string(b),
                   c.vorlage.empty() && c.becken >= 0 && b >= 4);
        }
    }

    std::printf("Zuordnung\n");
    const RtCharakter cj = rtCharakter(jka);
    const RtCharakter c2 = rtCharakter(jk2);
    {
        const RtZuordnung zu = rtZuordnen(c2, cj);
        int voll = 0;
        std::string fehlt;
        for (std::size_t i = 0; i < zu.quelle.size(); ++i) {
            voll += zu.quelle[i] >= 0 ? 1 : 0;
            if (zu.quelle[i] < 0) { fehlt += " " + cj.ketten[i].schluessel(); }
        }
        // JK2 hat keine Rockschoesse (ltail/rtail)
        expect("JK2 -> JKA: alle Ketten ausser den Rockschoessen zugeordnet (" + std::to_string(voll) + fehlt + ")",
               voll == static_cast<int>(cj.ketten.size()) - 2 && fehlt == " tail_l_0 tail_r_0");
        const std::vector<int> m = rtKnochenZuordnung(jk2, c2, jka, cj, zu);
        for (const char* n : {"lradius", "l_d4_j2", "lradiusX", "rhang_tag_bone", "jaw"}) {
            const int q = m[uz(kn(jka, n))];
            if (q != kn(jk2, n)) { std::printf("    %s <- %s\n", n, q >= 0 ? jk2.bones[uz(q)].name.c_str() : "-"); }
        }
        expect("JK2 -> JKA: lradius, l_d4_j2, lradiusX, rhang_tag_bone, jaw ueber Kette/Namen",
               m[uz(kn(jka, "lradius"))] == kn(jk2, "lradius") && m[uz(kn(jka, "l_d4_j2"))] == kn(jk2, "l_d4_j2") &&
                   m[uz(kn(jka, "lradiusX"))] == kn(jk2, "lradiusX") && m[uz(kn(jka, "rhang_tag_bone"))] == kn(jk2, "rhang_tag_bone") &&
                   m[uz(kn(jka, "jaw"))] == kn(jk2, "jaw"));
        // von Hand: arm_l <- arm_r bleibt, leere werden gefuellt
        RtZuordnung h = zu;
        int armL = -1;
        int armRq = -1;
        for (std::size_t i = 0; i < cj.ketten.size(); ++i) {
            if (cj.ketten[i].schluessel() == "arm_l_0") { armL = static_cast<int>(i); }
        }
        for (std::size_t i = 0; i < c2.ketten.size(); ++i) {
            if (c2.ketten[i].schluessel() == "arm_r_0") { armRq = static_cast<int>(i); }
        }
        h.quelle[uz(armL)] = armRq;
        h.vonHand[uz(armL)] = 1;
        h.quelle[0] = -1;   // leer
        const RtZuordnung neu = rtZuordnen(c2, cj, &h);
        expect("von Hand bleibt, leere gefuellt", neu.quelle[uz(armL)] == armRq && neu.quelle[0] == zu.quelle[0]);
    }

    std::printf("Gleiche Figur\n");
    {
        // der Gang ohne Rutschen, dazu schwingen Arme, Unterarm und Brustkorb
        std::vector<int> steht0;
        std::vector<std::vector<BoneMatrix>> w = gang(jka, 12, 2, 20.0F, steht0);
        const float ax[3] = {1.0F, 0.0F, 0.0F};
        const float ay[3] = {0.0F, 1.0F, 0.0F};
        for (std::size_t f = 0; f < w.size(); ++f) {
            const float t = static_cast<float>(f) / 24.0F * 6.2831853F;
            drehTeilbaum(jka, w[f], kn(jka, "lhumerus"), ay, 30.0F * std::cos(t));
            drehTeilbaum(jka, w[f], kn(jka, "rradius"), ax, 40.0F * std::sin(t));
            drehTeilbaum(jka, w[f], kn(jka, "thoracic"), ay, 10.0F * std::sin(t));
        }
        const GlaAnimation q = mitBildern(jka, w);
        const std::vector<std::vector<BoneMatrix>> wq = welten(q);
        RtEinstellung e;
        const RtZuordnung zu = rtZuordnen(cj, cj);
        std::vector<std::vector<BoneMatrix>> ziel;
        RtBericht br;
        expect("uebertragen", rtUebertragen(q, cj, jka, cj, zu, e, 0, q.numFrames, ziel, &br));
        float grad = 0.0F;
        const float d = groessteAbweichung(wq, ziel, &grad);
        // Grenzen: die Quelle ist gepackt (Ort 1/64 je Achse, compressBone), das Ziel rechnet mit den Laengen der Grundstellung;
        // die Fuss-IK (Reach 1) zieht den Fuss auf den gepackten Ort der Quelle (bis 4 Glieder x 1/64 je Achse) und beugt dafuer das
        // fast gestreckte Knie ein wenig (Winkel ~ Laengenfehler / (Laenge x sin Knie))
        expect("Quelle = Ziel (" + (g_schlimmster >= 0 ? jka.bones[uz(g_schlimmster)].name : std::string("-")) + "): Ort " + z(d) + " Einheiten, Drehung " + z(grad) +
                   " Grad (Grenze 0,4 / 1,5), k " + z(br.k),
               d < 0.4F && grad < 1.5F);
        RtEinstellung nurFk = e;
        nurFk.ik = false;
        std::vector<std::vector<BoneMatrix>> zf;
        (void)rtUebertragen(q, cj, jka, cj, zu, nurFk, 0, q.numFrames, zf, nullptr);
        float gradF = 0.0F;
        const float dF = groessteAbweichung(wq, zf, &gradF);
        expect("Quelle = Ziel ohne IK: Ort " + z(dF) + ", Drehung " + z(gradF) + " Grad (Grenze 0,05 / 0,01)", dF < 0.05F && gradF < 0.01F);
        float fuss = 0.0F;
        for (std::size_t f = 0; f < ziel.size(); ++f) {
            for (const char* n : {"ltalus", "rtalus"}) { fuss = std::max(fuss, laenge(sub(ort(ziel[f][uz(kn(jka, n))]), ort(wq[f][uz(kn(jka, n))])))); }
        }
        expect("Fuesse am Ort der Quelle: " + z(fuss) + " (Grenze 0,05: Packung)", fuss < 0.05F);
        GlaAnimation gb;
        expect("gebacken", rtBacken(jka, ziel, "x", gb));
        float gradB = 0.0F;
        const float dB = groessteAbweichung(wq, welten(gb), &gradB);
        // Grenze der Kompression: Ort 1/64 je Achse und Glied (Kette bis 8 Glieder), Drehung 1/16383 je Anteil
        expect("gebacken gegen Quelle: Ort " + z(dB) + ", Drehung " + z(gradB) + " Grad (Grenze 0,45 / 1,5)", dB < 0.45F && gradB < 1.5F);
        expect("keine Spruenge (" + std::to_string(br.spruenge.size()) + ")", br.spruenge.empty());
        // als Schicht ueber der Bindepose (Figur im Mission-Tab): gebacken wie die Karten-GLA (rigBacken) = rtBacken
        const RigSchicht sch = rtAlsSchicht(jka, ziel);
        const GlaAnimation basis = mitBildern(jka, std::vector<std::vector<BoneMatrix>>(ziel.size(), bindeWelt(jka)));
        GlaAnimation gs;
        BackBericht bb;
        const bool sOk = rigBacken(basis, sch, 0, basis.numFrames, false, "x", gs, &bb);
        std::string b1;
        std::string b2;
        const bool w1 = schreibGla(gs, b1);
        const bool w2 = schreibGla(gb, b2);
        const float dS = groessteAbweichung(welten(gs), welten(gb));
        expect("als Schicht gebacken = rtBacken: " + z(dS) + " Einheiten, " + (b1 == b2 ? "Byte fuer Byte" : "nicht bytegleich"), sOk && w1 && w2 && dS < 0.05F);
        // Spruenge: ein Bild im Ziel um 90 Grad gedreht
        std::vector<std::vector<BoneMatrix>> kaputt = ziel;
        drehTeilbaum(jka, kaputt[10], kn(jka, "lhumerus"), ax, 90.0F);
        const std::vector<int> mq = rtKnochenZuordnung(jka, cj, jka, cj, zu);
        const std::vector<int> s = rtSpruenge(wq, kaputt, mq, 1.0F);
        expect("Sprung gefunden bei Bild 10/11 (" + std::to_string(s.size()) + ")", s.size() == 2 && s[0] == 10 && s[1] == 11);
    }

    std::printf("Grundstellung mit fScale\n");
    {
        // Die GLAs des Spiels tragen die Grundstellung mit fScale (_humanoid: B = 0,64 R, mdxaSkel_t BasePoseMat), die Welt also
        // X = E B mit demselben Mass. Quelle = Ziel muss dann ebenso die Quelle ergeben (die Vorschau war 25 Einheiten daneben).
        GlaAnimation js = jka;
        for (GlaBone& b : js.bones) {
            for (int i = 0; i < 3; ++i) {
                b.basePose.m[i][i] = 0.64F;
                b.basePoseInv.m[i][i] = 1.0F / 0.64F;
                b.basePoseInv.m[i][3] = -b.basePose.m[i][3] / 0.64F;
            }
        }
        const RtCharakter cs = rtCharakter(js);
        std::vector<int> steht;
        std::vector<std::vector<BoneMatrix>> w = gang(jka, 12, 2, 20.0F, steht);
        // dieselben Bewegungen mit dem Mass der Grundstellung: X' = X S
        for (auto& f : w) {
            for (BoneMatrix& m : f) {
                for (int i = 0; i < 3; ++i) {
                    for (int j = 0; j < 3; ++j) { m.m[i][j] *= 0.64F; }
                }
            }
        }
        const GlaAnimation q = mitBildern(js, w);
        const std::vector<std::vector<BoneMatrix>> wq = welten(q);
        std::vector<std::vector<BoneMatrix>> ziel;
        RtEinstellung e;
        (void)rtUebertragen(q, cs, js, cs, rtZuordnen(cs, cs), e, 0, q.numFrames, ziel, nullptr);
        GlaAnimation gb;
        (void)rtBacken(js, ziel, "x", gb);
        float grad = 0.0F;
        const float d = groessteAbweichung(wq, welten(gb), &grad);
        expect("Quelle = Ziel mit fScale 0,64, gebacken: Ort " + z(d) + ", Drehung (Matrix) " + z(grad) + " (Grenze 0,45 / 1,5)", d < 0.45F && grad < 1.5F);
        std::vector<BoneMatrix> g0;
        (void)rtGangBild(js, cs, GangRegler{}, 0.25F, g0);
        float mass = 0.0F;
        for (int i = 0; i < 3; ++i) { mass += g0[uz(kn(js, "ltibia"))].m[i][0] * g0[uz(kn(js, "ltibia"))].m[i][0]; }
        expect("Generator mit fScale: die Welt traegt das Mass der Grundstellung (" + z(std::sqrt(mass)) + ")", std::fabs(std::sqrt(mass) - 0.64F) < 1e-3F);
    }

    std::printf("Bindepose\n");
    {
        // Quelle: JKA-Lagen, aber die Arme waagrecht (T-Bindepose); Ziel: die A-Bindepose des Spiels (48 Grad nach unten)
        const auto tLage = [&](const std::string& n, V3 p) {
            const bool arm = n.find("humerus") != std::string::npos || n.find("radius") != std::string::npos || n.find("hand") != std::string::npos ||
                             n.find("hang") != std::string::npos || n.find("_d") != std::string::npos;
            if (!arm) { return p; }
            const V3 schulter{n[0] == 'l' ? 5.92F : -5.92F, 1.13F, 54.51F};
            const V3 d = sub(p, schulter);
            // um die Vorwaertsachse so drehen, dass der Oberarm waagrecht liegt
            const float a = std::atan2(8.10F, 7.27F) * (n[0] == 'l' ? 1.0F : -1.0F);
            const float c = std::cos(a);
            const float s = std::sin(a);
            return V3{schulter[0] + c * d[0] - s * d[2], p[1], schulter[2] + s * d[0] + c * d[2]};
        };
        const GlaAnimation tq = ausTabelle("jka", tLage);
        const RtCharakter ct = rtCharakter(tq);
        const std::vector<std::vector<BoneMatrix>> bind{bindeWelt(tq), bindeWelt(tq)};
        const GlaAnimation q = mitBildern(tq, bind);
        const RtZuordnung zu = rtZuordnen(ct, cj);
        const auto armWinkel = [&](const std::vector<BoneMatrix>& wz) {
            const std::vector<BoneMatrix> wq = bind[0];
            return std::max(winkel(sub(ort(wq[uz(kn(tq, "lradius"))]), ort(wq[uz(kn(tq, "lhumerus"))])), sub(ort(wz[uz(kn(jka, "lradius"))]), ort(wz[uz(kn(jka, "lhumerus"))]))),
                            winkel(sub(ort(wq[uz(kn(tq, "rhand"))]), ort(wq[uz(kn(tq, "rradius"))])), sub(ort(wz[uz(kn(jka, "rhand"))]), ort(wz[uz(kn(jka, "rradius"))]))));
        };
        RtEinstellung ohne;
        ohne.poseQuelle.haltung = RtHaltung::Bindepose;
        ohne.poseZiel.haltung = RtHaltung::Bindepose;
        std::vector<std::vector<BoneMatrix>> wo;
        RtBericht bo;
        (void)rtUebertragen(q, ct, jka, cj, zu, ohne, 0, 2, wo, &bo);
        const float rot = armWinkel(wo[0]);
        expect("rot: ohne Ausgleich der Bindepose Arme um " + z(rot) + " Grad verdreht (Posen " + z(bo.poseAbweichung) + ")", rot > 40.0F && bo.poseAbweichung > 40.0F);
        RtEinstellung mit;   // T fuer beide
        std::vector<std::vector<BoneMatrix>> wm;
        RtBericht bm;
        (void)rtUebertragen(q, ct, jka, cj, zu, mit, 0, 2, wm, &bm);
        const float gruen = armWinkel(wm[0]);
        expect("gruen: ausgerichtet (T) " + z(gruen) + " Grad, Posen " + z(bm.poseAbweichung), gruen < 0.5F && bm.poseAbweichung < 0.5F);
        RtEinstellung aa;
        aa.poseQuelle.haltung = RtHaltung::A;
        aa.poseZiel.haltung = RtHaltung::A;
        std::vector<std::vector<BoneMatrix>> wa;
        (void)rtUebertragen(q, ct, jka, cj, zu, aa, 0, 2, wa, nullptr);
        expect("gruen: ausgerichtet (A) " + z(armWinkel(wa[0])) + " Grad", armWinkel(wa[0]) < 0.5F);
        // von Hand: 20 Grad am Oberarm des Ziels in der Pose -> der Arm des Ziels weicht um 20 Grad ab
        RtEinstellung hand = mit;
        hand.poseZiel.vonHand.assign(jka.bones.size(), {1.0F, 0.0F, 0.0F, 0.0F});
        const float halb = 10.0F / 57.29578F;
        hand.poseZiel.vonHand[uz(kn(jka, "lhumerus"))] = {std::cos(halb), 0.0F, std::sin(halb), 0.0F};
        std::vector<BoneMatrix> pz;
        rtPoseWelt(jka, cj, hand.poseZiel, pz);
        std::vector<BoneMatrix> pz0;
        rtPoseWelt(jka, cj, mit.poseZiel, pz0);
        const float w20 = winkel(sub(ort(pz[uz(kn(jka, "lradius"))]), ort(pz[uz(kn(jka, "lhumerus"))])), sub(ort(pz0[uz(kn(jka, "lradius"))]), ort(pz0[uz(kn(jka, "lhumerus"))])));
        expect("Pose von Hand: Oberarm um " + z(w20) + " Grad (soll 20)", std::fabs(w20 - 20.0F) < 0.05F);
    }

    std::printf("Laenge\n");
    {
        std::vector<int> steht;
        const std::vector<std::vector<BoneMatrix>> wg = gang(jka, 16, 4, 20.0F, steht);
        const GlaAnimation q = mitBildern(jka, wg);
        const std::vector<std::vector<BoneMatrix>> wq = welten(q);
        const float quelle = std::max(standRutschen(wq, kn(jka, "ltalus"), steht, 1), standRutschen(wq, kn(jka, "rtalus"), steht, 0));
        expect("Quelle rutscht nicht: " + z(quelle), quelle < 0.1F);
        // Ziel: Beine 1,3-fach (alles unter dem Becken, das Becken hoeher)
        // Beine um den Boden (z = 0) 1,3-fach, alles darueber um 0,3 x Hueftenhoehe hoeher
        const auto lang = [&](const std::string& n, V3 p) {
            const bool bein = n.find("femur") != std::string::npos || n.find("tibia") != std::string::npos || n.find("talus") != std::string::npos;
            return bein ? V3{p[0], p[1], p[2] * 1.3F} : V3{p[0], p[1], p[2] + 35.5866F * 0.3F};
        };
        const GlaAnimation lz = ausTabelle("jka", lang);
        const RtCharakter cl = rtCharakter(lz);
        const RtZuordnung zu = rtZuordnen(cj, cl);
        RtEinstellung ohne;
        ohne.becken = false;
        ohne.ik = false;
        std::vector<std::vector<BoneMatrix>> wo;
        RtBericht bo;
        (void)rtUebertragen(q, cj, lz, cl, zu, ohne, 0, q.numFrames, wo, &bo);
        const float rot = std::max(standRutschen(wo, kn(lz, "ltalus"), steht, 1), standRutschen(wo, kn(lz, "rtalus"), steht, 0));
        int kontaktBilder = 0;
        for (const RtBild& b : bo.bilder) { kontaktBilder += (b.kontakt[0] ? 1 : 0) + (b.kontakt[1] ? 1 : 0); }
        expect("rot: ohne Laengenausgleich rutscht der Standfuss " + z(rot) + " Einheiten (Messer " + z(bo.rutschenMax) + ", " + std::to_string(kontaktBilder) + " Kontaktbilder)", rot > 2.0F && bo.rutschenMax > 2.0F);
        RtEinstellung fk;
        fk.ik = false;
        std::vector<std::vector<BoneMatrix>> wf;
        RtBericht bf;
        (void)rtUebertragen(q, cj, lz, cl, zu, fk, 0, q.numFrames, wf, &bf);
        const float gruen = std::max(standRutschen(wf, kn(lz, "ltalus"), steht, 1), standRutschen(wf, kn(lz, "rtalus"), steht, 0));
        // k: Laenge der gestreckten Beine (Retarget-Pose T: Ober- und Unterschenkel senkrecht)
        const auto bein = [&](const GlaAnimation& g) {
            const V3 h = ort(g.bones[uz(kn(g, "lfemurYZ"))].basePose);
            const V3 k2 = ort(g.bones[uz(kn(g, "ltibia"))].basePose);
            const V3 a = ort(g.bones[uz(kn(g, "ltalus"))].basePose);
            return laenge(sub(k2, h)) + laenge(sub(a, k2));
        };
        const float kSoll = bein(lz) / bein(jka);
        expect("gruen: Becken mit k " + z(bf.k) + " (soll " + z(kSoll) + "): " + z(gruen) + " Einheiten", gruen < 0.1F && std::fabs(bf.k - kSoll) < 0.001F);
        // nur die Unterschenkel 1,3-fach: FK rutscht, IK + Kontakte nicht
        const auto schienbein = [&](const std::string& n, V3 p) {
            const float zusatz = (21.4029F - 7.4664F) * 0.3F;
            if (n.find("talus") != std::string::npos) { return V3{p[0], p[1], p[2]}; }
            return V3{p[0], p[1], p[2] + zusatz};
        };
        const GlaAnimation sz = ausTabelle("jka", schienbein);
        const RtCharakter cs = rtCharakter(sz);
        const RtZuordnung zs = rtZuordnen(cj, cs);
        std::vector<std::vector<BoneMatrix>> ws;
        RtBericht bs;
        (void)rtUebertragen(q, cj, sz, cs, zs, fk, 0, q.numFrames, ws, &bs);
        const float fkR = std::max(standRutschen(ws, kn(sz, "ltalus"), steht, 1), standRutschen(ws, kn(sz, "rtalus"), steht, 0));
        RtEinstellung ik;
        std::vector<std::vector<BoneMatrix>> wi;
        RtBericht bi;
        (void)rtUebertragen(q, cj, sz, cs, zs, ik, 0, q.numFrames, wi, &bi);
        const float ikR = std::max(standRutschen(wi, kn(sz, "ltalus"), steht, 1), standRutschen(wi, kn(sz, "rtalus"), steht, 0));
        expect("lange Unterschenkel: FK " + z(fkR) + " -> IK + Kontakte " + z(ikR) + " Einheiten (Messer " + z(bi.rutschenMax) + ")",
               fkR > 0.5F && ikR < 0.05F && bi.rutschenMax < 0.05F);
        // Reach 0 = FK, 0,5 dazwischen
        RtEinstellung r0 = ik;
        r0.reachFuss = 0.0F;
        std::vector<std::vector<BoneMatrix>> w0;
        (void)rtUebertragen(q, cj, sz, cs, zs, r0, 0, q.numFrames, w0, nullptr);
        expect("Reach 0 = FK (" + z(groessteAbweichung(w0, ws)) + ")", groessteAbweichung(w0, ws) < 1e-4F);
        RtEinstellung r5 = ik;
        r5.reachFuss = 0.5F;
        r5.kontakte = false;
        std::vector<std::vector<BoneMatrix>> w5;
        (void)rtUebertragen(q, cj, sz, cs, zs, r5, 0, q.numFrames, w5, nullptr);
        const float m5 = std::max(standRutschen(w5, kn(sz, "ltalus"), steht, 1), standRutschen(w5, kn(sz, "rtalus"), steht, 0));
        expect("Reach 0,5 dazwischen: " + z(m5), m5 > ikR && m5 < fkR);
        // Reihenfolge: IK vor dem Becken -> das Becken schiebt die geloesten Beine weg
        RtEinstellung um = ik;
        um.reihenfolge = {RtSchritt::Fk, RtSchritt::Ik, RtSchritt::Becken, RtSchritt::Wurzel};
        std::vector<std::vector<BoneMatrix>> wu;
        (void)rtUebertragen(q, cj, sz, cs, zs, um, 0, q.numFrames, wu, nullptr);
        const float uR = std::max(standRutschen(wu, kn(sz, "ltalus"), steht, 1), standRutschen(wu, kn(sz, "rtalus"), steht, 0));
        expect("Reihenfolge FK, IK, Becken: " + z(uR) + " (umgestellt wirkt)", uR > ikR + 0.5F);
        // Fussrutsch-Messer der Quelle
        expect("Messer Quelle " + z(bi.rutschenQuelle) + " Einheiten", bi.rutschenQuelle < 0.5F);
    }

    std::printf("Saebelgriff\n");
    {
        // JK2: eine Bewegung des rechten Unterarms und der Hand; im JKA-Ziel bleibt rhang_tag_bone an der Hand
        std::vector<std::vector<BoneMatrix>> w;
        const float ax[3] = {1.0F, 0.0F, 0.0F};
        const float az[3] = {0.0F, 0.0F, 1.0F};
        for (int f = 0; f < 10; ++f) {
            std::vector<BoneMatrix> x = bindeWelt(jk2);
            // flaches Skelett: Unterarm, Hand, Finger, Tag von Hand zusammen drehen
            const char* arm[] = {"rradius", "rradiusX", "rhand", "rhang_tag_bone", "r_d1_j1", "r_d1_j2", "r_d1_j3", "r_d2_j1", "r_d2_j2", "r_d2_j3",
                                 "r_d3_j1", "r_d3_j2", "r_d3_j3", "r_d4_j1", "r_d4_j2", "r_d4_j3", "r_d5_j1", "r_d5_j2", "r_d5_j3", "mc7"};
            const BoneMatrix r = drehungUm(ax, 8.0F * static_cast<float>(f));
            const BoneMatrix r2 = drehungUm(az, 5.0F * static_cast<float>(f));
            const V3 o = ort(x[uz(kn(jk2, "rradius"))]);
            for (const char* n : arm) {
                BoneMatrix m = x[uz(kn(jk2, n))];
                const V3 p = sub(ort(m), o);
                BoneMatrix rr = r * r2;
                rr.m[0][3] = rr.m[1][3] = rr.m[2][3] = 0.0F;
                BoneMatrix mr = m;
                mr.m[0][3] = mr.m[1][3] = mr.m[2][3] = 0.0F;
                BoneMatrix neu = rr * mr;
                for (int k = 0; k < 3; ++k) { neu.m[k][3] = o[uz(k)] + rr.m[k][0] * p[0] + rr.m[k][1] * p[1] + rr.m[k][2] * p[2]; }
                x[uz(kn(jk2, n))] = neu;
            }
            w.push_back(std::move(x));
        }
        const GlaAnimation q = mitBildern(jk2, w);
        const RtZuordnung zu = rtZuordnen(c2, cj);
        std::vector<std::vector<BoneMatrix>> wz;
        RtEinstellung e;
        (void)rtUebertragen(q, c2, jka, cj, zu, e, 0, q.numFrames, wz, nullptr);
        const int hand = kn(jka, "rhand");
        const int tag = kn(jka, "rhang_tag_bone");
        const V3 rel0 = sub(ort(wz[0][uz(tag)]), ort(wz[0][uz(hand)]));
        float abw = 0.0F;
        for (const auto& x : wz) {
            const BoneMatrix rel = starrInvers(x[uz(hand)]) * x[uz(tag)];
            const BoneMatrix rel1 = starrInvers(wz[0][uz(hand)]) * wz[0][uz(tag)];
            abw = std::max(abw, laenge(sub(ort(rel), ort(rel1))));
        }
        expect("Tag an der Hand: Abweichung " + z(abw) + " (Abstand " + z(laenge(rel0)) + ")", abw < 0.01F);
    }

    std::printf("Generator (5b)\n");
    {
        // jede Haltung, Gehen und Laufen: nahtlos (Phase 1 = Phase 0), der Fuss im Stand genau auf dem Boden der Pose und
        // gleichmaessig gleitend (relativ zum Boden steht er)
        const char* namen[kGangHaltungen] = {"normal", "schwer", "schleichend", "verletzt", "saebel"};
        for (const GangArt art : {GangArt::Gehen, GangArt::Laufen}) {
            for (int hi = 0; hi < kGangHaltungen; ++hi) {
                GangRegler r;
                r.art = art;
                r.haltung = static_cast<GangHaltung>(hi);
                r.tempo = art == GangArt::Laufen ? 300.0F : 90.0F;
                r.schritt = art == GangArt::Laufen ? 30.0F : 20.0F;
                const GangBefund b = rtGangPruefen(jka, cj, r, 60);
                expect(std::string(art == GangArt::Laufen ? "Laufen " : "Gehen ") + namen[hi] + ": Naht " + z(b.naht) + ", Boden " + z(b.bodenAbw) + ", Gleiten " +
                           z(b.gleitAbw) + " (" + std::to_string(b.standBilder) + " Standbilder)",
                       b.naht < 1e-3F && b.bodenAbw < 0.01F && b.gleitAbw < 0.01F && b.standBilder > 4);
            }
        }
        // Rot-Probe des Pruefers: ein Schritt, den das Bein nicht reicht - der Fuss bleibt nicht am Boden
        GangRegler weit;
        weit.schritt = 80.0F;   // mehr als zwei Beinlaengen
        const GangBefund b = rtGangPruefen(jka, cj, weit, 20);
        expect("Pruefer findet einen Fuss, der nicht am Boden bleibt (Schritt zu weit): Boden " + z(b.bodenAbw), b.bodenAbw > 1.0F);
        std::vector<std::vector<BoneMatrix>> w;
        GangRegler r;
        r.schritt = 45.0F;   // Zyklus 2 * 45 / 90 = 1 s
        expect("Zyklus 1 s bei 20 fps: 20 Bilder", rtGang(jka, cj, r, 20, w) && w.size() == 20U);
        r.schritt = 20.0F;
        GangRegler sb;
        sb.haltung = GangHaltung::Saebel;
        std::vector<BoneMatrix> a;
        std::vector<BoneMatrix> n0;
        (void)rtGangBild(jka, cj, sb, 0.25F, a);
        (void)rtGangBild(jka, cj, r, 0.25F, n0);
        // Saebel: die rechte Hand vorn (vor dem Becken, -Y), beim normalen Gang neben dem Koerper
        const float vornS = ort(a[uz(kn(jka, "pelvis"))])[1] - ort(a[uz(kn(jka, "rhand"))])[1];
        const float vornN = ort(n0[uz(kn(jka, "pelvis"))])[1] - ort(n0[uz(kn(jka, "rhand"))])[1];
        expect("Saebel gezogen: rechte Hand " + z(vornS) + " vor dem Becken (normal " + z(vornN) + ")", vornS > vornN + 5.0F);
    }

    std::printf("%s (%d)\n", fails == 0 ? "alles ok" : "FEHLER", fails);
    return fails == 0 ? 0 : 1;
}
