// schattenatlas.h - rc610 LICHT1: Plaetze der Schattenwuerfel des Echtzeit-Lichts im Karten-Editor (eigener Code,
// keine Vorlage in NetRadiant-Custom oder q3map2; gui/app_karte_echtlicht.cpp).
//
// Jedes Licht der Karte (ausser den Sonnen) behaelt seinen Schattenwuerfel, solange sich in seiner Huelle nichts
// aendert. Die Wuerfel liegen in einem Atlas aus Seiten (kAtlasBreite x kAtlasHoehe, 16 Bit je Texel); eine Seite haelt
// Wuerfel einer Groesse, die sechs Seiten eines Wuerfels nebeneinander (Zeile fuer Zeile). Die Groesse folgt der
// Huelle des Lichts (wuerfelGroesseFuer), das Budget begrenzt die Seiten: passt nicht alles, werden die Wuerfel der
// kleinsten Huellen zuerst eine Stufe kleiner (wuerfelGroessenVerteilen); reicht auch die kleinste Stufe nicht, bekommt
// der Rest keinen Wuerfel (0) - die Ansicht meldet das.
#ifndef BHED_SCHATTENATLAS_H
#define BHED_SCHATTENATLAS_H

#include <array>
#include <cstddef>
#include <vector>

namespace bhed {

inline constexpr int kAtlasBreite = 3072;   // 6 Seiten zu 512 nebeneinander
inline constexpr int kAtlasHoehe = 2048;
inline constexpr std::size_t kAtlasSeitenBytes = static_cast<std::size_t>(kAtlasBreite) * kAtlasHoehe * 2U;   // R16_FLOAT
inline constexpr std::array<int, 4> kWuerfelGroessen = {512, 256, 128, 64};
// Groesse nach der Huelle: die kleinste Stufe, deren Texel am Rand der Huelle (2 * Huelle / Groesse) hoechstens
// kWuerfelTexel Einheiten misst - so fein wie ein 512er Wuerfel am Rand einer Huelle von 768 (gemessen: Selbsttest
// licht1, Unterschied zum 512er Wuerfel im Bild unter einem Bildpunkt).
inline constexpr float kWuerfelTexel = 3.0F;
[[nodiscard]] int wuerfelGroesseFuer(float huelle);
[[nodiscard]] int wuerfelJeSeite(int groesse);   // (3072 / g) * (2048 / g) / 6
[[nodiscard]] std::size_t wuerfelBytes(int groesse);

struct AtlasPlatz {
    int groesse = 0;
    int seite = -1;
    int wuerfel = -1;   // in der Seite
    [[nodiscard]] bool gueltig() const { return seite >= 0; }
};

class SchattenAtlas {
public:
    void budgetSetzen(std::size_t bytes) { m_budget = bytes; }
    [[nodiscard]] std::size_t budget() const { return m_budget; }
    // Ein Platz der Groesse; ungueltig, wenn dafuer eine neue Seite noetig waere und das Budget voll ist.
    [[nodiscard]] AtlasPlatz belegen(int groesse);
    void freigeben(const AtlasPlatz& p);
    void leeren();
    // Die Textur braucht so viele Schichten (die hoechste je benutzte Seite + 1; leere Seiten nimmt die naechste Groesse).
    [[nodiscard]] int seiten() const { return static_cast<int>(m_seiten.size()); }
    [[nodiscard]] std::size_t bytes() const { return m_seiten.size() * kAtlasSeitenBytes; }
    [[nodiscard]] std::size_t belegt() const;   // belegte Wuerfel
    [[nodiscard]] int belegteSeiten() const;    // Seiten mit mindestens einem Wuerfel
    void kuerzen();                             // leere Seiten am Ende weg

private:
    struct Seite {
        int groesse = 0;   // 0: leer, fuer jede Groesse frei
        std::vector<bool> belegt;
        int frei = 0;
    };
    std::vector<Seite> m_seiten;
    std::size_t m_budget = 0;
};

// Die Groessen fuer Lichter mit diesen Huellen (Reihenfolge wie huellen), so dass die Seiten ins Budget passen. 0: kein Platz.
[[nodiscard]] std::vector<int> wuerfelGroessenVerteilen(const std::vector<float>& huellen, std::size_t budgetBytes);
// Seiten fuer so viele Wuerfel je Groesse (Reihenfolge wie kWuerfelGroessen)
[[nodiscard]] std::size_t atlasSeitenFuer(const std::array<std::size_t, 4>& anzahl);

}  // namespace bhed

#endif
