// dxt.h - MR4 Punkt 7: Texturkompression wie das Spiel (S3TC/DXT5) fuer die Vorschau.
//
// OpenJK/Original-JKA Upload32 (rd-vanilla/tr_image.cpp:629-689): ist glConfig.textureCompression TC_S3TC_DXT
// (r_ext_compress_textures 1 und GL_EXT_texture_compression_s3tc, tr_init.cpp:296-388) und das Bild darf komprimiert werden
// (allowTC: Shader-Stufen ohne "noTC", tr_shader.cpp:1263/2324; *default; nicht die Lightmaps, r_ext_compress_lightmaps 0), wird
// jede Mip-Stufe als GL_COMPRESSED_RGBA_S3TC_DXT5_EXT hochgeladen (r_texturebits 0; mit 16 DXT1). Den Block packt der Treiber;
// gerechnet wird hier, was der Grafikchip danach liest: je 4x4 Punkte zwei Farben in 5:6:5 und vier Stufen dazwischen
// (Farbblock immer mit vier Farben, DXT5) und fuer Alpha zwei Werte mit acht Stufen. Movie Duels setzt
// r_ext_compress_textures 0 ("...ignoring texture compression", MD9 rd-vanilla tr_init.cpp) - dort bleibt alles, wie es ist.
#ifndef BHED_DXT_H
#define BHED_DXT_H

#include <cstddef>
#include <cstdint>

namespace bhed::dxt {

// Ein Bild (RGBA8, Zeile fuer Zeile) durch DXT5 packen und wieder auspacken - an Ort und Stelle. Teilbloecke am Rand (Kanten
// unter 4 oder nicht durch 4 teilbar) packen nur die vorhandenen Punkte.
void dxt5Rundlauf(std::uint8_t* rgba, int breite, int hoehe, bool mitMannschaft = true);

// MR5 Punkt 2: mehrere Bilder in einem Auftrag (ladeMitStufen: alle Stufen einer Pyramide). Die Blockzeilen aller Bilder gehen
// zusammen an die Arbeitsfaeden (bhed/arbeiter.h); jeder Block rechnet fuer sich, also Byte fuer Byte dasselbe wie nacheinander
// (ctest dxt gegen die Fassung aus MR4).
struct Ebene {
    std::uint8_t* rgba = nullptr;
    int breite = 0;
    int hoehe = 0;
};
// P55: mitMannschaft false rechnet im eigenen Faden (dieselben Bytes) - fuer Nebenfaeden, die die gemeinsame Mannschaft
// (bhed/arbeiter.h) nicht belegen sollen (Bildlager des Karten-Editors).
void dxt5Rundlauf(const Ebene* ebenen, std::size_t anzahl, bool mitMannschaft = true);

}  // namespace bhed::dxt

#endif  // BHED_DXT_H
