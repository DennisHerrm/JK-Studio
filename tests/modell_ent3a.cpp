// modell_ent3a.cpp - ENTSCHEID3A (12.11.): die Rechnungen ohne Oberflaeche
//
//   Drehmodus (B9)   bhed/modell/drehmodus.h gegen Blender 5.0 mathutils (Y:\werkzeuge\ent3a\blender_dreh.py, ohne Fenster
//                    ausgefuehrt; die Zahlen unten sind seine Ausgabe): je Matrix die Euler-Winkel in allen sechs Reihenfolgen
//                    (Matrix.to_euler), die Quaternion (Matrix.to_quaternion), Achse/Winkel (Quaternion.to_axis_angle) und die
//                    Rueckwege Euler -> Matrix, nicht normierte Quaternion -> Matrix, Achse/Winkel -> Matrix
//   Begleitdatei     bhed/modell/stapelbegleit.h an data/fixtures/model.glm: Stapel mit Spiegeln (in die Datei) und Glaetten
//   (B12)            (Vorschau) schreiben, die .glm wieder lesen, Begleitdatei lesen und anwenden - derselbe Stapel, und
//                    Speichern ergibt dieselbe .glm Byte fuer Byte; nur Vorschau: keine Basis; leerer Stapel: keine Datei;
//                    geaenderte .glm -> Alt, abgeschnittene Datei -> Kaputt, neuere Fassung -> Fassung, andere Flaechen -> Flaechen
//
// Aufruf: modellent3atest <model.glm>
#include "bhed/modell/dokument.h"
#include "bhed/modell/drehmodus.h"
#include "bhed/modell/glmdatei.h"
#include "bhed/modell/stapel.h"
#include "bhed/modell/stapelbegleit.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {
int fails = 0;
void expect(const std::string& what, bool ok) {
    std::printf("%s %s\n", ok ? "OK    " : "FEHLER", what.c_str());
    if (!ok) { ++fails; }
}
std::string slurp(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}
using namespace bhed;
using namespace bhed::modell;

// Ausgabe von blender_dreh.py (Blender 5.0.0, a37564c4df7a)
const char* const kBlender = R"(M0 0.786438286 -0.556600094 -0.267789423 0.484613031 0.824831367 -0.291210264 0.382968783 0.0992446616 0.918414593
E0 XYZ 0.107643157 -0.393007964 0.552255154
E0 XZY 0.339393198 -0.453166217 0.505920708
E0 YXZ 0.0994083062 -0.395065695 0.593615472
E0 YZX 0.119745508 -0.328194857 0.590287745
E0 ZXY 0.295491695 -0.283712327 0.531199753
E0 ZYX 0.307051301 -0.271097928 0.615907073
Q0 0.939372718 0.103913747 -0.17318958 0.277103305
A0 0.699999273 0.303046167 -0.505076885 0.808122993
M1 0.917266965 0.396850646 -0.0336291417 0.265013725 -0.671207488 -0.692277968 -0.297303051 0.62609154 -0.720847309
E1 XYZ 2.42642784 0.30186671 0.281257838
E1 XZY 2.34074235 0.313434273 0.26821816
E1 YXZ 2.46506238 -0.391180575 0.533959985
E1 YZX 2.39095736 -0.0366459079 -0.408083081
E1 ZXY 2.37695193 0.0466184393 -0.376042753
E1 ZYX 2.37640882 -0.0336354747 -0.408327788
Q1 0.362357736 0.909577012 0.181915402 -0.0909577161
A1 2.4000001 0.975900054 0.195180014 -0.0975900218
M2 0.00872655213 0 0.999961972 0 1 0 -0.999961972 0 0.00872655213
E2 XYZ 0 1.56206965 0
E2 XZY -0 1.56206965 0
E2 YXZ 0 1.56206965 -0
E2 YZX 0 1.56206965 -0
E2 ZXY -0 1.56206965 0
E2 ZYX -0 1.56206965 -0
Q2 0.710185409 0 0.704014719 0
A2 1.56206954 0 1.00000012 0
M3 -0.914893806 0.382978737 0.127659678 0.382978797 0.723404229 0.574468136 0.127659515 0.574468195 -0.80851078
E3 XYZ -0.61775583 -3.0135839 -0.396441281
E3 XZY -0.67114383 -3.00295305 0.393018723
E3 YXZ 0.611954272 -2.98499084 -0.486899257
E3 YZX 0.671143949 3.00295281 -0.393018723
E3 ZXY -0.611954093 2.9849906 0.486899257
E3 ZYX 0.617755711 3.01358366 0.396441281
Q3 4.4144258e-08 0.206284255 0.928279102 0.309426397
A3 3.1415925 0.206284285 0.928279221 0.309426427
M4 0.999599159 -0.0276021194 0.00629924098 0.0273191594 0.998773813 0.0412852801 -0.0074310787 -0.0410966426 0.999127567
E4 XYZ -0.041109357 0.00743114715 0.0273233131
E4 XZY -0.0413124412 0.00743392156 0.0273225587
E4 YXZ -0.0411082208 0.00743743032 0.027628975
E4 YZX -0.0411238968 0.00630168337 0.0276056267
E4 ZXY -0.0412970185 0.00630465802 0.0273458771
E4 ZYX -0.0412978381 0.00629928242 0.0276061762
Q4 0.999687552 -0.0206019189 0.00343365269 0.0137346126
A4 0.0499971062 -0.824210286 0.137368366 0.549473524
M5 1 0 0 0 1 0 0 0 1
E5 XYZ 0 -0 0
E5 XZY -0 -0 0
E5 YXZ 0 -0 -0
E5 YZX 0 0 -0
E5 ZXY -0 0 0
E5 ZYX -0 0 -0
Q5 1 0 0 0
A5 0 0 1 0
R XYZ -0.188762575 -0.693093598 0.695693851 0.412453771 -0.69887054 -0.584347367 0.891207397 0.176638648 0.417789668
R XZY -0.188762575 -0.726947606 -0.660239279 0.909297407 -0.383296609 0.162055209 -0.370873123 -0.569763899 0.733363628
R YXZ 0.12681137 -0.837518394 0.531490207 0.556878567 -0.383296609 -0.736864984 0.820856333 0.389418334 0.417789668
R YZX -0.188762575 -0.909297407 0.370873123 0.0328425728 -0.383296609 -0.923041224 0.981473386 -0.162055209 0.10221573
R ZXY -0.504336536 -0.268028975 -0.820856333 0.837518394 -0.383296609 -0.389418334 -0.21025607 -0.883880198 0.417789668
R ZYX -0.188762575 -0.412453771 -0.891207397 0.98194319 -0.0677226707 -0.176638648 0.0125003308 -0.908457756 0.417789668
RQ 0.599999964 -0.376470596 -0.70588237 0 0.882352948 -0.470588267 0.800000072 0.282352954 0.529411733
RA 0.315322131 0 0.948984683 0 1 0 -0.948984683 0 0.315322131
)";

std::string g_blender;   // die Ausgabe oben

const char* kOrdnung[6] = {"XYZ", "XZY", "YXZ", "YZX", "ZXY", "ZYX"};
DrehModus modusVon(const std::string& o) {
    for (int i = 0; i < 6; ++i) {
        if (o == kOrdnung[i]) { return static_cast<DrehModus>(i + 1); }
    }
    return DrehModus::Xyz;
}

bool nah(float a, float b, float t = 2e-5F) { return std::fabs(a - b) <= t; }
bool matNah(const float a[3][3], const float b[3][3], float t = 2e-5F) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            if (!nah(a[r][c], b[r][c], t)) { return false; }
        }
    }
    return true;
}

void drehTests() {
    std::istringstream in(g_blender);
    std::string z;
    float m[3][3]{};
    int matrizen = 0;
    int euler = 0;
    int quats = 0;
    int achsen = 0;
    int rueck = 0;
    while (std::getline(in, z)) {
        std::istringstream w(z);
        std::string art;
        w >> art;
        if (art.empty()) { continue; }
        if (art[0] == 'M') {
            for (auto& r : m) {
                for (float& v : r) { w >> v; }
            }
            ++matrizen;
        } else if (art[0] == 'E') {
            std::string o;
            float soll[3];
            w >> o >> soll[0] >> soll[1] >> soll[2];
            float e[3];
            eulerAusMatrix(m, modusVon(o), e);
            const bool ok = nah(e[0], soll[0]) && nah(e[1], soll[1]) && nah(e[2], soll[2]);
            if (!ok) { std::printf("  %s %s: %.7f %.7f %.7f, Blender %.7f %.7f %.7f\n", art.c_str(), o.c_str(), e[0], e[1], e[2], soll[0], soll[1], soll[2]); }
            expect("Euler " + o + " aus Matrix " + art.substr(1) + " wie Blender Matrix.to_euler", ok);
            // und zurueck: dieselbe Matrix
            float zurueck[3][3];
            matrixAusEuler(e, modusVon(o), zurueck);
            expect("Euler " + o + " -> Matrix " + art.substr(1) + " ergibt die Matrix wieder", matNah(zurueck, m, 5e-5F));
            ++euler;
        } else if (art[0] == 'Q') {
            float soll[4];
            w >> soll[0] >> soll[1] >> soll[2] >> soll[3];
            float q[4];
            quatAusMatrix(m, q);
            bool ok = nah(q[0], soll[0]) && nah(q[1], soll[1]) && nah(q[2], soll[2]) && nah(q[3], soll[3]);
            if (!ok) { std::printf("  %s: %.7f %.7f %.7f %.7f, Blender %.7f %.7f %.7f %.7f\n", art.c_str(), q[0], q[1], q[2], q[3], soll[0], soll[1], soll[2], soll[3]); }
            expect("Quaternion aus Matrix " + art.substr(1) + " wie Blender Matrix.to_quaternion (w >= 0)", ok && q[0] >= 0.0F);
            ++quats;
        } else if (art[0] == 'A') {
            float soll[4];
            w >> soll[0] >> soll[1] >> soll[2] >> soll[3];
            float q[4];
            quatAusMatrix(m, q);
            float aw[4];
            achseWinkelAusQuat(q, aw);
            const bool ok = nah(aw[0], soll[0], 5e-5F) && nah(aw[1], soll[1], 5e-5F) && nah(aw[2], soll[2], 5e-5F) && nah(aw[3], soll[3], 5e-5F);
            if (!ok) { std::printf("  %s: %.7f %.7f %.7f %.7f, Blender %.7f %.7f %.7f %.7f\n", art.c_str(), aw[0], aw[1], aw[2], aw[3], soll[0], soll[1], soll[2], soll[3]); }
            expect("Achse/Winkel " + art.substr(1) + " wie Blender Quaternion.to_axis_angle", ok);
            float zurueck[3][3];
            matrixAusAchseWinkel(aw, zurueck);
            expect("Achse/Winkel -> Matrix " + art.substr(1) + " ergibt die Matrix wieder", matNah(zurueck, m, 5e-5F));
            ++achsen;
        } else if (art == "R" || art == "RQ" || art == "RA") {
            std::string o;
            if (art == "R") { w >> o; }
            float soll[3][3];
            for (auto& r : soll) {
                for (float& v : r) { w >> v; }
            }
            float ist[3][3];
            if (art == "R") {
                const float e[3] = {0.4F, -1.1F, 2.0F};
                matrixAusEuler(e, modusVon(o), ist);
            } else if (art == "RQ") {
                const float q[4] = {2.0F, 0.5F, -1.0F, 0.25F};   // eingetippt, nicht normiert
                matrixAusQuat(q, ist);
            } else {
                const float aw[4] = {1.25F, 0.0F, 2.0F, 0.0F};   // Achse nicht normiert
                matrixAusAchseWinkel(aw, ist);
            }
            expect("Rueckweg " + art + (o.empty() ? "" : " " + o) + " -> Matrix wie Blender", matNah(ist, soll));
            ++rueck;
        }
    }
    char t[160];
    std::snprintf(t, sizeof t, "Blender-Referenz vollstaendig: %d Matrizen, %d Euler, %d Quaternionen, %d Achse/Winkel, %d Rueckwege", matrizen, euler, quats, achsen, rueck);
    expect(t, matrizen == 6 && euler == 36 && quats == 6 && achsen == 6 && rueck == 8);
    // Kennzahlen der Modi
    expect("Werte je Modus: Quaternion 4, Euler 3, Achse/Winkel 4",
           drehWerteZahl(DrehModus::Quaternion) == 4 && drehWerteZahl(DrehModus::Zyx) == 3 && drehWerteZahl(DrehModus::AchseWinkel) == 4);
    // ohne Drehung: Quaternion 1 0 0 0, Achse/Winkel 0 mit Achse Y wie Blender
    const float einheit[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    float w4[4];
    drehWerte(einheit, DrehModus::Quaternion, w4);
    expect("Einheit: Quaternion (1, 0, 0, 0)", w4[0] == 1.0F && w4[1] == 0.0F && w4[2] == 0.0F && w4[3] == 0.0F);
    drehWerte(einheit, DrehModus::AchseWinkel, w4);
    expect("Einheit: Achse/Winkel (0, 0, 1, 0) wie Blender", w4[0] == 0.0F && w4[1] == 0.0F && w4[2] == 1.0F && w4[3] == 0.0F);
}

// --- Begleitdatei -----------------------------------------------------------------------------------------------------------

int ersteFlaeche(const Inhalt& in) {
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if (!in.teile[i].istTag() && !in.teile[i].stufen.empty() && !in.teile[i].stufen[0].ecken.empty()) { return static_cast<int>(i); }
    }
    return -1;
}

std::string glmVon(const Inhalt& in) {
    std::string b;
    (void)glmSchreiben(in, b);
    return b;
}

void begleitTests(const std::string& glmPfad) {
    const std::string bytes = slurp(glmPfad);
    Inhalt in;
    std::string fehler;
    expect("model.glm gelesen", !bytes.empty() && ausGlm(bytes, in, &fehler));
    const int t = ersteFlaeche(in);
    expect("eine Flaeche mit Ecken", t >= 0);
    if (t < 0) { return; }
    const StapelUmgebung u;
    expect("leerer Stapel: keine Begleitdatei", stapelBegleitSchreiben(in, bytes, nullptr).empty());
    // Spiegeln (geht in die Datei) und Glaetten (nur Vorschau)
    Modifikator sp;
    sp.art = ModArt::Spiegeln;
    sp.kennung = 1;
    Modifikator gl;
    gl.art = ModArt::Glaetten;
    gl.kennung = 2;
    gl.datei = false;
    gl.stufen = 2;
    gl.an = false;
    in.teile[static_cast<std::size_t>(t)].stapel = {sp, gl};
    const std::string glm = glmVon(stapelFuerDatei(in, u));    // wie Speichern: die .glm mit dem Ergebnis des Spiegelns
    Inhalt ohne = in;
    for (Teil& x : ohne.teile) { x.stapel.clear(); }
    const std::string basis = glmVon(ohne);
    expect("die .glm traegt das Spiegeln (anders als die Basis)", !glm.empty() && glm != basis);
    const std::string datei = stapelBegleitSchreiben(in, glm, &basis);
    expect("Begleitdatei geschrieben (Kopf, Basis)", datei.rfind("JKSTUDIO-STACK 1\n", 0) == 0 && datei.find("\nbasis ") != std::string::npos);
    // Oeffnen: die .glm lesen, die Begleitdatei lesen und anwenden
    Inhalt neu;
    expect("die gespeicherte .glm liest sich", ausGlm(glm, neu, &fehler));
    BegleitDaten d;
    expect("Begleitdatei liest sich und passt zur .glm", stapelBegleitLesen(datei, glm, d) == BegleitFehler::Keiner && d.eintraege() == 2 && !d.basisGlm.empty());
    expect("Begleitdatei angewendet", stapelBegleitAnwenden(d, neu) == BegleitFehler::Keiner);
    expect("derselbe Stapel nach dem Oeffnen (Art, Kennung, Auge, Datei, Achse, Richtung, Stufen, Raender)",
           neu.teile[static_cast<std::size_t>(t)].stapel == in.teile[static_cast<std::size_t>(t)].stapel);
    expect("die Flaeche darunter ist wieder die Basis (nicht das gespiegelte Netz der .glm)",
           neu.teile[static_cast<std::size_t>(t)].stufen[0].ecken.size() == ohne.teile[static_cast<std::size_t>(t)].stufen[0].ecken.size());
    expect("Speichern nach dem Oeffnen schreibt dieselbe .glm Byte fuer Byte", glmVon(stapelFuerDatei(neu, u)) == glm);
    // nur Vorschau: die .glm ist die Basis, keine Basis in der Begleitdatei
    in.teile[static_cast<std::size_t>(t)].stapel = {gl};
    const std::string glmV = glmVon(stapelFuerDatei(in, u));
    const std::string dateiV = stapelBegleitSchreiben(in, glmV, nullptr);
    Inhalt neuV;
    BegleitDaten dV;
    const bool okV = ausGlm(glmV, neuV, &fehler) && stapelBegleitLesen(dateiV, glmV, dV) == BegleitFehler::Keiner && dV.basisGlm.empty() &&
                     stapelBegleitAnwenden(dV, neuV) == BegleitFehler::Keiner;
    expect("nur Vorschau: ohne Basis, Stapel zurueck, .glm unveraendert",
           okV && neuV.teile[static_cast<std::size_t>(t)].stapel == in.teile[static_cast<std::size_t>(t)].stapel && glmVon(stapelFuerDatei(neuV, u)) == glmV && glmV == bytes);
    // Fehlerfaelle: das Modell laedt trotzdem (der Inhalt bleibt, wie er war)
    std::string anders = glm;
    anders[anders.size() / 2] = static_cast<char>(anders[anders.size() / 2] ^ 0x5A);
    BegleitDaten dx;
    expect("die .glm wurde ausserhalb geaendert -> Alt", stapelBegleitLesen(datei, anders, dx) == BegleitFehler::Alt);
    expect("abgeschnittene Begleitdatei -> Kaputt", stapelBegleitLesen(datei.substr(0, datei.size() / 3), glm, dx) == BegleitFehler::Kaputt);
    expect("leere Begleitdatei -> Kaputt", stapelBegleitLesen(std::string(), glm, dx) == BegleitFehler::Kaputt);
    std::string spaeter = datei;
    spaeter.replace(0, 16, "JKSTUDIO-STACK 9");
    expect("neuere Fassung -> Fassung", stapelBegleitLesen(spaeter, glm, dx) == BegleitFehler::Fassung);
    Inhalt fremd = neu;
    fremd.teile[static_cast<std::size_t>(t)].name += "_x";
    const std::string fremdGlm = glmVon(fremd);
    BegleitDaten df;
    const BegleitFehler lf = stapelBegleitLesen(stapelBegleitSchreiben(in, fremdGlm, nullptr), fremdGlm, df);
    Inhalt vorher = fremd;
    expect("andere Flaechen -> Flaechen, der Inhalt bleibt", lf == BegleitFehler::Keiner && stapelBegleitAnwenden(df, fremd) == BegleitFehler::Flaechen &&
                                                                fremd.teile[static_cast<std::size_t>(t)].stapel == vorher.teile[static_cast<std::size_t>(t)].stapel &&
                                                                fremd.teile[static_cast<std::size_t>(t)].name == vorher.teile[static_cast<std::size_t>(t)].name &&
                                                                fremd.teile.size() == vorher.teile.size());
    expect("Name der Begleitdatei: model.glm -> model.glm.stack", stapelBegleitName("a/b/model.glm") == "a/b/model.glm.stack");
}

}  // namespace

int main(int argc, char** argv) {
    g_blender = kBlender;
    drehTests();
    if (argc > 1) {
        begleitTests(argv[1]);
    } else {
        expect("Aufruf: modellent3atest <model.glm>", false);
    }
    std::printf("%s\n", fails == 0 ? "ALLES OK" : "FEHLGESCHLAGEN");
    return fails == 0 ? 0 : 1;
}
