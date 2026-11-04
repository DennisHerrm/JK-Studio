// platform.h - was nur Windows kann
//
// Die Oberflaeche kennt keine Windows-Aufrufe. Das ist kein Selbstzweck:
// app.cpp laesst sich dadurch auch unter Linux uebersetzen, und genau das
// prueft tools/check_gui.sh bei jedem Durchlauf.
#ifndef BHED_PLATFORM_H
#define BHED_PLATFORM_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace bhed::platform {

// Leerer Rueckgabewert heisst abgebrochen.
[[nodiscard]] std::string openFileDialog(const char* title, const char* filter,
                                         const std::string& startDir);
[[nodiscard]] std::string saveFileDialog(const char* title, const char* filter,
                                         const std::string& startDir,
                                         const std::string& suggestedName);

enum class Answer { Yes, No, Cancel };
[[nodiscard]] Answer askSaveChanges(const std::string& question, const char* title);
void showError(const std::string& message, const char* title);

// Eine Datei mit dem Programm oeffnen, das Windows dafuer vorgesehen hat.
// Der Export des Originals macht genau das - im Bildschirmfoto erscheint
// Notepad mit dem erzeugten Text.
void openWithDefaultApp(const std::string& path);
// behaved noch einmal starten, mit einer Datei auf der Kommandozeile.
//
// Das Original tut das beim "run"-Knopf eines Skriptfeldes ("start %s %s",
// "\BehavEd.exe"): das angegebene Skript geht in einer NEUEN Instanz auf,
// der Ereigniseditor der alten bleibt unberuehrt.
bool startBehavedWith(const std::string& datei);

// Ordner fuer die Einstellungsdatei (%APPDATA%\jkstudio, Stufe 8c; bis rc605
// %APPDATA%\behaved - beim ersten Start uebernommen, bhed/umzug.h), wird angelegt.
[[nodiscard]] std::string settingsDirectory();

// Stufe 8c: eine RCDATA-Ressource der .exe (res/behaved.rc), leer wenn es sie nicht gibt.
[[nodiscard]] std::string ressource(const wchar_t* name);

// Der Ordner, in dem das Programm selbst liegt.
//
// Dorthin gehoert das Protokoll: wer es hinschicken soll, findet es neben der
// .exe, statt es in %APPDATA% suchen zu muessen.
[[nodiscard]] std::string executableDirectory();
// Menue-Editor M5b: der volle Pfad der laufenden .exe
[[nodiscard]] std::string executablePath();
// Menue-Editor M5b: Werte unter HKEY_CURRENT_USER schreiben (Dateizuordnung nur nach Rueckfrage im
// Programm; danach SHChangeNotify, damit der Explorer es sofort weiss) bzw. den Standardwert lesen.
struct RegWert {
    std::string schluessel, name, wert;
};
bool schreibeBenutzerRegistry(const std::vector<RegWert>& werte);
[[nodiscard]] std::string liesBenutzerRegistry(const std::string& schluessel);
// Einen Ordner im Explorer zeigen.
void openInExplorer(const std::string& folder);
// "Dokumente" des Anwenders (auch wenn OneDrive ihn umgeleitet hat). Dort
// legt "New mission..." den Missionsordner an. Leer, wenn unbekannt.
[[nodiscard]] std::string dokumenteOrdner();

// Fuer den Selbsttest "einzel": diese exe ein zweites Mal starten, wie ein
// Doppelklick im Explorer - OHNE BHED_EDITORTEST in der Umgebung (die zweite
// soll sich wie ein echter Start verhalten). `argument` leer = ohne Datei.
// Rueckgabe: Griff auf den Prozess, 0 = nicht gestartet.
std::uintptr_t starteZweiteInstanz(const std::string& argument);
// -1 = laeuft noch, sonst der Rueckgabewert.
long zweiteInstanzStand(std::uintptr_t griff);
// Beenden (nur DIESEN eigenen Kindprozess) und den Griff freigeben.
void zweiteInstanzSchliessen(std::uintptr_t griff, bool beenden);

// Fuer den Selbsttest: das Hauptfenster auf eine Groesse (aussen, Pixel)
// setzen - ohne es nach vorn zu holen. Ein maximiertes Fenster wird dafuer
// wiederhergestellt. Rueckgabe: die Groesse vorher (Breite, Hoehe). Mit 0, 0
// nur die Groesse lesen.
std::pair<int, int> hauptfensterGroesse(int breite, int hoehe);

// Die Dateinamen eines Ordners, ohne Pfad und ohne Unterordner.
//
// Gebraucht, um die Haeute neben einem Modell zu finden: davon liegen oft
// mehrere - model_default.skin, model_red.skin, model_blue.skin - und sie
// entscheiden, welche Flaechen sichtbar sind.
[[nodiscard]] std::vector<std::string> listDirectory(const std::string& dir);

// Einen ORDNER auswaehlen lassen.
//
// Vorher gab es das nicht: man musste eine .pk3 im gewuenschten Ordner
// auswaehlen, damit das Programm den Ordner davon nehmen konnte. Der Common
// Item Dialog kann es mit FOS_PICKFOLDERS.
[[nodiscard]] std::string pickFolder(const char* title, const std::string& startDir);


// --- RenderDoc aus dem Programm heraus ---------------------------------
//
// RenderDoc kann sich NICHT nachtraeglich an ein laufendes Programm
// haengen - es legt seine Bibliothek beim Start hinein. Laeuft behaved
// schon darunter, liegt `renderdoc.dll` im Prozess, und ueber
// `RENDERDOC_GetAPI` laesst sich eine Aufnahme ausloesen: dasselbe wie F12,
// nur aus dem Menue.
//
// `renderDocDa()` sagt, ob die Bibliothek da ist - sonst waere der
// Menuepunkt ein Knopf, der nichts tut.
[[nodiscard]] bool renderDocDa();
void renderDocAufnehmen();

// --- rc601 release: ein anderes Programm starten (Selbsttest) -----------
//
// Fuer den Selbsttest "eigenstaendig": eine KOPIE dieser exe (in einem
// Ordner ohne data\base) mit `argumente` starten, wie es ein Anwender mit der
// heruntergeladenen .exe tut. Die Umgebung ist die eigene ohne
// BHED_EDITORTEST; jede "NAME=wert"-Zeile aus `setzen` ersetzt oder ergaenzt
// eine Variable. Rueckgabe wie bei starteZweiteInstanz: Griff auf den
// Prozess, 0 = nicht gestartet. Stand und Ende ueber zweiteInstanzStand und
// zweiteInstanzSchliessen.
std::uintptr_t starteProgramm(const std::string& exe, const std::string& argumente,
                              const std::vector<std::string>& setzen);

// --- Paket kamera, rc601 -------------------------------------------------
//
// Breite durch Hoehe des Hauptbildschirms in Bildpunkten, 0 wenn unbekannt.
// Movie Duels startet mit r_mode -2 und uebernimmt dessen Aufloesung - das
// Seitenverhaeltnis, mit dem es den Bildwinkel einer Kamera rechnet.
[[nodiscard]] float hauptBildschirmSeiten();

// --- Karten-Editor, Farbdialog (QColorDialog "Pick Screen Color") --------
//
// Lage des Mauszeigers in Bildschirmpunkten (QCursor::pos) und die Farbe
// eines Bildschirmpunkts (QScreen::grabWindow( desktop, x, y, 1, 1 )) - nur
// auf Knopfdruck des Anwenders, nie im Selbsttest.
bool zeigerLage(int& x, int& y);
// Stufe 7 Punkt 6c: der Zeiger in Fensterpunkten des Fensters fenster (HWND
// als void*; GetCursorPos + ScreenToClient) und dorthin setzen (ClientToScreen
// + SetCursorPos) - beides sofort, fuer den Freelook der Kamera (FreezePointer).
bool zeigerImFenster(void* fenster, int& x, int& y);
bool zeigerImFensterSetzen(void* fenster, int x, int y);
// dock1: den Zeiger auf Bildschirmpunkte setzen (SetCursorPos) - mit eigenen
// Fenstern rechnet ImGui in Bildschirmpunkten.
bool zeigerSetzen(int x, int y);
// dock1, Selbsttest: gibt es das Windows-Fenster und ist es sichtbar; seine
// Lage und Groesse (ganzes Fenster, Bildschirmpunkte) lesen und setzen - ohne
// es zu aktivieren, die Z-Folge bleibt.
bool fensterSichtbar(void* fenster);
bool fensterLage(void* fenster, int& x, int& y, int& b, int& h);
bool fensterLageSetzen(void* fenster, int x, int y, int b, int h);
bool bildschirmFarbe(int x, int y, unsigned char rgb[3]);
// dock3, Selbsttest: eine Datei auf ein Windows-Fenster ablegen wie aus dem
// Explorer gezogen (WM_DROPFILES mit DROPFILES-Block, Stelle in Fensterpunkten).
bool dateiAblegen(void* fenster, const std::string& pfad, int x, int y);
// rc609 T1, Selbsttest: ein Windows-Fenster schliessen wie mit Alt+F4 oder dem
// Kreuz der Taskleiste (WM_CLOSE in seine Warteschlange).
bool fensterSchliessenSenden(void* fenster);
// Abschluss rc607, Selbsttest: die Windows-Zwischenablage teilen sich alle gleichzeitig laufenden
// Selbsttests. Wer sie zwischen Schreiben und Lesen braucht, haelt diese prozessuebergreifende
// Sperre: sperren wartet nicht (true: jetzt gehalten), freigeben gibt sie wieder ab.
bool zwischenablageSperren();
void zwischenablageFreigeben();
// Abschluss 2, Selbsttest: was der Nutzer in der Zwischenablage hat, sichern und zurueckschreiben -
// alle Formate, die sich kopieren lassen. Das Zurueckschreiben legt keinen neuen Eintrag in den
// Windows-Zwischenablageverlauf. Die Nummer (GetClipboardSequenceNumber) aendert sich bei jedem
// Schreiben, auch von fremden Programmen.
struct ZwischenablageSicherung {
    struct Format {
        unsigned format = 0;
        std::vector<unsigned char> daten;
        void* gdi = nullptr;   // CF_BITMAP, CF_ENHMETAFILE: eine eigene Kopie
    };
    std::vector<Format> formate;
};
bool zwischenablageSichern(ZwischenablageSicherung& s);
bool zwischenablageZurueck(ZwischenablageSicherung& s);
unsigned zwischenablageNummer();
[[nodiscard]] bool zwischenablageHatText();   // geht auch, wenn gerade ein anderes Programm sie offen hat
// Selbsttest: laeuft ein Stoerer (STOERER=leser in SP/ab/lauf.py), haelt er die Ablage ab jetzt kurz offen. Sonst nichts.
void zwischenablageStoererRufen();
// dock4, Selbsttest: nimmt das Windows-Fenster Dateien an (WS_EX_ACCEPTFILES,
// DragAcceptFiles)? Nur dann schickt der Explorer beim Loslassen WM_DROPFILES.
bool nimmtDateien(void* fenster);

// M7: glGetString(GL_VENDOR/RENDERER/VERSION/EXTENSIONS) des OpenGL-Treibers, wie das Spiel sie fuer
// UI_GLINFO liest (verstecktes Fenster, Pixelformat 32/24/8, Kontext gleich wieder weg). false = kein GL.
bool openGlTreiber(std::string& vendor, std::string& renderer, std::string& version, std::string& erweiterungen);

// M7 Punkt 13: einen Befehl in die Konsole eines selbst gestarteten Spiels (Prozess pid) geben -
// Shift+Esc oeffnet die Konsole (CL_KeyDownEvent: "kg.keys[A_SHIFT].down && key == A_ESCAPE"), die
// Zeichen kommen per WM_CHAR, Enter fuehrt aus, Shift+Esc schliesst. Nur PostMessage an das Fenster
// dieses Prozesses (kein Fokus, keine fremden Fenster). false = der Prozess hat kein sichtbares Fenster.
// Nur OpenJK-Programme (SDL) nehmen so gesendete Tasten ohne Fokus an; die Original-Programme nicht.
bool spielKonsolenBefehl(unsigned long pid, const std::string& befehl);
[[nodiscard]] bool prozessLaeuft(unsigned long pid);

// --- Karten-Editor, Textmasse wie Qt 5 (QWindowsFontEngine, GDI) ---------
//
// Breite eines Textes in Pixeln: Schrift schrift mit lfHeight = -pixel,
// GetTextExtentPoint32W (Summe der gehinteten Vorschuebe, ohne Kerning).
// Qt legt Menues und Dialoge in logischen Punkten mit diesen Breiten an
// (Segoe UI 9 pt = 12 px, Tahoma 8 pt = 11 px). -1, wenn es nicht geht.
[[nodiscard]] int gdiTextBreite(const std::string& utf8, const wchar_t* schrift, int pixel, bool fett);

// --- Karten-Editor, Stufe 3 Punkt 0: Textmasse wie Qt 5 bei NRC -------------
//
// Qt 5.15 mit High-DPI-Skalierung misst mit DirectWrite in Entwurfsmassen
// (ungehintet, mit Unterschneidung), siehe qttext_win32.cpp. Schriftgroesse in
// logischen Punkten: Dialoge Tahoma 11 (DEFAULT_GUI_FONT), Menues Segoe UI 12.
struct QtTextMass {
    double vorschub = 0.0;   // Summe der Vorschuebe (horizontalAdvance ungerundet)
    double rechts = 0.0;     // negative rechte Peilung des letzten Zeichens (Betrag)
    bool gueltig = false;
};
[[nodiscard]] QtTextMass qtTextMass(const std::string& utf8, const wchar_t* schrift, float pixel, bool fett);
// fm.boundingRect(text).width() / fm.size(): naturalTextWidth aufgerundet; -1, wenn es nicht geht.
[[nodiscard]] int qtTextBreite(const std::string& utf8, const wchar_t* schrift, float pixel, bool fett);
// fm.horizontalAdvance(text): Summe der Vorschuebe gerundet; -1, wenn es nicht geht.
[[nodiscard]] int qtTextVorschub(const std::string& utf8, const wchar_t* schrift, float pixel, bool fett);
// Die Zeichen von `utf8`, die die Schrift selbst NICHT hat (DirectWrite misst
// sie dann mit einer Ersatzschrift oder dem Ersatzkaestchen) - leer = alle da.
// Fuer den Selbsttest der Sprachen (Spanisch, Tschechisch).
[[nodiscard]] std::string qtFehlendeZeichen(const std::string& utf8, const wchar_t* schrift, bool fett);

// Fuer den Selbsttest (Punkt 12): belegte Bytes des Prozess-Heaps (HeapSummary),
// also was new/malloc halten - der Rest des privaten Speichers gehoert z. B.
// dem Grafiktreiber. -1, wenn es nicht geht.
[[nodiscard]] long long heapBelegt();

// rc608 Teil 12 (Selbsttest "leistung"): Prozessorzeit des ganzen Prozesses
// (Kern und Nutzer, alle Faeden, Sekunden), groesster und jetziger Arbeitssatz
// und privater Speicher in Bytes. -1, wo es nicht geht.
struct ProzessStand {
    double cpuSekunden = -1.0;
    long long spitzeBytes = -1;
    long long arbeitBytes = -1;
    long long privatBytes = -1;
};
[[nodiscard]] ProzessStand prozessStand();

// --- Karten-Editor, Stufe 2 Punkt 11 -------------------------------------
//
// MainFrame_toggleFullscreen (NRC mainframe.cpp:1542-1548): das Hauptfenster
// ein- und ausschalten wie Qt::WindowFullScreen unter Windows - ohne Rahmen
// und Titel ueber den ganzen Bildschirm, mit QWindowsWindowFunctions::
// setHasBorderInFullScreen( true ) einen Rand von einem Punkt (WS_BORDER);
// zurueck mit Stil und Lage von vorher. Rueckgabe: jetzt Vollbild?
bool vollbildUmschalten();
[[nodiscard]] bool istVollbild();
// Fuer den Selbsttest: das Rechteck des Hauptfensters und des Bildschirms.
bool hauptfensterRechteck(int& x, int& y, int& b, int& h);

// Stufe 8a Punkt 1 (Bau fertig): ist das Hauptfenster vorne (GetForegroundWindow)?
// Und den Knopf in der Taskleiste blinken lassen, bis das Fenster vorne ist
// (FlashWindowEx FLASHW_TRAY | FLASHW_TIMERNOFG) - ohne Ton, ohne Dialog.
[[nodiscard]] bool hauptfensterVorne();
void taskleisteBlinken();
bool bildschirmRechteck(int& x, int& y, int& b, int& h);

// QFontDatabase::families() unter Windows (EnumFontFamiliesEx, DEFAULT_CHARSET,
// ohne die senkrechten '@'-Schriften), sortiert; dazu die Datei einer Schrift
// aus HKLM\...\Fonts ("<Familie>[ Bold][ Italic] (TrueType)"), leer wenn keine.
[[nodiscard]] std::vector<std::string> schriftFamilien();
[[nodiscard]] std::string schriftDatei(const std::string& familie, bool fett, bool kursiv);

// Q_Exec (NRC): eine Befehlszeile ohne Warten starten (CreateProcess); false,
// wenn es nicht ging.
bool befehlStarten(const std::string& befehlszeile);

}  // namespace bhed::platform
#endif
