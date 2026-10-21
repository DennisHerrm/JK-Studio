// eklasse_lesen.cpp - Karten-Editor: Entitaetsklassen aus .def (QUAKED) und
// .ent (XML) lesen.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/eclass_def.cpp
// (COM_Parse, setSpecialLoad, Eclass_InitFromText, Eclass_ScanFile),
// radiant/eclass_xml.cpp (ClassImporter, AttributeImporter, ListAttribute-
// Importer, attributeSupported), radiant/eclass.cpp (EntityClassFilterMode,
// EntityClassesLoadFile, EntityClassQuake3_Construct), libs/xml/xmlparser.h
// (SAX-Aufrufe). Der XML-Leser ist ein kleiner eigener SAX-Leser, der dieselben
// Ereignisse liefert wie libxml2 mit replaceEntities = 1 (Element auf/zu,
// Zeichen inklusive Leerraum, Entitaeten ersetzt, CDATA als Zeichen,
// Kommentare und Verarbeitungsanweisungen uebergangen).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/entdefs.h"
#include "bhed/karte/eklasse.h"
#include "bhed/karte/kartenformat.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace bhed::karte {

namespace {

// ===================== .def (eclass_def.cpp) ================================

// COM_Parse (:76-139): ein Token aus einem Text; nullptr am Ende.
const char* comParse(const char* data, std::string& token) {
    token.clear();
    if (!data) { return nullptr; }
    int c = 0;
    // Leerraum ueberspringen (char ist vorzeichenbehaftet: Bytes ab 0x80 sind < ' ')
skipwhite:
    while ((c = static_cast<signed char>(*data)) <= ' ') {
        if (c == 0) { return nullptr; }
        data++;
    }
    // // Kommentare
    if (c == '/' && data[1] == '/') {
        while (*data && *data != '\n') { data++; }
        goto skipwhite;
    }
    // Zeichenketten in Anfuehrungszeichen
    if (c == '\"') {
        data++;
        do {
            c = static_cast<signed char>(*data++);
            if (c == '\"' || c == 0) {   // NRC liest bei fehlendem " ueber das Ende hinaus
                return c == 0 ? data - 1 : data;
            }
            token += static_cast<char>(c);
        } while (true);
    }
    // einzelne Zeichen
    if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ':') {
        token += static_cast<char>(c);
        return data + 1;
    }
    // ein Wort
    do {
        token += static_cast<char>(c);
        data++;
        c = static_cast<signed char>(*data);
        if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ':') { break; }
    } while (c > 32);
    return data;
}

std::string pfadBereinigt(const std::string& p) {   // PathCleaned: '\' -> '/'
    std::string r = p;
    std::replace(r.begin(), r.end(), '\\', '/');
    return r;
}

// setSpecialLoad (:146-171): "model=" im Kommentar
void besonderesLaden(const EntitaetsKlasse& e, const char* was, std::string& p) {
    const std::size_t wo = e.kommentar.find(was);
    if (wo == std::string::npos) { return; }
    std::size_t text = wo + std::strlen(was);
    if (text < e.kommentar.size() && e.kommentar[text] == '\"') { text++; }
    const std::size_t ende = e.kommentar.find('\"', text);
    if (ende != std::string::npos) {
        p = e.kommentar.substr(text, ende - text);
    } else {
        p = e.kommentar.substr(text);
    }
}

bool beginntOhneFall(const std::string& s, const char* p) {   // string_equal_prefix_nocase
    for (std::size_t i = 0; p[i] != '\0'; ++i) {
        if (i >= s.size() || std::tolower(static_cast<unsigned char>(s[i])) != std::tolower(static_cast<unsigned char>(p[i]))) {
            return false;
        }
    }
    return true;
}

void attributEinfuegen(EntitaetsKlasse& e, const std::string& key, const KlassenAttribut& a) {   // EntityClass_insertAttribute
    e.attribute.emplace_back(key, a);
}

// Eclass_InitFromText (:185-263). nullptr: "Error parsing".
std::unique_ptr<EntitaetsKlasse> klasseAusText(const char* text, std::string& debugname) {
    auto e = std::make_unique<EntitaetsKlasse>();
    std::string token;
    // der Name
    text = comParse(text, token);
    e->setzeName(token);
    debugname = e->name;
    if (!text) { return e; }
    {
        // die Farbe
        const int r = std::sscanf(text, " (%f %f %f)", &e->farbe[0], &e->farbe[1], &e->farbe[2]);
        if (r != 3) { return e; }
    }
    while (*text != ')') {
        if (!*text) { return nullptr; }
        text++;
    }
    text++;
    // die Groesse
    text = comParse(text, token);
    if (!text) { return nullptr; }   // NRC liest hier ueber das Ende
    if (token[0] == '(') {
        e->fixedsize = true;
        const int r = std::sscanf(text, "%f %f %f) (%f %f %f)", &e->mins[0], &e->mins[1], &e->mins[2], &e->maxs[0],
                                  &e->maxs[1], &e->maxs[2]);
        if (r != 6) { return nullptr; }
        for (int i = 0; i < 2; ++i) {
            while (*text != ')') {
                if (!*text) { return nullptr; }
                text++;
            }
            text++;
        }
    }
    {   // der Rest der Zeile: Namen der Spawnflags
        const char* p = text;
        while (*text && *text++ != '\n') {}
        for (std::size_t i = 0; i < kMaxFlags; ++i) {
            p = comParse(p, token);
            if (p == nullptr || p > text) { break; }
            if (token == "-" || token == "x" || beginntOhneFall(token, "unused")) { continue; }
            e->flagnamen[i] = token.substr(0, 31);   // strncpy( flagnames[i], token, 31 )
        }
    }
    e->kommentar = text;
    besonderesLaden(*e, "model=", e->modell);
    e->modell = pfadBereinigt(e->modell);
    if (!e->fixedsize) {
        attributEinfuegen(*e, "angle", KlassenAttribut{"direction", "Direction", "", ""});
    } else {
        attributEinfuegen(*e, "angle", KlassenAttribut{"angle", "Yaw Angle", "0", ""});
    }
    attributEinfuegen(*e, "model", KlassenAttribut{"model", "Model", "", ""});
    attributEinfuegen(*e, "noise", KlassenAttribut{"sound", "Sound", "", ""});
    return e;
}

// ===================== kleiner SAX-Leser fuer .ent ==========================

struct XmlElement {   // XMLElement / SAXElement
    std::string name;
    std::vector<std::pair<std::string, std::string>> attribute;
    [[nodiscard]] std::string attribut(const char* n) const {   // "" wenn nicht da
        for (const auto& [k, v] : attribute) {
            if (k == n) { return v; }
        }
        return {};
    }
};

class XmlEmpfaenger {   // XMLImporter
public:
    virtual ~XmlEmpfaenger() = default;
    virtual void elementAuf(const XmlElement& e) = 0;
    virtual void elementZu(const std::string& name) = 0;
    virtual void zeichen(const std::string& text) = 0;
};

// Latin-1 -> UTF-8 (libxml2 wandelt in UTF-8)
std::string latin1ZuUtf8(const std::string& s) {
    std::string r;
    for (const unsigned char c : s) {
        if (c < 0x80) {
            r += static_cast<char>(c);
        } else {
            r += static_cast<char>(0xC0 | (c >> 6));
            r += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    return r;
}

void utf8Anhaengen(std::string& r, unsigned long cp) {
    if (cp < 0x80) {
        r += static_cast<char>(cp);
    } else if (cp < 0x800) {
        r += static_cast<char>(0xC0 | (cp >> 6));
        r += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        r += static_cast<char>(0xE0 | (cp >> 12));
        r += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        r += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        r += static_cast<char>(0xF0 | (cp >> 18));
        r += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        r += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        r += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

// Entitaeten ersetzen (&amp; &lt; &gt; &quot; &apos; &#n; &#xn;)
std::string entitaetenErsetzen(const std::string& s) {
    std::string r;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '&') {
            r += s[i];
            continue;
        }
        const std::size_t ende = s.find(';', i);
        if (ende == std::string::npos) {
            r += s[i];
            continue;
        }
        const std::string n = s.substr(i + 1, ende - i - 1);
        if (n == "amp") { r += '&'; }
        else if (n == "lt") { r += '<'; }
        else if (n == "gt") { r += '>'; }
        else if (n == "quot") { r += '"'; }
        else if (n == "apos") { r += '\''; }
        else if (!n.empty() && n[0] == '#') {
            const unsigned long cp = (n.size() > 1 && (n[1] == 'x' || n[1] == 'X')) ? std::strtoul(n.c_str() + 2, nullptr, 16)
                                                                                    : std::strtoul(n.c_str() + 1, nullptr, 10);
            utf8Anhaengen(r, cp);
        } else {
            r += s.substr(i, ende - i + 1);   // unbekannte Entitaet: stehen lassen
        }
        i = ende;
    }
    return r;
}

// Attributwert-Normalisierung (XML 1.0, 3.3.3): \t \n \r -> Leerzeichen
std::string attributNormalisieren(const std::string& s) {
    std::string r = s;
    for (char& c : r) {
        if (c == '\t' || c == '\n' || c == '\r') { c = ' '; }
    }
    return entitaetenErsetzen(r);
}

// Liefert false bei einem Syntaxfehler (libxml2: "XML ERROR").
bool xmlLesen(std::string text, XmlEmpfaenger& z, std::vector<std::string>& fehler) {
    // Kodierung aus der Deklaration
    if (text.compare(0, 5, "<?xml") == 0) {
        const std::size_t e = text.find("?>");
        const std::string kopf = text.substr(0, e);
        std::string klein;
        for (const char c : kopf) { klein += static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        if (klein.find("iso-8859-1") != std::string::npos || klein.find("latin1") != std::string::npos) {
            text = latin1ZuUtf8(text);
        }
    }
    std::size_t i = 0;
    std::string zeichen;
    auto zeichenAbgeben = [&]() {
        if (!zeichen.empty()) {
            z.zeichen(entitaetenErsetzen(zeichen));
            zeichen.clear();
        }
    };
    while (i < text.size()) {
        if (text[i] != '<') {
            zeichen += text[i++];
            continue;
        }
        if (text.compare(i, 4, "<!--") == 0) {
            const std::size_t e = text.find("-->", i + 4);
            if (e == std::string::npos) { fehler.push_back("XML ERROR: Comment not terminated"); return false; }
            zeichenAbgeben();
            i = e + 3;
            continue;
        }
        if (text.compare(i, 9, "<![CDATA[") == 0) {
            const std::size_t e = text.find("]]>", i + 9);
            if (e == std::string::npos) { fehler.push_back("XML ERROR: CData section not finished"); return false; }
            zeichenAbgeben();
            z.zeichen(text.substr(i + 9, e - i - 9));   // ohne cdataBlock: als characters
            i = e + 3;
            continue;
        }
        if (text.compare(i, 2, "<?") == 0 || text.compare(i, 2, "<!") == 0) {
            const std::size_t e = text.find('>', i);
            if (e == std::string::npos) { fehler.push_back("XML ERROR: unterminated declaration"); return false; }
            zeichenAbgeben();
            i = e + 1;
            continue;
        }
        zeichenAbgeben();
        if (text.compare(i, 2, "</") == 0) {
            const std::size_t e = text.find('>', i);
            if (e == std::string::npos) { fehler.push_back("XML ERROR: unterminated end tag"); return false; }
            std::string name = text.substr(i + 2, e - i - 2);
            while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) { name.pop_back(); }
            z.elementZu(name);
            i = e + 1;
            continue;
        }
        // Starttag
        std::size_t j = i + 1;
        XmlElement el;
        while (j < text.size() && !std::isspace(static_cast<unsigned char>(text[j])) && text[j] != '>' && text[j] != '/') {
            el.name += text[j++];
        }
        bool leer = false;
        while (j < text.size()) {
            while (j < text.size() && std::isspace(static_cast<unsigned char>(text[j]))) { ++j; }
            if (j >= text.size()) { break; }
            if (text[j] == '>') { ++j; break; }
            if (text[j] == '/' && j + 1 < text.size() && text[j + 1] == '>') {
                leer = true;
                j += 2;
                break;
            }
            std::string an;
            while (j < text.size() && text[j] != '=' && !std::isspace(static_cast<unsigned char>(text[j])) && text[j] != '>') {
                an += text[j++];
            }
            while (j < text.size() && std::isspace(static_cast<unsigned char>(text[j]))) { ++j; }
            if (j >= text.size() || text[j] != '=') { fehler.push_back("XML ERROR: Specification mandates value for attribute " + an); return false; }
            ++j;
            while (j < text.size() && std::isspace(static_cast<unsigned char>(text[j]))) { ++j; }
            if (j >= text.size() || (text[j] != '"' && text[j] != '\'')) { fehler.push_back("XML ERROR: AttValue: \" or ' expected"); return false; }
            const char q = text[j++];
            const std::size_t e = text.find(q, j);
            if (e == std::string::npos) { fehler.push_back("XML ERROR: AttValue: ' expected"); return false; }
            el.attribute.emplace_back(an, attributNormalisieren(text.substr(j, e - j)));
            j = e + 1;
        }
        z.elementAuf(el);
        if (leer) { z.elementZu(el.name); }
        i = j;
    }
    zeichenAbgeben();
    return true;
}

// ===================== .ent (eclass_xml.cpp) ================================

// attributeSupported (:285-306)
bool attributUnterstuetzt(const std::string& n) {
    static const char* const namen[] = {"string", "array", "integer", "boolean", "integer2", "integer3", "real",
                                        "angle", "direction", "real3", "angles", "color", "target", "targetname",
                                        "sound", "texture", "model", "skin", "flag"};
    return std::any_of(std::begin(namen), std::end(namen), [&](const char* x) { return n == x; });
}

// string_parse_vector3 (stringio.h:203-218): genau ein Leerzeichen zwischen den Zahlen
void vektorLesen(const std::string& s, float* v, std::size_t n) {
    const char* p = s.c_str();
    if (*p == '\0' || *p == ' ') { return; }
    for (std::size_t i = 0;;) {
        char* ende = nullptr;
        v[i] = static_cast<float>(std::strtod(p, &ende));
        p = ende;
        if (++i == n) { return; }
        if (*p++ != ' ') { return; }
    }
}

class EntLeser final : public XmlEmpfaenger {
public:
    EntLeser(EntitaetsKlassen& k, std::vector<std::string>& fehler, std::string quelle)
        : m_klassen(k), m_fehler(fehler), m_quelle(std::move(quelle)) {}

    void elementAuf(const XmlElement& el) override {
        const Stufe s = m_stufen.empty() ? Stufe::Dokument : m_stufen.back();
        switch (s) {
        case Stufe::Dokument:   // EclassXMLImporter
            if (el.name == "classes") {
                m_stufen.push_back(Stufe::Klassen);
            } else {
                fehlerMelden(el.name, "classes");
                m_stufen.push_back(s);
            }
            return;
        case Stufe::Klassen:   // ClassesImporter
            if (el.name == "group" || el.name == "point") {
                klasseBeginnen(el);
                m_stufen.push_back(Stufe::Klasse);
            } else if (el.name == "list") {
                m_liste = &m_listen[el.attribut("name")];   // listTypes[name]
                m_stufen.push_back(Stufe::Liste);
            } else {
                fehlerMelden(el.name, "classes");
                m_stufen.push_back(s);
            }
            return;
        case Stufe::Klasse:   // ClassImporter
            if (attributUnterstuetzt(el.name) || m_listen.count(el.name) != 0) {
                attributBeginnen(el);
                m_stufen.push_back(Stufe::Attribut);
            } else {
                fehlerMelden(el.name, "class");
                m_stufen.push_back(s);
            }
            return;
        case Stufe::Liste:   // ListAttributeImporter
            if (el.name == "item") {
                m_liste->emplace_back(el.attribut("name"), el.attribut("value"));
                m_stufen.push_back(Stufe::Eintrag);
            } else {
                fehlerMelden(el.name, "list");
                m_stufen.push_back(s);
            }
            return;
        case Stufe::Attribut:
            fehlerMelden(el.name, "attribute");
            m_stufen.push_back(s);
            return;
        case Stufe::Eintrag:
            fehlerMelden(el.name, "item");
            m_stufen.push_back(s);
            return;
        }
    }

    void elementZu(const std::string& name) override {
        if (m_stufen.empty()) { return; }
        const Stufe s = m_stufen.back();
        m_stufen.pop_back();
        const Stufe eltern = m_stufen.empty() ? Stufe::Dokument : m_stufen.back();
        if (s == Stufe::Klasse && eltern == Stufe::Klassen && (name == "group" || name == "point")) {
            klasseBeenden();
        }
    }

    void zeichen(const std::string& t) override {
        const Stufe s = m_stufen.empty() ? Stufe::Dokument : m_stufen.back();
        if (s == Stufe::Klasse) {
            m_kommentar += t;   // ClassImporter::write
        } else if (s == Stufe::Attribut) {
            // AttributeImporter::write: Beschreibung und Kommentar
            m_klasse->attribute[m_attribut].second.beschreibung += t;
            m_kommentar += t;
        }
    }

private:
    enum class Stufe { Dokument, Klassen, Klasse, Liste, Attribut, Eintrag };

    void fehlerMelden(const std::string& kind, const char* eltern) {   // PARSE_ERROR
        m_fehler.push_back("\"" + kind + "\" is not a valid child of \"" + eltern + "\"");
    }

    // ClassImporter( collector, listTypes, element ) (:320-345)
    void klasseBeginnen(const XmlElement& el) {
        m_klasse = std::make_unique<EntitaetsKlasse>();
        m_klasse->quelle = m_quelle;
        m_klasse->setzeName(el.attribut("name"));
        vektorLesen(el.attribut("color"), m_klasse->farbe.data(), 3);
        const std::string modell = el.attribut("model");
        if (!modell.empty()) { m_klasse->modell = pfadBereinigt(modell); }
        if (el.name == "point") {
            m_klasse->fixedsize = true;
            float box[6] = {m_klasse->mins[0], m_klasse->mins[1], m_klasse->mins[2],
                            m_klasse->maxs[0], m_klasse->maxs[1], m_klasse->maxs[2]};
            vektorLesen(el.attribut("box"), box, 6);
            m_klasse->mins = Vektor3f(box[0], box[1], box[2]);
            m_klasse->maxs = Vektor3f(box[3], box[4], box[5]);
        }
        m_kommentar.clear();
    }

    // ~ClassImporter (:346-354): Klasse und alle bekannten Listen einfuegen
    void klasseBeenden() {
        m_klasse->kommentar = m_kommentar;
        m_klassen.einfuegen(std::move(m_klasse));
        for (const auto& [name, liste] : m_listen) {
            m_klassen.listeEinfuegen(name, liste);
        }
    }

    // AttributeImporter( comment, entityClass, element ) (:205-245)
    void attributBeginnen(const XmlElement& el) {
        const std::string typ = el.name;
        const std::string key = el.attribut("key");
        m_klasse->attribute.emplace_back(key, KlassenAttribut{typ, el.attribut("name"), el.attribut("value"), ""});
        m_attribut = m_klasse->attribute.size() - 1;
        if (typ == "flag") {
            const std::size_t bit = static_cast<std::size_t>(std::atoi(el.attribut("bit").c_str()));
            if (bit < kMaxFlags) {   // NRC: ASSERT_MESSAGE( bit < MAX_FLAGS )
                m_klasse->flagnamen[bit] = key.substr(0, 31);
                m_klasse->flagAttribut[bit] = static_cast<int>(m_attribut);
            }
        } else if (m_klasse->fixedsize && typ == "model") {
            m_klasse->miscmodelIs = true;
            m_klasse->miscmodelKey = key;
        } else if (typ == "angle" || typ == "angles" || typ == "direction") {
            m_klasse->hasAngles = true;
            if (typ == "angles") {
                m_klasse->hasAnglesKey = true;
            } else if (typ == "direction") {
                m_klasse->hasDirectionKey = true;
            }
        }
        m_kommentar += key;
        m_kommentar += " : ";
    }

    EntitaetsKlassen& m_klassen;
    std::vector<std::string>& m_fehler;
    std::string m_quelle;
    std::vector<Stufe> m_stufen;
    std::unique_ptr<EntitaetsKlasse> m_klasse;
    std::string m_kommentar;
    std::size_t m_attribut = 0;
    std::map<std::string, ListenTyp> m_listen;   // ListAttributeTypes des ClassesImporter
    ListenTyp* m_liste = nullptr;
};

bool dateiLesen(const std::filesystem::path& p, std::string& out) {
    std::ifstream f(p, std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    out = textmodusLesen(s.str());   // TextFileInputStream "rt"
    return true;
}

bool kleinerOhneFall(const std::string& a, const std::string& b) {   // path_less (OS_CASE_INSENSITIVE)
    const std::size_t n = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        const int ca = std::tolower(static_cast<unsigned char>(a[i]));
        const int cb = std::tolower(static_cast<unsigned char>(b[i]));
        if (ca != cb) { return ca < cb; }
    }
    return a.size() < b.size();
}

}  // namespace

// Eclass_ScanFile (eclass_def.cpp:265-382): Zustandsautomat ueber den Text
std::size_t EntitaetsKlassen::defLesen(const std::string& text, const std::string& quelle) {
    enum Zustand { Normal, Schraeg, Kommentar, Quaked, Klasse, KlasseEnde } z = Normal;
    static const char* const kQuaked = "QUAKED";
    const char* p = nullptr;
    std::string puffer;
    std::size_t fehler = 0;
    for (const char c : text) {
        switch (z) {
        case Normal:
            if (c == '/') { z = Schraeg; }
            break;
        case Schraeg:
            if (c == '/') {
                z = Kommentar;
            } else if (c == '*') {
                p = kQuaked;
                z = Quaked;
            } else {
                z = Normal;
            }
            break;
        case Kommentar:
            if (c == '\n') { z = Normal; }
            break;
        case Quaked:
            if (c == *p) {
                if (*(++p) == '\0') { z = Klasse; }
            } else {
                z = Normal;
            }
            break;
        case Klasse:
            if (c == '*') {
                z = KlasseEnde;
            } else {
                puffer.push_back(c);
            }
            break;
        case KlasseEnde:
            if (c == '/') {
                std::string debugname;
                auto e = klasseAusText(puffer.c_str(), debugname);
                if (e) {
                    e->quelle = quelle;
                    einfuegen(std::move(e));
                } else {
                    ++fehler;   // NRC: "Error parsing: <debugname> in <filename>"
                }
                puffer.clear();
                z = Normal;
            } else {
                puffer.push_back('*');
                puffer.push_back(c);
                z = Klasse;
            }
            break;
        }
    }
    return fehler;
}

// ScanFile (eclass_xml.cpp:520-532)
std::size_t EntitaetsKlassen::entLesen(const std::string& text, const std::string& quelle) {
    std::vector<std::string> fehler;
    EntLeser leser(*this, fehler, quelle);
    const bool ok = xmlLesen(text, leser, fehler);
    return fehler.size() + (ok ? 0 : 1);
}

// EntityClassQuake3_Construct (eclass.cpp:185-213) mit EntityClassesLoadFile
// (:131-160): Module alphabetisch ("def" vor "xml"), je Modul alle Dateien
// des Ordners nach Pfad sortiert (ohne Gross/klein).
std::size_t EntitaetsKlassen::ordnerLaden(const std::string& ordner, const std::string& spielmodus) {
    std::size_t fehler = 0;
    const std::filesystem::path o(std::u8string(reinterpret_cast<const char8_t*>(ordner.data()), ordner.size()));
    for (const char* endung : {".def", ".ent"}) {
        std::vector<std::string> namen;
        std::error_code ec;
        for (const auto& d : std::filesystem::directory_iterator(o, ec)) {
            if (!d.is_regular_file()) { continue; }
            std::string ext = d.path().extension().string();
            for (char& c : ext) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            if (ext == endung) { namen.push_back(d.path().filename().string()); }
        }
        std::sort(namen.begin(), namen.end(), kleinerOhneFall);
        for (const auto& name : namen) {
            // eclass_filter_gamemode: sp ignoriert "mp_", mp ignoriert "sp_" (case-sensitiv, string_equal_n)
            const bool sp = spielmodus.empty() || spielmodus == "sp";
            if ((sp && name.compare(0, 3, "mp_") == 0) || (!sp && name.compare(0, 3, "sp_") == 0)) { continue; }
            std::string text;
            if (!dateiLesen(o / name, text)) {
                ++fehler;
                continue;
            }
            fehler += std::string(endung) == ".def" ? defLesen(text, name) : entLesen(text, name);
        }
    }
    return fehler;
}

// Die Tabelle aus bhed/entdefs.h als QUAKED-Bloecke: Kopfzeile wie im
// Quelltext ("?" fuer Pinselklassen, Spawnflags nach Bit, Luecken als "x"),
// darunter der englische Text und die Felder als "name"-Zeilen.
std::size_t EntitaetsKlassen::entdefsErgaenzen() {
    auto zahl = [](float f) {
        char b[32];
        std::snprintf(b, sizeof(b), "%g", static_cast<double>(f));
        return std::string(b);
    };
    std::string text;
    for (std::size_t i = 0; i < ent::anzahl(); ++i) {
        const ent::Def& d = ent::at(i);
        text += "/*QUAKED ";
        text += d.klasse;
        text += " (" + zahl(d.farbe[0]) + ' ' + zahl(d.farbe[1]) + ' ' + zahl(d.farbe[2]) + ") ";
        if (d.brush) {
            text += '?';
        } else {
            text += '(' + zahl(d.mins[0]) + ' ' + zahl(d.mins[1]) + ' ' + zahl(d.mins[2]) + ") (" + zahl(d.maxs[0]) + ' ' +
                    zahl(d.maxs[1]) + ' ' + zahl(d.maxs[2]) + ')';
        }
        std::string namen[kMaxFlags];
        std::size_t hoechste = 0;
        for (std::size_t f = 0; f < d.flagAnzahl; ++f) {
            for (std::size_t b = 0; b < kMaxFlags; ++b) {
                if (d.flags[f].bit == (1 << b)) {
                    namen[b] = d.flags[f].name;
                    hoechste = std::max(hoechste, b + 1);
                }
            }
        }
        for (std::size_t b = 0; b < hoechste; ++b) {
            text += ' ';
            text += namen[b].empty() ? std::string("x") : namen[b];
        }
        text += '\n';
        text += d.text[0];
        text += '\n';
        for (std::size_t f = 0; f < d.feldAnzahl; ++f) {
            text += '"';
            text += d.felder[f].name;
            text += "\" ";
            text += d.felder[f].text[0];
            text += '\n';
        }
        text += "*/\n\n";
    }
    return defLesen(text, "entdefs.h");
}

}  // namespace bhed::karte
