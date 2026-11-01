// menueeditor.h - Menue-Editor M1: der Arbeitsbereich "Menu Editor".
//
// Text und Vorschau desselben .menu: links Bauteile und Gliederung, in der
// Mitte die Seiten als Reiter und die Arbeitsflaeche (640x480 wie das Spiel),
// rechts Eigenschaften, Cvars und Meldungen der Engine, unten der Code-Editor.
#ifndef BHED_GUI_MENUEEDITOR_H
#define BHED_GUI_MENUEEDITOR_H

#include "bhed/menue/hud.h"
#include <map>
#include <functional>
#include <string>
#include <vector>

struct ImFont;

namespace bhed::gui::menueeditor {

[[nodiscard]] bool aktiv();
void zeichne(float hoehe);
// dock1: die herausgenommenen Panels (Bausteine, Flaeche, Reiter) in eigenen Fenstern.
void dockFenster();
// Fuer den Selbsttest und die Tour: Menue aus dem Spiel bzw. Text oeffnen.
bool oeffneAusSpiel(const std::string& spielPfad);
// M7 Punkt 14: ownerdraws/Feeder der Original-Menues, die noch einen Platzhalter bekommen ("SP ownerdraw 249 (ui/x.menu)")
[[nodiscard]] std::vector<std::string> platzhalterImSpiel();
bool oeffneText(const std::string& name, const std::string& text);
// Eine .menu/.txt von der Platte (abgelegt oder ueber den Dialog).
bool oeffneDatei(const std::string& platte);
// M2 Punkt 3: alle Dateien eines Menuesatzes (loadMenu in menus.txt); Zahl der Dateien
int oeffneMenuesatz(const std::string& spielPfad);
[[nodiscard]] std::string letzterSatzWechsel();
[[nodiscard]] std::string laufInfo();   // Selbsttest: aktives Dokument, Satz, Skriptprotokoll
// M2 Ebenenliste: Auge (nur Vorschau), Reihenfolge (verschiebt das itemDef im Text), Alt+Klick-Durchwahl
void verbergeItem(int item, bool verbergen);
void sperreItem(int item, bool sperren);   // rc608: das Schloss der Ebenenliste
bool ebeneAnStelle(int item, int ziel);
void altKlickVorschau(float x, float y);   // Selbsttest: Menue, in dessen Datei zuletzt gewechselt wurde
// Zustand fuer den Selbsttest (Wirkungspruefung).
struct Stand {
    int dokumente = 0;
    int menus = 0;
    int items = 0;
    int seite = 0;
    int seiten = 0;          // Reiter ueber der Flaeche
    std::vector<int> auswahl;
    std::string text;
    int meldungen = 0;
    int prims = 0;           // Zeichenbefehle der Vorschau im letzten Bild
    int asienGlyphen = 0;    // M3: davon Zeichen aus den asiatischen Glyphenseiten
    int hudBilder = 0;       // M3: davon Zahlen (gfx/2d/numbers) und Fadenkreuz des HUD
    bool hudNachgebildet = false;   // M3: cgame-Teil des HUD fuer dieses Spiel nachgebildet (JA SP)
    std::vector<std::string> klangListe;   // M3: ausgeloeste Klaenge seit klangListeLeeren ("musik:" = Hintergrund)
    int spiel = 0;                  // M3: 0 Jedi Academy, 1 Jedi Outcast, 2 Movie Duels (Spieldaten)
    int bilderGeladen = 0;   // Shader mit Textur
    bool interaktion = false;
    std::string cursorZeile; // Text der Zeile unter dem ersten Cursor
    int exporte = 0;         // H2: erfolgreiche .pk3-Exporte (Tour "Ein Menue aendern")
    int undoSchritte = 0;    // rc608: Schritte, die Rueckgaengig zuruecknehmen kann
    int zoomProzent = 0;     // rc608: Zoom der Flaeche
    int abbruchZeile = 0;    // rc608: Zeile, an der das Spiel dieses Menue verwirft (0 = es liest es ganz)
    bool codeFestbreite = false;   // M2: Code-Editor mit Festbreitenschrift
    int kinoBilder = 0;            // M2: gezeigte Bilder aus RoQ-Videos
    int klangStimmen = 0;          // M4: laufende Klaenge der Vorschau (Ton-Schalter aus = 0)
    int verborgen = 0, gesperrt = 0;   // M4: Items mit Auge aus bzw. Schloss zu (Ebenenliste)
    int modellAufrufe = 0;         // M2: Zeichenaufrufe der 3D-Modelle (gpu::zeichneMenueFigur)
    double modellMs = 0;           // reste2 Punkt 7: Prozessorzeit der Figuren bis zu ihrem Bild (Summe, ms)
    int modellZeichnungen = 0;     // reste2 Punkt 7: gezeichnete Figuren (Summe)
    int modellStufen = 0;          // M7: meiste Shaderstufen einer Modellflaeche (2 = Chrom-Stufe des JK2-Logos)
    int beispielPrims = 0;         // M7: beschriftete Beispielwerte im letzten Bild
    int platzhalterPrims = 0;      // M7: beschriftete Platzhalter (ownerdraw/Feeder ohne Darstellung) im letzten Bild
    int ladebalken = 0;            // M7: Zeichenbefehle des Ladebalkens (gfx/hud/mp_levelload, load_tick)
    std::string modellFehler;      // letzte Meldung dabei
    int klangEreignisse = 0;       // M2 Punkt 5: ausgeloeste Klaenge (im Selbsttest stumm)
    std::string letzterKlang;
    int paketDateien = 0;          // M2: eigene Dateien im Paket (Bilder, Videos, Texte, Schriften)
    std::vector<std::string> paket;   // ihre Spielpfade
    float flaeche[4] = {0, 0, 0, 0};  // Bildvergleich: die 640x480-Flaeche im Fenster (x, y, b, h)
    double bildzeitMs = 0;            // Arbeitszeit des Arbeitsbereichs je Bild (gleitender Mittelwert)
    // M2 Live-Diagnose des aktiven Dokuments
    int diagFehler = 0, diagWarnungen = 0, diagHinweise = 0, diagLaeufe = 0;
    bool diagAktuell = false;             // Ergebnis gehoert zum jetzigen Text
    std::vector<std::string> diagnosen;   // "Zeile: Meldung"
};
[[nodiscard]] Stand stand();
// Befehle wie die Knoepfe (Selbsttest)
void waehle(int item, bool dazu);
[[nodiscard]] int itemNummer(const std::string& name);   // M4: Item der Seite nach Namen, -1 = keins
// rc608: die Items im Menue der Seite, wie gelesen: rect im Text (x, y, b, h) und auf dem Bildschirm (sx, sy)
struct ItemLage {
    std::string name;
    float x = 0, y = 0, b = 0, h = 0, sx = 0, sy = 0;
    bool sichtbar = false;
};
[[nodiscard]] std::vector<ItemLage> itemLagen();
// rc608 Punkt 15: jedes Bild und Video der Originalmenues, aufgeloest auf dem Weg, den auch die Vorschau geht
struct BildZeile {
    std::string datei, art, name;
    int zeile = 0;
    bool da = false;
};
[[nodiscard]] std::vector<BildZeile> bildTabelle();
// rc608 Punkt 12 (Selbsttest): die Griffe des gewaehlten Modells auf dem Bildschirm (Mitte x, y; Kantenlaenge g); false = keins gewaehlt
[[nodiscard]] bool modellGriffe(float* xy8, float* g);
// rc608 Punkt 2 (Selbsttest): der Text-Editor im letzten Bild
struct TextAnsicht {
    long long zeichen = 0;
    float scrollX = 0, breiteste = 0, flaeche = 0;
    float balken[4] = {0, 0, 0, 0};
};
[[nodiscard]] TextAnsicht textAnsicht();
// rc608 Punkt 13 (Selbsttest): die Breiten der Spalten (Bildpunkte); was daraus wird, begrenzt das Programm
void setzeSpalten(float links, float rechts);
[[nodiscard]] float rechteSpalte();
[[nodiscard]] float linkeSpalte();
// rc608 Punkt 14 (Selbsttest): die Felder der Properties und die weiteren Eintraege im letzten Bild
[[nodiscard]] std::vector<std::string> eigenschaftsFelder();
[[nodiscard]] std::vector<std::string> weitereEintraege();
// rc608 Punkt 11 (Selbsttest): Pfad statt des Dateidialogs von "Bild hinzufuegen"; Rahmen beim Ziehen
void setzeBildDialog(std::function<std::string()> antwort);
[[nodiscard]] bool ablageRahmen(float* xywh);
// rc608 Punkt 5: Text an Ort und Stelle (Selbsttest)
struct SchreibStand {
    bool an = false;
    int item = -1;
    std::string puffer;
    float cursorX = 0, cursorY = 0, textRechts = 0;
};
[[nodiscard]] SchreibStand schreibStand();
[[nodiscard]] std::string letzteMeldung();   // rc608: was die Leiste gerade meldet ("" = nichts)
void verschiebeAuswahl(float dx, float dy);
void setzeInteraktion(bool an);
void klickVorschau(float x, float y);   // im virtuellen Bildschirm
void setzeCvar(const std::string& name, const std::string& wert);
// Rechte Spalte: 0 Hierarchie, 1 Text, 2 Eigenschaften (gespeichert in den Einstellungen)
void setzeReiter(int reiter);
void schliesseTestFrage();   // M4: offene Frage von "Im Spiel testen" (Mod-Ordner) mit Nein schliessen
// Live-Diagnose: die Schnellkorrektur der Meldung Nr. anwenden
bool korrigiereDiagnose(int nr);
[[nodiscard]] int reiter();
[[nodiscard]] std::string cvarWert(const std::string& name);   // Vorschau bzw. Bedienen
bool rueckgaengig();
bool wiederholen();
void fuegeEin(int typ);
int suche(const std::string& muster, bool regex);
int ersetzeAlle(const std::string& muster, const std::string& ersatz, bool regex);
void springeZuItem(int item);
// Items der Auswahl: duplizieren (Strg+D), loeschen (Entf), kopieren (Strg+C), einfuegen (Strg+V)
void dupliziereAuswahl();
void loescheAuswahl();
std::string kopiereAuswahl();
void fuegeItemsEin(const std::string& text);
bool speichereUnter(const std::string& pfad);
// H2 (KI-Auftraege): die Textaenderungen seit `vorher` als EIN Rueckgaengig-Schritt.
// false, wenn sich die Schritte nicht zusammenfassen liessen (dann bleiben sie einzeln).
bool zuEinemSchritt(const std::string& vorher);
// M5b Punkt 3: Projektordner + Projektdatei *.jksmenu (bhed/menue/projekt.h)
bool oeffneProjekt(const std::string& datei);         // sofort (Selbsttest, Dialog)
void projektVormerken(const std::string& datei);      // Doppelklick/Kommandozeile: im naechsten Bild
[[nodiscard]] bool istProjekt(const std::string& datei);
bool legeProjektAn(const std::string& datei);         // die offenen Menues kommen hinein
void neuesProjektFormat(bool mehrspieler);          // M7: Format des Startmenues (Einzel-/Mehrspieler)
// rc608 Selbsttest: liefert den Pfad statt der Dateidialoge von "Neues Projekt" (true) und "Projekt oeffnen" (false)
void setzeProjektDialog(std::function<std::string(bool)> antwort);
// M7 Punkt 13: ein laufendes Testspiel uebernehmen (Selbsttest-Nachweis: das Spiel startet ein Python-
// Skript mit der Befehlszeile von "Im Spiel testen"); Zahl der ui_load-Neuladungen seit dem Start
void testSpielUebernehmen(unsigned long pid, const std::string& ordner);
[[nodiscard]] int testNeuGeladen();
// M7 Punkt 3: Schluessel des gewaehlten Elements setzen wie die Eigenschaften (ein Rueckgaengig-Schritt)
void setzeEigenschaftAuswahl(const std::string& schluessel, const std::vector<std::string>& werte);
[[nodiscard]] std::string dokumentName();           // M7: Spielpfad des aktiven Dokuments ("" = keins)
[[nodiscard]] std::string fensterTitel();           // rc608: Titel der Fensterleiste, "ui/ail.menu * - JK Studio"
[[nodiscard]] std::vector<std::string> hierarchieZeilen();   // rc608: Namen der Elemente in der Hierarchie, letztes Bild
void setzeSeite(int seite);
[[nodiscard]] bool dokumentMp();                    // M7: aktives Dokument im MP-Dialekt
bool speichereProjektJetzt();
bool exportiereProjektJetzt();
[[nodiscard]] std::string projektDatei();             // "" = kein Projekt
void sichereJetzt();                                  // Sicherung sofort (sonst alle 30 Sekunden)
void sucheSicherung();                                // Sicherung am jetzigen Ort suchen (fragt nach)
void schliesseAlleOhneSpeichern();                    // Selbsttest: wie nach einem Absturz
void fragZuordnung();                                 // Frage zur Dateizuordnung zeigen
void beimBeenden();                                   // beim Beenden: Ungespeichertes sichern
// M5b Punkt 6: Messwerte fuer die Tour "Dein erstes eigenes Menue" (gui/tour_messwerte.cpp)
struct TourWerte {
    bool projekt = false;     // ein Projekt ist offen
    int elemente = 0;         // Items im Menue der Seite
    int layouts = 0;          // Layouts in diesem Menue
    int ereignisse = 0;       // Ereignis-Skripte (mouseEnter ... accept) seiner Items
    int gespeichert = 0;      // erfolgreiche Projekt-Speicherungen seit dem Start
};
[[nodiscard]] TourWerte tourWerte();
// M5b Punkt 4: Mod-Profile (bhed/menue/profil.h); "" = das eingebaute des Spiels
void setzeModProfil(const std::string& name);
[[nodiscard]] std::string modProfil();
bool legeProfilVorlageAn(const std::string& pfad);    // Vorlage zum Ausfuellen schreiben
bool exportierePk3(const std::string& pfad);
// M2 Punkt 7: nur geaenderte Menues (und das Paket); Pruefung fuer die Fassungen
bool exportiereAenderungen(const std::string& pfad);
[[nodiscard]] int letzterExport();   // Dateien im zuletzt geschriebenen Paket
int pruefeFuerSpiel(bool ja, bool jk2, bool md);   // Befunde aller offenen Menues
// M2 Punkt 8: Testpaket und Befehlszeile (im Selbsttest nie gestartet); "" = nicht moeglich
std::string imSpielTesten();
// M2 Punkt 9: Schrift aus einer TTF ins Paket (fonts/<name>.fontdat + .tga)
bool baueSchriftAusTtf(const std::string& ttf, const std::string& name, int groesse);
// M2 Punkt 6: Sprache der Vorschau, eigene Texte
void setzeVorschauSprache(const std::string& sprache);
// texte: Sprache -> Text wie getippt (UTF-8); gespeichert in der Kodierung der Sprache
bool legeTextAn(const std::string& schluessel, const std::map<std::string, std::string>& texte);
[[nodiscard]] std::string loeseText(const std::string& at);
[[nodiscard]] std::string textPaket();
// M3 Punkt 5: Beispielwerte des HUD (wie die Regler im Cvar-Feld)
void setzeHudWerte(const bhed::menue::HudWerte& w);
// M3 Punkt 3: das Klangprotokoll leeren
void klangListeLeeren();
// M3: eine Datei des Pakets (Spielpfad, Gross/Klein egal); "" = nicht da
[[nodiscard]] std::string paketInhalt(const std::string& spielPfad);
// M2: Festbreitenschrift des Code-Editors (main_win32.cpp buildFonts)
void setzeCodeSchrift(ImFont* f);
void setzeBalken(bool balken);   // 4:3 mit Balken (true) oder gestreckt
// M2 Punkt 2 / M4 Punkt 5: Video/GIF/RoQ als RoQ ins Paket (synchron, fuer Selbsttest),
// dazu der Shader menu/video/<name> in shaders/<paket>.shader; Spielpfad oder "" (fehler: Meldung)
struct VideoImport {
    int breite = 512, hoehe = 512;
    int einpassen = 0;     // 0 strecken, 1 Balken, 2 zuschneiden
    int guete = 70, fps = 30;
    bool ton = false, stereo = true;
    int verwendung = 0;    // 0 Hintergrund des gewaehlten Items, 1 neues Vollbild-Element, 2 cinematic, 3 nur ins Paket
};
std::string importiereVideo(const std::string& platte, const VideoImport& o, std::string* fehler = nullptr);
void zeigeVideoImport(const std::string& platte);   // M4: Dialog fuer eine Datei zeigen, "" = schliessen
// M2 Punkt 4: Vorlage aus der Bibliothek an (x, y) einsetzen; Auswahl als eigene Vorlage
bool fuegeVorlageEin(const std::string& id, float x, float y);
bool speichereAuswahlAlsVorlage(const std::string& name);
std::vector<std::string> vorlagenIds();
// Datei direkt ins Paket legen (Spielpfad -> Bytes)
void legeInsPaket(const std::string& spielPfad, const std::string& bytes);

// M6 (Selbsttest): Menuefluss, Tab-Reihenfolge, Referenzbild, Reiter schliessen
struct FlussInfo {
    int menues = 0, unerreichbar = 0, sackgassen = 0, ohneZurueck = 0, fehlend = 0, kanten = 0;
    bool fensterOffen = false, stileOffen = false, assistentOffen = false;
    int assistentUeberlappung = 0;       // M7 (Uebersetzung 2): Zeilen, deren Text sichtbar in die naechste Spalte ragt
    std::string aktivesMenue;            // Name des Menues der Seite
};
[[nodiscard]] FlussInfo flussInfo();
[[nodiscard]] std::vector<std::string> tabFolge();   // Namen in der Tab-Reihenfolge (Menue der Seite)
[[nodiscard]] bool tabFolgeAn();
[[nodiscard]] bool referenzbildGeladen();
void ladeReferenzbild(const std::string& platte);
[[nodiscard]] bool schliessenFrageOffen();
void schliesseReiter(int dok);   // wie das x am Reiter (fragt bei Ungespeichertem)
[[nodiscard]] std::vector<std::string> assistentKatalog();   // Cvars, fuer die der Assistent eine Steuerung kennt
void assistentWaehle(const std::string& cvar);
[[nodiscard]] std::vector<std::string> originalUnterschiede();   // Schluessel des gewaehlten Elements, die vom Original abweichen
[[nodiscard]] int menuesImDokument();
[[nodiscard]] std::string itemNameBei(int item);   // Name des Items der Seite ("" = keins/ohne Namen)
}  // namespace bhed::gui::menueeditor

#endif  // BHED_GUI_MENUEEDITOR_H
