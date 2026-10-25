// baumonitor.cpp - Karten-Editor: der Rueckkanal von q3map2 (Stufe 3 Punkt 4).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/watchbsp.cpp,
// radiant/feedback.cpp, radiant/points.cpp (siehe baumonitor.h).
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
#include "bhed/karte/baumonitor.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace bhed::karte {

namespace {

// sscanf( text, "%f %f %f", ... ): fehlende Zahlen bleiben 0
Vektor3f dreiZahlen(const std::string& text, int* gelesen = nullptr) {
    float v[3] = {0.0F, 0.0F, 0.0F};
    const int n = std::sscanf(text.c_str(), "%f %f %f", &v[0], &v[1], &v[2]);
    if (gelesen != nullptr) { *gelesen = n; }
    return Vektor3f(v[0], v[1], v[2]);
}

}  // namespace

std::vector<Vektor3f> punktdateiLesen(const std::string& text, std::string* meldung) {
    // Pointfile_Parse (points.cpp:254-305)
    std::vector<Vektor3f> punkte;
    const char* data = text.c_str();
    int zeile = 1;
    while (*data != '\0') {
        float v[3] = {0.0F, 0.0F, 0.0F};
        if (std::sscanf(data, "%f %f %f", &v[0], &v[1], &v[2]) != 3) {
            if (meldung != nullptr) { *meldung = "Corrupt point file, line " + std::to_string(zeile); }
            break;
        }
        while (*data != '\0' && *data != '\n') {
            if (data != text.c_str() && *(data - 1) == ' ' && *data == '-' && *(data + 1) == ' ') { break; }
            ++data;
        }
        // deal with zhlt style point files.
        if (*data == '-') {
            if (std::sscanf(data, "- %f %f %f", &v[0], &v[1], &v[2]) != 3) {
                if (meldung != nullptr) { *meldung = "Corrupt point file, line " + std::to_string(zeile); }
                break;
            }
            while (*data != '\0' && *data != '\n') { ++data; }
        }
        while (*data == '\n') {
            ++data;   // skip the \n
            ++zeile;
        }
        if (punkte.size() < 8192) { punkte.emplace_back(v[0], v[1], v[2]); }   // s_pointvecs[8192]
    }
    return punkte;
}

bool BauNetzRahmen::naechste(std::string& inhalt, bool* fehler) {
    if (fehler != nullptr) { *fehler = false; }
    if (m_puffer.size() < 4) { return false; }
    const auto b = [this](std::size_t i) { return static_cast<unsigned int>(static_cast<unsigned char>(m_puffer[i])); };
    const unsigned int laenge = b(0) | (b(1) << 8) | (b(2) << 16) | (b(3) << 24);
    if (laenge > 1024) {   // remaining > MAX_NETMESSAGE
        if (fehler != nullptr) { *fehler = true; }
        m_puffer.clear();
        return false;
    }
    if (m_puffer.size() < 4 + static_cast<std::size_t>(laenge)) { return false; }
    inhalt = m_puffer.substr(4, laenge);
    m_puffer.erase(0, 4 + static_cast<std::size_t>(laenge));
    const std::size_t null = inhalt.find('\0');
    if (null != std::string::npos) { inhalt.resize(null); }   // NMSG_ReadString
    return true;
}

BauMonitor::BauMonitor() { verbunden(); }

void BauMonitor::verbunden() {
    // memset( &m_message_info, 0, sizeof( message_info_t ) ); m_bNeedCtxtInit = true
    m_stufe = 0;
    m_tiefe = 0;
    m_ignorieren = 0;
    m_stopp = 0;
    m_geoTiefe = 0;
    m_geo = false;
    m_beenden = false;
    m_xml = std::make_unique<XmlStrom>();
    m_xml->anfang = [this](const std::string& n, const XmlAttribute& a) { anfang(n, a); };
    m_xml->ende = [this](const std::string& n) { ende(n); };
    m_xml->zeichen = [this](const std::string& t) { zeichen(t); };
    m_xml->fehler = [this](const std::string& f) {
        if (fehler) { fehler("XML error: " + f); }   // saxError
    };
}

void BauMonitor::einspeisen(const std::string& stueck) {
    if (m_xml) { m_xml->einspeisen(stueck); }
}

void BauMonitor::abbrechen() {   // abortStream
    m_beenden = true;
    m_ignorieren = -1;
    ++m_tiefe;
}

void BauMonitor::anfang(const std::string& name, const XmlAttribute& attribute) {
    if (m_ignorieren == 0) {
        if (m_geo) {   // data->pGeometry->saxStartElement
            if (m_eintrag.art == BauRueckmeldung::Art::Leck) {
                if (name == "polyline") {   // CPointfile::saxStartElement: Init()
                    m_eintrag.punkte.clear();
                    if (!leckStopp) { m_stopp = 0; }   // there's a prefs setting to avoid stopping on leak
                }
            } else {
                const bool kopf = (m_eintrag.art == BauRueckmeldung::Art::Auswahl && name == "select") ||
                                  (m_eintrag.art == BauRueckmeldung::Art::Punkt && name == "pointmsg") ||
                                  (m_eintrag.art == BauRueckmeldung::Art::Wicklung && name == "windingmsg");
                if (kopf) {
                    m_zustand = 0;   // SELECT_MESSAGE / POINT_MESSAGE / WINDING_MESSAGE
                } else {
                    m_zustand = 1;   // SELECT_BRUSH / POINT_POINT / WINDING_WINDING
                    if (warnung) { warnung(m_text); }   // globalWarningStream() << message << '\n'
                }
            }
        } else if (name == "q3map_feedback") {
            // check the correct version
            if (attribute.empty() || attribute.front().first != "version") {
                if (fehler) {
                    fehler("No stream version given in the feedback stream, this is an old q3map version.\n"
                           "Please turn off monitored compiling if you still wish to use this q3map executable");
                }
                abbrechen();
                return;
            }
            if (attribute.front().second != kQ3mapStromFassung) {
                if (fehler) {
                    fehler(std::string("This version of Radiant reads version ") + kQ3mapStromFassung +
                           " debug streams, I got an incoming connection with version " + attribute.front().second +
                           "\nPlease make sure your versions of Radiant and q3map are matching.");
                }
                abbrechen();
                return;
            }
        } else if (name == "message") {
            m_stufe = attribute.empty() ? 0 : std::atoi(attribute.front().second.c_str());
        } else if (name == "polyline") {
            // polyline has a particular status .. right now we only use it for leakfile ..
            m_geoTiefe = m_tiefe;
            m_geo = true;
            m_eintrag = BauRueckmeldung{};
            m_eintrag.art = BauRueckmeldung::Art::Leck;
            m_eintrag.name = "Map leaked";
            m_inhalt.clear();
            // pGeometry->saxStartElement: CPointfile::saxStartElement( "polyline" ) - Init()
            m_eintrag.punkte.clear();
            if (!leckStopp) { m_stopp = 0; }   // there's a prefs setting to avoid stopping on leak
        } else if (name == "select" || name == "pointmsg" || name == "windingmsg") {
            m_geoTiefe = m_tiefe;
            m_geo = true;
            m_eintrag = BauRueckmeldung{};
            m_eintrag.art = name == "select" ? BauRueckmeldung::Art::Auswahl
                            : name == "pointmsg" ? BauRueckmeldung::Art::Punkt : BauRueckmeldung::Art::Wicklung;
            m_zustand = 0;
            m_text.clear();
            m_inhalt.clear();
        } else {
            if (warnung) { warnung("Warning: ignoring unrecognized node in XML stream (" + name + ")"); }
            // we don't recognize this node, jump over it
            m_ignorieren = m_tiefe;
        }
    }
    ++m_tiefe;
}

void BauMonitor::ende(const std::string& name) {
    --m_tiefe;
    // we are out of an ignored chunk
    if (m_tiefe == m_ignorieren) {
        m_ignorieren = 0;
        return;
    }
    if (m_geo) {
        // data->pGeometry->saxEndElement
        if (m_eintrag.art == BauRueckmeldung::Art::Leck) {
            if (name == "polyline") {   // we are done: GenerateDisplayList, s_check_point = begin()
                m_leck = true;
                if (leckLinie) { leckLinie(m_eintrag.punkte); }
            } else if (name == "point") {
                if (m_eintrag.punkte.size() < 8192) { m_eintrag.punkte.push_back(dreiZahlen(m_inhalt)); }
                m_inhalt.clear();
            }
        } else if (m_eintrag.art == BauRueckmeldung::Art::Auswahl) {
            if (name == "select") {   // Highlight: sscanf( brush, "%i %i", ... )
                int en = -1;
                int br = -1;
                if (std::sscanf(m_inhalt.c_str(), "%i %i", &en, &br) == 2) {
                    m_eintrag.entitaet = en;
                    m_eintrag.pinsel = br;
                }
            }
        } else if (m_eintrag.art == BauRueckmeldung::Art::Punkt) {
            if (name == "point") {
                m_eintrag.punkt = dreiZahlen(m_inhalt);
                m_inhalt.clear();
            }
        } else if (name == "winding") {   // CWindingMsg::saxEndElement
            const char* c = m_inhalt.c_str();
            int anzahl = 0;
            (void)std::sscanf(c, "%i ", &anzahl);
            m_eintrag.punkte.clear();
            for (int i = 0; i < anzahl; ++i) {
                c = std::strchr(c + 1, '(');
                if (c == nullptr) { break; }   // don't trust numpoints too much
                float p[3] = {0.0F, 0.0F, 0.0F};
                (void)std::sscanf(c, "(%g %g %g)", &p[0], &p[1], &p[2]);
                m_eintrag.punkte.emplace_back(p[0], p[1], p[2]);
            }
        }
        // we add the object to the debug window
        if (m_geoTiefe == m_tiefe) {
            if (m_eintrag.art != BauRueckmeldung::Art::Leck) { m_eintrag.name = m_text; }
            if (rueckmeldung) { rueckmeldung(m_eintrag); }
            m_geo = false;
        }
    }
    if (m_tiefe == m_stopp) {   // Received error msg .. shutting down..
        m_beenden = true;
        return;
    }
}

void BauMonitor::zeichen(const std::string& text) {
    if (m_geo) {   // data->pGeometry->saxCharacters
        if (m_eintrag.art == BauRueckmeldung::Art::Leck || m_zustand == 1) {
            m_inhalt += text;
        } else {
            m_text += text;
        }
        return;
    }
    if (m_ignorieren == 0) {
        // output the message using the level
        if (drucken) { drucken(m_stufe, text); }
        // if this message has error level flag, we mark the depth to stop the compilation when we get out
        if (m_stufe == kBauStufeErr) {
            m_stopp = m_tiefe - 1;
            m_fehlerStufe = true;
        }
    }
}

}  // namespace bhed::karte
