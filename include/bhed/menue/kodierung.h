// kodierung.h - Menue-Editor M3 Punkt 7: eigene Texte in den Kodierungen, in
// denen die Engine sie fuer Koreanisch, Taiwanesisch, Japanisch und
// Chinesisch liest, sonst Latin-1 wie die Menues.
//
// Portiert aus OpenJK code/rd-common/tr_font.cpp (GetLanguageEnum,
// Korean_ValidKSC5601Hangul, Taiwanese_ValidBig5Code,
// Japanese_ValidShiftJISCode, Chinese_ValidGBCode, *_Collapse*Code,
// *_InitFields, AnyLanguage_ReadCharFromString, CFontInfo::UpdateAsianIfNeeded,
// CFontInfo::GetLetter) und code/qcommon/stringed_ingame.h (Language_Is*).
//
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
//
// Die Engine liest die Bytes der .str-Dateien unveraendert; erst die Schrift
// entscheidet je se_language, ob zwei Bytes ein Zeichen sind (KSC5601-Hangul,
// Big5, Shift-JIS, GB2312) und nimmt dann die Glyphenseiten
// fonts/<kor|tai|jap|chi>_<n>_1024_<seite>. Thai (TIS-620 mit fonts/tha_codes.dat)
// ist nicht nachgebildet und bleibt Latin-1.
#ifndef BHED_MENUE_KODIERUNG_H
#define BHED_MENUE_KODIERUNG_H

#include <cstddef>
#include <string>

namespace bhed::menue {

struct Glyph;
struct Schrift;

enum class Kodierung { Latin1, Koreanisch, Taiwanesisch, Japanisch, Chinesisch };

// GetLanguageEnum / Language_Is*: se_language ohne Gross/Klein ("korean", "taiwanese",
// "japanese", "chinese"), alles andere Latin-1.
[[nodiscard]] Kodierung kodierungDerSprache(const std::string& seLanguage);
// Language_IsAsian (ohne Thai)
[[nodiscard]] bool asiatisch(Kodierung k);

// AnyLanguage_ReadCharFromString: das Zeichen ab p; laenge = 2 bei einem gueltigen
// Doppelbyte-Code der Sprache, sonst 1 (das Byte selbst). rest = Bytes ab p.
[[nodiscard]] unsigned zeichenLesen(Kodierung k, const unsigned char* p, std::size_t rest, int& laenge);

// CFontInfo::GetCollapsedAsianCode: Index der Glyphe in den Seiten, 0 = keine
// asiatische Glyphe (auch fuer den ersten Code jeder Tabelle - wie die Engine).
[[nodiscard]] int asienIndex(Kodierung k, unsigned code);
// Seitenname wie UpdateAsianIfNeeded ("fonts/kor_32_1024_0"), seite 0..seiten-1
[[nodiscard]] std::string asienSeite(Kodierung k, int seite);
[[nodiscard]] int asienSeiten(Kodierung k);
// CFontInfo::GetLetter fuer einen Code > 255: Glyphe aus den Seiten (Groesse wie die
// westliche Schrift, mindestens 16) und der Shader der Seite. false = keine.
[[nodiscard]] bool asienGlyph(Kodierung k, unsigned code, const Schrift& westlich, Glyph& g, std::string& shader);

// UTF-8 der Oberflaeche -> Bytes in der Kodierung der Sprache (Latin-1, CP949, CP950,
// CP932, CP936). Zeichen, die die Kodierung nicht hat oder die die Schrift der Engine
// nicht als ein Zeichen liest (z. B. Hangul ausserhalb KSC5601, Halbbreiten-Katakana),
// werden '?' und landen (UTF-8) in nichtDarstellbar.
[[nodiscard]] std::string ausUtf8(const std::string& utf8, Kodierung k, std::string* nichtDarstellbar = nullptr);
// Bytes der Kodierung -> UTF-8 fuer die Anzeige
[[nodiscard]] std::string zuUtf8(const std::string& bytes, Kodierung k);

}  // namespace bhed::menue

#endif  // BHED_MENUE_KODIERUNG_H
