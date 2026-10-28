// lexer.h - Menue-Editor M1: die beiden Wortleser der Engine (intern).
//
// Portiert aus OpenJK: code/qcommon/q_shared.cpp (COM_ParseExt,
// COM_ParseString/Int/Float/Vec4) fuer die SP-Menues und
// codemp/botlib/l_script.cpp + l_precomp.cpp (PS_ReadToken, PC_ReadToken,
// #include/#define/#if, Makros) fuer die MP-Menues.
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
#ifndef BHED_MENUE_LEXER_H
#define BHED_MENUE_LEXER_H

#include "bhed/menue/menue.h"

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace bhed::menue::intern {

// --- SP: COM_ParseExt auf einem Puffer --------------------------------------
class ComLeser {
public:
    explicit ComLeser(const std::string& text) : text_(text) {}
    // COM_ParseExt: leeres Token = Dateiende (oder Zeilenende ohne allowLineBreaks).
    Token parseExt(bool allowLineBreaks);
    // Rueckgabe wie die Engine: true = FEHLER (COM_ParseInt ...).
    bool parseString(Token& t);        // PC_ParseString (ueberspringt Zeilenenden)
    bool parseInt(int& v, Token& t);
    bool parseFloat(float& v, Token& t);
    [[nodiscard]] int zeile() const noexcept { return lines_; }
    [[nodiscard]] bool amEnde() const noexcept { return ende_; }
    std::vector<std::string>* printf = nullptr;   // Com_Printf-Ausgaben

private:
    const std::string& text_;
    std::size_t p_ = 0;
    bool ende_ = false;   // *data_p == NULL
    int lines_ = 1;
};

// --- MP: botlib-Praeprozessor ------------------------------------------------
struct RohToken {
    std::string s;      // mit Anfuehrungszeichen wie in botlib
    int typ = 0, subtyp = 0;
    unsigned long ival = 0;
    long double fval = 0;
    int zeile = 0, linescrossed = 0;
    bool leerVorher = false;   // PC_WhiteSpaceBeforeToken
    int datei = 0;
    std::uint32_t anfang = 0, ende = 0;
    bool ausMakro = false;
};

struct Define {
    std::string name;
    std::vector<std::string> parms;
    std::vector<RohToken> tokens;
    int builtin = 0;    // 1 = __LINE__, 2 = __FILE__, 3 = __DATE__, 4 = __TIME__
    bool fixed = false;
};

class PcQuelle {
public:
    PcQuelle(Lader lader, std::vector<Quelle>& quellen, std::vector<Meldung>& meldungen)
        : lader_(std::move(lader)), quellen_(quellen), meld_(meldungen) {}
    // PC_LoadGlobalDefines
    bool ladeGlobal(const std::string& pfad);
    // PC_LoadSourceHandle fuer eine schon gelesene Datei (Index in quellen).
    void oeffne(int datei);
    // PC_ReadTokenHandle: false = Ende oder Fehler.
    bool lies(Token& t);
    // PC_SourceFileAndLine
    [[nodiscard]] std::string dateiName() const;
    [[nodiscard]] int zeile() const;
    [[nodiscard]] int dateiIndex() const;
    // PC_SourceError aus ui_shared ("ERROR: %s, line %d: %s")
    void uiFehler(const std::string& text);
    std::map<std::string, Define> globale;   // fuer andere Quellen wiederverwendbar

private:
    struct Skript {
        int datei = 0;
        std::size_t p = 0;
        int line = 1, lastline = 1;
        bool ende = false;
    };
    struct Einrueck {
        int typ = 0;
        bool skip = false;
        std::size_t skript = 0;   // Tiefe im Stapel
    };
    bool psLies(Skript& s, RohToken& t);
    bool psLeer(Skript& s);
    bool psString(Skript& s, RohToken& t, char quote);
    bool psEscape(Skript& s, char& ch);
    void psName(Skript& s, RohToken& t);
    void psZahl(Skript& s, RohToken& t);
    bool psPunkt(Skript& s, RohToken& t);
    bool quelleLies(RohToken& t);          // PC_ReadSourceToken
    void quelleZurueck(const RohToken& t); // PC_UnreadSourceToken
    bool zeileLies(RohToken& t);           // PC_ReadLine
    bool pcLies(RohToken& t);              // PC_ReadToken
    bool direktive();
    bool include();
    bool define();
    bool undef();
    bool ifdef(int typ);
    bool sonst();
    bool endif();
    bool wenn(bool elif);
    bool auswerten(long& wert);
    bool erweitern(const RohToken& auf, const Define& d);
    bool parmsLies(const Define& d, std::vector<std::vector<RohToken>>& parms);
    void einrueckPush(int typ, bool skip);
    void einrueckPop(int& typ, bool& skip);
    void fehler(const std::string& text);
    void warnung(const std::string& text);
    bool ladeDatei(const std::string& pfad, int& index);
    const std::string& text(const Skript& s) const { return quellen_[static_cast<std::size_t>(s.datei)].text; }

    Lader lader_;
    std::vector<Quelle>& quellen_;
    std::vector<Meldung>& meld_;
    std::vector<Skript> stapel_;
    std::deque<RohToken> tokens_;
    std::vector<Einrueck> einrueck_;
    int skip_ = 0;
    std::map<std::string, Define> defines_;
    bool alsGlobal_ = false;
};

}  // namespace bhed::menue::intern

#endif  // BHED_MENUE_LEXER_H
