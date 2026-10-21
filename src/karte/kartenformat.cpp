// kartenformat.cpp - Karten-Editor: .map lesen und schreiben wie NetRadiant-Custom.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026):
//   libs/script/scripttokeniser.h   ScriptTokeniser (Kartentokenizer)
//   libs/script/scripttokenwriter.h SimpleTokenWriter
//   libs/stream/textstream.h        Decimal, SingleCharacterInputStream::bufferContains
//   libs/stringio.h                 Tokeniser_getFloat/Double/Integer/Size, ...
//   plugins/mapq3/parse.cpp         Map_Read, Entity_parseTokens, LayersParser
//   plugins/mapq3/write.cpp         Map_Write, Layers_Write, Layer_Write
//   plugins/mapq3/plugin.cpp        MapQ3API::parsePrimitive (Formaterkennung)
//   radiant/brushtokens.h           Flaechen-Importer/-Exporter
//   radiant/patch.h                 Patch-Tokens (patchDef2, patchDef3)
//   radiant/referencecache.cpp      MapResource_loadFile/saveFile/save
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   Copyright (C) 2001-2006, William Joseph. All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/kartenformat.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <list>
#include <sstream>

namespace bhed::karte {

namespace {

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

void unerwartet(const Tokenizer& t, const char* token, const char* erwartet) {   // Tokeniser_unexpectedError
    t.melden(std::to_string(t.zeile()) + ':' + std::to_string(t.spalte()) + ": parse error at '" +
             (token != nullptr ? token : "#EOF") + "': expected '" + erwartet + "'");
}

// string_parse_float / string_parse_double: strtod, ganzer Text muss passen
bool zahlLesen(const char* s, double& d) {
    if (*s == '\0') { return false; }
    char* ende = nullptr;
    d = std::strtod(s, &ende);
    return *ende == '\0';
}

bool floatHolen(Tokenizer& t, float& f) {   // Tokeniser_getFloat
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

bool doubleHolen(Tokenizer& t, double& d) {   // Tokeniser_getDouble
    const char* token = t.getToken();
    if (token != nullptr && zahlLesen(token, d)) { return true; }
    unerwartet(t, token, "#number");
    return false;
}

bool ganzzahlHolen(Tokenizer& t, int& i) {   // Tokeniser_getInteger (strtol, Basis 10)
    const char* token = t.getToken();
    if (token != nullptr && *token != '\0') {
        char* ende = nullptr;
        i = static_cast<int>(std::strtol(token, &ende, 10));
        if (*ende == '\0') { return true; }
    }
    unerwartet(t, token, "#integer");
    return false;
}

bool groesseHolen(Tokenizer& t, std::size_t& n) {   // Tokeniser_getSize (strtoul, als int gespeichert)
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

bool tokenErwarten(Tokenizer& t, const char* erwartet) {   // Tokeniser_parseToken
    const char* token = t.getToken();
    if (token != nullptr && std::strcmp(token, erwartet) == 0) { return true; }
    unerwartet(t, token, erwartet);
    return false;
}

bool naechstesIstZiffer(Tokenizer& t) {   // Tokeniser_nextTokenIsDigit
    const char* token = t.getToken();
    if (token == nullptr) { return false; }
    const char c = *token;
    t.ungetToken();
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

void zurNaechstenZeile(Tokenizer& t) {   // Tokeniser_skipToNextLine
    const std::size_t zeile = t.zeile();
    while (t.getToken() && zeile == t.zeile()) {}
    t.ungetToken();
}

#define RUECK_FALSCH(ausdruck) do { if (!(ausdruck)) { return false; } } while (false)   // RETURN_FALSE_IF_FAIL

// ===================== Ebenen beim Lesen (LayersParser, parse.cpp:37-124) ===

class EbenenLeser {
public:
    struct Knoten {
        std::string name;
        int elternIndex = -1;   // m_parentIndex
        int farbe[3] = {0, 0, 0};
        std::list<int> kinder;
        int eigenerIndex = -1;  // m_ownIndex
    };

    EbenenLeser() {
        m_knoten.push_back(Knoten{"0", -1, {0, 0, 0}, {}, -1});   // Layers(): Standardebene
        m_wurzel.push_back(0);
    }

    // read_layers: false bei leerer Datei (EOF)
    bool definitionenLesen(Tokenizer& t) {
        for (;;) {
            t.nextLine();
            const char* token = t.getToken();
            if (!token) { return false; }
            if (std::strcmp(token, "layerdef") != 0) {
                t.ungetToken();
                return true;
            }
            token = t.getToken();
            if (!token) {
                unerwartet(t, token, "#layer-name");
                return false;
            }
            m_knoten.push_back(Knoten{token, -1, {0, 0, 0}, {}, -1});
            const int idx = static_cast<int>(m_knoten.size()) - 1;
            m_wurzel.push_back(idx);
            Knoten& k = m_knoten.back();
            if (!ganzzahlHolen(t, k.elternIndex) || !ganzzahlHolen(t, k.farbe[0]) || !ganzzahlHolen(t, k.farbe[1])
                || !ganzzahlHolen(t, k.farbe[2])) {
                return false;
            }
            zurNaechstenZeile(t);   // moegliche Formaterweiterung ueberspringen
        }
    }

    // construct_tree
    void baumBauen(const Tokenizer& t) {
        if (m_wurzel.size() > 1) { m_wurzel.pop_front(); }   // Standardebene weg, wenn etwas gelesen wurde
        m_vec.assign(m_wurzel.begin(), m_wurzel.end());
        const bool wurzelDa = std::any_of(m_wurzel.begin(), m_wurzel.end(), [&](int i) { return m_knoten[i].elternIndex == -1; });
        if (!wurzelDa) {
            t.melden("No layers in the root");
            m_knoten[m_wurzel.front()].elternIndex = -1;
        }
        for (const int it : m_vec) {
            Knoten& k = m_knoten[it];
            if (k.elternIndex < -1 || k.elternIndex >= static_cast<int>(m_vec.size())) {
                t.melden(std::to_string(k.elternIndex) + " parent layer index out of bounds");
                k.elternIndex = -1;
            }
            if (k.elternIndex > -1 && m_vec[static_cast<std::size_t>(k.elternIndex)] == it) {
                t.melden(std::to_string(k.elternIndex) + " parent layer index self reference");
                k.elternIndex = -1;
            }
            std::list<int>& eltern = k.elternIndex == -1 ? m_wurzel : m_knoten[m_vec[static_cast<std::size_t>(k.elternIndex)]].kinder;
            // splice( end, root.m_children, it ): it steht noch in der Wurzelliste
            auto pos = std::find(m_wurzel.begin(), m_wurzel.end(), it);
            if (pos != m_wurzel.end()) {
                eltern.splice(eltern.end(), m_wurzel, pos);
            }
        }
    }

    // read_layer: false bei EOF oder Lesefehler
    bool ebeneLesen(Tokenizer& t) {
        const char* token = t.getToken();
        if (token == nullptr) { return false; }
        if (std::strcmp(token, "layer") != 0) {
            t.ungetToken();
            return true;
        }
        if (!ganzzahlHolen(t, m_aktuell)) {
            return false;
        }
        if (m_aktuell <= -1 || m_aktuell >= static_cast<int>(m_vec.size())) {
            t.melden(std::to_string(m_aktuell) + " primitive layer index out of bounds");
            m_aktuell = 0;
        }
        zurNaechstenZeile(t);
        return true;
    }
    [[nodiscard]] int aktuell() const { return m_vec[static_cast<std::size_t>(m_aktuell)]; }   // getCurrentLayer

    // Layers::update_ownIndices + Ablage in der Karte (Tiefensuche ab der Wurzel)
    void inKarte(Karte& karte) {
        karte.schichten.clear();
        std::vector<std::pair<int, int>> stapel;   // (Knoten, Eltern-eigenerIndex)
        auto besuchen = [&](auto&& selbst, const std::list<int>& liste, int elternEigen) -> void {
            for (const int i : liste) {
                Knoten& k = m_knoten[i];
                k.eigenerIndex = static_cast<int>(karte.schichten.size());
                karte.schichten.push_back(Schicht{k.name, elternEigen, {k.farbe[0], k.farbe[1], k.farbe[2]}});
                selbst(selbst, k.kinder, k.eigenerIndex);
            }
        };
        besuchen(besuchen, m_wurzel, -1);
        karte.aktuelleSchicht = 0;   // m_currentLayer = &front
    }
    // Index einer gelesenen Ebene in der Karte
    [[nodiscard]] int kartenIndex(int knoten) const {
        const int e = m_knoten[static_cast<std::size_t>(knoten)].eigenerIndex;
        return e < 0 ? kSchichtUnerreichbar : e;
    }

private:
    std::vector<Knoten> m_knoten;
    std::list<int> m_wurzel;   // Layers::m_children
    std::vector<int> m_vec;    // m_layersVec
    int m_aktuell = 0;         // m_currentLayer (LAYERIDX0)
};

// ===================== Flaechen lesen (brushtokens.h) =======================

bool ebeneLesen(Flaeche& f, Tokenizer& t) {   // FacePlane_importTokens
    EbenenPunkte p{};
    for (std::size_t i = 0; i < 3; ++i) {
        RUECK_FALSCH(tokenErwarten(t, "("));
        for (std::size_t j = 0; j < 3; ++j) {
            RUECK_FALSCH(doubleHolen(t, p[i][j]));
        }
        RUECK_FALSCH(tokenErwarten(t, ")"));
    }
    f.ladeEbenenPunkte(p);
    return true;
}

bool shaderLesen(Flaeche& f, Tokenizer& t) {   // FaceShader_importTokens
    const char* textur = t.getToken();
    if (textur == nullptr) {
        unerwartet(t, textur, "#texture-name");
        return false;
    }
    if (std::strcmp(textur, "NULL") == 0) {
        f.ladeShader(kTexturPraefix);
    } else {
        f.ladeShader(std::string(kTexturPraefix) + textur);
    }
    return true;
}

bool texdefGesund(const Texdef& td) {   // texdef_sane
    return std::fabs(td.shift[0]) < (1 << 16) && std::fabs(td.shift[1]) < (1 << 16);
}

bool texdefLesen(Flaeche& f, Tokenizer& t) {   // FaceTexdef_importTokens
    Texdef& td = f.projektionRoh().texdef;
    RUECK_FALSCH(floatHolen(t, td.shift[0]));
    RUECK_FALSCH(floatHolen(t, td.shift[1]));
    RUECK_FALSCH(floatHolen(t, td.rotate));
    RUECK_FALSCH(floatHolen(t, td.scale[0]));
    RUECK_FALSCH(floatHolen(t, td.scale[1]));
    if (!texdefGesund(td)) { t.melden("FaceTexdef_importTokens: bad texdef"); }
    return true;
}

bool bpLesen(Flaeche& f, Tokenizer& t) {   // FaceTexdef_BP_importTokens
    BpTexdef& bp = f.projektionRoh().bp;
    RUECK_FALSCH(tokenErwarten(t, "("));
    for (int z = 0; z < 2; ++z) {
        RUECK_FALSCH(tokenErwarten(t, "("));
        RUECK_FALSCH(floatHolen(t, bp.coords[z][0]));
        RUECK_FALSCH(floatHolen(t, bp.coords[z][1]));
        RUECK_FALSCH(floatHolen(t, bp.coords[z][2]));
        RUECK_FALSCH(tokenErwarten(t, ")"));
    }
    RUECK_FALSCH(tokenErwarten(t, ")"));
    return true;
}

bool valveLesen(Flaeche& f, Tokenizer& t) {   // FaceTexdef_Valve220_importTokens
    TexturProjektion& p = f.projektionRoh();
    RUECK_FALSCH(tokenErwarten(t, "["));
    RUECK_FALSCH(floatHolen(t, p.basisS.x()));
    RUECK_FALSCH(floatHolen(t, p.basisS.y()));
    RUECK_FALSCH(floatHolen(t, p.basisS.z()));
    RUECK_FALSCH(floatHolen(t, p.texdef.shift[0]));
    RUECK_FALSCH(tokenErwarten(t, "]"));
    RUECK_FALSCH(tokenErwarten(t, "["));
    RUECK_FALSCH(floatHolen(t, p.basisT.x()));
    RUECK_FALSCH(floatHolen(t, p.basisT.y()));
    RUECK_FALSCH(floatHolen(t, p.basisT.z()));
    RUECK_FALSCH(floatHolen(t, p.texdef.shift[1]));
    RUECK_FALSCH(tokenErwarten(t, "]"));
    RUECK_FALSCH(floatHolen(t, p.texdef.rotate));
    RUECK_FALSCH(floatHolen(t, p.texdef.scale[0]));
    RUECK_FALSCH(floatHolen(t, p.texdef.scale[1]));
    p.texdef.rotate = -p.texdef.rotate;
    if (!texdefGesund(p.texdef)) { t.melden("FaceTexdef_Valve220_importTokens: bad texdef"); }
    return true;
}

bool flagsLesen(Flaeche& f, Tokenizer& t) {   // FaceShader_importContentsFlagsValue
    InhaltFlags& fl = f.flagsRoh();
    RUECK_FALSCH(ganzzahlHolen(t, fl.inhaltFlags));
    RUECK_FALSCH(ganzzahlHolen(t, fl.flaechenFlags));
    RUECK_FALSCH(ganzzahlHolen(t, fl.wert));
    return true;
}

bool flaecheLesen(Flaeche& f, Tokenizer& t, PinselFormat format) {
    switch (format) {
    case PinselFormat::Quake3:   // QuakeFaceTokenImporter
        RUECK_FALSCH(ebeneLesen(f, t));
        RUECK_FALSCH(shaderLesen(f, t));
        RUECK_FALSCH(texdefLesen(f, t));
        if (naechstesIstZiffer(t)) {
            f.flagsRoh().angegeben = true;
            RUECK_FALSCH(flagsLesen(f, t));
        }
        f.skalierungAngewandt = true;
        return true;
    case PinselFormat::Quake3BP:   // Quake3BPFaceTokenImporter
        RUECK_FALSCH(ebeneLesen(f, t));
        RUECK_FALSCH(bpLesen(f, t));
        RUECK_FALSCH(shaderLesen(f, t));
        if (naechstesIstZiffer(t)) {
            f.flagsRoh().angegeben = true;
            RUECK_FALSCH(flagsLesen(f, t));
        }
        f.projektionInitialisiert = true;
        f.skalierungAngewandt = true;
        return true;
    case PinselFormat::Quake3Valve220:   // Valve220FaceTokenImporter
        RUECK_FALSCH(ebeneLesen(f, t));
        RUECK_FALSCH(shaderLesen(f, t));
        RUECK_FALSCH(valveLesen(f, t));
        if (naechstesIstZiffer(t)) {
            f.flagsRoh().angegeben = true;
            RUECK_FALSCH(flagsLesen(f, t));
        }
        f.skalierungAngewandt = true;
        return true;
    }
    return false;
}

// BrushTokenImporter::importTokens (brushtokens.h:507-571)
bool pinselLesen(Pinsel& p, Tokenizer& t, PinselFormat format) {
    const bool bp = format == PinselFormat::Quake3BP;
    if (bp) {
        t.nextLine();
        RUECK_FALSCH(tokenErwarten(t, "{"));
    }
    while (true) {
        t.nextLine();   // Ende des Pinsels?
        const char* token = t.getToken();
        if (token != nullptr && std::strcmp(token, "}") == 0) {   // string_equal mit 0 -> NRC liest weiter
            break;
        }
        t.ungetToken();
        Flaeche& f = p.neueFlaecheLaden();
        t.nextLine();
        RUECK_FALSCH(flaecheLesen(f, t, format));
    }
    if (bp) {
        t.nextLine();
        RUECK_FALSCH(tokenErwarten(t, "}"));
    }
    p.geaendert();
    return true;
}

// ===================== Patch-Tokens (patch.h:1010-1131) =====================

bool patchKopfLesen(Tokenizer& t) {   // Patch_importHeader
    t.nextLine();
    RUECK_FALSCH(tokenErwarten(t, "{"));
    return true;
}

bool patchShaderLesen(Patch& p, Tokenizer& t) {   // Patch_importShader
    t.nextLine();
    const char* textur = t.getToken();
    if (textur == nullptr) {
        unerwartet(t, textur, "#texture-name");
        return false;
    }
    if (std::strcmp(textur, "NULL") == 0) {
        p.setzeShader(kTexturPraefix);
    } else {
        p.setzeShader(std::string(kTexturPraefix) + textur);
    }
    return true;
}

bool patchShaderDoom3Lesen(Patch& p, Tokenizer& t) {   // PatchDoom3_importShader
    t.nextLine();
    const char* shader = t.getToken();
    if (shader == nullptr) {
        unerwartet(t, shader, "#shader-name");
        return false;
    }
    if (std::strcmp(shader, "_emptyname") == 0) {
        p.setzeShader(kTexturPraefix);
    } else {
        p.setzeShader(shader);
    }
    return true;
}

bool patchParameterLesen(Patch& p, Tokenizer& t) {   // Patch_importParams
    t.nextLine();
    RUECK_FALSCH(tokenErwarten(t, "("));
    {
        std::size_t c = 0, r = 0;
        RUECK_FALSCH(groesseHolen(t, c));
        RUECK_FALSCH(groesseHolen(t, r));
        p.setzeGroesse(c, r);
    }
    if (p.patchDef3) {
        RUECK_FALSCH(groesseHolen(t, p.unterteilungX));
        RUECK_FALSCH(groesseHolen(t, p.unterteilungY));
    }
    int tmp = 0;   // contents/flags/value werden uebergangen
    RUECK_FALSCH(ganzzahlHolen(t, tmp));
    RUECK_FALSCH(ganzzahlHolen(t, tmp));
    RUECK_FALSCH(ganzzahlHolen(t, tmp));
    RUECK_FALSCH(tokenErwarten(t, ")"));
    return true;
}

bool patchMatrixLesen(Patch& p, Tokenizer& t) {   // Patch_importMatrix
    t.nextLine();
    RUECK_FALSCH(tokenErwarten(t, "("));
    for (std::size_t c = 0; c < p.breite(); ++c) {
        t.nextLine();
        RUECK_FALSCH(tokenErwarten(t, "("));
        for (std::size_t r = 0; r < p.hoehe(); ++r) {
            RUECK_FALSCH(tokenErwarten(t, "("));
            PatchPunkt& pp = p.punkt(r, c);
            RUECK_FALSCH(floatHolen(t, pp.vertex[0]));
            RUECK_FALSCH(floatHolen(t, pp.vertex[1]));
            RUECK_FALSCH(floatHolen(t, pp.vertex[2]));
            RUECK_FALSCH(floatHolen(t, pp.texcoord[0]));
            RUECK_FALSCH(floatHolen(t, pp.texcoord[1]));
            RUECK_FALSCH(tokenErwarten(t, ")"));
        }
        RUECK_FALSCH(tokenErwarten(t, ")"));
    }
    t.nextLine();
    RUECK_FALSCH(tokenErwarten(t, ")"));
    return true;
}

bool patchFussLesen(Patch& p, Tokenizer& t) {   // Patch_importFooter
    p.punkteGeaendert();
    t.nextLine();
    RUECK_FALSCH(tokenErwarten(t, "}"));
    t.nextLine();
    RUECK_FALSCH(tokenErwarten(t, "}"));
    return true;
}

bool patchLesen(Patch& p, Tokenizer& t) {   // PatchTokenImporter
    RUECK_FALSCH(patchKopfLesen(t));
    RUECK_FALSCH(patchShaderLesen(p, t));
    RUECK_FALSCH(patchParameterLesen(p, t));
    RUECK_FALSCH(patchMatrixLesen(p, t));
    RUECK_FALSCH(patchFussLesen(p, t));
    return true;
}

// ===================== Karte lesen (parse.cpp) ==============================

struct Leser {
    Tokenizer& t;
    Karte& karte;
    EbenenLeser& ebenen;
    bool formatErkannt = false;   // MapQ3API::m_formatDetected

    // MapQ3API::parsePrimitive (plugin.cpp:279-325)
    std::shared_ptr<Primitiv> primitivLesen() {
        const char* primitiv = t.getToken();
        if (primitiv != nullptr) {
            if (std::strcmp(primitiv, "patchDef2") == 0) {
                auto p = std::make_shared<Patch>();
                return patchLesen(*p, t) ? p : nullptr;
            }
            if (!formatErkannt) {
                PinselFormat erkannt;
                if (std::strcmp(primitiv, "brushDef") == 0) {
                    erkannt = PinselFormat::Quake3BP;
                    t.melden("detectedFormat = eBrushTypeQuake3BP");
                } else if (std::strcmp(primitiv, "(") == 0 && t.pufferEnthaelt(" [ ") && t.pufferEnthaelt(" ] ")) {
                    erkannt = PinselFormat::Quake3Valve220;
                    t.melden("detectedFormat = eBrushTypeQuake3Valve220");
                } else if (std::strcmp(primitiv, "(") == 0) {
                    erkannt = PinselFormat::Quake3;
                    t.melden("detectedFormat = eBrushTypeQuake3");
                } else {
                    t.melden("Format is not detected");
                    unerwartet(t, primitiv, "#different-brush-format");
                    return nullptr;
                }
                formatErkannt = true;
                karte.kontext.format = erkannt;   // toggleFormat
            }
            switch (karte.kontext.format) {
            case PinselFormat::Quake3:
            case PinselFormat::Quake3Valve220:
                t.ungetToken();   // (
                [[fallthrough]];
            case PinselFormat::Quake3BP: {
                auto p = std::make_shared<Pinsel>();
                return pinselLesen(*p, t, karte.kontext.format) ? p : nullptr;
            }
            }
        }
        unerwartet(t, primitiv, "#quake3-primitive");
        return nullptr;
    }

    // Entity_create (parse.cpp:135-142)
    std::shared_ptr<Entitaet> erzeugen(const std::string& klassenname, bool hatPinsel,
                                       const std::vector<std::pair<std::string, std::string>>& schluessel) {
        auto e = karte.neueEntitaet(klassenname, hatPinsel);
        e->schicht = kSchichtKeine;
        for (const auto& [k, v] : schluessel) {
            e->setzen(k, v);
        }
        return e;
    }

    // Entity_parseTokens (parse.cpp:144-205)
    std::shared_ptr<Entitaet> entitaetLesen(int index) {
        std::shared_ptr<Entitaet> e;
        std::vector<std::pair<std::string, std::string>> schluessel;
        std::string klassenname;
        int primitive = 0;
        while (true) {
            t.nextLine();
            const char* token = nullptr;
            if (!ebenen.ebeneLesen(t) || !(token = t.getToken())) {
                unerwartet(t, token, "#entity-token");
                return nullptr;
            }
            if (std::strcmp(token, "}") == 0) {   // Ende der Entitaet
                if (!e) { e = erzeugen(klassenname, false, schluessel); }   // ohne Pinsel
                return e;
            }
            if (std::strcmp(token, "{") == 0) {   // Primitiv beginnt
                if (!e) { e = erzeugen(klassenname, true, schluessel); }   // mit Pinseln
                t.nextLine();
                auto p = primitivLesen();
                if (!p) {
                    t.melden("brush " + std::to_string(primitive) + ": parse error");
                    return nullptr;
                }
                p->schicht = ebenen.kartenIndex(ebenen.aktuell());
                if (e->istBehaelter()) {
                    e->kindEinfuegen(p);
                } else {
                    t.melden("entity " + std::to_string(index) + ": type " + klassenname + ": discarding brush " +
                             std::to_string(primitive));
                }
                ++primitive;
            } else {   // Schluessel und Wert
                const std::string key(token);
                token = t.getToken();
                if (token == nullptr) {
                    unerwartet(t, token, "#epair-value");
                    return nullptr;
                }
                schluessel.emplace_back(key, token);
                if (key == "classname") { klassenname = token; }
            }
        }
    }
};

// ===================== Schreiben (scripttokenwriter.h, write.cpp) ===========

class Schreiber {   // SimpleTokenWriter
public:
    std::string text;
    void nextLine() { m_trenner = '\n'; }
    void token(const char* t) {
        trenner();
        text += t;
    }
    void zeichenkette(const std::string& s) {   // writeString: ohne Maskierung
        trenner();
        text += '"';
        text += s;
        text += '"';
    }
    void ganzzahl(int i) {
        trenner();
        text += std::to_string(i);
    }
    void vorzeichenlos(std::size_t i) {
        trenner();
        text += std::to_string(i);
    }
    void zahl(double f) {
        trenner();
        text += dezimal(f);
    }
    void ende() { trenner(); }   // ~SimpleTokenWriter
private:
    void trenner() {
        text += m_trenner;
        m_trenner = ' ';
    }
    char m_trenner = '\n';
};

// shader_get_textureName: einfach die Laenge von "textures/" ueberspringen
std::string texturName(const std::string& shader) {
    const std::size_t n = std::strlen(kTexturPraefix);
    return shader.size() > n ? shader.substr(n) : std::string();
}

void ebenenPunkteSchreiben(Schreiber& w, const Flaeche& f) {   // FacePlane_exportTokens (quantiseFloating)
    for (std::size_t i = 0; i < 3; ++i) {
        w.token("(");
        for (std::size_t j = 0; j < 3; ++j) {
            w.zahl(f.ebenenPunkte()[i][j]);
        }
        w.token(")");
    }
}

void shaderSchreiben(Schreiber& w, const Flaeche& f) {   // FaceShader_exportTokens
    const std::string n = texturName(f.shader());
    w.token(n.empty() ? "NULL" : n.c_str());
}

void flagsSchreiben(Schreiber& w, const Flaeche& f) {   // FaceShader_ContentsFlagsValue_exportTokens
    w.ganzzahl(f.flags().inhaltFlags);
    w.ganzzahl(f.flags().flaechenFlags);
    w.ganzzahl(f.flags().wert);
}

void flaecheSchreiben(Schreiber& w, const Flaeche& f, PinselFormat format, std::vector<std::string>* meldungen) {
    const TexturProjektion& p = f.projektion();
    switch (format) {
    case PinselFormat::Quake3:   // QuakeFaceTokenExporter<yes>
        ebenenPunkteSchreiben(w, f);
        shaderSchreiben(w, f);
        if (!texdefGesund(p.texdef) && meldungen) { meldungen->push_back("FaceTexdef_exportTokens: bad texdef"); }
        w.zahl(p.texdef.shift[0]);
        w.zahl(p.texdef.shift[1]);
        w.zahl(p.texdef.rotate);
        w.zahl(p.texdef.scale[0]);
        w.zahl(p.texdef.scale[1]);
        flagsSchreiben(w, f);
        w.nextLine();
        break;
    case PinselFormat::Quake3BP:   // Quake3BPFaceTokenExporter<yes>
        ebenenPunkteSchreiben(w, f);
        w.token("(");
        for (int z = 0; z < 2; ++z) {
            w.token("(");
            for (int i = 0; i < 3; ++i) { w.zahl(p.bp.coords[z][i]); }
            w.token(")");
        }
        w.token(")");
        shaderSchreiben(w, f);
        flagsSchreiben(w, f);
        w.nextLine();
        break;
    case PinselFormat::Quake3Valve220:   // Valve220FaceTokenExporter<yes>
        ebenenPunkteSchreiben(w, f);
        shaderSchreiben(w, f);
        if (!texdefGesund(p.texdef) && meldungen) { meldungen->push_back("FaceTexdef_Valve220_exportTokens: bad texdef"); }
        w.token("[");
        w.zahl(p.basisS.x());
        w.zahl(p.basisS.y());
        w.zahl(p.basisS.z());
        w.zahl(p.texdef.shift[0]);
        w.token("]");
        w.token("[");
        w.zahl(p.basisT.x());
        w.zahl(p.basisT.y());
        w.zahl(p.basisT.z());
        w.zahl(p.texdef.shift[1]);
        w.token("]");
        w.zahl(-p.texdef.rotate);
        w.zahl(p.texdef.scale[0]);
        w.zahl(p.texdef.scale[1]);
        flagsSchreiben(w, f);
        w.nextLine();
        break;
    }
}

// BrushTokenExporter (brushtokens.h:575-653): nur beitragende Flaechen; ein
// Pinsel ohne solche schreibt gar nichts.
void pinselSchreiben(Schreiber& w, const Pinsel& p, const Kontext& k, std::vector<std::string>* meldungen) {
    p.bRep(k);
    if (!p.hatBeitragendeFlaechen(k)) {
        return;
    }
    w.token("{");
    w.nextLine();
    const bool bp = k.format == PinselFormat::Quake3BP;
    if (bp) {
        w.token("brushDef");
        w.nextLine();
        w.token("{");
        w.nextLine();
    }
    for (const auto& f : p.flaechen()) {
        if (f->traegtBei()) {
            flaecheSchreiben(w, *f, k.format, meldungen);
        }
    }
    if (bp) {
        w.token("}");
        w.nextLine();
    }
    w.token("}");
    w.nextLine();
}

// Patch_export* (patch.h:1151-1246)
void patchSchreibenTeil(Schreiber& w, const Patch& p, bool doom3) {
    w.token("{");
    w.nextLine();
    w.token(p.patchDef3 ? "patchDef3" : "patchDef2");
    w.nextLine();
    w.token("{");
    w.nextLine();
    const std::string n = texturName(p.shader());
    if (doom3) {   // PatchDoom3_exportShader
        w.zeichenkette(n.empty() ? "_emptyname" : p.shader());
    } else {   // Patch_exportShader
        w.token(n.empty() ? "NULL" : n.c_str());
    }
    w.nextLine();
    w.token("(");
    w.vorzeichenlos(p.breite());
    w.vorzeichenlos(p.hoehe());
    if (p.patchDef3) {
        w.vorzeichenlos(p.unterteilungX);
        w.vorzeichenlos(p.unterteilungY);
    }
    w.ganzzahl(0);
    w.ganzzahl(0);
    w.ganzzahl(0);
    w.token(")");
    w.nextLine();
    w.token("(");
    w.nextLine();
    for (std::size_t c = 0; c < p.breite(); ++c) {
        w.token("(");
        for (std::size_t r = 0; r < p.hoehe(); ++r) {
            w.token("(");
            const PatchPunkt& pp = p.punkt(r, c);
            w.zahl(pp.vertex[0]);
            w.zahl(pp.vertex[1]);
            w.zahl(pp.vertex[2]);
            w.zahl(pp.texcoord[0]);
            w.zahl(pp.texcoord[1]);
            w.token(")");
        }
        w.token(")");
        w.nextLine();
    }
    w.token(")");
    w.nextLine();
    w.token("}");
    w.nextLine();
    w.token("}");
    w.nextLine();
}

// Layer_Write (write.cpp:42-50)
void ebeneSchreiben(Schreiber& w, int schicht, int& aktuell) {
    if (schicht == kSchichtKeine) { return; }
    const int eigen = schicht == kSchichtUnerreichbar ? -1 : schicht;
    if (aktuell != eigen) {
        aktuell = eigen;
        w.token("//@$&");
        w.token("layer");
        w.ganzzahl(aktuell);
        w.nextLine();
    }
}

// UTF-8-Pfad (u8path ist in C++20 veraltet)
std::filesystem::path pfadAus(const std::string& pfad) {
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(pfad.data()), pfad.size()));
}

bool binaerLesen(const std::string& pfad, std::string& out) {
    std::ifstream f(pfadAus(pfad), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    out = s.str();
    return true;
}

}  // namespace

// ===================== oeffentliche Funktionen ==============================

std::string dezimal(double f) {   // ostream_write( Decimal )
    constexpr std::size_t groesse = 22;
    char buf[groesse];
    const int n = std::snprintf(buf, groesse, "%10.10lf", f);
    const std::size_t laenge = n < 0 ? 0 : static_cast<std::size_t>(n);
    const char* erstes = buf;
    for (; *erstes == ' '; ++erstes) {}
    const char* letztes = buf + std::min(laenge, groesse - 1) - 1;
    for (; *letztes == '0'; --letztes) {}
    if (*letztes == '.') { --letztes; }
    return std::string(erstes, static_cast<std::size_t>(letztes - erstes + 1));
}

// MapQ3API::readGraph -> Map_Read (parse.cpp:211-241)
LeseErgebnis karteLesen(const std::string& text, Karte& karte) {
    LeseErgebnis erg;
    karte.leeren();
    Tokenizer t(text, &erg.meldungen);
    EbenenLeser ebenen;
    if (!ebenen.definitionenLesen(t)) {
        ebenen.baumBauen(t);   // immer, damit es mindestens eine Ebene gibt
        ebenen.inKarte(karte);
        karte.anmelden();
        return erg;
    }
    ebenen.baumBauen(t);
    ebenen.inKarte(karte);

    Leser leser{t, karte, ebenen};
    int anzahl = 0;
    for (;;) {
        t.nextLine();
        if (!ebenen.ebeneLesen(t) || !t.getToken()) {   // { oder 0
            break;
        }
        const int entitaetsEbene = ebenen.kartenIndex(ebenen.aktuell());
        auto e = leser.entitaetLesen(anzahl);
        if (!e) {
            erg.meldungen.push_back("entity " + std::to_string(anzahl) + ": parse error");
            erg.vollstaendig = false;
            break;
        }
        if (!e->istBehaelter()) { e->schicht = entitaetsEbene; }
        karte.entitaetLaden(e);
        ++anzahl;
    }
    karte.anmelden();
    return erg;
}

// MapQ3API::writeGraph -> Map_Write / WriteTokensWalker (write.cpp)
std::string karteSchreiben(const Karte& karte, std::vector<std::string>* meldungen) {
    Schreiber w;
    // Layers_Write
    for (const Schicht& s : karte.schichten) {
        w.token("//@$&");
        w.token("layerdef");
        w.zeichenkette(s.name);
        w.ganzzahl(s.eltern);
        w.ganzzahl(s.farbe[0]);
        w.ganzzahl(s.farbe[1]);
        w.ganzzahl(s.farbe[2]);
        w.nextLine();
    }
    int aktuelleEbene = 0;   // LAYERIDX0
    std::size_t entitaeten = 0;
    for (const auto& e : karte.entitaeten()) {
        if (e->istBehaelter() && e->kinder().empty() && e->klassenname() != "worldspawn" && !e->hat("origin")) {
            if (meldungen) {
                meldungen->push_back("discarding empty group entity: # = " + std::to_string(entitaeten) +
                                     "; classname = " + e->klassenname());
            }
            continue;
        }
        w.token("//");
        w.token("entity");
        w.vorzeichenlos(entitaeten++);
        w.nextLine();
        ebeneSchreiben(w, e->schicht, aktuelleEbene);
        w.token("{");
        w.nextLine();
        for (const auto& [k, v] : e->schluessel()) {   // Entity_ExportTokens
            w.zeichenkette(k);
            w.zeichenkette(v);
            w.nextLine();
        }
        std::size_t pinsel = 0;
        for (const auto& p : e->kinder()) {
            w.token("//");
            w.token("brush");
            w.vorzeichenlos(pinsel++);
            w.nextLine();
            ebeneSchreiben(w, p->schicht, aktuelleEbene);
            if (p->art() == Primitiv::Art::Pinsel) {
                pinselSchreiben(w, static_cast<const Pinsel&>(*p), karte.kontext, meldungen);
            } else {
                patchSchreibenTeil(w, static_cast<const Patch&>(*p), false);
            }
        }
        w.token("}");
        w.nextLine();
    }
    w.ende();
    return w.text;
}

// TextFileInputStream "rt" (msvcrt): CR LF -> LF, Strg+Z = Dateiende.
std::string textmodusLesen(const std::string& roh) {
    std::string out;
    out.reserve(roh.size());
    for (std::size_t i = 0; i < roh.size(); ++i) {
        const char c = roh[i];
        if (c == '\x1a') { break; }
        if (c == '\r' && i + 1 < roh.size() && roh[i + 1] == '\n') { continue; }
        out += c;
    }
    return out;
}

// TextFileOutputStream "wt" (msvcrt): jedes LF wird CR LF.
std::string textmodusSchreiben(const std::string& text) {
    std::string out;
    out.reserve(text.size() + text.size() / 16);
    for (const char c : text) {
        if (c == '\n') { out += '\r'; }
        out += c;
    }
    return out;
}

bool karteLaden(const std::string& pfad, Karte& karte, LeseErgebnis& ergebnis) {   // MapResource_loadFile
    std::string roh;
    if (!binaerLesen(pfad, roh)) {
        ergebnis.vollstaendig = false;
        ergebnis.meldungen.push_back("Open file " + pfad + " for read...failure");
        return false;
    }
    ergebnis = karteLesen(textmodusLesen(roh), karte);
    return true;
}

// MapResource_save (referencecache.cpp:122-140) und MapResource_saveFile
bool karteSpeichern(const std::string& pfad, const Karte& karte, bool sicherung, std::string* fehler) {
    const std::filesystem::path p = pfadAus(pfad);
    std::error_code ec;
    if (sicherung && std::filesystem::exists(p, ec) && !std::filesystem::is_symlink(p, ec)) {
        // file_saveBackup: PathExtensionless( path ) + ".bak"
        std::filesystem::path bak = p;
        bak.replace_extension(".bak");
        std::filesystem::rename(p, bak, ec);
        if (ec && std::filesystem::exists(p)) {
            if (fehler) { *fehler = "map path (or backup path) is not writable: \"" + pfad + "\""; }
            return false;
        }
    }
    const std::string text = textmodusSchreiben(karteSchreiben(karte));
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f) {
        if (fehler) { *fehler = "Open file " + pfad + " for write...failure"; }
        return false;
    }
    f.write(text.data(), static_cast<std::streamsize>(text.size()));
    return static_cast<bool>(f);
}

// PatchDoom3TokenImporter (patch.h:1134-1149): ab "{" nach "patchDef2/3"
bool patchDoom3Lesen(const std::string& text, Patch& patch, bool patchDef3, std::vector<std::string>* meldungen) {
    Tokenizer t(text, meldungen);
    patch.patchDef3 = patchDef3;
    RUECK_FALSCH(patchKopfLesen(t));
    RUECK_FALSCH(patchShaderDoom3Lesen(patch, t));
    RUECK_FALSCH(patchParameterLesen(patch, t));
    RUECK_FALSCH(patchMatrixLesen(patch, t));
    RUECK_FALSCH(patchFussLesen(patch, t));
    return true;
}

// PatchDoom3TokenExporter (patch.h:1248-1261)
std::string patchDoom3Schreiben(const Patch& patch) {
    Schreiber w;
    patchSchreibenTeil(w, patch, true);
    w.ende();
    return w.text;
}

}  // namespace bhed::karte
