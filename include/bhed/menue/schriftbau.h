// schriftbau.h - Menue-Editor M2 Punkt 9: eine Schrift fuer das Spiel bauen
// (fonts/<name>.fontdat und fonts/<name>.tga).
//
// Das Format ist das, was RE_RegisterFont liest (OpenJK code/rd-common/tr_font.h
// dfontdat_t, tr_font.cpp CFontInfo): 256 glyphInfo_t zu je 28 Bytes (short
// width, height, horizAdvance, horizOffset; int baseline; float s, t, s2, t2),
// dahinter short mPointSize, mHeight, mAscender, mDescender, mKoreanHack -
// 7180 Bytes mit Auffuellung. Die Werte wie in Ravens Schriften (z. B.
// fonts/ergoec): baseline = Zeilen ueber der Grundlinie - 1 (die Grundlinie
// zaehlt zum Zeichen), mHeight = Aufstieg + Abstieg; die Grafik ist weiss mit
// der Deckung im Alpha (impliziter 2D-Shader fonts/<name> mischt mit Alpha).
// Index = Latin-1/Windows-1252-Code wie die Menue-Texte. Die Bilder der
// Zeichen kommen von aussen (die Oberflaeche rastert die TTF).
#ifndef BHED_MENUE_SCHRIFTBAU_H
#define BHED_MENUE_SCHRIFTBAU_H

#include <cstdint>
#include <string>
#include <vector>

namespace bhed::menue {

struct GlyphBild {
    int code = 0;              // 0..255
    int breite = 0, hoehe = 0;
    int x0 = 0;                // linker Rand ab dem Stift (horizOffset)
    int y0 = 0;                // oberer Rand relativ zur Grundlinie (oben negativ)
    int vorschub = 0;          // horizAdvance
    std::vector<std::uint8_t> deckung;   // breite * hoehe, Zeile fuer Zeile von oben
};
struct SchriftMasse {
    int punkt = 0;             // mPointSize
    int aufstieg = 0;          // mAscender (positiv)
    int abstieg = 0;           // mDescender (positiv)
};
struct SchriftDateien {
    std::string fontdat;       // 7180 Bytes
    std::string tga;           // 32 Bit, oben links beginnend
    int texBreite = 0, texHoehe = 0;
};
// Packt die Zeichen zeilenweise (1 Bildpunkt Abstand) in eine 256 bzw. 512 breite
// Grafik mit Zweierpotenz-Hoehe.
[[nodiscard]] SchriftDateien baueSchrift(const std::vector<GlyphBild>& zeichen, const SchriftMasse& m);
// Windows-1252: der Unicode-Wert zu einem Byte (0 = keiner)
[[nodiscard]] int cp1252ZuUnicode(int byte);

}  // namespace bhed::menue

#endif  // BHED_MENUE_SCHRIFTBAU_H
