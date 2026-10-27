// baufortschritt.cpp - Karten-Editor Stufe 8a Punkt 1: Kompilier-Fortschritt
// mit Restzeit (siehe baufortschritt.h).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114): das Format seiner Ausgabe -
// threads.cpp GetThreadWork (der Pacifier, 40 Schritte, Ziffer = Zehntel), die
// Phasenkoepfe von bsp.cpp, vis.cpp, light.cpp, light_bounce.cpp und model.cpp.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   Copyright (C) 2026 BehavEd-Remake contributors.
//
//   This program is free software; you can redistribute it and/or modify it
//   under the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. This program is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/baufortschritt.h"

#include "bhed/json.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace bhed::karte {

namespace {

// Die Woerter eines Befehls (Anfuehrungszeichen halten zusammen)
std::vector<std::string> woerter(const std::string& befehl) {
    std::vector<std::string> w;
    std::string cur;
    bool inQuote = false;
    bool hat = false;
    for (const char c : befehl) {
        if (c == '"') {
            inQuote = !inQuote;
            hat = true;
            continue;
        }
        if (!inQuote && std::isspace(static_cast<unsigned char>(c)) != 0) {
            if (hat) { w.push_back(cur); }
            cur.clear();
            hat = false;
            continue;
        }
        cur += c;
        hat = true;
    }
    if (hat) { w.push_back(cur); }
    return w;
}

std::string klein(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

}  // namespace

// main.cpp: nach den allgemeinen Schaltern (InitPaths nimmt -game, -fs_* ...)
// entscheidet der erste verbleibende Schalter; hier der erste aus der Liste.
BauStufe bauStufeAusBefehl(const std::string& befehl, std::string* schalter) {
    static const char* const kAndere[] = {"-fixaas", "-analyze", "-info", "-exportents", "-export", "-import", "-scale", "-shift",
                                          "-pk3",    "-repack",  "-convert", "-json",   "-mergebsp", "-minimap"};
    const std::vector<std::string> w = woerter(befehl);
    for (std::size_t i = 1; i < w.size(); ++i) {
        const std::string k = klein(w[i]);
        if (k == "-vis") { return BauStufe::Vis; }
        if (k == "-light") { return BauStufe::Licht; }
        for (const char* a : kAndere) {
            if (k == a) {
                if (schalter != nullptr) { *schalter = k.substr(1); }
                return BauStufe::Andere;
            }
        }
    }
    return BauStufe::Bsp;
}

std::string bauStufeName(const std::string& befehl) {
    std::string s;
    switch (bauStufeAusBefehl(befehl, &s)) {
    case BauStufe::Bsp:
        return "BSP";
    case BauStufe::Vis:
        return "VIS";
    case BauStufe::Licht:
        return "LIGHT";
    case BauStufe::Andere:
        break;
    }
    return s;
}

int bauBounceAusBefehl(const std::string& befehl) {
    const std::vector<std::string> w = woerter(befehl);
    for (std::size_t i = 1; i + 1 < w.size(); ++i) {
        if (klein(w[i]) == "-bounce") { return std::max(0, std::atoi(w[i + 1].c_str())); }
    }
    return 0;
}

bool bauPhasenKopf(const std::string& zeile, std::string* name) {
    std::string z = zeile;
    while (!z.empty() && (z.back() == '\r' || z.back() == ' ')) { z.pop_back(); }
    for (const char c : {'-', '#'}) {
        std::size_t a = 0;
        while (a < z.size() && z[a] == c) { ++a; }
        std::size_t b = z.size();
        while (b > a && z[b - 1] == c) { --b; }
        if (a < 3 || z.size() - b < 3 || b <= a + 2 || z[a] != ' ' || z[b - 1] != ' ') { continue; }
        if (name != nullptr) { *name = z.substr(a + 1, b - a - 2); }
        return true;
    }
    return false;
}

std::string bauPhasenGrundname(const std::string& name) {
    std::string n;
    int tiefe = 0;
    for (const char c : name) {
        if (c == '(') {
            ++tiefe;
        } else if (c == ')') {
            tiefe = std::max(0, tiefe - 1);
        } else if (tiefe == 0) {
            n += c;
        }
    }
    while (!n.empty() && (n.back() == ' ' || std::isdigit(static_cast<unsigned char>(n.back())) != 0)) { n.pop_back(); }
    while (!n.empty() && n.front() == ' ') { n.erase(n.begin()); }
    return n;
}

void BauSchrittBeobachter::zeileFertig(double t) {
    std::string name;
    if (bauPhasenKopf(m_zeile, &name)) {
        m_phase = bauPhasenGrundname(name);
        int n = 1;
        bool gefunden = false;
        for (auto& [k, z] : m_zaehler) {
            if (k == m_phase) {
                n = ++z;
                gefunden = true;
            }
        }
        if (!gefunden) { m_zaehler.emplace_back(m_phase, 1); }
        m_schluessel = m_phase + "#" + std::to_string(n);
        m_phasenStart = t;
        m_pacifier = -1.0;
        m_marken.push_back({m_schluessel, t});
        // light.cpp RadiosityTest/LightMain: "--- Radiosity (bounce %d of %d) ---"
        int x = 0;
        int y = 0;
        const std::size_t p = name.find("(bounce ");
        if (p != std::string::npos && std::sscanf(name.c_str() + p, "(bounce %d of %d)", &x, &y) == 2) {
            m_bounce = x;
            m_bounceVon = y;
        }
        m_zustand = Zustand::ErwartePacifier;
    } else if (m_zustand == Zustand::ErwartePacifier) {
        m_zustand = Zustand::Normal;   // die Zeile nach dem Kopf war keine Pacifier-Zeile
    }
    m_zeile.clear();
}

void BauSchrittBeobachter::text(const std::string& stueck, double t) {
    for (const char c : stueck) {
        if (m_zustand == Zustand::ErwartePacifier && m_zeile.empty() && c != '\n' && c != '\r') {
            // GetThreadWork: oldf -1 -> 0 schreibt "0"
            if (c == '0') {
                m_zustand = Zustand::Pacifier;
                m_zeichen = 1;
                m_pacifier = 0.0;
                m_zeile += c;
                continue;
            }
            m_zustand = Zustand::Normal;
        } else if (m_zustand == Zustand::Pacifier) {
            if (c == '.' || std::isdigit(static_cast<unsigned char>(c)) != 0) {
                ++m_zeichen;
                m_pacifier = std::min(1.0, static_cast<double>(m_zeichen - 1) / 40.0);
                m_zeile += c;
                continue;
            }
            // " (sekunden)\n": die Phase ist durch
            m_pacifier = 1.0;
            m_zustand = Zustand::Normal;
        }
        if (c == '\n') {
            zeileFertig(t);
        } else if (c != '\r') {
            m_zeile += c;
        }
    }
}

BauSchaetzung bauSchaetzen(const BauLaufZeiten* letzter, const std::vector<std::string>& befehle, std::size_t schritt,
                           const BauSchrittBeobachter& b, double t, const std::vector<double>& fertig) {
    BauSchaetzung s;
    double summeFertig = 0.0;
    for (const double d : fertig) { summeFertig += d; }
    const std::size_t n = befehle.size();
    // Gibt es den letzten Lauf mit denselben Stufen? (Gleiche Karte, gleicher Eintrag;
    // ein im Build-Menue geaenderter Eintrag mit anderen Stufen zaehlt nicht.)
    bool verlauf = letzter != nullptr && letzter->schritte.size() == n && n > 0;
    for (std::size_t i = 0; verlauf && i < n; ++i) {
        verlauf = bauStufeAusBefehl(letzter->schritte[i].befehl) == bauStufeAusBefehl(befehle[i]) && letzter->schritte[i].dauer >= 0.0;
    }
    if (verlauf && schritt < n) {
        const BauSchrittZeit& p = letzter->schritte[schritt];
        const double d = std::max(p.dauer, 1e-3);
        double pos = std::min(t, d);
        for (std::size_t k = 0; k < p.phasen.size(); ++k) {
            if (p.phasen[k].schluessel != b.schluessel()) { continue; }
            const double tk = p.phasen[k].start;
            const double tn = k + 1 < p.phasen.size() ? p.phasen[k + 1].start : d;
            const double len = std::max(0.0, tn - tk);
            const double in = b.pacifier() >= 0.0 ? b.pacifier() * len : std::min(std::max(0.0, t - b.phasenStart()), len);
            pos = std::min(d, tk + in);
            break;
        }
        // Tempo gegen den letzten Lauf (laeuft der Rechner gerade langsamer, ist es
        // auch der Rest); erst ab einer Sekunde, begrenzt
        double tempo = 1.0;
        if (pos > 1.0 && t > 1.0) { tempo = std::clamp(t / pos, 0.5, 2.0); }
        double rest = std::max(0.0, d - pos) * tempo;
        for (std::size_t i = schritt + 1; i < n; ++i) { rest += letzter->schritte[i].dauer * tempo; }
        s.ausVerlauf = true;
        s.schrittAnteil = std::clamp(pos / d, 0.0, 1.0);
        s.rest = rest;
        const double gesamt = summeFertig + t + rest;
        s.gesamtAnteil = gesamt > 0.0 ? std::clamp((summeFertig + t) / gesamt, 0.0, 1.0) : 0.0;
        return s;
    }
    // Erster Bau: nur die laufende Phase laesst sich hochrechnen (Untergrenze)
    const double f = b.pacifier();
    const double inPhase = std::max(0.0, t - b.phasenStart());
    if (f >= 0.025 && f < 1.0 && inPhase > 0.2) { s.rest = inPhase * (1.0 - f) / f; }
    // Gesamtbalken in ganzen Schritten; im Licht mit -bounce je Durchgang
    double imSchritt = 0.0;
    if (schritt < n) {
        const int bounces = bauBounceAusBefehl(befehle[schritt]);
        if (bauStufeAusBefehl(befehle[schritt]) == BauStufe::Licht && bounces > 0) {
            imSchritt = std::clamp(static_cast<double>(b.bounce()) / static_cast<double>(bounces + 1), 0.0, 1.0);
        }
    }
    s.gesamtAnteil = n > 0 ? std::clamp((static_cast<double>(std::min(schritt, n)) + imSchritt) / static_cast<double>(n), 0.0, 1.0) : 0.0;
    return s;
}

std::string BauZeitenSpeicher::schluessel(const std::string& karte, const std::string& eintrag) {
    std::string k = klein(karte);
    std::replace(k.begin(), k.end(), '\\', '/');
    return k + "\t" + eintrag;
}

bool BauZeitenSpeicher::lesen(const std::string& text) {
    m_laeufe.clear();
    json::Wert w;
    if (!json::lies(text, w) || w.art != json::Wert::Art::Objekt) { return false; }
    const json::Wert* l = w.feld("laeufe");
    if (l == nullptr || l->art != json::Wert::Art::Liste) { return false; }
    for (const json::Wert& x : l->liste) {
        if (x.art != json::Wert::Art::Objekt) { continue; }
        BauLaufZeiten lauf;
        if (const json::Wert* sch = x.feld("schritte"); sch != nullptr && sch->art == json::Wert::Art::Liste) {
            for (const json::Wert& s : sch->liste) {
                BauSchrittZeit z;
                z.befehl = s.textVon("befehl");
                z.dauer = s.zahlVon("dauer", -1.0);
                if (const json::Wert* ph = s.feld("phasen"); ph != nullptr && ph->art == json::Wert::Art::Liste) {
                    for (const json::Wert& m : ph->liste) {
                        if (m.art == json::Wert::Art::Liste && m.liste.size() == 2) { z.phasen.push_back({m.liste[0].text, m.liste[1].zahl}); }
                    }
                }
                lauf.schritte.push_back(std::move(z));
            }
        }
        if (m_laeufe.size() < kBauZeitenMax) { m_laeufe.emplace_back(x.textVon("schluessel"), std::move(lauf)); }
    }
    return true;
}

std::string BauZeitenSpeicher::schreiben() const {
    json::Wert w = json::Wert::alsObjekt();
    w.setze("fassung", json::Wert::alsZahl(1));
    json::Wert l = json::Wert::alsListe();
    for (const auto& [k, lauf] : m_laeufe) {
        json::Wert x = json::Wert::alsObjekt();
        x.setze("schluessel", json::Wert::alsText(k));
        json::Wert sch = json::Wert::alsListe();
        for (const BauSchrittZeit& z : lauf.schritte) {
            json::Wert s = json::Wert::alsObjekt();
            s.setze("befehl", json::Wert::alsText(z.befehl));
            s.setze("dauer", json::Wert::alsZahl(std::round(z.dauer * 1000.0) / 1000.0));
            json::Wert ph = json::Wert::alsListe();
            for (const BauPhasenMarke& m : z.phasen) {
                json::Wert paar = json::Wert::alsListe();
                paar.liste.push_back(json::Wert::alsText(m.schluessel));
                paar.liste.push_back(json::Wert::alsZahl(std::round(m.start * 1000.0) / 1000.0));
                ph.liste.push_back(std::move(paar));
            }
            s.setze("phasen", std::move(ph));
            sch.liste.push_back(std::move(s));
        }
        x.setze("schritte", std::move(sch));
        l.liste.push_back(std::move(x));
    }
    w.setze("laeufe", std::move(l));
    return json::schreibe(w);
}

const BauLaufZeiten* BauZeitenSpeicher::finden(const std::string& karte, const std::string& eintrag) const {
    const std::string k = schluessel(karte, eintrag);
    for (const auto& [s, lauf] : m_laeufe) {
        if (s == k) { return &lauf; }
    }
    return nullptr;
}

void BauZeitenSpeicher::setzen(const std::string& karte, const std::string& eintrag, BauLaufZeiten lauf) {
    const std::string k = schluessel(karte, eintrag);
    m_laeufe.erase(std::remove_if(m_laeufe.begin(), m_laeufe.end(), [&k](const auto& p) { return p.first == k; }), m_laeufe.end());
    m_laeufe.insert(m_laeufe.begin(), {k, std::move(lauf)});
    if (m_laeufe.size() > kBauZeitenMax) { m_laeufe.resize(kBauZeitenMax); }
}

std::vector<std::string> bauZeitenVergleich(const BauLaufZeiten& jetzt, const BauLaufZeiten* letzter, const char* fmtJetzt, const char* fmtVergleich) {
    std::vector<std::string> z;
    for (std::size_t i = 0; i < jetzt.schritte.size(); ++i) {
        const BauSchrittZeit& s = jetzt.schritte[i];
        char a[64];
        std::snprintf(a, sizeof(a), "%.1f", s.dauer);
        char zeile[256];
        std::snprintf(zeile, sizeof(zeile), fmtJetzt, static_cast<int>(i + 1), bauStufeName(s.befehl).c_str(), a);
        std::string t = zeile;
        if (letzter != nullptr && i < letzter->schritte.size() && letzter->schritte[i].dauer >= 0.0 &&
            bauStufeAusBefehl(letzter->schritte[i].befehl) == bauStufeAusBefehl(s.befehl)) {
            const double alt = letzter->schritte[i].dauer;
            char b[64];
            std::snprintf(b, sizeof(b), "%.1f", alt);
            char p[32];
            if (alt > 0.05) {
                std::snprintf(p, sizeof(p), "%+.0f %%", (s.dauer - alt) / alt * 100.0);
            } else {
                std::snprintf(p, sizeof(p), "%+.1f s", s.dauer - alt);
            }
            char v[256];
            std::snprintf(v, sizeof(v), fmtVergleich, b, p);
            t += v;
        }
        z.push_back(t);
    }
    return z;
}

std::string bauZeitText(double sekunden) {
    const long long s = std::max(0LL, static_cast<long long>(std::llround(sekunden)));
    char z[32];
    if (s >= 3600) {
        std::snprintf(z, sizeof(z), "%lld:%02lld:%02lld", s / 3600, (s / 60) % 60, s % 60);
    } else {
        std::snprintf(z, sizeof(z), "%lld:%02lld", s / 60, s % 60);
    }
    return z;
}

}  // namespace bhed::karte
