// app_menue.cpp - Menue-Editor M1: der Arbeitsbereich "Menu Editor".
//
// Eigener Code. Die Engine-Logik (Lesen, Zeichnen, Skripte) steckt in
// src/menue (portiert aus OpenJK, siehe dort); hier nur die Oberflaeche:
//   links    Bauteile (Palette) und Gliederung
//   Mitte    Seiten als Reiter, Arbeitsflaeche 640x480 mit Linealen,
//            Raster, Einrasten, Auswahl/Ziehen/Groesse, Interaktionsmodus
//   rechts   Eigenschaften, Cvars (nur Vorschau), Meldungen der Engine,
//            Skriptprotokoll
//   unten    Code-Editor (Farben, Zeilennummern, Lesezeichen- und
//            Aenderungsrand wie im Skript-Editor, Falten, Minimap,
//            Klammerpaare, Mehrfachcursor, Suchen/Ersetzen, Vervollstaendigung,
//            Gehe zu Definition)
// Text und Vorschau sind dasselbe: jede Aenderung an der Flaeche ersetzt nur
// ihre Spanne im Text, jede Textaenderung liest das Menue neu.
#include "menueeditor.h"
#include "videoimport.h"
#include "karte_bau_plattform.h"
#include "schrift_ttf.h"

#include "app_internal.h"
#include "tabfenster.h"   // rc609 T1
#include "tour.h"   // H2: leerer Zustand mit Tutorial
#include "fenster_dock.h"   // dock1: die Panels als eigene Fenster
#include "dockvorgabe.h"    // dock4: die Panels im DockSpace
#include "backend.h"
#include "gpumap.h"
#include "platform.h"

#include "bhed/gpuconst.h"
#include "bhed/gpuskin.h"
#include "bhed/i18n.h"
#include "bhed/mapview.h"
#include "bhed/wave.h"
#include "bhed/image.h"
#include "bhed/menue/assistent.h"
#include "bhed/menue/bauteile.h"
#include "bhed/menue/datapad.h"
#include "bhed/menue/diagnose.h"
#include "bhed/menue/ereignisse.h"
#include "bhed/menue/feeder.h"
#include "bhed/menue/fluss.h"
#include "bhed/menue/hud.h"
#include "bhed/menue/komponenten.h"
#include "bhed/menue/imspiel.h"
#include "bhed/menue/ladebild.h"
#include "bhed/menue/layout.h"
#include "bhed/menue/lesbarkeit.h"
#include "bhed/menue/profil.h"
#include "bhed/menue/pruefung.h"
#include "bhed/menue/projekt.h"
#include "bhed/menue/schriftbau.h"
#include "bhed/menue/seitenvorlagen.h"
#include "bhed/menue/menue.h"
#include "bhed/menue/modell.h"
#include "bhed/menue/ownerdraw.h"
#include "bhed/menue/pazaak.h"
#include "bhed/menue/roq.h"
#include "bhed/menue/skript.h"
#include "bhed/menue/spieldaten.h"
#include "bhed/menue/stile.h"
#include "bhed/menue/tasten.h"
#include "bhed/menue/textpuffer.h"
#include "bhed/menue/vorschau.h"
#include "bhed/menue/werkzeuge.h"
#include "bhed/pk3.h"
#include "bhed/regelwerk.h"
#include "bhed/sicherschreiben.h"
#include "bhed/scene.h"
#include "bhed/shaderscript.h"
#include "bhed/utf8pfad.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <atomic>
#include <chrono>
#include <ctime>
#include <mutex>
#include <thread>

namespace bhed::gui::menueeditor {

using namespace bhed::menue;
using i18n::Str;
namespace fs = std::filesystem;

namespace {

// --- Zustand ---------------------------------------------------------------

// M2 Punkt 6: ein eigener Text (Schluessel ohne Paket) in den Sprachen
struct TextZeile {
    std::string schluessel;
    std::map<std::string, std::string> text;   // "English", "German" ... -> Text
};

struct Dokument {
    std::string name;          // Spielpfad (ui/main.menu) - bestimmt Dialekt
    std::string platte;        // Datei auf der Platte ("" = aus dem Spiel)
    bool ausSpiel = false;     // rc609 ME2: aus den Spieldaten gelesen (Aus dem Spiel, Menuesatz), nicht neu angelegt
    std::vector<Datei> altStaende;   // rc608: die Lesestaende dieses Bildes; die Panels halten Verweise hinein
    Textpuffer puffer;
    std::string geladen;       // Text beim Oeffnen
    std::string gespeichert;   // Text beim letzten Speichern
    Datei datei;
    int geparst = -1;          // Pufferversion des Modells
    // rc608 Punkt 4: die Ansicht der Flaeche. Nur Anzeige: kein Undo-Schritt, nicht in der Datei.
    float zoom = 1.0F;         // Faktor auf "passend"
    float panX = 0, panY = 0;  // Bildpunkte
    int zoomProzent = 100;     // Bildpunkte je virtuellem Punkt (x-Richtung) in Prozent
    int zoomWunsch = 0;        // aus Tasten und Knoepfen: 1 naeher, -1 weiter, 2 passend, 3 100 %
    std::set<int> lesezeichen;
    std::set<int> gefaltet;    // Anfangszeilen eingeklappter Bloecke
    std::set<int> gesperrt;    // Items des Menues gesperrtMenue, die die Flaeche nicht bewegt
    std::map<int, std::set<int>> gesperrtAndere;   // rc608 Teil 3b: das Schloss der uebrigen Menues; eine Nummer gilt nur im eigenen Menue
    int gesperrtMenue = -1;
    std::set<std::pair<int, int>> verborgen;   // M2 Ebenenliste: (Menue, Item) nur in der Vorschau ausgeblendet
    int umbenennen = -1;       // Item, dessen Name in der Ebenenliste bearbeitet wird
    int umbenennenMenu = -1;   // rc608: das Menue dazu; der Index allein gilt in jedem Menue
    bool umbenennenNeu = false;   // rc608: den Fokus nur beim Anfang nehmen
    int umbenennenBild = 0;
    std::string neuerName;
    int hierarchieAnker = -1;  // rc608 Teil 3a: von hier aus waehlt Umschalt+Klick einen Bereich
    std::set<std::pair<int, int>> abgebildet;   // rc609 ME1: (Menue, Item), die bei der letzten Lesung einen Vorgaenger hatten
    int seite = 0;
    int seiteGezeigt = -1;     // welcher Reiter zuletzt offen war
    std::vector<int> auswahl;  // Items im Menue der Seite
    float scrollZeile = 0;   // in Reihen (mit Umbruch mehr als Zeilen)
    float scrollX = 0;       // ohne Umbruch: seitwaerts (Bildpunkte)
    std::uint32_t letzterCursor = 0;
    int springeZeile = -1;
    std::map<std::string, std::string> bilder;   // eigene Grafiken: Spielpfad -> Bytes
    // rc608 Teil 3a: Was Bild und Video ins Paket legen, gehoert zum Rueckgaengig-Schritt: der Stand steht im Beiwerk des Textpuffers
    struct PaketAenderung {
        std::string datei;
        bool warDa = false, istDa = true;
        std::string vorher, nachher;
    };
    std::vector<std::vector<PaketAenderung>> paketSchritte;
    int paketStand = 0;        // so viele Schritte sind angewandt
    int satz = -1;             // M2: gehoert zum Menuesatz Nr. (menus.txt), -1 = einzeln
    // rc608 Teil 3b: Stand der Datei auf der Platte, um Aenderungen von aussen zu bemerken
    fs::file_time_type platteZeit{};
    std::uintmax_t platteGroesse = 0;
    bool platteGemerkt = false;
    double platteGeprueft = -10.0;
    bool fremd = false;
    std::string fremdText;     // was jetzt auf der Platte steht
    // M2 Live-Diagnose: Ergebnis fuer Pufferversion diagVersion; der Lauf im Hintergrund
    std::vector<Diagnose> diagnosen;
    int diagVersion = -1, diagGewollt = -1, diagLaufVersion = -1;
    double diagAb = 0;
    std::future<std::vector<Diagnose>> diagLauf;
    // M2 Punkt 6: eigene Texte (aus dem Paket gelesen, dorthin geschrieben)
    std::string textPaket;
    std::vector<TextZeile> eigeneTexte;
    bool texteGelesen = false;
    int textStand = 0;
};
// M2 Punkt 7: eine Zeile der Pruefung fuer das Spiel
struct PruefZeile {
    int dok = -1;
    Diagnose x;
};

struct Textur {
    void* tex = nullptr;
    int w = 0, h = 0;
    bool additiv = false;
    bool versucht = false;
};

// M4 Punkt 5: Einstellungen und Ergebnis des Video-Imports
struct VideoEinst {
    VideoZiel ziel;                 // 512 x 512 wie die Videos von JKA und Movie Duels
    int guete = 70, fps = 30;
    bool ton = false;               // Menues spielen Videos stumm (CIN_silent) - ab Werk aus
    bool stereo = true;
};
struct VideoErgebnis {
    std::string roq, fehler;
    bool ohneTon = false;           // Ton gewuenscht, aber keine Tonspur
    bool roqDirekt = false;         // RoQ unveraendert uebernommen
    int breite = 0, hoehe = 0, fps = 0;
};
enum class VideoVerwendung { Hintergrund, NeuesElement, Cinematic, NurPaket };

// M2: ein laufendes RoQ (cinematic) mit seiner Textur
struct Kino {
    RoqSpieler spieler;
    void* tex = nullptr;
    int w = 0, h = 0;
    bool fehlt = false;
    unsigned stand = 0;
    int bilder = 0;
};
// M2: ein 3D-Modell eines ITEM_TYPE_MODEL - Modell, Haeute und Bilder einmal
// geladen; gezeichnet von gpu::zeichneMenueFigur in sein Ziel Nr. nr
struct ModellAnsicht {
    std::map<int, int> zielJeItem;
    MenueModell modell;
    ModelTextures texturen;
    bool texturenDa = false;
    int nr = -1;
    int klingenNr = -1;   // eigenes Ziel fuer die Klingen eines Saebers
    std::vector<BoneMatrix> welt, haut;   // je Bild wiederverwendet
    int animStart = 0;    // M3: DC->realTime beim Laden (ItemParse_asset_model_go setzt die Animation)
};
// M2: das Bild eines Klingen-Shaders (gfx/effects/sabers/<farbe>_glow usw.)
struct KlingenBild {
    TextureSet::Tex bild;
    // M3: blendFunc der Stufe - 0 GL_ONE GL_ONE, 1 blend (GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA,
    // schwarze Klingen), -1 eine andere (nicht gezeichnet). GL_DST_ALPHA GL_ONE (black_glow)
    // zaehlt als 0: hinter dem Saeber steht der deckende Menuehintergrund (Ziel-Alpha 1).
    int mischung = 0;
    bool vertexFarbe = true;   // rgbGen vertex (shaderRGBA zaehlt)
    bool gesucht = false;
};

enum class Ziehen { Nichts, Verschieben, Groesse, Lineal, Rahmen };
// M7 Punkt 12: die Werte, die die Griffe am Modell aendern (app_menue_m7.inc)
struct GizmoWerte {
    int angle = 0;
    float mins[3] = {0, 0, 0}, maxs[3] = {0, 0, 0};
    float fovx = 0, fovy = 0;
};
struct DiagDaten;   // Schnappschuss der Spieldaten fuer die Diagnose (unten)
// rc608 Teil 3a: Miniatur einer eigenen Vorlage, Mass einer Kachel (Selbsttest)
struct MiniEintrag {
    Rechteck r;
    Farbe f;
    bool voll = false;
};
struct KachelMass {
    std::string name;
    ImRect rect;
    float textBreite = 0, textUnten = 0;
};

struct Zustand {
    std::vector<std::unique_ptr<Dokument>> docs;
    int exporte = 0;   // H2: erfolgreiche .pk3-Exporte (Tour)
    int aktiv = -1;
    Spieldaten daten;
    std::size_t datenStand = static_cast<std::size_t>(-1);
    Spiel spiel = Spiel::JA;
    Texte texte;
    bool texteGeladen = false;
    ShaderMap shader;
    bool shaderGeladen = false;
    std::map<std::string, Textur> texturen;
    unsigned texturStand = 0;
    std::map<std::string, Kino> kinos;               // M2: Name des Videos -> Spieler
    std::map<std::string, ModellAnsicht> modelle;    // M2: Modell|Haut|Animation -> Ansicht
    std::map<std::string, KlingenBild> klingenBilder;   // M2: Klingen-Shader -> Bild
    int kinoBilder = 0;                              // Selbsttest: gezeigte Videobilder
    int naechsterSatz = 0;                           // M2 Punkt 3: Nummer des naechsten Menuesatzes
    int aktivGezeigt = -1;                           // welcher Dokument-Reiter zuletzt offen war
    float flaeche[4] = {0, 0, 0, 0};                 // Bildvergleich: 640x480 im Fenster (x, y, b, h)
    double bildzeitMs = 0;                           // Messung: Arbeitszeit von zeichne() je Bild (gleitend)
    std::string satzWechsel;                         // Selbsttest: zuletzt ueber Dateien geoeffnetes Menue
    std::vector<int> unterZeiger;                    // Rechtsklick-Liste der Flaeche
    Kino* kinoGriff = nullptr;                       // rc608 Teil 3b: cin.currentHandle
    int kinoWechselBild = -10, kinoZweiBild = -10;   // ... wann der Griff zuletzt wechselte; zweimal in einem Bild: zwei Videos zugleich
    std::string pruefStand;                          // rc608 Teil 3b: der Stand, zu dem die Pruefliste gehoert
    std::vector<std::string> pruefZeilen;            // ... und ihre Zeilen im letzten Bild (Selbsttest)
    std::map<std::string, ImVec2> feldStelle;        // Selbsttest: Auswahlfelder ohne Beschriftung
    bool spaltenFrei = true, spaltenEng = false, linksZu = false, rechtsZu = false;   // rc608 Teil 3b: Spalten neben der Flaeche
    ImVec2 flaecheBild{0.0F, 0.0F};                  // rc608 Teil 3b: die ganze Flaeche ohne Zoom, in Bildpunkten
    int spielTests = 0;                              // rc608 Teil 3b: fertig vorbereitete "Im Spiel testen" (Tour)
    int bedingungNeu = -1;                           // rc608 Teil 3b: die gewaehlte Art einer noch leeren Bedingung
    std::vector<int> rahmenBasis;                    // rc608 Teil 3b: Rahmen aufziehen: was vorher schon gewaehlt war (Strg, Umschalt)
    int kontextWunsch = 0;                           // rc608 Teil 3b: Befehl aus dem Rechtsklick-Menue, im naechsten Bild vor den Panels
    std::function<std::string()> speichernDialog;    // Selbsttest: Pfad statt des Dialogs "Speichern unter"
    int seiteKontext = -1;                           // rc608 Teil 3b: Reiter, dessen Menue der Rechtsklick meint
    std::string seiteName;                           // ... und das Feld zum Umbenennen
    bool seiteNameBelegt = false;
    bool seiteNameFokus = false;
    // M2 Punkt 2 / M4 Punkt 5: Video-Import (Kodieren im Hintergrund)
    bool videoOffen = false;
    std::string videoQuelle;
    VideoEinst videoEinst;
    VideoVerwendung videoVerwendung = VideoVerwendung::Hintergrund;
    std::string videoInfoFuer;                       // Quelle, zu der videoInfo gehoert
    gui::VideoInfo videoInfo;
    int videoArt = 0;                                // 0 Video, 1 GIF, 2 RoQ (unveraendert)
    int videoRoqFps = 0;
    std::thread videoFaden;
    std::atomic<int> videoFertig{0}, videoGesamt{0};
    std::atomic<bool> videoLaeuft{false}, videoAbbruch{false};
    std::mutex videoMutex;
    VideoErgebnis videoErgebnis;
    std::string videoName;
    bool videoDa = false;
    std::thread schaetzFaden;                        // M4: Groesse vorher (die ersten Takte kodiert)
    std::atomic<bool> schaetzLaeuft{false};
    std::atomic<long long> schaetzBytes{-1};
    std::string schaetzFuer;
    std::vector<std::uint64_t> klangStimmen;         // M4: laufende Klaenge der Vorschau (zum Anhalten)
    // M2 Punkt 4: Bauteil-Bibliothek
    std::vector<Vorlage> vorlagen;
    bool vorlagenGeladen = false;
    std::string vorlageName;
    // rc608 Teil 3a: linke Leiste
    std::map<std::string, std::vector<MiniEintrag>> vorlagenMini;
    std::string vorlageUmbenennen;   // Kennung der Vorlage, deren Name gerade bearbeitet wird
    bool vorlageSpeichern = false, vorlageFokus = false;
    Vorlage vorlageWeg;              // zuletzt geloescht, laesst sich zuruecknehmen
    std::string teileSuche, hierarchieSuche;
    std::vector<KachelMass> kacheln;
    float kachelnScrollMaxX = 0;
    int kachelnBild = -1;   // Endrunde 4: das Bild, in dem die Kacheln zuletzt gezeichnet wurden (zu = keine)
    std::vector<int> hierarchieReihe, hierarchieReiheAlt;   // die Zeilen der offenen Seite, wie gezeigt
    int modellAufrufe = 0;                           // Selbsttest: Zeichenaufrufe der Menue-Figuren
    double modellMs = 0;                             // reste2 Punkt 7: Prozessorzeit der Figuren (Summe, ms)
    int modellZeichnungen = 0;                       // reste2 Punkt 7: gezeichnete Figuren (Summe)
    int modellStufen = 0;                            // M7: meiste Shaderstufen einer Flaeche (Chrom des JK2-Logos)
    std::string modellFehler;                        // letzte Meldung von gpu::zeichneMenueFigur
    Textur weiss;                                    // $whiteimage / *white (tr.whiteImage)
    unsigned weissStand = 0;
    int naechstesZiel = 0;                           // Nummer des naechsten Figurenziels
    std::map<std::string, Schrift> schriften;
    std::vector<int> rahmenTexteGezeigt;   // rc608 Teil 3a: Selbsttest
    std::vector<std::string> schriftNamenSp, schriftNamenMp;
    bool schriftNamenGeladen = false;
    FeederDaten feederSp, feederMp;   // M2: Listbox-Feeder aus den Spieldaten
    SaberMap saeber;                  // M2: ext_data/sabers/*.sab (UI_SaberModelForSaber)
    bool saeberGeladen = false;
    // M2: fadeClamp/fadeAmount/fadeCycle aus assetGlobalDef je Dialekt (Menu_Init), Spieltakt
    float fadeClamp[2] = {1.0F, 1.0F}, fadeAmount[2] = {0.0F, 0.0F};
    std::string fokusKlang[2];   // M2 Punkt 5: assetGlobalDef itemFocusSound (SP, MP)
    int fadeCycle[2] = {0, 0};
    int spielFps = 125;   // com_maxfps (OpenJK-Vorgabe)
    // Ansicht
    Seitenverhaeltnis seitenv = Seitenverhaeltnis::Strecken;
    int aufloesung = 0;   // 0 = Fenster, sonst Voreinstellung
    bool interaktion = false;
    bool alleItems = false;   // wie im Spiel; an: verborgene gedimmt
    int klangEreignisse = 0;  // Selbsttest: ausgeloeste Klaenge (abgespielt oder stumm)
    std::string letzterKlang;
    bool raster = false;
    bool anRaster = false;
    bool anKanten = true;
    float rasterWeite = 8.0F;
    // Ziehen
    Ziehen ziehen = Ziehen::Nichts;
    int griff = -1;
    ImVec2 ziehDruck{};       // rc608: wo der Druck war (Bildpunkte); erst ab der Schwelle zieht er
    bool ziehLaeuft = false;
    int klickWahl = -1;       // rc608: was ein Klick ohne Ziehen beim Loslassen waehlt (Alt, Mehrfachauswahl)
    ImVec2 ziehStart{0, 0};
    float zdx = 0, zdy = 0;
    Rechteck groesseNeu;
    std::vector<Hilfslinie> linien;
    std::vector<float> hilfslinienX, hilfslinienY;   // vom Lineal gezogene
    // Laufzeit (Interaktionsmodus)
    Laufzeit lauf;
    int laufDatei = -1;
    int laufVersion = -1;
    int laufGeparst = -1;
    Datei laufKopie;
    std::map<std::string, std::string> vorschauCvars;   // Editor-Modus
    // Code-Editor
    bool codeFokus = false;
    int codeBild = -2;    // rc608: zuletzt hatte der Text-Editor den Fokus (Bildnummer)
    int undoWunsch = 0;   // rc608: aus dem Text-Editor: -1 rueckgaengig, 1 wiederholen
    int werteBild = -2;   // rc608: zuletzt hatte ein Panel mit Werten den Fokus (Properties, Events, Text-Editor)
    bool suchenOffen = false;
    bool suchFokus = false;
    bool ersatzFokus = false;
    std::string such, ersatz;
    SuchOptionen suchOpt;
    bool inAllen = false;
    bool vervollOffen = false;
    std::vector<std::string> vervoll;
    int vervollWahl = 0;
    float codeHoehe = 0;
    float linksBreite = 0, rechtsBreite = 0;
    bool pannt = false;   // rc608 Punkt 4: die Ansicht wird gerade verschoben
    bool schachbrett = true;     // rc608 Punkt 9: zeigt, dass ein Menue ohne fullScreen nichts deckt
    int schachbrettZellen = 0;
    int meldungenLeerBild = -1;   // rc608: letztes Bild, in dem "Keine Meldungen" stand
    bool balkenZieht = false;   // rc608 Punkt 2: der waagerechte Rollbalken des Text-Editors
    float balkenGriff = 0;
    ImRect balkenRect;
    long long sichtbareZeichen = 0;
    float textBreiteste = 0, textFlaeche = 0;
    float textMass[4] = {0, 0, 0, 0};   // Selbsttest: Zeilenhoehe, x des Textes, Breite eines Zeichens, y des Textes
    int codeUeberBild = -2;             // rc608 Teil 3a: zuletzt stand der Zeiger ueber dem Text-Editor (Bildnummer)
    double codeZoomAb = -1.0;           // wann der Zoom zuletzt geaendert wurde (kurze Anzeige)
    std::vector<std::string> felderGezeigt;   // rc608 Punkt 14: die Felder der Properties im letzten Bild
    std::vector<std::string> weitereEintraege;
    std::string listenFilter;
    std::vector<std::string> modellListe, skinListe, animListe;
    Abbildung letztesAb;   // Selbsttest: die Abbildung und der Ursprung der Flaeche im letzten Bild
    ImVec2 letztesO{0, 0};
    bool ablageAn = false, ablageWar = false;   // rc608 Punkt 11: gerade wird etwas ueber die Flaeche gezogen
    Rechteck ablageRect;
    std::function<std::string()> bildDialog;    // Selbsttest statt des Dateidialogs
    std::function<std::string()> menueDialog;   // rc609 ME1: Selbsttest statt des Dialogs "Datei oeffnen"
    bool hierarchieRollen = false;      // rc608 Punkt 10: die Auswahl hat sich geaendert, die Liste rollt hin
    std::vector<int> hierarchieAuswahl;
    std::vector<std::string> hierarchieZeilen;   // Selbsttest: die Zeilen im letzten Bild
    // rc608 Punkt 5: Text an Ort und Stelle schreiben. Die Vorschau zeichnet das Getippte wie das Spiel, Eingabe und
    // Bearbeiten (Tasten, Ablage, eigenes Rueckgaengig) macht ein unsichtbares ImGui-Feld ueber dem Element.
    struct Schreiben {
        bool an = false, neu = false, mehrzeilig = false, zieht = false, warAktiv = false;
        Dokument* d = nullptr;
        int menu = -1, item = -1;
        std::string puffer, start;       // UTF-8, wie im Feld
        std::string schluessel;          // "@..." bei einem Text aus einem Paket
        int startPos = 0;                // Textscroll: erste Zeile
        int anker = 0;                   // Auswahl mit der Maus (Stelle im Feld)
        int cursorWunsch = -1;           // Stelle im Feld nach dem Doppelklick
        ImVec2 klick{0, 0};              // wo doppelgeklickt wurde
        ImVec2 a{0, 0}, b{0, 0};         // Rechteck auf dem Bildschirm
        std::vector<TextLauf> laeufe;
        float cursorX = 0, cursorY = 0;  // virtuell, fuer den Selbsttest
        float textRechts = 0;
    } schreib;
    int reiterGewollt = -2;   // rechte Spalte: -2 = aus den Einstellungen, -1 = keiner, sonst setzen
    // M2 Live-Diagnose
    std::shared_ptr<const DiagDaten> diagDaten;
    std::size_t diagDatenStand = static_cast<std::size_t>(-1);
    bool problemeFehler = true, problemeWarnungen = true, problemeHinweise = true;
    float problemeHoehe = 0;
    int diagLaeufe = 0;
    // M2 Punkt 6: Sprache der Vorschau, eigene Texte
    std::string sprache = "English";
    std::vector<std::string> sprachen;
    bool sprachenGeladen = false;
    Texte texteMit;
    std::string texteMitSchluessel;
    int texteVersion = 0;
    bool texteFensterOffen = false;
    // M2 Punkt 7/8/9
    bool pruefFenster = false, pruefGelaufen = false;
    bool pruefZiel[3] = {true, true, true};
    std::vector<PruefZeile> pruefErgebnis;
    std::string testBefehl;
    bool testFrage = false;
    bool testFrageZu = false;   // M4: die Frage von aussen schliessen (Selbsttest)
    bool schriftFenster = false;
    std::string schriftTtf, schriftName = "eigene";
    int schriftGroesse = 20;
    int letzterExport = 0;   // Dateien im zuletzt geschriebenen Paket
    bool ziehtText = false;
    std::string meldung;
    std::string ordnerAusgabe;
    // Selbsttest
    int letztePrims = 0;
    int asienPrims = 0;   // M3: Zeichen aus den asiatischen Glyphenseiten im letzten Bild
    int laufMenue = -1;   // M3: das im Bedienmodus gezeichnete Menue (Animationen der Figuren)
    float laufMausX = -1.0F, laufMausY = -1.0F;   // rc610 F46 B6-4: DC->cursorx/y der letzten Mausbewegung (-1: keine)
    int hudPrims = 0;     // M3: Zahlen und Fadenkreuz des HUD im letzten Bild
    std::vector<std::string> klangListe;   // M3 Punkt 3: ausgeloeste Klaenge seit klangListeLeeren
    // M3 Punkt 5: HUD-Menues wie im Spiel (CG_DrawHUD) mit Beispielwerten
    bool hudWie = true;
    HudWerte hudWerte;
    HudZustand hudZustand;
    // M7 Punkt 14: ownerdraws - glGetString des Treibers (einmal gelesen) und Beispielwerte des Spielzustands
    GlDaten glDaten;
    bool glGelesen = false;
    Spielbeispiel spielBeispiel;
    DatapadWerte datapadWerte;
    PazaakZustand pazaakZustand;
    LadeWerte ladeWerte;                         // M7 Punkt 11: Ladebildschirm (CG_DrawInformation)
    VerbindungsWerte verbindungsWerte;           // M7 Punkt 11: Verbindungsbild (UI_DrawConnectScreen)
    std::unique_ptr<Datei> datapadHud;            // ui/hud.menu fuer CG_DrawDataPadHUD (einmal gelesen)
    std::vector<std::string> waffenIcon;          // ext_data/weapons.dat weaponIcon
    bool datapadGelesen = false;
    Spiel datapadFuer = Spiel::JA;                // fuer welches Spiel datapadWerte das Beispiel ist
    Tastenbelegung tasten[2];                    // M7: Vorgabe-Config des Spiels je Dialekt (SP, MP)
    bool tastenGelesen[2] = {false, false};
    Dialekt neuDialekt = Dialekt::SP;            // M7: Format des Startmenues eines neuen Projekts
    std::function<std::string(bool)> projektDialog;   // rc608: Selbsttest statt der Windows-Dateidialoge
    // rc608 Teil 3a: was die rechte Leiste sich merkt (app_menue_leiste.inc); nur Ansicht, nie in der Datei
    struct Leiste {
        std::set<std::string> zu{"logik", "mehr", "original"};   // zugeklappte Gruppen
        std::vector<int> auswahlVorher; // rc608 Teil 3b: anderes Element gewaehlt: die Spalte beginnt wieder oben
        std::string such, suchRoh;      // Feldsuche
        std::string farbeOffen;         // die Farbzeile, deren R, G, B, A offen sind
        std::string hilfe, hilfeNeu;    // Hilfezeile: gezeigt, und was das Feld unter dem Zeiger in diesem Bild meldet
        bool zaehlen = false;           // Gruppe laeuft zum Zaehlen: Felder melden sich nur an
        int treffer = 0, trefferGesamt = 0;
        int markeItem = -1;             // das Element, dessen Felder die Marke "geaendert" tragen
        bool zeileOffen = false;
        ImVec2 zeileAnfang{0, 0};
        float zeileBreite = 0, zeileUnten = 0;
        std::string zeileHilfe;
        bool ziehAktiv = false;         // an der Beschriftung der Zeile wird gezogen
        float ziehDelta = 0;
        std::string trig;               // Events: der gewaehlte Ausloeser
        int auf = 0;                    // Events: der offene Schritt
        bool add = false, addNeu = false;
        std::string addq;
        std::vector<int> auswahl;       // zuletzt gesehene Auswahl
        const Dokument* auswahlDok = nullptr;
        int tablett = 0;                // Fuss: 0 zu, 1 Component, 2 Preview cvars, 3 Engine messages
        std::map<int, Item> vorgabe;    // Item_Init je Dialekt und Spiel
        std::vector<std::string> vorschlaege;
        // "Try on canvas": 1 wartet auf die Flaeche, 2 laeuft
        int probeStufe = 0, probeItem = -1, probeMenu = -1, probenGelaufen = 0;
        bool probeWarAn = false;
        double probeAb = 0;
        std::string probeSkript, probeText;
        // Selbsttest: was das letzte Bild gezeigt hat
        int schluesselGezaehlt = 0, schluesselZeilen = 0, gemischt = 0, gemischtGezeigt = 0;
        TextMass textMass;
    } leiste;
    // rc608 Teil 3a: das Fenster "Bild waehlen" / "Video waehlen" (app_menue_bildwahl.inc)
    struct BildWahl {
        bool oeffnen = false;
        bool istOffen = false;    // Selbsttest: im letzten Bild offen
        int art = 0, item = -1;   // 0 Bild, 1 Video; item < 0: das Menue
        std::string filter;
        std::vector<std::string> liste, gezeigt;
        std::function<void(const std::string&)> ziel;
    } wahl;
    // rc608: das Feld in Properties oder Events, das gerade gezogen oder getippt wird (siehe zahlenFeld)
    struct HaltFeld {
        ImGuiID id = 0;
        std::vector<float> zahlen;
        std::string text;
        ImRect rect;
        int bild = 0;
        std::function<void()> schreiben;
    } halt;
    // M7 Punkt 13: das zuletzt gestartete Testspiel (ui_load nach dem Speichern)
    unsigned long testPid = 0;
    bool testUiLoad = false;
    std::string testOrdnerLose;
    int testNeuGeladen = 0;
    // M7 Punkt 3/12: Komponenten-Name im Eingabefeld, Griff am Modell (0 = keiner, 1 Drehen, 2 Abstand, 3 Hoehe, 4 Blickwinkel)
    std::string kompFuer;   // Datei, Menue, Item und Name, fuer die kompName vorbelegt ist
    std::string kompName;
    int gizmoArt = 0;
    int gizmoItem = -1;
    GizmoWerte gizmoStart, gizmoJetzt;
    ImVec2 gizmoMaus{0, 0};
    int beispielPrims = 0;   // im letzten Bild
    int platzhalterPrims = 0, ladebalken = 0;   // im letzten Bild (Selbsttest)
    std::map<std::string, std::string> textFehlt;   // M3: Sprache -> nicht darstellbare Zeichen der letzten Eingabe
    bool spielListeOffen = false;
    std::string spielFilter;
    std::map<std::string, Texte> layoutTexte;     // M5 Punkt 1: Texte je Sprache (breitester Text)
    std::vector<std::string> layoutWarnungen;     // M5 Punkt 1: letzte Rechnung
    std::vector<std::string> layoutZustaende;     // M5b: Skripte, die nach show/hide die Lage setzen
    Farbe hoverFarbe{1, 0.8F, 0.2F, 1};           // M5 Punkt 2: Farbe der fertigen Hover-Effekte
    // M5b Punkt 3: Projekt (*.jksmenu), Sicherung, Dateizuordnung
    std::string projektDatei;                     // "" = kein Projekt offen
    Projekt projekt;
    std::string projektVorgemerkt;                // Doppelklick/Kommandozeile: im naechsten Bild oeffnen
    double sicherungAm = 0;                       // ImGui-Zeit der letzten Sicherung
    bool sicherungGeprueft = false;               // Sicherung ohne Projekt beim ersten Bild gesucht
    bool sicherungFrage = false;                  // Frage "Wiederherstellen?" oeffnen
    std::string sicherungGefunden;                // Ordner der gefundenen Sicherung
    std::vector<SicherungsEintrag> sicherungListe;
    std::int64_t sicherungZeitpunkt = 0;
    bool zuordnungFrage = false;                  // Frage zur Dateizuordnung oeffnen
    int projektGespeichert = 0;                   // M5b Punkt 6: erfolgreiche Projekt-Speicherungen (Tour)
    // M5b Punkt 4: Mod-Profil (eingebaut nach Spiel oder aus <Einstellungen>/menue_profile, Projektordner)
    std::shared_ptr<const ModProfil> profil;
    std::string profilFuer;                       // Name|Spiel, fuer den profil gilt
    // M6 Punkt 1: Menuefluss (Fenster, Ergebnis je Stand aller Dokumente)
    bool flussOffen = false;
    Fluss fluss;
    std::string flussSchluessel;
    // M6 Punkt 2: Tab-Reihenfolge auf der Flaeche, neue Folge per Klick
    bool tabFolge = false;
    std::vector<int> tabAlt, tabNeu;
    std::string tabSchluessel, tabNeuFuer;
    // M6 Punkt 7: Referenzbild (nur Anzeige, nie im Menue oder in der pk3)
    std::string refPfad, refFehler;
    Textur refTex;
    unsigned refStand = 0;
    float refDeckkraft = 0.5F;
    bool refOben = false;
    // M6 Punkt 4: Stile-Fenster; der Wert, der gerade gezogen/getippt wird (erst beim Loslassen ersetzen)
    bool stileOffen = false;
    // M6 Punkt 6: Einstellungs-Assistent (Katalog aus den Spielmenues, Auswahl als Cvar-Namen klein)
    bool assistentOffen = false;
    int assistentUeberlappung = 0;   // M7: im letzten Bild
    std::string assistentFuer, assistentSuche;
    std::vector<CvarSteuerung> assistentKatalog;
    std::set<std::string> assistentWahl;
    // M6 Punkt 9: Lesbarkeit des Menues der Seite (zwischengespeichert), naechstes Element per Klick
    std::vector<LesbarkeitsBefund> lesbar;
    std::string lesbarSchluessel;
    int lesbarNaechster = -1;
    // M6 Punkt 8: Originaldatei des Spiels zum aktiven Dokument
    std::string originalFuer;
    Datei original;
    bool originalDa = false;
    std::string stilAktiv, stilText;
    float stilFarbe[4] = {0, 0, 0, 1};
    // Rest M5b: Reiter mit ungespeicherten Aenderungen schliessen - Rueckfrage (-1 = keine)
    int schliessenDok = -1;
};
std::shared_ptr<const ModProfil> aktivesProfil();   // M5b (app_menue_projekt.inc)
bool profilEingebaut(const ModProfil& p);
// Leseoptionen fuer einen Spielpfad mit dem Mod-Profil
LeseOptionen leseOptionen(const std::string& pfad);

// InputText mit std::string (wie misc/cpp/imgui_stdlib.cpp)
int waechst(ImGuiInputTextCallbackData* d) {
    if (d->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* t = static_cast<std::string*>(d->UserData);
        t->resize(static_cast<std::size_t>(d->BufTextLen));
        d->Buf = t->data();
    }
    return 0;
}
bool eingabe(const char* id, std::string* s, ImGuiInputTextFlags f = 0, const char* hinweis = nullptr) {
    if (hinweis != nullptr) {
        return ImGui::InputTextWithHint(id, hinweis, s->data(), s->capacity() + 1, f | ImGuiInputTextFlags_CallbackResize, waechst, s);
    }
    return ImGui::InputText(id, s->data(), s->capacity() + 1, f | ImGuiInputTextFlags_CallbackResize, waechst, s);
}

std::string u8(const fs::path& p) {
    const std::u8string t = p.u8string();
    return std::string(t.begin(), t.end());
}

constexpr int kTextZoomMin = 50, kTextZoomMax = 300;   // rc608 Teil 3a
ImFont* g_codeSchrift = nullptr;   // Festbreitenschrift (main_win32.cpp buildFonts)

Zustand* z_ = nullptr;
Zustand& Z() {
    if (z_ == nullptr) { z_ = new Zustand(); }
    return *z_;
}

Dokument* doc() {
    Zustand& z = Z();
    return (z.aktiv >= 0 && static_cast<std::size_t>(z.aktiv) < z.docs.size()) ? z.docs[static_cast<std::size_t>(z.aktiv)].get()
                                                                              : nullptr;
}

std::string anzeige(std::string_view s);    // Latin-1 -> UTF-8 (unten)
std::string utf8ZuLatin1(const char* s);    // UTF-8 -> Latin-1 (unten)

// --- Spieldaten, Texte, Schriften, Shader ------------------------------------

void datenPruefen() {
    Zustand& z = Z();
    const int rw = regelwerkSpielErmitteln();
    z.spiel = rw == kRegelwerkMovieDuels ? Spiel::MD : rw == kRegelwerkJediOutcast ? Spiel::JK2 : Spiel::JA;
    std::size_t n = 0;
    for (const GamePath& g : g_app->gamePaths) { n += g.archives.size() + 1; }
    if (n == z.datenStand) { return; }
    z.datenStand = n;
    z.daten.ordner = g_app->gamePaths;   // nur lesen
    z.texteGeladen = z.shaderGeladen = z.schriftNamenGeladen = false;
    z.feederSp.gebaut = z.feederMp.gebaut = false;
    z.sprachenGeladen = false;
    ++z.texteVersion;
    z.saeberGeladen = false;
    z.datapadGelesen = false;
    z.tastenGelesen[0] = z.tastenGelesen[1] = false;
    z.schriften.clear();
    z.texturen.clear();
}

const Texte& texte() {
    Zustand& z = Z();
    if (!z.texteGeladen) {
        z.texte = ladeTexte(z.daten, z.spiel, z.sprache);
        z.texteGeladen = true;
    }
    return z.texte;
}

bool gleichOhneFallStr(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}

std::string kleinPfad(std::string s) {
    for (char& c : s) { c = c == '\\' ? '/' : static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

const Schrift* schriftNamens(const std::string& name) {
    if (name.empty()) { return nullptr; }
    Zustand& z = Z();
    const auto it = z.schriften.find(name);
    if (it != z.schriften.end()) { return it->second.ok ? &it->second : nullptr; }
    Schrift s;
    std::string daten;
    bool imPaket = false;
    if (Dokument* d = doc()) {   // M2 Punkt 9: eigene Schrift im Paket
        const auto f = d->bilder.find("fonts/" + name + ".fontdat");
        if (f != d->bilder.end()) {
            imPaket = ladeFontdat(name, f->second, s);
        }
    }
    if (!imPaket && z.daten.lies("fonts/" + name + ".fontdat", daten)) { (void)ladeFontdat(name, daten, s); }
    s.name = name;
    s.shader = "fonts/" + name;
    z.schriften[name] = s;
    return z.schriften[name].ok ? &z.schriften[name] : nullptr;
}

// Die Schriftnamen aus assetGlobalDef (SP ui/main.menu, MP ui/jamp/main.menu)
// Schriften und fade-Werte wie beim Laden der Menues: der Menuesatz des Spiels
// (SP UI_Load: ui/menus.txt; MP ui_menuFilesMP, Vorgabe ui/jampmenus.txt - Jedi Outcast
// und Movie Duels tragen eigene Dateinamen) Datei fuer Datei, jede mit ihrem
// assetGlobalDef und ihren menuDefs.
void schriftNamenLaden() {
    Zustand& z = Z();
    if (z.schriftNamenGeladen) { return; }
    z.schriftNamenGeladen = true;
    auto aus = [&](const std::vector<std::string>& saetze, Dialekt d) {
        std::vector<Datei> dateien;
        for (const std::string& satzPfad : saetze) {
            std::string t;
            if (!z.daten.lies(satzPfad, t)) { continue; }
            LeseOptionen o = leseOptionen(satzPfad);
            o.dialekt = d;
            const Datei satz = lies(satzPfad, t, o);
            for (const std::string& f : satz.loadMenus) {
                std::string inhalt;
                if (!z.daten.lies(f, inhalt)) { continue; }
                LeseOptionen of = leseOptionen(f);
                of.dialekt = d;
                dateien.push_back(lies(f, inhalt, of));
            }
            break;   // nur der erste vorhandene Satz (ui_menuFiles)
        }
        std::vector<const Datei*> zeiger;
        const int k = d == Dialekt::MP ? 1 : 0;
        for (const Datei& f : dateien) {
            zeiger.push_back(&f);
            for (const Eigenschaft& e : f.assets.eigenschaften) {
                if (!e.ok || e.args.empty()) { continue; }
                const float v = static_cast<float>(std::atof(e.args[0].s.c_str()));
                if (e.schluessel == "fadeClamp") { z.fadeClamp[k] = v; }
                else if (e.schluessel == "fadeAmount") { z.fadeAmount[k] = v; }
                else if (e.schluessel == "fadeCycle") { z.fadeCycle[k] = static_cast<int>(v); }
                else if (e.schluessel == "itemFocusSound") { z.fokusKlang[k] = e.args[0].s; }
            }
        }
        if (zeiger.empty()) { return std::vector<std::string>{}; }
        return schriftNamen(zeiger, d, z.spiel, [&](const std::string& n) { return z.daten.gibt("fonts/" + n + ".fontdat"); });
    };
    // M5b: Menuelisten aus dem Mod-Profil. rc608 Teil 3a: auch bei den eingebauten: Movie Duels MP liest ui/MovieDuels-MP-Menus.txt,
    // und dort heisst font 2 albertus_bold und font 3 ergoec (im Spiel fotografiert); mit der Liste von Jedi Academy zeigte die Vorschau andere Schriften.
    const std::shared_ptr<const ModProfil> pr = aktivesProfil();
    const ModProfil& listen = *pr;
    z.schriftNamenSp = aus(menuelisten(listen, Dialekt::SP), Dialekt::SP);
    z.schriftNamenMp = aus(menuelisten(listen, Dialekt::MP), Dialekt::MP);
}

void shaderLaden();
bool bildRegistriert(const std::string& s);
const Textur* textur(const std::string& shaderName) {
    Zustand& z = Z();
    if (shaderName.empty()) { return nullptr; }
    if (z.texturStand != render::texturStand()) {
        z.texturen.clear();
        z.texturStand = render::texturStand();
    }
    std::string key = kleinPfad(shaderName);
    const std::size_t punkt = key.rfind('.');
    if (punkt != std::string::npos && punkt + 5 >= key.size() && key.find('/', punkt) == std::string::npos) { key = key.substr(0, punkt); }
    Textur& t = z.texturen[key];
    if (t.versucht) { return t.tex != nullptr ? &t : nullptr; }
    t.versucht = true;
    if (key == kDefaultShader) {
        // M5: R_CreateDefaultImage - 16x16, Grau 32 mit weissem Rand; der "<default>"-Shader zeichnet
        // deckend (GLS_DEFAULT), also Alpha 255
        std::vector<unsigned char> px(16 * 16 * 4, 32);
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                unsigned char* q = &px[static_cast<std::size_t>((y * 16 + x) * 4)];
                if (x == 0 || y == 0 || x == 15 || y == 15) { q[0] = q[1] = q[2] = 255; }
                q[3] = 255;
            }
        }
        t.tex = render::createTexture(px.data(), 16, 16);
        t.w = t.h = 16;
        return t.tex != nullptr ? &t : nullptr;
    }
    shaderLaden();
    std::string bild = key;
    const auto s = z.shader.find(key);
    if (s != z.shader.end() && !s->second.image.empty()) {
        bild = kleinPfad(s->second.image);
        t.additiv = s->second.blend == BlendMode::Add || s->second.blend == BlendMode::AddAlpha;
    }
    std::string bytes;
    bool da = false;
    Dokument* d = doc();
    for (const std::string& k : textureCandidates(bild)) {
        if (d != nullptr) {
            for (const auto& [pfad, inhalt] : d->bilder) {
                if (kleinPfad(pfad) == kleinPfad(k)) {
                    bytes = inhalt;
                    da = true;
                }
            }
        }
        if (!da && z.daten.lies(k, bytes)) { da = true; }
        if (da) { break; }
    }
    if (!da) { return nullptr; }
    const image::Image im = image::decode(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
    if (!im.ok || im.width <= 0 || im.height <= 0) { return nullptr; }
    t.tex = render::createTexture(im.rgba.data(), im.width, im.height);
    t.w = im.width;
    t.h = im.height;
    return t.tex != nullptr ? &t : nullptr;
}

// M2: UI_SaberModelForSaber (code/ui/ui_main.cpp): saberModel aus den .sab-Dateien
bool saberModellFuer(const std::string& saber, std::string& modell, std::string& haut) {
    Zustand& z = Z();
    if (!z.saeberGeladen) {
        z.saeberGeladen = true;
        z.saeber.clear();
        for (const std::string& f : z.daten.liste("ext_data/sabers/", {".sab"})) {
            std::string t;
            if (z.daten.lies(f, t)) { parseSaberFile(t, z.saeber); }
        }
    }
    const auto it = z.saeber.find(kleinPfad(saber));
    if (it == z.saeber.end() || it->second.model.empty()) { return false; }
    modell = it->second.model;
    haut.clear();   // customSkin traegt SaberDef nicht: Grundhaut des Modells
    return true;
}

// --- Dokumente ---------------------------------------------------------------

// rc608 Teil 3b: Auge und Schloss haengen am Element, nicht an seiner Nummer: Sie ziehen ueber Name und Vorkommen mit,
// wenn sich die Reihenfolge aendert. Umbenannt (gleich viele Elemente): gleiche Stelle. Sonst ist das Element weg.
std::pair<int, int> elementImNeuen(const Datei& alt, const Datei& neu, int mi, int i) {
    if (mi < 0 || static_cast<std::size_t>(mi) >= alt.menus.size()) { return {-1, -1}; }
    const Menu& ma = alt.menus[static_cast<std::size_t>(mi)];
    if (i < 0 || static_cast<std::size_t>(i) >= ma.items.size()) { return {-1, -1}; }
    int mn = -1;
    for (std::size_t k = 0; k < neu.menus.size() && mn < 0; ++k) {
        if (neu.menus[k].window.name == ma.window.name) { mn = static_cast<int>(k); }
    }
    if (mn < 0 && static_cast<std::size_t>(mi) < neu.menus.size()) { mn = mi; }
    if (mn < 0) { return {-1, -1}; }
    const Menu& mb = neu.menus[static_cast<std::size_t>(mn)];
    const std::string& name = ma.items[static_cast<std::size_t>(i)].window.name;
    int nte = 0;
    for (int k = 0; k < i; ++k) { nte += ma.items[static_cast<std::size_t>(k)].window.name == name ? 1 : 0; }
    if (!name.empty()) {
        for (std::size_t k = 0; k < mb.items.size(); ++k) {
            if (mb.items[k].window.name == name && nte-- == 0) { return {mn, static_cast<int>(k)}; }
        }
    }
    return ma.items.size() == mb.items.size() ? std::pair<int, int>{mn, i} : std::pair<int, int>{-1, -1};
}

// rc609 ME1 (Tester: "it crashed when I deleted a button"): Was ein Element im neuen Text ist, sagt zuerst die Stelle seines
// "itemDef": Sie wandert mit jeder Aenderung genau mit (Textpuffer::stelleSeit), auch beim Loeschen davor und bei Rueckgaengig.
// Nur ein ersetztes oder eingebundenes Element sucht Name und Vorkommen, und nie eines, das schon seinen Vorgaenger hat.
std::map<std::pair<int, int>, std::pair<int, int>> nachfolger(const Dokument& d, const Datei& alt, int altVersion) {
    std::map<std::pair<int, int>, std::pair<int, int>> neu;
    std::map<std::uint32_t, std::pair<int, int>> anStelle;
    for (std::size_t m = 0; m < d.datei.menus.size(); ++m) {
        const std::vector<Item>& items = d.datei.menus[m].items;
        for (std::size_t k = 0; k < items.size(); ++k) {
            if (items[k].kopf.datei == 0) { anStelle.emplace(items[k].anfang, std::pair<int, int>{static_cast<int>(m), static_cast<int>(k)}); }
        }
    }
    std::set<std::pair<int, int>> belegt;
    std::vector<std::pair<int, int>> offen;
    for (std::size_t m = 0; m < alt.menus.size(); ++m) {
        const std::vector<Item>& items = alt.menus[m].items;
        for (std::size_t k = 0; k < items.size(); ++k) {
            const std::pair<int, int> a{static_cast<int>(m), static_cast<int>(k)};
            std::uint32_t off = items[k].anfang;
            if (items[k].kopf.datei == 0 && d.puffer.stelleSeit(altVersion, off) == Textpuffer::Stelle::Da) {
                const auto f = anStelle.find(off);
                if (f != anStelle.end()) {
                    neu[a] = f->second;
                    belegt.insert(f->second);
                    continue;
                }
            }
            offen.push_back(a);
        }
    }
    for (const std::pair<int, int>& a : offen) {
        const std::pair<int, int> n = elementImNeuen(alt, d.datei, a.first, a.second);
        if (n.first >= 0 && belegt.insert(n).second) { neu[a] = n; }
    }
    return neu;
}

// Schloss, Auge, Anker und Umbenennen ziehen mit ihren Elementen; was geloescht ist, verliert sie. mi: das Menue der Seite jetzt.
void merkerNachziehen(Dokument& d, const Datei& alt, int altVersion, int altSeitenMenue, int mi) {
    const std::map<std::pair<int, int>, std::pair<int, int>> neu = nachfolger(d, alt, altVersion);
    d.abgebildet.clear();
    for (const auto& [a, n] : neu) { d.abgebildet.insert(n); }
    const auto ziel = [&neu](int m, int i) {
        const auto f = neu.find({m, i});
        return f != neu.end() ? f->second : std::pair<int, int>{-1, -1};
    };
    std::map<int, std::set<int>> zu = d.gesperrtAndere;
    if (!d.gesperrt.empty()) { zu[d.gesperrtMenue] = d.gesperrt; }
    d.gesperrt.clear();
    d.gesperrtAndere.clear();
    for (const auto& [m, menge] : zu) {
        for (const int i : menge) {
            const auto [mn, k] = ziel(m, i);
            if (mn < 0) { continue; }
            if (mn == mi) { d.gesperrt.insert(k); } else { d.gesperrtAndere[mn].insert(k); }
        }
    }
    d.gesperrtMenue = mi;
    std::set<std::pair<int, int>> versteckt;
    for (const auto& [m, i] : d.verborgen) {
        const std::pair<int, int> n = ziel(m, i);
        if (n.first >= 0) { versteckt.insert(n); }
    }
    d.verborgen = std::move(versteckt);
    const std::pair<int, int> anker = ziel(altSeitenMenue, d.hierarchieAnker);
    d.hierarchieAnker = anker.first == mi ? anker.second : -1;
    const std::pair<int, int> name = ziel(d.umbenennenMenu, d.umbenennen);
    d.umbenennenMenu = name.first;
    d.umbenennen = name.second;
}

// Die Seite zeigt ein anderes Menue: dessen Schloesser gelten
void schlossAnSeite(Dokument& d, int mi) {
    if (d.gesperrtMenue == mi) { return; }
    if (!d.gesperrt.empty()) { d.gesperrtAndere[d.gesperrtMenue] = d.gesperrt; }
    d.gesperrt.clear();
    const auto it = d.gesperrtAndere.find(mi);
    if (it != d.gesperrtAndere.end()) {
        d.gesperrt = it->second;
        d.gesperrtAndere.erase(it);
    }
    d.gesperrtMenue = mi;
}

int menuDerSeite(const Dokument& d);

void neuParsen(Dokument& d) {
    if (d.geparst == d.puffer.version()) { return; }
    LeseOptionen o = leseOptionen(d.name);
    const int altSeitenMenue = menuDerSeite(d);
    const int altVersion = d.geparst;
    d.altStaende.push_back(std::move(d.datei));
    d.datei = lies(d.name, d.puffer.text(), o);
    d.geparst = d.puffer.version();
    if (d.datei.menus.empty()) { d.seite = 0; }
    const std::vector<Seite> s = seitenImMenue(d.datei);
    d.seite = std::clamp(d.seite, 0, std::max(0, static_cast<int>(s.size()) - 1));
    // rc608: Rueckgaengig nach dem Einfuegen oder Tippen im Text-Editor kann das gewaehlte Item nehmen (war ein Absturz)
    const int mi = s.empty() ? -1 : s[static_cast<std::size_t>(d.seite)].menu;
    merkerNachziehen(d, d.altStaende.back(), altVersion, altSeitenMenue, mi);
    const int anzahl = mi < 0 ? 0 : static_cast<int>(d.datei.menus[static_cast<std::size_t>(mi)].items.size());
    d.auswahl.erase(std::remove_if(d.auswahl.begin(), d.auswahl.end(), [anzahl](int i) { return i < 0 || i >= anzahl; }), d.auswahl.end());
}

int menuDerSeite(const Dokument& d) {
    const std::vector<Seite> s = seitenImMenue(d.datei);
    if (s.empty()) { return -1; }
    return s[static_cast<std::size_t>(std::clamp(d.seite, 0, static_cast<int>(s.size()) - 1))].menu;
}

// rc610 F46 B6-1: Seitenwechsel aus dem Code (Skript im Bedienen-Modus, Hierarchie, Gliederung, Diagnose) wie beim
// Reiter (zeichneSeiten): eine andere Seite leert die Auswahl - deren Indizes gehoerten zum Menue der alten Seite.
void zeigeSeite(Dokument& d, int seite) {
    if (seite == d.seite) { return; }
    d.seite = seite;
    d.auswahl.clear();
}

// Die Seite eines ganzen Menues (ohne cvar-Unterseite) zeigen
void zeigeMenue(Dokument& d, int menu) {
    const std::vector<Seite> s = seitenImMenue(d.datei);
    int ziel = -1;
    for (std::size_t k = 0; k < s.size(); ++k) {
        if (s[k].menu == menu && s[k].cvar.empty()) { ziel = static_cast<int>(k); }
    }
    if (ziel >= 0) { zeigeSeite(d, ziel); }
}

bool oeffne(const std::string& name, const std::string& platte, const std::string& text) {
    Zustand& z = Z();
    for (std::size_t i = 0; i < z.docs.size(); ++i) {
        if (z.docs[i]->name == name && z.docs[i]->platte == platte) {
            z.aktiv = static_cast<int>(i);
            return true;
        }
    }
    auto d = std::make_unique<Dokument>();
    d->name = name;
    d->platte = platte;
    d->puffer.setzeText(text);
    d->geladen = d->gespeichert = text;
    z.docs.push_back(std::move(d));
    z.aktiv = static_cast<int>(z.docs.size()) - 1;
    neuParsen(*doc());
    return true;
}

// Ein Spielpfad zu einer Datei auf der Platte: ab ".../ui/" (sonst ui/<name>);
// mit Schraegstrichen wie die Pfade des Spiels (FS_ReplaceSeparators gilt nur fuer die Platte)
std::string spielPfadVon(const std::string& platte) {
    const std::string k = kleinPfad(platte);
    const std::size_t p = k.rfind("/ui/");
    if (p != std::string::npos) {
        std::string s = platte.substr(p + 1);
        std::replace(s.begin(), s.end(), '\\', '/');
        return s;
    }
    return "ui/" + u8(utf8Pfad(platte).filename());
}

bool imSpielordner(const std::string& pfad) {
    std::error_code ec;
    const std::string k = kleinPfad(fs::weakly_canonical(fs::path(pfad), ec).string());
    auto pruefe = [&](const std::string& o) {
        if (o.empty()) { return false; }
        std::string g = kleinPfad(fs::weakly_canonical(fs::path(o), ec).string());
        // GameData-Ordner (eins ueber base) ebenfalls schuetzen
        const std::string eltern = kleinPfad(fs::path(g).parent_path().string());
        // rc610 F46 B6-5: nur der Ordner selbst und was darunter liegt (Trennzeichen danach wie loseImModOrdner) -
        // "GameData_x" neben "GameData" ist kein Spielordner
        const auto unter = [&k](const std::string& o) {
            return !o.empty() && k.compare(0, o.size(), o) == 0 && (k.size() == o.size() || o.back() == '/' || k[o.size()] == '/');
        };
        return unter(g) || unter(eltern);
    };
    for (const std::string& o : g_app->settings.gamePaths) {
        if (pruefe(o)) { return true; }
    }
    for (const std::string& o : g_app->settings.gamePathsJk2) {
        if (pruefe(o)) { return true; }
    }
    return false;
}

// rc609 ME2 (shank 04.11.: "Der Menue-Editor darf ein Menue, das LOSE in einem Mod-Ordner (oder base) unter GameData
// liegt, an seinen Ort zurueckspeichern - mit Sicherung"): eine vorhandene lose Datei in einem Unterordner des
// GameData-Ordners (base, MD, jeder Mod-Ordner). Gesperrt bleiben der Programmordner selbst (exe, dll) und die .pk3
// des Spiels: der Editor ersetzt keine .pk3, und eine lose Datei daneben wirkt nicht (fs_dirbeforepak 0, Befund W2a).
std::string kanonisch(const std::string& pfad) {
    std::error_code ec;
    std::string k = kleinPfad(u8(fs::weakly_canonical(utf8Pfad(pfad), ec)));
    while (k.size() > 1 && k.back() == '/') { k.pop_back(); }
    return k;
}
bool loseImModOrdner(const std::string& pfad) {
    std::error_code ec;
    if (!fs::is_regular_file(utf8Pfad(pfad), ec) || kleinPfad(u8(utf8Pfad(pfad).extension())) == ".pk3") { return false; }
    const std::string k = kanonisch(pfad);
    auto unterMod = [&](const std::string& o) {
        if (o.empty()) { return false; }
        std::string gameData = kanonisch(o);
        gameData.erase(std::min(gameData.size(), gameData.rfind('/')));   // eins ueber base bzw. dem Mod-Ordner
        if (gameData.empty() || k.size() <= gameData.size() + 1 || k.compare(0, gameData.size(), gameData) != 0 || k[gameData.size()] != '/') {
            return false;
        }
        return k.find('/', gameData.size() + 1) != std::string::npos;   // in einem Unterordner, nicht im Programmordner
    };
    for (const std::string& o : g_app->settings.gamePaths) {
        if (unterMod(o)) { return true; }
    }
    for (const std::string& o : g_app->settings.gamePathsJk2) {
        if (unterMod(o)) { return true; }
    }
    return false;
}

bool schreibeDatei(const std::string& pfad, const std::string& inhalt) {
    if (imSpielordner(pfad)) {
        Z().meldung = tr(Str::MeNieSpielordner);
        return false;
    }
    std::ofstream f(utf8Pfad(pfad), std::ios::binary);
    if (!f) { return false; }
    f.write(inhalt.data(), static_cast<std::streamsize>(inhalt.size()));
    return static_cast<bool>(f);
}

void imLaufendenSpielNeuLaden();   // M7 Punkt 13 (unten)

// rc609 ME2: Liest das Spiel fuer diesen Spielpfad etwas anderes als die Datei ziel (eine .pk3 im selben Ordner,
// fs_dirbeforepak 0, oder einen spaeteren Ordner)? Dann dessen Ort, sonst "". Nur, wenn ziel in einem Spielordner an
// genau diesem Spielpfad liegt - sonst liest das Spiel die Datei ohnehin nicht.
std::string verdecktVon(const std::string& spielPfad, const std::string& ziel) {
    datenPruefen();
    const Zustand& z = Z();
    std::string wo;
    bool lose = false;
    if (!z.daten.herkunft(spielPfad, wo, lose)) { return {}; }
    const std::string k = kanonisch(ziel);
    if (lose && kanonisch(wo) == k) { return {}; }
    for (const GamePath& o : z.daten.ordner) {
        if (!o.directory.empty() && kanonisch(u8(utf8Pfad(o.directory) / utf8Pfad(spielPfad))) == k) { return wo; }
    }
    return {};
}

void speichern(Dokument& d, const std::string& ziel) {
    char buf[600];
    const bool ok = schreibeDatei(ziel, d.puffer.text());
    if (ok) {
        d.platte = ziel;
        d.gespeichert = d.puffer.text();
        std::snprintf(buf, sizeof(buf), tr(Str::MeGespeichert), ziel.c_str());
    } else {
        std::snprintf(buf, sizeof(buf), tr(Str::MeNichtGespeichert), ziel.c_str());
    }
    Z().meldung = buf;
    addStatus(buf);
    if (ok) { imLaufendenSpielNeuLaden(); }
}

// rc609 ME2: an den Ort im Mod-Ordner. Die alte Datei wird <ziel>.bak (der Stand vor diesem Speichern, wie W2a
// sicherungAnlegenD), dann ersetzt sicherSchreiben die Datei atomar - haelt das laufende Spiel sie offen, bleibt sie.
void speichernAmOrt(Dokument& d, const std::string& ziel) {
    Zustand& z = Z();
    char buf[1600];
    std::error_code ec;
    fs::copy_file(utf8Pfad(ziel), utf8Pfad(ziel + ".bak"), fs::copy_options::overwrite_existing, ec);
    SchreibFehler art = SchreibFehler::Keiner;
    std::string fehler;
    if (ec || !sicherSchreiben(ziel, d.puffer.text(), &fehler, &art)) {
        std::snprintf(buf, sizeof(buf), tr(art == SchreibFehler::Gesperrt ? Str::DoGesperrt : Str::MeNichtGespeichert), (ec ? ziel + ".bak" : ziel).c_str());
        z.meldung = buf;
        addStatus(buf);
        return;
    }
    d.platte = ziel;
    d.gespeichert = d.puffer.text();
    std::snprintf(buf, sizeof(buf), tr(Str::MeOrtGespeichert), ziel.c_str());
    z.meldung = buf;
    const std::string vor = verdecktVon(d.name, ziel);
    if (!vor.empty()) {
        std::snprintf(buf, sizeof(buf), tr(Str::MeOrtVerdeckt), vor.c_str());
        z.meldung += "  " + std::string(buf);
    }
    // Die Leiste hat nur eine Zeile: Ort, .bak und Hinweis ganz in der Einblendung (wie W2a), mit Knopf zum Ordner
    ::bhed::gui::meldung(z.meldung, directoryOf(ziel), !vor.empty());
    imLaufendenSpielNeuLaden();
}

// --- Aenderungen an Text und Flaeche ------------------------------------------

// rc608 (Tester: "I move a button, but when I click off, it resets"): die Felder in Properties und Events
// lesen ihren Wert in jedem Bild neu aus dem Text. Was gezogen oder getippt wird, lebt darum hier, bis es
// fertig ist: Maustaste los, Enter, Klick daneben. Dann wird es geschrieben, als ein Rueckgaengig-Schritt.
void haltSchreiben();

// Ein Doppelklick macht aus einem Zugfeld ein Eingabefeld, das seinen Wert erst beim Verlassen abliefert. Nimmt ihm
// ein Klick auf die Flaeche die Aktivitaet, bevor es gezeichnet wird, liefert es gar nichts ab. Darum hier mitlesen.
// k und n wie beim Feld; true, wenn sich ein Wert aendert.
bool getipptLesen(const char* k, float* v, int n) {
    const ImGuiID aktiv = GImGui->ActiveId;
    const ImGuiInputTextState* st = ImGui::GetInputTextState(aktiv);
    if (st == nullptr) { return false; }
    int stelle = n == 1 ? 0 : -1;
    if (n > 1) {   // DragFloatN: PushID(k), PushID(i), ""; ColorEdit: PushID(k), "##X" ...
        static const char* const achsen[] = {"##X", "##Y", "##Z", "##W"};
        ImGui::PushID(k);
        for (int i = 0; i < n && i < 4; ++i) {
            ImGui::PushID(i);
            if (ImGui::GetID("") == aktiv) { stelle = i; }
            ImGui::PopID();
            if (ImGui::GetID(achsen[i]) == aktiv) { stelle = i; }
        }
        ImGui::PopID();
    }
    if (stelle < 0) { return false; }
    const char* text = st->TextA.Data;
    char* ende = nullptr;
    const float x = text != nullptr ? std::strtof(text, &ende) : 0.0F;
    if (text == nullptr || ende == text || x == v[stelle]) { return false; }
    v[stelle] = x;
    return true;
}

template <class Zeichnen, class Schreiben>
void zahlenFeld(const char* k, float* v, int n, Zeichnen zeichne, Schreiben schreiben) {
    Zustand& z = Z();
    const ImGuiID id = ImGui::GetID(k);
    if (z.halt.id == id) { std::copy_n(z.halt.zahlen.begin(), n, v); }
    const bool esc = ImGui::IsKeyPressed(ImGuiKey_Escape);
    bool neu = zeichne();
    const bool aktiv = ImGui::IsItemActive();
    if (aktiv && getipptLesen(k, v, n)) { neu = true; }
    if (esc && z.halt.id == id) {   // Esc: wie vorher, nichts schreiben
        z.halt = {};
        return;
    }
    if (neu) {
        z.halt.id = id;
        z.halt.zahlen.assign(v, v + n);
        z.halt.schreiben = [schreiben, werte = z.halt.zahlen] { schreiben(werte); };
    }
    if (z.halt.id == id) {
        z.halt.rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        z.halt.bild = ImGui::GetFrameCount();
        if (!aktiv && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) { haltSchreiben(); }
    }
}

template <class Schreiben>
void textFeld(const char* k, std::string* wert, Schreiben schreiben, const char* hinweis = nullptr) {
    Zustand& z = Z();
    const ImGuiID id = ImGui::GetID(k);
    if (z.halt.id == id) { *wert = z.halt.text; }
    const bool neu = eingabe(k, wert, 0, hinweis);
    const bool aktiv = ImGui::IsItemActive();
    if (neu) {
        z.halt.id = id;
        z.halt.text = *wert;
        z.halt.schreiben = [schreiben, t = *wert] { schreiben(t); };
    }
    if (z.halt.id == id) {
        z.halt.rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        z.halt.bild = ImGui::GetFrameCount();
        if (!aktiv && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) { haltSchreiben(); }
    }
}

// rc608 Punkte 3, 13, 14, Teil 3a: Bausteine fuer Properties und Events. Die Beschriftung steht vorn, im schmalen Feld darueber; das Feld nimmt den Rest.
void aenderungsMarke(const char* keys, ImVec2 stelle, float g);   // app_menue_leiste.inc
void gruppe(const char* id, const char* titel, const std::string& zus, const std::function<void()>& inhalt, bool zaehlbar = true);

// Ende einer Zeile: Wo der Zeiger in ihr steht, sagt die Hilfezeile, was sie bewirkt. Eine lange Beschriftung bestimmt die Hoehe mit.
void lsZeilenEnde() {
    Zustand::Leiste& l = Z().leiste;
    if (!l.zeileOffen) { return; }
    l.zeileOffen = false;
    const float y = ImGui::GetCursorScreenPos().y;
    if (y < l.zeileUnten) { ImGui::Dummy(ImVec2{1.0F, l.zeileUnten - y}); }
    const ImVec2 m = ImGui::GetIO().MousePos;
    if (l.hilfeNeu.empty() && !l.zeileHilfe.empty() && m.x >= l.zeileAnfang.x && m.x < l.zeileAnfang.x + l.zeileBreite && m.y >= l.zeileAnfang.y &&
        m.y < ImGui::GetCursorScreenPos().y && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
        l.hilfeNeu = l.zeileHilfe;
    }
}

// label: was der Anfaenger liest. key: Schluessel der Datei ("a, b" fuer mehrere), er bleibt im Tooltip und mit "File keys" sichtbar; nullptr: Zeile ohne Schluessel,
// die auch die Feldsuche nicht betrifft. false: die Zeile faellt weg (Suche), das Feld wird nicht gezeichnet. ziehbar: Ziehen an der Beschriftung veraendert die Zahl.
bool feldKopf(const char* label, const char* key = nullptr, const char* tip = nullptr, bool ziehbar = false) {
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    const bool trifft = l.such.empty() || key == nullptr || kleinPfad(std::string(label) + " " + key).find(l.such) != std::string::npos;
    if (l.zaehlen) {
        if (key != nullptr) {
            std::string w;
            for (const char* c = key;; ++c) {
                if (*c == ',' || *c == ' ' || *c == '\0') {
                    if (!w.empty()) { z.felderGezeigt.push_back(w); }
                    w.clear();
                    if (*c == '\0') { break; }
                } else {
                    w += *c;
                }
            }
        }
        if (trifft) { ++l.treffer; }
        return false;
    }
    if (!trifft) { return false; }
    lsZeilenEnde();
    const float fs = ImGui::GetFontSize();
    const ImGuiStyle& st = ImGui::GetStyle();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const float startX = ImGui::GetCursorPosX();
    const float breit = ImGui::GetContentRegionAvail().x;
    const bool mitKey = key != nullptr && key[0] != '\0';
    l.zeileOffen = true;
    l.zeileAnfang = start;
    l.zeileBreite = breit;
    l.zeileHilfe = tip == nullptr ? std::string(mitKey ? key : "") : (mitKey ? std::string(key) + ": " + tip : std::string(tip));
    l.ziehAktiv = false;
    l.ziehDelta = 0.0F;
    const bool zweiSpalten = breit >= fs * 25.0F;
    const float spalte = breit >= fs * 40.0F ? fs * 9.3F : fs * 7.9F;
    const float marke = std::floor(fs * 0.95F);
    ImGui::BeginGroup();
    ImGui::PushTextWrapPos(zweiSpalten ? startX + spalte - st.ItemSpacing.x - marke : startX + breit - marke - st.ItemSpacing.x);
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0xA6, 0xA9, 0xB1, 255));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    if (mitKey && g_app->settings.menueDateiSchluessel) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0x94, 0xBF, 0xE9, 255));
        ImGui::TextUnformatted(key);
        ImGui::PopStyleColor();
        ++l.schluesselGezaehlt;
    }
    ImGui::PopTextWrapPos();
    ImGui::EndGroup();
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    l.zeileUnten = b.y;
    if (!l.zeileHilfe.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) { ImGui::SetTooltip("%s", l.zeileHilfe.c_str()); }
    if (ziehbar) {
        ImGui::SetCursorScreenPos(a);
        ImGui::PushID(label);
        ImGui::InvisibleButton("##zieh", ImVec2{std::max(1.0F, b.x - a.x), std::max(1.0F, b.y - a.y)});
        ImGui::PopID();
        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) { ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); }
        l.ziehAktiv = ImGui::IsItemActive();
        l.ziehDelta = l.ziehAktiv ? ImGui::GetIO().MouseDelta.x : 0.0F;
    }
    const ImVec2 markeOrt{start.x + (zweiSpalten ? spalte - st.ItemSpacing.x - marke : breit - marke), start.y + std::floor((ImGui::GetFrameHeight() - marke) * 0.5F)};
    if (zweiSpalten) { ImGui::SetCursorScreenPos(ImVec2{start.x + spalte, start.y}); }
    else { ImGui::SetCursorScreenPos(ImVec2{start.x, b.y + st.ItemSpacing.y}); }
    if (mitKey) { aenderungsMarke(key, markeOrt, marke); }
    ImGui::SetNextItemWidth(-FLT_MIN);
    return true;
}

// Eine Reihe beschrifteter Zahlen, je 2 nebeneinander wie Rechteck und RGBA, im ganz schmalen Feld untereinander. Klick ins Feld tippt, Ziehen an der
// Beschriftung veraendert. Die Kennungen passen zu getipptLesen.
bool zahlenReihe(const char* k, float* v, int n, const char* const* teile, float tempo, const char* format) {
    static const char* const achsen[] = {"##X", "##Y", "##Z", "##W"};
    static const char* const griffe[] = {"##ziehX", "##ziehY", "##ziehZ", "##ziehW"};
    const float fs = ImGui::GetFontSize();
    const ImGuiStyle& st = ImGui::GetStyle();
    bool neu = false;
    ImGui::PushID(k);
    ImGui::BeginGroup();
    const float breit = std::max(fs * 4.0F, ImGui::GetContentRegionAvail().x);
    float beschriftung = 0.0F;
    for (int i = 0; i < n; ++i) { beschriftung = std::max(beschriftung, ImGui::CalcTextSize(teile[i]).x); }
    const float mindest = beschriftung + st.ItemInnerSpacing.x + fs * 3.0F;
    const int spalten = n > 1 && breit >= mindest * 2.0F + st.ItemSpacing.x ? 2 : 1;
    const float zelle = (breit - st.ItemSpacing.x * static_cast<float>(spalten - 1)) / static_cast<float>(spalten);
    for (int i = 0; i < n; ++i) {
        if (i % spalten != 0) { ImGui::SameLine(); }
        const ImVec2 h{beschriftung + st.ItemInnerSpacing.x * 0.5F, ImGui::GetFrameHeight()};
        ImGui::InvisibleButton(griffe[i], h);
        const ImVec2 a = ImGui::GetItemRectMin();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const bool ueber = ImGui::IsItemHovered() || ImGui::IsItemActive();
        dl->AddText(ImVec2{a.x, a.y + (h.y - ImGui::GetTextLineHeight()) * 0.5F}, ueber ? ImGui::GetColorU32(ImGuiCol_Text) : IM_COL32(0xA6, 0xA9, 0xB1, 255), teile[i]);
        if (ueber) { ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); }
        if (ImGui::IsItemActive() && ImGui::GetIO().MouseDelta.x != 0.0F) {
            v[i] += ImGui::GetIO().MouseDelta.x * tempo;
            neu = true;
        }
        ImGui::SameLine(0, st.ItemInnerSpacing.x * 0.5F);
        ImGui::SetNextItemWidth(std::max(fs * 2.0F, zelle - h.x - st.ItemInnerSpacing.x * 0.5F));
        neu |= ImGui::InputScalar(achsen[i], ImGuiDataType_Float, &v[i], nullptr, nullptr, format, ImGuiInputTextFlags_AutoSelectAll);
    }
    ImGui::EndGroup();
    ImGui::PopID();
    return neu;
}

// Ein Eintrag aus einer Liste mit Suche (Animationen, Modelle, Skins, Menues, Cvars). true = gewaehlt
bool suchListe(const char* id, const char* knopf, std::string& filter, const std::vector<std::string>& liste, std::string& gewaehlt) {
    bool neu = false;
    if (ImGui::Button(knopf)) {
        filter.clear();
        ImGui::OpenPopup(id);
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2{ImGui::GetFontSize() * 18.0F, 0}, ImVec2{ImGui::GetFontSize() * 30.0F, ImGui::GetFontSize() * 24.0F});
    if (ImGui::BeginPopup(id)) {
        if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }
        ImGui::SetNextItemWidth(-FLT_MIN);
        (void)eingabe("##suche", &filter, 0, tr(Str::MeListeSuche));
        std::string f = filter;
        for (char& c : f) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        for (const std::string& s : liste) {
            std::string l = s;
            for (char& c : l) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            if (!f.empty() && l.find(f) == std::string::npos) { continue; }
            if (ImGui::Selectable(s.c_str())) {
                gewaehlt = s;
                neu = true;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
    return neu;
}

std::vector<Ersetzung> mitKopienAbgleich(const Dokument& d, std::vector<Ersetzung> e);   // M7 Punkt 3 (app_menue_m7.inc)

void aendere(std::vector<Ersetzung> e) {
    Dokument* d = doc();
    if (d == nullptr || e.empty()) { return; }
    // rc608 Teil 3b: Was aus Feldern und Griffen kommt, zerstoert nie ein lesbares Menue: Wuerde das Spiel es danach verwerfen, bleibt alles, wie es war, und das Programm sagt es.
    {
        const auto alleOk = [](const Datei& x) { return std::all_of(x.menus.begin(), x.menus.end(), [](const Menu& m) { return m.ok; }); };
        if (alleOk(d->datei)) {
            std::vector<Ersetzung> sortiert = e;
            std::stable_sort(sortiert.begin(), sortiert.end(), [](const Ersetzung& a, const Ersetzung& b) { return a.anfang > b.anfang; });
            std::string probe = d->puffer.text();
            for (const Ersetzung& x : sortiert) { probe.replace(x.anfang, x.ende - x.anfang, x.neu); }
            if (!alleOk(lies(d->name, probe, leseOptionen(d->name)))) {
                std::vector<char> b(600);
                const std::string gezeigt = anzeige(e.front().neu.substr(0, 200));
                std::snprintf(b.data(), b.size(), tr(Str::MlUngueltig), gezeigt.c_str());
                Z().meldung = b.data();
                addStatus(b.data());
                return;
            }
        }
    }
    // Mehrere neue Zeilen an derselben Stelle (zwei fehlende Schluessel) bleiben in ihrer Folge: sonst haengt die Datei von der Sortierung ab
    std::vector<Ersetzung> zusammen;
    for (Ersetzung& x : e) {
        auto da = x.anfang == x.ende ? std::find_if(zusammen.begin(), zusammen.end(), [&x](const Ersetzung& y) { return y.anfang == x.anfang && y.ende == x.anfang; }) : zusammen.end();
        if (da != zusammen.end()) { da->neu += x.neu; }
        else { zusammen.push_back(std::move(x)); }
    }
    e = mitKopienAbgleich(*d, std::move(zusammen));
    d->puffer.ersetzeViele(std::move(e));
    neuParsen(*d);
}

// Dateien ins Paket; Teil des Schritts, der gleich im Text folgt (Stand im Beiwerk, anzeigeHolen)
void paketSetzen(Dokument& d, const std::vector<std::pair<std::string, std::string>>& dateien) {
    std::vector<Dokument::PaketAenderung> schritt;
    for (const auto& [datei, bytes] : dateien) {
        Dokument::PaketAenderung a;
        a.datei = datei;
        const auto f = d.bilder.find(datei);
        a.warDa = f != d.bilder.end();
        if (a.warDa) { a.vorher = f->second; }
        a.nachher = bytes;
        d.bilder[datei] = bytes;
        schritt.push_back(std::move(a));
    }
    d.paketSchritte.resize(static_cast<std::size_t>(d.paketStand));
    d.paketSchritte.push_back(std::move(schritt));
    ++d.paketStand;
}

void paketAufStand(Dokument& d, int ziel) {
    Zustand& z = Z();
    if (ziel == d.paketStand) { return; }
    while (d.paketStand > ziel && d.paketStand > 0) {
        --d.paketStand;
        for (const Dokument::PaketAenderung& a : d.paketSchritte[static_cast<std::size_t>(d.paketStand)]) {
            if (a.warDa) { d.bilder[a.datei] = a.vorher; }
            else { d.bilder.erase(a.datei); }
        }
    }
    while (d.paketStand < ziel && static_cast<std::size_t>(d.paketStand) < d.paketSchritte.size()) {
        for (const Dokument::PaketAenderung& a : d.paketSchritte[static_cast<std::size_t>(d.paketStand)]) {
            if (a.istDa) { d.bilder[a.datei] = a.nachher; }
            else { d.bilder.erase(a.datei); }
        }
        ++d.paketStand;
    }
    z.texturen.clear();
    z.kinos.clear();
    d.texteGelesen = false;   // die eigenen Texte stehen im Paket
    ++d.textStand;
    ++z.texteVersion;
}

// Der Unterschied zum Stand davor als Schritt, wenn eine Funktion das Paket selbst schreibt (eigene Texte)
void paketSchrittSeit(Dokument& d, const std::map<std::string, std::string>& vorher) {
    std::vector<Dokument::PaketAenderung> schritt;
    for (const auto& [datei, bytes] : d.bilder) {
        const auto f = vorher.find(datei);
        if (f == vorher.end() || f->second != bytes) {
            Dokument::PaketAenderung a;
            a.datei = datei;
            a.warDa = f != vorher.end();
            if (a.warDa) { a.vorher = f->second; }
            a.nachher = bytes;
            schritt.push_back(std::move(a));
        }
    }
    for (const auto& [datei, bytes] : vorher) {
        if (d.bilder.count(datei) == 0) {
            Dokument::PaketAenderung a;
            a.datei = datei;
            a.warDa = true;
            a.istDa = false;
            a.vorher = bytes;
            schritt.push_back(std::move(a));
        }
    }
    if (schritt.empty()) { return; }
    d.paketSchritte.resize(static_cast<std::size_t>(d.paketStand));
    d.paketSchritte.push_back(std::move(schritt));
    ++d.paketStand;
}

// Was zu einem Rueckgaengig-Schritt gehoert: die Seite, das Paket und die Auswahl
// Seite, Paket, Auswahl; rc609 ME1: dazu Schloss (0) und Auge (1) je (Menue, Item), damit ein geloeschtes Element sie mit Rueckgaengig wiederbekommt
std::vector<int> anzeigeStand(const Dokument& d) {
    std::vector<int> v;
    v.reserve(d.auswahl.size() + 3 + (d.gesperrt.size() + d.verborgen.size()) * 3);
    v.push_back(d.seite);
    v.push_back(d.paketStand);
    v.push_back(static_cast<int>(d.auswahl.size()));
    v.insert(v.end(), d.auswahl.begin(), d.auswahl.end());
    const auto merker = [&v](int art, int m, int i) { v.insert(v.end(), {art, m, i}); };
    for (const int i : d.gesperrt) { merker(0, d.gesperrtMenue, i); }
    for (const auto& [m, menge] : d.gesperrtAndere) {
        for (const int i : menge) { merker(0, m, i); }
    }
    for (const auto& [m, i] : d.verborgen) { merker(1, m, i); }
    return v;
}

void haltSchreiben() {
    Zustand& z = Z();
    if (z.halt.id == 0) { return; }
    const std::function<void()> f = std::move(z.halt.schreiben);
    z.halt.id = 0;
    z.halt.schreiben = nullptr;
    if (f) { f(); }
}

// Vor den Panels: ein Klick neben das Feld oder ein Feld, das nicht mehr gezeichnet wird, schreibt jetzt -
// noch bevor der Klick die Auswahl oder den Reiter wechselt.
void haltPruefen() {
    const Zustand& z = Z();
    if (z.halt.id == 0) { return; }
    const bool daneben = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !z.halt.rect.Contains(ImGui::GetIO().MousePos);
    if (daneben || z.halt.bild + 1 < ImGui::GetFrameCount()) { haltSchreiben(); }
}

// Kann ein Wert nicht geschrieben werden, sagt es das Programm sichtbar, statt still beim alten zu bleiben.
void ausMakroMelden() {
    Z().meldung = tr(Str::MeAusMakro);
    ::bhed::gui::meldung(tr(Str::MeAusMakro), "", true);
}

Rechteck clientRect(const Menu& m, const Item& it) { (void)m; return it.window.rectClient; }

void layoutsAnwenden(Dokument& d, int mi, const Layout* geaendert, bool loeschen, const std::map<std::string, Rechteck>& vorab,
                     std::vector<Ersetzung>& e);   // M5 Punkt 1 (app_menue_layout.inc)
void layoutTasten(Dokument& d);   // M5 Punkt 1: Strg+Umschalt+1..4

// rc608 Teil 3a: Die Ausrichtung bleibt beim Skalieren, auch senkrecht (app_menue_leiste.inc)
void textFolgtRect(Dokument& d, int mi, int i, const Rechteck& neu, std::vector<Ersetzung>& e);

void setzeRects(const std::vector<std::pair<int, Rechteck>>& neu) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    const int mi = menuDerSeite(*d);
    std::vector<Ersetzung> e;
    // M5: bewegt sich die Flaeche (frame) eines Layouts, rechnet das Layout die Kinder neu - eine Ersetzung
    if (mi >= 0) {
        const std::vector<Layout> ls = liesLayouts(d->datei, mi);
        const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
        std::map<std::string, Rechteck> vorab;
        bool rahmen = false;
        for (const auto& [i, r] : neu) {
            if (static_cast<std::size_t>(i) >= m.items.size()) { continue; }
            const std::string& n = m.items[static_cast<std::size_t>(i)].window.name;
            vorab[n] = r;
            rahmen = rahmen || std::any_of(ls.begin(), ls.end(), [&](const Layout& l) { return !n.empty() && l.rahmenItem == n; });
        }
        if (rahmen && vorab.size() == neu.size()) {
            layoutsAnwenden(*d, mi, nullptr, false, vorab, e);
            aendere(std::move(e));
            return;
        }
    }
    for (const auto& [i, r] : neu) {
        std::vector<Ersetzung> x;
        if (setzeRect(d->datei, mi, i, r, x)) {
            e.insert(e.end(), x.begin(), x.end());
            textFolgtRect(*d, mi, i, r, e);
        } else {
            ausMakroMelden();
        }
    }
    aendere(std::move(e));
}

std::string konstante(const Datei& d, const char* name, int wert) {
    if (d.dialekt == Dialekt::SP) { return name; }
    return d.defines.count(name) != 0 ? std::string(name) : std::to_string(wert);
}

void eigenschaft(int item, const char* schluessel, const std::vector<std::string>& werte) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    std::vector<Ersetzung> e;
    // rc608 Teil 3b: Ein leerer Wert ist keine leere Zeichenkette in der Datei (cvarTest "" verwirft im Spiel das Menue), sondern keine Zeile
    if (werte.size() == 1U && werte[0] == "\"\"" && std::strcmp(schluessel, "text") != 0) {
        if (entferneEigenschaft(d->datei, menuDerSeite(*d), item, schluessel, e)) { aendere(std::move(e)); }
        return;
    }
    if (setzeEigenschaft(d->datei, menuDerSeite(*d), item, schluessel, werte, e)) { aendere(std::move(e)); }
    else { ausMakroMelden(); }
}

std::string farbText(const Farbe& c) {
    return zahlText(c.r) + " " + zahlText(c.g) + " " + zahlText(c.b) + " " + zahlText(c.a);
}

// rc608 Teil 3b: Ein Wert aus einem Feld (UTF-8) in Anfuehrungszeichen fuer die Datei. Ein " beendete sonst die Zeichenkette, und das Spiel verwarf das ganze Menue.
// Im Einzelspieler kennt COM_ParseExt kein Escape, das Zeichen faellt weg; im Mehrspieler schreibt der Lexer es mit Backslash.
std::string feldWort(const std::string& utf8, const std::string& bytes) {
    const Dokument* d = doc();
    const bool mp = d != nullptr && d->datei.dialekt == Dialekt::MP;
    (void)utf8;
    std::string t = "\"";
    for (const char c : bytes) {
        if (c == '\n' || c == '\r') { continue; }
        if (!mp && c == '"') { continue; }
        if (mp && (c == '\\' || c == '"')) { t += '\\'; }
        t += c;
    }
    return t + "\"";
}
std::string inAnf(const std::string& s) { return feldWort(s, utf8ZuLatin1(s.c_str())); }

// Zeile eines Items im Text (fuer den Sprung Flaeche -> Code)
void springe(Dokument& d, std::uint32_t off) {
    d.puffer.einCursor(off);
    d.springeZeile = d.puffer.pos(off).zeile;
}

// Welches Item steht an der Cursorposition? (Code -> Flaeche)
void auswahlAusCursor(Dokument& d) {
    const int mi = menuDerSeite(d);
    if (mi < 0) { return; }
    const std::uint32_t c = d.puffer.cursor.front().pos;
    for (std::size_t m = 0; m < d.datei.menus.size(); ++m) {
        const Menu& me = d.datei.menus[m];
        if (c < me.anfang || c > me.ende) { continue; }
        for (std::size_t i = 0; i < me.items.size(); ++i) {
            if (c >= me.items[i].anfang && c <= me.items[i].ende) {
                if (static_cast<int>(m) != mi) { zeigeMenue(d, static_cast<int>(m)); }
                d.auswahl.assign(1, static_cast<int>(i));
                return;
            }
        }
    }
}

// --- Vorschau ------------------------------------------------------------------

const Texte& vorschauTexte(Dokument& d);   // M2 Punkt 6 (unten)

// M7 (app_menue_m7.inc)
void datapadDatenLaden();
// Was das Datapad aus der Umgebung der Vorschau braucht (Texte, Schrift, Bilder)
struct DatapadUmgebung {
    const Texte* texte = nullptr;
    Kodierung kodierung = Kodierung::Latin1;
    bool asienGlyphen = false;
    Jk2Schrift jk2 = Jk2Schrift::Nein;
    std::function<const Schrift*(int)> schriftNr;
    std::function<bool(const std::string&)> bildDa;
};
bool datapadZeichnen(int art, std::vector<OdTeil>& out, const DatapadUmgebung& du);

Umgebung umgebung(const Dokument& d) {
    Zustand& z = Z();
    schriftNamenLaden();
    Umgebung u;
    // rc608 Teil 3b: Die Rahmen rechnen mit der Aufloesung des Spiels; ohne gewaehlte Aufloesung gilt die Groesse der Flaeche wie ein Spielfenster
    {
        static const int kPunkte[10][2] = {{0, 0}, {640, 480}, {800, 600}, {1024, 768}, {1280, 720}, {1366, 768}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3440, 1440}};
        const float bx = z.aufloesung > 0 ? static_cast<float>(kPunkte[z.aufloesung][0]) : z.flaecheBild.x;
        const float by = z.aufloesung > 0 ? static_cast<float>(kPunkte[z.aufloesung][1]) : z.flaecheBild.y;
        if (bx > 1.0F && by > 1.0F) {
            u.rahmenX = bx / 640.0F;
            u.rahmenY = by / 480.0F;
        }
    }
    u.texte = &vorschauTexte(const_cast<Dokument&>(d));   // M2 Punkt 6: gewaehlte Sprache, eigene Texte darueber
    u.dialekt = d.datei.dialekt;
    const std::vector<std::string>& namen = d.datei.dialekt == Dialekt::MP ? z.schriftNamenMp : z.schriftNamenSp;
    // Nummer -> Schrift wie schriftNamen (SP: OpenJK registeredFonts bzw. Movie-Duels-Griffe).
    // Ausserhalb der Liste: MP und OpenJK nehmen qhMediumFont, Movie Duels findet keinen
    // Griff (GetFont = NULL, zeichnet nichts). Ohne assetGlobalDef (keine Spieldaten): ergoec.
    const bool mdGriffe = d.datei.dialekt == Dialekt::SP && z.spiel == Spiel::MD;
    u.schrift = [namen, mdGriffe](int f) -> const Schrift* {
        if (namen.empty()) { return schriftNamens("ergoec"); }
        std::size_t i = (f >= 0 && static_cast<std::size_t>(f) < namen.size()) ? static_cast<std::size_t>(f) : 0;
        if (mdGriffe && f > 0 && static_cast<std::size_t>(f) >= namen.size()) { return nullptr; }
        return namen[i].empty() ? nullptr : schriftNamens(namen[i]);
    };
    const Dialekt dl = d.datei.dialekt;
    const Spiel sp = z.spiel;
    u.cvar = [dl, sp](const std::string& n) {
        const auto& m = Z().vorschauCvars;
        const auto it = m.find(kleinPfad(n));
        return it == m.end() ? spielCvarVorgabe(dl, sp, n) : it->second;
    };
    // M2: die Listen wie das Spiel sie fuellt (feeder.cpp); einmal je Spieldatenstand gebaut
    FeederDaten& fd = d.datei.dialekt == Dialekt::MP ? z.feederMp : z.feederSp;
    if (!fd.gebaut) {
        fd.daten = &z.daten;
        fd.dialekt = d.datei.dialekt;
        fd.spiel = z.spiel;
        fd.ordner.clear();
        for (const GamePath& g : z.daten.ordner) { fd.ordner.push_back(g.directory); }
        fd.basisOrdner = z.daten.ordner.empty() ? std::string() : u8(fs::path(z.daten.ordner.front().directory).parent_path());
        fd.baue();
    }
    const FeederDaten* fdp = &fd;
    // Kopf/Rumpf/Beine/Farben der Spezies aus ui_char_model (UI_GetCharacterCvars), nicht immer der ersten
    fd.se = [tx = u.texte](const std::string& key) { return tx != nullptr ? tx->imSpiel("@" + key) : key; };
    u.feeder = [fdp, cv = u.cvar](float f) { return fdp->eintraege(f, fdp->speziesIndex(cv("ui_char_model"))); };
    u.feederArt = [fdp](float f) { return fdp->art(f); };
    // M7: Tastenbelegung aus der Vorgabe-Config (Com_Init: exec default.cfg / mpdefault.cfg ...)
    const int ti = d.datei.dialekt == Dialekt::MP ? 1 : 0;
    if (!z.tastenGelesen[ti]) {
        z.tastenGelesen[ti] = true;
        z.tasten[ti] = Tastenbelegung{};
        std::string cfg;
        const auto lader = [](const std::string& p, std::string& out) { return Z().daten.lies(p, out); };
        if (z.daten.lies(vorgabeConfig(d.datei.dialekt, z.spiel), cfg)) { configAusfuehren(cfg, z.tasten[ti], lader); }
    }
    u.bindung = [tb = &z.tasten[ti], dl = d.datei.dialekt, sp = z.spiel, tx = u.texte](const std::string& befehl) {
        return bindungsText(*tb, befehl, dl, sp, [tx](const std::string& key) { return tx != nullptr ? tx->imSpiel("@" + key) : std::string(); });
    };
    u.bildDa = [](const std::string& s) { return bildRegistriert(s); };
    u.zeitMs = static_cast<int>(ImGui::GetTime() * 1000.0);
    u.alleItems = z.alleItems && !z.interaktion;
    // M3: Schrift wie se_language; Glyphenseiten da (AsianGlyphsAvailable: Seite 0 laedt)
    u.kodierung = kodierungDerSprache(z.sprache);
    if (d.datei.spiel == Spiel::JK2) { u.jk2 = d.datei.dialekt == Dialekt::MP ? Jk2Schrift::Mp : Jk2Schrift::Sp; }
    if (asiatisch(u.kodierung)) {
        const std::string seite = asienSeite(u.kodierung, 0);
        u.asienGlyphen = z.daten.gibt(seite + ".tga") || z.daten.gibt(seite + ".jpg") || z.daten.gibt(seite + ".png");
    }
    // M7 Punkt 14: DC->ownerDrawItem (ownerdraw.cpp)
    if (!z.glGelesen) {
        z.glGelesen = true;
        z.glDaten.echt = platform::openGlTreiber(z.glDaten.vendor, z.glDaten.renderer, z.glDaten.version, z.glDaten.extensions);
    }
    u.schriftNamens = [](const std::string& n) { return schriftNamens(n); };
    OdKontext k;
    k.dialekt = d.datei.dialekt;
    k.spiel = z.spiel;
    k.daten = fdp;
    k.cvar = u.cvar;
    k.se = [tx = u.texte](const std::string& key) { return tx != nullptr ? tx->imSpiel("@" + key) : key; };
    k.breite = [sf = u.schrift, ko = u.kodierung, ag = u.asienGlyphen, j = u.jk2](const std::string& t, float sc, int font, const std::string& name) {
        const Schrift* f = name.empty() ? sf(font) : schriftNamens(name);
        return f != nullptr ? schriftBreite(*f, t, sc, ko, ag, j) : 0;
    };
    k.hoehe = [sf = u.schrift, j = u.jk2](float sc, int font, const std::string& name) {
        const Schrift* f = name.empty() ? sf(font) : schriftNamens(name);
        return f != nullptr ? schriftHoehe(*f, sc, j) : 0;
    };
    k.bildDa = u.bildDa;   // R_RegisterShaderNoMip: Bild oder Shaderskript
    k.zeitMs = u.zeitMs;
    k.beispiel = &z.spielBeispiel;
    k.gl = &z.glDaten;
    k.pazaak = &z.pazaakZustand;
    k.datapad = [du = DatapadUmgebung{u.texte, u.kodierung, u.asienGlyphen, u.jk2, u.schrift, u.bildDa}](int art, std::vector<OdTeil>& out) {
        return datapadZeichnen(art, out, du);
    };
    u.ownerDraw = [k](const OdAufruf& a) { return ownerDraw(a, k); };
    u.ownerDrawBreite = [k](int id, float scale) { return ownerDrawBreite(id, scale, k); };   // rc610 F46 B6-3
    return u;
}

// M2: CIN_PlayCinematic: ohne / -> video/<name>, Endung .roq (COM_DefaultExtension)
std::string kinoPfad(const std::string& name) {
    std::string n = name.find('/') == std::string::npos && name.find('\\') == std::string::npos ? "video/" + name : name;
    const std::size_t sl = n.find_last_of("/\\");
    if (n.find('.', sl == std::string::npos ? 0 : sl) == std::string::npos) { n += ".roq"; }
    return n;
}

Kino* kino(const std::string& name, bool shader = false) {
    Zustand& z = Z();
    if (name.empty()) { return nullptr; }
    Kino& k = z.kinos[kleinPfad(name) + (shader ? "|shader" : "")];
    if (k.stand != render::texturStand()) {   // Geraeteverlust: Textur neu
        k.tex = nullptr;
        k.stand = render::texturStand();
    }
    const int jetzt = static_cast<int>(ImGui::GetTime() * 1000.0);
    if (!k.fehlt && k.spieler.status() == RoqSpieler::Status::Eof && k.spieler.bilder() < 0) {
        std::string daten;
        bool da = false;
        const std::string pfad = kinoPfad(name);
        if (Dokument* d = doc()) {
            for (const auto& [p, b] : d->bilder) {
                if (kleinPfad(p) == kleinPfad(pfad)) {
                    daten = b;
                    da = true;
                }
            }
        }
        if (!da) { da = z.daten.lies(pfad, daten); }
        // UI_PlayCinematic: CIN_loop | CIN_silent; videoMap zusaetzlich CIN_shader
        k.fehlt = !da || !k.spieler.start(std::move(daten), jetzt, true, shader);
    }
    if (k.fehlt) { return nullptr; }
    // rc608 Teil 3b: Die Engine kennt nur ein laufendes Video (cin.currentHandle): Wechselt der Griff, setzt CIN_RunCinematic das andere zurueck und liest
    // in diesem Lauf nichts. Zwei Videos im selben Bild stehen darum still, jedes auf seinem Bild. Neu gestartet ist es selbst das laufende (CIN_PlayCinematic).
    const int bildNr = ImGui::GetFrameCount();
    if (z.kinoGriff != &k && z.kinoGriff != nullptr) {
        if (k.bilder > 0) {
            k.spieler.griffGewechselt();
        }
        if (z.kinoWechselBild == bildNr) { z.kinoZweiBild = bildNr; }
        z.kinoWechselBild = bildNr;
    }
    z.kinoGriff = &k;
    (void)k.spieler.lauf(jetzt);   // DC->runCinematicFrame
    if (k.spieler.neuesBild() && k.spieler.bild() != nullptr) {
        if (k.tex == nullptr || k.w != k.spieler.breite() || k.h != k.spieler.hoehe()) {
            k.w = k.spieler.breite();
            k.h = k.spieler.hoehe();
            k.tex = render::createTexture(k.spieler.bild(), k.w, k.h);
        } else {
            render::updateTexture(k.tex, k.spieler.bild(), k.w, k.h);
        }
        ++k.bilder;
        ++z.kinoBilder;
    }
    return k.tex != nullptr ? &k : nullptr;
}

// M2: das weisse Bild der Engine (tr.whiteImage) fuer map $whiteimage und *white
const Textur* weissesBild() {
    Zustand& z = Z();
    if (z.weiss.tex == nullptr || z.weissStand != render::texturStand()) {
        const unsigned char px[16] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
        z.weiss.tex = render::createTexture(px, 2, 2);
        z.weiss.w = z.weiss.h = 2;
        z.weissStand = render::texturStand();
    }
    return z.weiss.tex != nullptr ? &z.weiss : nullptr;
}

// M2: ein Bild ohne Shaderskript (fuer eine Stufe: map, animMap-Bild)
const Textur* stufenBild(const std::string& name) {
    Zustand& z = Z();
    const std::string key = "#stufe#" + kleinPfad(name);
    Textur& t = z.texturen[key];
    if (t.versucht) { return t.tex != nullptr ? &t : nullptr; }
    t.versucht = true;
    std::string ohne = kleinPfad(name);
    const std::size_t punkt = ohne.rfind('.');
    if (punkt != std::string::npos && punkt + 5 >= ohne.size() && ohne.find('/', punkt) == std::string::npos) { ohne = ohne.substr(0, punkt); }
    for (const std::string& k : textureCandidates(ohne)) {
        std::string bytes;
        if (!z.daten.lies(k, bytes)) { continue; }
        const image::Image im = image::decode(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size());
        if (!im.ok || im.width <= 0 || im.height <= 0) { continue; }
        t.tex = render::createTexture(im.rgba.data(), im.width, im.height);
        t.w = im.width;
        t.h = im.height;
        break;
    }
    return t.tex != nullptr ? &t : nullptr;
}

// M5, rc608: wie RE_RegisterShaderNoMip (0 = Griff 0, der "<default>"-Kasten): Shaderskript vor Bild, *white ist eingebaut.
// Findet eine Stufe ihr Bild nicht, verwirft ParseStage den ganzen Shader, auch wenn die uebrigen Stufen da waeren.
bool bildRegistriert(const std::string& s) {
    Zustand& z = Z();
    if (s == "*white") { return true; }
    const Textur* t = textur(s);   // laedt auch die Shaderskripte
    std::string key = kleinPfad(s);
    const std::size_t punkt = key.rfind('.');
    if (punkt != std::string::npos && punkt + 5 >= key.size() && key.find('/', punkt) == std::string::npos) { key = key.substr(0, punkt); }
    const auto it = z.shader.find(key);
    if (it == z.shader.end() || it->second.stages.empty()) { return t != nullptr; }
    for (const ShaderInfo::Stage& st : it->second.stages) {
        if (!st.image.empty() && stufenBild(st.image) == nullptr) { return false; }
        for (const std::string& a : st.animImages) {
            if (stufenBild(a) == nullptr) { return false; }
        }
    }
    return true;
}

// M2: ein Menue-Bild mit Shaderskript Stufe fuer Stufe wie RB_StageIteratorGeneric
// fuer 2D (RE_StretchPic): Bild bzw. animMap/videoMap je Stufe, Farbe nach
// rgbGen/alphaGen (ohne rgbGen: identity bzw. identityLighting, ohne Overbright
// wie im Fenster), tcMod ueber packeKonstanten, blendFunc und clampMap.
// false, wenn es kein Skript mit Stufen gibt (dann gilt der implizite Shader).
bool zeichneShaderStufen(ImDrawList* dl, const std::string& shaderName, ImVec2 a, ImVec2 b, const Prim& p) {
    Zustand& z = Z();
    (void)textur(shaderName);   // laedt die Shaderskripte
    std::string key = kleinPfad(shaderName);
    const std::size_t punkt = key.rfind('.');
    if (punkt != std::string::npos && punkt + 5 >= key.size() && key.find('/', punkt) == std::string::npos) { key = key.substr(0, punkt); }
    const auto it = z.shader.find(key);
    if (it == z.shader.end() || it->second.stages.empty()) { return false; }
    const float zeit = static_cast<float>(ImGui::GetTime());
    const ImVec2 ecken[4] = {a, ImVec2{b.x, a.y}, b, ImVec2{a.x, b.y}};
    const float st[4][2] = {{p.s0, p.t0}, {p.s1, p.t0}, {p.s1, p.t1}, {p.s0, p.t1}};
    for (const ShaderInfo::Stage& s : it->second.stages) {
        if (s.lightmap) { continue; }
        void* tex = nullptr;
        if (!s.video.empty()) {
            if (const Kino* k = kino(s.video, true)) { tex = k->tex; }
        } else if (!s.animImages.empty()) {
            const std::size_t n = s.animImages.size();
            if (const Textur* t = stufenBild(s.animImages[animBildIndex(zeit, s.animFreq, n, s.animOneShot)])) { tex = t->tex; }
        } else if (!s.image.empty()) {
            if (const Textur* t = stufenBild(s.image)) { tex = t->tex; }
        } else if (s.weiss) {
            if (const Textur* t = weissesBild()) { tex = t->tex; }
        }
        if (tex == nullptr) { continue; }
        // rgbGen / alphaGen
        float rgb[3] = {1, 1, 1};
        switch (s.rgbArt) {
            case RgbGenArt::Vertex:
            case RgbGenArt::ExactVertex:
                rgb[0] = p.farbe.r;
                rgb[1] = p.farbe.g;
                rgb[2] = p.farbe.b;
                break;
            case RgbGenArt::OneMinusVertex:
                rgb[0] = 1 - p.farbe.r;
                rgb[1] = 1 - p.farbe.g;
                rgb[2] = 1 - p.farbe.b;
                break;
            case RgbGenArt::Const:
                for (int k = 0; k < 3; ++k) { rgb[k] = s.rgbConst[k]; }
                break;
            case RgbGenArt::Wave: {
                const float w = std::clamp(waveValue(s.rgbWave, zeit), 0.0F, 1.0F);
                rgb[0] = rgb[1] = rgb[2] = w;
                break;
            }
            default:
                break;   // identity, identityLighting (ohne Overbright = 1)
        }
        float alpha = 1.0F;
        if (s.alphaGen == AlphaGen::Const) { alpha = s.alphaConst; }
        else if (s.alphaGen == AlphaGen::Vertex || (s.alphaGen == AlphaGen::FromImage && s.rgbArt == RgbGenArt::Vertex)) {
            alpha = p.farbe.a;   // rgbGen vertex setzt ohne alphaGen AGEN_VERTEX
        } else if (s.alphaGen == AlphaGen::OneMinusVertex) { alpha = 1 - p.farbe.a; }
        if (s.alphaWave.active()) { alpha = std::clamp(waveValue(s.alphaWave, zeit), 0.0F, 1.0F); }
        // tcMod: dieselbe Rechnung wie die Karte (packeKonstanten)
        BatchState bs;
        bs.numTexMods = std::min(s.numTexMods, kMaxTexMods);
        for (int k = 0; k < bs.numTexMods; ++k) { bs.texMods[k] = s.texMods[k]; }
        float konst[gpu::kKonstFloats] = {};
        gpu::packeKonstanten(bs, konst, zeit);
        const float* m = konst + gpu::kTexMatrixAt;
        const float* o = konst + gpu::kTexOffTurbAt;
        ImVec2 uv[4];
        for (int k = 0; k < 4; ++k) {
            uv[k] = ImVec2{st[k][0] * m[0] + st[k][1] * m[2] + o[0], st[k][0] * m[1] + st[k][1] * m[3] + o[1]};
        }
        const bool opak = s.srcFactor == BlendFactor::One && s.dstFactor == BlendFactor::Zero;
        const ImU32 col = ImGui::ColorConvertFloat4ToU32(
            ImVec4{std::clamp(rgb[0], 0.0F, 1.0F), std::clamp(rgb[1], 0.0F, 1.0F), std::clamp(rgb[2], 0.0F, 1.0F), std::clamp(alpha, 0.0F, 1.0F)});
        render::zeichneStufe(dl, tex, ecken, uv, col, static_cast<int>(opak ? BlendFactor::One : s.srcFactor),
                             static_cast<int>(opak ? BlendFactor::Zero : s.dstFactor), s.clamp);
    }
    return true;
}

// M2: das Bild eines Klingen-Shaders aus den Spieldaten des Menue-Editors: die erste
// Stufe mit Bild (map), sonst das gleichnamige Bild (impliziter Shader); dazu ihre
// Mischung (blendFunc) und ob sie rgbGen vertex hat. nullptr, wenn es kein Bild gibt (die Engine nimmt
// dann ihren Ersatzshader).
const KlingenBild* klingenBild(const std::string& shaderName) {
    Zustand& z = Z();
    const std::string key = kleinPfad(shaderName);
    KlingenBild& kb = z.klingenBilder[key];
    if (kb.gesucht) { return kb.bild.empty() ? nullptr : &kb; }
    kb.gesucht = true;
    (void)textur(shaderName);   // laedt die Shaderskripte
    std::string bild = key;
    const auto it = z.shader.find(key);
    if (it != z.shader.end()) {
        for (const ShaderInfo::Stage& s : it->second.stages) {
            if (s.image.empty()) { continue; }
            bild = kleinPfad(s.image);
            const bool eins = s.srcFactor == BlendFactor::One || s.srcFactor == BlendFactor::DstAlpha;
            kb.mischung = (eins && s.dstFactor == BlendFactor::One)                                            ? 0
                          : (s.srcFactor == BlendFactor::SrcAlpha && s.dstFactor == BlendFactor::OneMinusSrcAlpha) ? 1
                                                                                                                  : -1;
            kb.vertexFarbe = s.rgbArt == RgbGenArt::Vertex || s.rgbArt == RgbGenArt::ExactVertex;
            break;
        }
    } else {
        kb.mischung = -1;   // impliziter Shader: deckend
    }
    const std::size_t punkt = bild.rfind('.');
    if (punkt != std::string::npos && bild.find('/', punkt) == std::string::npos) { bild = bild.substr(0, punkt); }
    for (const char* endung : {".tga", ".jpg", ".png"}) {
        std::string daten;
        if (!z.daten.lies(bild + endung, daten)) { continue; }
        const image::Image im = image::decode(reinterpret_cast<const unsigned char*>(daten.data()), daten.size());
        if (!im.ok || im.width <= 0 || im.height <= 0) { continue; }
        kb.bild.width = im.width;
        kb.bild.height = im.height;
        kb.bild.rgba = im.rgba;
        break;
    }
    return kb.bild.empty() ? nullptr : &kb;
}

// M2: die Klingen eines Menue-Saebers (code/ui/ui_saber.cpp UI_SaberDrawBlades,
// UI_SaberDrawBlade): Saeber aus ui_saber (isSaber) bzw. ui_saber2 (isSaber2),
// Klingenzahl (1..8), -art, -laenge (Vorgabe 40) und -radius (Vorgabe 3) aus der
// .sab, noBlade/noBlade2 ab bladeStyle2Start, Farbe aus ui_saber_color bzw.
// ui_saber2_color; Lage ueber die *blade-Bolzen bzw. den tagHack mal DC->xscale
// (uiKlingenStrecke). Was der Szene uebergeben wird:
//   OpenJK/Jedi Outcast: UI_DoSaber (TranslateSaberColor: unbekannt = blau);
//   Movie Duels (M3): die Art aus cg_SFXSabers und der Saeberart (mdKlingenArt, alle
//     UI_Do*Saber), Shader je Art und Farbe (mdKlingenShader: rgb/white/custom/RGB-Werte,
//     black, unstable_red), Glanzfarbe ui_rgb_saber*_red/green/blue bei RGB-Farben.
// Glanz-Sprites und Kern wie RB_SurfaceSaberGlow/RB_SurfaceLine (uiSaberVierecke) mit
// der Mischung des Shaders (additiv bzw. blend fuer schwarze Klingen, rgbGen vertex),
// gegen die Tiefe des Griffs getestet - derselbe Szenenaufruf wie im Spiel.
void zeichneMenueKlingen(ImDrawList* dl, ModellAnsicht& mv, const Item& it, const MenueModellBild& mb, const Umgebung* umg, ImVec2 ia,
                         ImVec2 ib, int pw, int ph, float xscale) {
    Zustand& z = Z();
    const bool zweiter = (it.itemFlags & ITF_ISSABER) == 0;
    auto cv = [&](const std::string& n) { return umg != nullptr && umg->cvar ? umg->cvar(n) : std::string(); };
    auto zahl = [&](const std::string& n, float vorgabe) {
        const std::string w = cv(n);
        return w.empty() ? vorgabe : static_cast<float>(std::atof(w.c_str()));
    };
    std::string modell, haut;
    if (!saberModellFuer(cv(zweiter ? "ui_saber2" : "ui_saber"), modell, haut)) { return; }
    const SaberDef& s = z.saeber[kleinPfad(cv(zweiter ? "ui_saber2" : "ui_saber"))];
    const std::string farbe = kleinPfad(cv(zweiter ? "ui_saber2_color" : "ui_saber_color"));
    const bool md = z.spiel == Spiel::MD;
    const int art = md ? mdKlingenArt(std::atoi(cv("cg_SFXSabers").c_str()), s.type) : 0;
    if (art < 0) { return; }   // SABER_SITH_SWORD bzw. cg_SFXSabers ueber 11: keine Klinge
    const bool benannt = farbe == "red" || farbe == "orange" || farbe == "yellow" || farbe == "green" || farbe == "blue" || farbe == "purple";
    std::string glanzName = "gfx/effects/sabers/" + std::string(benannt ? farbe : "blue") + "_glow";
    std::string kernName = "gfx/effects/sabers/" + std::string(benannt ? farbe : "blue") + "_line";
    std::string kern2Name;   // MD: zweiter Kern bei genau "rgb"
    float glanzFarbe[3] = {1, 1, 1};
    if (md) {
        const MdKlingenShader sh = mdKlingenShader(art, farbe, std::atoi(cv("g_SerenityJediEngineMode").c_str()) != 0);
        glanzName = sh.glanz;
        kernName = sh.kern;
        kern2Name = sh.kern2;
        if (sh.rgbWerte) {
            const char* const n1[3] = {"ui_rgb_saber_red", "ui_rgb_saber_green", "ui_rgb_saber_blue"};
            const char* const n2[3] = {"ui_rgb_saber2_red", "ui_rgb_saber2_green", "ui_rgb_saber2_blue"};
            for (int k = 0; k < 3; ++k) { glanzFarbe[k] = static_cast<float>(std::atoi(cv(zweiter ? n2[k] : n1[k]).c_str())) / 255.0F; }
        }
    }
    const KlingenBild* glanz = klingenBild(glanzName);
    const KlingenBild* kern = klingenBild(kernName);
    const KlingenBild* kern2 = kern2Name.empty() ? nullptr : klingenBild(kern2Name);
    std::vector<gpu::KlingenQuad> quads;
    std::vector<UiKlingenViereck> vierecke;
    const float auge[3] = {0, 0, 0}, links[3] = {0, 1, 0}, oben[3] = {0, 0, 1};   // refdef: AxisClear, vieworg 0
    const int klingen = std::clamp(s.numBlades, 1, 8);
    std::string hinweis;
    for (int kn = 0; kn < klingen; ++kn) {
        if (!uiKlingeZeichnen(kn, s.bladeStyle2Start, s.noBlade, s.noBlade2)) { continue; }
        const std::size_t k8 = static_cast<std::size_t>(kn);
        const float laenge = s.bladeLength[k8] > 0 ? s.bladeLength[k8] : (s.length > 0 ? s.length : 40.0F);
        const float radius = s.bladeRadius[k8] > 0 ? s.bladeRadius[k8] : s.radius;
        float o[3], r[3];
        if (!uiKlingenStrecke(mv.modell.glm, s.type, kn, xscale, o, r)) { continue; }
        float a[3], dir[3];   // in die Welt: ursprung + x * achse0 + y * achse1 + z * achse2
        for (int k = 0; k < 3; ++k) {
            a[k] = mb.ursprung[k] + o[0] * mb.achsen[0][k] + o[1] * mb.achsen[1][k] + o[2] * mb.achsen[2][k];
            dir[k] = r[0] * mb.achsen[0][k] + r[1] * mb.achsen[1][k] + r[2] * mb.achsen[2][k];
        }
        const UiKlingenMasse m = md ? uiKlingenMasseMD(art, laenge, laenge, radius, zahl(mdGroessenCvar(art, false), 1), zahl(mdGroessenCvar(art, true), 1))
                                    : uiKlingenMasseOpenJK(laenge, laenge, radius);
        vierecke.clear();
        uiSaberVierecke(a, dir, laenge, m, auge, links, oben, vierecke);
        if (kern2 != nullptr && !vierecke.empty()) { vierecke.push_back(vierecke.back()); }
        std::size_t kerne = 0;
        for (const UiKlingenViereck& v : vierecke) {
            const KlingenBild* b = v.kern ? (kerne++ > 0 && kern2 != nullptr ? kern2 : kern) : glanz;
            if (b == nullptr || b->mischung < 0) {
                if (b != nullptr) { hinweis = (v.kern ? kernName : glanzName) + ": blendFunc nicht nachgebildet, nicht gezeichnet"; }
                continue;
            }
            gpu::KlingenQuad q;
            std::memcpy(q.ecke, v.ecke, sizeof(q.ecke));
            // rgbGen vertex: Bild x shaderRGBA (Glanz in Movie Duels 0.8 bzw. die RGB-Farbe, Kern 255)
            const bool glanzFarbig = b->vertexFarbe && !v.kern;
            for (int k = 0; k < 3; ++k) { q.farbe[k] = 255.0F * (glanzFarbig ? glanzFarbe[k] : 1.0F); }
            q.tex = &b->bild;
            q.spur = true;   // Bild x Farbe x Staerke (klingePixelShaderHlsl)
            q.staerke = glanzFarbig ? m.glanzStaerke : 1.0F;
            q.mischung = b->mischung;
            quads.push_back(q);
        }
    }
    if (!hinweis.empty()) { z.modellFehler = hinweis; }
    if (quads.empty()) { return; }
    if (mv.klingenNr < 0) { mv.klingenNr = z.naechstesZiel++; }
    if (!gpu::bereiteMenueZiel(mv.klingenNr, pw, ph)) { return; }
    std::string fehler;
    if (gpu::zeichneMenueKlingen(mv.klingenNr, quads, mb.kamera, &fehler, mv.nr) <= 0) {
        if (!fehler.empty()) { z.modellFehler = fehler; }
        return;
    }
    void* tex = gpu::menueZielTextur(mv.klingenNr);
    const ImVec2 ecken[4] = {ia, ImVec2{ib.x, ia.y}, ib, ImVec2{ia.x, ib.y}};
    const ImVec2 uv[4] = {ImVec2{0, 0}, ImVec2{1, 0}, ImVec2{1, 1}, ImVec2{0, 1}};
    // vormultipliziert auflegen: additive Klingen lassen das Alpha des Ziels bei 0 (wirkt wie
    // GL_ONE GL_ONE), schwarze (blend) decken mit ihrem Alpha ab - wie nacheinander im Spiel
    render::zeichneStufe(dl, tex, ecken, uv, IM_COL32(255, 255, 255, 255), static_cast<int>(BlendFactor::One),
                         static_cast<int>(BlendFactor::OneMinusSrcAlpha), true);
}

void zeichnePrims(ImDrawList* dl, const std::vector<Prim>& prims, const Abbildung& ab, ImVec2 o, const std::set<int>& verschoben,
                  float dx, float dy, const Menu* menue = nullptr, const Umgebung* umg = nullptr) {
    for (const Prim& p : prims) {
        float x = p.x, y = p.y;
        if (p.item >= 0 && verschoben.count(p.item) != 0) {
            x += dx;
            y += dy;
        }
        const ImVec2 a{o.x + ab.x(x), o.y + ab.y(y)};
        const ImVec2 b{o.x + ab.x(x + p.w), o.y + ab.y(y + p.h)};
        const ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4{std::clamp(p.farbe.r, 0.0F, 1.0F), std::clamp(p.farbe.g, 0.0F, 1.0F),
                                                                std::clamp(p.farbe.b, 0.0F, 1.0F), std::clamp(p.farbe.a, 0.0F, 1.0F)});
        switch (p.art) {
            case PrimArt::Fuellen:
                dl->AddRectFilled(a, b, col);
                break;
            case PrimArt::Bild: {
                if (zeichneShaderStufen(dl, p.shader, a, b, p)) { break; }   // M2: Shaderskript mit Stufen
                if (p.shader == "*white") {   // tr.whiteShader
                    dl->AddRectFilled(a, b, col);
                    break;
                }
                const Textur* t = textur(p.shader);
                if (t == nullptr) {
                    dl->AddRect(a, b, IM_COL32(255, 0, 255, 120));
                    break;
                }
                if (t->additiv) {
                    render::zeichneAdditiv(dl, t->tex, a, b, col);
                } else {
                    dl->AddImage(reinterpret_cast<ImTextureID>(t->tex), a, b, ImVec2{p.s0, p.t0}, ImVec2{p.s1, p.t1}, col);
                }
                break;
            }
            case PrimArt::Kino:
                if (const Kino* k = kino(p.shader)) {   // DC->drawCinematic (DrawStretchRaw, ohne Farbe)
                    dl->AddImage(reinterpret_cast<ImTextureID>(k->tex), a, b);
                    break;
                }
                [[fallthrough]];
            case PrimArt::Modell:
                if (p.art == PrimArt::Modell && menue != nullptr && p.item >= 0 && static_cast<std::size_t>(p.item) < menue->items.size()) {
                    Zustand& z = Z();
                    const Item& it = menue->items[static_cast<std::size_t>(p.item)];
                    const CvarLeser cv = umg != nullptr && umg->cvar ? umg->cvar : CvarLeser{};
                    const std::string schl = modellSchluessel(it, cv);
                    ModellAnsicht& mv = z.modelle[schl];
                    const int jetzt = static_cast<int>(ImGui::GetTime() * 1000.0);
                    if (mv.modell.schluessel != schl) {
                        (void)ladeMenueModell(z.daten, it, cv, mv.modell);
                        mv.modell.jk2Kamera = z.spiel == Spiel::JK2;   // M5b: Item_Model_Paint unter JK2_MODE
                        mv.texturenDa = false;
                        mv.animStart = jetzt;
                    }
                    // M3 Punkt 2: im Bedienmodus setzt UI_UpdateCharacter die Animation neu (uiScript character)
                    int animStart = mv.animStart;
                    if (z.interaktion && z.laufMenue >= 0 && static_cast<std::size_t>(z.laufMenue) < z.lauf.animGesetzt.size()) {
                        const std::vector<int>& g = z.lauf.animGesetzt[static_cast<std::size_t>(z.laufMenue)];
                        if (static_cast<std::size_t>(p.item) < g.size() && g[static_cast<std::size_t>(p.item)] >= 0) { animStart = g[static_cast<std::size_t>(p.item)]; }
                    }
                    // rc608: je Item ein Ziel. Zwei Items mit demselben Modell teilten sonst ein Bild, und das letzte model_angle galt fuer beide.
                    if (mv.zielJeItem.count(p.item) == 0) { mv.zielJeItem[p.item] = z.naechstesZiel++; }
                    mv.nr = mv.zielJeItem[p.item];
                    const auto figurAnfang = std::chrono::steady_clock::now();   // reste2 Punkt 7: Messung
                    // refdef: x = rect.x + 1, w = rect.w - 2 (in Bildpunkten des Fensters)
                    const float sx = (b.x - a.x) / std::max(1.0F, p.w), sy = (b.y - a.y) / std::max(1.0F, p.h);
                    const ImVec2 ia{a.x + sx, a.y + sy}, ib{b.x - sx, b.y - sy};
                    const int pw = std::clamp(static_cast<int>(ib.x - ia.x), 8, 2048);
                    const int ph = std::clamp(static_cast<int>(ib.y - ia.y), 8, 2048);
                    if (mv.modell.ok && gpu::bereiteMenueZiel(mv.nr, pw, ph)) {
                        if (!mv.texturenDa) {   // Bilder einmal je Modell und Haut (texturesFor wie die Kartenfiguren)
                            mv.texturen = texturesFor(mv.modell.glm);
                            mv.texturenDa = true;
                        }
                        for (const auto& st : mv.texturen.stufen) { z.modellStufen = std::max(z.modellStufen, static_cast<int>(st.size())); }
                        const Dokument* lichtDok = doc();
                        const ModellLicht licht{lichtDok == nullptr || lichtDok->datei.dialekt != Dialekt::MP, z.spiel == Spiel::MD ? 150.0F : 182.0F};
                        const MenueModellBild mb = menueModellBild(mv.modell, it, jetzt, jetzt - animStart, pw, ph, mv.welt, licht);
                        gpu::baueKnochenmatrizen(mv.welt, mv.modell.gla.bones, mv.haut);
                        if (mv.modell.md3) {   // M5b: .md3 starr - ein Knochen, Einheitsmatrix
                            BoneMatrix eins;
                            eins.m[0][0] = eins.m[1][1] = eins.m[2][2] = 1.0F;
                            mv.haut.assign(1, eins);
                        }
                        gpu::FigurLicht l;
                        for (int k = 0; k < 3; ++k) {
                            l.umgebung[k] = mb.umgebung[k];
                            l.gerichtet[k] = mb.gerichtet[k];
                            l.richtung[k] = mb.richtung[k];
                        }
                        l.gitter = true;
                        std::string fehler;
                        z.modellAufrufe += gpu::zeichneMenueFigur(mv.nr, mv.modell.glm, &mv.texturen, mv.haut, mb.viewProj, &l, &fehler);
                        if (!fehler.empty()) { z.modellFehler = fehler; }
                        z.modellMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - figurAnfang).count();
                        ++z.modellZeichnungen;
                        if (void* tex = gpu::menueZielTextur(mv.nr)) {
                            dl->AddImage(reinterpret_cast<ImTextureID>(tex), ia, ib);
                            if ((it.itemFlags & ITF_ISANYSABER) != 0 && (it.itemFlags & ITF_ISCHARACTER) == 0) {
                                zeichneMenueKlingen(dl, mv, it, mb, umg, ia, ib, pw, ph, sx);
                            }
                            break;
                        }
                    }
                }
                [[fallthrough]];
            case PrimArt::Beispiel:   // M7: ownerdraw mit Beispielwerten - nur beim Bearbeiten beschriftet
                if (!Z().interaktion) {
                    const char* t = tr(Str::MeOdBeispiel);
                    const ImVec2 g = ImGui::CalcTextSize(t);
                    const ImVec2 e{a.x + g.x + 6, a.y + g.y + 2};
                    dl->AddRectFilled(a, e, IM_COL32(40, 40, 120, 190));
                    dl->AddText(ImVec2{a.x + 3, a.y + 1}, IM_COL32(200, 210, 255, 255), t);
                }
                break;
            case PrimArt::Platzhalter: {
                const ImU32 f = p.art == PrimArt::Modell ? IM_COL32(90, 160, 255, 200)
                                : p.art == PrimArt::Kino ? IM_COL32(255, 170, 60, 200)
                                                         : IM_COL32(180, 180, 180, 160);
                dl->AddRectFilled(a, b, (f & 0x00FFFFFFU) | 0x30000000U);
                dl->AddRect(a, b, f);
                const std::string t = (p.art == PrimArt::Modell ? "model: " : p.art == PrimArt::Kino ? "cinematic: " : "") + p.shader;
                dl->PushClipRect(a, b, true);
                dl->AddText(ImVec2{a.x + 3, a.y + 2}, f, t.c_str());
                dl->PopClipRect();
                break;
            }
        }
    }
}

void rahmenPlatzhalter(ImDrawList* dl, const Dokument& d, const Menu& m, const Abbildung& ab, ImVec2 o);   // rc608 Teil 3a (app_menue_bildwahl.inc)
bool istVideoRahmen(const Dokument& d, const Item& it);
void auswahlOeffnen(int art, int item);
void zeichneBildWahl();
void zeichneTabLeiste(Dokument& d);   // M6 Punkt 2 (app_menue_m6.inc)
void zeichneGrenzenZeile(Dokument& d);   // M6 Punkt 10
void zeichneTabNummern(ImDrawList* dl, Dokument& d, int mi, const Abbildung& ab, ImVec2 o);
bool tabFolgeKlick(Dokument& d, int mi, float vx, float vy, const Umgebung& u);
// M7 Punkt 12 (app_menue_m7.inc)
bool gizmoVorschau(const Dokument& d, int mi, Datei& kopie);
bool gizmoKlick(Dokument& d, int mi, const Abbildung& ab, ImVec2 o, ImVec2 maus);
int gizmoTreffer(const Dokument& d, int mi, const Abbildung& ab, ImVec2 o, ImVec2 maus);
void zeichneModellGizmo(ImDrawList* dl, Dokument& d, int mi, const Abbildung& ab, ImVec2 o);
void zeichneRefBild(ImDrawList* dl, const Abbildung& ab, ImVec2 o, bool oben);   // M6 Punkt 7
void setzeVorlageEin(std::size_t nr, bool mitZiel, float zx, float zy);

// --- Arbeitsflaeche --------------------------------------------------------------

// M2 Ebenenliste: Item von an die Stelle nach in der Reihenfolge (verschiebt das itemDef im Text)
bool setzeAnStelle(Dokument& d, int menu, int von, int nach) {
    std::vector<Ersetzung> e;
    if (!itemAnStelle(d.datei, menu, von, nach, e)) { return false; }
    aendere(std::move(e));
    if (menuDerSeite(d) == menu) { d.auswahl.assign(1, nach); }
    return true;
}

// M2 Ebenenliste: die Items unter (x, y), vorderstes (zuletzt gezeichnet) zuerst -
// ohne verborgene und ohne die, die das Spiel gerade nicht zeigt (ausser "Verborgene Items")
std::vector<int> itemsUnterPunkt(const Dokument& d, int mi, float x, float y, const Umgebung& u) {
    std::vector<int> r;
    if (mi < 0 || static_cast<std::size_t>(mi) >= d.datei.menus.size()) { return r; }
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    for (std::size_t i = m.items.size(); i-- > 0;) {
        const Rechteck& q = m.items[i].window.rect;
        if (x < q.x || x >= q.x + q.w || y < q.y || y >= q.y + q.h) { continue; }
        if (d.verborgen.count({mi, static_cast<int>(i)}) != 0) { continue; }
        if (!Z().alleItems && !itemSichtbar(m.items[i], m.items[i].window.flags, u)) { continue; }
        r.push_back(static_cast<int>(i));
    }
    return r;
}

// Alt+Klick: das naechste Item unter dem Zeiger nach dem gewaehlten (ringsum)
int naechstesUnter(const std::vector<int>& unter, const std::vector<int>& auswahl) {
    if (unter.empty()) { return -1; }
    if (auswahl.size() == 1) {
        const auto it = std::find(unter.begin(), unter.end(), auswahl[0]);
        if (it != unter.end()) { return *(std::next(it) == unter.end() ? unter.begin() : std::next(it)); }
    }
    return unter.front();
}

// M2 Punkt 3: "open X" aus einer anderen Datei des Menuesatzes - dorthin wechseln
bool satzWechsel() {
    // M2 Punkt 3: "open X" aus einer anderen Datei des Menuesatzes - dorthin wechseln,
    // die Cvars nehmen wir mit (wie im Spiel: alle Menues teilen sich die Cvars)
    Zustand& z = Z();
    Dokument* dp = doc();
    if (dp == nullptr) { return false; }
    Dokument& d = *dp;
    if (!z.lauf.fremdesMenue.empty()) {
        const std::string ziel = z.lauf.fremdesMenue;
        z.lauf.fremdesMenue.clear();
        if (d.satz >= 0 && z.leiste.probeStufe == 0) {   // "Try on canvas" bleibt in dieser Datei
            for (std::size_t k = 0; k < z.docs.size(); ++k) {
                Dokument& od = *z.docs[k];
                if (od.satz != d.satz || &od == &d) { continue; }
                neuParsen(od);
                for (std::size_t q = 0; q < od.datei.menus.size(); ++q) {
                    if (!gleichOhneFallStr(od.datei.menus[q].window.name, ziel)) { continue; }
                    const std::map<std::string, std::string> cvars = z.lauf.cvars;
                    z.aktiv = static_cast<int>(k);
                    z.laufGeparst = od.geparst;
                    z.lauf.start(od.datei);
                    z.lauf.saberModell = saberModellFuer;
                    z.lauf.basis = umgebung(od);
                    z.lauf.cvars = cvars;
                    z.lauf.oeffne(ziel);
                    z.laufDatei = z.aktiv;
                    z.laufVersion = -1;
                    zeigeMenue(od, static_cast<int>(q));
                    z.satzWechsel = ziel;
                    return true;
                }
            }
        }
    }
    return false;
}

void spieleKlaenge();                                              // M2 Punkt 5 (unten)
ImU32 schwereFarbe(Schwere s);                                     // M2 Live-Diagnose (unten)
void schwereSymbol(ImDrawList* dl, ImVec2 m, float r, Schwere s);

// rc608 Punkt 11: ME_VORLAGE traegt eine Bibliothek-Nummer (>= 0) oder ein Widget (-100 - Typ)
constexpr int ablageWidget(int typ) { return -100 - typ; }
constexpr int ablageTyp(int n) { return -100 - n; }
// Vorgabegroesse eines neuen Elements (Punkt 12: das Modell aus den Menues der Spiele)
ImVec2 widgetGroesse(int typ);
void setzeAblageEin(int n, bool mitZiel, float zx, float zy);
ImVec2 ablageGroesse(int n);

// --- rc608 Punkt 5: Text an Ort und Stelle schreiben ---
const Texte& vorschauTexte(Dokument& d);
void eigeneTexteLesen(Dokument& d);
void eigeneTexteSchreiben(Dokument& d);
std::string textFuerDatei(const std::string& sprache, const std::string& utf8, std::string* nichtDarstellbar = nullptr);
std::string textAusDatei(const std::string& sprache, const std::string& bytes);
std::vector<std::string> sprachenListe();

// So kommt ein Text in die Datei: SP liest mit COM_ParseExt (kennt kein Escape, Zeilenumbruch steht roh im String, " ist
// nicht moeglich und wird beim Tippen nicht angenommen), MP mit dem Skript-Lexer aus der Botlib (kein Umbruch im String, dafuer \n).
std::string schreibZeichenkette(const std::string& bytes, Dialekt dialekt) {
    std::string t = "\"";
    for (const char c : bytes) {
        if (dialekt == Dialekt::SP && c == '"') { continue; }   // rc608 Teil 3b: COM_ParseExt kennt kein Escape
        if (dialekt == Dialekt::MP && (c == '\\' || c == '"')) { t += '\\'; }
        if (dialekt == Dialekt::MP && c == '\n') { t += "\\n"; }
        else { t += c; }
    }
    return t + "\"";
}

int schreibRueckruf(ImGuiInputTextCallbackData* c) {
    if (c->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* s = static_cast<std::string*>(c->UserData);
        s->resize(static_cast<std::size_t>(c->BufTextLen));
        c->Buf = s->data();
    } else if (c->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
        // Das Zeichen steht in SP nicht in einem String der Datei
        Dokument* d = doc();
        if (c->EventChar == '"' && d != nullptr && d->datei.dialekt == Dialekt::SP) { return 1; }
    }
    return 0;
}

// Warum ein Element hier nicht beschrieben werden kann; "" = es geht
std::string schreibenHindernis(const Item& it) {
    char b[400];
    if (!it.text.empty()) { return {}; }
    if (it.type == ITEM_TYPE_OWNERDRAW || it.window.ownerDraw != 0) {
        std::snprintf(b, sizeof(b), tr(Str::MeSchreibenOwnerdraw), it.window.ownerDraw);
        return b;
    }
    if (it.type == ITEM_TYPE_LISTBOX) { return tr(Str::MeSchreibenFeeder); }
    if (!it.cvar.empty()) {
        std::snprintf(b, sizeof(b), tr(Str::MeSchreibenCvar), it.cvar.c_str());
        return b;
    }
    if (it.type == ITEM_TYPE_MODEL) { return tr(Str::MeSchreibenModell); }
    if (!it.window.background.empty()) {
        std::snprintf(b, sizeof(b), tr(Str::MeSchreibenBild), it.window.background.c_str());
        return b;
    }
    return {};
}

void schreibenBeginnen(Dokument& d, int menu, int item, ImVec2 klick) {
    Zustand& z = Z();
    const Item& it = d.datei.menus[static_cast<std::size_t>(menu)].items[static_cast<std::size_t>(item)];
    const std::string warum = schreibenHindernis(it);
    if (!warum.empty()) {
        z.meldung = warum;
        ::bhed::gui::meldung(warum, "", false);
        return;
    }
    z.schreib = Zustand::Schreiben{};
    Zustand::Schreiben& s = z.schreib;
    s.an = s.neu = true;
    s.d = &d;
    s.menu = menu;
    s.item = item;
    s.klick = klick;
    s.mehrzeilig = it.type == ITEM_TYPE_TEXTSCROLL || (it.window.flags & (WINDOW_WRAPPED | WINDOW_AUTOWRAPPED)) != 0;
    if (!it.text.empty() && it.text[0] == '@') {
        s.schluessel = it.text;
        s.puffer = textAusDatei(z.sprache, vorschauTexte(d).imSpiel(it.text));
    } else {
        s.puffer = textAusDatei(z.sprache, it.text);
    }
    s.start = s.puffer;
}

// Beendet das Schreiben; Rueckgaengig: ein Schritt, im Text aendert sich nur die Zeile text
void schreibenEnde(bool uebernehmen) {
    Zustand& z = Z();
    if (!z.schreib.an) { return; }
    const Zustand::Schreiben s = std::move(z.schreib);
    z.schreib = Zustand::Schreiben{};
    if (!uebernehmen || s.d == nullptr || s.puffer == s.start) { return; }
    Dokument& d = *s.d;
    std::string neu = s.puffer;
    if (!s.mehrzeilig) { std::replace(neu.begin(), neu.end(), '\n', ' '); }
    std::string nichtDarstellbar;
    const std::string bytes = textFuerDatei(z.sprache, neu, &nichtDarstellbar);
    std::vector<Ersetzung> e;
    const auto zeileSetzen = [&](const std::string& wert) {
        if (setzeEigenschaft(d.datei, s.menu, s.item, "text", {wert}, e)) { aendere(std::move(e)); }
        else { ausMakroMelden(); }
    };
    if (s.schluessel.empty()) {
        zeileSetzen(schreibZeichenkette(bytes, d.datei.dialekt));
        return;
    }
    // Ein Text aus einem Paket: Das Spiel-Paket selbst bleibt, wie es ist. Das Element bekommt einen eigenen Text
    // (Texte-Fenster, Paket des Projekts), die anderen Sprachen behalten ihre Uebersetzung.
    eigeneTexteLesen(d);
    if (d.textPaket.empty()) { d.textPaket = "MYMENU"; }
    // rc608 Teil 3a: Das Paket gehoert zum Schritt, auch wenn sich in der Datei nichts aendert
    const std::map<std::string, std::string> paketVorher = d.bilder;
    const std::string alt = s.schluessel.substr(1);
    const std::string vor = d.textPaket + "_";
    std::string kurz = alt;
    if (kurz.compare(0, vor.size(), vor) == 0) { kurz = kurz.substr(vor.size()); }
    for (char& c : kurz) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    TextZeile* zeile = nullptr;
    for (TextZeile& r : d.eigeneTexte) {
        if (r.schluessel == kurz) { zeile = &r; }
    }
    char m[500];
    if (zeile != nullptr) {
        zeile->text[z.sprache] = bytes;
        eigeneTexteSchreiben(d);
        paketSchrittSeit(d, paketVorher);
        d.puffer.schrittOhneText();
        std::snprintf(m, sizeof(m), tr(Str::MeSchreibenEigenerGeaendert), (vor + kurz).c_str(), z.sprache.c_str());
    } else {
        TextZeile r;
        r.schluessel = kurz;
        for (const std::string& sprache : sprachenListe()) {
            const Texte t = ladeTexte(z.daten, z.spiel, sprache);
            if (t.kennt(s.schluessel)) { r.text[sprache] = t.imSpiel(s.schluessel); }
        }
        r.text[z.sprache] = bytes;
        d.eigeneTexte.push_back(r);
        eigeneTexteSchreiben(d);
        paketSchrittSeit(d, paketVorher);
        zeileSetzen("@" + vor + kurz);
        std::snprintf(m, sizeof(m), tr(Str::MeSchreibenEigenerNeu), alt.c_str(), (vor + kurz).c_str(), z.sprache.c_str());
    }
    z.meldung = m;
    ::bhed::gui::meldung(m, "", false);
}

// Wo steht die Stelle `feld` (Bytes im UTF-8 des Felds) in den gezeichneten Zeilen? Ergebnis virtuell; false = in keiner sichtbaren Zeile.
struct LaufStelle {
    std::size_t lauf = 0;
    std::size_t anfang = 0;   // Byte des Laufs im Text der Datei
};
std::vector<LaufStelle> laufStellen(const std::vector<TextLauf>& laeufe, const std::string& bytes) {
    std::vector<LaufStelle> v;
    std::size_t von = 0;
    for (std::size_t i = 0; i < laeufe.size(); ++i) {
        const std::size_t p = bytes.find(laeufe[i].text, von);
        const std::size_t an = p == std::string::npos ? von : p;
        v.push_back(LaufStelle{i, an});
        von = an + laeufe[i].text.size();
    }
    return v;
}

// rc608 Teil 3b (shank: Hintergrund versehentlich verschoben): Reine Dekoration ueber den ganzen Schirm laesst Klicks durch.
// Bearbeiten geht ueber die Hierarchie, das Rechtsklick-Menue oder Alt+Klick; einmal gewaehlt, laesst sie sich ziehen.
bool durchklickbar(const Item& it) {
    const Rechteck& r = it.window.rect;
    return (it.window.flags & WINDOW_DECORATION) != 0 && r.x <= 0.0F && r.y <= 0.0F && r.x + r.w >= kSchirmBreite && r.y + r.h >= kSchirmHoehe;
}

// Rechtsklick: Befehle laufen im naechsten Bild vor den Panels, das Bild zeichnet gerade aus dem jetzigen Text
enum KontextWunsch { KwNichts, KwAusschneiden, KwKopieren, KwEinfuegen, KwDuplizieren, KwLoeschen, KwVorn, KwHinten, KwAlle };

void kontextEintraege(Dokument& d, int mi, const Menu& m, const std::vector<int>* unter) {
    Zustand& z = Z();
    const std::string strg = tr(Str::MkStrg);
    const bool etwas = !d.auswahl.empty();
    const auto eintrag = [&](Str text, const char* taste, bool an, int wunsch) {
        const std::string kurz = taste != nullptr ? strg + taste : std::string();
        if (ImGui::MenuItem(tr(text), kurz.empty() ? nullptr : kurz.c_str(), false, an)) { z.kontextWunsch = wunsch; }
    };
    eintrag(Str::ActCut, "X", etwas, KwAusschneiden);
    eintrag(Str::ActCopy, "C", etwas, KwKopieren);
    const char* ablage = ImGui::GetClipboardText();
    eintrag(Str::ActPaste, "V", ablage != nullptr && std::strstr(ablage, "itemDef") != nullptr, KwEinfuegen);
    eintrag(Str::EntDuplicate, "D", etwas, KwDuplizieren);
    if (ImGui::MenuItem(tr(Str::ActDelete), tr(Str::MkEntf), false, etwas)) { z.kontextWunsch = KwLoeschen; }
    ImGui::Separator();
    eintrag(Str::MkGanzVorn, nullptr, etwas, KwVorn);
    eintrag(Str::MkGanzHinten, nullptr, etwas, KwHinten);
    ImGui::Separator();
    const bool alleZu = etwas && std::all_of(d.auswahl.begin(), d.auswahl.end(), [&d](int i) { return d.gesperrt.count(i) != 0; });
    if (ImGui::MenuItem(tr(alleZu ? Str::MkEntsperren : Str::MkSperren), nullptr, false, etwas)) {
        for (const int i : d.auswahl) {
            if (alleZu) { d.gesperrt.erase(i); } else { d.gesperrt.insert(i); }
        }
    }
    const bool alleWeg = etwas && std::all_of(d.auswahl.begin(), d.auswahl.end(), [&d, mi](int i) { return d.verborgen.count({mi, i}) != 0; });
    if (ImGui::MenuItem(tr(alleWeg ? Str::MkEinblenden : Str::MkAusblenden), nullptr, false, etwas)) {
        for (const int i : d.auswahl) {
            if (alleWeg) { d.verborgen.erase({mi, i}); } else { d.verborgen.insert({mi, i}); }
        }
    }
    if (ImGui::MenuItem(tr(Str::MkAlsVorlage), nullptr, false, etwas)) {
        z.vorlageSpeichern = true;
        z.vorlageName = tr(Str::MlVorlageVorgabe);
    }
    eintrag(Str::KeKoSelectAll, "A", !m.items.empty(), KwAlle);
    if (unter != nullptr && !unter->empty()) {
        ImGui::Separator();
        ImGui::TextDisabled("%s", tr(Str::MkUnterZeiger));
        for (const int i : *unter) {
            if (static_cast<std::size_t>(i) >= m.items.size()) { continue; }
            const Item& it = m.items[static_cast<std::size_t>(i)];
            char l[300];
            std::snprintf(l, sizeof(l), "%s  (%s)##u%d", it.window.name.empty() ? "?" : it.window.name.c_str(), typName(it.type), i);
            if (ImGui::Selectable(l, std::find(d.auswahl.begin(), d.auswahl.end(), i) != d.auswahl.end())) { waehle(i, false); }
            if (durchklickbar(it) && ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MkHintergrundTip)); }
        }
    }
}

// Ganz nach vorn oder hinten, auch mehrere: ihre Folge bleibt, ein Schritt
void auswahlAnsEnde(Dokument& d, bool nachVorn) {
    const int mi = menuDerSeite(d);
    if (mi < 0 || d.auswahl.empty()) { return; }
    std::vector<int> a = d.auswahl;
    std::sort(a.begin(), a.end());
    const int k = static_cast<int>(a.size());
    const int n = static_cast<int>(d.datei.menus[static_cast<std::size_t>(mi)].items.size());
    const std::string vorher = d.puffer.text();
    for (int j = 0; j < k; ++j) {
        if (nachVorn) {
            (void)setzeAnStelle(d, mi, a[static_cast<std::size_t>(j)] - j, n - 1);
        } else {
            (void)setzeAnStelle(d, mi, a[static_cast<std::size_t>(k - 1 - j)] + j, 0);
        }
        neuParsen(d);
    }
    d.auswahl.clear();
    for (int j = 0; j < k; ++j) { d.auswahl.push_back(nachVorn ? n - k + j : j); }
    if (d.puffer.text() != vorher) { (void)zuEinemSchritt(vorher); }
    neuParsen(d);
}

// Alles, was man auf dieser Seite gerade sieht und bewegen kann; das gilt auch fuer den Rahmen
bool rahmenWaehlbar(const Dokument& d, int mi, const Item& it, int i, const Umgebung& u) {
    return !durchklickbar(it) && d.verborgen.count({mi, i}) == 0 && d.gesperrt.count(i) == 0 && (Z().alleItems || itemSichtbar(it, it.window.flags, u));
}

void zeichneFlaeche(Dokument& d, ImVec2 groesse) {
    Zustand& z = Z();
    const int mi = menuDerSeite(d);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 o0 = ImGui::GetCursorScreenPos();
    const float fsLineal = ImGui::GetFontSize();
    const float lineal = std::floor(fsLineal * 1.2F);
    // Aufloesung: Fenster oder Voreinstellung (das Seitenverhaeltnis der Vorschau)
    static const float kAufl[][2] = {{0, 0}, {640, 480}, {800, 600}, {1024, 768}, {1280, 720}, {1366, 768}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3440, 1440}};
    // rc608 Punkt 8: rechts und unten ein schmaler Rand, sonst laegen die Griffe bei x = 640 und y = 480 halb ausserhalb
    const float griffRand = 6.0F;
    float bw = groesse.x - lineal - griffRand, bh = groesse.y - lineal - griffRand;
    // rc608 Teil 3a: Die Flaeche nimmt das Seitenverhaeltnis des eingestellten Spielfensters, nie das des Felds; ohne Angabe 4:3. Das Spiel streckt
    // 640 x 480 auf sein Fenster, ein hochkantes Fenster zeigt es nie.
    {
        const float ax = z.aufloesung > 0 ? kAufl[z.aufloesung][0] : 4.0F, ay = z.aufloesung > 0 ? kAufl[z.aufloesung][1] : 3.0F;
        const float f = std::max(0.01F, std::min(bw / ax, bh / ay));
        bw = ax * f;
        bh = ay * f;
    }
    const ImVec2 o{o0.x + lineal, o0.y + lineal};
    const Abbildung passend = abbildung(bw, bh, z.seitenv);
    z.flaecheBild = ImVec2{passend.sx * kSchirmBreite, passend.sy * kSchirmHoehe};
    dl->AddRectFilled(o, ImVec2{o.x + bw, o.y + bh}, IM_COL32(0, 0, 0, 255));
    // rc608 Punkt 5: das unsichtbare Eingabefeld liegt ueber dem Element und kommt vor der Flaeche an die Reihe
    const bool schreibt = z.schreib.an && z.schreib.d == &d && !z.interaktion && menuDerSeite(d) == z.schreib.menu;
    ImGuiInputTextState* schreibFeld = nullptr;
    if (z.schreib.an && !schreibt) { schreibenEnde(true); }
    if (schreibt) {
        Zustand::Schreiben& s = z.schreib;
        ImGui::SetCursorScreenPos(s.a);
        if (s.neu) { ImGui::SetKeyboardFocusHere(); }
        // Nicht 0: Mit Alpha 0 versteckt ImGui das Kindfenster des Felds, und jeder Klick hinein beendet es
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.001F);
        const ImVec2 feld{std::max(s.b.x - s.a.x, fsLineal * 2.0F), std::max(s.b.y - s.a.y, fsLineal)};
        const bool enter = ImGui::InputTextMultiline("##schreiben", s.puffer.data(), s.puffer.capacity() + 1, feld,
                                                     ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CtrlEnterForNewLine | ImGuiInputTextFlags_NoHorizontalScroll |
                                                         ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackCharFilter,
                                                     schreibRueckruf, &s.puffer);
        ImGui::PopStyleVar();
        s.puffer.resize(std::strlen(s.puffer.c_str()));
        const ImGuiID kennung = ImGui::GetID("##schreiben");
        const bool aktiv = ImGui::GetActiveID() == kennung;
        schreibFeld = aktiv ? ImGui::GetInputTextState(kennung) : nullptr;
        s.neu = false;
        ImGui::SetCursorScreenPos(o0);
        if (aktiv && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            schreibenEnde(false);
            schreibFeld = nullptr;
        } else if (enter || (s.warAktiv && !aktiv)) {
            schreibenEnde(true);
            schreibFeld = nullptr;
        } else {
            s.warAktiv = aktiv;
        }
    }
    const bool schreibtNoch = schreibt && z.schreib.an;
    ImGui::InvisibleButton("##flaeche", ImVec2{std::max(1.0F, groesse.x), std::max(1.0F, groesse.y)},
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    const bool hover = ImGui::IsItemHovered();
    const ImVec2 maus = ImGui::GetIO().MousePos;
    // rc608 Punkt 4 (shank: "ich kann nicht rein zoomen mit CNTRL + Mausrad"): Strg+Mausrad um den Zeiger, 25 bis 800 %,
    // Strg+0 passend, Strg+1 100 %, Mitteltaste oder Leertaste+Ziehen verschiebt.
    const bool leertaste = !ImGui::GetIO().WantTextInput && ImGui::IsKeyDown(ImGuiKey_Space);
    {
        const ImGuiIO& io = ImGui::GetIO();
        const auto zoomAuf = [&](float sx, float mx, float my) {   // sx: Bildpunkte je virtuellem Punkt, (mx, my) bleibt stehen
            const float neu = std::clamp(sx, 0.25F, 8.0F) / std::max(0.001F, passend.sx);
            const float vx0 = (mx - passend.ox - d.panX) / (passend.sx * d.zoom);
            const float vy0 = (my - passend.oy - d.panY) / (passend.sy * d.zoom);
            d.zoom = neu;
            d.panX = mx - vx0 * passend.sx * neu - passend.ox;
            d.panY = my - vy0 * passend.sy * neu - passend.oy;
        };
        const float jetzt = passend.sx * d.zoom;
        if (d.zoomWunsch == 2) {
            d.zoom = 1.0F;
            d.panX = d.panY = 0.0F;
        } else if (d.zoomWunsch == 3) {
            zoomAuf(1.0F, bw * 0.5F, bh * 0.5F);
        } else if (d.zoomWunsch != 0) {
            zoomAuf(jetzt * (d.zoomWunsch > 0 ? 1.25F : 0.8F), bw * 0.5F, bh * 0.5F);
        }
        d.zoomWunsch = 0;
        if (hover && io.KeyCtrl && io.MouseWheel != 0.0F) { zoomAuf(jetzt * std::pow(1.25F, io.MouseWheel), maus.x - o.x, maus.y - o.y); }
        if (hover && (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) || (leertaste && ImGui::IsMouseClicked(ImGuiMouseButton_Left)))) { z.pannt = true; }
        if (z.pannt) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle) || ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                d.panX += io.MouseDelta.x;
                d.panY += io.MouseDelta.y;
            } else {
                z.pannt = false;
            }
        }
        // Ein Stueck der Flaeche bleibt immer im Bild
        const float bleibt = lineal * 2.0F;
        d.panX = std::clamp(d.panX, bleibt - kSchirmBreite * passend.sx * d.zoom - passend.ox, bw - bleibt - passend.ox);
        d.panY = std::clamp(d.panY, bleibt - kSchirmHoehe * passend.sy * d.zoom - passend.oy, bh - bleibt - passend.oy);
    }
    Abbildung ab = passend;
    ab.sx *= d.zoom;
    ab.sy *= d.zoom;
    ab.ox += d.panX;
    ab.oy += d.panY;
    d.zoomProzent = static_cast<int>(std::lround(ab.sx * 100.0F));
    z.letztesAb = ab;
    z.letztesO = o;
    z.letztesAb = ab;
    z.letztesO = o;
    z.flaeche[0] = o.x + ab.ox;
    z.flaeche[1] = o.y + ab.oy;
    z.flaeche[2] = kSchirmBreite * ab.sx;
    z.flaeche[3] = kSchirmHoehe * ab.sy;
    if (z.pannt || (hover && leertaste)) { ImGui::SetMouseCursor(ImGuiMouseCursor_Hand); }
    const float vx = (maus.x - o.x - ab.ox) / ab.sx;
    const float vy = (maus.y - o.y - ab.oy) / ab.sy;
    // rc608 Punkt 11: Bibliothek und Widgets: Rahmen beim Ziehen, Einrasten, Ablegen genau dort (ein Undo-Schritt)
    z.ablageAn = false;
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("ME_VORLAGE", ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
            int n = 0;
            std::memcpy(&n, pl->Data, sizeof(n));
            const ImVec2 gr = ablageGroesse(n);
            Rechteck r{std::round(vx), std::round(vy), gr.x, gr.y};
            z.linien.clear();
            if (mi >= 0 && !ImGui::GetIO().KeyAlt) {
                std::vector<Rechteck> andere;
                for (const Item& it : d.datei.menus[static_cast<std::size_t>(mi)].items) { andere.push_back(it.window.rect); }
                const std::vector<Rechteck> elemente = andere;
                for (const float x : z.hilfslinienX) { andere.push_back(Rechteck{x, 0, 0, 480}); }
                for (const float y : z.hilfslinienY) { andere.push_back(Rechteck{0, y, 640, 0}); }
                const Einrasten e = einrasten(r, andere, z.rasterWeite, 5.0F / std::max(0.01F, ab.sx), z.anRaster, z.anKanten);
                r.x = std::round(r.x + e.dx);
                r.y = std::round(r.y + e.dy);
                z.linien = e.linien;
                // rc608 Teil 3a: Einrasten darf ein neues Element nicht unbemerkt genau auf ein gleich grosses legen. z.ablageRect zeigt die echte Stelle.
                const Rechteck frei = ohneDeckung(r, elemente);
                if (frei.x != r.x || frei.y != r.y) {
                    r = frei;
                    z.linien.clear();
                }
            }
            z.ablageAn = true;
            z.ablageRect = r;
            if (pl->IsDelivery()) {
                z.ablageAn = false;
                z.linien.clear();
                setzeAblageEin(n, true, r.x, r.y);
            }
        }
        ImGui::EndDragDropTarget();
    }
    if (z.ablageWar && !z.ablageAn && z.ziehen == Ziehen::Nichts) { z.linien.clear(); }   // abgebrochen oder von der Flaeche gezogen
    z.ablageWar = z.ablageAn;
    if (mi < 0) {
        z.letztePrims = 0;
        return;
    }
    // Lineale
    {
        const ImU32 grund = IM_COL32(40, 40, 46, 255), strich = IM_COL32(150, 150, 160, 255);
        dl->AddRectFilled(ImVec2{o0.x, o0.y}, ImVec2{o0.x + groesse.x, o.y}, grund);
        dl->AddRectFilled(ImVec2{o0.x, o0.y}, ImVec2{o.x, o0.y + groesse.y}, grund);
        // Die Zahlen stehen so dicht, wie sie lesbar bleiben; ohne Zoom wie bisher (alle 100, Striche alle 10)
        struct Teilung { int zahl, lang, kurz; };
        static const Teilung kTeilung[] = {{10, 5, 1}, {20, 10, 2}, {50, 10, 5}, {100, 50, 10}, {200, 100, 50}};
        const float dichte = std::min(ab.sx, ab.sy);
        Teilung t = kTeilung[std::size(kTeilung) - 1];
        for (const Teilung& k : kTeilung) {
            if (static_cast<float>(k.zahl) * dichte >= fsLineal * 8.0F) {
                t = k;
                break;
            }
        }
        dl->PushClipRect(ImVec2{o.x, o0.y}, ImVec2{o0.x + groesse.x, o.y}, true);
        for (int v = 0; v <= 640; v += t.kurz) {
            const float x = o.x + ab.x(static_cast<float>(v));
            const float l = (v % t.lang == 0) ? lineal * 0.6F : lineal * 0.25F;
            dl->AddLine(ImVec2{x, o.y - l}, ImVec2{x, o.y}, strich);
            if (v % t.zahl == 0) { dl->AddText(ImVec2{x + 2, o0.y}, strich, std::to_string(v).c_str()); }
        }
        dl->PopClipRect();
        dl->PushClipRect(ImVec2{o0.x, o.y}, ImVec2{o.x, o0.y + groesse.y}, true);
        for (int v = 0; v <= 480; v += t.kurz) {
            const float y = o.y + ab.y(static_cast<float>(v));
            const float l = (v % t.lang == 0) ? lineal * 0.6F : lineal * 0.25F;
            dl->AddLine(ImVec2{o.x - l, y}, ImVec2{o.x, y}, strich);
            if (v % t.zahl == 0) { dl->AddText(ImVec2{o0.x + 1, y + 1}, strich, std::to_string(v).c_str()); }
        }
        dl->PopClipRect();
        if (hover) {
            dl->AddLine(ImVec2{maus.x, o0.y}, ImVec2{maus.x, o.y}, IM_COL32(255, 200, 0, 255));
            dl->AddLine(ImVec2{o0.x, maus.y}, ImVec2{o.x, maus.y}, IM_COL32(255, 200, 0, 255));
        }
    }
    dl->PushClipRect(o, ImVec2{o.x + bw, o.y + bh}, true);
    // rc608 Punkt 9: Ohne fullScreen sieht man im Spiel durch das Menue. Das Schachbrett zeigt das; nur Anzeige, nie in der Datei.
    z.schachbrettZellen = 0;
    if (z.schachbrett && d.datei.menus[static_cast<std::size_t>(mi)].fullScreen == 0) {
        const float zelle = std::max(8.0F, std::floor(fsLineal * 0.75F));
        const float x0 = o.x + ab.x(0), y0 = o.y + ab.y(0), x1 = o.x + ab.x(kSchirmBreite), y1 = o.y + ab.y(kSchirmHoehe);
        dl->AddRectFilled(ImVec2{x0, y0}, ImVec2{x1, y1}, IM_COL32(44, 44, 50, 255));
        int zeile = 0;
        for (float y = y0; y < y1; y += zelle, ++zeile) {
            for (float x = x0 + ((zeile & 1) != 0 ? zelle : 0.0F); x < x1; x += zelle * 2.0F) {
                dl->AddRectFilled(ImVec2{x, y}, ImVec2{std::min(x + zelle, x1), std::min(y + zelle, y1)}, IM_COL32(64, 64, 72, 255));
                ++z.schachbrettZellen;
            }
        }
    }
    zeichneRefBild(dl, ab, o, false);   // M6 Punkt 7: unter dem Menue
    // Vorschau
    Umgebung u = umgebung(d);
    const Datei* quelle = &d.datei;
    if (z.interaktion) {
        if (z.laufDatei != z.aktiv || z.lauf.d != &d.datei || z.laufVersion < 0 || z.laufGeparst != d.geparst) {
            z.laufGeparst = d.geparst;
            z.lauf.start(d.datei);
            z.lauf.saberModell = saberModellFuer;
            z.lauf.basis = u;
            z.lauf.jetztMs = static_cast<int>(ImGui::GetTime() * 1000.0);   // DC->realTime fuer die onOpen-Skripte
            z.lauf.fokusKlangStd = z.fokusKlang[d.datei.dialekt == Dialekt::MP ? 1 : 0];   // schon fuer setfocus in onOpen
            z.lauf.oeffne(d.datei.menus[static_cast<std::size_t>(mi)].window.name);
            z.laufDatei = z.aktiv;
            z.laufVersion = -1;
            z.laufMausX = -1.0F;   // Menus_OpenByName: "handle a mouse move on the new menu in case your already over an item"
            z.laufMausY = -1.0F;
        }
        // M2: transition/orbit/fadein/fadeout im Takt der Engine (ein Item_Paint je Spielbild)
        {
            const int k = d.datei.dialekt == Dialekt::MP ? 1 : 0;
            z.lauf.fadeClampStd = z.fadeClamp[k];
            z.lauf.fokusKlangStd = z.fokusKlang[k];
            z.lauf.fadeAmountStd = z.fadeAmount[k];
            z.lauf.fadeCycleStd = z.fadeCycle[k];
            z.lauf.spielFps = z.spielFps;
            (void)z.lauf.takt(static_cast<int>(ImGui::GetTime() * 1000.0));
        }
        if (z.laufVersion != z.lauf.version) {
            z.laufKopie = d.datei;
            z.lauf.anwenden(z.laufKopie);
            z.laufVersion = z.lauf.version;
        }
        spieleKlaenge();
        z.lauf.fuelle(mi, u);
        u.schrift = umgebung(d).schrift;
        quelle = &z.laufKopie;
    } else {
        // ui_tab-Seite: deren Cvar in der Vorschau setzen
        const std::vector<Seite> s = seitenImMenue(d.datei);
        const Seite& sei = s[static_cast<std::size_t>(std::clamp(d.seite, 0, static_cast<int>(s.size()) - 1))];
        if (!sei.cvar.empty()) { z.vorschauCvars[kleinPfad(sei.cvar)] = sei.wert; }
    }
    Datei gizmoKopie;   // M7 Punkt 12: waehrend des Ziehens am Griff die neuen Werte im Bild
    if (z.gizmoArt != 0) {
        gizmoKopie = *quelle;
        if (gizmoVorschau(d, mi, gizmoKopie)) { quelle = &gizmoKopie; }
    }
    Datei schreibKopie;   // rc608 Punkt 5: das Getippte, gezeichnet wie im Spiel
    if (schreibtNoch && static_cast<std::size_t>(z.schreib.item) < quelle->menus[static_cast<std::size_t>(mi)].items.size()) {
        schreibKopie = *quelle;
        schreibKopie.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(z.schreib.item)].text = textFuerDatei(z.sprache, z.schreib.puffer, nullptr);
        quelle = &schreibKopie;
        z.schreib.laeufe.clear();
        u.textLaeufe = &z.schreib.laeufe;
        u.textLaeufeItem = z.schreib.item;
        u.textLaeufeStart = z.schreib.startPos;
    }
    std::vector<Prim> prims;
    if (z.hudWie && istHud(*quelle)) {   // M3: lefthud, righthud und was cgame darueber legt
        z.hudWerte.zeitMs = static_cast<int>(ImGui::GetTime() * 1000.0);
        zeichneHud(*quelle, mi, u, z.hudWerte, z.hudZustand, prims);
    } else if (z.hudWie && (istLadebild(*quelle) || istVerbindung(*quelle))) {   // M7 Punkt 11: mit Beispielwerten
        const auto se = [tx = u.texte](const std::string& key) { return tx != nullptr ? tx->imSpiel("@" + key) : key; };
        if (istLadebild(*quelle)) {
            datapadDatenLaden();
            LadeKontext lk;
            lk.se = se;
            lk.bildDa = u.bildDa;
            lk.waffenIcon = z.waffenIcon;
            lk.spieler = z.datapadWerte;
            zeichneLadebild(*quelle, mi, u, z.ladeWerte, lk, prims);
        } else {
            zeichneVerbindung(*quelle, mi, u, z.verbindungsWerte, se, prims);
        }
        Prim b;   // beschriftet: Werte aus dem Laden/Verbinden sind Beispiele
        b.art = PrimArt::Beispiel;
        b.w = 640;
        b.h = 480;
        prims.push_back(b);
    } else {
        zeichneMenu(*quelle, mi, u, prims);
    }
    if (!d.verborgen.empty()) {   // M2: Auge in der Ebenenliste (nur Vorschau, nichts im Text)
        prims.erase(std::remove_if(prims.begin(), prims.end(), [&](const Prim& p) { return d.verborgen.count({mi, p.item}) != 0; }),
                    prims.end());
    }
    z.letztePrims = static_cast<int>(prims.size());
    z.beispielPrims = static_cast<int>(std::count_if(prims.begin(), prims.end(), [](const Prim& p) { return p.art == PrimArt::Beispiel; }));
    z.platzhalterPrims = static_cast<int>(std::count_if(prims.begin(), prims.end(), [](const Prim& p) { return p.art == PrimArt::Platzhalter; }));
    z.ladebalken = static_cast<int>(std::count_if(prims.begin(), prims.end(), [](const Prim& p) { return p.shader.compare(0, 13, "gfx/hud/load_") == 0 || p.shader == "gfx/hud/mp_levelload"; }));
    z.asienPrims = static_cast<int>(std::count_if(prims.begin(), prims.end(), [](const Prim& p) {
        return p.shader.compare(0, 6, "fonts/") == 0 && p.shader.find("_1024_") != std::string::npos;
    }));
    z.hudPrims = static_cast<int>(std::count_if(prims.begin(), prims.end(), [](const Prim& p) {
        return p.shader.compare(0, 15, "gfx/2d/numbers/") == 0 || p.shader.compare(0, 16, "gfx/2d/crosshair") == 0;
    }));
    const Menu& m = quelle->menus[static_cast<std::size_t>(mi)];
    std::set<int> sel(d.auswahl.begin(), d.auswahl.end());
    const bool verschiebt = z.ziehen == Ziehen::Verschieben;
    z.laufMenue = z.interaktion ? mi : -1;
    zeichnePrims(dl, prims, ab, o, verschiebt ? sel : std::set<int>{}, z.zdx, z.zdy, &m, &u);
    zeichneRefBild(dl, ab, o, true);   // M6 Punkt 7: ueber dem Menue
    zeichneTabNummern(dl, d, mi, ab, o);   // M6 Punkt 2
    zeichneModellGizmo(dl, d, mi, ab, o);   // M7 Punkt 12
    // Raster
    if (z.raster && !z.interaktion && z.rasterWeite >= 2) {
        for (float x = 0; x <= 640; x += z.rasterWeite) {
            dl->AddLine(ImVec2{o.x + ab.x(x), o.y + ab.y(0)}, ImVec2{o.x + ab.x(x), o.y + ab.y(480)}, IM_COL32(255, 255, 255, 18));
        }
        for (float y = 0; y <= 480; y += z.rasterWeite) {
            dl->AddLine(ImVec2{o.x + ab.x(0), o.y + ab.y(y)}, ImVec2{o.x + ab.x(640), o.y + ab.y(y)}, IM_COL32(255, 255, 255, 18));
        }
    }
    for (const float x : z.hilfslinienX) {
        dl->AddLine(ImVec2{o.x + ab.x(x), o.y}, ImVec2{o.x + ab.x(x), o.y + bh}, IM_COL32(0, 200, 255, 160));
    }
    for (const float y : z.hilfslinienY) {
        dl->AddLine(ImVec2{o.x, o.y + ab.y(y)}, ImVec2{o.x + bw, o.y + ab.y(y)}, IM_COL32(0, 200, 255, 160));
    }

    if (d.zoom != 1.0F || d.panX != 0.0F || d.panY != 0.0F) {   // rc608 Punkt 4: wo der Schirm des Spiels endet
        dl->AddRect(ImVec2{o.x + ab.x(0), o.y + ab.y(0)}, ImVec2{o.x + ab.x(kSchirmBreite), o.y + ab.y(kSchirmHoehe)}, IM_COL32(150, 150, 160, 160));
    }

    if (z.interaktion) {
        dl->PopClipRect();
        if (hover) {
            // rc610 F46 B6-4: der Zeiger des Spiels steht auf ganzen Bildpunkten des 640x480-Schirms (uiDC.cursorx/y, int),
            // Menu_HandleMouseMove laeuft nur bei einer Bewegung (UI_MouseEvent -> Display_MouseMove)
            const float cx = std::clamp(std::floor(vx), 0.0F, kSchirmBreite);
            const float cy = std::clamp(std::floor(vy), 0.0F, kSchirmHoehe);
            if (cx != z.laufMausX || cy != z.laufMausY) {
                z.laufMausX = cx;
                z.laufMausY = cy;
                z.lauf.maus(mi, cx, cy);
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !leertaste) { z.lauf.klick(mi, cx, cy); }
            if (ImGui::GetIO().MouseWheel != 0.0F && !ImGui::GetIO().KeyCtrl) { z.lauf.rad(mi, cx, cy, static_cast<int>(ImGui::GetIO().MouseWheel)); }   // M4: Textscroll (Movie Duels)
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) { z.lauf.escape(mi); }
        }
        if (satzWechsel()) { return; }   // M2 Punkt 3
        // Ein Menue, das ein Skript geoeffnet hat: dessen Seite zeigen
        for (std::size_t k = 0; k < z.lauf.menuFlags.size(); ++k) {
            if ((z.lauf.menuFlags[k] & WINDOW_HASFOCUS) != 0 && static_cast<int>(k) != mi) { zeigeMenue(d, static_cast<int>(k)); }
        }
        return;
    }

    // --- Bearbeiten: Auswahl, Griffe, Ziehen ---
    auto bild = [&](const Rechteck& r) {
        return std::pair<ImVec2, ImVec2>{ImVec2{o.x + ab.x(r.x), o.y + ab.y(r.y)}, ImVec2{o.x + ab.x(r.x + r.w), o.y + ab.y(r.y + r.h)}};
    };
    // M2 Live-Diagnose: fehlerhafte Items auf der Flaeche (Fehler rot, Warnung gelb)
    for (const Diagnose& x : d.diagnosen) {
        if (x.menu != mi || x.item < 0 || x.schwere == Schwere::Hinweis || static_cast<std::size_t>(x.item) >= m.items.size()) { continue; }
        const auto [a, b] = bild(m.items[static_cast<std::size_t>(x.item)].window.rect);
        dl->AddRect(a, b, schwereFarbe(x.schwere), 0.0F, 0, 1.5F);
        schwereSymbol(dl, ImVec2{a.x + 5, a.y + 5}, 4.0F, x.schwere);
    }
    // M2: alle Items unter dem Zeiger, vorderstes zuerst; Alt+Klick waehlt der Reihe nach durch
    const std::vector<int> alleUnter = itemsUnterPunkt(d, mi, vx, vy, u);
    int unterMaus = -1;
    for (const int i : alleUnter) {
        if (!durchklickbar(m.items[static_cast<std::size_t>(i)]) || std::find(d.auswahl.begin(), d.auswahl.end(), i) != d.auswahl.end()) {
            unterMaus = i;
            break;
        }
    }
    if (ImGui::GetIO().KeyAlt && !alleUnter.empty()) { unterMaus = naechstesUnter(alleUnter, d.auswahl); }
    if (hover && !schreibt && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {   // rc608 Teil 3b: wie in jedem Zeichenprogramm: Das Element unter dem Zeiger ist gemeint
        z.unterZeiger = alleUnter;
        if (unterMaus >= 0 && std::find(d.auswahl.begin(), d.auswahl.end(), unterMaus) == d.auswahl.end()) { d.auswahl.assign(1, unterMaus); }
        ImGui::OpenPopup("##flaechekontext");
    }
    if (ImGui::BeginPopup("##flaechekontext")) {
        kontextEintraege(d, mi, m, &z.unterZeiger);
        ImGui::EndPopup();
    }
    if (hover && unterMaus >= 0 && z.ziehen == Ziehen::Nichts) {
        const auto [a, b] = bild(m.items[static_cast<std::size_t>(unterMaus)].window.rect);
        dl->AddRect(a, b, IM_COL32(255, 255, 255, 90));
        ImGui::SetTooltip("%s  (%s)", m.items[static_cast<std::size_t>(unterMaus)].window.name.c_str(),
                          typName(m.items[static_cast<std::size_t>(unterMaus)].type));
    }
    // Griffe der Einzelauswahl
    const float hs = 4.0F;
    int griffUnter = -1;
    Rechteck einzel;
    if (d.auswahl.size() == 1 && static_cast<std::size_t>(d.auswahl[0]) < m.items.size()) {
        einzel = m.items[static_cast<std::size_t>(d.auswahl[0])].window.rect;
        const float gx[3] = {einzel.x, einzel.x + einzel.w * 0.5F, einzel.x + einzel.w};
        const float gy[3] = {einzel.y, einzel.y + einzel.h * 0.5F, einzel.y + einzel.h};
        for (int yy = 0; yy < 3; ++yy) {
            for (int xx = 0; xx < 3; ++xx) {
                if (xx == 1 && yy == 1) { continue; }
                const ImVec2 c{o.x + ab.x(gx[xx]), o.y + ab.y(gy[yy])};
                if (std::fabs(maus.x - c.x) <= hs + 1 && std::fabs(maus.y - c.y) <= hs + 1) { griffUnter = yy * 3 + xx; }
            }
        }
        // rc608 Punkt 8: auch die Raender zwischen den Griffen skalieren (Griff der Kantenmitte: 1 oben, 3 links, 5 rechts, 7 unten)
        if (griffUnter < 0) {
            const float rl = o.x + ab.x(einzel.x), rr = o.x + ab.x(einzel.x + einzel.w);
            const float rt = o.y + ab.y(einzel.y), rb = o.y + ab.y(einzel.y + einzel.h);
            const bool imX = maus.x >= rl && maus.x <= rr;
            const bool imY = maus.y >= rt && maus.y <= rb;
            if (imY && std::fabs(maus.x - rl) <= hs) { griffUnter = 3; }
            else if (imY && std::fabs(maus.x - rr) <= hs) { griffUnter = 5; }
            else if (imX && std::fabs(maus.y - rt) <= hs) { griffUnter = 1; }
            else if (imX && std::fabs(maus.y - rb) <= hs) { griffUnter = 7; }
        }
        if (d.gesperrt.count(d.auswahl[0]) != 0 || (!hover && z.ziehen == Ziehen::Nichts)) { griffUnter = -1; }
    }
    // rc608 Punkt 8: der Zeiger sagt, was ein Druck hier tut, und bleibt beim Ziehen
    {
        const auto griffZeiger = [](int g) {
            if (g == 0 || g == 8) { return ImGuiMouseCursor_ResizeNWSE; }
            if (g == 2 || g == 6) { return ImGuiMouseCursor_ResizeNESW; }
            return g == 1 || g == 7 ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW;
        };
        const auto waehlbar = std::find(d.auswahl.begin(), d.auswahl.end(), unterMaus) != d.auswahl.end();
        const bool frei = std::none_of(d.auswahl.begin(), d.auswahl.end(), [&d](int i) { return d.gesperrt.count(i) != 0; });
        if (z.pannt || leertaste || z.gizmoArt != 0 || gizmoTreffer(d, mi, ab, o, maus) != 0) {
            // Hand (oben gesetzt) oder der Zeiger des Modell-Griffs
        } else if (z.ziehen == Ziehen::Groesse) {
            ImGui::SetMouseCursor(griffZeiger(z.griff));
        } else if (z.ziehen == Ziehen::Verschieben) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        } else if (z.ziehen == Ziehen::Nichts && griffUnter >= 0) {
            ImGui::SetMouseCursor(griffZeiger(griffUnter));
        } else if (z.ziehen == Ziehen::Nichts && hover && waehlbar && frei) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        }
    }
    // Rahmen und Griffe duerfen ueber den Rand der Flaeche ragen (bei x = 0 ueber das Lineal)
    dl->PushClipRect(ImVec2{o.x - hs - 1.0F, o.y - hs - 1.0F}, ImVec2{o0.x + groesse.x, o0.y + groesse.y}, false);
    for (const int i : d.auswahl) {
        if (static_cast<std::size_t>(i) >= m.items.size()) { continue; }
        Rechteck r = m.items[static_cast<std::size_t>(i)].window.rect;
        if (verschiebt) {
            r.x += z.zdx;
            r.y += z.zdy;
        }
        if (z.ziehen == Ziehen::Groesse && d.auswahl.size() == 1) { r = z.groesseNeu; }
        const auto [a, b] = bild(r);
        const bool gesperrt = d.gesperrt.count(i) != 0;
        dl->AddRect(a, b, gesperrt ? IM_COL32(255, 80, 80, 255) : IM_COL32(60, 170, 255, 255), 0, 0, 1.5F);
        if (d.auswahl.size() == 1 && !gesperrt) {
            const float gx[3] = {a.x, (a.x + b.x) * 0.5F, b.x};
            const float gy[3] = {a.y, (a.y + b.y) * 0.5F, b.y};
            for (int yy = 0; yy < 3; ++yy) {
                for (int xx = 0; xx < 3; ++xx) {
                    if (xx == 1 && yy == 1) { continue; }
                    dl->AddRectFilled(ImVec2{gx[xx] - hs, gy[yy] - hs}, ImVec2{gx[xx] + hs, gy[yy] + hs}, IM_COL32(255, 255, 255, 255));
                    dl->AddRect(ImVec2{gx[xx] - hs, gy[yy] - hs}, ImVec2{gx[xx] + hs, gy[yy] + hs}, IM_COL32(30, 90, 200, 255));
                }
            }
            // Abstandsanzeige zum Bildschirmrand
            char t[96];
            std::snprintf(t, sizeof(t), "%g, %g  %g x %g", static_cast<double>(r.x), static_cast<double>(r.y), static_cast<double>(r.w),
                          static_cast<double>(r.h));
            dl->AddText(ImVec2{a.x, b.y + 2}, IM_COL32(60, 170, 255, 255), t);
        }
    }
    dl->PopClipRect();
    for (const Hilfslinie& h : z.linien) {
        if (h.senkrecht) {
            dl->AddLine(ImVec2{o.x + ab.x(h.wert), o.y + ab.y(h.von)}, ImVec2{o.x + ab.x(h.wert), o.y + ab.y(h.bis)}, IM_COL32(255, 60, 200, 255));
        } else {
            dl->AddLine(ImVec2{o.x + ab.x(h.von), o.y + ab.y(h.wert)}, ImVec2{o.x + ab.x(h.bis), o.y + ab.y(h.wert)}, IM_COL32(255, 60, 200, 255));
        }
    }
    dl->PopClipRect();

    // rc608 Punkt 5: Cursor und Auswahl im Text, wie gezeichnet; die Maus setzt sie um
    if (schreibtNoch) {
        Zustand::Schreiben& s = z.schreib;
        const auto [ra, rb] = bild(m.items[static_cast<std::size_t>(s.item)].window.rect);
        s.a = ra;
        s.b = rb;
        s.textRechts = 0.0F;
        for (const Prim& p : prims) {
            if (p.item == s.item && p.shader.compare(0, 6, "fonts/") == 0) { s.textRechts = std::max(s.textRechts, p.x + p.w); }
        }
        const std::string bytes = textFuerDatei(z.sprache, s.puffer, nullptr);
        const auto dateiStelle = [&](int feld) { return textFuerDatei(z.sprache, s.puffer.substr(0, static_cast<std::size_t>(std::clamp(feld, 0, static_cast<int>(s.puffer.size())))), nullptr).size(); };
        const std::vector<LaufStelle> stellen = laufStellen(s.laeufe, bytes);
        const auto xVon = [&](const LaufStelle& l, std::size_t byte) {
            const TextLauf& r = s.laeufe[l.lauf];
            if (r.schrift == nullptr) { return r.x; }
            const std::size_t n = byte > l.anfang ? std::min(byte - l.anfang, r.text.size()) : 0;
            return r.x + static_cast<float>(schriftBreite(*r.schrift, r.text.substr(0, n), r.scale, u.kodierung, u.asienGlyphen, u.jk2));
        };
        // Mausstelle: die naechste Zeile, darin das naechste Zeichen, als Stelle im Feld
        const auto feldBei = [&](float mx, float my) {
            if (stellen.empty()) { return static_cast<int>(s.puffer.size()); }
            std::size_t beste = 0;
            float abstand = 1e9F;
            for (std::size_t k = 0; k < stellen.size(); ++k) {
                const TextLauf& r = s.laeufe[stellen[k].lauf];
                const float hoehe = r.schrift != nullptr ? static_cast<float>(schriftHoehe(*r.schrift, r.scale, u.jk2)) : 16.0F;
                const float w = my < r.y ? r.y - my : (my > r.y + hoehe ? my - r.y - hoehe : 0.0F);
                if (w < abstand) {
                    abstand = w;
                    beste = k;
                }
            }
            const LaufStelle& l = stellen[beste];
            const std::size_t anfang = l.anfang, ende = l.anfang + s.laeufe[l.lauf].text.size();
            int gefunden = -1;
            float best = 1e9F;
            for (int f = 0; f <= static_cast<int>(s.puffer.size()); ++f) {
                if (f < static_cast<int>(s.puffer.size()) && (static_cast<unsigned char>(s.puffer[static_cast<std::size_t>(f)]) & 0xC0U) == 0x80U) { continue; }
                const std::size_t by = dateiStelle(f);
                if (by < anfang || by > ende) { continue; }
                const float dx = std::fabs(xVon(l, by) - mx);
                if (dx < best) {
                    best = dx;
                    gefunden = f;
                }
            }
            return gefunden >= 0 ? gefunden : static_cast<int>(s.puffer.size());
        };
        const float mvx = (maus.x - o.x - ab.ox) / ab.sx, mvy = (maus.y - o.y - ab.oy) / ab.sy;
        const bool drin = maus.x >= s.a.x && maus.x <= s.b.x && maus.y >= s.a.y && maus.y <= s.b.y;
        if (schreibFeld != nullptr) {
            if (s.cursorWunsch < -1 && !stellen.empty()) {
                const int f = feldBei((s.klick.x - o.x - ab.ox) / ab.sx, (s.klick.y - o.y - ab.oy) / ab.sy);
                schreibFeld->SetSelection(f, f);
                s.cursorWunsch = -1;
            }
            if (drin && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                s.anker = feldBei(mvx, mvy);
                s.zieht = true;
            }
            if (s.zieht) {
                schreibFeld->SetSelection(s.anker, feldBei(mvx, mvy));
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) { s.zieht = false; }
            }
        }
        const int cur = schreibFeld != nullptr ? schreibFeld->GetCursorPos() : static_cast<int>(s.puffer.size());
        const int sa = schreibFeld != nullptr ? schreibFeld->GetSelectionStart() : cur;
        const int se = schreibFeld != nullptr ? schreibFeld->GetSelectionEnd() : cur;
        const std::size_t cb = dateiStelle(cur), sab = dateiStelle(std::min(sa, se)), seb = dateiStelle(std::max(sa, se));
        dl->PushClipRect(o, ImVec2{o.x + bw, o.y + bh}, true);
        bool cursorGezeichnet = false;
        for (const LaufStelle& l : stellen) {
            const TextLauf& r = s.laeufe[l.lauf];
            const float hoehe = r.schrift != nullptr ? static_cast<float>(schriftHoehe(*r.schrift, r.scale, u.jk2)) : 16.0F;
            const std::size_t ende = l.anfang + r.text.size();
            if (sab < seb && seb > l.anfang && sab < ende) {
                dl->AddRectFilled(ImVec2{o.x + ab.x(xVon(l, std::max(sab, l.anfang))), o.y + ab.y(r.y)}, ImVec2{o.x + ab.x(xVon(l, std::min(seb, ende))), o.y + ab.y(r.y + hoehe)}, IM_COL32(60, 130, 255, 110));
            }
            if (!cursorGezeichnet && cb >= l.anfang && cb <= ende) {
                cursorGezeichnet = true;
                s.cursorX = xVon(l, cb);
                s.cursorY = r.y;
                if (std::fmod(ImGui::GetTime(), 1.0) < 0.6) {
                    dl->AddLine(ImVec2{o.x + ab.x(s.cursorX), o.y + ab.y(r.y)}, ImVec2{o.x + ab.x(s.cursorX), o.y + ab.y(r.y + hoehe)}, IM_COL32(255, 255, 255, 255), 2.0F);
                }
            }
        }
        if (stellen.empty()) {   // noch kein Text: der Cursor steht, wo der Text anfangen wird
            const Item& gi = m.items[static_cast<std::size_t>(s.item)];
            s.cursorX = gi.window.rect.x + gi.textalignx;
            s.cursorY = gi.window.rect.y + gi.textaligny;
            if (std::fmod(ImGui::GetTime(), 1.0) < 0.6) {
                dl->AddLine(ImVec2{o.x + ab.x(s.cursorX), o.y + ab.y(s.cursorY)}, ImVec2{o.x + ab.x(s.cursorX), o.y + ab.y(s.cursorY) + fsLineal}, IM_COL32(255, 255, 255, 255), 2.0F);
            }
        } else if (!cursorGezeichnet) {   // Textscroll: der Cursor liegt ausserhalb der sichtbaren Zeilen
            s.startPos = std::max(0, s.startPos + (cb < stellen.front().anfang ? -1 : 1));
        }
        dl->PopClipRect();
        if (drin) { ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput); }
    }
    rahmenPlatzhalter(dl, d, m, ab, o);   // rc608 Teil 3a
    if (z.ablageAn) {   // rc608 Punkt 11: wo es landen wird
        const auto [ra, rb] = bild(z.ablageRect);
        dl->AddRectFilled(ra, rb, IM_COL32(60, 170, 255, 40));
        dl->AddRect(ra, rb, IM_COL32(60, 170, 255, 255), 0, 0, 1.5F);
    }
    if (z.ziehen == Ziehen::Rahmen && z.ziehLaeuft) {   // rc608 Teil 3b: Rahmen aufziehen
        const float rx0 = std::min(z.ziehStart.x, vx), rx1 = std::max(z.ziehStart.x, vx);
        const float ry0 = std::min(z.ziehStart.y, vy), ry1 = std::max(z.ziehStart.y, vy);
        dl->PushClipRect(o0, ImVec2{o0.x + groesse.x, o0.y + groesse.y}, true);
        const ImVec2 ra{o.x + ab.x(rx0), o.y + ab.y(ry0)}, rb{o.x + ab.x(rx1), o.y + ab.y(ry1)};
        dl->AddRectFilled(ra, rb, IM_COL32(60, 170, 255, 36));
        dl->AddRect(ra, rb, IM_COL32(60, 170, 255, 255));
        dl->PopClipRect();
        d.auswahl = z.rahmenBasis;
        for (std::size_t i = 0; i < m.items.size(); ++i) {
            const Rechteck& q = m.items[i].window.rect;
            const bool beruehrt = q.x < rx1 && q.x + q.w > rx0 && q.y < ry1 && q.y + q.h > ry0;
            if (beruehrt && rahmenWaehlbar(d, mi, m.items[i], static_cast<int>(i), u) && std::find(d.auswahl.begin(), d.auswahl.end(), static_cast<int>(i)) == d.auswahl.end()) {
                d.auswahl.push_back(static_cast<int>(i));
            }
        }
    }
    // rc608 Punkt 5: Doppelklick schreibt und zieht nie; gemeint ist das vorderste Element mit Text unter dem Zeiger
    bool textUnter = false;
    for (const int i : alleUnter) { textUnter = textUnter || !m.items[static_cast<std::size_t>(i)].text.empty(); }
    if (!schreibt && hover && !leertaste && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && (unterMaus >= 0 || textUnter)) {
        int ziel = unterMaus >= 0 ? unterMaus : alleUnter.front();
        for (const int i : alleUnter) {
            if (!m.items[static_cast<std::size_t>(i)].text.empty()) {
                ziel = i;
                break;
            }
        }
        z.ziehen = Ziehen::Nichts;
        z.ziehLaeuft = false;
        d.auswahl.assign(1, ziel);
        // rc608 Teil 3a: Auf einem Rahmen oeffnet der Doppelklick die Auswahl, dasselbe Fenster dient zum Wechseln
        if (istBildRahmen(m.items[static_cast<std::size_t>(ziel)])) {
            auswahlOeffnen(istVideoRahmen(d, d.datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(ziel)]) ? 1 : 0, ziel);
            return;
        }
        schreibenBeginnen(d, mi, ziel, maus);
        if (z.schreib.an) {
            const auto [ra, rb] = bild(m.items[static_cast<std::size_t>(ziel)].window.rect);
            z.schreib.a = ra;
            z.schreib.b = rb;
            z.schreib.cursorWunsch = -2;
        }
        return;
    }
    // Maus: Klick waehlt, Ziehen verschiebt/aendert die Groesse (M6: bei Tab-Reihenfolge waehlt er die naechste Nummer)
    if (hover && !schreibt && !leertaste && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !tabFolgeKlick(d, mi, vx, vy, u) && !gizmoKlick(d, mi, ab, o, maus)) {
        ImGui::SetKeyboardFocusHere(-1);
        z.codeFokus = false;
        z.ziehStart = ImVec2{vx, vy};
        z.ziehDruck = maus;
        z.ziehLaeuft = false;
        z.klickWahl = -1;
        z.zdx = z.zdy = 0;
        if (griffUnter >= 0 && d.auswahl.size() == 1 && d.gesperrt.count(d.auswahl[0]) == 0) {
            z.ziehen = Ziehen::Groesse;
            z.griff = griffUnter;
            z.groesseNeu = einzel;
        } else if (maus.y < o.y && maus.x > o.x) {
            z.ziehen = Ziehen::Lineal;   // Hilfslinie aus dem oberen Lineal
            z.griff = 1;
        } else if (maus.x < o.x && maus.y > o.y) {
            z.ziehen = Ziehen::Lineal;
            z.griff = 0;
        } else if (unterMaus >= 0) {
            // rc608 (shank): ein Druck meint das vorderste Element unter dem Zeiger, das mit dem Rahmen und dem
            // Namen. Ist es noch nicht gewaehlt, waehlt er es nur; erst ein neuer Druck auf Gewaehltes zieht.
            // Alt: der Klick geht beim Loslassen durch den Stapel, Ziehen bewegt das Gewaehlte auch verdeckt.
            const ImGuiIO& io = ImGui::GetIO();
            const bool dazu = io.KeyShift || io.KeyCtrl;
            const auto it = std::find(d.auswahl.begin(), d.auswahl.end(), unterMaus);
            const bool gewaehltDarunter = std::any_of(alleUnter.begin(), alleUnter.end(), [&d](int i) {
                return std::find(d.auswahl.begin(), d.auswahl.end(), i) != d.auswahl.end();
            });
            bool ziehen = false;
            if (dazu) {
                if (it != d.auswahl.end()) { d.auswahl.erase(it); }
                else { d.auswahl.push_back(unterMaus); }
            } else if (io.KeyAlt && gewaehltDarunter) {
                z.klickWahl = unterMaus;
                ziehen = true;
            } else if (it == d.auswahl.end()) {
                d.auswahl.assign(1, unterMaus);
            } else {
                if (d.auswahl.size() > 1) { z.klickWahl = unterMaus; }
                ziehen = true;
            }
            if (!d.auswahl.empty()) {
                springe(d, m.items[static_cast<std::size_t>(unterMaus)].anfang);
                bool frei = true;
                for (const int i : d.auswahl) { frei = frei && d.gesperrt.count(i) == 0; }
                if (ziehen && frei) { z.ziehen = Ziehen::Verschieben; }
            }
        } else {
            // rc608 Teil 3b: leere Stelle oder durchklickbarer Hintergrund: Rahmen aufziehen; Strg und Umschalt nehmen hinzu
            const ImGuiIO& io = ImGui::GetIO();
            z.rahmenBasis = io.KeyShift || io.KeyCtrl ? d.auswahl : std::vector<int>{};
            d.auswahl = z.rahmenBasis;
            z.ziehen = Ziehen::Rahmen;
        }
    }
    // rc608: die Zieh-Schwelle - ein Klick versetzt nichts, auch wenn die Hand ein wenig wandert
    if (z.ziehen != Ziehen::Nichts && !z.ziehLaeuft) {
        const float wx = maus.x - z.ziehDruck.x;
        const float wy = maus.y - z.ziehDruck.y;
        const float schwelle = ImGui::GetFontSize() * 0.25F;
        z.ziehLaeuft = wx * wx + wy * wy >= schwelle * schwelle;
    }
    if (z.ziehen != Ziehen::Nichts && z.ziehLaeuft && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        float dx = vx - z.ziehStart.x;
        float dy = vy - z.ziehStart.y;
        if (ImGui::GetIO().KeyShift) {   // nur waagrecht oder senkrecht
            if (std::fabs(dx) > std::fabs(dy)) { dy = 0; } else { dx = 0; }
        }
        z.linien.clear();
        std::vector<Rechteck> andere;
        for (std::size_t i = 0; i < m.items.size(); ++i) {
            if (std::find(d.auswahl.begin(), d.auswahl.end(), static_cast<int>(i)) == d.auswahl.end()) {
                andere.push_back(m.items[i].window.rect);
            }
        }
        for (const float x : z.hilfslinienX) { andere.push_back(Rechteck{x, 0, 0, 480}); }
        for (const float y : z.hilfslinienY) { andere.push_back(Rechteck{0, y, 640, 0}); }
        const float tol = 5.0F / std::max(0.01F, ab.sx);
        const bool snap = !ImGui::GetIO().KeyAlt;
        if (z.ziehen == Ziehen::Verschieben && !d.auswahl.empty()) {
            Rechteck r = m.items[static_cast<std::size_t>(d.auswahl[0])].window.rect;
            r.x += dx;
            r.y += dy;
            if (snap) {
                const Einrasten e = einrasten(r, andere, z.rasterWeite, tol, z.anRaster, z.anKanten);
                dx += e.dx;
                dy += e.dy;
                z.linien = e.linien;
            }
            z.zdx = std::round(dx);
            z.zdy = std::round(dy);
        } else if (z.ziehen == Ziehen::Groesse) {
            Rechteck r = einzel;
            const int gx = z.griff % 3, gy = z.griff / 3;
            if (gx == 0) { r.x += dx; r.w -= dx; }
            if (gx == 2) { r.w += dx; }
            if (gy == 0) { r.y += dy; r.h -= dy; }
            if (gy == 2) { r.h += dy; }
            r.x = std::round(r.x); r.y = std::round(r.y); r.w = std::round(std::max(1.0F, r.w)); r.h = std::round(std::max(1.0F, r.h));
            z.groesseNeu = r;
        }
    }
    if (z.ziehen != Ziehen::Nichts && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (z.ziehen == Ziehen::Verschieben && !z.ziehLaeuft && z.klickWahl >= 0 && static_cast<std::size_t>(z.klickWahl) < m.items.size()) {
            d.auswahl.assign(1, z.klickWahl);
        } else if (z.ziehen == Ziehen::Verschieben && (z.zdx != 0 || z.zdy != 0)) {
            std::vector<std::pair<int, Rechteck>> neu;
            for (const int i : d.auswahl) {
                if (static_cast<std::size_t>(i) >= m.items.size()) { continue; }
                Rechteck r = clientRect(m, m.items[static_cast<std::size_t>(i)]);
                r.x += z.zdx;
                r.y += z.zdy;
                neu.emplace_back(i, r);
            }
            setzeRects(neu);
        } else if (z.ziehen == Ziehen::Groesse && z.ziehLaeuft && d.auswahl.size() == 1 && static_cast<std::size_t>(d.auswahl[0]) < m.items.size()) {
            const Rechteck alt = einzel;
            Rechteck r = clientRect(m, m.items[static_cast<std::size_t>(d.auswahl[0])]);
            r.x += z.groesseNeu.x - alt.x;
            r.y += z.groesseNeu.y - alt.y;
            r.w = z.groesseNeu.w;
            r.h = z.groesseNeu.h;
            setzeRects({{d.auswahl[0], r}});
        } else if (z.ziehen == Ziehen::Lineal && hover) {
            if (z.griff == 1) { z.hilfslinienX.push_back(std::round(vx)); }
            else { z.hilfslinienY.push_back(std::round(vy)); }
        }
        z.ziehen = Ziehen::Nichts;
        z.zdx = z.zdy = 0;
        z.linien.clear();
    }
}

// rc608 (shank: "warum auch immer geht so gut wie kein undo in dem Menu tab"): die Tasten gelten im ganzen
// Menue-Editor, in jedem Bild vor den Panels. Nur echte Eingabefelder, der Text-Editor (gleicher Puffer,
// eigene Tasten) und offene Fenster behalten sie. Die Tasten fuer die Auswahl gelten, wo nicht gerade ein
// Panel mit Werten den Fokus hat (Pfeile wandern dort durch die Felder, Entf gehoert dem Feld).
void speichernWunsch(Dokument& d, bool unter);   // unten

// Strg+A auf der Flaeche: alles, was man hier sehen und bewegen kann
void alleWaehlen(Dokument& d) {
    const int mi = menuDerSeite(d);
    if (mi < 0) { return; }
    const Umgebung u = umgebung(d);
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    d.auswahl.clear();
    for (std::size_t i = 0; i < m.items.size(); ++i) {
        if (rahmenWaehlbar(d, mi, m.items[i], static_cast<int>(i), u)) { d.auswahl.push_back(static_cast<int>(i)); }
    }
}

void editorTasten(Dokument& d) {
    Zustand& z = Z();
    const ImGuiIO& io = ImGui::GetIO();
    const int bild = ImGui::GetFrameCount();
    if (z.schreib.an) {
        const ImVec2 m = io.MousePos;
        const bool drin = m.x >= z.schreib.a.x && m.x <= z.schreib.b.x && m.y >= z.schreib.a.y && m.y <= z.schreib.b.y;
        if (!drin && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) { schreibenEnde(true); }
    }
    if (z.undoWunsch != 0) {
        const int wunsch = z.undoWunsch;
        z.undoWunsch = 0;
        (void)(wunsch < 0 ? rueckgaengig() : wiederholen());
    }
    if (!tabfenster::fensterHatFokus()) { return; }   // rc609 T1: Tasten nur im Fenster mit dem Fokus
    // rc608 Teil 3a: Es zaehlt, was unter dem Zeiger liegt: ueber dem Text-Editor seine Schrift, sonst die Flaeche
    const bool strg = io.KeyCtrl;
    if (!io.WantTextInput && strg && !io.KeyShift && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId)) {
        const bool ueberText = z.codeUeberBild + 1 >= bild;
        if (ImGui::IsKeyPressed(ImGuiKey_0, false)) {
            if (ueberText) {
                g_app->settings.menueTextZoom = 100;
                z.codeZoomAb = ImGui::GetTime();
            } else {
                d.zoomWunsch = 2;
            }
        }
        if (!ueberText && ImGui::IsKeyPressed(ImGuiKey_1, false)) { d.zoomWunsch = 3; }
    }
    // rc608 Teil 3b: Befehle aus dem Rechtsklick-Menue
    if (z.kontextWunsch != KwNichts) {
        const int w = z.kontextWunsch;
        z.kontextWunsch = KwNichts;
        if (w == KwAusschneiden || w == KwKopieren) { ImGui::SetClipboardText(anzeige(kopiereAuswahl()).c_str()); }
        if (w == KwAusschneiden || w == KwLoeschen) { loescheAuswahl(); }
        if (w == KwEinfuegen) {
            if (const char* c = ImGui::GetClipboardText()) { einfuegenOhneDeckung(utf8ZuLatin1(c)); }
        }
        if (w == KwDuplizieren) { dupliziereAuswahl(); }
        if (w == KwVorn || w == KwHinten) { auswahlAnsEnde(d, w == KwVorn); }
        if (w == KwAlle) { alleWaehlen(d); }
    }
    // rc608 Teil 3b: Strg+S und Strg+Umschalt+S gab es im Menue-Editor nicht, auch nicht mit Fokus im Text-Editor. Ein offenes Feld schreibt zuerst.
    if (strg && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId) && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
        if (z.schreib.an) { schreibenEnde(true); }
        haltSchreiben();
        speichernWunsch(d, io.KeyShift);
    }
    if (io.WantTextInput || z.codeBild + 1 >= bild || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId)) { return; }
    if (strg && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)) { (void)rueckgaengig(); }
    if (strg && (ImGui::IsKeyPressed(ImGuiKey_Y) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)))) { (void)wiederholen(); }
    if (z.werteBild + 1 >= bild) { return; }
    if (strg && ImGui::IsKeyPressed(ImGuiKey_V)) {
        if (const char* c = ImGui::GetClipboardText()) { einfuegenOhneDeckung(utf8ZuLatin1(c)); }
    }
    if (strg && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_A, false)) { alleWaehlen(d); }
    if (d.auswahl.empty()) { return; }
    const float schritt = io.KeyShift ? 10.0F : 1.0F;
    float dx = 0, dy = 0;
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) { dx = -schritt; }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) { dx = schritt; }
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) { dy = -schritt; }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { dy = schritt; }
    if (dx != 0 || dy != 0) { verschiebeAuswahl(dx, dy); }
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) { loescheAuswahl(); }
    if (strg && ImGui::IsKeyPressed(ImGuiKey_D)) { dupliziereAuswahl(); }
    if (strg && ImGui::IsKeyPressed(ImGuiKey_C)) { ImGui::SetClipboardText(anzeige(kopiereAuswahl()).c_str()); }
    if (strg && ImGui::IsKeyPressed(ImGuiKey_X)) {
        ImGui::SetClipboardText(anzeige(kopiereAuswahl()).c_str());
        loescheAuswahl();
    }
    layoutTasten(d);   // M5: Strg+Umschalt+1..4
}

// --- Code-Editor ------------------------------------------------------------------

ImU32 farbeFuer(Farbklasse k) {
    switch (k) {
        case Farbklasse::Kommentar: return IM_COL32(106, 153, 85, 255);
        case Farbklasse::String: return IM_COL32(206, 145, 120, 255);
        case Farbklasse::Zahl: return IM_COL32(181, 206, 168, 255);
        case Farbklasse::MenuWort: return IM_COL32(86, 156, 214, 255);
        case Farbklasse::ItemWort: return IM_COL32(156, 220, 254, 255);
        case Farbklasse::Block: return IM_COL32(197, 134, 192, 255);
        case Farbklasse::Konstante: return IM_COL32(79, 193, 255, 255);
        case Farbklasse::Praeprozessor: return IM_COL32(155, 155, 155, 255);
        case Farbklasse::Skript: return IM_COL32(220, 220, 170, 255);
        case Farbklasse::MenuText: return IM_COL32(255, 198, 109, 255);
        case Farbklasse::Klammer: return IM_COL32(255, 215, 0, 255);
        case Farbklasse::Text: break;
    }
    return IM_COL32(212, 212, 212, 255);
}

// Latin-1 (die .menu-Dateien) -> UTF-8 fuer ImGui
std::string anzeige(std::string_view s) {
    std::string o;
    o.reserve(s.size());
    for (const char c : s) {
        const auto u = static_cast<unsigned char>(c);
        if (u < 0x80) { o += c; }
        else {
            o += static_cast<char>(0xC0 | (u >> 6));
            o += static_cast<char>(0x80 | (u & 0x3F));
        }
    }
    return o;
}

// x-Versatz einer Spalte (Tabs auf 4 Leerzeichen)
float spaltenX(std::string_view zeile, int spalte, float leer) {
    float x = 0;
    ImFont* f = ImGui::GetFont();
    const float fs = ImGui::GetFontSize();
    for (int i = 0; i < spalte && static_cast<std::size_t>(i) < zeile.size(); ++i) {
        const char c = zeile[static_cast<std::size_t>(i)];
        if (c == '\t') {
            x = (std::floor(x / (leer * 4.0F)) + 1.0F) * leer * 4.0F;
        } else {
            const std::string a = anzeige(zeile.substr(static_cast<std::size_t>(i), 1));
            x += f->CalcTextSizeA(fs, 1e9F, 0.0F, a.c_str(), a.c_str() + a.size()).x;
        }
    }
    return x;
}

int spalteBei(std::string_view zeile, float x, float leer) {
    for (int s = 0; s <= static_cast<int>(zeile.size()); ++s) {
        const float a = spaltenX(zeile, s, leer);
        const float b = spaltenX(zeile, s + 1, leer);
        if (x < (a + b) * 0.5F || s == static_cast<int>(zeile.size())) { return s; }
    }
    return static_cast<int>(zeile.size());
}

std::string utf8ZuLatin1(const char* s) {
    std::string o;
    for (std::size_t i = 0; s[i] != 0;) {
        const auto c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) { o += static_cast<char>(c); ++i; }
        else if ((c & 0xE0) == 0xC0 && s[i + 1] != 0) {
            const unsigned v = ((c & 0x1FU) << 6) | (static_cast<unsigned char>(s[i + 1]) & 0x3FU);
            o += v < 256 ? static_cast<char>(v) : '?';
            i += 2;
        } else {
            o += '?';
            ++i;
            while (s[i] != 0 && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) { ++i; }
        }
    }
    return o;
}

std::vector<std::string> kandidaten(const Dokument& d) {
    std::vector<std::string> k = menuSchluessel(d.datei.dialekt, d.datei.spiel);
    // rc608 Punkt 14: Steht der Cursor in einem itemDef, schlaegt es nur vor, was das Spiel bei diesem Typ auswertet
    const Item* imItem = nullptr;
    if (!d.puffer.cursor.empty()) {
        const std::uint32_t c = d.puffer.cursor.front().pos;
        for (const Menu& m : d.datei.menus) {
            for (const Item& it : m.items) {
                if (it.kopf.datei == 0 && c >= it.anfang && c <= it.ende) { imItem = &it; }
            }
        }
    }
    for (const std::string& s : itemSchluessel(d.datei.dialekt, d.datei.spiel)) {
        if (imItem == nullptr || feldGilt(s, imItem->type, imItem->window.style, d.datei.spiel)) { k.push_back(s); }
    }
    for (const std::string& s : skriptBefehle()) { k.push_back(s); }
    for (int t = 0; t < 15; ++t) { k.emplace_back(typName(t)); }
    for (int t = 0; t < 6; ++t) { k.emplace_back(styleName(t)); }
    for (const char* a : {"ITEM_ALIGN_LEFT", "ITEM_ALIGN_CENTER", "ITEM_ALIGN_RIGHT", "WINDOW_INACTIVE", "menuDef", "itemDef"}) { k.emplace_back(a); }
    for (const auto& [n, v] : d.datei.defines) { (void)v; k.push_back(n); }
    for (const Menu& m : d.datei.menus) {
        k.push_back(m.window.name);
        for (const Item& it : m.items) {
            if (!it.window.name.empty()) { k.push_back(it.window.name); }
            if (!it.cvar.empty()) { k.push_back(it.cvar); }
            if (!it.window.background.empty()) { k.push_back(it.window.background); }
        }
    }
    for (const CvarFund& c : cvarsImMenue(d.datei)) { k.push_back(c.name); }
    const Zustand& z = Z();
    for (const auto& [n, s] : z.shader) { (void)s; if (n.compare(0, 4, "gfx/") == 0 || n.compare(0, 5, "menu/") == 0) { k.push_back(n); } }
    for (const auto& [n, s] : z.texte.werte) { (void)s; if (n.compare(0, 6, "MENUS_") == 0) { k.push_back("@" + n); } }
    return k;
}

// F12: Name unter dem Cursor -> menuDef/itemDef mit diesem Namen
void gehZuDefinition(Dokument& d) {
    const Treffer w = d.puffer.wortBei(d.puffer.cursor.front().pos);
    const std::string wort = d.puffer.text().substr(w.anfang, w.ende - w.anfang);
    if (wort.empty()) { return; }
    for (const Menu& m : d.datei.menus) {
        if (m.window.name == wort) {
            springe(d, m.anfang);
            return;
        }
        for (const Item& it : m.items) {
            if (it.window.name == wort || it.window.group == wort) {
                springe(d, it.anfang);
                return;
            }
        }
    }
    if (wort[0] == '@') { Z().meldung = wort + " = " + texte().aufloesen(wort); }
}

// uebersetzung1: in der schmalen rechten Spalte (M2) passte die Suchzeile schon auf
// Englisch nicht ("In all op|"): das naechste Element in dieselbe Zeile, sonst in die naechste.
void daneben(float breite) {
    // rc608: erst messen. SameLine + NewLine zaehlte den Abstand als Inhalt, und das Feld wurde um diesen Abstand zu schmal.
    const float rest = ImGui::GetCurrentWindow()->WorkRect.Max.x - ImGui::GetItemRectMax().x - ImGui::GetStyle().ItemSpacing.x;
    if (rest >= breite) { ImGui::SameLine(); }
}
float knopfBreite(const char* text) { return ImGui::CalcTextSize(text, nullptr, true).x + ImGui::GetStyle().FramePadding.x * 2.0F; }
float hakenBreite(const char* text) {
    return ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize(text, nullptr, true).x;
}

void zeichneSuchleiste(Dokument& d) {
    Zustand& z = Z();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14);
    if (z.suchFokus) {
        ImGui::SetKeyboardFocusHere();
        z.suchFokus = false;
    }
    bool neu = eingabe("##such", &z.such);
    ImGui::SameLine();
    neu |= ImGui::Checkbox(".*", &z.suchOpt.regex);
    ImGui::SameLine();
    neu |= ImGui::Checkbox("Aa", &z.suchOpt.gross);
    ImGui::SameLine();
    neu |= ImGui::Checkbox("ab", &z.suchOpt.wort);
    daneben(hakenBreite(tr(Str::MeInAllen)));
    ImGui::Checkbox(tr(Str::MeInAllen), &z.inAllen);
    std::string fehler;
    const std::vector<Treffer> t = d.puffer.suche(z.such, z.suchOpt, &fehler);
    int gesamt = static_cast<int>(t.size());
    if (z.inAllen) {
        gesamt = 0;
        for (const auto& o : z.docs) { gesamt += static_cast<int>(o->puffer.suche(z.such, z.suchOpt).size()); }
    }
    char b[200];
    if (!fehler.empty()) { std::snprintf(b, sizeof(b), tr(Str::MeRegexFehler), fehler.c_str()); }
    else { std::snprintf(b, sizeof(b), trN(Str::MeTreffer, gesamt), gesamt); }
    daneben(ImGui::CalcTextSize(b).x);
    ImGui::TextUnformatted(b);
    auto naechster = [&](bool vor) {
        if (t.empty()) { return; }
        const std::uint32_t c = d.puffer.cursor.front().pos;
        const Treffer* w = nullptr;
        if (vor) {
            for (const Treffer& x : t) {
                if (x.anfang >= c && (x.anfang != d.puffer.cursor.front().von() || !d.puffer.cursor.front().auswahl())) { w = &x; break; }
            }
            if (w == nullptr) { w = &t.front(); }
        } else {
            for (auto it = t.rbegin(); it != t.rend(); ++it) {
                if (it->ende < d.puffer.cursor.front().von() + 1 && it->anfang < d.puffer.cursor.front().von()) { w = &*it; break; }
            }
            if (w == nullptr) { w = &t.back(); }
        }
        d.puffer.cursor.assign(1, Cursor{w->ende, w->anfang});
        d.springeZeile = d.puffer.pos(w->anfang).zeile;
    };
    daneben(ImGui::GetFrameHeight() * 2.0F + ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::ArrowButton("##zurueck", ImGuiDir_Up)) { naechster(false); }
    ImGui::SameLine();
    if (ImGui::ArrowButton("##vor", ImGuiDir_Down) || (neu && !t.empty())) { naechster(true); }
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused()) { naechster(!ImGui::GetIO().KeyShift); }
    // rc608 Punkt 17: Suchen und Ersetzen gehoeren zusammen, mit den Beschriftungen des Skript-Bereichs (drawFindDialog)
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14);
    if (z.ersatzFokus) {
        ImGui::SetKeyboardFocusHere();
        z.ersatzFokus = false;
    }
    eingabe("##ersatz", &z.ersatz, 0, tr(Str::FindReplaceWith));
    ImGui::BeginDisabled(z.such.empty());
    daneben(knopfBreite(tr(Str::FindReplaceBtn)));
    if (ImGui::Button(tr(Str::FindReplaceBtn))) { d.puffer.ersetzeEinen(z.such, z.ersatz, z.suchOpt); }
    const auto melde = [&](const std::string& text) {
        z.meldung = text;
        ::bhed::gui::meldung(text, "", false);
    };
    daneben(knopfBreite(tr(Str::FindReplaceAll)));
    if (ImGui::Button(tr(Str::FindReplaceAll))) {
        char m[120];
        const int n = d.puffer.ersetzeAlle(z.such, z.ersatz, z.suchOpt);
        std::snprintf(m, sizeof(m), tr(Str::FindReplacedN), n);
        melde(m);
    }
    daneben(knopfBreite(tr(Str::FindReplaceAllDocs)));
    if (ImGui::Button(tr(Str::FindReplaceAllDocs))) {
        char m[160];
        int n = 0;
        int dateien = 0;
        for (auto& o : z.docs) {
            const int k = o->puffer.ersetzeAlle(z.such, z.ersatz, z.suchOpt);
            n += k;
            dateien += k > 0 ? 1 : 0;
        }
        std::snprintf(m, sizeof(m), trN(Str::FindReplacedDocs, dateien), n, dateien);
        melde(m);
    }
    ImGui::EndDisabled();
}

void codeTasten(Dokument& d) {
    Zustand& z = Z();
    ImGuiIO& io = ImGui::GetIO();
    Textpuffer& p = d.puffer;
    const bool strg = io.KeyCtrl;
    const bool um = io.KeyShift;
    auto taste = [](ImGuiKey k) { return ImGui::IsKeyPressed(k, true); };
    if (z.vervollOffen && !z.vervoll.empty()) {
        if (taste(ImGuiKey_DownArrow)) { z.vervollWahl = (z.vervollWahl + 1) % static_cast<int>(z.vervoll.size()); return; }
        if (taste(ImGuiKey_UpArrow)) {
            z.vervollWahl = (z.vervollWahl + static_cast<int>(z.vervoll.size()) - 1) % static_cast<int>(z.vervoll.size());
            return;
        }
        if (taste(ImGuiKey_Enter) || taste(ImGuiKey_Tab)) {
            for (Cursor& c : p.cursor) {
                const Treffer w = p.wortBei(c.pos);
                c.anker = w.anfang;
                c.pos = std::min(c.pos, w.ende);
            }
            p.tippe(z.vervoll[static_cast<std::size_t>(z.vervollWahl)]);
            z.vervollOffen = false;
            return;
        }
        if (taste(ImGuiKey_Escape)) { z.vervollOffen = false; return; }
    }
    // rc608: erst vor dem naechsten Bild - der Editor zeichnet gerade aus dem jetzigen Text
    if (io.KeyAlt && !strg && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {   // rc608 Punkt 2: wie in VS Code; aendert nur die Anzeige, nicht die Datei
        g_app->settings.menueUmbruch = !g_app->settings.menueUmbruch;
        return;
    }
    if (strg && !um && taste(ImGuiKey_Z)) { z.undoWunsch = -1; return; }
    if (strg && (taste(ImGuiKey_Y) || (um && taste(ImGuiKey_Z)))) { z.undoWunsch = 1; return; }
    if (strg && taste(ImGuiKey_A)) { p.allesWaehlen(); return; }
    if (strg && (taste(ImGuiKey_C) || taste(ImGuiKey_X))) {
        const std::string s = p.auswahlText();
        if (!s.empty()) { ImGui::SetClipboardText(anzeige(s).c_str()); }
        if (ImGui::IsKeyPressed(ImGuiKey_X)) { p.tippe(""); }
        return;
    }
    if (strg && taste(ImGuiKey_V)) {
        if (const char* c = ImGui::GetClipboardText()) {
            std::string t = utf8ZuLatin1(c);
            if (p.zeilenUmbruch() == "\r\n") {
                std::string u;
                for (std::size_t i = 0; i < t.size(); ++i) {
                    if (t[i] == '\n' && (i == 0 || t[i - 1] != '\r')) { u += '\r'; }
                    u += t[i];
                }
                t = u;
            }
            p.tippe(t);
        }
        return;
    }
    if (strg && um && taste(ImGuiKey_L)) { p.alleVorkommen(); return; }
    if (strg && taste(ImGuiKey_D)) { p.naechstesVorkommenDazu(); return; }
    if (strg && taste(ImGuiKey_Slash)) { p.kommentar(); return; }
    if (strg && taste(ImGuiKey_F)) {
        z.suchenOffen = z.suchFokus = true;
        if (const std::string a = p.auswahlText(); !a.empty()) { z.such = a; }
        return;
    }
    if (strg && taste(ImGuiKey_H)) {   // wie Strg+F, nur steht der Cursor gleich im Ersetzen-Feld
        z.suchenOffen = true;
        z.ersatzFokus = true;
        if (const std::string a = p.auswahlText(); !a.empty()) { z.such = a; }
        return;
    }
    if (strg && taste(ImGuiKey_Space)) {
        const Treffer w = p.wortBei(p.cursor.front().pos);
        z.vervoll = vorschlaege(p.text().substr(w.anfang, p.cursor.front().pos - w.anfang), kandidaten(d));
        z.vervollWahl = 0;
        z.vervollOffen = !z.vervoll.empty();
        return;
    }
    if (taste(ImGuiKey_F12)) { gehZuDefinition(d); return; }
    if (taste(ImGuiKey_F2)) {
        const int zl = p.pos(p.cursor.front().pos).zeile;
        if (strg) {
            if (d.lesezeichen.count(zl) != 0) { d.lesezeichen.erase(zl); } else { d.lesezeichen.insert(zl); }
        } else if (!d.lesezeichen.empty()) {
            int ziel = um ? *d.lesezeichen.rbegin() : *d.lesezeichen.begin();
            if (um) {
                auto it = d.lesezeichen.lower_bound(zl);
                if (it != d.lesezeichen.begin()) { ziel = *std::prev(it); }
            } else {
                auto it = d.lesezeichen.upper_bound(zl);
                if (it != d.lesezeichen.end()) { ziel = *it; }
            }
            springe(d, p.zeilenAnfang(ziel));
        }
        return;
    }
    if (taste(ImGuiKey_Escape)) {
        if (p.cursor.size() > 1) { p.cursor.resize(1); }
        z.suchenOffen = false;
        return;
    }
    if (taste(ImGuiKey_LeftArrow)) { if (strg) { p.wortSprung(false, um); } else { p.bewege(0, -1, um); } }
    if (taste(ImGuiKey_RightArrow)) { if (strg) { p.wortSprung(true, um); } else { p.bewege(0, 1, um); } }
    if (taste(ImGuiKey_UpArrow)) { p.bewege(-1, 0, um); }
    if (taste(ImGuiKey_DownArrow)) { p.bewege(1, 0, um); }
    if (taste(ImGuiKey_PageUp)) { p.bewege(-30, 0, um); }
    if (taste(ImGuiKey_PageDown)) { p.bewege(30, 0, um); }
    if (taste(ImGuiKey_Home)) { if (strg) { p.einCursor(0, um); } else { p.zeilenAnfangEnde(false, um); } }
    if (taste(ImGuiKey_End)) {
        if (strg) { p.einCursor(static_cast<std::uint32_t>(p.text().size()), um); } else { p.zeilenAnfangEnde(true, um); }
    }
    if (taste(ImGuiKey_Backspace)) { p.loescheZurueck(); }
    if (taste(ImGuiKey_Delete)) { p.loescheVor(); }
    if (taste(ImGuiKey_Enter) || taste(ImGuiKey_KeypadEnter)) { p.neueZeile(); }
    if (taste(ImGuiKey_Tab)) {
        bool mehrzeilig = false;
        for (const Cursor& c : p.cursor) { mehrzeilig = mehrzeilig || p.pos(c.von()).zeile != p.pos(c.bis()).zeile; }
        if (um || mehrzeilig) { p.einruecken(um); } else { p.tippe("\t"); }
    }
    if (!strg) {
        std::string t;
        for (int i = 0; i < io.InputQueueCharacters.Size; ++i) {
            const ImWchar c = io.InputQueueCharacters[i];
            if (c == '\t' || c == '\r' || c == '\n') { continue; }
            if (c >= 32) { t += c < 256 ? static_cast<char>(c) : '?'; }
        }
        if (!t.empty()) {
            p.tippe(t);
            // Vervollstaendigung waehrend des Tippens
            const Treffer w = p.wortBei(p.cursor.front().pos);
            const std::string prae = p.text().substr(w.anfang, p.cursor.front().pos - w.anfang);
            if (prae.size() >= 2) {
                z.vervoll = vorschlaege(prae, kandidaten(d), 12);
                z.vervollWahl = 0;
                z.vervollOffen = !z.vervoll.empty();
            } else {
                z.vervollOffen = false;
            }
        }
    }
}

// --- M2: Live-Diagnose ------------------------------------------------------------------
//
// Nach jeder Aenderung (entprellt, 0.3 s Ruhe) laeuft diagnose() im Hintergrund auf einer
// Kopie des gelesenen Modells. Was es dafuer aus den Spieldaten braucht (Dateien, Shader,
// Menuenamen, bekannte Cvars), liegt als unveraenderlicher Schnappschuss bereit.

struct DiagDaten {
    std::set<std::string> dateien, shader, menues;
    std::vector<std::string> ordner;   // lose Dateien neben den pk3
    std::vector<std::string> cvars;
};

std::string normPfad(std::string p) {
    for (char& c : p) { c = c == '\\' ? '/' : static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return p;
}

// RE_RegisterShaderNoMip: Shaderskript vor Bild, *white ist eingebaut (tr.whiteImage), Gross/Klein egal
bool bildBekannt(const DiagDaten& dd, const std::string& n0, const std::function<bool(const std::string&)>& gibt) {
    const std::string n = normPfad(n0);
    if (n == "*white") { return true; }
    const std::size_t pt = n.rfind('.');
    const std::string ohne = (pt != std::string::npos && n.find('/', pt) == std::string::npos) ? n.substr(0, pt) : n;
    if (dd.shader.count(n) != 0 || dd.shader.count(ohne) != 0) { return true; }
    for (const char* e : {".jpg", ".png", ".tga", ".jpeg"}) {
        if (gibt(ohne + e)) { return true; }
    }
    return gibt(n);
}

// Menuenamen und Cvars aus einer .menu (grob nach Woertern, fuer die Diagnose genug)
void menueWoerter(const std::string& t, std::set<std::string>& menues, std::set<std::string>& cvars) {
    std::vector<std::string> w;
    std::size_t i = 0;
    while (i < t.size()) {
        const char c = t[i];
        if (c == '/' && i + 1 < t.size() && t[i + 1] == '/') {
            while (i < t.size() && t[i] != '\n') { ++i; }
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(c)) || c == '{' || c == '}' || c == ';') {
            ++i;
            continue;
        }
        std::string x;
        if (c == '"') {
            ++i;
            while (i < t.size() && t[i] != '"') { x += t[i++]; }
            ++i;
        } else {
            while (i < t.size() && !std::isspace(static_cast<unsigned char>(t[i])) && t[i] != '{' && t[i] != '}' && t[i] != ';' && t[i] != '"') { x += t[i++]; }
        }
        w.push_back(x);
    }
    bool imMenue = false;
    for (std::size_t k = 0; k + 1 < w.size(); ++k) {
        const std::string a = normPfad(w[k]);
        if (a == "menudef") { imMenue = true; }
        else if (a == "itemdef") { imMenue = false; }
        else if (a == "name" && imMenue) {
            menues.insert(normPfad(w[k + 1]));
            imMenue = false;
        } else if (a == "cvar" || a == "cvartest" || a == "cvarfloat" || a == "cvarint" || a == "setcvar" || a == "cvarstrlist" || a == "cvarfloatlist") {
            if (!w[k + 1].empty() && std::isalpha(static_cast<unsigned char>(w[k + 1][0])) != 0) { cvars.insert(w[k + 1]); }
        }
    }
}

void shaderLaden() {
    Zustand& z = Z();
    if (z.shaderGeladen) { return; }
    z.shaderGeladen = true;
    // ScanAndLoadShaderFiles haengt die Dateien in umgekehrter Reihenfolge von FS_ListFiles aneinander, und der erste
    // Treffer im Text gilt (ShaderEntryPtrs_Insert). Das heisst hier: in der Reihenfolge von FS_ListFiles, der letzte gilt.
    for (const std::string& f : z.daten.listeWieSpiel("shaders/", {".shader"})) {
        std::string inhalt;
        if (z.daten.lies(f, inhalt)) { parseShaderScript(inhalt, z.shader); }
    }
    // M4: und die des Pakets (z. B. menu/video/<name> vom Video-Import)
    for (const auto& od : z.docs) {
        for (const auto& [p, b] : od->bilder) {
            const std::string k = kleinPfad(p);
            if (k.compare(0, 8, "shaders/") == 0 && k.size() > 7 && k.compare(k.size() - 7, 7, ".shader") == 0) { parseShaderScript(b, z.shader); }
        }
    }
}

std::shared_ptr<const DiagDaten> diagDaten() {
    Zustand& z = Z();
    if (z.diagDaten && z.diagDatenStand == z.datenStand) { return z.diagDaten; }
    auto dd = std::make_shared<DiagDaten>();
    for (const std::string& p : z.daten.liste("", {})) { dd->dateien.insert(normPfad(p)); }
    shaderLaden();
    for (const GamePath& g : z.daten.ordner) { dd->ordner.push_back(g.directory); }
    // Wie bildRegistriert: Findet eine Stufe ihr Bild nicht, gilt der ganze Shader nicht (ParseStage)
    const auto stufeDa = [&](const std::string& bild) {
        for (const std::string& k : textureCandidates(normPfad(bild))) {
            if (dd->dateien.count(k) != 0) { return true; }
        }
        return false;
    };
    for (const auto& [k, v] : z.shader) {
        bool gut = true;
        for (const ShaderInfo::Stage& st : v.stages) {
            gut = gut && (st.image.empty() || stufeDa(st.image));
            for (const std::string& a : st.animImages) { gut = gut && stufeDa(a); }
        }
        if (gut) { dd->shader.insert(k); }
    }
    std::set<std::string> cvars;
    for (const std::string& p : z.daten.liste("ui/", {".menu"})) {
        std::string t;
        if (z.daten.lies(p, t)) { menueWoerter(t, dd->menues, cvars); }
    }
    dd->cvars.assign(cvars.begin(), cvars.end());
    z.diagDaten = dd;
    z.diagDatenStand = z.datenStand;
    return dd;
}

void diagnosePflegen(Dokument& d) {
    Zustand& z = Z();
    const double jetzt = ImGui::GetTime();
    if (d.diagLauf.valid() && d.diagLauf.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        d.diagnosen = d.diagLauf.get();
        d.diagVersion = d.diagLaufVersion;
        ++z.diagLaeufe;
    }
    if (d.geparst != d.diagGewollt) {
        d.diagGewollt = d.geparst;
        d.diagAb = jetzt + (d.diagVersion < 0 ? 0.0 : 0.3);
    }
    if (d.diagLauf.valid() || d.diagVersion == d.diagGewollt || jetzt < d.diagAb || d.geparst < 0) { return; }
    std::shared_ptr<const DiagDaten> dd = diagDaten();
    std::set<std::string> paket;
    for (const auto& [p, b] : d.bilder) { paket.insert(normPfad(p)); }
    std::set<std::string> offeneMenues;   // Menues der anderen offenen Dateien
    for (const auto& od : z.docs) {
        for (const Menu& m : od->datei.menus) { offeneMenues.insert(normPfad(m.window.name)); }
    }
    d.diagLaufVersion = d.diagGewollt;
    d.diagLauf = std::async(std::launch::async, [dd, kopie = d.datei, paket = std::move(paket), offeneMenues = std::move(offeneMenues)]() {
        DiagKontext k;
        k.pruefung.zielSpiele.clear();   // live nur die eigene Fassung; die Vertraeglichkeit prueft der Export
        k.pruefung.gibt = [&](const std::string& p0) {
            const std::string p = normPfad(p0);
            if (paket.count(p) != 0 || dd->dateien.count(p) != 0) { return true; }
            for (const std::string& o : dd->ordner) {
                std::error_code ec;
                if (!o.empty() && fs::is_regular_file(utf8Pfad(o) / utf8Pfad(p0), ec)) { return true; }
            }
            return false;
        };
        k.pruefung.gibtBild = [&](const std::string& n0) { return bildBekannt(*dd, n0, k.pruefung.gibt); };
        k.menueBekannt = [&](const std::string& n) { return dd->menues.count(normPfad(n)) != 0 || offeneMenues.count(normPfad(n)) != 0; };
        k.bekannteCvars = dd->cvars;
        k.pfadVorschlag = [&](const std::string& p0) -> std::string {
            const std::string p = normPfad(p0);
            const std::size_t sl = p.rfind('/');
            const std::string ordner = sl == std::string::npos ? std::string() : p.substr(0, sl + 1);
            const std::size_t pt = p.rfind('.');
            const bool mitEndung = pt != std::string::npos && (sl == std::string::npos || pt > sl);
            const std::string stamm = p.substr(ordner.size(), mitEndung ? pt - ordner.size() : std::string::npos);
            std::vector<std::string> kandidaten;
            std::map<std::string, std::string> voll;
            for (auto it = dd->dateien.lower_bound(ordner); it != dd->dateien.end() && it->compare(0, ordner.size(), ordner) == 0; ++it) {
                const std::string rest = it->substr(ordner.size());
                if (rest.find('/') != std::string::npos) { continue; }
                const std::size_t q = rest.rfind('.');
                const std::string s = q == std::string::npos ? rest : rest.substr(0, q);
                kandidaten.push_back(s);
                voll[s] = mitEndung ? ordner + rest : ordner + s;
            }
            for (const std::string& s : dd->shader) {
                if (s.compare(0, ordner.size(), ordner) == 0 && s.find('/', ordner.size()) == std::string::npos) {
                    kandidaten.push_back(s.substr(ordner.size()));
                    voll[s.substr(ordner.size())] = s;
                }
            }
            const std::string w = aehnlichstes(stamm, kandidaten, 3);
            return w.empty() ? std::string() : voll[w];
        };
        return diagnose(kopie, k);
    });
}

ImU32 schwereFarbe(Schwere s) {
    return s == Schwere::Fehler ? IM_COL32(240, 70, 70, 255) : s == Schwere::Warnung ? IM_COL32(235, 190, 50, 255) : IM_COL32(90, 170, 255, 255);
}
// Symbol im Rand: Fehler Kreis, Warnung Dreieck, Hinweis Raute
std::string fmtFlussName(const std::string& name) {
    std::vector<char> b(1024);
    std::snprintf(b.data(), b.size(), tr(Str::MeFlussUnerreichbar), name.c_str(), name.c_str());
    return b.data();
}

void schwereSymbol(ImDrawList* dl, ImVec2 m, float r, Schwere s) {
    const ImU32 c = schwereFarbe(s);
    if (s == Schwere::Fehler) { dl->AddCircleFilled(m, r, c); }
    else if (s == Schwere::Warnung) { dl->AddTriangleFilled(ImVec2{m.x, m.y - r}, ImVec2{m.x + r, m.y + r * 0.8F}, ImVec2{m.x - r, m.y + r * 0.8F}, c); }
    else { dl->AddQuadFilled(ImVec2{m.x, m.y - r}, ImVec2{m.x + r, m.y}, ImVec2{m.x, m.y + r}, ImVec2{m.x - r, m.y}, c); }
}
// Wellenlinie unter [x0, x1]
void welle(ImDrawList* dl, float x0, float x1, float y, ImU32 c) {
    const float h = 1.6F, schritt = 3.0F;
    ImVec2 p{x0, y};
    bool oben = true;
    for (float x = x0; x < x1; x += schritt) {
        const ImVec2 q{std::min(x + schritt, x1), y + (oben ? -h : h)};
        dl->AddLine(p, q, c, 1.0F);
        p = q;
        oben = !oben;
    }
    if (x1 - x0 < 2.0F) { dl->AddLine(ImVec2{x0, y}, ImVec2{x0 + 3, y}, c, 1.5F); }
}

std::string fmt(Str s, const std::string& a, const std::string& b = {}, const std::string& c = {}) {
    char buf[1024];
    std::snprintf(buf, sizeof(buf), tr(s), a.c_str(), b.c_str(), c.c_str());
    return buf;
}

// Die Meldung in einer Zeile: Wortlaut der Engine bzw. die Beschreibung (4 Sprachen)
std::string diagMeldung(const Diagnose& x) {
    switch (x.art) {
        case DiagArt::Engine: return anzeige(x.text);
        case DiagArt::KlammerOffen: return fmt(Str::MeDiagKlammerOffen, anzeige(x.text));
        case DiagArt::KlammerZuViel: return fmt(Str::MeDiagKlammerZuViel, anzeige(x.text));
        case DiagArt::KlammerFalsch: return fmt(Str::MeDiagKlammerFalsch, anzeige(x.text), anzeige(x.wert));
        case DiagArt::AnfuehrungOffen: return x.wert.empty() ? std::string(tr(Str::MeDiagAnfuehrungEnde)) : fmt(Str::MeDiagAnfuehrung, anzeige(x.wert));
        case DiagArt::KeineZahl: return fmt(Str::MeDiagKeineZahl, anzeige(x.text), anzeige(x.wert));
        case DiagArt::NameDoppelt: return fmt(x.item >= 0 ? Str::MeDiagNameItem : Str::MeDiagNameMenue, anzeige(x.text));
        case DiagArt::ZielFehlt:
            return fmt(x.wert == "open" || x.wert == "close" ? Str::MeDiagZielMenue : Str::MeDiagZielItem, anzeige(x.wert), anzeige(x.text));
        case DiagArt::CvarAehnlich: return fmt(Str::MeDiagCvar, anzeige(x.text), anzeige(x.wert));
        case DiagArt::Pruefung: {
            const Befund& b = x.befund;
            char buf[512];
            switch (b.art) {
                case BefundArt::ZuVieleItems: std::snprintf(buf, sizeof(buf), tr(Str::MeDiagZuVieleItems), b.a, b.b); return buf;
                case BefundArt::NurOpenJk: std::snprintf(buf, sizeof(buf), tr(Str::MeDiagNurOpenJk), b.a, b.b, b.wert.c_str()); return buf;
                case BefundArt::NiemandOeffnet: return fmtFlussName(b.name);
                case BefundArt::KeineFokusFarbe: return tr(Str::MeDiagKeineFokusFarbe);
                case BefundArt::ModellOhneSkin: return tr(Str::MeDiagModellOhneSkin);
                case BefundArt::ZuVieleMenues: std::snprintf(buf, sizeof(buf), tr(Str::MeDiagZuVieleMenues), b.a, b.b); return buf;
                case BefundArt::SkriptZuLang: std::snprintf(buf, sizeof(buf), trN(Str::MeDiagSkriptZuLang, b.a), b.a, b.b); return buf;
                case BefundArt::ZuVieleWerte:
                case BefundArt::ZuVieleFarben:
                case BefundArt::ZuVieleSpalten: std::snprintf(buf, sizeof(buf), tr(Str::MeDiagZuViele), b.a, b.b); return buf;
                case BefundArt::NieSichtbar: return b.name == "rect" ? std::string(tr(Str::MeDiagNieRect)) : std::string(tr(Str::MeDiagNieVisible));
                case BefundArt::DateiFehlt: return fmt(Str::MeDiagDateiFehlt, anzeige(b.name), anzeige(b.wert));
                case BefundArt::UnbekanntIn: return fmt(Str::MeDiagUnbekanntIn, anzeige(b.name), anzeige(b.wert));
                case BefundArt::CinematicSp: return tr(Str::MeDiagCinematicSp);
                case BefundArt::VollbildOhneBild: return tr(Str::MeDiagVollbildOhneBild);
                case BefundArt::KeinBild: return fmt(Str::MlDiagKeinBild, x.befund.name);
            }
            return {};
        }
    }
    return anzeige(x.text);
}

// Die kurze Erklaerung zu einer Engine-Meldung (deren Wortlaut bleibt englisch wie im Spiel)
std::string diagErklaerung(const Diagnose& x) {
    if (x.art != DiagArt::Engine) { return {}; }
    const std::string t = kleinPfad(x.text);
    if (t.find("unknown") != std::string::npos && (t.find("value") != std::string::npos)) { return tr(Str::MeDiagErklName); }
    if (t.find("unknown") != std::string::npos) { return tr(Str::MeDiagErklUnbekannt); }
    if (t.find("parse") != std::string::npos || t.find("expected") != std::string::npos) { return tr(Str::MeDiagErklParse); }
    if (t.find("end of file") != std::string::npos || t.find("eof") != std::string::npos) { return tr(Str::MeDiagErklDateiende); }
    return tr(Str::MeDiagErklEngine);
}

std::string korrekturText(const Diagnose& x, const std::string& text) {
    if (x.korrekturen.empty()) { return {}; }
    const Korrektur& k = x.korrekturen.front();
    if (k.text.empty()) { return fmt(Str::MeDiagFixEntfernen, anzeige(text.substr(k.anfang, k.ende - k.anfang))); }
    std::string neu = k.text;
    while (!neu.empty() && (neu.back() == '\n' || neu.back() == '\r')) { neu.pop_back(); }
    std::size_t a = 0;
    while (a < neu.size() && (neu[a] == ' ' || neu[a] == '\t' || neu[a] == '\n')) { ++a; }
    neu = neu.substr(a);
    return fmt(k.anfang == k.ende ? Str::MeDiagFixEinfuegen : Str::MeDiagFixErsetzen, anzeige(neu));
}

// Die Schnellkorrektur anwenden (rueckgaengig machbar); Stellen gelten fuer den Stand der Diagnose
bool korrigiere(Dokument& d, const Diagnose& x) {
    if (x.korrekturen.empty() || d.diagVersion != d.puffer.version()) { return false; }
    const Korrektur& k = x.korrekturen.front();
    if (k.ende > d.puffer.text().size()) { return false; }
    d.puffer.ersetze(k.anfang, k.ende, k.text);
    neuParsen(d);
    return true;
}

// Welche Diagnose liegt an dieser Stelle? (die schwerste)
const Diagnose* diagnoseBei(const Dokument& d, std::uint32_t off) {
    const Diagnose* best = nullptr;
    for (const Diagnose& x : d.diagnosen) {
        const std::uint32_t e = std::max(x.ende, x.anfang + 1);
        if (off >= x.anfang && off < e && (best == nullptr || x.schwere < best->schwere)) { best = &x; }
    }
    return best;
}

void zeigeDiagnoseTipp(const Dokument& d, const Diagnose& x) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(schwereFarbe(x.schwere)));
    ImGui::TextUnformatted(tr(x.schwere == Schwere::Fehler ? Str::MeDiagFehler1 : x.schwere == Schwere::Warnung ? Str::MeDiagWarnung1 : Str::MeDiagHinweis1));
    ImGui::PopStyleColor();
    ImGui::TextUnformatted(diagMeldung(x).c_str());   // schon UTF-8
    const std::string e = diagErklaerung(x);
    if (!e.empty()) { ImGui::TextDisabled("%s", e.c_str()); }
    const std::string k = korrekturText(x, d.puffer.text());
    if (!k.empty()) { ImGui::TextColored(ImVec4{0.5F, 0.85F, 0.5F, 1}, "%s", fmt(Str::MeDiagFixTaste, k).c_str()); }
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

// Probleme: Liste mit Filter; Klick springt zur Stelle und waehlt das Element auf der Flaeche
void panelProbleme(Dokument& d) {
    Zustand& z = Z();
    ImGui::PushID("panelProbleme");
    int n[3] = {0, 0, 0};
    for (const Diagnose& x : d.diagnosen) { ++n[static_cast<int>(x.schwere)]; }
    char l[96];
    std::snprintf(l, sizeof(l), "%s %d", tr(Str::MeDiagFehler), n[0]);
    ImGui::Checkbox(l, &z.problemeFehler);
    ImGui::SameLine();
    std::snprintf(l, sizeof(l), "%s %d", tr(Str::MeDiagWarnungen), n[1]);
    ImGui::Checkbox(l, &z.problemeWarnungen);
    ImGui::SameLine();
    std::snprintf(l, sizeof(l), "%s %d", tr(Str::MeDiagHinweise), n[2]);
    ImGui::Checkbox(l, &z.problemeHinweise);
    ImGui::BeginChild("##liste", ImVec2{0, 0}, ImGuiChildFlags_None);
    if (n[0] + n[1] + n[2] == 0) { ImGui::TextDisabled("%s", tr(Str::MeProblemeLeer)); }
    const bool zeigen[3] = {z.problemeFehler, z.problemeWarnungen, z.problemeHinweise};
    for (std::size_t i = 0; i < d.diagnosen.size(); ++i) {
        const Diagnose& x = d.diagnosen[i];
        if (!zeigen[static_cast<int>(x.schwere)]) { continue; }
        ImGui::PushID(static_cast<int>(i));
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float fs = ImGui::GetFontSize();
        schwereSymbol(ImGui::GetWindowDrawList(), ImVec2{p.x + fs * 0.4F, p.y + fs * 0.55F}, fs * 0.28F, x.schwere);
        ImGui::Dummy(ImVec2{fs * 0.9F, fs});
        ImGui::SameLine();
        char zeile[32];
        std::snprintf(zeile, sizeof(zeile), x.zeile > 0 ? "%d:" : "-", x.zeile);
        const std::string t = std::string(zeile) + " " + diagMeldung(x);
        if (!x.korrekturen.empty()) {
            if (ImGui::SmallButton(tr(Str::MeDiagBeheben))) { (void)korrigiere(d, x); }
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", korrekturText(x, d.puffer.text()).c_str()); }
            ImGui::SameLine();
        }
        if (ImGui::Selectable(t.c_str(), false)) {
            if (x.ende > x.anfang || x.zeile > 0) { springe(d, x.ende > x.anfang ? x.anfang : d.puffer.zeilenAnfang(std::max(0, x.zeile - 1))); }
            if (x.menu >= 0) {
                zeigeMenue(d, x.menu);
                if (x.item >= 0) { d.auswahl.assign(1, x.item); }
            }
        }
        if (ImGui::IsItemHovered()) { zeigeDiagnoseTipp(d, x); }
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::PopID();
}

// --- M2 Punkt 6: Texte in mehreren Sprachen ----------------------------------------------
//
// Die eigenen Texte stehen im Paket als strings/<Sprache>/<PAKET>.str (StringEd, Jedi
// Academy und Movie Duels) bzw. strip/<paket>.sp (StripEd, Jedi Outcast); die Tabelle des
// Fensters wird daraus gelesen und bei jeder Aenderung zurueckgeschrieben. Die Vorschau
// zeigt die gewaehlte Sprache: die Texte des Spiels, darueber die eigenen.

std::vector<std::string> sprachenListe() {
    Zustand& z = Z();
    if (z.sprachenGeladen) { return z.sprachen; }
    z.sprachenGeladen = true;
    z.sprachen.clear();
    if (z.spiel == Spiel::JK2) {
        z.sprachen = {"English", "French", "German"};   // StripEd TEXT_LANGUAGE1..3
        return z.sprachen;
    }
    std::set<std::string> gesehen;
    z.sprachen.push_back("English");
    gesehen.insert("english");
    for (const std::string& p : z.daten.liste("strings/", {".str"})) {
        const std::size_t a = p.find('/');
        const std::size_t b = a == std::string::npos ? std::string::npos : p.find('/', a + 1);
        if (b == std::string::npos) { continue; }
        const std::string s = p.substr(a + 1, b - a - 1);
        if (gesehen.insert(kleinPfad(s)).second) { z.sprachen.push_back(s); }
    }
    // M3: se_language, fuer die die Schrift der Engine Doppelbyte-Codes liest (tr_font
    // GetLanguageEnum) - auch ohne strings/<Sprache> im Spiel, fuer eigene Uebersetzungen
    for (const char* s : {"Korean", "Taiwanese", "Japanese", "Chinese"}) {
        if (gesehen.insert(kleinPfad(s)).second) { z.sprachen.emplace_back(s); }
    }
    return z.sprachen;
}

int spNummer(const std::string& sprache) {
    const std::string s = kleinPfad(sprache);
    return s == "french" ? 2 : s == "german" ? 3 : 1;
}

// M3 Punkt 7: Eingabe der Oberflaeche (UTF-8) -> Bytes der Datei in der Kodierung, in der
// die Engine die Sprache liest (kodierung.h), und zurueck fuer die Anzeige
std::string textFuerDatei(const std::string& sprache, const std::string& utf8, std::string* nichtDarstellbar) {
    return ausUtf8(utf8, kodierungDerSprache(sprache), nichtDarstellbar);
}
std::string textAusDatei(const std::string& sprache, const std::string& bytes) {
    const Kodierung k = kodierungDerSprache(sprache);
    return asiatisch(k) ? zuUtf8(bytes, k) : anzeige(bytes);
}

void eigeneTexteLesen(Dokument& d) {
    if (d.texteGelesen) { return; }
    d.texteGelesen = true;
    Zustand& z = Z();
    std::map<std::string, TextZeile> zeilen;
    std::vector<std::string> reihenfolge;
    auto merke = [&](const std::string& paket, const Texte& t, const std::string& sprache) {
        const std::string vor = paket + "_";
        for (const auto& [k, v] : t.werte) {
            if (k.compare(0, vor.size(), vor) != 0) { continue; }
            const std::string s = k.substr(vor.size());
            if (zeilen.count(s) == 0) {
                reihenfolge.push_back(s);
                zeilen[s].schluessel = s;
            }
            zeilen[s].text[sprache] = v;
        }
    };
    for (const auto& [p, b] : d.bilder) {
        const std::string k = kleinPfad(p);
        if (z.spiel == Spiel::JK2) {
            if (k.compare(0, 6, "strip/") != 0 || k.size() < 10 || k.compare(k.size() - 3, 3, ".sp") != 0) { continue; }
            std::string paket = p.substr(6, p.size() - 9);
            for (char& c : paket) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
            d.textPaket = paket;
            for (const std::string& s : sprachenListe()) {
                Texte t;
                t.ladeSp(b, spNummer(s));
                merke(paket, t, s);
            }
            continue;
        }
        if (k.compare(0, 8, "strings/") != 0 || k.size() < 13 || k.compare(k.size() - 4, 4, ".str") != 0) { continue; }
        const std::size_t sl = p.find('/', 8);
        if (sl == std::string::npos) { continue; }
        const std::string sprache = p.substr(8, sl - 8);
        std::string paket = p.substr(sl + 1, p.size() - sl - 5);
        for (char& c : paket) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
        d.textPaket = paket;
        std::string gross = sprache;
        for (char& c : gross) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
        Texte t;
        t.ladeStr(p, b, gross);
        merke(paket, t, kleinPfad(sprache) == "english" ? std::string("English") : sprache);
    }
    for (const std::string& s : reihenfolge) { d.eigeneTexte.push_back(zeilen[s]); }
    if (d.textPaket.empty()) { d.textPaket = "MYMENU"; }
}

void eigeneTexteSchreiben(Dokument& d) {
    Zustand& z = Z();
    // alte Dateien dieses Pakets weg
    for (auto it = d.bilder.begin(); it != d.bilder.end();) {
        const std::string k = kleinPfad(it->first);
        const bool str = k.compare(0, 8, "strings/") == 0 && k.size() > 4 && k.compare(k.size() - 4, 4, ".str") == 0;
        const bool sp = k.compare(0, 6, "strip/") == 0 && k.size() > 3 && k.compare(k.size() - 3, 3, ".sp") == 0;
        it = (str || sp) ? d.bilder.erase(it) : std::next(it);
    }
    ++d.textStand;
    if (d.eigeneTexte.empty()) { return; }
    const std::string paket = d.textPaket;
    if (z.spiel == Spiel::JK2) {
        std::vector<TextEintragSprachen> e;
        for (const TextZeile& r : d.eigeneTexte) {
            TextEintragSprachen x;
            x.schluessel = r.schluessel;
            for (const auto& [s, t] : r.text) { x.texte[spNummer(s)] = t; }
            e.push_back(x);
        }
        std::string klein = paket;
        for (char& c : klein) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
        d.bilder["strip/" + klein + ".sp"] = schreibeSpSprachen(paket, e);
        return;
    }
    for (const std::string& s : sprachenListe()) {
        std::vector<TextEintrag> e;
        bool etwas = kleinPfad(s) == "english";
        for (const TextZeile& r : d.eigeneTexte) {
            TextEintrag x;
            x.schluessel = r.schluessel;
            const auto en = r.text.find("English");
            x.englisch = en == r.text.end() ? std::string() : en->second;
            const auto u = r.text.find(s);
            if (kleinPfad(s) != "english" && u != r.text.end() && !u->second.empty()) {
                x.uebersetzung = u->second;
                etwas = true;
            }
            e.push_back(x);
        }
        if (etwas) { d.bilder["strings/" + s + "/" + paket + ".str"] = schreibeStr(s, e); }
    }
}

// Die Texte der Vorschau: Spiel in der gewaehlten Sprache, darueber die eigenen
const Texte& vorschauTexte(Dokument& d) {
    Zustand& z = Z();
    const Texte& basis = texte();
    eigeneTexteLesen(d);
    if (d.eigeneTexte.empty()) { return basis; }
    const std::string schl = d.name + "|" + std::to_string(d.textStand) + "|" + z.sprache + "|" + std::to_string(z.texteVersion);
    if (z.texteMitSchluessel != schl) {
        z.texteMit = basis;
        for (const TextZeile& r : d.eigeneTexte) {
            std::string k = d.textPaket + "_" + r.schluessel;
            for (char& c : k) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
            const auto en = r.text.find("English");
            const auto l = r.text.find(z.sprache);
            z.texteMit.werte[k] = (l != r.text.end() && !l->second.empty()) ? l->second : (en != r.text.end() ? en->second : std::string());
        }
        z.texteMitSchluessel = schl;
    }
    return z.texteMit;
}

void setzeSprache(const std::string& sprache) {
    Zustand& z = Z();
    if (z.sprache == sprache) { return; }
    z.sprache = sprache;
    z.texteGeladen = false;
    ++z.texteVersion;
}

// Die @-Schluessel, die das Menue benutzt (text, text2, descText)
std::vector<std::string> benutzteSchluessel(const Dokument& d) {
    std::vector<std::string> v;
    std::set<std::string> gesehen;
    for (const Menu& m : d.datei.menus) {
        for (const Item& it : m.items) {
            for (const std::string* s : {&it.text, &it.text2, &it.descText}) {
                if (s->size() > 1 && (*s)[0] == '@' && gesehen.insert(kleinPfad(*s)).second) { v.push_back(*s); }
            }
        }
    }
    return v;
}

void zeichneTexteFenster(Dokument& d) {
    Zustand& z = Z();
    if (!z.texteFensterOffen) { return; }
    eigeneTexteLesen(d);
    ImGui::SetNextWindowSize(ImVec2{ImGui::GetFontSize() * 44, ImGui::GetFontSize() * 26}, ImGuiCond_Appearing);
    bool offen = true;
    if (ImGui::Begin(tr(Str::MeTexte), &offen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        const std::vector<std::string> sprachen = sprachenListe();
        bool geaendert = false;
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 10);
        std::string paket = d.textPaket;
        if (eingabe("REFERENCE##paket", &paket)) {
            for (char& c : paket) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
            if (!paket.empty()) {
                d.textPaket = paket;
                geaendert = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeTextNeu))) {
            TextZeile r;
            r.schluessel = "TEXT_" + std::to_string(d.eigeneTexte.size() + 1);
            d.eigeneTexte.push_back(r);
            geaendert = true;
        }
        const int spalten = 2 + static_cast<int>(sprachen.size());
        if (ImGui::BeginTable("##texte", spalten, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable,
                              ImVec2{0, ImGui::GetFontSize() * 12})) {
            ImGui::TableSetupColumn(tr(Str::MeTextSchluessel));
            for (const std::string& s : sprachen) { ImGui::TableSetupColumn(s.c_str()); }
            ImGui::TableSetupColumn("##x", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 1.5F);
            ImGui::TableHeadersRow();
            int weg = -1;
            for (std::size_t i = 0; i < d.eigeneTexte.size(); ++i) {
                TextZeile& r = d.eigeneTexte[i];
                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-1);
                std::string k = r.schluessel;
                if (eingabe("##k", &k)) {
                    for (char& c : k) { c = c == ' ' ? '_' : static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
                    r.schluessel = k;
                    geaendert = true;
                }
                for (const std::string& s : sprachen) {
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(-1);
                    std::string t = textAusDatei(s, r.text[s]);
                    if (eingabe(("##" + s).c_str(), &t)) {
                        std::string fehlt;
                        r.text[s] = textFuerDatei(s, t, &fehlt);
                        if (fehlt.empty()) { z.textFehlt.erase(s); }
                        else { z.textFehlt[s] = fehlt; }
                        geaendert = true;
                    }
                }
                ImGui::TableNextColumn();
                if (ImGui::SmallButton("x")) { weg = static_cast<int>(i); }
                ImGui::PopID();
            }
            ImGui::EndTable();
            if (weg >= 0) {
                d.eigeneTexte.erase(d.eigeneTexte.begin() + weg);
                geaendert = true;
            }
        }
        // M3: Zeichen, die die Engine in dieser Sprache nicht darstellt (als ? gespeichert)
        for (const auto& [sp, zeichen] : z.textFehlt) {
            char b[512];
            std::snprintf(b, sizeof(b), tr(Str::MeTextNichtDarstellbar), sp.c_str(), zeichen.c_str());
            ImGui::TextColored(ImVec4{1, 0.7F, 0.3F, 1}, "%s", b);
        }
        // im Menue benutzte Schluessel: aus dem Spiel, eigene, fehlende (anlegen)
        ImGui::SeparatorText("@");
        const Texte& tx = vorschauTexte(d);
        for (const std::string& s : benutzteSchluessel(d)) {
            ImGui::PushID(s.c_str());
            const bool da = tx.kennt(s);
            ImGui::TextColored(da ? ImVec4{0.6F, 0.9F, 0.6F, 1} : ImVec4{1, 0.5F, 0.4F, 1}, "%s", s.c_str());
            ImGui::SameLine();
            if (da) {
                ImGui::TextDisabled("%s", textAusDatei(z.sprache, tx.aufloesen(s)).c_str());
            } else {
                ImGui::TextDisabled("%s", tr(Str::MeTextFehlt));
                std::string g = s.substr(1);
                for (char& c : g) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
                const std::string vor = d.textPaket + "_";
                if (g.compare(0, vor.size(), vor) == 0) {
                    ImGui::SameLine();
                    if (ImGui::SmallButton(tr(Str::MeTextNeu))) {
                        TextZeile r;
                        r.schluessel = g.substr(vor.size());
                        d.eigeneTexte.push_back(r);
                        geaendert = true;
                    }
                }
            }
            ImGui::PopID();
        }
        if (geaendert) { eigeneTexteSchreiben(d); }
        int dateien = 0;
        std::string liste;
        for (const auto& [p, b] : d.bilder) {
            const std::string k = kleinPfad(p);
            if (k.compare(0, 8, "strings/") == 0 || k.compare(0, 6, "strip/") == 0) {
                ++dateien;
                liste += (liste.empty() ? "" : ", ") + p;
            }
        }
        char buf[1024];
        std::snprintf(buf, sizeof(buf), trN(Str::MeTexteImPaket, d.eigeneTexte.size()), static_cast<int>(d.eigeneTexte.size()), liste.c_str());
        ImGui::TextWrapped("%s", buf);
        (void)dateien;
    }
    ImGui::End();
    if (!offen) { z.texteFensterOffen = false; }
}

// --- M2 Punkt 7: fuer das Spiel prufen, nur Aenderungen exportieren ----------------------

// Die Dateien fuer ein Paket: geaenderte bzw. alle offenen Menues und ihre eigenen Dateien
// rc608 Teil 3b: UI_LoadMenus liest nur, was der Menuesatz nennt. Eine neue Menuedatei steht darum im Paket auch in einer Kopie der Liste des Spiels,
// sonst erscheint sie im Spiel nie. Eine selbst geoeffnete Liste bleibt die Grundlage. Listen: die Menue- und die Ingame-Liste jedes Formats.
void satzErgaenzen(std::vector<Pk3Datei>& v, const std::vector<std::string>& neu, Dialekt dl) {
    Zustand& z = Z();
    if (neu.empty()) { return; }
    std::vector<std::string> listen = menuelisten(*aktivesProfil(), dl);
    if (dl == Dialekt::SP) { listen.emplace_back("ui/ingame.txt"); }
    for (const std::string& liste : listen) {
        const auto schon = std::find_if(v.begin(), v.end(), [&liste](const Pk3Datei& f) { return kleinPfad(f.name) == kleinPfad(liste); });
        std::string text;
        if (schon != v.end()) { text = schon->inhalt; }
        else if (!z.daten.lies(liste, text)) { continue; }
        LeseOptionen o = leseOptionen(liste);
        o.dialekt = dl;
        const Datei satz = lies(liste, text, o);
        std::string zusatz;
        const std::string nl = text.find("\r\n") != std::string::npos ? "\r\n" : "\n";
        for (const std::string& n : neu) {
            const bool da = std::any_of(satz.loadMenus.begin(), satz.loadMenus.end(), [&n](const std::string& m) { return kleinPfad(m) == kleinPfad(n); });
            if (!da) { zusatz += "\tloadMenu { \"" + n + "\" }" + nl; }
        }
        const std::size_t zu = text.rfind('}');
        if (zusatz.empty() || zu == std::string::npos) { continue; }
        text.insert(zu, zusatz);
        if (schon != v.end()) { schon->inhalt = std::move(text); }
        else { v.push_back(Pk3Datei{liste, std::move(text)}); }
    }
}

std::vector<Pk3Datei> paketDateien(bool nurAenderungen) {
    Zustand& z = Z();
    std::vector<Pk3Datei> v;
    std::set<std::string> drin;
    std::vector<std::string> neuSp, neuMp;
    for (const auto& od : z.docs) {
        std::string original;
        const bool ausSpiel = z.daten.lies(od->name, original);
        const bool geaendert = !ausSpiel || original != od->puffer.text();
        if ((!nurAenderungen || geaendert) && drin.insert(kleinPfad(od->name)).second) { v.push_back(Pk3Datei{od->name, od->puffer.text()}); }
        const std::string k = kleinPfad(od->name);
        if (!ausSpiel && k.size() > 5 && k.compare(k.size() - 5, 5, ".menu") == 0) {
            neuParsen(*od);
            (od->datei.dialekt == Dialekt::MP ? neuMp : neuSp).push_back(od->name);
        }
        for (const auto& [p, b] : od->bilder) {
            if (drin.insert(kleinPfad(p)).second) { v.push_back(Pk3Datei{p, b}); }
        }
    }
    satzErgaenzen(v, neuSp, Dialekt::SP);
    satzErgaenzen(v, neuMp, Dialekt::MP);
    return v;
}

// rc608 Teil 3b: "Im Spiel testen" zeigt das Menue der offenen Seite gleich beim Start: Nur im Testpaket oeffnet es das Hauptmenue des Spiels,
// wie ein Spieler es sonst ueber einen Knopf erreicht. In der exportierten pk3 steht das nie.
std::vector<Pk3Datei> testPaketDateien() {
    Zustand& z = Z();
    std::vector<Pk3Datei> v = paketDateien(true);
    Dokument* d = doc();
    if (d == nullptr) { return v; }
    const int mi = menuDerSeite(*d);
    if (mi < 0) { return v; }
    const Dialekt dl = d->datei.dialekt;
    const std::string ziel = d->datei.menus[static_cast<std::size_t>(mi)].window.name;
    for (const std::string& e : einstiegsMenues(dl)) {
        if (gleichOhneFallStr(e, ziel)) { return v; }   // Das Spiel oeffnet es selbst
    }
    // Das Hauptmenue: im Satz die Datei mit dem Einstieg
    std::string haupt, text;
    for (const std::string& liste : menuelisten(*aktivesProfil(), dl)) {
        std::string t;
        if (!z.daten.lies(liste, t)) { continue; }
        LeseOptionen o = leseOptionen(liste);
        o.dialekt = dl;
        for (const std::string& f : lies(liste, t, o).loadMenus) {
            const std::string k = kleinPfad(f);
            if (k.size() >= 9 && k.compare(k.size() - 9, 9, "main.menu") == 0) {
                haupt = f;
                break;
            }
        }
        if (!haupt.empty()) { break; }
    }
    if (haupt.empty()) { return v; }
    const auto schon = std::find_if(v.begin(), v.end(), [&haupt](const Pk3Datei& f) { return kleinPfad(f.name) == kleinPfad(haupt); });
    if (schon != v.end()) { text = schon->inhalt; }
    else if (!z.daten.lies(haupt, text)) { return v; }
    LeseOptionen o = leseOptionen(haupt);
    o.dialekt = dl;
    const Datei hd = lies(haupt, text, o);
    if (hd.menus.empty()) { return v; }
    const Menu& hm = hd.menus.front();
    if (gleichOhneFallStr(hm.window.name, ziel)) { return v; }
    const std::string oeffnen = " close \"" + hm.window.name + "\" ; open \"" + ziel + "\" ; ";
    std::size_t stelle = std::string::npos;
    for (const Eigenschaft& e : hm.eigenschaften) {
        if (gleichOhneFallStr(e.schluessel, "onOpen") && !e.args.empty() && e.args[0].s == "{" && e.args[0].datei == 0) { stelle = e.args[0].ende; }
    }
    std::string neu = text;
    if (stelle != std::string::npos) {
        neu.insert(stelle, oeffnen);
    } else {
        const std::size_t auf = text.find('{', hm.anfang);
        if (auf == std::string::npos) { return v; }
        neu.insert(auf + 1, " onOpen {" + oeffnen + "} ");
    }
    if (schon != v.end()) { schon->inhalt = std::move(neu); }
    else { v.push_back(Pk3Datei{haupt, std::move(neu)}); }
    return v;
}

const Fluss& flussAktuell();   // app_menue_m6.inc

// Pruefung aller offenen Menues fuer die gewaehlten Fassungen (synchron, auf Knopfdruck)
int pruefeAlleMenues() {
    Zustand& z = Z();
    z.pruefErgebnis.clear();
    std::shared_ptr<const DiagDaten> dd = diagDaten();
    int fehler = 0;
    for (std::size_t i = 0; i < z.docs.size(); ++i) {
        Dokument& od = *z.docs[i];
        neuParsen(od);
        std::set<std::string> paket;
        for (const auto& [p, b] : od.bilder) { paket.insert(normPfad(p)); }
        PruefKontext k;
        k.zielSpiele.clear();
        const Spiel spiele[3] = {Spiel::JA, Spiel::JK2, Spiel::MD};
        for (int s = 0; s < 3; ++s) {
            if (z.pruefZiel[s]) { k.zielSpiele.push_back(spiele[s]); }
        }
        k.gibt = [&](const std::string& p0) {
            const std::string p = normPfad(p0);
            return paket.count(p) != 0 || dd->dateien.count(p) != 0;
        };
        k.gibtBild = [&](const std::string& n0) { return bildBekannt(*dd, n0, k.gibt); };
        DiagKontext dk;
        dk.pruefung = k;
        for (const Diagnose& x : diagnose(od.datei, dk)) {
            if (x.art != DiagArt::Pruefung) { continue; }
            z.pruefErgebnis.push_back(PruefZeile{static_cast<int>(i), x});
            fehler += x.befund.fehler ? 1 : 0;
        }
    }
    // rc608 Teil 3b: Ein Menue, das niemand oeffnet, sieht man im Spiel nie
    for (const FlussMenue& n : flussAktuell().menues) {
        if (n.datei < 0 || n.erreichbar || n.menu < 0) { continue; }
        Diagnose x;
        x.art = DiagArt::Pruefung;
        x.schwere = Schwere::Warnung;
        x.befund.art = BefundArt::NiemandOeffnet;
        x.befund.name = n.name;
        const Dokument& od = *z.docs[static_cast<std::size_t>(n.datei)];
        const Menu& m = od.datei.menus[static_cast<std::size_t>(n.menu)];
        x.zeile = m.zeile;
        x.anfang = m.anfang;
        x.ende = m.ende;
        z.pruefErgebnis.push_back(PruefZeile{n.datei, x});
    }
    z.pruefGelaufen = true;
    return fehler;
}

void zeichnePruefFenster() {
    Zustand& z = Z();
    if (!z.pruefFenster) { return; }
    ImGui::SetNextWindowSize(ImVec2{ImGui::GetFontSize() * 40, ImGui::GetFontSize() * 22}, ImGuiCond_Appearing);
    bool offen = true;
    if (ImGui::Begin(tr(Str::MePruefen), &offen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::Checkbox("Jedi Academy", &z.pruefZiel[0]);
        ImGui::SameLine();
        ImGui::Checkbox("Jedi Outcast", &z.pruefZiel[1]);
        ImGui::SameLine();
        ImGui::Checkbox("Movie Duels", &z.pruefZiel[2]);
        ImGui::SameLine();
        // rc608 Teil 3b: Die Liste gehoert zum jetzigen Stand; eine alte nannte Dateien und Zeilen, die es so nicht mehr gibt
        std::string stand;
        for (const auto& od : z.docs) { stand += std::to_string(reinterpret_cast<std::uintptr_t>(od.get())) + ":" + std::to_string(od->puffer.version()) + ";"; }
        if (ImGui::Button(tr(Str::MePruefen)) || !z.pruefGelaufen || stand != z.pruefStand) {
            (void)pruefeAlleMenues();
            z.pruefStand = stand;
        }
        char buf[128];
        std::snprintf(buf, sizeof(buf), trN(Str::MePruefungAnzahl, z.pruefErgebnis.size()), static_cast<int>(z.pruefErgebnis.size()));
        ImGui::TextUnformatted(z.pruefErgebnis.empty() ? tr(Str::MePruefungOk) : buf);
        ImGui::BeginChild("##pruefliste");
        z.pruefZeilen.clear();
        for (std::size_t i = 0; i < z.pruefErgebnis.size(); ++i) {
            const PruefZeile& r = z.pruefErgebnis[i];
            if (r.dok < 0 || static_cast<std::size_t>(r.dok) >= z.docs.size()) { continue; }
            ImGui::PushID(static_cast<int>(i));
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float fs = ImGui::GetFontSize();
            schwereSymbol(ImGui::GetWindowDrawList(), ImVec2{p.x + fs * 0.4F, p.y + fs * 0.55F}, fs * 0.28F, r.x.schwere);
            ImGui::Dummy(ImVec2{fs * 0.9F, fs});
            ImGui::SameLine();
            const std::string t = z.docs[static_cast<std::size_t>(r.dok)]->name + ":" + std::to_string(r.x.zeile) + "  " + diagMeldung(r.x);
            // rc608 Teil 3b: Lange Meldungen brechen um, die ganze Meldung ist die Klickflaeche
            const float breit = std::max(fs * 4.0F, ImGui::GetContentRegionAvail().x);
            const float hoch = ImGui::CalcTextSize(t.c_str(), nullptr, false, breit).y;
            const ImVec2 oben = ImGui::GetCursorScreenPos();
            const bool gewaehlt = ImGui::Selectable(("##zeile" + std::to_string(i)).c_str(), false, ImGuiSelectableFlags_None, ImVec2{breit, hoch});
            ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), fs, oben, ImGui::GetColorU32(ImGuiCol_Text), t.c_str(), nullptr, breit);
            z.pruefZeilen.push_back(t);
            if (gewaehlt) {
                z.aktiv = r.dok;
                Dokument& od = *z.docs[static_cast<std::size_t>(r.dok)];
                if (r.x.ende > r.x.anfang) { springe(od, r.x.anfang); }
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    ImGui::End();
    if (!offen) { z.pruefFenster = false; }
}

// --- M2 Punkt 8: Im Spiel testen ---------------------------------------------------------

bool loseDateienSchreiben(const std::string& ordner);

std::string menueImSpielTesten(bool modErlaubt) {
    Zustand& z = Z();
    Dokument* d = doc();
    if (d == nullptr || z.daten.ordner.empty()) {
        z.meldung = tr(Str::MeTestKeinSpiel);
        return {};
    }
    // Programm: neben dem ersten Spielordner, passend zu SP/MP und Spiel
    const fs::path basis = utf8Pfad(z.daten.ordner.front().directory);
    const fs::path engine = basis.parent_path();
    const bool mp = d->datei.dialekt == Dialekt::MP;
    std::string exe;
    for (int runde = 0; runde < 2 && exe.empty(); ++runde) {
        for (const ::bhed::karte::TestEngine& t : ::bhed::karte::testEngines()) {
            if (t.mp != mp || t.jk2 != (z.spiel == Spiel::JK2)) { continue; }
            const bool md = std::string(t.exe).find("MovieDuels") != std::string::npos;
            if (runde == 0 && md != (z.spiel == Spiel::MD)) { continue; }
            std::error_code ec;
            if (fs::is_regular_file(engine / t.exe, ec)) {
                exe = t.exe;
                break;
            }
        }
    }
    MenueTestEingabe e;
    e.enginePfad = u8(engine);
    std::replace(e.enginePfad.begin(), e.enginePfad.end(), '\\', '/');
    if (!e.enginePfad.empty() && e.enginePfad.back() != '/') { e.enginePfad += '/'; }
    e.exe = exe;
    e.basisSpiel = u8(basis.filename());
    if (z.daten.ordner.size() > 1) { e.fsGame = u8(utf8Pfad(z.daten.ordner.back().directory).filename()); }
    e.testOrdner = platform::settingsDirectory() + "\\menuetest";
    e.modOrdnerErlaubt = modErlaubt;
    if (!exe.empty()) {
        std::ifstream f(engine / exe, std::ios::binary);
        std::ostringstream o;
        o << f.rdbuf();
        e.faehigkeit = ::bhed::karte::engineFaehigkeit(o.str());
    }
    const MenueTestPlan plan = menueTestPlanen(e);
    if (plan.weg == ::bhed::karte::SpielTestWeg::Keiner) {
        z.meldung = tr(Str::MeTestKeinSpiel);
        return {};
    }
    if (plan.bestaetigungNoetig) {
        z.testFrage = true;
        ++z.spielTests;   // das Tutorial: Die Frage steht, weiter geht es beim Nutzer
        return {};
    }
    if (plan.weg != ::bhed::karte::SpielTestWeg::ModOrdner && imSpielordner(plan.pk3)) {
        z.meldung = tr(Str::MeNieSpielordner);
        return {};
    }
    std::error_code ec;
    fs::create_directories(utf8Pfad(plan.pk3).parent_path(), ec);
    std::string fehler;
    if (plan.uiLoad) {   // M7 Punkt 13: lose Dateien, die ui_load spaeter neu liest
        fs::remove(utf8Pfad(plan.pk3), ec);   // eine alte pk3 im selben Ordner stuende darueber
        if (!loseDateienSchreiben(plan.ordner)) { return {}; }
    } else if (!writePk3(plan.pk3, testPaketDateien(), &fehler)) {
        z.meldung = fehler;
        return {};
    }
    ++z.spielTests;
    z.testBefehl = plan.befehl;
    z.testUiLoad = plan.uiLoad;
    z.testOrdnerLose = plan.uiLoad ? plan.ordner : std::string();
    char buf[2048];
    std::snprintf(buf, sizeof(buf), tr(Str::MeTestBereit), plan.pk3.c_str(), plan.befehl.c_str());
    z.meldung = buf;
    addStatus(buf);
    // Im Selbsttest (BHED_EDITORTEST) nur die Befehlszeile: nie ein Spiel starten
    if (std::getenv("BHED_EDITORTEST") == nullptr) {
        unsigned long pid = 0;
        if (bauplattform::losgeloestStarten(plan.befehl, u8(engine), &pid)) { z.testPid = plan.uiLoad ? pid : 0; }
    }
    return plan.befehl;
}

void zeichneTestFrage() {
    Zustand& z = Z();
    if (z.testFrage) {
        ImGui::OpenPopup("##testfrage");
        z.testFrage = false;
    }
    if (z.testFrageZu && !ImGui::IsPopupOpen("##testfrage")) { z.testFrageZu = false; }   // keine Frage offen
    if (ImGui::BeginPopupModal("##testfrage", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (z.testFrageZu) { ImGui::CloseCurrentPopup(); }
        z.testFrageZu = false;
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28);
        ImGui::TextUnformatted(tr(Str::MeTestModFrage));
        ImGui::PopTextWrapPos();
        if (ImGui::Button(tr(Str::MeJa))) {
            ImGui::CloseCurrentPopup();
            (void)menueImSpielTesten(true);
        }
        ImGui::SameLine();
        if (ImGui::Button(tr(Str::MeNein))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
}

// --- M2 Punkt 9: Schrift aus TTF -----------------------------------------------------------

bool schriftBauen(const std::string& ttfPfad, const std::string& name0, int groesse) {
    Zustand& z = Z();
    Dokument* d = doc();
    if (d == nullptr) { return false; }
    std::string ttf;
    {
        std::ifstream f(utf8Pfad(ttfPfad), std::ios::binary);
        std::ostringstream o;
        o << f.rdbuf();
        ttf = o.str();
    }
    std::vector<GlyphBild> zeichen;
    SchriftMasse m;
    if (!rastereTtf(ttf, groesse, zeichen, m)) {
        z.meldung = tr(Str::MeSchriftFehler);
        return false;
    }
    std::string name;
    for (const char c : name0) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') { name += static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    }
    if (name.empty()) { name = "eigene"; }
    const SchriftDateien f = baueSchrift(zeichen, m);
    d->bilder["fonts/" + name + ".fontdat"] = f.fontdat;
    d->bilder["fonts/" + name + ".tga"] = f.tga;
    z.schriften.erase(name);
    z.texturen.erase("fonts/" + name);
    char buf[512];
    std::snprintf(buf, sizeof(buf), tr(Str::MeSchriftFertig), name.c_str());
    z.meldung = buf;
    return true;
}

void zeichneSchriftFenster() {
    Zustand& z = Z();
    if (!z.schriftFenster) { return; }
    ImGui::SetNextWindowSize(ImVec2{ImGui::GetFontSize() * 24, 0}, ImGuiCond_Appearing);
    bool offen = true;
    if (ImGui::Begin(tr(Str::MeSchriftBauen), &offen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextWrapped("%s", anzeige(z.schriftTtf).c_str());
        eingabe(tr(Str::MeSchriftName), &z.schriftName);
        ImGui::SliderInt(tr(Str::MeSchriftGroesse), &z.schriftGroesse, 8, 64);
        if (ImGui::Button(tr(Str::MeSchriftBauen))) {
            if (schriftBauen(z.schriftTtf, z.schriftName, z.schriftGroesse)) { offen = false; }
        }
    }
    ImGui::End();
    if (!offen) { z.schriftFenster = false; }
}

// Breite je Zeichen (Latin-1 wie anzeige()), einmal je Schriftgroesse - die Spalten der
// Zeilen werden je Bild daraus aufsummiert statt je Zeichen gemessen.
const float* zeichenBreiten(float leer) {
    static float b[256];
    static float fuer = -1.0F;
    static ImFont* schrift = nullptr;
    ImFont* f = ImGui::GetFont();
    const float fs = ImGui::GetFontSize();
    if (fuer != fs || schrift != f) {
        for (int c = 0; c < 256; ++c) {
            const char ch = static_cast<char>(c);
            const std::string a = anzeige(std::string_view(&ch, 1));
            b[c] = c < 32 ? leer : f->CalcTextSizeA(fs, 1e9F, 0.0F, a.c_str(), a.c_str() + a.size()).x;
        }
        fuer = fs;
        schrift = f;
    }
    return b;
}

// x jeder Spalte 0..n einer Zeile (Tabs auf 4 Leerzeichen wie spaltenX)
void zeilenXe(std::string_view zeile, float leer, std::vector<float>& xs) {
    const float* br = zeichenBreiten(leer);
    xs.resize(zeile.size() + 1);
    float x = 0;
    for (std::size_t i = 0; i < zeile.size(); ++i) {
        xs[i] = x;
        const char c = zeile[i];
        if (c == '\t') { x = (std::floor(x / (leer * 4.0F)) + 1.0F) * leer * 4.0F; }
        else { x += br[static_cast<unsigned char>(c)]; }
    }
    xs[zeile.size()] = x;
}

// M2: der Code-Editor als eigenes Panel (fuellt den verfuegbaren Platz). Lange Zeilen
// werden umgebrochen (Einstellung "Umbrechen", an Leerzeichen, sonst hart) oder
// waagerecht gescrollt (Umschalt+Rad, folgt dem Cursor).
void zeichneCode(Dokument& d) {
    Zustand& z = Z();
    if (z.suchenOffen) { zeichneSuchleiste(d); }
    ImGui::BeginChild("##code", ImVec2{0, 0}, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    // M2: Festbreitenschrift (Spalten, Einrueckung und Minimap stehen untereinander). rc608 Teil 3a: in der Groesse des Zooms; alles darunter rechnet mit dieser Schrift
    Settings& einst = g_app->settings;
    einst.menueTextZoom = std::clamp(einst.menueTextZoom, kTextZoomMin, kTextZoomMax);
    ImGui::PushFont(g_codeSchrift, ImGui::GetStyle().FontSizeBase * static_cast<float>(einst.menueTextZoom) / 100.0F);
    Textpuffer& p = d.puffer;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImFont* font = ImGui::GetFont();
    const float fs = ImGui::GetFontSize();
    const float zh = std::floor(ImGui::GetTextLineHeightWithSpacing());
    const float leer = font->CalcTextSizeA(fs, 1e9F, 0.0F, " ").x;
    const ImVec2 o = ImGui::GetCursorScreenPos();
    const ImVec2 bereich = ImGui::GetContentRegionAvail();
    const int n = p.zeilen();
    char nr[16];
    std::snprintf(nr, sizeof(nr), "%d", n);
    const float nummernB = font->CalcTextSizeA(fs, 1e9F, 0.0F, nr).x + fs * 0.6F;
    const float rinne = randBreiteSkript();
    const float falteB = fs;
    const float textX = o.x + nummernB + rinne + falteB + 4.0F;
    // Minimap: im schmalen Editor kleiner, unter 20 Zeichen Breite ganz weg
    const float miniB = bereich.x < fs * 20 ? 0.0F : bereich.x < fs * 34 ? std::floor(fs * 3.0F) : std::floor(fs * 5.0F);
    const float textB = std::max(leer * 8, bereich.x - (textX - o.x) - miniB);
    const bool umbruch = g_app->settings.menueUmbruch;
    // sichtbare Zeilen (eingeklappte ueberspringen)
    const std::vector<Falte> falten = p.falten();
    std::vector<int> sichtbar;
    sichtbar.reserve(static_cast<std::size_t>(n));
    {
        int z0 = 0;
        while (z0 < n) {
            sichtbar.push_back(z0);
            int naechste = z0 + 1;
            if (d.gefaltet.count(z0) != 0) {
                for (const Falte& f : falten) {
                    if (f.von == z0) { naechste = f.bis; break; }
                }
            }
            z0 = std::max(naechste, z0 + 1);
        }
    }
    const int sichtbarN = static_cast<int>(sichtbar.size());
    // Reihen auf dem Schirm: je sichtbare Zeile eine, mit Umbruch mehrere
    struct Reihe {
        int zl = 0, a = 0, e = 0;   // Zeile, Spanne [a, e) in der Zeile
        bool letzte = true;
    };
    std::vector<Reihe> reihen;
    reihen.reserve(sichtbar.size());
    std::vector<float> xs;
    float breiteste = 0;
    for (const int zl : sichtbar) {
        const std::string_view zeile = p.zeile(zl);
        if (!umbruch) {
            reihen.push_back(Reihe{zl, 0, static_cast<int>(zeile.size()), true});
            breiteste = std::max(breiteste, static_cast<float>(zeile.size()) * leer);
            continue;
        }
        zeilenXe(zeile, leer, xs);
        int a = 0;
        const int len = static_cast<int>(zeile.size());
        while (true) {
            int e = a;
            while (e < len && xs[static_cast<std::size_t>(e) + 1] - xs[static_cast<std::size_t>(a)] <= textB - leer) { ++e; }
            if (e >= len) {
                reihen.push_back(Reihe{zl, a, len, true});
                break;
            }
            int b = e;   // am letzten Leerzeichen umbrechen, wenn eins in der Reihe liegt
            while (b > a + 1 && zeile[static_cast<std::size_t>(b) - 1] != ' ' && zeile[static_cast<std::size_t>(b) - 1] != '\t') { --b; }
            if (b <= a + 1) { b = std::max(e, a + 1); }
            reihen.push_back(Reihe{zl, a, b, false});
            a = b;
        }
    }
    const int reihenN = static_cast<int>(reihen.size());
    auto reiheVonZeile = [&](int zl) {
        const auto it = std::lower_bound(reihen.begin(), reihen.end(), zl, [](const Reihe& r, int x) { return r.zl < x; });
        return static_cast<int>(it - reihen.begin());
    };
    auto reiheVonOff = [&](std::uint32_t off) {
        const Pos cp = p.pos(off);
        int r = reiheVonZeile(cp.zeile);
        while (r + 1 < reihenN && reihen[static_cast<std::size_t>(r) + 1].zl == cp.zeile && reihen[static_cast<std::size_t>(r) + 1].a <= cp.spalte) { ++r; }
        return r;
    };
    const int proSeite = std::max(1, static_cast<int>(bereich.y / zh));
    // Scrollen (Umschalt+Rad bzw. das waagrechte Rad: seitwaerts ohne Umbruch)
    const ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsWindowHovered()) { z.codeUeberBild = ImGui::GetFrameCount(); }
    // rc608 Teil 3a: Strg+Rad aendert hier die Schriftgroesse; nur Ansicht, kein Undo-Schritt, nie in der Datei
    if (ImGui::IsWindowHovered() && io.KeyCtrl && io.MouseWheel != 0.0F) {
        const int schritt = io.MouseWheel > 0.0F ? 10 : -10;
        einst.menueTextZoom = std::clamp((einst.menueTextZoom + schritt) / 10 * 10, kTextZoomMin, kTextZoomMax);
        z.codeZoomAb = ImGui::GetTime();
    }
    if (ImGui::IsWindowHovered() && !io.KeyCtrl) {
        if (io.MouseWheel != 0.0F && !io.KeyShift) { d.scrollZeile -= io.MouseWheel * 3.0F; }
        if (!umbruch) {
            const float seit = io.KeyShift ? io.MouseWheel : io.MouseWheelH;
            if (seit != 0.0F) { d.scrollX -= seit * leer * 8.0F; }
        }
    }
    if (d.springeZeile >= 0) {
        const int idx = reiheVonZeile(d.springeZeile);
        if (static_cast<float>(idx) < d.scrollZeile || static_cast<float>(idx) >= d.scrollZeile + static_cast<float>(proSeite) - 1.0F) {
            d.scrollZeile = static_cast<float>(idx - proSeite / 3);
        }
        d.springeZeile = -1;
    }
    d.scrollZeile = std::clamp(d.scrollZeile, 0.0F, static_cast<float>(std::max(0, reihenN - proSeite / 2)));
    d.scrollX = umbruch ? 0.0F : std::clamp(d.scrollX, 0.0F, std::max(0.0F, breiteste - textB + leer * 4));
    const int erste = static_cast<int>(d.scrollZeile);
    // x einer Spalte in einer Reihe (Bildschirm)
    int xsZeile = -1;
    auto xsFuer = [&](int zl) {
        if (xsZeile != zl) {
            zeilenXe(p.zeile(zl), leer, xs);
            xsZeile = zl;
        }
    };
    auto spalteX = [&](const Reihe& r, int s) {
        xsFuer(r.zl);
        const float x0 = umbruch ? xs[static_cast<std::size_t>(r.a)] : d.scrollX;
        return textX + xs[static_cast<std::size_t>(std::clamp(s, 0, static_cast<int>(xs.size()) - 1))] - x0;
    };
    // Eingabe
    ImGui::InvisibleButton("##codeflaeche", ImVec2{std::max(1.0F, bereich.x), std::max(1.0F, bereich.y)});
    const bool hover = ImGui::IsItemHovered();
    // rc608 Punkt 16: ueber dem Text der Text-Cursor, im Zeilenrand und ueber der Minimap der Pfeil
    if (hover && io.MousePos.x >= textX && io.MousePos.x < textX + textB && !z.balkenRect.Contains(io.MousePos)) { ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput); }
    if (ImGui::IsItemActivated()) { z.codeFokus = true; }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !hover && !ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
        z.codeFokus = false;
    }
    const ImVec2 maus = io.MousePos;
    auto reiheBeiMaus = [&]() { return std::clamp(erste + static_cast<int>(std::floor((maus.y - o.y) / zh)), 0, std::max(0, reihenN - 1)); };
    auto offBeiMaus = [&]() -> std::uint32_t {
        if (reihenN == 0) { return 0; }
        const Reihe& r = reihen[static_cast<std::size_t>(reiheBeiMaus())];
        int s = r.a;
        while (s < r.e && maus.x >= (spalteX(r, s) + spalteX(r, s + 1)) * 0.5F) { ++s; }
        if (!r.letzte && s >= r.e) { s = r.e - 1; }
        return p.off(Pos{r.zl, s});
    };
    if (hover && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const int idx = erste + static_cast<int>(std::floor((maus.y - o.y) / zh));
        const bool inReihen = idx >= 0 && idx < reihenN;
        if (maus.x < textX - falteB) {
            // Rand: Lesezeichen umschalten (Klick in den Rand wie Notepad++)
            if (inReihen) {
                const int zl = reihen[static_cast<std::size_t>(idx)].zl;
                if (d.lesezeichen.count(zl) != 0) { d.lesezeichen.erase(zl); } else { d.lesezeichen.insert(zl); }
            }
        } else if (maus.x < textX) {
            if (inReihen) {
                const int zl = reihen[static_cast<std::size_t>(idx)].zl;
                if (d.gefaltet.count(zl) != 0) { d.gefaltet.erase(zl); } else {
                    for (const Falte& f : falten) {
                        if (f.von == zl) { d.gefaltet.insert(zl); }
                    }
                }
            }
        } else if (miniB > 0 && maus.x > textX + textB) {
            // Minimap: dorthin springen
            const float anteil = (maus.y - o.y) / std::max(1.0F, bereich.y);
            const int zIdx = std::clamp(static_cast<int>(anteil * static_cast<float>(sichtbarN)), 0, std::max(0, sichtbarN - 1));
            d.scrollZeile = static_cast<float>(reiheVonZeile(sichtbarN > 0 ? sichtbar[static_cast<std::size_t>(zIdx)] : 0)) - static_cast<float>(proSeite) * 0.5F;
        } else {
            const std::uint32_t off = offBeiMaus();
            if (io.KeyAlt) {
                p.cursorDazu(off);
            } else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                const Treffer w = p.wortBei(off);
                p.cursor.assign(1, Cursor{w.ende, w.anfang});
            } else {
                p.einCursor(off, io.KeyShift);
                z.ziehtText = true;
            }
            if (io.KeyCtrl) { gehZuDefinition(d); }
            auswahlAusCursor(d);
            z.vervollOffen = false;
        }
    }
    if (z.ziehtText) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            p.cursor.front().pos = offBeiMaus();
            if (maus.y < o.y) { d.scrollZeile -= 1; }
            if (maus.y > o.y + bereich.y) { d.scrollZeile += 1; }
        } else {
            z.ziehtText = false;
        }
    }
    if (z.codeFokus && ImGui::IsWindowFocused()) {
        z.codeBild = ImGui::GetFrameCount();
        ImGui::GetIO().WantTextInput = true;
        const int v = p.version();
        codeTasten(d);
        if (p.version() != v) { neuParsen(d); }
    }
    // ohne Umbruch: dem Cursor seitwaerts folgen, wenn er sich bewegt hat
    const std::uint32_t cur = p.cursor.front().pos;
    if (!umbruch && cur != d.letzterCursor && reihenN > 0) {
        const Pos cp = p.pos(cur);
        zeilenXe(p.zeile(cp.zeile), leer, xs);
        xsZeile = cp.zeile;
        const float x = xs[static_cast<std::size_t>(std::clamp(cp.spalte, 0, static_cast<int>(xs.size()) - 1))];
        if (x < d.scrollX) { d.scrollX = std::max(0.0F, x - leer * 4); }
        if (x > d.scrollX + textB - leer * 2) { d.scrollX = x - textB + leer * 4; }
    }
    d.letzterCursor = cur;
    // --- Zeichnen ---
    const std::vector<Farbstueck> farben = faerbe(p.text(), d.datei.dialekt, d.datei.spiel, &d.datei.defines);
    const ZeilenDiff seitSpeichern = zeilenDiff(d.gespeichert, p.text());
    const ZeilenDiff seitLaden = zeilenDiff(d.geladen, p.text());
    // M2 Live-Diagnose: die schwerste Meldung je Zeile (Rand, Minimap)
    std::map<int, Schwere> zeilenSchwere;
    for (const Diagnose& x : d.diagnosen) {
        if (x.zeile <= 0) { continue; }
        const auto it = zeilenSchwere.find(x.zeile - 1);
        if (it == zeilenSchwere.end() || x.schwere < it->second) { zeilenSchwere[x.zeile - 1] = x.schwere; }
    }
    // Klammerpaar und Wort unter dem Cursor
    const std::optional<std::uint32_t> partner = p.partnerKlammer(cur);
    std::uint32_t klammer = cur;
    if (partner && (cur >= p.text().size() || std::string("{}()[]").find(p.text()[cur]) == std::string::npos)) { klammer = cur - 1; }
    const Treffer wort = p.wortBei(cur);
    const std::string wortText = p.text().substr(wort.anfang, wort.ende - wort.anfang);
    std::vector<Treffer> gleiche;
    if (wortText.size() >= 2) {
        SuchOptionen so;
        so.gross = true;
        so.wort = true;
        gleiche = p.suche(wortText, so);
    }
    std::vector<Treffer> suchT;
    if (z.suchenOffen) { suchT = p.suche(z.such, z.suchOpt); }
    dl->PushClipRect(o, ImVec2{o.x + bereich.x, o.y + bereich.y}, true);
    const int curZeile = p.pos(cur).zeile;
    // Rand: Nummern, Lesezeichen, Aenderungen, Falten (je Zeile in ihrer ersten Reihe)
    for (int k = erste; k < reihenN && k < erste + proSeite + 1; ++k) {
        const Reihe& r = reihen[static_cast<std::size_t>(k)];
        if (r.a != 0) { continue; }
        const int zl = r.zl;
        const float y = o.y + static_cast<float>(k - erste) * zh;
        std::snprintf(nr, sizeof(nr), "%d", zl + 1);
        dl->AddText(ImVec2{o.x + nummernB - fs * 0.3F - font->CalcTextSizeA(fs, 1e9F, 0.0F, nr).x, y},
                    curZeile == zl ? IM_COL32(220, 220, 220, 255) : IM_COL32(120, 120, 120, 255), nr);
        std::uint8_t marke = kMarkeKeine;
        const bool seitS = static_cast<std::size_t>(zl) < seitSpeichern.geaendert.size() && seitSpeichern.geaendert[static_cast<std::size_t>(zl)] != 0;
        const bool seitL = static_cast<std::size_t>(zl) < seitLaden.geaendert.size() && seitLaden.geaendert[static_cast<std::size_t>(zl)] != 0;
        if (seitS) { marke = seitL ? kMarkeGeaendert : kMarkeZurueck; }
        else if (seitL) { marke = kMarkeGespeichert; }
        randZeichnenSkript(dl, o.x + nummernB, rinne, y, zh, marke, d.lesezeichen.count(zl) != 0);
        if (const auto zs = zeilenSchwere.find(zl); zs != zeilenSchwere.end()) {
            schwereSymbol(dl, ImVec2{o.x + nummernB - fs * 0.05F, y + zh * 0.5F}, fs * 0.22F, zs->second);
        }
        for (const Falte& f : falten) {
            if (f.von == zl) {
                dl->AddText(ImVec2{textX - falteB, y}, IM_COL32(160, 160, 160, 255), d.gefaltet.count(zl) != 0 ? "+" : "-");
                break;
            }
        }
    }
    // Text (eigener Ausschnitt: seitwaerts gescrollt laeuft er nicht in den Rand)
    dl->PushClipRect(ImVec2{textX, o.y}, ImVec2{textX + textB, o.y + bereich.y}, true);
    std::size_t fi = 0;
    for (int k = erste; k < reihenN && k < erste + proSeite + 1; ++k) {
        const Reihe& r = reihen[static_cast<std::size_t>(k)];
        const int zl = r.zl;
        const float y = o.y + static_cast<float>(k - erste) * zh;
        const std::uint32_t za = p.zeilenAnfang(zl);
        const std::uint32_t ze = p.zeilenEnde(zl);
        const std::uint32_t ra = za + static_cast<std::uint32_t>(r.a), re = za + static_cast<std::uint32_t>(r.e);
        const std::string_view zeile = p.zeile(zl);
        xsFuer(zl);
        if (const auto zs = zeilenSchwere.find(zl); zs != zeilenSchwere.end() && zs->second == Schwere::Fehler) {
            dl->AddRectFilled(ImVec2{textX, y}, ImVec2{textX + textB, y + zh}, IM_COL32(255, 40, 40, 28));
        }
        if (curZeile == zl && p.cursor.size() == 1 && !p.cursor.front().auswahl()) {
            dl->AddRectFilled(ImVec2{textX, y}, ImVec2{textX + textB, y + zh}, IM_COL32(255, 255, 255, 12));
        }
        // Auswahl, Suchtreffer, gleiche Woerter
        auto markiere = [&](std::uint32_t a, std::uint32_t e, ImU32 col) {
            if (e <= ra || a > re || (a == re && !r.letzte)) { return; }
            const int sa = static_cast<int>(std::max(a, ra) - za);
            const int se = static_cast<int>(std::min(e, re) - za);
            const float xa = spalteX(r, sa);
            float xe = spalteX(r, se);
            if (e > ze && r.letzte) { xe += leer; }
            dl->AddRectFilled(ImVec2{xa, y}, ImVec2{std::max(xe, xa + 2), y + zh}, col);
        };
        for (const Treffer& t : gleiche) { markiere(t.anfang, t.ende, IM_COL32(255, 255, 255, 25)); }
        for (const Treffer& t : suchT) { markiere(t.anfang, t.ende, IM_COL32(255, 160, 0, 90)); }
        for (const Cursor& c : p.cursor) {
            if (c.auswahl()) { markiere(c.von(), c.bis(), IM_COL32(38, 79, 120, 220)); }
        }
        if (partner) {
            markiere(klammer, klammer + 1, IM_COL32(255, 255, 255, 60));
            markiere(*partner, *partner + 1, IM_COL32(255, 255, 255, 60));
        }
        // Text mit Farben
        while (fi < farben.size() && farben[fi].ende <= ra) { ++fi; }
        std::uint32_t pos = ra;
        std::size_t fj = fi;
        while (pos < re) {
            std::uint32_t bis = re;
            ImU32 col = farbeFuer(Farbklasse::Text);
            if (fj < farben.size() && farben[fj].anfang <= pos) {
                bis = std::min(re, farben[fj].ende);
                col = farbeFuer(farben[fj].klasse);
                if (farben[fj].ende <= bis) { ++fj; }
            } else if (fj < farben.size()) {
                bis = std::min(re, farben[fj].anfang);
            }
            if (bis <= pos) { bis = pos + 1; }
            const int sa = static_cast<int>(pos - za);
            const std::string stueck = anzeige(zeile.substr(static_cast<std::size_t>(sa), bis - pos));
            // Tabs einzeln (Spaltenbreite)
            std::size_t s0 = 0;
            int spalte = sa;
            for (std::size_t q = 0; q <= stueck.size(); ++q) {
                if (q == stueck.size() || stueck[q] == '\t') {
                    if (q > s0) { dl->AddText(font, fs, ImVec2{spalteX(r, spalte), y}, col, stueck.c_str() + s0, stueck.c_str() + q); }
                    if (q < stueck.size()) {
                        spalte = sa + static_cast<int>(q) + 1;
                        s0 = q + 1;
                    }
                }
            }
            pos = bis;
        }
        // M2 Live-Diagnose: Wellenlinie an der Stelle, Rahmen um die Gegenstelle (Klammern)
        for (const Diagnose& x : d.diagnosen) {
            auto strich = [&](std::uint32_t a, std::uint32_t e, bool rahmen) {
                if (std::max(e, a + 1) <= ra || a > re || (a == re && !r.letzte)) { return; }
                const int sa = static_cast<int>(std::max(a, ra) - za);
                const int se = static_cast<int>(std::min(std::max(e, a + 1), re) - za);
                const float xa = spalteX(r, sa);
                const float xe = std::max(spalteX(r, se), xa + leer * 0.5F);
                if (rahmen) { dl->AddRect(ImVec2{xa - 1, y}, ImVec2{xe + 1, y + zh}, schwereFarbe(x.schwere), 2.0F); }
                else { welle(dl, xa, xe, y + zh - 2.0F, schwereFarbe(x.schwere)); }
            };
            if (x.ende > x.anfang) { strich(x.anfang, x.ende, false); }
            if (x.partner) { strich(x.partnerAnfang, x.partnerEnde, true); }
        }
        // Cursor
        if (z.codeFokus && std::fmod(ImGui::GetTime(), 1.0) < 0.6) {
            for (const Cursor& c : p.cursor) {
                if (c.pos >= ra && (c.pos < re || (c.pos == re && r.letzte))) {
                    const float x = spalteX(r, static_cast<int>(c.pos - za));
                    dl->AddLine(ImVec2{x, y}, ImVec2{x, y + zh}, IM_COL32(255, 255, 255, 255), 1.5F);
                }
            }
        }
        if (r.letzte && d.gefaltet.count(zl) != 0) {
            const float x = spalteX(r, static_cast<int>(zeile.size())) + leer;
            dl->AddRectFilled(ImVec2{x, y + 2}, ImVec2{x + leer * 4, y + zh - 2}, IM_COL32(80, 80, 80, 255), 3);
            dl->AddText(ImVec2{x + leer * 0.5F, y}, IM_COL32(200, 200, 200, 255), "...");
        }
    }
    dl->PopClipRect();
    // rc608 Punkt 2: ohne Umbruch ein richtiger Rollbalken unten (Umschalt+Mausrad oder daran ziehen)
    if (!umbruch && breiteste > textB) {
        const float dicke = std::floor(fs * 0.6F);
        const float a = d.scrollX / breiteste, b = std::min(1.0F, (d.scrollX + textB) / breiteste);
        const float by = o.y + bereich.y - dicke;
        const bool imBalken = maus.y >= by && maus.y <= o.y + bereich.y && maus.x >= textX && maus.x <= textX + textB;
        if (hover && imBalken && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            z.balkenZieht = true;
            z.balkenGriff = maus.x >= textX + a * textB && maus.x <= textX + b * textB ? maus.x - (textX + a * textB) : (b - a) * textB * 0.5F;
        }
        if (z.balkenZieht) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) { d.scrollX = std::max(0.0F, (maus.x - z.balkenGriff - textX) / textB * breiteste); }
            else { z.balkenZieht = false; }
        }
        z.balkenRect = ImRect(ImVec2{textX, by}, ImVec2{textX + textB, by + dicke});
        dl->AddRectFilled(ImVec2{textX, by}, ImVec2{textX + textB, by + dicke}, IM_COL32(255, 255, 255, 18));
        dl->AddRectFilled(ImVec2{textX + a * textB, by}, ImVec2{textX + b * textB, by + dicke}, IM_COL32(255, 255, 255, z.balkenZieht || imBalken ? 150 : 90), dicke * 0.5F);
    } else {
        z.balkenRect = ImRect();
    }
    // Selbsttest: wie viele Zeichen gerade zu sehen sind (Vorgabe Umbruch an oder aus)
    {
        long long zeichen = 0;
        const int spalten = static_cast<int>(textB / std::max(1.0F, leer));
        for (int r = erste; r < reihenN && r < erste + proSeite; ++r) {
            const Reihe& q = reihen[static_cast<std::size_t>(r)];
            zeichen += umbruch ? q.e - q.a : std::min(q.e - q.a, spalten);
        }
        z.sichtbareZeichen = zeichen;
        z.textBreiteste = breiteste;
        z.textFlaeche = textB;
        z.textMass[0] = zh;
        z.textMass[1] = textX;
        z.textMass[2] = leer;
        z.textMass[3] = o.y;
    }
    // Minimap (logische Zeilen)
    if (miniB > 0) {
        const float mx = textX + textB;
        dl->AddRectFilled(ImVec2{mx, o.y}, ImVec2{mx + miniB, o.y + bereich.y}, IM_COL32(30, 30, 30, 255));
        const float sk = std::min(2.0F, bereich.y / std::max(1.0F, static_cast<float>(sichtbarN)));
        const float px = miniB / 70.0F;   // 70 Zeichen auf die Breite
        std::size_t fk = 0;
        for (int k = 0; k < sichtbarN; ++k) {
            const int zl = sichtbar[static_cast<std::size_t>(k)];
            const std::uint32_t za = p.zeilenAnfang(zl);
            const std::uint32_t ze = p.zeilenEnde(zl);
            while (fk < farben.size() && farben[fk].ende <= za) { ++fk; }
            const float y = o.y + static_cast<float>(k) * sk;
            for (std::size_t q = fk; q < farben.size() && farben[q].anfang < ze; ++q) {
                const float x0 = mx + 2 + static_cast<float>(farben[q].anfang - za) * px;
                const float x1 = mx + 2 + static_cast<float>(std::min(farben[q].ende, ze) - za) * px;
                if (x0 > mx + miniB) { break; }
                dl->AddRectFilled(ImVec2{x0, y}, ImVec2{std::min(x1, mx + miniB), y + std::max(1.0F, sk - 0.3F)},
                                  (farbeFuer(farben[q].klasse) & 0x00FFFFFFU) | 0xA0000000U);
            }
            if (const auto zs = zeilenSchwere.find(zl); zs != zeilenSchwere.end()) {
                dl->AddRectFilled(ImVec2{mx + miniB - 4, y}, ImVec2{mx + miniB, y + 2}, schwereFarbe(zs->second));
            }
        }
        for (const Treffer& t : suchT) {
            const int zl = p.pos(t.anfang).zeile;
            const auto it = std::lower_bound(sichtbar.begin(), sichtbar.end(), zl);
            const float y = o.y + static_cast<float>(it - sichtbar.begin()) * sk;
            dl->AddRectFilled(ImVec2{mx + miniB - 4, y}, ImVec2{mx + miniB, y + 2}, IM_COL32(255, 160, 0, 255));
        }
        if (reihenN > 0) {
            const auto it0 = std::lower_bound(sichtbar.begin(), sichtbar.end(), reihen[static_cast<std::size_t>(std::min(erste, reihenN - 1))].zl);
            const int letzteR = std::min(reihenN - 1, erste + proSeite);
            const auto it1 = std::lower_bound(sichtbar.begin(), sichtbar.end(), reihen[static_cast<std::size_t>(letzteR)].zl);
            const float y0 = o.y + static_cast<float>(it0 - sichtbar.begin()) * sk;
            const float y1 = o.y + static_cast<float>(it1 - sichtbar.begin() + 1) * sk;
            dl->AddRectFilled(ImVec2{mx, y0}, ImVec2{mx + miniB, y1}, IM_COL32(255, 255, 255, 25));
        }
    }
    dl->PopClipRect();
    // M2 Live-Diagnose: Tipp ueber der Stelle bzw. dem Randsymbol; Strg+. wendet die Korrektur an
    if (hover && !z.ziehtText && reihenN > 0 && maus.y >= o.y && maus.y < o.y + bereich.y) {
        const Reihe& r = reihen[static_cast<std::size_t>(reiheBeiMaus())];
        if (maus.x >= textX && maus.x < textX + textB) {
            const std::uint32_t off = offBeiMaus();
            const Diagnose* x = diagnoseBei(d, off);
            if (x != nullptr && x->ende > x->anfang && maus.x <= spalteX(r, static_cast<int>(std::min<std::uint32_t>(x->ende, p.zeilenEnde(r.zl)) - p.zeilenAnfang(r.zl))) + leer) {
                zeigeDiagnoseTipp(d, *x);
            }
        } else if (maus.x < textX - falteB && r.a == 0 && zeilenSchwere.count(r.zl) != 0) {
            for (const Diagnose& x : d.diagnosen) {
                if (x.zeile - 1 == r.zl) {
                    zeigeDiagnoseTipp(d, x);
                    break;
                }
            }
        }
    }
    if (z.codeFokus && ImGui::IsWindowFocused() && io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Period, false)) {
        if (const Diagnose* x = diagnoseBei(d, cur)) { (void)korrigiere(d, *x); }
    }
    // Vervollstaendigung
    if (z.vervollOffen && z.codeFokus && !z.vervoll.empty() && reihenN > 0) {
        const int rk = reiheVonOff(cur);
        const Reihe& r = reihen[static_cast<std::size_t>(rk)];
        const float y = o.y + static_cast<float>(rk - erste + 1) * zh;
        const float x = spalteX(r, p.pos(cur).spalte);
        ImGui::SetNextWindowPos(ImVec2{x, y});
        ImGui::SetNextWindowSize(ImVec2{fs * 16, std::min(10, static_cast<int>(z.vervoll.size())) * zh + 8});
        ImGui::Begin("##vervoll", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_Tooltip);
        for (std::size_t i = 0; i < z.vervoll.size(); ++i) {
            if (ImGui::Selectable(anzeige(z.vervoll[i]).c_str(), static_cast<int>(i) == z.vervollWahl)) {
                z.vervollWahl = static_cast<int>(i);
            }
            if (static_cast<int>(i) == z.vervollWahl) { ImGui::SetScrollHereY(); }
        }
        ImGui::End();
    }
    if (ImGui::GetTime() - z.codeZoomAb < 1.2 && z.codeZoomAb >= 0.0) {   // kurze Anzeige in Prozent
        char b[16];
        std::snprintf(b, sizeof(b), "%d %%", einst.menueTextZoom);
        const ImVec2 g = ImGui::CalcTextSize(b);
        const ImVec2 a{o.x + bereich.x - miniB - g.x - fs * 1.2F, o.y + fs * 0.4F};
        dl->AddRectFilled(a, ImVec2{a.x + g.x + fs * 0.8F, a.y + g.y + fs * 0.4F}, IM_COL32(0x26, 0x28, 0x2D, 240), 4.0F);
        dl->AddText(ImVec2{a.x + fs * 0.4F, a.y + fs * 0.2F}, IM_COL32(0xE6, 0xE7, 0xEA, 255), b);
    }
    ImGui::PopFont();
    ImGui::EndChild();
}

// M4 Punkt 3: Knopf der Leisten des Menue-Editors - Symbol mit Hinweis, wahlweise (Einstellung
// "Leisten mit Beschriftung") Symbol und Text; eingeschaltete Umschalter mit Akzentflaeche
bool leistenKnopf(const char* text, UiIcon icon, const char* hinweis, bool an = false, bool aktiv = true, bool umschalter = false) {
    // M6: passt der Knopf nicht mehr in die Zeile, beginnt eine neue (die Leiste waechst mit den Werkzeugen)
    {
        const ImGuiStyle& st = ImGui::GetStyle();
        const float h = ImGui::GetFrameHeight();
        const float breite = g_app->settings.menueLeisteText
                                 ? st.FramePadding.x * 2.0F + std::floor(h * 0.62F) + st.ItemInnerSpacing.x + ImGui::CalcTextSize(text, ImGui::FindRenderedTextEnd(text)).x
                                 : h;
        if (ImGui::GetCursorPosX() > st.WindowPadding.x + 1.0F && ImGui::GetCursorPosX() + breite > ImGui::GetContentRegionMax().x) { ImGui::NewLine(); }
    }
    if (!g_app->settings.menueLeisteText) { return iconKnopf(text, icon, hinweis, an, aktiv, umschalter); }
    if (an) { ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)); }
    const bool k = iconTextKnopf(text, icon, hinweis, aktiv);
    if (an) { ImGui::PopStyleColor(); }
    return k;
}

// --- M2 Punkt 5: Klaenge ---------------------------------------------------------------

// M4: alle Klaenge der Vorschau (Fokus, Klick, Musik) aus - laufende werden angehalten
void stoppeKlaenge() {
    Zustand& z = Z();
    for (const std::uint64_t id : z.klangStimmen) { g_app->audio.stoppe(id); }
    z.klangStimmen.clear();
}

// Was die Laufzeit an Klaengen ausgeloest hat (startLocalSound, startBackgroundTrack):
// ueber das Tongeraet des Programms - stumm im Selbsttest (g_app->stumm) und wenn der
// Ton-Schalter der Leiste aus ist (Einstellung menueTon); gezaehlt wird trotzdem.
void spieleKlaenge() {
    Zustand& z = Z();
    for (const std::string& k : z.lauf.klaenge) {
        ++z.klangEreignisse;
        z.letzterKlang = k;
        if (z.klangListe.size() < 64) { z.klangListe.push_back(k); }   // M3: Protokoll fuer den Vergleich mit dem Spiel
        if (!g_app->settings.menueTon || g_app->stumm) { continue; }
        const bool musik = k.compare(0, 6, "musik:") == 0;
        const std::vector<std::int16_t>* daten = klangFuerWiedergabe(musik ? k.substr(6) : k);
        if (daten == nullptr || daten->empty()) { continue; }
        if (!g_app->audio.ready() && !g_app->audio.open(44100, 2)) { continue; }
        // mit Nummer, damit der Schalter auch laufende Klaenge (Musik) anhalten kann
        sound::Mixer::Start s;
        s.schleife = musik;   // S_StartBackgroundTrack ohne Schleifenteil spielt die Musik weiter
        const std::uint64_t id = g_app->audio.starte(sound::Mixer::Daten(sound::Mixer::Daten{}, daten), s);
        if (id != 0) { z.klangStimmen.push_back(id); }
    }
    z.lauf.klaenge.clear();
    z.klangStimmen.erase(std::remove_if(z.klangStimmen.begin(), z.klangStimmen.end(), [](std::uint64_t id) { return !g_app->audio.laeuft(id); }),
                         z.klangStimmen.end());
}

// --- M2 Punkt 2 / M4 Punkt 5: Video -> RoQ ------------------------------------------------

constexpr long long kVideoMaxMs = 10LL * 60 * 1000;   // laengstes Video (10 min)

std::string videoFehlerText(const gui::VideoInfo& i) {
    char buf[600];
    switch (i.fehler) {
        case gui::VideoFehler::Keiner: return {};
        case gui::VideoFehler::KeinMediaFoundation: return tr(Str::MeVideoKeinMf);
        case gui::VideoFehler::NichtLesbar: return tr(Str::MeVideoNichtLesbar);
        case gui::VideoFehler::KeinVideo: return tr(Str::MeVideoKeinVideo);
        case gui::VideoFehler::KeineBilder: return tr(Str::MeVideoKeineBilder);
        case gui::VideoFehler::Abbruch: return tr(Str::MeVideoAbbruch);
        case gui::VideoFehler::CodecFehlt:
            switch (codecGruppe(i.codec)) {
                case CodecGruppe::Hevc: return tr(Str::MeVideoCodecHevc);
                case CodecGruppe::Av1: return tr(Str::MeVideoCodecAv1);
                case CodecGruppe::Vp8:
                case CodecGruppe::Vp9: return tr(Str::MeVideoCodecVp);
                case CodecGruppe::DivxXvid: return tr(Str::MeVideoCodecDivx);
                case CodecGruppe::Unbekannt: break;
            }
            std::snprintf(buf, sizeof(buf), tr(Str::MeVideoCodecAnders), fourccText(i.codec).c_str());
            return buf;
    }
    return {};
}

bool istRoq(const std::string& d) {
    return d.size() >= 8 && static_cast<unsigned char>(d[0]) == (kRoqKennung & 0xff) && static_cast<unsigned char>(d[1]) == (kRoqKennung >> 8);
}

// Die ganze Datei, wenn sie ein RoQ oder GIF ist (beides wird im Speicher gelesen); sonst ""
// - ein Video liest Media Foundation selbst, es wird nicht ganz in den Speicher geholt.
std::string roqOderGifLesen(const std::string& platte) {
    std::ifstream in(utf8Pfad(platte), std::ios::binary);
    char kopf[8] = {};
    in.read(kopf, sizeof(kopf));
    const std::string k(kopf, static_cast<std::size_t>(in.gcount()));
    if (!istRoq(k) && k.compare(0, 3, "GIF") != 0) { return {}; }
    in.clear();
    in.seekg(0);
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

// Groesse, Bildrate eines RoQ (ROQ_QUAD_INFO und Kopf, wie RoQ_init/readQuadInfo)
bool roqKopf(const std::string& d, int& b, int& h, int& fps) {
    RoqSpieler s;
    if (!s.start(d, 0, false)) { return false; }
    (void)s.lauf(0);
    b = s.breite();
    h = s.hoehe();
    fps = s.fps();
    return b > 0 && h > 0;
}

// Eine Video-, GIF- oder RoQ-Datei als RoQ (laeuft auch im Hintergrund, fasst den Zustand nicht an).
// Ein RoQ wird unveraendert uebernommen.
VideoErgebnis videoKodieren(const std::string& platte, const VideoEinst& e, const std::function<bool(int, int)>& fortschritt) {
    VideoErgebnis r;
    RoqOptionen o;
    o.guete = e.guete;
    o.fps = e.fps;
    const std::string roh = roqOderGifLesen(platte);
    if (istRoq(roh)) {
        r.roqDirekt = true;
        if (!roqKopf(roh, r.breite, r.hoehe, r.fps)) {
            r.fehler = tr(Str::MeVideoKeineBilder);
            return r;
        }
        r.roq = roh;
        return r;
    }
    Bildfolge f;
    if (gifLesen(roh, f)) {
        r.roq = kodiereRoq(f, o, e.ziel, nullptr, fortschritt);
        if (r.roq.empty()) { r.fehler = tr(Str::MeVideoAbbruch); }
        return r;
    }
    gui::VideoInfo info;
    if (!gui::videoInfo(platte, info)) {
        r.fehler = videoFehlerText(info);
        return r;
    }
    const long long takte = RoqImport::takteFuer(std::min(info.dauerMs > 0 ? info.dauerMs : kVideoMaxMs, kVideoMaxMs), e.fps);
    RoqTon ton;
    if (e.ton) {
        std::vector<std::int16_t> pcm;
        int rate = 0, kanaele = 0;
        if (gui::leseVideoTon(platte, pcm, rate, kanaele, kVideoMaxMs)) {
            ton = tonAuf22050(pcm, rate, kanaele, e.stereo ? 2 : 1);
            // auf die Laenge des Videos (Takte * 22050 / fps)
            const std::size_t max = static_cast<std::size_t>(takte * kRoqTonRate / e.fps) * static_cast<std::size_t>(ton.kanaele);
            if (ton.pcm.size() > max) { ton.pcm.resize(max); }
        } else {
            r.ohneTon = true;
        }
    }
    RoqImport imp;
    if (!imp.beginne(e.ziel, o, ton.kanaele > 0 ? &ton : nullptr)) {
        r.fehler = tr(Str::MeVideoGroesseUngueltig);
        return r;
    }
    long long dauer = 0;
    gui::VideoInfo li;
    const bool ok = gui::leseVideoBilder(
        platte,
        [&](const RoqBild& b, long long ms) {
            imp.bild(b, ms);
            return !fortschritt || fortschritt(imp.takte(), static_cast<int>(takte));
        },
        kVideoMaxMs, dauer, li);
    if (!ok) {
        r.fehler = videoFehlerText(li);
        return r;
    }
    r.roq = imp.fertig(dauer);
    return r;
}

// Geschaetzte Dateigroesse: die ersten Takte kodieren und hochrechnen (Ton exakt: ein
// Byte je Probe und Kanal plus 8 Bytes je Stueck).
long long videoGroesseSchaetzen(const std::string& platte, const VideoEinst& e, const gui::VideoInfo& info) {
    const std::string roh = roqOderGifLesen(platte);
    if (istRoq(roh)) { return static_cast<long long>(roh.size()); }
    RoqOptionen o;
    o.guete = e.guete;
    o.fps = e.fps;
    Bildfolge f;
    if (gifLesen(roh, f)) { return static_cast<long long>(kodiereRoq(f, o, e.ziel).size()); }
    const long long takte = RoqImport::takteFuer(std::min(info.dauerMs > 0 ? info.dauerMs : kVideoMaxMs, kVideoMaxMs), e.fps);
    constexpr int kProbe = 6;
    RoqImport imp;
    if (!imp.beginne(e.ziel, o, nullptr)) { return -1; }
    std::size_t nachErstem = 0;
    long long dauer = 0;
    gui::VideoInfo li;
    (void)gui::leseVideoBilder(
        platte,
        [&](const RoqBild& b, long long ms) {
            imp.bild(b, ms);
            if (imp.takte() >= 1 && nachErstem == 0) { nachErstem = imp.bytes(); }
            return imp.takte() < kProbe;
        },
        kVideoMaxMs, dauer, li);
    if (imp.takte() == 0) { return -1; }
    const std::size_t jetzt = imp.bytes();
    const double jeBild = imp.takte() > 1 ? static_cast<double>(jetzt - nachErstem) / (imp.takte() - 1) : static_cast<double>(jetzt);
    long long s = static_cast<long long>(nachErstem + jeBild * static_cast<double>(takte - 1)) + 8;
    if (e.ton && info.ton) {
        const long long proben = takte * kRoqTonRate / e.fps;
        s += proben * (e.stereo ? 2 : 1) + 8 * (takte + 1);
    }
    return s;
}

std::string videoNameAus(const std::string& platte) {
    std::string n = fs::path(utf8Pfad(platte)).stem().string();
    for (char& c : n) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') { c = '_'; }
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));   // Shader- und Dateinamen ohne Gross/Klein
    }
    return n.empty() ? std::string("video") : n;
}

// shaders/<paket>.shader: Spiel-Shader liegen in shaders/ (ScanAndLoadShaderFiles)
std::string paketShaderDatei(const Dokument& d) {
    std::string n = d.textPaket.empty() ? std::string("mymenu") : d.textPaket;
    for (char& c : n) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return "shaders/" + n + ".shader";
}

// Der Shader, den ein Menue fuer ein Video braucht (tr_shader.cpp ParseStage: videoMap
// startet CIN_PlayCinematic(..., CIN_loop | CIN_silent | CIN_shader)).
std::string videoShaderText(const std::string& shader, const std::string& video) {
    return shader + "\n{\n\tnopicmip\n\tnomipmaps\n\t{\n\t\tvideoMap " + video + "\n\t\trgbGen identity\n\t}\n}\n";
}

void bildInRahmen(int item, const std::string& name);   // app_menue_bildwahl.inc

// Ins Paket: video/<name>.roq, der Shader menu/video/<name> in shaders/<paket>.shader,
// dann je nach Verwendung ans Menue.
void videoAblegen(const std::string& name, const std::string& roq, VideoVerwendung verwendung) {
    Dokument* d = doc();
    if (d == nullptr || roq.empty()) { return; }
    Zustand& z = Z();
    const std::string video = "video/" + name + ".roq";
    const std::string shader = "menu/video/" + name;
    const std::string datei = paketShaderDatei(*d);
    std::string text;
    if (const auto f = d->bilder.find(datei); f != d->bilder.end()) { text = f->second; }
    ShaderMap da;
    parseShaderScript(text, da);
    if (da.count(shader) == 0) { text += (text.empty() || text.back() == '\n' ? "" : "\n") + videoShaderText(shader, video); }
    z.kinos.clear();
    if (verwendung == VideoVerwendung::NurPaket) {
        d->bilder[video] = roq;   // ohne Schritt im Text: nichts zum Zuruecknehmen
        d->bilder[datei] = text;
    } else {
        paketSetzen(*d, {{video, roq}, {datei, text}});
    }
    shaderLaden();
    parseShaderScript(text, z.shader);
    z.texturen.erase(shader);
    const int mi = menuDerSeite(*d);
    if (verwendung == VideoVerwendung::Hintergrund && d->auswahl.size() == 1) {
        bildInRahmen(d->auswahl[0], shader);
    } else if (verwendung == VideoVerwendung::NeuesElement && mi >= 0) {
        const std::string vorher = d->puffer.text();
        fuegeEin(-1);
        const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
        const int it = static_cast<int>(m.items.size()) - 1;
        if (it >= 0) {
            eigenschaft(it, "name", {freierName(m, "video_" + name)});
            eigenschaft(it, "rect", {zahlText(-m.window.rect.x) + " " + zahlText(-m.window.rect.y) + " 640 480"});
            eigenschaft(it, "background", {inAnf(shader)});
            (void)setzeAnStelle(*d, mi, it, 0);   // ganz nach hinten: unter allen anderen Items
        }
        (void)zuEinemSchritt(vorher);
    } else if (verwendung == VideoVerwendung::Cinematic && d->auswahl.size() == 1) {
        eigenschaft(d->auswahl[0], "style", {konstante(d->datei, "WINDOW_STYLE_CINEMATIC", WINDOW_STYLE_CINEMATIC)});
        eigenschaft(d->auswahl[0], "cinematic", {inAnf(name)});
    }
    char buf[512];
    std::snprintf(buf, sizeof(buf), tr(Str::MeVideoFertigShader), video.c_str(), shader.c_str(), datei.c_str());
    z.meldung = buf;
}

std::string zeitText(long long ms) {
    char b[32];
    std::snprintf(b, sizeof(b), "%lld:%04.1f", ms / 60000, static_cast<double>(ms % 60000) / 1000.0);
    return b;
}

std::string schluesselFuer(const std::string& quelle, const VideoEinst& e) {
    return quelle + "|" + std::to_string(e.ziel.breite) + "x" + std::to_string(e.ziel.hoehe) + "|" + std::to_string(static_cast<int>(e.ziel.einpassen)) + "|" +
           std::to_string(e.guete) + "|" + std::to_string(e.fps) + "|" + (e.ton ? "t" : "") + (e.stereo ? "s" : "");
}

void zeichneVideoFenster() {
    Zustand& z = Z();
    // fertig gewordenes Ergebnis aus dem Hintergrund uebernehmen
    if (z.videoDa) {
        std::lock_guard<std::mutex> l(z.videoMutex);
        if (z.videoFaden.joinable()) { z.videoFaden.join(); }
        if (!z.videoErgebnis.roq.empty()) {
            videoAblegen(z.videoName, z.videoErgebnis.roq, z.videoVerwendung);
            if (z.videoErgebnis.ohneTon) { z.meldung += std::string(" ") + tr(Str::MeVideoOhneTon); }
        } else if (!z.videoErgebnis.fehler.empty()) {
            char buf[900];
            std::snprintf(buf, sizeof(buf), tr(Str::MeVideoFehler), z.videoErgebnis.fehler.c_str());
            z.meldung = buf;
        }
        z.videoErgebnis = VideoErgebnis{};
        z.videoDa = false;
        z.videoOffen = false;
    }
    if (!z.videoOffen) { return; }
    // Kopfdaten einmal je Quelle (Media Foundation, ohne zu dekodieren)
    if (z.videoInfoFuer != z.videoQuelle) {
        z.videoInfoFuer = z.videoQuelle;
        z.videoInfo = gui::VideoInfo{};
        const std::string roh = roqOderGifLesen(z.videoQuelle);
        z.videoArt = istRoq(roh) ? 2 : (roh.compare(0, 3, "GIF") == 0 ? 1 : 0);
        if (z.videoArt == 2) {
            (void)roqKopf(roh, z.videoInfo.breite, z.videoInfo.hoehe, z.videoRoqFps);
        } else if (z.videoArt == 0) {
            (void)gui::videoInfo(z.videoQuelle, z.videoInfo);
        }
        if (Dokument* d = doc()) { z.videoVerwendung = d->auswahl.size() == 1 ? VideoVerwendung::Hintergrund : VideoVerwendung::NeuesElement; }
    }
    ImGui::SetNextWindowSize(ImVec2{ImGui::GetFontSize() * 30, 0}, ImGuiCond_Appearing);
    bool offen = true;
    if (ImGui::Begin(tr(Str::MeVideoTitel), &offen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextWrapped("%s", anzeige(z.videoQuelle).c_str());
        const gui::VideoInfo& vi = z.videoInfo;
        const std::string fehler = videoFehlerText(vi);
        char buf[512];
        if (!fehler.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0F, 0.45F, 0.35F, 1.0F});
            ImGui::TextWrapped("%s", fehler.c_str());
            ImGui::PopStyleColor();
        } else if (z.videoArt == 0) {
            std::snprintf(buf, sizeof(buf), tr(Str::MeVideoQuelle), vi.breite, vi.hoehe, vi.fps, zeitText(vi.dauerMs).c_str());
            ImGui::TextUnformatted(buf);
            if (vi.ton) {
                std::snprintf(buf, sizeof(buf), trN(Str::MeVideoTonQuelle, vi.tonKanaele), vi.tonKanaele, vi.tonRate);
                ImGui::TextUnformatted(buf);
            } else {
                ImGui::TextDisabled("%s", tr(Str::MeVideoOhneTon));
            }
        }
        const bool laeuft = z.videoLaeuft.load();
        VideoEinst& e = z.videoEinst;
        ImGui::BeginDisabled(laeuft || !fehler.empty());
        if (z.videoArt == 2) {
            std::snprintf(buf, sizeof(buf), tr(Str::MeVideoRoqDirekt), vi.breite, vi.hoehe, z.videoRoqFps);
            ImGui::TextWrapped("%s", buf);
            if (roqGroessePruefen(vi.breite, vi.hoehe) != RoqGroesse::Ok) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0F, 0.75F, 0.3F, 1.0F});
                ImGui::TextWrapped("%s", tr(Str::MeVideoGroesseTip));
                ImGui::PopStyleColor();
            }
        } else {
            // Groesse: Zweierpotenzen (RE_StretchRaw), hoechstens 512 * 512 Punkte (cin.linbuf)
            static const int seiten[] = {16, 32, 64, 128, 256, 512, 1024};
            static const char* const seitenText[] = {"16", "32", "64", "128", "256", "512", "1024"};
            auto seitenWahl = [&](const char* id, int& wert) {
                int i = 0;
                while (i < 6 && seiten[i] < wert) { ++i; }
                ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5);
                if (ImGui::Combo(id, &i, seitenText, 7)) { wert = seiten[i]; }
            };
            ImGui::TextUnformatted(tr(Str::MeVideoZielGroesse));
            ImGui::SameLine();
            seitenWahl("##vb", e.ziel.breite);
            ImGui::SameLine();
            ImGui::TextUnformatted("x");
            ImGui::SameLine();
            seitenWahl("##vh", e.ziel.hoehe);
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MeVideoGroesseTip)); }
            if (roqGroessePruefen(e.ziel.breite, e.ziel.hoehe) != RoqGroesse::Ok) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0F, 0.45F, 0.35F, 1.0F});
                ImGui::TextWrapped("%s", tr(Str::MeVideoGroesseUngueltig));
                ImGui::PopStyleColor();
            }
            int ep = static_cast<int>(e.ziel.einpassen);
            const char* epText[] = {tr(Str::MeVideoStrecken), tr(Str::MeVideoBalken), tr(Str::MeVideoZuschneiden)};
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12);
            if (ImGui::Combo(tr(Str::MeVideoEinpassen), &ep, epText, 3)) { e.ziel.einpassen = static_cast<Einpassen>(ep); }
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MeVideoEinpassenTip)); }
            ImGui::SliderInt(tr(Str::MeVideoFps), &e.fps, 5, 60);
            ImGui::SliderInt(tr(Str::MeVideoGuete), &e.guete, 1, 100);
            if (z.videoArt == 0) {
                ImGui::BeginDisabled(!vi.ton);
                ImGui::Checkbox(tr(Str::MeVideoTon), &e.ton);
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 26);
                    ImGui::TextUnformatted(tr(Str::MeVideoTonTip));
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                ImGui::BeginDisabled(!e.ton);
                ImGui::Checkbox(tr(Str::MeVideoStereo), &e.stereo);
                ImGui::EndDisabled();
                ImGui::EndDisabled();
            }
        }
        // rc608 Teil 3a: Das Video kommt vom Rahmen (behaelt Lage und Groesse) oder ueber den Knopf der Leiste nur ins Paket; Menue-Videos sind stumm
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextDisabled("%s", tr(z.videoVerwendung == VideoVerwendung::Hintergrund ? Str::MlVideoInRahmen : Str::MeVideoNurPaketTip));
        ImGui::TextDisabled("%s", tr(Str::MlVideoStumm));
        ImGui::PopTextWrapPos();
        // Laenge und Groesse vorher (die ersten Takte kodiert und hochgerechnet)
        if (fehler.empty() && z.videoArt != 2) {
            const std::string schluessel = schluesselFuer(z.videoQuelle, e);
            if (!z.schaetzLaeuft.load() && z.schaetzFuer != schluessel && !laeuft) {
                if (z.schaetzFaden.joinable()) { z.schaetzFaden.join(); }
                z.schaetzLaeuft = true;
                z.schaetzFuer = schluessel;
                z.schaetzBytes = -1;
                const std::string q = z.videoQuelle;
                const gui::VideoInfo info = vi;
                z.schaetzFaden = std::thread([q, e, info] {
                    const long long b = videoGroesseSchaetzen(q, e, info);
                    Z().schaetzBytes = b;
                    Z().schaetzLaeuft = false;
                });
            }
            const long long dauer = z.videoArt == 0 ? vi.dauerMs : 0;
            const long long bilder = z.videoArt == 0 ? RoqImport::takteFuer(dauer, e.fps) : 0;
            if (z.schaetzLaeuft.load() || z.schaetzBytes.load() < 0) {
                ImGui::TextDisabled("%s", tr(Str::MeVideoSchaetze));
            } else if (z.videoArt == 0) {
                std::snprintf(buf, sizeof(buf), trN(Str::MeVideoErgebnis, static_cast<int>(bilder)), static_cast<int>(bilder), zeitText(dauer).c_str(),
                              static_cast<double>(z.schaetzBytes.load()) / (1024.0 * 1024.0));
                ImGui::TextUnformatted(buf);
            } else {
                std::snprintf(buf, sizeof(buf), tr(Str::MeVideoErgebnisGif), static_cast<double>(z.schaetzBytes.load()) / (1024.0 * 1024.0));
                ImGui::TextUnformatted(buf);
            }
        }
        const bool groesseOk = z.videoArt == 2 || roqGroessePruefen(e.ziel.breite, e.ziel.hoehe) == RoqGroesse::Ok;
        ImGui::BeginDisabled(!groesseOk);
        if (ImGui::Button(tr(z.videoArt == 2 ? Str::MeVideoUebernehmen : Str::MeVideoStart))) {
            if (z.videoFaden.joinable()) { z.videoFaden.join(); }
            z.videoName = videoNameAus(z.videoQuelle);
            z.videoLaeuft = true;
            z.videoAbbruch = false;
            z.videoFertig = 0;
            z.videoGesamt = 0;
            const std::string q = z.videoQuelle;
            const VideoEinst einst = e;
            z.videoFaden = std::thread([q, einst] {
                Zustand& zz = Z();
                VideoErgebnis r = videoKodieren(q, einst, [](int a, int b) {
                    Z().videoFertig = a;
                    Z().videoGesamt = b;
                    return !Z().videoAbbruch.load();
                });
                std::lock_guard<std::mutex> l(zz.videoMutex);
                zz.videoErgebnis = std::move(r);
                zz.videoLaeuft = false;
                zz.videoDa = true;
            });
        }
        ImGui::EndDisabled();
        ImGui::EndDisabled();
        if (laeuft) {
            std::snprintf(buf, sizeof(buf), tr(Str::MeVideoLaeuft), z.videoFertig.load(), z.videoGesamt.load());
            const float anteil = z.videoGesamt > 0 ? static_cast<float>(z.videoFertig) / static_cast<float>(z.videoGesamt) : 0.0F;
            ImGui::ProgressBar(std::min(1.0F, anteil), ImVec2{-1, 0}, buf);
        }
    }
    ImGui::End();
    if (!offen) {
        z.videoAbbruch = true;
        z.videoOffen = z.videoLaeuft.load();   // bis der Faden aufhoert, offen lassen
    }
}

// --- Seitenleisten ----------------------------------------------------------------


// M2 modernes Aussehen: ein Bauteil als Kachel mit kleinem Bild der Art und Namen
// rc608 Punkt 11: Ein Klick tut nichts, Doppelklick setzt ein (true), Ziehen legt auf der Flaeche ab
ImVec2 widgetGroesse(int typ) {
    if (typ == -1) { return ImVec2{200, 150}; }
    if (typ == -3) { return ImVec2{256, 192}; }
    if (typ == ITEM_TYPE_BUTTON) { return ImVec2{160, 36}; }
    if (typ != ITEM_TYPE_MODEL) { return ImVec2{200, 40}; }
    return Z().spiel == Spiel::JK2 ? ImVec2{250, 250} : ImVec2{200, 225};   // ui/jk2mp/credits.menu, ui/jamp/ingame_player2.menu
}

// --- M2 Punkt 4: Bauteil-Bibliothek --------------------------------------------------

std::string vorlagenOrdner() { return u8(fs::path(utf8Pfad(platform::settingsDirectory())) / "menue_vorlagen"); }

const std::vector<Vorlage>& vorlagen() {
    Zustand& z = Z();
    if (!z.vorlagenGeladen) {
        Dokument* d = doc();
        z.vorlagen = standardVorlagen(d != nullptr ? d->datei.dialekt : Dialekt::SP);
        for (Vorlage& v : ladeEigeneVorlagen(vorlagenOrdner())) { z.vorlagen.push_back(std::move(v)); }
        z.vorlagenGeladen = true;
    }
    return z.vorlagen;
}

// Eine Vorlage einsetzen; mit Ziel (virtueller Bildschirm) an diese Stelle schieben.
// rc608 Punkt 11: Was man ablegt, ist eine Bibliothek-Nummer (>= 0) oder ein Widget (ablageWidget); ein Weg fuer beide.
std::string ablageText(int n) {
    Dokument* d = doc();
    if (d == nullptr) { return {}; }
    if (n >= 0) {
        const std::vector<Vorlage>& v = vorlagen();
        if (static_cast<std::size_t>(n) >= v.size()) { return {}; }
        const Vorlage& vo = v[static_cast<std::size_t>(n)];
        return vo.eigen ? vo.text : vorlageMitSpielSchrift(vo.text, d->datei.dialekt, d->datei.spiel);
    }
    const int typ = ablageTyp(n);
    const int mi = menuDerSeite(*d);
    const Menu& m = d->datei.menus[static_cast<std::size_t>(std::max(0, mi))];
    const std::string basis = typ == -3 ? "video" : typ < 0 ? "image" : std::string(typName(typ)).substr(10);
    std::string name = basis;
    for (char& c : name) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    const ImVec2 gr = widgetGroesse(typ);
    const Rechteck r{static_cast<float>(220 - m.window.rect.x), static_cast<float>(200 - m.window.rect.y), gr.x, gr.y};
    return itemVorlage(typ, d->datei.dialekt, name, r, "\t\t", d->puffer.zeilenUmbruch(), d->datei.spiel);
}

TextMass textMassVon(Dokument& d, const Item& it);   // app_menue_leiste.inc

// Wie gross das Abgelegte wird (Umriss aller rect-Zeilen), fuer den Rahmen beim Ziehen
ImVec2 ablageGroesse(int n) {
    const std::string t = ablageText(n);
    float x0 = 1e9F, y0 = 1e9F, x1 = -1e9F, y1 = -1e9F;
    std::size_t p = 0;
    while ((p = t.find("rect", p)) != std::string::npos) {
        float a = 0, b = 0, c = 0, e = 0;
        if (std::sscanf(t.c_str() + p + 4, " %f %f %f %f", &a, &b, &c, &e) == 4) {
            x0 = std::min(x0, a);
            y0 = std::min(y0, b);
            x1 = std::max(x1, a + c);
            y1 = std::max(y1, b + e);
        }
        p += 4;
    }
    return x1 > x0 ? ImVec2{x1 - x0, y1 - y0} : ImVec2{200, 40};
}

void setzeAblageEin(int n, bool mitZiel, float zx, float zy) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    const std::string text = ablageText(n);
    if (text.empty()) { return; }
    const std::string vorher = d->puffer.text();
    fuegeItemsEin(text);
    neuParsen(*d);
    const int mi = menuDerSeite(*d);
    if (mi < 0 || d->auswahl.empty()) { return; }
    const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
    float lx = 1e9F, ly = 1e9F;
    for (const int i : d->auswahl) {
        if (i >= 0 && static_cast<std::size_t>(i) < m.items.size()) {
            lx = std::min(lx, m.items[static_cast<std::size_t>(i)].window.rect.x);
            ly = std::min(ly, m.items[static_cast<std::size_t>(i)].window.rect.y);
        }
    }
    if (mitZiel) {   // linke obere Ecke der eingesetzten Items an die Stelle des Loslassens
        if (lx < 1e8F) { verschiebeAuswahl(std::round(zx - lx), std::round(zy - ly)); }
    } else if (lx < 1e8F) {
        // Doppelklick: nicht uebereinanderstapeln, Versatz wie beim Einfuegen ueblich
        float versatz = 0;
        const auto belegt = [&](float x, float y) {
            for (std::size_t i = 0; i < m.items.size(); ++i) {
                if (std::find(d->auswahl.begin(), d->auswahl.end(), static_cast<int>(i)) != d->auswahl.end()) { continue; }
                if (std::fabs(m.items[i].window.rect.x - x) < 1.0F && std::fabs(m.items[i].window.rect.y - y) < 1.0F) { return true; }
            }
            return false;
        };
        while (belegt(lx + versatz, ly + versatz) && versatz < 400.0F) { versatz += 20.0F; }
        if (versatz > 0.0F) { verschiebeAuswahl(versatz, versatz); }
    }
    // Detailbereich: onOpen oeffnet Reiter 0 / Unterreiter 0
    const std::vector<Vorlage>& v = vorlagen();
    if (n >= 0 && static_cast<std::size_t>(n) < v.size() && v[static_cast<std::size_t>(n)].id == "detail") {
        neuParsen(*d);
        const Menu& mm = d->datei.menus[static_cast<std::size_t>(mi)];
        const DetailOptionen o = beispielDetail();
        if (mm.onOpen.find(o.cvarSel) == std::string::npos) {
            std::vector<Ersetzung> e;
            if (setzeEigenschaft(d->datei, mi, -1, "onOpen", {"{ " + mm.onOpen + detailOnOpen(o) + " }"}, e)) { aendere(std::move(e)); }
        }
    }
    // rc608 Teil 3a: Der Knopf bekommt seinen Text senkrecht in die Mitte, ueber dieselbe Regel wie die Ausrichtung in Properties
    if (n < 0 && ablageTyp(n) == ITEM_TYPE_BUTTON && d->auswahl.size() == 1) {
        neuParsen(*d);
        const Item& it = d->datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(d->auswahl[0])];
        const TextMass tm = textMassVon(*d, it);
        eigenschaft(d->auswahl[0], "textaligny", {zahlText(textYFuer(1, it.window.rectClient.h, tm.oben, tm.unten))});
    }
    (void)zuEinemSchritt(vorher);   // rc608: Einfuegen, an die Stelle ruecken und onOpen sind eine Handlung
    neuParsen(*d);
    g_app->settings.menueTeileHinweisWeg = true;
}

void nameSetzen(Dokument& d, int menu, int i, const std::string& utf8);   // app_menue_leiste.inc
const char* artName(const Dokument& d, const Item& it);

// M4: Symbolknopf der Ebenenliste (Auge, Schloss) in Zeilenhoehe - echte Symbole statt
// "o"/"-" und "L"/"."; aus = gedaempft, Hinweis beim Darueberfahren
bool ebenenSymbol(const char* id, UiIcon icon, bool an, const char* hinweis) {
    const float h = ImGui::GetTextLineHeight() + 2.0F;
    const bool geklickt = ImGui::InvisibleButton(id, ImVec2{h, h});
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    if (ImGui::IsItemHovered()) {
        dl->AddRectFilled(a, b, ImGui::GetColorU32(ImGuiCol_ButtonHovered), 2.0F);
        ImGui::SetTooltip("%s", hinweis);
    }
    ImVec4 f = uiIconFarbe(icon);
    if (!an) { f.w *= 0.5F; }
    const float g = std::floor(h * 0.86F);
    uiIcon(dl, icon, ImVec2{std::floor(a.x + (h - g) * 0.5F), std::floor(a.y + (h - g) * 0.5F)}, g, ImGui::GetColorU32(f));
    return geklickt;
}

// Eine Zeile der Ebenenliste: Auge (nur Vorschau), Schloss, Name (Doppelklick =
// umbenennen), Ziehen auf eine andere Zeile = an deren Stelle in der Reihenfolge.
void zeichneEbene(Dokument& d, int menu, int i) {
    if (menu < 0 || static_cast<std::size_t>(menu) >= d.datei.menus.size()) { return; }
    const Menu& me = d.datei.menus[static_cast<std::size_t>(menu)];
    if (i < 0 || static_cast<std::size_t>(i) >= me.items.size()) { return; }   // rc609 ME1: eine Aenderung mitten in der Liste
    const Item& it = me.items[static_cast<std::size_t>(i)];
    ImGui::PushID(i);
    const bool aufSeite = menuDerSeite(d) == menu;
    if (aufSeite) { Z().hierarchieReihe.push_back(i); }
    const float symbole = (ImGui::GetTextLineHeight() + 2.0F + ImGui::GetStyle().ItemSpacing.x) * 2.0F;
    const float zeilenBreite = std::max(ImGui::GetFontSize() * 3.0F, ImGui::GetContentRegionAvail().x - symbole);
    if (d.umbenennen == i && d.umbenennenMenu == menu) {
        // rc608: nimmt den Fokus nur am Anfang. Es endet mit Enter, Esc (ohne Aenderung) oder mit dem Klick
        // daneben, also auch mit jedem Auswahl- oder Reiterwechsel; bis dahin Getipptes zaehlt.
        d.umbenennenBild = ImGui::GetFrameCount();
        ImGui::SetNextItemWidth(zeilenBreite);
        if (d.umbenennenNeu) {
            ImGui::SetKeyboardFocusHere();
            d.umbenennenNeu = false;
        }
        const std::string alt = it.window.name;
        Dokument* dp = &d;
        textFeld("##neu", &d.neuerName, [dp, menu, i, alt](const std::string& t) {
            if (t.empty() || utf8ZuLatin1(t.c_str()) == alt) { return; }
            nameSetzen(*dp, menu, i, t);
        });
        if (ImGui::IsItemDeactivated()) { d.umbenennen = -1; }
    } else {
        const bool sel = aufSeite && std::find(d.auswahl.begin(), d.auswahl.end(), i) != d.auswahl.end();
        char l[400];
        std::snprintf(l, sizeof(l), "%s  %s", it.window.name.empty() ? "?" : anzeige(it.window.name).c_str(), artName(d, it));
        // rc608 Punkt 5: ein Button hat seinen Text als Eigenschaft, darum kein Kindelement: er steht mit in der Zeile
        if (!it.text.empty()) {
            std::string gezeigt = textAusDatei(Z().sprache, vorschauTexte(d).imSpiel(it.text));
            for (char& c : gezeigt) {
                if (c == '\n' || c == '\r' || c == '\t') { c = ' '; }
            }
            if (gezeigt.size() > 28) {
                std::size_t n = 28;
                while (n > 0 && (static_cast<unsigned char>(gezeigt[n]) & 0xC0U) == 0x80U) { --n; }
                gezeigt = gezeigt.substr(0, n) + "...";
            }
            if (!gezeigt.empty()) {
                const std::size_t lang = std::strlen(l);
                std::snprintf(l + lang, sizeof(l) - lang, "  \"%s\"", gezeigt.c_str());
            }
        }
        Z().hierarchieZeilen.push_back(it.window.name);
        const bool angeklickt = ImGui::Selectable(l, sel, ImGuiSelectableFlags_AllowDoubleClick, ImVec2{zeilenBreite, 0.0F});
        if (sel && Z().hierarchieRollen) { ImGui::SetScrollHereY(0.3F); }
        if (angeklickt) {
            if (!aufSeite) { zeigeMenue(d, menu); }
            // Strg nimmt hinzu oder heraus, Umschalt waehlt den Bereich ab der zuletzt angeklickten Zeile
            const std::vector<int>& reihe = Z().hierarchieReiheAlt;
            const auto von = std::find(reihe.begin(), reihe.end(), d.hierarchieAnker);
            const auto bis = std::find(reihe.begin(), reihe.end(), i);
            if (ImGui::GetIO().KeyShift && aufSeite && von != reihe.end() && bis != reihe.end()) {
                d.auswahl.assign(std::min(von, bis), std::max(von, bis) + 1);
            } else {
                waehle(i, ImGui::GetIO().KeyCtrl);
                d.hierarchieAnker = i;
            }
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                d.umbenennen = i;
                d.umbenennenMenu = menu;
                d.umbenennenNeu = true;
                d.umbenennenBild = ImGui::GetFrameCount();
                d.neuerName = anzeige(it.window.name);
            }
        }
        if (aufSeite && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            if (std::find(d.auswahl.begin(), d.auswahl.end(), i) == d.auswahl.end()) { waehle(i, false); }
            ImGui::OpenPopup("##zeilekontext");
        }
        if (ImGui::BeginPopup("##zeilekontext")) {
            kontextEintraege(d, menu, me, nullptr);
            ImGui::EndPopup();
        }
        if (aufSeite && ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("ME_EBENE", &i, sizeof(i));
            ImGui::TextUnformatted(l);
            ImGui::EndDragDropSource();
        }
        if (aufSeite && ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* pl = ImGui::AcceptDragDropPayload("ME_EBENE")) {
                int von = 0;
                std::memcpy(&von, pl->Data, sizeof(von));
                (void)setzeAnStelle(d, menu, von, i);
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImGui::SameLine();
    const bool verborgen = d.verborgen.count({menu, i}) != 0;
    if (ebenenSymbol("##auge", verborgen ? UiIcon::Versteckt : UiIcon::Sichtbar, !verborgen, tr(Str::MeEbeneAuge))) {
        if (verborgen) { d.verborgen.erase({menu, i}); } else { d.verborgen.insert({menu, i}); }
    }
    ImGui::SameLine();
    const bool gesperrt = aufSeite && d.gesperrt.count(i) != 0;
    if (ebenenSymbol("##schloss", gesperrt ? UiIcon::Schloss : UiIcon::SchlossOffen, gesperrt, tr(Str::MeEbeneSchloss))) {
        if (gesperrt) { d.gesperrt.erase(i); } else { d.gesperrt.insert(i); }
    }
    ImGui::PopID();
}

void zeichneGliederung(Dokument& d) {
    Zustand& z = Z();
    // rc608 Teil 3a: Suche; der Umschalter "Ganze Datei" daneben
    {
        const char* ganz = tr(Str::MeGanzeDatei);
        const float hb = hakenBreite(ganz);
        const float breit = ImGui::GetContentRegionAvail().x;
        const bool nebeneinander = breit >= hb + ImGui::GetFontSize() * 7.0F;
        ImGui::SetNextItemWidth(nebeneinander ? breit - hb - ImGui::GetStyle().ItemSpacing.x : breit);
        (void)eingabe("##hier_suche", &z.hierarchieSuche, 0, tr(Str::MlHierSuche));
        if (nebeneinander) { ImGui::SameLine(); }
    }
    // rc608 Punkt 10 (shank: "wieso sehe rechts in der Leiste jeden Tab seine Hierachy und nicht nur den Tab den ich gerade
    // geoeffnet habe?"): ab Werk nur die offene Seite; die ganze Datei braucht man zum Verschieben zwischen den Seiten.
    ImGui::Checkbox(tr(Str::MeGanzeDatei), &g_app->settings.menueGanzeDatei);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MeGanzeDateiTip)); }
    const int offenesMenue = menuDerSeite(d);
    // Zustandsreiter (ui_sel=0_0): nur, was dort sichtbar ist
    const std::vector<Seite> seiten = seitenImMenue(d.datei);
    const Seite* zustand = nullptr;
    if (!g_app->settings.menueGanzeDatei && !seiten.empty()) {
        const Seite& sei = seiten[static_cast<std::size_t>(std::clamp(d.seite, 0, static_cast<int>(seiten.size()) - 1))];
        if (!sei.cvar.empty()) { zustand = &sei; }
    }
    const Umgebung uHier = zustand != nullptr ? umgebung(d) : Umgebung{};
    const std::string suche = kleinPfad(z.hierarchieSuche);
    const auto zeigen = [&](const Item& it) {
        if (!suche.empty() && kleinPfad(it.window.name + " " + it.text).find(suche) == std::string::npos) { return false; }
        if (zustand == nullptr || it.cvarTest.empty() || kleinPfad(it.cvarTest) != kleinPfad(zustand->cvar)) { return true; }
        return (it.cvarFlags & (CVAR_SHOW | CVAR_HIDE)) == 0 || itemZeigtUeberCvar(it, CVAR_SHOW, uHier);
    };
    // Die Liste folgt der Auswahl auf der Flaeche
    z.hierarchieZeilen.clear();
    z.hierarchieRollen = d.auswahl != z.hierarchieAuswahl;
    z.hierarchieAuswahl = d.auswahl;
    z.hierarchieReiheAlt = std::move(z.hierarchieReihe);
    z.hierarchieReihe.clear();
    ImGui::BeginChild("##ebenen_liste", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_None);
    for (std::size_t m = 0; m < d.datei.menus.size(); ++m) {
        if (!g_app->settings.menueGanzeDatei && static_cast<int>(m) != offenesMenue) { continue; }
        const Menu& me = d.datei.menus[m];
        ImGui::PushID(static_cast<int>(m));
        const bool offen = ImGui::TreeNodeEx("##m", ImGuiTreeNodeFlags_DefaultOpen, "menuDef %s%s", me.window.name.c_str(), me.ok ? "" : " (!)");
        if (ImGui::IsItemClicked()) {
            springe(d, me.anfang);
            zeigeMenue(d, static_cast<int>(m));
        }
        if (offen) {
            // M2 Ebenenliste (wie der Object Inspector): oben = vorne (zuletzt gezeichnet),
            // Gruppen (group) als Knoten; Auge, Schloss, Doppelklick = umbenennen, Ziehen = Reihenfolge
            std::set<std::string> gruppenGezeigt;
            for (std::size_t r = me.items.size(); r-- > 0;) {
                const std::string& g = me.items[r].window.group;
                if (g.empty()) {
                    if (zeigen(me.items[r])) { zeichneEbene(d, static_cast<int>(m), static_cast<int>(r)); }
                    continue;
                }
                if (!gruppenGezeigt.insert(g).second) { continue; }
                bool etwas = false;
                for (const Item& it : me.items) { etwas = etwas || (it.window.group == g && zeigen(it)); }
                if (!etwas) { continue; }
                if (ImGui::TreeNodeEx(("##g" + g).c_str(), ImGuiTreeNodeFlags_DefaultOpen, "group %s", anzeige(g).c_str())) {
                    for (std::size_t q = me.items.size(); q-- > 0;) {
                        if (me.items[q].window.group == g && zeigen(me.items[q])) { zeichneEbene(d, static_cast<int>(m), static_cast<int>(q)); }
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
}

GizmoWerte gizmoAus(const Item& it);   // app_menue_m7.inc

// rc608 Punkt 14 (shank: "wo kann ich da die animation auswaehlen"): model_g2anim wirkt nur vor asset_model (ItemParse_asset_model_go),
// darum steht die Zeile dort; mit isCharacter genuegt sie an der alten Stelle. Ein Undo-Schritt.
void setzeModellAnim(Dokument& d, int mi, int i, const Item& it, const std::string& anim) {
    const std::string& t = d.puffer.text();
    const std::string wert = "\"" + anim + "\"";
    std::vector<Ersetzung> e;
    const auto zeile = [&](const char* schluessel, std::size_t& von, std::size_t& bis) {
        const std::string key = schluessel;
        std::size_t p = it.anfang;
        while (p < it.ende && p < t.size()) {
            std::size_t ende = t.find('\n', p);
            if (ende == std::string::npos) { ende = t.size(); }
            std::size_t s = p;
            while (s < ende && (t[s] == ' ' || t[s] == '\t')) { ++s; }
            if (s + key.size() <= ende && gleichOhneFallStr(t.substr(s, key.size()), key) && (s + key.size() == ende || std::isspace(static_cast<unsigned char>(t[s + key.size()])) != 0)) {
                von = p;
                bis = std::min(ende + 1, t.size());
                return true;
            }
            p = ende + 1;
        }
        return false;
    };
    std::size_t av = 0, ab = 0, mv = 0, mb = 0;
    const bool hatAnim = zeile("model_g2anim", av, ab);
    const bool hatModell = zeile("asset_model", mv, mb);
    const bool direkt = (it.itemFlags & ITF_ISCHARACTER) != 0 || !hatModell || (hatAnim && av < mv) || it.kopf.datei != 0;
    if (direkt) {
        if (setzeEigenschaft(d.datei, mi, i, "model_g2anim", {wert}, e)) { aendere(std::move(e)); }
        else { ausMakroMelden(); }
        return;
    }
    std::size_t s = mv;
    while (s < mb && (t[s] == ' ' || t[s] == '\t')) { ++s; }
    const std::string neueZeile = t.substr(mv, s - mv) + "model_g2anim\t\t\t" + wert + d.puffer.zeilenUmbruch();
    if (hatAnim) { e.push_back(Ersetzung{static_cast<std::uint32_t>(av), static_cast<std::uint32_t>(ab), ""}); }
    e.push_back(Ersetzung{static_cast<std::uint32_t>(mv), static_cast<std::uint32_t>(mv), neueZeile});
    aendere(std::move(e));
}

// Die Felder des Modells (Gruppe in Properties). Beim Zaehlen melden sie sich nur an, siehe gruppe().
bool lsZahlMitSchieber(const char* id, const char* idSchieber, float* v, float lo, float hi, float tempo, const char* format);   // app_menue_leiste.inc
bool lsZahlEin(const char* id, float* v, float tempo, const char* format);
float lsKnopfRest(const char* knopf);

void zeichneModellFelder(Dokument& d, int mi, int i, const Item& it) {
    Zustand& z = Z();
    const bool jk2 = d.datei.spiel == Spiel::JK2;
    std::string gewaehlt;
    // Modell
    if (feldKopf(tr(Str::MeModellGruppe), "asset_model", tr(Str::MlFModellTip))) {
        std::string modell = it.assetModel;
        ImGui::SetNextItemWidth(lsKnopfRest(tr(Str::MeModellWaehlen)));
        textFeld("##asset_model", &modell, [i](const std::string& t) { eigenschaft(i, "asset_model", {inAnf(t)}); });
        ImGui::SameLine();
        if (ImGui::IsPopupOpen("##modelle")) { z.modellListe = z.daten.liste("models/players/", {"/model.glm"}); }
        if (suchListe("##modelle", tr(Str::MeModellWaehlen), z.listenFilter, z.modellListe, gewaehlt)) { eigenschaft(i, "asset_model", {inAnf(gewaehlt)}); }
    }
    if (!jk2) {
        // Skin aus dem Ordner des Modells
        if (feldKopf(tr(Str::MlFSkin), "model_g2skin", tr(Str::MlFSkinTip))) {
            std::string skin = it.g2skin;
            ImGui::SetNextItemWidth(lsKnopfRest(tr(Str::MeSkinWaehlen)));
            textFeld("##model_g2skin", &skin, [i](const std::string& t) { eigenschaft(i, "model_g2skin", {inAnf(t)}); });
            ImGui::SameLine();
            std::string ordner = kleinPfad(it.assetModel);
            const std::size_t sl = ordner.rfind('/');
            ordner = sl == std::string::npos ? std::string() : ordner.substr(0, sl + 1);
            if (ImGui::IsPopupOpen("##skins") && !ordner.empty()) { z.skinListe = z.daten.liste(ordner, {".skin"}); }
            if (suchListe("##skins", tr(Str::MeSkinWaehlen), z.listenFilter, z.skinListe, gewaehlt)) { eigenschaft(i, "model_g2skin", {inAnf(gewaehlt)}); }
        }
        // Animation: die Namen der animation.cfg des gewaehlten Modells
        if (feldKopf(tr(Str::MlFAnimation), "model_g2anim", tr(Str::MlFAnimationTip))) {
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%s", it.g2anim.empty() ? tr(Str::MeModellKeineAnim) : it.g2anim.c_str());
            daneben(knopfBreite(tr(Str::MeModellAnimWaehlen)));
            if (ImGui::IsPopupOpen("##anims")) {
                z.animListe.clear();
                const auto mv = z.modelle.find(modellSchluessel(it, umgebung(d).cvar));
                if (mv != z.modelle.end()) {
                    for (const AnimEntry& a : mv->second.modell.anims) { z.animListe.push_back(a.name); }
                }
                std::sort(z.animListe.begin(), z.animListe.end());
            }
            if (suchListe("##anims", tr(Str::MeModellAnimWaehlen), z.listenFilter, z.animListe, gewaehlt)) { setzeModellAnim(d, mi, i, it, gewaehlt); }
            if (it.g2animOhneWirkung && (it.itemFlags & ITF_ISCHARACTER) == 0 && !it.g2anim.empty()) {
                ImGui::PushTextWrapPos(0.0F);
                ImGui::TextColored(ImVec4{1.0F, 0.75F, 0.3F, 1.0F}, "%s", tr(Str::MeModellAnimOhneWirkung));
                ImGui::PopTextWrapPos();
            }
        }
    }
    // Blickrichtung und laufende Drehung sind zwei Dinge: model_angle steht, model_rotation dreht von selbst
    if (feldKopf(tr(Str::MeModellRichtung), "model_angle", tr(Str::MeModellRichtungTip), true)) {
        float winkel = static_cast<float>(it.angle);
        zahlenFeld("model_angle", &winkel, 1, [&winkel] { return lsZahlMitSchieber("##angle", "##angle_s", &winkel, 0.0F, 360.0F, 1.0F, "%.0f"); },
                   [i](const std::vector<float>& w) { eigenschaft(i, "model_angle", {std::to_string(static_cast<int>(w[0]))}); });
    }
    if (feldKopf(tr(Str::MeModellDrehung), "model_rotation", tr(Str::MeModellDrehungTip), true)) {
        float dreh = static_cast<float>(it.rotationSpeed);
        zahlenFeld("model_rotation", &dreh, 1, [&dreh] { return lsZahlEin("##rotation", &dreh, 1.0F, "%.0f"); },
                   [i](const std::vector<float>& w) { eigenschaft(i, "model_rotation", {std::to_string(static_cast<int>(w[0]))}); });
    }
    // Hoehe und Abstand: wie die Griffe auf der Flaeche ueber g2mins/g2maxs, denn daraus setzt das Spiel die Figur (Item_Model_Paint)
    if (!jk2) {
        const GizmoWerte g = gizmoAus(it);
        if (feldKopf(tr(Str::MeModellHoehe), "model_g2mins, model_g2maxs", tr(Str::MeGizmoHoehe), true)) {
            float hoehe = 0.5F * (g.mins[2] + g.maxs[2]);
            zahlenFeld("model_hoehe", &hoehe, 1, [&hoehe] { return lsZahlMitSchieber("##hoehe", "##hoehe_s", &hoehe, -30.0F, 30.0F, 0.5F, "%.1f"); }, [&d, mi, i, g](const std::vector<float>& w) {
                const float halb = 0.5F * (g.maxs[2] - g.mins[2]);
                std::vector<Ersetzung> e;
                (void)setzeEigenschaft(d.datei, mi, i, "model_g2mins", {zahlText(g.mins[0]), zahlText(g.mins[1]), zahlText(w[0] - halb)}, e);
                (void)setzeEigenschaft(d.datei, mi, i, "model_g2maxs", {zahlText(g.maxs[0]), zahlText(g.maxs[1]), zahlText(w[0] + halb)}, e);
                aendere(std::move(e));
            });
        }
        if (feldKopf(tr(Str::MeModellAbstand), "model_g2mins, model_g2maxs", tr(Str::MeGizmoAbstand), true)) {
            float abstand = g.maxs[2] - g.mins[2];
            zahlenFeld("model_abstand", &abstand, 1, [&abstand] { return lsZahlMitSchieber("##abstand", "##abstand_s", &abstand, 20.0F, 160.0F, 0.5F, "%.1f"); }, [&d, mi, i, g](const std::vector<float>& w) {
                const float mitte = 0.5F * (g.mins[2] + g.maxs[2]);
                std::vector<Ersetzung> e;
                (void)setzeEigenschaft(d.datei, mi, i, "model_g2mins", {zahlText(g.mins[0]), zahlText(g.mins[1]), zahlText(mitte - 0.5F * w[0])}, e);
                (void)setzeEigenschaft(d.datei, mi, i, "model_g2maxs", {zahlText(g.maxs[0]), zahlText(g.maxs[1]), zahlText(mitte + 0.5F * w[0])}, e);
                aendere(std::move(e));
            });
        }
    }
    if (feldKopf(tr(Str::MlFBlickwinkel), "model_fovx, model_fovy", tr(Str::MlFBlickwinkelTip))) {
        static const char* const kFov[2] = {"x", "y"};
        float fov[2] = {it.fovx, it.fovy};
        zahlenFeld("model_fov", fov, 2, [&fov] { return zahlenReihe("model_fov", fov, 2, kFov, 0.5F, "%g"); }, [&d, mi, i](const std::vector<float>& w) {
            std::vector<Ersetzung> e;
            (void)setzeEigenschaft(d.datei, mi, i, "model_fovx", {zahlText(w[0])}, e);
            (void)setzeEigenschaft(d.datei, mi, i, "model_fovy", {zahlText(w[1])}, e);
            aendere(std::move(e));
        });
    }
    if (!jk2 && feldKopf(tr(Str::MlFArt), "isCharacter, isSaber", tr(Str::MlFArtTip))) {
        bool figur = (it.itemFlags & ITF_ISCHARACTER) != 0;
        if (ImGui::Checkbox(tr(Str::MlFFigur), &figur)) { eigenschaft(i, "isCharacter", {figur ? "1" : "0"}); }
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MeModellFigurTip)); }
        daneben(hakenBreite(tr(Str::MlFSaebel)));
        bool saber = (it.itemFlags & ITF_ISSABER) != 0;
        if (ImGui::Checkbox(tr(Str::MlFSaebel), &saber)) { eigenschaft(i, "isSaber", {saber ? "1" : "0"}); }
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MeModellSaberTip)); }
    }
}

// Stehen in der Datei Schluessel, die Properties nicht zeigt (auch solche, die bei diesem Typ nichts bewirken), bleiben sie erhalten und stehen unter "Weitere Eintraege"
void weitereEintraegeLesen(const Dokument& d, const Item& it) {
    Zustand& z = Z();
    z.weitereEintraege.clear();
    if (it.kopf.datei != 0) { return; }
    const std::string& t = d.puffer.text();
    // Die Skripte stehen in Events, Auge und Karte zeigen die uebrigen
    std::set<std::string> gezeigt = {"itemdef", "visible", "name", "type", "style", "rect", "mouseenter", "mouseexit", "action", "onfocus", "leavefocus", "accept", "focussound"};
    for (const std::string& f : z.felderGezeigt) { gezeigt.insert(kleinPfad(f)); }
    std::size_t p = it.anfang;
    int tiefe = 0;
    while (p < it.ende && p < t.size()) {
        std::size_t ende = t.find('\n', p);
        if (ende == std::string::npos || ende > it.ende) { ende = std::min<std::size_t>(it.ende, t.size()); }
        std::string zeile = t.substr(p, ende - p);
        p = ende + 1;
        if (const std::size_t k = zeile.find("//"); k != std::string::npos) { zeile.resize(k); }
        const std::size_t a = zeile.find_first_not_of(" \t\r");
        if (a == std::string::npos) { continue; }
        const int vorher = tiefe;
        for (const char c : zeile) {
            if (c == '{') { ++tiefe; }
            if (c == '}') { --tiefe; }
        }
        if (vorher != 1 || zeile[a] == '{' || zeile[a] == '}') { continue; }
        std::size_t e = a;
        while (e < zeile.size() && std::isspace(static_cast<unsigned char>(zeile[e])) == 0 && zeile[e] != '{') { ++e; }
        const std::string schluessel = zeile.substr(a, e - a);
        if (gezeigt.count(kleinPfad(schluessel)) != 0) { continue; }
        z.weitereEintraege.push_back(zeile.substr(a));
    }
}

// M3 Punkt 5: die Werte, die cgame fuer das HUD liest (playerState, cvars), als Regler
// M3 Punkt 5 / M7: die Werte des Spielers (playerState) als Regler - fuer das HUD und das Datapad
void hudRegler(HudWerte& w) {
    const float breit = ImGui::GetFontSize() * 9;
    auto regler = [&](Str name, int* v, int lo, int hi) {
        ImGui::SetNextItemWidth(breit);
        ImGui::SliderInt(tr(name), v, lo, hi);
    };
    regler(Str::MeHudGesundheit, &w.gesundheit, -10, 200);
    regler(Str::MeHudGesundheitMax, &w.gesundheitMax, 1, 200);
    regler(Str::MeHudRuestung, &w.ruestung, 0, 200);
    regler(Str::MeHudMacht, &w.macht, 0, 200);
    regler(Str::MeHudMachtMax, &w.machtMax, 1, 200);
    ImGui::Checkbox(tr(Str::MeHudMachtBekannt), &w.machtBekannt);
    ImGui::Checkbox(tr(Str::MeHudSaebel), &w.saebel);
    if (w.saebel) {
        static const char* const stile[] = {"SS_FAST", "SS_MEDIUM", "SS_STRONG", "SS_DESANN", "SS_TAVION", "SS_DUAL", "SS_STAFF"};
        int i = std::clamp(w.saebelStil, 1, 7) - 1;
        ImGui::SetNextItemWidth(breit);
        if (ImGui::Combo(tr(Str::MeHudSaebelStil), &i, stile, 7)) { w.saebelStil = i + 1; }
    } else {
        regler(Str::MeHudMunition, &w.munition, -1, 999);
        regler(Str::MeHudMunitionMax, &w.munitionMax, 1, 999);
    }
}

void zeichneHudWerte(const Dokument& d) {
    Zustand& z = Z();
    if (!istHud(d.datei) || !ImGui::CollapsingHeader(tr(Str::MeHudWerte), ImGuiTreeNodeFlags_DefaultOpen)) { return; }
    ImGui::Checkbox(tr(Str::MeHudWie), &z.hudWie);
    if (!hudNachgebildet(d.datei)) { ImGui::TextColored(ImVec4{1, 0.7F, 0.3F, 1}, "%s", tr(Str::MeHudNichtNachgebildet)); }
    HudWerte& w = z.hudWerte;
    hudRegler(w);
    const float breit = ImGui::GetFontSize() * 9;
    ImGui::SetNextItemWidth(breit);
    ImGui::SliderInt(tr(Str::MeHudFadenkreuz), &w.fadenkreuz, 0, 9);
    ImGui::SetNextItemWidth(breit);
    ImGui::SliderFloat(tr(Str::MeHudFadenkreuzGroesse), &w.fadenkreuzGroesse, 4, 64, "%.0f");
}

void zeichneBeispielWerte(const Dokument& d);   // M7 (app_menue_m7.inc)

void zeichneCvars(Dokument& d) {
    Zustand& z = Z();
    zeichneHudWerte(d);
    zeichneBeispielWerte(d);
    ImGui::PushTextWrapPos(0.0F);
    ImGui::TextDisabled("%s", tr(Str::MeCvarsNurVorschau));
    ImGui::PopTextWrapPos();
    if (ImGui::SmallButton(tr(Str::MeCvarsZuruecksetzen))) {
        z.vorschauCvars.clear();
        z.lauf.cvars.clear();
        ++z.lauf.version;
    }
    for (const CvarFund& c : cvarsImMenue(d.datei)) {
        ImGui::PushID(c.name.c_str());
        std::string w = z.interaktion ? z.lauf.cvar(c.name) : (z.vorschauCvars.count(kleinPfad(c.name)) != 0 ? z.vorschauCvars[kleinPfad(c.name)] : "");
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5);
        if (eingabe("##w", &w)) { setzeCvar(c.name, w); }
        daneben(ImGui::CalcTextSize(c.name.c_str()).x);
        if (c.sichtbarkeit) { ImGui::TextColored(ImVec4{0.4F, 0.8F, 1.0F, 1.0F}, "%s", c.name.c_str()); }
        else { ImGui::TextUnformatted(c.name.c_str()); }
        if (ImGui::IsItemHovered() && !c.wofuer.empty()) {
            std::string t;
            for (const std::string& x : c.wofuer) { t += x + "\n"; }
            ImGui::SetTooltip("%s", t.c_str());
        }
        for (const std::string& v : c.werte) {
            daneben(knopfBreite(v.c_str()));
            if (ImGui::SmallButton(v.c_str())) { setzeCvar(c.name, v); }
        }
        ImGui::PopID();
    }
}

void zeichneMeldungen(Dokument& d) {
    Zustand& z = Z();
    // rc608 (shank fragte, was der Abschnitt macht, weil er leer dastand)
    ImGui::PushTextWrapPos(0.0F);
    ImGui::TextDisabled("%s", tr(Str::MeMeldungenSatz));
    if (d.datei.meldungen.empty()) {
        ImGui::TextUnformatted(tr(Str::MeMeldungenKeine));
        z.meldungenLeerBild = ImGui::GetFrameCount();
    }
    ImGui::PopTextWrapPos();
    for (std::size_t k = 0; k < d.datei.meldungen.size(); ++k) {
        const Meldung& m = d.datei.meldungen[k];
        ImGui::PushID(static_cast<int>(k));
        const std::string t = meldungText(d.datei, m);
        const ImU32 farbe = ImGui::ColorConvertFloat4ToU32(m.fehler ? ImVec4{1, 0.45F, 0.45F, 1} : ImVec4{1, 0.85F, 0.4F, 1});
        // Umbricht statt abzuschneiden; die ganze Meldung ist die Klickflaeche, ein Klick springt zur Zeile
        const float breite = ImGui::GetContentRegionAvail().x;
        const float hoehe = ImGui::CalcTextSize(t.c_str(), nullptr, false, breite).y;
        // NoPadWithHalfSpacing: sonst ragt die Flaeche ueber das Feld hinaus und der Text laege um den halben Abstand daneben
        const bool klick = ImGui::Selectable("##meldung", false, ImGuiSelectableFlags_NoPadWithHalfSpacing, ImVec2{0.0F, hoehe});
        ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImGui::GetItemRectMin(), farbe, t.c_str(), nullptr, breite);
        if (klick && m.datei == 0 && m.zeile > 0) { springe(d, d.puffer.zeilenAnfang(m.zeile - 1)); }
        ImGui::PopID();
    }
    if (z.interaktion) {
        ImGui::SeparatorText(tr(Str::MeProtokoll));
        for (const std::string& s : z.lauf.protokoll) { ImGui::TextUnformatted(s.c_str()); }
    }
}

void zeichneSpielListe() {
    Zustand& z = Z();
    if (z.spielListeOffen) {
        ImGui::OpenPopup("##spielmenues");
        z.spielListeOffen = false;
    }
    if (ImGui::BeginPopup("##spielmenues")) {
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 20);
        if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }
        eingabe("##filter", &z.spielFilter);
        ImGui::BeginChild("##liste", ImVec2{ImGui::GetFontSize() * 24, ImGui::GetFontSize() * 24});
        for (const std::string& p : z.daten.liste("ui/", {".menu", ".txt"})) {
            if (!z.spielFilter.empty() && kleinPfad(p).find(kleinPfad(z.spielFilter)) == std::string::npos) { continue; }
            if (ImGui::Selectable(p.c_str())) {
                oeffneAusSpiel(p);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
}

// Trennstrich zwischen zwei Gruppen der Leiste
void leistenTrenner() {
    ImGui::SameLine();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float h = ImGui::GetFrameHeight();
    ImGui::GetWindowDrawList()->AddLine(ImVec2{p.x + 3, p.y + h * 0.15F}, ImVec2{p.x + 3, p.y + h * 0.85F}, ImGui::GetColorU32(ImGuiCol_Separator), 1.0F);
    ImGui::Dummy(ImVec2{7, h});
    ImGui::SameLine();
}

// M2 modernes Aussehen: die Leiste in Gruppen mit Symbolen und Hinweisen (vier Sprachen):
// Datei | Rueckgaengig | Paket (Video, Schrift, Texte, Sprache) | Pruefen, Ausgeben, im Spiel
// testen | Bearbeiten/Bedienen | Ansicht (Seitenverhaeltnis, Aufloesung, Schalter)
void zeichneProjektKnopf();   // M5b Punkt 3 (app_menue_projekt.inc)
void zeichneFlussFenster(Dokument* aktivDok);   // M6 (app_menue_m6.inc)
void zeichneRefKnopf();
void zeichneSchliessenFrage();
void zeichneStileFenster(Dokument* d);
void zeichneAssistentFenster(Dokument* d);
void zeichneOriginal(Dokument& d);

// M2 Punkt 3: den Menuesatz zum Dokument oeffnen (UI_LoadMenus) - eine offene menus.txt selbst,
// sonst die Menuelisten des Mod-Profils (M5b)
int oeffneMenuesatzFuer(Dokument* d) {
    int n = 0;
    if (d != nullptr && kleinPfad(d->name).size() > 4 && kleinPfad(d->name).compare(kleinPfad(d->name).size() - 4, 4, ".txt") == 0) {
        n = oeffneMenuesatz(d->name);
    }
    const bool mp = d != nullptr && d->datei.dialekt == Dialekt::MP;
    for (const std::string& p : menuelisten(*aktivesProfil(), mp ? Dialekt::MP : Dialekt::SP)) {
        if (n == 0) { n = oeffneMenuesatz(p); }
    }
    return n;
}
bool speichereProjekt();

// Ein Weg fuer den Knopf und die Tasten. Im Projekt speichert "Speichern" das ganze Projekt, "Speichern unter" nur eine Kopie dieser Datei.
void speichernWunsch(Dokument& d, bool unter) {
    Zustand& z = Z();
    if (!unter && !z.projektDatei.empty()) {   // M5b
        (void)speichereProjekt();
        return;
    }
    if (!unter && !d.platte.empty() && !imSpielordner(d.platte)) {
        speichern(d, d.platte);
        return;
    }
    // rc609 ME2: ein loses Menue im Mod-Ordner an seinen Ort; aus dem Spiel geoeffnet ist der Ort, wo das Spiel es findet.
    // Eines in einer .pk3 bleibt bei Speichern unter, mit Hinweis.
    std::string hinweis;
    if (!unter) {
        std::string ort = d.platte;
        if (ort.empty() && d.ausSpiel) {
            datenPruefen();
            bool lose = false;
            std::string wo;
            if (z.daten.herkunft(d.name, wo, lose)) {
                if (lose) {
                    fs::path p = utf8Pfad(wo);
                    ort = u8(p.make_preferred());
                } else {
                    char b[1600];
                    std::snprintf(b, sizeof(b), tr(Str::MeInPk3Kopie), d.name.c_str(), wo.c_str());
                    hinweis = b;
                }
            }
        }
        if (!ort.empty() && loseImModOrdner(ort)) {
            speichernAmOrt(d, ort);
            return;
        }
    }
    const std::string ziel = z.speichernDialog ? z.speichernDialog()
                                               : platform::saveFileDialog(tr(unter ? Str::ActSaveAs : Str::MeSpeichern), "Menu (*.menu)|*.menu|", z.ordnerAusgabe,
                                                                          u8(utf8Pfad(d.name).filename()));
    if (!hinweis.empty()) { ::bhed::gui::meldung(hinweis, std::string(), true); }   // auch bei Abbruch, ganz in der Einblendung
    if (ziel.empty()) {
        if (!hinweis.empty()) { z.meldung = hinweis; }
        return;
    }
    if (!z.projektDatei.empty()) {
        char b[600];
        std::snprintf(b, sizeof(b), schreibeDatei(ziel, d.puffer.text()) ? tr(Str::MeSpeichernKopie) : tr(Str::MeNichtGespeichert), ziel.c_str());
        z.meldung = b;
        addStatus(b);
        return;
    }
    std::string neuerName = u8(utf8Pfad(d.name).parent_path() / utf8Pfad(ziel).filename());
    std::replace(neuerName.begin(), neuerName.end(), '\\', '/');
    speichern(d, ziel);
    if (!hinweis.empty()) { z.meldung += "  " + hinweis; }
    if (d.platte == ziel && neuerName != d.name) {   // Der Ordner im Spielpfad bestimmt das Format, der neue Dateiname gilt beim Export
        d.name = neuerName;
        d.geparst = -1;
        neuParsen(d);
    }
}

// rc608 Teil 3b: Eine offene Datei hat sich auf der Platte geaendert. Eigene Schreibvorgaenge erkennt der gleiche Inhalt.
void fremdPruefen(Dokument& d) {
    if (d.platte.empty() || d.fremd) { return; }
    const double jetzt = ImGui::GetTime();
    if (jetzt - d.platteGeprueft < 1.0) { return; }
    d.platteGeprueft = jetzt;
    std::error_code ec;
    const fs::path p = utf8Pfad(d.platte);
    const fs::file_time_type zeit = fs::last_write_time(p, ec);
    if (ec) { return; }
    const std::uintmax_t groesse = fs::file_size(p, ec);
    if (ec || (d.platteGemerkt && zeit == d.platteZeit && groesse == d.platteGroesse)) { return; }
    const bool zumErstenMal = !d.platteGemerkt;
    d.platteZeit = zeit;
    d.platteGroesse = groesse;
    d.platteGemerkt = true;
    if (zumErstenMal) { return; }
    std::ifstream f(p, std::ios::binary);
    std::string inhalt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (f && inhalt != d.gespeichert && inhalt != d.puffer.text()) {
        d.fremd = true;
        d.fremdText = std::move(inhalt);
    } else if (f) {
        d.gespeichert = inhalt == d.puffer.text() ? inhalt : d.gespeichert;
    }
}

// rc608 Teil 3b: Ein Schritt des Tutorials zeigt nie auf etwas Zugeklapptes: Was er braucht, geht von selbst auf
void tourKlapptAuf() {
    const TourSicht t = tourSicht();
    if (!t.aktiv || t.id != "tut-menue-eigen") { return; }
    Zustand& z = Z();
    Zustand::Leiste& l = z.leiste;
    if (t.schritt == 2) {
        z.linksZu = false;
        l.zu.erase("l_zeigen");
        z.teileSuche.clear();
        if (z.spaltenEng) { z.rechtsZu = true; }   // wenig Platz: nur die Spalte, die der Schritt braucht
    }
    if (t.schritt >= 3 && t.schritt <= 6) {
        z.rechtsZu = false;
        // Die Tour geht beim ersten Knopf weiter, der Text verlangt drei: Die Teile bleiben, bis sie da sind.
        if (z.spaltenEng && tourWerte().elemente >= 4) { z.linksZu = true; }
    }
    if (t.schritt == 3) {
        l.zu.erase("ordnen");
        l.zu.insert("alle");   // "Alle aendern" steht darueber und schob "Anordnen" bei wenig Hoehe aus dem Bild
    }
}

void zeichneFremdZeile(Dokument& d) {
    if (!d.fremd) { return; }
    std::vector<char> b(800);
    std::snprintf(b.data(), b.size(), tr(Str::MeFremdText), d.platte.c_str());
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0F, 0.75F, 0.3F, 1.0F});
    ImGui::TextWrapped("%s", b.data());
    ImGui::PopStyleColor();
    if (ImGui::SmallButton(tr(Str::MeFremdLaden))) {
        d.fremd = false;
        d.gespeichert = d.fremdText;
        d.auswahl.clear();
        aendere({Ersetzung{0, static_cast<std::uint32_t>(d.puffer.text().size()), d.fremdText}});
        d.fremdText.clear();
        Z().meldung = tr(Str::MeFremdGeladen);
    } else {
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MeFremdLadenTip)); }
        ImGui::SameLine();
        if (ImGui::SmallButton(tr(Str::MeFremdBehalten))) {
            d.fremd = false;
            d.fremdText.clear();
        }
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MeFremdBehaltenTip)); }
    }
}

// "Datei oeffnen" der Leiste und des Startbildschirms
void dateiOeffnenWaehlen() {
    Zustand& z = Z();
    const std::string pfad = z.menueDialog ? z.menueDialog() : platform::openFileDialog(tr(Str::MeDateiOeffnen), "Menu (*.menu;*.txt;*.jksmenu)|*.menu;*.txt;*.jksmenu|", "");
    if (!pfad.empty()) { (void)oeffneDatei(pfad); }
}

void zeichneLeiste(Dokument* d) {
    Zustand& z = Z();
    zeichneProjektKnopf();   // M5b Punkt 3
    ImGui::SameLine();
    if (leistenKnopf(tr(Str::MeAusSpiel), UiIcon::Liste, tr(Str::MeAusSpielHint))) { z.spielListeOffen = true; }
    zeichneSpielListe();
    ImGui::SameLine();
    if (leistenKnopf(tr(Str::MeMenuesatzOeffnen), UiIcon::Explorer, tr(Str::MeTipMenuesatz))) { (void)oeffneMenuesatzFuer(d); }   // M2 Punkt 3
    ImGui::SameLine();
    if (leistenKnopf(tr(Str::MeDateiOeffnen), UiIcon::Oeffnen, tr(Str::MeDateiOeffnenHint))) { dateiOeffnenWaehlen(); }
    if (d != nullptr) {
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeSpeichern), UiIcon::Speichern, tr(z.projektDatei.empty() ? Str::MeSpeichernHint : Str::MePrjSpeichernTip))) { speichernWunsch(*d, false); }
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::ActSaveAs), UiIcon::SpeichernUnter, tr(Str::MeSpeichernUnterTip))) { speichernWunsch(*d, true); }
        leistenTrenner();
        if (leistenKnopf(tr(Str::MeRueckgaengig), UiIcon::Rueckgaengig, tr(Str::MeTipRueckgaengig), false, d->puffer.kannRueckgaengig())) { rueckgaengig(); }
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeWiederholen), UiIcon::Wiederholen, tr(Str::MeTipWiederholen), false, d->puffer.kannWiederholen())) { wiederholen(); }
        leistenTrenner();
        // Paket: Video, Schrift, Texte und Sprache der Vorschau
        if (leistenKnopf(tr(Str::MeVideoImport), UiIcon::Kamera, tr(Str::MeTipVideo))) {   // M2 Punkt 2
            const std::string pfad = platform::openFileDialog(tr(Str::MeVideoImport),
                                                              "Video (*.mp4;*.webm;*.avi;*.wmv;*.mov;*.mkv;*.gif;*.roq)|*.mp4;*.webm;*.avi;*.wmv;*.mov;*.mkv;*.gif;*.roq|", "");
            if (!pfad.empty()) {
                z.videoQuelle = pfad;
                z.videoVerwendung = VideoVerwendung::NurPaket;
                z.videoOffen = true;
            }
        }
        zeichneVideoFenster();
        zeichneBildWahl();
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeSchriftBauen), UiIcon::Zeilen, tr(Str::MeTipSchrift))) {   // M2 Punkt 9
            const std::string pfad = platform::openFileDialog(tr(Str::MeSchriftBauen), "TrueType (*.ttf;*.otf)|*.ttf;*.otf|", "");
            if (!pfad.empty()) {
                z.schriftTtf = pfad;
                z.schriftFenster = true;
            }
        }
        zeichneSchriftFenster();
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeTexte), UiIcon::Namen, tr(Str::MeTipTexte), z.texteFensterOffen)) { z.texteFensterOffen = !z.texteFensterOffen; }   // M2 Punkt 6
        zeichneTexteFenster(*d);
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeStile), UiIcon::Leuchten, tr(Str::MeStileTip), z.stileOffen)) { z.stileOffen = !z.stileOffen; }   // M6 Punkt 4
        zeichneStileFenster(d);
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeAssistent), UiIcon::Einstellungen, tr(Str::MeAssistentTip), z.assistentOffen)) { z.assistentOffen = !z.assistentOffen; }   // M6 Punkt 6
        zeichneAssistentFenster(d);
        ImGui::SameLine();
        {
            const std::vector<std::string> sp = sprachenListe();
            ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.5F);
            if (ImGui::BeginCombo("##sprache", z.sprache.c_str())) {
                for (const std::string& s : sp) {
                    if (ImGui::Selectable(s.c_str(), s == z.sprache)) { setzeSprache(s); }
                }
                ImGui::EndCombo();
            }
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s\n%s", tr(Str::MeSprache), tr(Str::MeTipSprache)); }
        }
        leistenTrenner();
        // Pruefen, Ausgeben, im Spiel testen
        if (leistenKnopf(tr(Str::MePruefen), UiIcon::Haken, tr(Str::MeTipPruefen), z.pruefFenster)) {   // M2 Punkt 7
            z.pruefFenster = true;
            z.pruefGelaufen = false;
        }
        zeichnePruefFenster();
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeFluss), UiIcon::Pfad, tr(Str::MeFlussTip), z.flussOffen)) { z.flussOffen = !z.flussOffen; }   // M6 Punkt 1
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeExportPk3), UiIcon::Paket, tr(Str::MeExportPk3Hint))) { ImGui::OpenPopup("##export"); }
        if (ImGui::BeginPopup("##export")) {
            if (ImGui::MenuItem(tr(Str::MeExportPk3))) {
                const std::string ziel = z.speichernDialog ? z.speichernDialog() : platform::saveFileDialog(tr(Str::MeExportPk3), "pk3 (*.pk3)|*.pk3|", z.ordnerAusgabe, "zz_menu.pk3");
                if (!ziel.empty()) { (void)exportierePk3(ziel); }
            }
            if (ImGui::MenuItem(tr(Str::MeExportAenderungen))) {
                const std::string ziel = platform::saveFileDialog(tr(Str::MeExportAenderungen), "pk3 (*.pk3)|*.pk3|", z.ordnerAusgabe, "zz_menu.pk3");
                if (!ziel.empty()) { (void)exportiereAenderungen(ziel); }
            }
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeImSpielTesten), UiIcon::Abspielen, tr(Str::MeTipImSpiel))) { (void)menueImSpielTesten(false); }   // M2 Punkt 8
        zeichneTestFrage();
    }
    leistenTrenner();
    // Bearbeiten / Bedienen
    if (leistenKnopf(tr(Str::MeBearbeiten), UiIcon::Verschieben, tr(Str::MeBearbeitenHint), !z.interaktion)) { setzeInteraktion(false); }
    ImGui::SameLine();
    if (leistenKnopf(tr(Str::MeBedienen), UiIcon::Steuern, tr(Str::MeBedienenHint), z.interaktion)) { setzeInteraktion(true); }
    leistenTrenner();
    // rc608 Teil 3b: Bei wenig Hoehe (1920 x 1080 bei 200 %) stehen die Ansicht-Schalter hinter einem Knopf, das spart eine Reihe fuer die Flaeche
    const auto ansicht = [&] {
        // Ansicht
        int sv = z.seitenv == Seitenverhaeltnis::Strecken ? 0 : 1;
        const char* svn[] = {tr(Str::MeStrecken), tr(Str::MeBalken)};
        // uebersetzung1: so breit wie der laengste Eintrag (cs "Roztáhnout (originál)" war abgeschnitten)
        ImGui::SetNextItemWidth(std::max(ImGui::CalcTextSize(svn[0]).x, ImGui::CalcTextSize(svn[1]).x) + ImGui::GetFrameHeight() +
                                ImGui::GetStyle().FramePadding.x * 2.0F);
        if (ImGui::Combo("##sv", &sv, svn, 2)) { z.seitenv = sv == 0 ? Seitenverhaeltnis::Strecken : Seitenverhaeltnis::Balken; }
        ImGui::SameLine();
        const char* const aufl[] = {tr(Str::MlAufloesungFrei), "640x480", "800x600", "1024x768", "1280x720", "1366x768", "1600x900", "1920x1080", "2560x1440", "3440x1440"};
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6);
        ImGui::Combo("##aufl", &z.aufloesung, aufl, 10);
        ImGui::SameLine();
        // M4: gut sichtbarer Ton-Schalter fuer alle Klaenge der Vorschau (Fokus, Klick, Musik),
        // gespeichert; aus haelt auch laufende Klaenge an
        if (iconTextKnopf(tr(g_app->settings.menueTon ? Str::MeTonAn : Str::MeTonAus), g_app->settings.menueTon ? UiIcon::Ton : UiIcon::TonAus,
                          tr(Str::MeTipTon))) {
            g_app->settings.menueTon = !g_app->settings.menueTon;
            if (!g_app->settings.menueTon) { stoppeKlaenge(); }
        }
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeAlleItems), UiIcon::Sichtbar, tr(Str::MeAlleItemsHint), z.alleItems, true, true)) { z.alleItems = !z.alleItems; }
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeRaster), UiIcon::Raster, tr(Str::MeRasterHint), z.raster, true, true)) { z.raster = !z.raster; }
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeAnRaster), UiIcon::Zielen, tr(Str::MeAnRasterHint), z.anRaster, true, true)) { z.anRaster = !z.anRaster; }
        ImGui::SameLine();
        if (leistenKnopf(tr(Str::MeAnKanten), UiIcon::Einpassen, tr(Str::MeAnKantenHint), z.anKanten, true, true)) { z.anKanten = !z.anKanten; }
        if (d != nullptr) {
            ImGui::SameLine();
            if (leistenKnopf(tr(Str::MeTabFolge), UiIcon::Zeilen, tr(Str::MeTabFolgeTip), z.tabFolge, true, true)) {   // M6 Punkt 2
                z.tabFolge = !z.tabFolge;
                z.tabNeu.clear();
                if (z.tabFolge) { setzeInteraktion(false); }
            }
            ImGui::SameLine();
            zeichneRefKnopf();   // M6 Punkt 7
        }
    };
    if (ImGui::GetIO().DisplaySize.y < ImGui::GetFontSize() * 24.0F) {
        if (leistenKnopf(tr(Str::MeAnsichtKnopf), UiIcon::Sichtbar, tr(Str::MeAnsichtKnopfTip))) { ImGui::OpenPopup("##ansichtmehr"); }
        if (ImGui::BeginPopup("##ansichtmehr")) {
            ImGui::Dummy(ImVec2{ImGui::GetFontSize() * 18.0F, 0.0F});
            ansicht();
            ImGui::EndPopup();
        }
    } else {
        ansicht();
    }
    zeichneFlussFenster(d);
    zeichneSchliessenFrage();
    if (!z.meldung.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", z.meldung.c_str());
    }
}

// Ein neues menuDef vor die letzte } der Datei (sonst ans Ende): Name basis, bei Gleichnamigem mit
// Nummer; block(name) liefert den Text. Danach steht die erste Seite des neuen Menues offen.
void menuEinfuegen(Dokument& d, const std::string& basis, const std::function<std::string(const std::string&)>& block) {
    std::string name = basis;
    for (int n = 2; n < 100; ++n) {
        bool frei = true;
        for (const Menu& m : d.datei.menus) { frei = frei && !gleichOhneFallStr(m.window.name, name); }
        if (frei) { break; }
        name = basis + std::to_string(n);
    }
    const std::string& t = d.puffer.text();
    std::size_t p = t.rfind('}');
    std::size_t q = (p != std::string::npos && !d.datei.menus.empty() && p >= d.datei.menus.back().ende) ? p : t.size();
    if (q < t.size()) {
        while (q > 0 && (t[q - 1] == ' ' || t[q - 1] == '\t')) { --q; }
    }
    aendere({Ersetzung{static_cast<std::uint32_t>(q), static_cast<std::uint32_t>(q), block(name)}});
    neuParsen(d);
    const std::vector<Seite> s = seitenImMenue(d.datei);
    d.seite = static_cast<int>(s.size()) - 1;
    for (std::size_t k = 0; k < s.size(); ++k) {
        if (s[k].menu == static_cast<int>(d.datei.menus.size()) - 1) {
            d.seite = static_cast<int>(k);
            break;
        }
    }
    d.auswahl.clear();
}

// Neue Seite: leer (vorlage -1) oder eine Vorlage eines ganzen Menues (M6 Punkt 5, SeitenVorlage)
void neueSeite(Dokument& d, int vorlage) {
    const std::string nl = d.puffer.zeilenUmbruch();
    const Dialekt dl = d.datei.dialekt;
    if (vorlage >= 0) {
        const auto art = static_cast<SeitenVorlage>(vorlage);
        menuEinfuegen(d, seitenVorlageName(art, dl), [&](const std::string& name) { return seitenVorlage(art, dl, name, "\t", nl); });
        return;
    }
    menuEinfuegen(d, "newMenu", [&](const std::string& name) {
        // rc608 Punkt 9: Im MP zeichnet fullScreen ohne background den grauen Kasten: Das neue Menue deckt ihn mit einem schwarzen Element
        const std::string grund = dl == Dialekt::MP ? "\t\titemDef" + nl + "\t\t{" + nl + "\t\t\tname\t\t\tbackground" + nl + "\t\t\trect\t\t\t0 0 640 480" + nl +
                                                          "\t\t\tstyle\t\t\t1" + nl + "\t\t\tbackcolor\t\t0 0 0 1" + nl + "\t\t\tdecoration" + nl + "\t\t\tvisible\t\t\t1" + nl + "\t\t}" + nl
                                                    : std::string();
        return "\tmenuDef" + nl + "\t{" + nl + "\t\tname\t\t\t\"" + name + "\"" + nl + "\t\tfullScreen\t\t1" + nl + "\t\trect\t\t\t0 0 640 480" + nl +
               "\t\tvisible\t\t\t1" + nl + "\t\tfocusColor\t\t1 1 1 1" + nl + grund + "\t}" + nl;
    });
}

// rc608 Teil 3b: Menue umbenennen, duplizieren, loeschen, sortieren; je ein Schritt, die Seite folgt ihrem Menue
void seiteVonMenue(Dokument& d, int menu) {
    const std::vector<Seite> s = seitenImMenue(d.datei);
    for (std::size_t k = 0; k < s.size(); ++k) {
        if (s[k].menu == menu) {
            d.seite = static_cast<int>(k);
            return;
        }
    }
}

void zeichneSeitenKontext(Dokument& d, const std::vector<Seite>& s) {
    Zustand& z = Z();
    if (!ImGui::BeginPopup("##seitekontext")) { return; }
    if (z.seiteKontext < 0 || static_cast<std::size_t>(z.seiteKontext) >= s.size()) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }
    const Seite& sei = s[static_cast<std::size_t>(z.seiteKontext)];
    const int mi = sei.menu;
    const int anzahl = static_cast<int>(d.datei.menus.size());
    if (!sei.cvar.empty()) {
        std::vector<char> b(600);
        std::snprintf(b.data(), b.size(), tr(Str::MsZustandTip), anzeige(sei.cvar).c_str(), anzeige(sei.wert).c_str());
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0F);
        ImGui::TextDisabled("%s", b.data());
        ImGui::PopTextWrapPos();
        ImGui::Separator();
    }
    ImGui::TextUnformatted(tr(Str::MsUmbenennen));
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 16.0F);
    if (ImGui::IsWindowAppearing() || z.seiteNameFokus) { ImGui::SetKeyboardFocusHere(); }
    z.seiteNameFokus = false;
    (void)eingabe("##seitename", &z.seiteName);
    const bool fertig = ImGui::IsItemDeactivatedAfterEdit() && ImGui::IsKeyPressed(ImGuiKey_Enter, false);
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(Str::MsUmbenennenTip)); }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) { ImGui::CloseCurrentPopup(); }   // das Feld hat den Fokus; ein Esc schliesst, nicht zwei
    if (fertig) {
        const std::string neu = utf8ZuLatin1(z.seiteName.c_str());
        const std::string& alt = d.datei.menus[static_cast<std::size_t>(mi)].window.name;
        bool belegt = false;
        for (int k = 0; k < anzahl; ++k) {
            belegt = belegt || (k != mi && gleichOhneFallStr(d.datei.menus[static_cast<std::size_t>(k)].window.name, neu));
        }
        z.seiteNameBelegt = belegt;
        z.seiteNameFokus = belegt;   // weiterschreiben, ohne erst ins Feld zu klicken
        if (!belegt && !neu.empty() && neu != alt) {
            std::vector<Ersetzung> e;
            if (menuUmbenennen(d.datei, mi, neu, e)) {
                aendere(std::move(e));
            } else {
                ausMakroMelden();
            }
        }
        if (!belegt) { ImGui::CloseCurrentPopup(); }
    }
    if (z.seiteNameBelegt) { ImGui::TextColored(ImVec4{1.0F, 0.75F, 0.3F, 1.0F}, "%s", tr(Str::MsNameBelegt)); }
    ImGui::Separator();
    if (ImGui::MenuItem(tr(Str::MsDuplizieren))) {
        const std::string alt = d.datei.menus[static_cast<std::size_t>(mi)].window.name;
        const Dokument* dp = &d;
        menuEinfuegen(d, alt, [dp, mi](const std::string& name) { return menuTextMitName(dp->datei, mi, name); });
    }
    if (ImGui::MenuItem(tr(Str::MsLoeschen), nullptr, false, anzahl > 1)) {
        std::vector<Ersetzung> e;
        if (menuLoeschen(d.datei, mi, e)) {
            d.auswahl.clear();
            aendere(std::move(e));
        } else {
            ausMakroMelden();
        }
    }
    for (const bool links : {true, false}) {
        if (ImGui::MenuItem(tr(links ? Str::MsNachLinks : Str::MsNachRechts), nullptr, false, links ? mi > 0 : mi + 1 < anzahl)) {
            std::vector<Ersetzung> e;
            if (menuTauschen(d.datei, links ? mi - 1 : mi, e)) {
                aendere(std::move(e));
                seiteVonMenue(d, links ? mi - 1 : mi + 1);
            } else {
                ausMakroMelden();
            }
        }
    }
    ImGui::EndPopup();
}

void zeichneSeiten(Dokument& d) {
    const std::vector<Seite> s = seitenImMenue(d.datei);
    if (!ImGui::BeginTabBar("##seiten", ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_TabListPopupButton)) { return; }
    // Seitenwechsel aus dem Code (Cursor, Gliederung, Skript): den Reiter waehlen
    const bool erzwingen = d.seite != d.seiteGezeigt;
    for (std::size_t k = 0; k < s.size(); ++k) {
        const std::string l = anzeige(s[k].titel) + "##s" + std::to_string(k);
        const ImGuiTabItemFlags f = (erzwingen && static_cast<int>(k) == d.seite) ? ImGuiTabItemFlags_SetSelected : 0;
        const bool offen = ImGui::BeginTabItem(l.c_str(), nullptr, f);
        const bool rechts = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right);
        if (offen) {
            if (!erzwingen && d.seite != static_cast<int>(k)) {
                d.seite = static_cast<int>(k);
                d.auswahl.clear();
            }
            d.seiteGezeigt = static_cast<int>(k);
            ImGui::EndTabItem();
        }
        if (rechts) {   // rc608 Teil 3b: Rechtsklick auf den Reiter verwaltet sein Menue
            Zustand& z = Z();
            z.seiteKontext = static_cast<int>(k);
            z.seiteName = anzeige(d.datei.menus[static_cast<std::size_t>(s[k].menu)].window.name);
            z.seiteNameBelegt = false;
            ImGui::OpenPopup("##seitekontext");
        }
    }
    zeichneSeitenKontext(d, s);
    if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip)) { ImGui::OpenPopup("##neueSeite"); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s\n%s", tr(Str::MeNeueSeite), tr(Str::MeVorlageTip)); }
    if (ImGui::BeginPopup("##neueSeite")) {   // M6 Punkt 5: leere Seite oder Vorlage eines ganzen Menues
        if (ImGui::MenuItem(tr(Str::MeVorlageLeer))) { neueSeite(d, -1); }
        const Str namen[] = {Str::MeVorlageHaupt, Str::MeVorlageEinst, Str::MeVorlageCredits, Str::MeVorlageLaden};
        for (int k = 0; k < 4; ++k) {
            if (ImGui::MenuItem(tr(namen[k]))) { neueSeite(d, k); }
        }
        ImGui::EndPopup();
    }
    ImGui::EndTabBar();
}

void splitter(const char* id, float& wert, bool senkrecht, float min, float max, float laenge, bool umgekehrt = false,
              ImGuiButtonFlags knopf = ImGuiButtonFlags_None) {
    ImGui::InvisibleButton(id, senkrecht ? ImVec2{6, std::max(1.0F, laenge)} : ImVec2{std::max(1.0F, laenge), 6}, knopf);
    // dock4: im DockSpace liegt dessen Wirt ueber dem Teiler - IsItemHovered meldet dort nichts, der Knopf selbst schon
    if (ImGui::IsItemHovered() || ImGui::IsItemActive() || GImGui->HoveredId == ImGui::GetItemID()) {
        ImGui::SetMouseCursor(senkrecht ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
    }
    if (ImGui::IsItemActive()) {
        const float dlt = senkrecht ? ImGui::GetIO().MouseDelta.x : -ImGui::GetIO().MouseDelta.y;
        wert += umgekehrt ? -dlt : dlt;
        wert = std::clamp(wert, min, max);
    }
}

}  // namespace

// ============================================================================

bool aktiv() { return g_app != nullptr && g_app->leftMode == 4; }

// --- Panels ---------------------------------------------------------------------------
//
// Jedes Panel zeichnet in den Platz, den es bekommt (GetContentRegionAvail), mit eigener,
// fester ID und Mauskoordinaten relativ zu sich selbst - damit es spaeter auch als eigenes,
// andockbares Fenster laufen kann.

// Hierarchie: Menues, Gruppen und Items (Ebenenliste)
void panelHierarchie(Dokument& d) {
    ImGui::PushID("panelHierarchie");
    zeichneGliederung(d);
    ImGui::PopID();
}

// Arbeitsflaeche: Seiten-Reiter und die 640x480-Flaeche
// rc608: nie still alles auf 0, 0 - das Spiel verwirft dieses Menue, und hier steht, wo und warum
void zeichneAbbruchZeile(Dokument& d) {
    const int mi = menuDerSeite(d);
    if (mi < 0 || d.datei.menus[static_cast<std::size_t>(mi)].abbruchZeile == 0) { return; }
    const Menu& m = d.datei.menus[static_cast<std::size_t>(mi)];
    std::vector<char> b(512);
    std::snprintf(b.data(), b.size(), tr(Str::MeMenuVerworfen), m.abbruchZeile);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0F, 0.45F, 0.35F, 1.0F});
    ImGui::TextWrapped("%s", b.data());
    ImGui::PopStyleColor();
    if (ImGui::SmallButton(tr(Str::MeZurZeile))) {
        Z().reiterGewollt = 1;
        springe(d, m.abbruchStelle);
    }
}

void panelFlaeche(Dokument& d) {
    ImGui::PushID("panelFlaeche");
    zeichneSeiten(d);
    zeichneTabLeiste(d);   // M6 Punkt 2
    zeichneGrenzenZeile(d);   // M6 Punkt 10
    zeichneAbbruchZeile(d);
    zeichneFremdZeile(d);
    if (Z().kinoZweiBild + 2 >= ImGui::GetFrameCount()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0F, 0.75F, 0.3F, 1.0F});
        ImGui::TextWrapped("%s", tr(Str::MeZweiVideos));
        ImGui::PopStyleColor();
    }
    zeichneFlaeche(d, ImGui::GetContentRegionAvail());
    ImGui::PopID();
}

// Text: der Code-Editor (Klick auf der Flaeche setzt springeZeile - steht beim Wechsel da),
// darunter die Probleme (Hoehe per Teiler)
void panelText(Dokument& d) {
    Zustand& z = Z();
    ImGui::PushID("panelText");
    const float fs = ImGui::GetFontSize();
    const ImVec2 r = ImGui::GetContentRegionAvail();
    if (z.problemeHoehe <= 0) { z.problemeHoehe = std::floor(r.y * 0.25F); }
    z.problemeHoehe = std::clamp(z.problemeHoehe, fs * 3, std::max(fs * 3, r.y - fs * 6));
    ImGui::BeginChild("##codeBereich", ImVec2{0, std::max(fs * 3, r.y - z.problemeHoehe - 8)}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    zeichneCode(d);
    ImGui::EndChild();
    splitter("##splProbleme", z.problemeHoehe, false, fs * 3, std::max(fs * 3, r.y - fs * 6), r.x);
    panelProbleme(d);
    ImGui::PopID();
}

void waehle(int item, bool dazu);   // unten

#include "app_menue_layout.inc"
#include "app_menue_ereignisse.inc"
namespace {
bool exportiere(const std::string& pfad, bool nurAenderungen);   // unten
}  // namespace
#include "app_menue_projekt.inc"
#include "app_menue_m6.inc"
#include "app_menue_m7.inc"

#include "app_menue_bildwahl.inc"
#include "app_menue_leiste.inc"
#include "app_menue_links.inc"

// Eigenschaften (auch das Layout "Text unten", dort stehen die Events darunter)
void panelEigenschaften(Dokument& d) {
    ImGui::PushID("panelEigenschaften");
    leisteSeite(d, 0);
    ImGui::PopID();
}

// Die rechte Spalte: Reiter, der gewaehlte steht in den Einstellungen. Das Programm
// wechselt ihn nur auf Wunsch (setzeReiter), nie ungefragt. Blau heisst nur: gewaehlt.
void panelReiter(Dokument& d) {
    Zustand& z = Z();
    Settings& st = g_app->settings;
    if (z.reiterGewollt == -2) { z.reiterGewollt = st.menueReiter == 0 ? 2 : std::clamp(st.menueReiter, 1, 3); }
    ImGui::PushStyleColor(ImGuiCol_Tab, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TabHovered, IM_COL32(0x2A, 0x2C, 0x32, 255));
    ImGui::PushStyleColor(ImGuiCol_TabSelected, IM_COL32(0x2A, 0x2C, 0x32, 255));
    ImGui::PushStyleColor(ImGuiCol_TabDimmed, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TabDimmedSelected, IM_COL32(0x2A, 0x2C, 0x32, 255));
    // rc608 Punkt 1: In schmalen Feldern werden die Reiter schmaler statt zu rollen, alle bleiben sichtbar; der Tooltip nennt den ganzen Namen
    const bool leiste = ImGui::BeginTabBar("##rechtsReiter", ImGuiTabBarFlags_FittingPolicyShrink);
    ImGui::PopStyleColor(5);
    if (leiste) {
        struct R {
            Str name;
            int nr;     // gespeicherte Nummer: 1 Text, 2 Properties, 3 Events; die Hierarchie (0) steht links
            int seite;  // leisteSeite
        };
        static const R reiter[] = {{Str::MeReiterEigenschaften, 2, 0}, {Str::MeEreignisse, 3, 1}, {Str::MeReiterText, 1, 2}};
        const int gewollt = z.reiterGewollt;
        for (const R& r : reiter) {
            const std::string l = std::string(tr(r.name)) + "###reiter" + std::to_string(r.nr);
            const ImGuiTabItemFlags f = r.nr == gewollt ? ImGuiTabItemFlags_SetSelected : 0;
            ImGui::PushStyleColor(ImGuiCol_Tab, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_TabHovered, IM_COL32(0x2A, 0x2C, 0x32, 255));
            ImGui::PushStyleColor(ImGuiCol_TabSelected, IM_COL32(0x2A, 0x2C, 0x32, 255));
            ImGui::PushStyleColor(ImGuiCol_TabDimmed, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_TabDimmedSelected, IM_COL32(0x2A, 0x2C, 0x32, 255));
            ImGui::PushStyleColor(ImGuiCol_Text, st.menueReiter == r.nr ? ImGui::GetColorU32(ImGuiCol_Text) : IM_COL32(0xA6, 0xA9, 0xB1, 255));
            const bool reiterOffen = ImGui::BeginTabItem(l.c_str(), nullptr, f);
            ImGui::PopStyleColor(6);
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tr(r.name)); }
            if (reiterOffen) {
                ImGui::GetWindowDrawList()->AddLine(ImVec2{ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y - 1.0F}, ImVec2{ImGui::GetItemRectMax().x, ImGui::GetItemRectMax().y - 1.0F}, IM_COL32(0x5A, 0x9B, 0xEA, 255), 2.0F);
                if (gewollt < 0 || r.nr == gewollt) { st.menueReiter = r.nr; }
                leisteSeite(d, r.seite);
                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) { z.werteBild = ImGui::GetFrameCount(); }
                ImGui::EndTabItem();
            }
        }
        z.reiterGewollt = -1;
        ImGui::EndTabBar();
    }
}

#include "app_menue_dock.inc"   // dock4: der Menu Editor im DockSpace

void zeichneInnen(float hoehe);

void zeichne(float hoehe) {
    const auto t0 = std::chrono::steady_clock::now();
    zeichneInnen(hoehe);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    Zustand& z = Z();
    z.bildzeitMs = z.bildzeitMs <= 0 ? ms : z.bildzeitMs * 0.9 + ms * 0.1;
}

void zeichneInnen(float hoehe) {
    datenPruefen();
    Zustand& z = Z();
    // M5b Punkt 3: vorgemerktes Projekt (Doppelklick), Sicherung suchen und pflegen, Fragen
    if (!z.projektVorgemerkt.empty()) {
        const std::string p = z.projektVorgemerkt;
        z.projektVorgemerkt.clear();
        (void)oeffneProjektJetzt(p);
    }
    if (!z.sicherungGeprueft) {
        z.sicherungGeprueft = true;
        if (z.projektDatei.empty()) { sicherungPruefen(sicherungsOrt()); }
    }
    sicherungPflegen();
    zeichneSicherungFrage();
    Dokument* d = doc();
    if (d != nullptr) { neuParsen(*d); }
    zeichneLeiste(d);
    // Dokument-Reiter
    if (z.docs.size() > 1 || d != nullptr) {
        if (ImGui::BeginTabBar("##menuedocs", ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_AutoSelectNewTabs)) {
            int schliessen = -1;
            // M2: aktiv aus dem Programm geaendert (schon offenes Dokument, Menuesatz-Wechsel):
            // den Reiter waehlen - sonst setzte die Reiterleiste den alten zurueck
            const int gewollt = z.aktiv != z.aktivGezeigt ? z.aktiv : -1;
            for (std::size_t k = 0; k < z.docs.size(); ++k) {
                bool offen = true;
                const bool geaendert = z.docs[k]->puffer.text() != z.docs[k]->gespeichert;
                const std::string l = z.docs[k]->name + (geaendert ? " *" : "") + "##d" + std::to_string(k);
                const ImGuiTabItemFlags f = static_cast<int>(k) == gewollt ? ImGuiTabItemFlags_SetSelected : 0;
                if (ImGui::BeginTabItem(l.c_str(), &offen, f)) {
                    if (gewollt < 0 || static_cast<int>(k) == gewollt) { z.aktiv = static_cast<int>(k); }
                    ImGui::EndTabItem();
                }
                if (!offen) { schliessen = static_cast<int>(k); }
            }
            ImGui::EndTabBar();
            z.aktivGezeigt = z.aktiv;
            if (schliessen >= 0) {   // M6 (Rest M5b): mit ungespeicherten Aenderungen erst fragen
                const Dokument& zu = *z.docs[static_cast<std::size_t>(schliessen)];
                if (zu.puffer.text() != zu.gespeichert) {
                    z.schliessenDok = schliessen;
                } else {
                    schliesseDokument(schliessen);
                    return;
                }
            }
        }
    }
    d = doc();
    if (d == nullptr) {
        z.halt = {};
        zeichneProjektLeer();   // M5b: Neues Projekt / Projekt oeffnen
        menueDockOhneMenue();   // dock4: Panels in eigenen Fenstern
        return;
    }
    neuParsen(*d);
    d->altStaende.clear();
    haltPruefen();
    fremdPruefen(*d);
    tourKlapptAuf();
    schlossAnSeite(*d, menuDerSeite(*d));
    editorTasten(*d);
    d = doc();
    if (d == nullptr) { return; }
    d->puffer.beiwerkFesthalten(anzeigeStand(*d));
    if (d->umbenennen >= 0 && d->umbenennenBild + 1 < ImGui::GetFrameCount()) { d->umbenennen = -1; }   // Zeile nicht mehr da
    const float fs = ImGui::GetFontSize();
    const ImVec2 rest = ImGui::GetContentRegionAvail();
    Settings& st = g_app->settings;
    (void)hoehe;
    // rc608 Teil 3b: Handbuchbilder: Das Menue steht gross im Bild, die Leisten nur so breit wie noetig (me::setzeSpalten(0, 0) stellt das wieder her)
    static const bool bilderBreit = std::getenv("BHED_MENUE_BILDER") != nullptr;
    if (bilderBreit && z.linksBreite <= 0) { z.linksBreite = fs * 11.0F; }
    if (bilderBreit && z.rechtsBreite <= 0) { z.rechtsBreite = fs * 17.0F; }
    if (z.linksBreite <= 0) { z.linksBreite = fs * 15; }
    // rc608 Punkt 2 (shank: "das bringt fast gar nichts weil ich viel zu wenig sehe"): ab Werk so breit, dass die Werte zu lesen sind; die gezogene Breite bleibt
    if (z.rechtsBreite <= 0) { z.rechtsBreite = st.menueRechtsBreite > 0 ? st.menueRechtsBreite * fs : (st.menueTextUnten ? fs * 19 : std::min(fs * 36.0F, rest.x * 0.42F)); }
    // rc608 Punkt 13 (shank: "ich kann auch rechts viel zu klein scalen das ich die Zahlen nicht mehr sehe"): Mindestbreiten, die Flaeche behaelt ihren Platz
    const float linksMin = fs * 11.0F, rechtsMin = fs * 16.0F, mitteMin = fs * 20.0F;
    // rc608 Teil 3b: Passt alles nicht (1920 x 1080 bei 200 %), geht die linke Spalte zu, sobald es eng wird, und die Flaeche wird brauchbar. Die Pfeile ueber der Flaeche oeffnen und schliessen beide.
    const bool eng = rest.x < linksMin + rechtsMin + mitteMin + 14.0F;
    if (eng != z.spaltenEng) {
        z.spaltenEng = eng;
        z.linksZu = eng;
    }
    if (z.reiterGewollt >= 0) { z.rechtsZu = false; }
    // rc608 Teil 3a: Die linke Spalte gibt nach, bevor die Flaeche unter ihr Mass faellt (1920 x 1080 bei 200 %)
    const float linksMax = std::max(linksMin, std::min(rest.x * 0.4F, rest.x - rechtsMin - mitteMin - 14.0F));
    z.linksBreite = std::clamp(z.linksBreite, linksMin, linksMax);
    const float rechtsMax = std::max(rechtsMin, rest.x - z.linksBreite - 14.0F - mitteMin);
    z.rechtsBreite = std::clamp(z.rechtsBreite, rechtsMin, rechtsMax);
    diagnosePflegen(*d);
    // "Reiter rechts": links die Bausteine, in der Mitte die Flaeche in voller Hoehe,
    // rechts die Reiter. "Text unten" (altes Layout): links Bausteine und Hierarchie,
    // rechts Eigenschaften, unten der Text.
    // dock1: ein Panel in einem eigenen Fenster (fenster_dock.h, dockFenster) fehlt mit
    // seiner Spalte und seinem Teiler, die Nachbarn nehmen den Platz.
    // dock4: im DockSpace misst spalte() nur; die Panels stehen danach in den Blaettern
    // eines Dock-Baums genau auf diesen Spalten (menueDockZeichnen).
    const bool dockRaum = menueDockBeginnen();
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const bool unten = st.menueTextUnten;
    // Ohne DockSpace kennt "Text unten" keine Panels - das Layout bleibt dort, wie es ist.
    const auto da = [&](const char* panel) { return (unten && !dockRaum) || dock::zuhause(panel); };
    const bool text = unten && da("menue.text");
    z.spaltenFrei = !unten && (!g_menueDock.aktiv || g_menueDock.qt);   // Hat jemand den Dock-Baum umgebaut, gehoeren die Spalten ihm
    const bool frei = z.spaltenFrei;
    const bool links = da("menue.bausteine") && !(frei && z.linksZu);
    const bool mitte = da("menue.flaeche");
    const bool rechts = da(unten ? "menue.eigenschaften" : "menue.reiter") && !(frei && z.rechtsZu);
    if (unten && z.codeHoehe <= 0) { z.codeHoehe = std::floor(rest.y * 0.38F); }
    const float h = text && (links || mitte || rechts) ? std::max(fs * 6, rest.y - z.codeHoehe - 8) : rest.y;
    if (links) {
        spalte("menue.bausteine", ImVec2{mitte || rechts ? z.linksBreite : 0.0F, h}, *d);
        if (mitte || rechts) {
            ImGui::SameLine(0, 0);
            spaltenTeiler("##spl1", z.linksBreite, true, linksMin, linksMax, h);
            ImGui::SameLine(0, 0);
        }
    }
    if (mitte) {
        const float mitteB = std::max(fs * 10, rest.x - (links ? z.linksBreite + 7 : 0.0F) - (rechts ? z.rechtsBreite + 7 : 0.0F));
        spalte("menue.flaeche", ImVec2{rechts ? mitteB : 0.0F, h}, *d);
        if (rechts) {
            ImGui::SameLine(0, 0);
            spaltenTeiler("##spl2", z.rechtsBreite, true, rechtsMin, rechtsMax, h, true);
            // Endrunde 4: Gemerkt wird nur, was der Anwender zieht. Vorher schrieb jedes Bild die Breite zurueck - auch die
            // Klemme bei wenig Platz (200 %, rechtsMax), und danach blieb die Spalte bei 100 % so schmal.
            if (!unten && ImGui::IsItemActive()) { st.menueRechtsBreite = std::floor(z.rechtsBreite / fs * 100.0F) / 100.0F; }
            ImGui::SameLine(0, 0);
        }
    }
    if (rechts) { spalte(unten ? "menue.eigenschaften" : "menue.reiter", ImVec2{0, h}, *d); }
    if (text) {
        if (links || mitte || rechts) { spaltenTeiler("##spl3", z.codeHoehe, false, fs * 4, rest.y - fs * 8, rest.x); }
        spalte("menue.text", ImVec2{0, 0}, *d);
    }
    if (dockRaum) { menueDockZeichnen(*d, ImRect{start, ImVec2{start.x + rest.x, start.y + rest.y}}); }
}

// dock1: die Panels des Menue-Editors in eigenen Fenstern. Nur im Layout mit den
// Reitern rechts - "Text unten" (Einstellungen) bleibt, wie es ist. dock4: im
// DockSpace beide Layouts, die Panels zeichnet dann menueDockZeichnen.
namespace {
bool panelVerfuegbar(const char* id) {
    if (g_app == nullptr || id == nullptr) { return false; }
    const bool unten = g_app->settings.menueTextUnten;
    const std::string p = id;
    if (p == "menue.eigenschaften" || p == "menue.text") { return unten && dockraumAn(); }
    if (p == "menue.reiter") { return !unten; }
    return !unten || dockraumAn();
}
}  // namespace

void dockFenster() {
    dock::setzeVerfuegbar(4, panelVerfuegbar);
    if (dockraumAn() || g_app == nullptr || g_app->settings.menueTextUnten) { return; }
    struct P {
        const char* id;
        void (*f)(Dokument&);
    };
    static const P liste[] = {{"menue.bausteine", panelBausteine}, {"menue.flaeche", panelFlaeche}, {"menue.reiter", panelReiter}};
    for (const P& p : liste) {
        if (!dock::draussen(p.id)) { continue; }
        if (dock::fensterBeginnen(p.id)) {
            Dokument* d = doc();
            if (d != nullptr) {
                p.f(*d);
            } else {
                ImGui::TextDisabled("%s", tr(Str::MeKeinMenue));
            }
        }
        dock::fensterEnde();
    }
}

// M2 Punkt 3: alle Dateien eines Menuesatzes oeffnen (UI_LoadMenus: loadMenu { ... })
int oeffneMenuesatz(const std::string& spielPfad) {
    datenPruefen();
    Zustand& z = Z();
    std::string t;
    if (!z.daten.lies(spielPfad, t)) {
        // auch ein offenes Dokument (z. B. eine eigene menus.txt)
        bool da = false;
        for (const auto& dd : z.docs) {
            if (kleinPfad(dd->name) == kleinPfad(spielPfad)) {
                t = dd->puffer.text();
                da = true;
            }
        }
        if (!da) { return 0; }
    }
    LeseOptionen o = leseOptionen(spielPfad);
    const Datei satz = lies(spielPfad, t, o);
    const int nr = z.naechsterSatz++;
    int n = 0;
    int erstes = -1;
    for (const std::string& f : satz.loadMenus) {
        std::string inhalt;
        bool da = z.daten.lies(f, inhalt);
        const bool ausDaten = da;
        const std::size_t vorher = z.docs.size();
        for (const auto& dd : z.docs) {   // schon offen (eigene Dateien): deren Text
            if (!da && kleinPfad(dd->name) == kleinPfad(f)) {
                inhalt = dd->puffer.text();
                da = true;
            }
        }
        if (!da) { continue; }
        if (oeffne(f, "", inhalt)) {
            doc()->satz = nr;
            if (z.docs.size() > vorher) { doc()->ausSpiel = ausDaten; }
            if (erstes < 0) { erstes = z.aktiv; }
            ++n;
        }
    }
    if (erstes >= 0) { z.aktiv = erstes; }   // die erste Datei des Satzes
    return n;
}

std::vector<BildZeile> bildTabelle() {
    Zustand& z = Z();
    datenPruefen();
    std::vector<BildZeile> tab;
    for (const std::string& pfad : z.daten.liste("ui/", {".menu"})) {
        std::string inhalt;
        if (!z.daten.lies(pfad, inhalt)) { continue; }
        std::size_t an = 0;
        int nr = 0;
        while (an < inhalt.size()) {
            std::size_t ende = inhalt.find('\n', an);
            if (ende == std::string::npos) { ende = inhalt.size(); }
            std::string zeile = inhalt.substr(an, ende - an);
            an = ende + 1;
            ++nr;
            if (const std::size_t k = zeile.find("//"); k != std::string::npos) { zeile.resize(k); }
            std::vector<std::string> w;
            for (std::size_t i = 0; i < zeile.size();) {
                const char c = zeile[i];
                if (std::isspace(static_cast<unsigned char>(c)) != 0 || c == '{' || c == '}' || c == ';') {
                    ++i;
                } else if (c == '"') {
                    const std::size_t q = zeile.find('"', i + 1);
                    const std::size_t e = q == std::string::npos ? zeile.size() : q;
                    w.push_back(zeile.substr(i + 1, e - i - 1));
                    i = e + 1;
                } else {
                    std::size_t e = i;
                    while (e < zeile.size() && std::isspace(static_cast<unsigned char>(zeile[e])) == 0 && std::strchr("{};\"", zeile[e]) == nullptr) { ++e; }
                    w.push_back(zeile.substr(i, e - i));
                    i = e;
                }
            }
            const auto eintrag = [&](const std::string& art, const std::string& name) {
                BildZeile b;
                b.datei = pfad;
                b.art = art;
                b.name = name;
                b.zeile = nr;
                b.da = art == "cinematic" ? z.daten.gibt(kinoPfad(name)) : bildRegistriert(name);
                tab.push_back(std::move(b));
            };
            if (w.size() >= 2) {
                const std::string a = kleinPfad(w[0]);
                if (a == "background" || a == "asset_shader" || a == "cinematic") { eintrag(a, w[1]); }
            }
            for (std::size_t i = 0; i < w.size(); ++i) {
                const std::string a = kleinPfad(w[i]);
                if (a == "setbackground" && i + 1 < w.size()) { eintrag(a, w[i + 1]); }
                if (a == "setitembackground" && i + 2 < w.size()) { eintrag(a, w[i + 2]); }
            }
        }
    }
    return tab;
}

std::vector<ItemLage> itemLagen() {
    std::vector<ItemLage> v;
    Dokument* d = doc();
    if (d == nullptr) { return v; }
    neuParsen(*d);
    const int mi = menuDerSeite(*d);
    if (mi < 0) { return v; }
    for (const Item& it : d->datei.menus[static_cast<std::size_t>(mi)].items) {
        ItemLage l;
        l.name = it.window.name;
        l.x = it.window.rectClient.x;
        l.y = it.window.rectClient.y;
        l.b = it.window.rectClient.w;
        l.h = it.window.rectClient.h;
        l.sx = it.window.rect.x;
        l.sy = it.window.rect.y;
        l.sichtbar = (it.window.flags & WINDOW_VISIBLE) != 0;
        v.push_back(l);
    }
    return v;
}

bool modellGriffe(float* xy8, float* g) {
    Zustand& z = Z();
    const Dokument* d = doc();
    if (d == nullptr || d->auswahl.size() != 1 || z.letztesAb.sx <= 0.0F) { return false; }
    const int mi = menuDerSeite(*d);
    if (mi < 0) { return false; }
    const Item& it = d->datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(d->auswahl[0])];
    if (it.type != ITEM_TYPE_MODEL) { return false; }
    const std::array<ImVec2, 4> m = gizmoGriffe(it.window.rect, z.letztesAb, z.letztesO);
    for (int i = 0; i < 4; ++i) {
        xy8[i * 2] = m[static_cast<std::size_t>(i)].x;
        xy8[i * 2 + 1] = m[static_cast<std::size_t>(i)].y;
    }
    *g = gizmoGroesse();
    return true;
}

int schachbrettZellen() { return Z().schachbrettZellen; }
int meldungenLeerBild() { return Z().meldungenLeerBild; }

TextAnsicht textAnsicht() {
    const Zustand& z = Z();
    TextAnsicht t;
    t.zeichen = z.sichtbareZeichen;
    t.breiteste = z.textBreiteste;
    t.flaeche = z.textFlaeche;
    t.zeilenHoehe = z.textMass[0];
    t.textX = z.textMass[1];
    t.leer = z.textMass[2];
    t.textY = z.textMass[3];
    t.zoom = g_app->settings.menueTextZoom;
    if (const Dokument* d = doc()) { t.scrollX = d->scrollX; }
    t.balken[0] = z.balkenRect.Min.x;
    t.balken[1] = z.balkenRect.Min.y;
    t.balken[2] = z.balkenRect.GetWidth();
    t.balken[3] = z.balkenRect.GetHeight();
    return t;
}

void setzeSpalten(float links, float rechts) {
    Z().linksBreite = links;
    Z().rechtsBreite = rechts;
}
float rechteSpalte() { return Z().rechtsBreite; }
float linkeSpalte() { return Z().linksBreite; }
std::vector<std::string> eigenschaftsFelder() { return Z().felderGezeigt; }
std::vector<std::string> weitereEintraege() { return Z().weitereEintraege; }

void setzeBildDialog(std::function<std::string()> antwort) { Z().bildDialog = std::move(antwort); }
void setzeMenueDialog(std::function<std::string()> antwort) { Z().menueDialog = std::move(antwort); }
bool ablageRahmen(float* xywh) {
    const Zustand& z = Z();
    xywh[0] = z.ablageRect.x;
    xywh[1] = z.ablageRect.y;
    xywh[2] = z.ablageRect.w;
    xywh[3] = z.ablageRect.h;
    return z.ablageAn;
}

SchreibStand schreibStand() {
    const auto& s = Z().schreib;
    SchreibStand x;
    x.an = s.an;
    x.item = s.item;
    x.puffer = s.puffer;
    x.cursorX = s.cursorX;
    x.cursorY = s.cursorY;
    x.textRechts = s.textRechts;
    return x;
}

std::string letzteMeldung() { return Z().meldung; }

bool oeffneAusSpiel(const std::string& spielPfad) {
    datenPruefen();
    std::string t;
    if (!Z().daten.lies(spielPfad, t)) { return false; }
    const std::size_t vorher = Z().docs.size();
    if (!oeffne(spielPfad, "", t)) { return false; }
    if (Z().docs.size() > vorher) { doc()->ausSpiel = true; }   // ein schon offenes Dokument gleichen Namens bleibt, was es ist
    return true;
}

bool oeffneDatei(const std::string& platte) {
    if (istProjektDatei(platte)) { return oeffneProjektJetzt(platte); }   // M5b
    const std::string k = kleinPfad(platte);
    if (k.size() < 5 || (k.compare(k.size() - 5, 5, ".menu") != 0 && k.compare(k.size() - 4, 4, ".txt") != 0)) { return false; }
    datenPruefen();
    std::ifstream f(utf8Pfad(platte), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    return oeffne(spielPfadVon(platte), platte, s.str());
}

bool oeffneText(const std::string& name, const std::string& text) {
    datenPruefen();
    return oeffne(name, "", text);
}

std::vector<std::string> platzhalterImSpiel();   // M7 (app_menue_m7.inc)

Stand stand() {
    Stand s;
    Zustand& z = Z();
    s.dokumente = static_cast<int>(z.docs.size());
    Dokument* d = doc();
    if (d == nullptr) { return s; }
    neuParsen(*d);
    s.menus = static_cast<int>(d->datei.menus.size());
    for (const Menu& m : d->datei.menus) { s.items += static_cast<int>(m.items.size()); }
    s.seite = d->seite;
    s.seiten = static_cast<int>(seitenImMenue(d->datei).size());
    s.auswahl = d->auswahl;
    s.text = d->puffer.text();
    s.meldungen = static_cast<int>(d->datei.meldungen.size());
    s.prims = z.letztePrims;
    s.asienGlyphen = z.asienPrims;
    s.hudBilder = z.hudPrims;
    s.hudNachgebildet = hudNachgebildet(d->datei);
    s.klangListe = z.klangListe;
    s.spiel = static_cast<int>(z.spiel);
    for (const auto& [k, t] : z.texturen) { (void)k; s.bilderGeladen += t.tex != nullptr ? 1 : 0; }
    s.interaktion = z.interaktion;
    s.cursorZeile = std::string(d->puffer.zeile(d->puffer.pos(d->puffer.cursor.front().pos).zeile));
    s.exporte = z.exporte;
    s.undoSchritte = d->puffer.undoTiefe();
    s.zoomProzent = d->zoomProzent;
    if (const int mi = menuDerSeite(*d); mi >= 0) { s.abbruchZeile = d->datei.menus[static_cast<std::size_t>(mi)].abbruchZeile; }
    for (int i = 0; i < 4; ++i) { s.flaeche[i] = z.flaeche[i]; }
    s.bildzeitMs = z.bildzeitMs;
    s.kinoBilder = z.kinoBilder;
    s.klangStimmen = static_cast<int>(z.klangStimmen.size());
    s.verborgen = static_cast<int>(d->verborgen.size());
    s.gesperrt = static_cast<int>(d->gesperrt.size());
    s.modellAufrufe = z.modellAufrufe;
    s.modellMs = z.modellMs;
    s.modellZeichnungen = z.modellZeichnungen;
    s.modellStufen = z.modellStufen;
    s.beispielPrims = z.beispielPrims;
    s.platzhalterPrims = z.platzhalterPrims;
    s.ladebalken = z.ladebalken;
    s.klangEreignisse = z.klangEreignisse;
    s.letzterKlang = z.letzterKlang;
    for (const Diagnose& x : d->diagnosen) {
        (x.schwere == Schwere::Fehler ? s.diagFehler : x.schwere == Schwere::Warnung ? s.diagWarnungen : s.diagHinweise) += 1;
        s.diagnosen.push_back(std::to_string(x.zeile) + ": " + diagMeldung(x));
    }
    s.diagAktuell = d->diagVersion == d->puffer.version();
    s.diagLaeufe = z.diagLaeufe;
    s.modellFehler = z.modellFehler;
    for (const auto& [p, b] : d->bilder) {
        (void)b;
        s.paket.push_back(p);
    }
    s.paketDateien = static_cast<int>(s.paket.size());
    if (g_codeSchrift != nullptr) {   // Festbreite: i und W gleich breit
        const float wi = g_codeSchrift->CalcTextSizeA(16.0F, 1e9F, 0.0F, "iiii").x;
        const float ww = g_codeSchrift->CalcTextSizeA(16.0F, 1e9F, 0.0F, "WWWW").x;
        s.codeFestbreite = wi > 0 && std::fabs(wi - ww) < 0.01F;
    }
    return s;
}

// M4: den Video-Import-Dialog fuer eine Datei zeigen ("" = schliessen), fuer Selbsttest und Handbuch
void zeigeVideoImport(const std::string& platte) {
    Zustand& z = Z();
    z.videoQuelle = platte;
    z.videoOffen = !platte.empty();
}

// M4: Nummer des Items mit diesem Namen im Menue der Seite (-1 = keins)
int itemNummer(const std::string& name) {
    Dokument* d = doc();
    if (d == nullptr) { return -1; }
    const int mi = menuDerSeite(*d);
    if (mi < 0) { return -1; }
    const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
    for (std::size_t i = 0; i < m.items.size(); ++i) {
        if (kleinPfad(m.items[i].window.name) == kleinPfad(name)) { return static_cast<int>(i); }
    }
    return -1;
}

void waehle(int item, bool dazu) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    if (item < 0) {   // keins: Auswahl leeren
        if (!dazu) { d->auswahl.clear(); }
        return;
    }
    if (const int mi0 = menuDerSeite(*d); mi0 < 0 || static_cast<std::size_t>(item) >= d->datei.menus[static_cast<std::size_t>(mi0)].items.size()) { return; }
    const auto it = std::find(d->auswahl.begin(), d->auswahl.end(), item);
    if (dazu) {
        if (it != d->auswahl.end()) { d->auswahl.erase(it); } else { d->auswahl.push_back(item); }
    } else {
        d->auswahl.assign(1, item);
    }
    const int mi = menuDerSeite(*d);
    if (mi >= 0 && item >= 0 && static_cast<std::size_t>(item) < d->datei.menus[static_cast<std::size_t>(mi)].items.size()) {
        springe(*d, d->datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(item)].anfang);
    }
}

void verschiebeAuswahl(float dx, float dy) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    const int mi = menuDerSeite(*d);
    if (mi < 0) { return; }
    const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
    std::vector<std::pair<int, Rechteck>> neu;
    for (const int i : d->auswahl) {
        if (static_cast<std::size_t>(i) >= m.items.size() || d->gesperrt.count(i) != 0) { continue; }
        Rechteck r = m.items[static_cast<std::size_t>(i)].window.rectClient;
        r.x += dx;
        r.y += dy;
        neu.emplace_back(i, r);
    }
    setzeRects(neu);
}

void setzeInteraktion(bool an) {
    Zustand& z = Z();
    z.interaktion = an;
    z.laufDatei = -1;
    z.laufVersion = -1;
    z.laufMausX = -1.0F;
    z.laufMausY = -1.0F;
    z.lauf.cvars.clear();
    z.lauf.protokoll.clear();
}

void klickVorschau(float x, float y) {
    Zustand& z = Z();
    Dokument* d = doc();
    if (d == nullptr || !z.interaktion) { return; }
    const int mi = menuDerSeite(*d);
    if (z.laufDatei != z.aktiv || z.lauf.d != &d->datei || z.laufGeparst != d->geparst) {
        z.laufGeparst = d->geparst;
        z.lauf.start(d->datei);
        z.lauf.saberModell = saberModellFuer;
        z.lauf.basis = umgebung(*d);
        if (mi >= 0) { z.lauf.oeffne(d->datei.menus[static_cast<std::size_t>(mi)].window.name); }
        z.laufDatei = z.aktiv;
    }
    z.lauf.maus(mi, x, y);
    z.lauf.klick(mi, x, y);
    (void)satzWechsel();
}

void setzeReiter(int reiter) { Z().reiterGewollt = std::clamp(reiter, 0, 3); }
LeisteStand leisteStand() {
    const Zustand::Leiste& l = Z().leiste;
    LeisteStand s;
    s.hilfe = l.hilfe;
    s.proben = l.probenGelaufen;
    s.probeLaeuft = l.probeStufe != 0;
    s.ausloeser = l.trig;
    s.offenerSchritt = l.auf;
    s.tablett = l.tablett;
    s.farbeOffen = l.farbeOffen;
    s.schluesselZeilen = l.schluesselZeilen;
    s.gemischt = l.gemischtGezeigt;
    s.textOben = l.textMass.oben;
    s.textUnten = l.textMass.unten;
    return s;
}
void sucheZu() { Z().suchenOffen = false; }
void vorlageWeg(const std::string& id) {
    (void)loescheVorlage(vorlagenOrdner(), id);
    Z().vorlagenGeladen = false;
}
void leisteAlleOffen() { Z().leiste.zu.clear(); }
void leisteVorgabe() {
    Zustand::Leiste& l = Z().leiste;
    l.zu = Zustand::Leiste{}.zu;
    l.tablett = 0;
}
void leisteTablett(int nr) { Z().leiste.tablett = nr; }
bool bildWahlOffen() { return Z().wahl.istOffen; }
void setzeTextScroll(float zeile) {
    if (Dokument* d = doc()) {
        d->scrollZeile = zeile;
        d->scrollX = 0.0F;
    }
}
std::vector<KachelLage> kachelLagen() {
    std::vector<KachelLage> v;
    if (Z().kachelnBild + 2 < ImGui::GetFrameCount()) { return v; }   // die Spalte ist zu: nichts gezeichnet
    for (const KachelMass& k : Z().kacheln) { v.push_back(KachelLage{k.name, k.rect.Min.x, k.rect.Min.y, k.rect.Max.x, k.rect.Max.y, k.textBreite, k.textUnten}); }
    return v;
}
float kachelnScrollMaxX() { return Z().kachelnScrollMaxX; }
void schliesseTestFrage() { Z().testFrageZu = true; }

bool korrigiereDiagnose(int nr) {
    Dokument* d = doc();
    if (d == nullptr || nr < 0 || static_cast<std::size_t>(nr) >= d->diagnosen.size()) { return false; }
    const Diagnose x = d->diagnosen[static_cast<std::size_t>(nr)];
    return korrigiere(*d, x);
}
int reiter() { return g_app->settings.menueReiter; }

void setzeCvar(const std::string& name, const std::string& wert) {
    Zustand& z = Z();
    if (z.interaktion) { z.lauf.setzeCvar(name, wert); }
    else { z.vorschauCvars[kleinPfad(name)] = wert; }
}

void setzeDatapadWaffe(int waffe) {
    datapadDatenLaden();
    DatapadWerte& w = Z().datapadWerte;
    w.waffe = std::clamp(waffe, 1, 13);
    w.waffen |= 1U << w.waffe;
}

std::string cvarWert(const std::string& name) {
    Zustand& z = Z();
    if (z.interaktion) { return z.lauf.cvar(name); }
    const auto it = z.vorschauCvars.find(kleinPfad(name));
    return it == z.vorschauCvars.end() ? std::string() : it->second;
}

// rc608: Rueckgaengig stellt auch Seite und Auswahl wieder her (festgehalten in anzeigeFesthalten)
void anzeigeHolen(Dokument& d) {
    const std::vector<int>& b = d.puffer.beiwerk();
    if (b.size() < 3 || b[2] < 0 || b.size() < 3U + static_cast<std::size_t>(b[2])) { return; }
    d.seite = b[0];
    paketAufStand(d, b[1]);
    const auto da = [&d](int m, int i) {
        return m >= 0 && static_cast<std::size_t>(m) < d.datei.menus.size() && i >= 0 && static_cast<std::size_t>(i) < d.datei.menus[static_cast<std::size_t>(m)].items.size();
    };
    const int mi = menuDerSeite(d);
    const auto ab = b.begin() + 3;
    d.auswahl.clear();
    for (auto it = ab; it != ab + b[2]; ++it) {
        if (da(mi, *it)) { d.auswahl.push_back(*it); }
    }
    // Ein Element, das eben wiederkam (Rueckgaengig nach dem Loeschen), hat keinen Vorgaenger: Es bekommt Schloss und Auge aus dem Stand
    for (std::size_t k = 3U + static_cast<std::size_t>(b[2]); k + 2 < b.size(); k += 3) {
        const int art = b[k], m = b[k + 1], i = b[k + 2];
        if (!da(m, i) || d.abgebildet.count({m, i}) != 0) { continue; }
        if (art == 1) { d.verborgen.insert({m, i}); }
        else if (m == d.gesperrtMenue) { d.gesperrt.insert(i); }
        else { d.gesperrtAndere[m].insert(i); }
    }
}

bool rueckgaengig() {
    Dokument* d = doc();
    if (d == nullptr) { return false; }
    haltSchreiben();
    if (!d->puffer.rueckgaengig()) { return false; }
    neuParsen(*d);
    anzeigeHolen(*d);
    return true;
}

bool wiederholen() {
    Dokument* d = doc();
    if (d == nullptr) { return false; }
    haltSchreiben();
    if (!d->puffer.wiederholen()) { return false; }
    neuParsen(*d);
    anzeigeHolen(*d);
    return true;
}

void fuegeEin(int typ) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    const int mi = menuDerSeite(*d);
    if (mi < 0) { return; }
    setzeAblageEin(ablageWidget(typ), false, 0, 0);
}

int suche(const std::string& muster, bool regex) {
    Dokument* d = doc();
    if (d == nullptr) { return 0; }
    Zustand& z = Z();
    z.such = muster;
    z.suchOpt.regex = regex;
    z.suchenOffen = true;
    return static_cast<int>(d->puffer.suche(muster, z.suchOpt).size());
}

int ersetzeAlle(const std::string& muster, const std::string& ersatz, bool regex) {
    Dokument* d = doc();
    if (d == nullptr) { return 0; }
    SuchOptionen o;
    o.regex = regex;
    const int n = d->puffer.ersetzeAlle(muster, ersatz, o);
    neuParsen(*d);
    return n;
}

void springeZuItem(int item) { waehle(item, false); }

std::string kopiereAuswahl() {
    Dokument* d = doc();
    if (d == nullptr) { return {}; }
    const int mi = menuDerSeite(*d);
    std::vector<int> a = d->auswahl;
    std::sort(a.begin(), a.end());
    std::string t;
    for (const int i : a) { t += itemText(d->datei, mi, i); }
    return t;
}

// Einen oder mehrere itemDef-Bloecke ans Ende des Menues; jedes bekommt einen freien Namen.
void fuegeItemsEin(const std::string& text) {
    Dokument* d = doc();
    if (d == nullptr || text.find("itemDef") == std::string::npos) { return; }
    const int mi = menuDerSeite(*d);
    if (mi < 0) { return; }
    Menu namen = d->datei.menus[static_cast<std::size_t>(mi)];   // fuer freie Namen
    std::string alles;
    std::size_t p = 0;
    int neu = 0;
    while (true) {
        const std::size_t a = text.find("itemDef", p);
        if (a == std::string::npos) { break; }
        std::size_t b = text.find("itemDef", a + 7);
        // Zeilenanfang des naechsten Blocks
        if (b != std::string::npos) {
            while (b > a && text[b - 1] != '\n') { --b; }
        }
        std::size_t s = a;
        while (s > p && text[s - 1] != '\n') { --s; }
        const std::string block = text.substr(s, (b == std::string::npos ? text.size() : b) - s);
        Item basis;
        LeseOptionen o;
        o.dialekt = d->datei.dialekt;
        o.spiel = d->datei.spiel;
        const Datei probe = lies("ui/kopie.menu", "{ menuDef { " + block + " } }", o);
        std::string alt = (!probe.menus.empty() && !probe.menus[0].items.empty()) ? probe.menus[0].items[0].window.name : "item";
        const std::string frei = freierName(namen, alt);
        basis.window.name = frei;
        namen.items.push_back(basis);
        alles += umbenennen(block, frei, d->datei.dialekt);
        if (!alles.empty() && alles.back() != '\n') { alles += d->puffer.zeilenUmbruch(); }
        ++neu;
        if (b == std::string::npos) { break; }
        p = b;
    }
    std::vector<Ersetzung> e;
    if (neu > 0 && itemEinfuegen(d->datei, mi, alles, e)) {
        aendere(std::move(e));
        const int n = static_cast<int>(d->datei.menus[static_cast<std::size_t>(mi)].items.size());
        d->auswahl.clear();
        for (int i = n - neu; i < n; ++i) { d->auswahl.push_back(i); }
    }
}

// rc608 Teil 3a: Einfuegen und Duplizieren: Die Kopie liegt sonst genau auf dem Original, und man sieht sie nicht. Einfuegen und Ruecken sind ein Schritt.
void einfuegenOhneDeckung(const std::string& text) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    const std::string vorher = d->puffer.text();
    fuegeItemsEin(text);
    neuParsen(*d);
    const int mi = menuDerSeite(*d);
    if (mi < 0) { return; }
    const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
    std::vector<Rechteck> alte, neue;
    for (std::size_t i = 0; i < m.items.size(); ++i) {
        const bool neu = std::find(d->auswahl.begin(), d->auswahl.end(), static_cast<int>(i)) != d->auswahl.end();
        (neu ? neue : alte).push_back(m.items[i].window.rect);
    }
    // Die Auswahl rueckt gemeinsam, um denselben Versatz, bis keines ihrer Rechtecke mehr deckt
    float versatz = 0.0F;
    for (int runde = 0; runde < 40; ++runde) {
        const bool frei = std::all_of(neue.begin(), neue.end(), [&](const Rechteck& r) {
            const Rechteck probe{r.x + versatz, r.y + versatz, r.w, r.h};
            const Rechteck g = ohneDeckung(probe, alte);
            return g.x == probe.x && g.y == probe.y;
        });
        if (frei) { break; }
        versatz += 20.0F;
    }
    if (versatz > 0.0F) { verschiebeAuswahl(versatz, versatz); }
    (void)zuEinemSchritt(vorher);
    neuParsen(*d);
}

void dupliziereAuswahl() { einfuegenOhneDeckung(kopiereAuswahl()); }

std::vector<int> rahmenTexte() { return Z().rahmenTexteGezeigt; }

void loescheAuswahl() {
    Dokument* d = doc();
    if (d == nullptr || d->auswahl.empty()) { return; }
    const int mi = menuDerSeite(*d);
    std::vector<Ersetzung> e;
    for (const int i : d->auswahl) {
        if (d->gesperrt.count(i) != 0) { continue; }
        (void)itemLoeschen(d->datei, mi, i, e);
    }
    d->auswahl.clear();
    // rc609 ME1: Entf, waehrend die Maus noch zieht: Der Zug gehoerte den geloeschten Elementen und endet ohne Wirkung.
    // Sonst las das Loslassen nach dem Skalieren d.auswahl[0] der leeren Auswahl (STL-Haertung: Abbruch, exit 127).
    Zustand& z = Z();
    z.ziehen = Ziehen::Nichts;
    z.ziehLaeuft = false;
    z.klickWahl = -1;
    z.zdx = z.zdy = 0;
    z.linien.clear();
    z.gizmoArt = 0;
    z.gizmoItem = -1;
    aendere(std::move(e));
}

// M5b Punkt 3: Projekte
bool oeffneProjekt(const std::string& datei) { return oeffneProjektJetzt(datei); }
void projektVormerken(const std::string& datei) { Z().projektVorgemerkt = datei; }
bool istProjekt(const std::string& datei) { return istProjektDatei(datei); }
bool legeProjektAn(const std::string& datei) { return neuesProjekt(datei); }
void neuesProjektFormat(bool mehrspieler) { Z().neuDialekt = mehrspieler ? Dialekt::MP : Dialekt::SP; }
void setzeProjektDialog(std::function<std::string(bool)> antwort) { Z().projektDialog = std::move(antwort); }
void testSpielUebernehmen(unsigned long pid, const std::string& ordner) {
    Zustand& z = Z();
    z.testPid = pid;
    z.testUiLoad = true;
    z.testOrdnerLose = ordner;
}
int testNeuGeladen() { return Z().testNeuGeladen; }
void setzeEigenschaftAuswahl(const std::string& schluessel, const std::vector<std::string>& werte) {
    Dokument* d = doc();
    if (d == nullptr || d->auswahl.empty()) { return; }
    std::vector<Ersetzung> e;
    if (setzeEigenschaft(d->datei, menuDerSeite(*d), d->auswahl[0], schluessel.c_str(), werte, e)) { aendere(std::move(e)); }
}
std::vector<std::string> hierarchieZeilen() { return Z().hierarchieZeilen; }
void setzeSeite(int seite) {
    if (Dokument* d = doc()) { d->seite = seite; }
}

std::string fensterTitel() {
    const Dokument* d = doc();
    if (d == nullptr) { return "JK Studio"; }
    return d->name + (d->puffer.text() != d->gespeichert ? " *" : "") + " - JK Studio";
}

std::string dokumentName() {
    const Dokument* d = doc();
    return d != nullptr ? d->name : std::string();
}
bool dokumentMp() {
    const Dokument* d = doc();
    return d != nullptr && d->datei.dialekt == Dialekt::MP;
}
bool speichereProjektJetzt() { return speichereProjekt(); }
bool exportiereProjektJetzt() { return exportiereProjekt(); }
std::string projektDatei() { return Z().projektDatei; }
void sichereJetzt() { sicherungJetzt(); }
void sucheSicherung() { sicherungPruefen(sicherungsOrt()); }
void schliesseAlleOhneSpeichern() {
    Zustand& z = Z();
    z.pruefFenster = false;
    z.pruefGelaufen = false;
    z.docs.clear();
    z.aktiv = -1;
    z.laufDatei = -1;
    z.projektDatei.clear();
    z.projekt = Projekt{};
}
void fragZuordnung() { Z().zuordnungFrage = true; }
FlussInfo flussInfo() {
    Zustand& z = Z();
    FlussInfo r;
    r.fensterOffen = z.flussOffen;
    r.assistentUeberlappung = z.assistentUeberlappung;
    r.stileOffen = z.stileOffen;
    r.assistentOffen = z.assistentOffen;
    const Fluss& f = flussAktuell();
    r.kanten = static_cast<int>(f.kanten.size());
    for (const FlussMenue& n : f.menues) {
        if (n.datei < 0) {
            ++r.fehlend;
            continue;
        }
        ++r.menues;
        r.unerreichbar += n.erreichbar ? 0 : 1;
        r.sackgassen += n.sackgasse ? 1 : 0;
        r.ohneZurueck += (n.ohneZurueck && !n.sackgasse) ? 1 : 0;
    }
    if (Dokument* d = doc()) {
        const int mi = menuDerSeite(*d);
        if (mi >= 0) { r.aktivesMenue = d->datei.menus[static_cast<std::size_t>(mi)].window.name; }
    }
    return r;
}
std::vector<std::string> tabFolge() {
    std::vector<std::string> r;
    Dokument* d = doc();
    if (d == nullptr) { return r; }
    neuParsen(*d);
    const int mi = menuDerSeite(*d);
    for (int i : tabFolgeVon(*d, mi)) { r.push_back(d->datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(i)].window.name); }
    return r;
}
bool tabFolgeAn() { return Z().tabFolge; }
std::vector<std::string> assistentKatalog() {
    std::vector<std::string> r;
    if (Dokument* d = doc()) {
        for (const CvarSteuerung& c : assistentKatalog(d->datei.dialekt)) { r.push_back(c.cvar); }
    }
    return r;
}
void assistentWaehle(const std::string& cvar) { Z().assistentWahl.insert(kleinPfad(cvar)); }
std::vector<std::string> originalUnterschiede() {
    Dokument* d = doc();
    const Datei* o = d != nullptr ? originalVon(*d) : nullptr;
    const int mi = d != nullptr ? menuDerSeite(*d) : -1;
    if (o == nullptr || mi < 0 || d->auswahl.size() != 1) { return {}; }
    const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
    const Item& it = m.items[static_cast<std::size_t>(d->auswahl[0])];
    for (const Menu& om : o->menus) {
        if (!gleichOhneFallStr(om.window.name, m.window.name)) { continue; }
        for (const Item& oi : om.items) {
            if (oi.window.name == it.window.name) { return itemUnterschiede(d->datei, it, *o, oi); }
        }
    }
    return {};
}
std::string itemNameBei(int item) {
    Dokument* d = doc();
    const int mi = d != nullptr ? menuDerSeite(*d) : -1;
    if (mi < 0 || item < 0 || static_cast<std::size_t>(item) >= d->datei.menus[static_cast<std::size_t>(mi)].items.size()) { return {}; }
    return d->datei.menus[static_cast<std::size_t>(mi)].items[static_cast<std::size_t>(item)].window.name;
}
int menuesImDokument() {
    Dokument* d = doc();
    return d == nullptr ? 0 : static_cast<int>(d->datei.menus.size());
}
bool referenzbildGeladen() { return Z().refTex.tex != nullptr; }
void ladeReferenzbild(const std::string& platte) { refBildLaden(platte); }
void entferneReferenzbild() { refBildEntfernen(); }
bool schliessenFrageOffen() { return Z().schliessenDok >= 0; }
void schliesseReiter(int dok) {
    Zustand& z = Z();
    if (dok < 0 || static_cast<std::size_t>(dok) >= z.docs.size()) { return; }
    const Dokument& d = *z.docs[static_cast<std::size_t>(dok)];
    if (d.puffer.text() != d.gespeichert) {
        z.schliessenDok = dok;
    } else {
        schliesseDokument(dok);
    }
}
TourWerte tourWerte() {
    Zustand& z = Z();
    TourWerte w;
    w.projekt = !z.projektDatei.empty();
    w.spieltests = z.spielTests;
    w.reiterProp = g_app->settings.menueReiter == 2;
    w.reiterEvents = g_app->settings.menueReiter == 3;
    w.trigHover = z.leiste.trig == "mouseEnter";
    w.trigKlick = z.leiste.trig == "action";
    w.gespeichert = z.projektGespeichert;
    Dokument* d = doc();
    if (d == nullptr) { return w; }
    neuParsen(*d);
    const int mi = menuDerSeite(*d);
    if (mi < 0) { return w; }
    const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
    w.elemente = static_cast<int>(m.items.size());
    w.layouts = static_cast<int>(liesLayouts(d->datei, mi).size());
    for (const Item& it : m.items) {
        for (const std::string* t : {&it.mouseEnter, &it.mouseExit, &it.action, &it.onFocus, &it.leaveFocus, &it.accept}) { w.ereignisse += t->empty() ? 0 : 1; }
    }
    return w;
}
// M5b Punkt 4: Mod-Profile
void setzeModProfil(const std::string& name) { setzeProfil(name); }
std::string schriftNameMp(int nummer) {
    schriftNamenLaden();
    const auto& v = Z().schriftNamenMp;
    return nummer >= 0 && static_cast<std::size_t>(nummer) < v.size() ? v[static_cast<std::size_t>(nummer)] : std::string();
}
std::string modProfil() { return aktivesProfil()->name; }
bool legeProfilVorlageAn(const std::string& pfad) { return profilVorlageSchreiben(pfad); }
void beimBeenden() {   // ungespeicherte Arbeit bleibt in der Sicherung (nur, wenn der Editor in dieser Sitzung lief)
    if (Z().sicherungGeprueft) { sicherungJetzt(); }
}

void setzeSpeichernDialog(std::function<std::string()> antwort) { Z().speichernDialog = std::move(antwort); }

bool speichereUnter(const std::string& pfad) {
    Dokument* d = doc();
    if (d == nullptr) { return false; }
    speichern(*d, pfad);
    return d->platte == pfad;
}

bool zuEinemSchritt(const std::string& vorher) {
    Dokument* d = doc();
    if (d == nullptr) { return false; }
    const std::string nachher = d->puffer.text();
    if (nachher == vorher) { return true; }
    int zurueck = 0;
    while (zurueck < 500 && d->puffer.text() != vorher && d->puffer.rueckgaengig()) { ++zurueck; }
    if (d->puffer.text() != vorher) {
        // nicht erreichbar (Grenze der Liste): alles wiederherstellen, einzeln lassen
        for (int i = 0; i < zurueck && d->puffer.wiederholen(); ++i) {}
        return false;
    }
    d->puffer.ersetze(0, static_cast<std::uint32_t>(vorher.size()), nachher);
    return d->puffer.text() == nachher;
}

namespace {
bool exportiere(const std::string& pfad, bool nurAenderungen) {
    Zustand& z = Z();
    Dokument* d = doc();
    if (d == nullptr) { return false; }
    if (imSpielordner(pfad)) {
        z.meldung = tr(Str::MeNieSpielordner);
        return false;
    }
    const std::vector<Pk3Datei> dateien = paketDateien(nurAenderungen);
    if (dateien.empty()) {
        z.meldung = tr(Str::MeExportNichts);
        addStatus(z.meldung);
        return false;
    }
    std::string fehler;
    const bool ok = writePk3(pfad, dateien, &fehler);
    z.exporte += ok ? 1 : 0;   // H2: fuer die Tour
    char b[600];
    std::snprintf(b, sizeof(b), ok ? tr(Str::MeExportiert) : tr(Str::MeNichtGespeichert), pfad.c_str());
    z.meldung = b;
    // M2 Punkt 7: Warnung beim Export, wenn die Engine etwas verliert (Grenzen)
    const int verlust = pruefeAlleMenues();
    if (ok && verlust > 0) {
        char w[200];
        std::snprintf(w, sizeof(w), trN(Str::MePruefungAnzahl, verlust), verlust);
        z.meldung += "  " + std::string(w);
        z.pruefFenster = true;
    }
    z.letzterExport = static_cast<int>(dateien.size());
    addStatus(z.meldung);
    return ok;
}
}  // namespace

std::vector<std::string> pruefZeilen() { return Z().pruefZeilen; }
std::array<float, 2> leisteFeldStelle(const char* id) {
    const ImVec2 p = Z().feldStelle[id];
    return {p.x, p.y};
}
bool linkeSpalteZu() { return Z().linksZu; }
void oeffneLinkeSpalte() { Z().linksZu = false; }   // wie der Pfeil ueber der Flaeche
bool zweiVideosGemeldet() { return Z().kinoZweiBild + 2 >= ImGui::GetFrameCount(); }

void testPaketAblegen(const std::string& ordner) {
    std::error_code ec;
    for (const Pk3Datei& f : testPaketDateien()) {
        const fs::path ziel = utf8Pfad(ordner) / utf8Pfad(f.name);
        fs::create_directories(ziel.parent_path(), ec);
        std::ofstream o(ziel, std::ios::binary);
        o.write(f.inhalt.data(), static_cast<std::streamsize>(f.inhalt.size()));
    }
}

std::string paketDatei(const std::string& name, bool test) {
    for (const Pk3Datei& f : test ? testPaketDateien() : paketDateien(false)) {
        if (kleinPfad(f.name) == kleinPfad(name)) { return f.inhalt; }
    }
    return {};
}

bool exportierePk3(const std::string& pfad) { return exportiere(pfad, false); }
bool exportiereAenderungen(const std::string& pfad) { return exportiere(pfad, true); }
int letzterExport() { return Z().letzterExport; }
int pruefeFuerSpiel(bool ja, bool jk2, bool md) {
    Zustand& z = Z();
    z.pruefZiel[0] = ja;
    z.pruefZiel[1] = jk2;
    z.pruefZiel[2] = md;
    (void)pruefeAlleMenues();
    return static_cast<int>(z.pruefErgebnis.size());
}
std::string imSpielTesten() { return menueImSpielTesten(false); }
bool baueSchriftAusTtf(const std::string& ttf, const std::string& name, int groesse) { return schriftBauen(ttf, name, groesse); }
void setzeVorschauSprache(const std::string& sprache) { setzeSprache(sprache); }
bool legeTextAn(const std::string& schluessel, const std::map<std::string, std::string>& texte) {
    Dokument* d = doc();
    if (d == nullptr) { return false; }
    eigeneTexteLesen(*d);
    TextZeile r;
    r.schluessel = schluessel;
    for (const auto& [sprache, t] : texte) { r.text[sprache] = textFuerDatei(sprache, t); }
    d->eigeneTexte.push_back(r);
    eigeneTexteSchreiben(*d);
    return true;
}
std::string loeseText(const std::string& at) {
    Dokument* d = doc();
    return d == nullptr ? std::string() : vorschauTexte(*d).aufloesen(at);
}
void setzeHudWerte(const HudWerte& w) { Z().hudWerte = w; }
void klangListeLeeren() { Z().klangListe.clear(); }
std::string paketInhalt(const std::string& spielPfad) {
    Dokument* d = doc();
    if (d == nullptr) { return {}; }
    for (const auto& [p, b] : d->bilder) {
        if (kleinPfad(p) == kleinPfad(spielPfad)) { return b; }
    }
    return {};
}
std::string textPaket() {
    Dokument* d = doc();
    if (d == nullptr) { return {}; }
    eigeneTexteLesen(*d);
    return d->textPaket;
}

void setzeCodeSchrift(ImFont* f) { g_codeSchrift = f; }

void setzeAufloesung(int nr) { Z().aufloesung = nr; }
void setzeBalken(bool balken) { Z().seitenv = balken ? Seitenverhaeltnis::Balken : Seitenverhaeltnis::Strecken; }

std::string letzterSatzWechsel() { return Z().satzWechsel; }

void sperreItem(int item, bool sperren) {
    Dokument* d = doc();
    if (d == nullptr || menuDerSeite(*d) != d->gesperrtMenue || item < 0) { return; }
    if (static_cast<std::size_t>(item) >= d->datei.menus[static_cast<std::size_t>(d->gesperrtMenue)].items.size()) { return; }
    if (sperren) { d->gesperrt.insert(item); } else { d->gesperrt.erase(item); }
}

std::vector<ItemMerker> itemMerker() {
    std::vector<ItemMerker> v;
    const Dokument* d = doc();
    const int mi = d != nullptr ? menuDerSeite(*d) : -1;
    if (mi < 0) { return v; }
    const Menu& m = d->datei.menus[static_cast<std::size_t>(mi)];
    const bool schlossHier = d->gesperrtMenue == mi;
    const auto andere = d->gesperrtAndere.find(mi);
    for (std::size_t i = 0; i < m.items.size(); ++i) {
        const int n = static_cast<int>(i);
        ItemMerker x;
        x.name = m.items[i].window.name;
        x.gruppe = m.items[i].window.group;
        x.text = m.items[i].text;
        x.gesperrt = schlossHier ? d->gesperrt.count(n) != 0 : andere != d->gesperrtAndere.end() && andere->second.count(n) != 0;
        x.verborgen = d->verborgen.count({mi, n}) != 0;
        v.push_back(std::move(x));
    }
    return v;
}

void verbergeItem(int item, bool verbergen) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    const int mi = menuDerSeite(*d);
    if (mi < 0 || item < 0 || static_cast<std::size_t>(item) >= d->datei.menus[static_cast<std::size_t>(mi)].items.size()) { return; }
    if (verbergen) { d->verborgen.insert({mi, item}); } else { d->verborgen.erase({mi, item}); }
}

bool ebeneAnStelle(int item, int ziel) {
    Dokument* d = doc();
    return d != nullptr && setzeAnStelle(*d, menuDerSeite(*d), item, ziel);
}

void altKlickVorschau(float x, float y) {
    Dokument* d = doc();
    if (d == nullptr) { return; }
    const int mi = menuDerSeite(*d);
    const int i = naechstesUnter(itemsUnterPunkt(*d, mi, x, y, umgebung(*d)), d->auswahl);
    if (i >= 0) { waehle(i, false); }
}

std::string laufInfo() {
    Zustand& z = Z();
    Dokument* d = doc();
    std::string s = "aktiv " + std::to_string(z.aktiv) + (d != nullptr ? " " + d->name + " satz " + std::to_string(d->satz) : std::string());
    for (const std::string& p : z.lauf.protokoll) { s += " | " + p; }
    return s;
}

bool fuegeVorlageEin(const std::string& id, float x, float y) {
    const std::vector<Vorlage>& v = vorlagen();
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i].id == id) {
            setzeAblageEin(static_cast<int>(i), true, x, y);
            return true;
        }
    }
    return false;
}

bool speichereAuswahlAlsVorlage(const std::string& name) {
    Dokument* d = doc();
    if (d == nullptr || d->auswahl.empty()) { return false; }
    Z().vorlagenGeladen = false;
    return speichereVorlage(vorlagenOrdner(), name, kopiereAuswahl());
}

std::vector<std::string> vorlagenIds() {
    std::vector<std::string> r;
    for (const Vorlage& v : vorlagen()) { r.push_back(v.id); }
    return r;
}

std::string importiereVideo(const std::string& platte, const VideoImport& o, std::string* fehler) {
    VideoEinst e;
    e.ziel.breite = o.breite;
    e.ziel.hoehe = o.hoehe;
    e.ziel.einpassen = static_cast<Einpassen>(std::clamp(o.einpassen, 0, 2));
    e.guete = o.guete;
    e.fps = o.fps;
    e.ton = o.ton;
    e.stereo = o.stereo;
    const VideoErgebnis r = videoKodieren(platte, e, {});
    if (fehler != nullptr) { *fehler = r.fehler.empty() && r.ohneTon ? std::string(tr(Str::MeVideoOhneTon)) : r.fehler; }
    if (r.roq.empty()) { return {}; }
    const std::string name = videoNameAus(platte);
    videoAblegen(name, r.roq, static_cast<VideoVerwendung>(std::clamp(o.verwendung, 0, 3)));
    return "video/" + name + ".roq";
}

void legeInsPaket(const std::string& spielPfad, const std::string& bytes) {
    if (Dokument* d = doc()) {
        d->bilder[spielPfad] = bytes;
        Z().kinos.clear();
        Z().texturen.clear();
    }
}

}  // namespace bhed::gui::menueeditor

namespace bhed::gui {
void setMenueCodeSchrift(ImFont* schrift) { menueeditor::setzeCodeSchrift(schrift); }
}  // namespace bhed::gui
