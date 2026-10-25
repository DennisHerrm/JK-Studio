// xmlstrom.cpp - Karten-Editor: XML Stueck fuer Stueck lesen (Stufe 3).
//
// Portiert aus NetRadiant-Custom 20260114: libs/xml/xmlparser.h (wie
// XMLSAXImporter die SAX-Ereignisse von libxml2 weitergibt). Das Lesen selbst
// folgt der XML-Empfehlung so weit, wie Build-Menue und q3map2-Rueckkanal es
// brauchen: Zeilenenden CR LF -> LF (2.11), Attributwerte mit Leerraum ->
// Leerzeichen (3.3.3), Text nur innerhalb des Wurzelelements.
//
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/xmlstrom.h"

#include <cstdlib>

namespace bhed::karte {

namespace {

void utf8Anhaengen(std::string& aus, unsigned long c) {
    if (c < 0x80) {
        aus += static_cast<char>(c);
    } else if (c < 0x800) {
        aus += static_cast<char>(0xC0 | (c >> 6));
        aus += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        aus += static_cast<char>(0xE0 | (c >> 12));
        aus += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        aus += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        aus += static_cast<char>(0xF0 | (c >> 18));
        aus += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        aus += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        aus += static_cast<char>(0x80 | (c & 0x3F));
    }
}

bool leer(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
bool namensZeichen(char c) { return !leer(c) && c != '/' && c != '>' && c != '=' && c != '<' && c != '"' && c != '\''; }

// Das '>', das den Tag schliesst (Anfuehrungszeichen beachten), oder npos.
std::size_t tagEnde(const std::string& s, std::size_t von) {
    char in = 0;
    for (std::size_t i = von; i < s.size(); ++i) {
        const char c = s[i];
        if (in != 0) {
            if (c == in) { in = 0; }
        } else if (c == '"' || c == '\'') {
            in = c;
        } else if (c == '>') {
            return i;
        }
    }
    return std::string::npos;
}

}  // namespace

std::string xmlEntitiesAufloesen(const std::string& text) {
    std::string aus;
    aus.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '&') {
            aus += text[i];
            continue;
        }
        const std::size_t e = text.find(';', i + 1);
        if (e == std::string::npos || e - i > 12) {
            aus += text[i];
            continue;
        }
        const std::string n = text.substr(i + 1, e - i - 1);
        if (n == "lt") { aus += '<'; }
        else if (n == "gt") { aus += '>'; }
        else if (n == "amp") { aus += '&'; }
        else if (n == "quot") { aus += '"'; }
        else if (n == "apos") { aus += '\''; }
        else if (n.size() > 1 && n[0] == '#') {
            char* rest = nullptr;
            const bool hex = n[1] == 'x' || n[1] == 'X';
            const unsigned long c = std::strtoul(n.c_str() + (hex ? 2 : 1), &rest, hex ? 16 : 10);
            if (rest == nullptr || *rest != '\0' || c == 0 || c > 0x10FFFF) {
                aus += text[i];
                continue;
            }
            utf8Anhaengen(aus, c);
        } else {
            aus += text[i];
            continue;
        }
        i = e;
    }
    return aus;
}

std::string xmlMaskieren(const std::string& text) {
    std::string aus;
    aus.reserve(text.size());
    for (const char c : text) {
        switch (c) {
        case '<': aus += "&lt;"; break;
        case '>': aus += "&gt;"; break;
        case '"': aus += "&quot;"; break;
        case '&': aus += "&amp;"; break;
        default: aus += c; break;
        }
    }
    return aus;
}

void XmlStrom::melde(const std::string& m) {
    m_kaputt = true;
    if (fehler) { fehler(m); }
}

void XmlStrom::einspeisen(const char* daten, std::size_t n) {
    if (m_kaputt || n == 0) { return; }
    // Zeilenenden wie XML 2.11: CR LF und CR allein -> LF. Ein CR am Ende des
    // Stuecks bleibt, bis das naechste Stueck zeigt, ob ein LF folgt.
    for (std::size_t i = 0; i < n; ++i) {
        const char c = daten[i];
        if (!m_puffer.empty() && m_puffer.back() == '\r') {
            m_puffer.back() = '\n';
            if (c == '\n') { continue; }
        }
        m_puffer += c;
    }
    verarbeiten(false);
}

void XmlStrom::schliessen() {
    if (m_kaputt) { return; }
    if (!m_puffer.empty() && m_puffer.back() == '\r') { m_puffer.back() = '\n'; }
    verarbeiten(true);
}

bool XmlStrom::verarbeiten(bool amEnde) {
    std::size_t p = 0;
    const auto text = [&](std::size_t bis) {
        if (bis > p && !m_offen.empty() && zeichen) {
            const std::string t = m_puffer.substr(p, bis - p);
            zeichen(xmlEntitiesAufloesen(t));
        }
        p = bis;
    };
    while (p < m_puffer.size() && !m_kaputt) {
        if (m_puffer[p] != '<') {
            const std::size_t lt = m_puffer.find('<', p);
            if (lt != std::string::npos) {
                text(lt);
                continue;
            }
            // Ohne folgendes '<': alles weitergeben, nur eine angefangene Entity
            // wartet auf das naechste Stueck.
            std::size_t bis = m_puffer.size();
            if (!amEnde) {
                const std::size_t amp = m_puffer.rfind('&');
                if (amp != std::string::npos && amp >= p && m_puffer.find(';', amp) == std::string::npos) { bis = amp; }
                // ein CR am Ende wartet: folgt ein LF, werden beide zu einem LF
                if (bis == m_puffer.size() && bis > p && m_puffer.back() == '\r') { --bis; }
            }
            text(bis);
            break;
        }
        // '<' ...
        const auto beginnt = [&](const char* s) { return m_puffer.compare(p, std::char_traits<char>::length(s), s) == 0; };
        const auto kurz = [&](std::size_t n) { return m_puffer.size() - p < n; };
        if (kurz(2)) { break; }
        if (beginnt("<!--")) {
            const std::size_t e = m_puffer.find("-->", p + 4);
            if (e == std::string::npos) { break; }
            p = e + 3;
            continue;
        }
        if (kurz(4) && std::string("<!--").compare(0, m_puffer.size() - p, m_puffer, p, m_puffer.size() - p) == 0) { break; }
        if (beginnt("<![CDATA[")) {
            const std::size_t e = m_puffer.find("]]>", p + 9);
            if (e == std::string::npos) { break; }
            if (!m_offen.empty() && zeichen && e > p + 9) { zeichen(m_puffer.substr(p + 9, e - p - 9)); }
            p = e + 3;
            continue;
        }
        if (beginnt("<?")) {
            const std::size_t e = m_puffer.find("?>", p + 2);
            if (e == std::string::npos) { break; }
            p = e + 2;
            continue;
        }
        if (beginnt("<!")) {
            const std::size_t e = tagEnde(m_puffer, p + 2);
            if (e == std::string::npos) { break; }
            p = e + 1;
            continue;
        }
        const std::size_t e = tagEnde(m_puffer, p + 1);
        if (e == std::string::npos) { break; }
        if (m_puffer[p + 1] == '/') {   // </name>
            std::size_t a = p + 2;
            std::size_t b = a;
            while (b < e && namensZeichen(m_puffer[b])) { ++b; }
            const std::string name = m_puffer.substr(a, b - a);
            if (m_offen.empty() || m_offen.back() != name) {
                melde("Opening and ending tag mismatch: " + (m_offen.empty() ? std::string() : m_offen.back()) + " and " + name);
                return false;
            }
            m_offen.pop_back();
            p = e + 1;
            if (ende) { ende(name); }
            continue;
        }
        // <name attr="wert" ...> oder <name .../>
        std::size_t i = p + 1;
        std::size_t b = i;
        while (b < e && namensZeichen(m_puffer[b])) { ++b; }
        if (b == i) {
            melde("StartTag: invalid element name");
            return false;
        }
        const std::string name = m_puffer.substr(i, b - i);
        XmlAttribute attribute;
        bool selbst = false;
        i = b;
        while (i < e) {
            while (i < e && leer(m_puffer[i])) { ++i; }
            if (i >= e) { break; }
            if (m_puffer[i] == '/') {
                selbst = true;
                ++i;
                continue;
            }
            std::size_t nb = i;
            while (nb < e && namensZeichen(m_puffer[nb])) { ++nb; }
            if (nb == i) {
                melde("attributes construct error");
                return false;
            }
            const std::string an = m_puffer.substr(i, nb - i);
            i = nb;
            while (i < e && leer(m_puffer[i])) { ++i; }
            if (i >= e || m_puffer[i] != '=') {
                melde("Specification mandates value for attribute " + an);
                return false;
            }
            ++i;
            while (i < e && leer(m_puffer[i])) { ++i; }
            if (i >= e || (m_puffer[i] != '"' && m_puffer[i] != '\'')) {
                melde("AttValue: \" or ' expected");
                return false;
            }
            const char q = m_puffer[i];
            const std::size_t w = m_puffer.find(q, i + 1);
            std::string wert = m_puffer.substr(i + 1, w - i - 1);
            for (char& c : wert) {
                if (c == '\t' || c == '\n' || c == '\r') { c = ' '; }   // 3.3.3
            }
            attribute.emplace_back(an, xmlEntitiesAufloesen(wert));
            i = w + 1;
        }
        p = e + 1;
        m_offen.push_back(name);
        if (anfang) { anfang(name, attribute); }
        if (selbst) {
            m_offen.pop_back();
            if (ende) { ende(name); }
        }
    }
    m_puffer.erase(0, p);
    return true;
}

}  // namespace bhed::karte
