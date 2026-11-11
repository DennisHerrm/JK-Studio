// baupause.h - Karten-Editor, vis2c: einen Bau anhalten und spaeter fortsetzen (eigene Funktion, nicht in
// NetRadiant-Custom).
//
// Beim Anhalten entsteht neben der gebauten Karte der Ordner <karte>.bhedpause mit allem, was der angehaltene Bau
// zum Weiterrechnen braucht: eine Kopie der Karte (.map bzw. .reg), die .bsp, .prt und .srf der fertigen Schritte,
// der VIS-Zwischenstand von bhed_q3map2 (-bhedvisstate, visflow.cpp), der Licht-Cache und stand.json (Eintrag,
// Befehle, Schritt). Fortsetzen geht auf zwei Wegen:
//   - "Karte wie angehalten": die uebrigen Befehle laufen auf den Kopien im Ordner, danach kommen die Ergebnisse
//     neben die Karte - genau der angehaltene Bau, zu Ende gefuehrt;
//   - "aktuelle Karte": der Eintrag wird fuer die Karte von jetzt neu gebaut, der VIS-Zwischenstand kommt vorher
//     zurueck neben die .bsp - bhed_q3map2 nimmt ihn nur bei derselben Portaldatei und denselben Schaltern, der
//     Licht-Cache prueft jede Lightmap selbst. Das Ergebnis ist das eines vollen Baus der aktuellen Karte.
// Ein Schritt, der sich nicht anhalten laesst (BSP, LIGHT, VIS ohne Zwischenstand), wird beendet und beginnt beim
// Fortsetzen neu; vom Licht bleiben mit -bhedlightcache die fertigen Durchgaenge (bauLichtCacheRettenDatei).
//
// Portiert aus q3map2 (NetRadiant-Custom 20260114): vis.cpp LoadPortals (portalbytes aus der Zahl der Portale) fuer die
// Hoechstgroesse des VIS-Zwischenstands. Alles andere ist eigener Code zu den BehavEd-Erweiterungen von bhed_q3map2
// (visflow.cpp BhedVisState: Zwischenstand und Stoppdatei; bhed_lichtcache.cpp: der Licht-Cache).
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
#ifndef BHED_KARTE_BAUPAUSE_H
#define BHED_KARTE_BAUPAUSE_H

#include "bhed/karte/baufortschritt.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bhed::karte {

struct BauPause {
    std::string eintrag;                 // der Eintrag des Build-Menues
    std::string karte;                   // die Karte des Editors
    std::string stamm;                   // die gebaute Datei ohne Endung ('/'): <stamm>.map, .bsp, .prt ...
    std::string mapDatei;                // was q3map2 baut: <stamm>.map bzw. <stamm>.reg
    std::vector<std::string> befehle;    // alle Schritte des Baus, wie sie gestartet wurden
    std::size_t schritt = 0;             // bei diesem Schritt geht es weiter
    std::string zeit;                    // wann angehalten wurde (Text fuer die Anzeige)
    double vergangen = 0.0;              // Sekunden Bauzeit bis dahin
    std::vector<double> fertig;          // die Dauern der fertigen Schritte
    int visFertig = -1;                  // Portale im VIS-Zwischenstand (-1: keiner)
    int visAlle = 0;
    int lichtPaesse = 0;                 // fertige Durchgaenge im Licht-Cache des angehaltenen Licht-Schritts
    int lichtPaesseVon = 0;
    std::uint64_t kartenHash = 0;        // FNV-1a der Karte beim Anhalten (bauDateiHash)
};

// <stamm>.bhedpause und <stamm>.bhedpause/<name> (der Stamm der Kopien)
[[nodiscard]] std::string bauPauseOrdner(const std::string& stamm);
[[nodiscard]] std::string bauPauseStamm(const std::string& stamm);
// stand.json als Text und zurueck
[[nodiscard]] std::string bauPauseText(const BauPause& p);
[[nodiscard]] bool bauPauseAusText(const std::string& json, BauPause& p);
// stand.json im Ordner des Stamms lesen bzw. (atomar) schreiben
[[nodiscard]] bool bauPauseLesen(const std::string& stamm, BauPause& p);
bool bauPauseSchreiben(const BauPause& p, std::string* fehler = nullptr);
// Bytes aller Dateien des Zwischenstands; -1 ohne Zwischenstand
[[nodiscard]] long long bauPauseGroesse(const std::string& stamm);
// FNV-1a 64 einer Datei (0: nicht lesbar)
[[nodiscard]] std::uint64_t bauDateiHash(const std::string& pfad);

// Anhalten: der Ordner entsteht neu (ein aelterer Zwischenstand derselben Karte geht weg). Kopiert werden die
// Karte, .bsp, .prt und .srf; liegt <stamm>.bhedprelight da (bauVorLichtSichern), ist das die .bsp. Der
// VIS-Zwischenstand wird verschoben (bis 2,9 GB - und ein spaeteres -vis -fast loeschte ihn sonst), der Licht-Cache
// verknuepft (fester Link, sonst kopiert), wenn noch ein Licht-Schritt mit -bhedlightcache kommt.
bool bauPauseSichern(const BauPause& p, std::string* fehler = nullptr);
// "Karte wie angehalten": alle Befehle auf die Kopien umgebogen (<stamm>. -> <ordner>/<name>.)
[[nodiscard]] std::vector<std::string> bauPauseBefehleAlt(const BauPause& p);
// ... und nach deren Ende: .bsp, .prt, .srf, Licht-Cache, Editor-Daten und der Ordner externer Lightmaps neben
// die Karte, der Zwischenstand weg
bool bauPauseAltFertig(const BauPause& p, std::string* fehler = nullptr);
// "aktuelle Karte": der VIS-Zwischenstand zurueck neben die .bsp (liegt dort kein neuerer), der Zwischenstand weg.
// true, wenn ein VIS-Zwischenstand zurueckgelegt wurde.
bool bauPauseAktuellVorbereiten(const BauPause& p);
bool bauPauseVerwerfen(const std::string& stamm);

// Vor einem Licht-Schritt: die .bsp daneben sichern (<stamm>.bhedprelight) - ein beendetes -light kann sie halb
// geschrieben hinterlassen (WriteBSPFile oeffnet "wb"), und der Schritt soll beim Fortsetzen mit derselben Eingabe
// neu beginnen. Nach dem Schritt weg; nach einem Abbruch zurueck ueber die .bsp.
bool bauVorLichtSichern(const std::string& stamm);
void bauVorLichtWeg(const std::string& stamm);
bool bauVorLichtZurueck(const std::string& stamm);

// Ein Licht-Schritt mit -bhedlightcache wurde beendet (der Prozess ist zu Ende): nach bauLichtCacheRetten aus
// <stamm>.bhedlight.neu den Cache <stamm>.bhedlight machen, sonst die .neu loeschen. true, wenn gerettet.
bool bauLichtCacheRettenDatei(const std::string& stamm, const BauLichtCache& c);

// -bhedvisstate fuer einen -vis-Schritt, dessen Programm bhed_q3map2 ist (ein fremdes q3map2 kennt den Schalter
// nicht) - vor die Karte, nicht doppelt
[[nodiscard]] std::string befehlMitVisStand(const std::string& befehl);
// Laesst sich dieser Schritt mit der Stoppdatei anhalten? (-vis mit -bhedvisstate)
[[nodiscard]] bool befehlVisAnhaltbar(const std::string& befehl);
// Die Stoppdatei des VIS-Schritts (vis.cpp: <karte>.bhedvisstop) und sein Zwischenstand
[[nodiscard]] std::string visStoppDatei(const std::string& stamm);
[[nodiscard]] std::string visStandDatei(const std::string& stamm);
// Wie gross der VIS-Zwischenstand hoechstens wird (visflow.cpp BhedStateWrite: Kopf, Schluessel, die .prt, je
// Portalseite 4 Bytes und portalbytes); waehrend des Sicherns liegt er kurz doppelt da (.tmp). 0: keine .prt
[[nodiscard]] long long visStandHoechstBytes(const std::string& prt);

}  // namespace bhed::karte

#endif
