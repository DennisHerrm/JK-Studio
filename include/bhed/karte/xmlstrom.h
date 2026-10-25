// xmlstrom.h - Karten-Editor: ein XML-Leser, der Stueck fuer Stueck gefuettert
// wird (Stufe 3: Build-Menue und der Rueckkanal von q3map2).
//
// NRC liest das Build-Menue (radiant/build.cpp: XMLStreamParser) und den
// Rueckkanal von q3map2 (radiant/watchbsp.cpp: xmlCreatePushParserCtxt,
// xmlParseChunk) mit dem SAX-Leser von libxml2. Beide brauchen nur einen
// kleinen Teil davon: Elemente mit Attributen, Text mit den fuenf benannten
// und den numerischen Entities, Kommentare, die XML-Deklaration. Dieser Leser
// tut genau das und meldet wie SAX: Anfang (Name, Attribute), Zeichen, Ende.
// Text kann in mehreren Stuecken kommen - wie bei libxml2 (characters).
//
// Portiert aus NetRadiant-Custom 20260114: libs/xml/xmlparser.h
// (XMLSAXImporter: startElement, endElement, characters) - das Verhalten,
// nicht libxml2 selbst.
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
#ifndef BHED_KARTE_XMLSTROM_H
#define BHED_KARTE_XMLSTROM_H

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

using XmlAttribute = std::vector<std::pair<std::string, std::string>>;

class XmlStrom {
public:
    std::function<void(const std::string& name, const XmlAttribute& attribute)> anfang;   // startElement
    std::function<void(const std::string& name)> ende;                                     // endElement
    std::function<void(const std::string& text)> zeichen;                                  // characters
    std::function<void(const std::string& meldung)> fehler;                                // error/fatalError

    // xmlParseChunk( ctxt, daten, n, 0 )
    void einspeisen(const char* daten, std::size_t n);
    void einspeisen(const std::string& daten) { einspeisen(daten.data(), daten.size()); }
    // xmlParseChunk( ctxt, nullptr, 0, 1 ): was noch im Puffer liegt
    void schliessen();

    [[nodiscard]] int tiefe() const { return static_cast<int>(m_offen.size()); }
    [[nodiscard]] bool kaputt() const { return m_kaputt; }

private:
    bool verarbeiten(bool amEnde);
    void melde(const std::string& m);
    std::string m_puffer;
    std::vector<std::string> m_offen;   // offene Elemente
    bool m_kaputt = false;
};

// Die benannten (&lt; &gt; &amp; &quot; &apos;) und numerischen Entities
// (&#65; &#x41;) aufloesen; Unbekanntes bleibt stehen.
[[nodiscard]] std::string xmlEntitiesAufloesen(const std::string& text);
// Wie NRCs XMLEntityOutputStream::writeEscaped: < > " & werden zu Entities.
[[nodiscard]] std::string xmlMaskieren(const std::string& text);

}  // namespace bhed::karte

#endif
