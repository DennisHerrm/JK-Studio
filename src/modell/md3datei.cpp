// modell/md3datei.cpp - .md3 aus dem Modelldokument schreiben (Modell-Block 1b, siehe bhed/modell/md3datei.h)
//
// Aufbau nach Quake III Arena / OpenJK code/qcommon/qfiles.h (md3Header_t, md3Surface_t, md3Triangle_t, md3Shader_t,
// md3St_t, md3XyzNormal_t). Copyright (C) 1999-2005 Id Software, Inc.; 2013-2026 OpenJK contributors; GPL v2.
#include "bhed/modell/md3datei.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <utility>

namespace bhed::modell {
namespace {

constexpr std::size_t kName = 64;                          // MAX_QPATH
constexpr std::size_t kKopf = 4 + 4 + kName + 4 * 8;       // md3Header_t
constexpr std::size_t kFlaechenKopf = 4 + kName + 4 * 10;  // md3Surface_t

std::int32_t lies(const std::string& b, std::size_t at) {
    std::int32_t v = 0;
    if (at + 4 <= b.size()) { std::memcpy(&v, b.data() + at, 4); }
    return v;
}
void setze(std::string& b, std::size_t at, std::int32_t v) { std::memcpy(b.data() + at, &v, 4); }

}  // namespace

bool md3Schreiben(const Inhalt& in, std::string& aus, int eckenGrenze, Md3SchreibBericht* bericht) {
    Md3SchreibBericht br;
    const auto fehler = [&](const std::string& f) {
        br.fehler = f;
        if (bericht != nullptr) { *bericht = br; }
        return false;
    };
    if (in.herkunft != Herkunft::Md3 || !in.md3Datei) { return fehler("keine gelesene .md3"); }
    const std::string& b = *in.md3Datei;
    if (b.size() < kKopf || b.compare(0, 4, "IDP3") != 0) { return fehler("keine .md3"); }
    const std::int32_t flaechen = lies(b, 8 + kName + 12);
    const std::int32_t ofsBilder = lies(b, 8 + kName + 20);
    const std::int32_t ofsTags = lies(b, 8 + kName + 24);
    const std::int32_t ofsFlaechen = lies(b, 8 + kName + 28);
    const std::int32_t ofsEnde = lies(b, 8 + kName + 32);
    if (flaechen != static_cast<std::int32_t>(in.teile.size())) { return fehler("die Flaechen passen nicht zur Datei"); }
    if (ofsFlaechen <= 0 || static_cast<std::size_t>(ofsFlaechen) > b.size() || ofsBilder > ofsFlaechen || ofsTags > ofsFlaechen || ofsEnde < ofsFlaechen ||
        static_cast<std::size_t>(ofsEnde) > b.size()) {
        return fehler("Bilder oder Tags hinter den Flaechen");
    }
    std::string neu = b.substr(0, static_cast<std::size_t>(ofsFlaechen));
    std::size_t at = static_cast<std::size_t>(ofsFlaechen);
    for (std::size_t s = 0; s < in.teile.size(); ++s) {
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
        const std::string alt = b.substr(at, static_cast<std::size_t>(sEnde));
        at += static_cast<std::size_t>(sEnde);
        if (in.teile[s].stufen.empty()) { return fehler("Flaeche " + std::to_string(s) + " ohne Netz"); }
        const Netz& nz = in.teile[s].stufen[0];
        const std::vector<std::uint32_t> idx = nz.indizes();
        bool gleich = nz.ecken.size() == static_cast<std::size_t>(sEcken) && idx.size() == static_cast<std::size_t>(sDreiecke) * 3U;
        for (std::size_t i = 0; gleich && i < idx.size(); ++i) {
            gleich = static_cast<std::uint32_t>(lies(alt, static_cast<std::size_t>(ofs[0]) + i * 4U)) == idx[i];
        }
        for (std::size_t e = 0; gleich && e < nz.ecken.size(); ++e) { gleich = nz.ecken[e].dateiIndex == static_cast<std::int32_t>(e); }
        br.ecken += static_cast<int>(nz.ecken.size());
        if (gleich) {
            // nur die UV: dieselben Bytes, wo sie gleich ist
            std::string f = alt;
            for (std::size_t e = 0; e < nz.ecken.size(); ++e) { std::memcpy(f.data() + static_cast<std::size_t>(ofs[2]) + e * 8U, nz.ecken[e].uv, 8); }
            neu += f;
            ++br.flaechenRoh;
            continue;
        }
        if (eckenGrenze > 0 && static_cast<int>(nz.ecken.size()) >= eckenGrenze) {
            return fehler("Flaeche " + in.teile[s].name + ": " + std::to_string(nz.ecken.size()) + " Ecken (Grenze " + std::to_string(eckenGrenze) + ")");
        }
        for (const Ecke& e : nz.ecken) {
            if (e.dateiIndex < 0 || e.dateiIndex >= sEcken) { return fehler("Flaeche " + in.teile[s].name + ": eine Ecke ohne Ecke der Datei"); }
        }
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
                default:   // md3XyzNormal_t je Bild, von der Datei-Ecke
                    for (std::int32_t bild = 0; bild < sBilder; ++bild) {
                        for (const Ecke& e : nz.ecken) {
                            f += alt.substr(static_cast<std::size_t>(alterOfs) + (static_cast<std::size_t>(bild) * static_cast<std::size_t>(sEcken) + static_cast<std::size_t>(e.dateiIndex)) * 8U, 8);
                        }
                    }
                    break;
            }
        }
        setze(f, 4 + kName + 12, neueEcken);
        setze(f, 4 + kName + 16, static_cast<std::int32_t>(idx.size() / 3U));
        for (int k = 0; k < 4; ++k) { setze(f, 4 + kName + 20 + static_cast<std::size_t>(k) * 4U, neuOfs[k]); }
        setze(f, 4 + kName + 36, static_cast<std::int32_t>(f.size()));
        neu += f;
        ++br.flaechenNeu;
    }
    if (at < static_cast<std::size_t>(ofsEnde)) { neu += b.substr(at, static_cast<std::size_t>(ofsEnde) - at); }
    setze(neu, 8 + kName + 32, static_cast<std::int32_t>(neu.size()));
    if (static_cast<std::size_t>(ofsEnde) < b.size()) { neu += b.substr(static_cast<std::size_t>(ofsEnde)); }   // Rest hinter ofsEnd
    aus = std::move(neu);
    if (bericht != nullptr) { *bericht = br; }
    return true;
}

}  // namespace bhed::modell
