// modell/md3datei.cpp - .md3 aus dem Modelldokument schreiben (Modell-Block 1b, siehe bhed/modell/md3datei.h)
//
// Aufbau nach Quake III Arena / OpenJK code/qcommon/qfiles.h (md3Header_t, md3Surface_t, md3Triangle_t, md3Shader_t,
// md3St_t, md3XyzNormal_t). Copyright (C) 1999-2005 Id Software, Inc.; 2013-2026 OpenJK contributors; GPL v2.
#include "bhed/modell/md3datei.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <utility>

namespace bhed::modell {
namespace {

constexpr std::size_t kName = 64;                          // MAX_QPATH
constexpr std::size_t kKopf = 4 + 4 + kName + 4 * 8;       // md3Header_t bis ofsSurfaces (ofsEnd steht dahinter)
constexpr std::size_t kKopfGanz = kKopf + 4;              // md3Header_t mit ofsEnd: 108 Bytes
constexpr std::size_t kFlaechenKopf = 4 + kName + 4 * 10;  // md3Surface_t

std::int32_t lies(const std::string& b, std::size_t at) {
    std::int32_t v = 0;
    if (at + 4 <= b.size()) { std::memcpy(&v, b.data() + at, 4); }
    return v;
}
void setze(std::string& b, std::size_t at, std::int32_t v) { std::memcpy(b.data() + at, &v, 4); }
void setzeF(std::string& b, std::size_t at, float v) { std::memcpy(b.data() + at, &v, 4); }
template <class T>
void dazu(std::string& b, T v) { b.append(reinterpret_cast<const char*>(&v), sizeof v); }
void nameDazu(std::string& b, const std::string& name) {
    std::string n = name.substr(0, kName - 1);
    n.resize(kName, '\0');
    b += n;
}

// Modell-Block 1d: Ort und Normale einer Ecke wie die Datei sie fuehrt (md3XyzNormal_t). Ort: MD3_XYZ_SCALE 1/64
// (qfiles.h), gerundet; Normale: die Umkehrung von R_LoadMD3 (tr_model.cpp: lat = ((n >> 8) & 255) * 2pi/256,
// lng = (n & 255) * 2pi/256, X = cos lat sin lng, Y = sin lat sin lng, Z = cos lng).
std::int16_t xyzKurz(float v) {
    const double x = std::round(static_cast<double>(v) * 64.0);
    return static_cast<std::int16_t>(std::clamp(x, -32768.0, 32767.0));
}
std::int16_t normaleKurz(const float n[3]) {
    const double x = n[0], y = n[1], z = n[2];
    const double l = std::sqrt(x * x + y * y + z * z);
    if (!(l > 0.0)) { return 0; }
    const double kPi = 3.14159265358979323846;
    const int lat = static_cast<int>(std::lround(std::atan2(y / l, x / l) * 256.0 / (2.0 * kPi))) & 255;
    const int lng = static_cast<int>(std::lround(std::acos(std::clamp(z / l, -1.0, 1.0)) * 256.0 / (2.0 * kPi))) & 255;
    return static_cast<std::int16_t>(static_cast<std::uint16_t>((lat << 8) | lng));
}
void normaleLang(std::int16_t k, float out[3]) {
    const auto u = static_cast<std::uint16_t>(k);
    const float kPi = 3.14159265358979323846F;
    const float lat = static_cast<float>((u >> 8) & 0xFF) * (2.0F * kPi / 256.0F);
    const float lng = static_cast<float>(u & 0xFF) * (2.0F * kPi / 256.0F);
    out[0] = std::cos(lat) * std::sin(lng);
    out[1] = std::sin(lat) * std::sin(lng);
    out[2] = std::cos(lng);
}
std::string xyzNormale(const Ecke& e) {
    std::string s;
    for (int k = 0; k < 3; ++k) { dazu(s, xyzKurz(e.ort[k])); }
    dazu(s, normaleKurz(e.normale));
    return s;
}

// Eine Flaeche neu (Kopf, Shader, Dreiecke, UV, Ort/Normale je Bild): alle Bilder mit dem Ort des Dokuments
std::string flaecheVonGrund(const Teil& t, const Netz& nz, std::int32_t bilder) {
    const std::vector<std::uint32_t> idx = nz.indizes();
    std::string f = "IDP3";
    nameDazu(f, t.name);
    dazu(f, std::int32_t{0});                                       // flags
    dazu(f, bilder);
    dazu(f, std::int32_t{1});                                       // numShaders
    dazu(f, static_cast<std::int32_t>(nz.ecken.size()));
    dazu(f, static_cast<std::int32_t>(idx.size() / 3U));
    const std::size_t ofsAt = f.size();
    for (int k = 0; k < 5; ++k) { dazu(f, std::int32_t{0}); }       // ofsTriangles, ofsShaders, ofsSt, ofsXyzNormals, ofsEnd
    setze(f, ofsAt + 4, static_cast<std::int32_t>(f.size()));      // Shader
    nameDazu(f, t.shader);
    dazu(f, std::int32_t{0});
    setze(f, ofsAt, static_cast<std::int32_t>(f.size()));          // Dreiecke
    for (const std::uint32_t i : idx) { dazu(f, static_cast<std::int32_t>(i)); }
    setze(f, ofsAt + 8, static_cast<std::int32_t>(f.size()));      // St
    for (const Ecke& e : nz.ecken) { f.append(reinterpret_cast<const char*>(e.uv), 8); }
    setze(f, ofsAt + 12, static_cast<std::int32_t>(f.size()));     // XyzNormals
    for (std::int32_t b = 0; b < bilder; ++b) {
        for (const Ecke& e : nz.ecken) { f += xyzNormale(e); }
    }
    setze(f, ofsAt + 16, static_cast<std::int32_t>(f.size()));
    return f;
}

}  // namespace

namespace {

// Bilder (md3Frame_t: bounds[2][3], localOrigin[3], radius, name[16]) aus den geschriebenen Ecken neu: Kasten je Bild
// und radius = RadiusFromBounds (q_math.c: je Achse der groessere Betrag, davon die Laenge).
void bilderNeu(std::string& b, std::int32_t ofsBilder, std::int32_t bilder, const std::vector<std::array<float, 6>>& kasten) {
    for (std::int32_t f = 0; f < bilder && static_cast<std::size_t>(f) < kasten.size(); ++f) {
        const std::size_t at = static_cast<std::size_t>(ofsBilder) + static_cast<std::size_t>(f) * 56U;
        if (at + 56U > b.size()) { return; }
        const auto& k = kasten[static_cast<std::size_t>(f)];
        if (k[0] > k[3]) { continue; }
        double r2 = 0.0;
        for (std::size_t a = 0; a < 3; ++a) {
            setzeF(b, at + a * 4U, k[a]);
            setzeF(b, at + 12U + a * 4U, k[a + 3U]);
            const double m = std::max(std::abs(static_cast<double>(k[a])), std::abs(static_cast<double>(k[a + 3U])));
            r2 += m * m;
        }
        setzeF(b, at + 36U, static_cast<float>(std::sqrt(r2)));
    }
}

void kastenDazu(std::vector<std::array<float, 6>>& kasten, std::int32_t bild, const std::string& xyz8) {
    if (static_cast<std::size_t>(bild) >= kasten.size()) { kasten.resize(static_cast<std::size_t>(bild) + 1U, {1e30F, 1e30F, 1e30F, -1e30F, -1e30F, -1e30F}); }
    auto& k = kasten[static_cast<std::size_t>(bild)];
    for (std::size_t a = 0; a < 3; ++a) {
        std::int16_t v = 0;
        std::memcpy(&v, xyz8.data() + a * 2U, 2);
        const float f = static_cast<float>(v) / 64.0F;
        k[a] = std::min(k[a], f);
        k[a + 3U] = std::max(k[a + 3U], f);
    }
}

// pfix N2: R_LoadMD3 bricht bei numTriangles*3 > SHADER_MAX_INDEXES ab (OpenJK code/rd-vanilla/tr_model.cpp:903, MD9
// tr_model.cpp:919), SHADER_MAX_INDEXES = 6*SHADER_MAX_VERTEXES (qfiles.h:34) - anders als Ghoul2 (Dreieckszahl als Indexzahl)
bool zuVieleDreiecke(std::size_t indizes, int eckenGrenze) { return eckenGrenze > 0 && indizes > 6U * static_cast<std::size_t>(eckenGrenze); }

std::string dreieckFehler(const std::string& name, std::size_t indizes, int eckenGrenze) {
    return "Flaeche " + name + ": " + std::to_string(indizes / 3U) + " Dreiecke (Grenze " + std::to_string(eckenGrenze * 2) + ")";
}

// Modell-Block 1d: ein Modell ohne gelesene Datei (Grundkoerper im Model-Tab): ein Bild, keine Tags, je Teil eine Flaeche
bool md3VonGrund(const Inhalt& in, std::string& aus, int eckenGrenze, Md3SchreibBericht& br) {
    std::vector<std::string> flaechen;
    std::vector<std::array<float, 6>> kasten;
    for (const Teil& t : in.teile) {
        if (t.istTag()) { continue; }
        const Netz leer;
        const Netz& nz = t.stufen.empty() ? leer : t.stufen[0];
        if (eckenGrenze > 0 && static_cast<int>(nz.ecken.size()) >= eckenGrenze) {
            br.fehler = "Flaeche " + t.name + ": " + std::to_string(nz.ecken.size()) + " Ecken (Grenze " + std::to_string(eckenGrenze) + ")";
            return false;
        }
        if (zuVieleDreiecke(static_cast<std::size_t>(nz.dreiecke()) * 3U, eckenGrenze)) {
            br.fehler = dreieckFehler(t.name, static_cast<std::size_t>(nz.dreiecke()) * 3U, eckenGrenze);
            return false;
        }
        flaechen.push_back(flaecheVonGrund(t, nz, 1));
        for (const Ecke& e : nz.ecken) { kastenDazu(kasten, 0, xyzNormale(e)); }
        br.ecken += static_cast<int>(nz.ecken.size());
        ++br.flaechenNeu;
    }
    std::string b = "IDP3";
    dazu(b, std::int32_t{15});   // MD3_VERSION
    nameDazu(b, in.name);
    dazu(b, std::int32_t{0});    // flags
    dazu(b, std::int32_t{1});    // numFrames
    dazu(b, std::int32_t{0});    // numTags
    dazu(b, static_cast<std::int32_t>(flaechen.size()));
    dazu(b, std::int32_t{0});    // numSkins
    const auto ofsBilder = static_cast<std::int32_t>(kKopfGanz);
    dazu(b, ofsBilder);
    dazu(b, static_cast<std::int32_t>(kKopfGanz + 56U));   // ofsTags (keine)
    dazu(b, static_cast<std::int32_t>(kKopfGanz + 56U));   // ofsSurfaces
    dazu(b, std::int32_t{0});                          // ofsEnd
    b.append(56U, '\0');
    std::memcpy(b.data() + kKopfGanz + 40U, "md3frame", 8);   // md3Frame_t name[16]
    for (const std::string& f : flaechen) { b += f; }
    setze(b, 8 + kName + 32, static_cast<std::int32_t>(b.size()));
    if (kasten.empty()) { kasten.push_back({0, 0, 0, 0, 0, 0}); }
    bilderNeu(b, ofsBilder, 1, kasten);
    aus = std::move(b);
    return true;
}

}  // namespace

bool md3Schreiben(const Inhalt& in, std::string& aus, int eckenGrenze, Md3SchreibBericht* bericht) {
    Md3SchreibBericht br;
    const auto fehler = [&](const std::string& f) {
        br.fehler = f;
        if (bericht != nullptr) { *bericht = br; }
        return false;
    };
    if (in.herkunft != Herkunft::Md3) { return fehler("keine .md3"); }
    if (!in.md3Datei) {
        const bool ok = md3VonGrund(in, aus, eckenGrenze, br);
        if (bericht != nullptr) { *bericht = br; }
        return ok;
    }
    const std::string& b = *in.md3Datei;
    if (b.size() < kKopf || b.compare(0, 4, "IDP3") != 0) { return fehler("keine .md3"); }
    const std::int32_t bilderDatei = lies(b, 8 + kName + 4);
    const std::int32_t flaechen = lies(b, 8 + kName + 12);
    const std::int32_t ofsBilder = lies(b, 8 + kName + 20);
    const std::int32_t ofsTags = lies(b, 8 + kName + 24);
    const std::int32_t ofsFlaechen = lies(b, 8 + kName + 28);
    const std::int32_t ofsEnde = lies(b, 8 + kName + 32);
    // pfix M1: je Flaeche der Datei ihr Teil (Teil::dateiFlaeche) - leere Flaechen hat readMd3 uebersprungen, sie haben keinen
    // Teil und bleiben wie gelesen; Teile ohne Flaeche der Datei (Detach in einen neuen Teil) kommen hinten dazu. Ein Dokument
    // ohne Zuordnung: Teil s ist Flaeche s.
    if (flaechen < 0) { return fehler("die Flaechen passen nicht zur Datei"); }
    std::vector<int> teilVon(static_cast<std::size_t>(flaechen), -1);
    std::vector<char> neuerTeil(in.teile.size(), 1);
    bool zugeordnet = false;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const int d = in.teile[i].dateiFlaeche;
        if (d >= 0 && d < flaechen && teilVon[static_cast<std::size_t>(d)] < 0) {
            teilVon[static_cast<std::size_t>(d)] = static_cast<int>(i);
            neuerTeil[i] = 0;
            zugeordnet = true;
        }
    }
    for (std::size_t s = 0; !zugeordnet && s < teilVon.size() && s < in.teile.size(); ++s) {
        teilVon[s] = static_cast<int>(s);
        neuerTeil[s] = 0;
    }
    if (ofsFlaechen <= 0 || static_cast<std::size_t>(ofsFlaechen) > b.size() || ofsBilder > ofsFlaechen || ofsTags > ofsFlaechen || ofsEnde < ofsFlaechen ||
        static_cast<std::size_t>(ofsEnde) > b.size()) {
        return fehler("Bilder oder Tags hinter den Flaechen");
    }
    std::string neu = b.substr(0, static_cast<std::size_t>(ofsFlaechen));
    std::size_t at = static_cast<std::size_t>(ofsFlaechen);
    std::vector<std::array<float, 6>> kasten;   // je Bild der Kasten aller geschriebenen Ecken
    bool orteNeu = false;                       // ein Ort weicht von der Datei ab: Kasten und Radius der Bilder neu
    for (std::size_t s = 0; s < static_cast<std::size_t>(flaechen); ++s) {
        if (at + kFlaechenKopf > b.size()) { return fehler("Flaeche " + std::to_string(s) + " abgeschnitten"); }
        const std::size_t n = at + 4 + kName;
        const std::int32_t sBilder = lies(b, n + 4);
        const std::int32_t sShader = lies(b, n + 8);
        const std::int32_t sEcken = lies(b, n + 12);
        const std::int32_t sDreiecke = lies(b, n + 16);
        const std::int32_t ofs[4] = {lies(b, n + 20), lies(b, n + 24), lies(b, n + 28), lies(b, n + 32)};   // Dreiecke, Shader, St, XyzNormal
        const std::int32_t sEnde = lies(b, n + 36);
        if (sEnde <= 0 || at + static_cast<std::size_t>(sEnde) > b.size() || sBilder <= 0 || sEcken < 0 || sDreiecke < 0) {
            return fehler("Flaeche " + std::to_string(s) + " unbrauchbar");
        }
        // pfix M2: jeder Abschnitt innerhalb der Flaeche (readMd3 prueft nur gegen das Dateiende) - sonst liest substr
        // hinter alt (std::out_of_range) oder memcpy in einen zu kurzen Rest
        const auto passt = [&](std::int32_t o, std::int32_t anzahl, std::uint64_t groesse) {
            return o >= 0 && o <= sEnde && anzahl >= 0 && static_cast<std::uint64_t>(anzahl) * groesse <= static_cast<std::uint64_t>(sEnde - o);
        };
        if (!passt(ofs[0], sDreiecke, 12U) || !passt(ofs[1], sShader, kName + 4U) || !passt(ofs[2], sEcken, 8U) ||
            !passt(ofs[3], sEcken, 8U * static_cast<std::uint64_t>(sBilder))) {
            return fehler("Flaeche " + std::to_string(s) + " unbrauchbar");
        }
        const std::string alt = b.substr(at, static_cast<std::size_t>(sEnde));
        at += static_cast<std::size_t>(sEnde);
        const int ti = teilVon[s];
        if (ti < 0) {
            if (sEcken != 0) { return fehler("die Flaechen passen nicht zur Datei"); }
            neu += alt;   // pfix M1: die leere Flaeche wie gelesen
            ++br.flaechenRoh;
            continue;
        }
        if (in.teile[static_cast<std::size_t>(ti)].stufen.empty()) { return fehler("Flaeche " + std::to_string(s) + " ohne Netz"); }
        const Netz& nz = in.teile[static_cast<std::size_t>(ti)].stufen[0];
        const std::vector<std::uint32_t> idx = nz.indizes();
        const auto xyzDatei = [&](std::int32_t bild, std::int32_t e) {
            return alt.substr(static_cast<std::size_t>(ofs[3]) + (static_cast<std::size_t>(bild) * static_cast<std::size_t>(sEcken) + static_cast<std::size_t>(e)) * 8U, 8);
        };
        // Modell-Block 1d: Ort und Normale wie die Datei (Bild 0)? Sonst hat Modellieren die Ecke bewegt.
        const auto wieDatei = [&](const Ecke& e) {
            if (e.dateiIndex < 0 || e.dateiIndex >= sEcken) { return false; }
            const std::string x = xyzDatei(0, e.dateiIndex);
            if (x.size() < 8U) { return false; }
            for (std::size_t k = 0; k < 3; ++k) {
                std::int16_t v = 0;
                std::memcpy(&v, x.data() + k * 2U, 2);
                if (v != xyzKurz(e.ort[k])) { return false; }
            }
            std::int16_t nk = 0;
            std::memcpy(&nk, x.data() + 6, 2);
            float nd[3];
            normaleLang(nk, nd);
            for (std::size_t k = 0; k < 3; ++k) {
                if (std::abs(nd[k] - e.normale[k]) > 1e-4F) { return false; }
            }
            return true;
        };
        bool gleich = nz.ecken.size() == static_cast<std::size_t>(sEcken) && idx.size() == static_cast<std::size_t>(sDreiecke) * 3U;
        for (std::size_t i = 0; gleich && i < idx.size(); ++i) {
            gleich = static_cast<std::uint32_t>(lies(alt, static_cast<std::size_t>(ofs[0]) + i * 4U)) == idx[i];
        }
        for (std::size_t e = 0; gleich && e < nz.ecken.size(); ++e) { gleich = nz.ecken[e].dateiIndex == static_cast<std::int32_t>(e) && wieDatei(nz.ecken[e]); }
        br.ecken += static_cast<int>(nz.ecken.size());
        if (gleich) {
            // nur die UV: dieselben Bytes, wo sie gleich ist
            std::string f = alt;
            for (std::size_t e = 0; e < nz.ecken.size(); ++e) { std::memcpy(f.data() + static_cast<std::size_t>(ofs[2]) + e * 8U, nz.ecken[e].uv, 8); }
            for (std::int32_t bild = 0; bild < sBilder; ++bild) {
                for (std::int32_t e = 0; e < sEcken; ++e) { kastenDazu(kasten, bild, xyzDatei(bild, e)); }
            }
            neu += f;
            ++br.flaechenRoh;
            continue;
        }
        if (eckenGrenze > 0 && static_cast<int>(nz.ecken.size()) >= eckenGrenze) {
            return fehler("Flaeche " + in.teile[static_cast<std::size_t>(ti)].name + ": " + std::to_string(nz.ecken.size()) + " Ecken (Grenze " + std::to_string(eckenGrenze) + ")");
        }
        if (zuVieleDreiecke(idx.size(), eckenGrenze)) { return fehler(dreieckFehler(in.teile[static_cast<std::size_t>(ti)].name, idx.size(), eckenGrenze)); }
        // die Abschnitte in der Reihenfolge der Datei neu (Kopf zuerst, er steht bei 0)
        std::array<std::pair<std::int32_t, int>, 4> folge{{{ofs[0], 0}, {ofs[1], 1}, {ofs[2], 2}, {ofs[3], 3}}};
        std::sort(folge.begin(), folge.end());
        std::string f = alt.substr(0, kFlaechenKopf);
        const std::int32_t neueEcken = static_cast<std::int32_t>(nz.ecken.size());
        std::int32_t neuOfs[4] = {};
        for (const auto& [alterOfs, art] : folge) {
            neuOfs[art] = static_cast<std::int32_t>(f.size());
            switch (art) {
                case 0:   // md3Triangle_t
                    for (const std::uint32_t i : idx) {
                        const std::int32_t v = static_cast<std::int32_t>(i);
                        f.append(reinterpret_cast<const char*>(&v), 4);
                    }
                    break;
                case 1:   // md3Shader_t: unveraendert
                    f += alt.substr(static_cast<std::size_t>(alterOfs), static_cast<std::size_t>(sShader) * (kName + 4));
                    break;
                case 2:   // md3St_t
                    for (const Ecke& e : nz.ecken) { f.append(reinterpret_cast<const char*>(e.uv), 8); }
                    break;
                default:   // md3XyzNormal_t je Bild
                    for (std::int32_t bild = 0; bild < sBilder; ++bild) {
                        for (const Ecke& e : nz.ecken) {
                            std::string x;
                            const bool datei = e.dateiIndex >= 0 && e.dateiIndex < sEcken;
                            if (datei && wieDatei(e)) {
                                x = xyzDatei(bild, e.dateiIndex);   // wie gelesen, in jedem Bild
                            } else if (datei && bild > 0) {
                                // Modell-Block 1d: bewegt - die anderen Bilder um denselben Weg wie Bild 0 (die Animation der
                                // Datei bleibt um den neuen Ort erhalten), ihre Normale wie gelesen
                                x = xyzDatei(bild, e.dateiIndex);
                                const std::string null = xyzDatei(0, e.dateiIndex);
                                for (std::size_t k = 0; k < 3; ++k) {
                                    std::int16_t v = 0, v0 = 0;
                                    std::memcpy(&v, x.data() + k * 2U, 2);
                                    std::memcpy(&v0, null.data() + k * 2U, 2);
                                    const int w = std::clamp(static_cast<int>(v) + static_cast<int>(xyzKurz(e.ort[k])) - static_cast<int>(v0), -32768, 32767);
                                    const auto w16 = static_cast<std::int16_t>(w);
                                    std::memcpy(x.data() + k * 2U, &w16, 2);
                                }
                                orteNeu = true;
                            } else {
                                x = xyzNormale(e);   // neue Ecke, oder Bild 0 einer bewegten: wie das Dokument
                                orteNeu = true;
                            }
                            kastenDazu(kasten, bild, x);
                            f += x;
                        }
                    }
                    break;
            }
        }
        setze(f, 4 + kName + 12, neueEcken);
        setze(f, 4 + kName + 16, static_cast<std::int32_t>(idx.size() / 3U));
        for (std::size_t k = 0; k < 4; ++k) { setze(f, 4 + kName + 20 + k * 4U, neuOfs[k]); }
        setze(f, 4 + kName + 36, static_cast<std::int32_t>(f.size()));
        neu += f;
        ++br.flaechenNeu;
    }
    if (at < static_cast<std::size_t>(ofsEnde)) { neu += b.substr(at, static_cast<std::size_t>(ofsEnde) - at); }
    // Modell-Block 1d: neue Teile (Detach in einen neuen Teil) als neue Flaechen mit allen Bildern der Datei
    const std::int32_t bilder = std::max<std::int32_t>(1, bilderDatei);
    std::int32_t zahl = flaechen;
    for (std::size_t s = 0; s < in.teile.size(); ++s) {
        if (neuerTeil[s] == 0) { continue; }
        const Teil& t = in.teile[s];
        if (t.istTag() || t.stufen.empty()) { continue; }
        if (eckenGrenze > 0 && static_cast<int>(t.stufen[0].ecken.size()) >= eckenGrenze) {
            return fehler("Flaeche " + t.name + ": " + std::to_string(t.stufen[0].ecken.size()) + " Ecken (Grenze " + std::to_string(eckenGrenze) + ")");
        }
        if (zuVieleDreiecke(static_cast<std::size_t>(t.stufen[0].dreiecke()) * 3U, eckenGrenze)) {
            return fehler(dreieckFehler(t.name, static_cast<std::size_t>(t.stufen[0].dreiecke()) * 3U, eckenGrenze));
        }
        neu += flaecheVonGrund(t, t.stufen[0], bilder);
        for (std::int32_t bild = 0; bild < bilder; ++bild) {
            for (const Ecke& e : t.stufen[0].ecken) { kastenDazu(kasten, bild, xyzNormale(e)); }
        }
        br.ecken += static_cast<int>(t.stufen[0].ecken.size());
        ++br.flaechenNeu;
        ++zahl;
        orteNeu = true;
    }
    setze(neu, 8 + kName + 12, zahl);
    setze(neu, 8 + kName + 32, static_cast<std::int32_t>(neu.size()));
    if (orteNeu) { bilderNeu(neu, ofsBilder, bilderDatei, kasten); }
    if (static_cast<std::size_t>(ofsEnde) < b.size()) { neu += b.substr(static_cast<std::size_t>(ofsEnde)); }   // Rest hinter ofsEnd
    aus = std::move(neu);
    if (bericht != nullptr) { *bericht = br; }
    return true;
}

}  // namespace bhed::modell
