// modell/xsiaufnahme.h - eine dotXSI-Animation in eine GLA aufnehmen (Modell-Block Stufe 2, Carcass-Weg)
//
// Wie Carcass $animation-Zeilen baut, ueber G2C (third_party/g2c, DennisHerrm/G2C-toolkit, GPL-2.0): der dotXSI-Leser
// (g2::xsi::parse, loadAnimation), das Auswerten gegen das Skelett einer vorhandenen GLA (g2::xsi::evaluate: Knoten nach
// Namen, Achsenwechsel, Massstab der GLA, Wurzelbewegung) und das Packen jedes Knochens in 14 Bytes (compressBone,
// MC_UnCompressQuat umgekehrt). Gleiche Eintraege teilen sich den Vorrat wie in writeMdxa.
#ifndef BHED_MODELL_XSIAUFNAHME_H
#define BHED_MODELL_XSIAUFNAHME_H

#include <string>
#include <vector>

#include "bhed/gla.h"

namespace bhed::modell {

struct XsiBericht {
    int bilder = 0;
    int vorrat = 0;
    std::vector<std::string> fehlendeKnochen;   // im Skelett, nicht in der XSI (bleiben in der Grundstellung)
    std::vector<std::string> zusatzKnochen;     // in der XSI, nicht im Skelett
    std::string fehler;
};

// xsiText -> eine GLA mit dem Skelett von `skelett` (Name im Kopf: kopfName) und den Frames der XSI.
[[nodiscard]] bool xsiAufnehmen(const std::string& xsiText, const GlaAnimation& skelett, const std::string& kopfName, GlaAnimation& aus,
                                XsiBericht* bericht = nullptr);

}  // namespace bhed::modell
#endif
