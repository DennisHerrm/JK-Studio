// baufortschritt.h - Karten-Editor Stufe 8a Punkt 1: Kompilier-Fortschritt
// mit Restzeit (eigene Funktion, nicht in NetRadiant-Custom).
//
// q3map2 meldet seinen Fortschritt nur als Text: jede Phase beginnt mit einer
// Kopfzeile "--- Name ---" (bzw. "----- Name -----", "####### model N
// #######"), und RunThreadsOnIndividual schreibt waehrend der Arbeit den
// "Pacifier" 0...1...2...3...4...5...6...7...8...9... - je Zeichen ein
// Vierzigstel (common/threads.cpp GetThreadWork: f = 40 * dispatch /
// workcount, je Schritt von oldf ein Zeichen, bei oldf % 4 == 0 die Ziffer
// f / 4), am Ende " (Sekunden)\n". -bounce schreibt "--- Radiosity (bounce
// x of y) ---". Dieser Teil liest genau das, Zeichen fuer Zeichen - im
// XML-Rueckkanal ebenso wie in der Pipe des Stapelbetriebs (der Text kommt
// in beliebigen Stuecken, auch mitten in einer Zeile).
//
// Die Restzeit kommt aus den Zeiten des letzten Baus derselben Karte mit
// demselben Build-Eintrag (je Schritt die Dauer und der Beginn jeder Phase);
// beim ersten Bau wird sie aus dem Fortschritt der laufenden Phase geschaetzt
// (dann als Untergrenze: spaetere Phasen und Schritte sind unbekannt).
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
#ifndef BHED_KARTE_BAUFORTSCHRITT_H
#define BHED_KARTE_BAUFORTSCHRITT_H

#include <cstddef>
#include <string>
#include <vector>

namespace bhed::karte {

// Die Stufe eines Befehls: -vis, -light, sonst BSP (-bsp ist q3map2s
// Vorgabe); andere Hauptschalter (-onlyents, -convert, -export ...) heissen
// nach ihrem Schalter.
enum class BauStufe { Bsp, Vis, Licht, Andere };
[[nodiscard]] BauStufe bauStufeAusBefehl(const std::string& befehl, std::string* schalter = nullptr);
// "BSP", "VIS", "LIGHT" bzw. der Schalter ohne '-'
[[nodiscard]] std::string bauStufeName(const std::string& befehl);
// Die Zahl hinter -bounce (0 ohne)
[[nodiscard]] int bauBounceAusBefehl(const std::string& befehl);

struct BauPhasenMarke {
    std::string schluessel;   // Name ohne Klammern, mit Zaehler: "IlluminateRawLightmap#2"
    double start = 0.0;       // Sekunden seit Beginn des Schritts
};
struct BauSchrittZeit {
    std::string befehl;
    double dauer = 0.0;
    std::vector<BauPhasenMarke> phasen;
};
struct BauLaufZeiten {
    std::vector<BauSchrittZeit> schritte;
};

// Liest die Ausgabe eines Schritts.
class BauSchrittBeobachter {
public:
    // Ein Stueck Text, t = Sekunden seit Beginn des Schritts.
    void text(const std::string& stueck, double t);

    [[nodiscard]] const std::string& phase() const { return m_phase; }            // "IlluminateRawLightmap" ("" vor der ersten)
    [[nodiscard]] const std::string& schluessel() const { return m_schluessel; }  // "IlluminateRawLightmap#2"
    [[nodiscard]] double phasenStart() const { return m_phasenStart; }
    // Anteil des Pacifiers der laufenden Phase (0..1), -1 ohne Pacifier
    [[nodiscard]] double pacifier() const { return m_pacifier; }
    [[nodiscard]] int bounce() const { return m_bounce; }          // laufender Bounce (0 = der erste Durchgang)
    [[nodiscard]] int bounceVon() const { return m_bounceVon; }    // y aus "bounce x of y"
    [[nodiscard]] const std::vector<BauPhasenMarke>& marken() const { return m_marken; }

private:
    void zeileFertig(double t);
    enum class Zustand { Normal, ErwartePacifier, Pacifier };
    Zustand m_zustand = Zustand::Normal;
    std::string m_zeile;
    std::string m_phase;
    std::string m_schluessel;
    double m_phasenStart = 0.0;
    double m_pacifier = -1.0;
    int m_zeichen = 0;
    int m_bounce = 0;
    int m_bounceVon = 0;
    std::vector<BauPhasenMarke> m_marken;
    std::vector<std::pair<std::string, int>> m_zaehler;
};

// Eine Kopfzeile von q3map2? name = der Text dazwischen ("Radiosity (bounce 1 of 2)").
[[nodiscard]] bool bauPhasenKopf(const std::string& zeile, std::string* name = nullptr);
// Name ohne Klammerteile und Zahlen am Ende: "BasePortalVis (20)" -> "BasePortalVis",
// "model 3" -> "model"
[[nodiscard]] std::string bauPhasenGrundname(const std::string& name);

struct BauSchaetzung {
    double schrittAnteil = -1.0;   // 0..1, -1 unbekannt
    double gesamtAnteil = 0.0;     // 0..1
    double rest = -1.0;            // Sekunden, -1 unbekannt
    bool ausVerlauf = false;       // aus dem letzten Bau (sonst aus dem Fortschritt geschaetzt, Untergrenze)
};

// Schaetzung waehrend des Schritts `schritt` von `befehle`.
//   letzter     die Zeiten des letzten Baus (nullptr: erster Bau)
//   fertig      die Dauern der schon fertigen Schritte dieses Laufs
//   t           Sekunden seit Beginn des laufenden Schritts
[[nodiscard]] BauSchaetzung bauSchaetzen(const BauLaufZeiten* letzter, const std::vector<std::string>& befehle, std::size_t schritt,
                                         const BauSchrittBeobachter& b, double t, const std::vector<double>& fertig);

// Die Zeiten aller Karten und Eintraege, als JSON in einer Datei
// (karte_bauzeiten.json im Einstellungsordner). Schluessel: Karte (Pfad, klein,
// '/') und Eintrag; hoechstens kBauZeitenMax Laeufe, der neueste vorn.
inline constexpr std::size_t kBauZeitenMax = 64;
class BauZeitenSpeicher {
public:
    bool lesen(const std::string& json);
    [[nodiscard]] std::string schreiben() const;
    [[nodiscard]] const BauLaufZeiten* finden(const std::string& karte, const std::string& eintrag) const;
    void setzen(const std::string& karte, const std::string& eintrag, BauLaufZeiten lauf);
    [[nodiscard]] std::size_t anzahl() const { return m_laeufe.size(); }
    [[nodiscard]] static std::string schluessel(const std::string& karte, const std::string& eintrag);

private:
    std::vector<std::pair<std::string, BauLaufZeiten>> m_laeufe;
};

// Je Schritt eine Zeile "<n> <STUFE>: <s> s" und, wenn es den letzten Lauf
// gibt, " (last build <s> s, <+-p> %)" - die Vorlage kommt aus der
// Uebersetzung: fmtJetzt ("%d %s: %s s"), fmtVergleich (" (last build %s s, %s)").
[[nodiscard]] std::vector<std::string> bauZeitenVergleich(const BauLaufZeiten& jetzt, const BauLaufZeiten* letzter, const char* fmtJetzt,
                                                          const char* fmtVergleich);

// Restzeit als Text "1:05" bzw. "0:07"; ab einer Stunde "1:02:03"
[[nodiscard]] std::string bauZeitText(double sekunden);

}  // namespace bhed::karte

#endif
