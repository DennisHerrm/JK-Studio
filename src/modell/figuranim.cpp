// modell/figuranim.cpp - Modell-Block Stufe 7: Figuren im Mission-Tab animieren und fuer das Spiel bauen
// (siehe bhed/modell/figuranim.h)
#include "bhed/modell/figuranim.h"

#include "bhed/modell/animkurven.h"
#include "bhed/modell/fehler.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/glavorrat.h"
#include "bhed/modell/ik.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <unordered_map>

namespace bhed::modell {
namespace {

constexpr const char* kMarke = "//(BehavEd) figur ";
constexpr const char* kDaten = "//(BehavEd) figur+ ";

std::string zahl(float v) {
    char b[32];
    std::snprintf(b, sizeof(b), "%.9g", static_cast<double>(v));
    return b;
}
std::string zahl(double v) {
    char b[40];
    std::snprintf(b, sizeof(b), "%.17g", v);
    return b;
}

std::string zitat(const std::string& s) { return "\"" + s + "\""; }

// Woerter einer Zeile: "..." ist ein Wort (ohne Anfuehrungszeichen).
std::vector<std::string> woerter(const std::string& z) {
    std::vector<std::string> w;
    std::size_t i = 0;
    while (i < z.size()) {
        while (i < z.size() && (z[i] == ' ' || z[i] == '\t' || z[i] == '\r')) { ++i; }
        if (i >= z.size()) { break; }
        if (z[i] == '"') {
            const std::size_t e = z.find('"', i + 1);
            w.push_back(z.substr(i + 1, e == std::string::npos ? std::string::npos : e - i - 1));
            i = e == std::string::npos ? z.size() : e + 1;
        } else {
            std::size_t e = i;
            while (e < z.size() && z[e] != ' ' && z[e] != '\t' && z[e] != '\r') { ++e; }
            w.push_back(z.substr(i, e - i));
            i = e;
        }
    }
    return w;
}

std::string ohneEinzug(const std::string& z) {
    std::size_t i = 0;
    while (i < z.size() && (z[i] == ' ' || z[i] == '\t')) { ++i; }
    return z.substr(i);
}

bool beginntMit(const std::string& z, const char* p) { return z.compare(0, std::strlen(p), p) == 0; }

// Henkelarten, die henkelNeu selbst rechnet (Blender BKE_nurb_handle_calc: HD_AUTO, HD_VECT, HD_AUTO_ANIM)
bool selbstGerechnet(Henkel h) { return h == Henkel::Auto || h == Henkel::Vektor || h == Henkel::AutoGeklemmt; }

void schluesselText(const RigSchluessel& k, std::string& z) {
    z += " " + std::to_string(k.bild) + " " + std::to_string(static_cast<int>(k.interp));
    for (const float v : k.q) { z += " " + zahl(v); }
    for (const float v : k.t) { z += " " + zahl(v); }
    std::string arten;
    bool frei = false;
    for (int c = 0; c < kKanaele; ++c) {
        arten += static_cast<char>('0' + static_cast<int>(k.henkelL[c]));
        arten += static_cast<char>('0' + static_cast<int>(k.henkelR[c]));
        frei = frei || !selbstGerechnet(k.henkelL[c]) || !selbstGerechnet(k.henkelR[c]);
    }
    z += " " + arten;
    if (frei) {
        z += " w";
        for (int c = 0; c < kKanaele; ++c) {
            z += " " + zahl(k.hl[c][0]) + " " + zahl(k.hl[c][1]) + " " + zahl(k.hr[c][0]) + " " + zahl(k.hr[c][1]);
        }
    } else {
        z += " a";
    }
}

struct Leser {
    const std::vector<std::string>& w;
    std::size_t i = 0;
    bool ok = true;
    [[nodiscard]] bool da() const { return i < w.size(); }
    std::string wort() {
        if (i >= w.size()) {
            ok = false;
            return {};
        }
        return w[i++];
    }
    int ganz() {
        const std::string s = wort();
        char* e = nullptr;
        const long v = std::strtol(s.c_str(), &e, 10);
        if (s.empty() || e == nullptr || *e != '\0') { ok = false; }
        return static_cast<int>(v);
    }
    float fl() {
        const std::string s = wort();
        char* e = nullptr;
        const float v = std::strtof(s.c_str(), &e);
        if (s.empty() || e == nullptr || *e != '\0') { ok = false; }
        return v;
    }
    double db() {
        const std::string s = wort();
        char* e = nullptr;
        const double v = std::strtod(s.c_str(), &e);
        if (s.empty() || e == nullptr || *e != '\0') { ok = false; }
        return v;
    }
};

bool schluesselLesen(Leser& l, RigSchluessel& k) {
    k.bild = l.ganz();
    const int ip = l.ganz();
    if (ip < 0 || ip > 2) { l.ok = false; }
    k.interp = static_cast<Interp>(std::clamp(ip, 0, 2));
    for (float& v : k.q) { v = l.fl(); }
    for (float& v : k.t) { v = l.fl(); }
    const std::string arten = l.wort();
    if (arten.size() != static_cast<std::size_t>(kKanaele) * 2U) {
        l.ok = false;
        return false;
    }
    for (int c = 0; c < kKanaele; ++c) {
        const int a = arten[static_cast<std::size_t>(c) * 2U] - '0';
        const int b = arten[static_cast<std::size_t>(c) * 2U + 1U] - '0';
        if (a < 0 || a > 4 || b < 0 || b > 4) { l.ok = false; }
        k.henkelL[c] = static_cast<Henkel>(std::clamp(a, 0, 4));
        k.henkelR[c] = static_cast<Henkel>(std::clamp(b, 0, 4));
    }
    const std::string art = l.wort();
    if (art == "w") {
        for (int c = 0; c < kKanaele; ++c) {
            k.hl[c][0] = l.fl();
            k.hl[c][1] = l.fl();
            k.hr[c][0] = l.fl();
            k.hr[c][1] = l.fl();
        }
    } else if (art != "a") {
        l.ok = false;
    }
    return l.ok;
}

// Die starre Lage (3x4) aus 12 Zahlen
std::string matrixText(const BoneMatrix& m) {
    std::string z;
    for (const auto& r : m.m) {
        for (const float v : r) { z += " " + zahl(v); }
    }
    return z;
}

// 4x4 zeilenweise (gpu::baueFigurWelt) als BoneMatrix
BoneMatrix ausFw(const float fw[16]) {
    BoneMatrix m;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 4; ++c) { m.m[r][c] = fw[r * 4 + c]; }
    }
    return m;
}

const AnimEntry* abschnitt(const std::vector<AnimEntry>& a, const std::string& name) {
    for (const AnimEntry& e : a) {
        if (e.name == name) { return &e; }
    }
    return nullptr;
}

// Das Bild f einer GLA samt Anhang: die GLA und das Bild darin.
const GlaAnimation* bildIn(const GlaAnimation& g, int& f) {
    const GlaAnimation* q = &g;
    while (f >= q->numFrames && q->anhang != nullptr) {
        f -= q->numFrames;
        q = q->anhang;
    }
    return f >= 0 && f < q->numFrames ? q : nullptr;
}

}  // namespace

// --- Kommentarzeilen ---------------------------------------------------------------------------------------------------

std::vector<std::string> figurZeilen(const FigurAnim& a) {
    std::vector<std::string> z;
    std::string m = std::string(kMarke) + zitat(a.figur) + " basis " + zitat(a.basis) + " bilder " + std::to_string(a.bilder) + " fps " +
                    std::to_string(a.fps) + " start " + zahl(a.startMs) + " platz " + zitat(a.platz) + " folge " + zitat(a.folge) + " warte " +
                    zitat(a.warteSignal) + " signal " + zitat(a.sendeSignal) + " origin " + (a.setOrigin ? "1" : "0") + " kino " +
                    (a.kino ? "1" : "0") + " binde " + (a.schicht.bindeBasis ? "1" : "0") + " interp " +
                    std::to_string(static_cast<int>(a.schicht.neuInterp));
    z.push_back(m);
    for (std::size_t b = 0; b < a.schicht.knochen.size(); ++b) {
        const auto& v = a.schicht.knochen[b];
        if (v.empty()) { continue; }
        std::string d = std::string(kDaten) + zitat(a.figur) + " k " + std::to_string(b) + " " + std::to_string(v.size());
        for (const RigSchluessel& k : v) { schluesselText(k, d); }
        z.push_back(d);
    }
    for (const IkSpur& sp : a.schicht.ik) {
        if (sp.ziel.empty()) { continue; }
        std::string d = std::string(kDaten) + zitat(a.figur) + " ik " + std::to_string(static_cast<int>(sp.art)) + " " + std::to_string(sp.ziel.size());
        for (std::size_t i = 0; i < sp.ziel.size(); ++i) {
            schluesselText(sp.ziel[i], d);
            const IkWert w = i < sp.wert.size() ? sp.wert[i] : IkWert{};
            d += " " + zahl(w.ik) + " " + std::to_string(w.raum) + " " + zahl(w.pol[0]) + " " + zahl(w.pol[1]) + " " + zahl(w.pol[2]) + " " +
                 zahl(w.rolle) + " " + zahl(w.spitze);
            for (const float q : w.zehen) { d += " " + zahl(q); }
        }
        z.push_back(d);
    }
    for (const FremdRaum& f : a.fremd) {
        z.push_back(std::string(kDaten) + zitat(a.figur) + " fremd " + std::to_string(static_cast<int>(f.art)) + " " + zitat(f.figur) + " " +
                    zitat(f.knochen) + " " + std::to_string(f.von) + " " + std::to_string(f.bis) + matrixText(f.versatz));
    }
    // Stufe 6: Auto-Physik (nur die Einstellungen; die Spur rechnet die Vorschau daraus)
    const PhysikEinstellung& e = a.schicht.physik.e;
    const PhysikEinstellung vorgabe;
    if (e.an || !e.ketten.empty() || e.absprung >= 0 || e.staerke != vorgabe.staerke || e.sprung != vorgabe.sprung || e.kontakte != vorgabe.kontakte ||
        e.federn != vorgabe.federn || e.kontaktAbstand != vorgabe.kontaktAbstand) {
        std::string d = std::string(kDaten) + zitat(a.figur) + " physik " + (e.an ? "1" : "0") + " " + zahl(e.staerke) + " " + (e.sprung ? "1" : "0") + " " +
                        (e.kontakte ? "1" : "0") + " " + (e.federn ? "1" : "0") + " " + std::to_string(e.absprung) + " " + std::to_string(e.landung) + " " +
                        zahl(e.kontaktAbstand) + " " + std::to_string(e.ketten.size());
        for (const PhysikKette& k : e.ketten) { d += " " + std::to_string(k.knochen) + " " + zahl(k.frequenz) + " " + zahl(k.daempfung); }
        z.push_back(d);
    }
    return z;
}

bool istFigurMarke(const std::string& zeile, std::string* figur) {
    const std::string z = ohneEinzug(zeile);
    if (!beginntMit(z, kMarke)) { return false; }
    const std::vector<std::string> w = woerter(z.substr(std::strlen(kMarke)));
    if (w.empty()) { return false; }
    if (figur != nullptr) { *figur = w[0]; }
    return true;
}

bool istFigurDaten(const std::string& zeile, std::string* figur) {
    const std::string z = ohneEinzug(zeile);
    if (!beginntMit(z, kDaten)) { return false; }
    const std::vector<std::string> w = woerter(z.substr(std::strlen(kDaten)));
    if (w.empty()) { return false; }
    if (figur != nullptr) { *figur = w[0]; }
    return true;
}

bool figurAusZeilen(const std::vector<std::string>& zeilen, const GlaAnimation& g, const Rig& r, FigurAnim& aus, std::string* fehler) {
    const auto falsch = [&](const std::string& f) {
        if (fehler != nullptr) { *fehler = f; }
        return false;
    };
    if (zeilen.empty() || !istFigurMarke(zeilen.front())) { return falsch("keine Marke"); }
    aus = FigurAnim{};
    {
        const std::string z = ohneEinzug(zeilen.front());
        const std::vector<std::string> w = woerter(z.substr(std::strlen(kMarke)));
        aus.figur = w[0];
        for (std::size_t i = 1; i + 1 < w.size(); i += 2) {
            const std::string& k = w[i];
            const std::string& v = w[i + 1];
            if (k == "basis") { aus.basis = v; }
            // pfix N3: animation_t numFrames ist unsigned short, NPC_stats.cpp:952-979 assert(< 65536)
            if (k == "bilder") { aus.bilder = std::clamp(std::atoi(v.c_str()), 1, 65535); }
            if (k == "fps") { aus.fps = std::atoi(v.c_str()); }
            if (k == "start") { aus.startMs = std::strtod(v.c_str(), nullptr); }
            if (k == "platz") { aus.platz = v; }
            if (k == "folge") { aus.folge = v; }
            if (k == "warte") { aus.warteSignal = v; }
            if (k == "signal") { aus.sendeSignal = v; }
            if (k == "origin") { aus.setOrigin = v != "0"; }
            if (k == "kino") { aus.kino = v != "0"; }
            if (k == "binde") { aus.schicht.bindeBasis = v != "0"; }
            if (k == "interp") { aus.schicht.neuInterp = static_cast<Interp>(std::clamp(std::atoi(v.c_str()), 0, 2)); }
        }
        if (aus.fps == 0) { aus.fps = 20; }
    }
    aus.schicht.knochen.assign(g.bones.size(), {});
    for (std::size_t zi = 1; zi < zeilen.size(); ++zi) {
        std::string fig;
        if (!istFigurDaten(zeilen[zi], &fig) || fig != aus.figur) { continue; }
        const std::string z = ohneEinzug(zeilen[zi]);
        const std::vector<std::string> w = woerter(z.substr(std::strlen(kDaten)));
        Leser l{w, 1};
        const std::string art = l.wort();
        if (art == "k") {
            const int b = l.ganz();
            const int n = l.ganz();
            if (!l.ok || b < 0 || static_cast<std::size_t>(b) >= g.bones.size() || n < 0) { return falsch("Zeile " + std::to_string(zi) + ": Knochen"); }
            // pfix N3: Schluessel fuer Schluessel lesen (wie ik) - die Zahl aus dem Text legt keinen Speicher an
            std::vector<RigSchluessel> v;
            for (int i = 0; i < n; ++i) {
                RigSchluessel k;
                if (!schluesselLesen(l, k)) { return falsch("Zeile " + std::to_string(zi) + ": Schluessel"); }
                v.push_back(k);
            }
            henkelNeu(v);
            aus.schicht.knochen[static_cast<std::size_t>(b)] = std::move(v);
        } else if (art == "ik") {
            const int a = l.ganz();
            const int n = l.ganz();
            if (!l.ok || a < 0 || a >= kIkArten || n < 0) { return falsch("Zeile " + std::to_string(zi) + ": IK"); }
            IkSpur sp;
            const bool angelegt = !r.leer() && ikSpurAnlegen(g, r, static_cast<IkArt>(a), sp);
            sp.art = static_cast<IkArt>(a);
            for (int i = 0; i < n; ++i) {
                RigSchluessel k;
                IkWert v;
                if (!schluesselLesen(l, k)) { return falsch("Zeile " + std::to_string(zi) + ": IK-Schluessel"); }
                v.ik = l.fl();
                v.raum = l.ganz();
                for (float& p : v.pol) { p = l.fl(); }
                v.rolle = l.fl();
                v.spitze = l.fl();
                for (float& q : v.zehen) { q = l.fl(); }
                if (!l.ok) { return falsch("Zeile " + std::to_string(zi) + ": IK-Wert"); }
                sp.ziel.push_back(k);
                sp.wert.push_back(v);
            }
            if (!angelegt) { continue; }   // ohne Rig keine Kette (die Zeile bleibt im Skript stehen)
            ikHenkelNeu(sp);
            aus.schicht.ik.push_back(std::move(sp));
        } else if (art == "fremd") {
            FremdRaum f;
            const int a = l.ganz();
            f.art = static_cast<IkArt>(std::clamp(a, 0, kIkArten - 1));
            f.figur = l.wort();
            f.knochen = l.wort();
            f.von = l.ganz();
            f.bis = l.ganz();
            for (auto& rr : f.versatz.m) {
                for (float& v : rr) { v = l.fl(); }
            }
            if (!l.ok || a < 0 || a >= kIkArten) { return falsch("Zeile " + std::to_string(zi) + ": fremd"); }
            aus.fremd.push_back(f);
        } else if (art == "physik") {
            PhysikEinstellung& e = aus.schicht.physik.e;
            e.an = l.ganz() != 0;
            e.staerke = l.fl();
            e.sprung = l.ganz() != 0;
            e.kontakte = l.ganz() != 0;
            e.federn = l.ganz() != 0;
            e.absprung = l.ganz();
            e.landung = l.ganz();
            e.kontaktAbstand = l.fl();
            const int n = l.ganz();
            for (int i = 0; i < n && l.ok; ++i) {
                PhysikKette k;
                k.knochen = l.ganz();
                k.frequenz = l.fl();
                k.daempfung = l.fl();
                if (k.knochen >= 0 && static_cast<std::size_t>(k.knochen) < g.bones.size()) { e.ketten.push_back(k); }
            }
            if (!l.ok || n < 0) { return falsch("Zeile " + std::to_string(zi) + ": physik"); }
        }
    }
    return true;
}

// --- Grundanimation -----------------------------------------------------------------------------------------------------

double figurBildMs(const FigurAnim& a) {
    AnimEntry e;
    e.fps = a.fps;
    return animBildMs(e);
}

int figurDauerMs(const FigurAnim& a) { return static_cast<int>(static_cast<double>(a.bilder) * figurBildMs(a) + 0.5); }

bool basisGla(const GlaAnimation& skelett, const std::vector<AnimEntry>& abschnitte, const std::string& basis, int bilder, int fps, GlaAnimation& aus,
              std::string* fehler) {
    aus = GlaAnimation{};
    if (skelett.bones.empty() || bilder <= 0) {
        if (fehler != nullptr) { *fehler = fehlerText(ModellFehler{FehlerArt::KeinSkelett}); }
        return false;
    }
    if (abschnitt(abschnitte, basis) == nullptr) {
        if (fehler != nullptr) { *fehler = fehlerText(ModellFehler{FehlerArt::GrundanimFehlt, basis}); }
        return false;
    }
    AnimEntry takt;
    takt.fps = fps;
    const double lerp = animBildMs(takt);
    aus.name = skelett.name;
    aus.skeletonName = skelett.skeletonName;
    aus.dateiName = skelett.dateiName;
    aus.scale = skelett.scale;
    aus.bones = skelett.bones;
    const std::size_t nb = aus.bones.size();
    GlaVorrat vorrat(aus.bonePool);   // REST N5
    aus.indexes.reserve(static_cast<std::size_t>(bilder) * nb);
    for (int f = 0; f < bilder; ++f) {
        int q = frameForAnimationIn(abschnitte, basis, static_cast<double>(f) * lerp);
        const GlaAnimation* g = bildIn(skelett, q);
        if (g == nullptr || g->bones.size() != nb) {
            if (fehler != nullptr) { *fehler = fehlerText(ModellFehler{FehlerArt::GrundbildFehlt}); }
            aus = GlaAnimation{};
            return false;
        }
        for (std::size_t b = 0; b < nb; ++b) {
            const std::uint32_t qi = g->indexes[static_cast<std::size_t>(q) * nb + b];
            if (static_cast<std::size_t>(qi) * 14U + 14U > g->bonePool.size()) {
                if (fehler != nullptr) { *fehler = fehlerText(ModellFehler{FehlerArt::VerweisHinterVorrat}); }
                aus = GlaAnimation{};
                return false;
            }
            aus.indexes.push_back(vorrat.dazu(g->bonePool.data() + static_cast<std::size_t>(qi) * 14U));
        }
        ++aus.numFrames;
    }
    if (vorrat.zuGross()) {   // REST N5: die Grenze der anderen Schreiber (mdxaIndex_t, 3 Bytes)
        if (fehler != nullptr) { *fehler = fehlerText(ModellFehler{FehlerArt::VorratZuGross}); }
        aus = GlaAnimation{};
        return false;
    }
    return true;
}

// --- Wurzelweg ----------------------------------------------------------------------------------------------------------

std::array<float, 3> wurzelVersatz(const GlaAnimation& basis, const RigSchicht& s, int becken, const GlaAnimation& skelett, int folgeBild) {
    std::array<float, 3> d{0.0F, 0.0F, 0.0F};
    if (basis.empty() || becken < 0 || static_cast<std::size_t>(becken) >= basis.bones.size()) { return d; }
    std::vector<BoneMatrix> ende;
    rigWelt(basis, s, basis.numFrames - 1, basis.numFrames - 1, 0.0F, ende);
    std::vector<BoneMatrix> folge;
    if (folgeBild >= 0 && folgeBild < skelett.bildZahl()) {
        skelett.worldMatrices(folgeBild, folge);
    }
    const int bf = boneIndex(skelett, basis.bones[static_cast<std::size_t>(becken)].name);
    if (static_cast<std::size_t>(becken) >= ende.size() || bf < 0 || static_cast<std::size_t>(bf) >= folge.size()) { return d; }
    for (int c = 0; c < 2; ++c) {
        d[static_cast<std::size_t>(c)] = ende[static_cast<std::size_t>(becken)].m[c][3] - folge[static_cast<std::size_t>(bf)].m[c][3];
    }
    return d;
}

std::array<float, 3> setOriginOrt(const float start[3], const float fw[16], const std::array<float, 3>& v) {
    std::array<float, 3> o{};
    for (int r = 0; r < 3; ++r) {
        o[static_cast<std::size_t>(r)] = start[r] + fw[r * 4] * v[0] + fw[r * 4 + 1] * v[1] + fw[r * 4 + 2] * v[2];
    }
    return o;
}

std::vector<std::array<float, 3>> wurzelPfad(const GlaAnimation& basis, const RigSchicht& s, int becken, const float fw[16]) {
    std::vector<std::array<float, 3>> p;
    if (basis.empty() || becken < 0 || static_cast<std::size_t>(becken) >= basis.bones.size()) { return p; }
    std::vector<BoneMatrix> w;
    for (int f = 0; f < basis.numFrames; ++f) {
        rigWelt(basis, s, f, f, 0.0F, w);
        const BoneMatrix& m = w[static_cast<std::size_t>(becken)];
        std::array<float, 3> o{};
        for (int r = 0; r < 3; ++r) {
            o[static_cast<std::size_t>(r)] = fw[r * 4] * m.m[0][3] + fw[r * 4 + 1] * m.m[1][3] + fw[r * 4 + 2] * m.m[2][3] + fw[r * 4 + 3];
        }
        p.push_back(o);
    }
    return p;
}

// --- Fremde Raeume ------------------------------------------------------------------------------------------------------

int fremdeEinrechnen(const GlaAnimation& basis, const FigurLage& selbst, const std::vector<FigurLage>& andere, RigSchicht& s) {
    if (selbst.anim == nullptr || basis.empty()) { return 0; }
    int gesetzt = 0;
    const double lerp = figurBildMs(*selbst.anim);
    const BoneMatrix eigeneInv = starrInvers(ausFw(selbst.fw));
    for (const FremdRaum& fr : selbst.anim->fremd) {
        const int spur = ikSpurIndex(s, fr.art);
        const FigurLage* a = nullptr;
        for (const FigurLage& x : andere) {
            if (x.anim != nullptr && x.anim->figur == fr.figur) { a = &x; }
        }
        if (spur < 0 || a == nullptr || a->basis == nullptr || a->basis->empty()) { continue; }
        const int k = boneIndex(*a->basis, fr.knochen);
        if (k < 0) { continue; }
        const double lerpA = figurBildMs(*a->anim);
        std::vector<BoneMatrix> w;
        for (int f = std::max(0, fr.von); f <= std::min(fr.bis, basis.numFrames - 1); ++f) {
            // die Zeit des Bilds, dort das Bild der anderen Figur (zwischen zwei Bildern gemischt wie die Anzeige)
            const double t = selbst.startMs + static_cast<double>(f) * lerp;
            const double fa = std::clamp((t - a->startMs) / lerpA, 0.0, static_cast<double>(a->basis->numFrames - 1));
            const int f0 = static_cast<int>(std::floor(fa));
            const int f1 = std::min(f0 + 1, a->basis->numFrames - 1);
            rigWelt(*a->basis, a->anim->schicht, f0, f1, static_cast<float>(fa - f0), w);
            const BoneMatrix ziel = eigeneInv * ausFw(a->fw) * starr(w[static_cast<std::size_t>(k)]) * fr.versatz;
            ikZielSetzen(basis, s, spur, f, ziel);
            ++gesetzt;
        }
    }
    return gesetzt;
}

// --- ICARUS -------------------------------------------------------------------------------------------------------------

bool figurNameGueltig(const std::string& name) { return name.find_first_of("\"\r\n") == std::string::npos; }

std::vector<Node> figurBlock(const FigurAnim& a, const std::array<float, 3>& u) {
    for (const std::string* n : {&a.figur, &a.basis, &a.platz, &a.folge, &a.warteSignal, &a.sendeSignal}) {
        if (!figurNameGueltig(*n)) { return {}; }
    }
    const std::vector<std::string> zeilen = figurZeilen(a);
    std::string text;
    for (const std::string& z : zeilen) { text += z + "\n"; }
    text += "affect ( \"" + a.figur + "\", /*@AFFECT_TYPE*/ FLUSH )\n{\n";
    char b[160];
    if (a.kino) { text += "\tset ( /*@SET_TYPES*/ \"SET_BEHAVIOR_STATE\", /*@BSTATE_STRINGS*/ \"BS_CINEMATIC\" );\n"; }
    if (a.startMs > 0.5) {
        std::snprintf(b, sizeof(b), "\twait ( %.3f );\n", a.startMs);
        text += b;
    }
    if (!a.warteSignal.empty()) { text += "\twaitsignal ( \"" + a.warteSignal + "\" );\n"; }
    const int dauer = figurDauerMs(a);
    text += "\tset ( /*@SET_TYPES*/ \"SET_ANIM_BOTH\", /*@ANIM_NAMES*/ \"" + a.platz + "\" );\n";
    std::snprintf(b, sizeof(b), "\tset ( /*@SET_TYPES*/ \"SET_ANIM_HOLDTIME_BOTH\", %d );\n", dauer);
    text += b;
    std::snprintf(b, sizeof(b), "\twait ( %.3f );\n", static_cast<double>(dauer));
    text += b;
    if (a.setOrigin) {
        std::snprintf(b, sizeof(b), "\tset ( /*@SET_TYPES*/ \"SET_ORIGIN\", < %.3f %.3f %.3f > );\n", static_cast<double>(u[0]), static_cast<double>(u[1]),
                      static_cast<double>(u[2]));
        text += b;
    }
    if (!a.folge.empty()) { text += "\tset ( /*@SET_TYPES*/ \"SET_ANIM_BOTH\", /*@ANIM_NAMES*/ \"" + a.folge + "\" );\n"; }
    if (!a.sendeSignal.empty()) { text += "\tsignal ( \"" + a.sendeSignal + "\" );\n"; }
    text += "}\n";
    Script s;
    std::vector<Diag> diag;
    if (!readScript(text, s, diag)) { return {}; }
    // die Kommentarzeilen wortgetreu (wie baueRoffBlock)
    std::size_t zi = 0;
    for (Node& n : s.nodes) {
        if (n.kind == Node::Kind::LineComment && zi < zeilen.size()) { n.raw = zeilen[zi++]; }
    }
    return s.nodes;
}

std::optional<std::size_t> findeFigurBlock(const Script& s, const std::string& figur) {
    for (std::size_t i = 0; i < s.nodes.size(); ++i) {
        std::string f;
        if (s.nodes[i].kind == Node::Kind::LineComment && istFigurMarke(s.nodes[i].raw, &f) && f == figur) { return i; }
    }
    return std::nullopt;
}

namespace {
// Das Ende des Blocks ab der Marke: Datenzeilen der Figur, dann der affect (einschliesslich). -1 ohne affect.
std::size_t blockEnde(const Script& s, std::size_t marke, const std::string& figur) {
    std::size_t j = marke + 1;
    std::string f;
    while (j < s.nodes.size() && s.nodes[j].kind == Node::Kind::LineComment && istFigurDaten(s.nodes[j].raw, &f) && f == figur) { ++j; }
    if (j < s.nodes.size() && s.nodes[j].kind == Node::Kind::Command && s.nodes[j].name == "affect" && !s.nodes[j].args.empty() &&
        s.nodes[j].args[0].text == figur) {
        return j + 1;
    }
    return j;
}
}  // namespace

std::vector<std::string> figurenImSkript(const Script& s) {
    std::vector<std::string> n;
    for (const Node& x : s.nodes) {
        std::string f;
        if (x.kind == Node::Kind::LineComment && istFigurMarke(x.raw, &f) && std::find(n.begin(), n.end(), f) == n.end()) { n.push_back(f); }
    }
    return n;
}

std::vector<std::string> figurZeilenImSkript(const Script& s, const std::string& figur) {
    std::vector<std::string> z;
    const auto i = findeFigurBlock(s, figur);
    if (!i) { return z; }
    z.push_back(s.nodes[*i].raw);
    std::string f;
    for (std::size_t j = *i + 1; j < s.nodes.size() && s.nodes[j].kind == Node::Kind::LineComment && istFigurDaten(s.nodes[j].raw, &f) && f == figur;
         ++j) {
        z.push_back(s.nodes[j].raw);
    }
    return z;
}

Script mitFigurBlock(const Script& s, const std::string& figur, const FigurAnim* a, const std::array<float, 3>& u) {
    // pfix M4: ein Block, der nicht zu schreiben ist (figurBlock leer), ersetzt den alten nicht - das Skript bleibt
    std::vector<Node> block;
    if (a != nullptr) {
        block = figurBlock(*a, u);
        if (block.empty()) { return s; }
    }
    Script neu = s;
    std::size_t at = 0;
    if (const auto alt = findeFigurBlock(neu, figur)) {
        at = *alt;
        const std::size_t ende = blockEnde(neu, at, figur);
        neu.nodes.erase(neu.nodes.begin() + static_cast<std::ptrdiff_t>(at), neu.nodes.begin() + static_cast<std::ptrdiff_t>(ende));
    } else {
        // nach den anderen Figurenbloecken (die Reihenfolge, in der sie angelegt wurden)
        for (std::size_t i = 0; i < neu.nodes.size(); ++i) {
            std::string f;
            if (neu.nodes[i].kind == Node::Kind::LineComment && istFigurMarke(neu.nodes[i].raw, &f)) { at = blockEnde(neu, i, f); }
        }
    }
    if (a != nullptr) {
        neu.nodes.insert(neu.nodes.begin() + static_cast<std::ptrdiff_t>(at), block.begin(), block.end());
    }
    return neu;
}

// --- Bauen --------------------------------------------------------------------------------------------------------------

std::vector<std::string> plaetzeVerteilen(const std::vector<const FigurAnim*>& figuren, const std::vector<std::string>& belegt, Zielspiel s,
                                          std::string* fehler) {
    const auto klein = [](std::string x) {
        for (char& c : x) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return x;
    };
    std::set<std::string> vergeben;
    std::vector<std::string> aus(figuren.size());
    std::vector<std::string> namen;
    for (const Platz& p : kinoPlaetze(s, {})) { namen.push_back(p.name); }
    // erst die gemerkten (ihr Platz gehoert ihnen, auch wenn die vorhandene Karten-GLA ihn schon hat - es ist ihr letzter Bau)
    for (std::size_t i = 0; i < figuren.size(); ++i) {
        const std::string p = figuren[i]->platz;
        if (p.empty() || std::none_of(namen.begin(), namen.end(), [&](const std::string& n) { return klein(n) == klein(p); }) ||
            vergeben.count(klein(p)) != 0U) {
            continue;
        }
        aus[i] = p;
        vergeben.insert(klein(p));
    }
    std::set<std::string> fremd;
    for (const std::string& b : belegt) { fremd.insert(klein(b)); }
    for (std::size_t i = 0; i < figuren.size(); ++i) {
        if (!aus[i].empty()) { continue; }
        for (const std::string& n : namen) {
            if (vergeben.count(klein(n)) == 0U && fremd.count(klein(n)) == 0U) {
                aus[i] = n;
                vergeben.insert(klein(n));
                break;
            }
        }
        if (aus[i].empty()) {
            if (fehler != nullptr) { *fehler = fehlerText(ModellFehler{FehlerArt::KeinKinoPlatz}); }
            return {};
        }
    }
    return aus;
}

bool figurenBacken(const std::vector<FigurBauEingabe>& eingaben, const std::vector<std::string>& plaetze, const GlaAnimation* vorhanden,
                   const std::vector<AnimEntry>& vorhandenCfg, Zielspiel s, std::vector<GlaAnimation>& gebacken, std::vector<Platz>& aus,
                   FigurBauBericht* bericht) {
    FigurBauBericht leer;
    FigurBauBericht& br = bericht != nullptr ? *bericht : leer;
    br = FigurBauBericht{};
    aus.clear();
    gebacken.clear();
    if (plaetze.size() != eingaben.size()) {
        br.fehler = fehlerText(ModellFehler{FehlerArt::PlaetzeFiguren});
        return false;
    }
    gebacken.resize(eingaben.size());   // feste Adressen: die Plaetze zeigen hinein
    for (std::size_t i = 0; i < eingaben.size(); ++i) {
        const FigurBauEingabe& e = eingaben[i];
        if (e.anim == nullptr || e.basis == nullptr || e.basis->empty()) {
            br.fehler = fehlerText(ModellFehler{FehlerArt::FigurOhneGrund});
            return false;
        }
        BackBericht bb;
        if (!rigBacken(*e.basis, e.schicht, 0, e.basis->numFrames, false, e.basis->skeletonName, gebacken[i], &bb)) {
            br.fehler = e.anim->figur + ": " + bb.fehler;
            return false;
        }
    }
    const auto klein = [](std::string x) {
        for (char& c : x) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        return x;
    };
    // die Reihenfolge der Zeilen: die Plaetze BOTH_CIN_1..n, dann die uebrigen Zeilen der vorhandenen Karten-GLA
    std::vector<std::string> namen;
    for (const Platz& p : kinoPlaetze(s, {})) { namen.push_back(p.name); }
    for (const AnimEntry& e : vorhandenCfg) {
        if (std::none_of(namen.begin(), namen.end(), [&](const std::string& n) { return klein(n) == klein(e.name); })) { namen.push_back(e.name); }
    }
    for (const std::string& n : namen) {
        Platz p;
        p.name = n;
        for (std::size_t i = 0; i < plaetze.size(); ++i) {
            if (klein(plaetze[i]) != klein(n)) { continue; }
            p.belegt = true;
            p.quelle = PlatzQuelle{&gebacken[i], eingaben[i].anim->figur, AnimEntry{n, 0, gebacken[i].numFrames, -1, eingaben[i].anim->fps}};
        }
        if (!p.belegt && vorhanden != nullptr) {
            for (const AnimEntry& e : vorhandenCfg) {
                if (klein(e.name) != klein(n) || e.numFrames <= 0) { continue; }
                p.belegt = true;
                p.quelle = PlatzQuelle{vorhanden, vorhanden->skeletonName, e};
                ++br.vorhandenBehalten;
            }
        }
        aus.push_back(std::move(p));
    }
    br.plaetze = plaetze;
    return true;
}

}  // namespace bhed::modell
