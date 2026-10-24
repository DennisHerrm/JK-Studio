// texturtags.cpp - Karten-Editor: die Textur-Tags (shadertags.xml).
// Portiert aus NetRadiant-Custom 20260114 (Quellen in texturtags.h).
//
//   Copyright (C) 2006, Stefan Greven. All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/texturtags.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace bhed::karte {

struct TexturTags::Knoten {
    enum class Art { Dokument, Element, Text, Kommentar };
    Art art = Art::Element;
    std::string name;
    std::vector<std::pair<std::string, std::string>> attribute;
    std::string inhalt;
    Knoten* eltern = nullptr;
    std::vector<std::unique_ptr<Knoten>> kinder;
    std::string kodierung;   // nur Dokument: encoding der Erklaerung
};

namespace {

using K = TexturTags::Knoten;

std::unique_ptr<K> neuerText(const std::string& s) {
    auto t = std::make_unique<K>();
    t->art = K::Art::Text;
    t->inhalt = s;
    return t;
}

std::unique_ptr<K> neuesElement(const std::string& name) {
    auto e = std::make_unique<K>();
    e->art = K::Art::Element;
    e->name = name;
    return e;
}

std::size_t indexVon(const K* k) {
    const auto& v = k->eltern->kinder;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i].get() == k) { return i; }
    }
    return v.size();
}

K* naechster(const K* k) {
    if (k->eltern == nullptr) { return nullptr; }
    const std::size_t i = indexVon(k);
    return i + 1 < k->eltern->kinder.size() ? k->eltern->kinder[i + 1].get() : nullptr;
}

K* voriger(const K* k) {
    if (k->eltern == nullptr) { return nullptr; }
    const std::size_t i = indexVon(k);
    return i > 0 && i < k->eltern->kinder.size() ? k->eltern->kinder[i - 1].get() : nullptr;
}

K* ersterKind(const K* k) { return k->kinder.empty() ? nullptr : k->kinder.front().get(); }

// xmlAddNextSibling: Text an Text wird zusammengelegt
K* danach(K* cur, std::unique_ptr<K> elem) {
    if (elem->art == K::Art::Text) {
        if (cur->art == K::Art::Text) {
            cur->inhalt += elem->inhalt;
            return cur;
        }
        K* n = naechster(cur);
        if (n != nullptr && n->art == K::Art::Text) {
            n->inhalt = elem->inhalt + n->inhalt;
            return n;
        }
    }
    K* eltern = cur->eltern;
    elem->eltern = eltern;
    K* r = elem.get();
    eltern->kinder.insert(eltern->kinder.begin() + static_cast<std::ptrdiff_t>(indexVon(cur) + 1), std::move(elem));
    return r;
}

// xmlAddChild: Text in einen Textknoten bzw. an einen Text am Ende wird zusammengelegt
K* kindDazu(K* eltern, std::unique_ptr<K> cur) {
    if (cur->art == K::Art::Text) {
        if (eltern->art == K::Art::Text) {
            eltern->inhalt += cur->inhalt;
            return eltern;
        }
        if (!eltern->kinder.empty() && eltern->kinder.back()->art == K::Art::Text) {
            eltern->kinder.back()->inhalt += cur->inhalt;
            return eltern->kinder.back().get();
        }
    }
    cur->eltern = eltern;
    eltern->kinder.push_back(std::move(cur));
    return eltern->kinder.back().get();
}

void entfernen(K* k) {   // xmlUnlinkNode + xmlFreeNode
    if (k == nullptr || k->eltern == nullptr) { return; }
    auto& v = k->eltern->kinder;
    v.erase(v.begin() + static_cast<std::ptrdiff_t>(indexVon(k)));
}

void inhaltSetzen(K* e, const std::string& s) {   // xmlNodeSetContent (Element)
    e->kinder.clear();
    if (!s.empty()) { kindDazu(e, neuerText(s)); }
}

std::string inhalt(const K* k) {   // xmlNodeGetContent
    if (k->art == K::Art::Text) { return k->inhalt; }
    std::string r;
    for (const auto& c : k->kinder) {
        if (c->art == K::Art::Text || c->art == K::Art::Element) { r += inhalt(c.get()); }
    }
    return r;
}

const std::string* attribut(const K* e, const std::string& name) {
    for (const auto& [n, v] : e->attribute) {
        if (n == name) { return &v; }
    }
    return nullptr;
}

void attributSetzen(K* e, const std::string& name, const std::string& wert) {   // xmlSetProp
    for (auto& [n, v] : e->attribute) {
        if (n == name) {
            v = wert;
            return;
        }
    }
    e->attribute.emplace_back(name, wert);
}

K* wurzelElement(const K* dok) {   // "/root"
    for (const auto& c : dok->kinder) {
        if (c->art == K::Art::Element) { return c->name == "root" ? c.get() : nullptr; }
    }
    return nullptr;
}

// "/root/*/*": alle Elementkinder der Elementkinder von root, in Dokumentreihenfolge
std::vector<K*> shaderKnoten(const K* dok) {
    std::vector<K*> r;
    const K* root = wurzelElement(dok);
    if (root == nullptr) { return r; }
    for (const auto& g : root->kinder) {
        if (g->art != K::Art::Element) { continue; }
        for (const auto& s : g->kinder) {
            if (s->art == K::Art::Element) { r.push_back(s.get()); }
        }
    }
    return r;
}

bool pfadGleich(const K* s, const std::string& shader) {
    const std::string* p = attribut(s, "path");
    return p != nullptr && *p == shader;
}

std::vector<K*> tagKinder(const K* s) {   // child::tag
    std::vector<K*> r;
    for (const auto& c : s->kinder) {
        if (c->art == K::Art::Element && c->name == "tag") { r.push_back(c.get()); }
    }
    return r;
}

bool hatTagInhalt(const K* s, const std::string& tag) {
    const auto tags = tagKinder(s);
    return std::any_of(tags.begin(), tags.end(), [&](const K* t) { return inhalt(t) == tag; });
}

// --- Lesen (xmlParseFile, Leerraum bleibt) -----------------------------------------

void utf8(std::string& s, std::uint32_t c) {
    if (c < 0x80) {
        s += static_cast<char>(c);
    } else if (c < 0x800) {
        s += static_cast<char>(0xC0 | (c >> 6));
        s += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        s += static_cast<char>(0xE0 | (c >> 12));
        s += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        s += static_cast<char>(0xF0 | (c >> 18));
        s += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        s += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (c & 0x3F));
    }
}

std::string entitaetenAufloesen(const std::string& s) {
    std::string r;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '&') {
            r += s[i];
            continue;
        }
        const std::size_t e = s.find(';', i);
        if (e == std::string::npos) {
            r += s[i];
            continue;
        }
        const std::string n = s.substr(i + 1, e - i - 1);
        if (n == "lt") {
            r += '<';
        } else if (n == "gt") {
            r += '>';
        } else if (n == "amp") {
            r += '&';
        } else if (n == "quot") {
            r += '"';
        } else if (n == "apos") {
            r += '\'';
        } else if (!n.empty() && n[0] == '#') {
            const std::uint32_t c = n.size() > 1 && (n[1] == 'x' || n[1] == 'X') ? static_cast<std::uint32_t>(std::stoul(n.substr(2), nullptr, 16))
                                                                                  : static_cast<std::uint32_t>(std::stoul(n.substr(1)));
            utf8(r, c);
        } else {
            r += s.substr(i, e - i + 1);
        }
        i = e;
    }
    return r;
}

struct Leser {
    const std::string& s;
    std::size_t i = 0;
    bool ok = true;
    bool am(const char* t) const { return s.compare(i, std::char_traits<char>::length(t), t) == 0; }
    void leerraum() {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) { ++i; }
    }
    std::string name() {
        const std::size_t a = i;
        while (i < s.size() && s[i] != ' ' && s[i] != '\t' && s[i] != '\r' && s[i] != '\n' && s[i] != '>' && s[i] != '/' && s[i] != '=') { ++i; }
        return s.substr(a, i - a);
    }
    void element(K* eltern) {
        ++i;   // '<'
        auto e = neuesElement(name());
        while (true) {
            leerraum();
            if (i >= s.size()) {
                ok = false;
                return;
            }
            if (s[i] == '/' && i + 1 < s.size() && s[i + 1] == '>') {
                i += 2;
                kindDazu(eltern, std::move(e));
                return;
            }
            if (s[i] == '>') {
                ++i;
                break;
            }
            const std::string n = name();
            leerraum();
            if (i >= s.size() || s[i] != '=') {
                ok = false;
                return;
            }
            ++i;
            leerraum();
            const char q = s[i];
            if (q != '"' && q != '\'') {
                ok = false;
                return;
            }
            const std::size_t ende = s.find(q, i + 1);
            if (ende == std::string::npos) {
                ok = false;
                return;
            }
            e->attribute.emplace_back(n, entitaetenAufloesen(s.substr(i + 1, ende - i - 1)));
            i = ende + 1;
        }
        K* ek = e.get();
        e->eltern = eltern;
        eltern->kinder.push_back(std::move(e));
        inhaltLesen(ek);
    }
    void inhaltLesen(K* e) {
        while (ok && i < s.size()) {
            if (am("</")) {
                i = s.find('>', i);
                if (i == std::string::npos) {
                    ok = false;
                    return;
                }
                ++i;
                return;
            }
            if (am("<!--")) {
                const std::size_t ende = s.find("-->", i + 4);
                if (ende == std::string::npos) {
                    ok = false;
                    return;
                }
                auto c = std::make_unique<K>();
                c->art = K::Art::Kommentar;
                c->inhalt = s.substr(i + 4, ende - i - 4);
                c->eltern = e;
                e->kinder.push_back(std::move(c));
                i = ende + 3;
                continue;
            }
            if (am("<![CDATA[")) {
                const std::size_t ende = s.find("]]>", i + 9);
                if (ende == std::string::npos) {
                    ok = false;
                    return;
                }
                kindDazu(e, neuerText(s.substr(i + 9, ende - i - 9)));
                i = ende + 3;
                continue;
            }
            if (s[i] == '<') {
                element(e);
                continue;
            }
            const std::size_t ende = s.find('<', i);
            const std::string t = s.substr(i, ende == std::string::npos ? std::string::npos : ende - i);
            kindDazu(e, neuerText(entitaetenAufloesen(t)));
            i = ende == std::string::npos ? s.size() : ende;
        }
        if (e->art == K::Art::Element) { ok = false; }   // nicht geschlossen
    }
};

// --- Schreiben (xmlSaveFile, xmlSaveNoEmptyTags = 1) ---------------------------------------

void textMaskieren(std::string& o, const std::string& s) {   // xmlEscapeContent
    for (const char c : s) {
        switch (c) {
        case '<': o += "&lt;"; break;
        case '>': o += "&gt;"; break;
        case '&': o += "&amp;"; break;
        case '\r': o += "&#13;"; break;
        default: o += c; break;
        }
    }
}

void attributMaskieren(std::string& o, const std::string& s) {   // xmlBufAttrSerializeTxtContent
    for (const char c : s) {
        switch (c) {
        case '<': o += "&lt;"; break;
        case '>': o += "&gt;"; break;
        case '&': o += "&amp;"; break;
        case '"': o += "&quot;"; break;
        case '\n': o += "&#10;"; break;
        case '\r': o += "&#13;"; break;
        case '\t': o += "&#9;"; break;
        default: o += c; break;
        }
    }
}

void schreiben(std::string& o, const K* k) {
    switch (k->art) {
    case K::Art::Text: textMaskieren(o, k->inhalt); break;
    case K::Art::Kommentar: o += "<!--" + k->inhalt + "-->"; break;
    case K::Art::Element:
        o += '<' + k->name;
        for (const auto& [n, v] : k->attribute) {
            o += ' ' + n + "=\"";
            attributMaskieren(o, v);
            o += '"';
        }
        o += '>';
        for (const auto& c : k->kinder) { schreiben(o, c.get()); }
        o += "</" + k->name + '>';
        break;
    case K::Art::Dokument: break;
    }
}

}  // namespace

TexturTags::TexturTags() = default;
TexturTags::~TexturTags() = default;

bool TexturTags::neu(const std::string& speicherDatei) {
    if (!speicherDatei.empty()) { m_speicherDatei = speicherDatei; }
    m_wurzel = std::make_unique<Knoten>();
    m_wurzel->art = Knoten::Art::Dokument;
    m_wurzel->kodierung = "UTF-8";
    K* root = kindDazu(m_wurzel.get(), neuesElement("root"));
    kindDazu(root, neuerText("\n  "));
    K* stock = kindDazu(root, neuesElement("stock"));
    kindDazu(stock, neuerText("\n  "));
    kindDazu(root, neuerText("\n  "));
    K* custom = kindDazu(root, neuesElement("custom"));
    kindDazu(custom, neuerText("\n  "));
    kindDazu(root, neuerText("\n"));
    return true;
}

bool TexturTags::lesen(const std::string& text) {
    auto dok = std::make_unique<Knoten>();
    dok->art = Knoten::Art::Dokument;
    Leser l{text};
    if (l.am("\xEF\xBB\xBF")) { l.i = 3; }
    l.leerraum();
    if (l.am("<?xml")) {
        const std::size_t ende = text.find("?>", l.i);
        if (ende == std::string::npos) { return false; }
        const std::string erkl = text.substr(l.i, ende - l.i);
        const std::size_t k = erkl.find("encoding=");
        if (k != std::string::npos && k + 10 < erkl.size()) {
            const char q = erkl[k + 9];
            const std::size_t e = erkl.find(q, k + 10);
            if (e != std::string::npos) { dok->kodierung = erkl.substr(k + 10, e - k - 10); }
        }
        l.i = ende + 2;
    }
    while (l.ok && l.i < text.size()) {
        l.leerraum();
        if (l.i >= text.size()) { break; }
        if (l.am("<!--")) {
            const std::size_t ende = text.find("-->", l.i);
            if (ende == std::string::npos) { return false; }
            l.i = ende + 3;
        } else if (l.am("<!") || l.am("<?")) {
            const std::size_t ende = text.find('>', l.i);
            if (ende == std::string::npos) { return false; }
            l.i = ende + 1;
        } else if (text[l.i] == '<') {
            l.element(dok.get());
        } else {
            return false;
        }
    }
    if (!l.ok || wurzelElement(dok.get()) == nullptr) { return false; }
    m_wurzel = std::move(dok);
    return true;
}

bool TexturTags::oeffnen(const std::string& datei, const std::string& speicherDatei) {
    m_speicherDatei = speicherDatei.empty() ? datei : speicherDatei;
    std::ifstream f(std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(datei.data()), datei.size())), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream o;
    o << f.rdbuf();
    return lesen(o.str());
}

std::string TexturTags::text() const {
    std::string o;
    if (!m_wurzel) { return o; }
    o += "<?xml version=\"1.0\"";
    if (!m_wurzel->kodierung.empty()) { o += " encoding=\"" + m_wurzel->kodierung + "\""; }
    o += "?>\n";
    for (const auto& c : m_wurzel->kinder) {
        schreiben(o, c.get());
        o += '\n';
    }
    return o;
}

bool TexturTags::speichern() const { return speichern(m_speicherDatei); }

bool TexturTags::speichern(const std::string& datei) const {
    if (!m_wurzel || datei.empty()) { return false; }
    std::ofstream f(std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(datei.data()), datei.size())), std::ios::binary);
    if (!f) { return false; }
    f << text();
    return static_cast<bool>(f);
}

bool TexturTags::shaderDazu(const std::string& shader, TagTexturArt art, TagKnotenArt knoten) {
    K* root = m_wurzel ? wurzelElement(m_wurzel.get()) : nullptr;
    if (root == nullptr) { return false; }
    const char* gruppe = art == TagTexturArt::Stock ? "stock" : "custom";
    K* eltern = nullptr;
    for (const auto& c : root->kinder) {
        if (c->art == Knoten::Art::Element && c->name == gruppe) {
            eltern = c.get();
            break;
        }
    }
    if (eltern == nullptr) { return false; }
    auto neuK = neuesElement(knoten == TagKnotenArt::Shader ? "shader" : "texture");
    attributSetzen(neuK.get(), "path", shader);
    inhaltSetzen(neuK.get(), "\n    ");
    K* erster = ersterKind(eltern);
    if (erster == nullptr) {   // kein Kind: libxml2 haengte an nodeParent->children (NULL) - hier wie ein leerer Text
        kindDazu(eltern, neuerText(""));
        erster = ersterKind(eltern);
    }
    if (naechster(erster) == nullptr) {   // noch keine Shader
        kindDazu(erster, neuerText("  "));
        K* n = danach(erster, std::move(neuK));
        danach(n, neuerText("\n  "));
    } else {
        K* n = danach(erster, std::move(neuK));
        danach(n, neuerText("\n    "));
    }
    return true;
}

bool TexturTags::shaderWeg(const std::string& shader) {
    if (!m_wurzel) { return false; }
    for (K* s : shaderKnoten(m_wurzel.get())) {
        if (pfadGleich(s, shader)) {
            K* leer = voriger(s);
            entfernen(s);
            entfernen(leer);
            return true;
        }
    }
    return false;
}

bool TexturTags::hatShader(const std::string& shader) const {
    if (!m_wurzel) { return false; }
    const auto v = shaderKnoten(m_wurzel.get());
    return std::any_of(v.begin(), v.end(), [&](const K* s) { return pfadGleich(s, shader); });
}

bool TexturTags::hatTag(const std::string& shader, const std::string& tag) const {
    if (!m_wurzel) { return false; }
    const auto v = shaderKnoten(m_wurzel.get());
    return std::any_of(v.begin(), v.end(), [&](const K* s) { return pfadGleich(s, shader) && hatTagInhalt(s, tag); });
}

bool TexturTags::tagDazu(const std::string& shader, const std::string& tag) {
    if (!m_wurzel) { return false; }
    for (K* s : shaderKnoten(m_wurzel.get())) {
        if (!pfadGleich(s, shader)) { continue; }
        auto neuK = neuesElement("tag");
        inhaltSetzen(neuK.get(), tag);
        K* erster = ersterKind(s);
        if (erster == nullptr) {
            kindDazu(s, neuerText(""));
            erster = ersterKind(s);
        }
        if (naechster(erster) == nullptr) {   // der Shader hat keine Kinder
            kindDazu(erster, neuerText("  "));
            K* n = danach(erster, std::move(neuK));
            danach(n, neuerText("\n    "));
        } else {   // schon Kinder: der neue Knoten wird der erste
            K* n = danach(erster, std::move(neuK));
            danach(n, neuerText("\n      "));
        }
        return true;
    }
    return false;
}

int TexturTags::tagUmbenennen(const std::string& alt, const std::string& neu) {
    if (!m_wurzel) { return 0; }
    int anzahl = 0;
    for (K* s : shaderKnoten(m_wurzel.get())) {   // "/root/*/*[child::tag='alt']/*"
        if (!hatTagInhalt(s, alt)) { continue; }
        for (const auto& c : s->kinder) {
            if (c->art == Knoten::Art::Element && inhalt(c.get()) == alt) {
                inhaltSetzen(c.get(), neu);
                ++anzahl;
            }
        }
    }
    return anzahl;
}

bool TexturTags::tagWeg(const std::string& shader, const std::string& tag) {
    if (!m_wurzel) { return false; }
    for (K* s : shaderKnoten(m_wurzel.get())) {   // "/root/*/*[@path='X']/tag"
        if (!pfadGleich(s, shader)) { continue; }
        for (K* t : tagKinder(s)) {
            if (inhalt(t) == tag) {
                K* leer = voriger(t);
                entfernen(t);
                entfernen(leer);
                return true;
            }
        }
    }
    return false;
}

bool TexturTags::tagUeberallWeg(const std::string& tag) {
    std::set<std::string> liste;   // TagSearch( "/root/*/*[child::tag='T']" )
    for (K* s : shaderKnoten(m_wurzel.get())) {
        if (hatTagInhalt(s, tag)) {
            if (const std::string* p = attribut(s, "path")) { liste.insert(*p); }
        }
    }
    for (const auto& p : liste) { tagWeg(p, tag); }
    return true;
}

std::vector<std::string> TexturTags::shaderTags(const std::string& shader) const {
    std::vector<std::string> r;
    if (!m_wurzel) { return r; }
    for (const K* s : shaderKnoten(m_wurzel.get())) {
        if (!shader.empty() && !pfadGleich(s, shader)) { continue; }
        for (const K* t : tagKinder(s)) { r.push_back(inhalt(t)); }
    }
    return r;
}

std::set<std::string> TexturTags::ohneTags() const {
    std::set<std::string> r;
    if (!m_wurzel) { return r; }
    for (const K* s : shaderKnoten(m_wurzel.get())) {
        if (tagKinder(s).empty()) {
            const std::string* p = attribut(s, "path");
            r.insert(p != nullptr ? *p : std::string());
        }
    }
    return r;
}

std::set<std::string> TexturTags::alleTags() const {
    const auto v = shaderTags(std::string());
    return std::set<std::string>(v.begin(), v.end());
}

std::set<std::string> TexturTags::suchen(const std::vector<std::string>& tags) const {
    std::set<std::string> r;
    if (!m_wurzel) { return r; }
    for (const K* s : shaderKnoten(m_wurzel.get())) {
        if (std::all_of(tags.begin(), tags.end(), [&](const std::string& t) { return hatTagInhalt(s, t); })) {
            const std::string* p = attribut(s, "path");
            r.insert(p != nullptr ? *p : std::string());
        }
    }
    return r;
}

}  // namespace bhed::karte
