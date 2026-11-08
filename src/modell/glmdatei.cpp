// modell/glmdatei.cpp - .glm aus dem Modelldokument schreiben (siehe bhed/modell/glmdatei.h)
//
//   File layout after mdx_format.h of OpenJK (code/rd-common/mdx_format.h; Copyright (C) 2000-2013 Raven Software,
//   Inc., 2013-2015 OpenJK contributors) and the writer of G2C (DennisHerrm/G2C-toolkit, src/mdxm.cpp writeMdxm),
//   both licensed under the GNU General Public License version 2. This file is part of BehavEd-Remake and licensed
//   under the GNU General Public License version 2 or later (LICENSE).
#include "bhed/modell/glmdatei.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>

#include "bhed/glm.h"

namespace bhed::modell {
namespace {

constexpr std::size_t kQPath = 64;          // MAX_QPATH
constexpr std::size_t kKopf = 164;          // sizeof(mdxmHeader_t)
constexpr std::size_t kHierarchie = 144;    // sizeof(mdxmSurfHierarchy_t) ohne childIndexes
constexpr std::size_t kFlaeche = 40;        // sizeof(mdxmSurface_t)
constexpr std::size_t kEcke = 32;           // sizeof(mdxmVertex_t)

std::int32_t i32(const std::string& b, std::size_t at) {
    if (at + 4 > b.size()) { return 0; }
    std::uint32_t v = 0;
    std::memcpy(&v, b.data() + at, 4);
    return static_cast<std::int32_t>(v);
}

std::string feld(const std::string& b, std::size_t at) {
    return at + kQPath <= b.size() ? b.substr(at, kQPath) : std::string();
}

// Der Name eines 64-Byte-Felds bis zur ersten Null (wie fixed() in glm.cpp, ohne Leerzeichen am Rand zu streichen:
// verglichen wird mit dem Namen, den readGlm gelesen hat).
std::string bisNull(const std::string& f) { return f.substr(0, std::min(f.find('\0'), f.size())); }

// Eine Ecke so, wie readGlm sie aus den Feldern liest (glm.cpp, G2_GetVertBoneWeight mdx_format.h:334-352): die
// ersten n-1 Gewichte fG2_BONEWEIGHT_RECIPROCAL_MULT * iTemp, das letzte 1 - Summe; unbenutzte Plaetze 0.
void dekodieren(std::uint32_t pack, const std::uint8_t g[4], const std::vector<std::int32_t>& refs, std::uint8_t knochen[4],
                float gewicht[4]) {
    std::memset(knochen, 0, 4);
    std::memset(gewicht, 0, 4 * sizeof(float));
    const int n = static_cast<int>((pack >> 30U) & 0x3U) + 1;
    float summe = 0.0F;
    for (int k = 0; k < n; ++k) {
        const auto lokal = static_cast<std::size_t>((pack >> (5U * static_cast<unsigned>(k))) & 0x1FU);
        const std::int32_t global = lokal < refs.size() ? refs[lokal] : 0;
        knochen[k] = static_cast<std::uint8_t>((global >= 0 && global < 256) ? global : 0);
        if (k + 1 == n) {
            gewicht[k] = 1.0F - summe;
        } else {
            const std::uint32_t oben = (pack >> (20U + 2U * static_cast<unsigned>(k))) & 0x3U;
            gewicht[k] = static_cast<float>(1.0F / 1023.0F) * static_cast<float>(g[k] | (oben << 8U));
            summe += gewicht[k];
        }
    }
}

bool eckePasst(const Ecke& e, const std::vector<std::int32_t>& refs) {
    if (!e.ausDatei) { return false; }
    std::uint8_t k[4];
    float g[4];
    dekodieren(e.dateiPack, e.dateiGewicht, refs, k, g);
    return std::memcmp(k, e.knochen, sizeof k) == 0 && std::memcmp(g, e.gewicht, sizeof g) == 0;
}

// Traegt die Stufe einer Flaeche noch die Gewichtsfelder der Datei?
bool rohPasst(const Netz& n) {
    if (!n.ausDatei || n.dateiRefs.size() != n.dateiRefsAufgeloest.size()) { return false; }
    return std::all_of(n.ecken.begin(), n.ecken.end(), [&](const Ecke& e) { return eckePasst(e, n.dateiRefsAufgeloest); });
}

// Knochen mit Gewicht einer Ecke in ihrer Reihenfolge (nur Gewichte > 0); ohne jedes: Knochen 0 mit 1 wie G2C
// (selectWeights: "kein Gewicht -> erster Knochen").
std::vector<std::pair<int, float>> paare(const Ecke& e) {
    std::vector<std::pair<int, float>> p;
    for (int k = 0; k < 4; ++k) {
        if (e.gewicht[k] > 0.0F) { p.emplace_back(e.knochen[k], e.gewicht[k]); }
    }
    if (p.empty()) { p.emplace_back(0, 1.0F); }
    return p;
}

// quantWeight (G2C src/mdxm.cpp): auf 10 Bit gerundet.
std::uint32_t zehnBit(float w) { return static_cast<std::uint32_t>(std::clamp(std::lround(static_cast<double>(w) * 1023.0), 0L, 1023L)); }

class Puffer {
public:
    std::string b;
    void i32(std::int32_t v) {
        char c[4];
        std::memcpy(c, &v, 4);
        b.append(c, 4);
    }
    void u32(std::uint32_t v) { i32(static_cast<std::int32_t>(v)); }
    void f32(float v) {
        char c[4];
        std::memcpy(c, &v, 4);
        b.append(c, 4);
    }
    void flicken(std::size_t at, std::int32_t v) { std::memcpy(b.data() + at, &v, 4); }
    std::size_t platz() {
        const std::size_t at = b.size();
        i32(0);
        return at;
    }
    // Ein 64-Byte-Feld: die Bytes der Datei, solange ihr Name derselbe ist, sonst der Name mit Nullen.
    void name(const std::string& n, const std::string& roh) {
        if (roh.size() == kQPath && bisNull(roh) == n) {
            b += roh;
        } else {
            std::string f = n;
            f.resize(kQPath, '\0');
            b += f;
        }
    }
};

}  // namespace

void glmRohfelder(const std::string& b, Inhalt& in) {
    if (in.herkunft != Herkunft::Glm || b.size() < kKopf || b.compare(0, 4, "2LGM") != 0) { return; }
    in.dateiName = feld(b, 8);
    in.dateiGla = feld(b, 72);
    in.dateiKnochen = i32(b, 140);
    const std::int32_t numLods = i32(b, 144);
    const std::int32_t ofsLods = i32(b, 148);
    const std::int32_t numSurfaces = i32(b, 152);
    const std::int32_t ofsHier = i32(b, 156);
    if (numSurfaces <= 0 || static_cast<std::size_t>(numSurfaces) != in.teile.size() || ofsLods <= 0 || ofsHier <= 0) { return; }
    // Hierarchie: Name und Shader roh
    std::size_t p = static_cast<std::size_t>(ofsHier);
    for (Teil& t : in.teile) {
        if (p + kHierarchie > b.size()) { return; }
        t.dateiName = feld(b, p);
        t.dateiShader = feld(b, p + 68);
        const std::int32_t nk = std::max(i32(b, p + 140), 0);
        t.dateiKinder.clear();
        for (std::int32_t k = 0; k < nk && p + kHierarchie + static_cast<std::size_t>(k) * 4U + 4U <= b.size(); ++k) {
            t.dateiKinder.push_back(i32(b, p + kHierarchie + static_cast<std::size_t>(k) * 4U));
        }
        p += kHierarchie + static_cast<std::size_t>(std::max(i32(b, p + 140), 0)) * 4U;
    }
    // Umlegen wie readGlm (glm.cpp): 72 Knochen mit "_humanoid" unter JKA, 53 unter JK2_MODE
    const bool humanoid = bisNull(in.dateiGla).find("_humanoid") != std::string::npos;
    const bool jk2Modell = in.jk2Umgewandelt && humanoid;
    const bool jkaModell = in.jkaUmgewandelt && humanoid;
    std::size_t lod = static_cast<std::size_t>(ofsLods);
    for (int s = 0; s < std::min(numLods, in.stufenZahl); ++s) {
        if (lod + 4 + static_cast<std::size_t>(numSurfaces) * 4 > b.size()) { return; }
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            if (static_cast<std::size_t>(s) >= in.teile[i].stufen.size()) { continue; }
            Netz& n = in.teile[i].stufen[static_cast<std::size_t>(s)];
            const std::int32_t rel = i32(b, lod + 4 + i * 4);
            const std::size_t base = lod + 4 + static_cast<std::size_t>(rel);
            if (rel < 0 || base + kFlaeche > b.size()) { continue; }
            const std::int32_t numVerts = i32(b, base + 12);
            const std::int32_t ofsVerts = i32(b, base + 16);
            const std::int32_t numRefs = i32(b, base + 28);
            const std::int32_t ofsRefs = i32(b, base + 32);
            if (numVerts < 0 || static_cast<std::size_t>(numVerts) != n.ecken.size()) { continue; }
            n.dateiRefs.clear();
            n.dateiRefsAufgeloest.clear();
            // REST N1: die Versaetze zaehlen ab der Flaeche und duerfen negativ sein (die Engine rechnet (byte *)surf + ofs);
            // vorzeichenbehaftet gerechnet und nur innerhalb der Datei - base + size_t(ofs) lief bei einem Ziel vor dem
            // Anfang der Datei um, die Grenzpruefung passte, und es wurde vor dem Puffer gelesen
            const auto inDatei = [&](std::int32_t ofs, std::size_t laenge, std::size_t& at) {
                const std::int64_t p = static_cast<std::int64_t>(base) + ofs;
                if (p < 0 || static_cast<std::uint64_t>(p) > b.size() || laenge > b.size() - static_cast<std::size_t>(p)) { return false; }
                at = static_cast<std::size_t>(p);
                return true;
            };
            std::size_t refsAt = 0;
            // readGlm liest Verweise nur bei 1..64 (sonst keine)
            if (numRefs > 0 && numRefs <= 64 && inDatei(ofsRefs, static_cast<std::size_t>(numRefs) * 4, refsAt)) {
                for (std::int32_t r = 0; r < numRefs; ++r) {
                    const std::int32_t ref = i32(b, refsAt + static_cast<std::size_t>(r) * 4);
                    n.dateiRefs.push_back(ref);
                    n.dateiRefsAufgeloest.push_back(glmVerweisUmlegen(ref, jk2Modell, jkaModell));
                }
            } else if (numRefs != 0) {
                continue;   // so nicht wieder zu schreiben: neu kodieren
            }
            std::size_t v0 = 0;
            if (!inDatei(ofsVerts, n.ecken.size() * kEcke, v0)) { continue; }
            for (std::size_t v = 0; v < n.ecken.size(); ++v) {
                Ecke& e = n.ecken[v];
                e.dateiPack = static_cast<std::uint32_t>(i32(b, v0 + v * kEcke + 24));
                std::memcpy(e.dateiGewicht, b.data() + v0 + v * kEcke + 28, 4);
                e.ausDatei = true;
            }
            n.ausDatei = true;
        }
        const std::int32_t ende = i32(b, lod);
        if (ende <= 0) { return; }
        lod += static_cast<std::size_t>(ende);
    }
}

bool glmSchreiben(const Inhalt& in, std::string& aus, GlmSchreibBericht* bericht) {
    GlmSchreibBericht leer;
    GlmSchreibBericht& br = bericht != nullptr ? *bericht : leer;
    br = GlmSchreibBericht{};
    aus.clear();
    const auto fehler = [&](const ModellFehler& f) {
        br.fehler = fehlerText(f);
        br.fehlerArt = f;
        return false;
    };
    if (in.herkunft != Herkunft::Glm || in.teile.empty()) { return fehler(ModellFehler{FehlerArt::KeinGlm}); }
    // MAX_QPATH: 63 Zeichen und die Null; 64 ohne Null nur, wie es in der Datei stand (Movie Duels holo.glm).
    const auto passt = [](const std::string& n, const std::string& roh) { return n.size() < kQPath || (roh.size() == kQPath && roh == n); };
    if (!passt(in.name, in.dateiName) || !passt(in.glaPfad, in.dateiGla)) { return fehler(ModellFehler{FehlerArt::GlaPfadZuLang}); }
    const int stufen = std::max(in.stufenZahl, 1);
    for (const Teil& t : in.teile) {
        if (!passt(t.name, t.dateiName) || !passt(t.shader, t.dateiShader)) { return fehler(ModellFehler{FehlerArt::TeilNameZuLang, t.name}); }
        if (static_cast<int>(t.stufen.size()) < stufen) { return fehler(ModellFehler{FehlerArt::OhneLods, t.name}); }
    }
    // Ein umgelegtes Modell (JK2 <-> JKA): unveraendert mit den Verweisen der Datei, sonst ganz mit dem Skelett des
    // Dokuments - beides zu mischen ergaebe Verweise aus zwei Skeletten.
    const bool umgelegt = in.jk2Umgewandelt || in.jkaUmgewandelt;
    bool alleRoh = true;
    for (const Teil& t : in.teile) {
        for (int s = 0; s < stufen && alleRoh; ++s) { alleRoh = rohPasst(t.stufen[static_cast<std::size_t>(s)]); }
    }
    br.umgelegt = umgelegt && !alleRoh;
    br.knochen = (alleRoh || !umgelegt) && in.dateiKnochen > 0 ? in.dateiKnochen : in.knochenZahl;

    const auto ns = static_cast<std::int32_t>(in.teile.size());
    Puffer o;
    o.b.reserve(1U << 20U);
    o.b += "2LGM";
    o.i32(6);   // MDXM_VERSION
    o.name(in.name, in.dateiName);
    o.name(in.glaPfad, in.dateiGla);
    o.i32(0);   // animIndex: traegt die Engine ein
    o.i32(br.knochen);
    o.i32(stufen);
    const std::size_t ofsLods = o.platz();
    o.i32(ns);
    const std::size_t ofsHier = o.platz();
    const std::size_t ofsEnde = o.platz();
    // Verzeichnis und Hierarchie
    std::vector<std::size_t> hierPlatz(in.teile.size());
    for (std::size_t& h : hierPlatz) { h = o.platz(); }
    o.flicken(ofsHier, static_cast<std::int32_t>(o.b.size()));
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const Teil& t = in.teile[i];
        o.flicken(hierPlatz[i], static_cast<std::int32_t>(o.b.size() - kKopf));
        o.name(t.name, t.dateiName);
        o.u32(t.flaggen);
        o.name(t.shader, t.dateiShader);
        o.i32(0);   // shaderIndex: traegt die Engine ein
        o.i32(t.eltern);
        // Die Liste der Datei, solange sie dieselben Kinder nennt (nahdar: ein Kind doppelt), sonst aus den Eltern.
        std::vector<int> a = t.dateiKinder;
        std::vector<int> c = t.kinder;
        std::sort(a.begin(), a.end());
        a.erase(std::unique(a.begin(), a.end()), a.end());
        std::sort(c.begin(), c.end());
        const std::vector<int>& kinder = (!t.dateiKinder.empty() && a == c) ? t.dateiKinder : t.kinder;
        o.i32(static_cast<std::int32_t>(kinder.size()));
        for (int k : kinder) { o.i32(k); }
    }
    o.flicken(ofsLods, static_cast<std::int32_t>(o.b.size()));
    for (int s = 0; s < stufen; ++s) {
        const std::size_t lodAnfang = o.platz();
        const std::size_t tabelle = o.b.size();
        std::vector<std::size_t> flPlatz(in.teile.size());
        for (std::size_t& f : flPlatz) { f = o.platz(); }
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            const Netz& n = in.teile[i].stufen[static_cast<std::size_t>(s)];
            const std::size_t anfang = o.b.size();
            o.flicken(flPlatz[i], static_cast<std::int32_t>(anfang - tabelle));
            const bool roh = alleRoh || (!umgelegt && rohPasst(n));
            // Knochenverweise: roh die der Datei; neu die der Datei, wenn sie alle benutzten Knochen enthalten (ohne
            // Umlegen), sonst aufsteigend wie G2C collectBoneRefs (std::map) und Carcass.
            std::vector<std::int32_t> refs;
            std::map<int, int> lokal;
            if (roh) {
                refs = n.dateiRefs;
                ++br.flaechenRoh;
            } else {
                std::map<int, int> benutzt;
                for (const Ecke& e : n.ecken) {
                    for (const auto& [k, g] : paare(e)) { benutzt.emplace(k, 0); }
                }
                bool dateiReicht = n.ausDatei && !umgelegt && !n.dateiRefs.empty();
                for (const auto& [k, x] : benutzt) {
                    dateiReicht = dateiReicht && std::find(n.dateiRefs.begin(), n.dateiRefs.end(), k) != n.dateiRefs.end();
                }
                if (dateiReicht) {
                    refs = n.dateiRefs;
                } else {
                    for (const auto& [k, x] : benutzt) { refs.push_back(k); }
                }
                if (refs.size() > 32U) {   // iMAX_G2_BONEREFS_PER_SURFACE, 5 Bit je Verweis
                    return fehler(ModellFehler{FehlerArt::ZuVieleKnochen, in.teile[i].name, s, static_cast<long long>(refs.size())});
                }
                for (std::size_t r = refs.size(); r-- > 0;) { lokal[refs[r]] = static_cast<int>(r); }
                ++br.flaechenNeu;
            }
            const std::vector<std::uint32_t> idx = n.indizes();
            const auto nv = static_cast<std::int32_t>(n.ecken.size());
            const auto nt = static_cast<std::int32_t>(idx.size() / 3U);
            o.i32(0);                                        // ident
            o.i32(static_cast<std::int32_t>(i));             // thisSurfaceIndex
            o.i32(-static_cast<std::int32_t>(anfang));       // ofsHeader: zurueck zum Kopf
            o.i32(nv);
            o.i32(static_cast<std::int32_t>(kFlaeche) + nt * 12);              // ofsVerts
            o.i32(nt);
            o.i32(static_cast<std::int32_t>(kFlaeche));                        // ofsTriangles
            o.i32(static_cast<std::int32_t>(refs.size()));
            o.i32(static_cast<std::int32_t>(kFlaeche) + nt * 12 + nv * 40);    // ofsBoneReferences
            o.i32(static_cast<std::int32_t>(kFlaeche) + nt * 12 + nv * 40 + static_cast<std::int32_t>(refs.size()) * 4);
            for (std::uint32_t x : idx) { o.i32(static_cast<std::int32_t>(x)); }
            for (const Ecke& e : n.ecken) {
                for (float x : e.normale) { o.f32(x); }
                for (float x : e.ort) { o.f32(x); }
                std::uint32_t pack = 0;
                std::uint8_t g[4] = {0, 0, 0, 0};
                if (roh) {
                    pack = e.dateiPack;
                    std::memcpy(g, e.dateiGewicht, 4);
                } else {
                    // writeMdxm (G2C): je Gewicht 5 Bit Verweis, 8 Bit ins Byte, 2 Bit nach 20 + 2k; Bit 30-31 Anzahl - 1.
                    // Das letzte Gewicht steht mit in der Datei, das Spiel liest es als Rest (G2_GetVertBoneWeight).
                    const auto p = paare(e);
                    const std::size_t m = std::min<std::size_t>(p.size(), 4U);
                    for (std::size_t k = 0; k < m; ++k) {
                        const std::uint32_t q = zehnBit(p[k].second);
                        pack |= (static_cast<std::uint32_t>(lokal[p[k].first]) & 0x1FU) << (5U * k);
                        pack |= (q & 0x300U) << (12U + 2U * k);
                        g[k] = static_cast<std::uint8_t>(q & 0xFFU);
                    }
                    pack |= static_cast<std::uint32_t>(m - 1U) << 30U;
                    ++br.eckenNeu;
                }
                o.u32(pack);
                o.b.append(reinterpret_cast<const char*>(g), 4);
            }
            for (const Ecke& e : n.ecken) {
                o.f32(e.uv[0]);
                o.f32(e.uv[1]);
            }
            for (std::int32_t r : refs) { o.i32(r); }
        }
        o.flicken(lodAnfang, static_cast<std::int32_t>(o.b.size() - lodAnfang));
    }
    o.flicken(ofsEnde, static_cast<std::int32_t>(o.b.size()));
    aus = std::move(o.b);
    return true;
}

std::string glmUnterschied(const std::string& a, const std::string& b) {
    if (a == b) { return {}; }
    std::size_t d = 0;
    while (d < a.size() && d < b.size() && a[d] == b[d]) { ++d; }
    const std::string bei = " (Byte " + std::to_string(d) + ", Groesse " + std::to_string(a.size()) + " / " + std::to_string(b.size()) + ")";
    if (d < kKopf) {
        static const char* const kFelder[] = {"Kennung", "Fassung", "Name", "Name", "animIndex", "numBones", "numLODs",
                                              "ofsLODs", "numSurfaces", "ofsSurfHierarchy", "ofsEnd"};
        const std::size_t f = d < 8 ? d / 4 : d < 136 ? (d < 72 ? 2 : 3) : 4 + (d - 136) / 4;
        return std::string("Kopf: ") + (f == 3 ? "Pfad der .gla" : kFelder[std::min<std::size_t>(f, 10)]) + bei;
    }
    const std::int32_t ns = i32(a, 152);
    const std::int32_t ofsHier = i32(a, 156);
    const std::int32_t ofsLods = i32(a, 148);
    if (d < static_cast<std::size_t>(ofsLods)) {
        if (d < static_cast<std::size_t>(ofsHier)) { return "Verzeichnis der Hierarchie" + bei; }
        std::size_t p = static_cast<std::size_t>(ofsHier);
        for (std::int32_t i = 0; i < ns; ++i) {
            const std::size_t ende = p + kHierarchie + static_cast<std::size_t>(std::max(i32(a, p + 140), 0)) * 4U;
            if (d < ende) {
                const std::size_t r = d - p;
                const char* f = r < 64 ? "Name" : r < 68 ? "Flaggen" : r < 132 ? "Shader" : r < 136 ? "shaderIndex" : r < 140 ? "Eltern" : "Kinder";
                return "Hierarchie " + std::to_string(i) + " " + bisNull(feld(a, p)) + ": " + f + bei;
            }
            p = ende;
        }
    }
    std::size_t lod = static_cast<std::size_t>(ofsLods);
    for (int l = 0; l < i32(a, 144) && lod < a.size(); ++l) {
        const std::size_t lodEnde = lod + static_cast<std::size_t>(std::max(i32(a, lod), 0));
        if (d < lodEnde || lodEnde <= lod) {
            if (d < lod + 4 + static_cast<std::size_t>(ns) * 4) { return "LOD " + std::to_string(l) + ": Kopf/Verzeichnis" + bei; }
            for (std::int32_t i = 0; i < ns; ++i) {
                const std::size_t base = lod + 4 + static_cast<std::size_t>(i32(a, lod + 4 + static_cast<std::size_t>(i) * 4));
                const std::size_t ende = base + static_cast<std::size_t>(std::max(i32(a, base + 36), 0));
                if (d >= ende) { continue; }
                const std::size_t r = d - base;
                const auto nv = static_cast<std::size_t>(std::max(i32(a, base + 12), 0));
                const auto ov = static_cast<std::size_t>(std::max(i32(a, base + 16), 0));
                const auto ot = static_cast<std::size_t>(std::max(i32(a, base + 24), 0));
                const auto orf = static_cast<std::size_t>(std::max(i32(a, base + 32), 0));
                std::string was;
                if (r < kFlaeche) {
                    was = "Flaechenkopf Feld " + std::to_string(r / 4);
                } else if (r >= ot && r < ov) {
                    was = "Dreieck " + std::to_string((r - ot) / 12);
                } else if (r >= ov && r < ov + nv * kEcke) {
                    const std::size_t e = (r - ov) / kEcke;
                    const std::size_t k = (r - ov) % kEcke;
                    was = "Ecke " + std::to_string(e) + (k < 12 ? ": Normale" : k < 24 ? ": Ort" : ": Gewichtsfelder");
                } else if (r >= ov + nv * kEcke && r < orf) {
                    was = "UV " + std::to_string((r - ov - nv * kEcke) / 8);
                } else {
                    was = "Knochenverweise";
                }
                std::string name;
                std::size_t p = static_cast<std::size_t>(ofsHier);
                for (std::int32_t h = 0; h < i && p + kHierarchie <= a.size(); ++h) {
                    p += kHierarchie + static_cast<std::size_t>(std::max(i32(a, p + 140), 0)) * 4U;
                }
                name = bisNull(feld(a, p));
                return "LOD " + std::to_string(l) + ", Flaeche " + std::to_string(i) + " " + name + ", " + was + bei;
            }
            return "LOD " + std::to_string(l) + bei;
        }
        lod = lodEnde;
    }
    return "hinter den LODs" + bei;
}

}  // namespace bhed::modell
