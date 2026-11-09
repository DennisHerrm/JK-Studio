// update.h - der Auto-Updater in der Oberflaeche (Netz, Installation, Fenster)
//
// Der pruefbare Kern steht in include/bhed/update.h. Hier: GitHub fragen
// (WinHTTP, im Hintergrund), das .zip laden und neben die .exe entpacken,
// neu starten - und was man davon sieht: ein Hinweis in der Statuszeile und
// ein kleines Fenster mit den Notizen des Releases.
#ifndef BHED_GUI_UPDATE_H
#define BHED_GUI_UPDATE_H

#include <cstdint>
#include <functional>
#include <string>

#include "bhed/update.h"

namespace bhed::gui::updater {

enum class Stand { Ruhe, Pruefe, Aktuell, Verfuegbar, Laedt, Fertig, Fehler };

struct Zustand {
    Stand stand = Stand::Ruhe;
    std::string meldung;          // bei Fehler: was schiefging
    bhed::update::Release release;
    double fortschritt = 0.0;     // 0..1 beim Laden
    // REST10 (wie G2C "x,x / y,y MB"): beim Laden so viele Bytes da, von so vielen
    // (Content-Length, sonst die Groesse aus der Release-Antwort; 0 = unbekannt).
    std::uint64_t geladen = 0;
    std::uint64_t gesamt = 0;
    int dateien = 0;              // so viele wurden ersetzt
    // REST9: der Fehler kam beim Installieren (nicht beim Nachsehen) - nur dann
    // zeigt die Leiste oben ihn mit "Erneut versuchen".
    bool beimInstallieren = false;
    // REST10: die Suche kam von Hand (Hilfe > Nach Updates suchen) - dann zeigt die
    // Leiste auch "Suche ...", "aktuell" und einen Fehler beim Nachsehen (wie G2C
    // st.manual). Die Suche beim Start bleibt dabei still.
    bool vonHand = false;
};

// Beim Programmstart: Reste eines frueheren Updates wegraeumen und - wenn
// eingeschaltet und kein Selbsttest laeuft - im Hintergrund nachsehen.
void beimStart();
// Nachsehen. `stumm` (beim Start): nur bei einem Fund die Leiste und der Hinweis in
// der Statuszeile. Von Hand (REST10): das Ergebnis steht in der Leiste oben - "Suche
// ...", "aktuell" oder der Fehler; das Fenster bleibt fuer "Was ist neu?".
void pruefen(bool stumm);
// Das gefundene Release laden und installieren (im Hintergrund).
void installieren();
// REST10 (wie G2C [Abbrechen]): das Laden abbrechen. Nichts wird eingespielt, das
// geladene Stueck verworfen, der Zustand geht zurueck auf Verfuegbar. Ein begonnener
// Tausch laeuft zu Ende (wie beimBeenden).
void abbrechen();
// Neu starten (fragt vorher nach ungespeicherten Aenderungen). Merkt den
// Neustart nur vor und beendet wie "Exit"; gestartet wird erst, wenn das
// Programm wirklich zugeht (neustartWennVorgemerkt aus main).
void neuStarten();
// Das Beenden wurde abgebrochen - dann auch kein Neustart.
void neustartVergessen();
// rc610 F78 (Fund B8-4): aus main, wenn das Fenster zu ist. Ein laufendes Laden
// oder Entpacken bricht ab (es ist noch nichts getauscht); ein begonnener Tausch
// laeuft zu Ende - bis rc609 endete der Prozess mittendrin, im schlimmsten Fall
// ohne jkstudio.exe.
void beimBeenden();
// Ganz am Ende von main: den vorgemerkten Neustart ausfuehren.
void neustartWennVorgemerkt();
// Eine Kopie des Zustands (der Hintergrundfaden schreibt ihn).
[[nodiscard]] Zustand zustand();
// Die Fassung, gegen die verglichen wird: kFassung, im Selbsttest
// ueberschreibbar mit BHED_UPDATE_LOKAL.
[[nodiscard]] std::string lokaleFassung();

// Einmal je Bild: das Fenster (wenn offen) und der Hinweis unten.
void zeichneFenster();
void zeichneStatus();
void oeffneFenster();
[[nodiscard]] bool fensterOffen();

// --- REST9 (shank 09.11., "wie bei G2C"): die Leiste oben ----------------
//
// Sobald ein neueres Release gefunden ist, steht direkt unter der Menueleiste
// des Hauptfensters, ueber allen Reitern, eine auffaellige Leiste: "Neues
// Update rcNNN verfuegbar (installiert rcMMM)" mit [Jetzt aktualisieren]
// [Was ist neu?] [Spaeter] [Diese Fassung ueberspringen] [x]; waehrend des
// Ladens ein Fortschrittsbalken, danach [Jetzt neu starten], bei einem Fehler
// [Erneut versuchen]. Spaeter und x blenden sie bis zum naechsten Start aus
// (der Hinweis in der Statuszeile bleibt); "ueberspringen" merkt die Fassung
// in den Einstellungen (updateUebersprungen) - eine neuere meldet sich wieder.
//
// Aus fensterKopf, direkt nach der Menueleiste. Rueckgabe: um so viel rueckt
// der Arbeitsbereich darunter nach unten (0 = keine Leiste in diesem Bild).
float zeichneLeiste();
// Wird die Leiste gerade gezeigt (Zustand, Spaeter, uebersprungene Fassung)?
[[nodiscard]] bool leisteSichtbar();
// Was die Leiste im letzten Bild gezeigt hat (zeilen 0 = nicht gezeichnet):
// ihr Rechteck in Bildschirmpunkten, wie viele Zeilen (2, wenn die Knoepfe nicht
// neben den Text passen) und den Text (vollstaendig, auch wenn er gekuerzt stand).
// REST10: das Symbol vor dem Text (Info, Haken, Fehler), seine Farbe und die der
// Leiste (fuer den Kontrast im Selbsttest) und der Text im Fortschrittsbalken.
enum class LeistenSymbol { Keins, Info, Haken, Fehler };
struct LeistenBild {
    float x0 = 0.0F, y0 = 0.0F, x1 = 0.0F, y1 = 0.0F;
    int zeilen = 0;
    bool gekuerzt = false;
    std::string text;
    LeistenSymbol symbol = LeistenSymbol::Keins;
    float sx0 = 0.0F, sy0 = 0.0F, sx1 = 0.0F, sy1 = 0.0F;   // Rechteck des Symbols
    unsigned symbolFarbe = 0;   // ImU32
    unsigned grundFarbe = 0;    // ImU32
    std::string balken;         // "x.x / y.y MB" beim Laden
};
[[nodiscard]] LeistenBild leisteBild();
// Ist ein Neustart vorgemerkt ("Jetzt neu starten" gedrueckt, Beenden laeuft)?
[[nodiscard]] bool neustartVorgemerkt();

// Nur fuer den Selbsttest: statt GitHub antwortet `netz` (Adresse -> HTTP-Status
// und Inhalt; false = keine Verbindung). Leer = das echte Netz.
using TestNetz = std::function<bool(const std::string& adresse, int& status, std::string& inhalt)>;
void testNetzSetzen(TestNetz netz);
// Nur fuer den Selbsttest: wie nach einem neuen Start - Zustand Ruhe, "Spaeter"
// (Leiste und Statuszeile) vergessen, das Fenster zu.
void testNeuerStart();
// Nur fuer den Selbsttest: so viel Breite hat die Leiste fuer Text und Knoepfe
// (ein schmales Fenster; 0 = die des Fensters).
void testLeistenBreite(float breite);
// Nur fuer den Selbsttest (REST10): so lange wartet die simulierte Antwort je Stueck
// (langsames Netz, Abbrechen mitten im Laden); 0 = ohne Pause.
void testStueckPause(int millisekunden);
// Nur fuer den Selbsttest (REST10): im Selbsttest oeffnet [Releases-Seite] keinen
// Browser; die Adresse steht hier.
[[nodiscard]] std::string testLetzteSeite();

// Aus main: "--nach-update=<pid>" - auf das Ende der alten Instanz warten,
// damit sie ihre Einstellungen fertig schreibt, bevor diese sie liest.
// true, wenn das Argument dieses war (dann ist es keine Datei zum Oeffnen).
bool warteAufVorgaenger(const std::string& argument);

}  // namespace bhed::gui::updater

#endif  // BHED_GUI_UPDATE_H
