// modell/stapelbegleit.h - der Modifier-Stapel in einer Begleitdatei neben der .glm (ENTSCHEID3A B12, rc612)
//
// Entscheidung B12 (plaene/entscheidungen_1211.md): der Stapel bleibt nach Schliessen und Oeffnen erhalten, in einer Begleitdatei
// neben der .glm ("model.glm" -> "model.glm.stack"). Die .glm bleibt bytegleich (sie wird wie bisher geschrieben), das Spiel liest
// die Begleitdatei nicht (es oeffnet nur die Namen, die Modell, Haut und Animation nennen). Laesst sich die Begleitdatei nicht lesen,
// laedt das Modell trotzdem - der Aufrufer meldet den Grund.
//
// Inhalt (Text, dann wahlweise Bytes):
//   JKSTUDIO-STACK 1
//   glm <Bytes> <FNV-1a 64 der .glm, hex>    die .glm, zu der der Stapel gehoert (aendert sie jemand, gilt er nicht mehr)
//   teil <Index> <Eintraege> <Name>          je Flaeche mit Stapel, danach ihre Eintraege in der Reihenfolge des Rechnens:
//   mod <Art> <Kennung> <an> <Datei> <Achse> <vonPlus> <Stufen> <RaenderHalten>
//   basis <Bytes>                            nur wenn ein Eintrag in die Datei ging: danach die .glm des Dokuments OHNE Stapel
//   ende
// Ging ein Eintrag in die Datei (Spiegeln ab Werk), traegt die .glm schon sein Ergebnis - die Flaeche darunter steht dann nur in
// der Basis. Beim Oeffnen wird das Dokument aus der Basis gelesen und der Stapel daraufgelegt: die Ansicht und das naechste
// Speichern ergeben wieder dieselbe .glm.
#ifndef BHED_MODELL_STAPELBEGLEIT_H
#define BHED_MODELL_STAPELBEGLEIT_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "bhed/modell/dokument.h"
#include "bhed/modell/stapeldaten.h"

namespace bhed::modell {

inline constexpr const char* kBegleitEndung = ".stack";
inline constexpr int kBegleitFassung = 1;

enum class BegleitFehler : std::uint8_t { Keiner = 0, Alt, Kaputt, Fassung, Flaechen };

struct BegleitDaten {
    std::uint64_t glmSumme = 0;
    std::uint64_t glmGroesse = 0;
    struct TeilStapel {
        int index = -1;
        std::string name;
        std::vector<Modifikator> stapel;
    };
    std::vector<TeilStapel> teile;
    std::string basisGlm;   // leer: die .glm selbst ist die Basis
    [[nodiscard]] int eintraege() const;
};

// FNV-1a 64 ueber Bytes (Pruefsumme der .glm)
[[nodiscard]] std::uint64_t begleitSumme(const std::string& bytes);
// "pfad/model.glm" -> "pfad/model.glm.stack"
[[nodiscard]] std::string stapelBegleitName(const std::string& glmPfad);
// Die Begleitdatei fuer den Inhalt `in` (mit Stapel) und die geschriebene .glm `glmBytes`. `basisGlm`: die .glm des Inhalts ohne
// Stapel - nur noetig, wenn ein Eintrag in die Datei ging (sonst nullptr). Leer: kein Eintrag auf keiner Flaeche (keine Datei).
[[nodiscard]] std::string stapelBegleitSchreiben(const Inhalt& in, const std::string& glmBytes, const std::string* basisGlm);
// Lesen und gegen die geoeffnete .glm pruefen.
[[nodiscard]] BegleitFehler stapelBegleitLesen(const std::string& datei, const std::string& glmBytes, BegleitDaten& aus);
// Den gelesenen Stapel auf den Inhalt aus der .glm legen (mit Basis: der Inhalt wird aus ihr gelesen; was die .glm sonst
// mitbringt - Name, Kino-Belegung - bleibt). Bei einem Fehler bleibt `in` unveraendert.
[[nodiscard]] BegleitFehler stapelBegleitAnwenden(const BegleitDaten& d, Inhalt& in);

}  // namespace bhed::modell
#endif
