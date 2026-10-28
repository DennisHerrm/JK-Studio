// lexer.cpp - Menue-Editor M1: die beiden Wortleser der Engine.
//
// Portiert aus OpenJK: code/qcommon/q_shared.cpp (SkipWhitespace,
// COM_ParseExt, COM_ParseString/Int/Float) und codemp/botlib/l_script.cpp
// (PS_ReadWhiteSpace, PS_ReadEscapeCharacter, PS_ReadString, PS_ReadName,
// PS_ReadNumber/NumberValue, PS_ReadPunctuation, PS_ReadToken) sowie
// codemp/botlib/l_precomp.cpp (PC_ReadSourceToken, PC_ReadLine,
// PC_ReadDirective, PC_Directive_*, PC_ExpandDefine, PC_ReadDefineParms,
// PC_StringizeTokens, PC_MergeTokens, PC_Evaluate, PC_ReadToken,
// PC_LoadGlobalDefines, PC_ReadTokenHandle). Die Eigenheiten bleiben, auch
// wo sie Fehler sind (siehe die Anmerkungen "wie botlib").
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
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
#include "lexer.h"

#include <cstdlib>
#include <cstring>

namespace bhed::menue::intern {

namespace {

constexpr std::size_t kMaxTokenChars = 1024;   // MAX_TOKEN_CHARS (q_shared.h)
constexpr std::size_t kMaxToken = 1024;        // MAX_TOKEN (l_script.h)

// subtype-Bits (l_script.h)
constexpr int TT_DECIMAL = 0x0008;
constexpr int TT_HEX = 0x0100;
constexpr int TT_OCTAL = 0x0200;
constexpr int TT_BINARY = 0x0400;
constexpr int TT_FLOAT = 0x0800;
constexpr int TT_INTEGER = 0x1000;
constexpr int TT_LONG = 0x2000;
constexpr int TT_UNSIGNED = 0x4000;

constexpr int INDENT_IF = 0x0001;
constexpr int INDENT_ELSE = 0x0002;
constexpr int INDENT_ELIF = 0x0004;
constexpr int INDENT_IFDEF = 0x0008;
constexpr int INDENT_IFNDEF = 0x0010;

// default_punctuations (l_script.cpp), laengste zuerst wie dort.
const char* const kPunkte[] = {">>=", "<<=", "...", "##", "&&", "||", ">=", "<=", "==", "!=", "*=", "/=", "%=",
                               "+=",  "-=",  "++",  "--", "&=", "|=", "^=", ">>", "<<", "->", "::", ".*", "*",
                               "/",   "%",   "+",   "-",  "=",  "&",  "|",  "^",  "~",  "!",  ">",  "<",  ".",
                               ",",   ";",   ":",   "?",  "(",  ")",  "{",  "}",  "[",  "]",  "\\", "#",  "$",
                               "@"};

char an(const std::string& t, std::size_t p) noexcept { return p < t.size() ? t[p] : '\0'; }

}  // namespace

// ============================================================================
// SP: COM_ParseExt
// ============================================================================

Token ComLeser::parseExt(bool allowLineBreaks) {
    Token t;
    t.typ = TT_NAME;
    if (ende_) {
        t.anfang = t.ende = static_cast<std::uint32_t>(text_.size());
        t.zeile = lines_;
        return t;
    }
    bool hasNewLines = false;
    char c = 0;
    while (true) {
        // SkipWhitespace ("eurofix": unsigned)
        while (true) {
            const auto u = static_cast<unsigned char>(an(text_, p_));
            if (u > ' ') { break; }
            if (u == 0) {
                ende_ = true;
                t.anfang = t.ende = static_cast<std::uint32_t>(p_);
                t.zeile = lines_;
                return t;
            }
            if (u == '\n') {
                ++lines_;
                hasNewLines = true;
            }
            ++p_;
        }
        if (hasNewLines && !allowLineBreaks) {
            t.anfang = t.ende = static_cast<std::uint32_t>(p_);
            t.zeile = lines_;
            return t;
        }
        c = an(text_, p_);
        if (c == '/' && an(text_, p_ + 1) == '/') {
            p_ += 2;
            while (an(text_, p_) != '\0' && an(text_, p_) != '\n') { ++p_; }
        } else if (c == '/' && an(text_, p_ + 1) == '*') {
            p_ += 2;
            while (an(text_, p_) != '\0' && (an(text_, p_) != '*' || an(text_, p_ + 1) != '/')) {
                if (an(text_, p_) == '\n') { ++lines_; }
                ++p_;
            }
            if (an(text_, p_) != '\0') { p_ += 2; }
        } else {
            break;
        }
    }
    t.zeile = lines_;
    t.anfang = static_cast<std::uint32_t>(p_);
    if (c == '"') {
        t.typ = TT_STRING;
        ++p_;
        while (true) {
            const char d = an(text_, p_);
            if (p_ < text_.size()) { ++p_; }
            if (d == '"' || d == '\0') {
                if (d == '\0') { ende_ = true; }   // die Engine liefe ueber das Pufferende
                t.ende = static_cast<std::uint32_t>(p_);
                return t;
            }
            if (d == '\n') { ++lines_; }
            if (t.s.size() < kMaxTokenChars - 1) { t.s += d; }
        }
    }
    // Ein Wort: bis zu einem Zeichen <= 32 - VORZEICHENBEHAFTET wie die Engine
    // (c ist int aus signed char): Bytes ab 0x80 beenden das Wort.
    do {
        if (t.s.size() < kMaxTokenChars - 1) { t.s += c; }
        ++p_;
        c = an(text_, p_);
    } while (static_cast<signed char>(c) > 32);
    t.ende = static_cast<std::uint32_t>(p_);
    return t;
}

// PC_ParseString: COM_ParseString liefert nie einen Fehler (die Engine prueft
// `s[0] == 0`, einen Zeiger) - PC_ParseString liest weiter, bis ein Wort
// kommt. Am Dateiende liefe die Engine endlos; hier ist das ein Fehler.
bool ComLeser::parseString(Token& t) {
    while (true) {
        t = parseExt(false);
        if (!t.s.empty() || t.typ == TT_STRING) {
            if (!t.s.empty()) { return false; }
            // "" ist fuer die Engine leer: weiterlesen wie sie.
        }
        if (ende_) {
            if (printf != nullptr) { printf->push_back("unexpected EOF in COM_ParseString"); }
            return true;
        }
    }
}

bool ComLeser::parseInt(int& v, Token& t) {
    t = parseExt(false);
    if (t.s.empty()) {
        if (printf != nullptr) { printf->push_back("unexpected EOF in COM_ParseInt"); }
        return true;
    }
    v = std::atoi(t.s.c_str());
    return false;
}

bool ComLeser::parseFloat(float& v, Token& t) {
    t = parseExt(false);
    if (t.s.empty()) {
        if (printf != nullptr) { printf->push_back("unexpected EOF in COM_ParseFloat"); }
        return true;
    }
    v = static_cast<float>(std::atof(t.s.c_str()));
    return false;
}

// ============================================================================
// MP: botlib l_script.cpp
// ============================================================================

void PcQuelle::fehler(const std::string& text) {
    Meldung m;
    if (!stapel_.empty()) {
        m.datei = stapel_.back().datei;
        m.zeile = stapel_.back().line;
    }
    m.text = text;
    m.fehler = true;
    meld_.push_back(std::move(m));
}

void PcQuelle::warnung(const std::string& text) {
    fehler(text);
    meld_.back().fehler = false;
}

void PcQuelle::uiFehler(const std::string& text) { fehler(text); }

std::string PcQuelle::dateiName() const {
    return stapel_.empty() ? std::string() : quellen_[static_cast<std::size_t>(stapel_.back().datei)].name;
}
int PcQuelle::zeile() const { return stapel_.empty() ? 0 : stapel_.back().line; }
int PcQuelle::dateiIndex() const { return stapel_.empty() ? 0 : stapel_.back().datei; }

// PS_ReadWhiteSpace
bool PcQuelle::psLeer(Skript& s) {
    const std::string& t = text(s);
    while (true) {
        while (static_cast<unsigned char>(an(t, s.p)) <= ' ') {
            if (an(t, s.p) == '\0') { return false; }
            if (t[s.p] == '\n') { ++s.line; }
            ++s.p;
        }
        if (an(t, s.p) == '/') {
            if (an(t, s.p + 1) == '/') {
                ++s.p;
                do {
                    ++s.p;
                    if (an(t, s.p) == '\0') { return false; }
                } while (t[s.p] != '\n');
                ++s.line;
                ++s.p;
                if (an(t, s.p) == '\0') { return false; }
                continue;
            }
            if (an(t, s.p + 1) == '*') {
                ++s.p;
                do {
                    ++s.p;
                    if (an(t, s.p) == '\0') { return false; }
                    if (t[s.p] == '\n') { ++s.line; }
                } while (!(t[s.p] == '*' && an(t, s.p + 1) == '/'));
                ++s.p;
                if (an(t, s.p) == '\0') { return false; }
                ++s.p;
                if (an(t, s.p) == '\0') { return false; }
                continue;
            }
        }
        break;
    }
    return true;
}

// PS_ReadEscapeCharacter
bool PcQuelle::psEscape(Skript& s, char& ch) {
    const std::string& t = text(s);
    ++s.p;
    int c = 0;
    switch (an(t, s.p)) {
        case '\\': c = '\\'; break;
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        case 'v': c = '\v'; break;
        case 'b': c = '\b'; break;
        case 'f': c = '\f'; break;
        case 'a': c = '\a'; break;
        case '\'': c = '\''; break;
        case '"': c = '"'; break;
        case '?': c = '?'; break;
        case 'x': {
            ++s.p;
            int val = 0;
            for (;; ++s.p) {
                int d = an(t, s.p);
                if (d >= '0' && d <= '9') { d -= '0'; }
                else if (d >= 'A' && d <= 'Z') { d = d - 'A' + 10; }
                else if (d >= 'a' && d <= 'z') { d = d - 'a' + 10; }
                else { break; }
                val = (val << 4) + d;
            }
            --s.p;
            if (val > 0xFF) {
                warnung("too large value in escape character");
                val = 0xFF;
            }
            c = val;
            break;
        }
        default: {   // dezimal, NICHT oktal (wie botlib)
            if (an(t, s.p) < '0' || an(t, s.p) > '9') { fehler("unknown escape char"); }
            int val = 0;
            for (;; ++s.p) {
                const int d = an(t, s.p);
                if (d < '0' || d > '9') { break; }
                val = val * 10 + (d - '0');
            }
            --s.p;
            if (val > 0xFF) {
                warnung("too large value in escape character");
                val = 0xFF;
            }
            c = val;
            break;
        }
    }
    ++s.p;
    ch = static_cast<char>(c);
    return true;
}

// PS_ReadString - mit dem Zusammenfuegen aufeinanderfolgender Strings.
bool PcQuelle::psString(Skript& s, RohToken& tok, char quote) {
    const std::string& t = text(s);
    tok.typ = (quote == '"') ? TT_STRING : TT_LITERAL;
    tok.s.assign(1, t[s.p++]);
    while (true) {
        if (tok.s.size() >= kMaxToken - 2) {
            fehler("string longer than MAX_TOKEN = 1024");
            return false;
        }
        const char c = an(t, s.p);
        if (c == '\\') {
            char ch = 0;
            if (!psEscape(s, ch)) { return false; }
            tok.s += ch;
        } else if (c == quote) {
            ++s.p;
            const std::size_t merkP = s.p;
            const int merkL = s.line;
            if (!psLeer(s)) {
                s.p = merkP;
                s.line = merkL;
                break;
            }
            if (an(t, s.p) != quote) {
                s.p = merkP;
                s.line = merkL;
                break;
            }
            ++s.p;
        } else {
            if (c == '\0') {
                fehler("missing trailing quote");
                return false;
            }
            if (c == '\n') {
                fehler("newline inside string " + tok.s);
                return false;
            }
            tok.s += c;
            ++s.p;
        }
    }
    tok.s += quote;
    tok.subtyp = static_cast<int>(tok.s.size());
    return true;
}

void PcQuelle::psName(Skript& s, RohToken& tok) {
    const std::string& t = text(s);
    tok.typ = TT_NAME;
    char c = 0;
    do {
        tok.s += t[s.p++];
        c = an(t, s.p);
    } while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
    tok.subtyp = static_cast<int>(tok.s.size());
}

namespace {
// NumberValue (l_script.cpp)
void zahlWert(const std::string& str, int subtype, unsigned long& iv, long double& fv) {
    iv = 0;
    fv = 0;
    std::size_t i = 0;
    if ((subtype & TT_FLOAT) != 0) {
        unsigned long dotfound = 0;
        while (i < str.size()) {
            if (str[i] == '.') {
                if (dotfound != 0) { return; }
                dotfound = 10;
                ++i;
            }
            if (i >= str.size()) { break; }
            if (dotfound != 0) {
                fv = fv + static_cast<long double>(str[i] - '0') / static_cast<long double>(dotfound);
                dotfound *= 10;
            } else {
                fv = fv * 10.0L + static_cast<long double>(str[i] - '0');
            }
            ++i;
        }
        iv = static_cast<unsigned long>(fv);
    } else if ((subtype & TT_DECIMAL) != 0) {
        while (i < str.size()) { iv = iv * 10 + static_cast<unsigned long>(str[i++] - '0'); }
        fv = static_cast<long double>(iv);
    } else if ((subtype & TT_HEX) != 0) {
        i = 2;
        while (i < str.size()) {
            iv <<= 4;
            const char c = str[i];
            if (c >= 'a' && c <= 'f') { iv += static_cast<unsigned long>(c - 'a' + 10); }
            else if (c >= 'A' && c <= 'F') { iv += static_cast<unsigned long>(c - 'A' + 10); }
            else { iv += static_cast<unsigned long>(c - '0'); }
            ++i;
        }
        fv = static_cast<long double>(iv);
    } else if ((subtype & TT_OCTAL) != 0) {
        i = 1;
        while (i < str.size()) { iv = (iv << 3) + static_cast<unsigned long>(str[i++] - '0'); }
        fv = static_cast<long double>(iv);
    } else if ((subtype & TT_BINARY) != 0) {
        i = 2;
        while (i < str.size()) { iv = (iv << 1) + static_cast<unsigned long>(str[i++] - '0'); }
        fv = static_cast<long double>(iv);
    }
}
}  // namespace

// PS_ReadNumber
void PcQuelle::psZahl(Skript& s, RohToken& tok) {
    const std::string& t = text(s);
    tok.typ = TT_NUMBER;
    const char n1 = an(t, s.p + 1);
    if (an(t, s.p) == '0' && (n1 == 'x' || n1 == 'X')) {
        tok.s += t[s.p++];
        tok.s += t[s.p++];
        char c = an(t, s.p);
        while ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            tok.s += t[s.p++];
            c = an(t, s.p);
        }
        tok.subtyp |= TT_HEX;
    } else if (an(t, s.p) == '0' && (n1 == 'b' || n1 == 'B')) {
        tok.s += t[s.p++];
        tok.s += t[s.p++];
        char c = an(t, s.p);
        while (c == '0' || c == '1') {
            tok.s += t[s.p++];
            c = an(t, s.p);
        }
        tok.subtyp |= TT_BINARY;
    } else {
        bool octal = an(t, s.p) == '0';
        bool dot = false;
        while (true) {
            const char c = an(t, s.p);
            if (c == '.') { dot = true; }
            else if (c == '8' || c == '9') { octal = false; }
            else if (c < '0' || c > '9') { break; }
            tok.s += t[s.p++];
            if (tok.s.size() >= kMaxToken - 1) { break; }
        }
        tok.subtyp |= octal ? TT_OCTAL : TT_DECIMAL;
        if (dot) { tok.subtyp |= TT_FLOAT; }
    }
    for (int i = 0; i < 2; ++i) {
        const char c = an(t, s.p);
        if ((c == 'l' || c == 'L') && (tok.subtyp & TT_LONG) == 0) {
            ++s.p;
            tok.subtyp |= TT_LONG;
        } else if ((c == 'u' || c == 'U') && (tok.subtyp & (TT_UNSIGNED | TT_FLOAT)) == 0) {
            ++s.p;
            tok.subtyp |= TT_UNSIGNED;
        }
    }
    zahlWert(tok.s, tok.subtyp, tok.ival, tok.fval);
    if ((tok.subtyp & TT_FLOAT) == 0) { tok.subtyp |= TT_INTEGER; }
}

bool PcQuelle::psPunkt(Skript& s, RohToken& tok) {
    const std::string& t = text(s);
    for (std::size_t i = 0; i < sizeof(kPunkte) / sizeof(kPunkte[0]); ++i) {
        const std::size_t len = std::strlen(kPunkte[i]);
        if (s.p + len <= t.size() && t.compare(s.p, len, kPunkte[i]) == 0) {
            tok.s = kPunkte[i];
            s.p += len;
            tok.typ = TT_PUNCTUATION;
            tok.subtyp = static_cast<int>(i) + 1;
            return true;
        }
    }
    return false;
}

// PS_ReadToken
bool PcQuelle::psLies(Skript& s, RohToken& tok) {
    s.lastline = s.line;
    tok = RohToken{};
    const std::size_t leerAnfang = s.p;
    if (!psLeer(s)) {
        s.ende = true;
        return false;
    }
    tok.leerVorher = s.p > leerAnfang;
    tok.zeile = s.line;
    tok.linescrossed = s.line - s.lastline;
    tok.datei = s.datei;
    tok.anfang = static_cast<std::uint32_t>(s.p);
    const std::string& t = text(s);
    const char c = t[s.p];
    if (c == '"') {
        if (!psString(s, tok, '"')) { return false; }
    } else if (c == '\'') {
        if (!psString(s, tok, '\'')) { return false; }
    } else if ((c >= '0' && c <= '9') || (c == '.' && an(t, s.p + 1) >= '0' && an(t, s.p + 1) <= '9')) {
        psZahl(s, tok);
    } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '@') {
        psName(s, tok);
    } else if (!psPunkt(s, tok)) {
        fehler("can't read token");
        return false;
    }
    tok.ende = static_cast<std::uint32_t>(s.p);
    return true;
}

// ============================================================================
// MP: botlib l_precomp.cpp
// ============================================================================

bool PcQuelle::ladeDatei(const std::string& pfadRoh, int& index) {
    // PC_ConvertPath: doppelte Trenner weg; die Engine nimmt / und \ gleich.
    std::string pfad;
    for (const char c : pfadRoh) {
        const char d = (c == '\\') ? '/' : c;
        if (d == '/' && !pfad.empty() && pfad.back() == '/') { continue; }
        pfad += d;
    }
    for (std::size_t i = 0; i < quellen_.size(); ++i) {
        // dieselbe Datei nicht zweimal halten (ihr Text ist unveraendert)
        if (quellen_[i].name == pfad && i != 0) {
            index = static_cast<int>(i);
            return true;
        }
    }
    std::string inhalt;
    if (!lader_ || !lader_(pfad, inhalt)) { return false; }
    quellen_.push_back(Quelle{pfad, std::move(inhalt)});
    index = static_cast<int>(quellen_.size() - 1);
    return true;
}

void PcQuelle::oeffne(int datei) {
    stapel_.clear();
    tokens_.clear();
    einrueck_.clear();
    skip_ = 0;
    defines_ = globale;   // PC_AddGlobalDefinesToSource
    // PC_AddBuiltinDefines
    const char* const eingebaut[] = {"__LINE__", "__FILE__", "__DATE__", "__TIME__"};
    for (int i = 0; i < 4; ++i) {
        Define d;
        d.name = eingebaut[i];
        d.builtin = i + 1;
        d.fixed = true;
        defines_[d.name] = d;
    }
    Skript s;
    s.datei = datei;
    stapel_.push_back(s);
}

bool PcQuelle::ladeGlobal(const std::string& pfad) {
    int index = 0;
    if (!ladeDatei(pfad, index)) { return false; }
    oeffne(index);
    alsGlobal_ = true;
    RohToken t;
    while (pcLies(t)) {}
    alsGlobal_ = false;
    stapel_.clear();
    return true;
}

// PC_ReadSourceToken
bool PcQuelle::quelleLies(RohToken& t) {
    while (tokens_.empty()) {
        if (stapel_.empty()) { return false; }
        Skript& s = stapel_.back();
        if (psLies(s, t)) { return true; }
        if (s.p >= text(s).size() || s.ende) {
            while (!einrueck_.empty() && einrueck_.back().skript == stapel_.size() - 1) {
                warnung("missing #endif");
                int typ = 0;
                bool skip = false;
                einrueckPop(typ, skip);
            }
        }
        if (stapel_.size() == 1) { return false; }
        stapel_.pop_back();
    }
    t = tokens_.front();
    tokens_.pop_front();
    return true;
}

void PcQuelle::quelleZurueck(const RohToken& t) { tokens_.push_front(t); }

bool PcQuelle::zeileLies(RohToken& t) {
    int crossline = 0;
    do {
        if (!quelleLies(t)) { return false; }
        if (t.linescrossed > crossline) {
            quelleZurueck(t);
            return false;
        }
        crossline = 1;
    } while (t.s == "\\");
    return true;
}

void PcQuelle::einrueckPush(int typ, bool skip) {
    Einrueck e;
    e.typ = typ;
    e.skip = skip;
    e.skript = stapel_.empty() ? 0 : stapel_.size() - 1;
    skip_ += skip ? 1 : 0;
    einrueck_.push_back(e);
}

void PcQuelle::einrueckPop(int& typ, bool& skip) {
    typ = 0;
    skip = false;
    if (einrueck_.empty()) { return; }
    if (einrueck_.back().skript != (stapel_.empty() ? 0 : stapel_.size() - 1)) { return; }
    typ = einrueck_.back().typ;
    skip = einrueck_.back().skip;
    skip_ -= skip ? 1 : 0;
    einrueck_.pop_back();
}

bool PcQuelle::include() {
    if (skip_ > 0) { return true; }
    RohToken t;
    if (!quelleLies(t)) {
        fehler("#include without file name");
        return false;
    }
    if (t.linescrossed > 0) {
        fehler("#include without file name");
        return false;
    }
    std::string pfad;
    if (t.typ == TT_STRING) {
        pfad = t.s.substr(1, t.s.size() >= 2 ? t.s.size() - 2 : 0);   // StripDoubleQuotes
    } else if (t.typ == TT_PUNCTUATION && t.s == "<") {
        while (quelleLies(t)) {
            if (t.linescrossed > 0) {
                quelleZurueck(t);
                break;
            }
            if (t.typ == TT_PUNCTUATION && t.s == ">") { break; }
            pfad += t.s;
        }
        if (t.s != ">") { warnung("#include missing trailing >"); }
        if (pfad.empty()) {
            fehler("#include without file name between < >");
            return false;
        }
    } else {
        fehler("#include without file name");
        return false;
    }
    int index = 0;
    if (!ladeDatei(pfad, index)) {
        fehler("file " + pfad + " not found");
        return false;
    }
    Skript s;
    s.datei = index;
    stapel_.push_back(s);
    return true;
}

bool PcQuelle::undef() {
    if (skip_ > 0) { return true; }
    RohToken t;
    if (!zeileLies(t)) {
        fehler("undef without name");
        return false;
    }
    if (t.typ != TT_NAME) {
        quelleZurueck(t);
        fehler("expected name, found " + t.s);
        return false;
    }
    const auto it = defines_.find(t.s);
    if (it != defines_.end()) {
        if (it->second.fixed) { warnung("can't undef " + t.s); }
        else { defines_.erase(it); }
    }
    return true;
}

bool PcQuelle::define() {
    if (skip_ > 0) { return true; }
    RohToken t;
    if (!zeileLies(t)) {
        fehler("#define without name");
        return false;
    }
    if (t.typ != TT_NAME) {
        quelleZurueck(t);
        fehler("expected name after #define, found " + t.s);
        return false;
    }
    const auto alt = defines_.find(t.s);
    if (alt != defines_.end()) {
        if (alt->second.fixed) {
            fehler("can't redefine " + t.s);
            return false;
        }
        warnung("redefinition of " + t.s);
        defines_.erase(alt);
    }
    Define d;
    d.name = t.s;
    auto ablegen = [&]() {
        defines_[d.name] = d;
        if (alsGlobal_) { globale[d.name] = d; }
    };
    if (!zeileLies(t)) {
        ablegen();
        return true;
    }
    if (!t.leerVorher && t.s == "(") {
        RohToken n;
        bool leer = false;
        if (quelleLies(n)) {   // PC_CheckTokenString(source, ")")
            if (n.s == ")") { leer = true; }
            else { quelleZurueck(n); }
        }
        if (!leer) {
            while (true) {
                if (!zeileLies(t)) {
                    fehler("expected define parameter");
                    return false;
                }
                if (t.typ != TT_NAME) {
                    fehler("invalid define parameter");
                    return false;
                }
                for (const std::string& p : d.parms) {
                    if (p == t.s) {
                        fehler("two the same define parameters");
                        return false;
                    }
                }
                d.parms.push_back(t.s);
                if (!zeileLies(t)) {
                    fehler("define parameters not terminated");
                    return false;
                }
                if (t.s == ")") { break; }
                if (t.s != ",") {
                    fehler("define not terminated");
                    return false;
                }
            }
        }
        if (!zeileLies(t)) {
            ablegen();
            return true;
        }
    }
    do {
        if (t.typ == TT_NAME && t.s == d.name) {
            fehler("recursive define (removed recursion)");
            continue;
        }
        RohToken k = t;
        k.leerVorher = false;   // PC_ClearTokenWhiteSpace
        k.linescrossed = 0;
        d.tokens.push_back(k);
    } while (zeileLies(t));
    if (!d.tokens.empty() && (d.tokens.front().s == "##" || d.tokens.back().s == "##")) {
        ablegen();
        fehler("define with misplaced ##");
        return false;
    }
    ablegen();
    return true;
}

bool PcQuelle::ifdef(int typ) {
    RohToken t;
    if (!zeileLies(t)) {
        fehler("#ifdef without name");
        return false;
    }
    if (t.typ != TT_NAME) {
        quelleZurueck(t);
        fehler("expected name after #ifdef, found " + t.s);
        return false;
    }
    const bool da = defines_.count(t.s) != 0;
    einrueckPush(typ, (typ == INDENT_IFDEF) == !da);
    return true;
}

bool PcQuelle::sonst() {
    int typ = 0;
    bool skip = false;
    einrueckPop(typ, skip);
    if (typ == 0) {
        fehler("misplaced #else");
        return false;
    }
    if (typ == INDENT_ELSE) {
        fehler("#else after #else");
        return false;
    }
    einrueckPush(INDENT_ELSE, !skip);
    return true;
}

bool PcQuelle::endif() {
    int typ = 0;
    bool skip = false;
    einrueckPop(typ, skip);
    if (typ == 0) {
        fehler("misplaced #endif");
        return false;
    }
    return true;
}

namespace {

// PC_EvaluateTokens, als Rangfolge-Abstieg mit den C-Prioritaeten von
// PC_OperatorPriority.
struct Auswerter {
    const std::vector<RohToken>& t;
    std::size_t i = 0;
    bool fehler = false;
    const RohToken* an() const { return i < t.size() ? &t[i] : nullptr; }
    bool ist(const char* s) const { return an() != nullptr && an()->typ == TT_PUNCTUATION && an()->s == s; }
    long primaer() {
        if (an() == nullptr) {
            fehler = true;
            return 0;
        }
        if (ist("(")) {
            ++i;
            const long v = bedingt();
            if (!ist(")")) { fehler = true; }
            ++i;
            return v;
        }
        if (ist("-")) { ++i; return -primaer(); }
        if (ist("+")) { ++i; return primaer(); }
        if (ist("!")) { ++i; return primaer() == 0 ? 1 : 0; }
        if (ist("~")) { ++i; return ~primaer(); }
        if (an()->typ == TT_NUMBER) { return static_cast<long>(t[i++].ival); }
        fehler = true;
        ++i;
        return 0;
    }
    static int rang(const std::string& op) {
        if (op == "*" || op == "/" || op == "%") { return 10; }
        if (op == "+" || op == "-") { return 9; }
        if (op == "<<" || op == ">>") { return 8; }
        if (op == "<" || op == ">" || op == "<=" || op == ">=") { return 7; }
        if (op == "==" || op == "!=") { return 6; }
        if (op == "&") { return 5; }
        if (op == "^") { return 4; }
        if (op == "|") { return 3; }
        if (op == "&&") { return 2; }
        if (op == "||") { return 1; }
        return 0;
    }
    long binaer(int min) {
        long l = primaer();
        while (!fehler && an() != nullptr && an()->typ == TT_PUNCTUATION) {
            const std::string op = an()->s;
            const int r = rang(op);
            if (r == 0 || r < min) { break; }
            ++i;
            const long rr = binaer(r + 1);
            if (op == "*") { l = l * rr; }
            else if (op == "/") { if (rr == 0) { fehler = true; return 0; } l = l / rr; }
            else if (op == "%") { if (rr == 0) { fehler = true; return 0; } l = l % rr; }
            else if (op == "+") { l = l + rr; }
            else if (op == "-") { l = l - rr; }
            else if (op == "<<") { l = l << rr; }
            else if (op == ">>") { l = l >> rr; }
            else if (op == "<") { l = l < rr; }
            else if (op == ">") { l = l > rr; }
            else if (op == "<=") { l = l <= rr; }
            else if (op == ">=") { l = l >= rr; }
            else if (op == "==") { l = l == rr; }
            else if (op == "!=") { l = l != rr; }
            else if (op == "&") { l = l & rr; }
            else if (op == "^") { l = l ^ rr; }
            else if (op == "|") { l = l | rr; }
            else if (op == "&&") { l = (l != 0 && rr != 0) ? 1 : 0; }
            else if (op == "||") { l = (l != 0 || rr != 0) ? 1 : 0; }
        }
        return l;
    }
    long bedingt() {
        const long c = binaer(1);
        if (ist("?")) {
            ++i;
            const long a = bedingt();
            if (!ist(":")) {
                fehler = true;
                return 0;
            }
            ++i;
            const long b = bedingt();
            return c != 0 ? a : b;
        }
        return c;
    }
};

}  // namespace

// PC_Evaluate (ganzzahlig)
bool PcQuelle::auswerten(long& wert) {
    wert = 0;
    RohToken t;
    if (!zeileLies(t)) {
        fehler("no value after #if/#elif");
        return false;
    }
    std::vector<RohToken> liste;
    bool defined = false;
    do {
        if (t.typ == TT_NAME) {
            if (defined) {
                defined = false;
                RohToken z;
                z.typ = TT_NUMBER;
                z.ival = defines_.count(t.s) != 0 ? 1U : 0U;
                if (!liste.empty() && liste.back().s == "defined") { liste.back() = z; }
                else { liste.push_back(z); }
            } else if (t.s == "defined") {
                defined = true;
                liste.push_back(t);
            } else {
                const auto it = defines_.find(t.s);
                if (it == defines_.end()) {
                    fehler("can't evaluate " + t.s + ", not defined");
                    return false;
                }
                const Define d = it->second;
                if (!erweitern(t, d)) { return false; }
            }
        } else if (t.typ == TT_NUMBER || t.typ == TT_PUNCTUATION) {
            if (defined && t.s == "(") {
                // defined ( NAME ) - die Klammern fallen weg
                continue;
            }
            liste.push_back(t);
        } else {
            fehler("can't evaluate " + t.s);
            return false;
        }
    } while (zeileLies(t));
    // "defined ( X )": die schliessende Klammer nach dem Ergebnis entfernen
    std::vector<RohToken> sauber;
    int offen = 0;
    for (const RohToken& r : liste) {
        if (r.typ == TT_PUNCTUATION && r.s == "(") { ++offen; }
        if (r.typ == TT_PUNCTUATION && r.s == ")") {
            if (offen == 0) { continue; }
            --offen;
        }
        sauber.push_back(r);
    }
    Auswerter a{sauber};
    wert = a.bedingt();
    if (a.fehler || a.i != sauber.size()) {
        fehler("can't evaluate expression");
        return false;
    }
    return true;
}

bool PcQuelle::wenn(bool elif) {
    if (elif) {
        int typ = 0;
        bool skip = false;
        einrueckPop(typ, skip);
        if (typ == 0 || typ == INDENT_ELSE) {
            fehler("misplaced #elif");
            return false;
        }
    }
    long v = 0;
    if (!auswerten(v)) { return false; }
    einrueckPush(elif ? INDENT_ELIF : INDENT_IF, v == 0);
    return true;
}

// PC_ReadDirective
bool PcQuelle::direktive() {
    RohToken t;
    if (!quelleLies(t)) {
        fehler("found # without name");
        return false;
    }
    if (t.linescrossed > 0) {
        quelleZurueck(t);
        fehler("found # at end of line");
        return false;
    }
    if (t.typ == TT_NAME) {
        if (t.s == "if") { return wenn(false); }
        if (t.s == "ifdef") { return ifdef(INDENT_IFDEF); }
        if (t.s == "ifndef") { return ifdef(INDENT_IFNDEF); }
        if (t.s == "elif") { return wenn(true); }
        if (t.s == "else") { return sonst(); }
        if (t.s == "endif") { return endif(); }
        if (t.s == "include") { return include(); }
        if (t.s == "define") { return define(); }
        if (t.s == "undef") { return undef(); }
        if (t.s == "line") {
            fehler("#line directive not supported");
            return false;
        }
        if (t.s == "error") {
            RohToken z;
            const bool ok = zeileLies(z);
            fehler("#error directive: " + (ok ? z.s : std::string()));
            return false;
        }
        if (t.s == "pragma") {
            warnung("#pragma directive not supported");
            RohToken z;
            while (zeileLies(z)) {}
            return true;
        }
        if (t.s == "eval" || t.s == "evalfloat") {
            if (skip_ > 0) {
                RohToken z;
                while (zeileLies(z)) {}
                return true;
            }
            long v = 0;
            if (!auswerten(v)) { return false; }
            RohToken z = t;
            z.typ = TT_NUMBER;
            z.subtyp = TT_DECIMAL | TT_INTEGER;
            z.s = std::to_string(v < 0 ? -v : v);
            z.ival = static_cast<unsigned long>(v < 0 ? -v : v);
            z.fval = static_cast<long double>(z.ival);
            quelleZurueck(z);
            if (v < 0) {
                RohToken m = t;
                m.typ = TT_PUNCTUATION;
                m.s = "-";
                quelleZurueck(m);
            }
            return true;
        }
    }
    fehler("unknown precompiler directive " + t.s);
    return false;
}

// PC_ReadDefineParms - mit der botlib-Eigenheit, dass eine innere "("
// nicht mitkopiert wird und die erste ")" auf Ebene 0 die Liste schliesst.
bool PcQuelle::parmsLies(const Define& d, std::vector<std::vector<RohToken>>& parms) {
    RohToken t;
    if (!quelleLies(t)) {
        fehler("define " + d.name + " missing parms");
        return false;
    }
    parms.assign(d.parms.size(), {});
    if (t.s != "(") {
        quelleZurueck(t);
        fehler("define " + d.name + " missing parms");
        return false;
    }
    std::size_t numparms = 0;
    int indent = 0;
    bool done = false;
    while (!done) {
        if (numparms >= 128) {   // MAX_DEFINEPARMS
            fehler("define " + d.name + " with too many parms");
            return false;
        }
        if (numparms >= d.parms.size()) {
            warnung("define " + d.name + " has too many parms");
            return false;
        }
        bool lastcomma = true;
        while (!done) {
            if (!quelleLies(t)) {
                fehler("define " + d.name + " incomplete");
                return false;
            }
            if (t.s == "," && indent <= 0) {
                if (lastcomma) { warnung("too many comma's"); }
                break;
            }
            lastcomma = false;
            if (t.s == "(") {
                ++indent;
                continue;
            }
            if (t.s == ")") {
                if (--indent <= 0) {
                    if (parms.back().empty()) { warnung("too few define parms"); }
                    done = true;
                    break;
                }
            }
            if (numparms < d.parms.size()) { parms[numparms].push_back(t); }
        }
        ++numparms;
    }
    return true;
}

// PC_ExpandDefine + PC_ExpandDefineIntoSource
bool PcQuelle::erweitern(const RohToken& auf, const Define& d) {
    std::vector<RohToken> aus;
    std::uint32_t spannenEnde = auf.ende;
    if (d.builtin != 0) {
        RohToken t = auf;
        switch (d.builtin) {
            case 1:
                t.typ = TT_NUMBER;
                t.subtyp = TT_DECIMAL | TT_INTEGER;
                t.ival = static_cast<unsigned long>(auf.zeile);
                t.fval = static_cast<long double>(t.ival);
                t.s = std::to_string(auf.zeile);
                break;
            case 2:
                t.typ = TT_STRING;
                t.s = "\"" + quellen_[static_cast<std::size_t>(auf.datei)].name + "\"";
                break;
            case 3:
                t.typ = TT_STRING;
                t.s = "\"Jan 01 2000\"";
                break;
            default:
                t.typ = TT_STRING;
                t.s = "\"00:00:00\"";
                break;
        }
        aus.push_back(t);
    } else {
        std::vector<std::vector<RohToken>> parms;
        if (!d.parms.empty()) {
            if (!parmsLies(d, parms)) { return false; }
        }
        auto parmNr = [&](const std::string& n) -> int {
            for (std::size_t i = 0; i < d.parms.size(); ++i) {
                if (d.parms[i] == n) { return static_cast<int>(i); }
            }
            return -1;
        };
        for (std::size_t k = 0; k < d.tokens.size(); ++k) {
            const RohToken& dt = d.tokens[k];
            const int nr = dt.typ == TT_NAME ? parmNr(dt.s) : -1;
            if (nr >= 0) {
                for (const RohToken& pt : parms[static_cast<std::size_t>(nr)]) {
                    aus.push_back(pt);
                    if (pt.datei == auf.datei && pt.ende > spannenEnde) { spannenEnde = pt.ende; }
                }
                continue;
            }
            if (dt.s == "#") {
                const int n2 = (k + 1 < d.tokens.size()) ? parmNr(d.tokens[k + 1].s) : -1;
                if (n2 < 0) {
                    warnung("stringizing operator without define parameter");
                    continue;
                }
                ++k;
                RohToken s = dt;   // PC_StringizeTokens
                s.typ = TT_STRING;
                s.s = "\"";
                for (const RohToken& pt : parms[static_cast<std::size_t>(n2)]) { s.s += pt.s; }
                s.s += "\"";
                aus.push_back(s);
                continue;
            }
            aus.push_back(dt);
        }
        // ## zusammenfuegen (PC_MergeTokens)
        for (std::size_t i = 0; i + 1 < aus.size();) {
            if (aus[i + 1].s == "##" && i + 2 < aus.size()) {
                RohToken& t1 = aus[i];
                const RohToken& t2 = aus[i + 2];
                if (t1.typ == TT_NAME && (t2.typ == TT_NAME || t2.typ == TT_NUMBER)) {
                    t1.s += t2.s;
                } else if (t1.typ == TT_STRING && t2.typ == TT_STRING) {
                    t1.s.pop_back();
                    t1.s += t2.s.substr(1);
                } else {
                    fehler("can't merge " + t1.s + " with " + t2.s);
                    return false;
                }
                aus.erase(aus.begin() + static_cast<std::ptrdiff_t>(i) + 1, aus.begin() + static_cast<std::ptrdiff_t>(i) + 3);
                continue;
            }
            ++i;
        }
    }
    if (aus.empty()) { return false; }   // wie botlib: leere Erweiterung beendet das Lesen
    for (auto it = aus.rbegin(); it != aus.rend(); ++it) {
        RohToken t = *it;
        t.datei = auf.datei;
        t.anfang = auf.anfang;
        t.ende = spannenEnde;
        t.zeile = auf.zeile;
        t.ausMakro = true;
        tokens_.push_front(t);
    }
    return true;
}

// PC_ReadToken
bool PcQuelle::pcLies(RohToken& t) {
    while (true) {
        if (!quelleLies(t)) { return false; }
        if (t.typ == TT_PUNCTUATION && t.s == "@") {   // StringEd-Schluessel
            RohToken n;
            quelleLies(n);
            n.s = "@" + n.s;
            n.anfang = t.anfang;
            t = n;
            return true;
        }
        if (t.typ == TT_PUNCTUATION && t.s == "#") {
            if (!direktive()) { return false; }
            continue;
        }
        if (t.typ == TT_PUNCTUATION && t.s == "$") {
            RohToken n;
            if (!quelleLies(n)) {
                fehler("found $ without name");
                return false;
            }
            fehler("unknown precompiler directive " + n.s);
            return false;
        }
        if (t.typ == TT_STRING) {
            RohToken n;
            if (pcLies(n)) {
                if (n.typ == TT_STRING) {
                    t.s.pop_back();
                    if (t.s.size() + n.s.size() >= kMaxToken) {
                        fehler("string longer than MAX_TOKEN 1024");
                        return false;
                    }
                    t.s += n.s.substr(1);
                    if (n.datei == t.datei && !n.ausMakro && !t.ausMakro) { t.ende = n.ende; }
                } else {
                    quelleZurueck(n);
                }
            }
        }
        if (skip_ > 0) { continue; }
        if (t.typ == TT_NAME) {
            const auto it = defines_.find(t.s);
            if (it != defines_.end()) {
                const Define d = it->second;
                if (!erweitern(t, d)) { return false; }
                continue;
            }
        }
        return true;
    }
}

// PC_ReadTokenHandle
bool PcQuelle::lies(Token& out) {
    RohToken t;
    const bool ok = pcLies(t);
    out = Token{};
    out.s = t.s;
    out.typ = t.typ;
    out.subtyp = t.subtyp;
    out.ival = static_cast<long>(static_cast<int>(t.ival));
    out.fval = static_cast<double>(static_cast<float>(t.fval));
    out.datei = t.datei;
    out.anfang = t.anfang;
    out.ende = t.ende;
    out.zeile = t.zeile;
    out.ausMakro = t.ausMakro;
    if (out.typ == TT_STRING && !out.s.empty() && out.s[0] != '@') {
        // StripDoubleQuotes
        if (out.s[0] == '"') { out.s.erase(0, 1); }
        if (!out.s.empty() && out.s.back() == '"') { out.s.pop_back(); }
    }
    return ok;
}

}  // namespace bhed::menue::intern
