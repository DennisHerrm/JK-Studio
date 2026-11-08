// glavorrat.h - REST N5 (Code-Pruefung Modell-Block): der Vorrat einer GLA beim Schreiben - die 14-Byte-Eintraege
// (MC_UnCompressQuat, mdxaCompQuatBone_t) im bonePool, gleiche Eintraege einmal, je Bild und Knochen der Index
// (mdxaIndex_t: 3 Bytes, also hoechstens 0xFFFFFF + 1 Eintraege). Bis rc610 stand dieselbe Schleife sechsmal in
// animcfg (zusammenfuehren, anhaengen), figuranim (basisGla, dort ohne die Grenze), retarget, rig und xsiaufnahme.
#ifndef BHED_MODELL_GLAVORRAT_H
#define BHED_MODELL_GLAVORRAT_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace bhed::modell {

class GlaVorrat {
public:
    static constexpr std::size_t kEintrag = 14;
    // Die Eintraege, die schon im Vorrat stehen, gelten (der erste gleicher Eintraege, wie die Datei sie hat).
    explicit GlaVorrat(std::vector<std::uint8_t>& pool) : pool_(pool) {
        je_.reserve(pool.size() / kEintrag);
        for (std::size_t i = 0; i + kEintrag <= pool.size(); i += kEintrag) {
            je_.emplace(std::string(reinterpret_cast<const char*>(pool.data()) + i, kEintrag), static_cast<std::uint32_t>(i / kEintrag));
        }
    }
    // Der Index des Eintrags (14 Bytes); neu angehaengt, wenn es ihn noch nicht gibt.
    std::uint32_t dazu(const std::uint8_t* eintrag) {
        const std::string k(reinterpret_cast<const char*>(eintrag), kEintrag);
        const auto [it, neu] = je_.emplace(k, static_cast<std::uint32_t>(pool_.size() / kEintrag));
        if (neu) { pool_.insert(pool_.end(), k.begin(), k.end()); }   // aus der Kopie: der Eintrag darf im Vorrat selbst liegen
        return it->second;
    }
    [[nodiscard]] int eintraege() const noexcept { return static_cast<int>(pool_.size() / kEintrag); }
    // Mehr Eintraege, als ein 24-Bit-Index erreicht (die Datei waere falsch)
    [[nodiscard]] bool zuGross() const noexcept { return pool_.size() / kEintrag > 0xFFFFFFU; }

private:
    std::vector<std::uint8_t>& pool_;
    std::unordered_map<std::string, std::uint32_t> je_;
};

}  // namespace bhed::modell
#endif
