// modell/md3datei.h - .md3 aus dem Modelldokument schreiben (Modell-Block 1b)
//
// Das Format nach qfiles.h (Quake III Arena / OpenJK code/qcommon/qfiles.h: md3Header_t 108 Bytes, md3Surface_t 108
// Bytes, md3Triangle_t, md3Shader_t, md3St_t, md3XyzNormal_t 8 Bytes je Ecke und Bild). Eine .md3 hat Bilder (Ort und
// Normale je Ecke und Bild) und Tags; das Dokument traegt nur Bild 0. Geschrieben wird deshalb auf der gelesenen Datei
// (Inhalt::md3Datei):
//
//   unveraenderte Flaeche   Byte fuer Byte wie gelesen, die UV (md3St_t) aus dem Dokument - ohne Aenderung dieselbe Datei
//   geteilte Ecken          die Flaeche neu: Kopf, Shader, Dreiecke, UV und je Bild Ort und Normale der Datei-Ecke, von der
//                           die Ecke stammt (Ecke::dateiIndex); die Reihenfolge der Abschnitte wie in der Datei
//   alles andere            Kopf, Bilder, Tags und was hinter den Flaechen steht, wie gelesen; die Versaetze neu
//
// md3Paket (bhed/karte/modellimport.h) schreibt dagegen ein neues Requisit mit einem Bild und teilt Flaechen auf - fuer
// ein unveraendertes Modell aus dem Spiel waere das nicht dieselbe Datei.
#ifndef BHED_MODELL_MD3DATEI_H
#define BHED_MODELL_MD3DATEI_H
#include <string>
#include "bhed/modell/dokument.h"
namespace bhed::modell {
struct Md3SchreibBericht {
    int flaechenRoh = 0;     // nur die UV ersetzt
    int flaechenNeu = 0;     // neu aufgebaut (mehr Ecken)
    int ecken = 0;           // Ecken in allen Flaechen
    std::string fehler;
};
// false: nicht schreibbar (Grund in bericht.fehler) - keine gelesene .md3, Flaechen passen nicht zur Datei, eine Ecke
// ohne Datei-Ecke, mehr Ecken als die Grenze (SHADER_MAX_VERTEXES: R_LoadMD3 weist die Flaeche ab).
[[nodiscard]] bool md3Schreiben(const Inhalt& in, std::string& aus, int eckenGrenze, Md3SchreibBericht* bericht = nullptr);
}  // namespace bhed::modell
#endif
