// baumonitor.h - Karten-Editor: der Rueckkanal von q3map2 (Stufe 3 Punkt 4).
//
// q3map2 mit "-connect 127.0.0.1:39000" schickt jede Ausgabe als XML ueber
// eine Socket-Verbindung (<q3map_feedback version="1">, darin <message
// level="1|2|3">Text</message>, <polyline><point>x y z</point>...</polyline>
// fuer ein Leck, <select> mit <brush>Entity Pinsel</brush>, <pointmsg>,
// <windingmsg>). Dieser Teil liest den Strom genau wie NRCs SAX-Rueckrufe:
// Text mit seiner Stufe in die Konsole, ein Fehler (Stufe 3) beendet die
// Ueberwachung am Ende seines Elements (die naechsten Schritte laufen nicht
// mehr), die Leck-Linie wird sofort gezeigt (ausser "Stop Compilation on
// Leak" ist aus - dann geht es weiter), die geometrischen Meldungen kommen in
// das "Q3Map debug window".
//
// Portiert aus NetRadiant-Custom 20260114: radiant/watchbsp.cpp
// (saxStartElement, saxEndElement, saxCharacters, abortStream,
// MessageOutputStream), radiant/feedback.cpp (CSelectMsg, CPointMsg,
// CWindingMsg), radiant/points.cpp (CPointfile::saxStartElement/
// saxEndElement/saxCharacters, getName), radiant/xmlstuff.h
// (message_info_t).
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
#ifndef BHED_KARTE_BAUMONITOR_H
#define BHED_KARTE_BAUMONITOR_H

#include "bhed/karte/mathe.h"
#include "bhed/karte/xmlstrom.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace bhed::karte {

inline constexpr const char* kQ3mapStromFassung = "1";   // Q3MAP_STREAM_VERSION
inline constexpr int kBauStufeStd = 1;   // SYS_STD
inline constexpr int kBauStufeWrn = 2;   // SYS_WRN
inline constexpr int kBauStufeErr = 3;   // SYS_ERR

// Ein Eintrag des "Q3Map debug window" (ISAXHandler in g_DbgDlg).
struct BauRueckmeldung {
    enum class Art { Auswahl, Punkt, Wicklung, Leck };   // CSelectMsg, CPointMsg, CWindingMsg, CPointfile
    Art art = Art::Auswahl;
    std::string name;            // getName(): der Text der Meldung, beim Leck "Map leaked"
    int entitaet = -1;           // CSelectMsg: "%i %i" aus <brush>
    int pinsel = -1;
    Vektor3f punkt{0, 0, 0};     // CPointMsg
    std::vector<Vektor3f> punkte;   // CWindingMsg (wt), CPointfile (die Leck-Linie)
};

// Die Leck-Linie (.lin) lesen wie Pointfile_Parse: je Zeile "x y z", auch der
// zhlt-Stil "x y z - x y z"; meldung bekommt "Corrupt point file, line N".
[[nodiscard]] std::vector<Vektor3f> punktdateiLesen(const std::string& text, std::string* meldung = nullptr);

// Die Nachrichten von l_net (libs/l_net/l_net.c Net_Send/Net_Receive): je
// Nachricht 4 Byte Laenge (little endian, ohne die 4) und der Inhalt;
// NMSG_ReadString liest den Inhalt bis zur ersten 0.
class BauNetzRahmen {
public:
    void einspeisen(const char* daten, std::size_t n) { m_puffer.append(daten, n); }
    // Die naechste vollstaendige Nachricht; false, wenn noch keine da ist.
    // fehler: eine ungueltige Laenge (Net_Receive: "invalid message size").
    bool naechste(std::string& inhalt, bool* fehler = nullptr);

private:
    std::string m_puffer;
};

class BauMonitor {
public:
    BauMonitor();
    bool leckStopp = true;   // g_WatchBSP_LeakStop
    std::function<void(int stufe, const std::string& text)> drucken;   // Sys_Print( msg_level, ... )
    std::function<void(const std::string& zeile)> warnung;             // globalWarningStream() << ... << '\n'
    std::function<void(const std::string& zeile)> fehler;              // globalErrorStream()
    std::function<void(const BauRueckmeldung&)> rueckmeldung;          // g_DbgDlg.Push
    std::function<void(const std::vector<Vektor3f>&)> leckLinie;       // CPointfile: Linie fertig (gezeigt)

    // Eine neue Verbindung: message_info_t auf 0, neuer XML-Leser (m_bNeedCtxtInit).
    void verbunden();
    // Ein empfangenes Stueck (xmlParseChunk).
    void einspeisen(const std::string& stueck);
    // EndMonitoringLoop wurde ausgeloest (Fehler, abortStream).
    [[nodiscard]] bool beenden() const { return m_beenden; }
    [[nodiscard]] bool leck() const { return m_leck; }       // eine Leck-Linie kam
    [[nodiscard]] bool fehlerStufe() const { return m_fehlerStufe; }   // Text der Stufe 3 kam

private:
    void anfang(const std::string& name, const XmlAttribute& attribute);
    void ende(const std::string& name);
    void zeichen(const std::string& text);
    void abbrechen();   // abortStream

    std::unique_ptr<XmlStrom> m_xml;
    // message_info_t
    int m_stufe = 0;              // msg_level
    int m_tiefe = 0;              // recurse
    int m_ignorieren = 0;         // ignore_depth
    int m_stopp = 0;              // stop_depth
    int m_geoTiefe = 0;           // geometry_depth
    bool m_geo = false;           // pGeometry != 0
    BauRueckmeldung m_eintrag;    // der Handler in Arbeit
    int m_zustand = 0;            // 0 Meldung lesen, 1 Inhalt (brush/point/winding)
    std::string m_text;           // message (Text der Meldung)
    std::string m_inhalt;         // brush / point / winding / m_characters
    bool m_beenden = false;
    bool m_leck = false;
    bool m_fehlerStufe = false;
};

}  // namespace bhed::karte

#endif
