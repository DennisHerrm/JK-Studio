// modell_cwvergleich.cpp - CW-MODELL (15.11.2026): Speichern, Export und Laden mit den Vorgaben bytegleich wie auf karte1 13ea0a9bd
//
// shank 14.11.: "das ist wichtig das es zwar drin ist aber nicht standard die clone wars aenderung von meine bruder" - die neuen
// Werkzeuge des Model-Tabs (Detailstufen, Bolts, eigene Skelette) wirken nur beim Aufruf. Dieses Werkzeug benutzt nur Funktionen,
// die es auf 13ea0a9bd schon gab, und schreibt je Datei eine Zeile:
//
//   <name> <art> <groesse> ein=<fnv64> laden=<fnv64> aus=<fnv64> <gleich|ANDERS>
//
//   ein     die gelesenen Bytes
//   laden   das Dokument nach dem Laden (ausGlm/ausMd3, je Stufe zuGlm: Ecken, Gewichte, Dreiecke, Namen, Flaggen)
//   aus     was Speichern schreibt (glmSchreiben, md3Schreiben mit der Grenze von Jedi Academy SP, schreibGla, animCfgSchreiben)
//   gleich  aus == ein
//
// Eingaben: Dateien (.glm .md3 .gla .cfg) oder .pk3 (darin alle .glm, .md3, .gla und animation.cfg, nach Namen sortiert). Dazu
// immer eingebaute Proben (eine .md3 mit 2 Bildern und einem Tag, eine .gla mit 3 Knochen und 4 Bildern, zwei animation.cfg), damit
// der ctest ohne Spieldaten MD3, GLA und animation.cfg abdeckt.
//
// Gegen den Kern von 13ea0a9bd uebersetzt (Y:\werkzeuge\cwmodell\basis) und gegen den dieses Zweigs: gleiche Zeilen = gleiche Bytes.
// Mit --soll <datei> vergleicht das Werkzeug selbst (ctest modell_cwvergleich, Soll tests/modell/cwvergleich_soll.txt).
//
// Aufruf: modellcwvergleich <ausgabe.txt> [--soll <datei>] <eingabe> ...
#include "bhed/gla.h"
#include "bhed/glm.h"
#include "bhed/md3.h"
#include "bhed/modell/animcfg.h"
#include "bhed/modell/dokument.h"
#include "bhed/modell/g2.h"
#include "bhed/modell/glmdatei.h"
#include "bhed/modell/md3datei.h"
#include "bhed/pk3.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

using namespace bhed;
using namespace bhed::modell;

std::uint64_t fnv(const void* p, std::size_t n, std::uint64_t h = 1469598103934665603ULL) {
    const auto* b = static_cast<const unsigned char*>(p);
    for (std::size_t i = 0; i < n; ++i) {
        h ^= b[i];
        h *= 1099511628211ULL;
    }
    return h;
}
std::uint64_t fnv(const std::string& s, std::uint64_t h = 1469598103934665603ULL) { return fnv(s.data(), s.size(), h); }

std::string hex(std::uint64_t h) {
    char b[24];
    std::snprintf(b, sizeof b, "%016llx", static_cast<unsigned long long>(h));
    return b;
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

bool endetMit(const std::string& s, const std::string& e) { return s.size() >= e.size() && klein(s).compare(s.size() - e.size(), e.size(), e) == 0; }

std::string slurp(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

// Das gezeichnete Modell einer Stufe als Pruefsumme (was Laden ins Dokument legt).
std::uint64_t modellSumme(const GlmModel& m, std::uint64_t h) {
    h = fnv(m.name, h);
    h = fnv(m.animFile, h);
    h = fnv(&m.numBones, sizeof m.numBones, h);
    h = fnv(&m.numLods, sizeof m.numLods, h);
    for (const GlmSurface& s : m.surfaces) {
        h = fnv(s.name, h);
        h = fnv(s.shader, h);
        h = fnv(&s.flags, sizeof s.flags, h);
        h = fnv(&s.parent, sizeof s.parent, h);
        for (const GlmVertex& v : s.verts) { h = fnv(&v, sizeof v, h); }
        if (!s.indexes.empty()) { h = fnv(s.indexes.data(), s.indexes.size() * sizeof(std::uint32_t), h); }
        for (const GlmVertex& v : s.grobVerts) { h = fnv(&v, sizeof v, h); }
        if (!s.grobIndexes.empty()) { h = fnv(s.grobIndexes.data(), s.grobIndexes.size() * sizeof(std::uint32_t), h); }
    }
    return h;
}

std::uint64_t inhaltSumme(const Inhalt& in) {
    std::uint64_t h = fnv(in.name);
    const int stufen = std::max(in.stufenZahl, 1);
    h = fnv(&stufen, sizeof stufen, h);
    for (int s = 0; s < stufen; ++s) { h = modellSumme(zuGlm(in, s), h); }
    return h;
}

// --- eingebaute Proben ---------------------------------------------------------------------------------------------
void i32(std::string& b, std::int32_t v) { b.append(reinterpret_cast<const char*>(&v), 4); }
void f32(std::string& b, float v) { b.append(reinterpret_cast<const char*>(&v), 4); }
void i16(std::string& b, std::int16_t v) { b.append(reinterpret_cast<const char*>(&v), 2); }
void name(std::string& b, const std::string& n, std::size_t laenge) {
    std::string x = n;
    x.resize(laenge, '\0');
    b += x;
}
void setze(std::string& b, std::size_t at, std::int32_t v) { std::memcpy(b.data() + at, &v, 4); }

// qfiles.h: md3Header_t 108, md3Frame_t 56, md3Tag_t 112, md3Surface_t 108, md3Shader_t 68, Dreieck 12, St 8, XyzNormal 8.
std::string probeMd3() {
    constexpr int n = 5;   // 5 x 5 Ecken
    constexpr int bilder = 2;
    std::string b;
    b += "IDP3";
    i32(b, 15);
    name(b, "models/map_objects/cw/probe.md3", 64);
    i32(b, 0);         // flags
    i32(b, bilder);    // numFrames
    i32(b, 1);         // numTags
    i32(b, 1);         // numSurfaces
    i32(b, 0);         // numSkins
    const std::size_t kopf = b.size();
    for (int k = 0; k < 4; ++k) { i32(b, 0); }   // ofsFrames, ofsTags, ofsSurfaces, ofsEnd
    setze(b, kopf, static_cast<std::int32_t>(b.size()));
    for (int f = 0; f < bilder; ++f) {
        f32(b, 0); f32(b, 0); f32(b, 0);
        f32(b, 64); f32(b, 64); f32(b, 8.0F * static_cast<float>(f + 1));
        f32(b, 0); f32(b, 0); f32(b, 0);
        f32(b, 90.0F);
        name(b, "frame" + std::to_string(f), 16);
    }
    setze(b, kopf + 4, static_cast<std::int32_t>(b.size()));
    for (int f = 0; f < bilder; ++f) {
        name(b, "tag_flash", 64);
        f32(b, 32); f32(b, 32); f32(b, 4.0F + static_cast<float>(f));
        f32(b, 1); f32(b, 0); f32(b, 0);
        f32(b, 0); f32(b, 1); f32(b, 0);
        f32(b, 0); f32(b, 0); f32(b, 1);
    }
    setze(b, kopf + 8, static_cast<std::int32_t>(b.size()));
    const std::size_t fl = b.size();
    b += "IDP3";
    name(b, "probe", 64);
    i32(b, 0);
    i32(b, bilder);
    i32(b, 1);
    i32(b, n * n);
    i32(b, (n - 1) * (n - 1) * 2);
    const std::size_t ofs = b.size();
    for (int k = 0; k < 5; ++k) { i32(b, 0); }
    setze(b, ofs + 4, static_cast<std::int32_t>(b.size() - fl));   // Shader
    name(b, "models/map_objects/cw/probe", 64);
    i32(b, 0);
    setze(b, ofs, static_cast<std::int32_t>(b.size() - fl));       // Dreiecke
    for (int y = 0; y + 1 < n; ++y) {
        for (int x = 0; x + 1 < n; ++x) {
            const int a = y * n + x;
            i32(b, a); i32(b, a + 1); i32(b, a + n + 1);
            i32(b, a); i32(b, a + n + 1); i32(b, a + n);
        }
    }
    setze(b, ofs + 8, static_cast<std::int32_t>(b.size() - fl));   // St
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            f32(b, static_cast<float>(x) / (n - 1));
            f32(b, static_cast<float>(y) / (n - 1));
        }
    }
    setze(b, ofs + 12, static_cast<std::int32_t>(b.size() - fl));  // XyzNormal je Bild
    for (int f = 0; f < bilder; ++f) {
        for (int y = 0; y < n; ++y) {
            for (int x = 0; x < n; ++x) {
                i16(b, static_cast<std::int16_t>(x * 16 * 64));
                i16(b, static_cast<std::int16_t>(y * 16 * 64));
                i16(b, static_cast<std::int16_t>((f * (x + y)) * 64));
                i16(b, 0);   // Normale nach oben (lat 0)
            }
        }
    }
    setze(b, ofs + 16, static_cast<std::int32_t>(b.size() - fl));  // ofsEnd der Flaeche
    setze(b, kopf + 12, static_cast<std::int32_t>(b.size()));
    return b;
}

BoneMatrix verschoben(float x) {
    BoneMatrix m = BoneMatrix::identity();
    m.m[0][3] = x;
    return m;
}

// wie tests/modell_dateien.cpp probeGla
GlaAnimation probeGla() {
    GlaAnimation a;
    a.skeletonName = "models/players/_probe/_probe";
    a.dateiName = a.skeletonName;
    a.dateiName.resize(64, '\0');
    a.dateiName[40] = 'X';
    a.scale = 1.0F;
    a.numFrames = 4;
    for (int b = 0; b < 3; ++b) {
        GlaBone k;
        k.name = b == 0 ? "model_root" : b == 1 ? "pelvis" : "lower_lumbar";
        k.parent = b - 1;
        k.flags = b == 0 ? 1U : 0U;
        k.basePose = verschoben(2.0F * static_cast<float>(b));
        k.basePoseInv = verschoben(-2.0F * static_cast<float>(b));
        if (b < 2) { k.kinder.push_back(b + 1); }
        a.bones.push_back(k);
    }
    for (int f = 0; f < 4; ++f) {
        for (int b = 0; b < 3; ++b) {
            const std::uint32_t idx = static_cast<std::uint32_t>(a.bonePool.size() / 14U);
            if (b == 0 && f > 0) {
                a.indexes.push_back(a.indexes[0]);
                continue;
            }
            for (int k = 0; k < 14; ++k) { a.bonePool.push_back(static_cast<std::uint8_t>(1 + f * 3 + b + k)); }
            a.indexes.push_back(idx);
        }
    }
    return a;
}

// --- eine Datei ------------------------------------------------------------------------------------------------------
std::string zeile(const std::string& n, const std::string& bytes) {
    std::string art = "?";
    std::uint64_t laden = 0;
    std::string aus;
    bool ok = false;
    if (endetMit(n, ".glm")) {
        art = "glm";
        Inhalt in;
        if (ausGlm(bytes, in)) {
            laden = inhaltSumme(in);
            ok = glmSchreiben(in, aus);
        }
    } else if (endetMit(n, ".md3")) {
        art = "md3";
        Md3Model m;
        if (readMd3(bytes, m)) {
            Inhalt in;
            ausMd3(m, in);
            in.md3Datei = std::make_shared<const std::string>(bytes);   // wie das Laden im Model-Tab (app_modelltab_uv.cpp)
            laden = inhaltSumme(in);
            ok = md3Schreiben(in, aus, grenzen(Zielspiel::JkaSp).eckenJeFlaeche);
        }
    } else if (endetMit(n, ".gla")) {
        art = "gla";
        GlaAnimation a;
        if (readGla(bytes, a)) {
            std::uint64_t h = fnv(a.skeletonName);
            for (const GlaBone& k : a.bones) {
                h = fnv(k.name, h);
                h = fnv(&k.parent, sizeof k.parent, h);
                h = fnv(&k.basePose, sizeof k.basePose, h);
            }
            h = fnv(&a.numFrames, sizeof a.numFrames, h);
            if (!a.bonePool.empty()) { h = fnv(a.bonePool.data(), a.bonePool.size(), h); }
            if (!a.indexes.empty()) { h = fnv(a.indexes.data(), a.indexes.size() * sizeof(std::uint32_t), h); }
            laden = h;
            ok = schreibGla(a, aus);
        }
    } else if (endetMit(n, ".cfg")) {
        art = "cfg";
        const AnimCfg c = animCfgLesen(bytes);
        std::uint64_t h = fnv(&c.schlussUmbruch, 1);
        for (const AnimEntry& e : c.eintraege()) {
            h = fnv(e.name, h);
            h = fnv(&e.firstFrame, sizeof e.firstFrame, h);
            h = fnv(&e.numFrames, sizeof e.numFrames, h);
            h = fnv(&e.loopFrame, sizeof e.loopFrame, h);
            h = fnv(&e.fps, sizeof e.fps, h);
        }
        laden = h;
        aus = animCfgSchreiben(c);
        ok = true;
    }
    char b[700];
    std::snprintf(b, sizeof b, "%s %s %zu ein=%s laden=%s aus=%s %s", n.c_str(), art.c_str(), bytes.size(), hex(fnv(bytes)).c_str(),
                  hex(laden).c_str(), ok ? hex(fnv(aus)).c_str() : "-", !ok ? "FEHLER" : aus == bytes ? "gleich" : "ANDERS");
    return b;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "Aufruf: modellcwvergleich <ausgabe.txt> [--soll <datei>] <eingabe> ...\n");
        return 2;
    }
    std::vector<std::string> zeilen;
    std::string soll;
    {
        std::string md3 = probeMd3();
        zeilen.push_back(zeile("probe/cw_probe.md3", md3));
        std::string gla;
        if (schreibGla(probeGla(), gla)) { zeilen.push_back(zeile("probe/cw_probe.gla", gla)); }
        // eine animation.cfg wie Raven sie schreibt (CRLF, Kommentar, Tabulatoren) und eine eigene fuer ein Fahrzeug
        zeilen.push_back(zeile("probe/cw_probe_animation.cfg", "// CW-MODELL Probe\r\nBOTH_STAND1         \t0\t40\t0\t20\r\n"
                                                                "BOTH_RUN1           \t40\t16\t0\t20\r\n\r\nBOTH_DEATH1\t56\t30\t-1\t15\r\n"));
        zeilen.push_back(zeile("probe/cw_fahrzeug_animation.cfg", "BOTH_VS_IDLE 0 1 0 20\nBOTH_WALK1 1 20 0 20\nNICHT_IM_SPIEL 21 4 0 20"));
    }
    for (int i = 2; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--soll" && i + 1 < argc) {
            soll = argv[++i];
            continue;
        }
        if (endetMit(a, ".pk3")) {
            Pk3 p;
            if (!readPk3Directory(a, p)) {
                zeilen.push_back(a + " pk3 FEHLER");
                continue;
            }
            std::vector<const Pk3Entry*> liste;
            for (const Pk3Entry& e : p.entries) {
                if (endetMit(e.name, ".glm") || endetMit(e.name, ".md3") || endetMit(e.name, ".gla") || endetMit(e.name, "animation.cfg")) {
                    liste.push_back(&e);
                }
            }
            std::sort(liste.begin(), liste.end(), [](const Pk3Entry* x, const Pk3Entry* y) { return x->name < y->name; });
            const std::string kurz = a.substr(a.find_last_of("/\\") + 1);
            for (const Pk3Entry* e : liste) {
                std::string bytes;
                if (readPk3File(p, *e, bytes)) { zeilen.push_back(zeile(kurz + ":" + e->name, bytes)); }
            }
            continue;
        }
        std::string kurz = a.substr(a.find_last_of("/\\") + 1);
        zeilen.push_back(zeile("datei/" + kurz, slurp(a)));
    }
    std::string text;
    for (const std::string& z : zeilen) { text += z + "\n"; }
    std::ofstream(argv[1], std::ios::binary) << text;
    int anders = 0;
    for (const std::string& z : zeilen) { anders += z.find(" ANDERS") != std::string::npos || z.find(" FEHLER") != std::string::npos ? 1 : 0; }
    std::printf("%zu Dateien, %d nicht gleich geschrieben\n", zeilen.size(), anders);
    if (!soll.empty()) {
        const std::string s = slurp(soll);
        // Zeilenenden des Solls egal (CRLF im Arbeitsbaum)
        std::string t;
        for (char c : s) {
            if (c != '\r') { t += c; }
        }
        const bool gleich = t == text;
        std::printf("Soll %s: %s\n", soll.c_str(), gleich ? "gleich" : "VERSCHIEDEN");
        if (!gleich) {
            std::istringstream a(t), b(text);
            std::string x, y;
            int n = 0;
            while (n < 10) {
                const bool ha = static_cast<bool>(std::getline(a, x));
                const bool hb = static_cast<bool>(std::getline(b, y));
                if (!ha && !hb) { break; }
                if (!ha) { x = "(nichts)"; }
                if (!hb) { y = "(nichts)"; }
                if (x != y) {
                    std::printf("  soll: %s\n  ist:  %s\n", x.c_str(), y.c_str());
                    ++n;
                }
            }
            return 1;
        }
    }
    return 0;
}
