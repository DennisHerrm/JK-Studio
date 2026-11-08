// Probe REST3 Punkt 7: Rechnen die Shader mit Bitgleichheits-Anspruch auf JEDER Grafikkarte wie das Spiel? (nur Windows/MSVC)
//
// Gemessen wird auf WARP und der Grafikkarte des Rechners (defgpu, turbgpu, figurglanz); fuer NVIDIA und AMD steht hier der Beleg aus
// dem Bytecode (D3DDisassemble) gegen die Regeln der Spezifikation:
//
//   D3D11.3 Functional Spec 3.1.3.1: float32 add, sub und mul sind auf 0,5 ULP genau (also wie IEEE 754 gerundet, wie die SSE-Rechnung
//     des Spiels). 3.1.3.2: verschmolzene Anweisungen (mad, dp2/dp3/dp4) muessen nur so genau sein wie die ungenaueste unverschmolzene
//     Reihenfolge - nicht bitgleich; rcp und sqrt 1 ULP, Division x * (1/y) erlaubt, rsq/exp/log/sincos eigene, lockere Grenzen.
//     Denormale werden an Ein- und Ausgang jeder Gleitkommarechnung auf eine Null mit Vorzeichen gesetzt.
//   HLSL "precise" (Variable Syntax, learn.microsoft.com): getrennte Rechnungen bleiben getrennt (kein mul + add zu mad), die
//     Reihenfolge bleibt, keine IEEE-unsicheren Umformungen. Im Bytecode steht das als [precise] an der Anweisung; vs_4_x kennt es nicht
//     (deshalb vs_5_0 ab Merkmalsstufe 11_0, kartenVsZiel/wolkenPsZiel).
//
// Also verlangt die Probe: Jede Anweisung, die zu einem beanspruchten Wert beitraegt, ist [precise] fuer die gebrauchten Anteile, und
// darunter steht keine mad, dp2/dp3/dp4, div, rcp, rsq, sqrt, exp, log oder sincos. Beansprucht sind: der verformte Ort (deformVertexes,
// verformeEcke) und die Texturkoordinate der Stufe (tcGen vector, die tcMod-Kette) im Vertex-Shader der Karte, die Texturkoordinate der
// Figurenstufe (tcMod-Kette), die Koordinate der Wolkenstufe (Pixel-Shader, tcMod-Kette) und die Kette des Echtzeit-Lichts (vs3: tcKette
// mit Profil und Fahnen von karte_gpu_win32.cpp). Grenzen des Anspruchs: die Kameramatrix (gViewProj, cb0[0..3]) - das Spiel projiziert
// mit OpenGL anders - und die Haut der Figur (Knochen aus dem dynamisch indizierten Puffer: RB_SurfaceGhoul rechnet sie auf der CPU,
// die Vorschau auf der Grafikkarte; der Anspruch gilt ab der gehaeuteten Ecke). Ein Denormal (|x| < 2^-126) wuerde auf der Grafikkarte
// zu 0, im Spiel nicht - Koordinaten, Wellen und Tafelwerte erreichen das nicht.
//
// Die Pruefung ist eine Rueckwaertsscheibe ueber die Register-Anteile, ohne Ablaufrichtung (ein wiederverwendetes Register zaehlt ganz):
// sie meldet eher zu viel als zu wenig.

#include "bhed/gpushader.h"
#include "bhed/gpustate.h"

#include <d3dcompiler.h>
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace bhed::gpu;

namespace {

int fehler = 0;
void erwarte(const std::string& was, bool ok) {
    std::printf("  %-4s  %s\n", ok ? "ok" : "FEHL", was.c_str());
    if (!ok) { ++fehler; }
}

struct Operand {
    std::string reg;     // r0, o1, v2, x0, cb0[3] (fest), cb2 (dynamisch), l, icb, t0, s0, null
    std::string anteile; // xyzw-Buchstaben (Ziel: Maske, Quelle: Swizzle)
    bool dynamischCb = false;
    std::vector<Operand> index;   // Register in eckigen Klammern
};
struct Anweisung {
    std::string op;
    std::string precise;   // "" nicht, "*" ganz, sonst die Anteile
    std::vector<Operand> ziele;
    std::vector<Operand> quellen;
    std::string text;
};

std::string ohneRand(const std::string& s) {
    const std::size_t a = s.find_first_not_of(' ');
    const std::size_t b = s.find_last_not_of(' ');
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}

Operand leseOperand(std::string t) {
    Operand o;
    t = ohneRand(t);
    while (!t.empty() && (t[0] == '-' || t[0] == '|')) { t.erase(0, 1); }
    while (!t.empty() && t.back() == '|') { t.pop_back(); }
    if (t.rfind("l(", 0) == 0 || t.rfind("d(", 0) == 0) {
        o.reg = "l";
        return o;
    }
    std::size_t i = 0;
    while (i < t.size() && (std::isalpha(static_cast<unsigned char>(t[i])) || std::isdigit(static_cast<unsigned char>(t[i])) || t[i] == '_')) { ++i; }
    o.reg = t.substr(0, i);
    if (i < t.size() && t[i] == '[') {
        const std::size_t z = t.find(']', i);
        const std::string innen = t.substr(i + 1, z - i - 1);
        // fest: cb0[3]; dynamisch: cb2[r1.x + 0]
        bool ziffern = !innen.empty();
        for (const char c : innen) { ziffern = ziffern && std::isdigit(static_cast<unsigned char>(c)) != 0; }
        if (ziffern) {
            o.reg += "[" + innen + "]";
        } else {
            o.dynamischCb = o.reg.rfind("cb", 0) == 0;
            std::size_t p = 0;
            while ((p = innen.find_first_of("rx", p)) != std::string::npos) {
                std::size_t e = p + 1;
                while (e < innen.size() && std::isdigit(static_cast<unsigned char>(innen[e]))) { ++e; }
                if (e < innen.size() && innen[e] == '.' && e > p + 1) {
                    Operand q;
                    q.reg = innen.substr(p, e - p);
                    q.anteile = innen.substr(e + 1, 1);
                    o.index.push_back(q);
                }
                p = e;
            }
        }
        i = z + 1;
    }
    if (i < t.size() && t[i] == '.') { o.anteile = t.substr(i + 1); }
    if (o.anteile.empty()) { o.anteile = "xyzw"; }
    return o;
}

// Ziel-Operanden je Anweisung (die meisten eins; null zaehlt als Ziel)
int zielZahl(const std::string& op) {
    static const std::set<std::string> zwei = {"sincos", "udiv", "imul", "umul", "uaddc", "usubb", "swapc"};
    return zwei.count(op) != 0U ? 2 : 1;
}

std::vector<Anweisung> zerlege(const std::string& dis) {
    static const std::set<std::string> ohneOperand = {"loop", "endloop", "else", "endif", "break", "continue", "ret", "endswitch", "default"};
    std::vector<Anweisung> aus;
    std::istringstream ein(dis);
    std::string z;
    bool code = false;
    while (std::getline(ein, z)) {
        if (!z.empty() && z.back() == '\r') { z.pop_back(); }
        const std::string t = ohneRand(z);
        if (t.empty() || t.rfind("//", 0) == 0) { continue; }
        if (t.rfind("vs_5_0", 0) == 0 || t.rfind("ps_5_0", 0) == 0 || t.rfind("vs_4", 0) == 0 || t.rfind("ps_4", 0) == 0) {
            code = true;
            continue;
        }
        if (!code || t.rfind("dcl_", 0) == 0) { continue; }
        std::size_t sp = t.find(' ');
        Anweisung a;
        a.text = t;
        a.op = t.substr(0, sp);
        const std::size_t klammer = a.op.find('(');
        if (klammer != std::string::npos) { a.op = a.op.substr(0, klammer); }   // sample_indexable(texture2d)(...)
        if (a.op == "switch" || a.op == "case" || a.op == "label" || a.op == "call" || a.op == "endswitch" || a.op == "default") {
            continue;   // kommen in den erzeugten Shadern nicht vor
        }
        if (ohneOperand.count(a.op) != 0U) {
            aus.push_back(a);   // loop, endloop, else, endif, break, continue, ret: Marken ohne Operanden
            continue;
        }
        if (sp == std::string::npos) { continue; }
        if (a.op.rfind("if_", 0) == 0 || a.op.rfind("breakc", 0) == 0 || a.op.rfind("retc", 0) == 0 || a.op.rfind("continuec", 0) == 0) {
            a.quellen.push_back(leseOperand(t.substr(sp + 1)));   // die Bedingung
            aus.push_back(a);
            continue;
        }
        std::string rest = t.substr(sp + 1);
        if (rest.rfind("[precise", 0) == 0) {
            const std::size_t e = rest.find(']');
            const std::string p = rest.substr(0, e + 1);
            const std::size_t k = p.find('(');
            a.precise = k == std::string::npos ? "*" : p.substr(k + 1, p.find(')') - k - 1);
            rest = ohneRand(rest.substr(e + 1));
        }
        // Operanden an den Kommas der obersten Ebene trennen
        std::vector<std::string> teile;
        int tiefe = 0;
        std::string cur;
        for (const char c : rest) {
            if (c == '(' || c == '[') { ++tiefe; }
            if (c == ')' || c == ']') { --tiefe; }
            if (c == ',' && tiefe == 0) {
                teile.push_back(cur);
                cur.clear();
            } else {
                cur += c;
            }
        }
        if (!cur.empty()) { teile.push_back(cur); }
        const int nz = zielZahl(a.op);
        for (std::size_t i = 0; i < teile.size(); ++i) {
            (static_cast<int>(i) < nz ? a.ziele : a.quellen).push_back(leseOperand(teile[i]));
        }
        aus.push_back(a);
    }
    return aus;
}

// Gelesene Anteile einer Quelle: dp2/dp3/dp4 lesen die ersten 2/3/4 des Swizzles, sonst alle Buchstaben
std::string gelesen(const Anweisung& a, const Operand& q) {
    if (a.op == "dp2") { return q.anteile.substr(0, 2); }
    if (a.op == "dp3") { return q.anteile.substr(0, 3); }
    return q.anteile;
}

bool registerZaehlt(const std::string& r) { return !r.empty() && (r[0] == 'r' || r[0] == 'x' || r[0] == 'v' || r[0] == 'o'); }

struct Befund {
    int anweisungen = 0;
    std::vector<std::string> verstoesse;
};

// Die Grenze des Anspruchs: eine Anweisung, die einen der `grenzen`-Puffereintraege (die Kamera gViewProj, die Stellung der Figur)
// oder einen dynamisch indizierten Puffer (die Knochen) liest. Sie selbst zaehlt nicht; der Ort, den Kamera bzw. Stellung bekommen,
// zaehlt weiter, der Weg in die Knochen nicht.
using Menge = std::set<std::string>;
struct Scheibe {
    const std::vector<Anweisung>& code;
    const Menge& grenzen;
    std::vector<bool> drin;
    std::vector<bool> grenze;
};

bool istGrenze(const Anweisung& a, const Menge& grenzen, bool& kamera) {
    bool g = false;
    kamera = false;
    for (const Operand& q : a.quellen) {
        if (q.dynamischCb) { g = true; }
        if (grenzen.count(q.reg) != 0U) { g = kamera = true; }
    }
    return g && a.op != "mov";
}

void quellenDazu(const Anweisung& a, Menge& noetig, bool nurRegister) {
    for (const Operand& q : a.quellen) {
        if (registerZaehlt(q.reg) && !q.dynamischCb) {
            for (const char c : gelesen(a, q)) { noetig.insert(q.reg + "." + c); }
        }
        if (!nurRegister) {
            for (const Operand& x : q.index) { noetig.insert(x.reg + "." + x.anteile); }
        }
    }
}

// Ende des Blocks, der bei `i` (loop bzw. if_*) beginnt: Index der passenden endloop/endif; `sonst` die else-Stelle oder -1
std::size_t blockEnde(const std::vector<Anweisung>& c, std::size_t i, long& sonst) {
    int tiefe = 0;
    sonst = -1;
    for (std::size_t k = i; k < c.size(); ++k) {
        const std::string& op = c[k].op;
        if (op == "loop" || op.rfind("if_", 0) == 0) { ++tiefe; }
        if (op == "endloop" || op == "endif") {
            if (--tiefe == 0) { return k; }
        }
        if (op == "else" && tiefe == 1) { sonst = static_cast<long>(k); }
    }
    return c.size();
}

// Rueckwaerts ueber [von, bis): was danach gebraucht wird (`noetig`) -> was davor gebraucht wird. Verzweigungen getrennt und vereinigt
// (die Bedingung zaehlt mit), eine Schleife bis nichts Neues mehr hinzukommt.
Menge rueckwaerts(Scheibe& sc, std::size_t von, std::size_t bis, Menge noetig);

Menge block(Scheibe& sc, std::size_t von, std::size_t bis, const Menge& noetig) {
    // [von, bis) beginnt mit loop oder if_* und endet hinter dem passenden Ende
    const Anweisung& kopf = sc.code[von];
    long sonst = -1;
    const std::size_t ende = blockEnde(sc.code, von, sonst);
    if (kopf.op == "loop") {
        Menge aus = noetig;
        for (int runde = 0; runde < 64; ++runde) {
            const Menge davor = rueckwaerts(sc, von + 1, ende, aus);
            Menge neu = aus;
            neu.insert(davor.begin(), davor.end());
            if (neu == aus) { break; }
            aus = std::move(neu);
        }
        (void)bis;
        return aus;
    }
    Menge dann = rueckwaerts(sc, von + 1, sonst >= 0 ? static_cast<std::size_t>(sonst) : ende, noetig);
    const Menge sonstZweig = sonst >= 0 ? rueckwaerts(sc, static_cast<std::size_t>(sonst) + 1, ende, noetig) : noetig;
    dann.insert(sonstZweig.begin(), sonstZweig.end());
    quellenDazu(kopf, dann, false);
    return dann;
}

Menge rueckwaerts(Scheibe& sc, std::size_t von, std::size_t bis, Menge noetig) {
    std::size_t i = bis;
    while (i > von) {
        --i;
        const Anweisung& a = sc.code[i];
        if (a.op == "endloop" || a.op == "endif") {
            // den Anfang suchen
            int tiefe = 0;
            std::size_t k = i + 1;
            while (k > von) {
                --k;
                const std::string& op = sc.code[k].op;
                if (op == "endloop" || op == "endif") { ++tiefe; }
                if (op == "loop" || op.rfind("if_", 0) == 0) {
                    if (--tiefe == 0) { break; }
                }
            }
            noetig = block(sc, k, i + 1, noetig);
            i = k;
            continue;
        }
        if (a.op.rfind("breakc", 0) == 0 || a.op.rfind("retc", 0) == 0 || a.op.rfind("continuec", 0) == 0) {
            quellenDazu(a, noetig, false);
            continue;
        }
        if (a.ziele.empty()) { continue; }
        bool trifft = false;
        for (const Operand& z : a.ziele) {
            for (const char c : z.anteile) { trifft = trifft || noetig.count(z.reg + "." + c) != 0U; }
        }
        if (!trifft) { continue; }
        sc.drin[i] = true;
        for (const Operand& z : a.ziele) {
            if (z.reg.rfind('x', 0) == 0) { continue; }   // ein indiziertes Feld wird nicht ganz ueberschrieben
            for (const char c : z.anteile) { noetig.erase(z.reg + "." + c); }
        }
        bool kamera = false;
        if (istGrenze(a, sc.grenzen, kamera)) {
            sc.grenze[i] = true;
            if (kamera) { quellenDazu(a, noetig, true); }
            continue;
        }
        quellenDazu(a, noetig, false);
    }
    return noetig;
}

Befund pruefe(const std::vector<Anweisung>& code, const Menge& wurzeln, const Menge& grenzen = {"cb0[0]", "cb0[1]", "cb0[2]", "cb0[3]"}) {
    static const std::set<std::string> arith = {"add", "mul", "mad", "div", "dp2", "dp3", "dp4", "sqrt", "rsq", "rcp", "exp", "log",
                                                "sincos", "frc", "round_ne", "round_ni", "round_pi", "round_z", "min", "max", "itof",
                                                "utof", "ftoi", "ftou"};
    static const std::set<std::string> verboten = {"mad", "dp2", "dp3", "dp4", "div", "sqrt", "rsq", "rcp", "exp", "log", "sincos"};
    Scheibe sc{code, grenzen, std::vector<bool>(code.size(), false), std::vector<bool>(code.size(), false)};
    (void)rueckwaerts(sc, 0, code.size(), wurzeln);
    Befund b;
    for (std::size_t i = 0; i < code.size(); ++i) {
        if (!sc.drin[i] || sc.grenze[i]) { continue; }
        const Anweisung& a = code[i];
        ++b.anweisungen;
        if (arith.count(a.op) == 0U) { continue; }
        if (verboten.count(a.op) != 0U) {
            b.verstoesse.push_back("nicht genau gerundet: " + a.text);
            continue;
        }
        bool genau = a.precise == "*";
        if (!genau && !a.precise.empty()) {
            genau = true;
            for (const Operand& z : a.ziele) {
                for (const char c : z.anteile) { genau = genau && a.precise.find(c) != std::string::npos; }
            }
        }
        if (!genau) { b.verstoesse.push_back("ohne [precise]: " + a.text); }
    }
    return b;
}

bool uebersetze(const std::string& quelle, const char* profil, UINT fahnen, std::string& dis) {
    ID3DBlob* blob = nullptr;
    ID3DBlob* err = nullptr;
    if (FAILED(D3DCompile(quelle.data(), quelle.size(), "precisebeleg", nullptr, nullptr, "main", profil, fahnen, 0, &blob, &err))) {
        std::printf("%s\n", err != nullptr ? static_cast<const char*>(err->GetBufferPointer()) : "D3DCompile");
        if (err != nullptr) { err->Release(); }
        return false;
    }
    if (err != nullptr) { err->Release(); }
    ID3DBlob* t = nullptr;
    const bool ok = SUCCEEDED(D3DDisassemble(blob->GetBufferPointer(), blob->GetBufferSize(), 0, nullptr, &t)) && t != nullptr;
    if (ok) { dis.assign(static_cast<const char*>(t->GetBufferPointer()), t->GetBufferSize()); }
    if (t != nullptr) { t->Release(); }
    blob->Release();
    return ok;
}

void melde(const std::string& was, const std::string& dis, const std::set<std::string>& wurzeln,
           const Menge& grenzen = {"cb0[0]", "cb0[1]", "cb0[2]", "cb0[3]"}) {
    const std::vector<Anweisung> code = zerlege(dis);
    const Befund b = pruefe(code, wurzeln, grenzen);
    std::string z = was + ": " + std::to_string(b.anweisungen) + " Anweisungen im Anspruch, " + std::to_string(b.verstoesse.size()) + " Verstoesse";
    for (std::size_t i = 0; i < b.verstoesse.size() && i < (std::getenv("PB_ALLE") != nullptr ? 999U : 3U); ++i) { z += "\n          " + b.verstoesse[i]; }
    erwarte(z, b.anweisungen > 0 && b.verstoesse.empty());
}

// Wie turbgpu kVsHuelle: die Kette mit den Werten je Ecke aus Buffer<float4> t0
const char* const kVsHuelle = R"(
Buffer<float4> gEin : register(t0);
struct VSIn { float3 pos : POSITION; };
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
VSOut main(VSIn e, uint id : SV_VertexID) {
    uint b = id * 8;
    float4 a = gEin[b];
    VSOut o;
    o.pos = float4(e.pos, 1.0);
    o.uv = tcKette(a.zw, gEin[b + 1], gEin[b + 2], gEin[b + 3], gEin[b + 4], gEin[b + 5], gEin[b + 6], gEin[b + 7], a.xy, true);
    return o;
}
)";

// Dieselbe Kette im Pixel-Shader (die Wolkenstufe), die Werte unveraendert vom Vertex-Shader
const char* const kPsHuelle = R"(
struct PSIn {
    float4 pos : SV_POSITION;
    nointerpolation float4 a : TEXCOORD0;
    nointerpolation float4 w0 : TEXCOORD1;
    nointerpolation float4 w1 : TEXCOORD2;
    nointerpolation float4 w2 : TEXCOORD3;
    nointerpolation float4 w3 : TEXCOORD4;
    nointerpolation float4 w4 : TEXCOORD5;
    nointerpolation float4 w5 : TEXCOORD6;
    nointerpolation float4 w6 : TEXCOORD7;
};
float4 main(PSIn e) : SV_Target {
    return float4(tcKette(e.a.zw, e.w0, e.w1, e.w2, e.w3, e.w4, e.w5, e.w6, e.a.xy, true), 0.0, 1.0);
}
)";

}  // namespace

int main() {
    std::printf("precisebeleg\n");
    const std::set<std::string> ortUndSt = {"o0.x", "o0.y", "o0.z", "o0.w", "o1.x", "o1.y"};
    // --- Vertex-Shader der Karte (vs_5_0, Fahnen 0 wie holeShader) -----------------------------------------------------------
    struct Fall {
        const char* was;
        std::uint32_t merkmale;
        int tcMods;
    };
    const Fall faelle[] = {
        {"wave", kDeform, 0},
        {"bulge", kBulge, 0},
        {"move", kDeformMove, 0},
        {"normal", kDeformNormal, 0},
        {"wave, normal, bulge, move", kDeform | kDeformNormal | kBulge | kDeformMove, 0},
        {"tcMod 1", kTexMod, 1},
        {"tcMod 4", kTexMod, 4},
        {"tcMod 4 mit turb", kTexMod | kTurb, 4},
        {"tcGen vector, tcMod 2", kTexGenVec | kTexMod, 2},
        {"wave, tcMod 4 mit turb", kDeform | kTexMod | kTurb, 4},
        {"autosprite (Ecken von der CPU)", kAutosprite | kTexMod, 2},
    };
    for (const Fall& f : faelle) {
        std::string dis;
        const bool ok = uebersetze(vertexShaderHlsl(f.merkmale, f.tcMods), "vs_5_0", 0, dis);
        erwarte(std::string("Karte ") + f.was + ": uebersetzt", ok);
        if (ok) { melde(std::string("Karte ") + f.was, dis, ortUndSt); }
    }
    // --- Figur (vs_5_0): der Ort ab der gehaeuteten Ecke (deformVertexes) ------------------------------------------------------
    // Die Koordinate der Figurenstufe laeuft im selben Shader auch durch tcGen environment und den Glanz im Raum der Entity (dp3,
    // sqrt - nicht beansprucht, PTFIX3/MERGE2: gemessen gegen Spielfotos); die tcMod-Kette darin ist derselbe Text wie unten (tcKette).
    {
        std::string dis;
        const bool ok = uebersetze(figurVertexShaderHlsl(0), "vs_5_0", 0, dis);
        erwarte("Figur: uebersetzt", ok);
        // Grenzen: die Kamera (cb0[0..3]) und die Stellung der Figur (gFigurWelt, cb2[384..387] hinter den 384 Knochenzeilen)
        if (ok) {
            melde("Figur (deformVertexes)", dis, {"o0.x", "o0.y", "o0.z", "o0.w"},
                  {"cb0[0]", "cb0[1]", "cb0[2]", "cb0[3]", "cb2[384]", "cb2[385]", "cb2[386]", "cb2[387]"});
        }
    }
    // --- Wolkenstufe: die tcMod-Kette als Pixel-Shader mit dem Profil der Wolkenstufe (wolkenPsZiel, ab 11_0 ps_5_0) -------------
    // Die Ausgangskoordinate der Kuppel (R_InitSkyTexCoords, im Spiel auf der CPU an einem Gitter) rechnet der echte Shader mit div
    // und sqrt - nicht beansprucht; beansprucht ist die Kette darauf (DEF1 P46).
    {
        std::string dis;
        const std::string echt = pixelShaderHlsl(kWolke | kTexMod);
        erwarte("Wolken: der Pixel-Shader rechnet die Kette mit tcKette", echt.find("tcKette(") != std::string::npos);
        const bool ok = uebersetze(tcModHlsl() + kPsHuelle, wolkenPsZiel(true), 0, dis);
        erwarte("Wolken (tcKette): uebersetzt", ok);
        if (ok) { melde("Wolken (tcKette, ps_5_0)", dis, {"o0.x", "o0.y"}); }
    }
    // --- Echtzeit-Licht vs3: tcKette mit vs_5_0 und D3DCOMPILE_OPTIMIZATION_LEVEL3 (karte_gpu_win32.cpp) -------------------------
    {
        std::string dis;
        const bool ok = uebersetze(tcModHlsl() + kVsHuelle, "vs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, dis);
        erwarte("Echtzeit-Licht vs3 (tcKette): uebersetzt", ok);
        if (ok) { melde("Echtzeit-Licht vs3 (tcKette, O3)", dis, {"o1.x", "o1.y"}); }
    }
    // --- Die Probe selbst: ein Shader ohne precise muss auffallen ----------------------------------------------------------------
    {
        std::string dis;
        const char* const ohne = "float4 main(float4 a : POSITION, float4 b : NORMAL) : SV_POSITION { return a * b + a.yzwx; }";
        const bool ok = uebersetze(ohne, "vs_5_0", 0, dis);
        const Befund b = ok ? pruefe(zerlege(dis), {"o0.x", "o0.y", "o0.z", "o0.w"}) : Befund{};
        erwarte("Gegenprobe: a * b + c ohne precise wird gemeldet (" + std::to_string(b.verstoesse.size()) + ")", ok && !b.verstoesse.empty());
    }
    std::printf("%s\n", fehler == 0 ? "alles gut" : "FEHLER");
    return fehler == 0 ? 0 : 1;
}
