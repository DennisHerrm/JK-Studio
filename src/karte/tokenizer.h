// tokenizer.h - Karten-Editor (intern): der Tokenizer von NetRadiant-Custom
// fuer .map und .shader und die Tokeniser_*-Helfer.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): libs/script/
// scripttokeniser.h (ScriptTokeniser: NewMapTokeniser = special false,
// specialComments true; NewScriptTokeniser = special true), libs/stream/
// textstream.h (SingleCharacterInputStream::bufferContains), libs/stringio.h
// (Tokeniser_getFloat/Double/Integer/Size, parseToken, nextTokenIsDigit,
// skipToNextLine, unexpectedError).
//
//   Copyright (C) 2001-2006, William Joseph. All Rights Reserved.
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_KARTE_TOKENIZER_H
#define BHED_KARTE_TOKENIZER_H

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace bhed::karte::detail {

constexpr std::size_t kMaxToken = 1024;   // MAXTOKEN (include/iscriplib.h:30)

// ===================== Tokenizer (scripttokeniser.h) ========================

class Tokenizer {
public:
    // NewMapTokeniser: special = false, specialComments = true
    Tokenizer(const std::string& text, std::vector<std::string>* meldungen, bool special = false,
              bool specialComments = true)
        : m_text(text), m_meldungen(meldungen), m_special(special), m_specialComments(specialComments) {
        m_stapel[0] = Zustand::Normal;
        m_eof = !zeichenLesen(m_current);
        m_token[kMaxToken - 1] = '\0';
        m_token[0] = '\0';
    }
    void nextLine() { m_crossline = true; }
    const char* getToken() {
        if (m_unget) {
            m_unget = false;
            return m_token;
        }
        return tokenFuellen();
    }
    void ungetToken() { m_unget = true; }
    [[nodiscard]] std::size_t zeile() const { return m_zeile; }
    [[nodiscard]] std::size_t spalte() const { return m_spalte; }
    // SingleCharacterInputStream::bufferContains: sucht in den naechsten
    // (hoechstens) 1024 noch ungelesenen Zeichen.
    [[nodiscard]] bool pufferEnthaelt(const char* s) const {
        const std::size_t ende = std::min(m_text.size(), m_pos + 1024);
        if (m_pos >= ende) { return false; }
        const std::string fenster = m_text.substr(m_pos, ende - m_pos);
        return fenster.find(s) != std::string::npos;
    }
    void melden(const std::string& s) const {
        if (m_meldungen) { m_meldungen->push_back(s); }
    }

private:
    enum class Zeichen { Leer, Token, Zeilenende, Anfuehrung, Schraeg, Stern, Sonder };   // CharType
    enum class Zustand { Normal, Token, Zitat, Schraeg, Kommentar, KommentarSig, Block, BlockEnde, ZitatEnde, Sonder };

    Zeichen zeichenArt(char c) const {   // charType
        switch (c) {
        case '\n': return Zeichen::Zeilenende;
        case '"': return Zeichen::Anfuehrung;
        case '/': return Zeichen::Schraeg;
        case '*': return Zeichen::Stern;
        case '{':
        case '(':
        case '}':
        case ')':
        case '[':
        case ']':
        case ',':
        case ':':
            return m_special ? Zeichen::Sonder : Zeichen::Token;
        default:
            break;
        }
        // char ist vorzeichenbehaftet (MinGW wie MSVC): Bytes ab 0x80 sind Leerraum
        if (static_cast<signed char>(c) > 32) { return Zeichen::Token; }
        return Zeichen::Leer;
    }
    bool zeichenLesen(char& c) {
        if (m_pos >= m_text.size()) { return false; }
        c = m_text[m_pos++];
        return true;
    }
    void push(Zustand z) { m_stapel[++m_tiefe] = z; }
    void pop() { --m_tiefe; }
    void add(char c) {
        if (m_write < kMaxToken - 1) { m_token[m_write++] = c; }
    }
    std::string ort() const { return std::to_string(m_zeile) + ':' + std::to_string(m_spalte); }

    bool schritt(char c) {
        switch (m_stapel[m_tiefe]) {
        case Zustand::Normal:   // tokeniseDefault
            switch (zeichenArt(c)) {
            case Zeichen::Zeilenende:
                if (!m_crossline) {
                    melden(ort() + ": unexpected end-of-line before token");
                    return false;
                }
                break;
            case Zeichen::Token:
            case Zeichen::Stern:
                push(Zustand::Token);
                add(c);
                break;
            case Zeichen::Sonder:
                push(Zustand::Sonder);
                add(c);
                break;
            case Zeichen::Anfuehrung:
                push(Zustand::Zitat);
                break;
            case Zeichen::Schraeg:
                push(Zustand::Schraeg);
                break;
            default:
                break;
            }
            return true;
        case Zustand::Token:   // tokeniseToken
            switch (zeichenArt(c)) {
            case Zeichen::Zeilenende:
            case Zeichen::Leer:
            case Zeichen::Anfuehrung:
            case Zeichen::Sonder:
                pop();
                m_emit = true;
                break;
            case Zeichen::Schraeg:   // MID_TOKEN_COMMENTS 0: faellt durch
            case Zeichen::Token:
            case Zeichen::Stern:
                add(c);
                break;
            }
            return true;
        case Zustand::Zitat:   // tokeniseQuotedToken
            switch (zeichenArt(c)) {
            case Zeichen::Zeilenende:
                if (m_crossline) {
                    melden(ort() + ": unexpected end-of-line in quoted token");
                    return false;
                }
                break;
            case Zeichen::Leer:
            case Zeichen::Token:
            case Zeichen::Schraeg:
            case Zeichen::Stern:
            case Zeichen::Sonder:
                add(c);
                break;
            case Zeichen::Anfuehrung:
                pop();
                push(Zustand::ZitatEnde);
                break;
            }
            return true;
        case Zustand::Schraeg:   // tokeniseSolidus
            switch (zeichenArt(c)) {
            case Zeichen::Zeilenende:
            case Zeichen::Leer:
            case Zeichen::Anfuehrung:
            case Zeichen::Sonder:
                pop();
                add('/');
                m_emit = true;   // einzelner Schraegstrich
                break;
            case Zeichen::Token:
                pop();
                add('/');
                add(c);
                break;
            case Zeichen::Schraeg:
                pop();
                if (m_specialComments) {
                    push(Zustand::KommentarSig);
                    m_sigLesen = 0;
                } else {
                    push(Zustand::Kommentar);
                }
                break;
            case Zeichen::Stern:
                pop();
                push(Zustand::Block);
                break;
            }
            return true;
        case Zustand::Kommentar:   // tokeniseComment
            if (c == '\n') { pop(); }
            return true;
        case Zustand::KommentarSig:   // tokeniseCommentSpecialSig: "//@$&"
            if (kSig[m_sigLesen++] == c) {
                if (kSig[m_sigLesen] == '\0') { pop(); }
                return true;
            }
            pop();
            push(Zustand::Kommentar);
            if (c == '\n') { pop(); }
            return true;
        case Zustand::Block:   // tokeniseBlockComment
            if (c == '*') {
                pop();
                push(Zustand::BlockEnde);
            }
            return true;
        case Zustand::BlockEnde:   // tokeniseEndBlockComment
            switch (c) {
            case '/': pop(); break;
            case '*': break;
            default:
                pop();
                push(Zustand::Block);
                break;
            }
            return true;
        case Zustand::ZitatEnde:   // tokeniseEndQuote
        case Zustand::Sonder:      // tokeniseSpecial
            pop();
            m_emit = true;
            return true;
        }
        return true;
    }

    bool tokenisieren() {   // tokenise
        m_write = 0;
        while (!m_eof) {
            const char c = m_current;
            if (!schritt(c)) {
                m_eof = true;
                return false;
            }
            if (m_emit) {
                m_emit = false;
                return true;
            }
            if (c == '\n') {
                ++m_zeile;
                m_spalte = 1;
            } else {
                ++m_spalte;
            }
            m_eof = !zeichenLesen(m_current);
        }
        return m_write != 0;
    }
    const char* tokenFuellen() {   // fillToken
        if (!tokenisieren()) { return nullptr; }
        add('\0');
        return m_token;
    }

    static constexpr char kSig[4] = "@$&";
    const std::string& m_text;
    std::vector<std::string>* m_meldungen;
    std::size_t m_pos = 0;
    Zustand m_stapel[3]{};
    std::size_t m_tiefe = 0;
    std::size_t m_zeile = 1;
    std::size_t m_spalte = 1;
    char m_token[kMaxToken]{};
    std::size_t m_write = 0;
    char m_current = 0;
    bool m_eof = false;
    bool m_crossline = false;
    bool m_unget = false;
    bool m_emit = false;
    const bool m_special;
    const bool m_specialComments;
    std::size_t m_sigLesen = 0;
};

// ===================== Tokeniser_* (stringio.h) =============================

inline void unerwartet(const Tokenizer& t, const char* token, const char* erwartet) {   // Tokeniser_unexpectedError
    t.melden(std::to_string(t.zeile()) + ':' + std::to_string(t.spalte()) + ": parse error at '" +
             (token != nullptr ? token : "#EOF") + "': expected '" + erwartet + "'");
}

// string_parse_float / string_parse_double: strtod, ganzer Text muss passen
inline bool zahlLesen(const char* s, double& d) {
    if (*s == '\0') { return false; }
    char* ende = nullptr;
    d = std::strtod(s, &ende);
    return *ende == '\0';
}

inline bool floatHolen(Tokenizer& t, float& f) {   // Tokeniser_getFloat
    const char* token = t.getToken();
    double d = 0;
    if (token != nullptr && zahlLesen(token, d)) {
        f = static_cast<float>(d);
        return true;
    }
    // 1.#IND 1.#INF 1.#QNAN: Warnung, Wert bleibt
    if (token != nullptr && std::strstr(token, ".#")) {
        t.melden("Warning: " + std::to_string(t.zeile()) + ':' + std::to_string(t.spalte()) + ": expected parse problem at '" +
                 token + "': wanted '#number'\nProcessing anyway");
        return true;
    }
    unerwartet(t, token, "#number");
    return false;
}

inline bool doubleHolen(Tokenizer& t, double& d) {   // Tokeniser_getDouble
    const char* token = t.getToken();
    if (token != nullptr && zahlLesen(token, d)) { return true; }
    unerwartet(t, token, "#number");
    return false;
}

inline bool ganzzahlHolen(Tokenizer& t, int& i) {   // Tokeniser_getInteger (strtol, Basis 10)
    const char* token = t.getToken();
    if (token != nullptr && *token != '\0') {
        char* ende = nullptr;
        i = static_cast<int>(std::strtol(token, &ende, 10));
        if (*ende == '\0') { return true; }
    }
    unerwartet(t, token, "#integer");
    return false;
}

inline bool groesseHolen(Tokenizer& t, std::size_t& n) {   // Tokeniser_getSize (strtoul, als int gespeichert)
    const char* token = t.getToken();
    if (token != nullptr && *token != '\0') {
        char* ende = nullptr;
        // buffer_parse_unsigned_decimal_integer_literal gibt int zurueck
        n = static_cast<std::size_t>(static_cast<int>(std::strtoul(token, &ende, 10)));
        if (*ende == '\0') { return true; }
    }
    unerwartet(t, token, "#unsigned-integer");
    return false;
}

inline bool tokenErwarten(Tokenizer& t, const char* erwartet) {   // Tokeniser_parseToken
    const char* token = t.getToken();
    if (token != nullptr && std::strcmp(token, erwartet) == 0) { return true; }
    unerwartet(t, token, erwartet);
    return false;
}

inline bool naechstesIstZiffer(Tokenizer& t) {   // Tokeniser_nextTokenIsDigit
    const char* token = t.getToken();
    if (token == nullptr) { return false; }
    const char c = *token;
    t.ungetToken();
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

inline void zurNaechstenZeile(Tokenizer& t) {   // Tokeniser_skipToNextLine
    const std::size_t zeile = t.zeile();
    while (t.getToken() && zeile == t.zeile()) {}
    t.ungetToken();
}

#define RUECK_FALSCH(ausdruck) do { if (!(ausdruck)) { return false; } } while (false)   // RETURN_FALSE_IF_FAIL

}  // namespace bhed::karte::detail
#endif
